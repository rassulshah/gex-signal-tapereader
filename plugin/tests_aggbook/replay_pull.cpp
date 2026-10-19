// replay_pull.cpp - BookFlow 1.4.0 pull-marker calibration on a recorded MBO day (stdin: the .mbo.csv lines).
// build: g++ -O2 -std=c++17 -I<plugin> replay_pull.cpp -o replay_pull ; run: zcat <day>.mbo.csv.gz | ./replay_pull
// Uses BookFlow's own engine (OrderFlowFeed.h off::Engine: near band, adds, pulls = DEL not after a FILL / MOD away / size cut).
// Per 3-min candle (close-stamped buckets) per side: added, pulled; the rule: pulled >= X x the plain average of the prior 20 candles'
// pulled (>= 5 needed) AND pulled >= Y x added. Prints the RTH (8:30-15:00 CT) marker counts for a grid of X / Y.
#include "BidAskStatsLogic.h"
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>
struct Bar { long long add[2] = {0, 0}, pull[2] = {0, 0}; double hi = -1, lo = 1e18; long long events = 0; };
int main() {
    const double tick = 0.25; off::Engine e; e.refillMs = 1000; nl::Zone z = nl::zoneFor("ES");
    std::map<long long, Bar> bars; std::map<long long, off::Acc> acc;   // key = bucket end (seconds of day, 180 s)
    std::string ln; std::getline(std::cin, ln);
    long long n = 0; long long curKey = -1; std::vector<double> ranges;
    while (std::getline(std::cin, ln)) {
        if (ln.size() < 30) continue;
        // time|action|price|prev_price|size|order_id|aggressor_id|side|bid|ask
        size_t p[10]; int k = 0; p[0] = 0; for (size_t i = 0; i < ln.size() && k < 9; ++i) if (ln[i] == '|') p[++k] = i + 1;
        if (k < 9) continue;
        auto f = [&](int j) { size_t a = p[j], b = j < 9 ? p[j + 1] - 1 : ln.size(); return ln.substr(a, b - a); };
        const std::string t = f(0); const int hh = std::stoi(t.substr(11, 2)), mm = std::stoi(t.substr(14, 2)), ss = std::stoi(t.substr(17, 2));
        const long long sec = hh * 3600 + mm * 60 + ss; const long long key = (sec / 180 + 1) * 180;
        const std::string a = f(1); off::Ev ev;
        ev.action = a == "NEW" ? off::NEW : a == "MOD" ? off::MOD : a == "DEL" ? off::DEL : a == "FILL" ? off::FILL : -1;
        const double px = std::stod(f(2)); ev.px = (off::Tick)std::llround(px / tick); ev.size = std::stoll(f(4)); ev.id = std::stoll(f(5)); ev.aggr = std::stoll(f(6));
        const std::string sd = f(7); ev.side = sd == "B" ? off::BID : sd == "S" ? off::ASK : -1;
        ev.arrMs = sec * 1000 + (n % 1000); ev.stamp = sec;
        if (key != curKey) {                                         // a new candle: the near band from the last 20 candles' ranges (BookFlow: ATR of closed bars)
            if (curKey >= 0) { Bar& B = bars[curKey]; if (B.hi >= 0) ranges.insert(ranges.begin(), B.hi - B.lo); if (ranges.size() > 20) ranges.pop_back(); }
            e.near = nl::nearBand(nl::atrOf(ranges, 180), tick, z, 0); curKey = key;
        }
        const double bid = std::stod(f(8)), ask = std::stod(f(9));
        if (bid > 0 && ask > 0) e.fallbackRef((off::Tick)std::llround((bid + ask) / 2 / tick), ev.arrMs);
        e.apply(acc[key], ev);
        Bar& B = bars[key]; B.events++;
        if (ev.action == off::FILL) { if (B.hi < 0 || px > B.hi) B.hi = px; if (px < B.lo) B.lo = px; }
        ++n;
    }
    for (auto& kv : acc) { Bar& B = bars[kv.first]; for (int s = 0; s < 2; ++s) { B.add[s] = kv.second.addC[s]; B.pull[s] = kv.second.pullC[s]; } }
    std::vector<long long> keys; for (auto& kv : bars) if (kv.second.events > 50) keys.push_back(kv.first);
    std::fprintf(stderr, "events %lld, candles %zu\n", n, keys.size());
    const double X[] = { 1.25, 1.5, 1.75, 2.0, 2.5, 3.0 }, Y[] = { 0.0, 0.80, 0.83, 0.85, 0.87, 1.2 };
    std::printf("RTH 8:30-15:00 markers (bids / offers / total) by pulled-vs-average X (rows) and pulled-vs-added Y (cols)\n      ");
    for (double y : Y) std::printf("   Y=%.2f    ", y); std::printf("\n");
    for (double x : X) {
        std::printf("X=%.2f", x);
        for (double y : Y) {
            int c[2] = {0, 0};
            for (size_t i = 0; i < keys.size(); ++i) {
                const long long t = keys[i]; if (t <= 8 * 3600 + 30 * 60 || t > 15 * 3600) continue;
                for (int s = 0; s < 2; ++s) {
                    std::vector<long long> pr; for (size_t j = i >= 20 ? i - 20 : 0; j < i; ++j) pr.push_back(bars[keys[j]].pull[s]);
                    if (pr.size() < 5) continue; double m = 0; for (long long v : pr) m += (double)v; m /= (double)pr.size(); if (m < 1) m = 1;
                    const Bar& B = bars[t];
                    if ((double)B.pull[s] >= x * m && (double)B.pull[s] >= y * (double)B.add[s] && B.pull[s] > 0) ++c[s];
                }
            }
            std::printf("   %2d/%2d=%2d  ", c[0], c[1], c[0] + c[1]);
        }
        std::printf("\n");
    }
    // the 2.0 / 1.2 hits with their times
    for (size_t i = 0; i < keys.size(); ++i) {
        const long long t = keys[i]; if (t <= 8 * 3600 + 30 * 60 || t > 15 * 3600) continue;
        for (int s = 0; s < 2; ++s) {
            std::vector<long long> pr; for (size_t j = i >= 20 ? i - 20 : 0; j < i; ++j) pr.push_back(bars[keys[j]].pull[s]);
            if (pr.size() < 5) continue; double m = 0; for (long long v : pr) m += (double)v; m /= (double)pr.size(); if (m < 1) m = 1;
            const Bar& B = bars[t];
            if ((double)B.pull[s] >= 2.0 * m && B.pull[s] * 10 >= 12 * B.add[s] && B.pull[s] > 0)
                std::printf("hit %02lld:%02lld %s pulled %lld (%.1fx avg %.0f) added %lld\n", t / 3600, t / 60 % 60, s ? "offers" : "bids", B.pull[s], B.pull[s] / m, m, B.add[s]);
        }
    }
    // the RTH candles' pull / add ratio distribution
    std::vector<double> pa; for (long long t : keys) if (t > 8 * 3600 + 30 * 60 && t <= 15 * 3600) for (int s = 0; s < 2; ++s) { const Bar& B = bars[t]; if (B.add[s] > 0) pa.push_back((double)B.pull[s] / B.add[s]); }
    std::sort(pa.begin(), pa.end()); if (!pa.empty()) std::printf("pulled / added per side-candle (RTH, n=%zu): p50 %.2f  p90 %.2f  p99 %.2f  max %.2f\n", pa.size(), pa[pa.size() / 2], pa[pa.size() * 9 / 10], pa[pa.size() * 99 / 100], pa.back());
    return 0;
}
