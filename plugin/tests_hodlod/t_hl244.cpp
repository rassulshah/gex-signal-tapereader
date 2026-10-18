// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl244.cpp -o t244 && ./t244   (from plugin\tests_hodlod)
// HL 2.4.4 + BigBidAsk 1.16.2 (2026-10-18, Rassul approved 17:13 CT "build") tests, SDK-free:
//  [1] the compact table: his 10-18 17:20 chart content -> <= 380 px (was ~790), columns sized to text, header "ES M2 O0 V2" /
//      "Neg . S.Dn", "1st" / "2nd", Exp / Act / Est, footer "Exp 3:00 PM . v2.4.4"; every text inside its box; no two texts overlap
//  [2] the Gex S/R strip: its own box ABOVE the table, same width, 4 px gap, R green + levels above, S red + levels below, no current
//      price in the text (it is in the hover); a strip wider than the table widens the table (never cut / wrapped)
//  [3] the hovers carry what left the text: RTH hours, the expiry countdown, full regime / trend words, the current price
//  [4] docking fuzz (BoxDock.h placeMatched, as lsBigBidAsk 1.16.2 calls it): 9 positions x panes x avoid boxes - the Big Lot box never
//      overlaps the table, the strip, the banner, or anything right of the last bar; docked = the table's height, top and bottom
//  [5] versions: HL_VERSION == hlset::PLUGIN_VERSION == the setVersion literal; bba::VERSION == BBA_VERSION == bbaset == setVersion
#include <cstdio>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "HodLodLogic.h"
#include "HodLodGex.h"
#include "HodLodQ.h"
#include "BoxDock.h"
#include "SettingsSpec.h"
#include "HodLodSettings.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
// ~Helvetica at 96 dpi: 0.56 / 0.62 of the point size per character (as t_hl230)
static int measure(const std::string& s, int fs, bool bold) { return (int)(s.size() * fs * (bold ? 0.62 : 0.56) + 0.999); }
static bool inside(const hl::Box& a, const hl::Box& o) { return a.l >= o.l && a.r <= o.r && a.t >= o.t && a.b <= o.b; }
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static hl::View screenshotView() {   // his ES chart 10-18 17:20 CT
    hl::View v; v.has = true; v.prior = true; v.headMain = "prior RTH 10/16";
    v.headLeftSegs.push_back(hl::Seg{ "ES RTH 8:30-3:00", hl::C_SLATE });
    v.sessSegs.push_back(hl::Seg{ "Regime: Negative", hl::C_RED }); v.sessSegs.push_back(hl::Seg{ "Expiry 3:00 PM (in 21h 51m)", hl::C_GREY });
    v.trendReady = true; v.trendLvl = -2;
    v.E[hl::K_1ST] = hl::mkCell("8:33 - 9:21", hl::C_BLUE); v.headTail = "late-day LOD";
    v.A[hl::K_LABEL] = hl::mkCell("Actual", hl::C_SLATE); v.A[hl::K_1ST] = hl::mkCell("8:33 HOD in", hl::C_SLATE); v.A[hl::K_2ND] = hl::mkCell("2:09 LOD PFL", hl::C_SLATE);
    hlq::toView(v, hlq::parse("VERSION|1\nMARKET|ES\nDATE|2026-10-16\nQ|2|0|2|-\nPREV|2026-10-15|2|1|3|-\nRECV|1792230000\n", "ES"), "S. Dntrend", -2);
    auto gi = [](const char* st, const char* nm, bool above, bool mid) { hl::View::GexItem g; g.strike = st; g.name = nm; g.above = above; g.mid = mid; return g; };
    v.gexItems = { gi("7700", "PS/PS0", true, false), gi("7750", "G1", true, false), gi("7800", "", true, false), gi("7717.50", "", false, true),
                   gi("7650", "", false, false), gi("7640", "", false, false), gi("7625", "", false, false) };
    v.gexHover = { hl::Seg{ "Gex S/R - where dealers' hedging can stop or speed a move", hl::C_SLATE }, hl::Seg{ "the 3 biggest abs-GEX strikes each side", hl::C_GREY } };
    return v;
}
static std::vector<hl::Item> texts(const hl::Layout& L) { std::vector<hl::Item> t; for (const hl::Item& it : L.items) if (it.k == hl::K_TEXT && !it.s.empty()) t.push_back(it); return t; }
static int overlaps(const std::vector<hl::Item>& tx) { int n = 0; for (size_t i = 0; i < tx.size(); i++) for (size_t j = i + 1; j < tx.size(); j++) { const hl::Item& a = tx[i]; const hl::Item& b = tx[j];
    if (std::abs((a.t + a.b) / 2 - (b.t + b.b) / 2) < 6 && a.l < b.r - 1 && b.l < a.r - 1) { if (++n < 4) std::printf("  overlap '%s' / '%s'\n", a.s.c_str(), b.s.c_str()); } } return n; }
static bool has(const std::vector<hl::Item>& tx, const std::string& s, unsigned col = 0) { for (const hl::Item& it : tx) if (it.s == s && (!col || it.c1 == col)) return true; return false; }

int main() {
    auto yOf = [](long long) { return 300; };
    std::printf("[1] the compact table\n");
    {   const hl::View v = screenshotView();
        hl::Layout L = hl::layoutAll(v, hl::Box{ 0, 0, 1530, 520 }, yOf, measure, 960, 7, -1, nullptr, nullptr);
        CHECK(L.tableOk && L.gexOk, "table + strip drawn");
        const int W = L.table.r - L.table.l; std::printf("  table %d x %d px, strip %d x %d\n", W, L.table.b - L.table.t, L.gexRow.r - L.gexRow.l, L.gexRow.b - L.gexRow.t);
        CHECK(W <= 380 && W >= 200, "compact: " + std::to_string(W) + " px (was ~790)");
        const std::vector<hl::Item> tx = texts(L);
        CHECK(has(tx, "ES") && has(tx, "   M2") && has(tx, "  O0", hl::C_RED) && has(tx, "  V2"), "header left: ES M2 O0 V2 (Q colours kept)");
        CHECK(has(tx, "Neg", hl::C_RED) && has(tx, "  \xB7  S.Dn", hl::C_RED), "header right: Neg . S.Dn in the regime / trend colours");
        CHECK(has(tx, "1st") && has(tx, "2nd") && has(tx, "Exp") && has(tx, "Act") && has(tx, "Est"), "1st / 2nd, Exp / Act / Est");
        CHECK(has(tx, "8:33 - 9:21", hl::C_BLUE) && has(tx, "late-day LOD", hl::C_BLUE), "expected times blue");
        CHECK(has(tx, "prior RTH 10/16"), "Est value kept");
        CHECK(has(tx, "Exp 3:00 PM") && has(tx, std::string("v") + hl::HL_VERSION), "footer: Exp 3:00 PM . v" + std::string(hl::HL_VERSION));
        bool noLong = true; for (const hl::Item& it : tx) if (it.s.find("Expected") != std::string::npos || it.s.find("Actual") != std::string::npos || it.s.find("Estimate") != std::string::npos || it.s.find("(in ") != std::string::npos || it.s.find("RTH 8:30") != std::string::npos || it.s.find("Reg:") != std::string::npos) noLong = false;
        CHECK(noLong, "no long words / countdown / RTH hours in the text");
        int out = 0; for (const hl::Item& it : tx) { const hl::Box b = { it.l, it.t, it.r, it.b }; if (!inside(b, L.table) && !inside(b, L.gexRow)) ++out; }
        CHECK(out == 0, "every text inside the table or the strip (" + std::to_string(out) + " outside)");
        CHECK(overlaps(tx) == 0, "no two texts overlap");
        // the columns: Act 2nd starts at the 2nd heading; 1st heading left of it
        int x1 = -1, x2 = -1; for (const hl::Item& it : tx) { if (it.s == "1st") x1 = it.l; if (it.s == "2nd") x2 = it.l; }
        CHECK(x1 > L.table.l && x2 > x1 + 40, "columns in order, sized to text");
    }
    std::printf("[2] the strip\n");
    {   const hl::View v = screenshotView(); int bad = 0, cnt = 0;
        for (int W : { 1530, 1100, 800 }) for (int pos = 0; pos < 9; pos++) {
            hl::Box pane = { 0, 0, W, 520 }; const int pe = W - 300;
            hl::Layout L = hl::layoutAll(v, pane, yOf, measure, pe, pos, -1, nullptr, nullptr);
            if (!L.tableOk || !L.gexOk) { ++bad; continue; }
            ++cnt;
            if (L.gexRow.l != L.table.l || L.gexRow.r != L.table.r || L.table.t - L.gexRow.b != hl::STRIP_GAP) ++bad;
            if (!inside(L.gexRow, pane) || !inside(L.table, pane) || L.table.r > pe) ++bad;
        }
        CHECK(bad == 0 && cnt == 27, "strip: same width, 4 px above, both inside the pane and left of the last bar (" + std::to_string(cnt) + ")");
        hl::Layout L = hl::layoutAll(v, hl::Box{ 0, 0, 1530, 520 }, yOf, measure, 960, 7, -1, nullptr, nullptr);
        const std::vector<hl::Item> tx = texts(L);
        CHECK(has(tx, "R  ", hl::C_RED) && has(tx, "S  ", hl::C_GREEN) && has(tx, "PS/PS0 7700", hl::C_RED) && has(tx, "G1 7750", hl::C_RED) && has(tx, "7800", hl::C_RED)
              && has(tx, "7650", hl::C_GREEN) && has(tx, "7625", hl::C_GREEN), "R + levels above RED (resistance), S + levels below GREEN (support), names before strikes");
        {   // no two tokens touch: a text ending in a letter / digit is followed by one starting with a space (or >= 4 px apart)
            std::vector<hl::Item> row; for (const hl::Item& it : tx) if (it.t >= L.gexRow.t && it.b <= L.gexRow.b) row.push_back(it);
            std::vector<hl::Item> hd; for (const hl::Item& it : tx) if (it.t >= L.qBox.t && it.b <= L.qBox.b) hd.push_back(it);
            int touch = 0;
            for (std::vector<hl::Item>* R : { &row, &hd }) {
                std::sort(R->begin(), R->end(), [](const hl::Item& a, const hl::Item& b) { return a.l < b.l; });
                for (size_t i = 0; i + 1 < R->size(); i++) { const hl::Item& a = (*R)[i]; const hl::Item& b = (*R)[i + 1];
                    if (std::isalnum((unsigned char)a.s.back()) && std::isalnum((unsigned char)b.s[0]) && b.l - a.r < 4) { ++touch; std::printf("  touch '%s' '%s'\n", a.s.c_str(), b.s.c_str()); } }
            }
            CHECK(touch == 0, "no two tokens touch on the strip or the header (" + std::to_string(touch) + ")");
            int gaps = 0; for (const hl::Item& it : row) if (it.s == " \xB7 ") ++gaps;
            CHECK(gaps == 4, "' . ' between the levels of each side (" + std::to_string(gaps) + ")");
        }
        CHECK(!has(tx, "7717.50"), "the current price is not in the strip text");
        int rx = -1, sx = -1; for (const hl::Item& it : tx) { if (it.s == "R  ") rx = it.l; if (it.s == "S  ") sx = it.l; }
        CHECK(rx >= 0 && sx > rx, "R before S on one line");
        hl::View w = v; for (auto& g : w.gexItems) if (!g.mid) g.name = "CR0+GW0+HVL0";
        hl::Layout L2 = hl::layoutAll(w, hl::Box{ 0, 0, 1530, 520 }, yOf, measure, 1200, 7, -1, nullptr, nullptr);
        int maxR = 0; for (const hl::Item& it : texts(L2)) if (it.t >= L2.gexRow.t && it.b <= L2.gexRow.b) maxR = (std::max)(maxR, it.r);
        CHECK(L2.tableOk && L2.table.r - L2.table.l > L.table.r - L.table.l && maxR <= L2.gexRow.r - 8 && L2.gexRow.r == L2.table.r, "a long strip widens the table (never cut): " + std::to_string(L2.table.r - L2.table.l) + " px");
    }
    std::printf("[3] hovers\n");
    {   const hl::View v = screenshotView(); hl::Box pane = { 0, 0, 1530, 520 };
        hl::Layout L = hl::layoutAll(v, pane, yOf, measure, 960, 7, -1, nullptr, nullptr);
        hl::Layout Lq = L; hl::layoutQHover(Lq, v, pane, 960, measure);
        std::string all; for (size_t i = L.items.size(); i < Lq.items.size(); i++) all += Lq.items[i].s + "|";
        CHECK(Lq.qHoverOk && all.find("RTH 8:30-3:00") != std::string::npos && all.find("(in 21h 51m)") != std::string::npos, "header hover: RTH hours + expiry countdown");
        CHECK(all.find("Negative") != std::string::npos && all.find("Strong Downtrend") != std::string::npos && all.find("Expected") != std::string::npos, "header hover: full regime / trend words, what Exp means");
        CHECK(inside(Lq.qHover, pane) && Lq.qHover.r <= 960, "header hover inside the pane, left of the last bar");
        hl::Layout Lg = L; hl::layoutGexHover(Lg, v, pane, 960, measure);
        std::string g; for (size_t i = L.items.size(); i < Lg.items.size(); i++) g += Lg.items[i].s + "|";
        CHECK(Lg.gexHoverOk && g.find("Price now 7717.50") != std::string::npos, "strip hover: the current price");
    }
    std::printf("[4] docking fuzz (the Big Lot box)\n");
    {   const hl::View v = screenshotView(); unsigned long long rng = 88172645463325252ull;
        auto rnd = [&](int n) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return (int)(rng % (unsigned long long)n); };
        int runs = 0, docked = 0, badOv = 0, badH = 0, badArea = 0, drawn = 0;
        for (int it = 0; it < 3000; it++) {
            const int W = 700 + rnd(900), H = 300 + rnd(400), pe = W - 150 - rnd(250), pos = rnd(9);
            hl::Box pane = { 0, 0, W, H };
            std::vector<hl::Box> av; if (rnd(2)) av.push_back(hl::Box{ 0, 0, 440, 32 });   // the Dealer Profile banner
            hl::Box tr = { 10 + rnd(300), 30 + rnd(H - 100), 0, 0 }; tr.r = tr.l + 300 + rnd(300); tr.b = tr.t + 48; if (rnd(2)) av.push_back(tr);   // a Tape Reader banner
            hl::Layout L = hl::layoutAll(v, pane, yOf, measure, pe, pos, -1, &av, nullptr);
            if (!L.tableOk) continue;
            ++runs;
            bdk::Box full = bdk::mk(L.table.l, L.gexOk ? L.gexRow.t : L.table.t, L.table.r, L.table.b), tab = bdk::mk(L.table.l, L.table.t, L.table.r, L.table.b);
            if (L.qOk) { full.t = (std::min)(full.t, L.qBox.t); }
            const bdk::Box area = bdk::mk(64, 22, pe, H - 1);
            std::vector<bdk::Box> avoid; for (const hl::Box& a : av) avoid.push_back(bdk::mk(a.l, a.t, a.r, a.b));
            const int w = 90, h0 = 4 + 3 * 15 + 4; int h = 0;
            const bdk::Result R = bdk::placeMatched(true, full, tab, w, h0, area, 9, avoid, &h);
            if (R.mode == bdk::NONE) continue;
            ++drawn;
            if (bdk::overlap(R.b, full) || bdk::overlap(R.b, tab)) ++badOv;
            for (const bdk::Box& a : avoid) if (bdk::overlap(R.b, a)) ++badOv;
            if (!bdk::inside(area, R.b) || R.b.r > pe) ++badArea;
            if (R.mode == bdk::DOCKED && (R.side == bdk::S_RIGHT || R.side == bdk::S_LEFT) && R.why.find("table height") != std::string::npos) {
                ++docked; if (R.b.t != tab.t || R.b.b != tab.b || h != tab.b - tab.t) ++badH; }
        }
        std::printf("  %d layouts, %d boxes drawn, %d docked at the table height\n", runs, drawn, docked);
        CHECK(runs > 2000 && drawn > runs * 9 / 10, "the box is drawn in nearly every layout");
        CHECK(docked > drawn / 2, "docked beside the table at its height in most layouts");
        CHECK(badOv == 0, "never over the table, the strip or a banner (" + std::to_string(badOv) + ")");
        CHECK(badArea == 0, "never right of the last bar / outside the area (" + std::to_string(badArea) + ")");
        CHECK(badH == 0, "docked = the table's top and bottom (not the strip's)");
        // his chart: the table bottom centre, room on its right -> docked right, same height, gap 4
        hl::Layout L = hl::layoutAll(v, hl::Box{ 0, 0, 1530, 520 }, yOf, measure, 960, 7, -1, nullptr, nullptr);
        const bdk::Box tab = bdk::mk(L.table.l, L.table.t, L.table.r, L.table.b), full = bdk::mk(L.table.l, L.gexRow.t, L.table.r, L.table.b);
        int h = 0; const bdk::Result R = bdk::placeMatched(true, full, tab, 90, 53, bdk::mk(64, 22, 960, 519), 9, {}, &h);
        CHECK(R.mode == bdk::DOCKED && R.side == bdk::S_RIGHT && R.b.l == tab.r + bdk::GAP && R.b.t == tab.t && R.b.b == tab.b, "his chart: docked right of the table, same height");
        const bdk::Result R2 = bdk::placeMatched(false, full, tab, 90, 53, bdk::mk(64, 22, 960, 519), 9, {}, &h);
        CHECK(R2.mode == bdk::OWN && R2.b.b - R2.b.t == 53, "no HOD/LOD table: its own Position at its own height");
    }
    std::printf("[5] versions\n");
    {   CHECK(std::string(hl::HL_VERSION) == "2.4.4" && std::string(hlset::PLUGIN_VERSION) == hl::HL_VERSION, "HL_VERSION == hlset::PLUGIN_VERSION == 2.4.4");
        const std::string src = readAll("../HodLod.cpp"); CHECK(src.find(std::string("p->setVersion(\"") + hl::HL_VERSION + "\")") != std::string::npos, "HodLod setVersion literal");
        CHECK(hlset::spec().paramVersion == 7 && ss::find(hlset::spec(), "offset_x") < 0 && ss::find(hlset::spec(), "offset_y") < 0, "Offset X / Y removed, parameter version 7");
        const std::string b = readAll("../BigBidAsk.cpp"), bl = readAll("../BigBidAskLogic.h"), bs = readAll("../BigBidAskSettings.h");
        CHECK(b.find("#define BBA_VERSION \"1.16.2\"") != std::string::npos && b.find("p->setVersion(\"1.16.2\")") != std::string::npos
              && bl.find("VERSION = \"1.16.2\"") != std::string::npos && bs.find("PLUGIN_VERSION = \"1.16.2\"") != std::string::npos, "BigBidAsk 1.16.2 everywhere");
        CHECK(std::string(bdk::VERSION) == "1.1.0", "BoxDock 1.1.0");
    }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
