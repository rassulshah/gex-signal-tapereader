// test_zzodelta_ref.cpp - ZzoDeltaLogic.h (lsZzoDelta 1.3.1 / lsZzoVolume 1.2.1; 1.3.1 adds the per-market ZzoParams.h check) against the independent Python model zzo_ref.py:
// every completed wave (end bar, direction, delta, volume, the three normals, Ab?, Cmx, AR, ST, levels swept, CoB / weak) must match,
// on the RTH-only ES fixture and on full-session ES (Oct 19-28) and GC (Oct 5-14) fixtures with overnight bars (key levels).
#include "../ZzoDeltaLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
static std::string f1(bool ok, double v) { if (!ok) return "na"; char b[32]; std::snprintf(b, sizeof b, "%.1f", v); return b; }
struct Px { bool set = false; double abE = 0, abB = 0, cmx = 0, swD = 0, swV = 0, swS = 0; };
static void run(const char* csv, const char* ref, const char* mkt, double tick, Px px = Px()) {
    zzd::Engine E; E.setMarket(mkt, 180);
    if (px.set) { E.abEffort = px.abE; E.abBack = px.abB; E.cmxVol = px.cmx; E.swDelta = px.swD; E.swVol = px.swV; E.swSize = px.swS; }
    std::ifstream f(csv); std::string ln; int n = 0;
    while (std::getline(f, ln)) {
        if (ln.empty() || ln[0] == '#') continue;
        std::stringstream s(ln); std::string a; std::vector<std::string> x; while (std::getline(s, a, ',')) x.push_back(a);
        zzd::Bar b; b.t = std::stoll(x[0]); zzd::toTicks(std::stod(x[1]), tick, b.h); zzd::toTicks(std::stod(x[2]), tick, b.l); b.vol = std::stoll(x[3]); b.dlt = std::stoll(x[4]); b.vap = true;
        E.addBar(b); ++n;
    }
    std::ifstream r(ref); std::vector<std::string> want; while (std::getline(r, ln)) if (!ln.empty()) want.push_back(ln);
    int same = 0, shown = 0;
    for (size_t k = 0; k < E.W.size() && k < want.size(); ++k) {
        const zzd::Wave& w = E.W[k]; char b[256];
        std::snprintf(b, sizeof b, "%d,%d,%lld,%lld,%s,%s,%s,%d,%d,%d,%d,%d,%d,%d", w.e, w.dir, w.dlt, w.vol, f1(w.hasR, w.med).c_str(), f1(w.hasV, w.medV).c_str(), f1(w.hasS, w.medS).c_str(), w.ab, (int)w.cmx, (int)w.cob, (int)w.st, w.sw, w.cobTag, w.swLv);
        if (want[k] == b) ++same; else if (shown++ < 5) std::printf("  %s wave %zu:\n    got  %s\n    want %s\n", csv, k, b, want[k].c_str());
    }
    std::printf("  %s: %d bars, %zu waves, %d match\n", csv, n, E.W.size(), same);
    CHECK(E.W.size() == want.size(), "wave count = the Python model"); CHECK(same == (int)want.size(), "every wave = the Python model");
    // the CoB / weak tags are drawn (both views) and stay inside the pane
    int wantCob = 0, wantWeak = 0; for (const zzd::Wave& w : E.W) { wantCob += w.cobTag == 1; wantWeak += w.cobTag == 2; }
    for (int vol = 0; vol < 2; ++vol) {
        zzd::View v; v.volume = vol != 0; v.l = 0; v.t = 0; v.r = 40000; v.b = 260; v.first = 0; v.last = (int)E.B.size() - 1; v.bw = 9;
        v.X = [](int j) { return j * 9.0 + 5; }; v.textW = [](const std::string& t) { return (int)t.size() * 6; };
        const zzd::Scene sc = zzd::layout(E, E.view(nullptr), 0, v, "ZZO");
        bool in = true; for (const zzd::Prim& p : sc.p) if (p.y0 < v.t || p.y1 > v.b) in = false;
        CHECK(sc.swCob == wantCob && sc.swWeak == wantWeak && in, "CoB / weak boxes drawn inside the pane");
    }
    std::string st = zzd::statusText(E, mkt, 180, "ok"); CHECK(wantCob == 0 || (st.find("CoB after the sweep of") != std::string::npos && st.find("LEVELS,ONH") != std::string::npos), "status lists CoB with its levels");
}
int main() {
    run("zzo_es_rth_1029_1030.csv", "ref_zzo_es_rth_1029_1030.txt", "ES", 0.25);
    run("zzo_es_full_1019_1028.csv", "ref_zzo_es_full_1019_1028.txt", "ES", 0.25);
    run("zzo_gc_full_1005_1014.csv", "ref_zzo_gc_full_1005_1014.txt", "GC", 0.1);
    // (1.3.1) ZzoParams.h: every market starts on the 1.3.0 numbers; moved thresholds = the nightly model (lra.zzo_model) with the same numbers
    { zzd::Engine E; CHECK(zzd::zzoParamsBuilt(), "ZzoParams.h compiled in");
      const char* ms[] = { "ES", "NQ", "CL", "GC", "HG", "NG", "6E" };
      for (const char* m : ms) { E.setMarket(m, 180); CHECK(E.abEffort == 1.9 && E.abBack == 0.75 && E.cmxVol == 2.5 && E.swDelta == 1.9 && E.swVol == 1.5 && E.swSize == 1.5, "market starts on the 1.3.0 numbers"); }
      E.setMarket("ES", 180); CHECK(zzd::statusText(E, "ES", 180, "ok").find("PARAMS,1.90,0.75,2.50,1.90,1.50,1.50,ZzoParams.h") != std::string::npos, "status shows the numbers in use"); }
    Px px; px.set = true; px.abE = 2.5; px.abB = 0.6; px.cmx = 2.0; px.swD = 1.5; px.swV = 1.0; px.swS = 1.0;
    run("zzo_gc_full_1005_1014.csv", "ref_params_gc_full_1005_1014.txt", "GC", 0.1, px);
    run("zzo_es_full_1019_1028.csv", "ref_params_es_full_1019_1028.txt", "ES", 0.25, px);
    std::printf("%d passed, %d failed\n", pass, fail); return fail ? 1 : 0;
}
