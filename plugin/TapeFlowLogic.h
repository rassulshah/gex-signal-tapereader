/********************************************************************************
 *  TapeFlowLogic.h  --  the testable half of lsTapeFlow<MKT> (no IRT SDK)          v1.0.1 (2026-10-08)
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
 *    BASELINES  non-overlapping 20-s windows of up to 10 prior sessions (same contract), per 5-minute bin: the 95th percentile
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

namespace tfl {

#define TFL_VERSION "1.0.1"

struct Cfg {
    int fastW = 30, ctxW = 180, obsW = 20, part = 5;   // seconds
    int holdS = 3, candTTL = 10, watchTTL = 90, inTTL = 10;
    int minWin = 60, sessionsBack = 10;
    double conc = 0.50, partFrac = 1.0 / 8.0, fresh = 0.10, cov = 0.90;
    double arImprove = 15, inFast = 35, inAct = 1.2, neutral = 15;
    int hMin = 1, hMax = 8, warm = 200, quoteAge = 5, quietReset = 30, inRearm = 5;
    int maxQuietGap = 300;                             // no trade for longer = the tape stopped (closed market / feed gap)
    int keepSec = 3 * 86400;                           // per-second history kept for drawing
};

enum { SIDE_BUY = 0, SIDE_SELL = 1, SIDE_UNK = 2 };

struct Tick { long long t = 0; int px = 0; int bid = 0; int ask = 0; long long q = 0; };   // bid / ask <= 0 = no quote

inline int sideOf(int px, int bid, int ask)
{
    if (bid <= 0 || ask <= 0 || ask <= bid) return SIDE_UNK;      // no quote, crossed or locked: no side
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
    bool path = false;              // a valid quote known all through this second
    bool carried = false;           // a quote was known when the second began
    int spread = -1;                // ticks, the last quote of the second
    int mn() const { int m = INT_MAX; for (int v : mids) m = std::min(m, v); return m; }
    int mx() const { int m = INT_MIN; for (int v : mids) m = std::max(m, v); return m; }
    long long side(int sd) const { return sd == SIDE_BUY ? B : S; }
};

// one baseline window (stored per session)
struct Win { int bin = 0; float rate20 = 0, r5[4] = {0, 0, 0, 0}, spread = -1; float Mb[8] = {0}, Ms[8] = {0}; };
typedef std::map<std::string, std::vector<Win>> SessWins;      // contract -> windows (a roll day can hold two)
typedef std::map<long long, SessWins> Store;                    // session -> contracts -> windows
inline double avgRate(const std::vector<Win>& v) { double a = 0; for (const Win& w : v) a += w.rate20; return v.empty() ? 0 : a / (double)v.size(); }
// the session's FRONT contract = the one that traded the most per window (a back month's thin history never wins)
inline const std::vector<Win>* frontOf(const SessWins& s)
{
    const std::vector<Win>* b = nullptr; double bv = -1;
    for (auto& kv : s) { if (kv.second.empty()) continue; double v = avgRate(kv.second); if (v > bv) { bv = v; b = &kv.second; } }
    return b;
}

struct BinBase { int n = 0; float qb[8] = {0}, qs[8] = {0}; float med20 = 0, med5 = 0, medSpread = -1; };

inline float nearestRank(std::vector<float>& v, double p)
{
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    long k = (long)std::ceil(p * (double)v.size()) - 1; if (k < 0) k = 0; if (k >= (long)v.size()) k = (long)v.size() - 1;
    return v[(size_t)k];
}
inline float median(std::vector<float>& v)
{
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    size_t n = v.size(); return n % 2 ? v[n / 2] : 0.5f * (v[n / 2 - 1] + v[n / 2]);
}

// baselines for session `sid` from up to N prior sessions: each session's front contract (the chart follows the front month,
// so across a roll the old front's sessions are used until the new one has its own); a session trading under 30% of the
// typical per-window rate (a back month's history, a holiday) is left out.
inline void freezeBase(const Store& st, long long sid, int N, BinBase* out, int* usedSessions)
{
    std::vector<const std::vector<Win>*> cand;
    for (auto it = st.rbegin(); it != st.rend() && (int)cand.size() < 2 * N; ++it) {
        if (it->first >= sid) continue;
        const std::vector<Win>* f = frontOf(it->second); if (f) cand.push_back(f);
    }
    std::vector<float> rates; for (auto* f : cand) rates.push_back((float)avgRate(*f));
    float med = median(rates);
    std::vector<const std::vector<Win>*> ss;
    for (auto* f : cand) { if (avgRate(*f) < 0.3 * med) continue; ss.push_back(f); if ((int)ss.size() >= N) break; }
    if (usedSessions) *usedSessions = (int)ss.size();
    std::vector<std::vector<const Win*>> by(NBINS);
    for (auto* f : ss) for (const Win& w : *f) if (w.bin >= 0 && w.bin < NBINS) by[(size_t)w.bin].push_back(&w);
    for (int b = 0; b < NBINS; b++) {
        BinBase& o = out[b]; o = BinBase();
        const std::vector<const Win*>& v = by[(size_t)b];
        o.n = (int)v.size(); if (!o.n) continue;
        std::vector<float> a;
        for (int h = 0; h < 8; h++) {
            a.clear(); for (auto* w : v) a.push_back(w->Mb[h]); o.qb[h] = nearestRank(a, 0.95);
            a.clear(); for (auto* w : v) a.push_back(w->Ms[h]); o.qs[h] = nearestRank(a, 0.95);
        }
        a.clear(); for (auto* w : v) a.push_back(w->rate20); o.med20 = median(a);
        a.clear(); for (auto* w : v) for (int k = 0; k < 4; k++) a.push_back(w->r5[k]); o.med5 = median(a);
        a.clear(); for (auto* w : v) if (w->spread >= 0) a.push_back(w->spread); o.medSpread = a.empty() ? -1 : median(a);
    }
}

// the volume of each price over some slices, dense from lo to hi (tick prices)
struct Dense {
    int lo = 0, hi = -1; std::vector<long long> b, s, u;
    void build(const std::deque<Slice>& R, size_t from, size_t to)
    {
        lo = INT_MAX; hi = INT_MIN;
        for (size_t i = from; i < to; i++) for (const Row& r : R[i].rows) { lo = std::min(lo, r.px); hi = std::max(hi, r.px); }
        if (lo > hi) { hi = lo - 1; b.clear(); s.clear(); u.clear(); return; }
        if (hi - lo > 4000) lo = hi - 4000;
        size_t n = (size_t)(hi - lo + 1); b.assign(n, 0); s.assign(n, 0); u.assign(n, 0);
        for (size_t i = from; i < to; i++) for (const Row& r : R[i].rows) {
            if (r.px < lo) continue;
            size_t k = (size_t)(r.px - lo); b[k] += r.b; s[k] += r.s; u[k] += r.u;
        }
    }
    // the band [c-h, c+h] with the most known volume of `sd` among bands with >= cov known side; false = none eligible.
    // Ties (mirror-symmetric): nearest the single price with the most `sd` volume, then nearest the midpoint (half-ticks),
    // then the lower band for sellers / the higher band for buyers.
    bool best(int sd, int h, double cov, int mid2, int* center, long long* vol, double* bandCov) const
    {
        if (hi < lo) return false;
        int n = hi - lo + 1;
        const std::vector<long long>& V = sd == SIDE_BUY ? b : s;
        int pk = lo; long long pv = -1;
        for (int k = 0; k < n; k++) { long long v = V[(size_t)k]; int px = lo + k; if (v > pv || (v == pv && (sd == SIDE_SELL ? px < pk : px > pk))) { pv = v; pk = px; } }
        std::vector<long long> pb(n + 1, 0), ps(n + 1, 0), pu(n + 1, 0);
        for (int k = 0; k < n; k++) { pb[k + 1] = pb[k] + b[(size_t)k]; ps[k + 1] = ps[k] + s[(size_t)k]; pu[k + 1] = pu[k] + u[(size_t)k]; }
        bool found = false; long long bestV = -1; int bestC = 0; double bestCov = 0;
        for (int c = lo; c <= hi; c++) {
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
                else if (mid2 != INT_MIN && std::abs(2 * c - mid2) != std::abs(2 * bestC - mid2)) better = std::abs(2 * c - mid2) < std::abs(2 * bestC - mid2);
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
        for (int p = std::max(a, lo); p <= std::min(z, hi); p++) { size_t k = (size_t)(p - lo); *B += b[k]; *S += s[k]; *U += u[k]; }
    }
};

struct Ev {
    long long t = 0; int ep = 0; std::string kind; int dir = 0;    // dir +1 bullish, -1 bearish
    int lo = 0, hi = 0, h = 0; double q95 = 0, f30 = NAN, f180 = NAN, a = NAN, cov = NAN, qzone = 0, conc = 0, prog = 0;
    std::string ctx, why; int ver = 0;
};

struct SecRec { long long t = 0; float f30 = NAN, f180 = NAN, a = NAN, c30 = NAN, l30 = NAN, h30 = NAN; float b = 0, s = 0; unsigned char flags = 0; };   // (1.0.2) b / s: this second's bought / sold volume (the bar histogram)
enum { SR_QUALITY = 1, SR_WARM = 2, SR_BASE = 4, SR_LOWACT = 8, SR_SIDES = 16 };

struct Feat {
    bool f30ok = false, f180ok = false, f20ok = false;
    double f30 = 0, l30 = 0, h30 = 0, c30 = 0, f180 = 0, l180 = 0, h180 = 0, c180 = 0, c20 = 0;
    double rate20 = 0, rate5 = 0, A = NAN; bool aok = false;
    int R = 0, h = 1; bool quoteOk = false, spreadOk = true, spreadCal = false, classRecent = false, warm = false, baseOk = false;
    int bin = 0; BinBase bbv; int mid2 = INT_MIN;      // a COPY of the slot's baselines (no pointer: engines are moved)
    const BinBase* bb() const { return &bbv; }
};

struct Ep {
    int id = 0, dir = 0, state = 0;                // dir +1 bullish (sellers attempted), -1 bearish; state 1 candidate, 2 watch
    int lo = 0, hi = 0, h = 1; double q95 = 0, med20 = 0, med5 = 0; int entry2 = 0, ext2 = 0;
    long long t0 = 0, fireT = 0; int holdN = 0; bool holdFresh = false;
    int breachN = 0; double breachQty = 0;
    double f30aw = 0, covAW = 0; bool faded = false;
    int arN = 0; double arQty = 0, arKnown = 0, arAll = 0;
    int ver = 0;
};
struct Latch { int dir = 0, lo = 0, hi = 0, h = 1; double q95 = 0; bool movedAway = false; int quietN = 0; };
struct InSetup { bool on = false; long long t0 = 0; int bound2 = 0; double med5 = 0, med20 = 0; int n = 0; double qty = 0, known = 0, all = 0; };

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
    long long curSid = LLONG_MIN;
    std::string lastWhy;             // why the last second showed no signal (for the readout)
    BinBase base[NBINS];
    bool fixedBase = false;          // tests: baselines given, not frozen from the store

    Engine() {}
    void attach(Store* baselineStore, const std::string& symbol) { store = baselineStore; sym = symbol; }

    // ---- input
    bool add(const Tick& k)
    {
        if (k.q <= 0) return true;
        if ((open && k.t < cur.t) || (lastT && k.t <= lastT)) {   // late: the second was already evaluated - never rewrite it
            late++; lateNote(k.t); return false;            // counted, never added to a closed (or the wrong) second
        }
        if (!open) { stopped = false; startAt(k.t); }
        else if (k.t > cur.t) { closeUntil(k.t - 1); stopped = false; startAt(k.t); }
        addTo(cur, k); ticks++; lastTradeT = k.t;
        return true;
    }
    // close every second up to and including t (the clock moved on without trades)
    void advanceTo(long long t) { if (open && t >= cur.t) { closeUntil(t); if (!stopped) startAt(t + 1); } }
    // a hole in the data (feed gap, missed request): episodes resolve DATA_INVALID, the warm-up starts over
    void gap(long long t, const std::string& why)
    {
        for (int d = 0; d < 2; d++) if (ep[d].state) terminal(ep[d], t, "DINV", why);
        for (int d = 0; d < 2; d++) inS[d] = InSetup();
        warmN = 0; R.clear(); open = false; haveMid = false;
    }
    void setFixedBase(const BinBase& b) { fixedBase = true; for (int i = 0; i < NBINS; i++) base[i] = b; }
    void refreeze() { if (!fixedBase && store && curSid != LLONG_MIN) { freezeBase(*store, curSid, cfg.sessionsBack, base, &baseSessions); ver++; } }
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
    Slice cur; bool open = false;
    std::deque<Slice> R;             // the last ~200 closed seconds
    bool haveMid = false; int mid2 = 0; long long quoteT = LLONG_MIN; int spreadNow = -1;
    long long lastClass = LLONG_MIN;
    Ep ep[2];                        // [0] bullish (+1), [1] bearish (-1)
    std::vector<Latch> latches;
    InSetup inS[2]; bool inLatched[2] = {false, false}; int inQuiet[2] = {0, 0};
    bool balance = false; long long lastTradeT = LLONG_MIN; bool stopped = false;
    // episode ids from the creation second + side (+ kind): the same event gets the same id in every rebuild
    static int epId(long long t, int dir, bool initiative) { long long b = ((t % 10000000) + 10000000) % 10000000; return (int)(b * 4 + (dir > 0 ? 1 : 0) + (initiative ? 2 : 0)); }
    std::vector<Win> sessWins;       // the current session's windows

    static int di(int dir) { return dir > 0 ? 0 : 1; }
    static int attemptedSide(int dir) { return dir > 0 ? SIDE_SELL : SIDE_BUY; }

    void lateNote(long long t)
    {
        for (int d = 0; d < 2; d++) if (ep[d].state && t >= ep[d].t0 - (cfg.ctxW + cfg.obsW)) terminal(ep[d], cur.t, "DINV", "late trade data");
    }
    void startAt(long long t)
    {
        long long sid = sessionOf(t);
        if (sid != curSid) newSession(sid);
        cur = Slice(); cur.t = t; open = true; cur.carried = haveMid && (t - quoteT) <= cfg.quoteAge;
        cur.mids.push_back(haveMid ? mid2 : INT_MIN);
    }
    void newSession(long long sid)
    {
        flushSession();
        curSid = sid; sessWins.clear();
        if (!fixedBase && store) { freezeBase(*store, curSid, cfg.sessionsBack, base, &baseSessions); ver++; }
    }
public:
    // the current session's windows -> the store (call any time; the engine calls it at a session change)
    void flushSession()
    {
        if (!store || curSid == LLONG_MIN || sessWins.empty() || curSid == noFlushSid) return;
        std::vector<Win>& v = (*store)[curSid][sym];
        if (v.size() <= sessWins.size()) v = sessWins;
    }
private:
    void addTo(Slice& s, const Tick& k)
    {
        int sd = sideOf(k.px, k.bid, k.ask);
        Row* r = nullptr; for (Row& x : s.rows) if (x.px == k.px) { r = &x; break; }
        if (!r) { s.rows.push_back(Row{k.px, 0, 0, 0}); r = &s.rows.back(); }
        if (sd == SIDE_BUY) { r->b += k.q; s.B += k.q; } else if (sd == SIDE_SELL) { r->s += k.q; s.S += k.q; } else { r->u += k.q; s.U += k.q; }
        if (sd != SIDE_UNK) lastClass = std::max(lastClass, k.t);
        if (k.bid > 0 && k.ask > 0 && k.ask >= k.bid) {
            int m = k.bid + k.ask; haveMid = true; quoteT = std::max(quoteT, k.t); spreadNow = k.ask - k.bid;
            if (s.mids.empty() || s.mids.back() != m) { if (!s.mids.empty() && s.mids.back() == INT_MIN) s.mids.back() = m; else s.mids.push_back(m); }
            mid2 = m;
        }
    }
    void closeUntil(long long t)
    {
        while (open && cur.t <= t) {
            // no trade for 5+ minutes (closed market, halt, feed gap): stop at the same second whether the clock is walked
            // one second at a time (live) or jumped (back-fill) - the next trade starts again with a fresh warm-up
            if (lastTradeT != LLONG_MIN && cur.t - lastTradeT > cfg.maxQuietGap) { gap(cur.t, "no trades for 5+ minutes"); stopped = true; return; }
            cur.path = cur.carried && haveMid && (cur.t - quoteT) <= cfg.quoteAge;
            if (!cur.mids.empty() && cur.mids[0] == INT_MIN) cur.mids.erase(cur.mids.begin());
            if (cur.mids.empty() && haveMid) cur.mids.push_back(mid2);
            cur.spread = spreadNow;
            R.push_back(cur);
            while ((int)R.size() > cfg.ctxW + cfg.obsW + 5) R.pop_front();
            evaluate(cur.t);
            long long nt = cur.t + 1;
            if (nt > t) { open = false; break; }
            if (lastTradeT != LLONG_MIN && nt - lastTradeT > cfg.maxQuietGap) { gap(nt, "no trades for 5+ minutes"); stopped = true; return; }
            cur = Slice(); cur.t = nt; cur.carried = haveMid && (nt - quoteT) <= cfg.quoteAge; cur.mids.push_back(haveMid ? mid2 : INT_MIN);
            if (sessionOf(nt) != curSid) newSession(sessionOf(nt));
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
        f.bin = binOf(t); f.bbv = base[f.bin];
        f.baseOk = f.bb()->n >= cfg.minWin && f.bb()->med20 > 0;
        if (f.bb()->med20 > 0) { f.A = f.rate20 / f.bb()->med20; f.aok = true; }
        f.quoteOk = haveMid && (t - quoteT) <= cfg.quoteAge && !R.empty() && R.back().path;
        f.mid2 = haveMid ? mid2 : INT_MIN;
        f.spreadCal = f.bb()->medSpread >= 0;
        if (f.spreadCal && spreadNow >= 0) f.spreadOk = spreadNow <= std::max(2.0, 3.0 * f.bb()->medSpread);
        else f.spreadOk = spreadNow >= 0;
        f.classRecent = lastClass >= t - (cfg.part - 1);
        f.warm = warmN >= cfg.warm;
        // R: the midpoint range in ticks over (t-200, t-20]
        int n = (int)R.size(), lo = INT_MAX, hi = INT_MIN;
        for (int i = std::max(0, n - (cfg.ctxW + cfg.obsW)); i < n - cfg.obsW; i++) {
            const Slice& s = R[(size_t)i]; if (!s.path) continue; lo = std::min(lo, s.mn()); hi = std::max(hi, s.mx());
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
        size_t n = R.size(); if ((int)n < cfg.obsW) { r.why = "short history"; return r; }
        size_t w0 = n - (size_t)cfg.obsW;
        Dense D; D.build(R, w0, n);
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
        if (!gatesOk(f, &why)) { r.why = why; return r; }
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
        evs.push_back(e);
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
            L.quietN = qz < 0.25 * L.q95 ? L.quietN + 1 : 0;
            if (L.quietN >= cfg.quietReset) rearm = true;
            if (rearm) latches.erase(latches.begin() + (long)i); else i++;
        }
    }

    void windowStat(long long t, const Feat& f)
    {
        (void)f;
        long long x = (t + 1 - 17LL * 3600) % cfg.obsW; if (x < 0) x += cfg.obsW;
        if (x != 0 || (int)R.size() < cfg.obsW || warmN < cfg.obsW) return;
        size_t n = R.size(), w0 = n - (size_t)cfg.obsW;
        if (R[w0].t != t - cfg.obsW + 1) return;                     // not 20 contiguous seconds
        long long B = 0, S = 0, U = 0; bool path = true;
        for (size_t i = w0; i < n; i++) { B += R[i].B; S += R[i].S; U += R[i].U; if (!R[i].path) path = false; }
        const double total = (double)B + (double)S + (double)U;
        const double known = (double)B + (double)S;
        if (!(total > 0) || !path) return;
        if (known / total < cfg.cov) return;
        Win w; w.bin = binOf(t); w.rate20 = (float)(total / cfg.obsW); w.spread = (float)R.back().spread;
        for (int p = 0; p < 4; p++) { long long v = 0; for (int k = 0; k < cfg.part; k++) { const Slice& s = R[w0 + (size_t)(p * cfg.part + k)]; v += s.B + s.S + s.U; } w.r5[p] = (float)((double)v / cfg.part); }
        Dense D; D.build(R, w0, n);
        for (int h = 1; h <= 8; h++) {
            int c; long long v; double cv;
            w.Mb[h - 1] = D.best(SIDE_BUY, h, cfg.cov, INT_MIN, &c, &v, &cv) ? (float)v : 0.f;
            w.Ms[h - 1] = D.best(SIDE_SELL, h, cfg.cov, INT_MIN, &c, &v, &cv) ? (float)v : 0.f;
        }
        sessWins.push_back(w);
    }

    void evaluate(long long t)
    {
        warmN++;
        Feat f = features(t);
        last = f; lastT = t;
        windowStat(t, f);
        const Slice& s = R.back();
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
            bool qual = f.quoteOk && f.spreadOk && f.warm && f.classRecent;
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
                inQuiet[d] = (dir > 0 ? f.l30 < cfg.neutral : f.h30 > -cfg.neutral) ? inQuiet[d] + 1 : 0;
                if (inQuiet[d] >= cfg.inRearm) { inLatched[d] = false; inQuiet[d] = 0; }
            }
            double m5 = I.on ? I.med5 : f.bb()->med5, m20 = I.on ? I.med20 : f.bb()->med20;   // frozen once the setup exists
            bool P = gates && f.f30ok && f.f180ok && f.c30 >= cfg.cov && f.c180 >= cfg.cov && m20 > 0 && f.rate20 / m20 >= cfg.inAct &&
                     (dir > 0 ? (f.l30 >= cfg.inFast && f.l180 > 0) : (f.h30 <= -cfg.inFast && f.h180 < 0)) && m5 > 0 && f.rate5 >= m5;
            (void)bound;
            if (inLatched[d]) { I = InSetup(); continue; }
            if (!P) { I = InSetup(); continue; }
            if (!I.on) {
                // the preceding 20 s extreme (before this second)
                int ext = dir > 0 ? INT_MIN : INT_MAX; size_t n = R.size();
                for (size_t k = n > (size_t)cfg.obsW + 1 ? n - (size_t)cfg.obsW - 1 : 0; k + 1 < n; k++) {
                    const Slice& q = R[k]; if (!q.path) continue; ext = dir > 0 ? std::max(ext, q.mx()) : std::min(ext, q.mn());
                }
                if (ext == INT_MIN || ext == INT_MAX) continue;
                I.on = true; I.t0 = t; I.bound2 = dir > 0 ? ext + 2 : ext - 2; I.med5 = f.bb()->med5; I.med20 = f.bb()->med20; I.n = 0; I.qty = I.known = I.all = 0;
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
                e.med20 = f.bb()->med20; e.med5 = f.bb()->med5; e.entry2 = rw[d].entry2; e.ext2 = rw[d].ext2; e.t0 = t; e.ver = ver;
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
        SecRec q; q.t = t; q.b = (float)R.back().B; q.s = (float)R.back().S;
        if (f.f30ok) q.f30 = (float)f.f30;
        if (f.f180ok) q.f180 = (float)f.f180;
        if (f.aok) q.a = (float)f.A;
        q.c30 = (float)f.c30; q.l30 = (float)f.l30; q.h30 = (float)f.h30;
        bool lowAct = f.aok && f.A < 0.5;
        q.flags = (unsigned char)((f.warm ? SR_WARM : 0) | (f.baseOk ? SR_BASE : 0) | (lowAct ? SR_LOWACT : 0) |
                                  (f.c30 >= cfg.cov ? SR_SIDES : 0) | (f.warm && f.c30 >= cfg.cov && f.quoteOk && !lowAct ? SR_QUALITY : 0));
        hist.push_back(q);
        if (hist.size() > (size_t)cfg.keepSec + 3600) hist.erase(hist.begin(), hist.begin() + 3600);
    }
};

// ---- the store file: W|session|symbol|bin|rate20|r5a|r5b|r5c|r5d|spread|Mb1..8|Ms1..8
inline std::string storeLine(long long sid, const std::string& sym, const Win& w)
{
    char b[600]; int k = snprintf(b, sizeof(b), "W|%lld|%s|%d|%.4g|%.4g|%.4g|%.4g|%.4g|%.4g", sid, sym.c_str(), w.bin, w.rate20, w.r5[0], w.r5[1], w.r5[2], w.r5[3], w.spread);
    for (int h = 0; h < 8; h++) k += snprintf(b + k, sizeof(b) - (size_t)k, "|%.6g", w.Mb[h]);
    for (int h = 0; h < 8; h++) k += snprintf(b + k, sizeof(b) - (size_t)k, "|%.6g", w.Ms[h]);
    return std::string(b);
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
    double value = std::strtod(text.c_str(), &end);
    if (errno == ERANGE || end == text.c_str() || *end != '\0' || !std::isfinite(value) ||
        value < -(double)std::numeric_limits<float>::max() || value > (double)std::numeric_limits<float>::max()) return false;
    *out = (float)value; return true;
}
inline bool parseStoreLine(const std::string& ln, long long* sid, std::string* sym, Win* w)
{
    if (!sid || !sym || !w || ln.size() < 3 || ln[0] != 'W' || ln[1] != '|') return false;
    std::vector<std::string> c; size_t a = 2;
    while (true) { size_t p = ln.find('|', a); c.push_back(ln.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; }
    if (c.size() < 25 || c[1].empty() || c[1].size() > 63 || !parseStoreLong(c[0], sid) || !parseStoreInt(c[2], &w->bin) ||
        !parseStoreFloat(c[3], &w->rate20) || w->rate20 < 0) return false;
    *sym = c[1];
    for (int k = 0; k < 4; k++) if (!parseStoreFloat(c[4 + (size_t)k], &w->r5[k]) || w->r5[k] < 0) return false;
    if (!parseStoreFloat(c[8], &w->spread) || w->spread < -1) return false;
    for (int h = 0; h < 8; h++) {
        if (!parseStoreFloat(c[9 + (size_t)h], &w->Mb[h]) || !parseStoreFloat(c[17 + (size_t)h], &w->Ms[h]) || w->Mb[h] < 0 || w->Ms[h] < 0) return false;
    }
    return w->bin >= 0 && w->bin < NBINS;
}

// how many prior sessions (same contract) the store holds before session sid
inline int priorSessions(const Store& st, long long sid)
{
    int n = 0; for (auto& kv : st) if (kv.first < sid && frontOf(kv.second)) n++;
    return n;
}

} // namespace tfl
