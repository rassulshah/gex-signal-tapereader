// test_fmirror.cpp -- FMirror.h 1.0.0 (lsBookFlow 1.28.0): F? the pivot likely dies at once.
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_fmirror.cpp
#include "FMirror.h"
#include <iostream>
static int passed = 0, failed = 0;
static void check(bool c, const std::string& m) { if (c) ++passed; else { ++failed; std::cout << "FAIL " << m << "\n"; } }
static fmr::Bar bar(long long k, long long o, long long h, long long l, long long c) { fmr::Bar b; b.key = k; b.ok = true; b.o = o; b.h = h; b.l = l; b.c = c; return b; }
int main() {
    check(fmr::floorOf("ES") == 10 && fmr::floorOf("NQ") == 5 && fmr::floorOf("CL") == 5 && fmr::floorOf("GC") == 3 && fmr::floorOf("HG") == 3 && fmr::floorOf("NG") == 3 && fmr::floorOf("EU") == 9 && fmr::floorOf("6E") == 9, "[floor] the research floors per market");
    {   fmr::Params p; check(!p.set(), "[par] no file = thresholds not set");
        check(fmr::parseParam("PARAM,F_FRONT,1.3", p) && fmr::parseParam("F_BEHIND,0.74\r", p) && p.set() && p.front == 1.3 && p.behind == 0.74, "[par] F_FRONT / F_BEHIND");
        check(fmr::parseParam("PARAM,SHOW,0", p) && !p.show && !fmr::parseParam("SHOW,2", p) && !fmr::parseParam("F_FRONT,x", p) && !fmr::parseParam("F_FRONT,-1", p) && p.front == 1.3, "[par] SHOW, bad values ignored"); }
    // 6 bars: 5 flat, then a new low that closes in its upper half (key = the bar END, seconds)
    std::vector<fmr::Bar> B; for (int i = 0; i < 15; ++i) B.push_back(bar(1000 + 180 * i, 400, 404, 396, 401));
    B.push_back(bar(1000 + 180 * 15, 400, 402, 390, 400));   // low 390 < 396, close 400 in the upper half (range 12)
    const size_t piv = B.size() - 1; const long long T = B[piv].key;
    check(fmr::pivotOf(B, piv) == fmr::LOW && fmr::pivotOf(B, piv - 1) == -1, "[piv] a 5-candle low closing in its upper half");
    { std::vector<fmr::Bar> C = B; C[piv].c = 391; check(fmr::pivotOf(C, piv) == -1, "[piv] a low closing in its lower half is not a candidate"); }
    // ATR: 15 x range 8 then 12 -> mean of the last 14 = (13 x 8 + 12) / 14 = 8.29 -> band 4
    fmr::Acc A;
    A.on(fmr::K_ADD, fmr::ASK, 402, 300, T - 30);   // offers stacked in front (above the close 400, within 4)
    A.on(fmr::K_PULL, fmr::ASK, 403, 50, T - 20);
    A.on(fmr::K_ADD, fmr::ASK, 410, 999, T - 20);   // outside the band: ignored
    A.on(fmr::K_PULL, fmr::BID, 398, 120, T - 10);  // bids pulled behind (below the close)
    A.on(fmr::K_ADD, fmr::BID, 397, 20, T - 5);
    A.on(fmr::K_ADD, fmr::ASK, 401, 500, T - 90);   // before the 60-s window: ignored
    A.on(fmr::K_ADD, fmr::ASK, 401, 500, T);        // at the close (not before it): ignored
    A.on(fmr::K_FILL, fmr::BID, 398, 1000, T - 3);  // a fill is never a pull
    fmr::Params P; fmr::Read r;
    check(fmr::decide(B, piv, A, "ES", P, r) && r.side == fmr::LOW && r.key == T && r.ext == 390, "[read] decided at the pivot-bar close");
    check(r.front == 25.0 && r.behind == 10.0, "[read] front = (300 - 50) / 10 = 25 floors, behind = (120 - 20) / 10 = 10 floors");
    check(r.armed == -1, "[read] thresholds not set: recorded, armed -1 (never drawn)");
    P.front = 20; P.behind = 8; check(fmr::decide(B, piv, A, "ES", P, r) && r.armed == 1, "[read] both >= the thresholds: F?");
    P.behind = 12; check(fmr::decide(B, piv, A, "ES", P, r) && r.armed == 0, "[read] behind below its threshold: no F?");
    {   const std::string ln = fmr::lineOf(r); fmr::Read q; check(fmr::parse(ln, q) && q.key == r.key && q.side == r.side && q.front == r.front && q.behind == r.behind && q.armed == r.armed && q.ext == r.ext, "[rec] F line round trip: " + ln);
        check(!fmr::parse("F|1|5|0|0|1|0", q) && !fmr::parse("F|x", q) && !fmr::parse("G|1|0|0|0|1|0", q), "[rec] bad lines rejected"); }
    {   // a high: bids in front (below the close), offers pulled behind
        std::vector<fmr::Bar> H; for (int i = 0; i < 15; ++i) H.push_back(bar(1000 + 180 * i, 400, 404, 396, 401)); H.push_back(bar(1000 + 180 * 15, 402, 410, 398, 402));
        fmr::Acc A2; const long long T2 = H.back().key; A2.on(fmr::K_ADD, fmr::BID, 400, 200, T2 - 10); A2.on(fmr::K_PULL, fmr::ASK, 404, 90, T2 - 10);
        fmr::Read r2; check(fmr::decide(H, H.size() - 1, A2, "ES", fmr::Params(), r2) && r2.side == fmr::HIGH && r2.front == 20.0 && r2.behind == 9.0, "[read] high mirrored"); }
    {   fmr::Params Q; Q.front = 1.3; Q.behind = 0.7; const std::string h = fmr::hover(r, Q);
        check(h.rfind("Question:", 0) == 0 && h.find("\nAnswer:") != std::string::npos && h.find("\nHow to use:") != std::string::npos && h.find("\nExample:") != std::string::npos, "[hover] his format"); }
    std::cout << "FMirror " << fmr::VERSION << ": " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
