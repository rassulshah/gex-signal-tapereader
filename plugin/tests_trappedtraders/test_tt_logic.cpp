// tests_trappedtraders: lsTrappedTraders 0.1.0 logic on REAL data (ES 10-26 his recording, NQ 10-22 Databento store) - parity with the
// Python reference (outputs/trapped_traders/code/tt_calc.py), no repaint across a restart after EVERY candle, levels + direction rule,
// ratio text, hover rows, trimming. Build: g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. test_tt_logic.cpp
#include "../TrappedTradersLogic.h"
#include "../TrappedTradersSettings.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

static int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (!(c)) { ++fails; std::cerr << "FAIL: " << msg << " (" << __LINE__ << ")\n"; } } while (0)

struct Fx { double tick = 0.25; int start = 0; std::vector<ttr::Bar> bars; std::vector<std::vector<ttr::Cell> > cells; std::vector<long long> g, r; std::vector<std::vector<long long> > trig; };
static Fx load(const std::string& p) {
    Fx F; std::ifstream f(p.c_str()); std::string ln;
    while (std::getline(f, ln)) {
        std::istringstream s(ln); std::string k; s >> k;
        if (k == "TICK") s >> F.tick; else if (k == "START") s >> F.start;
        else if (k == "B") { int i, sod; long long o, h, l, c, g, r; s >> i >> sod >> o >> h >> l >> c >> g >> r; ttr::Bar b; b.t = 3800000000LL + sod; b.sess = 20752; b.sod = sod; b.o = o; b.h = h; b.l = l; b.c = c;
                             F.bars.push_back(b); F.cells.push_back(std::vector<ttr::Cell>()); F.g.push_back(g); F.r.push_back(r); }
        else if (k == "C") { ttr::Cell c; s >> c.px >> c.bid >> c.ask; F.cells.back().push_back(c); }
        else if (k == "T") { long long a, b, c, d; s >> a >> b >> c >> d; F.trig.push_back({a, b, c, d}); }
    }
    return F;
}
static ttr::Engine run(const Fx& F, size_t upto, const std::vector<std::string>* record) {
    ttr::Engine E; if (record) E.loadRecord(*record);
    for (size_t i = 0; i < upto && i < F.bars.size(); ++i) E.feed(F.bars[i], F.cells[i], true);
    return E;
}
static void parity(const char* name, const std::string& path) {
    const Fx F = load(path);
    CHECK(F.bars.size() > 50, std::string(name) + " fixture loaded");
    ttr::Engine E = run(F, F.bars.size(), nullptr);
    int same = 0; for (size_t i = 0; i < F.bars.size(); ++i) same += (E.bars[i].g == F.g[i] && E.bars[i].r == F.r[i]);
    CHECK(same == (int)F.bars.size(), std::string(name) + " green / red equal the Python reference on every candle (" + std::to_string(same) + "/" + std::to_string(F.bars.size()) + ")");
    // triggers of extremes from START on (the Python reference starts there)
    std::vector<std::vector<long long> > got;
    for (size_t q = 0; q < E.trig.size(); ++q) {
        const ttr::Trigger& T = E.trig[q]; long long k = -1, j = -1;
        for (size_t i = 0; i < E.bars.size(); ++i) { if (E.bars[i].t == T.kT) k = (long long)i; if (E.bars[i].t == T.t) j = (long long)i; }
        if (k >= F.start) got.push_back({k, j, (long long)T.side, T.ext});
    }
    std::vector<std::vector<long long> > want = F.trig; std::sort(got.begin(), got.end()); std::sort(want.begin(), want.end());
    CHECK(got == want, std::string(name) + " triggers equal the Python reference (" + std::to_string(got.size()) + " vs " + std::to_string(F.trig.size()) + ")");
    if (got != want) { for (auto& v : got) std::cerr << "  C++ " << v[0] << " " << v[1] << " " << v[2] << "\n"; for (auto& v : want) std::cerr << "  PY  " << v[0] << " " << v[1] << " " << v[2] << "\n"; }
    // no repaint: a restart after EVERY candle (record of the first c candles reloaded) gives the identical trigger list, no duplicate lines
    std::vector<std::string> fullTTT; for (const std::string& s : E.newLines) if (s.compare(0, 4, "TTT|") == 0) fullTTT.push_back(s);
    int okRestart = 0;
    for (size_t c = 1; c < F.bars.size(); ++c) {
        ttr::Engine A = run(F, c, nullptr);
        std::vector<std::string> rec = A.newLines;
        ttr::Engine B = run(F, F.bars.size(), &rec);
        std::vector<std::string> lines = rec; for (const std::string& s : B.newLines) lines.push_back(s);
        std::vector<std::string> ttt; for (const std::string& s : lines) if (s.compare(0, 4, "TTT|") == 0) ttt.push_back(s);
        bool same2 = B.trig.size() == E.trig.size();
        for (size_t q = 0; same2 && q < B.trig.size(); ++q) same2 = B.trig[q].t == E.trig[q].t && B.trig[q].side == E.trig[q].side && B.trig[q].ext == E.trig[q].ext;
        okRestart += (same2 && ttt == fullTTT);
    }
    CHECK(okRestart == (int)F.bars.size() - 1, std::string(name) + " restart after every candle = one straight pass (" + std::to_string(okRestart) + ")");
    // closed-candle only: feeding candle by candle never changes an earlier trigger
    ttr::Engine P; std::vector<std::pair<long long, int> > seen; bool stable = true;
    for (size_t i = 0; i < F.bars.size(); ++i) {
        P.feed(F.bars[i], F.cells[i], true);
        for (size_t q = 0; q < seen.size(); ++q) stable = stable && q < P.trig.size() && P.trig[q].t == seen[q].first && P.trig[q].side == seen[q].second;
        seen.clear(); for (size_t q = 0; q < P.trig.size(); ++q) seen.push_back({P.trig[q].t, P.trig[q].side});
    }
    CHECK(stable, std::string(name) + " a decided trigger never changes as later candles arrive");
}
int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "fixtures";
    parity("ES 10-26", dir + "/es_1026.txt");
    parity("NQ 10-22", dir + "/nq_1022.txt");
    // green / red definition on a hand case: close 100; sells below 100 = green, buys above 100 = red, at the close = neither
    { std::vector<ttr::Cell> c = { {98, 10, 3}, {100, 7, 9}, {102, 4, 6} }; long long g, r; ttr::trapped(c, 100, g, r); CHECK(g == 10 && r == 6, "green = sells below the close, red = buys above it, the close itself = neither"); }
    // levels: a low at a level that price came DOWN into is at_level; the same low with price coming from below is not (direction rule)
    {
        auto mk = [](long long t, long long o, long long h, long long l, long long c) { ttr::Bar b; b.t = t; b.sess = 1; b.sod = 34200; b.o = o; b.h = h; b.l = l; b.c = c; return b; };
        auto cell = [](long long px, long long bid, long long ask) { ttr::Cell c; c.px = px; c.bid = bid; c.ask = ask; return c; };
        ttr::Engine E; ttr::Level L; L.name = "PDL"; L.px = 1000; E.levels.push_back(L); E.bandTicks = 8;
        long long t = 100; long long px = 1060;
        for (int i = 0; i < 8; ++i) { E.feed(mk(t, px, px + 4, px - 8, px - 6), {cell(px - 5, 5, 50)}, true); t += 180; px -= 8; }   // falling into PDL, red ahead (buys above the close)
        E.feed(mk(t, px, px + 2, 999, 1003), {cell(1000, 300, 10), cell(1005, 5, 20)}, true); t += 180;                          // the low at 999, sellers trapped below the close
        CHECK(!E.trig.empty() && E.trig.back().side == 1 && E.trig.back().atLevel && E.trig.back().level == "PDL", "a low at PDL approached from above = bullish trigger at the level");
        ttr::Engine U; U.levels = E.levels; U.bandTicks = 8; t = 100; px = 990;
        for (int i = 0; i < 4; ++i) { U.feed(mk(t, px, px + 3, px - 2, px + 2), {cell(px - 1, 20, 5)}, true); t += 180; px += 1; }
        bool anyLevel = false; for (auto& T : U.trig) anyLevel = anyLevel || T.atLevel; CHECK(!anyLevel, "price coming from below is never 'at the level' for a low (direction rule)");
    }
    CHECK(ttr::ratioText(2.04) == "2.0x" && ttr::ratioText(12) == "10x+" && ttr::ratioText(-1) == "?", "ratio text 2.0x / 10x+ / ?");
    { ttr::Bar b; b.g = 8333; b.r = 3887; b.norm = 5824; const std::vector<std::string> h = ttr::hoverRows(b, nullptr, "9:12 AM"); CHECK(h.size() >= 8 && h[0].find("Who is trapped?") == 0 && h[1].find("8,333") != std::string::npos, "hover rows: question first, the numbers, what / how / use / example"); }
    // a malformed record line is counted, never crashes; a valid one wins
    { ttr::Engine E; const int bad = E.loadRecord({ "TTREC,1", "TTT|x|y", "TTB|10|1|5|6|-1", "TTT|20|10|1|1|240|1000|1|PDL" }); CHECK(bad == 1 && E.rec.size() == 1 && E.recLastT == 20 && E.recBar.size() == 1, "record: 1 bad line counted, 1 bar + 1 trigger loaded"); }
    // trimming keeps the engine consistent
    { const Fx F = load(dir + "/nq_1022.txt"); ttr::Engine E; E.maxBars = 40; for (size_t i = 0; i < F.bars.size(); ++i) E.feed(F.bars[i], F.cells[i], true); CHECK(E.bars.size() <= 40 && !E.bars.empty(), "trim keeps the last bars only"); }
    // settings table: 6 rows, real controls, defaults (0.1.1) everywhere / ratio on / box on / green / red
    { const ss::Spec& S = ttrset::spec(); const ss::Values V = ss::defaults(S, 0); CHECK(S.n == 6 && V.i(ttrset::K_MARKS) == ttrset::MARKS_ALL && V.b(ttrset::K_SHOW_RATIO) && V.b(ttrset::K_SHOW_BOX) && V.c(ttrset::K_COL_BULL) == 0x22C55E && V.c(ttrset::K_COL_BEAR) == 0xEF4444, "settings defaults"); CHECK(std::string(S.version) == ttr::VERSION, "settings version = logic version"); }
    // no overlap: on any pane size the bars and their triangle + ratio stay under the title and the live box, and placed marks never touch
    {
        unsigned seed = 7; auto rnd = [&](int m) { seed = seed * 1103515245u + 12345u; return (int)((seed >> 8) % (unsigned)m); };
        int bad = 0, placedN = 0, boxShown = 0;
        for (int it = 0; it < 3000; ++it) {
            const int top = rnd(400), h = 60 + rnd(500), left = rnd(50);
            const ttr::Layout L = ttr::layout(left, top, top + h, 90 + rnd(60), rnd(2) == 0);
            boxShown += L.box;
            if (L.base - L.y0 < 20) continue;                              // the plugin draws nothing then
            const int head = L.box ? L.live.b : L.title.b;
            if (L.box && ttr::overlaps(L.title, L.live)) ++bad;
            ttr::Placer P; P.placed.push_back(L.title); if (L.box) P.placed.push_back(L.live);
            for (int m = 0; m < 60; ++m) {
                const double v = rnd(1000), mx = 1000.0;                         // worst case: the scale top equals the tallest bar
                const int yb = (int)std::lround((double)L.base - v / mx * (double)(L.base - L.y0));
                const int ty = yb - 9, xc = left + rnd(800);
                ttr::Box tri; tri.l = xc - 5; tri.r = xc + 5; tri.t = ty - 1; tri.b = ty + 7;
                ttr::Box lb; lb.l = xc - 12; lb.r = xc + 12; lb.t = ty - 15; lb.b = ty - 2;
                if (yb < L.y0 || lb.t <= head) ++bad;                           // a bar or its label reaching into the header
                if (P.tryPlace(tri, &lb)) ++placedN;
            }
            for (size_t a = 0; a < P.placed.size(); ++a) for (size_t b = a + 1; b < P.placed.size(); ++b) if (ttr::overlaps(P.placed[a], P.placed[b])) ++bad;
        }
        CHECK(bad == 0 && placedN > 1000 && boxShown > 500, "no overlap on 3,000 random panes (" + std::to_string(placedN) + " marks placed, 0 touching)");
        CHECK(!ttr::layout(0, 0, 120, 100, true).box && ttr::layout(0, 0, 200, 100, true).box, "live box only on a pane tall enough (160 px)");
    }
    std::cout << (checks - fails) << " / " << checks << " passed\n";
    return fails ? 1 : 0;
}
