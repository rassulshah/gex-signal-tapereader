// golden.cpp - writes golden_2178.txt from lsHodLod 2.17.8's HodLodLogic.h (build with -I<the 2.17.8 copy> first): one line per scenario
#include <cstdio>
#include "HodLodLogic.h"
#include "lib_widths.h"
#include "scen.h"
static double SC = 1.0;
static int MS(const std::string& s, int fs, bool b) { return (int)(measureLib(s, fs, b) * SC + 0.5); }
int main() {
    auto yOf = [](long long) { return 300; };
    for (int m = 0; m < 6; m++) for (int W : { 930, 950, 980, 1050, 1200, 1530, 1900 }) for (int sci = 0; sci < 5; sci++) for (int pos : { 6, 7, 1 }) for (int var = 0; var < 3; var++) {
        static const double SCS[5] = { 1.0, 1.25, 1.3, 1.35, 1.4 }; SC = SCS[sci];
        hl::View v = mkView(m, var != 1); hl::Opts O; const hl::Box pane = { 0, 0, W, var == 2 ? 300 : 640 };
        hl::Layout L = hl::layoutAll(v, pane, yOf, MS, (int)(W * 0.75), pos, -1, nullptr, &O);
        std::printf("%s %d %d %d %d %d %016llx\n", MKS[m].code, W, sci, pos, var, (int)L.swOk, fnv(itemsText(L)));
    }
}
