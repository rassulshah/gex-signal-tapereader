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
    {   // (Profile 2.0) net GEX per strike, as MenthorQ shows it
        dl::Data L3; dl::parseLine(L3, "NODE|4150|-29|-29|-29|-53|-724|0|1.4|34|0|-3945425|-1822449");
        CHECK(L3.nodes.size() == 1 && L3.nodes[0].hasUsd && std::fabs(L3.nodes[0].usd + 3945425.0f) < 1.0f, "NODE carries net GEX $");
        CHECK(dl::usdLabel(L3.nodes[0].usd) == "-3.9M", "-3,945,425 -> -3.9M");
        CHECK(dl::usdLabel(420495.0f) == "420K" && dl::usdLabel(7894.0f) == "8K" && dl::usdLabel(-950.0f) == "-950", "K / plain labels");
        CHECK(dl::usdLabel(-30696300000.0f) == "-30.7B" && dl::usdLabel(-984600000.0f) == "-984.6M", "B for billions (EU), M stays (NG)");
        CHECK(std::fabs(dl::odtePart(L3.nodes[0]) - 1822449.0f) < 1.0f, "0DTE part of a short strike");
        dl::Node m; m.hasUsd = true; m.usd = 7894; m.usd0 = -223276; CHECK(dl::odtePart(m) == 0, "0DTE of the other sign is not drawn inside");
        dl::Data L4; dl::parseLine(L4, "NODE|4215|35|32|44|20|-63|0|2.8|5.0|1"); CHECK(!dl::anyUsd(L4), "an old file has no net GEX: classic drawing");
    }
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
    {   // (Read 1.2.4) the price level then the MenthorQ level; NO WALL; the FUEL title
        dl::Data E;
        dl::parseLine(E, "NAMES|ONL 4,198|PS 0D / HVL 0D 4,200|Support");
        dl::parseLine(E, "WALLHDR|No Wall||N");
        dl::parseLine(E, "NOWALL|4,200 is FUEL (-13 per 5.8 pts) - it holds only if dealers cover (see FUEL)");
        dl::parseLine(E, "FUELHDR|FUEL for ONL 4,198 + PS 0D / HVL 0D 4,200 Support|0/3 AT RISK|R");
        CHECK(E.keyName == "ONL 4,198" && E.mqName == "PS 0D / HVL 0D 4,200" && E.sideWord == "Support" && E.wall.empty()
              && E.wallTitle == "No Wall" && E.noWall.find("FUEL (-13") != std::string::npos
              && E.fuelTitle == "FUEL for ONL 4,198 + PS 0D / HVL 0D 4,200 Support" && E.fuelScore == "0/3 AT RISK", "names / no wall / fuel title");
    }
    {   // (Read 1.2.6) TAPE / CHARM rows and the Analyst
        dl::Data A;
        dl::parseLine(A, "XROW|TAPE|-11.7|1.0x vol|wait|last 15 min: no side in control, volume 1.0x normal");
        dl::parseLine(A, "ANOW|Gold is at 4,191, 7 pts under ONL 4,198.|11:18");
        dl::parseLine(A, "ABIAS|SHORT the rally into 4,198|R");
        dl::parseLine(A, "ASCN|A REJECTION at 4,198|SHORT|most likely|R|a poke into 4,198|go short when it turns|options at 4,200: 3.4k puts");
        dl::parseLine(A, "ASCN|B FLUSH to 4,180|WAIT > LONG|if 4,186 breaks|L|l1|l2");
        CHECK(A.xrows.size() == 1 && A.xrows[0].n == "TAPE" && A.aTime == "11:18" && A.aBias == "SHORT the rally into 4,198" && A.aBiasCol == 'R'
              && A.scn.size() == 2 && A.scn[0].l3 == "options at 4,200: 3.4k puts" && A.scn[1].l3.empty() && A.scn[1].col == 'L', "tape + analyst rows");
    }
    {   // (Read 1.5) the level verdict
        dl::Data V;
        dl::parseLine(V, "VERDICT|WEAK|0|2|no floor: a pocket 4,165-4,176 under the level - a sweep can run through your stop");
        CHECK(V.vLabel == "WEAK" && V.vPts == 0 && V.vOf == 2 && V.vText.find("pocket") != std::string::npos, "verdict row");
        dl::Data W2; CHECK(W2.vPts == -1 && W2.vLabel.empty(), "no verdict = none drawn");
    }
    {   // (Read 1.6.0) the decision layer: verdict tally, votes, dealer lines, BOOK flag, plan, summary (lra/verdict.py)
        dl::Data V;
        const char* rows[] = { "VERDICT2|SUPPORTED|SHORT|4|5|1|0", "VOTE|RE-SWEEP|ok|14%", "VOTE|DEALERS|ok|2 of 3", "DLINE|DELTA|ok|sell 310 0.60%",
            "DLINE|GAMMA|ok|wall 1.40%", "DLINE|NET|neutral|+2%", "VOTE|TAPE|ok|sellers 1.4x", "VOTE|MAGNET|neutral|none", "VOTE|RANGE|ok|38% ADR",
            "VOTE|WALL LIFE|wait|", "BOOKCHK|MATCH|78", "PLAN|TRIGGER|wait|wait CoB / RBar", "PLAN|STOP|ok|7,766.25 est",
            "PLAN|TARGET|ok|7,733.50 wall 3.2R est", "SUMMARY|ES: supported short (4 of 5 votes), 1 still waiting." };
        for (const char* r : rows) dl::parseLine(V, r);
        CHECK(V.hasV2 && V.v2Word == "SUPPORTED" && V.v2Side == "SHORT" && V.v2Sup == 4 && V.v2Meas == 5 && V.v2Wait == 1 && V.v2Opp == 0, "VERDICT2 tally");
        CHECK(V.votes.size() == 6 && V.votes[1].n == "DEALERS" && V.votes[5].q == "wait" && V.votes[5].v.empty(), "six votes, an empty value kept");
        CHECK(V.dlines.size() == 3 && V.dlines[2].n == "NET" && V.dlines[2].q == "neutral", "the dealer lines under DEALERS");
        CHECK(V.bookState == "MATCH" && V.bookPct == "78", "BOOK trust flag");
        CHECK(V.plan.size() == 3 && V.plan[1].v == "7,766.25 est" && V.plan[2].q == "ok", "plan: trigger, stop est, target");
        CHECK(V.summary.find("supported short") != std::string::npos, "summary text");
        dl::Data O; CHECK(!O.hasV2 && O.votes.empty() && O.summary.empty(), "an older file = no decision rows (the old layout is drawn)");
    }
    {   // (file 1.6 / Read 2.0 / Sig 1.0) forced futures, gamma strength, reasons (lra/forced.py, GC 1 Oct)
        dl::Data F;
        const char* rows[] = { "FSUM|10:15|10:40|589|-10|14|0|6871|4185.90|4202.10|27.32|27.49", "FWIN|10:15|10:20|196|-4|3|0|1549",
            "FWIN|10:20|10:30|204|-6|6|0|1340", "GSTR|10:15|4185.90|27.32|-40.9|-50.5|-31.6|-38.8|-43.1|-40.8|-40.6",
            "SIG|2026-10-01 10:20:00|SC|B|196|dealers bought 196 back as price moved (12.6% of futures)",
            "SIG|2026-10-01 10:50:00|SC|S|-168|dealers sold 168 back", "SIG|bad|SC|B|1|x", "FERR|KeyError: x" };
        for (const char* r : rows) dl::parseLine(F, r);
        CHECK(F.hasFsum && F.fsum.ta == "10:15" && F.fsum.gam == 589 && F.fsum.van == -10 && F.fsum.fut == 6871 && F.fsum.pb == "4202.10" && std::fabs(F.fsum.ivb - 27.49f) < 1e-3, "FSUM");
        CHECK(F.fwin.size() == 2 && F.fwin[1].gam == 204 && F.fwin[1].fut == 1340, "FWIN rows");
        CHECK(F.hasGstr && std::fabs(F.gstr.g + 40.9f) < 1e-3 && std::fabs(F.gstr.sup + 31.6f) < 1e-3 && F.gstr.t == "10:15", "GSTR");
        CHECK(F.sigs.size() == 2 && F.sigs[0].code == "SC" && F.sigs[0].side == 'B' && F.sigs[0].h == 10 && F.sigs[0].mi == 20 && F.sigs[1].side == 'S', "SIG rows, a bad time dropped");
        CHECK(F.ferr == "KeyError: x", "FERR");
        CHECK(dl::signedN(589.4f) == "+589" && dl::signedN(-10.2f) == "-10" && dl::signedN(0.2f) == "0", "signed numbers");
        CHECK(dl::shareTxt(593, 6871) == "8.6%" && dl::shareTxt(1, 0).empty(), "share of futures");
        CHECK(dl::gChange(-40.9f, -31.6f) == "-23%" && dl::gChange(-40.9f, -50.5f) == "+23%", "gamma change in %");
        CHECK(dl::sigInBar(F.sigs[0], 2026, 10, 1, 10 * 3600 + 18 * 60, 10 * 3600 + 21 * 60) && !dl::sigInBar(F.sigs[0], 2026, 10, 1, 10 * 3600 + 21 * 60, 10 * 3600 + 24 * 60), "a reason lands in the bar that contains its time");
    }
    {   // (file 1.7 / Read 2.1) the stages
        dl::Data G;
        const char* rows[] = { "STAGE|3|L|4,178.40|4,179.60|10:09", "STG|1|1 APPROACH|09:39 -> 10:09 - what dealers will be forced to do",
            "SR|1|10:15|Fuel|S|-576|Fuel:A|Dealers SOLD 576 futures", "SSUM|1|AT THE LEVEL: short gamma",
            "STG|2|2 TURN STARTS|extreme 4,179.60", "SR|2|10:20|PT?|B|~25|Guess:P,Small:N|Profit taking (guess)", "SR|2||GLD|N||No data:N|NO DATA" };
        for (const char* r : rows) dl::parseLine(G, r);
        CHECK(G.hasStage && G.stageNow == 3 && G.stageSide == 'L' && G.stageExt == "4,179.60" && G.stageExtT == "10:09", "STAGE row");
        CHECK(G.stgs.size() == 2 && G.stgs[0].sum == "AT THE LEVEL: short gamma" && G.stgs[1].sum.empty(), "STG + SSUM");
        CHECK(G.srows.size() == 3 && G.srows[1].code == "PT?" && G.srows[1].chips.size() == 2 && G.srows[1].chips[0].name == "Guess" && G.srows[1].chips[0].col == 'P', "SR rows + chips");
        CHECK(G.srows[2].t.empty() && G.srows[2].val.empty() && G.srows[2].side == 'N', "an SR row with empty fields");
        CHECK(dl::isGuess("PT?") && dl::isGuess("CallB?") && !dl::isGuess("SC") && !dl::isGuess(""), "guess codes");
    }
    {   // (file 2.0) SRC row
        dl::Data Q; dl::parseLine(Q, "SRC|SPX|1.00471|T1 09:00 CT|09:55");
        CHECK(Q.srcBook == "SPX" && std::fabs(Q.srcRatio - 1.00471f) < 1e-5f && Q.srcMode == "T1 09:00 CT" && Q.srcAt == "09:55", "SRC row: the index book and its ratio");
    }
    {   // (Dealer Summary 1.0) LRA-Summary-<MKT>.txt
        dl::Summary S = dl::parseSummary("VERSION|1.0\nASOF|14:09:52|2026-10-02|1791054592\nHEAD|HG FAILED BREAKOUT - PRIOR-DAY HIGH|#f87171\r\nBODY|HG swept PDH 6.5970.\nMETA|PDH|6.5970|now|RECLAIMED|OPPOSES|1|0\n");
        CHECK(S.ok && S.head == "HG FAILED BREAKOUT - PRIOR-DAY HIGH" && S.col == 0x00F87171, "summary HEAD + colour");
        CHECK(S.body == "HG swept PDH 6.5970." && S.asof == "14:09:52" && S.day == "2026-10-02" && S.epoch == 1791054592LL, "summary BODY + ASOF");
        CHECK(S.key == "PDH" && S.state == "RECLAIMED" && S.verdict == "OPPOSES" && S.trusted && S.gridAge == 0, "summary META");
        CHECK(!dl::parseSummary("VERSION|1.0\n").ok && dl::hexColour("#zz0000", 7) == 7 && dl::hexColour("#4ade80", 0) == 0x004ADE80, "summary empty + bad colour");
        CHECK(dl::ageMin(1000, 1000 + 125) == 2 && dl::ageMin(0, 5) == -1 && dl::ageTxt(5) == "5 min" && dl::ageTxt(130) == "2h" && dl::ageTxt(3000) == "2d", "summary age");
    }
    printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
