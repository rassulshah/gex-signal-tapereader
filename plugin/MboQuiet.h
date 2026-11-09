// MboQuiet.h 1.0.0 (2026-11-09, Rassul 15:27 CT: "if mbo is quiet, display a message on the chart somewhere") - SDK-free.
// The live order-by-order (MBO) feed is silent when NO order event arrived for N minutes while the market is open:
//   N = 2 min 07:00-15:00 CT (the day session), 5 min otherwise (overnight). Recorded feed, 2026-11-05, longest silence outside
//   the 16:00-17:00 halt: day CL 5.9 s, GC 6.8 s, HG 4.1 s, EU 4.9 s, NG 24.7 s; overnight CL 33 s, HG 26 s, EU 49 s, NG 168 s,
//   GC 176 s - so 2 / 5 min never fires on a normal feed and catches a dead one (today 7:33-7:57 CT and 9:27-9:54 CT).
// No check while CME Globex is closed: the 16:00-17:00 CT daily halt, Friday 16:00 -> Sunday 17:00. The silence is counted from the
// later of the last event and the current open (a 17:00 reopen never shows "quiet 61m").
#pragma once
#include <string>

namespace mbq {
static const char* const VERSION = "MboQuiet 1.0.0";
static const int DAY_FROM = 7 * 60, DAY_TO = 15 * 60, DAY_MIN = 2, ON_MIN = 5, OPEN = 17 * 60, HALT = 16 * 60;

// minutes the market has been open in the current Globex stretch at (ctMin = minute of the CT day, wday 0 = Sunday); -1 = closed
inline int minutesOpen(int ctMin, int wday) {
    if (ctMin < 0 || ctMin >= 1440 || wday < 0 || wday > 6) return -1;
    if (wday == 6) return -1;                                   // Saturday
    if (wday == 0) return ctMin >= OPEN ? ctMin - OPEN : -1;    // Sunday: opens 17:00
    if (ctMin >= HALT && ctMin < OPEN) return -1;               // daily halt
    if (wday == 5 && ctMin >= HALT) return -1;                  // Friday close
    return ctMin >= OPEN ? ctMin - OPEN : ctMin + (1440 - OPEN);
}
// the silence (minutes) that counts as quiet now; 0 = no check (closed)
inline int thresholdMin(int ctMin, int wday) {
    if (minutesOpen(ctMin, wday) < 0) return 0;
    return (ctMin >= DAY_FROM && ctMin < DAY_TO) ? DAY_MIN : ON_MIN;
}
// whole minutes of silence: from the later of the last event and the open of this stretch
inline long long quietMin(long long lastEvSec, long long nowSec, int ctMin, int wday) {
    const int open = minutesOpen(ctMin, wday); if (open < 0 || nowSec <= 0) return 0;
    long long from = nowSec - (long long)open * 60;
    if (lastEvSec > from) from = lastEvSec;
    return nowSec > from ? (nowSec - from) / 60 : 0;
}
// "MBO QUIET 12m" or "" (plain ASCII)
inline std::string text(long long lastEvSec, long long nowSec, int ctMin, int wday) {
    const int thr = thresholdMin(ctMin, wday); if (thr <= 0) return std::string();
    const long long q = quietMin(lastEvSec, nowSec, ctMin, wday);
    return q >= thr ? "MBO QUIET " + std::to_string(q) + "m" : std::string();
}
} // namespace mbq
