// CobCheck.h 1.0.0 on the recorded ES 3-min chart bars (argv[1] = "t,o,h,l,c,v", through Fri 2026-11-13)
#include "../CobCheck.h"
#include <fstream>
static int ok = 0, bad = 0;
#define CHECK(c) do { if (c) ++ok; else { ++bad; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)
int main(int argc, char** argv)
{
    CHECK(argc > 1); if (argc < 2) return 1;
    cobs::Engine E; E.setMarket("ES", 0.25, 180); std::ifstream f(argv[1]); std::string l; std::vector<cobs::Bar> all;
    while (std::getline(f, l)) { cobs::Bar b; if (std::sscanf(l.c_str(), "%lld,%lf,%lf,%lf,%lf,%lf", &b.t, &b.o, &b.h, &b.l, &b.c, &b.v) == 6) all.push_back(b); }
    CHECK(all.size() > 20000);
    // replay: the items already decided never change their checks; Lvp / CW only ever switch on (no repaint)
    std::map<long long, cck::Item> seen; int changed = 0, off = 0; size_t k = 0;
    for (const cobs::Bar& b : all) { E.addBar(b); E.out.clear(); E.lvpOut.clear(); if (++k % 20 && k != all.size()) continue;
        for (const cck::Item& x : cck::scan(E)) { auto it = seen.find(x.t);
            if (it != seen.end()) { const cck::Item& o = it->second;
                if (o.row3 != x.row3 || o.sma != x.sma || o.first != x.first || o.lv != x.lv || o.lvPx != x.lvPx || o.cbT != x.cbT || o.srT != x.srT || o.side != x.side) ++changed;
                if ((o.cw && !x.cw) || (o.lvpAt && !x.lvpAt) || x.cwX + 1e-9 < o.cwX) ++off; }
            seen[x.t] = x; } }
    const std::vector<cck::Item> v = cck::scan(E);
    std::printf("drawn Cobs %zu, checks changed %d, Lvp / CW switched off %d\n", v.size(), changed, off);
    CHECK(!v.empty()); CHECK(changed == 0); CHECK(off == 0);
    // Fri Nov 13 7:33 AM origin: the Cob Dn off R1 7,649.75 (ONH area), count 29 - as lsCobSignal's record: COB,1794557520,BEAR,Cob,R1 / HTF ... R1,7649.75
    const cck::Item* x = nullptr; for (const cck::Item& q : v) if (q.t == 1794557520LL - 2082844800LL + 2082844800LL) x = &q;
    if (!x) for (const cck::Item& q : v) if (q.side < 0 && q.lv == "R1" && std::fabs(q.lvPx - 7649.75) < 1e-6) x = &q;
    CHECK(x != nullptr);
    if (x) { std::printf("%s", cck::statusText(E, v).c_str());
        CHECK(x->side == -1); CHECK(x->lv == "R1"); CHECK(std::fabs(x->lvPx - 7649.75) < 1e-9); CHECK(x->cnt == 29); CHECK(x->first); CHECK(x->row3); CHECK(x->checks == (x->row3 ? 1 : 0) + (x->sma ? 1 : 0) + 1);
        CHECK(x->win == cck::WIN[x->checks >= 3 ? 1 : 0][cck::state(*x)]); CHECK(x->srT > 0 && x->srT <= x->tOrg); CHECK(x->tEnd > x->tOrg && x->t >= x->tEnd); }
    // every item: the checks add up, the win comes from the table, CW means >= 0.8x
    for (const cck::Item& q : v) { CHECK(q.checks == (q.row3 ? 1 : 0) + (q.sma ? 1 : 0) + (q.first ? 1 : 0)); CHECK(q.win == cck::WIN[q.checks >= 3 ? 1 : 0][cck::state(q)]); CHECK(q.cw == (q.cwX >= cck::CW_X)); CHECK(q.first == (q.lv == (q.side > 0 ? "S1" : "R1")) || !q.first); CHECK(q.cbT == 0 || q.cbRank >= cck::CB_MIN); }
    // the read box: 6 rows in order; the newest bar's session (Fri Nov 13) has the Cob Dn
    const cck::Read R = cck::read(E, v);
    CHECK(R.rows.size() == 6); CHECK(R.rows[0].k == "Cob"); CHECK(R.rows[1].k == "Lvl"); CHECK(R.rows[2].k == "Check"); CHECK(R.rows[3].k == "Lvp"); CHECK(R.rows[4].k.rfind("Warn", 0) == 0); CHECK(R.rows[5].k == "Win");
    for (const cck::Row& r : R.rows) std::printf("  %-9s %s [%d]\n", r.k.c_str(), r.v.c_str(), r.col);
    CHECK(R.any && R.dir == -1); CHECK(R.rows[0].v.rfind("Dn ", 0) == 0 && R.rows[0].v.find("(29)") != std::string::npos); CHECK(R.rows[0].col == 2);
    CHECK(R.rows[1].v.rfind("R1 7,649.75", 0) == 0);
    // no Cob in the session = dashes
    { cobs::Engine Z; Z.setMarket("ES", 0.25, 180); const cck::Read N = cck::read(Z, cck::scan(Z)); CHECK(!N.any); CHECK(N.rows.size() == 6); for (const cck::Row& r : N.rows) CHECK(r.v == "-"); }
    // texts
    CHECK(cck::priceText(7649.75, 0.25) == "7,649.75"); CHECK(cck::priceText(28583.75, 0.25) == "28,583.75"); CHECK(cck::priceText(89.25, 0.01) == "89.25"); CHECK(cck::priceText(1.15235, 0.00005) == "1.15235"); CHECK(cck::priceText(3927.2, 0.1) == "3,927.2"); CHECK(cck::priceText(4.0125, 0.0005) == "4.0125");
    CHECK(cck::clock12(86400LL * 5 + 8 * 3600 + 15 * 60) == "8:15 AM"); CHECK(cck::clock12(86400LL * 5 + 12 * 3600) == "12:00 PM"); CHECK(cck::clock12(86400LL * 5 + 5 * 60) == "12:05 AM");
    CHECK(cck::WIN[1][1] > cck::WIN[1][0] && cck::WIN[1][2] < cck::WIN[0][0]);
    std::printf("%d passed, %d failed\n", ok, bad); return bad ? 1 : 0;
}
