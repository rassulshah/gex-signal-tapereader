// test_oh.cpp -- DealerHedgeLogic.h 0.1.0 (lsDealerHedge "oH" zones): params, levels, ATR, each detection path alone, the side logic,
// LIVE -> SETTLED -> LIVE again, never removed / never changed, the record round trip (no repaint), the null shift, hover, pulse.
// From plugin\:  g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. tests_oh/test_oh.cpp -o /tmp/toh && /tmp/toh
#include "DealerHedgeLogic.h"
#include <cstdio>
using namespace dh;
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static const double TK = 0.25;
static const long long DAY = 20368LL * 86400000LL;            // a CT day (ms), session boundaries at 17:00
static long long at(int h, int m, int s = 0) { return DAY + ((h * 60 + m) * 60 + s) * 1000LL; }
static Tick T(double px) { return (Tick)std::llround(px / TK); }
static Engine make(double burstX = 100) {
    Engine E; E.market = "ES"; E.tick = TK; E.P = defaultsFor(M_ES); E.P.minLots = 10; E.P.burstX = burstX; E.P.repMinLots = 3;
    for (int s = 0; s < 1440; s += 30) { SlotNorm n; n.add[0] = n.add[1] = 5; n.fill[0] = n.fill[1] = 5; E.slotNorm[s] = n; }
    E.newSession(sessionOf(at(10, 0)));
    Level L; L.name = "CR 0DTE"; L.px = 7840; L.zero = true; L.prio = 0;
    Level H; H.name = "HVL"; H.px = 7800; H.prio = 6;
    E.setLevels({ L, H }); E.setAtr(4.0); E.trade(T(7842));
    return E;
}
static const Zone* zoneOf(const Engine& E, const std::string& nm) { for (const Zone& z : E.zones) if (z.name.find(nm) == 0) return &z; return nullptr; }
int main() {
    // ---- params ----
    { ParamLoad L = parseParams("", false, "ES"); CHECK(L.p.minLots == 150 && L.p.repMinLots == 10 && L.source.find("defaults") == 0, "no file = per-market defaults");
      L = parseParams("VERSION,1\nMARKET,ES\nPARAM,FLOW_X,2.5\nPARAM,REP_N,5\nPARAM,MIN_LOTS,-3\nPARAM,REP_CV,abc\nNORM,600,10,11,12,13\nNORM,605,1,1,1,1\n", true, "ES");
      CHECK(L.p.flowX == 2.5 && L.p.repN == 5 && L.p.minLots == 150, "good rows used, out-of-range / garbled rows ignored");
      CHECK(L.norms.size() == 1 && L.norms.count(600) && L.norms[600].fill[1] == 13, "NORM rows on 30-min slots only");
      CHECK(L.notes.size() == 3, "each bad row noted");
      L = parseParams("VERSION,1\nMARKET,NQ\nPARAM,FLOW_X,3\n", true, "ES"); CHECK(L.p.flowX == 2.0 && L.source.find("another market") != std::string::npos, "wrong market = defaults");
      L = parseParams("PARAM,FLOW_X,3\n", true, "ES"); CHECK(L.p.flowX == 2.0, "no VERSION / MARKET = defaults"); }
    // ---- levels: the Dealer Profile's MenthorQLevels.csv parse ----
    { const std::string mq = "SYMBOL,PRICE,LABEL,PENCOLOR\nEPZ26,7840.000000,CR 0D / GW 0D,1\nEPZ26,7725,PS 0D,1\nEPZ26,7800,HVL,1\nEPZ26,7750,G1,1\nEPZ26,7760,G10,1\nEPZ26,7770,G11,1\nEPH27,7900,CR,1\nENQZ26,29810,CR 0D,1\nEPZ26,bad,PS,1\n";
      std::vector<Level> v = parseMq(mq, "ES", "ESZ26", TK);
      CHECK(v.size() == 6, "ES Dec levels only: CR 0DTE, GW 0DTE, PS 0DTE, HVL, G1, G10 (G11, March, NQ, bad price out)");
      bool cr = false, gw = false; for (const Level& l : v) { if (l.name == "CR 0DTE" && l.zero && l.px == 7840) cr = true; if (l.name == "GW 0DTE") gw = true; }
      CHECK(cr && gw, "split on '/', 0D -> 0DTE, no 'MQ' in names");
      std::vector<Level> nodes; std::string why;
      CHECK(parseNodes("MARKET|ES\nASOF|1000\nSTR|1|7840|7840|20|21.5|W|1|0|CR0|1\nSTR|2|7790|7790|10|9.0|F|1|0|-|2\n", "ES", 1500, 3600, TK, nodes, why) && nodes.size() == 2, "nodes parsed");
      std::vector<Level> m = mergeNodes(v, nodes, TK); int n7790 = 0; double g = -1; char sg = 0;
      for (const Level& l : m) { if (l.name == "CR 0DTE") { g = l.gexM; sg = l.sign; } if (l.name == "Node 7790") ++n7790; }
      CHECK(g == 21.5 && sg == 'W' && n7790 == 1, "a node on a level gives it |GEX| + sign; other nodes are their own levels");
      CHECK(parseNodes("MARKET|ES\nASOF|1000\nSTR|1|7840|7840|20|21.5|W|1|0|CR0|1\n", "ES", 100000, 3600, TK, nodes, why) && nodes.empty() && why == "nodes stale", "stale nodes = none");
      CHECK(!parseNodes("MARKET|NQ\n", "ES", 0, 3600, TK, nodes, why), "nodes of another market refused"); }
    // ---- ATR ----
    { std::vector<double> h, l, c; for (int i = 0; i < 20; ++i) { h.push_back(102); l.push_back(98); c.push_back(100); }
      CHECK(std::fabs(atrOf(h, l, c, 14) - 4) < 1e-9, "ATR(14) of 4-point bars = 4"); CHECK(atrOf(h, l, c, 30) == 0, "too few bars = 0 (no zones)"); }
    // ---- invisible until activity; (a) same-size repeats alone -> BUY ----
    { Engine E = make(); CHECK(E.zones.size() == 2 && E.zones[0].state == HIDDEN, "zones start HIDDEN");
      long long id = 100;
      for (int i = 0; i < 6; ++i) { E.onEvent(NEW, 0, T(7839.5), 6, id++, at(10, 0, 2 * i)); E.evaluate(at(10, 0, 2 * i), 777); }
      const Zone* z = zoneOf(E, "CR 0DTE");
      CHECK(z && z->state == LIVE && z->side == BUY && z->paths == "a", "(a) 6 x 6-lot bids every 2 s -> lit, BUY, path a only");
      CHECK(z && z->repSize == 6 && z->repCount >= 4 && z->repMedMs == 2000 && z->barKey == 777, "repeat size / count / interval / candle recorded");
      CHECK(z && z->lo == T(7838) && z->hi == T(7842), "zone = level +/- 0.5 ATR (2 pts)");
      CHECK(z && z->pos == 1 && z->posSrc == 'H' && z->expect == BUY, "price above HVL = long gamma; price above the level = support: expect BUY");
      CHECK(E.rows.size() == 2 && E.rows[0][0] == 'Z' && E.rows[1][0] == 'L', "first decision recorded: Z + L"); }
    // the same repeats with irregular spacing (CV >= 0.5) do not count
    { Engine E = make(); long long id = 100; const int t[6] = { 0, 1, 9, 10, 25, 26 };
      for (int i = 0; i < 6; ++i) { E.onEvent(NEW, 0, T(7839.5), 6, id++, at(10, 0, t[i])); E.evaluate(at(10, 0, t[i]), 1); }
      CHECK(zoneOf(E, "CR 0DTE")->state == HIDDEN, "irregular same-size orders: not programmatic"); }
    // ---- (b) refills alone (different sizes: no repeat) ----
    { Engine E = make(); const int sz[5] = { 5, 6, 7, 8, 9 }; long long id = 500;
      E.onEvent(NEW, 0, T(7840), 2, id, at(10, 0, 0));
      for (int i = 0; i < 5; ++i) {
          const long long t0 = at(10, 0, 3 * i + 1);
          E.onEvent(FILL, 1, T(7840), 2, id, t0); E.onEvent(DEL, 0, T(7840), 0, id, t0);              // hit (an aggressive 2-lot sell)
          E.onEvent(NEW, 0, T(7840), sz[i], ++id, t0 + 200);                                            // refilled 200 ms later
          E.evaluate(t0 + 1500, 5);
      }
      E.evaluate(at(10, 0, 20), 5);
      const Zone* z = zoneOf(E, "CR 0DTE");
      CHECK(z && z->state == LIVE && z->side == BUY && z->paths == "b" && z->refCount >= 4 && z->refPx == T(7840), "(b) refills at 7,840 that rest 1 s+ -> BUY, path b only"); }
    { Engine E = make(); long long id = 500; E.onEvent(NEW, 0, T(7840), 2, id, at(10, 0, 0));
      for (int i = 0; i < 6; ++i) { const long long t0 = at(10, 0, 3 * i + 1);
          E.onEvent(FILL, 1, T(7840), 2, id, t0); E.onEvent(DEL, 0, T(7840), 0, id, t0);
          E.onEvent(NEW, 0, T(7840), 5 + i, ++id, t0 + 100); E.onEvent(DEL, 0, T(7840), 0, id, t0 + 400);   // flashed and pulled
          E.onEvent(NEW, 0, T(7840), 2, ++id, t0 + 2900); E.evaluate(t0 + 2900, 5); }
      CHECK(zoneOf(E, "CR 0DTE")->paths.find('b') == std::string::npos, "refills pulled within 1 s do not count"); }
    // ---- (c) a burst alone; aggressive sells -> SELL ----
    { Engine E = make(2.0); E.onEvent(NEW, 0, T(7841), 40, 900, at(10, 0, 0)); E.evaluate(at(10, 0, 1), 9);
      const Zone* z = zoneOf(E, "CR 0DTE"); CHECK(z && z->state == LIVE && z->paths == "c" && z->side == BUY, "(c) one 40-lot bid = 8x the add normal -> burst, BUY"); }
    { Engine E = make(2.0); E.onEvent(NEW, 0, T(7841), 50, 901, at(10, 0, 0));
      for (int i = 0; i < 5; ++i) E.onEvent(FILL, 1, T(7841), 6, 901, at(10, 0, i));
      E.evaluate(at(10, 0, 5), 9); const Zone* z = zoneOf(E, "CR 0DTE");
      CHECK(z && z->side == BUY, "the resting 50-lot bid (adds 50) outweighs 30 sold");
      Engine F = make(2.0); F.onEvent(NEW, 0, T(7841), 3, 901, at(10, 0, 0));
      for (int i = 0; i < 8; ++i) F.onEvent(FILL, 1, T(7841), 4, 901, at(10, 0, i));
      F.evaluate(at(10, 0, 9), 9); const Zone* y = zoneOf(F, "CR 0DTE");
      CHECK(y && y->state == LIVE && y->side == SELL, "aggressive sells into the zone -> SELL (red)");
      CHECK(y && y->expect == BUY, "long gamma at support expects BUY: the hover notes the book disagrees"); }
    // below the 2.0x normal / below MIN_LOTS: nothing
    { Engine E = make(2.0); E.onEvent(NEW, 0, T(7841), 9, 902, at(10, 0, 0)); E.evaluate(at(10, 0, 1), 9);
      CHECK(zoneOf(E, "CR 0DTE")->state == HIDDEN, "9 lots (1.8x, under MIN_LOTS) = stays invisible"); }
    // outside the zone: nothing
    { Engine E = make(2.0); E.onEvent(NEW, 0, T(7836), 40, 903, at(10, 0, 0)); E.evaluate(at(10, 0, 1), 9);
      CHECK(zoneOf(E, "CR 0DTE")->state == HIDDEN, "a burst 4 pts from the level (outside +/- 2) does not light it"); }
    // ---- LIVE -> SETTLED -> LIVE again; never removed, never changed ----
    { Engine E = make(2.0); E.onEvent(NEW, 0, T(7841), 40, 904, at(10, 0, 0)); E.evaluate(at(10, 0, 1), 9);
      const Zone before = *zoneOf(E, "CR 0DTE");
      E.setAtr(8.0);                                                                                   // ATR doubles later
      for (int s = 2; s <= 30; ++s) E.evaluate(at(10, 0, s), 10);
      const Zone* z = zoneOf(E, "CR 0DTE"); CHECK(z && z->state == SETTLED, "20 s after the last activity: SETTLED (faint band)");
      CHECK(z && z->lo == before.lo && z->hi == before.hi && z->side == before.side && z->firstMs == before.firstMs && z->barKey == 9, "the lit band never changes (ATR moved)");
      E.onEvent(FILL, 1, T(7840.5), 30, 904, at(10, 1, 0)); E.evaluate(at(10, 1, 0), 11);
      z = zoneOf(E, "CR 0DTE"); CHECK(z && z->state == LIVE && z->episodes == 2 && z->liveSide == SELL && z->side == BUY, "activity restarts: pulses again (this burst's side), band colour = the first decision");
      int Z = 0, Lc = 0; for (auto& r : E.rows) { if (r[0] == 'Z') ++Z; if (r[0] == 'L') ++Lc; } CHECK(Z == 1 && Lc == 2, "one Z line per zone, one L per live episode");
      for (int s = 1; s < 120; ++s) E.evaluate(at(10, 1, s), 12);
      CHECK(zoneOf(E, "CR 0DTE") != nullptr, "never removed");
      // ---- the record round trip: a restart redraws the same zone, settled, and keeps deciding the same way ----
      Engine R = make(2.0); R.setAtr(8.0); const int n = R.restore(E.rows);
      const Zone* r = zoneOf(R, "CR 0DTE");
      CHECK(n == 1 && r && r->state == SETTLED && r->lo == before.lo && r->hi == before.hi && r->side == BUY && r->barKey == 9 && r->episodes == 2, "restore: same band, side, candle, episodes");
      CHECK(r && r->flowX == before.flowX && r->lots == before.lots && r->zero && r->expect == before.expect && r->pos == before.pos, "restore: the hover facts are the same");
      CHECK(r && R.zLine(*r) == E.rows[0], "restored Z line is byte-identical (no repaint)");
      std::vector<std::string> torn = E.rows; torn.push_back("Z|half|line"); torn.push_back(E.rows[0]);
      Engine R2 = make(2.0); CHECK(R2.restore(torn) == 1, "torn and duplicate lines skipped"); }
    // ---- merged levels, the null shift, the gamma node sign ----
    { Engine E = make(); Level G; G.name = "G1"; G.px = 7841; G.prio = 9; G.gexM = 12; G.sign = 'F';
      Level L; L.name = "CR 0DTE"; L.px = 7840; L.zero = true; L.prio = 0; E.setLevels({ G, L }); E.setAtr(4.0);
      CHECK(E.zones.size() == 1 && E.zones[0].name == "CR 0DTE/G1" && E.zones[0].lvl == T(7840), "levels within half a zone share one (priority leads)");
      E.onEvent(NEW, 0, T(7841), 40, 1, at(10, 0, 0)); E.P.burstX = 2; E.evaluate(at(10, 0, 1), 3);
      CHECK(E.zones[0].pos == -1 && E.zones[0].posSrc == 'N' && E.zones[0].expect == SELL, "node sign F = short gamma; falling into the level -> expect SELL");
      Engine N = make(); N.P.nullShiftAtr = 2.0; N.setLevels({ L }); N.setAtr(4.0);
      CHECK(N.zones.size() == 1 && N.zones[0].lvl == T(7848), "null zone = level + 2 ATR"); }
    // ---- normal: learning, rolling ----
    { Engine E; E.market = "ES"; E.tick = TK; E.P = defaultsFor(M_ES); E.P.minLots = 10; E.P.burstX = 2; E.newSession(sessionOf(at(10, 0)));
      Level L; L.name = "PS"; L.px = 7840; L.prio = 5; E.setLevels({ L }); E.setAtr(4.0); E.trade(T(7840));
      E.onEvent(NEW, 0, T(7840), 500, 1, at(10, 0, 0)); E.evaluate(at(10, 0, 1), 1);
      CHECK(E.zones[0].state == HIDDEN && E.normSrc == "learning", "no normal yet (no slot row, < 10 minutes): no decision");
      long long id = 10; for (int m = 1; m <= 12; ++m) { E.onEvent(NEW, 1, T(7840.5), 3, id++, at(10, m, 5)); E.evaluate(at(10, m, 6), 1); }
      CHECK(E.normSrc == "rolling", "10+ minutes measured: rolling normal");
      for (int k = 0; k < 6; ++k) E.onEvent(NEW, 0, T(7839.75), 20, id++, at(10, 13, k));
      E.evaluate(at(10, 13, 7), 2);
      CHECK(E.zones[0].state == LIVE && E.zones[0].side == BUY, "120 lots of bids vs a quiet normal -> lit");
      for (int m = 14; m <= 61; ++m) E.evaluate(at(10, m, 0), 3);
      CHECK(!E.normRows.empty() && E.normRows[0].compare(0, 2, "N|") == 0, "a finished slot writes its N row");
      Engine F; F.P = defaultsFor(M_ES); std::vector<std::string> h = { "N|100|600|500|600|700|800|30", "N|101|600|300|400|500|600|30", "N|102|600|100|100|100|100|5", "N|103|600|1|1|1|1|30" };
      F.loadNormHistory(h, 103); CHECK(F.slotNorm.count(600) && std::fabs(F.slotNorm[600].add[0] - 4.0) < 1e-9, "slot normal = median of earlier sessions (short slot / today ignored)"); }
    // ---- hover, pulse, colour ----
    { Engine E = make(2.0); E.onEvent(NEW, 0, T(7841), 40, 904, at(10, 0, 0)); E.evaluate(at(10, 0, 1), 9);
      Zone z = *zoneOf(E, "CR 0DTE"); z.gexM = 21.5; std::vector<HoverLine> H = hover(z, "ES", TK, at(10, 0, 2), 0);
      CHECK(H[0].text == "oH ? Dealer hedging BUYING at CR 0DTE 7,840.00", "header: oH ?, side, level name + price");
      bool q = false, d = false, sz = false, ex = false, how = false, eg = false, zero = false, mq = false;
      for (auto& l : H) { if (l.text.find("Question:") == 0) q = true;
        if (l.text.find("Dealers: LONG gamma") == 0) d = true;
        if (l.text.find("Size: 40 lots") == 0 && l.text.find("contracts") != std::string::npos) sz = true;
        if (l.text.find("What it is:") == 0) ex = true;
        if (l.text.find("How to use:") == 0) how = true;
        if (l.text.find("Example:") == 0) eg = true;
        if (l.text.find("0DTE level: expires 3:00 PM") == 0) zero = true;
        if (l.text.find("MQ") != std::string::npos) mq = true; }
      CHECK(q && d && sz && ex && how && eg && zero && !mq, "hover: question / what / dealers / size ($ + contracts) / 0DTE / how / example, no MQ");
      CHECK(clock12(at(13, 5)) == "1:05 PM" && priceText(29810.25, TK) == "29,810.25", "12-hour CT, thousands");
      CHECK(pulse(0) < 0.01 && pulse(500) > 0.99 && pulse(1000) < 0.01, "pulse: 1 s breath from the wall clock");
      CHECK(blend(0x00FF0000, 0, 0.5) == 0x00800000 && blend(0x0000FF00, 0, 0) == 0, "blend toward the background");
      CHECK(intensity(LIVE, 0, true, 30) >= 0.45 && intensity(LIVE, 500, true, 30) <= 0.85 + 1e-9 && std::fabs(intensity(SETTLED, 0, true, 30) - 0.30) < 1e-9 && intensity(HIDDEN, 0, true, 30) == 0, "intensity: live 0.45-0.85, settled faint, hidden 0"); }
    // ---- session change: a new session starts invisible again ----
    { Engine E = make(2.0); E.onEvent(NEW, 0, T(7841), 40, 904, at(10, 0, 0)); E.evaluate(at(10, 0, 1), 9);
      E.evaluate(at(17, 0, 5), 20); CHECK(zoneOf(E, "CR 0DTE") && zoneOf(E, "CR 0DTE")->state == HIDDEN, "17:00 CT: the new session's zones start hidden"); }
    std::printf("test_oh: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
