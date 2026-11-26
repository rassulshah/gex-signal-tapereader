// t_cxclose128.cpp -- LevelMarks 1.28.0 (Rassul 2026-11-26 13:06-13:16 CT "its close would not close over the level", "in any buying climax, no bar
// closes above the level", "no" to also checking the confirmation bars): a Cx level is one the climax failed to close beyond - the climax candle and,
// when the climax is the candle before the pivot, the pivot candle too; mirror for a selling climax. Bars after the pivot are not checked.
//   g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra -Werror t_cxclose128.cpp -o t && ./t
#include "../LevelMarks.h"
#include <cstdio>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static wyr::Bar bar(long long t, long long o, long long h, long long l, long long c, long long vol) { wyr::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = vol / 2; b.sell = vol - vol / 2; return b; }
static wyr::Lvl lv(const char* n, long long px) { wyr::Lvl z; z.name = n; z.px = px; return z; }
// a rally, [climax candle], [pivot candle], 8 falling candles; sign -1 mirrors everything (a selling climax at a pivot low)
static std::vector<wyr::Bar> make(const std::vector<wyr::Bar>& top, int sign) {
    std::vector<wyr::Bar> B; long long t = 100000;
    auto add = [&](long long o, long long h, long long l, long long c, long long v) { if (sign > 0) B.push_back(bar(t, o, h, l, c, v)); else B.push_back(bar(t, 2000 - o, 2000 - l, 2000 - h, 2000 - c, v)); t += 180; };
    for (int i = 0; i < 40; ++i) { const long long p = 960 + (i % 4); add(p, p + 3, p - 3, p + 1, 100); }
    for (int i = 0; i < 6; ++i) { const long long p = 970 + 6 * i; add(p, p + 5, p - 2, p + 4, 120 - i); }
    for (const wyr::Bar& b : top) add(b.o, b.h, b.l, b.c, b.buy + b.sell);
    for (int i = 0; i < 8; ++i) { const long long p = 990 - 5 * i; add(p, p + 2, p - 4, p - 3, 110); }
    return B; }
int main() {
    C(std::string(lvm::VERSION) == "1.28.0", "version 1.28.0");
    for (int sign : { 1, -1 }) {
        const std::string sd = sign > 0 ? "buying climax (pivot high): " : "selling climax (pivot low, mirror): ";
        auto L = [&](long long px) { return std::vector<wyr::Lvl>{ lv("VAH", sign > 0 ? px : 2000 - px) }; };
        {   // 1 the climax IS the pivot candle, green, closes back under the level (wick through): Cx
            const auto B = make({ bar(0, 1003, 1014, 1002, 1008, 600) }, sign); const long long tp = B[46].t;
            const auto M = lvm::run(B, L(1010)); C(M.count(tp) && M.at(tp).code == 'C', sd + "1 pivot candle closes under the level: Cx"); }
        {   // 2 the pivot candle closes OVER the level: a breakout, no Cx
            const auto B = make({ bar(0, 1003, 1014, 1002, 1012, 600) }, sign); const long long tp = B[46].t;
            const auto M = lvm::run(B, L(1010)); C(!M.count(tp) || M.at(tp).code != 'C', sd + "2 pivot candle closes over the level: no Cx"); }
        {   // 3 red, opened above, closes under (ES 11-26 10:15 AM at ONH): Cx
            const auto B = make({ bar(0, 1012, 1014, 1004, 1006, 600) }, sign); const long long tp = B[46].t;
            const auto M = lvm::run(B, L(1010)); C(M.count(tp) && M.at(tp).code == 'C', sd + "3 red candle opened over, closed under: Cx"); }
        {   // 4 close exactly AT the level: not beyond -> Cx
            const auto B = make({ bar(0, 1003, 1014, 1002, 1010, 600) }, sign); const long long tp = B[46].t;
            const auto M = lvm::run(B, L(1010)); C(M.count(tp) && M.at(tp).code == 'C', sd + "4 close exactly at the level: Cx"); }
        {   // 5 the climax is the candle BEFORE the pivot; both close under: Cx on the climax candle
            const auto B = make({ bar(0, 1003, 1012, 1002, 1006, 600), bar(0, 1006, 1013, 1004, 1007, 200) }, sign); const long long tc = B[46].t;
            const auto M = lvm::run(B, L(1010)); C(M.count(tc) && M.at(tc).code == 'C', sd + "5 climax before the pivot, both close under: Cx on the climax candle"); }
        {   // 5b the climax closes under but the PIVOT candle closes over: no Cx at that level
            const auto B = make({ bar(0, 1003, 1012, 1002, 1006, 600), bar(0, 1006, 1013, 1004, 1011, 200) }, sign); const long long tc = B[46].t, tp = B[47].t;
            const auto M = lvm::run(B, L(1010)); C(!(M.count(tc) && M.at(tc).code == 'C') && !(M.count(tp) && M.at(tp).code == 'C'), sd + "5b the pivot candle after the climax closes over: no Cx"); }
        {   // 6 the climax candle itself closes over (a breakout bar), then a failing pivot: no Cx
            const auto B = make({ bar(0, 1003, 1012, 1002, 1011, 600), bar(0, 1011, 1013, 1005, 1006, 200) }, sign); const long long tc = B[46].t, tp = B[47].t;
            const auto M = lvm::run(B, L(1010)); C(!(M.count(tc) && M.at(tc).code == 'C') && !(M.count(tp) && M.at(tp).code == 'C'), sd + "6 the climax candle closed over: no Cx"); }
        {   // the bars AFTER the pivot are not checked ("no"): one closes back over the level -> still Cx
            std::vector<wyr::Bar> B = make({ bar(0, 1003, 1014, 1002, 1008, 600) }, sign); const long long tp = B[46].t;
            wyr::Bar& a = B[47]; if (sign > 0) { a.o = 1007; a.h = 1012; a.l = 1005; a.c = 1011; } else { a.o = 993; a.h = 995; a.l = 988; a.c = 989; }
            const auto M = lvm::run(B, L(1010)); C(M.count(tp) && M.at(tp).code == 'C', sd + "a candle after the pivot closing over is not checked: Cx"); }
    }
    std::printf("t_cxclose128: %d passed, %d failed\n", ok, f); return f ? 1 : 0; }
