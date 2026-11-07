// CobSignalLogic.h (1.3.0, 2026-11-04: + LVP, low-volume pullback; 1.0.0 2026-11-01) - SDK-free logic of lsCobSignal (CobSignal.cpp): Rassul's CoB computed natively in IRT, so his
// own RTL CoB indicators (cob_bull / cob_bear) can come off the chart. Rassul 22:12-22:24 CT: "i want you to build the cob signals
// instead of me having my own", "consider the new levels also PHH and PHL", labels "Cob , Cob+R , Cob+V" (mockup cob_ba_mockup v2 "build").
//
// His locked rules (memory: trading.md, cob_bull / cob_bear RTL 2026-09-30 + 10-17 + 10-24), on CLOSED bars only:
//   ZigZag  his 1-bar ZigZag (the same walk as CobImpulseLogic::legPivot / trade_def.cob_live): up while highs extend, turns down on
//           a lower low (and the mirror). A leg = pivot to pivot.
//   bull    on bar j the up leg s -> e just ended with e = j - 1 (Ups#1 = 1), j an RTH bar ending at or before 11:00 CT, and
//           - the low of day (RTH, through j) SWEPT a level (ONL / PDL / PFL / LonLO / PHL): the LOD is below the level and the
//             up-leg high H[e] is back above it
//           - dratio = the leg / the average of the previous 20 up legs >= 1.9 (at least 5 earlier legs)
//           - the LOD printed after the previous up pivot (LOD < Ups#2) and is the 50-bar low (REF(LO,LOD) = MIN(LO,50))
//           - the leg has >= 3 bars (2026-10-24)
//           - room: close[j] - the leg's start low >= 0.1 ADR;  H[e] - LOD < 0.4 ADR
//   bear    the mirror with ONH / PDH / PFH / LonHI / PHH, the HOD, the down leg's low below the previous down pivot's low, no room
//           line and no time cutoff (his cob_bear).
//   ADR     20-day full-session true range (trade_def, min 10 sessions) - as CobImpulseLogic::adrAt.
//   levels  KeyLevelsLogic::compute (the Dealer Profile's own levels) as of each session's first RTH bar: PFH / PFL, PDH / PDL,
//           ONH / ONL, LonHI / LonLO and PHH / PHL (2026-11-01).
//   tag     CobImpulseLogic::evaluate (his approved +R / +V rule, 2026-10-14): a bar from the leg's pivot to 2 bars after it, inside
//           the pivot bar, with the biggest RANGE (R) / VOLUME (V) in >= 35 bars. Label Cob / Cob+R / Cob+V / Cob+RV.
// Decided once at the CoB bar's close; every decision is a record line; nothing after the bar is read (no repaint).
//
// (1.3.0, 2026-11-04, Rassul approved the LVP mockup irt_lvp.png - GC 3-min Nov 3 - and said "build") LVP = low-volume pullback after a CoB:
//   ZigZag  the 3-bar High / Low ZigZag of lsZzoCombo / lsBreakout's Pb = zzd::Engine::step with COMBO_REV_BARS (ZzoDeltaLogic.h, reused, not
//           re-written): an up wave turns down on a LOW below the previous 3 bars' lows (mirror), a strictly higher high / lower low moves
//           the extreme. A wave = the bars after the previous pivot through its own pivot (the ZZO Volume wave); range = previous pivot .. pivot.
//   CoB wave the ZigZag wave in the CoB's direction (bear = down wave, bull = up wave) that holds the CoB leg's extreme bar (the bar before
//           the CoB bar, Mark::legEnd); volume = the sum of its bars' volumes; range = its high .. low. (No such wave = no LVP for that CoB,
//           logged in the status file.)
//   pullback the next waves the OTHER way (after a bear CoB the up waves, after a bull CoB the down waves), the first 3 only, and only while
//           each stays inside the CoB wave's high .. low; the first one that leaves the range ends the look.
//   LVP     pullback volume <= CoB wave volume / 3 -> bear CoB: red "LVP" above the pullback's pivot high (no demand); bull CoB: green "LVP"
//           below its pivot low (no supply). Text only, Helvetica 10 bold (as the CoB labels), row 3 out from the wick (+51 px: the
//           wave-time label row 0 = +3..+17, lsCandleAbsorb A row 1 = +19..+33, lsBreakout Pb row 2 = +35..), never on another label.
//   Decided when the pullback wave is CONFIRMED (the ZigZag turned off it, on a closed bar); never repainted; one record line "LVP,...".
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "MarketKey.h"
#include "KeyLevelsLogic.h"
#include "CobImpulseLogic.h"
#include "ZzoDeltaLogic.h"   // (1.3.0) zzd::Engine::step = the 3-bar High / Low ZigZag (COMBO_REV_BARS) for LVP

namespace cobs {

static const char* const VERSION = "1.6.2";   // (1.6.2) the shared MarketKey.h lookup (IQFeed / Rithmic / CQG roots)
// was "1.6.1";   // (1.6.1) roots QGC / QHG / QCL / NGx mapped: the indicator never ran on his GC / HG / CL / NG charts
// was "1.6.0";   // (1.6.0, 2026-11-06 18:49-18:50 CT Rassul on GC / HG "this is the cob for gold, you need to use 3bar", "why does the cob still not on the gold and hg charts", and 2026-11-01 "the cob should be a completed wave, not mid way") the CoB = a COMPLETED 3-bar High / Low ZigZag wave (the ZZO Combo / chart ZigZag wave) from a swing low (bull) / high (bear) that swept a key level, decided when the ZigZag confirms the wave; the old 1-bar pause check is off (wave3)
// was "1.5.1";   // (1.5.1, 2026-11-06) CQG roots (EP / ENQ / CLE / GCE / CPE / NGE / EU6) -> ES / NQ / CL / GC / HG / NG / EU: the indicator never ran on his charts before
// was "1.5.0";   // (1.5.0, 2026-11-05 12:25-12:42 CT Rassul "build a line on top of the cob with targets", "lets use 1.272, 1.618 and 2.0", "an entry line at 50% retracement and a stop line at 0", line endings + "i'll go with your recommendations") the CoB Fib lines (fibLines / fibLayout)
// was "1.4.0";   // (1.4.0, 2026-11-05 10:17-10:21 CT Rassul "add a zzo wave filter of the cob having a wave that has at least a 10", "the filter should apply to both") a CoB is DRAWN only once its ZZO Combo wave count reaches 10
// was "1.3.0";   // (1.3.0, Rassul 2026-11-04 "build" after the LVP mockup) + LVP (low-volume pullback after a CoB, 3-bar ZigZag waves)   // was "1.2.0";   // (1.2.0, Rassul 2026-11-02 19:07-19:19 CT, his cob_bull_3m_1b / cob_bear_3m_1b + "11:00 am cutoff" + "build it so i can add it") = his two signals: + PWL / AsiaLO (bull) and PWH / AsiaHI (bear) as swept levels, PHL / PHH kept as extras; BOTH sides by 11:00 CT; the bear gets his room line (the leg-start high - close >= 0.1 ADR)   // was "1.1.0":   // (1.1.0, Rassul 2026-11-02 10:14 CT "ok.. lets add it .. build") a CoB whose ONLY swept level is PHH / PHL
// is drawn in ES / HG / GC (the year test: +0.61 / +0.18 / +0.16 R) and recorded but NOT drawn in NQ / CL / NG / 6E (+0.01 / -0.22 / -0.53 / -0.15 R) until the
// nightly proves it there; a CoB that also swept another level is drawn everywhere as before   // was "1.0.0"
inline bool phShown(const std::string& mkt) { return mkt == "ES" || mkt == "HG" || mkt == "GC"; }
#ifndef COBS_MAX_BARS
#define COBS_MAX_BARS 40000
#endif
static const int MAX_BARS = COBS_MAX_BARS;            // a longer history restarts the engine from the newest bars

struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0, v = 0; };   // t = bar END, local (CT) seconds
struct Mark {
    long long t = 0; int idx = -1; int side = 0;     // side +1 bull, -1 bear
    std::string levels; double dratio = 0, ext = 0, adr = 0; int legStart = -1, legEnd = -1;
    bool r = false, v = false; int rBar = -1, vBar = -1, rN = 0, vN = 0;
    bool phOnly = false, show = true;   // (1.1.0) PHH / PHL the only level swept; drawn (false = recorded only)
    // (1.4.0) the ZZO wave filter: the CoB's wave on the ZZO Combo engine (3-bar ZigZag, the forming wave after the running extreme);
    // zOk once its count reaches ZZO_COB_MIN at the CoB bar or a later bar of the same wave; drawn only then (decided once, never undone)
    int zCnt = -1; bool zOk = false, zDone = false; long long zAt = 0, zWave = 0; int zBars = 0;
    std::string label() const { std::string s = "Cob"; if (r || v) s += "+"; if (r) s += "R"; if (v) s += "V"; return s; }
};
struct Leg { int s = -1, e = -1, dir = 0; double size = 0; };
static const int ZZO_COB_MIN = 10;      // (1.4.0) the CoB wave's ZZO Combo count (Rassul: "at least a 10")
static const int ZZO_COB_LOOK = 60;     // (1.4.0) bars a CoB waits for its wave to reach the count (then: not drawn)

// (1.5.1, 2026-11-06 12:13 CT Rassul "why isnt' the cob there ?") his IRT feed is CQG: the chart's ROOT symbol is EP / ENQ / CLE / GCE / CPE /
// NGE / EU6 (as lsBigBidAsk / lsDealerHedge map them), not ES / NQ / CL / GC / HG / NG / EU. 1.0.0 - 1.5.0 kept the CQG root, svl::rthOf()
// did not know it, identify() returned false and the indicator never ran on his charts (no CoB, no status, no record, any market).
inline std::string mktKey(const std::string& root) { return mkey::norm(root); }
inline std::string mktKeyOld(const std::string& root) {
    static const char* const P[8][2] = { {"ENQ", "NQ"}, {"EP", "ES"}, {"CLE", "CL"}, {"GCE", "GC"}, {"CPE", "HG"}, {"NGE", "NG"}, {"EU6", "EU"}, {"6E", "EU"} };
    for (const auto& p : P) if (root == p[0]) return p[1];
    // (1.6.0) his gold / copper / crude charts carry the root QGC / QHG / QCL (the ZZO Combo writes ZzoCombo.status-QGC-180.txt), NG can be
    // NGX6: the CoB indicator never ran on GC / HG (no CobSignal.status-GC / HG file ever) - map them too
    static const char* const K[] = { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" };
    if (root.size() == 3 && root[0] == 'Q') { for (const char* k : K) if (root.compare(1, 2, k) == 0) return k; }
    if (root.size() >= 3 && root.compare(0, 2, "NG") == 0) return "NG";
    return root;
}
inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }

// ---- (1.3.0) LVP: low-volume pullback after a CoB (rules in the header comment)
static const int LVP_REV_BARS = zzd::COMBO_REV_BARS;   // the 3-bar High / Low ZigZag (lsZzoCombo / lsBreakout Pb)
static const int LVP_MAX_PB = 3;                       // the first 3 pullback waves only
static const long long LVP_DIV = 3;                    // pullback volume <= CoB wave volume / 3
struct LvpMark {
    long long t = 0;        // the pullback wave's pivot bar END (local secs) = the bar the label sits on
    int idx = -1, s = -1;   // pivot bar index / first bar of the pullback wave
    int side = 0;           // the CoB's side: -1 bear (red LVP above) / +1 bull (green LVP below)
    long long cobT = 0, waveT0 = 0, decT = 0;   // the CoB bar END, the pullback's first bar END, the bar that confirmed it
    long long vol = 0, cobVol = 0; int nth = 0; bool show = true;
};
struct LvpWave { int s = 0, e = 0, dir = 0; long long vol = 0, hi = 0, lo = 0; };   // hi / lo in ticks over previous pivot .. pivot
struct LvpTrack {
    long long cobT = 0; int cobIdx = -1, legEnd = -1, side = 0; bool show = true;
    int state = 0;          // 0 waiting for the CoB wave, 1 counting pullbacks, 2 done
    int cw = -1; long long hi = 0, lo = 0, vol = 0; int n = 0, lvp = 0; std::string why = "waiting for the CoB wave";
};
struct Lvp {
    double tick = 0.25;
    std::vector<zzd::Bar> Z; zzd::Walk W; std::vector<LvpWave> waves; std::vector<LvpTrack> tr; std::vector<LvpMark> marks;
    void reset() { Z.clear(); W = zzd::Walk(); waves.clear(); tr.clear(); marks.clear(); }
    long long tk(double px) const { long long q = 0; return zzd::toTicks(px, tick, q) ? q : 0; }
    // a CoB decided on bar idx (its leg's extreme = legEnd): call BEFORE bar(idx)
    void cob(long long t, int idx, int legEnd, int side, bool show) { LvpTrack x; x.cobT = t; x.cobIdx = idx; x.legEnd = legEnd; x.side = side; x.show = show;
        for (size_t k = waves.size(); k-- > 0;) {                           // (1.6.0) the CoB = a completed wave: its wave is already here
            const LvpWave& w = waves[k]; if (w.e < legEnd) break;
            if (w.dir == side && w.s <= legEnd && w.e >= legEnd) { x.state = 1; x.cw = (int)k; x.hi = w.hi; x.lo = w.lo; x.vol = w.vol; x.why = "counting pullbacks"; break; } }
        tr.push_back(x); }
    // closed bar j (= the CoB engine's index); true = a new LVP was decided on it
    bool bar(const Bar& b) {
        zzd::Bar z; z.t = b.t; z.h = tk(b.h); z.l = tk(b.l); if (z.h < z.l) std::swap(z.h, z.l);
        z.vol = std::isfinite(b.v) && b.v > 0 ? (long long)std::llround(std::min(b.v, 9e15)) : 0; z.vap = true;
        Z.push_back(z); const int j = (int)Z.size() - 1;
        int s = 0, e = 0, d = 0;
        if (!zzd::Engine::step(Z, W, j, s, e, d, LVP_REV_BARS)) return false;
        LvpWave w; w.s = s; w.e = e; w.dir = d;
        for (int i = s; i <= e; ++i) w.vol += Z[(size_t)i].vol;
        const int p0 = s > 0 ? s - 1 : s; w.hi = Z[(size_t)p0].h; w.lo = Z[(size_t)p0].l;
        for (int i = p0; i <= e; ++i) { w.hi = std::max(w.hi, Z[(size_t)i].h); w.lo = std::min(w.lo, Z[(size_t)i].l); }
        waves.push_back(w);
        return onWave((int)waves.size() - 1, j);
    }
    bool onWave(int k, int j) {
        const LvpWave& w = waves[(size_t)k]; bool made = false;
        for (LvpTrack& x : tr) {
            if (x.state == 2) continue;
            if (x.state == 0) {
                if (w.e < x.legEnd) continue;                                   // a wave before the CoB leg's extreme
                if (w.dir == x.side && w.s <= x.legEnd) { x.state = 1; x.cw = k; x.hi = w.hi; x.lo = w.lo; x.vol = w.vol; x.why = "counting pullbacks"; }
                else { x.state = 2; x.why = "the CoB leg's extreme is not in a ZigZag wave its way"; }
                continue;
            }
            if (k <= x.cw || w.dir != -x.side) continue;                        // only the pullback waves after the CoB wave
            if (w.hi > x.hi || w.lo < x.lo) { x.state = 2; x.why = "pullback " + std::to_string(x.n + 1) + " left the CoB wave's range"; continue; }
            ++x.n;
            if (x.vol > 0 && w.vol * LVP_DIV <= x.vol) {
                bool dup = false; for (const LvpMark& m : marks) dup = dup || m.idx == w.e;
                if (!dup) {
                    LvpMark m; m.t = Z[(size_t)w.e].t; m.idx = w.e; m.s = w.s; m.side = x.side; m.cobT = x.cobT; m.waveT0 = Z[(size_t)w.s].t; m.decT = Z[(size_t)j].t;
                    m.vol = w.vol; m.cobVol = x.vol; m.nth = x.n; m.show = x.show; marks.push_back(m); ++x.lvp; made = true;
                }
            }
            if (x.n >= LVP_MAX_PB) { x.state = 2; x.why = "3 pullbacks seen"; }
        }
        return made;
    }
};
inline std::string lvpLine(const LvpMark& m) {
    char b[300];
    std::snprintf(b, sizeof b, "LVP,%lld,%s,%lld,%lld,%lld,%lld,%lld,%.1f,%d,%s", m.t, m.side > 0 ? "BULL" : "BEAR", m.cobT, m.waveT0, m.decT, m.vol, m.cobVol,
                  m.cobVol > 0 ? 100.0 * (double)m.vol / (double)m.cobVol : 0.0, m.nth, m.show ? "SHOW" : "LOG");
    return b;
}

struct Engine {
    std::string mkt; double tick = 0.25; int openMin = 510, closeMin = 900, perSec = 180;
    std::vector<Bar> B; std::vector<klv::Bar> K; std::vector<cimp::Bar> CB; std::vector<svl::Cls> cls;
    // ZigZag state (the legPivot walk, incremental)
    int dirn = 0, ext = 0, start = 0; std::vector<Leg> legs;
    // per session
    struct Sess { double h = -1e300, l = 1e300, c = 0; bool rth = false; double rh = -1e300, rl = 1e300; int rhI = -1, rlI = -1; bool lv = false; std::map<std::string, double> L; double adr = 0; };
    std::map<long, Sess> S;
    std::vector<long> doneOrder;                    // completed sessions in order (for the ADR)
    std::vector<Mark> marks; std::vector<std::string> out;   // decisions + record lines not yet written
    long long trimmed = 0; std::string lastWhy;
    Lvp lvp; std::vector<std::string> lvpOut;        // (1.3.0) LVP decisions + record lines not yet written
    zzd::Engine Z3;                                  // (1.4.0) the ZZO Combo engine on the same bars (wave counts for the CoB filter)
    void reset() { wSeen = 0; lastWaveT = 0; if (!mkt.empty()) { Z3.reset(); Z3.revBars = zzd::COMBO_REV_BARS; Z3.setMarket(rootRaw.empty() ? mkt : rootRaw, perSec); } lvp.reset(); lvpOut.clear(); B.clear(); K.clear(); CB.clear(); cls.clear(); dirn = 0; ext = 0; start = 0; legs.clear(); lastLowPivot = -1; S.clear(); doneOrder.clear(); marks.clear(); out.clear(); lastWhy.clear(); }
    bool setMarket(const std::string& m, double tk, int spb) {
        mkt = mktKey(m); rootRaw = m; tick = tk > 0 ? tk : 0.25; perSec = spb > 0 ? spb : 180; lvp.tick = tick;
        Z3.reset(); Z3.revBars = zzd::COMBO_REV_BARS; Z3.setMarket(rootRaw, perSec);   // (1.4.0); (1.6.2) the chart's own root = exactly the ZZO Combo pane's counts
        return svl::rthOf(mkt.c_str(), openMin, closeMin);
    }
    double adrFor(long sess) const {               // 20 prior completed sessions' true range (>= 10)
        std::vector<long> prior; for (long s : doneOrder) if (s < sess) prior.push_back(s);
        if (prior.size() < 11) return 0;
        double sum = 0; int n = 0;
        for (size_t i = prior.size() >= 21 ? prior.size() - 20 : 1; i < prior.size(); ++i) {
            const Sess& a = S.at(prior[i]); const double pc = S.at(prior[i - 1]).c;
            sum += std::max(a.h - a.l, std::max(std::fabs(a.h - pc), std::fabs(a.l - pc))); ++n;
        }
        return n >= 10 ? sum / n : 0;
    }
    double avgPrev(int dir, size_t upto) const {     // the average of the previous 20 same-direction legs before legs[upto]
        double s = 0; int n = 0;
        for (size_t k = upto; k-- > 0 && n < 20;) if (legs[k].dir == dir) { s += legs[k].size; ++n; }
        return n >= 5 ? s / n : 0;
    }
    int prevPivotEnd(int dir, size_t upto) const { for (size_t k = upto; k-- > 0;) if (legs[k].dir == dir) return legs[k].e; return -1; }
    void addBar(const Bar& b) {
        if ((int)B.size() >= MAX_BARS) { trimmed += (long long)B.size(); reset(); }
        B.push_back(b);
        klv::Bar kb; kb.days = (long)floorDiv(b.t, 86400); kb.endSec = (int)(b.t - (long long)kb.days * 86400); kb.h = (float)b.h; kb.l = (float)b.l; kb.c = (float)b.c; kb.o = (float)b.o; kb.v = b.v;
        K.push_back(kb);
        cimp::Bar cb; cb.t = b.t; cb.o = b.o; cb.h = b.h; cb.l = b.l; cb.c = b.c; cb.v = b.v; CB.push_back(cb);
        const svl::Cls c = svl::classify(kb.days, kb.endSec, perSec, openMin, closeMin); cls.push_back(c);
        const int j = (int)B.size() - 1;
        // sessions: full high / low / last close; a new session closes the previous one (ADR)
        if (c.kind != svl::NONE) {
            if (!S.count(c.sess) && !S.empty()) { const long prev = S.rbegin()->first; if (prev < c.sess && (doneOrder.empty() || doneOrder.back() != prev)) doneOrder.push_back(prev); }
            Sess& s = S[c.sess]; s.h = std::max(s.h, b.h); s.l = std::min(s.l, b.l); s.c = b.c;
            if (c.kind == svl::RTH) {
                if (!s.rth) {                         // the first RTH bar: the session's key levels (as the Dealer Profile computes them), its ADR
                    s.rth = true; s.adr = adrFor(c.sess);
                    const std::vector<klv::Level> lv = klv::compute(K, perSec, openMin, closeMin, tick);
                    for (const klv::Level& x : lv) s.L[x.code] = x.price;
                    s.lv = true;
                }
                if (b.h > s.rh) { s.rh = b.h; s.rhI = j; }
                if (b.l < s.rl) { s.rl = b.l; s.rlI = j; }
            }
        }
        // 1) the CoB check on this bar with the ZigZag as it stood after the previous bar (trade_def.cob_live, 95% parity with his
        //    platform flags): Ups#1 = 1 = the bar before made the developing leg's extreme and this bar did not exceed it
        const size_t nm = marks.size();
        if (j >= 2 && !wave3) check(j, 1), check(j, -1);   // (1.6.0) the 1-bar pause CoB only with wave3 off
        // (1.3.0) LVP: the CoBs decided on this bar start a look; then this bar on the 3-bar ZigZag (it may confirm a pullback wave)
        for (size_t q = nm; q < marks.size(); ++q) { const Mark& m = marks[q]; lvp.cob(m.t, m.idx, m.legEnd, m.side, m.show); }
        { const size_t nl = lvp.marks.size(); lvp.bar(b); for (size_t q = nl; q < lvp.marks.size(); ++q) lvpOut.push_back(lvpLine(lvp.marks[q])); }
        zzoBar(b);                                     // (1.4.0) the ZZO Combo engine with this bar, then the pending CoBs' wave counts
        if (wave3) {                                   // (1.6.0) a 3-bar wave completed on this bar: is it a CoB?
            const size_t nm3 = marks.size();
            // the waves completed on this bar (Z3 trims its own bars / waves, so they are found by their end time, not by index)
            size_t k0 = Z3.W.size(); while (k0 > 0 && Z3.W[k0 - 1].e >= 0 && Z3.W[k0 - 1].e < (int)Z3.B.size() && Z3.B[(size_t)Z3.W[k0 - 1].e].t > lastWaveT) --k0;
            for (size_t k = k0; k < Z3.W.size(); ++k) { checkWave(k, j); if (Z3.W[k].e >= 0 && Z3.W[k].e < (int)Z3.B.size()) lastWaveT = Z3.B[(size_t)Z3.W[k].e].t; }
            for (size_t q = nm3; q < marks.size(); ++q) { const Mark& m = marks[q]; lvp.cob(m.t, m.idx, m.legEnd, m.side, m.show); }
        }
        // 2) the ZigZag (his 1-bar walk) with this bar
        if (j == 1) { dirn = B[1].h >= B[0].h ? 1 : -1; ext = 0; start = 0; }
        if (j >= 1) {
            if (dirn == 1) {
                if (B[j].h > B[ext].h) ext = j;
                else if (B[j].l < B[j - 1].l && j != ext) { Leg L; L.s = start; L.e = ext; L.dir = 1; L.size = B[ext].h - B[start].l; legs.push_back(L); start = ext; ext = j; dirn = -1; }
            } else {
                if (B[j].l < B[ext].l) ext = j;
                else if (B[j].h > B[j - 1].h && j != ext) { Leg L; L.s = start; L.e = ext; L.dir = -1; L.size = B[start].h - B[ext].l; legs.push_back(L); lastLowPivot = ext; start = ext; ext = j; dirn = 1; }
            }
        }
    }
    int lastLowPivot = -1;
    bool wave3 = true; size_t wSeen = 0; long long lastWaveT = 0; std::string rootRaw;                        // (1.6.0) the 3-bar completed-wave CoB (false = the 1.5.x 1-bar pause rule)
    // (1.6.0) THE CoB ON A COMPLETED 3-BAR WAVE. Wave k of the Combo engine (Z3, same bar index as B) was confirmed on bar j:
    //   start pivot p0 = the bar before its first bar (the previous ZigZag pivot), end pivot p1 = its extreme bar
    //   bull (up wave): p0's LOW is below a key level (ONL / PDL / PFL / LonLO / PWL / AsiaLO / PhrL) and p1's HIGH is back above it;
    //   bear the mirror (p0's HIGH above ONH / PDH / PFH / LonHI / PWH / AsiaHI / PhrH, p1's LOW back below it)
    //   size (p0 -> p1) >= 1.9 x the average of the previous 20 completed waves of its direction (>= 5 of them)
    //   p0 an RTH bar of j's session, p1 - p0 >= 3 bars, room |close[j] - p0's extreme| >= 0.1 ADR, j ends by 11:00 CT
    //   drawn only with its wave's ZZO Combo count >= 10 (the Combo's own count of that completed wave)
    // The label goes on bar j (the bar that completed the wave); the Fib lines run from p0 (0) to p1 (1.0). Decided once, never repainted.
    void checkWave(size_t k, int j) {
        if (k >= Z3.W.size() || j < 0 || j >= (int)B.size()) return;
        const zzd::Wave& w = Z3.W[k]; const int side = w.dir > 0 ? 1 : -1;
        const int nz = (int)Z3.B.size();
        if (w.s < 1 || w.e <= w.s - 1 || w.e >= nz) return;
        auto idxOf = [&](long long t) { int lo = 0, hi = (int)B.size() - 1; while (lo <= hi) { const int md = (lo + hi) / 2; if (B[(size_t)md].t == t) return md; if (B[(size_t)md].t < t) lo = md + 1; else hi = md - 1; } return -1; };
        const int p0 = idxOf(Z3.B[(size_t)(w.s - 1)].t), p1 = idxOf(Z3.B[(size_t)w.e].t);   // the ZigZag pivots on this engine's bars
        if (p0 < 0 || p1 <= p0) return;
        const svl::Cls& c = cls[(size_t)j]; const svl::Cls& c0 = cls[(size_t)p0];
        if (c.kind != svl::RTH || c0.kind != svl::RTH || c0.sess != c.sess || !S.count(c.sess)) return;
        const Sess& s = S[c.sess]; if (!s.lv) return;
        const double adr = s.adr; if (!(adr > 0)) return;
        if (p1 - p0 < 3) return;
        auto sizeOf = [&](const zzd::Wave& x) { const int a = x.s - 1; return (double)(x.dir > 0 ? Z3.B[(size_t)x.e].h - Z3.B[(size_t)a].l : Z3.B[(size_t)a].h - Z3.B[(size_t)x.e].l); };   // ticks
        double sum = 0; int n = 0;
        for (size_t q = k; q-- > 0 && n < 20;) { const zzd::Wave& x = Z3.W[q]; if (x.dir == w.dir && x.s >= 1 && x.e > x.s - 1 && x.e < nz) { sum += sizeOf(x); ++n; } }
        if (n < 5 || !(sum > 0)) return;
        const double dr = sizeOf(w) / (sum / n); if (!(dr >= 1.9)) return;
        const int mod = (int)((B[(size_t)j].t % 86400 + 86400) % 86400) / 60; if (mod > 11 * 60) return;
        auto lvl = [&](const char* nm, double& v) { auto it = s.L.find(nm); if (it == s.L.end()) return false; v = it->second; return true; };
        std::string sw; auto add = [&](const char* nm) { if (!sw.empty()) sw += "/"; sw += nm; };
        const double ext0 = side > 0 ? B[(size_t)p0].l : B[(size_t)p0].h, ext1 = side > 0 ? B[(size_t)p1].h : B[(size_t)p1].l;
        if (side > 0) {
            const char* N[] = { "ONL", "PDL", "PFL", "LonLO", "PWL", "AsiaLO", "PHL" };
            for (const char* nm : N) { double v; if (lvl(nm, v) && ext0 < v && v < ext1) add(nm); }
            if (sw.empty() || !(B[(size_t)j].c - ext0 >= 0.1 * adr)) return;
        } else {
            const char* N[] = { "ONH", "PDH", "PFH", "LonHI", "PWH", "AsiaHI", "PHH" };
            for (const char* nm : N) { double v; if (lvl(nm, v) && ext1 < v && v < ext0) add(nm); }
            if (sw.empty() || !(ext0 - B[(size_t)j].c >= 0.1 * adr)) return;
        }
        Mark m; m.t = B[(size_t)j].t; m.idx = j; m.side = side; m.dratio = dr; m.adr = adr; m.legStart = p0; m.legEnd = p1; m.ext = ext0; m.levels = sw;
        m.phOnly = sw == "PHH" || sw == "PHL"; m.show = !m.phOnly || phShown(mkt);
        cimp::Params P; const cimp::Tag T = cimp::evaluate(CB, j, side, P, false);
        m.r = T.r.bar >= 0; m.v = T.v.bar >= 0; m.rBar = T.r.bar; m.vBar = T.v.bar; m.rN = T.r.n; m.vN = T.v.n;
        m.zCnt = w.cnt + (w.cntCap ? 1 : 0); m.zBars = 0; m.zWave = Z3.B[(size_t)w.s].t;
        marks.push_back(m); out.push_back(recLine(m));
        marks.back().zAt = m.t; zzoDone(marks.back(), w.hasV && (w.cnt >= ZZO_COB_MIN || w.cntCap));
    }
    bool useNew = true;                                         // PHH / PHL (2026-11-01); false = his RTL level set only (parity tests)
    bool legacy = false;                                        // (1.2.0) parity with his OLD signals (recorded flags): bear without room / cutoff
    bool hisSet = true;                                         // (1.2.0) with useNew false: true = his 6 levels (+ PWx / Asia), false = the old 4
    void check(int j, int side) {
        if (dirn != side || ext != j - 1) return;
        const Bar& bj = B[(size_t)j];
        if (side > 0 ? !(bj.h <= B[(size_t)ext].h) : !(bj.l >= B[(size_t)ext].l)) return;
        const svl::Cls& c = cls[(size_t)j];
        if (c.kind != svl::RTH || !S.count(c.sess)) return;
        const Sess& s = S[c.sess]; if (!s.lv) return;
        const double adr = s.adr; if (!(adr > 0)) return;
        int nPrev = 0; double sum = 0;
        for (size_t k = legs.size(); k-- > 0 && nPrev < 20;) if (legs[k].dir == side) { sum += legs[k].size; ++nPrev; }
        int nAll = 0; for (const Leg& L : legs) if (L.dir == side) { ++nAll; if (nAll >= 5) break; }
        if (nAll < 5 || nPrev == 0) return;
        const int p0 = start, p1 = ext;
        if (cls[(size_t)p0].sess != c.sess) return;                 // the leg starts in this session
        const double size = side > 0 ? B[(size_t)p1].h - B[(size_t)p0].l : B[(size_t)p0].h - B[(size_t)p1].l;
        const double dr = size / (sum / nPrev); if (dr < 1.9) return;
        if (p1 - p0 < 3) return;                                    // >= 3 bars (2026-10-24)
        const int mod = (int)((bj.t % 86400 + 86400) % 86400) / 60;
        auto lvl = [&](const char* n, double& v) { auto it = s.L.find(n); if (it == s.L.end()) return false; v = it->second; return true; };
        std::string sw; auto add = [&](const char* n) { if (!sw.empty()) sw += "/"; sw += n; };
        Mark m; m.t = bj.t; m.idx = j; m.side = side; m.dratio = dr; m.adr = adr; m.legStart = p0; m.legEnd = p1;
        double mn = 1e300, mx = -1e300; for (int k = std::max(0, j - 49); k <= j; ++k) { mn = std::min(mn, B[(size_t)k].l); mx = std::max(mx, B[(size_t)k].h); }
        if (side > 0) {
            const double lod = s.rl;
            if (!(B[(size_t)p0].l <= lod)) return;                  // the leg starts at the LOD
            const double hi = B[(size_t)p1].h;
            const char* N[] = { "ONL", "PDL", "PFL", "LonLO", "PWL", "AsiaLO", "PHL" };   // (1.2.0) his set (6) + PHL
            for (int q = 0; q < (useNew ? 7 : (hisSet ? 6 : 4)); ++q) { double v; if (lvl(N[q], v) && lod < v && v < hi) add(N[q]); }
            if (sw.empty()) return;
            if (!(bj.c - B[(size_t)p0].l >= 0.1 * adr)) return;     // room
            if (!(hi - lod < 0.4 * adr)) return;
            if (mod > 11 * 60) return;                              // by 11:00 CT
            if (std::fabs(lod - mn) > 1e-9) return;                 // LOD = MIN(LO,50)
            m.ext = lod;
        } else {
            const double hod = s.rh;
            if (!(B[(size_t)p0].h >= hod)) return;                  // the leg starts at the HOD
            const double lo = B[(size_t)p1].l;
            const char* N[] = { "ONH", "PDH", "PFH", "LonHI", "PWH", "AsiaHI", "PHH" };   // (1.2.0) his set (6) + PHH
            for (int q = 0; q < (useNew ? 7 : (hisSet ? 6 : 4)); ++q) { double v; if (lvl(N[q], v) && lo < v && v < hod) add(N[q]); }
            if (sw.empty()) return;
            if (!legacy && !(B[(size_t)p0].h - bj.c >= 0.1 * adr)) return;     // (1.2.0) his bear room: the leg-start high - close >= 0.1 ADR
            if (!legacy && mod > 11 * 60) return;                              // (1.2.0) by 11:00 CT (his TIME <= 1100), both sides
            if (!(lastLowPivot >= 0 && lo < B[(size_t)lastLowPivot].l)) return;   // below the previous down pivot
            if (!(hod - lo < 0.4 * adr)) return;
            if (std::fabs(hod - mx) > 1e-9) return;                 // HOD = MAX(HI,50)
            m.ext = hod;
        }
        m.levels = sw;
        m.phOnly = sw == "PHH" || sw == "PHL"; m.show = !m.phOnly || phShown(mkt);   // (1.1.0)
        cimp::Params P; const cimp::Tag T = cimp::evaluate(CB, j, side, P, false);
        m.r = T.r.bar >= 0; m.v = T.v.bar >= 0; m.rBar = T.r.bar; m.vBar = T.v.bar; m.rN = T.r.n; m.vN = T.v.n;
        marks.push_back(m); out.push_back(recLine(m));
    }
    // (1.4.0) the ZZO wave filter. The forming wave of the CoB's colour on the Combo engine (lsZzoCombo 2.2.0 liveTail): when the walk
    // already runs the CoB's way, from the bar after its start pivot; else the bars after the running extreme (the provisional wave).
    void zzoBar(const Bar& b) {
        zzd::Bar zb; zb.t = b.t; zb.vap = true; zb.vol = (long long)std::llround(b.v);
        if (!zzd::toTicks(b.h, tick, zb.h) || !zzd::toTicks(b.l, tick, zb.l)) { zb.h = Z3.B.empty() ? 0 : Z3.B.back().h; zb.l = Z3.B.empty() ? 0 : Z3.B.back().l; }
        Z3.addBar(zb);
        const int j3 = (int)Z3.B.size() - 1; if (j3 < 1) return;
        for (Mark& m : marks) {
            if (m.zDone) continue;
            const zzd::Walk& z = Z3.Z; const int d = m.side > 0 ? 1 : -1;
            const int s0 = z.dirn == d ? (z.any ? z.start + 1 : z.start) : z.ext + 1;
            if (++m.zBars > ZZO_COB_LOOK) { zzoDone(m, false); continue; }
            if (s0 < 0 || s0 > j3) continue;                                  // the CoB's wave has no bar yet (the walk is still the other way at this bar)
            const long long wt = Z3.B[(size_t)s0].t;
            if (m.zWave == 0) m.zWave = wt;
            else if (wt != m.zWave) { zzoDone(m, false); continue; }          // its wave ended before reaching the count
            const zzd::Wave w = Z3.make(Z3.B, s0, j3, d, Z3.W.size(), true);
            if (!w.hasV) continue;
            m.zCnt = std::max(m.zCnt, w.cnt + (w.cntCap ? 1 : 0));
            if (w.cnt >= ZZO_COB_MIN || w.cntCap) { m.zAt = b.t; zzoDone(m, true); }
        }
    }
    void zzoDone(Mark& m, bool ok) {
        m.zDone = true; m.zOk = ok;
        char r[160]; std::snprintf(r, sizeof r, "ZZO,%lld,%s,%d,%s,%lld", m.t, m.side > 0 ? "BULL" : "BEAR", m.zCnt, ok ? "OK" : "NO", m.zAt);
        out.push_back(r);
    }
    std::string recLine(const Mark& m) const {
        char b[320];
        std::snprintf(b, sizeof b, "COB,%lld,%s,%s,%s,%.2f,%.6g,%.6g,%lld,%lld,%d,%d,%s", m.t, m.side > 0 ? "BULL" : "BEAR", m.label().c_str(), m.levels.c_str(), m.dratio, m.ext, m.adr,
                      B[(size_t)m.legStart].t, B[(size_t)m.legEnd].t, m.rN, m.vN, m.show ? "SHOW" : "LOG");   // (1.1.0) SHOW = drawn, LOG = recorded only
        return b;
    }
};

// ---- the drawing (SDK-free): the label under a bull CoB bar's low / over a bear's high, bull green / bear red, never over another label
struct View { int l = 0, t = 0, r = 800, b = 600, first = 0, last = 0; std::function<double(int)> X; std::function<double(double)> Y; std::function<int(const std::string&)> textW; };
struct Prim { int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned col = 0; std::string text; bool line = false; bool tiny = false; };   // (1.5.0) line = a horizontal line x0..x1 at y0; tiny = the Fib labels' smaller font
static const unsigned C_BULL = 0x0022C55E, C_BEAR = 0x00EF4444;   // 0x00RRGGBB
static const int GAP = 18;                                     // px from the bar's extreme (clear of Candle Absorb's codes on pivot bars)
static const int LVP_OFF = 3 + 3 * 16;                         // (1.3.0) = 51 px: row 3 out (wave-time label row 0, Candle Absorb A row 1, lsBreakout Pb row 2; 16 px rows)
inline std::vector<Prim> layout(const Engine& E, const View& v)
{
    std::vector<Prim> P; std::vector<std::pair<int, int>> used;   // label boxes (x0, y0) taken
    for (const Mark& m : E.marks) {
        if (!m.show || !m.zOk || m.idx < v.first || m.idx > v.last || m.idx >= (int)E.B.size()) continue;   // (1.1.0) LOG CoBs are not drawn; (1.4.0) nor a CoB whose ZZO wave count is under 10
        const Bar& b = E.B[(size_t)m.idx];
        const std::string s = m.label(); const int w = v.textW ? v.textW(s) : (int)s.size() * 7, h = 14;
        const double x = v.X(m.idx);
        int y = m.side > 0 ? (int)std::lround(v.Y(b.l)) + GAP : (int)std::lround(v.Y(b.h)) - GAP - h;
        for (int k = 0; k < 4; ++k) {                              // step away from another label at the same spot
            bool clash = false; for (auto& u : used) if (std::abs(u.first - (int)x) < w && std::abs(u.second - y) < h) clash = true;
            if (!clash) break;
            y += m.side > 0 ? h : -h;
        }
        if (y < v.t || y + h > v.b) continue;
        Prim p; p.x0 = (int)std::lround(x - w / 2.0); p.x1 = p.x0 + w; p.y0 = y; p.y1 = y + h; p.col = m.side > 0 ? C_BULL : C_BEAR; p.text = s;
        P.push_back(p); used.push_back(std::make_pair((int)x, y));
    }
    // (1.3.0) LVP: bear CoB -> red "LVP" above the pullback's pivot high, bull CoB -> green "LVP" below its pivot low, row 3 out from the
    // wick (LVP_OFF), one more row out if another label of this plugin is there (up to 2), never outside the pane
    for (const LvpMark& m : E.lvp.marks) {
        if (!m.show || m.idx < v.first || m.idx > v.last || m.idx >= (int)E.B.size()) continue;
        const Bar& b = E.B[(size_t)m.idx];
        const std::string s = "LVP"; const int w = v.textW ? v.textW(s) : 21, h = 14;
        const double x = v.X(m.idx); const bool below = m.side > 0;
        int y = below ? (int)std::lround(v.Y(b.l)) + LVP_OFF : (int)std::lround(v.Y(b.h)) - LVP_OFF - h;
        for (int k = 0; k < 3; ++k) {
            bool clash = false; for (auto& u : used) if (std::abs(u.first - (int)x) < w && std::abs(u.second - y) < h) clash = true;
            if (!clash) break;
            y += below ? 16 : -16;
        }
        if (y < v.t || y + h > v.b) continue;
        Prim p; p.x0 = (int)std::lround(x - w / 2.0); p.x1 = p.x0 + w; p.y0 = y; p.y1 = y + h; p.col = below ? C_BULL : C_BEAR; p.text = s;
        P.push_back(p); used.push_back(std::make_pair((int)x, y));
    }
    return P;
}

// ---- (1.5.0) the CoB Fib lines. Rassul 2026-11-05: targets 1.272 / 1.618 / 2.0, entry at the 50% retracement, stop at 0 (the CoB origin),
// lines end on a close back through the origin and at the session end, entry + stop gone after 3 h at most; his "go with your
// recommendations": entry + stop also end when the stop is touched (wick) or when T1 trades before the entry fills (missed), a hit target
// ends at its hit bar, a newer drawn CoB (either side) ends the older CoB's lines, CoBs are by 11:00 CT already (check), stop 1 tick beyond 0.
// Leg: bull = the leg-start bar's low (0) to the leg-end bar's high (1.0); bear the mirror. Only CoBs that are drawn (show + ZZO ok).
// Computed from closed bars only, so a finished line never changes; an open line runs to the live bar.
static const double FIB_K[3] = { 1.272, 1.618, 2.0 };
static const long long FIB_ENTRY_SECS = 3 * 3600;           // entry + stop: 3 h at most from the CoB bar
enum FibKind { F_STOP = 0, F_ENTRY = 1, F_ONE = 2, F_T1 = 3, F_T2 = 4, F_T3 = 5 };
struct FibSeg { int kind = 0; double px = 0; int i0 = -1, i1 = -1; bool open = false, hit = false; };
struct FibSet { long long cobT = 0; int side = 0; double origin = 0, one = 0; std::vector<FibSeg> seg; std::string why; };
inline bool fibDrawn(const Mark& m) { return m.show && m.zOk && m.legStart >= 0 && m.legEnd >= m.legStart; }
inline FibSet fibLines(const Engine& E, const Mark& m)
{
    FibSet F; F.cobT = m.t; F.side = m.side;
    const int n = (int)E.B.size();
    if (!fibDrawn(m) || m.idx < 0 || m.idx >= n || m.legEnd >= n || m.idx >= (int)E.cls.size()) { F.why = "not drawn"; return F; }
    const int sd = m.side > 0 ? 1 : -1;
    const double origin = sd > 0 ? E.B[(size_t)m.legStart].l : E.B[(size_t)m.legStart].h;
    const double one = sd > 0 ? E.B[(size_t)m.legEnd].h : E.B[(size_t)m.legEnd].l;
    const double L = (one - origin) * sd; if (!(L > 0)) { F.why = "no leg"; return F; }
    F.origin = origin; F.one = one;
    auto at = [&](double k) { return origin + sd * k * L; };
    const double stop = origin - sd * E.tick, entry = at(0.5), tk[3] = { at(FIB_K[0]), at(FIB_K[1]), at(FIB_K[2]) };
    int nextCob = -1;
    for (const Mark& o : E.marks) if (&o != &m && o.idx > m.idx && fibDrawn(o)) { if (nextCob < 0 || o.idx < nextCob) nextCob = o.idx; }
    const long sess = E.cls[(size_t)m.idx].sess;
    int endAll = -1, entryEnd = -1, beEnd = -1, tEnd[3] = { -1, -1, -1 }; bool filled = false;   // (13:08) beEnd: BE hit = the first bar after the entry fill that trades back to the CoB high
    for (int j = m.idx + 1; j < n && j < (int)E.cls.size(); ++j) {
        if (nextCob >= 0 && j >= nextCob) { endAll = j; F.why = "next CoB"; break; }
        if (E.cls[(size_t)j].kind != svl::RTH || E.cls[(size_t)j].sess != sess) { endAll = j - 1; F.why = "session end"; break; }
        const Bar& b = E.B[(size_t)j];
        const double fav = sd > 0 ? b.h : b.l, adv = sd > 0 ? b.l : b.h;
        if (entryEnd < 0) {
            if (!filled && (adv - entry) * sd <= 0) filled = true;
            if ((adv - stop) * sd <= 0) entryEnd = j;                           // stopped (a wick through the stop)
            else if (!filled && (fav - tk[0]) * sd >= 0) entryEnd = j;         // missed: T1 traded before the 50% filled
            else if (b.t - m.t >= FIB_ENTRY_SECS) entryEnd = j;                // 3 h
        }
        if (beEnd < 0 && filled && entryEnd != j && (fav - one) * sd >= 0) beEnd = j;   // (13:08 Rassul "keep the be line like the other green lines that have hit")
        for (int k = 0; k < 3; ++k) if (tEnd[k] < 0 && (fav - tk[k]) * sd >= 0) tEnd[k] = j;
        if ((b.c - origin) * sd < 0) { endAll = j; F.why = "closed through 0"; break; }
    }
    const bool open = endAll < 0; const int last = open ? n - 1 : std::max(m.idx, endAll);
    if (open) F.why = "live";
    auto add = [&](int kind, double px, int e, bool hit) { FibSeg g; g.kind = kind; g.px = px; g.i0 = m.legStart; g.hit = hit;
        if (e >= 0 && e <= last) { g.i1 = e; g.open = false; } else { g.i1 = last; g.open = open; } F.seg.push_back(g); };
    add(F_STOP, stop, entryEnd, false); add(F_ENTRY, entry, entryEnd, false); add(F_ONE, one, beEnd >= 0 ? beEnd : (!filled ? entryEnd : -1), beEnd >= 0 && beEnd <= last);   // (13:08) BE: green like a target, ends at its hit (no fill: with the entry)
    for (int k = 0; k < 3; ++k) add(F_T1 + k, tk[k], tEnd[k], tEnd[k] >= 0 && tEnd[k] <= last);
    return F;
}
static const unsigned C_ENTRY = 0x003B82F6, C_ONE = 0x006B7280;   // entry blue; (13:08) BE at the CoB high in the CoB colour like the targets (C_ONE no longer drawn); was grey = BE (Rassul 13:05 "add one more line for be which will be the cob high"); targets = the CoB's colour, stop = the other
inline const char* fibName(int k) { static const char* N[] = { "Stop", "50%", "BE", "1.272", "1.618", "2.0" }; return N[k >= 0 && k < 6 ? k : 0]; }
// lines (1 px, solid) from the CoB origin bar to the end bar (the live bar + 1 when open), the name right of the line's end, small
inline std::vector<Prim> fibLayout(const Engine& E, const View& v)
{
    std::vector<Prim> P;
    for (const Mark& m : E.marks) {
        if (!fibDrawn(m)) continue;
        const FibSet F = fibLines(E, m); if (F.seg.empty()) continue;
        for (const FibSeg& g : F.seg) {
            const int e = g.open ? g.i1 + 1 : g.i1;
            if (e < v.first || g.i0 > v.last + 1) continue;
            const int x0 = (int)std::lround(v.X(g.i0)), x1 = (int)std::lround(v.X(e)), y = (int)std::lround(v.Y(g.px));
            if (x1 <= x0 || y < v.t || y > v.b) continue;
            const unsigned col = g.kind == F_ENTRY ? C_ENTRY : g.kind == F_STOP ? (F.side > 0 ? C_BEAR : C_BULL) : (F.side > 0 ? C_BULL : C_BEAR);
            Prim L; L.line = true; L.x0 = x0; L.x1 = x1; L.y0 = L.y1 = y; L.col = col; P.push_back(L);
            const std::string s = fibName(g.kind); const int w = v.textW ? v.textW(s) : (int)s.size() * 6, h = 12;
            Prim T; T.tiny = true; T.x0 = x1 + 3; T.x1 = T.x0 + w; T.y0 = y - h / 2; T.y1 = T.y0 + h; T.col = col; T.text = s;
            if (T.x1 <= v.r && T.y0 >= v.t && T.y1 <= v.b) P.push_back(T);
        }
    }
    return P;
}
inline std::string fibStatus(const Engine& E)
{
    std::string o = "FIB_RULES,1.5.0 targets 1.272/1.618/2.0, entry 50%, stop 0 - 1 tick; end: close through 0, session end, next drawn CoB; entry+stop: stop touched, T1 before fill, 3 h; a hit target ends at its hit\n";
    int k = 0; for (size_t q = E.marks.size(); q-- > 0 && k < 4;) { const Mark& m = E.marks[q]; if (!fibDrawn(m)) continue; ++k;
        const FibSet F = fibLines(E, m); char b[200]; std::snprintf(b, sizeof b, "FIB,%lld,%s,0=%.6g,1=%.6g,%s", m.t, m.side > 0 ? "BULL" : "BEAR", F.origin, F.one, F.why.c_str());
        std::string l = b; for (const FibSeg& g : F.seg) { l += std::string(",") + fibName(g.kind) + (g.hit ? " hit" : g.open ? " open" : " ended"); } o += l + "\n"; }
    return o;
}

inline std::string statusText(const Engine& E, const std::string& sym, int spb, const std::string& rec)
{
    std::string o = std::string("VERSION,") + VERSION + "\nMARKET," + E.mkt + "\nSYMBOL," + sym + "\nBAR_SECONDS," + std::to_string(spb) +
                     "\nRTH," + std::to_string(E.openMin) + "-" + std::to_string(E.closeMin) + "\nBARS," + std::to_string(E.B.size()) + "\nLEGS," + std::to_string(E.legs.size()) +
                     "\nRULES,1-bar ZigZag; sweep ONH/ONL PDH/PDL PFH/PFL LonHI/LonLO PWH/PWL AsiaHI/AsiaLO PHH/PHL; dratio >= 1.9 (20 legs); >= 3 bars; 50-bar extreme; < 0.4 ADR; bull room 0.1 ADR, bull <= 11:00; PHH/PHL-only CoBs drawn in ES/HG/GC, recorded only elsewhere\n";
    o += "RECORD," + rec + "\nCOBS," + std::to_string(E.marks.size()) + "\n";
    {   int ok = 0, no = 0, wait = 0; for (const Mark& m : E.marks) { if (!m.zDone) ++wait; else if (m.zOk) ++ok; else ++no; }   // (1.4.0)
        o += "ZZO_FILTER,1.4.0 drawn only when the CoB's ZZO Combo wave count reaches " + std::to_string(ZZO_COB_MIN) + ",passed " + std::to_string(ok) + ",failed " + std::to_string(no) + ",waiting " + std::to_string(wait) + "\n"; }
    const size_t n = E.marks.size();
    for (size_t k = n > 8 ? n - 8 : 0; k < n; ++k) o += "LAST," + E.recLine(E.marks[k]) + "\n";
    // (1.3.0) LVP
    o += "LVP_RULES,3-bar High/Low ZigZag (zzd COMBO_REV_BARS " + std::to_string(LVP_REV_BARS) + "); CoB wave = the wave its way holding the CoB leg's extreme; first " +
         std::to_string(LVP_MAX_PB) + " pullback waves inside the CoB wave's range; LVP = pullback volume <= CoB wave volume / " + std::to_string(LVP_DIV) + "; decided when the pullback wave is confirmed\n";
    o += "LVPS," + std::to_string(E.lvp.marks.size()) + "\n";
    const size_t nl = E.lvp.marks.size();
    for (size_t k = nl > 8 ? nl - 8 : 0; k < nl; ++k) o += "LAST" + lvpLine(E.lvp.marks[k]) + "\n";
    o += fibStatus(E);   // (1.5.0)
    const size_t nt = E.lvp.tr.size();
    for (size_t k = nt > 4 ? nt - 4 : 0; k < nt; ++k) { const LvpTrack& x = E.lvp.tr[k]; o += "LVPLOOK," + std::to_string(x.cobT) + "," + (x.side > 0 ? "BULL" : "BEAR") + "," + std::to_string(x.vol) + "," + std::to_string(x.n) + "," + std::to_string(x.lvp) + "," + x.why + "\n"; }
    return o;
}

} // namespace cobs
