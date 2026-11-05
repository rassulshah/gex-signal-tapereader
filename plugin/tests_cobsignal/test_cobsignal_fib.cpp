// test_cobsignal_fib.cpp - lsCobSignal 1.5.0 CoB Fib lines (fibLines / fibLayout / fibStatus), SDK-free.
//   (in plugin\tests_cobsignal) g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_cobsignal_fib.cpp -o tf && ./tf fib_eu_1105.csv
// Rassul 2026-11-05 12:25-12:42 CT: targets 1.272 / 1.618 / 2.0, entry 50%, stop 0; end on a close through 0 and at the session end;
// entry + stop gone after 3 h; "go with your recommendations" (stop touch / T1 before the fill end entry + stop, a hit target ends at its
// hit bar, a newer drawn CoB ends the older CoB's lines, stop 1 tick beyond 0).
// Covers: [1] EU Nov 5 09:00 long CoB (the approved mockup): 0 = 1.12195 (8:39), 1.0 = 1.12410 (8:57), stop 1.12190, entry 1.123025,
// T1 1.124685 hit 9:30, T2 1.125429 hit 9:54, T3 1.12625 open, entry + stop end 12:00 (3 h); [2] the same bars mirrored = a short CoB, same
// bars; [3] a close back through 0 ends every line; a stop wick ends entry + stop only; [4] T1 before the 50% fills = missed; [5] a newer
// drawn CoB ends the lines, a not-drawn one does not; [6] session end; [7] no repaint: every finished line on a prefix of the bars equals the
// full pass; [8] layout: colours, 1 px lines from the origin bar, names right of the line ends, nothing outside the pane; [9] status lines;
// [10] a CoB not drawn (LOG or ZZO failed) gets no lines.
#include "CobSignalLogic.h"
#include <fstream>
#include <iostream>
#include <sstream>

static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)

static std::vector<cobs::Bar> load(const char* path) {
    std::vector<cobs::Bar> B; std::ifstream f(path); std::string l;
    while (std::getline(f, l)) {
        if (l.empty() || l[0] == '#') continue;
        std::stringstream ss(l); std::string tok; std::vector<std::string> x; while (std::getline(ss, tok, ',')) x.push_back(tok);
        if (x.size() < 6) continue;
        cobs::Bar b; b.t = std::atoll(x[0].c_str()); b.o = std::atof(x[1].c_str()); b.h = std::atof(x[2].c_str()); b.l = std::atof(x[3].c_str()); b.c = std::atof(x[4].c_str()); b.v = std::atof(x[5].c_str());
        B.push_back(b);
    }
    return B;
}
static int at(const std::vector<cobs::Bar>& B, const char* hhmm) {   // the Nov 5 bar stamped hh:mm
    const int h = std::atoi(hhmm), m = std::atoi(hhmm + 3);
    for (size_t i = 0; i < B.size(); ++i) { const long long s = (B[i].t % 86400 + 86400) % 86400; if (B[i].t / 86400 == B.back().t / 86400 && s == h * 3600 + m * 60) return (int)i; }
    return -1;
}
// an Engine holding just these bars + their session classes (the Fib code reads B, cls, marks, tick)
static cobs::Engine eng(const std::vector<cobs::Bar>& B, size_t n) {
    cobs::Engine E; E.setMarket("EU", 0.00005, 180);
    for (size_t i = 0; i < n && i < B.size(); ++i) {
        E.B.push_back(B[i]); const long days = (long)cobs::floorDiv(B[i].t, 86400); const int es = (int)(B[i].t - (long long)days * 86400);
        E.cls.push_back(svl::classify(days, es, E.perSec, E.openMin, E.closeMin));
    }
    return E;
}
static cobs::Mark cob(const std::vector<cobs::Bar>& B, int s, int e, int idx, int side) {
    cobs::Mark m; m.legStart = s; m.legEnd = e; m.idx = idx; m.side = side; m.t = B[(size_t)idx].t; m.show = true; m.zOk = m.zDone = true; return m;
}
static const cobs::FibSeg* seg(const cobs::FibSet& F, int k) { for (const cobs::FibSeg& g : F.seg) if (g.kind == k) return &g; return nullptr; }
static bool near(double a, double b) { return std::fabs(a - b) < 1e-7; }
static cobs::Bar mir(cobs::Bar b) { const double h = b.h, l = b.l; b.h = -l; b.l = -h; b.o = -b.o; b.c = -b.c; return b; }

int main(int argc, char** argv) {
    const std::vector<cobs::Bar> B = load(argc > 1 ? argv[1] : "fib_eu_1105.csv");
    CHECK(B.size() == 300, "EU fixture (" << B.size() << " bars)");
    const int i0 = at(B, "08:39"), i1 = at(B, "08:57"), ic = at(B, "09:00"), last = (int)B.size() - 1;
    CHECK(i0 > 0 && i1 > i0 && ic == i1 + 1, "CoB bars found");
    CHECK(cobs::FIB_K[0] == 1.272 && cobs::FIB_K[1] == 1.618 && cobs::FIB_K[2] == 2.0 && cobs::FIB_ENTRY_SECS == 10800, "his levels 1.272 / 1.618 / 2.0, 3 h");

    // [1] the mockup CoB
    {   cobs::Engine E = eng(B, B.size()); E.marks.push_back(cob(B, i0, i1, ic, 1));
        const cobs::FibSet F = cobs::fibLines(E, E.marks[0]);
        CHECK(F.seg.size() == 6 && near(F.origin, 1.12195) && near(F.one, 1.1241), "EU: 0 = 1.12195, 1.0 = 1.12410, 6 lines");
        const cobs::FibSeg *st = seg(F, cobs::F_STOP), *en = seg(F, cobs::F_ENTRY), *t1 = seg(F, cobs::F_T1), *t2 = seg(F, cobs::F_T2), *t3 = seg(F, cobs::F_T3), *one = seg(F, cobs::F_ONE);
        CHECK(st && en && t1 && t2 && t3 && one, "all kinds");
        if (st && en && t1 && t2 && t3 && one) {
            CHECK(near(st->px, 1.1219) && near(en->px, 1.123025) && near(t1->px, 1.12195 + 1.272 * 0.00215) && near(t2->px, 1.12195 + 1.618 * 0.00215) && near(t3->px, 1.12625), "prices: stop 1.12190, entry 1.123025, T1 1.124685, T2 1.125429, T3 1.12625");
            CHECK(st->i0 == i0 && en->i0 == i0 && t1->i0 == i0, "lines start at the CoB origin bar (8:39)");
            CHECK(t1->hit && t1->i1 == at(B, "09:30") && !t1->open, "T1 hit 9:30 (" << t1->i1 << ")");
            CHECK(t2->hit && t2->i1 == at(B, "09:54"), "T2 hit 9:54");
            CHECK(!t3->hit && t3->open && t3->i1 == last, "T3 open to the live bar (day high 1.12610 < 1.12625)");
            CHECK(!en->open && en->i1 == at(B, "12:00") && st->i1 == en->i1, "entry + stop end at 12:00 (3 h after the 9:00 CoB)");
            CHECK(one->open && one->i1 == last && F.why == "live", "the CoB high line runs on (" << F.why << ")");
        }
    }
    // [2] mirrored = a short CoB
    {   std::vector<cobs::Bar> M; for (const cobs::Bar& b : B) M.push_back(mir(b));
        cobs::Engine E = eng(M, M.size()); E.marks.push_back(cob(M, i0, i1, ic, -1));
        const cobs::FibSet F = cobs::fibLines(E, E.marks[0]);
        const cobs::FibSeg *st = seg(F, cobs::F_STOP), *en = seg(F, cobs::F_ENTRY), *t1 = seg(F, cobs::F_T1), *t2 = seg(F, cobs::F_T2), *t3 = seg(F, cobs::F_T3);
        CHECK(st && en && t1 && t2 && t3 && near(F.origin, -1.12195) && near(st->px, -1.1219) && near(en->px, -1.123025), "short: 0 above, stop 1 tick above, entry 50%");
        if (st && t1 && t2 && t3) CHECK(t1->i1 == at(B, "09:30") && t2->i1 == at(B, "09:54") && t3->open && st->i1 == at(B, "12:00"), "short: same hits and ends as the long");
    }
    // [3] a close back through 0 ends everything; a stop wick (no close through) ends entry + stop only
    {   std::vector<cobs::Bar> C = B; const int k = at(B, "09:06");
        C[(size_t)k].l = 1.1218; C[(size_t)k].c = 1.1219;                         // closes below 0
        cobs::Engine E = eng(C, C.size()); E.marks.push_back(cob(C, i0, i1, ic, 1));
        const cobs::FibSet F = cobs::fibLines(E, E.marks[0]); bool all = !F.seg.empty();
        for (const cobs::FibSeg& g : F.seg) if (g.i1 != k || g.open) all = false;
        CHECK(all && F.why == "closed through 0", "a close below 0 at 9:06 ends every line there (" << F.why << ")");
        std::vector<cobs::Bar> W = B; W[(size_t)k].l = 1.1218;                  // wick only, close 1.12305 above 0
        cobs::Engine E2 = eng(W, W.size()); E2.marks.push_back(cob(W, i0, i1, ic, 1));
        const cobs::FibSet G = cobs::fibLines(E2, E2.marks[0]);
        const cobs::FibSeg *st = seg(G, cobs::F_STOP), *t1 = seg(G, cobs::F_T1);
        CHECK(st && t1 && st->i1 == k && !st->open && t1->i1 == at(B, "09:30") && G.why == "live", "a stop wick ends entry + stop, the targets go on");
    }
    // [4] T1 before the 50% fills = missed (entry + stop end there)
    {   std::vector<cobs::Bar> C = B; for (int j = ic + 1; j < at(B, "09:30"); ++j) C[(size_t)j].l = std::max(C[(size_t)j].l, 1.1231);   // no pullback to 1.123025
        cobs::Engine E = eng(C, C.size()); E.marks.push_back(cob(C, i0, i1, ic, 1));
        const cobs::FibSet F = cobs::fibLines(E, E.marks[0]); const cobs::FibSeg* en = seg(F, cobs::F_ENTRY);
        CHECK(en && en->i1 == at(B, "09:30") && !en->open, "missed: the entry ends at the T1 bar (9:30)");
    }
    // [5] a newer drawn CoB ends the lines; a LOG / ZZO-failed one does not
    {   const int k = at(B, "10:15");
        cobs::Engine E = eng(B, B.size()); E.marks.push_back(cob(B, i0, i1, ic, 1)); cobs::Mark o = cob(B, k - 5, k - 1, k, -1); E.marks.push_back(o);
        const cobs::FibSet F = cobs::fibLines(E, E.marks[0]); bool ok = F.why == "next CoB";
        for (const cobs::FibSeg& g : F.seg) if (g.open || g.i1 > k) ok = false;
        CHECK(ok && seg(F, cobs::F_T3)->i1 == k, "a drawn 10:15 CoB ends the 9:00 lines at 10:15");
        E.marks[1].zOk = false;
        const cobs::FibSet G = cobs::fibLines(E, E.marks[0]);
        CHECK(G.why == "live", "a CoB that failed the ZZO filter does not end them");
    }
    // [6] session end (EU RTH 07:20-14:00): bars after 14:00 end the lines at the last RTH bar
    {   std::vector<cobs::Bar> C = B; const long long d = B.back().t / 86400 * 86400;
        for (int q = 1; q <= 4; ++q) { cobs::Bar b = B.back(); b.t = d + 13 * 3600 + 57 * 60 + q * 180 + (q > 1 ? 0 : 0); C.push_back(b); }
        // stamps 14:00, 14:03, 14:06, 14:09 after a gap - the 14:03 bar is post-RTH
        cobs::Engine E = eng(C, C.size()); E.marks.push_back(cob(C, i0, i1, ic, 1));
        const cobs::FibSet F = cobs::fibLines(E, E.marks[0]); const cobs::FibSeg* t3 = seg(F, cobs::F_T3);
        int lastRth = -1; for (int j = 0; j < (int)E.cls.size(); ++j) if (E.cls[(size_t)j].kind == svl::RTH) lastRth = j;
        CHECK(t3 && !t3->open && t3->i1 == lastRth && F.why == "session end", "session end: T3 stops at the last RTH bar (" << F.why << ")");
    }
    // [7] no repaint: a finished line never changes as bars arrive
    {   cobs::Engine Ef = eng(B, B.size()); Ef.marks.push_back(cob(B, i0, i1, ic, 1)); const cobs::FibSet Ff = cobs::fibLines(Ef, Ef.marks[0]);
        int bad = 0;
        for (size_t n = (size_t)ic + 1; n <= B.size(); ++n) {
            cobs::Engine E = eng(B, n); E.marks.push_back(cob(B, i0, i1, ic, 1)); const cobs::FibSet F = cobs::fibLines(E, E.marks[0]);
            for (size_t q = 0; q < F.seg.size(); ++q) { const cobs::FibSeg& g = F.seg[q]; const cobs::FibSeg& h = Ff.seg[q];
                if (!near(g.px, h.px)) ++bad;
                if (!g.open && (g.i1 != h.i1 || g.open != h.open || g.hit != h.hit)) ++bad;
                if (g.open && g.i1 != (int)n - 1) ++bad; }
        }
        CHECK(bad == 0, "bar by bar: finished lines == the full pass, open lines run to the live bar (" << bad << " bad)");
    }
    // [8] layout
    {   cobs::Engine E = eng(B, B.size()); E.marks.push_back(cob(B, i0, i1, ic, 1));
        cobs::View v; v.l = 0; v.t = 0; v.r = 3200; v.b = 800; v.first = 0; v.last = last;
        v.X = [](int j) { return 10.0 + j * 10.0; }; v.Y = [](double px) { return 400.0 - (px - 1.124) * 100000.0; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; };
        const std::vector<cobs::Prim> P = cobs::fibLayout(E, v);
        int lines = 0, labels = 0; bool colOk = true, posOk = true;
        for (const cobs::Prim& p : P) {
            if (p.line) { ++lines; if (p.y0 != p.y1 || p.x0 != (int)v.X(i0) || p.x0 < v.l || p.x1 > v.r) posOk = false; }
            else { ++labels; if (!p.tiny) posOk = false;
                const unsigned want = p.text == "Stop" ? cobs::C_BEAR : p.text == "50%" ? cobs::C_ENTRY : p.text == "BE" ? cobs::C_ONE : cobs::C_BULL; if (p.col != want) colOk = false; }
        }
        CHECK(lines == 6 && labels == 6 && colOk && posOk, "6 lines from the origin bar + 6 names: stop red, 50% blue, BE grey, targets green (" << lines << "/" << labels << ")");
        bool t1x = false; for (const cobs::Prim& p : P) if (p.line && p.y0 == (int)std::lround(v.Y(1.12195 + 1.272 * 0.00215)) && p.x1 == (int)v.X(at(B, "09:30"))) t1x = true;
        CHECK(t1x, "T1's line ends at its hit bar");
        bool t3x = false; for (const cobs::Prim& p : P) if (p.line && p.y0 == (int)std::lround(v.Y(1.12625)) && p.x1 == (int)v.X(last + 1)) t3x = true;
        CHECK(t3x, "T3's open line runs to the forming bar");
        v.first = at(B, "12:15"); const std::vector<cobs::Prim> Q = cobs::fibLayout(E, v); int ql = 0; for (const cobs::Prim& p : Q) if (p.line) ++ql;
        CHECK(ql == 2, "scrolled past 12:15: only the open lines (1.0, T3) are drawn (" << ql << ")");
        std::vector<cobs::Prim> both = cobs::fibLayout(E, v); const std::vector<cobs::Prim> lab = cobs::layout(E, v); both.insert(both.end(), lab.begin(), lab.end());
        CHECK(both.size() >= Q.size(), "fib + CoB label layout combine");
    }
    // [9] status
    {   cobs::Engine E = eng(B, B.size()); E.marks.push_back(cob(B, i0, i1, ic, 1));
        const std::string s = cobs::fibStatus(E);
        CHECK(s.find("FIB_RULES,1.5.0") == 0 && s.find("BULL,0=1.12195,1=1.1241,live,Stop ended,50% ended,BE open,1.272 hit,1.618 hit,2.0 open") != std::string::npos, "status: " << s);
        CHECK(std::string(cobs::VERSION) == "1.5.0", "version 1.5.0");
    }
    // [10] not drawn = no lines
    {   cobs::Engine E = eng(B, B.size()); cobs::Mark m = cob(B, i0, i1, ic, 1); m.show = false; E.marks.push_back(m); m.show = true; m.zOk = false; E.marks.push_back(m);
        cobs::View v; v.r = 3200; v.b = 800; v.last = last; v.X = [](int j) { return 10.0 + j * 10.0; }; v.Y = [](double px) { return 400.0 - (px - 1.124) * 100000.0; };
        CHECK(cobs::fibLayout(E, v).empty() && cobs::fibLines(E, E.marks[0]).seg.empty(), "LOG / ZZO-failed CoBs get no lines");
    }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
