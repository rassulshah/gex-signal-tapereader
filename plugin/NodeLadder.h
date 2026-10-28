#ifndef LS_NODELADDER_H
#define LS_NODELADDER_H
// NodeLadder.h 1.0.0 (2026-10-28 overnight, Rassul 21:52 CT "go" - the per-node order-book measure; research round 2:
// research1028/ + https://claude.ai/artifact/5C8fwBzEAiwVj3wfewmDPz "What the Delta Profile needs to read better") -- SDK-free.
// OrderFlowFeed's engine (off::Engine) feeds every CLASSIFIED book event near price into this ladder: per candle (the engine's candle key),
// per price (ticks), per resting side: lots ADDED, PULLED (a cancel / size cut / move away that was not a fill), FILLED, and FILLED IN THE
// CANDLE'S FIRST HALF (the attack pace). One owner per fact: the engine computes (lsBookFlow owns the feed that posts it), lsDeltaProfile
// reads the posted ladder (SignalBoard 1.5.0 channel LRA_SLD_) and sums its node's zone itself (nodeRead below - the same code in both the
// plugin and the tests). Bounded: at most MAX_BARS candles, MAX_ROWS prices per candle (further new prices of that candle are counted as
// dropped, never mis-assigned).
//
// The node read (research definitions, scratchpad lmtf2/mbo_in.py + feat.py, zone z = max(1, round(0.15 x ATR in ticks))):
//   bullish node (sellers attacked, BIDS defend), node rows lo..hi in ticks:
//     defenders added   = bids added in [lo - 3z, hi + z]       (at and behind the node)
//     defenders pulled  = bids pulled in [lo - 3z, hi + 3z]
//     defenders filled  = bids filled in [lo - z, hi + z]       (first half of the node's candle -> the pace)
//     attackers pulled  = offers pulled in [hi + 1, hi + ATR]   (the attackers' own resting orders above)
//   bearish node: the mirror (offers defend; adds in [lo - z, hi + 3z]; attackers = bids in [lo - ATR, lo - 1])
//   net x   = (added - pulled) / the market's median defenders-added for that hour of day (LRA-ReadTier-<MKT>.csv HOURMED)
//   leave   = pulled / added        pace fade = 2nd-half fills / 1st-half fills
//   HOLDING = net x >= NET_HI or leave <= LEAVE_LO (per-market thresholds, the 2/3 and 1/3 quantiles of the build days)
#include <map>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
namespace nl2 {
static const char* const VERSION = "1.0.0";
static const size_t MAX_BARS = 6, MAX_ROWS = 600;
enum { BIDS = 0, OFFERS = 1 };
struct Row { long long add[2] = {0, 0}, pull[2] = {0, 0}, fill[2] = {0, 0}, fill1[2] = {0, 0}; };
struct Bar { long long key = 0, half = 0; std::map<long long, Row> rows; long long dropped = 0; };
// the engine side: kind 0 = added near price, 1 = pulled near price, 2 = filled (the RESTING side), ts = the event second, key = its candle
struct Ladder {
    long long spb = 180;                   // seconds per bar (the owner sets it); a fill before key - spb / 2 is in the first half
    std::map<long long, Bar> bars; long long events = 0, droppedRows = 0, droppedOld = 0;
    void on(int kind, int side, long long px, long long size, long long ts, long long key) {
        if (key <= 0 || size <= 0 || px <= 0 || (side != BIDS && side != OFFERS) || kind < 0 || kind > 2) return;
        std::map<long long, Bar>::iterator b = bars.find(key);
        if (b == bars.end()) {
            if (!bars.empty() && key < bars.begin()->first && bars.size() >= MAX_BARS) { ++droppedOld; return; }
            b = bars.insert(std::make_pair(key, Bar())).first; b->second.key = key; b->second.half = key - spb / 2;
            while (bars.size() > MAX_BARS) bars.erase(bars.begin());
            b = bars.find(key); if (b == bars.end()) { ++droppedOld; return; }
        }
        std::map<long long, Row>::iterator r = b->second.rows.find(px);
        if (r == b->second.rows.end()) {
            if (b->second.rows.size() >= MAX_ROWS) { ++b->second.dropped; ++droppedRows; return; }
            r = b->second.rows.insert(std::make_pair(px, Row())).first;
        }
        ++events;
        if (kind == 0) r->second.add[side] += size;
        else if (kind == 1) r->second.pull[side] += size;
        else { r->second.fill[side] += size; if (ts > 0 && ts < b->second.half) r->second.fill1[side] += size; }
    }
};
// ---- the posted form (one string; SignalBoard 1.5.0 puts it on the chart's board) ---------------------------------------------------
// "<stamp ms>|<bar ct sec>,<key>,<tick>,<p0>,<n>:<off>,a0,a1,p0,p1,f0,f1,h0,h1/<off>,...;<next bar>..."  (only rows with any number)
struct PBar { long long bar = 0, key = 0, p0 = 0; double tick = 0; std::map<long long, Row> rows; };
inline std::string encode(long long ms, const std::vector<PBar>& v) {
    std::string o = std::to_string(ms) + "|";
    for (size_t i = 0; i < v.size(); ++i) {
        char h[128]; std::snprintf(h, sizeof(h), "%lld,%lld,%.10g,%lld,%zu:", v[i].bar, v[i].key, v[i].tick, v[i].p0, v[i].rows.size()); o += h;
        bool first = true;
        for (std::map<long long, Row>::const_iterator r = v[i].rows.begin(); r != v[i].rows.end(); ++r) {
            const Row& w = r->second; char b[200];
            std::snprintf(b, sizeof(b), "%s%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld", first ? "" : "/", r->first - v[i].p0, w.add[0], w.add[1], w.pull[0], w.pull[1], w.fill[0], w.fill[1], w.fill1[0], w.fill1[1]);
            o += b; first = false;
        }
        o += ";";
    }
    return o;
}
inline bool decode(const std::string& s, long long nowMs, long long freshMs, std::vector<PBar>& out) {
    out.clear(); if (s.empty() || s.size() > 30000) return false;
    const size_t bar = s.find('|'); if (bar == std::string::npos) return false;
    const std::string head = s.substr(0, bar); char* e = nullptr; const long long ms = std::strtoll(head.c_str(), &e, 10); if (head.empty() || !e || *e) return false;
    if (nowMs >= 0 && (nowMs < ms || nowMs - ms > freshMs)) return false;
    size_t a = bar + 1;
    while (a < s.size() && out.size() < MAX_BARS) {
        const size_t end = s.find(';', a); if (end == std::string::npos) break;
        const std::string part = s.substr(a, end - a); a = end + 1;
        const size_t c = part.find(':'); if (c == std::string::npos) continue;
        PBar p; size_t n = 0;
        if (std::sscanf(part.substr(0, c).c_str(), "%lld,%lld,%lf,%lld,%zu", &p.bar, &p.key, &p.tick, &p.p0, &n) != 5 || !(p.tick > 0)) continue;
        size_t q = c + 1; bool ok = true;
        while (q < part.size()) {
            size_t z = part.find('/', q); if (z == std::string::npos) z = part.size();
            long long off = 0; Row w;
            if (std::sscanf(part.substr(q, z - q).c_str(), "%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld", &off, &w.add[0], &w.add[1], &w.pull[0], &w.pull[1], &w.fill[0], &w.fill[1], &w.fill1[0], &w.fill1[1]) != 9) { ok = false; break; }
            p.rows[p.p0 + off] = w; q = z + 1;
        }
        if (ok && p.rows.size() == n) out.push_back(p);
    }
    return true;
}
// the newest `count` closed candles of a ladder (keys < formingKey), mapped to their bar stamps by the caller's function
template <class BarOf> std::vector<PBar> closedBars(const Ladder& L, long long formingKey, double tick, size_t count, BarOf barOf) {
    std::vector<PBar> out;
    for (std::map<long long, Bar>::const_reverse_iterator b = L.bars.rbegin(); b != L.bars.rend() && out.size() < count; ++b) {
        if (formingKey > 0 && b->first >= formingKey) continue;
        PBar p; p.key = b->first; p.bar = barOf(b->first); p.tick = tick; p.rows = b->second.rows;
        p.p0 = p.rows.empty() ? 0 : p.rows.begin()->first; if (p.bar > 0) out.push_back(p);
    }
    std::reverse(out.begin(), out.end());
    return out;
}
// ---- the per-market tier params (LRA-ReadTier-<MKT>.csv, written by the nightly part read_tier; research round 2 seeds) ----------
struct Tier {
    bool ok = false; std::string why = "no LRA-ReadTier file";
    double zAtr = 0.15, netHi = 0, leaveLo = 1; double hourMed[24] = {0}; bool hourOk[24] = {false};
    bool plusProven = false, nextShown = true;   // A+ proven (else "A+?"), A? drawn
    int lateFrom = 60;                           // minutes before the RTH close that count as late (+ the post-close evening)
};
inline Tier parseTier(const std::string& text, const std::string& market) {
    Tier t; t.why = "bad LRA-ReadTier file"; bool ver = false, mk = false, th = false; int hours = 0;
    size_t a = 0;
    while (a <= text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        std::string line = text.substr(a, e - a); a = e + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::vector<std::string> f; size_t b = 0; while (b <= line.size()) { size_t c = line.find(',', b); if (c == std::string::npos) c = line.size(); f.push_back(line.substr(b, c - b)); b = c + 1; }
        if (f.empty() || f[0].empty()) { if (e >= text.size()) break; continue; }
        char* z = nullptr;
        if (f[0] == "VERSION") ver = f.size() > 1 && f[1] == "1";
        else if (f[0] == "MARKET") mk = f.size() > 1 && f[1] == market;
        else if (f[0] == "Z_ATR" && f.size() > 1) { const double v = std::strtod(f[1].c_str(), &z); if (z && !*z && v > 0 && v < 2) t.zAtr = v; }
        else if (f[0] == "HOLD" && f.size() > 2) { const double n = std::strtod(f[1].c_str(), &z); bool ok1 = z && !*z; const double l = std::strtod(f[2].c_str(), &z); if (ok1 && z && !*z && std::isfinite(n) && std::isfinite(l)) { t.netHi = n; t.leaveLo = l; th = true; } }
        else if (f[0] == "HOURMED" && f.size() > 2) { const long h = std::strtol(f[1].c_str(), &z, 10); bool ok1 = z && !*z; const double v = std::strtod(f[2].c_str(), &z);
            if (ok1 && z && !*z && h >= 0 && h < 24 && v > 0) { t.hourMed[h] = v; if (!t.hourOk[h]) ++hours; t.hourOk[h] = true; } }
        else if (f[0] == "LATE" && f.size() > 1) { const long v = std::strtol(f[1].c_str(), &z, 10); if (z && !*z && v >= 0 && v <= 240) t.lateFrom = (int)v; }
        else if (f[0] == "SHOW" && f.size() > 2) { if (f[1] == "A+") t.plusProven = f[2] == "proven"; else if (f[1] == "A?") t.nextShown = f[2] != "off"; }
        if (e >= text.size()) break;
    }
    if (!ver || !mk || !th || hours < 12) { t.ok = false; return t; }
    t.ok = true; t.why = "nightly read tier"; return t;
}
// the hour median for an hour with none: the nearest hour that has one
inline double hourMedOf(const Tier& t, int hour) {
    if (hour < 0 || hour > 23) return 0;
    for (int d = 0; d < 12; ++d) { const int a = (hour + d) % 24, b = (hour - d + 24) % 24; if (t.hourOk[a]) return t.hourMed[a]; if (t.hourOk[b]) return t.hourMed[b]; }
    return 0;
}
struct Read { bool ok = false; long long add = 0, pull = 0, fill = 0, fill1 = 0, attPull = 0; double netX = 0, leave = 0, fade = -1; bool holding = false; };
// the node read from one candle's ladder rows; bull = the node is bullish (sellers attacked, bids defend); lo / hi node rows in ticks;
// atrTicks = the 20-candle ATR before the candle in ticks; hour = the candle's START hour (Central)
inline Read nodeRead(const std::map<long long, Row>& rows, bool bull, long long lo, long long hi, double atrTicks, int hour, const Tier& t) {
    Read r; if (!(atrTicks > 0) || hi < lo) return r;
    const long long z = std::max(1LL, (long long)std::llround(t.zAtr * atrTicks)), A = std::max(1LL, (long long)atrTicks);
    const int d = bull ? BIDS : OFFERS, att = 1 - d;
    const long long addLo = bull ? lo - 3 * z : lo - z, addHi = bull ? hi + z : hi + 3 * z;
    const long long aLo = bull ? hi + 1 : lo - A, aHi = bull ? hi + A : lo - 1;
    for (std::map<long long, Row>::const_iterator it = rows.begin(); it != rows.end(); ++it) {
        const long long p = it->first; const Row& w = it->second;
        if (p >= addLo && p <= addHi) r.add += w.add[d];
        if (p >= lo - 3 * z && p <= hi + 3 * z) r.pull += w.pull[d];
        if (p >= lo - z && p <= hi + z) { r.fill += w.fill[d]; r.fill1 += w.fill1[d]; }
        if (p >= aLo && p <= aHi) r.attPull += w.pull[att];
    }
    const double med = hourMedOf(t, hour);
    if (!(med > 0)) return r;
    r.netX = (double)(r.add - r.pull) / med;
    r.leave = r.add > 0 ? (double)r.pull / (double)r.add : (r.pull > 0 ? 99.0 : 0.0);
    r.fade = r.fill1 > 0 ? (double)(r.fill - r.fill1) / (double)r.fill1 : -1;
    r.holding = r.netX >= t.netHi || (r.add > 0 && r.leave <= t.leaveLo);
    r.ok = true; return r;
}
// "Hold +1.8x" / "Pull -1.3x" (the ratio with its sign, one decimal)
inline std::string ratioText(double x) { char b[32]; std::snprintf(b, sizeof(b), "%s%.1fx", x >= 0 ? "+" : "-", std::fabs(x)); return b; }
}  // namespace nl2
#endif
