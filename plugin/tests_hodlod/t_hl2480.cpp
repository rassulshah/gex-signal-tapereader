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
    C(std::string(hl::HL_VERSION) >= std::string("2.48.0") && std::string(hlset::PLUGIN_VERSION) == hl::HL_VERSION, "versions 2.48.x");
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
    C(B.b - B.t == 3 * 18 + 4, "three rows (18 px, 2.48.1)");
    for (const hsb::Item& it : B.items) C(it.l >= B.l && it.r <= B.r && it.t >= B.t && it.b <= B.b, "every item inside the box: " + it.s);
    C(has(B.text, "Exp 7:24 - 8:12  70% | likely 8:00-9:15 LOD"), "Exp row: " + B.text);
    C(has(B.text, "Act HOD so far 7:33 | LOD so far 8:00 ONL"), "Act row: " + B.text);
    C(has(B.text, "7:35=34% 7:50=45% 8:20=63% 8:20+=37%"), "stats from GC's open (7:20): " + B.text);
    int heads = 0, labs = 0, seps = 0; bool exp = false, act = false;
    for (const hsb::Item& it : B.items) { if (it.k == 1 && (it.s == "1st" || it.s == "2nd" || it.s == "HOD / LOD Stats")) ++heads;
        if (it.k == 1 && it.s == "1st") ++labs;   /* (2.50.0) the stats row label "1st" (the 2nd row is empty without SEC2) */
        if (it.k == 2) ++seps;
        if (it.k == 1 && it.s == "Exp" && it.bold) exp = true;
        if (it.k == 1 && it.s == "Act" && it.bold) act = true; }
    C(heads == 4 && labs == 2 && seps == 2 && exp && act, "(2.50.0) headings 1st / 2nd / HOD / LOD Stats + the stats row label 1st, two grey separators, Exp / Act bold");
    // the stats rows line up with Exp / Act
    int yExp = -1, yBy = -1, yAct = -1, yPct = -1;
    for (const hsb::Item& it : B.items) { if (it.s == "Exp") yExp = it.t; if (it.s == "1st" && it.t != B.t + 2) yBy = it.t; if (it.s == "Act") yAct = it.t; if (it.s == "34%") yPct = it.t; }
    C(yExp == yBy && yExp == yPct && yExp < yAct, "(2.50.0) the stats 1st row (label and %) on the Exp row");
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

    // (2.48.1) "2nd - 3h 44m", Exp 2nd = Exp 1st + the gap, Act "8:48 - LOD ONL" - ES 11-25 (his status file): 1st 8:33 - 9:12, gap 3h44, LOD 8:48 ONL, HOD 2:57 LonHI
    {   hl::View s; s.E[hl::K_1ST] = hl::mkCell("8:33 - 9:12  62%", hl::C_SLATE); s.win1a = 8 * 60 + 33; s.win1b = 9 * 60 + 12; s.gapMed = 224;
        s.E[hl::K_2ND] = hl::mkCell("late-day", hl::C_GREY); s.E[hl::K_2ND].s3 = "HOD";
        s.tmOk = true; s.tmOpenMin = 510; s.tmCum[0] = 46; s.tmCum[1] = 61; s.tmCum[2] = 77; s.tmCum[3] = 23;
        hsb::Draw X = hsb::layout(s, meas, 400, 500, 0, 835);
        C(X.ok && has(X.text, "2nd - 3h 44m"), "heading 2nd - 3h 44m: " + X.text);
        C(has(X.text, "| 12:17 - 12:56 HOD ;"), "before the 1st: Exp 2nd = 8:33 - 9:12 + 3h44 = 12:17 - 12:56 HOD: " + X.text);
        C(!has(X.text, "~") && !has(X.text, "late-day"), "no ~, no late-day");
        bool red = false; for (const hsb::Item& it : X.items) if (it.s == " HOD" && it.col == hl::C_RED) red = true; C(red, "HOD word red");
        // the 1st called in at 8:48 (LOD first)
        s.side = 1; s.lodMod = 8 * 60 + 48; s.hodMod = 14 * 60 + 57; s.firstFinal = true;
        s.A[hl::K_1ST] = hl::mkCell("8:48", hl::C_SLATE); s.A[hl::K_1ST].s2 = "LOD"; s.A[hl::K_1ST].col2 = hl::C_GREEN; s.A[hl::K_1ST].s3 = "in"; s.A[hl::K_1ST].col3 = hl::C_SLATE;
        s.A[hl::K_1ST].lv.push_back(hl::LvSeg{ "ONL", hl::C_SLATE });
        s.A[hl::K_2ND] = hl::mkCell("2:57", hl::C_SLATE); s.A[hl::K_2ND].s2 = "HOD"; s.A[hl::K_2ND].col2 = hl::C_RED; s.A[hl::K_2ND].lv.push_back(hl::LvSeg{ "LonHI", hl::C_SLATE });
        X = hsb::layout(s, meas, 400, 500, 0, 835);
        C(has(X.text, "| 12:32 HOD ;"), "after the 1st: 8:48 + 3h44 = 12:32 HOD: " + X.text);
        C(has(X.text, "Act 8:48 - LOD ONL | 2:57 - HOD LonHI"), "Act 8:48 - LOD ONL: " + X.text);
        C(!has(X.text, " in"), "no in tag");
        // GC: 2h33 gap -> "2nd - 2h 33m"; no gap known -> the old cell, plain "2nd"
        s.gapMed = 153; X = hsb::layout(s, meas, 400, 500, 0, 835); C(has(X.text, "2nd - 2h 33m"), "2nd - 2h 33m");
        s.gapMed = 45; X = hsb::layout(s, meas, 400, 500, 0, 835); C(has(X.text, "2nd - 45m ;"), "under an hour: 45m");
        s.gapMed = NAN; X = hsb::layout(s, meas, 400, 500, 0, 835); C(has(X.text, "2nd ; Exp") && has(X.text, "late-day  HOD"), "no gap: the old Exp 2nd cell: " + X.text);
        // the 1st / 2nd columns have 14 px of room each side: the separators sit 14 px from the text
        s.gapMed = 224; X = hsb::layout(s, meas, 400, 500, 0, 835);
        int sep1 = -1; int maxR = -1; for (const hsb::Item& it : X.items) { if (it.k == 2 && sep1 < 0) sep1 = it.l; }
        for (const hsb::Item& it : X.items) if (it.k == 1 && it.r <= sep1) maxR = (std::max)(maxR, it.r);
        C(sep1 - maxR >= 14, "14 px between the 1st column and its separator");
    }
    // the Level line sits right on top of the box, centred the same way
    nowl::Out o; o.ok = true; o.dn.ok = true; o.dn.name = "PDL"; o.up.ok = true; o.up.name = "LHI"; o.haveBk = true; o.bk.lv = "R1"; o.bk.dir = 1;
    const nowl::Draw D = nowl::layout(nowl::segments(o), meas, cx, B.t, paneL, lastBar, 10);
    C(D.ok && D.b == B.t, "the Level line's bottom edge ON the box's top edge (no gap): " + std::to_string(D.b) + " vs " + std::to_string(B.t));
    C(std::abs((D.l + D.r) / 2 - cx) <= 1, "the Level line centred on the candles");
    std::printf("t_hl2480 (lsHodLod %s): %d passed, %d failed\n", hl::HL_VERSION, ok, f);
    return f ? 1 : 0;
}
