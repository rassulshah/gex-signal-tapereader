// build: g++ -std=c++17 -Wall -Wextra -I.. t_hl230.cpp -o t230 && ./t230   (from plugin\tests_hodlod)
// HL 2.3.0 (2026-10-17, Rassul 19:53-20:20 CT) tests, SDK-free (HodLodLogic.h + HodLodGex.h + HodLodQ.h):
//  [1] Q file parse: good / strict (market, version, scores 0-5, '-', torn, oversized, CRLF)
//  [2] Q header parts "Q: " M O V and colours (4-5 green, 0-1 red, 2-3 white, V always white); the hover = the mockup's lines
//  [3] BAND row parse; band-mode Gex row: A above the open red with an up triangle first, ascending; price; B below green, a down
//      triangle first, descending; at most 3 a side; a bad BAND row = no band (the row still drawn); old files keep the price split
//  [4] layout, 9 positions x 3 panes: Q at the top left, Reg / trend right-aligned top right, expiry between, nothing overlaps,
//      the table inside the pane; no Q file = no Q box; the Gex row draws 2 triangles
//  [5] the Q hover: inside the pane, coloured codes, wraps on a narrow pane, none when no Q
#include <cstdio>
#include <cmath>
#include "HodLodLogic.h"
#include "HodLodGex.h"
#include "HodLodQ.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static int measure(const std::string& s, int fs, bool bold) { return (int)(s.size() * fs * (bold ? 0.62 : 0.56) + 0.999); }
static bool inside(const hl::Box& a, const hl::Box& o) { return a.l >= o.l && a.r <= o.r && a.t >= o.t && a.b <= o.b; }
static std::string qFile(const std::string& mkt, const std::string& q, const std::string& extra = "") {
    return "VERSION|1\nMARKET|" + mkt + "\nDATE|2026-10-16\nQ|" + q + "\nPREV|2026-10-15|2|1|3|-\nRECV|1792230000\n" + extra; }
static std::string gexBand(const std::string& band, const std::vector<std::string>& rows) {
    std::string s = "VERSION|2\nMARKET|ES\nSESSION|2026-10-16\nASOF|1792160000|10:00 AM|09:58\nSTATE|LIVE|\nVWAP|7720|down\nBASIS|VERIFIED|0|0|5|-|same\n";
    if (!band.empty()) s += band + "\n";
    for (const std::string& r : rows) s += r + "\n";
    return s;
}
static std::vector<std::string> bandRows() {   // file order: above nearest the open first, then below nearest first (as select_band writes)
    return { "LOC|1|A|7765|7765|F|0|1", "STR|1|7765|7765|9|30|F|-|-|-|1", "LOC|2|A|7780|7780|F|0|1", "STR|2|7780|7780|8|28|F|-|-|C1|2",
             "LOC|3|A|7800|7800|W|1|1", "STR|3|7800|7800|12|40|W|-|-|CR|1", "LOC|4|B|7700|7700|F|0|1", "STR|4|7700|7700|10|33|F|-|-|PS|1",
             "LOC|5|B|7675|7675|F|0|1", "STR|5|7675|7675|7|22|F|-|-|-|3", "LOC|6|B|7650|7650|M|0|1", "STR|6|7650|7650|6|20|M|-|-|-|4" };
}
static const char* BAND = "BAND|7703.00|RTH|7800.00|7650.00|7773.00|7664.00|1.38|8:30|EOD";

int main() {
    std::printf("[1] Q parse\n");
    {   hlq::File F = hlq::parse(qFile("ES", "1|1|4|-"), "ES");
        CHECK(F.ok && F.m == 1 && F.o == 1 && F.v == 4 && F.s == -1 && F.date == "2026-10-16" && F.prevDate == "2026-10-15" && F.pm == 2 && F.recv == 1792230000, "a good file");
        CHECK(!hlq::parse(qFile("NQ", "1|1|4|-"), "ES").ok, "another market: refused");
        CHECK(!hlq::parse(qFile("ES", "1|6|4|-"), "ES").ok, "a score above 5: refused");
        CHECK(!hlq::parse(qFile("ES", "1|x|4|-"), "ES").ok, "a garbled score: refused");
        CHECK(!hlq::parse(qFile("ES", "-|-|-|-"), "ES").ok, "no scores at all: refused");
        CHECK(hlq::parse(qFile("ES", "2|-|3|-"), "ES").ok, "one score missing ('-'): shown");
        std::string v2 = qFile("ES", "1|1|4|-"); v2.replace(0, 9, "VERSION|2"); CHECK(!hlq::parse(v2, "ES").ok, "unknown version: refused");
        CHECK(!hlq::parse(qFile("ES", "1|1|4|-").substr(0, 25), "ES").ok, "torn: refused");
        CHECK(!hlq::parse(std::string(9000, 'x'), "ES").ok, "over 8 KiB: refused");
        std::string c = qFile("ES", "1|1|4|-"), w; for (char ch : c) { if (ch == '\n') w += '\r'; w += ch; }
        CHECK(hlq::parse(w, "ES").ok, "CRLF");
        CHECK(hlq::parse(qFile("ES", "1|1|4|-", "PREV|bad\n"), "ES").ok, "a bad PREV row is ignored");
    }
    std::printf("[2] Q header + hover\n");
    {   hl::View v; hlq::toView(v, hlq::parse(qFile("ES", "1|5|4|-"), "ES"), "S. Dntrend", -2);
        CHECK(v.qSegs.size() == 4 && v.qSegs[0].s == "Q: " && v.qSegs[1].s == "M1" && v.qSegs[1].col == hl::C_RED && v.qSegs[2].s == "  O5" && v.qSegs[2].col == hl::C_GREEN
              && v.qSegs[3].s == "  V4" && v.qSegs[3].col == hlq::C_WHITE, "Q: M1 (red)  O5 (green)  V4 (white)");
        hl::View w; hlq::toView(w, hlq::parse(qFile("ES", "3|2|1|-"), "ES"), "", 0);
        CHECK(w.qSegs[1].col == hlq::C_WHITE && w.qSegs[2].col == hlq::C_WHITE && w.qSegs[3].col == hlq::C_WHITE, "2-3 white; V1 white (no direction)");
        hl::View m; hlq::toView(m, hlq::parse(qFile("ES", "1|1|4|-"), "ES"), "S. Dntrend", -2);
        auto txt = [](const hl::Line& L) { std::string s; for (const hl::Seg& g : L) s += g.s; return s; };
        CHECK(m.qHover.size() == 6, "6 hover lines (title, M, O, V, how to use, example)");
        CHECK(txt(m.qHover[0]) == "MenthorQ Q-Scores (end of day, 0 - 5, as of Oct 16 close)", "title: " + txt(m.qHover[0]));
        CHECK(txt(m.qHover[1]) == "M1 Momentum 1 - trend under pressure (0 - 1 bearish, 2 - 3 neutral, 4 - 5 bullish).", "M line: " + txt(m.qHover[1]));
        CHECK(txt(m.qHover[2]) == "O1 Option 1 - options flow leaning bearish: put demand / downside hedging (0 bearish, 3 neutral, 5 bullish).", "O line: " + txt(m.qHover[2]));
        CHECK(txt(m.qHover[3]) == "V4 Volatility 4 - implied vol rich (4+ overpriced, 2 or less cheap): bigger swings likely.", "V line: " + txt(m.qHover[3]));
        CHECK(txt(m.qHover[4]) == "How to use: M and O agreeing with the trend = trade with it; against it = take turns at levels more seriously.", "how-to line");
        CHECK(txt(m.qHover[5]).find("Example: S. Dntrend with M1 O1 = sellers in control - favour shorts on pullbacks into Gex S/R above") == 0, "example: " + txt(m.qHover[5]));
        CHECK(m.qHover[1][0].col == hl::C_RED && m.qHover[1][1].col == hlq::C_WHITE, "the code coloured, the meaning white");
        hl::View b; hlq::toView(b, hlq::parse(qFile("ES", "5|4|2|-"), "ES"), "Uptrend", 1);
        CHECK(txt(b.qHover.back()).find("Example: Uptrend with M5 O4 = buyers in control - favour longs") == 0, "live example when trend and scores agree: " + txt(b.qHover.back()));
        hl::View n; hlq::toView(n, hlq::File(), "", 0); CHECK(n.qSegs.empty() && n.qHover.empty(), "no file: nothing");
        bool cp = true; for (const hl::Line& L : m.qHover) for (const hl::Seg& g : L) for (unsigned char ch : g.s) if (ch >= 0x80) cp = false;
        CHECK(cp, "plain ASCII (chart code page safe)");
        CHECK(hlq::statusLine(hlq::parse(qFile("ES", "1|1|4|-"), "ES"), "") == "QSCORE,2026-10-16,M1,O1,V4,S-\n", "status line");
    }
    std::printf("[3] BAND + the Gex row\n");
    {   hlg::File F = hlg::parse(gexBand(BAND, bandRows()), "ES");
        CHECK(F.ok && F.band && F.bandKind == "RTH" && std::fabs(F.bandOpen - 7703) < 1e-9 && F.bandTopTxt == "7800.00" && F.bandAt == "8:30" && F.locs.size() == 6, "BAND row parsed");
        hl::View v; hlg::toView(v, F, 7712.25, 1792160100, 5, 600);
        CHECK(v.gexItems.size() == 7, "3 + price + 3");
        CHECK(v.gexItems[0].strike == "7765" && v.gexItems[1].strike == "7780" && v.gexItems[2].strike == "7800" && v.gexItems[3].mid && v.gexItems[3].strike == "7712.25"
              && v.gexItems[4].strike == "7700" && v.gexItems[5].strike == "7675" && v.gexItems[6].strike == "7650", "order: 7765 7780 7800 | price | 7700 7675 7650");
        CHECK(v.gexItems[0].above && v.gexItems[2].above && !v.gexItems[4].above && v.gexItems[0].mark == 'U' && v.gexItems[1].mark == 0 && v.gexItems[4].mark == 'D' && v.gexItems[5].mark == 0, "red above / green below, one triangle a side");
        hl::View p; hlg::toView(p, F, 7790.0, 1792160100, 5, 600);
        CHECK(p.gexItems[0].above && p.gexItems[0].strike == "7765" && p.gexItems[3].mid, "price above some red strikes: the side is the open's, not the price's");
        std::string hv; for (const hl::Seg& g : v.gexHover) hv += g.s + "\n";
        CHECK(hv.find("Band from the RTH open 7703.00 (8:30): up to 7800.00 (1.38x to 1D Max 7773.00), down to 7650.00 (1.38x to 1D Min 7664.00)") != std::string::npos, "hover band line");
        CHECK(hlg::statusLine(F, v, "").find(",band RTH open 7703.00 up 7800.00 down 7650.00") != std::string::npos, "status band");
        hlg::File nb = hlg::parse(gexBand("BAND|x|RTH|1|2|3|4|1.38", bandRows()), "ES");
        hl::View vb; hlg::toView(vb, nb, 7712.25, 1792160100, 5, 600);
        CHECK(nb.ok && !nb.band && vb.gexItems.size() == 7, "a bad BAND row: no band, the row still drawn");
        std::vector<std::string> many = bandRows(); many.push_back("LOC|7|A|7900|7900|F|0|1"); many.push_back("STR|7|7900|7900|1|1|F|-|-|-|9");
        hl::View vm; hlg::toView(vm, hlg::parse(gexBand(BAND, many), "ES"), 7712.25, 1792160100, 5, 600);
        CHECK(vm.gexItems.size() == 7 && vm.gexItems[2].strike == "7800", "at most 3 a side");
        hl::View old; hlg::toView(old, hlg::parse(gexBand("", { "LOC|1|T|7780|7780|F|0|1", "STR|1|7780|7780|9|30|F|-|-|-|1", "LOC|2|O|7650|7650|F|0|1", "STR|2|7650|7650|9|30|F|-|-|-|1" }), "ES"), 7712.25, 1792160100, 5, 600);
        CHECK(old.gexItems.size() == 3 && old.gexItems[0].mark == 0 && old.gexItems[0].above, "an old file (T / O): the 2.2.4 price split, no triangles");
    }
    std::printf("[4] layout\n");
    {   hl::View v; v.has = true; v.headLeftSegs.push_back(hl::Seg{ "ES RTH 8:30-3:00", hl::C_SLATE });
        v.sessSegs.push_back(hl::Seg{ "Regime: Negative", hl::C_RED }); v.trendReady = true; v.trendLvl = -2;
        v.E[hl::K_1ST] = hl::mkCell("8:33 - 9:21", hl::C_BLUE); v.A[hl::K_LABEL] = hl::mkCell("Actual", hl::C_SLATE); v.A[hl::K_1ST] = hl::mkCell("HOD IN 8:33", hl::C_SLATE);
        hlg::toView(v, hlg::parse(gexBand(BAND, bandRows()), "ES"), 7703.0, 1792160100, 5, 600);
        hlq::toView(v, hlq::parse(qFile("ES", "1|1|4|-"), "ES"), "S. Dntrend", -2);
        hl::View vx = v; vx.sessSegs.push_back(hl::Seg{ "Expiry 3:00 PM (in 5h 46m)", hl::C_GREY });
        hl::View v0 = v; v0.qSegs.clear(); v0.qHover.clear();
        auto yOf = [](long long) { return 300; };
        int bad = 0, overl = 0, regR = 0, qL = 0, tri = 0, cnt = 0;
        for (const hl::View* pv : { &v, &vx })
        for (int W : { 1400, 1150, 900 }) for (int pos = 0; pos < 9; pos++) {
            hl::Box pane = { 0, 0, W, 620 }; const int pe = W - 200;
            hl::Layout L = hl::layoutAll(*pv, pane, yOf, measure, pe, pos, -1, nullptr, nullptr);
            if (!L.tableOk) continue; cnt++;
            if (!L.qOk || !inside(L.qBox, L.table) || !inside(L.table, pane) || L.table.r > pe) ++bad;
            std::vector<hl::Item> tx; for (const hl::Item& it : L.items) if (it.k == hl::K_TEXT && !it.s.empty()) tx.push_back(it);
            for (size_t i = 0; i < tx.size(); i++) for (size_t j = i + 1; j < tx.size(); j++) { const hl::Item& a = tx[i]; const hl::Item& b = tx[j];
                const int ah = (a.t + a.b) / 2, bh = (b.t + b.b) / 2; if (std::abs(ah - bh) < 6 && a.l < b.r - 1 && b.l < a.r - 1) { ++overl; if (overl < 4) std::printf("  overlap '%s' [%d,%d] / '%s' [%d,%d]\n", a.s.c_str(), a.l, a.r, b.s.c_str(), b.l, b.r); } }
            // (2.4.4 compact) "ES" top left, " \xB7 S.Dn" ends right-aligned top right; the strip: red "R  " (resistance) and green "S  " (support)
            // (2.5.0 card) "ES" at the 10 px padding, the trend in full ("Strong Downtrend") right-aligned; the ladder tags S green / R red
            for (const hl::Item& it : tx) { if (it.s == "Strong Downtrend" && it.r == L.table.r - 10) ++regR; if (it.s == "ES" && it.l == L.table.l + 10) ++qL; }
            int rs = 0; for (const hl::Item& it : tx) if (it.t >= L.gexRow.t && it.b <= L.gexRow.b && ((it.s == "R" && it.c1 == hl::C_RED) || (it.s == "S" && it.c1 == hl::C_GREEN))) ++rs;
            if (rs == 2) ++tri;
        }
        CHECK(cnt > 30, "the table drawn (" + std::to_string(cnt) + " layouts)");
        CHECK(bad == 0, "Q box inside the table, the table inside the pane, left of the last bar");
        CHECK(overl == 0, "no two texts overlap (" + std::to_string(overl) + ")");
        CHECK(regR == cnt && qL == cnt, "Q: top left (" + std::to_string(qL) + "), Reg / trend right-aligned top right (" + std::to_string(regR) + ") of " + std::to_string(cnt));
        CHECK(tri == cnt, "(2.5.0) the ladder: green S (support) + red R (resistance)");
        hl::Layout L0 = hl::layoutAll(v0, hl::Box{ 0, 0, 1400, 620 }, yOf, measure, 1200, 7, -1, nullptr, nullptr);
        { int mov = 0; for (const hl::Item& it : L0.items) if (it.k == hl::K_TEXT && (it.s == " M1" || it.s == " O1")) ++mov; CHECK(L0.tableOk && mov == 0, "(2.4.4) no Q file: no M / O / V in the header (the table still drawn)"); }
    }
    std::printf("[5] the Q hover\n");
    {   hl::View v; v.has = true; v.headLeftSegs.push_back(hl::Seg{ "ES RTH 8:30-3:00", hl::C_SLATE });
        v.sessSegs.push_back(hl::Seg{ "Regime: Negative", hl::C_RED }); v.trendReady = true; v.trendLvl = -2;
        hlq::toView(v, hlq::parse(qFile("ES", "1|1|4|-"), "ES"), "S. Dntrend", -2);
        auto yOf = [](long long) { return 300; }; int bad = 0, shown = 0, wrapped = 0, red = 0;
        for (int W : { 1400, 1000, 700 }) for (int H : { 620, 420 }) for (int pos = 0; pos < 9; pos++) {
            hl::Box pane = { 0, 0, W, H }; const int pe = W - 150;
            hl::Layout L = hl::layoutAll(v, pane, yOf, measure, pe, pos, -1, nullptr, nullptr);
            if (!L.qOk) continue;
            const size_t n0 = L.items.size(); hl::layoutQHover(L, v, pane, pe, measure);
            if (!L.qHoverOk) continue; ++shown;
            if (!inside(L.qHover, pane) || L.qHover.r > pe) ++bad;
            int rows = 0; for (size_t i = n0; i < L.items.size(); i++) { if (L.items[i].k == hl::K_TEXT && L.items[i].c1 == hl::C_RED && (L.items[i].s == "M1" || L.items[i].s == "O1")) ++red; if (L.items[i].k == hl::K_TEXT) ++rows; }
            if (W == 700) wrapped += rows > 9; if (W == 1400 && rows > 24) ++bad;   // (2.4.4) + the header lines (RTH, expiry, regime, Exp / Act / Est)
        }
        CHECK(shown > 30, "the hover shown (" + std::to_string(shown) + ")");
        CHECK(bad == 0, "inside the pane, left of the last bar");
        CHECK(red == 2 * shown, "M1 / O1 red in every hover");
        CHECK(wrapped > 0, "a narrow pane wraps");
        hl::View e = v; e.qSegs.clear(); e.qHover.clear(); hl::Layout L = hl::layoutAll(e, hl::Box{ 0, 0, 1400, 620 }, yOf, measure, 1200, 7, -1, nullptr, nullptr);
        hl::layoutQHover(L, e, hl::Box{ 0, 0, 1400, 620 }, 1200, measure); CHECK(L.qHoverOk, "(2.4.4) no Q: the header hover still explains the header");
    }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
