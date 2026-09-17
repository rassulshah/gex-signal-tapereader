/********************************************************************************
 *  test_kingtracker_logic.cpp — Gate B (logic) for lsKingTracker (plugin/KingTrackerLogic.h). No IRT SDK.
 *      cloud   : g++ -std=c++14 -o /tmp/ktl plugin/test_kingtracker_logic.cpp && /tmp/ktl
 *      windows : plugin\run-logic-tests.bat
 *  Pins: the KINGTRACK / KINGNOW grammar, the v0.6 re-derivation across Skylit's mid-session roll (2026-09-16: the
 *  SPX 7685 King was 7688 at 09:47 and 7757 after ES1 rolled to Dec — no phantom step), the anchor and its clamp,
 *  the stale wrap. The anchor BAR is test_contractoffset_logic.cpp (shared with lsGammaProfile).
 ********************************************************************************/
#include "KingTrackerLogic.h"
#include <cstdio>
#include <sstream>
#include <cmath>

static int fails = 0, passes = 0;
#define CHECK(cond, msg) do { if (cond) { passes++; printf("  ok    %s\n", msg); } else { fails++; printf("  FAIL  %s   (line %d)\n", msg, __LINE__); } } while (0)
static std::vector<std::string> split(const std::string& line) { std::vector<std::string> t; std::stringstream ss(line); std::string it; while (std::getline(ss, it, ',')) t.push_back(it); return t; }

int main()
{
    printf("test_kingtracker_logic — lsKingTracker's decisions\n");
    // the SPX book on 2026-09-16: 7500 at the open (old scale 7504.19), 7600 at 09:19 (7604.25), 7685 at 09:47 (7688.37),
    // then Skylit's ES1 rolled to Dec: KINGNOW says 7685 -> 7757.30 (ratio 1.0094)
    ktl::Book B; std::string fam, bk; ktl::Step s;
    CHECK(ktl::parseTrack(split("KINGTRACK,ES,SPX,2,7504.19,7500"), fam, bk, s) && fam == "ES" && bk == "SPX" && s.so == 2 && s.strike == 7500 && std::fabs(s.px - 7504.19f) < 0.01f, "KINGTRACK,<fam>,<book>,<so>,<px>,<strike> parses");
    B.steps.push_back(s);
    ktl::parseTrack(split("KINGTRACK,ES,SPX,4746,7604.25,7600"), fam, bk, s); B.steps.push_back(s);
    ktl::parseTrack(split("KINGTRACK,ES,SPX,33608,7688.37,7685"), fam, bk, s); B.steps.push_back(s);
    CHECK(!ktl::parseTrack(split("KINGTRACK,ES,SPX,2,7504.19"), fam, bk, s), "a 5-field KINGTRACK is refused");
    ktl::Book N; CHECK(ktl::parseNow(split("KINGNOW,ES,SPX,7757.30,7685,100"), fam, bk, N) && N.hasNow && N.nowStrike == 7685 && N.nowPct == 100 && std::fabs(N.nowPx - 7757.30f) < 0.01f, "KINGNOW parses with the pct");
    ktl::Book N4; CHECK(ktl::parseNow(split("KINGNOW,ES,SPX,7757.30,7685"), fam, bk, N4) && N4.nowPct == 0, "KINGNOW without a pct -> 0");
    B.hasNow = true; B.nowPx = N.nowPx; B.nowStrike = N.nowStrike; B.nowPct = N.nowPct;
    // ---- the re-derivation (v0.6)
    ktl::rederive(B);
    float r = 7757.30f / 7685.0f;
    CHECK(std::fabs(B.steps[0].px - 7500.0f * r) < 0.01f && std::fabs(B.steps[1].px - 7600.0f * r) < 0.01f && std::fabs(B.steps[2].px - 7757.30f) < 0.05f, "every step re-derived as strike x (nowPx / nowStrike): 7500 -> 7570.6, 7600 -> 7671.5, 7685 -> 7757.3 — no phantom step at the roll");
    CHECK(std::fabs(B.steps[2].px - B.steps[1].px - 85.0f * r) < 0.05f, "the 7600 -> 7685 step is 85 strikes in the current scale, not 84 + a 69-pt calendar spread");
    ktl::Book C = B; C.hasNow = false; C.steps[0].px = 7504.19f; ktl::rederive(C);
    CHECK(std::fabs(C.steps[0].px - 7504.19f) < 0.01f, "without KINGNOW the stored prices stand");
    ktl::Book D = B; D.steps[0].strike = 0; D.steps[0].px = 1.0f; ktl::rederive(D);
    CHECK(D.steps[0].px == 1.0f, "a step without a strike is left alone");
    // ---- the anchor and the offset
    float anchor = 0, off = 0;
    CHECK(ktl::anchorPrice(true, 7622.0f, true, 7615.5f, anchor) && anchor == 7622.0f, "SCALEREF wins over SPOT");
    CHECK(ktl::anchorPrice(false, 0, true, 7615.5f, anchor) && anchor == 7615.5f, "SPOT when there is no SCALEREF");
    CHECK(!ktl::anchorPrice(false, 0, false, 0, anchor), "neither -> nothing to anchor");
    CHECK(ktl::offsetFor(7625.0f, 7622.0f, off) && std::fabs(off - 3.0f) < 0.01f, "offset = anchor-bar close - SCALEREF");
    CHECK(!ktl::offsetFor(7622.0f, 7622.0f, off), "already aligned -> no shift");
    CHECK(!ktl::offsetFor(29000.0f, 7622.0f, off), "the NQ books on an NQ chart against an ES SCALEREF: refused (> 300)");
    ktl::shift(B, 3.0f);
    CHECK(std::fabs(B.nowPx - 7760.30f) < 0.01f && std::fabs(B.steps[2].px - 7760.30f) < 0.05f, "the shift moves every step and KINGNOW together");
    // ---- (v0.11) the Source switch: Skylit's tape books or IF's Magnet books, one or the other, this chart's family only
    CHECK(ktl::bookDrawn("SPX", "ES", "ES", false) && ktl::bookDrawn("SPY", "ES", "ES", false) && !ktl::bookDrawn("IF", "ES", "ES", false), "Source Skylit on ES: SPX and SPY drawn, IF not");
    CHECK(ktl::bookDrawn("IF", "ES", "ES", true) && !ktl::bookDrawn("SPX", "ES", "ES", true) && !ktl::bookDrawn("SPY", "ES", "ES", true), "Source IF on ES: only the IF Magnet — the SPX and SPY tape Kings hide (no IF SPY book exists)");
    CHECK(ktl::bookDrawn("IFQ", "NQ", "NQ", true) && !ktl::bookDrawn("IF", "ES", "NQ", true) && !ktl::bookDrawn("QQQ", "NQ", "NQ", true), "Source IF on NQ: IFQ only");
    CHECK(!ktl::bookDrawn("QQQ", "NQ", "ES", false) && !ktl::bookDrawn("IFQ", "NQ", "ES", true), "a book of the other family is never drawn, whichever source");
    // the IF rows parse with the same grammar
    ktl::Book IFB; std::string f2, b2;
    CHECK(ktl::parseNow(split("KINGNOW,ES,IF,7757.30,7685,-100"), f2, b2, IFB) && b2 == "IF" && IFB.nowStrike == 7685 && IFB.nowPct == -100, "KINGNOW,ES,IF parses like any book (a negative Magnet carries -100)");
    // ---- (v0.13) the polarity colour of the IF Magnet, and the 7-field row
    ktl::Step s7; std::string f7, b7;
    CHECK(ktl::parseTrack(split("KINGTRACK,ES,IF,33608,7688.37,7685,-100"), f7, b7, s7) && s7.pct == -100, "(16.39) KINGTRACK's 7th field is the step's polarity");
    CHECK(ktl::parseTrack(split("KINGTRACK,ES,IF,33608,7688.37,7685"), f7, b7, s7) && s7.pct == 0, "a 6-field row (pre-16.39) parses with the polarity unknown (0)");
    ktl::Book PB; PB.hasNow = true; PB.nowPct = -100;
    CHECK(ktl::polarity(100, PB) == 1 && ktl::polarity(-100, PB) == -1, "a step's own polarity wins: +100 gold, -100 magenta");
    CHECK(ktl::polarity(0, PB) == -1, "a step without a polarity takes KINGNOW's (here negative)");
    ktl::Book PN; CHECK(ktl::polarity(0, PN) == 1, "nothing known -> positive (gold)");
    // ---- (v0.14) the status line the toggle test reads
    ktl::Book SB; SB.hasNow = true; SB.nowStrike = 7600; SB.nowPct = 100; ktl::Step st1; st1.strike = 7500; SB.steps.push_back(st1);
    CHECK(ktl::statusLine(true, "ES", "IF", "ES", SB, 3.25f) == "KTSTATUS,IF,ES,IF,1,1,7600,100,3.25", "Source IF: the IF book reports drawn=1 with its steps, KINGNOW and the offset");
    CHECK(ktl::statusLine(true, "ES", "SPX", "ES", SB, 3.25f) == "KTSTATUS,IF,ES,SPX,0,1,7600,100,3.25", "Source IF: the SPX book reports drawn=0 (its data is still there, just not drawn)");
    CHECK(ktl::statusLine(false, "ES", "SPX", "ES", SB, 0.0f) == "KTSTATUS,Skylit,ES,SPX,1,1,7600,100,0.00" && ktl::statusLine(false, "ES", "IF", "ES", SB, 0.0f).find(",IF,0,") != std::string::npos, "Source Skylit: SPX drawn, IF not");
    ktl::Book EB; CHECK(ktl::statusLine(true, "ES", "IF", "ES", EB, 0.0f) == "KTSTATUS,IF,ES,IF,0,0,0,0,0.00", "an empty book is never 'drawn' whichever source");
    // ---- the stale age
    CHECK(ktl::staleAge(-1, 1) < 0 && ktl::staleAge(35229, 35229 + 180) == 3.0 && ktl::staleAge(23 * 3600 + 50 * 60, 10 * 60) == 20.0, "stale age: unknown, 3 min, the overnight wrap");
    printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
