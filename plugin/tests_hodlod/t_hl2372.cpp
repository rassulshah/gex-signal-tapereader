// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2372.cpp -o t2372 && ./t2372   (from plugin\tests_hodlod)
// lsHodLod 2.37.2 (2026-11-19 22:53 CT, Rassul, screenshot of NQ at 10:51 PM still showing "NQ Sw PFL 5:39": "remove signals after an
// hour"): the signal line (Cob | Sw | BkTst) shows a tag only for 60 minutes after its candle (TopSignals.h 1.1.0)
//  [1] age: 0 / 30 / 59 min / 59:59 shows; 60 / 61 min / 5 h gone; the bar clock is used, the PC clock only without a bar time
//  [2] mixed groups: a Cob (30 min) and a BkTst (10 min) stay, the Sw (70 min) goes; its third of the line is empty
//  [3] a reload with old signals (his NQ 5:39 PM Sw at 10:51 PM, a whole session of old tags): none shown
//  [4] idle feed: the bar clock does not move -> the tag stays while the PC clock runs on
//  [5] the box: all three groups empty -> the signal line is still there, the box height does not jump
//  [7] (23:02 CT) the Now line: live = gold "TESTING: <level> Bk / Sw", BREAK / SWEEP only at the close from his marks (full tests in t_hl2360 [4b])
//  [6] host: ages every draw on ageClock(lastClosedAbs, hlNowCt()), status names expired / clock; versions 2.37.2 everywhere
#include <cstdio>
#include <climits>
#include <fstream>
#include <sstream>
#include <set>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../TopSignals.h"
#include "../NowLevels.h"
#include "lib_widths.h"
#include "scen.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; if (fails < 60) std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static int MS(const std::string& s, int fs, bool b) { return measureLib(s, fs, b); }
static std::string rd(const char* p) { std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
static long long L(int y, int mo, int d, int h, int mi) {   // local secs, days from 1970 (civil)
    y -= mo <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + (long long)doe - 719468) * 86400 + h * 3600 + mi * 60; }
static tsig::Sig mk(const char* m, const char* ty, const char* lv, long long t, int d) { tsig::Sig s; s.mkt = m; s.type = ty; s.lv = lv; s.t = t; s.dir = d; return s; }
static std::string wy(long long t, const char* code, int d, const char* lv) { return std::to_string(t + tsig::E1904) + "|" + code + "|" + std::to_string(d) + "|" + lv + "|0|x\n"; }
// what the host does every draw (HodLod.cpp 2.37.2)
static std::vector<std::string> shown(const std::vector<tsig::Sig>& latest, long long barNow, long long pcNow) {
    std::vector<std::string> o; for (const tsig::Sig& g : tsig::freshOnly(latest, tsig::ageClock(barNow, pcNow))) o.push_back(tsig::label(g)); return o; }
static bool has(const std::vector<std::string>& v, const std::string& s) { for (const std::string& x : v) if (x == s) return true; return false; }

int main() { (void)&fnv; (void)&itemsText;
    const long long now = L(2026, 11, 19, 22, 51), sig = L(2026, 11, 19, 21, 0);
    std::printf("[1] age\n");
    for (int sec : { 0, 60, 1800, 3540, 3599 }) CHECK(tsig::fresh(mk("ES", "Sw", "PDL", sig, 1), sig + sec), "shows at " + std::to_string(sec) + " s");
    for (int sec : { 3600, 3601, 3660, 5 * 3600, 86400 }) CHECK(!tsig::fresh(mk("ES", "Sw", "PDL", sig, 1), sig + sec), "gone at " + std::to_string(sec) + " s");
    CHECK(tsig::SHOW_SECS == 3600, "60 minutes");
    CHECK(tsig::fresh(mk("ES", "Sw", "PDL", sig, 1), sig - 180), "a candle stamped after the clock (forming) still shows");
    CHECK(shown({ mk("ES", "Sw", "PDL", sig, 1) }, sig + 59 * 60, sig + 5 * 3600).size() == 1, "59 min on the bar clock shows even if the PC clock says 5 h");
    CHECK(shown({ mk("ES", "Sw", "PDL", sig, 1) }, sig + 60 * 60, sig).empty(), "60 min on the bar clock is gone even if the PC clock says 0");
    CHECK(tsig::ageClock(0, 123) == 123 && tsig::ageClock(-5, 123) == 123 && tsig::ageClock(456, 123) == 456, "PC clock only without a bar time");
    CHECK(shown({ mk("ES", "Sw", "PDL", sig, 1) }, 0, sig + 59 * 60).size() == 1 && shown({ mk("ES", "Sw", "PDL", sig, 1) }, 0, sig + 61 * 60).empty(), "fallback PC clock: 59 shows, 61 gone");

    std::printf("[2] mixed groups\n");
    {   const std::vector<tsig::Sig> lat = tsig::latest({ mk("ES", "Cob", "", now - 30 * 60, -1), mk("NQ", "Sw", "PFL", now - 70 * 60, -1),
                                                          mk("ES", "BkTst", "PDL", now - 10 * 60, -1), mk("GC", "Sw", "PDH", now - 61 * 60, 1) });
        const std::vector<std::string> v = shown(lat, now, now);
        CHECK(v.size() == 2 && has(v, "ES Cob 10:21") && has(v, "ES BkTst PDL 10:41"), "Cob (30 min) and BkTst (10 min) stay");
        CHECK(!has(v, "NQ Sw PFL 9:41") && !has(v, "GC Sw PDH 9:50"), "both Sw (70 / 61 min) gone");
        std::vector<htop::Tag> tg; for (const tsig::Sig& g : tsig::freshOnly(lat, now)) { htop::Tag t; t.text = tsig::label(g); t.dir = g.dir; t.group = htop::groupOf(g.type); tg.push_back(t); }
        auto ms = [](const std::string& s, int fs, bool b) { return MS(s, fs, b); };
        const htop::Sig S = htop::sigLayout(tg, ms, 10, 1210, 100, 121);
        CHECK(S.ok && S.shown[0] == 1 && S.shown[1] == 0 && S.shown[2] == 1 && S.dropped == 0, "Sw third empty, Cob left, BkTst right");
        // a newer Sw in the same market replaces an older: latest per market / type, then aged
        const std::vector<tsig::Sig> l2 = tsig::latest({ mk("NQ", "Sw", "PFL", now - 5 * 3600, -1), mk("NQ", "Sw", "ONH", now - 20 * 60, 1) });
        const std::vector<std::string> v2 = shown(l2, now, now);
        CHECK(v2.size() == 1 && v2[0] == "NQ Sw ONH 10:31", "the newer NQ Sw (20 min) shows, the old one never comes back"); }

    std::printf("[3] reload with old signals\n");
    {   // his screenshot: NQ Sw PFL 5:39 PM still shown at 10:51 PM; plus a whole evening of old tags in every group
        std::string t = wy(L(2026, 11, 19, 17, 39), "Sw", -1, "PFL") + wy(L(2026, 11, 19, 18, 30), "Tst", -1, "PFL") + wy(L(2026, 11, 19, 18, 45), "Rev", -1, "")
                      + wy(L(2026, 11, 19, 21, 50), "Sw", 1, "ONL");   // 61 min before 10:51 PM
        const long long from = tsig::sessionStart(now);
        const std::vector<tsig::Sig> all = tsig::parseWy(t, "NQ", from, now);
        CHECK(all.size() == 3, "parsed 2 Sw + 1 BkTst of this session");
        const std::string cob = "COB," + std::to_string(L(2026, 11, 19, 19, 0)) + ",BEAR,x,SHOW\nZZO," + std::to_string(L(2026, 11, 19, 19, 0)) + ",BEAR,12,OK\nREV,"
                              + std::to_string(L(2026, 11, 19, 19, 12)) + ",BEAR," + std::to_string(L(2026, 11, 19, 19, 0)) + "\n";
        std::vector<tsig::Sig> both = all; const std::vector<tsig::Sig> c = tsig::parseCob(cob, "NQ", from, now); both.insert(both.end(), c.begin(), c.end());
        CHECK(c.size() == 1, "one Cob Rev");
        const std::vector<tsig::Sig> lat = tsig::latest(both);
        CHECK(lat.size() == 3, "latest: Cob, Sw, BkTst");
        CHECK(shown(lat, now, now).empty(), "reload at 10:51 PM: none of the old tags shown (bar clock)");
        CHECK(shown(lat, 0, now).empty(), "reload before the bars are read (PC clock): none shown either");
        const std::vector<tsig::Sig> early = tsig::latest(tsig::parseWy(wy(L(2026, 11, 19, 17, 39), "Sw", -1, "PFL"), "NQ", from, now));
        const std::vector<std::string> e = shown(early, L(2026, 11, 19, 17, 45), now);
        CHECK(e.size() == 1 && e[0] == "NQ Sw PFL 5:39", "the 5:39 PM Sw at 5:45 PM bar time: shown (6 min old)");
        CHECK(shown(early, L(2026, 11, 19, 18, 38), now).size() == 1 && shown(early, L(2026, 11, 19, 18, 39), now).empty(), "5:39 Sw: shown at 6:38 (59 min), gone at 6:39 (60 min)"); }

    std::printf("[4] idle feed\n");
    {   const std::vector<tsig::Sig> lat = { mk("ES", "Sw", "PDL", sig, 1) };
        CHECK(shown(lat, sig + 30 * 60, sig + 3 * 3600).size() == 1, "the bar clock stopped 30 min after the tag: it stays (market time)");
        CHECK(shown(lat, sig + 75 * 60, sig + 75 * 60).empty(), "bars move on 75 min: gone"); }

    std::printf("[5] the box keeps its height\n");
    {   auto yOf = [](long long) { return 300; };
        std::set<int> hs; int n = 0;
        for (int ntag : { 0, 1, 3 }) {
            const hl::Box pane = { 0, 0, 1530, 640 }; hl::View v = mkView(0, true); hl::Opts O;
            const char* TX[3] = { "ES Cob 3:18", "NQ Sw PHL 1:09", "ES BkTst PDL 11:39" };
            for (int i = 0; i < ntag; i++) v.cobs.push_back(hl::View::CobSeg{ "", "", -1, TX[i] });
            hl::Layout Lo = hl::layoutAll(v, pane, yOf, MS, (int)(1530 * 0.8), 2, -1, nullptr, &O);
            if (!Lo.tableOk) continue;
            n++;
            hs.insert(Lo.table.b - Lo.table.t);
            CHECK(Lo.sigRow.b > Lo.sigRow.t, "the signal row is there with " + std::to_string(ntag) + " tags");
            if (ntag == 0) CHECK(Lo.sigShown == 0, "no tags: empty line"); }
        CHECK(n == 3 && hs.size() == 1, "one box height with 0 / 1 / 3 tags"); }

    std::printf("[6] host + versions\n");
    {   const std::string s = rd("../HodLod.cpp");
        CHECK(s.find("S->tsClock = tsig::ageClock(S->lastClosedAbs, hlNowCt());") != std::string::npos, "host ages on the bar clock (PC clock fallback)");
        CHECK(s.find("tsig::freshOnly(S->tsLatest, S->tsClock)") != std::string::npos, "host filters every draw (not only at the 5 s re-read)");
        CHECK(s.find("tsSegs") == std::string::npos, "the unfiltered segment cache is gone");
        CHECK(s.find("expired %d,clock %s") != std::string::npos, "status names expired tags and the clock");
        CHECK(s.find("p->setVersion(\"2.37.2\")") != std::string::npos && s.find(" *  2.37.2 (2026-11-19 22:53 CT") != std::string::npos, "setVersion + header comment 2.37.2"); }
    CHECK(std::string(hl::HL_VERSION) == "2.33.0" && std::string(hlset::PLUGIN_VERSION) == "2.37.2" && std::string(tsig::VERSION) == "1.1.0", "HL_VERSION = PLUGIN_VERSION = 2.37.2; TopSignals 1.1.0");
    {   const std::string b = rd("../../tools/gex-build.bat"); const size_t p = b.find("call :one HodLod ");
        const std::string ln = p == std::string::npos ? std::string() : b.substr(p, b.find('\n', p) - p);
        CHECK(ln.find("TopSignals.h") != std::string::npos, "gex-build HodLod line lists TopSignals.h (edit triggers a rebuild)"); }
    std::printf("[7] Now line live TESTING\n");
    {   using namespace nowl; std::vector<Bar> B; const long long t0 = L(2026, 11, 19, 22, 0);
        const double R[][5] = { { 100, 101, 99, 100, 10 }, { 100, 101, 99, 100.5, 10 }, { 100.5, 101, 100, 100.8, 10 }, { 100.8, 102.2, 100.6, 101.5, 10 } };
        for (int i = 0; i < 4; i++) { Bar b; b.t = t0 + 180LL * (i + 1); b.o = R[i][0]; b.h = R[i][1]; b.l = R[i][2]; b.c = R[i][3]; b.v = R[i][4]; B.push_back(b); }
        const std::vector<Key> K = { { "ONH", 102.0 }, { "PWL", 90 } };   // (PHL comes from the bars, not the key file)
        std::vector<Bar> F = B; F.back().c = 101.9; Out o = compute(F, {}, K, 101.9, 0.25);   // his NQ case: the forming candle through the level, back under it
        CHECK(o.bs.state == 3 && plain(segments(o)).rfind("[TESTING: ONH Bk / Sw]", 0) == 0, "forming through ONH: TESTING: ONH Bk / Sw, not SWEEP: " + plain(segments(o)));
        CHECK(plain(segments(o)).find("SWEEP") == std::string::npos, "no SWEEP while forming");
        CHECK(std::string(nowl::VERSION) == "1.2.0", "NowLevels 1.2.0"); }
    std::printf("t_hl2372 (lsHodLod 2.37.2): %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
