// ReadBox 1.6.0: the Flow read in two lines - Now (forming candle's market orders) + Above / Below (decided event in play, time, distance)
#include "../ReadBox.h"
#include <iostream>
static int P = 0, Fl = 0;
#define CHECK(c, m) do { if (c) ++P; else { ++Fl; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
int main() {
    using namespace rbx;
    // Now line
    { LiveIn k; Line L = nowLine(k, "11:18"); CHECK(L.text.empty(), "no producer: no Now line"); }
    { LiveIn k; k.ok = true; k.buy = 64; k.sell = 23; k.x = 2.1; Line L = nowLine(k, "11:18"); CHECK(L.text == "Now 11:18: Buyers lifting - delta +41 (2.1x)" && L.col == GREEN, "buyers (" << L.text << ")"); }
    { LiveIn k; k.ok = true; k.buy = 10; k.sell = 48; k.x = 1.8; Line L = nowLine(k, "11:18"); CHECK(L.text == "Now 11:18: Sellers hitting - delta -38 (1.8x)" && L.col == RED, "sellers (" << L.text << ")"); }
    { LiveIn k; k.ok = true; k.buy = 23; k.sell = 21; k.x = 0.2; Line L = nowLine(k, "11:18"); CHECK(L.text == "Now 11:18: Even - 23 bought, 21 sold" && L.col == WHITE, "even (" << L.text << ")"); }
    { LiveIn k; k.ok = true; Line L = nowLine(k, "11:18"); CHECK(L.text == "Now 11:18: no trades yet", "no trades yet"); }
    { LiveIn k; k.ok = true; k.buy = 30; k.sell = 10; Line L = nowLine(k, "11:18"); CHECK(L.text == "Now 11:18: Buyers lifting - delta +20", "no normal yet: no ratio (" << L.text << ")"); }
    // level line: HG 11:16 - sellers absorbed 22 (3.4x) @5.9993 decided 11:07, price 5.9920 (tick 0.0005 -> 15 ticks)
    In I; I.dec = 4; I.per = 180; I.tick = 0.0005; const long long t0 = 11 * 3600 + 13 * 60;   // CT wall clock seconds of the day (any day works)
    for (int i = 0; i < 6; ++i) { Bar b; b.t = t0 - (5 - i) * 180; b.o = b.c = 5.990; b.h = 5.991; b.l = 5.989; I.bars.push_back(b); }
    Sig a; a.t = 11 * 3600 + 7 * 60; a.code = "A"; a.bias = -1; a.px = 5.9993; a.net = 22; a.mult = 3.4; I.sigs.push_back(a);
    I.livePx = 5.9920;
    { Line L = flowLine(I, t0, 0.02); CHECK(L.text == "Above: Sellers absorbed 22 (3.4x) @5.9993 (11:07, 15 ticks)" && L.col == RED, "above (" << L.text << ")"); }
    I.livePx = 5.9995; { Line L = flowLine(I, t0, 0.02); CHECK(L.text == "Level: none in play" && L.col == GREY, "traded through: none (" << L.text << ")"); }
    { In J = I; J.livePx = 5.9990; J.sigs[0].bias = 1; J.sigs[0].px = 5.9950; Line L = flowLine(J, t0, 0.02); CHECK(L.text.rfind("Below: Buyers absorbed 22 (3.4x) @5.995 (11:07, 8 ticks)", 0) == 0 && L.col == GREEN, "below (" << L.text << ")"); }
    { In J = I; J.livePx = 5.9920; J.tick = NAN; Line L = flowLine(J, t0, 0.02); CHECK(L.text.find("(11:07, 0.0073 pts)") != std::string::npos, "no tick: points (" << L.text << ")"); }
    { CHECK(clockOf(13 * 3600 + 5 * 60) == "1:05" && clockOf(0) == "12:00", "clock"); }
    // compose: Now, level, Book in order; applyNow / applyFlow keep the order; an empty Now drops out
    { Out o; o.ok = true; o.flow.text = "Level: none in play"; compose(o); CHECK(o.n == 1 && o.l[0].text == "Level: none in play", "level only");
      LiveIn k; k.ok = true; k.buy = 64; k.sell = 23; k.x = 2.1; applyNow(o, nowLine(k, "11:18")); CHECK(o.n == 2 && o.l[0].text.rfind("Now 11:18", 0) == 0 && o.l[1].text == "Level: none in play", "now first");
      Line f; f.text = "Above: x"; f.col = RED; applyFlow(o, f); CHECK(o.n == 2 && o.l[1].text == "Above: x" && o.l[0].text.rfind("Now", 0) == 0, "level refreshed in place");
      o.bkLine.text = "Book: Bids defending 2.1x (2.0x)"; o.bkLine.col = GREEN; compose(o); CHECK(o.n == 3 && o.l[2].text.rfind("Book:", 0) == 0 && o.l[2].col == WHITE, "book third, white under a red line");
      applyNow(o, Line()); CHECK(o.n == 2 && o.l[0].text == "Above: x" && o.l[1].text.rfind("Book:", 0) == 0, "Now gone: level then book"); }
    CHECK(std::string(VERSION) == "1.6.0", "version");
    std::cout << "t_readbox_160: " << P << " passed, " << Fl << " failed\n"; return Fl ? 1 : 0;
}
