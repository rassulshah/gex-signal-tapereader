// lsZzoCombo 3.2.0: the split wave read on a synthetic zigzag - verdict + sentences left, numbers right, inside the pane, nothing overlapping
#include "../ZzoComboView.h"
#include <cstdio>
static int ok = 0, bad = 0;
#define CHECK(c, m) do { if (c) ++ok; else { ++bad; std::printf("FAIL line %d: %s\n", __LINE__, m); } } while (0)
static bool isHead(const std::string& t) { return t.rfind("Bullish", 0) == 0 || t.rfind("Bearish", 0) == 0 || t.rfind("Mixed", 0) == 0 || t.rfind("Quiet", 0) == 0; }
int main() {
    zzd::Engine Z; Z.revBars = zzd::COMBO_REV_BARS; Z.setMarket("ES", 180);
    long long px = 30000; int dir = 1;
    for (int i = 0; i < 300; ++i) { if (i % 9 == 0) dir = -dir; const int step = dir > 0 ? (i % 27 < 9 ? 6 : 3) : 2;
        zzd::Bar b; b.t = 3877700000LL + i * 180; b.vol = 400 + (i % 9) * 50 + (dir > 0 ? 300 : 0); const long long o = px; px += dir * step; b.h = std::max(o, px) + 1; b.l = std::min(o, px) - 1; Z.addBar(b); }
    const std::vector<zzd::Wave> waves = Z.view(nullptr); const int n = (int)Z.B.size();
    CHECK(waves.size() >= 6, "the zigzag has waves");
    for (int pick : { -1, n - 30, n - 5 }) {
        zzd::View v; v.l = 0; v.t = 0; v.r = 1500; v.b = 120; v.first = n - 150; v.last = n - 1; v.bw = 6; const int first = v.first;
        v.X = [first](int j) { return 20.0 + (j - first) * 6.0; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; }; v.textH = 12;
        zzd::XRead x; x.on = true; x.readX = 20 + 150 * 6 + 20; x.pick = pick;
        const zzd::ComboScene S = zzd::layoutCombo(Z, waves, 0, v, "", nullptr, &x);
        bool head = false, inside = true; int sent = 0, leftR = 0, dataX = 1 << 30, over = 0;
        std::vector<const zzd::Prim*> T; for (const zzd::Prim& p : S.p) if (p.k == zzd::Prim::TEXT && p.x0 >= x.readX) T.push_back(&p);
        for (const zzd::Prim* p : T) { if (p->x0 == x.readX) { if (isHead(p->text)) head = true; else { ++sent; leftR = std::max(leftR, p->x1); } }
            if (p->text == "Wave" && p->x0 > x.readX) dataX = p->x0;
            if (p->x1 > v.r || p->y1 > v.b || p->y0 < v.t) inside = false; }
        for (size_t a = 0; a < T.size(); ++a) for (size_t b = a + 1; b < T.size(); ++b) if (T[a]->y0 == T[b]->y0 && T[a]->x0 < T[b]->x1 && T[b]->x0 < T[a]->x1) ++over;
        char m[160]; std::snprintf(m, sizeof m, "pick %d: verdict %d sentences %d leftR %d dataX %d inside %d overlaps %d", pick, (int)head, sent, leftR, dataX, (int)inside, over);
        CHECK(head && sent >= 1 && inside && over == 0, m);
        CHECK(leftR <= x.readX + spt::LEFT_MAX && dataX >= leftR + 13 && dataX < (1 << 30), m);
        if (pick == -1) for (const auto& b : S.box) std::printf("   %s: %s\n", b.first.c_str(), b.second.c_str());
    }
    { zzd::View v; v.l = 0; v.t = 0; v.r = 1500; v.b = 30; v.first = n - 150; v.last = n - 1; v.bw = 6; const int first = v.first;
      v.X = [first](int j) { return 20.0 + (j - first) * 6.0; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; }; v.textH = 12;
      zzd::XRead x; x.on = true; x.readX = 940; const zzd::ComboScene S = zzd::layoutCombo(Z, waves, 0, v, "", nullptr, &x);
      bool inside = true; for (const zzd::Prim& p : S.p) if (p.k == zzd::Prim::TEXT && p.x0 >= 940 && p.y1 > v.b) inside = false; CHECK(inside && S.boxRows <= 2, "a low pane keeps what fits"); }
    std::printf("t_zc320_split: %d passed, %d failed\n", ok, bad); return bad ? 1 : 0;
}
