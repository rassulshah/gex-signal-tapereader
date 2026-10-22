// test_attack_126.cpp - AttackLogic.h 1.0.0 + OrderFlowFeed.h 1.13.0 (lsBidAskStats 1.26.0, 2026-10-21, Rassul 20:11-20:23 CT, attacker-mockup-v2.png):
// K / S rules (tier by session, >= 3 prices, biggest of the leg, the 5-candle extreme, the initiative veto, K replacing L), the badge
// (colours by meaning, at most 4 letters, no "?"), the hover (every letter incl. the cut ones, the combination line with a Defense letter),
// the record lines (round trip, strict), the other panes' records (G / A / H / Q / W / TR), the "?"-free read line.
//   g++ -std=c++17 -Wall -Wextra -Werror -I.. test_attack_126.cpp -o /tmp/test_attack && /tmp/test_attack
#include "AttackLogic.h"
#include "EffortReward.h"
#include <cstdio>
#include <fstream>
static int f = 0, p = 0;
#define CHECK(c, m) do { if (c) ++p; else { ++f; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static off::Ev fill(int restSide, off::Tick px, long long size, long long id, long long aggr, long long sec) {
    off::Ev e; e.action = off::FILL; e.side = restSide; e.px = px; e.size = size; e.id = id; e.aggr = aggr; e.stamp = sec; e.arrMs = sec * 1000; return e;
}
static std::string segs(const std::vector<dt::BadgeSeg>& v) { std::string s; for (size_t k = 0; k < v.size(); ++k) s += (k ? "/" : "") + v[k].s + (v[k].colourKey == atk::HIGH ? "r" : "g"); return s; }
int main() {
    CHECK(std::string(atk::VERSION) == "1.1.0" && std::string(off::VERSION).compare(0, 6, "1.14.0") == 0, "AttackLogic 1.1.0, OrderFlowFeed 1.14.0");
    // ---------------- OrderFlowFeed 1.13.0: the biggest order and the biggest sweep per side, its first second and prices
    off::Params P; P.sweepLevels = 3;
    {   off::Engine e; off::Acc a;
        // buyers (offers hit = ASK): order 7 = 173 lots over 39 prices from second 1018; order 8 = 200 lots at 2 prices (not a sweep); order 9 = 60 lots over 3 prices
        for (int k = 0; k < 39; ++k) e.apply(a, fill(off::ASK, 28000 + k, k == 38 ? 173 - 38 * 4 : 4, 1000 + k, 7, 1018 + (k > 20 ? 1 : 0)));
        e.apply(a, fill(off::ASK, 28050, 100, 2000, 8, 1030)); e.apply(a, fill(off::ASK, 28051, 100, 2001, 8, 1031));
        for (int k = 0; k < 3; ++k) e.apply(a, fill(off::ASK, 27990 + k, 20, 3000 + k, 9, 1005));
        // sellers (bids hit = BID): order 11 = 30 lots at 2 prices
        e.apply(a, fill(off::BID, 27980, 15, 4000, 11, 1040)); e.apply(a, fill(off::BID, 27979, 15, 4001, 11, 1041));
        const off::Rec r = e.close(a, false, P);
        CHECK(r.big[off::ASK].lots == 200 && r.big[off::ASK].sec == 1030 && r.big[off::ASK].nPx == 2, "biggest buy order: the 200-lot one (2 prices), first fill second 1030");
        CHECK(r.sweep[off::ASK].lots == 173 && r.sweep[off::ASK].nPx == 39 && r.sweep[off::ASK].lo == 28000 && r.sweep[off::ASK].hi == 28038 && r.sweep[off::ASK].sec == 1018,
              "biggest buy SWEEP (>= 3 prices, by lots): 173 lots, 39 prices 28000-28038, from second 1018 (the 200-lot 2-price order is not a sweep)");
        CHECK(r.sweep[off::BID].lots == 0 && r.big[off::BID].lots == 30, "sellers: a 2-price order is no sweep; their biggest order 30");
        CHECK(a.hitSec[off::ASK].size() >= 5 && a.hitSec[off::ASK].at(1005) == 60, "per-second hits kept (1005: 60 lots)");
        std::vector<long long> v; off::recNums(r, v); off::Rec back; CHECK(off::recFrom(v, 0, back) && back.big[off::ASK].lots == 0 && (int)v.size() == off::REC_NUMS, "the record line is unchanged (53 numbers): Top fields are not in it");
    }
    // ---------------- the pace (hover only)
    {   off::Acc a; a.tFirst = 1000; a.tHi = 1060; a.tLast = 1150; a.hitC[off::ASK] = 340; a.cumHi[off::ASK] = 300;
        a.hitSec[off::ASK][1005] = 50; a.hitSec[off::ASK][1040] = 150; a.hitSec[off::ASK][1060] = 100;
        const atk::Pace pc = atk::paceOf(a, atk::HIGH);
        CHECK(pc.e100 == 9, "E: 40 lots in 90 s after the high vs 300 in 61 s before = 0.09x");
        CHECK(pc.into100 == 500, "speeding up: 2nd half 250 vs 1st half 50 = 5.0x");
        CHECK(atk::paceRow(pc, atk::HIGH) == "Hover only:  E fading 0.1x after the high  |  speeding up 5.0x into it (2nd vs 1st half)", "the hover-only pace line");
        off::Acc b = a; b.tLast = 1070; CHECK(atk::paceOf(b, atk::HIGH).e100 == -1, "E needs >= 20 s after the extreme");
        CHECK(atk::paceOf(a, atk::LOW).e100 == -1, "no low fill time: n/a");
    }
    // ---------------- the 5-candle extreme
    {   bool h = false, l = false;
        CHECK(atk::extremeOf({10, 11, 12, 11, 10, 12}, {5, 5, 5, 5, 5, 6}, h, l) && h && !l, "high = the 5 before (a tie counts), not a low");
        CHECK(!atk::extremeOf({10, 11, 13, 11, 10, 12}, {5, 5, 5, 5, 5, 6}, h, l) && !h && !l, "a higher high among the 5 before: no extreme");
        CHECK(!atk::extremeOf({10, 11, 12}, {5, 5, 5}, h, l), "fewer than 6 candles: not decided");
    }
    // ---------------- K: >= S10 on the attacker side, tier frozen (by session channel), initiative veto
    const long long RTH[3] = {92, 122, 162}, ON[3] = {55, 73, 97};
    {   off::Rec r; r.big[off::ASK].lots = 122; r.big[off::ASK].sec = 1018; r.big[off::BID].lots = 500;
        atk::KRead k; const atk::Pace pc;
        CHECK(atk::decideK(r, 777, atk::HIGH, true, RTH, llp::CH_RTH, false, pc, k) && k.K && k.lots == 122 && k.ch == llp::CH_RTH, "K at the high: one buy order of 122 = RTH S10 (boundary)");
        r.big[off::ASK].lots = 121; CHECK(atk::decideK(r, 777, atk::HIGH, true, RTH, llp::CH_RTH, false, pc, k) && !k.K, "121 < S10 122: no K (the sellers' 500 do not count at a high)");
        CHECK(atk::decideK(r, 777, atk::HIGH, true, ON, llp::CH_ON, false, pc, k) && k.K && k.tier[1] == 73, "the same 121 lots overnight (S10 73): K - the tier is the session's");
        r.big[off::ASK].lots = 300; CHECK(atk::decideK(r, 777, atk::HIGH, true, RTH, llp::CH_RTH, true, pc, k) && !k.K && k.init, "initiative candle: no K (veto), the line still written for the nightly");
        CHECK(!atk::decideK(r, 777, atk::HIGH, false, RTH, llp::CH_RTH, false, pc, k), "tiers still learning: not decided (never guessed)");
        off::Rec rp = r; rp.partial = true; CHECK(!atk::decideK(rp, 777, atk::HIGH, true, RTH, llp::CH_RTH, false, pc, k), "a partial candle: not decided");
        CHECK(atk::decideK(r, 777, atk::LOW, true, RTH, llp::CH_RTH, false, pc, k) && k.K && k.lots == 500, "at a low the sellers' (bids hit) biggest order: 500 -> K");
        // initiative (DefenseTurn rule): delta with the move >= 2x the median |delta| of the prior, close in the outer quarter
        dt::Candle x; x.o = 100; x.h = 120; x.l = 99; x.c = 119; x.aggr[1] = 400; x.aggr[0] = 100;
        CHECK(dt::initiativeOf(x, dt::HIGH, {50, -40, 60, 30, -20, 45}), "a bullish initiative candle into the high (delta 300 vs median 42, close at the top)");
        x.c = 105; CHECK(!dt::initiativeOf(x, dt::HIGH, {50, -40, 60, 30, -20, 45}), "closed back in the middle: not initiative");
    }
    // ---------------- S: >= 3 prices, biggest of the leg (each of the 10 candles before), the leg must be known
    {   off::Rec r; r.sweep[off::ASK].lots = 173; r.sweep[off::ASK].nPx = 39; r.sweep[off::ASK].lo = 120076; r.sweep[off::ASK].hi = 120114; r.sweep[off::ASK].sec = 1018;
        std::vector<atk::AS> leg(10); for (int k = 0; k < 10; ++k) { leg[(size_t)k].t = 100 + k; leg[(size_t)k].sw[off::ASK] = k == 4 ? 60 : 20; leg[(size_t)k].sw[off::BID] = 500; }
        std::vector<const atk::AS*> pv; for (auto& a : leg) pv.push_back(&a);
        long long mx = 0; CHECK(atk::legMax(pv, off::ASK, mx) && mx == 60, "the leg's biggest buy sweep: 60 lots (the sellers' 500 are the other side)");
        atk::SRead w; const atk::Pace pc;
        CHECK(atk::decideS(r, 777, atk::HIGH, true, mx, 0.25, pc, w) && w.S && w.prior == 60, "S: 173 lots > 60, at the high");
        r.sweep[off::ASK].lots = 60; CHECK(atk::decideS(r, 777, atk::HIGH, true, mx, 0.25, pc, w) && !w.S, "a tie with the leg's biggest: no S (must be bigger)");
        r.sweep[off::ASK].lots = 0; CHECK(atk::decideS(r, 777, atk::HIGH, true, 0, 0.25, pc, w) && !w.S, "no sweep of >= 3 prices: no S");
        pv[3] = nullptr; CHECK(!atk::legMax(pv, off::ASK, mx), "a candle of the leg without the order feed: the leg is unknown");
        CHECK(!atk::decideS(r, 777, atk::HIGH, false, 0, 0.25, pc, w), "unknown leg: S not decided (never guessed)");
        pv.pop_back(); CHECK(!atk::legMax(pv, off::ASK, mx), "fewer than 10 candles before: unknown");
        r.sweep[off::ASK].lots = 1; std::vector<atk::AS> z(10); std::vector<const atk::AS*> pz; for (auto& a : z) pz.push_back(&a);
        CHECK(atk::legMax(pz, off::ASK, mx) && mx == 0 && atk::decideS(r, 777, atk::HIGH, true, mx, 0.25, pc, w) && w.S, "the first sweep of the leg is the biggest");
    }
    // ---------------- L: >= S20 but < S10; K replaces L
    {   atk::AS a; a.tier[0] = 92; a.tier[1] = 122; a.tier[2] = 162; a.big[off::BID] = 92; a.big[off::ASK] = 122;
        CHECK(atk::lOf(a, off::BID) && !atk::lOf(a, off::ASK), "L: 92 (= S20) yes; 122 (= S10) is no longer L (K at an extreme, nothing elsewhere)");
        a.big[off::BID] = 91; CHECK(!atk::lOf(a, off::BID), "91 < S20: no L");
        atk::AS u; u.big[off::BID] = 500; CHECK(!atk::lOf(u, off::BID), "tiers unknown: no L");
        CHECK(atk::lColour(off::ASK, false, false) == atk::LOW && atk::lColour(off::ASK, true, false) == atk::HIGH && atk::lColour(off::BID, false, false) == atk::HIGH && atk::lColour(off::BID, false, true) == atk::LOW,
              "L by meaning: buyers green, buyers into a high red; sellers red, sellers into a low green");
    }
    // ---------------- the badge: colours by meaning, at most 4 letters, no "?"
    {   atk::BadgeIn in; in.S = atk::HIGH; in.K = atk::HIGH; in.G = atk::HIGH; in.Tr = atk::HIGH; in.R = atk::HIGH; in.L = atk::LOW;
        std::vector<std::string> cut; const std::vector<dt::BadgeSeg> s = atk::badgeSegs(in, &cut);
        CHECK(segs(s) == "S|r/K|r/G|r/Tr" + std::string("r"), ("at a high everything red, order S K G Tr, 4 letters: " + segs(s)).c_str());
        CHECK(cut.size() == 2 && cut[0] == "R" && cut[1] == "L", "R and L cut from the badge (named in the hover)");
        atk::BadgeIn lo; lo.S = atk::LOW; lo.K = atk::LOW; lo.G = atk::LOW;
        CHECK(segs(atk::badgeSegs(lo)) == "S|g/K|g/Gg", "sellers attacking / giving up at a low: green");
        atk::BadgeIn r1; r1.R = atk::LOW; r1.L = atk::HIGH; CHECK(segs(atk::badgeSegs(r1)) == "R|g/Lr", "R green (sellers trapped below), L red (a seller)");
        CHECK(atk::badgeSegs(atk::BadgeIn()).empty(), "no letter: no badge");
        bool q = false; for (auto& x : s) q = q || x.s.find('?') != std::string::npos; CHECK(!q, "no '?' on the badge");
    }
    // ---------------- the hover: every letter (incl. the cut ones), the record numbers, the combination with a Defense letter
    {   atk::KRead k; k.t = 777; k.side = atk::HIGH; k.K = true; k.lots = 173; k.sec = 1018; k.tier[0] = 92; k.tier[1] = 122; k.tier[2] = 162;
        atk::SRead w; w.t = 777; w.side = atk::HIGH; w.S = true; w.lots = 173; w.nPx = 39; w.lo = 120076; w.hi = 120114; w.prior = 60; w.tick = 0.25; w.p.e100 = 20; w.p.into100 = 230;
        dt::GiveUp g; g.t = 777; g.side = dt::HIGH; g.G = true; g.pull = 19226; g.add = 18673; g.secs = 60; g.x = 3.6; g.pace = 0.2; g.normal = 32.56;
        std::vector<dt::HoverBlock> B;
        B.push_back(atk::hoverS(w, "11:39:18 AM", "NQ", {"K", "G"}, {"A"}, 3));
        B.push_back(atk::hoverK(k, "11:39:18 AM", "NQ", {"S", "G"}, {"A"}, 2));
        B.push_back(atk::hoverG(g, {"S", "K", "E"}, {"A"}, 4));
        B.push_back(atk::hoverTr(true, 640, 2.9, {"K"}, {}, 5));
        B.push_back(atk::hoverR(1, "PDH", 6, 1, "3.1x", 1));
        const long long T3[3] = {92, 122, 162}; B.push_back(atk::hoverL(off::ASK, 100, T3, "11:30:02 AM", 7));
        const std::string t = atk::hoverText("S|K|G|Tr", "11:39 AM", atk::HIGH, {"R", "L"}, B, atk::paceRow(w.p, atk::HIGH));
        auto has = [&](const std::string& x) { std::string j; for (size_t i = 0; i < t.size(); ++i) j += t[i]; std::string flat; for (size_t i = 0; i < j.size(); ++i) { if (j[i] == '\n' && j.compare(i + 1, 5, "     ") == 0) { flat += ' '; i += 5; } else flat += j[i]; } return flat.find(x) != std::string::npos; };
        CHECK(has("Attack S|K|G|Tr   11:39 AM   (bearish = red)  (+ R, L: no room on the badge)"), "header: badge, time, meaning, the letters cut");
        CHECK(has("K  Big aggressor - did one big buyer hit the high?") && has("One buy order of 173 lots at 11:39:18 AM (Big Lot S5 = 162)"), "K: the question + the record's numbers (S5 reached)");
        CHECK(has("S  Sweep of the leg - did one order rip through several prices?") && has("One buy order took 39 prices (30,019 -> 30,028.50, 173 lots at 11:39:18 AM), 2.9x the biggest sweep of the 10 candles before (60 lots)"), "S: prices, lots, the leg ratio");
        CHECK(has("with Defense A: K + A held 61% vs 47% (n 49, learning)") && has("with Defense A: S + A held 55% vs 51% (n 116, learning)") && has("with Defense A: G + A held 57% vs 48% (n 114, learning)"), "the combination line with the Defense A (from attacker_ev.json)");
        CHECK(has("with G: K + G held 57% vs 43% (n 88, learning)") && has("with E: G + E held 56% vs 47% (n 283, learning)") && has("with K: K + Tr held 65% vs 49% (n 60, learning)"), "the attacker-letter combinations");
        CHECK(has("Tested: held 51% vs 44% matched (n 628, 7 markets); NQ +11.1 pts (n 73)") && has("Tested: held 49% vs 44% matched (n 2,008, 7 markets); NQ +5.8 pts (n 460)"), "K / S tested rows with the market's lift");
        CHECK(has("G  Giving up") && has("Tr  Trapped") && has("R  Stop run - did buyers get trapped above PDH?") && has("L  Large order - did one big buyer hit this candle?"), "every letter has its block, also R and L cut from the badge");
        CHECK(has("Example:") && has("Use:") && has("Status: still learning, n = 2 (this chart; scored nightly)"), "his format: Use, Example, Status still learning n");
        CHECK(has("Hover only:  E fading 0.2x after the high  |  speeding up 2.3x into it (2nd vs 1st half)"), "E / speeding up: hover only");
        bool q = false; std::string ln; for (size_t i = 0; i <= t.size(); ++i) { if (i < t.size() && t[i] != '\n') { ln += t[i]; continue; } if (ln.find('?') != std::string::npos && ln.find(" - ") == std::string::npos && ln.compare(0, 5, "     ") != 0) q = true; ln.clear(); }
        CHECK(!q, "no '?' in the hover except the questions themselves");
        size_t maxLen = 0; { std::string x; for (size_t i = 0; i <= t.size(); ++i) { if (i < t.size() && t[i] != '\n') { x += t[i]; continue; } maxLen = std::max(maxLen, x.size()); x.clear(); } }
        CHECK(maxLen <= 120, ("rows wrapped (longest " + std::to_string(maxLen) + ")").c_str());
        const std::string c1 = atk::compact(t, 1), c2 = atk::compact(t, 2);
        CHECK(c1.find("Use:") == std::string::npos && c1.find("Example:") == std::string::npos && c1.find("with Defense A: K + A") != std::string::npos && c1.find("Status:") != std::string::npos, "fit level 1: Use / Example dropped, combinations kept");
        CHECK(c2.find("Status:") == std::string::npos && c2.find("Tested:") == std::string::npos && c2.find("One buy order of 173 lots") != std::string::npos, "fit level 2: Tested / Status dropped, the numbers kept");
        CHECK(atk::scoreText("Q+Z").empty() && atk::pairKey("A", "K") == "K+A" && atk::pairKey("H", "K").empty(), "the table: pair keys either way, unknown pairs empty");
        dt::HoverBlock hb = atk::hoverK(k, "", "XX", {}, {"H"}, 1); bool found = false; for (auto& r : hb.rows) found = found || r == "   with Defense H: no combined score yet (learning)";
        CHECK(found, "a Defense letter with no score yet: says so");
    }
    // ---------------- the record lines: round trip, strict
    {   atk::KRead k; k.t = 1791900000; k.side = 1; k.K = true; k.lots = 173; k.sec = 1791899958; k.tier[0] = 92; k.tier[1] = 122; k.tier[2] = 162; k.ch = 1; k.p.e100 = 20; k.p.into100 = -1;
        atk::KRead k2; CHECK(atk::parseK(atk::kLine(k), k2) && k2.K && k2.lots == 173 && k2.sec == k.sec && k2.tier[1] == 122 && k2.ch == 1 && k2.p.e100 == 20 && k2.p.into100 == -1, "K line round trip");
        CHECK(atk::kLine(k) == "K|1791900000|1|1|173|1791899958|92|122|162|1|0|20|-1", "K line format");
        k.lots = 100; CHECK(!atk::parseK(atk::kLine(k), k2), "a K line that breaks its own rule (100 < S10) is rejected");
        CHECK(!atk::parseK("K|1|1|1|173", k2) && !atk::parseK("K|x|1|1|173|5|92|122|162|1|0|20|-1", k2) && !atk::parseK("K|5|3|1|173|5|92|122|162|1|0|20|-1", k2), "malformed K lines rejected");
        atk::SRead w; w.t = 1791900000; w.side = 0; w.S = true; w.lots = 80; w.nPx = 4; w.lo = 100; w.hi = 103; w.sec = 5; w.prior = 30; w.tick = 0.25;
        atk::SRead w2; CHECK(atk::parseS(atk::sLine(w), w2) && w2.S && w2.nPx == 4 && w2.hi == 103 && w2.tick == 0.25 && w2.side == 0, "S line round trip");
        w.prior = 90; CHECK(!atk::parseS(atk::sLine(w), w2), "an S line that breaks its own rule is rejected");
        atk::AS a; a.t = 9; a.sw[0] = 1; a.sw[1] = 2; a.big[0] = 3; a.big[1] = 4; a.sec[0] = -1; a.sec[1] = 7; a.tier[0] = 92; a.tier[1] = 122; a.tier[2] = 162;
        atk::AS a2; CHECK(atk::parseAS(atk::asLine(a), a2) && a2.big[1] == 4 && a2.sec[0] == -1 && a2.tier[2] == 162, "AS line round trip");
        CHECK(!atk::parseAS("AS|9|1|2|3|4|-1|7|92|122", a2) && !atk::parseAS("AS|9|1|2|3|-4|-1|7|92|122|162", a2), "malformed AS lines rejected");
        CHECK(!atk::parseS("SR|1|2", w2) && !atk::parseK("KX|1", k2), "other tags never parse as K / S");
    }
    // ---------------- the other panes' records
    {   atk::Others o;
        dt::GiveUp g; g.t = 500; g.side = dt::HIGH; g.G = true; g.pull = 4000; g.add = 300; g.mov = 100; g.secs = 90; g.x = 2.4; g.pace = 0.1; g.normal = 17.8;
        o.turnsLine(dt::gLineOf(g));
        dt::Turn t; t.t = 500; t.side = dt::HIGH; t.A = true; t.pivotPx = 100; t.linePx = 100; o.turnsLine(dt::lineOf('T', t, 0.25));
        dt::Counter c; c.t = 400; c.side = dt::LOW; c.H = true; c.lots = 10; c.attack = 5; c.at = 500; o.turnsLine(dt::hLineOf(c));
        o.turnsLine("W|500|0|27980|300|0|40|27990|W|1000|PDL|0.25");
        o.turnsLine("garbage|line");
        CHECK(o.G.count(std::make_pair((int)dt::HIGH, 500LL)) && o.G[std::make_pair((int)dt::HIGH, 500LL)].G, "G line read (lsBookFlow's record, unchanged)");
        CHECK(o.defenseAt(500, dt::HIGH).size() == 2, "(1.1.0) the T line's own A no longer makes an A: H, W only");
        o.SA.insert(std::make_pair((int)dt::HIGH, 500LL));                 // (1.1.0) the SHARED A on that candle (filled by the host from the record)
        const std::vector<std::string> d = o.defenseAt(500, dt::HIGH);
        CHECK(d.size() == 3 && d[0] == "A" && d[1] == "H" && d[2] == "W", "the Defense letters on the candle: A (the shared A, same extreme), H, W");
        CHECK(o.defenseAt(500, dt::LOW).size() == 2, "A at the other extreme does not combine");
        trapl::Rec tr; tr.id = 0; tr.top = true; tr.barT = 5; tr.barRaw = 500; tr.lo = 1; tr.hi = 2; tr.tick = 0.25; tr.lots = 640; tr.med = 220; tr.x = 2.9; tr.version = "2.21.1"; tr.root = "ES";
        o.trLine(trapl::line(tr)); trapl::Rec t2 = tr; t2.lots = 9; o.trLine(trapl::line(t2)); o.trLine("TR|bad");
        CHECK(o.Tr.size() == 1 && o.Tr[std::make_pair((int)dt::HIGH, 500LL)].lots == 640 && o.bad == 1, "TR read: the first line per candle + side wins, a bad line counted");
        // tailRead: whole lines only, from where it stopped; a rewritten (shorter) file is read again
        const std::string fp = "/tmp/attack126_tail.txt"; { std::ofstream q(fp.c_str(), std::ios::binary); q << "a\r\nb\nc"; }
        long long off0 = 0; std::vector<std::string> got; auto fn = [&](const std::string& l) { got.push_back(l); };
        CHECK(atk::tailRead(fp, off0, fn) && got.size() == 2 && got[0] == "a" && got[1] == "b", "whole lines only (CRLF ok), the partial 'c' waits");
        { std::ofstream q(fp.c_str(), std::ios::binary | std::ios::app); q << "\nd\n"; } got.clear();
        CHECK(atk::tailRead(fp, off0, fn) && got.size() == 2 && got[0] == "c" && got[1] == "d", "appended lines from where it stopped");
        CHECK(!atk::tailRead(fp, off0, fn), "nothing new: nothing read");
        { std::ofstream q(fp.c_str(), std::ios::binary); q << "x\n"; } got.clear();
        CHECK(atk::tailRead(fp, off0, fn) && got.size() == 1 && got[0] == "x", "rewritten shorter: read again from the start");
        long long o2 = 0; CHECK(!atk::tailRead("/tmp/no-such-attack-file", o2, fn), "missing file: nothing");
        std::remove(fp.c_str());
    }
    // ---------------- no "?" on the pane's message line
    {   const er::Side sd[2] = { er::Side(), er::Side() }; (void)sd;
        br::Read r; r.head = "New high: Sellers hitting 4.2x - strong resistance coming in ?"; r.opt.push_back(", fast (2.1x) ?");
        atk::noQ(r);
        CHECK(r.head == "New high: Sellers hitting 4.2x - strong resistance coming in" && r.opt[0] == ", fast (2.1x)", "the read line without '?'");
        CHECK(atk::noQ("absorbed ?") == "absorbed" && atk::noQ("a?") == "a" && atk::noQ("no mark") == "no mark", "noQ");
    }
    std::printf("AttackLogic 1.0.0: %d checks, %d failed\n", p, f);
    return f ? 1 : 0;
}
