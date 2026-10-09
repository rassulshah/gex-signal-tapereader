// Focused audit regressions for TapeFlowLogic.h. These use fabricated data only.
#include "TapeFlowSupport.h"
#include "TapeFlowLogic.h"
#include <cmath>
#include <cstdio>
#include <string>

using namespace tfl;

static int failures = 0;
static int passes = 0;
#define CHECK(condition, message) do { if (condition) ++passes; else { ++failures; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, message); } } while (0)

static std::string replaceField(std::string line, int field, const std::string& replacement)
{
    size_t begin = 2; // first field follows W|
    for (int i = 0; i < field; ++i) {
        begin = line.find('|', begin);
        if (begin == std::string::npos) return line;
        ++begin;
    }
    size_t end = line.find('|', begin);
    line.replace(begin, end == std::string::npos ? std::string::npos : end - begin, replacement);
    return line;
}

int main()
{
    // Regression: Windows long is 32-bit. Dense selection must not narrow a large
    // long long quantity while selecting a price or a band.
    {
        // (1.1.x) Dense keeps cached prefix sums, so it is filled through build() from 1-s slices
        std::deque<Slice> R(1); R[0].rows = {Row{100, 7, 0, 0}, Row{101, 4294967313LL, 0, 0}, Row{102, 11, 0, 0}};
        Dense d; d.build(R, 0, 1);
        int center = 0; long long volume = 0; double coverage = 0;
        CHECK(d.best(SIDE_BUY, 1, 0.90, INT_MIN, &center, &volume, &coverage), "large known-volume band is eligible");   // (1.1.x) half-width 1..8 only
        CHECK(center == 101 && volume == 4294967331LL && coverage == 1.0, "Dense::best preserves quantities above LONG_MAX on Windows");
    }

    // Regression: flow/rate accumulators must remain precise beyond signed 32-bit.
    {
        Engine e;
        BinBase base;
        base.n = 100; base.med20 = 2000000000.0f; base.med5 = 2000000000.0f; base.medSpread = 1;
        e.setFixedBase(base);
        const long long start = 1790000000LL - (1790000000LL % 86400) + 9 * 3600;
        for (int second = 0; second < 250; ++second) {
            Tick k; k.t = start + second; k.px = 1001; k.bid = 1000; k.ask = 1001; k.q = 2000000000LL;
            e.add(k);
        }
        e.advanceTo(start + 249);
        CHECK(e.last.f30ok && std::fabs(e.last.f30 - 100.0) < 1e-9, "large classified flow remains finite and directional");
        CHECK(std::fabs(e.last.rate20 - 2000000000.0) < 0.5, "large 20-second rate does not overflow a Windows long");
    }

    // Regression: malformed persisted baseline rows cannot silently become zero,
    // infinity, NaN, an out-of-range bin, or a negative volume.
    {
        Win source;
        source.bin = 7; source.rate20 = 12.5f; source.r5[0] = 10.0f; source.r5[1] = 11.0f;
        source.r5[2] = 12.0f; source.r5[3] = 13.0f; source.spread = 1.0f;
        for (int h = 0; h < 8; ++h) { source.Mb[h] = (float)(h + 1); source.Ms[h] = (float)(h + 11); }
        const std::string row = storeLine(1234, "ESZ6", source);
        long long sid = 0; std::string symbol; Win parsed;
        CHECK(parseStoreLine(row, &sid, &symbol, &parsed) && sid == 1234 && symbol == "ESZ6" && parsed.Ms[7] == 18.0f, "valid baseline row still round-trips");
        CHECK(!parseStoreLine(replaceField(row, 0, "9223372036854775808"), &sid, &symbol, &parsed), "session integer overflow is rejected");
        CHECK(!parseStoreLine(replaceField(row, 2, "2147483648"), &sid, &symbol, &parsed), "bin integer overflow is rejected");
        CHECK(!parseStoreLine(replaceField(row, 3, "nan"), &sid, &symbol, &parsed), "NaN rate is rejected");
        CHECK(!parseStoreLine(replaceField(row, 3, "3.5e39"), &sid, &symbol, &parsed), "out-of-range float is rejected");
        CHECK(!parseStoreLine(replaceField(row, 4, "-1"), &sid, &symbol, &parsed), "negative rate component is rejected");
        CHECK(!parseStoreLine(replaceField(row, 8, "-2"), &sid, &symbol, &parsed), "invalid negative spread is rejected");
        CHECK(!parseStoreLine(replaceField(row, 9, "-1"), &sid, &symbol, &parsed), "negative baseline volume is rejected");
        CHECK(!parseStoreLine(replaceField(row, 1, ""), &sid, &symbol, &parsed), "empty symbol is rejected");
    }

    {   // (1.1.4) a back-fill out of time order is put in order (not rejected), the live cursor still refuses it
        using tf_support::TradeIdentity;
        std::vector<TradeIdentity> v = { {100, 100.0, 4, 3, 4, 1}, {102, 102.0, 5, 4, 5, 2}, {101, 101.5, 4, 3, 4, 3}, {101, 101.2, 4, 3, 4, 4}, {103, 103.0, 6, 5, 6, 5} };
        tf_support::SnapshotCursor live; std::size_t b0 = 0; std::string e0;
        CHECK(!live.plan(v, &b0, &e0), "the live cursor still rejects an out-of-order snapshot");
        std::size_t moved = tf_support::sortByTime(v);
        CHECK(moved == 2, "two records were out of order");
        CHECK(v[1].quantity == 4 && v[2].quantity == 3 && v[3].quantity == 2, "sorted by second, then raw time, stable");
        tf_support::SnapshotCursor c; std::size_t b = 9; std::string e;
        CHECK(c.plan(v, &b, &e) && b == 0, "a sorted back-fill plans from the start");
        long long q = 0; for (const auto& r : v) q += r.quantity;
        CHECK(q == 15, "no trade lost or duplicated by the sort");
        std::vector<TradeIdentity> w = v; CHECK(tf_support::sortByTime(w) == 0, "an ordered snapshot is left as it is");
    }

    std::printf("%d passed, %d failed\n", passes, failures);
    return failures ? 1 : 0;
}
