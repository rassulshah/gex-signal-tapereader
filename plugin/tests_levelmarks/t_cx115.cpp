// t_cx115.cpp -- (1.27.0: each climax candle tested gets a level inside it - a Cx needs one now) LevelMarks 1.15.0: the Cx look-back reaches into the candles before the session (run() preV). GC 2026-11-23, his 3-minute
// candles from lsFlexLevels\ChartView\GC_180_GCZ6.json (gc1123_cx.csv: t|o|h|l|c|vol in ticks, bar END, the lines before 17:03 = the prior
// session): "why isnt this a cmx ?" - the 5:51 PM pivot low on 324 lots, the biggest in 35 candles, the 17th candle of the session.
//   g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra t_cx115.cpp -o t && ./t gc1123_cx.csv
#include "../LevelMarks.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static wyr::Bar bar(long long t, long long o, long long h, long long l, long long c, long long vol) { wyr::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = vol / 2; b.sell = vol - vol / 2; return b; }
int main(int argc, char** argv) {
    std::vector<wyr::Bar> S; std::vector<double> pre; std::vector<std::string> when; long long t551 = 0, t603 = 0;
    { std::ifstream in(argc > 1 ? argv[1] : "gc1123_cx.csv"); std::string ln; while (std::getline(in, ln)) { std::stringstream ss(ln); std::string x; std::vector<std::string> v; while (std::getline(ss, x, '|')) v.push_back(x); if (v.size() < 8) continue;
        const long long t = std::stoll(v[1]); if (v[0] == "P") { pre.push_back(std::stod(v[7])); continue; }
        S.push_back(bar(t, std::stoll(v[3]), std::stoll(v[4]), std::stoll(v[5]), std::stoll(v[6]), std::stoll(v[7]))); if (v[2] == "17:51") t551 = t; if (v[2] == "18:03") t603 = t; } }
    C(S.size() > 40 && pre.size() == 20 && t551 && t603, "fixture loaded");
    std::vector<wyr::Lvl> K;   // (1.27.0) a Cx needs a level inside its candle: one PDL / PDH through each climax candle tested here (the look-back rules are what this test checks)
    for (const wyr::Bar& b : S) { if (b.t == t551) { wyr::Lvl z; z.name = "PDL"; z.px = (b.h + b.l) / 2; K.push_back(z); } if (b.t == t603) { wyr::Lvl z; z.name = "PDH"; z.px = 36300; K.push_back(z); } }   /* 6:03 PM: low 36299, already traded at 6:00 PM, closes on the same side - inside the candle, no Bk / Sw */
    const auto old = lvm::run(S, K), now = lvm::run(S, K, -1, 180, nullptr, nullptr, &pre);
    C(!old.count(t551), "without the candles before the session: no Cx at 5:51 PM (the 17th candle) - 1.14.0");
    C(now.count(t551) && now.at(t551).code == 'C' && now.at(t551).dir == 1, "(1.15.0) Cx at the 5:51 PM pivot low, green");
    C(old.count(t603) && old.at(t603).code == 'C' && old.at(t603).dir == -1 && now.count(t603) && now.at(t603).code == 'C' && now.at(t603).dir == -1, "the 6:03 PM Cx at the pivot high stays");
    { int extra = 0, lost = 0; for (const auto& kv : now) { const auto it = old.find(kv.first); if (it == old.end()) { ++extra; C(kv.second.code == 'C', "a new mark is a Cx"); C(kv.first <= S[19].t + 180, "a new Cx is in the first 20 candles (or the candle after: its pivot)"); } else C(it->second.code == kv.second.code && it->second.dir == kv.second.dir && it->second.lv == kv.second.lv, "an old mark is unchanged"); }
      for (const auto& kv : old) { if (!now.count(kv.first)) ++lost; }
      C(lost == 0, "no mark is lost"); std::printf("marks before %zu, now %zu (+%d)\n", old.size(), now.size(), extra); }
    // the look-back rules on the real candles
    { std::vector<double> big = pre; big[18] = 100000; C(!lvm::run(S, K, -1, 180, nullptr, nullptr, &big).count(t551), "a bigger candle within the 20 before (in the prior session): no Cx"); }
    { std::vector<double> far = pre; far[0] = 100000; far[1] = 100000; far[2] = 100000; C(lvm::run(S, K, -1, 180, nullptr, nullptr, &far).count(t551), "a bigger candle more than 20 back does not count (5:51 PM is the 17th: it looks 4 candles into the prior session)"); }
    { std::vector<double> few(pre.end() - 3, pre.end()); C(!lvm::run(S, K, -1, 180, nullptr, nullptr, &few).count(t551), "under 20 candles before it in all (16 + 3): no Cx"); }
    { std::vector<double> four(pre.end() - 4, pre.end()); C(lvm::run(S, K, -1, 180, nullptr, nullptr, &four).count(t551), "exactly 20 before it (16 + 4): Cx"); }
    { std::vector<double> none; const auto e = lvm::run(S, K, -1, 180, nullptr, nullptr, &none); C(e.size() == old.size(), "an empty preV = 1.14.0"); }
    // no repaint: every prefix of the session - a mark never changes or disappears once given
    { std::map<long long, char> seen; int bad = 0; for (size_t n = 8; n <= S.size(); ++n) { std::vector<wyr::Bar> P(S.begin(), S.begin() + (long)n); const auto M = lvm::run(P, K, -1, 180, nullptr, nullptr, &pre);
        for (const auto& kv : seen) { const auto it = M.find(kv.first); if (it == M.end() || it->second.code != kv.second) ++bad; } for (const auto& kv : M) seen[kv.first] = kv.second.code; } C(bad == 0, "no repaint over every prefix (" + std::to_string(bad) + ")"); }
    std::printf("%d checks, %d failed\n", ok + f, f); return f ? 1 : 0;
}
