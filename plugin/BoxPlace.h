// BoxPlace.h - shared, SDK-free placement rules for every on-chart box (1.0.0, 2026-10-14, CR4800).
//
// Rassul 2026-10-14 07:07 "Add Offset X and Offset Y settings ... so he can nudge a box off whatever it overlaps"; 07:18 "give these
// options in the indicators, but please test them thoroughly to ensure they work and dont shake or act erratic, and dont have
// garbage values". Used by lsTapeReader 1.1.3, lsHodLod 2.0.6, lsDealerRead 4.5.9, lsBidAskStats 1.5.2, lsStackStats 1.1.2,
// lsPullStats 1.2.1 and lsLiquidityProfile 0.15.3.
//
//   Place      the 9-spot Position (0 Top left .. 8 Bottom right) + Offset X / Y in pixels (right / down = +), kept per market in
//              the indicator's place file as "pos,dx,dy[,extra...]". Offsets are whole numbers in [-600, 600]; anything else
//              (garbage, NaN, out of range, unset) = the default, and the reason is reported once (fallback()).
//   clampIn    the box is kept FULLY inside the area (the pane, up to the last bar): an offset can never push it off-screen or right
//              of the last bar.
//   Hyst       overlap avoidance with hysteresis: a new spot is taken only after the same candidate has won 2 draws in a row, and the
//              spot just left is not taken back for HOLD draws unless the current one is blocked (no ping-pong).
//   StableW    a width that never shakes: grows at once to fit, shrinks only after the narrower width held for SHRINK_MS.
#ifndef BOXPLACE_H
#define BOXPLACE_H
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>

namespace bp {

static const char* const VERSION = "1.0.0";
static const int OFF_MIN = -600, OFF_MAX = 600;
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";

struct Box { int l = 0, t = 0, r = 0, b = 0; int w() const { return r - l; } int h() const { return b - t; } };
inline Box mk(int l, int t, int r, int b) { Box x; x.l = l; x.t = t; x.r = r; x.b = b; return x; }
inline bool overlap(const Box& a, const Box& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
inline bool same(const Box& a, const Box& b) { return a.l == b.l && a.t == b.t && a.r == b.r && a.b == b.b; }

struct Place { int pos = 0, dx = 0, dy = 0; };

// a whole number token: optional '-', 1-4 digits, nothing else
inline bool intTok(const std::string& s, int& v) {
    if (s.empty() || s.size() > 5) return false;
    size_t i = s[0] == '-' ? 1 : 0;
    if (i == s.size()) return false;
    for (size_t k = i; k < s.size(); ++k) if (s[k] < '0' || s[k] > '9') return false;
    v = std::atoi(s.c_str()); return true;
}
inline bool offOk(int v) { return v >= OFF_MIN && v <= OFF_MAX; }
inline bool posOk(int v) { return v >= 0 && v <= 8; }
// a value from IRT's dialog: in range = itself, else the default (garbage / unset never reach a file)
inline int cleanOff(long long v) { return v >= OFF_MIN && v <= OFF_MAX ? (int)v : 0; }
inline int cleanOff(double v) { return std::isfinite(v) && v >= OFF_MIN && v <= OFF_MAX ? (int)std::lround(v) : 0; }
inline int cleanPos(long long v, int def) { return v >= 0 && v <= 8 ? (int)v : def; }

inline std::vector<std::string> fields(const std::string& line) {
    std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t c = line.find(',', a); f.push_back(line.substr(a, c == std::string::npos ? std::string::npos : c - a)); if (c == std::string::npos) break; a = c + 1; }
    return f;
}
// "pos" (old files) or "pos,dx,dy[,...]". Returns false (and the default) for anything malformed; why = the reason.
inline bool parse(const std::string& lineIn, int defPos, Place& out, std::string* why = nullptr, std::vector<std::string>* rest = nullptr) {
    Place p; p.pos = defPos; out = p;
    std::string line = lineIn;
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) line.pop_back();
    if (line.empty()) { if (why) *why = "empty"; return false; }
    if (line.size() > 120) { if (why) *why = "too long"; return false; }
    const std::vector<std::string> f = fields(line);
    int v = 0;
    if (!intTok(f[0], v) || !posOk(v)) { if (why) *why = "bad position '" + f[0] + "'"; return false; }
    p.pos = v;
    if (f.size() >= 3) {
        int x = 0, y = 0;
        if (!intTok(f[1], x) || !intTok(f[2], y) || !offOk(x) || !offOk(y)) { if (why) *why = "bad offset '" + f[1] + "," + f[2] + "'"; Place d; d.pos = p.pos; out = d; return false; }
        p.dx = x; p.dy = y;
        if (rest) rest->assign(f.begin() + 3, f.end());
    } else if (f.size() == 2) { if (why) *why = "offset Y missing"; out = p; return false; }
    out = p; if (why) why->clear(); return true;
}
inline std::string format(const Place& p) { return std::to_string(posOk(p.pos) ? p.pos : 0) + "," + std::to_string(offOk(p.dx) ? p.dx : 0) + "," + std::to_string(offOk(p.dy) ? p.dy : 0); }

// the 9-spot place of a W x H box inside area (m = margin), then the offset, then clamped fully inside the area
inline Box spot(const Box& area, int W, int H, int pos, int m = 0) {
    if (!posOk(pos)) pos = 0;
    const int c = pos % 3, r = pos / 3;
    int x = c == 0 ? area.l + m : c == 1 ? (area.l + area.r) / 2 - W / 2 : area.r - m - W;
    int y = r == 0 ? area.t + m : r == 1 ? (area.t + area.b) / 2 - H / 2 : area.b - m - H;
    return mk(x, y, x + W, y + H);
}
inline Box clampIn(Box b, const Box& area) {
    const int W = b.w(), H = b.h();
    if (b.r > area.r) { b.l = area.r - W; b.r = area.r; }
    if (b.l < area.l) { b.l = area.l; b.r = area.l + W; }
    if (b.b > area.b) { b.t = area.b - H; b.b = area.b; }
    if (b.t < area.t) { b.t = area.t; b.b = area.t + H; }
    if (b.r > area.r) b.r = area.r;           // the box is wider / taller than the area: cut to it (never outside)
    if (b.b > area.b) b.b = area.b;
    return b;
}
inline Box offset(const Box& b, int dx, int dy, const Box& area) {
    Box o = mk(b.l + (offOk(dx) ? dx : 0), b.t + (offOk(dy) ? dy : 0), b.r + (offOk(dx) ? dx : 0), b.b + (offOk(dy) ? dy : 0));
    return clampIn(o, area);
}
inline Box placeBox(const Box& area, int W, int H, const Place& p, int m = 0) { return offset(clampIn(spot(area, W, H, p.pos, m), area), p.dx, p.dy, area); }

// overlap avoidance with hysteresis (per chart): want = the spot the rules pick this draw (from STABLE inputs only)
struct Hyst {
    static const int CONFIRM = 2, HOLD = 20;
    bool have = false; Box cur; Box cand; int candN = 0; Box left; int leftAge = 1 << 20; long long moves = 0;
    // blockedNow: the current box overlaps a hard obstacle now (then a move is allowed at once, but still not back to 'left' while it is blocked too)
    Box step(const Box& want, bool blockedNow = false) {
        if (leftAge < (1 << 20)) ++leftAge;
        if (!have) { have = true; cur = want; candN = 0; return cur; }
        if (same(want, cur)) { candN = 0; return cur; }
        const bool backToLeft = leftAge < HOLD && same(want, left);
        if (backToLeft && !blockedNow) { candN = 0; return cur; }           // never ping-pong
        if (same(want, cand)) ++candN; else { cand = want; candN = 1; }
        if (candN >= CONFIRM || (blockedNow && candN >= 1 && !backToLeft)) { left = cur; leftAge = 0; cur = want; candN = 0; ++moves; }
        return cur;
    }
    void reset() { *this = Hyst(); }
};

// a width that never shakes: grows at once, shrinks only after the narrower need held SHRINK_MS
struct StableW {
    static const long long SHRINK_MS = 60000;
    int w = 0; long long lowSince = -1; int lowMax = 0;
    int step(int need, long long nowMs) {
        if (need >= w) { w = need; lowSince = -1; lowMax = 0; return w; }
        if (lowSince < 0 || nowMs < lowSince) { lowSince = nowMs; lowMax = need; return w; }
        if (need > lowMax) lowMax = need;
        if (nowMs - lowSince >= SHRINK_MS) { w = lowMax; lowSince = -1; lowMax = 0; }
        return w;
    }
};

// report a fallback once per (key, reason): the host writes the returned line to its status file
struct Fallbacks {
    std::vector<std::string> seen;
    bool once(const std::string& key) { for (size_t i = 0; i < seen.size(); ++i) if (seen[i] == key) return false; if (seen.size() >= 32) seen.erase(seen.begin()); seen.push_back(key); return true; }
    std::string text() const { std::string o; for (size_t i = 0; i < seen.size(); ++i) o += (i ? "; " : "") + seen[i]; return o; }
};

// the settings dialog (Rassul 07:07: the Tape Reader dialog showed "Top left" while the chart used Bottom centre): when the dialog opens
// it is filled from the per-market file; what it shows right after is remembered, and on OK / Apply a control the user did NOT
// change keeps the FILE's value (IRT may ignore the fill or hand back stale values - a stale value is never saved over a real one)
struct DlgSnap {
    bool have = false; std::vector<long long> v;
    void take(const std::vector<long long>& now) { v = now; have = true; }
    long long pick(size_t i, long long now, long long fileVal) const { return have && i < v.size() && v[i] == now ? fileVal : now; }
};

} // namespace bp
#endif
