// HtfSr.h 1.0.0: parity with lsCobSignal's Htf on every recorded bar, the standing list, the join into a level list
#include "../HtfSr.h"
#include "CobSignalLogic.h"
#include <cstdio>
#include <fstream>
static int ok = 0, bad = 0;
#define CHECK(c) do { if (c) ++ok; else { ++bad; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)
struct M { std::string name; double price; bool pivotOnly; };
static bool same(double a, double b) { return (std::isnan(a) && std::isnan(b)) || a == b; }
int main(int argc, char** argv)
{
    // 1. parity on real bars: argv = files of "t,o,h,l,c,v"
    for (int a = 1; a < argc; ++a) {
        std::ifstream f(argv[a]); std::string l; cobs::Htf H; hsr::Walk W; long n = 0, diff = 0; hsr::Snap last;
        while (std::getline(f, l)) { cobs::Bar b; if (std::sscanf(l.c_str(), "%lld,%lf,%lf,%lf,%lf,%lf", &b.t, &b.o, &b.h, &b.l, &b.c, &b.v) != 6) continue;
            const cobs::HSnap x = H.add(b, 180); hsr::Bar q; q.t = b.t; q.h = b.h; q.l = b.l; q.c = b.c; const hsr::Snap y = W.add(q, 180); ++n; last = y;
            if (!(same(x.r1, y.r1) && same(x.r2, y.r2) && same(x.s1, y.s1) && same(x.s2, y.s2) && x.r1ok == y.r1ok && x.r2ok == y.r2ok && x.s1ok == y.s1ok && x.s2ok == y.s2ok && x.dir == y.dir && x.ext == y.ext)) ++diff; }
        std::printf("%s bars %ld differences %ld | %s\n", argv[a], n, diff, hsr::statusText(last, 0.25, (int)W.HB.size(), 180).c_str());
        CHECK(n > 1000); CHECK(diff == 0);
    }
    // 2. a hand-made walk: 3-hour bars fed as 3-hour bars (a 180-min chart)
    {   hsr::Walk W; long long t = 86400LL * 100 + 20 * 3600;   // a bar ending 20:00 = the 17-20 bar
        struct R { double h, l, c; }; const R r[] = { {100,95,99}, {104,98,103}, {103,96,97}, {99,92,93}, {98,93,97}, {101,96,100}, {100,94,95} };
        hsr::Snap s; for (const R& x : r) { hsr::Bar b; b.t = t; b.h = x.h; b.l = x.l; b.c = x.c; s = W.add(b, 10800); t += 10800; }
        // up to 104 (bar 1); bar 2 lower low -> pivot high 104; down to 92 (bar 3); bar 4 higher high? 98 < 99 no; bar 5 h 101 > 98 -> pivot low 92; up; bar 6 (forming) l 94 < 96 and no new high -> pivot high 101
        CHECK(s.r1 == 101); CHECK(s.r2 == 104); CHECK(s.s1 == 92); CHECK(s.s2 == 92); CHECK(s.dir == -1);
        CHECK(s.r1ok && s.r2ok && s.s1ok);
        CHECK((int)W.HB.size() == 7);
        auto v = hsr::standing(s, 0.25); CHECK(v.size() == 3); CHECK(v[0].name == "R1" && v[0].px == 101 && v[0].side == 1); CHECK(v[1].name == "R2" && v[1].px == 104); CHECK(v[2].name == "S1" && v[2].px == 92 && v[2].side == -1);
        // a completed bar closing above R2 breaks R1 and R2 once a later bar exists
        hsr::Bar b; b.t = t; b.h = 106; b.l = 99; b.c = 105; s = W.add(b, 10800); t += 10800;
        b.t = t; b.h = 105.5; b.l = 103; b.c = 104; s = W.add(b, 10800);
        v = hsr::standing(s, 0.25); bool anyR = false; for (auto& q : v) if (q.side > 0) anyR = true;
        std::printf("after break: r1 %.2f ok %d r2 %.2f ok %d s1 %.2f ok %d\n", s.r1, s.r1ok, s.r2, s.r2ok, s.s1, s.s1ok);
        CHECK(!(s.r1 == 101 && s.r1ok)); CHECK(!(s.r2 == 104 && s.r2ok)); (void)anyR;
    }
    // 2b. (review) a 3-hour chart: the short last bar of the session (14:00 - 16:00, stamped 16:00) is its own bar, never merged into 11 - 14
    {   hsr::Walk W; hsr::Bar b; const long long day = 86400LL * 200;
        b.t = day + 14 * 3600; b.h = 10; b.l = 5; b.c = 9; W.add(b, 10800); b.t = day + 16 * 3600; b.h = 12; b.l = 8; b.c = 11; W.add(b, 10800); b.t = day + 20 * 3600; b.h = 13; b.l = 9; b.c = 12; W.add(b, 10800);
        CHECK(W.HB.size() == 3); CHECK(W.HB[0].h == 10 && W.HB[1].h == 12 && W.HB[2].h == 13);
        hsr::Walk V; b.t = day + 14 * 3600 - 180; b.h = 10; b.l = 5; b.c = 9; V.add(b, 180); b.t = day + 14 * 3600; b.h = 11; b.l = 6; b.c = 9; V.add(b, 180); b.t = day + 14 * 3600 + 180; b.h = 12; b.l = 7; b.c = 9; V.add(b, 180);
        CHECK(V.HB.size() == 2 && V.HB[0].h == 11 && V.HB[1].h == 12); }   // 3-min bars: 13:57-14:00 ends the 11-14 bar, 14:00-14:03 starts the next
    // 3. periods
    CHECK(hsr::periodOk(180)); CHECK(hsr::periodOk(3600)); CHECK(hsr::periodOk(10800)); CHECK(!hsr::periodOk(14400)); CHECK(!hsr::periodOk(0)); CHECK(!hsr::periodOk(420));
    {   hsr::Walk W; hsr::Bar b; b.t = 1000; b.h = NAN; b.l = 1; b.c = 1; hsr::Snap s = W.add(b, 180); CHECK(s.dir == 0 && W.HB.empty()); }
    // 4. standing: broken levels dropped, R2 on R1's price dropped, rounded to the tick
    {   hsr::Snap s; s.r1 = 7649.75; s.r1ok = true; s.r2 = 7649.75; s.r2ok = true; s.s1 = 7598.5; s.s1ok = false; s.s2 = 7572.0000001; s.s2ok = true;
        auto v = hsr::standing(s, 0.25); CHECK(v.size() == 2); CHECK(v[0].name == "R1"); CHECK(v[1].name == "S2" && v[1].px == 7572.0);
        s.r1ok = false; v = hsr::standing(s, 0.25); CHECK(v.size() == 2 && v[0].name == "R2" && v[0].px == 7649.75);   // R1 broken, R2 standing on the same price is still shown
        hsr::Snap e; CHECK(hsr::standing(e, 0.25).empty()); }
    // 5. names
    CHECK(hsr::onlyHtf("R1")); CHECK(hsr::onlyHtf("R1/S2")); CHECK(!hsr::onlyHtf("ONH/R1")); CHECK(!hsr::onlyHtf("")); CHECK(!hsr::onlyHtf("PDH"));
    CHECK(hsr::hasHtf("ONH/R1")); CHECK(hsr::hasHtf("S2")); CHECK(!hsr::hasHtf("ONH/PHH")); CHECK(!hsr::hasHtf("R10"));
    // 6. the join
    {   std::vector<M> V = { {"ONH/PHH", 7669.25, false}, {"AsiaHI", 7649.75, false}, {"VWAP", 7598.5, false}, {"PDL", 7581.5, false} };
        std::vector<hsr::Lv> L; hsr::Lv q; q.name = "R1"; q.px = 7649.75; q.side = 1; L.push_back(q); q.name = "S1"; q.px = 7598.5; q.side = -1; L.push_back(q); q.name = "S2"; q.px = 7572.0; L.push_back(q);
        CHECK(hsr::addTo(V, L, 0.25) == 3); CHECK(V.size() == 6);
        CHECK(V[1].name == "AsiaHI/R1"); CHECK(V[2].name == "VWAP"); CHECK(V[4].name == "S1" && V[4].price == 7598.5); CHECK(V[5].name == "S2");
        CHECK(hsr::addTo(V, L, 0.25) == 3); CHECK(V[1].name == "AsiaHI/R1");   // twice = no double name (S1 / S2 join their own rows)
        CHECK(V.size() == 6); CHECK(hsr::addTo(V, L, 0) == 0); }
    // 7. the lines: standing values only, stepped, named at the right end, inside the pane
    {   std::vector<hsr::Snap> hs(10); for (int i = 0; i < 10; ++i) { hs[i].r1 = i < 5 ? 110 : 108; hs[i].r1ok = i != 7; hs[i].r2 = 110; hs[i].r2ok = true; hs[i].s1 = 100; hs[i].s1ok = true; hs[i].s2 = 100; hs[i].s2ok = true; }
        hsr::View v; v.l = 0; v.t = 0; v.r = 200; v.b = 200; v.first = 0; v.last = 9; v.X = [](int j) { return 5.0 + j * 10.0; }; v.Y = [](double p) { return 200.0 - (p - 95.0) * 10.0; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; };
        const std::vector<hsr::Prim> P = hsr::layout(hs, 0.25, v); int lines = 0, texts = 0, out = 0; std::string names;
        for (const hsr::Prim& p : P) { if (p.line) ++lines; else { ++texts; names += p.text + " "; } if (p.x0 < v.l || p.x1 > v.r || p.y0 < v.t || p.y1 > v.b) ++out; }
        std::printf("lines %d texts %d (%s) outside %d\n", lines, texts, names.c_str(), out);
        // R1: 110 over bars 0-4, 108 over 5-6, (7 broken = gap), 108 over 8-9 = 3 pieces; R2: hidden on bars 0-4 (same as R1), 110 over 5-9 = 1 piece (bar 7: R1 broken, so R2 shows; same price, one piece); S1 1 piece; S2 none
        CHECK(lines == 5); CHECK(texts == 3); CHECK(names == "R1 R2 S1 "); CHECK(out == 0);
        CHECK(std::isnan(hsr::shown(hs[7], 0, 0.25))); CHECK(hsr::shown(hs[7], 1, 0.25) == 110); CHECK(std::isnan(hsr::shown(hs[0], 1, 0.25))); CHECK(std::isnan(hsr::shown(hs[0], 3, 0.25)));
        v.first = 20; CHECK(hsr::layout(hs, 0.25, v).empty()); v.first = 0; CHECK(hsr::layout(hs, 0, v).empty()); }
    std::printf("%d passed, %d failed\n", ok, bad); return bad ? 1 : 0;
}
