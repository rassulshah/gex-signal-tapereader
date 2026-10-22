// IMPLIED FILLS option B (2026-10-22 evening): OrderFlowFeed 1.14.0 + ImpliedFill.h 1.0.0 + SweepRefillLogic 1.1.1.
// A synthetic order feed with side "?" (implied) FILLs: delta / hits / aggressor orders / big lots / sweeps unchanged by them, volume
// includes them, the session record line byte-identical with and without them (no repaint on a restart), the params file rules.
#include "../OrderFlowFeed.h"
#include "../SweepRefillLogic.h"
#include <cassert>
#include <cstdio>
#include <iostream>
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

struct O { unsigned action, buySell; double price; long long size, orderID, aggressorOrderID, timestamp; };
static off::Ev ev(unsigned a, unsigned bs, double px, long long sz, long long id, long long ag, long long ms) {
    O o{a, bs, px, sz, id, ag, 1000 + ms / 1000}; return off::evOf(o, 0.25, ms);
}
// the base feed: bids / offers near 5000, buyers lift the offers (aggr 900, 901), sellers hit a bid (aggr 902)
static std::vector<off::Ev> base() {
    std::vector<off::Ev> v; long long t = 0;
    v.push_back(ev(0, 1, 4999.75, 50, 1, 0, t += 10)); v.push_back(ev(0, 1, 4999.50, 40, 2, 0, t += 10));
    v.push_back(ev(0, 0, 5000.00, 60, 3, 0, t += 10)); v.push_back(ev(0, 0, 5000.25, 30, 4, 0, t += 10)); v.push_back(ev(0, 0, 5000.50, 30, 5, 0, t += 10));
    v.push_back(ev(3, 0, 5000.00, 25, 3, 900, t += 10)); v.push_back(ev(3, 0, 5000.25, 10, 4, 900, t += 1)); v.push_back(ev(3, 0, 5000.50, 5, 5, 900, t += 1));
    v.push_back(ev(3, 1, 4999.75, 12, 1, 902, t += 10)); v.push_back(ev(3, 0, 5000.00, 3, 3, 901, t += 10));
    return v;
}
// the same feed + implied fills: on known resting orders (ids 1, 3), on an unknown order (id 77) and with a named aggressor id
static std::vector<off::Ev> withImplied() {
    std::vector<off::Ev> v = base(), out; long long t = 1000;
    for (size_t i = 0; i < v.size(); ++i) {
        out.push_back(v[i]);
        if (i == 6) { out.push_back(ev(3, 2, 5000.00, 7, 3, 950, t += 1)); out.push_back(ev(3, 9, 4999.75, 4, 1, 0, t += 1)); }
        if (i == 8) out.push_back(ev(3, 2, 4999.50, 9, 77, 951, t += 1));
    }
    return out;
}
static off::Rec run(const std::vector<off::Ev>& v, off::Engine& e, off::Acc& a, const off::Params& P) {
    e.near = 40; for (size_t i = 0; i < v.size(); ++i) e.apply(a, v[i]); return e.numbers(a, false, P);
}
int main() {
    off::Params P;
    {   // evOf flags implied once: FILL + side not 0 / 1 only
        CHECK(ev(3, 2, 5000, 1, 1, 0, 0).implied); CHECK(ev(3, 255, 5000, 1, 1, 0, 0).implied);
        CHECK(!ev(3, 1, 5000, 1, 1, 0, 0).implied); CHECK(!ev(3, 0, 5000, 1, 1, 0, 0).implied); CHECK(!ev(0, 2, 5000, 1, 1, 0, 0).implied);
    }
    off::Engine e0, e1; off::Acc a0, a1;
    const off::Rec r0 = run(base(), e0, a0, P), r1 = run(withImplied(), e1, a1, P);
    for (int s = 0; s < 2; ++s) {                                 // delta / hits / aggressor / big lots / sweeps unchanged
        CHECK(r0.hitC[s] == r1.hitC[s]); CHECK(r0.aggrC[s] == r1.aggrC[s]); CHECK(r0.aggrN[s] == r1.aggrN[s]); CHECK(r0.aggrMax[s] == r1.aggrMax[s]);
        CHECK(r0.sweepLv[s] == r1.sweepLv[s]); CHECK(r0.sweepSize[s] == r1.sweepSize[s]); CHECK(r0.big[s].lots == r1.big[s].lots); CHECK(r0.sweep[s].lots == r1.sweep[s].lots);
        CHECK(r0.refN[s] == r1.refN[s]); CHECK(r0.pullC[s] == r1.pullC[s]);
    }
    CHECK(r0.hitC[off::ASK] == 43 && r0.hitC[off::BID] == 12);    // the base feed's own numbers (delta = 43 - 12 = +31)
    CHECK(r1.hitC[off::ASK] - r1.hitC[off::BID] == 31);
    CHECK(r0.fillAll == 55 && r0.fillImpl == 0);                  // volume: every fill
    CHECK(r1.fillAll == 55 + 7 + 4 + 9 && r1.fillImpl == 20);     // implied kept in volume, counted apart
    CHECK(e1.impliedFills == 3 && e1.impliedLots == 20 && e1.garbage == e0.garbage);
    CHECK(e1.ref > 0);                                            // the last trade still moves with an implied print
    {   // the resting order lost the implied lots (book right): id 3 had 60, 25 + 3 aggressive + 7 implied -> 25 left
        CHECK(e1.book.count(3) && e1.book[3].size == 25); CHECK(e0.book[3].size == 32);
    }
    {   // the record line: byte-identical with and without implied fills (a restart reloads the same numbers - no repaint)
        std::vector<long long> n0, n1; off::recNums(r0, n0); off::recNums(r1, n1); CHECK(n0.size() == n1.size());
        for (size_t i = 0; i < n0.size() && i < n1.size(); ++i) if (n0[i] != n1[i]) { if (i == 1) { CHECK(n1[1] == n0[1] + 3); continue; } std::printf("rec field %zu: %lld vs %lld\n", i, n0[i], n1[i]); CHECK(false); }   // field 1 = order events read (the 3 implied events)
        off::Rec back; CHECK(off::recFrom(n1, 0, back)); std::vector<long long> n2; off::recNums(back, n2); CHECK(n2 == n1);
        CHECK(back.fillAll == 0);                                 // a reloaded candle has no implied numbers -> no hover row
        off::InfoRow ir; imp::Params F; F.fromFile = true; CHECK(!off::impliedRow(F, back.fillAll, back.fillImpl, ir));
    }
    {   // include mode (PARAM,exclude,0): implied fills on known orders count again (the old behaviour)
        off::Engine e2; off::Acc a2; e2.ip.exclude = false; const off::Rec r2 = run(withImplied(), e2, a2, P);
        CHECK(r2.hitC[off::ASK] + r2.hitC[off::BID] == 55 + 7 + 4); CHECK(e2.impliedFills == 0); CHECK(e2.garbage == e0.garbage + 1);
    }
    {   // params file
        const std::string M = "CL";
        imp::Params p = imp::parse(false, false, {}, M); CHECK(p.exclude && !p.fromFile);
        p = imp::parse(true, true, {"VERSION,1", "MARKET,CL", "PARAM,exclude,1", "PARAM,live_code,?"}, M); CHECK(p.exclude && p.fromFile && p.code == '?');
        p = imp::parse(true, true, {"PARAM,exclude,0", "PARAM,live_code,'?'"}, M); CHECK(!p.exclude && p.fromFile);
        p = imp::parse(true, true, {"MARKET,NG", "PARAM,exclude,0"}, M); CHECK(p.exclude && !p.fromFile);   // wrong market = safe default
        p = imp::parse(true, true, {"MARKET,6E", "PARAM,exclude,1"}, "EU"); CHECK(p.fromFile);              // 6E file on an EU chart
        p = imp::parse(true, true, {"PARAM,exclude,yes"}, M); CHECK(p.exclude && !p.fromFile);              // garbled
        p = imp::parse(true, true, {"PARAM,live_code,??"}, M); CHECK(p.exclude && !p.fromFile);
        p = imp::parse(true, false, {}, M); CHECK(p.exclude && !p.fromFile);
        p = imp::parse(true, true, {"PARAM,live_code,?"}, M); CHECK(p.exclude && !p.fromFile);              // no exclude row
    }
    {   // the hover row
        imp::Params F; off::InfoRow ir;
        CHECK(!off::impliedRow(F, 100, 23, ir));                  // no file: no row
        F.fromFile = true; CHECK(off::impliedRow(F, 100, 23, ir)); CHECK(ir.label == "Implied" && ir.v[0] == "23% of" && ir.v[1] == "volume");
        CHECK(imp::hoverText("23%") == "Implied: 23% of this candle's volume");
        CHECK(!off::impliedRow(F, 0, 0, ir)); CHECK(off::impliedRow(F, 3, 9, ir) && ir.v[0] == "100% of");
        std::string pc; CHECK(imp::hoverPct(F, 1, 3, pc) && pc == "33%"); CHECK(imp::hoverPct(F, 2, 3, pc) && pc == "67%");
    }
    {   // SweepRefill: an implied fill is never a sweep trade, the resting order still loses the lots
        swr::Detector d0, d1; long long ms = 0;
        auto feed = [&](swr::Detector& d, bool withImp) {
            auto E = [&](int a, int sd, long long px, long long sz, long long id, bool im) { swr::Ev x; x.action = a; x.side = sd; x.px = px; x.size = sz; x.id = id; x.ms = (ms += 10); x.sec = 1000 + ms / 1000; x.implied = im; d.onEvent(x); };
            E(swr::NEW, swr::ASK, 20000, 50, 3, false); E(swr::NEW, swr::ASK, 20001, 50, 4, false);
            if (withImp) { E(swr::FILL, -1, 20000, 8, 3, true); E(swr::FILL, -1, 20001, 6, 99, true); }
            E(swr::FILL, swr::ASK, 20000, 10, 3, false);
        };
        feed(d0, false); ms = 0; feed(d1, true);
        CHECK(d0.fills == d1.fills); CHECK(d1.impliedFills == 2 && d1.impliedLots == 14); CHECK(d0.impliedFills == 0);
    }
    std::printf("%s: %d checks, %d failures\n", fails ? "FAILED" : "PASS", checks, fails);
    return fails ? 1 : 0;
}
