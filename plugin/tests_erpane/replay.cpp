// replay an IRT-style event file through erp::Live; print the strict lots and the raw turn values of chosen candles
#include "../ErPaneLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>
int main(int argc, char** argv) {
    if (argc < 3) return 2;
    erp::Live L; L.reset(180);
    std::ifstream f(argv[1]); long long sec, act, bs, px, sz, oid, aid; long long n = 0;
    size_t peakFills = 0, peakOrd = 0;
    while (f >> sec >> act >> bs >> px >> sz >> oid >> aid) { L.onEvent(sec, (int)act, (int)bs, px, sz, oid, aid); ++n; peakFills = std::max(peakFills, L.openFills()); peakOrd = std::max(peakOrd, L.orders()); }
    L.closeThrough(L.forming());
    std::printf("events %lld fills %lld implied %lld candles %lld complete %lld late %lld bad %lld noAggr %lld peakFills %zu peakOrders %zu clock %zu resets %lld\n",
        n, L.n.fills, L.n.implied, L.n.candles, L.n.complete, L.n.late, L.n.bad, L.n.noAggr, peakFills, peakOrd, L.clockPoints(), L.n.shownResets);
    for (int i = 2; i + 2 < argc + 0 && i + 2 <= argc - 1; i += 3) {
        const long long key = std::atoll(argv[i]); const int dir = std::atoi(argv[i + 1]); const long long lvt = std::atoll(argv[i + 2]);
        const erp::Sum* s = L.find(key); if (!s) { std::printf("key %lld not found\n", key); continue; }
        const erp::Strict x = erp::strictOf(*s, dir, lvt); const erp::Turn& t = dir < 0 ? s->hi : s->lo;
        std::printf("key %lld dir %d lvt %lld complete %d strict %lld tot %lld hid %lld unk %lld | ext %lld extSec %lld late %d fresh %d hard %lld big %lld\n",
            key, dir, lvt, (int)s->complete, x.strict, x.tot, x.hid, x.unk, t.extPx, t.extSec % 86400, (int)t.late, (int)t.fresh, t.hard, t.big);
    }
}
