// srrun: the SR engine as a CLI (parity tests + the nightly). srrun <events.csv> <levels.csv> <rthOpenSec> <rthCloseSec> <atrTicks> [recBars] [bigA] [bigP]
//   events: F,tMs,px,sz,ag,aggressor id[,resting id]   A / C,tMs,side,px,sz   V,tMs,sz (a print that only counts as volume)
#include "../SRLogic.h"
#include <fstream>
#include <sstream>
#include <iostream>
int main(int argc, char** argv) {
    if (argc < 6) return 2;
    sr::Engine e; e.rthOpenSec = atoll(argv[3]); e.rthCloseSec = atoll(argv[4]); e.P.atrTicks = atof(argv[5]); if (argc > 6) e.P.recBars = atoi(argv[6]);
    if (argc > 8) { e.P.bigA = atof(argv[7]); e.P.bigP = atof(argv[8]); }
    std::vector<sr::Level> L; { std::ifstream in(argv[2]); std::string ln; while (std::getline(in, ln)) { std::stringstream s(ln); std::string a, b, c, d; if (!std::getline(s, a, ',') || !std::getline(s, b, ',') || !std::getline(s, c, ',') || !std::getline(s, d, ',')) continue; sr::Level l; l.name = a; l.px = atoll(b.c_str()); l.side = atoi(c.c_str()); l.fromSec = atoll(d.c_str()); L.push_back(l); } }
    e.setLevels(L);
    std::ifstream in(argv[1]); std::string ln; long long last = 0;
    while (std::getline(in, ln)) { char k; long long t, px, sz; int a; unsigned long long id, rest;
        if (ln.empty()) continue;
        k = ln[0];
        if (k == 'F') { rest = 0; const int n = sscanf(ln.c_str() + 2, "%lld,%lld,%lld,%d,%llu,%llu", &t, &px, &sz, &a, &id, &rest); if (n >= 5) { e.onFill(t, px, sz, a, id, n >= 6 ? rest : 0); last = t; } }
        else if (k == 'V') { if (sscanf(ln.c_str() + 2, "%lld,%lld", &t, &sz) == 2) { e.onVolume(t, sz); last = t; } }
        else if (k == 'A' || k == 'C') { int side; if (sscanf(ln.c_str() + 2, "%lld,%d,%lld,%lld", &t, &side, &px, &sz) == 4) { e.onBook(t, side, k == 'A', px, sz); last = t; } } }
    e.advance(last + 600000);
    for (size_t i = 0; i < e.journal.size(); ++i) std::cout << e.journal[i] << "\n";
    return 0;
}
