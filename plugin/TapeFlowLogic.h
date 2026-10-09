/********************************************************************************
 *  TapeFlowLogic.h  --  the testable half of lsTapeFlow<MKT> (no IRT SDK)          v2.0.0 (2026-10-09)
 *  (2.0.0) + the SIGNALS (P push / E exhaustion / A absorption / F flip) decided on closed bars only - see "THE SIGNALS" below.
 *
 *  Rassul 2026-10-08 13:17-13:26 (TapeFlow brief): "review ... then we will build it out on irt"; "get it on all the markets,
 *  if signals are tentative just add ?"; "the markers should be on the indicator on a separate pane"; "keep levels out of it,
 *  similar to the delta profile"; "make sure you add backfill ... it must be native without any outside dependencies".
 *
 *  What it computes, one second at a time, from IRT's own trades (price, size, bid, ask at the trade):
 *    PRESSURE   F30 / F180 = 100 (B - S) / (B + S) over 30 s / 180 s; coverage C = (B + S) / (B + S + U); the unknown-side
 *               bounds L = 100 (B - S - U) / total, H = 100 (B - S + U) / total. Side: a trade at / above the ask = BUY, at / below
 *               the bid = SELL, between = unknown (IRT's quote at the trade - "platform quote rule", not an exchange flag).
 *    ACTIVITY   A = (all quantity in 20 s / 20) / the median 20-s rate of the same 5-minute session bin (prior sessions only).
 *    BASELINES  non-overlapping 20-s windows of up to 10 prior sessions (FRONT-ROLL by default; configurable same contract), per 5-minute bin: the 95th percentile
 *               (nearest rank) of the biggest same-side volume in any 2h+1-tick band (h = 1..8, bands with >= 90% known side),
 *               the median 20-s and 5-s rates and the median spread. 60+ windows in the bin or no signals (CALIBRATING).
 *    WATCH      AW+ / AW- (brief section 7) at the band with the most attempted-side volume in the last 20 s (NO levels -
 *               Rassul 13:24): sellers hitting a band without progress = AW+ (bullish watch), buyers = AW-. Conditions 1-8,
 *               candidate -> 3-s hold with fresh in-zone execution -> AW; 10-s candidate expiry; a breach cancels + latches.
 *    OUTCOMES   FAILED_ACCEPTANCE / INVALIDATED_PRICE (3 s beyond the adverse edge with / without 10% Q95 executed there),
 *               EXPIRED (90 s), DATA_INVALID (gap), RESPONSE (AR, section 9), ATTACK_FADING (diagnostic).
 *    INITIATIVE IN+ / IN- (section 10): L30 >= 35, L180 > 0, A >= 1.2, 5-s rate >= its median, acceptance 3 s above the frozen
 *               preceding-20-s high + 1 tick with a fresh BUY in the final second. AR beats IN in the same second.
 *  Precedence each second: data invalid -> expiry -> adverse failure -> AR -> IN -> new candidates. Every transition is an
 *  event record (never edited later). Same inputs in the same order = the same events (prefix / speed invariant).
 *  Quote snapshots only occur at trades: unobserved quote changes cannot be reconstructed.
 *  Invalid/stale/missing observed paths fail closed; this is not a raw quote-path API.
 *  Ev.t is the evaluated second label; Ev.knownAt is earliest conservative availability (t+1).
 *  (1.1.5, Rassul 2026-10-09 09:36 "GC and HG are always CALIBRATING or QUIET"):
 *    - a 20-s baseline window no longer needs a trade in every 5 s (a "continuous quote path"): GC lost ~44% and HG ~90% of
 *      its RTH windows that way, so a 5-min slot topped out near 30 (GC) / 3 (HG) of the 60 it needs. A window now counts when
 *      its 20 seconds are contiguous, no trade had a missing / locked / crossed quote and >= 90% of its volume has a known side;
 *      a window with NO trade counts too (rate 0) - a quiet 20 s is a real observation of how busy that time of day is.
 *      Each window also records how many of its 20 seconds traded (act) -> the slot's typical gap between trades.
 *    - a slot with fewer than minWin windows borrows its neighbours symmetrically (+-1, then +-2 slots); BinBase.k says how far.
 *    - "current quote" / QUIET adapt to the market: a quote stays current for max(quoteAge, 3 x the slot's typical gap between
 *      trades) (capped at maxQuoteAge); ES / NQ (a trade every second) are unchanged at 5 s.
 *  Prices are integer ticks, midpoints integer half-ticks (bid + ask). Times are LOCAL wall-clock seconds (seconds since
 *  1970 of the local date / time), so the 17:00 session and the 5-minute bins follow the clock through DST.
 ********************************************************************************/
#pragma once
#include <vector>
#include <deque>
#include <map>
#include <string>
#include <cmath>
#include <climits>
#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <locale>
#include <set>
#include <unordered_set>

namespace tfl {

#define TFL_VERSION "2.0.0"

struct Cfg {
    int fastW = 30, ctxW = 180, obsW = 20, part = 5;   // seconds
    int holdS = 3, candTTL = 10, watchTTL = 90, inTTL = 10;
    int minWin = 60, sessionsBack = 10;
    double conc = 0.50, partFrac = 1.0 / 8.0, fresh = 0.10, cov = 0.90;
    double arImprove = 15, inFast = 35, inAct = 1.2, neutral = 15;
    int hMin = 1, hMax = 8, warm = 200, quoteAge = 5, quietReset = 30, inRearm = 5;
    int maxQuietGap = 300;                             // no trade for longer = the tape stopped (closed market / feed gap)
    int maxPool = 2;                                   // (1.1.5) a thin slot borrows up to +-2 neighbouring 5-min slots
    double quietMult = 3.0; int maxQuoteAge = 30;      // (1.1.5) quote current for max(quoteAge, 3 x typical trade gap), <= 30 s
    int keepSec = 3 * 86400;                           // per-second history kept for drawing
    bool frontRoll = true;                            // explicit default: historical front contracts across rolls
    bool validate(std::string* why = nullptr) const
    {
        const char* error = nullptr;
        if (obsW != 20 || part != 5) error = "obsW=20 and part=5 required by fixed baseline schema";
        else if (fastW <= 0 || ctxW < fastW || ctxW > 86400 || warm <= 0 || warm > 86400) error = "invalid flow/warm windows";
        else if (holdS <= 0 || candTTL <= holdS || watchTTL <= 0 || inTTL <= holdS || minWin <= 0 || sessionsBack <= 0 || sessionsBack > 1000) error = "invalid hold/TTL/baseline counts";
        else if (hMin < 1 || hMax > 8 || hMin > hMax) error = "band h must be in 1..8";
        else if (maxPool < 0 || maxPool > 12 || !std::isfinite(quietMult) || quietMult < 0 || quietMult > 20 || maxQuoteAge < quoteAge || maxQuoteAge > 120) error = "invalid pooling / quiet settings";
        else if (quoteAge < 0 || maxQuietGap <= 0 || quietReset <= 0 || inRearm <= 0 || keepSec < 0 || keepSec > INT_MAX - 3600) error = "invalid timing/retention";
        else if (!std::isfinite(conc) || conc < 0 || conc > 1 || !std::isfinite(partFrac) || partFrac < 0 || partFrac > 1 ||
                 !std::isfinite(fresh) || fresh < 0 || fresh > 1 || !std::isfinite(cov) || cov <= 0 || cov > 1 ||
                 !std::isfinite(arImprove) || arImprove < 0 || arImprove > 200 || !std::isfinite(inFast) || inFast < 0 || inFast > 100 ||
                 !std::isfinite(inAct) || inAct <= 0 || !std::isfinite(neutral) || neutral < 0 || neutral > 100) error = "invalid thresholds";
        if (why) *why = error ? error : "";
        return error == nullptr;
    }
};

enum { SIDE_BUY = 0, SIDE_SELL = 1, SIDE_UNK = 2 };

struct Tick { long long t = 0; int px = 0; int bid = 0; int ask = 0; long long q = 0; };   // bid / ask <= 0 = no quote

inline int sideOf(int px, int bid, int ask)
{
    if (bid <= 0 || ask <= 0 || ask <= bid || (long long)bid + ask > INT_MAX - 32LL) return SIDE_UNK;      // no quote, crossed or locked: no side
    if (px >= ask) return SIDE_BUY;
    if (px <= bid) return SIDE_SELL;
    return SIDE_UNK;
}

// the futures session: 17:00 local -> the next weekday; bins of 5 minutes from 17:00
inline long long sessionOf(long long t) { long long x = t - 17LL * 3600; long long d = x >= 0 ? x / 86400 : -((-x + 86399) / 86400); return d + 1; }
inline int binOf(long long t) { long long x = (t - 17LL * 3600) % 86400; if (x < 0) x += 86400; return (int)(x / 300); }
static const int NBINS = 288;

struct Row { int px; long long b, s, u; };
struct Slice {
    long long t = 0;
    std::vector<Row> rows;
    long long B = 0, S = 0, U = 0;
    std::vector<int> mids;          // the midpoint path (half-ticks) through the second; [0] = the value carried in
    bool path = false;              // continuous observed quote-at-trade proxy; NOT proof of full quote path
    bool quoteBroken = false;       // any missing/locked/crossed quote poisons this whole second
    bool carried = false;           // a quote was known when the second began
    int spread = -1;                // maximum observed spread in this second (including carry-in)
    int mn() const { int m = INT_MAX; for (int v : mids) m = std::min(m, v); return m; }
    int mx() const { int m = INT_MIN; for (int v : mids) m = std::max(m, v); return m; }
    long long side(int sd) const { return sd == SIDE_BUY ? B : S; }
};

// one baseline window (stored per session)
struct Win { long long endT = LLONG_MIN; int bin = 0; float rate20 = 0, r5[4] = {0, 0, 0, 0}, spread = -1; float Mb[8] = {0}, Ms[8] = {0}; float act = -1; };   // act (1.1.5): seconds of the 20 with a trade (-1 = not recorded)
typedef std::map<std::string, std::vector<Win>> SessWins;      // contract -> windows (a roll day can hold two)
typedef std::map<long long, SessWins> Store;                    // session -> contracts -> windows
inline double avgRate(const std::vector<Win>& v) { double a = 0; for (const Win& w : v) a += w.rate20; return v.empty() ? 0 : a / (double)v.size(); }
// Exact identity is (session, symbol, endT); first committed measurement wins.
// Legacy count-only windows have no identity and cannot safely calibrate or merge.
inline bool identified(const Win& w) {
    return w.endT >= LLONG_MIN + 1000000LL && w.endT <= LLONG_MAX - 1000000LL &&
           w.bin >= 0 && w.bin < NBINS && binOf(w.endT) == w.bin && (w.endT + 1 - 17LL * 3600) % 20 == 0;
}
inline void mergeWindows(std::vector<Win>& dst, const std::vector<Win>& src)
{
    std::map<long long, size_t> seen;
    std::vector<Win> merged; merged.reserve(dst.size() + src.size());
    for (const Win& w : dst) if (identified(w) && seen.insert(std::make_pair(w.endT, merged.size())).second) merged.push_back(w);
    for (const Win& w : src) {
        if (!identified(w)) continue;
        auto ins = seen.insert(std::make_pair(w.endT, merged.size()));
        if (ins.second) merged.push_back(w);
        else if (merged[ins.first->second].act < 0 && w.act >= 0) merged[ins.first->second].act = w.act;   // (1.1.5) enrich only
    }
    std::sort(merged.begin(), merged.end(), [](const Win& a, const Win& b) { return a.endT < b.endT; });
    dst.swap(merged);
}
inline void mergeStore(Store& dst, const Store& src)
{
    for (const auto& session : src) for (const auto& contract : session.second) {
        std::vector<Win> valid;
        for (const Win& w : contract.second) if (identified(w) && sessionOf(w.endT) == session.first) valid.push_back(w);
        if (!valid.empty()) mergeWindows(dst[session.first][contract.first], valid);
    }
}
inline double executedVolume(const std::vector<Win>& v)
{
    double volume = 0; std::unordered_set<long long> seen; seen.reserve(v.size());
    for (const Win& w : v) if (identified(w) && seen.insert(w.endT).second) volume += 20.0 * w.rate20;
    return volume;
}
// Front is selected by total executed volume in eligible windows, not average thin-window rate.
inline const std::vector<Win>* frontOf(const SessWins& session)
{
    const std::vector<Win>* best = nullptr; double bv = 0;
    for (const auto& kv : session) { double v = executedVolume(kv.second); if (v > bv) { bv = v; best = &kv.second; } }
    return best; // deterministic lexical-symbol tie break
}

struct BinBase { int n = 0; float qb[8] = {0}, qs[8] = {0}; float med20 = 0, med5 = 0, medSpread = -1;
                 int own = 0, k = 0; float gap = -1; };   // (1.1.5) own = the slot's own windows, k = slots borrowed each side, gap = typical s between trades

inline float nearestRank(std::vector<float>& v, double p)
{
    if (v.empty()) return 0;
    if (!std::isfinite(p)) return 0;
    p = std::max(0.0, std::min(1.0, p));
    long k = (long)std::ceil(p * (double)v.size()) - 1; if (k < 0) k = 0; if (k >= (long)v.size()) k = (long)v.size() - 1;
    std::nth_element(v.begin(), v.begin() + k, v.end());
    return v[(size_t)k];
}
inline float median(std::vector<float>& v)
{
    if (v.empty()) return 0;
    size_t n = v.size();
    std::nth_element(v.begin(), v.begin() + n / 2, v.end());
    float upper = v[n / 2];
    return n % 2 ? upper : (float)(0.5 * (double)*std::max_element(v.begin(), v.begin() + n / 2) + 0.5 * (double)upper);
}

// (1.1.5) the typical rate: the median, or the mean when more than half the windows had no trade at all (a thin market's
// overnight slot) - a zero median would leave the slot uncalibrated for ever.
inline float typicalRate(std::vector<float>& v)
{
    if (v.empty()) return 0;
    float m = median(v);
    if (m > 0) return m;
    double a = 0; for (float x : v) a += x; return (float)(a / (double)v.size());
}
// the statistics of one slot from its (possibly pooled) windows
inline void binStats(const std::vector<const Win*>& v, BinBase& o)
{
    o.n = (int)v.size(); if (!o.n) return;
    std::vector<float> a; a.reserve(v.size() * 4);
    for (int h = 0; h < 8; ++h) {
        a.clear(); for (auto w : v) a.push_back(w->Mb[h]); o.qb[h] = nearestRank(a, 0.95);
        a.clear(); for (auto w : v) a.push_back(w->Ms[h]); o.qs[h] = nearestRank(a, 0.95);
    }
    a.clear(); for (auto w : v) a.push_back(w->rate20); o.med20 = typicalRate(a);
    a.clear(); for (auto w : v) for (int k = 0; k < 4; ++k) a.push_back(w->r5[k]); o.med5 = typicalRate(a);
    a.clear(); for (auto w : v) if (w->spread >= 0) a.push_back(w->spread); o.medSpread = a.empty() ? -1 : median(a);
    a.clear(); for (auto w : v) if (w->act >= 0) a.push_back(w->act);
    if (a.empty()) o.gap = -1.f;
    else { double m = 0; for (float x : a) m += x; m /= (double)a.size(); o.gap = (float)(20.0 / std::max(0.5, m)); }   // 20 s / mean traded seconds = mean s between trades
}

// Prior-only frozen cohort. FRONT-ROLL is the default; frontRoll=false selects symbol only.
// No count-only legacy windows: overlapping historical loads must not inflate calibration.
// (1.1.5) minWin > 0: a slot with fewer windows pools +-1, then +-2 ... (up to maxPool) neighbouring slots, never across
// the 17:00 start of the session. dropped = sessions left out as too thin (< 0.3 x the typical session's rate).
inline void freezeBase(const Store& st, long long sid, int N, BinBase* out, int* usedSessions,
                       const std::string& symbol = std::string(), bool frontRoll = true, int minWin = 0, int maxPool = 0,
                       int* droppedSessions = nullptr)
{
    if (!out) return;
    for (int b = 0; b < NBINS; ++b) out[b] = BinBase();
    if (usedSessions) *usedSessions = 0;
    if (droppedSessions) *droppedSessions = 0;
    if (N <= 0 || N > 1000) return;
    std::vector<std::vector<Win>> candidates;
    for (auto it = st.rbegin(); it != st.rend() && (int)candidates.size() < 2 * N; ++it) {
        if (it->first >= sid) continue;
        const std::vector<Win>* selected = nullptr;
        if (frontRoll) selected = frontOf(it->second);
        else { auto found = it->second.find(symbol); if (found != it->second.end()) selected = &found->second; }
        if (!selected) continue;
        std::vector<Win> wins; wins.reserve(selected->size()); std::unordered_set<long long> seen; seen.reserve(selected->size());
        for (const Win& w : *selected)   // (1.1.5) rate20 == 0 windows count: a quiet 20 s is an observation
            if (identified(w) && sessionOf(w.endT) == it->first && seen.insert(w.endT).second && w.rate20 >= 0) wins.push_back(w);
        if (!wins.empty() && avgRate(wins) > 0) candidates.push_back(wins);
    }
    std::vector<float> rates;
    for (const auto& wins : candidates) rates.push_back((float)avgRate(wins));
    float typical = median(rates);
    std::vector<std::vector<const Win*>> by(NBINS);
    int used = 0, dropped = 0;
    for (const auto& wins : candidates) {
        if (avgRate(wins) < 0.3 * typical) { ++dropped; continue; }
        for (const Win& w : wins) by[(size_t)w.bin].push_back(&w);
        if (++used >= N) break;
    }
    if (usedSessions) *usedSessions = used;
    if (droppedSessions) *droppedSessions = dropped;
    std::vector<const Win*> pooled;
    for (int b = 0; b < NBINS; ++b) {
        BinBase& o = out[b];
        o.own = (int)by[(size_t)b].size(); o.k = 0;
        pooled.assign(by[(size_t)b].begin(), by[(size_t)b].end());
        for (int k = 1; minWin > 0 && k <= maxPool && (int)pooled.size() < minWin; ++k) {
            if (b - k >= 0) pooled.insert(pooled.end(), by[(size_t)(b - k)].begin(), by[(size_t)(b - k)].end());
            if (b + k < NBINS) pooled.insert(pooled.end(), by[(size_t)(b + k)].begin(), by[(size_t)(b + k)].end());
            o.k = k;
        }
        if (o.k && (int)pooled.size() == o.own) o.k = 0;   // the neighbours had nothing: not pooled
        binStats(pooled, o);
        o.own = (int)by[(size_t)b].size();
    }
}

// the volume of each price over some slices, dense from lo to hi (tick prices)
struct Dense {
    int lo = 0, hi = -1; bool valid = true; std::vector<long long> b, s, u;
    std::vector<long long> pb, ps, pu;
    void build(const std::deque<Slice>& R, size_t from, size_t to)
    {
        valid = true; b.clear(); s.clear(); u.clear(); pb.clear(); ps.clear(); pu.clear();
        lo = INT_MAX; hi = INT_MIN;
        if (from > to || to > R.size()) { valid = false; lo = 0; hi = -1; return; }
        for (size_t i = from; i < to; i++) for (const Row& r : R[i].rows) { lo = std::min(lo, r.px); hi = std::max(hi, r.px); }
        if (lo > hi) { lo = 0; hi = -1; return; }
        if ((long long)hi - lo > 4000) { valid = false; lo = 0; hi = -1; return; }
        size_t n = (size_t)(hi - lo + 1); b.assign(n, 0); s.assign(n, 0); u.assign(n, 0);
        for (size_t i = from; i < to; i++) for (const Row& r : R[i].rows) {
            if (r.b < 0 || r.s < 0 || r.u < 0 || r.b > LLONG_MAX - r.s || r.u > LLONG_MAX - r.b - r.s) { valid = false; lo = 0; hi = -1; return; }
            size_t k = (size_t)((long long)r.px - lo);
            if (r.b > LLONG_MAX - b[k] || r.s > LLONG_MAX - s[k] || r.u > LLONG_MAX - u[k]) { valid = false; lo = 0; hi = -1; return; }
            b[k] += r.b; s[k] += r.s; u[k] += r.u;
        }
        pb.assign(n + 1, 0); ps.assign(n + 1, 0); pu.assign(n + 1, 0);
        long long total = 0;
        for (size_t k = 0; k < n; ++k) {
            if (b[k] > LLONG_MAX - total) { valid = false; return; } total += b[k];
            if (s[k] > LLONG_MAX - total) { valid = false; return; } total += s[k];
            if (u[k] > LLONG_MAX - total) { valid = false; return; } total += u[k];
            pb[k + 1] = pb[k] + b[k]; ps[k + 1] = ps[k] + s[k]; pu[k + 1] = pu[k] + u[k];
        }
    }
    // the band [c-h, c+h] with the most known volume of `sd` among bands with >= cov known side; false = none eligible.
    // Ties (mirror-symmetric): nearest the single price with the most `sd` volume, then nearest the midpoint (half-ticks),
    // then the lower band for sellers / the higher band for buyers.
    bool best(int sd, int h, double cov, int mid2, int* center, long long* vol, double* bandCov) const
    {
        if (!valid || hi < lo || h < 1 || h > 8 || !std::isfinite(cov) || cov < 0 || cov > 1) return false;
        int n = hi - lo + 1;
        const std::vector<long long>& V = sd == SIDE_BUY ? b : s;
        int pk = lo; long long pv = -1;
        for (int k = 0; k < n; k++) { long long v = V[(size_t)k]; int px = lo + k; if (v > pv || (v == pv && (sd == SIDE_SELL ? px < pk : px > pk))) { pv = v; pk = px; } }
        bool found = false; long long bestV = -1; int bestC = 0; double bestCov = 0;
        for (int k = 0; k < n; ++k) {
            int c = lo + k;
            if (c < INT_MIN / 2 + 32 || c > INT_MAX / 2 - 32) continue; // half-tick API cannot represent this band
            int a = std::max(lo, c - h), z = std::min(hi, c + h);
            int ia = a - lo, iz = z - lo + 1;
            long long B = pb[iz] - pb[ia], S = ps[iz] - ps[ia], U = pu[iz] - pu[ia];
            if (B + S + U <= 0) continue;
            double cv = (double)(B + S) / (double)(B + S + U);
            if (cv < cov) continue;
            long long v = sd == SIDE_BUY ? B : S;
            bool better = !found || v > bestV;
            if (found && v == bestV) {
                int d1 = std::abs(c - pk), d0 = std::abs(bestC - pk);
                if (d1 != d0) better = d1 < d0;
                else if (mid2 != INT_MIN && std::llabs(2LL * c - mid2) != std::llabs(2LL * bestC - mid2)) better = std::llabs(2LL * c - mid2) < std::llabs(2LL * bestC - mid2);
                else better = sd == SIDE_SELL ? c < bestC : c > bestC;
            }
            if (better) { found = true; bestV = v; bestC = c; bestCov = cv; }
        }
        if (!found) return false;
        if (center) *center = bestC;
        if (vol) *vol = bestV;
        if (bandCov) *bandCov = bestCov;
        return true;
    }
    void band(int a, int z, long long* B, long long* S, long long* U) const
    {
        *B = *S = *U = 0;
        if (!valid || hi < lo || a > z) return;
        int first = std::max(a, lo), last = std::min(z, hi); if (first > last) return;
        size_t x = (size_t)((long long)first - lo), y = (size_t)((long long)last - lo + 1);
        *B = pb[y] - pb[x]; *S = ps[y] - ps[x]; *U = pu[y] - pu[x];
    }
};

struct Ev {
    long long t = 0, knownAt = 0; int ep = 0; std::string kind; int dir = 0;    // dir +1 bullish, -1 bearish
    int lo = 0, hi = 0, h = 0; double q95 = 0, f30 = NAN, f180 = NAN, a = NAN, cov = NAN, qzone = 0, conc = 0, prog = 0;
    std::string ctx, why; int ver = 0;
};

struct SecRec { long long t = 0; float f30 = NAN, f180 = NAN, a = NAN, c30 = NAN, l30 = NAN, h30 = NAN; long long b = 0, s = 0, u = 0; unsigned char flags = 0; };   // exact executed quantities (unknown included), not float histogram rounding
enum { SR_QUALITY = 1, SR_WARM = 2, SR_BASE = 4, SR_LOWACT = 8, SR_SIDES = 16 };

struct Feat {
    bool f30ok = false, f180ok = false, f20ok = false;
    double f30 = 0, l30 = 0, h30 = 0, c30 = 0, f180 = 0, l180 = 0, h180 = 0, c180 = 0, c20 = 0;
    double rate20 = 0, rate5 = 0, A = NAN; bool aok = false;
    double quoteAgeEff = 5, gapTypical = -1; long long sinceTrade = -1; bool gapFromBase = false;   // (1.1.5) adaptive quiet
    bool quoteResumed = false;       // (1.1.5) the quote is current again but this second had no carried-in quote (first trade after a lull)
    int R = 0, h = 1; bool quoteOk = false, spreadOk = true, spreadCal = false, classRecent = false, warm = false, baseOk = false, rangeOk = false;
    int bin = 0; BinBase bbv; int mid2 = INT_MIN;      // a COPY of the slot's baselines (no pointer: engines are moved)
    const BinBase* bb() const { return &bbv; }
};

struct Ep {
    int id = 0, dir = 0, state = 0;                // dir +1 bullish (sellers attempted), -1 bearish; state 1 candidate, 2 watch
    int lo = 0, hi = 0, h = 1; double q95 = 0, med20 = 0, med5 = 0; double medSpread = -1; int baseN = 0; int entry2 = 0, ext2 = 0;
    long long t0 = 0, fireT = 0; int holdN = 0; bool holdFresh = false;
    int breachN = 0; double breachQty = 0;
    double f30aw = 0, covAW = 0; bool faded = false;
    int arN = 0; double arQty = 0, arKnown = 0, arAll = 0;
    int ver = 0;
};
struct Latch { int dir = 0, lo = 0, hi = 0, h = 1; double q95 = 0; bool movedAway = false; int quietN = 0; };
struct InSetup { bool on = false; long long t0 = 0; int bound2 = 0; double med5 = 0, med20 = 0, medSpread = -1; int baseN = 0; int n = 0; double qty = 0, known = 0, all = 0; };

class Engine {
public:
    Cfg cfg;
    Store* store = nullptr;          // baseline windows (shared with the file); this engine adds the sessions it completes
    std::string sym;
    std::vector<SecRec> hist;
    std::vector<Ev> evs;
    long long ticks = 0, late = 0;
    int ver = 1;                     // the baseline / config version (bumped when the baselines are frozen again)
    int baseSessions = 0;            // prior sessions behind the current baselines
    int baseDropped = 0;             // (1.1.5) prior sessions left out as too thin
    long long curSid = LLONG_MIN;
    std::string lastWhy;             // why the last second showed no signal (for the readout)
    BinBase base[NBINS];
    bool fixedBase = false;          // tests: baselines given, not frozen from the store

    Engine() {}
    bool configOk(std::string* why = nullptr) const { return cfg.validate(why); }
    void attach(Store* baselineStore, const std::string& symbol) { store = baselineStore; sym = symbol; }

    // ---- input
    bool add(const Tick& k)
    {
        if (!configOk(&lastWhy)) { gap(k.t, "invalid configuration"); return false; }
        if (k.t < LLONG_MIN + 1000000LL || k.t > LLONG_MAX - 1000000LL) { lastWhy = "timestamp outside supported range"; return false; }
        if (k.q <= 0) return true;
        if (k.px <= 0 || k.px > INT_MAX / 2 - 32) { gap(k.t, "price outside supported half-tick range"); return false; }
        if ((open && k.t < cur.t) || (haveLastT && k.t <= lastT)) {   // late: the second was already evaluated - never rewrite it
            late++; lateNote(k.t); return false;            // counted, never added to a closed (or the wrong) second
        }
        if (!open) { stopped = false; startAt(k.t); }
        else if (k.t > cur.t) { closeUntil(k.t - 1); stopped = false; startAt(k.t); }
        const long long currentQty = cur.B + cur.S + cur.U;
        if (k.q > LLONG_MAX - retainedQty - currentQty) { gap(k.t, "quantity accumulation overflow"); return false; }
        if (k.t != lastTradeT) noteTradeSecond(k.t);
        addTo(cur, k); ticks++; lastTradeT = k.t;
        return true;
    }
    // close every second up to and including t (the clock moved on without trades)
    void advanceTo(long long t) {
        if (!configOk(&lastWhy)) { gap(t, "invalid configuration"); return; }
        if (t < LLONG_MIN + 1000000LL || t > LLONG_MAX - 1000000LL) { lastWhy = "timestamp outside supported range"; return; }
        if (open && t >= cur.t) { closeUntil(t); if (!stopped) startAt(t + 1); }
    }
    // a hole in the data (feed gap, missed request): episodes resolve DATA_INVALID, the warm-up starts over
    void gap(long long t, const std::string& why)
    {
        for (int d = 0; d < 2; d++) if (ep[d].state) terminal(ep[d], t, "DINV", why);
        for (int d = 0; d < 2; d++) inS[d] = InSetup();
        warmN = 0; R.clear(); retainedQty = 0; open = false; haveMid = false;
        quoteT = lastClass = lastTradeT = LLONG_MIN; mid2 = 0; spreadNow = -1;
        latches.clear(); balance = false; stopped = true; last = Feat(); lastWhy = why;
        for (int d = 0; d < 2; ++d) { inLatched[d] = false; inQuiet[d] = 0; }
        // Keep lastT/haveLastT: a gap must never reopen already committed seconds.
    }
    void setFixedBase(const BinBase& b) { fixedBase = true; for (int i = 0; i < NBINS; i++) base[i] = b; }
    void refreeze() { if (!fixedBase && store && curSid != LLONG_MIN) { freezeBase(*store, curSid, cfg.sessionsBack, base, &baseSessions, sym, cfg.frontRoll, cfg.minWin, cfg.maxPool, &baseDropped); ver++; } }
    // (1.1.5) how long a quote stays current at t: max(quoteAge, quietMult x the typical gap between trades), <= maxQuoteAge.
    // The typical gap is the slot's (from the baseline windows' traded seconds) or, before any window has it, the median gap
    // between this engine's own recent traded seconds (the last 60 traded seconds within 15 min). ES / NQ (gap ~1 s) stay at quoteAge = 5 s.
    double typicalGap(long long t, bool* fromBase = nullptr) const
    {
        const BinBase& b = base[binOf(t)];
        if (fromBase) *fromBase = b.gap > 0;
        if (b.gap > 0) return b.gap;
        return liveGap;
    }
    int quoteAgeAt(long long t) const
    {
        double g = typicalGap(t);
        double a = cfg.quoteAge;
        if (g > 0 && std::isfinite(g)) a = std::max(a, std::ceil(cfg.quietMult * g - 1e-9));
        return (int)std::min<double>(a, (double)cfg.maxQuoteAge);
    }
    long long lastTrade() const { return lastTradeT; }
    long long noFlushSid = LLONG_MIN;   // a back-fill's oldest, partial session is never stored as a prior session

    // ---- state for the readout
    const Ep& episode(int d) const { return ep[d]; }
    Feat last;                       // the features of the last closed second
    long long lastT = 0;
    int warmN = 0;
    bool watching(int* dir, const Ep** e) const
    {
        for (int d = 0; d < 2; d++) if (ep[d].state == 2) { if (dir) *dir = ep[d].dir; if (e) *e = &ep[d]; return true; }
        return false;
    }

private:
    Slice cur; bool open = false, haveLastT = false;
    long long retainedQty = 0;
    std::deque<Slice> R;             // the last ~200 closed seconds
    bool haveMid = false; int mid2 = 0; long long quoteT = LLONG_MIN; int spreadNow = -1;
    long long lastClass = LLONG_MIN;
    Ep ep[2];                        // [0] bullish (+1), [1] bearish (-1)
    std::vector<Latch> latches;
    InSetup inS[2]; bool inLatched[2] = {false, false}; int inQuiet[2] = {0, 0};
    bool balance = false; long long lastTradeT = LLONG_MIN; bool stopped = false;
    std::deque<long long> tradeSecs; double liveGap = -1;   // (1.1.5) the last traded seconds -> the live typical gap
    void noteTradeSecond(long long t)
    {
        tradeSecs.push_back(t);
        while (tradeSecs.size() > 61 || (!tradeSecs.empty() && tradeSecs.front() < t - 900)) tradeSecs.pop_front();
        if (tradeSecs.size() < 11) { liveGap = -1; return; }
        liveGap = (double)(tradeSecs.back() - tradeSecs.front()) / (double)(tradeSecs.size() - 1);   // the mean spacing of traded seconds
    }
    // episode ids from the creation second + side (+ kind): the same event gets the same id in every rebuild
    static int epId(long long t, int dir, bool initiative) { long long b = ((t % 10000000) + 10000000) % 10000000; return (int)(b * 4 + (dir > 0 ? 1 : 0) + (initiative ? 2 : 0)); }
    std::vector<Win> sessWins;       // the current session's windows

    static int di(int dir) { return dir > 0 ? 0 : 1; }
    static int attemptedSide(int dir) { return dir > 0 ? SIDE_SELL : SIDE_BUY; }

    void lateNote(long long t)
    {
        // (1.1.2) a trade stamped in a second that is already closed (IRT delivers a few a minute, slightly out of order) is
        // COUNTED and left out, and any open episode whose look-back includes that second is invalidated (DINV) - as in 1.0.x.
        // 1.1.0 treated each one as a data hole: on the live ES feed (14 late trades in 6 minutes) that cleared the 180-s
        // history and restarted the warm-up every minute or two, so the 180-s line vanished and no signal could ever fire.
        // Committed records are still never rewritten.
        for (int d = 0; d < 2; d++) if (ep[d].state && t >= ep[d].t0 - (cfg.ctxW + cfg.obsW)) terminal(ep[d], haveLastT ? lastT : cur.t, "DINV", "late trade data");
    }
    void startAt(long long t)
    {
        long long sid = sessionOf(t);
        if (sid != curSid) newSession(sid);
        cur = Slice(); cur.t = t; open = true; stopped = false; cur.carried = haveMid && (t - quoteT) <= quoteAgeAt(t);
        if (cur.carried) { cur.mids.push_back(mid2); cur.spread = spreadNow; }
    }
    void newSession(long long sid)
    {
        flushSession();
        if (curSid != LLONG_MIN) gap(lastT, "session boundary"); // no episode/path carry across a market halt
        curSid = sid; sessWins.clear();
        if (!fixedBase && store) { freezeBase(*store, curSid, cfg.sessionsBack, base, &baseSessions, sym, cfg.frontRoll, cfg.minWin, cfg.maxPool, &baseDropped); ver++; }
    }
public:
    // the current session's windows -> the store (call any time; the engine calls it at a session change)
    void flushSession()
    {
        if (!store || curSid == LLONG_MIN || sessWins.empty() || curSid == noFlushSid) return;
        std::vector<Win>& v = (*store)[curSid][sym];
        mergeWindows(v, sessWins);
    }
private:
    void addTo(Slice& s, const Tick& k)
    {
        int sd = sideOf(k.px, k.bid, k.ask);
        Row* r = nullptr; for (Row& x : s.rows) if (x.px == k.px) { r = &x; break; }
        if (!r) { s.rows.push_back(Row{k.px, 0, 0, 0}); r = &s.rows.back(); }
        if (sd == SIDE_BUY) { r->b += k.q; s.B += k.q; } else if (sd == SIDE_SELL) { r->s += k.q; s.S += k.q; } else { r->u += k.q; s.U += k.q; }
        if (sd != SIDE_UNK) lastClass = std::max(lastClass, k.t);
        const bool validQuote = k.bid > 0 && k.ask > k.bid && (long long)k.bid + k.ask <= INT_MAX - 32LL;
        if (validQuote) {
            int m = k.bid + k.ask; haveMid = true; quoteT = k.t; spreadNow = k.ask - k.bid;
            s.spread = std::max(s.spread, spreadNow);
            if (s.mids.empty() || s.mids.back() != m) s.mids.push_back(m);
            mid2 = m;
        } else {
            s.quoteBroken = true; haveMid = false; quoteT = LLONG_MIN; spreadNow = -1;
        }
    }

    void closeUntil(long long t)
    {
        while (open && cur.t <= t) {
            // no trade for 5+ minutes (closed market, halt, feed gap): stop at the same second whether the clock is walked
            // one second at a time (live) or jumped (back-fill) - the next trade starts again with a fresh warm-up
            if (lastTradeT != LLONG_MIN && cur.t - lastTradeT > cfg.maxQuietGap) { gap(cur.t, "no trades for 5+ minutes"); stopped = true; return; }
            cur.path = cur.carried && !cur.quoteBroken && haveMid && (cur.t - quoteT) <= quoteAgeAt(cur.t) && !cur.mids.empty();
            if (!cur.path) cur.spread = -1;
            R.push_back(cur); retainedQty += cur.B + cur.S + cur.U;
            while ((int)R.size() > cfg.ctxW + cfg.obsW + 5) { retainedQty -= R.front().B + R.front().S + R.front().U; R.pop_front(); }
            evaluate(cur.t);
            long long nt = cur.t + 1;
            if (nt > t) { open = false; break; }
            if (lastTradeT != LLONG_MIN && nt - lastTradeT > cfg.maxQuietGap) { gap(nt, "no trades for 5+ minutes"); stopped = true; return; }
            if (sessionOf(nt) != curSid) {
                newSession(sessionOf(nt)); stopped = true; return; // wait for a real trade after a session halt
            }
            startAt(nt);
        }
    }

    // sums over the last w closed seconds
    void sums(int w, long long* B, long long* S, long long* U) const
    {
        *B = *S = *U = 0; int n = (int)R.size();
        for (int i = std::max(0, n - w); i < n; i++) { *B += R[(size_t)i].B; *S += R[(size_t)i].S; *U += R[(size_t)i].U; }
    }
    static void flow(long long B, long long S, long long U, bool* ok, double* F, double* L, double* H, double* C)
    {
        const double b = (double)B, s = (double)S, u = (double)U;
        *ok = B > 0 || S > 0;
        const double known = b + s, T = known + u;
        *F = *ok ? 100.0 * (b - s) / known : 0;
        *L = T > 0 ? 100.0 * (b - s - u) / T : 0; *H = T > 0 ? 100.0 * (b - s + u) / T : 0;
        *C = T > 0 ? known / T : 0;
    }

    Feat features(long long t)
    {
        Feat f; long long B, S, U;
        sums(cfg.fastW, &B, &S, &U); flow(B, S, U, &f.f30ok, &f.f30, &f.l30, &f.h30, &f.c30);
        sums(cfg.ctxW, &B, &S, &U); flow(B, S, U, &f.f180ok, &f.f180, &f.l180, &f.h180, &f.c180);
        double x1, x2, x3;
        sums(cfg.obsW, &B, &S, &U); flow(B, S, U, &f.f20ok, &x1, &x2, &x3, &f.c20);
        f.rate20 = ((double)B + (double)S + (double)U) / cfg.obsW;
        sums(cfg.part, &B, &S, &U); f.rate5 = ((double)B + (double)S + (double)U) / cfg.part;
        f.f30ok = f.f30ok && (int)R.size() >= cfg.fastW;
        f.f180ok = f.f180ok && (int)R.size() >= cfg.ctxW;
        f.f20ok = f.f20ok && (int)R.size() >= cfg.obsW;
        f.bin = binOf(t); f.bbv = base[f.bin];
        f.baseOk = f.bb()->n >= cfg.minWin && f.bb()->med20 > 0;
        if (f.bb()->med20 > 0) { f.A = f.rate20 / f.bb()->med20; f.aok = true; }
        f.quoteAgeEff = quoteAgeAt(t); f.gapTypical = typicalGap(t, &f.gapFromBase);
        f.sinceTrade = lastTradeT == LLONG_MIN ? -1 : t - lastTradeT;
        f.quoteOk = haveMid && (t - quoteT) <= f.quoteAgeEff && !R.empty() && R.back().path;
        f.quoteResumed = !f.quoteOk && haveMid && (t - quoteT) <= f.quoteAgeEff && !R.empty() && !R.back().quoteBroken && !R.back().carried;
        f.mid2 = haveMid ? mid2 : INT_MIN;
        f.spreadCal = f.bb()->medSpread >= 0;
        const int observedSpread = R.empty() ? -1 : R.back().spread;
        if (f.spreadCal && observedSpread >= 0) f.spreadOk = observedSpread <= std::max(2.0, 3.0 * f.bb()->medSpread);
        else f.spreadOk = observedSpread >= 0;
        f.classRecent = lastClass != LLONG_MIN && lastClass >= t - std::max<long long>(cfg.part - 1, (long long)f.quoteAgeEff - 1);   // (1.1.5) adaptive like the quote
        f.warm = warmN >= cfg.warm;
        // R: the midpoint range in ticks over (t-200, t-20]
        int n = (int)R.size(), lo = INT_MAX, hi = INT_MIN;
        f.rangeOk = n >= cfg.ctxW + cfg.obsW;
        for (int i = std::max(0, n - (cfg.ctxW + cfg.obsW)); i < n - cfg.obsW; i++) {
            const Slice& s = R[(size_t)i]; if (!s.path || s.mids.empty()) { f.rangeOk = false; continue; }
            lo = std::min(lo, s.mn()); hi = std::max(hi, s.mx());
        }
        f.R = hi >= lo ? (hi - lo) / 2 : 0;                                   // for the record (ticks, rounded down)
        double Rt = hi >= lo ? (hi - lo) / 2.0 : 0.0;                         // the half-tick range in ticks, unrounded
        f.h = std::max(cfg.hMin, std::min(cfg.hMax, (int)std::ceil(0.05 * Rt - 1e-9)));
        return f;
    }

    bool gatesOk(const Feat& f, std::string* why) const
    {
        if (!f.warm) { *why = "warming up"; return false; }
        if (!f.baseOk) { *why = "calibrating"; return false; }
        if (!f.quoteOk) { *why = "no current quote"; return false; }
        if (!f.spreadCal) { *why = "spread uncalibrated"; return false; }
        if (!f.spreadOk) { *why = "spread too wide"; return false; }
        if (!f.classRecent) { *why = "no classified trade in 5 s"; return false; }
        return true;
    }

    // the last 20 s: for a band, the attempted side's numbers
    struct Raw { bool pass = false; int lo = 0, hi = 0, h = 1; double q95 = 0, qzone = 0, qopp = 0, qside = 0, conc = 0, czone = 0, q4 = 0, prog = 0; int entry2 = 0, ext2 = 0; int parts = 0; std::string why; };

    // completed adverse test in the window after the entry: 3 whole seconds beyond the adverse boundary
    bool adverseTested(int dir, int lo, int hi, size_t fromIdx) const
    {
        int run = 0;
        for (size_t i = fromIdx; i < R.size(); i++) {
            const Slice& s = R[i];
            bool beyond = s.path && (dir > 0 ? s.mx() <= 2 * (lo - 1) : s.mn() >= 2 * (hi + 1));
            run = beyond ? run + 1 : 0;
            if (run >= cfg.holdS) return true;
        }
        return false;
    }

    Raw rawFor(int dir, const Feat& f, const Ep* fr)
    {
        Raw r; int sd = attemptedSide(dir);
        Feat qualified = f;
        if (fr) {
            qualified.baseOk = fr->baseN >= cfg.minWin && fr->med20 > 0;
            qualified.spreadCal = fr->medSpread >= 0;
            qualified.spreadOk = qualified.spreadCal && !R.empty() && R.back().spread > 0 && R.back().spread <= std::max(2.0, 3.0 * fr->medSpread);
        }
        std::string gateWhy;
        if (!gatesOk(qualified, &gateWhy)) { r.why = gateWhy; return r; }
        if (!fr && !f.rangeOk) { r.why = "incomplete context midpoint path"; return r; }
        size_t n = R.size(); if ((int)n < cfg.obsW) { r.why = "short history"; return r; }
        size_t w0 = n - (size_t)cfg.obsW;
        for (size_t i = w0; i < n; ++i) if (!R[i].path || R[i].mids.empty() || R[i].t != R[w0].t + (long long)(i - w0)) { r.why = "incomplete observation midpoint path"; return r; }
        Dense D; D.build(R, w0, n);
        if (!D.valid) { r.why = "price span exceeds 4000 ticks"; return r; }
        long long Bw, Sw, Uw; sums(cfg.obsW, &Bw, &Sw, &Uw);
        r.qside = (double)(sd == SIDE_BUY ? Bw : Sw);
        if (fr) { r.lo = fr->lo; r.hi = fr->hi; r.h = fr->h; r.q95 = fr->q95; }
        else {
            r.h = f.h;
            int c = 0; long long v = 0; double cv = 0;
            if (!D.best(sd, r.h, cfg.cov, f.mid2, &c, &v, &cv)) { r.why = "no band with 90% known side"; return r; }
            r.lo = c - r.h; r.hi = c + r.h;
            r.q95 = f.bb() ? (sd == SIDE_BUY ? f.bb()->qb[r.h - 1] : f.bb()->qs[r.h - 1]) : 0;
        }
        long long bB, bS, bU; D.band(r.lo, r.hi, &bB, &bS, &bU);
        r.qzone = (double)(sd == SIDE_BUY ? bB : bS); r.qopp = (double)(sd == SIDE_BUY ? bS : bB);
        r.czone = bB + bS + bU > 0 ? (double)(bB + bS) / (double)(bB + bS + bU) : 0;
        r.conc = r.qside > 0 ? r.qzone / r.qside : 0;
        // partitions
        double q[4] = {0, 0, 0, 0};
        for (int p = 0; p < 4; p++) for (int k = 0; k < cfg.part; k++) {
            const Slice& s = R[w0 + (size_t)(p * cfg.part + k)];
            for (const Row& x : s.rows) if (x.px >= r.lo && x.px <= r.hi) q[p] += (double)(sd == SIDE_BUY ? x.b : x.s);
        }
        r.q4 = q[3];
        for (int p = 0; p < 4; p++) if (r.q95 > 0 && q[p] >= r.q95 * cfg.partFrac) r.parts++;
        // entry + progress
        size_t entryIdx = n;
        if (fr) { r.entry2 = fr->entry2; r.ext2 = fr->ext2; entryIdx = w0; }
        else {
            bool got = false; int ext = 0;
            for (size_t i = w0; i < n && !got; i++) {
                const Slice& s = R[i]; if (!s.path) continue;
                for (size_t k = 0; k < s.mids.size(); k++) {
                    int m = s.mids[k];
                    if (!got && m >= 2 * r.lo && m <= 2 * r.hi) { got = true; r.entry2 = m; ext = m; entryIdx = i;
                        for (size_t j = k; j < s.mids.size(); j++) ext = dir > 0 ? std::min(ext, s.mids[j]) : std::max(ext, s.mids[j]); }
                }
            }
            if (!got) { r.why = "price never in the band"; return r; }
            for (size_t i = entryIdx + 1; i < n; i++) { const Slice& s = R[i]; if (!s.path) continue; ext = dir > 0 ? std::min(ext, s.mn()) : std::max(ext, s.mx()); }
            r.ext2 = ext;
        }
        r.prog = (dir > 0 ? (r.entry2 - r.ext2) : (r.ext2 - r.entry2)) / 2.0;
        // the predicate
        std::string why;
        if (!gatesOk(qualified, &why)) { r.why = why; return r; }
        if (f.c20 < cfg.cov) { r.why = "20-s sides known < 90%"; return r; }
        if (r.czone < cfg.cov) { r.why = "band sides known < 90%"; return r; }
        if (!(r.q95 > 0) || r.qzone < r.q95) { r.why = "band effort below its 95th pct"; return r; }
        if (r.conc < cfg.conc) { r.why = "effort not concentrated"; return r; }
        if (r.parts < 3) { r.why = "effort not sustained"; return r; }
        if (r.q4 < cfg.fresh * r.q95) { r.why = "no fresh effort"; return r; }
        if (r.prog > r.h) { r.why = "price progressed through"; return r; }
        if (!fr && adverseTested(dir, r.lo, r.hi, w0)) { r.why = "band already broken"; return r; }   // anywhere in the 20 s
        double med20 = fr ? fr->med20 : (f.bb() ? f.bb()->med20 : 0);
        if (!(med20 > 0 && f.rate20 >= med20)) { r.why = "activity below median"; return r; }
        r.pass = true; return r;
    }

    void push(const Ev& e)
    {
        Ev committed = e; if (!committed.knownAt) committed.knownAt = e.t + 1;
        evs.push_back(committed);
        if (evs.size() > 20000) {                                  // keep the last cfg.keepSec of events (time-ordered)
            long long cut = e.t - cfg.keepSec; size_t k = 0; while (k < evs.size() && evs[k].t < cut) k++;
            if (k) evs.erase(evs.begin(), evs.begin() + (long)k);
        }
    }
    Ev mk(const Ep& e, long long t, const char* kind, const std::string& why)
    {
        Ev v; v.t = t; v.ep = e.id; v.kind = kind; v.dir = e.dir; v.lo = e.lo; v.hi = e.hi; v.h = e.h; v.q95 = e.q95;
        v.f30 = last.f30ok ? last.f30 : NAN; v.f180 = last.f180ok ? last.f180 : NAN; v.a = last.aok ? last.A : NAN; v.cov = last.c30;
        v.why = why; v.ver = e.ver;
        return v;
    }
    void terminal(Ep& e, long long t, const char* kind, const std::string& why)
    {
        if (!e.state) return;
        bool wasWatch = e.state == 2;
        if (wasWatch || std::string(kind) == "CBRK") {
            Latch L; L.dir = e.dir; L.lo = e.lo; L.hi = e.hi; L.h = e.h; L.q95 = e.q95; latches.push_back(L);
        }
        push(mk(e, t, kind, why));
        e = Ep();
    }

    bool latched(int dir, int lo, int hi) const
    {
        for (const Latch& L : latches) if (L.dir == dir && !(hi < L.lo || lo > L.hi)) return true;
        return false;
    }
    void updateLatches(long long t)
    {
        (void)t;
        if (R.empty()) return;
        const Slice& s = R.back();
        size_t n = R.size();
        for (size_t i = 0; i < latches.size();) {
            Latch& L = latches[i]; bool rearm = false;
            if (s.path) {
                int distance = 2 * L.h + 1;
                if (s.mx() >= 2 * (L.hi + distance) || s.mn() <= 2 * (L.lo - distance)) L.movedAway = true;
                else if (L.movedAway) for (int m : s.mids) if (m >= 2 * L.lo && m <= 2 * L.hi) rearm = true;
            }
            // quiet reset: the attempted side's band volume over 20 s below 25% of the old Q95 for 30 s
            double qz = 0; int sd = attemptedSide(L.dir);
            for (size_t k = n > (size_t)cfg.obsW ? n - (size_t)cfg.obsW : 0; k < n; k++)
                for (const Row& x : R[k].rows) if (x.px >= L.lo && x.px <= L.hi) qz += (double)(sd == SIDE_BUY ? x.b : x.s);
            bool observed = n >= (size_t)cfg.obsW;
            for (size_t k = n > (size_t)cfg.obsW ? n - (size_t)cfg.obsW : 0; k < n; ++k) if (!R[k].path) observed = false;
            if (!s.path) L.movedAway = false;
            L.quietN = observed && qz < 0.25 * L.q95 ? L.quietN + 1 : 0;
            if (L.quietN >= cfg.quietReset) rearm = true;
            if (rearm) latches.erase(latches.begin() + (long)i); else i++;
        }
    }

    void windowStat(long long t, const Feat& f)
    {
        (void)f;
        long long x = (t + 1 - 17LL * 3600) % cfg.obsW; if (x < 0) x += cfg.obsW;
        if (x != 0 || (int)R.size() < cfg.obsW) return;
        size_t n = R.size(), w0 = n - (size_t)cfg.obsW;
        if (R[w0].t != t - cfg.obsW + 1) return;                     // not 20 contiguous seconds
        if (R[w0].t < (curSid - 1) * 86400 + 17LL * 3600) return;     // starts before this session
        // (1.1.5) no "continuous quote path" requirement any more (it dropped ~44% of GC's and ~90% of HG's RTH windows and kept
        // only the busy ones). A window counts unless a trade in it had a missing / locked / crossed quote or < 90% of its volume
        // has a known side. A window with no trade at all counts as rate 0.
        long long B = 0, S = 0, U = 0; int act = 0;
        for (size_t i = w0; i < n; i++) {
            if (R[i].quoteBroken) return;
            B += R[i].B; S += R[i].S; U += R[i].U;
            if (R[i].B + R[i].S + R[i].U > 0) ++act;
        }
        const double total = (double)B + (double)S + (double)U;
        const double known = (double)B + (double)S;
        if (total > 0 && known / total < cfg.cov) return;
        Win w; w.endT = t; w.bin = binOf(t); w.rate20 = (float)(total / cfg.obsW); w.spread = (float)(R.back().path ? R.back().spread : -1); w.act = (float)act;
        for (int p = 0; p < 4; p++) { long long v = 0; for (int k = 0; k < cfg.part; k++) { const Slice& s = R[w0 + (size_t)(p * cfg.part + k)]; v += s.B + s.S + s.U; } w.r5[p] = (float)((double)v / cfg.part); }
        if (total > 0) {
            Dense D; D.build(R, w0, n);
            if (!D.valid) return;
            for (int h = 1; h <= 8; h++) {
                int c; long long v; double cv;
                w.Mb[h - 1] = D.best(SIDE_BUY, h, cfg.cov, INT_MIN, &c, &v, &cv) ? (float)v : 0.f;
                w.Ms[h - 1] = D.best(SIDE_SELL, h, cfg.cov, INT_MIN, &c, &v, &cv) ? (float)v : 0.f;
            }
        }
        sessWins.push_back(w);
    }

    void evaluate(long long t)
    {
        // (1.1.0 integration) count observed-path seconds toward the warm-up, but a merely quiet second (no trade for 5+ s, so
        // no fresh quote) only PAUSES it - it does not start it over. Resetting on every quiet second meant thin markets never warmed
        // up: HG trades ~25 times a minute in RTH, so a 6-s pause inside any 200-s run is near certain and HG would never signal.
        // Real holes (feed gap, 5+ min with no trades, new session) still reset it in gap(); every window and episode still
        // requires its own continuous observed path.
        if (R.back().path) warmN = std::min(warmN + 1, 86400);
        else if (R.back().quoteBroken) warmN = 0;                     // a missing / crossed / locked quote still starts it over
        Feat f = features(t);
        last = f; lastT = t; haveLastT = true;
        windowStat(t, f);
        const Slice& s = R.back();
        if (!s.path) {
            for (int d = 0; d < 2; ++d) {
                if (ep[d].state) terminal(ep[d], t, "DINV", "invalid observed midpoint path");
                inS[d] = InSetup(); inQuiet[d] = 0;
            }
        }
        std::string gw; bool gates = gatesOk(f, &gw);

        // 1-2. expiry (data invalidation happens in gap())
        for (int d = 0; d < 2; d++) {
            Ep& e = ep[d];
            if (e.state == 2 && t >= e.fireT + cfg.watchTTL) terminal(e, t, "EXP", "90 s without a resolution");
            else if (e.state == 1 && t >= e.t0 + cfg.candTTL) { push(mk(e, t, "CEXP", "not qualified in 10 s")); e = Ep(); }
        }
        // 3. adverse boundary (candidates and watches)
        for (int d = 0; d < 2; d++) {
            Ep& e = ep[d]; if (!e.state) continue;
            int sd = attemptedSide(e.dir);
            bool beyond = s.path && (e.dir > 0 ? s.mx() <= 2 * (e.lo - 1) : s.mn() >= 2 * (e.hi + 1));
            if (beyond) {
                e.breachN++;
                for (const Row& x : s.rows) if (e.dir > 0 ? x.px <= e.lo - 1 : x.px >= e.hi + 1) e.breachQty += (double)(sd == SIDE_BUY ? x.b : x.s);
            } else { e.breachN = 0; e.breachQty = 0; }
            if (e.breachN >= cfg.holdS) {
                if (e.state == 1) terminal(e, t, "CBRK", "broken before the watch");
                else if (e.breachQty >= cfg.fresh * e.q95) terminal(e, t, "FAIL", "accepted through with execution");
                else terminal(e, t, "INVP", "price beyond without enough execution");
            }
        }
        // watch diagnostics + AR
        bool arFired[2] = {false, false};
        for (int d = 0; d < 2; d++) {
            Ep& e = ep[d]; if (e.state != 2) continue;
            int sd = attemptedSide(e.dir), rs = sd == SIDE_BUY ? SIDE_SELL : SIDE_BUY;
            if (!e.faded) {
                double q4 = 0; size_t n = R.size();
                for (size_t k = n > (size_t)cfg.part ? n - (size_t)cfg.part : 0; k < n; k++)
                    for (const Row& x : R[k].rows) if (x.px >= e.lo && x.px <= e.hi) q4 += (double)(sd == SIDE_BUY ? x.b : x.s);
                if (q4 < cfg.fresh * e.q95) { e.faded = true; push(mk(e, t, "FADE", "attack fading")); }
            }
            bool price = s.path && (e.dir > 0 ? s.mn() >= 2 * (e.hi + 1) : s.mx() <= 2 * (e.lo - 1));
            bool flowOk = f.f30ok && f.c30 >= cfg.cov && (e.dir > 0 ? (f.f30 >= 0 && f.l30 >= 0 && f.f30 - e.f30aw >= cfg.arImprove)
                                                                    : (f.f30 <= 0 && f.h30 <= 0 && e.f30aw - f.f30 >= cfg.arImprove));
            bool act = f.rate5 >= e.med5 && e.med5 > 0;
            bool qual = f.quoteOk && e.medSpread >= 0 && s.spread > 0 && s.spread <= std::max(2.0, 3.0 * e.medSpread) && f.warm && f.classRecent;
            if (price && flowOk && act && qual) {
                e.arN++;
                for (const Row& x : s.rows) if (e.dir > 0 ? x.px >= e.hi + 1 : x.px <= e.lo - 1) e.arQty += (double)(rs == SIDE_BUY ? x.b : x.s);
                e.arKnown += (double)(s.B + s.S); e.arAll += (double)(s.B + s.S + s.U);
                double fin = 0; for (const Row& x : s.rows) if (e.dir > 0 ? x.px >= e.hi + 1 : x.px <= e.lo - 1) fin += (double)(rs == SIDE_BUY ? x.b : x.s);
                if (e.arN >= cfg.holdS && e.arQty >= cfg.fresh * e.q95 && fin > 0 && e.arAll > 0 && e.arKnown / e.arAll >= cfg.cov) {
                    Ev v = mk(e, t, "AR", "absorption response");
                    v.ctx = !f.f180ok || f.c180 < cfg.cov || f.f180 == 0 ? "unknown" : ((f.f180 > 0) == (e.dir > 0) ? "aligned" : "opposing");
                    e.state = 0;                                    // terminal: latch it
                    Latch L; L.dir = e.dir; L.lo = e.lo; L.hi = e.hi; L.h = e.h; L.q95 = e.q95; latches.push_back(L);
                    push(v); e = Ep(); arFired[d] = true;
                }
            } else { e.arN = 0; e.arQty = 0; e.arKnown = 0; e.arAll = 0; }
        }
        // 5. initiative
        for (int d = 0; d < 2; d++) {
            int dir = d == 0 ? 1 : -1;
            InSetup& I = inS[d];
            double bound = dir > 0 ? f.l30 : f.h30;
            if (inLatched[d]) {
                inQuiet[d] = f.quoteOk && f.f30ok && f.c30 >= cfg.cov && f.l30 >= -cfg.neutral && f.h30 <= cfg.neutral ? inQuiet[d] + 1 : 0;
                if (inQuiet[d] >= cfg.inRearm) { inLatched[d] = false; inQuiet[d] = 0; }
            }
            double m5 = I.on ? I.med5 : f.bb()->med5, m20 = I.on ? I.med20 : f.bb()->med20;   // frozen once the setup exists
            Feat qualified = f;
            if (I.on) {
                qualified.baseOk = I.baseN >= cfg.minWin && I.med20 > 0;
                qualified.spreadCal = I.medSpread >= 0;
                qualified.spreadOk = qualified.spreadCal && s.spread > 0 && s.spread <= std::max(2.0, 3.0 * I.medSpread);
            }
            std::string inWhy;
            bool P = gatesOk(qualified, &inWhy) && f.f30ok && f.f180ok && f.c30 >= cfg.cov && f.c180 >= cfg.cov && m20 > 0 && f.rate20 / m20 >= cfg.inAct &&
                     (dir > 0 ? (f.l30 >= cfg.inFast && f.l180 > 0) : (f.h30 <= -cfg.inFast && f.h180 < 0)) && m5 > 0 && f.rate5 >= m5;
            (void)bound;
            if (inLatched[d]) { I = InSetup(); continue; }
            if (!P) { I = InSetup(); continue; }
            if (!I.on) {
                // the preceding 20 s extreme (before this second)
                int ext = dir > 0 ? INT_MIN : INT_MAX; size_t n = R.size();
                if (n < (size_t)cfg.obsW + 1) continue;
                bool continuous = true;
                for (size_t k = n > (size_t)cfg.obsW + 1 ? n - (size_t)cfg.obsW - 1 : 0; k + 1 < n; k++) {
                    const Slice& q = R[k]; if (!q.path || q.mids.empty()) { continuous = false; break; } ext = dir > 0 ? std::max(ext, q.mx()) : std::min(ext, q.mn());
                }
                if (!continuous || ext == INT_MIN || ext == INT_MAX) continue;
                I.on = true; I.t0 = t; I.bound2 = dir > 0 ? ext + 2 : ext - 2; I.med5 = f.bb()->med5; I.med20 = f.bb()->med20; I.medSpread = f.bb()->medSpread; I.baseN = f.bb()->n; I.n = 0; I.qty = I.known = I.all = 0;
                continue;                                           // the setup second itself is not part of the hold
            }
            if (t >= I.t0 + cfg.inTTL) { I = InSetup(); continue; }
            bool price = s.path && (dir > 0 ? s.mn() >= I.bound2 : s.mx() <= I.bound2);
            if (!price) { I.n = 0; I.qty = I.known = I.all = 0; continue; }
            I.n++;
            double fin = 0;
            for (const Row& x : s.rows) if (dir > 0 ? 2 * x.px >= I.bound2 : 2 * x.px <= I.bound2) fin += (double)(dir > 0 ? x.b : x.s);
            I.qty += fin; I.known += (double)(s.B + s.S); I.all += (double)(s.B + s.S + s.U);
            if (I.n >= cfg.holdS && fin > 0 && I.all > 0 && I.known / I.all >= cfg.cov) {
                Ev v; v.t = t; v.ep = epId(t, dir, true); v.kind = arFired[d] ? "INSUP" : "IN"; v.dir = dir;
                v.lo = v.hi = (dir > 0 ? (I.bound2 + 1) / 2 : I.bound2 / 2); v.f30 = f.f30; v.f180 = f.f180; v.a = f.A; v.cov = f.c30;
                v.ctx = "aligned"; v.why = arFired[d] ? "AR fired this second (AR wins)" : "initiative"; v.ver = ver;
                push(v); I = InSetup(); inLatched[d] = true; inQuiet[d] = 0;
                inLatched[1 - d] = false; inS[1 - d] = InSetup();   // an opposite initiative rearms the other side
            }
        }
        // latches
        updateLatches(t);
        // 4 (hold) + 6. candidates: hold the existing one, else look for new ones
        Raw rw[2]; bool tried[2] = {false, false};
        for (int d = 0; d < 2; d++) {
            Ep& e = ep[d]; if (e.state != 1) continue;
            Raw r = rawFor(e.dir, f, &e);
            int sd = attemptedSide(e.dir);
            // keep the frozen progress up to date
            if (s.path) e.ext2 = e.dir > 0 ? std::min(e.ext2, s.mn()) : std::max(e.ext2, s.mx());
            r.ext2 = e.ext2; r.prog = (e.dir > 0 ? (e.entry2 - e.ext2) : (e.ext2 - e.entry2)) / 2.0;
            if (r.pass && r.prog > e.h) { r.pass = false; r.why = "price progressed through"; }
            double freshIn = 0; for (const Row& x : s.rows) if (x.px >= e.lo && x.px <= e.hi) freshIn += (double)(sd == SIDE_BUY ? x.b : x.s);
            if (r.pass) { e.holdN++; if (freshIn > 0) e.holdFresh = true; }
            else { e.holdN = 0; e.holdFresh = false; }
            if (e.holdN >= cfg.holdS && e.holdFresh) {
                e.state = 2; e.fireT = t; e.f30aw = f.f30ok ? f.f30 : 0; e.covAW = r.czone;
                Ev v = mk(e, t, "AW", r.qopp >= 0.5 * r.qzone ? "absorption watch (two-way)" : "absorption watch");
                v.qzone = r.qzone; v.conc = r.conc; v.prog = r.prog; push(v);
            }
        }
        for (int d = 0; d < 2; d++) {
            int dir = d == 0 ? 1 : -1;
            if (ep[d].state) continue;
            rw[d] = rawFor(dir, f, nullptr); tried[d] = true;
            if (rw[d].pass && latched(dir, rw[d].lo, rw[d].hi)) { rw[d].pass = false; rw[d].why = "same zone already used (latched)"; }
        }
        bool overlapBoth = tried[0] && tried[1] && rw[0].pass && rw[1].pass && !(rw[0].hi < rw[1].lo || rw[0].lo > rw[1].hi);
        if (overlapBoth) {
            if (!balance) { Ev v; v.t = t; v.kind = "BAL"; v.dir = 0; v.lo = std::min(rw[0].lo, rw[1].lo); v.hi = std::max(rw[0].hi, rw[1].hi); v.why = "two-sided balance"; v.ver = ver; push(v); }
            balance = true;
        } else {
            balance = false;
            for (int d = 0; d < 2; d++) {
                if (!tried[d] || !rw[d].pass) continue;
                int o = 1 - d;
                if (ep[o].state && !(rw[d].hi < ep[o].lo || rw[d].lo > ep[o].hi)) {
                    if (ep[o].state == 1) { push(mk(ep[o], t, "BAL", "two-sided balance")); ep[o] = Ep(); }
                    continue;                                       // a committed opposite watch keeps the zone
                }
                Ep& e = ep[d]; e = Ep();
                e.id = epId(t, d == 0 ? 1 : -1, false); e.dir = d == 0 ? 1 : -1; e.state = 1; e.lo = rw[d].lo; e.hi = rw[d].hi; e.h = rw[d].h; e.q95 = rw[d].q95;
                e.med20 = f.bb()->med20; e.med5 = f.bb()->med5; e.medSpread = f.bb()->medSpread; e.baseN = f.bb()->n; e.entry2 = rw[d].entry2; e.ext2 = rw[d].ext2; e.t0 = t; e.ver = ver;
                for (size_t k = R.size(); k-- > 0;) {                     // a breach already under way keeps its age
                    const Slice& q = R[k];
                    bool beyond = q.path && (e.dir > 0 ? q.mx() <= 2 * (e.lo - 1) : q.mn() >= 2 * (e.hi + 1));
                    if (!beyond) break;
                    e.breachN++;
                    for (const Row& x : q.rows) if (e.dir > 0 ? x.px <= e.lo - 1 : x.px >= e.hi + 1) e.breachQty += (double)(attemptedSide(e.dir) == SIDE_BUY ? x.b : x.s);
                }
                Ev v = mk(e, t, "CAND", "candidate"); v.qzone = rw[d].qzone; v.conc = rw[d].conc; v.prog = rw[d].prog; push(v);
            }
        }
        lastWhy = gates ? (tried[0] ? rw[0].why : std::string()) : gw;
        // the per-second record
        SecRec q; q.t = t; q.b = R.back().B; q.s = R.back().S; q.u = R.back().U;
        if (f.f30ok) q.f30 = (float)f.f30;
        if (f.f180ok) q.f180 = (float)f.f180;
        if (f.aok) q.a = (float)f.A;
        q.c30 = (float)f.c30; q.l30 = (float)f.l30; q.h30 = (float)f.h30;
        bool lowAct = f.aok && f.A < 0.5;
        q.flags = (unsigned char)((f.warm ? SR_WARM : 0) | (f.baseOk ? SR_BASE : 0) | (lowAct ? SR_LOWACT : 0) |
                                  (f.c30 >= cfg.cov ? SR_SIDES : 0) | (f.warm && f.baseOk && f.c30 >= cfg.cov && f.quoteOk && f.spreadCal && f.spreadOk && f.classRecent && !lowAct ? SR_QUALITY : 0));
        hist.push_back(q);
        if (hist.size() > (size_t)cfg.keepSec + 3600) hist.erase(hist.begin(), hist.begin() + 3600);
    }
};

// ---- (1.1.5) the pane's state, as data: the wrapper turns it into the header title, the one-line pane readout and the status
// file; the replay tests use the same function. Order = what blocks a read first.
struct StateInfo {
    std::string code;                 // WAIT NOTRADES ERROR QUIET NOQUOTE LOWSIDES WIDESPREAD NOSIDES WATCH SIGNAL WARM CAL LOWACT READY
    int n = 0, need = 0;              // CAL: windows behind this slot / needed; WARM: seconds warmed / needed
    int k = 0;                        // CAL / READY: neighbouring slots borrowed each side
    double act = NAN, sides = NAN;    // activity (x normal), side coverage
    int dir = 0; long left = 0;       // WATCH / SIGNAL: +1 bullish, -1 bearish; WATCH seconds left
    std::string kind;                 // SIGNAL: AR / IN
    long long sigKnownAt = 0;         // SIGNAL: when it was known
    long long sinceTrade = -1; int quietAfter = 5;   // QUIET: seconds since the last trade, and the allowance
    int zoneLo = 0, zoneHi = 0;       // WATCH zone (ticks)
};
inline StateInfo classify(const Engine& e, bool haveSymbol, bool hasError)
{
    StateInfo s; const Feat& f = e.last;
    s.act = f.aok ? f.A : NAN; s.sides = f.c30; s.k = f.bb()->k;
    s.quietAfter = (int)f.quoteAgeEff;
    s.sinceTrade = f.sinceTrade < 0 ? -1 : f.sinceTrade;   // as of the evaluated second (the same moment as every other number)
    if (!haveSymbol) { s.code = "WAIT"; return s; }
    if (e.ticks <= 0) { s.code = "NOTRADES"; return s; }
    if (hasError) { s.code = "ERROR"; return s; }
    if (s.sinceTrade > (long long)f.quoteAgeEff) { s.code = "QUIET"; return s; }
    const bool quoteShown = f.quoteOk || f.quoteResumed;   // the first trade after a lull: hold the read, it is not bad data
    if (!quoteShown && f.warm) { s.code = "NOQUOTE"; return s; }
    if (f.warm && f.c30 < e.cfg.cov) { s.code = "LOWSIDES"; return s; }
    if (!f.spreadOk) { s.code = "WIDESPREAD"; return s; }
    if (!f.classRecent && f.warm) { s.code = "NOSIDES"; return s; }
    int d = 0; const Ep* w = nullptr;
    if (e.watching(&d, &w) && w) {
        s.code = "WATCH"; s.dir = d; s.zoneLo = w->lo; s.zoneHi = w->hi;
        s.left = (long)std::max(0LL, w->fireT + e.cfg.watchTTL - e.lastT); return s;
    }
    for (auto it = e.evs.rbegin(); it != e.evs.rend(); ++it) {
        if (e.lastT - it->t > 60) break;
        if (it->kind == "AR" || it->kind == "IN") { s.code = "SIGNAL"; s.dir = it->dir; s.kind = it->kind; s.sigKnownAt = it->knownAt; return s; }
    }
    if (!f.warm) { s.code = "WARM"; s.n = e.warmN; s.need = e.cfg.warm; return s; }
    if (f.f180ok && f.c180 < 0.5) { s.code = "LOWSIDES"; s.sides = f.c180; return s; }
    if (!f.baseOk) { s.code = "CAL"; s.n = f.bb()->n; s.need = e.cfg.minWin; return s; }
    if (!quoteShown) { s.code = "NOQUOTE"; return s; }
    if (f.aok && f.A < 0.5) { s.code = "LOWACT"; return s; }
    s.code = "READY"; return s;
}
// the short title for IRT's grey header line: "TF 1.1.5 GC  READY  act 1.2x" / "TF 1.1.5 GC  CAL 51/60" / "TF 1.1.5 GC  QUIET"
inline std::string shortTitle(const std::string& version, const std::string& market, const StateInfo& s, bool proven = false)
{
    std::string w;
    char b[64];
    const std::string q = proven ? "" : "?";
    if (s.code == "READY" || s.code == "LOWACT") {
        w = s.code == "READY" ? "READY" : "LOW ACT";
        if (std::isfinite(s.act)) { snprintf(b, sizeof(b), "  act %.1fx", s.act); w += b; }
    }
    else if (s.code == "CAL") { snprintf(b, sizeof(b), "CAL %d/%d", s.n, s.need); w = b; }
    else if (s.code == "WARM") { snprintf(b, sizeof(b), "WARM %d/%d", s.n, s.need); w = b; }
    else if (s.code == "WATCH") { snprintf(b, sizeof(b), "WATCH%s %lds", s.dir > 0 ? "+" : "-", s.left); w = b; }
    else if (s.code == "SIGNAL") w = s.kind + (s.dir > 0 ? "+" : "-") + q;
    else if (s.code == "QUIET") w = "QUIET";
    else if (s.code == "NOQUOTE") w = "NO QUOTE";
    else if (s.code == "LOWSIDES") w = "LOW SIDES";
    else if (s.code == "WIDESPREAD") w = "WIDE SPREAD";
    else if (s.code == "NOSIDES") w = "NO SIDES";
    else if (s.code == "NOTRADES") w = "NO TRADES";
    else if (s.code == "ERROR") w = "DATA ISSUE";
    else w = "WAIT";
    return "TF " + version + " " + market + "  " + w;
}
// (1.1.5) the pane's own vertical range: symmetric, max(60, 1.15 x the biggest |value| shown), rounded up to 10
inline double fitRange(double maxAbs)
{
    if (!std::isfinite(maxAbs) || maxAbs < 0) maxAbs = 0;
    double r = std::ceil(1.15 * maxAbs / 10.0 - 1e-9) * 10.0;
    return std::max(60.0, r);
}

// ---- W2|session|symbol|endT|bin|rate20|r5a..d|spread|Mb1..8|Ms1..8
// W legacy records are accepted with endT=LLONG_MIN but cannot calibrate.
inline std::string storeLine(long long sid, const std::string& sym, const Win& w)
{
    if (sym.empty() || sym.find_first_of("|\r\n") != std::string::npos) return std::string();
    std::ostringstream os; os.imbue(std::locale::classic());
    os << std::setprecision(std::numeric_limits<float>::max_digits10);
    if (w.endT == LLONG_MIN) os << "W|" << sid << '|' << sym;
    else os << "W2|" << sid << '|' << sym << '|' << w.endT;
    os << '|' << w.bin << '|' << w.rate20;
    for (int k = 0; k < 4; ++k) os << '|' << w.r5[k];
    os << '|' << w.spread;
    for (int h = 0; h < 8; ++h) os << '|' << w.Mb[h];
    for (int h = 0; h < 8; ++h) os << '|' << w.Ms[h];
    if (w.endT != LLONG_MIN && w.act >= 0) os << '|' << w.act;          // (1.1.5) appended: readers that index fields 0-26 are unaffected
    return os.str();
}
inline bool parseStoreLong(const std::string& text, long long* out)
{
    if (!out || text.empty()) return false;
    char* end = nullptr; errno = 0;
    long long value = std::strtoll(text.c_str(), &end, 10);
    if (errno == ERANGE || end == text.c_str() || *end != '\0') return false;
    *out = value; return true;
}
inline bool parseStoreInt(const std::string& text, int* out)
{
    long long value = 0;
    if (!out || !parseStoreLong(text, &value) || value < (long long)INT_MIN || value > (long long)INT_MAX) return false;
    *out = (int)value; return true;
}
inline bool parseStoreFloat(const std::string& text, float* out)
{
    if (!out || text.empty()) return false;
    char* end = nullptr; errno = 0;
    float value = std::strtof(text.c_str(), &end);
    // max_digits10 may round the decimal just above exact FLT_MAX; strtof rounds correctly.
    // Permit representable subnormals (ERANGE is allowed there), never overflow/underflow-to-zero.
    if (end == text.c_str() || *end != '\0' || !std::isfinite(value) || (errno == ERANGE && value == 0)) return false;
    *out = value; return true;
}
inline bool parseStoreLine(const std::string& ln, long long* sid, std::string* sym, Win* w)
{
    if (!sid || !sym || !w) return false;
    bool modern = ln.compare(0, 3, "W2|") == 0;
    if (!modern && ln.compare(0, 2, "W|") != 0) return false;
    std::vector<std::string> c; size_t a = modern ? 3 : 2;
    while (true) { size_t p = ln.find('|', a); c.push_back(ln.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; }
    const bool withAct = modern && c.size() == 27U;                       // (1.1.5) W2 + traded seconds
    if (c.size() != (modern ? 26U : 25U) && !withAct) return false;
    if (c[1].empty() || c[1].find_first_of("\r\n") != std::string::npos) return false;
    long long session; Win result;
    if (!parseStoreLong(c[0], &session)) return false;
    if (modern) {
        if (!parseStoreLong(c[2], &result.endT) || result.endT < LLONG_MIN + 1000000LL || result.endT > LLONG_MAX - 1000000LL) return false;
        c.erase(c.begin() + 2);
    }
    if (!parseStoreInt(c[2], &result.bin) || result.bin < 0 || result.bin >= NBINS ||
        !parseStoreFloat(c[3], &result.rate20) || result.rate20 < 0) return false;
    for (int k = 0; k < 4; ++k) if (!parseStoreFloat(c[4 + (size_t)k], &result.r5[k]) || result.r5[k] < 0) return false;
    if (!parseStoreFloat(c[8], &result.spread) || result.spread < -1) return false;
    for (int h = 0; h < 8; ++h) {
        if (!parseStoreFloat(c[9 + (size_t)h], &result.Mb[h]) || !parseStoreFloat(c[17 + (size_t)h], &result.Ms[h]) || result.Mb[h] < 0 || result.Ms[h] < 0) return false;
    }
    if (withAct && (!parseStoreFloat(c[25], &result.act) || result.act < 0 || result.act > 20)) return false;
    if (modern && (!identified(result) || sessionOf(result.endT) != session || (result.endT + 1 - 17LL * 3600) % 20 != 0)) return false;
    *sid = session; *sym = c[1]; *w = result; return true; // transactional parse
}

// (1.1.5) a session id is the trading day's date (days since 1970); Monday..Friday trade, Saturday / Sunday never do
inline bool tradingWeekday(long long sid) { long long w = ((sid % 7) + 7 + 4) % 7; return w >= 1 && w <= 5; }   // 1970-01-01 = Thursday
// (1.1.5) a stored session is COMPLETE when its front contract has >= minWins windows (a 23-h session has 4,140; the 1.1.4
// files kept only the busy windows, ~800-1,300 for GC and < 100 for HG, so they count as incomplete and get re-measured)
inline bool completeSession(const Store& st, long long sid, size_t minWins)
{
    auto it = st.find(sid); if (it == st.end()) return false;
    const std::vector<Win>* f = frontOf(it->second);
    return f && f->size() >= minWins;
}
inline int completeSessions(const Store& st, long long sid, size_t minWins)
{
    int n = 0; for (auto& kv : st) if (kv.first < sid && completeSession(st, kv.first, minWins)) n++;
    return n;
}
// (1.1.5) the oldest session of the unbroken run of complete weekday sessions just before sid (LLONG_MIN = none): a deep
// back-fill only has to replay the trades BEFORE that run (holidays break the run - they are just replayed again)
inline long long completeRunStart(const Store& st, long long sid, size_t minWins)
{
    long long first = LLONG_MIN;
    for (long long s = sid - 1; s > sid - 60; --s) {
        if (!tradingWeekday(s)) continue;
        if (!completeSession(st, s, minWins)) break;
        first = s;
    }
    return first;
}

// how many prior sessions (FRONT-ROLL by default; configurable same contract) the store holds before session sid
inline int priorSessions(const Store& st, long long sid)
{
    int n = 0; for (auto& kv : st) if (kv.first < sid && frontOf(kv.second)) n++;
    return n;
}


// =====================================================================================================================
//  (2.0.0) THE SIGNALS - P (push), E (exhaustion), A (absorption), F (flip). Rassul 2026-10-09: "implement proposed tapeflow.
//  add an up arrow head or down arrowhead to the signal ... use 1 letter to identify a signal but you can keep the ? until it is
//  confirmed"; "once a signal is given it should not change".
//
//  Decided ONLY on CLOSED chart bars (never the forming bar), in bar-time order, from: the bar's open / high / low / close (the
//  chart's own candle), the closed seconds of that bar (SecRec: 30-s and 180-s pressure, activity vs this time of day, aggressive
//  buy / sell volume) and the engine's AW / IN events in it. At most ONE new signal per bar; priority F > A > E > P.
//  dir +1 = bullish (green, UP arrowhead, bottom of the pane), -1 = bearish (red, DOWN arrowhead, top of the pane).
//
//  P  PUSH        one side hits the tape hard and price goes with it. Bar i, side d:
//                   [>= 20 s of the bar with d x 30-s pressure >= 35 (2:1 aggressive), activity >= 1.5x this 5-min slot's normal
//                    and >= 90% of the volume with a known side]  OR  [the engine's initiative IN d fired in the bar]
//                   AND the bar closes in that direction (green for buyers, red for sellers) AND beyond the previous bar's close
//                   AND no P the same way in the 3 bars before (one mark per push, not one per bar of a trend).
//                 Decided at the close -> drawn as P (no '?').
//  E  EXHAUSTION  a push runs out at a new 60-minute extreme. Push side p (+1 buyers):
//                   the bars just before (>= 2 in a row) had p-side aggressive volume > the other side (the run);
//                   the run made the 60-min extreme (its high >= every high of the 20 bars before this one);
//                   this bar's p-side aggressive volume < 50% of the run's average AND it does not extend the run's extreme.
//                 -> E? (dir -p) on this bar, anchored at the run's extreme. Confirmed (E) by a LATER bar closing against the push
//                   (a red bar after a buy push, a green bar after a sell push). Ends unconfirmed if a bar CLOSES beyond the anchor
//                   or after 5 bars.
//  A  ABSORPTION  the engine's absorption watch (AW: heavy aggressive volume >= its 95th pct in a 2h+1-tick band with no price
//                   progress) fired in the bar -> A? at the zone. dir +1 = sellers absorbed, -1 = buyers absorbed. Confirmed (A) by a
//                   bar (this one or later) closing AWAY against the aggressor: buyers absorbed -> a RED bar closing below the zone;
//                   sellers absorbed -> a GREEN bar closing above it (the Delta Profile 2.5.2 rule). Ends unconfirmed when a bar
//                   closes beyond the zone on the aggressor's side, or after 10 bars.
//  F  FLIP        control switches sides within 4 bars (12 min) after an A / A? / E / E? of direction d: the 180-s pressure was
//                   clearly against d (d x F180 <= -20) at some second since that signal's bar began, is clearly with d at this
//                   bar's close (d x F180 >= +20) and this bar closes in direction d. Decided at the close -> F. One F per origin.
//
//  NO REPAINT: a decided bar is never decided again (SignalBook.decidedThrough, persisted); a confirmed signal never changes; a '?'
//  only ever loses its '?' (confirmation) - one that never confirms stays drawn exactly as it was (state X in the record), never
//  removed. Every decision is a journal line (TapeFlow\<MKT>-signals-<session>.csv): a restart loads the file and redraws exactly
//  what was shown; bars already decided only rebuild the look-back (feed() with b.te <= decidedThrough decides nothing).
// =====================================================================================================================
struct SigCfg {
    double pushF = 35, pushAct = 1.5, sidesMin = 0.9; int pushSecs = 20, pushCool = 3;
    int runMin = 2; double collapse = 0.5; int extremeBars = 20;
    double flipF = 20; int flipBars = 4;
    int aExpire = 10, eExpire = 5;
    int minFlowSecs = 60;
};
struct BarIn { long long te = 0, ts = 0; int o = 0, h = 0, l = 0, c = 0; };   // ticks; te = the chart's bar stamp (its close)
struct BarFlow {                                       // the bar's tape over its closed seconds [ts, te)
    int secs = 0; long long buy = 0, sell = 0;
    int pushUp = 0, pushDn = 0;                        // seconds of the bar meeting the P pressure test
    bool f180ok = false; double f180end = 0, f180lo = 0, f180hi = 0;
    int inUp = 0, inDn = 0;                            // engine initiative events
    int awDir = 0, awLo = 0, awHi = 0;                 // the LAST engine absorption watch (AW) in the bar
    long long side(int d) const { return d > 0 ? buy : sell; }
};
// the bar's tape from the per-second records (sorted by t) and the engine's events (sorted by t)
template <class Recs>
inline BarFlow barFlow(const Recs& H, const std::vector<Ev>& evs, long long ts, long long te, const SigCfg& c)
{
    BarFlow f;
    auto it = std::lower_bound(H.begin(), H.end(), ts, [](const SecRec& r, long long t) { return r.t < t; });
    bool first = true;
    for (; it != H.end() && it->t < te; ++it) {
        const SecRec& r = *it;
        f.secs++; f.buy += r.b; f.sell += r.s;
        const bool sides = std::isfinite(r.c30) && r.c30 >= c.sidesMin;
        if (sides && std::isfinite(r.f30) && std::isfinite(r.a) && r.a >= c.pushAct) {
            if (r.f30 >= c.pushF) f.pushUp++;
            if (r.f30 <= -c.pushF) f.pushDn++;
        }
        if (std::isfinite(r.f180)) {
            if (first) { f.f180lo = f.f180hi = r.f180; first = false; }
            f.f180lo = std::min(f.f180lo, (double)r.f180); f.f180hi = std::max(f.f180hi, (double)r.f180);
            f.f180end = r.f180; f.f180ok = true;
        }
    }
    auto e = std::lower_bound(evs.begin(), evs.end(), ts, [](const Ev& v, long long t) { return v.t < t; });
    for (; e != evs.end() && e->t < te; ++e) {
        if (e->kind == "IN") { if (e->dir > 0) f.inUp++; else f.inDn++; }
        else if (e->kind == "AW") { f.awDir = e->dir > 0 ? 1 : -1; f.awLo = e->lo; f.awHi = e->hi; }
    }
    return f;
}

enum { MK_PENDING = 0, MK_CONFIRMED = 1, MK_EXPIRED = 2 };
struct Mark {
    long long barT = 0;                                // the bar it is drawn on (chart bar time = its close)
    char kind = 0;                                     // 'P' 'E' 'A' 'F'
    int dir = 0, state = MK_PENDING;
    int px = 0, lo = 0, hi = 0;                        // ticks: the signal's price (P / F close, E anchor, A zone middle), A zone
    long long confT = 0;                               // the bar that confirmed it
    long long ref = 0;                                 // F: the bar time of the A / E it flips from
    bool flipped = false;                              // A / E: an F already used it (not persisted - derived from F rows)
    std::string why;
    bool question() const { return state != MK_CONFIRMED; }   // '?' until confirmed (also when it never was)
};
inline int kindRank(char k) { return k == 'F' ? 4 : k == 'A' ? 3 : k == 'E' ? 2 : k == 'P' ? 1 : 0; }

class SignalBook {
public:
    SigCfg cfg;
    std::vector<Mark> marks;                           // in bar-time order
    long long decidedThrough = LLONG_MIN;              // every bar with te <= this is decided (persisted)
    long long lastFed = LLONG_MIN;
    std::vector<std::string> journal;                  // record lines not yet written to the file
    std::string version = "2.0.0";

    // one CLOSED bar, in time order. A bar at or before decidedThrough only extends the look-back (a restart).
    void feed(const BarIn& b, const BarFlow& f)
    {
        if (b.te <= lastFed) return;                   // each bar once
        lastFed = b.te;
        if (b.te <= decidedThrough) { remember(b, f); return; }
        // 1. what this bar does to the open '?' signals (decided forward only)
        for (Mark& m : marks) {
            if (m.state != MK_PENDING || m.barT > b.te) continue;
            const int age = barsSince(m.barT);          // bars after the signal's bar, before this one
            if (m.kind == 'A') {
                const bool conf = m.dir < 0 ? (b.c < b.o && b.c < m.lo) : (b.c > b.o && b.c > m.hi);
                const bool fail = m.dir < 0 ? b.c > m.hi : b.c < m.lo;
                if (conf) settle(m, MK_CONFIRMED, b.te, "a bar closed away from the zone");
                else if (fail) settle(m, MK_EXPIRED, b.te, "a bar closed through the zone");
                else if (age + 1 >= cfg.aExpire) settle(m, MK_EXPIRED, b.te, "no confirmation in 10 bars");
            } else if (m.kind == 'E' && b.te > m.barT) {
                const int p = -m.dir;                   // the push that ran out
                const bool fail = p > 0 ? b.c > m.px : b.c < m.px;
                const bool conf = m.dir * (b.c - b.o) > 0;
                if (fail) settle(m, MK_EXPIRED, b.te, "a bar closed beyond the extreme");
                else if (conf) settle(m, MK_CONFIRMED, b.te, "a bar closed against the push");
                else if (age + 1 >= cfg.eExpire) settle(m, MK_EXPIRED, b.te, "no confirmation in 5 bars");
            }
        }
        // 2. this bar's candidates; the strongest one is the bar's ONE signal
        Mark best; best.kind = 0;
        auto offer = [&](const Mark& m) { if (!best.kind || kindRank(m.kind) > kindRank(best.kind)) best = m; };
        const bool flow = f.secs >= cfg.minFlowSecs;
        const BarIn* prev = recent.empty() ? nullptr : &recent.back().first;
        // F
        for (int d = -1; d <= 1 && flow; d += 2) {
            if (!(d * (b.c - b.o) > 0) || !f.f180ok || d * f.f180end < cfg.flipF) continue;
            for (auto it = marks.rbegin(); it != marks.rend(); ++it) {
                Mark& o = *it;
                if (o.dir != d || (o.kind != 'A' && o.kind != 'E') || o.flipped || o.barT >= b.te) continue;
                const int age = barsSince(o.barT);
                if (age < 0 || age + 1 > cfg.flipBars) continue;
                double worst = d > 0 ? f.f180lo : -f.f180hi; bool any = f.f180ok;   // d x F180 at its lowest since the origin bar began
                for (const auto& rb : recent) if (rb.first.te >= o.barT && rb.second.f180ok) { worst = std::min(worst, d > 0 ? rb.second.f180lo : -rb.second.f180hi); any = true; }
                if (!any || worst > -cfg.flipF) continue;
                Mark m; m.barT = b.te; m.kind = 'F'; m.dir = d; m.state = MK_CONFIRMED; m.confT = b.te; m.px = b.c; m.ref = o.barT;
                char w[96]; snprintf(w, sizeof(w), "180-s pressure %+.0f -> %+.0f after %c", d > 0 ? worst : -worst, f.f180end, o.kind); m.why = w;
                offer(m); break;
            }
        }
        // A
        if (f.awDir) {
            Mark m; m.barT = b.te; m.kind = 'A'; m.dir = f.awDir; m.lo = f.awLo; m.hi = f.awHi; m.px = (f.awLo + f.awHi) / 2;
            m.why = f.awDir > 0 ? "sellers absorbed" : "buyers absorbed";
            offer(m);
        }
        // E
        for (int p = -1; p <= 1 && flow; p += 2) {
            int n = 0; double vol = 0; int ext = p > 0 ? INT_MIN : INT_MAX;
            for (auto it = recent.rbegin(); it != recent.rend(); ++it) {
                const BarFlow& rf = it->second;
                if (rf.secs < cfg.minFlowSecs || !(rf.side(p) > rf.side(-p))) break;
                n++; vol += (double)rf.side(p); ext = p > 0 ? std::max(ext, it->first.h) : std::min(ext, it->first.l);
            }
            if (n < cfg.runMin) continue;
            int lookExt = p > 0 ? INT_MIN : INT_MAX, k = 0;
            for (auto it = recent.rbegin(); it != recent.rend() && k < cfg.extremeBars; ++it, ++k) lookExt = p > 0 ? std::max(lookExt, it->first.h) : std::min(lookExt, it->first.l);
            if (p > 0 ? ext < lookExt : ext > lookExt) continue;                 // the run did not make the 60-min extreme
            const double avg = vol / n;
            if (!((double)f.side(p) < cfg.collapse * avg)) continue;            // aggression did not collapse
            if (p > 0 ? b.h > ext : b.l < ext) continue;                         // price extended
            bool dup = false;
            for (const Mark& o : marks) if (o.kind == 'E' && o.dir == -p && o.px == ext && o.state != MK_EXPIRED) dup = true;   // once per run extreme
            if (dup) continue;
            Mark m; m.barT = b.te; m.kind = 'E'; m.dir = -p; m.px = ext;
            char w[96]; snprintf(w, sizeof(w), "%s volume %.0f%% of the run's average at the 60-min %s", p > 0 ? "buy" : "sell", 100.0 * (double)f.side(p) / std::max(1.0, avg), p > 0 ? "high" : "low");
            m.why = w; offer(m);
        }
        // P
        for (int d = -1; d <= 1 && flow && prev; d += 2) {
            const bool hit = (d > 0 ? f.pushUp : f.pushDn) >= cfg.pushSecs || (d > 0 ? f.inUp : f.inDn) > 0;
            if (!hit || !(d * (b.c - b.o) > 0) || !(d * (b.c - prev->c) > 0)) continue;
            bool recentP = false;                                                // a push already marked in the last pushCool bars
            for (auto it = marks.rbegin(); it != marks.rend(); ++it) { const int age = barsSince(it->barT); if (age >= cfg.pushCool) break; if (it->kind == 'P' && it->dir == d) { recentP = true; break; } }
            if (recentP) continue;
            Mark m; m.barT = b.te; m.kind = 'P'; m.dir = d; m.state = MK_CONFIRMED; m.confT = b.te; m.px = b.c;
            char w[96]; snprintf(w, sizeof(w), "%d s of %s pressure%s", d > 0 ? f.pushUp : f.pushDn, d > 0 ? "buy" : "sell", (d > 0 ? f.inUp : f.inDn) ? " + initiative" : "");
            m.why = w; offer(m);
        }
        if (best.kind) {
            if (best.kind == 'F') for (Mark& o : marks) if (o.barT == best.ref && o.dir == best.dir && (o.kind == 'A' || o.kind == 'E')) o.flipped = true;
            marks.push_back(best);
            log(marks.back());
            Mark& m = marks.back();
            if (m.kind == 'A') {                                                   // the absorption bar itself may already close away
                const bool conf = m.dir < 0 ? (b.c < b.o && b.c < m.lo) : (b.c > b.o && b.c > m.hi);
                if (conf) settle(m, MK_CONFIRMED, b.te, "its own bar closed away from the zone");
            }
        }
        decidedThrough = b.te;
        char d[64]; snprintf(d, sizeof(d), "%lld|D", b.te); journal.push_back(std::string(d) + "|0|-|0|0|0|0|0||" + version);
        remember(b, f);
    }
    // the last signal of a kind set that is confirmed (for the header): nullptr = none
    const Mark* lastConfirmed() const { for (auto it = marks.rbegin(); it != marks.rend(); ++it) if (it->state == MK_CONFIRMED) return &*it; return nullptr; }
    const Mark* at(long long barT) const { for (const Mark& m : marks) if (m.barT == barT) return &m; return nullptr; }

    // ---- the record: t|kind|dir|state|px|lo|hi|confT|ref|why|version   (state ? C X; kind D = decided-through marker)
    static std::string line(const Mark& m, const std::string& ver)
    {
        std::ostringstream o; o.imbue(std::locale::classic());
        o << m.barT << '|' << m.kind << '|' << m.dir << '|' << (m.state == MK_CONFIRMED ? 'C' : m.state == MK_EXPIRED ? 'X' : '?') << '|'
          << m.px << '|' << m.lo << '|' << m.hi << '|' << m.confT << '|' << m.ref << '|';
        for (char ch : m.why) o << (ch == '|' || ch == '\r' || ch == '\n' ? ' ' : ch);
        o << '|' << ver;
        return o.str();
    }
    // a record line -> the book (later lines of the same signal update its state forward only; a confirmed one never goes back)
    bool load(const std::string& ln)
    {
        std::vector<std::string> c; size_t a = 0;
        while (true) { size_t p = ln.find('|', a); c.push_back(ln.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; }
        if (c.size() < 11 || c[0].empty() || !(c[0][0] >= '0' && c[0][0] <= '9')) return false;
        long long t = 0, ct = 0, rf = 0; int dir = 0, px = 0, lo = 0, hi = 0;
        if (!parseStoreLong(c[0], &t) || c[1].size() != 1) return false;
        if (c[1] == "D") { decidedThrough = std::max(decidedThrough, t); return true; }
        const char k = c[1][0];
        if (!kindRank(k) || !parseStoreInt(c[2], &dir) || (dir != 1 && dir != -1) || c[3].size() != 1 ||
            !parseStoreInt(c[4], &px) || !parseStoreInt(c[5], &lo) || !parseStoreInt(c[6], &hi) || !parseStoreLong(c[7], &ct) || !parseStoreLong(c[8], &rf)) return false;
        const int st = c[3] == "C" ? MK_CONFIRMED : c[3] == "X" ? MK_EXPIRED : c[3] == "?" ? MK_PENDING : -1;
        if (st < 0) return false;
        for (Mark& m : marks) if (m.barT == t && m.kind == k && m.dir == dir) {
            if (m.state == MK_PENDING && st != MK_PENDING) { m.state = st; m.confT = ct; }
            return true;
        }
        Mark m; m.barT = t; m.kind = k; m.dir = dir; m.state = st; m.px = px; m.lo = lo; m.hi = hi; m.confT = ct; m.ref = rf; m.why = c[9];
        auto pos = std::upper_bound(marks.begin(), marks.end(), t, [](long long x, const Mark& y) { return x < y.barT; });
        marks.insert(pos, m);
        if (k == 'F') for (Mark& o : marks) if (o.barT == rf && o.dir == dir && (o.kind == 'A' || o.kind == 'E')) o.flipped = true;
        return true;
    }
    void loadText(const std::string& text) { std::istringstream s(text); std::string ln; while (std::getline(s, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); load(ln); } relinkFlips(); }
    void relinkFlips() { for (const Mark& f : marks) if (f.kind == 'F') for (Mark& o : marks) if (o.barT == f.ref && o.dir == f.dir && (o.kind == 'A' || o.kind == 'E')) o.flipped = true; }
    static const char* header() { return "t|kind|dir|state|price_ticks|zone_lo|zone_hi|confirmed_bar|ref_bar|why|version"; }

private:
    std::deque<std::pair<BarIn, BarFlow> > recent;     // the look-back (last 30 closed bars)
    void remember(const BarIn& b, const BarFlow& f) { recent.push_back(std::make_pair(b, f)); while (recent.size() > 30) recent.pop_front(); }
    int barsSince(long long t) const { int n = 0; for (auto it = recent.rbegin(); it != recent.rend() && it->first.te > t; ++it) n++; return recent.empty() || recent.front().first.te > t ? 99 : n; }
    void settle(Mark& m, int st, long long when, const char* why) { m.state = st; m.confT = when; (void)why; log(m); }
    void log(const Mark& m) { journal.push_back(line(m, version)); }
};

// ---- the header line (IRT's grey title): "TF 2.0 ES  BUYERS 1.4x  last A 7,861.50"
inline std::string shortVersion(const std::string& v) { return v.size() > 2 && v.compare(v.size() - 2, 2, ".0") == 0 && std::count(v.begin(), v.end(), '.') == 2 ? v.substr(0, v.size() - 2) : v; }
inline const char* sideWord(bool ok, double f180) { return !ok ? "BALANCED" : f180 >= 15 ? "BUYERS" : f180 <= -15 ? "SELLERS" : "BALANCED"; }
// state code -> the header's middle word(s); READY-like codes show who is in control and how busy vs normal
inline std::string titleLine(const std::string& version, const std::string& market, const StateInfo& s, bool f180ok, double f180,
                             const Mark* last, const std::string& lastPx)
{
    std::string w; char b[64];
    const bool live = s.code == "READY" || s.code == "LOWACT" || s.code == "WATCH" || s.code == "SIGNAL";
    if (live) {
        w = sideWord(f180ok, f180);
        if (std::isfinite(s.act)) { snprintf(b, sizeof(b), " %.1fx", s.act); w += b; }
    }
    else if (s.code == "CAL") { snprintf(b, sizeof(b), "CAL %d/%d", s.n, s.need); w = b; }
    else if (s.code == "WARM") { snprintf(b, sizeof(b), "WARM %d/%d", s.n, s.need); w = b; }
    else if (s.code == "QUIET") w = "QUIET";
    else if (s.code == "NOQUOTE") w = "NO QUOTE";
    else if (s.code == "LOWSIDES") w = "LOW SIDES";
    else if (s.code == "WIDESPREAD") w = "WIDE SPREAD";
    else if (s.code == "NOSIDES") w = "NO SIDES";
    else if (s.code == "NOTRADES") w = "NO TRADES";
    else if (s.code == "ERROR") w = "DATA ISSUE";
    else w = "WAIT";
    std::string t = "TF " + shortVersion(version) + " " + market + "  " + w;
    if (last) t += std::string("  last ") + last->kind + " " + lastPx;
    return t;
}

// ---- the pane's layout: marks in two bands, never overlapping. Each candidate has its x and width; stronger (confirmed, then
// F > A > E > P, then later) are placed first, a weaker one that would touch a placed one in the same band is skipped.
struct Slot { int x = 0, w = 0, band = 0, rank = 0; long long t = 0; bool show = false; };
inline void layoutMarks(std::vector<Slot>& v, int gap = 2)
{
    std::vector<size_t> order(v.size()); for (size_t i = 0; i < v.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { if (v[a].rank != v[b].rank) return v[a].rank > v[b].rank; return v[a].t > v[b].t; });
    std::vector<std::pair<int, int> > placed[2];
    for (size_t i : order) {
        Slot& s = v[i]; s.show = false;
        const int band = s.band > 0 ? 1 : 0, l = s.x - s.w / 2, r = l + s.w;
        bool clash = false;
        for (auto& p : placed[band]) if (!(r + gap <= p.first || l >= p.second + gap)) { clash = true; break; }
        if (clash) continue;
        placed[band].push_back(std::make_pair(l, r)); s.show = true;
    }
}
inline int markRank(const Mark& m) { return kindRank(m.kind) + (m.state == MK_CONFIRMED ? 10 : 0); }
// the axis range IRT should show so that its scale matches the drawing's inner area (pane minus the two signal bands)
inline double axisRange(double r, int paneH, int band) { if (paneH <= 2 * band + 10) return r; return r * (paneH / 2.0) / (paneH / 2.0 - band); }

} // namespace tfl