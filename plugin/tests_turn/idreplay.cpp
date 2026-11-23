// idreplay.cpp -- IRT's own recorded order events (lsIRTReader: time|action|price|prev_price|size|order_id|aggressor_id|side|bid|ask) through
// turn::Live (the TurnLogic.h on the include path): per 10 minutes the fills, the fills counted as stops, the clock's size.
// usage: zcat file.mbo.csv.gz | idreplay <tick> [startHH:MM]   (startHH:MM = the pane is loaded then: earlier events are dropped)
#include "TurnLogic.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
int main(int argc, char** argv) {
    const double tick = argc > 1 ? std::atof(argv[1]) : 0.01; const std::string from = argc > 2 ? argv[2] : "";
    turn::Live L; L.reset(180);
    std::string ln; std::getline(std::cin, ln);
    long long lastBucket = -1, f0 = 0, s0 = 0, n = 0; long long day0 = -1;
    while (std::getline(std::cin, ln)) {
        const std::vector<std::string> c = turn::split(ln, '|'); if (c.size() < 8 || c[0].size() < 19) continue;
        int Y, M, D, h, m, s; if (std::sscanf(c[0].c_str(), "%d-%d-%d %d:%d:%d", &Y, &M, &D, &h, &m, &s) != 6) continue;
        if (!from.empty() && c[0].substr(11, 5) < from && day0 < 0) continue;
        if (day0 < 0) day0 = D;
        const long long sec = 1795000000LL + (long long)(D - day0) * 86400 + h * 3600 + m * 60 + s;
        const int act = c[1] == "NEW" ? 0 : c[1] == "MOD" ? 1 : c[1] == "DEL" ? 2 : c[1] == "FILL" ? 3 : -1; if (act < 0) continue;
        const int bs = c[7] == "B" ? 1 : c[7] == "S" ? 0 : 2;
        const long long px = (long long)std::llround(std::atof(c[2].c_str()) / tick);
        L.onEvent(sec, act, bs, px, std::atoll(c[4].c_str()), std::atoll(c[5].c_str()), std::atoll(c[6].c_str())); ++n;
        const long long b = sec / 600;
        if (b != lastBucket) { if (lastBucket >= 0) std::printf("%02lld:%02lld  fills %6lld  stops %6lld (%3lld%%)  clock %4zu  orders %5zu\n", (lastBucket * 600 % 86400) / 3600, (lastBucket * 600 % 3600) / 60, L.n.fills - f0, L.n.stopFills - s0, L.n.fills - f0 ? (L.n.stopFills - s0) * 100 / (L.n.fills - f0) : 0, L.clockPoints(), L.orders());
            lastBucket = b; f0 = L.n.fills; s0 = L.n.stopFills; }
    }
    std::printf("events %lld fills %lld stop fills %lld (%lld%%) implied %lld clock %zu\n", n, L.n.fills, L.n.stopFills, L.n.fills ? L.n.stopFills * 100 / L.n.fills : 0, L.n.implied, L.clockPoints());
    return 0;
}
