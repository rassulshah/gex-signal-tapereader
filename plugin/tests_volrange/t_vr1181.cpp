// VolRangeLogic 1.18.1 - "the box should always be on the right": no room right of the last candle -> the small box at the pane's RIGHT edge, no bar under it
#include "../VolRangeLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int f = 0, p = 0; static void chk(bool c, const std::string& w) { std::printf("%s %s\n", c ? "PASS" : "FAIL", w.c_str()); c ? ++p : ++f; }
int main() {
    for (int mode = 0; mode < 2; ++mode) {
        std::ifstream in("es1127b.csv"); std::string ln; vrr::Engine E; E.mode = mode; E.tick = 0.25; E.spb = 180;
        while (std::getline(in, ln)) { std::stringstream ss(ln); std::string c[6]; for (int k = 0; k < 6; ++k) std::getline(ss, c[k], ',');
            vrr::Bar b; b.o = std::stod(c[1]); b.h = std::stod(c[2]); b.l = std::stod(c[3]); b.c = std::stod(c[4]); b.v = std::stod(c[5]); E.addBar(b, 0); }
        const int n = (int)E.V.size();
        // the chart filled with candles to the right edge (scrolled back): readX right of the last visible candle is past the pane
        vrr::View v; v.l = 0; v.r = 800; v.t = 0; v.b = 66; v.bw = 8; v.textH = 11; v.first = n - 120; v.last = n - 41;
        v.X = [&](int i) { return 4.0 + (i - (n - 120)) * 10.0; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; }; v.readX = (int)v.X(n - 41) + 4 + 18;
        const std::vector<vrr::Prim> P = vrr::layout(E, v, nullptr, -1);
        const vrr::Prim* bg = nullptr; for (const auto& q : P) if (q.fill && q.col == vrr::C_BOXBG) bg = &q;
        chk(bg && bg->x1 <= v.r && bg->x0 >= v.r - vrr::BOX_W - 3, std::string(mode == vrr::VOL ? "Volume" : "Range") + ": the box at the RIGHT edge (" + (bg ? std::to_string(bg->x0) + ".." + std::to_string(bg->x1) : "none") + ")");
        bool under = false; for (const auto& q : P) if (bg && q.fill && q.col != vrr::C_BOXBG && q.col != vrr::C_BOXLN && q.x1 > bg->x0 && q.x0 < bg->x1 && q.y1 > bg->y0 && q.y0 < bg->y1) under = true;
        chk(!under, "no bar drawn under the box");
        bool thr = false; for (const auto& q : P) if (q.text == (mode == vrr::VOL ? "VolTH" : "Thrust")) thr = true; chk(thr, "the box carries the Thrust / VolTH row");
        // room right of the last visible candle (scrolled back, the right margin empty): the read goes there, right of that candle
        vrr::View w = v; w.r = 1100; w.readX = (int)w.X(n - 41) + 4 + 18;
        const std::vector<vrr::Prim> Q = vrr::layout(E, w, nullptr, -1);
        int minX = 1 << 30; for (const auto& q : Q) if (!q.text.empty() && (q.text == "Thrust" || q.text == "VolTH")) minX = std::min(minX, q.x0);
        chk(minX >= w.readX && minX < w.r, "scrolled back with room: the numbers sit right of the last visible candle (x " + std::to_string(minX) + ", readX " + std::to_string(w.readX) + ")");
    }
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", p, f); return f ? 1 : 0;
}
