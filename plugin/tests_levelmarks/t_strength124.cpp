// t_strength124.cpp -- LevelMarks 1.24.0 (Rassul 2026-11-25 13:56 CT "tighten strength to reuire the bigest volume in at least 20 bars", 13:59 HG 12:09 LonHI R91
// "now it would not work becasue it doesnt have BV of at least 20"): a Sw needs ITS OWN candle's BVMax >= 20; range alone no longer qualifies; the label is "V<n>".
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined t_strength124.cpp -o t && ./t
#include "../LevelMarks.h"
#include <cstdio>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
int main() {
    C(std::string(lvm::VERSION) == "1.24.2", "version 1.24.0");
    const int n = 40; std::vector<long long> T; std::vector<double> V, R; std::vector<char> G;
    for (int i = 0; i < n; ++i) { T.push_back(1000 + 180LL * i); V.push_back(100); R.push_back(10); G.push_back(i % 2); }
    V[25] = 500;                 // candle 25: the biggest volume in 25 candles (V25), ordinary range
    R[30] = 90; V[30] = 50;      // candle 30: a huge range (R30), small volume - the HG 12:09 case
    V[33] = 101; R[33] = 11;     // candle 33: biggest volume in only 2 candles (below 20) - V2
    V[36] = 120; R[36] = 12;     // candle 36: bigger than everything back to candle 25's 500? no - V10 (stops at 25)
    std::map<long long, lvm::Mark> M; for (int i : { 25, 30, 33, 36 }) { lvm::Mark m; m.code = 'S'; m.dir = -1; m.lv = "LHI"; m.t0 = T[(size_t)i - 1]; M[T[(size_t)i]] = m; }
    lvm::Mark bk; bk.code = 'B'; bk.dir = 1; bk.lv = "ONH"; M[T[30]] = M[T[30]]; M[T[31]] = bk; V[31] = 100; R[31] = 95;   // a Bk keeps the range strength
    lvm::fillStrength(M, T, V, R, G);
    C(M.count(T[25]) && M[T[25]].st == "V25", "big volume (V25): the Sw stays, labelled V25");
    C(!M.count(T[30]), "big range, small volume (R30): no Sw (HG 12:09 LonHI R91)");
    C(!M.count(T[33]), "volume biggest in 2 candles: no Sw");
    C(!M.count(T[36]), "volume biggest in 10 candles: no Sw (needs 20)");
    { lvm::Mark m; m.code = 'S'; m.dir = 1; m.lv = "ALO"; std::map<long long, lvm::Mark> A; A[T[24]] = m; A[T[25]] = m; std::vector<double> V2 = V; V2[24] = 9999; V2[25] = 1;
      lvm::fillStrength(A, T, V2, R, G); C(A.count(T[24]) && !A.count(T[25]), "the candle's OWN volume decides (a big candle before it does not carry over)"); }
    C(M.count(T[31]) && M[T[31]].st.size() > 0 && M[T[31]].st[0] == 'R', "a Bk still shows its range strength");
    {   // (1.24.1, "the vob / vot strength requires BV 20+") BV (all candles), not BVMax: a candle biggest among its own colour for 30 candles but
        // with a bigger candle of the other colour 5 back is BV 5 -> no Sw (1.24.0 kept it as BVMax 30)
        std::vector<long long> T2; std::vector<double> V2, R2; std::vector<char> G2;
        for (int i = 0; i < 70; ++i) { T2.push_back(5000 + 180LL * i); V2.push_back(100); R2.push_back(10); G2.push_back(i % 2); }
        V2[64] = 150; V2[59] = 300;   // 59 is the other colour (odd), 5 candles back
        V2[66] = 400; V2[45] = 500;   // 66: the first bigger candle is 21 back -> BV 20
        std::map<long long, lvm::Mark> A; lvm::Mark m; m.code = 'S'; m.dir = 1; m.lv = "ALO"; A[T2[64]] = m; A[T2[66]] = m;
        C(lvm::bvCount(V2, 64) == 4 && lvm::stCount(V2, G2, 64) >= 20, "candle 64: BV 4 but BVMax 20+");
        lvm::fillStrength(A, T2, V2, R2, G2);
        C(A.count(T2[64]) && A[T2[64]].st == "V" + std::to_string(lvm::stCount(V2, G2, 64)), "(1.24.2) BVMax 20+ (BV 4): the Sw stays, labelled with its BVMax");
        C(A.count(T2[66]) && A[T2[66]].st == "V" + std::to_string(lvm::stCount(V2, G2, 66)), "BVMax 20+: Sw, labelled with its BVMax - " + (A.count(T2[66]) ? A[T2[66]].st : std::string("gone"))); }
    std::printf("t_strength124: %d passed, %d failed\n", ok, f); return f ? 1 : 0; }
