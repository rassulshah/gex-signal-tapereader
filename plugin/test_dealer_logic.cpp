/********************************************************************************
 *  test_dealer_logic.cpp -- Gate B (logic) for lsDealerProfile + lsDealerRead (plugin/DealerLogic.h). No IRT SDK.
 *      cloud   : g++ -std=c++14 -o /tmp/dlt plugin/test_dealer_logic.cpp && /tmp/dlt
 *      windows : plugin\run-logic-tests.bat dealer
 *  Pins: the LRA-Dealer-<MKT>.csv grammar (the real 9/29 GC 08:45 file, LRA-Dealer-GC.sample.csv), the market from
 *  IRT's root symbol, the contract offset and its refusal, the profile scale, the stale clock.
 ********************************************************************************/
#include "DealerLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>

static int fails = 0, passes = 0;
#define CHECK(cond, msg) do { if (cond) { passes++; printf("  ok    %s\n", msg); } else { fails++; printf("  FAIL  %s   (line %d)\n", msg, __LINE__); } } while (0)

int main()
{
    printf("test_dealer_logic -- lsDealerProfile / lsDealerRead\n");
    std::ifstream f("LRA-Dealer-GC.sample.csv"); std::stringstream ss; ss << f.rdbuf();
    dl::Data D = dl::parseText(ss.str());
    CHECK(D.ver == "1.1" && D.market == "GC", "VERSION and MARKET");
    CHECK(D.asofSo == 34560 && D.y == 2026 && D.mo == 9 && D.d == 29, "ASOF 09:36 CT on 2026-09-29");
    CHECK(D.hasPrice && std::fabs(D.px - 4201.4f) < 0.01f && std::fabs(D.em - 55.46f) < 0.01f, "PRICE 4201.40, EM 55.46");
    CHECK(D.hasBook && D.book < 0, "BOOK short gamma");
    CHECK(D.nodes.size() == 28, "28 strikes within 1.25 EM");
    const dl::Node* n4215 = 0; for (size_t i = 0; i < D.nodes.size(); i++) if (D.nodes[i].k == 4215.0f) n4215 = &D.nodes[i];
    CHECK(n4215 && std::fabs(n4215->g - 35.0f) < 0.05f && n4215->lo < n4215->g && n4215->hi > n4215->g && n4215->d < 0 && !n4215->far, "4215: +35 on arrival, whisker around it, dealers SELL");
    CHECK(n4215 && std::fabs(n4215->gp - 2.8f) < 0.01f && std::fabs(n4215->dp - 5.0f) < 0.01f, "(1.1) its strength share: 2.8% / 5.0% of a normal 15 min");
    CHECK(dl::pctTxt(2.8f) == "2.8%" && dl::pctTxt(14.2f) == "14%" && dl::pctTxt(-1) == "", "share text");
    CHECK(dl::strengthBars(0.1f) == 0 && dl::strengthBars(0.5f) == 1 && dl::strengthBars(2.8f) == 2 && dl::strengthBars(5.0f) == 3, "(1.2) strength bars: quiet / small / meaningful / big");
    CHECK(n4215 && !n4215->live, "(1.2) 09:36: COVER not confirmed -> no live mark");
    { dl::Data L2; dl::parseLine(L2, "NODE|4215|35|32|44|20|-63|0|2.8|5.0|1"); CHECK(L2.nodes.size() == 1 && L2.nodes[0].live, "a NODE with the live flag"); }
    { bool okf = true; int nf = 0; for (size_t i = 0; i < D.nodes.size(); i++) { bool beyond = std::fabs(D.nodes[i].k - D.px) > 1.2f * D.em; if (beyond != D.nodes[i].far) okf = false; nf += D.nodes[i].far; }
      CHECK(okf && nf >= 1, "far = beyond 1.2 EM, exactly"); }
    CHECK(D.hasLevel && D.phase == 3 && D.side == 'R' && std::fabs(D.lvlPx - 4203.8f) < 0.01f, "LEVEL PDH 4203.80, RECLAIM (phase 3 of 6), resistance");
    CHECK(D.wallTitle.find("CR0 4,200") == 0 && D.wallCol == 'G' && D.wall.size() == 3, "wall checklist titled by the MenthorQ level, green, 3 rows");
    CHECK(D.wall[0].n == "SPEED" && D.wall[1].n == "COLOR" && D.wall[2].n == "ZOMMA" && D.wall[0].st == "ok" && D.wall[0].hasV, "SPEED / COLOR / ZOMMA in order, SPEED ok with a meter");
    CHECK(D.fuel.size() == 3 && D.fuel[0].n == "COVER" && D.fuel[1].n == "GAMMA" && D.fuel[2].n == "VANNA" && D.fuel[0].hasV, "fuel: COVER / GAMMA / VANNA, measured after the sweep");
    CHECK(D.trig.size() == 4 && D.trig[0].group == "SWEEP" && D.trig[0].name == "Trapped" && D.trig[0].on && D.trig[3].name == "RevWave" && D.trig[3].on && !D.trig[1].on && D.trig[3].note == "holds 88-95%", "(1.1) Trapped on the sweep, RevWave the trigger (9/29 09:21 high: 1,402 lots 2.1x; leg 1.9x)");
    CHECK(D.trade.size() >= 4 && D.trade[0].k == "ENTRY" && D.trade.back().k == "RECORD" && D.trade.back().col == 'Y', "TRADE line: ENTRY first after the reclaim, RECORD last (learning, yellow)");
    CHECK(D.flags.size() == 2 && D.flags[0].k == "NEWS none" && D.flags[0].col == 'G', "flags: NEWS none (green), DATA");
    CHECK(std::string(dl::phaseName(2)) == "TRIGGER" && std::string(dl::phaseName(5)) == "TRADE" && dl::NPHASE == 6, "six phases with TRIGGER third");
    CHECK(!D.doText.empty() && !D.bookLine.empty() && D.learn, "DO, BOOKLINE, LEARN");
    // text is ASCII only (IRT draws single-byte text)
    bool ascii = true; std::string all = ss.str(); for (size_t i = 0; i < all.size(); i++) if ((unsigned char)all[i] > 126) ascii = false;
    CHECK(ascii, "the whole file is ASCII");
    // grammar edges
    dl::Data E; CHECK(dl::parseLine(E, "LEVEL|NONE") && !E.hasLevel, "LEVEL|NONE -> no level");
    CHECK(!dl::parseLine(E, "NODE|4200|1|2") && E.nodes.empty(), "a short NODE row is refused");
    dl::Row r; std::vector<std::string> t = dl::split("WALLROW|SPEED|250|+250%|ok|x");
    CHECK(dl::parseRow(t, r) && r.v == 100, "a meter is clamped to +-100");
    CHECK(dl::split("A|b c|").size() == 3, "split keeps an empty last field");
    // market from the root symbol
    CHECK(dl::marketForRoot("EP") == "ES" && dl::marketForRoot("ENQ") == "NQ" && dl::marketForRoot("GCE") == "GC" && dl::marketForRoot("CLE") == "CL"
          && dl::marketForRoot("CPE") == "HG" && dl::marketForRoot("NGE") == "NG" && dl::marketForRoot("EU6") == "EU" && dl::marketForRoot("MES") == "ES", "IRT roots -> markets");
    CHECK(dl::marketForRoot("ZB") == "", "an unknown root -> no market");
    CHECK(dl::marketForRoot("QGC") == "GC" && dl::marketForRoot("QG") == "NG" && dl::marketForRoot("QES") == "ES" && dl::marketForRoot("QNQ") == "NQ", "(1.0.1) his CQG roots: QGC is GOLD, not NG");
    CHECK(dl::marketFor(4, "EP") == "GC" && dl::marketFor(0, "GCE") == "GC", "the Market setting overrides Auto");
    // offset
    float off = 0;
    CHECK(dl::offsetFor(4201.0f, 4192.5f, off) && std::fabs(off - 8.5f) < 0.01f, "contract offset = chart close at the file's minute - PRICE");
    CHECK(!dl::offsetFor(7600.0f, 4192.5f, off) && off == 0, "a wrong market (ES chart, GC file) is refused");
    // scale and fit
    float gm = 0, dm = 0; dl::scales(D, gm, dm);
    CHECK(gm >= 44.0f && dm >= 63.0f, "the scale covers the 4215 whisker and its delta");
    CHECK(dl::barLen(45.0f, 90.0f, 200) == 100 && dl::barLen(-500, 90, 200) == 200, "bar length is proportional and capped");
    CHECK(dl::fits(60, 80) && !dl::fits(60, 62), "text goes inside a node only when it fits");
    CHECK(dl::gammaLabel(*n4215) == "4215  35  (32 to 45)" && dl::gammaShort(*n4215) == "4215  35" && dl::deltaLabel(*n4215) == "SELL 63", "node labels");
    // stale
    CHECK(std::fabs(dl::staleMin(31500, 31800) - 5.0) < 1e-9 && dl::staleMin(86000, 100) > 0 && dl::staleMin(-1, 5) == 0, "stale minutes, across midnight");
    
    CHECK(dl::fmtPx(4203.8f, "GC") == "4,203.80" && dl::fmtPx(4200.0f, "GC", true) == "4,200" && dl::fmtPx(4.9512f, "HG") == "4.9512" && dl::fmtPx(27123.25f, "NQ") == "27,123.25", "prices with separators, decimals by market");
    std::vector<std::string> w = dl::wrap("Expect the sweep into a stiffening wall at 4,215. Short only after reclaim.", 30, [](const std::string& s) { return (int)s.size(); });
    CHECK(w.size() == 3 && w[0].size() <= 30 && w[2] == "Short only after reclaim.", "word wrap by measured width");
    printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
