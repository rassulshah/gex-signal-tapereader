// Defense 1.14 audit: direct integration of the production OrderFlowFeed and
// BookFlowLogic headers; no extracted production-function copies or SDK mock.
#include "BookFlowLogic.h"
#include <cstdio>
#include <limits>
#include <string>

static int checks = 0, fails = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static off::Rec watchRec() {
    off::Rec r;
    r.addC[off::BID] = 900; r.pullC[off::BID] = 100;    // defenders: +800
    r.addC[off::ASK] = 300;                              // other side: +300
    r.hitC[off::BID] = 500; r.refC[off::BID] = 2000;    // 4.0 raw -> STRONG ES refill
    return r;
}

int main() {
    {   // A price untraded for exactly 300 seconds is a new fresh touch.
        off::Engine e; e.rest[off::BID][100] = 80;
        e.touchFill(off::BID, 100, 10, 1000, 80);
        e.touchFill(off::BID, 100, 10, 1300, 80);
        CHECK(e.touches.size() == 2 && e.touches[1].t0 == 1300 && e.lastTradeS[off::BID][100] == 1300);
    }
    {   // Time cannot be rewound: an older event neither changes lastTradeS nor enters h1/h2.
        off::Engine e; e.rest[off::BID][100] = 80;
        e.touchFill(off::BID, 100, 10, 1000, 80);
        e.touchFill(off::BID, 100, 10, 900, 80);
        e.touchFill(off::BID, 100, 10, 1201, 80);
        CHECK(e.touches.size() == 1 && e.touches[0].h1 == 10 && e.touches[0].h2 == 0 && e.lastTradeS[off::BID][100] == 1201);
    }
    {   // Invalid side, price, amount, or timestamp makes no touch and cannot advance the clock.
        off::Engine e;
        e.touchFill(7, 100, 10, 1000, 80); e.touchFill(off::BID, 0, 10, 1000, 80);
        e.touchFill(off::BID, 100, 0, 1000, 80); e.touchFill(off::BID, 100, -1, 1000, 80);
        e.touchFill(off::BID, 100, 10, 0, 80);
        CHECK(e.touches.empty() && e.lastS == 0 && e.lastTradeS[off::BID].empty());
    }
    {   // finishTouches cannot decide before all 20--60 seconds; its h2 rate is over all 40 seconds.
        off::Engine e; e.adMedian = 0.095; e.rest[off::BID][100] = 80;
        e.touchFill(off::BID, 100, 40, 1000, 80);
        e.touchFill(off::BID, 100, 2, 1020, 80);
        e.rest[off::BID][100] = 40;
        e.finishTouches(1059); CHECK(e.touches.size() == 1 && !e.touches[0].done && e.touches[0].h1 == 40 && e.touches[0].h2 == 2);
        e.finishTouches(1060); CHECK(e.touches[0].done && e.touches[0].ad);
    }
    {   // A non-finite engine median cannot create an attack-died decision.
        off::Engine e; e.adMedian = std::numeric_limits<double>::infinity(); e.rest[off::BID][100] = 80;
        e.touchFill(off::BID, 100, 40, 1000, 80); e.rest[off::BID][100] = 40; e.finishTouches(1060);
        CHECK(e.touches.size() == 1 && e.touches[0].done && !e.touches[0].ad);
    }
    {   // CAUTION starts at 25 seconds, while WATCH requires the full 20--60-second phase.
        bf::EwTouch t; t.ok = true; t.s = off::BID; t.px = 7746.5; t.elapsed = 45; t.h1 = 400; t.h2 = 10;
        br::Read out; off::Rec r = watchRec();
        CHECK(!bf::earlyRead(t, r, "ES", 0.095, 0.25, out));
        t.elapsed = 60; CHECK(bf::earlyRead(t, r, "ES", 0.095, 0.25, out) && out.tag == "WATCH" && out.impl == br::BULL);
        t.elapsed = 25; off::Rec c = r; c.addC[off::BID] = 300; c.pullC[off::BID] = 100; c.addC[off::ASK] = 300;
        CHECK(bf::earlyRead(t, c, "ES", 0.095, 0.25, out) && out.tag == "CAUTION" && out.impl == br::BEAR);
    }
    {   // Both signals are strictly net-side larger: 1.3x WATCH and 1.1x CAUTION pass; equality fails.
        bf::EwTouch t; t.ok = true; t.s = off::BID; t.px = 7746.5; t.elapsed = 60; t.h1 = 400; t.h2 = 10;
        br::Read out; off::Rec r = watchRec();
        r.addC[off::ASK] = 600; CHECK(bf::earlyRead(t, r, "ES", 0.095, 0.25, out) && out.tag == "WATCH");
        r.addC[off::ASK] = 800; CHECK(!bf::earlyRead(t, r, "ES", 0.095, 0.25, out));
        r.addC[off::ASK] = 900; CHECK(bf::earlyRead(t, r, "ES", 0.095, 0.25, out) && out.tag == "CAUTION" && out.head.find("1.1x more above") != std::string::npos);
    }
    {   // Reject invalid side and every non-finite or non-positive value needed to construct a valid read.
        bf::EwTouch t; t.ok = true; t.s = off::BID; t.px = 7746.5; t.elapsed = 60; t.h1 = 400; t.h2 = 10;
        br::Read out; const off::Rec r = watchRec();
        t.s = 7; CHECK(!bf::earlyRead(t, r, "ES", 0.095, 0.25, out));
        t.s = off::BID; t.h2 = -1; CHECK(!bf::earlyRead(t, r, "ES", 0.095, 0.25, out));
        t.h2 = 10; CHECK(!bf::earlyRead(t, r, "ES", std::numeric_limits<double>::quiet_NaN(), 0.25, out));
        CHECK(!bf::earlyRead(t, r, "ES", std::numeric_limits<double>::infinity(), 0.25, out));
        t.px = std::numeric_limits<double>::quiet_NaN(); CHECK(!bf::earlyRead(t, r, "ES", 0.095, 0.25, out));
        t.px = 7746.5; CHECK(!bf::earlyRead(t, r, "ES", 0.095, std::numeric_limits<double>::infinity(), out));
        CHECK(!bf::earlyRead(t, r, "ES", 0.095, 0.0, out));
    }
    std::printf("Defense 1.14 real-header integration: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
