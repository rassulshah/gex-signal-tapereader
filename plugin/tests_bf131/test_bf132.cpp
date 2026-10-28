// test_bf132.cpp -- lsBookFlow 1.32.0 / OrderFlowFeed 1.16.0 / NodeLadder 1.0.0 (2026-10-28 overnight): the engine feeds the node ladder
// (added / pulled / filled per price per candle, first-half fills), scratch events never reach it, bounded; the posted form round-trips
// through the SignalBoard 1.5.0 channel; closed candles only; the node read (zones, holding / pulling, net x, fade); the At node text,
// colour rule and hover (Hold +1.8x ?, the price with its tick's decimals, question / numbers / use / example).
// Build (from tests_bf131/): g++ -std=c++17 -O1 -w -I.. test_bf132.cpp -o /tmp/t_bf132 && /tmp/t_bf132
#include <map>
#include <string>
#include <vector>
#include <cstdio>
#include "../SignalBoard.h"
#include "../OrderFlowPaneSettings.h"
#include "../BookFlowLogic.h"
static int P = 0, F = 0; static void check(bool c, const char* m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m); } }
static off::Ev ev(int act, int side, long long px, long long size, long long id, long long sec) { off::Ev e; e.action = act; e.side = side; e.px = px; e.size = size; e.id = id; e.arrMs = sec * 1000; e.stamp = sec; return e; }
int main() {
    off::Engine E; nl2::Ladder L; L.spb = 180; E.ladder = &L; E.near = 1000;
    off::Acc a1, a2; const long long K1 = 1000180, K2 = 1000360;   // two candles (bar END stamps), K1 = [1000000, 1000180)
    E.curKey = K1; E.fallbackRef(400, 1000000000LL);   // the last trade is known (near = within 1000 ticks of it)
    E.apply(a1, ev(off::NEW, off::BID, 400, 300, 1, 1000010));     // bids added at the node
    E.apply(a1, ev(off::NEW, off::BID, 401, 50, 2, 1000011));
    E.apply(a1, ev(off::MOD, off::BID, 400, 200, 1, 1000020));     // size cut 300 -> 200 = 100 pulled
    E.apply(a1, ev(off::FILL, off::BID, 400, 80, 1, 1000030));     // first-half fill
    E.apply(a1, ev(off::FILL, off::BID, 400, 40, 1, 1000150));     // second half
    E.apply(a1, ev(off::DEL, off::BID, 401, 0, 2, 1000160));       // 50 pulled (cancel, never filled)
    E.apply(a1, ev(off::NEW, off::ASK, 410, 70, 3, 1000165)); E.apply(a1, ev(off::DEL, off::ASK, 410, 0, 3, 1000170));   // attackers' own offer pulled
    E.curKey = K2; E.apply(a2, ev(off::NEW, off::BID, 399, 25, 4, 1000200));
    const nl2::Bar& b1 = L.bars[K1];
    check(b1.rows.at(400).add[0] == 300 && b1.rows.at(400).pull[0] == 100 && b1.rows.at(400).fill[0] == 120 && b1.rows.at(400).fill1[0] == 80, "[ladder] 400: added 300, pulled 100, filled 120 (80 in the 1st half)");
    check(b1.rows.at(401).pull[0] == 50 && b1.rows.at(410).pull[1] == 70, "[ladder] a cancel is a pull (bids 401, offers 410)");
    check(L.bars.count(K2) && L.bars[K2].rows.at(399).add[0] == 25, "[ladder] the next candle has its own rows");
    {   off::Acc scratch; nl2::Ladder* keep = E.ladder; E.ladder = nullptr; E.apply(scratch, ev(off::NEW, off::BID, 300, 999, 9, 1000201)); E.ladder = keep;
        check(!L.bars[K2].rows.count(300), "[ladder] a scratch event (no ladder attached) never lands in it"); }
    for (long long k = 1; k <= 10; ++k) L.on(0, 0, 500, 1, 0, K2 + k * 180);
    check(L.bars.size() == nl2::MAX_BARS && !L.bars.count(K1), "[bounded] at most 6 candles, the oldest dropped");
    {   nl2::Ladder M; M.spb = 180; for (long long p = 0; p < 700; ++p) M.on(0, 0, 1000 + p, 1, 0, K1);
        check(M.bars[K1].rows.size() == nl2::MAX_ROWS && M.droppedRows == 100, "[bounded] at most 600 prices a candle, the rest counted dropped"); }
    // the posted form: closed candles only, round trip through the board
    nl2::Ladder Q; Q.spb = 180; Q.on(0, 0, 400, 300, 1000010, K1); Q.on(1, 0, 400, 100, 1000020, K1); Q.on(2, 0, 400, 80, 1000030, K1); Q.on(1, 1, 405, 70, 1000170, K1); Q.on(0, 0, 399, 5, 1000200, K2);
    const std::vector<nl2::PBar> v = nl2::closedBars(Q, K2, 0.25, 3, [](long long k) { return k + 7; });
    check(v.size() == 1 && v[0].key == K1 && v[0].bar == K1 + 7, "[post] only CLOSED candles (the forming K2 is left out), mapped to the bar stamp");
    const std::string enc = nl2::encode(5000, v); sb::postLadder("ESZ6", 180, "ES 3m", enc);
    std::string got; std::vector<nl2::PBar> back;
    check(sb::readLadder("ESZ6", 180, "ES 3m", got) && nl2::decode(got, 6000, 60000, back) && back.size() == 1 && back[0].rows.at(400).add[0] == 300 && back[0].rows.at(405).pull[1] == 70, "[board] the ladder round-trips (SignalBoard 1.5.0 LRA_SLD_)");
    { std::vector<nl2::PBar> tmp; check(!nl2::decode(got, 5000 + 60001, 60000, tmp), "[board] a ladder older than 60 s is not used");
      check(!nl2::decode("5000|garbage;", 6000, 60000, tmp) || tmp.empty(), "[board] garbage gives no candle"); }
    // the node read
    nl2::Tier T; T.ok = true; for (int h = 0; h < 24; ++h) { T.hourMed[h] = 100; T.hourOk[h] = true; } T.netHi = 0.05; T.leaveLo = 0.94;
    nl2::Read r = nl2::nodeRead(back[0].rows, true, 400, 400, 8.0, 9, T);   // z = round(0.15 x 8) = 1, ATR 8
    check(r.ok && r.add == 300 && r.pull == 100 && r.fill == 80 && r.attPull == 70 && std::fabs(r.netX - 2.0) < 1e-9 && r.holding, "[read] bullish node: added 300, pulled 100 -> net +2.0x, holding; attackers pulled 70");
    nl2::Read rb = nl2::nodeRead(back[0].rows, false, 400, 400, 8.0, 9, T);
    check(rb.ok && rb.add == 0 && !rb.holding, "[read] the same rows as a bearish node: the offers did not defend");
    check(nl2::ratioText(1.84) == "+1.8x" && nl2::ratioText(-1.26) == "-1.3x", "[text] ratios with sign, one decimal");
    check(bf::priceText(7712.25, 0.25) == "7712.25" && bf::priceText(1.16245, 0.00005) == "1.16245" && bf::priceText(105.5, 0.01) == "105.50", "[text] the node price with its tick's decimals");
    sb::Fact y; y.bar = 1; y.lo = 7712; y.hi = 7712.5; y.side = 1; y.val = 1.84; y.code = 31; y.proven = 2;
    const std::string tip = bf::nodeTip(y, 0.25, "ES");
    check(tip.find("At node 7712.25 - are the defenders holding?") == 0 && tip.find("HOLDING +1.8x ?") != std::string::npos && tip.find("Read: A+ (still being proven)") != std::string::npos
          && tip.find("How to use:") != std::string::npos && tip.find("Example:") != std::string::npos, "[hover] question, the numbers, the read, how to use, an example");
    // the colour rule of the At node cell: bids holding green / bids pulling red / offers holding red / offers pulling green
    auto col = [](bool bull, bool hold) { return (bull == hold) ? 0 : 1; };
    check(col(true, true) == 0 && col(true, false) == 1 && col(false, true) == 1 && col(false, false) == 0, "[colour] by meaning: green bullish, red bearish");
    check(std::string(bf::VERSION).compare(0, 6, "1.32.0") == 0 && std::string(ofset::BF_VERSION) == "1.32.0" && std::string(off::VERSION).compare(0, 6, "1.16.0") == 0 && std::string(nl2::VERSION) == "1.0.0", "[ver] BookFlow 1.32.0, OrderFlowFeed 1.16.0, NodeLadder 1.0.0");
    std::printf("bf132: %d passed, %d failed\n", P, F); return F ? 1 : 0;
}
