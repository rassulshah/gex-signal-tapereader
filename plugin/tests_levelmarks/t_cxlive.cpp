// t_cxlive.cpp -- LevelMarks 1.16.0 liveCx: the climax candle shows "Cx?" from its own close. HG 2026-11-23, his 3-minute candles from
// lsFlexLevels\ChartView\HG_180_HGZ6.json (hg1123_cxlive.csv: P = the 20 candles before the session, B = the session; prices in ticks):
// Rassul 23:18 CT "the CX came late .. why ?" - the 11:03 PM candle (46 lots, the 1hL sweep) was drawn Cx only at 11:12 PM.
//   g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra t_cxlive.cpp -o t && ./t hg1123_cxlive.csv
#include "../LevelMarks.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static wyr::Bar bar(long long t, long long o, long long h, long long l, long long c, long long vol) { wyr::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = vol / 2; b.sell = vol - vol / 2; return b; }
int main(int argc, char** argv) {
    std::vector<wyr::Bar> S; std::vector<double> pre; std::map<long long, std::string> hm; long long t1103 = 0, t1112 = 0;
    { std::ifstream in(argc > 1 ? argv[1] : "hg1123_cxlive.csv"); std::string ln; while (std::getline(in, ln)) { std::stringstream ss(ln); std::string x; std::vector<std::string> v; while (std::getline(ss, x, '|')) v.push_back(x); if (v.size() < 8) continue;
        const long long t = std::stoll(v[1]); if (v[0] == "P") { pre.push_back(std::stod(v[7])); continue; }
        S.push_back(bar(t, std::stoll(v[3]), std::stoll(v[4]), std::stoll(v[5]), std::stoll(v[6]), std::stoll(v[7]))); hm[t] = v[2]; if (v[2] == "23:03") t1103 = t; if (v[2] == "23:12") t1112 = t; } }
    C(S.size() > 100 && pre.size() == 20 && t1103 && t1112, "fixture loaded");
    const std::vector<wyr::Lvl> K;
    // replay the session candle by candle as closed candles (the pane runs on the closed ones): when is the Cx decided, when does the Cx? show
    long long firstLive = 0, firstCx = 0; int liveWhileCx = 0, flips = 0; long long prevLive = 0;
    for (size_t n = 8; n <= S.size(); ++n) { std::vector<wyr::Bar> P(S.begin(), S.begin() + (long)n); const auto M = lvm::run(P, K, -1, 180, nullptr, nullptr, &pre); const auto L = lvm::liveCx(P, M, &pre);
        if (!firstCx && M.count(t1103) && M.at(t1103).code == 'C') firstCx = P.back().t;
        if (!firstLive && L.first == t1103) firstLive = P.back().t;
        if (L.first && M.count(L.first)) ++liveWhileCx;
        if (L.first != prevLive && prevLive && L.first) ++flips; prevLive = L.first; }
    std::printf("Cx? at %s, Cx decided at %s; the Cx? moved to another candle %d times in the session\n", hm[firstLive].c_str(), hm[firstCx].c_str(), flips);
    C(firstLive == t1103, "Cx? shows at the close of the 11:03 PM candle itself");
    C(firstCx == t1103 + 180, "the Cx itself is decided when the 3-bar ZigZag confirms: the 11:06 PM candle's high went over the 3 candles before");
    C(liveWhileCx == 0, "never a Cx? on a candle that already has a mark");
    // the Cx it later becomes is the same candle and side
    { const auto M = lvm::run(S, K, -1, 180, nullptr, nullptr, &pre); C(M.count(t1103) && M.at(t1103).dir == 1, "the 11:03 PM Cx: green, at the low"); }
    // a lower low with less volume after it: the Cx? goes (the leg's extreme moved, the new candle is not the biggest)
    { std::vector<wyr::Bar> P; for (const auto& b : S) { P.push_back(b); if (b.t == t1103) break; }
      wyr::Bar x = P.back(); x.t += 180; x.h = x.l + 2; x.l -= 10; x.c = x.l + 1; x.buy = 1; x.sell = 1; P.push_back(x);
      C(lvm::liveCx(P, lvm::run(P, K, -1, 180, nullptr, nullptr, &pre), &pre).first == t1103, "one new low on low volume: the Cx? stays on 11:03 PM (the candle before the pivot counts, as for the Cx)");
      wyr::Bar y = P.back(); y.t += 180; y.h = y.l + 2; y.l -= 10; y.c = y.l + 1; P.push_back(y);
      C(lvm::liveCx(P, lvm::run(P, K, -1, 180, nullptr, nullptr, &pre), &pre).first != t1103, "two lower lows: no Cx? on 11:03 PM any more"); }
    // a quiet candle at the extreme: no Cx?
    { std::vector<wyr::Bar> P; for (const auto& b : S) { P.push_back(b); if (b.t == t1103) break; } P.back().buy = 1; P.back().sell = 1; P[P.size() - 2].buy = 1; P[P.size() - 2].sell = 1;
      const auto L = lvm::liveCx(P, lvm::run(P, K, -1, 180, nullptr, nullptr, &pre), &pre); C(L.first == 0, "not the biggest volume: no Cx?"); }
    C(lvm::liveCx(std::vector<wyr::Bar>(), std::map<long long, lvm::Mark>()).first == 0, "no candles");
    std::printf("%d checks, %d failed\n", ok + f, f); return f ? 1 : 0;
}
