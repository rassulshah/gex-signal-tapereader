#ifndef LS_FMIRROR_H
#define LS_FMIRROR_H
// FMirror.h 1.0.0 (2026-10-26) -- SDK-free rule of F? (MIRROR FAIL: the pivot likely dies at once) for lsBookFlow 1.28.0.
// BUILD-SPEC 2026-10-25 (Rassul approved 22:29 CT): "F? (amber): the pivot dies at once. Decided at the pivot-bar close. PROVEN read;
// it means skip / wait." The research rule (o4reads f_mirror_skip, registry "PROVEN read"; 46% die at once vs 13%):
//   pivot candidate  a 5-candle low / high whose close is in the far half of the bar (a low closing in its upper half; a high mirrored)
//   T                the pivot-bar close (its bar key, IRT bar END time, in seconds)
//   band             0.5 ATR from the close (ATR = the mean range of the 14 candles up to the pivot bar)
//   front            the ATTACKERS stacking in front: opposing-side adds - pulls in [close, close + band] (a low: offers above the close)
//   behind           the DEFENDERS leaving behind: defending-side pulls - adds in [close - band, close] (a low: bids below the close)
//                    both over [T - 60 s, T), in floor units (lots / the market's large-order floor), fills never count as pulls
//   F? (armed)       front >= F_FRONT and behind >= F_BEHIND - the market's top third of prior sessions, from the nightly params file
//                    lsFlexLevels\LRA-FMirror-<MKT>.csv ("PARAM,F_FRONT,x" / "PARAM,F_BEHIND,y" / "PARAM,SHOW,1"). No thresholds in the
//                    file = every read is still computed and recorded, never drawn (armed = -1, "thresholds not set").
//   Decided once at the pivot-bar close, written as an F line, read back after a restart (never repainted).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <string>
#include <vector>

namespace fmr {

static const char* const VERSION = "1.0.0";
enum { BID = 0, ASK = 1 };                    // off::BID / off::ASK
enum { LOW = 0, HIGH = 1 };                   // the pivot side (dt::LOW / dt::HIGH)
enum { K_ADD = 0, K_PULL = 1, K_FILL = 2 };   // the engine's BookSink kinds
static const long long WINDOW_SEC = 60;
static const int EXT_BARS = 5, ATR_BARS = 14;

inline int floorOf(const std::string& m) {     // the fixed large-order floor (lots) of the research (o4reads core.FLOOR)
    if (m == "ES") return 10;
    if (m == "EU" || m == "6E") return 9;
    if (m == "NQ" || m == "CL") return 5;
    if (m == "GC" || m == "HG" || m == "NG") return 3;
    return 10;
}

struct Params { double front = -1, behind = -1; bool show = true; std::string why = "no file"; bool set() const { return front > 0 && behind > 0; } };
inline bool parseParam(const std::string& line0, Params& p) {
    std::string line = line0; while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) line.pop_back();
    std::vector<std::string> f; size_t a = 0; for (;;) { const size_t b = line.find(',', a); f.push_back(line.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
    if (f.size() == 3 && f[0] == "PARAM") f.erase(f.begin());
    if (f.size() != 2 || f[1].empty()) return false;
    char* e = nullptr; const double v = std::strtod(f[1].c_str(), &e); if (!e || *e || !std::isfinite(v)) return false;
    if (f[0] == "F_FRONT" && v > 0 && v < 1e6) { p.front = v; return true; }
    if (f[0] == "F_BEHIND" && v > 0 && v < 1e6) { p.behind = v; return true; }
    if (f[0] == "SHOW" && (v == 0 || v == 1)) { p.show = v == 1; return true; }
    return false;
}

// the classified book events of the last ~2 minutes (adds / true pulls; fills ignored)
struct Ev { long long ts = 0; int kind = 0, side = 0; long long px = 0, size = 0; };
struct Acc {
    std::deque<Ev> q; long long last = 0;
    void on(int kind, int side, long long px, long long size, long long ts) {
        if ((kind != K_ADD && kind != K_PULL) || (side != BID && side != ASK) || size <= 0 || ts <= 0) return;
        Ev e; e.ts = ts; e.kind = kind; e.side = side; e.px = px; e.size = size; q.push_back(e); if (ts > last) last = ts;
        while (!q.empty() && q.front().ts < last - 2 * WINDOW_SEC) q.pop_front();
        while (q.size() > 400000) q.pop_front();
    }
    // adds - pulls (sign +1) or pulls - adds (sign -1) of one side in [lo, hi] over [T - 60, T)
    long long net(int side, long long lo, long long hi, long long T, int sign) const {
        long long s = 0;
        for (const Ev& e : q) { if (e.side != side || e.ts < T - WINDOW_SEC || e.ts >= T || e.px < lo || e.px > hi) continue; s += (e.kind == K_ADD ? 1 : -1) * sign * e.size; }
        return s;
    }
};

struct Bar { long long key = 0; bool ok = false; long long o = 0, h = 0, l = 0, c = 0; };
// the pivot side of bar i (LOW / HIGH / -1): a new 5-candle extreme with the close in the far half (a low pivot that closes in its upper half)
inline int pivotOf(const std::vector<Bar>& b, size_t i) {
    if (i < (size_t)EXT_BARS || !b[i].ok) return -1;
    long long mn = b[i - 1].l, mx = b[i - 1].h; for (size_t j = i - EXT_BARS; j < i; ++j) { if (!b[j].ok) return -1; mn = std::min(mn, b[j].l); mx = std::max(mx, b[j].h); }
    const Bar& x = b[i]; const long long r = x.h - x.l; if (r <= 0) return -1;
    const bool low = x.l < mn && 2 * (x.c - x.l) >= r, high = x.h > mx && 2 * (x.h - x.c) >= r;
    if (low && !high) return LOW;
    if (high && !low) return HIGH;
    return -1;
}
inline double atrAt(const std::vector<Bar>& b, size_t i) { double s = 0; int n = 0; for (size_t j = i + 1; j-- > 0 && n < ATR_BARS;) { if (!b[j].ok) continue; s += (double)(b[j].h - b[j].l); ++n; } return n >= 5 ? s / n : 0.0; }

struct Read { long long key = 0; int side = -1; double front = 0, behind = 0; int armed = -1; long long ext = 0; };   // armed 1 = F?, 0 = no, -1 = thresholds not set
// the read at the close of bar i (call once, when bar i has closed); false = not a pivot candidate / no ATR
inline bool decide(const std::vector<Bar>& b, size_t i, const Acc& A, const std::string& market, const Params& P, Read& out) {
    const int side = pivotOf(b, i); if (side < 0) return false;
    const double atr = atrAt(b, i); if (!(atr > 0)) return false;
    const long long band = std::max<long long>(1, (long long)std::llround(0.5 * atr)), c = b[i].c, T = b[i].key;
    const double fl = (double)floorOf(market);
    long long front = 0, behind = 0;
    if (side == LOW) { front = A.net(ASK, c, c + band, T, +1); behind = A.net(BID, c - band, c, T, -1); }
    else { front = A.net(BID, c - band, c, T, +1); behind = A.net(ASK, c, c + band, T, -1); }
    Read r; r.key = T; r.side = side; r.front = (double)front / fl; r.behind = (double)behind / fl; r.ext = side == LOW ? b[i].l : b[i].h;
    r.armed = P.set() ? ((r.front >= P.front && r.behind >= P.behind) ? 1 : 0) : -1;
    out = r; return true;
}
inline std::string lineOf(const Read& r) { char b[160]; std::snprintf(b, sizeof b, "F|%lld|%d|%lld|%lld|%d|%lld", r.key, r.side, (long long)std::llround(r.front * 100), (long long)std::llround(r.behind * 100), r.armed, r.ext); return b; }
inline bool parse(const std::string& s0, Read& r) {
    std::string s = s0; while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) s.pop_back();
    if (s.size() < 3 || s[0] != 'F' || s[1] != '|') return false;
    std::vector<std::string> f; size_t a = 2; for (;;) { const size_t q = s.find('|', a); f.push_back(s.substr(a, q == std::string::npos ? std::string::npos : q - a)); if (q == std::string::npos) break; a = q + 1; }
    if (f.size() != 6) return false;
    long long v[6]; for (int k = 0; k < 6; ++k) { if (f[(size_t)k].empty()) return false; char* e = nullptr; v[k] = std::strtoll(f[(size_t)k].c_str(), &e, 10); if (!e || *e) return false; }
    if (v[0] <= 0 || (v[1] != LOW && v[1] != HIGH) || v[4] < -1 || v[4] > 1) return false;
    r.key = v[0]; r.side = (int)v[1]; r.front = v[2] / 100.0; r.behind = v[3] / 100.0; r.armed = (int)v[4]; r.ext = v[5]; return true;
}
// the hover text (his format)
inline std::string hover(const Read& r, const Params& P) {
    char b[900]; const bool low = r.side == LOW;
    std::snprintf(b, sizeof b, "Question: is this %s about to fail at once?\nAnswer: %s stacked %.1f floors in front (%s), %s pulled %.1f floors behind (%s) in the last 60 s before the close.\nHow to use: F? = do not take the reversal entry at the close; expect a retest or a break within minutes (skip / wait, never a trade the other way).\nExample: ES 10-22 1:12 PM high: buyers stacked 1.6x, sellers added only 0.6x; the high broke 0.04 s after the close.",
        low ? "low" : "high", low ? "sellers" : "buyers", r.front, P.set() ? (std::to_string(P.front).substr(0, 4) + "+").c_str() : "threshold not set", low ? "buyers" : "sellers", r.behind, P.set() ? (std::to_string(P.behind).substr(0, 4) + "+").c_str() : "threshold not set");
    return b;
}

} // namespace fmr
#endif
