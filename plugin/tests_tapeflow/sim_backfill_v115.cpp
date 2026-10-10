// (1.1.5) Runs the REAL TapeFlow.cpp (TapeFlowES) against mockirt: the deep back-fill reaches 10 COMPLETE sessions across
// weekends (1.1.4 stopped after 6d on a weekend), later steps replay only what is missing, and the status file says so.
// (2.0.0) rewritten for the 2.0 calibration back-fill: ONE prior session per job (newest missing first), 200-ms slices, outside RTH
// only, resumed after a restart from the files, stops for good where IRT's history ends.
#include "mockirt.h"
#include <vector>
std::vector<float> mockOut(int k);   // (1.1.5) the mock's output arrays 1 / 2
#include "TapeFlowES.cpp"
#undef mkt
#undef root
#undef sym
#undef chartRoot
#undef err
#undef tick
#undef eng
#undef store
#undef storeLoaded
#undef cur
#undef wallAtTick
#undef dataAtTick
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
#undef deepDone
#undef stepNote
#undef lastStepName
#undef stepTicks
#undef evWritten
#undef evLastT
#undef written
#undef keysLoaded
#undef savedDigest
#undef liveTicks
#undef passes
#undef font
#include <random>
#include <set>
#include <cassert>
#include <fstream>
#include <iostream>
static int fails = 0, npass_ = 0;
#define CHECK(c, m) do { if (c) npass_++; else { fails++; printf("FAIL %d: %s\n", __LINE__, m); } } while (0)

// (2.0.4) every TapeFlow object the test creates is destroyed and deleted at the end (IRT owns them in real life), so LeakSanitizer
// sees only what the PLUGIN would leak
static std::vector<TapeFlow*> g_ownedTF;
static TapeFlow* own(TapeFlow* p) { g_ownedTF.push_back(p); return p; }
static void freeAll() { for (TapeFlow* p : g_ownedTF) { static_cast<cppExtension*>(p)->destroy(); delete p; } g_ownedTF.clear(); { std::lock_guard<std::mutex> g(booksMx()); books().clear(); } }
static const long long DAY0 = 1791417600LL;               // 2026-10-08 00:00 (a Thursday), "local"
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static const std::string LS = "/tmp/tfbf115/home\\InvestorRT\\rtx\\lsFlexLevels";
static void tickPlugin(TapeFlow* tf, int secs)
{
    for (int i = 0; i < secs; i++) {
        g_now++;
        RTX_EVENT ev; memset(&ev, 0, sizeof(ev)); ev.v.timer.id = g_timerId; tf->timer(&ev);
        if (g_now % 180 == 0) g_bars.push_back(g_now);
        if (i % 30 == 0) { g_text.clear(); static_cast<cppExtension*>(tf)->calc(0); tf->draw(); }
    }
}
// a thin tape: one trade every 2 s, every session Monday-Friday (17:00 the evening before -> 16:00), nothing at weekends
static std::vector<MTick> thinTape(long long from, long long to)
{
    std::vector<MTick> v; int p = 20000;
    for (long long t = from; t < to; t += 2) {
        const long long sid = tfl::sessionOf(t);
        if (!tfl::tradingWeekday(sid)) continue;
        int hod = (int)(((t % 86400) + 86400) % 86400 / 3600);
        if (hod == 16) continue;
        if ((t / 2) % 7 == 0) p += ((t / 14) % 3) - 1;
        bool buy = (t / 2) % 3 != 0;
        v.push_back(MTick{t, buy ? (p + 1) * 0.25f : p * 0.25f, p * 0.25f, (p + 1) * 0.25f, 1 + (long)((t / 2) % 3)});
    }
    return v;
}
// (2.0.4) a calibration REQUEST line ("calibration back-fill: session N (date)..."), not the "measured" line of a pane back-fill
static bool calStart(const std::string& t) { std::stringstream s(t); std::string ln; while (std::getline(s, ln)) if (ln.find("calibration back-fill: session") != std::string::npos && ln.find("measured") == std::string::npos && ln.find("...") != std::string::npos) return true; return false; }
int main()
{
    setenv("USERPROFILE", "/tmp/tfbf115/home", 1); setenv("TZ", "UTC", 1); tzset();
    system("mkdir -p /tmp/tfbf115 && rm -f /tmp/tfbf115/home\\\\*");
    g_now = DAY0 + 18 * 3600 + 1800;                         // Thursday 18:30 - outside ES RTH, the Friday session is under way
    g_ticks = thinTape(DAY0 - 26 * 86400, g_now + 4 * 3600);
    for (long long b = g_now - 3 * 3600; b <= g_now; b += 180) g_bars.push_back(b - b % 180);
    TapeFlow* tf = own(static_cast<TapeFlow*>(CreateExtension()));
    tf->setup(); static_cast<cppExtension*>(tf)->calc(0);
    tickPlugin(tf, 6 * 60);                                   // 6 minutes: 10m, session, then one prior session at a time
    const std::string tr = slurp(LS + "\\TapeFlow.trace-ES.txt");
    const long long sid = tfl::sessionOf(g_now);
    const int complete = tfl::completeSessions(tf->S().store, sid, COMPLETE_WINDOWS);
    size_t jobs = 0; for (size_t a = 0; (a = tr.find("calibration back-fill: session", a)) != std::string::npos; a++) jobs++;
    printf("complete prior sessions %d, on file %d, calibration jobs traced %zu\n", complete, (int)tfl::priorSessions(tf->S().store, sid), jobs);
    CHECK(complete >= 10, "10 complete prior sessions after the calibration back-fill (weekends skipped, never stopped by one)");
    CHECK(tr.find(" measured (") != std::string::npos && tr.find(" slices)") != std::string::npos, "each session measured in timed slices");
    CHECK(tr.find("session " + std::to_string(sid - 1) + " (") < tr.find("session " + std::to_string(sid - 2) + " ("), "newest missing session first");
    std::string stat = slurp(LS + "\\TapeFlow.status-ES.txt");
    CHECK(stat.find("BACKFILL_calibration,") != std::string::npos, "status shows the calibration back-fill");
    CHECK(stat.find("CAL_SESSIONS,used 10,") != std::string::npos, "the baselines use 10 sessions");
    CHECK(tr.find("DATA INVALID") == std::string::npos && tr.find("rejected") == std::string::npos, "no rejected request");
    { size_t a = stat.find("BACKFILL_calibration,"); if (a != std::string::npos) printf("  %s\n", stat.substr(a, stat.find('\n', a) - a).c_str()); }
    // a restart: the files are complete, so nothing is fetched again
    static_cast<cppExtension*>(tf)->destroy();
    g_ttMaxBack = 0; const size_t trBefore = slurp(LS + "\\TapeFlow.trace-ES.txt").size();
    TapeFlow* r = own(static_cast<TapeFlow*>(CreateExtension())); r->setup(); static_cast<cppExtension*>(r)->calc(0);
    tickPlugin(r, 3 * 60);
    {   // (2.0.4) the only history request after a restart: the last traded session for the pane (its tape comes from IRT, never a file)
        const std::string t1 = slurp(LS + "\\TapeFlow.trace-ES.txt").substr(trBefore); size_t panes = 0, a = 0; while ((a = t1.find("pane back-fill (", a)) != std::string::npos) { panes++; a++; }
        CHECK(!calStart(t1) && panes == 1 && r->S().deepNote.find("nothing to do") != std::string::npos, "after a restart with 10 complete sessions on file: no calibration request at all (one pane back-fill of the last session)"); }
    static_cast<cppExtension*>(r)->destroy();
    // inside RTH, with sessions missing: the calibration back-fill waits (only the 10m / session reads happen)
    for (long long s2 = sid - 6; s2 >= sid - 21; --s2) std::remove((LS + "\\TapeFlow\\ES-base-v110-" + std::to_string(s2) + ".csv").c_str());
    std::remove((LS + "\\TapeFlow\\_deep-ES.txt").c_str());
    g_now = DAY0 + 86400 + 10 * 3600; g_ttMaxBack = 0;                       // Friday 10:00 CT
    TapeFlow* q = own(static_cast<TapeFlow*>(CreateExtension())); q->setup(); static_cast<cppExtension*>(q)->calc(0);
    tickPlugin(q, 3 * 60);
    printf("RTH: complete %d, longest request %lld s, note: %s\n", tfl::completeSessions(q->S().store, tfl::sessionOf(g_now), COMPLETE_WINDOWS), g_ttMaxBack, q->S().deepNote.c_str());
    CHECK(g_ttMaxBack < 86400 && q->S().deepNote.find("RTH") != std::string::npos, "inside RTH: no calibration request (it waits for the end of RTH)");
    g_now = DAY0 + 86400 + 15 * 3600 + 5;                                     // 15:00: RTH over -> it resumes where the files left off
    tickPlugin(q, 4 * 60);
    CHECK(tfl::completeSessions(q->S().store, tfl::sessionOf(g_now), COMPLETE_WINDOWS) >= 10, "after RTH: resumed and back to 10 complete sessions");
    static_cast<cppExtension*>(q)->destroy();
    // IRT's history ends 4 sessions back: recorded once, never asked again after a restart
    system("rm -f /tmp/tfbf115/home\\\\*");
    g_now = DAY0 + 18 * 3600 + 1800; g_ticks = thinTape(DAY0 - 5 * 86400, g_now + 4 * 3600);
    TapeFlow* h = own(static_cast<TapeFlow*>(CreateExtension())); h->setup(); static_cast<cppExtension*>(h)->calc(0);
    tickPlugin(h, 6 * 60);
    std::string prog = slurp(LS + "\\TapeFlow\\_deep-ES.txt");
    printf("history-end progress file:\n%s", prog.c_str());
    CHECK(prog.find("exhaustedBelow,") != std::string::npos, "where IRT's history ends is recorded");
    static_cast<cppExtension*>(h)->destroy();
    const std::string tr0 = slurp(LS + "\\TapeFlow.trace-ES.txt");
    TapeFlow* h2 = own(static_cast<TapeFlow*>(CreateExtension())); h2->setup(); static_cast<cppExtension*>(h2)->calc(0);
    tickPlugin(h2, 3 * 60);
    const std::string tr1 = slurp(LS + "\\TapeFlow.trace-ES.txt").substr(tr0.size());
    CHECK(!calStart(tr1), "after a restart: no calibration request for sessions IRT does not have (only the pane's last traded session)");
    static_cast<cppExtension*>(h2)->destroy();
    freeAll();
    printf("%d passed, %d failed\n", npass_, fails);
    return fails ? 1 : 0;
}
