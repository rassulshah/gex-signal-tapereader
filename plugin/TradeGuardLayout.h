#ifndef LS_TRADEGUARD_LAYOUT_H
#define LS_TRADEGUARD_LAYOUT_H
// TradeGuardLayout.h (lsTradeManager 1.2.0, 2026-10-10) -- the SDK-free half of the Topstep Guard box: where it may go, the
// 9-spot Position, the drag (click on the box, move, release), and the per-chart file that remembers a dragged spot.
// Tested by test_tradeguard_layout.cpp (g++ / MinGW, sanitizers) and test_trademanager_mock.cpp (the real TradeManager.cpp on a
// mock IRT host with two charts).
//
// Rules (Rassul 2026-10-07 / 10-09): the box never overlaps the profiles; layout and centring use only the space up to where
// price ends (the last bar), never the empty space right of price; 9 spots Top/Middle/Bottom x Left/Centre/Right.
// (2026-10-10) the box can also be dragged; the dragged spot is remembered PER CHART (market + bar size).
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cctype>

namespace tgl {

struct Box { int l = 0, t = 0, r = 0, b = 0; int w() const { return r - l; } int h() const { return b - t; } };
inline bool inside(const Box& b, int x, int y, int slop = 2) { return x >= b.l - slop && x <= b.r + slop && y >= b.t - slop && y <= b.b + slop; }

// ---- strict numbers (checklist #4): the whole token, finite, in range
inline bool parseIntStrict(const std::string& s, long long lo, long long hi, long long& out)
{
    if (s.empty() || s.size() > 20 || std::isspace((unsigned char)s[0])) return false;   // the whole token, nothing around it
    const char* p = s.c_str(); char* e = nullptr; errno = 0;
    long long v = std::strtoll(p, &e, 10);
    if (e == p || *e || errno == ERANGE || v < lo || v > hi) return false;
    out = v; return true;
}
inline bool parseDoubleStrict(const std::string& s, double lo, double hi, double& out)
{
    if (s.empty() || s.size() > 32 || std::isspace((unsigned char)s[0])) return false;
    const char* p = s.c_str(); char* e = nullptr; errno = 0;
    double v = std::strtod(p, &e);
    if (e == p || *e || errno == ERANGE || !std::isfinite(v) || v < lo || v > hi) return false;
    out = v; return true;
}

// ---- where the profiles start (the same rule every LRA box uses: Dealer Profile REACH from the right edge, plus the Delta
// Profile's WIDTH and its letters when it sits on the right). Parsed strictly from their status files; bad lines are ignored.
struct Profiles { int reach = 150; int deltaW = 50; bool deltaLeft = false; };
inline void parseProfileStatus(const std::string& dealerText, const std::string& deltaText, Profiles& P)
{
    auto lines = [](const std::string& t, std::vector<std::string>& out) {
        size_t p = 0; int n = 0;
        while (p < t.size() && n++ < 200) { size_t e = t.find('\n', p); if (e == std::string::npos) e = t.size(); std::string l = t.substr(p, e - p); if (!l.empty() && l.back() == '\r') l.pop_back(); out.push_back(l); p = e + 1; }
    };
    std::vector<std::string> a, b; lines(dealerText, a); lines(deltaText, b);
    long long v = 0;
    for (const auto& l : a) if (l.rfind("REACH,", 0) == 0 && parseIntStrict(l.substr(6), 40, 1500, v)) P.reach = (int)v;
    for (const auto& l : b) {
        if (l.rfind("WIDTH,", 0) == 0 && parseIntStrict(l.substr(6), 20, 600, v)) P.deltaW = (int)v;
        if (l.rfind("PLACE,", 0) == 0) P.deltaLeft = l.substr(6) == "Left";
    }
}
inline int profilesLeft(int paneRight, const Profiles& P)
{
    int r = paneRight - P.reach - 8;
    if (!P.deltaLeft) r -= P.deltaW + 110;
    return r;
}

// ---- the room the box may use: below IRT's title line; never right of the profiles (hard); and, when there is enough room,
// not right of where price ends (soft: the box may reach past the last bar only when the room left of price is too narrow,
// and still never into the profiles). lastBarRight <= 0 = unknown.
static const int TITLE_H = 18, MARGIN = 6;
inline Box room(int paneL, int paneT, int paneR, int paneB, int profL, int lastBarRight, int boxW)
{
    Box b; b.l = paneL + MARGIN; b.t = paneT + TITLE_H; b.b = paneB - MARGIN;
    int hardR = paneR - MARGIN;
    if (profL > paneL && profL - MARGIN < hardR) hardR = profL - MARGIN;
    int R = hardR;
    if (lastBarRight > b.l && lastBarRight < R) R = lastBarRight;
    if (R - b.l < boxW) R = (b.l + boxW < hardR) ? b.l + boxW : hardR;
    b.r = R;
    return b;
}
inline bool fits(const Box& rm, int w, int h) { return rm.w() >= w && rm.h() >= h && w > 0 && h > 0; }

// ---- positions
inline void clampXY(const Box& rm, int w, int h, int& x, int& y)
{
    if (x + w > rm.r) x = rm.r - w;
    if (x < rm.l) x = rm.l;
    if (y + h > rm.b) y = rm.b - h;
    if (y < rm.t) y = rm.t;
}
// the 9 spots: 0 Top left ... 8 Bottom right (the same list as dl::ANCHORS)
inline void anchor(const Box& rm, int w, int h, int pos, int& x, int& y)
{
    if (pos < 0 || pos > 8) pos = 2;
    int col = pos % 3, row = pos / 3;
    x = col == 0 ? rm.l : col == 1 ? (rm.l + rm.r) / 2 - w / 2 : rm.r - w;
    y = row == 0 ? rm.t : row == 1 ? (rm.t + rm.b) / 2 - h / 2 : rm.b - h;
    clampXY(rm, w, h, x, y);
}
// a dragged spot is kept as fractions of the box's free travel in the room (0 = left / top, 1 = right / bottom), so it stays
// in the same place relative to the price area when the window is resized, and is always inside the room.
inline void toFrac(const Box& rm, int w, int h, int x, int y, double& fx, double& fy)
{
    int tx = rm.w() - w, ty = rm.h() - h;
    fx = tx > 0 ? (double)(x - rm.l) / tx : 0.0; fy = ty > 0 ? (double)(y - rm.t) / ty : 0.0;
    fx = fx < 0 ? 0.0 : (fx > 1 ? 1.0 : fx);
    fy = fy < 0 ? 0.0 : (fy > 1 ? 1.0 : fy);
}
inline void fromFrac(const Box& rm, int w, int h, double fx, double fy, int& x, int& y)
{
    int tx = rm.w() - w, ty = rm.h() - h;
    x = rm.l + (tx > 0 ? (int)std::lround(fx * tx) : 0);
    y = rm.t + (ty > 0 ? (int)std::lround(fy * ty) : 0);
    clampXY(rm, w, h, x, y);
}

// ---- the per-chart drag file: lsFlexLevels\TradeManager.drag-<MKT>_<seconds per bar>.txt = "TGDRAG|1|<pos>|<fx>|<fy>"
// pos = the Position setting in force when it was dragged: a later different Position (chosen in the settings) wins.
struct Saved { bool ok = false; int pos = -1; double fx = 0, fy = 0; };
inline std::string chartKey(const std::string& mkt, int spb) { return mkt + "_" + std::to_string(spb > 0 ? spb : 0); }
inline std::string formatSaved(int pos, double fx, double fy)
{
    char b[96]; std::snprintf(b, sizeof b, "TGDRAG|1|%d|%.5f|%.5f\n", pos, fx, fy); return b;
}
inline Saved parseSaved(const std::string& text)
{
    Saved s;
    if (text.size() > 256) return s;
    std::string t = text;
    while (!t.empty() && (t.back() == '\n' || t.back() == '\r' || t.back() == ' ')) t.pop_back();
    std::vector<std::string> c; size_t p = 0;
    while (p <= t.size()) { size_t e = t.find('|', p); if (e == std::string::npos) e = t.size(); c.push_back(t.substr(p, e - p)); p = e + 1; if (c.size() > 6) return s; }
    if (c.size() != 5 || c[0] != "TGDRAG" || c[1] != "1") return s;
    long long pos = 0; double fx = 0, fy = 0;
    if (!parseIntStrict(c[2], 0, 8, pos) || !parseDoubleStrict(c[3], 0, 1, fx) || !parseDoubleStrict(c[4], 0, 1, fy)) return s;
    s.ok = true; s.pos = (int)pos; s.fx = fx; s.fy = fy;
    return s;
}
// the dragged spot is used only while the Position setting is the one it was dragged from, and the setting was not saved
// after the drag (placeStamp > dragStamp = the Position was chosen again later)
inline bool savedApplies(const Saved& s, int placePos, long long dragStamp, long long placeStamp)
{
    return s.ok && s.pos == placePos && (placeStamp < 0 || dragStamp >= placeStamp);
}

// ---- the drag. IRT sends E_MOUSE_CLICK (down), E_MOUSE_MOVE, E_MOUSE_UP, E_MOUSE_DBL. The plugin turns each into pane
// coordinates with the SDK's PNT::getMouse (havePos); if that fails, MOVE / UP use the event's v.mouse h / v (the SDK documents
// v.mouse for those two) and a press uses the last MOVE position. A MOVE while dragging with the button already released
// (released outside the chart) ends the drag there.
enum MouseEv { M_DOWN = 0, M_MOVE = 1, M_UP = 2, M_DBL = 3 };
struct Drag {
    bool down = false, moved = false;
    int grabDX = 0, grabDY = 0, x = 0, y = 0, startX = 0, startY = 0;
    int lastH = -10000, lastV = -10000;
};
struct MouseOut { bool handled = false, redraw = false, commit = false, reset = false, hover = false; };
static const int DRAG_SLOP = 3;          // a press that moves less than this is a click, not a drag (nothing is saved)
inline MouseOut onMouse(Drag& d, int ev, int h, int v, bool havePos, bool buttonHeld, const Box& box, const Box& rm)
{
    MouseOut o;
    const int bw = box.w(), bh = box.h();
    if (!havePos && ev == M_DBL) { h = d.lastH; v = d.lastV; }
    if (ev == M_MOVE) {
        d.lastH = h; d.lastV = v;
        if (d.down && !buttonHeld) ev = M_UP;                       // released outside: finish the drag at this point
        else if (d.down) {
            int nx = h - d.grabDX, ny = v - d.grabDY;
            clampXY(rm, bw, bh, nx, ny);
            if (std::abs(nx - d.startX) >= DRAG_SLOP || std::abs(ny - d.startY) >= DRAG_SLOP) d.moved = true;
            if (nx != d.x || ny != d.y) { d.x = nx; d.y = ny; o.redraw = true; }
            o.handled = true; o.hover = true;
            return o;
        } else { o.hover = inside(box, h, v); return o; }
    }
    if (ev == M_DOWN) {
        int ph = havePos ? h : d.lastH, pv = havePos ? v : d.lastV;
        d.lastH = ph; d.lastV = pv;
        if (!inside(box, ph, pv) || bw <= 0 || bh <= 0) return o;
        d.down = true; d.moved = false; d.grabDX = ph - box.l; d.grabDY = pv - box.t;
        d.x = d.startX = box.l; d.y = d.startY = box.t;
        o.handled = true; o.hover = true;
        return o;
    }
    if (ev == M_UP) {
        if (!d.down) return o;
        int nx = h - d.grabDX, ny = v - d.grabDY;
        clampXY(rm, bw, bh, nx, ny);
        if (std::abs(nx - d.startX) >= DRAG_SLOP || std::abs(ny - d.startY) >= DRAG_SLOP) d.moved = true;
        d.x = nx; d.y = ny; d.down = false;
        o.handled = true; o.redraw = true; o.commit = d.moved;
        if (!d.moved) { d.x = d.startX; d.y = d.startY; }
        return o;
    }
    if (ev == M_DBL) {
        if (!inside(box, h, v)) return o;
        d.down = false; d.moved = false;
        o.handled = true; o.redraw = true; o.reset = true;
        return o;
    }
    return o;
}

} // namespace tgl
#endif
