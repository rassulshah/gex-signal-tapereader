// t_bo3h_logic.cpp -- b3h 1.1.0 (+ SP / UT); was b3h 1.0.0 (ls3hBO 1.0.0, the indicator "3h BO"): the 3-hour BO / Tst rules on gc3h_1123.csv (his GC 180-minute candles 2026-10-13 .. 11-23): his picture,
// no repaint over every prefix, each rule, mirror, bad input.   g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra -Werror t_bo3h_logic.cpp -o tl && ./tl gc3h_1123.csv
#include "../Bo3hLogic.h"
#include <fstream>
#include <sstream>
#include <iostream>
static int fails = 0, checks = 0;
#define CK(c, msg) do { ++checks; if (!(c)) { ++fails; std::cout << "FAIL " << msg << "\n"; } } while (0)
static std::vector<b3h::Bar> load(const char* f, std::vector<std::string>& T) { std::ifstream in(f); std::string ln; std::vector<b3h::Bar> B;
    while (std::getline(in, ln)) { std::stringstream s(ln); std::string t, c; std::getline(s, t, ','); std::vector<double> v; while (std::getline(s, c, ',')) v.push_back(atof(c.c_str())); if (v.size() < 4) continue; b3h::Bar b; b.h = v[1]; b.l = v[2]; b.c = v[3]; B.push_back(b); T.push_back(t); } return B; }
static std::string key(const b3h::Mark& m, const std::vector<std::string>& T) { return T[(size_t)m.bar] + " " + b3h::text(m) + (m.live ? " live" : ""); }
int main(int argc, char** argv) {
    std::vector<std::string> T; const std::vector<b3h::Bar> B = load(argv[1], T); const bool dump = argc > 2;
    const std::vector<b3h::Mark> M = b3h::run(B);
    if (dump) for (const auto& m : M) std::cout << key(m, T) << " lv " << m.lv << "\n";
    auto has = [&](const std::vector<b3h::Mark>& V, const std::string& k) { for (const auto& m : V) if (key(m, T) == k) return true; return false; };
    // his picture (GC 180-minute, 2026-11-23 19:23 CT)
    CK(has(M, "2026-11-22 20:00:00 BO"), "BO through 3596.7"); CK(has(M, "2026-11-23 05:00:00 Tst"), "Tst 3600"); CK(has(M, "2026-11-23 08:00:00 BO"), "BO through 3636.4"); CK(has(M, "2026-11-23 20:00:00 Tst live"), "live Tst 3618.5");
    int bo = 0, bd = 0, ts = 0, sp = 0, ut = 0; for (const auto& m : M) { if (m.code == 'T') ++ts; else if (m.code == 'S') ++sp; else if (m.code == 'U') ++ut; else if (m.dir > 0) ++bo; else ++bd; }
    std::cout << "BO " << bo << " BD " << bd << " Tst " << ts << " SP " << sp << " UT " << ut << "\n";
    CK(bo == 11 && bd == 17 && ts == 10, "BO / BD / Tst unchanged by 1.1.0 (11 / 17 / 10)"); CK(sp == 13 && ut == 9, "SP 13 / UT 9 on his GC candles");
    // no repaint: every prefix (bar by bar, as a live chart) - a final mark never changes or disappears; a BO is never on the forming candle
    std::vector<std::string> fin; int liveSeen = 0;
    for (size_t k = 30; k <= B.size(); ++k) { std::vector<b3h::Bar> P(B.begin(), B.begin() + (long)k); const std::vector<b3h::Mark> V = b3h::run(P);
        for (const std::string& f : fin) CK(has(V, f), "kept " + f + " at " + std::to_string(k));
        for (const auto& m : V) { if (m.live) { ++liveSeen; CK(m.code == 'T', "only a Tst is live"); continue; } CK(m.bar < (int)k - 1, "final mark on a closed candle"); const std::string s = key(m, T); bool in = false; for (auto& f : fin) if (f == s) in = true; if (!in) fin.push_back(s); }
        const std::vector<b3h::Mark> V2 = b3h::run(P); CK(V.size() == V2.size(), "deterministic"); }
    int finalNow = 0; for (const auto& m : M) if (!m.live) ++finalNow; CK((int)fin.size() == finalNow, "final marks over all prefixes = final marks now (" + std::to_string(fin.size()) + " / " + std::to_string(finalNow) + ")");
    std::cout << "live marks seen while forming " << liveSeen << "\n";
    // each rule on every mark
    for (const auto& m : M) { const b3h::Bar& x = B[(size_t)m.bar];
        if (m.code == 'B') { CK(m.dir > 0 ? x.c > m.lv : x.c < m.lv, "BO closes beyond the pivot"); CK(x.h - x.l > B[(size_t)m.bar - 1].h - B[(size_t)m.bar - 1].l, "BO bigger than the candle before"); } }
    // forming candle only: a huge forming candle that closes through a pivot gives no BO
    { std::vector<b3h::Bar> P(B.begin(), B.end()); P.back().h += 500; P.back().c = P.back().h; const auto V = b3h::run(P); for (const auto& m : V) CK(!(m.code == 'B' && m.bar == (int)P.size() - 1), "no BO on the forming candle"); }
    // bad input
    { std::vector<b3h::Bar> P(B.begin(), B.end()); P[50].h = NAN; CK(b3h::run(P).empty(), "NaN = nothing"); CK(b3h::run(std::vector<b3h::Bar>()).empty(), "empty"); std::vector<b3h::Bar> F(200); CK(b3h::run(F).empty() || true, "flat bars no crash"); }
    // mirror: flip prices -> BO <-> BD at the same candles
    { std::vector<b3h::Bar> P; for (const auto& b : B) { b3h::Bar q; q.h = 10000 - b.l; q.l = 10000 - b.h; q.c = 10000 - b.c; P.push_back(q); } const auto V = b3h::run(P); CK(V.size() == M.size(), "mirror count");
      for (size_t i = 0; i < V.size() && i < M.size(); ++i) CK(V[i].bar == M[i].bar && V[i].code == (M[i].code == 'S' ? 'U' : M[i].code == 'U' ? 'S' : M[i].code) && V[i].dir == -M[i].dir && V[i].live == M[i].live, "mirror mark " + std::to_string(i)); }
    // (1.1.0) SP / UT rules on hand-made candles (1-bar ZigZag): h, l, c
    auto mk = [](std::vector<std::vector<double>> v) { std::vector<b3h::Bar> P; for (auto& x : v) { b3h::Bar b; b.h = x[0]; b.l = x[1]; b.c = x[2]; P.push_back(b); } return P; };
    auto codes = [&](const std::vector<b3h::Bar>& P, char c) { std::vector<int> r; for (const auto& m : b3h::run(P)) if (m.code == c) r.push_back(m.bar); return r; };
    std::vector<std::vector<double>> base;
    for (int i = 0; i < 14; ++i) base.push_back({50.0 + i, 48.0 + i, 49.5 + i});     // a steady climb (MOM warm-up)
    // pivot high H2 = 100 at bar 14, pullback to L1 = 90 at bar 16, new high 103 at bar 18 (3 past = under half of 10), confirmed at bar 19, then a forming candle
    auto seq = base; seq.push_back({100, 95, 99}); seq.push_back({97, 92, 93}); seq.push_back({94, 90, 92}); seq.push_back({98, 93, 97}); seq.push_back({103, 99, 100}); seq.push_back({101, 96, 97}); seq.push_back({99, 95, 96});
    { const auto u = codes(mk(seq), 'U'); CK(u.size() == 1 && u[0] == 18, "UT 1: 3 past H2 < half the pullback"); }
    { auto q = seq; q[18] = {108, 101, 106}; q[19] = {107, 100, 104}; const auto u = codes(mk(q), 'U'); bool on18 = false; for (int x : u) on18 |= x == 18; CK(!on18, "no UT when the candle never trades back to H2 (low 101 > 100)"); }
    { auto q = seq; q[18] = {107, 99, 106}; q[19] = {106, 98, 99}; const auto u = codes(mk(q), 'U'); bool on18 = false; for (int x : u) on18 |= x == 18; CK(on18, "UT 1: 7 past but the next candle closes back under 100"); }
    { auto q = seq; q[18] = {107, 99, 106}; q[19] = {106, 101, 105}; for (auto& b : q) { (void)b; } const auto P = mk(q); const auto u = codes(P, 'U'); bool on18 = false; for (int x : u) on18 |= x == 18;
      // 7 past, closes above 100: a UT only if momentum made a lower high than the leg into 100
      double m1 = -1e9, m2 = -1e9; for (int k = 17; k <= 18; ++k) m1 = std::max(m1, P[(size_t)k].c - P[(size_t)k - b3h::MOM_LEN].c); for (int k = 14; k <= 14; ++k) m2 = std::max(m2, P[(size_t)k].c - P[(size_t)k - b3h::MOM_LEN].c);
      CK(on18 == (m1 < m2), "UT 1 by momentum only when the leg's momentum is lower"); }
    { std::vector<b3h::Bar> P = mk(seq), Q; for (const auto& b : P) { b3h::Bar q; q.h = 1000 - b.l; q.l = 1000 - b.h; q.c = 1000 - b.c; Q.push_back(q); } const auto s2 = codes(Q, 'S'); CK(s2.size() == 1 && s2[0] == 18, "SP = the mirror"); }
    { auto q = seq; q.pop_back(); const auto u = codes(mk(q), 'U'); bool on18 = false; for (int x : u) on18 |= x == 18; CK(!on18, "not before a CLOSED candle confirms the pivot"); }
    std::cout << checks << " checks, " << fails << " failed\n"; return fails ? 1 : 0;
}
