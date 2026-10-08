// test_tapeflow_logic.cpp - behavioural tests of TapeFlowLogic.h on FABRICATED tick fixtures (brief section 15).
// These prove the logic does what the brief says; they are NOT evidence of any trading edge.
// build: g++ -std=c++17 -O1 -Wall -Wextra -o t test_tapeflow_logic.cpp && ./t
#include "TapeFlowLogic.h"
#include <cstdio>
#include <functional>
using namespace tfl;

static int fails = 0, passes = 0;
#define CHECK(c, msg) do { if (c) passes++; else { fails++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); } } while (0)

static const long long T0 = 1790000000LL - (1790000000LL % 86400) + 9 * 3600;   // a local 09:00
struct Fx {
    std::vector<Tick> ticks; long long t = T0; int bid = 1000, ask = 1001;
    void q(int b, int a) { bid = b; ask = a; }
    void buy(long n, int px = 0) { ticks.push_back(Tick{t, px ? px : ask, bid, ask, n}); }
    void sell(long n, int px = 0) { ticks.push_back(Tick{t, px ? px : bid, bid, ask, n}); }
    void unk(long n) { ticks.push_back(Tick{t, bid, bid - 1, bid + 1, n}); }   // a trade between the quotes... (bid-1/bid+1 around px)
    // one second of routine two-way trade: 10 bought at the ask, 10 sold at the bid
    void routine(int secs, long b = 10, long s = 10) { for (int i = 0; i < secs; i++) { buy(b); sell(s); t++; } }
};

static BinBase stdBase(float qs = 300, float qb = 300)
{
    BinBase b; b.n = 100; for (int h = 0; h < 8; h++) { b.qb[h] = qb; b.qs[h] = qs; } b.med20 = 20; b.med5 = 20; b.medSpread = 1; return b;
}
static Engine run(const std::vector<Tick>& ticks, const BinBase& b, bool interleave = false, long long endAdvance = 0)
{
    Engine e; e.setFixedBase(b);
    for (const Tick& k : ticks) { if (interleave) e.advanceTo(k.t - 1); e.add(k); }
    if (endAdvance) e.advanceTo(endAdvance); else if (!ticks.empty()) e.advanceTo(ticks.back().t);   // close the last second
    return e;
}
static int count(const Engine& e, const char* kind, int dir = 0)
{
    int n = 0; for (auto& v : e.evs) if (v.kind == kind && (dir == 0 || v.dir == dir)) n++; return n;
}
static const Ev* first(const Engine& e, const char* kind)
{
    for (auto& v : e.evs) if (v.kind == kind) return &v;
    return nullptr;
}
static void dump(const Engine& e) { for (auto& v : e.evs) printf("   %lld %s dir %d band %d-%d %s\n", v.t - T0, v.kind.c_str(), v.dir, v.lo, v.hi, v.why.c_str()); }

// the standard qualifying watch: 250 s routine, then sellers hammer 1000 (20/s, + 10 routine) without progress
static Fx watchFixture(int attackSecs = 30)
{
    Fx f; f.routine(250);
    for (int i = 0; i < attackSecs; i++) { f.buy(10); f.sell(30); f.t++; }
    return f;
}

static Fx mirror(const Fx& f)
{
    Fx m = f; for (Tick& k : m.ticks) { int b = k.bid, a = k.ask; k.px = 4000 - k.px; k.bid = 4000 - a; k.ask = 4000 - b; } return m;
}

int main()
{
    BinBase B = stdBase();

    // basics
    CHECK(sideOf(1001, 1000, 1001) == SIDE_BUY && sideOf(1000, 1000, 1001) == SIDE_SELL && sideOf(1000, 999, 1001) == SIDE_UNK, "aggressor rule");
    CHECK(sideOf(1000, 0, 1001) == SIDE_UNK, "no quote = unknown, never the tick rule");
    { std::vector<float> v = {5, 1, 4, 2, 3}; CHECK(nearestRank(v, 0.95) == 5 && nearestRank(v, 0.5) == 3, "nearest rank ceil(pN)-1"); }
    { long long a = T0 - 9 * 3600 + 17 * 3600; CHECK(sessionOf(a) == sessionOf(a + 86399) && sessionOf(a - 1) == sessionOf(a) - 1, "17:00 session boundary");
      CHECK(binOf(a) == 0 && binOf(a + 299) == 0 && binOf(a + 300) == 1, "5-min bins from 17:00"); }

    // 1. routine balance: two-way trade, flat price -> no watch
    { Fx f; f.routine(400); Engine e = run(f.ticks, B);
      CHECK(count(e, "AW") == 0 && count(e, "CAND") == 0, "1 routine balance: no AW");
      CHECK(!e.hist.empty() && std::fabs(e.hist.back().f30) < 1e-6, "F30 = 0 on balanced tape");
      CHECK(e.hist.back().a > 0.99f && e.hist.back().a < 1.01f, "A = 1 at the median rate"); }

    // 3. true watch: exactly one AW+, only after the 3-s hold
    { Fx f = watchFixture(); Engine e = run(f.ticks, B);
      const Ev* c = first(e, "CAND"); const Ev* w = first(e, "AW");
      CHECK(c && w, "3 qualifying watch fires"); if (!(c && w)) dump(e);
      if (c && w) { CHECK(w->t - c->t == 3, "3 AW 3 s after the candidate"); CHECK(w->dir == 1, "3 sellers stalled = bullish watch");
                    CHECK(w->lo <= 1000 && w->hi >= 1000, "3 band at the selling"); CHECK(c->qzone >= c->q95 && c->conc >= 0.5, "3 candidate effort >= Q95, concentrated"); }
      CHECK(count(e, "AW") == 1, "13 one AW per episode"); }

    // 2. pure exhaustion: heavy selling, then quiet (only a trickle) -> no NEW watch
    { Fx f; f.routine(250); int p = 1000; for (int i = 0; i < 16; i++) { p--; f.q(p, p + 1); f.buy(10); f.sell(60); f.t++; } for (int i = 0; i < 40; i++) { f.buy(1); f.sell(1); f.t++; }
      Engine e = run(f.ticks, B); CHECK(count(e, "AW") == 0, "2 exhaustion then quiet: no AW"); }

    // 4. wrong location / spread selling: heavy selling spread over many prices while price falls -> no AW
    { Fx f; f.routine(250); int p = 1000; for (int i = 0; i < 40; i++) { if (i % 2 == 0) p--; f.q(p, p + 1); f.buy(10); f.sell(30); f.t++; }
      Engine e = run(f.ticks, B); CHECK(count(e, "AW", 1) == 0, "4/6 selling that makes progress: no AW+"); }

    // 5. balanced net delta: big buying AND selling, only the selling is unusual for its side -> AW+ (tagged two-way)
    { Fx f; f.routine(250); for (int i = 0; i < 30; i++) { f.buy(30); f.sell(30); f.t++; }
      Engine e = run(f.ticks, stdBase(300, 1000)); const Ev* w = first(e, "AW");
      CHECK(w && w->dir == 1, "5 two-way execution with qualifying sell effort -> AW+"); if (!w) dump(e);
      if (w) CHECK(w->why.find("two-way") != std::string::npos, "5 tagged two-way"); }
    // ... and if both sides qualify in the same zone: balance, no watch
    { Fx f; f.routine(250); for (int i = 0; i < 30; i++) { f.buy(30); f.sell(30); f.t++; }
      Engine e = run(f.ticks, B); CHECK(count(e, "AW") == 0 && count(e, "BAL") >= 1, "5b both sides qualify: TWO-SIDED BALANCE, no AW"); }

    // 6. path matters: price dips 6 ticks during the selling and comes back -> progress > h -> no AW
    { Fx f; f.routine(250); for (int i = 0; i < 30; i++) { int d = (i >= 2 && i < 6) ? 6 : 0; f.q(1000 - d, 1001 - d); f.buy(10); f.sell(d ? 5 : 30); f.t++; }
      Engine e = run(f.ticks, B); bool early = false; for (auto& v : e.evs) if (v.kind == "AW" && v.t < T0 + 250 + 6 + 20) early = true;
      CHECK(!early, "6 same endpoint after a big adverse excursion: no AW while the excursion is in the window"); if (early) dump(e); }

    // 7. frozen zone: after AW+, lower selling cannot move the band down
    { Fx f = watchFixture(); f.q(998, 999); for (int i = 0; i < 2; i++) { f.buy(10); f.sell(30); f.t++; }
      Engine e = run(f.ticks, B); const Ev* w = first(e, "AW"); bool same = true;
      for (auto& v : e.evs) if (w && v.ep == w->ep && (v.lo != w->lo || v.hi != w->hi)) same = false;
      CHECK(w && same, "7 the episode's band never moves"); }

    // 8. sweep: a 2-s dip below the band and back is not a failure; a sustained, executed breach is FAIL
    { Fx f = watchFixture(); const int lo = 999;
      f.q(lo - 3, lo - 2); for (int i = 0; i < 2; i++) { f.buy(10); f.sell(30); f.t++; }
      f.q(1000, 1001); for (int i = 0; i < 5; i++) { f.buy(10); f.sell(10); f.t++; }
      Engine e1 = run(f.ticks, B); CHECK(count(e1, "AW") == 1 && count(e1, "FAIL") == 0 && count(e1, "INVP") == 0, "8 a sweep and reclaim is not a failure");
      f.q(lo - 3, lo - 2); for (int i = 0; i < 4; i++) { f.buy(10); f.sell(30); f.t++; }
      Engine e2 = run(f.ticks, B); CHECK(count(e2, "FAIL") == 1, "8 sustained breach with selling = FAILED_ACCEPTANCE"); if (!count(e2, "FAIL")) dump(e2);
      CHECK(count(e2, "AW") == 1, "8 the AW marker stays"); }

    // 9. quote-led breach: price sits below for 4 s but almost nothing sells there -> INVALIDATED_PRICE
    { Fx f = watchFixture(); f.q(996, 997); for (int i = 0; i < 4; i++) { f.buy(5); f.sell(1); f.t++; }
      Engine e = run(f.ticks, B); CHECK(count(e, "INVP") == 1 && count(e, "FAIL") == 0, "9 quote-led breach = INVALIDATED_PRICE"); if (!count(e, "INVP")) dump(e); }

    // 10. response: reclaim above the band with fresh buying and improving flow -> one AR+; a quote-only reclaim does not
    { Fx f = watchFixture(); f.q(1002, 1003); for (int i = 0; i < 40; i++) { f.buy(30); f.sell(5); f.t++; }
      Engine e = run(f.ticks, B); CHECK(count(e, "AR", 1) == 1, "10 AR+ after reclaim + buying"); if (count(e, "AR", 1) != 1) dump(e);
      const Ev* a = first(e, "AR"); const Ev* w = first(e, "AW");
      if (a && w) { CHECK(a->f30 >= 0 && a->f30 - 0 >= 0, "10 F30 >= 0 at AR"); CHECK(a->ep == w->ep, "10 AR linked to its AW"); } }
    { Fx f = watchFixture(); f.q(1002, 1003); for (int i = 0; i < 40; i++) { f.sell(3); f.t++; }    // quotes up, only selling
      Engine e = run(f.ticks, B); CHECK(count(e, "AR") == 0, "10 quote-only reclaim: no AR"); }

    // 11. context: an AR may come against a negative 180-s line (tagged opposing); IN never claims alignment without L180 > 0
    { Fx f; f.routine(100, 5, 25); f.routine(150); for (int i = 0; i < 30; i++) { f.buy(10); f.sell(30); f.t++; }
      f.q(1002, 1003); for (int i = 0; i < 12; i++) { f.buy(40); f.sell(1); f.t++; }
      Engine e = run(f.ticks, B); const Ev* a = first(e, "AR");
      if (a) CHECK(a->ctx == "opposing" || a->ctx == "aligned" || a->ctx == "unknown", "11 AR context tag present");
      for (auto& v : e.evs) if (v.kind == "IN" && v.dir == 1) CHECK(v.ctx == "aligned", "11 IN only when aligned"); }

    // 12. repeated tests do not strengthen anything: the same zone is latched after its episode
    { Fx f = watchFixture(); f.q(996, 997); for (int i = 0; i < 4; i++) { f.sell(30); f.t++; }       // FAIL
      f.q(1000, 1001); for (int i = 0; i < 40; i++) { f.buy(10); f.sell(30); f.t++; }                  // the same zone again, price never left 2h+1 away
      Engine e = run(f.ticks, B); CHECK(count(e, "AW") == 1, "12 the latched zone does not fire again until rearmed"); }

    // 13 + 19. no duplication / speed invariance: redrawing (advance calls between trades) changes nothing
    { Fx f = watchFixture(); f.q(1002, 1003); for (int i = 0; i < 40; i++) { f.buy(30); f.sell(5); f.t++; }
      Engine a = run(f.ticks, B), b = run(f.ticks, B, true);
      bool same = a.evs.size() == b.evs.size();
      for (size_t i = 0; same && i < a.evs.size(); i++) same = a.evs[i].t == b.evs[i].t && a.evs[i].kind == b.evs[i].kind;
      CHECK(same, "19 speed / redraw invariance"); }

    // 14. unknown-side sensitivity: the reclaim's buying is printed between the quotes (unknown) -> no AR
    { Fx f = watchFixture(); f.q(1002, 1004); for (int i = 0; i < 40; i++) { f.ticks.push_back(Tick{f.t, 1003, 1002, 1004, 30}); f.buy(3); f.t++; }
      Engine e = run(f.ticks, B); CHECK(count(e, "AR") == 0, "14 unknown-side volume blocks the confirmation"); }

    // 15. expiry precedence: nothing resolves -> EXPIRED at 90 s, and nothing after it
    { Fx f = watchFixture(); for (int i = 0; i < 100; i++) { f.buy(10); f.sell(2); f.t++; }
      Engine e = run(f.ticks, B); const Ev* w = first(e, "AW"); const Ev* x = first(e, "EXP");
      CHECK(w && x && x->t - w->t == 90, "15 EXPIRED exactly 90 s after AW"); if (!(w && x)) dump(e);
      CHECK(count(e, "AR") == 0, "15 no AR after expiry"); }
    { // an AR whose hold would complete exactly at the TTL second: expiry wins
      Fx f = watchFixture(); f.t += 0;
      Engine probe = run(f.ticks, B); const Ev* w = first(probe, "AW");
      if (w) { while (f.t < w->t + 1 + 86) { f.buy(10); f.sell(2); f.t++; }      // quiet until 3 s before TTL... then reclaim
               f.q(1002, 1003); for (int i = 0; i < 10; i++) { f.buy(40); f.sell(1); f.t++; }
               Engine e = run(f.ticks, B); const Ev* x = first(e, "EXP"); const Ev* a = first(e, "AR");
               CHECK(x && (!a || a->t < x->t), "15 at the TTL second expiry beats confirmation"); } }

    // 16. mirror symmetry: sign-inverted prices / sides give the mirrored signals
    { Fx f = watchFixture(); f.q(1002, 1003); for (int i = 0; i < 40; i++) { f.buy(30); f.sell(5); f.t++; }
      Engine a = run(f.ticks, B), m = run(mirror(f).ticks, B);
      bool ok = a.evs.size() == m.evs.size() && !a.evs.empty();
      for (size_t i = 0; ok && i < a.evs.size(); i++) ok = a.evs[i].t == m.evs[i].t && a.evs[i].kind == m.evs[i].kind && a.evs[i].dir == -m.evs[i].dir &&
                                                       a.evs[i].lo == 4000 - m.evs[i].hi && a.evs[i].hi == 4000 - m.evs[i].lo;
      CHECK(ok, "16 mirror symmetry"); if (!ok) { puts("  orig"); dump(a); puts("  mirror"); dump(m); } }

    // 17 + 18. intrabar / prefix invariance: cutting the future off never changes earlier events
    { Fx f = watchFixture(); f.q(1002, 1003); for (int i = 0; i < 40; i++) { f.buy(30); f.sell(5); f.t++; }
      Engine full = run(f.ticks, B); bool ok = true;
      for (size_t cut = 1000; cut < f.ticks.size(); cut += 37) {
          std::vector<Tick> pre(f.ticks.begin(), f.ticks.begin() + (long)cut);
          Engine p = run(pre, B); long long lastT = pre.back().t;
          for (auto& v : p.evs) { if (v.t >= lastT) continue;   // the open second is not closed yet
              bool found = false; for (auto& w : full.evs) if (w.t == v.t && w.kind == v.kind && w.ep == v.ep) found = true;
              if (!found) ok = false; }
          size_t nFullBefore = 0; for (auto& w : full.evs) if (w.t < lastT) nFullBefore++;
          size_t nPre = 0; for (auto& v : p.evs) if (v.t < lastT) nPre++;
          if (nFullBefore != nPre) ok = false;
      }
      CHECK(ok, "18 prefix invariance"); }

    // 20. data failure: a gap during a watch resolves DATA_INVALID and the warm-up starts over (no hold accumulates)
    { Fx f = watchFixture(); Engine e; e.setFixedBase(B);
      for (auto& k : f.ticks) e.add(k);
      long long tg = f.t; e.gap(tg, "test gap");
      CHECK(count(e, "DINV") == 1 && count(e, "AW") == 1, "20 gap -> DATA_INVALID, AW kept");
      Fx g; g.t = tg + 1; for (int i = 0; i < 60; i++) { g.buy(10); g.sell(30); g.t++; } for (auto& k : g.ticks) e.add(k);
      CHECK(count(e, "AW") == 1, "20 no new watch inside the 200-s warm-up after a gap"); }

    // late data never rewrites a closed second; it invalidates the open episode
    { Fx f = watchFixture(); Engine e = run(f.ticks, B); size_t n = e.evs.size();
      e.add(Tick{f.t - 30, 1000, 1000, 1001, 5});
      CHECK(e.late == 1 && e.evs.size() == n + 1 && e.evs.back().kind == "DINV", "late trade -> DATA_INVALID record, nothing rewritten"); }

    // initiative: strong one-sided buying, rising, 2x activity -> IN+ once (latched)
    { Fx f; f.routine(250); int p = 1000; for (int i = 0; i < 60; i++) { if (i % 3 == 0) p++; f.q(p, p + 1); f.buy(40); f.t++; }
      Engine e = run(f.ticks, B); CHECK(count(e, "IN", 1) == 1, "IN+ on strong rising buying, once"); if (count(e, "IN", 1) != 1) dump(e);
      CHECK(count(e, "IN", -1) == 0 && count(e, "AW") == 0, "no IN- / AW on pure buying");
      Engine m = run(mirror(f).ticks, B); CHECK(count(m, "IN", -1) == 1, "IN- mirrors IN+"); }

    // calibration: no baselines -> pressure only, no signals
    { Fx f = watchFixture(); Engine e; Store st; e.store = &st; for (auto& k : f.ticks) e.add(k);
      CHECK(count(e, "AW") == 0 && count(e, "CAND") == 0 && !e.hist.empty() && !std::isnan(e.hist.back().f30), "CALIBRATING: pressure shown, no signals"); }

    // baselines from the store: windows -> nearest-rank Q95, medians, prior sessions only, each session's FRONT contract
    { Store st; long long sid = 100;
      auto mkw = [](int k, float scale) { Win w; w.bin = 3; w.rate20 = (float)(k + 1) * scale; for (int j = 0; j < 4; j++) w.r5[j] = 2; w.spread = 1; for (int h = 0; h < 8; h++) { w.Mb[h] = (float)(k + 1) * 10; w.Ms[h] = (float)(k + 1); } return w; };
      for (int s = 0; s < 5; s++) for (int k = 0; k < 20; k++) st[sid - 1 - s]["ESZ6"].push_back(mkw(k, 1));
      for (int k = 0; k < 20; k++) st[sid - 1]["ESH7"].push_back(mkw(0, 0.05f));   // the back month on roll day: thin, never the front
      for (int k = 0; k < 20; k++) st[sid - 7]["ESU6"].push_back(mkw(0, 0.01f));   // a holiday-thin session: left out (< 30% of typical)
      st[sid]["ESZ6"].push_back(Win());                                             // the current session: never used
      BinBase bb[NBINS]; int used = 0; freezeBase(st, sid, 10, bb, &used);
      CHECK(used == 5 && bb[3].n == 100, "baseline: the 5 prior sessions' front contract, thin sessions left out");
      CHECK(bb[3].qs[0] == 19 && bb[3].qb[0] == 190, "Q95 nearest rank (100 windows -> 95th = 19)");
      CHECK(bb[3].med20 == 10.5f && bb[3].med5 == 2 && bb[3].medSpread == 1, "medians");
      CHECK(bb[4].n == 0, "empty bin stays empty");
      CHECK(priorSessions(st, sid) == 6, "prior sessions counted (any contract)"); }
    // after a roll: the new front's own sessions win as soon as it is the busier contract that day
    { Store st; for (int k = 0; k < 10; k++) { Win w; w.bin = 1; w.rate20 = 10; st[50]["OLD"].push_back(w); Win v; v.bin = 1; v.rate20 = 30; for (int h = 0; h < 8; h++) v.Ms[h] = 99; st[50]["NEW"].push_back(v); }
      BinBase bb[NBINS]; int used = 0; freezeBase(st, 51, 10, bb, &used);
      CHECK(used == 1 && bb[1].med20 == 30 && bb[1].qs[0] == 99, "roll day: the busier contract is that session's front"); }

    // the engine builds windows itself and the next session's baselines come from them (end to end)
    { Store st; Engine e; e.store = &st; e.sym = "X";
      Fx f; f.t = T0; f.routine(3600); for (auto& k : f.ticks) e.add(k); e.advanceTo(f.t); e.flushSession();
      long long s0 = sessionOf(T0);
      const std::vector<Win>& W = st[s0]["X"];
      CHECK(W.size() >= 170, "engine stores the session's 20-s windows");
      if (!W.empty()) { const Win& w = W.back(); CHECK(std::fabs(w.rate20 - 20) < 1e-3 && w.Ms[0] == 200 && w.Mb[0] == 200, "window numbers"); }
      std::string ln = storeLine(s0, "X", W[0]); long long sid; std::string sy; Win w2;
      CHECK(parseStoreLine(ln, &sid, &sy, &w2) && sid == s0 && sy == "X" && w2.Ms[0] == W[0].Ms[0] && w2.bin == W[0].bin, "store line round trip");
      Engine e2; Store st2; e2.store = &st2; e2.sym = "X"; e2.noFlushSid = s0; for (auto& k : f.ticks) e2.add(k); e2.advanceTo(f.t); e2.flushSession();
      CHECK(st2.empty(), "a back-fill's partial oldest session is not stored"); }

    // REVIEW #2: a quiet stretch > 5 min behaves the same walked second by second (live) or jumped (back-fill)
    { Fx f; f.routine(250); f.t += 400; f.routine(230); for (int i = 0; i < 30; i++) { f.buy(10); f.sell(30); f.t++; }
      Engine a = run(f.ticks, B);                                   // back-fill: one jump per trade
      Engine l; l.setFixedBase(B); size_t i = 0;                    // live: the clock walks every second
      for (long long t = f.ticks.front().t; t <= f.ticks.back().t; t++) { while (i < f.ticks.size() && f.ticks[i].t == t) l.add(f.ticks[i++]); l.advanceTo(t - 3); }
      l.advanceTo(f.ticks.back().t);
      bool same = a.evs.size() == l.evs.size(); for (size_t k = 0; same && k < a.evs.size(); k++) same = a.evs[k].t == l.evs[k].t && a.evs[k].kind == l.evs[k].kind;
      CHECK(same && a.warmN == l.warmN, "quiet gap: live == back-fill (events and warm-up)");
      CHECK(count(a, "AW") == 1, "after a quiet gap the warm-up (200 s) passes before a watch can fire");
      const Ev* w = first(a, "AW"); CHECK(w && w->t - (T0 + 650) >= 200, "the watch comes >= 200 s after trading resumed");
      size_t h1 = a.hist.size(), h2 = l.hist.size(); CHECK(h1 == h2, "the same seconds are recorded"); }
    // REVIEW #9: a locked market (bid == ask) has no side
    CHECK(sideOf(1000, 1000, 1000) == SIDE_UNK, "locked quote: unknown side");
    // REVIEW: episode ids are the same in every rebuild (append-only records stay linked)
    { Fx f; f.routine(600); for (int i = 0; i < 30; i++) { f.buy(10); f.sell(30); f.t++; } f.q(1002, 1003); for (int i = 0; i < 40; i++) { f.buy(30); f.sell(5); f.t++; }
      Engine a = run(f.ticks, B); std::vector<Tick> later; for (auto& k : f.ticks) if (k.t >= T0 + 300) later.push_back(k); Engine b = run(later, B);
      const Ev* x = first(a, "AW"); std::vector<std::pair<long long, int>> A, Bv;
      for (auto& v : a.evs) if (v.kind == "AW") A.push_back({v.t, v.ep});
      for (auto& v : b.evs) if (v.kind == "AW") Bv.push_back({v.t, v.ep});
      CHECK(!A.empty() && A == Bv, "every AW has the same episode id when rebuilt from a later start");
      const Ev* r = first(a, "AR"); CHECK(r && x && r->ep == x->ep, "AR keeps its AW's id"); }
    // REVIEW #12: a candidate admitted while price is already beyond the adverse edge keeps the breach's age
    // REVIEW #7: IN thresholds frozen at setup (exercised by the IN tests; med5 / med20 now come from the setup)

    // a long closed market (weekend) is a gap, not 49 h of empty seconds with stale state
    { Fx f; f.routine(300); Engine e = run(f.ticks, B); size_t h = e.hist.size(); e.add(Tick{f.t + 49 * 3600, 1000, 1000, 1001, 5});
      CHECK(e.hist.size() < h + 400, "weekend jump is skipped"); CHECK(e.warmN < 5, "warm-up restarts after the closed market"); }

    printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
