// t_step.cpp -- lsStepTrend 1.1.0 (1.1.0: the support line ends when TESTED or at the NEXT STEP - checked against the rule written out candle by candle): the logic and the plugin on the mock host, on REAL candles (fx_mockup.txt = the 7 chart days of the
// mockup Rassul approved: ES Nov 10 day + evening, NQ / GC / CL Nov 9 and 10; the expected steps are written by the MOCKUP'S OWN code,
// ref.js, for both rules with the rotation on and off = 28 cases).
//   1 PARITY     stp::Engine gives exactly the mockup's steps (direction, level, hour, start candle, support end) in all 28 cases
//   2 NO REPAINT candle by candle: a step, once there, never changes; a fresh engine restarted after every candle agrees with the running one
//   1b BODY     the breakout hour is a green (up) / red (down) candle: the mockup's steps minus the hours that were not
//   3 FORMING    a step appears only when the hour is complete (the forming candle of the next hour), never inside the hour
//   4 LAYOUT     risers at the wave tops, flats to the next top, one live dashed level, thin support only after a same-way step
//   5 HOST       the plugin: same steps as the engine; drawing inside the pane, green / red only, widths 3 / 1; clipping; two charts
//                on one DLL; symbol change; a period that does not divide the hour; history change; unreadable candles; status file;
//                a host with no slot; chart removal
//   6 VERSIONS   STEPTREND_VERSION == stp::VERSION == the setVersion literal == the description
// Build (tests_step/): g++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation -g -fsanitize=address,undefined -I.. -Imock t_step.cpp -o t && ./t
#include "StepTrend.cpp"
#include <map>
#include <memory>
#include <set>
#include <unistd.h>
#include <sys/stat.h>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, std::string(m).c_str()); } } while (0)
typedef StepTrend Ind;
static void dispose(Ind* p) { p->destroy(); delete p; }
static Ind* make() { Ind* p = static_cast<Ind*>(CreateExtension()); p->setup(); return p; }

struct FB { long long t; double o, h, l, c; };
struct RS { int dir; double px; long long hour, t, o, xe; };
struct Ref { int d, rule, rot, nskip; std::vector<RS> s; };
struct Data { std::string mkt; double tick; std::vector<FB> b; };
static std::vector<Data> DS; static std::vector<Ref> RF;
static bool loadFx(const std::string& p) {
    std::ifstream f(p.c_str()); if (!f.is_open()) return false; std::string tag;
    while (f >> tag) {
        if (tag == "D") { int i, n; Data d; f >> i >> d.mkt >> d.tick >> n; for (int k = 0; k < n; ++k) { FB b; f >> b.t >> b.o >> b.h >> b.l >> b.c; d.b.push_back(b); } DS.push_back(d); }
        else if (tag == "R") { Ref r; int n; f >> r.d >> r.rule >> r.rot >> n >> r.nskip; for (int k = 0; k < n; ++k) { RS s; f >> s.dir >> s.px >> s.hour >> s.t >> s.o >> s.xe; r.s.push_back(s); } RF.push_back(r); }
        else return false;
    }
    return true;
}
static stp::Bar tk(const FB& b, double tick) { stp::Bar x; x.t = b.t; x.o = std::llround(b.o / tick); x.h = std::llround(b.h / tick); x.l = std::llround(b.l / tick); x.c = std::llround(b.c / tick); return x; }
static bool sameStep(const stp::Step& a, const stp::Step& b) { return a.dir == b.dir && a.px == b.px && a.hour == b.hour && a.tConf == b.tConf && a.oi == b.oi && a.ot == b.ot; }
static const long long BASE = 1794268800LL;      // 2026-11-10 00:00 as the chart's wall clock (the mock's clock is UTC); the fixture's times are seconds of the day

static int ES_STEPS = 0;
static void logicTests() {
    { const Data& d = DS[0]; stp::Engine E; for (const FB& b : d.b) E.addBar(tk(b, d.tick)); E.sealTo(d.b.back().t + 180); ES_STEPS = (int)E.S.size(); }
    int cases = 0, stepsSeen = 0, supSeen = 0, supEnded = 0, supCut = 0;
    for (const Ref& r : RF) { const Data& d = DS[(size_t)r.d];
        stp::Engine E; E.rule = r.rule; E.rot = r.rot != 0; E.body = false;      // the mockup had no candle-colour rule (added 00:11)
        for (const FB& b : d.b) E.addBar(tk(b, d.tick));
        E.sealTo(d.b.back().t + 180);                                          // the mockup counts an hour that ends with the last candle on the chart
        // the mockup needs the hour to END by the last candle + 3 min; the engine needs a later candle or the forming one - the same hours
        bool ok = E.S.size() == r.s.size() && (int)E.skipped.size() == r.nskip;
        for (size_t i = 0; ok && i < r.s.size(); ++i) { const stp::Step& q = E.S[i]; const RS& x = r.s[i];
            ok = q.dir == x.dir && q.px == std::llround(x.px / d.tick) && q.hour == x.hour && q.tConf == x.t && q.ot == x.o;
            if (ok && i + 1 < r.s.size() && r.s[i + 1].dir == x.dir) { ++supSeen;      // (1.1.0) the rule, candle by candle: tested after the next step's start, or the candle that closes the hour of the step after next
                long long end = -1; for (const FB& b : d.b) if (b.t > r.s[i + 1].o && (x.dir > 0 ? std::llround(b.l / d.tick) <= q.px : std::llround(b.h / d.tick) >= q.px)) { end = b.t; break; }
                if (i + 2 < r.s.size()) { if (end < 0 || end >= r.s[i + 2].t) end = r.s[i + 2].o; ++supCut; }   // not tested before that step's hour closed: the line stops where that step starts
                if (end >= 0) ++supEnded;
                ok = end < 0 ? q.sup == 1 : (q.sup == 2 && d.b[(size_t)q.supEnd].t == end); }
            if (ok && (i + 1 >= r.s.size() || r.s[i + 1].dir != x.dir)) ok = q.sup == 0; }
        ++cases; stepsSeen += (int)E.S.size();
        CHECK(ok, "PARITY " + d.mkt + " data " + std::to_string(r.d) + " rule " + std::to_string(r.rule) + " rot " + std::to_string(r.rot) + ": engine " + std::to_string(E.S.size()) + " steps / " + std::to_string(E.skipped.size()) + " skipped, mockup " + std::to_string(r.s.size()) + " / " + std::to_string(r.nskip));
    }
    CHECK(cases == 28 && stepsSeen > 100 && supSeen > 40 && supEnded > 30 && supCut > 20, "PARITY covered 28 cases (" + std::to_string(stepsSeen) + " steps, " + std::to_string(supSeen) + " support lines, " + std::to_string(supEnded) + " ended, " + std::to_string(supCut) + " with a later step)");
    // BODY (Rassul 00:11): with the rule on, every step's hour closed its own way, and the steps are the mockup's minus the hours that did not
    { int dropped = 0, kept = 0; bool sub = true, col = true;
      for (const Ref& r : RF) { if (r.rule != 0 || r.rot != 1) continue; const Data& d = DS[(size_t)r.d];
          stp::Engine E; for (const FB& b : d.b) E.addBar(tk(b, d.tick)); E.sealTo(d.b.back().t + 180);
          std::map<long long, std::pair<double, double> > oc; for (const FB& b : d.b) { const long long k = stp::floorDiv(b.t, 3600); if (!oc.count(k)) oc[k].first = b.o; oc[k].second = b.c; }
          for (const stp::Step& q : E.S) { const std::pair<double, double>& h = oc[q.hour / 3600]; if (q.dir > 0 ? !(h.second > h.first) : !(h.second < h.first)) col = false; bool in = false; for (const RS& x : r.s) if (x.hour == q.hour && x.dir == q.dir) in = true; if (!in) sub = false; ++kept; }
          for (const RS& x : r.s) { const std::pair<double, double>& h = oc[x.hour / 3600]; const bool own = x.dir > 0 ? h.second > h.first : h.second < h.first; bool in = false; for (const stp::Step& q : E.S) if (q.hour == x.hour) in = true; if (own != in) sub = false; if (!own) ++dropped; } }
      CHECK(col && sub && kept > 20, "BODY: every step's hour is a green (up) / red (down) candle; the mockup's steps minus " + std::to_string(dropped) + " hours that were not (" + std::to_string(kept) + " kept)");
      stp::Engine E; long long t = 10 * 3600; auto add = [&](long long o, long long h, long long l, long long c) { stp::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; E.addBar(b); t += 180; };
      for (int i = 0; i < 20; ++i) add(98, 100, 95, 98);                         // 10:00 high 100
      add(108, 109, 101, 102); for (int i = 0; i < 19; ++i) add(102, 104, 101, 103);   // 11:00 opens 108 (a gap up), closes 103: above the 10:00 high but a RED hour
      E.sealTo(t); CHECK(E.S.empty(), "BODY: an hour that closes above the prior high but below its own open is no step up");
      E.body = false; E.reset(); t = 10 * 3600; for (int i = 0; i < 20; ++i) add(98, 100, 95, 98); add(108, 109, 101, 102); for (int i = 0; i < 19; ++i) add(102, 104, 101, 103); E.sealTo(t); CHECK(E.S.size() == 1, "BODY off: the same hour is a step"); }
    // 2 NO REPAINT + restart after every candle
    for (size_t di = 0; di < DS.size(); ++di) { const Data& d = DS[di];
        stp::Engine E; std::vector<stp::Step> prev; bool stable = true, restart = true, supMono = true;
        for (size_t i = 0; i < d.b.size(); ++i) {
            E.addBar(tk(d.b[i], d.tick)); if (i + 1 < d.b.size()) E.sealTo(d.b[i + 1].t);
            if (E.S.size() < prev.size()) stable = false;
            for (size_t k = 0; stable && k < prev.size(); ++k) { if (!sameStep(prev[k], E.S[k])) stable = false; if (E.S[k].sup < prev[k].sup || (prev[k].sup == 2 && E.S[k].supEnd != prev[k].supEnd)) supMono = false; }
            prev = E.S;
            if (i % 7 == 0 || i + 1 == d.b.size()) { stp::Engine F; for (size_t k = 0; k <= i; ++k) F.addBar(tk(d.b[k], d.tick)); if (i + 1 < d.b.size()) F.sealTo(d.b[i + 1].t);
                if (F.S.size() != E.S.size()) restart = false; for (size_t k = 0; restart && k < F.S.size(); ++k) if (!sameStep(F.S[k], E.S[k]) || F.S[k].sup != E.S[k].sup || F.S[k].supEnd != E.S[k].supEnd) restart = false; }
        }
        CHECK(stable && supMono, "NO REPAINT " + d.mkt + " " + std::to_string(di) + ": a step never changes, a support line only gets its end once");
        CHECK(restart, "RESTART " + d.mkt + " " + std::to_string(di) + ": a restart after any candle gives the running engine's steps");
    }
    // 3 FORMING: hours 10 and 11; the 11:00 hour closes above the 10:00 high only with its last candle
    { stp::Engine E; long long t = 10 * 3600; auto add = [&](long long h, long long l, long long c) { stp::Bar b; b.t = t; b.o = c - 1; b.h = h; b.l = l; b.c = c; E.addBar(b); t += 180; };
      for (int i = 0; i < 20; ++i) add(100 + (i == 7 ? 5 : 0), 95, 98);         // 10:00 hour: high 105 at candle 7
      for (int i = 0; i < 19; ++i) add(104, 97, 103);                            // 11:00 hour, 19 candles: no close above 105
      E.sealTo(t); CHECK(E.S.empty(), "FORMING: nothing while the hour's last candle is still forming");
      add(110, 103, 108);                                                        // the last candle of the hour closes at 108
      CHECK(E.S.empty(), "FORMING: nothing until a later candle exists (the hour's last candle just closed, the next has not started)");
      E.sealTo(t); CHECK(E.S.size() == 1 && E.S[0].dir == 1 && E.S[0].px == 105 && E.S[0].oi == 7 && E.S[0].tConf == 12 * 3600, "FORMING: the step appears with the first candle of the next hour, at the prior hour's high, from the candle that made it");
      E.sealTo(t + 180); add(109, 107, 108); CHECK(E.S.size() == 1, "FORMING: decided once"); }
    // ties: the LAST candle that made the high is the start; a both-ways hour is no breakout; out-of-order candle refused; a gap hour
    { stp::Engine E; long long t = 5 * 3600; auto add = [&](long long h, long long l, long long c) { stp::Bar b; b.t = t; b.o = 44; b.h = h; b.l = l; b.c = c; bool r = E.addBar(b); t += 180; return r; };
      for (int i = 0; i < 20; ++i) add(i == 3 || i == 11 ? 50 : 48, 40, 45);
      t = 8 * 3600;                                                              // 06:00 and 07:00 have no candles (a session gap)
      for (int i = 0; i < 20; ++i) add(60, 44, 55);
      E.sealTo(9 * 3600); CHECK(E.S.size() == 1 && E.S[0].oi == 11 && E.S[0].px == 50, "TIE + GAP: the prior hour is the last hour that traded; the start is the last candle at the high");
      stp::Bar b; b.t = 8 * 3600; b.h = b.l = b.c = 1; CHECK(!E.addBar(b) && E.B.size() == 40, "ORDER: an out-of-order candle is refused"); }
    // rotation: a breakout against the hourly ZigZag is not counted; with rot off it is
    { for (int rot = 0; rot < 2; ++rot) { stp::Engine E; E.rot = rot != 0; long long t = 0; auto hour = [&](long long h, long long l, long long c) { for (int i = 0; i < 20; ++i) { stp::Bar b; b.t = t; b.o = c - 1; b.h = i == 10 ? h : std::min(h, c + 1); b.l = i == 5 ? l : std::max(l, c - 1); b.c = c; if (b.l > b.c) b.l = b.c; if (b.h < b.c) b.h = b.c; if (b.l > b.o) b.l = b.o; E.addBar(b); t += 180; } };
        hour(100, 90, 95); hour(110, 96, 108); hour(120, 105, 118); hour(119, 112, 114); hour(125, 113, 124); E.sealTo(t);
        // hours: up, up (breakout 1 above 100), up (breakout 2 above 110), an inside-low lower-high hour -> the ZigZag turns DOWN at the 120 high, then a close above 119 = breakout against the rotation
        if (rot) CHECK(E.S.size() == 2 && E.skipped.size() == 1 && E.skipped[0].px == 119 && E.skipped[0].rotAt < 0, "ROTATION on: the breakout against the hourly ZigZag is not counted (" + std::to_string(E.S.size()) + " / " + std::to_string(E.skipped.size()) + ")");
        else CHECK(E.S.size() == 3 && E.skipped.empty(), "ROTATION off: every breakout counts"); } }
    // 1.1.0 SUPPORT ENDS: (a) tested, (b) another step up, (c) a step down - and never runs on after any of them
    { auto mk = [](int variant) { stp::Engine E; E.rot = false; long long t = 0; auto hour = [&](long long o, long long h, long long l, long long c) { for (int i = 0; i < 20; ++i) { stp::Bar b; b.t = t; b.o = i == 0 ? o : c - 1; b.c = c; b.h = i == 10 ? h : c; b.l = i == 5 ? l : std::min(b.o, c); if (b.h < std::max(b.o, b.c)) b.h = std::max(b.o, b.c); E.addBar(b); t += 180; } };
          hour(95, 100, 94, 98);              // hour 0: high 100
          hour(99, 112, 99, 110);             // hour 1: closes above 100 -> step 1 UP at 100 (start: the candle that made 100)
          hour(109, 125, 108, 122);           // hour 2: closes above 112 -> step 2 UP at 112; step 1's level 100 is now support (running)
          if (variant == 0) hour(121, 124, 99, 123);       // (a) hour 3 dips to 99 <= 100: TESTED, no new step (123 < 125)
          if (variant == 1) hour(123, 140, 121, 138);      // (b) hour 3 closes above 125: ANOTHER STEP UP, never back at 100
          if (variant == 2) hour(121, 122, 104, 105);      // (c) hour 3 closes below 108: A STEP DOWN, never back at 100
          hour(variant == 2 ? 105 : 123, variant == 2 ? 106 : 124, variant == 2 ? 104 : 122, variant == 2 ? 105 : 123); E.sealTo(t); return E; };
      stp::Engine A = mk(0), Bq = mk(1), C = mk(2);
      CHECK(A.S.size() == 2 && A.S[0].sup == 2 && A.S[0].supEnd == 65 && A.S[1].sup == 0, "SUPPORT (a) tested: the line ends at the candle that trades back to the level (candle " + std::to_string(A.S.empty() ? -1 : A.S[0].supEnd) + ")");
      CHECK(Bq.S.size() == 3 && Bq.S[2].dir == 1 && Bq.S[0].sup == 2 && Bq.S[0].supEnd == 50 && Bq.S[2].oi == 50 && Bq.S[1].sup == 1, "SUPPORT (b) another step up: the older line ends where that step starts (candle " + std::to_string(Bq.S.empty() ? -1 : Bq.S[0].supEnd) + "); the newer level's line starts");
      CHECK(C.S.size() == 3 && C.S[2].dir == -1 && C.S[0].sup == 2 && C.S[0].supEnd == 45 && C.S[2].oi == 45 && C.S[1].sup == 0, "SUPPORT (c) a step down: the line ends where that step starts; no line for the level before the turn");
      std::vector<stp::Seg> g; std::vector<stp::Dot> dd; stp::layout(Bq, 99, g, dd); int open = 0; for (const stp::Seg& s : g) if (s.kind == 2 && s.i1 == 99 && s.p0 == 100) ++open;
      CHECK(open == 0, "SUPPORT: an ended line is not drawn on to the right edge"); }
    // 4 LAYOUT on the ES day
    { const Data& d = DS[0]; stp::Engine E; for (const FB& b : d.b) E.addBar(tk(b, d.tick)); E.sealTo(d.b.back().t + 180);
      std::vector<stp::Seg> g; std::vector<stp::Dot> dots; const int last = (int)d.b.size() - 1; stp::layout(E, last, g, dots);
      int live = 0, sup = 0, riser = 0, flat = 0; bool geo = true;
      for (const stp::Seg& s : g) { if (s.kind == 1) { ++live; geo = geo && s.i1 == last && s.p0 == s.p1 && s.i0 == E.S.back().oi && s.p0 == E.S.back().px; }
          else if (s.kind == 2) { ++sup; geo = geo && s.p0 == s.p1 && s.i1 > s.i0; }
          else if (s.p0 != s.p1) { ++riser; geo = geo && s.i0 == s.i1; } else { ++flat; geo = geo && s.i1 > s.i0; } }
      const int n = (int)E.S.size(); int same = 0; for (int i = 0; i + 1 < n; ++i) same += E.S[(size_t)i].dir == E.S[(size_t)i + 1].dir;
      CHECK(n == ES_STEPS && n >= 4 && live == 1 && riser == n - 1 && flat == n - 1 && sup == same && (int)dots.size() == n && geo, "LAYOUT ES: " + std::to_string(n) + " steps = n-1 risers + n-1 flats + 1 live level + " + std::to_string(sup) + " support lines, a dot per step");
      for (size_t i = 0; i + 1 < E.S.size(); ++i) { bool found = false; for (const stp::Seg& s : g) if (s.kind == 0 && s.p0 == s.p1 && s.i0 == E.S[i].oi && s.i1 == E.S[i + 1].oi && s.p0 == E.S[i + 1].px) found = true; CHECK(found, "LAYOUT: the flat runs from top " + std::to_string(i) + " to the next top at the next level"); } }
    CHECK(stp::periodOk(180) && stp::periodOk(60) && stp::periodOk(1800) && !stp::periodOk(420) && !stp::periodOk(3600) && !stp::periodOk(0) && !stp::periodOk(-1), "PERIOD: only periods that divide the hour");
    CHECK(stp::floorDiv(-1, 3600) == -1 && stp::floorDiv(3599, 3600) == 0 && stp::floorDiv(3600, 3600) == 1, "floorDiv");
}

// ---- host ----
static mock::Bar mbar(long long endT, const FB& b) { mock::Bar x; x.time = (unsigned long)endT; x.open = (float)b.o; x.high = (float)b.h; x.low = (float)b.l; x.close = (float)b.c; x.volume = 100; return x; }
static void load(mock::Host& h, const Data& d, size_t n, long long shift = 0) { h.bars.clear(); for (size_t i = 0; i < n && i < d.b.size(); ++i) h.bars.push_back(mbar(BASE + shift + d.b[i].t + 180, d.b[i])); }
static void scale(mock::Host& h, const Data& d) { double lo = 1e18, hi = -1e18; for (const FB& b : d.b) { lo = std::min(lo, b.l); hi = std::max(hi, b.h); } h.priceBase = (float)lo; h.pxPerPrice = (float)(900.0 / (hi - lo)); h.paneTop = 0; h.paneBottom = 950; h.paneLeft = 0; h.barW = 6; h.paneRight = 50 + (int)d.b.size() * 6 + 40; }
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }

static void hostTests() {
    char tmpl[] = "/tmp/stepXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mkdir(flex.c_str(), 0777); mkdir((flex + "\\StepTrend").c_str(), 0777);     // on Linux a path with backslashes is one file name: the two "folders" are prefixes
    sth::testWall() = 1000000;
    for (size_t di = 0; di < DS.size(); ++di) { const Data& d = DS[di];
        stp::Engine R; for (const FB& b : d.b) R.addBar(tk(b, d.tick));         // the engine on the same candles; the newest candle on a chart is the forming one
        stp::Engine R2; for (size_t i = 0; i + 1 < d.b.size(); ++i) R2.addBar(tk(d.b[i], d.tick)); R2.sealTo(d.b.back().t);
        mock::Host h; mock::current = &h; h.tick = (float)d.tick; h.seconds = 180; h.root = d.mkt == "GC" ? "QGC" : d.mkt == "CL" ? "QCL" : "@" + d.mkt; h.symbol = d.mkt + "Z6"; scale(h, d);
        Ind* p = make();
        // candle by candle: the plugin's steps = the engine's on the closed candles, and never change
        std::vector<stp::Step> prev; bool stable = true;
        for (size_t n = 2; n <= d.b.size(); ++n) { load(h, d, n); h.visFrom = 0; h.visTo = (int)n - 1; sth::testWall() += 180; p->calc(0);
            STState* S = p->testState(); if (!S) { stable = false; break; }
            if (S->E.S.size() < prev.size()) stable = false; for (size_t k = 0; stable && k < prev.size(); ++k) if (!sameStep(prev[k], S->E.S[k])) stable = false; prev = S->E.S;
            if (n % 40 == 0) { h.draws.clear(); p->draw(); } }
        STState* S = p->testState();
        bool same = S && S->E.S.size() == R2.S.size(); for (size_t k = 0; same && k < R2.S.size(); ++k) { stp::Step x = R2.S[k]; x.hour += BASE; x.tConf += BASE; x.ot += BASE; same = sameStep(S->E.S[k], x) && S->E.S[k].sup == x.sup && S->E.S[k].supEnd == x.supEnd; }
        CHECK(stable && same && S && S->resets == 0 && S->badBars == 0, "HOST " + d.mkt + " " + std::to_string(di) + ": candle by candle the plugin has the engine's steps (" + std::to_string(S ? S->E.S.size() : 0) + "), none ever changed, no re-feed");
        CHECK(S && S->mkt == d.mkt, "HOST market key from the chart's root " + h.root + " -> " + (S ? S->mkt : "?"));
        // draw
        h.draws.clear(); p->draw();
        std::vector<stp::Seg> g; std::vector<stp::Dot> dots; stp::layout(S->E, S->lastIdx - S->off, g, dots);
        int lines = 0, rings = 0, thick = 0, thin = 0, other = 0, outside = 0, badCol = 0, dashShort = 0;
        for (const mock::Draw& w : h.draws) {
            if (w.type == "ring") { ++rings; if (w.pen != sth::C_GREEN && w.pen != sth::C_RED) ++badCol; if (w.x < h.paneLeft || w.x > h.paneRight || w.y < h.paneTop - 5 || w.y > h.paneBottom + 5) ++outside; continue; }
            if (w.type != "line") { ++other; continue; }
            ++lines; if (w.penW == 3) { ++thick; if (w.pen != sth::C_GREEN && w.pen != sth::C_RED) ++badCol; if (w.pat != P_SOLID) ++badCol; if (w.y == w.b && std::abs(w.x - w.r) <= 6) ++dashShort; }
            else if (w.penW == 1) { ++thin; if (w.pen != sth::C_GREEN_D && w.pen != sth::C_RED_D) ++badCol; if (w.pat != P_DASH) ++badCol; }
            else ++badCol;
            if (w.x < h.paneLeft || w.x > h.paneRight || w.r < h.paneLeft || w.r > h.paneRight || w.y < h.paneTop - 5 || w.y > h.paneBottom + 5 || w.b < h.paneTop - 5 || w.b > h.paneBottom + 5) ++outside;
            if (w.x != w.r && w.y != w.b) ++badCol;                            // only flats and risers: nothing goes across
        }
        int nSup = 0, nSolid = 0; for (const stp::Seg& s : g) { nSup += s.kind == 2; nSolid += s.kind == 0; }
        const bool has = !S->E.S.empty();
        CHECK(other == 0 && outside == 0 && badCol == 0 && rings == (int)dots.size() && thin == nSup && thick >= nSolid + (has ? 5 : 0) && (!has || dashShort >= 5),
              "DRAW " + d.mkt + " " + std::to_string(di) + ": " + std::to_string(lines) + " lines (" + std::to_string(thick) + " thick, " + std::to_string(thin) + " thin dashed), " + std::to_string(rings) + " dots, no text, nothing outside the pane (" + std::to_string(outside) + "), colours / widths ok (" + std::to_string(badCol) + ")");
        // clipping: a window in the middle draws fewer lines, all inside the window's candles
        { h.draws.clear(); const int vs = (int)d.b.size() / 3, ve = 2 * (int)d.b.size() / 3; h.visFrom = vs; h.visTo = ve; p->draw();
          const int xl = 50 + vs * h.barW - h.barW / 2 - 1, xr = 50 + ve * h.barW + h.barW / 2 + 1; int out = 0; for (const mock::Draw& w : h.draws) { if (w.x < xl - 5 || w.x > xr + 5) ++out; if (w.type == "line" && (w.r < xl - 5 || w.r > xr + 5)) ++out; }
          CHECK(out == 0, "CLIP " + d.mkt + ": with candles " + std::to_string(vs) + "-" + std::to_string(ve) + " on screen nothing is drawn outside them (" + std::to_string(out) + ")"); }
        // status file
        { const std::string st = readAll(flex + "\\StepTrend\\" + d.mkt + "-180.status.txt");
          CHECK(st.find("STEPTREND,version " STEPTREND_VERSION ",market " + d.mkt) == 0 && st.find("steps " + std::to_string(S->E.S.size()) + ",") != std::string::npos && (S->E.S.empty() || st.find("\nSTEP,") != std::string::npos) && S->statusFails == 0,
                "STATUS " + d.mkt + ": the status file has the version, market and steps"); }
        // history change (a re-load with the times moved by a day): a re-feed, the same steps a day later
        { load(h, d, d.b.size(), 86400); h.visFrom = 0; h.visTo = (int)d.b.size() - 1; p->calc(0); STState* T = p->testState();
          bool ok = T && T->resets == 1 && T->E.S.size() == R2.S.size(); for (size_t k = 0; ok && k < R2.S.size(); ++k) ok = T->E.S[k].px == R2.S[k].px && T->E.S[k].oi == R2.S[k].oi && T->E.S[k].hour == R2.S[k].hour + BASE + 86400;
          CHECK(ok, "HISTORY " + d.mkt + ": a changed history is re-fed once and gives the same steps"); }
        dispose(p); CHECK(h.data == NULL, "REMOVE " + d.mkt + ": the chart's state is freed");
        (void)R;
    }
    // two charts on one DLL (ES and GC), fed in turns: each keeps its own steps
    { const Data& a = DS[0]; const Data& b = DS[3]; mock::Host ha, hb; ha.tick = (float)a.tick; hb.tick = (float)b.tick; ha.seconds = hb.seconds = 180; ha.root = "ES"; hb.root = "GC"; ha.symbol = "ESZ6"; hb.symbol = "GCZ6"; scale(ha, a); scale(hb, b);
      mock::current = &ha; Ind* p = make(); const size_t N = std::max(a.b.size(), b.b.size());
      for (size_t n = 2; n <= N; ++n) { mock::current = &ha; load(ha, a, std::min(n, a.b.size())); p->calc(0); mock::current = &hb; load(hb, b, std::min(n, b.b.size())); p->calc(0); }
      stp::Engine Ra, Rb; for (size_t i = 0; i + 1 < a.b.size(); ++i) Ra.addBar(tk(a.b[i], a.tick)); Ra.sealTo(a.b.back().t); for (size_t i = 0; i + 1 < b.b.size(); ++i) Rb.addBar(tk(b.b[i], b.tick)); Rb.sealTo(b.b.back().t);
      mock::current = &ha; STState* Sa = p->testState(); mock::current = &hb; STState* Sb = p->testState();
      bool ok = Sa && Sb && Sa != Sb && Sa->mkt == "ES" && Sb->mkt == "GC" && Sa->E.S.size() == Ra.S.size() && Sb->E.S.size() == Rb.S.size();
      for (size_t k = 0; ok && k < Ra.S.size(); ++k) ok = Sa->E.S[k].px == Ra.S[k].px && Sa->E.S[k].oi == Ra.S[k].oi;
      for (size_t k = 0; ok && k < Rb.S.size(); ++k) ok = Sb->E.S[k].px == Rb.S[k].px && Sb->E.S[k].oi == Rb.S[k].oi;
      CHECK(ok, "TWO CHARTS on one DLL: ES and GC keep their own steps");
      // symbol change on chart A: the state starts again for the new market
      mock::current = &ha; ha.root = "NQ"; ha.symbol = "NQZ6"; load(ha, DS[1], DS[1].b.size()); ha.tick = 0.25f; p->calc(0); Sa = p->testState();
      stp::Engine Rn; for (size_t i = 0; i + 1 < DS[1].b.size(); ++i) Rn.addBar(tk(DS[1].b[i], 0.25)); Rn.sealTo(DS[1].b.back().t);
      CHECK(Sa && Sa->mkt == "NQ" && Sa->E.S.size() == Rn.S.size() && Sa->E.B.size() == DS[1].b.size() - 1, "SYMBOL CHANGE: the chart starts again on the new market");
      mock::current = &hb; Sb = p->testState(); CHECK(Sb && Sb->mkt == "GC" && Sb->E.S.size() == Rb.S.size(), "SYMBOL CHANGE: the other chart is untouched");
      mock::current = &ha; p->destroy(); mock::current = &hb; p->destroy(); CHECK(ha.data == NULL && hb.data == NULL, "REMOVE both charts"); delete p; }
    // a period that does not divide the hour: nothing drawn, said in the status file; back to 3 minutes: works
    { const Data& d = DS[0]; mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 420; h.root = "ES"; h.symbol = "ESZ6"; scale(h, d); load(h, d, d.b.size()); h.visFrom = 0; h.visTo = (int)d.b.size() - 1;
      Ind* p = make(); sth::testWall() += 100; p->calc(0); h.draws.clear(); p->draw(); STState* S = p->testState();
      CHECK(S && S->E.S.empty() && h.draws.empty() && !S->note.empty() && readAll(flex + "\\StepTrend\\ES-420.status.txt").find("note the chart's period must divide the hour") != std::string::npos, "PERIOD 7 min: nothing drawn, the reason is in the status file");
      h.seconds = 180; p->calc(0); S = p->testState(); h.draws.clear(); p->draw(); CHECK(S && (int)S->E.S.size() == ES_STEPS && !h.draws.empty() && S->note.empty(), "PERIOD back to 3 min: the steps are drawn");
      // unreadable candles (NaN / 0) in the middle keep their place: the steps after them stay on the right candles
      const float keepH = h.bars[100].high; h.bars[100].high = std::nanf(""); h.bars[101].close = 0; h.bars[0].time += 1;   // (the first time changed = a re-feed)
      p->calc(0); S = p->testState(); CHECK(S && S->badBars == 2 && S->E.B.size() == d.b.size() - 1 && S->resets == 1, "UNREADABLE candles are counted and keep their place (" + std::to_string(S ? S->badBars : -1) + ")");
      h.draws.clear(); p->draw(); int out = 0; for (const mock::Draw& w : h.draws) if (w.x < 0 || w.x > h.paneRight) ++out; CHECK(out == 0, "UNREADABLE: still draws inside the pane"); h.bars[100].high = keepH;
      // no tick size: nothing, no crash
      h.tick = 0; p->calc(0); h.draws.clear(); p->draw(); CHECK(p->testState() != NULL, "NO TICK: no crash"); h.tick = 0.25f;
      // fewer than 2 candles, and draw with no visible range from the host
      h.bars.resize(1); p->calc(0); h.draws.clear(); p->draw(); CHECK(h.draws.empty(), "ONE CANDLE: nothing drawn");
      load(h, d, d.b.size()); h.visFrom = -1; p->calc(0); h.draws.clear(); p->draw(); CHECK(!h.draws.empty(), "NO VISIBLE RANGE from the host: the whole chart is used");
      dispose(p); }
    // a host with no per-chart slot: fails closed (nothing, no crash, no leak)
    { mock::Host h; mock::current = &h; h.noSlot = true; h.tick = 0.25f; h.seconds = 180; load(h, DS[0], 100); Ind* p = make(); p->calc(0); p->calc(0); h.draws.clear(); p->draw(); CHECK(h.draws.empty() && p->testState() == NULL, "NO SLOT host: nothing, no crash"); dispose(p); }
    // long history: more than MAX_BARS + SLACK candles -> one re-feed of the last MAX_BARS, indices still on the chart
    { mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.root = "ES"; h.symbol = "ESZ6"; h.barW = 1; h.paneRight = 30000; h.priceBase = 7000; h.pxPerPrice = 1; h.paneBottom = 2000;
      const int N = stp::MAX_BARS + stp::SLACK + 600; long long t = BASE; unsigned seed = 7; double px = 7500;
      Ind* p = make(); int grew = 0;
      for (int i = 0; i < N; ++i) { seed = seed * 1103515245u + 12345u; const double mv = ((int)((seed >> 16) % 41) - 20) * 0.25; FB b; b.t = 0; b.o = px; b.c = px + mv; b.h = std::max(b.o, b.c) + 0.5; b.l = std::min(b.o, b.c) - 0.5; px = b.c; t += 180; h.bars.push_back(mbar(t, b));
          if (i == 3000 || i == stp::MAX_BARS + stp::SLACK + 100 || i == N - 1) { p->calc(0); ++grew; } else if (i > 3000 && i % 50 == 0) p->calc(0); }
      STState* S = p->testState(); h.visFrom = N - 400; h.visTo = N - 1; h.draws.clear(); p->draw();
      int out = 0; for (const mock::Draw& w : h.draws) if (w.x < 50 + (N - 400) - 3 || w.x > 50 + N + 3) ++out;
      CHECK(S && S->resets >= 1 && (int)S->E.B.size() <= stp::MAX_BARS + stp::SLACK + 1 && S->off + (int)S->E.B.size() == N - 1 && !S->E.S.empty() && !h.draws.empty() && out == 0 && grew == 3,
            "LONG HISTORY: " + std::to_string(N) + " candles -> " + std::to_string(S ? S->resets : -1) + " re-feed(s), " + std::to_string(S ? S->E.B.size() : 0) + " candles kept, " + std::to_string(S ? S->E.S.size() : 0) + " steps, drawing on the right candles");
      dispose(p); }
}

static void versionTests(const std::string& dir) {
    const std::string src = readAll(dir + "/../StepTrend.cpp");
    CHECK(std::string(STEPTREND_VERSION) == stp::VERSION, "VERSION macro == stp::VERSION");
    CHECK(src.find(std::string("    p->setVersion(\"") + STEPTREND_VERSION + "\");\n") != std::string::npos, "VERSION: the setVersion literal, on its own line");
    CHECK(src.find(std::string("LRA Step Trend v") + STEPTREND_VERSION) != std::string::npos && src.find(std::string("\"Step Trend\"  ") + STEPTREND_VERSION) != std::string::npos, "VERSION: description and header comment");
    CHECK(src.find(" small") == std::string::npos && src.find(" near ") == std::string::npos && src.find(" far ") == std::string::npos, "no Windows-macro identifiers");
    const std::string es = "ES", xx = "XX"; const stpp::Mkt& M = stpp::forMarket(es); CHECK(M.rule == 0 && M.rot == 1 && M.body == 1 && stpp::forMarket(xx).rule == 0, "PARAMS: his rule (close beyond the prior hour, rotation required) for every market");
}

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : ".";
    if (!loadFx(dir + "/fx_mockup.txt") || DS.size() != 7 || RF.size() != 28) { std::printf("FAIL fixture\n"); return 1; }
    logicTests(); hostTests(); versionTests(dir);
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
