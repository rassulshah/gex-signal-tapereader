// VwapTrendLogic.h -- vwapTrend (VW100 2026-10-10, + STRONG level VW110 2026-10-10): "Strong Uptrend" / "Uptrend" / "Dntrend" /
// "Strong Dntrend" from the session VWAP and its SD bands, computed natively from the chart's own bars (no file, no SDK), with the
// SAME session rules and VWAP / SD arithmetic as lsSessionVWAP (SessionVWAPLogic.h: typical price (H + L + C) / 3 x volume from the
// session's first traded bar, weighted Welford SD).
//
// Rassul's rule ("you know the vwap rules"), applied to CLOSED bars only, from the 20th bar of each session (RTH or overnight):
//   Uptrend  if the +2SD band was touched (bar high >= VWAP + 2 SD of that bar) in the last 20 bars
//            OR the majority (more than 10) of the last 20 closes are above VWAP with no -1SD tag (low <= VWAP - 1 SD) in them
//   Dntrend  the mirror (-2SD touch, or the majority below with no +1SD tag)
//   when BOTH 2SD bands were touched in the window the LATEST touch wins (both on the same bar: that bar's close vs its VWAP)
//   otherwise no trend ("")
// STRONG (VW110, Rassul: "if more than half of the last 20 closes (11 or more) are above VWAP +1SD, and none of those 20 bars
// tagged the VWAP (no low at or below VWAP), then it's a strong uptrend, and vice versa"):
//   Strong Uptrend  11+ of the last 20 closes > VWAP + 1 SD (each bar's own band) AND no bar of the 20 has low <= VWAP
//   Strong Dntrend  11+ of the last 20 closes < VWAP - 1 SD AND no bar of the 20 has high >= VWAP
//   Strong overrides plain. Ladder: Strong Uptrend (+2) > Uptrend (+1) > none (0) > Dntrend (-1) > Strong Dntrend (-2).
//   With the default rule a strong state always implies the plain state on the same side (no VWAP tag => no opposite 1SD / 2SD
//   tag, and 11 closes above +1SD => 11 closes above VWAP), so 'via' still names the plain definition that qualified. Research
//   variants (StrongRule: 9 / 13 of 20, tag line VWAP -/+ 0.5 SD) can qualify strong without plain: then via = VIA_STRONG.
// The result records WHICH definition qualified (via) and the counts behind it, so Session Info can show the label and the hover
// can say why. label() keeps returning the plain "Uptrend" / "Dntrend" (SessionVWAP's status file uses it); displayLabel() is the
// 4-level text for Session Info, displayColor() green (+) / red (-).
// Tested by test_vwaptrend_logic.cpp (rules, every strong boundary, mirror symmetry, replay == live, Windows macro names) and
// vw110_trend_xcheck (the LRA Python twin lra.vwap_odds.trend_levels on 17 months of bars, all 7 markets).
#pragma once
#include <cmath>
#include <vector>
#include "SessionVWAPLogic.h"

namespace vtl {

static const int TREND_BARS = 20;
static const int STRONG_MIN = 11;      // closes beyond the 1 SD band out of the last 20 (Rassul: "11 or more")
enum Via { VIA_NONE = 0, VIA_MAJORITY = 1, VIA_2SD = 2, VIA_STRONG = 3 };
enum Strength { STR_NONE = 0, STR_PLAIN = 1, STR_STRONG = 2 };

// the strong rule's two knobs. Default = Rassul's rule; other values are research variants only (the nightly scores them, a
// better one becomes a PROPOSAL, never applied by itself).
struct StrongRule {
    int minCloses = STRONG_MIN;   // closes beyond +/-1 SD needed out of 20
    double tagSd = 0.0;           // the tag line, in SD toward the trend side: 0 = VWAP, +0.5 = VWAP + 0.5 SD for an uptrend
};                                //   (stricter), -0.5 = VWAP - 0.5 SD (looser); mirrored for a downtrend

struct Trend {
    int state = 0;          // +1 Uptrend side, -1 Dntrend side, 0 none (unchanged meaning since VW100)
    int via = VIA_NONE;     // which definition qualified
    int strength = STR_NONE;// VW110: 2 strong, 1 plain, 0 none
    int kind = svl::NONE;   // the session kind evaluated (svl::RTH or svl::ON)
    long sess = 0;          // the session (days since 1970 of its trading day)
    int bars = 0;           // closed bars of that session with a VWAP so far
    int nAbove = 0, nBelow = 0;          // closes above / below VWAP in the last 20 bars
    int nAbove1 = 0, nBelow1 = 0;        // VW110: closes above VWAP + 1 SD / below VWAP - 1 SD in the last 20 bars
    int up2Ago = -1, dn2Ago = -1;        // bars since the latest +2SD / -2SD touch in the window (-1 = none)
    bool tagUp1 = false, tagDn1 = false; // a +1SD / -1SD tag in the window
    bool tagVwDn = false, tagVwUp = false;// VW110: a low at / below the tag line (VWAP) / a high at / above it, in the window
    bool ready = false;                  // 20+ bars: the rule applies
};

// one closed bar of the session with its running VWAP and SD (the values drawn on that bar)
struct SBar { double h, l, c, vw, sd; };

// the rule on the last TREND_BARS of b[0..n-1] (n = closed bars of the session so far)
inline Trend evalWindow(const SBar* b, int n, const StrongRule& r = StrongRule())
{
    Trend t;
    t.bars = n;
    if (!b || n < TREND_BARS) return t;
    t.ready = true;
    int a = n - TREND_BARS, lu = -1, lw = -1;
    for (int k = a; k < n; k++) {
        const SBar& x = b[k];
        bool sdp = x.sd > 0 && std::isfinite(x.sd);
        if (sdp && x.h >= x.vw + 2.0 * x.sd) lu = k;
        if (sdp && x.l <= x.vw - 2.0 * x.sd) lw = k;
        if (sdp && x.h >= x.vw + x.sd) t.tagUp1 = true;
        if (sdp && x.l <= x.vw - x.sd) t.tagDn1 = true;
        if (x.c > x.vw) t.nAbove++;
        else if (x.c < x.vw) t.nBelow++;
        if (sdp && x.c > x.vw + x.sd) t.nAbove1++;
        if (sdp && x.c < x.vw - x.sd) t.nBelow1++;
        double off = (sdp ? r.tagSd * x.sd : 0.0);
        if (x.l <= x.vw + off) t.tagVwDn = true;            // uptrend's tag line: VWAP (+ tagSd SD)
        if (x.h >= x.vw - off) t.tagVwUp = true;            // downtrend's tag line: VWAP (- tagSd SD)
    }
    t.up2Ago = lu >= 0 ? n - 1 - lu : -1;
    t.dn2Ago = lw >= 0 ? n - 1 - lw : -1;
    if (lu >= 0 || lw >= 0) {
        if (lu != lw) t.state = lu > lw ? 1 : -1;
        else { const SBar& x = b[lu]; t.state = x.c > x.vw ? 1 : (x.c < x.vw ? -1 : 0); }
        t.via = t.state ? VIA_2SD : VIA_NONE;
    }
    else if (t.nAbove > TREND_BARS / 2 && !t.tagDn1) { t.state = 1; t.via = VIA_MAJORITY; }
    else if (t.nBelow > TREND_BARS / 2 && !t.tagUp1) { t.state = -1; t.via = VIA_MAJORITY; }
    t.strength = t.state ? STR_PLAIN : STR_NONE;
    // STRONG overrides plain (both sides cannot qualify: minCloses > 10 needs a majority; for research minCloses <= 10, the
    // two counts plus "no tag on the other side" are still exclusive because a close beyond +1 SD is a high above VWAP)
    int need = r.minCloses < 1 ? 1 : r.minCloses;
    bool su = t.nAbove1 >= need && !t.tagVwDn, sd_ = t.nBelow1 >= need && !t.tagVwUp;
    if (su != sd_) {
        int s = su ? 1 : -1;
        if (t.state != s) t.via = VIA_STRONG;             // only a research variant can get here (see the header comment)
        t.state = s;
        t.strength = STR_STRONG;
    }
    return t;
}

// From the chart's bars (stamped at their END, like svl::Bar): B[0..nClosed-1] are CLOSED bars (in IRT pass n - 1: the last
// bar is still forming). wantKind = svl::RTH or svl::ON; 0 = the kind of the last closed bar, where "after the close" (POST)
// means the day's RTH (its final state). Only the session of the last closed bar is read (backwards to its first bar), so the
// work per call is one session, not the whole chart.
inline Trend fromChart(const std::vector<svl::Bar>& B, int nClosed, int per, int openMin, int closeMin, int wantKind = 0,
                       const StrongRule& rule = StrongRule())
{
    Trend t;
    if (nClosed <= 0 || nClosed > (int)B.size() || per <= 0) return t;
    int last = nClosed - 1;
    svl::Cls cl = svl::classify(B[last].days, B[last].endSec, per, openMin, closeMin);
    if (cl.kind == svl::NONE) return t;
    int kind = wantKind ? wantKind : (cl.kind == svl::POST ? (int)svl::RTH : cl.kind);
    t.kind = kind; t.sess = cl.sess;
    int s = last;
    while (s > 0) {
        svl::Cls c = svl::classify(B[s - 1].days, B[s - 1].endSec, per, openMin, closeMin);
        if (c.sess != cl.sess || c.kind == svl::NONE) break;
        s--;
    }
    std::vector<SBar> sb;
    sb.reserve((size_t)(last - s + 1));
    svl::Acc acc;
    for (int i = s; i <= last; i++) {
        svl::Cls c = svl::classify(B[i].days, B[i].endSec, per, openMin, closeMin);
        if (c.kind != kind || c.sess != cl.sess) continue;
        const svl::Bar& x = B[i];
        if (!std::isfinite(x.h) || !std::isfinite(x.l) || !std::isfinite(x.c)) continue;
        acc.add(((double)x.h + (double)x.l + (double)x.c) / 3.0, x.v);
        if (!acc.ok()) continue;                                   // no VWAP before the first trade
        SBar y; y.h = x.h; y.l = x.l; y.c = x.c; y.vw = acc.vw; y.sd = acc.sd();
        sb.push_back(y);
    }
    Trend r = evalWindow(sb.empty() ? nullptr : &sb[0], (int)sb.size(), rule);
    r.kind = kind; r.sess = cl.sess;
    return r;
}

// the 5-step ladder: +2 Strong Uptrend, +1 Uptrend, 0 none, -1 Dntrend, -2 Strong Dntrend
inline int level(const Trend& t) { return t.state == 0 ? 0 : (t.strength == STR_STRONG ? 2 * t.state : t.state); }

// plain side label (unchanged since VW100; SessionVWAP 1.4.0 writes it to its status file)
inline const char* label(const Trend& t) { return t.state > 0 ? "Uptrend" : (t.state < 0 ? "Dntrend" : ""); }
// Session Info display text (VW110, exact strings agreed with Rassul)
inline const char* displayLabel(const Trend& t)
{
    switch (level(t)) {
        case 2: return "Strong Uptrend";
        case 1: return "Uptrend";
        case -1: return "Dntrend";
        case -2: return "Strong Dntrend";
        default: return "";
    }
}
// Session Info colour: green for any up level, red for any down level, 0 for none (0x00BBGGRR, like a Windows COLORREF / RGB())
static const unsigned int COLOR_UP = 0x0000B050u;    // RGB(80, 176, 0)  green  (bullish)
static const unsigned int COLOR_DN = 0x002828E0u;    // RGB(224, 40, 40) red    (bearish)
inline unsigned int displayColor(const Trend& t) { return t.state > 0 ? COLOR_UP : (t.state < 0 ? COLOR_DN : 0u); }
inline const char* viaLabel(const Trend& t)
{
    return t.via == VIA_2SD ? "2SD touch" : (t.via == VIA_MAJORITY ? "majority of 20 closes" : (t.via == VIA_STRONG ? "strong rule" : ""));
}
inline const char* strengthLabel(const Trend& t)
{
    return t.strength == STR_STRONG ? (t.state > 0 ? "11+ of 20 closes above +1SD, no VWAP tag" : "11+ of 20 closes below -1SD, no VWAP tag") : "";
}

} // namespace vtl
