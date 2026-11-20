// test_volrange_130.cpp - lsVolRead / lsRangeRead 1.3.0: the marks (CB SR Bk Sw Rev Ct P, biggest in 50+). SDK-free.
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_volrange_130.cpp -o tv130 && ./tv130 <ES bars csv>
#include "VolRangeLogic.h"
#include <fstream>
#include <iostream>
static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
static const long long D0 = 20000LL * 86400;                       // a local midnight
static vrr::Bar bar(long long t, double o, double h, double l, double c, double v) { vrr::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.v = v; return b; }
static vrr::Engine eng(int mode = vrr::VOL) { vrr::Engine E; E.mode = mode; E.tick = 0.25; E.spb = 180; E.hlOn = true; return E; }
int main(int argc, char** argv) {
    {   // parsers
        std::map<long long, std::vector<double>> d;
        vrr::parseLvHist("2026-11-06|1794109922|ONL=7322.250000;PHL=7378.000000;ONH/LonHI=7427.250000;PHH/PDH=7500.000000;bad=x;=5\nrubbish\n2026-13-06|1|A=1\n", d);
        CHECK(d.size() == 1 && d.begin()->first == vrr::lvCivil(2026, 11, 6), "hist: one good session line, keyed by its civil day");
        CHECK(d.begin()->second.size() == 3, "hist: the prior-hour level alone is left out, a joined name with another level stays (" << d.begin()->second.size() << ")");
        long long at = 0; std::vector<double> lv;
        CHECK(vrr::parseLvLive("KEYLV,1,1794797657\r\nL|PWL|7322.250000|0\r\nL|PHL|7636.000000|0\r\nL|ONH/PHH|7669.250000|0\r\nL|X|abc|0\r\n", at, lv) && at == 1794797657 && lv.size() == 2, "live: header + levels, no prior-hour level, no bad price");
        CHECK(!vrr::parseLvLive("L|PWL|7322.25|0\n", at, lv) && lv.empty(), "live: no header = nothing");
        std::vector<vrr::CobIt> c;
        vrr::parseCk("COBCHECK,1.0.1\nCK,1794472200,BULL,S1,7572,-,count 12,3row 1,SMA 1,first 1,checks 3,Lvp 0,CW 0 0.00 running,win 55,CB 1794472380 44,SR 1794472200\r\nCK,5,BEAR,R1,1,-,CB x 1,SR 2\nCK,7,BEAR,R1,1,-,count 1,CB 0 0,SR 0\n", c);
        CHECK(c.size() == 2 && c[0].side == 1 && c[0].cbT == 1794472380 && c[0].srT == 1794472200 && c[1].side == -1 && c[1].cbT == 0, "CK: two good lines, the torn one dropped");
        CHECK(vrr::sessionDay(D0 + 9 * 3600) == 20000 && vrr::sessionDay(D0 - 7 * 3600 + 180) == 20000 && vrr::sessionDay(D0 - 7 * 3600) == 19999 && vrr::sessionDay(D0 + 17 * 3600) == 20000 && vrr::sessionDay(D0 + 17 * 3600 + 180) == 20001, "session day: 17:00 CT ends a session, the 17:03 bar starts the next");
        CHECK(vrr::lvNameOk("ONH") && !vrr::lvNameOk("PHH/PHL") && !vrr::lvNameOk("VWAP") && vrr::lvNameOk("PHH/PDH"), "level names");
    }
    {   // (1.7.0) the price chart's rules: a green bar can't be a bearish Sw; a Bk bar smaller than the bar before is no Bk; POC / Open are not levels
        vrr::Engine E = eng(); E.lvDay[20000] = { 105.0 };
        long long t = D0 + 9 * 3600; double v = 100;
        for (int i = 0; i < 8; ++i) { E.addBar(bar(t, 100, 101, 99, 100, v), t); t += 180; v -= 1; }
        E.addBar(bar(t, 100, 106, 100, 104, 500), t); t += 180;                // 8: sweeps 105 and closes back under, but GREEN
        E.markLevels(); CHECK(E.markOf(8).code != vrr::MK_SW, "(1.7.0) a green bar is not a bearish Sw");
        vrr::Engine F = eng(); F.lvDay[20000] = { 105.0 }; t = D0 + 9 * 3600; v = 100;
        for (int i = 0; i < 7; ++i) { F.addBar(bar(t, 100, 101, 99, 100, v), t); t += 180; v -= 1; }
        F.addBar(bar(t, 100, 104.9, 95, 104, 90), t); t += 180;                // 7: a wide bar under the level
        F.addBar(bar(t, 104, 106, 103, 105.5, 500), t); t += 180;              // 8: closes over 105 but its range (3) < the bar before (9.9)
        F.markLevels(); CHECK(F.markOf(8).code != vrr::MK_BK, "(1.7.0) the BO candle rule: a Bk bar's range >= the bar before");
        CHECK(!vrr::lvNameOk("DPoc") && !vrr::lvNameOk("WPoc") && !vrr::lvNameOk("Open") && vrr::lvNameOk("VAL"), "(1.7.0) no POC / Open levels");
    }
    {   // Bk / Sw at a level: gate 5, direction, cooldown, Bk wins
        vrr::Engine E = eng(); E.lvDay[20000] = { 105.0 };
        long long t = D0 + 9 * 3600; double v = 100;
        for (int i = 0; i < 8; ++i) { E.addBar(bar(t, 100, 101, 99, 100, v), t); t += 180; v -= 1; }                 // quiet, falling volume
        E.addBar(bar(t, 105.5, 106, 100, 104, 500), t); t += 180;            // 8: through 105, closes back under = Sw, bearish (1.7.0: a red bar)
        E.addBar(bar(t, 104, 104.5, 103, 104, 90), t); t += 180;             // 9
        E.addBar(bar(t, 104, 107, 104, 106, 600), t); t += 180;              // 10: closes over 105 within the cooldown = no mark
        E.markLevels();
        CHECK(E.markOf(8).code != vrr::MK_SW && E.markOf(8).code != vrr::MK_BK, "(1.8.0) levels alone give no Sw / Bk: they come from the price chart's marks file (t_vr180)");
        CHECK(E.markOf(10).code != vrr::MK_BK, "the same level again within 10 bars: no second mark");
        for (int i = 0; i < 12; ++i) { E.addBar(bar(t, 106, 107, 105.5, 106, 50 - i), t); t += 180; }
        E.addBar(bar(t, 106, 106, 103, 103.5, 700), t); t += 180;            // closes under 105 = Bk down
        E.markLevels(); const int k = (int)E.V.size() - 1;
        CHECK(E.markOf(k).code != vrr::MK_BK, "(1.8.0) levels alone give no Bk (t_vr180 covers the marks file)");
        vrr::Engine F = eng(); F.lvDay[20000] = { 105.0 }; t = D0 + 9 * 3600;
        for (int i = 0; i < 8; ++i) { F.addBar(bar(t, 100, 101, 99, 100, 100 + i * 50), t); t += 180; }
        F.addBar(bar(t, 100, 106, 100, 106, 120), t); F.markLevels();
        CHECK(F.markOf(8).code == 0, "a break on a bar that is not the biggest in 5 = no mark");
        vrr::Engine H = eng(); H.lvDay[20000] = { 105.0, 105.5 }; t = D0 + 9 * 3600; v = 100;
        for (int i = 0; i < 8; ++i) { H.addBar(bar(t, 100, 101, 99, 100, v), t); t += 180; v -= 1; }
        H.addBar(bar(t, 100, 106, 100, 105.25, 500), t); H.markLevels();
        { std::map<long long, vrr::Mk> w; vrr::Mk m1; m1.code = vrr::MK_BK; m1.dir = 1; w[H.T[8]] = m1; H.wy.swap(w); H.wyLines = 1; } CHECK(H.markOf(8).code == vrr::MK_BK && H.markOf(8).dir == 1, "(1.8.0) a Bk in the marks file = Bk on the same bar");
        vrr::Engine N = eng(); N.lvDay[19990] = { 105.0 }; t = D0 + 9 * 3600; v = 100;
        for (int i = 0; i < 8; ++i) { N.addBar(bar(t, 100, 101, 99, 100, v), t); t += 180; v -= 1; }
        N.addBar(bar(t, 100, 106, 100, 106, 500), t); N.markLevels();
        CHECK(N.markOf(8).code == 0, "another session's levels are not used");
    }
    {   // P: a pivot high on the biggest bar in 20+, marked only once the next wave confirms it; big 50 has no code
        vrr::Engine E = eng(); long long t = D0 + 9 * 3600; double p = 100;
        for (int i = 0; i < 30; ++i) { E.addBar(bar(t, p, p + 1, p - 0.5, p + 0.75, 100), t); t += 180; p += 0.75; }   // a steady rise
        E.addBar(bar(t, p, p + 3, p - 0.5, p + 2, 900), t); t += 180; const int top = (int)E.V.size() - 1; const double hi = p + 3;
        CHECK(E.markOf(top).code == 0, "the top bar is not a pivot yet (biggest in 30, under 50)");
        E.addBar(bar(t, p + 2, p + 2.5, p, p + 0.5, 150), t); t += 180;
        E.addBar(bar(t, p + 0.5, p + 1, p - 4, p - 3.5, 200), t); t += 180;                                           // takes out the 3-bar low
        CHECK(E.markOf(top).code == vrr::MK_P && E.markOf(top).dir == -1, "confirmed pivot high on a 20+ bar = P, red");
        (void)hi;
        vrr::Engine B = eng(); t = D0 + 9 * 3600;
        for (int i = 0; i < 60; ++i) { B.addBar(bar(t, 100, 100.5, 99.5, 100.25, 100), t); t += 180; }
        B.addBar(bar(t, 100, 100.75, 99.5, 99.75, 400), t);
        CHECK(B.markOf(60).code == vrr::MK_BIG && B.markOf(60).dir == -1 && std::string(vrr::mkText(vrr::MK_BIG)).empty(), "biggest in 50+ = coloured by its candle, no code");
    }
    {   // Cob marks: CB / SR by time; Ct = a counter bar bigger than every bar since the origin; gone once the origin breaks
        vrr::Engine E = eng(); long long t = D0 + 9 * 3600; std::vector<long long> ts;
        auto add = [&](double o, double h, double l, double c, double v) { ts.push_back(t); E.addBar(bar(t + 7, o, h, l, c, v), t); t += 180; };
        for (int i = 0; i < 6; ++i) add(100, 100.5, 99 - i * 0.25, 99.5, 100);
        add(98, 98.5, 96, 98.25, 300);      // 6: the origin low 96
        add(98.25, 100, 98, 99.75, 400);    // 7: CB
        add(99.75, 101, 99.5, 100.75, 250); // 8
        add(100.75, 102, 100.5, 101.75, 200); // 9: the Cob wave ends (t of the CK line)
        add(101.75, 102, 100.5, 100.75, 150); // 10: red, smaller
        add(100.75, 101, 99, 99.25, 450);     // 11: red, bigger than every bar since the origin = Ct
        add(99.25, 99.5, 98, 98.25, 500);     // 12: red, bigger again = Ct
        add(98.25, 98.5, 95, 95.5, 900);      // 13: breaks the origin - no Ct
        vrr::CobIt c; c.t = ts[9]; c.side = 1; c.cbT = ts[7]; c.srT = ts[6]; E.setCobs({ c }); E.cobMarks();
        CHECK(E.markOf(7).code == vrr::MK_CB && E.markOf(7).dir == 1 && E.markOf(6).code == vrr::MK_SR && E.markOf(6).dir == 1, "CB and SR bars from the Cob Reader, the Cob's colour");
        CHECK(E.markOf(11).code == vrr::MK_CT && E.markOf(11).dir == -1 && E.markOf(12).code == vrr::MK_CT && E.markOf(10).code == 0, "counter bars bigger than every bar since the origin = Ct, against the Cob");
        CHECK(E.markOf(13).code != vrr::MK_CT, "after the origin breaks: no Ct");
        E.setCobs({}); E.cobMarks(); CHECK(E.markOf(7).code == vrr::MK_CB, "a Cob that left the status list keeps its marks");
        E.hlLocal[ts[7]] = vrr::Hl{ -1, vrr::HL_PBREV }; E.hlLocal[ts[10]] = vrr::Hl{ 1, vrr::HL_PBREV };
        CHECK(E.markOf(7).code == vrr::MK_CB && E.markOf(10).code == vrr::MK_REV && E.markOf(10).dir == 1, "priority: CB over Rev; Rev shows in the Volume pane");
        // layout: grey unless marked; the code is drawn bold in the bar's colour
        vrr::View v; v.l = 0; v.t = 0; v.r = 900; v.b = 120; v.first = 0; v.last = (int)E.V.size() - 1; v.bw = 10; v.X = [](int j) { return 150.0 + j * 40; };
        const std::vector<vrr::Prim> P = vrr::layout(E, v, nullptr);
        int cb = 0, grey = 0, inside = 1; for (const vrr::Prim& q : P) { if (!q.fill && q.text == "CB" && q.bold && q.col == vrr::C_UP) ++cb; if (q.fill && q.col == vrr::C_GREY) ++grey; if (!q.fill && (q.y0 < v.t || q.y1 > v.b)) inside = 0; }
        CHECK(cb == 1 && grey >= 5 && inside, "layout: one bold green CB, unmarked bars grey, every text inside the pane");
    }
    if (argc > 1) {   // the ES fixture: bar by bar = one pass; a mark given at a bar's close only ever changes to a later-confirmed P / CB / SR
        std::vector<vrr::Bar> B; { std::ifstream f(argv[1]); std::string l; std::getline(f, l); while (std::getline(f, l)) { vrr::Bar b; if (std::sscanf(l.c_str(), "%lld,%lf,%lf,%lf,%lf,%lf", &b.t, &b.o, &b.h, &b.l, &b.c, &b.v) == 6) B.push_back(b); } }
        CHECK(B.size() > 5000, "fixture loaded (" << B.size() << ")");
        for (int mode = 0; mode < 2; ++mode) {
            std::map<long long, std::vector<double>> days; std::map<long long, std::pair<double, double>> hl;
            for (const vrr::Bar& b : B) { auto& q = hl[vrr::sessionDay(b.t)]; if (q.first == 0) q = { b.h, b.l }; q.first = std::max(q.first, b.h); q.second = std::min(q.second, b.l); }
            { long long prev = 0; bool have = false; for (const auto& kv : hl) { if (have) days[kv.first] = { hl[prev].first, hl[prev].second, (hl[prev].first + hl[prev].second) / 2 }; prev = kv.first; have = true; } }   // the prior session's high, low and middle
            vrr::Engine A = eng(mode), C = eng(mode); A.lvDay = days; C.lvDay = days; std::vector<vrr::Mk> first;
            for (const vrr::Bar& b : B) { A.addBar(b, b.t); A.markLevels(); first.push_back(A.markOf((int)A.V.size() - 1)); }
            for (const vrr::Bar& b : B) C.addBar(b, b.t);
            C.markLevels();
            bool same = true, stable = true; int cnt[vrr::MK_CB + 1] = { 0 };
            for (int i = 0; i < (int)B.size(); ++i) { const vrr::Mk a = A.markOf(i), c = C.markOf(i); if (a.code != c.code || a.dir != c.dir) same = false; ++cnt[a.code];
                if ((first[(size_t)i].code != a.code || first[(size_t)i].dir != a.dir) && a.code != vrr::MK_P) stable = false; }
            CHECK(same, (mode ? "Range" : "Volume") << ": bar by bar = one straight pass");
            CHECK(stable, (mode ? "Range" : "Volume") << ": a mark given at the close never changes, except to a pivot confirmed later");
            CHECK(cnt[vrr::MK_BK] > 5 && cnt[vrr::MK_SW] > 5 && cnt[vrr::MK_P] > 5 && cnt[vrr::MK_BIG] > 5, (mode ? "Range" : "Volume") << ": every kind occurs (Bk " << cnt[vrr::MK_BK] << ", Sw " << cnt[vrr::MK_SW] << ", P " << cnt[vrr::MK_P] << ", big50 " << cnt[vrr::MK_BIG] << " of " << B.size() << ")");
            const int marked = cnt[vrr::MK_BK] + cnt[vrr::MK_SW] + cnt[vrr::MK_P] + cnt[vrr::MK_BIG];
            CHECK(marked < (int)B.size() / 12, (mode ? "Range" : "Volume") << ": marks stay rare (" << marked << " of " << B.size() << ")");
            std::cout << (mode ? "Range " : "Volume") << " marks on the fixture: Bk " << cnt[vrr::MK_BK] << " Sw " << cnt[vrr::MK_SW] << " P " << cnt[vrr::MK_P] << " big50 " << cnt[vrr::MK_BIG] << " of " << B.size() << " bars\n";
            int htfN = 0; for (size_t i = 0; i < C.htf.size(); ++i) if (std::isfinite(C.htf[i])) ++htfN;
            CHECK(htfN > 100, "the 3-hour levels are carried per bar (" << htfN << ")");
        }
    }
    std::cout << "test_volrange_130: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
