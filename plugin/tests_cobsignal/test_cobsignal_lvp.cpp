// test_cobsignal_lvp.cpp - lsCobSignal 1.3.0 LVP (low-volume pullback) logic suite, SDK-free.
//   (in plugin\tests_cobsignal) g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_cobsignal_lvp.cpp -o tl && ./tl lvp_gc_1103.csv
// Covers: the GC 3-min Nov 3 replay of the approved mockup (short CoB 8:21 start = 8:24 bar END; LVPs on the 8:24-8:33 and 8:48-8:54 pullbacks,
// the 9:25 pullback leaves the CoB range); the same bars mirrored = a long CoB with green LVPs below; a pullback leaving the range stops the
// count; at most 3 pullbacks; nothing decided before the pullback wave is confirmed; bar-by-bar replay with a restart after every bar =
// one pass (no repaint, no mark ever changes); the ZigZag is zzd::Engine::step with COMBO_REV_BARS; layout row 3 out, red above / green
// below, no overlap with a CoB label; record / status lines; the full Engine wires CoBs into the LVP look.
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
static long long hm(const std::vector<cobs::Bar>& B, const char* hhmm) {   // the Nov 3 bar whose END is hh:mm local
    const int h = std::atoi(hhmm), m = std::atoi(hhmm + 3);
    for (size_t i = 0; i < B.size(); ++i) { const long long s = (B[i].t % 86400 + 86400) % 86400; if (B[i].t / 86400 == B.back().t / 86400 && s == h * 3600 + m * 60) return (long long)i; }
    return -1;
}
static std::string sig(const cobs::Lvp& L) { std::string s; for (const cobs::LvpMark& m : L.marks) s += cobs::lvpLine(m) + "\n"; return s; }
static cobs::Bar mirror(cobs::Bar b) { const double h = b.h, l = b.l; b.h = -l; b.l = -h; b.o = -b.o; b.c = -b.c; return b; }
static cobs::Bar mk(long long t, double h, double l, double v) { cobs::Bar b; b.t = t; b.h = h; b.l = l; b.o = (h + l) / 2; b.c = (h + l) / 2; b.v = v; return b; }

// one pass with a CoB injected before bar cobIdx (legEnd = cobIdx - 1), as Engine::addBar does
static cobs::Lvp run(const std::vector<cobs::Bar>& B, int cobIdx, int side, size_t upto, double tick = 0.1) {
    cobs::Lvp L; L.tick = tick;
    for (size_t i = 0; i < upto && i < B.size(); ++i) { if ((int)i == cobIdx) L.cob(B[i].t, cobIdx, cobIdx - 1, side, true); L.bar(B[i]); }
    return L;
}

int main(int argc, char** argv) {
    const std::vector<cobs::Bar> G = load(argc > 1 ? argv[1] : "lvp_gc_1103.csv");
    CHECK(G.size() > 500, "GC fixture loaded (" << G.size() << " bars)");
    CHECK(cobs::LVP_REV_BARS == zzd::COMBO_REV_BARS && cobs::LVP_REV_BARS == 3, "LVP ZigZag = zzd COMBO_REV_BARS (3-bar High / Low)");
    const int icob = (int)hm(G, "08:24");               // the mockup's "CoB short 8:21 AM" (bar START) = IRT bar END 8:24
    CHECK(icob > 10, "CoB bar found");

    // ---- 1) GC Nov 3 replay of the mockup: short CoB -> red LVPs on 8:24-8:33 and 8:48-8:54 (bar START labels = END 8:27-8:36, 8:51-8:57)
    const cobs::Lvp L = run(G, icob, -1, G.size());
    CHECK(L.marks.size() == 2, "GC: 2 LVPs after the 8:21 short CoB (" << L.marks.size() << ")");
    if (L.marks.size() == 2) {
        const cobs::LvpMark& a = L.marks[0]; const cobs::LvpMark& b = L.marks[1];
        CHECK(a.idx == hm(G, "08:36") && a.s == hm(G, "08:27") + 1 && a.vol == 3575 && a.cobVol == 16402 && a.nth == 1 && a.side == -1, "GC LVP 1 = the 8:24-8:33 pullback, 3,575 vs 16,402 (" << cobs::lvpLine(a) << ")");
        CHECK(b.idx == hm(G, "08:57") && b.s == hm(G, "08:51") + 1 && b.vol == 1151 && b.nth == 2, "GC LVP 2 = the 8:48-8:54 pullback, 1,151 (" << cobs::lvpLine(b) << ")");
        CHECK(a.decT == G[(size_t)hm(G, "08:48")].t && b.decT == G[(size_t)hm(G, "08:58")].t, "decided on the bars that confirmed each wave (8:45 / 8:55 start)");
    }
    CHECK(L.tr.size() == 1 && L.tr[0].state == 2 && L.tr[0].n == 2 && L.tr[0].why.find("left") != std::string::npos, "GC: the 3rd pullback (9:25) leaves the CoB range and ends the look (" << (L.tr.empty() ? "" : L.tr[0].why) << ")");

    // ---- 2) no LVP before the pullback wave is confirmed
    {   const int conf1 = (int)hm(G, "08:48");
        CHECK(run(G, icob, -1, (size_t)conf1).marks.empty(), "nothing on the bars before the 8:24-8:33 wave is confirmed");
        CHECK(run(G, icob, -1, (size_t)conf1 + 1).marks.size() == 1, "LVP 1 appears on the confirming bar's close");
        CHECK(run(G, icob, -1, (size_t)hm(G, "08:58")).marks.size() == 1 && run(G, icob, -1, (size_t)hm(G, "08:58") + 1).marks.size() == 2, "LVP 2 appears only once its wave is confirmed"); }

    // ---- 3) no repaint: bar-by-bar, every earlier mark unchanged; a restart (fresh pass) after every bar = the incremental state
    {   cobs::Lvp I; I.tick = 0.1; std::string prev; bool prefix = true, restart = true;
        for (size_t i = 0; i < G.size(); ++i) {
            if ((int)i == icob) I.cob(G[i].t, icob, icob - 1, -1, true);
            I.bar(G[i]); const std::string s = sig(I);
            prefix = prefix && s.compare(0, prev.size(), prev) == 0; prev = s;
            if (i % 7 == 0 || i + 1 == G.size()) restart = restart && sig(run(G, icob, -1, i + 1)) == s;
        }
        CHECK(prefix, "no repaint: a decided LVP never changes or disappears as bars arrive");
        CHECK(restart, "a restart after any bar gives the same LVPs (byte-identical)");
        CHECK(sig(I) == sig(L), "bar-by-bar = one pass"); }

    // ---- 4) long CoB = the mirror: green LVPs below the same (mirrored) pullback waves
    {   std::vector<cobs::Bar> M; for (const cobs::Bar& b : G) M.push_back(mirror(b));
        const cobs::Lvp R = run(M, icob, +1, M.size());
        bool same = R.marks.size() == L.marks.size(); for (size_t k = 0; same && k < R.marks.size(); ++k) same = R.marks[k].idx == L.marks[k].idx && R.marks[k].vol == L.marks[k].vol && R.marks[k].side == +1;
        CHECK(same, "long CoB (mirrored bars): the same pullbacks, side +1 (" << R.marks.size() << ")");
        cobs::Engine E; E.setMarket("GC", 0.1, 180); for (const cobs::Bar& b : M) E.B.push_back(b); E.lvp = R;
        cobs::View v; v.first = 0; v.last = (int)M.size() - 1; v.t = -1000000; v.b = 1000000; v.X = [](int i) { return (double)i * 10; }; v.Y = [](double p) { return -p * 4; };
        const auto P = cobs::layout(E, v); bool ok = P.size() == R.marks.size();
        for (size_t k = 0; ok && k < P.size(); ++k) { const cobs::Bar& b = M[(size_t)R.marks[k].idx]; ok = P[k].text == "LVP" && P[k].col == cobs::C_BULL && P[k].y0 == (int)std::lround(v.Y(b.l)) + cobs::LVP_OFF; }
        CHECK(ok, "long CoB: green LVP below the pivot low, 51 px out"); }

    // ---- 5) short CoB layout: red above, row 3 out (clear of the wave-time / A / Pb rows), never on a CoB label
    {   cobs::Engine E; E.setMarket("GC", 0.1, 180); for (const cobs::Bar& b : G) E.B.push_back(b); E.lvp = L;
        cobs::View v; v.first = 0; v.last = (int)G.size() - 1; v.t = -1000000; v.b = 1000000; v.X = [](int i) { return (double)i * 10; }; v.Y = [](double p) { return -p * 4; };
        auto P = cobs::layout(E, v); bool ok = P.size() == 2;
        for (size_t k = 0; ok && k < P.size(); ++k) { const cobs::Bar& b = G[(size_t)L.marks[k].idx]; ok = P[k].col == cobs::C_BEAR && P[k].y1 == (int)std::lround(v.Y(b.h)) - 51 && P[k].y1 <= (int)std::lround(v.Y(b.h)) - 3 - 3 * 16; }
        CHECK(ok, "short CoB: red LVP above the pivot high, bottom 51 px out (rows 0-2 = wave-time, A, Pb stay clear)");
        // a bear CoB label on the same bar at its GAP: the LVP stays clear of it
        cobs::Mark m; m.idx = L.marks[0].idx; m.side = -1; m.t = G[(size_t)m.idx].t; m.legStart = 0; m.legEnd = 0; E.marks.push_back(m);
        P = cobs::layout(E, v); bool clear = P.size() == 3;
        for (size_t a = 0; clear && a < P.size(); ++a) for (size_t b = a + 1; b < P.size(); ++b) clear = clear && !(P[a].x0 < P[b].x1 && P[b].x0 < P[a].x1 && P[a].y0 < P[b].y1 && P[b].y0 < P[a].y1);
        CHECK(clear, "LVP never overlaps a CoB label");
        // a LOG CoB's LVP is recorded but not drawn
        E.marks.clear(); E.lvp.marks[0].show = false; CHECK(cobs::layout(E, v).size() == 1, "an LVP of a recorded-only (LOG) CoB is not drawn");
        v.t = (int)std::lround(v.Y(G[(size_t)L.marks[1].idx].h)) - 20; CHECK(cobs::layout(E, v).empty(), "an LVP outside the pane is not drawn"); }

    // ---- 6) synthetic: leaving the range stops the count; at most 3 pullbacks; volume rule <= 1/3
    {   // bear CoB wave 100 -> 80 (5 bars), then up / down pullbacks. Each up wave: 2 bars up, then 4 bars down for the 3-bar turn.
        std::vector<cobs::Bar> B; long long t = 1000000;
        auto add = [&](double h, double l, double v) { B.push_back(mk(t += 180, h, l, v)); };
        for (int k = 0; k < 4; ++k) add(90 + k, 89 + k, 100);          // rising into the high (up wave)
        add(100, 93, 100);                                             // the high (pivot)
        add(99, 95, 1000); add(97, 92, 1000); add(94, 88, 1000); add(90, 84, 1000); add(86, 80, 1000);   // the CoB wave down (5000)
        const int cobBar = (int)B.size();                              // CoB bar: no new low
        auto pull = [&](double top, double v, double bottom) {         // an up wave to `top`, then a down wave to `bottom`
            add(top - 3, 83, v / 2); add(top, top - 4, v / 2);          // up 2 bars (high = top; the first low stays above the bottom)
            add(top - 1, top - 6, 100); add(top - 2, bottom + 2, 100); add(top - 3, bottom, 100); add(top - 4, bottom + 1, 100);   // turns down
        };
        add(83, 81, 400);                                              // the CoB bar (low 81 > 80)
        pull(90, 1000, 82);     // pullback 1: 1000 + CoB bar 400 ... (counted from the bar after the low)
        pull(92, 3000, 82);     // pullback 2: heavy (not LVP)
        pull(91, 600, 82);      // pullback 3: light
        pull(89, 500, 82);      // pullback 4: light but past the 3-wave limit
        for (int k = 0; k < 5; ++k) add(84 - k, 82 - k, 100);          // ends it
        const cobs::Lvp S = run(B, cobBar, -1, B.size(), 0.25);
        CHECK(S.tr.size() == 1 && S.tr[0].vol == 5000 && S.tr[0].n == 3 && S.tr[0].state == 2, "synthetic: CoB wave volume 5,000; exactly 3 pullbacks counted (" << (S.tr.empty() ? 0 : S.tr[0].n) << ", " << (S.tr.empty() ? 0 : S.tr[0].vol) << ")");
        int fourth = 0; for (const cobs::LvpMark& m : S.marks) fourth += m.nth > 3;
        CHECK(fourth == 0 && S.marks.size() == 2, "synthetic: LVP on pullbacks 1 and 3 only - none on the heavy 2nd, none past the 3rd (" << S.marks.size() << ")");
        bool rule = true; for (const cobs::LvpMark& m : S.marks) rule = rule && m.vol * 3 <= m.cobVol; CHECK(rule, "every LVP: pullback volume <= CoB wave volume / 3");
        // the same with pullback 2 rising ABOVE the CoB wave's high (100): the look stops there, pullback 3 never counts
        std::vector<cobs::Bar> X; t = 1000000; B.swap(X); B.clear();
        for (int k = 0; k < 4; ++k) add(90 + k, 89 + k, 100);
        add(100, 93, 100); add(99, 95, 1000); add(97, 92, 1000); add(94, 88, 1000); add(90, 84, 1000); add(86, 80, 1000);
        add(83, 81, 400); pull(90, 1000, 82); pull(101, 300, 82); pull(91, 600, 82); for (int k = 0; k < 5; ++k) add(84 - k, 82 - k, 100);
        const cobs::Lvp Q = run(B, cobBar, -1, B.size(), 0.25);
        CHECK(Q.marks.size() == 1 && Q.marks[0].nth == 1 && Q.tr[0].why.find("pullback 2 left") != std::string::npos, "a pullback leaving the CoB range (above its high) ends the look: no LVP on it or after (" << Q.marks.size() << ", " << (Q.tr.empty() ? "" : Q.tr[0].why) << ")");
        // exactly 1/3 counts, a hair over does not
        cobs::Lvp E1; E1.tick = 0.25; cobs::LvpTrack x; x.side = -1; x.state = 1; x.cw = 0; x.hi = 1000; x.lo = 0; x.vol = 3000; E1.tr.push_back(x);
        E1.Z.resize(10); cobs::LvpWave w0; w0.dir = -1; E1.waves.push_back(w0);
        cobs::LvpWave w1; w1.s = 1; w1.e = 3; w1.dir = 1; w1.hi = 500; w1.lo = 10; w1.vol = 1000; E1.waves.push_back(w1); E1.onWave(1, 5);
        cobs::LvpWave w2 = w1; w2.s = 5; w2.e = 7; w2.vol = 1001; E1.waves.push_back(w2); E1.onWave(2, 8);
        CHECK(E1.marks.size() == 1 && E1.marks[0].vol == 1000, "volume rule: 1,000 vs 3,000 = LVP, 1,001 = not"); }

    // ---- 7) record / status lines, the Engine wiring (a CoB from the Engine starts the look; reset clears it)
    {   const std::string l = cobs::lvpLine(L.marks.empty() ? cobs::LvpMark() : L.marks[0]);
        CHECK(l.rfind("LVP,", 0) == 0 && l.find(",BEAR,") != std::string::npos && l.find(",3575,16402,21.8,1,SHOW") != std::string::npos, "record line (" << l << ")");
        cobs::Engine E; E.setMarket("GC", 0.1, 180);
        for (size_t i = 0; i < G.size(); ++i) { if ((int)i == icob) { cobs::Mark m; m.t = G[i].t; m.idx = icob; m.side = -1; m.legStart = icob - 5; m.legEnd = icob - 1; E.lvp.cob(m.t, m.idx, m.legEnd, m.side, true); } E.addBar(G[i]); }
        CHECK(E.lvp.marks.size() >= 2 && E.lvpOut.size() == E.lvp.marks.size() && E.lvp.tick == 0.1, "Engine: every LVP queued once for the record (" << E.lvpOut.size() << ")");
        bool gcIn = false; for (const cobs::LvpMark& m : E.lvp.marks) gcIn = gcIn || (m.idx == L.marks[0].idx && m.vol == 3575); CHECK(gcIn, "Engine: the GC LVP through Engine::addBar");
        const std::string st = cobs::statusText(E, "GCZ26", 180, "x");
        CHECK(st.find("\nLVP_RULES,3-bar") != std::string::npos && st.find("\nLVPS,") != std::string::npos && st.find("\nLASTLVP,") != std::string::npos && st.find("\nLVPLOOK,") != std::string::npos, "status: LVP rules, count, last LVPs, looks");
        E.reset(); CHECK(E.lvp.marks.empty() && E.lvp.tr.empty() && E.lvp.Z.empty() && E.lvpOut.empty(), "reset clears the LVP state"); }

    std::cout << "test_cobsignal_lvp: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
