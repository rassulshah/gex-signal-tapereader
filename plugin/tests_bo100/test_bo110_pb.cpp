// test_bo110_pb.cpp -- lsBreakout 1.1.0 (2026-11-03) "Pb" (a pullback pivot that TESTS the BO / BD candle), SDK-free (BreakoutLogic.h).
//   [1] the pivots = the 3-bar High / Low ZigZag of lsZzoCombo 2.1.0 (zzd::Engine::step, COMBO_REV_BARS 3) == an independent port of the
//       mockup's walk (pb.py / r4.py) on ES Oct 28 - 30 (same pivots once both walks are in step).
//   [2] PARITY ES Oct 30 (the mockup r4.py on the same 3-min bars, the same 20 BO / BD): the 9 Pb the mockup drew
//       09:00 BD(08:33), 09:51 / 10:03 / 10:24 BD(09:12), 11:00 BD(10:33), 11:42 BD(11:03), 12:45 BD(12:33), 14:12 / 14:30 BD(13:15).
//   [3] no repaint: replayed bar by bar (the chart as IRT feeds it), a Pb once made never changes or disappears; one full pass at the
//       end (a restart) gives exactly the same Pb; each Pb is decided no earlier than its pivot's confirm bar and the BO / BD's fire bar.
//   [4] the rule on hand-made bars: overlap, the next BO / BD stops it, the origin break stops it, only BO / BD fired by the decision
//       bar block; layout: Pb under a pivot LOW (green) / over a pivot HIGH (red) at 3 px, font 7, one row out when a letter is there.
// Run: g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. test_bo110_pb.cpp -o t && ./t
#include "BreakoutLogic.h"
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) { ++passes; std::printf("PASS %s\n", std::string(m).c_str()); } else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static const long long OFF = 2082844800LL;
static std::string clk(long long u) { char b[32]; const long long s = ((u % 86400) + 86400) % 86400; std::snprintf(b, sizeof b, "%02lld:%02lld", s / 3600, s / 60 % 60); return b; }
static std::vector<bko::In> load(const char* f) {
    std::ifstream in(f); std::string l; std::vector<bko::In> v;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        bko::In b; b.days = svl::daysFromCivil(y, mo, d); b.endSec = h * 3600 + mi * 60 + s; b.t = (long long)b.days * 86400 + b.endSec + OFF;
        b.ok = true; b.o = o; b.h = hi; b.l = lo; b.c = c; b.vol = (long long)vo; v.push_back(b); }
    return v;
}
// the mockup's walk (pb.py): starts up at bar 0; returns (type, bar) in order
static std::vector<std::pair<int, int>> mockPivots(const std::vector<bko::In>& B, int a, int z) {
    std::vector<std::pair<int, int>> P; int d = 1, ext = a;
    for (int i = a + 1; i <= z; ++i) { const bko::In& b = B[(size_t)i];
        if (d == 1) { if (b.h > B[(size_t)ext].h) ext = i; else if (i - a >= 3) { double m = 1e18; for (int k = i - 3; k < i; ++k) m = std::min(m, B[(size_t)k].l); if (b.l < m) { P.push_back({ 1, ext }); d = -1; ext = i; } } }
        else { if (b.l < B[(size_t)ext].l) ext = i; else if (i - a >= 3) { double m = -1e18; for (int k = i - 3; k < i; ++k) m = std::max(m, B[(size_t)k].h); if (b.h > m) { P.push_back({ -1, ext }); d = 1; ext = i; } } } }
    return P;
}
static std::string pbKey(const bko::PbMark& m) { char b[96]; std::snprintf(b, sizeof b, "%lld|%lld|%lld|%lld|%d", m.tp, m.t0, m.tc, m.td, m.dir); return b; }

static void unitRule() {
    // hand-made bars: 1 tick = 1.0; a rally, a BO candle at bar 10 (fired at 11), then pullbacks
    auto mk = [](std::vector<std::pair<double, double>> hl) { std::vector<bko::In> B; long long t = 1000000;
        for (auto& p : hl) { bko::In b; b.t = (t += 180); b.ok = true; b.days = 20000; b.endSec = (int)(t % 86400); b.h = p.first; b.l = p.second; b.o = (b.h + b.l) / 2; b.c = b.o; b.vol = 100; B.push_back(b); } return B; };
    //                0          1          2          3          4         5         6         7         8          9          10 BO      11 FT      12         13         14 (pivot low, overlaps)  15        16        17        18
    std::vector<bko::In> B = mk({ {105,100},{104,99},{103,98},{102,97},{101,95},{103,96},{104,97},{106,99},{107,100},{108,101},{112,104},{114,108},{113,107},{111,106},{110,105},{113,107},{115,109},{116,110},{117,111},{118,112},{119,113} });
    bko::Engine E; E.bars = B; E.tick = 1; E.spb = 180; E.count = (long)B.size();
    bba::BoDec D; D.t0 = B[10].t; D.t = B[11].t; D.dir = 1; D.fired = true; D.i0 = 10; D.im = 11; E.bo[D.t0] = D;
    E.pbPass();
    bool low14 = E.pb.count(B[14].t) && E.pb[B[14].t].dir == 1 && E.pb[B[14].t].t0 == B[10].t;
    CHECK(low14 && E.pb.size() == 1, "[4] the pivot LOW after the BO whose bar overlaps the BO candle gets Pb (green side), nothing else (" + std::to_string(E.pb.size()) + ")");
    { const auto& m = E.pb[B[14].t]; CHECK(m.tc == B[16].t && m.td == B[16].t, "[4] decided at the confirm bar (bar 16: the first high above the previous 3 highs; bar 15 only equals 113)"); }
    // no overlap: the pullback stays above the BO high
    { std::vector<bko::In> C = B; for (int k = 12; k <= 14; ++k) { C[(size_t)k].l += 10; C[(size_t)k].h += 10; } for (size_t k = 15; k < C.size(); ++k) { C[k].l += 10; C[k].h += 10; }
      bko::Engine F; F.bars = C; F.tick = 1; F.spb = 180; F.bo = E.bo; F.pbPass(); CHECK(F.pb.empty(), "[4] a pivot low above the BO candle's high: no Pb"); }
    // a next BD fired before the pivot is confirmed, its candle before the pivot: blocks
    { bko::Engine F = E; F.pb.clear(); bba::BoDec X; X.t0 = B[12].t; X.t = B[13].t; X.dir = -1; X.fired = true; F.bo[X.t0] = X; F.pbPass(); CHECK(F.pb.empty(), "[4] the next BO / BD (fired by the decision bar) stops the Pb"); }
    // the next one fired only AFTER the decision bar: does not block (the chart could not know it); not fired at all: does not block
    { bko::Engine F = E; F.pb.clear(); bba::BoDec X; X.t0 = B[12].t; X.t = B[18].t; X.dir = -1; X.fired = true; F.bo[X.t0] = X; F.pbPass(); CHECK(F.pb.count(B[14].t) == 1, "[4] a later BO / BD fired after the decision bar does not remove / block the Pb"); }
    { bko::Engine F = E; F.pb.clear(); bba::BoDec X; X.t0 = B[12].t; X.t = B[13].t; X.dir = -1; X.fired = false; F.bo[X.t0] = X; F.pbPass(); CHECK(F.pb.count(B[14].t) == 1, "[4] a BO / BD candidate that never fired does not block"); }
    // origin break: a bar between the BO and the pivot trades below the wave's origin low (95, bar 4)
    { std::vector<bko::In> C = B; C[13].l = 94; bko::Engine F; F.bars = C; F.tick = 1; F.spb = 180; F.bo = E.bo; F.pbPass(); CHECK(F.pb.empty(), "[4] price below the BO wave's origin pivot low before the pivot: no Pb"); }
    // kept: once made, a later pass never removes it (even if the BO record vanished)
    { bko::Engine F = E; F.bo.clear(); F.pbPass(); CHECK(F.pb.count(B[14].t) == 1, "[4] a made Pb is never removed by a later pass"); }
    // layout: green under the low, 3 px, font 7; one row out when a BD sits on the same bar
    {   bko::View v; v.first = 0; v.last = (int)B.size() - 1; v.area = { 0, 0, 2000, 2000 };
        v.X = [](int j) { return 20 + j * 10; }; v.Y = [](double p) { return (int)std::lround(1500 - p * 10); }; v.textW = [](const char* s, int) { return (int)std::strlen(s) * 6; };
        E.bars = B; bko::Scene S = bko::layout(E, v); const bko::Txt* T = nullptr; for (const auto& t : S.txt) if (t.kind == 2) T = &t;
        CHECK(T && T->s == "Pb" && T->col == bko::C_UP && T->font == bko::FONT_BO && T->f.t == v.Y(B[14].l) + 3 && T->bar == 14 && S.pbShown == 1, "[4] layout: green Pb at Y(low) + 3, font 7, on the pivot bar");
        bool post = false; for (const auto& b : S.board) if (b.t == B[14].t && b.side == -1) post = true; CHECK(post, "[4] SignalBoard 'L' carries the Pb bar (below)");
        bko::Engine G = E; bba::BoDec X; X.t0 = B[14].t; X.t = B[20].t; X.dir = -1; X.fired = true; G.bo[X.t0] = X;
        S = bko::layout(G, v); const bko::Txt* bd = nullptr; T = nullptr; for (const auto& t : S.txt) { if (t.kind == 2) T = &t; if (t.kind == 0 && t.bar == 14) bd = &t; }
        CHECK(T && bd && !bko::hits(T->f, bd->f) && T->f.t == bd->f.b + 1, "[4] a BD on the same bar: Pb one row further out, no overlap");
        bko::Engine H2 = E; for (auto& kv : H2.pb) kv.second.dir = -1; H2.bars[14].h = 111;
        S = bko::layout(H2, v); T = nullptr; for (const auto& t : S.txt) if (t.kind == 2) T = &t;
        CHECK(T && T->col == bko::C_DN && T->f.b == v.Y(H2.bars[14].h) - 3, "[4] layout: red Pb (after a BD) at Y(high) - 3 - height"); }
}

int main() {
    unitRule();
    const std::vector<bko::In> B = load("es_2026-10-28_30_3m.txt"); const int n = (int)B.size();
    CHECK(n > 1000, "ES Oct 28 - 30 3-min bars (" + std::to_string(n) + ")");
    if (n <= 1000) { std::printf("RESULT %d passed, %d failed (run from plugin/tests_bo100)\n", passes, fails); return 1; }
    // live replay, as test_bo100_parity: the chart grows bar by bar (the last bar forming)
    bko::Engine E; std::map<long long, std::string> seen; int changed = 0, early = 0;
    for (int N = 60; N <= n; ++N) {
        std::vector<bko::In> P(B.begin(), B.begin() + N);
        E.update(P, N, P[0].t, 0.25, 180, 510, 900);
        const long long lastClosed = P[(size_t)N - 2].t;
        for (const auto& kv : E.pb) { const std::string k = pbKey(kv.second); auto it = seen.find(kv.first);
            if (it == seen.end()) { seen[kv.first] = k; if (kv.second.td != lastClosed) ++early; } else if (it->second != k) ++changed; }
        for (const auto& kv : seen) if (!E.pb.count(kv.first)) ++changed;
    }
    CHECK(changed == 0, "[3] no repaint: " + std::to_string(seen.size()) + " Pb over the replay, " + std::to_string(changed) + " changed / vanished");
    CHECK(early == 0, "[3] every Pb appears at the close of its decision bar (max of pivot confirm, BO / BD fire), none earlier or later (" + std::to_string(early) + ")");
    {   bko::Engine F; F.update(B, n, B[0].t, 0.25, 180, 510, 900); int same = 0, diff = 0;
        for (const auto& kv : E.pb) { const auto it = F.pb.find(kv.first); (it != F.pb.end() && pbKey(it->second) == pbKey(kv.second)) ? ++same : ++diff; }
        for (const auto& kv : F.pb) if (!E.pb.count(kv.first)) ++diff;
        CHECK(diff == 0 && same == (int)E.pb.size(), "[3] restart (one full pass) == the live replay: " + std::to_string(same) + " same, " + std::to_string(diff) + " different"); }
    // [1] pivots vs the mockup walk over Oct 30 RTH (both started at the first RTH bar, as the mockup)
    int a = -1, z = -1; for (int i = 0; i < n; ++i) { const long long u = B[(size_t)i].t - OFF; if (B[(size_t)i].days == svl::daysFromCivil(2026, 10, 30) && B[(size_t)i].endSec >= 8 * 3600 + 33 * 60 && B[(size_t)i].endSec <= 15 * 3600) { if (a < 0) a = i; z = i; } (void)u; }
    {   const std::vector<std::pair<int, int>> M = mockPivots(B, a, z);
        std::set<std::pair<long long, int>> mp, ep; for (const auto& p : M) mp.insert({ B[(size_t)p.second].t, p.first });
        for (const bko::Pivot& p : E.piv) if (p.t >= B[(size_t)a].t && p.t <= B[(size_t)z].t) ep.insert({ p.t, p.type });
        // compare once both walks are in step: from the first pivot they share
        long long sync = 0; for (const auto& p : mp) if (ep.count(p)) { sync = p.first; break; }
        int only1 = 0, only2 = 0; for (const auto& p : mp) if (p.first >= sync && !ep.count(p) && p.first < B[(size_t)z - 3].t) { ++only1; std::printf("  pivot only in the mockup walk %s %s\n", clk(p.first - OFF).c_str(), p.second > 0 ? "H" : "L"); }
        for (const auto& p : ep) if (p.first >= sync && !mp.count(p) && p.first < B[(size_t)z - 3].t) { ++only2; std::printf("  pivot only in the plugin %s %s\n", clk(p.first - OFF).c_str(), p.second > 0 ? "H" : "L"); }
        CHECK(sync > 0 && only1 == 0 && only2 == 0, "[1] Oct 30 RTH pivots: the plugin (full session, zzd::Engine::step N=3) == the mockup walk from " + clk(sync - OFF) + " on (" + std::to_string(mp.size()) + " mockup pivots)");
        std::printf("first shared pivot %s; mockup's first pivots:", clk(sync - OFF).c_str()); for (size_t k = 0; k < M.size() && k < 4; ++k) std::printf(" %s%s", clk(B[(size_t)M[k].second].t - OFF).c_str(), M[k].first > 0 ? "H" : "L"); std::printf("\n"); }
    // [2] parity with the mockup's 9 Pb
    const char* want[] = { "09:00|-1|08:33", "09:51|-1|09:12", "10:03|-1|09:12", "10:24|-1|09:12", "11:00|-1|10:33", "11:42|-1|11:03", "12:45|-1|12:33", "14:12|-1|13:15", "14:30|-1|13:15" };
    std::set<std::string> W(std::begin(want), std::end(want)), G;
    for (const auto& kv : E.pb) { const long long u = kv.first - OFF; if (u < (long long)svl::daysFromCivil(2026, 10, 30) * 86400 + 8 * 3600 + 33 * 60 || u > (long long)svl::daysFromCivil(2026, 10, 30) * 86400 + 15 * 3600) continue;
        G.insert(clk(u) + "|" + std::to_string(kv.second.dir) + "|" + clk(kv.second.t0 - OFF)); }
    int m = 0; for (const auto& w : W) { if (G.count(w)) ++m; else std::printf("  MISSING Pb %s\n", w.c_str()); }
    int x = 0; for (const auto& g : G) if (!W.count(g)) { ++x; std::printf("  EXTRA   Pb %s\n", g.c_str()); }
    std::printf("PB PARITY: %d / %zu mockup Pb matched, %d extra (Oct 30 08:33 - 15:00)\n", m, W.size(), x);
    for (const auto& g : G) std::printf("  Pb %s\n", g.c_str());
    CHECK(m == (int)W.size() && x == 0, "[2] Pb parity ES Oct 30 with the mockup (" + std::to_string(m) + " / 9 matched, " + std::to_string(x) + " extra)");
    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
