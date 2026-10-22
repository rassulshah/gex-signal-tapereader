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
    CHECK(std::string(dt::VERSION) == "1.8.0", "DefenseTurn 1.8.0");
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
        CHECK(dt::segsText(v) == "A|Tr|G" && dr.empty() && segs(v) == "A|g/Tr|g/Gg", "A|Tr|G at a low: every letter green (by meaning), nothing dropped");
        b.defSide = dt::HIGH; b.tr = dt::HIGH; b.g = dt::HIGH; v = dt::badgeSegs7(b, &dr); CHECK(segs(v) == "A|r/Tr|r/Gr", "A|Tr|G at a high: red");
        dt::BadgeIn w; w.w = true; v = dt::badgeSegs7(w, &dr); CHECK(segs(v) == "Wr", "W alone: red (a support at risk)");
        dt::BadgeIn m; m.A = true; m.H = true; m.Q = 'L'; v = dt::badgeSegs7(m, &dr); CHECK(dt::segsText(v) == "A|H|L" && v.size() == 1, "A|H|L stays one part (1.16.0 look)");
        dt::BadgeIn x; x.Q = 'X'; x.defSide = dt::LOW; v = dt::badgeSegs7(x, &dr); CHECK(segs(v) == "Xr", "X alone: the opposite colour (1.16.0)");
        dt::BadgeIn all; all.A = true; all.H = true; all.tr = dt::LOW; all.g = dt::LOW; all.w = true; all.bidMove = 'P'; all.offerMove = 'B'; all.Q = 'X'; all.defSide = dt::LOW;
        v = dt::badgeSegs7(all, &dr);
        CHECK(dt::segsText(v) == "A|H|Tr|G" && dr.size() == 4 && dr[0] == "W" && dr[1] == "P" && dr[2] == "B" && dr[3] == "X", "8 letters: the 4 most important on the badge (A|H|Tr|G), W, P, B, X to the hover in that order");
        dt::BadgeIn five; five.g = dt::HIGH; five.w = true; five.bidMove = 'P'; five.offerMove = 'P'; five.Q = 'L'; five.defSide = dt::HIGH;
        v = dt::badgeSegs7(five, &dr); CHECK(dt::segsText(v) == "G|W|P|P" && dr.size() == 1 && dr[0] == "L" && segs(v) == "G|r/W|r/P|g/Pr", "G|W|P|P (bids P green, offers P red), L to the hover");
        dt::BadgeIn old; old.A = true; old.bidMove = 'P'; old.offerMove = 'P'; old.defSide = dt::LOW; v = dt::badgeSegs7(old, &dr);
        CHECK(segs(v) == "A|g/P|g/Pr", "the 1.18.0 badge unchanged: A| P| P as three parts");
        CHECK(dt::badgeSegs7(dt::BadgeIn(), &dr).empty(), "nothing decided: no badge");
        auto wd = [](const std::string& s) { return [s](int pt) { return (int)(s.size() * pt * 0.62) + 4; }; };
        CHECK(dt::fitFont(wd("A|H|Tr|G"), 34, 9, 6) == 6 && dt::fitFont(wd("A|Tr|G"), 34, 9, 6) == 8 && dt::fitFont(wd("W"), 34, 9, 6) == 9, "the font shrinks: 4 letters 6 pt, 3 letters 8 pt (1.8.0: no '?'), 1 letter keeps 9 pt");
    }
    // ---------------- (1.8.0, Rassul 19:48 CT) no "?" on any badge; the per-letter hover built from the records
    {   bool anyQ = false;
        for (int m = 0; m < 4096; ++m) { dt::BadgeIn b; b.A = m & 1; b.H = m & 2; b.tr = (m & 4) ? dt::LOW : ((m & 8) ? dt::HIGH : -1); b.g = (m & 16) ? dt::HIGH : -1; b.w = m & 32; b.bidMove = (m & 64) ? 'P' : 0; b.offerMove = (m & 128) ? 'B' : 0;
            b.Q = (m & 256) ? 'L' : ((m & 512) ? 'X' : 0); b.defSide = (m & 1024) ? dt::HIGH : dt::LOW; std::vector<std::string> dr;
            const std::string t = dt::segsText(dt::badgeSegs7(b, &dr)); if (t.find('?') != std::string::npos) anyQ = true;
            for (size_t k = 0; k < dr.size(); ++k) if (dr[k].find('?') != std::string::npos) anyQ = true; }
        CHECK(!anyQ, "(1.8.0) no '?' on any of 4,096 badge combinations (nor in the letters moved to the hover)");
        CHECK(dt::segsText(dt::badgeSegs(true, true, 'L', dt::LOW, 'P', 'B')).find('?') == std::string::npos, "(1.8.0) the 1.6.0 badgeSegs has no '?' either");
        dt::Turn t; t.side = dt::LOW; t.attAggr = 640; t.defNet = 820; t.attNet = 310; t.marginPct = 165; t.linePx = 31005; t.A = true;   // 7,751.25 at 0.25
        dt::HoverBlock a = dt::hoverA(t, 0.25, false, 12);
        auto has = [](const dt::HoverBlock& b, const std::string& x) { for (size_t k = 0; k < b.rows.size(); ++k) if (b.rows[k].find(x) != std::string::npos) return true; return false; };
        CHECK(a.letter == "A" && a.rows.size() == 5 && has(a, "Absorbed - did the sellers' attack fail against the bids?") && has(a, "Sellers hit 640 lots into the bids") && has(a, "bids 820 vs offers 310") && has(a, "165% bigger") && has(a, "7,751.25") && has(a, "Use: the buyers held the low") && has(a, "Status: still learning, n = 12 (this chart; scored nightly)"), "(1.8.0) A: question, the record's numbers, use, status");
        t.side = dt::HIGH; a = dt::hoverA(t, 0.25, true, 3); CHECK(has(a, "Buyers lifted 640 lots into the offers at the high") && has(a, "close below") && has(a, "Status: proven"), "(1.8.0) A at a high mirrors; proven -> 'Status: proven'");
        dt::Counter c; c.side = dt::LOW; c.H = true; c.lots = 1536; c.attack = 640; dt::HoverBlock hh = dt::hoverH(c, false, 4);
        CHECK(has(hh, "Within 2 candles buyers lifted 1,536 lots = 2.4x the 640-lot attack") && has(hh, "0.25+ ATR above the low"), "(1.8.0) H: lots, x the attack, the ATR rule");
        dt::HoverBlock tr = dt::hoverTr(false, 640, 2.9, false, 1); CHECK(has(tr, "Sellers trapped 640 lots (2.9x normal) below the old low") && has(tr, "they must buy back"), "(1.8.0) Tr: lots, x, below the old low");
        dt::GiveUp g; g.side = dt::HIGH; g.G = true; g.x = 2.4; g.pull = 812; g.add = 120; g.secs = 45; g.pace = 0.4; dt::HoverBlock gb = dt::hoverG(g, false, 2);
        CHECK(has(gb, "the buyers pulled their bids at 2.4x normal (812 pulled vs 120 added in 45 s); pace down to 0.4x"), "(1.8.0) G: x normal, pulled vs added, pace");
        g.pace = -1; CHECK(has(dt::hoverG(g, false, 2), "pace n/a"), "(1.8.0) G with no pace: 'pace n/a', never a made-up number");
        dt::WRead w; w.name = "PDL"; w.px = 31000; w.pulled = 1000; w.added = 100; w.filled = 120; w.W = true; dt::HoverBlock wb = dt::hoverW(w, 0.25, false, 1);
        CHECK(has(wb, "In the first 60 s at PDL 7,750 the bids pulled 88%: 900 lots net pulled vs 120 traded"), "(1.8.0) W: level, % pulled, net lots vs traded");
        dt::HoverBlock pb = dt::hoverMove('P', 0, 420, 3, 2.5, false, 5), bb = dt::hoverMove('B', 1, 300, 1, 2.2, false, 2);
        CHECK(has(pb, "Bids (buyers) moved 420 lots up to 3 ticks toward price = 2.5x normal") && has(bb, "Offers (sellers) moved 300 lots up to 1 tick away from price = 2.2x normal") && pb.letter == "P" && bb.letter == "B", "(1.8.0) P / B: which side, lots, ticks, x normal");
        dt::Retest q; q.side = dt::LOW; q.res = 'L'; q.rest = 860; q.ratio = 1.2; dt::HoverBlock lb = dt::hoverQ(q, 7751.25, 0.25, false, 3);
        CHECK(has(lb, "Re-test of the R line 7,751.25: 860 lots of bids waited in the zone = 1.2x normal (reloaded at 1.0x or more)"), "(1.8.0) L: the R line, lots waiting, x normal");
        q.res = 'X'; q.rest = 40; q.ratio = 0.1; CHECK(has(dt::hoverQ(q, 0, 0.25, false, 3), "Re-test of the R line: 40 lots of bids waited in the zone = 0.1x normal (empty at 0.35x or less)"), "(1.8.0) X: empty on the re-test (unknown line price left out)");
        std::vector<dt::HoverBlock> all; all.push_back(a); all.push_back(hh); all.push_back(tr); all.push_back(gb); all.push_back(wb); all.push_back(pb); all.push_back(bb); all.push_back(lb);
        bool longRow = false, qm = false; for (size_t k = 0; k < all.size(); ++k) for (size_t r = 0; r < all[k].rows.size(); ++r) { if (all[k].rows[r].size() > 115) longRow = true; }
        const std::string ht = dt::hoverText("A|H|Tr|G", std::vector<std::string>{ "W", "P", "B", "L" }, all);
        for (size_t k = 1; k < all.size(); ++k) for (size_t r = 0; r < all[k].rows.size(); ++r) if (all[k].rows[r].find('?') != std::string::npos && all[k].rows[r].find(" - ") == std::string::npos) qm = true;
        CHECK(!longRow, "(1.8.0) every hover row <= 115 characters (lesson 83)");
        { bool wide = false; std::string cur; for (size_t k = 0; k <= ht.size(); ++k) { if (k == ht.size() || ht[k] == '\n') { if (cur.size() > dt::HOVER_WRAP + 5) wide = true; cur.clear(); } else cur += ht[k]; }
          CHECK(!wide, "(1.8.0) the drawn hover lines are wrapped at 56 characters (narrow columns fit beside the info box)"); }
        { dt::Turn t0 = t; t0.side = dt::LOW; t0.attAggr = 0; CHECK(has(dt::hoverA(t0, 0.25, false, 1), "Sellers out-hit the bids at the low (lots not in the record)"), "(1.8.0) A read back with no attack size: says so, never '0 lots'"); }
        CHECK(dt::hoverCompact("x\n   Use: a b\n     c\n   Status: s\nq", 1) == "x\n   Status: s\nq" && dt::hoverCompact("x\n   Use: a b\n     c\n   Status: s\n     t\nq", 2) == "x\nq", "(1.8.0) the fit cascade drops a row with its wrapped continuation");
        CHECK(!qm, "(1.8.0) '?' only ends a block's question, never a status");
        CHECK(ht.find("Defense A|H|Tr|G  (+ W, P, B, L: no room on the badge)") == 0 && ht.find("\n\nW  Withdrew") != std::string::npos && ht.find("\n\nL  Reloaded") != std::string::npos, "(1.8.0) the hover lists every letter, including the ones cut from the badge, one block each");
        CHECK(dt::lotsText(1234567) == "1,234,567" && dt::lotsText(-1540) == "-1,540" && dt::priceText(7751.25, 0.25) == "7,751.25" && dt::priceText(7750, 0.25) == "7,750" && dt::priceText(1.16255, 0.00005) == "1.16255" && dt::priceText(3.215, 0.0005) == "3.2150", "(1.8.0) lots / price text per market");
        // placement: inside the pane, below the badge, never over the info box (>= 4 px clear); scanning sideways; nothing when no room
        unsigned seed = 7; auto rnd = [&](int n) { seed = seed * 1103515245u + 12345u; return (int)((seed >> 8) % (unsigned)n); };
        int placed = 0, none = 0; bool bad = false;
        for (int it = 0; it < 20000; ++it) {
            const int L = 0, T = 0, R = 300 + rnd(1500), B = 90 + rnd(260);
            dt::TipRect badge; badge.l = L + rnd(R - 20); badge.r = badge.l + 10 + rnd(30); badge.t = T + 5; badge.b = badge.t + 13;
            dt::TipRect box; box.l = rnd(R); box.r = std::min(R, box.l + 60 + rnd(260)); box.t = rnd(B / 2); box.b = std::min(B, box.t + 30 + rnd(120));
            const int w = 60 + rnd(900), h = 20 + rnd(200); dt::TipRect at;
            if (dt::tipSpot(L, T, R, B, badge, rnd(5) ? &box : nullptr, [&](int, int& ww, int& hh2) { ww = w; hh2 = h; }, at)) { ++placed;
                if (at.l < L + 2 || at.r > R - 2 || at.b > B - 2 || at.t < badge.b) bad = true;
                const bool over = at.l < box.r + 4 && at.r + 4 > box.l && at.t < box.b + 4 && at.b + 4 > box.t; (void)over; }
            else ++none;
        }
        CHECK(!bad && placed > 5000 && none > 100, ("(1.8.0) 20,000 random panes: the hover always inside the pane, below the badge (" + std::to_string(placed) + " placed, " + std::to_string(none) + " no room)").c_str());
        bool overBox = false; seed = 11;
        for (int it = 0; it < 20000; ++it) {
            const int R = 300 + rnd(1500), B = 90 + rnd(260); dt::TipRect badge; badge.l = rnd(R - 20); badge.r = badge.l + 20; badge.t = 5; badge.b = 18;
            dt::TipRect box; box.l = rnd(R); box.r = std::min(R, box.l + 60 + rnd(260)); box.t = rnd(B / 2); box.b = std::min(B, box.t + 30 + rnd(120));
            const int w = 60 + rnd(900), h = 20 + rnd(200); dt::TipRect at;
            if (dt::tipSpot(0, 0, R, B, badge, &box, [&](int, int& ww, int& hh2) { ww = w; hh2 = h; }, at) && at.l < box.r + 4 && at.r + 4 > box.l && at.t < box.b + 4 && at.b + 4 > box.t) overBox = true;
        }
        CHECK(!overBox, "(1.8.0) 20,000 random boxes: the hover never within 4 px of the info box");
        dt::TipRect badge; badge.l = 100; badge.r = 130; badge.t = 5; badge.b = 18; dt::TipRect box; box.l = 0; box.r = 400; box.t = 0; box.b = 200; dt::TipRect at;
        CHECK(!dt::tipSpot(0, 0, 400, 200, badge, &box, [&](int, int& ww, int& hh2) { ww = 100; hh2 = 40; }, at), "(1.8.0) the box fills the pane: no hover (counted in the status), never drawn over it");
        box.l = 0; box.r = 150; box.t = 20; box.b = 80;
        CHECK(dt::tipSpot(0, 0, 600, 200, badge, &box, [&](int, int& ww, int& hh2) { ww = 200; hh2 = 60; }, at) && at.l >= 154 && at.t == 21, "(1.8.0) a box left of the badge: the hover slides right of it, same height under the badge");
    }
    printf("G / Tr / W: %d passed, %d failed\n", p, f);
    return f ? 1 : 0;
}
