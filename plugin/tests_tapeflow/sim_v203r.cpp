// (2.0.3, audit #11 / #28) the REAL TapeFlow.cpp (bar-node A): an IRT RESTART AFTER EVERY BAR gives exactly the record of one straight run.
//  (1) minute windows: the engine's per-minute bought / sold over [end-180+60k, end-180+60(k+1)) equal the trades in that window, the
//      3 minutes add up to the chart bar's volume ([end-180, end), bar time = the bar END), and every drawn minute bar's height is
//      exactly that minute's contracts on the pane scale
//  (2) pixels at 3 zooms (narrow / normal / wide): every minute bar inside its candle's x-range, the minutes in time order with a
//      gap, no overlap with the next candle; every A arrowhead's x inside the x-range of the minute where its node traded
//  (3) the dashed grey normal-minute line at +- the normal minute; only green / red / grey are drawn; the header says TESTING
//  (4) two charts on ONE object (ES 3-min + ES 60-min): separate states, separate records and status files, ONE writer of the
//      market files (no duplicate second), a timer fired in the other chart's context still runs its own chart, removing the writer
//      chart hands the writing to the other one without a duplicate or a lost second, a dead chart's timer runs nothing
#include "mockirt.h"
#include <vector>
std::vector<float> mockOut(int k);
void mockBar(int i, float o, float h, float l, float c);
#define TF_TESTING_MODE 0
#define TF_ABSORB_METHOD 1
#include "TapeFlowES.cpp"
#undef mkt
#undef root
#undef sym
#undef err
#undef tick
#undef eng
#undef store
#undef storeLoaded
#undef cur
#undef timerOn
#undef timerRefused
#undef busy
#undef firstPass
#undef lastTimerTick
#undef timerAt
#undef lastStepAt
#undef lastSave
#undef lastEvWrite
#undef lastStatus
#undef lastFallback
#undef step
#undef blocked
#undef guardsRead
#undef stepNote
#undef evWritten
#undef evLastT
#undef written
#undef keysLoaded
#undef savedDigest
#undef liveTicks
#undef passes
#include <random>
#include <set>
#include <fstream>
#include <iostream>
#include <chrono>
static int fails = 0, npass_ = 0;
#define CHECK(c, m) do { if (c) npass_++; else { fails++; printf("FAIL %d: %s\n", __LINE__, m); } } while (0)
static const long long DAY0 = 1791417600LL;               // 2026-10-08 00:00 "local"
static std::vector<MTick> dayTape(long long from, long long to, unsigned seed, float px0)
{
    std::mt19937 r(seed); std::vector<MTick> v; int p = (int)(px0 / 0.25f); int attack = 0, dirA = 1, trend = 0;
    for (long long t = from; t < to; t++) {
        int hod = (int)(((t % 86400) + 86400) % 86400 / 3600);
        if (hod == 16) continue;
        bool rth = hod >= 8 && hod < 15;
        if (t % 1200 == 0) trend = (int)(r() % 3) - 1;
        if (attack <= 0 && r() % (rth ? 400 : 1500) == 0) { attack = 25 + (int)(r() % 20); dirA = r() % 2 ? 1 : -1; }
        if (attack <= 0 && r() % 5 == 0) p += (int)(r() % 3) - 1 + (r() % 4 == 0 ? trend : 0);
        int n = rth ? 4 + (int)(r() % 6) : (r() % 3 ? 1 : 0);
        for (int k = 0; k < n; k++) {
            float bid = p * 0.25f, ask = (p + 1) * 0.25f; bool buy = (int)(r() % 100) < 50 + 25 * trend;
            long q = 1 + (long)(r() % 4);
            if (r() % 13 == 0) { v.push_back(MTick{t, (p + 1) * 0.25f, bid, (p + 2) * 0.25f, q}); continue; }   // a 2-tick spread, traded inside: unknown side
            v.push_back(MTick{t, buy ? ask : bid, bid, ask, q});
        }
        if (attack > 0) { attack--; float bid = p * 0.25f, ask = (p + 1) * 0.25f; v.push_back(MTick{t, dirA > 0 ? bid : ask, bid, ask, 15 + (long)(r() % 10)}); v.push_back(MTick{t, dirA > 0 ? ask : bid, bid, ask, 3}); }
    }
    return v;
}

static std::string slurp(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static std::vector<long long> barsTe;
static void addBar(long long te)
{
    float o = 0, h = 0, l = 0, cl = 0; bool any = false;
    for (auto it = std::lower_bound(g_ticks.begin(), g_ticks.end(), te - 180, [](const MTick& k, long long t) { return k.t < t; }); it != g_ticks.end() && it->t < te; ++it) {
        if (!any) { o = h = l = it->px; any = true; } h = std::max(h, it->px); l = std::min(l, it->px); cl = it->px; }
    if (!any) { o = h = l = cl = 5000; }
    g_bars.push_back(te); mockBar((int)g_bars.size() - 1, o, h, l, cl);
}
static void secondOf(TapeFlow* tf)
{
    g_now++;
    RTX_EVENT ev; memset(&ev, 0, sizeof(ev)); ev.v.timer.id = g_timerId; tf->timer(&ev);
    if (g_now % 180 == 0) addBar(g_now);
    if (g_now % 20 == 0) static_cast<cppExtension*>(tf)->calc((int)g_bars.size() - 2);
}
static TapeFlow* start() { TapeFlow* t = static_cast<TapeFlow*>(CreateExtension()); t->setup(); static_cast<cppExtension*>(t)->calc(0); return t; }
static void restart(TapeFlow*& tf)   // IRT closes (the process and its memory gone) and starts again
{
    static_cast<cppExtension*>(tf)->destroy(); delete tf;
    { std::lock_guard<std::mutex> g(booksMx()); books().clear(); }
    tf = start();
}
static std::vector<std::string> keys(const std::string& text, long long through)
{
    tfl::SignalBook b; b.loadText(text); std::vector<std::string> v;
    for (auto& m : b.marks) if (m.barT <= through) { char x[200]; snprintf(x, sizeof(x), "%lld %c %d %d %c %.2f %lld %lld", m.barT, m.kind, m.dir, m.px, m.state == tfl::MK_CONFIRMED ? 'C' : m.state == tfl::MK_EXPIRED ? 'X' : '?', m.mult, m.minuteT, m.minuteVol); v.push_back(x); }
    return v;
}
int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0); setenv("TZ", "UTC", 1); tzset();
    system("rm -rf /tmp/tfv203r && mkdir -p /tmp/tfv203r");
    tfl::Store prior;
    for (int d = 5; d >= 1; d--) {
        long long s0 = DAY0 - d * 86400 - 7 * 3600, s1 = DAY0 - d * 86400 + 16 * 3600;
        auto v = dayTape(s0, s1, 300 + d, 5000);
        tfl::Engine e; e.store = &prior; e.sym = "ESZ26";
        for (auto& k : v) { tfl::Tick t; t.t = k.t; t.px = (int)llround(k.px / 0.25); t.bid = (int)llround(k.bid / 0.25); t.ask = (int)llround(k.ask / 0.25); t.q = k.q; e.add(t); }
        e.advanceTo(s1); e.flushSession();
    }
    g_ticks = dayTape(DAY0 - 7 * 3600, DAY0 + 14 * 3600, 23, 5000);
    const long long T0 = DAY0 + 9 * 3600, T1 = DAY0 + 10 * 3600 + 30 * 60;   // 09:00 -> 10:30: 30 bars, each followed by a restart
    std::string rec[2];
    for (int run = 0; run < 2; ++run) {
        const std::string home = std::string("/tmp/tfv203r/home") + (run ? "B" : "A"), LSD = home + "\\InvestorRT\\rtx\\lsFlexLevels";
        setenv("USERPROFILE", home.c_str(), 1);
        for (auto& kv : prior) { std::ofstream o((LSD + "\\TapeFlow\\ES-base-v110-" + std::to_string(kv.first) + ".csv").c_str()); for (auto& c : kv.second) for (auto& w : c.second) o << tfl::storeLine(kv.first, c.first, w) << "\n"; }
        g_bars.clear(); for (long long b = DAY0 - 7 * 3600 + 180; b <= T0; b += 180) addBar(b);
        g_now = T0; { std::lock_guard<std::mutex> g(booksMx()); books().clear(); }
        TapeFlow* tf = start();
        int restarts = 0;
        while (g_now < T1) { secondOf(tf); if (run == 1 && g_now % 180 == 5 && g_now > T0 + 300) { restart(tf); restarts++; } }
        for (int i = 0; i < 6 * 60; ++i) secondOf(tf);                   // settle: every bar through T1 decided (the restarted run replays first)
        static_cast<cppExtension*>(tf)->destroy(); delete tf;
        rec[run] = slurp(LSD + "\\TapeFlow\\ES-signals-" + std::to_string(tfl::sessionOf(T0)) + ".csv");
        printf("run %s: %d restarts, record %zu bytes\n", run ? "B (restart after every bar)" : "A (straight)", restarts, rec[run].size());
    }
    const auto a = keys(rec[0], T1), b = keys(rec[1], T1);
    size_t same = 0; for (size_t i = 0; i < std::min(a.size(), b.size()); ++i) if (a[i] == b[i]) same++;
    int drawnA = 0; for (auto& k : a) if (k.find(" A ") != std::string::npos) drawnA++;
    printf("signals through 10:30: straight %zu (%d A), restarted %zu, identical in order %zu\n", a.size(), drawnA, b.size(), same);
    for (size_t i = 0; i < std::max(a.size(), b.size()) && i < 400; ++i) if (i >= a.size() || i >= b.size() || a[i] != b[i]) { printf("  first difference at %zu: '%s' vs '%s'\n", i, i < a.size() ? a[i].c_str() : "-", i < b.size() ? b[i].c_str() : "-"); break; }
    printf("record files byte-identical: %s\n", rec[0] == rec[1] ? "yes" : "no (same signals, line order differs)");
    CHECK(!a.empty() && drawnA > 0 && a == b, "(#11 / #28) a restart after every bar gives exactly the straight run's record (same signals, states, multiples, minutes)");
    printf("%d passed, %d failed\n", npass_, fails);
    return fails ? 1 : 0;
}
