// VwapTrendLogic.h -- vwapTrend (VW100, 2026-10-10): "Uptrend" / "Dntrend" from the session VWAP and its SD bands, computed
// natively from the chart's own bars (no file, no SDK), with the SAME session rules and VWAP / SD arithmetic as lsSessionVWAP
// (SessionVWAPLogic.h: typical price (H + L + C) / 3 x volume from the session's first traded bar, weighted Welford SD).
//
// Rassul's rule ("you know the vwap rules"), applied to CLOSED bars only, from the 20th bar of each session (RTH or overnight):
//   Uptrend  if the +2SD band was touched (bar high >= VWAP + 2 SD of that bar) in the last 20 bars
//            OR the majority (more than 10) of the last 20 closes are above VWAP with no -1SD tag (low <= VWAP - 1 SD) in them
//   Dntrend  the mirror (-2SD touch, or the majority below with no +1SD tag)
//   when BOTH 2SD bands were touched in the window the LATEST touch wins (both on the same bar: that bar's close vs its VWAP)
//   otherwise no trend ("")
// The result records WHICH definition qualified (via 2SD touch / via majority) and the counts behind it, so Session Info can show
// "Uptrend" next to "Regime: Positive" and the hover can say why.
// Tested by test_vwaptrend_logic.cpp (rules, edges, replay == live, Windows macro names) and vw100_trend_xcheck (the LRA study's
// Python rule on 17 months of bars, all 7 markets).
#pragma once
#include <cmath>
#include <vector>
#include "SessionVWAPLogic.h"

namespace vtl {

static const int TREND_BARS = 20;
enum Via { VIA_NONE = 0, VIA_MAJORITY = 1, VIA_2SD = 2 };

struct Trend {
    int state = 0;          // +1 Uptrend, -1 Dntrend, 0 none
    int via = VIA_NONE;     // which definition qualified
    int kind = svl::NONE;   // the session kind evaluated (svl::RTH or svl::ON)
    long sess = 0;          // the session (days since 1970 of its trading day)
    int bars = 0;           // closed bars of that session with a VWAP so far
    int nAbove = 0, nBelow = 0;          // closes above / below VWAP in the last 20 bars
    int up2Ago = -1, dn2Ago = -1;        // bars since the latest +2SD / -2SD touch in the window (-1 = none)
    bool tagUp1 = false, tagDn1 = false; // a +1SD / -1SD tag in the window
    bool ready = false;                  // 20+ bars: the rule applies
};

// one closed bar of the session with its running VWAP and SD (the values drawn on that bar)
struct SBar { double h, l, c, vw, sd; };

// the rule on the last TREND_BARS of b[0..n-1] (n = closed bars of the session so far)
inline Trend evalWindow(const SBar* b, int n)
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
    }
    t.up2Ago = lu >= 0 ? n - 1 - lu : -1;
    t.dn2Ago = lw >= 0 ? n - 1 - lw : -1;
    if (lu >= 0 || lw >= 0) {
        if (lu != lw) t.state = lu > lw ? 1 : -1;
        else { const SBar& x = b[lu]; t.state = x.c > x.vw ? 1 : (x.c < x.vw ? -1 : 0); }
        t.via = t.state ? VIA_2SD : VIA_NONE;
        return t;
    }
    if (t.nAbove > TREND_BARS / 2 && !t.tagDn1) { t.state = 1; t.via = VIA_MAJORITY; }
    else if (t.nBelow > TREND_BARS / 2 && !t.tagUp1) { t.state = -1; t.via = VIA_MAJORITY; }
    return t;
}

// From the chart's bars (stamped at their END, like svl::Bar): B[0..nClosed-1] are CLOSED bars (in IRT pass n - 1: the last
// bar is still forming). wantKind = svl::RTH or svl::ON; 0 = the kind of the last closed bar, where "after the close" (POST)
// means the day's RTH (its final state). Only the session of the last closed bar is read (backwards to its first bar), so the
// work per call is one session, not the whole chart.
inline Trend fromChart(const std::vector<svl::Bar>& B, int nClosed, int per, int openMin, int closeMin, int wantKind = 0)
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
    Trend r = evalWindow(sb.empty() ? nullptr : &sb[0], (int)sb.size());
    r.kind = kind; r.sess = cl.sess;
    return r;
}

inline const char* label(const Trend& t) { return t.state > 0 ? "Uptrend" : (t.state < 0 ? "Dntrend" : ""); }
inline const char* viaLabel(const Trend& t) { return t.via == VIA_2SD ? "2SD touch" : (t.via == VIA_MAJORITY ? "majority of 20 closes" : ""); }

} // namespace vtl
