// test_lp073_host.cpp -- lsLiquidityProfile 0.73.0 on the mock host (tests_lqp044/irtsdk.h, the REAL LiquidityProfile.cpp + LiqTouch 1.6.0):
// the RESTART REPLAY of Rassul's 2026-11-03 7:49 PM CT audit. An old 300-lot bid at 7830.00 rests from before the restart: the order feed
// never sees it, only the depth book shows it. After the restart a new 400-lot bid at 7810.00 (outside today's reachable range) and 30-lot
// bids near price arrive. Checks: 0-10 min WARMING (status WARMING, SignalBoard B code 9 state 1, NO king code 3); after 10 min READY (code 9
// state 0), the old bid seeded from depth is a whole-book node AND the king (code 3, bid 7830, >= 300 lots); the 400 bid outside the range
// is a node but not the king; LRA-LiqZones Z rows carry field 7 (x normal); an IRT restart -> warming again (no king for 10 min).
// LP073_WRITE=<path> also writes the posted B facts (code,side,price,lots,state,proven) for test_dealerprofile_2522.cpp (end-to-end).
// From plugin\:  g++ -std=c++17 -O1 -fsanitize=address,undefined -Itests_lqp044 -I. tests_liqtouch/test_lp073_host.cpp -o tlp073h && ./tlp073h
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <unistd.h>
#define private public
#include "LiquidityProfile.cpp"
#undef private
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void bar(mock::Host& h, unsigned long t, float o, float hi, float lo, float c) { mock::Bar b; b.time = t; b.open = o; b.high = hi; b.low = lo; b.close = c; b.rows.push_back(mock::Price(c, 50, 40)); h.bars.push_back(b); }
static std::string findFile(const std::string& tmpDir, const std::string& part) {
    const std::string base = tmpDir.substr(tmpDir.rfind('/') + 1); std::string cmd = "ls /tmp 2>/dev/null | grep -a '^" + base + "'"; FILE* p = popen(cmd.c_str(), "r"); char b[512]; std::string r;
    while (p && fgets(b, sizeof(b), p)) { std::string n(b); if (!n.empty() && n.back() == '\n') n.pop_back(); if (n.find(part) != std::string::npos && n.size() > part.size() && n.compare(n.size() - 4, 4, ".bak") != 0) r = "/tmp/" + n; }
    if (p) pclose(p); return r; }
static std::vector<sb::Fact> facts(LQState* S) { std::vector<sb::Fact> B; sb::readFacts('B', S->symbol, S->spb, S->chartKey, -1, B, false); return B; }
static const sb::Fact* code(const std::vector<sb::Fact>& B, int c, double px = -1) { for (const sb::Fact& f : B) if (f.code == c && (px < 0 || std::fabs(f.lo - px) < 1e-6)) return &f; return nullptr; }
static std::string line(const std::string& s, const char* key) { const size_t a = s.find(std::string("\n") + key); if (a == std::string::npos) return ""; return s.substr(a + 1, s.find('\n', a + 1) - a - 1); }
int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/lp073XXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const unsigned long day = (unsigned long)(civilDays(2026, 11, 3) * 86400LL);
    mock::Host h; mock::current = &h;
    h.tick = 0.25f; h.seconds = 180; h.symbol = "EPZ26"; h.root = "EPZ26";
    h.priceBase = 7800.0f; h.pxPer = 10.0f; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
    const unsigned long t0 = day + 12 * 3600 + 30 * 60;   // 12:30 CT
    for (int d = 12; d >= 1; --d) for (int k = 0; k < 3; ++k) bar(h, day - d * 86400 + 12 * 3600 + k * 180, 7865.0f, 7880.0f, 7850.0f, 7865.0f);   // 12 prior sessions: ADR 30
    for (int i = 0; i < 31; ++i) bar(h, t0 - (30 - i) * 180, 7853.0f, 7856.0f, 7850.0f, 7853.0f);   // today: high 7856, low 7850 -> kings 7826 .. 7880
    // the depth book (MBP): 10 lots a level, 160 levels a side; the OLD 300-lot bid at 7830.00 (placed before the restart: never in the feed)
    for (int i = 0; i < 160; ++i) { DEPTH_LEVEL L; L.bid = (float)(7853.0 - i * 0.25); L.ask = (float)(7853.25 + i * 0.25); L.bidsize = std::fabs(L.bid - 7830.0f) < 1e-3 ? 300 : 10; L.asksize = 10; h.depth.push_back(L); }
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    auto run = [&](unsigned long from, unsigned long to) { for (unsigned long s = from; s <= to; ++s) { h.now = s; p->calc(0); usleep(1500); } h.draws.clear(); p->draw(); };
    int id = 1000;
    auto prints = [&](unsigned long from, unsigned long to) { for (unsigned long s = from; s < to; s += 3) mock::feed.push(3, 0, 7853.25f, 1, (RTDATE)s, 12, ++id); };
    for (int s = 0; s < 3; ++s) { h.now = t0 - 70 + s; p->calc(0); usleep(2000); }   // the feed opens, the session starts
    // after the restart: the new orders
    for (int i = 0; i < 5; ++i) mock::feed.push(0, 1, 7852.0f - 1.0f * i, 30, (RTDATE)(t0 - 60), 300 + i);
    mock::feed.push(0, 1, 7810.0f, 400, (RTDATE)(t0 - 60), 50);                                   // a big NEW bid outside today's reachable range
    mock::feed.push(0, 0, 7853.25f, 400, (RTDATE)(t0 - 60), 12);
    for (int i = 1; i <= 12; ++i) mock::feed.push(0, 0, 7853.25f + 0.25f * i, 5, (RTDATE)(t0 - 60), 200 + i);
    prints(t0 - 60, t0);
    run(t0 - 67, t0);
    auto live = [&](unsigned long from, unsigned long to) { for (unsigned long q = from; q < to; q += 3) { prints(q, q + 3); run(q + 1, q + 3); } };   // events arrive as the clock runs (as live)
    live(t0, t0 + 240);
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    {   const std::vector<sb::Fact> B = facts(S); const sb::Fact* w = code(B, 9);
        CHECK(w && w->state == 1 && w->val >= 240 && w->val < 600, "5 min after the restart: B code 9 = warming (tracked ~300 s)");
        CHECK(code(B, 3) == nullptr, "no king (code 3) posted while warming");
        S->lastBuild = -1; p->calc(0);   // the status now (it is built at most once a second of real time)
        const std::string st = readAll(findFile(tmp, "LiquidityProfile.status-ES-180.txt"));
        CHECK(line(st, "WARMING,").find("WARMING,warming,tracked") == 0, "status WARMING,warming");
        std::printf("  %s\n", line(st, "WARMING,").c_str()); }
    live(t0 + 240, t0 + 640);
    S = static_cast<LQState*>(h.data);
    {   const std::vector<sb::Fact> B = facts(S); const sb::Fact* w = code(B, 9);
        CHECK(w && w->state == 0 && w->val >= 600, "after 10 min: B code 9 = ready");
        CHECK(w && w->proven >= 0 && w->proven < 100, "code 9 carries the share of the depth the order book explains (%)");
        const sb::Fact* k = code(B, 3);
        CHECK(k && std::fabs(k->lo - 7830.0) < 1e-6 && k->side == 1 && k->val >= 300, "THE KING (code 3) = the old 300 bid at 7830.00, seeded from depth");
        CHECK(k && k->proven >= 300, "king x its side's median (x100) >= 3.0x");
        const sb::Fact* n30 = code(B, 4, 7830.0); const sb::Fact* n10 = code(B, 4, 7810.0);
        CHECK(n30 && n30->val >= 300, "the old bid is a whole-book node (code 4)");
        CHECK(n10 && n10->val == 400, "the 400 bid outside today's range is a node (bigger) but not the king");
        int k3 = 0; double big = 0; for (auto& f : B) if (f.code == 3) { ++k3; big = std::max(big, f.val); }
        CHECK(k3 >= 1 && k3 <= 2 && k && big == k->val, "at most one king a side; the bid king is the bigger (the offer side holds only 10-lot pre-restart levels)");
        S->lastBuild = -1; p->calc(0);
        const std::string st = readAll(findFile(tmp, "LiquidityProfile.status-ES-180.txt"));
        CHECK(line(st, "WARMING,").find("WARMING,ready,tracked") == 0 && line(st, "WARMING,").find("king posted (B code 3) 2") != std::string::npos, "status WARMING,ready + kings posted 2 (bid 320 + a 30-lot offer of pre-restart levels)");
        CHECK(line(st, "DRAW,").find("DRAW,only >= 2.0x the book behind AND >= 1.5x a normal band") == 0, "status DRAW: the wall rule names both gates (still starts \"DRAW,only >= 2.0x\")");
        CHECK(line(st, "KING_NODES,").find("lots at 31320") != std::string::npos, "status KING_NODES bid at tick 31320 (7830.00)");
        std::printf("  %s\n  %s\n  %s\n", line(st, "WARMING,").c_str(), line(st, "KING_NODES,").c_str(), line(st, "DRAW,").c_str());
        const std::string z = readAll(findFile(tmp, "LRA-LiqZones-ES.csv"));
        bool f7 = true; int zr = 0; { std::istringstream in(z); std::string ln; while (std::getline(in, ln)) if (ln.rfind("Z,", 0) == 0 || ln.rfind("N,", 0) == 0) { ++zr; if (std::count(ln.begin(), ln.end(), ',') != 6) f7 = false; } }
        CHECK(z.rfind("LIQZ,1,", 0) == 0 && f7, "LRA-LiqZones: header + every Z / N row has 7 fields (field 7 = x normal)");
        std::printf("  LiqZones rows %d\n", zr);
        if (const char* out = getenv("LP073_WRITE")) { std::ofstream o(out); o << "# lsLiquidityProfile 0.73.0 B facts after 10+ min (test_lp073_host.cpp): code,side,price,lots,state,proven\n";
            for (const sb::Fact& f : B) if (f.code == 3 || f.code == 4 || f.code == 9) o << f.code << "," << f.side << "," << f.lo << "," << f.val << "," << f.state << "," << f.proven << "\n"; }
    }
    // an IRT RESTART: the order book starts again -> warming again, no king, even though the zone / touch records exist
    p->destroy(); p->slot_.release(p); p->flushPark(); delete p; h.data = nullptr; mock::feed.ev.clear(); mock::feed.at = 0;
    p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int i = 0; i < 5; ++i) mock::feed.push(0, 1, 7852.0f - 1.0f * i, 30, (RTDATE)(t0 + 645), 900 + i);
    run(t0 + 641, t0 + 644); live(t0 + 645, t0 + 700); S = static_cast<LQState*>(h.data);
    {   const std::vector<sb::Fact> B = facts(S); const sb::Fact* w = code(B, 9);
        CHECK(w && w->state == 1 && code(B, 3) == nullptr, "after an IRT restart: warming again, no king posted");
        CHECK(S->lt.seedLots(ltc::BID, 31320, S->lt.firstMs + 60000) == 0, "the depth seed clock restarts with the order book (not seeded 1 min after the restart)"); }
    p->destroy(); p->slot_.release(p); p->flushPark(); delete p;
    std::printf("lsLiquidityProfile %s warm-up / king host: %d passed, %d failed\n", LQ_VERSION, passes, fails);
    return fails ? 1 : 0;
}
