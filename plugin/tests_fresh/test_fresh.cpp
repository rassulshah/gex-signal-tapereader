// test_fresh.cpp -- Fresh.h 1.1.0 (2026-10-28; 1.0.0 2026-10-18), the one freshness / STALE rule. SDK-free.
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_fresh.cpp -o tf && ./tf   (from plugin\tests_fresh)
#include "Fresh.h"
#include <cstdio>
#include <string>
static int n = 0, f = 0;
#define CHECK(c, m) do { ++n; if (!(c)) { ++f; std::printf("FAIL line %d: %s\n", __LINE__, std::string(m).c_str()); } } while (0)
static long long at(int y, int mo, int d, int h, int mi) { return fr::ctAbs(y, mo, d, h * 3600 + mi * 60); }
int main()
{
    // calendar: 2026-10-16 = Friday, 10-18 = Sunday, 10-19 = Monday
    CHECK(fr::weekday(fr::civilDays(2026, 10, 16)) == 5 && fr::weekday(fr::civilDays(2026, 10, 18)) == 0 && fr::weekday(fr::civilDays(1970, 1, 1)) == 4, "weekdays");
    long long dd = 0; CHECK(fr::parseYmd("2026-10-16", dd) && dd == fr::civilDays(2026, 10, 16), "parseYmd");
    CHECK(!fr::parseYmd("2026-13-01", dd) && !fr::parseYmd("2026-1-01", dd) && !fr::parseYmd("abcd-ef-gh", dd) && !fr::parseYmd("", dd), "parseYmd rejects garbage");
    // the update window
    CHECK(fr::updateHours(1, 9 * 60) && fr::updateHours(1, 2 * 60) && fr::updateHours(0, 22 * 60) && fr::updateHours(5, 15 * 60), "in hours: Mon 9:00, Mon 2:00, Sun 22:00, Fri 15:00");
    CHECK(!fr::updateHours(1, 17 * 60) && !fr::updateHours(6, 12 * 60) && !fr::updateHours(0, 18 * 60) && !fr::updateHours(5, 16 * 60 + 1), "paused: Mon 17:00, Sat, Sun 18:00, Fri 16:01");
    CHECK(fr::lastUpdateEnd(at(2026, 10, 18, 0, 10)) == at(2026, 10, 16, 16, 0), "weekend: last close Fri 16:00");
    CHECK(fr::lastUpdateEnd(at(2026, 10, 19, 18, 0)) == at(2026, 10, 19, 16, 0), "Mon evening: Mon 16:00");
    CHECK(fr::lastUpdateEnd(at(2026, 10, 20, 9, 0)) == at(2026, 10, 19, 16, 0), "Tue morning: Mon 16:00");
    // intraday rule in hours
    const long long mon10 = at(2026, 10, 19, 10, 0);
    fr::Verdict v = fr::judge(fr::K_MQ_LEVELS, mon10 - 19 * 60, mon10);
    CHECK(v.st == fr::LIVE && v.tag.empty() && v.ageSec == 19 * 60, "19 min old in hours = LIVE");
    v = fr::judge(fr::K_MQ_LEVELS, mon10 - 20 * 60, mon10); CHECK(v.st == fr::LIVE, "20 min = LIVE (the edge)");
    v = fr::judge(fr::K_MQ_LEVELS, mon10 - 21 * 60, mon10); CHECK(v.st == fr::STALE && v.tag == "STALE 21m", "21 min in hours = STALE 21m: " + v.tag);
    v = fr::judge(fr::K_SESSION, mon10 - 5 * 60, mon10, true); CHECK(v.st == fr::STALE, "a STOPPED feed in hours = STALE at any age");
    v = fr::judge(fr::K_DEALER, at(2026, 10, 16, 16, 2), mon10); CHECK(v.st == fr::STALE && v.tag == "STALE 3d", "Friday's close on Monday 10:00 = STALE: " + v.tag);
    v = fr::judge(fr::K_GEX, at(2026, 10, 16, 16, 2), at(2026, 10, 18, 21, 50)); CHECK(v.st == fr::STALE && v.tag == "STALE 2d", "Sun 21:50 with Fri's close set = STALE (MenthorQ updates again)");
    // outside hours: the closing set is PAUSED, an older one STALE
    const long long sat = at(2026, 10, 17, 23, 50);
    v = fr::judge(fr::K_SESSION, at(2026, 10, 16, 16, 5), sat); CHECK(v.st == fr::PAUSED && v.tag == "PAUSED" && v.usable(), "the audit case: Fri 16:05 data on Sat night = PAUSED (closing set), not live");
    v = fr::judge(fr::K_SESSION, at(2026, 10, 16, 15, 41), sat); CHECK(v.st == fr::PAUSED, "15:41 = within 20 min of the 16:00 close");
    v = fr::judge(fr::K_SESSION, at(2026, 10, 16, 15, 39), sat); CHECK(v.st == fr::STALE && v.stale(), "15:39 = the feed had stopped before the close: STALE");
    v = fr::judge(fr::K_SESSION, at(2026, 10, 15, 16, 0), sat); CHECK(v.st == fr::STALE, "Thursday's set on Saturday = STALE");
    v = fr::judge(fr::K_MQ_LEVELS, at(2026, 10, 19, 15, 55), at(2026, 10, 19, 18, 0)); CHECK(v.st == fr::PAUSED, "Mon 18:00 with the 15:55 set = PAUSED");
    v = fr::judge(fr::K_MQ_LEVELS, at(2026, 10, 19, 11, 0), at(2026, 10, 19, 18, 0)); CHECK(v.st == fr::STALE, "Mon 18:00 with an 11:00 set = STALE");
    // unknown / future
    CHECK(fr::judge(fr::K_DEALER, 0, mon10).tag == "STALE ?" && fr::judge(fr::K_DEALER, -5, mon10).stale(), "no time = STALE ?");
    CHECK(fr::judge(fr::K_DEALER, mon10 + 121, mon10).stale() && fr::judge(fr::K_DEALER, mon10 + 60, mon10).st == fr::LIVE, "future > 120 s = STALE; small clock skew = LIVE");
    // daily (Q-Scores)
    const long long fri = fr::civilDays(2026, 10, 16), thu = fr::civilDays(2026, 10, 15), mon = fr::civilDays(2026, 10, 19);
    CHECK(fr::judgeDaily(fri, sat).st == fr::LIVE, "Fri scores on Saturday = LIVE");
    CHECK(fr::judgeDaily(fri, at(2026, 10, 19, 9, 0)).st == fr::LIVE, "Fri scores on Monday 9:00 = LIVE");
    CHECK(fr::judgeDaily(fri, at(2026, 10, 19, 21, 59)).st == fr::LIVE, "Monday's scores not yet due at 21:59");
    CHECK(fr::judgeDaily(fri, at(2026, 10, 19, 22, 0)).stale(), "Monday 22:00: Monday's scores due -> Friday's STALE");
    CHECK(fr::judgeDaily(mon, at(2026, 10, 19, 22, 0)).st == fr::LIVE, "Monday's scores at 22:00 = LIVE");
    CHECK(fr::judgeDaily(thu, sat).stale() && fr::judgeDaily(thu, sat).tag.compare(0, 6, "STALE ") == 0, "Thursday's on Saturday = STALE");
    CHECK(fr::judgeDaily(-1, sat).stale() && fr::judgeDaily(mon, sat).stale(), "no date / a future date = STALE");
    CHECK(fr::requiredDay(at(2026, 10, 18, 0, 10)) == fri, "requiredDay on Sunday = Friday");
    // the status file of another plugin
    CHECK(fr::fileFresh(1000, 1100) && fr::fileFresh(1000, 1120) && !fr::fileFresh(1000, 1121) && !fr::fileFresh(0, 1100) && !fr::fileFresh(1300, 1100), "fileFresh 120 s");
    // clock offset and conversion
    const long long u = 1792300203;   // Sun 2026-10-18 00:10:03 CDT (05:10 UTC)
    CHECK(fr::offsetFromClock(u, 10) == -5 * 3600, "CDT offset -5 h");
    CHECK(fr::offsetFromClock(u, 10 + 1440 - 1440) == -18000 && fr::offsetFromClock(1792300203 + 59, 10) == -18000, "offset stable within the minute");
    CHECK(fr::ctFromUnix(u, -18000) == at(2026, 10, 18, 0, 10) + 3, "ctFromUnix");
    CHECK(fr::offsetFromClock(1792300203, 23 * 60 + 10) == -6 * 3600, "previous CT day (CST -6 h) wraps correctly");
    CHECK(fr::ageShort(25 * 60) == "25m" && fr::ageShort(3 * 3600) == "3h" && fr::ageShort(3 * 86400) == "3d" && fr::ageShort(-1) == "?", "ageShort");
    // every intraday kind follows the same rule (consistency across indicators)
    for (int k = fr::K_MQ_LEVELS; k <= fr::K_GEX; ++k) { if (k == fr::K_QSCORE) continue;
        CHECK(fr::judge((fr::Kind)k, mon10 - 25 * 60, mon10).stale() && fr::judge((fr::Kind)k, mon10 - 10 * 60, mon10).st == fr::LIVE, "kind " + std::to_string(k) + " same thresholds"); }
    // (1.1.0) the overnight limit: 45 min from 16:00 to 08:30 CT, 20 min in RTH
    const long long tue2245 = at(2026, 10, 27, 22, 45), tue0820 = at(2026, 10, 27, 8, 20), tue0835 = at(2026, 10, 27, 8, 35);
    CHECK(fr::liveLimitMinAt(mon10) == 20 && fr::liveLimitMinAt(tue2245) == 45 && fr::liveLimitMinAt(tue0820) == 45 && fr::liveLimitMinAt(tue0835) == 20, "limit: RTH 20, overnight 45");
    CHECK(fr::liveLimitMinAt(at(2026, 10, 19, 15, 59)) == 20 && fr::liveLimitMinAt(at(2026, 10, 19, 16, 0)) == 45, "16:00 switches to 45");
    CHECK(fr::liveLimitMinAt(at(2026, 10, 18, 22, 0)) == 45, "Sunday evening = 45");
    v = fr::judge(fr::K_MQ_LEVELS, tue2245 - 25 * 60, tue2245); CHECK(v.st == fr::LIVE, "the 10-27 case: 22:45 with a 22:20 as-of = LIVE now (was STALE 25m)");
    v = fr::judge(fr::K_MQ_LEVELS, tue2245 - 46 * 60, tue2245); CHECK(v.st == fr::STALE && v.tag == "STALE 46m", "46 min overnight = STALE: " + v.tag);
    v = fr::judge(fr::K_MQ_LEVELS, mon10 - 25 * 60, mon10); CHECK(v.stale(), "RTH still 20 min");
    // (1.1.0) judgeArrived: by arrival, as-of for display
    v = fr::judgeArrived(fr::K_MQ_LEVELS, tue2245 - 40 * 60, tue2245 - 3 * 60, tue2245);
    CHECK(v.st == fr::LIVE && v.ageSec == 40 * 60 && v.tag.empty(), "arrived 3 min ago with a 40-min as-of overnight = LIVE, age shows the as-of");
    v = fr::judgeArrived(fr::K_MQ_LEVELS, tue2245 - 50 * 60, tue2245 - 46 * 60, tue2245);
    CHECK(v.st == fr::STALE && v.tag == "STALE 46m", "nothing arrived for 46 min overnight = STALE 46m: " + v.tag);
    v = fr::judgeArrived(fr::K_MQ_LEVELS, mon10 - 18 * 60, mon10 - 2 * 60, mon10); CHECK(v.st == fr::LIVE, "RTH: arrived 2 min ago, as-of 18 = LIVE");
    v = fr::judgeArrived(fr::K_MQ_LEVELS, mon10 - 30 * 60, mon10 - 21 * 60, mon10); CHECK(v.stale() && v.tag == "STALE 21m", "RTH: nothing for 21 min = STALE 21m: " + v.tag);
    v = fr::judgeArrived(fr::K_MQ_LEVELS, mon10 - 70 * 60, mon10 - 1 * 60, mon10); CHECK(v.stale() && v.tag == "STALE 70m", "RTH: a file re-written with a 70-min-old set is not live (as-of > 20 + 45): " + v.tag);
    v = fr::judgeArrived(fr::K_MQ_LEVELS, mon10 - 60 * 60, mon10 - 1 * 60, mon10); CHECK(v.st == fr::LIVE, "RTH: as-of 60 min with a fresh arrival = within 20 + 45 (as-of lag allowance)");
    v = fr::judgeArrived(fr::K_GEX, mon10 - 5 * 60, mon10 - 1 * 60, mon10, true); CHECK(v.stale(), "a STOPPED feed = STALE even if a file just arrived");
    v = fr::judgeArrived(fr::K_GEX, mon10 - 25 * 60, 0, mon10); CHECK(v.stale() && v.tag == "STALE 25m", "unknown arrival = the old as-of rule");
    v = fr::judgeArrived(fr::K_GEX, mon10 - 5 * 60, mon10 + 600, mon10); CHECK(v.stale() && !v.known, "arrival in the future = STALE ?");
    v = fr::judgeArrived(fr::K_SESSION, at(2026, 10, 16, 16, 5), at(2026, 10, 16, 16, 6), sat); CHECK(v.st == fr::PAUSED, "outside hours: the closing set stays PAUSED");
    v = fr::judgeArrived(fr::K_SESSION, at(2026, 10, 15, 16, 0), sat - 60, sat); CHECK(v.stale(), "outside hours: Thursday's set re-written Saturday = STALE (as-of rule)");
    std::printf("Fresh.h %s: %d checks, %d failures\n", fr::VERSION, n, f);
    return f ? 1 : 0;
}
