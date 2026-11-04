// gen_2179.cpp - writes grid_2179.txt from the 2.17.9 header (compile with -I<2.17.9 plugin dir>)
#include <cstdio>
#include <cmath>
#include <fstream>
#include "HodLodLogic.h"
#include "lib_widths.h"
#include "scen.h"
#include "grid_2179.h"
static double SC = 1.0;
static int MS(const std::string& s, int fs, bool b) { return (int)(measureLib(s, fs, b) * SC + 0.5); }
int main() { auto yOf = [](long long) { return 300; }; std::ofstream o("grid_2179.txt"); const double SCS[5] = { 1.0, 1.25, 1.3, 1.35, 1.4 };
    for (int m = 0; m < 6; m++) for (int W : { 930, 950, 980, 1050, 1200, 1530, 1900 }) for (int sci = 0; sci < 5; sci++) for (int pos : { 6, 7, 1 }) for (int var = 0; var < 2; var++) {
        SC = SCS[sci]; const hl::Box pane = { 0, 0, W, 640 }; hl::View v = mkView(m, var == 0); hl::Opts O;
        hl::Layout L = hl::layoutAll(v, pane, yOf, MS, (int)(W * 0.75), pos, -1, nullptr, &O);
        o << m << " " << W << " " << sci << " " << pos << " " << var << " " << gridLine(L) << "\n";
        if (hl::stretchToAxis(L, v, pane, yOf, MS, (int)(W * 0.75), pos, -1, nullptr, O, W - 2)) o << "S " << gridLine(L) << "\n"; }
}
