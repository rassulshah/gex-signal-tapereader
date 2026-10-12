// test_dealerprofile_274.cpp -- DealerProfile 2.7.4 review fixes (SDK-free parts)
//   g++ -std=c++17 -Wall -Wextra -Werror -I. test_dealerprofile_274.cpp -o t && ./t
#include "DealerProfileAuditLogic.h"
#include <cstdio>
#include <string>
#include <vector>

static int f = 0, p = 0;
#define CHECK(c, m) do { if (c) ++p; else { ++f; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)

int main()
{
    // ---- badges "<price> <NAME>  <odds>" (Rassul 2026-10-12: key-level badges had no price); price format = the MenthorQ badge's
    CHECK(dpaudit::badge("ES", 6850.25, "PDH", "33%  /  50%").all() == "6850.25 PDH  33%  /  50%", "ES key level: 2 decimals, price first");
    CHECK(dpaudit::badge("ES", 7850.0, "CR 0D / GW 0D", "").all() == "7850.00 CR 0D / GW 0D", "ES MenthorQ badge unchanged (7850.00), no odds = no trailing gap");
    CHECK(dpaudit::badge("GC", 4221.7, "PDH", "2%  /  4%?").all() == "4221.7 PDH  2%  /  4%?", "GC key level: 1 decimal (0.1 tick), as Rassul's example");
    CHECK(dpaudit::badge("GC", 4225.0, "G1", "1%  /  2%?").all() == "4225.0 G1  1%  /  2%?", "GC MenthorQ badge: 4225.0 G1 (as on his chart)");
    CHECK(dpaudit::badge("NQ", 31000.5, "LonHI", "7%  /  12%?").all() == "31000.50 LonHI  7%  /  12%?", "NQ LonHI with its price");
    CHECK(dpaudit::priceText("HG", 6.7805) == "6.7805" && dpaudit::priceText("EU", 1.13255) == "1.13255" && dpaudit::priceText("NG", 3.351) == "3.351"
          && dpaudit::priceText("CL", 94.07) == "94.07", "HG 4 / EU 5 / NG 3 / CL 2 decimals (the MenthorQ badge's table)");
    { dpaudit::Badge b = dpaudit::badge("ES", 6850.25, "PDH/HrHI", "1h 45%?");
      CHECK(b.price == "6850.25 " && b.name == "PDH/HrHI" && b.odds == "  1h 45%?", "segments: price, bold name, odds"); }

    // ---- MenthorQ colours: green = support, red = resistance, 2 others
    CHECK(dpaudit::mqKind("CR 0D / GW 0D") == 'R' && dpaudit::mqKind("PS 0D / HVL 0D") == 'G' && dpaudit::mqKind("HVL 0D") == 'Y'
          && dpaudit::mqKind("G1") == 'C' && dpaudit::mqKind("GW 0D") == 'C' && dpaudit::mqKind("MQ CR") == 'R', "CR red, PS green, HVL yellow, others cyan");
    CHECK(dpaudit::stripMQ("MQ CR 0D") == "CR 0D" && dpaudit::stripMQ("CR 0D / MQ GW") == "CR 0D / GW" && dpaudit::stripMQ("G1") == "G1"
          && dpaudit::stripMQ("SMQ X") == "SMQ X", "no \"MQ\" in level names");
    CHECK(dpaudit::liftForText(0x000000FF) != 0x000000FF && (dpaudit::liftForText(0x000000FF) & 0xFF) == 0xFF, "pure blue PW text lifted (same hue) to read on black");
    CHECK(dpaudit::liftForText(0x00FFA500) == 0x00FFA500, "orange PD text unchanged");

    // ---- contracts: Rithmic names, the euro root, rolls
    CHECK(dl::sameContract("ESZ6", "EPZ26") && dl::sameContract("NQZ6", "ENQZ26") && dl::sameContract("6EZ6", "EU6Z26"), "Rithmic ESZ6 = CQG EPZ26 (same month)");
    CHECK(!dl::sameContract("ESH7", "EPZ26"), "Rithmic ESH7 in the roll is ANOTHER contract (2.7.3 read it as continuous: never shifted)");
    CHECK(!dl::hasMonthCode("EU6") && !dl::hasMonthCode("QHG") && !dl::hasMonthCode("CPE") && !dl::hasMonthCode("ENQ") && dl::hasMonthCode("ESZ6"),
          "roots EU6 / QHG / CPE / ENQ are not dated; ESZ6 is");
    CHECK(dl::sameContract("EU6", "EU6Z26") && dl::sameContract("QHG", "CPEZ26") && dl::sameContract("NQ", "ENQZ26"), "continuous roots (EU6, QHG, NQ) = the front contract");

    std::vector<std::string> K = { "CLEX26", "CPEZ26", "ENQZ26", "EPH27", "EPZ26", "EU6Z26", "GCEZ26", "NGEX26" };
    bool sh = true;
    CHECK(dpaudit::pickMQSymbol(K, "EPZ26", "ES", "EPZ26", sh) == "EPZ26" && !sh, "own symbol, drawn at the strike");
    CHECK(dpaudit::pickMQSymbol(K, "ESZ6", "ES", "EPZ26", sh) == "EPZ26" && !sh, "Rithmic ESZ6 -> EPZ26, no shift");
    CHECK(dpaudit::pickMQSymbol(K, "ES", "ES", "", sh) == "EPZ26" && !sh, "continuous ES with two months listed -> the FRONT (Z26), not H27 by alphabet");
    CHECK(dpaudit::pickMQSymbol(K, "ESM7", "ES", "EPZ26", sh) == "EPZ26" && sh, "a chart on another month -> the dealer file's contract, shifted by the offset");
    CHECK(dpaudit::pickMQSymbol(K, "QHG", "HG", "CPEZ26", sh) == "CPEZ26" && !sh, "copper QHG -> CPEZ26");
    CHECK(dpaudit::pickMQSymbol(K, "EU6", "EU", "EU6Z26", sh) == "EU6Z26" && !sh, "euro EU6 -> EU6Z26, not shifted");
    CHECK(dpaudit::pickMQSymbol(K, "QGC", "GC", "", sh) == "GCEZ26" && !sh, "gold QGC -> GCEZ26");
    CHECK(dpaudit::pickMQSymbol(K, "ZB", "", "", sh) == "", "unknown market: no levels");

    // ---- the status file gate: not on every repaint
    dpaudit::WriteGate g;
    CHECK(g.due("A", 1000) && !g.due("A", 1001) && !g.due("A", 1059) && g.due("A", 1060) && g.due("B", 1061), "write on change or once a minute only");

    {   // (2.8.0) liquidity on the level badges
        std::vector<dpaudit::LiqZ> Z; std::string why;
        const std::string txt = "LIQZ,1,1000,0.25\nZ,S,7847.5,7850,2.59151,977\nZ,R,7857.25,7859.75,2.44,1152\nZ,S,7865.5,7868,1.67,628\nZ,R,bad,1,2,3\n";
        CHECK(dpaudit::parseLiqZones(txt, 1005, Z, why) && Z.size() == 3, "3 good zones parsed, the bad row skipped");
        CHECK(!dpaudit::parseLiqZones(txt, 1021, Z, why) && why == "stale" && Z.empty(), "a file 21 s old is ignored (the Liquidity Profile stopped)");
        CHECK(!dpaudit::parseLiqZones("garbage", 1000, Z, why), "no header -> nothing");
        dpaudit::parseLiqZones(txt, 1000, Z, why);
        const dpaudit::LiqZ* cr = dpaudit::liqAt(Z, 7850.00, false);
        CHECK(cr && cr->bid && cr->n == 977 && dpaudit::liqTag(*cr) == "  Liq 977 (2.6x)", "CR 7,850 on the 7,847.50-7,850 bid zone: Liq 977 (2.6x)");
        CHECK(dpaudit::liqAt(Z, 7851.25, false) != nullptr && dpaudit::liqAt(Z, 7851.50, false) == nullptr, "covers the zone +/- half its width (1.25), not beyond");
        const dpaudit::LiqZ* onl = dpaudit::liqAt(Z, 7857.75, false);
        CHECK(onl && !onl->bid && onl->n == 1152, "ONL 7,857.75 inside the offer zone: red, 1152");
        CHECK(dpaudit::liqAt(Z, 7866.00, false) == nullptr && dpaudit::liqAt(Z, 7866.00, true) == nullptr, "1.67x: no tag (below the 1.7x line)");
        std::vector<dpaudit::LiqZ> Z2; dpaudit::parseLiqZones("LIQZ,1,1000,0.25\nZ,S,7847.5,7850,1.8,700\n", 1000, Z2, why);
        CHECK(dpaudit::liqAt(Z2, 7850, false) != nullptr, "(2.8.3) 1.8x: a zone the Liquidity Profile still draws (kept to 1.7x) is tagged - badge and column agree");
    }
    {   // (2.8.0) hourly charts take the 3-min chart's key levels
        std::vector<dpaudit::KeyLv> v = { { "PDH", 7870.5, false }, { "ONL/LonLO", 7857.75, false }, { "HrHI", 7880.25, true } };
        std::vector<dpaudit::KeyLv> r;
        CHECK(dpaudit::parseKeyLevels(dpaudit::keyLevelsText(v, 50000), 50100, r) && r.size() == 3 && r[1].name == "ONL/LonLO" && r[1].price == 7857.75 && r[2].pivot, "round trip: names, prices, pivots");
        CHECK(!dpaudit::parseKeyLevels(dpaudit::keyLevelsText(v, 50000), 50000 + 19 * 3600, r) && r.empty(), "a file 19 h old (yesterday's) is not used");
        CHECK(!dpaudit::parseKeyLevels("junk\nL|PDH|1|0\n", 50000, r), "no header -> nothing");
        CHECK(dpaudit::barSeconds({ 0, 3600, 7200, 10800 }) == 3600 && dpaudit::barSeconds({ 0, 180, 360, 3600 }) == 180 && dpaudit::barSeconds({ 5 }) == 0, "bar size from the stamps (hourly 3600 > 900: levels only; 3-min 180: full profile)");
    }
    {   // (2.8.1) sticky badges: the old spot is kept while free, given up only when something covers it
        std::vector<dpaudit::LblBox> T = { dpaudit::LblBox{ 300, 100, 400, 116 } };
        CHECK(dpaudit::spotFree(150, 100, 100, 116, T, 110, 900), "old spot left of another badge, 3 px clear: kept");
        CHECK(!dpaudit::spotFree(250, 100, 100, 116, T, 110, 900), "a badge now on the old spot: it moves");
        CHECK(dpaudit::spotFree(250, 100, 120, 136, T, 110, 900), "another row: kept");
        CHECK(!dpaudit::spotFree(100, 100, 100, 116, {}, 110, 900) && !dpaudit::spotFree(850, 100, 100, 116, {}, 110, 900), "out of bounds (the profiles / the Liquidity column): moves");
        std::vector<dpaudit::LblBox> B = { dpaudit::LblBox{ 160, 90, 170, 130, true } };
        CHECK(!dpaudit::spotFree(150, 100, 100, 116, B, 110, 900), "a price bar grew into it: it moves (once, then stays)");
    }
    {   // (2.8.5) 13:27 ES: ONL 7857.75 sat inside the 954 zone (7855.75-7857.75) but the badge showed the 854 zone (7858-7860, 1.9x)
        std::vector<dpaudit::LiqZ> Z; std::string why;
        dpaudit::parseLiqZones("LIQZ,1,1000,0.25\nZ,R,7858,7860,1.9,854\nZ,R,7855.75,7857.75,1.8,954\n", 1000, Z, why);
        const dpaudit::LiqZ* onl = dpaudit::liqAt(Z, 7857.75, false); const dpaudit::LiqZ* lon = dpaudit::liqAt(Z, 7860.0, false);
        CHECK(onl && onl->n == 954 && lon && lon->n == 854, "ONL -> the 954 zone holding it, LonLO -> the 854 zone (as the Liquidity column names them)");
    }
    std::printf("%d passed, %d failed\n", p, f);
    return f != 0;
}
