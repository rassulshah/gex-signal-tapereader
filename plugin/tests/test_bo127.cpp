// lsBreakout 1.27.0: FT label (ftPass + layout kind 6), Rev range >= 1.0 x ATR(13) required, rule 7 (bigger than a same-colour prior bar)
#include "BreakoutLogic.h"
#include <iostream>
using namespace bko;
static int fails = 0;
#define CK(x) do { if (!(x)) { ++fails; std::cout << "FAIL line " << __LINE__ << ": " #x "\n"; } } while (0)
static In bar(int k, double o, double h, double l, double c) { In b; b.t = 1000 + 180 * k; b.ok = true; b.o = o; b.h = h; b.l = l; b.c = c; b.vol = 100; return b; }
static Engine boEng(const std::vector<In>& B, int i0, int dir) { Engine E; E.tick = 0.25; E.zzRev = 3; E.bars = B; bba::BoDec D; D.fired = true; D.dir = dir; D.t0 = B[(size_t)i0].t; D.t = D.t0; E.bo[D.t0] = D; return E; }
static std::vector<In> mirror(std::vector<In> B) { for (In& b : B) { const double o = 200 - b.o, h = 200 - b.l, l = 200 - b.h, c = 200 - b.c; b.o = o; b.h = h; b.l = l; b.c = c; } return B; }
int main() {
    CK(std::string(VERSION) == "1.27.0"); CK(REV_ATR_X == 1.0);
    // ---- FT. Bars 0-2 lead in; bar 3 = the BO candle (high 104); the last bar is forming (never read)
    std::vector<In> base = { bar(0, 100, 101, 99.5, 100.5), bar(1, 100.5, 101.5, 100, 101), bar(2, 101, 102, 100.5, 101.5), bar(3, 101.5, 104, 101.25, 103.5) };
    auto run = [&](std::vector<In> tail, int dir = 1) { std::vector<In> B = base; for (In& b : tail) { b.t = 1000 + 180 * (int)B.size(); B.push_back(b); } B.push_back(bar((int)B.size(), 0, 0, 0, 0));
        if (dir < 0) B = mirror(B); Engine E = boEng(B, 3, dir); E.ftPass(); const long long t0 = B[3].t; struct R { long long ft; bool none; } r = { E.ft.count(t0) ? E.ft[t0] : 0, E.ftNone.count(t0) > 0 }; return r; };
    const long long T4 = 1000 + 180 * 4, T5 = 1000 + 180 * 5, T6 = 1000 + 180 * 6;
    for (int dir = 1; dir >= -1; dir -= 2) {
        { auto r = run({ bar(0, 103.5, 105, 103.25, 104.25) }, dir); CK(r.ft == T4 && !r.none); }                       // green, closes above the BO high
        { auto r = run({ bar(0, 103.5, 104.5, 103.25, 104.0) }, dir); CK(r.ft == T4); }                                // closes AT the BO high, high above it
        { auto r = run({ bar(0, 103.5, 104.0, 103.25, 104.0) }, dir); CK(r.ft == 0 && !r.none); }                      // closes at the BO high, high NOT above it: no FT (still open)
        { auto r = run({ bar(0, 103.5, 104.75, 103.25, 103.75) }, dir); CK(r.ft == 0); }                                // high above, close below the BO high: no
        { auto r = run({ bar(0, 104.75, 105, 104.0, 104.25) }, dir); CK(r.ft == 0); }                                   // closes above the BO high but RED: no
        { auto r = run({ bar(0, 103.5, 104.75, 103.25, 103.75), bar(0, 103.75, 104.5, 103.5, 104.25) }, dir); CK(r.ft == T5); }   // the second candle (no new high, wave not turned)
        { auto r = run({ bar(0, 103.5, 103.75, 103.0, 103.25), bar(0, 103.25, 103.5, 102.75, 103.0), bar(0, 103.0, 105, 102.9, 104.5) }, dir); CK(r.ft == T6); }   // three later, wave intact (lows above the 3 bars before)
        // the wave turns first (a low below the lows of the 3 candles before it, no new high): no FT, even if a later candle closes above
        { auto r = run({ bar(0, 103.5, 103.75, 99.75, 100.5), bar(0, 100.5, 105, 100.4, 104.5) }, dir); CK(r.ft == 0 && r.none); }
        // a candle that turns the wave is not an FT itself
        { auto r = run({ bar(0, 100.4, 104.0, 99.75, 104.0) }, dir); CK(r.ft == 0 && r.none); }
    }
    {   // kept once found (no repaint): the candles change afterwards, the FT stays; an unfired BO gets none
        std::vector<In> B = base; B.push_back(bar(4, 103.5, 105, 103.25, 104.25)); B.push_back(bar(5, 0, 0, 0, 0)); Engine E = boEng(B, 3, 1); E.ftPass(); CK(E.ft[B[3].t] == T4);
        E.bars[4] = bar(4, 103.5, 103.6, 103.25, 103.3); E.ftPass(); CK(E.ft[B[3].t] == T4);
        Engine U = boEng(B, 3, 1); U.bo.begin()->second.fired = false; U.ftPass(); CK(U.ft.empty() && U.ftNone.empty());
        // the forming candle is never an FT
        std::vector<In> F = base; F.push_back(bar(4, 103.5, 105, 103.25, 104.25)); Engine G = boEng(F, 3, 1); G.ftPass(); CK(G.ft.empty());
        // layout: green FT above the candle for a BO, red under it for a BD
        View v; v.first = 0; v.last = 5; v.area.l = 0; v.area.t = 0; v.area.r = 2000; v.area.b = 4000; v.X = [](int j) { return 100 + 20 * j; }; v.Y = [](double px) { return (int)(2000 - (px - 100) * 40); }; v.textW = [](const char* s, int) { return 7 * (int)std::string(s).size(); };
        E.bars = B; Scene S = layout(E, v); int nf = 0; for (const Txt& T : S.txt) if (T.kind == 6) { ++nf; CK(T.s == "FT" && T.bar == 4 && T.col == C_UP && T.f.b <= v.Y(105.0)); for (const Txt& o : S.txt) if (&o != &T) CK(!hits(T.f, o.f)); } CK(nf == 1 && S.ftShown == 1);
        std::vector<In> M = mirror(B); Engine D = boEng(M, 3, -1); D.ftPass(); Scene S2 = layout(D, v); nf = 0; for (const Txt& T : S2.txt) if (T.kind == 6) { ++nf; CK(T.col == C_DN && T.bar == 4 && T.f.t >= v.Y(200 - 105.0)); } CK(nf == 1);
    }
    // ---- Rev: 20 flat candles of range 2 (ATR 2), a Pb pivot LOW at bar 20, candidates after it
    auto revRun = [&](std::vector<In> tail, int dir) { std::vector<In> B; for (int k = 0; k < 20; ++k) B.push_back(bar(k, 100.5, 101.5, 99.5, 100.5));
        B.push_back(bar(20, 100.5, 100.75, 98.75, 99.25));                           // the pivot candle (red, range 2)
        for (In& b : tail) { b.t = 1000 + 180 * (int)B.size(); B.push_back(b); } B.push_back(bar((int)B.size(), 0, 0, 0, 0)); if (dir < 0) B = mirror(B);
        Engine E; E.tick = 0.25; E.bars = B; PbMark P; P.tp = B[20].t; P.tc = B[20].t; P.t0 = B[10].t; P.dir = dir; E.pb[P.tp] = P; E.revPass();
        return E.rev.count(P.tp) ? E.rev[P.tp].tr : 0LL; };
    const long long R21 = 1000 + 180 * 21, R22 = 1000 + 180 * 22;
    for (int dir = 1; dir >= -1; dir -= 2) {
        CK(revRun({ bar(0, 99.25, 101.5, 99.0, 101.25) }, dir) == R21);               // green, range 2.5 >= ATR, prior bar red: Rev
        CK(revRun({ bar(0, 99.25, 100.9, 99.0, 100.85) }, dir) == 0);                 // range 1.9 < ATR although it closes above the prior high: NO Rev (1.19.0 gave one)
        CK(revRun({ bar(0, 99.25, 101.0, 99.0, 100.9) }, dir) == R21);                // range exactly 1.0 x ATR (about): Rev
        // rule 7: the prior bar is green and BIGGER -> the next green candle is not a Rev; a bigger one after it is
        CK(revRun({ bar(0, 99.25, 102.0, 98.9, 101.9), bar(0, 101.9, 103.5, 101.0, 103.25) }, dir) == R21);   // the first one already qualifies (prior = red pivot)
        CK(revRun({ bar(0, 99.25, 100.5, 99.0, 100.4), bar(0, 100.4, 102.9, 99.9, 102.75) }, dir) == R22);    // small green (no Rev), then range 3.0 > its 1.5: Rev
        CK(revRun({ bar(0, 99.5, 102.5, 99.0, 99.25) , bar(0, 99.3, 101.6, 99.1, 101.5) }, dir) == R22);       // prior bar RED and bigger (3.5): no size test against it
    }
    {   // same colour prior bar that is bigger or equal blocks the Rev
        std::vector<In> B; for (int k = 0; k < 20; ++k) B.push_back(bar(k, 100.5, 101.5, 99.5, 100.5));
        B.push_back(bar(20, 99.0, 102.5, 98.75, 102.25));                            // the pivot candle is GREEN, range 3.75, closes high
        B.push_back(bar(21, 100.0, 102.4, 99.9, 102.3));                             // green, range 2.5 >= ATR, low below the pivot high, but smaller than the green bar before
        B.push_back(bar(22, 0, 0, 0, 0));
        Engine E; E.tick = 0.25; E.bars = B; PbMark P; P.tp = B[20].t; P.tc = B[20].t; P.dir = 1; E.pb[P.tp] = P; E.revPass();
        CK(E.rev.count(P.tp) && E.rev[P.tp].tr == B[20].t);                          // the pivot candle itself is the Rev (prior bar flat: not green)
        Engine F; F.tick = 0.25; F.bars = B; F.bars[20] = bar(20, 99.0, 102.5, 98.75, 98.9); F.bars[20].t = B[20].t;   // pivot candle red: bar 21 is the Rev
        F.pb[P.tp] = P; F.revPass(); CK(F.rev.count(P.tp) && F.rev[P.tp].tr == B[21].t);
        Engine G; G.tick = 0.25; G.bars = B; G.bars[19] = bar(19, 99.0, 103.5, 98.9, 103.0);                        // a bigger GREEN bar before the green pivot candle: the pivot candle fails rule 7, bar 21 fails it too
        G.pb[P.tp] = P; G.revPass(); CK(!G.rev.count(P.tp));
    }
    std::cout << (fails ? "FAILED " : "all passed ") << fails << "\n"; return fails ? 1 : 0;
}
