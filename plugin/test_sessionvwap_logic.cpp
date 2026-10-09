// lsSessionVWAP 1.2.0 session rules (SessionVWAPLogic.h) - the day and the overnight for every market he trades.
#include "SessionVWAPLogic.h"
#include <cstdio>
#include <cmath>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) passes++; else { fails++; printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
int main()
{
    using namespace svl;
    long thu = daysFromCivil(2026, 10, 8), fri = thu + 1, sun = thu + 3, mon = thu + 4;
    CHECK(weekdayOf(thu) == 4 && weekdayOf(sun) == 0 && weekdayOf(mon) == 1, "weekday arithmetic (2026-10-08 = Thursday)");
    CHECK(daysFromCivil(2026, 3, 1) - daysFromCivil(2026, 2, 28) == 1 && daysFromCivil(2028, 3, 1) - daysFromCivil(2028, 2, 28) == 2, "month ends / leap years");
    int o, c;
    const char* M[7] = { "ES", "NQ", "CL", "NG", "GC", "HG", "EU" };
    int wantO[7] = { 510, 510, 480, 480, 440, 440, 440 }, wantC[7] = { 900, 900, 810, 810, 750, 750, 840 };
    for (int i = 0; i < 7; i++) CHECK(rthOf(M[i], o, c) && o == wantO[i] && c == wantC[i], M[i]);
    CHECK(!rthOf("XX", o, c) && !rthOf("E", o, c) && !rthOf("ESX", o, c), "unknown market refused");
    // ES, 3-min bars stamped at their END
    rthOf("ES", o, c);
    Cls a = classify(thu, 17 * 3600 + 180, 180, o, c);  CHECK(a.kind == ON && a.sess == fri, "17:00-17:03 Thu = Friday's overnight");
    a = classify(fri, 0, 180, o, c);                     CHECK(a.kind == ON && a.sess == fri, "a bar closing at 00:00 belongs to the session that began the evening before");
    a = classify(fri, 3 * 3600, 180, o, c);              CHECK(a.kind == ON && a.sess == fri, "03:00 = overnight");
    a = classify(fri, 8 * 3600 + 30 * 60, 180, o, c);    CHECK(a.kind == ON, "08:27-08:30 is still overnight");
    a = classify(fri, 8 * 3600 + 33 * 60, 180, o, c);    CHECK(a.kind == RTH && a.sess == fri, "08:30-08:33 = first RTH bar");
    a = classify(fri, 8 * 3600 + 31 * 60, 60, o, c);     CHECK(a.kind == RTH, "1-min chart: 08:30-08:31 is RTH");
    a = classify(fri, 15 * 3600, 180, o, c);             CHECK(a.kind == RTH, "14:57-15:00 = last RTH bar");
    a = classify(fri, 15 * 3600 + 180, 180, o, c);       CHECK(a.kind == POST && a.sess == fri, "15:00-15:03 = closed, RTH frozen");
    a = classify(fri, 15 * 3600 + 90, 180, o, c);        CHECK(a.kind == POST, "a bar straddling the close is not RTH");
    a = classify(fri, 17 * 3600 + 180, 180, o, c);       CHECK(a.kind == NONE, "Friday 17:00 opens a Saturday session: none");
    a = classify(sun, 17 * 3600 + 180, 180, o, c);       CHECK(a.kind == ON && a.sess == mon, "Sunday 17:00 = Monday's overnight");
    a = classify(sun, 12 * 3600, 180, o, c);             CHECK(a.kind == NONE, "no Sunday daytime session");
    // GC / HG / 6E 07:20: 3-min bars from 17:00 run 07:18-07:21, 07:21-07:24
    rthOf("GC", o, c);
    a = classify(fri, 7 * 3600 + 21 * 60, 180, o, c);    CHECK(a.kind == ON, "GC 07:18-07:21 straddles the open: overnight");
    a = classify(fri, 7 * 3600 + 24 * 60, 180, o, c);    CHECK(a.kind == RTH, "GC 07:21-07:24 = first RTH bar (lra.markets)");
    a = classify(fri, 7 * 3600 + 21 * 60, 60, o, c);     CHECK(a.kind == RTH, "GC 1-min 07:20-07:21 = RTH");
    a = classify(fri, 12 * 3600 + 33 * 60, 180, o, c);   CHECK(a.kind == POST, "GC after 12:30 frozen");
    rthOf("CL", o, c);
    a = classify(fri, 8 * 3600 + 3 * 60, 180, o, c);     CHECK(a.kind == RTH, "CL 08:00-08:03 RTH");
    a = classify(fri, 13 * 3600 + 33 * 60, 180, o, c);   CHECK(a.kind == POST, "CL after 13:30 frozen");
    // the arithmetic: two sessions, a zero-volume bar, freeze, pRTH
    rthOf("ES", o, c);
    std::vector<Bar> B;
    auto add = [&](long d, int h, int m, float p, double v) { Bar b; b.days = d; b.endSec = h * 3600 + m * 60; b.h = b.l = b.c = p; b.v = v; B.push_back(b); };
    add(thu, 8, 33, 100, 100); add(thu, 8, 36, 102, 100); add(thu, 15, 3, 999, 50);           // RTH 100/102 -> 101, SD 1; post frozen
    add(thu, 17, 3, 200, 10); add(thu, 17, 6, 0, 0); add(fri, 1, 0, 204, 30);              // O/N: 200, then (200*10 + 204*30)/40 = 203
    add(fri, 8, 33, 300, 5);
    std::vector<float> out; Result R = run(B, 180, o, c, out);
    CHECK(R.nRth == 3 && R.nOn == 3, "bar counts");
    CHECK(out[1 * 11 + 0] == 101.0f && out[1 * 11 + 1] == 102.0f && out[1 * 11 + 4] == 99.0f, "RTH VWAP 101, +1 = 102, -2 = 99");
    CHECK(out[2 * 11 + 0] == 101.0f && out[2 * 11 + 3] == 103.0f, "after the close: frozen at the final values, the 999 print ignored");
    CHECK(out[3 * 11 + 5] == 200.0f && out[3 * 11 + 10] == 101.0f && out[3 * 11 + 0] == 0.0f, "O/N starts at 17:00, pRTH = 101, RTH lines off");
    CHECK(out[4 * 11 + 5] == 200.0f, "zero-volume bar carries the O/N VWAP");
    CHECK(out[5 * 11 + 5] == 203.0f && out[5 * 11 + 10] == 101.0f, "O/N VWAP across midnight = 203");
    CHECK(out[6 * 11 + 0] == 300.0f && out[6 * 11 + 5] == 0.0f && out[6 * 11 + 10] == 0.0f, "RTH restarts at the open; O/N and pRTH lines end");
    // (1.3.0) overnight odds = lra.on_vwap_study p_touch (V = sigma^2 x 60)
    CHECK(std::fabs(onTouch(5, 0.6) - 0.26289409668390906) < 1e-9, "onTouch(5, 0.6) matches the study");
    CHECK(std::fabs(onTouch(-12, 0.6) - 0.026167324043041607) < 1e-9, "onTouch(-12, 0.6) matches the study");
    CHECK(std::fabs(onTouch(0.5, 0.2) - 0.7178343256057438) < 1e-9, "onTouch(0.5, 0.2) matches the study");
    CHECK(std::fabs(onTouch(30, 1.5) - 0.0261673240430416) < 1e-9, "onTouch(30, 1.5) matches the study");
    { double c[21]; for (int i = 0; i < 21; i++) c[i] = 100 + ((i % 2) ? 0.5 : -0.5);
      CHECK(std::fabs(onSigmaMin(c, 21, 180) - 1.0 / std::sqrt(3.0)) < 1e-9, "sigma per minute from 20 three-minute closes");
      CHECK(onSigmaMin(c, 10, 180) == 0.0, "too few closes: no odds");
      c[20] = NAN; CHECK(onSigmaMin(c, 21, 180) == 0.0, "a bad close: no odds"); }
    CHECK(onTouch(0.0, 0.0) == 1.0 && onTouch(1.0, 0.0) == 0.0 && onTouch(NAN, 1.0) < 0, "edge cases");
    printf("%d passed, %d failed\n", passes, fails);
    return fails;
}
