// WavePOCLogic.h  1.3.3  (2026-10-29; 1.3.3 = 14-bar extreme, qualifies by AVG OR 3 ATR, direction-coloured spine (Rassul 04:51 / 04:52 CT); 1.3.2 = IRT's ZIGZAG pivots, node = max raw row of its distribution, one profile per start, 1-tick rows, Levels-column clip; 1.3.0 = + CoB legs (Rassul approved 17:27 CT): every CoB leg gets a profile; 1.1.0 = + the liquidity nodes, WavePOCLiq.h, Rassul 11:27 CT; 1.1.1 = liquidity nodes 1.9x + raw-row separation)  -  lsWavePOC "Wave POC": every rule, the files and the layout, SDK-free (unit-tested on Linux).
// Rassul 2026-10-28 (approved 10:59 CT "just build now", mockup wavepoc_v3): per-wave volume-profile nodes as pullback targets.
//   PIVOTS   (1.3.2) IRT's own ZIGZAG, Min Price Change = N BARS (N = "Reversal bars", default 3), Use High/Low: the live leg's extreme
//            moves to every STRICTLY more extreme bar; a bar whose high beats the previous N bars' highs (low: below their lows) reverses
//            a down (up) leg - the old extreme is then a final pivot. (1.3.0: an N-bar fractal, equal = the later.)
//   WAVE     one zigzag leg, both directions. (1.3.3, Rassul 04:51 CT 2026-10-29) A CANDIDATE ends at a 14-bar high (up) / 14-bar low
//            (down) (was 20). It QUALIFIES when its size (points, start pivot -> end pivot) is bigger than the average of the last 20 earlier
//            candidates of that direction (AVG; no look-ahead: only candidates that ended before it count; at least 5 are needed) OR
//            bigger than 3 x ATR (ATR; 14-bar simple ATR of the chart's closed bars ending at the wave's end bar - true range with the
//            previous close, no look-ahead; the plugin had no ATR). The record line says which: AVG / ATR / BOTH (+ the ATR).
//            The candidate store is version 2 (14-bar candidates): an older (20-bar) store is rebuilt from the chart's history. The candidates are kept per market and bar size in lsFlexLevels\WavePOC\<MKT>-<spb>.csv (garbled = rebuilt
//            from the chart's own history).
//   PROFILE  the wave's own volume at price (IRT's VAP, every bar from the start pivot to the end pivot). (1.3.2) ROW SIZE: 1 tick per
//            row in every profile (more than 400 ticks: ceil(range / 400)). 1.3.0 ROW SIZE RULE (all markets): rowTicks = max(1, ceil(wave range in ticks / 80)) -> at most 80 rows; waves of 40+ ticks get 40-80 rows,
//            smaller waves 1 tick per row (ES 37.5 pts = 150 ticks -> 2-tick rows, 76 rows).
//   NODES    (1.3.2) every separate node >= 2.0x the wave's normal row (median non-zero row): a 3-row smoothed peak finds each mini-distribution,
//            the node = the distribution's HIGHEST RAW row; (1.3.0 wording follows) a local peak of the 3-row smoothed volume;
//            between two picked nodes the smoothed valley <= 0.7x the smaller peak and >= 3 rows apart; biggest first, max 5.
//            The biggest = WPOC. Highlight = the bar fill only (WPOC bright amber, others bright light grey; no outline boxes).
//   LINES    from the wave's extreme bar to the right: DASHED while provisional (a new extreme moves the wave), SOLID from the
//            confirmation bar (N bars after the extreme). A line STOPS at the first of: price touches the node (circle); another new
//            qualifying wave confirms (end tick); a close beyond the wave's start pivot (end tick). Confirmed = never repainted
//            (the record file lsFlexLevels\WavePOC\<MKT>-<spb>-nodes.csv redraws recorded waves after a restart).
//   COB LEGS (1.3.0, Rassul approved 17:27 CT 2026-10-28 "every CoB leg automatically gets a Wave POC profile"): a zigzag leg that holds a
//            CoB of its own direction (bull CoB on an up leg, bear on a down leg; the trigger bar inside the leg, start .. end pivot) gets
//            the same profile / nodes / liquidity nodes as a qualifying wave even when it fails the 20-bar / average rule. The CoB is his
//            locked rule = the SAME source as the CoB marks: IRT's RTL custom indicators cob_bull_3m_1b / cob_bear_3m_1b, as lsCobImpulse
//            reads them (host: cobFn per closed bar, captured when the bar is fed - never re-read). Decided when the leg's end pivot is
//            confirmed (N bars after the extreme - no earlier, no provisional CoB leg, never repainted). One profile per CoB (a later
//            extension of the same leg does not draw it again); a leg that is also a qualifying wave is drawn once (kind BOTH).
//            Stops: touch; a later qualifying wave (any direction, as 1.0) or a later CoB leg of the SAME direction; a close beyond the
//            leg's start. Record lines carry the kind WAVE / COB / BOTH (+ the CoB bar time on the WAVE line) for the nightly.
#pragma once
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "WavePOCLiq.h"

namespace wpc {

static const char* const VERSION = "1.6.2";   // (1.6.2, Rassul 09:44-09:54 CT) letters from the top-3 delta nodes: first 66% of the wave = I? (with) / A? (against), last 20% with the wave = T?; cluster A? off, decided when the wave is, no close-back   // was "1.6.1":   // (1.6.1, Rassul 2026-11-02 05:06 CT "the left side of the profile still has a bar") the liquidity-node bars on the left are off too (still computed and recorded)   // was "1.6.0":   // (1.6.0, 2026-11-01 Rassul 21:33 CT "get rid of the volume profile on the left but keep the poc, make it cyan and put it on the right where the wave delta profile is", mockup 21:34) no volume outline / HVN fills on the left; the WPOC a cyan line across the delta side; Pb line + right tag cyan; liquidity nodes stay left   // was "1.5.0":   // (1.5.0, 2026-10-31 Rassul "implement what we have discussed") delta profile on the right (wave delta, delta POC outlined), volume outline + HVN / WPOC + liquidity nodes grouped on the LEFT, 30 px a side, top-2 delta nodes boxed (solid) and classified by location I? / A? / T? + cluster A?, POC pullback Pb (next 3 pullbacks), waves: reversal (14-bar extreme AND > average AND > 3 ATR) OR breakout (> 3 ATR, closes beyond the previous swing) OR rally / decline (> 4 ATR)   // was "1.4.2":   // (1.4.2) right-edge tags restored (1.3.4 behaviour); header bottom-right   // was "1.4.1":   // (1.4.1) host crash fix only (no logic change)   // was "1.4.0":   // (1.4.0, 2026-10-29 Rassul approved 11:18 CT) no right-edge tags; the latest wave's OPEN WPOC goes to lsDealerProfile's Levels column (latestOpenPoc)   // was "1.3.4":   // (1.3.3) 14-bar extreme, AVG or 3 ATR, coloured spine
// 1.3.2:   // (1.3.2) node = the max raw row of its distribution, one profile per start, 1-tick rows, no outline look
// 1.3.0:   // (1.3.0, 2026-10-28 17:27 CT Rassul) every CoB leg gets a profile (kind WAVE / COB / BOTH)
// was 1.2.0:   // (1.2.0, 2026-10-28 16:26 / 16:31 CT Rassul) no node / liquidity lines, circles or end ticks; tags only for OPEN nodes; profile max width x 0.67; liquidity nodes max 40 px
static const double PROF_SCALE = 0.67;   // (1.2.0) the wave profile max width factor
static const double LNODE_W = 40;        // (1.2.0) the liquidity node max width (was 60 px)
#define WPC_VERSION "1.5.0"
// (1.5.0) the profile sides, letters and pullbacks
static const double SIDE_W = 30;        // px each side: volume (left) / delta (right) (Rassul 10-31 22:48 CT "40 px on both sides is too much, reduce to 30")
static const double RALLY_MULT = 4.0;   // a rally / decline wave: bigger than 4 ATR on its own
static const int LET_TOP = 3;   // (1.6.2, Rassul 2026-11-02 09:54 CT "first consider top 3 nodes, then placement")   // was 2:           // (1.5.0, Rassul 2026-11-01 06:35 CT "instead of 4 lets shorten it to 2") the 2 biggest delta rows (each >= 2x the wave median |delta|) are boxed and classified
static const int CL_BAND = 8;           // cluster A: 8-tick delta bands
static const int CL_N = 20, CL_MIN = 10;// cluster A: vs the median of the prior 20 waves (needs 10)
static const double CL_X = 2.0;         // cluster A: band >= 2.0x that median
static const int TRAP_BARS = 5;         // (unused since 1.6.2) T used to need a close back through the node within 5 bars after the end pivot
static const double I_A_ZONE = 0.66;   // (1.6.2, Rassul 09:59 CT "lets make it 66% for initiation and absorbtion") I? / A?: a top-3 node in the first 66% of the wave (with = I, against = A)
static const bool CLUSTER_A = false;   // (1.6.2) the 8-tick cluster A? is off: letters come from the top-3 nodes only
static const double T_ZONE = 0.80;     // (1.6.2, Rassul 2026-11-02 09:44 CT "keep T simple, its an upwave with heavy green delta in the end of the wave near the top (20%) and vice versa") + 09:46 "we should also make it the poc of the wave") T = the wave's biggest with-wave row (green among green in an up wave, red among red in a down wave; 09:51), >= 2x the median, in the last 20%
static const int PB_GAP = 16;   // (1.5.0) Pb one text row away from the bar (the candle code A? / Ae? / D? / S? owns the row next to it)
static const int PB_MAX = 3;            // POC pullback: the next 3 pullback pivots after the wave

// fixed rules (from the approved mockup / reference wp2.py + wp3.py)
static const int LOOKBACK = 14;        // (1.3.3) 14-bar high / low (was 20)
static const int ATR_N = 14;           // (1.3.3) 14-bar ATR (simple mean of the true range) at the wave's end bar
static const double ATR_MULT = 3.0;    // (1.3.3) a wave bigger than 3 ATR qualifies
static const int AVG_N = 20;           // rolling average of the last 20 candidates per direction
static const int MIN_HIST = 5;         // candidates needed before anything qualifies (a fresh market warms up)
static const int MAX_ROWS = 400;       // (1.3.2) row size rule: 1 tick per row (the same row height in every profile) unless > 400 ticks (was 80)
static const int SEP_ROWS = 3;         // nodes >= 3 rows apart
static const int MAX_NODES = 5;
static const int STORE_VER = 2;        // (1.3.3) store version 2 = 14-bar candidates (1 = the 20-bar ones: rebuilt)
static const int STORE_CAP = 1000;     // candidates kept per direction in the file
static const int N_MIN = 2, N_MAX = 10, N_DEF = 3;

enum Stop { OPEN = 0, TOUCH = 1, NEWWAVE = 2, ORIGIN = 3 };
enum QWhy { Q_AVG = 1, Q_ATR = 2, Q_BO = 4, Q_RALLY = 8 };   // (1.5.0) 3 = REVERSAL (AVG and ATR both), BREAKOUT, RALLY (rally / decline)   // (1.3.3) what qualified the wave (bits; 3 = BOTH)
inline const char* qName(int q) { return q == 3 ? "BOTH" : q == Q_BO ? "BREAKOUT" : q == Q_RALLY ? "RALLY" : q == Q_ATR ? "ATR" : q == Q_AVG ? "AVG" : "-"; }
inline int qOf(const std::string& s) { return s == "BOTH" ? 3 : s == "BREAKOUT" ? Q_BO : s == "RALLY" ? Q_RALLY : s == "ATR" ? Q_ATR : s == "AVG" ? Q_AVG : s == "-" ? 0 : -1; }
enum Kind { K_WAVE = 1, K_COB = 2, K_BOTH = 3 };   // (1.3.0) why a leg is drawn
inline const char* kindName(int k) { return k == K_BOTH ? "BOTH" : k == K_COB ? "COB" : "WAVE"; }
inline int kindOf(const std::string& s) { return s == "BOTH" ? K_BOTH : s == "COB" ? K_COB : s == "WAVE" ? K_WAVE : 0; }
inline const char* stopName(int s) { return s == TOUCH ? "touch" : s == NEWWAVE ? "new wave" : s == ORIGIN ? "origin close" : "open"; }

struct Bar { long long t = 0, o = 0, h = 0, l = 0, c = 0; };      // prices in whole ticks; t = IRT bar END, local wall clock seconds
struct VRow { long long tick = 0; long long vol = 0; long long dlt = 0; };   // (1.5.0) dlt = same-price delta (IRT VAP buy - sell volume)
typedef std::function<int(int bar, std::vector<VRow>& out)> VapFn;  // 1 = rows read, 0 = no VAP for that bar (yet), -1 = fault

struct Pivot { int i = 0; int type = 0; long long px = 0; };       // type +1 swing high, -1 swing low

struct Node { long long tick = 0; long long vol = 0; double ratio = 0; int rank = 0; int row = -1; int stop = OPEN; int stopBar = -1; };
// (1.1.0) a liquidity node: average resting real-order lots of its row over the wave; side 1 = mostly bids, 0 = mostly offers
struct LNode { long long tick = 0; double lots = 0; double ratio = 0; int rank = 0; int side = 1; bool nearVol = false; int stop = OPEN; int stopBar = -1; };

struct Wave {
    int dir = 0;                    // +1 up (ends at a swing high), -1 down
    int i0 = 0, i1 = 0, conf = 0;   // start pivot, end pivot (the extreme), confirmation bar (1.3.2: IRT's reversal bar)
    long long t0 = 0, t1 = 0, tConf = 0;
    long long p0 = 0, p1 = 0, size = 0;   // ticks
    double avg = 0; int nHist = 0;        // average size (ticks) of the earlier candidates it beat
    long long e0 = 0; int rowTicks = 1;   // first row's low tick, row height in ticks
    std::vector<long long> rows;          // volume per row (empty = no VAP)
    long long med2 = 0, maxVol = 0;       // 2 x the median non-zero row; the biggest row
    std::vector<Node> nodes;              // rank order: [0] = WPOC
    std::vector<LNode> lnodes;            // (1.1.0) liquidity nodes, [0] = LPOC (empty = no book for this wave)
    bool liqTried = false, liqOk = false;
    bool provisional = false, fromRecord = false, pending = false, recorded = false;
    int vapMissing = 0;                   // bars of the wave IRT had no VAP for
    int scanned = 0;                      // stop rules evaluated up to this bar
    int kind = K_WAVE;                    // (1.3.0) WAVE / COB / BOTH
    long long cobT = 0;                   // (1.3.0) the CoB bar's time (0 = no CoB in the leg)
    std::vector<long long> drows;         // (1.5.0) same-price delta per row (rows aligned)
    long long dMax = 0, dMed2 = 0;        // (1.5.0) the biggest |delta| row; 2 x the median |delta| of the rows that traded
    double clRef = 0;                     // (1.5.0) cluster A reference: median of the prior 20 waves' best mid-wave counter band (0 = too few)
    int qWhy = 0; double atr = 0;         // (1.3.3) AVG / ATR / BOTH (0 = not a qualifying wave); the 14-bar ATR (ticks, 0 = too few bars)
};

// ---------- small helpers ----------
// the chart's market from IRT's root symbol (same rule as DealerLogic.h marketForRoot: "contains", in this order)
inline std::string marketForRoot(const std::string& rootIn) {
    std::string r; for (char c : rootIn) r += (char)((c >= 'a' && c <= 'z') ? c - 32 : c);
    auto has = [&](const char* k) { return r.find(k) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return "";
}
inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
inline int decimalsFor(double tick) {
    if (!(tick > 0)) return 2;
    for (int d = 0; d <= 8; ++d) { const double s = tick * std::pow(10.0, d); if (std::fabs(s - std::llround(s)) < 1e-6) return d; }
    return 8;
}
inline std::string pxText(long long tk, double tick) {
    char b[48]; std::snprintf(b, sizeof b, "%.*f", decimalsFor(tick), (double)tk * tick); return b;
}
inline std::string ratioText(double r) { char b[24]; std::snprintf(b, sizeof b, "%.1fx", r); return b; }
inline std::string tagText(const Node& n, double tick) { return std::string(n.rank == 0 ? "WPOC " : "") + pxText(n.tick, tick) + " " + ratioText(n.ratio); }
inline bool toTicks(double px, double tick, long long& out) {
    if (!std::isfinite(px) || !(tick > 0) || !std::isfinite(tick)) return false;
    const double q = px / tick; if (!std::isfinite(q) || std::fabs(q) > 9e15) return false;
    out = std::llround(q); return true;
}
inline int clampN(long long n) { return n < N_MIN || n > N_MAX ? N_DEF : (int)n; }   // out of range = the default (never garbage)
inline int rowTicksFor(long long rangeTicks) { const long long r = (rangeTicks + MAX_ROWS - 1) / MAX_ROWS; return r < 1 ? 1 : (int)std::min<long long>(r, 1000000); }

// ---------- the profile and its nodes (pure; mirrors wp2.profile / wp2.pick) ----------
// rows from the wave's low to its high in rowTicks steps; a VAP price outside is clipped into the first / last row (as the reference)
inline void buildRows(const std::vector<std::vector<VRow> >& perBar, long long lo, long long hi, int rowTicks, long long& e0, std::vector<long long>& rows) {
    rows.clear(); if (rowTicks < 1 || hi < lo) return;
    e0 = floorDiv(lo, rowTicks) * rowTicks;
    const long long n = (hi - e0) / rowTicks + 1; if (n < 1 || n > 100000) return;
    rows.assign((size_t)n, 0);
    for (const std::vector<VRow>& b : perBar) for (const VRow& v : b) {
        if (v.vol <= 0) continue;
        long long k = floorDiv(v.tick - e0, rowTicks); if (k < 0) k = 0; if (k >= n) k = n - 1;
        rows[(size_t)k] += v.vol;
    }
}
// (1.5.0) the delta per row, same rows as buildRows (clipped the same way)
inline void buildDelta(const std::vector<std::vector<VRow> >& perBar, long long e0, size_t n, int rowTicks, std::vector<long long>& drows) {
    drows.assign(n, 0); if (n == 0 || rowTicks < 1) return;
    for (const std::vector<VRow>& b : perBar) for (const VRow& v : b) {
        if (v.vol <= 0) continue;
        long long k = floorDiv(v.tick - e0, rowTicks); if (k < 0) k = 0; if (k >= (long long)n) k = (long long)n - 1;
        drows[(size_t)k] += v.dlt;
    }
}
inline long long median2(const std::vector<long long>& rows) {     // 2 x the median of the non-zero rows (exact integers)
    std::vector<long long> nz; for (long long v : rows) if (v > 0) nz.push_back(v);
    if (nz.empty()) return 0;
    std::sort(nz.begin(), nz.end()); const size_t m = nz.size();
    return m % 2 ? 2 * nz[m / 2] : nz[m / 2 - 1] + nz[m / 2];
}
// the picked row indexes, biggest first (numpy convolve 'same' with ones(3)/3 = zero-padded 3-row sums; all comparisons in integers)
// (1.3.2, Rassul 18:15 CT "you highlighted the wrong node because the node above it has more volume") the 3-row smoothing only FINDS
// the mini-distributions (a smoothed peak and the rows falling away from it on both sides); the node's row = the HIGHEST RAW row of
// that distribution (ties: the one nearest the smoothed peak), >= 2x the median; separation / valley rules as before (on the smoothed
// peaks); one node per distribution, max 5, biggest raw row first.
inline std::vector<int> pickNodes(const std::vector<long long>& v, long long med2) {
    std::vector<int> out; const int n = (int)v.size(); if (n < 3 || med2 <= 0) return out;
    std::vector<long long> s((size_t)n, 0);
    for (int j = 0; j < n; ++j) s[(size_t)j] = (j > 0 ? v[(size_t)j - 1] : 0) + v[(size_t)j] + (j + 1 < n ? v[(size_t)j + 1] : 0);
    struct Cand { int row; long long ps; };
    std::vector<Cand> cs;
    for (int j = 0; j < n; ++j) {
        if (s[(size_t)j] <= 0) continue;
        if ((j > 0 && s[(size_t)j] < s[(size_t)j - 1]) || (j + 1 < n && s[(size_t)j] < s[(size_t)j + 1])) continue;   // a smoothed peak (edges included)
        int L = j, R = j;
        while (L > 0 && s[(size_t)L - 1] <= s[(size_t)L] && s[(size_t)L - 1] > 0) --L;       // the distribution: rows falling away from the peak
        while (R + 1 < n && s[(size_t)R + 1] <= s[(size_t)R] && s[(size_t)R + 1] > 0) ++R;
        int m = j;
        for (int k = L; k <= R; ++k) if (v[(size_t)k] > v[(size_t)m] || (v[(size_t)k] == v[(size_t)m] && std::abs(k - j) < std::abs(m - j))) m = k;
        if (v[(size_t)m] < med2) continue;                                                     // vol >= 2 x median
        bool dup = false; for (Cand& c : cs) if (c.row == m) { dup = true; c.ps = std::max(c.ps, s[(size_t)j]); }
        if (!dup) cs.push_back(Cand{ m, s[(size_t)j] });
    }
    std::stable_sort(cs.begin(), cs.end(), [&](const Cand& a, const Cand& b) { return v[(size_t)a.row] > v[(size_t)b.row]; });
    std::vector<Cand> took;
    for (const Cand& j : cs) {
        bool ok = true;
        for (const Cand& c : took) {
            if (std::abs(j.row - c.row) < SEP_ROWS) { ok = false; break; }
            const int a = std::min(j.row, c.row), b = std::max(j.row, c.row);
            long long mn = s[(size_t)a]; for (int k = a; k <= b; ++k) mn = std::min(mn, s[(size_t)k]);
            if (10 * mn > 7 * std::min(j.ps, c.ps)) { ok = false; break; }   // valley <= 0.7 x the smaller (smoothed) peak
        }
        if (ok) { took.push_back(j); out.push_back(j.row); }
        if ((int)out.size() == MAX_NODES) break;
    }
    return out;
}

struct Store {
    std::vector<std::pair<long long, long long> > up, dn;   // (end time, size ticks), sorted by time
    int n = N_DEF; long long tickKey = 0;                     // what the sizes were measured with (N, tick in 1e-9 units)
    bool dirty = false;
    std::vector<std::pair<long long, long long> >& of(int dir) { return dir > 0 ? up : dn; }
    const std::vector<std::pair<long long, long long> >& of(int dir) const { return dir > 0 ? up : dn; }
    void add(int dir, long long t, long long size) {
        auto& v = of(dir);
        auto it = std::lower_bound(v.begin(), v.end(), std::make_pair(t, (long long)-1));
        if (it != v.end() && it->first == t) { if (it->second != size) { it->second = size; dirty = true; } return; }
        v.insert(it, std::make_pair(t, size)); dirty = true;
        if ((int)v.size() > STORE_CAP) v.erase(v.begin(), v.begin() + ((long)v.size() - STORE_CAP));
    }
    void remove(int dir, long long t) {
        auto& v = of(dir);
        auto it = std::lower_bound(v.begin(), v.end(), std::make_pair(t, (long long)-1));
        if (it != v.end() && it->first == t) { v.erase(it); dirty = true; }
    }
    // the mean size of the last AVG_N candidates that ENDED BEFORE t (no look-ahead); count = how many were used
    double avgBefore(int dir, long long t, int& count) const {
        const auto& v = of(dir);
        auto it = std::lower_bound(v.begin(), v.end(), std::make_pair(t, (long long)-1));
        long n = (long)(it - v.begin()); const long from = std::max(0L, n - AVG_N);
        double sum = 0; count = 0;
        for (long k = from; k < n; ++k) { sum += (double)v[(size_t)k].second; ++count; }
        return count ? sum / count : 0.0;
    }
    std::string toText(const std::string& mkt, int spb) const {
        std::ostringstream o;
        o << "WAVEPOC_STORE," << STORE_VER << "," << mkt << "," << spb << "," << n << "," << tickKey << "\n";
        for (const auto& e : up) o << "U," << e.first << "," << e.second << "\n";
        for (const auto& e : dn) o << "D," << e.first << "," << e.second << "\n";
        o << "END," << (up.size() + dn.size()) << "\n";
        return o.str();
    }
};
inline bool parseLL(const std::string& s, long long& out) {
    if (s.empty() || s.size() > 20) return false;
    char* e = nullptr; errno = 0; const long long v = std::strtoll(s.c_str(), &e, 10);
    if (!e || *e || errno) return false;
    out = v; return true;
}
inline bool parseD(const std::string& s, double& out) {
    if (s.empty() || s.size() > 40) return false;
    char* e = nullptr; const double v = std::strtod(s.c_str(), &e); if (!e || *e || !std::isfinite(v)) return false; out = v; return true;
}
inline std::vector<std::string> splitCsv(const std::string& s) {
    std::vector<std::string> f; std::string cur;
    for (char c : s) { if (c == ',') { f.push_back(cur); cur.clear(); } else if (c != '\r' && c != '\n') cur += c; }
    f.push_back(cur); return f;
}
// strict: header first (version STORE_VER = 2 from 1.3.3, market, bar size, N, tick must match), every line valid, END count matches; anything else = false
inline bool storeFromText(const std::string& text, const std::string& mkt, int spb, int n, long long tickKey, Store& out, std::string& why) {
    Store s; s.n = n; s.tickKey = tickKey;
    std::istringstream in(text); std::string line; int ln = 0; bool end = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        ++ln; const std::vector<std::string> f = splitCsv(line);
        if (ln == 1) {
            long long v = 0, sp = 0, nn = 0, tk = 0;
            if (f.size() != 6 || f[0] != "WAVEPOC_STORE" || !parseLL(f[1], v)) { why = "bad header"; return false; }
            if (v != STORE_VER) { why = "older store version (20-bar candidates) - rebuilt"; return false; }   // (1.3.3)
            if (f[2] != mkt || !parseLL(f[3], sp) || sp != spb) { why = "other market / bar size"; return false; }
            if (!parseLL(f[4], nn) || nn != n) { why = "other reversal bars (N)"; return false; }
            if (!parseLL(f[5], tk) || tk != tickKey) { why = "other tick size"; return false; }
            continue;
        }
        if (end) { why = "text after END"; return false; }
        if (f.size() == 2 && f[0] == "END") { long long c = 0; if (!parseLL(f[1], c) || c != (long long)(s.up.size() + s.dn.size())) { why = "END count mismatch (torn)"; return false; } end = true; continue; }
        long long t = 0, sz = 0;
        if (f.size() != 3 || (f[0] != "U" && f[0] != "D") || !parseLL(f[1], t) || !parseLL(f[2], sz) || t <= 0 || sz <= 0 || sz > 100000000LL) { why = "garbled line " + std::to_string(ln); return false; }
        auto& v = s.of(f[0] == "U" ? 1 : -1);
        if (!v.empty() && v.back().first >= t) { why = "out of order line " + std::to_string(ln); return false; }
        v.push_back(std::make_pair(t, sz));
    }
    if (ln == 0) { why = "empty"; return false; }
    if (!end) { why = "no END line (torn)"; return false; }
    s.dirty = false; out = s; why = "ok"; return true;
}

// ---------- the node record (append-only; recorded waves are redrawn as recorded) ----------
struct RecWave { int dir = 0; int kind = K_WAVE; int qWhy = 0; long long cobT = 0; int rowTicks = 1; std::vector<Node> nodes; std::vector<LNode> lnodes; };
inline std::string hm(long long t) {      // local wall clock seconds -> "YYYY-MM-DD HH:MM"
    const long long d = floorDiv(t, 86400), s = t - d * 86400;
    long long z = d + 719468; const long long era = floorDiv(z, 146097); const long long doe = z - era * 146097;
    const long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; long long y = yoe + era * 400;
    const long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100); const long long mp = (5 * doy + 2) / 153;
    const long long dd = doy - (153 * mp + 2) / 5 + 1; const long long mm = mp + (mp < 10 ? 3 : -9); if (mm <= 2) ++y;
    char b[96]; std::snprintf(b, sizeof b, "%04lld-%02lld-%02lld %02lld:%02lld", y, mm, dd, s / 3600, (s % 3600) / 60); return b;
}
inline std::string waveLine(const Wave& w, int n, double tick) {
    std::ostringstream o;
    o << "WAVE," << VERSION << "," << n << "," << (w.dir > 0 ? "up" : "down") << "," << w.t0 << "," << w.t1 << "," << w.tConf << "," << hm(w.t0) << "," << hm(w.t1) << ","
      << hm(w.tConf) << "," << pxText(w.p0, tick) << "," << pxText(w.p1, tick) << "," << pxText(w.size, tick) << "," << pxText(std::llround(w.avg), tick) << ","
      << w.nHist << "," << w.rowTicks << "," << w.rows.size() << "," << w.nodes.size() << "," << w.vapMissing << "," << kindName(w.kind) << "," << w.cobT;
    char a[40]; std::snprintf(a, sizeof a, "%.*f", decimalsFor(tick) + 2, w.atr * tick);
    o << "," << qName(w.qWhy) << "," << a;   // (1.3.3) qualified by AVG / ATR / BOTH (- = CoB only), the 14-bar ATR (price)
    return o.str();
}
inline std::string nodeLine(const Wave& w, const Node& nd, double tick) {
    std::ostringstream o; char r[24]; std::snprintf(r, sizeof r, "%.3f", nd.ratio);
    o << "NODE," << w.t0 << "," << w.t1 << "," << nd.rank << "," << pxText(nd.tick, tick) << "," << nd.vol << "," << r << "," << kindName(w.kind);
    return o.str();
}
inline std::string stopLine(const Wave& w, const Node& nd, long long tStop, double tick) {
    std::ostringstream o; o << "STOP," << w.t0 << "," << w.t1 << "," << pxText(nd.tick, tick) << "," << stopName(nd.stop) << "," << tStop << "," << hm(tStop) << "," << kindName(w.kind);
    return o.str();
}
inline std::string lnodeLine(const Wave& w, const LNode& nd, double tick) {
    std::ostringstream o; char r[24], l[32]; std::snprintf(r, sizeof r, "%.3f", nd.ratio); std::snprintf(l, sizeof l, "%.1f", nd.lots);
    o << "LNODE," << w.t0 << "," << w.t1 << "," << nd.rank << "," << pxText(nd.tick, tick) << "," << l << "," << r << "," << (nd.side ? "bid" : "offer") << "," << (nd.nearVol ? "near_volume_node" : "alone") << "," << kindName(w.kind);
    return o.str();
}
inline std::string lstopLine(const Wave& w, const LNode& nd, long long tStop, double tick) {
    std::ostringstream o; o << "LSTOP," << w.t0 << "," << w.t1 << "," << pxText(nd.tick, tick) << "," << stopName(nd.stop) << "," << tStop << "," << hm(tStop) << "," << kindName(w.kind);
    return o.str();
}
// (1.3.2) ROWS,t0,t1,firstRowPrice,rowTicks,vol;vol;... (low to high) - the wave's own volume profile, for the nightly / mockups
inline std::string rowsLine(const Wave& w, double tick) {
    std::ostringstream o; o << "ROWS," << w.t0 << "," << w.t1 << "," << pxText(w.e0, tick) << "," << w.rowTicks << ",";
    for (size_t j = 0; j < w.rows.size(); ++j) o << (j ? ";" : "") << w.rows[j];
    return o.str();
}
static const char* const REC_HEAD = "# WavePOC record: WAVE,ver,N,dir,t0,t1,tConf,start,end,confirmed,startPx,endPx,size,avg,nHist,rowTicks,rows,nodes,vapMissing,kind,cobTime,qualifiedBy,atr14 | NODE,t0,t1,rank,price,vol,ratio,kind | STOP,t0,t1,price,reason,tStop,time,kind | LNODE,t0,t1,rank,price,lots,ratio,bid/offer,near_volume_node/alone,kind | LSTOP,t0,t1,price,reason,tStop,time,kind | ROWS,t0,t1,firstRowPrice,rowTicks,vol;vol;...  (qualifiedBy AVG / ATR / BOTH / - + atr14 from 1.3.3; ROWS from 1.3.2; kind WAVE / COB / BOTH from 1.3.0; older lines have no kind = WAVE)";
// WAVE lines of THIS N and their NODE lines; garbled lines counted and skipped
inline std::map<std::pair<long long, long long>, RecWave> parseRecord(const std::vector<std::string>& lines, int n, double tick, int& bad,
                                                                     std::map<std::pair<long long, long long>, std::vector<std::pair<long long, int> > >* stops = nullptr) {
    std::map<std::pair<long long, long long>, RecWave> m; bad = 0;
    for (const std::string& raw : lines) {
        if (raw.empty() || raw[0] == '#') continue;
        const std::vector<std::string> f = splitCsv(raw);
        if (f[0] == "WAVE") {
            long long nn = 0, t0 = 0, t1 = 0;
            long long ct = 0; int kd = K_WAVE, qw = 0; double at = 0;   // (1.3.0) 21 fields = + kind, CoB time; 19 = an older WAVE line; (1.3.3) 23 = + qualifiedBy, atr14
            if ((f.size() != 19 && f.size() != 21 && f.size() != 23) || !parseLL(f[2], nn) || !parseLL(f[4], t0) || !parseLL(f[5], t1) || (f[3] != "up" && f[3] != "down")) { ++bad; continue; }
            if (f.size() >= 21 && ((kd = kindOf(f[19])) == 0 || !parseLL(f[20], ct) || ct < 0)) { ++bad; continue; }
            if (f.size() == 23 && ((qw = qOf(f[21])) < 0 || !parseD(f[22], at) || at < 0)) { ++bad; continue; }
            if (nn != n) continue;
            RecWave& r = m[std::make_pair(t0, t1)]; r.dir = f[3] == "up" ? 1 : -1; r.kind = kd; r.qWhy = qw; r.cobT = ct; r.nodes.clear(); r.lnodes.clear();
            { long long rt = 1; r.rowTicks = parseLL(f[15], rt) && rt >= 1 && rt <= 1000 ? (int)rt : 1; }   // (1.3.2) the row height it was recorded with
        } else if (f[0] == "NODE") {
            long long t0 = 0, t1 = 0, rank = 0, vol = 0; double px = 0, ratio = 0; long long tk = 0;
            if ((f.size() != 7 && !(f.size() == 8 && kindOf(f[7]))) || !parseLL(f[1], t0) || !parseLL(f[2], t1) || !parseLL(f[3], rank) || !parseD(f[4], px) || !parseLL(f[5], vol) || !parseD(f[6], ratio)
                || !toTicks(px, tick, tk) || rank < 0 || rank >= MAX_NODES) { ++bad; continue; }
            auto it = m.find(std::make_pair(t0, t1)); if (it == m.end()) continue;    // a node of another N (or a torn WAVE line)
            bool dup = false; for (const Node& x : it->second.nodes) dup = dup || x.rank == (int)rank;
            if (dup || (int)rank != (int)it->second.nodes.size()) continue;
            Node nd; nd.tick = tk; nd.vol = vol; nd.ratio = ratio; nd.rank = (int)rank; it->second.nodes.push_back(nd);
        } else if (f[0] == "LNODE") {   // (1.1.0)
            long long t0 = 0, t1 = 0, rank = 0, tk = 0; double px = 0, lots = 0, ratio = 0;
            if ((f.size() != 9 && !(f.size() == 10 && kindOf(f[9]))) || !parseLL(f[1], t0) || !parseLL(f[2], t1) || !parseLL(f[3], rank) || !parseD(f[4], px) || !parseD(f[5], lots) || !parseD(f[6], ratio)
                || !toTicks(px, tick, tk) || rank < 0 || rank >= MAX_NODES || (f[7] != "bid" && f[7] != "offer")) { ++bad; continue; }
            auto it = m.find(std::make_pair(t0, t1)); if (it == m.end()) continue;
            if ((int)rank != (int)it->second.lnodes.size()) continue;
            LNode nd; nd.tick = tk; nd.lots = lots; nd.ratio = ratio; nd.rank = (int)rank; nd.side = f[7] == "bid" ? 1 : 0; nd.nearVol = f[8] == "near_volume_node"; it->second.lnodes.push_back(nd);
        } else if (f[0] == "LSTOP") {
            long long t0 = 0, t1 = 0, ts = 0, tk = 0; double px = 0;
            if ((f.size() != 7 && !(f.size() == 8 && kindOf(f[7]))) || !parseLL(f[1], t0) || !parseLL(f[2], t1) || !parseD(f[3], px) || !toTicks(px, tick, tk) || !parseLL(f[5], ts)) { ++bad; continue; }
        } else if (f[0] == "ROWS") {   // (1.3.2) read by the nightly; checked only
            long long t0 = 0, t1 = 0; if (f.size() != 6 || !parseLL(f[1], t0) || !parseLL(f[2], t1)) { ++bad; continue; }
        } else if (f[0] == "STOP") {
            long long t0 = 0, t1 = 0, ts = 0, tk = 0; double px = 0;
            if ((f.size() != 7 && !(f.size() == 8 && kindOf(f[7]))) || !parseLL(f[1], t0) || !parseLL(f[2], t1) || !parseD(f[3], px) || !toTicks(px, tick, tk) || !parseLL(f[5], ts)) { ++bad; continue; }
            if (stops) (*stops)[std::make_pair(t0, t1)].push_back(std::make_pair(tk, 0));
        } else ++bad;
    }
    return m;
}

// ---------- the engine: closed bars in, waves / nodes / stops out (incremental: each bar is processed once) ----------
struct Counters { long long pb = 0, letters[3] = {0, 0, 0}, clusterA = 0; long long legs = 0, cand[2] = {0, 0}, qual[2] = {0, 0}, stops[4] = {0, 0, 0, 0}, vapMissing = 0, vapFault = 0, recordUsed = 0, superseded = 0, liqWaves = 0, liqNoBook = 0, lnodes = 0, cobBars[2] = {0, 0}, cobOnly = 0, cobBoth = 0, cobRepeat = 0, replaced = 0, qBy[16] = {}; };   // (1.3.3) qBy[AVG=1 / ATR=2 / BOTH=3]: qualifying waves by what qualified them   // (1.3.0) CoB bars seen bull / bear, legs drawn COB / BOTH, legs whose CoB already had a profile

#ifndef WPC_DEFAULT_RULE
#define WPC_DEFAULT_RULE 150   // the plugin build never defines it; the legacy host suite compiles with -DWPC_DEFAULT_RULE=133
#endif
class Engine {
public:
    int N = N_DEF;
    Store* store = nullptr;                                          // shared with the host (persisted there)
    std::map<std::pair<long long, long long>, RecWave> rec;          // recorded waves (redrawn as recorded)
    std::vector<Bar> B;                                              // closed bars, in order
    int zDir = 0, zI = 0, zHi = 0, zLo = 0;                         // (1.3.2) IRT zigzag: live leg (+1 up / -1 down / 0 none), its extreme; first-leg high / low
    std::vector<Pivot> alt;                                          // the zigzag (confirmed pivots)
    std::vector<Wave> waves;                                         // qualified waves, confirmation order
    std::vector<int> confs;                                          // confirmation bars of the qualifying waves (sorted; stop every direction)
    std::vector<int> confsCob[2];                                    // (1.3.0) confirmation bars of COB-only legs, [0] up / [1] down (stop that direction)
    std::vector<signed char> cob;                                    // (1.3.0) per closed bar: 1 bull CoB, 2 bear CoB (3 both), captured when the bar is added
    std::set<long long> cobUsed;                                     // (1.3.0) CoB bar times that already have a profile (one per CoB)
    bool cobLegs = true;                                             // (1.3.0) setting "CoB legs"
    int rule = WPC_DEFAULT_RULE;   // (1.5.0) 150 = reversal / breakout / rally; 133 = the 1.3.3-1.4.2 rule (AVG or 3 ATR at a 14-bar extreme) - legacy suites pin it
    double atrMult = ATR_MULT;                                       // (1.3.3) 3 ATR (fixed; the CoB-only unit tests raise it to isolate the CoB rule)
    std::function<int(int)> cobFn;                                   // (1.3.0) host: the CoB flags of closed bar k (the RTL cob_bull_3m_1b / cob_bear_3m_1b)
    std::vector<std::string> out;                                    // record lines to append (host drains)
    std::vector<long long> clHist;                                   // (1.5.0) each drawn wave's best mid-wave counter-delta band |net| (cluster A reference)
    Counters C;
    double tick = 0.25;
    // (1.1.0) the wave's average resting lots per row + the bid part (host: the once-a-second book samples); false = no book
    std::function<bool(const Wave&, std::vector<double>&, std::vector<double>&)> liqFn;

    void liquidity(Wave& w) {
        w.liqTried = true; w.lnodes.clear(); w.liqOk = false;
        if (!liqFn || w.rows.empty()) return;
        std::vector<double> rows, bids;
        if (!liqFn(w, rows, bids) || rows.size() != w.rows.size()) { if (!w.provisional) ++C.liqNoBook; return; }
        double med = 0; const std::vector<int> pk = wpl::pickRaw(rows, med);
        for (size_t r = 0; r < pk.size(); ++r) {
            const int k = pk[r]; LNode nd; nd.rank = (int)r; nd.lots = rows[(size_t)k]; nd.ratio = med > 0 ? rows[(size_t)k] / med : 0;
            nd.tick = w.e0 + (long long)k * w.rowTicks + w.rowTicks / 2; nd.side = bids[(size_t)k] >= rows[(size_t)k] / 2 ? 1 : 0;
            for (const Node& v : w.nodes) nd.nearVol = nd.nearVol || std::llabs(v.tick - nd.tick) <= 2;
            w.lnodes.push_back(nd);
        }
        w.liqOk = true;
        if (!w.provisional) { ++C.liqWaves; C.lnodes += (long long)w.lnodes.size(); }
    }
    void recordWave(Wave& w) {
        out.push_back(waveLine(w, N, tick)); for (const Node& nd : w.nodes) out.push_back(nodeLine(w, nd, tick));
        out.push_back(rowsLine(w, tick));   // (1.3.2) the wave's volume rows
        for (const LNode& nd : w.lnodes) out.push_back(lnodeLine(w, nd, tick));
        w.recorded = true; RecWave& r = rec[std::make_pair(w.t0, w.t1)]; r.dir = w.dir; r.kind = w.kind; r.qWhy = w.qWhy; r.cobT = w.cobT; r.nodes = w.nodes; r.lnodes = w.lnodes;
    }

    void reset() { clHist.clear(); zDir = 0; zI = zHi = zLo = 0; B.clear(); alt.clear(); waves.clear(); confs.clear(); confsCob[0].clear(); confsCob[1].clear(); cob.clear(); cobUsed.clear(); out.clear(); C = Counters(); }
    int lastBar() const { return (int)B.size() - 1; }

    // (1.3.3) 14-bar ATR at bar i: the simple mean of the true range (high / low vs the previous close) of bars i-13 .. i, closed bars only
    // (the leg is evaluated at its confirmation bar, after i: no look-ahead); false = fewer than 15 bars
    bool atrAt(int i, double& atr) const {
        atr = 0; if (i < ATR_N || i >= (int)B.size()) return false;
        double sum = 0;
        for (int k = i - ATR_N + 1; k <= i; ++k) { const Bar& b = B[(size_t)k]; const long long pc = B[(size_t)k - 1].c; sum += (double)(std::max(b.h, pc) - std::min(b.l, pc)); }
        atr = sum / ATR_N; return true;
    }
    // (1.3.3) AVG (> the direction's average) / ATR (> 3 ATR) / both; 0 = does not qualify  (kept for the 1.3.3 record / unit checks)
    int qualify(Wave& w, int endBar) const {
        int q = 0; if (w.nHist >= MIN_HIST && (double)w.size > w.avg + 1e-9) q |= Q_AVG;
        if (atrAt(endBar, w.atr) && (double)w.size > atrMult * w.atr + 1e-9) q |= Q_ATR;
        return q;
    }
    // (1.5.0, Rassul 2026-10-31 20:21 / 22:56 CT) the wave rule:
    //   REVERSAL (3) = ends at a 14-bar high / low AND bigger than the direction's average AND bigger than 3 ATR (both - the 2:18 profile he circled);
    //   BREAKOUT     = bigger than 3 ATR and a bar of the wave CLOSES beyond the previous swing of its direction (prevPx; LLONG_MIN = none);
    //   RALLY        = bigger than 4 ATR on its own (rally up / decline down; also the session's first big waves, which have no average yet).
    int qualify5(Wave& w, int s0, int endBar, bool cand, long long prevPx) const {
        const bool atr = atrAt(endBar, w.atr);
        const bool big3 = atr && (double)w.size > atrMult * w.atr + 1e-9;
        if (cand && big3 && w.nHist >= MIN_HIST && (double)w.size > w.avg + 1e-9) return Q_AVG | Q_ATR;
        if (big3 && prevPx != LLONG_MIN) for (int k = std::max(0, s0); k <= endBar && k < (int)B.size(); ++k) if (w.dir > 0 ? B[(size_t)k].c > prevPx : B[(size_t)k].c < prevPx) return Q_BO;
        if (atr && (double)w.size > atrMult * (RALLY_MULT / ATR_MULT) * w.atr + 1e-9) return Q_RALLY;   // 4 ATR (scales with atrMult: the CoB-only unit tests raise it)
        return 0;
    }
    // (1.5.0) cluster A: the wave's best MIDDLE band (25-75% of the wave) whose net delta is AGAINST the wave, |net| (0 = none)
    static long long clusterBest(const Wave& w, int* bandLo = nullptr, int* bandHi = nullptr) {
        if (w.drows.empty() || w.rows.size() != w.drows.size()) return 0;
        const long long lo = std::min(w.p0, w.p1), hi = std::max(w.p0, w.p1), rng = hi - lo; if (rng <= 0) return 0;
        const int per = std::max(1, CL_BAND / std::max(1, w.rowTicks));
        long long best = 0;
        for (int a = 0; a < (int)w.drows.size(); a += per) {
            const int b = std::min((int)w.drows.size() - 1, a + per - 1);
            long long net = 0; for (int j = a; j <= b; ++j) net += w.drows[(size_t)j];
            const double c = (double)(w.e0 + (long long)a * w.rowTicks + w.e0 + (long long)(b + 1) * w.rowTicks - 1) / 2.0;
            const double f = w.dir > 0 ? (c - (double)lo) / (double)rng : ((double)hi - c) / (double)rng;
            if (!(f > 0.25 && f < 0.75) || net * w.dir >= 0) continue;
            if (std::llabs(net) > best) { best = std::llabs(net); if (bandLo) *bandLo = a; if (bandHi) *bandHi = b; }
        }
        return best;
    }
    // (1.5.0) the wave's cluster reference from the prior waves (decided once, when its profile is first built), then its own band joins
    void clusterRef(Wave& w) {
        std::vector<long long> h(clHist.size() > (size_t)CL_N ? clHist.end() - CL_N : clHist.begin(), clHist.end());
        w.clRef = 0;
        if ((int)h.size() >= CL_MIN) { std::sort(h.begin(), h.end()); const size_t m = h.size(); w.clRef = m % 2 ? (double)h[m / 2] : ((double)h[m / 2 - 1] + (double)h[m / 2]) / 2.0; }
        if (!w.provisional) { const long long b = clusterBest(w); if (b > 0) clHist.push_back(b); if (clHist.size() > 400) clHist.erase(clHist.begin(), clHist.begin() + 200); }
    }
    // (1.3.3) 14-bar high / low at bar i (the bar itself included; was 20)
    bool is20(int i, int dir) const {
        const int lb = std::max(0, i - (LOOKBACK - 1));
        for (int k = lb; k <= i; ++k) { if (dir > 0 ? B[(size_t)k].h > B[(size_t)i].h : B[(size_t)k].l < B[(size_t)i].l) return false; }
        return true;
    }
    // the profile + nodes of the leg i0 .. i1; false = VAP missing for some bar (pending)
    bool profile(Wave& w, VapFn& vap) {
        long long lo = B[(size_t)w.i0].l, hi = B[(size_t)w.i0].h;
        for (int k = w.i0; k <= w.i1; ++k) { lo = std::min(lo, B[(size_t)k].l); hi = std::max(hi, B[(size_t)k].h); }
        w.rowTicks = rowTicksFor(hi - lo + 1);
        std::vector<std::vector<VRow> > per; per.reserve((size_t)(w.i1 - w.i0 + 1));
        w.vapMissing = 0;
        for (int k = w.i0; k <= w.i1; ++k) {
            std::vector<VRow> r; const int ok = vap ? vap(k, r) : 0;
            if (ok < 0) { ++C.vapFault; ++w.vapMissing; continue; }
            if (ok == 0) { ++w.vapMissing; continue; }
            per.push_back(std::move(r));
        }
        if (w.vapMissing > 0) { w.rows.clear(); return false; }
        buildRows(per, lo, hi, w.rowTicks, w.e0, w.rows);
        w.med2 = median2(w.rows); w.maxVol = 0; for (long long v : w.rows) w.maxVol = std::max(w.maxVol, v);
        buildDelta(per, w.e0, w.rows.size(), w.rowTicks, w.drows);   // (1.5.0) the wave's delta profile
        {   std::vector<long long> a; w.dMax = 0;
            for (size_t j = 0; j < w.drows.size(); ++j) { const long long d = std::llabs(w.drows[j]); w.dMax = std::max(w.dMax, d); if (w.rows[j] > 0) a.push_back(d); }
            w.dMed2 = median2(a); }
        return !w.rows.empty();
    }
    void nodesFromRows(Wave& w) {
        w.nodes.clear();
        const std::vector<int> pk = pickNodes(w.rows, w.med2);
        for (size_t r = 0; r < pk.size(); ++r) {
            Node nd; nd.row = pk[r]; nd.rank = (int)r; nd.vol = w.rows[(size_t)pk[r]];
            nd.tick = w.e0 + (long long)pk[r] * w.rowTicks + w.rowTicks / 2;
            nd.ratio = w.med2 > 0 ? 2.0 * (double)nd.vol / (double)w.med2 : 0; w.nodes.push_back(nd);
        }
    }
    // (1.3.2) a recorded node of an older row height (e.g. 2 ticks): highlight the highest RAW row of the ticks its recorded row covered
    int recRow(const Wave& w, long long tk, int oldRt) const {
        const int r0 = rowOf(w, tk); if (r0 < 0 || oldRt <= w.rowTicks) return r0;
        const long long lo = tk - oldRt / 2, hi = lo + oldRt - 1; int best = r0;
        for (long long t = lo; t <= hi; ++t) { const int r = rowOf(w, t); if (r >= 0 && w.rows[(size_t)r] > w.rows[(size_t)best]) best = r; }
        return best;
    }
    int rowOf(const Wave& w, long long tk) const { if (w.rows.empty()) return -1; const long long k = floorDiv(tk - w.e0, w.rowTicks); return k >= 0 && k < (long long)w.rows.size() ? (int)k : -1; }
    // the stop rules for every open node of w over bars scanned+1 .. upto (touch > new wave > origin close on the same bar)
    void advance(Wave& w, int upto, bool record) {
        for (int k = std::max(w.scanned + 1, w.i1 + 1); k <= upto; ++k) {
            const Bar& b = B[(size_t)k];
            const std::vector<int>& cc = confsCob[w.dir > 0 ? 0 : 1];
            const bool nw = (std::binary_search(confs.begin(), confs.end(), k) || std::binary_search(cc.begin(), cc.end(), k)) && k > w.conf && !w.provisional;   // (1.3.0) + a CoB leg of the same direction
            const bool org = w.dir > 0 ? b.c < w.p0 : b.c > w.p0;
            for (Node& nd : w.nodes) {
                if (nd.stop != OPEN) continue;
                if (b.l <= nd.tick && nd.tick <= b.h) nd.stop = TOUCH; else if (nw) nd.stop = NEWWAVE; else if (org) nd.stop = ORIGIN; else continue;
                nd.stopBar = k;
                if (record) { ++C.stops[nd.stop]; out.push_back(stopLine(w, nd, b.t, tick)); }
            }
            for (LNode& nd : w.lnodes) {   // (1.1.0) the same stop rules
                if (nd.stop != OPEN) continue;
                if (b.l <= nd.tick && nd.tick <= b.h) nd.stop = TOUCH; else if (nw) nd.stop = NEWWAVE; else if (org) nd.stop = ORIGIN; else continue;
                nd.stopBar = k;
                if (record) out.push_back(lstopLine(w, nd, b.t, tick));
            }
        }
        if (upto > w.scanned) w.scanned = upto;
    }
    // a confirmed leg ends at alt.back(); evaluate it at bar k (its confirmation)
    void leg(int k, VapFn& vap) {
        if (alt.size() < 2) return;
        const Pivot& a = alt[alt.size() - 2]; const Pivot& z = alt.back();
        ++C.legs;
        Wave w; w.dir = z.type; w.i0 = a.i; w.i1 = z.i; w.conf = k; w.t0 = B[(size_t)a.i].t; w.t1 = B[(size_t)z.i].t; w.tConf = B[(size_t)k].t;
        w.p0 = a.px; w.p1 = z.px; w.size = w.dir > 0 ? z.px - a.px : a.px - z.px; w.scanned = w.i1;
        const auto rk = rec.find(std::make_pair(w.t0, w.t1));
        const bool cand = is20(z.i, w.dir);
        bool q = false;
        if (cand) {
            ++C.cand[w.dir > 0 ? 0 : 1];
            w.avg = store ? store->avgBefore(w.dir, w.t1, w.nHist) : 0;
            if (store) store->add(w.dir, w.t1, w.size);
        }
        {   // (1.5.0) reversal / breakout / rally; the previous swing of the wave's direction = the pivot before its start pivot
            const long long prevPx = alt.size() >= 3 && alt[alt.size() - 3].type == z.type ? alt[alt.size() - 3].px : LLONG_MIN;
            w.qWhy = rule == 133 ? (cand ? qualify(w, z.i) : (atrAt(z.i, w.atr), 0)) : qualify5(w, a.i, z.i, cand, prevPx); q = w.qWhy != 0;
        }
        // (1.3.0) a CoB of the leg's own direction on a bar of the leg (start .. end pivot) that has no profile yet
        int cobBar = -1;
        if (cobLegs) for (int j = w.i0; j <= w.i1 && j < (int)cob.size(); ++j) {
            if (!(cob[(size_t)j] & (w.dir > 0 ? 1 : 2))) continue;
            if (cobUsed.count(B[(size_t)j].t)) { ++C.cobRepeat; continue; }
            cobBar = j; break;
        }
        const bool recHit = rk != rec.end() && rk->second.dir == w.dir && (cobLegs || (rk->second.kind & K_WAVE));   // CoB legs off: recorded COB-only legs are not drawn
        w.kind = (q ? K_WAVE : 0) | (cobBar >= 0 ? K_COB : 0); w.cobT = cobBar >= 0 ? B[(size_t)cobBar].t : 0;
        if (recHit) { w.kind = rk->second.kind; w.cobT = rk->second.cobT; if (rk->second.qWhy) w.qWhy = rk->second.qWhy; }             // recorded = drawn as recorded (never repainted)
        if (!w.kind) return;
        if (w.cobT > 0) cobUsed.insert(w.cobT);
        if (!(w.kind & K_WAVE)) w.qWhy = 0;
        if (w.kind & K_WAVE) { ++C.qual[w.dir > 0 ? 0 : 1]; ++C.qBy[w.qWhy & 15]; confs.insert(std::upper_bound(confs.begin(), confs.end(), k), k); }
        else { std::vector<int>& cc = confsCob[w.dir > 0 ? 0 : 1]; cc.insert(std::upper_bound(cc.begin(), cc.end(), k), k); }
        if (w.kind == K_COB) ++C.cobOnly; else if (w.kind == K_BOTH) ++C.cobBoth;
        if (recHit && !rk->second.nodes.empty()) {
            w.fromRecord = true; w.recorded = true; w.nodes = rk->second.nodes; w.lnodes = rk->second.lnodes; w.liqTried = true; w.liqOk = !w.lnodes.empty(); ++C.recordUsed;
            for (Node& nd : w.nodes) { nd.stop = OPEN; nd.stopBar = -1; }
            for (LNode& nd : w.lnodes) { nd.stop = OPEN; nd.stopBar = -1; }
            profile(w, vap);                                                             // the histogram only (nodes stay as recorded)
            for (Node& nd : w.nodes) nd.row = recRow(w, nd.tick, rk->second.rowTicks);   // (1.3.2) the max raw row inside the recorded row
            C.vapMissing += w.vapMissing;
        } else if (recHit && !rk->second.lnodes.empty() && profile(w, vap)) {          // (1.3.2) no volume node recorded: nodes now, the recorded book
            w.fromRecord = true; w.recorded = true; nodesFromRows(w); ++C.recordUsed;
            w.lnodes = rk->second.lnodes; w.liqTried = true; w.liqOk = true;
            for (LNode& nd : w.lnodes) { nd.stop = OPEN; nd.stopBar = -1; }
        } else if (profile(w, vap)) {
            nodesFromRows(w); liquidity(w);
        } else { w.pending = true; C.vapMissing += w.vapMissing; }
        if (!w.pending) clusterRef(w);   // (1.5.0)
        if (recHit) w.recorded = true;   // (1.3.2) a recorded wave is never written again (each restart re-wrote every zero-node wave)
        if (!w.pending && !w.recorded) recordWave(w);
        w.scanned = w.i1;
        // (1.3.2, Rassul 18:15 CT the "outline" node) the leg was EXTENDED (an equal / more extreme end pivot later): the earlier wave of the
        // same start and direction is replaced - two profiles stacked at one start drew the old one's highlight under the new one's rows
        const size_t before = waves.size();
        waves.erase(std::remove_if(waves.begin(), waves.end(), [&](const Wave& o) { return o.dir == w.dir && o.i0 == w.i0 && o.i1 < w.i1 && !o.provisional; }), waves.end());
        C.replaced += (long long)(before - waves.size());
        waves.push_back(w);
    }
    // a closed bar (index = B.size()); everything for it is decided now and never again
    void addBar(const Bar& b, VapFn& vap) {
        B.push_back(b); const int k = lastBar();
        { const int f = cobFn ? cobFn(k) : 0; cob.push_back((signed char)(f & 3)); if (f & 1) ++C.cobBars[0]; if (f & 2) ++C.cobBars[1]; }   // (1.3.0) captured once, at the close
        // (1.3.2, Rassul 18:32 CT "why is your zig zag different than irts") IRT's ZIGZAG, Min Price Change = N BARS, Use High/Low
        // (Linnsoft: "only a bar with a high greater than the high of the previous N bars can establish a new pivot and a new upward
        // leg ... if we encounter a bar with a lower low than our pivot bar, that bar replaces the pivot"): the live leg's extreme moves
        // with every STRICTLY more extreme bar (an equal one does not); a bar beyond the previous N bars' opposite extreme reverses the
        // leg - the old extreme is then a final pivot and its leg is evaluated at that bar. Was: an N-bar fractal (N each side, equal =
        // later) - 10-28 ES: 13:36 L instead of IRT's 13:18 L, 10:27 / 12:33 instead of IRT's 10:24 / 12:30 (the doubled profiles).
        if (k >= N) {
            long long ph = LLONG_MIN, pl = LLONG_MAX; for (int j = k - N; j < k; ++j) { ph = std::max(ph, B[(size_t)j].h); pl = std::min(pl, B[(size_t)j].l); }
            const Bar& c = B[(size_t)k];
            auto turn = [&](int type, int at) { Pivot p; p.i = at; p.type = type; p.px = type > 0 ? B[(size_t)at].h : B[(size_t)at].l; alt.push_back(p); if (alt.size() >= 2) leg(k, vap); };
            if (zDir == 0) {
                if (c.h > ph) { turn(-1, zLo); zDir = 1; zI = k; }
                else if (c.l < pl) { turn(+1, zHi); zDir = -1; zI = k; }
            } else if (zDir > 0) {
                if (c.h > B[(size_t)zI].h) zI = k;
                else if (c.l < pl) { turn(+1, zI); zDir = -1; zI = k; }
            } else {
                if (c.l < B[(size_t)zI].l) zI = k;
                else if (c.h > ph) { turn(-1, zI); zDir = 1; zI = k; }
            }
        }
        if (zDir == 0) { if (B[(size_t)k].h > B[(size_t)zHi].h) zHi = k; if (B[(size_t)k].l < B[(size_t)zLo].l) zLo = k; }
        for (Wave& w : waves) if (!w.pending) advance(w, k, true);
        if (alt.size() > 4000) alt.erase(alt.begin(), alt.begin() + 2000);
        if (cobUsed.size() > 20000) cobUsed.erase(cobUsed.begin(), std::next(cobUsed.begin(), 10000));
    }
    // waves whose VAP was missing: try again (host calls this every 10 min); the stops are then replayed from the wave's end
    int retryPending(VapFn& vap) {
        int done = 0;
        for (Wave& w : waves) {
            if (!w.pending) continue;
            if (!profile(w, vap)) continue;
            w.pending = false; nodesFromRows(w); liquidity(w); clusterRef(w); ++done;
            recordWave(w);
            w.scanned = w.i1; advance(w, lastBar(), true);
        }
        return done;
    }
    int pendingCount() const { int n = 0; for (const Wave& w : waves) n += w.pending ? 1 : 0; return n; }

    // the PROVISIONAL wave (live): IRT's live zigzag leg (last final pivot -> the live extreme) while it would qualify; moves with every
    // new extreme; it becomes the confirmed wave when IRT's rule reverses the leg.
    bool provisional(Wave& w, VapFn& vap) {
        w = Wave();
        if (alt.empty() || B.empty()) return false;
        // (1.3.2) IRT's live zigzag leg: from the last final pivot to the live leg's extreme, until IRT's rule reverses it
        const Pivot& a = alt.back(); const int k = lastBar(); const int dir = zDir; const int e = zI;
        if (dir == 0 || dir != -a.type || e <= a.i || e > k) return false;
        w.dir = dir; w.i0 = a.i; w.i1 = e; w.conf = e + N; w.t0 = B[(size_t)a.i].t; w.t1 = B[(size_t)e].t; w.tConf = 0;
        w.p0 = a.px; w.p1 = dir > 0 ? B[(size_t)e].h : B[(size_t)e].l; w.size = dir > 0 ? w.p1 - w.p0 : w.p0 - w.p1; w.provisional = true;
        const bool cand = is20(e, dir);
        w.avg = cand && store ? store->avgBefore(dir, w.t1, w.nHist) : 0;
        {   const long long prevPx = alt.size() >= 2 && alt[alt.size() - 2].type == dir ? alt[alt.size() - 2].px : LLONG_MIN;   // (1.5.0)
            if (rule == 133 && !cand) return false;
            w.qWhy = rule == 133 ? qualify(w, e) : qualify5(w, a.i, e, cand, prevPx); if (!w.qWhy) return false; }
        if (!profile(w, vap)) return false;
        nodesFromRows(w); liquidity(w); clusterRef(w); w.scanned = w.i1; advance(w, k, false);
        return true;
    }
};

// ---------- (1.5.0) the letters on the delta side and the POC pullbacks (pure; decided from closed bars only) ----------
// Rassul 2026-10-31 (21:07 "lets go with top 4", 22:43 "so identify top 4, consider location, then classify"): the wave's 4 biggest
// delta rows (each >= 2x the median |delta| of the rows that traded), each classified by WHERE it sits in the wave and WHICH side made it:
//   I = with the wave, first third (initiation: aggressors pushing away from the turn)                  - the wave's colour
//   A = against the wave, from the start up to the last quarter (a counter-attack that was absorbed)      - the wave's colour
//   T = with the wave, last quarter, and a bar closes back through it within 5 bars after the end pivot  - the opposite colour, one per wave
//   (with the wave in the middle / against it in the last quarter: no letter). The same letter within 10% of the wave (min 8 ticks): the
//   bigger row keeps it. Cluster A (his 12:51 example): the best middle band (8 ticks) against the wave >= 2.0x the median of the prior 20
//   waves' best such band gets an A on its biggest row unless an A is already within 8 ticks. All carry "?" (unproven, the nightly scores them).
// (1.5.0) the wave's 4 biggest delta rows (each >= 2x the median |delta| of the rows that traded), biggest first
inline std::vector<int> topDelta(const Wave& w) {
    std::vector<int> cand; if (w.drows.empty() || w.drows.size() != w.rows.size() || w.dMed2 <= 0) return cand;
    for (int j = 0; j < (int)w.drows.size(); ++j) if (w.drows[(size_t)j] != 0 && std::llabs(w.drows[(size_t)j]) >= w.dMed2) cand.push_back(j);
    std::stable_sort(cand.begin(), cand.end(), [&](int a, int b) { return std::llabs(w.drows[(size_t)a]) > std::llabs(w.drows[(size_t)b]); });
    if ((int)cand.size() > LET_TOP) cand.resize((size_t)LET_TOP);
    return cand;
}
// (1.5.0, Rassul 2026-11-01 05:50 CT "i want the dotted border around the distributions for the top 4 nodes") a node's distribution = the
// rows around it with the SAME sign of delta and at least the median |delta| (half of dMed2), contiguous; [a, b] row range
inline void distOf(const Wave& w, int row, int& a, int& b) {
    a = b = row; if (row < 0 || row >= (int)w.drows.size()) return;
    const long long d = w.drows[(size_t)row]; const long long th = std::max<long long>(1, w.dMed2 / 2);
    auto same = [&](int j) { const long long x = w.drows[(size_t)j]; return (x > 0) == (d > 0) && x != 0 && std::llabs(x) >= th; };
    while (a > 0 && same(a - 1)) --a;
    while (b + 1 < (int)w.drows.size() && same(b + 1)) ++b;
}
struct Letter { int row = -1; char t = 0; bool bull = false; bool cluster = false; int a = -1, b = -1; };   // a..b = the cluster band rows
inline std::vector<Letter> letters(const Wave& w, const std::vector<Bar>& B) {
    std::vector<Letter> out;
    if (w.drows.empty() || w.drows.size() != w.rows.size() || w.dMed2 <= 0) return out;
    const long long lo = std::min(w.p0, w.p1), hi = std::max(w.p0, w.p1), rng = hi - lo; if (rng <= 0) return out;
    auto tickOf = [&](int j) { return w.e0 + (long long)j * w.rowTicks + w.rowTicks / 2; };
    auto posf = [&](long long t) { const double f = w.dir > 0 ? (double)(t - lo) / (double)rng : (double)(hi - t) / (double)rng; return std::max(0.0, std::min(1.0, f)); };
    const std::vector<int> cand = topDelta(w);
    const bool wc = w.dir > 0;   // the wave's colour: green up, red down
    const long long sep = std::max<long long>(8, rng / 10);
    (void)B;   // (1.6.2) T no longer reads the bars after the pivot
    for (int j : cand) {
        const long long d = w.drows[(size_t)j], t = tickOf(j); const bool with = d * w.dir > 0; const double f = posf(t);
        Letter L; L.row = j;
        // (1.6.2, Rassul 09:54 CT) "first consider top 3 nodes, then placement: if its in the first 1/2 it can be absorbtion or initiation,
        // if it is in the last 20% with the wave, consider it to be trapped traders"
        if (f <= I_A_ZONE + 1e-9) { if (with) { L.t = 'I'; L.bull = wc; } else { L.t = 'A'; L.bull = wc; } }   // both in the wave's colour
        else if (with && f >= T_ZONE - 1e-9) { L.t = 'T'; L.bull = !wc; }                                      // the trapped side's opposite colour
        else continue;
        bool dup = false; for (const Letter& o : out) if (o.t == L.t && (L.t == 'T' || std::llabs(tickOf(o.row) - t) <= sep)) dup = true;
        if (!dup) out.push_back(L);
    }
    if (CLUSTER_A && w.clRef > 0) {
        int a = -1, b = -1; const long long best = Engine::clusterBest(w, &a, &b);
        if (best > 0 && (double)best >= CL_X * w.clRef && a >= 0) {
            const long long c0 = tickOf(a) - CL_BAND, c1 = tickOf(b) + CL_BAND; bool near = false;
            for (const Letter& o : out) if (o.t == 'A' && tickOf(o.row) >= c0 && tickOf(o.row) <= c1) near = true;
            if (!near) {
                int pk = -1; for (int j = a; j <= b; ++j) if (w.drows[(size_t)j] * w.dir < 0 && (pk < 0 || std::llabs(w.drows[(size_t)j]) > std::llabs(w.drows[(size_t)pk]))) pk = j;
                if (pk < 0) pk = a;
                Letter L; L.row = pk; L.t = 'A'; L.bull = wc; L.cluster = true; L.a = a; L.b = b; out.push_back(L);
            }
        }
    }
    return out;
}
// (1.5.0, Rassul 2026-10-31 23:02 / 23:05 CT) POC pullback: after an UP wave, any of the next 3 pullback LOWS (down pivots) whose bar trades
// at or below the wave's volume POC while its high stays above it (price tested the POC and held); mirror after a DOWN wave. Returns
// the pivot bars that qualify (confirmed pivots only - never repainted).
inline std::vector<int> pullbacks(const Wave& w, const std::vector<Pivot>& alt, const std::vector<Bar>& B) {
    std::vector<int> out; if (w.nodes.empty() || w.provisional || w.pending) return out;
    const long long poc = w.nodes[0].tick; int seen = 0;
    for (const Pivot& p : alt) {
        if (p.i <= w.i1 || p.type != -w.dir) continue;
        if (++seen > PB_MAX) break;
        if (p.i < 0 || p.i >= (int)B.size()) continue;
        const Bar& b = B[(size_t)p.i];
        if (w.dir > 0 ? (b.l <= poc && b.h > poc) : (b.h >= poc && b.l < poc)) out.push_back(p.i);
    }
    return out;
}

// ---------- the layout (pixels): what the host draws ----------
struct Prim {
    enum K { FILL, BOX, LINE, OVAL, TEXT } k = FILL;
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned long col = 0; int w = 1; std::string text; bool back = true; int bold = 0;
};
static const bool SHOW_VOL_LEFT = false;   // (1.6.0) the left volume profile (outline + HVN / WPOC fills); the WPOC is now cyan on the right
static const bool SHOW_LIQ_LEFT = false;   // (1.6.1) the liquidity-node bars left of the spine: off (computed, recorded, counted in the status file)
struct Colours { unsigned long letterBg = 0x374151; unsigned long prof = 0x334155, amber = 0xF5B428, amberFill = 0xC88C14, white = 0xE5E7EB, whiteFill = 0xB4BECD, zz = 0x78849A, zzBig = 0xCBD5E1, grey = 0x94A3B8, cyan = 0x22D3EE, bid = 0x22C55E, offer = 0xEF4444, bg = 0x0B0F17, up = 0x22C55E, down = 0xEF4444; };   // (1.3.3) up / down = the spine (bullish green, bearish red)
inline unsigned long mix(unsigned long c, unsigned long bg, double a) {   // a of the colour over the background
    unsigned long o = 0; for (int sh = 0; sh <= 16; sh += 8) { const double x = (double)((bg >> sh) & 255) + ((double)((c >> sh) & 255) - (double)((bg >> sh) & 255)) * a; o |= (unsigned long)std::lround(std::max(0.0, std::min(255.0, x))) << sh; }
    return o;
}
struct View {
    int l = 0, t = 0, r = 800, b = 600;           // the pane (price area)
    int first = 0, last = 0;                      // visible bars
    int lastBar = 0;                              // the newest bar on the chart (forming included)
    double bw = 6;                                // pixels per bar
    std::function<double(int)> X;                 // bar -> centre x
    std::function<double(double)> Y;              // price in ticks -> y
    std::function<int(const std::string&, bool)> textW;   // text width in px (bold?)
    long long formH = 0, formL = 0; bool haveForming = false;   // the forming bar (touch shown live)
    bool zigzag = false;
    double tick = 0.25;
    int clipL = -30000, clipR = 30000;            // (1.3.2) nothing drawn left of the Levels column / right of the price area (ColumnBoard)
};
struct Scene { std::vector<Prim> p; int tagsShown = 0, tagsHidden = 0, nodesDrawn = 0, wavesDrawn = 0, lnodesDrawn = 0, lettersDrawn = 0, pbDrawn = 0, boxesDrawn = 0; };

inline int ci(double v) { return v < -30000 ? -30000 : v > 30000 ? 30000 : (int)std::lround(v); }
inline void addLine(Scene& s, double x0, double y0, double x1, double y1, unsigned long c, int w, bool back) { Prim p; p.k = Prim::LINE; p.x0 = ci(x0); p.y0 = ci(y0); p.x1 = ci(x1); p.y1 = ci(y1); p.col = c; p.w = w; p.back = back; s.p.push_back(p); }
inline void addDashed(Scene& s, double x0, double x1, double y, unsigned long c, int w) { for (double x = x0; x < x1; x += 9) addLine(s, x, y, std::min(x + 5, x1), y, c, w, true); }
// (1.5.0) a dotted rectangle (2 px dots, 2 px gaps), front layer

// one wave (1.5.0, Rassul 2026-10-31 mockups "Wave profile redesign" v5-v13): at the wave start x0
//   LEFT  : the volume profile as an OUTLINE (grey, 1 px), only the HVN rows filled (light grey) and the WPOC (amber), the liquidity
//           nodes (bid green / offer red, LPOC bright) grouped with it - all pointing left, 30 px max;
//   RIGHT : the wave's DELTA profile (green buyers / red sellers; rows >= 2x the median |delta| bright, the rest faint), the delta POC
//           outlined in white, 30 px max (clipped at the next profile's start), the letters I? / A? / T? centred on their node;
//   the spine (green up / red down) at x0; POC pullbacks: "Pb" under (after an up wave) / over (after a down wave) the pullback bar,
//   the WPOC extended as an amber dotted line from the wave to it.
// phase 0 = outlines / delta rows / spine of every wave, phase 1 = fills, liquidity, letters, pullbacks (no later profile paints over them)
inline void drawWave(Scene& s, const Wave& w, const View& v, const Colours& C, std::vector<std::pair<const Wave*, const Node*> >& tagNodes, bool /*latest*/, int phase = -1, double xMax = 1e9,
                     const std::vector<Bar>* bars = nullptr, const std::vector<Pivot>* piv = nullptr) {
    const int endShown = v.lastBar;
    std::vector<int> pbs; if (bars && piv) pbs = pullbacks(w, *piv, *bars);
    int far = w.i1; for (const Node& nd : w.nodes) far = std::max(far, nd.stop == OPEN ? endShown : nd.stopBar);
    for (const LNode& nd : w.lnodes) far = std::max(far, nd.stop == OPEN ? endShown : nd.stopBar);
    for (int b : pbs) far = std::max(far, b);
    if (far < v.first || w.i0 > v.last) return;
    if (phase != 1) ++s.wavesDrawn;
    const double x0 = v.X(w.i0) - v.bw * 0.45;
    const double PL = SIDE_W;                                            // the volume side (left)
    double PR = SIDE_W; if (xMax - 3 - x0 < PR) PR = std::max(4.0, xMax - 3 - x0);   // the delta row starts at x0 + 1: ends <= next start - 2   // the delta side (right), clipped at the next start
    auto rowY = [&](int j, double& top, double& bot) {
        const double lo = (double)(w.e0 + (long long)j * w.rowTicks);
        top = v.Y(lo + w.rowTicks - 0.5); bot = v.Y(lo - 0.5);
    };
    auto vw = [&](int j) { return w.maxVol > 0 && j >= 0 && j < (int)w.rows.size() ? PL * (double)w.rows[(size_t)j] / (double)w.maxVol : 0.0; };
    const int nr = (int)w.rows.size();
    const bool haveDelta = !w.drows.empty() && (int)w.drows.size() == nr && w.dMax > 0;
    int dPoc = -1; if (haveDelta) for (int j = 0; j < nr; ++j) if (dPoc < 0 || std::llabs(w.drows[(size_t)j]) > std::llabs(w.drows[(size_t)dPoc])) dPoc = j;
    if (phase != 1) {
        if (SHOW_VOL_LEFT && nr > 0 && w.maxVol > 0) {   // the volume OUTLINE, left (1.6.0: off - Rassul 21:33 CT "get rid of the volume profile on the left but keep the poc")
            for (int j = 0; j < nr; ++j) {
                double top, bot; rowY(j, top, bot); const double wj = vw(j);
                if (wj > 0) addLine(s, x0 - wj, top, x0 - wj, bot, C.grey, 1, true);
                const double wn = j + 1 < nr ? vw(j + 1) : 0.0;                      // the step to the next row (at this row's top)
                if (std::fabs(wn - wj) >= 0.5) addLine(s, x0 - std::max(wj, wn), top, x0 - std::min(wj, wn), top, C.grey, 1, true);
                if (j == 0 && wj > 0) addLine(s, x0 - wj, bot, x0, bot, C.grey, 1, true);
            }
        }
        if (haveDelta) {                // the DELTA rows, right
            for (int j = 0; j < nr; ++j) {
                const long long d = w.drows[(size_t)j]; if (d == 0) continue;
                double top, bot; rowY(j, top, bot);
                const unsigned long base = d > 0 ? C.bid : C.offer;
                const unsigned long col = j == dPoc ? base : std::llabs(d) >= w.dMed2 && w.dMed2 > 0 ? mix(base, C.bg, 0.75) : mix(base, C.bg, 0.3);
                Prim p; p.k = Prim::FILL; p.x0 = ci(x0 + 1); p.x1 = ci(x0 + 1 + PR * (double)std::llabs(d) / (double)w.dMax); p.y0 = ci(top + 0.5); p.y1 = ci(std::max(top + 1.0, bot - 0.5)); p.col = col; p.back = true;
                if (p.x1 > p.x0 && p.y1 >= p.y0) s.p.push_back(p);
            }
        }
        const double ya = v.Y((double)std::min(w.p0, w.p1) - 0.5), yb = v.Y((double)std::max(w.p0, w.p1) + 0.5);
        addLine(s, x0, std::min(ya, yb), x0, std::max(ya, yb), w.dir > 0 ? C.up : C.down, 2, true);   // (1.3.3) the spine
    }
    if (phase == 0) return;
    // the liquidity nodes, grouped with the volume profile on the left (LPOC bright, others 60%)
    if (SHOW_LIQ_LEFT && !w.lnodes.empty() && w.lnodes[0].lots > 0) {
        const double lmx = w.lnodes[0].lots;
        for (const LNode& nd : w.lnodes) {
            const double yc = v.Y((double)nd.tick); if (yc < v.t - 10 || yc > v.b + 10) continue;
            ++s.lnodesDrawn;
            const bool lp = nd.rank == 0; const unsigned long side = nd.side ? C.bid : C.offer;
            const unsigned long fill = lp ? side : mix(side, C.bg, 0.6);
            const long long r0 = w.e0 + floorDiv(nd.tick - w.e0, w.rowTicks) * w.rowTicks;
            const double top = v.Y((double)r0 + w.rowTicks - 0.5), bot = v.Y((double)r0 - 0.5);
            Prim f; f.k = Prim::FILL; f.x1 = ci(x0 - 1); f.x0 = ci(x0 - 1 - PL * std::min(1.0, nd.lots / lmx)); f.y0 = ci(top); f.y1 = ci(std::max(top + 1.0, bot)); f.col = fill; f.back = true;
            if (f.x1 > f.x0) s.p.push_back(f);
        }
    }
    for (const Node& nd : w.nodes) {   // HVN rows (light grey) + WPOC (amber), filled, left
        const bool poc = nd.rank == 0;
        const double yc = v.Y((double)nd.tick);
        if (yc < v.t - 10 || yc > v.b + 10) continue;
        ++s.nodesDrawn;
        if (!SHOW_VOL_LEFT && poc && nd.row >= 0 && nd.row < nr) {   // (1.6.0) the WPOC: a CYAN line across the delta side (right), in front
            double top, bot; rowY(nd.row, top, bot); const double yc = (top + bot) / 2.0;
            Prim f; f.k = Prim::FILL; f.x0 = ci(x0 + 1); f.x1 = ci(x0 + 1 + PR); f.y0 = ci(yc - 1); f.y1 = ci(yc + 1); f.col = C.cyan; f.back = false; s.p.push_back(f);
        }
        if (SHOW_VOL_LEFT && nd.row >= 0 && nd.row < nr && w.maxVol > 0) {
            double top, bot; rowY(nd.row, top, bot);
            const double wj = vw(nd.row) > 0 ? vw(nd.row) : PL * (double)nd.vol / (double)w.maxVol;
            Prim f; f.k = Prim::FILL; f.x1 = ci(x0); f.x0 = ci(x0 - wj); f.y0 = ci(top + 0.5); f.y1 = ci(std::max(top + 1.0, bot - 0.5)); f.col = poc ? C.amberFill : C.whiteFill; f.back = true;
            if (f.x1 > f.x0) s.p.push_back(f);
        }
        const bool liveTouch = nd.stop == OPEN && v.haveForming && v.lastBar > w.scanned && v.formL <= nd.tick && nd.tick <= v.formH;
        if (nd.stop == OPEN && !liveTouch) tagNodes.push_back(std::make_pair(&w, &nd));
    }
    const std::vector<int> top2 = haveDelta ? topDelta(w) : std::vector<int>();
    if (haveDelta && dPoc >= 0 && std::find(top2.begin(), top2.end(), dPoc) == top2.end()) {     // the delta POC outlined in white (when it is a top-2 node its solid box already marks it)
        double top, bot; rowY(dPoc, top, bot);
        Prim b; b.k = Prim::BOX; b.x0 = ci(x0 + 1); b.x1 = ci(x0 + 1 + PR * (double)std::llabs(w.drows[(size_t)dPoc]) / (double)w.dMax); b.y0 = ci(top); b.y1 = ci(std::max(top + 1.0, bot)); b.col = C.white; b.w = 1; b.back = true;
        if (b.x1 > b.x0) s.p.push_back(b);
    }
    // (1.5.0, Rassul 06:35 CT "identify 2 largest nodes, box them (solid box), classify them") a SOLID white box around the distribution of
    // each of the 2 biggest delta nodes (and the cluster A band); was a dotted border round the top 4 (05:50 CT)
    if (haveDelta) {
        auto box = [&](int a, int b) {
            double mx = 0; for (int j = a; j <= b; ++j) mx = std::max(mx, PR * (double)std::llabs(w.drows[(size_t)j]) / (double)w.dMax);
            double t1, b1, t2, b2; rowY(b, t1, b1); rowY(a, t2, b2);
            const double y0 = std::min(t1, b2) - 1, y1 = std::max(t1, b2) + 1;
            if (y1 < v.t || y0 > v.b) return;
            Prim bx; bx.k = Prim::BOX; bx.x0 = ci(x0 + 1); bx.x1 = ci(x0 + 3 + std::max(mx, 8.0)); bx.y0 = ci(y0); bx.y1 = ci(y1); bx.col = C.white; bx.w = 1; bx.back = false;
            s.p.push_back(bx); ++s.boxesDrawn;
        };
        for (int r : top2) { int a, b; distOf(w, r, a, b); box(a, b); }
        if (bars) for (const Letter& L : letters(w, *bars)) if (L.cluster && L.a >= 0) box(L.a, L.b);
    }
    // the letters, centred on their node (front), each on a small background patch so it reads over the row
    if (haveDelta && bars) {
        // (1.6.2) letters on nearby rows (e.g. an I? and an A? 3 ticks apart) never overlap: sorted top to bottom, each at least 13 px
        // below the one above it (the text is 12 px tall)
        std::vector<std::pair<double, Letter> > ys;
        for (const Letter& L : letters(w, *bars)) { double top, bot; rowY(L.row, top, bot); ys.push_back(std::make_pair((top + bot) / 2.0, L)); }
        std::stable_sort(ys.begin(), ys.end(), [](const std::pair<double, Letter>& p, const std::pair<double, Letter>& q) { return p.first < q.first; });
        for (size_t k = 1; k < ys.size(); ++k) if (ys[k].first < ys[k - 1].first + 13) ys[k].first = ys[k - 1].first + 13;
        for (const auto& yl : ys) {
            const Letter& L = yl.second; const double yc = yl.first;
            if (yc < v.t + 7 || yc > v.b - 7) continue;
            const std::string txt = std::string(1, L.t) + "?";
            const int tw = v.textW ? v.textW(txt, true) : 12;
            const double len = PR * (double)std::llabs(w.drows[(size_t)L.row]) / (double)w.dMax;
            const double cx = x0 + 1 + std::max(len / 2.0, tw / 2.0 + 2.0);
            Prim bg; bg.k = Prim::FILL; bg.x0 = ci(cx - tw / 2.0 - 2); bg.x1 = ci(cx + tw / 2.0 + 2); bg.y0 = ci(yc - 6); bg.y1 = ci(yc + 6); bg.col = C.letterBg; bg.back = false; s.p.push_back(bg);   // (1.6.2, Rassul 10:03 CT "use a gray background instead of black for the signals")
            Prim t; t.k = Prim::TEXT; t.x0 = ci(cx - tw / 2.0); t.x1 = ci(cx + tw / 2.0); t.y0 = ci(yc - 7); t.y1 = ci(yc + 7); t.col = L.bull ? C.up : C.down; t.text = txt; t.bold = 1; t.back = false; s.p.push_back(t);
            ++s.lettersDrawn;
        }
    }
    // the POC pullbacks
    if (bars && !w.nodes.empty()) {
        const double yp = v.Y((double)w.nodes[0].tick);
        for (int b : pbs) {
            if (b < v.first || b > v.last) continue;
            addDashed(s, SHOW_VOL_LEFT ? x0 : x0 + 1 + PR, v.X(b), yp, SHOW_VOL_LEFT ? C.amber : C.cyan, 1);   // (1.6.0) cyan from the end of the delta side
            const Bar& br = (*bars)[(size_t)b];
            const std::string txt = "Pb"; const int tw = v.textW ? v.textW(txt, true) : 14; const double cx = v.X(b);
            const double ye = w.dir > 0 ? v.Y((double)br.l - 0.5) + 3 + PB_GAP : v.Y((double)br.h + 0.5) - 3 - 14 - PB_GAP;   // (1.5.0) one row past the bar: lsCandleAbsorb's code sits next to the extreme of the same pivot bar
            Prim t; t.k = Prim::TEXT; t.x0 = ci(cx - tw / 2.0); t.x1 = ci(cx + tw / 2.0); t.y0 = ci(ye); t.y1 = ci(ye + 14); t.col = w.dir > 0 ? C.up : C.down; t.text = txt; t.bold = 1; t.back = false;
            if (t.y0 > v.t && t.y1 < v.b) { s.p.push_back(t); ++s.pbDrawn; }
        }
    }
}

// the right-edge price tags: right of the last bar only (never over candles), stacked without overlap, WPOC first when crowded
inline void drawTags(Scene& s, const View& v, const Colours& C, std::vector<std::pair<const Wave*, const Node*> >& tags) {
    struct T { double y; unsigned long col; bool poc; std::string text; int w; };
    std::vector<T> ts;
    for (const auto& pr : tags) {
        const double y = v.Y((double)pr.second->tick); if (y < v.t + 4 || y > v.b - 4) continue;
        T t; t.y = y; t.poc = pr.second->rank == 0; t.col = t.poc ? (SHOW_VOL_LEFT ? C.amber : C.cyan) : C.white; t.text = tagText(*pr.second, v.tick);
        t.w = v.textW ? v.textW(t.text, t.poc) : (int)t.text.size() * 7; ts.push_back(t);
    }
    if (ts.empty()) return;
    if (ts.size() > 12) { std::stable_sort(ts.begin(), ts.end(), [](const T& a, const T& b) { return a.poc && !b.poc; }); s.tagsHidden += (int)ts.size() - 12; ts.resize(12); }
    std::sort(ts.begin(), ts.end(), [](const T& a, const T& b) { return a.y < b.y; });
    const double xLast = v.X(v.lastBar) + v.bw * 0.5 + 6;
    double last = -1e9; const double LH = 14;
    for (T& t : ts) {
        double ty = std::max(t.y, last + LH);
        if (ty > v.b - 8) { ++s.tagsHidden; continue; }
        const int x1 = v.r - 4, x0 = x1 - t.w;
        if (x0 < xLast + 10) { ++s.tagsHidden; continue; }                          // no room right of the last bar: never over candles
        last = ty;
        addLine(s, xLast, t.y, x0 - 4, ty, t.col, 1, false);
        Prim p; p.k = Prim::TEXT; p.x0 = x0; p.x1 = x1; p.y0 = ci(ty - 7); p.y1 = ci(ty + 7); p.col = t.col; p.text = t.text; p.back = false; p.bold = t.poc ? 1 : 0; s.p.push_back(p);
        ++s.tagsShown;
    }
}

inline void drawZigzag(Scene& s, const Engine& E, const View& v, const Colours& C) {
    std::map<std::pair<int, int>, bool> big; for (const Wave& w : E.waves) big[std::make_pair(w.i0, w.i1)] = true;
    for (size_t k = 0; k < E.alt.size(); ++k) {
        const Pivot& p = E.alt[k];
        if (p.i >= v.first && p.i <= v.last) { Prim o; o.k = Prim::FILL; const double x = v.X(p.i), y = v.Y((double)p.px); o.x0 = ci(x - 2); o.x1 = ci(x + 2); o.y0 = ci(y - 2); o.y1 = ci(y + 2); o.col = C.zzBig; o.back = false; s.p.push_back(o); }
        if (k == 0) continue;
        const Pivot& a = E.alt[k - 1]; if (p.i < v.first || a.i > v.last) continue;
        const bool q = big.count(std::make_pair(a.i, p.i)) > 0;
        addLine(s, v.X(a.i), v.Y((double)a.px), v.X(p.i), v.Y((double)p.px), q ? C.zzBig : C.zz, q ? 2 : 1, false);
    }
}

// the whole picture: the confirmed waves, the provisional one, tags, zigzag, the version header
// (1.4.0) the latest (non-pending, confirmed) wave's WPOC while it is still OPEN: true + its tick; false = none (closed / no wave)
inline bool latestOpenPoc(const Engine& E, long long& tick) {
    const Wave* latest = nullptr; for (const Wave& w : E.waves) if (!w.pending) latest = &w;
    if (!latest || latest->nodes.empty() || latest->nodes[0].stop != OPEN) return false;
    tick = latest->nodes[0].tick; return true;
}
inline Scene layout(const Engine& E, const Wave* prov, const View& v, const Colours& C = Colours()) {
    Scene s; std::vector<std::pair<const Wave*, const Node*> > tags;
    const Wave* latest = nullptr; for (const Wave& w : E.waves) if (!w.pending) latest = &w;
    // (1.3.2) every profile's rows first, then every highlight / liquidity node; each profile clipped at the next start to its right
    std::vector<const Wave*> ws; for (const Wave& w : E.waves) if (!w.pending) ws.push_back(&w);
    if (prov) ws.push_back(prov);
    auto xNext = [&](const Wave* w) { double x = 1e9; for (const Wave* o : ws) if (o->i0 > w->i0) x = std::min(x, v.X(o->i0) - v.bw * 0.45); return x; };
    for (int ph = 0; ph < 2; ++ph)
        for (const Wave* w : ws) drawWave(s, *w, v, C, tags, w == prov || (w == latest && !prov), ph, xNext(w), &E.B, &E.alt);
    drawTags(s, v, C, tags);   // (1.4.2, 2026-10-29 13:02 CT) the right-edge tags are back exactly as 1.3.4 drew them: the "WPOC" Rassul asked for in the Levels column was the WEEKLY POC, not this indicator's   // was (1.4.0): not drawn
    if (v.zigzag) drawZigzag(s, E, v, C);
    // (1.3.2, Rassul 18:24 CT profiles drawn under the Levels column) every primitive kept inside [clipL, clipR]: fills / lines cut
    // (a profile that starts inside the column is clipped, never shifted), text not drawn when it would cross an edge
    {   std::vector<Prim> kept; kept.reserve(s.p.size());
        for (Prim p : s.p) {
            const int a = std::min(p.x0, p.x1), b = std::max(p.x0, p.x1);
            if (p.k == Prim::TEXT || p.k == Prim::OVAL || p.k == Prim::BOX) { if (a < v.clipL || b > v.clipR) continue; kept.push_back(p); continue; }
            const int ca = std::max(a, v.clipL), cb = std::min(b, v.clipR);
            if (cb < ca || (cb == ca && p.k == Prim::FILL)) continue;
            if (p.k == Prim::LINE && p.x0 != p.x1 && p.y0 != p.y1 && (ca != a || cb != b)) continue;   // a slanted line crossing an edge: dropped
            if (p.x0 <= p.x1) { p.x0 = ca; p.x1 = cb; } else { p.x1 = ca; p.x0 = cb; }
            kept.push_back(p);
        }
        s.p.swap(kept);
    }
    // (1.4.2) the version header moved to the BOTTOM-RIGHT of the drawing area: at the top-left it sat on the REV UP / HOD-LOD banner (13:03 screenshot)
    {   Prim h; h.k = Prim::TEXT; h.text = std::string("Wave POC v") + VERSION; const int w = (v.textW ? v.textW(h.text, false) : 100) + 2;
        h.x1 = std::min(v.r, v.clipR) - 4; h.x0 = h.x1 - w; h.y1 = v.b - 3; h.y0 = h.y1 - 13; h.col = C.grey; h.back = false;
        if (h.x0 >= std::max(v.l, v.clipL)) s.p.push_back(h); }
    return s;
}

// ---------- the status file ----------
inline std::string statusText(const Engine& E, const std::string& mkt, int spb, const std::string& parms, const std::string& storeWhy, const std::string& recWhy,
                              int recBad, const Scene* sc, bool provShown, int vapRetries) {
    std::ostringstream o; char a[64];
    o << "VERSION," << VERSION << "\nMARKET," << mkt << ",SECONDS_PER_BAR," << spb << "\nPARMS," << parms << ",reversal bars " << E.N << "\n";
    o << "BARS," << E.B.size() << ",pivots " << E.alt.size() << ",legs " << E.C.legs << "\n";
    o << "WAVES,candidates up " << E.C.cand[0] << " down " << E.C.cand[1] << ",qualified up " << E.C.qual[0] << " down " << E.C.qual[1] << ",superseded " << E.C.superseded
      << ",pending VAP " << E.pendingCount() << ",from record " << E.C.recordUsed << "\n";
    o << "QUALIFIED_BY,reversal " << E.C.qBy[3] << ",breakout " << E.C.qBy[Q_BO] << ",rally/decline " << E.C.qBy[Q_RALLY] << ",(older records AVG " << E.C.qBy[Q_AVG] << " ATR " << E.C.qBy[Q_ATR]
      << "),rule (1.5.0) reversal = " << LOOKBACK << "-bar extreme and > average and > " << ATR_MULT << " x ATR" << ATR_N << " | breakout = > " << ATR_MULT << " x ATR and a close beyond the previous swing | rally / decline = > " << RALLY_MULT << " x ATR\n";
    o << "PROFILE,(1.5.0) left volume outline + HVN / WPOC + liquidity, right wave delta + delta POC outline, " << SIDE_W << " px a side,letters drawn now " << (sc ? sc->lettersDrawn : 0) << ",POC pullbacks drawn now " << (sc ? sc->pbDrawn : 0) << "\n";
    if (E.store) {
        int nu = 0, nd = 0; const long long far = (long long)4e18;
        const double au = E.store->avgBefore(1, far, nu), ad = E.store->avgBefore(-1, far, nd);
        std::snprintf(a, sizeof a, "%.2f", au * E.tick); o << "AVG_SIZE,up " << a << " pts (" << nu << " waves)";
        std::snprintf(a, sizeof a, "%.2f", ad * E.tick); o << ",down " << a << " pts (" << nd << " waves),store " << storeWhy << ",kept up " << E.store->up.size() << " down " << E.store->dn.size() << "\n";
    }
    o << "VAP,missing bars " << E.C.vapMissing << ",faults " << E.C.vapFault << ",retries " << vapRetries << "\n";
    long long nodes = 0; for (const Wave& w : E.waves) nodes += (long long)w.nodes.size();
    o << "NODES,total " << nodes << ",drawn now " << (sc ? sc->nodesDrawn : 0) << ",waves drawn " << (sc ? sc->wavesDrawn : 0) << ",tags " << (sc ? sc->tagsShown : 0)
      << ",tags hidden (no room right of the last bar) " << (sc ? sc->tagsHidden : 0) << ",provisional " << (provShown ? "yes" : "no") << "\n";
    o << "STOPS,touch " << E.C.stops[TOUCH] << ",new wave " << E.C.stops[NEWWAVE] << ",origin close " << E.C.stops[ORIGIN] << "\n";
    o << "LIQUIDITY_NODES,waves with liquidity " << E.C.liqWaves << ",waves without book " << E.C.liqNoBook << ",liquidity nodes " << E.C.lnodes << ",drawn now " << (sc ? sc->lnodesDrawn : 0) << "\n";
    o << "COB_LEGS," << (E.cobLegs ? "on" : "off") << ",CoB bars bull " << E.C.cobBars[0] << " bear " << E.C.cobBars[1] << ",legs drawn COB only " << E.C.cobOnly << " BOTH (also a wave) " << E.C.cobBoth
      << ",CoB already profiled " << E.C.cobRepeat << "\n";
    o << "RECORD," << recWhy << ",bad lines " << recBad << "\n";
    return o.str();
}

} // namespace wpc
