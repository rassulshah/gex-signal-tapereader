// ZzoDeltaLogic.h - lsZzoCombo 2.1.1 (2026-11-03: count drawn inside the wave - view only, engine unchanged) / was lsZzoCombo 2.1.0 / lsZzoZigZag 1.1.0 (2026-11-03 14:00 CT, Rassul 13:59 "make it 3bar using high and low by default";
//   14:02 "remove weak" + the count: "why is there 50+ on green waves that are not bigger than the last 50+ green waves"; 14:03 "move the
//   count 1 more to the left"):
//   (1) ZigZag reversal bars per engine (Engine::revBars): Combo and the ZZO ZigZag overlay use COMBO_REV_BARS = 3 = IRT's own ZIGZAG
//       "Min Price Change = 3 bars, Use High/Low", the same rule as lsWavePOC 1.3.2 (WavePOCLogic.h addBar): an up wave turns down only
//       on a bar whose LOW is below the lows of the previous 3 bars (a down wave turns up on a HIGH above the previous 3 bars' highs);
//       a strictly higher high (lower low) moves the wave's extreme. ZZO Delta / ZZO Volume keep 1 bar (revBars default 1, unchanged).
//       There is no settings dialog on these plugins (SS_SAFE_DIALOG, "dialog settings never worked"), so the 1-bar / 3-bar choice is
//       the compiled constant COMBO_REV_BARS, not a chart setting.
//   (2) count = how many PRIOR waves of the SAME colour in a row this wave's volume RATIO (wave volume / its normal = the ramp height
//       the pane draws) beats, walking back over the whole history the engine holds (overnight included) and stopping at the first one
//       with a ratio >= this one; capped at CNT_CAP ("50+"). Waves with no normal yet (no ramp drawn) are skipped. Was raw volume, so a
//       normal RTH wave beat every quiet overnight wave and showed "50+" while its ramp was not the tallest.
//   (3) weak is still computed and logged in the status file but never drawn (ZzoComboView.h).
// was lsZzoDelta 1.3.2 / lsZzoVolume 1.2.2 / lsZzoCombo 2.0.0 (2026-11-03 09:00 CT, Rassul "build" - ZZO Combo): every wave
//   also carries cnt (the biggest wave of its colour in the last cnt waves of its colour, overnight included, capped at CNT_CAP) and xsh
//   (the share of its pivot bar's volume in the bar's extreme 2 ticks, from VAP); Ab heavy = Ab rule + xsh >= AB_HEAVY (ES 49% vs 33%
//   reached 2R, both halves); showAb per market from ZzoParams.h (ES on; the nightly turns other markets on). Delta / Volume unchanged.
// was 1.3.1 / 1.2.1 (2026-11-02 23:40 CT, Rassul "make sure everything is added to the self improvmeent
//   process"): the Ab? / Cmx / CoB-after-sweep thresholds are per market from ZzoParams.h (written by the nightly lra.zzo_score when its
//   judge promotes a challenger: 20+ new-day cases, Wilson lower bound above the champion; compiled in, nothing read at run time). The
//   header ships with the 1.3.0 numbers for every market, so the chart is unchanged until a promotion. No header = the 1.3.0 numbers.
// was 1.3.0 / 1.2.0 (2026-11-02 23:03 CT, Rassul "build out indicator and make sure it is ready for
//   tomorrow" after the CoB-after-sweep study, ES + GC Sep 14 - Nov 2, 127 first key-level sweeps):
//   CoB   on the wave right after a wave that made the FIRST RTH sweep of a key level (ONH/ONL, LonHI/LonLO, PDH/PDL, PFH/PFL, computed
//         natively from the chart's own bars): that reversal wave has delta >= 1.9x normal its way AND volume >= 1.5x AND size >= 1.5x
//         (sweep held the rest of the day 46% vs 28% for all sweeps, both halves; ES 38% vs 17%, GC 53% vs 39%) - coloured by its side
//   weak  the reversal wave after a sweep is light (volume < 1x normal) or its delta runs against it (held 14%) - grey
//   normals: when a session part has < 3 same-way waves yet, the RTH normals fall back to the previous RTH waves (sweeps happen at the
//   open and need a normal to compare with)
// was lsZzoVolume 1.1.0 (22:52 CT, Rassul "you can use AR to mean automatic rally in the classic wyckoff sequence of
//   looking for a cmx then ar the secondary test"): the CoB volume tag is renamed AR (automatic rally / reaction) and ST is added:
//     ST   secondary test: the wave right after an AR, the climax's way, on <= 0.7x the climax wave's volume, giving back >= 75% of the AR
//   (36-day ES: Cmx 30%, Cmx + AR 38%, ST 29% reach 2R vs the 31% base - descriptive Wyckoff tags, the nightly scores them)
// was lsZzoDelta 1.2.0 / lsZzoVolume 1.0.0 (2026-11-02 22:40 CT, Rassul "lets build, use delta as default and keep the Ab tag and add
//   the cmx and cob tags for volume ... remove lo tag"): the same engine also gives each wave's VOLUME (Weis Wave) - the "ZZO Volume"
//   indicator (ZzoVolume.cpp) draws wave volume / the median of the last 5 same-way waves, green up / red down waves, with tags:
//     Ab?  the delta absorption rules (as in ZZO Delta)
//     Cmx  climax: a wave in a clear trend making a new session high / low on volume >= 2.5x normal AND bigger than the last 5 same-way waves
//     CoB  change of behaviour: the wave right after a Cmx, the other way, with volume >= 1.5x normal
//   (36-day ES test: Cmx alone 30% reach 2R vs the 31% base, Cmx followed by a CoB wave 38% - descriptive tags, the nightly scores them)
// was 1.1.1 (21:48 CT, Rassul "show all the waves up because they are easier to compare": bars = |running total| up from one
//   bottom line, colour = buying / selling) - was 1.1.0 (2026-11-02 21:33 CT, Rassul "yes, fix it" after mockup "ZZO Absorption Mockup" v6):
//   (1) the STANDARD delta convention: + / green = net BUYING in the wave, - / red = net SELLING (buy - sell volume), bars coloured by
//       the sign of the running total (the wave's direction is the ZigZag - lsZzoZigZag on the price chart);
//   (2) Ab? absorption marks at a COMPLETED wave's end, coloured by the side that won (green = sellers absorbed under a down wave,
//       red = buyers absorbed over an up wave), decided once when the wave completes, never repainted; tested on ES 3m RTH 36 days
//       (Sep 14 - Nov 2, 1,333 waves; base 31.1% of pivots reach 2R): with-wave effort E = dir * delta, Er = E / median of the last 5
//         rule A  Er > 1.9 AND E >= |delta| of the prior OPPOSITE wave AND size <= 0.75 x that wave's size   (30 waves, 53% 2R, halves 45 / 58)
//         rule B  Er > 1.9 AND no new extreme past the prior SAME-WAY wave's pivot                           (105 waves, 39% 2R, halves 31 / 49)
//       (prior waves from the same session part; "?" = not yet proven at t > 3, the nightly keeps scoring them)
// was 1.0.0 (2026-11-02, Rassul 19:40-20:05 CT: "creat an indicator based on delta like zzo delta using high
// low and 1 bar reversal default. it shoud use 1 pane and should be green for upwave and red for down wave" -> "use a ratio instead"
// -> mockup "ZZO Delta Mockup" option 1 (median of the last 5) -> "ok lets do option 2" = the history restarts at the RTH open).
// SDK-free (tested: tests_zzodelta).
//   ZigZag   High / Low, 1-bar reversal: an up wave runs while highs extend and turns down on a bar with a LOWER LOW than the bar
//            before (the mirror for a down wave) - the same walk as CoB Signal (CobSignalLogic.h). A wave = the bars after the
//            previous pivot through its own pivot (the first wave on the chart starts at its first bar).
//   delta    each bar's buy - sell volume from IRT's VAP (summed over its rows); a wave's delta = the sum over its bars.
//   ratio    the wave's delta / the MEDIAN |delta| of the last 5 COMPLETED waves in the same direction in the same part of the day
//            (overnight / RTH / after the close, by the bar that made the wave's pivot); needs 3 such waves, so the first 3 up and
//            3 down waves after the RTH open show no ratio (option 2). 1.0x = a normal wave; plotted bar by bar as the wave's running
//            total / that median (green up wave, red down wave; below zero = delta against the wave). The label at the wave's end
//            is the ratio WITH the wave ("2.6x"; "-0.4x" = an up wave sold into / a down wave bought).
//   closed bars only, plus the forming bar live; the live wave's last bars move to the next wave when the 1-bar reversal flips
//   (as the ZigZag does). Completed waves never change.
#ifndef ZZO_DELTA_LOGIC_H
#define ZZO_DELTA_LOGIC_H
#include "MarketKey.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <iterator>
#include <map>
#include <string>
#include <vector>
#include "SessionVWAPLogic.h"
#if defined(__has_include)
#if __has_include("ZzoParams.h")
#include "ZzoParams.h"      // (1.3.1) per-market thresholds from the nightly self-improvement (lra.zzo_score)
#define ZZO_HAVE_PARAMS 1
#endif
#endif

namespace zzd {
static const char* const VERSION = "1.3.3";   // (1.3.3) roots via MarketKey.h (QGC / QHG / QCL / NGX6 ran with no session hours and no market params)
// was "1.3.2";
static const char* const VOL_VERSION = "1.2.3";   // lsZzoVolume
static const char* const COMBO_VERSION = "2.2.1";   // (2.2.1) roots via MarketKey.h: on GC / HG / CL / NG the session split (RTH hours) and the market params now apply
// was "2.2.0";   // (2.2.0, Rassul 2026-11-04 21:01 "zzo combo is still red while the zig zag turned up already .. they should be in sync", 21:14 "do it") the live tail splits at the live extreme: the bars after it show at once as the provisional opposite wave (repaints until the 3-bar reversal confirms), as the chart ZigZag draws its last leg   // was "2.1.1":   // lsZzoCombo (2.1.1: the count drawn inside the wave, ZzoComboView.h)
static const char* const ZIGZAG_VERSION = "1.2.1";   // (1.2.1) roots via MarketKey.h
// was "1.2.0";   // (1.2.0) the live leg the same way (low -> provisional up leg at once)   // was "1.1.0":  // lsZzoZigZag
static const bool LIVE_SPLIT = true;   // (Combo 2.2.0) see Engine::liveTail
static const int COMBO_REV_BARS = 3;   // (Combo 2.1.0) ZigZag reversal bars for lsZzoCombo + lsZzoZigZag: IRT ZIGZAG 3 bars, High/Low (1 = the 1-bar walk)
static const int CNT_CAP = 50;          // (Combo) count cap ("cap it at 50")
static const double AB_HEAVY = 0.04;    // (Combo) Ab drawn only with >= 4% of the pivot bar's volume in its extreme 2 ticks
static const double CNT_MIN_VOL = 2.0;  // (Combo) volume ratio, count and ratio label only on waves of 2.0x+ ("only on volume that is at least 2x")   // lsZzoVolume
static const int HIST = 5, MIN_HIST = 3, MAX_BARS = 6000, TRIM = 1000;
static const double AB_EFFORT = 1.9, AB_BACK = 0.75;   // (1.1.0) Ab? rule thresholds
static const double CMX_VOL = 2.5, COB_VOL = 1.5, ST_VOL = 0.7, ST_BACK = 0.75;
static const double SW_DELTA = 1.9, SW_VOL = 1.5, SW_SIZE = 1.5, SW_WEAK_VOL = 1.0;   // (1.3.0) CoB-after-sweep thresholds
static const char* const LV_NAMES[8] = { "ONH", "ONL", "LonHI", "LonLO", "PDH", "PDL", "PFH", "PFL" };   // (1.3.0) bit i = LV_NAMES[i]      // (1.2.0) Cmx / CoB volume thresholds (x the median of the last 5 same-way waves)
static const bool UP_ONLY = true;   // (1.1.0, Rassul 21:37 CT "show all the waves up because they are easier to compare") bars = |running total| from the bottom line, colour = buying / selling

inline bool toTicks(double px, double tick, long long& out) {
    if (!std::isfinite(px) || !(tick > 0)) return false;
    const double q = px / tick;
    if (!std::isfinite(q) || std::fabs(q) > 9e15) return false;
    out = (long long)std::llround(q); return true;
}
inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }

struct Bar { long long t = 0, h = 0, l = 0, dlt = 0; bool vap = false; long long seg = 0; long long vol = 0; long long xh = 0, xl = 0; bool xok = false; };   // (Combo) xh / xl = VAP volume at the bar's top / bottom 2 ticks   // (1.2.0) vol = the bar's volume   // t = bar END (local secs), prices in ticks
struct Wave { int s = 0, e = 0, dir = 0; long long dlt = 0, seg = 0; bool hasR = false; double med = 0; bool live = false;
              long long ext = 0, size = 0; int ab = 0;
              long long vol = 0; bool hasV = false; double medV = 0; bool cmx = false, cob = false, st = false;
              bool hasS = false; double medS = 0; int sw = 0, swLv = 0, cobTag = 0;
              int cnt = 0; bool cntCap = false; double xsh = -1; };   // (Combo 2.1.0) count = prior same-colour waves in a row with a smaller volume ratio (0 = the one before is >=; cntCap = >= CNT_CAP); xsh -1 = no footprint   // (1.3.0) size normal; sw = levels this wave swept; swLv = the sweep it answers; cobTag 1 CoB, 2 weak   // (1.1.0) cob = the AR tag; st = secondary test   // (1.2.0) wave volume / its normal, Cmx / CoB tags   // (1.1.0) pivot price / pivot-to-pivot size (ticks); ab 0 none, 1 rule A, 2 rule B
struct Walk { int dirn = 0, ext = 0, start = 0; bool any = false; };   // any = a wave completed already

inline bool medianOf(std::vector<long long> v, double& m) {
    if (v.empty()) return false;
    std::sort(v.begin(), v.end()); const size_t n = v.size();
    m = n % 2 ? (double)v[n / 2] : ((double)v[n / 2 - 1] + (double)v[n / 2]) / 2.0; return true;
}

// (Combo 2.1.0) the count: walking back over W[0 .. nW), only waves of w's colour that have a normal, how many in a row have a volume
// RATIO (vol / medV = the ramp height drawn) below w's; stops at the first >= one; CNT_CAP or more = cntCap ("50+")
inline void countBack(const std::vector<Wave>& W, size_t nW, Wave& w) {
    w.cnt = 0; w.cntCap = false;
    if (!w.hasV || !(w.medV > 0)) return;
    const double r = (double)w.vol / w.medV;
    for (size_t k = std::min(nW, W.size()); k-- > 0;) {
        const Wave& x = W[k]; if (x.dir != w.dir || !x.hasV || !(x.medV > 0)) continue;
        if ((double)x.vol / x.medV >= r) break;
        if (++w.cnt >= CNT_CAP) { w.cntCap = true; break; }
    }
}

inline bool zzoParamsBuilt() {
#ifdef ZZO_HAVE_PARAMS
    return true;
#else
    return false;
#endif
}

struct Engine {
    int openMin = 510, closeMin = 900, per = 180;
    // (1.3.1) this market's thresholds (setMarket: ZzoParams.h row, else the 1.3.0 numbers)
    double abEffort = AB_EFFORT, abBack = AB_BACK, cmxVol = CMX_VOL, swDelta = SW_DELTA, swVol = SW_VOL, swSize = SW_SIZE;
    int revBars = 1;       // (Combo 2.1.0) ZigZag reversal bars: 1 (beyond the previous bar; Delta / Volume) or COMBO_REV_BARS = 3 (Combo / ZigZag); kept by reset()
    bool showAb = false;   // (Combo) draw Ab on this market (ZzoParams.h; built-in: ES only)
    std::vector<Bar> B; Walk Z; std::vector<Wave> W; long long trimmed = 0; long long noVap = 0;
    // (1.3.0) per session (keyed by its day): high / low of all bars, RTH, overnight, London; levels already swept in RTH
    struct HL { bool any = false; long long h = 0, l = 0; void add(long long hi, long long lo) { if (!any || hi > h) h = hi; if (!any || lo < l) l = lo; any = true; } };
    struct Sess { HL full, rth, on, lon; int swept = 0; };
    std::map<long long, Sess> S;
    void reset() { B.clear(); Z = Walk(); W.clear(); trimmed = 0; noVap = 0; S.clear(); }
    static long long sessOf(long long seg) { return floorDiv(seg, 4); }
    void addSess(const Bar& b) {
        const int k = kindOf(b.seg); if (k == svl::NONE) return;
        Sess& s = S[sessOf(b.seg)]; s.full.add(b.h, b.l);
        if (k == svl::RTH) s.rth.add(b.h, b.l);
        if (k == svl::ON) {
            s.on.add(b.h, b.l);
            const long long days = floorDiv(b.t, 86400); const int sec = (int)(b.t - days * 86400);
            if (sec > 2 * 3600 && sec <= std::min(7 * 3600 + 1800, openMin * 60)) s.lon.add(b.h, b.l);   // London: bar END in (02:00, 07:30], before the RTH open
        }
        if (S.size() > 40) S.erase(S.begin());
    }
    // the 8 levels of a session (ticks); ok[i] false = not known yet
    void levels(long long sess, long long lv[8], bool ok[8]) const {
        for (int i = 0; i < 8; ++i) ok[i] = false;
        auto it = S.find(sess); if (it == S.end()) return;
        const Sess& s = it->second;
        if (s.on.any) { lv[0] = s.on.h; lv[1] = s.on.l; ok[0] = ok[1] = true; }
        if (s.lon.any) { lv[2] = s.lon.h; lv[3] = s.lon.l; ok[2] = ok[3] = true; }
        for (auto p = std::make_reverse_iterator(it); p != S.rend(); ++p) if (p->second.rth.any) { lv[4] = p->second.rth.h; lv[5] = p->second.rth.l; ok[4] = ok[5] = true; break; }
        for (auto p = std::make_reverse_iterator(it); p != S.rend(); ++p) if (p->second.full.any) { lv[6] = p->second.full.h; lv[7] = p->second.full.l; ok[6] = ok[7] = true; break; }
    }
    // (1.3.0) the FIRST RTH sweep of each level: a completed RTH wave whose pivot goes past a level its way while every earlier RTH bar
    // of the session stayed at / inside it
    void sweeps(Wave& w) {
        w.sw = 0;
        if (kindOf(w.seg) != svl::RTH || w.s < 0) return;
        const long long ss = sessOf(w.seg); long long lv[8]; bool ok[8]; levels(ss, lv, ok);
        bool any = false; long long hi = 0, lo = 0;
        for (int i = w.s - 1; i >= 0 && B[(size_t)i].seg == w.seg; --i) { if (!any || B[(size_t)i].h > hi) hi = B[(size_t)i].h; if (!any || B[(size_t)i].l < lo) lo = B[(size_t)i].l; any = true; }
        Sess& s = S[ss];
        for (int i = 0; i < 8; ++i) {
            if (!ok[i] || (s.swept & (1 << i))) continue;
            const bool high = (i % 2) == 0;
            if (high && w.dir > 0 && w.ext > lv[i] && (!any || hi <= lv[i])) { w.sw |= 1 << i; s.swept |= 1 << i; }
            if (!high && w.dir < 0 && w.ext < lv[i] && (!any || lo >= lv[i])) { w.sw |= 1 << i; s.swept |= 1 << i; }
        }
    }
    // (1.3.0) the reversal wave right after a sweep: CoB (heavy delta + volume + size its way) or weak (light, or delta against it)
    void cobAfterSweep(Wave& w, size_t nW) const {
        w.cobTag = 0; w.swLv = 0;
        if (nW == 0) return;
        const Wave& p = W[nW - 1];
        if (!p.sw || p.seg != w.seg || p.dir == w.dir) return;
        w.swLv = p.sw;
        if (!w.hasR || !w.hasV || !w.hasS) return;
        const double Er = (double)w.dir * (double)w.dlt / w.med, Vr = (double)w.vol / w.medV, Sr = (double)w.size / w.medS;
        if (Er >= swDelta && Vr >= swVol && Sr >= swSize) w.cobTag = 1;
        else if (Vr < SW_WEAK_VOL || Er < 0) w.cobTag = 2;
    }
    bool setMarket(const std::string& root, int spb) {
        per = spb > 0 ? spb : 180; const std::string m = mkey::norm(root);   // (1.3.3) any feed's root (QGC, @ES, GCE, 6E ...)
        abEffort = AB_EFFORT; abBack = AB_BACK; cmxVol = CMX_VOL; swDelta = SW_DELTA; swVol = SW_VOL; swSize = SW_SIZE; showAb = m == "ES";
#ifdef ZZO_HAVE_PARAMS
        for (int i = 0; i < ZZO_PARAMS_N; ++i) if (m == ZZO_PARAMS[i].m) {
            const ZzoParamRow& r = ZZO_PARAMS[i];
            auto ok = [](double v) { return std::isfinite(v) && v > 0 && v < 100; };   // a garbled row keeps the 1.3.0 numbers
            if (ok(r.abEffort) && ok(r.abBack) && ok(r.cmxVol) && ok(r.swDelta) && ok(r.swVol) && ok(r.swSize)) {
                abEffort = r.abEffort; abBack = r.abBack; cmxVol = r.cmxVol; swDelta = r.swDelta; swVol = r.swVol; swSize = r.swSize; showAb = r.showAb != 0; }
            break; }
#endif
        return svl::rthOf(m.c_str(), openMin, closeMin);
    }
    long long segOf(long long tEnd) const {   // overnight / RTH / after the close of one session = one history
        const long long days = floorDiv(tEnd, 86400); const int sec = (int)(tEnd - days * 86400);
        const svl::Cls c = svl::classify((long)days, sec, per, openMin, closeMin);
        return (long long)c.sess * 4 + c.kind;
    }
    static int kindOf(long long seg) { return (int)(((seg % 4) + 4) % 4); }
    // the median of f() over the last HIST completed waves with this direction and segment among W[0 .. nW); (1.3.0) an RTH segment
    // with fewer than MIN_HIST of its own fills in from the most recent same-way waves of earlier RTH segments
    template <class F> bool baseOf(int dir, long long seg, size_t nW, F f, double& med) const {
        std::vector<long long> v;
        for (size_t k = nW; k-- > 0 && (int)v.size() < HIST;) { const Wave& w = W[k]; if (w.dir == dir && w.seg == seg) v.push_back(f(w)); }
        if ((int)v.size() < MIN_HIST && kindOf(seg) == svl::RTH)
            for (size_t k = nW; k-- > 0 && (int)v.size() < HIST;) { const Wave& w = W[k]; if (w.dir == dir && w.seg < seg && kindOf(w.seg) == svl::RTH) v.push_back(f(w)); }
        if ((int)v.size() < MIN_HIST) return false;
        return medianOf(v, med) && med > 0;
    }
    bool baseFor(int dir, long long seg, size_t nW, double& med) const { return baseOf(dir, seg, nW, [](const Wave& w) { return w.dlt < 0 ? -w.dlt : w.dlt; }, med); }
    bool baseVol(int dir, long long seg, size_t nW, double& med) const { return baseOf(dir, seg, nW, [](const Wave& w) { return w.vol; }, med); }   // (1.2.0)
    bool baseSize(int dir, long long seg, size_t nW, double& med) const { return baseOf(dir, seg, nW, [](const Wave& w) { return w.size; }, med); }   // (1.3.0)
    Wave make(const std::vector<Bar>& bars, int s, int e, int dir, size_t nW, bool live) const {
        Wave w; w.s = s; w.e = e; w.dir = dir; w.live = live;
        for (int i = s; i <= e; ++i) { w.dlt += bars[(size_t)i].dlt; w.vol += bars[(size_t)i].vol; }
        w.seg = bars[(size_t)e].seg; w.hasR = baseFor(dir, w.seg, nW, w.med); w.hasV = baseVol(dir, w.seg, nW, w.medV);
        const int p0 = s > 0 ? s - 1 : 0;   // the previous pivot (the first wave starts at its own first bar)
        w.ext = dir > 0 ? bars[(size_t)e].h : bars[(size_t)e].l;
        w.size = dir > 0 ? bars[(size_t)e].h - bars[(size_t)p0].l : bars[(size_t)p0].h - bars[(size_t)e].l;
        w.hasS = baseSize(dir, w.seg, nW, w.medS);
        // (Combo) count back over the earlier waves of this colour (all session parts) until a bigger one; capped
        // (2.1.0) by the volume RATIO (the ramp height drawn), same colour only, skipping waves with no normal; stop at the first >= one
        countBack(W, nW, w);
        { const Bar& pb = bars[(size_t)e]; if (pb.xok && pb.vol > 0) w.xsh = (double)(dir > 0 ? pb.xh : pb.xl) / (double)pb.vol; }
        return w;
    }
    // the walk with bar j (1-bar = CobSignalLogic.h's): an up wave turns on a bar whose low is below the lows of the previous N bars
    // (N = 1: the bar before; N = 3: IRT's ZIGZAG "Min Price Change = 3 bars", as lsWavePOC), a down wave on a high above the previous
    // N bars' highs; a strictly more extreme bar moves the live extreme. true = a wave completed (s .. e, dir)
    static bool step(const std::vector<Bar>& b, Walk& z, int j, int& s, int& e, int& dir, int N = 1) {
        if (j == 1) { z.dirn = b[1].h >= b[0].h ? 1 : -1; z.ext = 0; z.start = 0; }
        if (j < 1) return false;
        if (N < 1) N = 1;
        const int j0 = std::max(0, j - N);
        if (z.dirn == 1) {
            if (b[(size_t)j].h > b[(size_t)z.ext].h) { z.ext = j; return false; }
            long long pl = b[(size_t)j0].l; for (int k = j0 + 1; k < j; ++k) pl = std::min(pl, b[(size_t)k].l);
            if (b[(size_t)j].l < pl && j != z.ext) { s = z.any ? z.start + 1 : z.start; e = z.ext; dir = 1; z.start = z.ext; z.ext = j; z.dirn = -1; z.any = true; return true; }
        } else {
            if (b[(size_t)j].l < b[(size_t)z.ext].l) { z.ext = j; return false; }
            long long ph = b[(size_t)j0].h; for (int k = j0 + 1; k < j; ++k) ph = std::max(ph, b[(size_t)k].h);
            if (b[(size_t)j].h > ph && j != z.ext) { s = z.any ? z.start + 1 : z.start; e = z.ext; dir = -1; z.start = z.ext; z.ext = j; z.dirn = 1; z.any = true; return true; }
        }
        return false;
    }
    void addBar(Bar b) {
        b.seg = segOf(b.t); if (!b.vap) ++noVap;
        B.push_back(b); const int j = (int)B.size() - 1;
        int s = 0, e = 0, d = 0;
        if (step(B, Z, j, s, e, d, revBars)) { Wave w = make(B, s, e, d, W.size(), false); w.ab = absorb(w, W.size()); volTags(w, W.size()); cobAfterSweep(w, W.size()); sweeps(w); W.push_back(w); }
        addSess(b);   // (1.3.0) after the wave check: a wave completed by this bar is judged on the bars before it
        if ((int)B.size() > MAX_BARS) trim(TRIM);
    }
    // (1.1.0) the Ab? rules on a completed wave against the completed waves W[0 .. nW) of its session part
    int absorb(const Wave& w, size_t nW) const {
        if (!w.hasR) return 0;
        const double E = (double)w.dir * (double)w.dlt, Er = E / w.med;
        if (!(Er > abEffort)) return 0;
        const Wave* ps = nullptr; const Wave* po = nullptr;
        for (size_t k = nW; k-- > 0 && (!ps || !po);) { const Wave& x = W[k]; if (x.seg != w.seg) continue; if (x.dir == w.dir) { if (!ps) ps = &x; } else if (!po) po = &x; }
        if (!ps || !po) return 0;
        const long long pd = po->dlt < 0 ? -po->dlt : po->dlt;
        if (pd > 0 && E / (double)pd >= 1.0 && po->size > 0 && (double)w.size / (double)po->size <= abBack) return 1;
        if ((double)w.dir * (double)(w.ext - ps->ext) <= 0) return 2;
        return 0;
    }
    // (1.2.0) Cmx: volume >= CMX_VOL x normal AND a new extreme past the prior same-way pivot; CoB: the wave right after a Cmx (the
    // previous completed wave, the other way, same session part) with volume >= COB_VOL x normal
    void volTags(Wave& w, size_t nW) const {
        w.cmx = w.cob = w.st = false;
        if (!w.hasV) return;
        const double Vr = (double)w.vol / w.medV;
        // (1.2.0, Rassul 23:00 CT "you need a clear down trend and then a new low with very heavy volume, bigger than prior downwaves ...
        // implement for bullish buying cmx too") Cmx = volume >= CMX_VOL x normal AND bigger than each of the last 5 same-way waves AND
        // a clear trend its way (the last 2 same-way pivots AND the last 2 opposite pivots each further its way) AND a new session
        // high / low (past every earlier bar of this session part). Both sides: selling climax (down) / buying climax (up).
        std::vector<const Wave*> same, opp;
        for (size_t k = nW; k-- > 0 && (same.size() < 5 || opp.size() < 2);) { const Wave& x = W[k]; if (x.seg != w.seg) continue; if (x.dir == w.dir) { if (same.size() < 5) same.push_back(&x); } else if (opp.size() < 2) opp.push_back(&x); }
        bool c = Vr >= cmxVol && same.size() >= 3 && opp.size() >= 2;
        if (c) for (const Wave* x : same) if (w.vol <= x->vol) { c = false; break; }
        if (c) c = (double)w.dir * (double)(same[0]->ext - same[1]->ext) > 0 && (double)w.dir * (double)(opp[0]->ext - opp[1]->ext) > 0;
        if (c && w.s >= 0) {
            bool any = false; long long ex = 0;
            for (int i = w.s - 1; i >= 0 && B[(size_t)i].seg == w.seg; --i) { const long long v = w.dir > 0 ? B[(size_t)i].h : B[(size_t)i].l; if (!any || (w.dir > 0 ? v > ex : v < ex)) ex = v; any = true; }
            c = any && (double)w.dir * (double)(w.ext - ex) > 0;
        }
        w.cmx = c;
        if (nW > 0) { const Wave& p = W[nW - 1]; if (p.cmx && p.dir != w.dir && p.seg == w.seg && Vr >= COB_VOL) w.cob = true; }   // AR
        if (nW > 1) { const Wave& a = W[nW - 1]; const Wave& c = W[nW - 2];   // (1.1.0) ST after Cmx + AR
            if (a.cob && c.cmx && c.dir == w.dir && a.seg == w.seg && c.seg == w.seg && (double)w.vol <= ST_VOL * (double)c.vol && a.size > 0 && (double)w.size >= ST_BACK * (double)a.size) w.st = true; }
    }
    void trim(int k) {
        if (k <= 0 || k >= (int)B.size() || Z.start < k) return;   // never cut the live wave
        B.erase(B.begin(), B.begin() + k);
        std::vector<Wave> hist, keep;
        for (Wave w : W) {
            if (w.e < k) { w.s = w.e = -1; hist.push_back(w); }            // before the cut: kept for the median only (delta stays)
            else { w.s = std::max(0, w.s - k); w.e -= k; keep.push_back(w); }
        }
        if (hist.size() > 40) hist.erase(hist.begin(), hist.end() - 40);
        hist.insert(hist.end(), keep.begin(), keep.end()); W.swap(hist);
        Z.ext -= k; Z.start -= k; trimmed += k;
    }
    // the waves to draw: the completed ones + the live tail (closed bars after the last pivot, and the forming bar when given;
    // a flip on the forming bar shows the provisional completed wave and the new live one)
    std::vector<Wave> view(const Bar* form) const {
        std::vector<Wave> out; for (const Wave& w : W) if (w.s >= 0) out.push_back(w);
        if (B.empty()) return out;
        if (form) {
            std::vector<Bar> b2 = B; Bar f = *form; f.seg = segOf(f.t); b2.push_back(f);
            Walk z = Z; int s = 0, e = 0, d = 0; const int j = (int)b2.size() - 1;
            Engine tmp; tmp.W = W;
            if (step(b2, z, j, s, e, d, revBars)) { Wave w = tmp.make(b2, s, e, d, tmp.W.size(), true); out.push_back(w); tmp.W.push_back(w); }
            const int ls = z.any ? z.start + 1 : 0;
            liveTail(tmp, b2, ls, j, z, out);
            return out;
        }
        const int j = (int)B.size() - 1, ls = Z.any ? Z.start + 1 : 0;
        liveTail(*this, B, ls, j, Z, out);
        return out;
    }
    // (Combo 2.2.0 / ZigZag 1.2.0, Rassul 2026-11-04 "they should be in sync") the live tail ls .. j: with the 3-bar walk (revBars > 1) it
    // ends at the live extreme z.ext and the bars after it are shown at once as the PROVISIONAL opposite wave (z.ext + 1 .. j), the way the
    // chart's ZigZag draws its last leg from the last pivot to the newest bar; a new extreme folds it back (repaint); the 3-bar reversal
    // confirms it (completed waves W unchanged). The 1-bar engines (ZZO Delta / Volume) keep one live wave ls .. j.
    void liveTail(const Engine& M, const std::vector<Bar>& b, int ls, int j, const Walk& z, std::vector<Wave>& out) const {
        if (j < 1 || ls > j) return;
        if (LIVE_SPLIT && revBars > 1 && z.ext >= ls && z.ext < j) {
            const Wave a = M.make(b, ls, z.ext, z.dirn, M.W.size(), true); out.push_back(a);
            Engine tmp = M; tmp.W.push_back(a);                                      // the provisional wave is judged after the live one
            out.push_back(tmp.make(b, z.ext + 1, j, -z.dirn, tmp.W.size(), true));
            return;
        }
        out.push_back(M.make(b, ls, j, z.dirn, M.W.size(), true));
    }
};

// (Combo) effort / result: the wave's delta its own way vs normal over its size vs normal (result floored at 0.1); false = no effort
inline bool effortResult(const Wave& w, double& er) {
    if (!w.hasR || !w.hasS) return false;
    const double E = (double)w.dir * (double)w.dlt / w.med; if (!(E > 0)) return false;
    er = E / std::max((double)w.size / w.medS, 0.1); return std::isfinite(er);
}
inline bool abHeavy(const Wave& w) { return w.ab != 0 && w.xsh >= AB_HEAVY; }
// (1.1.0) the standard sign: + = net buying, - = net selling; size vs the median of the last 5 waves the same way
inline std::string ratioText(const Wave& w) {
    if (!w.hasR) return "-";
    const double r = (double)w.dlt / w.med; char b[24];
    const bool z = std::fabs(r) < 0.05; std::snprintf(b, sizeof b, "%s%.1fx", z ? "" : r < 0 ? "-" : "+", z ? 0.0 : std::fabs(r)); return b;
}
// (1.3.0) the level names of a sweep mask ("ONL/LonLO")
inline std::string lvText(int m) { std::string t; for (int i = 0; i < 8; ++i) if (m & (1 << i)) { if (!t.empty()) t += "/"; t += LV_NAMES[i]; } return t; }
// (1.2.0) the volume label: wave volume vs the median of the last 5 same-way waves ("2.1x")
inline std::string volText(const Wave& w) {
    if (!w.hasV) return "-";
    char b[24]; std::snprintf(b, sizeof b, "%.1fx", (double)w.vol / w.medV); return b;
}
// (1.1.0) the ZigZag points for the price-chart overlay: the first wave's start, every pivot, then the live leg's extreme
struct ZPt { int i = 0; long long px = 0; bool live = false; };
inline std::vector<ZPt> zigzag(const Engine& E, const Bar* form) {
    std::vector<ZPt> out; const std::vector<Wave> v = E.view(form); if (v.empty()) return out;
    std::vector<Bar> b = E.B; if (form) b.push_back(*form);
    const Wave& f = v.front();
    if (f.s >= 0 && f.s < (int)b.size()) { const int p0 = f.s > 0 ? f.s - 1 : f.s; ZPt z; z.i = p0; z.px = f.dir > 0 ? b[(size_t)p0].l : b[(size_t)p0].h; out.push_back(z); }
    for (const Wave& w : v) {
        int best = w.s;
        for (int i = w.s; i <= w.e && i < (int)b.size(); ++i) if (w.dir > 0 ? b[(size_t)i].h >= b[(size_t)best].h : b[(size_t)i].l <= b[(size_t)best].l) best = i;
        if (!w.live) best = w.e;
        ZPt z; z.i = best; z.px = w.dir > 0 ? b[(size_t)best].h : b[(size_t)best].l; z.live = w.live; out.push_back(z);
    }
    return out;
}

// ---- drawing (own pane) ----
struct Prim { enum K { FILL, TEXT } k = FILL; int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned long col = 0; std::string text; };
struct Colours { unsigned long up = 0x22C55E, down = 0xEF4444, zero = 0x334155, grey = 0x94A3B8; };
struct View {
    bool volume = false;   // (1.2.0) true = the ZZO Volume view (wave volume, Cmx / CoB tags)
    int l = 0, t = 0, r = 0, b = 0, first = 0, last = 0; double bw = 8;   // engine bar indexes of the visible range, pixels per bar
    std::function<double(int)> X; std::function<int(const std::string&)> textW; int textH = 12;
};
struct Scene { std::vector<Prim> p; int bars = 0, labels = 0, marks = 0, cmx = 0, cob = 0, st = 0, swCob = 0, swWeak = 0; double lo = 0, hi = 0; };
inline int ci(double v) { return (int)std::lround(std::max(-30000.0, std::min(30000.0, v))); }

inline Scene layout(const Engine& E, const std::vector<Wave>& waves, long long formDlt, const View& v, const std::string& head, long long formVol = 0) {
    Scene S; const Colours C;
    const int TH = v.textH, top = v.t + TH + 4 + TH + 2 + (UP_ONLY ? (TH + 3) * 2 : 0), bot = UP_ONLY ? v.b - 3 : v.b - TH - 2;   // (1.1.0) up-only: room for label + Ab? above   // header row, room for a label above / below
    // running value per bar of the visible waves
    struct Pt { int i; double val; int dir; bool buy; };
    std::vector<Pt> pts; double lo = 0, hi = 0;
    const int nB = (int)E.B.size();
    for (const Wave& w : waves) {
        if (!(v.volume ? w.hasV : w.hasR) || w.e < v.first || w.s > v.last) continue;
        double run = 0;
        for (int i = w.s; i <= w.e; ++i) {
            const long long d = v.volume ? (i < nB ? E.B[(size_t)i].vol : formVol) : (i < nB ? E.B[(size_t)i].dlt : formDlt);   // the forming bar = index nB
            run += (double)d / (v.volume ? w.medV : w.med);
            if (i < v.first || i > v.last) continue;
            const double pv = UP_ONLY ? std::fabs(run) : run; pts.push_back({ i, pv, w.dir, v.volume ? w.dir > 0 : run >= 0 }); lo = std::min(lo, pv); hi = std::max(hi, pv);   // (1.2.0) volume: colour = the wave's direction
        }
    }
    if (UP_ONLY) { lo = 0; if (hi < 1.0) hi = 1.0; }
    else if (hi - lo < 1.0) { hi = std::max(hi, 0.5); lo = std::min(lo, -0.5); }
    S.lo = lo; S.hi = hi;
    auto Y = [&](double x) { return (double)bot - (x - lo) / (hi - lo) * (double)(bot - top); };
    const int z = ci(Y(0));
    { Prim p; p.k = Prim::FILL; p.x0 = v.l; p.x1 = v.r; p.y0 = p.y1 = z; p.col = C.zero; S.p.push_back(p); }
    const double half = std::max(1.0, v.bw * 0.35);
    for (const Pt& q : pts) {
        const double xc = v.X(q.i); const int y = ci(Y(q.val));
        Prim p; p.k = Prim::FILL; p.x0 = ci(xc - half); p.x1 = std::max(p.x0, ci(xc + half) - 1); p.y0 = std::min(y, z); p.y1 = std::max(y, z);
        if (p.y1 == p.y0) p.y1 = p.y0 + 1;
        p.col = q.buy ? C.up : C.down; if (p.x1 >= v.l && p.x0 <= v.r) { S.p.push_back(p); ++S.bars; }   // (1.1.0) green = net buying so far
    }
    // labels at each wave's end (skipped when they would overlap the previous one)
    int lastR = -100000;
    for (const Wave& w : waves) {
        if (w.e < v.first || w.e > v.last) continue;
        const std::string t = v.volume ? volText(w) : ratioText(w); const int tw = v.textW(t); const double xc = v.X(w.e);
        // (1.2.0) the tags of a completed wave, stacked above its label: Ab? (coloured by the side that won), Cmx / CoB (volume view,
        // coloured by the wave's own direction)
        std::vector<std::pair<std::string, unsigned long>> tags;
        if (!w.live) {
            if (w.cobTag == 1) tags.push_back({ "CoB", w.dir > 0 ? C.up : C.down });   // (1.3.0) both views
            if (w.cobTag == 2) tags.push_back({ "weak", C.grey });
            if (w.ab) tags.push_back({ "Ab?", w.dir > 0 ? C.down : C.up });
            if (v.volume && w.cmx) tags.push_back({ "Cmx", w.dir > 0 ? C.up : C.down });
            if (v.volume && w.cob) tags.push_back({ "AR", w.dir > 0 ? C.up : C.down });
            if (v.volume && w.st) tags.push_back({ "ST", w.dir > 0 ? C.down : C.up });   // coloured by the side expected to take over
        }
        int x0 = ci(xc - tw / 2.0); if (x0 < v.l + 1) x0 = v.l + 1; if (x0 + tw > v.r - 1) x0 = v.r - 1 - tw;
        if (x0 <= lastR + 2 && tags.empty()) continue;   // (1.1.0) a tagged wave always keeps its label
        double end = v.volume ? (w.hasV ? (double)w.vol / w.medV : 0.0) : (w.hasR ? (double)w.dlt / w.med : 0.0); if (UP_ONLY) end = std::fabs(end);
        // clear of every bar under the label's width: above the highest (end >= 0) / below the lowest (end < 0)
        int yTop = z, yBot = z;
        for (const Pt& q : pts) { const double xc2 = v.X(q.i); if (xc2 + half < x0 - 1 || xc2 - half > x0 + tw + 1) continue; const int y = ci(Y(q.val)); yTop = std::min(yTop, y); yBot = std::max(yBot, y); }
        Prim p; p.k = Prim::TEXT; p.x0 = x0; p.x1 = x0 + tw; p.text = t;
        if (end >= 0) { p.y1 = std::max(yTop - 2, v.t + TH + 4 + TH + (UP_ONLY ? (TH + 3) * 2 : 0)); p.y0 = p.y1 - TH; } else { p.y0 = std::min(yBot + 2, v.b - TH); p.y1 = p.y0 + TH; }
        p.col = v.volume ? (!w.hasV ? C.grey : w.dir > 0 ? C.up : C.down) : (!w.hasR ? C.grey : w.dlt >= 0 ? C.up : C.down); S.p.push_back(p); ++S.labels; lastR = x0 + tw;
        int edge = end >= 0 ? p.y0 : p.y1;
        for (const auto& tg : tags) {
            const int aw = v.textW(tg.first) + 6; int ax = ci(xc - aw / 2.0); if (ax < v.l + 1) ax = v.l + 1; if (ax + aw > v.r - 1) ax = v.r - 1 - aw;
            Prim bx; bx.k = Prim::FILL; bx.x0 = ax; bx.x1 = ax + aw; bx.col = tg.second;
            if (end >= 0) { bx.y1 = edge - 2; bx.y0 = bx.y1 - TH - 1; edge = bx.y0; } else { bx.y0 = edge + 2; bx.y1 = bx.y0 + TH + 1; edge = bx.y1; }
            if (bx.y0 >= v.t + TH + 2 && bx.y1 <= v.b) {
                S.p.push_back(bx); Prim tx; tx.k = Prim::TEXT; tx.x0 = ax + 3; tx.x1 = ax + aw - 3; tx.y0 = bx.y0 + 1; tx.y1 = bx.y1; tx.col = 0xFFFFFF; tx.text = tg.first; S.p.push_back(tx);
                if (tg.first == "Ab?") ++S.marks; else if (tg.first == "Cmx") ++S.cmx; else if (tg.first == "AR") ++S.cob; else if (tg.first == "ST") ++S.st; else if (tg.first == "CoB") ++S.swCob; else ++S.swWeak;
            }
        }
    }
    { Prim p; p.k = Prim::TEXT; p.x0 = v.l + 4; p.y0 = v.t + 2; p.x1 = p.x0 + v.textW(head); p.y1 = p.y0 + TH; p.col = C.grey; p.text = head; S.p.push_back(p); }
    return S;
}

inline std::string statusText(const Engine& E, const std::string& mkt, int spb, const std::string& vap) {
    auto pf = [](double v) { char t[24]; std::snprintf(t, sizeof t, "%.2f", v); return std::string(t); };   // (1.3.1)
    std::string s = std::string("lsZzoDelta ") + VERSION + "\nMARKET," + mkt + "," + std::to_string(spb) + "\nRTH," + std::to_string(E.openMin) + "-" + std::to_string(E.closeMin) +
        "\nRULES,1-bar High/Low ZigZag; wave delta (+ buying / - selling) / median |delta| of the last 5 same-way waves in the same session part (>= 3; RTH restarts)\nVOL,Cmx clear trend + new session extreme + volume >= 2.5x normal and > the last 5 same-way waves; AR the next wave the other way >= 1.5x normal; ST the wave after an AR on <= 0.7x the climax volume giving back >= 75% of the AR\nCOB,after the first RTH sweep of ONH/ONL LonHI/LonLO PDH/PDL PFH/PFL: CoB = reversal wave delta >= 1.9x + volume >= 1.5x + size >= 1.5x; weak = volume < 1x or delta against\nAB,A effort > 1.9x and >= the prior opposite wave's, size <= 0.75x it; B effort > 1.9x and no new extreme past the prior same-way pivot\nPARAMS," + pf(E.abEffort) + "," + pf(E.abBack) + "," + pf(E.cmxVol) + "," + pf(E.swDelta) + "," + pf(E.swVol) + "," + pf(E.swSize) +
        (zzoParamsBuilt() ? ",ZzoParams.h" : ",built-in") + "\nBARS," +
        std::to_string(E.B.size()) + ",no VAP " + std::to_string(E.noVap) + ",VAP " + vap + "\nWAVES," + std::to_string(E.W.size()) + "\n";
    {   // (1.3.0) the last 12 CoB / weak decisions with the level that was swept and the current key levels
        std::vector<std::string> ev;
        for (size_t k = E.W.size(); k-- > 0 && ev.size() < 12;) { const Wave& w = E.W[k]; if (w.s < 0 || !w.cobTag) continue;
            char b[160]; std::snprintf(b, sizeof b, "SWEEP,%lld,%s %s after the sweep of %s\n", E.B[(size_t)w.e].t, w.dir > 0 ? "UP" : "DN", w.cobTag == 1 ? "CoB" : "weak", lvText(w.swLv).c_str()); ev.push_back(b); }
        for (size_t i = ev.size(); i-- > 0;) s += ev[i];
        if (!E.B.empty()) { long long lv[8]; bool ok[8]; E.levels(Engine::sessOf(E.B.back().seg), lv, ok); s += "LEVELS";
            for (int i = 0; i < 8; ++i) { s += ","; s += LV_NAMES[i]; s += " "; s += ok[i] ? std::to_string(lv[i]) + " ticks" : std::string("-"); } s += "\n"; }
    }
    int n = 0;
    for (size_t k = E.W.size(); k-- > 0 && n < 30; ++n) {
        const Wave& w = E.W[k]; if (w.s < 0) break;
        char b[320]; std::snprintf(b, sizeof b, "WAVE,%lld,%s,delta %lld,%s,vol %lld,%s%s%s%s%s%s%s%s\n", E.B[(size_t)w.e].t, w.dir > 0 ? "UP" : "DN", w.dlt, ratioText(w).c_str(), w.vol, volText(w).c_str(), w.ab == 1 ? ",Ab? A" : w.ab == 2 ? ",Ab? B" : "", w.cmx ? ",Cmx" : "", w.cob ? ",AR" : w.st ? ",ST" : "", w.sw ? ",swept " : "", w.sw ? lvText(w.sw).c_str() : "", w.cobTag == 1 ? ",CoB after " : w.cobTag == 2 ? ",weak after " : "", w.cobTag ? lvText(w.swLv).c_str() : ""); s += b;
    }
    return s;
}
}  // namespace zzd
#endif
