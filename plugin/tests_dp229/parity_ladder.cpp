// parity_ladder.cpp -- NodeLadder 1.0.0 parity on REAL recorded MBO (Databento, the research archive): the OrderFlowFeed engine's classified
// events -> nl2::Ladder -> nl2::nodeRead at each same-candle A of the session vs the research python (scratchpad lmtf2/mbo_in.py, the
// defenders' added / pulled / filled lots at the node zone, z = round(0.15 ATR)). Events come on stdin as 7 x int64 (ts ns, act 0 new / 1 mod /
// 2 cancel / 3 fill / 9 clear, side 0 bid / 1 ask, price ticks, size, order id, the bar END second); argv[1] = the nodes file.
// g++ -std=c++17 -O2 -w -I/mnt/user-data/uploads/sdk/c++/include -I.. parity_ladder.cpp -Wl,--unresolved-symbols=ignore-all -o /tmp/plad
//   python3 ladder_parity_export.py ES 2026-09-21 parity/ES | /tmp/plad parity/ES.nodes
#include "../OrderFlowFeed.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
int main(int argc, char** argv) {
    if (argc < 2) return 2;
    struct Nd { int bull; long long lo, hi, key; double atr, rAdd, rPull, rFill, rH1, rH2, rAtt; };
    std::multimap<long long, Nd> want;
    long long rec[7]; bool first = std::fread(rec, sizeof(long long), 7, stdin) == 7;   // the exporter writes the nodes file before the first event
    {   std::ifstream in(argv[1]); std::string line;
        while (std::getline(in, line)) { std::istringstream ss(line); Nd d; if (ss >> d.bull >> d.lo >> d.hi >> d.atr >> d.key >> d.rAdd >> d.rPull >> d.rFill >> d.rH1 >> d.rH2 >> d.rAtt) want.insert(std::make_pair(d.key, d)); } }
    off::Engine E; nl2::Ladder L; L.spb = 180; E.ladder = &L; E.near = 1000000; E.refillMs = 1000;
    nl2::Tier T; T.ok = true; T.zAtr = 0.15; for (int h = 0; h < 24; ++h) { T.hourMed[h] = 1; T.hourOk[h] = true; }
    std::map<long long, off::Acc> accs; long long n = 0, lastKey = 0; int nodes = 0, exact = 0, close10 = 0; double sumAbs = 0, sumRes = 0;
    auto judge = [&](long long key) {
        std::pair<std::multimap<long long, Nd>::iterator, std::multimap<long long, Nd>::iterator> rg = want.equal_range(key);
        for (std::multimap<long long, Nd>::iterator it = rg.first; it != rg.second; ++it) {
            const Nd& d = it->second; std::map<long long, nl2::Bar>::const_iterator b = L.bars.find(key);
            nl2::Read r; if (b != L.bars.end()) r = nl2::nodeRead(b->second.rows, d.bull != 0, d.lo, d.hi, d.atr, 10, T);
            ++nodes; const double df = std::fabs(r.add - d.rAdd) + std::fabs(r.pull - d.rPull) + std::fabs(r.fill - d.rFill), tot = d.rAdd + d.rPull + d.rFill;
            if (df == 0) ++exact; if (df <= 0.10 * std::max(1.0, tot)) ++close10; sumAbs += df; sumRes += tot;
            std::printf("  node %s %lld-%lld key %lld: added %lld vs %.0f, pulled %lld vs %.0f, filled %lld vs %.0f, 1st half %lld vs %.0f, attackers pulled %lld vs %.0f\n",
                d.bull ? "bull" : "bear", d.lo, d.hi, key, r.add, d.rAdd, r.pull, d.rPull, r.fill, d.rFill, r.fill1, d.rH1, r.attPull, d.rAtt);
        }
    };
    while (first || std::fread(rec, sizeof(long long), 7, stdin) == 7) { first = false;
        const long long key = rec[6];
        if (key != lastKey) { if (lastKey) judge(lastKey); lastKey = key; }
        if (rec[1] == 9) continue;   // a clear: the research zeroes its depth; the engine keeps its orders (later events rebuild them)
        off::Ev e; e.action = (int)rec[1]; e.side = (int)rec[2]; e.px = rec[3]; e.size = rec[4]; e.id = rec[5]; e.aggr = 0;
        e.arrMs = rec[0] / 1000000; e.stamp = rec[0] / 1000000000;
        E.curKey = key; E.apply(accs[key], e); ++n;
        while (accs.size() > 8) accs.erase(accs.begin());
    }
    if (lastKey) judge(lastKey);
    std::printf("parity: %lld events, %d nodes, exact %d, within 10%% %d, total abs diff %.0f of %.0f lots (%.1f%%), rows dropped %lld\n",
        n, nodes, exact, close10, sumAbs, sumRes, sumRes > 0 ? 100.0 * sumAbs / sumRes : 0.0, L.droppedRows);
    return 0;
}
