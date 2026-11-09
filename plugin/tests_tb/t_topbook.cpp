// t_topbook.cpp - TopBook.h 1.0.0 tests (synthetic) + a replay of a recorded MBO day (binary fixture from export_bin.py)
#include "../TopBook.h"
#include <cstdio>
#include <cstdint>
#include <chrono>
#include <string>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
using namespace tbk;
static In ev(int act, int side, long long px, long long sz, long long oid, long long t) { In e; e.act = act; e.side = side; e.px = px; e.sz = sz; e.oid = oid; e.tMs = t; return e; }
// a book around 1000: 1-lot noise orders on both sides every tick out to 60 ticks, trades at 1000
static void base(Engine& E, long long t, long long& oid) {
    for (int d = 0; d <= 60; ++d) { E.feed(ev(ADD, 0, 1000 - d, 5, ++oid, t)); E.feed(ev(ADD, 1, 1001 + d, 5, ++oid, t)); }
    E.feed(ev(ADD, 0, 1000, 1, 999001, t)); E.feed(ev(FILL, 0, 1000, 1, 999001, t));   // a trade at 1000 sets the last price
}
static const Lvl* find(const std::vector<Lvl>& v, int s, long long px) { for (const Lvl& L : v) if (L.side == s && L.px == px) return &L; return nullptr; }
static void units() {
    {   Engine E; long long oid = 0; base(E, 0, oid);
        E.feed(ev(ADD, 0, 990, 100, 5001, 0));          // big bid 10 ticks below
        std::vector<Lvl> v; for (long long t = 0; t <= 20000; t += 1000) v = E.evaluate(t);
        CHECK(!find(v, 0, 990), "young (< 30 s) not shown");
        for (long long t = 21000; t <= 100000; t += 1000) v = E.evaluate(t);
        CHECK(!find(v, 0, 990), "(1.1.0) big for 80 s: not shown yet (2 min)");
        for (long long t = 101000; t <= 160000; t += 1000) v = E.evaluate(t);
        const Lvl* L = find(v, 0, 990); CHECK(L != nullptr, "(1.1.0) rested 30 s + big for 2 min -> shown");
        CHECK(L && L->lots == 105 && L->orders == 2, "lots = all real orders at the price");
        CHECK(L && L->single, "100 of 105 = one order -> single ?");
        CHECK(L && L->rank == 1, "the biggest = rank 1 (K)");
    }
    {   Engine E; long long oid = 0; base(E, 0, oid);
        E.feed(ev(ADD, 0, 999, 100, 5001, 0));          // 1 tick below the inside bid (1000) = inside 2 -> skipped
        E.feed(ev(ADD, 1, 1002, 100, 5002, 0));         // 1 tick above the inside offer (1001) -> skipped
        E.feed(ev(ADD, 1, 1003, 100, 5003, 0));         // 2 ticks above -> allowed
        std::vector<Lvl> v; for (long long t = 0; t <= 160000; t += 1000) v = E.evaluate(t);
        CHECK(!find(v, 0, 999) && !find(v, 1, 1002), "first 2 ticks skipped (market maker)");
        CHECK(find(v, 1, 1003) != nullptr, "3rd tick shown");
    }
    {   Engine E; long long oid = 0; base(E, 0, oid);   // quoting: an order re-priced twice in 5 min
        E.feed(ev(ADD, 0, 990, 100, 5001, 0)); E.feed(ev(MOD, 0, 989, 100, 5001, 1000)); E.feed(ev(MOD, 0, 990, 100, 5001, 2000));
        std::vector<Lvl> v; for (long long t = 0; t <= 180000; t += 1000) v = E.evaluate(t);
        CHECK(!find(v, 0, 990), "re-priced 2x in 5 min = quoting, not shown");
        CHECK(E.lastWhy().small >= 1, "its 100 lots left out - only the 5-lot noise counts there");
    }
    {   Engine E; long long oid = 0; base(E, 0, oid);   // under 2.0x the median real level (5 lots): 9 lots -> small
        E.feed(ev(ADD, 0, 990, 4, 5001, 0));
        std::vector<Lvl> v; for (long long t = 0; t <= 160000; t += 1000) v = E.evaluate(t);
        CHECK(!find(v, 0, 990), "9 lots vs median 5 (< 2.0x) not shown");
    }
    {   Engine E; long long oid = 0; base(E, 0, oid);   // 3 a side cap, ranks over all 6
        long long px[5] = { 990, 985, 980, 975, 970 }; long long sz[5] = { 50, 80, 30, 120, 60 };
        for (int i = 0; i < 5; ++i) E.feed(ev(ADD, 0, px[i], sz[i], 6000 + i, 0));
        for (int i = 0; i < 4; ++i) E.feed(ev(ADD, 1, 1010 + 5 * i, 40 + 10 * i, 7000 + i, 0));
        std::vector<Lvl> v; for (long long t = 0; t <= 160000; t += 1000) v = E.evaluate(t);
        int nb = 0, na = 0; for (const Lvl& L : v) (L.side == 0 ? nb : na)++;
        CHECK(nb == 3 && na == 3, "3 bids + 3 offers");
        CHECK(find(v, 0, 975) && find(v, 0, 985) && find(v, 0, 970) && !find(v, 0, 980), "top 3 bids by size");
        CHECK(v.size() == 6 && v[0].px == 975 && v[0].rank == 1, "K = the biggest of the 6");
    }
    {   Engine E; long long oid = 0; base(E, 0, oid);   // pulled on approach -> banned for the session
        E.feed(ev(ADD, 0, 996, 100, 5001, 0));
        std::vector<Lvl> v; for (long long t = 0; t <= 160000; t += 1000) v = E.evaluate(t);
        CHECK(find(v, 0, 996) != nullptr, "shown before the approach");
        // price trades down to 998 (inside bid 998 -> 2 ticks from 996), the bid is cancelled
        for (int d = 0; d <= 1; ++d) { /* remove bids at 1000, 999 */ }
        E.feed(ev(ADD, 0, 998, 1, 999101, 161000)); E.feed(ev(FILL, 0, 998, 1, 999101, 161000));
        for (long long o = 1; o <= oid; ++o) {}   // noise stays
        E.feed(ev(DEL, 0, 996, 100, 5001, 162000));
        v = E.evaluate(163000);
        E.feed(ev(ADD, 0, 996, 100, 5002, 164000));        // re-posted later at the same price
        for (long long t = 164000; t <= 300000; t += 1000) v = E.evaluate(t);
        CHECK(!find(v, 0, 996), "pulled on approach -> not shown again at that price");
        CHECK(E.nBanned == 1, "one banned price");
    }
    {   Engine E; long long oid = 0; base(E, 0, oid);   // S20 tag: ES RTH S20 = 45, one 50-lot order resting 2 min, never moved
        E.setMarket("ES"); E.setRth(true);
        E.feed(ev(ADD, 0, 990, 50, 5001, 0)); E.feed(ev(ADD, 1, 1010, 30, 5002, 0)); E.feed(ev(ADD, 1, 1010, 30, 5003, 0));
        E.feed(ev(ADD, 0, 985, 60, 5004, 0)); E.feed(ev(MOD, 0, 984, 60, 5004, 1000));   // moved once: never S20
        std::vector<Lvl> v; for (long long t = 0; t <= 160000; t += 1000) v = E.evaluate(t);
        CHECK(find(v, 0, 990) && find(v, 0, 990)->s20, "50-lot order >= S20 45, 2 min, never moved -> S20");
        CHECK(find(v, 1, 1010) && !find(v, 1, 1010)->s20, "60 lots from 2 x 30 = no single S20 order");
        CHECK(find(v, 0, 984) && !find(v, 0, 984)->s20, "moved once = not S20");
        Params g; s20For("GC", g); CHECK(g.s20Rth == 0 && g.bigRth == 9, "GC never S20-tagged, cluster 'big' 9 lots");
    }
    {   Engine E; long long oid = 0; base(E, 0, oid);   // (1.1.0) CLUSTER: two big bids within 2 ticks posted 20 s apart -> both levels out
        E.setMarket("ES"); E.setRth(true);
        E.feed(ev(ADD, 0, 990, 60, 5001, 0)); E.feed(ev(ADD, 0, 989, 50, 5002, 20000));
        E.feed(ev(ADD, 0, 980, 60, 5003, 0)); E.feed(ev(ADD, 0, 975, 50, 5004, 90000));   // 5 ticks away / 90 s later: not a cluster
        std::vector<Lvl> v; for (long long t = 0; t <= 260000; t += 1000) v = E.evaluate(t);
        CHECK(!find(v, 0, 990) && !find(v, 0, 989), "clustered big orders (2 within 2 ticks, 60 s) not shown");
        CHECK(find(v, 0, 980) && find(v, 0, 975), "big orders apart in price / time shown");
        CHECK(E.lastWhy().cluster == 2, "two levels counted as cluster");
    }
    {   Engine E; long long oid = 0; base(E, 0, oid);   // (1.1.0) % real from the model file (ES 11-09 coefficients) - LIVE only
        E.setMarket("ES"); E.setRth(true);
        const std::string f = "# c\nstatus,LIVE\nnear,6.0\nintercept,0.2244\ncoef,size_3SD,-0.0803\ncoef,size_S,2.6107\ncoef,age_10s_2m,-2.0420\ncoef,age_2_10m,-1.9084\ncoef,age_10m_plus,-2.4108\ncoef,clustered,0.7329\ncoef,posted_5_25t,0.1391\ncoef,posted_25t_plus,-0.3190\ncoef,rth,-0.4733\n";
        RealModel M = parseRealModel(f); CHECK(M.live && M.n == 10, "model parsed (LIVE, 10 numbers)");
        E.setModel(M); E.feed(ev(ADD, 0, 990, 30, 5001, 0)); E.feed(ev(ADD, 0, 980, 50, 5002, 0));
        std::vector<Lvl> v; for (long long t = 0; t <= 160000; t += 1000) v = E.evaluate(t);
        // 30 lots (2SD-ish), age 160 s (2-10 m), posted 10 ticks away (5-25), RTH: z = 0.2244 - 1.9084 + 0.1391 - 0.4733 = -2.0182 -> fake 11.7% -> 88% real
        CHECK(find(v, 0, 990) && find(v, 0, 990)->realPct == 88, "30-lot bid: 88% real (" + std::to_string(find(v, 0, 990) ? find(v, 0, 990)->realPct : -9) + ")");
        CHECK(find(v, 0, 980) && find(v, 0, 980)->realPct == 36, "50-lot (S20) bid: 36% real (ES S orders are faker)");
        RealModel L2 = parseRealModel("status,LEARNING\n" + f.substr(f.find("near"))); CHECK(!L2.live, "LEARNING = not shown");
        E.setModel(L2); v = E.evaluate(161000); CHECK(find(v, 0, 990) && find(v, 0, 990)->realPct == -1, "LEARNING: no % real");
        CHECK(!parseRealModel("status,LIVE\nintercept,x\n").live, "garbled file = not shown");
    }
    {   Engine E; long long oid = 0; base(E, 0, oid);   // HELD: touched, still there, price 0.5 ATR back away
        E.feed(ev(ADD, 0, 990, 100, 5001, 0));
        std::vector<Lvl> v; for (long long t = 0; t <= 160000; t += 1000) v = E.evaluate(t);
        E.feed(ev(ADD, 0, 990, 1, 999201, 161000)); E.feed(ev(FILL, 0, 990, 1, 999201, 161000)); v = E.evaluate(161000);
        E.feed(ev(ADD, 1, 1000, 1, 999202, 170000)); E.feed(ev(FILL, 1, 1000, 1, 999202, 170000)); v = E.evaluate(170000);   // 10 ticks away (ATR default 20 -> 0.5 = 10)
        std::vector<Outcome> o = E.takeOutcomes();
        CHECK(o.size() == 1 && o[0].held, "touched + still there + 0.5 ATR away = HELD");
    }
    {   Engine E; long long oid = 0; base(E, 0, oid);   // BROKE: 0.25 ATR through
        E.feed(ev(ADD, 0, 990, 100, 5001, 0));
        std::vector<Lvl> v; for (long long t = 0; t <= 160000; t += 1000) v = E.evaluate(t);
        E.feed(ev(FILL, 0, 990, 100, 5001, 161000)); v = E.evaluate(161000);
        E.feed(ev(ADD, 0, 984, 1, 999301, 162000)); E.feed(ev(FILL, 0, 984, 1, 999301, 162000)); v = E.evaluate(162000);
        std::vector<Outcome> o = E.takeOutcomes();
        CHECK(o.size() == 1 && !o[0].held, "0.25 ATR through = BROKE");
    }
    {   Engine E; long long oid = 0; base(E, 0, oid); E.feed(ev(CLEAR, 0, 0, 0, 0, 1000)); std::vector<Lvl> v = E.evaluate(2000);
        CHECK(v.empty() && E.orders() == 0, "clear = cold book");
    }
}
int main(int argc, char** argv) {
    units();
    std::printf("units: %d pass, %d fail\n", pass, fail);
    if (argc >= 4) {   // replay: <bin> <market> <snapshot CT HH:MM> [every-sec]
        FILE* f = std::fopen(argv[1], "rb"); if (!f) { std::printf("no fixture\n"); return 1; }
        Engine E; E.setMarket(argv[2]); if (argc >= 5) { FILE* mf = std::fopen(argv[4], "rb"); if (mf) { std::string t; char b[4096]; size_t n; while ((n = std::fread(b, 1, sizeof b, mf)) > 0) t.append(b, n); std::fclose(mf); E.setModel(parseRealModel(t)); } }
        int hh = 0, mm = 0; std::sscanf(argv[3], "%d:%d", &hh, &mm);
        const long long CT = -6LL * 3600 * 1000;
        long long next = -1, n = 0, worstUs = 0; bool printed = false;
        struct __attribute__((packed)) R { int8_t act, side; int32_t px, sz; uint64_t oid; int64_t t; } r;
        while (std::fread(&r, sizeof r, 1, f) == 1) {
            In e; e.act = r.act; e.side = r.side; e.px = r.px; e.sz = r.sz; e.oid = (long long)r.oid; e.tMs = r.t;
            if (next < 0) next = r.t;
            if (r.t >= next) {
                const long long s = ((r.t + CT) / 1000) % 86400; E.setRth(s >= 8 * 3600 + 30 * 60 && s < 15 * 3600 + 15 * 60);
                const auto a = std::chrono::steady_clock::now(); std::vector<Lvl> v = E.evaluate(r.t);
                const long long us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - a).count(); if (us > worstUs) worstUs = us;
                if (!printed && s >= hh * 3600 + mm * 60 && s < 16 * 3600) { printed = true;
                    std::printf("SNAP %02lld:%02lld:%02lld\n%s\n", s / 3600, s / 60 % 60, s % 60, E.statusLine().c_str());
                    for (const Lvl& L : v) std::printf("  #%d %s %.2f lots %lld orders %d big %lld (%.0f%%) %.1fx%s%s%s real %d\n", L.rank, L.side ? "offer" : "bid", L.px * 0.25, L.lots, L.orders, L.big, 100.0 * L.big / std::max(1LL, L.lots), L.x, L.single ? " ?" : "", L.s20 ? " S20" : "", "", L.realPct);
                    if (v.size()) {} 
                }
                next = r.t + 1000; }
            E.feed(e); ++n; }
        std::fclose(f);
        std::printf("events %lld, orders now %zu, worst evaluate %lld us\n%s\n", n, E.orders(), worstUs, E.statusLine().c_str());
    }
    return fail ? 1 : 0;
}
