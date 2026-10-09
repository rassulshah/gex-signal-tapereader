// Same implementation unit: exercises DealerRead.cpp static helpers without an IRT runtime.
#include <cstdio>
#include <cstring>
#include <limits>

#include "DealerRead.cpp"

static int failures = 0;
static int passes = 0;
#define CHECK(condition, label) do { if (condition) { ++passes; std::printf("  ok    %s\n", label); } else { ++failures; std::printf("  FAIL  %s (line %d)\n", label, __LINE__); } } while (0)

static struct tm localTime(int year, int month, int day, int hour, int minute, int second = 0)
{
    struct tm value;
    std::memset(&value, 0, sizeof(value));
    value.tm_year = year - 1900; value.tm_mon = month - 1; value.tm_mday = day;
    value.tm_hour = hour; value.tm_min = minute; value.tm_sec = second;
    return value;
}

int main()
{
    std::printf("test_dealerread_regression -- same implementation unit\n");

    struct tm now = localTime(2026, 10, 8, 0, 5);
    CHECK(dealerAgeMin(23 * 3600 + 55 * 60, 2026, 10, 7, 0, now) == 10.0, "dated ASOF remains correct across midnight");
    CHECK(dealerAgeMin(23 * 3600 + 55 * 60, 2026, 10, 5, 0, now) == 2890.0, "two-day-old dated ASOF is stale, not apparently fresh");
    CHECK(dealerAgeMin(23 * 3600 + 50 * 60, 2026, 10, 7, 30, localTime(2026, 10, 8, 0, 25)) == 5.0, "clock offset crossing midnight carries into the next date");
    CHECK(dealerAgeMin(1000.0, 2026, 10, 8, 0, localTime(2026, 10, 8, 0, 5)) == 0.0, "future dated ASOF does not become negative age");
    CHECK(dealerAgeMin(23 * 3600 + 59 * 60, 0, 0, 0, 0, now) == 6.0, "undated legacy ASOF retains midnight fallback");

    dl::Row row;
    float meter = 0.0f;
    CHECK(!drMeterValue(row, meter), "missing meter stays unavailable");
    row.hasV = true; row.v = std::numeric_limits<float>::quiet_NaN();
    CHECK(!drMeterValue(row, meter), "NaN meter stays unavailable instead of converting to pixels");
    row.v = 125.0f;
    CHECK(drMeterValue(row, meter) && meter == 100.0f, "positive meter remains bounded");
    row.v = -125.0f;
    CHECK(drMeterValue(row, meter) && meter == -100.0f, "negative meter remains bounded");

    CHECK(to12("00:00 open; 13:05 close") == "12:00 AM open; 1:05 PM close", "time formatter converts valid HH:MM values");
    CHECK(to12("13:05:09 stays seconds") == "13:05:09 stays seconds", "time formatter leaves HH:MM:SS unchanged");

    const std::string statusPath = "DealerRead.status.txt", oldPayload = "STATE,drawn\n", newPayload = "STATE,no data\n";
    CHECK(drShouldWriteStatus(statusPath, oldPayload, "", "", 0, 100), "first status state writes");
    CHECK(!drShouldWriteStatus(statusPath, newPayload, statusPath, oldPayload, 100, 100), "different state is bounded within one second");
    CHECK(drShouldWriteStatus(statusPath, newPayload, statusPath, oldPayload, 100, 101), "different state writes after throttle interval");
    CHECK(!drShouldWriteStatus(statusPath, oldPayload, statusPath, oldPayload, 100, 159), "unchanged state skips repaint writes");
    CHECK(drShouldWriteStatus(statusPath, oldPayload, statusPath, oldPayload, 100, 160), "unchanged state retains sixty-second heartbeat");

    std::printf("\n%d passed, %d failed\n", passes, failures);
    return failures;
}
