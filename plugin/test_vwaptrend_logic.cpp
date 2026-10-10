// test_vwaptrend_logic.cpp -- VwapTrendLogic.h (VW100 + the STRONG level, VW110). No SDK. Build: g++ -std=c++17 -Wall -Wextra test_vwaptrend_logic.cpp
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
#include <cstdint>

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
    int o = 0, c = 0; svl::rthOf("ES", o, c);
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

    // ================= VW110: the STRONG level =================
    // a window: VWAP 100, SD 1 on every bar; 'na1' closes at 101.2 (above +1SD), the rest at 100.5 (above VWAP, inside +1SD);
    // lows 100.1 (no VWAP tag), highs 101.5 (a +1SD tag but no +2SD touch)
    {
        SBar b[30];
        auto fill = [&](int na1) {
            for (int i = 0; i < 30; i++) { b[i].vw = 100; b[i].sd = 1; b[i].h = 101.5; b[i].l = 100.1; b[i].c = 100.5; }
            for (int i = 0; i < na1; i++) b[29 - i].c = 101.2;              // the newest na1 bars of the last 20
        };
        fill(10); Trend t = evalWindow(b, 30);
        CHECK(t.state == 1 && t.strength == STR_PLAIN && t.nAbove1 == 10 && level(t) == 1, "10 of 20 closes above +1SD: plain Uptrend, not strong");
        CHECK(std::string(displayLabel(t)) == "Uptrend", "display: Uptrend");
        fill(11); t = evalWindow(b, 30);
        CHECK(t.state == 1 && t.strength == STR_STRONG && t.nAbove1 == 11 && level(t) == 2, "11 of 20 closes above +1SD, no VWAP tag: Strong Uptrend");
        CHECK(t.via == VIA_MAJORITY, "strong keeps recording the plain definition (majority)");
        CHECK(std::string(displayLabel(t)) == "Strong Uptrend" && displayColor(t) == COLOR_UP, "display: Strong Uptrend, green");
        CHECK(std::string(label(t)) == "Uptrend", "label() stays plain (status-file compatible)");
        b[15].l = 100.0;  t = evalWindow(b, 30);                              // low EXACTLY at VWAP inside the window = a tag
        CHECK(t.tagVwDn && t.strength == STR_PLAIN && level(t) == 1, "a low exactly at VWAP is a tag: back to plain Uptrend");
        b[15].l = 100.0000001; t = evalWindow(b, 30);
        CHECK(!t.tagVwDn && level(t) == 2, "a low a hair above VWAP is not a tag");
        b[15].l = 100.1; b[9].l = 99.0; t = evalWindow(b, 30);                // a tag 21 bars ago (index 9 of n 30) is out of the window
        CHECK(level(t) == 2, "a VWAP tag 21 bars ago is outside the window");
        b[10].l = 99.5; t = evalWindow(b, 30);                                // 20 bars ago: inside
        CHECK(level(t) == 1, "a VWAP tag 20 bars ago is inside the window");
        b[9].l = b[10].l = 100.1;
        // a close EXACTLY at +1SD does not count (strictly above)
        fill(11); b[29].c = 101.0; t = evalWindow(b, 30);
        CHECK(t.nAbove1 == 10 && level(t) == 1, "a close exactly at +1SD does not count");
        // strong with a +2SD touch on the way: via 2SD, still strong
        fill(11); b[28].h = 102.2; t = evalWindow(b, 30);
        CHECK(level(t) == 2 && t.via == VIA_2SD && t.up2Ago == 1, "strong with a +2SD touch: via 2SD, Strong Uptrend");
        // SD 0 / NaN: no band, so no strong (the 1SD band is undefined)
        fill(20); for (int i = 0; i < 30; i++) b[i].sd = 0; t = evalWindow(b, 30);
        CHECK(t.nAbove1 == 0 && t.strength != STR_STRONG, "SD 0: no strong");
        for (int i = 0; i < 30; i++) b[i].sd = std::numeric_limits<double>::quiet_NaN();
        t = evalWindow(b, 30);
        CHECK(t.strength != STR_STRONG, "SD NaN: no strong");
        // the mirror, built by hand
        for (int i = 0; i < 30; i++) { b[i].vw = 100; b[i].sd = 1; b[i].l = 98.5; b[i].h = 99.9; b[i].c = 99.5; }
        for (int i = 0; i < 11; i++) b[29 - i].c = 98.8;
        t = evalWindow(b, 30);
        CHECK(level(t) == -2 && t.nBelow1 == 11 && std::string(displayLabel(t)) == "Strong Dntrend" && displayColor(t) == COLOR_DN, "Strong Dntrend, red");
        b[19].c = 99.5; t = evalWindow(b, 30);
        CHECK(level(t) == -1 && std::string(displayLabel(t)) == "Dntrend", "10 of 20 below -1SD: plain Dntrend");
        b[19].c = 98.8; b[20].h = 100.0; t = evalWindow(b, 30);
        CHECK(level(t) == -1 && t.tagVwUp, "a high exactly at VWAP is a tag: plain Dntrend");
        // research variants (StrongRule)
        fill(9); StrongRule r9; r9.minCloses = 9;
        CHECK(level(evalWindow(b, 30)) == 1 && level(evalWindow(b, 30, r9)) == 2, "variant 9 of 20");
        fill(12); StrongRule r13; r13.minCloses = 13;
        CHECK(level(evalWindow(b, 30)) == 2 && level(evalWindow(b, 30, r13)) == 1, "variant 13 of 20");
        fill(11); b[20].l = 100.3; StrongRule rp; rp.tagSd = 0.5; StrongRule rm; rm.tagSd = -0.5;
        CHECK(level(evalWindow(b, 30)) == 2 && level(evalWindow(b, 30, rp)) == 1, "tag line VWAP + 0.5 SD (stricter): a low at 100.3 is a tag");
        b[20].l = 99.7;
        CHECK(level(evalWindow(b, 30)) == 1 && level(evalWindow(b, 30, rm)) == 2, "tag line VWAP - 0.5 SD (looser): a low at 99.7 is not a tag");
        // a variant that qualifies strong without plain: via STRONG
        for (int i = 0; i < 30; i++) { b[i].vw = 100; b[i].sd = 1; b[i].h = 101.5; b[i].l = 100.1; b[i].c = 99.9; }
        for (int i = 0; i < 9; i++) b[29 - i].c = 101.2;                          // 9 above +1SD, 11 below VWAP; highs tag +1SD
        t = evalWindow(b, 30, r9);
        CHECK(level(t) == 2 && t.via == VIA_STRONG, "variant: strong without the plain rule records via STRONG");
        CHECK(level(evalWindow(b, 30)) == 0, "the same bars under his rule: no trend");
        CHECK(std::string(displayLabel(Trend())) == "" && displayColor(Trend()) == 0u && level(Trend()) == 0, "none: empty label, no colour");
    }
    // --- property checks on 200,000 random windows: mirror symmetry, strong => plain (his rule), ladder exclusive
    {
        uint64_t sd = 88172645463325252ull;
        auto rnd = [&]() { sd ^= sd << 13; sd ^= sd >> 7; sd ^= sd << 17; return (double)(sd >> 11) / 9007199254740992.0; };
        int bad_mirror = 0, bad_implies = 0, nStrong[5] = {0, 0, 0, 0, 0};
        SBar b[24], m[24];
        StrongRule rules[3]; rules[1].minCloses = 9; rules[2].tagSd = -0.5;
        for (int it = 0; it < 200000; it++) {
            double drift = (rnd() - 0.5) * 3.0;
            for (int i = 0; i < 24; i++) {
                b[i].vw = 100 + (rnd() - 0.5) * 0.2; b[i].sd = (it % 97 == 0 && i == 5) ? 0.0 : 0.5 + rnd();
                double c0 = b[i].vw + drift * b[i].sd + (rnd() - 0.5) * 2.0 * b[i].sd;
                b[i].c = c0; b[i].h = c0 + rnd() * 1.5 * b[i].sd; b[i].l = c0 - rnd() * 1.5 * b[i].sd;
                if (it % 50 == 0) b[i].l = b[i].vw;                           // exact ties at VWAP
                m[i].vw = -b[i].vw; m[i].sd = b[i].sd; m[i].c = -b[i].c; m[i].h = -b[i].l; m[i].l = -b[i].h;
            }
            for (int q = 0; q < 3; q++) {
                Trend a = evalWindow(b, 24, rules[q]), z = evalWindow(m, 24, rules[q]);
                if (level(a) != -level(z) || a.via != z.via || a.nAbove1 != z.nBelow1) bad_mirror++;
                if (q == 0) {
                    nStrong[level(a) + 2]++;
                    if (a.strength == STR_STRONG && a.via == VIA_STRONG) bad_implies++;
                }
            }
        }
        CHECK(bad_mirror == 0, "mirror symmetry on 600,000 random windows (levels, via, counts)");
        CHECK(bad_implies == 0, "his rule: strong always implies the plain state on the same side");
        printf("random levels -2..2: %d %d %d %d %d\n", nStrong[0], nStrong[1], nStrong[2], nStrong[3], nStrong[4]);
        CHECK(nStrong[0] > 100 && nStrong[4] > 100 && nStrong[1] > 1000 && nStrong[3] > 1000 && nStrong[2] > 1000, "random windows reach all 5 levels");
    }
    // --- from chart bars: replay == live including strength, and a climb that reads strong
    {
        std::vector<svl::Bar> B; double px = 100; int e;
        for (e = 8 * 60 + 33; e <= 15 * 60; e += 3) {
            int k = (int)B.size();
            px += (k < 60) ? 0.25 : (k < 90 ? -0.35 : 0.05 * ((k % 7) - 3));     // climb, drop, chop
            B.push_back(mk(FRI, e, (float)px + 0.3f, (float)px - 0.3f, (float)px, 1000 + 37 * (k % 11)));
        }
        Trend t = fromChart(B, 40, 180, o, c);
        CHECK(level(t) == 2, "a steady climb reads Strong Uptrend");
        std::vector<SBar> sb; svl::Acc acc; bool same = true; int seen[5] = {0, 0, 0, 0, 0};
        for (int i = 0; i < (int)B.size(); i++) {
            acc.add(((double)B[i].h + B[i].l + B[i].c) / 3.0, B[i].v);
            SBar y; y.h = B[i].h; y.l = B[i].l; y.c = B[i].c; y.vw = acc.vw; y.sd = acc.sd(); sb.push_back(y);
            Trend a = evalWindow(&sb[0], (int)sb.size()), b2 = fromChart(B, i + 1, 180, o, c);
            if (level(a) != level(b2) || a.via != b2.via || a.nAbove1 != b2.nAbove1 || a.tagVwDn != b2.tagVwDn) same = false;
            seen[level(b2) + 2]++;
        }
        CHECK(same, "replay bar by bar == one straight pass (with strength)");
        CHECK(seen[4] > 0 && seen[0] + seen[1] > 0, "the climb-drop-chop day visits strong up and the down side");
    }
    printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
