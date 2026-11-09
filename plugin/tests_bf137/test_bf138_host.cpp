// test_bf138_host.cpp -- lsBookFlow 1.38.0 (2-min show rule, cluster rule, % real from LRA-RealOrder-ES.csv) on the mock host (2026-11-09, Rassul 12:55 CT "do it"): the TOP BOOK (TopBook.h 1.0.0) fed by the
//   pane's own order feed posts SignalBoard kind G once a second: 3 real bids below / 3 real offers above, ranks 1-6, the "?" / S20 bits,
//   the code 99 heartbeat; young orders not posted; a reconnect = a cold book; the status line; versions agree.
// Build (tests_bf137/): g++ -std=c++17 -g -fsanitize=address,undefined -Wall -Wextra -Wno-misleading-indentation -DOFF_TEST_CLOCK
//        -DPC_TEST_CURSOR -I.. -I../tests_aggbook/mock test_bf137_host.cpp -o th && ./th
#include "BookFlow.cpp"
typedef BookFlow Ind;
namespace off { long long off_test_now_ms = 1000000; void* off_test_cursor_wnd = nullptr; }
static long long& NOW = off::off_test_now_ms;
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
namespace mock { Host* current = nullptr; Feed feed; }
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, std::string(m).c_str()); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void dispose(Ind* p) { p->destroy(); p->~Ind(); ::operator delete(p); }
static Ind* make() { Ind* p = static_cast<Ind*>(CreateExtension()); static bool once = false; if (!once) { once = true; p->setup(); } return p; }
static const unsigned long T0 = 1791900000UL;
static mock::Bar bar(unsigned long t) { mock::Bar b; b.time = t; b.close = 7000.0f; b.open = 7000.0f; b.low = 6999.5f; b.high = 7001.0f; b.volume = 100; b.rows.push_back(mock::Price(7000.0f, 50, 50, 100)); return b; }
static int nextId = 1;
static void ev(unsigned char a, unsigned char side, float px, unsigned long size, RTDATE t, int id) { mock::feed.push(a, side, px, size, t, id); }
int main() {
    char tmpl[] = "/tmp/bf137XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mkdir(flex.c_str(), 0700);
    { std::ofstream f((flex + "\\BookFlow.place-ES.txt").c_str()); f << "20,0,20,15,10,0,0,0,0,0\n"; }
    { std::ofstream f((flex + "\\LRA-RealOrder-ES.csv").c_str()); f << "# test\nstatus,LIVE\nnear,6.0\nintercept,0.2244\ncoef,size_3SD,-0.0803\ncoef,size_S,2.6107\ncoef,age_10s_2m,-2.0420\ncoef,age_2_10m,-1.9084\ncoef,age_10m_plus,-2.4108\ncoef,clustered,0.7329\ncoef,posted_5_25t,0.1391\ncoef,posted_25t_plus,-0.3190\ncoef,rth,-0.4733\n"; }
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ6"; h.root = "ES"; h.chart = "ES 3m #1"; h.paneBottom = 300; h.paneRight = 2000; h.scaleLeft = 1980; h.ppb = 10; h.barW = 20; h.listIndex = 0;
    for (int i = 0; i < 31; ++i) h.bars.push_back(bar(T0 + 180UL * i));
    Ind* p = make();
    CHECK(std::string(BF_VERSION) == "1.38.0" && std::string(bf::VERSION) == "1.38.0" && readAll("../OrderFlowPaneSettings.h").find("BF_VERSION = \"1.38.0\"") != std::string::npos && std::string(tbk::VERSION) == "1.1.0", "versions 1.38.0 (BF / bf / settings spec) + TopBook 1.1.0");
    CHECK(readAll("../BookFlow.cpp").find("p->setVersion(\"1.38.0\")") != std::string::npos, "the setVersion literal 1.38.0");
    p->calc(0); h.draws.clear(); p->draw();
    // the book: 1-lot noise every tick, 60 ticks each side of 7000.00 / 7000.25; a trade at 7000.00; big bids 6997.00 (100) / 6995.00 (60),
    // offers 7003.00 (80) / 7005.00 (50 + 1 = one order "?") / 7008.00 (70); an offer 7001.00 (200) inside 2 ticks
    const RTDATE t0 = (RTDATE)(T0 + 180UL * 30 + 1);
    for (int d = 0; d < 60; ++d) { ev(0, 1, 7000.0f - 0.25f * d, 1, t0, nextId++); ev(0, 0, 7000.25f + 0.25f * d, 1, t0, nextId++); }
    { const int r = nextId++; ev(0, 1, 7000.0f, 1, t0, r); ev(3, 1, 7000.0f, 1, t0, r); }
    ev(0, 1, 6997.0f, 100, t0, nextId++); ev(0, 1, 6995.0f, 60, t0, nextId++);
    ev(0, 0, 7003.0f, 40, t0, nextId++); ev(0, 0, 7003.0f, 40, t0, nextId++); ev(0, 0, 7005.0f, 50, t0, nextId++); ev(0, 0, 7008.0f, 35, t0, nextId++); ev(0, 0, 7008.0f, 35, t0, nextId++);
    ev(0, 0, 7000.5f, 200, t0, nextId++);
    NOW += 300; p->calc(0); NOW += 1100; p->calc(0);
    Ind::Ext* e = p->ext(*p->testState());
    std::vector<sb::Fact> G; bool ok = sb::readFacts('G', "ESZ6", 180, "ES 3m #1", BookFlow::steadyNow(), G, false);
    long long hb = -1; int lv = 0; for (auto& f : G) { if (f.code == 99) hb = (long long)f.val; else ++lv; }
    CHECK(ok && hb > 100 && lv == 0, "young orders (< 30 s): only the heartbeat is posted (book " + std::to_string(hb) + ", levels " + std::to_string(lv) + ")");
    for (int k = 0; k < 160; ++k) { NOW += 1000; p->calc(0); }   // 30 s rest + 2 min to show
    ok = sb::readFacts('G', "ESZ6", 180, "ES 3m #1", BookFlow::steadyNow(), G, false);
    std::map<double, sb::Fact> at; for (auto& f : G) if (f.code != 99) at[f.lo] = f;
    for (auto& kv : at) std::printf("  G %.2f side %d val %.0f rank %d orders %lld bits %d\n", kv.first, kv.second.side, kv.second.val, kv.second.code, kv.second.bar, kv.second.proven);
    CHECK(ok && at.size() == 3, "3 levels posted: 7003 (2 x 40) and 7008 (2 x 35) are CLUSTERS - two big orders in the same tick posted together (" + std::to_string(at.size()) + ")");
    CHECK(at.count(6997.0) && at[6997.0].side == 1 && at[6997.0].val == 101 && at[6997.0].code == 1, "bid 6997.00 = 100 + the 1-lot noise, rank 1 = K (the 200 at 7000.50 is inside 2 ticks)");
    CHECK(!at.count(7003.0) && !at.count(7008.0), "clustered levels not posted");
    CHECK(e->tb.lastWhy().cluster >= 2, "counted as cluster");
    CHECK(at.count(7005.0) && (at[7005.0].proven & 2) && (at[7005.0].proven & 4), "offer 7005.00 one 50-lot order: '?' and S20 (2 min, never moved)");
    { bool any = false; for (auto& kv : at) if ((kv.second.proven >> 3) > 0) any = true; CHECK(any == e->tbModelLive, "% real posted only with a LIVE model (" + std::to_string(e->tbModelLive) + ")"); }
    for (int k = 0; k < 60; ++k) { NOW += 1000; p->calc(0); }
    { sb::readFacts('G', "ESZ6", 180, "ES 3m #1", BookFlow::steadyNow(), G, false); bool s20 = false; for (auto& f : G) if (f.lo == 7005.0 && (f.proven & 4)) s20 = true; CHECK(s20, "still S20"); }
    CHECK(!at.count(7000.5), "7000.50 (inside 2 ticks) never posted");
    {   int k1 = 0; for (auto& kv : at) if (kv.second.code == 1) { ++k1; CHECK(kv.first == 6997.0 || kv.second.val == 100, "rank 1 = the biggest"); } CHECK(k1 == 1, "one rank 1"); }
    {   NOW += 40000; p->calc(0); const std::string st = readAll(flex + "\\BookFlow\\status-ES-ESZ6-180.txt");
        { size_t i = st.find("TOP_BOOK"); std::printf("  %s\n", i == std::string::npos ? "no TOP_BOOK line" : st.substr(i, 200).c_str()); }
        CHECK(st.find("TOP_BOOK,TopBook 1.1.0,S20 ") != std::string::npos && st.find("posted G ") != std::string::npos, "status TOP_BOOK line"); }
    // a reconnect = a cold book: heartbeat 0, no levels
    p->testState()->feed.reconnects += 1; NOW += 1100; p->calc(0);
    ok = sb::readFacts('G', "ESZ6", 180, "ES 3m #1", BookFlow::steadyNow(), G, false); lv = 0; hb = -1; for (auto& f : G) { if (f.code == 99) hb = (long long)f.val; else ++lv; }
    CHECK(ok && hb == 0 && lv == 0, "reconnect: cold book, nothing ranked (" + std::to_string(hb) + " / " + std::to_string(lv) + ")");
    CHECK(e->tbFaults == 0, "no faults");
    dispose(p);
    std::printf("BookFlow 1.38.0 host (TopBook): %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
