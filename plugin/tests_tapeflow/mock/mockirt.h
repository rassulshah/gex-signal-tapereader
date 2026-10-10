// A stand-in for Investor/RT used ONLY to run the real TapeFlow.cpp on Linux: fake trades, a fake chart, a drawing log.
#pragma once
#include <vector>
#include <string>
#include <map>
#include <ctime>
extern long long g_now;                                   // the mock clock (seconds, UTC = "local")
static inline time_t mock_time(time_t* p) { if (p) *p = (time_t)g_now; return (time_t)g_now; }
#define time(x) mock_time(x)
struct MTick { long long t; float px, bid, ask; long q; };
extern std::vector<MTick> g_ticks;                       // all trades IRT "has"
extern std::vector<long long> g_bars;                    // the chart's bar close stamps
extern std::string g_root, g_sym;
extern std::vector<std::string> g_text;                  // text drawn this frame
extern long g_lines, g_rects, g_ttCalls; extern long long g_ttLastStart, g_ttMaxBack;
extern bool g_timerOk; extern int g_timerId;
extern std::map<unsigned long, long> g_lineColors;                  // (2.0.2) lines drawn per pen colour
extern std::vector<std::pair<int, float> > g_pntSets;              // (2.0.3) PNT::set calls with a price
extern int g_ppb;                                                  // (2.0.3) pixels per bar (zoom)
struct MRect { short l, t, r, b; unsigned long fill; };
extern std::vector<MRect> g_rectList;                              // (2.0.3) drawn rectangles
// (2.0.3) line segments (from the last setDrawPosition to drawLineTo) with their pen; dashed lines counted
struct MSeg { short x1, y1, x2, y2; unsigned long color; int style; };
extern std::vector<MSeg> g_segs; extern long g_dashLines;
// (2.0.3) several charts: each has its own bars, OHLC arrays, seconds per bar and user-data slot. mockSelectChart(id) switches
// the "current chart" (what IRT's calls answer for); chart 0 is the default every older suite uses.
void mockSelectChart(int id); int mockCurrentChart(); void mockSetSpb(int spb);
extern std::vector<std::pair<std::string, short> > g_textAt;   // (2.0.3) drawn text and its rectangle's left x
extern int g_paneH;   // (2.0.3) pane height
extern float g_tickIncr;   // (2.0.3) the chart's SYM_TICKINCR
struct MText { std::string s; short l, t, r, b; unsigned long color; };
extern std::vector<MText> g_textRects;   // (2.0.4) drawn text with its rectangle
extern float g_yOrigin, g_yPxPerPt;      // (2.0.4) overlay price -> y
extern bool g_ticksThrow;   // (2.0.4) RTTICKS throws (fault injection)
