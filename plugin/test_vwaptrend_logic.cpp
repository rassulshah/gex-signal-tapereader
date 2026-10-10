// test_vwaptrend_logic.cpp -- VwapTrendLogic.h (VW100). No SDK. Build: g++ -std=c++17 -Wall -Wextra test_vwaptrend_logic.cpp
// (and with -DVW_WINMACROS / MinGW + windows.h: the Windows macro names must not break it).
#ifdef VW_WINDOWS_H
#include <windows.h>
#endif
#ifdef VW_WINMACROS
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define near
#define far
#endif
#include "VwapTrendLogic.h"
#include <cstdio>
#include <cmath>
#include <limits>
#include <string>

static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) passes++; else { fails++; printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)

// a session of 3-min ES bars on Friday 2026-10-09: k = 1.. at end 08:30 + 3k, with given close path
static long FRI = svl::daysFromCivil(2026, 10, 9);
static svl::Bar mk(long day, int endMin, float h, float l, float c, double v)
{
    svl::Bar b; b.days = day; b.endSec = endMin * 60; b.h = h; b.l = l; b.c = c; b.v = v; return b;
}

int main()
{
    using namespace vtl;
    int o, c; svl::rthOf("ES", o, c);
    // --- the window rule on hand-made bars
    {
        SBar b[25];
        for (int i = 0; i < 25; i++) { b[i].vw = 100; b[i].sd = 1; b[i].h = 100.5; b[i].l = 99.5; b[i].c = 100.2; }   // 25 closes above, no tags
        Trend t = evalWindow(b, 19);  CHECK(!t.ready && t.state == 0, "fewer than 20 bars: no state");
        t = evalWindow(b, 20);        CHECK(t.ready && t.state == 1 && t.via == VIA_MAJORITY && t.nAbove == 20, "majority above, no -1SD tag = Uptrend (majority)");
        b[22].l = 98.9;                                                          // a -1SD tag
        t = evalWindow(b, 25);        CHECK(t.state == 0 && t.tagDn1, "majority above WITH a -1SD tag = no trend");
        b[24].h = 102.0;                                                         // +2SD touch on the last bar
        t = evalWindow(b, 25);        CHECK(t.state == 1 && t.via == VIA_2SD && t.up2Ago == 0, "+2SD touch = Uptrend via 2SD even with a -1SD tag");
        b[23].l = 97.9;                                                          // -2SD touch one bar earlier
        t = evalWindow(b, 25);        CHECK(t.state == 1 && t.dn2Ago == 1, "both 2SD bands: the latest (+2SD) wins");
        b[24].h = 100.5; b[21].h = 102.5;                                        // now +2SD at 21, -2SD at 23 -> latest is -2SD
        t = evalWindow(b, 25);        CHECK(t.state == -1 && t.via == VIA_2SD, "both 2SD bands: the latest (-2SD) wins");
        b[21].h = 100.5; b[23].h = 102.5; b[23].c = 100.4;                       // both on the same bar, close above VWAP
        t = evalWindow(b, 25);        CHECK(t.state == 1, "both on the same bar: its close above VWAP decides");
        b[23].c = 99.6;               t = evalWindow(b, 25); CHECK(t.state == -1, "both on the same bar: close below VWAP");
        b[23].c = 100.0;              t = evalWindow(b, 25); CHECK(t.state == 0 && t.via == VIA_NONE, "both on the same bar, close at VWAP: no trend");
        // a touch older than 20 bars no longer counts
        for (int i = 0; i < 25; i++) { b[i].h = 100.5; b[i].l = 99.5; b[i].c = 100.2; }
        b[4].h = 103;                 t = evalWindow(b, 25); CHECK(t.via == VIA_MAJORITY, "a +2SD touch 21 bars ago is out of the window");
        b[5].h = 103;                 t = evalWindow(b, 25); CHECK(t.via == VIA_2SD, "a +2SD touch 20 bars ago is in the window");
        // mirror: majority below with no +1SD tag
        for (int i = 0; i < 25; i++) { b[i].h = 100.5; b[i].l = 99.5; b[i].c = 99.8; }
        t = evalWindow(b, 25);        CHECK(t.state == -1 && t.via == VIA_MAJORITY, "majority below, no +1SD tag = Dntrend (majority)");
        b[24].h = 101.0;              t = evalWindow(b, 25); CHECK(t.state == 0, "majority below WITH a +1SD tag = no trend");
        // exactly 10 of 20 above is not a majority
        for (int i = 0; i < 25; i++) { b[i].c = (i % 2) ? 100.2 : 99.8; b[i].h = 100.5; b[i].l = 99.5; }
        t = evalWindow(b, 25);        CHECK(t.state == 0 && t.nAbove == 10 && t.nBelow == 10, "10 of 20 is not a majority");
        // zero / non-finite SD: no band tags
        for (int i = 0; i < 25; i++) { b[i].sd = 0; b[i].h = 105; b[i].l = 95; b[i].c = 100.2; }
        t = evalWindow(b, 25);        CHECK(t.state == 1 && t.via == VIA_MAJORITY && !t.tagDn1, "SD 0: no band touches (majority still counts)");
        for (int i = 0; i < 25; i++) b[i].sd = std::numeric_limits<double>::quiet_NaN();
        t = evalWindow(b, 25);        CHECK(t.via == VIA_MAJORITY, "SD NaN: no band touches");
        t = evalWindow(nullptr, 30);  CHECK(t.state == 0 && !t.ready, "null input");
    }
    // --- from chart bars: sessions, closed bars only, replay == live
    {
        std::vector<svl::Bar> B;
        // overnight: 02:03 .. 08:30 (130 bars), then RTH 08:33 .. 15:00 (130 bars), then 15:03 .. 15:57 (POST)
        int e = 2 * 60 + 3; double px = 100;
        for (; e <= 8 * 60 + 30; e += 3) { B.push_back(mk(FRI, e, (float)px + 0.5f, (float)px - 0.5f, (float)px, 1000)); }
        int firstRth = (int)B.size();
        for (int k = 0; e <= 15 * 60; e += 3, k++) { px += 0.25; B.push_back(mk(FRI, e, (float)px + 0.3f, (float)px - 0.3f, (float)px, 1000)); }
        int firstPost = (int)B.size();
        for (; e < 16 * 60; e += 3) B.push_back(mk(FRI, e, (float)px + 0.3f, (float)px - 0.3f, (float)px, 1000));
        // RTH climbing steadily: every bar above VWAP and riding up -> Uptrend from bar 20
        Trend t = fromChart(B, firstRth + 19, 180, o, c);  CHECK(t.kind == svl::RTH && !t.ready, "19 RTH bars: not ready");
        t = fromChart(B, firstRth + 20, 180, o, c);        CHECK(t.ready && t.state == 1, "a steady climb: Uptrend at the 20th RTH bar");
        t = fromChart(B, firstPost + 5, 180, o, c);        CHECK(t.kind == svl::RTH && t.state == 1 && t.bars == firstPost - firstRth, "after the close: the day's final RTH state");
        t = fromChart(B, firstRth, 180, o, c);             CHECK(t.kind == svl::ON && t.state == 0, "flat overnight: no trend");
        t = fromChart(B, firstRth + 25, 180, o, c, svl::ON); CHECK(t.kind == svl::ON, "an explicit session kind is honoured");
        // replay == live: the state at every closed bar, computed from that bar alone, equals a straight pass over the session
        std::vector<SBar> sb; svl::Acc acc; bool same = true;
        for (int i = firstRth; i < firstPost; i++) {
            acc.add(((double)B[i].h + B[i].l + B[i].c) / 3.0, B[i].v);
            SBar y; y.h = B[i].h; y.l = B[i].l; y.c = B[i].c; y.vw = acc.vw; y.sd = acc.sd(); sb.push_back(y);
            Trend a = evalWindow(&sb[0], (int)sb.size()), b2 = fromChart(B, i + 1, 180, o, c);
            if (a.state != b2.state || a.via != b2.via || a.bars != b2.bars) same = false;
        }
        CHECK(same, "replay bar by bar == one straight pass");
        // a non-finite bar is skipped, a zero-volume bar has no VWAP of its own but keeps the count honest
        std::vector<svl::Bar> B2(B.begin(), B.begin() + firstRth + 30);
        B2[firstRth + 3].h = std::numeric_limits<float>::quiet_NaN();
        t = fromChart(B2, (int)B2.size(), 180, o, c);      CHECK(t.bars == 29, "a non-finite bar is skipped");
        std::vector<svl::Bar> B3(B.begin(), B.begin() + firstRth + 30);
        for (int i = firstRth; i < firstRth + 5; i++) B3[i].v = 0;
        t = fromChart(B3, (int)B3.size(), 180, o, c);      CHECK(t.bars == 25, "no VWAP before the first trade");
        // weekend: Saturday's 'session' does not exist
        std::vector<svl::Bar> W; W.push_back(mk(FRI, 17 * 60 + 3, 1, 1, 1, 1));
        t = fromChart(W, 1, 180, o, c);                    CHECK(t.kind == svl::NONE && t.state == 0, "Friday 17:00 is no session");
        t = fromChart(B, 0, 180, o, c);                    CHECK(t.state == 0, "no closed bars");
        t = fromChart(B, (int)B.size() + 1, 180, o, c);    CHECK(t.state == 0, "nClosed beyond the chart is refused");
        // a falling RTH day: Dntrend; a 1-min chart works too
        std::vector<svl::Bar> D; double q = 100;
        for (int e2 = 8 * 60 + 31; e2 <= 9 * 60 + 30; e2++) { q -= 0.1; D.push_back(mk(FRI, e2, (float)q + 0.05f, (float)q - 0.05f, (float)q, 300)); }
        t = fromChart(D, (int)D.size(), 60, o, c);         CHECK(t.kind == svl::RTH && t.state == -1, "1-min chart, falling: Dntrend");
        CHECK(std::string(label(t)) == "Dntrend" && std::string(viaLabel(t)).size() > 0, "labels");
    }
    // --- volumes above 2^31 (checklist #2)
    {
        std::vector<svl::Bar> B; double px = 100;
        for (int e = 8 * 60 + 33; e <= 10 * 60; e += 3) { px += 0.1; B.push_back(mk(FRI, e, (float)px + 0.1f, (float)px - 0.1f, (float)px, 5e9)); }
        Trend t = fromChart(B, (int)B.size(), 180, o, c); CHECK(t.state == 1, "huge volumes: still Uptrend");
    }
    printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
