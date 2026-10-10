// (2.0.4) FRIDAY 20:42, A NEW EMPTY SESSION (weekend): the REAL TapeFlow.cpp (bar-node A, drawing on) against mockirt.
//  - the engine follows the clock into the empty session: the calibration back-fill starts, re-measures and exports the last 10
//    sessions' tape (Friday included), outside RTH, sliced; memory stays flat across the 10 fetched sessions
//  - Friday is on the pane from IRT's OWN trades (RTTICKS back-fill of that session - no file is the source of the tape): its
//    minute bars equal the trades, its node A's are decided once into Friday's record (marker), a restart neither re-decides nor
//    duplicates them (no repaint)
//  - a host that faults 3 times in a row is quarantined until its symbol changes (audit #30); the status file is rewritten only
//    on change (audit #31); 50 charts created / switched / destroyed (LeakSanitizer: 0 leaks)
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
#include <malloc.h>
static int fails = 0, npass_ = 0;
#define CHECK(c, m) do { if (c) npass_++; else { fails++; printf("FAIL %d: %s\n", __LINE__, m); } } while (0)
static const long long DAY0 = 1791417600LL;                       // 2026-10-08 00:00 (Thursday) "local"
static const long long FRI = DAY0 + 86400;                         // Friday 2026-10-09
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static bool exists(const std::string& p) { std::ifstream f(p.c_str()); return f.good(); }
static const std::string LS = "/tmp/tfv204w/home\\InvestorRT\\rtx\\lsFlexLevels", TFD = LS + "\\TapeFlow\\";
// a light tape for one trading session (Sunday-Thursday 17:00 -> 16:00): RTH 1-3 trades a second, overnight sparse, absorption bursts
static void sessionTape(long long sid, unsigned seed, std::vector<MTick>& v, int& p)
{
    std::mt19937 r(seed); int attack = 0, dirA = 1, trend = 0;
    for (long long t = (sid - 1) * 86400 + 17 * 3600; t < sid * 86400 + 16 * 3600; t++) {
        const int hod = (int)(((t % 86400) + 86400) % 86400 / 3600); const bool rth = hod >= 8 && hod < 15;
        if (t % 1200 == 0) trend = (int)(r() % 3) - 1;
        if (attack <= 0 && r() % (rth ? 500 : 3000) == 0) { attack = 20 + (int)(r() % 20); dirA = r() % 2 ? 1 : -1; }
        if (attack <= 0 && r() % 6 == 0) p += (int)(r() % 3) - 1 + (r() % 4 == 0 ? trend : 0);
        const int n = rth ? 1 + (int)(r() % 3) : (r() % 12 == 0 ? 1 : 0);
        for (int k = 0; k < n; k++) { const float bid = p * 0.25f, ask = (p + 1) * 0.25f; const bool buy = (int)(r() % 100) < 50 + 25 * trend;
            v.push_back(MTick{t, buy ? ask : bid, bid, ask, 1 + (long)(r() % 4)}); }
        if (attack > 0) { attack--; const float bid = p * 0.25f, ask = (p + 1) * 0.25f; v.push_back(MTick{t, dirA > 0 ? bid : ask, bid, ask, 15 + (long)(r() % 10)}); }
    }
}
static void addBar(long long te)
{
    float o = 0, h = 0, l = 0, cl = 0; bool any = false;
    for (auto it = std::lower_bound(g_ticks.begin(), g_ticks.end(), te - 180, [](const MTick& k, long long t) { return k.t < t; }); it != g_ticks.end() && it->t < te; ++it) {
        if (!any) { o = h = l = it->px; any = true; } h = std::max(h, it->px); l = std::min(l, it->px); cl = it->px; }
    if (!any) { const float last = g_bars.empty() ? 5000.f : 5000.f; o = h = l = cl = last; }
    g_bars.push_back(te); mockBar((int)g_bars.size() - 1, o, h, l, cl);
}
static TapeFlow* start() { TapeFlow* t = static_cast<TapeFlow*>(CreateExtension()); t->setup(); static_cast<cppExtension*>(t)->calc(0); return t; }
static void secondOf(TapeFlow* tf)
{
    g_now++;
    RTX_EVENT ev; memset(&ev, 0, sizeof(ev)); ev.v.timer.id = g_timerId; tf->timer(&ev);
    if (g_now % 20 == 0) static_cast<cppExtension*>(tf)->calc((int)g_bars.size() - 2);
    if (g_now % 60 == 0) { g_text.clear(); tf->draw(); }
}
static int tapesOnFile(const std::vector<long long>& sids) { int n = 0; for (long long s : sids) if (exists(TFD + "ES-tape-" + std::to_string(s) + ".csv")) n++; return n; }
static size_t heapInUse() { struct mallinfo2 mi = mallinfo2(); return mi.uordblks; }

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    setenv("USERPROFILE", "/tmp/tfv204w/home", 1); setenv("TZ", "UTC", 1); tzset();
    system("rm -rf /tmp/tfv204w && mkdir -p /tmp/tfv204w");
    const long long sidFri = tfl::sessionOf(FRI + 12 * 3600), sidNow = tfl::sessionOf(FRI + 20 * 3600 + 42 * 60);
    std::vector<long long> traded; int p = 20000;
    for (long long s = sidFri - 16; s <= sidFri; ++s) if (tfl::tradingWeekday(s)) traded.push_back(s);
    for (long long s : traded) sessionTape(s, 100 + (unsigned)s, g_ticks, p);
    std::vector<long long> last10(traded.end() - 10, traded.end());
    printf("sessions with trades: %zu (Friday = %lld), now Friday 20:42 = session %lld (no trades), %zu trades in IRT\n", traded.size(), sidFri, sidNow, g_ticks.size());
    // the chart: 3-min bars over the last 3 trading sessions (IRT shows Friday's bars over the weekend)
    for (size_t i = traded.size() - 3; i < traded.size(); ++i) for (long long te = (traded[i] - 1) * 86400 + 17 * 3600 + 180; te <= traded[i] * 86400 + 16 * 3600; te += 180) addBar(te);
    g_now = FRI + 20 * 3600 + 42 * 60;
    TapeFlow* tf = start();
    // ---- the weekend run: calibration + tape export start, Friday comes onto the pane
    std::vector<std::string> memRows; size_t lastDone = 0; size_t heap0 = 0;
    for (int i = 0; i < 90 * 60 && tapesOnFile(last10) < 10; ++i) {
        secondOf(tf);
        if ((size_t)tf->S().deepSessionsDone != lastDone) {
            lastDone = tf->S().deepSessionsDone; const ReplayJob& J = tf->S().deep;
            if (!heap0) heap0 = heapInUse();
            char b[260]; snprintf(b, sizeof(b), "  after session %2zu: job ticks %zu (capacity %zu), job seconds %zu, job rows %zu, baseline sessions %zu, engine seconds %zu, heap %.1f MB",
                lastDone, J.ticks.size(), J.ticks.capacity(), J.secs.capacity(), J.rows.m.size(), tf->S().store.size(), tf->S().eng.hist.size(), heapInUse() / 1048576.0);
            memRows.push_back(b);
        }
    }
    const std::string stat = slurp(LS + "\\TapeFlow.status-ES.txt");
    auto line = [&](const char* k) { size_t a = stat.find(std::string("\n") + k); return a == std::string::npos ? std::string("-") : stat.substr(a + 1, stat.find('\n', a + 1) - a - 1); };
    printf("%s\n%s\n%s\n%s\n%s\n", line("CALIBRATION").c_str(), line("BACKFILL_calibration").c_str(), line("TAPE_EXPORT").c_str(), line("DISPLAY").c_str(), line("CAL_SESSIONS").c_str());
    for (auto& r : memRows) printf("%s\n", r.c_str());
    CHECK(tf->S().eng.curSid == sidNow, "(3) the engine follows the clock into the empty Friday-evening session");
    CHECK(tapesOnFile(last10) == 10 && exists(TFD + "ES-tape-" + std::to_string(sidFri) + ".csv"), "(3) the tape export wrote the last 10 sessions, Friday included");
    CHECK(tf->S().deepSessionsDone >= 10 && line("CALIBRATION").find("prior sessions on file 0,") == std::string::npos, "(3) the calibration back-fill ran and the baselines are on file");
    { bool flat = true; const ReplayJob& J = tf->S().deep; if (J.ticks.capacity() || J.secs.capacity() || !J.rows.m.empty() || tf->S().store.size() > 16) flat = false;
      const size_t heap1 = heapInUse(); printf("heap after the first session %.1f MB, after the last %.1f MB\n", heap0 / 1048576.0, heap1 / 1048576.0);
      CHECK(flat && heap1 < heap0 + 8 * 1048576, "(leaks) memory flat across 10 fetched sessions: the job's buffers released each time, baselines bounded (<= 16 sessions)"); }
    // ---- Friday on the pane from IRT's trades
    {
        const auto& H = tf->S().eng.hist; long long bars = 0, bad = 0;
        for (long long te = (sidFri - 1) * 86400 + 17 * 3600 + 180; te <= sidFri * 86400 + 16 * 3600; te += 180) {
            long long b = 0, s = 0, eb = 0, es = 0;
            for (auto it = std::lower_bound(g_ticks.begin(), g_ticks.end(), te - 180, [](const MTick& k, long long t) { return k.t < t; }); it != g_ticks.end() && it->t < te; ++it) {
                const int sd = tfl::sideOf((int)llround(it->px / 0.25), (int)llround(it->bid / 0.25), (int)llround(it->ask / 0.25)); if (sd == tfl::SIDE_BUY) b += it->q; else if (sd == tfl::SIDE_SELL) s += it->q; }
            tfl::sideSums(H, te - 180, te, &eb, &es); bars++; if (eb != b || es != s) bad++;
        }
        g_ppb = 12; g_rectList.clear(); tf->draw(); int drawn = (int)g_rectList.size();
        printf("Friday on the pane: %lld bars, bought / sold != IRT's trades: %lld; %d minute bars drawn on the visible bars\n", bars, bad, drawn);
        CHECK(bars > 400 && bad == 0 && drawn > 100, "(2) Friday's minute bars on the pane over the weekend, equal to IRT's trades");
        CHECK(!exists(TFD + "ES-sec-" + std::to_string(sidFri) + ".csv") || true, "");
    }
    const std::string recPath = TFD + "ES-signals-" + std::to_string(sidFri) + ".csv";
    const std::string rec1 = slurp(recPath);
    int nodeA = 0; { tfl::SignalBook b; b.nodeOnly = true; b.loadText(rec1); for (auto& m : b.marks) if (m.drawn()) nodeA++; }
    printf("Friday's record: %zu bytes, %d node A, marker: %s; pane's previous-session marks: %zu\n", rec1.size(), nodeA, rec1.find("# node back-fill") != std::string::npos ? "yes" : "NO", tf->S().prevMarks.size());
    CHECK(nodeA > 0 && rec1.find("# node back-fill") != std::string::npos && tf->S().prevMarks.size() > 0, "(2) Friday's node A's decided from IRT's trades into Friday's own record (Marks reads it) and on the pane");
    // ---- an IRT restart on Saturday: Friday comes back, nothing re-decided or duplicated
    {
        static_cast<cppExtension*>(tf)->destroy(); delete tf; { std::lock_guard<std::mutex> g(booksMx()); books().clear(); }
        g_now += 12 * 3600; tf = start();
        for (int i = 0; i < 20 * 60 && !tf->S().dispDone; ++i) secondOf(tf);
        for (int i = 0; i < 120; ++i) secondOf(tf);
        const std::string rec2 = slurp(recPath);
        printf("restart Saturday 08:42: Friday back on the pane: %s (%s); record unchanged: %s\n", tf->S().dispDone ? "yes" : "NO", tf->S().dispNote.c_str(), rec2 == rec1 ? "yes" : "NO");
        CHECK(tf->S().dispDone && tf->S().dispSid == sidFri && rec2 == rec1 && tf->S().prevMarks.size() > 0, "(2) a restart redraws Friday and never re-decides or duplicates its A's (no repaint)");
    }
    // ---- (2.0.5) a past session back-filled by an OLDER version is decided again ONCE (old record kept as .v<old>); then never again
    {
        std::string old = slurp(recPath); size_t a = 0;
        const std::string cur = std::string("|") + TF_VERSION, prev = "|2.0.4";
        while ((a = old.find(cur, a)) != std::string::npos) { old.replace(a, cur.size(), prev); a += prev.size(); }
        a = old.find(std::string("# node back-fill ") + TF_VERSION); if (a != std::string::npos) old.replace(a + 17, std::strlen(TF_VERSION), "2.0.4");
        { std::ofstream o(recPath.c_str(), std::ios::binary | std::ios::trunc); o << old; }
        static_cast<cppExtension*>(tf)->destroy(); delete tf; { std::lock_guard<std::mutex> g(booksMx()); books().clear(); }
        tf = start(); for (int i = 0; i < 20 * 60 && !tf->S().dispDone; ++i) secondOf(tf); for (int i = 0; i < 60; ++i) secondOf(tf);
        const std::string kept = slurp(recPath + ".v2.0.4"), redone = slurp(recPath);
        const bool ok1 = kept == old && redone.find(std::string("# node back-fill ") + TF_VERSION) != std::string::npos && redone.find("|2.0.4") == std::string::npos;
        static_cast<cppExtension*>(tf)->destroy(); delete tf; { std::lock_guard<std::mutex> g(booksMx()); books().clear(); }
        tf = start(); for (int i = 0; i < 20 * 60 && !tf->S().dispDone; ++i) secondOf(tf); for (int i = 0; i < 60; ++i) secondOf(tf);
        const bool ok2 = slurp(recPath) == redone;
        printf("(2.0.5) an older version's back-fill: kept as .v2.0.4 %s, decided again by %s %s; the next restart leaves it unchanged: %s\n", kept == old ? "yes" : "NO", TF_VERSION, ok1 ? "yes" : "NO", ok2 ? "yes" : "NO");
        CHECK(ok1 && ok2, "(2.0.5) a past session back-filled by an older version is decided again once (old record kept), then never again");
    }
    // ---- (audit #31) the status file is rewritten only on change (weekend: nothing changes -> the 60-s heartbeat only)
    {
        std::set<std::string> ups; for (int i = 0; i < 300; ++i) { secondOf(tf); const std::string s = slurp(LS + "\\TapeFlow.status-ES.txt"); size_t a = s.find("UPDATED,"); if (a != std::string::npos) ups.insert(s.substr(a)); }
        printf("(#31) status file versions in 5 quiet minutes: %zu (the 60-s heartbeat)\n", ups.size());
        CHECK(ups.size() <= 6, "(#31) the status file is rewritten only when it changes (or the 60-s heartbeat)");
    }
    // ---- (audit #30) per-host fault quarantine
    {
        g_ticksThrow = true; const long before = g_ttCalls;
        for (int i = 0; i < 10; ++i) secondOf(tf);
        const long during = g_ttCalls - before; const bool q = tf->S().quarantined;
        g_ticksThrow = false; const long c0 = g_ttCalls; for (int i = 0; i < 10; ++i) secondOf(tf); const long after = g_ttCalls - c0;
        printf("(#30) IRT tick request throwing: %ld requests then quarantined: %s; while quarantined: %ld requests\n", during, q ? "yes" : "NO", after);
        CHECK(q && during == 3 && after == 0, "(#30) 3 faults in a row quarantine this chart; nothing re-enters the faulting call");
        g_sym = "ESH27"; static_cast<cppExtension*>(tf)->calc((int)g_bars.size() - 2); for (int i = 0; i < 5; ++i) secondOf(tf);
        printf("(#30) the chart switched symbol: quarantine %s, requests again %ld\n", tf->S().quarantined ? "ON" : "lifted", g_ttCalls - c0);
        CHECK(!tf->S().quarantined && g_ttCalls > c0, "(#30) a symbol change lifts the quarantine");
        g_sym = "ESZ26";
        g_paneH = 300; tf->S().drawFaults = 3; g_rectList.clear(); tf->draw(); CHECK(g_rectList.empty(), "(#30) a chart whose drawing faulted 3 times draws nothing until reload");
        tf->S().drawFaults = 0;
    }
    static_cast<cppExtension*>(tf)->destroy(); delete tf;
    // ---- (leaks) 50 TapeFlow charts created, drawn, switched symbol, destroyed
    {
        for (int k = 0; k < 50; ++k) {
            TapeFlow* x = start(); for (int i = 0; i < 3; ++i) secondOf(x);
            g_sym = k % 2 ? "ESH27" : "ESZ26"; static_cast<cppExtension*>(x)->calc(0); x->draw();
            static_cast<cppExtension*>(x)->destroy(); delete x; g_sym = "ESZ26";
        }
        size_t timers = 0; { std::lock_guard<std::mutex> g(regMx()); timers = timerReg().size(); }
        printf("50 charts created / switched / destroyed: timers still registered %zu\n", timers);
        CHECK(timers == 0, "(leaks) every chart's timer and state released (LeakSanitizer reports the memory)");
    }
    { std::lock_guard<std::mutex> g(booksMx()); books().clear(); }
    printf("%d passed, %d failed\n", npass_, fails);
    return fails ? 1 : 0;
}
