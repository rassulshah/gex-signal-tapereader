// test_blb_logic.cpp -- BookLevelBox 1.0.0 + the lsBookFlow 1.34.0 turn label, SDK-free: persistence, colour by implication (flip at a
// resistance), texts, layout (drops from the right, never past the axis), squares, params file, Pb zones / windows, record lines, versions.
// Build (tests_bf134/): g++ -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_blb_logic.cpp -o t && ./t
#include <cstdio>
#include "../SignalBoard.h"
#include "../OrderFlowPaneSettings.h"
#include "../BookFlowLogic.h"
#include "../BookLevelBox.h"
static int P = 0, F = 0; static void check(bool c, const std::string& m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m.c_str()); } }
static blb::Read rd(int w, double x, int c) { blb::Read r; r.word = w; r.x = x; r.col = c; return r; }
int main() {
    // persistence (figs2.smooth): a new state shows after 15 identical seconds; a ratio / colour change inside the same word is immediate
    {   blb::Engine E; for (int q = 0; q < 3; ++q) E.sm.cur[q] = rd(blb::W_QUIET, 0, blb::GRAY);
        int shownAt = -1;
        for (int k = 0; k < 20; ++k) { blb::Read r[3] = { rd(blb::W_PULL, 2.0, blb::RED), rd(blb::W_STEADY, 1, blb::GRAY), rd(blb::W_NORMAL, 1, blb::GRAY) }; E.smoothStep(r); if (shownAt < 0 && E.sm.cur[0].word == blb::W_PULL) shownAt = k; }
        check(shownAt == 14, "a new state shows on its 15th second (" + std::to_string(shownAt) + ")");
        blb::Read r[3] = { rd(blb::W_PULL, 1.2, blb::GRAY), rd(blb::W_STEADY, 1.1, blb::GRAY), rd(blb::W_NORMAL, 1, blb::GRAY) }; E.smoothStep(r);
        check(E.sm.cur[0].word == blb::W_PULL && E.sm.cur[0].col == blb::GRAY && E.sm.cur[0].x == 1.2, "same word: the new ratio / colour at once");
        for (int k = 0; k < 14; ++k) { blb::Read q[3] = { rd(blb::W_STACK, 3, blb::GREEN), r[1], r[2] }; E.smoothStep(q); }
        check(E.sm.cur[0].word == blb::W_PULL, "14 s of a new word: not shown yet");
        { blb::Read q[3] = { rd(blb::W_PULL, 1.6, blb::RED), r[1], r[2] }; E.smoothStep(q); }
        for (int k = 0; k < 14; ++k) { blb::Read q[3] = { rd(blb::W_STACK, 3, blb::GREEN), r[1], r[2] }; E.smoothStep(q); }
        check(E.sm.cur[0].word == blb::W_PULL && E.sm.cur[0].col == blb::RED, "interrupted: the count starts again"); }
    // colours by implication: support raw, resistance flipped (sellers stacking = red); gray stays gray
    {   blb::Engine::Shown sh; sh.on = true; sh.c.name = "VWAP"; sh.r[0] = rd(blb::W_STACK, 25, blb::GREEN); sh.r[1] = rd(blb::W_THIN, 1.6, blb::RED); sh.r[2] = rd(blb::W_STACK, 1.8, blb::RED);
        sh.c.ds = 0; blb::BoxText s = blb::boxText(sh, blb::Params());
        check(s.col[0] == blb::GREEN && s.col[1] == blb::RED && s.col[2] == blb::RED, "support: defenders stacking green, behind thinning red, attackers stacking red");
        sh.c.ds = 1; blb::BoxText r = blb::boxText(sh, blb::Params());
        check(r.col[0] == blb::RED && r.col[1] == blb::GREEN && r.col[2] == blb::GREEN, "resistance: flipped (sellers stacking = red, their support thinning = green, buyers stacking under = green)");
        check(r.level == "VWAP" && r.part[0] == "Defenders stacking 20x+?" && r.part[1] == "Behind thinning 1.6x?" && r.part[2] == "Attackers stacking 1.8x?", "texts: 20x+, ratios one decimal, ?");
        sh.r[0] = rd(blb::W_STACK, 1.3, blb::GRAY); sh.r[1] = rd(blb::W_STEADY, 1.1, blb::GRAY); sh.r[2] = rd(blb::W_NORMAL, 0.9, blb::GRAY);
        blb::BoxText g = blb::boxText(sh, blb::Params());
        check(g.part[0] == "Defenders steady?" && g.part[1] == "Behind steady?" && g.part[2] == "Attackers normal?" && g.col[0] == blb::GRAY && g.col[2] == blb::GRAY, "under the thresholds: steady / steady / normal, gray");
        sh.r[0] = rd(blb::W_QUIET, 0, blb::GRAY); check(blb::boxText(sh, blb::Params()).part[0] == "Defenders quiet?", "quiet");
        blb::Engine::Shown off; check(blb::boxText(off, blb::Params()).level.empty(), "no box: nothing"); }
    // layout: one row, left to right, drops Attackers then Behind then Defenders, never past `right`, never the name alone
    {   blb::BoxText t; t.level = "G 7630"; t.part[0] = "Defenders stacking 2.1x?"; t.part[1] = "Behind stacking 3.3x?"; t.part[2] = "Attackers normal?";
        auto M = [](const std::string& s) { return (int)s.size() * 7; };
        const blb::Lay L = blb::layout(t, 300, 1, 2000, M);
        check(L.ok && L.segs.size() == 7 && L.segs[0].text == "G 7630" && L.segs[1].text == "|" && L.segs[6].text == "Attackers normal?" && L.b - L.t == blb::BOX_H && L.r <= 2000, "full line: level | D | B | A");
        const blb::Lay L2 = blb::layout(t, 300, 1, L.r - 1, M); check(L2.ok && L2.segs.size() == 5 && L2.dropped == 1 && L2.r <= L.r - 1, "narrow: Attackers dropped first");
        const blb::Lay L3 = blb::layout(t, 300, 1, 300 + 4 + 42 + 6 + 7 + 6 + 168 + 4, M); check(L3.ok && L3.segs.size() == 3 && L3.dropped == 2, "narrower: level | Defenders only");
        const blb::Lay L4 = blb::layout(t, 300, 1, 360, M); check(!L4.ok && L4.segs.empty(), "no room for one part: nothing (never the name alone)"); }
    // squares
    {   blb::SqAcc a; a.secs = 60; a.red[0] = 20; a.green[0] = 40; a.green[1] = 19; a.red[1] = 0;
        const blb::Square q = blb::squareOf(a); check(q.col[0] == blb::RED && q.col[1] == blb::GRAY, "red if red >= 20 s (before green); 19 s green = gray");
        a.red[0] = 19; check(blb::squareOf(a).col[0] == blb::GREEN, "green if green >= 20 s and red < 20 s");
        check(blb::candleOf(1000, 180) == 1080 && blb::candleOf(1080, 180) == 1260 && blb::candleOf(1079, 180) == 1080, "a second belongs to [K - spb, K)");
        blb::Square back; long long k = 0; a.level = "PWL"; const std::string ln = blb::sqLine(1080, blb::squareOf(a), a);
        check(blb::parseSq(ln, k, back) && k == 1080 && back.col[0] == blb::squareOf(a).col[0] && back.col[1] == blb::squareOf(a).col[1] && back.secs == 60 && back.level == "PWL", "C line round trip: " + ln);
        check(!blb::parseSq("C|x|g|g|1|0|0|0|0|L", k, back) && !blb::parseSq("C|5|q|g|1|0|0|0|0|L", k, back) && !blb::parseSq("S|5|g", k, back), "garbled C lines rejected"); }
    // params file
    {   blb::Params p; blb::Profile pr;
        check(blb::parseParams({ "VERSION|1", "MARKET|ES", "AT|2026-11-04 18:00", "PARAM|def_x|2.0", "PARAM|quiet_lots|40", "PARAM|behind_stack_x|2.5", "PARAM|behind_thin_x|0.67", "PARAM|att_stack_x|1.4", "PARAM|att_pull_x|1.5",
                                  "SHOW|D|0", "SHOW|B|1", "PROFILE|2026-11-03|POC|7600.25", "PROFILE|2026-11-03|VAH|7620", "PROFILE|2026-11-03|VAL|7580.5" }, "ES", p, pr)
              && p.defX == 2.0 && p.behStack == 2.5 && !p.showD && p.showB && pr.ok && pr.day == "2026-11-03" && pr.poc == 7600.25, "a good file: thresholds, SHOW, the prior RTH profile");
        blb::Params q; check(!blb::parseParams({ "MARKET|ES", "VERSION|1" }, "ES", q, pr) && q.why.find("VERSION") != std::string::npos, "VERSION first");
        check(!blb::parseParams({ "VERSION|1", "MARKET|NQ" }, "ES", q, pr) && q.why == "wrong market", "wrong market");
        check(!blb::parseParams({ "VERSION|1", "MARKET|ES", "PARAM|def_x|nan" }, "ES", q, pr), "a garbled number");
        check(!blb::parseParams({ "VERSION|1", "MARKET|ES", "PARAM|behind_thin_x|1.5" }, "ES", q, pr), "thresholds out of range");
        {   // the exact file lra.book_level_combo 1.2.0 export_flex writes (CRLF, a # line, AT)
            blb::Params x; blb::Profile y;
            check(blb::parseParams({ "VERSION|1\r", "MARKET|ES\r", "AT|2026-11-04 18:00\r", "# book_level_combo 1.2.0 (BLC120 2026-11-04) - lsBookFlow level box params\r", "PARAM|def_x|1.5\r", "PARAM|quiet_lots|40\r",
                                     "PARAM|behind_stack_x|2\r", "PARAM|behind_thin_x|0.67\r", "PARAM|att_stack_x|1.4\r", "PARAM|att_pull_x|1.5\r", "SHOW|D|0\r", "SHOW|B|0\r",
                                     "PROFILE|2026-11-03|POC|7600\r", "PROFILE|2026-11-03|VAH|7601\r", "PROFILE|2026-11-03|VAL|7600\r" }, "ES", x, y)
                  && x.file && x.quiet == 40 && !x.showD && y.ok && y.day == "2026-11-03" && y.vah == 7601, "the nightly's export parses (CRLF, comment, AT, PROFILE)"); }
        blb::Params d; blb::Engine E; E.market = "ES"; check(E.quietFloor() == 40, "ES quiet floor 40 lots");
        E.market = "NQ"; check(E.quietFloor() == 40, "another market before its first arrival: 40");
        E.arrAct = { 15, 30, 45 }; check(E.quietFloor() == 20.0, "then scaled by its own arrivals as quiet_floor: 40 x 30 / 60 = 20");
        E.P.quiet = 12; check(E.quietFloor() == 12, "the nightly's quiet_lots wins"); }
    // Pb zones from the BBO records (bottom half of the BO candle / top half of the BD candle; Python half-to-even mid), windows
    {   const std::string l1 = "BBO|1000|820|1|1|fired|7600|1|30420|30400|20|10|500|300|0|0|0|0|0|1|2|30418|1|1.4.0-zz3|5|0|0";
        const std::string l2 = "BBO|2000|1900|-1|1|fired|7600|1|30461|30440|20|10|500|300|0|0|0|0|0|1|2|30418|1|1.4.0-zz3|5|0|0";
        const std::string nf = "BBO|2000|1900|-1|0|no_confirm|7600|1|30461|30440|20|10|500|300|0|0|0|0|0|1|2|30418|1|1.4.0-zz3|5|0|0";
        const std::string old = "BBO|2000|1900|-1|1|fired|7600|1|30461|30440|20|10|500|300|0|0|0|0|0|1|2|30418|1|1.32.0|5|0|0";
        blb::BoRec a, b, c; check(blb::parseBbo(l1, a) && a.dir == 1 && a.t0 == 820 && a.t == 1000 && a.hk == 30420 && a.lk == 30400, "a fired BO line");
        check(blb::parseBbo(l2, b) && b.dir == -1 && !blb::parseBbo(nf, c) && !blb::parseBbo(old, c) && !blb::parseBbo("BBO|1|2", c), "fired 3-bar lines only");
        check(blb::midTicks(30420, 30400) == 30410 && blb::midTicks(30461, 30440) == 30450 && blb::midTicks(30463, 30440) == 30452 && blb::midTicks(30461, 30442) == 30452, "mid = round half to even (30450.5 -> 30450, 30451.5 -> 30452)");
        std::vector<blb::PbIn> bars; std::vector<blb::Piv> piv;
        blb::Piv o; o.t = 700; o.tc = 760; o.type = -1; o.l = 30380; o.h = 30390; piv.push_back(o);   // the BO wave's origin low 30380
        blb::PbIn x; x.t = 1500; x.h = 30400; x.l = 30370; bars.push_back(x);                       // a bar below the origin at 1500
        const std::vector<blb::Level> L = blb::pbLevels({ a, b }, bars, piv);
        check(L.size() == 2 && L[0].lo == 30400 && L[0].hi == 30410 && L[0].fixedZone && L[0].tFrom == 1000 && L[0].tTo == 1500 && L[0].name == "Pb BO", "BO: zone low .. mid, from its fire bar, until the origin broke (1500 < the next BD 2000)");
        check(L[1].lo == 30450 && L[1].hi == 30461 && L[1].tFrom == 2000 && L[1].tTo == 0 && L[1].name == "Pb BD", "BD: zone mid .. high, open window"); }
    // the turn label: the two drawn bars
    {   check(bf::turnLabel(90, 60) == "1.5x" && bf::turnLabel(300, 100) == "3.0x" && bf::turnLabel(61, 60) == "1.0x", "defending / attacking, one decimal");
        check(bf::turnLabel(90, 0) == "" && bf::turnLabel(90, -40) == "" && bf::turnLabel(0, 10) == "", "attacking bar <= 0 (or no defending bar): no number (was 999%)");
        check(bf::turnMarkText(90, 60, true) == "1.5x ?" && bf::turnMarkText(90, 0, true) == "?" && bf::turnMarkText(90, 0, false) == "", "forming candle keeps its ?");
        for (long long d = 1; d < 400; d += 7) for (long long a2 = 1; a2 < d; a2 += 5) { char b[32]; std::snprintf(b, sizeof b, "%.1fx", (double)d / (double)a2); if (bf::turnLabel(d, a2) != b) { check(false, "label != ratio at " + std::to_string(d) + "/" + std::to_string(a2)); d = 400; break; } }
        check(true, "label == the bars' ratio over a grid"); }
    // record lines
    {   blb::Engine::Shown sh; sh.on = true; sh.c.name = "G|7630"; sh.c.type = blb::L_MQ; sh.c.ds = 0; sh.c.arrived = true; sh.c.anchor = 500; sh.k = 60; sh.c.lo = 1; sh.c.hi = 9;
        sh.r[0] = rd(blb::W_STACK, 4.25, blb::GREEN); sh.r[1] = rd(blb::W_STACK, 3.29, blb::GREEN); sh.r[2] = rd(blb::W_NORMAL, 0.96, blb::GRAY);
        const std::string ln = blb::stateLine(560, "09:23:23", sh, blb::boxText(sh, blb::Params()));
        check(ln.compare(0, 26, "S|560|09:23:23|G 7630|mq|s") == 0 && ln.find("|1|4.25|g|1|3.29|g|5|0.96|n|") != std::string::npos, "S line: " + ln);
        blb::Engine::Shown off; check(blb::stateLine(1, "x", off, blb::BoxText()) == "S|1|x|off" && blb::stateKey(off) == "off", "off line"); }
    check(std::string(blb::VERSION) == "1.1.0" && std::string(bf::VERSION).compare(0, 6, "1.35.0") == 0 && std::string(ofset::BF_VERSION) == "1.35.0", "versions: BookLevelBox 1.1.0, bf::VERSION / ofset::BF_VERSION 1.35.0 (pins moved with the 1.35.0 bump)");
    std::printf("blb logic: %d passed, %d failed\n", P, F); return F ? 1 : 0;
}
