#ifndef LS_HOVERBOARD_H
#define LS_HOVERBOARD_H
// HoverBoard.h (1.0.0, 2026-10-13 9:45 PM CT) -- ONE chart-wide hover for lsBidAskStats / lsPullStats / lsStackStats / lsTapeReader.
// Rassul 9:25 PM: "when i move the mouse the tape reader changes but the other reads don't change". IRT sends a plugin mouse events only
// for its own pane, so each pane followed the mouse only while the mouse was inside it. Now every one of the four plugins PUBLISHES the
// candle under the mouse when it gets a mouse event, and every one FOLLOWS what was published for its chart:
//   board   one process-wide variable per chart symbol and bar size, LRA_HV_<symbol>_<spb> = "<window>,<bar time>,<first>,<last>"
//           (the process environment block: shared by every plugin DLL in the IRT process; no files, no window properties to leak)
//           window = the window the mouse event came from (WindowFromPoint), bar time = the hovered CLOSED candle's time (0 = the
//           forming candle / past the last bar = Live), first / last = the chart's first and last VISIBLE bar times
//   follow  the window under the cursor NOW is the publisher's window AND its visible range is this chart's (the time axis is shared
//           by every pane of one chart, so a second chart of the same symbol - scrolled or zoomed differently - never matches) ->
//           that candle; anything else (the mouse left the chart, another chart, another program) -> Live (the last closed candle)
// Cost: a GetCursorPos + WindowFromPoint + one environment read per draw / calc; the chart is invalidated only when the hovered candle
// changes. The pure part (encode / decode / follow) is unit-tested on Linux; the Win32 calls are behind hbCursor / hbSet / hbGet.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#if defined(_WIN32) && !defined(HB_FAKE)
#include <windows.h>
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")   // GetCursorPos / WindowFromPoint (Set / GetEnvironmentVariableA are kernel32)
#endif
#endif

namespace hb {

static const char* const VERSION = "1.0.0";
struct Pub { unsigned long long wnd = 0, t = 0, t0 = 0, t1 = 0; };

// "LRA_HV_ES_180" - the symbol's letters / digits only (an environment name never holds '=' or spaces)
inline std::string varName(const std::string& sym, long spb) {
    std::string o = "LRA_HV_";
    for (size_t i = 0; i < sym.size() && i < 24; ++i) { const char c = sym[i]; if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) o += c; }
    return o + "_" + std::to_string(spb > 0 ? spb : 0);
}
inline std::string encode(const Pub& p) {
    char b[96]; std::snprintf(b, sizeof(b), "%llu,%llu,%llu,%llu", p.wnd, p.t, p.t0, p.t1); return b;
}
inline bool decode(const std::string& s, Pub& out) {
    if (s.empty() || s.size() > 90) return false;
    Pub p; unsigned long long* f[4] = { &p.wnd, &p.t, &p.t0, &p.t1 }; size_t a = 0;
    for (int k = 0; k < 4; ++k) {
        const size_t c = s.find(',', a); const std::string x = s.substr(a, c == std::string::npos ? std::string::npos : c - a);
        if (x.empty() || x.size() > 20 || x.find_first_not_of("0123456789") != std::string::npos) return false;
        *f[k] = std::strtoull(x.c_str(), nullptr, 10);
        if ((c == std::string::npos) != (k == 3)) return false;
        a = c + 1;
    }
    out = p; return true;
}
// follow: true + t (0 = Live) when the cursor is over the publisher's window of THIS chart; false = not over this chart (Live)
inline bool follow(const Pub& p, bool have, unsigned long long cursorWnd, unsigned long long t0, unsigned long long t1, unsigned long long& t) {
    t = 0;
    if (!have || cursorWnd == 0 || p.wnd != cursorWnd || p.t0 != t0 || p.t1 != t1 || t0 == 0) return false;
    t = p.t; return true;
}
// the hovered bar index from a published time: the bar whose time equals it, never the forming bar (count - 1); -1 = Live
template <class Times> int indexOf(Times& times, long count, unsigned long long t) {   // non-const: the SDK array operator[] is non-const
    if (t == 0 || count < 2) return -1;
    long lo = 0, hi = count - 2;                                            // closed bars only
    while (lo <= hi) {
        const long mid = lo + (hi - lo) / 2; const unsigned long long v = (unsigned long long)times[(int)mid];
        if (v == t) return (int)mid;
        if (v < t) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}

// ---- the process-wide calls (Win32), or a fake for the Linux tests --------------------------------------------------------
#if defined(_WIN32) && !defined(HB_FAKE)
inline unsigned long long hbCursor() { POINT p; if (!GetCursorPos(&p)) return 0; return (unsigned long long)(size_t)WindowFromPoint(p); }
inline void hbSet(const std::string& n, const std::string& v) { SetEnvironmentVariableA(n.c_str(), v.c_str()); }
inline bool hbGet(const std::string& n, std::string& v) {
    char b[128]; const DWORD k = GetEnvironmentVariableA(n.c_str(), b, (DWORD)sizeof(b));
    if (k == 0 || k >= sizeof(b)) return false;
    v.assign(b, (size_t)k); return true;
}
#else
inline unsigned long long& fakeCursor() { static unsigned long long w = 0; return w; }
inline unsigned long long hbCursor() { return fakeCursor(); }
inline void hbSet(const std::string& n, const std::string& v) { setenv(n.c_str(), v.c_str(), 1); }
inline bool hbGet(const std::string& n, std::string& v) { const char* s = std::getenv(n.c_str()); if (!s) return false; v = s; return true; }
#endif

// a plugin got a mouse event over its pane: publish the candle (0 = Live) for its chart
inline void publish(const std::string& sym, long spb, unsigned long long t, unsigned long long t0, unsigned long long t1) {
    const unsigned long long w = hbCursor(); if (w == 0 || t0 == 0 || sym.empty()) return;
    Pub p; p.wnd = w; p.t = t; p.t0 = t0; p.t1 = t1; hbSet(varName(sym, spb), encode(p));
}
// the candle this chart's panes should show now: false = no board entry for this symbol / bar size yet (keep the pane's own mouse
// hover, e.g. outside IRT's process in tests); true + t = the board decides (t = the candle time, 0 = Live)
inline bool hovered(const std::string& sym, long spb, unsigned long long t0, unsigned long long t1, unsigned long long& t) {
    t = 0; std::string v; Pub p;
    if (sym.empty() || !hbGet(varName(sym, spb), v) || !decode(v, p)) return false;
    follow(p, true, hbCursor(), t0, t1, t); return true;
}

} // namespace hb
#endif
