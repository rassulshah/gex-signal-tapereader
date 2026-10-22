// replay_tor.cpp -- lsTurnOrder 1.0.0 on a recorded MBO session (tests_lmz/conv_mbo.py format: "E,t_ct_ms,act,side,tick,size,oid" / "T,t,tick,size").
// 3-min candles are built from the trades (keyed by their END, as IRT), closed when the clock passes their end; every event goes to the
// engine in order. Min size = the market's Big Lot S20 (LmtZonesLogic ruleFor: RTH / overnight). Prints every mark + N line, and
// checks NO REPAINT: a second engine restarted from the first one's record after every candle yields the same marks.
// usage: replay_tor <events.csv | - (stdin)> <MKT> [tick=0.25] [gate=0.5]
#include "../TurnOrderLogic.h"
#include "../LmtZonesLogic.h"
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "usage\n"); return 2; }
    const std::string mkt = argv[2]; const double tick = argc > 3 ? std::atof(argv[3]) : 0.25, gate = argc > 4 ? std::atof(argv[4]) : 0.5;
    const lmz::Rule R = lmz::ruleFor(mkt);
    tor::Engine E; E.tick = tick; E.P.gateAtr = gate;
    std::ifstream fin; if (std::string(argv[1]) != "-") fin.open(argv[1]); std::istream& in = std::string(argv[1]) == "-" ? std::cin : fin; std::string ln; const long long SPB = 180000;
    std::unordered_map<long long, long long> rem; tor::Bar cur; bool have = false; long long nEv = 0; std::vector<std::string> rec;
    const long long fixedMin = std::getenv("TOR_MIN") ? std::atoll(std::getenv("TOR_MIN")) : 0;
    auto s20At = [&](long long endMs) { if (fixedMin > 0) return fixedMin; const int mod = (int)(((endMs / 60000LL) % 1440LL + 1440LL) % 1440LL); return mod > 510 && mod <= 900 ? R.s20Rth : R.s20On; };
    auto flush = [&]() { for (auto& r : E.rows) rec.push_back(r); E.rows.clear(); };
    auto closeUpTo = [&](long long t) { if (have && t >= cur.endMs) { E.closeBar(cur); flush(); have = false; E.P.minSize = s20At(cur.endMs + SPB); } };
    E.P.minSize = fixedMin > 0 ? fixedMin : R.s20On;
    while (std::getline(in, ln)) {
        if (ln.size() < 3) continue;
        const std::vector<std::string> f = tor::split(ln, ',');
        if (f[0] == "T" && f.size() == 4) {
            const long long t = std::atoll(f[1].c_str()), px = std::atoll(f[2].c_str());
            closeUpTo(t);
            if (!have) { cur = tor::Bar(); cur.endMs = (t / SPB + 1) * SPB; cur.startMs = cur.endMs - SPB; cur.o = cur.h = cur.l = cur.c = px; have = true; }
            cur.h = std::max(cur.h, px); cur.l = std::min(cur.l, px); cur.c = px;
        } else if (f.size() == 6 && f[1].size() == 1 && std::isalpha((unsigned char)f[1][0])) {   // a raw Databento row: t,act,side,tick,size,oid
            const long long t = std::atoll(f[0].c_str()), pt = std::atoll(f[3].c_str()), sz = std::atoll(f[4].c_str()); const long long oid = (long long)std::strtoull(f[5].c_str(), nullptr, 10);
            const char a = f[1][0]; const int sd = f[2] == "B" ? 0 : f[2] == "A" ? 1 : -1;
            closeUpTo(t);
            if (a == 'T') {
                if (!have) { cur = tor::Bar(); cur.endMs = (t / SPB + 1) * SPB; cur.startMs = cur.endMs - SPB; cur.o = cur.h = cur.l = cur.c = pt; have = true; }
                cur.h = std::max(cur.h, pt); cur.l = std::min(cur.l, pt); cur.c = pt; continue;
            }
            if (a == 'R') { rem.clear(); continue; }
            ++nEv;
            if (a == 'A') { rem[oid] = sz; E.onEvent(tor::NEW, sd, pt, sz, oid, t); }
            else if (a == 'M') { if (rem.count(oid)) rem[oid] = sz; E.onEvent(tor::MOD, sd, pt, sz, oid, t); }
            else if (a == 'C') { auto it = rem.find(oid); if (it == rem.end()) continue;
                if (sz >= it->second) { E.onEvent(tor::DEL, sd, pt, it->second, oid, t); rem.erase(it); } else { it->second -= sz; E.onEvent(tor::MOD, sd, pt, it->second, oid, t); } }
            else if (a == 'F') { auto it = rem.find(oid); if (it == rem.end()) continue;
                E.onEvent(tor::FILL, sd >= 0 ? 1 - sd : -1, pt, sz, oid, t);
                if (sz >= it->second) { E.onEvent(tor::DEL, sd, pt, 0, oid, t); rem.erase(it); } else it->second -= sz; }
        } else if (f[0] == "E" && f.size() == 7) {
            const long long t = std::atoll(f[1].c_str()); closeUpTo(t);
            E.onEvent(std::atoi(f[2].c_str()), std::atoi(f[3].c_str()), std::atoll(f[4].c_str()), std::atoll(f[5].c_str()), std::atoll(f[6].c_str()), t); ++nEv;
        }
    }
    if (have) { E.closeBar(cur); flush(); }
    // no repaint: the record read back gives the same marks
    tor::Engine B; B.tick = tick; long long bad = 0; const size_t got = B.restore(rec, &bad);
    std::printf("%s events %lld candles %zu pivots %lld marks %lld (FILLED %lld THICK %lld) no-order %lld not-confirmed %lld broken %lld no-feed %lld unknownFill %lld pruned %lld\n",
        mkt.c_str(), nEv, E.bars.size(), E.tally.pivots, E.tally.marks, E.tally.absorbed, E.tally.thick, E.tally.noOrder, E.tally.noConfirm, E.tally.broken, E.tally.noFeed, E.tally.unknownFill, E.tally.pruned);
    std::printf("RESTORE %zu marks, %lld bad lines, identical %s\n", got, bad, got == E.marks.size() ? "yes" : "NO");
    for (auto& r : rec) std::printf("%s\n", r.c_str());
    return got == E.marks.size() && bad == 0 ? 0 : 1;
}
