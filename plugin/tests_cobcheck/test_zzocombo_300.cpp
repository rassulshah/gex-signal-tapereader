// lsZzoCombo 3.0.0: the pane view with the checklist read (XRead) on the recorded ES chart bars - tags, the read box, nothing of the old finder
#include "../ZzoComboView.h"
#include "../CobCheck.h"
#include <fstream>
static int ok = 0, bad = 0;
#define CHECK(c) do { if (c) ++ok; else { ++bad; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)
int main(int argc, char** argv)
{
    CHECK(argc > 1); if (argc < 2) return 1;
    const double tick = 0.25; zzd::Engine Z; Z.revBars = zzd::COMBO_REV_BARS; Z.setMarket("ES", 180); zcf::Finder F; F.setMarket("ES", 180);
    cobs::Engine C; C.setMarket("ES", tick, 180); std::ifstream f(argv[1]); std::string l; long long lastWaveT = -1; int off = 0;
    while (std::getline(f, l)) { cobs::Bar b; if (std::sscanf(l.c_str(), "%lld,%lf,%lf,%lf,%lf,%lf", &b.t, &b.o, &b.h, &b.l, &b.c, &b.v) != 6) continue;
        C.addBar(b); C.out.clear(); C.lvpOut.clear();
        zzd::Bar z; z.t = b.t; z.vol = (long long)b.v; zzd::toTicks(b.h, tick, z.h); zzd::toTicks(b.l, tick, z.l); long long c = 0, o = 0; zzd::toTicks(b.c, tick, c); zzd::toTicks(b.o, tick, o);
        Z.addBar(z); F.addBar(z, c, o);
        if (Z.trimmed > 0) { F.trim(Z.trimmed); off += (int)Z.trimmed; Z.trimmed = 0; }
        if (!Z.W.empty() && F.F.size() == Z.B.size()) { const zzd::Wave& w = Z.W.back(); if (w.s >= 0 && w.e >= 0 && (size_t)w.e < F.F.size()) { const long long tw = F.F[(size_t)w.e].t * 2 + (w.dir > 0 ? 1 : 0); if (tw != lastWaveT) { lastWaveT = tw; F.onWave(Z.W); } } }
    }
    const std::vector<cck::Item> items = cck::scan(C); CHECK(!items.empty());
    zzd::XRead x; x.on = true;
    for (const cck::Item& it : items) { x.cob[std::make_pair(it.tEnd, it.side)] = it.side; for (long long t : it.lvpT) x.lvp[std::make_pair(t, -it.side)] = it.side; }
    const cck::Read R = cck::read(C, items); x.any = R.any; x.dir = R.dir; for (const cck::Row& r : R.rows) { x.rows.push_back(std::make_pair(r.k, r.v)); x.col.push_back(r.col); }
    // every Cob / Lvp of the signal ends on a pivot of the pane's own ZigZag (the tag has a wave to sit on), for the bars the pane still holds
    const std::vector<zzd::Wave> waves = Z.view(nullptr); int cobOn = 0, cobIn = 0, lvpOn = 0, lvpIn = 0;
    for (const cck::Item& it : items) { if (it.tEnd < Z.B.front().t) continue; ++cobIn; for (const zzd::Wave& w : waves) if (!w.live && w.e >= 0 && Z.B[(size_t)w.e].t == it.tEnd && w.dir == it.side) { ++cobOn; break; }
        for (long long t : it.lvpT) { ++lvpIn; for (const zzd::Wave& w : waves) if (!w.live && w.e >= 0 && Z.B[(size_t)w.e].t == t && w.dir == -it.side) { ++lvpOn; break; } } }
    std::printf("Cobs in the pane's bars %d on a pane wave %d | Lvps %d on a pane wave %d\n", cobIn, cobOn, lvpIn, lvpOn);
    CHECK(cobIn >= 1); CHECK(cobOn == cobIn); CHECK(lvpOn == lvpIn);
    // the view: the last 220 bars (Fri Nov 13: the Cob Dn off R1) in a 1400 x 150 pane
    const int n = (int)Z.B.size(); zzd::View v; v.l = 0; v.t = 0; v.r = 1400; v.b = 150; v.first = n - 220; v.last = n - 1; v.bw = 6; const int first = v.first;
    v.X = [first](int j) { return 20.0 + (j - first) * 6.0; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; }; v.textH = 12;
    const zzd::ComboScene S = zzd::layoutCombo(Z, waves, 0, v, "Cob Reader 3.0.0", &F, &x);
    std::printf("scene: prims %zu, Cob tags %d, Lvp tags %d, box rows %d (%d,%d)-(%d,%d)\n", S.p.size(), S.cob, S.lvp, S.boxRows, S.boxL, S.boxT, S.boxR, S.boxB);
    CHECK(S.boxRows == 6); CHECK(S.cob == 1); CHECK(S.cob2 == 0 && S.cb == 0 && S.sr == 0);
    CHECK(S.box.size() == 6 && S.box[0].first == "Cob" && S.box[2].second == "3/3 3row SMA R1" && S.box[5].first == "Win");
    int outside = 0, rowsOver = 0; std::vector<std::pair<int, int>> ys;
    for (const zzd::Prim& p : S.p) { if (p.k != zzd::Prim::TEXT) continue; if (p.x0 >= S.boxL && p.x1 <= S.boxR && p.y0 >= S.boxT && p.y1 <= S.boxB) continue; if (p.x1 > S.boxL && p.x0 < S.boxR && p.y1 > S.boxT && p.y0 < S.boxB) ++outside; }
    for (const zzd::Prim& p : S.p) if (p.k == zzd::Prim::TEXT && p.x0 >= S.boxL && p.x1 <= S.boxR && p.y0 >= S.boxT && p.y1 <= S.boxB && p.x0 == S.boxL + 6) ys.push_back(std::make_pair(p.y0, p.y1));
    for (size_t i = 1; i < ys.size(); ++i) if (ys[i].first < ys[i - 1].second) ++rowsOver;
    CHECK(outside == 0); CHECK(rowsOver == 0); CHECK(ys.size() == 6);
    // the value and its label of a row never touch
    { int touch = 0; for (const zzd::Prim& a : S.p) for (const zzd::Prim& b : S.p) if (&a != &b && a.k == zzd::Prim::TEXT && b.k == zzd::Prim::TEXT && a.y0 == b.y0 && a.y0 > S.boxT + 12 && a.x0 >= S.boxL && a.x1 <= S.boxR && b.x0 >= S.boxL && b.x1 <= S.boxR && a.x0 < b.x0 && a.x1 + 4 > b.x0) ++touch; CHECK(touch == 0); }
    // a short pane drops rows from the bottom, never draws outside it; without an XRead the 2.9.5 box is back (6 rows, its own labels)
    { zzd::View s = v; s.b = 60; const zzd::ComboScene T = zzd::layoutCombo(Z, waves, 0, s, "Cob Reader 3.0.0", &F, &x); CHECK(T.boxRows >= 2 && T.boxRows < 6); CHECK(T.boxB <= 60); }
    { const zzd::ComboScene O = zzd::layoutCombo(Z, waves, 0, v, "Cob Reader 3.0.0", &F, nullptr); CHECK(O.boxRows == 6 && O.box[2].first == "Q / Rng"); }
    { zzd::XRead offx; const zzd::ComboScene O = zzd::layoutCombo(Z, waves, 0, v, "Cob Reader 3.0.0", &F, &offx); CHECK(O.box.size() == 6 && O.box[2].first == "Q / Rng"); }
    std::printf("%d passed, %d failed\n", ok, bad); return bad ? 1 : 0;
}
