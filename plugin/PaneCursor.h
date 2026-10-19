// PaneCursor.h  1.0.0  (2026-10-18 23:19 CT, lsBidAskStats 1.14.1 / lsBookFlow 1.3.4) - is the mouse inside THIS pane right now?
// Rassul: "when i move the mouse outside the aggression pane the updated values should still be displayed in the info box. for some
// reason my mouse currently needs to be in the aggression pane for me to see updated values."
// ROOT CAUSE: IRT sends a plugin mouse events only while the mouse is over its own pane and sends nothing when it leaves. The panes kept
// the last hovered candle until the cursor left the whole chart WINDOW (HoverTrack::left / mouseLeft) and also followed the candle other
// panes published (HoverBoard.h) - so with the mouse on the price pane the box stayed on an old candle instead of the live one.
// FIX: at every mouse event remember where the pane sits on the screen (screen cursor - the event's pane coordinates) and its window;
// on every draw / calc the cursor is INSIDE only when it is over that window and within the pane's rectangle. Outside = Live.
// Pure except cursorPos (GetCursorPos on Windows; a test hook under PC_TEST_CURSOR).
#pragma once
#if defined(_WIN32) && !defined(PC_TEST_CURSOR)
#include <windows.h>
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")
#endif
#endif

namespace pc {

static const char* const VERSION = "1.0.0";

#if defined(PC_TEST_CURSOR)
extern int test_cx, test_cy; extern bool test_ok;
inline bool cursorPos(int& x, int& y) { x = test_cx; y = test_cy; return test_ok; }
#elif defined(_WIN32)
inline bool cursorPos(int& x, int& y) { POINT p; if (!GetCursorPos(&p)) return false; x = (int)p.x; y = (int)p.y; return true; }
#else
inline bool cursorPos(int& x, int& y) { x = y = 0; return false; }
#endif

struct Rect {
    int l = 0, t = 0, r = 0, b = 0;
    bool ok() const { return r > l && b > t; }
    bool has(int x, int y) const { return x >= l && x < r && y >= t && y < b; }
};

struct Tracker {
    bool have = false; void* wnd = nullptr; int dx = 0, dy = 0;
    Rect pane;                                    // the pane rectangle in the drawing coordinates (set by every render)
    // a mouse event at (h, v) in pane coordinates while the screen cursor was at (sx, sy) over window w
    void onEvent(void* w, bool cursorOk, int sx, int sy, int h, int v) {
        if (!w || !cursorOk) { have = false; return; }
        wnd = w; dx = sx - h; dy = sy - v; have = true;
    }
    // the cursor NOW (window under it, screen position): inside this pane?
    bool inside(void* wNow, bool cursorOk, int sx, int sy) const {
        return have && cursorOk && wNow != nullptr && wNow == wnd && pane.ok() && pane.has(sx - dx, sy - dy);
    }
    // an event's own position: inside the pane rectangle (no rectangle yet = trust the event)
    bool eventInside(int h, int v) const { return !pane.ok() || pane.has(h, v); }
};

} // namespace pc
