// test_lmz.cpp -- LmtZonesLogic.h 1.0.0 (lsLiquidityProfile 0.48.0 LMT ZONES, Rassul approved mockup v3 2026-10-22 09:34 CT).
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
      CHECK(nPrint == 1 && printQ, "ES: the with-trend BD-print offer is a zone, drawn with ?");
      bool p11 = false; for (const Zone& z : E.zones) if (z.oid == 11 && z.kind == K_PRINT) p11 = true;
      CHECK(p11, "the print zone holds order 11 (the biggest real order in the print area)");
      CHECK(nNear == 0 || !nearDrawn, "ES: a plain near offer is logged, never drawn");
      CHECK(nBid >= 1 && !bidDrawn, "bids: tracked (LMT columns) but never drawn");
      bool small = false, young = false; for (const Zone& z : E.zones) { if (z.oid == 13) small = true; if (z.oid == 15) young = true; }
      CHECK(!small, "an order under S20 is never a zone");
      CHECK(!young, "an order that has not rested 30 s is not a zone yet");
      std::string lab; for (const Zone& z : E.zones) if (z.kind == K_PRINT) lab = label(z);
      CHECK(lab == "BD print 100?", "label: BD print 100?");
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
    { Rule r = ruleFor("CL"); CHECK(r.gammaOnly && r.nearO == 1, "CL: offers only with gamma confluence");
      r = ruleFor("GC"); CHECK(logOnly(r), "GC: log only"); r = ruleFor("EU"); CHECK(logOnly(r) && !r.farOn, "6E: log only");
      r = ruleFor("NQ"); CHECK(r.nearO == 2, "NQ: offers with ?"); r = ruleFor("NG"); CHECK(r.nearO == 1 && !r.bids, "NG: offers drawn, bids never");
      r = ruleFor("ES"); CHECK(r.print == 2 && r.nearO == 0 && r.origin == 0, "ES: only the BD-print offer with ?"); }
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
            CHECK(drawn == (withG == 1) && conf == (withG == 1), withG ? "CL on a gamma level: drawn + diamond" : "CL off gamma: log only");
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
        Rule r = parseParams({ "VERSION,1", "MARKET,ES", "PARAM,NEAR,1", "PARAM,S20_RTH,60", "PARAM,BIDS,1", "PARAM,JUNK,5" }, "ES");
        CHECK(r.nearO == 1 && r.s20Rth == 60 && r.bids && r.src.find("params file") == 0, "params override the defaults (bids can be switched on by the nightly)");
        r = parseParams({ "VERSION,1", "MARKET,NQ", "PARAM,NEAR,1" }, "ES"); CHECK(r.nearO == 0 && r.src.find("MARKET") != std::string::npos, "wrong market: defaults");
        r = parseParams({ "VERSION,2", "MARKET,ES" }, "ES"); CHECK(r.src.find("VERSION") != std::string::npos, "wrong version: defaults");
        r = parseParams({ "VERSION,1", "MARKET,ES", "PARAM,NEAR,7", "PARAM,S20_RTH,1e9" }, "ES"); CHECK(r.nearO == 0 && r.s20Rth == 45, "out-of-range values ignored");
    }
    // 12) settings line round trip + garbled; the checkboxes hide kinds
    {
        Settings s; s.print = false; s.cOffer = 0x123456; Settings b; CHECK(parseSettings(settingsLine(s), b) && !b.print && b.cOffer == 0x123456, "settings round trip");
        CHECK(!parseSettings("LMZ1,1,1", b) && !parseSettings("LMZ1,1,1,1,1,1,1,1,2,0,0,0,0", b), "garbled settings rejected");
        Zone z; z.kind = K_PRINT; z.drawn = true; CHECK(shown(z, Settings()) && !shown(z, s), "print checkbox hides print zones");
        z.drawn = false; CHECK(!shown(z, Settings()), "a log-only zone is never drawn");
    }
    // 13) hover: <= 12 rows, <= 118 chars, the close-back rule and study numbers present
    {
        Zone z; z.kind = K_PRINT; z.side = ASK; z.size0 = 100; z.s20 = 45; z.restMs = 95000; z.drawn = true; z.q = true; z.conf = true; z.confPx = 30832; Seg g; g.px = 30832; z.segs.push_back(g);
        HoverCtx c; c.market = "ES"; c.tick = 0.25; c.sizeLive = 100;
        const std::vector<Row> v = hoverRows(z, c); bool fits = v.size() <= 12, close = false, study = false, diamond = false;
        for (const Row& r : v) { if (r.text.size() > 118) fits = false; if (r.text.find("close back") != std::string::npos) close = true; if (r.text.find("n 11") != std::string::npos) study = true; if (r.ink == INK_YELLOW) diamond = true; }
        for (const Row& r : v) std::printf("    | %s\n", r.text.c_str());
        CHECK(fits && close && study && diamond, "hover: question / what / why / how (close back, not the touch) / study / example");
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
    std::printf("LmtZonesLogic %s: %d passed, %d failed\n", VERSION, P, F);
    return F ? 1 : 0;
}
