// test_lmz.cpp -- LmtZonesLogic.h 1.2.0 (lsLiquidityProfile 0.53.0: grey map lines, pooled draw rule, FILLING / HELD? tags; 1.0.0 = 0.48.0 mockup v3).
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra -Werror -I. tests_lmz/test_lmz.cpp -o tlmz && ./tlmz
#include "LmtZonesLogic.h"
#include <cstdio>
using namespace lmz;
static int P = 0, F = 0;
#define CHECK(c, m) do { if (c) ++P; else { ++F; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static const long long T0 = 1792656000000LL + 8LL * 3600000LL;   // a CT-epoch day, 08:00
// bars that walk through waypoints, 2 ticks per bar, 6-tick range; red when falling, green when rising
static std::vector<Bar> path(const std::vector<Tick>& wp, long long t0, int& n) {
    std::vector<Bar> v; Tick p = wp[0]; long long t = t0;
    for (size_t w = 1; w < wp.size(); ++w) {
        while (p != wp[w]) {
            const Tick q = p + (wp[w] > p ? 2 : -2);
            Bar b; b.endMs = (t += 180000); b.o = p; b.c = q; b.h = std::max(p, q) + 2; b.l = std::min(p, q) - 2; v.push_back(b); p = q;
        }
    }
    n = (int)v.size(); return v;
}
int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    // 1) the zigzag / breakouts / trend: a down staircase gives with-trend BDs; a counter rally break is internal
    {
        Engine E; E.market = "ES"; E.R = ruleFor("ES"); E.W.fixedAtr = 20; int n = 0;
        const std::vector<Bar> B = path({ 1000, 960, 984, 940, 964, 920, 944, 900 }, T0, n);
        for (const Bar& b : B) E.closeBar(b, 45);
        int bd = 0, bdWith = 0, bo = 0; for (const Break& b : E.W.brk) { if (b.dir < 0) { ++bd; if (b.with) ++bdWith; } else ++bo; }
        std::printf("  waves: %zu pivots, %d BD (%d with-trend), %d BO, atr %.2f\n", E.W.piv.size(), bd, bdWith, bo, E.W.atr);
        { Engine A; for (const Bar& b : B) A.closeBar(b, 45); CHECK(A.W.atr > 5.9 && A.W.atr < 6.1, "ATR = 14-bar true range of closed bars (6 ticks here)"); }
        CHECK(bd >= 2 && bdWith >= 1, "down staircase: breakdowns below confirmed lows, at least one with the trend");
        bool redOnly = true; for (const Break& b : E.W.brk) if (b.dir < 0) { const Bar& c = E.W.bars[(size_t)b.bar]; if (!(c.c < c.o)) redOnly = false; }
        CHECK(redOnly, "BD only on a red candle");
        bool origin = true; for (const Break& b : E.W.brk) if (b.dir < 0 && (!b.haveOrigin || b.origin <= b.level)) origin = false;
        CHECK(origin, "every BD has an origin high above the broken low");
    }
    // 2) zones: ES - a real offer in the with-trend BD print area -> PRINT zone, drawn with "?"; a near plain offer -> logged only
    Engine E; E.market = "ES"; E.R = ruleFor("ES"); E.W.fixedAtr = 20; int n = 0;
    std::vector<Bar> B = path({ 1000, 960, 984, 940, 964, 920, 944, 900 }, T0, n);
    // find the last with-trend BD to place the order
    { Engine probe; probe.R = E.R; probe.W.fixedAtr = 20; for (const Bar& b : B) probe.closeBar(b, 45);
      Break last; bool have = false; for (const Break& b : probe.W.brk) if (b.dir < 0 && b.with) { last = b; have = true; }
      CHECK(have, "a with-trend BD exists in the path");
      // run bars up to that BD, then feed orders, then continue
      const long long tBD = last.endMs; size_t i = 0;
      for (; i < B.size() && B[i].endMs <= tBD; ++i) E.closeBar(B[i], 45);
      const long long t = tBD + 1000;
      E.onEvent(NEW, ASK, last.level, 100, 11, t);                    // real: 100 lots at the broken level (print area)
      E.onEvent(NEW, ASK, last.close + 9, 60, 12, t);                 // a second offer, above the print area (plain near)
      E.onEvent(NEW, ASK, last.level + 1, 10, 13, t);                 // small: never a zone
      E.onEvent(NEW, BID, last.close - 4, 120, 14, t);                // a bid: log only everywhere
      E.onEvent(NEW, ASK, last.close + 15, 300, 15, t + 170000);      // big but rested only 10 s at the next close: not yet
      // (1.1.0) the live book around price: 41 small resting bids of 4 lots -> median 4 lots -> LMT >= max(ES floor 10, 3 x 4 = 12) = 12 lots
      for (int k = 0; k < 41; ++k) E.onEvent(NEW, BID, last.close - 12 - (k % 8), 4, 500 + k, t);
      // the next bar closes 3 min later: orders rested >= 30 s, the feed known long enough (30 s + 30 s)
      E.closeBar(B[i], 45); ++i;
      int nPrint = 0, nNear = 0, nBid = 0; bool printQ = false, nearDrawn = true, bidDrawn = true;
      for (const Zone& z : E.zones) { if (z.kind == K_PRINT && z.side == ASK) { ++nPrint; printQ = z.q && z.drawn; } if (z.kind == K_NEAR && z.side == ASK) { ++nNear; nearDrawn = z.drawn; } if (z.side == BID) { ++nBid; bidDrawn = z.drawn; } }
      std::printf("  zones after the first close: %zu (print %d, near %d, bid %d)\n", E.zones.size(), nPrint, nNear, nBid);
      bool printDrawn = false; for (const Zone& z : E.zones) if (z.kind == K_PRINT && z.side == ASK) printDrawn = z.drawn;
      CHECK(nPrint == 1 && printDrawn && !printQ, "(1.2.0) ES: the with-trend BD-print offer is a drawn zone, no ?");
      bool p11 = false; for (const Zone& z : E.zones) if (z.oid == 11 && z.kind == K_PRINT) p11 = true;
      bool anyQ = false; for (const Zone& z : E.zones) if (z.q) anyQ = true;
      CHECK(p11, "the print zone holds order 11 (the biggest real order in the print area)");
      CHECK(nNear == 0 || nearDrawn, "(1.2.0) pooled: a near offer is drawn in ES too (no per-market exception)");
      CHECK(!anyQ, "(1.2.0) no zone carries the ? any more");
      CHECK(nBid >= 1 && !bidDrawn, "bids: tracked (LMT columns) but never drawn");
      bool small = false, young = false; for (const Zone& z : E.zones) { if (z.oid == 13) small = true; if (z.oid == 15) young = true; }
      CHECK(!small, "an order under S20 is never a zone");
      CHECK(!young, "an order that has not rested 30 s is not a zone yet");
      std::string lab; for (const Zone& z : E.zones) if (z.kind == K_PRINT) lab = label(z);
      CHECK(lab == "LMT 100 (PRINT)", "(1.2.0) label: LMT 100 (PRINT)");
      // 3) the zone follows a first re-price inside its band, then a 2nd move ends it (X)
      int zi = -1; for (size_t k = 0; k < E.zones.size(); ++k) if (E.zones[k].oid == 11) zi = (int)k;
      const Tick lo = E.zones[(size_t)zi].segs[0].lo;
      E.onEvent(MOD, ASK, lo, 100, 11, B[i].endMs - 60000);
      CHECK(E.zones[(size_t)zi].end == LIVE && E.zones[(size_t)zi].segs.size() == 2 && E.zones[(size_t)zi].px() == lo, "a first re-price inside the band: the zone follows (new segment)");
      E.onEvent(MOD, ASK, lo + 1, 100, 11, B[i].endMs - 50000);
      CHECK(E.zones[(size_t)zi].end == MOVED, "a second re-price ends it (MOVED, X)");
      // 4) the bid: partly filled, then pulled -> PULLED with LMT F lots kept
      int bi = -1; for (size_t k = 0; k < E.zones.size(); ++k) if (E.zones[k].oid == 14) bi = (int)k;
      E.onEvent(FILL, ASK, last.close - 4, 30, 14, B[i].endMs - 40000);      // the FILL row carries the aggressor's side
      CHECK(E.zones[(size_t)bi].traded == 30 && E.zones[(size_t)bi].side == BID, "fills counted on the order's own side (not the aggressor's)");
      E.onEvent(MOD, ASK, last.close - 4, 90, 14, B[i].endMs - 39000);
      E.onEvent(DEL, ASK, last.close - 4, 90, 14, B[i].endMs - 30000);
      CHECK(E.zones[(size_t)bi].end == PULLED && E.zones[(size_t)bi].traded == 30, "partly filled then pulled: PULLED, LMT F keeps 30");
      const std::vector<int> fz = E.filledZones(B[i].endMs, 3600000);
      CHECK(fz.size() == 1 && fz[0] == bi, "LMT F = the same order ids as the zones");
      // 5) a fully filled order: FILL + DEL -> FILLED
      int ni = -1; for (size_t k = 0; k < E.zones.size(); ++k) if (E.zones[k].oid == 12) ni = (int)k;
      if (ni >= 0) { E.onEvent(FILL, BID, last.close + 9, 60, 12, B[i].endMs - 20000); E.onEvent(DEL, BID, last.close + 9, 0, 12, B[i].endMs - 20000);
          CHECK(E.zones[(size_t)ni].end == FILLED && E.zones[(size_t)ni].traded == 60, "filled away: FILLED (ring)"); }
      // 6) record round trip: restore = identical zones (no repaint)
      std::vector<std::string> rec = E.rows;
      Engine R2; R2.R = E.R; long long bad = -1; const size_t got = R2.restore(rec, &bad);
      CHECK(got == E.zones.size() && bad == 0, "restore reads every Z line, no bad lines");
      bool same = got == E.zones.size();
      for (size_t k = 0; same && k < got; ++k) { const Zone& a = E.zones[k]; const Zone& b = R2.zones[k];
          same = a.oid == b.oid && a.kind == b.kind && a.end == b.end && a.t0 == b.t0 && a.t1 == b.t1 && a.segs.size() == b.segs.size() && a.px() == b.px() && a.traded == b.traded && a.drawn == b.drawn && a.q == b.q; }
      CHECK(same, "restored zones are identical (start, segments, end, fills, drawn, ?)");
      std::vector<std::string> torn = rec; torn.push_back("Z|99|1|1|PRINT|5"); torn.push_back("E|1|5|WHAT|0"); torn.push_back("garbage");
      Engine R3; long long bad3 = 0; R3.restore(torn, &bad3); CHECK(bad3 == 3, "torn / unknown lines are counted and skipped");
      // 7) the session ends every live zone once
      for (; i < B.size(); ++i) E.closeBar(B[i], 45);
      E.endSession(B.back().endMs + 1);
      bool none = true; for (const Zone& z : E.zones) if (z.end == LIVE) none = false;
      CHECK(none && E.live.empty(), "session end: no live zone left");
      int ends = 0; for (const std::string& r : E.rows) if (r[0] == 'E') ++ends;
      CHECK(ends == (int)E.zones.size(), "exactly one E line per zone (ends once)");
    }
    // 8) per-market rules
    {   // (1.2.0) one pooled rule in all 7 markets; FAR as before (off in 6E); bids never; floors per market unchanged
        bool same = true; for (const char* m : { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" }) { Rule r = ruleFor(m); if (!(r.print == 1 && r.origin == 1 && r.nearO == 1 && r.abc && !r.gammaOnly && !r.bids && !logOnly(r))) same = false; }
        CHECK(same, "(1.2.0) all 7 markets: PRINT / ABC / ORIGIN / NEAR drawn, no gamma-only, no ?, bids never");
        CHECK(ruleFor("NG").farOn && ruleFor("ES").farOn && !ruleFor("EU").farOn, "FAR stays as it was (on; 6E off)");
        CHECK(logOnly(ruleFor("ZZ")), "a market that is not one of the 7: log only"); }
    // 9) CL confluence: the near offer is drawn only on a gamma level; the diamond flag
    {
        for (int withG = 0; withG < 2; ++withG) {
            Engine C; C.market = "CL"; C.R = ruleFor("CL"); int m = 0;
            std::vector<Bar> B2 = path({ 1000, 1010, 1000, 1010, 1000, 1010, 1000, 1010, 1000 }, T0, m);
            for (size_t k = 0; k + 1 < B2.size(); ++k) C.closeBar(B2[k], 15);
            const Tick px = B2[B2.size() - 2].c + 5;
            if (withG) C.setGamma({ px + 1 });
            C.onEvent(NEW, ASK, px, 40, 21, B2.back().endMs - 170000);
            for (int k = 0; k < 21; ++k) C.onEvent(NEW, BID, B2[B2.size() - 2].c - 3 - (k % 5), 3, 600 + k, B2.back().endMs - 170000);   // (1.1.0) median 3 lots -> LMT >= 9
            C.closeBar(B2.back(), 15);
            bool made = false, drawn = false, conf = false; for (const Zone& z : C.zones) if (z.oid == 21) { made = true; drawn = z.drawn; conf = z.conf; }
            CHECK(made, "CL: the near offer is tracked either way");
            CHECK(drawn && conf == (withG == 1), withG ? "(1.2.0) CL on a gamma level: drawn + diamond" : "(1.2.0) CL off gamma: drawn too (no gamma-only rule), no diamond");
        }
    }
    // 10) far orders: thin, at most 2 per side, beyond 2 ATR; they end NEARED when price comes within 2 ATR (then may become near)
    {
        Engine G; G.market = "NG"; G.R = ruleFor("NG"); int m = 0;
        std::vector<Bar> B2 = path({ 1000, 1010, 1000, 1010, 1000, 1010, 1000, 1010, 1000 }, T0, m);
        for (size_t k = 0; k + 1 < B2.size(); ++k) G.closeBar(B2[k], 10);
        const Tick c = B2[B2.size() - 2].c; const long long t = B2.back().endMs - 170000;
        for (int q = 0; q < 4; ++q) G.onEvent(NEW, ASK, c + 14 + q, 20 + q, 31 + q, t);   // 14-17 ticks above = 2.3 - 2.8 ATR (ATR 6)
        G.closeBar(B2.back(), 10);
        int nFar = 0; for (const Zone& z : G.zones) if (z.kind == K_FAR) ++nFar;
        CHECK(nFar == 1, "far: the single biggest beyond 2 ATR (thin map line)");
        CHECK(!G.zones.empty() && label(G.zones[0]) == "22", "far label = the size only (biggest first; the 23-lot order is past 3 ATR)");
    }
    // 11) params file: strict; wrong market / version = defaults
    {
        Rule r = parseParams({ "VERSION,1", "MARKET,ES", "PARAM,NEAR,0", "PARAM,S20_RTH,60", "PARAM,BIDS,1", "PARAM,GAMMA_ONLY,1", "PARAM,JUNK,5" }, "ES");
        CHECK(r.nearO == 1 && r.s20Rth == 60 && !r.bids && !r.gammaOnly && r.src.find("params file") == 0 && r.src.find("3 per-market draw rows ignored") != std::string::npos,
              "(1.2.0) params: thresholds read; the old per-market draw rows (NEAR / BIDS / GAMMA_ONLY) ignored - bids stay off");
        r = parseParams({ "VERSION,1", "MARKET,NQ", "PARAM,S20_RTH,60" }, "ES"); CHECK(r.s20Rth == 45 && r.src.find("MARKET") != std::string::npos, "wrong market: defaults");
        r = parseParams({ "VERSION,2", "MARKET,ES" }, "ES"); CHECK(r.src.find("VERSION") != std::string::npos, "wrong version: defaults");
        r = parseParams({ "VERSION,1", "MARKET,ES", "PARAM,TAG_ATR,7", "PARAM,S20_RTH,1e9" }, "ES"); CHECK(r.tagAtr == 0.25 && r.s20Rth == 45, "out-of-range values ignored");
        r = parseParams({ "VERSION,1", "MARKET,ES", "PARAM,FAST_ATR,0.9", "PARAM,DRIFT_ATR,1.5", "PARAM,EATEN_S,45" }, "ES"); CHECK(r.fastAtr == 0.9 && r.driftAtr == 0.9 && r.eatenS == 45, "(1.2.0) FAST / DRIFT / EATEN params; DRIFT never above FAST");
    }
    // 12) settings line round trip + garbled; the checkboxes hide kinds
    {
        Settings s; s.print = false; s.cOffer = 0x123456; s.tags = false; Settings b; CHECK(parseSettings(settingsLine(s), b) && !b.print && b.cOffer == 0x123456 && !b.tags && settingsLine(s).compare(0, 5, "LMZ3,") == 0, "(1.3.0) settings round trip (LMZ3 with the after-fill box)");
        { Settings o2; CHECK(parseSettings("LMZ2,1,1,1,1,1,1,1,1,15680580,2278750,9080728,7041664,0", o2) && o2.tags, "(1.3.0) an LMZ2 file (0.53, old State tags box off) loads with the after-fill tags ON (default)"); }
        Settings o; CHECK(parseSettings("LMZ1,1,0,1,1,1,1,1,1,15680580,2278750,9080728,7041664", o) && !o.print && o.tags, "(1.2.0) an old LMZ1 file (0.48-0.52) still loads: tags default on");
        CHECK(!parseSettings("LMZ2,1,1,1,1,1,1,1,1,0,0,0,0,2", o) && !parseSettings("LMZ2,1,1,1,1,1,1,1,1,0,0,0,0", o), "LMZ2 garbled / short tags field rejected");
        CHECK(!parseSettings("LMZ1,1,1", b) && !parseSettings("LMZ1,1,1,1,1,1,1,1,2,0,0,0,0", b), "garbled settings rejected");
        Zone z; z.kind = K_PRINT; z.drawn = true; CHECK(shown(z, Settings()) && !shown(z, s), "print checkbox hides print zones");
        z.drawn = false; CHECK(!shown(z, Settings()), "a log-only zone is never drawn");
    }
    // 13) (1.2.0) hover: <= 12 rows, <= 118 chars; target / magnet, the honest odds, x median, age at contact, traded %, status
    {
        Zone z; z.kind = K_PRINT; z.side = ASK; z.size0 = 100; z.maxShown = 100; z.med = 8; z.s20 = 45; z.t0 = T0; z.restMs = 95000; z.tContact = T0 + 145000; z.drawn = true; z.conf = true; z.confPx = 30832;
        z.traded = 30; z.tagHeldMs = T0 + 360000; Seg g; g.px = 30832; z.segs.push_back(g);
        HoverCtx c; c.market = "ES"; c.tick = 0.25; c.sizeLive = 70; c.nowMs = T0 + 400000;
        const std::vector<Row> v = hoverRows(z, c); bool fits = v.size() <= 12, magnet = false, odds = false, xm = false, age = false, traded = false, held = false, q = false, ex = false, how = false, diamond = false;
        for (const Row& r : v) { const std::string& t = r.text; if (t.size() > 118) fits = false;
            if (t.find("TARGET / magnet, not a wall") != std::string::npos) magnet = true;
            if (t.find("reverses here 30% vs 35% at a random price. Once filled: RE-BID sweeps failed 53%, ACCEPTED 39%") != std::string::npos) odds = true;
            if (t.find("13x the median resting order (8 lots)") != std::string::npos) xm = true;
            if (t.find("age at contact 4 min") != std::string::npos) age = true;
            if (t.find("traded 30 (30% of 100 shown)") != std::string::npos) traded = true;
            if (t.find("Now: RESTING") == 0 && r.ink == INK_WHITE && t.find("HELD") == std::string::npos) held = true;   // (1.3.0) HELD is never shown
            if (t.find("Question:") == 0) q = true;
            if (t.find("Example:") == 0) ex = true;
            if (t.find("FILLING = being filled") != std::string::npos) how = true;
            if (r.ink == INK_YELLOW) diamond = true; }
        for (const Row& r : v) std::printf("    | %s\n", r.text.c_str());
        CHECK(fits && q && ex && how, "hover: question / what / why / how / example, every row <= 118 chars");
        CHECK(magnet && odds, "hover: a target / magnet, not a wall + the honest odds line");
        CHECK(xm && age && traded, "hover: lots x median, age at contact, traded so far (lots, % of shown)");
        CHECK(held && diamond, "(1.3.0) hover: status RESTING (HELD never shown) + the gamma diamond row");
        {   Zone y = z; y.end = FILLED; y.t1 = T0 + 380000; y.afFillMs = T0 + 380000; y.afDecMs = T0 + 410000; y.afRead = 1; y.afStack = 42; y.afDelta = 17;
            bool rb = false, fitsY = true; for (const Row& r : hoverRows(y, c)) { if (r.text.size() > 118) fitsY = false; if (r.text.find("RE-BID") != std::string::npos && r.text.find("stack 42, delta 17") != std::string::npos && r.ink == INK_RED) rb = true; }
            CHECK(rb && fitsY, "(1.3.0) hover: an offer's RE-BID (time, stack, delta) in red, rows <= 118");
            y.afRead = 2; bool ac = false; for (const Row& r : hoverRows(y, c)) if (r.text.find("ACCEPTED") != std::string::npos && r.ink == INK_GREY) ac = true;
            CHECK(ac, "(1.3.0) hover: ACCEPTED in grey"); }
        z.end = PULLED; z.t1 = T0 + 200000; z.tContact = 0; z.tagHeldMs = 0; z.traded = 0;
        bool pulled = false; for (const Row& r : hoverRows(z, c)) if (r.text.find("PULLED before contact (X) - no read") != std::string::npos && r.ink == INK_GREY) pulled = true;
        CHECK(pulled, "hover: pulled before contact = grey, no read");
    }
    // 11) (1.1.0) the live-median rule: the threshold adapts to the book, the floor, hysteresis 2.5x, params strict
    {   Engine E; E.market = "ES"; E.R = ruleFor("ES"); E.W.fixedAtr = 20; int m = 0;
        std::vector<Bar> B3 = path({ 1000, 1010, 1000, 1010, 1000, 1010, 1000, 1010, 1000 }, T0, m);
        for (size_t k = 0; k + 1 < B3.size(); ++k) E.closeBar(B3[k], 194);   // the night S20 194 never gates any more
        const Tick c = B3[B3.size() - 2].c; const long long t = B3.back().endMs - 170000;
        for (int k = 0; k < 31; ++k) E.onEvent(NEW, k % 2 ? ASK : BID, k % 2 ? c + 3 + (k % 7) : c - 3 - (k % 7), 5, 700 + k, t);   // the book: 5-lot orders
        E.onEvent(NEW, ASK, c + 6, 40, 31, t);   // 40 lots = 8x the median: an LMT (the old S20 194 rule found 0 all night)
        E.onEvent(NEW, ASK, c + 8, 14, 32, t);   // 14 < 3 x 5 = 15: not an LMT
        E.closeBar(B3.back(), 194);
        CHECK(E.medLots == 5 && E.threshold() == 15 && E.keepLots == 13, "median 5 lots -> LMT >= 15, kept while >= 13 (2.5x)");
        bool z31 = false, z32 = false; for (const Zone& z : E.zones) { if (z.oid == 31) z31 = true; if (z.oid == 32) z32 = true; }
        CHECK(z31 && !z32, "40 lots is an LMT, 14 is not (S20 194 is context only)");
        Engine G; G.R = ruleFor("ES"); G.W.fixedAtr = 20; for (size_t k = 0; k + 1 < B3.size(); ++k) G.closeBar(B3[k], 194);
        for (int k = 0; k < 11; ++k) G.onEvent(NEW, BID, c - 3 - k, 1, 800 + k, t);   // a thin 1-lot book: 3 x 1 = 3 < the ES floor 10
        G.closeBar(B3.back(), 194); CHECK(G.threshold() == 10, "the per-market floor (ES 10) holds in a thin book");
        CHECK(floorFor("ES") == 10 && floorFor("NQ") == 5 && floorFor("CL") == 5 && floorFor("GC") == 3 && floorFor("HG") == 3 && floorFor("NG") == 3 && floorFor("EU") == 5, "floors per market");
        Rule r = parseParams({ "VERSION,1", "MARKET,ES", "PARAM,MULT,4.0", "PARAM,KEEP_MULT,5", "PARAM,FLOOR,12" }, "ES");
        CHECK(r.mult == 4.0 && r.keepMult == 4.0 && r.floorLots == 12, "params: MULT / FLOOR read; KEEP_MULT never above MULT");
        r = parseParams({ "VERSION,1", "MARKET,ES", "PARAM,MULT,nan", "PARAM,MULT,0.5" }, "ES"); CHECK(r.mult == 3.0, "garbled / out-of-range MULT ignored (default 3.0)");
        // hysteresis: the order shrinks to 14 (still >= 13): kept; then 12: SHRANK
        int zi = -1; for (size_t k = 0; k < E.zones.size(); ++k) if (E.zones[k].oid == 31) zi = (int)k;
        Bar nb = B3.back(); nb.endMs += 180000; E.onEvent(MOD, ASK, c + 6, 14, 31, nb.endMs - 100000); E.closeBar(nb, 194);
        CHECK(zi >= 0 && E.zones[(size_t)zi].end == LIVE, "shrank to 14 (>= 2.5x = 13): kept (no flicker)");
        nb.endMs += 180000; E.onEvent(MOD, ASK, c + 6, 12, 31, nb.endMs - 100000); E.closeBar(nb, 194);
        CHECK(zi >= 0 && E.zones[(size_t)zi].end == SHRANK && std::string(endCode(SHRANK)) == "SHRANK" && endOf("SHRANK") == SHRANK, "under 2.5x: ends SHRANK (recorded once)");
    }
    // ================= (1.2.0) STATE TAGS =================
    // a NEAR offer zone: ATR fixed 20 ticks -> tag band w = 5 ticks, tolerance = +10; a 5-lot book -> LMT >= 15 lots
    struct Setup { Engine E; std::vector<Bar> B; Tick c = 0, px = 0; int zi = -1; long long tEnd = 0; };
    auto setup = [&](const std::string& m, Side side, Setup& U) {
        U.E = Engine(); U.E.market = m; U.E.R = ruleFor(m); U.E.W.fixedAtr = 20; int k = 0;
        U.B = path({ 1000, 1010, 1000, 1010, 1000, 1010, 1000, 1010, 1000 }, T0, k);
        for (size_t i = 0; i + 1 < U.B.size(); ++i) U.E.closeBar(U.B[i], 45);
        U.c = U.B[U.B.size() - 2].c; const long long t = U.B.back().endMs - 170000;
        for (int q = 0; q < 31; ++q) U.E.onEvent(NEW, q % 2 ? ASK : BID, q % 2 ? U.c + 30 + (q % 5) : U.c - 30 - (q % 5), 5, 900 + q, t);
        U.px = side == ASK ? U.c + 12 : U.c - 12;
        U.E.onEvent(NEW, side, U.px, 60, 77, t);
        U.E.closeBar(U.B.back(), 45); U.tEnd = U.B.back().endMs;
        for (size_t i = 0; i < U.E.zones.size(); ++i) if (U.E.zones[i].oid == 77) U.zi = (int)i;
    };
    auto bar = [&](Setup& U, Tick o, Tick h, Tick l, Tick c) { Bar b; b.endMs = (U.tEnd += 180000); b.o = o; b.h = h; b.l = l; b.c = c; return b; };
    auto countRows = [](const std::vector<std::string>& r, const std::string& pre) { int n = 0; for (auto& x : r) if (x.compare(0, pre.size(), pre) == 0) ++n; return n; };
    // 14) HELD: the bar trades into the band (high px-3 >= px-5), closes back below (px-8 < px-5), the order still rests -> HELD (T line)
    {   Setup U; setup("NQ", ASK, U); CHECK(U.zi >= 0 && U.E.zones[(size_t)U.zi].kind == K_NEAR && U.E.zones[(size_t)U.zi].drawn, "setup: a drawn NEAR offer zone (60 lots)");
        Engine& E = U.E; const Tick px = U.px;
        E.closeBar(bar(U, U.c, U.c + 4, U.c - 2, U.c + 2), 45);                                  // not in the band yet
        CHECK(E.zones[(size_t)U.zi].tagHeldMs == 0 && E.zones[(size_t)U.zi].contactBarMs == 0, "no contact: no tag");
        const Bar hb = bar(U, U.c + 2, px - 3, U.c, px - 8); E.closeBar(hb, 45);
        const Zone& z = E.zones[(size_t)U.zi];
        CHECK(z.tagHeldMs == hb.endMs && z.tagFillMs == 0 && E.tally.held == 1, "HELD on the close-back bar, order still resting");
        CHECK(countRows(E.rows, "T|" + std::to_string(z.id) + "|" + std::to_string(hb.endMs) + "|HELD|0|60") == 1, "recorded: T|id|bar|HELD|traded 0|resting 60");
        E.closeBar(bar(U, px - 8, px - 2, px - 12, px - 9), 45);                                 // a second close back
        CHECK(E.tally.held == 1 && countRows(E.rows, "T|") == 1, "HELD once per zone (never twice, never moved)");
        // the label / tag text / tagsShown
        Settings st; CHECK(tagsShown(z, st) && std::string(tagText(false)) == "FILLING", "FILLING shown with the zone line");
        st.tags = false; CHECK(tagsShown(z, st) && shown(z, st) && !afterShown(z, st, true), "(1.3.0) the after-fill box does not hide FILLING or the line");
        // a later FILLING may follow the HELD (later bar)
        const Bar fb = bar(U, px - 9, px + 1, px - 10, px - 2); E.onEvent(FILL, BID, px, 20, 77, fb.endMs - 30000); E.closeBar(fb, 45);
        CHECK(E.zones[(size_t)U.zi].tagFillMs == fb.endMs && E.zones[(size_t)U.zi].tagHeldMs == hb.endMs, "a later FILLING follows; the HELD stays as printed");
        // NO REPAINT: a fresh engine restores the record, the SAME bars come again (the IRT restart path) -> identical tags, no new T line
        std::vector<std::string> rec = E.rows; Engine R2; R2.market = "NQ"; R2.R = ruleFor("NQ"); R2.W.fixedAtr = 20; long long bad = -1; R2.restore(rec, &bad);
        CHECK(bad == 0 && R2.judgedMs == fb.endMs, "restore: no bad lines, judged up to the last bar");
        const Zone& r2 = R2.zones[(size_t)U.zi];
        CHECK(r2.tagHeldMs == hb.endMs && r2.tagFillMs == fb.endMs, "restored tags identical (T lines read back)");
        for (const Bar& b : E.W.bars) R2.closeBar(b, 45);
        CHECK(countRows(R2.rows, "T|") == 0 && countRows(R2.rows, "B|") == 0 && R2.zones[(size_t)U.zi].tagHeldMs == hb.endMs, "re-fed bars after a restart decide NO tag again (judged bars track only)");
        CHECK(R2.zones[(size_t)U.zi].contactBarMs == E.zones[(size_t)U.zi].contactBarMs && R2.zones[(size_t)U.zi].extQ == E.zones[(size_t)U.zi].extQ, "the contact / extreme tracking is rebuilt the same from the bars");
        CHECK(R2.zones[(size_t)U.zi].end == LIVE, "(1.2.0 fix) re-fed old bars before the book refills never end a zone SHRANK (the 1.1.0 restart path did)");
        // the old 1.1.0 reader (Z needs 21 fields; T / B / A / W unknown) would skip the new lines; the new reader skips unknown types
        std::vector<std::string> fut = rec; fut.push_back("Q|1|5|NEW"); fut.push_back("X|9"); Engine R3; long long b3 = -1; R3.restore(fut, &b3);
        CHECK(b3 == 0 && R3.tally.unknownLines == 2 && R3.zones.size() == E.zones.size(), "unknown line types (a newer build) are skipped, not bad");
        std::vector<std::string> old = { "Z|1|77|1|NEAR|" + std::to_string(T0) + "|1012|1011|1013|60|1|0|0|45|200|0|0|0|40|1|1.1.0", "E|1|" + std::to_string(T0 + 600000) + "|PULLED|0" };
        Engine R4; long long b4 = -1; CHECK(R4.restore(old, &b4) == 1 && b4 == 0 && R4.zones[0].end == PULLED && R4.zones[0].med == 0, "a 1.1.0 record (21-field Z lines) still loads");
    }
    // 15) FILLING: the zone order trades during the bar (price in the band) -> FILLING, never HELD on that bar even if it closes back
    {   Setup U; setup("CL", ASK, U); Engine& E = U.E; const Tick px = U.px;
        const Bar b = bar(U, U.c, px, U.c, px - 8); E.onEvent(FILL, BID, px, 25, 77, b.endMs - 60000); E.closeBar(b, 45);
        const Zone& z = E.zones[(size_t)U.zi];
        CHECK(z.tagFillMs == b.endMs && z.tagHeldMs == 0 && z.end == LIVE && E.tally.filling == 1, "FILLING (traded 25 in the bar) - not HELD on the same bar");
        CHECK(countRows(E.rows, "T|" + std::to_string(z.id) + "|" + std::to_string(b.endMs) + "|FILLING|25|35") == 1, "recorded: T|id|bar|FILLING|traded 25|resting 35");
        // the whole order fills inside a bar (FILL + DEL): the zone ends FILLED and the bar still gets its FILLING tag (no earlier tag)
        Setup V; setup("HG", ASK, V); const Bar b2 = bar(V, V.c, V.px + 1, V.c, V.px + 1);
        V.E.onEvent(FILL, BID, V.px, 60, 77, b2.endMs - 50000); V.E.onEvent(DEL, ASK, V.px, 0, 77, b2.endMs - 50000); V.E.closeBar(b2, 45);
        CHECK(V.E.zones[(size_t)V.zi].end == FILLED && V.E.zones[(size_t)V.zi].tagFillMs == b2.endMs, "filled away inside the bar: FILLED + the FILLING tag on that bar");
    }
    // 16) no HELD: (a) no close back, (b) the extreme went beyond the tolerance (+10), (c) the order was PULLED before contact (X)
    {   Setup U; setup("GC", ASK, U); Engine& E = U.E; const Tick px = U.px;
        E.closeBar(bar(U, U.c, px - 2, U.c, px - 4), 45);                                         // in the band, close px-4 is NOT below px-5
        CHECK(E.zones[(size_t)U.zi].tagHeldMs == 0 && E.zones[(size_t)U.zi].contactBarMs > 0, "(a) traded in, no close back: no HELD");
        E.closeBar(bar(U, px - 4, px + 11, px - 9, px - 9), 45);                                  // a wick to px+11 (> px+10), closes back
        CHECK(E.zones[(size_t)U.zi].tagHeldMs == 0, "(b) the extreme went past the tolerance: no HELD (swept)");
        Setup V; setup("ES", ASK, V); V.E.onEvent(DEL, ASK, V.px, 60, 77, V.tEnd + 30000);
        V.E.closeBar(bar(V, V.c, V.px - 3, V.c, V.px - 8), 45);
        const Zone& z = V.E.zones[(size_t)V.zi];
        CHECK(z.end == PULLED && z.tagHeldMs == 0 && z.tagFillMs == 0, "(c) pulled before contact: PULLED (grey X), no tag");
    }
    // 17) bids: tracked, tagged (log), never drawn - also with BIDS,1 in an old nightly params file; all 7 markets draw the near offer
    {   Setup U; setup("NG", BID, U); Engine& E = U.E; const Tick px = U.px;
        E.closeBar(bar(U, U.c, U.c + 2, px + 3, px + 8), 45);                                     // low into the bid band, close back above
        const Zone& z = E.zones[(size_t)U.zi];
        CHECK(z.side == BID && !z.drawn && z.tagHeldMs > 0 && !tagsShown(z, Settings()), "a bid zone: HELD recorded (log), the zone and its tag never drawn");
        Engine P2; P2.R = parseParams({ "VERSION,1", "MARKET,NG", "PARAM,BIDS,1" }, "NG"); CHECK(!P2.R.bids, "BIDS,1 in the nightly file no longer switches bids on");
        int drawnAll = 0; for (const char* m : { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" }) { Setup M; setup(m, ASK, M); if (M.zi >= 0 && M.E.zones[(size_t)M.zi].drawn && !M.E.zones[(size_t)M.zi].q) ++drawnAll; }
        CHECK(drawnAll == 7, "(1.2.0) the near offer zone is drawn in all 7 markets with the same rule");
    }
    // 18) LOG ONLY: FAST / DRIFT arrival, EATEN, STEPPED (A / W / S lines; nothing drawn)
    {   Setup U; setup("ES", ASK, U); Engine& E = U.E; const Tick px = U.px; long long t = U.tEnd + 1000;
        E.onEvent(FILL, BID, U.c - 10, 1, 901, t);                                 // a print 1 s in: 0.6 ATR below the band edge... (px-5 - (c-10)) = 27 ticks = 1.35 ATR
        E.onEvent(NEW, ASK, px - 4, 3, 5001, t + 20000); E.onEvent(FILL, BID, px - 4, 3, 5001, t + 20000);   // the first print in the band 20 s later
        const Zone& z = E.zones[(size_t)U.zi];
        CHECK(z.tContact == t + 20000 && z.arrival == 1 && countRows(E.rows, "A|" + std::to_string(z.id) + "|") == 1, "FAST: 1.35 ATR in the 60 s before contact (>= 0.77 for ES), A line");
        E.onEvent(NEW, ASK, px - 3, 3, 5002, t + 60000); E.onEvent(FILL, BID, px - 3, 3, 5002, t + 60000);   // still inside 40 s later
        E.onEvent(NEW, BID, U.c, 3, 5003, t + 61000); E.onEvent(FILL, ASK, U.c, 3, 5003, t + 61000);         // leaves the band
        CHECK(z.dwellMs == 41000 && z.eaten && countRows(E.rows, "W|" + std::to_string(z.id) + "|") == 1, "EATEN: 41 s inside the band (> 30 s), W line");
        E.closeBar(bar(U, U.c, px - 3, U.c - 2, px - 8), 45);                                       // close back: the turn
        E.onEvent(MOD, ASK, px - 1, 60, 77, U.tEnd + 20000);                                        // re-priced 1 tick TOWARD price after the turn
        CHECK(z.stepped && countRows(E.rows, "S|" + std::to_string(z.id) + "|") == 1 && E.tally.stepped == 1, "STEPPED: the same id re-priced toward price after the close back, S line");
        Setup V; setup("HG", ASK, V); long long tv = V.tEnd + 1000;
        V.E.onEvent(NEW, ASK, V.px - 6, 2, 6001, tv); V.E.onEvent(FILL, BID, V.px - 6, 2, 6001, tv);
        V.E.onEvent(NEW, ASK, V.px - 4, 2, 6002, tv + 50000); V.E.onEvent(FILL, BID, V.px - 4, 2, 6002, tv + 50000);
        CHECK(V.E.zones[(size_t)V.zi].arrival == 3, "DRIFT: 2 ticks = 0.1 ATR in the 60 s before contact (<= 0.37 for HG)");
        std::vector<std::string> rec = E.rows; Engine R5; long long b5 = -1; R5.restore(rec, &b5);
        CHECK(b5 == 0 && R5.zones[(size_t)U.zi].arrival == 1 && R5.zones[(size_t)U.zi].eaten && R5.zones[(size_t)U.zi].stepped, "the log-only lines read back (no double logging after a restart)");
    }
    // ================= (1.3.0) THE AFTER-FILL READ (H6-FILL) =================
    // an offer zone (60 lots, oid 77) at px; ATR 20 -> band beyond = px+1 .. px+5 (floor(max(1, 0.25 x 20)) = 5); delta from fill + 10 s
    auto fillSetup = [&](Setup& U, const char* m) { setup(m, ASK, U); U.E.R.rth0Min = 0; U.E.R.rth1Min = 1440; };   // (T0 is 16:00 CT)   // RTH gate tested on its own below
    {   Setup U; fillSetup(U, "NQ"); Engine& E = U.E; const Tick px = U.px; const long long t1 = U.tEnd + 10000;
        E.onEvent(NEW, ASK, px + 3, 4, 7001, t1 - 5000);                                // an offer already resting beyond (cancelled later)
        E.onEvent(FILL, BID, px, 40, 77, t1 - 1000);                                    // partly filled: no read yet
        CHECK(E.pend.empty() && E.zones[(size_t)U.zi].afFillMs == 0, "partial fill: no after-fill read");
        E.onEvent(FILL, BID, px, 20, 77, t1); E.onEvent(DEL, ASK, px, 0, 77, t1);      // fully filled (60 of 60) + the DEL
        CHECK(E.pend.size() == 1 && E.zones[(size_t)U.zi].afFillMs == t1 && E.zones[(size_t)U.zi].end == FILLED, "full fill: the 30 s watch starts (zone FILLED)");
        E.onEvent(NEW, ASK, px + 2, 20, 7002, t1 + 5000);                               // new offers stacked beyond: +20
        E.onEvent(NEW, ASK, px + 6, 50, 7003, t1 + 6000);                               // beyond the band (px+6): not counted
        E.onEvent(NEW, BID, px - 1, 50, 7004, t1 + 6000);                               // the other side: not counted
        E.onEvent(DEL, ASK, px + 3, 0, 7001, t1 + 7000);                                // a cancel beyond: -4 (its last shown size)
        E.onEvent(FILL, ASK, U.c - 30, 5, 900, t1 + 8000);                              // sellers hit a bid BEFORE +10 s: not in the delta
        E.onEvent(FILL, ASK, U.c - 32, 5, 902, t1 + 15000);                             // sellers hit a bid at +15 s: defender (offer) delta +5
        E.onEvent(FILL, BID, U.c + 31, 2, 901, t1 + 20000);                             // buyers lift an offer at +20 s: -2
        CHECK(E.pend.size() == 1 && E.pend[0].adds == 20 && E.pend[0].cans == 4 && E.pend[0].delta == 3, "window: adds 20, cancels 4 beyond (fixed band, defender side), delta +3 from +10 s");
        E.onEvent(NEW, ASK, px + 1, 99, 7005, t1 + 30000);                              // AT fill + 30 s: decided first, not counted
        const Zone& z = E.zones[(size_t)U.zi];
        CHECK(E.pend.empty() && z.afRead == 1 && z.afDecMs == t1 + 30000 && z.afStack == 16 && z.afDelta == 3 && E.tally.rebid == 1, "RE-BID: stack +16 (nb 0.67 >= NQ line) and delta +3 > 0, decided at fill + 30 s");
        CHECK(countRows(E.rows, "R|" + std::to_string(z.id) + "|" + std::to_string(t1) + "|" + std::to_string(t1 + 30000) + "|RE-BID|16|3|20|4") == 1, "recorded: R|id|fill|decide|RE-BID|stack|delta|adds|cancels");
        Settings st; CHECK(afterShown(z, st, true) && std::string(afterText(z.afRead)) == "RE-BID", "drawn: RE-BID on the offer zone");
        st.tags = false; CHECK(!afterShown(z, st, true), "the after-fill box hides it");
        // NO REPAINT: a fresh engine reads the record and is fed every bar again -> the same read, no new R line, nothing pending
        std::vector<std::string> rec = E.rows; Engine R2; R2.market = "NQ"; R2.R = E.R; R2.W.fixedAtr = 20; long long bad = -1; R2.restore(rec, &bad);
        for (const Bar& b : E.W.bars) R2.closeBar(b, 45);
        std::printf("  restart R2: bad %lld afRead %d dec %lld stack %lld R rows %d pend %zu rebid %lld\n", bad, R2.zones[(size_t)U.zi].afRead, R2.zones[(size_t)U.zi].afDecMs, R2.zones[(size_t)U.zi].afStack, countRows(R2.rows, "R|"), R2.pend.size(), R2.tally.rebid);
        CHECK(bad == 0 && R2.zones[(size_t)U.zi].afRead == 1 && R2.zones[(size_t)U.zi].afDecMs == t1 + 30000 && R2.zones[(size_t)U.zi].afStack == 16 && countRows(R2.rows, "R|") == 0 && R2.pend.empty() && R2.tally.rebid == 1,
              "restart: the R line reads back identically, re-fed bars decide nothing again");
        CHECK(R2.afDone.count(std::make_pair((int)ASK, px)) == 1, "restart: the dedup key is rebuilt from the R line");
        // strict dedup: another zone at the same side / price this session is never read again
        Zone d2 = E.zones[(size_t)U.zi]; d2.afRead = 0; d2.afFillMs = 0; d2.id = 99; E.zones.push_back(d2); Ord od; od.tBig = 1; od.tFill1 = t1; E.afStart((int)E.zones.size() - 1, t1 + 400000, od);
        CHECK(E.pend.empty() && E.tally.afDup == 1, "strict dedup: one read per side / price per session");
        std::vector<std::string> g = { rec[0], "R|1|5|4|RE-BID|1|1|1|0", "R|1|5|9|MAYBE|1|1|1|0", "R|1|5|9|RE-BID|1|1" }; Engine R3; long long b3 = -1; R3.restore(g, &b3);
        CHECK(b3 == 3, "garbled R lines (decide before fill, unknown read, short) are bad, never applied");
    }
    {   // ACCEPTED (a): stack beyond but the attackers win the tape (delta < 0); (b) delta positive but more cancelled than added
        Setup U; fillSetup(U, "ES"); Engine& E = U.E; const Tick px = U.px; const long long t1 = U.tEnd + 10000;
        E.onEvent(FILL, BID, px, 60, 77, t1); E.onEvent(DEL, ASK, px, 0, 77, t1);
        E.onEvent(NEW, ASK, px + 1, 30, 7101, t1 + 2000);
        E.onEvent(FILL, BID, U.c + 31, 5, 901, t1 + 12000);                             // buyers lift: offer-defender delta -5
        E.onEvent(NEW, ASK, U.c + 40, 1, 7102, t1 + 31000);
        CHECK(E.zones[(size_t)U.zi].afRead == 2 && E.tally.accepted == 1 && [&]() { int n = 0; for (auto& x : E.rows) if (x[0] == 'R' && x.find("|ACCEPTED|30|-5|30|0") != std::string::npos) ++n; return n; }() == 1, "ACCEPTED: stacked 30 but delta -5 (attackers kept lifting)");
        Setup V; fillSetup(V, "ES"); Engine& F2 = V.E; const Tick p2 = V.px; const long long t2 = V.tEnd + 10000;
        F2.onEvent(NEW, ASK, p2 + 2, 30, 7201, t2 - 20000);
        F2.onEvent(FILL, BID, p2, 60, 77, t2); F2.onEvent(DEL, ASK, p2, 0, 77, t2);
        F2.onEvent(NEW, ASK, p2 + 1, 10, 7202, t2 + 1000); F2.onEvent(DEL, ASK, p2 + 2, 0, 7201, t2 + 2000);
        F2.onEvent(FILL, ASK, V.c - 30, 5, 900, t2 + 12000);
        F2.onEvent(NEW, ASK, V.c + 40, 1, 7203, t2 + 30500);
        CHECK(F2.zones[(size_t)V.zi].afRead == 2 && F2.zones[(size_t)V.zi].afStack == -20 && F2.zones[(size_t)V.zi].afDelta == 5, "ACCEPTED: delta +5 but stack -20 (the size beyond left)");
    }
    {   // a BID zone swept below: RE-BID (green); the delta is signed for the bid defender (buyers)
        Setup U; setup("CL", BID, U); U.E.R.rth0Min = 0; U.E.R.rth1Min = 1440; Engine& E = U.E; const Tick px = U.px; const long long t1 = U.tEnd + 10000;
        E.onEvent(FILL, ASK, px, 60, 77, t1); E.onEvent(DEL, BID, px, 0, 77, t1);
        E.onEvent(NEW, BID, px - 3, 12, 7301, t1 + 3000);                               // new bids stacked below the filled bid
        E.onEvent(FILL, BID, U.c + 31, 4, 901, t1 + 11000);                             // buyers lift an offer: bid-defender delta +4
        E.onEvent(NEW, ASK, U.c + 40, 1, 7302, t1 + 30000);
        const Zone& z = E.zones[(size_t)U.zi];
        CHECK(z.side == BID && z.afRead == 1 && z.afStack == 12 && z.afDelta == 4, "bid zone: RE-BID (stack 12 below, buyers +4)");
        Settings st; CHECK(!z.drawn && afterShown(z, st, true) && !afterShown(z, st, false), "a bid's read is drawn (no line) in a market with a draw rule, never in a log-only market");
        st.nearO = false; CHECK(!afterShown(z, st, true), "the bid's kind box hides its read too");
    }
    {   // RTH gate, idle feed, lost window, FAR never, params
        Setup U; setup("ES", ASK, U); Engine& E = U.E; E.R.rth0Min = 0; E.R.rth1Min = 1; const long long t1 = U.tEnd + 10000;   // window 00:00-00:01 CT: outside
        E.onEvent(FILL, BID, U.px, 60, 77, t1);
        CHECK(E.pend.empty() && E.tally.afOff == 1 && E.zones[(size_t)U.zi].afRead == 0, "outside the study's RTH window: no read (counted)");
        const long long M0 = T0 - 16LL * 3600000LL; Engine Q; Q.R = ruleFor("GC"); CHECK(Q.inRth(M0 + 440LL * 60000LL) && !Q.inRth(M0 + 440LL * 60000LL - 1) && Q.inRth(M0 + 900LL * 60000LL) && !Q.inRth(M0 + 900LL * 60000LL + 1), "GC RTH = 7:20 .. 15:00 CT (the study)");
        CHECK(ruleFor("ES").rth0Min == 510 && ruleFor("HG").rth0Min == 430 && ruleFor("CL").rth0Min == 480 && ruleFor("EU").rth0Min == 440 && ruleFor("NG").nbMin > 0.04 && ruleFor("GC").nbMin < 0, "per-market RTH start and stack line (research)");
        Setup V; fillSetup(V, "HG"); const long long t2 = V.tEnd + 10000;
        V.E.onEvent(FILL, BID, V.px, 60, 77, t2);
        Bar ib = bar(V, V.c, V.c + 1, V.c - 1, V.c); ib.endMs = t2 + 60000; V.E.closeBar(ib, 45);
        CHECK(V.E.pend.size() == 1, "idle feed: not decided at a bar close < 60 s after the window");
        Bar ib2 = ib; ib2.endMs = t2 + 240000; V.E.closeBar(ib2, 45);
        CHECK(V.E.pend.empty() && V.E.zones[(size_t)V.zi].afRead == 2, "idle feed: decided at the first bar close >= 60 s after the window (nothing stacked: ACCEPTED)");
        Setup L; fillSetup(L, "GC"); L.E.onEvent(FILL, BID, L.px, 60, 77, L.tEnd + 10000); L.E.endSession(L.tEnd + 20000);
        CHECK(L.E.tally.afLost == 1 && L.E.zones[(size_t)L.zi].afRead == 0 && countRows(L.E.rows, "R|") == 0, "a window cut by the session end: never decided (lost)");
        Setup Fz; fillSetup(Fz, "NQ"); Fz.E.zones[(size_t)Fz.zi].kind = K_FAR; Fz.E.onEvent(FILL, BID, Fz.px, 60, 77, Fz.tEnd + 10000);
        CHECK(Fz.E.pend.empty(), "a FAR zone never gets an after-fill read");
        Rule r = parseParams({ "VERSION,1", "MARKET,ES", "PARAM,FILL_NB,0.05", "PARAM,FILL_NB,7" }, "ES"); CHECK(r.nbMin == 0.05, "params: FILL_NB read (out of range ignored)");
        Setup Y; fillSetup(Y, "NQ"); Y.E.book[77].tBig = Y.tEnd - 5000;   // as if it reached the floor size only 5 s before the hit (a re-size up)
        Y.E.onEvent(FILL, BID, Y.px, 60, 77, Y.tEnd + 1000);
        CHECK(Y.E.pend.empty() && Y.E.tally.afYoung == 1, "first hit < 30 s after it showed the floor size: no read (the study's rule)");
        Setup Mv; fillSetup(Mv, "NQ"); Mv.E.onEvent(MOD, ASK, Mv.px + 1, 60, 77, Mv.tEnd + 1000);   // the zone follows one re-price (approved), the study would end it MOVED
        Mv.E.onEvent(FILL, BID, Mv.px + 1, 60, 77, Mv.tEnd + 50000);
        CHECK(Mv.E.zones[(size_t)Mv.zi].end == LIVE && Mv.E.pend.empty() && Mv.E.tally.afMoved == 1, "an order re-priced after it showed the floor size: the zone follows, but no fill read (the study ended it MOVED)");
    }
    std::printf("LmtZonesLogic %s: %d passed, %d failed\n", VERSION, P, F);
    return F ? 1 : 0;
}
