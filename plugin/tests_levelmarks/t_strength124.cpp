// t_strength124.cpp -- LevelMarks sweep strength (1.24.0 volume only; 1.25.0, Rassul 2026-11-25 16:59 CT "In this case we should use 99+ bvmax"):
// a Sw needs ITS OWN candle's BVMax >= 99 ("99+"); range alone never qualifies; the label is "V99+". Bk keeps its own strength line.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined t_strength124.cpp -o t && ./t
#include "../LevelMarks.h"
#include <cstdio>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
int main() {
    C(std::string(lvm::VERSION) == "1.26.0" && lvm::SW_MIN == 99 && lvm::SW_BOTH_SIDES, "version 1.25.0, SW_MIN 99, both sides");
    const int n = 220; std::vector<long long> T; std::vector<double> V, R; std::vector<char> G;
    for (int i = 0; i < n; ++i) { T.push_back(1000 + 180LL * i); V.push_back(100); R.push_back(10); G.push_back(i % 2); }
    V[150] = 900;                // biggest volume in 150 candles -> 99+
    V[200] = 800; V[160] = 850;  // 160 = 850 > 800 -> 200's BV is 39 (and the same-colour count stops there too: 160 and 200 are both even) -> 39
    R[210] = 99; V[210] = 50;    // a huge range, small volume
    std::map<long long, lvm::Mark> M; for (int i : { 150, 200, 210 }) { lvm::Mark m; m.code = 'S'; m.dir = -1; m.lv = "LHI"; m.t0 = T[(size_t)i - 1]; M[T[(size_t)i]] = m; }
    lvm::Mark bk; bk.code = 'B'; bk.dir = 1; bk.lv = "ONH"; M[T[211]] = bk; V[211] = 100; R[211] = 98;
    lvm::fillStrength(M, T, V, R, G);
    C(M.count(T[150]) && M[T[150]].st == "V99+", "BVMax 99+: the Sw stays, labelled V99+");
    C(!M.count(T[200]), "BVMax 39: no Sw (needs 99+)");
    C(!M.count(T[210]), "big range, small volume: no Sw (range never qualifies)");
    C(M.count(T[211]) && !M[T[211]].st.empty() && M[T[211]].st[0] == 'R', "a Bk keeps its range strength line");
    {   // the candle's OWN volume decides: a 99+ candle right before a weak Sw candle does not carry over
        lvm::Mark m; m.code = 'S'; m.dir = 1; m.lv = "ALO"; std::map<long long, lvm::Mark> A; m.t0 = T[150]; A[T[151]] = m; std::vector<double> V2 = V; V2[151] = 1;
        lvm::fillStrength(A, T, V2, R, G); C(!A.count(T[151]), "the candle's own volume decides (a 99+ candle before it does not carry over)"); }
    std::printf("t_strength124: %d passed, %d failed\n", ok, f); return f ? 1 : 0; }
