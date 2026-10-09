// Drives the REAL SessionVWAP.cpp through the mock SDK on a bar file and writes its 11 outputs per bar.
//   run_plugin <bars.csv: end_epoch,h,l,c,v> <root> <out.csv> [incremental step]
// With a step, the bars arrive the way IRT delivers them (the chart grows; calc(from = the new bar)), and the result must
// equal one full calc - checked by the Python side.
#include "irtsdk.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
namespace mock { Host* current = NULL; }
#include "../SessionVWAP.cpp"
int main(int argc, char** argv)
{
    if (argc < 4) return 2;
    std::ifstream in(argv[1]); std::string line;
    std::vector<unsigned long> T, V; std::vector<float> H, L, C;
    while (std::getline(in, line)) {
        if (line.empty() || !isdigit((unsigned char)line[0])) continue;
        std::stringstream ss(line); std::string a; std::vector<std::string> f;
        while (std::getline(ss, a, ',')) f.push_back(a);
        T.push_back(strtoul(f[0].c_str(), 0, 10)); H.push_back(strtof(f[1].c_str(), 0)); L.push_back(strtof(f[2].c_str(), 0));
        C.push_back(strtof(f[3].c_str(), 0)); V.push_back(strtoul(f[4].c_str(), 0, 10));
    }
    int step = argc > 4 ? atoi(argv[4]) : 0;
    mock::Host Hh; Hh.root = argv[2]; mock::current = &Hh;
    SessionVWAP ext;
    auto grow = [&](size_t m) {
        for (int a = 0; a < 14; a++) Hh.f[a].resize(m, 0.0f);
        for (int a = 0; a < 2; a++) Hh.i[a].resize(m, 0UL);
        for (size_t i = 0; i < m; i++) {
            Hh.i[(int)barDateTime][i] = T[i]; Hh.i[(int)barVolume][i] = V[i];
            Hh.f[(int)barHigh][i] = H[i]; Hh.f[(int)barLow][i] = L[i]; Hh.f[(int)barClose][i] = C[i];
        }
    };
    size_t n = T.size();
    if (step > 0) {
        size_t m = 400 < n ? 400 : n; grow(m); ext.fill(0);
        while (m < n) { size_t m2 = m + step < n ? m + step : n; grow(m2); ext.fill((int)m); m = m2; }
    } else { grow(n); ext.fill(0); }
    ext.draw();                                              // the badge path runs too (ASAN build)
    std::ofstream o(argv[3]);
    for (size_t i = 0; i < n; i++) {
        o << T[i];
        for (int k = 0; k < 11; k++) { char b[32]; snprintf(b, sizeof(b), ",%.6f", Hh.f[k][i]); o << b; }
        o << "\n";
    }
    ext.done();
    return 0;
}
