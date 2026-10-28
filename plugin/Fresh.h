#ifndef LS_FRESH_H
#define LS_FRESH_H
// Fresh.h 1.1.0 (2026-10-28; 1.0.0 2026-10-18) -- ONE rule for "how old is this input and is it STALE", shared by every LRA plugin. SDK-free, tested
// (tests_fresh/test_fresh.cpp). Audit C finding 7 (2026-10-17): the same MenthorQ grid was "STALE" on one indicator and live on another
// (HodLodGex 20 min, Dealer Profile fade 20 / badge 25 min, Tape Reader 18 h outside RTH, CobImpulse its own), and the HOD / LOD box drew
// "Reg: Neg" from a session file 1,902 min old with no mark. Rassul's rule: stale data is never shown as live.
//
// TIME: every time here is "CT wall-clock seconds" = civil days since 1970-01-01 x 86400 + seconds of the Central day (the convention
// every plugin already uses for bar stamps / SignalBoard). A unix time converts with ctFromUnix(unix, offsetSec) where offsetSec is the
// PC's Central offset (offsetFromClock: from the same moment's unix time and CT minute of day).
//
// THE THRESHOLDS (the same for every indicator; change them only here):
//   intraday MenthorQ-derived inputs (kinds K_MQ_LEVELS, K_DEALER, K_SESSION, K_GEX):
//     * while MenthorQ updates futures (Sunday 21:40 -> Friday 16:00 CT, paused 16:00-21:40 each evening; lra.menthorq._update_hours,
//       the same window as HodLodGex.h / DealerProfileAuditLogic.h): LIVE when the data is <= 20 min old (LIVE_MAX_MIN; a grid arrives
//       every ~10-15 min, so 20 min = one refresh missed), else STALE. A feed the bridge reports STOPPED is STALE at any age.
//     * outside those hours (the evening pause, the weekend): PAUSED - the closing set, shown with its time - when it was written no
//       earlier than 20 min (PAUSE_SLACK_MIN) before the last update window closed (the last weekday 16:00 CT); older = STALE.
//     * (1.1.0, 2026-10-28) the LIVE limit depends on the session: 20 min in RTH (weekday 08:30-16:00 CT), 45 min (LIVE_MAX_MIN_ON)
//       from 16:00 to 08:30 CT - overnight MenthorQ's own as-of stamp runs ~20 min behind the clock, so a just-arrived set read
//       "STALE 20m" (HOD / LOD "GAMMA STALE" on 10-27 22:00-23:00 CT while sets were arriving every 5 min).
//     * (1.1.0) judgeArrived(): when the caller knows WHEN the set arrived (the file's write time), freshness is judged by the ARRIVAL
//       age; the as-of stays for display and must itself be within the limit + ASOF_EXTRA_MIN (a file re-written with an old set is
//       not live). judge() without an arrival time keeps judging the as-of, with the same session limits.
//   daily inputs (K_QSCORE, MenthorQ Q-Scores, end of day): LIVE when the date is the latest weekday whose scores are due by now
//     (due 22:00 CT that day, QSCORE_DUE_MIN; the bridge receives them ~20:10), else STALE. Exchange holidays are not known here: on
//     the day after a holiday the scores read STALE (honest: no new scores were published).
//   another plugin's status file (K_TAPE_STATUS, e.g. TapeReader.status-<MKT>.txt): fresh when written in the last 120 s
//     (TAPE_STATUS_MAX_SEC; the Tape Reader writes it at least every 30 s while it draws).
//   unknown, missing or future (> 120 s ahead) time = STALE (lesson 34: unknown age is never live).
#include <string>
#include <cstdio>
#include <cstdlib>

namespace fr {

static const char* const VERSION = "1.1.0";
enum Kind { K_MQ_LEVELS = 0, K_DEALER, K_SESSION, K_QSCORE, K_GEX, K_TAPE_STATUS };
enum State { LIVE = 0, PAUSED = 1, STALE = 2 };
static const int LIVE_MAX_MIN = 20;          // RTH (weekday 08:30-16:00 CT)
static const int LIVE_MAX_MIN_ON = 45;       // (1.1.0) 16:00-08:30 CT and Sunday evening
static const int ASOF_EXTRA_MIN = 45;        // (1.1.0) judgeArrived: the as-of may lag the arrival by at most this on top of the limit
static const int RTH_START_MIN = 8 * 60 + 30;
static const int RTH_END_MIN = 16 * 60;
static const int PAUSE_SLACK_MIN = 20;
static const int UPDATE_END_MIN = 16 * 60;          // 16:00 CT: MenthorQ stops updating futures
static const int UPDATE_START_MIN = 21 * 60 + 40;   // 21:40 CT: it starts again (Sunday to Thursday evenings)
static const int QSCORE_DUE_MIN = 22 * 60;
static const int TAPE_STATUS_MAX_SEC = 120;
static const long long FUTURE_SLACK_SEC = 120;

inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
inline int weekday(long long days) { long long w = (days + 4) % 7; return (int)(w < 0 ? w + 7 : w); }   // 0 = Sunday (1970-01-01 = Thursday)
inline long long civilDays(int y, int m, int d)
{
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (unsigned)((153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1); const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
inline long long ctAbs(int y, int m, int d, int secOfDay) { return civilDays(y, m, d) * 86400LL + secOfDay; }
// "2026-10-16" -> civil days; false = not a valid date
inline bool parseYmd(const std::string& s, long long& days)
{
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (size_t i = 0; i < 10; ++i) if (i != 4 && i != 7 && (s[i] < '0' || s[i] > '9')) return false;
    const int y = std::atoi(s.substr(0, 4).c_str()), m = std::atoi(s.substr(5, 2).c_str()), d = std::atoi(s.substr(8, 2).c_str());
    if (y < 2000 || y > 2200 || m < 1 || m > 12 || d < 1 || d > 31) return false;
    days = civilDays(y, m, d); return true;
}
// the PC's Central offset (seconds, CT = unix + offset) from one moment read both ways; whole quarter hours, -12 h .. +12 h
inline long long offsetFromClock(long long unixNow, int ctMinuteOfDay)
{
    long long d = (long long)ctMinuteOfDay * 60 - floorDiv(unixNow, 60) % 1440 * 60;
    while (d < -43200) d += 86400;
    while (d >= 43200) d -= 86400;
    return floorDiv(d + 450, 900) * 900;
}
inline long long ctFromUnix(long long unixT, long long offsetSec) { return unixT <= 0 ? 0 : unixT + offsetSec; }

// MenthorQ's futures update window (wd 0 = Sunday, min = CT minute of day)
inline bool updateHours(int wd, int min)
{
    if (wd == 6 || (wd == 5 && min > UPDATE_END_MIN) || (wd == 0 && min < UPDATE_START_MIN)) return false;
    return !(min > UPDATE_END_MIN && min < UPDATE_START_MIN);
}
inline bool updateHoursAt(long long ct) { const long long d = floorDiv(ct, 86400); return updateHours(weekday(d), (int)((ct - d * 86400) / 60)); }
// (1.1.0) the LIVE limit (minutes) at nowCt: 20 in RTH (Mon-Fri 08:30-16:00 CT), 45 otherwise
inline int liveLimitMinAt(long long ct)
{
    const long long d = floorDiv(ct, 86400); const int wd = weekday(d); const int mn = (int)((ct - d * 86400) / 60);
    return (wd >= 1 && wd <= 5 && mn >= RTH_START_MIN && mn < RTH_END_MIN) ? LIVE_MAX_MIN : LIVE_MAX_MIN_ON;
}
// the latest close of an update window (a weekday 16:00 CT) at or before nowCt
inline long long lastUpdateEnd(long long nowCt)
{
    const long long d = floorDiv(nowCt, 86400);
    for (int k = 0; k < 10; ++k) { const long long day = d - k; const int wd = weekday(day); const long long e = day * 86400 + UPDATE_END_MIN * 60LL;
                                   if (wd >= 1 && wd <= 5 && e <= nowCt) return e; }
    return nowCt;
}
// "25m" / "3h" / "2d"
inline std::string ageShort(long long sec)
{
    if (sec < 0) return "?";
    char b[24]; const long long m = sec / 60;
    if (m < 90) std::snprintf(b, sizeof(b), "%lldm", m); else if (m < 2880) std::snprintf(b, sizeof(b), "%lldh", (m + 30) / 60); else std::snprintf(b, sizeof(b), "%lldd", (m + 720) / 1440);
    return b;
}
inline const char* word(State s) { return s == LIVE ? "LIVE" : (s == PAUSED ? "PAUSED" : "STALE"); }

struct Verdict {
    State st = STALE; long long ageSec = -1; bool known = false; std::string tag;   // tag: "" (LIVE), "PAUSED", "STALE 25m", "STALE ?"
    bool stale() const { return st == STALE; }
    bool usable() const { return st != STALE; }   // LIVE or PAUSED (the closing set while MenthorQ does not update)
};
inline Verdict staleV(long long age, bool known) { Verdict v; v.st = STALE; v.ageSec = age; v.known = known; v.tag = "STALE " + (known ? ageShort(age) : std::string("?")); return v; }

// an intraday input (kinds K_MQ_LEVELS / K_DEALER / K_SESSION / K_GEX) stamped asofCt, judged at nowCt
inline Verdict judge(Kind k, long long asofCt, long long nowCt, bool feedStopped = false)
{
    (void)k;   // one rule for every intraday kind (the kind is kept for the status lines and for a future per-kind rule)
    if (asofCt <= 0 || nowCt <= 0 || asofCt > nowCt + FUTURE_SLACK_SEC) return staleV(-1, false);
    const long long age = asofCt > nowCt ? 0 : nowCt - asofCt;
    if (updateHoursAt(nowCt)) {
        if (age <= liveLimitMinAt(nowCt) * 60LL && !feedStopped) { Verdict v; v.st = LIVE; v.ageSec = age; v.known = true; return v; }
        return staleV(age, true);
    }
    if (asofCt >= lastUpdateEnd(nowCt) - PAUSE_SLACK_MIN * 60LL) { Verdict v; v.st = PAUSED; v.ageSec = age; v.known = true; v.tag = "PAUSED"; return v; }
    return staleV(age, true);
}
// (1.1.0) the same input with its ARRIVAL time known (arrivedCt = when the set was written here, CT wall-clock seconds; <= 0 = unknown ->
// judge()). In MenthorQ hours: LIVE when it arrived within the limit (20 min RTH / 45 min otherwise), its as-of is within the limit +
// ASOF_EXTRA_MIN, and the feed is not stopped. ageSec / the tag show the AS-OF age (what the data describes). Outside the hours: judge().
inline Verdict judgeArrived(Kind k, long long asofCt, long long arrivedCt, long long nowCt, bool feedStopped = false)
{
    if (arrivedCt <= 0) return judge(k, asofCt, nowCt, feedStopped);
    if (asofCt <= 0 || nowCt <= 0 || asofCt > nowCt + FUTURE_SLACK_SEC || arrivedCt > nowCt + FUTURE_SLACK_SEC) return staleV(-1, false);
    if (!updateHoursAt(nowCt)) return judge(k, asofCt, nowCt, feedStopped);
    const long long age = asofCt > nowCt ? 0 : nowCt - asofCt;
    const long long arrAge = arrivedCt > nowCt ? 0 : nowCt - arrivedCt;
    const long long lim = liveLimitMinAt(nowCt) * 60LL;
    if (!feedStopped && arrAge <= lim && age <= lim + ASOF_EXTRA_MIN * 60LL) { Verdict v; v.st = LIVE; v.ageSec = age; v.known = true; return v; }
    return staleV(arrAge > lim ? arrAge : age, true);
}
// the Q-Scores day that must be on file at nowCt (civil days): the latest weekday whose scores were due by now
inline long long requiredDay(long long nowCt)
{
    const long long d = floorDiv(nowCt, 86400);
    for (int k = 0; k < 10; ++k) { const long long day = d - k; const int wd = weekday(day);
                                   if (wd >= 1 && wd <= 5 && day * 86400 + QSCORE_DUE_MIN * 60LL <= nowCt) return day; }
    return d;
}
// a daily input (K_QSCORE) dated dataDay (civil days, -1 = none)
inline Verdict judgeDaily(long long dataDay, long long nowCt)
{
    if (dataDay < 0 || nowCt <= 0 || dataDay > floorDiv(nowCt, 86400)) return staleV(-1, false);
    const long long need = requiredDay(nowCt);
    const long long age = nowCt - (dataDay * 86400 + QSCORE_DUE_MIN * 60LL);
    if (dataDay >= need) { Verdict v; v.st = LIVE; v.ageSec = age < 0 ? 0 : age; v.known = true; return v; }
    return staleV(age < 0 ? 0 : age, true);
}
// another plugin's status file: written within maxSec (and not from the future) = that plugin is drawing now
inline bool fileFresh(long long mtimeUnix, long long nowUnix, int maxSec = TAPE_STATUS_MAX_SEC)
{
    return mtimeUnix > 0 && nowUnix > 0 && nowUnix - mtimeUnix <= maxSec && mtimeUnix - nowUnix <= maxSec;
}

}   // namespace fr
#endif
