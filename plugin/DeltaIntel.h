#ifndef LS_DELTAINTEL_H
#define LS_DELTAINTEL_H
// DeltaIntel.h 1.2.0 (2026-11-09, Rassul 23:17 CT "looks good build" on the A / I model mockup): DINTEL_A_MODE 3 (DEFAULT) = ONE rule set
// for A and I on the 90-min profile (60 rows): node |delta| >= 2.0x the average non-zero row and one of the top 3; the peak candle (most
// same-side delta on the row) = the decision candle k (same) or k-1 (2-bar); bullish = a GREEN close above the node's top on the peak
// candle, or (2-bar, the peak did not) on the NEXT candle whose low is below the peak candle's high (bearish mirror); the 5-bar pivot rule;
// the TRUE WICK rule (bullish: the whole node <= min(open, close) of the PEAK candle; bearish: >= max(open, close)); the halfway rule.
// Letter by node side: bullish + RED (selling) node = A, bullish + GREEN node = I, bearish + GREEN node = A, bearish + RED node = I.
// Decided at the close (never repaints). The circle = the strongest A / I of its candle, drawn ON THE CANDLE at the node, side colour.
// DeltaIntel.h 1.1.0 (2026-11-07 ~22:10, Rassul "i'll go with your recommendations"): A = THE CHOSEN RULE (ownA below, = lra.dp_intel
// 1.2.0 own_A) - the node is one of the CANDLE's own 3 biggest delta rows (>= 2.0x the candle's average row), same candle, winner's colour
// close beyond it, at / below the open and below 1/2 (in the wick), close in the top 1/3, the attackers' (bid) POC or the volume POC in the
// bottom 1/3, his 3-bar ZigZag pivot rule; bearish mirror. The CIRCLE = the strongest A of its candle; its MARK = acc (the attackers' POC
// itself in the absorbing third, known at the close) or ai (an I the same way within 5 candles - added later, the circle never changes).
// Engine::probe() judges the FORMING candle the same way for the live circle (never recorded; final at the close).
// 1.0.0 (2026-11-07, Rassul 19:52-19:58 CT: all Delta Profile signals as letters in front of the ratio - A absorbed,
// F fading, E exhaustion, I initiation, T trapped - "highly accurate ... run it on data for each market", "aware of ... levels, vwap,
// pivots, trend, momentum, breakouts ... its own history and the price chart history"; "go and make an Intelligent Delta Profile").
// SDK-free and unit-tested (tests_dintel/t_dintel.cpp, parity with lra.dp_intel 1.0.0 on real sessions of all 7 markets).
//
// THE ENGINE: one session's CLOSED 3-min candles (OHLC in whole ticks, volume, per-price bought / sold lots), fed in order. At the close
// of candle k it decides every letter that candle makes - the same rules as lra.dp_intel (the research that measured them):
//   A  dp::researchA (DeltaProfileCore.h 2.27) - 30-candle window, 60 rows, top 3 rows >= 2.0x, peak = k or k-1, defenders' close
//      (same / next candle), pivot, open and halfway rules. Selling row -> bullish.
//   I  the mirror of A: a top-3 row >= 2.0x with peak k or k-1 whose candle k closes BEYOND it in the ATTACKERS' colour.
//   F  exh::attackerFade after a decided A (within 20 candles of its peak): later half / earlier half of the attack < 0.5 (3+ candles).
//   E  exh::evaluate + resolve: a 10-candle extreme within 4 ticks of a key level, thin (< 0.35x) and fading (< 0.5x), printed on the
//      first of the next 3 candles that closes back.
//   T  trapl::decide: new 20-candle extreme, attackers beyond the old extreme >= 1.0x the median attack, closed back inside.
// THE CONTEXT (15 conditions, all from the chart's own candles): key level / VWAP near, with / against the trend, at a pivot, a
// breakout with / against, fast move, OWN HISTORY (the same letter at this price earlier today; the last one there failed; today's
// letters of this kind mostly right) and CHART HISTORY (first test of the level, an old swing, at the day's extreme, first RTH hour).
// THE GATE: DeltaCtxParams.h (written nightly by lra.dp_intel) - per market and letter: 0 LOG (recorded, never drawn), 1 "?", 2 SHOW,
// plus up to two conditions that must both hold. A letter is decided ONCE at its candle's close and never changes (no repaint).
// THE TRUTH (scored here too, for the status line and the record): the next 10 candles - a 3-min CLOSE beyond = broken; I = 0.5 ATR its
// way before a close back through the node.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "DeltaCtxParams.h"

namespace dintel {
static const char* const VERSION = "1.2.0";
static const int HORIZON = 10, WIN = 30, NROWS = 60, TOP = 3;
static const double RATIO = 2.0;
// (2026-11-07 ~20:45, coordinator for Rassul) two A switches wait for his answer - the defaults are the CURRENT plugin (dp::researchA):
//   A_OPEN_HALFWAY true  = lsDeltaProfile 2.27.0's open + halfway rules (his recorded 10-27 15:04 instruction); false = his 10-17 "no halfway"
//   A_PIVOT_ZZ3    false = the pivot bar = the lowest low (bullish) of the candles [peak-5, k]; true = his 3-bar ZigZag down-pivot
#ifndef DINTEL_A_OPEN_HALFWAY
#define DINTEL_A_OPEN_HALFWAY 1
#endif
#ifndef DINTEL_A_PIVOT_ZZ3
#define DINTEL_A_PIVOT_ZZ3 0
#endif
static const bool A_OPEN_HALFWAY = DINTEL_A_OPEN_HALFWAY != 0, A_PIVOT_ZZ3 = DINTEL_A_PIVOT_ZZ3 != 0;
// (2026-11-07 ~21:00, Rassul's NEW A - DEFAULT, supersedes the two switches above): bullish A = on its OWN candle only (no next-candle
// close), the candle GREEN, the node at / below the candle's OPEN, below 1/2 of its range and IN THE LOWER WICK (below the body); the pivot
// rule on his 3-bar ZigZag (the candle IS the down-pivot bar or its low is below the down-pivot bar's high). Bearish mirror (red, upper
// wick). DINTEL_A_MODE 0 = the old dp::researchA path with the two switches.
// DINTEL_A_MODE 2 (DEFAULT, 1.1.0) = the chosen own-candle rule; 1 = the 1.0.0 wick rule on the 90-min top-3 node; 0 = dp::researchA
#ifndef DINTEL_A_MODE
#define DINTEL_A_MODE 3
#endif
static const bool A_OWN = DINTEL_A_MODE == 2;
static const bool A_UNI = DINTEL_A_MODE == 3;   // (1.2.0) the unified A / I rule
static const bool A_WICK = DINTEL_A_MODE == 1;
static const char* const A_RULE_LABEL = DINTEL_A_MODE == 3 ? "A/I (90-min top-3 node >= 2x, peak k or k-1, close beyond, 5-bar pivot, true wick, halfway; red node bull = A, green node bull = I)"
                                      : DINTEL_A_MODE == 2 ? "own (candle top-3 node >= 2x, same candle, winner's close, in the wick, close top 1/3, bid or volume POC bottom 1/3, 3-bar ZigZag pivot)"
                                      : DINTEL_A_MODE == 1 ? "wick (own candle, favourable colour, node in the wick below the open and 1/2 range, 3-bar ZigZag pivot)" : "researchA";
// (2026-11-07 ~21:10, Rassul chose 9 minutes) the hold judged per letter: A, T, A-then-I (J), T-confirmed (U) over 3 candles; I, E, F over 10
inline int horizonOf(char l) { return (l == 'A' || l == 'T' || l == 'J' || l == 'U') ? 3 : HORIZON; }
enum Cond { C_NONE = 0, LVL_KEY, LVL_VWAP, WITH_TREND, AGAINST_TREND, AT_PIVOT, BO_WITH, BO_AGAINST, MOM_HI, OWN_SEEN, OWN_FAILED, OWN_GOOD,
            TEST1, SWING_OLD, AT_EXT, OPEN_HR, N_COND };
static const char* const COND_NAME[N_COND] = {"none", "lvl_key", "lvl_vwap", "with_trend", "against_trend", "at_pivot", "bo_with", "bo_against",
                                              "mom_hi", "own_seen", "own_failed", "own_good", "test1", "swing_old", "at_ext", "open_hr"};
struct Cell { long long key = 0; double buy = 0, sell = 0; };
struct Candle { long long t = 0; int mod = 0; long long O = 0, H = 0, L = 0, C = 0; double vol = 0; std::vector<Cell> cells; };   // t = start (sec), mod = start minute of day
struct Event {
    char letter = 'A'; int k = -1, pk = -1, side = 0; long long lo = 0, hi = 0, ext = 0; int tpr = 1; double ratio = 0;
    bool ctx[N_COND] = {false}; int right = -1; int status = 0; bool shown = false; bool faded = false, ai = false; std::string conf;
    bool circle = false, acc = false; int markK = -1;   // (1.1.0) the A circle (strongest A of its candle) and the candle its mark was added at
    bool marked() const { return circle && markK >= 0; }
    unsigned bits() const { unsigned b = 0; for (int c = 1; c < N_COND; ++c) if (ctx[c]) b |= 1u << c; return b; }
};
struct Rth { int open = 510, close = 900; };
inline Rth rthOf(const std::string& m) {
    Rth r;
    if (m == "CL" || m == "NG") { r.open = 480; r.close = 810; }
    else if (m == "GC" || m == "HG") { r.open = 440; r.close = 750; }
    else if (m == "EU") { r.open = 440; r.close = 840; }
    return r;
}
inline double median(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}
inline long long floorDiv(long long a, long long b) { long long q = a / b, r = a % b; return q - (r < 0 ? 1 : 0); }

struct Levels { std::vector<std::pair<std::string, long long> > v; };
// PDH / PDL / PRV_POC from the PREVIOUS session's RTH candles; ONH / ONL (17:00 -> RTH open) and LonHI / LonLO (02:00 -> min(07:30, open))
// from this session's candles before the open. Order = lra.dp_intel.key_levels (ONH ONL LonHI LonLO PDH PDL PRV_POC).
inline Levels keyLevels(const std::vector<Candle>& cur, const std::vector<Candle>* prev, const Rth& r) {
    Levels L;
    long long onh = 0, onl = 0, lh = 0, ll = 0; bool on = false, lon = false;
    for (const Candle& c : cur) {
        if (c.mod >= 17 * 60 || c.mod < r.open) { if (!on) { onh = c.H; onl = c.L; on = true; } else { onh = std::max(onh, c.H); onl = std::min(onl, c.L); } }
        if (c.mod >= 120 && c.mod < std::min(450, r.open)) { if (!lon) { lh = c.H; ll = c.L; lon = true; } else { lh = std::max(lh, c.H); ll = std::min(ll, c.L); } }
    }
    if (on) { L.v.push_back(std::make_pair(std::string("ONH"), onh)); L.v.push_back(std::make_pair(std::string("ONL"), onl)); }
    if (lon) { L.v.push_back(std::make_pair(std::string("LonHI"), lh)); L.v.push_back(std::make_pair(std::string("LonLO"), ll)); }
    if (prev) {
        long long ph = 0, pl = 0; bool any = false; std::map<long long, double> vol;
        for (const Candle& c : *prev) {
            if (!(c.mod >= r.open && c.mod < r.close)) continue;
            if (!any) { ph = c.H; pl = c.L; any = true; } else { ph = std::max(ph, c.H); pl = std::min(pl, c.L); }
            for (const Cell& x : c.cells) vol[x.key] += x.buy + x.sell;
        }
        if (any) {
            L.v.push_back(std::make_pair(std::string("PDH"), ph)); L.v.push_back(std::make_pair(std::string("PDL"), pl));
            double best = 0; long long at = 0; bool got = false;
            for (std::map<long long, double>::const_iterator i = vol.begin(); i != vol.end(); ++i) if (i->second > best) { best = i->second; at = i->first; got = true; }
            if (got) L.v.push_back(std::make_pair(std::string("PRV_POC"), at));
        }
    }
    return L;
}
inline const dctx_params::Row* gateOf(const std::string& m, char letter) {
    for (int i = 0; i < dctx_params::N_ROWS; ++i) if (m == dctx_params::ROWS[i].mkt && letter == dctx_params::ROWS[i].letter) return &dctx_params::ROWS[i];
    return nullptr;
}

class Engine {
public:
    std::string market; Rth rth; Levels lv; std::vector<Candle> c; std::vector<double> atr, vwap; std::vector<Event> ev;
    std::map<std::string, int> touch; double cv = 0, ctp = 0; int r0 = -1; bool levelsSet = false;
    long long decided[7] = {0, 0, 0, 0, 0, 0, 0}, shownN = 0, scored = 0, right = 0, circles = 0, marksAcc = 0, marksAi = 0;
    std::vector<int> newMarks;   // (1.1.0) indexes (in ev) of A circles whose "ai" mark was added by the last add()
    explicit Engine(const std::string& m = "ES") : market(m), rth(rthOf(m)) {}
    void setLevels(const Levels& L) { lv = L; levelsSet = true; }
    bool isRth(int k) const { return c[k].mod >= rth.open && c[k].mod < rth.close; }
    // append the next CLOSED candle; returns the indexes (in ev) of the letters it decided
    std::vector<int> add(const Candle& x) {
        newMarks.clear();
        c.push_back(x);
        const int k = static_cast<int>(c.size()) - 1;
        // ATR(13) Wilder from the session's first candle; RTH VWAP of the typical price
        double tr = static_cast<double>(x.H - x.L);
        {   const double pc = static_cast<double>(k > 0 ? c[k - 1].C : x.C);   // = lra.dp_intel.atr13 (the first candle's previous close = its own close)
            tr = std::max(tr, std::max(std::fabs(static_cast<double>(x.H) - pc), std::fabs(static_cast<double>(x.L) - pc))); }
        atr.push_back(k == 0 ? tr : (atr[k - 1] * 12 + tr) / 13.0);
        const bool r = isRth(k);
        if (r) { const double v = x.vol, tp = (x.H + x.L + x.C) / 3.0; cv += v; ctp += tp * v; if (r0 < 0) r0 = k; }
        vwap.push_back(cv > 0 ? ctp / cv : NAN);
        std::vector<int> out;
        // score letters whose 10 candles are now closed (needed for own history before this candle's letters are decided)
        for (Event& e : ev) if (e.right < 0 && e.k + horizonOf(e.letter) == k) score(e);
        if (!r || k < 40) return out;
        for (const std::pair<std::string, long long>& L : lv.v) if (x.L - 4 <= L.second && L.second <= x.H + 4) ++touch[L.first];
        std::vector<Event> nw;
        detectAI(k, nw); detectT(k, nw); detectE(k, nw); detectF(k, nw);
        // research letters (= lra.dp_intel): J = an A followed within 5 candles by an I the SAME way (the I's of this candle before the
        // duplicate check); U = last candle's T confirmed now (no new extreme beyond the trap, a close back inside the old extreme)
        {   std::vector<int> iSides; for (const Event& e : nw) if (e.letter == 'I') iSides.push_back(e.side);
            for (Event& a : ev) {
                if (a.letter != 'A' || a.ai || !(a.k < k && k <= a.k + 5)) continue;
                if (std::find(iSides.begin(), iSides.end(), a.side) == iSides.end()) continue;
                a.ai = true; if (a.circle && a.markK < 0) { a.markK = k; ++marksAi; newMarks.push_back(static_cast<int>(&a - &ev[0])); }
                Event j = a; j.letter = 'J'; j.k = k; j.right = -1; j.faded = false; j.ai = false; j.shown = false; j.status = 0; nw.push_back(j);
            }
            for (const Event& t : ev) {
                if (t.letter != 'T' || t.k != k - 1) continue;
                const bool top = t.side == -1; const long long px = top ? t.lo - 1 : t.hi + 1;
                if (top ? (c[k].H <= t.ext && c[k].C <= px) : (c[k].L >= t.ext && c[k].C >= px)) { Event u = t; u.letter = 'U'; u.k = k; u.right = -1; u.shown = false; u.status = 0; nw.push_back(u); }
            } }
        std::vector<Event> seen;
        for (Event& e : nw) {
            bool dup = false;
            for (const Event& s : seen) if (same(s, e)) dup = true;
            for (const Event& h : ev) if (same(h, e) && h.k >= k - 2) dup = true;
            if (dup) continue;
            seen.push_back(e);
            context(e);
            const dctx_params::Row* g = gateOf(market, e.letter);
            e.status = g ? g->status : 0;
            auto holds = [&](int id) { return id == 0 || (id > 0 && id < N_COND && e.ctx[id]); };   // an id this build does not know never holds
            e.shown = e.status > 0 && g && holds(g->c1) && holds(g->c2);
            if (e.shown) ++shownN;
            if ((e.letter == 'A' || (A_UNI && e.letter == 'I')) && e.circle) { ++circles; if (e.acc) { e.markK = k; ++marksAcc; } }
            ev.push_back(e); out.push_back(static_cast<int>(ev.size()) - 1);
            ++decided[idx(e.letter)];
        }
        return out;
    }
    // (1.1.0) the LIVE circle: the chosen-rule A's of the FORMING candle x (as if it closed now) - nothing is stored; the A's of the same
    // node decided in the last 2 candles are left out (the closed-candle duplicate rule)
    std::vector<Event> probe(const Candle& x) {
        std::vector<Event> out;
        c.push_back(x);
        const int k = static_cast<int>(c.size()) - 1;
        if (isRth(k) && k >= 40) {
            std::vector<Event> nw; detectAI(k, nw);
            for (const Event& e : nw) {
                if (!(e.letter == 'A' || (A_UNI && e.letter == 'I'))) continue;
                bool dup = false; for (const Event& h : ev) if (same(h, e) && h.k >= k - 2) dup = true;
                if (!dup) out.push_back(e);
            }
        }
        c.pop_back();
        return out;
    }
    static int idx(char l) { return l == 'A' ? 0 : l == 'F' ? 1 : l == 'E' ? 2 : l == 'I' ? 3 : l == 'T' ? 4 : l == 'J' ? 5 : 6; }

private:
    static bool same(const Event& a, const Event& b) { return a.letter == b.letter && a.lo == b.lo && a.hi == b.hi && a.side == b.side; }
    void score(Event& e) {
        const int k = e.k; const int s = e.side; const int hz = horizonOf(e.letter);
        int res = 0;
        if (e.letter == 'A' || e.letter == 'F' || e.letter == 'J') {
            res = 1; for (int j = k + 1; j <= k + hz; ++j) if (s == 1 ? c[j].C < e.lo : c[j].C > e.hi) res = 0;
        } else if (e.letter == 'E' || e.letter == 'T' || e.letter == 'U') {
            res = 1; for (int j = k + 1; j <= k + hz; ++j) if (s == -1 ? c[j].C > e.ext : c[j].C < e.ext) res = 0;
        } else {
            const double tgt = c[k].C + s * 0.5 * atr[k]; const long long stop = s == 1 ? e.lo : e.hi;
            for (int j = k + 1; j <= k + hz; ++j) {
                if (s == 1 ? c[j].H >= tgt : c[j].L <= tgt) { res = 1; break; }
                if (s == 1 ? c[j].C < stop : c[j].C > stop) { res = 0; break; }
            }
        }
        e.right = res; ++scored; right += res;
    }
    // delta / volume of candle i in [lo, hi] (ticks)
    void region(int i, long long lo, long long hi, double& d, double& v) const {
        d = 0; v = 0;
        for (const Cell& x : c[i].cells) if (x.key >= lo && x.key <= hi) { d += x.buy - x.sell; v += x.buy + x.sell; }
    }
    // his 3-bar High / Low ZigZag (zzd::Engine::step, N = 3) over candles 0..k (= lra.dp_intel.zz3_piv): bullish -> the down pivot
    // (the live down-leg low, else the up-leg's start), bearish mirror
    int zz3Pivot(int k, bool bullish) const {
        if (k < 1) return 0;
        int dirn = c[1].H >= c[0].H ? 1 : -1, ext = 0, start = 0;
        for (int j = 1; j <= k; ++j) {
            const int j0 = std::max(0, j - 3);
            if (dirn == 1) {
                if (c[j].H > c[ext].H) { ext = j; continue; }
                long long pl = c[j0].L; for (int q = j0 + 1; q < j; ++q) pl = std::min(pl, c[q].L);
                if (c[j].L < pl && j != ext) { start = ext; ext = j; dirn = -1; }
            } else {
                if (c[j].L < c[ext].L) { ext = j; continue; }
                long long ph = c[j0].H; for (int q = j0 + 1; q < j; ++q) ph = std::max(ph, c[q].H);
                if (c[j].H > ph && j != ext) { start = ext; ext = j; dirn = 1; }
            }
        }
        if (bullish) return dirn == -1 ? ext : start;
        return dirn == 1 ? ext : start;
    }
    // THE CHOSEN A (= lra.dp_intel.own_A): the candle's own rows at the 90-min profile's row size [lo + r * tpr, ...]
    void ownA(int k, long long lo, long long hi, long long tpr, std::vector<Event>& out) const {
        const Candle& x = c[k];
        std::map<long long, double> rows;
        for (const Cell& q : x.cells) if (q.key >= std::max(lo, x.L) && q.key <= std::min(hi, x.H) && q.buy + q.sell > 0) rows[(q.key - lo) / tpr] += q.buy - q.sell;
        std::vector<long long> nz; double tot = 0;
        for (std::map<long long, double>::const_iterator i = rows.begin(); i != rows.end(); ++i) if (i->second != 0) { nz.push_back(i->first); tot += std::fabs(i->second); }
        if (nz.empty()) return;
        const double n = static_cast<double>(nz.size());
        std::stable_sort(nz.begin(), nz.end(), [&rows](long long p, long long q) { return std::fabs(rows[p]) > std::fabs(rows[q]); });
        const long long rng = std::max<long long>(1, x.H - x.L);
        // per-price lots over [L, H]: sold (bid), bought (ask), volume
        const size_t np = static_cast<size_t>(x.H - x.L + 1);
        std::vector<double> sold(np, 0), bought(np, 0), vol(np, 0);
        for (const Cell& q : x.cells) if (q.key >= x.L && q.key <= x.H) { const size_t i = static_cast<size_t>(q.key - x.L); sold[i] += q.sell; bought[i] += q.buy; vol[i] += q.buy + q.sell; }
        auto poc = [&](const std::vector<double>& a, bool bull, long long& at) -> bool {   // ties -> the price nearer the absorbing extreme
            double sum = 0; for (double v : a) sum += v; if (sum <= 0) return false;
            double best = -1; at = 0;
            if (bull) { for (size_t i = 0; i < np; ++i) if (a[i] > best) { best = a[i]; at = x.L + static_cast<long long>(i); } }
            else { for (size_t i = np; i-- > 0;) if (a[i] > best) { best = a[i]; at = x.L + static_cast<long long>(i); } }
            return true;
        };
        const size_t first = out.size();
        for (size_t t = 0; t < nz.size() && t < 3; ++t) {
            const long long r = nz[t]; const double v = rows[r];
            if (std::fabs(v) * n < 2.0 * tot) continue;
            const bool bull = v < 0;
            const long long rlo = lo + r * tpr, rhi = std::min(rlo + tpr - 1, hi);
            const bool ok = bull ? (x.C > x.O && x.C > rhi && rhi < x.O && (rlo + rhi) < (x.H + x.L) && 3 * (x.C - x.L) >= 2 * rng)
                                 : (x.C < x.O && x.C < rlo && rlo > x.O && (rlo + rhi) > (x.H + x.L) && 3 * (x.H - x.C) >= 2 * rng);
            if (!ok) continue;
            long long pa = 0, pv = 0;
            const bool ha = poc(bull ? sold : bought, bull, pa), hv = poc(vol, bull, pv);
            auto low3 = [&](bool has, long long p) { return has && (bull ? 3 * (p - x.L) <= rng : 3 * (x.H - p) <= rng); };
            if (!(low3(ha, pa) || low3(hv, pv))) continue;
            const int zp = zz3Pivot(k, bull);
            if (!(k == zp || (bull ? x.L < c[zp].H : x.H > c[zp].L))) continue;
            Event e; e.letter = 'A'; e.k = k; e.pk = k; e.side = bull ? 1 : -1; e.lo = rlo; e.hi = rhi; e.ratio = std::fabs(v) * n / tot;
            e.tpr = static_cast<int>(tpr); e.conf = "own"; e.acc = low3(ha, pa); out.push_back(e);
        }
        if (out.size() > first) {   // the circle = the strongest A of the candle (ties -> the first)
            size_t b = first; for (size_t i = first + 1; i < out.size(); ++i) if (out[i].ratio > out[b].ratio) b = i;
            out[b].circle = true;
        }
    }
    void detectAI(int k, std::vector<Event>& out) const {
        const int a = std::max(0, k - WIN + 1);
        if (k - a + 1 < 7) return;
        std::map<long long, std::pair<double, double> > prof;
        for (int i = a; i <= k; ++i) for (const Cell& x : c[i].cells) { std::pair<double, double>& p = prof[x.key]; p.first += x.buy - x.sell; p.second += x.buy + x.sell; }
        std::vector<long long> pres;
        for (std::map<long long, std::pair<double, double> >::const_iterator i = prof.begin(); i != prof.end(); ++i) if (i->second.second > 0) pres.push_back(i->first);
        if (pres.size() < 5) return;
        const long long lo = pres.front(), hi = pres.back(), span = hi - lo;
        const long long tpr = std::max<long long>(1, (span + NROWS - 1) / NROWS);
        const long long nrow = span / tpr + 1;
        std::vector<double> rows(static_cast<size_t>(nrow), 0.0);
        for (std::map<long long, std::pair<double, double> >::const_iterator i = prof.begin(); i != prof.end(); ++i)
            if (i->first >= lo && i->first <= hi) rows[static_cast<size_t>((i->first - lo) / tpr)] += i->second.first;
        double sum = 0; int nz = 0;
        for (double v : rows) if (std::fabs(v) > 0) { sum += std::fabs(v); ++nz; }
        if (nz < 3) return;
        const double mean = sum / nz;
        std::vector<int> order(rows.size()); for (size_t i = 0; i < order.size(); ++i) order[i] = static_cast<int>(i);
        std::stable_sort(order.begin(), order.end(), [&rows](int p, int q) { return std::fabs(rows[static_cast<size_t>(p)]) > std::fabs(rows[static_cast<size_t>(q)]); });
        if (A_OWN) ownA(k, lo, hi, tpr, out);
        const size_t first = out.size();
        for (int rank = 0; rank < TOP && rank < static_cast<int>(order.size()); ++rank) {
            const int r = order[static_cast<size_t>(rank)];
            const double ratio = std::fabs(rows[static_cast<size_t>(r)]) / mean;
            if (ratio < RATIO) break;
            const long long rlo = lo + r * tpr, rhi = std::min(rlo + tpr - 1, hi);
            const bool sell = rows[static_cast<size_t>(r)] < 0;
            int pk = -1; double best = 0;
            for (int i = a; i <= k; ++i) { double d, v; region(i, rlo, rhi, d, v); d *= sell ? -1.0 : 1.0; if (d > 0 && (pk < 0 || d > best)) { best = d; pk = i; } }
            if (pk < 0 || (pk != k && pk != k - 1)) continue;
            const Candle* C = &c[0];
            if (A_UNI) {   // (1.2.0) the unified A / I rule
                Event e; if (uniAI(k, pk, rlo, rhi, sell, e)) { e.ratio = ratio; e.tpr = static_cast<int>(tpr); out.push_back(e); }
                continue;
            }
            if (A_OWN) {   // (1.1.0) the 90-min rows give I only; A comes from ownA
                if ((!sell && C[k].C > C[k].O && C[k].C > rhi) || (sell && C[k].C < C[k].O && C[k].C < rlo)) {
                    Event e; e.k = k; e.pk = pk; e.lo = rlo; e.hi = rhi; e.ratio = ratio; e.tpr = static_cast<int>(tpr); e.letter = 'I'; e.side = sell ? -1 : 1; e.conf = "k"; out.push_back(e);
                }
                continue;
            }
            auto conf = [&](int q) { return sell ? (C[q].C > C[q].O && C[q].C > rhi) : (C[q].C < C[q].O && C[q].C < rlo); };
            if (A_WICK) {
                const int zp = zz3Pivot(k, sell);
                const bool okw = pk == k && (sell ? (C[k].C > C[k].O && rhi < C[k].O && (rlo + rhi) < (C[k].H + C[k].L))
                                                  : (C[k].C < C[k].O && rlo > C[k].O && (rlo + rhi) > (C[k].H + C[k].L)));
                if (okw && (k == zp || (sell ? C[k].L < C[zp].H : C[k].H > C[zp].L))) {
                    Event e; e.k = k; e.pk = pk; e.lo = rlo; e.hi = rhi; e.ratio = ratio; e.tpr = static_cast<int>(tpr);
                    e.letter = 'A'; e.side = sell ? 1 : -1; e.conf = "wick"; out.push_back(e); continue;
                }
            }
            int pb;
            if (A_PIVOT_ZZ3) pb = zz3Pivot(k, sell);
            else { pb = std::max(0, pk - 5); for (int i = pb + 1; i <= k; ++i) if (sell ? C[i].L < C[pb].L : C[i].H > C[pb].H) pb = i; }
            const bool piv = pb == pk || (sell ? C[pk].L < C[pb].H : C[pk].H > C[pb].L);
            std::string how;
            if (pk == k && conf(k)) how = "same";
            else if (pk == k - 1 && !conf(pk) && conf(k) && (sell ? C[k].L < C[pb].H : C[k].H > C[pb].L)) how = "next";
            const bool body = !A_OPEN_HALFWAY || ((sell ? rhi <= C[pk].O : rlo >= C[pk].O) && (sell ? (rlo + rhi) <= (C[pk].H + C[pk].L) : (rlo + rhi) >= (C[pk].H + C[pk].L)));
            Event e; e.k = k; e.pk = pk; e.lo = rlo; e.hi = rhi; e.ratio = ratio; e.tpr = static_cast<int>(tpr);
            if (!A_WICK && !how.empty() && piv && body) { e.letter = 'A'; e.side = sell ? 1 : -1; e.conf = how; out.push_back(e); continue; }
            if ((!sell && C[k].C > C[k].O && C[k].C > rhi) || (sell && C[k].C < C[k].O && C[k].C < rlo)) {
                e.letter = 'I'; e.side = sell ? -1 : 1; e.conf = "k"; out.push_back(e);
            }
        }
        if (A_UNI && out.size() > first) {   // the circle = the strongest A / I of the candle (ties -> the first)
            size_t b = first; for (size_t i = first + 1; i < out.size(); ++i) if (out[i].ratio > out[b].ratio) b = i;
            out[b].circle = true;
        }
    }
public:
    // (1.2.0) the unified A / I rule for one node [rlo, rhi] (sell = a red / selling node) whose peak candle is pk (k or k-1), judged at the
    // close of k. Rules 4-9 of the model: close beyond (same candle, or 2-bar on the next candle that overlaps the peak), the 5-bar pivot,
    // the TRUE wick (the whole node outside the peak candle's body), the halfway rule; A when the node is the losing side, else I.
    bool uniAI(int k, int pk, long long rlo, long long rhi, bool sell, Event& e) const {
        if (pk != k && pk != k - 1) return false;
        const Candle* C = &c[0];
        for (int d = 0; d < 2; ++d) {
            const bool bull = d == 0;
            auto beyond = [&](int q) { return bull ? (C[q].C > C[q].O && C[q].C > rhi) : (C[q].C < C[q].O && C[q].C < rlo); };
            std::string how;
            if (pk == k) { if (beyond(k)) how = "same"; }
            else if (!beyond(pk) && beyond(k) && (bull ? C[k].L < C[pk].H : C[k].H > C[pk].L)) how = "next";
            if (how.empty()) continue;
            int pb = std::max(0, pk - 5);   // the 5-bar pivot rule (unchanged)
            for (int i = pb + 1; i <= k; ++i) if (bull ? C[i].L < C[pb].L : C[i].H > C[pb].H) pb = i;
            if (!(pb == pk || (bull ? C[pk].L < C[pb].H : C[pk].H > C[pb].L))) continue;
            if (!(bull ? rhi <= std::min(C[pk].O, C[pk].C) : rlo >= std::max(C[pk].O, C[pk].C))) continue;   // the TRUE wick rule
            if (!(bull ? (rlo + rhi) <= (C[pk].H + C[pk].L) : (rlo + rhi) >= (C[pk].H + C[pk].L))) continue;   // the halfway rule
            e = Event(); e.k = k; e.pk = pk; e.lo = rlo; e.hi = rhi; e.side = bull ? 1 : -1; e.conf = how;
            e.letter = (bull == sell) ? 'A' : 'I';
            return true;
        }
        return false;
    }
private:
    void detectT(int k, std::vector<Event>& out) const {
        if (k < 20) return;
        for (int t = 0; t < 2; ++t) {
            const bool top = t == 0;
            long long prevX = top ? c[k - 20].H : c[k - 20].L;
            for (int i = k - 20; i < k; ++i) prevX = top ? std::max(prevX, c[i].H) : std::min(prevX, c[i].L);
            if (!(top ? c[k].H > prevX : c[k].L < prevX)) continue;
            std::vector<double> prior;
            for (int i = k - 20; i < k; ++i) { double a = 0; for (const Cell& x : c[i].cells) a += top ? x.buy : x.sell; if (a > 0) prior.push_back(a); }
            if (prior.size() < 10) continue;
            double lots = 0; for (const Cell& x : c[k].cells) if (top ? x.key > prevX : x.key < prevX) lots += top ? x.buy : x.sell;
            const bool backIn = top ? c[k].C <= prevX : c[k].C >= prevX;
            const double xr = lots / std::max(median(prior), 1.0);
            if (lots > 0 && backIn && xr >= 1.0) {
                Event e; e.letter = 'T'; e.k = k; e.pk = k; e.side = top ? -1 : 1; e.lo = top ? prevX + 1 : c[k].L; e.hi = top ? c[k].H : prevX - 1;
                e.ratio = xr; e.ext = top ? c[k].H : c[k].L; out.push_back(e);
            }
        }
    }
    void detectE(int k, std::vector<Event>& out) const {
        for (int e = std::max(40, k - 3); e < k; ++e) for (int t = 0; t < 2; ++t) {
            const bool top = t == 0;
            const long long ext = top ? c[e].H : c[e].L;
            bool newX = true; for (int i = e - 10; i < e; ++i) if (top ? !(c[e].H > c[i].H) : !(c[e].L < c[i].L)) newX = false;
            if (!newX || lv.v.empty()) continue;
            long long dmin = -1; for (const std::pair<std::string, long long>& L : lv.v) { const long long d = std::llabs(ext - L.second); if (dmin < 0 || d < dmin) dmin = d; }
            if (dmin > 4) continue;
            int at = -2;
            for (int f = e + 1; f <= std::min(e + 3, k); ++f) {
                if (top ? c[f].H > ext : c[f].L < ext) { at = -1; break; }
                if (top ? c[f].C < c[f].O : c[f].C > c[f].O) { at = f; break; }
            }
            if (at != k) continue;
            const int a = std::max(0, e - 29);
            std::map<long long, double> prof, vol;
            for (int i = a; i <= e; ++i) for (const Cell& x : c[i].cells) { prof[x.key] += x.buy - x.sell; vol[x.key] += x.buy + x.sell; }
            std::vector<long long> pres; for (std::map<long long, double>::const_iterator i = vol.begin(); i != vol.end(); ++i) if (i->second > 0) pres.push_back(i->first);
            if (pres.size() < 5) continue;
            const long long span = pres.back() - pres.front(), tpr = std::max<long long>(1, (span + 59) / 60);
            std::map<long long, double> band;
            for (long long j : pres) band[floorDiv(j, tpr)] += prof[j];
            std::vector<double> mags; for (std::map<long long, double>::const_iterator i = band.begin(); i != band.end(); ++i) if (i->second != 0) mags.push_back(std::fabs(i->second));
            if (mags.empty()) continue;
            const double med = median(mags);
            const long long eb = floorDiv(ext, tpr);
            double thin = 0; for (int r = 0; r < 2; ++r) { const std::map<long long, double>::const_iterator q = band.find(top ? eb - r : eb + r); if (q != band.end()) thin += std::fabs(q->second); }
            thin = thin / 2.0 / med;
            auto agg = [&](int q) { double d = 0; for (const Cell& x : c[q].cells) d += x.buy - x.sell; return top ? std::max(0.0, d) : std::max(0.0, -d); };
            const double b2 = agg(e - 2) + agg(e - 3);
            const double fade = b2 > 0 ? (agg(e) + agg(e - 1)) / b2 : 9.0;
            if (thin < 0.35 && fade < 0.5) {
                Event x; x.letter = 'E'; x.k = k; x.pk = e; x.side = top ? -1 : 1; x.lo = ext; x.hi = ext; x.ratio = thin; x.ext = ext; out.push_back(x);
            }
        }
    }
    void detectF(int k, std::vector<Event>& out) {
        for (Event& A : ev) {
            if (A.letter != 'A' || A.faded || !(A.pk < k && k <= A.pk + 20)) continue;
            double d0, v0; region(k, A.lo, A.hi, d0, v0);
            if (v0 <= 0) continue;
            const long long since = c[k].t - 3600;
            const bool sell = A.side == 1;
            std::vector<double> att;
            for (int i = 0; i <= k; ++i) {
                if (c[i].t <= since) continue;
                double d, v; region(i, A.lo, A.hi, d, v);
                if (v > 0) att.push_back(std::max(0.0, sell ? -d : d));
            }
            if (att.size() < 3) continue;
            const size_t h = att.size() / 2; double early = 0, late = 0;
            for (size_t i = 0; i < h; ++i) { early += att[i]; late += att[att.size() - 1 - i]; }
            if (early > 0 && late / early < 0.5) {
                A.faded = true;
                Event e; e.letter = 'F'; e.k = k; e.pk = A.pk; e.side = A.side; e.lo = A.lo; e.hi = A.hi; e.ratio = late / early; out.push_back(e);
            }
        }
    }
    void context(Event& e) {
        const int k = e.k, s = e.side;
        const long long reach = std::max<long long>(4, e.tpr);
        auto isNear = [&](double x) { return e.lo - reach <= x && x <= e.hi + reach; };
        bool lk = false; for (const std::pair<std::string, long long>& L : lv.v) if (isNear(static_cast<double>(L.second))) lk = true;
        e.ctx[LVL_KEY] = lk;
        e.ctx[LVL_VWAP] = !std::isnan(vwap[k]) && isNear(vwap[k]);
        const int kb = std::max(0, k - 10);
        const double slope = std::isnan(vwap[kb]) ? 0.0 : vwap[k] - vwap[kb];
        const bool up = c[k].C > vwap[k] && slope > 0, dn = c[k].C < vwap[k] && slope < 0;   // NaN compares false
        e.ctx[WITH_TREND] = (s == 1 && up) || (s == -1 && dn);
        e.ctx[AGAINST_TREND] = (s == 1 && dn) || (s == -1 && up);
        const int pk = e.pk;
        { bool piv = true; for (int i = std::max(0, pk - 9); i <= pk; ++i) if (s == 1 ? c[i].L < c[pk].L : c[i].H > c[pk].H) piv = false; e.ctx[AT_PIVOT] = piv; }
        bool boUp = false, boDn = false;
        for (int i = std::max(10, k - 14); i <= k; ++i) {
            long long mh = c[i - 10].H, ml = c[i - 10].L; for (int j = i - 10; j < i; ++j) { mh = std::max(mh, c[j].H); ml = std::min(ml, c[j].L); }
            if (c[i].H > mh) boUp = true;
            if (c[i].L < ml) boDn = true;
        }
        e.ctx[BO_WITH] = s == 1 ? boUp : boDn; e.ctx[BO_AGAINST] = s == 1 ? boDn : boUp;
        { long long h3 = c[std::max(0, k - 2)].H, l3 = c[std::max(0, k - 2)].L; for (int i = std::max(0, k - 2); i <= k; ++i) { h3 = std::max(h3, c[i].H); l3 = std::min(l3, c[i].L); }
          e.ctx[MOM_HI] = static_cast<double>(h3 - l3) > 1.5 * atr[k]; }
        // own history: the same letter earlier today, overlapping this node (+ reach); outcomes only when resolved before k
        bool seen = false; int lastRes = -1; int nres = 0, nright = 0;
        for (const Event& h : ev) {
            if (h.letter != e.letter || h.k >= k) continue;
            const bool ov = !(h.hi + reach < e.lo || h.lo - reach > e.hi);
            const int hz = horizonOf(h.letter);
            if (ov) { seen = true; if (h.k + hz < k && h.right >= 0) lastRes = h.right; }
            if (h.k + hz < k && h.right >= 0) { ++nres; nright += h.right; }
        }
        e.ctx[OWN_SEEN] = seen; e.ctx[OWN_FAILED] = lastRes == 0; e.ctx[OWN_GOOD] = nres >= 3 && nright * 2 >= nres;
        // chart history
        if (!lv.v.empty()) {
            const double mid = (e.lo + e.hi) / 2.0; size_t bi = 0; double bd = -1;
            for (size_t i = 0; i < lv.v.size(); ++i) { const double d = std::fabs(lv.v[i].second - mid); if (bd < 0 || d < bd) { bd = d; bi = i; } }
            const std::map<std::string, int>::const_iterator tt = touch.find(lv.v[bi].first);
            e.ctx[TEST1] = (tt == touch.end() || tt->second <= 1) && isNear(static_cast<double>(lv.v[bi].second));
        }
        bool sw = false;
        for (int i = 5; i < k - 4 && !sw; ++i) {
            long long mh = c[i - 5].H, ml = c[i - 5].L; for (int j = i - 5; j <= i + 5; ++j) { mh = std::max(mh, c[j].H); ml = std::min(ml, c[j].L); }
            if (c[i].H >= mh && isNear(static_cast<double>(c[i].H))) sw = true;
            if (c[i].L <= ml && isNear(static_cast<double>(c[i].L))) sw = true;
        }
        e.ctx[SWING_OLD] = sw;
        const int r = r0 < 0 ? 0 : r0;
        long long dh = c[r].H, dl = c[r].L; for (int i = r; i <= k; ++i) { dh = std::max(dh, c[i].H); dl = std::min(dl, c[i].L); }
        const long long rng = std::max<long long>(1, dh - dl);
        e.ctx[AT_EXT] = s == 1 ? (e.lo - dl) <= 0.1 * rng : (dh - e.hi) <= 0.1 * rng;
        e.ctx[OPEN_HR] = k - r < 20;
    }
};

// the record line (the Delta Profile's session file; letter A F E I T AI T1): DI|version|root|letter|side|candle_end_t|pk_end_t|lo|hi|tick|ratio|ctx_bits|status|shown|conf
inline std::string line(const Event& e, const std::string& dltVersion, const std::string& root, long long endT, long long pkEndT, double tick) {
    char b[400];
    const std::string nm = e.letter == 'J' ? "AI" : e.letter == 'U' ? "T1" : std::string(1, e.letter);
    std::snprintf(b, sizeof(b), "DI|%s|%s|%s|%s|%lld|%lld|%lld|%lld|%.10g|%.3f|%u|%d|%d|%s", dltVersion.c_str(), root.c_str(), nm.c_str(),
                  e.side == 1 ? "bullish" : "bearish", endT, pkEndT, e.lo, e.hi, tick, e.ratio, e.bits(), e.status, e.shown ? 1 : 0, e.conf.c_str());
    return b;
}
// (1.1.0) the circle record lines: DC|version|root|side|candle_end_t|lo|hi|tick|ratio|acc  and its mark  DM|version|root|side|circle_end_t|mark_end_t|acc or ai
inline std::string circleLine(const Event& e, const std::string& dltVersion, const std::string& root, long long endT, double tick) {
    char b[300];
    std::snprintf(b, sizeof(b), "DC|%s|%s|%s|%lld|%lld|%lld|%.10g|%.3f|%d|%c", dltVersion.c_str(), root.c_str(), e.side == 1 ? "bullish" : "bearish", endT, e.lo, e.hi, tick, e.ratio, e.acc ? 1 : 0, e.letter);   // (1.2.0) + letter (A / I) at the end
    return b;
}
inline std::string markLine(const Event& e, const std::string& dltVersion, const std::string& root, long long circleEndT, long long markEndT) {
    char b[300];
    std::snprintf(b, sizeof(b), "DM|%s|%s|%s|%lld|%lld|%s", dltVersion.c_str(), root.c_str(), e.side == 1 ? "bullish" : "bearish", circleEndT, markEndT, e.acc ? "acc" : "ai");
    return b;
}
// (1.1.0) the circle label left of the node: "A 3.9x 7746" ("A?" when the market's A gate is not SHOW). Labels are placed top-down, each
// on its own text row: a label that would overlap one already placed (or leave the box) steps down / up one row (<= 3 tries), else the
// circle is drawn without its label. SDK-free so the layout is unit-tested.
struct LBoxI { int l = 0, t = 0, r = 0, b = 0; };
inline bool boxOverlap(const LBoxI& a, const LBoxI& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
inline bool placeLabel(int right, int y, int width, int h, int top, int bottom, int left, std::vector<LBoxI>& placed, int& yOut) {
    if (right - width < left) return false;
    for (int k = 0; k <= 6; ++k) {
        const int yy = y + ((k + 1) / 2) * h * (k % 2 ? 1 : -1);
        if (yy - h / 2 <= top || yy + h / 2 >= bottom) continue;
        const LBoxI bx{ right - width, yy - h / 2, right, yy + h / 2 };
        bool clash = false; for (const LBoxI& q : placed) if (boxOverlap(bx, q)) clash = true;
        if (!clash) { placed.push_back(bx); yOut = yy; return true; }
    }
    return false;
}
// the label in front of the ratio: "I", "I?" (status 1)
// J (A then counter-I) is drawn "A", U (T confirmed) "T": the same letters on the chart, decided later and more surely
inline std::string code(const Event& e) { std::string s(1, e.letter == 'J' ? 'A' : e.letter == 'U' ? 'T' : e.letter); if (e.status == 1) s += "?"; return s; }
}  // namespace dintel
#endif
