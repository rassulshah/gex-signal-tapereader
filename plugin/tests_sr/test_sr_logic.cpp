// lsSweepRefill 1.0.0 logic tests (SweepRefillLogic.h): sweep detection, pre / post with the 1 s flicker rule (no look-ahead),
// the 4 s decision, the level / pivot / 5-candle "taken out" rule (v3), the local-extreme gate, the size gates, the no-adjacent rule,
// the bearish mirror, the record round trip, determinism (no repaint), the clock.
#include "../SweepRefillLogic.h"
#include <cstdio>
static int fails = 0, checks = 0;
#define CHECK(c, m) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
using namespace swr;

static const long long T0 = 1791550800;   // a bar END (CT seconds)
struct Sim {
    Detector D; long long id = 1000;
    Sim() { D.P.spb = 180; }
    // closed candles: 8 pre-move candles around `mid` with lows >= floor, highs <= ceil
    void history(long long floor, long long ceil, int nBars = 8, long long t0 = T0) {
        for (int i = 0; i < nBars; ++i) { Bar b; b.t = t0 + 180LL * i; b.l = floor + (i % 3); b.h = ceil - (i % 2); b.c = (floor + ceil) / 2; b.session = 1; D.barClose(b); }
    }
    long long lastEnd() const { return D.barEnds.back(); }
    void ev(int a, int side, long long px, long long sz, long long oid, long long ms) { Ev e; e.action = a; e.side = side; e.px = px; e.size = sz; e.id = oid; e.ms = ms; e.sec = ms / 1000; D.onEvent(e); }
    // resting orders: n prices from top down (bids) or bottom up (asks), `lots` each, added at ms
    std::vector<long long> rest(int side, long long from, int n, long long lots, long long ms) {
        std::vector<long long> ids;
        for (int k = 0; k < n; ++k) { const long long px = side == BID ? from - k : from + k; ev(NEW, side, px, lots, ++id, ms); ids.push_back(id); }
        return ids;
    }
    // the sweep: fills every resting order in turn, 2 ms apart
    long long sweep(int side, const std::vector<long long>& ids, long long from, long long lots, long long ms) {
        for (size_t k = 0; k < ids.size(); ++k) { const long long px = side == BID ? from - (long long)k : from + (long long)k; ev(FILL, side, px, lots, ids[k], ms + 2 * (long long)k); ev(DEL, side, px, 0, ids[k], ms + 2 * (long long)k); }
        return ms + 2 * (long long)(ids.size() - 1);
    }
};

int main() {
    // keyDir
    CHECK(keyDir("ONL") == -1 && keyDir("LonHI") == 1 && keyDir("PFH/PDH") == 1 && keyDir("PDL / PFL") == -1 && keyDir("VAL") == 0 && keyDir("PWL") == 0, "key names -> side");
    // clock: monotone, the arrival ms orders events inside a second, never across
    { Clock c; long long a = c.at(100, 5000), b = c.at(100, 5040), d = c.at(101, 5050), e = c.at(100, 5060), f = c.idle(9050);
      CHECK(a == 100000 && b == 100040 && d == 101000 && e == 101010 && f >= 104000, "clock"); }

    // ---- 1. a bullish Sr: 10 bid prices swept through ONL, 80% back in 3 s -----------------------------------------------------
    {
        Sim s; s.history(2000, 2040);                    // pre-move candles: lows >= 2000
        const long long base = (s.lastEnd() + 30) * 1000;  // inside the forming candle
        Level onl; onl.name = "ONL"; onl.px = 1997; onl.kind = REF_KEY; onl.dir = -1; s.D.levels.push_back(onl);
        const std::vector<long long> ids = s.rest(BID, 2000, 10, 10, base - 5000);    // 100 lots resting on 2000..1991 for 5 s
        const long long end = s.sweep(BID, ids, 2000, 10, base);
        CHECK(s.D.b.on && s.D.pending.empty(), "burst open until the window passes");
        s.D.advance(end + 60);
        CHECK(s.D.pending.size() == 1 && s.D.bigSweeps == 1 && s.D.tookOut == 1, "one candidate");
        CHECK(s.D.pending.size() == 1 && s.D.pending[0].e.pre == 100 && s.D.pending[0].e.prices == 10 && s.D.pending[0].e.ref == "ONL", "pre = 100 lots on 10 prices, through ONL");
        s.rest(BID, 2000, 10, 8, end + 500);             // 80 lots back 0.5 s after
        s.ev(NEW, BID, 1995, 50, 777777, end + 2600);    // a flicker order: added 2.6 s, gone 3.2 s -> lived 0.6 s
        s.D.advance(end + 3000);
        s.ev(DEL, BID, 1995, 0, 777777, end + 3200);
        s.D.advance(end + 3900);
        CHECK(s.D.decided.empty(), "not decided before 4 s");
        s.D.advance(end + 4000);
        CHECK(s.D.decided.size() == 1, "decided at 4 s");
        if (s.D.decided.size() == 1) {
            const Event& e = s.D.decided[0];
            CHECK(e.post == 80 && e.refilled && e.gapOk && bullish(e) && e.refKind == REF_KEY, "post 80 (flicker excluded), refilled, drawn");
            CHECK(e.ttrMs >= 400 && e.ttrMs <= 600, "time to refill ~0.5 s");
            CHECK(e.barT == s.lastEnd() + 180, "on the forming candle (its END key)");
            CHECK(shown(e, true, true) && !shown(e, false, true), "bullish checkbox");
            // record round trip
            const std::string ln = recLine(e, 0.25); std::vector<std::string> L; L.push_back(REC_HEAD); L.push_back(ln); L.push_back(ln); L.push_back("SR|torn"); L.push_back("XX|1");
            long long bad = 0; const std::vector<Event> v = parseRecord(L, 0.25, &bad);
            CHECK(v.size() == 1 && bad == 2 && v[0].key == e.key && v[0].post == 80 && v[0].refilled && v[0].gapOk && v[0].refKind == REF_KEY && v[0].barT == e.barT && recLine(v[0], 0.25) == ln, "record round trip, duplicate key ignored, bad lines counted");
            const std::vector<std::string> h = hoverRows(e, 0.25, 3, 70);
            CHECK(h.size() == 8 && h[0].find("Sweep refilled?") != std::string::npos && h[1].find("Swept 10 prices (500.00 - 497.75) through ONL 499.25 - refilled 80% in 3 s - bids defending") == 0, "hover read line");
            CHECK(h[3].find("Question:") == 0 && h[4].find("What it is:") == 0 && h[5].find("Why it matters:") == 0 && h[6].find("How to use:") == 0 && h[7].find("Example:") == 0, "hover structure");
        }
        // ---- 2. the next candle: refilled again -> recorded, NOT drawn (no adjacent); 2 candles later -> drawn ------------------
        s.D.decided.clear();
        Bar c; c.t = s.lastEnd() + 180; c.l = 1990; c.h = 2010; c.c = 2000; c.session = 1; s.D.barClose(c);
        const long long b2 = (s.lastEnd() + 20) * 1000;
        const std::vector<long long> i2 = s.rest(BID, 1990, 9, 10, b2 - 3000);
        const long long e2 = s.sweep(BID, i2, 1990, 10, b2);
        s.D.advance(e2 + 60); s.rest(BID, 1990, 9, 10, e2 + 300); s.D.advance(e2 + 4000);
        CHECK(s.D.decided.size() == 1 && s.D.decided[0].refilled && !s.D.decided[0].gapOk, "adjacent candle: recorded only");
        CHECK(s.D.decided.size() == 1 && !shown(s.D.decided[0], true, true), "adjacent: not shown");
    }
    // ---- 3. not refilled: recorded, not drawn; the burst rules ---------------------------------------------------------------
    {
        Sim s; s.history(2000, 2040);
        const long long base = (s.lastEnd() + 30) * 1000;
        const std::vector<long long> ids = s.rest(BID, 2000, 8, 5, base - 5000);
        const long long end = s.sweep(BID, ids, 2000, 5, base);
        s.D.advance(end + 4100);
        CHECK(s.D.decided.size() == 1 && !s.D.decided[0].refilled && s.D.decided[0].post == 0 && s.D.decided[0].refKind == REF_PIVOT && s.D.decided[0].refPx == 2000, "empty: recorded, through the pivot low 2000");
    }
    // ---- 4. gates: 7 prices < 8; 25 lots < 30; not at a local low; nothing intact taken out ----------------------------------
    {
        Sim s; s.history(2000, 2040);
        long long base = (s.lastEnd() + 30) * 1000;
        std::vector<long long> ids = s.rest(BID, 2000, 7, 10, base - 5000); long long end = s.sweep(BID, ids, 2000, 10, base); s.D.advance(end + 5000);
        CHECK(s.D.candidates == 0 && s.D.sweeps == 1, "7 prices: a sweep, not a candidate");
        base = end + 6000; ids = s.rest(BID, 1999, 10, 2, base - 5000); end = s.sweep(BID, ids, 1999, 2, base); s.D.advance(end + 5000);
        CHECK(s.D.candidates == 0 && s.D.bigSweeps == 0, "20 lots resting: not big enough");
        // a fresh book on the ASK side above, a buy sweep that stays below the 5-candle high: nothing taken out
        base = end + 6000; ids = s.rest(ASK, 2020, 10, 10, base - 5000); end = s.sweep(ASK, ids, 2020, 10, base); s.D.advance(end + 5000);
        CHECK(s.D.candidates == 0 && s.D.bigSweeps == 1 && s.D.tookOut == 0, "inside the range: no level / pivot taken out");
    }
    {   // the old low was much lower in the last 15 min -> not a local low
        Sim s; s.history(2000, 2040);
        s.D.trade(1950, s.lastEnd() - 100);
        const long long base = (s.lastEnd() + 30) * 1000;
        const std::vector<long long> ids = s.rest(BID, 2000, 10, 10, base - 5000); const long long end = s.sweep(BID, ids, 2000, 10, base); s.D.advance(end + 5000);
        CHECK(s.D.bigSweeps == 1 && s.D.atExtreme == 0 && s.D.candidates == 0, "not at the 15-min low: no candidate");
    }
    // ---- 5. flicker in pre: orders added < 1 s before the sweep are not counted ------------------------------------------------
    {
        Sim s; s.history(2000, 2040);
        const long long base = (s.lastEnd() + 30) * 1000;
        const std::vector<long long> a = s.rest(BID, 2000, 10, 4, base - 5000);
        // 6 more lots per price arriving 0.3 s before the sweep (flicker)
        std::vector<long long> b; for (int k = 0; k < 10; ++k) { s.ev(NEW, BID, 2000 - k, 6, ++s.id, base - 300); b.push_back(s.id); }
        for (int k = 0; k < 10; ++k) { s.ev(FILL, BID, 2000 - k, 4, a[k], base + 2 * k); s.ev(DEL, BID, 2000 - k, 0, a[k], base + 2 * k); s.ev(FILL, BID, 2000 - k, 6, b[k], base + 2 * k + 1); s.ev(DEL, BID, 2000 - k, 0, b[k], base + 2 * k + 1); }
        s.D.advance(base + 100);
        CHECK(s.D.pending.size() == 1 && s.D.pending[0].e.pre == 40, "pre ignores orders younger than 1 s");
    }
    // ---- 6. the bearish mirror through a pivot high taken out by the LAST candle (v3: NQ 9:55) -------------------------------
    {
        Sim s;
        // candles: a 5-candle pivot high at 2050 (bar 3), lower highs around it, then the last closed candle takes it out (2052)
        const long long H[9] = { 2040, 2044, 2046, 2050, 2045, 2043, 2041, 2042, 2052 };
        for (int i = 0; i < 9; ++i) { Bar b; b.t = T0 + 180LL * i; b.h = H[i]; b.l = H[i] - 20; b.c = H[i] - 5; b.session = 1; s.D.barClose(b); }
        CHECK(!s.D.pivots.empty() && s.D.pivots[0].px == 2050 && s.D.pivots[0].brokenAt == T0 + 180LL * 8, "pivot high found, taken out by the last candle");
        const long long base = (s.lastEnd() + 30) * 1000;
        const std::vector<long long> ids = s.rest(ASK, 2055, 9, 8, base - 5000);
        const long long end = s.sweep(ASK, ids, 2055, 8, base);
        s.D.advance(end + 60); s.rest(ASK, 2055, 9, 7, end + 400); s.D.advance(end + 4000);
        CHECK(s.D.decided.size() == 1 && !bullish(s.D.decided[0]) && s.D.decided[0].refilled && s.D.decided[0].refKind == REF_PIVOT && s.D.decided[0].refPx == 2050, "bearish Sr through the pivot high taken out by the last candle");
        CHECK(s.D.decided.size() == 1 && shown(s.D.decided[0], true, true) && !shown(s.D.decided[0], true, false), "bearish shown by default, the checkbox hides it");
    }
    {   // a pivot taken out BEFORE the move window does not count (falls back to the 5-candle high)
        Sim s;
        const long long H[11] = { 2040, 2044, 2046, 2050, 2045, 2043, 2052, 2048, 2047, 2049, 2051 };
        for (int i = 0; i < 11; ++i) { Bar b; b.t = T0 + 180LL * i; b.h = H[i]; b.l = H[i] - 20; b.c = H[i] - 5; b.session = 1; s.D.barClose(b); }
        const long long base = (s.lastEnd() + 30) * 1000;
        const std::vector<long long> ids = s.rest(ASK, 2053, 9, 8, base - 5000);
        const long long end = s.sweep(ASK, ids, 2053, 8, base); s.D.advance(end + 4100);
        CHECK(s.D.decided.size() == 1 && s.D.decided[0].refPx == 2052, "old pivot 2050 (taken out before the move) ignored; 2052 taken out");
    }
    // ---- 7. determinism (no repaint): the same stream twice gives byte-identical records ---------------------------------------
    {
        std::string out[2];
        for (int r = 0; r < 2; ++r) {
            Sim s; s.history(2000, 2040);
            const long long base = (s.lastEnd() + 30) * 1000;
            const std::vector<long long> ids = s.rest(BID, 2000, 12, 6, base - 5000);
            const long long end = s.sweep(BID, ids, 2000, 6, base);
            s.D.advance(end + 60); s.rest(BID, 2000, 12, 4, end + 700); s.D.advance(end + 10000);
            for (size_t k = 0; k < s.D.decided.size(); ++k) out[r] += recLine(s.D.decided[k], 0.25) + "\n";
        }
        CHECK(!out[0].empty() && out[0] == out[1], "same stream -> identical record");
    }
    // ---- 8. garbage events never crash or count ------------------------------------------------------------------------------
    {
        Sim s; s.ev(9, BID, 100, 5, 1, 1000); s.ev(NEW, -1, 100, 5, 2, 1000); s.ev(NEW, BID, 0, 5, 3, 1000); s.ev(NEW, BID, 100, -4, 4, 1000); s.ev(FILL, BID, 100, 5, 99, 1000); s.ev(DEL, BID, 100, 0, 12345, 1000);
        CHECK(s.D.book.empty() && s.D.fills == 1 && s.D.unknownFills == 1, "garbage ignored; an unknown fill still counts as a print");
    }
    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
