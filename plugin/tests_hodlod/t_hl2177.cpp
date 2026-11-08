// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2177.cpp -o t2177 && ./t2177   (from plugin\tests_hodlod)
// lsHodLod 2.17.7 (2026-11-03, Rassul "go" on hodlod_wide.png "now vs proposed") - the box runs to the PRICE AXIS, SDK-free:
//  [1] right edge == the axis (within 2 px) at several pane widths / positions / both font metrics; left, bottom and height unchanged
//  [2] the extra width spread over SESSION | HOD SWEEPS | LOD SWEEPS in proportion; sections butt up; LOD ends at the box edge
//  [3] the same texts (none lost / cut), every text inside the box, no two texts overlap, values right-aligned to their section edge
//  [4] a docked (moved) table still ends at the axis; no sweep sections (narrow pane) / axis left of the box = unchanged
//  [5] versions 2.17.7 everywhere; the host calls stretchToAxis after the dock and before the published rectangle
// lsHodLod 2.13.0 / 2.14.0 (2026-11-01, Rassul 18:52-20:38 CT mockups "HodLod box" v8-v14 + "go" / "ok") - the HOD / LOD sweep panels, SDK-free:
//  2.14.0: panel strips, column heads, % bars, the 1st-extreme time table, header extras (Day forecast / RTH % / bar time left)
//  [1] HodLodSweeps.h: the nightly file + the Dealer Profile key levels -> rows (ON3 range, wrong side of the open dimmed, "?" when no file / n < 20)
//  [2] layout: Session | HOD sweeps | LOD sweeps side by side, the session section unchanged, no overlaps, every text inside the box
//  [3] no room on the pane -> the 2.12.0 box (never the "pane too small" line); 9 positions x 3 widths
#include <cstdio>
#include <set>
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
#include <fstream>
#include <sstream>
#include <map>
#include "../HodLodSettings.h"
static std::multiset<std::string> strs(const std::vector<hl::Item>& tx) { std::multiset<std::string> m; for (auto& t : tx) m.insert(t.s); return m; }
static std::multiset<std::string> looks(const std::vector<hl::Item>& tx) { std::multiset<std::string> m; for (auto& t : tx) m.insert(t.s + "|" + std::to_string(t.fs) + "|" + std::to_string(t.c1) + "|" + (t.bold ? "b" : "")); return m; }
static void shift(hl::Layout& L, int dx, int dy) {   // as HodLod.cpp's dock / hysteresis move
    for (auto& it : L.items) if (it.role == hl::R_TABLE || it.role == hl::R_NOTE) { it.l += dx; it.r += dx; it.t += dy; it.b += dy; }
    L.table.l += dx; L.table.r += dx; L.table.t += dy; L.table.b += dy; L.sessR += dx;
    for (auto& b : L.swCols) { b.l += dx; b.r += dx; b.t += dy; b.b += dy; } }
static void runOne(const hl::Measure& M, int W, int pos, int dockDx, int dockDy, const char* tag) {
    auto yOf = [](long long) { return 300; };
    const hl::View v = shotView(true); v.tmOk ? (void)0 : (void)0;
    hl::View vv = v; vv.tmOpenMin = 510;
    const hl::Box pane = { 0, 0, W, 520 }; const int pe = (int)(W * 0.55), axisR = W - 2;
    hl::Opts O; hl::Layout L = hl::layoutAll(vv, pane, yOf, M, pe, pos, -1, nullptr, &O);
    std::string id = std::string(tag) + " W" + std::to_string(W) + " pos" + std::to_string(pos) + (dockDx || dockDy ? " docked" : "");
    if (!L.tableOk || !L.swOk) { CHECK(false, id + ": pass 1 has the 3 sections"); return; }
    if (dockDx || dockDy) { const int dx = std::max(-L.table.l + 4, dockDx), dy = std::min(pane.b - 1 - L.table.b, dockDy); shift(L, dx, dy); }
    const hl::Layout B = L; const std::vector<hl::Item> tx0 = texts(B);
    const int w0 = B.table.r - B.table.l, sess0 = B.swCols[0].l - B.table.l, hod0 = B.swCols[0].r - B.swCols[0].l, lod0 = B.swCols[1].r - B.swCols[1].l;
    const bool st = hl::stretchToAxis(L, vv, pane, yOf, M, pe, pos, -1, nullptr, O, axisR);
    if (B.table.r >= axisR - 3) { CHECK(!st && L.table.r == B.table.r, id + ": already at the axis - unchanged"); return; }
    CHECK(st && L.stretched && std::abs(L.table.r - axisR) <= 2, id + ": right edge " + std::to_string(L.table.r) + " == axis " + std::to_string(axisR) + " (+-2)");
    CHECK(L.table.l == B.table.l && L.table.b == B.table.b && L.table.t == B.table.t, id + ": left / top / bottom unchanged (the dock holds)");
    const int w1 = L.table.r - L.table.l, sess1 = L.swCols[0].l - L.table.l, hod1 = L.swCols[0].r - L.swCols[0].l, lod1 = L.swCols[1].r - L.swCols[1].l;
    const double f = (double)w1 / w0;
    CHECK(L.swCols.size() == 2 && L.swCols[0].l == L.sessR && L.swCols[1].l == L.swCols[0].r && L.swCols[1].r == L.table.r, id + ": sections butt up, LOD ends at the box edge");
    CHECK(std::fabs(sess1 / (double)sess0 - f) < 0.03 && std::fabs(hod1 / (double)hod0 - f) < 0.03 && std::fabs(lod1 / (double)lod0 - f) < 0.03,
          id + ": proportional (x" + std::to_string(f).substr(0, 4) + ": " + std::to_string(sess0) + "->" + std::to_string(sess1) + ", " + std::to_string(hod0) + "->" + std::to_string(hod1) + ", " + std::to_string(lod0) + "->" + std::to_string(lod1) + ")");
    const std::vector<hl::Item> tx = texts(L);
    CHECK(strs(tx) == strs(tx0), id + ": the same texts as before (none lost / cut)");
    int out = 0; for (const hl::Item& it : tx) if (!inside(hl::Box{ it.l, it.t, it.r, it.b }, L.table)) ++out;
    CHECK(out == 0, id + ": every text inside the box"); CHECK(overlaps(tx) == 0, id + ": no overlaps");
    CHECK(looks(tx) == looks(tx0), id + ": same font sizes / colours / weights");
    {   const hl::Item* p46 = find(tx, "46%"); const hl::Item* p42 = nullptr; for (const hl::Item& it : tx) if (it.s == "41%" && it.l > L.swCols[1].l) p42 = &it;
        CHECK(p46 && std::abs(p46->r - (L.swCols[0].r - 9)) <= 1 && p42 && std::abs(p42->r - (L.swCols[1].r - 9)) <= 1, id + ": the % right-aligned to each section's right edge");
        const hl::Item* ses = find(tx, "SESSION"); const hl::Item* exp = find(tx, "Exp"); const hl::Item* lv = find(tx, "Level");
        CHECK(ses && exp && lv && ses->l < L.table.l + 20 && exp->l < L.table.l + 12 && lv->l < L.swCols[0].l + 12, id + ": labels stay left");
        const hl::Item* t930 = find(tx, "9:30+"); CHECK(t930 && t930->r > L.sessR - 40 && t930->r <= L.sessR, id + ": the 1st-extreme times spread across the session section");
        const hl::Item* ver = nullptr; for (const hl::Item& it : tx) if (it.s == std::string("v") + hl::HL_VERSION) ver = &it;
        CHECK(ver && std::abs(ver->r - (L.table.r - 10)) <= 1, id + ": the version at the box's right end"); }
    {   int roleBad = 0; for (const hl::Item& it : L.items) if ((it.role == hl::R_TABLE) && (it.r > L.table.r + 1 || it.l < L.table.l - 1)) ++roleBad;
        CHECK(roleBad == 0, id + ": no table item (fills / lines / bars) past the box"); }
}
int main() {
    auto yOf = [](long long) { return 300; };
    std::printf("[1-3] widths x positions x metrics\n");
    for (int W : { 2400, 1900, 1530, 1200 }) for (int pos : { 6, 7, 0 }) { runOne(measure, W, pos, 0, 0, "approx"); runOne(measureLib, W, pos, 0, 0, "arial"); }
    std::printf("[4] docked + no sections\n");
    for (int W : { 2400, 1530 }) { runOne(measureLib, W, 6, 120, 0, "arial"); runOne(measureLib, W, 0, 60, 200, "arial"); }
    {   const hl::View v = shotView(false); hl::Opts O; const hl::Box pane = { 0, 0, 1530, 520 };
        hl::Layout L = hl::layoutAll(v, pane, yOf, measure, 900, 6, -1, nullptr, &O); const hl::Box b = L.table;
        CHECK(L.tableOk && !L.swOk && !hl::stretchToAxis(L, v, pane, yOf, measure, 900, 6, -1, nullptr, O, 1528) && L.table.r == b.r, "no sweep sections (the 2.12.0 box): unchanged"); }
    {   const hl::View v = shotView(true); hl::Opts O; const hl::Box pane = { 0, 0, 2400, 520 };
        hl::Layout L = hl::layoutAll(v, pane, yOf, measure, 1900, 6, -1, nullptr, &O); const hl::Box b = L.table;
        CHECK(!hl::stretchToAxis(L, v, pane, yOf, measure, 1900, 6, -1, nullptr, O, b.r - 50) && L.table.r == b.r, "axis left of the box: unchanged");
        CHECK(!hl::stretchToAxis(L, v, pane, yOf, measure, 1900, 6, -1, nullptr, O, 0) && L.table.r == b.r, "no axis: unchanged"); }
    std::printf("[5] versions + host order\n");
    {   CHECK(std::string(hl::HL_VERSION) == "2.21.0" && std::string(hlset::PLUGIN_VERSION) == hl::HL_VERSION, "HL_VERSION == PLUGIN_VERSION == 2.17.7");
        std::ifstream f("../HodLod.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string src = ss.str();
        CHECK(src.find(std::string("p->setVersion(\"") + hl::HL_VERSION + "\")") != std::string::npos, "setVersion literal 2.17.7");
        const size_t dk = src.find("S->placeLine += S->dockLine"), sa = src.find("hl::stretchToAxis("), pub = src.find("bdk::publishHodLod(sym"), ex = src.find("    execute(L);");
        CHECK(dk != std::string::npos && sa != std::string::npos && pub != std::string::npos && dk < sa && sa < ex && ex < pub, "host: stretch after the dock, before drawing and the published rectangle");
        CHECK(src.find("sc.getScaleRect()") != std::string::npos, "host: the axis from the scale rectangle"); }
    std::printf("t_hl2177 (lsHodLod %s): %d passed, %d failed\n", hl::HL_VERSION, passes, fails);
    return fails ? 1 : 0;
}
