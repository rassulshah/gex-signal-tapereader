#include "irtsdk.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace mock { Host* current = NULL; }

#ifndef SESSIONVWAP_SOURCE
#error SESSIONVWAP_SOURCE must name the implementation under test
#endif
#include SESSIONVWAP_SOURCE

static int failures = 0;
#define CHECK(condition, text) do { if (!(condition)) { ++failures; std::cerr << "FAIL: " << text << "\n"; } } while (0)

static RTDATE utc(int y, int mo, int d, int h, int mi, int s = 0)
{
    struct tm t = {};
    t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d;
    t.tm_hour = h; t.tm_min = mi; t.tm_sec = s; t.tm_isdst = 0;
    return (RTDATE)timegm(&t);
}

static void sizeHost(mock::Host& H, int n)
{
    for (int a = 0; a < 8; ++a) H.f[a].assign((size_t)n, 0.0f);
    for (int a = 0; a < 2; ++a) H.i[a].assign((size_t)n, 0UL);
}

static void setBar(mock::Host& H, int i, RTDATE end, float tp, unsigned long vol)
{
    H.i[(int)barDateTime][(size_t)i] = end;
    H.i[(int)barVolume][(size_t)i] = vol;
    H.f[(int)barHigh][(size_t)i] = tp;
    H.f[(int)barLow][(size_t)i] = tp;
    H.f[(int)barClose][(size_t)i] = tp;
}

static void testProbability(void)
{
    CHECK(pTouch(-10.0, 0.0, 0.3, 1.0) == 0.0, "zero variance must not report a lower, unreached level as touched");
    CHECK(pTouch(0.0, 0.0, 0.3, 1.0) == 1.0, "zero variance reports the current level as touched");
    CHECK(pTouch(10.0, 0.0, 0.3, 1.0) == 0.0, "zero variance must not report an upper, unreached level as touched");
    const double up = pTouch(2.0, 4.0, 0.3, 1.0), down = pTouch(-2.0, 4.0, 0.3, 1.0);
    CHECK(std::isfinite(up) && up >= 0.0 && up <= 1.0, "normal touch probability is finite and bounded");
    CHECK(std::fabs(up - down) < 1e-12, "touch probability is symmetric around the current price");
    CHECK(!std::isfinite(pTouch(1.0, std::numeric_limits<double>::quiet_NaN(), 0.3, 1.0)), "invalid forecast input is not converted to a fabricated percentage");
}

static void testVWAPAndRth(void)
{
    mock::Host H; H.root = "ES"; sizeHost(H, 4);
    // A 90-second bar closing 08:31 starts at 08:29:30 and must be excluded.
    setBar(H, 0, utc(2026, 10, 5, 8, 31, 0), 999.0f, 100);
    setBar(H, 1, utc(2026, 10, 5, 8, 32, 30), 100.0f, 100);
    setBar(H, 2, utc(2026, 10, 5, 8, 34, 0), 200.0f, 0);
    setBar(H, 3, utc(2026, 10, 5, 8, 35, 30), 102.0f, 100);
    mock::current = &H;
    SessionVWAP ext;
    ext.fill(0);
    CHECK(H.f[(int)fOut1][0] == 0.0f, "bar beginning before RTH open is excluded using seconds, not truncated minutes");
    CHECK(H.f[(int)fOut1][1] == 100.0f, "first traded RTH bar establishes VWAP");
    CHECK(H.f[(int)fOut1][2] == 100.0f, "zero-volume bar carries established VWAP rather than its typical price");
    CHECK(H.f[(int)fOut1][3] == 101.0f, "volume-weighted mean is correct after a second trade");
    CHECK(H.f[(int)fOut2][3] == 102.0f && H.f[(int)fOut3][3] == 100.0f, "weighted one-standard-deviation bands are correct");
    ext.done();
    CHECK(H.userData == NULL, "done deletes current-host state and clears the SDK pointer");
}

static void testNoVolumeAndStableVariance(void)
{
    mock::Host noVol; noVol.root = "ES"; sizeHost(noVol, 2);
    setBar(noVol, 0, utc(2026, 10, 5, 8, 33), 200.0f, 0);
    setBar(noVol, 1, utc(2026, 10, 5, 8, 36), 100.0f, 100);
    mock::current = &noVol;
    SessionVWAP ext;
    ext.fill(0);
    CHECK(noVol.f[(int)fOut1][0] == 0.0f, "VWAP remains unavailable before the first positive-volume trade");
    CHECK(noVol.f[(int)fOut1][1] == 100.0f, "VWAP starts on the first positive-volume trade");
    ext.done();

    mock::Host stable; stable.root = "ES"; sizeHost(stable, 130);
    for (int i = 0; i < 130; ++i) {
        // 3-minute RTH bars: 08:33 through 15:00.  Large volumes stress the
        // cancellation-prone second-moment formula while expected SD is exactly 1.
        setBar(stable, i, utc(2026, 10, 5, 8, 33 + 3 * i), 10000000.0f + (i & 1 ? 1.0f : -1.0f), 4000000000UL);
    }
    mock::current = &stable;
    ext.fill(0);
    CHECK(stable.f[(int)fOut1][129] == 10000000.0f, "large-volume VWAP remains centered");
    CHECK(stable.f[(int)fOut2][129] == 10000001.0f && stable.f[(int)fOut3][129] == 9999999.0f, "stable weighted variance preserves the known one-point SD");
    ext.done();
}

static void testCurrentHostStateIsolation(void)
{
    mock::Host A, B; A.root = "ES"; B.root = "NQ";
    sizeHost(A, 2); sizeHost(B, 2);
    setBar(A, 0, utc(2026, 10, 5, 8, 33), 100.0f, 1); setBar(A, 1, utc(2026, 10, 5, 8, 36), 101.0f, 1);
    setBar(B, 0, utc(2026, 10, 5, 8, 33), 200.0f, 1); setBar(B, 1, utc(2026, 10, 5, 8, 36), 201.0f, 1);
    SessionVWAP ext;
    mock::current = &A; ext.fill(0); void* a = A.userData;
    mock::current = &B; ext.fill(0); void* b = B.userData;
    CHECK(a != NULL && b != NULL && a != b, "mutable state is allocated independently for each SDK host/chart");
    mock::current = &A; ext.done();
    CHECK(A.userData == NULL && B.userData == b, "done only releases the current chart's state");
    mock::current = &B; ext.destroy();
    CHECK(B.userData == NULL, "destroy releases the remaining current chart state");
}

int main()
{
    testProbability();
    testVWAPAndRth();
    testNoVolumeAndStableVariance();
    testCurrentHostStateIsolation();
    if (failures) { std::cerr << failures << " regression assertion(s) failed\n"; return 1; }
    std::cout << "SessionVWAP regressions: 19 assertions passed\n";
    return 0;
}
