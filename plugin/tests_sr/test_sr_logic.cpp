// lsSweepRefill 1.1.0 logic tests (SweepRefillLogic.h + SwingRef.h): the 1-bar ZigZag (rly2.py rules incl. the outside-candle tie),
// the swing reference (1 / 2 / 3 back eligibility, intact, 0.5 ATR start, deepest, RTH only), the key / MenthorQ fallback (5 pre
// candles), the params file (locked rule + SHOW / ? / LOG), the refill rule (50% within 5 s AND still there 5 s later, orders resting
// 1 s+), the no-adjacent rule, the bearish mirror, the record (23 fields + 1.0.0 lines), determinism (no repaint), garbage.
#include "../SweepRefillLogic.h"
#include <cstdio>
#include <map>
static int fails = 0, checks = 0;
#define CHECK(c, m) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
using namespace swr;

static const long long DAY = 20735;                        // CT day number (CT seconds / 86400)
static const long long OPEN_END = DAY * 86400 + 8 * 3600 + 33 * 60;   // the first RTH candle's END (8:30-8:33)
struct C { long long o, h, l, c; };
struct Sim {
    Detector D; long long id = 1000;
    Sim() { D.P.spb = 180; D.setRth(510, 900); swg::StopSweep X; X.srBull = swg::SHOW; X.srBear = swg::QUERY; applyParams(D.P, X); }
    void candles(const std::vector<C>& v, long long t0 = OPEN_END) {
        for (size_t i = 0; i < v.size(); ++i) {
            Bar b; b.t = t0 + 180LL * (long long)i; b.o = v[i].o; b.h = v[i].h; b.l = v[i].l; b.c = v[i].c; b.session = DAY; b.day = DAY;
            b.rth = swg::isRth((int)(b.t % 86400), 180, 510, 900); D.barClose(b);
        }
    }
    long long lastEnd() const { return D.barEnds.back(); }
    void ev(int a, int side, long long px, long long sz, long long oid, long long ms) { Ev e; e.action = a; e.side = side; e.px = px; e.size = sz; e.id = oid; e.ms = ms; e.sec = ms / 1000; D.onEvent(e); }
    std::vector<long long> rest(int side, long long from, int n, long long lots, long long ms) {
        std::vector<long long> ids;
        for (int k = 0; k < n; ++k) { const long long px = side == BID ? from - k : from + k; ev(NEW, side, px, lots, ++id, ms); ids.push_back(id); }
        return ids;
    }
    long long sweep(int side, const std::vector<long long>& ids, long long from, long long lots, long long ms) {
        for (size_t k = 0; k < ids.size(); ++k) { const long long px = side == BID ? from - (long long)k : from + (long long)k; ev(FILL, side, px, lots, ids[k], ms + 2 * (long long)k); ev(DEL, side, px, 0, ids[k], ms + 2 * (long long)k); }
        return ms + 2 * (long long)(ids.size() - 1);
    }
};
// a day: up candle, then a swing high at 0 (2040), a swing low at 2 (2000) confirmed by 3, then higher candles that stay above 2000
static std::vector<C> dayA() {
    std::vector<C> v;
    v.push_back(C{ 2030, 2040, 2020, 2035 });   // 0 green: up
    v.push_back(C{ 2030, 2036, 2010, 2012 });   // 1 lower low -> swing high 2040 (0) confirmed
    v.push_back(C{ 2012, 2020, 2000, 2005 });   // 2 down extends
    v.push_back(C{ 2006, 2025, 2004, 2020 });   // 3 higher high -> swing low 2000 (2) confirmed
    for (int i = 0; i < 6; ++i) v.push_back(C{ 2020, 2030, 2010, 2025 });   // 4-9 stay above 2000 (equal lows: no lower low)
    return v;
}

int main() {
    // ---- keyDir: his session levels incl. PWH / PWL / AsiaHI / AsiaLO; mixed joined names = not a side --------------------------
    CHECK(keyDir("ONL") == -1 && keyDir("LonHI") == 1 && keyDir("PFH/PDH") == 1 && keyDir("PDL / PFL") == -1 && keyDir("VAL") == 0
          && keyDir("PWL") == -1 && keyDir("AsiaHI") == 1 && keyDir("PDH/PDL") == 0, "key names -> side");
    { Clock c; long long a = c.at(100, 5000), b = c.at(100, 5040), d = c.at(101, 5050), e = c.at(100, 5060), f = c.idle(9050);
      CHECK(a == 100000 && b == 100040 && d == 101000 && e == 101010 && f >= 104000, "clock"); }
    CHECK(swg::isRth(8 * 3600 + 33 * 60, 180, 510, 900) && !swg::isRth(8 * 3600 + 30 * 60, 180, 510, 900) && swg::isRth(15 * 3600, 180, 510, 900) && !swg::isRth(15 * 3600 + 180, 180, 510, 900), "RTH candles: start >= open, END <= close");

    // ---- 1. the ZigZag (rly2.py): flips, extension ties (the later candle), outside candles -----------------------------------
    {
        Sim s; s.candles(dayA());
        const swg::ZigZag& Z = s.D.Z;
        CHECK(Z.bars.size() == 10 && Z.swings.size() == 2, "two confirmed swings");
        CHECK(Z.swings.size() == 2 && Z.swings[0].dir == 1 && Z.swings[0].px == 2040 && Z.swings[0].idx == 0 && Z.swings[0].confirmIdx == 1, "swing high 2040 at 0, confirmed by 1");
        CHECK(Z.swings.size() == 2 && Z.swings[1].dir == -1 && Z.swings[1].px == 2000 && Z.swings[1].idx == 2 && Z.swings[1].confirmIdx == 3 && Z.swings[1].barT == OPEN_END + 360, "swing low 2000 at 2, confirmed by 3");
        // outside candle in an up swing: lower low AND higher high, closes green -> no flip (extends the high)
        swg::ZigZag z; z.spb = 180; z.rthLenSec = 390 * 60;
        auto add = [&](long long t, long long o, long long h, long long l, long long c) { swg::ZBar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.day = 1; b.rth = true; z.add(b); };
        add(100, 10, 20, 5, 15); add(280, 15, 25, 3, 24);        // green outside -> up continues, ext = 1
        CHECK(z.swings.empty() && z.st == 1 && z.ext == 1, "green outside candle: no flip");
        add(460, 24, 30, 2, 4);                                   // red outside (lower low + higher high, red) -> flips down
        CHECK(z.swings.size() == 1 && z.swings[0].px == 25 && z.st == -1, "red outside candle with a lower low: flips, pivot = the prior high");
        add(640, 4, 6, 2, 5);                                     // equal low: the later candle becomes the extreme
        CHECK(z.ext == 3, "tie: the later candle");
        add(820, 5, 9, 1, 2);                                     // lower low + higher high + red -> no flip in a down swing
        CHECK(z.swings.size() == 1 && z.st == -1 && z.ext == 4, "red outside in a down swing: extends");
        swg::ZBar on; on.t = 1000; on.o = on.h = on.l = on.c = 1; on.day = 1; on.rth = false; z.add(on);
        CHECK(z.bars.size() == 5, "overnight candles are not fed");
        swg::ZBar nd; nd.t = 90000; nd.o = 1; nd.h = 2; nd.l = 1; nd.c = 2; nd.day = 2; nd.rth = true; z.add(nd);
        CHECK(z.bars.size() == 1 && z.swings.empty() && z.day == 2, "a new day resets at its RTH open");
    }
    // ---- 2. a bullish Sr through the swing low 2000 (1 back), refilled 50%+ within 5 s and still there 5 s later --------------
    {
        Sim s; s.candles(dayA());
        const long long base = (s.lastEnd() + 30) * 1000;
        const std::vector<long long> ids = s.rest(BID, 2003, 10, 10, base - 5000);    // 100 lots on 2003..1994 for 5 s
        const long long end = s.sweep(BID, ids, 2003, 10, base);
        s.D.advance(end + 60);
        CHECK(s.D.pending.size() == 1 && s.D.tookOut == 1, "one candidate");
        CHECK(s.D.pending.size() == 1 && s.D.pending[0].e.pre == 100 && s.D.pending[0].e.refKind == REF_PIVOT && s.D.pending[0].e.refPx == 2000 && s.D.pending[0].e.refBack == 1
              && s.D.pending[0].e.refT == OPEN_END + 360 && s.D.pending[0].e.ref == "swing low", "through the swing low 2000, 1 back, its candle");
        s.rest(BID, 2003, 10, 6, end + 500);              // 60 lots back 0.5 s after (60%)
        s.ev(NEW, BID, 1998, 50, 777777, end + 2600);      // a young order: counts only once it has rested 1 s
        s.D.advance(end + 9900);
        CHECK(s.D.decided.empty(), "not decided before window + stay (10 s)");
        s.D.advance(end + 10000);
        CHECK(s.D.decided.size() == 1, "decided at 10 s");
        if (s.D.decided.size() == 1) {
            const Event& e = s.D.decided[0];
            CHECK(e.post == 110 && e.refilled && e.gapOk && bullish(e) && e.show == swg::SHOW, "post 110 at 10 s, refilled, drawn (SHOW)");
            CHECK(e.ttrMs == 2000, "50% reached at the 2 s check (the 0.5 s orders had rested 1 s+ by then)");
            CHECK(e.barT == s.lastEnd() + 180, "on the forming candle (its END key)");
            CHECK(shown(e, true, true) && !shown(e, false, true) && std::string(markText(e)) == "Sr", "bullish checkbox; text Sr");
            const std::string ln = recLine(e, 0.25); std::vector<std::string> L; L.push_back(REC_HEAD); L.push_back(ln); L.push_back(ln); L.push_back("SR|torn"); L.push_back("XX|1");
            long long bad = 0; const std::vector<Event> v = parseRecord(L, 0.25, &bad);
            CHECK(v.size() == 1 && bad == 2 && v[0].key == e.key && v[0].post == 110 && v[0].refilled && v[0].gapOk && v[0].refKind == REF_PIVOT && v[0].refT == e.refT && v[0].refBack == 1
                  && v[0].show == swg::SHOW && v[0].barT == e.barT && recLine(v[0], 0.25) == ln, "record round trip (23 fields), duplicate key ignored, bad lines counted");
            Event h0 = e; h0.refCt = "8:36 AM";
            const std::vector<std::string> h = hoverRows(h0, 0.25, 5, 5, 50);
            CHECK(h.size() == 8 && h[0].find("Sr  Sweep refilled?") == 0 && h[1].find("Swept 10 prices (500.75 - 498.50) through the swing low 500.00 from 8:36 AM - refilled 110% at 10 s - bids defending") == 0, "hover read line names the swing");
            CHECK(h[2].find("Took out the swing low 500.00 from 8:36 AM") == 0, "hover: took out the swing");
            CHECK(h[3].find("Question:") == 0 && h[4].find("What it is:") == 0 && h[5].find("Why it matters:") == 0 && h[6].find("How to use:") == 0 && h[7].find("Example:") == 0, "hover structure");
            // a 1.0.0 line (20 fields) still reads, as SHOW
            std::string old = ln.substr(0, ln.rfind('|')); old = old.substr(0, old.rfind('|')); old = old.substr(0, old.rfind('|'));
            std::vector<std::string> L2; L2.push_back(old); const std::vector<Event> v2 = parseRecord(L2, 0.25);
            CHECK(v2.size() == 1 && v2[0].show == swg::SHOW && v2[0].refT == 0, "1.0.0 record line read");
        }
        // the next candle: refilled again -> recorded, NOT drawn (no adjacent)
        s.D.decided.clear();
        Bar c; c.t = s.lastEnd() + 180; c.o = 2000; c.l = 1990; c.h = 2010; c.c = 2000; c.session = DAY; c.day = DAY; c.rth = true; s.D.barClose(c);
        const long long b2 = (s.lastEnd() + 20) * 1000;
        Level L; L.name = "LonLO"; L.px = 1988; L.kind = REF_KEY; L.dir = -1; s.D.levels.push_back(L);
        const std::vector<long long> i2 = s.rest(BID, 1989, 9, 10, b2 - 3000);
        const long long e2 = s.sweep(BID, i2, 1989, 10, b2);
        s.D.advance(e2 + 60); s.rest(BID, 1989, 9, 10, e2 + 300); s.D.advance(e2 + 10000);
        CHECK(s.D.decided.size() == 1 && s.D.decided[0].refilled && !s.D.decided[0].gapOk && s.D.decided[0].ref == "swing low", "adjacent candle: recorded only (a swing is taken before a level)");
    }
    // ---- 3. refilled within 5 s but gone at 10 s -> not refilled; never reached 50% -> not refilled ----------------------------
    {
        Sim s; s.candles(dayA());
        const long long base = (s.lastEnd() + 30) * 1000;
        const std::vector<long long> ids = s.rest(BID, 2003, 10, 10, base - 5000);
        const long long end = s.sweep(BID, ids, 2003, 10, base);
        s.D.advance(end + 60);
        const std::vector<long long> back = s.rest(BID, 2003, 10, 8, end + 200);
        s.D.advance(end + 6000);
        for (size_t k = 0; k < back.size(); ++k) s.ev(DEL, BID, 2003 - (long long)k, 0, back[k], end + 6500);
        s.D.advance(end + 10000);
        CHECK(s.D.decided.size() == 1 && !s.D.decided[0].refilled && s.D.decided[0].peak >= 79 && s.D.decided[0].post == 0, "back at 2 s, gone before 10 s: not refilled");
    }
    {
        Sim s; s.candles(dayA());
        const long long base = (s.lastEnd() + 30) * 1000;
        const std::vector<long long> ids = s.rest(BID, 2003, 10, 10, base - 5000);
        const long long end = s.sweep(BID, ids, 2003, 10, base);
        s.D.advance(end + 60); s.rest(BID, 2003, 10, 4, end + 200); s.D.advance(end + 10000);
        CHECK(s.D.decided.size() == 1 && !s.D.decided[0].refilled && s.D.decided[0].ttrMs == -1, "40% only: not refilled");
    }
    // ---- 4. the reference rules ------------------------------------------------------------------------------------------------
    {   // 2 back counts only when beyond 1 back; the deepest taken out wins; 3 back needs beyond 1 and 2
        Sim s; std::vector<C> v;
        v.push_back(C{ 2030, 2040, 2020, 2035 });   // 0 up
        v.push_back(C{ 2030, 2036, 2000, 2002 });   // 1 lower low -> high 2040 (0)
        v.push_back(C{ 2002, 2040, 2010, 2030 });   // 2 higher high -> low 2000 (1)
        v.push_back(C{ 2030, 2035, 2005, 2008 });   // 3 lower low -> high 2040 (2)
        v.push_back(C{ 2008, 2038, 2007, 2030 });   // 4 higher high -> low 2005 (3)
        v.push_back(C{ 2030, 2039, 2008, 2035 });   // 5
        v.push_back(C{ 2035, 2045, 2009, 2040 });   // 6
        v.push_back(C{ 2040, 2046, 2012, 2041 });   // 7
        v.push_back(C{ 2041, 2047, 2015, 2042 });   // 8
        s.candles(v);
        const swg::ZigZag& Z = s.D.Z;
        int lows = 0; for (size_t i = 0; i < Z.swings.size(); ++i) if (Z.swings[i].dir < 0) ++lows;
        CHECK(lows == 2, "two swing lows: 2000 then 2005");
        // a sweep 2009 -> 1998: takes out 2005 (1 back) and 2000 (2 back, beyond 1 back) -> the deepest = 2000, 2 back
        swg::Sweep w; w.down = true; w.start = 2009; w.ext = 1998; w.barT = s.lastEnd() + 180;
        swg::Ref r = swg::pick(Z, s.D.W, s.D.levels, w, 180);
        CHECK(r.ok && r.px == 2000 && r.back == 2 && r.kind == swg::SWING, "deepest: the swing low 2000, 2 swings back");
        swg::Params p1; p1.maxBack = 1; r = swg::pick(Z, s.D.W, s.D.levels, w, 180, p1);
        CHECK(r.ok && r.px == 2005 && r.back == 1, "most recent only (St NQ): 2005");
        w.ext = 2003; r = swg::pick(Z, s.D.W, s.D.levels, w, 180);
        CHECK(r.ok && r.px == 2005, "ends between: only 2005 taken out");
        w.start = 2005 - 30; w.ext = 1970; r = swg::pick(Z, s.D.W, s.D.levels, w, 180);   // started 30 ticks past 2005 / 2000 (ATR ~ 30) -> > 0.5 ATR
        CHECK(!r.ok, "started more than 0.5 ATR past: not a take-out");
        // an older swing that is NOT beyond the newer one does not count: sweep of 2007 only between
        Sim t; std::vector<C> u;
        u.push_back(C{ 2030, 2040, 2020, 2035 }); u.push_back(C{ 2030, 2036, 2006, 2008 });   // low 2006 (1)
        u.push_back(C{ 2008, 2041, 2010, 2030 }); u.push_back(C{ 2030, 2035, 2002, 2003 });   // high 2041 (2), low 2002 (3)
        u.push_back(C{ 2003, 2040, 2004, 2030 }); u.push_back(C{ 2030, 2042, 2012, 2041 }); u.push_back(C{ 2041, 2044, 2013, 2043 });
        t.candles(u);
        swg::Sweep w2; w2.down = true; w2.start = 2008; w2.ext = 2004; w2.barT = t.lastEnd() + 180;
        r = swg::pick(t.D.Z, t.D.W, t.D.levels, w2, 180);
        CHECK(!r.ok, "2 back (2006) is above 1 back (2002): not eligible");
    }
    {   // intact: a swing that a candle before the last one traded through does not count; the last candle may have
        Sim s; std::vector<C> v = dayA(); v[6].l = 1999; s.candles(v);     // candle 6 traded below 2000 (not the last one)
        swg::Sweep w; w.down = true; w.start = 2003; w.ext = 1990; w.barT = s.lastEnd() + 180;
        CHECK(!swg::pick(s.D.Z, s.D.W, s.D.levels, w, 180).ok, "taken out earlier: not intact");
        Sim t; std::vector<C> u = dayA(); u[9].l = 1999; t.candles(u);      // the LAST closed candle traded below: still counts
        swg::Sweep w2 = w; w2.barT = t.lastEnd() + 180;
        CHECK(swg::pick(t.D.Z, t.D.W, t.D.levels, w2, 180).ok, "the last candle may have traded through");
    }
    {   // levels: key (side) and MenthorQ (either side), intact over the 5 pre candles, the deepest; flags
        Sim s; std::vector<C> v; for (int i = 0; i < 12; ++i) v.push_back(C{ 2020, 2030, 2010, 2025 }); s.candles(v, DAY * 86400 + 3 * 3600);   // overnight: no ZigZag
        CHECK(s.D.Z.bars.empty(), "overnight candles: no ZigZag");
        Level a; a.name = "ONL"; a.px = 2008; a.kind = REF_KEY; a.dir = -1;
        Level b; b.name = "ONH"; b.px = 2005; b.kind = REF_KEY; b.dir = 1;
        Level m; m.name = "HVL"; m.px = 2006; m.kind = REF_MQ; m.dir = 0;
        Level z; z.name = "PDL"; z.px = 2012; z.kind = REF_KEY; z.dir = -1;    // inside the pre candles' range: not intact
        s.D.levels.push_back(a); s.D.levels.push_back(b); s.D.levels.push_back(m); s.D.levels.push_back(z);
        swg::Sweep w; w.down = true; w.start = 2010; w.ext = 2000; w.barT = s.lastEnd() + 180;
        swg::Ref r = swg::pick(s.D.Z, s.D.W, s.D.levels, w, 180);
        CHECK(r.ok && r.name == "HVL" && r.kind == swg::MQ, "the deepest intact: HVL 2006 (ONH is a high, PDL was traded through)");
        swg::Params p; p.useMq = false; r = swg::pick(s.D.Z, s.D.W, s.D.levels, w, 180, p);
        CHECK(r.ok && r.name == "ONL", "MenthorQ off: ONL");
    }
    // ---- 4b. volume-profile levels: the profile rule, developing POC / VAH / VAL, prior day, naked POCs, VAL taken out ------------
    {
        std::map<long long, double> rows; rows[100] = 10; rows[101] = 50; rows[102] = 20; rows[103] = 5; rows[99] = 15;   // total 100
        swg::Prof P = swg::profileOf(rows);
        CHECK(P.ok && P.poc == 101 && P.val == 99 && P.vah == 103, "POC 101; the next two above (25) = the next two below (25): both -> value area 99..103");
        swg::VolProf V; std::vector<std::pair<long long, double> > r1 = { {100, 10}, {101, 40}, {102, 10} };
        V.add(100, 1, 102, 100, r1);                                  // session 1
        V.add(280, 2, 105, 104, { {104, 5}, {105, 30} });             // session 2: session 1 done (POC 101)
        std::vector<swg::Lvl> L; V.levels(L);
        bool pd = false, cur = false; for (const swg::Lvl& x : L) { if (x.name == "pdPOC" && x.px == 101) pd = true; if (x.name == "POC" && x.px == 105) cur = true; }
        CHECK(pd && cur && L[0].kind == swg::VP && L[0].dir == 0, "current POC 105 (developing) + the prior day's pdPOC 101");
        V.add(460, 3, 110, 108, { {109, 9} });                        // session 3: session 2 done; session 1's POC 101 untested -> nPOC
        L.clear(); V.levels(L); bool naked = false; for (const swg::Lvl& x : L) if (x.name == "nPOC" && x.px == 101) naked = true;
        CHECK(naked, "naked prior POC 101");
        V.add(640, 3, 110, 100, { {100, 1} });                        // a candle trades at 101: no longer naked
        L.clear(); V.levels(L); naked = false; for (const swg::Lvl& x : L) if (x.name == "nPOC") naked = true;
        CHECK(!naked, "tested: no nPOC");
        // an Sr through today's VAL (no swing below): "Took out VAL 498.00"
        Sim s; std::vector<C> v; for (int i = 0; i < 12; ++i) v.push_back(C{ 2020, 2030, 2010, 2025 });
        std::vector<std::pair<long long, double> > vr = { {1992, 30}, {2015, 200}, {2020, 300}, {2025, 150} };
        for (size_t i = 0; i < v.size(); ++i) { Bar b; b.t = DAY * 86400 + 3 * 3600 + 180LL * (long long)i; b.o = v[i].o; b.h = v[i].h; b.l = v[i].l; b.c = v[i].c; b.session = DAY; b.day = DAY; b.rth = false; s.D.barClose(b, &vr); }
        CHECK(s.D.V.now.ok && s.D.V.now.poc == 2020, "developing POC from the candles' volume at price");
        const long long val = s.D.V.now.val;
        const long long base = (s.lastEnd() + 30) * 1000;
        const std::vector<long long> ids = s.rest(BID, val + 3, 10, 10, base - 5000);
        const long long end = s.sweep(BID, ids, val + 3, 10, base); s.D.advance(end + 60); s.rest(BID, val + 3, 10, 8, end + 100); s.D.advance(end + 10000);
        CHECK(s.D.decided.size() == 1 && s.D.decided[0].refKind == REF_VP && s.D.decided[0].ref == "VAL" && s.D.decided[0].refPx == val && s.D.decided[0].refilled, "Sr through today's VAL");
        if (s.D.decided.size() == 1) { const std::vector<std::string> h = hoverRows(s.D.decided[0], 0.25, 5, 5, 50); CHECK(h[2].find("Took out VAL ") == 0, "hover names the level: Took out VAL ..."); 
            const std::string ln = recLine(s.D.decided[0], 0.25); CHECK(ln.find("|VAL|vp|") != std::string::npos && parseRecord(std::vector<std::string>(1, ln), 0.25).size() == 1, "record kind vp"); }
    }
    // ---- 5. orders younger than 1 s are not in pre ---------------------------------------------------------------------------
    {
        Sim s; s.candles(dayA());
        const long long base = (s.lastEnd() + 30) * 1000;
        const std::vector<long long> a = s.rest(BID, 2003, 10, 4, base - 5000);
        std::vector<long long> b; for (int k = 0; k < 10; ++k) { s.ev(NEW, BID, 2003 - k, 6, ++s.id, base - 300); b.push_back(s.id); }
        for (int k = 0; k < 10; ++k) { s.ev(FILL, BID, 2003 - k, 4, a[k], base + 2 * k); s.ev(DEL, BID, 2003 - k, 0, a[k], base + 2 * k); s.ev(FILL, BID, 2003 - k, 6, b[k], base + 2 * k + 1); s.ev(DEL, BID, 2003 - k, 0, b[k], base + 2 * k + 1); }
        s.D.advance(base + 100);
        CHECK(s.D.pending.size() == 1 && s.D.pending[0].e.pre == 40, "pre ignores orders younger than 1 s");
    }
    // ---- 6. the bearish mirror through the swing high 2040, verdict "?" -> "Sr?" -----------------------------------------------
    {
        Sim s; std::vector<C> v = dayA(); for (size_t i = 4; i < v.size(); ++i) v[i].h = 2036; s.candles(v);
        const long long base = (s.lastEnd() + 30) * 1000;
        const std::vector<long long> ids = s.rest(ASK, 2037, 9, 8, base - 5000);
        const long long end = s.sweep(ASK, ids, 2037, 8, base);
        s.D.advance(end + 60); s.rest(ASK, 2037, 9, 7, end + 400); s.D.advance(end + 10000);
        CHECK(s.D.decided.size() == 1 && !bullish(s.D.decided[0]) && s.D.decided[0].refilled && s.D.decided[0].refPx == 2040 && s.D.decided[0].show == swg::QUERY, "bearish Sr? through the swing high 2040");
        CHECK(s.D.decided.size() == 1 && shown(s.D.decided[0], true, true) && !shown(s.D.decided[0], true, false) && std::string(markText(s.D.decided[0])) == "Sr?", "shown as Sr?, the checkbox hides it");
    }
    // ---- 7. the params file ------------------------------------------------------------------------------------------------------
    {
        std::vector<std::string> L = { "VERSION,1", "MARKET,GC", "ASOF,1", "SOURCE,x", "# c", "PARAM,srMinPrices,3", "PARAM,srStaySec,2", "PARAM,stRatio,3.0", "PARAM,unknown,9", "SR_BULL_SHOW,SHOW", "SR_BEAR_SHOW,LOG", "STA_BULL_SHOW,?" };
        swg::StopSweep X = swg::parseStopSweep(L, "GC");
        CHECK(X.loaded && X.srMinPrices == 3 && X.srStaySec == 2 && X.stRatio == 3.0 && X.srBull == swg::SHOW && X.srBear == swg::LOG && X.staBull == swg::QUERY, "GC params parsed");
        Params P; applyParams(P, X); CHECK(P.minPrices == 3 && P.stayS == 2 && P.windowS == 5 && P.refillPct == 50 && P.showBull == swg::SHOW, "applied to the detector");
        CHECK(!swg::parseStopSweep(L, "NQ").loaded && swg::parseStopSweep(L, "NQ").srBull == swg::LOG, "wrong market: defaults, LOG");
        std::vector<std::string> B = L; B[5] = "PARAM,srMinPrices,abc"; CHECK(!swg::parseStopSweep(B, "GC").loaded, "bad value rejected");
        B = L; B[9] = "SR_BULL_SHOW,YES"; CHECK(!swg::parseStopSweep(B, "GC").loaded, "bad verdict rejected");
        B = L; B.erase(B.begin()); CHECK(!swg::parseStopSweep(B, "GC").loaded, "VERSION must be first");
        CHECK(!swg::parseStopSweep(std::vector<std::string>(), "GC").loaded, "empty = defaults");
        swg::StopSweep D0; CHECK(D0.srBull == swg::LOG && D0.srBear == swg::LOG && D0.staBull == swg::LOG && D0.stGrey == swg::LOG, "defaults: every mark LOG");
        // a LOG verdict: refilled and recorded, never shown
        Sim s; swg::StopSweep Y; applyParams(s.D.P, Y); s.candles(dayA());
        const long long base = (s.lastEnd() + 30) * 1000;
        const std::vector<long long> ids = s.rest(BID, 2003, 10, 10, base - 5000);
        const long long end = s.sweep(BID, ids, 2003, 10, base); s.D.advance(end + 60); s.rest(BID, 2003, 10, 9, end + 100); s.D.advance(end + 10000);
        CHECK(s.D.decided.size() == 1 && s.D.decided[0].refilled && s.D.decided[0].show == swg::LOG && !shown(s.D.decided[0], true, true), "LOG: recorded, not shown");
    }
    // ---- 8. determinism (no repaint): the same stream twice gives byte-identical records ---------------------------------------
    {
        std::string out[2];
        for (int r = 0; r < 2; ++r) {
            Sim s; s.candles(dayA());
            const long long base = (s.lastEnd() + 30) * 1000;
            const std::vector<long long> ids = s.rest(BID, 2003, 12, 6, base - 5000);
            const long long end = s.sweep(BID, ids, 2003, 6, base);
            s.D.advance(end + 60); s.rest(BID, 2003, 12, 4, end + 700); s.D.advance(end + 20000);
            for (size_t k = 0; k < s.D.decided.size(); ++k) out[r] += recLine(s.D.decided[k], 0.25) + "\n";
        }
        CHECK(!out[0].empty() && out[0] == out[1], "same stream -> identical record");
    }
    // ---- 9. garbage events never crash or count ------------------------------------------------------------------------------
    {
        Sim s; s.ev(9, BID, 100, 5, 1, 1000); s.ev(NEW, -1, 100, 5, 2, 1000); s.ev(NEW, BID, 0, 5, 3, 1000); s.ev(NEW, BID, 100, -4, 4, 1000); s.ev(FILL, BID, 100, 5, 99, 1000); s.ev(DEL, BID, 100, 0, 12345, 1000);
        CHECK(s.D.book.empty() && s.D.fills == 1 && s.D.unknownFills == 1, "garbage ignored; an unknown fill still counts as a print");
        swg::BarWin W; CHECK(W.atr(100) == 0, "no candles: ATR 0");
    }
    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
