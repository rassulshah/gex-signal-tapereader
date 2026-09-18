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
    // ---- (v0.10) the A row's prices are the chart's own session extremes (the after-hours drift: 7722 -> 7708)
    { dsl::StatRow C; C.first = "HOD"; C.second = "LOD"; C.firstPx = "7716.25"; C.secondPx = "7680.50"; C.rngPts = "35.8"; C.rngUsd = "1788";
      dsl::applyChartExtremes(C, 7722.25, 7687.25, true);
      CHECK(C.firstPx == "7722.25" && C.secondPx == "7687.25", "HOD first: the chart's high is the 1ST price, its low the 2ND (no offset)");
      CHECK(C.rngPts == "35.0" && C.rngUsd == "1750", "the range follows the chart's H-L ($50/pt)");
      dsl::StatRow D = C; D.first = "LOD"; D.second = "HOD"; dsl::applyChartExtremes(D, 7722.25, 7687.25, true);
      CHECK(D.firstPx == "7687.25" && D.secondPx == "7722.25", "LOD first: swapped");
      dsl::StatRow E2 = C; E2.firstPx = "1"; dsl::applyChartExtremes(E2, 0, 0, false); CHECK(E2.firstPx == "1", "no chart day: the panel's prices stay");
      int yy[5] = {2026,2026,2026,2026,2026}, mm[5] = {9,9,9,9,9}, dd[5] = {16,17,17,17,17}; double so[5] = {50000, 30000, 30780, 32940, 60000};
      float hh[5] = {7800, 7790, 7722.25f, 7700, 7799}, ll[5] = {7600, 7610, 7710, 7687.25f, 7690};
      dsl::Ext X; bool ok = dsl::chartExtremes(yy, mm, dd, so, hh, ll, 5, 2026, 9, 17, X);
      CHECK(ok && X.hi == 7722.25f && X.lo == 7687.25f && X.hiSod == 30780 && X.loSod == 32940, "only that day's RTH bars count: not yesterday, not the 08:20 pre-open bar, not the 16:40 after-hours bar");
      CHECK(!dsl::chartExtremes(yy, mm, dd, so, hh, ll, 5, 2026, 9, 18, X), "a day with no RTH bar -> false"); }

    // ---- (0.11) the READ line's second half: one rung, descending with the clock
    { dsl::Cond C; C.lastHr = 39; C.last30 = 29; C.p20 = 132; C.p50 = 267; C.p80 = 372;   // pos60-top, 2026-09-17
      CHECK(dsl::secondRung("LOD", C, 30600 + 60 * 60, false, -1) == "LOD after 10:42am  80%", "before p20: 'LOD after 10:42am 80%' (132 min after the open)");
      CHECK(dsl::secondRung("LOD", C, 30600 + 200 * 60, false, -1) == "LOD after 12:57pm  50%", "past p20, before p50: the median rung");
      CHECK(dsl::secondRung("LOD", C, 30600 + 300 * 60, false, -1) == "LOD 39% last hr", "past p50, before 14:00: the last-hour share");
      CHECK(dsl::secondRung("LOD", C, 50400 + 600, false, -1) == "LOD 29% last 30", "14:00-14:30: the last-30 share");
      CHECK(dsl::secondRung("LOD", C, 52200 + 60, false, -1) == "LOD any minute", "after 14:30: any minute");
      CHECK(dsl::secondRung("HOD", C, 60000, true, 32940) == "HOD IN 9:09am", "after the close with the actual 2ND known: IN + its clock");
      dsl::Cond N; CHECK(dsl::secondRung("LOD", N, 40000, false, -1) == "LOD any minute" && dsl::secondRung("", C, 40000, false, -1) == "", "no percentiles (old panel): the last rung only; no second extreme: nothing");
      std::vector<std::string> t = {"CONDE","pos60-top+orclock","3.0","267.0","35","99","39","29","132","267","372"}; dsl::Cond P; dsl::parseCond(t, P);
      CHECK(P.p20 == 132 && P.p50 == 267 && P.p80 == 372 && P.lastHr == 39, "CONDE fields 9-11 parse; an 8-field row (old panel) leaves them -1");
      std::vector<std::string> t8 = {"CONDE","pre-open","24","286.5","52","300","39","29"}; dsl::Cond Q; dsl::parseCond(t8, Q); CHECK(Q.p20 == -1 && Q.lastHr == 39, "(old row)"); }

    // ---- (0.12) the second extreme's read (READ2) and its line
    { std::vector<std::string> t = {"READ2","LOD","25","0.201","300.0","51.0","0.220","9","70430","baked"}; dsl::Read2 r; CHECK(dsl::parseRead2(t, r) && r.side == "LOD" && r.p == 25 && r.arrival == 9 && r.src == "baked", "READ2 parses: side, p, features, arrival, source");
      CHECK(dsl::secondLine(r, 36000, false, -1) == "LOD IN 25%  · if not, ~10:09am (50%)", "p < 50: the probability plus the arrival from the export's own clock (10:00 + 9 min)");
      r.p = 80; CHECK(dsl::secondLine(r, 39600, false, -1) == "LOD IN 80%", "p >= 50: the probability alone (no clock for something probably done)");
      CHECK(dsl::secondLine(r, 60000, true, 32940) == "LOD IN 9:09am", "after the close with the actual 2ND: IN + its clock");
      std::vector<std::string> bad = {"READ2","XOD","25"}; dsl::Read2 q; CHECK(!dsl::parseRead2(bad, q) && dsl::secondLine(q, 36000, false, -1) == "", "a malformed row draws nothing");
      r.p = 25; r.arrival = -1; CHECK(dsl::secondLine(r, 36000, false, -1) == "LOD IN 25%", "no arrival median for the bin: the probability alone"); }

    // ---- (0.13) the whole READ line: the first extreme leads; after the close both halves print their clocks
    { CHECK(dsl::readLine("HOD", "IN", 96, false, -1, "LOD IN 80%") == "HOD IN  96%   \xB7   LOD IN 80%", "live, HOD first: 'HOD IN 96% · LOD IN 80%'");
      CHECK(dsl::readLine("LOD", "", 45, false, -1, "HOD IN 25%  \xC2\xB7 if not, ~10:09am (50%)").rfind("LOD  45%   \xB7   HOD IN 25%", 0) == 0, "LOD first: the LOD leads, the HOD is the second half (operator: 'if the lod occurs, it should be before the HOD and vice versa')");
      CHECK(dsl::readLine("HOD", "NOTIN", 12, false, -1, "") == "HOD NOT IN  12%", "NOT IN, no second half yet: the first half alone");
      CHECK(dsl::readLine("HOD", "IN", 96, true, 30780, "LOD IN 9:09am") == "HOD IN 8:33am   \xB7   LOD IN 9:09am", "after the close: BOTH clocks (0.12 printed 'HOD IN 96%' beside 'LOD IN 9:09am')");
      CHECK(dsl::readLine("HOD", "IN", 96, true, -1, "LOD IN 95%") == "HOD IN  96%   \xB7   LOD IN 95%", "closed but no A row: the live halves stay");
      CHECK(dsl::readLine("", "IN", 96, false, -1, "x") == "", "no first extreme: nothing"); }
    // ---- (0.14) the first half from READ1 (the same model); the HLTAB cell only before the model's gate
    { std::vector<std::string> t = {"READ1","HOD","74","0.310","300.0","87.0","0.290","27","70430","baked"}; dsl::Read2 r1; CHECK(dsl::parseRead2(t, r1) && r1.side == "HOD" && r1.p == 74, "READ1 parses through parseRead2 (same shape as READ2)");
      CHECK(dsl::firstHalf(r1, "HOD", "", 47, false, -1, 36000) == "HOD IN 74%", "READ1 present: 'HOD IN 74%' from the model, not the HLTAB cell's 47");
      r1.p = 44; CHECK(dsl::firstHalf(r1, "HOD", "IN", 96, false, -1, 39600) == "HOD IN 44%  \xC2\xB7 if not, ~11:27am (50%)", "p < 50: the arrival clock, like the second half (11:00 + 27 min)");
      dsl::Read2 none; CHECK(dsl::firstHalf(none, "HOD", "", 40, false, -1, 32400) == "HOD  40%", "no READ1 yet (before 36 min): the HLTAB cell as before");
      CHECK(dsl::firstHalf(none, "HOD", "NOTIN", 12, false, -1, 32400) == "HOD NOT IN  12%", "... with its call");
      CHECK(dsl::firstHalf(r1, "HOD", "", 47, true, 30780, 60000) == "HOD IN 8:33am", "after the close: the clock, whatever the rows say");
      r1.side = "LOD"; CHECK(dsl::firstHalf(r1, "HOD", "IN", 96, false, -1, 36000) == "HOD IN  96%", "a READ1 for the other side is ignored (a stale row): the cell stands");
      r1.side = "HOD"; r1.p = 74; CHECK(dsl::firstTone(r1, "HOD", "", false, -1) == 1, "tone: >= 70 green");
      r1.p = 50; CHECK(dsl::firstTone(r1, "HOD", "IN", false, -1) == 0, "tone: 31-69 plain even if the old cell said IN");
      r1.p = 12; CHECK(dsl::firstTone(r1, "HOD", "", false, -1) == -1 && dsl::firstTone(none, "HOD", "NOTIN", false, -1) == -1 && dsl::firstTone(none, "HOD", "", true, 30780) == 1, "tone: <= 30 amber; no READ1 -> the call; closed -> green"); }
    // ---- (0.13) the block's anchor: centre entries, and never cut off at the left
    { CHECK(dsl::anchorX(0, 60, 1400, 1000, 8) == 68 && dsl::anchorX(1, 60, 1400, 1000, 8) == 392, "TL / TR anchors as before on a wide pane");
      CHECK(dsl::anchorX(4, 60, 1400, 1000, 8) == 230 && dsl::anchorX(5, 60, 1400, 1000, 8) == 230, "Top-centre / Bottom-centre: the block on the pane's midline");
      CHECK(dsl::anchorX(1, 60, 900, 1000, 8) == 68 && dsl::anchorX(4, 60, 900, 1000, 8) == 68, "a block wider than the pane: clamped to the left inset (the right overflows, the READ line stays)");
      CHECK(!dsl::anchorBottom(0) && !dsl::anchorBottom(1) && !dsl::anchorBottom(4) && dsl::anchorBottom(2) && dsl::anchorBottom(3) && dsl::anchorBottom(5), "bottom anchors: BL, BR, Bottom-centre"); }

    printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
