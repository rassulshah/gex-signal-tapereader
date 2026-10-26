#ifndef LS_BIDASKSTATS_LOGIC_H
#define LS_BIDASKSTATS_LOGIC_H
// BidAskStatsLogic.h (1.13.0 2026-10-18: one spike rule, closed bars only, Market State facts; 1.5.0, 2026-10-13; 1.4.0 / 1.3.0 / 1.2.0 / 1.1.0 / 1.0.0 2026-10-13) -- SDK-free logic of lsBidAskStats.
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
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "OrderFlowFeed.h"
#include "BoxRead.h"

namespace bas {

static const char* const QUESTION = "Who is aggressive?";   // (1.26.0, Rassul 20:11-20:23 CT "the attacker pane") was "Is the effort rewarded?" (1.22.0), "Is there reward for the effort?"
// (1.22.0, Rassul 23:23 / 23:29, info-icon-mockup.png) the info icon's hover text: the description, how it is calculated, an example
static const char* const INFO_TIP =
"Effort Reward - Who is aggressive?\n"
"Compares each side's aggressive volume with its own normal (Effort) and how far that\n"
"volume moved price its way (Reward), to show whether buyers or sellers are getting\n"
"paid for pushing or are being absorbed.\n"
"\n"
"How it is calculated (each candle, each side)\n"
"  Lots: buyers = bought at the offer, sellers = sold at the bid\n"
"  Effort: Lots / average of the last 20 candles (2.0x = twice normal)\n"
"  Reward: ticks price moved that side's way (to the high / low) per 1,000 lots,\n"
"     divided by the market normal (ES 5.73)\n"
"  Rwd = Reward 2.0x or more.  Abs = Reward 0.5x or less AND the candle\n"
"     closed against that side\n"
"\n"
"Example - ES 8:18 PM\n"
"  Sellers sold 762 lots and pushed 10 ticks to the low = 13.1 ticks per 1,000\n"
"  = 2.3x normal, but the candle closed green: sellers lost the candle.\n"
"  Buyers bought 1,118 lots = 2.4x Effort. Next candle buyers got Rwd: +17 ticks.\n"
"\n"
"Badge row (attacker letters, decided at the candle close; hover a badge)\n"
"  S sweep of the leg, K big aggressor (S10+), G giving up, Tr trapped,\n"
"  R stop run, L large order (S20 to S10). Red = bearish, green = bullish.";   // (1.20.0, Rassul 19:31; 1.18.0 was "Who is aggressive - and is it moving price?")
static const char* const VERSION = "1.33.1";   // (1.33.0) display: bars + S only //   // 1.32.0 (2026-10-23) D letter + attack average fill for the 4-step Reversal Stage //   // 1.31.0 (2026-10-23 MBO review merge) E letter, typed EV-ATK evidence //   // 1.30.0 (2026-10-22 evening fix build) compact strips, 50% bar floor, one-line Implied row;   // (1.29.0, 2026-10-22 evening, Rassul 17:16 CT approved IMPLIED FILLS option B) implied (spread-driven) fills - the order feed's side "?" FILLs (OrderFlowFeed 1.14.0 / ImpliedFill.h) - are no longer hits, aggressor orders, sweeps, big lots (S20 / S10 / S5) or attacker letters; kept in volume; the candle box gets the row "Implied  NN% of  volume" when lsFlexLevels\LRA-Implied-<MKT>.csv is present; status IMPLIED // (1.28.0, 2026-10-22 Rassul approved layout-stages-2strips.png 11:41-11:46 CT, build 15:20 CT) the badge row -> TWO LETTER STRIPS (LetterStrips.h): "Attack" S > K > L and "Running out" R > Tr > G, one letter per strip per candle in the colour of the side that acts, no A here (Defense pane only), hovers per strip, the letters appended to lsFlexLevels\Strips\ATK-<MKT>-<spb>.csv for lsReversalStage // (1.27.0, 2026-10-22 Rassul 06:43 "then do it" / 06:49 "then require it") the SHARED absorption A (SharedAbsorb.h 1.0.0): decided here at each finalized candle (5-candle extreme + attackers >= E x their 20-candle average + the defenders' colour close), appended to lsFlexLevels\SharedAbsorb\SAB-<MKT>-<spb>.csv with the book / pace / level; the attacker badge shows A first (green at a low, red at a high) from that record, its hover in the agreed order; the Defense T line A no longer shows anywhere; status SHARED_A // (1.26.0, 2026-10-21 Rassul 20:11-20:23 CT, attacker-mockup-v2.png) the ATTACKER pane: question "Who is aggressive?"; the BADGE ROW under it (AttackLogic.h): S (biggest sweep of the leg) / K (big aggressor >= S10, no initiative candle) at a 5-candle high / low, G / Tr moved here from lsBookFlow (their rules and records unchanged, read from the turns file / the Delta Profile TR record), R (stop run), L (one order >= S20 but < S10); one badge per candle, at most 4 letters, colour by meaning, no "?"; the badge hover per letter with the record numbers and the combination with a Defense letter; K / S / AS lines in the session record (reloaded, never repainted); no "?" on the message line; OrderFlowFeed 1.13.0 (biggest order / sweep per side with its second and prices);  //   // (1.25.0, 2026-10-21 MBO Roadmap Top Defender #1) posts the order-book absorption prices of each decided candle on the SignalBoard (kind 'O': traded >= 2x shown + >= 2 refills, OrderFlowFeed 1.7.0) for lsDeltaProfile / lsBigBidAsk; status ORDERBOOK_ABS; nothing drawn changes // (1.24.1 (2026-10-20 evening, Rassul "implement" the audit): stricter turns-file parsing, stale live-line fix, WATCH waits for the full 20-60 s window, touch freshness / stale-event guards, incomplete order totals marked partial, invalid effort / reward inputs rejected; ) // (1.24.0, 2026-10-20 CT Rassul) the ORDER-BY-ORDER totals ON after the close (07:43 "do it"); 08:19 / 14:22 "if i move it dont the bars move also ?": the bars and the Big Lot cap use the pane's y-axis (IRT's value -> pixel mapping): dragging / stretching the axis zooms the bars; // (1.23.0, 2026-10-20 07:43 / 07:44 / 08:01 CT Rassul "it crashed ... use a hoverover approach") the info icon description DRAWN as a multi-line box on hover - IRT's help tip (showHelpTip) crashed IRT and showed one long line; (staged, OFF until after the close: BAS_MBO_TOTALS) bars, Lots, Effort, Reward and the reads use the ORDER-BY-ORDER totals (the exchange aggressor fills from the order feed, what the turn rates were tested on); a candle without order data keeps IRT's bid / ask volume and its box row reads "Lots (IRT)"; // (1.22.0, 2026-10-19 23:29 CT Rassul "build", info-icon-mockup.png) question "Is the effort rewarded?" just right of the info box + info icon with the description / calculation / example on hover; Pace removed from the bar labels (unproven, already off the box); // (1.21.0, 2026-10-19 20:48 CT Rassul) Abs only when the close went against the side (49% / 51% vs 22%), "pushed Xx but closed green - lost the candle ?" (no strong-support read), headings Buyers green / Sellers red; // (1.20.0, 2026-10-19 19:31 CT Rassul) title "Effort Reward", question "Is there reward for the effort?", box columns Buyers | Sellers (buyers left, as the bars and Defense), row "Effort" (was Attack Effort), Reward "0.3x Abs" / "2.4x Rwd" / plain; // (1.19.0, 2026-10-19 19:25 CT Rassul "go with your recommendation") the bars and the Lots row = each side's TOTAL aggressive lots, bright + its ratio at 2.0x+ Attack Effort (the validated measure; was the biggest lots at one price); Pace and BBI / BAI rows removed (unproven); the Read from Attack Effort / Reward; // (1.18.0, 2026-10-19 19:02-19:09 CT Rassul, mockup side-by-side-question-mockup.png "build") PAIR bars - buyers green left, sellers red right, both above zero (was the 1.16.0 mirror); the question line "Who is aggressive - and is it moving price?" on top of the pane; // (1.17.0, 2026-10-19 18:28 CT Rassul approved aggr-cap-mockup.png: "there needs to be a different scale .. it is difficult to see and gets distorted") the SCALE CAP - the bars scale to 1.5x the 90th-percentile bar on screen, a taller bar stops at the edge with a white tip (label + box keep the true size); // (1.16.0, 2026-10-19 Rassul 17:04 "show both bid and ask and i want the selling side to be on the bottom" + Effort vs Reward, "build it already" 17:42, mockup bidask-mirror-mockup.png) MIRROR bars: buyers green above the grey zero line, sellers red below it, both every candle (OrderFlowFeed drawMirrorBars; the zero line where the data needs it); box rows + Attack Effort and Reward (EffortReward.h) under Lots; at a new 10-bar low / high the Effort vs Reward read leads (absorbed / going through, "?"); // (1.15.0, 2026-10-18 23:48 CT approved aggr-read-v2-mockup.png) box rows Lots, Pace, BBI / BAI, Read (aggRead, BoxRead.h); the BL5 row removed (the cyan cap stays); // (1.14.2, 2026-10-18 23:34 CT approved aggr-bbi-mockup.png) box row "BBI / BAI": how many prior candles in a row this biggest bid / ask beats (strictly), cap 50+ (beatText); // (1.14.1, 2026-10-18 23:19 CT) the box follows the mouse only inside the pane (PaneCursor.h), else Live = the forming candle; // (1.14.0, 2026-10-18, Rassul 17:26 CT) Big Lot mark: a cyan cap on a closed bar >= BL5 / BL10 / BL20 of its session type (BigLotTiers.h), decided at the close, frozen; box row "BL5 311"; history + marks recorded for the nightly; // (1.13.3, 2026-10-18 his decision list) window: Position only (Offset X / Y removed, Ratio bars / Spike vs average hidden, kept in the file);  (1.13.2, 2026-10-18 settings audit) the window from OrderFlowPaneSettings.h through SettingsHost.h (same rows / file); (1.13.1, 2026-10-18 settings audit) Spike vs average moved to its own line of the settings window (the first line was ~458 px and cut off); rows / file unchanged // (1.13.0, 2026-10-18, Rassul approved) ONE spike rule for Aggression and Book Flow: >= Spike vs average (2.0x) the plain 20-candle average of that side, decided at the close (the "1.5x the prior biggest" jump is a record field only); the forming bar is drawn dim with no label (no repaint); "Box shows" removed; Market State SRC AGG facts (aggFacts);   // (1.12.1) 6E seeds read (named 6E, chart code EU);   // (1.12.0) Big Lot S20 / S10 / S5: aggression L = S20;   // 1.10.0 (2026-10-16): numbers box in the pane;   // 1.9.1: ratio + pace above the bar, no lots, no white tick;   // 1.9.0 (2026-10-16): the pane is called Aggression; no text lines; only bars at 2.0x+; plain 20-bar average;   // 1.8.1: no hovers (Rassul 18:57);   // (1.7.2) "Bid/Ask Stats � Who is pressing?" in front of the read line;   // (1.7.1) no S / L / R letters drawn;   // 1.7.0 (2026-10-14 night, Rassul 22:37 CT): one bar per candle, title "Bid/Ask Stats - Who is Pressing", attacking -> pressing;   // 1.6.0 (2026-10-14 night): the read box -> one zone read line (ZoneRead.h), unsigned options line;   // 1.5.3 (2026-10-14 CR5300): spike labels keep pace / lots (OrderFlowFeed 1.3.1); 1.5.2 (2026-10-14 CR4800): read box + numbers hover, Offset X / Y, white letters, one spike label, rare L; 1.5.1 (2026-10-13 9:40 PM): row order + Crowd, 10 pt labels, chart-wide hover; 1.5.0 (2026-10-13): pace, SPEC grammar, hover rows, options line; 1.4.0: stop run R?; 1.3.1: no white outline
static const char* const TITLE = "Effort Reward";   // (1.20.0, Rassul 19:31) was "Aggression"   // (1.7.0) the pane title (parmsTitle), "v1.7.0" next to it
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
    bool on() const { return avgT > 0; }                                // (1.13.0) only "Spike vs average" switches spikes on / off
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
// (1.13.0) THE spike rule (both panes, Rassul: spikes only at >= 2.0x the plain 20-candle average): v / mean >= X, exact at the boundary
// (10 v >= tenths * mean). The jump (v / the biggest of the prior lookback candles) is no longer part of it - kept as a record field (Jump).
inline bool isSpike(long long v, double mean, const SpikeCfg& c) {
    if (!c.on() || v <= 0 || !(mean > 0)) return false;
    return (double)v * 10.0 >= (double)c.avgT * mean;
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
    int slotN = 0;                             // (1.8.0) same-slot bars the ratio / spike used (0 = the 20-bar rule)
};
// (1.2.0) the spike fields of a stat from its prior bars (oldest first)
inline void applySpike(Stat& s, const std::vector<long long>& pb, const std::vector<long long>& pa, const SpikeCfg& c) {
    s.priorMax = 0; s.bidSpike = s.askSpike = false;
    if (!s.m.ok) return;
    s.priorMax = priorMaxEither(pb, pa, c.lookback > 0 ? c.lookback : SPIKE_LOOKBACK_DEFAULT);
    s.bidSpike = s.bidRatioOk && isSpike(s.m.bid, s.bidMean, c);         // (1.13.0) priorMax stays for the Jump record field only
    s.askSpike = s.askRatioOk && isSpike(s.m.ask, s.askMean, c);
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
    // (1.8.0, Rassul 2026-10-15 audit #1) the same 30-min slot: the chart's bars of this half hour before t (all sessions on the chart,
    // newest 80); 10+ of them = ratio against their typical size (log mean) and spike = 2 SD above it, else the 20-bar rule stays
    void slotFix(long long t, Stat& s) const {
        s.slotN = 0; if (!s.m.ok) return;
        std::vector<long long> vb, va; const int sl = off::slotOfKey(t);
        std::map<long long, Stat>::const_iterator it = bars.lower_bound(t);
        while (it != bars.begin() && (int)vb.size() < off::SLOT_MAX_N) { --it; if (!it->second.m.ok || off::slotOfKey(it->first) != sl) continue; vb.push_back(it->second.m.bid); va.push_back(it->second.m.ask); }
        const off::SlotLog zb = off::slotLog(vb), za = off::slotLog(va);
        off::applySlot(s.m.bid, zb, s.bidRatioOk, s.bidRatio, s.bidMean, s.bidSpike);
        off::applySlot(s.m.ask, za, s.askRatioOk, s.askRatio, s.askMean, s.askSpike);
        if (!spike.on()) s.bidSpike = s.askSpike = false;
        s.slotN = (zb.ok || za.ok) ? std::max(zb.n, za.n) : 0;
    }
    const Stat& add(long long t, const BarMax& m, int ratioBars) {
        std::vector<long long> pb, pa;
        const size_t need = (size_t)(COUNT_CAP > ratioBars ? COUNT_CAP : ratioBars);
        if (m.ok) prior(t, need, pb, pa);
        ++computed;
        Stat s = statOf(m, pb, pa, ratioBars); applySpike(s, pb, pa, spike); slotFix(t, s);
        return bars[t] = s;
    }
    // (1.13.0) peek() removed: the forming bar is never judged (drawn dim, no label) - only closed bars get a colour and a label
    // (1.2.0) a spike settings change (from the dialog only): spikes of the recorded bars re-decided from the stored maxima
    void respike(const SpikeCfg& c) {
        spike = c; std::vector<long long> pb, pa;
        for (std::map<long long, Stat>::iterator it = bars.begin(); it != bars.end(); ++it) {
            if (!it->second.m.ok) continue;
            prior(it->first, (size_t)SPIKE_LOOKBACK_MAX, pb, pa); applySpike(it->second, pb, pa, c); slotFix(it->first, it->second);
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
            slotFix(it->first, it->second);
        }
    }
    void trim(size_t keep) { while (bars.size() > keep) bars.erase(bars.begin()); }
};

inline std::string fmtRatio(bool ok, double r) {
    if (!ok || !finite(r) || r < 0) return "n/a";
    char b[24]; std::snprintf(b, sizeof(b), "%.1fx", r > 9999 ? 9999.0 : r); return b;
}
// (1.14.2) BBI / BAI (Biggest Bid / Ask In): the number of consecutive PRIOR candles whose biggest bid (ask) is strictly smaller than this
// candle's - stops at the first prior candle with an equal or bigger value; n/a candles are skipped as in every count; shown up to 50
// ("50+" beyond). From countBiggest (1 + that run): beats = count - 1. Fewer prior candles = the count available (the first candle 0).
static const int BEAT_CAP = 50;
inline int beatsOf(const std::vector<long long>& prior, long long v) { bool c = false; return countBiggest(prior, v, c) - 1; }
inline std::string beatText(int count, bool capped, bool ok) {
    if (!ok || count <= 0) return "n/a";
    const int n = count - 1;
    return capped || n >= BEAT_CAP ? std::to_string(BEAT_CAP) + "+" : std::to_string(n);
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
// (1.13.0) HIGH = the spike (one rule: >= Spike vs average x the plain average); the message, the bright bar and the label agree
inline bool high(const Stat& s, int side) { return side == 0 ? s.bidSpike : s.askSpike; }
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
    if (side < 0) { m.line1 = sc[0] > 0 ? "Both sides pressing evenly (" + nl::ratioText(sc[0]) + ")" : "Normal activity - no spike"; return m; }
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

// =====================================================================================================================
// (1.13.0, 2026-10-18) WHAT IS DRAWN, decided from closed bars only, and the Market State facts (SRC "AGG", MARKET-STATE-SPEC.md 1.0)
// =====================================================================================================================
// a record row written before 1.13.0 kept the stricter old spike (2.0x AND the 1.5x jump) while 1.12 drew every closed bar at 2.0x+
// bright: the colour as drawn then = the recorded spike OR the one rule (identical for rows written by 1.13.0) - no repaint at the upgrade
inline bool fileSpike(bool recorded, long long v, bool ratioOk, double mean, const SpikeCfg& c) { return recorded || (ratioOk && isSpike(v, mean, c)); }
// the look of one candle's side: closed = a later bar exists. Only a closed spike is bright and carries its ratio label (drawBars' onlyHigh
// shows a label exactly when the side is bright: ratioOk is handed over only for a spike, so a 2.0x bar under a higher setting stays dim).
struct SideLook { bool bright = false, ratioOk = false; double ratio = 0; };
inline SideLook sideLook(const Stat* s, int side, bool closed) {
    SideLook k; if (!closed || !s || !s->m.ok) return k;
    const bool sp = side == 0 ? s->bidSpike : s->askSpike;
    if (!sp) return k;
    k.bright = true; k.ratioOk = side == 0 ? s->bidRatioOk : s->askRatioOk; k.ratio = side == 0 ? s->bidRatio : s->askRatio; return k;
}
inline std::string fmt1(double x) { if (!finite(x) || x < 0) return "?"; char b[24]; std::snprintf(b, sizeof(b), "%.1f", x > 9999 ? 9999.0 : x); return b; }
// the Big Lot tier of a lot size: t = S20 / S10 / S5 (ascending); "?" when the tiers are not known yet
inline std::string tierOf(long long lots, bool ok, const long long t[3]) {
    if (!ok || t[0] <= 0) return "?";
    if (t[2] > 0 && lots >= t[2]) return "S5";
    if (t[1] > 0 && lots >= t[1]) return "S10";
    return lots >= t[0] ? "S20" : "none";
}
// the side the candle's single bar shows (the bigger side; a tie = the close direction, else sellers) - 0 = bids hit (sellers), 1 = buyers
inline int domSide(const Stat& s, int dir) { return s.m.ask > s.m.bid ? 1 : s.m.bid > s.m.ask ? 0 : (dir > 0 ? 1 : 0); }
// SRC AGG: buy_lots / sell_lots = the biggest at one price (the drawn bars), buy_x / sell_x = vs the plain Ratio-bars average, spike, pace_x
// (the spiking side, else the drawn side), sweep, big_aggr (the largest aggressor order), big_tier. r = nullptr: the order feed was not live.
struct AggIn { const Stat* s = nullptr; const off::Rec* r = nullptr; std::string let[2]; long long p100[2] = {0, 0}; int dir = 0; bool tiersOk = false; long long tiers[3] = {0, 0, 0}; long long sweepMin = 3; };
inline std::map<std::string, std::string> aggFacts(const AggIn& in) {
    std::map<std::string, std::string> kv;
    const Stat* s = in.s; const bool ok = s && s->m.ok;
    kv["sell_lots"] = ok ? std::to_string(s->m.bid) : "?";
    kv["buy_lots"] = ok ? std::to_string(s->m.ask) : "?";
    kv["sell_x"] = ok && s->bidRatioOk ? fmt1(s->bidRatio) : "?";
    kv["buy_x"] = ok && s->askRatioOk ? fmt1(s->askRatio) : "?";
    int sp = -1;
    if (ok && (s->bidSpike || s->askSpike)) sp = s->bidSpike && s->askSpike ? (s->askRatio > s->bidRatio ? 1 : s->bidRatio > s->askRatio ? 0 : domSide(*s, in.dir)) : (s->askSpike ? 1 : 0);
    kv["spike"] = !ok ? "?" : sp == 1 ? "buy" : sp == 0 ? "sell" : "none";
    const int ps = ok ? (sp >= 0 ? sp : domSide(*s, in.dir)) : -1;
    kv["pace_x"] = ps >= 0 && in.p100[ps] > 0 ? fmt1((double)in.p100[ps] / 100.0) : "?";
    if (!in.r) { kv["sweep"] = "?"; kv["big_aggr"] = "?"; kv["big_tier"] = "?"; return kv; }
    int sw = -1;
    for (int k = 0; k < 2; ++k) if (in.let[k].find('S') != std::string::npos && (sw < 0 || in.r->sweepLv[k] > in.r->sweepLv[sw])) sw = k;
    kv["sweep"] = sw == 1 ? "buy" : sw == 0 ? "sell" : "none";
    const long long big = std::max(in.r->aggrMax[0], in.r->aggrMax[1]);
    kv["big_aggr"] = std::to_string(big);
    kv["big_tier"] = big > 0 ? tierOf(big, in.tiersOk, in.tiers) : (in.tiersOk ? "none" : "?");
    return kv;
}

// (1.15.0, Rassul approved 23:48 CT) THE READ ROW - who is attacking, from the candle shown (hovered closed, else the live forming one):
//   sellers' lots >= 2.0x their average and buyers' < 2.0x -> BEARISH "Sellers hitting {r}x"; buyers >= 2.0x and sellers < 2.0x -> BULLISH
//   "Buyers lifting {r}x"; both >= 2.0x -> NEUTRAL "Both sides hitting ~{r}x - fight; watch the close and Book Flow"; neither -> NEUTRAL
//   "Quiet - no side attacking". Clauses on the attacking side: ", fast ({p}x)" when its pace >= 2.0x; " - biggest bid / ask in {BBI}" when
//   it beats >= 20 prior candles (50+ capped); ", buyers / sellers quiet ({r}x)" when the other side is < 1.5x. (Spec gap: the other side
//   1.5x - 2.0x keeps the attacking side's read without the quiet clause.)
static const int READ_HIT_T = 20, READ_QUIET_T = 15, READ_FAST_P100 = 200, READ_BIGGEST_N = 20;
inline br::Read aggRead(const Stat* s, const PaceIn& pc) {
    br::Read r;
    if (!s || !s->m.ok) { r.head = "No volume data for this candle"; return r; }
    const double rt[2] = { s->bidRatioOk ? s->bidRatio : 0.0, s->askRatioOk ? s->askRatio : 0.0 };
    const bool ok[2] = { s->bidRatioOk, s->askRatioOk };
    const bool hit[2] = { ok[0] && rt[0] * 10.0 >= READ_HIT_T - 1e-9, ok[1] && rt[1] * 10.0 >= READ_HIT_T - 1e-9 };
    if (hit[0] && hit[1]) { r.head = "Both sides hitting ~" + br::approx((rt[0] + rt[1]) / 2.0) + "x - fight; watch the close and Book Flow"; return r; }
    if (!hit[0] && !hit[1]) { r.head = "Quiet - no side attacking"; return r; }
    const int a = hit[0] ? 0 : 1, o = 1 - a;
    r.impl = a == 0 ? br::BEAR : br::BULL;
    r.head = std::string(a == 0 ? "Sellers hitting " : "Buyers lifting ") + br::x1(rt[a]) + "x";
    if (pc.p100[a] >= READ_FAST_P100) r.opt.push_back(", fast (" + br::x1((double)pc.p100[a] / 100.0) + "x)");
    const int cnt = a == 0 ? s->bBid : s->bAsk; const bool cap = a == 0 ? s->bBidCap : s->bAskCap;
    if (cnt - 1 >= READ_BIGGEST_N || cap) r.opt.push_back(std::string(" - biggest ") + (a == 0 ? "bid" : "ask") + " in " + beatText(cnt, cap, true));
    if (ok[o] && rt[o] * 10.0 < READ_QUIET_T) r.opt.push_back(std::string(", ") + (o == 0 ? "sellers" : "buyers") + " quiet (" + br::x1(rt[o]) + "x)");
    return r;
}

} // namespace bas
#endif
