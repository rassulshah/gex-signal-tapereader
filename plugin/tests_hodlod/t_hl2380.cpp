// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2380.cpp -o t2380 && ./t2380   (from plugin\tests_hodlod)
// lsHodLod 2.38.0 (2026-11-20 06:04 CT, Rassul "ok lets do this", mockup now-box-levels-mockup.html gold PROPOSED 06:02 rows): the Now line
// adds ONE short liquidity phrase for the level its badge names, from lsLimitProfile 2.68.0's levelliq-<MKT>-<spb>.txt (NowLevels.h 1.3.0)
//  [1] each state: building / leaving / holding (X, H) / pulled (P), bids and offers, the colour by who it helps
//  [2] only SHOWN (passed) states; steady / unshown = no phrase
//  [3] stale (> 2 min), missing, garbled, other market / bar size = no phrase
//  [4] the WATCH badge (two levels) names the level: "ALO bids leaving"; a merged file row "PFL/PDL" names each
//  [5] narrow pane: the phrase drops first (before Next / Last Bk)
//  [6] two markets: an ES file never shows on NQ
//  [7] host source + versions 2.38.0 everywhere
#include <cstdio>
#include <fstream>
#include <sstream>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../NowLevels.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static std::string rd(const char* p) { std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
using namespace nowl;
static const long long NOW = 1795176000;
static std::string file(const std::string& rows, const char* mkt = "ES", long long spb = 180, long long stamp = NOW - 10) {
    return std::string("LEVELLIQ,1,2.68.0\nMARKET|") + mkt + "\nSPB|" + std::to_string(spb) + "\nTICK|0.25\nSTAMP|" + std::to_string(stamp) + "\n" + rows; }
static std::vector<Bar> bars() { std::vector<Bar> B; const double R[][4] = { { 100, 101, 99, 100 }, { 100, 101, 99, 100.5 }, { 100.5, 101, 100, 100.8 }, { 100.8, 102.2, 100.6, 101.5 } };
    for (int i = 0; i < 4; i++) { Bar b; b.t = NOW - 720 + 180LL * i; b.o = R[i][0]; b.h = R[i][1]; b.l = R[i][2]; b.c = R[i][3]; b.v = 10; B.push_back(b); } return B; }
// the TESTING case: forming candle through ONH
static Out testingOnh() { std::vector<Bar> F = bars(); F.back().c = 101.9; const std::vector<Key> K = { { "ONH", 102.0 }, { "PWL", 90 } }; return compute(F, {}, K, 101.9, 0.25); }
// the WATCH case: price between two levels
static Out watchCase() { std::vector<Bar> F = bars(); F.back().h = 101.2; F.back().c = 101.0; const std::vector<Key> K = { { "AsiaLO", 99.5 }, { "ONH", 103.0 } }; return compute(F, {}, K, 101.0, 0.25); }
static unsigned wordCol(const std::vector<Seg>& v, const std::string& w) { for (const Seg& g : v) if (g.drop == 3 && g.s == w) return g.col; return 0; }
int main() {
    std::printf("[1] each state\n");
    {   const Out o = testingOnh(); CHECK(o.bs.state == 3, "fixture: TESTING ONH");
        struct C { const char* side; const char* st; const char* word; unsigned col; } cs[] = {
            { "B", "building", "building", C_GREEN }, { "B", "leaving", "leaving", C_RED }, { "B", "H", "holding", C_GREEN }, { "B", "X", "holding", C_GREEN }, { "B", "P", "pulled", C_RED },
            { "O", "building", "building", C_RED }, { "O", "leaving", "leaving", C_GREEN }, { "O", "H", "holding", C_RED }, { "O", "P", "pulled", C_GREEN } };
        for (const C& c : cs) { const Liq q = parseLiq(file(std::string("L|ONH|408|") + c.side + "|" + c.st + "|1|" + std::to_string(NOW - 100) + "|0\n"), "ES", 180, NOW);
            const std::vector<Seg> v = segments(o, q); const std::string p = plain(v);
            const std::string want = std::string("[TESTING: ONH Bk / Sw] \xE2\x94\x82 ") + (c.side[0] == 'B' ? "Bids " : "Offers ") + c.word;
            CHECK(p.rfind(want, 0) == 0 && wordCol(v, c.word) == c.col, std::string(c.side) + " " + c.st + ": " + p); } }
    std::printf("[2] only shown states\n");
    {   const Out o = testingOnh();
        CHECK(plain(segments(o, parseLiq(file("L|ONH|408|B|leaving|0|1|0\n"), "ES", 180, NOW))).find("leaving") == std::string::npos, "unshown (recorded only) state: no phrase");
        CHECK(plain(segments(o, parseLiq(file("L|ONH|408|B|steady|1|1|0\n"), "ES", 180, NOW))).find("Bids") == std::string::npos, "steady: no phrase");
        CHECK(plain(segments(o, parseLiq(file("L|PDL|400|B|H|1|1|0\n"), "ES", 180, NOW))).find("Bids") == std::string::npos, "another level's state: no phrase");
        CHECK(plain(segments(o)) == plain(segments(o, Liq())), "no file = the 2.37.2 line unchanged"); }
    std::printf("[3] stale / missing / garbled / other\n");
    {   const Out o = testingOnh(); const std::string r = "L|ONH|408|B|H|1|1|0\n";
        Liq q = parseLiq(file(r, "ES", 180, NOW - 121), "ES", 180, NOW); CHECK(!q.ok && q.why == "stale" && plain(segments(o, q)).find("holding") == std::string::npos, "121 s old: stale, no phrase");
        q = parseLiq(file(r, "ES", 180, NOW - 119), "ES", 180, NOW); CHECK(q.ok, "119 s old: fresh");
        q = parseLiq("", "ES", 180, NOW); CHECK(!q.ok && q.why == "no file", "missing: no phrase");
        q = parseLiq("garbage\nL|ONH|1|B|H|1|1|0\n", "ES", 180, NOW); CHECK(!q.ok && q.why == "bad header", "garbled header: no phrase");
        q = parseLiq(file(r, "ES", 60), "ES", 180, NOW); CHECK(!q.ok && q.why == "other bar size", "other bar size: no phrase");
        q = parseLiq(file("L|ONH|408|X|H|1|1|0\nL|ONH|408|B|H|7|1|0\nL|ONH\n"), "ES", 180, NOW); CHECK(q.ok && q.rows.empty(), "bad rows skipped");
        q = parseLiq(file(r) + std::string(70000, 'x'), "ES", 180, NOW); CHECK(!q.ok && q.why == "too big", "oversized: no phrase");
        q = parseLiq(file(r, "ES", 180, NOW + 600), "ES", 180, NOW); CHECK(!q.ok, "a stamp 10 min in the future: no phrase"); }
    std::printf("[4] WATCH names the level; merged rows\n");
    {   const Out o = watchCase(); const std::string base = plain(segments(o));
        CHECK(base.rfind("[WATCH: ALO", 0) == 0, "fixture: WATCH ALO ... : " + base);
        const std::string p = plain(segments(o, parseLiq(file("L|AsiaLO|398|B|leaving|1|5|0\n"), "ES", 180, NOW)));
        CHECK(p.find("\xE2\x94\x82 ALO bids leaving") != std::string::npos, "WATCH: \"ALO bids leaving\": " + p);
        const std::string p2 = plain(segments(testingOnh(), parseLiq(file("L|PFL\xB7ONH|408|O|P|1|5|0\n"), "ES", 180, NOW)));
        CHECK(p2.find("Offers pulled") != std::string::npos, "a merged row (PFL\xB7ONH) names ONH: " + p2);
        const std::string p3 = plain(segments(testingOnh(), parseLiq(file("L|ONH|408|B|H|1|5|0\nL|ONH|408|O|P|1|9|0\n"), "ES", 180, NOW)));
        CHECK(p3.find("Offers pulled") != std::string::npos && p3.find("Bids") == std::string::npos, "two rows: the newest shown one: " + p3); }
    std::printf("[5] narrow pane\n");
    {   const Out o = testingOnh(); const std::vector<Seg> all = segments(o, parseLiq(file("L|ONH|408|B|H|1|5|0\n"), "ES", 180, NOW));
        auto meas = [](const std::string& s, bool) { return (int)s.size() * 7; };
        const Draw wide = layout(all, meas, 500, 100, 0, 1000, 10); CHECK(wide.ok && wide.level == 0 && wide.shown == (int)all.size(), "wide: everything");
        int wAll = 0; { Draw d = layout(fit(all, 3), meas, 500, 100, 0, 100000, 10); wAll = d.r - d.l; }
        const Draw mid = layout(all, meas, 500, 100, 0, wAll + 2, 10);
        bool phrase = false, next = false; for (const Item& it : mid.items) { if (it.s == "holding") phrase = true; if (it.s == "Next") next = true; }
        CHECK(mid.ok && mid.level == 3 && !phrase && next, "narrower: the phrase drops first, Next stays"); }
    std::printf("[6] two markets\n");
    {   const std::string es = file("L|ONH|408|B|H|1|5|0\n", "ES");
        CHECK(!parseLiq(es, "NQ", 180, NOW).ok && parseLiq(es, "NQ", 180, NOW).why == "other market", "the ES file never shows on NQ");
        CHECK(parseLiq(file("L|ONH|408|B|H|1|5|0\n", "NQ"), "NQ", 180, NOW).ok, "NQ's own file shows on NQ"); }
    std::printf("[7] host + versions\n");
    {   const std::string s = rd("../HodLod.cpp");
        CHECK(s.find("p->setVersion(\"2.38.0\")") != std::string::npos && s.find(" *  2.38.0 (2026-11-20 06:04 CT") != std::string::npos, "setVersion + header comment 2.38.0");
        CHECK(s.find("\"levelliq-\" + S->mkt + \"-\" + std::to_string(S->per) + \".txt\"") != std::string::npos && s.find("nowl::segments(S->nlOut, lq)") != std::string::npos && s.find("nowE - S->nlLiqChecked >= 3") != std::string::npos, "host reads levelliq-<MKT>-<per>.txt at most every 3 s and passes it to segments");
        CHECK(s.find(",liq \" + S->nlLiqWhy") != std::string::npos, "status names the liq file state");
        CHECK(std::string(hlset::PLUGIN_VERSION) == "2.38.0" && std::string(nowl::VERSION) == "1.3.0", "PLUGIN_VERSION 2.38.0, NowLevels 1.3.0");
        const std::string g = rd("../HodLodLogic.h"); CHECK(g.find("static const char* const HL_VERSION = \"2.38.0\"") != std::string::npos, "HodLodLogic HL_VERSION 2.38.0"); }
    std::printf("t_hl2380 (lsHodLod 2.38.0): %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
