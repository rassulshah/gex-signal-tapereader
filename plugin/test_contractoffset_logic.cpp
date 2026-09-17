/********************************************************************************
 *  test_contractoffset_logic.cpp — Gate B (logic) for the CONTRACT OFFSET ANCHOR shared by lsGammaProfile and
 *  lsKingTracker (plugin/ContractOffsetLogic.h). No IRT SDK.
 *      cloud   : g++ -std=c++14 -o /tmp/col plugin/test_contractoffset_logic.cpp && /tmp/col
 *      windows : plugin\run-logic-tests.bat
 *  Every case is a day the rule got wrong before it lived here (2026-09-16), plus the days it must keep right.
 ********************************************************************************/
#include "ContractOffsetLogic.h"
#include <cstdio>
#include <vector>

static int fails = 0, passes = 0;
#define CHECK(cond, msg) do { if (cond) { passes++; printf("  ok    %s\n", msg); } else { fails++; printf("  FAIL  %s   (line %d)\n", msg, __LINE__); } } while (0)

// a 3-minute END-stamped chart of 2026-09-16: the evening of the 15th (17:03 .. 23:57), the night, RTH 08:33 .. 15:00,
// the evening 17:03 .. 21:00. Closes encode the time so a test can read which bar was chosen.
static std::vector<col::Bar> chart(bool withEvening, bool withPrevDay = true)
{
    std::vector<col::Bar> v;
    auto add = [&](int y, int m, int d, int sod, float close) { col::Bar b; b.y = y; b.m = m; b.d = d; b.sod = sod; b.close = close; v.push_back(b); };
    if (withPrevDay) {
        for (int s = 8 * 3600 + 33 * 60; s <= 15 * 3600; s += 180) add(2026, 9, 15, s, 7600.0f + s / 3600.0f);   // the 15th's RTH: 7608.5 .. 7615
        for (int s = 17 * 3600 + 180; s < 24 * 3600; s += 180) add(2026, 9, 15, s, 7620.0f);                    // the 15th's evening
    }
    for (int s = 180; s < 8 * 3600 + 30 * 60; s += 180) add(2026, 9, 16, s, 7630.0f);                              // the night into the 16th
    for (int s = 8 * 3600 + 33 * 60; s <= 15 * 3600; s += 180) add(2026, 9, 16, s, 7000.0f + s / 100.0f);          // RTH: the 14:24 bar -> 7518.4, 15:00 -> 7540
    if (withEvening) for (int s = 17 * 3600 + 180; s <= 21 * 3600; s += 180) add(2026, 9, 16, s, 7660.0f + (s - 17 * 3600) / 1000.0f);   // evening, rising
    return v;
}
static float closeAt(const std::vector<col::Bar>& v, int idx) { return idx >= 0 ? v[(size_t)idx].close : -1.0f; }

int main()
{
    printf("test_contractoffset_logic — the anchor bar\n");
    const double S1426 = 14 * 3600 + 26 * 60, S1424 = 14 * 3600 + 24 * 60, S1500 = 15 * 3600, S1945 = 19 * 3600 + 45 * 60, S0930 = 9 * 3600 + 30 * 60;
    {
        std::vector<col::Bar> v = chart(true);
        // 1. inside RTH, the quote's minute is the ASOF minute: the same bar either way
        int a = col::anchorIndex(&v[0], (int)v.size(), S0930, S0930, 2026, 9, 16);
        CHECK(a >= 0 && v[a].d == 16 && v[a].sod == S0930, "RTH, timed SCALEREF at 09:30 -> the 09:30 bar");
        int b = col::anchorIndex(&v[0], (int)v.size(), S0930, -1, 0, 0, 0);
        CHECK(b == a, "RTH, no time on the row -> ASOF's bar (the 0.48 rule agrees inside RTH)");
        // 2. THE FOMC EVENING: quote frozen at 14:26, export at 19:45, chart at 7663 — the 14:26 bar, nothing else
        int c = col::anchorIndex(&v[0], (int)v.size(), S1945, S1426, 2026, 9, 16);
        CHECK(c >= 0 && v[c].d == 16 && v[c].sod == S1424 && closeAt(v, c) > 7518.0f && closeAt(v, c) < 7519.0f, "evening, SCALEREF stamped 14:26 -> the 14:24 bar (last at or before; 7518.4), not 15:00, not the live 7663");
        // 3. an untimed row after hours: the 15:00 bar (0.48), never the evening bar (0.46)
        int d = col::anchorIndex(&v[0], (int)v.size(), S1945, -1, 0, 0, 0);
        CHECK(d >= 0 && v[d].sod == S1500 && closeAt(v, d) == 7540.0f, "evening, untimed SCALEREF -> the last RTH bar (15:00, 7540)");
        // 4. the quote's minute falls between bars: the last bar at or before it
        int e = col::anchorIndex(&v[0], (int)v.size(), S1945, S1426 - 60, 2026, 9, 16);
        CHECK(e == c, "a quote at 14:25 (between bars) anchors on the 14:24 bar (last at or before)");
        int e2 = col::anchorIndex(&v[0], (int)v.size(), S1945, S1426 + 60, 2026, 9, 16);
        CHECK(e2 >= 0 && v[e2].sod == S1426 + 60, "a quote exactly on a bar's stamp (14:27) takes that bar");
    }
    {
        // 5. PRE-OPEN on the 16th, quote frozen at the 15th's close (untimed): the 15th's 15:00 bar, not a night bar
        std::vector<col::Bar> v = chart(false);
        // trim to the pre-open (drop the 16th's RTH)
        while (!v.empty() && v.back().d == 16 && v.back().sod >= 8 * 3600 + 33 * 60) v.pop_back();
        int a = col::anchorIndex(&v[0], (int)v.size(), 7 * 3600, -1, 0, 0, 0);
        CHECK(a >= 0 && v[a].d == 15 && v[a].sod == S1500, "pre-open, untimed -> the previous session's 15:00 bar");
        // 6. pre-open with a timed quote from the 15th at 14:26
        int b = col::anchorIndex(&v[0], (int)v.size(), 7 * 3600, S1426, 2026, 9, 15);
        CHECK(b >= 0 && v[b].d == 15 && v[b].sod == S1424, "pre-open, quote stamped 15th 14:26 -> the 14:24 bar on the 15th");
    }
    {
        // 7. the quote's date is not on the chart (a CSV from another day): fall through to the RTH rule, never crash
        std::vector<col::Bar> v = chart(true, false);
        int a = col::anchorIndex(&v[0], (int)v.size(), S1945, S1426, 2026, 9, 10);
        CHECK(a >= 0 && v[a].sod == S1500, "quote dated 09-10, chart starts 09-16 -> the RTH rule's 15:00 bar");
        // 8. nothing to anchor on: no ASOF, no time -> -1 (live close)
        CHECK(col::anchorIndex(&v[0], (int)v.size(), -1, -1, 0, 0, 0) == -1, "no ASOF, no quote time -> live close (-1)");
        CHECK(col::anchorIndex(&v[0], 0, S1945, -1, 0, 0, 0) == -1, "no bars -> -1");
    }
    {
        // 9. a chart with ONLY RTH bars (no evening session): after hours the last RTH bar is the last bar
        std::vector<col::Bar> v;
        for (int s = 8 * 3600 + 33 * 60; s <= 15 * 3600; s += 180) { col::Bar b; b.y = 2026; b.m = 9; b.d = 16; b.sod = s; b.close = 7500.0f + s / 100.0f; v.push_back(b); }
        int a = col::anchorIndex(&v[0], (int)v.size(), S1945, -1, 0, 0, 0);
        CHECK(a == (int)v.size() - 1, "RTH-only chart after hours -> its last bar");
    }
    {
        // 10. the offset and its clamp
        float off = 0;
        CHECK(col::offsetFor(7625.0f, 7622.0f, off) && off > 2.9f && off < 3.1f, "offset = anchor close - SCALEREF (7625 at the quote's bar vs 7622 = +3, what 0.49 drew at 21:33)");
        CHECK(!col::offsetFor(29000.0f, 7622.0f, off), "an NQ chart against an ES book (off > 300) is refused");
        CHECK(!col::offsetFor(0.0f, 7622.0f, off), "no chart close -> refused");
    }
    // ---- mutations (each must fail at least one case above; run by hand: change the header, watch the count) ----
    printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
