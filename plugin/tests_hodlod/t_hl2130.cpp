// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2130.cpp -o t2130 && ./t2130   (from plugin\tests_hodlod)
// lsHodLod 2.13.0 / 2.14.0 (2026-11-01, Rassul 18:52-20:38 CT mockups "HodLod box" v8-v14 + "go" / "ok") - the HOD / LOD sweep panels, SDK-free:
//  2.14.0: panel strips, column heads, % bars, the 1st-extreme time table, header extras (Day forecast / RTH % / bar time left)
//  [1] HodLodSweeps.h: the nightly file + the Dealer Profile key levels -> rows (ON3 range, wrong side of the open dimmed, "?" when no file / n < 20)
//  [2] layout: Session | HOD sweeps | LOD sweeps side by side, the session section unchanged, no overlaps, every text inside the box
//  [3] no room on the pane -> the 2.12.0 box (never the "pane too small" line); 9 positions x 3 widths
#include <cstdio>
#include <cmath>
#include <algorithm>
#include "HodLodLogic.h"
#include "HodLodSweeps.h"
#include "lib_widths.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static int measure(const std::string& s, int fs, bool bold) { return (int)(s.size() * fs * (bold ? 0.62 : 0.56) + 0.999); }
static bool inside(const hl::Box& a, const hl::Box& o) { return a.l >= o.l && a.r <= o.r && a.t >= o.t && a.b <= o.b; }
static std::vector<hl::Item> texts(const hl::Layout& L) { std::vector<hl::Item> t; for (const hl::Item& it : L.items) if (it.k == hl::K_TEXT && !it.s.empty()) t.push_back(it); return t; }
static int overlaps(const std::vector<hl::Item>& tx) { int n = 0; for (size_t i = 0; i < tx.size(); i++) for (size_t j = i + 1; j < tx.size(); j++) { const hl::Item& a = tx[i]; const hl::Item& b = tx[j];
    if (a.t < b.b - 1 && b.t < a.b - 1 && a.l < b.r - 1 && b.l < a.r - 1) { if (++n < 4) std::printf("  overlap '%s' / '%s'\n", a.s.c_str(), b.s.c_str()); } } return n; }
static const hl::Item* find(const std::vector<hl::Item>& tx, const std::string& s) { for (const hl::Item& it : tx) if (it.s == s) return &it; return nullptr; }
static const char* FILE_ES = "VERSION,1\nMARKET,ES\nROW,HI,PHH,46,54,117\nROW,HI,ON3,41,48,118\nROW,HI,DPoc,26,15,58\nROW,HI,VAH,22,18,82\nROW,HI,PDH,16,15,94\nROW,HI,PFH,14,14,12\n"
                             "ROW,LO,PHL,41,53,130\nROW,LO,ON3,38,50,130\nROW,LO,DPoc,28,19,69\nROW,LO,VAL,11,10,88\nROW,LO,PDL,9,9,102\nROW,LO,PFL,6,6,106\nTIME,43,15,17,25,248\nONCUT,0.62,0.95\nDAY,0,11,20,84\nPROJ,0,74,84\nDAY,1,21,15,84\nPROJ,1,101,84\nDAY,2,36,6,84\nPROJ,2,119,84\n";
static const char* KEYS_ES = "KEYLV,1,1793500000\nL|ONH|7713.00|0\nL|AsiaHI/PFL|7710.00|0\nL|LonHI|7693.75|0\nL|PHH|7674.50|0\nL|Open|7669.50|0\nL|AsiaLO|7665.25|0\nL|PHL|7664.25|0\n"
                             "L|ONL/LonLO|7664.00|0\nL|DPoc|7751.25|0\nL|VAH|7757.50|0\nL|VAL|7738.25|0\nL|PDH|7767.25|0\nL|PDL|7725.25|0\nL|PFH|7773.75|0\nL|HrHI|7800.00|1\n";
static hl::View shotView(bool sw) {   // his ES box (Exp / Act / Feed) + the Oct 30 sweep rows
    hl::View v; v.has = true; v.prior = false;
    v.headLeftSegs.push_back(hl::Seg{ "ES RTH 8:30-3:00", hl::C_SLATE }); v.trendReady = true; v.trendLvl = -1;
    v.sessSegs.push_back(hl::Seg{ "Regime Negative", hl::C_RED });
    v.E[hl::K_1ST] = hl::mkCell("8:33 - 9:21", hl::C_BLUE); v.headTail = "LOD after 9:30";
    v.A[hl::K_1ST] = hl::mkCell("8:33", hl::C_SLATE); v.A[hl::K_1ST].s2 = "HOD"; v.A[hl::K_1ST].col2 = hl::C_GREEN;
    v.A[hl::K_2ND] = hl::mkCell("1:54", hl::C_SLATE); v.A[hl::K_2ND].s2 = "LOD"; v.A[hl::K_2ND].col2 = hl::C_RED;
    if (sw) {
        std::vector<hsw::Lv> L; hsw::parseKeys(KEYS_ES, 1793500100, 80 * 3600, L);
        std::vector<hsw::Row> hi, lo; hsw::build(hsw::parse(FILE_ES, "ES"), L, 7669.50, 2, hi, lo);
        const hsw::File F = hsw::parse(FILE_ES, "ES"); v.tmOk = hsw::timeCum(F, v.tmCum);
        v.hdrExtra.push_back(hl::Seg{ "Day  " + hsw::dayText(F, hsw::onTercile(F, 49.0, 58.53)), hl::C_SLATE });
        v.hdrLeft.push_back(hl::Seg{ "RTH 83% done", hl::C_SLATE }); v.hdrLeft.push_back(hl::Seg{ "Bar 2:13 left", hl::C_SLATE });
        v.swOn = true; for (auto& r : hi) v.swHi.push_back(hl::View::SwRow{ r.name, r.px, r.pct, r.elig }); for (auto& r : lo) v.swLo.push_back(hl::View::SwRow{ r.name, r.px, r.pct, r.elig });
    }
    return v;
}
int main() {
    auto yOf = [](long long) { return 300; };
    std::printf("[1] rows\n");
    {   std::vector<hsw::Lv> L; CHECK(hsw::parseKeys(KEYS_ES, 1793500100, 80 * 3600, L), "key levels parsed");
        CHECK(!hsw::parseKeys(KEYS_ES, 1793500000 + 81 * 3600, 80 * 3600, L), "a key levels file over 80 h old is refused");
        hsw::parseKeys(KEYS_ES, 1793500100, 80 * 3600, L);
        bool hr = false; for (auto& x : L) if (x.name == "HrHI") hr = true; CHECK(!hr, "pivot-only rows skipped");
        const hsw::File F = hsw::parse(FILE_ES, "ES"); CHECK(F.ok && F.hi.size() == 6 && F.lo.size() == 6, "sweep file parsed");
        CHECK(!hsw::parse(FILE_ES, "NQ").ok, "another market's file refused");
        std::vector<hsw::Row> hi, lo; hsw::build(F, L, 7669.50, 2, hi, lo);
        CHECK(hi.size() == 6 && lo.size() == 6, "6 rows a side");
        CHECK(hi[0].name == "PHH" && hi[0].px == "7,674.50" && hi[0].pct == "46%" && hi[0].elig, "PHH 7,674.50 46%");
        CHECK(hi[1].name == "ONH / LonHI / AsiaHI" && hi[1].px == "7,693.75 - 7,713.00" && hi[1].pct == "41%", "ON3 high: the range of the three (" + hi[1].px + ")");
        CHECK(hi[5].pct == "14%?", "fewer than 20 days: % with ?");
        CHECK(lo[0].px == "7,664.25" && lo[0].pct == "41%", "PHL");
        CHECK(lo[1].px == "7,664.00 - 7,665.25", "ON3 low: ONL / LonLO / AsiaLO range (" + lo[1].px + ")");
        CHECK(!lo[2].elig && lo[2].px == "above open" && !lo[4].elig, "DPoc / PDL above the open: dimmed, 'above open'");
        std::vector<hsw::Row> h2, l2; hsw::build(hsw::File(), L, 0, 2, h2, l2);
        CHECK(h2[0].pct == "?" && h2[2].elig && h2[2].px == "7,751.25", "no file: '?'; before the open every level shows its price");
        CHECK(hsw::timeText(F) == "0-15m 43% \xB7 15-30m 15% \xB7 30-60m 17% \xB7 60m+ 25%", "1st-extreme time row text");
        CHECK(hsw::timeText(hsw::File()).empty(), "no file: no time row");
        int c[4]; CHECK(hsw::timeCum(F, c) && c[0] == 43 && c[1] == 58 && c[2] == 75 && c[3] == 25, "cumulative 43 / 58 / 75 / 25");
        CHECK(hsw::onTercile(F, 49.0, 58.53) == 1 && hsw::onTercile(F, 30.0, 58.53) == 0 && hsw::onTercile(F, 70.0, 58.53) == 2, "ON range terciles (0.84 ADR = mid)");
        CHECK(hsw::dayText(F, 1) == "ON range mid \xBB Trend 21% \xB7 Chop 15%" && hsw::dayText(F, -1).empty(), "day text: " + hsw::dayText(F, 1));
        CHECK(hsw::dayText(F, 2) == "ON range big \xBB Trend 36% \xB7 Chop 6%", "big tercile");
        std::vector<hsw::Lv> none; hsw::build(F, none, 7669.5, 2, h2, l2); CHECK(h2[0].px == "-" && h2[0].pct == "46%", "no key levels file: '-' prices, % kept");
    }
    std::printf("[2] layout\n");
    {   const hl::View v = shotView(true);
        hl::Layout L = hl::layoutAll(v, hl::Box{ 0, 0, 2400, 520 }, yOf, measure, 1900, 6, -1, nullptr, nullptr);
        CHECK(L.tableOk && L.swOk && L.swCols.size() == 2, "table with 2 sweep sections");
        const std::vector<hl::Item> tx = texts(L);
        const hl::Item* ex = find(tx, "Exp"); const hl::Item* th = find(tx, "HOD SWEEPS \xB7 1st = HOD"); const hl::Item* tl = find(tx, "LOD SWEEPS \xB7 1st = LOD");
        CHECK(ex && th && tl && ex->r < th->l && th->r < tl->l, "order left to right: Session | HOD | LOD");
        CHECK(L.swCols.size() == 2 && L.swCols[0].l > ex->r && L.swCols[1].l == L.swCols[0].r && L.swCols[1].r == L.table.r, "sections butt up, the LOD one ends at the box's right edge");
        CHECK(find(tx, "SESSION") && find(tx, "HOD SWEEPS \xB7 1st = HOD") && find(tx, "LOD SWEEPS \xB7 1st = LOD"), "three panel strips");
        const hl::Item* d15 = find(tx, "0-15m"); const hl::Item* p58 = find(tx, "58%"); const hl::Item* lab = find(tx, "1st extreme");
        CHECK(lab && d15 && d15->l - lab->r < 120, "2.14.1 compact: the time columns start right after the label");
        CHECK(L.swCols[0].l - L.table.l < 480, "2.14.1 compact: the session panel is as wide as its content (" + std::to_string(L.swCols[0].l - L.table.l) + " px)");
        CHECK(d15 && p58 && d15->r <= L.swCols[0].l && p58->r <= L.swCols[0].l && p58->t > d15->t, "time table in the session panel (heads, then 58%)");
        CHECK(find(tx, "Level") && find(tx, "Today") && find(tx, "Swept"), "column heads");
        int bars = 0; for (const hl::Item& it : L.items) if (it.k == hl::K_FILL && (it.c1 == hl::C_BAR_R || it.c1 == hl::C_BAR_G || it.c1 == hl::C_BAR_RD || it.c1 == hl::C_BAR_GD)) ++bars;
        CHECK(bars == 12, "12 % bars (" + std::to_string(bars) + ")");
        const hl::Item* dy = nullptr; for (const hl::Item& it : tx) if (it.s.find("Day  ON range mid") == 0) dy = &it;
        const hl::Item* bl = find(tx, "Bar 2:13 left"); const hl::Item* rh = find(tx, "RTH 83% done");
        const hl::Item* es = find(tx, "ES");
        { const hl::Item* dt = find(tx, "Downtrend"); const hl::Item* gp = find(tx, "-GAMMA");
          CHECK(dt && gp && gp->r <= dt->l && std::abs((gp->l - 6 + dt->r) / 2 - (L.table.l + L.table.r) / 2) <= 2 && bl && bl->r < gp->l && dy && dt->r < dy->l, "header: -GAMMA Downtrend centred"); }
        CHECK(dy && bl && rh && es && dy->t < L.table.t + 30 && es->r <= rh->l && rh->r <= bl->l && bl->r < dy->l, "header: ES | RTH | Bar ... Day (right)");
        CHECK(!find(tx, "Feed"), "no Feed row");
        CHECK(find(tx, "7,674.50") && find(tx, "46%") && find(tx, "above open") && find(tx, "above open")->c1 == hl::C_DIM, "prices, % and a dimmed 'above open'");
        int out = 0; for (const hl::Item& it : tx) if (!inside(hl::Box{ it.l, it.t, it.r, it.b }, L.table) && !(L.gexOk && inside(hl::Box{ it.l, it.t, it.r, it.b }, L.gexRow))) ++out;
        CHECK(out == 0, "every text inside the box"); CHECK(overlaps(tx) == 0, "no overlaps");
        const int rowH = hl::lineH(ex->fs) + 1; CHECK(L.table.b - L.table.t == 1 + rowH + 4 + 1 + 8 * rowH + 3 + 2, "height = header band + strip + heads + 6 rows (" + std::to_string(L.table.b - L.table.t) + ")");
        {   hl::Layout LR = hl::layoutAll(v, hl::Box{ 0, 0, 2400, 520 }, yOf, measureLib, 1900, 6, -1, nullptr, nullptr);   // real Arial metrics
            const std::vector<hl::Item> txr = texts(LR); CHECK(LR.swOk && overlaps(txr) == 0, "Arial metrics: no overlaps");
            std::FILE* o = std::fopen("/tmp/hl214_items.txt", "w");   // for the render check
            const hl::Layout& L = LR;
            if (o) { std::fprintf(o, "BOX %d %d %d %d\n", L.table.l, L.table.t, L.table.r, L.table.b);
                for (const hl::Item& it : L.items) std::fprintf(o, "%d|%d|%d|%d|%d|%u|%u|%d|%d|%d|%s\n", it.k, it.l, it.t, it.r, it.b, it.c1, it.c2, it.fs, it.bold ? 1 : 0, it.w, it.s.c_str());
                std::fclose(o); } }
        hl::Layout L0 = hl::layoutAll(shotView(false), hl::Box{ 0, 0, 2400, 520 }, yOf, measure, 1900, 6, -1, nullptr, nullptr);
        CHECK(!L0.swOk, "no sweep rows: the 2.12.0 box");
        std::printf("  box %d x %d px\n", L.table.r - L.table.l, L.table.b - L.table.t);
    }
    std::printf("[3] fuzz + no room\n");
    {   int bad = 0, runs = 0, withSw = 0;
        for (int W : { 2400, 1530, 900, 600 }) for (int pos = 0; pos < 9; pos++) {
            const hl::View v = shotView(true); hl::Layout L = hl::layoutAll(v, hl::Box{ 0, 0, W, 520 }, yOf, measure, W - 300, pos, -1, nullptr, nullptr);
            if (!L.tableOk) { ++bad; continue; } ++runs; if (L.swOk) ++withSw;
            const std::vector<hl::Item> tx = texts(L); if (overlaps(tx)) { ++bad; continue; }
            for (const hl::Item& it : tx) if (!inside(hl::Box{ it.l, it.t, it.r, it.b }, L.table) && !(L.gexOk && inside(hl::Box{ it.l, it.t, it.r, it.b }, L.gexRow))) { ++bad; break; }
            if (W == 600 && L.swOk) ++bad;   // no room: the 2.12.0 box (2.14.1: the compact box fits 900)
        }
        CHECK(bad == 0 && runs == 36 && withSw >= 18, std::to_string(runs) + " layouts clean, " + std::to_string(withSw) + " with sweeps (" + std::to_string(bad) + " bad)");
    }
    {   // (2.17.3, Rassul 2026-11-02 14:49 CT) "| Dly: Dn, Wk | Hrly: Side, Wk | 3m: Dn, Wk" centred in the header, before the -GAMMA pill
        hl::View v0 = shotView(true); hl::View v = shotView(true);
        v.htf = { hl::Seg{ "Dly:", hl::C_GREY }, hl::Seg{ "Dn,", hl::C_RED }, hl::Seg{ "Wk", hl::C_RED }, hl::Seg{ "Hrly:", hl::C_GREY }, hl::Seg{ "Side,", hl::C_SLATE }, hl::Seg{ "Wk", hl::C_RED },
                  hl::Seg{ "3m:", hl::C_GREY }, hl::Seg{ "Dn,", hl::C_RED }, hl::Seg{ "Wk", hl::C_RED } };
        hl::Layout L0 = hl::layoutAll(v0, hl::Box{ 0, 0, 2400, 520 }, yOf, measure, 1900, 6, -1, nullptr, nullptr);
        hl::Layout L = hl::layoutAll(v, hl::Box{ 0, 0, 2400, 520 }, yOf, measure, 1900, 6, -1, nullptr, nullptr);
        const std::vector<hl::Item> tx = texts(L);
        const hl::Item* a = find(tx, "Dly:"); const hl::Item* m3 = find(tx, "3m:"); const hl::Item* bl = find(tx, "Bar 2:13 left"); const hl::Item* gp = find(tx, "-GAMMA"); const hl::Item* dt = find(tx, "Downtrend");
        int out = 0; for (const hl::Item& it : tx) if (!inside(hl::Box{ it.l, it.t, it.r, it.b }, L.table)) ++out;
        CHECK(a && m3 && bl && gp && dt && std::abs(a->t - bl->t) <= 1 && a->l > bl->r && a->l < m3->l && m3->r < gp->l, "(2.17.3) Dly | Hrly | 3m in the header, after 'Bar left', before the gamma pill");
        const int mid = (L.table.l + L.table.r) / 2, cen = (a->l + dt->r) / 2;
        CHECK(std::abs(cen - mid) <= 40, "(2.17.3) the HTF + pill + trend block is centred (" + std::to_string(cen - mid) + " px off)");
        CHECK(out == 0 && overlaps(tx) == 0, "(2.17.3) inside the box, no overlaps");
        CHECK(L.table.b - L.table.t == L0.table.b - L0.table.t, "(2.17.3) the box keeps its height"); }
    {   // (2.15.4, Rassul 2026-11-02 13:15 CT "put the time like 8:45, 8:30") the 1st-extreme heads as clock times from the market's open
        hl::View v = shotView(true); v.tmOpenMin = 510;   // ES 8:30
        hl::Layout L = hl::layoutAll(v, hl::Box{ 0, 0, 2400, 520 }, yOf, measure, 1900, 6, -1, nullptr, nullptr);
        const std::vector<hl::Item> tx = texts(L);
        CHECK(find(tx, "8:45") && find(tx, "9:00") && find(tx, "9:30") && find(tx, "9:30+") && find(tx, "1st extreme by") && !find(tx, "0-15m"), "(2.15.4) heads 8:45 / 9:00 / 9:30 / 9:30+ under '1st extreme by'");
        int out = 0; for (const hl::Item& it : tx) if (!inside(hl::Box{ it.l, it.t, it.r, it.b }, L.table)) ++out;
        CHECK(out == 0 && overlaps(tx) == 0, "(2.15.4) inside the box, no overlaps");
        hl::View g = shotView(true); g.tmOpenMin = 440;   // GC 7:20
        const std::vector<hl::Item> tg = texts(hl::layoutAll(g, hl::Box{ 0, 0, 2400, 520 }, yOf, measure, 1900, 6, -1, nullptr, nullptr));
        CHECK(find(tg, "7:35") && find(tg, "7:50") && find(tg, "8:20") && find(tg, "8:20+"), "(2.15.4) GC from its 7:20 open: 7:35 / 7:50 / 8:20 / 8:20+"); }
    {   // (2.15.3, Rassul 2026-11-02 10:25 CT "get rid of the above line") gamma levels on the view, still no strip above the box
        hl::View v = shotView(true);
        hl::View::GexItem a; a.strike = "7650"; a.name = "PS0"; a.above = true; v.gexItems.push_back(a);
        hl::View::GexItem m; m.strike = "7659.25"; m.mid = true; v.gexItems.push_back(m);
        hl::View::GexItem b; b.strike = "7625"; b.name = "G2"; b.above = false; v.gexItems.push_back(b);
        const hl::Layout L = hl::layoutAll(v, hl::Box{ 0, 0, 2400, 520 }, yOf, measure, 1900, 6, -1, nullptr, nullptr);
        bool none = true; for (const hl::Item& it : L.items) if (it.s == "PS0" || it.s == "G2" || it.s.find("7659.25") != std::string::npos) none = false;
        CHECK(L.tableOk && !L.gexOk && none && !hl::HL_SHOW_GEX_LADDER, "(2.15.3) no gamma strip above the box, even with gamma levels on hand");
    }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
