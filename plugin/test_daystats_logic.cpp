/********************************************************************************
 *  test_daystats_logic.cpp — Gate B (logic) for lsDayStats (plugin/DayStatsLogic.h). No IRT SDK.
 *      cloud   : g++ -std=c++14 -o /tmp/dsl plugin/test_daystats_logic.cpp && /tmp/dsl
 *      windows : plugin\run-logic-tests.bat
 *  Pins: the DAYSA / DAYSE / CONDE row grammar, his clocks and durations, the price to the chart's contract, the twelve
 *  A and E cells as printed on 2026-09-16 (the 21:33 strip), the 2ND ladder, the '=' rule, the colour rule.
 ********************************************************************************/
#include "DayStatsLogic.h"
#include <cstdio>
#include <sstream>

static int fails = 0, passes = 0;
#define CHECK(cond, msg) do { if (cond) { passes++; printf("  ok    %s\n", msg); } else { fails++; printf("  FAIL  %s   (line %d)\n", msg, __LINE__); } } while (0)
static std::vector<std::string> split(const std::string& line) { std::vector<std::string> t; std::stringstream ss(line); std::string it; while (std::getline(ss, it, ',')) t.push_back(it); return t; }

int main()
{
    printf("test_daystats_logic — lsDayStats' rows and cells\n");
    // ---- formatting
    CHECK(dsl::clk(38880) == "10:48am" && dsl::clk(52200) == "2:30pm" && dsl::clk(43200) == "12:00pm" && dsl::clk(0) == "12:00am", "clocks: 10:48am, 2:30pm, 12:00pm, 12:00am");
    CHECK(dsl::clk(-1) == "--", "a missing clock prints --");
    CHECK(dsl::dur(138) == "2h 18m" && dsl::dur(12) == "12m" && dsl::dur(59.6) == "1h 00m" && dsl::dur(-1) == "--", "durations: 2h 18m, 12m, 1h 00m, --");
    CHECK(dsl::px1("7675.75", 68.94) == "7745" && dsl::px1("", 0) == "--", "prices land on the chart's contract (+68.94 -> 7745), missing -> --");
    CHECK(dsl::pct(23.4, true) == "~23%" && dsl::pct(18, false) == "18%" && dsl::pct(-1, true) == "--", "percentages with and without the ~");
    // ---- the rows of 2026-09-16 (the 21:33 strip): A = actual, E = expected (16.35), CONDE = the stage + the ladder
    dsl::StatRow A, E; dsl::Cond C;
    CHECK(dsl::parseStatRow(split("DAYSA,HOD,7741.00,38880,138,12,50,33600,18,201,LOD,7617.00,52200,222,124.0,6200,,"), A) && A.valid && A.first == "HOD" && A.took == 138 && A.second == "LOD" && A.rngPts == "124.0", "DAYSA parses (17 fields, the last two empty)");
    CHECK(dsl::parseStatRow(split("DAYSE,HOD,7741.00,38880,138,12,50,33600,23,201,LOD,7622.00,48960,168,44.9,2247,34.4,75.7"), E) && E.valid && E.rngP25 == "34.4", "DAYSE parses with the IQR fields carried (not drawn)");
    CHECK(!dsl::parseStatRow(split("DAYSA,HOD,7741.00"), A), "a short row is refused");
    CHECK(dsl::parseCond(split("CONDE,pos60-middle,36,306,53,93,39,29"), C) && C.basis == "pos60-middle" && C.lastHr == 39 && C.last30 == 29, "CONDE parses: basis, last-hour 39%, last-30 29%");
    dsl::Cond C6; CHECK(dsl::parseCond(split("CONDE,pos30-bottom,21,286.5,59,114"), C6) && C6.lastHr == -1, "a 16.32 CONDE (no ladder) parses with the ladder unknown");
    // ---- the cells
    std::string a[dsl::NCOL], e[dsl::NCOL];
    dsl::actualCells(A, 0.0, a);
    CHECK(a[0] == "A" && a[1] == "HOD 10:48am 7741" && a[2] == "2h 18m" && a[3] == "12m" && a[4] == "50m" && a[5] == "9:20am" && a[6] == "18%", "A cells 0-6: HOD 10:48am 7741 · 2h 18m · 12m · 50m · 9:20am · 18%");
    CHECK(a[7] == "3h 21m" && a[8] == "3h 30m" && a[9] == "LOD 2:30pm 7617" && a[10] == "3h 42m" && a[11] == "$6200  124.0p", "A cells 7-11: MUD 3h 21m · MUDt 3h 30m (gap - bop) · LOD 2:30pm 7617 · 3h 42m · $6200  124.0p");
    dsl::expectedCells(E, C, e);
    CHECK(e[0] == "E" && e[1] == "HOD ~10:48am" && e[2] == "~2h 18m" && e[6] == "~23%", "E cells: the ~ on every expectation");
    CHECK(e[9] == "LOD ~1:36pm  39% last hr", "the 2ND cell carries the ladder: 'LOD ~1:36pm  39% last hr'");
    CHECK(e[11] == "~$2247  44.9p" || e[11] == "~$2247  ~44.9p", "the E range cell");
    dsl::expectedCells(E, C6, e);
    CHECK(e[9] == "LOD ~1:36pm", "no ladder on the CONDE row -> no suffix");
    dsl::Cond Cr; Cr.basis = "read-in-HOD"; Cr.lastHr = 39;
    dsl::expectedCells(E, Cr, e);
    CHECK(e[1] == "HOD =10:48am", "a read-in basis prints '=' (16.36 no longer emits it, the rule stays for older CSVs)");
    // ---- the colour rule
    int tone[dsl::NCOL]; dsl::actualTone(A, tone);
    CHECK(tone[1] == 1 && tone[9] == -1 && tone[7] == -1, "HOD first -> 1ST green, LOD second -> 2ND red, MUD red (markdown after a first HOD)");
    dsl::StatRow B = A; B.first = "LOD"; B.second = "HOD"; dsl::actualTone(B, tone);
    CHECK(tone[1] == -1 && tone[9] == 1 && tone[7] == 1, "LOD first -> 1ST red, 2ND green, MUD green (markup)");
    printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
