// test_aggbook_114.cpp -- BigLotTiers.h 1.0.0 (lsBidAskStats 1.14.0 Big Lot mark), SDK-free (2026-10-18).
//   the tier comes from complete EARLIER sessions only (3 needed, latest 5, one outlier dropped), RTH vs overnight are separate,
//   decided once (a later session / more history never changes it), Off / BL5 / BL10 / BL20, the history file round trip + garbage.
#include "BigLotTiers.h"
#include "SessionVWAPLogic.h"
#include <cstdio>

static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static blt::Sess sess(long long n5, long long n10, long long n20, int candles = 500) { blt::Sess s; s.candles = candles; s.n[0] = n5; s.n[1] = n10; s.n[2] = n20; return s; }

int main() {
    // nth / sessOf: the 5th biggest of the session's prints (both sides of every candle)
    std::vector<long long> pr; for (int i = 1; i <= 30; ++i) pr.push_back(i * 10);
    const blt::Sess s0 = blt::sessOf(pr, 120); CHECK(s0.n[0] == 260 && s0.n[1] == 210 && s0.n[2] == 110 && s0.candles == 120);
    CHECK(blt::sessOf(std::vector<long long>(4, 50), 10).n[0] == 0);          // fewer than 5 prints: unknown, never a guess

    // tier from earlier complete sessions only
    std::map<long, blt::Sess> h;
    h[100] = sess(300, 200, 100); h[101] = sess(320, 210, 110); h[102] = sess(310, 205, 105, 50);   // 102 incomplete (50 < need)
    h[103] = sess(330, 220, 120);
    const int need = 112;
    blt::Tier t103 = blt::tierFor(103, h, need);
    CHECK(!t103.ok && t103.nSess == 2 && t103.why.find("learning") == 0);   // 100, 101 complete; 102 not; 103 itself never
    h[104] = sess(9999, 9999, 9999);                                          // a LATER session: never used for 104 or before
    blt::Tier t105 = blt::tierFor(105, h, need);
    CHECK(t105.ok && t105.nSess == 4);                                        // 100, 101, 103, 104 (4 values -> the outlier rule runs)
    CHECK(t105.bl[0] == 317);                                                 // 9999 dropped as the outlier: (300 + 320 + 330) / 3 = 316.7
    blt::Tier t104 = blt::tierFor(104, h, need);
    CHECK(t104.ok && t104.bl[0] == 317 && t104.bl[1] == 210 && t104.bl[2] == 110);   // 100, 101, 103: (300+320+330)/3, (200+210+220)/3, (100+110+120)/3

    // Off / BL5 / BL10 / BL20
    CHECK(blt::markSize(t104, 0) == 0 && blt::markSize(t104, 1) == 317 && blt::markSize(t104, 2) == 210 && blt::markSize(t104, 3) == 110);
    CHECK(!blt::hit(10000, t104, 0));                                          // Off: nothing
    CHECK(blt::hit(317, t104, 1) && !blt::hit(316, t104, 1));                  // exact at the boundary
    CHECK(blt::hit(250, t104, 2) && !blt::hit(250, t104, 1) && blt::hit(120, t104, 3));
    CHECK(!blt::hit(100000, t103, 1));                                         // learning: no mark at any size
    CHECK(blt::reached(400, t104) == "BL5" && blt::reached(250, t104) == "BL10" && blt::reached(115, t104) == "BL20" && blt::reached(50, t104).empty());
    CHECK(blt::label(1) == "BL5" && blt::label(3) == "BL20" && blt::label(0).empty());

    // RTH vs overnight: the day rule (ES open 8:30 = 510, close 15:00 = 900) and separate books
    CHECK(blt::typeOf(510, 510, 900) == blt::NIGHT && blt::typeOf(513, 510, 900) == blt::DAY && blt::typeOf(960, 510, 900) == blt::DAY && blt::typeOf(963, 510, 900) == blt::NIGHT);
    { int o = 0, c = 0; CHECK(svl::rthOf("HG", o, c) && blt::typeOf(o + 3, o, c) == blt::DAY && blt::typeOf(o, o, c) == blt::NIGHT); }   // HG: its own open
    CHECK(blt::typeNeed(blt::DAY, 510, 900, 180) == 112 && blt::typeNeed(blt::NIGHT, 510, 900, 180) == 232);
    blt::Book B;
    for (long s = 100; s <= 103; ++s) { B.put(s, blt::DAY, sess(300, 200, 100)); B.put(s, blt::NIGHT, sess(120, 80, 40)); }
    const blt::Tier* d104 = B.tierAt(104, blt::DAY, need, true); const blt::Tier* n104 = B.tierAt(104, blt::NIGHT, need, true);
    CHECK(d104 && n104 && d104->bl[0] == 300 && n104->bl[0] == 120);          // a 200-lot bar: no mark in RTH, a mark overnight
    CHECK(!blt::hit(200, *d104, 1) && blt::hit(200, *n104, 1));

    // decided once: more history / a changed session afterwards never moves the tier of 104
    B.put(103, blt::DAY, sess(900, 900, 900)); B.put(99, blt::DAY, sess(900, 900, 900));
    CHECK(B.tierAt(104, blt::DAY, need, true)->bl[0] == 300);
    CHECK(B.tierAt(105, blt::DAY, need, false) == nullptr);                   // not ready (history still reading): not decided
    // a partly loaded session never overwrites a fuller one
    CHECK(!B.put(100, blt::DAY, sess(5, 5, 5, 30)) && B.hist[blt::DAY][100].n[0] == 300);

    // the history file: round trip (S + ok T lines; learning tiers are not kept), garbage rejected, the first decision stays
    B.tierAt(101, blt::DAY, need, true);                                      // learning (1 earlier session): not written
    const std::string txt = B.text();
    blt::Book C; C.load(txt);
    CHECK(C.rejected == 0 && C.hist[blt::DAY].size() == B.hist[blt::DAY].size() && C.hist[blt::NIGHT].size() == 4);
    CHECK(C.tiers[blt::DAY].count(104) == 1 && C.tiers[blt::DAY][104].bl[0] == 300 && C.tiers[blt::DAY].count(101) == 0);
    CHECK(C.tierAt(104, blt::DAY, need, true)->bl[0] == 300);                 // read back = the same tier after a restart
    blt::Book G; G.load("garbage\nS|1|2|3|4|5|6\nS|x|0|1|2|3|4\nT|104|1|2|50|40|30\nS|100|1|500|-5|1|1\nT|104|1|3|0|1|1\n" + txt + "T|104|1|5|1|1|1\n");
    CHECK(G.rejected == 6 && G.tiers[blt::DAY][104].bl[0] == 300);            // bad type / token / nSess < 3 / negative / BL5 0; a later T line never replaces
    // KEEP_SESS bounds the file
    blt::Book K; for (long s = 1; s <= 100; ++s) K.put(s, blt::DAY, sess(300, 200, 100));
    blt::Book K2; K2.load(K.text()); CHECK((int)K2.hist[blt::DAY].size() == blt::KEEP_SESS && K2.hist[blt::DAY].rbegin()->first == 100);

    // a Mark: frozen numbers, the setting only changes what is shown
    blt::Mark m; m.decided = true; m.lots = 250; m.tier = *d104;
    CHECK(!blt::shows(m, 1) && blt::shows(m, 2) && blt::shows(m, 3) && !blt::shows(m, 0));
    blt::Mark u; u.lots = 100000; u.tier = *d104; CHECK(!blt::shows(u, 1));  // not decided: never shown

    std::printf("BigLotTiers: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
