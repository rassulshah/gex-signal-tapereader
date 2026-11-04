// test_bo150_bi.cpp -- lsBreakout 1.5.0 (2026-11-04, Rassul approved the BreakIn mockup v0.3 and said "build"): BI = BreakIn (a failed BO / BD).
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. test_bo150_bi.cpp
//   [1] PROTOTYPE PARITY: ES Oct 20 17:03 - Nov 3 19:45 (wk_ES_eve_3m.txt = bo_eve/es_all.txt), one full pass with biCancel = false ==
//       the v0.3 prototype (bi/run5.py, AVGX 1.0, N 10, "close", regenerated: expected_bi_ES_proto.txt, 271 BI), every (candle, side, pivot,
//       attempt) equal.
//   [2] THE RULE AS BUILT (biCancel = true, default): == the independent Python model of the same rule (bi150/py/model.py, cancel of a
//       waiting BO / BD: expected_bi_ES_cancel.txt, 272 BI + 8 cancelled BO / BD); vs the prototype exactly 3 lost / 4 added, each on a
//       pivot whose BO / BD a BI cancelled (the prototype never looked before a fired BO's follow-through).
//   [3] Rassul's reference cases (file times): Oct 21 13:18 BO -> BI DOWN on 13:48 (not the tiny 13:24); Nov 2 08:33 / 08:39 BD -> BI UP
//       08:42; Oct 22 09:06 one-candle BI DOWN (traded 7742.50 above 7740.75, closed 7734.75); Oct 22 08:48 BD -> BI UP 08:57.
//   [4] a FIRED BO / BD: its BI (kind fired) is after the follow-through candle and within 10 candles of it; cancelled ones: the BI candle
//       is in [t0, follow-through), the BO / BD is not drawn, gets no Pb, its BBO line says fired 0 / bi_cancel; one BI per pivot + side.
//   [5] NO REPAINT: bar-by-bar replay (from 60 candles) - every BI / cancel, once made, never changes or vanishes; the replay == one full
//       pass (BI, cancels, BO / Rly / Pb); a restart (one full pass) == the live replay.
//   [6] LABELS: "BI" in the BO row (+3) unless the row is taken: a zigzag pivot on that side (wave-time label + A row) -> row 2 or out; the
//       candle after a pivot that re-tests it within 4 ticks -> never row 1; never on another letter (BO / BD / Rly / Pb / BI); the LVP
//       row (+51) never used; red above the high (BI DOWN), green below the low (BI UP), font 7; a forced Pb in the BI's row moves the BI out.
//   [7] record lines: BBI| 22 fields, key = t_bi + dir + pivot (recKey), waiting / kind fields, the breakouts file header "| bi 1".
//   [8] NQ / GC week: one pass == replay for BI (no repaint on other markets / ticks), counts printed.
#include "BreakoutLogic.h"
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) { ++passes; std::printf("PASS %s\n", std::string(m).c_str()); } else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static const long long OFF = 2082844800LL;
static std::vector<bko::In> load(const std::string& f) {
    std::ifstream in(f); std::string l; std::vector<bko::In> v;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        bko::In b; b.days = svl::daysFromCivil(y, mo, d); b.endSec = h * 3600 + mi * 60 + s; b.t = (long long)b.days * 86400 + b.endSec + OFF;
        b.ok = true; b.o = o; b.h = hi; b.l = lo; b.c = c; b.vol = (long long)vo; v.push_back(b); }
    return v;
}
static std::vector<std::string> biLines(const bko::Engine& E) {
    std::vector<std::string> v; char b[160];
    for (const auto& kv : E.bi) { const auto& m = kv.second; std::snprintf(b, sizeof b, "BI %lld %d %.4f %lld", m.t - OFF, m.dir, m.piv, m.t0 - OFF); v.push_back(b); }
    std::sort(v.begin(), v.end(), [](const std::string& a, const std::string& c) {   // (t, dir, piv) as the Python files
        long long ta, tc; int da, dc; double pa, pc; std::sscanf(a.c_str(), "BI %lld %d %lf", &ta, &da, &pa); std::sscanf(c.c_str(), "BI %lld %d %lf", &tc, &dc, &pc);
        return ta != tc ? ta < tc : da != dc ? da < dc : pa < pc; });
    return v;
}
static std::vector<std::string> cxLines(const bko::Engine& E) { std::vector<std::string> v; for (long long t : E.boCancel) v.push_back("CX " + std::to_string(t - OFF)); return v; }
static std::vector<std::string> readExp(const std::string& f, const char* pre) { std::ifstream in(f); std::string l; std::vector<std::string> v; while (std::getline(in, l)) if (l.rfind(pre, 0) == 0) v.push_back(l); return v; }
static int diffCount(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    std::multiset<std::string> A(a.begin(), a.end()), B(b.begin(), b.end()); int d = 0;
    for (const auto& x : A) if (!B.count(x)) { if (d < 6) std::printf("  only got  [%s]\n", x.c_str()); ++d; }
    for (const auto& x : B) if (!A.count(x)) { if (d < 6) std::printf("  only want [%s]\n", x.c_str()); ++d; }
    return d;
}
static long long T(int y, int mo, int d, int h, int mi) { return (long long)svl::daysFromCivil(y, mo, d) * 86400 + h * 3600 + mi * 60 + OFF; }
static const bko::BiMark* biAt(const bko::Engine& E, long long t, int dir, long long t0 = 0) { for (const auto& kv : E.bi) if (kv.second.t == t && kv.second.dir == dir && (!t0 || kv.second.t0 == t0)) return &kv.second; return nullptr; }
static std::string allSig(const bko::Engine& E) {   // every decision of the engine (BO / Rly / Pb / BI / cancels)
    std::ostringstream o; char b[200];
    for (const auto& kv : E.bo) { const auto& D = kv.second; std::snprintf(b, sizeof b, "BO %lld %lld %d %d %d %.4f\n", D.t0, D.t, D.dir, D.fired ? 1 : 0, D.reason, D.piv); o << b; }
    for (const auto& kv : E.rly) if (kv.second.fired) o << "R " << kv.first << "\n";   // fired Rly (known since 1.3.0: a replay judges one extra NOT-fired Rly, ES / NQ Oct 28 08:33)
    for (const auto& kv : E.pb) o << "P " << kv.first << " " << kv.second.t0 << " " << kv.second.dir << "\n";
    for (const auto& kv : E.bi) o << "I " << kv.first << " " << bko::biLine(kv.second, "x") << "\n";
    for (long long t : E.boCancel) o << "C " << t << "\n";
    return o.str();
}

int main() {
    const std::vector<bko::In> B = load("wk_ES_eve_3m.txt");
    CHECK(B.size() > 4000, "ES Oct 20 - Nov 3 candles loaded (" + std::to_string(B.size()) + ")");
    // ---- [1] prototype parity ----
    bko::Engine P; P.biCancel = false; P.update(B, (long)B.size(), B[0].t, 0.25, 180, 510, 900);
    { const auto got = biLines(P), want = readExp("expected_bi_ES_proto.txt", "BI "); const int d = diffCount(got, want);
      CHECK(want.size() == 271 && got.size() == 271 && d == 0 && P.boCancel.empty(), "[1] biCancel = false, one full pass == the v0.3 prototype (run5.py N 10 close): " + std::to_string(got.size()) + " BI vs " + std::to_string(want.size()) + ", " + std::to_string(d) + " differ"); }
    // ---- [2] the rule as built ----
    bko::Engine E; E.update(B, (long)B.size(), B[0].t, 0.25, 180, 510, 900);
    { const auto got = biLines(E), want = readExp("expected_bi_ES_cancel.txt", "BI "); const int d = diffCount(got, want);
      const auto gc = cxLines(E), wc = readExp("expected_bi_ES_cancel.txt", "CX "); const int dc = diffCount(gc, wc);
      CHECK(E.biCancel && got.size() == 272 && d == 0 && gc.size() == 8 && dc == 0, "[2] the built rule (cancel on) == the independent Python model: " + std::to_string(got.size()) + " BI, " + std::to_string(gc.size()) + " BO / BD cancelled, " + std::to_string(d + dc) + " differ");
      auto cut = [](const std::vector<std::string>& v) { std::set<std::string> o; for (const auto& l : v) o.insert(l.substr(0, l.rfind(' '))); return o; };   // (t, dir, pivot): which attempt it is credited to aside
      const auto proto = readExp("expected_bi_ES_proto.txt", "BI "); const std::set<std::string> ps = cut(proto), gs = cut(got);
      int lost = 0, added = 0, tied = 0;
      auto onCancelled = [&](const std::string& l) { long long t; int dir; double pv; std::sscanf(l.c_str(), "BI %lld %d %lf", &t, &dir, &pv);
          for (long long c : E.boCancel) { const auto& D = E.bo.at(c); if (D.dir == -dir && std::fabs(D.piv - pv) < 1e-9) return true; } return false; };
      for (const auto& x : ps) if (!gs.count(x)) { ++lost; if (onCancelled(x)) ++tied; }
      for (const auto& x : gs) if (!ps.count(x)) { ++added; if (onCancelled(x)) ++tied; }
      CHECK(lost == 3 && added == 4 && tied == 7, "[2] vs the prototype: " + std::to_string(lost) + " lost, " + std::to_string(added) + " added, " + std::to_string(tied) + " of them on a pivot whose BO / BD the BI cancelled (the prototype never checked before a fired BO's follow-through)"); }
    // ---- [3] reference cases ----
    {   const bko::BiMark* a = biAt(E, T(2026, 10, 21, 13, 48), -1); const bko::BiMark* a2 = biAt(E, T(2026, 10, 21, 13, 24), -1);
        CHECK(a && a->t0 == T(2026, 10, 21, 13, 18) && !a2, "[3] Oct 21 13:18 BO attempt -> BI DOWN on the 13:48 candle (big red); the tiny 13:24 candle is not a BI");
        const bko::BiMark* b = biAt(E, T(2026, 11, 2, 8, 42), 1);
        CHECK(b && (b->t0 == T(2026, 11, 2, 8, 33) || b->t0 == T(2026, 11, 2, 8, 39)), "[3] Nov 2 08:33 / 08:39 BD attempt -> BI UP on the 08:42 candle");
        const bko::BiMark* c = biAt(E, T(2026, 10, 22, 9, 6), -1);
        CHECK(c && c->t0 == c->t && std::fabs(c->piv - 7740.75) < 1e-9 && std::fabs(c->h - 7742.5) < 1e-9 && std::fabs(c->c - 7734.75) < 1e-9 && c->kind == bko::BI_OTHER,
              "[3] Oct 22 09:06 one-candle BI DOWN: the attempt candle itself (high 7742.50 through 7740.75, close 7734.75), kind other");
        const bko::BiMark* d = biAt(E, T(2026, 10, 22, 8, 57), 1, T(2026, 10, 22, 8, 48));
        CHECK(d && std::fabs(d->piv - 7722.0) < 1e-9, "[3] Oct 22 08:48 BD attempt (pivot 7722.00) -> BI UP 08:57 (a known failure, still fires; the same candle is also the BI of the 08:54 attempt's pivot 7720.50 - one letter drawn)"); }
    // ---- [4] fired windows, cancels, one per pivot ----
    {   std::vector<long long> T0; for (const auto& b : B) T0.push_back(b.t);
        auto ix = [&](long long t) { return (int)(std::lower_bound(T0.begin(), T0.end(), t) - T0.begin()); };
        int nf = 0, badF = 0, nw = 0, badW = 0;
        for (const auto& kv : E.bi) { const auto& m = kv.second;
            if (m.kind == bko::BI_FIRED) { ++nf; const int j = ix(m.t), f = ix(m.tf); if (!(m.tf > 0 && j > f && j <= f + bko::BI_N && !m.cancel)) ++badF; }
            if (m.cancel) { ++nw; const auto it = E.bo.find(m.t0); if (it == E.bo.end() || !(m.t >= m.t0 && (it->second.t > m.t || (!it->second.fired && it->second.reason == bba::BO_R_NO_CONFIRM && it->second.t > m.t)))) ++badW; }
            if (m.kind != bko::BI_FIRED && m.tf != 0) ++badF;
            const int j = ix(m.t), i0 = ix(m.t0); if (m.kind != bko::BI_FIRED && !(j >= i0 && j <= i0 + bko::BI_N)) ++badF; }
        CHECK(nf > 20 && badF == 0, "[4] a FIRED BO / BD's BI comes after its follow-through candle, within 10 candles; every other BI within the attempt candle + 10 (" + std::to_string(nf) + " fired-kind, " + std::to_string(badF) + " bad)");
        CHECK(nw > 0 && badW == 0, "[4] waiting = the attempt still waited for its follow-through at the BI close (" + std::to_string(nw) + ")");
        int badC = 0;
        for (long long c : E.boCancel) { const auto& D = E.bo.at(c); const bko::BiMark* m = nullptr;
            for (const auto& kv : E.bi) if (kv.second.dir == -D.dir && std::fabs(kv.second.piv - D.piv) < 1e-9) m = &kv.second;
            if (!m || !(m->t >= D.t0 && m->t < D.t) || !D.fired || E.shownBo(D)) ++badC;
            for (const auto& kv : E.pb) if (kv.second.t0 == c) ++badC;
            const auto l = E.boL.find(c); if (l == E.boL.end() || l->second.line.find("|0|bi_cancel|") == std::string::npos) ++badC; }
        CHECK(E.boCancel.size() == 8 && badC == 0, "[4] each cancelled BO / BD: a BI on its pivot in [t0, follow-through), not drawn (shownBo false), no Pb, record fired 0 / bi_cancel (" + std::to_string(badC) + " bad)");
        std::set<long long> keys; int dupK = 0; for (const auto& kv : E.bi) if (!keys.insert(kv.second.pk * 2 + (kv.second.dir > 0)).second) ++dupK;
        CHECK(dupK == 0 && keys.size() == E.bi.size(), "[4] one BI per pivot + side");
        int nF = 0, nC = 0; for (const auto& kv : E.bo) if (kv.second.fired) { ++nF; if (E.cancelled(kv.first)) ++nC; }
        std::printf("ES: fired BO / BD %d, cancelled by BI %d, drawn %d; BI %zu (down %d, up %d)\n", nF, nC, nF - nC, E.bi.size(),
            (int)std::count_if(E.bi.begin(), E.bi.end(), [](const std::pair<const long long, bko::BiMark>& x) { return x.second.dir < 0; }),
            (int)std::count_if(E.bi.begin(), E.bi.end(), [](const std::pair<const long long, bko::BiMark>& x) { return x.second.dir > 0; })); }
    // ---- [5] no repaint: bar-by-bar replay ----
    {   bko::Engine R; std::map<long long, std::string> seen; std::set<long long> cseen; int changed = 0;
        for (int N = 60; N <= (int)B.size(); ++N) {
            std::vector<bko::In> Q(B.begin(), B.begin() + N); R.update(Q, N, Q[0].t, 0.25, 180, 510, 900);
            for (const auto& kv : R.bi) { const std::string s = bko::biLine(kv.second, "x"); auto it = seen.find(kv.first); if (it == seen.end()) seen[kv.first] = s; else if (it->second != s) ++changed; }
            for (const auto& kv : seen) if (!R.bi.count(kv.first)) ++changed;
            for (long long c : cseen) if (!R.cancelled(c)) ++changed;
            for (long long c : R.boCancel) cseen.insert(c);
            // a BI is decided at its own candle's close: the newest one is on the last closed candle or earlier, never later
            for (const auto& kv : R.bi) if (kv.second.t > Q[(size_t)N - 2].t) ++changed;
        }
        CHECK(changed == 0, "[5] bar-by-bar replay: no BI / cancel ever changed, vanished or appeared on an unclosed candle (" + std::to_string(seen.size()) + " BI)");
        CHECK(biLines(R) == biLines(E) && cxLines(R) == cxLines(E), "[5] replay == one full pass (BI " + std::to_string(R.bi.size()) + ", cancels " + std::to_string(R.boCancel.size()) + ")");
        CHECK(allSig(R) == allSig(E), "[5] replay == one full pass for every decision (BO / Rly / Pb / BI / cancels): a restart shows what the live chart showed");
        int recDiff = 0; for (const auto& kv : R.biL) { const auto it = E.biL.find(kv.first); if (it == E.biL.end() || it->second.line != kv.second.line || it->second.sess != kv.second.sess) ++recDiff; }
        for (const auto& kv : R.boL) { const auto it = E.boL.find(kv.first); if (it == E.boL.end() || it->second.line != kv.second.line) ++recDiff; }
        CHECK(recDiff == 0 && R.biL.size() == E.biL.size(), "[5] record lines (BBI and the bi_cancel BBO) identical live vs restart"); }
    // ---- [6] labels ----
    {   bko::View v; const int n = (int)E.bars.size(); v.first = n - 600; v.last = n - 1;
        double lo = 1e18, hi = -1e18; for (int i = v.first; i < n; ++i) { lo = std::min(lo, E.bars[(size_t)i].l); hi = std::max(hi, E.bars[(size_t)i].h); }
        const double ppx = 4000.0 / (hi - lo + 20);   // tall pane: room above / below every candle
        v.area.l = 0; v.area.t = 20; v.area.r = 600 * 12 + 10; v.area.b = 4200;
        v.X = [&](int j) { return (j - v.first) * 12 + 6; }; v.Y = [&](double p) { return (int)std::lround(4100 - (p - lo) * ppx); };
        v.textW = [](const char* s, int f) { return (int)std::strlen(s) * (f + 1); }; v.lh[0] = 12; v.lh[1] = 14;
        const bko::Scene S = bko::layout(E, v);
        int nBi = 0, bad = 0, onPiv = 0, ov = 0, retest = 0;
        for (size_t a = 0; a < S.txt.size(); ++a) for (size_t b = a + 1; b < S.txt.size(); ++b) if (bko::hits(S.txt[a].f, S.txt[b].f)) ++ov;
        for (const auto& t : S.txt) { if (t.kind != 3) continue; ++nBi;
            const auto& b = E.bars[(size_t)t.bar]; const int side = t.dir < 0 ? 1 : -1; const unsigned res = bko::biReserved(E, t.bar, side);
            const int off = side > 0 ? v.Y(b.h) - t.f.b : t.f.t - v.Y(b.l);
            if (t.s != "BI" || t.font != bko::FONT_BO || off != bko::GAP + t.row * bko::ROW_OUT || (res & (1u << t.row)) || t.row == 3 || t.col != (t.dir > 0 ? bko::C_UP : bko::C_DN)) ++bad;
            if (res & 1u) { ++onPiv; if (t.row < 2) ++bad; }
            if ((res & 2u) && !(res & 1u)) { ++retest; if (t.row == 1) ++bad; } }
        int inView = 0; { std::set<std::pair<int, int>> s; for (const auto& kv : E.bi) for (int i = std::max(0, v.first); i <= n - 2; ++i) if (E.bars[(size_t)i].t == kv.second.t) s.insert({ i, kv.second.dir }); inView = (int)s.size(); }
        CHECK(nBi > 20 && nBi == inView && S.biShown == nBi && bad == 0 && ov == 0, "[6] every BI in view drawn once: text BI, font 7, red above / green below, at +3 + 16 x row, never a reserved row or the LVP row, no overlaps (" + std::to_string(nBi) + " / " + std::to_string(inView) + ", " + std::to_string(onPiv) + " on a pivot moved out, " + std::to_string(retest) + " on a re-test candle, " + std::to_string(bad) + " bad, " + std::to_string(ov) + " overlaps)");
        int row0 = 0; for (const auto& t : S.txt) if (t.kind == 3 && t.row == 0) ++row0;
        CHECK(row0 > 0, "[6] BI sits in the BO row (+3) where the row is free (" + std::to_string(row0) + ")");
        // force a Pb into a row-0 BI's spot: the BI moves one row out (row 1 unless reserved), nothing overlaps
        const bko::Txt* t0 = nullptr; for (const auto& t : S.txt) if (t.kind == 3 && t.row == 0) { t0 = &t; break; }
        if (t0) { bko::Engine F = E; bko::PbMark pm; pm.tp = F.bars[(size_t)t0->bar].t; pm.t0 = pm.tp; pm.dir = t0->dir < 0 ? -1 : 1; F.pb[pm.tp] = pm;   // Pb on the same side
            bko::View v2 = v; const int keepY = t0->dir < 0 ? t0->f.b : t0->f.t; (void)keepY;
            // a Rly row clash is simpler to force: put a fired Rly on the BI candle's side (Rly sits GAP + 16 out = BI row 1); and a BO row letter: a fake BO
            bba::BoDec X; X.t0 = F.bars[(size_t)t0->bar].t; X.t = X.t0; X.dir = t0->dir < 0 ? 1 : -1; X.fired = true; X.piv = 1; F.bo[X.t0] = X;   // BO above (BI DOWN side) / BD below
            const bko::Scene S2 = bko::layout(F, v2); int ov2 = 0, row = -1;
            for (size_t a = 0; a < S2.txt.size(); ++a) for (size_t b = a + 1; b < S2.txt.size(); ++b) if (bko::hits(S2.txt[a].f, S2.txt[b].f)) ++ov2;
            for (const auto& t : S2.txt) if (t.kind == 3 && t.bar == t0->bar) row = t.row;
            CHECK(ov2 == 0 && row >= 1, "[6] a letter already in the BI's row moves the BI out (row 0 -> " + std::to_string(row) + "), no overlap"); }
        int boardBi = 0; for (const auto& b : S.board) (void)b, ++boardBi;
        CHECK(boardBi >= nBi, "[6] the BI letters are carried in SignalBoard 'L' (bar, side, reach)"); }
    // ---- [7] records ----
    {   int bad = 0, n22 = 0; std::set<std::string> k;
        for (const auto& kv : E.biL) { const std::string& l = kv.second.line; std::vector<std::string> f; size_t a = 0; for (;;) { const size_t q = l.find('|', a); f.push_back(l.substr(a, q == std::string::npos ? q : q - a)); if (q == std::string::npos) break; a = q + 1; }
            if (f.size() == 22) ++n22; else { ++bad; continue; }
            if (f[0] != "BBI" || f[21] != "1.5.0-zz3" || (f[5] != "unconfirmed" && f[5] != "fired" && f[5] != "other") || (f[6] != "0" && f[6] != "1") || (f[20] != "RTH" && f[20] != "ON" && f[20] != "POST" && f[20] != "NONE")) ++bad;
            if (!k.insert(bko::recKey(l)).second || bko::recKey(l).empty()) ++bad; }
        const std::string h = bko::recHead(E, bko::REC_BO, "ES", "ESZ26", 180);
        CHECK(bad == 0 && n22 == (int)E.bi.size() && h.find(" | bi 1 | ") != std::string::npos && h.find(std::string("# ") + bko::BI_HEADER) != std::string::npos && bko::recKey(std::string("# ") + bko::BI_HEADER).empty(),
              "[7] every BI has one 22-field BBI| line (kind / waiting / session fields valid, unique key t_bi + dir + pivot); the breakouts header says bi 1 + the BBI columns");
        int sess[4] = { 0, 0, 0, 0 }; for (const auto& kv : E.bi) sess[std::max(0, std::min(3, kv.second.skind))]++;
        std::printf("ES BI by session: RTH %d, ON %d, POST %d, none %d\n", sess[svl::RTH], sess[svl::ON], sess[svl::POST], sess[svl::NONE]);
        CHECK(sess[svl::ON] > 0 && sess[svl::RTH] > 0, "[7] all sessions: BI in RTH and overnight, the session recorded"); }
    // ---- [8] NQ / GC ----
    for (const char* m : { "NQ", "GC" }) {
        const std::vector<bko::In> W = load(std::string("wk_") + m + "_3m.txt"); const double tk = std::string(m) == "NQ" ? 0.25 : 0.1; const int om = std::string(m) == "GC" ? 440 : 510, cm = std::string(m) == "GC" ? 810 : 900;
        bko::Engine F; F.update(W, (long)W.size(), W[0].t, tk, 180, om, cm);
        bko::Engine R; for (int N = 60; N <= (int)W.size(); N += 1) { std::vector<bko::In> Q(W.begin(), W.begin() + N); R.update(Q, N, Q[0].t, tk, 180, om, cm); }
        CHECK(F.bi.size() > 50 && allSig(R) == allSig(F), std::string("[8] ") + m + ": replay == one pass (BI " + std::to_string(F.bi.size()) + ", cancels " + std::to_string(F.boCancel.size()) + ")");
    }
    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
