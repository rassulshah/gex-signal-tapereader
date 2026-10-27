// test_levelscolumn.cpp -- LevelsColumn.h 1.0.0 (lsDealerProfile 2.28.0 combined Levels + Liquidity column), SDK-free.
// [real] ES 2026-10-27 13:06 levels (lra levels tool) + the limit orders on Rassul's 13:06 chart.
// Run (from plugin/): g++ -std=c++17 -I. tests_lvc/test_levelscolumn.cpp -o t && ./t
#include "LevelsColumn.h"
#include <cstdio>
#include <string>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) { ++passes; std::printf("PASS %s\n", std::string(m).c_str()); } else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static lvc::In L(double px, const char* n, int src) { lvc::In x; x.px = px; x.name = n; x.src = src; return x; }
static std::string chips(const lvc::Row& R) { std::string s; for (auto& c : R.chips) s += (s.empty() ? "" : " | ") + c.text + "(" + std::to_string(c.fam) + ")"; return s; }
int main() {
    const double tick = 0.25, atr = 3.0, last = 7755.0;
    std::vector<lvc::In> in = { L(7749.25, "ONH/LonHI", 0), L(7739.5, "PFH/PDH", 0), L(7734.75, "Open", 0), L(7731.0, "AsiaHI", 0), L(7736.5, "VAH", 0),
        L(7714.5, "DPoc", 0), L(7690.0, "WPoc", 0), L(7699.75, "ONL/LonLO", 0), L(7750.0, "G2", 1), L(7755.0, "G3", 1), L(7760.0, "CR/CR0/GW0", 1),
        L(7765.0, "G4", 1), L(7725.0, "HVL", 1), L(7700.0, "PS/PS0", 1), L(7690.0, "GEX", 2), L(7697.0, "VAL", 0) };
    for (size_t i = 0; i < in.size(); ++i) in[i].idx = i;
    std::vector<lvc::Row> R = lvc::group(in, tick, atr);
    auto rowAt = [&](double px) -> const lvc::Row* { for (auto& r : R) if (std::fabs(r.px - px) < 1e-9) return &r; return nullptr; };
    CHECK(R.size() == 13, "13 rows from 16 levels (ONH/G2, ONL/PS, WPoc/GEX merge) got " + std::to_string(R.size()));
    for (size_t k = 1; k < R.size(); ++k) CHECK(R[k - 1].px > R[k].px, "rows top-down " + std::to_string(R[k].px));
    const lvc::Row* a = rowAt(7749.25);
    CHECK(a && chips(*a) == "ONH/LonHI(0) | G2(2)", "ONH/LonHI (key) before G2 (gamma), row at the key price: " + (a ? chips(*a) : std::string("none")));
    const lvc::Row* b = rowAt(7699.75);
    CHECK(b && chips(*b) == "ONL/LonLO(0) | PS/PS0(2)", "ONL/LonLO before PS/PS0: " + (b ? chips(*b) : std::string("none")));
    const lvc::Row* c = rowAt(7690.0);
    CHECK(c && chips(*c) == "WPoc(1) | GEX(2)", "volume (WPoc) before gamma (GEX): " + (c ? chips(*c) : std::string("none")));
    CHECK(rowAt(7736.5) && rowAt(7734.75), "VAH 7,736.50 and Open 7,734.75 (1.75 apart > 0.9) stay separate rows");
    {   // a key-level merge that holds a volume token: "VAH/PDH" -> PDH (key) first, then VAH (volume)
        std::vector<lvc::In> v = { L(7700.0, "VAH/PDH", 0), L(7700.0, "CR", 1) };
        const std::vector<lvc::Row> q = lvc::group(v, tick, atr);
        CHECK(q.size() == 1 && chips(q[0]) == "PDH(0) | VAH(1) | CR(2)", "split one name across families, key > volume > gamma: " + (q.empty() ? std::string() : chips(q[0])));
    }
    {   // the same name from two sources is drawn once
        std::vector<lvc::In> v = { L(7760.0, "CR0", 1), L(7760.0, "CR0", 1) };
        CHECK(lvc::group(v, tick, atr)[0].chips.size() == 1, "duplicate CR0 drawn once");
    }
    // the order file (LP 0.60.0) and the ranks
    const long long now = 1793126000;
    const std::string file = "LMTL,1,1793125995,0.25\nO,A,800,800,G4,7765.00,7765.50\nO,A,50,50,G4,7765.00,7765.25\nO,A,100,100,CR0,7760.00,7760.25\n"
                             "O,A,100,100,G2,7750.00,7750.25\nO,B,150,4o 150,PFH,7739.50,7740.25\nO,B,52,52,VAH,7736.50,7737.25\nO,B,54,54,VAH,7736.50,7737.75\n"
                             "O,X,9,9,VAH,7736.5,7737\ngarbage\n";
    std::vector<lvc::Order> O; std::string why;
    CHECK(lvc::parseOrders(file, now, O, why) && O.size() == 7, "7 orders read, torn lines skipped (" + why + ")");
    std::vector<lvc::Order> O2; CHECK(!lvc::parseOrders(file, now + 60, O2, why) && why == "stale", "an order file older than 20 s = no liquidity");
    lvc::attach(R, in, O, tick, atr, last);
    std::vector<size_t> all; for (size_t k = 0; k < R.size(); ++k) all.push_back(k);
    lvc::rank(R, all, last);
    auto rk = [&](double px) { const lvc::Row* r = rowAt(px); return r ? r->rank : -1; };
    auto lq = [&](double px) { const lvc::Row* r = rowAt(px); return r ? r->liq : std::string("-"); };
    CHECK(lq(7765.0) == "800 + 50" && rk(7765.0) == 1, "G4 '800 + 50' rank 1: " + lq(7765.0));
    CHECK(lq(7739.5) == "4o 150" && rk(7739.5) == 2, "PFH '4o 150' rank 2");
    CHECK(lq(7736.5) == "54 + 52" && rk(7736.5) == 3, "VAH '54 + 52' (106) rank 3 (biggest first): " + lq(7736.5));
    CHECK(rk(7760.0) == 4 && rk(7749.25) == 5, "100 vs 100: the tie goes to the row nearer 7,755 - CR0 (5.00 away) 4, ONH/LonHI | G2 (5.75) 5");
    CHECK(lq(7749.25) == "100" && !rowAt(7749.25)->bid, "the G2 offer joins the ONH/LonHI | G2 row, red");
    CHECK(rowAt(7739.5)->bid, "PFH bids = green");
    CHECK(rk(7755.0) == 0 && lq(7755.0).empty(), "G3: no orders, no rank");
    std::printf("RESULT %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
