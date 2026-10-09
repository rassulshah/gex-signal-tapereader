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
