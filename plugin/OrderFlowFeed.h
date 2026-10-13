#ifndef LS_ORDERFLOWFEED_H
#define LS_ORDERFLOWFEED_H
// OrderFlowFeed.h (1.0.0, 2026-10-13) -- the ONE shared order-flow header of lsBidAskStats 1.3.0, lsPullStats 1.0.0 and
// lsStackStats 1.0.0. Rassul 2026-10-13: "i want to identify spikes" / "number of bids and ask pulled like a histogram" /
// "add a stacking indicator" / "keep seperate" / "include L for Bid Ask Stacks and Pull" / "why dont you just use a dash in the bar".
//
// PART 1 (SDK-free, unit-tested on Linux): the order-by-order (DBO / MBO) engine - every NEW / MOD / DEL / FILL classified into
//   per-candle numbers near price: orders PULLED, orders ADDED, REFILLS (iceberg reloads), ICEBERG confirmation, aggressor SWEEPS
//   and LARGE lots; the session size distributions behind the "large" thresholds; the per-market params file; the stats as of each
//   bar (ratio / spike, as lsBidAskStats 1.2.0); letters / bars / two-line layout; the per-session record file.
// PART 2 (#ifdef OFF_SDK, after irtsdk.h): the DEPTH_BY_ORDER connection and drain exactly as lsMboCheck 1.1.0 (one object per
//   chart kept alive, opened from calc / draw only behind a crash-guard file, <= 5,000 events or ~8 ms per call with the backlog
//   carried, bDboSubscribed through a pointer-to-member), the shared drawing, and MboPane<L> - the whole host of the two MBO panes.
//
// THE FEED (IRT DEPTH_BY_ORDER, checked on his own recordings for lsPulling 0.2.x): NEW = an order rests (buySell 1 = a bid, 0 = an
// offer); FILL = part or all of a RESTING order traded (orderID = the resting order, aggressorOrderID = the order that hit it, size =
// lots filled); then the resting order is DELeted (all filled) or MODified (the rest); DEL = the order left the book; MOD = a new
// price or a new size.
//   PULLED (who is LEAVING):  DEL of an order whose last event was not a FILL           -> +1 order, + its resting size
//                             DEL of an order never seen (rested before load)           -> +1 order, + the DEL's size
//                             MOD to a new price, the OLD price near                    -> +1 order, + its old size
//                             MOD to a smaller size at the same price (not after a fill)-> contracts only (a partial pull)
//                             DEL right after a FILL of that order                      -> NOT a pull (the trade took it)
//   ADDED (who is ARRIVING):  NEW near; MOD to a new price, the NEW price near          -> +1 order, + its size
//                             MOD to a bigger size at the same price                    -> contracts only
//   REFILL:                   after a FILL at a price, a NEW (or a size increase) on the SAME side at the SAME price within
//                             refillWindowMs (default 1000) -> +1 refill. Checked on the arrival clock AND on the event stamps
//                             (whole seconds, at most ceil(window / 1 s) apart) so a drained backlog cannot squeeze old events in.
//   ICEBERG confirmed:        at a refill price, the bar's traded volume there > the most ever DISPLAYED there in the bar
//                             (a price where a fill hit an order never seen is not judged).
//   SWEEP:                    one aggressorOrderID filling at >= sweepLevels (3) distinct prices in the candle.
//   LARGE aggressive order:   one aggressorOrderID's filled lots in the candle >= largeLotAggr.
//   LARGE resting order:      one order pulled (PullStats) / added (StackStats) with size >= largeLotResting.
//   Large thresholds: 0 in the params file = auto = the largeLotPct (99th) percentile of the sizes seen THIS SESSION BEFORE the
//   candle (aggressor orders / resting orders near price), never under largeLotMin (20) lots. No look-ahead.
//   NEAR = |price - the last trade price| <= the Liquidity Profile 0.14.0 near band (NearLevels.h).
#include "NearLevels.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

namespace off {

static const char* const VERSION = "1.0.0";
typedef long long Tick;
enum Side { BID = 0, ASK = 1 };                    // BID = resting buy orders, ASK = resting sell orders
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
using nl::finite;

static const size_t BOOK_CAP = 200000;             // tracked orders at most (far ones dropped first)
static const size_t PER_ORDER_CAP = 50000;         // per bar per side: orders tracked for the concentration line
static const size_t ICE_CAP = 2000;                // per bar per side: prices tracked for the iceberg check
static const size_t FILLMEM_CAP = 5000;            // per side: recent fill prices remembered for refills
static const size_t AGGR_CAP = 50000;              // per bar: aggressor orders tracked
static const size_t REST_SIZES_CAP = 20000;        // per bar: resting sizes kept for the session distribution
static const int AGGR_PX_MAX = 24;                 // distinct prices remembered per aggressor order (more = "24+")
static const long long SIZE_MAX_SANE = 10000000;   // a size above this is garbage
static const long long REF_STALE_MS = 5000;        // no fill for 5 s: the chart's close is the last trade price

inline bool toTick(double price, double tick, Tick& out) {
    if (!finite(price) || !finite(tick) || tick <= 0) return false;
    const double q = price / tick;
    if (!finite(q) || q > 9.0e15 || q < -9.0e15) return false;
    out = (Tick)(q >= 0 ? std::floor(q + .5) : std::ceil(q - .5));
    return true;
}

// =====================================================================================================================
// PARAMS: %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-OrderFlow-<MKT>.csv, written by the nightly (format: NOTES.md)
//   VERSION,1 / MARKET,<MKT> / PARAM,<name>,<value> ... ('#' lines and blank lines ignored). Missing / bad -> defaults, each
//   reason listed in the status file. A bad VERSION / MARKET = the whole file ignored; a bad value = that one default.
// =====================================================================================================================
struct Params {
    long long sweepLevels = 3, largeLotAggr = 0, largeLotResting = 0, largeLotPct = 99, largeLotMin = 20;
    long long refillWindowMs = 1000, refillNotableT = 20;                 // tenths: 20 = 2.0x
    long long spikeAvgT = 20, spikeJumpT = 15, spikeLookback = 10;        // tenths / tenths / bars
    long long closeTicks = 2, baitPct = 70;
    std::set<std::string> fromFile;                                       // the names the file set (valid)
    bool file(const char* n) const { return fromFile.count(n) > 0; }
};
struct ParamDef { const char* name; bool tenths; long long lo, hi; long long Params::* slot; };
static const ParamDef PARAM_DEFS[] = {
    { "sweepLevels", false, 2, 20, &Params::sweepLevels },
    { "largeLotAggr", false, 0, 100000, &Params::largeLotAggr },
    { "largeLotResting", false, 0, 100000, &Params::largeLotResting },
    { "largeLotPct", false, 50, 100, &Params::largeLotPct },
    { "largeLotMin", false, 1, 100000, &Params::largeLotMin },
    { "refillWindowMs", false, 50, 60000, &Params::refillWindowMs },
    { "refillNotableX", true, 10, 100, &Params::refillNotableT },
    { "spikeAvgX", true, 0, 100, &Params::spikeAvgT },
    { "spikeJumpX", true, 0, 100, &Params::spikeJumpT },
    { "spikeLookback", false, 0, 50, &Params::spikeLookback },
    { "closeTicks", false, 0, 20, &Params::closeTicks },
    { "baitPct", false, 50, 100, &Params::baitPct },
};
static const size_t PARAM_COUNT = sizeof(PARAM_DEFS) / sizeof(PARAM_DEFS[0]);
inline std::string tenthsText(long long t) { char b[32]; std::snprintf(b, sizeof(b), "%lld.%lldx", t / 10, t % 10); return b; }
struct ParamLoad { Params p; std::vector<std::string> notes; std::string source = "defaults"; };
inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}
inline std::vector<std::string> csv(const std::string& line) {
    std::vector<std::string> c; size_t a = 0;
    for (;;) { const size_t p = line.find(',', a); c.push_back(trim(line.substr(a, p == std::string::npos ? std::string::npos : p - a))); if (p == std::string::npos) break; a = p + 1; }
    return c;
}
// fileFound = false: no file (defaults). lines = the file's lines (bounded read done by the caller).
inline ParamLoad parseParams(bool fileFound, bool readOk, const std::vector<std::string>& lines, const std::string& market) {
    ParamLoad L;
    if (!fileFound) { L.source = "defaults (no params file)"; return L; }
    if (!readOk) { L.source = "defaults (params file unreadable or too big)"; L.notes.push_back("file: unreadable / over 64 KB / over 400 lines"); return L; }
    bool haveVer = false, haveMkt = false; Params p; std::vector<std::string> notes;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string ln = trim(lines[i]);
        if (ln.empty() || ln[0] == '#') continue;
        const std::vector<std::string> c = csv(ln);
        if (c[0] == "VERSION") { if (c.size() != 2 || c[1] != "1") { L.source = "defaults (unsupported VERSION)"; L.notes.push_back("VERSION must be 1"); return L; } haveVer = true; continue; }
        if (!haveVer) { L.source = "defaults (VERSION line missing)"; L.notes.push_back("first line must be VERSION,1"); return L; }
        if (c[0] == "MARKET") {
            if (c.size() != 2 || c[1] != market) { L.source = "defaults (MARKET mismatch)"; L.notes.push_back("MARKET is " + (c.size() > 1 ? c[1] : std::string("?")) + ", chart is " + market); return L; }
            haveMkt = true; continue;
        }
        if (c[0] == "ASOF" || c[0] == "SOURCE") continue;                  // informational
        if (c[0] != "PARAM") { notes.push_back("line " + std::to_string(i + 1) + ": unknown key " + c[0].substr(0, 20) + " (ignored)"); continue; }
        if (c.size() != 3) { notes.push_back("line " + std::to_string(i + 1) + ": PARAM needs name,value (ignored)"); continue; }
        size_t k = 0; while (k < PARAM_COUNT && c[1] != PARAM_DEFS[k].name) ++k;
        if (k == PARAM_COUNT) { notes.push_back("unknown PARAM " + c[1].substr(0, 30) + " (ignored)"); continue; }
        const ParamDef& d = PARAM_DEFS[k]; long long v = 0; bool ok = false;
        if (d.tenths) { double x = 0; if (nl::parseReal(c[2], x) && x >= 0 && x < 1e6) { v = (long long)std::llround(x * 10.0); ok = v >= d.lo && v <= d.hi; } }
        else ok = nl::parseWhole(c[2], d.lo, d.hi, v);
        if (!ok) {
            const long long def = Params().*(d.slot);
            notes.push_back(std::string(d.name) + ": bad value '" + c[2].substr(0, 20) + "' (" + (d.tenths ? tenthsText(d.lo) + ".." + tenthsText(d.hi) : std::to_string(d.lo) + ".." + std::to_string(d.hi))
                + ") - default " + (d.tenths ? tenthsText(def) : std::to_string(def)));
            continue;
        }
        if (p.fromFile.count(d.name)) notes.push_back(std::string(d.name) + ": duplicate (the last one used)");
        p.*(d.slot) = v; p.fromFile.insert(d.name);
    }
    if (!haveVer) { L.source = "defaults (empty params file)"; L.notes.push_back("no VERSION line"); return L; }
    if (!haveMkt) { L.source = "defaults (MARKET line missing)"; L.notes.push_back("no MARKET line"); return L; }
    L.p = p; L.notes = notes; L.source = "file (" + std::to_string(p.fromFile.size()) + " params)";
    return L;
}
inline std::string paramsText(const Params& p) {
    std::string o;
    for (size_t k = 0; k < PARAM_COUNT; ++k) {
        const ParamDef& d = PARAM_DEFS[k]; const long long v = p.*(d.slot);
        if (k) o += ";";
        o += std::string(d.name) + "=" + (d.tenths ? tenthsText(v) : std::to_string(v)) + (p.file(d.name) ? "(file)" : "(default)");
    }
    return o;
}

// =====================================================================================================================
// SIZE DISTRIBUTION (this session): exact counts per size; percentile = nearest rank
// =====================================================================================================================
struct SizeDist {
    std::map<long long, long long> n; long long total = 0;
    static const long long CAP = 1000000; static const size_t KEYS = 4096;
    void clear() { n.clear(); total = 0; }
    void add(long long s, long long k = 1) {
        if (s <= 0 || k <= 0) return;
        if (s > CAP) s = CAP;
        std::map<long long, long long>::iterator it = n.find(s);
        if (it == n.end() && n.size() >= KEYS) {                          // bounded: fold into the nearest smaller size
            it = n.upper_bound(s); if (it != n.begin()) --it;
            it->second += k; total += k; return;
        }
        n[s] += k; total += k;
    }
    long long pct(long long p) const {
        if (total <= 0 || n.empty()) return 0;
        if (p >= 100) return n.rbegin()->first;
        long long need = (p * total + 99) / 100; if (need < 1) need = 1;
        long long c = 0;
        for (std::map<long long, long long>::const_iterator it = n.begin(); it != n.end(); ++it) { c += it->second; if (c >= need) return it->first; }
        return n.rbegin()->first;
    }
    std::string save() const {
        std::string o; for (std::map<long long, long long>::const_iterator it = n.begin(); it != n.end(); ++it) { if (!o.empty()) o += ","; o += std::to_string(it->first) + ":" + std::to_string(it->second); }
        return o;
    }
    bool load(const std::string& s) {                                     // strict; false = left unchanged
        SizeDist d; if (s.empty()) { *this = d; return true; }
        if (s.size() > 200000) return false;
        size_t a = 0;
        for (;;) {
            const size_t c = s.find(',', a); const std::string f = s.substr(a, c == std::string::npos ? std::string::npos : c - a);
            const size_t k = f.find(':'); long long sz = 0, ct = 0;
            if (k == std::string::npos || !nl::parseWhole(f.substr(0, k), 1, CAP, sz) || !nl::parseWhole(f.substr(k + 1), 1, 1000000000000LL, ct)) return false;
            d.add(sz, ct);
            if (c == std::string::npos) break;
            a = c + 1;
        }
        *this = d; return true;
    }
};
inline long long largeThreshold(const SizeDist& d, long long fixed, long long pct, long long minLots) {
    if (fixed > 0) return fixed;
    const long long v = d.pct(pct);
    return v > minLots ? v : minLots;
}

// =====================================================================================================================
// THE ENGINE
// =====================================================================================================================
struct Ev {
    int action = NEW;          // 0..3, anything else = garbage
    int side = -1;             // BID / ASK, -1 = unknown (buySell 2)
    Tick px = 0;               // price in ticks (> 0)
    long long size = 0, id = 0, aggr = 0;   // size, the exchange order id, FILL: the aggressor's order id (0 = none)
    long long arrMs = 0;       // arrival (steady clock, ms)
    long long stamp = 0;       // the event's own time (RTDATE, whole seconds), 0 = none
};
inline int sideOfRaw(unsigned buySell) { return buySell == 1 ? BID : buySell == 0 ? ASK : -1; }
// a DEPTH_ORDER (any struct with its fields) -> Ev; a bad price -> px 0 (the engine counts it as garbage)
template <class O> Ev evOf(const O& o, double tick, long long arrMs) {
    Ev x; x.action = o.action <= 3 ? (int)o.action : 99; x.side = sideOfRaw((unsigned)o.buySell);
    x.size = (long long)o.size; x.id = (long long)o.orderID; x.aggr = (long long)o.aggressorOrderID; x.arrMs = arrMs; x.stamp = (long long)o.timestamp;
    if (!finite((double)o.price) || (double)o.price <= 0 || !toTick((double)o.price, tick, x.px)) x.px = 0;
    return x;
}

struct Ice { long long traded = 0, shown = 0; bool unknown = false; };
struct Aggr { int side = -1; long long size = 0; Tick px[AGGR_PX_MAX]; int nPx = 0; bool more = false; };
// what one candle collected (open while the candle is forming / closing)
struct Acc {
    long long events = 0;
    long long pullN[2] = {0, 0}, pullC[2] = {0, 0}, pullMax[2] = {0, 0}, pullClose[2] = {0, 0};
    long long addN[2] = {0, 0}, addC[2] = {0, 0}, addMax[2] = {0, 0};
    long long refN[2] = {0, 0}, refC[2] = {0, 0};
    std::unordered_map<long long, long long> addBy[2];  // order id -> contracts added in this candle (concentration)
    bool orderCap = false, aggrCap = false;
    std::map<Tick, Ice> ice[2];
    std::set<Tick> refPx[2];                             // prices with a refill this candle
    std::unordered_map<long long, Aggr> aggr;            // aggressor order id -> what it filled this candle
    long long aggrUnknown = 0;                           // fills without an aggressor id
    std::vector<long long> restSizes;                    // resting sizes added near price (the session distribution)
};
// a decided candle's numbers (all three indicators keep all of them; the nightly reads the record)
struct Rec {
    bool partial = false; long long events = 0;
    long long pullN[2] = {0, 0}, pullC[2] = {0, 0}, pullMax[2] = {0, 0}, pullClose[2] = {0, 0};
    long long addN[2] = {0, 0}, addC[2] = {0, 0}, addTop[2] = {0, 0}, addMax[2] = {0, 0};
    long long refN[2] = {0, 0}, refC[2] = {0, 0}, ice[2] = {0, 0}, iceShown[2] = {0, 0}, iceTraded[2] = {0, 0};   // the biggest iceberg price: shown / traded
    long long sweepLv[2] = {0, 0}, sweepSize[2] = {0, 0}, aggrMax[2] = {0, 0}, sweepIsMax[2] = {0, 0};   // [side of the RESTING orders hit]
    long long thrAggr = 0, thrRest = 0;                  // the large thresholds in force for this candle
};
static const int REC_NUMS = 38;
static const char* const REC_COLS = "partial|events|pull_n_b|pull_n_a|pull_cts_b|pull_cts_a|pull_max_b|pull_max_a|pull_close_b|pull_close_a|"
    "add_n_b|add_n_a|add_cts_b|add_cts_a|add_top_b|add_top_a|add_max_b|add_max_a|refill_n_b|refill_n_a|refill_cts_b|refill_cts_a|iceberg_b|iceberg_a|"
    "sweep_lv_b|sweep_lv_a|sweep_size_b|sweep_size_a|aggr_max_b|aggr_max_a|sweep_is_max_b|sweep_is_max_a|ice_shown_b|ice_shown_a|ice_traded_b|ice_traded_a|thr_aggr|thr_rest";
inline void recNums(const Rec& r, std::vector<long long>& v) {
    v.push_back(r.partial ? 1 : 0); v.push_back(r.events);
    const long long* pairs[17] = { r.pullN, r.pullC, r.pullMax, r.pullClose, r.addN, r.addC, r.addTop, r.addMax, r.refN, r.refC, r.ice, r.sweepLv, r.sweepSize, r.aggrMax, r.sweepIsMax, r.iceShown, r.iceTraded };
    for (int k = 0; k < 17; ++k) { v.push_back(pairs[k][0]); v.push_back(pairs[k][1]); }
    v.push_back(r.thrAggr); v.push_back(r.thrRest);
}
inline bool recFrom(const std::vector<long long>& v, size_t at, Rec& out) {
    if (v.size() < at + REC_NUMS || v[at] > 1) return false;
    Rec r; r.partial = v[at] == 1; r.events = v[at + 1];
    long long* pairs[17] = { r.pullN, r.pullC, r.pullMax, r.pullClose, r.addN, r.addC, r.addTop, r.addMax, r.refN, r.refC, r.ice, r.sweepLv, r.sweepSize, r.aggrMax, r.sweepIsMax, r.iceShown, r.iceTraded };
    for (int k = 0; k < 17; ++k) { pairs[k][0] = v[at + 2 + 2 * k]; pairs[k][1] = v[at + 3 + 2 * k]; }
    r.thrAggr = v[at + 36]; r.thrRest = v[at + 37];
    for (int s = 0; s < 2; ++s) if (r.addTop[s] > r.addC[s] || r.sweepIsMax[s] > 1 || (r.ice[s] > 0 && r.iceTraded[s] <= r.iceShown[s])) return false;
    out = r; return true;
}

class Engine {
public:
    struct Ord { int side = BID; Tick px = 0; long long size = 0; bool filledLast = false; bool known = true; };
    Tick near = 0;                                     // the near band in ticks (0 = nothing is near)
    Tick ref = 0; bool haveRef = false; long long lastFillMs = -1;
    long long refillMs = 1000; Tick closeTicks = 2;
    std::unordered_map<long long, Ord> book;
    std::unordered_map<Tick, long long> rest[2];       // known resting size per price
    std::map<Tick, std::pair<long long, long long> > lastFill[2];   // price -> (arrival ms, stamp) of the latest fill there
    SizeDist aggrDist, restDist;                       // this session's aggressor order sizes / resting sizes near price
    long long events = 0, garbage = 0, unknownDel = 0, unknownMod = 0, unknownFill = 0, dupNew = 0, bookDrops = 0, bookResets = 0;
    long long perAction[4] = {0, 0, 0, 0};

    bool isNear(Tick px) const { return haveRef && near > 0 && std::llabs(px - ref) <= near; }
    void fallbackRef(Tick px, long long nowMs) {
        if (px <= 0) return;
        if (lastFillMs < 0 || nowMs - lastFillMs > REF_STALE_MS || nowMs < lastFillMs) { ref = px; haveRef = true; }
    }
    void restAdd(int s, Tick px, long long d) {
        if (d == 0) return;
        std::unordered_map<Tick, long long>::iterator it = rest[s].find(px);
        if (it == rest[s].end()) { if (d > 0) rest[s][px] = d; return; }
        it->second += d; if (it->second <= 0) rest[s].erase(it);
    }
    long long restAt(int s, Tick px) const { std::unordered_map<Tick, long long>::const_iterator it = rest[s].find(px); return it == rest[s].end() ? 0 : it->second; }
    static void bump(std::unordered_map<long long, long long>& m, long long id, long long c, Acc& a) {
        if (c <= 0) return;
        std::unordered_map<long long, long long>::iterator it = m.find(id);
        if (it != m.end()) { it->second += c; return; }
        if (m.size() >= PER_ORDER_CAP) { a.orderCap = true; return; }
        m[id] = c;
    }
    void shown(Acc& a, int s, Tick px) {                // a price with fills this candle: the most displayed there
        std::map<Tick, Ice>::iterator it = a.ice[s].find(px);
        if (it != a.ice[s].end()) { const long long r = restAt(s, px); if (r > it->second.shown) it->second.shown = r; }
    }
    bool refillHit(int s, Tick px, const Ev& e) const {
        if (refillMs <= 0) return false;
        std::map<Tick, std::pair<long long, long long> >::const_iterator it = lastFill[s].find(px);
        if (it == lastFill[s].end()) return false;
        const long long dt = e.arrMs - it->second.first;
        if (dt < 0 || dt > refillMs) return false;
        if (e.stamp > 0 && it->second.second > 0) {
            const long long ds = e.stamp - it->second.second, maxS = (refillMs + 999) / 1000;
            if (ds < 0 || ds > maxS) return false;
        }
        return true;
    }
    void pull(Acc& a, int s, Tick px, long long c, bool wholeOrder) {
        if (wholeOrder) {
            a.pullN[s]++;
            if (c > a.pullMax[s]) a.pullMax[s] = c;
            if (haveRef && std::llabs(px - ref) <= closeTicks) a.pullClose[s]++;   // pulled just before price got there
        }
        if (c > 0) a.pullC[s] += c;
    }
    void add(Acc& a, int s, long long id, long long c, bool wholeOrder) {
        if (wholeOrder) {
            a.addN[s]++;
            if (c > a.addMax[s]) a.addMax[s] = c;
            if (c > 0 && a.restSizes.size() < REST_SIZES_CAP) a.restSizes.push_back(c);
        }
        if (c > 0) { a.addC[s] += c; bump(a.addBy[s], id, c, a); }
    }
    void refill(Acc& a, int s, Tick px, long long c) { a.refN[s]++; a.refC[s] += c; if (a.refPx[s].size() < ICE_CAP) a.refPx[s].insert(px); }
    void aggressor(Acc& a, int restingSide, const Ev& e) {
        if (e.aggr == 0) { a.aggrUnknown++; return; }
        std::unordered_map<long long, Aggr>::iterator it = a.aggr.find(e.aggr);
        if (it == a.aggr.end()) {
            if (a.aggr.size() >= AGGR_CAP) { a.aggrCap = true; return; }
            it = a.aggr.insert(std::make_pair(e.aggr, Aggr())).first;
            it->second.side = restingSide;
        }
        Aggr& g = it->second;
        if (g.side != restingSide) return;                                 // an id seen on both sides: keep the first (garbage guard)
        g.size += e.size;
        for (int i = 0; i < g.nPx; ++i) if (g.px[i] == e.px) return;
        if (g.nPx < AGGR_PX_MAX) g.px[g.nPx++] = e.px; else g.more = true;
    }
    void apply(Acc& a, const Ev& e) {
        if (e.action < NEW || e.action > FILL || e.size < 0 || e.size > SIZE_MAX_SANE || e.px <= 0) { garbage++; return; }
        std::unordered_map<long long, Ord>::iterator it = book.find(e.id);
        int s = e.side;
        if (it != book.end() && e.action != NEW) s = it->second.side;     // the order's own side wins
        if (s != BID && s != ASK) { garbage++; return; }                  // an unknown side cannot be classified
        events++; perAction[e.action]++; a.events++;
        switch (e.action) {
        case NEW: {
            if (it != book.end()) {                                        // a duplicate / replayed NEW: refresh, never counted twice
                Ord& o = it->second; dupNew++;
                if (o.known) restAdd(o.side, o.px, -o.size);
                o.side = s; o.px = e.px; o.size = e.size; o.filledLast = false; o.known = true;
                restAdd(s, e.px, e.size); shown(a, s, e.px);
                break;
            }
            Ord o; o.side = s; o.px = e.px; o.size = e.size; book[e.id] = o;
            restAdd(s, e.px, e.size); shown(a, s, e.px);
            if (isNear(e.px)) {
                add(a, s, e.id, e.size, true);
                if (refillHit(s, e.px, e)) refill(a, s, e.px, e.size);
            }
            break; }
        case FILL: {
            Ice* ic = nullptr;
            {
                std::map<Tick, Ice>::iterator ii = a.ice[s].find(e.px);
                if (ii != a.ice[s].end()) ic = &ii->second;
                else if (a.ice[s].size() < ICE_CAP) ic = &a.ice[s][e.px];
            }
            if (ic) { const long long r = restAt(s, e.px); if (r > ic->shown) ic->shown = r; ic->traded += e.size; }
            if (it != book.end() && it->second.known) {
                Ord& o = it->second;
                const long long take = e.size < o.size ? e.size : o.size;
                restAdd(o.side, o.px, -take); o.size -= take; o.filledLast = true;
            } else {
                unknownFill++;
                if (ic) ic->unknown = true;
                if (it == book.end()) { Ord o; o.side = s; o.px = e.px; o.size = 0; o.filledLast = true; o.known = false; book[e.id] = o; }
                else it->second.filledLast = true;
            }
            aggressor(a, s, e);
            ref = e.px; haveRef = true; lastFillMs = e.arrMs;              // a fill's price = the last trade
            lastFill[s][e.px] = std::make_pair(e.arrMs, e.stamp);
            if (lastFill[s].size() > FILLMEM_CAP) {                        // forget fills older than the window, then everything
                for (std::map<Tick, std::pair<long long, long long> >::iterator f = lastFill[s].begin(); f != lastFill[s].end();)
                    if (e.arrMs - f->second.first > refillMs) lastFill[s].erase(f++); else ++f;
                if (lastFill[s].size() > FILLMEM_CAP) lastFill[s].clear();
            }
            break; }
        case DEL: {
            if (it == book.end()) {                                        // rested before load: a real cancel
                unknownDel++;
                if (isNear(e.px)) pull(a, s, e.px, e.size, true);
                break;
            }
            const Ord o = it->second; book.erase(it);
            if (o.known) restAdd(o.side, o.px, -o.size);
            if (o.filledLast || !o.known) break;                           // just filled: the trade took it, not a pull
            if (isNear(o.px)) pull(a, o.side, o.px, o.size > 0 ? o.size : e.size, true);
            break; }
        case MOD: {
            if (it == book.end() || !it->second.known) {                   // first sight: its state from now on, nothing counted
                unknownMod++;
                Ord o; o.side = s; o.px = e.px; o.size = e.size; book[e.id] = o;
                restAdd(s, e.px, e.size);
                break;
            }
            Ord& o = it->second;
            if (e.px != o.px) {                                            // moved: a pull at the old price + an add at the new
                if (isNear(o.px)) pull(a, o.side, o.px, o.size, true);
                if (isNear(e.px)) add(a, o.side, e.id, e.size, true);
                restAdd(o.side, o.px, -o.size); restAdd(o.side, e.px, e.size); shown(a, o.side, e.px);
            } else {
                const long long d = e.size - o.size;
                if (d < 0 && !o.filledLast && isNear(o.px)) pull(a, o.side, o.px, -d, false);
                if (d > 0 && isNear(o.px)) {
                    add(a, o.side, e.id, d, false);
                    if (refillHit(o.side, o.px, e)) refill(a, o.side, o.px, d);
                }
                restAdd(o.side, o.px, d); if (d > 0) shown(a, o.side, o.px);
            }
            o.px = e.px; o.size = e.size; o.filledLast = false;
            break; }
        default: break;
        }
        if (book.size() > BOOK_CAP) trimBook();
    }
    // bounded memory: drop tombstones and orders far from price (> 4 near bands); if still too many, forget the book
    void trimBook() {
        const Tick far = near > 0 ? 4 * near : 400;
        for (std::unordered_map<long long, Ord>::iterator it = book.begin(); it != book.end();) {
            const Ord& o = it->second;
            if (!o.known || !haveRef || std::llabs(o.px - ref) > far) { if (o.known) restAdd(o.side, o.px, -o.size); it = book.erase(it); bookDrops++; }
            else ++it;
        }
        if (book.size() > BOOK_CAP * 9 / 10) { book.clear(); rest[0].clear(); rest[1].clear(); bookResets++; }
    }
    // a candle is decided: its numbers, the large thresholds AS OF the candles before it, then its sizes join the distributions
    // the numbers only (nothing recorded): the forming candle's live look uses this
    Rec numbers(const Acc& a, bool partial, const Params& P) const {
        Rec r; r.partial = partial; r.events = a.events;
        r.thrAggr = largeThreshold(aggrDist, P.largeLotAggr, P.largeLotPct, P.largeLotMin);
        r.thrRest = largeThreshold(restDist, P.largeLotResting, P.largeLotPct, P.largeLotMin);
        for (int s = 0; s < 2; ++s) {
            r.pullN[s] = a.pullN[s]; r.pullC[s] = a.pullC[s]; r.pullMax[s] = a.pullMax[s]; r.pullClose[s] = a.pullClose[s];
            r.addN[s] = a.addN[s]; r.addC[s] = a.addC[s]; r.addMax[s] = a.addMax[s];
            long long top = 0; for (std::unordered_map<long long, long long>::const_iterator i = a.addBy[s].begin(); i != a.addBy[s].end(); ++i) if (i->second > top) top = i->second;
            r.addTop[s] = top > r.addC[s] ? r.addC[s] : top;
            r.refN[s] = a.refN[s]; r.refC[s] = a.refC[s];
            for (std::set<Tick>::const_iterator p = a.refPx[s].begin(); p != a.refPx[s].end(); ++p) {
                std::map<Tick, Ice>::const_iterator i = a.ice[s].find(*p);
                if (i != a.ice[s].end() && !i->second.unknown && i->second.shown > 0 && i->second.traded > i->second.shown) {
                    r.ice[s]++;
                    if (i->second.traded - i->second.shown > r.iceTraded[s] - r.iceShown[s]) { r.iceShown[s] = i->second.shown; r.iceTraded[s] = i->second.traded; }
                }
            }
        }
        long long bestLv[2] = {0, 0}, bestSz[2] = {0, 0};
        for (std::unordered_map<long long, Aggr>::const_iterator i = a.aggr.begin(); i != a.aggr.end(); ++i) {
            const Aggr& g = i->second; if (g.side != BID && g.side != ASK) continue;
            const int s = g.side; const long long lv = g.nPx + (g.more ? 1 : 0);
            if (g.size > r.aggrMax[s]) r.aggrMax[s] = g.size;
            if (lv > bestLv[s] || (lv == bestLv[s] && g.size > bestSz[s])) { bestLv[s] = lv; bestSz[s] = g.size; }
        }
        for (int s = 0; s < 2; ++s) { r.sweepLv[s] = bestLv[s]; r.sweepSize[s] = bestSz[s]; r.sweepIsMax[s] = bestSz[s] > 0 && bestSz[s] >= r.aggrMax[s] ? 1 : 0; }
        return r;
    }
    Rec close(const Acc& a, bool partial, const Params& P) {
        const Rec r = numbers(a, partial, P);
        for (std::unordered_map<long long, Aggr>::const_iterator i = a.aggr.begin(); i != a.aggr.end(); ++i) if (i->second.side == BID || i->second.side == ASK) aggrDist.add(i->second.size);
        for (size_t i = 0; i < a.restSizes.size(); ++i) restDist.add(a.restSizes[i]);
        return r;
    }
    void newSession() { aggrDist.clear(); restDist.clear(); }
};

// =====================================================================================================================
// STATS AS OF EACH BAR (the lsBidAskStats 1.2.0 rules)
//   ratio = the value / the mean of that side's prior `ratioBars` valid closed bars (needs 5); for the MBO counts the mean is
//   floored at 1 (a quiet history of 0.3 pulls a bar does not make 2 pulls a 6x spike).
//   spike = >= spikeAvg x that mean AND >= spikeJump x the biggest bar of EITHER side in the prior `lookback` valid bars.
// =====================================================================================================================
static const int RATIO_MIN_PRIOR = 5, RATIO_DEFAULT = 20, RATIO_LO = 5, RATIO_HI = 200, SPIKE_LOOKBACK_MAX = 50;
static const double HIGH_X = 2.0;                   // a side is HIGH (it gets the message) when a spike or >= 2.0x its average
struct SpikeCfg {
    long long avgT = 20, jumpT = 15, lookback = 10;
    bool on() const { return avgT > 0 && jumpT > 0 && lookback > 0; }
    bool operator==(const SpikeCfg& o) const { return avgT == o.avgT && jumpT == o.jumpT && lookback == o.lookback; }
    bool operator!=(const SpikeCfg& o) const { return !(*this == o); }
};
// the spike settings in force: the nightly params win where the file sets them; the dialog fills the rest; a dialog 0 = off
inline SpikeCfg effectiveSpike(const SpikeCfg& dialog, const Params& P) {
    SpikeCfg c;
    c.avgT = P.file("spikeAvgX") ? P.spikeAvgT : dialog.avgT;
    c.jumpT = P.file("spikeJumpX") ? P.spikeJumpT : dialog.jumpT;
    c.lookback = P.file("spikeLookback") ? P.spikeLookback : dialog.lookback;
    if (!dialog.on()) c.avgT = 0;                                          // the dialog's 0 = off always wins
    return c;
}
inline long long priorMaxEither(const std::vector<long long>& pb, const std::vector<long long>& pa, long long n) {
    long long mx = 0; if (n <= 0) return 0;
    const size_t k = pb.size() < (size_t)n ? pb.size() : (size_t)n;
    for (size_t i = pb.size() - k; i < pb.size(); ++i) { if (pb[i] > mx) mx = pb[i]; if (i < pa.size() && pa[i] > mx) mx = pa[i]; }
    return mx;
}
inline bool isSpike(long long v, double mean, long long priorMax, const SpikeCfg& c) {
    if (!c.on() || v <= 0 || !(mean > 0) || priorMax <= 0) return false;
    return (double)v * 10.0 >= (double)c.avgT * mean && v * 10 >= c.jumpT * priorMax;
}
inline double ratioOf(const std::vector<long long>& prior, long long v, int bars, bool& ok, double* meanOut = nullptr, double floor = 0) {
    ok = false; if (meanOut) *meanOut = 0;
    if (bars < RATIO_LO) bars = RATIO_LO;
    if (bars > RATIO_HI) bars = RATIO_HI;
    const size_t n = prior.size() < (size_t)bars ? prior.size() : (size_t)bars;
    if (n < (size_t)RATIO_MIN_PRIOR) return 0;
    double sum = 0;
    for (size_t i = prior.size() - n; i < prior.size(); ++i) sum += (double)prior[i];
    double mean = sum / (double)n;
    if (floor > 0 && mean < floor) mean = floor;
    if (!(mean > 0)) return 0;
    if (meanOut) *meanOut = mean;
    ok = true; return (double)v / mean;
}
struct MStat { bool ratioOk[2] = {false, false}; double ratio[2] = {0, 0}, mean[2] = {0, 0}; long long priorMax = 0; bool spike[2] = {false, false}; };
inline MStat metricStat(const std::vector<long long>& pb, const std::vector<long long>& pa, const long long v[2], int ratioBars, const SpikeCfg& c, double floor) {
    MStat m;
    for (int s = 0; s < 2; ++s) m.ratio[s] = ratioOf(s == BID ? pb : pa, v[s], ratioBars, m.ratioOk[s], &m.mean[s], floor);
    m.priorMax = priorMaxEither(pb, pa, c.lookback > 0 ? c.lookback : 10);
    for (int s = 0; s < 2; ++s) m.spike[s] = m.ratioOk[s] && isSpike(v[s], m.mean[s], m.priorMax, c);
    return m;
}
inline bool high(const MStat& m, int s) { return m.spike[s] || (m.ratioOk[s] && m.ratio[s] >= HIGH_X); }
inline std::string ratioText(double r) { return nl::ratioText(r); }
inline std::string ratioOr(bool ok, double r) { return ok ? nl::ratioText(r) : std::string("n/a"); }

// =====================================================================================================================
// THE MBO PANES' SERIES (PullStats / StackStats): one bar = a Rec + its display, decided once at the close
// =====================================================================================================================
struct PaneBar {
    bool ok = false, fromFile = false;
    Rec r; MStat m[2];                                   // m: stats as of the bar (metric 0 = the bars, 1 = refills)
    std::string let[2];                                  // letters on [left = BID side, right = ASK side], max 2 each
    bool spike[2] = {false, false}; long long dash[2] = {-1, -1};
    nl::Msg msg; std::string level;
};
template <class L> struct PaneSeries {
    int ratioBars = RATIO_DEFAULT; SpikeCfg spike;
    std::map<long long, PaneBar> bars;
    long long computed = 0;
    // the prior values of one metric before t: valid, not partial, newest last (at most `need`)
    void prior(long long t, size_t need, int metric, std::vector<long long>& pb, std::vector<long long>& pa) const {
        pb.clear(); pa.clear();
        typename std::map<long long, PaneBar>::const_iterator it = bars.lower_bound(t);
        while (it != bars.begin() && pb.size() < need) {
            --it;
            if (it->second.ok && !it->second.r.partial) { pb.push_back(L::value(it->second.r, metric, BID)); pa.push_back(L::value(it->second.r, metric, ASK)); }
        }
        for (size_t i = 0, j = pb.size(); i < j / 2; ++i) { std::swap(pb[i], pb[j - 1 - i]); std::swap(pa[i], pa[j - 1 - i]); }
    }
    void stats(long long t, PaneBar& b) const {
        const size_t need = (size_t)(ratioBars > SPIKE_LOOKBACK_MAX ? ratioBars : SPIKE_LOOKBACK_MAX);
        std::vector<long long> pb, pa;
        for (int k = 0; k < L::metrics(); ++k) {
            prior(t, need, k, pb, pa);
            const long long v[2] = { L::value(b.r, k, BID), L::value(b.r, k, ASK) };
            b.m[k] = metricStat(pb, pa, v, ratioBars, spike, 1.0);
        }
    }
    PaneBar make(long long t, const Rec& r) const { PaneBar b; b.r = r; b.ok = r.events > 0; if (b.ok) stats(t, b); return b; }
    // a settings change (dialog) only: every recorded bar re-decided, oldest first, from its stored numbers (letters / level kept)
    template <class Decide> void recompute(Decide decide) {
        for (typename std::map<long long, PaneBar>::iterator it = bars.begin(); it != bars.end(); ++it) {
            if (!it->second.ok) continue;
            stats(it->first, it->second); decide(it->second);
        }
    }
    void trim(size_t keep) { while (bars.size() > keep) bars.erase(bars.begin()); }
};

// =====================================================================================================================
// LAYOUT (pixels): 9-spot placement, the two bars of a candle, bar height, the value axis, letters, hover, clocks
// =====================================================================================================================
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";
static const int POS_DEFAULT = 0;                  // the INFO BOX: Top left (12:05, as lsBidAskStats 1.2.0); the message line is always top centre
static const int TEXT_RESERVE = 18;                // px kept free at the top for the one message line
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
// the two bars inside one candle slot: [0] left, [1] right (never wider than the slot)
struct Pair { int l[2], r[2]; };
inline Pair pairIn(int xc, int slot) {
    if (slot < 2) slot = 2;
    int inner = (slot * 8) / 10; if (inner < 2) inner = 2;
    const int gap = inner >= 6 ? 1 : 0;
    const int half = (inner - gap) / 2 > 0 ? (inner - gap) / 2 : 1;
    Pair p; p.r[0] = xc; p.l[0] = xc - half; p.l[1] = xc + gap; p.r[1] = p.l[1] + half;
    return p;
}
inline int barHeight(long long v, long long top, int px) {
    if (v <= 0 || top <= 0 || px <= 0) return 0;
    const double h = (double)px * (double)v / (double)top;
    const int r = (int)(h + .5);
    return r < 1 ? 1 : (r > px ? px : r);
}
// (12:03 / 12:10) HEADROOM for the spike labels and the message line: the tallest bar tops out at <= 1/1.45 (~69%) of the drawing
// height AND leaves LABEL_ROOM px (two label lines) plus the text reserve above it. The axis (scale()) uses the same px, so every
// bar top still sits at its value on the axis.
static const int LABEL_LH = 10, LABEL_ROOM = 2 * LABEL_LH + 2;
inline int drawPx(int paneH, int reserve) {
    const int avail = paneH - 6; if (avail <= 1) return 1;
    int p = (int)((double)avail / 1.45);
    const int q = avail - reserve - LABEL_ROOM; if (q < p) p = q;
    if (p < 8) p = avail > 8 ? 8 : avail;
    return p;
}
// the axis range IRT draws over the whole pane so each value sits at its bar's top pixel (value 0 at bottom - 1)
inline void axisFor(long long top, int paneH, int px, double& mn, double& mx) {
    if (top <= 0) { mn = 0; mx = 10; return; }
    if (paneH < 24 || px <= 0) { mn = 0; mx = (double)top * 1.02; return; }
    const double unit = (double)top / (double)px;
    mn = -unit; mx = (double)(paneH - 1) * unit;
}
inline int textReserve(int paneH) { return paneH >= 80 ? TEXT_RESERVE : 0; }
// LETTERS: white boxes (black bold letter) at the BOTTOM of a side's bar, priority letter on top (mockup), at most 2 per side.
// Never overlapping and never outside the candle's slot: if the two sides' columns would touch, one column in the slot's centre
// (left side's letters at the bottom, then the right side's); slot narrower than a box = no letters (zoomed out).
struct LBox { int l, t, r, b; char c; int side; };
inline std::vector<LBox> letterBoxes(int slotL, int slotR, int cx0, int cx1, int yBottom, int yTop, const std::string& let0, const std::string& let1, int bw, int bh) {
    std::vector<LBox> out;
    const std::string L[2] = { let0.substr(0, 2), let1.substr(0, 2) };
    if (bw <= 0 || bh <= 0 || slotR - slotL + 1 < bw || (L[0].empty() && L[1].empty())) return out;
    auto colX = [&](int cx) { int x = cx - bw / 2; if (x + bw - 1 > slotR) x = slotR - bw + 1; if (x < slotL) x = slotL; return x; };
    int x[2] = { colX(cx0), colX(cx1) };
    const bool single = !L[0].empty() && !L[1].empty() && x[1] <= x[0] + bw;   // the columns would touch
    if (single) x[0] = x[1] = colX((slotL + slotR + 1) / 2);
    int y = yBottom;
    for (int s = 0; s < 2; ++s) {
        if (!single) y = yBottom;
        for (int k = (int)L[s].size() - 1; k >= 0; --k) {               // bottom-up: the last (lower priority) letter lowest
            const int t = y - bh + 1;
            if (t < yTop) break;
            out.push_back(LBox{ x[s], t, x[s] + bw - 1, y, L[s][(size_t)k], s });
            y = t - 2;
        }
    }
    return out;
}
inline bool overlaps(const LBox& a, const LBox& b) { return !(a.r < b.l || b.r < a.l || a.b < b.t || b.b < a.t); }
// SPIKE LABELS (Rassul 12:03 "i want the quantity and a ratio on top of the spikes"): above every spike bar, centred on it, two small
// lines in the spike colour - the ratio on top, the quantity just above the bar top ("4.1x" over "240"). Always ABOVE the bar (12:10:
// no inside-the-bar fallback). A label that would touch another label or a letter box is staggered one label height up (at most 3
// steps); one that would cross yTop (the message lines / pane top) is not drawn - labels never overlap anything.
struct SpikeIn { int cx = 0, barTop = 0, side = 0; std::string q, x; };
struct SLabel { int l, t, r, b, side; std::string q, x; };
template <class W> std::vector<SLabel> spikeLabels(const std::vector<SpikeIn>& in, int yTop, const std::vector<LBox>& letters, W width) {
    std::vector<SLabel> out;
    for (size_t i = 0; i < in.size(); ++i) {
        const int w = std::max(width(in[i].q), width(in[i].x)) + 2, h = 2 * LABEL_LH;
        for (int step = 0; step < 4; ++step) {
            const int b = in[i].barTop - 2 - step * (h + 2), t = b - h + 1;
            if (t < yTop) break;
            const LBox me = { in[i].cx - w / 2, t, in[i].cx - w / 2 + w - 1, b, 0, 0 };
            bool hit = false;
            for (size_t k = 0; k < out.size() && !hit; ++k) { const LBox o = { out[k].l, out[k].t, out[k].r, out[k].b, 0, 0 }; hit = overlaps(me, o); }
            for (size_t k = 0; k < letters.size() && !hit; ++k) hit = overlaps(me, letters[k]);
            if (hit) continue;
            out.push_back(SLabel{ me.l, me.t, me.r, me.b, in[i].side, in[i].q, in[i].x });
            break;
        }
    }
    return out;
}
inline int hoverIndex(long bar, long count) { return bar >= 0 && bar < count - 1 ? (int)bar : -1; }
struct Hover { int bar = -1; bool set(int b) { if (b < -1) b = -1; if (b == bar) return false; bar = b; return true; } };
inline std::string clock12(int hour, int minute) {
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return "n/a";
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", hour % 12 == 0 ? 12 : hour % 12, minute, hour < 12 ? "AM" : "PM"); return b;
}
using nl::clockShort; using nl::fitText; using nl::Msg; using nl::NEUTRAL; using nl::BULL; using nl::BEAR;
// the first top line: "10:24  <message>"
inline std::string line1Of(const std::string& ct, const std::string& msg) { return ct.empty() ? msg : ct + "  " + msg; }
// (12:05) the info box's content (drawn by drawInfo): label | left value | right value; side -1 = white
struct InfoRow { std::string label, v[2]; int side[2] = {0, 1}; };          // side -1 = white
struct Info { std::string head; bool twoCol = true; std::string colHead[2]; std::vector<InfoRow> rows; std::string tmpl = "9999 (99.9x)"; };

// =====================================================================================================================
// TIME: the 17:00 Central trading day of a bar END stamp (local clock fields), as yyyymmdd; at / after 17:00 = the next day
// =====================================================================================================================
inline long long civilDays(long long y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
inline std::string ymdOf(long long days) {
    const long long z = days + 719468, era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153, day = doy - (153 * mp + 2) / 5 + 1, mon = mp < 10 ? mp + 3 : mp - 9; const long long yr = (long long)yoe + era * 400 + (mon <= 2);
    char b[48]; std::snprintf(b, sizeof(b), "%04lld%02u%02u", yr, mon % 100u, day % 100u); return b;
}
inline std::string sessionOf(int year, int mon, int mday, int hour) {
    if (mon < 1 || mon > 12 || mday < 1 || mday > 31 || hour < 0 || hour > 23) return std::string();
    return ymdOf(civilDays(year, (unsigned)mon, (unsigned)mday) + (hour >= 17 ? 1 : 0));
}
// the candle an event belongs to: IRT stamps bar ENDS, so the first bar end AFTER the stamp; past the forming bar (the chart has not
// made the new bar yet) = the predicted end of a later bar (at most 3 bars ahead); -1 = no bar (a gap, too old)
inline long long eventKey(const std::map<long long, int>& barEnds, long long formingKey, int spb, long long st) {
    if (barEnds.empty() || st <= 0) return -1;
    std::map<long long, int>::const_iterator i = barEnds.upper_bound(st);
    if (i != barEnds.end()) { if (spb > 0 && i->first - st > spb) return -1; return i->first; }
    if (spb <= 0) return formingKey;
    const long long k = formingKey + (long long)spb * ((st - formingKey) / spb + 1);
    return k - formingKey > 3LL * spb ? -1 : k;
}

// =====================================================================================================================
// FILES (no SDK): the lsFlexLevels folder, atomic publish, bounded reads, per-market names
// =====================================================================================================================
inline std::string flexDir() {
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels";
}
inline void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    (void)p;
#endif
}
inline long long fileStamp(const std::string& p) { return nl::fileSig(p); }   // -1 = no regular file
inline bool moveOver(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
// <path>.tmp, the old file to .bak, rename into place; on failure the .bak goes back: a reader never sees a partial file
inline bool publishFile(const std::string& path, const std::string& contents) {
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false;
      f.write(contents.data(), (std::streamsize)contents.size()); f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    const bool hadOld = fileStamp(path) >= 0;
    if (hadOld) { std::remove(bak.c_str()); if (!moveOver(path, bak)) { std::remove(tmp.c_str()); return false; } }
    if (!moveOver(tmp, path)) { if (hadOld) moveOver(bak, path); std::remove(tmp.c_str()); return false; }
    if (hadOld) std::remove(bak.c_str());
    return true;
}
inline std::string fileKey(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) {
        const unsigned char c = (unsigned char)s[i];
        out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_');
    }
    return out.empty() ? "UNKNOWN" : out;
}
inline std::string marketName(std::string r) {
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return fileKey(r.substr(0, 8));
}
// the params file of a market, re-read only when its stamp changes (checked at most every 5 s)
// (a same-second, same-size rewrite keeps the stamp: the file is also re-read once a minute; true only when the result changed)
struct ParamCache { long long stamp = -3, checked = -1, readMs = -1; std::string text = "\x01"; ParamLoad load; };
inline bool refreshParams(ParamCache& C, const std::string& dir, const std::string& market, long long nowMs) {
    if (C.checked >= 0 && nowMs >= C.checked && nowMs - C.checked < 5000) return false;
    C.checked = nowMs;
    const std::string p = dir.empty() || market.empty() ? std::string() : dir + "\\LRA-OrderFlow-" + market + ".csv";
    const long long st = p.empty() ? -1 : fileStamp(p);
    if (st == C.stamp && C.readMs >= 0 && nowMs >= C.readMs && nowMs - C.readMs < 60000) return false;
    C.stamp = st; C.readMs = nowMs;
    std::vector<std::string> lines; bool ok = false;
    if (st >= 0) ok = nl::readBoundedLines(p, lines, 64 * 1024, 400, 512);
    ParamLoad L = parseParams(st >= 0, ok, lines, market);
    std::string t = L.source + "|" + paramsText(L.p); for (size_t i = 0; i < L.notes.size(); ++i) t += "|" + L.notes[i];
    if (t == C.text) return false;
    C.text = t; C.load = L;
    return true;
}

// =====================================================================================================================
// THE SESSION RECORD: lsFlexLevels\<Indicator>\<MKT>-<CONTRACT>-<spb>-<session>.csv, one line per closed candle (written once,
// the whole file published atomically), re-read on load so a restart within the session shows the same history:
//   bar_time|ct_time|dbo|<numbers...>|letters_left|letters_right|spike_left|spike_right|dash_left|dash_right|implication|level|message|numbers
// and the session size distributions in <stem>-<session>-sizes.txt (AGGR|s:n,... / REST|s:n,...) for the large thresholds.
// =====================================================================================================================
struct Row {
    long long t = 0; std::string ct; int dbo = 0; std::vector<long long> nums;
    std::string let[2]; int spike[2] = {0, 0}; long long dash[2] = {-1, -1};
    int impl = NEUTRAL; std::string level, line1, line2;
};
inline std::string clean(const std::string& s, size_t maxLen) { std::string o; for (char c : s) { if (o.size() >= maxLen) break; o += (c == '|' || c == '\n' || c == '\r') ? ' ' : c; } return o; }
inline const char* implText(int i) { return i == BULL ? "bull" : i == BEAR ? "bear" : "neutral"; }
inline std::string rowLine(const Row& r) {
    std::string o = std::to_string(r.t) + "|" + clean(r.ct, 12) + "|" + (r.dbo ? "1" : "0");
    for (size_t i = 0; i < r.nums.size(); ++i) o += "|" + std::to_string(r.nums[i] < 0 ? 0 : r.nums[i]);
    for (int s = 0; s < 2; ++s) o += "|" + clean(r.let[s], 2);
    for (int s = 0; s < 2; ++s) o += std::string("|") + (r.spike[s] ? "1" : "0");
    for (int s = 0; s < 2; ++s) o += "|" + (r.dash[s] < 0 ? std::string("-") : std::to_string(r.dash[s]));
    o += std::string("|") + implText(r.impl) + "|" + clean(r.level, 40) + "|" + clean(r.line1, 300) + "|" + clean(r.line2, 300);
    return o;
}
inline bool allDigits(const std::string& f, size_t maxLen) { if (f.empty() || f.size() > maxLen) return false; for (char c : f) if (c < '0' || c > '9') return false; return true; }
// strict: exactly 3 + nNums + 10 fields, each checked; anything else = not a record
inline bool parseRow(const std::string& ln, size_t nNums, Row& out) {
    if (ln.empty() || ln.size() > 4096) return false;
    const std::vector<std::string> c = nl::fields(ln);
    if (c.size() != 13 + nNums) return false;
    Row r;
    if (!allDigits(c[0], 13) || c[1].size() > 12 || (c[2] != "0" && c[2] != "1")) return false;
    r.t = std::strtoll(c[0].c_str(), nullptr, 10); if (r.t <= 0) return false;
    r.ct = c[1]; r.dbo = c[2] == "1" ? 1 : 0;
    for (size_t i = 0; i < nNums; ++i) { if (!allDigits(c[3 + i], 15)) return false; r.nums.push_back(std::strtoll(c[3 + i].c_str(), nullptr, 10)); }
    size_t k = 3 + nNums;
    for (int s = 0; s < 2; ++s) { const std::string& L = c[k++]; if (L.size() > 2) return false; for (char ch : L) if (ch != 'S' && ch != 'L' && ch != 'R') return false; r.let[s] = L; }
    for (int s = 0; s < 2; ++s) { const std::string& f = c[k++]; if (f != "0" && f != "1") return false; r.spike[s] = f == "1"; }
    for (int s = 0; s < 2; ++s) { const std::string& f = c[k++]; if (f == "-") r.dash[s] = -1; else if (allDigits(f, 12)) r.dash[s] = std::strtoll(f.c_str(), nullptr, 10); else return false; }
    const std::string& im = c[k++];
    if (im == "bull") r.impl = BULL; else if (im == "bear") r.impl = BEAR; else if (im == "neutral") r.impl = NEUTRAL; else return false;
    if (c[k].size() > 40 || c[k + 1].size() > 300 || c[k + 2].size() > 300) return false;
    r.level = c[k]; r.line1 = c[k + 1]; r.line2 = c[k + 2];
    out = r; return true;
}
static const long long SESSION_MAX_BYTES = 6 * 1024 * 1024; static const size_t SESSION_MAX_LINES = 6000;
struct SessionFile {
    std::string session, path, sizesPath, head; size_t nNums = 0;
    std::map<long long, Row> rows;
    bool dirty = false; long long writes = 0, fails = 0, loaded = 0, rejected = 0;
    std::string sizesText; bool sizesDirty = false;
    void open(const std::string& sess, const std::string& p, const std::string& sp, const std::string& header, size_t n) {
        session = sess; path = p; sizesPath = sp; head = header; nNums = n; rows.clear(); dirty = false; sizesText.clear(); sizesDirty = false;
    }
    // this session's rows (bounded: 6 MB, 6,000 lines of <= 4 KB), every line validated; false = no file / unreadable
    bool load() {
        if (path.empty()) return false;
        std::vector<std::string> lines;
        if (!nl::readBoundedLines(path, lines, (size_t)SESSION_MAX_BYTES, SESSION_MAX_LINES, 4096)) return false;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (lines[i].empty() || lines[i][0] == '#' || lines[i].compare(0, 9, "bar_time|") == 0) continue;
            Row r; if (parseRow(lines[i], nNums, r)) { rows[r.t] = r; loaded++; } else rejected++;
        }
        return true;
    }
    bool loadSizes(SizeDist& aggr, SizeDist& rest) {
        std::vector<std::string> lines;
        if (sizesPath.empty() || !nl::readBoundedLines(sizesPath, lines, 512 * 1024, 4, 300000)) return false;
        SizeDist a, r; bool ga = false, gr = false;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (lines[i].compare(0, 5, "AGGR|") == 0) ga = a.load(lines[i].substr(5));
            else if (lines[i].compare(0, 5, "REST|") == 0) gr = r.load(lines[i].substr(5));
        }
        if (!ga || !gr) return false;
        aggr = a; rest = r; return true;
    }
    void put(const Row& r) { if (rows.count(r.t)) return; if (rows.size() >= SESSION_MAX_LINES - 4) return; rows[r.t] = r; dirty = true; }   // once per candle
    void sizes(const SizeDist& aggr, const SizeDist& rest) { const std::string t = "AGGR|" + aggr.save() + "\nREST|" + rest.save() + "\n"; if (t != sizesText) { sizesText = t; sizesDirty = true; } }
    void save() {
        if (path.empty()) return;
        if (dirty) {
            std::string o = head;
            for (std::map<long long, Row>::const_iterator it = rows.begin(); it != rows.end(); ++it) o += rowLine(it->second) + "\n";
            if (publishFile(path, o)) { writes++; dirty = false; } else fails++;
        }
        if (sizesDirty && !sizesPath.empty()) { if (publishFile(sizesPath, sizesText)) sizesDirty = false; else fails++; }
    }
};

// events per minute: 60 one-second buckets by arrival
struct Rolling {
    long long n[60]; long long sec[60];
    Rolling() { for (int i = 0; i < 60; ++i) { n[i] = 0; sec[i] = -1; } }
    void add(long long ms, long long k = 1) { if (ms < 0) return; const long long s = ms / 1000; const int i = (int)(s % 60); if (sec[i] != s) { sec[i] = s; n[i] = 0; } n[i] += k; }
    long long perMin(long long nowMs) const { const long long s = nowMs / 1000; long long t = 0; for (int i = 0; i < 60; ++i) if (sec[i] >= 0 && s >= sec[i] && s - sec[i] < 60) t += n[i]; return t; }
};

static const int SLICE_MAX = 5000;                 // events per call at most
static const long long SLICE_BUDGET_MS = 8;        // and about this many ms
static const long long WAIT_MS = 60000;            // no event for 60 s after the 1st open: one 2nd try with NULL
static const long long DECIDE_DELAY_MS = 500;      // a closed candle is decided 0.5 s after it closed with the drain caught up
static const long long FINAL_GRACE_MS = 5000;      // ... or 5 s after it closed whatever the backlog (late events then counted, not used)

} // namespace off

// =====================================================================================================================
// PART 2 - host side (Investor/RT SDK). Define OFF_SDK after including irtsdk.h.
// =====================================================================================================================
#ifdef OFF_SDK
#include "HostSlot.h"
#include <chrono>
#include <climits>
#include <new>
#include <sstream>

namespace off {

// bDboSubscribed is private: read through a pointer-to-member from an explicit instantiation ([temp.explicit] lets it name private
// members; no layout assumptions) - as lsMboCheck 1.1.0
template <class Tag, typename Tag::type M> struct Rob { friend typename Tag::type robbed(Tag) { return M; } };
struct DboFlag { typedef RTBOOL cppExtension::DEPTH_BY_ORDER::*type; friend type robbed(DboFlag); };
template struct Rob<DboFlag, &cppExtension::DEPTH_BY_ORDER::bDboSubscribed>;

inline short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }

// ---- the order feed of one chart (lsMboCheck 1.1.0 pattern) --------------------------------------------------------
struct Conn {
    cppExtension::DEPTH_BY_ORDER* dbo = nullptr;
    int attempt = 0, flag = -1; bool blocked = false, dead = false;
    long long openMs = -1, lastResubMs = -1, lastEventMs = -1;
    long long faults = 0, slices = 0, capHits = 0, backlogRun = 0, longestBacklog = 0, events = 0;
    bool lastCapped = false; std::string via; Rolling roll;
    Conn() {}
    Conn(const Conn&) = delete; Conn& operator=(const Conn&) = delete;
    ~Conn() { delete dbo; }
    void reset() { delete dbo; dbo = nullptr; attempt = 0; flag = -1; blocked = dead = false; openMs = lastResubMs = lastEventMs = -1; lastCapped = false; via.clear(); }
    bool due(long long now) const {
        if (blocked || dead || attempt >= 2) return false;
        if (attempt == 1) return lastEventMs < 0 && openMs >= 0 && now - openMs >= WAIT_MS;
        return true;
    }
    // chart context only (calc / draw). 1st: the chart symbol spelled out; after 60 s with no event, one 2nd try with NULL.
    // A crash-guard file marks each open; if IRT dies inside it, the next start does not open it again (delete it to retry).
    void open(const std::string& symbol, const std::string& guard, long long now, std::string& state) {
        if (!due(now)) return;
        if (attempt == 0 && !guard.empty() && fileStamp(guard) >= 0) { blocked = true; attempt = 2; state = "IRT closed while opening the order feed last time: delete " + guard + " to retry"; return; }
        if (!guard.empty()) { std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening DEPTH_BY_ORDER try " << (attempt + 1) << "\n"; }
        try {
            if (attempt == 0) {
                const bool named = !symbol.empty();
                dbo = new (std::nothrow) cppExtension::DEPTH_BY_ORDER(named ? symbol.c_str() : NULL);
                via = named ? "chart symbol " + symbol : "default (NULL = this chart)";
                attempt = named ? 1 : 2;
            } else {
                delete dbo; dbo = nullptr;                                 // its destructor unsubscribes
                dbo = new (std::nothrow) cppExtension::DEPTH_BY_ORDER(NULL);
                via = "default (NULL = this chart), 2nd try";
                attempt = 2;
            }
        } catch (...) { faults++; dead = true; }
        flag = -1; openMs = now;
        if (!dbo && !dead) { dead = true; state = "order feed unavailable (out of memory)"; }
        if (!guard.empty()) std::remove(guard.c_str());
    }
    // at most SLICE_MAX events or ~SLICE_BUDGET_MS; the rest waits for the next call. Re-subscribe asked at most once a second.
    template <class Clock, class Sink> int drain(long long now, Clock clock, Sink sink) {
        lastCapped = false;
        if (!dbo || dead) return 0;
        if (flag != 1 && lastResubMs >= 0 && now - lastResubMs < 1000 && now >= lastResubMs) return 0;
        const long long t0 = clock(); int n = 0; bool cap = false;
        try {
            if (!(dbo->*robbed(DboFlag()))) lastResubMs = now;
            for (;;) {
                if (n >= SLICE_MAX) { cap = true; break; }
                if ((n & 511) == 511 && clock() - t0 > SLICE_BUDGET_MS) { cap = true; break; }
                if (dbo->getNext() != RTX_OK) break;
                ++n; sink(dbo->dbo);
            }
            flag = (dbo->*robbed(DboFlag())) ? 1 : 0;                      // after the reads: getNext may have just subscribed
        } catch (...) { faults++; dead = true; }                            // never re-enter a faulting call
        if (n > 0) { lastEventMs = now; slices++; events += n; roll.add(now, n); }
        if (cap) { capHits++; backlogRun++; if (backlogRun > longestBacklog) longestBacklog = backlogRun; } else backlogRun = 0;
        lastCapped = cap;
        return n;
    }
    std::string stateText(long long now) const {
        if (blocked) return std::string();
        if (dead) return "order feed fault: reload indicator";
        if (flag == 0) return "MBO: not subscribed";
        if (lastEventMs < 0) return openMs >= 0 && now - openMs >= WAIT_MS && attempt >= 2 ? "MBO: no events" : "MBO: waiting";
        return "ready";
    }
};

// ---- drawing ----------------------------------------------------------------------------------------------------------------
static const COLOR C_WHITE = 0x00FFFFFF, C_BLACKC = 0x00000000, C_NUMS = 0x009CA3AF, C_NEUTRAL = 0x00D8DDE3;
static const COLOR C_GREEN_BAR = 0x001F9D55, C_RED_BAR = 0x00962D2D, C_GREEN_SPIKE = 0x0000FF00, C_RED_SPIKE = 0x00FF3B3B;
inline COLOR implColor(int impl) { return impl == BULL ? C_GREEN_SPIKE : impl == BEAR ? C_RED_SPIKE : C_NEUTRAL; }
inline void fillRect(int l, int t, int r, int b, COLOR border, COLOR fill, int w = 0) {
    cppExtension::RCT rc; rc.set(coord(l), coord(t), coord(r), coord(b)); rc.draw((short)w, border, fill, cppExtension::DRAW_OPAQUE, PAT_SOLID);
}
struct BarLook { bool ok = false; long long v[2] = {0, 0}; bool spike[2] = {false, false}; std::string let[2]; long long dash[2] = {-1, -1}; bool ratioOk[2] = {false, false}; double ratio[2] = {0, 0}; };
struct PaneGeo { int left = 0, top = 0, right = 0, bottom = 0, yBase = 0, px = 1, reserve = 0, edge = 0; };
// the pane geometry: bars from yBase = bottom - 1 up px pixels; the text area ends at the last bar (never the empty space right
// of price) and never over the price axis
inline PaneGeo paneGeo(cppExtension* x, int pos) {
    PaneGeo g; cppExtension::RCT pane; pane.getPaneRect(false);
    g.left = pane.left; g.top = pane.top; g.right = pane.right; g.bottom = pane.bottom;
    g.reserve = textReserve(pane.bottom - pane.top); (void)pos;
    g.yBase = pane.bottom - 1; g.px = drawPx(pane.bottom - pane.top, g.reserve);
    cppExtension::RCT scl; scl.getScaleRect();
    int edge = pane.right;
    if (scl.left > pane.left && scl.left < edge) edge = scl.left - 2;
    const long n = x->getBarCount();
    if (n > 0) { cppExtension::PNT e; if (e.set((int)n - 1, 0.0f, cppExtension::kBarRight) == RTX_OK && e.h > pane.left + 120 && e.h < edge) edge = e.h; }
    g.edge = edge;
    return g;
}
// the candles: two bars each (left / right), spike colours, the hovered candle outlined white, white refill dashes, letters
inline void drawBars(cppExtension* x, const PaneGeo& g, int v0, int v1, int slot, long long top, int hov, const COLOR normal[2], const COLOR spike[2],
                     const std::function<int(int)>& xOf, const std::function<BarLook(int)>& look, const std::vector<LBox>& avoid = std::vector<LBox>()) {
    if (top <= 0) return;
    cppExtension::FONT lf; lf.id = cppExtension::HELVETICA; lf.size = 7; lf.style = cppExtension::BOLD;
    std::vector<std::pair<int, BarLook> > lets;
    for (int i = v0; i <= v1; ++i) {
        const BarLook b = look(i); if (!b.ok) continue;
        const int xc = xOf(i); if (xc < g.left || xc > g.right) continue;
        const Pair p = pairIn(xc, slot);
        const short ow = i == hov ? 1 : 0;
        int h[2] = {0, 0};
        for (int s = 0; s < 2; ++s) {
            h[s] = barHeight(b.v[s], top, g.px);
            const COLOR c = b.spike[s] ? spike[s] : normal[s];
            if (h[s] > 0) fillRect(p.l[s], g.yBase - h[s], p.r[s], g.yBase, ow ? C_WHITE : c, c, ow);
            if (b.dash[s] >= 0 && h[s] > 0) {                               // the refill dash: inside the bar at min(refills, bar)
                int dh = barHeight(b.dash[s] < b.v[s] ? b.dash[s] : b.v[s], top, g.px); if (dh < 2) dh = 2; if (dh > h[s]) dh = h[s];
                fillRect(p.l[s], g.yBase - dh, p.r[s], g.yBase - dh + 2, C_WHITE, C_WHITE);
            }
        }
        if (!b.let[0].empty() || !b.let[1].empty()) lets.push_back(std::make_pair(i, b));
    }
    x->setFont(lf);
    std::vector<LBox> allLets(avoid);                 // the info box / message line: spike labels never go over them
    if (!lets.empty()) {
        const int bw = std::max(9, (int)x->getTextWidth("W", -1) + 3), bh = 11;
        for (size_t k = 0; k < lets.size(); ++k) {
            const int xc = xOf(lets[k].first); const Pair p = pairIn(xc, slot);
            const int sl = xc - slot / 2, sr = sl + (slot > 2 ? slot - 1 : 1);
            const std::vector<LBox> bx = letterBoxes(sl, sr, (p.l[0] + p.r[0]) / 2, (p.l[1] + p.r[1]) / 2, g.yBase - 1, g.top + g.reserve, lets[k].second.let[0], lets[k].second.let[1], bw, bh);
            for (size_t j = 0; j < bx.size(); ++j) {
                allLets.push_back(bx[j]);
                fillRect(bx[j].l, bx[j].t, bx[j].r, bx[j].b, C_WHITE, C_WHITE);
                x->setTextColor(C_BLACKC);
                const char t[2] = { bx[j].c, 0 };
                cppExtension::RCT r; r.set(coord(bx[j].l), coord(bx[j].t), coord(bx[j].r + 1), coord(bx[j].b + 1)); r.drawText(t, true, false);
            }
        }
    }
    // (12:03) the quantity and the ratio above every spike
    std::vector<SpikeIn> sp;
    for (int i = v0; i <= v1; ++i) {
        const BarLook b = look(i); if (!b.ok || (!b.spike[0] && !b.spike[1])) continue;
        const int xc = xOf(i); if (xc < g.left || xc > g.right) continue;
        const Pair p = pairIn(xc, slot);
        for (int s = 0; s < 2; ++s) if (b.spike[s]) {
            SpikeIn si; si.cx = (p.l[s] + p.r[s]) / 2; si.barTop = g.yBase - barHeight(b.v[s], top, g.px); si.side = s;
            si.q = std::to_string(b.v[s]); si.x = b.ratioOk[s] ? ratioText(b.ratio[s]) : std::string();
            sp.push_back(si);
        }
    }
    if (sp.empty()) return;
    cppExtension::FONT sf; sf.id = cppExtension::HELVETICA; sf.size = 7; sf.style = cppExtension::PLAIN; x->setFont(sf);
    const std::vector<SLabel> L = spikeLabels(sp, g.top + g.reserve, allLets, [&](const std::string& t) { return t.empty() ? 0 : (int)x->getTextWidth(t.c_str(), -1); });
    for (size_t k = 0; k < L.size(); ++k) {
        x->setTextColor(spike[L[k].side]);
        auto put = [&](const std::string& t, int y) { if (t.empty()) return; const int w = x->getTextWidth(t.c_str(), -1), cx = (L[k].l + L[k].r) / 2;
            cppExtension::RCT r; r.set(coord(cx - w / 2), coord(y), coord(cx - w / 2 + w + 2), coord(y + LABEL_LH)); r.drawText(t.c_str(), false, false); };
        put(L[k].x, L[k].t); put(L[k].q, L[k].t + LABEL_LH);
    }
}
// ---- (12:05) THE INFO BOX - Rassul "all values go in the info box, which I should be able to see as I move my mouse over different
// bars": solid border, black, the hovered candle's time (12-hour CT) or "Live" centred on top, white labels, values in the side's colour,
// FIXED size (measured on a template value, values cut to fit), 9-spot Position (default Top left), never past the last bar.
// Two-column boxes (PullStats / StackStats): a column header row (Bids / Offers), then label | left value | right value.
static const COLOR C_BOX_BORDER = 0x009CA3AF;
inline Box infoRect(cppExtension* x, const PaneGeo& g, int pos, const Info& I) {
    cppExtension::FONT f; f.id = cppExtension::HELVETICA; f.size = 9; f.style = cppExtension::BOLD; x->setFont(f);
    int lw = 0, vw = (int)x->getTextWidth(I.tmpl.c_str(), -1);
    for (size_t k = 0; k < I.rows.size(); ++k) lw = std::max(lw, (int)x->getTextWidth(I.rows[k].label.c_str(), -1));
    if (I.twoCol) for (int s = 0; s < 2; ++s) vw = std::max(vw, (int)x->getTextWidth(I.colHead[s].c_str(), -1));
    const int hw = std::max((int)x->getTextWidth("12:59 PM", -1), (int)x->getTextWidth(I.head.c_str(), -1));
    const int pad = 5, lh = 13, rows = 1 + (I.twoCol ? 1 : 0) + (int)I.rows.size();
    const int W = std::max(I.twoCol ? pad + lw + 10 + vw + 10 + vw + pad : pad + lw + 12 + vw + pad, hw + 2 * pad), H = pad + lh * rows + pad - 2;
    const Box area = { g.left + 3, g.top + 3, g.edge - 3, g.bottom - 3 };
    return placeIn(area, W, H, pos);
}
inline void drawInfo(cppExtension* x, const Box& b, const Info& I, const COLOR col[2]) {
    cppExtension::FONT f; f.id = cppExtension::HELVETICA; f.size = 9; f.style = cppExtension::BOLD; x->setFont(f);
    fillRect(b.l, b.t, b.r, b.b, C_BOX_BORDER, C_BLACKC, 1);
    const int pad = 5, lh = 13;
    int lw = 0; for (size_t k = 0; k < I.rows.size(); ++k) lw = std::max(lw, (int)x->getTextWidth(I.rows[k].label.c_str(), -1));
    const int W = b.r - b.l, vw = I.twoCol ? (W - 2 * pad - lw - 20) / 2 : W - 2 * pad - lw - 12;
    auto txt = [&](int xx, int y, const std::string& t, COLOR c) { x->setTextColor(c); const int w = x->getTextWidth(t.c_str(), -1); cppExtension::RCT r; r.set(coord(xx), coord(y), coord(xx + w + 2), coord(y + 12)); r.drawText(t.c_str(), false, false); };
    auto fit = [&](const std::string& t) { return fitText(t, vw, [&](const std::string& u) { return (int)x->getTextWidth(u.c_str(), -1); }); };
    { const int w = x->getTextWidth(I.head.c_str(), -1); txt(b.l + (W - w) / 2, b.t + pad - 1, I.head, C_WHITE); }
    const int rightCol[2] = { b.l + pad + lw + 10 + vw, b.r - pad };
    int row = 1;
    if (I.twoCol) {
        for (int s = 0; s < 2; ++s) { const std::string t = fit(I.colHead[s]); txt(rightCol[s] - (int)x->getTextWidth(t.c_str(), -1), b.t + pad - 1 + lh, t, col[s]); }
        ++row;
    }
    for (size_t k = 0; k < I.rows.size(); ++k, ++row) {
        const int y = b.t + pad - 1 + lh * row;
        txt(b.l + pad, y, I.rows[k].label, C_WHITE);
        for (int s = 0; s < (I.twoCol ? 2 : 1); ++s) {
            const std::string t = fit(I.rows[k].v[s]); if (t.empty()) continue;
            const int sd = I.rows[k].side[s];
            txt((I.twoCol ? rightCol[s] : b.r - pad) - (int)x->getTextWidth(t.c_str(), -1), y, t, sd < 0 ? C_WHITE : col[sd]);
        }
    }
}
// the ONE message line, centred at the top of the pane (12:05: the numbers line moved into the box). If the box sits in the top band,
// the line is centred in the wider free part beside it; never over the box, never past the last bar. Returns its rectangle.
inline LBox drawMessage(cppExtension* x, const PaneGeo& g, const Box* box, const std::string& l1, int impl) {
    int L = g.left + 3, R = g.edge - 3; const int y = g.top + 2, h = 15;
    if (box && box->t < y + h && box->b > y) {
        const int leftW = box->l - 4 - L, rightW = R - (box->r + 4);
        if (rightW >= leftW) L = box->r + 4; else R = box->l - 4;
    }
    LBox none = { 0, 0, -1, -1, 0, 0 };
    if (R - L < 20 || l1.empty()) return none;
    cppExtension::FONT f1; f1.id = cppExtension::HELVETICA; f1.size = 10; f1.style = cppExtension::BOLD; x->setFont(f1);
    const std::string t = fitText(l1, R - L, [&](const std::string& u) { return (int)x->getTextWidth(u.c_str(), -1); });
    if (t.empty()) return none;
    const int w = x->getTextWidth(t.c_str(), -1), xx = (L + R) / 2 - w / 2;
    x->setTextColor(implColor(impl));
    cppExtension::RCT r; r.set(coord(xx), coord(y), coord(xx + w + 2), coord(y + h)); r.drawText(t.c_str(), false, false);
    LBox out = { xx, y, xx + w + 2, y + h, 0, 0 }; return out;
}

#ifdef OFF_TEST_CLOCK
extern long long off_test_now_ms;
extern void* off_test_cursor_wnd;
inline long long steadyMs() { return off_test_now_ms; }
inline void* cursorWindow() { return off_test_cursor_wnd; }
#else
inline long long steadyMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
#if defined(_WIN32)
#pragma comment(lib, "user32.lib")   // (2026-10-13) GetCursorPos / WindowFromPoint - link fix
inline void* cursorWindow() { POINT p; if (!GetCursorPos(&p)) return NULL; return (void*)WindowFromPoint(p); }
#else
inline void* cursorWindow() { return NULL; }
#endif
#endif

// ---- the windows the chart's mouse events came from (hover off the chart -> Live), as lsBidAskStats 1.1.0 ----------------------
struct HoverTrack {
    Hover hover; std::vector<void*> wnds; long long redraws = 0;
    void seen() {
        void* w = cursorWindow(); if (!w) return;
        std::vector<void*>::iterator it = std::find(wnds.begin(), wnds.end(), w);
        if (it != wnds.end()) wnds.erase(it);
        wnds.push_back(w);
        if (wnds.size() > 8) wnds.erase(wnds.begin());
    }
    bool left() {
        if (hover.bar < 0 || wnds.empty()) return false;
        void* w = cursorWindow(); if (!w) return false;
        if (std::find(wnds.begin(), wnds.end(), w) != wnds.end()) return false;
        return hover.set(-1);
    }
};

// =====================================================================================================================
// MboPane<L>: the whole host of lsPullStats / lsStackStats (L = ps::Logic / st::Logic). Own pane, two bars per candle from the
// order feed near price, letters, spike labels, the info box + message line, the session record, the per-market params file.
// =====================================================================================================================
struct PaneSettings { int ratioBars = RATIO_DEFAULT, pos = POS_DEFAULT, nearTicks = 0; SpikeCfg spike; };
inline bool parsePaneSettings(const std::string& line, PaneSettings& out) {
    if (line.empty() || line.size() > 40) return false;
    std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t c = line.find(',', a); f.push_back(line.substr(a, c == std::string::npos ? std::string::npos : c - a)); if (c == std::string::npos) break; a = c + 1; }
    if (f.size() != 6) return false;
    for (size_t k = 0; k < f.size(); ++k) if (!allDigits(f[k], 5)) return false;
    PaneSettings s; s.ratioBars = std::atoi(f[0].c_str()); s.pos = std::atoi(f[1].c_str());
    s.spike.avgT = std::atoi(f[2].c_str()); s.spike.jumpT = std::atoi(f[3].c_str()); s.spike.lookback = std::atoi(f[4].c_str()); s.nearTicks = std::atoi(f[5].c_str());
    if (s.ratioBars < RATIO_LO || s.ratioBars > RATIO_HI || s.pos < 0 || s.pos > 8 || s.spike.avgT > 100 || s.spike.jumpT > 100 || s.spike.lookback > SPIKE_LOOKBACK_MAX || s.nearTicks > nl::NEAR_MAX) return false;
    out = s; return true;
}
inline std::string paneSettingsLine(const PaneSettings& s) {
    return std::to_string(s.ratioBars) + "," + std::to_string(s.pos) + "," + std::to_string(s.spike.avgT) + "," + std::to_string(s.spike.jumpT) + "," + std::to_string(s.spike.lookback) + "," + std::to_string(s.nearTicks);
}
inline int tenthsOf(float v) { if (!(v >= 0) || v > 10.04f) return -1; return (int)(v * 10.0f + .5f); }

template <class L> struct PaneState {
    bool busy = false;
    std::string identity, market, symbol, state = "starting", lastStatusText;
    int spb = 0; double tick = 0;
    Conn feed;
    Engine e; Acc scratch; nl::Zone zone; double atr = 0; long atrCount = -1;
    std::map<long long, Acc> accs;                   // bar end -> what it collected (open bars only)
    std::map<long long, long long> closedAt;         // bar end -> steady ms it was first seen closed (not decided yet)
    PaneSeries<L> ser;                               // decided bars
    std::map<long long, int> barEnds;                // chart bar end -> index
    long long formingKey = 0, firstBarKey = -1, lastClosedKey = 0;
    long long lateDropped = 0, noBar = 0, orphans = 0, finalized = 0, decidedLate = 0;
    std::string curSession; SessionFile file; bool loaded = false; long long sessionResets = 0;
    nl::Cache lv; std::vector<nl::Level> levels;
    ParamCache params; PaneSettings set; SpikeCfg dlgSpike; long long settingsCheckMs = -1, settingsStamp = -2;
    HoverTrack hov; long long axisTop = 0; int paneH = 0;
    long long statusMs = -1, changed = 0;
};

template <class L> class MboPane : public cppExtension {
public:
    typedef PaneState<L> S_;
    MboPane() : cppExtension() {}
    virtual int draw(void) {
        pump();
        S_* S = slot_.get(this, false);
        if (S) S->hov.left();
        if (S) { try { render(*S); } catch (...) { S->state = "draw error"; } }
        return RTX_OK;
    }
    virtual int parmsLoad(void) { return RTX_OK; }                         // loading never saves
    virtual int parmsApply(void) { saveSettings(); return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { saveSettings(); return RTX_OK; }
    virtual int mouse(RTX_EVENT* ev) {
        if (!ev) return RTX_FAIL;
        S_* S = slot_.get(this, false); if (!S) return RTX_FAIL;
        if (ev->type != E_MOUSE_MOVE && ev->type != E_MOUSE_HOVER) return RTX_FAIL;
        S->hov.seen();
        if (S->hov.hover.set(hoverIndex(ev->v.mouse.bar, getBarCount()))) { ++S->hov.redraws; invalidateChart(false); }
        return RTX_FAIL;                                                   // never swallow the event
    }
    virtual int scale(int iStartBar, int iEndBar, double* dMin, double* dMax) {
        double mn = 0, mx = 10;
        S_* S = slot_.get(this, false);
        if (S) {
            const long count = getBarCount();
            RTARRAYI times(barDateTime);
            long long top = 0;
            if (count > 0 && times.count >= count) {
                const int last = (int)count - 1;
                int a = std::max(0, iStartBar), b = std::min(last, iEndBar);
                if (b < a) { a = std::max(0, last - 200); b = last; }
                if (b - a > 20000) a = b - 20000;
                PaneBar lb; const bool live = liveBar(*S, lb);
                for (int i = a; i <= b; ++i) {
                    const long long bt = (long long)times[i];
                    if (i == last) { if (live && bt == S->formingKey) top = std::max(top, std::max(L::value(lb.r, 0, BID), L::value(lb.r, 0, ASK))); continue; }
                    typename std::map<long long, PaneBar>::const_iterator it = S->ser.bars.find(bt);
                    if (it != S->ser.bars.end() && it->second.ok) top = std::max(top, std::max(L::value(it->second.r, 0, BID), L::value(it->second.r, 0, ASK)));
                }
            }
            S->axisTop = top;
            axisFor(top, S->paneH, drawPx(S->paneH, textReserve(S->paneH)), mn, mx);
        }
        if (dMin) *dMin = mn;
        if (dMax) *dMax = mx;
        return RTX_OK;
    }
    void calcHook() { pump(); S_* S = slot_.get(this, false); if (S && S->hov.left()) { ++S->hov.redraws; invalidateChart(false); } }
    void release() { slot_.release(this); }
    S_* testState() { return slot_.get(this, false); }
    void pump() {
        S_* P = slot_.get(this, true);
        if (!P || P->busy) return;
        S_& S = *P; S.busy = true;
        struct Unbusy { S_* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
        try {
            identify(S);
            const long long now = steadyMs();
            settings(S, now);
            if (refreshParams(S.params, flexDir(), S.market, now)) applyParams(S);
            const long count = getBarCount();
            if (S.tick <= 0) S.state = "native tick size unavailable";
            else if (count < 2) S.state = "waiting for bars";
            else {
                bars(S, count, now);
                S.levels = nl::levels(S.lv, flexDir().empty() ? std::string() : flexDir() + "\\", S.market, S.symbol, now, (long long)std::time(NULL));
                load(S);
                std::string st;
                S.feed.open(S.symbol, dir().empty() ? std::string() : dir() + "\\_opening-" + stem(S) + ".txt", now, st);
                if (!st.empty()) S.state = st;
                const double tick = S.tick;
                S.feed.drain(now, steadyMs, [&](const DEPTH_ORDER& o) { route(S, evOf(o, tick, now)); });
                if (S.feed.lastEventMs >= 0 && S.firstBarKey < 0) S.firstBarKey = S.formingKey;
                decide(S, now);
                const std::string fs = S.feed.stateText(now);
                if (!fs.empty()) S.state = fs;
            }
            status(S, now);
        } catch (...) { S.state = "calculation error"; }
    }
private:
    HostSlot<S_> slot_;
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + "\\" + L::name(); }
    std::string stem(const S_& S) { return S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb); }
    std::string placePath(const std::string& mkt) { const std::string f = flexDir(); return f.empty() || mkt.empty() ? std::string() : f + "\\" + L::name() + ".place-" + mkt + ".txt"; }
    std::string ctOf(long long t) { struct tm x; std::memset(&x, 0, sizeof(x)); if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return std::string(); return clockShort(x.tm_hour, x.tm_min); }
    std::string sessionKey(long long t) { struct tm x; std::memset(&x, 0, sizeof(x)); if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return std::string(); return sessionOf(x.tm_year + 1900, x.tm_mon + 1, x.tm_mday, x.tm_hour); }
    std::string stamp(long long t) {
        struct tm x; std::memset(&x, 0, sizeof(x));
        if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return "n/a";
        char b[64]; std::snprintf(b, sizeof(b), "%04d-%02d-%02d %02d:%02d:%02d", x.tm_year + 1900, x.tm_mon + 1, x.tm_mday, x.tm_hour, x.tm_min, x.tm_sec); return b;
    }
    void identify(S_& S) {
        char rb[32]; std::memset(rb, 0, sizeof(rb));
        const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
        const std::string root = rs ? rs : "", sym = sy ? sy : "";
        const int spb = getSecondsPerBar();
        double tick = (double)getProperty(SYM_TICKINCR); if (!finite(tick) || tick <= 0 || tick >= 1000) tick = 0;
        const std::string id = root + "|" + sym + "|" + std::to_string(spb);
        if (id == S.identity && tick == S.tick) return;
        S.feed.reset();                                                    // a new symbol / bar size: start clean (unsubscribes)
        S.e = Engine(); S.accs.clear(); S.closedAt.clear(); S.ser.bars.clear(); S.barEnds.clear();
        S.formingKey = 0; S.firstBarKey = -1; S.lastClosedKey = 0; S.atr = 0; S.atrCount = -1; S.loaded = false; S.curSession.clear(); S.file = SessionFile();
        S.identity = id; S.symbol = sym; S.spb = spb; S.tick = tick; S.market = marketName(root.empty() ? sym : root);
        S.zone = nl::zoneFor(S.market); S.params = ParamCache();
        S.settingsStamp = -2; S.settingsCheckMs = -1; S.hov.hover.set(-1); S.hov.wnds.clear(); S.lastStatusText.clear();
    }
    void applyParams(S_& S) {
        const Params& P = S.params.load.p;
        S.e.refillMs = P.refillWindowMs; S.e.closeTicks = P.closeTicks;
        S.ser.spike = effectiveSpike(S.dlgSpike, P);                       // new bars only: a closed bar's colour is final
    }
    void settings(S_& S, long long now) {
        if (S.settingsCheckMs >= 0 && now >= S.settingsCheckMs && now - S.settingsCheckMs < 1000) return;
        S.settingsCheckMs = now;
        const std::string p = placePath(S.market);
        const long long st = p.empty() ? -1 : fileStamp(p);
        if (st == S.settingsStamp) return;
        const bool first = S.settingsStamp == -2;
        S.settingsStamp = st;
        PaneSettings ns;
        if (st >= 0) {
            std::ifstream f(p.c_str(), std::ios::binary); std::string ln;
            if (f && std::getline(f, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); PaneSettings c; if (parsePaneSettings(ln, c)) ns = c; }
        }
        const bool renear = ns.nearTicks != S.set.nearTicks;
        S.set = ns; S.dlgSpike = ns.spike;
        const SpikeCfg eff = effectiveSpike(S.dlgSpike, S.params.load.p);
        if (S.ser.ratioBars != ns.ratioBars || S.ser.spike != eff) {      // a dialog change: every recorded bar re-decided
            S.ser.ratioBars = ns.ratioBars; S.ser.spike = eff;
            if (!first) { const Params P = S.params.load.p; S.ser.recompute([&](PaneBar& b) { L::decide(b, P, false); }); }
        }
        if (renear) S.atrCount = -1;
    }
    void saveSettings() {
        PaneSettings s;
        s.ratioBars = getIntegerValue(0); s.pos = getListIndex(1);
        s.spike.avgT = tenthsOf(getFloatValue(2)); s.spike.jumpT = tenthsOf(getFloatValue(3)); s.spike.lookback = getIntegerValue(4); s.nearTicks = getIntegerValue(5);
        PaneSettings chk;
        if (s.spike.avgT < 0 || s.spike.jumpT < 0 || s.spike.lookback < 0 || s.nearTicks < 0 || !parsePaneSettings(paneSettingsLine(s), chk)) return;   // never overwrite with garbage
        char b[32]; std::memset(b, 0, sizeof(b)); const char* rs = getRootSymbol(b); const char* sy = getSymbol();
        const std::string root = rs ? rs : "";
        const std::string p = placePath(marketName(root.empty() ? (sy ? sy : "") : root)); if (p.empty()) return;
        makeDir(flexDir());
        publishFile(p, paneSettingsLine(s) + "\n");
        S_* S = slot_.get(this, false);
        if (S) { S->settingsCheckMs = -1; S->settingsStamp = -1 - 3; }    // re-read on the next pass (not "first")
    }
    // chart bars: bar ends -> index, the forming bar, ATR of closed bars and the near band, the fallback last price
    void bars(S_& S, long count, long long now) {
        RTARRAYI times(barDateTime); RTARRAY high(barHigh), low(barLow), close(barClose);
        if (times.count < count || high.count < count || low.count < count || close.count < count) { S.state = "chart data arrays unavailable"; return; }
        const long first = std::max(0L, count - 1500L);
        if (S.barEnds.empty() || S.barEnds.rbegin()->first != (long long)times[(int)count - 1] || (long)S.barEnds.size() != count - first) {
            S.barEnds.clear();
            for (long i = first; i < count; ++i) { const long long bt = (long long)times[(int)i]; if (bt > 0) S.barEnds[bt] = (int)i; }
        }
        S.formingKey = (long long)times[(int)count - 1];
        if (count != S.atrCount) {                                         // ATR from CLOSED bars only (newest first)
            S.atrCount = count;
            std::vector<double> rg;
            for (int i = (int)count - 2; i >= 0 && rg.size() < 20; --i) rg.push_back((double)high[i] - (double)low[i]);
            S.atr = nl::atrOf(rg, S.spb);
            S.e.near = nl::nearBand(S.atr, S.tick, S.zone, S.set.nearTicks);
        }
        Tick px = 0;
        if (toTick((double)close[(int)count - 1], S.tick, px)) S.e.fallbackRef(px, now);
        { std::map<long long, int>::const_reverse_iterator r = S.barEnds.rbegin(); if (r != S.barEnds.rend()) { ++r; if (r != S.barEnds.rend()) S.lastClosedKey = r->first; } }
        // a new trading session: this session's file, the size distributions start over
        const std::string ss = sessionKey(S.formingKey);
        if (!ss.empty() && ss != S.curSession) {
            if (!S.curSession.empty()) { S.file.save(); S.e.newSession(); S.sessionResets++; }
            S.curSession = ss; S.loaded = false;
        }
    }
    // an event to the candle containing its own stamp; one that cannot be counted (no bar, a decided bar, a bar before the stream
    // opened - e.g. a snapshot of old orders) still teaches the engine its order (a scratch candle, thrown away)
    void route(S_& S, const Ev& x) {
        const long long key = eventKey(S.barEnds, S.formingKey, S.spb, x.stamp);
        const bool noBar = key < 0, decided = !noBar && S.ser.bars.count(key) > 0, before = !noBar && S.firstBarKey > 0 && key < S.firstBarKey;
        const bool early = !noBar && S.firstBarKey < 0 && key < S.formingKey;      // before the first event of this load
        if (noBar || decided || before || early) {
            if (noBar) S.noBar++; else S.lateDropped++;
            S.e.apply(S.scratch, x);
            if (S.scratch.events > 20000) S.scratch = Acc();
            return;
        }
        S.e.apply(S.accs[key], x);
    }
    std::string levelAt(S_& S, long long key) {
        std::map<long long, int>::const_iterator i = S.barEnds.find(key);
        if (i == S.barEnds.end() || S.e.near <= 0) return std::string();
        RTARRAY close(barClose); if (close.count <= i->second) return std::string();
        return nl::nearestLevel(S.levels, (double)close[i->second], (double)S.e.near * S.tick);
    }
    Row rowOf(long long key, const PaneBar& b) {
        Row r; r.t = key; r.ct = ctOf(key); r.dbo = 1; recNums(b.r, r.nums);
        for (int s = 0; s < 2; ++s) { r.let[s] = b.let[s]; r.spike[s] = b.spike[s] ? 1 : 0; r.dash[s] = b.dash[s]; }
        r.impl = b.msg.impl; r.level = b.level; r.line1 = b.msg.line1; r.line2 = b.msg.line2;
        return r;
    }
    // closed candles are decided once: 0.5 s after they closed with the drain caught up, or 5 s after they closed
    void decide(S_& S, long long now) {
        bool any = false;
        for (typename std::map<long long, Acc>::iterator it = S.accs.begin(); it != S.accs.end() && it->first < S.formingKey;) {
            const long long key = it->first;
            if (!S.barEnds.count(key)) { S.closedAt.erase(key); S.accs.erase(it++); S.orphans++; continue; }   // a predicted bar the chart never made
            std::map<long long, long long>::iterator c = S.closedAt.find(key);
            if (c == S.closedAt.end()) c = S.closedAt.insert(std::make_pair(key, now)).first;
            const long long age = now >= c->second ? now - c->second : 0;
            if (age < FINAL_GRACE_MS && (S.feed.lastCapped || age < DECIDE_DELAY_MS)) { ++it; continue; }
            if (S.feed.lastCapped) S.decidedLate++;
            if (!S.ser.bars.count(key)) {
                const Params& P = S.params.load.p;
                PaneBar nb = S.ser.make(key, S.e.close(it->second, key == S.firstBarKey, P));
                nb.level = levelAt(S, key);                                 // decided once, at the close
                L::decide(nb, P, false);
                S.ser.bars[key] = nb; S.finalized++; any = true;
                if (nb.ok && sessionKey(key) == S.curSession) S.file.put(rowOf(key, nb));
            }
            S.closedAt.erase(c); S.accs.erase(it++);
        }
        if (any) { S.file.sizes(S.e.aggrDist, S.e.restDist); S.file.save(); S.changed++; invalidateChart(false); }
        if (S.ser.bars.size() > 5000) S.ser.trim(5000);
        while (S.accs.size() > 16) S.accs.erase(S.accs.begin());          // bounded: never more than a few open bars
    }
    // this session's record, once per session: its candles come back exactly as they were decided (bars, letters, message)
    void load(S_& S) {
        if (S.loaded || S.curSession.empty()) return;
        S.loaded = true;
        const std::string d = dir(); if (d.empty()) return;
        makeDir(flexDir()); makeDir(d);
        const std::string base = d + "\\" + stem(S) + "-" + S.curSession;
        S.file.open(S.curSession, base + ".csv", base + "-sizes.txt",
                     std::string("# ") + L::name() + " " + L::version() + " " + S.market + " " + S.symbol + " " + std::to_string(S.spb) + "\nbar_time|ct_time|dbo|" + REC_COLS + "|letters_left|letters_right|spike_left|spike_right|dash_left|dash_right|implication|level|message|numbers\n",
                     (size_t)REC_NUMS);
        S.file.load();
        S.file.loadSizes(S.e.aggrDist, S.e.restDist);
        for (std::map<long long, Row>::const_iterator it = S.file.rows.begin(); it != S.file.rows.end(); ++it) {
            const Row& r = it->second; if (r.t >= S.formingKey || S.ser.bars.count(r.t)) continue;   // never the forming bar
            PaneBar b; if (!recFrom(r.nums, 0, b.r)) continue;
            b.ok = true; b.fromFile = true; b.level = r.level;
            for (int s = 0; s < 2; ++s) { b.let[s] = r.let[s]; b.spike[s] = r.spike[s] != 0; b.dash[s] = r.dash[s]; }
            b.msg.impl = r.impl; b.msg.line1 = r.line1; b.msg.line2 = r.line2;
            S.ser.bars[r.t] = b;
        }
        for (typename std::map<long long, PaneBar>::iterator it = S.ser.bars.begin(); it != S.ser.bars.end(); ++it) if (it->second.fromFile) S.ser.stats(it->first, it->second);
    }
    bool liveBar(const S_& S, PaneBar& b) {
        typename std::map<long long, Acc>::const_iterator it = S.accs.find(S.formingKey);
        if (it == S.accs.end() || it->second.events <= 0) return false;
        b = S.ser.make(S.formingKey, S.e.numbers(it->second, S.formingKey == S.firstBarKey, S.params.load.p));   // as of now, nothing recorded
        L::decide(b, S.params.load.p, true);
        return b.ok;
    }
    void status(S_& S, long long now) {
        if (S.statusMs >= 0 && now >= S.statusMs && now - S.statusMs < 15000) return;
        const Engine& e = S.e; const Conn& f = S.feed;
        std::ostringstream o; o.imbue(std::locale::classic());
        o << "VERSION," << L::version() << "\nINDICATOR," << L::name() << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nSTATE," << S.state
          << "\nSUBSCRIBED," << (f.flag < 0 ? "unknown" : f.flag ? "1" : "0") << "\nOPENED_WITH," << (f.via.empty() ? "not opened" : f.via)
          << "\nEVENTS_PER_MIN," << f.roll.perMin(now) << "\nEVENTS_TOTAL," << e.events
          << "\nACTIONS,NEW " << e.perAction[0] << ",MOD " << e.perAction[1] << ",DEL " << e.perAction[2] << ",FILL " << e.perAction[3]
          << "\nUNKNOWN,del " << e.unknownDel << ",mod " << e.unknownMod << ",fill " << e.unknownFill << ",dup_new " << e.dupNew << ",garbage " << e.garbage
          << "\nDROPPED,late " << S.lateDropped << ",no_bar " << S.noBar << ",orphan_bars " << S.orphans << ",decided_with_backlog " << S.decidedLate
          << "\nBOOK,orders " << e.book.size() << ",drops " << e.bookDrops << ",resets " << e.bookResets
          << "\nNEAR_TICKS," << e.near << (S.set.nearTicks > 0 ? " (setting)" : " (auto)") << "\nATR," << S.atr << "\nLAST_PRICE_TICKS," << e.ref
          << "\nREADS,slices " << f.slices << ",cap_hits " << f.capHits << ",longest_backlog " << f.longestBacklog << ",faults " << f.faults
          << "\nSETTINGS," << paneSettingsLine(S.set) << "\nSPIKE_IN_FORCE,avg " << tenthsText(S.ser.spike.avgT) << ",jump " << tenthsText(S.ser.spike.jumpT) << ",lookback " << S.ser.spike.lookback
          << "\nPARAMS_SOURCE," << S.params.load.source << "\nPARAMS," << paramsText(S.params.load.p);
        for (size_t i = 0; i < S.params.load.notes.size() && i < 20; ++i) o << "\nPARAMS_NOTE," << S.params.load.notes[i];
        o << "\nLARGE_NOW,aggressor " << largeThreshold(e.aggrDist, S.params.load.p.largeLotAggr, S.params.load.p.largeLotPct, S.params.load.p.largeLotMin) << " (" << e.aggrDist.total << " orders)"
          << ",resting " << largeThreshold(e.restDist, S.params.load.p.largeLotResting, S.params.load.p.largeLotPct, S.params.load.p.largeLotMin) << " (" << e.restDist.total << " orders)"
          << "\nSESSION," << S.curSession << "\nSESSION_FILE,rows " << S.file.rows.size() << ",loaded " << S.file.loaded << ",rejected " << S.file.rejected << ",writes " << S.file.writes << ",fails " << S.file.fails
          << "\nBARS_DECIDED," << S.finalized << "\nLEVELS," << S.levels.size() << "," << S.lv.note;
        typename std::map<long long, PaneBar>::const_iterator it = S.ser.bars.find(S.lastClosedKey);
        o << "\nLAST_CLOSED_BAR," << S.lastClosedKey << "," << stamp(S.lastClosedKey);
        if (it != S.ser.bars.end()) o << "\nMESSAGE," << implText(it->second.msg.impl) << "," << it->second.msg.line1 << "\nNUMBERS," << it->second.msg.line2 << "\nLETTERS," << it->second.let[0] << "," << it->second.let[1];
        o << "\n";
        const std::string text = o.str(); if (text == S.lastStatusText) return;
        const std::string d = dir(); if (d.empty()) return;
        makeDir(flexDir()); makeDir(d);
        S.statusMs = now;
        if (publishFile(d + "\\status-" + stem(S) + ".txt", text)) S.lastStatusText = text;
    }
    void render(S_& S) {
        RCT pane; pane.getPaneRect(false);
        const long count = getBarCount();
        if (count < 2 || pane.bottom - pane.top < 24 || pane.right - pane.left < 60) return;
        RTARRAYI times(barDateTime);
        if (times.count < count) return;
        const int last = (int)count - 1;
        auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
        int v0 = 0, v1 = last;
        if (getVisibleBars(&v0, &v1) != RTX_OK || v0 < 0 || v1 < v0 || v1 > last) { v1 = last; v0 = last; while (v0 > 0 && xOf(v0 - 1) >= pane.left) --v0; }
        int slot = (int)getPixelsPerBar();
        if (slot <= 0 || slot > 400) slot = v1 > v0 ? std::max(2, std::abs(xOf(v1) - xOf(v1 - 1))) : 8;
        PaneBar lb; const bool haveLive = liveBar(S, lb) && (long long)times[last] == S.formingKey;
        auto barOf = [&](int i) -> const PaneBar* {
            if (i == last) return haveLive ? &lb : nullptr;
            typename std::map<long long, PaneBar>::const_iterator it = S.ser.bars.find((long long)times[i]);
            return it != S.ser.bars.end() && it->second.ok ? &it->second : nullptr;
        };
        long long top = 0;
        for (int i = v0; i <= v1; ++i) { const PaneBar* b = barOf(i); if (b) top = std::max(top, std::max(L::value(b->r, 0, BID), L::value(b->r, 0, ASK))); }
        if (S.axisTop > top) top = S.axisTop;
        S.paneH = pane.bottom - pane.top;
        const PaneGeo g = paneGeo(this, S.set.pos);
        const int hov = S.hov.hover.bar >= 0 && S.hov.hover.bar < last ? S.hov.hover.bar : -1;
        const COLOR normal[2] = { C_GREEN_BAR, C_RED_BAR }, spike[2] = { C_GREEN_SPIKE, C_RED_SPIKE };   // left = bids (green), right = offers (red)
        // (12:05) the info box (hovered candle, else the last closed) and the message line move together
        const int mb = hov >= 0 ? hov : last - 1;
        const long long mt = (long long)times[mb];
        typename std::map<long long, PaneBar>::const_iterator mi = S.ser.bars.find(mt);
        const PaneBar* mbar = mi != S.ser.bars.end() && mi->second.ok ? &mi->second : nullptr;
        Info info = L::info(mbar);
        { struct tm x; std::memset(&x, 0, sizeof(x)); info.head = hov < 0 ? std::string("Live") : (mt > 0 && getLocaltime((RTDATE)mt, &x) ? clock12(x.tm_hour, x.tm_min) : std::string("n/a")); }
        const Box ib = infoRect(this, g, S.set.pos, info);
        std::vector<LBox> avoid(1, LBox{ ib.l, ib.t, ib.r, ib.b, 0, 0 });
        drawBars(this, g, v0, v1, slot, top, hov, normal, spike, xOf, [&](int i) {
            BarLook k; const PaneBar* b = barOf(i); if (!b) return k;
            k.ok = true;
            for (int s = 0; s < 2; ++s) { k.v[s] = L::value(b->r, 0, s); k.spike[s] = b->spike[s]; k.dash[s] = b->dash[s]; k.ratioOk[s] = b->m[0].ratioOk[s]; k.ratio[s] = b->m[0].ratio[s]; if (i != last) k.let[s] = b->let[s]; }
            return k;
        }, avoid);
        const COLOR vcol[2] = { 0x0022C55E, 0x00EF4444 };                 // values: bids (buyers) green, offers (sellers) red
        drawInfo(this, ib, info, vcol);
        Msg m;
        if (mbar) m = mbar->msg;
        else {
            const bool pending = S.accs.count(mt) > 0;
            m.line1 = pending ? "Deciding this candle..." : S.firstBarKey < 0 || mt < S.firstBarKey ? "No order data for this candle (before the indicator loaded)" : "No order data for this candle";
            if (S.state != "ready") m.line1 += " - " + S.state;
        }
        drawMessage(this, g, &ib, line1Of(ctOf(mt), m.line1), m.impl);
    }
};

} // namespace off
#endif // OFF_SDK
#endif
