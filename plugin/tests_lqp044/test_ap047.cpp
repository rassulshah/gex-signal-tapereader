// test_ap047.cpp -- lsLiquidityProfile 0.47.0, AirPocketLogic 1.0.0 (MBO roadmap Middle Ahead #1, Rassul 14:25 CT option A).
// The AIR POCKET: after a 5-candle turn, the resting size between the close and T1 (close +/- 1 ATR) fell to <= 0.67x of a minute
// before, by PULLS (not trades), close-back >= 0.5 -> a green (up) / red (down) band close..T1 with an "AP?" badge in the column while
// live; ends at T1 / size back / 15 min; decided once at the turn candle's close, never re-decided (a restart reads the record back).
// Part A: the rule on its own (fires / not: traded through, already thin, nothing resting, shallow close-back, no ATR, not a turn, no
// history; the down mirror; the ends; the record round trip; the band rectangle). Part B: the plugin on the mock host (end to end
// from depth + MBO + bars: fires, band inside the column, green, AP? badge, status, record; fixed while live; T1 ends it once; a
// restart restores without re-deciding; the setting off draws nothing and decides nothing; the settings row reads / writes its file).
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Itests_lqp044 -I. tests_lqp044/test_ap047.cpp -o tap047 && ./tap047
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <unistd.h>
#define private public                 // the test reads the chart's state (LQState) and calls the settings window's file path
#include "LiquidityProfile.cpp"
#undef private
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)

static std::vector<apl::Candle> lowTurn(double cb) {   // 5 candles low 100, then a candle with low 99 and the given close-back
    std::vector<apl::Candle> c;
    for (int i = 0; i < 5; ++i) { apl::Candle x; x.o = 101; x.h = 102; x.l = 100; x.c = 101; c.push_back(x); }
    apl::Candle t; t.o = 100.5; t.h = 101; t.l = 99; t.c = 99 + 2 * cb; c.push_back(t); return c;
}
static std::map<apl::Tick, double> lv(apl::Tick lo, apl::Tick hi, double each) { std::map<apl::Tick, double> m; for (apl::Tick p = lo; p <= hi; ++p) m[p] = each; return m; }

static void partA() {
    const double tick = 0.25, atr = 2.5;                 // T1 = close + 10 ticks
    const long long T = 1000000000LL;
    const std::map<apl::Tick, double> none;
    // 1) fires: offers ahead 10 / level a minute ago, 2 now, 80 lots pulled, nothing traded; close-back 0.8
    {
        const std::vector<apl::Candle> c = lowTurn(0.8); const apl::Tick ct = (apl::Tick)std::llround(c.back().c / tick);
        apl::Tracker tr; tr.snap(T - 60000, none, lv(ct + 1, ct + 12, 10), ct, 40); tr.snap(T, none, lv(ct + 1, ct + 12, 2), ct, 40);
        for (int i = 0; i < 8; ++i) tr.flow(T - 50000 + i * 1000, apl::ASK, ct + 1 + i, 10, false);
        const apl::Case k = apl::decide(c, T, tick, atr, tr);
        CHECK(k.fired && k.why == "air pocket" && k.dir == 1, "A1 fires: a low turn, offers ahead pulled to 0.2x, close-back 0.8");
        CHECK(k.t1Tk == k.closeTk + 10 && k.side() == apl::ASK && k.lo() == k.closeTk + 1 && k.hi() == k.t1Tk, "A1 T1 = close + 1 ATR (10 ticks); the path is the offers close+1..T1");
        CHECK(std::fabs(k.ratio - 0.2) < 1e-9 && k.before == 100 && k.now == 20 && k.pulled == 80 && k.filled == 0, "A1 ratio 0.2 = 20 / 100 (only close+1..T1 counted)");
        CHECK(k.live(T + 1000) && !k.live(T + 15 * 60000) && !k.live(T - 1), "A1 live from the decision for 15 min, never before it");
    }
    // 2) traded through: the same thinning, but by fills
    {
        const std::vector<apl::Candle> c = lowTurn(0.8); const apl::Tick ct = (apl::Tick)std::llround(c.back().c / tick);
        apl::Tracker tr; tr.snap(T - 60000, none, lv(ct + 1, ct + 12, 10), ct, 40); tr.snap(T, none, lv(ct + 1, ct + 12, 2), ct, 40);
        for (int i = 0; i < 8; ++i) tr.flow(T - 50000 + i * 1000, apl::ASK, ct + 1 + i, 10, true);
        tr.flow(T - 40000, apl::ASK, ct + 2, 5, false);
        const apl::Case k = apl::decide(c, T, tick, atr, tr);
        CHECK(!k.fired && k.why == "opened by trades, not pulls" && k.filled == 80 && k.pulled == 5, "A2 traded through: not an air pocket");
    }
    // 3) already thin (no change in the minute) / nothing resting a minute ago
    {
        const std::vector<apl::Candle> c = lowTurn(0.8); const apl::Tick ct = (apl::Tick)std::llround(c.back().c / tick);
        apl::Tracker tr; tr.snap(T - 60000, none, lv(ct + 1, ct + 12, 2), ct, 40); tr.snap(T, none, lv(ct + 1, ct + 12, 2), ct, 40);
        tr.flow(T - 30000, apl::ASK, ct + 3, 50, false);
        const apl::Case k = apl::decide(c, T, tick, atr, tr);
        CHECK(!k.fired && k.why == "path did not open" && std::fabs(k.ratio - 1.0) < 1e-9, "A3 already thin (1.0x): static thinness is not the signal");
        apl::Tracker t2; t2.snap(T - 60000, none, none, ct, 40); t2.snap(T, none, none, ct, 40);
        CHECK(apl::decide(c, T, tick, atr, t2).why == "nothing resting ahead a minute ago", "A3 nothing resting a minute ago: no ratio, not fired");
    }
    // 4) shallow close-back (0.3)
    {
        const std::vector<apl::Candle> c = lowTurn(0.3); const apl::Tick ct = (apl::Tick)std::llround(c.back().c / tick);
        apl::Tracker tr; tr.snap(T - 60000, none, lv(ct + 1, ct + 12, 10), ct, 40); tr.snap(T, none, lv(ct + 1, ct + 12, 2), ct, 40);
        tr.flow(T - 30000, apl::ASK, ct + 3, 80, false);
        const apl::Case k = apl::decide(c, T, tick, atr, tr);
        CHECK(!k.fired && k.why == "shallow close-back" && std::fabs(k.cb - 0.3) < 1e-9, "A4 close-back 0.3 < 0.5: not fired");
    }
    // 5) no T1 (no ATR / tick), not a turn, no book history for the minute
    {
        const std::vector<apl::Candle> c = lowTurn(0.8); apl::Tracker tr;
        CHECK(apl::decide(c, T, tick, 0, tr).why == "no tick / ATR", "A5 no ATR: no T1, not fired");
        CHECK(apl::decide(c, T, 0, atr, tr).why == "no tick / ATR", "A5 no tick: not fired");
        std::vector<apl::Candle> flat = c; flat.back().l = 100.5; flat.back().h = 101.5;
        CHECK(apl::decide(flat, T, tick, atr, tr).why == "not a local extreme" && apl::localExtreme(flat) == 0, "A5 inside the last 5 candles: not a turn");
        std::vector<apl::Candle> both = c; both.back().h = 103;   // a new high AND a new low (outside bar): neither
        CHECK(apl::localExtreme(both) == 0, "A5 an outside bar is not a turn either way");
        tr.snap(T, none, lv(1, 9999, 1), 400, 40);               // only the snapshot at the decision, none a minute before
        CHECK(apl::decide(c, T, tick, atr, tr).why == "no book history for the minute", "A5 no snapshot a minute ago: not decided as fired");
    }
    // 6) the down mirror: a high turn, the bids below pulled -> red, T1 below
    {
        std::vector<apl::Candle> c;
        for (int i = 0; i < 5; ++i) { apl::Candle x; x.o = 101; x.h = 102; x.l = 100; x.c = 101; c.push_back(x); }
        apl::Candle t; t.o = 102; t.h = 103; t.l = 101; t.c = 101.25; c.push_back(t);           // close-back (103 - 101.25) / 2 = 0.875
        const apl::Tick ct = (apl::Tick)std::llround(t.c / tick);
        apl::Tracker tr; tr.snap(T - 60000, lv(ct - 12, ct - 1, 10), none, ct, 40); tr.snap(T, lv(ct - 12, ct - 1, 3), none, ct, 40);
        tr.flow(T - 20000, apl::BID, ct - 4, 70, false);
        const apl::Case k = apl::decide(c, T, tick, atr, tr);
        CHECK(k.fired && k.dir == -1 && k.side() == apl::BID && k.t1Tk == ct - 10 && k.lo() == ct - 10 && k.hi() == ct - 1, "A6 down: a high turn, bids close-1..T1 pulled -> fired, T1 below");
        tr.flow(T - 10000, apl::ASK, ct - 4, 500, false);                                          // pulls on the wrong side never count
        CHECK(apl::decide(c, T, tick, atr, tr).pulled == 70, "A6 only the side standing in the path counts");
    }
    // 7) the ends: T1 / size back (after 5 s) / 15 min, each once, the end time fixed
    {
        const std::vector<apl::Candle> c = lowTurn(0.8); const apl::Tick ct = (apl::Tick)std::llround(c.back().c / tick);
        apl::Tracker tr; tr.snap(T - 60000, none, lv(ct + 1, ct + 12, 10), ct, 40); tr.snap(T, none, lv(ct + 1, ct + 12, 2), ct, 40);
        tr.flow(T - 30000, apl::ASK, ct + 3, 80, false);
        apl::Case a = apl::decide(c, T, tick, atr, tr), b = a, d = a;
        CHECK(!apl::step(a, T + 2000, ct + 9, true, tr), "A7 short of T1: still live");
        CHECK(apl::step(a, T + 3000, ct + 10, true, tr) && a.endWhy == "T1" && a.endMs == T + 3000 && !a.live(T + 3001), "A7 price reaches T1: ends (T1)");
        CHECK(!apl::step(a, T + 4000, ct, true, tr) && a.endMs == T + 3000, "A7 ends once: the end time never moves");
        tr.snap(T + 2000, none, lv(ct + 1, ct + 12, 9), ct, 40);                                   // 90 = 0.9x: the size is back
        CHECK(!apl::step(b, T + 2500, ct + 2, true, tr), "A7 size back inside the first 5 s: not yet (one snapshot is noise)");
        tr.snap(T + 6000, none, lv(ct + 1, ct + 12, 9), ct, 40);
        CHECK(apl::step(b, T + 6000, ct + 2, true, tr) && b.endWhy == "size back", "A7 the offers came back above 0.67x: ends (size back)");
        apl::Tracker quiet = tr; quiet.snaps.clear();
        CHECK(!apl::step(d, T + 14 * 60000, ct + 2, true, quiet) && apl::step(d, T + 15 * 60000 + 5, ct + 2, true, quiet) && d.endWhy == "15 min" && d.endMs == T + 15 * 60000,
              "A7 15 min: ends at exactly turn + 15 min");
        apl::Case n = apl::decide(lowTurn(0.3), T, tick, atr, tr);
        CHECK(!apl::step(n, T + 16 * 60000, 0, false, tr) && n.endMs < 0, "A7 a case that did not fire never 'ends' (no APE line)");
    }
    // 8) the record round trip (AP / APE)
    {
        const std::vector<apl::Candle> c = lowTurn(0.8); const apl::Tick ct = (apl::Tick)std::llround(c.back().c / tick);
        apl::Tracker tr; tr.snap(T - 60000, none, lv(ct + 1, ct + 12, 10), ct, 40); tr.snap(T, none, lv(ct + 1, ct + 12, 2), ct, 40);
        tr.flow(T - 30000, apl::ASK, ct + 3, 80, false);
        apl::Case k = apl::decide(c, T, tick, atr, tr), r;
        const std::string L = apl::line(k);
        CHECK(L.compare(0, 3, "AP|") == 0 && L.find("|air pocket|1.0.0") != std::string::npos, "A8 AP line carries the reason and the version");
        CHECK(apl::parse(L, r) && r.turnMs == k.turnMs && r.dir == 1 && r.closeTk == k.closeTk && r.t1Tk == k.t1Tk && r.fired && std::fabs(r.ratio - 0.2) < 1e-3 && r.endMs < 0,
              "A8 AP line read back = the same case (live, no end)");
        apl::step(k, T + 3000, ct + 10, true, tr); long long tm = 0, em = 0; std::string w;
        CHECK(apl::parseEnd(apl::endLine(k), tm, em, w) && tm == T && em == T + 3000 && w == "T1", "A8 APE line read back");
        CHECK(!apl::parse("AP|x|1", r) && !apl::parse(apl::endLine(k), r) && !apl::parseEnd(L, tm, em, w), "A8 garbage / the other kind is refused");
    }
    // 9) the band rectangle: clipped inside [top, bottom], either order, nothing when out of view
    {
        int yt = 0, yb = 0;
        CHECK(apl::bandRect(500, 300, 100, 900, yt, yb) && yt == 300 && yb == 500, "A9 close below T1 on screen (up band): 300..500");
        CHECK(apl::bandRect(50, 300, 100, 900, yt, yb) && yt == 100 && yb == 300, "A9 clipped to the top");
        CHECK(!apl::bandRect(950, 990, 100, 900, yt, yb), "A9 out of view: nothing");
        CHECK(apl::bandRect(400, 400, 100, 900, yt, yb) && yb - yt == 2, "A9 at least 2 px");
    }
}

// ---------------------------------------------------------------- Part B: the plugin on the mock host
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static int countOf(const std::string& s, const std::string& w) { int n = 0; for (size_t i = s.find(w); i != std::string::npos; i = s.find(w, i + 1)) ++n; return n; }

struct Rig {
    mock::Host h; std::string tmp; unsigned long T = 0; int id = 1000;
    LiquidityProfile* p = nullptr;
    void bars(int n, unsigned long last) {   // n quiet 3-min bars ending at `last` (the last = forming)
        h.bars.clear();
        for (int i = 0; i < n; ++i) { mock::Bar b; b.time = last - (n - 1 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.05f; b.rows.push_back(mock::Price(80.05f, 50, 40, 90)); h.bars.push_back(b); }
    }
    void depth(double askEach) {             // bids 80.00 down, offers 80.01 up; the offers 80.01..80.20 hold askEach each
        h.depth.clear();
        for (int i = 0; i < 80; ++i) { DEPTH_LEVEL L; L.bid = (float)(80.00 - i * 0.01); L.ask = (float)(80.01 + i * 0.01); L.bidsize = 10; L.asksize = (unsigned long)(i < 20 ? askEach : 10); h.depth.push_back(L); }
    }
    void step(unsigned long now) { h.now = now; p->calc(0); usleep(2000); }
    LQState& S() { return *p->slot_.get(p, false); }
    void restart() { p->destroy(); p->slot_.release(p); p->flushPark(); delete p; h.data = nullptr; p = static_cast<LiquidityProfile*>(CreateExtension()); }   // IRT restarted: a new chart state
};

static void partB(const char* tmp) {
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL);
    Rig R; mock::current = &R.h;
    R.h.tick = 0.01f; R.h.seconds = 180; R.h.symbol = "CLEX26"; R.h.root = "CLEX26";
    R.h.priceBase = 79.5f; R.h.pxPer = 1000.0f; R.h.barW = 10; R.h.barX0 = 50; R.h.paneBottom = 1000; R.h.paneRight = 2000; R.h.scaleLeft = 1980;
    const unsigned long T = day + 9 * 3600; R.T = T;               // the turn candle ENDS at T
    R.bars(25, T);                                                 // bar 24 (ends T) is forming
    R.depth(30);
    R.p = static_cast<LiquidityProfile*>(CreateExtension());
    for (unsigned long s = T - 100; s < T - 70; ++s) R.step(s);    // history seeded, the book read
    // the turn candle: a new low 79.90 (the 5 before: 79.98), closing 79.97 = close-back 0.7
    mock::Bar& tb = R.h.bars.back(); tb.low = 79.90f; tb.high = 80.00f; tb.open = 79.99f; tb.close = 79.97f;
    // a minute before the close: offers 80.01..80.20 rest 30 each; resting orders the feed knows (NEW), then PULLED (DEL)
    for (int k = 0; k < 10; ++k) mock::feed.push(0, 0, 80.01f + 0.01f * (float)k, 25, (RTDATE)(T - 69), R.id + k);
    for (unsigned long s = T - 70; s < T - 30; ++s) R.step(s);
    for (int k = 0; k < 10; ++k) mock::feed.push(2, 0, 80.01f + 0.01f * (float)k, 25, (RTDATE)(T - 29), R.id + k);
    R.depth(5);                                                    // the offers between the close and T1 fell 30 -> 5
    for (unsigned long s = T - 30; s <= T; ++s) R.step(s);
    // the bar closes: a new forming bar (price back at the close)
    { mock::Bar b; b.time = T + 180; b.open = 79.97f; b.high = 79.98f; b.low = 79.96f; b.close = 79.97f; b.rows.push_back(mock::Price(79.97f, 5, 5, 10)); R.h.bars.push_back(b); }
    R.step(T + 1); R.step(T + 2);
    LQState& S = R.S();
    std::printf("  ATR %.4f tick %.4f lastPx %lld cases %zu fired %lld not %lld why '%s' snaps %zu flows %zu\n", S.atr, S.tick, (long long)S.e.lastPx, S.apCases.size(), S.apFired, S.apNot, S.apWhy.c_str(), S.ap.snaps.size(), S.ap.flows.size());
    CHECK(S.apCases.size() == 1 && S.apFired == 1, "B1 the turn candle's close decided once, and it fired (end to end: depth + MBO pulls + bars)");
    const apl::Case k = S.apCases.empty() ? apl::Case() : S.apCases[0];
    CHECK(k.dir == 1 && k.closeTk == 7997 && k.t1Tk > k.closeTk && k.pulled > k.filled && k.ratio <= 0.67, "B1 up case: close 79.97, T1 above, pulled > filled, ratio <= 0.67");
    CHECK(k.turnMs == R.p->ctSec((RTDATE)T) * 1000LL, "B1 decided at the turn candle's END (its close)");
    // the draw: one band inside the column, green outline on a dark tint, the AP? badge green; no numbers
    R.h.draws.clear(); R.p->draw();
    const int colL = R.h.paneLeft, colR = R.h.paneLeft + S.width;
    int bands = 0, badges = 0, nums = 0; mock::Draw band("none", 0, 0), badge("none", 0, 0);
    for (auto& d : R.h.draws) {
        if (d.type == "box" && d.border == C_BUY && d.pat == PAT_HOLLOW && d.r - d.l == S.width - 2) { ++bands; band = d; }
        if (d.type == "text" && d.text == "AP?") { ++badges; badge = d; }
        if (d.type == "text" && d.text.find("0.") != std::string::npos) ++nums;
    }
    std::printf("  column %d..%d band %d..%d y %d..%d badge '%s' colour %06X\n", colL, colR, band.l, band.r, band.t, band.b, badge.text.c_str(), (unsigned)badge.color);
    if (const char* dump = std::getenv("AP_DUMP")) {                // the render check: the draws + the candles -> a PNG (tools outside the test)
        std::ofstream o(dump);
        for (size_t i = 0; i < R.h.bars.size(); ++i) { LiquidityProfile::PNT ph, pl, po, pc; ph.set((int)i, R.h.bars[i].high); pl.set((int)i, R.h.bars[i].low); po.set((int)i, R.h.bars[i].open); pc.set((int)i, R.h.bars[i].close);
            o << "bar|" << ph.h << "|" << ph.v << "|" << pl.v << "|" << po.v << "|" << pc.v << "\n"; }
        for (auto& d : R.h.draws) if (d.type == "box" || d.type == "text") o << d.type << "|" << d.l << "|" << d.t << "|" << d.r << "|" << d.b << "|" << (unsigned)d.color << "|" << (unsigned)d.border << "|" << d.pat << "|" << (d.centred ? 1 : 0) << "|" << d.text << "\n";
    }
    CHECK(bands == 1 && badges == 1, "B2 one band + one AP? badge while live");
    CHECK(band.l >= colL && band.r <= colR && badge.l >= colL && badge.r <= colR, "B2 band and badge inside the column");
    CHECK(badge.color == C_BUY, "B2 path up open at a low: green");
    { LiquidityProfile::PNT a, b; a.set(0, 79.97f); b.set(0, (float)((double)k.t1Tk * 0.01)); CHECK(band.t == std::min((int)a.v, (int)b.v) && band.b == std::max((int)a.v, (int)b.v), "B2 the band spans close .. T1 exactly"); }
    bool tint = false; for (auto& d : R.h.draws) if (d.type == "box" && d.color == (COLOR)0x000F2A1A && d.l == band.l) tint = true;
    CHECK(tint, "B2 a dark green tint under the outline");
    CHECK(nums == 0, "B2 no numbers on the chart (the ratio is in the status)");
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\";
    usleep(1100000); R.step(T + 2);                                 // the status is rebuilt at most once a second
    const std::string st = slurp(flex + "LiquidityProfile.status-CL-180.txt");
    CHECK(st.find("AIR_POCKET,1.0.0,on,") != std::string::npos && st.find(",fired 1,") != std::string::npos && st.find(",live 1 (drawn 1)") != std::string::npos && st.find("last ratio 0.") != std::string::npos && st.find("last why air pocket") != std::string::npos, "B3 status line: on, fired 1, the reason, the ratio");
    // the record
    const std::string rec = R.p->apPath(S);
    { size_t a = st.find("AIR_POCKET"); if (a != std::string::npos) std::printf("  %s\n", st.substr(a, st.find('\n', a) - a).c_str()); }
    std::string body = slurp(rec);
    CHECK(countOf(body, "\nAP|") == 1 && body.compare(0, 4, "kind") == 0 && body.find("|air pocket|1.0.0") != std::string::npos, "B4 the session record has the case (AP line)");
    // fixed while live: more calcs / draws, same case, same band
    for (unsigned long s = T + 3; s < T + 20; ++s) R.step(s);
    R.h.draws.clear(); R.p->draw(); int same = 0;
    for (auto& d : R.h.draws) if (d.type == "box" && d.border == C_BUY && d.pat == PAT_HOLLOW && d.l == band.l && d.t == band.t && d.b == band.b) ++same;
    CHECK(S.apCases.size() == 1 && same == 1, "B5 no repaint: the same band, the case decided once");
    // T1: price reaches it -> the band is gone, one APE line "T1"
    R.h.bars.back().close = (float)((double)k.t1Tk * 0.01); R.h.bars.back().high = R.h.bars.back().close;
    R.step(T + 21); R.step(T + 22);
    std::printf("  after T1: lastPx %lld t1 %lld endWhy '%s'\n", (long long)S.e.lastPx, (long long)k.t1Tk, S.apCases[0].endWhy.c_str());
    R.h.draws.clear(); R.p->draw(); int left = 0; for (auto& d : R.h.draws) if (d.type == "text" && d.text == "AP?") ++left;
    body = slurp(rec);
    CHECK(S.apCases[0].endWhy == "T1" && left == 0 && countOf(body, "APE|") == 1 && body.find("|T1") != std::string::npos, "B6 price at T1: ended (APE T1), no band");
    R.h.bars.back().close = 79.97f; R.step(T + 23); R.step(T + 24);
    R.h.draws.clear(); R.p->draw(); left = 0; for (auto& d : R.h.draws) if (d.type == "text" && d.text == "AP?") ++left;
    CHECK(left == 0 && countOf(slurp(rec), "APE|") == 1, "B6 price back: an ended band never comes back");
    // restart: a new chart instance reads the record back, never re-decides
    R.restart();
    for (unsigned long s = T + 30; s < T + 40; ++s) R.step(s);
    LQState& S2 = R.S();
    std::printf("  restart: restored %lld fired %lld not %lld cases %zu session %lld apSession %lld end '%s' why '%s'\n", S2.apRestored, S2.apFired, S2.apNot, S2.apCases.size(), S2.session, S2.apSession, S2.apCases.empty() ? "" : S2.apCases[0].endWhy.c_str(), S2.apWhy.c_str());
    CHECK(S2.apRestored == 1 && S2.apFired == 0 && S2.apNot == 0 && S2.apCases.size() == 1 && S2.apCases[0].endWhy == "T1", "B7 restart: the case restored with its end, nothing re-decided");
    CHECK(countOf(slurp(rec), "\nAP|") == 1 && countOf(slurp(rec), "\nAPE|") == 1, "B7 restart: no second AP line");
    // a restart while a band is live keeps it (restored live, same geometry): write a live case for a later turn
    {
        apl::Case z = k; z.turnMs = R.p->ctSec((RTDATE)(T + 35)) * 1000LL; z.endMs = -1; z.endWhy.clear();
        std::ofstream o(rec.c_str(), std::ios::app); o << apl::line(z) << "\n"; o.close();
        R.restart();
        for (unsigned long s = T + 40; s < T + 45; ++s) R.step(s);
        R.h.draws.clear(); R.p->draw(); int live = 0; for (auto& d : R.h.draws) if (d.type == "text" && d.text == "AP?") ++live;
        CHECK(R.S().apRestored == 2 && live == 1, "B7 a live band survives a restart (restored from its AP line, drawn)");
    }
    // the setting off: no band, nothing decided
    std::remove(rec.c_str());
    { std::ofstream o((flex + "LiquidityProfile.airpocket-CL.txt").c_str()); o << "0\n"; }
    R.restart();
    for (unsigned long s = T + 50; s < T + 55; ++s) R.step(s);
    { mock::Bar b; b.time = T + 360; b.open = 79.97f; b.high = 79.98f; b.low = 79.80f; b.close = 79.95f; R.h.bars.push_back(b); }
    { mock::Bar b; b.time = T + 540; b.open = 79.95f; b.high = 79.96f; b.low = 79.94f; b.close = 79.95f; R.h.bars.push_back(b); }
    for (unsigned long s = T + 361; s < T + 366; ++s) R.step(s);
    R.h.draws.clear(); R.p->draw(); int off = 0; for (auto& d : R.h.draws) if (d.type == "text" && d.text == "AP?") ++off;
    const std::string st2 = slurp(flex + "LiquidityProfile.status-CL-180.txt");
    CHECK(!R.S().apOn && off == 0 && R.S().apFired == 0 && R.S().apNot == 0 && st2.find("AIR_POCKET,1.0.0,off,settings file: off") != std::string::npos, "B8 setting off: no band, nothing decided, status says off");
    // the settings row: the window shows the file's value; OK writes it
    {
        const ss::Values V = R.p->fileValues();
        CHECK(V.v[lpset::K_AIR_POCKET] == 0, "B9 the window shows Air pocket off (from the file)");
        ss::Values W = V; W.v[lpset::K_AIR_POCKET] = 1; R.p->saveAirPocket(W);
        CHECK(slurp(flex + "LiquidityProfile.airpocket-CL.txt") == "1\n", "B9 OK writes 1 for the market");
        for (unsigned long s = T + 370; s < T + 373; ++s) R.step(s);
        CHECK(R.S().apOn && R.S().apSetWhy == "file: on", "B9 the chart reads it back within a second");
        std::remove((flex + "LiquidityProfile.airpocket-CL.txt").c_str());
        CHECK(R.p->fileValues().v[lpset::K_AIR_POCKET] == 1, "B9 no file: on (the default)");
        const ss::Spec& sp = lpset::profileSpec();
        CHECK(sp.n == lpset::K_N && std::string(sp.rows[lpset::K_AIR_POCKET].key) == "air_pocket" && sp.paramVersion >= 7, "B9 the row air_pocket, param version 7+ (0.48.0: 8, the LMT zones rows appended)");
    }
    {   // a high turn (path down open): red band below the close, red badge, dark red tint, inside the column
        LQState& S3 = R.S(); apl::Case z; z.dir = -1; z.closeTk = 7997; z.t1Tk = 7983; z.fired = true; z.why = "air pocket"; z.ratio = 0.4; z.turnMs = S3.apNowMs - 1000;
        S3.apCases.push_back(z);
        R.h.draws.clear(); R.p->draw(); int red = 0, redTint = 0, green = 0; const int cl = R.h.paneLeft, cr = R.h.paneLeft + S3.width;
        LiquidityProfile::PNT a, b; a.set(0, 79.97f); b.set(0, 79.83f);
        for (auto& d : R.h.draws) {
            if (d.type == "text" && d.text == "AP?" && d.color == C_SELL && d.l >= cl && d.r <= cr) ++red;
            if (d.type == "text" && d.text == "AP?" && d.color == C_BUY) ++green;
            if (d.type == "box" && d.color == (COLOR)0x002A0F0F && d.t == (int)a.v && d.b == (int)b.v && d.l >= cl && d.r <= cr) ++redTint;
        }
        CHECK(red == 1 && redTint == 1 && green == 0, "B10 path down open at a high: red band close..T1 below, red AP? badge, inside the column");
    }
    CHECK(std::string(LQ_VERSION) == "0.48.0" && std::string(lqp::VERSION) == "0.48.0" && std::string(lpset::PLUGIN_VERSION) == "0.48.0" && std::string(apl::VERSION) == "1.0.0", "versions 0.47.0 / AirPocketLogic 1.0.0");
    R.p->destroy(); R.p->slot_.release(R.p); R.p->flushPark(); delete R.p;
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/ap047XXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    partA();
    partB(tmp);
    std::printf("air pocket 0.47.0: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
