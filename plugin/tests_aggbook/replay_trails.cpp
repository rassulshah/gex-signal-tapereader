// Run: python3 replay_trails_feed.py <raw MBO part.parquet> | ./replay_trails NQ out.txt   (build: g++ -std=c++17 -O2 -I.. replay_trails.cpp -o replay_trails)
// replay_trails.cpp - MoveLogic 1.2.0 trails on a recorded real MBO day, through the REAL shared engine (OrderFlowFeed.h Engine with
// trackMoves + trailMin, the same code lsBookFlow runs), then the chart side (TrailSet + decideStops on 3-min candles, gap 0.5 ATR, N 2).
// stdin: packed records {int64 ts_ns; int8 act; int8 side; int32 sz; int64 tick; uint64 oid} (act A M C F T R; side B A N).
#include "BidAskStatsLogic.h"
#include "MoveLogic.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <map>
#include <algorithm>
#pragma pack(push, 1)
struct R { int64_t ts; int8_t act; int8_t side; int32_t sz; int64_t tk; uint64_t oid; };
#pragma pack(pop)
int main(int argc, char** argv) {
    const std::string mkt = argc > 1 ? argv[1] : "NQ"; const int spb = 180; const char* outLines = argc > 2 ? argv[2] : nullptr;
    off::Engine g; g.near = 40; g.trackMoves = true; g.mvt.minLots = mvl::minLotsOf(mkt); g.mvt.trailMin = mvl::trailMinOf(mkt);
    std::vector<std::string> lines; lines.push_back("RS|0|0.25");
    std::map<long long, mvl::TBar> bars; long long curKey = -1; off::Acc acc; long long n = 0, trades = 0;
    R r;
    while (std::fread(&r, sizeof r, 1, stdin) == 1) {
        ++n;
        const long long sec = r.ts / 1000000000LL, ms = r.ts / 1000000LL;
        const long long key = (sec / spb + 1) * spb;                       // the candle END (IRT bar time)
        if (key != curKey) { curKey = key; acc = off::Acc();               // a new candle: the near band from the last 14 closed candles (as lsBookFlow: max(1 ATR, the zone))
            std::vector<double> rg; for (auto it = bars.rbegin(); it != bars.rend() && rg.size() < 14; ++it) if (it->first < key) rg.push_back((double)(it->second.hi - it->second.lo) * 0.25);
            double atr = 0; for (double x : rg) atr += x; atr = rg.empty() ? 0 : atr / rg.size();
            g.near = nl::nearBand(atr, 0.25, nl::zoneFor(mkt), 0); }
        g.curKey = key;
        if (r.act == 'T') { if (r.tk > 0) { auto it = bars.find(key); if (it == bars.end()) { mvl::TBar b; b.key = key; b.hi = b.lo = b.cl = r.tk; bars[key] = b; } else { it->second.hi = std::max(it->second.hi, (long long)r.tk); it->second.lo = std::min(it->second.lo, (long long)r.tk); it->second.cl = r.tk; } ++trades; } continue; }
        if (r.act == 'R') continue;
        off::Ev e; e.px = r.tk; e.size = r.sz; e.id = (long long)r.oid; e.arrMs = ms; e.stamp = sec;
        e.side = r.side == 'B' ? off::BID : r.side == 'A' ? off::ASK : -1;
        e.action = r.act == 'A' ? off::NEW : r.act == 'M' ? off::MOD : r.act == 'C' ? off::DEL : off::FILL;
        if (e.px <= 0) continue;
        g.apply(acc, e);
        if (!g.mvt.trailOut.empty()) { for (auto& s : g.mvt.trailOut) lines.push_back(s); g.mvt.trailOut.clear(); }
    }
    std::vector<mvl::TBar> B; for (auto& kv : bars) B.push_back(kv.second);
    mvl::TrailSet T; for (auto& s : lines) T.line(s);
    mvl::TrailSet T2 = T;
    for (int i = 0; i < (int)B.size(); ++i) mvl::decideStops(T2, B, i, 0.5, 2);   // candle by candle
    mvl::decideStops(T, B, (int)B.size() - 2, 0.5, 2);                          // one pass
    bool same = T.v.size() == T2.v.size(); for (size_t k = 0; same && k < T.v.size(); ++k) same = T.v[k].end == T2.v[k].end && T.v[k].endKey == T2.v[k].endKey;
    if (outLines) { FILE* f = std::fopen(outLines, "w"); if (f) { for (auto& s : lines) std::fprintf(f, "%s\n", s.c_str()); for (auto& t : T.v) std::fprintf(f, "%s\n", mvl::trLineOf(t, 0.25).c_str()); std::fclose(f); } }
    // per session part: RTH = 8:30-15:00 CT = 13:30-20:00 UTC (CDT)
    auto rth = [](long long key) { const long long m = (key % 86400) / 60; return m > 13 * 60 + 30 && m <= 20 * 60; };
    std::map<char, int> ends[2]; int sides[2] = {0, 0}, part[2] = {0, 0}; std::vector<int> moves, ticks; std::vector<long long> dur, sizes;
    for (auto& t : T.v) { sides[t.side]++; part[rth(t.key0)]++; ends[rth(t.key0)][t.end ? t.end : 'o']++; moves.push_back(t.moves()); ticks.push_back((int)t.ticks()); dur.push_back(t.followMs()); sizes.push_back(t.size); }
    auto med = [](auto v) { if (v.empty()) return (double)0; std::sort(v.begin(), v.end()); return (double)v[v.size() / 2]; };
    std::printf("%s: %lld events, %lld trades, %zu candles; trail floor %lld lots (q90 of moves %lld)\n", mkt.c_str(), n, trades, B.size(), g.mvt.trailMin, g.mvt.minLots);
    std::printf("trails %zu (bids %d, offers %d); overnight %d, RTH %d; candidates %lld, ladder steps dropped %lld\n", T.v.size(), sides[0], sides[1], part[0], part[1], g.mvt.trCands, g.mvt.trLadder);
    for (int p = 0; p < 2; ++p) { std::printf("  %s ends:", p ? "RTH" : "ON "); for (auto& kv : ends[p]) std::printf(" %c=%d", kv.first, kv.second); std::printf("\n"); }
    std::printf("median: size %.0f lots, moves %.0f, ticks %.0f, followed %.0f s\n", med(sizes), med(moves), med(ticks), med(dur) / 1000.0);
    // descriptive look at B? (the nightly scores it properly): after a B? on a BID trail, how much higher did price go in the next 3
    // candles, and did a 1-ATR drop from the high come within 6 candles? (mirror for offers)
    int nb = 0, turn = 0, little = 0;
    for (auto& t : T.v) {
        if (t.end != 'B') continue; const int e = mvl::barIdx(B, t.endKey); if (e < 0 || e + 6 >= (int)B.size()) continue; ++nb;
        const double atr = mvl::atrAtIdx(B, e); long long ext = t.side == 0 ? B[e].hi : B[e].lo; for (int j = std::max(0, e - 3); j <= e; ++j) ext = t.side == 0 ? std::max(ext, B[j].hi) : std::min(ext, B[j].lo);
        long long ext3 = ext; for (int j = e + 1; j <= e + 3; ++j) ext3 = t.side == 0 ? std::max(ext3, B[j].hi) : std::min(ext3, B[j].lo);
        if ((double)std::llabs(ext3 - ext) <= 0.5 * atr) ++little;
        long long top = ext; bool tr = false; for (int j = e + 1; j <= e + 6; ++j) { top = t.side == 0 ? std::max(top, B[j].hi) : std::min(top, B[j].lo); const long long away = t.side == 0 ? top - B[j].lo : B[j].hi - top; if ((double)away >= atr) tr = true; }
        turn += tr;
    }
    std::printf("B? (decidable, 6 candles after): %d; extreme extended <= 0.5 ATR in the next 3 candles: %d; a 1-ATR move back within 6 candles: %d\n", nb, little, turn);
    std::printf("candle-by-candle decisions == one pass: %s\n", same ? "yes" : "NO");
    return same ? 0 : 1;
}
