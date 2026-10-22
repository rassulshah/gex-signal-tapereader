#ifndef LS_ATTACK_LOGIC_H
#define LS_ATTACK_LOGIC_H
// AttackLogic.h 1.0.0 (2026-10-21, Rassul 20:11-20:23 CT, attacker-mockup-v2.png approved) - SDK-free logic of the ATTACKER letters on the
// attacker pane (lsBidAskStats 1.26.0, "Who is aggressive?"). Unit-tested on Linux (tests_aggbook/test_attack_126.cpp).
//
// The BADGE ROW (one badge per candle, at most 4 letters, the font shrinking to fit, the rest in the hover), colour BY MEANING: bearish red,
// bullish green - buyers attacking or giving up at a high = red, sellers attacking or giving up at a low = green. No "?" anywhere.
//   S  = the biggest SWEEP of the leg: one aggressor order (fills sharing one aggressorOrderID) filling >= 3 prices, the most lots of such
//        sweeps on the attacker side in this candle, and bigger than the biggest sweep on that side in each of the 10 candles before it
//        (all 10 must have the order feed - else not decided, never guessed); at a 5-candle high (buyers) / low (sellers).
//   K  = the big aggressor: one aggressor order >= Big Lot S10 (LargeLotPool S20 / S10 / S5 per market and session type, frozen at the
//        close) on the attacker side, at a 5-candle high / low, NOT on an initiative candle (dt::initiativeOf: the candle's delta with the
//        move >= 2x the median |delta| of the 20 before it and the close in the outer quarter).
//   G  = the attackers giving up (lsBookFlow's G line, its exact rule and record - moved here from the Defense pane, read only).
//   Tr = trapped traders (the Delta Profile's TR record - moved here from the Defense pane, read only).
//   R  = the stop run (StopRun.h event on its break candle; logged-only events are not drawn).
//   L  = one large aggressive order >= S20 but < S10 (K replaces L: an order >= S10 is K at an extreme, nothing elsewhere).
//   E (attack fading after the extreme) and speeding up into it: hover only, never drawn.
// Decided once at the candle close (K / S / AS lines in the BidAskStats session record, reloaded after a restart) - never repainted.
// Research: /home/claude/pt/attk (ev.py, attacker_ev.json, lifts.json): 7 markets Sep 24 - Oct 9, 16,449 5-candle extremes, "held" =
// the extreme held (+1.5R before beyond it), matched controls by market / session part / ATR tercile / side / distance.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "OrderFlowFeed.h"
#include "DefenseTurn.h"
#include "TrapLogic.h"
#include "MoveLogic.h"
#include "BoxRead.h"

namespace atk {

static const char* const VERSION = "1.0.0";
static const int EXT_BARS = 5;              // a 5-candle high / low: beyond the 5 candles before (lsBookFlow decideGiveUp, the research universe)
static const int LEG_BARS = 10;             // S: bigger than the biggest sweep of each of the 10 candles before
static const int INIT_BARS = 20;            // the initiative veto: the deltas of the 20 candles before
static const int BADGE_MAX = 4;
static const long long E_MIN_SECS = 20;     // E measured only with >= 20 s after the extreme (as the research / G)
static const long long INTO_MIN_SECS = 10;  // speeding up: >= 10 s from the candle's first fill to the extreme
static const long long E_SHOW100 = 50, INTO_SHOW100 = 200;   // hover: "fading" at 0.5x or less, "speeding up" at 2.0x or more
enum { LOW = dt::LOW, HIGH = dt::HIGH };

// ---- the combination table (attacker_ev.json + lifts.json 2026-10-21; to be replaced by the nightly scores) -----------------------------
// held % with the letter(s) vs the matched control %, n cases (7 markets)
struct Score { const char* key; double held, ctl; int n; };
static const Score SCORES[] = {
    { "K", 51.4, 43.9, 628 }, { "S", 48.7, 43.7, 2008 }, { "G", 49.0, 44.7, 1055 }, { "Tr", 56.8, 50.2, 560 },
    { "K+A", 61.2, 47.0, 49 }, { "S+A", 55.2, 50.6, 116 }, { "G+A", 57.0, 47.5, 114 }, { "Tr+A", 67.1, 49.3, 79 },
    { "K+G", 56.8, 42.8, 88 }, { "K+Tr", 65.0, 49.2, 60 }, { "S+Tr", 55.3, 49.6, 123 }, { "Tr+G", 61.6, 49.2, 73 },
    { "G+E", 56.2, 46.8, 283 }, { "K+E", 48.3, 44.2, 232 }, { "S+E", 49.7, 44.8, 715 } };
inline const Score* scoreOf(const std::string& key) {
    for (size_t k = 0; k < sizeof(SCORES) / sizeof(SCORES[0]); ++k) if (key == SCORES[k].key) return &SCORES[k];
    return nullptr;
}
// the pair key in the table's order ("K" + "G" -> "K+G", "G" + "Tr" -> "Tr+G")
inline std::string pairKey(const std::string& a, const std::string& b) {
    if (scoreOf(a + "+" + b)) return a + "+" + b;
    if (scoreOf(b + "+" + a)) return b + "+" + a;
    return std::string();
}
// per-market lift (points vs the control) and n for K and S (attacker_ev.json "pm")
struct MarketLift { const char* m; double kLift; int kN; double sLift; int sN; };
static const MarketLift LIFTS[] = {
    { "ES", 10.3, 88, 8.9, 290 }, { "NQ", 11.1, 73, 5.8, 460 }, { "CL", 17.1, 106, 4.5, 355 }, { "GC", 10.6, 70, 3.3, 464 },
    { "HG", 3.6, 93, 3.1, 236 }, { "NG", -7.3, 71, -2.4, 133 }, { "EU", 4.2, 127, 17.5, 70 }, { "6E", 4.2, 127, 17.5, 70 } };
inline const MarketLift* liftOf(const std::string& m) { for (size_t k = 0; k < sizeof(LIFTS) / sizeof(LIFTS[0]); ++k) if (m == LIFTS[k].m) return &LIFTS[k]; return nullptr; }
inline std::string pct(double v) { char b[16]; std::snprintf(b, sizeof b, "%.0f%%", v); return b; }
inline std::string signed1(double v) { char b[24]; std::snprintf(b, sizeof b, "%+.1f", v); return b; }
// "K + A held 61% vs 47% (n 49, learning)"
inline std::string scoreText(const std::string& key) {
    const Score* s = scoreOf(key); if (!s) return std::string();
    std::string k = key; for (size_t p = k.find('+'); p != std::string::npos; p = k.find('+', p + 3)) k.replace(p, 1, " + ");
    return k + " held " + pct(s->held) + " vs " + pct(s->ctl) + " (n " + dt::lotsText(s->n) + ", learning)";
}

// ---- the attack pace around the extreme (hover only) ----------------------------------------------------------------------------------
// e100 = lots / s hit by the attackers from the extreme to their last fill / lots / s before it (x100; -1 = n/a: < 20 s after it or no attack
// before); into100 = the second half vs the first half of the time from the candle's first fill to the extreme (x100; -1 = n/a)
struct Pace { long long e100 = -1, into100 = -1; };
inline Pace paceOf(const off::Acc& a, int ext) {
    Pace p; if (ext != LOW && ext != HIGH) return p;
    const int s = ext == HIGH ? off::ASK : off::BID;                  // the RESTING side the attackers hit
    const long long te = ext == HIGH ? a.tHi : a.tLo;
    if (te < 0 || a.tFirst < 0 || a.tLast < 0 || te < a.tFirst) return p;
    const long long before = ext == HIGH ? a.cumHi[s] : a.cumLo[s], secsBefore = te - a.tFirst + 1;
    const long long after = a.hitC[s] - before, secsAfter = a.tLast - te;
    if (before > 0 && secsBefore > 0 && secsAfter >= E_MIN_SECS && after >= 0)
        p.e100 = (long long)std::llround(100.0 * ((double)after / (double)secsAfter) / ((double)before / (double)secsBefore));
    if (te - a.tFirst >= INTO_MIN_SECS) {
        const long long mid = a.tFirst + (te - a.tFirst + 1) / 2; long long h1 = 0, h2 = 0;
        for (std::map<long long, long long>::const_iterator q = a.hitSec[s].lower_bound(a.tFirst); q != a.hitSec[s].end() && q->first <= te; ++q) (q->first < mid ? h1 : h2) += q->second;
        if (h1 > 0) p.into100 = (long long)std::llround(100.0 * (double)h2 / (double)h1);
    }
    return p;
}

// ---- the 5-candle extreme and the leg's sweeps ------------------------------------------------------------------------------------
// hi / lo = the candles' highs / lows in ticks, oldest first, the candle itself LAST; true when it is >= every one of the 5 before (a high)
inline bool extremeOf(const std::vector<long long>& hi, const std::vector<long long>& lo, bool& isHigh, bool& isLow) {
    isHigh = isLow = false;
    if (hi.size() < (size_t)EXT_BARS + 1 || lo.size() != hi.size()) return false;
    const size_t n = hi.size() - 1; isHigh = isLow = true;
    for (size_t j = n - EXT_BARS; j < n; ++j) { if (hi[j] > hi[n]) isHigh = false; if (lo[j] < lo[n]) isLow = false; }
    return isHigh || isLow;
}
// the biggest sweep of the LEG_BARS candles before on one RESTING side: prior = their AS records (nullptr = no record = unknown)
struct AS { long long t = 0; long long sw[2] = {0, 0}, big[2] = {0, 0}, sec[2] = {-1, -1}; long long tier[3] = {0, 0, 0}; };
inline bool legMax(const std::vector<const AS*>& prior, int restSide, long long& mx) {
    mx = 0; if (prior.size() < (size_t)LEG_BARS) return false;
    for (size_t k = prior.size() - LEG_BARS; k < prior.size(); ++k) { if (!prior[k]) return false; mx = std::max(mx, prior[k]->sw[restSide]); }
    return true;
}

// ---- K / S decided at the close of a 5-candle extreme candle ------------------------------------------------------------------------
struct KRead { long long t = 0; int side = -1; bool K = false; long long lots = 0, sec = -1, tier[3] = {0, 0, 0}; int ch = 0; bool init = false; Pace p; };
struct SRead { long long t = 0; int side = -1; bool S = false; long long lots = 0, nPx = 0, lo = 0, hi = 0, sec = -1, prior = 0; double tick = 0; Pace p; };
// tiers t[0..2] = S20 / S10 / S5 (ok = the pool has them); false = not decided (tiers learning / no order)
inline bool decideK(const off::Rec& r, long long t, int ext, bool tiersOk, const long long tier[3], int ch, bool init, const Pace& p, KRead& out) {
    if ((ext != LOW && ext != HIGH) || !tiersOk || tier[1] <= 0 || r.partial) return false;
    const int s = ext == HIGH ? off::ASK : off::BID;
    KRead k; k.t = t; k.side = ext; k.lots = r.big[s].lots; k.sec = r.big[s].sec; for (int q = 0; q < 3; ++q) k.tier[q] = tier[q]; k.ch = ch; k.init = init; k.p = p;
    k.K = k.lots >= tier[1] && !init;
    out = k; return true;
}
inline bool decideS(const off::Rec& r, long long t, int ext, bool priorKnown, long long priorMax, double tick, const Pace& p, SRead& out) {
    if ((ext != LOW && ext != HIGH) || !priorKnown || r.partial) return false;
    const int s = ext == HIGH ? off::ASK : off::BID;
    SRead w; w.t = t; w.side = ext; w.lots = r.sweep[s].lots; w.nPx = r.sweep[s].nPx; w.lo = r.sweep[s].lo; w.hi = r.sweep[s].hi; w.sec = r.sweep[s].sec; w.prior = priorMax; w.tick = tick; w.p = p;
    w.S = w.lots > 0 && w.lots > priorMax;
    out = w; return true;
}
inline AS asOf(const off::Rec& r, long long t, const long long tier[3]) {
    AS a; a.t = t; for (int s = 0; s < 2; ++s) { a.sw[s] = r.sweep[s].lots; a.big[s] = r.big[s].lots; a.sec[s] = r.big[s].sec; } for (int q = 0; q < 3; ++q) a.tier[q] = tier[q]; return a;
}
// L on one RESTING side: one order >= S20 but < S10 (tiers frozen in the AS line; unknown tiers = no L)
inline bool lOf(const AS& a, int restSide) { return a.tier[0] > 0 && a.tier[1] > 0 && a.big[restSide] >= a.tier[0] && a.big[restSide] < a.tier[1]; }

// ---- the record lines (BidAskStats session record, after the candle rows) -------------------------------------------------------------
//   K|candle_key|side|K(0/1)|lots|second|s20|s10|s5|channel|initiative(0/1)|e_x100|into_x100                                  (13)
//   S|candle_key|side|S(0/1)|lots|prices (the span when over 24)|lo_ticks|hi_ticks|second|leg_max_lots|e_x100|into_x100|tick                          (13)
//   AS|candle_key|sweep_bids_hit|sweep_offers_hit|big_bids_hit|big_offers_hit|sec_bids|sec_offers|s20|s10|s5                   (11)
// side: 0 = a low (sellers attack), 1 = a high (buyers attack); every decided candle with the order feed gets one AS line.
inline std::string kLine(const KRead& k) {
    char b[320]; std::snprintf(b, sizeof b, "K|%lld|%d|%d|%lld|%lld|%lld|%lld|%lld|%d|%d|%lld|%lld", k.t, k.side, k.K ? 1 : 0, k.lots, k.sec, k.tier[0], k.tier[1], k.tier[2], k.ch, k.init ? 1 : 0, k.p.e100, k.p.into100);
    return b;
}
inline std::string sLine(const SRead& w) {
    char b[360]; std::snprintf(b, sizeof b, "S|%lld|%d|%d|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%.10g", w.t, w.side, w.S ? 1 : 0, w.lots, w.nPx, w.lo, w.hi, w.sec, w.prior, w.p.e100, w.p.into100, w.tick);
    return b;
}
inline std::string asLine(const AS& a) {
    char b[320]; std::snprintf(b, sizeof b, "AS|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld", a.t, a.sw[0], a.sw[1], a.big[0], a.big[1], a.sec[0], a.sec[1], a.tier[0], a.tier[1], a.tier[2]);
    return b;
}
inline bool nonNeg(const std::string& f, long long& v) { return dt::parseLongLong(f, v) && v >= 0; }
inline bool paceTok(const std::string& f, long long& v) { return dt::parseLongLong(f, v) && v >= -1 && v <= 100000000; }
inline bool parseK(const std::string& s, KRead& K) {
    if (s.compare(0, 2, "K|") != 0) return false;
    const std::vector<std::string> f = dt::barFields(s); if (f.size() != 13) return false;
    KRead k; long long side = 0, ch = 0;
    if (!dt::parseLongLong(f[1], k.t) || k.t <= 0 || !dt::parseLongLong(f[2], side) || (side != LOW && side != HIGH)) return false;
    if ((f[3] != "0" && f[3] != "1") || (f[10] != "0" && f[10] != "1")) return false;
    if (!nonNeg(f[4], k.lots) || !dt::parseLongLong(f[5], k.sec) || k.sec < -1 || !nonNeg(f[6], k.tier[0]) || !nonNeg(f[7], k.tier[1]) || !nonNeg(f[8], k.tier[2])) return false;
    if (!dt::parseLongLong(f[9], ch) || ch < 0 || ch > 2 || !paceTok(f[11], k.p.e100) || !paceTok(f[12], k.p.into100)) return false;
    k.side = (int)side; k.K = f[3] == "1"; k.init = f[10] == "1"; k.ch = (int)ch;
    if (k.K && (k.lots < k.tier[1] || k.tier[1] <= 0 || k.init)) return false;   // a K line must satisfy its own rule
    K = k; return true;
}
inline bool parseS(const std::string& s, SRead& S) {
    if (s.compare(0, 2, "S|") != 0) return false;
    const std::vector<std::string> f = dt::barFields(s); if (f.size() != 13) return false;
    SRead w; long long side = 0;
    if (!dt::parseLongLong(f[1], w.t) || w.t <= 0 || !dt::parseLongLong(f[2], side) || (side != LOW && side != HIGH) || (f[3] != "0" && f[3] != "1")) return false;
    if (!nonNeg(f[4], w.lots) || !nonNeg(f[5], w.nPx) || !nonNeg(f[6], w.lo) || !nonNeg(f[7], w.hi) || !dt::parseLongLong(f[8], w.sec) || w.sec < -1 || !nonNeg(f[9], w.prior)) return false;
    if (!paceTok(f[10], w.p.e100) || !paceTok(f[11], w.p.into100) || !dt::parseFiniteDouble(f[12], w.tick) || !(w.tick > 0) || w.lo > w.hi) return false;
    w.side = (int)side; w.S = f[3] == "1";
    if (w.S && !(w.lots > w.prior)) return false;
    S = w; return true;
}
inline bool parseAS(const std::string& s, AS& A) {
    if (s.compare(0, 3, "AS|") != 0) return false;
    const std::vector<std::string> f = dt::barFields(s); if (f.size() != 11) return false;
    AS a; if (!dt::parseLongLong(f[1], a.t) || a.t <= 0) return false;
    for (int q = 0; q < 2; ++q) { if (!nonNeg(f[(size_t)2 + q], a.sw[q]) || !nonNeg(f[(size_t)4 + q], a.big[q]) || !dt::parseLongLong(f[(size_t)6 + q], a.sec[q]) || a.sec[q] < -1) return false; }
    for (int q = 0; q < 3; ++q) if (!nonNeg(f[(size_t)8 + q], a.tier[q])) return false;
    A = a; return true;
}

// ---- the badge ------------------------------------------------------------------------------------------------------------------
// colourKey: LOW = green (bullish), HIGH = red (bearish). K / S / G / Tr at their extreme (a high = red); R by the trapped side (buyers
// trapped above a high = red); L by meaning: buyers at a 5-candle high = red, sellers at a 5-candle low = green, else buyers green / sellers red.
struct BadgeIn { int S = -1, K = -1, G = -1, Tr = -1, R = -1; int L = -1; };   // the side / colour key of each letter, -1 = none
struct Letter { std::string s; int colourKey = LOW; };
inline std::vector<Letter> letters(const BadgeIn& b) {
    std::vector<Letter> v; auto add = [&](const char* s, int key) { if (key == LOW || key == HIGH) { Letter L; L.s = s; L.colourKey = key; v.push_back(L); } };
    add("S", b.S); add("K", b.K); add("G", b.G); add("Tr", b.Tr); add("R", b.R); add("L", b.L);
    return v;
}
// the L colour of an order on RESTING side restSide (BID = sellers hit, ASK = buyers lifted) on a candle that is a 5-candle high / low
inline int lColour(int restSide, bool isHigh, bool isLow) {
    if (restSide == off::ASK) return isHigh ? HIGH : LOW;            // buyers: bullish, but buying into a high = red
    return isLow ? LOW : HIGH;                                        // sellers: bearish, but selling into a low = green
}
inline std::vector<dt::BadgeSeg> badgeSegs(const BadgeIn& b, std::vector<std::string>* dropped = nullptr, int maxLetters = BADGE_MAX) {
    std::vector<Letter> v = letters(b);
    if (dropped) dropped->clear();
    if ((int)v.size() > maxLetters) { if (dropped) for (size_t k = (size_t)maxLetters; k < v.size(); ++k) dropped->push_back(v[k].s); v.resize((size_t)maxLetters); }
    std::vector<dt::BadgeSeg> o;
    for (size_t k = 0; k < v.size(); ++k) { if (!o.empty()) o.back().s += "|"; dt::BadgeSeg s; s.s = v[k].s; s.colourKey = v[k].colourKey; o.push_back(s); }
    return o;                                                         // no "?" (Rassul 20:11-20:23): the hover says "still learning"
}

// ---- the hover: one block per letter (also the ones cut from the badge) ------------------------------------------------------------
// rows: the question, what happened (the record's numbers), Use:, Example:, Tested:, the combination lines, Status:
inline std::string priceTicks(long long t, double tick) { return dt::priceText((double)t * tick, tick); }
inline void comboRows(std::vector<std::string>& rows, const std::string& me, const std::vector<std::string>& others, const std::vector<std::string>& defense) {
    for (size_t k = 0; k < others.size(); ++k) { const std::string key = pairKey(me, others[k]); if (!key.empty()) rows.push_back("   with " + others[k] + ": " + scoreText(key)); }
    for (size_t k = 0; k < defense.size(); ++k) {
        const std::string key = pairKey(me, defense[k]);
        rows.push_back("   with Defense " + defense[k] + ": " + (key.empty() ? std::string("no combined score yet (learning)") : scoreText(key)));
    }
}
inline std::string testedRow(const std::string& letter, const std::string& market) {
    const Score* s = scoreOf(letter); if (!s) return std::string();
    std::string o = "   Tested: held " + pct(s->held) + " vs " + pct(s->ctl) + " matched (n " + dt::lotsText(s->n) + ", 7 markets)";
    const MarketLift* m = liftOf(market);
    if (m && (letter == "K" || letter == "S")) o += "; " + market + " " + signed1(letter == "K" ? m->kLift : m->sLift) + " pts (n " + std::to_string(letter == "K" ? m->kN : m->sN) + ")";
    return o;
}
inline dt::HoverBlock hoverK(const KRead& k, const std::string& at, const std::string& market, const std::vector<std::string>& others, const std::vector<std::string>& defense, long long n) {
    const bool high = k.side == HIGH; dt::HoverBlock b; b.letter = "K";
    b.rows.push_back(std::string("K  Big aggressor - did one big ") + (high ? "buyer hit the high?" : "seller hit the low?"));
    const std::string tier = k.lots >= k.tier[2] && k.tier[2] > 0 ? "S5 = " + dt::lotsText(k.tier[2]) : "S10 = " + dt::lotsText(k.tier[1]);
    b.rows.push_back(std::string("   One ") + (high ? "buy" : "sell") + " order of " + dt::lotsText(k.lots) + " lots" + (at.empty() ? std::string() : " at " + at) + " (Big Lot " + tier + ")");
    b.rows.push_back(std::string("   Use: a climax push into the ") + (high ? "high" : "low") + " - the turn is on when the " + (high ? "buyers" : "sellers") + " then quit (G) or get absorbed (A); not a "
                     + (high ? "sell" : "buy") + " signal alone");
    b.rows.push_back("   Example: NQ 10-21 11:39 AM, one 173-lot buy into the 30,110.75 high, then the buyers pulled their bids (G): -70 pts by 11:44");
    b.rows.push_back(testedRow("K", market));
    comboRows(b.rows, "K", others, defense);
    b.rows.push_back(dt::statusText(false, n)); return b;
}
inline dt::HoverBlock hoverS(const SRead& w, const std::string& at, const std::string& market, const std::vector<std::string>& others, const std::vector<std::string>& defense, long long n) {
    const bool high = w.side == HIGH; dt::HoverBlock b; b.letter = "S";
    b.rows.push_back("S  Sweep of the leg - did one order rip through several prices?");
    std::string what = std::string("   One ") + (high ? "buy" : "sell") + " order took " + std::to_string(w.nPx) + " prices (" + priceTicks(high ? w.lo : w.hi, w.tick) + " -> " + priceTicks(high ? w.hi : w.lo, w.tick)
                       + ", " + dt::lotsText(w.lots) + " lots" + (at.empty() ? std::string() : " at " + at) + ")";
    if (w.prior > 0) { char x[24]; std::snprintf(x, sizeof x, "%.1fx", (double)w.lots / (double)w.prior); what += std::string(", ") + x + " the biggest sweep of the 10 candles before (" + dt::lotsText(w.prior) + " lots)"; }
    else what += ", the first sweep on that side in 10 candles";
    b.rows.push_back(what);
    b.rows.push_back("   Use: the last big push of a move; fade it only with G / A / Tr");
    b.rows.push_back("   Example: NQ 10-21 11:39 AM, one buy order through 39 prices, 2.9x the leg's biggest sweep (60 lots), at the day's high");
    b.rows.push_back(testedRow("S", market));
    comboRows(b.rows, "S", others, defense);
    b.rows.push_back(dt::statusText(false, n)); return b;
}
inline dt::HoverBlock hoverL(int restSide, long long lots, const long long tier[3], const std::string& at, long long n) {
    const bool buy = restSide == off::ASK; dt::HoverBlock b; b.letter = "L";
    b.rows.push_back(std::string("L  Large order - did one big ") + (buy ? "buyer" : "seller") + " hit this candle?");
    b.rows.push_back(std::string("   One ") + (buy ? "buy" : "sell") + " order of " + dt::lotsText(lots) + " lots" + (at.empty() ? std::string() : " at " + at) + " (Big Lot S20 = " + dt::lotsText(tier[0])
                     + ", under S10 = " + dt::lotsText(tier[1]) + ")");
    b.rows.push_back("   Use: size is active here - watch whether price follows it; at a 5-candle high / low an S10 order becomes K");
    b.rows.push_back("   Example: ES RTH, one 300-lot sell (S20 273, S10 369) in a quiet candle - size, not yet a climax");
    b.rows.push_back(dt::statusText(false, n)); return b;
}
// R: dir +1 = a failed upside break (buyers trapped above the level), -1 = failed downside break
inline dt::HoverBlock hoverR(int dir, const std::string& level, int ticks, int bars, const std::string& burst, long long n) {
    const bool up = dir > 0; dt::HoverBlock b; b.letter = "R";
    b.rows.push_back(std::string("R  Stop run - did ") + (up ? "buyers get trapped above " : "sellers get trapped below ") + level + "?");
    b.rows.push_back(std::string("   Price ran ") + std::to_string(ticks) + (ticks == 1 ? " tick " : " ticks ") + (up ? "above " : "below ") + level + " (" + burst + "), then closed back inside "
                     + (bars <= 0 ? std::string("on the same candle") : "in " + std::to_string(bars) + (bars == 1 ? " candle" : " candles")));
    b.rows.push_back(std::string("   Use: the trapped ") + (up ? "buyers must sell" : "sellers must buy") + " - fuel " + (up ? "down" : "up") + "; it fails on a close back " + (up ? "above " : "below ") + level);
    b.rows.push_back(std::string("   Example: ES breaks PDH by 6 ticks on a buy spike, closes back under it next candle - buyers trapped"));
    b.rows.push_back(dt::statusText(false, n)); return b;
}
// G / Tr: lsBookFlow's / the Delta Profile's blocks (their own wording, unchanged) + Example, Tested and the combination lines
inline void addRowsBeforeStatus(dt::HoverBlock& b, const std::vector<std::string>& more) {
    std::vector<std::string>::iterator at = b.rows.end(); if (!b.rows.empty() && b.rows.back().compare(0, 10, "   Status:") == 0) at = b.rows.end() - 1;
    b.rows.insert(at, more.begin(), more.end());
}
inline dt::HoverBlock hoverG(const dt::GiveUp& g, const std::vector<std::string>& others, const std::vector<std::string>& defense, long long n) {
    dt::HoverBlock b = dt::hoverG(g, false, n); std::vector<std::string> m;
    m.push_back("   Example: NQ 10-21 11:39 AM, after the high 19,226 bid lots pulled near price (3.6x normal) vs 18,673 added");
    m.push_back(testedRow("G", ""));
    comboRows(m, "G", others, defense);
    addRowsBeforeStatus(b, m); return b;
}
inline dt::HoverBlock hoverTr(bool top, double lots, double x, const std::vector<std::string>& others, const std::vector<std::string>& defense, long long n) {
    dt::HoverBlock b = dt::hoverTr(top, lots, x, false, n); std::vector<std::string> m;
    m.push_back(std::string("   Example: buyers bought 640 lots above the old high, the candle closed back under it - they are underwater"));
    m.push_back(testedRow("Tr", ""));
    comboRows(m, "Tr", others, defense);
    addRowsBeforeStatus(b, m); return b;
}
// the hover-only pace line: "Hover only:  E fading 0.2x after the high  |  speeding up 2.3x into it (2nd vs 1st half)"; "" = nothing notable
inline std::string paceRow(const Pace& p, int side) {
    std::string a, c; char x[24];
    if (p.e100 >= 0 && p.e100 <= E_SHOW100) { std::snprintf(x, sizeof x, "%.1fx", (double)p.e100 / 100.0); a = std::string("E fading ") + x + " after the " + (side == HIGH ? "high" : "low"); }
    if (p.into100 >= INTO_SHOW100) { std::snprintf(x, sizeof x, "%.1fx", (double)p.into100 / 100.0); c = std::string("speeding up ") + x + " into it (2nd vs 1st half)"; }
    if (a.empty() && c.empty()) return std::string();
    return "Hover only:  " + a + (a.empty() || c.empty() ? "" : "  |  ") + c;
}
// the whole hover: the header, one block per letter, the pace line
inline std::string hoverText(const std::string& badge, const std::string& when, int meaning, const std::vector<std::string>& dropped, const std::vector<dt::HoverBlock>& blocks, const std::string& pace) {
    std::string t = "Attack " + badge;
    if (!when.empty()) t += "   " + when;
    if (meaning == HIGH) t += "   (bearish = red)"; else if (meaning == LOW) t += "   (bullish = green)";
    if (!dropped.empty()) { t += "  (+ "; for (size_t k = 0; k < dropped.size(); ++k) t += (k ? ", " : "") + dropped[k]; t += ": no room on the badge)"; }
    for (size_t k = 0; k < blocks.size(); ++k) { t += "\n"; for (size_t r = 0; r < blocks[k].rows.size(); ++r) if (!blocks[k].rows[r].empty()) t += "\n" + dt::wrapRow(blocks[k].rows[r]); }
    if (!pace.empty()) t += "\n\n" + dt::wrapRow(pace);
    return t;
}
// the fit cascade: level 1 drops Use / Example rows, level 2 also Tested / Status, level 3 also the attacker-letter combinations and the
// pace line (the question, the numbers and the combination with a Defense letter always stay)
inline std::string compact(const std::string& t, int level) {
    if (level <= 0) return t;
    std::string o, cur; bool first = true, dropping = false;
    for (size_t i = 0; i <= t.size(); ++i) {
        if (i < t.size() && t[i] != '\n') { cur += t[i]; continue; }
        const bool cont = cur.compare(0, 5, "     ") == 0;
        const bool drop = cont ? dropping : (cur.compare(0, 7, "   Use:") == 0 || cur.compare(0, 11, "   Example:") == 0
                                             || (level >= 2 && (cur.compare(0, 10, "   Status:") == 0 || cur.compare(0, 10, "   Tested:") == 0))
                                             || (level >= 3 && cur.compare(0, 8, "   with ") == 0 && cur.compare(0, 16, "   with Defense ") != 0)
                                             || (level >= 3 && cur.compare(0, 11, "Hover only:") == 0));
        if (!cont) dropping = drop;
        if (!drop) { if (!first) o += '\n'; o += cur; first = false; }
        cur.clear();
    }
    return o;
}
// the pane's read line without the "?" (Rassul 20:11-20:23 "remove the ? ... e.g. strong resistance coming in ?")
inline std::string noQ(std::string s) {
    for (;;) { const size_t p = s.find(" ?"); if (p == std::string::npos) break; s.erase(p, 2); }
    while (!s.empty() && s.back() == '?') s.pop_back();
    return s;
}
inline void noQ(br::Read& r) { r.head = noQ(r.head); for (size_t k = 0; k < r.opt.size(); ++k) r.opt[k] = noQ(r.opt[k]); }

// ---- the other panes' records read here (never re-decided): lsBookFlow's turns file (G lines + the Defense letters of each candle) and
// the Delta Profile's TR lines. Both files are append-only between rewrites: read from where the last read stopped; a shorter file = read
// again from the start. Bounded (8 MB per read); only whole lines.
struct Defense { std::set<std::pair<int, long long> > A; std::set<long long> H, W; std::map<long long, char> Q; std::map<std::pair<int, long long>, std::string> M; };
struct Others {
    std::map<std::pair<int, long long>, dt::GiveUp> G;               // [side, candle key]
    std::map<std::pair<int, long long>, trapl::Rec> Tr;              // [side, candle key]
    Defense D; long long lines = 0, bad = 0;
    void turnsLine(const std::string& ln) {
        dt::GiveUp g; if (dt::parseG(ln, g)) { G[std::make_pair(g.side, g.t)] = g; ++lines; return; }
        dt::Counter h; if (dt::parseH(ln, h)) { if (h.H) D.H.insert(h.at); ++lines; return; }
        dt::Retest q; if (dt::parseQ(ln, q)) { if (q.res == 'L' || q.res == 'X') D.Q[q.at] = q.res; ++lines; return; }
        mvl::Read m; if (mvl::parseM(ln, m)) { std::string s; const char b = mvl::letterOf(m, mvl::BIDS), o = mvl::letterOf(m, mvl::OFFERS); if (b) s += b; if (o) s += o; if (!s.empty()) D.M[std::make_pair(m.ext, m.t)] = s; ++lines; return; }
        dt::Line L; if (dt::parseLine(ln, L) && L.kind == 'T') { if (L.A) D.A.insert(std::make_pair(L.side, L.t)); ++lines; return; }
        if (ln.compare(0, 2, "W|") == 0) { const std::vector<std::string> f = dt::barFields(ln); long long key = 0; if (f.size() >= 9 && dt::parseLongLong(f[1], key) && key > 0 && f[8].find('W') != std::string::npos && f[2] == "0") D.W.insert(key); ++lines; return; }
    }
    void trLine(const std::string& ln) {
        if (ln.compare(0, 3, "TR|") != 0) return;
        trapl::Rec r; if (!trapl::parse(ln, r)) { ++bad; return; }
        const std::pair<int, long long> k(r.top ? HIGH : LOW, (long long)r.barRaw);
        if (!Tr.count(k)) Tr[k] = r;                                  // the first line per candle + side wins (as the Delta Profile)
    }
    void trim() { while (G.size() > 3000) G.erase(G.begin()); while (Tr.size() > 2000) Tr.erase(Tr.begin()); }
    // the Defense letters on candle key for an attacker letter at extreme `side` (A on the same extreme; H / L / X / W / P / B any side)
    std::vector<std::string> defenseAt(long long key, int side) const {
        std::vector<std::string> v;
        if (D.A.count(std::make_pair(side, key))) v.push_back("A");
        if (D.H.count(key)) v.push_back("H");
        std::map<long long, char>::const_iterator q = D.Q.find(key); if (q != D.Q.end()) v.push_back(std::string(1, q->second));
        for (int s = 0; s < 2; ++s) { std::map<std::pair<int, long long>, std::string>::const_iterator m = D.M.find(std::make_pair(s, key)); if (m != D.M.end()) for (char c : m->second) { const std::string x(1, c); if (std::find(v.begin(), v.end(), x) == v.end()) v.push_back(x); } }
        if (D.W.count(key)) v.push_back("W");
        return v;
    }
};
// read the new whole lines of path from off0 (updated); fn(line) per line; false = nothing new / unreadable
template <class Fn> inline bool tailRead(const std::string& path, long long& off0, Fn fn, long long maxRead = 8 * 1024 * 1024) {
    if (path.empty()) return false;
    std::ifstream in(path.c_str(), std::ios::binary); if (!in.is_open()) return false;
    in.seekg(0, std::ios::end); const long long size = (long long)in.tellg(); if (size < 0) return false;
    if (size < off0) off0 = 0;                                        // rewritten (trimmed): read again
    if (size == off0) return false;
    if (size - off0 > maxRead) off0 = size - maxRead;                 // bounded: the newest part only
    in.seekg(off0, std::ios::beg); std::string buf((size_t)(size - off0), '\0'); in.read(&buf[0], (std::streamsize)buf.size());
    if (!in && in.gcount() != (std::streamsize)buf.size()) return false;
    const size_t last = buf.rfind('\n'); if (last == std::string::npos) return false;
    off0 += (long long)last + 1;
    size_t a = 0;
    while (a <= last) { const size_t b = buf.find('\n', a); std::string ln = buf.substr(a, b - a); a = b + 1; if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (!ln.empty()) fn(ln); }
    return true;
}

} // namespace atk
#endif
