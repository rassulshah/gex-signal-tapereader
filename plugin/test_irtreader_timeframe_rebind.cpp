// Regression: a chart period change must not reset DBO arrival ordering for the same market/contract.
#include "IRTReaderLogic.h"
#include "ArrivalStamp.h"
#include <cstdio>

static int passes = 0, fails = 0;
#define CHECK(c, m) do { if (c) { ++passes; std::printf("  ok    %s\n", m); } else { ++fails; std::printf("  FAIL  %s\n", m); } } while (0)

static bool applyAfterRebind(arr::Clock& clock, const char* oldMarket, const char* oldSymbol,
                             const char* newMarket, const char* newSymbol,
                             int& y, int& mo, int& d, int& h, int& mi, int& s)
{
    if (!irl::sameMboStream(oldMarket, oldSymbol, newMarket, newSymbol)) clock.reset();
    return clock.apply(y, mo, d, h, mi, s);
}

int main()
{
    CHECK(irl::sameMboStream("ES", "EPZ26", "ES", "EPZ26"), "the same market and contract retain DBO arrival ordering");
    CHECK(!irl::sameMboStream("ES", "EPZ26", "ES", "EPH27"), "a contract roll starts a distinct DBO stream");
    CHECK(!irl::sameMboStream("", "EPZ26", "ES", "EPZ26"), "an uninitialized stream cannot retain arrival ordering");

    arr::Clock clock;
    int y = 2026, mo = 10, d = 22, h = 10, mi = 15, s = 0;
    (void)clock.apply(y, mo, d, h, mi, s);  // latest row written before a chart timeframe change
    y = 2026; mo = 10; d = 22; h = 10; mi = 10; s = 0;  // stale timestamp resent after re-subscription
    const bool restamped = applyAfterRebind(clock, "ES", "EPZ26", "ES", "EPZ26", y, mo, d, h, mi, s);
    CHECK(restamped && h == 10 && mi == 15 && s == 0,
          "a period-only rebind keeps a resent stale order at the latest written arrival time");

    y = 2026; mo = 10; d = 22; h = 10; mi = 10; s = 0;
    const bool crossContractRestamped = applyAfterRebind(clock, "ES", "EPZ26", "ES", "EPH27", y, mo, d, h, mi, s);
    CHECK(!crossContractRestamped && h == 10 && mi == 10 && s == 0,
          "a changed contract resets arrival order rather than leaking the old stream into it");

    std::printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
