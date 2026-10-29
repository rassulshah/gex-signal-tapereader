// WavePOCLogic.h  1.3.2  (2026-10-28; 1.3.2 = IRT's ZIGZAG pivots, node = max raw row of its distribution, one profile per start, 1-tick rows, Levels-column clip; 1.3.0 = + CoB legs (Rassul approved 17:27 CT): every CoB leg gets a profile; 1.1.0 = + the liquidity nodes, WavePOCLiq.h, Rassul 11:27 CT; 1.1.1 = liquidity nodes 1.9x + raw-row separation)  -  lsWavePOC "Wave POC": every rule, the files and the layout, SDK-free (unit-tested on Linux).
// Rassul 2026-10-28 (approved 10:59 CT "just build now", mockup wavepoc_v3): per-wave volume-profile nodes as pullback targets.
//   PIVOTS   (1.3.2) IRT's own ZIGZAG, Min Price Change = N BARS (N = "Reversal bars", default 3), Use High/Low: the live leg's extreme
//            moves to every STRICTLY more extreme bar; a bar whose high beats the previous N bars' highs (low: below their lows) reverses
//            a down (up) leg - the old extreme is then a final pivot. (1.3.0: an N-bar fractal, equal = the later.)
//   WAVE     one zigzag leg, both directions. A CANDIDATE ends at a 20-bar high (up) / 20-bar low (down). It QUALIFIES when its size
//            (points) is bigger than the average of the last 20 earlier candidates of that direction ("it should be bigger than the
//            average wave making a new 20 bar high (or low)"). No look-ahead: only candidates that ended before it count; at least 5
//            are needed. The candidates are kept per market and bar size in lsFlexLevels\WavePOC\<MKT>-<spb>.csv (garbled = rebuilt
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

static const char* const VERSION = "1.3.2";   // (1.3.2) node = the max raw row of its distribution, one profile per start, 1-tick rows, no outline look
// 1.3.0:   // (1.3.0, 2026-10-28 17:27 CT Rassul) every CoB leg gets a profile (kind WAVE / COB / BOTH)
// was 1.2.0:   // (1.2.0, 2026-10-28 16:26 / 16:31 CT Rassul) no node / liquidity lines, circles or end ticks; tags only for OPEN nodes; profile max width x 0.67; liquidity nodes max 40 px
static const double PROF_SCALE = 0.67;   // (1.2.0) the wave profile max width factor
static const double LNODE_W = 40;        // (1.2.0) the liquidity node max width (was 60 px)
#define WPC_VERSION "1.3.2"

// fixed rules (from the approved mockup / reference wp2.py + wp3.py)
static const int LOOKBACK = 20;        // 20-bar high / low
static const int AVG_N = 20;           // rolling average of the last 20 candidates per direction
static const int MIN_HIST = 5;         // candidates needed before anything qualifies (a fresh market warms up)
static const int MAX_ROWS = 400;       // (1.3.2) row size rule: 1 tick per row (the same row height in every profile) unless > 400 ticks (was 80)
static const int SEP_ROWS = 3;         // nodes >= 3 rows apart
static const int MAX_NODES = 5;
static const int STORE_CAP = 1000;     // candidates kept per direction in the file
static const int N_MIN = 2, N_MAX = 10, N_DEF = 3;

enum Stop { OPEN = 0, TOUCH = 1, NEWWAVE = 2, ORIGIN = 3 };
enum Kind { K_WAVE = 1, K_COB = 2, K_BOTH = 3 };   // (1.3.0) why a leg is drawn
inline const char* kindName(int k) { return k == K_BOTH ? "BOTH" : k == K_COB ? "COB" : "WAVE"; }
inline int kindOf(const std::string& s) { return s == "BOTH" ? K_BOTH : s == "COB" ? K_COB : s == "WAVE" ? K_WAVE : 0; }
inline const char* stopName(int s) { return s == TOUCH ? "touch" : s == NEWWAVE ? "new wave" : s == ORIGIN ? "origin close" : "open"; }

struct Bar { long long t = 0, o = 0, h = 0, l = 0, c = 0; };      // prices in whole ticks; t = IRT bar END, local wall clock seconds
struct VRow { long long tick = 0; long long vol = 0; };
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
        o << "WAVEPOC_STORE,1," << mkt << "," << spb << "," << n << "," << tickKey << "\n";
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
// strict: header first (version 1, market, bar size, N, tick must match), every line valid, END count matches; anything else = false
inline bool storeFromText(const std::string& text, const std::string& mkt, int spb, int n, long long tickKey, Store& out, std::string& why) {
    Store s; s.n = n; s.tickKey = tickKey;
    std::istringstream in(text); std::string line; int ln = 0; bool end = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        ++ln; const std::vector<std::string> f = splitCsv(line);
        if (ln == 1) {
            long long v = 0, sp = 0, nn = 0, tk = 0;
            if (f.size() != 6 || f[0] != "WAVEPOC_STORE" || !parseLL(f[1], v) || v != 1) { why = "bad header"; return false; }
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
struct RecWave { int dir = 0; int kind = K_WAVE; long long cobT = 0; int rowTicks = 1; std::vector<Node> nodes; std::vector<LNode> lnodes; };
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
static const char* const REC_HEAD = "# WavePOC record: WAVE,ver,N,dir,t0,t1,tConf,start,end,confirmed,startPx,endPx,size,avg,nHist,rowTicks,rows,nodes,vapMissing,kind,cobTime | NODE,t0,t1,rank,price,vol,ratio,kind | STOP,t0,t1,price,reason,tStop,time,kind | LNODE,t0,t1,rank,price,lots,ratio,bid/offer,near_volume_node/alone,kind | LSTOP,t0,t1,price,reason,tStop,time,kind | ROWS,t0,t1,firstRowPrice,rowTicks,vol;vol;...  (ROWS from 1.3.2; kind WAVE / COB / BOTH from 1.3.0; older lines have no kind = WAVE)";
// WAVE lines of THIS N and their NODE lines; garbled lines counted and skipped
inline std::map<std::pair<long long, long long>, RecWave> parseRecord(const std::vector<std::string>& lines, int n, double tick, int& bad,
                                                                     std::map<std::pair<long long, long long>, std::vector<std::pair<long long, int> > >* stops = nullptr) {
    std::map<std::pair<long long, long long>, RecWave> m; bad = 0;
    for (const std::string& raw : lines) {
        if (raw.empty() || raw[0] == '#') continue;
        const std::vector<std::string> f = splitCsv(raw);
        if (f[0] == "WAVE") {
            long long nn = 0, t0 = 0, t1 = 0;
            long long ct = 0; int kd = K_WAVE;   // (1.3.0) 21 fields = + kind, CoB time; 19 = an older WAVE line
            if ((f.size() != 19 && f.size() != 21) || !parseLL(f[2], nn) || !parseLL(f[4], t0) || !parseLL(f[5], t1) || (f[3] != "up" && f[3] != "down")) { ++bad; continue; }
            if (f.size() == 21 && ((kd = kindOf(f[19])) == 0 || !parseLL(f[20], ct) || ct < 0)) { ++bad; continue; }
            if (nn != n) continue;
            RecWave& r = m[std::make_pair(t0, t1)]; r.dir = f[3] == "up" ? 1 : -1; r.kind = kd; r.cobT = ct; r.nodes.clear(); r.lnodes.clear();
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
struct Counters { long long legs = 0, cand[2] = {0, 0}, qual[2] = {0, 0}, stops[4] = {0, 0, 0, 0}, vapMissing = 0, vapFault = 0, recordUsed = 0, superseded = 0, liqWaves = 0, liqNoBook = 0, lnodes = 0, cobBars[2] = {0, 0}, cobOnly = 0, cobBoth = 0, cobRepeat = 0, replaced = 0; };   // (1.3.0) CoB bars seen bull / bear, legs drawn COB / BOTH, legs whose CoB already had a profile

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
    std::function<int(int)> cobFn;                                   // (1.3.0) host: the CoB flags of closed bar k (the RTL cob_bull_3m_1b / cob_bear_3m_1b)
    std::vector<std::string> out;                                    // record lines to append (host drains)
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
        w.recorded = true; RecWave& r = rec[std::make_pair(w.t0, w.t1)]; r.dir = w.dir; r.nodes = w.nodes; r.lnodes = w.lnodes;
    }

    void reset() { zDir = 0; zI = zHi = zLo = 0; B.clear(); alt.clear(); waves.clear(); confs.clear(); confsCob[0].clear(); confsCob[1].clear(); cob.clear(); cobUsed.clear(); out.clear(); C = Counters(); }
    int lastBar() const { return (int)B.size() - 1; }

    // 20-bar high / low at bar i (the bar itself included)
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
            q = w.nHist >= MIN_HIST && (double)w.size > w.avg + 1e-9;
            if (store) store->add(w.dir, w.t1, w.size);
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
        if (recHit) { w.kind = rk->second.kind; w.cobT = rk->second.cobT; }             // recorded = drawn as recorded (never repainted)
        if (!w.kind) return;
        if (w.cobT > 0) cobUsed.insert(w.cobT);
        if (w.kind & K_WAVE) { ++C.qual[w.dir > 0 ? 0 : 1]; confs.insert(std::upper_bound(confs.begin(), confs.end(), k), k); }
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
            w.pending = false; nodesFromRows(w); liquidity(w); ++done;
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
        if (!is20(e, dir)) return false;
        w.avg = store ? store->avgBefore(dir, w.t1, w.nHist) : 0;
        if (!(w.nHist >= MIN_HIST && (double)w.size > w.avg + 1e-9)) return false;
        if (!profile(w, vap)) return false;
        nodesFromRows(w); liquidity(w); w.scanned = w.i1; advance(w, k, false);
        return true;
    }
};

// ---------- the layout (pixels): what the host draws ----------
struct Prim {
    enum K { FILL, BOX, LINE, OVAL, TEXT } k = FILL;
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned long col = 0; int w = 1; std::string text; bool back = true; int bold = 0;
};
struct Colours { unsigned long prof = 0x334155, amber = 0xF5B428, amberFill = 0xC88C14, white = 0xE5E7EB, whiteFill = 0xB4BECD, zz = 0x78849A, zzBig = 0xCBD5E1, grey = 0x94A3B8, bid = 0x22C55E, offer = 0xEF4444, bg = 0x0B0F17; };
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
struct Scene { std::vector<Prim> p; int tagsShown = 0, tagsHidden = 0, nodesDrawn = 0, wavesDrawn = 0, lnodesDrawn = 0; };

inline int ci(double v) { return v < -30000 ? -30000 : v > 30000 ? 30000 : (int)std::lround(v); }
inline void addLine(Scene& s, double x0, double y0, double x1, double y1, unsigned long c, int w, bool back) { Prim p; p.k = Prim::LINE; p.x0 = ci(x0); p.y0 = ci(y0); p.x1 = ci(x1); p.y1 = ci(y1); p.col = c; p.w = w; p.back = back; s.p.push_back(p); }
inline void addDashed(Scene& s, double x0, double x1, double y, unsigned long c, int w) { for (double x = x0; x < x1; x += 9) addLine(s, x, y, std::min(x + 5, x1), y, c, w, true); }

// one wave: profile rows at the wave start pointing right, highlighted nodes, the node lines (dashed until confirmed)
// (1.3.2) phase 0 = the profile rows of every wave, phase 1 = the liquidity nodes + highlights (so no later profile paints over a
// highlight and leaves its edges as an "outline"); xMax = the next profile's start (profiles never overlap)
inline void drawWave(Scene& s, const Wave& w, const View& v, const Colours& C, std::vector<std::pair<const Wave*, const Node*> >& tagNodes, bool /*latest*/, int phase = -1, double xMax = 1e9) {
    const int endShown = v.lastBar;
    // visible? the wave start .. its longest line end
    int far = w.i1; for (const Node& nd : w.nodes) far = std::max(far, nd.stop == OPEN ? endShown : nd.stopBar);
    for (const LNode& nd : w.lnodes) far = std::max(far, nd.stop == OPEN ? endShown : nd.stopBar);
    if (far < v.first || w.i0 > v.last) return;
    if (phase != 1) ++s.wavesDrawn;
    const double x0 = v.X(w.i0) - v.bw * 0.45;
    // (1.2.0, Rassul 16:31 CT "the wave poc lines are too large can you make them about 33% less") the profile's max width x 0.67
    double PW = PROF_SCALE * std::max(4.0, std::min(170.0, (v.X(w.i1) - x0) * 0.95));
    if (xMax - 2 - x0 < PW) PW = std::max(4.0, xMax - 2 - x0);   // (1.3.2) clipped at the next profile's start
    // row j spans ticks [lo - 0.5, lo + rowTicks - 0.5) so the node price (lo + rowTicks / 2) sits inside its own row
    auto rowY = [&](int j, double& top, double& bot) {
        const double lo = (double)(w.e0 + (long long)j * w.rowTicks);
        top = v.Y(lo + w.rowTicks - 0.5); bot = v.Y(lo - 0.5);
    };
    if (phase != 1 && !w.rows.empty() && w.maxVol > 0) {
        for (int j = 0; j < (int)w.rows.size(); ++j) {
            if (w.rows[(size_t)j] <= 0) continue;
            double top, bot; rowY(j, top, bot);
            Prim p; p.k = Prim::FILL; p.x0 = ci(x0); p.x1 = ci(x0 + PW * (double)w.rows[(size_t)j] / (double)w.maxVol); p.y0 = ci(top + 0.5); p.y1 = ci(std::max(top + 1.0, bot - 0.5)); p.col = C.prof; p.back = true;
            if (p.x1 > p.x0 && p.y1 >= p.y0) s.p.push_back(p);
        }
    }
    // (1.1.0) the liquidity nodes: compact bars pointing LEFT from the wave start (max 60 px), LPOC in the bright side colour, the others
    // 60% of it; a faint dotted line to the right (same stop rules); no outline boxes, no price tags
    if (phase == 0) return;
    if (!w.lnodes.empty() && w.lnodes[0].lots > 0) {
        const double LW = LNODE_W, lmx = w.lnodes[0].lots;   // (1.2.0) 60 -> 40 px
        for (const LNode& nd : w.lnodes) {
            const double yc = v.Y((double)nd.tick); if (yc < v.t - 10 || yc > v.b + 10) continue;
            ++s.lnodesDrawn;
            const bool lp = nd.rank == 0; const unsigned long side = nd.side ? C.bid : C.offer;
            const unsigned long fill = lp ? side : mix(side, C.bg, 0.6);
            const long long r0 = w.e0 + floorDiv(nd.tick - w.e0, w.rowTicks) * w.rowTicks;
            const double top = v.Y((double)r0 + w.rowTicks - 0.5), bot = v.Y((double)r0 - 0.5);
            Prim f; f.k = Prim::FILL; f.x1 = ci(x0 - 2); f.x0 = ci(x0 - 2 - LW * std::min(1.0, nd.lots / lmx)); f.y0 = ci(top); f.y1 = ci(std::max(top + 1.0, bot)); f.col = fill; f.back = true;
            if (f.x1 > f.x0) s.p.push_back(f);   // (1.2.0, Rassul 16:26 CT "the chart is extremely messy") no dotted line across the chart
        }
    }
    for (const Node& nd : w.nodes) {
        const bool poc = nd.rank == 0;
        const double yc = v.Y((double)nd.tick);
        if (yc < v.t - 10 || yc > v.b + 10) continue;
        ++s.nodesDrawn;
        // the highlighted node: its bar fill only (Rassul 11:21 CT: no outline boxes) - WPOC bright amber, others bright light grey
        if (nd.row >= 0 && !w.rows.empty() && w.maxVol > 0) {
            double top, bot; rowY(nd.row, top, bot);
            // (1.3.2) exactly its own row (same y as the profile row, no +-1 px band) and its own row's length - a filled row, never an edge
            const long long rv = nd.row < (int)w.rows.size() && w.rows[(size_t)nd.row] > 0 ? w.rows[(size_t)nd.row] : nd.vol;
            Prim f; f.k = Prim::FILL; f.x0 = ci(x0); f.x1 = ci(x0 + PW * (double)rv / (double)w.maxVol); f.y0 = ci(top + 0.5); f.y1 = ci(std::max(top + 1.0, bot - 0.5)); f.col = poc ? C.amberFill : C.whiteFill; f.back = true;
            if (f.x1 > f.x0) s.p.push_back(f);
        }
        // (1.2.0, Rassul 16:26 CT "remove the drawing of levels across the chart and from the wave pocs") no node line, no touch circle,
        // no end tick; the right-edge tag only while the node is still OPEN (untouched - the forming bar included -, wave not replaced,
        // origin not broken); the stop rules still decide and record open / closed
        const bool liveTouch = nd.stop == OPEN && v.haveForming && v.lastBar > w.scanned && v.formL <= nd.tick && nd.tick <= v.formH;
        if (nd.stop == OPEN && !liveTouch) tagNodes.push_back(std::make_pair(&w, &nd));
    }
}

// the right-edge price tags: right of the last bar only (never over candles), stacked without overlap, WPOC first when crowded
inline void drawTags(Scene& s, const View& v, const Colours& C, std::vector<std::pair<const Wave*, const Node*> >& tags) {
    struct T { double y; unsigned long col; bool poc; std::string text; int w; };
    std::vector<T> ts;
    for (const auto& pr : tags) {
        const double y = v.Y((double)pr.second->tick); if (y < v.t + 4 || y > v.b - 4) continue;
        T t; t.y = y; t.poc = pr.second->rank == 0; t.col = t.poc ? C.amber : C.white; t.text = tagText(*pr.second, v.tick);
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
inline Scene layout(const Engine& E, const Wave* prov, const View& v, const Colours& C = Colours()) {
    Scene s; std::vector<std::pair<const Wave*, const Node*> > tags;
    const Wave* latest = nullptr; for (const Wave& w : E.waves) if (!w.pending) latest = &w;
    // (1.3.2) every profile's rows first, then every highlight / liquidity node; each profile clipped at the next start to its right
    std::vector<const Wave*> ws; for (const Wave& w : E.waves) if (!w.pending) ws.push_back(&w);
    if (prov) ws.push_back(prov);
    auto xNext = [&](const Wave* w) { double x = 1e9; for (const Wave* o : ws) if (o->i0 > w->i0) x = std::min(x, v.X(o->i0) - v.bw * 0.45); return x; };
    for (int ph = 0; ph < 2; ++ph)
        for (const Wave* w : ws) drawWave(s, *w, v, C, tags, w == prov || (w == latest && !prov), ph, xNext(w));
    drawTags(s, v, C, tags);
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
    Prim h; h.k = Prim::TEXT; h.text = std::string("Wave POC v") + VERSION; h.x0 = std::max(v.l, v.clipL) + 4; h.y0 = v.t + 22; h.x1 = h.x0 + (v.textW ? v.textW(h.text, false) : 100) + 2; h.y1 = h.y0 + 13; h.col = C.grey; h.back = false; s.p.push_back(h);
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
