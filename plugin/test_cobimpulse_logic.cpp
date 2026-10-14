// test_cobimpulse_logic.cpp -- unit tests for CobImpulseLogic.h (lsCobImpulse 0.1.0)
// g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra test_cobimpulse_logic.cpp -o t && ./t [nq_20261014.csv]
#include "CobImpulseLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); } } while (0)

using cimp::Bar;
// a calm history (ranges 2.0-2.8, volumes 100-160), a 5-bar down leg, the pivot LOW on bar 55, an up leg to bar 58, the CoB on bar 59
static std::vector<Bar> base()
{
    std::vector<Bar> B;
    for (int i = 0; i < 50; ++i) { Bar b; b.t = 1000 + i; b.h = 101 + 0.1 * (i % 5); b.l = 99 - 0.1 * (i % 5); b.o = b.c = 100; b.v = 100 + 10 * (i % 7); B.push_back(b); }
    for (int k = 1; k <= 5; ++k) { Bar b; b.t = 1000 + 49 + k; b.h = 100.5 - k; b.l = 98.5 - k; b.o = b.h; b.c = b.l; b.v = 100; B.push_back(b); }
    const double hl[5][2] = { { 94, 92 }, { 95, 93 }, { 96, 94 }, { 97, 95 }, { 96.5, 95.5 } };
    for (int k = 0; k < 5; ++k) { Bar b; b.t = 1055 + k; b.h = hl[k][0]; b.l = hl[k][1]; b.o = b.l; b.c = b.h; b.v = 100; B.push_back(b); }
    return B;   // 60 bars; trig = 59
}
static std::vector<Bar> mirror(std::vector<Bar> B)
{
    for (auto& b : B) { const double h = b.h, l = b.l, o = b.o, c = b.c; b.h = -l; b.l = -h; b.o = -o; b.c = -c; }
    return B;
}
static cimp::Tag ev(const std::vector<Bar>& B, int side = 1, bool abs = false, cimp::Params P = cimp::Params())
{
    return cimp::evaluate(B, 59, side, P, abs);
}

int main(int argc, char** argv)
{
    cimp::Params P;
    {   // the leg pivot
        auto B = base(); int le = -1;
        CHECK(cimp::legPivot(B, 59, 1, &le) == 55 && le == 58, "bull leg pivot = bar 55, leg end 58");
        CHECK(cimp::legPivot(B, 59, -1) == -1, "the leg runs up: no bear pivot");
        auto T = ev(B);
        CHECK(T.pivot == 55 && !T.any() && T.why == "no thrust", "calm leg: plain Cob");
        CHECK(cimp::label(T.r.bar >= 0, T.v.bar >= 0, true, P) == "Cob", "plain label");
    }
    {   // range thrust ON the pivot bar (down thrust making the low)
        auto B = base(); B[55].l = 88;
        auto T = ev(B);
        CHECK(T.pivot == 55 && T.r.bar == 55 && T.r.n == 56 && T.v.bar < 0, "R on the pivot bar, biggest in 56 bars (all history)");
        CHECK(T.r.x > 2.3 && T.r.x < 3.1, "range 6 vs the average (2.0-2.8) = 2.3-3.0x");
    }
    {   // +1
        auto B = base(); B[56].h = 99;
        auto T = ev(B);
        CHECK(T.pivot == 55 && T.r.bar == 56, "R on pivot + 1");
    }
    {   // +2
        auto B = base(); B[57].h = 100; B[57].l = 93.5;
        auto T = ev(B);
        CHECK(T.pivot == 55 && T.r.bar == 57, "R on pivot + 2");
    }
    {   // +3 = outside the window
        auto B = base(); B[58].h = 101; B[58].l = 93.5;
        auto T = ev(B);
        CHECK(T.pivot == 55 && !T.any(), "a thrust on pivot + 3 is outside the window");
        cimp::Params W; W.window = 3;
        CHECK(ev(B, 1, false, W).r.bar == 58, "WINDOW 3 reaches it");
    }
    {   // low not below the pivot bar's high -> rejected
        auto B = base(); B[57].h = 100.5; B[57].l = 94.5;          // range 6, but its low 94.5 >= pivot high 94
        CHECK(!ev(B).any(), "impulse bar low not below the pivot high: rejected");
        B[57].l = 94;                                              // equal is not below
        CHECK(!ev(B).any(), "low EQUAL to the pivot high: rejected");
        B[57].l = 93.75;
        CHECK(ev(B).r.bar == 57, "low just below the pivot high: accepted");
    }
    {   // ties: an equal earlier range does not stop the count; a bigger one does
        auto B = base(); B[56].h = 99;                             // range 6
        B[30].h = 103; B[30].l = 97;                               // range 6 at 26 bars back: a tie
        auto T = ev(B);
        CHECK(T.r.bar == 56 && T.r.n == 57, "tie keeps counting (biggest in 57)");
        B[40].h = 103.5; B[40].l = 96.5;                           // range 7 at 16 bars back: bigger
        T = ev(B);
        CHECK(T.r.bar < 0, "a bigger bar 16 back: not the biggest in 35");
        CHECK(cimp::rangeSince(B, 56, 100) == 16, "since = 16");
    }
    {   // exactly 35
        auto B = base(); B[56].h = 99; B[21].h = 104; B[21].l = 96;   // bigger at 35 bars back
        CHECK(cimp::rangeSince(B, 56, 100) == 35 && ev(B).r.bar == 56, "biggest in exactly 35: tagged");
        B[21].h = 101; B[21].l = 99; B[22].h = 104; B[22].l = 96;      // bigger at 34 back
        CHECK(cimp::rangeSince(B, 56, 100) == 34 && ev(B).r.bar < 0, "biggest in 34: not tagged");
    }
    {   // volume thrust, missing volume
        auto B = base(); B[56].v = 500;
        auto T = ev(B);
        CHECK(T.v.bar == 56 && T.r.bar < 0 && T.v.x > 3.5, "V only on pivot + 1, 3.5x+");
        CHECK(cimp::label(false, true, true, P) == "Cob+V?", "label Cob+V?");
        B[56].v = 0; CHECK(!ev(B).any(), "zero volume on the candidate: no V");
        B[56].v = std::nan(""); CHECK(!ev(B).any(), "NaN volume: no V");
        B[56].v = 500; B[46].v = 0;                                 // unknown volume 10 bars back stops the count
        CHECK(cimp::volSince(B, 56, 100) == 10 && !ev(B).any(), "missing volume in the history is not counted as smaller");
    }
    {   // RV on one bar and on two bars
        auto B = base(); B[56].h = 99; B[56].v = 400;
        auto T = ev(B);
        CHECK(T.r.bar == 56 && T.v.bar == 56, "RV same bar");
        CHECK(cimp::label(true, true, true, P) == "Cob+RV?", "label Cob+RV?");
        B[56].v = 100; B[55].v = 400;
        T = ev(B);
        CHECK(T.r.bar == 56 && T.v.bar == 55, "R on +1, V on the pivot");
    }
    {   // bear mirror
        auto B = mirror(base()); B[56].l = -99;                     // mirror of the +1 range thrust
        auto T = ev(B, -1);
        CHECK(T.pivot == 55 && T.r.bar == 56, "bear mirror: R on pivot-high + 1");
        CHECK(cimp::evaluate(B, 59, 1, P, false).pivot == -1, "bear leg: no bull pivot");
        auto C = mirror(base()); C[57].l = -100.5; C[57].h = -94.5;  // mirror of the rejection
        CHECK(!ev(C, -1).any(), "bear: impulse high not above the pivot low: rejected");
        C[57].h = -93.75; CHECK(ev(C, -1).r.bar == 57, "bear: high above the pivot low: accepted");
    }
    {   // decision time: bars after the CoB never change the tag (no look-ahead, no repaint)
        auto B = base(); B[56].h = 99;
        const auto T0 = ev(B);
        auto C = B;
        for (int k = 0; k < 30; ++k) { Bar b; b.t = 1060 + k; b.h = 200 + k; b.l = 50; b.v = 1e9; C.push_back(b); }
        const auto T1 = cimp::evaluate(C, 59, 1, P, false);
        CHECK(T0.r.bar == T1.r.bar && T0.r.n == T1.r.n && T0.v.bar == T1.v.bar && T0.pivot == T1.pivot && T0.r.x == T1.r.x, "extra later bars change nothing");
        CHECK(cimp::recLine(cimp::toRec(T0, B)) == cimp::recLine(cimp::toRec(T1, C)), "the record is byte-identical");
    }
    {   // (0.2.0) +C climax at the origin
        auto B = base(); B[54].v = 500;                                // bar 54 (pivot - 1) is red (o = high, c = low): a selling climax
        auto T = ev(B);
        CHECK(T.c.bar == 54 && T.cConf == 57 && T.v.bar < 0 && T.r.bar < 0, "C: red climax on pivot-1, confirmed by the 57 close above its high");
        CHECK(cimp::label(false, false, true, P, true) == "Cob+C?", "label Cob+C?");
        B[54].o = B[54].l; B[54].c = B[54].h;                          // green: not a selling climax
        CHECK(ev(B).c.bar < 0, "a green bar is no bull climax");
        auto C = base(); C[52].v = 500;                                 // pivot - 3: outside
        CHECK(ev(C).c.bar < 0, "pivot - 3 is outside the climax window");
        auto D = base(); D[54].v = 500; D[56].c = 95; D[57].c = 95.2; D[58].c = 95.2; D[59].c = 95.5;   // never closes ABOVE 95.5
        CHECK(ev(D).c.bar < 0, "no close above the climax high by the CoB bar: no C");
        D[59].c = 95.75; CHECK(ev(D).c.bar == 54 && ev(D).cConf == 59, "a close above it on the CoB bar itself counts");
        auto E = base(); E[55].o = 94; E[55].c = 92.5; E[55].v = 500;   // the pivot bar red + biggest volume -> V and C
        T = ev(E);
        CHECK(T.v.bar == 55 && T.c.bar == 55 && T.cConf == 56, "VC on the pivot bar");
        CHECK(cimp::label(T.r.bar >= 0, T.v.bar >= 0, true, P, T.c.bar >= 0) == "Cob+VC?", "label Cob+VC?");
        CHECK(cimp::code(true, false, true) == "RC" && cimp::code(true, true, true) == "RVC", "fixed order R, V, C");
        auto F = base(); F[57].o = 96; F[57].c = 94.5; F[57].v = 500;  // climax at pivot + 2; 58 closes 97 > 96
        T = ev(F); CHECK(T.c.bar == 57 && T.cConf == 58, "C at pivot + 2 confirmed after it");
        F[58].c = 95.5; F[59].c = 95.9;                                 // no close above 96 by bar 59
        CHECK(ev(F).c.bar < 0, "climax at pivot + 2 not confirmed yet");
        auto G = F; for (int k = 0; k < 10; ++k) { Bar b; b.t = 1060 + k; b.h = 120; b.l = 90; b.o = 90; b.c = 119; b.v = 100; G.push_back(b); }
        CHECK(cimp::evaluate(G, 59, 1, P, false).c.bar < 0, "a later confirmation (after the CoB bar) never adds C");
        auto M = mirror(base()); M[54].v = 500;                         // bear mirror: a GREEN climax near the pivot high
        T = ev(M, -1); CHECK(T.c.bar == 54 && T.cConf == 57, "bear C: green climax, then a close below its low");
        cimp::Rec r; r.trig = 9; r.side = 1; r.cT = 5; r.cn = 77; r.cx = 3.2; r.cConf = 7; cimp::Rec q;
        CHECK(cimp::parseRec(cimp::recLine(r), q) && q.c() && q.cn == 77 && q.cConf == 7, "record with C round trip");
        CHECK(cimp::parseRec("1|bull|2|3|4|5|1.00|0|0|0.00|1", q) && !q.c() && q.abs, "a 0.1.0 record (11 fields) still parses");
        cimp::HoverIn h; h.c = true; h.cn = 77; h.cx = 3.2; h.cTime = "8:30 AM"; h.cConfTime = "8:39 AM"; h.abs = true;
        auto L = cimp::hover(h, P); bool ok = false; for (auto& x : L) if (x.find("8:30 AM red bar was the biggest volume in 77 bars") != std::string::npos && x.find("8:39 AM bar closed above its high") != std::string::npos) ok = true;
        CHECK(ok && L[0].rfind("Cob+C?", 0) == 0, "hover explains the climax");
    }
    {   // modes and labels
        cimp::Params M;
        M.mode = cimp::MODE_ABS;    CHECK(cimp::label(true, false, false, M) == "Cob" && cimp::label(true, false, true, M) == "Cob+R?", "Abs mode");
        M.mode = cimp::MODE_ALWAYS; CHECK(cimp::label(true, false, false, M) == "Cob+R?", "Always mode");
        M.mode = cimp::MODE_OFF;    CHECK(cimp::label(true, true, true, M) == "Cob", "Off mode");
        M.mode = cimp::MODE_ALWAYS; M.proven = true; CHECK(cimp::label(true, true, false, M) == "Cob+RV", "proven drops the ?");
        CHECK(cimp::label(false, false, true, M) == "Cob", "no impulse = plain in every mode");
    }
    {   // the market file
        std::string why;
        auto Q = cimp::parseFile("VERSION,1\nMARKET,NQ\nMODE,always\nPROVEN,1\nPLAIN,0\nMIN_BARS,40\nWINDOW,3\n", "NQ", why);
        CHECK(why.empty() && Q.mode == cimp::MODE_ALWAYS && Q.proven && !Q.plain && Q.minBars == 40 && Q.window == 3, "file parsed");
        Q = cimp::parseFile("VERSION,1\r\nMARKET,NQ\r\nMODE,off\r\n", "NQ", why);
        CHECK(why.empty() && Q.mode == cimp::MODE_OFF, "CRLF file");
        Q = cimp::parseFile("VERSION,1\nMARKET,ES\nMODE,always\n", "NQ", why);
        CHECK(!why.empty() && Q.mode == cimp::MODE_ABS, "wrong market -> defaults");
        Q = cimp::parseFile("MARKET,NQ\nVERSION,1\n", "NQ", why);   CHECK(!why.empty(), "VERSION must be first");
        Q = cimp::parseFile("VERSION,1\nMARKET,NQ\nMODE,sometimes\n", "NQ", why); CHECK(!why.empty() && Q.mode == cimp::MODE_ABS, "bad MODE -> defaults");
        Q = cimp::parseFile("VERSION,1\nMARKET,NQ\nMIN_BARS,35x\n", "NQ", why); CHECK(!why.empty() && Q.minBars == 35, "garbled number -> defaults");
        Q = cimp::parseFile("VERSION,1\nMARKET,NQ\nMIN_BARS,5\n", "NQ", why);   CHECK(!why.empty(), "out of range");
        Q = cimp::parseFile("", "NQ", why); CHECK(why == "missing" && Q.mode == cimp::MODE_ABS, "empty = defaults");
        Q = cimp::parseFile(std::string(70000, 'x'), "NQ", why); CHECK(!why.empty(), "oversized");
        Q = cimp::parseFile("VERSION,1\nMARKET,NQ\n# comment\n\nMODE,abs\n", "NQ", why); CHECK(why.empty(), "comments / blank lines ok");
    }
    {   // records
        cimp::Rec r; r.trig = 1791945540; r.side = -1; r.pivot = 1791945000; r.legEnd = 1791945300; r.rT = 1791945180; r.rn = 100; r.rx = 2.43; r.abs = true;
        cimp::Rec q; CHECK(cimp::parseRec(cimp::recLine(r), q) && q.trig == r.trig && q.side == -1 && q.rn == 100 && q.r() && !q.v() && q.abs, "record round trip");
        CHECK(!cimp::parseRec("1|bull|2|3|4|5|6", q), "short record rejected");
        CHECK(!cimp::parseRec("x|bull|2|3|4|5|1.0|0|0|0.00|0", q), "garbled record rejected");
        CHECK(!cimp::parseRec("1|up|2|3|4|5|1.0|0|0|0.00|0", q), "bad side rejected");
        CHECK(!cimp::parseRec("1|bull|2|3|4|5|1.0|0|0|0.00|2", q), "bad abs rejected");
    }
    {   // clock / hover
        CHECK(cimp::clock12(8, 36) == "8:36 AM" && cimp::clock12(12, 3) == "12:03 PM" && cimp::clock12(0, 5) == "12:05 AM" && cimp::clock12(13, 0) == "1:00 PM", "12-hour clock");
        cimp::HoverIn h; h.r = true; h.rn = 100; h.rx = 2.4; h.rTime = "8:36 AM"; h.abs = true;
        auto L = cimp::hover(h, P);
        bool found = false; for (auto& s : L) if (s.find("8:36 AM bar was the biggest range in 100+ bars, 2.4x the average") != std::string::npos) found = true;
        CHECK(found && L[0].rfind("Cob+R?", 0) == 0 && L.size() <= 6, "hover text");
        h.abs = false; L = cimp::hover(h, P);
        CHECK(L[0].find("hidden by the Impulse tag setting") != std::string::npos, "hover says when the mode hides the tag");
    }
    {   // label placement inside the pane
        auto b = cimp::labelBox(100, 30, 1, 40, 14, 30, 0, 500);
        CHECK(b.t >= 1 && b.b - b.t == 14, "label clamped to the pane top");
        b = cimp::labelBox(100, 480, -1, 40, 14, 30, 0, 500);
        CHECK(b.b <= 499, "label clamped to the pane bottom");
    }
    // the real chart: NQ 2026-10-14 (ChartView export of his 3-min chart). IRT's cob_bull_3m_1b fired at 08:39 and 08:54.
    const char* fx = argc > 1 ? argv[1] : "nq_20261014.csv";
    std::ifstream in(fx);
    if (in) {
        std::vector<Bar> B; std::vector<std::string> T; std::string line; std::getline(in, line);
        while (std::getline(in, line)) {
            std::stringstream ss(line); std::string f[6]; for (int k = 0; k < 6; ++k) std::getline(ss, f[k], ',');
            Bar b; b.t = (long long)B.size(); b.o = std::stod(f[1]); b.h = std::stod(f[2]); b.l = std::stod(f[3]); b.c = std::stod(f[4]); b.v = std::stod(f[5]);
            B.push_back(b); T.push_back(f[0]);
        }
        int i839 = -1, i854 = -1, i833 = -1, i836 = -1, i839p = -1;
        (void)i836;
        for (size_t i = 0; i < T.size(); ++i) {
            if (T[i] == "2026-10-14 08:39:00") i839 = (int)i;
            if (T[i] == "2026-10-14 08:54:00") i854 = (int)i;
            if (T[i] == "2026-10-14 08:33:00") i833 = (int)i;
            if (T[i] == "2026-10-14 08:36:00") i836 = (int)i;
        }
        i839p = i839;
        auto A = cimp::evaluate(B, i839, 1, P, false);
        std::printf("NQ 08:39 CoB: pivot %s, R %s (%d bars, %.1fx), V %s (%d bars, %.1fx)\n", A.pivot >= 0 ? T[A.pivot].c_str() : "-",
                    A.r.bar >= 0 ? T[A.r.bar].c_str() : "-", A.r.n, A.r.x, A.v.bar >= 0 ? T[A.v.bar].c_str() : "-", A.v.n, A.v.x);
        CHECK(A.pivot == i833 && A.r.bar < 0 && A.v.bar == i833 && A.v.n >= 35, "NQ 08:39 = Cob+V? (volume 08:33; the 08:36 range 82.25 is only the biggest in 21 bars - the 07:33 news bar was 232.5)");
        CHECK(cimp::rangeSince(B, i836, 100) == 21, "08:36 range: biggest in 21 bars");
        auto C = cimp::evaluate(B, i854, 1, P, false);
        std::printf("NQ 08:54 CoB: pivot %s, tag %s\n", C.pivot >= 0 ? T[C.pivot].c_str() : "-", cimp::code(C.r.bar >= 0, C.v.bar >= 0, C.c.bar >= 0).c_str());
        CHECK(C.pivot == i839p && C.r.bar < 0 && C.v.bar < 0 && C.c.bar == i833 && C.c.n >= 35 && T[C.cConf] == "2026-10-14 08:42:00",
              "NQ 08:54 = Cob+C? (no thrust; the red 08:33 volume climax, closed above at 08:42)");
        CHECK(A.c.bar < 0, "NQ 08:39: the 08:33 climax was not yet closed above by 08:39 - no C");
    } else std::printf("(no NQ fixture %s - skipped)\n", fx);
    std::printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
