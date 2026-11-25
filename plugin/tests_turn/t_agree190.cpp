// t_agree190.cpp -- lsTurn 1.9.0: the Price line (layer 6) and the Big Lot tier on the read's biggest orders (layer 4)
#include "../TurnLogic.h"
#include <cstdio>
static int checks = 0, fails = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    using namespace turn;
    auto N = [](long long hi, long long lo, const char* w) { Nx n; n.hi = hi; n.lo = lo; n.when = w; return n; };
    // GC 9:30 AM initiative low 35277, ATR 83 ticks (8.3): 9:33 high 35449 >= 35277 + 83 -> agreed by 9:33
    std::vector<Nx> a = { N(35449, 35354, "9:33 AM") };
    CHECK(agreeText(1, true, 35277, a, 83, 0.1) == "agreed - went 1 ATR (8.3) up by 9:33 AM.");
    // through the low by more than 1 tick first -> failed (a candle that does both: failed for a hold)
    std::vector<Nx> b = { N(35300, 35290, "9:33 AM"), N(35400, 35275, "9:36 AM") };
    CHECK(agreeText(1, true, 35277, b, 83, 0.1) == "failed - traded through 3527.7 at 9:36 AM.");
    std::vector<Nx> b1 = { N(35300, 35276, "9:33 AM") }; CHECK(agreeText(1, true, 35277, b1, 83, 0.1) == "held 1 candle so far.");   // 1 tick through = still held
    std::vector<Nx> b2 = { N(35400, 35270, "9:33 AM") }; CHECK(agreeText(1, true, 35277, b2, 83, 0.1).compare(0, 6, "failed") == 0);
    // a high that should hold (mirror)
    std::vector<Nx> c = { N(35640, 35500, "9:45 AM") }; CHECK(agreeText(-1, true, 35642, c, 83, 0.1) == "agreed - went 1 ATR (8.3) down by 9:45 AM.");
    // aggressor at the bottom: price should go ON down
    std::vector<Nx> d = { N(35500, 35400, "9:21 AM") }; CHECK(agreeText(1, false, 35484, d, 83, 0.1) == "agreed - went 1 ATR (8.3) down by 9:21 AM.");
    std::vector<Nx> e = { N(35570, 35480, "9:21 AM") }; CHECK(agreeText(1, false, 35484, e, 83, 0.1) == "failed - came back 1 ATR by 9:21 AM.");
    std::vector<Nx> f2 = { N(35500, 35480, "9:21 AM"), N(35500, 35480, "9:24 AM") }; CHECK(agreeText(1, false, 35484, f2, 83, 0.1) == "no follow-through yet (2 candles).");
    // waiting / held / 10 candles / no ATR
    CHECK(agreeText(1, true, 35277, std::vector<Nx>(), 83, 0.1) == "waiting for the next candle.");
    std::vector<Nx> h(3, N(35300, 35280, "x")); CHECK(agreeText(1, true, 35277, h, 83, 0.1) == "held 3 candles so far.");
    std::vector<Nx> t(12, N(35300, 35280, "x")); CHECK(agreeText(1, true, 35277, t, 83, 0.1) == "neither 1 ATR nor through it in 10 candles.");
    CHECK(agreeText(1, true, 35277, a, 0, 0.1) == "held 1 candle so far.");
    // ATR(13): constant 10-tick candles = 10; under 14 candles = 0
    std::vector<std::vector<long long> > k; for (int i = 0; i < 20; ++i) k.push_back(std::vector<long long>{ 110, 100, 105 });
    CHECK(atrTicks(k) == 10); k.resize(13); CHECK(atrTicks(k) == 0);
    // the tier on the read's biggest orders
    IniCalc ic; ic.feed = true; ic.burst1000 = 6500; ic.sweep = 4; ic.bigLots = 41; Ini in; in.x1000 = 6800; in.lots = 135; in.att = 24; in.away = true; in.tick = 35299; in.attX1000 = 1100;
    CHECK(iniText(1, in, ic, "", 0.1, "GC", true).rows[1].second.find("Biggest 41 (S20)") != std::string::npos);
    CHECK(iniText(1, in, ic, "", 0.1, "GC", false).rows[1].second.find("Biggest 41 (S20)") != std::string::npos);   // ON S20 = 39
    ic.bigLots = 11; CHECK(iniText(1, in, ic, "", 0.1, "GC", true).rows[1].second.find("Biggest 11") != std::string::npos && iniText(1, in, ic, "", 0.1, "GC", true).rows[1].second.find("(S") == std::string::npos);
    AbsCalc ac; ac.feed = true; ac.attQ = 263; ac.big.lots = 60; ac.big.lo = 35401; ac.big.hi = 35407;
    CHECK(compactText(1, 3100, true, ac, "", "", 0, 0.1, "GC", true).rows[0].second.find("biggest print 60 (S5) (swept 7 ticks)") != std::string::npos);
    CHECK(compactText(1, 3100, true, ac, "", "", 0, 0.1).rows[0].second.find("biggest print 60 (swept") != std::string::npos);   // no market: no tier
    std::printf("t_agree190: %d checks, %d fails\n", checks, fails); return fails ? 1 : 0; }
