// t_cx126.cpp -- LevelMarks 1.26.0 (Rassul 2026-11-26 12:23 CT "why is this not a cmx" - ES 10:15 AM HOD, 1,150 lots = the biggest in 32,
// closed back under ONH / LonHI / AsiaHI): a sweep candle too light for a VOB / VOT (own BVMax < 99) used to block the Cx step and was then
// dropped by fillStrength, leaving NO mark. run(noSw = swWeak()) frees the candle before the Bk / Cx steps: it becomes the Cx.
//   g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra -Werror t_cx126.cpp -o t && ./t
#include "../LevelMarks.h"
#include <cstdio>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static wyr::Bar bar(long long t, long long o, long long h, long long l, long long c, long long vol) { wyr::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = vol / 2; b.sell = vol - vol / 2; return b; }
static void arrays(const std::vector<wyr::Bar>& B, std::vector<long long>& T, std::vector<double>& V, std::vector<double>& R, std::vector<char>& G) {
    for (const wyr::Bar& b : B) { T.push_back(b.t); V.push_back((double)(b.buy + b.sell)); R.push_back((double)(b.h - b.l)); G.push_back(b.c >= b.o ? 1 : 0); } }
static std::vector<wyr::Bar> session(long long sweepVol) {
    std::vector<wyr::Bar> B; long long t = 100000;
    for (int i = 0; i < 40; ++i, t += 180) { const long long p = 960 + (i % 4); B.push_back(bar(t, p, p + 3, p - 3, p + 1, 100)); }           // quiet base, highs < 970
    for (int i = 0; i < 6; ++i, t += 180) { const long long p = 970 + 6 * i; B.push_back(bar(t, p, p + 5, p - 2, p + 4, 120)); }             // rally into the level (highs 975 .. 1005)
    B.push_back(bar(t, 1004, 1014, 1003, 1006, sweepVol)); t += 180;                                                                         // 46: up through PDH 1010, closes back under it (red? no: 1006 > 1004 green) -> make it red below
    B.back().o = 1009; B.back().c = 1005;
    for (int i = 0; i < 8; ++i, t += 180) { const long long p = 1000 - 5 * i; B.push_back(bar(t, p, p + 2, p - 4, p - 3, 110)); }          // falling: pivot high at 46 confirmed
    return B; }
int main() {
    C(std::string(lvm::VERSION) == "1.27.0", "version 1.26.0");
    std::vector<wyr::Lvl> K; { wyr::Lvl z; z.name = "PDH"; z.px = 1010; z.high = true; K.push_back(z); }
    const int sw = 46;
    {   // light sweep candle: biggest volume in 46 (>= 20 for a Cx, < 99 for a Sw)
        const std::vector<wyr::Bar> B = session(500); std::vector<long long> T; std::vector<double> V, R; std::vector<char> G; arrays(B, T, V, R, G);
        std::map<long long, lvm::Mark> old = lvm::run(B, K); const bool wasSw = old.count(B[sw].t) && old[B[sw].t].code == 'S';
        lvm::fillStrength(old, T, V, R, G);
        C(wasSw, "1.25.0 path: the candle is a Sw candidate first");
        C(!old.count(B[sw].t), "1.25.0 path: after fillStrength (BVMax < 99) the candle has NO mark - the bug");
        const std::set<long long> W = lvm::swWeak(T, V, G); C(W.count(B[sw].t) == 1, "swWeak lists the light sweep candle");
        std::map<long long, lvm::Mark> now = lvm::run(B, K, -1, 0, nullptr, nullptr, nullptr, &W); lvm::fillStrength(now, T, V, R, G);
        C(now.count(B[sw].t) && now[B[sw].t].code == 'C' && now[B[sw].t].dir == -1, "1.26.0: the candle is a Cx at the pivot high");
        C(now.count(B[sw].t) && now[B[sw].t].lv == "PDH", "1.26.0: the Cx names the level it traded through (PDH)");
        C(old.count(B[sw - 1].t) && old[B[sw - 1].t].code == 'C' && !now.count(B[sw - 1].t), "1.25.0 had put the Cx on the candle BEFORE the pivot (the pivot was taken); now it sits on the pivot candle itself");
        int changed = 0; for (const auto& kv : old) { if (kv.first == B[sw - 1].t) continue; const auto it = now.find(kv.first); if (it == now.end() || it->second.code != kv.second.code) ++changed; }
        C(changed == 0, "no other mark changes"); }
    {   // heavy sweep candle: biggest in 99+ -> still the Sw (VOT), not the Cx
        std::vector<wyr::Bar> B = session(5000); std::vector<wyr::Bar> pre; long long t0 = B.front().t - 180 * 70;
        for (int i = 0; i < 70; ++i) pre.push_back(bar(t0 + 180 * i, 950, 952, 948, 951, 90));
        B.insert(B.begin(), pre.begin(), pre.end()); const int s2 = sw + 70;
        std::vector<long long> T; std::vector<double> V, R; std::vector<char> G; arrays(B, T, V, R, G);
        const std::set<long long> W = lvm::swWeak(T, V, G); C(W.count(B[s2].t) == 0, "a 99+ candle is not in swWeak");
        std::map<long long, lvm::Mark> now = lvm::run(B, K, -1, 0, nullptr, nullptr, nullptr, &W); lvm::fillStrength(now, T, V, R, G);
        C(now.count(B[s2].t) && now[B[s2].t].code == 'S' && now[B[s2].t].st == "V99+", "a heavy sweep stays the Sw (VOT) V99+"); }
    {   // no repaint: every prefix with its own swWeak
        const std::vector<wyr::Bar> B = session(500); std::map<long long, char> seen; int bad = 0;
        for (size_t n = 8; n <= B.size(); ++n) { std::vector<wyr::Bar> P(B.begin(), B.begin() + (long)n); std::vector<long long> T; std::vector<double> V, R; std::vector<char> G; arrays(P, T, V, R, G);
            const std::set<long long> W = lvm::swWeak(T, V, G); std::map<long long, lvm::Mark> M = lvm::run(P, K, -1, 0, nullptr, nullptr, nullptr, &W); lvm::fillStrength(M, T, V, R, G);
            for (const auto& kv : seen) { const auto it = M.find(kv.first); if (it == M.end() || it->second.code != kv.second) ++bad; } for (const auto& kv : M) seen[kv.first] = kv.second.code; }
        C(bad == 0, "no repaint over every prefix (" + std::to_string(bad) + ")"); }
    std::printf("t_cx126: %d passed, %d failed\n", ok, f); return f ? 1 : 0; }
