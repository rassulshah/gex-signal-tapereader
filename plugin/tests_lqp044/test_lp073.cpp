// test_lp073.cpp -- lsLiquidityProfile 0.73.0 (2026-11-03 night, Rassul "fix all" after the audit of his 7:49 PM CT ES files): SDK-free.
// 1) a WALL needs BOTH >= 2.0x the book behind it AND >= 1.5x a normal band: a 250-lot zone in front of an empty book (bx 50x) with a
//    266-lot normal is never drawn / published / an LQ; the same zone with a 100-lot normal is; 2) the LABEL = lots x the normal band
//    ("0.9x", "2.5x", "10x+"), not the emptiness behind it; 3) a drawn wall that falls under 1.5x normal leaves at the next bar close (not
//    mid-bar); 4) the NEAREST non-wall zone per side within the near band (durable, nearest first) for the dim N rows; 5) no normal yet =
//    the size gate is off (0.72 behaviour).
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. tests_lqp044/test_lp073.cpp -o t073 && ./t073
#include "LiquidityProfileLogic.h"
#include <cstdio>
#include <string>
#include <vector>
static int fails = 0, passes = 0;
#define CHECK(cond, msg) do { if (cond) ++passes; else { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); } } while (0)
using namespace lqp;
typedef std::vector<std::pair<double, double>> Lv;
// a THIN evening book: 1 lot per level from 70.00 down to 69.40 / 70.01 up to 70.61, plus extra lots per tick
static void book(Engine& e, Ms t, const std::map<long long, double>& xb, const std::map<long long, double>& xa = {}) {
    Lv b, a;
    for (int i = 0; i < 61; ++i) {
        const long long tb = 7000 - i, ta = 7001 + i;
        b.push_back(std::make_pair(tb * 0.01, 1.0 + (xb.count(tb) ? xb.at(tb) : 0)));
        a.push_back(std::make_pair(ta * 0.01, 1.0 + (xa.count(ta) ? xa.at(ta) : 0)));
    }
    e.onBook(t, b, a);
}
static Engine fresh(double normal) { Engine e; e.P.tick = 0.01; e.P.hwMin = 2; e.P.hwMax = 2; e.P.persistMs = 30000; e.P.requireShown = true; if (normal > 0) e.norms.median[0] = normal; e.onLastPrice(70.00); return e; }
static const Zone* zoneAt(const Engine& e, int side, Tick c) { for (size_t i = 0; i < e.zones.size(); ++i) if (e.zones[i].lo <= c && e.zones[i].hi >= c && e.zones[i].side == side) return &e.zones[i]; return nullptr; }
static void close(Engine& e, int bar, Ms t) { std::vector<std::pair<Tick, Tick>> none; e.onBarClose(bar, 1000 + bar, t, 70.00, none, none); }
static std::map<long long, double> wall(long long c, double each) { std::map<long long, double> m; for (long long p = c - 2; p <= c + 2; ++p) m[p] = each; return m; }

int main() {
    {   // 1) + 2) the 7:49 PM case: a 250-lot band in front of an EMPTY book (~5 lots behind) with a 266-lot normal
        Engine e = fresh(266);
        for (int s = 0; s <= 40; ++s) { book(e, s * 1000, wall(6990, 49)); e.updateZones(s * 1000, 0.08, 0); }
        const Zone* z = zoneAt(e, BID, 6990);
        CHECK(z != nullptr, "the 250-lot band is tracked");
        if (z) {
            CHECK(z->bx >= 40, "its old ratio (vs the empty book behind) is huge: 50x");
            CHECK(!z->drawn && e.drawOrder().empty(), "but 250 lots = 0.94x a normal band: NOT a wall (never drawn / published)");
            CHECK(e.normLabel(*z) == "0.9x", "the label = lots x the normal band: 0.9x (was 10x+)");
            CHECK(e.confirming(6990, 7000) == nullptr, "and it never confirms a level (LQ)");
        }
    }
    {   // the same band with a 100-lot normal: 2.5x a normal band AND 50x the book behind = a wall
        Engine e = fresh(100);
        for (int s = 0; s <= 40; ++s) { book(e, s * 1000, wall(6990, 49)); e.updateZones(s * 1000, 0.08, 0); }
        const Zone* z = zoneAt(e, BID, 6990);
        CHECK(z && z->drawn && e.drawOrder().size() == 1, "250 lots = 2.5x a 100-lot normal and 50x the book behind: a wall");
        CHECK(z && e.normLabel(*z) == "2.5x", "label 2.5x");
        CHECK(z && e.normOk(*z), "normOk");
        // 3) it shrinks to 140 lots (1.4x normal, still >> the book behind) mid-bar: kept until the close, then removed
        for (int s = 41; s <= 45; ++s) { book(e, s * 1000, wall(6990, 27)); e.updateZones(s * 1000, 0.08, 0); }
        z = zoneAt(e, BID, 6990);
        CHECK(z && z->drawn && z->bx >= 2.0 && z->size < 150, "1.4x normal mid-bar: still drawn (decided at the close, no flicker)");
        close(e, 1, 46000);
        z = zoneAt(e, BID, 6990);
        CHECK(z && !z->drawn, "at the bar close under 1.5x normal: removed (the bx alone would have kept it)");
    }
    {   // the 10x+ cap
        Engine e = fresh(20);
        for (int s = 0; s <= 40; ++s) { book(e, s * 1000, wall(6990, 49)); e.updateZones(s * 1000, 0.08, 0); }
        const Zone* z = zoneAt(e, BID, 6990);
        CHECK(z && e.normLabel(*z) == "10x+", "250 lots / 20 normal = 12.5x -> \"10x+\"");
    }
    {   // 5) no normal known: the size gate is off (the 0.72 rule)
        Engine e = fresh(0); e.normal = 0;
        Zone z; z.size = 5;
        CHECK(e.normOk(z) && e.normLabel(z).empty(), "no normal yet: size gate off, no label");
    }
    {   // 4) the NEAREST non-wall zone per side within the near band (8 ticks here: 4 half-widths)
        Engine e = fresh(266);
        std::map<long long, double> b = wall(6996, 9), bfar = wall(6990, 49);   // a 50-lot bid band 4 ticks under price, a 250 band 10 under
        for (auto& kv : bfar) b[kv.first] = kv.second;
        const std::map<long long, double> a = wall(7004, 9);                   // a 50-lot offer band 4 ticks over price
        for (int s = 0; s <= 20; ++s) { book(e, s * 1000, b, a); e.updateZones(s * 1000, 0.08, 0); }
        long nb = -1, na = -1; e.nearestNear(nb, na);
        CHECK(nb < 0 && na < 0, "not rested 30 s yet: no nearest zone");
        for (int s = 21; s <= 40; ++s) { book(e, s * 1000, b, a); e.updateZones(s * 1000, 0.08, 0); }
        e.nearestNear(nb, na);
        CHECK(nb >= 0 && e.zones[(size_t)nb].lo <= 6996 && e.zones[(size_t)nb].hi >= 6996, "nearest support = the 50-lot band 4 ticks under (not the bigger far one)");
        CHECK(na >= 0 && e.zones[(size_t)na].lo <= 7004 && e.zones[(size_t)na].hi >= 7004, "nearest resistance = the 50-lot offer band");
        CHECK(nb >= 0 && !e.zones[(size_t)nb].drawn && e.normLabel(e.zones[(size_t)nb]) == "0.2x", "it is not a wall; its label 0.2x of normal (shown dim)");
        // a wall near price is not a 'nearest dim' zone (it is drawn as a wall already)
        Engine w = fresh(30);
        for (int s = 0; s <= 40; ++s) { book(w, s * 1000, wall(6996, 9), a); w.updateZones(s * 1000, 0.08, 0); }
        long wb = -1, wa = -1; w.nearestNear(wb, wa);
        const Zone* zw = zoneAt(w, BID, 6996);
        CHECK(zw && zw->drawn, "50 lots = 1.7x a 30-lot normal and 10x the book behind: a wall near price");
        CHECK(wb < 0 || w.zones[(size_t)wb].id != zw->id, "a drawn wall is never also listed as the dim nearest zone");
        CHECK(wb < 0, "and the flat 5-lot bands next to it (exactly 1.0x the book behind them) are not listed");
        // the floor: a near band no bigger than the book behind it (bx < 1.0) is never the nearest zone
        Engine f = fresh(266);
        for (int s = 0; s <= 40; ++s) { book(f, s * 1000, {}, a); f.updateZones(s * 1000, 0.08, 0); }
        long fb = -1, fa = -1; f.nearestNear(fb, fa);
        CHECK(fb < 0, "a flat book: no bid band (1.0x the book behind) is listed as the nearest zone");
    }
    std::printf("lqp %s (0.73.0) wall size gate / label / nearest: %d passed, %d failed\n", VERSION, passes, fails);
    return fails ? 1 : 0;
}
