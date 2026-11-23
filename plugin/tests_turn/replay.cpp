// tests_turn/replay.cpp -- replay an IRT-style event file (sec action buySell tick size oid aid) through turn::Live and print, for chosen
// candles, every raw reading with full precision and (with a params file) the decided read. Used by golden.py (the 119 golden candles).
// usage: replay <events> [--params <file> <MKT>] { <key> <dir> <level ticks> <tod> }...
#include "../TurnLogic.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
int main(int argc, char** argv) {
    if (argc < 2) return 2;
    turn::Live L; L.reset(180);
    std::ifstream f(argv[1]); long long sec, act, bs, px, sz, oid, aid; long long n = 0;
    size_t peakFills = 0, peakOrd = 0;
    while (f >> sec >> act >> bs >> px >> sz >> oid >> aid) { L.onEvent(sec, (int)act, (int)bs, px, sz, oid, aid); ++n; peakFills = std::max(peakFills, L.openFills()); peakOrd = std::max(peakOrd, L.orders()); }
    L.closeThrough(L.forming());
    std::printf("events %lld fills %lld implied %lld candles %lld complete %lld late %lld bad %lld noAggr %lld stopFills %lld peakFills %zu peakOrders %zu clock %zu resets %lld\n",
        n, L.n.fills, L.n.implied, L.n.candles, L.n.complete, L.n.late, L.n.bad, L.n.noAggr, L.n.stopFills, peakFills, peakOrd, L.clockPoints(), L.n.shownResets);
    int i = 2; turn::Params P; bool haveP = false;
    if (i + 2 < argc && std::string(argv[i]) == "--params") {
        std::ifstream pf(argv[i + 1], std::ios::binary); std::stringstream ss; ss << pf.rdbuf(); P = turn::parseParams(ss.str(), argv[i + 2]); haveP = true; i += 3;
        std::printf("params %s %s ranks %zu %zu %zu hist %zu odds %d %d %d %d proven %d\n", argv[i - 1], P.why.c_str(), P.rkLate.size(), P.rkDecay.size(), P.rkAfter.size(), P.hist.size(), P.odds[0], P.odds[1], P.odds[2], P.odds[3], (int)P.bestProven);
    }
    for (; i + 3 < argc; i += 4) {
        const long long key = std::atoll(argv[i]); const int dir = std::atoi(argv[i + 1]); const long long lvt = std::atoll(argv[i + 2]); const int tod = std::atoi(argv[i + 3]);
        const turn::Sum* s = L.find(key); if (!s) { std::printf("key %lld not found\n", key); continue; }
        const turn::Turn& t = dir < 0 ? s->hi : s->lo; const turn::Exh& e = dir < 0 ? s->exHi : s->exLo; const turn::Bars b = turn::sumBars(*s);
        std::printf("key %lld dir %d lvt %lld complete %d stopOk %d thin %d stop %lld effOk %d eff %.17g ext %lld into %lld left %lld exh %d late %.17g decayOk %d decay %.17g after %.17g fresh %d hard %lld big %lld hi %lld lo %lld",
            key, dir, lvt, (int)s->complete, (int)s->stopOk, (int)s->thin, turn::stopLots(*s, dir, lvt), (int)(dir < 0 ? b.topOk : b.botOk), dir < 0 ? b.top : b.bot,
            t.extPx, t.extSec - s->start, s->key - t.extSec, (int)e.ok, e.late, (int)e.hasDecay, e.decay, e.after, (int)t.fresh, t.hard, t.big, s->hiPx, s->loPx);
        if (haveP) {
            turn::Sw w; w.stamp = key; w.dir = dir; w.lvt = lvt; w.lv = "LVL";
            const turn::Read r = turn::decideLive(*s, w, P, tod, false, 0); const turn::Dem d = turn::demOf(e, P);
            std::printf(" | dem %.17g demOk %d | T|%s", d.dem, (int)d.ok, turn::readText(r, '|').c_str());
        }
        std::printf("\n");
    }
    return 0;
}
