// test_readbox_120.cpp -- ReadBox 1.2.0 (Rassul 2026-11-11 11:41-11:53 CT: "updat eht readt at the top to be more simple"; "where it says HTF
// change it to Hrly Trend: and give it values like up or down or NA"; "1. ok. 2. ok 3. ok, and i dont even think it is working becasue the
// levels it is mentioning are passed"). 4 lines: Hrly Trend (+ the hourly breakout level) / Day (no ADR, no Used) / Lvl (nearest above and
// below) / Flow (one line, the verdict first, only events still in play).
// Run (from plugin/): g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. tests_readbox/test_readbox_120.cpp -o t && ./t
#include "../ReadBox.h"
#include <cstdio>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static rbx::In base() {
    rbx::In I; I.mkt = "ES"; I.dec = 2; I.per = 180; I.openMin = 510; I.closeMin = 900; I.day.kind = 0; I.day.pBig = 0.3; I.day.pSmall = 0.35; I.day.xr = 1.0; I.day.adr = 60;
    const long long day0 = 20760LL * 86400; double px = 7500; unsigned s = 7;   // 3-min bars from the prior 17:00 to 10:30
    for (long long t = day0 - 7 * 3600; t <= day0 + 10 * 3600 + 1800; t += 180) { s = s * 1103515245u + 12345u; const double d = ((int)((s >> 16) % 9) - 4) * 0.25;
        rbx::Bar b; b.t = t; b.o = px; px += d; b.c = px; b.h = std::max(b.o, b.c) + 0.5; b.l = std::min(b.o, b.c) - 0.5; I.bars.push_back(b); }
    return I; }
static rbx::Sig sig(long long t, const char* code, int bias, double px, double net, double mult) { rbx::Sig s; s.t = t; s.code = code; s.bias = bias; s.px = px; s.net = net; s.mult = mult; return s; }
int main() {
    rbx::In I = base(); const double px = I.bars.back().c; const long long now = I.bars.back().t;
    {   const rbx::Out o = rbx::build(I);
        CHECK(o.ok && o.n == 1 && o.l[0].text.compare(0, 5, "Flow:") == 0, "the read box is ONE line: the order-flow read");
        CHECK(o.trend.text.compare(0, 12, "Hrly Trend: ") == 0 && o.day.text.compare(0, 4, "Day:") == 0 && o.lvl.text.compare(0, 4, "Lvl:") == 0 && o.l[0].text == "Flow: Even", "Hrly Trend / Day / Lvl / Flow (" + o.l[0].text + ")");
        CHECK(o.day.text.find("ADR") == std::string::npos && o.day.text.find("Used") == std::string::npos, "the RTH Day line has no ADR / Used");
        bool old = false; for (int k = 0; k < o.n; ++k) if (o.l[k].text.find("HTF") != std::string::npos || o.l[k].text.find("Mkt:") != std::string::npos || o.l[k].text.find("Lmt:") != std::string::npos) old = true;
        CHECK(!old, "no HTF / Mkt / Lmt line"); }
    {   // Lvl: the nearest above and below, never a level price has left behind
        rbx::In J = I; J.levels = { { "PDH", px + 30 }, { "LonHI", px + 4.5 }, { "CWH", px - 15 }, { "PDL", px - 60 } };
        const rbx::Out o = rbx::build(J);
        CHECK(o.lvl.text == "Lvl: LonHI " + rbx::num(px + 4.5, 2) + " (4.5 above) / CWH " + rbx::num(px - 15, 2) + " (15 below)", "Lvl names the nearest level above and below with the distance: " + o.lvl.text);
        CHECK(o.lvl.text.find("PDH") == std::string::npos && o.lvl.text.find("PDL") == std::string::npos, "further levels are not named");
        rbx::In K = I; K.levels = { { "PDH", px + 30 } }; const rbx::Out q = rbx::build(K);
        CHECK(q.lvl.text == "Lvl: PDH " + rbx::num(px + 30, 2) + " (30 above) / none below", "one side only: " + q.lvl.text);
        rbx::In N = I; const rbx::Out z = rbx::build(N); CHECK(z.lvl.text == "Lvl: no level near", "no levels: " + z.lvl.text); }
    {   // Flow: verdict first; only events still in play
        rbx::In J = I; J.sigs = { sig(now - 360, "A", -1, px + 1.0, -311, 5.1) };     // sellers absorbing 1 pt above the close
        CHECK(rbx::build(J).l[0].text == "Flow: Sellers absorbing 311 (5.1x) @" + rbx::num(px + 1.0, 2) && rbx::build(J).l[0].col == rbx::RED, "an absorption still above price: " + rbx::build(J).l[0].text);
        J.sigs = { sig(now - 360, "A", -1, px - 1.0, -311, 5.1) };                    // price has gone up through it
        CHECK(rbx::build(J).l[0].text == "Flow: Even", "PASSED: price closed through the sellers' absorption - not mentioned");
        J.sigs = { sig(now - 360, "A", 1, px - 400.0, 200, 3.0) };                    // far behind (more than 1 ATR)
        CHECK(rbx::build(J).l[0].text == "Flow: Even", "left behind by more than 1 ATR - not mentioned");
        J.sigs = { sig(now - 3600, "A", -1, px + 1.0, -311, 5.1) };                   // older than 5 candles
        CHECK(rbx::build(J).l[0].text == "Flow: Even", "older than 5 candles - not mentioned");
        J.sigs = { sig(now - 360, "I", -1, px + 0.5, -100, 2.6), sig(now - 180, "A", -1, px + 1.0, -311, 5.1) };
        CHECK(rbx::build(J).l[0].text == "Flow: Sellers in control: pushing 2.6x, absorbing 311 (5.1x) @" + rbx::num(px + 1.0, 2), "both the same side: " + rbx::build(J).l[0].text);
        J.sigs = { sig(now - 360, "I", 1, px - 0.5, 100, 2.6), sig(now - 180, "A", -1, px + 1.0, -311, 5.1) };
        CHECK(rbx::build(J).l[0].text.compare(0, 12, "Flow: Mixed:") == 0 && rbx::build(J).l[0].col == rbx::WHITE, "opposite sides = Mixed, white: " + rbx::build(J).l[0].text);
        J.sigs = { sig(now - 360, "T", 1, px - 0.5, 0, 0) };
        CHECK(rbx::build(J).l[0].text == "Flow: Sellers trapped @" + rbx::num(px - 0.5, 2) && rbx::build(J).l[0].col == rbx::GREEN, "a trap alone: " + rbx::build(J).l[0].text); }
    {   // Hrly Trend + the hourly breakout level
        rbx::In J = I; const size_t n = J.bars.size();   // the last 3 completed hours rise 6 points an hour, each closing above the prior hour's high
        for (size_t k = n - 70; k < n; ++k) { const double up = (double)(k - (n - 70)) * 0.3; J.bars[k].o += up; J.bars[k].c += up + 0.3; J.bars[k].h += up + 0.3; J.bars[k].l += up; }
        const rbx::Out o = rbx::build(J);
        CHECK(o.trend.text.compare(0, 22, "Hrly Trend: Up, above ") == 0 && o.trend.col == rbx::GREEN, "rising hours: " + o.trend.text);
        rbx::In K = J; K.bars.back().c -= 60; K.bars.back().l -= 60; const rbx::Out q = rbx::build(K);
        CHECK(q.trend.text.compare(0, 27, "Hrly Trend: Up, back below ") == 0, "the last close under the breakout level: " + q.trend.text);
        rbx::In Z = I; Z.bars.resize(3); CHECK(rbx::build(Z).trend.text == "Hrly Trend: NA", "no completed hours: NA"); }
    {   const rbx::Layout L4 = rbx::layout(0, 0, 800, 600, 200, 10, 4), L5 = rbx::layout(0, 0, 800, 600, 200, 10);
        CHECK(L4.ok && L5.ok && (L5.box.b - L5.box.t) - (L4.box.b - L4.box.t) == L4.lh, "the box is one line shorter than the 5-line box"); }
    std::printf("RESULT %d passed, %d failed\n", pass, fail); return fail ? 1 : 0;
}
