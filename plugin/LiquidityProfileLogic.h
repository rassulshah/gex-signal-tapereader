#ifndef LS_LIQUIDITY_PROFILE_LOGIC_H
#define LS_LIQUIDITY_PROFILE_LOGIC_H
// LiquidityProfileLogic.h  (lsLiquidityProfile 0.1.0, 2026-10-11) -- the SDK-free engine of the Liquidity Profile. Tested by
// test_liquidityprofile_logic.cpp (g++ / MSVC, no Investor/RT needed).
//
// WHAT IT DOES (Rassul 2026-10-10: "get the liquidity profile done and the signals that support absorption ... so i can trade monday"):
//   1. BOOK    the resting size per price on each side, from the chart's own depth (MARKET_DEPTH snapshots; native, no outside
//              dependency). Order-by-order (DBO) events are only RECORDED for now: their Rithmic / CQG semantics (does a FILL also
//              delete? is a DEL a cancel?) are not verified yet, so they never move the book here (fail closed).
//   2. TRADES  aggressive volume per price from the chart's own volume-at-price (VAP) of the bar in progress: the increase since the
//              last read, buy = lifted the offer, sell = hit the bid.
//   3. FLOW    between two book reads, per price: executed (trades against that side), PULLED (size that left without trading),
//              ADDED, REFILL (traded more than was shown = reloaded). Only prices inside BOTH reads' depth window are counted, so a
//              level that scrolls out of a 10-level book is never mistaken for a pull.
//   4. ZONES   the size summed over a zone-wide band (width 2*hw+1 ticks, hw = clamp(round(k*ATR/tick), hwMin, hwMax) - the study's
//              MEDIUM zone per market, lra.mbo_fetch.ZONES) at every price; the top N local peaks per side within reach; tracked
//              by centre (drift <= hw). x normal = size / the normal band size for that 30-min slot (the nightly study's walk-
//              forward median, LRA-LiqNorms-<MKT>.csv), else the session's own median band size so far.
//   5. TOUCH   price trades into a durable zone (rested >= persist) from outside on the attacking side -> an ATTEMPT:
//              shown = the zone's size at the last book before arrival; then eaten = attackers' volume in the zone / shown,
//              stayed = 1 - pulled / shown, E = exhaustion (attackers' 2nd-half volume < 0.5 x their 1st half since arrival),
//              resilience = how much of what the zone lost was rebuilt.
//   6. MARKS   (0.5.0) two pane signals, decided on CLOSED bars only and never changed afterwards (no repaint):
//              Exh  (aggression pane) the attackers' 2nd-half / 1st-half volume since arrival under 0.5, drawn with that number
//              Pull (defense pane) the defenders pulled this market's share of the shown size, not replaced, while attacked
//              Each is drawn only in markets where the nightly study confirmed it (SIGNAL lines); unconfirmed = not drawn.
//              Holds / breaks / expiries and the O / R crosses are logged for the study, never drawn.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <locale>
#include <deque>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace lqp {

static const char* const VERSION = "0.9.0";
typedef long long Tick;
typedef long long Ms;
enum Side { BID = 0, ASK = 1 };                 // BID = buy orders below price (support), ASK = sell orders above (resistance)

struct Params {
    double tick = 0.01;
    double k = 0.25; int hwMin = 4, hwMax = 8;  // MEDIUM zone half-width rule in ticks (CL default; the norms file sets it per market)
    int topN = 3;                               // zones per side
    double reachAtr = 6.0;                      // zones are looked for within reachAtr x ATR of price
    Ms persistMs = 30000;                       // a zone must have rested 30 s (durable, not flicker) to be bright / touchable
    Ms graceMs = 10000;                         // a tracked zone not seen for 10 s is dropped (unless in an attempt)
    Ms decisionMs = 180000;                     // the study's decision time: arrival + 3 min
    int maxBars = 20;                           // an attempt lives 20 bars after arrival
    double eatenBreak = 1.0;                    // LX: eaten >= 1x the size shown (Q02 / Q17)
    double exhaust = 0.5;                       // E: attackers' 2nd-half volume < 0.5 x the 1st half (Q22)
    double exhShow = 0.5;                       // (0.4.1, Rassul 2026-10-10 23:44 "i want to see weakness, especially under .5") Exh prints under this
    double pullShare = 0.5;                     // (0.3.1) PULL? / P / LX P: share of the shown size pulled. eatenBreak, exhaust and pullShare are
                                                // set PER MARKET from the data (LRA-LiqNorms-<MKT>.csv PARAM lines, lra.liq_params: chosen on the
                                                // build days, kept only if the later validate days agree; else these defaults)
    Ms oldMs = 600000;                          // (0.5.0, Rassul 2026-10-11 "just add the old order mark") OLD = resting 10 min+
    double oldShow = 0.5;                       // a zone is marked old when half+ of its size is old (study old_share >= 0.5, Q-old)
    // (0.8.0, Rassul 2026-10-11 "now we need something that shows sellers stepped in after that" / "use s" / "rename step in to counter")
    // s = COUNTER: after e, a bar where the defenders out-traded the attackers near the zone at counterX x their normal pace per tick
    double counterX = 1.25;                     // the line (PARAM counter_x per market, lra.liq_params; this default until the data sets it)
    int counterBars = 10;                       // s is looked for up to 10 bars after the e bar (lra.liq_params COUNTER_BARS)
    Ms barMs = 180000;                          // the chart's bar length (the STEPPING IN warning stays 2 bars after an s)
    double minShown = 1.0;                      // an attempt needs some size shown at arrival
    double minAttack = 1.0;                     // E needs attackers in the first half
};

inline Tick toTick(double price, double tick) { return (Tick)std::llround(price / tick); }
inline bool finite(double v) { return v == v && v < std::numeric_limits<double>::infinity() && v > -std::numeric_limits<double>::infinity(); }

inline int halfWidth(const Params& p, double atrPrice) {
    double a = atrPrice > 0 && finite(atrPrice) ? atrPrice : 15 * p.tick;
    int hw = (int)std::llround(p.k * a / p.tick);
    return std::max(p.hwMin, std::min(p.hwMax, hw));
}

// ---- normal band size per 30-min Central slot (slot = minute of day / 30), from the nightly file; else the session's median
struct Norms {
    std::map<int, double> median;               // slot -> median band size (contracts)
    bool loaded = false;
    std::vector<double> session;                // fallback: band sizes sampled this session (capped)
    double at(int slot) const {
        std::map<int, double>::const_iterator it = median.find(slot);
        if (it != median.end() && it->second > 0) return it->second;
        if (session.size() >= 30) {
            std::vector<double> v(session); std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
            return v[v.size() / 2] > 0 ? v[v.size() / 2] : 0;
        }
        return 0;
    }
    void sample(double band) { if (band > 0) { if (session.size() >= 5000) session.erase(session.begin(), session.begin() + 1000); session.push_back(band); } }
};

struct Zone {
    long long id = 0; int side = BID; Tick center = 0, lo = 0, hi = 0; int hw = 0;
    double size = 0, peak = 0; Ms born = 0, seen = 0;
    bool inAttempt = false;
    bool durable(Ms now, Ms persist) const { return now - born >= persist; }
};

struct FlowEvt { Ms t; int side; Tick px; double executed, pulled, added, refill; };
struct TradeEvt { Ms t; Tick px; double buy, sell; };

struct Stats { double attack = -1, eaten = -1, exh = -1, pulled = -1, rebuilt = -1, counter = -1; int side = 0; bool active = false; };

struct Attempt {
    long long zoneId = 0; int side = BID; Tick lo = 0, hi = 0; Ms tA = 0; int barA = 0;
    double shown = 0, attackVol = 0, counterVol = 0, pulled = 0, added = 0, refill = 0, minSize = 0, startSize = 0, curSize = 0;
    std::vector<std::pair<Ms, double>> attacks;  // (time, attacker contracts) inside the zone since arrival
    bool breachTrade = false;                    // an attacker trade beyond the far edge
    bool done = false; std::string end; Ms tEnd = 0;
    double oldShare = -1;                        // (0.5.0) the zone's old-order share at arrival (-1 = not known), logged for the study
    // (0.2.0) the absorption STORY per bar, for the aggression / defense panes (lsLiquidityFlow) and the nightly study:
    // attack / defend = aggressive contracts in the zone that bar (attackers vs the defending side turning aggressive);
    // rep / pul = replenished (added + refill) and pulled since arrival, cumulative, as of that bar
    // (0.3.0, Rassul 2026-10-10 23:11 "buyers are always green and sellers are always red ... buyers are stronger in aggression and
    // defense") defNet / oppNet = NET resting size since arrival (added + refilled - pulled; fills excluded) of the DEFENDING side in the
    // zone and of the OTHER side in the band of the same width just beyond it (offers above a bid zone, bids below an offer zone).
    // Net is churn-proof: an order placed and cancelled nets to zero. Pane: buyers' bids green, sellers' offers red, up = adding,
    // down = pulling.
    struct BarFlow { double attack = 0, defend = 0, rep = 0, pul = 0, defNet = 0, oppNet = 0; bool book = false; };
    double defNet = 0, oppNet = 0; Tick oppLo = 0, oppHi = 0;
    std::map<int, BarFlow> flowBars;
    std::map<int, Stats> barStats;               // (0.7.0) the pane statistics as of each closed bar (the mouse-over read of a past touch)
    int barEW = -1, barPW = -1, barT = -1;        // (0.7.0) e? / p? watches and t (trapped) - first bar each printed
    int barS = -1; Ms tS = 0; double sX = -1, exE = -1;   // (0.8.0) s (Counter) bar / time / its pace, and Exh when e printed
    double counterNow = -1;                      // (0.8.0) the Counter readout: the defenders' pace per tick on the latest closed bar
    int barE = -1, barO = -1, barP = -1;         // first bar the attackers' pace halved (E), the defenders went on offense (O), the zone was pulled (P)
    int barR = -1, barX = -1;                    // (0.3.0) the defenders' net resting size crossed above the other side's (R); O and R both done = X
    bool defendedFirst = false;                  // (0.3.0) the attackers' volume since arrival led the defenders' before O
    bool pulledFirst = false;                    // (0.3.0) the other side's net resting size was at or above the defenders' before R
};

struct Mark {
    std::string code;      // (0.5.0) Exh / Pull only (Rassul 2026-10-11 "remove whatever doesn't work and keep what works")
    std::string reasons;   // unused since 0.4.0 (kept in the record format)
    int side = BID; long long barTime = 0;       // the closed bar's time (host clock, matched on redraw)
    Tick lo = 0, hi = 0; double eaten = 0, stayed = 0, exhaust = -1, resil = -1, shown = 0; Ms tA = 0, tDecided = 0;
    double value = -1;     // (0.4.1) the number drawn with the code: Exh = 2nd / 1st half pace, Pull = share pulled
};

// (0.6.0, Rassul 2026-10-11) the pane statistics, in the order the process happens (Metric: now (threshold) on the panes):
//   AGGRESSION  attack  the attackers' biggest chart bar inside the zone since arrival, per tick of zone width, vs their average volume per
//                       bar per tick that bar traded over the 20 bars before the arrival bar (1.0x = normal, 2.4x = 2.4 times normal)
//               eaten   the attackers' volume / EVERYTHING that was there to trade against (size at arrival + new size + refills): 0..1
//               exh     the attackers' 2nd-half / 1st-half volume since arrival (the Exh signal's number)
//   DEFENSE     pulled  size pulled (not traded) / size shown at arrival
//               rebuilt what came back / what the zone lost (resilience; -1 = it never lost size)
// Every number is measured exactly as the nightly study measures it (lra.mbo_study att_tick_x, eaten_total, exhaust_ratio, pull_snap,
// resilience), so the per-market lines it chooses apply to what is drawn.

// (0.7.0) the per-market lines the host loaded from the nightly study; -1 = not confirmed for this market (that number is never used)
struct Lines { double attack = -1, eaten = -1, exh = -1, pulled = -1, rebuilt = -1, counter = -1; bool trapped = false; };
// (0.7.0, Rassul 2026-10-11) the warning on the price chart: one at a time, the strongest that applies, in plain words
struct Warn { std::string name, detail; int favour = 0; };   // favour: +1 buyers (green), -1 sellers (red), 0 not decided (grey)

struct Readout {         // the live read of the zone price is in (or the last one), for the column label
    bool active = false; int side = BID; Tick lo = 0, hi = 0; double eaten = 0, stayed = 1, exhaust = -1; bool exhausted = false;
    double pulled = 0, refill = -1;      // pulled share of what was shown; refill = new size that came in / what was taken (eaten + pulled)
    bool pulling = false, depleting = false;   // PULL?: half+ of the shown size left without trading and was not replaced; DEPLETING?: eaten 1x+, not replaced
};

inline double exhaustRatio(const std::vector<std::pair<Ms, double>>& a, Ms t0, Ms t1) {
    if (t1 <= t0) return -1;
    const Ms mid = t0 + (t1 - t0) / 2; double v1 = 0, v2 = 0;
    for (size_t i = 0; i < a.size(); ++i) { if (a[i].first < t0 || a[i].first > t1) continue; (a[i].first < mid ? v1 : v2) += a[i].second; }
    return v1 > 0 ? v2 / v1 : -1;
}

class Engine {
public:
    Params P;
    Norms norms;
    std::map<Tick, double> book[2];              // current resting size per price
    Tick win[2][2] = {{0, 0}, {0, 0}};           // [side][lo, hi] of the last book's depth window
    bool haveBook = false; Ms bookT = 0;
    std::map<Tick, double> pendingExec[2];       // traded against each side since the last book
    std::deque<FlowEvt> flow; std::deque<TradeEvt> tape;
    std::vector<Zone> zones; long long nextZone = 1;
    std::vector<Attempt> attempts; std::vector<Attempt> finished;
    std::vector<Mark> marks;
    Tick lastPx = 0; bool havePx = false; int hw = 4;
    // (0.5.0) OLD ORDERS. The chart's depth is the total size per price, not single orders, so an order's age cannot be read. The
    // estimate: the size at a price that never dropped below during the last oldMs (10 min) while that price stayed inside the depth
    // window = the size resting 10 min+ (orders join the back of the queue, so a level that never dipped kept its oldest size). A price
    // that left the window, or a gap in the depth feed, restarts its clock (age unknown = not old: conservative).
    struct Age { Ms since = 0; std::deque<std::pair<Ms, double>> q; };   // q: sliding minimum of the size over the last oldMs
    std::map<Tick, Age> age[2];
    void ageUpdate(int s, Tick px, Ms t, double size) {
        std::map<Tick, Age>::iterator it = age[s].find(px);
        if (it == age[s].end()) { it = age[s].insert(std::make_pair(px, Age())).first; it->second.since = t; }
        std::deque<std::pair<Ms, double>>& q = it->second.q;
        while (!q.empty() && q.back().second >= size) q.pop_back();
        q.push_back(std::make_pair(t, size));
        while (q.size() > 1 && q[1].first <= t - P.oldMs) q.pop_front();
        if (q.size() > 5000) q.pop_front();                               // bounded (checklist #33)
    }
    // the old size at a price now (-1 = unknown: not seen continuously for oldMs)
    double oldAt(int s, Tick px, Ms now) const {
        std::map<Tick, Age>::const_iterator it = age[s].find(px);
        if (it == age[s].end() || it->second.q.empty() || now - it->second.since < P.oldMs) return -1;
        return std::min(it->second.q.front().second, get(book[s], px));
    }
    // the share of a zone's size that is old; -1 when the age of less than half of its size is known
    double zoneOld(int side, Tick lo, Tick hi, Ms now) const {
        double total = 0, known = 0, old = 0;
        for (std::map<Tick, double>::const_iterator i = book[side].lower_bound(lo); i != book[side].end() && i->first <= hi; ++i) {
            total += i->second; const double o = oldAt(side, i->first, now);
            if (o >= 0) { known += i->second; old += o; }
        }
        if (total <= 0 || known < 0.5 * total) return -1;
        return old / total;
    }
    // (0.6.0) the chart's closed bars: buyers' / sellers' volume and the ticks each bar traded - the Attack readout's normal
    struct BarNorm { double buy = 0, sell = 0; int ticks = 1; Tick hi = 0, lo = 0; bool range = false; };
    std::map<int, BarNorm> barNorm;
    void setBar(int bar, double buy, double sell, int ticks, Tick hi = 0, Tick lo = 0) {
        if (bar < 0 || !(buy >= 0) || !(sell >= 0) || !finite(buy) || !finite(sell)) return;
        BarNorm& b = barNorm[bar]; b.buy = buy; b.sell = sell; b.ticks = std::max(1, ticks);
        b.range = hi > 0 && lo > 0 && hi >= lo; b.hi = hi; b.lo = lo;
        while (barNorm.size() > 64) barNorm.erase(barNorm.begin());
    }
    // the attackers' (sellers at a support, buyers at a resistance) average volume per bar per tick traded over the 20 bars before 'bar';
    // -1 until 15 of those bars are known
    double attackNormal(int side, int bar) const {
        double s = 0; int n = 0;
        for (std::map<int, BarNorm>::const_iterator i = barNorm.lower_bound(bar - 20); i != barNorm.end() && i->first < bar; ++i) {
            s += (side == BID ? i->second.sell : i->second.buy) / (double)i->second.ticks; ++n;
        }
        return n >= 15 && s > 0 ? s / n : -1;
    }
    // (0.8.0) the DEFENDERS' (buyers at a support, sellers at a resistance) average volume per bar per tick over the 20 bars before 'bar'
    double defNormal(int side, int bar) const { return attackNormal(side == BID ? ASK : BID, bar); }
    Stats stats(const Attempt& a, Ms now) const {
        Stats r; r.side = a.side; r.active = !a.done;
        const double w = (double)(a.hi - a.lo + 1), nrm = attackNormal(a.side, a.barA);
        if (nrm > 0) { double mx = 0; for (std::map<int, Attempt::BarFlow>::const_iterator i = a.flowBars.begin(); i != a.flowBars.end(); ++i) mx = std::max(mx, i->second.attack); r.attack = mx / w / nrm; }
        const double tot = a.shown + a.added + a.refill;
        if (tot > 0) r.eaten = std::min(1.0, a.attackVol / tot);
        const Ms t1 = a.done ? (a.tEnd > a.tA ? a.tEnd : a.tA) : now;
        r.exh = t1 - a.tA >= 60000 ? exhaustRatio(a.attacks, a.tA, t1) : -1;   // (0.7.0) halves of under 30 s say nothing: no Exh read before 1 min
        if (a.shown > 0) r.pulled = a.pulled / a.shown;
        r.rebuilt = resilience(a);
        r.counter = a.counterNow;
        return r;
    }
    // the statistics of the zone price is in now (or the last one)
    bool latest(Ms now, Stats& out) const {
        const Attempt* best = nullptr;
        for (size_t i = 0; i < attempts.size(); ++i) if (!attempts[i].done && (!best || attempts[i].tA > best->tA)) best = &attempts[i];
        if (!best && !finished.empty()) best = &finished.back();
        if (!best) return false;
        out = stats(*best, now); return true;
    }
    long long books = 0, trades = 0, badBooks = 0;
    std::string why;                             // last reason a book or trade was refused

    void setAtr(double atrPrice) { hw = halfWidth(P, atrPrice); }

    // ---- trades: aggressive volume at a price since the last read (buy lifts the offer, sell hits the bid)
    void onTrade(Ms t, double price, double buy, double sell) {
        if (!finite(price) || price <= 0 || !(buy >= 0) || !(sell >= 0) || buy + sell <= 0) return;
        const Tick px = toTick(price, P.tick);
        if (buy > 0) pendingExec[ASK][px] += buy;
        if (sell > 0) pendingExec[BID][px] += sell;
        TradeEvt e; e.t = t; e.px = px; e.buy = buy; e.sell = sell; tape.push_back(e); trades++;
        for (size_t i = 0; i < attempts.size(); ++i) {
            Attempt& a = attempts[i]; if (a.done) continue;
            const double attack = a.side == BID ? sell : buy;            // support is attacked by sellers, resistance by buyers
            if (attack > 0 && px >= a.lo && px <= a.hi) { a.attackVol += attack; a.attacks.push_back(std::make_pair(t, attack)); a.flowBars[formBar].attack += attack; }
            if (attack > 0 && (a.side == BID ? px < a.lo : px > a.hi)) a.breachTrade = true;
            const double counter = a.side == BID ? buy : sell;           // the defending side turning aggressive (pushback)
            if (counter > 0 && px >= a.lo && px <= a.hi) { a.counterVol += counter; a.flowBars[formBar].defend += counter; }
        }
        startAttempts(t, px, buy, sell);
        lastPx = px; havePx = true;
        trim(t);
    }
    void onLastPrice(double price) { if (finite(price) && price > 0) { lastPx = toTick(price, P.tick); havePx = true; } }

    // ---- a depth snapshot: (price, size) per level; zero / bad levels are skipped
    bool onBook(Ms t, const std::vector<std::pair<double, double>>& bids, const std::vector<std::pair<double, double>>& asks) {
        std::map<Tick, double> nb[2];
        for (size_t i = 0; i < bids.size(); ++i) if (finite(bids[i].first) && bids[i].first > 0 && finite(bids[i].second) && bids[i].second > 0) nb[BID][toTick(bids[i].first, P.tick)] += bids[i].second;
        for (size_t i = 0; i < asks.size(); ++i) if (finite(asks[i].first) && asks[i].first > 0 && finite(asks[i].second) && asks[i].second > 0) nb[ASK][toTick(asks[i].first, P.tick)] += asks[i].second;
        if (nb[BID].empty() || nb[ASK].empty()) { why = "one-sided book"; badBooks++; return false; }
        if (nb[BID].rbegin()->first >= nb[ASK].begin()->first) { why = "crossed book"; badBooks++; return false; }
        Tick nw[2][2] = {{nb[BID].begin()->first, nb[BID].rbegin()->first}, {nb[ASK].begin()->first, nb[ASK].rbegin()->first}};
        if (haveBook) {
            for (int s = 0; s < 2; ++s) {
                // only prices inside BOTH windows: bids from max(old lowest, new lowest) up; asks up to min(old highest, new highest)
                const Tick lo = std::max(win[s][0], nw[s][0]), hi = std::min(win[s][1], nw[s][1]);
                std::map<Tick, double> keys;
                for (std::map<Tick, double>::const_iterator i = book[s].begin(); i != book[s].end(); ++i) keys[i->first] = 0;
                for (std::map<Tick, double>::const_iterator i = nb[s].begin(); i != nb[s].end(); ++i) keys[i->first] = 0;
                for (std::map<Tick, double>::const_iterator k = pendingExec[s].begin(); k != pendingExec[s].end(); ++k) keys[k->first] = 0;
                for (std::map<Tick, double>::const_iterator k = keys.begin(); k != keys.end(); ++k) {
                    const Tick px = k->first;
                    const double old = get(book[s], px), now = get(nb[s], px), ex = get(pendingExec[s], px);
                    const bool inWin = px >= lo && px <= hi;
                    FlowEvt e; e.t = t; e.side = s; e.px = px; e.executed = ex; e.pulled = e.added = e.refill = 0;
                    const double rest = old - std::min(ex, old);
                    e.refill = std::max(0.0, ex - old);
                    if (inWin) { e.pulled = std::max(0.0, rest - now); e.added = std::max(0.0, now - rest); }
                    if (e.executed > 0 || e.pulled > 0 || e.added > 0 || e.refill > 0) { flow.push_back(e); applyFlow(e); }
                }
            }
        }
        for (int s = 0; s < 2; ++s) { book[s].swap(nb[s]); pendingExec[s].clear(); win[s][0] = nw[s][0]; win[s][1] = nw[s][1]; }
        // (0.5.0) the ages: a feed gap of a minute+ restarts every clock; prices that left the window restart theirs
        if (haveBook && t - bookT > 60000) { age[0].clear(); age[1].clear(); }
        for (int s = 0; s < 2; ++s) {
            for (std::map<Tick, Age>::iterator i = age[s].begin(); i != age[s].end();) { if (i->first < win[s][0] || i->first > win[s][1]) i = age[s].erase(i); else ++i; }
            if (win[s][1] - win[s][0] <= 400) for (Tick px = win[s][0]; px <= win[s][1]; ++px) ageUpdate(s, px, t, get(book[s], px));
        }
        haveBook = true; bookT = t; books++;
        for (size_t i = 0; i < attempts.size(); ++i) if (!attempts[i].done) {
            Attempt& a = attempts[i]; a.curSize = band(a.side, a.lo, a.hi); a.minSize = std::min(a.minSize, a.curSize);
        }
        trim(t);
        return true;
    }

    double band(int side, Tick lo, Tick hi) const {
        double s = 0; for (std::map<Tick, double>::const_iterator i = book[side].lower_bound(lo); i != book[side].end() && i->first <= hi; ++i) s += i->second;
        return s;
    }

    // ---- the top N zones per side within reach; tracked by centre. Call after onBook (at most a few times a second).
    void updateZones(Ms t, double atrPrice, int slot) {
        if (!haveBook) return;
        setAtr(atrPrice);
        const Tick reach = (Tick)std::llround(std::max(10.0 * hw, P.reachAtr * (atrPrice > 0 ? atrPrice : 15 * P.tick) / P.tick));
        const Tick ref = havePx ? lastPx : (book[BID].rbegin()->first + book[ASK].begin()->first) / 2;
        for (int s = 0; s < 2; ++s) {
            std::vector<std::pair<double, Tick>> cand;
            const std::map<Tick, double>& b = book[s];
            if (b.empty()) continue;
            const Tick from = s == BID ? std::max(b.begin()->first, ref - reach) : b.begin()->first;
            const Tick to = s == BID ? b.rbegin()->first : std::min(b.rbegin()->first, ref + reach);
            std::vector<std::pair<Tick, double>> lv; for (std::map<Tick, double>::const_iterator i = b.lower_bound(from - hw); i != b.end() && i->first <= to + hw; ++i) lv.push_back(*i);
            // sliding band sum at every price that has size
            size_t a = 0, c = 0; double sum = 0;
            for (size_t i = 0; i < lv.size(); ++i) {
                const Tick p = lv[i].first; if (p < from || p > to) continue;
                while (c < lv.size() && lv[c].first <= p + hw) { sum += lv[c].second; ++c; }
                while (a < lv.size() && lv[a].first < p - hw) { sum -= lv[a].second; ++a; }
                cand.push_back(std::make_pair(sum, p));
                norms.sample(sum);
            }
            std::sort(cand.begin(), cand.end(), [](const std::pair<double, Tick>& x, const std::pair<double, Tick>& y) { return x.first != y.first ? x.first > y.first : x.second < y.second; });
            std::vector<std::pair<double, Tick>> pick;
            for (size_t i = 0; i < cand.size() && (int)pick.size() < P.topN; ++i) {
                bool clash = false; for (size_t j = 0; j < pick.size(); ++j) if (std::llabs(pick[j].second - cand[i].second) <= 2 * hw) clash = true;
                if (!clash && cand[i].first > 0) pick.push_back(cand[i]);
            }
            std::vector<Tick> placed;
            for (size_t i = 0; i < pick.size(); ++i) {
                // the zone's centre = the size-weighted middle of its band (a flat run of equal band sums otherwise picks its edge);
                // a zone whose band would overlap a bigger one already placed is dropped (zones never overlap)
                double w = 0, m = 0;
                for (std::map<Tick, double>::const_iterator j = b.lower_bound(pick[i].second - hw); j != b.end() && j->first <= pick[i].second + hw; ++j) { w += j->second; m += j->second * (double)j->first; }
                const Tick c = w > 0 ? (Tick)std::llround(m / w) : pick[i].second;
                bool clash = false; for (size_t j = 0; j < placed.size(); ++j) if (std::llabs(placed[j] - c) <= 2 * hw) clash = true;
                if (clash) continue;
                placed.push_back(c);
                track(t, s, c, band(s, c - hw, c + hw));
            }
        }
        for (size_t i = 0; i < zones.size();) {
            if (!zones[i].inAttempt && t - zones[i].seen > P.graceMs) zones.erase(zones.begin() + i); else ++i;
        }
        normal = norms.at(slot);
    }
    double normal = 0;
    double xNormal(double size) const { return normal > 0 ? size / normal : -1; }

    // ---- a closed bar: decide every attempt (marks are final once written)
    void onBarClose(int bar, long long barTime, Ms barEnd, double close, const std::vector<std::pair<Tick, Tick>>& deltaAbsorbBid,
                    const std::vector<std::pair<Tick, Tick>>& deltaAbsorbAsk) {
        const Tick c = toTick(close, P.tick);
        // (0.8.0) s after a touch already decided as held (the defenders often step in on the bars after the close back out): finished
        // attempts first, so one closed on THIS bar is not read twice
        for (size_t i = 0; i < finished.size(); ++i) {
            Attempt& f = finished[i];
            if (f.end.compare(0, 4, "hold") != 0 || f.barE < 0 || bar <= f.barA || bar - f.barE > P.counterBars) continue;
            if (f.barStats.count(bar)) continue;
            counter(f, bar, barTime, barEnd);
            f.barStats[bar] = stats(f, barEnd);
        }
        for (size_t i = 0; i < attempts.size(); ++i) {
            Attempt& a = attempts[i]; if (a.done) continue;
            if (bar < a.barA) continue;
            a.tEnd = barEnd;
            story(a, bar, barEnd, barTime);
            const bool broke = a.side == BID ? c < a.lo : c > a.hi;
            if (!broke) counter(a, bar, barTime, barEnd);                // never an s on the bar that closed through
            a.barStats[bar] = stats(a, barEnd);
            const bool out = a.side == BID ? c > a.hi : c < a.lo;
            const double eaten = a.shown > 0 ? a.attackVol / a.shown : 0;
            // (0.1.3, Rassul 2026-10-10 "pulling was a major factor to why the absorption failed") the defenders leaving without being
            // replaced: PULLING = half+ of the shown size pulled and less than half of what was taken replaced; DEPLETING = eaten 1x+, not replaced
            const double taken = a.attackVol + a.pulled, replaced = a.added + a.refill;
            const double refillX = taken > 0 ? replaced / taken : -1;
            const bool pulling = a.shown > 0 && a.pulled / a.shown >= P.pullShare && refillX >= 0 && refillX < 0.5;
            const bool depleting = !pulling && eaten >= P.eatenBreak && refillX >= 0 && refillX < 0.5;
            // (0.4.0, Rassul 2026-10-10 23:35 "get rid of X and L and LX") the outcome ends the attempt and is LOGGED for the nightly study;
            // nothing is drawn on the price chart. The pane signals (Exh / Pull) come from story().
            // (0.7.0) t = TRAPPED: the attackers traded beyond the zone's far edge, then a bar closes back beyond the zone on the defended
            // side (a green close above a support after the push below) - they broke it, got nothing, now they are stuck
            if (a.barT < 0 && a.breachTrade && (a.side == BID ? c > a.hi : c < a.lo)) { a.barT = bar; push("t", a, barTime, barEnd, -1); }
            if (broke) {
                pushReason(a, false, barTime, barEnd);
                a.end = std::string("break") + (pulling ? " pulled" : "") + (eaten >= P.eatenBreak ? " eaten" : "") + (depleting ? " depleting" : "");
                close_(a);
                continue;
            }
            if (out && barEnd - a.tA >= P.decisionMs) {
                bool A = false;
                const std::vector<std::pair<Tick, Tick>>& dp = a.side == BID ? deltaAbsorbBid : deltaAbsorbAsk;
                for (size_t k = 0; k < dp.size(); ++k) if (dp[k].first <= a.hi && dp[k].second >= a.lo) A = true;
                pushReason(a, true, barTime, barEnd);
                a.end = std::string("hold") + (pulling ? " pulled" : "") + (depleting ? " depleting" : "") + (a.barE >= 0 ? " Exh" : "") + (A ? " A" : "")
                      + (a.barO >= 0 ? " offense" : "") + (a.barR >= 0 ? " replenished" : "");
                close_(a);
                continue;
            }
            if (bar - a.barA >= P.maxBars) { a.end = "expired"; close_(a); }
        }
        attempts.erase(std::remove_if(attempts.begin(), attempts.end(), [](const Attempt& a) { return a.done; }), attempts.end());
        curBar = bar;
    }
    int curBar = 0;
    int formBar = 0;                             // the bar in progress (set by the host before trades / books)

    Readout readout(Ms now) const {
        Readout r;
        const Attempt* best = nullptr;
        for (size_t i = 0; i < attempts.size(); ++i) if (!attempts[i].done && (!best || attempts[i].tA > best->tA)) best = &attempts[i];
        if (!best && !finished.empty()) best = &finished.back();
        if (!best) return r;
        r.active = !best->done; r.side = best->side; r.lo = best->lo; r.hi = best->hi;
        r.eaten = best->shown > 0 ? best->attackVol / best->shown : 0;
        r.stayed = best->shown > 0 ? std::max(0.0, std::min(1.0, 1.0 - best->pulled / best->shown)) : 1;
        const Ms t1 = best->done ? (best->attacks.empty() ? best->tA : best->attacks.back().first) : now;
        r.exhaust = exhaustRatio(best->attacks, best->tA, t1);
        r.exhausted = r.exhaust >= 0 && r.exhaust < P.exhaust;
        r.pulled = best->shown > 0 ? best->pulled / best->shown : 0;
        const double taken = best->attackVol + best->pulled, replaced = best->added + best->refill;
        r.refill = taken > 0 ? replaced / taken : -1;
        r.pulling = r.pulled >= P.pullShare && r.refill >= 0 && r.refill < 0.5;
        r.depleting = !r.pulling && r.eaten >= P.eatenBreak && r.refill >= 0 && r.refill < 0.5;
        return r;
    }

    // the record line of a mark (pipe-separated, append-only; LiquidityProfile\<MKT>-<spb>-signals-<session>.csv)
    static std::string header() { return "bar_time|code|reasons|side|zone_lo|zone_hi|shown|eaten|stayed|exhaust|resilience|t_arrival|t_decided|version"; }
    static std::string line(const Mark& m, double tick) {
        std::ostringstream o; o.imbue(std::locale::classic()); o.precision(10);
        o << m.barTime << '|' << m.code << '|' << m.reasons << '|' << (m.side == BID ? "support" : "resistance") << '|'
          << (double)m.lo * tick << '|' << (double)m.hi * tick << '|' << m.shown << '|' << round2(m.eaten) << '|' << round2(m.stayed) << '|'
          << round2(m.exhaust) << '|' << round2(m.resil) << '|' << m.tA << '|' << m.tDecided << '|' << VERSION << '|' << round2(m.value);
        return o.str();
    }
    static bool num(const std::string& t, double& v) {                 // strict full-token parse (checklist #4)
        if (t.empty() || t.size() > 40) return false;
        char* e = nullptr; v = std::strtod(t.c_str(), &e); return e == t.c_str() + t.size() && finite(v);
    }
    static bool parse(const std::string& s, Mark& m, double tick) {
        std::vector<std::string> f; size_t a = 0;
        for (size_t i = 0; i <= s.size(); ++i) if (i == s.size() || s[i] == '|') { f.push_back(s.substr(a, i - a)); a = i + 1; }
        if (f.size() < 14 || !paneCode(f[1]) || !(tick > 0)) return false;
        double bt, lo, hi, ta, td;
        if (!num(f[0], bt) || bt <= 0 || !num(f[4], lo) || !num(f[5], hi) || !num(f[6], m.shown) || !num(f[7], m.eaten) || !num(f[8], m.stayed)
            || !num(f[9], m.exhaust) || !num(f[10], m.resil) || !num(f[11], ta) || !num(f[12], td)) return false;
        if (f[3] != "support" && f[3] != "resistance") return false;
        m.barTime = (long long)bt; m.code = f[1]; m.reasons = f[2]; m.side = f[3] == "support" ? BID : ASK;
        m.lo = toTick(lo, tick); m.hi = toTick(hi, tick); m.tA = (Ms)ta; m.tDecided = (Ms)td;
        m.value = -1; double v = 0; if (f.size() >= 15 && num(f[14], v)) m.value = v;
        return m.hi >= m.lo;
    }
    static bool paneCode(const std::string& c) { return c == "Exh" || c == "Pull" || c == "ew" || c == "pw" || c == "t" || c == "s" || c == "Held" || c == "Broke"; }   // (0.5.0) older records with LoF / HiB / RpB / RpO are skipped
    static double round2(double v) { return std::floor(v * 100 + 0.5) / 100; }

    // the attempt log line (every attempt, drawn or not) for the nightly scoring
    static std::string attemptHeader() { return "t_arrival|side|zone_lo|zone_hi|shown|eaten|stayed|added_x|refill_x|exhaust|resilience|breach_trade|end|version|pushback|defender_speed|pull_speed|response_cross|t_end|e_bar|o_bar|p_bar|story|r_bar|x_bar|def_net_x|opp_net_x|old_x"; }
    std::string attemptLine(const Attempt& a, Ms tEnd) const {
        std::ostringstream o; o.imbue(std::locale::classic()); o.precision(10);
        const double sh = a.shown > 0 ? a.shown : 1;
        o << a.tA << '|' << (a.side == BID ? "support" : "resistance") << '|' << (double)a.lo * P.tick << '|' << (double)a.hi * P.tick << '|'
          << a.shown << '|' << round2(a.attackVol / sh) << '|' << round2(std::max(0.0, 1 - a.pulled / sh)) << '|' << round2(a.added / sh) << '|'
          << round2(a.refill / sh) << '|' << round2(exhaustRatio(a.attacks, a.tA, tEnd)) << '|' << round2(resilience(a)) << '|'
          << (a.breachTrade ? 1 : 0) << '|' << a.end << '|' << VERSION;
        // (the incoming package's buyer / seller response, LOGGED for the nightly study, never drawn): the defending side's
        // aggressive contracts / s vs the defending side's pulled contracts / s over the defence; cross = defenders outpaced pulls
        const double secs = std::max(1.0, (double)(tEnd - a.tA) / 1000.0), tot = a.attackVol + a.counterVol;
        o << '|' << round2(tot > 0 ? a.counterVol / tot : -1) << '|' << round2(a.counterVol / secs) << '|' << round2(a.pulled / secs) << '|'
          << (a.counterVol > a.pulled ? 1 : 0) << '|' << tEnd;
        // (0.2.0) the story: bars after arrival when E / O / P first showed (-1 = never) and their order, e.g. "E>O" = exhaustion, then offense
        std::vector<std::pair<int, char>> seq;
        if (a.barE >= 0) seq.push_back(std::make_pair(a.barE, 'E'));
        if (a.barO >= 0) seq.push_back(std::make_pair(a.barO, 'O'));
        if (a.barP >= 0) seq.push_back(std::make_pair(a.barP, 'P'));
        if (a.barR >= 0) seq.push_back(std::make_pair(a.barR, 'R'));
        std::sort(seq.begin(), seq.end());
        std::string st; for (size_t i = 0; i < seq.size(); ++i) { if (i) st += '>'; st += seq[i].second; }
        o << '|' << (a.barE >= 0 ? a.barE - a.barA : -1) << '|' << (a.barO >= 0 ? a.barO - a.barA : -1) << '|' << (a.barP >= 0 ? a.barP - a.barA : -1) << '|' << (st.empty() ? "-" : st)
          << '|' << (a.barR >= 0 ? a.barR - a.barA : -1) << '|' << (a.barX >= 0 ? a.barX - a.barA : -1)
          << '|' << round2(a.defNet / sh) << '|' << round2(a.oppNet / sh) << '|' << round2(a.oldShare);
        return o.str();
    }
    std::vector<std::string> attemptLog;          // lines waiting to be appended by the host

    // (0.2.0) the story marks, each decided once at a bar close: E = the attackers' 2nd-half pace < 0.5 x the 1st half; P = half+ of the
    // shown size pulled and not replaced; O = the defenders out-traded the attackers in a bar after the attackers had led one (defense
    // first, then offense - Rassul 2026-10-10 "buyers playing defense first and then offense")
    // (0.4.0, agreed with Rassul 2026-10-10 23:31-23:35, one signal per idea, each ONLY on its pane, decided once at a bar close):
    //   AGGRESSION  Exh  the attackers' pace dried up (2nd-half / 1st-half volume < this market's 'exhaust')
    //   DEFENSE     Pull the defenders pulled (this market's 'pull_share' of the shown size, not replaced) WHILE the attackers lead
    // (0.5.0, Rassul 2026-10-11 07:4x "remove whatever doesn't work and keep what works", "remove stop") LoF / HiB / RpB / RpO removed:
    // the data did not confirm them (LoF pace 2x+ in 0-4% of touches and heavy defender volume held LESS; RpB only in 2 of 7 markets,
    // backwards in ES / CL). The O / R crosses are still LOGGED in the attempt line for the study, never drawn.
    // Each mark is a record line, re-read after a restart: never repaints. The host draws a code only where the market's nightly study
    // confirmed it (SIGNAL lines); nothing unconfirmed is drawn.
    void story(Attempt& a, int bar, Ms barEnd, long long barTime) {
        // (0.7.0) e = Exh: the attackers' 2nd-half / 1st-half volume since arrival under THIS MARKET's line (P.exhaust, from the data);
        //        e? = the watch: under twice the line (at most 0.9x) - the attack is slowing toward it. e? first, e when the line is crossed.
        // e is decided from the study's decision point on (arrival + 3 min: the window lra.mbo_study measures exhaust_ratio over), e? from
        // 1 min - a few quiet seconds right after arrival are not exhaustion
        if (a.barE < 0 && firstHalf(a, barEnd) >= P.minAttack && barEnd - a.tA >= 60000) {
            const double ex = exhaustRatio(a.attacks, a.tA, barEnd);
            if (ex >= 0 && ex < P.exhaust && barEnd - a.tA >= P.decisionMs) { a.barE = bar; a.exE = ex; push("Exh", a, barTime, barEnd, ex); }
            else if (a.barEW < 0 && ex >= 0 && ex < std::min(0.9, 2 * P.exhaust)) { a.barEW = bar; push("ew", a, barTime, barEnd, ex); }
        }
        // (0.7.0) p = Pull: the share of the shown size pulled reaches THIS MARKET's line while the attackers lead (the indicator's own
        //        snapshot measure, lra.mbo_study pull_snap); p? = the watch: half-way to the line (first look: hold odds already drop there)
        if (a.barP < 0 && a.shown > 0 && a.attackVol > a.counterVol) {
            const double pr = a.pulled / a.shown;
            if (pr >= P.pullShare) { a.barP = bar; push("Pull", a, barTime, barEnd, pr); }
            else if (a.barPW < 0 && pr >= 0.5 * P.pullShare) { a.barPW = bar; push("pw", a, barTime, barEnd, pr); }
        }
        // the O / R crosses (logged only): the defenders' total since arrival crosses above the attackers' after they led; the defenders'
        // net resting size crosses above the other side's after it led
        if (a.attackVol > a.counterVol) a.defendedFirst = true;
        else if (a.barO < 0 && a.defendedFirst && a.counterVol > a.attackVol) a.barO = bar;
        if (a.oppNet >= a.defNet && (a.oppNet != 0 || a.defNet != 0)) a.pulledFirst = true;
        else if (a.barR < 0 && a.pulledFirst && a.defNet > a.oppNet) a.barR = bar;
    }
    // (0.8.0) s = COUNTER: the defenders turned aggressive after the attackers exhausted - a closed bar, from the e bar on (up to
    // counterBars), where the defenders out-traded the attackers (the whole bar, IRT's own bar statistics), the bar traded within one zone
    // width of the zone, and their volume per tick traded was counterX x their normal (the 20 bars before arrival). The Counter readout
    // is that pace on every bar of the touch. lra.mbo_study ctr_seq measures exactly this; lra.liq_params replays it for the line.
    void counter(Attempt& a, int bar, long long barTime, Ms barEnd) {
        std::map<int, BarNorm>::const_iterator it = barNorm.find(bar);
        const double nrm = defNormal(a.side, a.barA);
        if (it == barNorm.end() || nrm <= 0) return;
        const BarNorm& b = it->second;
        const bool sup = a.side == BID;
        const double dv = sup ? b.buy : b.sell, av = sup ? b.sell : b.buy, x = dv / (double)b.ticks / nrm;
        a.counterNow = x;
        if (a.barE < 0 || a.barS >= 0 || bar < a.barE || bar - a.barE > P.counterBars) return;
        const Tick w = a.hi - a.lo + 1;
        const bool near = b.range && (sup ? b.lo <= a.hi + w : b.hi >= a.lo - w);
        if (!(dv > av) || !near || x + 1e-9 < P.counterX) return;
        a.barS = bar; a.tS = barEnd; a.sX = x;
        push("s", a, barTime, barEnd, x);
        const std::string dfn = sup ? "Buyers" : "Sellers", att = sup ? "sellers" : "buyers";
        marks.back().reasons = dfn + " stepped in: " + fx(x) + " normal, out-traded " + att;
    }
    static bool topPane(const std::string& c) { return c == "Exh" || c == "ew" || c == "Held" || c == "Broke"; }
    // green = buyers win / bullish, red = sellers win / bearish (Rassul: buyers always green, sellers always red)
    static bool bullish(const Mark& m) {
        if (m.code == "Exh" || m.code == "ew" || m.code == "t" || m.code == "s" || m.code == "Held") return m.side == BID;   // good for the defenders
        if (m.code == "Broke") return m.side == ASK;
        return m.side == ASK;                                // Pull: buyers pulling at a support is bearish, sellers pulling at a resistance bullish
    }
    Lines L;                                       // (0.7.0) set by the host from the norms file
    static std::string fx(double v) { char b[16]; std::snprintf(b, sizeof(b), v >= 10 ? "%.0fx" : "%.1fx", v); return b; }
    static std::string pc(double v) { char b[16]; std::snprintf(b, sizeof(b), "%.0f%%", v * 100.0); return b; }
    // (0.7.0) the reason in plain words when an attempt is decided (held = absorbed, broke), built ONLY from numbers whose line is confirmed
    // for this market (Rassul's wording: "sellers hit the bid 2.4x normal, then exhausted (0.3x); buyers held and refilled their bids (1.2x)")
    std::string reason(const Attempt& a, bool held, Ms tEnd) const {
        const Stats st = stats(a, tEnd); const bool sup = a.side == BID;
        const std::string att = sup ? "sellers" : "buyers", dfn = sup ? "buyers" : "sellers", hit = sup ? "hit the bid" : "lifted the offer", ord = sup ? "bids" : "offers";
        std::string first = att + " " + hit;
        if (L.attack > 0 && st.attack >= 0) first += " " + fx(st.attack) + " normal";
        const bool exh = L.exh > 0 && st.exh >= 0 && st.exh < L.exh;
        if (L.exh > 0 && st.exh >= 0) first += exh ? ", then exhausted (" + fx(st.exh) + ")" : (held ? ", kept going (" + fx(st.exh) + ")" : " and never slowed");
        std::string second;
        const bool pulled = L.pulled > 0 && st.pulled >= L.pulled;
        if (pulled) second = dfn + " pulled " + pc(std::min(st.pulled, 9.99)) + " of their " + ord + (held ? "" : " instead of defending");
        else if (L.rebuilt > 0 && st.rebuilt >= L.rebuilt) second = dfn + " held and refilled their " + ord + " (" + fx(st.rebuilt) + ")";
        else if (!held && L.eaten > 0 && st.eaten >= L.eaten) second = att + " ate " + pc(st.eaten) + " of everything there";
        else second = held ? dfn + " held" : dfn + " did not refill";
        return std::string(held ? "Absorbed: " : "Broke: ") + first + "; " + second;
    }
    void pushReason(const Attempt& a, bool held, long long barTime, Ms barEnd) {
        Mark m; m.code = held ? "Held" : "Broke"; m.reasons = reason(a, held, barEnd); m.value = -1; m.side = a.side; m.barTime = barTime;
        m.lo = a.lo; m.hi = a.hi; m.shown = a.shown; m.tA = a.tA; m.tDecided = barEnd; marks.push_back(m);
    }
    // (0.7.0) the warning for the zone price is in now: the strongest that applies (risk first, the good ones only when nothing worse
    // applies); every condition uses only lines confirmed for this market; nothing when no attempt is live or nothing applies
    bool warning(Ms now, Warn& w, double tick) const {
        const Attempt* a = nullptr;
        for (size_t i = 0; i < attempts.size(); ++i) if (!attempts[i].done && (!a || attempts[i].tA > a->tA)) a = &attempts[i];
        // (0.8.0) STEPPING IN: an s in the last 2 bars (a live touch, or one just decided as held) - the defenders turned aggressive
        const Attempt* sa = nullptr;
        if (L.counter > 0) {
            if (a && a->barS >= 0) sa = a;
            else for (size_t i = finished.size(); i-- > 0;) if (finished[i].barS >= 0 && now - finished[i].tS <= 2 * P.barMs && now >= finished[i].tS) { sa = &finished[i]; break; }
        }
        auto stepping = [&](const Attempt& x) {
            const bool s_ = x.side == BID;
            w = Warn(); w.name = std::string(s_ ? "BUYERS" : "SELLERS") + " STEPPING IN"; w.favour = s_ ? +1 : -1;
            w.detail = std::string(s_ ? "sellers" : "buyers") + " exhausted" + (x.exE >= 0 ? " (" + fx(x.exE) + ")" : std::string()) + "; " +
                       (s_ ? "buyers now lifting offers " : "sellers now hitting bids ") + fx(x.sX) + " normal";
            return true;
        };
        if (!a) return sa ? stepping(*sa) : false;
        const Stats st = stats(*a, now); const bool sup = a->side == BID; const int good = sup ? +1 : -1, bad = -good;
        const std::string att = sup ? "sellers" : "buyers", dfn = sup ? "buyers" : "sellers", hitting = sup ? "hitting the bid" : "lifting the offer",
            ord = sup ? "bids" : "offers", Att = sup ? "SELLERS" : "BUYERS", Dfn = sup ? "BUYERS" : "SELLERS", Ord = sup ? "BIDS" : "OFFERS";
        const bool okA = L.attack > 0 && st.attack >= 0, okX = L.exh > 0 && st.exh >= 0, okP = L.pulled > 0 && st.pulled >= 0, okE = L.eaten > 0 && st.eaten >= 0;
        const bool heavy = okA && st.attack >= L.attack, light = okA && st.attack < L.attack;
        const bool exh = okX && st.exh < L.exh, slowing = okX && !exh && st.exh < std::min(0.9, 2 * L.exh);
        const bool lead = a->attackVol > a->counterVol;
        const bool pull = okP && lead && st.pulled >= L.pulled, pullW = okP && lead && !pull && st.pulled >= 0.5 * L.pulled;
        const bool thin = okE && st.eaten >= L.eaten;
        const bool trapped = L.trapped && a->barT >= 0;
        auto A = [&]() { return okA ? " " + fx(st.attack) + " normal" : std::string(); };
        char px[32]; std::snprintf(px, sizeof(px), "%.*f", tick >= 1 ? 0 : (int)std::ceil(-std::log10(tick) - 1e-9), (double)(sup ? a->hi : a->lo) * tick);
        w = Warn();
        if (heavy && pull && !exh) { w.name = sup ? "BREAKDOWN RISK" : "BREAKOUT RISK"; w.favour = bad;
            w.detail = att + " " + hitting + A() + ", not slowing; " + dfn + " pulling their " + ord + " (" + pc(st.pulled) + ")"; }
        else if (exh && pull) { w.name = "ABSORPTION WEAKENING"; w.favour = bad;
            w.detail = att + " exhausted (" + fx(st.exh) + "), but " + dfn + " are pulling their " + ord + " (" + pc(st.pulled) + ")"; }
        else if (pull) { w.name = Dfn + " PULLING"; w.favour = bad;
            w.detail = dfn + " pulling their " + ord + " (" + pc(st.pulled) + ") while " + att + " keep " + hitting; }
        else if (thin && !exh) { w.name = Ord + " THINNING"; w.favour = bad;
            w.detail = att + " have eaten " + pc(st.eaten) + " of everything at " + px + " (line " + pc(L.eaten) + ")"; }
        else if (pullW) { w.name = "PULLING WATCH"; w.favour = bad;
            w.detail = dfn + " have pulled " + pc(st.pulled) + " of their " + ord + " while " + att + " keep " + hitting + " (Pull line " + pc(L.pulled) + ")"; }
        else if (sa == a && a->barS >= 0) return stepping(*a);
        else if (heavy && slowing) { w.name = "EXHAUSTION WATCH"; w.favour = 0;
            w.detail = att + " " + hitting + A() + " and starting to slow (" + fx(st.exh) + ")"; }
        else if (heavy && !exh) { w.name = Dfn + " HOLDING"; w.favour = 0;
            w.detail = att + " " + hitting + A() + ", not slowing; " + dfn + " holding - wait"; }
        else if (trapped) { w.name = "TRAPPED " + Att; w.favour = good;
            w.detail = att + " pushed past " + std::string(px) + " and failed; price back " + (sup ? "above" : "below") + " the zone"; }
        else if (exh && light) { w.name = Att + " GIVING UP"; w.favour = good;
            w.detail = att + " tested the " + (sup ? "bid" : "offer") + " lightly (" + fx(st.attack) + " normal) and exhausted (" + fx(st.exh) + ")"; }
        else if (exh) { w.name = "ABSORPTION OCCURRING"; w.favour = good;
            w.detail = att + " exhausted (" + fx(st.exh) + "); " + dfn + " holding" +
                       (L.rebuilt > 0 && st.rebuilt >= L.rebuilt ? " and refilling their " + ord + " (" + fx(st.rebuilt) + ")" : std::string()); }
        else return false;
        return true;
    }
    static double resilience(const Attempt& a) {
        if (a.startSize - a.minSize <= 0) return -1;
        return std::max(0.0, (a.curSize - a.minSize) / (a.startSize - a.minSize));
    }

private:
    static double get(const std::map<Tick, double>& m, Tick k) { std::map<Tick, double>::const_iterator i = m.find(k); return i == m.end() ? 0 : i->second; }
    double firstHalf(const Attempt& a, Ms t1) const {
        const Ms mid = a.tA + (t1 - a.tA) / 2; double v = 0;
        for (size_t i = 0; i < a.attacks.size(); ++i) if (a.attacks[i].first < mid) v += a.attacks[i].second;
        return v;
    }
    void applyFlow(const FlowEvt& e) {
        for (size_t i = 0; i < attempts.size(); ++i) {
            Attempt& a = attempts[i]; if (a.done || e.t < a.tA) continue;
            const double net = e.added + e.refill - e.pulled;
            if (a.side == e.side && e.px >= a.lo && e.px <= a.hi) {
                a.pulled += e.pulled; a.added += e.added; a.refill += e.refill; a.defNet += net;
            } else if (a.side != e.side && e.px >= a.oppLo && e.px <= a.oppHi) {
                a.oppNet += net;
            } else continue;
            Attempt::BarFlow& f = a.flowBars[formBar]; f.rep = a.added + a.refill; f.pul = a.pulled; f.defNet = a.defNet; f.oppNet = a.oppNet; f.book = true;
        }
    }
    void track(Ms t, int side, Tick center, double size) {
        Zone* z = nullptr;
        for (size_t i = 0; i < zones.size(); ++i) if (zones[i].side == side && std::llabs(zones[i].center - center) <= hw) { z = &zones[i]; break; }
        if (!z) { Zone n; n.id = nextZone++; n.side = side; n.born = t; zones.push_back(n); z = &zones.back(); }
        if (!z->inAttempt) { z->center = center; z->hw = hw; z->lo = center - hw; z->hi = center + hw; }   // frozen while touched
        z->size = size; z->peak = std::max(z->peak, size); z->seen = t;
    }
    void startAttempts(Ms t, Tick px, double buy, double sell) {
        if (!havePx || !haveBook) return;
        for (int side = 0; side < 2; ++side) {
            // one attempt per side per arrival: the biggest durable zone the trade is in (a stale neighbour never doubles it)
            Zone* best = nullptr;
            for (size_t i = 0; i < zones.size(); ++i) {
                Zone& z = zones[i];
                if (z.side != side || !z.durable(t, P.persistMs) || t - z.seen > P.graceMs) continue;
                if (px < z.lo || px > z.hi) continue;
                if (z.inAttempt) { best = nullptr; break; }               // already being defended: nothing new
                const bool fromOutside = z.side == BID ? lastPx > z.hi : lastPx < z.lo;
                if (!fromOutside) continue;
                if (!best || z.size > best->size) best = &z;
            }
            if (!best) continue;
            Zone& z = *best;
            const double shown = band(z.side, z.lo, z.hi);
            if (shown < P.minShown) continue;
            Attempt a; a.zoneId = z.id; a.side = z.side; a.lo = z.lo; a.hi = z.hi; a.tA = t; a.barA = formBar > 0 ? formBar : curBar;
            a.shown = shown; a.startSize = a.minSize = a.curSize = shown;
            a.oldShare = zoneOld(z.side, z.lo, z.hi, t);
            const Tick w = z.hi - z.lo + 1;                               // (0.3.0) the other side's band: same width, just beyond the zone
            if (a.side == BID) { a.oppLo = z.hi + 1; a.oppHi = z.hi + w; } else { a.oppLo = z.lo - w; a.oppHi = z.lo - 1; }
            const double first = a.side == BID ? sell : buy;          // the trade that arrived counts as the first attack
            if (first > 0) { a.attackVol = first; a.attacks.push_back(std::make_pair(t, first)); }
            a.flowBars[a.barA].attack += first > 0 ? first : 0;
            attempts.push_back(a); z.inAttempt = true;
        }
    }
    void push(const char* code, const Attempt& a, long long barTime, Ms barEnd, double value) {
        Mark m; m.code = code; m.reasons = ""; m.value = value; m.side = a.side; m.barTime = barTime; m.lo = a.lo; m.hi = a.hi; m.shown = a.shown;
        m.eaten = a.shown > 0 ? a.attackVol / a.shown : 0; m.stayed = a.shown > 0 ? std::max(0.0, std::min(1.0, 1 - a.pulled / a.shown)) : 1;
        m.exhaust = exhaustRatio(a.attacks, a.tA, barEnd); m.resil = resilience(a); m.tA = a.tA; m.tDecided = barEnd;
        marks.push_back(m);
    }
    void close_(Attempt& a) {
        a.done = true;
        attemptLog.push_back(attemptLine(a, a.tEnd > a.tA ? a.tEnd : (a.attacks.empty() ? a.tA : a.attacks.back().first)));
        for (size_t i = 0; i < zones.size(); ++i) if (zones[i].id == a.zoneId) zones[i].inAttempt = false;
        finished.push_back(a); if (finished.size() > 50) finished.erase(finished.begin());
    }
    void trim(Ms t) {
        while (!flow.empty() && t - flow.front().t > 3600000) flow.pop_front();
        while (!tape.empty() && t - tape.front().t > 3600000) tape.pop_front();
    }
};

}  // namespace lqp
#endif
