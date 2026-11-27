// VolRangeLogic 1.19.0 - the cyan thrust dot ("t|TD|dir" from the price chart's marks file) in the middle of its Range bar; the Volume pane draws none
#include "../VolRangeLogic.h"
#include "../WyMarks.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int f = 0, p = 0; static void chk(bool c, const std::string& w) { std::printf("%s %s\n", c ? "PASS" : "FAIL", w.c_str()); c ? ++p : ++f; }
int main() {
    { std::map<long long, vrr::Mk> w; std::map<long long, int> ar, td; vrr::parseWy("WY|1|ES|180|0.44.0\n500|TD|1\n501|TD|-1\n502|TD|3\n503|Bk|1|PDH|1|\n", w, &ar, &td);
      chk(td.size() == 2 && td[500] == 1 && td[501] == -1, "TD lines read (a bad direction dropped)"); chk(w.count(503) == 1, "Bk still read"); }
    { std::map<long long, std::vector<wyi::M> > m; wyi::parseAll("WY|1|ES|180|0.44.0\n3847000000|TD|1\n", m); chk(m.size() == 1 && m.begin()->second[0].code == 'D', "WyMarks reads TD as code D"); }
    for (int mode = 0; mode < 2; ++mode) {
        std::ifstream in("es1127b.csv"); std::string ln; vrr::Engine E; E.mode = mode; E.tick = 0.25; E.spb = 180; std::vector<std::string> W;
        while (std::getline(in, ln)) { std::stringstream ss(ln); std::string c[6]; for (int k = 0; k < 6; ++k) std::getline(ss, c[k], ',');
            vrr::Bar b; b.t = (long long)W.size() * 180 + 1000; b.o = std::stod(c[1]); b.h = std::stod(c[2]); b.l = std::stod(c[3]); b.c = std::stod(c[4]); b.v = std::stod(c[5]); E.addBar(b, 0); W.push_back(c[0].substr(11, 5)); }
        const int n = (int)E.V.size(); int k = -1; for (int i = 0; i < n; ++i) if (W[(size_t)i] == "09:06") k = i;
        E.wyTd[E.T[(size_t)k]] = 1;
        vrr::View v; v.l = 0; v.r = 1100; v.t = 0; v.b = 66; v.bw = 8; v.textH = 11; v.first = n - 80; v.last = n - 1;
        v.X = [&](int i) { return 4.0 + (i - (n - 80)) * 10.0; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; }; v.readX = (int)v.X(n - 1) + 22;
        const std::vector<vrr::Prim> P = vrr::layout(E, v, nullptr, -1);
        const vrr::Prim* dot = nullptr; const vrr::Prim* bar = nullptr; const int xk = (int)std::lround(v.X(k));
        for (const auto& q : P) { if (q.oval) dot = &q; else if (q.fill && q.x0 <= xk && xk <= q.x1 && q.y1 == v.b) bar = &q; }
        if (mode == vrr::RANGE) { chk(dot && bar, "Range: a dot on the 9:09 bar");
            chk(dot && bar && dot->col == vrr::C_TD && std::abs((dot->y0 + dot->y1) / 2 - (bar->y0 + bar->y1) / 2) <= 1 && std::abs((dot->x0 + dot->x1) / 2 - xk) <= 1, "cyan, in the middle of the bar"); }
        else chk(!dot, "Volume: no dot");
    }
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", p, f); return f ? 1 : 0;
}
