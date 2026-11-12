// cobrun.cpp - replay CLI of the CoB finder (lsZzoCombo 2.3.0): bars "t|o|h|l|c|v" (t = bar END local secs, prices in ticks) on stdin,
// fed exactly as ZzoCombo.cpp feeds them (engine + finder, trims included) -> the finder's record lines on stdout.
//   cobrun <ROOT> [restartEvery]   restartEvery > 0: the whole state is rebuilt from the last 6000 bars every N bars (no-repaint check)
#include "../ZzoComboView.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>
struct Row { long long t, o, h, l, c, v; };
struct Host { zzd::Engine E; zcf::Finder F; long long lastWaveT = -1;
    void init(const std::string& root) { E.reset(); E.revBars = zzd::COMBO_REV_BARS; E.setMarket(root, 180); F.reset(); F.setMarket(root, 180); lastWaveT = -1; }
    void add(const Row& r) {
        zzd::Bar b; b.t = r.t; b.h = r.h; b.l = r.l; b.vol = r.v; b.vap = true; if (b.h < b.l) std::swap(b.h, b.l);
        E.addBar(b); long long c = r.c; if (c > b.h) c = b.h; if (c < b.l) c = b.l; F.addBar(b, c);
        if (E.trimmed > 0) { F.trim(E.trimmed); E.trimmed = 0; }
        if (!E.W.empty() && F.F.size() == E.B.size()) { const zzd::Wave& w = E.W.back();
            if (w.s >= 0 && w.e >= 0 && (size_t)w.e < F.F.size()) { const long long tw = F.F[(size_t)w.e].t * 2 + (w.dir > 0 ? 1 : 0); if (tw != lastWaveT) { lastWaveT = tw; F.onWave(E.W); } } }
    } };
int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "ES"; const int every = argc > 2 ? std::atoi(argv[2]) : 0;
    std::vector<Row> R; std::string ln;
    while (std::getline(std::cin, ln)) { Row r; if (std::sscanf(ln.c_str(), "%lld|%lld|%lld|%lld|%lld|%lld", &r.t, &r.o, &r.h, &r.l, &r.c, &r.v) == 6) R.push_back(r); }
    Host H; H.init(root); std::set<std::string> seen; long long fed = 0;
    for (size_t i = 0; i < R.size(); ++i) {
        if (every > 0 && i > 0 && i % (size_t)every == 0) {   // a restart: rebuild from the bars IRT would still hold
            H.init(root); const size_t a = i > 5000 ? i - 5000 : 0; for (size_t k = a; k < i; ++k) H.add(R[k]); H.F.out.clear(); }
        H.add(R[i]); ++fed;
        for (const std::string& o : H.F.out) if (seen.insert(o).second) std::printf("%s\n", o.c_str());
        H.F.out.clear();
    }
    std::fprintf(stderr, "bars %lld waves %zu tags %zu\n", fed, H.E.W.size(), H.F.tags.size());
    return 0;
}
