// render_dump_delta.cpp (1.5.0; 1.9.0 + the BO / BD candles from bko::Engine) - the scene WavePOCLogic.h draws for a fixture with delta (B / V,i,price,vol,delta lines), bars first..last, as text
//   g++ -std=c++17 -I.. render_dump_delta.cpp -o rdd && ./rdd wavepoc_es1029_1030_delta.csv 117 246 > scene.txt && python3 render_check.py scene.txt out.png
#include "../WavePOCLogic.h"
#include "../BreakoutLogic.h"   // (1.9.0) lsBreakout's BO / BD candles for the HVN break level
#include <fstream>
#include <iostream>
int main(int argc, char** argv) {
    std::ifstream f(argv[1]); const int first = atoi(argv[2]), last = atoi(argv[3]); std::string line; std::vector<wpc::Bar> B; std::vector<std::vector<wpc::VRow> > V;
    while (std::getline(f, line)) { if (line.empty() || line[0] == '#') continue; auto x = wpc::splitCsv(line);
        if (x[0] == "B") { wpc::Bar b; b.t = atoll(x[2].c_str()); wpc::toTicks(atof(x[3].c_str()), .25, b.o); wpc::toTicks(atof(x[4].c_str()), .25, b.h); wpc::toTicks(atof(x[5].c_str()), .25, b.l); wpc::toTicks(atof(x[6].c_str()), .25, b.c); B.push_back(b); V.resize(B.size()); }
        else { wpc::VRow r; wpc::toTicks(atof(x[2].c_str()), .25, r.tick); r.vol = atoll(x[3].c_str()); r.dlt = atoll(x[4].c_str()); V[(size_t)atoi(x[1].c_str())].push_back(r); } }
    wpc::Store S; wpc::Engine E; E.store = &S;
    {   std::vector<bko::In> I; for (size_t i = 0; i < B.size(); ++i) { bko::In b; b.t = B[i].t; b.ok = true; b.days = (long)wpc::floorDiv(b.t, 86400); b.endSec = (int)(b.t - (long long)b.days * 86400);
            b.o = B[i].o * .25; b.h = B[i].h * .25; b.l = B[i].l * .25; b.c = B[i].c * .25; for (auto& r : V[i]) b.vol += r.vol; I.push_back(b); }
        bko::Engine K; K.update(I, (long)I.size(), I[0].t, .25, 180, 510, 900);
        for (const auto& kv : K.bo) if (kv.second.fired) for (size_t j = 0; j < I.size(); ++j) if (I[j].t == kv.second.t0) { wpc::BoCand c; c.t0 = kv.second.t0; c.dir = kv.second.dir; c.hi = B[j].h; c.lo = B[j].l; E.bos.push_back(c); } }
    wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = V[(size_t)i]; return 1; };
    for (int i = 0; i <= last && i < (int)B.size(); ++i) E.addBar(B[(size_t)i], vap);
    long long lo = LLONG_MAX, hi = LLONG_MIN; for (int i = first; i <= last; ++i) { lo = std::min(lo, B[(size_t)i].l); hi = std::max(hi, B[(size_t)i].h); }
    const double W = 1700, H = 900, bw = (W - 80) / (last - first + 3);
    wpc::View v; v.l = 0; v.t = 0; v.r = (int)W; v.b = (int)H; v.first = first; v.last = last; v.lastBar = last; v.bw = bw;
    v.X = [&](int i) { return 40 + (i - first + 0.5) * bw; }; v.Y = [&](double tk) { return H - 20 - (tk - (lo - 8)) / (double)(hi - lo + 16) * (H - 40); };
    v.textW = [](const std::string& s, bool) { return (int)s.size() * 7; }; v.zigzag = true;
    wpc::Scene sc = wpc::layout(E, nullptr, v);
    for (int i = first; i <= last; i++) std::cout << "C " << v.X(i) << " " << v.Y(B[(size_t)i].o) << " " << v.Y(B[(size_t)i].h) << " " << v.Y(B[(size_t)i].l) << " " << v.Y(B[(size_t)i].c) << "\n";
    for (auto& q : sc.p) std::cout << "P " << (int)q.k << " " << q.x0 << " " << q.y0 << " " << q.x1 << " " << q.y1 << " " << q.col << " " << q.w << " " << q.back << " " << q.bold << " " << q.text << "\n";
    for (const wpc::Wave& w : E.waves) if (w.i1 >= first) std::cerr << "WAVE " << (w.dir > 0 ? "up " : "down ") << wpc::hm(w.t0) << " -> " << wpc::hm(w.t1) << " " << wpc::qName(w.qWhy) << " kind " << wpc::kindName(w.kind) << " letters " << wpc::letters(w, E.B).size() << " pb " << wpc::pullbacks(w, E.alt, E.B).size() << " clRef " << w.clRef << "\n";
    std::cerr << "letters " << sc.lettersDrawn << " pb " << sc.pbDrawn << " boxes " << sc.boxesDrawn << "\n";
}
