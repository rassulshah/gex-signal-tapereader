// t_ca120.cpp -- lsCandleAbsorb 1.2.0 (Rassul 2026-11-11 "Update the candle absorb indicator so that it doesn't display designers by default and only
// displays the dash marking but the setting allows you to select the signals you want displayed along with markings like delta poc (default) as well
// as volume poc and bid poc and ask poc"). Real ES 3m bars + VAP (pboc_es1109.txt), fed bar by bar.
//  -DOLD: compiled against the 1.1.1 header, prints the scene hash + record hash (the parity reference).
//  [1] default: no letters, the delta POC dashes exactly as 1.1.1   [2] all letters on + delta only = the 1.1.1 scene, byte for byte
//  [3] each letter switch shows only its code   [4] volume / bid / ask POC = the brute-force rows; ticks sit outside the body, left / right
//  [5] every marking off = nothing drawn; the records (MARK lines) never depend on the settings
#include "CandleAbsorbLogic.h"
#include <fstream>
#include <map>
#include <cstring>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
struct VR { double p, bu, se; };
struct Fx { std::vector<cab::Bar> B; std::vector<std::vector<VR>> V; };
static Fx load() { Fx X; std::ifstream in("pboc_es1109.txt"); std::string l; std::map<std::string, size_t> at; const double tick = 0.25;
    while (std::getline(in, l)) { if (l.size() < 3 || l[1] != '|') continue; char ts[32] = { 0 };
        if (l[0] == 'B') { double o, h, lo, c, v; if (std::sscanf(l.c_str() + 2, "%19[^|]|%lf|%lf|%lf|%lf|%lf", ts, &o, &h, &lo, &c, &v) != 6) continue;
            cab::Bar b; b.t = (long long)X.B.size() * 180 + 180; cab::toTicks(o, tick, b.o); cab::toTicks(h, tick, b.h); cab::toTicks(lo, tick, b.l); cab::toTicks(c, tick, b.c); at[ts] = X.B.size(); X.B.push_back(b); X.V.emplace_back(); }
        else if (l[0] == 'V') { double p, v, bu, se; if (std::sscanf(l.c_str() + 2, "%19[^|]|%lf|%lf|%lf|%lf", ts, &p, &v, &bu, &se) != 5) continue; const auto it = at.find(ts); if (it != at.end()) X.V[it->second].push_back(VR{ p, bu, se }); } }
    return X; }
static std::vector<cab::DRow> rowsOf(const std::vector<VR>& v) { std::vector<cab::DRow> o; for (const VR& r : v) { cab::DRow d; cab::toTicks(r.p, 0.25, d.tick); d.dlt = (long long)(r.bu - r.se);
#ifndef OLD
    d.buy = (long long)r.bu; d.sell = (long long)r.se;
#endif
    o.push_back(d); } return o; }
static unsigned long long hashS(const cab::Scene& s) { unsigned long long h = 1469598103934665603ULL; auto mix = [&](long long v) { h ^= (unsigned long long)v; h *= 1099511628211ULL; };
    for (const cab::Prim& p : s.p) { mix(p.k); mix(p.x0); mix(p.y0); mix(p.x1); mix(p.y1); mix((long long)p.col); for (char c : p.text) mix(c); } return h; }
static cab::View view(const cab::Engine& E) { cab::View v; v.l = -100000; v.t = -100000; v.r = 100000; v.b = 100000; v.first = 0; v.last = (int)E.B.size() - 1; v.bw = 12;
    v.X = [](int j) { return 20.0 + j * 12.0; }; v.Y = [](double tk) { return 400000.0 - tk * 6.0; }; v.textW = [](const std::string& s) { return (int)s.size() * 7; }; return v; }
int main() {
    const Fx X = load(); cab::Engine E; for (size_t i = 0; i < X.B.size(); ++i) E.addBar(X.B[i], rowsOf(X.V[i]));
    unsigned long long rec = 1469598103934665603ULL; for (const std::string& l : E.out) for (char c : l) { rec ^= (unsigned char)c; rec *= 1099511628211ULL; }
    cab::View v = view(E);
#ifdef OLD
    const cab::Scene s = cab::layout(E, v); int tx = 0; for (const cab::Prim& p : s.p) tx += p.k == cab::Prim::TEXT;
    std::printf("%llu %llu %d %d %zu\n", hashS(s), rec, s.dashes, tx, E.marks.size()); return 0;
#else
    unsigned long long oh = 0, orec = 0; int od = 0, ot = 0; size_t om = 0; { FILE* f = std::fopen("old.txt", "r"); if (f) { if (std::fscanf(f, "%llu %llu %d %d %zu", &oh, &orec, &od, &ot, &om) != 5) oh = 0; std::fclose(f); } }
    CHECK(std::string(cab::VERSION) == "1.2.0", "version 1.2.0");
    CHECK(oh != 0 && ot >= 10 && od > 300, "the 1.1.1 reference loaded: dashes " + std::to_string(od) + ", letters " + std::to_string(ot));
    CHECK(rec == orec && E.marks.size() == om, "[5] the records and decisions are the 1.1.1 ones");
    { const cab::Scene s = cab::layout(E, v); int tx = 0; for (const cab::Prim& p : s.p) tx += p.k == cab::Prim::TEXT;
      CHECK(tx == 0 && s.marksDrawn == 0 && s.marksOff >= ot, "[1] default: no letters (" + std::to_string(s.marksOff) + " switched off)");
      CHECK(s.dashes == od && s.volDashes == 0 && s.bidTicks == 0 && s.askTicks == 0, "[1] default: the delta POC dashes only"); }
    { cab::View w = v; w.show.a = w.show.ae = w.show.d = w.show.s = true; const cab::Scene s = cab::layout(E, w);
      CHECK(hashS(s) == oh && s.marksDrawn == ot, "[2] all letters on + delta POC = the 1.1.1 scene exactly"); }
    { const int codes[4] = { cab::A, cab::AE, cab::D, cab::S }; int sum = 0, bad = 0;
      for (int k = 0; k < 4; ++k) { cab::View w = v; w.show.a = k == 0; w.show.ae = k == 1; w.show.d = k == 2; w.show.s = k == 3; const cab::Scene s = cab::layout(E, w);
          for (const cab::Prim& p : s.p) if (p.k == cab::Prim::TEXT) { ++sum; if (p.text != cab::codeName(codes[k])) ++bad; } }
      CHECK(bad == 0 && sum == ot, "[3] each switch shows only its own letter (" + std::to_string(sum) + " in all)"); }
    { int bad = 0, n = 0, same = 0; for (size_t i = 0; i < X.B.size(); ++i) { const cab::BarRead& r = E.R[i]; if (!r.have) continue; ++n;
          std::map<long long, std::pair<long long, long long>> m; for (const VR& q : X.V[i]) { long long t; cab::toTicks(q.p, 0.25, t); m[t].first += (long long)q.bu; m[t].second += (long long)q.se; }
          long long vp = 0, bp = 0, ap = 0, vv = 0, bv = 0, av = 0; for (const auto& kv : m) { if (kv.second.first + kv.second.second > vv) { vv = kv.second.first + kv.second.second; vp = kv.first; } if (kv.second.second > bv) { bv = kv.second.second; bp = kv.first; } if (kv.second.first > av) { av = kv.second.first; ap = kv.first; } }
          if (!r.haveV || r.vpoc != vp || (bv > 0) != r.haveB || (bv > 0 && r.bpoc != bp) || (av > 0) != r.haveA || (av > 0 && r.apoc != ap)) ++bad;
          if (r.vpoc == r.poc && r.pocD != 0) ++same; }
      CHECK(bad == 0 && n > 300, "[4] volume / bid / ask POC = the brute-force rows on " + std::to_string(n) + " bars (" + std::to_string(same) + " share the delta POC row)");
      cab::View w = v; w.show.dPoc = false; w.show.vPoc = w.show.bPoc = w.show.aPoc = true; const cab::Scene s = cab::layout(E, w); int out = 0, col = 0;
      for (const cab::Prim& p : s.p) { if (p.col == 0x000000 || p.col == 0xFFFFFF) continue; const double cx = (p.x0 + p.x1) / 2.0; const int j = (int)std::lround((cx - 20.0) / 12.0); const double xc = 20.0 + j * 12.0;
          const bool left = cx < xc && p.x0 >= xc - 6, right = cx > xc && p.x1 <= xc + 6; if (!left && !right) ++out; if ((left && p.col != 0xEF4444) || (right && p.col != 0x22C55E)) ++col; }
      CHECK(s.dashes == 0 && s.volDashes == n && s.bidTicks > 300 && s.askTicks > 300 && out == 0 && col == 0, "[4] ticks outside the body: bid red left, ask green right; volume dash white");
      cab::View b2 = v; b2.show.vPoc = true; const cab::Scene s2 = cab::layout(E, b2); int wide = 0; for (const cab::Prim& p : s2.p) if (p.col == 0xFFFFFF && p.x1 - p.x0 > 13) ++wide;
      CHECK(wide == same && wide > 0, "[4] a volume dash on the delta POC row is wider (" + std::to_string(wide) + ")"); }
    { cab::View w = v; w.show = cab::Show(); w.show.dPoc = false; const cab::Scene s = cab::layout(E, w); CHECK(s.p.empty(), "[5] everything off = nothing drawn"); }
    std::printf("t_ca120 (lsCandleAbsorb %s): %d passed, %d failed\n", cab::VERSION, passes, fails); return fails ? 1 : 0;
#endif
}
