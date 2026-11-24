// t_bo3h_logic.cpp -- b3h 1.0.0 (ls3hBO 1.0.0, the indicator "3h BO"): the 3-hour BO / Tst rules on gc3h_1123.csv (his GC 180-minute candles 2026-10-13 .. 11-23): his picture,
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
    int bo = 0, bd = 0, ts = 0; for (const auto& m : M) { if (m.code == 'T') ++ts; else if (m.dir > 0) ++bo; else ++bd; } std::cout << "BO " << bo << " BD " << bd << " Tst " << ts << "\n";
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
      for (size_t i = 0; i < V.size() && i < M.size(); ++i) CK(V[i].bar == M[i].bar && V[i].code == M[i].code && V[i].dir == -M[i].dir && V[i].live == M[i].live, "mirror mark " + std::to_string(i)); }
    std::cout << checks << " checks, " << fails << " failed\n"; return fails ? 1 : 0;
}
