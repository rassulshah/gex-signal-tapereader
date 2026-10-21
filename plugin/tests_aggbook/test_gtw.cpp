// test_gtw.cpp - DefenseTurn 1.7.0 (lsBookFlow 1.19.0, 2026-10-21, Rassul approved 14:20-14:22 CT): G (Top Attacker #2, attackers giving up),
// Tr hover (Middle Attacker #1, the Delta Profile's TR record), W (Top Defender #2, defenders withdrew at a key level), the 4-letter badge.
//   g++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation -I.. test_gtw.cpp -o /tmp/test_gtw && /tmp/test_gtw
#include "DefenseTurn.h"
#include "TrapLogic.h"
#include <cstdio>
#include <cmath>
static int f = 0, p = 0;
#define CHECK(c, m) do { if (c) ++p; else { ++f; printf("FAIL %d %s\n", __LINE__, m); } } while (0)
// a candle at a HIGH (side HIGH): before the extreme 300 lots lifted in 60 s (5/s); after it `after` lots in `secs` s; the buyers' bids after the
// high: pulled / added / moved
static dt::Candle hi(long long after, long long secs, long long pull, long long add, long long mov) {
    dt::Candle x; x.t = 500; x.ok = true; x.o = 1000; x.h = 1010; x.l = 998; x.c = 1003;   // close in the middle (not an initiative close)
    x.before[dt::HIGH] = 300; x.secsBefore[dt::HIGH] = 60; x.after[dt::HIGH] = after; x.secsAfter[dt::HIGH] = secs;
    x.apull[dt::HIGH] = pull; x.aadd[dt::HIGH] = add; x.amov[dt::HIGH] = mov; x.gOk[dt::HIGH] = true;
    x.aggr[1] = 400; x.aggr[0] = 380;
    return x;
}
static std::string segs(const std::vector<dt::BadgeSeg>& v) { std::string s; for (size_t k = 0; k < v.size(); ++k) s += (k ? "/" : "") + v[k].s + (v[k].colourKey == dt::HIGH ? "r" : "g"); return s; }
int main() {
    CHECK(std::string(dt::VERSION) == "1.7.0", "DefenseTurn 1.7.0");
    // ---------------- G
    {   const double N = 10.0;                                             // normal 10 lots / s
        dt::GiveUp g;
        // 90 s after the high: 2,000 bid lots pulled net (22.2/s = 2.2x), 300 added, attack 40 lots in 90 s (0.44/s vs 5/s before: died)
        CHECK(dt::giveUpDecide(hi(40, 90, 2000, 300, 0), dt::HIGH, N, false, g) && g.G && std::fabs(g.x - 2.222) < 0.01 && g.pace < 0.5, "G fires: 2.2x normal, pulled > added, attack died");
        CHECK(dt::attackersText(g) == "Pulling 2.2x", "box row Attackers: Pulling 2.2x");
        CHECK(dt::giveUpDecide(hi(40, 90, 1700, 300, 0), dt::HIGH, N, false, g) && !g.G && dt::attackersText(g) == "Still in 1.9x", "1.9x normal: no G (Still in 1.9x)");
        CHECK(dt::giveUpDecide(hi(40, 90, 2000, 2100, 0), dt::HIGH, N, false, g) && !g.G, "added more than pulled: no G (not leaving)");
        CHECK(dt::giveUpDecide(hi(300, 90, 2000, 300, 0), dt::HIGH, N, false, g) && !g.G, "attack pace after the high 3.3/s vs 5/s (0.67x): not died, no G");
        CHECK(dt::giveUpDecide(hi(40, 90, 2000, 300, 0), dt::HIGH, N, true, g) && !g.G && g.init, "initiative candle: vetoed (no G), recorded init 1");
        CHECK(dt::giveUpDecide(hi(40, 90, 2600, 300, 900), dt::HIGH, N, false, g) && !g.G && std::fabs(g.x - 1.889) < 0.01, "2,600 pulled but 900 of them MOVED: 1,700 net = 1.9x -> no G (pulls counted net of moves)");
        CHECK(dt::giveUpDecide(hi(40, 90, 2600, 300, 800), dt::HIGH, N, false, g) && g.G && std::fabs(g.x - 2.0) < 0.01, "800 moved: 1,800 net = exactly 2.0x -> G (>= 2.0)");
        CHECK(dt::giveUpDecide(hi(40, 90, 2600, 300, 200), dt::HIGH, N, false, g) && g.G && g.mov == 200, "200 moved: 2,400 net = 2.7x -> G");
        CHECK(!dt::giveUpDecide(hi(5, 15, 2000, 300, 0), dt::HIGH, N, false, g), "15 s after the extreme (< 20): not decided (never guessed)");
        dt::Candle nf = hi(40, 90, 2000, 300, 0); nf.gOk[dt::HIGH] = false;
        CHECK(!dt::giveUpDecide(nf, dt::HIGH, N, false, g), "no order feed for the candle: not decided");
        CHECK(!dt::giveUpDecide(hi(40, 90, 2000, 300, 0), dt::HIGH, 0.0, false, g), "no normal (unknown market): not decided");
        CHECK(dt::giveUpNormal("ES", dt::HIGH) > 17 && dt::giveUpNormal("XX", dt::LOW) == 0, "the market normals (ES ~17.8 lots/s; unknown 0)");
        // the initiative rule: delta with the move >= 2x the median |delta| of the 20 before and the close in the outer quarter
        std::vector<long long> pr(10, 100);
        dt::Candle x = hi(40, 90, 2000, 300, 0); x.aggr[1] = 700; x.aggr[0] = 400; x.c = 1009;   // +300 >= 2 x 100, close at 11/12 of the range
        CHECK(dt::initiativeOf(x, dt::HIGH, pr), "initiative: delta +300 >= 2x 100, close in the top quarter");
        x.c = 1004; CHECK(!dt::initiativeOf(x, dt::HIGH, pr), "close in the middle: not initiative");
        x.c = 1009; x.aggr[1] = 550; CHECK(!dt::initiativeOf(x, dt::HIGH, pr), "delta +150 < 200: not initiative");
        CHECK(!dt::initiativeOf(x, dt::HIGH, std::vector<long long>(3, 100)), "fewer than 5 earlier candles: no veto");
        // a LOW: sellers' offers after the low, sellers hitting bids before / after
        dt::Candle lo; lo.t = 600; lo.ok = true; lo.o = 1005; lo.h = 1008; lo.l = 990; lo.c = 997;
        lo.before[dt::LOW] = 600; lo.secsBefore[dt::LOW] = 60; lo.after[dt::LOW] = 30; lo.secsAfter[dt::LOW] = 60; lo.apull[dt::LOW] = 1500; lo.aadd[dt::LOW] = 100; lo.gOk[dt::LOW] = true;
        CHECK(dt::giveUpDecide(lo, dt::LOW, N, false, g) && g.G && g.side == dt::LOW, "LOW: the sellers pulled their offers 2.5x, attack died -> G");
        dt::GiveUp r; CHECK(dt::parseG(dt::gLineOf(g), r) && r.G && r.side == dt::LOW && r.t == 600 && r.pull == 1500 && std::fabs(r.x - g.x) < 0.01 && std::fabs(r.normal - 10) < 1e-9, "G line round trip (a restart redraws the same, never re-decided)");
        CHECK(!dt::parseG("G|1|0|2|1|1|1|30|100|50|0|1000", r) && !dt::parseG("G|1|0|1|1|1", r) && !dt::parseG("G|1|3|1|1|1|1|30|100|50|0|1000", r) && !dt::parseG("G|1|0|1|1|1|1|0|100|50|0|1000", r) && !dt::parseG("W|1", r), "garbled G lines refused");
    }
    // ---------------- Tr (hover only; the decision is the Delta Profile's)
    {   CHECK(dt::trapHover(false, 640, 2.9) == "Sellers trapped: 640 lots (2.9x) below the old low", "Tr hover at a low");
        CHECK(dt::trapHover(true, 1210, 1.4) == "Buyers trapped: 1210 lots (1.4x) above the old high", "Tr hover at a high");
        trapl::Rec t; t.id = 3; t.top = false; t.barT = 100; t.barRaw = 123456; t.lo = 10; t.hi = 12; t.tick = 0.25; t.lots = 640; t.med = 220; t.x = 2.9; t.version = "2.21.1"; t.root = "ESZ6";
        trapl::Rec q; CHECK(trapl::parse(trapl::line(t), q) && q.barRaw == 123456 && !q.top && q.lots == 640, "a TR line (the Delta Profile's own writer) reads back: candle key = bar_raw");
    }
    // ---------------- W
    {   CHECK(dt::wKeyName("PDL") && dt::wKeyName("PFH/PDH") && dt::wKeyName("LonLO") && !dt::wKeyName("VAH") && !dt::wKeyName("Open") && !dt::wKeyName("PWH"), "key levels: PDH/PDL/PFH/PFL/ONH/ONL/LonHI/LonLO only");
        dt::LevelWatch w; w.tol = 2; w.rearm = 6;
        std::vector<dt::WLevel> L(1); L[0].name = "PDL"; L[0].px = 1000; w.setLevels(L);
        w.onBook(2, 1, 1010, 1, 1000, 77);                                 // a trade at 1010 (away above): armed for a support test
        CHECK(w.open.empty() && w.armed[0] == 1, "away above the level: armed, no touch");
        w.onBook(0, 0, 999, 50, 1001, 77);                                 // bids added before the touch: not counted (no read open)
        w.onBook(2, 0, 1002, 5, 1010, 78);                                 // traded within 2 ticks: the TOUCH (support), candle 78
        CHECK(w.open.size() == 1 && w.open[0].side == dt::LOW && w.open[0].key == 78 && w.open[0].t0 == 1010 && w.open[0].filled == 0, "touch from above = a support test, on the touch candle; the touching trade at 1002 is outside L-2..L+1");
        w.onBook(1, 0, 1000, 300, 1020, 78);                               // 300 bid lots pulled at the level
        w.onBook(1, 0, 998, 40, 1025, 78);                                 // L-2: in the zone
        w.onBook(1, 0, 997, 500, 1025, 78);                                // L-3: outside
        w.onBook(1, 1, 1000, 900, 1030, 78);                               // offers: the wrong side
        w.onBook(0, 0, 1001, 30, 1040, 79);                                // 30 added at L+1
        w.onBook(2, 0, 1000, 40, 1050, 79);                                // 40 traded at the level
        w.step(1069); CHECK(w.done.empty(), "59 s: still measuring");
        w.onBook(1, 0, 1000, 999, 1070, 79);                               // 60 s after the touch: outside the window
        CHECK(w.open[0].pulled == 340 && w.open[0].added == 30 && w.open[0].filled == 40, "first minute in L-2..L+1 on the bids: pulled 340, added 30, traded 40 (wrong side / outside / late not counted)");
        w.step(1070); CHECK(w.done.size() == 1 && w.open.empty() && w.done[0].pxEnd == 1000, "60 s: finished, price then = the last trade (1000)");
        dt::WRead r = w.done[0]; dt::wDecide(r, "ES");
        CHECK(r.W && !r.F && !r.bounced && dt::wDrawn(r) && dt::atLevelText(r) == "Pulled 89%", "net pulled 310 >= 2 x 40 traded: W, drawn at a support, 'Pulled 89%' (310 / 350)");
        dt::WRead r2 = r; r2.pxEnd = 1001; dt::wDecide(r2, "ES"); CHECK(r2.bounced && r2.W && dt::wDrawn(r2), "already 1 tick back off the level: recorded b, still drawn (the veto tested worse, W_VETO_BOUNCE off)");
        dt::WRead rs = r; rs.side = dt::HIGH; dt::wDecide(rs, "ES"); CHECK(rs.W && !dt::wDrawn(rs), "a resistance W: recorded, never drawn");
        dt::WRead fl = r; fl.pulled = 100; fl.added = 0; fl.filled = 500; dt::wDecide(fl, "ES");
        CHECK(!fl.W && fl.F && dt::atLevelText(fl) == "Filled 1.2x", "500 traded >= the ES support median 426 and >= net pulled: Filled 1.2x (grey)");
        dt::WRead nn = r; nn.pulled = 100; nn.added = 0; nn.filled = 60; dt::wDecide(nn, "ES"); CHECK(!nn.W && !nn.F && dt::atLevelText(nn) == "-", "100 pulled vs 60 traded (< 2x), 60 < median: neither ('-')");
        dt::WRead nq = r; dt::wDecide(nq, "NQ"); CHECK(nq.W && !nq.F, "NQ has no fill median: W still decided, never Filled");
        dt::WRead back; CHECK(dt::parseW(dt::wLineOf(r2, 0.25), back, "ES") && back.W && back.bounced && back.pulled == 340 && back.key == 78 && back.px == 1000 && back.name == "PDL" && dt::wDrawn(back), "W line round trip (a restart redraws the same, flags not re-decided)");
        CHECK(!dt::parseW("W|78|0|1000|1|1|1|1|Z|5|PDL|0.25", back, "ES") && !dt::parseW("W|78|0|1000|1|1", back, "ES") && !dt::parseW("W|78|0|1000|1|1|1|1|W|5|PDL|0", back, "ES"), "garbled W lines refused");
        // dedupe: a second touch within 15 min of the first is not a new read; re-arming is needed
        w.onBook(2, 0, 1001, 1, 1100, 80); CHECK(w.open.empty(), "staying at the level: no new touch (needs re-arming)");
        w.onBook(2, 1, 1007, 1, 1200, 81); w.onBook(2, 0, 1001, 1, 1300, 82); CHECK(w.open.empty() && w.deduped == 1, "re-armed and touched again 290 s later: within 15 min, not a new read");
        w.onBook(2, 1, 1007, 1, 1900, 83); w.onBook(2, 0, 1002, 1, 2000, 84); CHECK(w.open.size() == 1 && w.open[0].key == 84, "touched again 16 min after the last: a new read");
        // from below: a resistance test
        dt::LevelWatch u; u.tol = 2; u.rearm = 6; std::vector<dt::WLevel> R(1); R[0].name = "ONH"; R[0].px = 2000; u.setLevels(R);
        u.onBook(2, 0, 1990, 1, 10, 1); u.onBook(2, 1, 1998, 1, 20, 2);
        CHECK(u.open.size() == 1 && u.open[0].side == dt::HIGH, "touch from below = a resistance test");
        u.onBook(1, 1, 2001, 70, 25, 2); u.onBook(1, 1, 1998, 70, 25, 2); CHECK(u.open[0].pulled == 70, "resistance zone L-1..L+2 on the offers (2001 in, 1998 out)");
        dt::LevelWatch z; z.tol = 2; z.rearm = 6; z.setLevels(L); z.onBook(2, 0, 1001, 1, 10, 1); CHECK(z.open.empty(), "the first trade already at the level: not a touch (it never came from away)");
    }
    // ---------------- the badge: A, H, Tr, G, W, P/B, L/X - at most 4 letters, the rest in the hover
    {   dt::BadgeIn b; b.A = true; b.defSide = dt::LOW; b.tr = dt::LOW; b.g = dt::LOW;
        std::vector<std::string> dr;
        std::vector<dt::BadgeSeg> v = dt::badgeSegs7(b, &dr);
        CHECK(dt::segsText(v) == "A|Tr|G?" && dr.empty() && segs(v) == "A|g/Tr|g/G?g", "A|Tr|G? at a low: every letter green (by meaning), nothing dropped");
        b.defSide = dt::HIGH; b.tr = dt::HIGH; b.g = dt::HIGH; v = dt::badgeSegs7(b, &dr); CHECK(segs(v) == "A|r/Tr|r/G?r", "A|Tr|G? at a high: red");
        dt::BadgeIn w; w.w = true; v = dt::badgeSegs7(w, &dr); CHECK(segs(v) == "W?r", "W? alone: red (a support at risk)");
        dt::BadgeIn m; m.A = true; m.H = true; m.Q = 'L'; v = dt::badgeSegs7(m, &dr); CHECK(dt::segsText(v) == "A|H|L?" && v.size() == 1, "A|H|L stays one part (1.16.0 look)");
        dt::BadgeIn x; x.Q = 'X'; x.defSide = dt::LOW; v = dt::badgeSegs7(x, &dr); CHECK(segs(v) == "X?r", "X alone: the opposite colour (1.16.0)");
        dt::BadgeIn all; all.A = true; all.H = true; all.tr = dt::LOW; all.g = dt::LOW; all.w = true; all.bidMove = 'P'; all.offerMove = 'B'; all.Q = 'X'; all.defSide = dt::LOW;
        v = dt::badgeSegs7(all, &dr);
        CHECK(dt::segsText(v) == "A|H|Tr|G?" && dr.size() == 4 && dr[0] == "W" && dr[1] == "P" && dr[2] == "B" && dr[3] == "X", "8 letters: the 4 most important on the badge (A|H|Tr|G?), W, P, B, X to the hover in that order");
        dt::BadgeIn five; five.g = dt::HIGH; five.w = true; five.bidMove = 'P'; five.offerMove = 'P'; five.Q = 'L'; five.defSide = dt::HIGH;
        v = dt::badgeSegs7(five, &dr); CHECK(dt::segsText(v) == "G|W|P|P?" && dr.size() == 1 && dr[0] == "L" && segs(v) == "G|r/W|r/P|g/P?r", "G|W|P|P? (bids P green, offers P red), L to the hover");
        dt::BadgeIn old; old.A = true; old.bidMove = 'P'; old.offerMove = 'P'; old.defSide = dt::LOW; v = dt::badgeSegs7(old, &dr);
        CHECK(segs(v) == "A|g/P|g/P?r", "the 1.18.0 badge unchanged: A| P| P? as three parts");
        CHECK(dt::badgeSegs7(dt::BadgeIn(), &dr).empty(), "nothing decided: no badge");
        auto wd = [](const std::string& s) { return [s](int pt) { return (int)(s.size() * pt * 0.62) + 4; }; };
        CHECK(dt::fitFont(wd("A|H|Tr|G?"), 34, 9, 6) == 6 && dt::fitFont(wd("A|Tr|G?"), 34, 9, 6) == 7 && dt::fitFont(wd("W?"), 34, 9, 6) == 9, "the font shrinks: 4 letters 6 pt, 3 letters 7 pt, 1 letter keeps 9 pt");
    }
    printf("G / Tr / W: %d passed, %d failed\n", p, f);
    return f ? 1 : 0;
}
