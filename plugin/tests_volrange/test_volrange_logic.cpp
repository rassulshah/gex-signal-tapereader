// test_volrange_logic.cpp - lsVolRead / lsRangeRead 1.0.0 logic suite (SDK-free).
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_volrange_logic.cpp -o tv && ./tv ../tests_cobsignal/cobsignal_es.csv [dump.txt]
#include "VolRangeLogic.h"
#include <fstream>
#include <iostream>
static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
static vrr::Bar mk(double o, double c, double v, double h = 0, double l = 0) { vrr::Bar b; b.o = o; b.c = c; b.v = v; b.h = h ? h : std::max(o, c); b.l = l ? l : std::min(o, c); return b; }
int main(int argc, char** argv) {
    {   vrr::Engine E;   // volumes: G5 R3 G4 R9 G2 G6 R1 G8 ; the last bar G10
        const double V[] = { 5, 3, 4, 9, 2, 6, 1, 8 }; const bool G[] = { true, false, true, false, true, true, false, true };
        for (int i = 0; i < 8; ++i) E.addBar(mk(10, G[i] ? 11 : 9, V[i]));
        CHECK(E.S[7].all == 3 && E.S[7].same == 4 && E.S[7].green, "G8: 3 bars in a row smaller (1, 6, 2 - stops at 9); all 4 green bars before it are smaller (" << E.S[7].all << ", " << E.S[7].same << ")");
        E.addBar(mk(10, 11, 10)); CHECK(E.S[8].all == 8 && E.S[8].same == 5 && E.S[8].mx() == 8, "G10 beats all 8 bars and all 5 green bars (" << E.S[8].all << ", " << E.S[8].same << ")");
        E.addBar(mk(10, 9, 9)); CHECK(E.S[9].all == 0 && E.S[9].same == 1, "R9 after G10: 0 in a row; red bars: 1 is smaller, the earlier R9 is equal and stops the count (" << E.S[9].all << ", " << E.S[9].same << ")");
        E.addBar(mk(10, 9, 10)); CHECK(E.S[10].all == 1 && E.S[10].same == 4, "an equal bar stops the count (G10 = 10): all 1, red 4 (" << E.S[10].all << ", " << E.S[10].same << ")");
        E.addBar(mk(10, 10, 0)); CHECK(E.S[11].mx() == 0 && E.S[11].green, "a zero bar counts nothing; close = open is green");
        const vrr::Stat f = E.forming(mk(10, 11, 99)); CHECK(f.all == 12 && E.S.size() == 12, "the forming bar is measured, never stored");
    }
    {   vrr::Engine E; for (int i = 0; i < 300; ++i) E.addBar(mk(10, 11, 1 + (i % 7))); E.addBar(mk(10, 11, 50)); CHECK(E.S.back().all == vrr::CAP && E.S.back().same == vrr::CAP && vrr::fm(E.S.back().mx()) == "99+", "counts stop at 99 -> 99+"); CHECK(vrr::fm(98) == "98", "98 prints as 98"); }
    {   vrr::Engine E; E.mode = vrr::RANGE; E.tick = 0.25;
        E.addBar(mk(10, 11, 1, 11.0, 10.0)); E.addBar(mk(10, 9, 1, 10.5, 9.0)); E.addBar(mk(10, 11, 1, 11.5, 10.0 - 1e-9));
        CHECK(E.V[0] == 4 && E.V[1] == 6 && E.V[2] == 6, "range in whole ticks (" << E.V[0] << ", " << E.V[1] << ", " << E.V[2] << ")"); CHECK(E.S[2].all == 0 && E.S[2].same == 1, "equal ranges do not count (" << E.S[2].all << ", " << E.S[2].same << ")"); }
    if (argc < 2) { std::cerr << "fixture path needed\n"; return 2; }
    std::vector<vrr::Bar> B; { std::ifstream f(argv[1]); std::string l; std::getline(f, l); while (std::getline(f, l)) { vrr::Bar b; if (std::sscanf(l.c_str(), "%lld,%lf,%lf,%lf,%lf,%lf", &b.t, &b.o, &b.h, &b.l, &b.c, &b.v) == 6) B.push_back(b); } }
    CHECK(B.size() > 5000, "fixture loaded (" << B.size() << ")");
    for (int mode = 0; mode < 2; ++mode) {
        vrr::Engine E; E.mode = mode; E.tick = 0.25; for (const vrr::Bar& b : B) E.addBar(b);
        bool same = true; int big = 0;   // brute force
        for (int i = 0; i < (int)B.size(); i += 7) { const double x = E.V[(size_t)i]; const bool g = E.G[(size_t)i] != 0; int a = 0, s = 0;
            for (int j = i - 1; j >= 0 && E.V[(size_t)j] < x && a < vrr::CAP; --j) ++a;
            for (int j = i - 1; j >= 0 && s < vrr::CAP; --j) { if ((E.G[(size_t)j] != 0) != g) continue; if (E.V[(size_t)j] >= x) break; ++s; }
            if (!(x > 0)) a = s = 0;
            same = same && a == E.S[(size_t)i].all && s == E.S[(size_t)i].same; }
        for (const vrr::Stat& s : E.S) if (s.mx() >= vrr::SHOW_MIN) ++big;
        CHECK(same, (mode ? "Range" : "Volume") << ": counts = brute force on the ES fixture"); CHECK(big > 20 && big < (int)B.size() / 4, (mode ? "Range" : "Volume") << ": a sensible share of bars is numbered (" << big << " of " << B.size() << ")");
        {   vrr::Engine P; P.mode = mode; P.tick = 0.25; const size_t cut = B.size() / 2; for (size_t i = 0; i < cut; ++i) P.addBar(B[i]); bool pre = true; for (size_t i = 0; i < cut; ++i) pre = pre && P.S[i].all == E.S[i].all && P.S[i].same == E.S[i].same; CHECK(pre, "no repaint: a prefix run = the full run's prefix"); }
        vrr::View v; v.l = 0; v.t = 0; v.r = 1400; v.b = 90; v.bw = 5; v.last = (int)E.V.size(); v.first = v.last - 220; const int f0 = v.first; v.X = [f0](int j) { return 110.0 + (j - f0) * 5.8; };
        const vrr::Bar form = B.back(); const std::vector<vrr::Prim> P = vrr::layout(E, v, &form);
        int bars = 0, nums = 0, boxTxt = 0; bool in = true, order = true; std::vector<std::string> keys; std::string title;
        for (const vrr::Prim& p : P) { in = in && p.x0 >= v.l && p.x1 <= v.r && p.y0 >= v.t && p.y1 <= v.b; if (p.fill) { if (p.col == vrr::C_UP || p.col == vrr::C_DN) ++bars; } else if (p.x1 <= 2 + vrr::BOX_W) { ++boxTxt; if (p.text.rfind("B", 0) == 0) keys.push_back(p.text); else if (p.text.find(" v") != std::string::npos) title = p.text; } else { ++nums; order = order && (p.text == "99+" || std::atoi(p.text.c_str()) >= vrr::SHOW_MIN) && p.x0 > 2 + vrr::BOX_W; } }
        const std::string pre = mode ? "BR" : "BV";
        { bool clear = true; for (size_t i = 0; i < P.size(); ++i) for (size_t k = i + 1; k < P.size(); ++k) if (!P[i].fill && !P[k].fill && P[i].x0 > 2 + vrr::BOX_W && P[k].x0 > 2 + vrr::BOX_W && P[i].x0 < P[k].x1 && P[k].x0 < P[i].x1 && P[i].y0 < P[k].y1 && P[k].y0 < P[i].y1) clear = false; CHECK(clear, "no number overlaps another"); }
        CHECK(in, "everything inside the pane"); CHECK(bars > 150, "bars drawn (" << bars << ")"); CHECK(nums > 0 && order, "numbers only for Max 20+, right of the box (" << nums << ")");
        CHECK(keys.size() == 4 && keys[0] == pre + "Max" && keys[1] == pre && keys[2] == pre + "G" && keys[3] == pre + "R" && boxTxt == 9, "box rows in his order: Max, " << pre << ", " << pre << "G, " << pre << "R");
        CHECK(title == std::string(mode ? "Range v" : "Volume v") + vrr::VERSION, "title with the version (" << title << ")");
        { int dash = 0; for (const vrr::Prim& p : P) if (!p.fill && p.text == "-") ++dash; CHECK(dash == 1, "the other colour's row shows -"); }
        vrr::View s2 = v; s2.r = 120; CHECK([&] { for (const vrr::Prim& p : vrr::layout(E, s2, &form)) if (!p.fill && p.text.rfind("B", 0) == 0) return false; return true; }(), "a narrow pane draws no box");
        vrr::View s3 = v; s3.b = 8; CHECK(vrr::layout(E, s3, &form).empty(), "a pane under 12 px draws nothing");
        vrr::Engine Z; Z.mode = mode; CHECK([&] { for (const vrr::Prim& p : vrr::layout(Z, v, nullptr)) if (p.fill && (p.col == vrr::C_UP || p.col == vrr::C_DN)) return false; return true; }(), "no bars: only the empty box");
        const std::string st = vrr::statusText(E, "ES", "ESZ6", 180); CHECK(st.rfind(std::string("VERSION,") + vrr::VERSION + "\nPANE," + (mode ? "Range" : "Volume"), 0) == 0 && st.find("\nLAST,") != std::string::npos, "status text");
        if (argc > 2) { std::ofstream d(std::string(argv[2]) + (mode ? ".range" : ".vol")); for (const vrr::Prim& p : P) d << (p.fill ? "F" : p.bold ? "B" : "T") << "," << p.x0 << "," << p.y0 << "," << p.x1 << "," << p.y1 << "," << p.col << "," << p.text << "\n"; }
    }
    std::cout << "test_volrange_logic: " << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
