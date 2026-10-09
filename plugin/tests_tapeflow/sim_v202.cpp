// (2.0.2) Runs the REAL TapeFlow.cpp (TapeFlowES, default build = TEST MODE) against mockirt: test mode draws nothing but the
// header; the outside-RTH back-fill exports each of the last 10 sessions' tape (second x price) and 3-min OHLCV, equal to the trades
// IRT has, resumable (a stale .part is redone, a finished file is never fetched again), never inside RTH.
// (from sim_backfill_v115:) the deep back-fill reaches 10 COMPLETE sessions across
// weekends (1.1.4 stopped after 6d on a weekend), later steps replay only what is missing, and the status file says so.
// (2.0.0) rewritten for the 2.0 calibration back-fill: ONE prior session per job (newest missing first), 200-ms slices, outside RTH
// only, resumed after a restart from the files, stops for good where IRT's history ends.
#include "mockirt.h"
#include <vector>
std::vector<float> mockOut(int k);
void mockBar(int i, float o, float h, float l, float c);   // (1.1.5) the mock's output arrays 1 / 2
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
#include <array>
#include <map>
#include <sstream>
static int fails = 0, npass_ = 0;
#define CHECK(c, m) do { if (c) npass_++; else { fails++; printf("FAIL %d: %s\n", __LINE__, m); } } while (0)
static const long long DAY0 = 1791417600LL;               // 2026-10-08 00:00 (a Thursday), "local"
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static const std::string LS = "/tmp/tfv202/home\\InvestorRT\\rtx\\lsFlexLevels";
static void addBar(long long te)
{
    float o = 0, h = 0, l = 0, c = 0; bool any = false;
    for (auto it = std::lower_bound(g_ticks.begin(), g_ticks.end(), te - 179, [](const MTick& k, long long t) { return k.t < t; }); it != g_ticks.end() && it->t <= te; ++it) {
        if (!any) { o = h = l = it->px; any = true; } h = std::max(h, it->px); l = std::min(l, it->px); c = it->px; }
    if (!any) o = h = l = c = 5000;
    g_bars.push_back(te); mockBar((int)g_bars.size() - 1, o, h, l, c);
}
static void tickPlugin(TapeFlow* tf, int secs)
{
    for (int i = 0; i < secs; i++) {
        g_now++;
        RTX_EVENT ev; memset(&ev, 0, sizeof(ev)); ev.v.timer.id = g_timerId; tf->timer(&ev);
        if (g_now % 180 == 0) addBar(g_now);
        if (i % 30 == 0) { g_text.clear(); static_cast<cppExtension*>(tf)->calc(0); tf->draw(); }
    }
}
// a tape with both sides, quotes, unknown-side trades (inside the spread) and several prices per second
static std::vector<MTick> tape(long long from, long long to)
{
    std::vector<MTick> v; int p = 20000;
    for (long long t = from; t < to; t += 1) {
        const long long sid = tfl::sessionOf(t);
        if (!tfl::tradingWeekday(sid)) continue;
        int hod = (int)(((t % 86400) + 86400) % 86400 / 3600);
        if (hod == 16 || t % 3) continue;
        if ((t / 3) % 5 == 0) p += ((t / 15) % 3) - 1;
        const float bid = p * 0.25f, ask = (p + 2) * 0.25f;
        v.push_back(MTick{t, ask, bid, ask, 1 + (long)((t / 3) % 4)});
        if (t % 2 == 0) v.push_back(MTick{t, bid, bid, ask, 2});
        if (t % 7 == 0) v.push_back(MTick{t, (p + 1) * 0.25f, bid, ask, 1});   // inside the spread: unknown side
    }
    return v;
}
int main()
{
    setenv("USERPROFILE", "/tmp/tfv202/home", 1); setenv("TZ", "UTC", 1); tzset();
    system("mkdir -p /tmp/tfv202 && rm -f /tmp/tfv202/home*");
    g_now = DAY0 + 18 * 3600 + 1800;                         // Thursday 18:30 - outside ES RTH
    g_ticks = tape(DAY0 - 20 * 86400, g_now + 4 * 3600);
    for (long long b = g_now - 3 * 3600; b <= g_now; b += 180) addBar(b - b % 180);
    TapeFlow* tf = static_cast<TapeFlow*>(CreateExtension()); tf->setup(); static_cast<cppExtension*>(tf)->calc(0);
    tickPlugin(tf, 8 * 60);
    const long long sid = tfl::sessionOf(g_now);
    // ---- test mode: nothing drawn; the header says so
    { g_text.clear(); tf->draw(); char t[160] = {0}; tf->parmsTitle(t, sizeof(t));
      CHECK(std::string(t) == "TF 2.0.2 ES  TESTING - not for trading yet", "test mode: the header title");
      bool onlyTitle = true; for (auto& s : g_text) if (s != t) onlyTitle = false;
      g_lines = g_rects = 0; tf->S().lastTitleCall = (long long)g_now; tf->S().titleChangedAt = (time_t)g_now; g_text.clear(); tf->draw();
      CHECK(onlyTitle && g_text.empty() && g_lines == 0 && g_rects == 0, "test mode: the pane draws nothing (the title only as the fallback line)");
      CHECK(tf->S().book && tf->S().book->book.decidedThrough != LLONG_MIN, "test mode: signals are still decided and recorded"); }
    // ---- the tape export: the last 10 trading sessions, each equal to the trades IRT has
    int files = 0; bool equal = true, barsOk = true; long long bytes = 0;
    int wk = 0;
    for (long long s = sid - 1; s >= sid - 21 && wk < 10; --s) {
        if (!tfl::tradingWeekday(s)) continue;
        ++wk;
        const std::string tp = slurp(LS + "\\TapeFlow\\ES-tape-" + std::to_string(s) + ".csv"), bp = slurp(LS + "\\TapeFlow\\ES-bars3-" + std::to_string(s) + ".csv");
        if (tp.empty()) continue;
        files++; bytes += (long long)(tp.size() + bp.size());
        // expected: aggregate the mock trades of that session
        std::map<std::pair<long long, int>, std::array<long long, 4>> want; std::map<long long, std::array<long long, 9>> wb;
        for (auto& k : g_ticks) {
            if (tfl::sessionOf(k.t) != s) continue;
            const int px = (int)llround(k.px / 0.25), bid = (int)llround(k.bid / 0.25), ask = (int)llround(k.ask / 0.25);
            const int sd = tfl::sideOf(px, bid, ask); auto& a = want[std::make_pair(k.t, px)];
            a[sd == tfl::SIDE_BUY ? 0 : sd == tfl::SIDE_SELL ? 1 : 2] += k.q; a[3]++;
            const long long te = k.t / 180 * 180 + 180; auto it = wb.find(te);
            if (it == wb.end()) { std::array<long long, 9> z = {te, px, px, px, px, 0, 0, 0, 0}; it = wb.insert(std::make_pair(te, z)).first; }
            auto& b = it->second; b[2] = std::max<long long>(b[2], px); b[3] = std::min<long long>(b[3], px); b[4] = px; b[5] += k.q; if (sd == tfl::SIDE_BUY) b[6] += k.q; else if (sd == tfl::SIDE_SELL) b[7] += k.q; b[8]++;
        }
        std::stringstream ss(tp); std::string ln; size_t got = 0;
        while (std::getline(ss, ln)) { if (ln.empty() || !isdigit((unsigned char)ln[0])) continue;
            long long t, b, se, u, n; int px; if (sscanf(ln.c_str(), "%lld|%d|%lld|%lld|%lld|%lld", &t, &px, &b, &se, &u, &n) != 6) { equal = false; continue; }
            auto it = want.find(std::make_pair(t, px)); if (it == want.end() || it->second[0] != b || it->second[1] != se || it->second[2] != u || it->second[3] != n) equal = false; got++; }
        if (got != want.size()) equal = false;
        std::stringstream sb(bp); size_t gb = 0;
        while (std::getline(sb, ln)) { if (ln.empty() || !isdigit((unsigned char)ln[0])) continue;
            long long v[9]; if (sscanf(ln.c_str(), "%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8]) != 9) { barsOk = false; continue; }
            auto it = wb.find(v[0]); if (it == wb.end()) { barsOk = false; continue; } for (int k = 0; k < 9; ++k) if (it->second[k] != v[k]) barsOk = false; gb++; }
        if (gb != wb.size()) barsOk = false;
        if (tp.compare(0, 18, "# TapeFlow 2.0.2 t") != 0 || tp.find("\nt|px_ticks|buy|sell|unknown|trades\n") == std::string::npos) equal = false;
    }
    printf("tape files %d (of the last 10 trading sessions), %.1f MB\n", files, bytes / 1048576.0);
    CHECK(files == 10, "tape export: the last 10 trading sessions (weekends skipped)");
    CHECK(equal, "tape: every second x price line equals the trades IRT has (buy / sell / unknown / trades)");
    CHECK(barsOk, "bars3: every 3-min OHLCV bar equals the trades (te = bar end)");
    std::string stat = slurp(LS + "\\TapeFlow.status-ES.txt");
    { size_t a = stat.find("TAPE_EXPORT,"); if (a != std::string::npos) printf("  %s\n", stat.substr(a, stat.find('\n', a) - a).c_str()); }
    CHECK(stat.find("TAPE_EXPORT,last session ") != std::string::npos && stat.find(" MB") != std::string::npos && stat.find("TEST_MODE,on") != std::string::npos, "status: the tape export (with size) and the test mode");
    const std::string tr = slurp(LS + "\\TapeFlow.trace-ES.txt");
    CHECK(tr.find("tape export session ") != std::string::npos, "trace: each exported session");
    static_cast<cppExtension*>(tf)->destroy();
    // ---- resumable: a session interrupted mid-export (only a .part) is redone in full; finished ones are never fetched again
    long long s1 = sid - 1; while (!tfl::tradingWeekday(s1)) --s1;
    const std::string f1 = LS + "\\TapeFlow\\ES-tape-" + std::to_string(s1) + ".csv";
    const std::string full = slurp(f1);
    std::rename(f1.c_str(), (f1 + ".part").c_str()); { std::ofstream o((f1 + ".part").c_str(), std::ios::app); o << "999|garbage\n"; }
    g_ttMaxBack = 0; g_ttCalls = 0;
    TapeFlow* r = static_cast<TapeFlow*>(CreateExtension()); r->setup(); static_cast<cppExtension*>(r)->calc(0);
    tickPlugin(r, 3 * 60);
    CHECK(slurp(f1) == full, "resume: the interrupted session is exported again in full (the stale .part is discarded)");
    std::ifstream part((f1 + ".part").c_str());
    CHECK(!part.good(), "resume: no .part left behind");
    CHECK(g_ttMaxBack < 2 * 86400 + 3600, "resume: only the missing session is fetched (the others are on file)");
    static_cast<cppExtension*>(r)->destroy();
    // ---- never inside RTH
    std::remove(f1.c_str());
    g_now = DAY0 + 86400 + 10 * 3600; g_ttMaxBack = 0;                       // Friday 10:00 CT
    for (long long b = g_bars.back() + 180; b <= g_now; b += 180) addBar(b);
    TapeFlow* q = static_cast<TapeFlow*>(CreateExtension()); q->setup(); static_cast<cppExtension*>(q)->calc(0);
    tickPlugin(q, 3 * 60);
    std::ifstream again(f1.c_str());
    CHECK(!again.good() && g_ttMaxBack < 86400, "inside RTH: no tape export (it waits for the end of RTH)");
    static_cast<cppExtension*>(q)->destroy();
    printf("%d passed, %d failed\n", npass_, fails);
    return fails ? 1 : 0;
}
