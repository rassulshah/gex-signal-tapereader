// t_hl2480.cpp -- lsHodLod 2.48.0 (Rassul 2026-11-26 08:10-08:31 CT, TopBox-mockup.html "ok build"): the HOD / LOD box (hsb::layout) docked on the
// pane's bottom edge, centred on the candles, Exp / Act x 1st / 2nd + "HOD / LOD Stats" (1st extreme by / % of days, always shown), and the Level line
// sitting right on top of the box (its bottom edge = the box's top edge), centred the same way.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2480.cpp -o t && ./t
#include "../NowLevels.h"
#include "../HodLodLogic.h"
#include "../HodLodSettings.h"
#include <cstdio>
#include <string>
static int ok = 0, f = 0;
static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static bool has(const std::string& s, const std::string& x) { return s.find(x) != std::string::npos; }
int main() {
    C(std::string(hl::HL_VERSION) == "2.48.0" && std::string(hlset::PLUGIN_VERSION) == "2.48.0", "versions 2.48.0");
    auto meas = [](const std::string& s, bool bold) { return (int)s.size() * (bold ? 7 : 6); };
    // GC at 8:01 AM on 11-26 (his status file): Exp 1st 7:24 - 8:12 70%, 2nd likely 8:00-9:15 LOD; Act HOD so far 7:33, LOD so far 8:00 ONL
    hl::View v;
    v.E[hl::K_1ST] = hl::mkCell("7:24 - 8:12  70%", hl::C_SLATE);
    v.E[hl::K_2ND] = hl::mkCell("likely 8:00-9:15", hl::C_SLATE); v.E[hl::K_2ND].s2 = "LOD"; v.E[hl::K_2ND].col2 = hl::C_GREEN;
    v.A[hl::K_1ST] = hl::mkCell("HOD so far 7:33", hl::C_GREY);
    v.A[hl::K_2ND] = hl::mkCell("LOD so far 8:00", hl::C_GREY); v.A[hl::K_2ND].lv.push_back(hl::LvSeg{ "ONL", hl::C_GREY });
    v.tmOk = true; v.tmOpenMin = 7 * 60 + 20; v.tmCum[0] = 34; v.tmCum[1] = 45; v.tmCum[2] = 63; v.tmCum[3] = 37;
    const int paneL = 0, lastBar = 641, bottom = 446, cx = (paneL + lastBar) / 2;
    const hsb::Draw B = hsb::layout(v, meas, cx, bottom, paneL, lastBar);
    C(B.ok, "laid out: " + B.why);
    C(B.b == bottom, "docked on the pane's bottom edge (no black space under it)");
    C(std::abs((B.l + B.r) / 2 - cx) <= 1, "centred on the candles");
    C(B.l >= paneL && B.r <= lastBar, "inside the candle area (never right of the last bar)");
    C(B.b - B.t == 3 * 15 + 4, "three rows");
    for (const hsb::Item& it : B.items) C(it.l >= B.l && it.r <= B.r && it.t >= B.t && it.b <= B.b, "every item inside the box: " + it.s);
    C(has(B.text, "Exp 7:24 - 8:12  70% | likely 8:00-9:15 LOD"), "Exp row: " + B.text);
    C(has(B.text, "Act HOD so far 7:33 | LOD so far 8:00 ONL"), "Act row: " + B.text);
    C(has(B.text, "7:35=34% 7:50=45% 8:20=63% 8:20+=37%"), "stats from GC's open (7:20): " + B.text);
    int heads = 0, labs = 0, seps = 0; bool exp = false, act = false;
    for (const hsb::Item& it : B.items) { if (it.k == 1 && (it.s == "1st" || it.s == "2nd" || it.s == "HOD / LOD Stats")) ++heads;
        if (it.k == 1 && (it.s == "1st extreme by" || it.s == "% of days")) ++labs;
        if (it.k == 2) ++seps;
        if (it.k == 1 && it.s == "Exp" && it.bold) exp = true;
        if (it.k == 1 && it.s == "Act" && it.bold) act = true; }
    C(heads == 3 && labs == 2 && seps == 2 && exp && act, "headings 1st / 2nd / HOD / LOD Stats, the two stat labels, two grey separators, Exp / Act bold");
    // the stats rows line up with Exp / Act
    int yExp = -1, yBy = -1, yAct = -1, yPct = -1;
    for (const hsb::Item& it : B.items) { if (it.s == "Exp") yExp = it.t; if (it.s == "1st extreme by") yBy = it.t; if (it.s == "Act") yAct = it.t; if (it.s == "% of days") yPct = it.t; }
    C(yExp == yBy && yAct == yPct && yExp < yAct, "1st extreme by on the Exp row, % of days on the Act row");
    // no data yet (before the open) - the box still shows, stats always
    hl::View e; e.tmOk = true; e.tmOpenMin = 510; e.tmCum[0] = 46; e.tmCum[1] = 61; e.tmCum[2] = 77; e.tmCum[3] = 23;
    const hsb::Draw B2 = hsb::layout(e, meas, 400, 500, 0, 835);
    C(B2.ok && has(B2.text, "Exp - | - ; Act - | -") && has(B2.text, "8:45=46% 9:00=61% 9:30=77% 9:30+=23%"), "no read yet: dashes, ES stats: " + B2.text);
    // stats file missing: "?"
    hl::View q; const hsb::Draw B3 = hsb::layout(q, meas, 400, 500, 0, 835); C(B3.ok && has(B3.text, "15m=?"), "no stats file: ? marks: " + B3.text);
    // too narrow: not drawn, with the reason
    const hsb::Draw B4 = hsb::layout(v, meas, 100, 500, 0, 200); C(!B4.ok && has(B4.why, "no room"), "narrow pane: " + B4.why);
    // near the left edge: shifted right to fit, never off the pane
    const hsb::Draw B5 = hsb::layout(v, meas, 10, 500, 0, 835); C(B5.ok && B5.l == 0, "pushed inside on the left");
    // the Level line sits right on top of the box, centred the same way
    nowl::Out o; o.ok = true; o.dn.ok = true; o.dn.name = "PDL"; o.up.ok = true; o.up.name = "LHI"; o.haveBk = true; o.bk.lv = "R1"; o.bk.dir = 1;
    const nowl::Draw D = nowl::layout(nowl::segments(o), meas, cx, B.t, paneL, lastBar, 10);
    C(D.ok && D.b == B.t, "the Level line's bottom edge ON the box's top edge (no gap): " + std::to_string(D.b) + " vs " + std::to_string(B.t));
    C(std::abs((D.l + D.r) / 2 - cx) <= 1, "the Level line centred on the candles");
    std::printf("t_hl2480 (lsHodLod %s): %d passed, %d failed\n", hl::HL_VERSION, ok, f);
    return f ? 1 : 0;
}
