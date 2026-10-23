// lsStopRun 1.2.0 logic tests (StopRunLogic.h + SwingRef.h): the burst, the SAME reference as Sr (last 3 ZigZag swings, key /
// MenthorQ / volume-profile levels, 0.5 ATR + near ticks, intact), the 3.0x size gate, the LIVE 10 s decision (back across + a big
// resting fill; no share rule), the candle-close mode, once per level, the verdicts stamped (SHOW / ? / LOG), the record (1.2.0 fields
// + 1.1 lines), determinism.
#include "../StopRunLogic.h"
#include <cstdio>
static int fails = 0, checks = 0;
#define CHECK(c, m) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
using namespace stl;

static const long long DAY = 20735, OPEN_END = DAY * 86400 + 8 * 3600 + 33 * 60;
struct C { long long o, h, l, c; };
static std::vector<C> dayA() {   // swing high 2040 (0), swing low 2000 (2, confirmed by 3), then candles above 2000
    std::vector<C> v;
    v.push_back(C{ 2030, 2040, 2020, 2035 }); v.push_back(C{ 2030, 2036, 2010, 2012 }); v.push_back(C{ 2012, 2020, 2000, 2005 }); v.push_back(C{ 2006, 2025, 2004, 2020 });
    for (int i = 0; i < 6; ++i) v.push_back(C{ 2020, 2030, 2010, 2025 });
    return v;
}
struct Sim {
    Detector D; long long t = 0;
    Sim(bool live = true) {
        D.setRth(510, 900, 180);
        swg::StopSweep X; X.stRatio = 3.0; X.stNearTicks = 4; X.staBigLots = 5; X.staDecideLive = live ? 1 : 0; X.staReclaimSec = 10; X.staAbsorbBars = 2;
        X.staBull = swg::QUERY; X.staBear = swg::LOG; X.stGrey = swg::LOG; applyParams(D.P, X); D.P.seedLots = 10;
        std::vector<C> v = dayA();
        for (size_t i = 0; i < v.size(); ++i) { Bar b; b.t = OPEN_END + 180LL * (long long)i; b.o = v[i].o; b.h = v[i].h; b.l = v[i].l; b.c = v[i].c; b.day = DAY; b.rth = true; D.barClose(b); }
        D.formingT = OPEN_END + 180LL * (long long)v.size(); t = (D.formingT - 120) * 1000;
    }
    // a sell burst: n fills from `from` down (step 1 tick every 2 fills), `lots` each, aggressor ids distinct
    void sellBurst(long long from, int n, long long lots, long long ms) { for (int i = 0; i < n; ++i) { D.aggFill(ms + i, ms / 1000, DOWN, from - i / 2, lots, 100 + i); D.restFill(ms + i, DOWN, from - i / 2, lots, 9000 + i); } }
};

int main() {
    // keyDir (host level filter): session levels +-1 with rank, the 9 MenthorQ codes = 2
    { int r = 0; CHECK(keyDir("PWL", r) == -1 && keyDir("AsiaHI", r) == 1 && keyDir("HVL0", r) == 2 && keyDir("VAL", r) == 0, "key / MQ names"); }
    // ---- 1. a run through the swing low 2000: fired once, LIVE decision at 10 s -------------------------------------------------
    {
        Sim s; const long long ms = s.t;
        s.sellBurst(2002, 10, 4, ms);                                 // 2002 -> 1998, 40 lots >= 3.0 x 10
        s.D.restFill(ms + 3, DOWN, 1999, 6, 4242);                  // a 6-lot resting bid below the level (>= 5)
        s.D.tick(ms + 200, ms / 1000);
        CHECK(s.D.fired.size() == 1 && s.D.fired[0].refKind == REF_SWING && s.D.fired[0].refPx == 2000 && s.D.fired[0].stPx == 1999 && s.D.fired[0].refBack == 1
              && s.D.fired[0].refT == OPEN_END + 360 && s.D.fired[0].showFire == swg::LOG, "St through the swing low 2000 (1 back), stop price 1999, grey verdict LOG");
        s.D.aggFill(ms + 2000, ms / 1000 + 2, UP, 2001, 1, 555);     // a buyer back above 2000
        s.D.tick(ms + 9000, ms / 1000 + 9);
        CHECK(s.D.decided.empty(), "not decided before 10 s");
        s.D.tick(ms + 10100, ms / 1000 + 10);
        CHECK(s.D.decided.size() == 1 && s.D.decided[0].outcome == ABSORBED && s.D.decided[0].show == swg::QUERY && s.D.decided[0].bigLotsMax >= 6, "Sta at 10 s: back above + a big resting bid; verdict ?");
        if (s.D.decided.size() == 1) {
            Event e = s.D.fired[0]; const Event& d = s.D.decided[0];
            e.outcome = d.outcome; e.decidedT = d.decidedT; e.bars = d.bars; e.bigIds = d.bigIds; e.excAtr = d.excAtr; e.reclaimed = d.reclaimed; e.decidedSec = d.decidedSec; e.bigLotsMax = d.bigLotsMax; e.show = d.show;
            CHECK(markText(e) == "Sta?" && drawn(e) && markColour(e) == 1, "drawn as a green Sta?");
            std::vector<std::string> L; L.push_back(REC_HEAD); L.push_back(stLine(e, 0.25)); L.push_back(saLine(e)); L.push_back(saLine(e)); L.push_back("ST|torn");
            long long bad = 0; const std::vector<Event> v = parseRecord(L, 0.25, &bad);
            CHECK(v.size() == 1 && bad == 1 && v[0].outcome == ABSORBED && v[0].show == swg::QUERY && v[0].showFire == swg::LOG && v[0].refKind == REF_SWING && v[0].refT == e.refT && v[0].bigLotsMax == e.bigLotsMax
                  && stLine(v[0], 0.25) == stLine(e, 0.25) && saLine(v[0]) == saLine(e), "record round trip (1.2.0), the second SA ignored (decided once)");
            std::vector<std::string> old; old.push_back("ST|k|100|90|9:00:00 AM|down|LonLO|500.00|499.75|40|10|-1|100|400|1"); old.push_back("SA|k|100|absorbed|1|1|50|1|95");
            const std::vector<Event> o = parseRecord(old, 0.25);
            CHECK(o.size() == 1 && o[0].show == swg::SHOW && o[0].showFire == swg::SHOW && o[0].outcome == ABSORBED, "1.1 lines still read (as SHOW)");
            e.refCt = "8:36 AM"; const std::vector<std::string> h = hoverRows(e, 0.25, 10, 5);
            CHECK(h.size() == 9 && h[0].find("Sta?  Stop run?") == 0 && h[1].find("Stop run at 499.75 below the swing low 500.00 from 8:36 AM: 4.0x") == 0 && h[4].find("Question:") == 0 && h[8].find("Example:") == 0, "hover");
        }
        // once per level: a second run through 2000 does not fire
        s.D.fired.clear(); s.sellBurst(2002, 10, 4, ms + 60000); s.D.tick(ms + 60300, ms / 1000 + 60);
        CHECK(s.D.fired.empty(), "the stops at a level fire once per session");
    }
    // ---- 2. not absorbed: stays below at 10 s; a big fill alone is not enough ---------------------------------------------------
    {
        Sim s; const long long ms = s.t;
        s.sellBurst(2002, 10, 4, ms); s.D.restFill(ms + 3, DOWN, 1999, 6, 4242); s.D.tick(ms + 200, ms / 1000);
        s.D.aggFill(ms + 2000, ms / 1000 + 2, UP, 2001, 1, 555); s.D.aggFill(ms + 5000, ms / 1000 + 5, DOWN, 1997, 1, 556);   // back below at the end
        s.D.tick(ms + 10100, ms / 1000 + 10);
        CHECK(s.D.decided.size() == 1 && s.D.decided[0].outcome == NOT_ABSORBED && s.D.decided[0].show == swg::LOG && !drawn(s.D.decided[0]), "the LAST trade below: not absorbed, St LOG = not drawn");
    }
    {
        Sim s; const long long ms = s.t;
        s.sellBurst(2002, 10, 4, ms); s.D.tick(ms + 200, ms / 1000);   // 4-lot resting fills only (< 5)
        s.D.aggFill(ms + 2000, ms / 1000 + 2, UP, 2001, 1, 555); s.D.tick(ms + 10100, ms / 1000 + 10);
        CHECK(s.D.decided.size() == 1 && s.D.decided[0].outcome == NOT_ABSORBED && s.D.decided[0].bigIds == 0, "back above but no big resting fill: not absorbed");
    }
    // ---- 3. gates: size (3.0x), near ticks, the 0.5 ATR start, not through, intact -------------------------------------------
    {
        Sim s; const long long ms = s.t;
        s.sellBurst(2002, 10, 2, ms); s.D.tick(ms + 200, ms / 1000);
        CHECK(s.D.fired.empty() && s.D.nearLevel == 1 && s.D.sized == 0, "20 lots < 3.0 x 10: no St");
        Sim a; a.sellBurst(2006, 14, 4, ms); a.D.tick(ms + 200, ms / 1000);   // starts 6 ticks before 2000 (> 4)
        CHECK(a.D.fired.empty(), "started more than near ticks before the level: no St");
        Sim b; b.sellBurst(2004, 6, 10, ms); b.D.tick(ms + 200, ms / 1000);    // 2004 -> 2002: never through 2000
        CHECK(b.D.fired.empty() && b.D.nearLevel == 0, "not through the level: nothing");
    }
    {   // a MenthorQ level when no swing is taken out; a volume-profile VAL
        Sim s; const long long ms = s.t;
        swg::Lvl m; m.name = "HVL"; m.px = 2017; m.kind = swg::MQ; m.dir = 0; s.D.levels.push_back(m);   // pre candles low 2010 < 2017? -> not intact
        swg::Lvl k; k.name = "PDL"; k.px = 2008; k.kind = swg::KEY; k.dir = -1; s.D.levels.push_back(k);
        s.sellBurst(2010, 10, 4, ms); s.D.tick(ms + 200, ms / 1000);
        CHECK(s.D.fired.size() == 1 && s.D.fired[0].ref == "PDL" && s.D.fired[0].refKind == REF_KEY, "through PDL 2008 (HVL 2017 was traded through before)");
        Sim v; std::vector<std::pair<long long, double> > rows = { {2009, 300}, {2020, 900}, {2025, 400} };
        Bar b; b.t = v.D.formingT; b.o = 2020; b.h = 2030; b.l = 2010; b.c = 2025; b.day = DAY; b.rth = true; v.D.barClose(b, &rows, DAY); v.D.formingT += 180;
        std::vector<swg::Lvl> L; v.D.V.levels(L); bool val = false; for (const swg::Lvl& x : L) if (x.name == "VAL") val = true;
        CHECK(v.D.V.now.ok && val, "volume profile levels from the candle's VAP");
    }
    // ---- 4. candle-close mode (staDecideLive 0): absorbed at a close back across with a big fill; else not after N closes --------
    {
        Sim s(false); const long long ms = s.t;
        s.sellBurst(2002, 10, 4, ms); s.D.restFill(ms + 3, DOWN, 1999, 8, 4242); s.D.tick(ms + 200, ms / 1000);
        s.D.tick(ms + 30000, ms / 1000 + 30);
        CHECK(s.D.decided.empty(), "candle mode: nothing decided live");
        Bar b; b.t = s.D.formingT; b.o = 2001; b.h = 2004; b.l = 1997; b.c = 2003; b.day = DAY; b.rth = true; s.D.barClose(b);
        CHECK(s.D.decided.size() == 1 && s.D.decided[0].outcome == ABSORBED, "closed back above with the big fill: absorbed");
    }
    // ---- 5. An order's fills before the level do not count toward its at/beyond-level Sta size -----------------------------------
    {
        Detector d; d.P.minFills = 2; d.P.minIds = 1; d.P.ratio = 1.0; d.P.seedLots = 1.0; d.P.bigLots = 5; d.P.reclaimS = 10; d.formingT = 1000;
        swg::Lvl L; L.name = "PDL"; L.px = 100; L.dir = -1; L.kind = swg::KEY; d.levels.push_back(L);
        d.aggFill(0, 100, DOWN, 102, 2, 1); d.restFill(0, DOWN, 102, 2, 4242);   // same bid: 2 lots before the level
        d.aggFill(1, 100, DOWN, 99, 4, 2); d.restFill(1, DOWN, 99, 4, 4242);    // only 4 lots at/beyond the level
        d.tick(100, 100);
        CHECK(d.fired.size() == 1 && d.pending.size() == 1, "mixed-price resting order still produces a pending St");
        d.aggFill(2000, 102, UP, 101, 1, 3);                                     // price reclaimed the level before the decision
        d.tick(10100, 110);
        CHECK(d.decided.size() == 1 && d.decided[0].outcome == NOT_ABSORBED && d.decided[0].bigIds == 0 && d.decided[0].bigLotsMax == 4,
              "only the 4 at/beyond-level lots count; the 2 pre-level lots cannot make a 5-lot Sta");
    }
    // ---- 6. determinism: the same stream twice -> identical lines -----------------------------------------------------------------
    {
        std::string out[2];
        for (int r = 0; r < 2; ++r) {
            Sim s; const long long ms = s.t;
            s.sellBurst(2002, 10, 4, ms); s.D.restFill(ms + 3, DOWN, 1999, 6, 4242); s.D.tick(ms + 200, ms / 1000);
            s.D.aggFill(ms + 2000, ms / 1000 + 2, UP, 2001, 1, 555); s.D.tick(ms + 10100, ms / 1000 + 10);
            for (size_t k = 0; k < s.D.fired.size(); ++k) out[r] += stLine(s.D.fired[k], 0.25) + "\n";
            for (size_t k = 0; k < s.D.decided.size(); ++k) out[r] += saLine(s.D.decided[k]) + "\n";
        }
        CHECK(!out[0].empty() && out[0] == out[1], "same stream -> identical record");
    }
    // ---- 7. garbage never crashes ------------------------------------------------------------------------------------------------
    { Detector D; D.aggFill(1, 1, 7, 100, 5, 1); D.aggFill(1, 1, DOWN, -5, 5, 1); D.aggFill(1, 1, DOWN, 100, 0, 1); D.restFill(1, DOWN, 100, 5, 0); D.tick(5000, 5);
      CHECK(D.bursts == 0 && D.fired.empty(), "garbage ignored"); }
    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
