// render_dump.cpp - the scene CandleAbsorbLogic.h draws for the ES fixture, bars first..last, as text (render_check.py draws it)
#include "../CandleAbsorbLogic.h"
#include <fstream>
#include <iostream>
static std::vector<std::string> split(const std::string& s) { std::vector<std::string> o; std::string c; for (char ch : s) { if (ch == ',') { o.push_back(c); c.clear(); } else c += ch; } o.push_back(c); return o; }
int main(int, char** argv) {
    std::ifstream f(argv[1]); const int first = atoi(argv[2]), last = atoi(argv[3]); std::string line; std::vector<cab::Bar> B; std::vector<std::vector<cab::DRow> > V;
    while (std::getline(f, line)) { if (line.empty() || line[0] == '#') continue; auto x = split(line);
        if (x[0] == "B") { cab::Bar b; b.t = atoll(x[2].c_str()); cab::toTicks(atof(x[3].c_str()), .25, b.o); cab::toTicks(atof(x[4].c_str()), .25, b.h); cab::toTicks(atof(x[5].c_str()), .25, b.l); cab::toTicks(atof(x[6].c_str()), .25, b.c); B.push_back(b); V.resize(B.size()); }
        else { cab::DRow r; cab::toTicks(atof(x[2].c_str()), .25, r.tick); r.dlt = atoll(x[4].c_str()); V[(size_t)atoi(x[1].c_str())].push_back(r); } }
    cab::Engine E; for (int i = 0; i < last && i < (int)B.size(); ++i) E.addBar(B[(size_t)i], V[(size_t)i]);   // bar `last` = forming
    long long lo = LLONG_MAX, hi = LLONG_MIN; for (int i = first; i <= last; ++i) { lo = std::min(lo, B[(size_t)i].l); hi = std::max(hi, B[(size_t)i].h); }
    const double W = 1500, H = 820, bw = (W - 80) / (last - first + 3);
    cab::View v; v.l = 0; v.t = 0; v.r = (int)W; v.b = (int)H; v.first = first; v.last = last; v.bw = bw;
    v.X = [&](int i) { return 40 + (i - first + 0.5) * bw; }; v.Y = [&](double tk) { return H - 20 - (tk - (lo - 8)) / (double)(hi - lo + 16) * (H - 40); };
    v.textW = [](const std::string& s) { return (int)s.size() * 8; };
    v.haveForming = true; v.formIdx = last; v.forming = B[(size_t)last]; v.formRead = cab::readBar(B[(size_t)last], V[(size_t)last]);
    const cab::Scene sc = cab::layout(E, v);
    for (int i = first; i <= last; i++) std::cout << "C " << v.X(i) << " " << v.Y(B[(size_t)i].o) << " " << v.Y(B[(size_t)i].h) << " " << v.Y(B[(size_t)i].l) << " " << v.Y(B[(size_t)i].c) << " " << bw * 0.6 << " " << cab::hm(B[(size_t)i].t) << "\n";
    for (auto& q : sc.p) std::cout << "P " << (int)q.k << " " << q.x0 << " " << q.y0 << " " << q.x1 << " " << q.y1 << " " << q.col << " " << q.text << "\n";
    std::cerr << "dashes " << sc.dashes << " marks " << sc.marksDrawn << "\n";
}
