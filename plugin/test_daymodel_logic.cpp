/********************************************************************************
 *  test_daymodel_logic.cpp — Gate B (logic) for lsDayModel (plugin/DayModelLogic.h). No IRT SDK.
 *      cloud   : g++ -std=c++14 -o /tmp/dml plugin/test_daymodel_logic.cpp && /tmp/dml
 *      windows : plugin\run-logic-tests.bat
 *  Pins: the bar-stamp detection (v0.16 — swept PDH 7690 vs 7687), the RTH / evening / pre-open windows under each
 *  convention, PDH/PDL/ONH/ONL from the bars, the actual day measured on the most recent RTH day (v0.15 — not the
 *  rolling evening session), the open-anchored re-alignment, the contract offset clamp, the stale age wrap.
 ********************************************************************************/
#include "DayModelLogic.h"
#include <cstdio>
#include <vector>

static int fails = 0, passes = 0;
#define CHECK(cond, msg) do { if (cond) { passes++; printf("  ok    %s\n", msg); } else { fails++; printf("  FAIL  %s   (line %d)\n", msg, __LINE__); } } while (0)

static void add(std::vector<dml::Bar>& v, int y, int m, int d, int sod, float o, float h, float l, float c)
{ dml::Bar b; b.y = y; b.m = m; b.d = d; b.sod = sod; b.o = o; b.h = h; b.l = l; b.c = c; v.push_back(b); }

// his chart: EPZ26 3-minute, full session 17:00-16:00, bars stamped at their END (17:03 ... 16:00)
static std::vector<dml::Bar> endStampedChart(bool includeToday, bool includeTodayRth)
{
    std::vector<dml::Bar> v;
    // Tue 15 Sep: evening of the 14th (17:03..23:57) + night + RTH 08:33..15:00 + evening 17:03..16:00 next day belongs to the 16th
    for (int s = 17 * 3600 + 180; s < 24 * 3600; s += 180) add(v, 2026, 9, 14, s, 7600, 7605, 7595, 7600);          // 14th evening
    for (int s = 180; s <= 8 * 3600 + 30 * 60; s += 180) add(v, 2026, 9, 15, s, 7610, (s == 8 * 3600 + 30 * 60 ? 7690.0f : 7640), 7590, 7610);   // night into the 15th; the bar STAMPED 08:30 (08:27-08:30, pre-open) prints 7690 — the v0.16 trap
    for (int s = 8 * 3600 + 33 * 60; s <= 15 * 3600; s += 180) add(v, 2026, 9, 15, s, 7684, (s == 9 * 3600 ? 7687 : 7680), (s == 14 * 3600 + 30 * 60 ? 7643.5f : 7660), 7656);   // 15th RTH: HOD 7687 at 09:00, LOD 7643.5 at 14:30
    for (int s = 15 * 3600 + 180; s <= 16 * 3600; s += 180) add(v, 2026, 9, 15, s, 7656, 7670, 7650, 7660);         // 15th 15:03..16:00 (outside RTH either way)
    if (includeToday) {
        for (int s = 17 * 3600 + 180; s < 24 * 3600; s += 180) add(v, 2026, 9, 15, s, 7660, 7668, 7655, 7662);      // 15th evening (belongs to the 16th's overnight)
        for (int s = 180; s <= 8 * 3600 + 30 * 60; s += 180) add(v, 2026, 9, 16, s, 7662, 7669.75f, 7617.5f, 7622);  // night into the 16th: ONH 7669.75 / ONL 7617.5
        if (includeTodayRth) for (int s = 8 * 3600 + 33 * 60; s <= 15 * 3600; s += 180) add(v, 2026, 9, 16, s, 7622, (s == 10 * 3600 + 48 * 60 ? 7740 : 7700), (s == 14 * 3600 + 30 * 60 ? 7616 : 7650), 7625);
    }
    return v;
}

int main()
{
    printf("test_daymodel_logic — lsDayModel's decisions\n");
    // ---- 1. the stamp convention
    {
        std::vector<dml::Bar> v = endStampedChart(true, true);
        CHECK(dml::endStamped(&v[0], (int)v.size()), "a chart with a 16:00 bar and no 17:00 bar is END-stamped (his EPZ26 3m)");
        std::vector<dml::Bar> w; for (int s = 17 * 3600; s < 24 * 3600; s += 180) add(w, 2026, 9, 15, s, 1, 1, 1, 1); for (int s = 0; s < 16 * 3600; s += 180) add(w, 2026, 9, 16, s, 1, 1, 1, 1);
        CHECK(!dml::endStamped(&w[0], (int)w.size()), "a chart with a 17:00 bar is START-stamped");
        CHECK(!dml::endStamped(&v[0], 1), "one bar: not decidable -> start-stamped default");
        // the windows under each convention
        CHECK(dml::inRth(15 * 3600, true) && !dml::inRth(15 * 3600, false), "the 15:00 stamp is the last RTH bar when END-stamped, not RTH when START-stamped");
        CHECK(!dml::inRth(8 * 3600 + 30 * 60, true) && dml::inRth(8 * 3600 + 30 * 60, false), "the 08:30 stamp is pre-open when END-stamped, the first RTH bar when START-stamped");
        CHECK(dml::inEvening(17 * 3600 + 180, true) && !dml::inEvening(17 * 3600, true) && dml::inEvening(17 * 3600, false), "17:00 belongs to the evening only when START-stamped");
        CHECK(dml::inPreOpen(8 * 3600 + 30 * 60, true) && !dml::inPreOpen(8 * 3600 + 30 * 60, false), "08:30 is pre-open when END-stamped");
    }
    // ---- 2. the chart-native levels (the v0.16 bug: PDH must be the RTH high 7687, not the 15:03 print 7690)
    {
        std::vector<dml::Bar> v = endStampedChart(true, true);
        dml::Levels L = dml::chartLevels(&v[0], (int)v.size(), true);
        CHECK(L.hp && L.pdh == 7687.0f, "PDH = the prior RTH day's high 7687 (the 7690 on the bar stamped 08:30 is pre-open when END-stamped) — IRT agreed after v0.16");
        CHECK(L.hp && L.pdl == 7643.5f, "PDL = 7643.5 at 14:30");
        CHECK(L.ho && L.onh == 7669.75f && L.onl == 7617.5f, "ONH / ONL = the 15th's evening + the 16th's pre-open");
        dml::Levels L2 = dml::chartLevels(&v[0], (int)v.size(), false);
        CHECK(L2.pdh == 7690.0f, "under the START-stamped reading the same bars would give PDH 7690 — the wrong answer v0.15 drew");
    }
    // ---- 3. the actual day (v0.15: the most recent RTH day, stepping back over a weekend)
    {
        std::vector<dml::Bar> v = endStampedChart(true, true);
        dml::DayMeasure M = dml::measureDay(&v[0], (int)v.size(), true);
        CHECK(M.ok && M.o == 7622.0f && M.h == 7740.0f && M.l == 7616.0f && M.c == 7625.0f, "today's RTH day: O 7622 H 7740 L 7616 C 7625 (the 16th)");
        std::vector<dml::Bar> pre = endStampedChart(true, false);   // the 16th before its open: only the night so far
        dml::DayMeasure P = dml::measureDay(&pre[0], (int)pre.size(), true);
        CHECK(P.ok && P.h == 7687.0f && P.l == 7643.5f, "pre-open on the 16th: the completed 15th keeps showing (not the evening session)");
        CHECK(dml::dayNumber(2026, 9, 16) - dml::dayNumber(2026, 9, 15) == 1 && dml::dayNumber(2026, 9, 14) - dml::dayNumber(2026, 9, 11) == 3, "day numbers: the 14th is three days after the 11th (the weekend step-back)");
        CHECK(dml::dayOffset(7684.25f, true, 7615.5f) > 68.7f && dml::dayOffset(7684.25f, true, 7615.5f) < 68.8f, "the open-anchored spread: chart open 7684.25 vs the CSV's 7615.50 = +68.75 (Sep vs Dec on the 15th)");
        CHECK(dml::dayOffset(7684.25f, false, 0.0f) == 0.0f, "no CSV actual: no spread");
    }
    // ---- 4. the contract offset (pre-open path) and the stale age
    {
        float off = 0;
        CHECK(dml::contractOffset(7684.25f, 7615.5f, off) && off > 68.7f, "offset = chart close - SPOT");
        CHECK(!dml::contractOffset(7615.5f, 7615.5f, off), "already aligned -> nothing to do");
        CHECK(!dml::contractOffset(29000.0f, 7615.5f, off), "an NQ chart -> refused");
        CHECK(dml::staleAge(-1, 40000) < 0, "no ASOF -> unknown");
        CHECK(dml::staleAge(35229, 35229 + 120) == 2.0, "two minutes old");
        CHECK(dml::staleAge(23 * 3600 + 50 * 60, 10 * 60) == 20.0, "the overnight wrap: 23:50 -> 00:10 = 20 min, not -1420");
        CHECK(!dml::staleBadgeShown(4.0) && dml::staleBadgeShown(4.5), "the badge from 4 minutes (the panel writes every 3)");
    }
    printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
