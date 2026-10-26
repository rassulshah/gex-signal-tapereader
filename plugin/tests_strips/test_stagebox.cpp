// test_stagebox.cpp -- StageBox.h 1.0.0 (lsReversalStage 2.1.0): the stage info box rows and the letters 1 2 3 C.
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_stagebox.cpp
#include "StageBox.h"
#include <iostream>
#include <map>
static int passed = 0, failed = 0;
static void check(bool c, const std::string& m) { if (c) ++passed; else { ++failed; std::cout << "FAIL " << m << "\n"; } }
struct FakeStep { int side = -1, stage = 0; long long start = 0, ext = 0; std::string story, gate; };
static sbx::Bar bar(long long k, long long o, long long h, long long l, long long c, double vol) { sbx::Bar b; b.key = k; b.ok = true; b.o = o; b.h = h; b.l = l; b.c = c; b.vol = vol; return b; }
// 25 quiet bars (volume 1000), then a pivot low at index 22: red bar 400 -> low 360 -> close 380; 23 green close 400; 24 higher high
static std::vector<sbx::Bar> day(bool greenNext = true, bool newLow = false) {
    std::vector<sbx::Bar> B;
    for (int i = 0; i < 22; ++i) B.push_back(bar(1000 + i, 420, 440, 410, 425, 1000));
    sbx::Bar p = bar(1022, 400, 405, 360, 380, 2000);
    std::vector<sbx::Row> R; for (long long px = 360; px <= 405; px += 5) { sbx::Row r; r.px = px; r.sell = px < 380 ? 100 : 20; r.buy = 30; r.total = r.sell + r.buy; R.push_back(r); }
    sbx::fillVap(p, R); B.push_back(p);
    if (newLow) B.push_back(bar(1023, 380, 385, 350, 355, 1500));
    else B.push_back(greenNext ? bar(1023, 382, 402, 375, 400, 1500) : bar(1023, 382, 390, 375, 378, 1500));
    if (greenNext) { B.push_back(bar(1024, 400, 420, 395, 415, 1200)); B.push_back(bar(1025, 415, 418, 405, 410, 1000)); B.push_back(bar(1026, 410, 412, 400, 405, 1000)); }
    else { B.push_back(bar(1024, 378, 380, 365, 370, 1200)); B.push_back(bar(1025, 370, 372, 362, 365, 1000)); B.push_back(bar(1026, 365, 368, 361, 362, 1000)); }
    return B;
}
int main() {
    // params
    { const sbx::Params es = sbx::defaultsFor("ES"), eu = sbx::defaultsFor("EU"), e6 = sbx::defaultsFor("6E"), nq = sbx::defaultsFor("NQ"), hg = sbx::defaultsFor("HG");
      check(es.trapThr == 0.30 && es.wickThr == 0.37 && nq.trapThr == 0.31 && nq.wickThr == 0.43 && hg.trapThr == 0.26 && hg.wickThr == 0.35, "[par] TRAPPED.md defaults ES / NQ / HG");
      check(eu.trapThr == 0.22 && eu.wickThr == 0.24 && e6.trapThr == 0.22 && sbx::defaultsFor("CL").trapThr == 0.33 && sbx::defaultsFor("GC").wickThr == 0.48 && sbx::defaultsFor("NG").trapThr == 0.28, "[par] 6E / EU / CL / GC / NG");
      sbx::Params p = es; check(sbx::parseParam("PARAM,TRAP_THR,0.41", p) && p.trapThr == 0.41, "[par] PARAM,KEY,VALUE");
      check(sbx::parseParam("HB_MIN,1.5\r", p) && p.hbMin == 1.5, "[par] KEY,VALUE with CR");
      check(!sbx::parseParam("TRAP_THR,abc", p) && !sbx::parseParam("TRAP_THR,-1", p) && !sbx::parseParam("NOPE,1", p) && p.trapThr == 0.41, "[par] bad lines ignored, value kept"); }
    // VAP sums
    { sbx::Bar b = bar(1, 100, 110, 90, 105, 0); std::vector<sbx::Row> R; for (long long px = 90; px <= 110; ++px) { sbx::Row r; r.px = px; r.buy = 2; r.sell = 3; r.total = 5; R.push_back(r); }
      sbx::fillVap(b, R); check(b.vol == 105 && b.sellsBelowClose == 15 * 3 && b.buysAboveClose == 5 * 2 && b.wickLow == 10 * 5 && b.wickHigh == 5 * 5, "[vap] sums: volume, sells below close, buys above, wicks"); }
    std::map<long long, FakeStep> steps;
    { FakeStep s; s.side = sbx::LOW; s.start = 1022; s.ext = 360; s.stage = 2; s.story = "bids 7,752 > offers 6,316~2,169 lots taken, Q 117%~~"; steps[1022] = s; steps[1023] = s; }
    const std::vector<sbx::Attempt> A = sbx::attemptsOf(steps);
    check(A.size() == 1 && A[0].firstKey == 1022 && A[0].lastKey == 1023 && A[0].stage == 2, "[att] one attempt from two recorded candles");
    std::vector<sbx::Letter> L;
    { sbx::Letter e; e.key = 1022; e.pane = 0; e.code = "E"; e.pace100 = 40; L.push_back(e);
      sbx::Letter h; h.key = 1023; h.pane = 1; h.code = "H"; h.ratio = 1.5; L.push_back(h); }
    const sbx::Params P = sbx::defaultsFor("ES");
    {   const sbx::Box X = sbx::build(A[0], day(), L, P, 0.25, "1:03 PM");
        check(X.on && X.pivKey == 1022 && X.ext == 360 && X.side == sbx::LOW, "[box] the pivot bar = the candle that made the low");
        check(X.rows.size() == 5 && X.rows[0].code == "1" && X.rows[1].code == "2" && X.rows[2].code == "2" && X.rows[3].code == "3" && X.rows[4].code == "C", "[box] rows 1 / 2 / 2 / 3 / C");
        check(X.rows[0].state == sbx::SEEN && X.rows[0].value == "bids 7,752 > offers 6,316", "[box] 1 Trap from the engine's story");
        check(X.rows[1].state == sbx::SEEN && X.rows[1].value == "2,169 lots taken, Q 117%", "[box] 2 absorption from the story");
        check(X.rows[2].state == sbx::SEEN && X.rows[2].value.find("pace 0.4x (0.5x)") == 0, "[box] 2 exhaustion: pace 0.4x <= 0.5x");
        check(X.rows[3].state == sbx::SEEN && X.triggerKey == 1023 && X.rows[3].value.find("close 100.00") != std::string::npos && X.rows[3].value.find("HELD") != std::string::npos, "[box] 3 Turn: the green close = the HELD trigger");
        // trapped: sells below the close 380 = 4 rows x 100 = 400 / avg 1000 = 0.40x ; wick (below min(o,c)=380) volume 4 x 130 = 520 -> 0.52x
        check(std::fabs(X.trapped - 0.40) < 1e-9 && std::fabs(X.wick - 0.52) < 1e-9 && X.rows[3].value.find("trapped 0.40x (0.30x)") == 0, "[box] trapped 0.40x (0.30x) = sells below the close / avg bar volume; wick 0.52x");
        check(X.rows[4].state == sbx::SEEN && X.hb == 1.5 && X.follow && X.rows[4].value.find("Hb 1.5x (1.25x)") == 0, "[box] C: Hb 1.5x (1.25x) + next bar higher high");
        check(X.letters == "1 2 3 C" && X.decided, "[box] letters 1 2 3 C, decided");
        check(X.hover.size() == 4 && X.hover[0].rfind("Question:", 0) == 0 && X.hover[1].rfind("Answer:", 0) == 0 && X.hover[2].rfind("How to use:", 0) == 0 && X.hover[3].rfind("Example:", 0) == 0 && X.hover[1].find("description only") != std::string::npos, "[box] hover in his format, trapped / wick = description only");
        check(X.title.find("1:03 PM") != std::string::npos && X.title.find("bullish") != std::string::npos, "[box] title");
        // no repaint: the box decided at the trigger's next bar is identical with every later bar
        std::vector<sbx::Bar> early = day(); early.resize(25);
        const sbx::Box Y = sbx::build(A[0], early, L, P, 0.25, "1:03 PM");
        bool same = Y.letters == X.letters && Y.rows.size() == X.rows.size(); for (size_t k = 0; same && k < X.rows.size(); ++k) same = Y.rows[k].state == X.rows[k].state && Y.rows[k].value == X.rows[k].value;
        check(same, "[box] no repaint: decided at bar 24, identical with later bars"); }
    {   // trapped high but NO defenders' close: 3 never checked by trapped alone
        const sbx::Box X = sbx::build(A[0], day(false), L, P, 0.25);
        check(X.rows[3].state == sbx::NOT_SEEN && X.trapped > P.trapThr && X.letters == "1 2", "[box] trapped above threshold but no green close: 3 not checked (description only)");
        check(X.rows[4].state == sbx::NOT_SEEN, "[box] no trigger: C not checked even with Hb 1.5x"); }
    {   const sbx::Box X = sbx::build(A[0], day(true, true), L, P, 0.25);
        check(X.rows[3].state == sbx::NOT_SEEN, "[box] a new low before any green close: no turn"); }
    {   std::vector<sbx::Letter> L2 = L; L2[1].ratio = 1.1;
        const sbx::Box X = sbx::build(A[0], day(), L2, P, 0.25);
        check(X.rows[4].state == sbx::NOT_SEEN && X.rows[4].value.find("Hb 1.1x (1.25x)") == 0 && X.letters == "1 2 3", "[box] Hb 1.1x < 1.25x: C not checked"); }
    {   std::vector<sbx::Letter> L3 = L; sbx::Letter f; f.key = 1022; f.pane = 1; f.code = "F?"; f.nums = "front 1.6x behind 0.6x"; L3.push_back(f);
        const sbx::Box X = sbx::build(A[0], day(), L3, P, 0.25);
        check(X.rows.size() == 6 && X.rows[5].code == "F?" && X.rows[5].state == sbx::WARN && X.rows[5].value == "front 1.6x behind 0.6x", "[box] F? on the pivot bar = the amber warning row"); }
    {   std::map<long long, FakeStep> s2 = steps; for (auto& kv : s2) kv.second.stage = 0;
        const sbx::Box X = sbx::build(sbx::attemptsOf(s2)[0], day(), std::vector<sbx::Letter>(), P, 0.25);
        check(X.rows[0].state == sbx::NOT_SEEN && X.rows[1].state == sbx::NOT_SEEN && X.rows[2].value.rfind("not here", 0) == 0 && X.letters == "3", "[box] nothing recorded: only 3 (the close)"); }
    {   // a high: mirror (red close below, buys above the close)
        std::vector<sbx::Bar> B; for (int i = 0; i < 22; ++i) B.push_back(bar(1000 + i, 380, 390, 370, 385, 1000));
        B.push_back(bar(1022, 400, 440, 395, 420, 2000)); B.push_back(bar(1023, 418, 425, 400, 402, 1500)); B.push_back(bar(1024, 402, 405, 390, 392, 1000));
        std::map<long long, FakeStep> sh; FakeStep s; s.side = sbx::HIGH; s.start = 1022; s.ext = 440; s.stage = 1; s.story = "offers 2.0x~~~"; sh[1022] = s;
        const sbx::Box X = sbx::build(sbx::attemptsOf(sh)[0], B, std::vector<sbx::Letter>(), P, 0.25);
        check(X.side == sbx::HIGH && X.rows[3].state == sbx::SEEN && X.triggerKey == 1023 && X.title.find("bearish") != std::string::npos && X.letters == "1 3", "[box] high: red close = the trigger, letters 1 3"); }
    std::cout << "StageBox " << sbx::VERSION << ": " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
