// render_lvp_dump.cpp - dumps bars + the labels cobs::layout draws (GC Nov 3, short CoB 8:24 END) for render_lvp.py (render check only)
#include "CobSignalLogic.h"
#include <fstream>
#include <iostream>
#include <sstream>
int main(int argc, char** argv) {
    std::vector<cobs::Bar> G; std::ifstream f(argc > 1 ? argv[1] : "lvp_gc_1103.csv"); std::string l;
    while (std::getline(f, l)) { if (l.empty() || l[0] == '#') continue; std::stringstream ss(l); std::string t; std::vector<std::string> x; while (std::getline(ss, t, ',')) x.push_back(t);
        cobs::Bar b; b.t = std::atoll(x[0].c_str()); b.o = std::atof(x[1].c_str()); b.h = std::atof(x[2].c_str()); b.l = std::atof(x[3].c_str()); b.c = std::atof(x[4].c_str()); b.v = std::atof(x[5].c_str()); G.push_back(b); }
    int icob = -1; for (size_t i = 0; i < G.size(); ++i) if ((G[i].t % 86400) == 8 * 3600 + 24 * 60 && G[i].t / 86400 == G.back().t / 86400) icob = (int)i;
    cobs::Engine E; E.setMarket("GC", 0.1, 180);
    for (size_t i = 0; i < G.size(); ++i) { if ((int)i == icob) { cobs::Mark m; m.t = G[i].t; m.idx = icob; m.side = -1; m.legStart = icob - 9; m.legEnd = icob - 1; E.marks.push_back(m); E.lvp.cob(m.t, m.idx, m.legEnd, m.side, true); } E.addBar(G[i]); }
    int a0 = icob - 25, a1 = icob + 55; double lo = 1e9, hi = -1e9; for (int i = a0; i <= a1; ++i) { lo = std::min(lo, G[(size_t)i].l); hi = std::max(hi, G[(size_t)i].h); }
    lo -= 8; hi += 8; const int W = 1100, H = 560;
    cobs::View v; v.l = 0; v.t = 0; v.r = W; v.b = H; v.first = a0; v.last = a1;
    v.X = [&](int i) { return 20 + (i - a0 + 0.5) * (W - 40.0) / (a1 - a0 + 1); }; v.Y = [&](double p) { return (hi - p) / (hi - lo) * H; };
    v.textW = [](const std::string& s) { return (int)s.size() * 8; };
    for (int i = a0; i <= a1; ++i) { const cobs::Bar& b = G[(size_t)i]; std::cout << "B," << v.X(i) << "," << v.Y(b.o) << "," << v.Y(b.h) << "," << v.Y(b.l) << "," << v.Y(b.c) << "\n"; }
    for (const cobs::Prim& p : cobs::layout(E, v)) std::cout << "T," << p.x0 << "," << p.y0 << "," << p.x1 << "," << p.y1 << "," << p.col << "," << p.text << "\n";
}
