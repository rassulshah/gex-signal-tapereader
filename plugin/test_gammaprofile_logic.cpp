/********************************************************************************
 *  test_gammaprofile_logic.cpp — Gate B (logic) regression for lsGammaProfile.
 *
 *  Pins every decision in GammaProfileLogic.h against the doctrine fixtures. No IRT SDK:
 *      cloud   : g++ -std=c++14 -o /tmp/gpl plugin/test_gammaprofile_logic.cpp && /tmp/gpl
 *      windows : plugin\run-logic-tests.bat  (cl.exe, same file)
 *  Exit code = number of failures. Every case names the doctrine rule it pins.
 *
 *  Fixture A = the live SPXW ladder of 2026-09-16 09:47 CT (the same-moment test that passed
 *  row-for-row against Atlas and the IRT rail) — 40 strikes 7500..7695, King 7685 (+100).
 ********************************************************************************/
#include "GammaProfileLogic.h"
#include <cstdio>
#include <cstring>

static int fails = 0, passes = 0;
#define CHECK(cond, msg) do { if (cond) { passes++; printf("  ok    %s\n", msg); } else { fails++; printf("  FAIL  %s   (line %d)\n", msg, __LINE__); } } while (0)

static gpl::Node N(float spx, float pct, int rank, bool king = false, const char* type = "", float basis = 4.25f)
{
    gpl::Node n; n.spx = spx; n.price = spx + basis; n.pct = pct; n.rank = rank; n.king = king; n.type = type; return n;
}
static int idxOfSpx(const std::vector<gpl::Node>& v, float spx){ for (size_t i=0;i<v.size();i++) if (std::fabs(v[i].spx-spx)<0.01f) return (int)i; return -1; }
static void rankByMag(std::vector<gpl::Node>& v){ std::vector<int> o(v.size()); for(size_t i=0;i<v.size();i++)o[i]=(int)i;
    std::sort(o.begin(),o.end(),[&](int a,int b){ return std::fabs(v[a].pct)>std::fabs(v[b].pct); }); for(size_t i=0;i<o.size();i++) v[o[i]].rank=(int)i+1; }

// ---- Fixture A: 2026-09-16 09:47:09 CT, ES1 (front) = SPX + 4.25 -------------------------------
static std::vector<gpl::Node> fixtureA()
{
    std::vector<gpl::Node> v;
    float rows[][2] = { {7695,1},{7690,-18},{7685,100},{7680,16},{7675,31},{7670,0},{7665,12},{7660,63},{7655,-10},{7650,-18},
                        {7645,-4},{7640,4},{7635,3},{7630,-11},{7625,-2},{7620,8},{7615,-28},{7610,-49},{7605,-54},{7600,-40},
                        {7595,-3},{7590,-14},{7585,-25},{7580,-10},{7575,3},{7570,-7},{7565,-6},{7560,-6},{7555,-8},{7550,6},
                        {7545,-6},{7540,-6},{7535,-10},{7530,2},{7525,7},{7520,27},{7515,29},{7510,0},{7505,5},{7500,68} };
    for (size_t i=0;i<sizeof(rows)/sizeof(rows[0]);i++) v.push_back(N(rows[i][0], rows[i][1], 0, rows[i][1]==100));
    rankByMag(v);
    return v;
}

int main()
{
    printf("=== lsGammaProfile logic regression (GammaProfileLogic.h) ===\n");

    // ------------------------------------------------------------------ ROLES / GATEKEEPER
    {
        printf("[roles] fixture A, spot SPX 7609.45 (below the King 7685)\n");
        std::vector<gpl::Node> v = fixtureA();
        gpl::Roles R = gpl::roles(v, 7609.45f + 4.25f, true);
        CHECK(R.kIdx == idxOfSpx(v, 7685), "King is 7685");
        CHECK(R.fIdx == idxOfSpx(v, 7500), "Floor = biggest |node| below spot = 7500 (+68)");
        CHECK(R.cIdx == idxOfSpx(v, 7660), "Ceiling = biggest |node| above spot excluding the King = 7660 (+63)");
        // doctrine: ONE gatekeeper, the largest node strictly between spot and the King, >= 30% King
        int g = R.gIdx;
        CHECK(g == idxOfSpx(v, 7660), "Gatekeeper = the dominant blocker between 7609 and 7685 = 7660 (+63), magnitude-ranked");
        int gCount = 0; for (size_t i=0;i<R.role.size();i++) if (R.role[i]>=gpl::R_GATE) gCount++;
        CHECK(gCount == 1, "exactly ONE G on the board (v0.41 printed four)");
        CHECK(R.role[g] == gpl::R_GATE_CEIL, "7660 is both gatekeeper and ceiling -> G.C (the ceiling is not lost)");
        CHECK(std::string(gpl::roleTag(R.role[g], "K")).size() == 3, "G.C tag is three characters");
        CHECK(R.role[idxOfSpx(v, 7610)] == gpl::R_NONE, "7610 (-49, below spot) carries no G");
        CHECK(R.role[idxOfSpx(v, 7675)] == gpl::R_NONE, "7675 (+31, on the path but smaller than 7660) carries no G");
    }
    {
        printf("[roles] spot inside the barney stack, 12:04 CT geometry (spot 7607, King 7685)\n");
        std::vector<gpl::Node> v;
        float rows[][2] = { {7685,-100},{7660,45},{7615,-50},{7610,-95},{7605,-86},{7600,-60},{7500,60} };
        for (size_t i=0;i<7;i++) v.push_back(N(rows[i][0], rows[i][1], 0, i==0)); rankByMag(v);
        gpl::Roles R = gpl::roles(v, 7607.0f + 4.25f, true);
        CHECK(R.gIdx == idxOfSpx(v, 7610), "G = 7610 (-95): largest node between 7607 and 7685 (3 pts above price is still 'between')");
        CHECK(R.fIdx == idxOfSpx(v, 7605), "F = 7605 (-86): biggest below spot");
        CHECK(R.role[idxOfSpx(v, 7610)] == gpl::R_GATE_CEIL, "7610 is gatekeeper AND ceiling -> G.C");
    }
    {
        printf("[roles] no node >= 30%% between spot and King -> NO gatekeeper (a small node is not a gate)\n");
        std::vector<gpl::Node> v;
        float rows[][2] = { {7685,100},{7660,25},{7640,12},{7550,80} };
        for (size_t i=0;i<4;i++) v.push_back(N(rows[i][0], rows[i][1], 0, i==0)); rankByMag(v);
        gpl::Roles R = gpl::roles(v, 7620.0f + 4.25f, true);
        CHECK(R.gIdx == -1, "no G when nothing on the path clears 30% of the King");
        CHECK(R.cIdx == idxOfSpx(v, 7660), "ceiling still named (25%) — the C role is magnitude-only");
    }
    {
        printf("[roles] spot ABOVE the King: the path is downward\n");
        std::vector<gpl::Node> v;
        float rows[][2] = { {7600,100},{7640,55},{7620,35},{7700,70} };
        for (size_t i=0;i<4;i++) v.push_back(N(rows[i][0], rows[i][1], 0, i==0)); rankByMag(v);
        gpl::Roles R = gpl::roles(v, 7660.0f + 4.25f, true);
        CHECK(R.gIdx == idxOfSpx(v, 7640), "G = 7640 (+55): the biggest node between spot 7660 and the King 7600 below");
        CHECK(R.role[idxOfSpx(v, 7640)] == gpl::R_GATE_FLOOR, "...and it is also the floor -> G.F");
        CHECK(R.cIdx == idxOfSpx(v, 7700), "C = 7700 above spot");
    }
    {
        printf("[roles] no spot: spot falls back to the King, so nothing is 'between'\n");
        std::vector<gpl::Node> v = fixtureA();
        gpl::Roles R = gpl::roles(v, 0.0f, false);
        CHECK(R.gIdx == -1, "no G without a spot");
        CHECK(R.kIdx >= 0, "King still identified");
    }

    // ------------------------------------------------------------------ AIR POCKETS
    {
        printf("[air] fixture A: thin runs exist, but only the BOUNDED one is a pocket\n");
        std::vector<gpl::Node> v = fixtureA();
        std::vector<gpl::Pocket> P = gpl::airPockets(v);
        // thin runs in A: 7645..7620 (7645 -4, 7640 4, 7635 3, 7630 -11? no: 11 >= 8 breaks it) -> runs: 7645,7640,7635 (3 thin) bounded by 7650 (-18) below? |−18| < 20 -> NOT an edge.
        // 7575..7505 tail: bounded above by 7580 (-10)? no. So fixture A has NO doctrine pocket. v0.41 shaded the whole tail.
        CHECK(P.empty(), "no pocket in fixture A: every thin run touches a sub-20% neighbour or the tail (v0.41 shaded 7505-7575)");
    }
    {
        printf("[air] a thin run between two real nodes IS a pocket; the same run at the tail is NOT\n");
        std::vector<gpl::Node> v;
        float rows[][2] = { {7700,100},{7695,2},{7690,-3},{7685,1},{7680,-45},{7675,3},{7670,2},{7665,1} };
        for (size_t i=0;i<8;i++) v.push_back(N(rows[i][0], rows[i][1], 0, i==0)); rankByMag(v);
        std::vector<gpl::Pocket> P = gpl::airPockets(v);
        CHECK(P.size() == 1, "exactly one pocket");
        if (P.size()==1) {
            CHECK(std::fabs(P[0].lo - (7685+4.25f)) < 0.01f && std::fabs(P[0].hi - (7695+4.25f)) < 0.01f, "the pocket is 7685..7695 (between 7680 -45 and 7700 King)");
            CHECK(P[0].neg == false, "run 2,-3,1 sums to 0 -> not negative");
        }
        CHECK(true, "the tail run 7665..7675 (3 thin) has no node beyond it -> not banded");
    }
    {
        printf("[air] two thin strikes are not a pocket (min run 3); a run under -gamma leans negative\n");
        std::vector<gpl::Node> v;
        float rows[][2] = { {7700,100},{7695,-2},{7690,-5},{7685,-1},{7680,40},{7675,-6},{7670,-7},{7665,-50} };
        for (size_t i=0;i<8;i++) v.push_back(N(rows[i][0], rows[i][1], 0, i==0)); rankByMag(v);
        std::vector<gpl::Pocket> P = gpl::airPockets(v);
        CHECK(P.size() == 1, "7675/7670 (two thin) is NOT a pocket; 7685..7695 (three) is");
        if (P.size()==1) CHECK(P[0].neg == true, "run -2,-5,-1 is net negative -> violent pathway tint");
    }

    // ------------------------------------------------------------------ STACKS / TAGS
    {
        printf("[stacks] the panel's tags -> one bracket per run, named once\n");
        std::vector<gpl::Node> v;
        v.push_back(N(7685,-100,1,true)); v.push_back(N(7615,-50,6,false,"BARNEYM")); v.push_back(N(7610,-95,2,false,"BARNEY"));
        v.push_back(N(7605,-86,3,false,"BARNEYM")); v.push_back(N(7600,-60,5,false,"BARNEYM")); v.push_back(N(7500,60,4,false,"RUG"));
        v.push_back(N(7660,45,7,false,"PIKA")); v.push_back(N(7655,35,8,false,"PIKAM"));
        std::vector<gpl::Stack> S = gpl::stacks(v);
        CHECK(S.size() == 2, "two stacks: the barney 7600-7615 and the pika 7655-7660");
        int nb=0, np=0; for (size_t i=0;i<S.size();i++){ if(!S[i].pos){ nb++; CHECK(S[i].n==4 && std::fabs(S[i].lo-7604.25f)<0.01f && std::fabs(S[i].hi-7619.25f)<0.01f, "barney spans 7600..7615, 4 members"); CHECK(S[i].namedIdx==idxOfSpx(v,7610), "barney named on 7610 (the biggest)"); } else { np++; CHECK(S[i].n==2, "pika has 2 members"); } }
        CHECK(nb==1 && np==1, "one of each family");
        CHECK(std::strcmp(gpl::patternTag("BARNEY"),"B")==0 && std::strcmp(gpl::patternTag("PIKA"),"P")==0, "named members print B / P");
        CHECK(std::strcmp(gpl::patternTag("BARNEYM"),"")==0 && std::strcmp(gpl::patternTag("PIKAM"),"")==0, "members print NO letter (the bracket marks the run)");
        CHECK(std::strcmp(gpl::patternTag("RUG"),"R")==0 && std::strcmp(gpl::patternTag("RRUG"),"RR")==0, "rug / reverse rug print R / RR");
    }
    {
        printf("[stacks] two adjacent named runs of the same family stay two brackets\n");
        std::vector<gpl::Node> v;
        v.push_back(N(7700,100,1,true)); v.push_back(N(7620,-40,3,false,"BARNEYM")); v.push_back(N(7615,-70,2,false,"BARNEY"));
        v.push_back(N(7610,-65,4,false,"BARNEY")); v.push_back(N(7605,-35,5,false,"BARNEYM"));
        std::vector<gpl::Stack> S = gpl::stacks(v);
        CHECK(S.size() == 2, "a second NAMED member starts a new bracket");
    }

    // ------------------------------------------------------------------ LEVEL -> NODE
    {
        printf("[levels] CW / PW tag the node with the same SPX strike; price only as a fallback\n");
        std::vector<gpl::Node> v = fixtureA();
        CHECK(gpl::levelNode(v, 7679.25f, 7675.0f) == idxOfSpx(v, 7675), "CW spx 7675 -> node 7675");
        CHECK(gpl::levelNode(v, 7604.25f, 7600.0f) == idxOfSpx(v, 7600), "PW spx 7600 -> node 7600");
        CHECK(gpl::levelNode(v, 7604.25f + 70.0f, 7600.0f) == idxOfSpx(v, 7600), "strike match ignores a 70-pt price basis (Dec chart)");
        CHECK(gpl::levelNode(v, 7679.25f, 0.0f) == idxOfSpx(v, 7675), "no strike: nearest price within 1 pt");
        CHECK(gpl::levelNode(v, 7621.96f + 4.25f, 7621.96f) == -1, "the FLIP (7621.96) is a price, not a strike -> no node");
    }

    // ------------------------------------------------------------------ CONTRACT OFFSET
    {
        printf("[offset] SCALEREF anchors; SPOT is the fallback; clamp protects the NQ chart\n");
        float off;
        CHECK(gpl::contractOffset(7683.0f, true, 7614.06f, true, 7670.75f, off) && std::fabs(off - 68.94f) < 0.01f, "Dec chart 7683 vs SCALEREF 7614.06 -> +68.94 (the Sep->Dec basis, 09:47)");
        CHECK(gpl::contractOffset(7686.75f, true, 7687.07f, true, 7686.75f, off) && std::fabs(off + 0.32f) < 0.01f, "after Skylit's roll SCALEREF ~= chart -> ~0 (self-healed, 10:44)");
        CHECK(gpl::contractOffset(7683.0f, false, 0.0f, true, 7670.75f, off) && std::fabs(off - 12.25f) < 0.01f, "no SCALEREF -> SPOT anchor");
        CHECK(!gpl::contractOffset(29400.0f, true, 7614.06f, true, 7670.75f, off) && off == 0.0f, "NQ chart vs an ES anchor -> clamp, no shift");
        CHECK(!gpl::contractOffset(0.0f, true, 7614.06f, true, 7670.75f, off), "no chart close -> no shift");
        CHECK(!gpl::contractOffset(7683.0f, false, 0.0f, false, 0.0f, off), "no anchor at all -> no shift");
    }

    // ------------------------------------------------------------------ TOP-N / PRIMARY
    {
        printf("[topN] the Show filter decides which bars are primary (coloured) vs grey\n");
        std::vector<gpl::Node> v = fixtureA();
        int prim5=0, prim3=0, thr20=0;
        for (size_t i=0;i<v.size();i++){ if (gpl::isPrimary(v[i],1,20)) prim5++; if (gpl::isPrimary(v[i],0,20)) prim3++; if (gpl::isPrimary(v[i],4,20)) thr20++; }
        CHECK(prim5 == 5, "Top 5 -> exactly five primary bars");
        CHECK(prim3 == 3, "Top 3 -> exactly three");
        CHECK(thr20 == 11, ">= 20% -> the eleven prominent nodes of 09:47 (100,68,63,54,49,40,31,29,28,27,25)");
        CHECK(gpl::isPrimary(v[idxOfSpx(v,7685)],1,20) && v[idxOfSpx(v,7685)].rank==1, "King is rank 1 and primary");
        CHECK(v[idxOfSpx(v,7500)].rank==2 && v[idxOfSpx(v,7660)].rank==3, "ranks 2/3 = 7500 (+68) / 7660 (+63) — magnitude, not sign");
    }

    // ------------------------------------------------------------------ REGIME LINE
    {
        printf("[regime] the panel's row is displayed; the sum is a labelled fallback\n");
        gpl::RegimeText r = gpl::regimeLine(true, "NEG", "WHIPSAW", false, 0.0f);
        CHECK(r.neg && !r.at && !r.na && r.line.find("-gamma") != std::string::npos && r.line.find("WHIPSAW") != std::string::npos && r.line.find("EXTREMES") != std::string::npos, "NEG WHIPSAW -> '-gamma | WHIPSAW | fade EXTREMES only / sit out'");
        r = gpl::regimeLine(true, "POS", "RANGE", false, 0.0f);
        CHECK(!r.neg && r.line.find("+gamma") != std::string::npos && r.line.find("FADE the extremes") != std::string::npos, "POS RANGE -> fade");
        r = gpl::regimeLine(true, "NEG", "TREND_DN", true, 0.0f);
        CHECK(r.line.find("TREND DOWN") != std::string::npos && r.line.find("FOLLOW") != std::string::npos && r.line.find("!CONFLICT") != std::string::npos, "TREND_DN + conflict -> 'TREND DOWN | FOLLOW, don't fade  !CONFLICT'");
        r = gpl::regimeLine(true, "AT", "MIXED", false, 0.0f);
        CHECK(r.at && r.line.find("AT flip") != std::string::npos && r.line.find("no edge - wait") != std::string::npos, "AT -> 'AT flip', ASCII hyphen only (no mojibake)");
        r = gpl::regimeLine(true, "NA", "FORMING", false, 0.0f);
        CHECK(r.na && r.line.find("flip n/a") != std::string::npos, "NA -> 'flip n/a' (was drawn as AT flip in 0.42)");
        r = gpl::regimeLine(false, "", "", false, -12.0f);
        CHECK(r.neg && r.line.find("(sum, no row)") != std::string::npos, "no row -> the sum, and it SAYS so");
        for (size_t i=0;i<r.line.size();i++) if ((unsigned char)r.line[i] > 127) { CHECK(false, "non-ASCII byte in a regime line"); break; }
    }

    // ---- (v0.59) the rank the rail draws with: Book or the Atlas pool
    CHECK(gpl::effectiveRank(3, 0, false) == 3 && gpl::effectiveRank(3, 7, false) == 3, "Rank = Book: the within-book rank, whatever the pool says");
    CHECK(gpl::effectiveRank(3, 7, true) == 7 && gpl::effectiveRank(1, 2, true) == 2, "Rank = Atlas merge: the pooled rank (SPX 7650 is 1 in its book, 2 in the pool)");
    CHECK(gpl::effectiveRank(1, 0, true) == gpl::NO_RANK, "an unpooled row under Atlas merge gets NO_RANK — never its book rank in disguise");
    { gpl::Node n; n.rank = gpl::effectiveRank(2, 0, true); n.pct = 66; CHECK(!gpl::isPrimary(n, 1, 20), "... so it is not primary under Top 5: grey, no badge"); }
    { gpl::Node n; n.rank = gpl::effectiveRank(2, 4, true); n.pct = 66; CHECK(gpl::isPrimary(n, 1, 20) && !gpl::isPrimary(n, 0, 20), "SPX 7655: pooled 4 -> primary under Top 5, not under Top 3"); }
    { gpl::Node n; n.rank = gpl::effectiveRank(3, 6, true); n.pct = 36; CHECK(!gpl::isPrimary(n, 1, 20), "SPY 761: pooled 6 -> outside the five (Atlas showed it grey)"); }

    // ---- (v0.60) the per-instance status line (two rails verified from outside)
    { std::string l = gpl::statusLine(2, 1, 11, true, 0, 1400, 2, 0, 40, 3, 11, true);
      CHECK(l == "GPSTATUS,SPY,Left,GammaProfile-SPY.csv,11,Atlas,0,1400,2,0,40,3,11,1", "SPY rail, Left: the status names its book, side, file and anchor"); }
    { std::string l = gpl::statusLine(1, 0, 15, true, 0, 1400, 1398, 0, 90, 2, 15, true);
      CHECK(l.find("GPSTATUS,SPX,Right,GammaProfile.csv,15,Atlas,") == 0 && l.find(",90,2,15,1") != std::string::npos, "SPX rail, Right: its own line, from GammaProfile.csv"); }
    { std::string l = gpl::statusLine(3, 0, 0, false, 0, 0, 0, 0, 100, 0, 0, false);
      CHECK(l == "GPSTATUS,IF,Right,GammaProfile-IF.csv,0,Book,0,0,0,0,100,0,0,0", "an instance that loaded nothing says so: 0 strikes, rendered 0"); }
    CHECK(std::string(gpl::bookName(0)) == "Auto" && std::string(gpl::bookFile(0)) == "GammaProfile.csv" && std::string(gpl::bookFile(1)) == "GammaProfile.csv", "Auto and SPX both read the Skylit SPX file");

    // ---- (v0.61) Book = Both: one instance, SPY rail left, SPX rail right
    { gpl::RailLayout r = gpl::railLayout(4, 1, 90, 40); CHECK(r.both && r.mainSide == 0 && r.spySide == 1 && r.spyWidth == 40, "Both: the SPX rail is RIGHT even with Side = Left; the SPY rail LEFT at its own width"); }
    { gpl::RailLayout r = gpl::railLayout(1, 1, 90, 40); CHECK(!r.both && r.mainSide == 1 && r.spyWidth == 0, "Book = SPX: Side is honoured, no SPY rail"); }
    { gpl::RailLayout r = gpl::railLayout(4, 0, 90, 500); CHECK(r.spyWidth == 400, "the SPY width is clamped like Width px (40..400)"); }
    { gpl::RailLayout r = gpl::railLayout(4, 0, 90, 0); CHECK(r.spyWidth == 90, "SPY width 0 = same as the SPX rail"); }
    CHECK(std::string(gpl::bookName(4)) == "Both" && std::string(gpl::bookFile(4)) == "GammaProfile.csv", "Both names itself; its main file is the SPX tape");

    // ---- (v0.62) the bubble on a short bar, the shared thickness
    CHECK(!gpl::bubbleOutside(0, 40, 10) && gpl::bubbleOutside(0, 11, 10) && gpl::bubbleOutside(0, 23, 10) && !gpl::bubbleOutside(0, 24, 10), "Rank at = Inside: inside only when the bar is at least 2r+4 long (11 px at r=10 -> outside)");
    CHECK(gpl::bubbleOutside(1, 400, 10), "Rank at = Outside: always outside");
    CHECK(gpl::autoBarH(31) == 24 && gpl::autoBarH(62) == 40 && gpl::autoBarH(4) == 6 && gpl::autoBarH(3) == 6, "auto thickness: 0.78 x spacing, capped 40, 6 below 5 px (the SPX 5-pt spacing -> ~24; the SPY 10-pt spacing would hit the cap)");

    printf("=== %d passed, %d failed ===\n", passes, fails);
    return fails;
}
