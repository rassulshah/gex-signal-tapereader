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
//   6. MARKS   decided on CLOSED bars only and never changed afterwards (no repaint):
//              LX  a bar closes through the zone's far edge after it was eaten 1x+ (the break read)
//              L?  a bar closes back out of the zone on the defended side at least decisionMs after arrival, never broken, with
//                  a supporting reason: E (exhaustion) and / or A (a Delta Profile absorption in the zone since arrival)
//              Everything else (a hold with no reason, a break without eating, an expiry) is logged, never drawn.
//   Every number is LEARNING (shown with '?') until the nightly study proves it on new days (the improvement loop).
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <locale>
#include <deque>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace lqp {

static const char* const VERSION = "0.1.2";
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

struct Attempt {
    long long zoneId = 0; int side = BID; Tick lo = 0, hi = 0; Ms tA = 0; int barA = 0;
    double shown = 0, attackVol = 0, counterVol = 0, pulled = 0, added = 0, refill = 0, minSize = 0, startSize = 0, curSize = 0;
    std::vector<std::pair<Ms, double>> attacks;  // (time, attacker contracts) inside the zone since arrival
    bool breachTrade = false;                    // an attacker trade beyond the far edge
    bool done = false; std::string end; Ms tEnd = 0;
};

struct Mark {
    std::string code;      // "LX" or "L?"
    std::string reasons;   // "E", "A", "E A" (L? only)
    int side = BID; long long barTime = 0;       // the closed bar's time (host clock, matched on redraw)
    Tick lo = 0, hi = 0; double eaten = 0, stayed = 0, exhaust = -1, resil = -1, shown = 0; Ms tA = 0, tDecided = 0;
};

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
            if (attack > 0 && px >= a.lo && px <= a.hi) { a.attackVol += attack; a.attacks.push_back(std::make_pair(t, attack)); }
            if (attack > 0 && (a.side == BID ? px < a.lo : px > a.hi)) a.breachTrade = true;
            const double counter = a.side == BID ? buy : sell;           // the defending side turning aggressive (pushback)
            if (counter > 0 && px >= a.lo && px <= a.hi) a.counterVol += counter;
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
        bool marked = false;
        for (size_t i = 0; i < attempts.size(); ++i) {
            Attempt& a = attempts[i]; if (a.done) continue;
            if (bar < a.barA) continue;
            a.tEnd = barEnd;
            const bool broke = a.side == BID ? c < a.lo : c > a.hi;
            const bool out = a.side == BID ? c > a.hi : c < a.lo;
            const double eaten = a.shown > 0 ? a.attackVol / a.shown : 0;
            if (broke) {
                if (eaten >= P.eatenBreak && !marked) { push("LX", "", a, barTime, barEnd); marked = true; a.end = "LX"; }
                else a.end = eaten >= P.eatenBreak ? "LX (bar already marked)" : "break, not eaten (pulled / thin)";
                close_(a);
                continue;
            }
            if (out && barEnd - a.tA >= P.decisionMs) {
                const double ex = exhaustRatio(a.attacks, a.tA, barEnd);
                const bool E = ex >= 0 && ex < P.exhaust && firstHalf(a, barEnd) >= P.minAttack;
                bool A = false;
                const std::vector<std::pair<Tick, Tick>>& dp = a.side == BID ? deltaAbsorbBid : deltaAbsorbAsk;
                for (size_t k = 0; k < dp.size(); ++k) if (dp[k].first <= a.hi && dp[k].second >= a.lo) A = true;
                if ((E || A) && !marked) {
                    std::string r = E && A ? "E A" : E ? "E" : "A";
                    push("L?", r, a, barTime, barEnd); marked = true; a.end = "L? " + r;
                } else a.end = (E || A) ? "hold (bar already marked)" : "hold, no supporting read";
                close_(a);
                continue;
            }
            if (bar - a.barA >= P.maxBars) { a.end = "expired"; close_(a); }
        }
        attempts.erase(std::remove_if(attempts.begin(), attempts.end(), [](const Attempt& a) { return a.done; }), attempts.end());
        curBar = bar;
    }
    int curBar = 0;

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
        r.pulling = r.pulled >= 0.5 && r.refill >= 0 && r.refill < 0.5;
        r.depleting = !r.pulling && r.eaten >= P.eatenBreak && r.refill >= 0 && r.refill < 0.5;
        return r;
    }

    // the record line of a mark (pipe-separated, append-only; LiquidityProfile\<MKT>-<spb>-signals-<session>.csv)
    static std::string header() { return "bar_time|code|reasons|side|zone_lo|zone_hi|shown|eaten|stayed|exhaust|resilience|t_arrival|t_decided|version"; }
    static std::string line(const Mark& m, double tick) {
        std::ostringstream o; o.imbue(std::locale::classic()); o.precision(10);
        o << m.barTime << '|' << m.code << '|' << m.reasons << '|' << (m.side == BID ? "support" : "resistance") << '|'
          << (double)m.lo * tick << '|' << (double)m.hi * tick << '|' << m.shown << '|' << round2(m.eaten) << '|' << round2(m.stayed) << '|'
          << round2(m.exhaust) << '|' << round2(m.resil) << '|' << m.tA << '|' << m.tDecided << '|' << VERSION;
        return o.str();
    }
    static bool num(const std::string& t, double& v) {                 // strict full-token parse (checklist #4)
        if (t.empty() || t.size() > 40) return false;
        char* e = nullptr; v = std::strtod(t.c_str(), &e); return e == t.c_str() + t.size() && finite(v);
    }
    static bool parse(const std::string& s, Mark& m, double tick) {
        std::vector<std::string> f; size_t a = 0;
        for (size_t i = 0; i <= s.size(); ++i) if (i == s.size() || s[i] == '|') { f.push_back(s.substr(a, i - a)); a = i + 1; }
        if (f.size() < 14 || (f[1] != "LX" && f[1] != "L?") || !(tick > 0)) return false;
        double bt, lo, hi, ta, td;
        if (!num(f[0], bt) || bt <= 0 || !num(f[4], lo) || !num(f[5], hi) || !num(f[6], m.shown) || !num(f[7], m.eaten) || !num(f[8], m.stayed)
            || !num(f[9], m.exhaust) || !num(f[10], m.resil) || !num(f[11], ta) || !num(f[12], td)) return false;
        if (f[3] != "support" && f[3] != "resistance") return false;
        m.barTime = (long long)bt; m.code = f[1]; m.reasons = f[2]; m.side = f[3] == "support" ? BID : ASK;
        m.lo = toTick(lo, tick); m.hi = toTick(hi, tick); m.tA = (Ms)ta; m.tDecided = (Ms)td;
        return m.hi >= m.lo;
    }
    static double round2(double v) { return std::floor(v * 100 + 0.5) / 100; }

    // the attempt log line (every attempt, drawn or not) for the nightly scoring
    static std::string attemptHeader() { return "t_arrival|side|zone_lo|zone_hi|shown|eaten|stayed|added_x|refill_x|exhaust|resilience|breach_trade|end|version|pushback|defender_speed|pull_speed|response_cross|t_end"; }
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
        return o.str();
    }
    std::vector<std::string> attemptLog;          // lines waiting to be appended by the host

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
            Attempt& a = attempts[i]; if (a.done || a.side != e.side || e.px < a.lo || e.px > a.hi || e.t < a.tA) continue;
            a.pulled += e.pulled; a.added += e.added; a.refill += e.refill;
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
            Attempt a; a.zoneId = z.id; a.side = z.side; a.lo = z.lo; a.hi = z.hi; a.tA = t; a.barA = curBar;
            a.shown = shown; a.startSize = a.minSize = a.curSize = shown;
            const double first = a.side == BID ? sell : buy;          // the trade that arrived counts as the first attack
            if (first > 0) { a.attackVol = first; a.attacks.push_back(std::make_pair(t, first)); }
            attempts.push_back(a); z.inAttempt = true;
        }
    }
    void push(const char* code, const std::string& reasons, const Attempt& a, long long barTime, Ms barEnd) {
        Mark m; m.code = code; m.reasons = reasons; m.side = a.side; m.barTime = barTime; m.lo = a.lo; m.hi = a.hi; m.shown = a.shown;
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
