// test_cvd_review.cpp -- the LRA review fixes in CvdEngine.h (R1 whole ticks, R2 stable IDs) and the per-market scale in CvdFeed.h.
// From tests/:  g++ -std=c++17 -Wall -Wextra -I../plugin test_cvd_review.cpp -o tr && ./tr
#include "CvdFeed.h"
#include <cstdio>
using namespace cvd;
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static double f(double v) { return (double)(float)v; }   // what IRT hands over: float prices
static Bar bar(int seq, double o, double h, double l, double c, double buy, double sell, const std::string& anchor = "S1") {
    Bar b; b.seq = seq; b.timestamp = "T" + std::to_string(seq); b.anchor = anchor;
    b.open = f(o); b.high = f(h); b.low = f(l); b.close = f(c); b.buy = buy; b.sell = sell; b.total = buy + sell; return b;
}
static Config gc() {
    Config c; c.tick_size = 0.1; c.lookback = 2; c.enable_div = c.enable_exh = c.enable_br = false; c.enable_abs = true; c.stall_window = 2;
    c.min_relative_volume = 0; c.level_tolerance_ticks = 3; c.confirmation_cvd_units = .1; return c;
}
int main() {
    // R1: GC, level 2650.0, the bars' low is exactly 3 ticks (the tolerance) above it - given as float 2650.3
    CHECK(!(f(2650.3) <= 2650.0 + 3 * 0.1), "the v0.1 double compare misses this touch (float 2650.3 > 2650.0 + 0.3)");
    {
        Engine e(gc()); std::vector<Event> ev;
        for (int i = 0; i < 4; ++i) {
            Bar b = bar(i, 2650.5, 2650.6, 2650.3, 2650.5, i < 2 ? 100 : 20, i < 2 ? 100 : 180);
            if (i >= 2) b.levels.push_back(Level{ "PDL", 2650.0, true, 0 });
            const Output o = e.push(b); ev.insert(ev.end(), o.events.begin(), o.events.end());
        }
        bool abs = false; for (const Event& x : ev) if (x.type == Type::Abs && x.state == State::Pending) abs = true;
        CHECK(abs, "R1: whole-tick touch at exactly the tolerance -> Abs? (bullish absorption at PDL)");
    }
    // R2: the same session gives the same event IDs whether or not an earlier session was fed first
    {
        auto run = [](bool withEarlier) {
            Config c = gc(); Engine e(c); std::vector<std::string> ids; int seq = 0;
            if (withEarlier) for (int i = 0; i < 6; ++i) { Bar b = bar(seq++, 2650.5, 2650.6, 2650.3, 2650.5, i % 3 == 2 ? 20 : 100, i % 3 == 2 ? 180 : 100, "S0"); b.levels.push_back(Level{ "PDL", 2650.0, true, 0 }); e.push(b); }
            for (int i = 0; i < 4; ++i) {
                Bar b = bar(seq++, 2650.5, 2650.6, 2650.3, 2650.5, i < 2 ? 100 : 20, i < 2 ? 100 : 180, "S1");
                if (i >= 2) b.levels.push_back(Level{ "PDL", 2650.0, true, 0 });
                for (const Event& x : e.push(b).events) if (x.anchor == "S1") ids.push_back(x.id.substr(x.id.find(':', 3)));
            }
            return ids;
        };
        const std::vector<std::string> a = run(false), b = run(true);
        CHECK(!a.empty() && a.size() == b.size(), "R2: same events");
        bool noCounter = true; for (const std::string& s : a) if (s.find("bull:PDL") == std::string::npos) noCounter = false;
        CHECK(noCounter, "R2: ID = anchor:seq:type:dir:level, no running counter");
    }
    // scale: ES 3-min = the package defaults; NQ / 6E scaled by their own bar range; never below 1 tick; 1-min ES smaller
    {
        const Config es = cvdf::scaled(cvdf::rangeTable("ES"), 0.25), nq = cvdf::scaled(cvdf::rangeTable("NQ"), 0.25), eu = cvdf::scaled(cvdf::rangeTable("EU"), 0.00005);
        const Config d;
        CHECK(es.level_tolerance_ticks == d.level_tolerance_ticks && es.stall_range_ticks == d.stall_range_ticks && es.invalidate_ticks == d.invalidate_ticks, "ES 3-min = package defaults");
        CHECK(nq.stall_range_ticks == 44 && nq.level_tolerance_ticks == 17, "NQ: 8 x 83/15 = 44, 3 x 83/15 = 17");
        CHECK(eu.price_extension_ticks == 1 && eu.confirm_ticks == 1 && eu.stall_range_ticks == 2, "6E: never below 1 tick");
        const Config es1 = cvdf::scaled(cvdf::rangeTable("ES") * std::sqrt(60 / 180.0), 0.25);
        CHECK(es1.stall_range_ticks == 5, "ES 1-min: 8 x 8.66/15 = 5");
    }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
