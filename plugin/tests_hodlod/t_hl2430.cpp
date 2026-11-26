// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2430.cpp -o t2430 && ./t2430   (from plugin\tests_hodlod)
// lsHodLod 2.43.0 (Rassul 2026-11-22 19:10 CT "i need to simplify the display and only keep the now line"; 19:36 "1. yes. 2. no go";
// mockup now-line-mockup.html): NOW LINE ONLY - the box and the Options line are not drawn, the Now line sits on the box's bottom edge.
//  [1] the two switches are off; HL_VERSION == PLUGIN_VERSION == the ONE setVersion literal (version-agnostic); the host is wired to them
//  [2] hl::nowSpot: form 0 = the 2.42.4 arithmetic exactly, form 1 = on the hidden box's bottom edge, form 2 = the pane's bottom centre
//  [3] the host's sequence on the real layout, 6 markets x 6 panes x projection up / down: box laid out -> layout redone with no table ->
//      no table item is left, the projection is the "Show table off" one, the real Now line pieces land inside the pane, centred on the
//      box's spot, bottom edge on the box's bottom edge, below the title row, and take less height than the box did
//  [4] the Options line: laid out only for its text, never part of the picture; the Now line's bottom does not move for it
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../NowLevels.h"
#include "../GammaLine.h"
#include "lib_widths.h"
#include "scen.h"
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
static int MS(const std::string& s, int fs, bool b) { return (int)(measureLib(s, fs, b) + 0.5); }
static std::string slurp(const char* p) { std::ifstream f(p, std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static int count(const std::string& h, const std::string& n) { int c = 0; for (size_t p = h.find(n); p != std::string::npos; p = h.find(n, p + 1)) ++c; return c; }
static std::vector<int> ver(const std::string& s) { std::vector<int> v; std::stringstream ss(s); std::string t; while (std::getline(ss, t, '.')) v.push_back(std::atoi(t.c_str())); return v; }
// the Now line as on his CL chart, 2026-11-22 7:10 PM CT: "TESTING: 2hH Bk / Sw | Next v 3hH . ^ VAH | Last Bk ^ PHH"
static std::vector<nowl::Seg> hisLine() {
    nowl::Out o; o.ok = true; o.bs.state = 3; o.bs.name = "2hH";
    o.dn.ok = true; o.dn.name = "3hH"; o.up.ok = true; o.up.name = "VAH";
    o.haveBk = true; o.bk.lv = "PHH"; o.bk.dir = 1;
    return nowl::segments(o);
}
int main() {
    (void)&itemsText; (void)&fnv;   // scen.h helpers of the golden tests, not needed here
    // [1] switches, versions, wiring
    CHECK(!hl::HL_BOX_DRAW, "the box is not drawn");
    CHECK(!hl::HL_OPTIONS_LINE_DRAW, "the Options line is not drawn");
    CHECK(std::string(hl::HL_VERSION) == hlset::PLUGIN_VERSION, "HL_VERSION == PLUGIN_VERSION");
    {   const std::vector<int> v = ver(hl::HL_VERSION);
        CHECK(v.size() == 3 && (v[0] > 2 || (v[0] == 2 && v[1] >= 43)), "version 2.43.0 or later carries this display"); }
    {   const std::string src = slurp("../HodLod.cpp");
        CHECK(!src.empty(), "HodLod.cpp read");
        CHECK(count(src, "setVersion(") == 1, "exactly ONE setVersion( in HodLod.cpp (gex-build reads it with findstr)");
        CHECK(src.find(std::string("p->setVersion(\"") + hl::HL_VERSION + "\")") != std::string::npos, "setVersion literal == HL_VERSION");
        CHECK(src.find("hl::nowSpot(boxLaid, boxAt, boxNotes, L.tableOk") != std::string::npos, "the host places the Now line with hl::nowSpot");
        CHECK(src.find("if (!hl::HL_BOX_DRAW && boxLaid)") != std::string::npos, "the host takes the box out of the drawing");
        CHECK(src.find("if (!hl::HL_OPTIONS_LINE_DRAW) { if (G.ok)") != std::string::npos, "the host never paints the Options line");
        CHECK(src.find("if (!gmDrawn && hl::HL_OPTIONS_LINE_DRAW)") != std::string::npos, "no regime pill at the left of the Now row either");
        CHECK(src.find("\"DISPLAY,\"") != std::string::npos, "status line DISPLAY");
        // the spoken alerts and the gamma record are not behind either switch (they sit in their own blocks, before the painting)
        const size_t voice = src.find("THE SPOKEN SWEEP ALERTS (SweepVoice.h): this chart's drawn Sw marks"), rec = src.find("gml::closedStep("), sw1 = src.find("if (!hl::HL_BOX_DRAW && boxLaid)");
        const size_t paint = src.find("hl::nowSpot(boxLaid");
        CHECK(voice != std::string::npos && rec != std::string::npos && sw1 != std::string::npos && paint != std::string::npos, "anchors found");
        CHECK(rec < paint && voice < paint, "the record and the spoken alerts run before the painting block");
        const std::string between = src.substr(rec, paint - rec);
        CHECK(between.find("HL_BOX_DRAW") == std::string::npos && between.find("HL_OPTIONS_LINE_DRAW") == std::string::npos, "no switch between the record / voice code and the painting");
    }

    // [2] nowSpot
    {   const hl::Box box = { 10, 400, 700, 520 }, gexHi = { 10, 385, 700, 399 }, gexLo = { 10, 410, 700, 424 }, none = { 0, 0, 0, 0 };
        const std::vector<hl::Box> noNotes;
        hl::NowSpot s = hl::nowSpot(true, box, noNotes, true, false, none, 600, 4, 990);
        CHECK(s.form == 0 && s.cx == 355 && s.left == 10 && s.bottom == 396, "form 0: 4 px above the drawn box");
        s = hl::nowSpot(true, box, noNotes, true, true, gexHi, 600, 4, 990);
        CHECK(s.form == 0 && s.bottom == 381, "form 0: above the GEX strip when that is drawn");
        s = hl::nowSpot(true, box, noNotes, true, true, gexLo, 600, 4, 990);
        CHECK(s.form == 0 && s.bottom == 396, "form 0: a GEX strip below the table top does not lower it");
        // form 0 == the 2.42.4 host arithmetic on a grid
        int same = 0, n = 0;
        for (int l = 0; l <= 300; l += 60) for (int w = 200; w <= 1400; w += 300) for (int t = 100; t <= 500; t += 100) for (int g = 0; g < 3; g++) {
            const hl::Box b = { l, t, l + w, t + 120 }, gr = { l, g == 1 ? t - 15 : t + 9, l + w, t };
            const bool gexOk = g > 0;
            const int anchorT = gexOk ? (std::min)(b.t, gr.t) : b.t, cxOld = (b.l + b.r) / 2, xLOld = b.l;
            const hl::NowSpot q = hl::nowSpot(true, b, noNotes, true, gexOk, gr, 900, 4, 1500);
            ++n; if (q.form == 0 && q.bottom == anchorT - 4 && q.cx == cxOld && q.left == xLOld) ++same;
        }
        CHECK(n == 450 && same == n, "form 0 = the 2.42.4 arithmetic on 450 boxes");
        s = hl::nowSpot(true, box, noNotes, false, true, gexHi, 600, 4, 990);
        CHECK(s.form == 1 && s.cx == 355 && s.left == 10 && s.bottom == 520, "form 1: on the hidden box's bottom edge, centred on its spot");
        {   std::vector<hl::Box> nb; nb.push_back(hl::Box{ 10, 520, 700, 538 });
            s = hl::nowSpot(true, box, nb, false, false, none, 600, 4, 990); CHECK(s.form == 1 && s.bottom == 538, "form 1: a note row under the table counts");
            s = hl::nowSpot(true, box, nb, false, false, none, 530, 4, 990); CHECK(s.form == 1 && s.bottom == 529, "form 1: never below the pane");
            nb[0] = hl::Box{ 10, 380, 700, 398 };
            s = hl::nowSpot(true, box, nb, false, false, none, 600, 4, 990); CHECK(s.bottom == 520, "form 1: a note row above the table does not raise it");
            nb[0] = hl::Box{ 50, 520, 50, 560 };
            s = hl::nowSpot(true, box, nb, false, false, none, 600, 4, 990); CHECK(s.bottom == 520, "form 1: an empty note row is ignored"); }
        s = hl::nowSpot(false, none, noNotes, false, false, none, 600, 4, 990);
        CHECK(s.form == 2 && s.cx == 497 && s.left == 4 && s.bottom == 574, "form 2: the pane's bottom centre, 26 px up (as before)");
        s = hl::nowSpot(true, none, noNotes, false, false, none, 600, 4, 990);
        CHECK(s.form == 2, "form 2: a box with no area is no spot");
        s = hl::nowSpot(false, box, noNotes, true, false, none, 600, 4, 990);
        CHECK(s.form == 0, "a drawn box always wins");
    }

    // [3] + [4] the host's sequence on the real layout
    {   const std::vector<nowl::Seg> segs = hisLine();
        CHECK(nowl::plain(segs).find("TESTING") != std::string::npos && nowl::plain(segs).find("2hH") != std::string::npos && nowl::plain(segs).find("LAST BK") != std::string::npos, "his line's pieces (2.47.0 chip format)");
        auto meas = [&](const std::string& s2, bool bold) { return MS(s2, 10, bold); };
        const int PW[3] = { 1530, 1100, 760 }, PH[2] = { 640, 420 };
        int cases = 0, laid = 0, onEdge = 0, centred = 0, shorter = 0, uncut = 0, cutBefore = 0;
        for (int m = 0; m < 6; m++) for (int a = 0; a < 3; a++) for (int c = 0; c < 2; c++) for (int up = 0; up < 2; up++) {
            const hl::Box pane = { 0, 0, PW[a], PH[c] }; const int pe = (int)(PW[a] * 0.75);
            hl::View v = mkView(m);
            v.projOk = true; v.projUp = up == 1; v.projFrom = up ? 1000 : 9000; v.projTo = up ? 9000 : 1000; v.projTxt = up ? "Proj HOD 61.42" : "Proj LOD 60.22";
            // price -> y: the projection runs from one end of the pane to the other, so it must cross the box's rows
            auto yOf = [&](long long p) { return (int)(pane.t + 30 + (10000 - p) * (long long)(pane.b - pane.t - 40) / 10000); };
            hl::Opts O;
            // pass 1 (as the host): the box laid out, widened to the last candle. projX is found afterwards so that it stands INSIDE the box's columns
            hl::Layout L1 = hl::layoutAll(v, pane, yOf, MS, pe, hl::POS_DEFAULT, -1, nullptr, &O);
            if (L1.tableOk) hl::stretchToAxis(L1, v, pane, yOf, MS, pe, hl::POS_DEFAULT, -1, nullptr, O, pe + 1);
            const int projX = L1.tableOk ? (L1.table.l + L1.table.r) / 2 : pe - 40;
            hl::Layout L = hl::layoutAll(v, pane, yOf, MS, pe, hl::POS_DEFAULT, projX, nullptr, &O);
            if (L.tableOk) hl::stretchToAxis(L, v, pane, yOf, MS, pe, hl::POS_DEFAULT, projX, nullptr, O, pe + 1);
            const bool boxLaid = L.tableOk; const hl::Box boxAt = L.table; const std::vector<hl::Box> boxNotes = L.noteRows;
            int projLen1 = 0; for (const hl::Item& it : L.items) if (it.elem == hl::E_PROJ && it.k == hl::K_LINE && it.l == it.r && it.w == 2) projLen1 += std::abs(it.b - it.t);
            // the 2.43.0 step
            if (!hl::HL_BOX_DRAW && boxLaid) { hl::Opts O3 = O; O3.table = false; L = hl::layoutAll(v, pane, yOf, MS, pe, hl::POS_DEFAULT, projX, nullptr, &O3); }
            hl::applyOpts(L, O);
            ++cases; if (boxLaid) ++laid;
            bool tableItem = false; for (const hl::Item& it : L.items) if (it.role == hl::R_TABLE || it.role == hl::R_NOTE) tableItem = true;
            CHECK(!tableItem && !L.tableOk && !L.gexOk && !L.qOk && !L.preOk && !L.dayOk && L.noteRows.empty(), "no part of the box is left in the picture");
            {   hl::Opts Ooff; Ooff.table = false; hl::Layout R = hl::layoutAll(v, pane, yOf, MS, pe, hl::POS_DEFAULT, projX, nullptr, &Ooff); hl::applyOpts(R, Ooff);
                bool eq = R.items.size() == L.items.size();
                for (size_t i = 0; eq && i < R.items.size(); i++) { const hl::Item& x = R.items[i]; const hl::Item& y = L.items[i];
                    eq = x.k == y.k && x.l == y.l && x.t == y.t && x.r == y.r && x.b == y.b && x.c1 == y.c1 && x.c2 == y.c2 && x.s == y.s && x.elem == y.elem; }
                CHECK(eq, "what is left is exactly the Show-table-off picture"); }
            int projLen2 = 0; for (const hl::Item& it : L.items) if (it.elem == hl::E_PROJ && it.k == hl::K_LINE && it.l == it.r && it.w == 2) projLen2 += std::abs(it.b - it.t);
            if (boxLaid) { CHECK(projLen2 >= projLen1, "the projection is never shorter without the table"); if (projLen2 > projLen1) ++uncut; if (projLen1 < projLen2) ++cutBefore; }
            // the Now line
            const int l = pane.l + 4, r = pe - 6;
            const hl::NowSpot NS = hl::nowSpot(boxLaid, boxAt, boxNotes, L.tableOk, L.gexOk, L.gexRow, pane.b, l, r);
            CHECK(NS.form == (boxLaid ? 1 : 2), "spot form: 1 on a laid-out box, 2 with none");
            gml::Draw G;   // [4] HL_OPTIONS_LINE_DRAW false: the host resets it after taking its text
            CHECK(!G.ok && gml::nowBottom(G, NS.bottom) == NS.bottom, "the Options line does not move the Now line");
            const nowl::Draw D = nowl::layout(segs, meas, NS.cx, gml::nowBottom(G, NS.bottom), l, r, 10);
            CHECK(D.ok, "the Now line is laid out");
            if (!D.ok) continue;
            CHECK(D.l >= l && D.r <= r && D.r > D.l, "inside the pane, left of the last candle");
            CHECK(D.b <= pane.b - 1 && D.t > pane.t + 24 && D.b - D.t == 18, "18 px tall, under the title row, inside the pane");
            for (const nowl::Item& it : D.items) CHECK(it.l >= D.l && it.r <= D.r && it.t >= D.t && it.b <= D.b, "every piece inside the line's box");
            if (boxLaid) {
                int lowest = boxAt.b; for (const hl::Box& nb : boxNotes) if (nb.b > lowest) lowest = nb.b;
                CHECK(D.b == (std::min)(lowest, pane.b - 1), "bottom edge on the box's bottom edge"); if (D.b == boxAt.b) ++onEdge;
                const int want = (boxAt.l + boxAt.r) / 2 - (D.r - D.l) / 2;
                if (want >= l && want + (D.r - D.l) <= r) { CHECK(D.l == want, "centred on the box's spot"); ++centred; }
                CHECK(D.t >= boxAt.t, "inside the rows the box used"); if (D.b - D.t < boxAt.b - boxAt.t) ++shorter;
            }
        }
        CHECK(cases == 72, "72 cases");
        CHECK(laid >= 60, "the box is laid out in nearly every case");
        CHECK(onEdge == laid && shorter == laid, "always on the box's edge, always less tall than the box");
        CHECK(centred >= laid - 12, "centred on the box's spot wherever the line fits");
        CHECK(uncut > 0 && cutBefore == uncut, "the table used to cut the projection in some cases; it no longer does");
        std::printf("  cases %d, box laid out %d, on its bottom edge %d, centred %d, projection uncut now %d\n", cases, laid, onEdge, centred, uncut);
    }
    std::printf("t_hl2430 (lsHodLod 2.43.0 now line only): %d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
