// test_ex.cpp -- ExhaustLogic.h 1.0.0 unit tests (g++ -std=c++17 -O1 test_ex.cpp -I..)
//   ./test_ex            the unit tests
//   ./test_ex dump DIR   also writes random visits (trades + zone) for the Python parity check against mbo_study.exh_at_zone
#include "ExhaustLogic.h"
#include <cstdio>
#include <fstream>
#include <random>
#include <string>
#include <vector>

static int passed = 0, failed = 0;
#define CHECK(c) do { if (c) ++passed; else { ++failed; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
static bool near(double a, double b, double e = 1e-9) { return std::fabs(a - b) <= e; }

static const long long T0 = 1790000000000LL + (14LL * 3600 + 52 * 60) * 1000;   // a CT clock at 14:52:00 (ms % day = 14:52)
static long long ctAt(int h, int m, int s) { const long long day = 1790000000000LL / 86400000LL * 86400000LL; return day + ((long long)h * 3600 + m * 60 + s) * 1000; }

int main(int argc, char** argv) {
    using namespace exh;
    // ---- zones / config ----
    {   ZoneCfg es = zoneCfg("ES"); CHECK(es.ok && es.lo == 4 && es.hi == 6);
        CHECK(halfWidth(es, 0) == 5);              // no ATR = 15 ticks x 0.30 = 4.5 -> 5 (round half to even? no: std::round 4.5 = 5)
        CHECK(halfWidth(es, 10) == 4);             // 3 -> clamp 4
        CHECK(halfWidth(es, 100) == 6);            // 30 -> clamp 6
        CHECK(zoneCfg("6E").ok && zoneCfg("6E").hi == 3 && zoneCfg("EU").lo == 2);
        CHECK(!zoneCfg("ZB").ok);
        CHECK(near(atr3m(10, 60), 10 * std::sqrt(3.0)));
        CHECK(inRth(es, ctAt(8, 30, 0)) && inRth(es, ctAt(14, 59, 59)) && !inRth(es, ctAt(15, 0, 0)) && !inRth(es, ctAt(8, 29, 59)));
        std::vector<Level> L = { {"PDH", 100}, {"ONH", 108}, {"PWH", 130}, {"CR", 160}, {"DPoc", 200}, {"VAH", 220}, {"CWH", 240} };
        std::vector<Zone> Z = mergeLevels(L, 5);   // key price levels only: gamma (CR), volume (DPoc / VAH) and CWH are not Ex zones
        CHECK(Z.size() == 2 && Z[0].name == "PDH+ONH" && Z[0].lo == 95 && Z[0].hi == 113 && Z[1].lo == 125 && Z[1].name == "PWH");
        CHECK(isKeyCode("LonHI") && isKeyCode("AsiaLO") && isKeyCode("Open") && !isKeyCode("WPoc") && !isKeyCode("HVL"));
    }
    // ---- decide(): the measure on hand-made visits ----
    auto mk = [](int side) { Visit v; v.key = "T1"; v.name = "PDH"; v.side = side; v.lo = 100; v.hi = 108; v.ta = T0; return v; };
    {   // sellers at a support: 10 lots / s in the 1st half, 1 lot / s in the 2nd, price at the zone the whole time -> 0.1 = Ex
        Visit v = mk(SUP);
        for (int k = 0; k < NSEC; ++k) { v.last[k] = 104; v.seen[k] = true; v.att[k] = k < HALF ? 10 : 1; }
        Dec d = decide(v); CHECK(d.s1 == 90 && d.s2 == 91 && near(d.share, 1.0)); CHECK(near(d.exh, 0.1)); CHECK(d.isEx);
        CHECK(d.td == T0 + 180000);
    }
    {   // the same pace both halves -> 1.0, not Ex
        Visit v = mk(SUP); for (int k = 0; k < NSEC; ++k) { v.last[k] = 104; v.seen[k] = true; v.att[k] = 5; }
        Dec d = decide(v); CHECK(near(d.exh, 1.0, 0.02)); CHECK(!d.isEx);
    }
    {   // exactly at the threshold: 0.164 is Ex (<=), 0.165 is not
        Visit v = mk(RES); for (int k = 0; k < NSEC; ++k) { v.last[k] = 104; v.seen[k] = true; }
        v.att[0] = 1000; v.att[HALF] = 1000 * 0.164 * 91.0 / 90.0; CHECK(decide(v).isEx && near(decide(v).exh, 0.164));
        v.att[HALF] = 1000 * 0.1652 * 91.0 / 90.0; CHECK(!decide(v).isEx);
    }
    {   // price left the zone early: share < 0.5 -> never Ex even with exh 0 (the study's "it left early" guard)
        Visit v = mk(SUP); v.seen[0] = true; v.last[0] = 104; v.att[0] = 50; v.seen[60] = true; v.last[60] = 140;   // away from second 60
        Dec d = decide(v); CHECK(d.s1 == 60 && d.s2 == 0); CHECK(d.exh < 0); CHECK(!d.isEx);
    }
    {   // within 1 tick of the zone counts as "at"; 2 ticks does not
        Visit v = mk(SUP); for (int k = 0; k < NSEC; ++k) { v.seen[k] = true; v.last[k] = k % 2 ? 99 : 98; v.att[k] = 0; }
        v.att[0] = 10; Dec d = decide(v); CHECK(d.s1 == 45 && d.s2 == 45);   // odd seconds at 99 (1 tick below lo) = at
    }
    {   // no attackers in the 1st half -> no measure
        Visit v = mk(SUP); for (int k = 0; k < NSEC; ++k) { v.last[k] = 104; v.seen[k] = true; } v.att[100] = 9;
        CHECK(decide(v).exh < 0 && !decide(v).isEx);
    }
    {   // the ES 2026-09-22 14:52 case as recorded by the study (Ex_worked_1: resistance 7844.25-7846.25, exh 0.000, share 0.525, HELD):
        // no raw trades for that day in this container, so the fixture is BUILT to the study's recorded numbers - buyers lift in the
        // 1st half only, price at the zone 95 of 181 seconds -> exh 0.000, share 0.525 (95 / 181), Ex
        Visit v; v.key = "L1"; v.kind = 'L'; v.side = RES; v.lo = 31377; v.hi = 31385; v.ta = ctAt(14, 52, 10);
        for (int k = 0; k < NSEC; ++k) { v.seen[k] = true; v.last[k] = k < 70 || (k >= 120 && k < 145) ? 31380 : 31370; }
        for (int k = 0; k < 40; ++k) v.att[k] = 7;
        Dec d = decide(v); CHECK(near(d.share, 0.525)); CHECK(near(d.exh, 0.0)); CHECK(d.isEx && d.side == RES);
    }
    // ---- the engine: touches, arming, attackers, RTH, decision time, duplicates ----
    {   Engine E; E.setMarket("ES"); std::vector<Level> L = { {"PDH", 31380} }; E.setZones(L, 4);   // zone 31376..31384
        E.closeBar(31370, 31360, 31365);                                // a candle fully BELOW the zone -> armed as resistance
        CHECK(E.armed.count("T31380") && E.armed["T31380"] == RES);
        const long long ta = ctAt(10, 0, 0);
        E.onTrade(ta - 5000, 31370, 3, BUYER);                         // below the zone: nothing
        CHECK(E.active.empty());
        E.onTrade(ta, 31377, 2, BUYER);                                 // arrival
        CHECK(E.active.size() == 1 && E.active[0].ta == ta && E.active[0].side == RES && !E.armed.count("T31380"));
        for (int s = 0; s < 90; ++s) { E.onTrade(ta + s * 1000 + 10, 31380, 10, BUYER); E.onTrade(ta + s * 1000 + 20, 31380, 4, SELLER); }
        for (int s = 90; s <= 180; ++s) E.onTrade(ta + s * 1000, 31380, 1, s % 3 ? SELLER : BUYER);
        E.onTrade(ta + 180000, 31381, 50, BUYER);                       // AT the decision time: still counts (second 180)
        CHECK(E.active.size() == 1 && E.decided.empty());
        E.onTrade(ta + 180001, 31381, 999, BUYER);                      // after it: decides first, this one is outside the window
        CHECK(E.active.empty() && E.decided.size() == 1 && E.rows.size() == 1);
        const Dec& d = E.decided[0];
        // 1st half buyers: 2 + 90 x 10 = 902 lots / 90 s; 2nd half buyers: s % 3 == 0 for s 90..180 = 31 lots + 50 = 81 / 91 s
        CHECK(near(d.att1, 902) && near(d.att2, 81)); CHECK(near(d.exh, std::round((81.0 / 91) / (902.0 / 90) * 1000) / 1000));
        CHECK(d.isEx && d.label == "Ex?" && d.gateN == -1);             // no params file -> Ex?
        CHECK(E.tally.ex == 1 && E.tally.decided == 1);
        // the record round-trips and is never decided twice
        Engine F; F.setMarket("ES"); CHECK(F.adopt({ header(), E.rows[0], E.rows[0] }) == 1);
        Dec r; CHECK(parseRec(E.rows[0], r) && r.restored && r.id() == d.id() && r.isEx && r.label == "Ex?" && r.td == d.td);
        CHECK(!parseRec("EXH|1.0.0|ES|k|n|T|S|1|2|3", r));
        // a second touch needs a candle fully outside again
        E.onTrade(ta + 400000, 31379, 1, BUYER); CHECK(E.active.empty());
        E.closeBar(31400, 31390, 31395); CHECK(E.armed["T31380"] == SUP); // fully ABOVE -> support next time
    }
    {   // outside RTH: armed, touched, not measured
        Engine E; E.setMarket("ES"); E.setZones({ {"ONH", 500} }, 4); E.closeBar(480, 470, 475);
        E.onTrade(ctAt(7, 0, 0), 500, 1, BUYER); CHECK(E.active.empty() && E.tally.outRth == 1);
    }
    {   // only key price levels start a visit; the zones refresh keeps the armed state; a 2nd visit needs a candle outside again
        Engine E; E.setMarket("ES");
        E.setZones({ {"PDL", 1002}, {"HVL", 1100}, {"DPoc", 1200} }, 4); CHECK(E.zones.size() == 1);
        E.closeBar(1050, 1020, 1030);                                     // fully above -> armed as support
        E.setZones({ {"PDL", 1002} }, 4); CHECK(E.armed.count("T1002"));   // a refresh keeps it
        const long long ta = ctAt(11, 0, 0);
        E.onTrade(ta, 1100, 5, SELLER); E.onTrade(ta, 1200, 5, SELLER); CHECK(E.active.empty());   // gamma / volume prices: nothing
        E.onTrade(ta + 1, 1001, 5, SELLER); CHECK(E.active.size() == 1 && E.active[0].side == SUP && E.active[0].name == "PDL");
        E.step(ta + 184002); CHECK(E.decided.size() == 1 && E.active.empty());
        E.onTrade(ta + 300000, 1000, 5, SELLER); CHECK(E.active.empty());
        E.closeBar(1050, 1020, 1030); E.onTrade(ta + 400000, 1000, 5, SELLER); CHECK(E.active.size() == 1);
    }
    {   // implied fills (NOATT) set the price, never the attacker lots
        Engine E; E.setMarket("CL"); E.setZones({ {"PDH", 6000} }, 5); E.closeBar(6050, 6020, 6030);
        const long long ta = ctAt(9, 0, 0); E.onTrade(ta, 6001, 7, NOATT);
        CHECK(E.active.size() == 1 && near(E.active[0].att[0], 0) && E.active[0].seen[0]);
    }
    // ---- the gate ----
    {   // the key-level re-test of 2026-10-27: EU 25 cases (closest), none in ES / NQ -> every market "Ex?"
        const std::vector<std::string> today = { "market,n,held,control,min_n,min_lift,updated", "EU,25,0.76,0.596,30,0,2026-10-27", "ES,0,0,0.667,30,0,2026-10-27",
                                                 "NG,10,0.7,0.696,30,0,2026-10-27" };
        for (const char* m : { "6E", "ES", "NG", "NQ" }) CHECK(!parseGate(today, m).ok);
        Gate g = parseGate({ "EU,31,0.76,0.596,30,0,2026-11-20" }, "6E");
        CHECK(g.ok && g.n == 31 && std::string(label(g)) == "Ex");          // 30+ key-level cases: "Ex"
        CHECK(!parseGate({ "HG,67,0.627,0.524,30,0.15,x" }, "HG").ok);    // a min_lift in the file is honoured (+10 pts < 15)
        CHECK(!parseGate({ "market,n" }, "NQ").ok);                        // no line for the market
        CHECK(!parseGate({ "ES,abc,0.8,0.5" }, "ES").ok);
        CHECK(parseGate({ "ES,40,0.80,0.50" }, "ES").ok);                  // defaults for min_n / min_lift
        Gate none; CHECK(std::string(label(none)) == "Ex?");
        // the label is stamped at decision time
        Engine E; E.setMarket("6E"); E.gate = parseGate({ "EU,31,0.76,0.596,30,0,2026-11-20" }, "6E");
        Visit v; v.key = "T5"; v.side = SUP; v.lo = 10; v.hi = 14; v.ta = ctAt(9, 0, 0);
        for (int k = 0; k < NSEC; ++k) { v.last[k] = 12; v.seen[k] = true; v.att[k] = k < HALF ? 9 : 0; } E.active.push_back(v);
        E.step(v.ta + 183001); CHECK(E.decided.size() == 1 && E.decided[0].label == "Ex" && E.decided[0].gateN == 31 && E.tally.exConfirmed == 1);
    }
    // ---- marks by candle: decision candle, closed only, one per candle ----
    {   std::vector<Dec> D(4);
        const long long e0 = ctAt(10, 3, 0);
        std::vector<long long> ends = { e0, e0 + 180000, e0 + 360000, e0 + 540000 };   // 3-min candles ending 10:03, 10:06, 10:09, 10:12
        D[0].isEx = true; D[0].td = e0 + 60000; D[0].side = SUP; D[0].key = "a";     // 10:04 -> candle 1 (10:03-10:06)
        D[1].isEx = true; D[1].td = e0 + 90000; D[1].side = RES; D[1].key = "b";     // same candle: dropped (one per candle)
        D[2].isEx = false; D[2].td = e0 + 200000; D[2].key = "c";                    // not Ex
        D[3].isEx = true; D[3].td = e0 + 500000; D[3].side = RES; D[3].key = "d";    // candle 3 = still forming
        std::map<int, Mark> M = marksByBar(D, ends, 180, 2);
        CHECK(M.size() == 1 && M.count(1) && M[1].side == SUP);
        M = marksByBar(D, ends, 180, 3); CHECK(M.size() == 2 && M.count(3) && M[3].side == RES);
        D[1].isEx = false; D[0].td = e0 + 180000; M = marksByBar(D, ends, 180, 3); CHECK(M.count(1) && M[1].td == e0 + 180000);   // at the candle's end = that candle
        std::vector<long long> gap = { e0, e0 + 3600000 }; std::vector<Dec> G(1); G[0].isEx = true; G[0].td = e0 + 60000; G[0].key = "g";
        CHECK(marksByBar(G, gap, 180, 1).empty());                                     // inside a gap: no candle holds it
    }
    // ---- placement ----
    {   Box area; area.l = 0; area.t = 0; area.r = 1000; area.b = 800; Box none;
        Place p = place(500, 300, 400, RES, 20, 14, {}, none, area); CHECK(p.ok && p.box.b == 298 && p.box.t == 284 && p.box.l == 490 && !p.nudged);
        p = place(500, 300, 400, SUP, 20, 14, {}, none, area); CHECK(p.ok && p.box.t == 402);
        Box bo; bo.l = 488; bo.r = 512; bo.t = 284; bo.b = 298;                              // a BO letter in the same spot
        p = place(500, 300, 400, RES, 20, 14, { bo }, none, area); CHECK(p.ok && p.nudged && !hits(p.box, bo) && p.box.b <= bo.t && p.box.l == 490);   // one step aside is not free: further out, still centred
        Box nar; nar.l = 486; nar.r = 496; nar.t = 284; nar.b = 298;                         // a small circle: one step sideways clears it
        p = place(500, 300, 400, RES, 20, 14, { nar }, none, area); CHECK(p.ok && p.nudged && !hits(p.box, nar) && p.box.t == 284 && p.box.l != 490);
        Box nbR; nbR.l = 506; nbR.r = 520; nbR.t = 200; nbR.b = 420; Box nbL = nbR; nbL.l = 480; nbL.r = 494;   // neighbour candles both sides
        p = place(500, 300, 400, RES, 20, 14, { nar, nbR, nbL }, none, area); CHECK(p.ok && !hits(p.box, nar) && !hits(p.box, nbR) && !hits(p.box, nbL));
        Box table; table.l = 470; table.r = 700; table.t = 250; table.b = 300;               // the HOD/LOD table over it
        p = place(500, 300, 400, RES, 20, 14, {}, table, area); CHECK(p.ok ? !hits(p.box, table) : p.kept);
        Box wide; wide.l = 0; wide.r = 1000; wide.t = 0; wide.b = 300;
        p = place(500, 300, 400, RES, 20, 14, {}, wide, area); CHECK(!p.ok && p.kept);      // nowhere outside it: not drawn
        p = place(5, 300, 400, RES, 20, 14, {}, none, area); CHECK(p.ok && p.box.l >= 0);   // at the left edge: nudged inside
        p = place(500, 5, 400, RES, 20, 14, {}, none, area); CHECK(!p.ok);                  // no room above the candle
    }
    // ---- other plugins' records ----
    {   long long t0 = 0; int dir = 0; double lo = 0, hi = 0, x = 0;
        auto boLine = [](const char* fired, const char* ver, const char* rule) { std::string s = std::string("BBO|1790|1789|1|") + fired + "|fired|7840|1";
            for (int k = 8; k <= 22; ++k) s += "|1"; return s + "|" + ver + "|" + rule + "|1|0"; };   // 27 fields: [23] version, [24] rule
        const std::string bo = boLine("1", "1.31.0", "5");
        CHECK(parseBoMark(bo, t0, dir) && t0 == 1789 && dir == 1);
        CHECK(!parseBoMark(boLine("0", "1.31.0", "5"), t0, dir));   // not fired
        CHECK(!parseBoMark(boLine("1", "1.29.0", "3"), t0, dir));        // old rule
        CHECK(parseDeltaA("1790000|x|A|support|y|7840.25|7840.75|z", t0, lo, hi) && t0 == 1790000 && near(lo, 7840.25));
        CHECK(!parseDeltaA("1790000|x|AE|support|y|7840.25|7840.75|z", t0, x, x));
    }
    // ---- dump random visits for the Python parity check ----
    if (argc >= 3 && std::string(argv[1]) == "dump") {
        std::mt19937 rng(20261027); std::ofstream o(std::string(argv[2]) + "/visits.csv"), r(std::string(argv[2]) + "/cpp.csv");
        o << "case,t_ms,px,lots,aggr\n"; r << "case,side,lo,hi,ta,exh,share,is_ex\n";
        for (int c = 0; c < 400; ++c) {
            Engine E; E.setMarket("ES"); const int side = rng() % 2 ? SUP : RES; const Tick lvl = 30000;
            E.setZones({ {"PDH", lvl} }, 4);
            if (side == SUP) E.closeBar(lvl + 40, lvl + 10, lvl + 20); else E.closeBar(lvl - 10, lvl - 40, lvl - 20);
            long long t = ctAt(10, 0, 0) + (long long)(rng() % 1000); Tick px = side == SUP ? lvl + 5 : lvl - 5;
            const int n = 200 + (int)(rng() % 1400); const double fade = rng() % 3 == 0 ? 0.04 : 0.1 + (rng() % 1000) / 1000.0 * 1.6;   // a third fade hard (Ex cases)
            std::vector<long long> tt; std::vector<Tick> pp; std::vector<double> ll; std::vector<int> aa;
            bool arrived = false;
            for (int i = 0; i < n; ++i) {
                t += 1 + (long long)(rng() % 700);
                if (!arrived) { px += side == SUP ? -1 : 1; if (px >= lvl - 4 && px <= lvl + 4) arrived = true; }
                else { const int mv = (int)(rng() % 9) - 4; if (rng() % 3 == 0) px += mv > 2 ? 1 : mv < -2 ? -1 : 0; if (rng() % 97 == 0) px += (side == SUP ? 6 : -6); }
                const bool second = arrived && !tt.empty() && t - tt.front() > 90000;
                const double lots = 1 + (rng() % 12) * (second ? fade : 1.0);
                const int aggr = rng() % 10 == 0 ? NOATT : (rng() % 2 ? BUYER : SELLER);
                tt.push_back(t); pp.push_back(px); ll.push_back(std::round(lots)); aa.push_back(aggr);
                E.onTrade(t, px, std::round(lots), aggr);
            }
            E.step(t + 400000);
            for (size_t i = 0; i < tt.size(); ++i) o << c << "," << tt[i] << "," << pp[i] << "," << ll[i] << "," << aa[i] << "\n";
            if (!E.decided.empty()) { const Dec& d = E.decided[0]; r << c << "," << (d.side == SUP ? "support" : "resistance") << "," << d.lo << "," << d.hi << "," << d.ta << "," << d.exh << "," << d.share << "," << (d.isEx ? 1 : 0) << "\n"; }
        }
        std::printf("dumped 400 random visits\n");
    }
    std::printf("%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
