#ifndef LS_REVSTORY_LOGIC_H
#define LS_REVSTORY_LOGIC_H
// RevStoryLogic.h 0.1.1 (2026-11-11) -- lsRevStory "Reversal Story": every rule of the indicator, no SDK (unit-tested on Linux).
//
// Rassul 2026-11-11 21:00-21:33 CT: "these bids were there getting filled and accumulating .. i need to know what was going on with the bids to
// understand why the reversal happened"; "I want to know how i can have indicators that tell this story"; "create a similar pane that has each
// candles poc and also makes a dash ont he candle. in the indicator pane you can place the read as well as mark the candles with letters like c for
// climax, whether it is a buying or selling climax"; "you need to ensure you appropriately identify climaxes, absorb, exhaustion, fading, flush,
// buyers or sellers etc all of which idetnify the story of the reversal"; "i dont understand how you marked the 63 as a climax" (a climax is heavy
// volume that is STOPPED at a price, not fast selling that runs through prices); "it must be added to the nightly refinement"; "jumpstart this
// indicator for all markets and refine"; "pay attention to session, volatility times, location and context"; "key levels, volume levels, gamma
// levels, vwap, hbo and its tests (like the testing line), whether the market has a pb or its variant ... the hourly rotation and hbo and whether
// its pulling back, testing or going higher with the rotation"; "pay attention to the location of the poc in the candle"; 21:33 "go".
//
// THE STORY of a bottom (a top is the mirror), from the market-by-order feed in 10-second BLOCKS:
//   1 CLIMAX     a block whose market SELLING is far above this market's normal right now (>= cxX x the 95th percentile of the last 30 minutes of
//                blocks, >= cxSide x the buying of the same block, >= minLots) at a new low of the last 3 minutes. The ANCHOR is the price near
//                the block's low where the most was sold into the bid; "shown" = the bid size resting there just before it was first hit.
//   2 ABSORBED   the lots sold into the anchor reach absX x what was shown AND absShare x the climax AND, two blocks or more after the climax, a
//                block ends at / above the anchor. (A climax whose price keeps falling is not a climax here: it ends "ran through" and gets no letter.)
//   3 FADING     each return to the zone sells less than the one before, the last <= fadeX x the climax (read line only).
//   4 EXHAUSTED  a new low under the zone made on <= flushX x the climax lots, no deeper than flushDepth x the climax block's range, and price
//                is back at / above the anchor ("flush").
//   5 INITIATION a block whose market BUYING is >= initX x the 95th percentile and >= initSide x its selling, ending above the zone.
// LETTERS (decided when their candle closes, never changed after): C climax (red = selling climax, green = buying climax), A absorbed,
// E exhausted, I initiation; at most 2 per candle (the first and the last step of that candle). C / A need the climax candle's POC in the third
// of the candle at the climax end with the close on the other side of it (the volume was stopped there). Every letter carries "?" until the
// nightly marks the market proven (LRA-RevStory-<MKT>.csv, PROVEN 1).
// EVERYTHING measured is recorded (ratios, context, also stories that failed a threshold) so the nightly can score and move the thresholds.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
namespace rvs {
static const char* const VERSION = "0.1.1";   // (0.1.1, the same night, his 2nd example ES 21:54 CT) absorbed over the ZONE; fading also while pinned; "below the test line"
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum { BID = 0, ASK = 1 };                    // the feed's side codes (resting bids / offers; on a FILL: BID = the aggressor BOUGHT)
enum { BUY = 0, SELL = 1 };                   // aggressor sides
enum Step { S_NONE = 0, S_CLIMAX = 1, S_ABSORBED = 2, S_EXHAUSTED = 3, S_INIT = 4 };
enum End { E_OPEN = 0, E_COMPLETE = 1, E_RAN = 2, E_NOTABS = 3, E_TIMEOUT = 4, E_DEEP = 5, E_RESET = 6 };
inline const char* endName(int e) { static const char* const n[7] = { "open", "complete", "ran through", "not absorbed", "timeout", "broke deeper", "feed reset" }; return e >= 0 && e < 7 ? n[e] : "?"; }

struct Params {
    int blockSec = 10, baseBlocks = 180, minBase = 30, lookBlocks = 18;   // 10-s blocks; the normal = the last 30 min; a "new low" = of the last 3 min
    double cxX = 2.0, cxSide = 2.0; long long minLots = 20;               // climax
    double logX = 0.6;                                                    // stories from logX x the climax threshold are RECORDED (shown = 0) for the nightly
    int zoneTicks = 2; double absX = 1.5, absShare = 0.4; int absBlocks = 2, absWait = 12; // absorbed: zone = anchor - 1 .. anchor + zoneTicks; decided within absWait blocks
    double fadeX = 0.5, flushX = 0.6, flushDepth = 1.5; int minDepthTicks = 4;
    double initX = 1.0, initSide = 2.0;
    int timeoutSec = 900; double volX = 1.5;                              // in a volatility window the climax threshold is x volX
    int proven = 0;                                                       // 1 = the nightly proved the letters for this market: no "?"
};
inline Params forMarket(const std::string& m) {   // starting values (0.1.0, not yet measured): only the lot floor differs
    Params p; p.minLots = m == "ES" ? 30 : m == "NQ" ? 15 : m == "CL" ? 20 : m == "GC" ? 15 : m == "EU" ? 15 : 8; return p;
}
// LRA-RevStory-<MKT>.csv (the nightly): "KEY|value" lines; unknown keys / garbage are ignored, values are clamped to sane ranges
inline int applyParam(Params& p, const std::string& k, double v) {
    if (!std::isfinite(v)) return 0;
    auto cl = [](double x, double a, double b) { return x < a ? a : x > b ? b : x; };
    if (k == "CX_X") p.cxX = cl(v, 1.0, 10); else if (k == "CX_SIDE") p.cxSide = cl(v, 1.0, 10); else if (k == "MIN_LOTS") p.minLots = (long long)cl(v, 1, 5000);
    else if (k == "ABS_X") p.absX = cl(v, 1.0, 10); else if (k == "ABS_SHARE") p.absShare = cl(v, 0.05, 1.0); else if (k == "FADE_X") p.fadeX = cl(v, 0.1, 1.0); else if (k == "FLUSH_X") p.flushX = cl(v, 0.1, 2.0);
    else if (k == "FLUSH_DEPTH") p.flushDepth = cl(v, 0.5, 5); else if (k == "INIT_X") p.initX = cl(v, 0.5, 10); else if (k == "INIT_SIDE") p.initSide = cl(v, 1.0, 10);
    else if (k == "ZONE_TICKS") p.zoneTicks = (int)cl(v, 1, 12); else if (k == "PROVEN") p.proven = v >= 0.5 ? 1 : 0; else return 0;
    return 1;
}
inline int parseParams(const std::vector<std::string>& lines, const std::string& mkt, Params& p) {   // returns the keys applied; MARKET must match
    bool ok = false; int n = 0; Params q = p;
    for (const std::string& l : lines) { const size_t b = l.find('|'); if (b == std::string::npos) continue; const std::string k = l.substr(0, b), v = l.substr(b + 1);
        if (k == "MARKET") { ok = v == mkt; continue; }
        char* e = nullptr; const double d = std::strtod(v.c_str(), &e); if (e == v.c_str() || (e && *e && *e != '\r')) continue; n += applyParam(q, k, d); }
    if (!ok) return 0;
    p = q; return n;
}

// ---- context (the host fills it once a second; stamped on a story when it starts) ----
struct Ctx { std::string session = "?"; int volWin = 0; std::string hourly; std::string level; int pb = 0; };   // pb from the host: bit 1 = a long Pb / Pboc / Pdoc was decided in the last 30 min, bit 2 = a short one (a story keeps 1 / 0 for its own side)
// the session of a Central wall-clock second-of-day: ON 17:00-02:00, LON 02:00-RTH open, RTH open-close (rthOpen / rthClose in minutes)
inline std::string sessionOf(int ctSec, int rthOpenMin, int rthCloseMin) {
    const int m = ctSec / 60; if (m >= rthOpenMin && m < rthCloseMin) return "RTH"; if (m >= 120 && m < rthOpenMin) return "LON"; return "ON";
}
// volatility windows: the first 5 min of RTH, the last 5 min before the RTH close, 07:30 and 09:00 CT releases (+- 2 min); 0 = none
inline int volWindow(int ctSec, int rthOpenMin, int rthCloseMin) {
    const int s = ctSec, o = rthOpenMin * 60, c = rthCloseMin * 60;
    if (s >= o && s < o + 300) return 1;
    if (s >= c - 300 && s < c) return 2;
    if (std::abs(s - (7 * 3600 + 1800)) <= 120 || std::abs(s - 9 * 3600) <= 120) return 3;
    return 0;
}
inline const char* volName(int w) { return w == 1 ? "RTH open" : w == 2 ? "into the close" : w == 3 ? "release time" : ""; }
// the hourly read from lsStepTrend's facts: dir = the newest step (+1 up / -1 down / 0 none), rot = the hourly rotation, px = the test line (the
// broken level), top = the step candle's high (low), broken = an hourly close beyond the step candle's other end; last / lo / hi = price now (ticks)
inline std::string hourlyText(int dir, int rot, long long px, long long top, bool broken, long long last, long long lo, long long hi, int tol, double tick) {
    if (dir == 0) return "Hrly none";
    const bool up = dir > 0; char b[96]; std::string s = up ? "Hrly up" : "Hrly dn";
    if (rot != 0 && (rot > 0) != up) s += up ? " (rotation dn)" : " (rotation up)";
    if (broken) return s + ", trend broken";
    const bool testing = up ? lo <= px + tol : hi >= px - tol, with = up ? last >= top : last <= top, beyond = up ? hi < px - tol : lo > px + tol;
    std::snprintf(b, sizeof b, "%.10g", (double)px * tick);
    if (beyond) return s + (up ? ", below the test line " : ", above the test line ") + b;   // (0.1.1) price is through the HBO test line (no hourly close has broken the step yet)
    return s + (testing ? std::string(", testing ") + b : with ? std::string(", going with it") : std::string(", pulling back to ") + b);
}

// ---- the candle POC ----
struct VRow { long long tick = 0, vol = 0, dlt = 0; };
struct Poc { long long t = 0; long long tick = 0, vol = 0, dlt = 0, total = 0; double pos = 0.5, closePos = 0.5; bool ok = false; };   // pos: 0 = at the low .. 1 = at the high
inline Poc pocOf(long long tEnd, const std::vector<VRow>& rows, long long lo, long long hi, long long close) {
    Poc p; p.t = tEnd;
    for (const VRow& r : rows) { if (r.vol <= 0) continue; p.total += r.vol; if (!p.ok || r.vol > p.vol || (r.vol == p.vol && r.tick < p.tick)) { p.ok = true; p.tick = r.tick; p.vol = r.vol; p.dlt = r.dlt; } }
    if (p.ok && hi > lo) { p.pos = (double)(p.tick - lo) / (double)(hi - lo); p.closePos = (double)(close - lo) / (double)(hi - lo); p.pos = p.pos < 0 ? 0 : p.pos > 1 ? 1 : p.pos; }
    return p;
}
// the POC gate of a climax candle: bottom story (sell climax) = POC in the lower third and the close at / above it; top story = the mirror
inline bool pocGate(const Poc& p, bool bottom, long long close) { if (!p.ok) return false; return bottom ? (p.pos <= 1.0 / 3.0 + 1e-9 && close >= p.tick) : (p.pos >= 2.0 / 3.0 - 1e-9 && close <= p.tick); }

// ---- the 10-second block ----
struct Block { long long k = 0; long long vol[2] = { 0, 0 }, maxOrder[2] = { 0, 0 }; long long lo = 0, hi = 0, last = 0; bool any = false;
    std::map<long long, long long> hit[2];          // [aggressor side] price -> lots (SELL: sold into the bid there)
    std::map<long long, long long> shown[2]; };     // [resting side] price -> the size resting there just before its first hit in this block

struct Story {
    int id = 0; bool bottom = true, shown = false; int step = S_NONE, end = E_OPEN;
    long long t0 = 0, tA = 0, tE = 0, tI = 0, tEnd = 0;            // ms: the climax block's end, absorbed, exhausted, initiation, ended
    long long burst = 0, opp = 0, bigOrder = 0; double p95 = 0;     // the climax block: lots on the attacking side / the other side, its biggest order, the normal
    long long anchor = 0, zoneLo = 0, zoneHi = 0, range = 0, shownSz = 0, absLots = 0; double absR = 0;
    int blocks = 0; std::vector<long long> attBlocks; std::vector<long long> returns, fadeList; bool inZone = true; long long retCur = 0; bool fading = false;   // fadeList = the returns when fading was first seen (what the read shows)
    bool below = false; long long flushLots = 0, flushExt = 0; long long initLots = 0, initOpp = 0;
    long long extreme = 0;                                           // the story's lowest (highest) trade
    std::string sweep;                                               // the level the flush took out and reclaimed ("stop run of <level>")
    Ctx ctx; int pocGate = -1; double pocPos = -1;                  // the climax candle's POC gate: -1 = not decided yet
};
struct Mark { long long candle = 0; char letter = 'C'; int col = 0; };   // col: +1 green, -1 red; candle = the candle's END (seconds)
struct Seg { std::string text; int col = 0; };                          // the read line: col +1 green, -1 red, 0 pending (grey), 2 = white (context)

struct Engine {
    Params P; double tick = 0.25; int spb = 180;
    // the resting book (shown size)
    struct Ord { int side; long long px, sz; };
    std::unordered_map<long long, Ord> book; std::map<long long, long long> lvl[2];
    long long sideMismatch = 0, fills = 0, events = 0, garbage = 0;
    // blocks
    Block cur; std::deque<Block> hist; std::deque<long long> base; double p95 = 0;
    long long aggrId = 0, aggrLots = 0; int aggrSide = -1;
    // stories: [0] bottom (sell climax), [1] top (buy climax)
    Story st[2]; bool live[2] = { false, false }; int nextId = 1; std::vector<Story> ended;
    std::vector<std::pair<long long, std::string> > levels;         // key / volume / gamma levels and VWAP (ticks, name), from the host
    std::string clock;                                              // the Central wall clock of the newest event ("2026-11-11 20:43:20"), from the host: written on every record line for the nightly
    Ctx ctx; std::vector<std::string> out;                          // record lines (the host appends them to the session file)
    std::map<long long, std::vector<std::pair<int, std::pair<char, int> > > > pend;   // candle end -> (story id, (letter, colour)) in time order
    std::map<long long, std::vector<Mark> > marks;                  // decided letters per candle
    std::map<int, int> gate;                                        // story id -> its climax candle's POC gate (1 / 0)
    std::map<int, long long> climaxCandle;                          // story id -> the climax candle's end

    void reset() { book.clear(); lvl[0].clear(); lvl[1].clear(); for (int b = 0; b < 2; ++b) if (live[b]) finish(st[b], b, E_RESET, cur.k * 1000LL * P.blockSec); cur = Block(); aggrId = 0; aggrLots = 0; aggrSide = -1; }
    long long shownAt(int side, long long px) const { const auto it = lvl[side].find(px); return it == lvl[side].end() ? 0 : it->second; }
    void lvlAdd(int side, long long px, long long d) { if (side < 0 || side > 1) return; long long& v = lvl[side][px]; v += d; if (v <= 0) lvl[side].erase(px); }
    long long candleOf(long long tMs) const { const long long s = tMs / 1000; return (s / spb + 1) * (long long)spb; }

    // one order event; t = ms (whole seconds are fine), side = the feed's code, px in ticks
    void onEvent(long long t, int act, int side, long long px, long long sz, long long id, long long aggr) {
        ++events; if (act < 0 || act > 3 || px <= 0 || sz < 0) { ++garbage; return; }
        const long long k = t / (1000LL * P.blockSec);
        if (cur.k == 0) cur.k = k;
        if (k > cur.k) { closeBlock(); cur = Block(); cur.k = k; aggrId = 0; aggrLots = 0; aggrSide = -1; }
        if (act == NEW) { if (side < 0) return; auto it = book.find(id); if (it != book.end()) { lvlAdd(it->second.side, it->second.px, -it->second.sz); book.erase(it); }
            if (book.size() < 400000) { book[id] = Ord{ side, px, sz }; lvlAdd(side, px, sz); } }
        else if (act == MOD) { auto it = book.find(id); if (it != book.end()) { lvlAdd(it->second.side, it->second.px, -it->second.sz); it->second.px = px; it->second.sz = sz; lvlAdd(it->second.side, px, sz); }
            else if (side >= 0 && book.size() < 400000) { book[id] = Ord{ side, px, sz }; lvlAdd(side, px, sz); } }
        else if (act == DEL) { auto it = book.find(id); if (it != book.end()) { lvlAdd(it->second.side, it->second.px, -it->second.sz); book.erase(it); } }
        else {   // FILL: the resting order `id` traded `sz` with the aggressor `aggr`
            ++fills; int a = side == BID ? BUY : side == ASK ? SELL : -1;                       // the feed: BID on a fill = the aggressor bought
            auto it = book.find(id);
            if (it != book.end()) { const int byBook = it->second.side == BID ? SELL : BUY; if (a >= 0 && a != byBook) ++sideMismatch; if (a < 0) a = byBook; }
            if (a < 0 || sz <= 0) return;                                                         // an implied / unknown-side fill: not counted
            const int rest = a == SELL ? BID : ASK;
            if (!cur.shown[rest].count(px)) cur.shown[rest][px] = shownAt(rest, px);            // what rested there just before its first hit
            if (it != book.end()) { const long long d = std::min(sz, it->second.sz); lvlAdd(it->second.side, it->second.px, -d); it->second.sz -= d; if (it->second.sz <= 0) book.erase(it); }
            cur.vol[a] += sz; cur.hit[a][px] += sz; cur.last = px; if (!cur.any) { cur.lo = cur.hi = px; cur.any = true; } else { cur.lo = std::min(cur.lo, px); cur.hi = std::max(cur.hi, px); }
            if (aggr != 0 && aggr == aggrId && a == aggrSide) aggrLots += sz; else { aggrId = aggr; aggrSide = a; aggrLots = sz; }
            cur.maxOrder[a] = std::max(cur.maxOrder[a], aggrLots);
            for (int b = 0; b < 2; ++b) if (live[b]) { Story& s = st[b]; const int att = b == 0 ? SELL : BUY; if (a == att && px >= s.zoneLo && px <= s.zoneHi) s.absLots += sz; }   // lots the attackers traded into the ZONE (0.1.1: was the anchor price only)
        }
    }
    // the clock moved with no event (the host calls it once a second): closes a finished block
    void onClock(long long t) { const long long k = t / (1000LL * P.blockSec); if (cur.k != 0 && k > cur.k) { closeBlock(); cur = Block(); cur.k = k; aggrId = 0; aggrLots = 0; aggrSide = -1; } }

    static double pct95(std::deque<long long> v) { if (v.empty()) return 0; const size_t i = (size_t)std::floor(0.95 * (double)(v.size() - 1)); std::nth_element(v.begin(), v.begin() + (long)i, v.end()); return (double)v[i]; }

    void closeBlock() {
        const long long tEnd = (cur.k + 1) * 1000LL * P.blockSec;
        const bool haveBase = (int)hist.size() >= P.minBase;
        if (cur.any) for (int b = 0; b < 2; ++b) if (live[b]) advance(st[b], b, tEnd);
        for (int b = 0; b < 2; ++b) if (live[b] && tEnd - st[b].t0 > 1000LL * P.timeoutSec) finish(st[b], b, E_TIMEOUT, tEnd);
        if (cur.any && haveBase) for (int b = 0; b < 2; ++b) tryClimax(b, tEnd);
        hist.push_back(cur); while ((int)hist.size() > P.baseBlocks) hist.pop_front();
        base.push_back(cur.vol[0]); base.push_back(cur.vol[1]); while ((int)base.size() > 2 * P.baseBlocks) base.pop_front();
        p95 = pct95(base);
    }
    void tryClimax(int b, long long tEnd) {
        const bool bottom = b == 0; const int att = bottom ? SELL : BUY, def = bottom ? BUY : SELL;
        const long long v = cur.vol[att], o = cur.vol[def];
        const double thr = std::max((double)P.minLots, P.cxX * p95 * (ctx.volWin ? P.volX : 1.0));
        if ((double)v < P.logX * thr || (double)v < P.cxSide * (double)o || p95 <= 0) return;
        long long ext = bottom ? cur.lo : cur.hi; int n = 0;                                    // location: a new low (high) of the last 3 minutes
        for (auto it = hist.rbegin(); it != hist.rend() && n < P.lookBlocks; ++it, ++n) if (it->any && (bottom ? it->lo < ext : it->hi > ext)) return;
        if (live[b]) { Story& s = st[b];                                                         // a second climax while one is open: a bigger one beyond the zone restarts it
            if ((bottom ? cur.lo < s.zoneLo : cur.hi > s.zoneHi) && v >= s.burst) finish(s, b, E_DEEP, tEnd); else return; }
        Story s; s.id = nextId++; s.bottom = bottom; s.shown = (double)v >= thr; s.step = S_CLIMAX; s.t0 = tEnd; s.burst = v; s.opp = o; s.bigOrder = cur.maxOrder[att]; s.p95 = p95;
        s.range = std::max<long long>(1, cur.hi - cur.lo); s.extreme = ext; s.ctx = ctx; s.ctx.pb = (ctx.pb & (bottom ? 1 : 2)) ? 1 : 0;
        long long best = -1, bp = ext;                                                           // the anchor: the most lots hit within zoneTicks + 1 of the extreme
        for (const auto& kv : cur.hit[att]) { const long long d = bottom ? kv.first - ext : ext - kv.first; if (d >= 0 && d <= P.zoneTicks + 1 && kv.second > best) { best = kv.second; bp = kv.first; } }
        s.anchor = bp; s.zoneLo = bottom ? bp - 1 : bp - P.zoneTicks; s.zoneHi = bottom ? bp + P.zoneTicks : bp + 1;
        // (0.1.1) absorbed is measured over the ZONE, not one price: what was resting at the zone's prices just before each was first hit, and the lots
        // traded into them ("8.5x" against 11 lots at one price overstated it - ES 21:54 CT 2026-11-11)
        const int rest = bottom ? BID : ASK; s.shownSz = 0; s.absLots = 0;
        for (const auto& kv : cur.shown[rest]) if (kv.first >= s.zoneLo && kv.first <= s.zoneHi) s.shownSz += kv.second;
        for (const auto& kv : cur.hit[att]) if (kv.first >= s.zoneLo && kv.first <= s.zoneHi) s.absLots += kv.second;
        { long long bd = P.zoneTicks + 3; for (const auto& lv : levels) { const long long d = std::llabs(lv.first - bp); if (d < bd) { bd = d; s.ctx.level = lv.second; } } }   // location: the nearest level within zoneTicks + 2 of the anchor
        st[b] = s; live[b] = true; climaxCandle[s.id] = candleOf(tEnd - 1);
        note(s, "C", tEnd);   // the C letter is given only once the climax is ABSORBED (advance): selling that runs through prices is not a climax here
    }
    // one closed block against an open story (b = 0 bottom: prices mirrored for a top through sgn)
    void advance(Story& s, int b, long long tEnd) {
        if (tEnd <= s.t0) return;
        const bool bottom = b == 0; const int att = bottom ? SELL : BUY, def = bottom ? BUY : SELL;
        auto below = [&](long long px, long long ref) { return bottom ? px < ref : px > ref; };   // "beyond" in the attack's direction
        auto atOrAbove = [&](long long px, long long ref) { return bottom ? px >= ref : px <= ref; };
        const long long ext = bottom ? cur.lo : cur.hi, zFar = bottom ? s.zoneLo : s.zoneHi, zNear = bottom ? s.zoneHi : s.zoneLo;
        ++s.blocks; if (below(ext, s.extreme)) s.extreme = ext;
        const long long depth = bottom ? s.zoneLo - ext : ext - s.zoneHi;
        { const double den = std::max((double)s.shownSz, 0.25 * (double)s.burst); s.absR = den > 0 ? (double)s.absLots / den : 0; }   // (0.1.1) never against a tiny display: at least a quarter of the climax
        if (s.step == S_CLIMAX) {
            if (depth > (long long)std::max<double>(P.minDepthTicks, P.flushDepth * (double)s.range)) { finish(s, b, E_RAN, tEnd); return; }
            if (s.blocks >= P.absBlocks && s.absR >= P.absX && (double)s.absLots >= P.absShare * (double)s.burst && atOrAbove(cur.last, s.anchor))   /* absShare: the anchor took a real share of the climax (a 4-lot bid "absorbing 16x" is nothing) */ { s.step = S_ABSORBED; s.tA = tEnd; note(s, "A", tEnd);
                if (s.shown) pend[climaxCandle[s.id]].push_back(std::make_pair(s.id, std::make_pair('C', bottom ? -1 : 1)));   // C on the climax candle (red = selling climax), now that it held
                mark(s, 'A', tEnd, bottom ? 1 : -1); }
            else if (s.blocks >= P.absWait) { finish(s, b, below(cur.last, zFar) ? E_RAN : E_NOTABS, tEnd); return; }
        }
        // (0.1.1) fading while price stays PINNED at the zone: the attackers' lots of the last 3 blocks <= fadeX x those of the first 3 after the climax
        s.attBlocks.push_back(cur.vol[att]);
        if (!s.fading && s.attBlocks.size() >= 6) { const size_t n = s.attBlocks.size(); const long long first = s.attBlocks[0] + s.attBlocks[1] + s.attBlocks[2], last = s.attBlocks[n - 1] + s.attBlocks[n - 2] + s.attBlocks[n - 3];
            if (first > 0 && (double)last <= P.fadeX * (double)first) { s.fading = true; s.fadeList.clear(); s.fadeList.push_back(first); s.fadeList.push_back(last); } }
        // returns to the zone (fading): lots the attackers traded while price was in / beyond the zone, per visit
        const bool touch = !below(s.anchor, ext);                                                // the block traded at / beyond the ANCHOR price
        if (touch) { s.retCur += cur.vol[att]; s.inZone = true; }
        else if (s.inZone) { if (s.blocks > 1 && s.retCur > 0) s.returns.push_back(s.retCur); s.retCur = 0; s.inZone = false;
            const size_t n = s.returns.size(); const bool f = n >= 2 && s.returns[n - 1] <= s.returns[n - 2] && (double)s.returns[n - 1] <= P.fadeX * (double)s.burst;
            if (f && !s.fading) { s.fading = true; s.fadeList.assign(s.returns.begin() + (long)(n > 3 ? n - 3 : 0), s.returns.end()); } }
        if (s.step >= S_ABSORBED && s.step < S_INIT) {
            if (depth > 0) { if (!s.below) { s.below = true; s.flushLots = 0; s.flushExt = ext; } s.flushLots += cur.vol[att]; if (below(ext, s.flushExt)) s.flushExt = ext;
                if (depth > (long long)std::max<double>(P.minDepthTicks, P.flushDepth * (double)s.range) || (double)s.flushLots > 1.5 * (double)s.burst) { finish(s, b, E_DEEP, tEnd); return; } }
            else if (s.below && atOrAbove(cur.last, s.anchor)) { s.below = false;
                if (s.step == S_ABSORBED && (double)s.flushLots <= P.flushX * (double)s.burst) { s.step = S_EXHAUSTED; s.tE = tEnd;
                    for (const auto& lv : levels) if (bottom ? (lv.first >= s.flushExt && lv.first < s.zoneLo) : (lv.first <= s.flushExt && lv.first > s.zoneHi)) { s.sweep = lv.second; break; }   // the flush took out a level and came back: a stop run
                    note(s, "E", tEnd); mark(s, 'E', tEnd, bottom ? 1 : -1); } }
            const long long v = cur.vol[def], o = cur.vol[att];
            if (!s.below && (double)v >= P.initX * s.p95 && (double)v >= P.initSide * (double)o && below(zNear, cur.last)) {   // ends beyond the zone's near edge, away from the attack
                s.initLots = v; s.initOpp = o; s.step = S_INIT; s.tI = tEnd; note(s, "I", tEnd); mark(s, 'I', tEnd, bottom ? 1 : -1); finish(s, b, E_COMPLETE, tEnd); return; }
        }
    }
    void mark(const Story& s, char letter, long long tEnd, int col) { if (s.shown) pend[candleOf(tEnd - 1)].push_back(std::make_pair(s.id, std::make_pair(letter, col))); }
    void finish(Story& s, int b, int why, long long tEnd) {
        s.end = why; s.tEnd = tEnd; if (s.retCur > 0 && s.blocks > 1) s.returns.push_back(s.retCur);
        note(s, "END", tEnd); ended.push_back(s); while (ended.size() > 200) ended.erase(ended.begin()); live[b] = false;
    }
    // ---- records: one line per step, everything measured ("S|ver|id|side|shown|step|t|...") ----
    static std::string num(double v) { char b[32]; std::snprintf(b, sizeof b, "%.10g", v); return b; }
    void note(const Story& s, const char* step, long long t) {
        std::string l = std::string("S|") + VERSION + "|" + std::to_string(s.id) + "|" + (s.bottom ? "bottom" : "top") + "|" + (s.shown ? "1" : "0") + "|" + step + "|" + std::to_string(t / 1000)
            + "|burst " + std::to_string(s.burst) + "|opp " + std::to_string(s.opp) + "|big " + std::to_string(s.bigOrder) + "|p95 " + num(s.p95) + "|anchor " + num((double)s.anchor * tick)
            + "|shown " + std::to_string(s.shownSz) + "|abs " + std::to_string(s.absLots) + "|absx " + num(s.absR) + "|range " + std::to_string(s.range) + "|ext " + num((double)s.extreme * tick) + "|ret ";
        for (size_t i = 0; i < s.returns.size(); ++i) l += (i ? "/" : "") + std::to_string(s.returns[i]);
        l += "|flush " + std::to_string(s.flushLots) + "|init " + std::to_string(s.initLots) + "|initopp " + std::to_string(s.initOpp) + "|gate " + std::to_string(s.pocGate) + "|pocpos " + num(s.pocPos)
            + "|sess " + s.ctx.session + "|volwin " + std::to_string(s.ctx.volWin) + "|hourly " + s.ctx.hourly + "|level " + s.ctx.level + "|pb " + std::to_string(s.ctx.pb) + "|sweep " + s.sweep + "|ct " + clock + "|end " + endName(s.end);
        out.push_back(l);
    }
    // ---- a candle closed: its letters are decided, once (candle = its END in seconds; poc = that candle's POC; close in ticks) ----
    void onCandle(long long candle, const Poc& poc, long long close) {
        for (auto& kv : climaxCandle) if (kv.second == candle && !gate.count(kv.first)) {       // the POC gate of every story whose climax was in this candle
            bool bottom = true, found = false; for (int b = 0; b < 2; ++b) if (live[b] && st[b].id == kv.first) { bottom = st[b].bottom; found = true; st[b].pocGate = pocGate(poc, bottom, close) ? 1 : 0; st[b].pocPos = poc.pos; }
            if (!found) for (Story& e : ended) if (e.id == kv.first) { bottom = e.bottom; found = true; e.pocGate = pocGate(poc, bottom, close) ? 1 : 0; e.pocPos = poc.pos; }
            if (found) { gate[kv.first] = pocGate(poc, bottom, close) ? 1 : 0; out.push_back("G|" + std::to_string(kv.first) + "|" + std::to_string(candle) + "|" + std::to_string(gate[kv.first]) + "|" + num(poc.pos)); } }
        for (auto it = pend.begin(); it != pend.end() && it->first <= candle; it = pend.erase(it)) {
            std::vector<Mark> v;
            for (const auto& pr : it->second) { const auto g = gate.find(pr.first); if (g == gate.end() || g->second != 1) continue;   // a story whose climax candle failed the POC gate shows no letter
                Mark m; m.candle = it->first; m.letter = pr.second.first; m.col = pr.second.second; v.push_back(m); }
            if (v.size() > 2) { std::vector<Mark> w; w.push_back(v.front()); w.push_back(v.back()); v.swap(w); }                            // at most 2: the first and the last step of the candle
            if (!v.empty()) { std::vector<Mark>& d = marks[it->first];
                if (!d.empty() && d.size() < 2 && it->first < candle) { for (const Mark& m : v) if (d.size() < 2) { d.insert(d.begin(), m); out.push_back(std::string("L|") + std::to_string(it->first) + "|" + m.letter + (m.col > 0 ? "+" : "-")); } }   // a C that arrives after its candle closed (absorbed one candle later) joins it
                else if (d.empty()) { d = v; std::string l = "L|" + std::to_string(it->first) + "|"; for (const Mark& m : v) { l += m.letter; l += m.col > 0 ? "+" : "-"; } out.push_back(l); } }
        }
        while (marks.size() > 4000) marks.erase(marks.begin());
        while (gate.size() > 2000) { climaxCandle.erase(gate.begin()->first); gate.erase(gate.begin()); }
    }
    // a restart: the letters decided before stay (the session file's "L|candle|C-A+" lines)
    int loadMarks(const std::vector<std::string>& lines) { int n = 0;
        for (const std::string& l : lines) { if (l.size() < 5 || l[0] != 'L' || l[1] != '|') continue; const size_t b = l.find('|', 2); if (b == std::string::npos) continue;
            char* e = nullptr; const long long c = std::strtoll(l.c_str() + 2, &e, 10); if (!e || *e != '|' || c <= 0) continue; std::vector<Mark> v;
            for (size_t i = b + 1; i + 1 < l.size() && v.size() < 2; i += 2) {
                const char ch = l[i], sg = l[i + 1]; if ((ch != 'C' && ch != 'A' && ch != 'E' && ch != 'I') || (sg != '+' && sg != '-')) break; Mark m; m.candle = c; m.letter = ch; m.col = sg == '+' ? 1 : -1; v.push_back(m); }
            if (!v.empty() && !marks.count(c)) { marks[c] = v; ++n; } }
        return n; }

    // ---- the read line of the newest story (live first, else the last ended within keepSec) ----
    const Story* newest(long long nowMs, int keepSec = 1800) const {
        const Story* s = nullptr; for (int b = 0; b < 2; ++b) if (live[b] && st[b].shown && (!s || st[b].t0 > s->t0)) s = &st[b];
        if (s) return s;
        for (auto it = ended.rbegin(); it != ended.rend(); ++it) if (it->shown && it->tA > 0 && nowMs - it->tEnd <= 1000LL * keepSec) return &*it;
        return nullptr; }
    static std::string lots(long long v) { return std::to_string(v); }
    std::vector<Seg> readLine(const Story& s) const {
        std::vector<Seg> L; const bool bt = s.bottom; const int a = bt ? -1 : 1, d = bt ? 1 : -1;   // attackers' colour / defenders' colour
        auto add = [&](const std::string& t, int c) { Seg g; g.text = t; g.col = c; L.push_back(g); };
        add(std::string(bt ? "Sell climax " : "Buy climax ") + lots(s.burst) + (s.bigOrder >= 10 ? " (" + lots(s.bigOrder) + "-lot)" : std::string()), a);
        char b[64];
        if (s.step >= S_ABSORBED) { std::snprintf(b, sizeof b, "Absorbed %.1fx at %.10g", s.absR, (double)s.anchor * tick); add(b, d); }
        else if (s.end == E_OPEN) add("absorbed?", 0); else { add(endName(s.end), 0); return L; }
        if (s.fading) { std::string t = "Fading "; for (size_t i = 0; i < s.fadeList.size(); ++i) t += (i ? " / " : "") + lots(s.fadeList[i]); add(t, d); }
        else if (s.end == E_OPEN && s.step < S_INIT) add("fading?", 0);
        if (s.tE > 0) add((s.sweep.empty() ? std::string(bt ? "Flush on " : "Push on ") : "Stop run of " + s.sweep + " on ") + lots(s.flushLots), d);
        else if (s.end == E_OPEN && s.step < S_INIT) add(bt ? "flush?" : "push?", 0);
        if (s.step >= S_INIT) add(std::string(bt ? "Buyers " : "Sellers ") + lots(s.initLots), d);
        else if (s.end == E_OPEN) add(bt ? "buyers?" : "sellers?", 0);
        else add(endName(s.end), 0);
        std::string c = s.ctx.hourly;
        auto join = [&](const std::string& t) { c += (c.empty() ? "" : " | ") + t; };
        if (!s.ctx.level.empty()) join("at " + s.ctx.level);
        if (s.ctx.pb) join("Pb");
        if (s.ctx.volWin) join(volName(s.ctx.volWin));
        if (s.pocGate == 0) join("POC not at the extreme");
        if (!c.empty()) add(c, 2);
        return L;
    }
};

// ---- the pane layout (pure): one bar per visible candle = its POC lots, the number on top, the letters above ----
struct PBar { int x = 0, top = 0, bottom = 0, w = 0, col = 0; std::string num; int numY = 0; std::vector<Mark> marks; int letY = 0; };
// candles: (x centre, Poc); paneTop / paneBottom in pixels; textH = the number's line height; bw = pixels per candle
inline std::vector<PBar> layoutBars(const std::vector<std::pair<int, Poc> >& c, const std::map<long long, std::vector<Mark> >& marks, int paneTop, int paneBottom, int textH, double bw, int headH) {
    std::vector<PBar> out; long long mx = 0; for (const auto& p : c) if (p.second.ok) mx = std::max(mx, p.second.vol);
    const int top = paneTop + headH + 2 * textH + 4, bot = paneBottom - 2; if (mx <= 0 || bot - top < 6) return out;
    const int w = std::max(1, (int)std::lround(bw * 0.6)); const bool nums = bw >= 16;         // numbers only when the candles are wide enough to read them
    for (const auto& p : c) { if (!p.second.ok) continue; PBar b; b.x = p.first; b.w = w; b.bottom = bot; b.top = bot - std::max(1, (int)std::lround((double)(bot - top) * (double)p.second.vol / (double)mx));
        b.col = p.second.dlt > 0 ? 1 : p.second.dlt < 0 ? -1 : 0; if (nums) b.num = std::to_string(p.second.vol); b.numY = b.top - textH - 1; b.letY = b.numY - textH;
        const auto m = marks.find(p.second.t); if (m != marks.end()) b.marks = m->second; out.push_back(b); }
    return out;
}
} // namespace rvs
#endif
