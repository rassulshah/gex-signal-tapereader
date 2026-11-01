// Runs the REAL TapeFlow.cpp (TapeFlowES) against mockirt: back-fill + live must equal one straight engine run.
// (2.0.0) updated for 2.0: steps are 10m + the sliced session replay (which BECOMES the live engine), no settings, the readout
// is the header title (pane line only as fallback), the pane draws one-letter signals instead of AW / AR / IN markers.
#include "mockirt.h"
#include <vector>
std::vector<float> mockOut(int k);   // (1.1.5) the mock's output arrays 1 / 2
void mockBar(int i, float o, float h, float l, float c);   // (2.0.0) the chart OHLC
#define TF_TESTING_MODE 0   // (2.0.2) these suites check the drawing: build with test mode OFF
#define TF_ABSORB_METHOD 0  // (2.0.3) this suite covers the 20-second method (sim_v203 covers the bar node + the minute tape)
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
static std::vector<MTick> dayTape(long long from, long long to, unsigned seed, float px0)
{
    std::mt19937 r(seed); std::vector<MTick> v; int p = (int)(px0 / 0.25f); int attack = 0, dirA = 1;
    for (long long t = from; t < to; t++) {
        int hod = (int)(((t % 86400) + 86400) % 86400 / 3600);
        if (hod == 16) continue;                                        // the daily break 16:00-17:00
        bool rth = hod >= 8 && hod < 15;
        if (attack <= 0 && r() % (rth ? 400 : 1500) == 0) { attack = 25 + (int)(r() % 20); dirA = r() % 2 ? 1 : -1; }
        if (attack <= 0 && r() % 5 == 0) p += (int)(r() % 3) - 1;
        int n = rth ? 4 + (int)(r() % 6) : (r() % 3 ? 1 : 0);
        for (int k = 0; k < n; k++) {
            float bid = p * 0.25f, ask = (p + 1) * 0.25f; bool buy = r() % 2;
            long q = 1 + (long)(r() % 4);
            v.push_back(MTick{t, buy ? ask : bid, bid, ask, q});
        }
        if (attack > 0) { attack--; float bid = p * 0.25f, ask = (p + 1) * 0.25f; v.push_back(MTick{t, dirA > 0 ? bid : ask, bid, ask, 15 + (long)(r() % 10)}); v.push_back(MTick{t, dirA > 0 ? ask : bid, bid, ask, 3}); }
    }
    return v;
}
static tfl::Engine pure(const std::vector<MTick>& v, long long until, tfl::Store& st)
{
    tfl::Engine e; e.store = &st; e.sym = "ESZ26";
    for (auto& k : v) if (k.t <= until) { tfl::Tick t; t.t = k.t; t.px = (int)llround(k.px / 0.25); t.bid = (int)llround(k.bid / 0.25); t.ask = (int)llround(k.ask / 0.25); t.q = k.q; e.add(t); }
    return e;
}
// (2.0.1) an absorption label: "A 3.1x" / "A? 3.1x"
static bool aLabel(const std::string& s) { if (s == "A 10x+" || s == "A? 10x+") return true;   // (2.0.5) the display cap
    size_t i = 1; if (s.empty() || s[0] != 'A') return false; if (i < s.size() && s[i] == '?') i++; if (i >= s.size() || s[i] != ' ') return false; i++;
    size_t d = i; while (i < s.size() && (isdigit((unsigned char)s[i]) || s[i] == '.')) i++; return i > d && i + 1 == s.size() && s[i] == 'x'; }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static const std::string LS = "/tmp/tfchk/home\\InvestorRT\\rtx\\lsFlexLevels";
// (2.0.0) a chart bar ending at te with its OHLC from the trades IRT has
static void addBar(long long te)
{
    float o = 0, h = 0, l = 0, c = 0; bool any = false;
    for (auto& k : g_ticks) if (k.t > te - 180 && k.t <= te) { if (!any) { o = h = l = k.px; any = true; } h = std::max(h, k.px); l = std::min(l, k.px); c = k.px; }
    if (!any) { o = h = l = c = 5000; }
    g_bars.push_back(te); mockBar((int)g_bars.size() - 1, o, h, l, c);
}
static void tickPlugin(TapeFlow* tf, int secs, bool drawEach = false)
{
    for (int i = 0; i < secs; i++) {
        g_now++;
        RTX_EVENT ev; memset(&ev, 0, sizeof(ev)); ev.v.timer.id = g_timerId; tf->timer(&ev);
        if (g_now % 180 == 0) addBar(g_now);
        if (drawEach || i % 30 == 0) { g_text.clear(); static_cast<cppExtension*>(tf)->calc(0); tf->draw(); }
    }
}
int main()
{
    setenv("USERPROFILE", "/tmp/tfchk/home", 1); setenv("TZ", "UTC", 1); tzset();
    system("rm -rf /tmp/tfchk/home\\\\*");
    // ---- 5 prior sessions -> the plugin's own baseline file (as if it had run those days)
    tfl::Store prior;
    for (int d = 5; d >= 1; d--) {
        long long s0 = DAY0 - d * 86400 - 7 * 3600, s1 = DAY0 - d * 86400 + 16 * 3600;   // 17:00 -> 16:00
        auto v = dayTape(s0, s1, 100 + d, 5000);
        tfl::Engine e = pure(v, s1, prior); e.advanceTo(s1); e.flushSession();
    }
    { for (auto& kv : prior) { std::ofstream o((LS + "\\TapeFlow\\ES-base-v110-" + std::to_string(kv.first) + ".csv").c_str()); for (auto& c : kv.second) for (auto& w : c.second) o << tfl::storeLine(kv.first, c.first, w) << "\n"; } }
    printf("prior sessions %zu, windows %zu\n", prior.size(), prior.begin()->second.begin()->second.size());

    // ---- today: the session from 17:00 yesterday; IRT opens at 09:00 with the history already there
    g_ticks = dayTape(DAY0 - 7 * 3600, DAY0 + 12 * 3600, 7, 5000);
    for (long long b = DAY0 - 7 * 3600 + 180; b <= DAY0 + 9 * 3600; b += 180) addBar(b);
    g_now = DAY0 + 9 * 3600;
    TapeFlow* tf = own(static_cast<TapeFlow*>(CreateExtension()));
    tf->setup();
    static_cast<cppExtension*>(tf)->calc(0); tf->draw();
    CHECK(g_ttCalls == 0, "calc / draw never ask IRT for data (only the timer does)");
    tickPlugin(tf, 1); CHECK(g_ttCalls <= 2 && g_ttMaxBack == 600, "first timer tick: only the last 10 minutes (+ the live read)");
    tickPlugin(tf, 24);
    CHECK(g_ttMaxBack == 600, "no history request in the first 30 s (IRT settles)");
    tickPlugin(tf, 120);
    CHECK(g_ttMaxBack > 9 * 3600, "the session back-fill ran (from 17:00)");
    std::string tr = slurp(LS + "\\TapeFlow.trace-ES.txt");
    CHECK(tr.find("back-fill session: ") != std::string::npos && tr.find("back-fill session done: ") != std::string::npos && tr.find("back-fill 1h") == std::string::npos, "(2.0.0) 10m, then the sliced session replay");
    CHECK(tr.find("calibration back-fill: session") == std::string::npos, "no calibration history during trading hours");
    // live for 50 minutes, drawing as we go
    tickPlugin(tf, 3000);
    long long until = tf->S().eng.lastT;
    tfl::Store st2 = prior; tfl::Engine ref = pure(g_ticks, g_now, st2); ref.advanceTo(until);   // every trade IRT has; closed to the same second
    auto key = [](const tfl::Ev& e) { char b[120]; snprintf(b, sizeof(b), "%lld %s %d %d %d", e.t, e.kind.c_str(), e.dir, e.lo, e.hi); return std::string(b); };
    // (2.0.0) the session replay becomes the live engine: the ENGINE's events equal the straight run's (the events file keeps
    // the 10-minute start's records it already wrote - append-only - and is checked for duplicates below)
    std::vector<std::string> A, B; for (auto& e : tf->S().eng.evs) if (e.t <= until && e.kind != "BASE") A.push_back(key(e)); for (auto& e : ref.evs) if (e.t <= until) B.push_back(key(e));
    int nAW = 0; for (auto& e : ref.evs) if (e.kind == "AW") nAW++;
    printf("plugin events %zu, straight run %zu (AW %d), trades %lld vs %lld\n", A.size(), B.size(), nAW, tf->S().eng.ticks, ref.ticks);
    CHECK(A == B && !A.empty(), "back-fill + live == one straight run (same events, same times, same zones)");
    if (A != B) for (size_t i = 0; i < std::max(A.size(), B.size()); i++) { std::string a = i < A.size() ? A[i] : "-", b = i < B.size() ? B[i] : "-"; if (a != b) { printf("  %zu: %s | %s\n", i, a.c_str(), b.c_str()); if (i > 8) break; } }
    CHECK(tf->S().eng.ticks == ref.ticks, "every trade counted once (no duplicates, none missed)");
    bool histSame = tf->S().eng.hist.size() > 0;
    { size_t j = 0; for (auto& h : tf->S().eng.hist) { while (j < ref.hist.size() && ref.hist[j].t < h.t) j++; if (j < ref.hist.size() && ref.hist[j].t == h.t) { if (!(std::isnan(h.f30) && std::isnan(ref.hist[j].f30)) && h.f30 != ref.hist[j].f30) histSame = false; } } }
    CHECK(histSame, "the drawn 30-s flow equals the straight run second by second");
    { long nanP = 0, nanR = 0; long long lo = tf->S().eng.hist.empty() ? 0 : tf->S().eng.hist.front().t;
      for (auto& h : tf->S().eng.hist) if (std::isnan(h.f180)) nanP++;
      for (auto& h : ref.hist) if (h.t >= lo && h.t <= until && std::isnan(h.f180)) nanR++;
      printf("seconds without a 180-s value: plugin %ld, straight run %ld\n", nanP, nanR);
      CHECK(nanP <= nanR + 5, "no extra gap in the 180-s line where the plugin started"); }
    CHECK(tf->S().eng.baseSessions == 5, "baselines from the 5 sessions on file");
    // the drawing
    g_text.clear(); g_lines = g_rects = 0; static_cast<cppExtension*>(tf)->calc(0); tf->draw();
    bool head = false; std::string state; const std::string TV = std::string("TF ") + tfl::shortVersion(TF_VERSION) + " ES";
    for (auto& s : g_text) { if (s.find(TV) != std::string::npos) head = true; }
    CHECK(head, "(1.1.5) IRT never asked for the header title: the one-line pane readout shows the name and version");
    // ---- (1.1.5) header mode: the title, the two outputs, the fitted scale, no box
    {
        char title[128] = {0};
        CHECK(tf->parmsTitle(title, sizeof(title)) == RTX_OK && std::string(title).find(TV + "  ") == 0, "parmsTitle: 'TF <version> ES  <state>'");
        printf("header title: %s\n", title);
        g_text.clear(); g_rects = 0; static_cast<cppExtension*>(tf)->calc(0); tf->draw();
        bool anyReadout = false; for (auto& s : g_text) if (s.find("TF ") == 0 || s.find("TapeFlow ") == 0 || s.find("READY") != std::string::npos || s.find("CALIBRATING") != std::string::npos) anyReadout = true;
        CHECK(!anyReadout, "header live (IRT asked for the title after the state changed): no box and no readout line in the pane");
        tf->S().lastTitleCall = 0; tf->S().titleChangedAt = (time_t)g_now - 30; g_text.clear(); tf->draw(); bool line1 = false; int lines = 0;   // (2.0.0) IRT stops re-reading the title
        for (auto& s : g_text) if (s.find("TF ") == 0) { line1 = true; lines++; }
        CHECK(line1 && lines == 1, "header not re-read: exactly one compact line in the pane");
        tf->S().lastTitleCall = (long long)g_now; tf->S().titleChangedAt = (time_t)g_now;
        std::vector<float> o1 = mockOut(0), o2 = mockOut(1);
        const tfl::Feat& lf = tf->S().eng.last;
        CHECK(o1.size() == g_bars.size() && o2.empty(), "(2.0.2) one output (30-s pressure) covering every bar; no 180-s output");
        if (o2.empty()) o2.assign(1, 0.f);
        bool sane = true; for (float v : o1) if (!(v >= -100 && v <= 100)) sane = false;
        CHECK(sane, "outputs are pressures in -100..100 (no fixed 135 anchors)");
        printf("last bar outputs: 30s %.0f  180s %.0f   (engine now: %.1f / %.1f)\n", o1.back(), o2.back(), lf.f30, lf.f180);
        double lo = 0, hi = 0; tf->scale((int)g_bars.size() - 160, (int)g_bars.size() - 1, &lo, &hi);
        {   // (2.0.2) "get rid of the white line": no 180-s line, and the scale fits the bars only
            g_lineColors.clear(); g_text.clear(); tf->draw();
            CHECK(g_lineColors.count(0x00CBD5E1UL) == 0, "(2.0.2) the 180-s line is not drawn");
            bool onlyKnown = true; for (auto& kv : g_lineColors) if (kv.first != 0x0022C55EUL && kv.first != 0x00EF4444UL && kv.first != 0x00373F4CUL && kv.first != 0x00505C6FUL) onlyKnown = false;
            for (auto& kv : g_lineColors) printf("   lines in colour %06lX: %ld\n", kv.first, kv.second);
            CHECK(onlyKnown, "(2.0.2) the only lines: the faint zero line, the (2.0.3) dashed normal-minute line and the arrowheads (green / red)");
            const int nb = (int)g_bars.size(); double barsMax = 0;
            { tfl::SecRec dummy; (void)dummy; }
            const double fitted = tfl::fitContracts(tf->visibleMaxAbs(nb - 160, nb - 1), tf->normalMinute());
            double lo2 = 0, hi2 = 0; tf->scale(nb - 160, nb - 1, &lo2, &hi2); (void)barsMax;
            CHECK(std::fabs(tf->S().range - fitted) < 1e-9, "(2.0.3) the scale fits the minute bars (contracts) and the normal minute");
        }
        CHECK(hi > 0 && lo == -hi && hi >= 60 && std::fabs(hi - tfl::axisRange(tf->S().range, tf->S().paneH, tf->S().band)) < 1e-9 && std::fmod(tf->S().range, 10.0) == 0, "scale(): symmetric, >= 60, the fitted range (rounded to 10) widened for the signal bands");
        printf("fitted range +-%.0f\n", hi);
        extern long g_invalidates; CHECK(g_invalidates > 0, "state changes ask IRT to repaint (invalidateChart)");
    } CHECK(g_rects > 100 && g_lines > 50, "histogram bars and the 180-s line drawn");
    printf("rects drawn %ld for the visible bars (30-s sub-bars: expect ~6 per 3-min bar)\n", g_rects);
    {   // (2.0.0) the signals: decided on closed bars only, drawn as one letter (+ '?')
        size_t nm = 0; long long through = LLONG_MIN;
        if (tf->S().book) { nm = tf->S().book->book.marks.size(); through = tf->S().book->book.decidedThrough; }
        printf("signals decided through %lld (last closed bar %lld, forming bar %lld): %zu\n", through, g_bars[g_bars.size() - 2], g_bars.back(), nm);
        CHECK(through == g_bars[g_bars.size() - 2] || through == g_bars[g_bars.size() - 3], "(2.0.0) every closed bar decided, the forming bar never");
        bool letters = true; const std::vector<std::string> tp = tf->S().titleParts;   // the fallback header line's parts are allowed
        for (auto& s : g_text) if (!s.empty() && !aLabel(s) && std::find(tp.begin(), tp.end(), s) == tp.end()) letters = false;
        if (!letters) for (auto& s : g_text) printf("   text: [%s]\n", s.c_str());
        CHECK(letters, "(2.0.1) the pane's only text: absorption labels 'A 3.1x' / 'A? 3.1x'");
    }
    for (auto& s : g_text) if (s.find("TF ") == 0) state = s;
    { char t2[128] = {0}; tf->parmsTitle(t2, sizeof(t2)); state = t2; }
    printf("state line: %s\n", state.c_str());
    CHECK(!state.empty(), "a state line is shown");
    // files
    std::string stat = slurp(LS + "\\TapeFlow.status-ES.txt");
    CHECK(stat.find((std::string("VERSION,") + TF_VERSION).c_str()) != std::string::npos && stat.find("SYMBOL,ESZ26") != std::string::npos, "status file");
    CHECK(stat.find("STATE_CODE,") != std::string::npos && stat.find("CAL_SLOT,bin ") != std::string::npos && stat.find("QUIET,last trade") != std::string::npos &&
          stat.find("BACKFILL_calibration,") != std::string::npos && stat.find("TF_SIGNALS,decided through") != std::string::npos && stat.find("TF_COUNTS,P ") != std::string::npos && stat.find("SIGNALS_TODAY,AW+ ") != std::string::npos && stat.find("LATE,today ") != std::string::npos &&
          stat.find("WARN5,") != std::string::npos && stat.find("TITLE,TF ") != std::string::npos && stat.find("\nUPDATED,") != std::string::npos, "(1.1.5) status: full diagnostic snapshot");
    std::string evf = slurp(LS + "\\TapeFlow\\ES-events-v110-ESZ26-2026-10-08.csv");   // (1.1.5 harness) the 1.1.0 file name
    size_t lines = (size_t)std::count(evf.begin(), evf.end(), '\n');
    CHECK(lines >= 2 && evf.find("time|episode|kind") == 0, "events file for today's session");
    printf("events file lines %zu\n", lines);
    // a close: guards cleared, baselines saved with today's windows
    static_cast<cppExtension*>(tf)->destroy();
    std::string base = slurp(LS + "\\TapeFlow\\ES-base-v110-" + std::to_string(tfl::sessionOf(DAY0 + 9 * 3600)) + ".csv");
    CHECK(base.find("|" + std::to_string(tfl::sessionOf(DAY0 + 9 * 3600)) + "|ESZ26|") != std::string::npos, "today's windows saved to the baseline file");
    CHECK(slurp(LS + "\\TapeFlow\\_trying-ES-session.txt").empty(), "no guard left after a normal close");

    // ---- REVIEW #1: an IRT restart never shrinks or duplicates the day's records
    {
        std::string f0 = slurp(LS + "\\TapeFlow\\ES-events-v110-ESZ26-2026-10-08.csv"); size_t n0 = (size_t)std::count(f0.begin(), f0.end(), '\n');
        g_now = DAY0 + 13 * 3600; while (g_bars.back() + 180 <= g_now) g_bars.push_back(g_bars.back() + 180);
        TapeFlow* r = own(static_cast<TapeFlow*>(CreateExtension())); r->setup(); static_cast<cppExtension*>(r)->calc(0);
        tickPlugin(r, 15);                                          // only the 10-minute back-fill so far
        std::string f1 = slurp(LS + "\\TapeFlow\\ES-events-v110-ESZ26-2026-10-08.csv"); size_t n1 = (size_t)std::count(f1.begin(), f1.end(), '\n');
        CHECK(n1 >= n0 && f1.compare(0, f0.size(), f0) == 0, "restart: the earlier records are kept untouched (append-only)");
        tickPlugin(r, 120);                                         // the session back-fill re-creates the morning's events
        std::string f2 = slurp(LS + "\\TapeFlow\\ES-events-v110-ESZ26-2026-10-08.csv");
        std::set<std::string> keys; bool dup = false; std::stringstream ss(f2); std::string ln;
        while (std::getline(ss, ln)) { if (ln.empty() || ln[0] == 't') continue; size_t p = 0; for (int k = 0; k < 4; k++) p = ln.find('|', p + 1); std::string key = ln.substr(0, p); if (!keys.insert(key).second) dup = true; }
        CHECK(!dup, "no record written twice after the back-fill re-creates them");
        printf("events file: %zu lines before restart, %zu after\n", n0, keys.size() + 1);
        static_cast<cppExtension*>(r)->destroy();
    }
    // ---- REVIEW #3: a stalled timer never brings the back-fill into calc / draw
    {
        g_ttCalls = 0; g_ttMaxBack = 0; g_now = DAY0 + 14 * 3600;
        TapeFlow* s3 = own(static_cast<TapeFlow*>(CreateExtension())); s3->setup();
        for (int i = 0; i < 90; i++) { g_now++; g_text.clear(); static_cast<cppExtension*>(s3)->calc(0); s3->draw(); }   // no timer ticks at all
        CHECK(g_ttMaxBack <= 600, "no timer: calc / draw only ever read the last minutes, never a back-fill");
        static_cast<cppExtension*>(s3)->destroy();
    }
    // ---- crash guard: a back-fill that killed IRT twice is blocked
    {
        { std::ofstream o((LS + "\\TapeFlow\\_trying-ES-session.txt").c_str()); o << "1\n"; }
        g_ttCalls = 0; g_ttMaxBack = 0; g_now = DAY0 + 10 * 3600 + 5;
        TapeFlow* t2 = own(static_cast<TapeFlow*>(CreateExtension())); t2->setup(); static_cast<cppExtension*>(t2)->calc(0);
        tickPlugin(t2, 120);
        CHECK(!slurp(LS + "\\TapeFlow\\_blocked-ES-session.txt").empty(), "second stop -> blocked file");
        std::string tr2 = slurp(LS + "\\TapeFlow.trace-ES.txt");
        CHECK(tr2.find("session BLOCKED") != std::string::npos, "blocked step traced");
        static_cast<cppExtension*>(t2)->destroy();
        std::remove((LS + "\\TapeFlow\\_blocked-ES-session.txt").c_str());
    }
    // ---- the wrong chart: says so, asks IRT for nothing
    {
        g_root = "NQ"; g_ttCalls = 0; TapeFlow* t3 = own(static_cast<TapeFlow*>(CreateExtension())); t3->setup();
        g_text.clear(); static_cast<cppExtension*>(t3)->calc(0); t3->draw(); tickPlugin(t3, 40);
        bool said = false; for (auto& s : g_text) if (s.find("this one is for the ES chart") != std::string::npos && s.find("TapeFlowNQ") != std::string::npos) said = true;
        CHECK(said && g_ttCalls == 0, "on the NQ chart: a note, no requests");
        g_root = "ES";
    }
    // ---- the market opens after IRT: no trades at load, reading starts when they come
    {
        g_ticks = dayTape(DAY0 + 17 * 3600, DAY0 + 19 * 3600, 9, 5000); g_now = DAY0 + 16 * 3600 + 1800;   // 16:30, the break
        TapeFlow* t4 = own(static_cast<TapeFlow*>(CreateExtension())); t4->setup(); static_cast<cppExtension*>(t4)->calc(0);
        tickPlugin(t4, 600);  // 16:40
        CHECK(t4->S().eng.ticks == 0, "nothing in the break");
        tickPlugin(t4, 1800); // 17:10
        CHECK(t4->S().eng.ticks > 0 && t4->S().eng.lastT >= DAY0 + 17 * 3600 + 500, "trades read once the market opens");
        g_text.clear(); t4->draw(); bool ok = false; for (auto& s : g_text) if (s.find(TV) == 0) ok = true;
        CHECK(ok, "a state is shown after the open"); if (!ok) for (auto& s : g_text) printf("   text: %s\n", s.c_str());
        // contract roll: the chart moves to the next contract -> starts over on it
        g_sym = "ESH27"; tickPlugin(t4, 5);
        std::string tr4 = slurp(LS + "\\TapeFlow.trace-ES.txt");
        CHECK(tr4.find("contract changed ESZ26 -> ESH27") != std::string::npos && t4->S().sym == "ESH27", "contract change starts over");
        g_sym = "ESZ26";
    }
    freeAll();
    printf("%d passed, %d failed\n", npass_, fails);
    return fails ? 1 : 0;
}
