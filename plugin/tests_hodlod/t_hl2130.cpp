// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2130.cpp -o t2130 && ./t2130   (from plugin\tests_hodlod)
// lsHodLod 2.13.0 (2026-11-01, Rassul 18:52-19:06 CT mockup "HodLod box: 3 sections" + "go") - the HOD / LOD sweep sections, SDK-free:
//  [1] HodLodSweeps.h: the nightly file + the Dealer Profile key levels -> rows (ON3 range, wrong side of the open dimmed, "?" when no file / n < 20)
//  [2] layout: Session | HOD sweeps | LOD sweeps side by side, the session section unchanged, no overlaps, every text inside the box
//  [3] no room on the pane -> the 2.12.0 box (never the "pane too small" line); 9 positions x 3 widths
#include <cstdio>
#include <cmath>
#include <algorithm>
#include "HodLodLogic.h"
#include "HodLodSweeps.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static int measure(const std::string& s, int fs, bool bold) { return (int)(s.size() * fs * (bold ? 0.62 : 0.56) + 0.999); }
static bool inside(const hl::Box& a, const hl::Box& o) { return a.l >= o.l && a.r <= o.r && a.t >= o.t && a.b <= o.b; }
static std::vector<hl::Item> texts(const hl::Layout& L) { std::vector<hl::Item> t; for (const hl::Item& it : L.items) if (it.k == hl::K_TEXT && !it.s.empty()) t.push_back(it); return t; }
static int overlaps(const std::vector<hl::Item>& tx) { int n = 0; for (size_t i = 0; i < tx.size(); i++) for (size_t j = i + 1; j < tx.size(); j++) { const hl::Item& a = tx[i]; const hl::Item& b = tx[j];
    if (a.t < b.b - 1 && b.t < a.b - 1 && a.l < b.r - 1 && b.l < a.r - 1) { if (++n < 4) std::printf("  overlap '%s' / '%s'\n", a.s.c_str(), b.s.c_str()); } } return n; }
static const hl::Item* find(const std::vector<hl::Item>& tx, const std::string& s) { for (const hl::Item& it : tx) if (it.s == s) return &it; return nullptr; }
static const char* FILE_ES = "VERSION,1\nMARKET,ES\nROW,HI,PHH,46,54,117\nROW,HI,ON3,41,48,118\nROW,HI,DPoc,26,15,58\nROW,HI,VAH,22,18,82\nROW,HI,PDH,16,15,94\nROW,HI,PFH,14,14,12\n"
                             "ROW,LO,PHL,41,53,130\nROW,LO,ON3,38,50,130\nROW,LO,DPoc,28,19,69\nROW,LO,VAL,11,10,88\nROW,LO,PDL,9,9,102\nROW,LO,PFL,6,6,106\nTIME,43,15,17,25,248\n";
static const char* KEYS_ES = "KEYLV,1,1793500000\nL|ONH|7713.00|0\nL|AsiaHI/PFL|7710.00|0\nL|LonHI|7693.75|0\nL|PHH|7674.50|0\nL|Open|7669.50|0\nL|AsiaLO|7665.25|0\nL|PHL|7664.25|0\n"
                             "L|ONL/LonLO|7664.00|0\nL|DPoc|7751.25|0\nL|VAH|7757.50|0\nL|VAL|7738.25|0\nL|PDH|7767.25|0\nL|PDL|7725.25|0\nL|PFH|7773.75|0\nL|HrHI|7800.00|1\n";
static hl::View shotView(bool sw) {   // his ES box (Exp / Act / Feed) + the Oct 30 sweep rows
    hl::View v; v.has = true; v.prior = false;
    v.headLeftSegs.push_back(hl::Seg{ "ES RTH 8:30-3:00", hl::C_SLATE }); v.trendReady = true; v.trendLvl = -1;
    v.E[hl::K_1ST] = hl::mkCell("8:33 - 9:21", hl::C_BLUE); v.headTail = "LOD after 9:30";
    v.A[hl::K_1ST] = hl::mkCell("8:33", hl::C_SLATE); v.A[hl::K_1ST].s2 = "HOD"; v.A[hl::K_1ST].col2 = hl::C_GREEN;
    v.A[hl::K_2ND] = hl::mkCell("1:54", hl::C_SLATE); v.A[hl::K_2ND].s2 = "LOD"; v.A[hl::K_2ND].col2 = hl::C_RED;
    hl::View::Note n; n.label = "1st"; n.text = "0-15m 43% \xB7 15-30m 15% \xB7 30-60m 17% \xB7 60m+ 25%"; n.col = hl::C_SLATE; v.notes.push_back(n);
    if (sw) {
        std::vector<hsw::Lv> L; hsw::parseKeys(KEYS_ES, 1793500100, 80 * 3600, L);
        std::vector<hsw::Row> hi, lo; hsw::build(hsw::parse(FILE_ES, "ES"), L, 7669.50, 2, hi, lo);
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
        std::vector<hsw::Lv> none; hsw::build(F, none, 7669.5, 2, h2, l2); CHECK(h2[0].px == "-" && h2[0].pct == "46%", "no key levels file: '-' prices, % kept");
    }
    std::printf("[2] layout\n");
    {   const hl::View v = shotView(true);
        hl::Layout L = hl::layoutAll(v, hl::Box{ 0, 0, 2400, 520 }, yOf, measure, 1900, 6, -1, nullptr, nullptr);
        CHECK(L.tableOk && L.swOk && L.swCols.size() == 2, "table with 2 sweep sections");
        const std::vector<hl::Item> tx = texts(L);
        const hl::Item* ex = find(tx, "Exp"); const hl::Item* th = find(tx, "HOD sweeps (1st = HOD)"); const hl::Item* tl = find(tx, "LOD sweeps (1st = LOD)");
        CHECK(ex && th && tl && ex->r < th->l && th->r < tl->l, "order left to right: Session | HOD | LOD");
        CHECK(L.swCols.size() == 2 && L.swCols[0].l > ex->r && L.swCols[1].l == L.swCols[0].r && L.swCols[1].r == L.table.r, "sections butt up, the LOD one ends at the box's right edge");
        const hl::Item* f = find(tx, "1st"); CHECK(f && f->r < L.swCols[0].l, "1st time row stays in the session section");
        for (const hl::Item& it : tx) if (it.s.find("0-15m") != std::string::npos) CHECK(it.r <= L.swCols[0].l, "the time text never runs into the HOD section");
        CHECK(!find(tx, "Feed"), "no Feed row");
        CHECK(find(tx, "7,674.50") && find(tx, "46%") && find(tx, "above open") && find(tx, "above open")->c1 == hl::C_DIM, "prices, % and a dimmed 'above open'");
        int out = 0; for (const hl::Item& it : tx) if (!inside(hl::Box{ it.l, it.t, it.r, it.b }, L.table) && !(L.gexOk && inside(hl::Box{ it.l, it.t, it.r, it.b }, L.gexRow))) ++out;
        CHECK(out == 0, "every text inside the box"); CHECK(overlaps(tx) == 0, "no overlaps");
        const int rowH = hl::lineH(ex->fs) + 1; CHECK(L.table.b - L.table.t == 1 + rowH + 4 + 1 + 7 * rowH + 3, "height = header band + title + 6 rows");
        hl::Layout L0 = hl::layoutAll(shotView(false), hl::Box{ 0, 0, 2400, 520 }, yOf, measure, 1900, 6, -1, nullptr, nullptr);
        CHECK(!L0.swOk && L0.table.r - L0.table.l == L.swCols[0].l - L.table.l, "the session section = the 2.12.0 box exactly (width " + std::to_string(L0.table.r - L0.table.l) + ")");
        const std::vector<hl::Item> tx0 = texts(L0); const hl::Item* ex0 = find(tx0, "Exp"); CHECK(ex0 && ex0->l - L0.table.l == ex->l - L.table.l, "session text at the same place");
        std::printf("  box %d x %d px\n", L.table.r - L.table.l, L.table.b - L.table.t);
    }
    std::printf("[3] fuzz + no room\n");
    {   int bad = 0, runs = 0, withSw = 0;
        for (int W : { 2400, 1530, 900 }) for (int pos = 0; pos < 9; pos++) {
            const hl::View v = shotView(true); hl::Layout L = hl::layoutAll(v, hl::Box{ 0, 0, W, 520 }, yOf, measure, W - 300, pos, -1, nullptr, nullptr);
            if (!L.tableOk) { ++bad; continue; } ++runs; if (L.swOk) ++withSw;
            const std::vector<hl::Item> tx = texts(L); if (overlaps(tx)) { ++bad; continue; }
            for (const hl::Item& it : tx) if (!inside(hl::Box{ it.l, it.t, it.r, it.b }, L.table) && !(L.gexOk && inside(hl::Box{ it.l, it.t, it.r, it.b }, L.gexRow))) { ++bad; break; }
            if (W == 900 && L.swOk) ++bad;   // no room: the 2.12.0 box
        }
        CHECK(bad == 0 && runs == 27 && withSw >= 9, std::to_string(runs) + " layouts clean, " + std::to_string(withSw) + " with sweeps (" + std::to_string(bad) + " bad)");
    }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
