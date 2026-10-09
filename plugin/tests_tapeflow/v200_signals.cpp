// TapeFlow 2.0.0 regressions (fabricated data): the four signals' confirm rules (incl. the colour of the confirming bar), one signal
// per bar and its priority, no decision twice / out of order, confirmed signals never change, the session record round-trip
// (write -> restart -> identical marks, pending ones still confirm), the no-overlap layout, slot pooling, the adaptive QUIET,
// the scale fit and the header title.
#include <vector>
#include <deque>
#include <map>
#include <string>
#include <cmath>
#include <climits>
#include <cstdio>
#include <iostream>
#include <random>
#include "../TapeFlowLogic.h"

using namespace tfl;
static int checks = 0, fails = 0;
#define CHECK(x, msg) do { ++checks; if (!(x)) { ++fails; std::printf("FAIL %d: %s\n", __LINE__, msg); } } while (0)

static const long long T0 = 86400LL * 20000 + 9 * 3600;      // a 09:00 bar boundary
struct Day {
    std::vector<SecRec> H; std::vector<Ev> evs; std::vector<BarIn> bars;
    // one 3-min bar: OHLC (ticks) and its 180 seconds: buy / sell per second, 30-s pressure, 180-s pressure, activity
    void bar(int o, int h, int l, int c, long long buy, long long sell, double f30 = 0, double f180 = 0, double act = 1.0)
    {
        BarIn b; b.ts = bars.empty() ? T0 : bars.back().te; b.te = b.ts + 180; b.o = o; b.h = h; b.l = l; b.c = c; bars.push_back(b);
        for (long long t = b.ts; t < b.te; ++t) {
            SecRec r; r.t = t; r.b = buy / 180 + (t - b.ts < buy % 180 ? 1 : 0); r.s = sell / 180 + (t - b.ts < sell % 180 ? 1 : 0);
            r.f30 = (float)f30; r.f180 = (float)f180; r.a = (float)act; r.c30 = 1.0f; r.flags = SR_SIDES | SR_WARM | SR_BASE; H.push_back(r);
        }
    }
    void ev(const char* kind, int dir, int lo, int hi, int secIntoLastBar = 100)
    { Ev e; e.t = bars.back().ts + secIntoLastBar; e.knownAt = e.t + 1; e.kind = kind; e.dir = dir; e.lo = lo; e.hi = hi; evs.push_back(e); }
    void feedAll(SignalBook& bk, size_t from = 0) const { for (size_t i = from; i < bars.size(); ++i) bk.feed(bars[i], barFlow(H, evs, bars[i].ts, bars[i].te, bk.cfg)); }
    void feed(SignalBook& bk, size_t i) const { bk.feed(bars[i], barFlow(H, evs, bars[i].ts, bars[i].te, bk.cfg)); }
};
static const Mark* find(const SignalBook& b, char k) { for (auto& m : b.marks) if (m.kind == k) return &m; return nullptr; }
static int count(const SignalBook& b, char k) { int n = 0; for (auto& m : b.marks) if (m.kind == k) n++; return n; }

int main()
{
    // ---------------- P: push - pressure + price with it, decided at the close, the close colour decides
    {
        Day d; d.bar(1000, 1004, 998, 1002, 500, 500);
        d.bar(1002, 1010, 1001, 1009, 900, 300, 50, 20, 2.0);          // 180 s of 2:1 buying at 2x normal, GREEN, above the last close
        SignalBook b; d.feedAll(b);
        const Mark* p = find(b, 'P');
        CHECK(p && p->dir == 1 && p->state == MK_CONFIRMED && p->barT == d.bars[1].te, "P: buy pressure + a green close above the last close = bullish P, confirmed at the close");
        Day r; r.bar(1000, 1004, 998, 1002, 500, 500); r.bar(1002, 1010, 999, 1000, 900, 300, 50, 20, 2.0);   // same pressure, RED close
        SignalBook b2; r.feedAll(b2); CHECK(!find(b2, 'P'), "P: a red bar never confirms a buy push");
        Day s; s.bar(1000, 1004, 998, 1002, 500, 500); s.bar(1001, 1003, 999, 1002, 900, 300, 50, 20, 2.0);  // green, but not beyond the last close
        SignalBook b3; s.feedAll(b3); CHECK(!find(b3, 'P'), "P: price must go with it (close beyond the previous close)");
        Day q; q.bar(1000, 1004, 998, 1002, 500, 500); q.bar(1002, 1010, 1001, 1009, 900, 300, 50, 20, 1.0);  // pressure at normal activity
        SignalBook b4; q.feedAll(b4); CHECK(!find(b4, 'P'), "P: pressure at normal activity is not a push");
        Day v; v.bar(1000, 1004, 998, 1002, 500, 500); v.bar(1002, 1001 + 9, 990, 994, 300, 900, -50, -20, 2.0);
        v.bars[1].h = 1003; v.bars[1].o = 1002;
        SignalBook b5; v.feedAll(b5); const Mark* p5 = find(b5, 'P');
        CHECK(p5 && p5->dir == -1, "P: sell pressure + a red close below the last close = bearish P");
        Day w; w.bar(1000, 1004, 998, 1002, 500, 500); w.bar(1002, 1010, 1001, 1009, 600, 590, 5, 2, 1.0); w.ev("IN", 1, 1008, 1008);
        SignalBook b6; w.feedAll(b6); CHECK(find(b6, 'P') && find(b6, 'P')->dir == 1, "P: the engine's initiative (IN) counts as the push");
        Day c; c.bar(1000, 1004, 998, 1002, 500, 500);
        for (int i = 0; i < 5; ++i) c.bar(1002 + 8 * i, 1012 + 8 * i, 1001 + 8 * i, 1010 + 8 * i, 900, 300, 50, 20, 2.0);
        SignalBook b7; c.feedAll(b7); CHECK(count(b7, 'P') == 2, "P: one push mark, then not again in the same direction for 3 bars");
    }
    // ---------------- E: exhaustion - a run to the 60-min extreme, volume collapses, no new extreme; confirmed by a LATER opposite bar
    auto runUp = [](Day& d) {
        for (int i = 0; i < 6; ++i) d.bar(1000, 1004, 996, 1000, 400, 400);   // an hour of balance (high 1004)
        d.bar(1000, 1008, 999, 1007, 800, 300);                                 // the buy run: 2 bars, new 60-min high 1012
        d.bar(1007, 1012, 1006, 1011, 900, 350);
    };
    {
        Day d; runUp(d); d.bar(1011, 1012, 1008, 1010, 200, 250);               // buying collapses to ~23% of the run, no new high
        SignalBook b; d.feedAll(b);
        const Mark* e = find(b, 'E');
        CHECK(e && e->dir == -1 && e->state == MK_PENDING && e->px == 1012 && e->barT == d.bars.back().te, "E?: collapse at the 60-min high after a buy run (bearish, anchored at the high)");
        d.bar(1010, 1011, 1004, 1005, 300, 600);                                // a RED bar after the buy push
        d.feed(b, d.bars.size() - 1);
        CHECK(find(b, 'E')->state == MK_CONFIRMED, "E: confirmed by a later red bar");
        CHECK(b.marks.size() == 1 || b.marks.back().kind != 'E', "E: one mark per run extreme");
        Day g; runUp(g); g.bar(1011, 1012, 1008, 1010, 200, 250); g.bar(1010, 1014, 1009, 1013, 300, 200);   // closes ABOVE the high
        SignalBook b2; g.feedAll(b2); CHECK(find(b2, 'E') && find(b2, 'E')->state == MK_EXPIRED, "E?: a close beyond the extreme ends it unconfirmed");
        Day h; runUp(h); h.bar(1011, 1012, 1008, 1010, 200, 250); for (int i = 0; i < 5; ++i) h.bar(1009, 1011, 1008, 1010, 200, 200);
        SignalBook b3; h.feedAll(b3); CHECK(find(b3, 'E') && find(b3, 'E')->state == MK_EXPIRED, "E?: green / flat bars for 5 bars -> unconfirmed (stays drawn as E?)");
        Day k; runUp(k); k.bar(1011, 1012, 1005, 1006, 200, 250);              // the stall bar itself is red: not a LATER bar
        SignalBook b4; k.feedAll(b4); CHECK(find(b4, 'E') && find(b4, 'E')->state == MK_PENDING, "E?: its own bar never confirms it");
        Day m; runUp(m); m.bar(1011, 1015, 1008, 1010, 200, 250);              // a NEW high on the low volume: not exhaustion
        SignalBook b5; m.feedAll(b5); CHECK(!find(b5, 'E'), "E: price extended -> no E");
        Day n; runUp(n); n.bar(1011, 1012, 1008, 1010, 700, 250);              // volume held
        SignalBook b6; n.feedAll(b6); CHECK(!find(b6, 'E'), "E: buying did not collapse -> no E");
    }
    // ---------------- A: absorption - the engine's AW; confirmed by a bar closing AWAY in the right colour
    {
        Day d; d.bar(1000, 1004, 998, 1002, 500, 500); d.bar(1002, 1006, 1000, 1003, 900, 300); d.ev("AW", -1, 1004, 1006);   // buyers absorbed at 1004-1006
        SignalBook b; d.feedAll(b);
        const Mark* a = find(b, 'A');
        CHECK(a && a->dir == -1 && a->state == MK_PENDING && a->lo == 1004 && a->hi == 1006, "A?: buyers absorbed (bearish) at the zone");
        d.bar(1003, 1003, 1001, 1003, 300, 300); d.feed(b, 2);                  // flat close inside / below the zone but NOT red
        CHECK(find(b, 'A')->state == MK_PENDING, "A?: a non-red bar below the zone does not confirm buyers absorbed");
        d.bar(1003, 1003, 998, 999, 200, 600); d.feed(b, 3);                    // RED close below the zone
        CHECK(find(b, 'A')->state == MK_CONFIRMED && find(b, 'A')->confT == d.bars[3].te, "A: a red bar closing below the zone confirms buyers absorbed");
        Day u; u.bar(1000, 1004, 998, 1002, 500, 500); u.bar(1002, 1003, 996, 998, 300, 900); u.ev("AW", 1, 996, 998);   // sellers absorbed at 996-998
        u.bar(997, 999, 996, 998, 300, 300);                                    // green but not above the zone
        u.bar(999, 1002, 998, 1001, 600, 200);                                  // GREEN close above the zone
        SignalBook b2; u.feedAll(b2);
        CHECK(find(b2, 'A') && find(b2, 'A')->dir == 1 && find(b2, 'A')->state == MK_CONFIRMED && find(b2, 'A')->confT == u.bars[3].te, "A: a green bar closing above the zone confirms sellers absorbed");
        Day f; f.bar(1000, 1004, 998, 1002, 500, 500); f.bar(1002, 1006, 1000, 1003, 900, 300); f.ev("AW", -1, 1004, 1006);
        f.bar(1003, 1009, 1003, 1008, 900, 300);                               // closes ABOVE the zone (buyers went through)
        SignalBook b3; f.feedAll(b3); CHECK(find(b3, 'A')->state == MK_EXPIRED, "A?: a close through the zone on the aggressor's side ends it unconfirmed");
        Day s; s.bar(1000, 1004, 998, 1002, 500, 500); s.bar(1002, 1006, 998, 1001, 900, 300); s.ev("AW", -1, 1004, 1006, 30);   // red bar, closes below the zone
        SignalBook b4; s.feedAll(b4); CHECK(find(b4, 'A') && find(b4, 'A')->state == MK_CONFIRMED, "A: the absorption bar itself can confirm (red close below the zone)");
        CHECK(b4.journal.size() >= 2, "A: its record has the '?' line and then the confirmation line");
        Day x; x.bar(1000, 1004, 998, 1002, 500, 500); x.bar(1002, 1006, 1000, 1003, 900, 300); x.ev("AW", -1, 1004, 1006);
        for (int i = 0; i < 10; ++i) x.bar(1003, 1004, 1002, 1003, 300, 300);
        SignalBook b5; x.feedAll(b5); CHECK(find(b5, 'A')->state == MK_EXPIRED, "A?: 10 bars without confirmation -> stays A? (never confirmed)");
    }
    // ---------------- F: flip after an A / E; priority; one per bar
    {
        Day d; d.bar(1000, 1004, 998, 1002, 500, 500, 0, -10);
        d.bar(1002, 1003, 996, 998, 300, 900, -40, -35); d.ev("AW", 1, 996, 998);      // sellers absorbed, 180-s pressure clearly SELL
        d.bar(998, 1001, 997, 1000, 500, 450, 10, -5);
        d.bar(1000, 1004, 999, 1003, 800, 300, 40, 30);                                // 180-s now clearly BUY, green close
        SignalBook b; d.feedAll(b);
        const Mark* f = find(b, 'F');
        CHECK(f && f->dir == 1 && f->state == MK_CONFIRMED && f->ref == d.bars[1].te && f->barT == d.bars[3].te, "F: 180-s pressure -35 -> +30 within 4 bars of the A, green close = bullish F");
        CHECK(find(b, 'A')->state == MK_CONFIRMED, "the same green close above the zone also confirmed the A (on its own bar)");
        d.bar(1003, 1008, 1002, 1007, 800, 300, 40, 35); d.feed(b, 4);
        CHECK(count(b, 'F') == 1, "F: one flip per origin");
        Day r; r.bar(1000, 1004, 998, 1002, 500, 500, 0, -10);
        r.bar(1002, 1003, 996, 998, 300, 900, -40, -35); r.ev("AW", 1, 996, 998);
        r.bar(998, 1001, 997, 1000, 500, 450, 10, -5);
        r.bar(1000, 1004, 997, 999, 800, 300, 40, 30);                                 // flips but RED close
        SignalBook b2; r.feedAll(b2); CHECK(!find(b2, 'F'), "F: the bar must close in the new direction (green for bullish)");
        Day l; l.bar(1000, 1004, 998, 1002, 500, 500, 0, -10);
        l.bar(1002, 1003, 996, 998, 300, 900, -40, -35); l.ev("AW", 1, 996, 998);
        for (int i = 0; i < 4; ++i) l.bar(998, 999, 997, 998, 400, 400, 0, -5);
        l.bar(998, 1004, 997, 1003, 800, 300, 40, 30);                                 // 5 bars later: too late
        SignalBook b3; l.feedAll(b3); CHECK(!find(b3, 'F'), "F: only within 4 bars (12 min) of the A / E");
        // priority: F > A > E > P on one bar; never two signals on one bar
        Day p; p.bar(1000, 1004, 998, 1002, 500, 500, 0, -10);
        p.bar(1002, 1003, 996, 998, 300, 900, -40, -35); p.ev("AW", 1, 996, 998);
        p.bar(998, 1001, 997, 1000, 500, 450, 10, -5);
        p.bar(1000, 1006, 999, 1005, 900, 300, 50, 30, 2.0); p.ev("AW", 1, 1000, 1001);  // flip + a new AW + a push on ONE bar
        SignalBook b4; p.feedAll(b4);
        int onBar = 0; char k = 0; for (auto& m : b4.marks) if (m.barT == p.bars[3].te) { onBar++; k = m.kind; }
        CHECK(onBar == 1 && k == 'F', "priority: F beats A and P on the same bar; one signal per bar");
        Day q; q.bar(1000, 1004, 998, 1002, 500, 500); q.bar(1002, 1010, 1001, 1009, 900, 300, 50, 20, 2.0); q.ev("AW", -1, 1008, 1009);
        SignalBook b5; q.feedAll(b5); CHECK(b5.marks.size() == 1 && b5.marks[0].kind == 'A', "priority: A beats P");
        std::set<long long> seen; bool one = true; for (auto& m : b4.marks) if (!seen.insert(m.barT).second) one = false;
        CHECK(one, "never two signals on one bar");
    }
    // ---------------- decided once, in order; confirmed signals never change
    {
        Day d; d.bar(1000, 1004, 998, 1002, 500, 500); d.bar(1002, 1010, 1001, 1009, 900, 300, 50, 20, 2.0);
        SignalBook b; d.feedAll(b); const size_t n = b.marks.size(), j = b.journal.size();
        d.feed(b, 1); d.feed(b, 0);
        CHECK(b.marks.size() == n && b.journal.size() == j, "a bar fed again (or an older one) is never decided again");
        Day e; e.bar(1000, 1004, 998, 1002, 500, 500); e.bar(1002, 1006, 998, 1001, 900, 300); e.ev("AW", -1, 1004, 1006, 30);
        e.bar(1001, 1012, 1000, 1011, 900, 300);                                       // would 'expire' it - but it is confirmed
        SignalBook c; e.feedAll(c); CHECK(find(c, 'A')->state == MK_CONFIRMED, "a confirmed A never changes (a later close through the zone does nothing)");
        SignalBook c2; c2.loadText(std::string(SignalBook::header()) + "\n" + "100|A|-1|C|5|4|6|100|0|x|2.0.0\n100|A|-1|?|5|4|6|0|0|x|2.0.0\n100|A|-1|X|5|4|6|200|0|x|2.0.0\n");
        CHECK(c2.marks.size() == 1 && c2.marks[0].state == MK_CONFIRMED, "the record: a confirmed signal never goes back to ? or X");
    }
    // ---------------- the session record round-trip: write -> restart -> identical marks; pending ones still confirm
    {
        std::mt19937 rng(7); Day d; int px = 10000;
        for (int i = 0; i < 160; ++i) {
            int o = px, c = px + (int)(rng() % 9) - 4, h = std::max(o, c) + (int)(rng() % 4), l = std::min(o, c) - (int)(rng() % 4);
            long long buy = 300 + rng() % 700, sell = 300 + rng() % 700; double f30 = (double)(rng() % 120) - 60, f180 = (double)(rng() % 80) - 40;
            d.bar(o, h, l, c, buy, sell, f30, f180, 0.6 + (rng() % 20) / 10.0);
            if (rng() % 9 == 0) d.ev("AW", rng() % 2 ? 1 : -1, std::min(o, c) - 1, std::min(o, c) + 1, 60);
            if (rng() % 11 == 0) d.ev("IN", rng() % 2 ? 1 : -1, c, c, 120);
            px = c;
        }
        SignalBook full; d.feedAll(full);
        std::string rec = std::string(SignalBook::header()) + "\n";
        SignalBook live; bool sameEvery = true;
        for (size_t i = 0; i < d.bars.size(); ++i) {                          // a restart after EVERY bar
            SignalBook fresh; fresh.loadText(rec);
            for (size_t j = 0; j <= i; ++j) d.feed(fresh, j);
            for (auto& l : fresh.journal) rec += l + "\n";
            if (i == d.bars.size() - 1) live = fresh;
        }
        SignalBook restored; restored.loadText(rec);
        sameEvery = restored.marks.size() == full.marks.size();
        for (size_t i = 0; sameEvery && i < full.marks.size(); ++i) { const Mark &a = restored.marks[i], &b = full.marks[i];
            if (a.barT != b.barT || a.kind != b.kind || a.dir != b.dir || a.state != b.state || a.px != b.px || a.lo != b.lo || a.hi != b.hi) sameEvery = false; }
        int k[4] = {0, 0, 0, 0}; for (auto& m : full.marks) k[std::string("PEAF").find(m.kind)]++;
        printf("round-trip: %zu signals (P %d, E %d, A %d, F %d), record %zu bytes\n", full.marks.size(), k[0], k[1], k[2], k[3], rec.size());
        CHECK(!full.marks.empty() && sameEvery, "record round-trip: restarting from the file after every bar gives exactly the marks of one straight pass");
        CHECK(restored.decidedThrough == d.bars.back().te, "the record knows how far it decided");
        SignalBook again; again.loadText(rec); d.feedAll(again);
        CHECK(again.journal.empty(), "a restart re-feeding the decided bars writes nothing new");
        bool line = true; for (auto& m : full.marks) { SignalBook one; one.load(SignalBook::line(m, "2.0.0")); if (one.marks.size() != 1 || one.marks[0].px != m.px || one.marks[0].why != m.why) line = false; }
        CHECK(line, "every record line parses back to the same signal");
        CHECK(!SignalBook().load("garbage|x") && !SignalBook().load("12|Q|1|C|0|0|0|0|0||v"), "bad record lines are refused");
    }
    // ---------------- layout: never overlapping, the stronger keeps the spot
    {
        std::mt19937 rng(3); bool ok = true, strong = true;
        for (int trial = 0; trial < 200; ++trial) {
            std::vector<Slot> v(40);
            for (auto& s : v) { s.x = (int)(rng() % 400); s.w = 9 + (int)(rng() % 8); s.band = rng() % 2 ? 1 : -1; s.rank = (int)(rng() % 15); s.t = (long long)(rng() % 1000); }
            layoutMarks(v);
            for (size_t i = 0; i < v.size(); ++i) for (size_t j = i + 1; j < v.size(); ++j) {
                if (!v[i].show || !v[j].show || v[i].band != v[j].band) continue;
                const int li = v[i].x - v[i].w / 2, ri = li + v[i].w, lj = v[j].x - v[j].w / 2, rj = lj + v[j].w;
                if (!(ri + 2 <= lj || li >= rj + 2)) ok = false;
            }
            for (size_t i = 0; i < v.size(); ++i) if (!v[i].show) {
                bool blocked = false;
                for (size_t j = 0; j < v.size(); ++j) if (v[j].show && v[j].band == v[i].band && v[j].rank >= v[i].rank) {
                    const int li = v[i].x - v[i].w / 2, ri = li + v[i].w, lj = v[j].x - v[j].w / 2, rj = lj + v[j].w;
                    if (!(ri + 2 <= lj || li >= rj + 2)) blocked = true;
                }
                if (!blocked) strong = false;
            }
        }
        CHECK(ok, "layout: no two drawn signals touch in the same band (200 random layouts)");
        CHECK(strong, "layout: a signal is only left out for an equal or stronger one in its spot");
        std::vector<Slot> two(2); two[0].x = 100; two[0].w = 12; two[0].band = 1; two[0].rank = markRank(Mark{}) + 1; two[1] = two[0]; two[1].x = 104; two[1].rank = 13;
        layoutMarks(two); CHECK(!two[0].show && two[1].show, "layout: the confirmed (stronger) one wins a shared spot");
        Mark c; c.kind = 'P'; c.state = MK_CONFIRMED; Mark q; q.kind = 'F'; q.state = MK_PENDING;
        CHECK(markRank(c) > markRank(q), "layout rank: confirmed beats '?'");
    }
    // ---------------- slot pooling, adaptive QUIET
    {
        Store st; const long long sid = 20000;
        for (int s = 1; s <= 4; ++s) for (int b : {100, 101, 102, 99, 98}) for (int k = 0; k < (b == 100 ? 4 : 5); ++k) {
            Win w; w.endT = (sid - s - 1) * 86400 + 17LL * 3600 + b * 300 + k * 20 + 19; w.bin = b; w.rate20 = 2; w.act = 10; st[sid - s]["X"].push_back(w);
        }
        BinBase out[NBINS]; int used = 0;
        freezeBase(st, sid, 10, out, &used, "X", true, 60, 2);
        CHECK(out[100].own == 16 && out[100].k == 2 && out[100].n == 16 + 4 * 20 && out[100].n >= 60, "pooling: a thin slot (16) borrows +-1 then +-2 slots until 60");
        BinBase o0[NBINS]; freezeBase(st, sid, 10, o0, &used, "X", true, 0, 0);
        CHECK(o0[100].n == 16 && o0[100].k == 0, "pooling off: the slot's own windows only");
        CHECK(std::fabs(out[100].gap - 2.0f) < 1e-6, "typical gap between trades = 20 s / traded seconds (10) = 2 s");
        Engine e; e.setFixedBase(out[100]);
        CHECK(e.quoteAgeAt(T0) == 6, "adaptive QUIET: max(5 s, 3 x 2 s) = 6 s");
        BinBase thin = out[100]; thin.gap = 8; Engine e2; e2.setFixedBase(thin);
        CHECK(e2.quoteAgeAt(T0) == 24, "a thin slot (a trade every 8 s): quiet only after 24 s");
        thin.gap = 30; Engine e3; e3.setFixedBase(thin);
        CHECK(e3.quoteAgeAt(T0) == 30, "capped at 30 s");
        thin.gap = 0.5f; Engine e4; e4.setFixedBase(thin);
        CHECK(e4.quoteAgeAt(T0) == 5, "a busy slot (ES): 5 s as before");
    }
    // ---------------- the scale and the header title
    {
        CHECK(fitRange(0) == 60 && fitRange(52) == 60 && fitRange(70) == 90 && fitRange(100) == 120 && fitRange(NAN) == 60, "scale: max(60, 1.15 x max |value|) rounded up to 10");
        CHECK(std::fabs(axisRange(60, 300, 22) - 60 * 150.0 / 128.0) < 1e-9 && axisRange(60, 40, 22) == 60, "axis range covers the signal bands (tiny pane: unchanged)");
        StateInfo s; s.code = "READY"; s.act = 1.43;
        Mark a; a.kind = 'A'; a.px = 31446; a.state = MK_CONFIRMED;
        CHECK(titleLine("2.0.0", "ES", s, true, 25, &a, "7,861.50") == "TF 2.0 ES  BUYERS 1.4x  last A 7,861.50", "title: READY + buyers + last confirmed signal");
        CHECK(titleLine("2.0.0", "NQ", s, true, -20, nullptr, "") == "TF 2.0 NQ  SELLERS 1.4x", "title: sellers, no signal yet");
        CHECK(titleLine("2.0.0", "NQ", s, true, 10, nullptr, "") == "TF 2.0 NQ  BALANCED 1.4x", "title: balanced inside +-15");
        StateInfo c; c.code = "CAL"; c.n = 51; c.need = 60;
        CHECK(titleLine("2.0.0", "GC", c, true, 30, nullptr, "") == "TF 2.0 GC  CAL 51/60", "title: calibrating");
        StateInfo q; q.code = "QUIET";
        CHECK(titleLine("2.0.1", "HG", q, false, 0, nullptr, "") == "TF 2.0.1 HG  QUIET", "title: quiet; a patch version is shown in full");
        StateInfo w; w.code = "WATCH"; w.act = 0.8;
        CHECK(titleLine("2.0.0", "CL", w, false, 0, nullptr, "") == "TF 2.0 CL  BALANCED 0.8x", "title: an engine watch is just READY in the header");
    }
    printf("v200 signals: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
