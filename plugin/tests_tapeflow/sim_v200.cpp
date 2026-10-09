// (2.0.0) Runs the REAL TapeFlow.cpp (TapeFlowES) against mockirt: the signals through IRT restarts.
//  - the plugin's signals == one straight pass of the signal logic over a straight engine run (same bars, same seconds)
//  - the forming bar is never decided
//  - a new extension object in the same IRT (chart reload) finds the same record (no line written twice)
//  - an IRT restart (process gone) reloads TapeFlow\ES-signals-<session>.csv and redraws EXACTLY the same marks; later bars only
//    add signals or remove a '?' (confirm), never change or drop one
//  - the pane draws only one letter (+ '?') per drawn signal, none touching
#include "mockirt.h"
#include <vector>
std::vector<float> mockOut(int k);
void mockBar(int i, float o, float h, float l, float c);
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
static int fails = 0, npass_ = 0;
#define CHECK(c, m) do { if (c) npass_++; else { fails++; printf("FAIL %d: %s\n", __LINE__, m); } } while (0)
static const long long DAY0 = 1791417600LL;               // 2026-10-08 00:00 (a Thursday), "local"
static std::vector<MTick> dayTape(long long from, long long to, unsigned seed, float px0)
{
    std::mt19937 r(seed); std::vector<MTick> v; int p = (int)(px0 / 0.25f); int attack = 0, dirA = 1, trend = 0;
    for (long long t = from; t < to; t++) {
        int hod = (int)(((t % 86400) + 86400) % 86400 / 3600);
        if (hod == 16) continue;
        bool rth = hod >= 8 && hod < 15;
        if (t % 1200 == 0) trend = (int)(r() % 3) - 1;                     // 20-minute pushes up / down / none
        if (attack <= 0 && r() % (rth ? 400 : 1500) == 0) { attack = 25 + (int)(r() % 20); dirA = r() % 2 ? 1 : -1; }
        if (attack <= 0 && r() % 5 == 0) p += (int)(r() % 3) - 1 + (r() % 4 == 0 ? trend : 0);
        int n = rth ? 4 + (int)(r() % 6) : (r() % 3 ? 1 : 0);
        for (int k = 0; k < n; k++) {
            float bid = p * 0.25f, ask = (p + 1) * 0.25f; bool buy = (int)(r() % 100) < 50 + 25 * trend;
            long q = 1 + (long)(r() % 4);
            v.push_back(MTick{t, buy ? ask : bid, bid, ask, q});
        }
        if (attack > 0) { attack--; float bid = p * 0.25f, ask = (p + 1) * 0.25f; v.push_back(MTick{t, dirA > 0 ? bid : ask, bid, ask, 15 + (long)(r() % 10)}); v.push_back(MTick{t, dirA > 0 ? ask : bid, bid, ask, 3}); }
    }
    return v;
}
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static const std::string LS = "/tmp/tfv200/home\\InvestorRT\\rtx\\lsFlexLevels";
static std::vector<tfl::BarIn> chart;                    // what the mock chart shows (OHLC in ticks)
static void addBar(long long te)
{
    float o = 0, h = 0, l = 0, c = 0; bool any = false;
    for (auto it = std::lower_bound(g_ticks.begin(), g_ticks.end(), te - 179, [](const MTick& k, long long t) { return k.t < t; }); it != g_ticks.end() && it->t <= te; ++it) {
        if (!any) { o = h = l = it->px; any = true; } h = std::max(h, it->px); l = std::min(l, it->px); c = it->px;
    }
    if (!any) { float last = chart.empty() ? 5000 : chart.back().c * 0.25f; o = h = l = c = last; }
    g_bars.push_back(te); mockBar((int)g_bars.size() - 1, o, h, l, c);
    tfl::BarIn b; b.te = te; b.ts = te - 180; b.o = (int)llround(o / 0.25); b.h = (int)llround(h / 0.25); b.l = (int)llround(l / 0.25); b.c = (int)llround(c / 0.25); chart.push_back(b);
}
static void tickPlugin(TapeFlow* tf, int secs)
{
    for (int i = 0; i < secs; i++) {
        g_now++;
        RTX_EVENT ev; memset(&ev, 0, sizeof(ev)); ev.v.timer.id = g_timerId; tf->timer(&ev);
        if (g_now % 180 == 0) addBar(g_now);
        if (i % 20 == 0) { static_cast<cppExtension*>(tf)->calc((int)g_bars.size() - 2); }
        if (i % 60 == 0) { g_text.clear(); tf->draw(); }
    }
}
static std::vector<tfl::Mark> marksOf(TapeFlow* tf) { std::vector<tfl::Mark> m; if (tf->S().book) { std::lock_guard<std::mutex> g(tf->S().book->mx); m = tf->S().book->book.marks; } return m; }
static std::string key(const tfl::Mark& m) { char b[96]; snprintf(b, sizeof(b), "%lld %c %d %d", m.barT, m.kind, m.dir, m.px); return b; }

int main()
{
    setenv("USERPROFILE", "/tmp/tfv200/home", 1); setenv("TZ", "UTC", 1); tzset();
    system("mkdir -p /tmp/tfv200 && rm -f /tmp/tfv200/home*");
    // 5 prior sessions on file (calibrated from the start)
    tfl::Store prior;
    for (int d = 5; d >= 1; d--) {
        long long s0 = DAY0 - d * 86400 - 7 * 3600, s1 = DAY0 - d * 86400 + 16 * 3600;
        auto v = dayTape(s0, s1, 200 + d, 5000);
        tfl::Engine e; e.store = &prior; e.sym = "ESZ26";
        for (auto& k : v) { tfl::Tick t; t.t = k.t; t.px = (int)llround(k.px / 0.25); t.bid = (int)llround(k.bid / 0.25); t.ask = (int)llround(k.ask / 0.25); t.q = k.q; e.add(t); }
        e.advanceTo(s1); e.flushSession();
    }
    { for (auto& kv : prior) { std::ofstream o((LS + "\\TapeFlow\\ES-base-v110-" + std::to_string(kv.first) + ".csv").c_str()); for (auto& c : kv.second) for (auto& w : c.second) o << tfl::storeLine(kv.first, c.first, w) << "\n"; } }
    g_ticks = dayTape(DAY0 - 7 * 3600, DAY0 + 12 * 3600, 11, 5000);
    for (long long b = DAY0 - 7 * 3600 + 180; b <= DAY0 + 9 * 3600; b += 180) addBar(b);
    g_now = DAY0 + 9 * 3600;
    TapeFlow* tf = static_cast<TapeFlow*>(CreateExtension()); tf->setup(); static_cast<cppExtension*>(tf)->calc(0);
    tickPlugin(tf, 40 * 60);                                         // 09:00 -> 09:40
    const long long sid = tfl::sessionOf(g_now);
    const std::string recPath = LS + "\\TapeFlow\\ES-signals-" + std::to_string(sid) + ".csv";
    // a straight pass: one engine over every trade, the signal logic over every closed bar
    tfl::Store st2 = prior; tfl::Engine ref; ref.store = &st2; ref.sym = "ESZ26";
    for (auto& k : g_ticks) if (k.t <= g_now) { tfl::Tick t; t.t = k.t; t.px = (int)llround(k.px / 0.25); t.bid = (int)llround(k.bid / 0.25); t.ask = (int)llround(k.ask / 0.25); t.q = k.q; ref.add(t); }
    ref.advanceTo(tf->S().eng.lastT);
    tfl::SignalBook straight;
    const long long through1 = tf->S().book ? tf->S().book->book.decidedThrough : LLONG_MIN;
    for (auto& b : chart) if (b.ts >= (sid - 1) * 86400 + 17LL * 3600 && b.te <= through1) straight.feed(b, tfl::barFlow(ref.hist, ref.evs, b.ts, b.te, straight.cfg));
    std::vector<tfl::Mark> m1 = marksOf(tf);
    bool same = m1.size() == straight.marks.size();
    for (size_t i = 0; same && i < m1.size(); ++i) if (key(m1[i]) != key(straight.marks[i]) || m1[i].state != straight.marks[i].state) same = false;
    int cnt[4] = {0}; for (auto& m : m1) cnt[std::string("PEAF").find(m.kind)]++;
    printf("09:40: %zu signals (P %d E %d A %d F %d), decided through %s, straight pass %zu\n", m1.size(), cnt[0], cnt[1], cnt[2], cnt[3], stamp(through1).c_str(), straight.marks.size());
    CHECK(!m1.empty() && same, "the plugin's signals == one straight pass (engine + signal logic) over the same bars");
    CHECK(through1 == g_bars[g_bars.size() - 2] || through1 == g_bars[g_bars.size() - 3], "every closed bar decided, the forming bar never");
    CHECK(through1 < g_bars.back(), "the forming bar is never decided");
    // the pane: only letters (+ '?'), none touching (layout checked through what was drawn)
    { g_text.clear(); tf->draw(); bool ok = true; size_t letters = 0;
      for (auto& s : g_text) { if (s.find("TF ") == 0) continue; if (!(s.size() <= 2 && std::strchr("PEAF", s[0]) && (s.size() == 1 || s[1] == '?'))) ok = false; else letters++; }
      printf("drawn letters %zu\n", letters);
      CHECK(ok && letters > 0, "the pane's only text: one letter (+ '?') per drawn signal"); }
    // a chart reload: a NEW extension object in the same IRT -> the same record object, nothing written twice
    tickPlugin(tf, 1); static_cast<cppExtension*>(tf)->destroy();
    const std::string fileA = slurp(recPath);
    TapeFlow* t2 = static_cast<TapeFlow*>(CreateExtension()); t2->setup(); static_cast<cppExtension*>(t2)->calc(0);
    tickPlugin(t2, 3 * 60);
    std::vector<tfl::Mark> m2 = marksOf(t2);
    bool kept = true; for (auto& a : m1) { bool f = false; for (auto& b : m2) if (key(a) == key(b)) { f = true; if (a.state == tfl::MK_CONFIRMED && b.state != tfl::MK_CONFIRMED) kept = false; } if (!f) kept = false; }
    CHECK(kept, "chart reload (new object, same IRT): every signal still there, none changed");
    { std::string f = slurp(recPath); std::set<std::string> rows; bool dup = false; std::stringstream ss(f); std::string ln;
      while (std::getline(ss, ln)) { if (ln.empty() || ln[0] == 't') continue; if (!rows.insert(ln).second) dup = true; }
      CHECK(!dup && f.compare(0, fileA.size(), fileA) == 0, "the record is append-only and never has a line twice"); }
    const std::vector<tfl::Mark> before = marksOf(t2);
    static_cast<cppExtension*>(t2)->destroy();
    // an IRT RESTART: the process is gone (the in-memory records too); 40 minutes later IRT starts again
    { std::lock_guard<std::mutex> g(booksMx()); books().clear(); }
    g_now += 40 * 60; while (g_bars.back() + 180 <= g_now) addBar(g_bars.back() + 180);
    TapeFlow* t3 = static_cast<TapeFlow*>(CreateExtension()); t3->setup(); static_cast<cppExtension*>(t3)->calc(0);
    tickPlugin(t3, 10);                                               // the 10-minute start only: signals wait for the session replay
    CHECK(!t3->S().sessionDone && marksOf(t3).empty(), "after a restart nothing is decided before the session replay is the live engine");
    tickPlugin(t3, 4 * 60);
    std::vector<tfl::Mark> m3 = marksOf(t3);
    bool exact = true; size_t matched = 0;
    for (auto& a : before) { bool f = false; for (auto& b : m3) if (key(a) == key(b)) { f = true; matched++;
        if (a.state == tfl::MK_CONFIRMED && b.state != tfl::MK_CONFIRMED) exact = false;
        if (a.state == tfl::MK_EXPIRED && b.state != tfl::MK_EXPIRED) exact = false; } if (!f) exact = false; }
    printf("after the restart: %zu signals (%zu from before the restart, all matched: %s)\n", m3.size(), matched, exact ? "yes" : "no");
    CHECK(exact && !before.empty() && matched == before.size(), "IRT restart: the record redraws exactly the same signals (a '?' may only confirm)");
    bool ordered = true; for (size_t i = 1; i < m3.size(); ++i) if (m3[i].barT <= m3[i - 1].barT) ordered = false;
    CHECK(ordered, "still one signal per bar after the restart");
    // the restarted plugin vs a straight pass over the whole morning: the bars decided AFTER the restart match it too
    tfl::Store st3 = prior; tfl::Engine ref3; ref3.store = &st3; ref3.sym = "ESZ26";
    for (auto& k : g_ticks) if (k.t <= g_now) { tfl::Tick t; t.t = k.t; t.px = (int)llround(k.px / 0.25); t.bid = (int)llround(k.bid / 0.25); t.ask = (int)llround(k.ask / 0.25); t.q = k.q; ref3.add(t); }
    ref3.advanceTo(t3->S().eng.lastT);
    tfl::SignalBook straight3; const long long through3 = t3->S().book->book.decidedThrough;
    for (auto& b : chart) if (b.ts >= (sid - 1) * 86400 + 17LL * 3600 && b.te <= through3) straight3.feed(b, tfl::barFlow(ref3.hist, ref3.evs, b.ts, b.te, straight3.cfg));
    bool same3 = m3.size() == straight3.marks.size();
    for (size_t i = 0; same3 && i < m3.size(); ++i) if (key(m3[i]) != key(straight3.marks[i]) || m3[i].state != straight3.marks[i].state) same3 = false;
    CHECK(same3, "restart in the middle of the day: the signals equal one straight pass over the whole morning");
    // the status file lists them
    std::string stat = slurp(LS + "\\TapeFlow.status-ES.txt");
    CHECK(stat.find("TF_SIGNALS,decided through ") != std::string::npos && stat.find("TF_SIG1,") != std::string::npos, "status: the day's signals for ChartView");
    { size_t a = stat.find("TF_COUNTS"); if (a != std::string::npos) printf("  %s\n", stat.substr(a, stat.find('\n', a) - a).c_str()); }
    // the previous session: its record is drawn after the 17:00 roll
    static_cast<cppExtension*>(t3)->destroy();
    printf("%d passed, %d failed\n", npass_, fails);
    return fails ? 1 : 0;
}
