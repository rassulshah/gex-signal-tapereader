// t_cxlevel127.cpp -- LevelMarks 1.27.0 (Rassul 2026-11-26 12:40 CT "cx ... requires a level, meaning the bar with the volume needs to be crossing
// a level", 12:45 "A" = every level a Cx can name): a heavy pivot candle is a Cx only when a level sits inside it (low <= level <= high);
// the live Cx? follows the same rule when the host hands it the levels.
//   g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra -Werror t_cxlevel127.cpp -o t && ./t
#include "../LevelMarks.h"
#include <cstdio>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static wyr::Bar bar(long long t, long long o, long long h, long long l, long long c, long long vol) { wyr::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = vol / 2; b.sell = vol - vol / 2; return b; }
static std::vector<wyr::Bar> run(int after) {   // a rally, a heavy pivot-high candle (46: 1003 .. 1014), then `after` falling candles
    std::vector<wyr::Bar> B; long long t = 100000;
    for (int i = 0; i < 40; ++i, t += 180) { const long long p = 960 + (i % 4); B.push_back(bar(t, p, p + 3, p - 3, p + 1, 100)); }
    for (int i = 0; i < 6; ++i, t += 180) { const long long p = 970 + 6 * i; B.push_back(bar(t, p, p + 5, p - 2, p + 4, 120 - i)); }   // fading volume: the candle before the pivot is not a climax
    B.push_back(bar(t, 1009, 1014, 1003, 1005, 600)); t += 180;
    for (int i = 0; i < after; ++i, t += 180) { const long long p = 1000 - 5 * i; B.push_back(bar(t, p, p + 2, p - 4, p - 3, 110)); }
    return B; }
static wyr::Lvl lv(const char* n, long long px) { wyr::Lvl z; z.name = n; z.px = px; return z; }
int main() {
    C(std::string(lvm::VERSION) == "1.27.0", "version 1.27.0");
    const std::vector<wyr::Bar> B = run(8); const long long tp = B[46].t;
    { const auto M = lvm::run(B, { lv("VAH", 1100) }); C(!M.count(tp), "no level inside the heavy pivot candle: no Cx"); }
    { const auto M = lvm::run(B, {}); C(!M.count(tp), "no key levels at all (and no hour level inside): no Cx"); }
    { const auto M = lvm::run(B, { lv("VAH", 1003) }); C(M.count(tp) && M.at(tp).code == 'C' && M.at(tp).lv == "VAH", "a level at the candle's low (inside, inclusive): Cx VAH"); }
    { const auto M = lvm::run(B, { lv("VAH", 1014) }); C(M.count(tp) && M.at(tp).code == 'C', "a level at the candle's high (inclusive): Cx"); }
    { const auto M = lvm::run(B, { lv("VAH", 1015) }); C(!M.count(tp), "one tick above the high: no Cx"); }
    {   // the live Cx? (the leg still running at the heavy candle): with levels handed in it needs one inside, without them it is as before
        const std::vector<wyr::Bar> L = run(0); const std::map<long long, lvm::Mark> none; std::vector<double> pre(20, 50.0);
        const std::vector<wyr::Lvl> out = { lv("VAH", 1100) }, in = { lv("VAH", 1010) };
        C(lvm::liveCx(L, none, &pre).first == L.back().t, "live Cx? without levels handed in: as before");
        C(lvm::liveCx(L, none, &pre, &out).first == 0, "live Cx? with no level inside: none");
        C(lvm::liveCx(L, none, &pre, &in).first == L.back().t, "live Cx? with a level inside: shown"); }
    std::printf("t_cxlevel127: %d passed, %d failed\n", ok, f); return f ? 1 : 0; }
