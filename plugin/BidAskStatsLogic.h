#ifndef LS_BIDASKSTATS_LOGIC_H
#define LS_BIDASKSTATS_LOGIC_H
// BidAskStatsLogic.h (1.5.0, 2026-10-13; 1.4.0 / 1.3.0 / 1.2.0 / 1.1.0 / 1.0.0 2026-10-13) -- SDK-free logic of lsBidAskStats.
// (1.5.0, approved by Rassul 4:49 PM CT "build"; SPEC /home/claude/aud/of140/SPEC.md) bars unchanged (the biggest bid / ask at one price,
// in lots); PACE = aggressive lots / s per side (VAP totals) vs the same-time norm; the SPEC message grammar
//   "Sellers hit the bid with a 240-lot (3.4x) fast (p 2.3x), swept 3 levels into PDL - price held: buyers absorbing."
// and the hover rows Sellers | Buyers: Lots (ratio), Pace x (thr), Orders N · avg X lots, Top order NN%, Sweep, Large, StopRun, Burst. Pure functions, unit-tested
// on Linux. (1.3.0) WHO IS ATTACKING: a message line + the info box (12:05) + spike labels (12:03); S / L letters from the order feed
// (OrderFlowFeed.h, shared with lsPullStats / lsStackStats). Rassul 2026-10-13: "i want to identify spikes", "include L for Bid Ask
// Stacks and Pull"; approved final-signals-mockup.png / three-panes-messages-mockup.png. See the 1.3.0 section at the end.
//
// Definitions (Rassul 2026-10-13 06:34):
//   bid volume at a price = contracts traded AT THE BID there in the bar (sellers hitting the bid) = VOLPROFILE.sellVolume
//   ask volume at a price = contracts traded AT THE ASK there in the bar (buyers lifting the offer) = VOLPROFILE.buyVolume
//   Biggest bid  (red bar)   = the largest bid volume at any single price (tick-grid key) in the candle.
//   Biggest ask  (green bar) = the largest ask volume at any single price in the candle.
//   BBid  = how many candles this biggest bid is the largest of: 1 + the run of prior (valid) bars whose biggest bid is
//           STRICTLY smaller, capped at the bars available and at 999 ("999+").  BAsk likewise.
//   BidRatio = biggest bid / mean biggest bid of the prior `ratioBars` (default 20) valid closed bars; needs >= 5 prior bars,
//           else n/a (also n/a when that mean is 0).  Shown with one decimal + "x" ("3.5x").  AskRatio likewise.
//   An unreadable bar (failed / missing VAP) is n/a: no histogram bar, excluded from counts and averages - never 0.
//   Ties inside a bar: the bid's price is the LOWEST of the tied prices, the ask's the HIGHEST (row order never matters).
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "OrderFlowFeed.h"

namespace bas {

static const char* const VERSION = "1.5.3";   // 1.5.3 (2026-10-14 CR5300): spike labels keep pace / lots (OrderFlowFeed 1.3.1); 1.5.2 (2026-10-14 CR4800): read box + numbers hover, Offset X / Y, white letters, one spike label, rare L; 1.5.1 (2026-10-13 9:40 PM): row order + Crowd, 10 pt labels, chart-wide hover; 1.5.0 (2026-10-13): pace, SPEC grammar, hover rows, options line; 1.4.0: stop run R?; 1.3.1: no white outline
static const int COUNT_CAP = 999, RATIO_MIN_PRIOR = 5, RATIO_DEFAULT = 20, RATIO_LO = 5, RATIO_HI = 200;
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";
static const int POS_DEFAULT = 0;   // Top left: the info box (12:05 the box stays; the message line is always top centre)
// (1.1.0) the Ratio line (removed in 1.2.0, Rassul 10:51 "forget the gray lines"): an old file's field is 0..100 tenths
static const int LINE_DEFAULT_TENTHS = 15, LINE_MAX_TENTHS = 100;
// (1.2.0) SPIKES. Rassul 10:47 "i want to identify spikes on the bid ask indicator and color them differently like lime and some
// other color. for example it is not only over 2x but it is bigger than the prior biggest bid and ask values by at least 1.5x".
// A bar's biggest bid (ask) is a SPIKE when, as of that bar (prior bars only):
//   (a) it is >= "Spike vs average" (default 2.0) x that side's average   (the same average the box ratio divides by), AND
//   (b) it is >= "Spike vs prior biggest" (default 1.5) x the biggest value of EITHER side (bid or ask) in the prior
//       "Spike lookback" (default 10) valid candles (this bar not included; n/a candles skipped, as every other count).
// Any of the three settings 0 = spikes off. Ask spike (buyers) = bright green, bid spike (sellers) = bright red (10:51).
static const int SPIKE_AVG_DEFAULT_TENTHS = 20, SPIKE_JUMP_DEFAULT_TENTHS = 15, SPIKE_LOOKBACK_DEFAULT = 10, SPIKE_LOOKBACK_MAX = 50;
struct SpikeCfg {
    int avgT = SPIKE_AVG_DEFAULT_TENTHS, jumpT = SPIKE_JUMP_DEFAULT_TENTHS, lookback = SPIKE_LOOKBACK_DEFAULT;
    bool on() const { return avgT > 0 && jumpT > 0 && lookback > 0; }
    bool operator==(const SpikeCfg& o) const { return avgT == o.avgT && jumpT == o.jumpT && lookback == o.lookback; }
    bool operator!=(const SpikeCfg& o) const { return !(*this == o); }
};
// the biggest of either side over the last n prior valid bars (prior = oldest first); 0 = none
inline long long priorMaxEither(const std::vector<long long>& pb, const std::vector<long long>& pa, int n) {
    long long mx = 0; if (n <= 0) return 0;
    const size_t k = pb.size() < (size_t)n ? pb.size() : (size_t)n;
    for (size_t i = pb.size() - k; i < pb.size(); ++i) { if (pb[i] > mx) mx = pb[i]; if (i < pa.size() && pa[i] > mx) mx = pa[i]; }
    return mx;
}
// exact at the boundaries: v / mean >= X  <=>  10 v >= tenths * mean;  v / priorMax >= J  <=>  10 v >= tenths * priorMax
inline bool isSpike(long long v, double mean, long long priorMax, const SpikeCfg& c) {
    if (!c.on() || v <= 0 || !(mean > 0) || priorMax <= 0) return false;
    return (double)v * 10.0 >= (double)c.avgT * mean && v * 10 >= (long long)c.jumpT * priorMax;
}

inline bool finite(double v) { return v == v && v < 1e300 && v > -1e300; }

// one VAP row as read from IRT (volumes already promoted to long long)
struct Row { double price; long long bid, ask; };

struct BarMax {
    bool ok = false;                 // false = n/a (failed / missing / malformed VAP)
    long long bid = 0, ask = 0;      // biggest bid / ask volume at one price
    long long totBid = 0, totAsk = 0;   // (1.5.0) the bar's whole volume at the bid / at the ask (the PACE: aggressive lots / s)
    double bidPx = 0, askPx = 0;     // the price where it traded (on the tick grid)
};

// tick key: price / tick rounded to the nearest whole tick; false for non-finite or out of range
inline bool tickKey(double price, double tick, long long& key) {
    if (!finite(price) || !finite(tick) || tick <= 0) return false;
    const double q = price / tick;
    if (!finite(q) || q > 9.0e15 || q < -9.0e15) return false;
    key = (long long)(q >= 0 ? std::floor(q + .5) : std::ceil(q - .5));
    return true;
}

// the bar's biggest bid / ask from its rows; rows on the same tick are summed.  Any malformed row (non-finite price,
// negative volume) makes the whole bar n/a - never a partial answer.  No rows = a valid bar with 0 / 0 (a bar that
// traded nothing); the adapter decides beforehand whether "no rows" means missing VAP.
inline BarMax maxOf(const std::vector<Row>& rows, double tick) {
    BarMax m;
    if (!finite(tick) || tick <= 0) return m;
    std::map<long long, std::pair<long long, long long> > at;
    for (size_t i = 0; i < rows.size(); ++i) {
        long long k = 0;
        if (rows[i].bid < 0 || rows[i].ask < 0 || !tickKey(rows[i].price, tick, k)) return m;
        std::pair<long long, long long>& v = at[k]; v.first += rows[i].bid; v.second += rows[i].ask;
        m.totBid += rows[i].bid; m.totAsk += rows[i].ask;
    }
    bool haveB = false, haveA = false; long long kb = 0, ka = 0;
    for (std::map<long long, std::pair<long long, long long> >::const_iterator it = at.begin(); it != at.end(); ++it) {
        if (!haveB || it->second.first > m.bid) { m.bid = it->second.first; kb = it->first; haveB = true; }        // ascending: first = lowest tie
        if (!haveA || it->second.second >= m.ask) { m.ask = it->second.second; ka = it->first; haveA = true; }     // >= : last = highest tie
    }
    m.bidPx = (double)kb * tick; m.askPx = (double)ka * tick; m.ok = true;
    return m;
}

// BBid / BAsk: prior = the earlier VALID bars' values, oldest first. 1 + the run of most recent prior values strictly smaller.
inline int countBiggest(const std::vector<long long>& prior, long long v, bool& capped) {
    int n = 1; capped = false;
    for (size_t i = prior.size(); i-- > 0;) {
        if (n >= COUNT_CAP) { capped = true; break; }
        if (prior[i] < v) ++n; else break;
    }
    return n;
}

// ratio vs the mean of the last `bars` prior valid values; ok = false with fewer than 5 prior or a zero mean
inline double ratioOf(const std::vector<long long>& prior, long long v, int bars, bool& ok, double* meanOut = nullptr) {
    ok = false; if (meanOut) *meanOut = 0;
    if (bars < RATIO_LO) bars = RATIO_LO;
    if (bars > RATIO_HI) bars = RATIO_HI;
    const size_t n = prior.size() < (size_t)bars ? prior.size() : (size_t)bars;
    if (n < (size_t)RATIO_MIN_PRIOR) return 0;
    double sum = 0;
    for (size_t i = prior.size() - n; i < prior.size(); ++i) sum += (double)prior[i];
    const double mean = sum / (double)n;
    if (!(mean > 0)) return 0;
    if (meanOut) *meanOut = mean;                          // the average AS OF this bar (prior bars only), for the spike rule
    ok = true; return (double)v / mean;
}

struct Stat {
    BarMax m;                                  // m.ok = false -> everything n/a
    int bBid = 0, bAsk = 0; bool bBidCap = false, bAskCap = false;
    bool bidRatioOk = false, askRatioOk = false; double bidRatio = 0, askRatio = 0;
    double bidMean = 0, askMean = 0;           // the averages the ratios divide by (0 = n/a): spike condition (a)
    long long priorMax = 0;                    // (1.2.0) the biggest bid or ask of the prior `lookback` valid bars (0 = none)
    bool bidSpike = false, askSpike = false;   // (1.2.0) decided as of this bar
};
// (1.2.0) the spike fields of a stat from its prior bars (oldest first)
inline void applySpike(Stat& s, const std::vector<long long>& pb, const std::vector<long long>& pa, const SpikeCfg& c) {
    s.priorMax = 0; s.bidSpike = s.askSpike = false;
    if (!s.m.ok) return;
    s.priorMax = priorMaxEither(pb, pa, c.lookback > 0 ? c.lookback : SPIKE_LOOKBACK_DEFAULT);
    s.bidSpike = s.bidRatioOk && isSpike(s.m.bid, s.bidMean, s.priorMax, c);
    s.askSpike = s.askRatioOk && isSpike(s.m.ask, s.askMean, s.priorMax, c);
}

// the stats of one bar from the bars before it (prior = oldest first, valid bars only)
inline Stat statOf(const BarMax& m, const std::vector<long long>& priorBid, const std::vector<long long>& priorAsk, int ratioBars) {
    Stat s; s.m = m;
    if (!m.ok) return s;
    s.bBid = countBiggest(priorBid, m.bid, s.bBidCap);
    s.bAsk = countBiggest(priorAsk, m.ask, s.bAskCap);
    s.bidRatio = ratioOf(priorBid, m.bid, ratioBars, s.bidRatioOk, &s.bidMean);
    s.askRatio = ratioOf(priorAsk, m.ask, ratioBars, s.askRatioOk, &s.askMean);
    return s;
}

// The closed-bar series: stats are computed once when a bar is added (from the bars already in it) and never change.
// Bars are keyed by their time stamp. n/a bars are kept (as n/a) but skipped by every later count and average.
struct Series {
    std::map<long long, Stat> bars;
    long long computed = 0;                    // how many stats were computed (performance test hook)
    SpikeCfg spike;                            // (1.2.0) the spike settings new bars are decided with
    // the prior valid values before time t, newest last (at most `need` of them)
    void prior(long long t, size_t need, std::vector<long long>& pb, std::vector<long long>& pa) const {
        pb.clear(); pa.clear();
        std::map<long long, Stat>::const_iterator it = bars.lower_bound(t);
        while (it != bars.begin() && pb.size() < need) {
            --it;
            if (it->second.m.ok) { pb.push_back(it->second.m.bid); pa.push_back(it->second.m.ask); }
        }
        // collected newest first -> reverse to oldest first
        for (size_t i = 0, j = pb.size(); i < j / 2; ++i) { std::swap(pb[i], pb[j - 1 - i]); std::swap(pa[i], pa[j - 1 - i]); }
    }
    const Stat& add(long long t, const BarMax& m, int ratioBars) {
        std::vector<long long> pb, pa;
        const size_t need = (size_t)(COUNT_CAP > ratioBars ? COUNT_CAP : ratioBars);
        if (m.ok) prior(t, need, pb, pa);
        ++computed;
        Stat s = statOf(m, pb, pa, ratioBars); applySpike(s, pb, pa, spike);
        return bars[t] = s;
    }
    // (1.2.0) the stats a bar at time t WOULD get now (the forming bar's live colour); nothing stored
    Stat peek(long long t, const BarMax& m, int ratioBars) const {
        std::vector<long long> pb, pa;
        if (m.ok) prior(t, (size_t)(COUNT_CAP > ratioBars ? COUNT_CAP : ratioBars), pb, pa);
        Stat s = statOf(m, pb, pa, ratioBars); applySpike(s, pb, pa, spike); return s;
    }
    // (1.2.0) a spike settings change (from the dialog only): spikes of the recorded bars re-decided from the stored maxima
    void respike(const SpikeCfg& c) {
        spike = c; std::vector<long long> pb, pa;
        for (std::map<long long, Stat>::iterator it = bars.begin(); it != bars.end(); ++it) {
            if (!it->second.m.ok) continue;
            prior(it->first, (size_t)SPIKE_LOOKBACK_MAX, pb, pa); applySpike(it->second, pb, pa, c);
        }
    }
    // a Ratio bars change: ratios of every recorded bar recomputed from the stored maxima (no VAP re-read; counts unchanged)
    void reratio(int ratioBars) {
        std::vector<long long> pb, pa;
        for (std::map<long long, Stat>::iterator it = bars.begin(); it != bars.end(); ++it) {
            if (!it->second.m.ok) continue;
            prior(it->first, (size_t)RATIO_HI, pb, pa);
            it->second.bidRatio = ratioOf(pb, it->second.m.bid, ratioBars, it->second.bidRatioOk, &it->second.bidMean);
            it->second.askRatio = ratioOf(pa, it->second.m.ask, ratioBars, it->second.askRatioOk, &it->second.askMean);
        }
    }
    void trim(size_t keep) { while (bars.size() > keep) bars.erase(bars.begin()); }
};

inline std::string fmtRatio(bool ok, double r) {
    if (!ok || !finite(r) || r < 0) return "n/a";
    char b[24]; std::snprintf(b, sizeof(b), "%.1fx", r > 9999 ? 9999.0 : r); return b;
}
inline std::string fmtCount(int n, bool capped, bool ok) {
    if (!ok || n <= 0) return "n/a";
    if (capped || n >= COUNT_CAP) return "999+";
    return std::to_string(n);
}

// the info box: label, value, side (-1 = bid / red, +1 = ask / green)
struct Line { std::string label, value; int side; };
inline std::vector<Line> boxLines(const Stat* s) {
    const bool ok = s && s->m.ok;
    std::vector<Line> v;
    v.push_back(Line{ "BBid", ok ? fmtCount(s->bBid, s->bBidCap, true) : "n/a", -1 });
    v.push_back(Line{ "BAsk", ok ? fmtCount(s->bAsk, s->bAskCap, true) : "n/a", 1 });
    v.push_back(Line{ "BidRatio", ok ? fmtRatio(s->bidRatioOk, s->bidRatio) : "n/a", -1 });
    v.push_back(Line{ "AskRatio", ok ? fmtRatio(s->askRatioOk, s->askRatio) : "n/a", 1 });
    // (1.2.0) Jump = the bigger side / the biggest bid or ask of the prior lookback bars, in that side's colour
    const bool jok = ok && s->priorMax > 0;
    const bool askSide = jok && s->m.ask >= s->m.bid;
    v.push_back(Line{ "Jump", jok ? fmtRatio(true, (double)(askSide ? s->m.ask : s->m.bid) / (double)s->priorMax) : "n/a", askSide ? 1 : -1 });
    return v;
}

// 9-spot placement inside an area (Top / Middle / Bottom x Left / Centre / Right), clamped inside it
struct Box { int l, t, r, b; };
inline Box placeIn(const Box& area, int W, int H, int pos) {
    if (pos < 0 || pos > 8) pos = POS_DEFAULT;
    const int c = pos % 3, r = pos / 3;
    int x = c == 0 ? area.l : c == 1 ? (area.l + area.r) / 2 - W / 2 : area.r - W;
    int y = r == 0 ? area.t : r == 1 ? (area.t + area.b) / 2 - H / 2 : area.b - H;
    if (x > area.r - W) x = area.r - W;
    if (x < area.l) x = area.l;
    if (y > area.b - H) y = area.b - H;
    if (y < area.t) y = area.t;
    return Box{ x, y, x + W, y + H };
}

// the two histogram bars inside one candle slot: red (bid) left, green (ask) right, on one shared scale.
// xc = slot centre, slot = pixels per bar; returns false when the slot cannot hold them (never wider than the slot)
struct Pair { int redL, redR, greenL, greenR; };
inline Pair pairIn(int xc, int slot) {
    if (slot < 2) slot = 2;
    int inner = (slot * 8) / 10; if (inner < 2) inner = 2;        // 80% of the slot, leaving air between candles
    const int gap = inner >= 6 ? 1 : 0;
    const int half = (inner - gap) / 2 > 0 ? (inner - gap) / 2 : 1;
    Pair p; p.redR = xc; p.redL = xc - half;
    p.greenL = xc + gap; p.greenR = p.greenL + half;
    return p;
}
inline int barHeight(long long v, long long top, int px) {
    if (v <= 0 || top <= 0 || px <= 0) return 0;
    const double h = (double)px * (double)v / (double)top;
    const int r = (int)(h + .5);
    return r < 1 ? 1 : (r > px ? px : r);
}
inline int clampRatioBars(int v) { return v < RATIO_LO || v > RATIO_HI ? -1 : v; }

// ---- (1.1.0) hover: Rassul 2026-10-13 10:33 "the bid ask indicator info box should depend on mouse over.. i should be able
// to move it around and see the numbers change."
// the bar the box follows: a CLOSED bar (0 .. count-2) under the mouse; anything else (no bar, the forming bar, past the
// last bar, before the first) = -1 = Live (the last closed bar, as in 1.0.0)
inline int hoverIndex(long bar, long count) { return bar >= 0 && bar < count - 1 ? (int)bar : -1; }
// the hovered bar; set() returns true only when it CHANGED (= one redraw), the same bar again = no redraw
struct Hover {
    int bar = -1;
    bool set(int b) { if (b < -1) b = -1; if (b == bar) return false; bar = b; return true; }
};
// the box header: "Live" or the hovered bar's time on the 12-hour clock ("10:24 AM"); hour 0-23, minute 0-59
inline std::string clock12(int hour, int minute) {
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return "n/a";
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", hour % 12 == 0 ? 12 : hour % 12, minute, hour < 12 ? "AM" : "PM"); return b;
}
// ---- (1.1.0) the pane's own value axis (contracts, 0 at the bottom). Without it IRT showed PRICES on this pane's axis.
// The bars are drawn in pixels: value 0 at yBase = bottom - 1, value `top` px = paneH - 6 pixels above it. The axis IRT
// draws spans the whole pane (bottom -> min, top -> max), so min / max are chosen to put every value at its bar's pixel.
inline void axisFor(long long top, int paneH, double& mn, double& mx) {
    if (top <= 0) { mn = 0; mx = 10; return; }
    if (paneH < 24) { mn = 0; mx = (double)top * 1.02; return; }
    const double unit = (double)top / (double)(paneH - 6);
    mn = -unit; mx = (double)(paneH - 1) * unit;
}
// ---- tenths settings (spike thresholds; the 1.1.0 Ratio line field of an old settings file is range-checked with it)
inline int clampLookback(int v) { return v < 0 || v > SPIKE_LOOKBACK_MAX ? -1 : v; }
inline int clampLineTenths(int v) { return v < 0 || v > LINE_MAX_TENTHS ? -1 : v; }
inline int tenthsOf(float x) { if (!(x >= 0) || x > 10.04f) return -1; return (int)(x * 10.0f + .5f); }
inline std::string fmtTenths(int t) { char b[16]; std::snprintf(b, sizeof(b), "%d.%dx", t / 10, t % 10); return b; }

// =====================================================================================================================
// (1.3.0) WHO IS ATTACKING - letters, the message and its numbers (Rassul 2026-10-13, three-panes-messages-mockup.png)
//   Left bar  = the biggest sell AT THE BID (red)  = aggressive SELLERS hitting resting bids  -> off:: side BID (the bids hit)
//   Right bar = the biggest buy AT THE ASK (green) = aggressive BUYERS lifting resting offers -> off:: side ASK (the offers hit)
//   S = sweep: one aggressor order filling at >= sweepLevels (3) distinct prices in the candle (DBO FILLs sharing aggressorOrderID)
//   L = large: one aggressor order's filled lots in the candle >= largeLotAggr (default the 99th percentile of this session's
//       aggressor order sizes before the candle, at least 20 lots). Priority S, L; letters only where the order feed was live.
//   A side is HIGH when its bar is a spike or its ratio >= 2.0x; the higher ratio gets the message.
//   Bid side high: price HELD (the close did not break the prior candle's low by more than 1 tick) -> green "Sellers hit the bid
//       4.1x normal - price held: buyers absorbing"; broke lower -> red "... - price giving way". Ask side mirrored (held = the close
//       did not break the prior high by more than 1 tick -> red "sellers absorbing"; broke higher -> green "price giving way").
//   Sweep / large in words: "Sellers swept 4 levels with one 120-lot into LonLO - price held: buyers absorbing".
//   " into <LEVEL>" (the approved wording) only when a key / gamma level is within the near band of the candle's close.
// =====================================================================================================================
inline std::string letters(const off::Rec& r, int side, const off::Params& P) {
    std::string o;
    if (r.sweepLv[side] >= P.sweepLevels) o += "S";
    if (r.thrAggr > 0 && r.aggrMax[side] >= r.thrAggr) o += "L";
    return o;
}
// 1 = price held, 0 = gave way (side 0 = bids hit: the prior low; side 1 = offers lifted: the prior high); no prior candle = held
inline int held(int side, double close, double prevLow, double prevHigh, double tick, bool havePrev) {
    if (!havePrev || !finite(close) || !finite(prevLow) || !finite(prevHigh) || !(tick > 0)) return 1;
    const double eps = tick * 1e-6;
    return side == 0 ? (close >= prevLow - tick - eps ? 1 : 0) : (close <= prevHigh + tick + eps ? 1 : 0);
}
inline bool high(const Stat& s, int side) {
    return side == 0 ? (s.bidSpike || (s.bidRatioOk && s.bidRatio >= off::HIGH_X)) : (s.askSpike || (s.askRatioOk && s.askRatio >= off::HIGH_X));
}
// "BBid 240  BidRatio 4.1x  Jump 2.3x  Sweep 4 lvls  Large 120" (Jump only on the first side)
inline std::string sideNums(const Stat& s, const off::Rec* r, const std::string& let, int side, bool jump) {
    const bool b = side == 0;
    std::string o = std::string(b ? "BBid " : "BAsk ") + std::to_string(b ? s.m.bid : s.m.ask) + "  " + (b ? "BidRatio " : "AskRatio ") + fmtRatio(b ? s.bidRatioOk : s.askRatioOk, b ? s.bidRatio : s.askRatio);
    if (jump) o += "  Jump " + (s.priorMax > 0 ? fmtRatio(true, (double)(b ? s.m.bid : s.m.ask) / (double)s.priorMax) : std::string("n/a"));
    if (r && let.find('S') != std::string::npos) o += "  Sweep " + std::to_string(r->sweepLv[side]) + " lvls";
    if (r && let.find('L') != std::string::npos) o += "  Large " + std::to_string(r->aggrMax[side]);
    return o;
}
// (1.5.0) the candle's pace per side (side 0 = bids hit, 1 = offers lifted): x100 (0 = n/a), note (1 fast, 2 slowly), the gate in force
struct PaceIn { long long p100[2] = {0, 0}, note[2] = {0, 0}, gate = 1; };
// the message line (line1) and the record's numbers line (line2) of one closed candle. r = its order-feed numbers (nullptr = the feed was not live for it: no letters)
// (1.5.0, SPEC exact) "Sellers hit the bid with a 240-lot (3.4x) fast (p 2.3x), swept 3 levels into PDL - price held: buyers absorbing."
inline nl::Msg message(const Stat& s, const off::Rec* r, const std::string let[2], const std::string& level, const int heldOf[2], const PaceIn& pc = PaceIn()) {
    nl::Msg m;
    if (!s.m.ok) { m.line1 = "No volume data for this candle"; return m; }
    double sc[2] = {0, 0};
    for (int k = 0; k < 2; ++k) if (high(s, k)) sc[k] = k == 0 ? s.bidRatio : s.askRatio;
    int side = -1;
    if (sc[0] > 0 || sc[1] > 0) {
        if (sc[0] > sc[1]) side = 0; else if (sc[1] > sc[0]) side = 1;
        else side = s.m.bid > s.m.ask ? 0 : s.m.ask > s.m.bid ? 1 : -1;
    }
    const int first = side >= 0 ? side : (s.m.ask > s.m.bid ? 1 : 0);
    m.line2 = sideNums(s, r, let[first], first, true) + "   |   " + sideNums(s, r, let[1 - first], 1 - first, false);
    for (int k = 0; k < 2; ++k) if (pc.p100[k] > 0) m.line2 += std::string(k == 0 ? "  BidPace " : "  AskPace ") + off::paceX(pc.p100[k]);
    if (side < 0) { m.line1 = sc[0] > 0 ? "Both sides attacking evenly (" + nl::ratioText(sc[0]) + ")" : "Normal activity - no spike"; return m; }
    const bool b = side == 0, S = r && let[side].find('S') != std::string::npos, L = r && let[side].find('L') != std::string::npos;
    const std::string rt = nl::ratioText(b ? s.bidRatio : s.askRatio);
    std::string t = std::string(b ? "Sellers hit the bid" : "Buyers lifted the offer") + " with a " + std::to_string(b ? s.m.bid : s.m.ask) + "-lot (" + rt + ")"
                  + off::paceWords(pc.note[side], pc.p100[side], pc.gate);
    if (S && L && r->sweepIsMax[side]) t += ", swept " + std::to_string(r->sweepLv[side]) + " levels with one " + std::to_string(r->aggrMax[side]) + "-lot order";
    else if (S && L) t += ", swept " + std::to_string(r->sweepLv[side]) + " levels, plus one " + std::to_string(r->aggrMax[side]) + "-lot order";
    else if (S) t += ", swept " + std::to_string(r->sweepLv[side]) + " levels";
    else if (L) t += ", one " + std::to_string(r->aggrMax[side]) + "-lot order";
    if (!level.empty()) t += " into " + level;
    if (heldOf[side]) { t += b ? " - price held: buyers absorbing." : " - price held: sellers absorbing."; m.impl = b ? nl::BULL : nl::BEAR; }
    else { t += " - price giving way."; m.impl = b ? nl::BEAR : nl::BULL; }
    m.line1 = t;
    return m;
}

// (12:05) the info box rows: the 1.2.0 box (BBid / BAsk = biggest in N candles, BidRatio / AskRatio, Jump) + Sweep + Large.
// side: -1 = bid (red), +1 = ask (green), 0 = white. r = nullptr when the order feed was not live for the candle (n/a).
inline std::vector<Line> infoLines(const Stat* s, const off::Rec* r, const std::string let[2], bool dbo) {
    std::vector<Line> v = boxLines(s);
    int sw = -1, lg = -1;
    if (dbo && r) for (int k = 0; k < 2; ++k) {
        if (let[k].find('S') != std::string::npos && (sw < 0 || r->sweepLv[k] > r->sweepLv[sw])) sw = k;
        if (let[k].find('L') != std::string::npos && (lg < 0 || r->aggrMax[k] > r->aggrMax[lg])) lg = k;
    }
    const char* who[2] = { " sellers", " buyers" };
    v.push_back(Line{ "Sweep", !dbo || !r ? std::string("n/a") : sw < 0 ? std::string("none") : std::to_string(r->sweepLv[sw]) + " lvls" + who[sw], sw < 0 ? 0 : (sw == 0 ? -1 : 1) });
    v.push_back(Line{ "Large", !dbo || !r ? std::string("n/a") : lg < 0 ? std::string("none") : std::to_string(r->aggrMax[lg]) + who[lg], lg < 0 ? 0 : (lg == 0 ? -1 : 1) });
    return v;
}

// (1.5.0) the hover box, two columns: Sellers (bids hit, red) | Buyers (offers lifted, green). dbo = the order feed was live for it.
//   Lots "240 (3.4x)", Pace "2.3x (2.0x) ?", Orders "42 · avg 3.6", Top order "12%", Sweep "3 lvls" / "-", Large "120" / "-"
struct Row2 { std::string label, v[2]; };
inline std::vector<Row2> infoRows(const Stat* s, const off::Rec* r, const std::string let[2], bool dbo, const PaceIn& pc, long long paceFastT) {
    std::vector<Row2> v; const bool ok = s && s->m.ok;
    // (1.5.1, Rassul 13 Oct "just do it") Lots, Pace, Sweep, Large, [StopRun - added by the plugin], Crowd, [Burst]: Crowd = orders \xB7 avg \xB7 top share
    Row2 a; a.label = "Lots"; Row2 p; p.label = "Pace"; Row2 o; o.label = "Crowd"; Row2 w; w.label = "Sweep"; Row2 l; l.label = "Large";
    for (int k = 0; k < 2; ++k) {
        a.v[k] = ok ? std::to_string(k == 0 ? s->m.bid : s->m.ask) + " (" + fmtRatio(k == 0 ? s->bidRatioOk : s->askRatioOk, k == 0 ? s->bidRatio : s->askRatio) + ")" : std::string("n/a");
        p.v[k] = ok ? off::paceHover(pc.p100[k], paceFastT, pc.gate) : std::string("n/a");
        const bool live = dbo && r;
        o.v[k] = live ? off::crowdText(r->aggrN[k], r->aggrC[k], r->aggrMax[k]) : std::string("n/a");
        w.v[k] = !live ? std::string("n/a") : let[k].find('S') != std::string::npos ? std::to_string(r->sweepLv[k]) + " lvls" : std::string("-");
        l.v[k] = !live ? std::string("n/a") : let[k].find('L') != std::string::npos ? std::to_string(r->aggrMax[k]) : std::string("-");
    }
    v.push_back(a); v.push_back(p); v.push_back(w); v.push_back(l); v.push_back(o);
    return v;
}

} // namespace bas
#endif
