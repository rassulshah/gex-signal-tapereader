// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2350.cpp -o t2350 && ./t2350   (from plugin\tests_hodlod)
// lsHodLod 2.35.0 (2026-11-19 17:27-18:10 CT, Rassul "lets include sweeps too", "build"): the right end of the top line = each market's
// latest Cob (Rev), Sw and BkTst, newest first, green / red badges with black text (TopSignals.h 1.0.0)
//  [1] parse: Sw lines, a Tst then its Rev = BkTst (the Tst's level), a Rev without a Tst = nothing, other sessions / garbage / bad sides
//      ignored; Cob Revs only for a SHOW Cob with ZZO 10+ of this session; latest per market and type, newest first; labels; 6E
//  [2] his real 11-19 CobSignal records (ES, NQ): the ES Cob Rev = 3:18 PM bearish; NQ none (its Cobs had ZZO under 10)
//  [3] layout: badges right-aligned on the top line, newest first, green / red fill with black text, inside the box, no overlaps with
//      each other or the market moves; a narrow pane drops the OLDEST badges first
//  [4] versions 2.35.0 everywhere
#include <cstdio>
#include <fstream>
#include <sstream>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../TopSignals.h"
#include "lib_widths.h"
#include "scen.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; if (fails < 40) std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static double SC = 1.0;
static int MS(const std::string& s, int fs, bool b) { return (int)(measureLib(s, fs, b) * SC + 0.5); }
static std::string rd(const char* p) { std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
static long long L(int y, int mo, int d, int h, int mi) {   // local secs, days from 1970 (civil)
    y -= mo <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + (long long)doe - 719468) * 86400 + h * 3600 + mi * 60; }
int main() { (void)&fnv; (void)&itemsText;
    std::printf("[1] parse\n");
    const long long now = L(2026, 11, 19, 14, 30), from = tsig::sessionStart(now);
    CHECK(from == L(2026, 11, 18, 17, 0), "session starts 5 PM the day before");
    CHECK(tsig::sessionStart(L(2026, 11, 19, 17, 5)) == L(2026, 11, 19, 17, 0), "after 5 PM: today's 5 PM");
    auto st = [](long long t) { return std::to_string(t + tsig::E1904); };
    std::string wy = "WY|1|ES|180|0.20.0\n";
    wy += st(L(2026, 11, 18, 15, 0)) + "|Sw|1|PDL|7305.25|V99+\n";            // last session: ignored
    wy += st(L(2026, 11, 19, 9, 42)) + "|Sw|1|PDL|29221|V99+\n";
    wy += st(L(2026, 11, 19, 8, 42)) + "|Sw|1|PHL|29634|V99+\n";              // out of order: still the 9:42 one is latest
    wy += st(L(2026, 11, 19, 11, 33)) + "|Tst|-1|PDL|0|62%\n";
    wy += st(L(2026, 11, 19, 11, 39)) + "|Rev|-1||0|\n";                      // BkTst PDL 11:39
    wy += st(L(2026, 11, 19, 12, 0)) + "|Rev|1||0|V35\n";                     // no bullish Tst before it: not a BkTst
    wy += st(L(2026, 11, 19, 12, 3)) + "|Rev|-1||0|\n";                       // its Tst was used: nothing
    wy += st(L(2026, 11, 19, 14, 45)) + "|Sw|-1|PHH|1|V20\n";                 // after now: ignored
    wy += "garbage line\n" + st(L(2026, 11, 19, 10, 0)) + "|Sw|2|PDH|1|\n" + st(L(2026, 11, 19, 10, 0)) + "|Sw|1|P<D>H|1|\n";
    wy += st(L(2026, 11, 19, 10, 0)) + "|Bk|-1|PDL|29221|V99+\n";             // plain Bk: not shown
    std::vector<tsig::Sig> w = tsig::parseWy(wy, "ES", from, now);
    int sw = 0, bt = 0; for (const auto& s : w) { if (s.type == "Sw") sw++; if (s.type == "BkTst") bt++; }
    CHECK(sw == 2 && bt == 1 && w.size() == 3, "2 Sw + 1 BkTst (got " + std::to_string(w.size()) + ")");
    std::vector<tsig::Sig> lw = tsig::latest(w);
    CHECK(lw.size() == 2 && lw[0].type == "BkTst" && tsig::label(lw[0]) == "ES BkTst PDL 11:39" && lw[0].dir == -1, "newest first: ES BkTst PDL 11:39 (red)");
    CHECK(lw.size() == 2 && tsig::label(lw[1]) == "ES Sw PDL 9:42" && lw[1].dir == 1, "then ES Sw PDL 9:42 (green)");
    { tsig::Sig s; s.mkt = "EU"; s.type = "Cob"; s.t = L(2026, 11, 19, 0, 5); s.dir = 1; CHECK(tsig::label(s) == "6E Cob 12:05", "EU shows as 6E, 12-hour clock"); }
    CHECK(tsig::parseWy(std::string(tsig::MAX_BYTES + 10, 'x'), "ES", from, now).empty(), "oversized text refused");
    std::string cob = "# header\nCOB," + std::to_string(L(2026, 11, 19, 10, 12)) + ",BULL,Cob,PDL,2.78,7293,168,1,1,0,0,SHOW\nZZO," + std::to_string(L(2026, 11, 19, 10, 12)) + ",BULL,16,OK,1\n";
    cob += "COB," + std::to_string(L(2026, 11, 19, 9, 0)) + ",BEAR,Cob,PDH,2.0,7400,168,1,1,0,0,SHOW\nZZO," + std::to_string(L(2026, 11, 19, 9, 0)) + ",BEAR,8,NO,1\n";   // ZZO 8: not drawn
    cob += "COB," + std::to_string(L(2026, 11, 19, 9, 30)) + ",BEAR,Cob,PDH,2.0,7400,168,1,1,0,0,LOG\nZZO," + std::to_string(L(2026, 11, 19, 9, 30)) + ",BEAR,20,OK,1\n";   // LOG: not drawn
    cob += "REV," + std::to_string(L(2026, 11, 19, 10, 21)) + ",BULL," + std::to_string(L(2026, 11, 19, 10, 12)) + ",0.27,0.53,1\n";
    cob += "REV," + std::to_string(L(2026, 11, 19, 9, 6)) + ",BEAR," + std::to_string(L(2026, 11, 19, 9, 0)) + ",0.1,0.1,1\n";
    cob += "REV," + std::to_string(L(2026, 11, 19, 9, 36)) + ",BEAR," + std::to_string(L(2026, 11, 19, 9, 30)) + ",0.1,0.1,1\n";
    cob += "REV," + std::to_string(L(2026, 11, 19, 10, 30)) + ",BEAR," + std::to_string(L(2026, 11, 19, 10, 12)) + ",0.1,0.1,1\n";   // side not the Cob's
    std::vector<tsig::Sig> c = tsig::parseCob(cob, "ES", from, now);
    CHECK(c.size() == 1 && tsig::label(c[0]) == "ES Cob 10:21" && c[0].dir == 1, "only the Rev of the drawn Cob (SHOW, ZZO 10+, same side)");
    std::printf("[2] his 11-19 Cob records\n");
    {   const std::string es = rd("cob_ES_1119.csv"), nq = rd("cob_NQ_1119.csv");
        CHECK(!es.empty() && !nq.empty(), "real files read");
        const long long n2 = L(2026, 11, 19, 16, 0), f2 = tsig::sessionStart(n2);
        std::vector<tsig::Sig> e = tsig::latest(tsig::parseCob(es, "ES", f2, n2)), q = tsig::parseCob(nq, "NQ", f2, n2);
        CHECK(e.size() == 1 && tsig::label(e[0]) == "ES Cob 3:18" && e[0].dir == -1, "ES latest Cob Rev = 3:18 PM bearish (" + (e.empty() ? std::string("none") : tsig::label(e[0])) + ")");
        CHECK(q.empty(), "NQ: no drawn Cob today (ZZO under 10)"); }
    std::printf("[3] layout\n");
    auto yOf = [](long long) { return 300; };
    int scen = 0, dropOk = 0;
    for (int W : { 930, 1050, 1200, 1530, 1900 }) for (double sc : { 1.0, 1.25, 1.4 }) for (int pos : { 6, 7, 1 }) {
        SC = sc; const hl::Box pane = { 0, 0, W, 640 }; hl::View v = mkView(0, true); hl::Opts O;
        v.mkts.push_back(hl::View::MkSeg{ "ES -95%", hl::C_RED, true, -1 }); v.mkts.push_back(hl::View::MkSeg{ "NQ -70%", hl::C_RED, false, 0 }); v.mkts.push_back(hl::View::MkSeg{ "CL +25%", hl::C_GREEN, false, 0 });
        const char* tx[6] = { "ES Cob 3:18", "NQ BkTst PHL 2:21", "NQ Sw PHL 1:09", "ES BkTst PDL 11:39", "ES Sw PDL 9:42", "GC Sw PDL 9:30" }; const int dr[6] = { -1, -1, 1, -1, 1, 1 };
        for (int i = 0; i < 6; i++) v.cobs.push_back(hl::View::CobSeg{ "", "", dr[i], tx[i] });
        hl::Layout Lo = hl::layoutAll(v, pane, yOf, MS, (int)(W * 0.75), pos, -1, nullptr, &O);
        if (!Lo.tableOk) continue;
        scen++;
        std::vector<hl::Item> b, t;   // badges (fill + text), other top-line texts
        int topY = 1 << 30; for (const hl::Item& it : Lo.items) if (it.k == hl::K_TEXT && it.role == hl::R_TABLE) for (int i = 0; i < 6; i++) if (it.s == tx[i]) topY = std::min(topY, it.t);
        std::vector<int> order;
        for (const hl::Item& it : Lo.items) { if (it.role != hl::R_TABLE) continue;
            if (it.k == hl::K_TEXT) { int k = -1; for (int i = 0; i < 6; i++) if (it.s == tx[i]) k = i; if (k >= 0) { b.push_back(it); order.push_back(k);
                    CHECK(it.c1 == 0x00000000u && it.bold, "badge text black, bold");
                    bool filled = false; for (const hl::Item& f : Lo.items) if (f.k == hl::K_FILL && f.l <= it.l && f.r >= it.r && f.c1 == (dr[k] > 0 ? hl::C_GREEN : hl::C_RED)) filled = true;
                    CHECK(filled, std::string("badge fill in its side's colour: ") + tx[k]); }
                else if (it.t <= topY + 2 && it.t >= topY - 2) t.push_back(it); } }
        CHECK(!b.empty(), "at least one badge shown W" + std::to_string(W));
        bool newestFirst = true; for (size_t i = 1; i < order.size(); i++) if (order[i] != order[i - 1] + 1 || order[0] != 0) newestFirst = false;
        CHECK(newestFirst, "badges in order newest first, the oldest dropped (W" + std::to_string(W) + ")");
        if (b.size() < 6) dropOk++;
        for (size_t i = 0; i < b.size(); i++) { CHECK(b[i].l >= Lo.table.l && b[i].r <= Lo.table.r, "badge inside the box");
            for (size_t j = 0; j < b.size(); j++) if (i != j) CHECK(!(b[i].l < b[j].r && b[j].l < b[i].r), "badges do not overlap");
            for (const hl::Item& o : t) CHECK(!(b[i].l < o.r && o.l < b[i].r), "badge clear of the market moves (" + o.s + ")");
            if (i) CHECK(b[i].l > b[i - 1].r, "left to right"); }
    }
    CHECK(scen >= 30, "scenarios laid out: " + std::to_string(scen));
    CHECK(dropOk > 0, "narrow panes drop the oldest badges (" + std::to_string(dropOk) + ")");
    std::printf("[4] versions\n");
    CHECK(std::string(hl::HL_VERSION) == "2.35.0" && std::string(hlset::PLUGIN_VERSION) == "2.35.0", "2.35.0");
    { const std::string s = rd("../HodLod.cpp"); CHECK(s.find("p->setVersion(\"2.35.0\")") != std::string::npos, "setVersion 2.35.0"); }
    std::printf("t_hl2350 (lsHodLod 2.35.0): %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
