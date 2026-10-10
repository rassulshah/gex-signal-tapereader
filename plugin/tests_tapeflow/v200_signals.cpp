// TapeFlow 2.0.0 / 2.0.1 regressions (2.0.1: only A is drawn - with its multiple; P / E / F logged; F relative to the slot's swing)
// Earlier: (fabricated data): the four signals' confirm rules (incl. the colour of the confirming bar), one signal
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
#include <fstream>
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
    void ev(const char* kind, int dir, int lo, int hi, int secIntoLastBar = 100, double qzone = 620, double norm = 200, int held = 0)
    { Ev e; e.held = held; e.t = bars.back().ts + secIntoLastBar; e.knownAt = e.t + 1; e.kind = kind; e.dir = dir; e.lo = lo; e.hi = hi; e.qzone = qzone; e.norm = norm; evs.push_back(e); }
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
        int drawnOn = 0, loggedOn = 0; char k = 0; for (auto& m : b4.marks) if (m.barT == p.bars[3].te) { if (m.drawn()) drawnOn++; else { loggedOn++; k = m.kind; } }
        CHECK(drawnOn == 1 && loggedOn == 1 && k == 'F', "(2.0.1) one bar: its A is drawn, and of the logged ones F beats P");
        Day q; q.bar(1000, 1004, 998, 1002, 500, 500); q.bar(1002, 1010, 1001, 1009, 900, 300, 50, 20, 2.0); q.ev("AW", -1, 1008, 1009);
        SignalBook b5; q.feedAll(b5); CHECK(b5.marks.size() == 2 && b5.drawnAt(q.bars[1].te) && b5.drawnAt(q.bars[1].te)->kind == 'A' && find(b5, 'P'), "(2.0.1) an A and a logged P share a bar; only the A is drawn");
        std::map<long long, int> nd, nl; bool one = true; for (auto& m : b4.marks) { if (m.drawn() ? ++nd[m.barT] > 1 : ++nl[m.barT] > 1) one = false; }
        CHECK(one, "never two drawn marks (or two logged signals) on one bar");
        // (2.0.1) F's threshold is the slot's normal 180-s swing: the same -35 -> +30 move is a flip in a slot that usually swings 25,
        // not in one that usually swings 40
        auto flipDay = [](Day& x) { x.bar(1000, 1004, 998, 1002, 500, 500, 0, -10); x.bar(1002, 1003, 996, 998, 300, 900, -40, -35); x.ev("AW", 1, 996, 998);
                                    x.bar(998, 1001, 997, 1000, 500, 450, 10, -5); x.bar(1000, 1004, 999, 1003, 800, 300, 40, 30); };
        auto feedSwing = [](const Day& x, SignalBook& bk, double sw) { for (auto& b : x.bars) bk.feed(b, barFlow(x.H, x.evs, b.ts, b.te, bk.cfg, sw)); };
        Day s1; flipDay(s1); SignalBook k1; feedSwing(s1, k1, 25); CHECK(find(k1, 'F'), "F: |180-s| beyond the slot's swing of 25 = a flip");
        Day s2; flipDay(s2); SignalBook k2; feedSwing(s2, k2, 40); CHECK(!find(k2, 'F'), "F: not beyond a slot swing of 40 = no flip");
        Day s3; flipDay(s3); SignalBook k3; feedSwing(s3, k3, 3); CHECK(find(k3, 'F') && std::fabs(barFlow(s3.H, s3.evs, s3.bars[3].ts, s3.bars[3].te, k3.cfg, 3).flipK - 10) < 1e-9, "F: the threshold never goes below 10");
    }
    // ---------------- (2.0.1) the A's multiple, contracts absorbed, ticks held; the label
    {
        Day d; d.bar(1000, 1004, 998, 1002, 500, 500); d.bar(1002, 1007, 1000, 1003, 900, 300);
        d.ev("AW", -1, 1004, 1006, 40, 840, 300, 3); d.ev("AW", -1, 1003, 1005, 120, 450, 300, 1);          // two watches: the bigger multiple wins
        d.bar(1003, 1009, 1001, 1002, 400, 400);                                                     // pokes 3 ticks above the zone, closes red but inside / below? (1002 < 1004)
        SignalBook b; d.feedAll(b);
        const Mark* a = b.drawnAt(d.bars[1].te);
        CHECK(a && std::fabs(a->mult - 2.8) < 1e-9 && a->absorbed == 840 && a->lo == 1004, "A: multiple = zone contracts / the slot's normal (840 / 300 = 2.8x); +840 = buyers absorbed");
        CHECK(a && a->state == MK_CONFIRMED && a->held == 3, "A: held = the AW's farthest aggressor trade past the zone edge (3 ticks), frozen with the A");
        CHECK(a && markLabel(*a) == "A 2.8x", "label: 'A 2.8x' once confirmed");
        Mark p = *a; p.state = MK_PENDING; CHECK(markLabel(p) == "A? 2.8x", "label: 'A? 2.8x' while pending");
        Day u; u.bar(1000, 1004, 998, 1002, 500, 500); u.bar(1002, 1003, 996, 998, 300, 900); u.ev("AW", 1, 996, 998, 100, 500, 400);
        SignalBook b2; u.feedAll(b2); CHECK(b2.marks.size() == 1 && b2.marks[0].absorbed == -500 && std::fabs(b2.marks[0].mult - 1.25) < 1e-9, "A: sellers absorbed -> negative contracts");
        // the record: 2.0.0 lines (11 columns) still load; 2.0.1 lines carry the three new columns
        SignalBook old; CHECK(old.load("1000|A|-1|C|5|4|6|1180|0|buyers absorbed|2.0.0") && old.marks[0].mult == 0 && old.marks[0].state == MK_CONFIRMED, "record: a 2.0.0 line (11 columns) still loads");
        const std::string l = SignalBook::line(*a, "2.0.1");
        CHECK(std::count(l.begin(), l.end(), '|') == 16 && l.find("|2.0.1|2.80|840|3|0|0|0.0") != std::string::npos, "record: ...|version|multiple|absorbed|held_ticks|minute_t|minute_vol|normal_minute");
        SignalBook nb; nb.load(l); CHECK(nb.marks.size() == 1 && std::fabs(nb.marks[0].mult - 2.8) < 1e-6 && nb.marks[0].absorbed == 840 && nb.marks[0].held == 3, "record: the new columns round-trip");
        CHECK(std::string(SignalBook::header()).find("|version|multiple|absorbed|held_ticks") != std::string::npos, "record header: the 2.0.0 columns first, then the new ones");
    }
    // ---------------- (2.0.1) baseline windows keep aggressive buy / sell -> the slot's normal 180-s swing and band volume
    {
        Win w; w.endT = (20000 - 1) * 86400LL + 17 * 3600 + 19; w.bin = 0; w.rate20 = 3; w.act = 7; w.b20 = 40; w.s20 = 20; w.Mb[0] = 12; w.Ms[7] = 9;
        long long sidX; std::string sy; Win r;
        const std::string ln = storeLine(20000, "X", w);
        CHECK(parseStoreLine(ln, &sidX, &sy, &r) && r.b20 == 40 && r.s20 == 20 && r.act == 7, "store: W2 + act + buy / sell round-trips");
        { std::vector<std::string> c; size_t a0 = 0; while (true) { size_t q = ln.find('|', a0); c.push_back(ln.substr(a0, q == std::string::npos ? q : q - a0)); if (q == std::string::npos) break; a0 = q + 1; }
          CHECK(c.size() == 30 && c[0] == "W2" && c[3] == std::to_string(w.endT) && c[26] == "9", "store: the LRA reader's fields (epoch c[3], ms8 c[26]) are where they were"); }
        Store st; const long long sid = 20000;
        for (int s = 1; s <= 3; ++s) for (int i = 0; i < 15 * 6; ++i) {          // 30 minutes per session from 17:00: 60% buyers
            Win x; x.endT = (sid - s - 1) * 86400LL + 17 * 3600 + i * 20 + 19; x.bin = binOf(x.endT); x.rate20 = 5; x.act = 10; x.b20 = 60; x.s20 = 40;
            x.Mb[1] = (float)(10 + i % 7); st[sid - s]["X"].push_back(x);
        }
        BinBase out[NBINS]; int used = 0; freezeBase(st, sid, 10, out, &used, "X", true, 60, 2);
        CHECK(std::fabs(out[3].swing180 - 20.0f) < 1e-4, "slot swing: median |180-s pressure| from 9 contiguous windows (60 / 40 -> 20)");
        CHECK(out[3].mb50[1] >= 10 && out[3].mb50[1] <= 16 && out[3].mb50[1] < out[3].qb[1], "slot normal band volume (median) is below its 95th percentile");
        Win y = w; y.b20 = -1; y.s20 = -1; Store s2; s2[sid - 1]["X"].push_back(y); BinBase o2[NBINS]; freezeBase(s2, sid, 10, o2, &used, "X", true, 60, 2);
        CHECK(o2[0].swing180 < 0, "slot swing unknown until windows with buy / sell are measured (then the last 60 min is used)");
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
        Mark big; big.kind = 'A'; big.mult = 3.1; big.state = MK_PENDING; Mark small_; small_.kind = 'A'; small_.mult = 1.4; small_.state = MK_CONFIRMED;
        std::vector<Slot> two(2); two[0].x = 100; two[0].w = 40; two[0].band = 1; two[0].rank = markRank(small_); two[1] = two[0]; two[1].x = 110; two[1].rank = markRank(big);
        layoutMarks(two); CHECK(!two[0].show && two[1].show, "(2.0.1) layout: the bigger multiple keeps a contested spot");
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
        Mark a; a.kind = 'A'; a.px = 31446; a.state = MK_CONFIRMED; a.mult = 3.14; a.absorbed = -840; a.held = 4; a.dir = 1;
        s.act = 1.62;
        CHECK(titleLine("2.0.1", "ES", s, true, 25, &a, "7,861.50") == "TF 2.0.1 ES  BUYERS 1.6x  last: A 3.1x 7,861.50  -840 absorbed  held 4t", "(2.0.1) title: READY + buyers + the last confirmed A");
        std::vector<std::string> tp = titleParts("2.0.1", "ES", s, true, 25, &a, "7,861.50");
        CHECK(tp.size() == 4 && tp[1] == "BUYERS" && tp[3].find("last: A 3.1x") != std::string::npos, "title parts for the coloured pane fallback (word, last A)");
        a.absorbed = 1200; a.held = 0; CHECK(titleLine("2.0.1", "ES", s, true, -25, &a, "7,861.50") == "TF 2.0.1 ES  SELLERS 1.6x  last: A 3.1x 7,861.50  +1200 absorbed  held 0t", "title: buyers absorbed = + contracts");
        s.act = 1.43;
        CHECK(titleLine("2.0.0", "NQ", s, true, -20, nullptr, "") == "TF 2.0 NQ  SELLERS 1.4x", "title: sellers, no signal yet");
        CHECK(titleLine("2.0.0", "NQ", s, true, 10, nullptr, "") == "TF 2.0 NQ  BALANCED 1.4x", "title: balanced inside +-15");
        StateInfo c; c.code = "CAL"; c.n = 51; c.need = 60;
        CHECK(titleLine("2.0.0", "GC", c, true, 30, nullptr, "") == "TF 2.0 GC  CAL 51/60", "title: calibrating");
        StateInfo q; q.code = "QUIET";
        CHECK(titleLine("2.0.1", "HG", q, false, 0, nullptr, "") == "TF 2.0.1 HG  QUIET", "title: quiet; a patch version is shown in full");
        StateInfo w; w.code = "WATCH"; w.act = 0.8;
        CHECK(titleLine("2.0.0", "CL", w, false, 0, nullptr, "") == "TF 2.0 CL  BALANCED 0.8x", "title: an engine watch is just READY in the header");
    }
    // ---------------- (2.0.2) the bar node (absorbMethod = 1)
    {
        // ES 2026-10-09 13:54: O 7869 H 7870.5 L 7864.75 C 7865, buyers hit 7869 hard (Delta Profile: +539 at 7869, 5.6x);
        // 13:57 closes red at 7864.25 -> a bearish A? at 7869 on the 13:54 bar, confirmed by 13:57
        auto T = [](double p) { return (int)std::llround(p / 0.25); };
        SignalBook bk; bk.cfg.absorbMethod = 1;
        std::vector<BarIn> bars; long long te = T0;
        for (int i = 0; i < 20; ++i) { BarIn b; b.ts = te; b.te = te + 180; b.o = T(7860); b.h = T(7862 + 0.25 * (i % 3)); b.l = T(7858); b.c = T(7861); bars.push_back(b); te += 180; }
        BarIn top; top.ts = te; top.te = te + 180; top.o = T(7869); top.h = T(7870.5); top.l = T(7864.75); top.c = T(7865); te += 180;
        BarIn nxt; nxt.ts = te; nxt.te = te + 180; nxt.o = T(7865); nxt.h = T(7866); nxt.l = T(7863.75); nxt.c = T(7864.25);
        std::map<int, std::pair<long long, long long>> rows;
        rows[T(7870.5)] = {40, 20}; rows[T(7870.25)] = {60, 40}; rows[T(7870)] = {90, 70}; rows[T(7869.75)] = {110, 60}; rows[T(7869.5)] = {120, 80};
        rows[T(7869.25)] = {150, 90}; rows[T(7869)] = {539, 120}; rows[T(7868.75)] = {80, 90}; rows[T(7868)] = {40, 60}; rows[T(7867)] = {30, 70};
        rows[T(7866)] = {30, 90}; rows[T(7865)] = {20, 110}; rows[T(7864.75)] = {10, 60};
        BarFlow none; none.secs = 180;
        for (auto& b : bars) bk.feed(b, none);
        BarFlow f = none; nodeFill(f, &rows, top, 1); bk.feed(top, f);
        const Mark* a = bk.drawnAt(top.te);
        CHECK(a && a->dir == -1 && a->lo == T(7869) && a->state == MK_PENDING && a->absorbed == 539, "NODE: ES 13:54 - bearish A? at 7869 (buyers absorbed, 539 contracts)");
        CHECK(a && a->mult > 3 && markLabel(*a).compare(0, 3, "A? ") == 0, "NODE: the multiple = node / the bar's average row");
        bk.feed(nxt, none);
        CHECK(a && bk.drawnAt(top.te)->state == MK_CONFIRMED && bk.drawnAt(top.te)->confT == nxt.te, "NODE: confirmed by 13:57 (red close 7864.25 below the node)");
        // the 20-s method on the same bar would have said bullish: here the bar node decides, the AW is ignored
        SignalBook b2; b2.cfg.absorbMethod = 1; for (auto& b : bars) b2.feed(b, none);
        BarFlow g = f; g.awDir = 1; g.awLo = T(7866); g.awHi = T(7867); g.awMult = 8.4; b2.feed(top, g);
        CHECK(b2.drawnAt(top.te) && b2.drawnAt(top.te)->dir == -1, "NODE switch: the 20-s watch does not draw an A");
        // rules: node near the bar's LOW with buying = not absorption at a top; the bar closing at its high = no A; not at the
        // 60-min high and no key level = no A; a key level within tolerance = A
        SignalBook b3; b3.cfg.absorbMethod = 1; for (auto& b : bars) b3.feed(b, none);
        BarIn hiClose = top; hiClose.c = T(7870.25); BarFlow h = none; nodeFill(h, &rows, hiClose, 1); b3.feed(hiClose, h);
        CHECK(!b3.drawnAt(hiClose.te), "NODE: a bar closing at its high (buyers not absorbed) gives no A");
        std::map<int, std::pair<long long, long long>> low = rows; low[T(7869)] = {120, 120}; low[T(7865)] = {539, 110};
        SignalBook b4; b4.cfg.absorbMethod = 1; for (auto& b : bars) b4.feed(b, none);
        BarFlow l = none; nodeFill(l, &low, top, 1); b4.feed(top, l);
        CHECK(!b4.drawnAt(top.te), "NODE: buying in the bottom of the bar is not absorption at a top");
        SignalBook b5; b5.cfg.absorbMethod = 1; b5.cfg.nodeLocation = true;   // (2.0.4) the location rule is a study variant now (the zigzag pivot is the default)
        for (auto& b : bars) { BarIn x = b; x.h = T(7880); b5.feed(x, none); }                   // the hour's high was higher
        BarFlow k = none; nodeFill(k, &rows, top, 1); b5.feed(top, k);
        CHECK(!b5.drawnAt(top.te), "NODE location: not the 60-min high and no key level -> no A");
        SignalBook b6; b6.cfg.absorbMethod = 1; b6.cfg.nodeLocation = true;
        for (auto& b : bars) { BarIn x = b; x.h = T(7880); b6.feed(x, none); }
        BarFlow k2 = none; nodeFill(k2, &rows, top, 1); k2.keyLevels.push_back(T(7871)); k2.keyTol = 2; b6.feed(top, k2);
        CHECK(b6.drawnAt(top.te) && b6.drawnAt(top.te)->dir == -1, "NODE location: within 2 ticks of a key level (7871) -> A");
        SignalBook b7; b7.cfg.absorbMethod = 1; b7.cfg.nodeMult = 9; for (auto& b : bars) b7.feed(b, none);
        BarFlow m9 = none; nodeFill(m9, &rows, top, 1); b7.feed(top, m9);
        CHECK(!b7.drawnAt(top.te), "NODE: below the minimum multiple -> no A");
        // rows of N ticks (NQ / CL scale)
        BarFlow n2 = none; nodeFill(n2, &rows, top, 4);
        CHECK(n2.ndHi - n2.ndLo == 3 && n2.ndLo <= T(7869) && n2.ndHi >= T(7869), "NODE: rows of N ticks group the prices");
        BarRows br; br.spb = 180; Tick x; x.t = T0 + 5; x.px = 100; x.bid = 99; x.ask = 100; x.q = 7; br.add(x); x.t = T0 + 179; x.px = 99; x.bid = 99; x.ask = 100; x.q = 3; br.add(x);
        x.t = T0 + 180; br.add(x);
        CHECK(br.at(T0 + 180) && br.at(T0 + 180)->at(100).first == 7 && br.at(T0 + 180)->at(99).second == 3 && br.at(T0 + 360), "BarRows: trades by bar end (the chart grid) and price, buy / sell");
    }
    // ---------------- (2.0.3) SANITY (a): ES 13:54 with the minute tape - arrow over 13:51-13:52, 2,934 bought = 3.6x a normal minute (812)
    {
        auto T = [](double p) { return (int)std::llround(p / 0.25); };
        const long long ts = T0 + 21 * 180;                              // the "13:54" bar: [13:51, 13:54)
        BarRows br; br.spb = 180;
        std::vector<SecRec> H;
        for (long long t = ts - 21 * 180; t < ts + 360; ++t) { SecRec r; r.t = t; r.b = 5; r.s = 5; H.push_back(r); }
        // minute 0 (13:51-13:52): buyers lift 7869 hard; 2,934 bought in that minute in all
        for (auto& r : H) if (r.t >= ts && r.t < ts + 60) { r.b = 2934 / 60 + (r.t - ts < 2934 % 60 ? 1 : 0); r.s = 15; }
        for (auto& r : H) if (r.t >= ts + 60 && r.t < ts + 180) { r.b = 12; r.s = 25; }
        auto addT = [&](long long t, double px, int side, long long q) { Tick k; k.t = t; k.px = T(px); k.q = q; if (side > 0) { k.bid = k.px - 1; k.ask = k.px; } else { k.bid = k.px; k.ask = k.px + 1; } br.add(k); };
        addT(ts + 10, 7869, 1, 500); addT(ts + 40, 7869.25, 1, 140); addT(ts + 70, 7869, 1, 39); addT(ts + 100, 7866, -1, 300); addT(ts + 150, 7865, -1, 200);
        addT(ts + 20, 7870.5, 1, 40); addT(ts + 30, 7868, -1, 90); addT(ts + 120, 7864.75, -1, 60);
        BarIn top; top.ts = ts; top.te = ts + 180; top.o = T(7869); top.h = T(7870.5); top.l = T(7864.75); top.c = T(7865);
        BarIn nxt; nxt.ts = ts + 180; nxt.te = ts + 360; nxt.o = T(7865); nxt.h = T(7866); nxt.l = T(7863.75); nxt.c = T(7864.25);
        SignalBook bk; bk.cfg.absorbMethod = 1; BarFlow none; none.secs = 180;
        for (int i = 0; i < 20; ++i) { BarIn b; b.ts = ts - (21 - i) * 180; b.te = b.ts + 180; b.o = T(7860); b.h = T(7862); b.l = T(7858); b.c = T(7861); bk.feed(b, none); }
        { BarIn b; b.ts = ts - 180; b.te = ts; b.o = T(7861); b.h = T(7869); b.l = T(7860.5); b.c = T(7869); bk.feed(b, none); }
        BarFlow f = none; nodeFill(f, br.at(top.te), top, 1); nodeMinuteFill(f, br, top, H, 812);
        bk.feed(top, f);
        const Mark* a = bk.drawnAt(top.te);
        printf("ES 13:54 sanity: dir %d node %.2f-%.2f pos %.2f minute %d (%lld bought) norm %.0f -> %s\n", a ? a->dir : 0, f.ndLo * 0.25, f.ndHi * 0.25, f.ndPos, f.ndMinute, a ? a->minuteVol : 0, f.minNorm, a ? markLabel(*a).c_str() : "-");
        CHECK(a && a->dir == -1 && a->lo == T(7869) && f.ndPos <= 0.25 && a->state == MK_PENDING, "(a) ES 13:54: bearish A? at the node 7869, inside the top 25% of the bar");
        CHECK(a && a->minuteT == ts && f.ndMinute == 0, "(a) the arrow belongs over minute 13:51-13:52 (where the node's buying traded)");
        CHECK(a && a->minuteVol == 2934 && std::fabs(a->mult - 2934.0 / 812.0) < 1e-9 && markLabel(*a) == "A? 3.6x", "(a) multiple = 2,934 bought / 812 normal minute = 3.6x");
        bk.feed(nxt, none);
        CHECK(bk.drawnAt(top.te)->state == MK_CONFIRMED && bk.drawnAt(top.te)->confT == nxt.te && markLabel(*bk.drawnAt(top.te)) == "A 3.6x", "(a) confirmed at the 13:57 close (7864.25, red, below the node) -> 'A 3.6x'");
        StateInfo st; st.code = "READY"; st.act = 1.2;
        std::vector<std::string> tp = titleParts("2.0.3", "ES", st, true, 0, bk.drawnAt(top.te), "7,869.00"); tp[1] = "TESTING"; tp[2].clear();
        const std::string title = tp[0] + tp[1] + tp[2] + tp[3];
        printf("header: %s\n", title.c_str());
        CHECK(title == "TF 2.0.3 ES  TESTING  last: A 3.6x 7,869.00  2,934 bought into the high = 3.6x a normal minute", "(a) the 2.0.3 header text");
        SignalBook rt; rt.loadText(std::string(SignalBook::header()) + "\n" + SignalBook::line(*bk.drawnAt(top.te), "2.0.3") + "\n");
        CHECK(rt.marks.size() == 1 && rt.marks[0].minuteT == ts && rt.marks[0].minuteVol == 2934 && std::fabs(rt.marks[0].norm - 812) < 0.05, "record: the node minute, its volume and the normal minute round-trip");
        Mark sm = *bk.drawnAt(top.te); sm.dir = 1; sm.minuteVol = 1500; sm.mult = 1500.0 / 812;
        CHECK(lastText(sm, "7,800.00").find("1,500 sold into the low = 1.8x a normal minute") != std::string::npos, "header: bullish = 'sold into the low'");
    }
    // ---------------- (2.0.3) SANITY (c): the minute bars sit inside their candle and never touch the next one, at every zoom
    {
        bool inside = true, apart = true, ordered = true; int collapsed = 0;
        for (int ppb = 1; ppb <= 120; ++ppb) for (int x = 0; x < 3; ++x) {
            const int cx = 500 + x;
            int a1[6], a2[6], b1[6], b2[6]; const int n = minuteSlots(cx, ppb, 3, a1, a2); const int m = minuteSlots(cx + ppb, ppb, 3, b1, b2);
            if (n == 1) collapsed++;
            for (int k = 0; k < n; ++k) { if (a1[k] < cx - ppb / 2.0 - 0.01 || a2[k] >= cx + ppb / 2.0) inside = false; if (k && a1[k] <= a2[k - 1] + 1) ordered = false; if (n > 1 && a2[k] - a1[k] < 1) ordered = false; if (a2[k] < a1[k]) ordered = false; }
            if (a2[n - 1] >= b1[0]) apart = false; (void)m;
        }
        CHECK(inside, "(c) every minute bar is inside its candle's width (ppb 1..120)");
        CHECK(apart, "(c) the last minute bar never reaches the next candle's first (ppb 1..120)");
        CHECK(ordered, "(c) the three minute bars never overlap each other, a 1-px gap between them, each >= 2 px wide");
        int s1[6], s2[6]; CHECK(minuteSlots(100, 12, 3, s1, s2) == 3 && minuteSlots(100, 9, 3, s1, s2) == 1, "(c) three bars from 10 px per candle, one (the minutes summed) below");
        printf("minute slots: one summed bar at %d of 360 zoom cases (ppb < 10)\n", collapsed);
    }
    // ---------------- (2.0.3, audit #2 / #26) volumes above 2^31: per-minute sums, the node minute and the record keep every contract
    {
        const long long big = 3000000001LL;                              // > 2^31
        std::vector<SecRec> H; for (long long t = 0; t < 120; ++t) { SecRec r; r.t = 1000 + t; r.b = big; r.s = big + 1; H.push_back(r); }
        long long B = 0, S = 0; sideSums(H, 1000, 1060, &B, &S);
        BarRows br; br.spb = 180; Tick k; k.t = 1000; k.px = 100; k.bid = 99; k.ask = 100; k.q = big; br.add(k); br.add(k);
        const auto* row = br.at(1080);
        Mark m; m.barT = 1080; m.kind = 'A'; m.dir = -1; m.px = 100; m.lo = 100; m.hi = 100; m.minuteT = 900; m.minuteVol = 60 * big + 7; m.norm = 1e9f; m.mult = 2.5;
        SignalBook rt; rt.loadText(std::string(SignalBook::header()) + "\n" + SignalBook::line(m, "2.0.3") + "\n");
        CHECK(B == 60 * big && S == 60 * (big + 1) && row && row->at(100).first == 2 * big && rt.marks.size() == 1 && rt.marks[0].minuteVol == 60 * big + 7,
              "(#2 / #26) volumes above 2^31 summed exactly (long long), the record keeps every contract");
    }
    // ---------------- (2.0.4) a REAL 2.0.0 record (20-s method, no multiple): the plugin's book (nodeOnly) keeps no A from it
    {
        std::ifstream f("/mnt/user-data/uploads/lsFlexLevels/TapeFlow/ES-signals-20735.csv", std::ios::binary);
        if (!f.good()) printf("(real 2.0.0 record) SKIPPED - file not here\n");
        else {
            std::stringstream ss; ss << f.rdbuf(); const std::string text = ss.str();
            long aLines = 0; { std::stringstream q(text); std::string ln; while (std::getline(q, ln)) if (ln.find("|A|") != std::string::npos) aLines++; }
            SignalBook node; node.nodeOnly = true; node.loadText(text); SignalBook all; all.loadText(text);
            int nA = 0, allA = 0; for (auto& m : node.marks) if (m.drawn()) nA++; for (auto& m : all.marks) if (m.drawn()) allA++;
            printf("real 2.0.0 record: %ld A lines; node-only book draws %d A (skipped %ld); a legacy book would hold %d\n", aLines, nA, node.skippedOld, allA);
            CHECK(nA == 0 && node.skippedOld == aLines && allA > 0, "(2.0.4) old 2.0.0 A records are never drawn by the plugin (skipped and counted)");
            Mark m; m.barT = 1000; m.kind = 'A'; m.dir = -1; m.px = 5; m.lo = 5; m.hi = 5; m.mult = 2.0; m.why = "buyers absorbed (bar node)";
            SignalBook mixed; mixed.nodeOnly = true; mixed.loadText(text + SignalBook::line(m, "2.0.4") + "\n");
            int mA = 0; for (auto& x : mixed.marks) if (x.drawn()) mA++;
            CHECK(mA == 1, "(2.0.4) a node A appended after old lines is drawn (only it)");
        }
    }
    // ---------------- (2.0.3) SANITY (b): the per-minute sums equal the per-second file's totals for a full session
    {
        const char* fn = getenv("TF_SEC_FILE");
        std::ifstream f(fn ? fn : "/mnt/user-data/uploads/lsFlexLevels/TapeFlow/ES-sec-20735.csv");
        if (!f.good()) printf("(b) SKIPPED - no per-second file here (set TF_SEC_FILE)\n");
        else {
            std::vector<SecRec> H; long long tb = 0, tsl = 0; std::string ln;
            while (std::getline(f, ln)) { if (ln.empty() || !isdigit((unsigned char)ln[0])) continue;
                std::vector<std::string> c; size_t a0 = 0; while (true) { size_t q = ln.find('|', a0); c.push_back(ln.substr(a0, q == std::string::npos ? q : q - a0)); if (q == std::string::npos) break; a0 = q + 1; }
                if (c.size() < 10) continue; SecRec r; r.t = atoll(c[0].c_str()); r.b = atoll(c[6].c_str()); r.s = atoll(c[7].c_str());
                if (!H.empty() && r.t <= H.back().t) continue; H.push_back(r); tb += r.b; tsl += r.s; }
            long long mb = 0, ms = 0; size_t minutes = 0; bool same = true;
            for (long long a = (H.front().t / 60) * 60; a <= H.back().t; a += 60) {
                long long B, S; sideSums(H, a, a + 60, &B, &S); mb += B; ms += S; minutes++;
                long long b2 = 0, s2 = 0; for (const SecRec& r : H) if (r.t >= a && r.t < a + 60) { b2 += r.b; s2 += r.s; } if (b2 != B || s2 != S) same = false;
                if (minutes > 30 && !same) break;
            }
            printf("(b) %zu seconds, %zu minutes: bought %lld / %lld, sold %lld / %lld\n", H.size(), minutes, mb, tb, ms, tsl);
            CHECK(mb == tb && ms == tsl && same, "(b) the 1-minute bars' contracts add up to the per-second file's buy / sell totals");
        }
    }
    {   // (2.0.3, audit #4 / #22) a half-written last line (the writer was mid-append) is not a record; a complete one is
        Mark m; m.barT = 1000; m.kind = 'A'; m.dir = -1; m.px = 5; m.lo = 5; m.hi = 5; m.mult = 3.6;
        const std::string full = SignalBook::line(m, "2.0.3");
        SignalBook a1; a1.loadText(std::string(SignalBook::header()) + "\n" + full.substr(0, full.size() - 6));
        SignalBook a2; a2.loadText(std::string(SignalBook::header()) + "\n" + full + "\n" + full.substr(0, 20));
        SignalBook a3; a3.loadText(std::string("garbage\n1e99|A|-1|C|5|5|5|0|0|x|v\n9223372036854775808|A|-1|C|5|5|5|0|0|x|v\n1000|A|-1|C|nan|5|5|0|0|x|v\n"));
        CHECK(a1.marks.empty() && a2.marks.size() == 1 && std::fabs(a2.marks[0].mult - 3.6) < 1e-5 && a3.marks.empty(), "record: partial last line ignored; malformed / out-of-range / non-finite lines rejected");
    }
    printf("v200 signals: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
