// ArrivalStamp.h (1.0.0, 2026-10-22) -- MBO recorder fix P1 (MBO-RECORDING-DIAGNOSIS-2026-10-21, Rassul 07:08 "complete the fixes").
// WHAT: IRT stamps a few order-feed rows (MOD / DEL / FILL) with the RESTING order's own time instead of the time the event happened,
//   and after a re-subscription it re-sends the resting book with the orders' original times (ES 10-19 17:08: 2,962 rows "behind").
//   Written as-is, such a row lands in an older hour file and replays up to 16 minutes too early.
// HOW: each recorder keeps one Clock. An event stamped EARLIER than the latest time already written is re-stamped to that latest time
//   (= when it arrived; the feed is drained in arrival order). A jump back of 30 minutes or more is a real clock change (the DST
//   fall-back hour) and is kept. Counted: restamped rows and the biggest jump back (seconds), shown in the recorder's status line.
// The Python side does the same for old files (analytics\lra\mbo_live_portal.py 1.2.0 restamp_feed_order), so both agree.
// Pure logic, no SDK: tests in tests_of\test_arrival_stamp.cpp.
#ifndef ARRIVAL_STAMP_H
#define ARRIVAL_STAMP_H

namespace arr {

static const char* const VERSION = "1.0.0";
static const long long KEEP_BACK_S = 1800;     // a jump back this big (or bigger) is a clock change, not a stale stamp

// days since 1970-01-01 of a civil date (proleptic Gregorian)
inline long long daysOf(long long y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}

struct Clock {
    long long last = -1;                          // seconds of the latest stamp written (-1 = none yet)
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0; // its clock fields
    long long restamped = 0, maxBackS = 0, kept = 0;

    // in: the event's clock fields; out: the fields to write. true = the event was re-stamped to its arrival time.
    bool apply(int& ey, int& emo, int& ed, int& eh, int& emi, int& es) {
        if (emo < 1 || emo > 12 || ed < 1 || ed > 31) return false;       // not a date: leave it to the caller's own check
        const long long t = daysOf(ey, (unsigned)emo, (unsigned)ed) * 86400LL + eh * 3600LL + emi * 60LL + es;
        if (last >= 0 && t < last) {
            const long long back = last - t;
            if (back < KEEP_BACK_S) {
                ++restamped; if (back > maxBackS) maxBackS = back;
                ey = y; emo = mo; ed = d; eh = h; emi = mi; es = s;
                return true;
            }
            ++kept;                                                        // the DST fall-back hour: the clock really went back
        }
        last = t; y = ey; mo = emo; d = ed; h = eh; mi = emi; s = es;
        return false;
    }
    void reset() { last = -1; }
};

} // namespace arr
#endif
