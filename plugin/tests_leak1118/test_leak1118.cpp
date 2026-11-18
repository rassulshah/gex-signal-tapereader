// test_leak1118.cpp -- 2026-11-18 memory / freeze fixes (Rassul 09:53-10:14 CT "consider the memory leaks because irt has been freezing alot", "start now"):
//   OrderFlowFeed 1.17.0 the queue is routed in slices, in order; BookLevelBox 1.2.0 the book cap keeps depth consistent; ZoneRead 1.2.0 prune.
// Build (tests_leak1118/): g++ -std=c++17 -g -fsanitize=address,undefined -Wall -Wextra -Wno-misleading-indentation -DOFF_TEST_CLOCK -DPC_TEST_CURSOR
//        -I.. -I../tests_aggbook/mock test_leak1118.cpp -o tl && ./tl
#include "BookFlow.cpp"
namespace off { long long off_test_now_ms = 1000000; void* off_test_cursor_wnd = nullptr; }
namespace mock { Host* current = nullptr; Feed feed; }
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, std::string(m).c_str()); } } while (0)
int main() {
    // ---- 1. the queue in slices, in order
    {   off::Conn c; const int N = 150000;
        for (int i = 0; i < N; ++i) { off::Conn::QEv e; std::memset(&e.o, 0, sizeof(e.o)); e.o.orderID = (decltype(e.o.orderID))i; e.ms = i; c.q.push_back(e); }
        long long expect = 0; bool inOrder = true; size_t calls = 0, total = 0;
        auto route = [&](const DEPTH_ORDER& o, long long ms) { if ((long long)o.orderID != expect || ms != expect) inOrder = false; ++expect; };
        while (c.queued() > 0 && calls < 100) { const size_t d = c.flushQueue(route, 25); CHECK(d <= off::FLUSH_MAX, "a slice <= FLUSH_MAX"); total += d; ++calls;
            // new events arrive between calls: appended behind the backlog
            if (calls == 1) for (int i = 0; i < 10; ++i) { off::Conn::QEv e; std::memset(&e.o, 0, sizeof(e.o)); e.o.orderID = (decltype(e.o.orderID))(N + i); e.ms = N + i; c.q.push_back(e); } }
        CHECK(calls >= 4, "150,000 events need >= 4 calls (" + std::to_string(calls) + ")");
        CHECK(inOrder && expect == N + 10 && total == (size_t)N + 10, "every event routed once, in order");
        CHECK(c.q.empty() && c.qHead == 0, "queue emptied");
        // unlimited = the old behaviour
        for (int i = 0; i < 1000; ++i) { off::Conn::QEv e; std::memset(&e.o, 0, sizeof(e.o)); c.q.push_back(e); }
        size_t n = 0; c.flushQueue([&](const DEPTH_ORDER&, long long) { ++n; }); CHECK(n == 1000 && c.queued() == 0, "no budget = all at once");
    }
    // ---- 2. BookLevelBox cap: the depth stays equal to the sum of the book
    {   blb::Engine E; E.market = "ES"; long long id = 1; const long long px = 28000;
        for (long long s = 1000; s < 1000 + 200; ++s) {
            for (int k = 0; k < 1000; ++k) { const long long p = px + (k % 2 ? 1 : -1) * (1 + (k * 7) % 600); E.onEvent(s, 0, k % 2 ? 1 : 0, p, 1 + k % 5, id++, px, px + 1); }
            E.onEvent(s, 3, 0, px, 1, 999999999LL, px, px + 1);
        }
        CHECK(E.book.size() <= blb::BOOK_CAP, "book capped (" + std::to_string(E.book.size()) + ")");
        CHECK(E.trimmed > 0, "trimmed some");
        long long sum[2] = {0, 0}; for (const auto& kv : E.book) if (std::llabs(kv.second.pt - E.base - blb::DEPTH_N / 2) < blb::DEPTH_N / 2) sum[kv.second.side] += kv.second.sz;
        long long dep[2] = {0, 0}; for (int s = 0; s < 2; ++s) for (long long v : E.dep[s]) dep[s] += v;
        CHECK(sum[0] == dep[0] && sum[1] == dep[1], "depth = the book after trimming (" + std::to_string(sum[0]) + "/" + std::to_string(dep[0]) + ")");
        blb::Engine F; F.market = "ES"; long long fid = 1;
        F.onEvent(5000, 3, 0, px, 1, 777, px, px + 1);                                     // a trade: price known
        for (int k = 0; k < 20000; ++k) F.onEvent(5000, 0, 0, px - 450 - k % 50, 1, fid++, px, px + 1);   // far, old
        while (F.book.size() < blb::BOOK_CAP) F.onEvent(5001, 0, 0, px - 1 - (long long)(fid % 300), 1, fid++, px, px + 1);   // near, newer
        F.onEvent(5002, 0, 1, px + 2, 1, fid++, px, px + 1);                                 // one more: the trim
        size_t far = 0; for (const auto& kv : F.book) if (std::llabs(kv.second.pt - px) > 400) ++far;
        CHECK(far == 0 && F.book.size() <= blb::BOOK_CAP * 3 / 4 + 1 && F.book.count(fid - 1) == 1, "far orders leave first, then the oldest; the newest stays (" + std::to_string(far) + ")");
    }
    // ---- 3. ZoneRead prune
    {   zread::Engine Z; Z.tick = 0.25; Z.hw = 4;
        std::vector<zrule::Level> L(1); L[0].px = 7000.0; L[0].name = "PDL"; L[0].kind = 0; Z.setLevels(L);
        for (int i = 0; i < 40000; ++i) Z.onEvent(1000 + i, zread::NEW, zread::BID, i + 1, 28000 + (i % 9) - 4, 3);
        CHECK(Z.ord.size() == 40000, "band orders kept");
        L[0].px = 7100.0; Z.setLevels(L);
        CHECK(Z.ord.empty() && Z.pruned == 40000, "a moved level: the old band's orders leave (" + std::to_string(Z.ord.size()) + ")");
        L[0].px = 7000.0; Z.setLevels(L);
        for (int i = 0; i < (int)zread::ORD_CAP; ++i) Z.onEvent(100000 + i, zread::NEW, zread::BID, 100000 + i, 28000, 3);
        for (int i = 0; i < 1000; ++i) Z.onEvent(200000 + i, zread::FILL, zread::ASK, 100000 + i, 28000, 3);   // filled to 0
        Z.onEvent(300000, zread::NEW, zread::BID, 999999, 28000, 5);
        CHECK(Z.ord.count(999999) == 1 && Z.orderDrops == 0, "at the cap, empty orders make room before a new one is refused");
    }
    std::printf("%s %d pass, %d fail\n", fails ? "FAILED" : "ALL PASS", passes, fails); return fails ? 1 : 0;
}
