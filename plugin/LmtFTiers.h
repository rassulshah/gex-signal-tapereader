#ifndef LS_LMTFTIERS_H
#define LS_LMTFTIERS_H
// LmtFTiers.h 1.0.0 (2026-10-28, Rassul 23:00 CT "are you overfiltering for the overnight session ... determining what large is based on
// session") -- SDK-free. THE LMT F PRINT = the passive (resting) lots FILLED AT ONE PRICE IN ONE CANDLE, summed over every resting order
// there (his 10-20 definition: "big size at a price level, from one or many orders, not just a big lot"). 0.62.0 counted only the biggest
// SINGLE resting order, and only orders the big-order tracker followed (68+ lots on ES) - overnight nothing qualified.
// TIERS per market x SESSION PART (EVE 17:00-19:00, ASIA 19:00-02:00, LON 02:00-RTH open, OPEN first 30 min, MID, CLOSE last 30 min of RTH,
// POST RTH close-17:00) from the nightly part lmtf_read 2.0.0 (LRA-LmtF-<MKT>.csv PART / FLOOR rows; the old TIER RTH / ON rows are the
// fallback for a part with no row): S20 / S10 / S5 = the size the 20th / 10th / 5th biggest print of that part reaches per session (5
// sessions, causal), never under the FLOOR (the 90th percentile of all prints of the RTH / overnight pool, at least 3 lots).
// ADAPTIVE LIVE FLOOR: after 10 closed candles of the current part with NOTHING at S20, the cut becomes this part's own K-th biggest closed
// print so far (K = 20 x the share of the part elapsed; S10 / S5 at K/2 and K/4), never under the FLOOR - a quiet night still shows its
// biggest prints. No look-ahead: only closed candles of this session part.
// Verified 2026-10-28 (Databento MBO 10 Sep - 22 Oct, 7 markets, tiers from the 5 sessions before each test session): about 15-25 prints
// per session part (outputs/lmtf_fix/res/table.csv).
#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <cmath>
namespace lft {
static const char* const VERSION = "1.0.0";
enum { EVE = 0, ASIA, LON, OPEN, MID, CLOSE, POST, NPART };
static const char* const PART_NAME[NPART] = { "EVE", "ASIA", "LON", "OPEN", "MID", "CLOSE", "POST" };
static const int ADAPT_AFTER = 10;          // closed candles of the part before the adaptive floor may act
static const int K20 = 20;                  // RASSUL (meaning): ~20 / 10 / 5 a session (part)
static const size_t MAX_CELLS = 20000;      // bound
inline int partOf(int mins, int o, int c) {  // mins = Central minute of the candle's START
    if (mins >= 1020 && mins < 1140) return EVE;
    if (mins >= 1140 || mins < 120) return ASIA;
    if (mins < o) return LON;
    if (mins < o + 30) return OPEN;
    if (mins < c - 30) return MID;
    if (mins < c) return CLOSE;
    return POST;
}
inline bool isRth(int p) { return p == OPEN || p == MID || p == CLOSE; }
// the part's first minute and its length in minutes
inline void partSpan(int p, int o, int c, int& start, int& len) {
    switch (p) {
    case EVE: start = 1020; len = 120; break;
    case ASIA: start = 1140; len = 420; break;
    case LON: start = 120; len = std::max(1, o - 120); break;
    case OPEN: start = o; len = 30; break;
    case MID: start = o + 30; len = std::max(1, c - 30 - (o + 30)); break;
    case CLOSE: start = c - 30; len = 30; break;
    default: start = c; len = std::max(1, 1020 - c); break;
    }
}
inline int partByName(const std::string& s) { for (int i = 0; i < NPART; ++i) if (s == PART_NAME[i]) return i; return -1; }
struct Tiers {
    long long part[NPART][3]; bool have[NPART]; long long on[3], rth[3]; long long floorOn = 3, floorRth = 3; bool ok = false, haveOn = false, haveRth = false;
    Tiers() { for (int i = 0; i < NPART; ++i) { have[i] = false; for (int k = 0; k < 3; ++k) part[i][k] = 0; } for (int k = 0; k < 3; ++k) on[k] = rth[k] = 0; }
    // one CSV line: TIER,RTH|ON,a,b,c / PART,<name>,a,b,c / FLOOR,RTH|ON,n (other lines ignored)
    void line(const std::string& ln) {
        std::vector<std::string> f; std::string cur; for (size_t i = 0; i <= ln.size(); ++i) { if (i == ln.size() || ln[i] == ',') { f.push_back(cur); cur.clear(); } else if (ln[i] != '\r' && ln[i] != ' ') cur += ln[i]; }
        if (f.size() >= 5 && (f[0] == "TIER" || f[0] == "PART")) {
            long long v[3]; for (int k = 0; k < 3; ++k) { v[k] = std::atoll(f[2 + k].c_str()); if (v[k] <= 0) return; }
            v[1] = std::max(v[1], v[0]); v[2] = std::max(v[2], v[1]);
            if (f[0] == "TIER") { long long* d = f[1] == "RTH" ? rth : f[1] == "ON" ? on : 0; if (!d) return; for (int k = 0; k < 3; ++k) d[k] = v[k]; (f[1] == "RTH" ? haveRth : haveOn) = true; ok = true; }
            else { const int p = partByName(f[1]); if (p < 0) return; for (int k = 0; k < 3; ++k) part[p][k] = v[k]; have[p] = true; ok = true; }
        } else if (f.size() >= 3 && f[0] == "FLOOR") { const long long n = std::atoll(f[2].c_str()); if (n > 0) { if (f[1] == "RTH") floorRth = n; else if (f[1] == "ON") floorOn = n; } }
    }
    // the file cuts for a part (part row, else the RTH / ON row), each >= the floor; false = none known
    bool cuts(int p, long long out[3]) const {
        const long long* src = (p >= 0 && p < NPART && have[p]) ? part[p] : (isRth(p) ? (haveRth ? rth : 0) : (haveOn ? on : 0));
        if (!src) return false;
        const long long fl = std::max(1LL, isRth(p) ? floorRth : floorOn);
        for (int k = 0; k < 3; ++k) out[k] = std::max(src[k], fl);
        out[1] = std::max(out[1], out[0]); out[2] = std::max(out[2], out[1]); return true;
    }
    long long floorOf(int p) const { return std::max(1LL, isRth(p) ? floorRth : floorOn); }
};
// one print cell: a candle (bar END, Central seconds) x resting side (0 bid, 1 offer) x price (ticks)
struct Cell { long long bar = 0; int side = 0; long long px = 0, lots = 0, big = 0; int orders = 0; };
struct Prints {
    long long spb = 180;
    std::map<std::pair<long long, std::pair<int, long long> >, size_t> at; std::vector<Cell> cells;
    std::map<std::pair<long long, unsigned long long>, long long> perOrd;   // (bar, order id) -> lots filled in that bar (single-order max)
    long long fills = 0, dropped = 0, lastSec = 0;   // lastSec = the newest fill (Central seconds)
    static long long barEnd(long long sec, long long spb) { return (sec / std::max(1LL, spb) + 1) * std::max(1LL, spb); }
    void on(long long sec, int side, long long px, long long size, unsigned long long id) {
        if (sec <= 0 || size <= 0 || px <= 0 || (side != 0 && side != 1)) return;
        const long long b = barEnd(sec, spb); ++fills; if (sec > lastSec) lastSec = sec;
        const std::pair<long long, std::pair<int, long long> > k(b, std::make_pair(side, px));
        std::map<std::pair<long long, std::pair<int, long long> >, size_t>::iterator it = at.find(k);
        if (it == at.end()) { if (cells.size() >= MAX_CELLS) { ++dropped; return; } Cell c; c.bar = b; c.side = side; c.px = px; cells.push_back(c); it = at.insert(std::make_pair(k, cells.size() - 1)).first; }
        Cell& c = cells[it->second]; c.lots += size;
        long long& o = perOrd[std::make_pair(b, id)]; if (o == 0) ++c.orders; o += size; if (o > c.big) c.big = o;
    }
    // keep only candles ending after keepFrom (the caller: the session part start or 60 min, whichever is older)
    void prune(long long keepFrom) {
        bool any = false; for (size_t i = 0; i < cells.size() && !any; ++i) any = cells[i].bar < keepFrom; if (!any) return;
        std::vector<Cell> keep; for (size_t i = 0; i < cells.size(); ++i) if (cells[i].bar >= keepFrom) keep.push_back(cells[i]);
        cells.swap(keep); at.clear(); for (size_t i = 0; i < cells.size(); ++i) at[std::make_pair(cells[i].bar, std::make_pair(cells[i].side, cells[i].px))] = i;
        for (std::map<std::pair<long long, unsigned long long>, long long>::iterator it = perOrd.begin(); it != perOrd.end();) { if (it->first.first < keepFrom) perOrd.erase(it++); else ++it; }
    }
};
// the cuts in force now for part p: the file's (>= floor), or - after ADAPT_AFTER closed candles of this part with nothing at S20 - the
// part's own K-th biggest closed print so far (never under the floor). partStartSec = the part's first candle START (Central seconds).
inline bool liveCuts(const Tiers& T, const Prints& P, int p, long long nowBarEnd, long long partStartSec, int partLenMin, long long out[3], bool& adaptive) {
    adaptive = false;
    const bool have = T.cuts(p, out);
    const long long fl = T.floorOf(p);
    if (!have) { out[0] = out[1] = out[2] = fl; }
    const long long spb = std::max(1LL, P.spb);
    std::vector<long long> v; for (size_t i = 0; i < P.cells.size(); ++i) { const Cell& c = P.cells[i]; if (c.bar - spb >= partStartSec && c.bar < nowBarEnd) v.push_back(c.lots); }   // closed candles of this part
    const long long elapsed = (nowBarEnd - spb - partStartSec) / spb;   // closed candles of the part
    bool any20 = false; for (size_t i = 0; i < v.size(); ++i) if (v[i] >= out[0]) { any20 = true; break; }
    if (elapsed >= ADAPT_AFTER && !any20 && !v.empty()) {
        std::sort(v.begin(), v.end(), std::greater<long long>());
        const double share = std::min(1.0, (double)(elapsed * spb) / (double)std::max(60, partLenMin * 60));
        const int k20 = std::max(1, (int)std::lround(K20 * share)), k10 = std::max(1, k20 / 2), k5 = std::max(1, k20 / 4);
        auto kth = [&](int k) { return std::max(fl, v[(size_t)std::min<long long>((long long)v.size() - 1, k - 1)]); };
        out[0] = kth(k20); out[1] = std::max(out[0], kth(k10)); out[2] = std::max(out[1], kth(k5)); adaptive = true;
    }
    return have || adaptive;
}
}  // namespace lft
#endif
