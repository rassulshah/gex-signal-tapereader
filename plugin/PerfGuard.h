#ifndef LSPERF_H
#define LSPERF_H
// PerfGuard.h 1.0.0 (2026-10-26) -- the IRT PERFORMANCE GUARD, compiled into every lsFlexLevels plugin.
// Rassul 2026-10-26 13:47 "do you have a way of optimizing irt and how it uses memory so there is no degredation" / 13:49 "ok. do it".
//
// WHAT it measures (per plugin DLL, per chart = per extension object):
//   - time of every calc / preCalc / draw (LSPERF_SCOPE at the top of the function; std::chrono::steady_clock = QPC on MSVC)
//   - the gap between calcs of a chart (IRT calls calc every few seconds on busy charts: the user-visible lag)
//   - IRT process memory once a minute (working set + private bytes via K32GetProcessMemoryInfo, looked up at run time: no psapi link)
//     and its trend (MB per hour over the last hour = the leak flag)
//   - feed lag (wall clock - the exchange stamp of the last order-feed event; only plugins with OrderFlowFeed.h; "?" when unknown)
//   - the board size (bytes of the LRA_* process environment entries every plugin shares)
//   - the time the perf file write itself took
// WHERE it goes:
//   - %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\Perf\perf-<plugin>-<yyyymmdd>.csv : one row per chart per minute (batched: ONE append a minute)
//   - the board: LRA_PERF_<plugin> = "<ms>|<minute>|M.calc_p95_ms=..;M.draw_p95_ms=..;M.ws_mb=..;..." (MarketState encoding)
//                LRA_PERF_OVER_<plugin>_<sym> = "<wallSec>|<why>" while that indicator is over budget on that market
//                LRA_PERF_MEMOVER = "<wallSec>|<why>" while the process is over its memory budget
// BUDGETS: lsFlexLevels\LRA-PerfBudget.csv "plugin,calc_ms,draw_ms,mem_mb" ('*' = default row; defaults calc 20 / draw 30 / mem 0 = observe).
//   Over = the minute p95 of calc or draw above budget 3 minutes in a row; cleared after 10 good minutes in a row.
//   While over, lsperf::smartAllowed(plugin, sym) is false: the SMART features (Market Awareness links, self-awareness, ask) of that
//   indicator on that market switch OFF; its basic function keeps running. Every switch is an EVENT row in the perf file.
// SAFETY: never throws into IRT (every entry point is guarded; a fault turns the guard off for the DLL); no thread, no timer; the
//   chart table is capped (64) and stale charts are dropped after 10 minutes; nothing here changes what an indicator computes.
// The state is per DLL (function static, mutex) and keyed by the extension pointer: deliberate, documented exception to BUILD-LESSONS #1
//   (it is instrumentation, not indicator state). Linux tests: -DLSPERF_FAKE (clocks, memory and the environment are injectable).
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <sys/stat.h>
#if defined(_WIN32) && !defined(LSPERF_FAKE)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#define LSPERF_WIN 1
#else
#include <sys/types.h>
#include <unistd.h>
#endif

namespace lsperf {

static const char* const VERSION = "1.0.0";
static const int NBK = 24;
static const double EDGE_US[NBK] = { 50, 100, 200, 500, 1000, 2000, 3000, 5000, 7500, 10000, 15000, 20000, 30000, 50000, 75000,
                                     100000, 150000, 250000, 500000, 1e6, 2e6, 5e6, 1e7, 1e12 };
static const size_t MAX_CHARTS = 64;
static const int OVER_AFTER = 3, CLEAR_AFTER = 10;
static const long long STALE_MS = 600000, INFO_MS = 60000, BUDGET_CHECK_MS = 60000, FLAG_FRESH_S = 900;
static const size_t MAX_PENDING = 4000, MAX_BUDGET_BYTES = 16384;
static const double DEF_CALC_MS = 20, DEF_DRAW_MS = 30, DEF_MEM_MB = 0;

// ---------------------------------------------------------------- clocks (injectable in fake mode)
#ifdef LSPERF_FAKE
inline long long& fakeMs() { static long long v = 1000; return v; }
inline long long& fakeWall() { static long long v = 1793030880; return v; }      // 2026-10-26 ~11:08 CT
inline int& fakeTzMin() { static int v = -300; return v; }                         // CDT
inline double& fakeWsMb() { static double v = 1000; return v; }
inline double& fakePrivMb() { static double v = 1200; return v; }
inline long long steadyMs() { return fakeMs(); }
inline long long wallSec() { return fakeWall(); }
inline int tzOffsetMin() { return fakeTzMin(); }
#else
inline long long steadyMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
inline long long wallSec() { return (long long)std::time(nullptr); }
inline int tzOffsetMin() {
    const std::time_t t = std::time(nullptr); std::tm lt{}, gt{};
#ifdef LSPERF_WIN
    localtime_s(&lt, &t); gmtime_s(&gt, &t);
#else
    localtime_r(&t, &lt); gmtime_r(&t, &gt);
#endif
    long long d = (long long)(lt.tm_hour - gt.tm_hour) * 60 + (lt.tm_min - gt.tm_min);
    const int dd = lt.tm_yday - gt.tm_yday; if (dd == 1 || dd < -1) d += 1440; else if (dd == -1 || dd > 1) d -= 1440;
    return (int)d;
}
#endif
// local wall-clock civil fields of a UTC second (via the offset: one rule on both platforms, testable)
inline void civil(long long utc, int& y, int& mo, int& d, int& h, int& mi) {
    long long s = utc + (long long)tzOffsetMin() * 60; long long z = (s >= 0 ? s / 86400 : (s - 86399) / 86400); const long long sod = s - z * 86400;
    h = (int)(sod / 3600); mi = (int)((sod % 3600) / 60);
    z += 719468; const long long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153; d = (int)(doy - (153 * mp + 2) / 5 + 1); mo = (int)(mp < 10 ? mp + 3 : mp - 9); y = (int)(yoe + era * 400 + (mo <= 2));
}
inline std::string dayStr(long long utc) { int y, mo, d, h, mi; civil(utc, y, mo, d, h, mi); char b[16]; std::snprintf(b, sizeof(b), "%04d%02d%02d", y, mo, d); return b; }
inline std::string stampStr(long long utc) { int y, mo, d, h, mi; civil(utc, y, mo, d, h, mi); char b[24]; std::snprintf(b, sizeof(b), "%04d-%02d-%02d %02d:%02d", y, mo, d, h, mi); return b; }

// ---------------------------------------------------------------- small helpers
inline std::string fmt(double x, int prec = 3) { if (!(x == x) || x < 0) return "?"; char b[32]; std::snprintf(b, sizeof(b), "%.*g", prec, x); return b; }
inline std::string clean(const char* s, size_t cap = 24) {
    std::string o; if (!s) return o;
    for (const char* p = s; *p && o.size() < cap; ++p) { const char c = *p; o += ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') ? c : '_'; }
    return o;
}
inline bool strictD(const std::string& t, double& v) {
    if (t.empty() || t.size() > 24) return false;
    char* e = nullptr; const double x = std::strtod(t.c_str(), &e);
    if (!e || *e != 0 || !(x == x) || x < 0 || x > 1e7) return false;
    v = x; return true;
}
inline std::string envGet(const std::string& n) {
#ifdef LSPERF_WIN
    char b[256]; const DWORD k = GetEnvironmentVariableA(n.c_str(), b, (DWORD)sizeof(b)); return (k > 0 && k < sizeof(b)) ? std::string(b, (size_t)k) : std::string();
#else
    const char* s = std::getenv(n.c_str()); return s ? std::string(s) : std::string();
#endif
}
inline void envSet(const std::string& n, const std::string& v) {
#ifdef LSPERF_WIN
    SetEnvironmentVariableA(n.c_str(), v.empty() ? NULL : v.c_str());
#else
    if (v.empty()) unsetenv(n.c_str()); else setenv(n.c_str(), v.c_str(), 1);
#endif
}
inline std::string flexDir() {
#ifdef LSPERF_WIN
    const std::string up = envGet("USERPROFILE"); return up.empty() ? std::string() : up + "\\InvestorRT\\rtx\\lsFlexLevels";
#else
    return envGet("LRA_FLEX_DIR");
#endif
}
inline std::string sep() {
#ifdef LSPERF_WIN
    return "\\";
#else
    return "/";
#endif
}
inline void makeDir(const std::string& p) {
#ifdef LSPERF_WIN
    CreateDirectoryA(p.c_str(), NULL);
#else
    ::mkdir(p.c_str(), 0755);
#endif
}
inline long long fileStamp(const std::string& p) {
    struct stat s; if (::stat(p.c_str(), &s) != 0) return -1; return (long long)s.st_mtime * 1000 + (long long)s.st_size % 1000;
}

// ---------------------------------------------------------------- process memory and board size
struct Mem { double wsMb = -1, privMb = -1; };
inline Mem memNow() {
    Mem m;
#ifdef LSPERF_WIN
    struct PMC { DWORD cb; DWORD PageFaultCount; SIZE_T PeakWorkingSetSize, WorkingSetSize, QuotaPeakPagedPoolUsage, QuotaPagedPoolUsage,
                 QuotaPeakNonPagedPoolUsage, QuotaNonPagedPoolUsage, PagefileUsage, PeakPagefileUsage, PrivateUsage; };
    typedef int (WINAPI *Fn)(HANDLE, void*, DWORD);
    static Fn fn = (Fn)(void*)GetProcAddress(GetModuleHandleA("kernel32.dll"), "K32GetProcessMemoryInfo");
    if (fn) { PMC c; std::memset(&c, 0, sizeof(c)); c.cb = (DWORD)sizeof(c);
        if (fn(GetCurrentProcess(), &c, c.cb)) { m.wsMb = (double)c.WorkingSetSize / 1048576.0; m.privMb = (double)c.PrivateUsage / 1048576.0; } }
#elif defined(LSPERF_FAKE)
    m.wsMb = fakeWsMb(); m.privMb = fakePrivMb();
#endif
    return m;
}
struct Board { long long bytes = 0, entries = 0; };
inline Board boardNow() {
    Board b;
#ifdef LSPERF_WIN
    char* env = GetEnvironmentStringsA(); if (!env) return b;
    for (const char* p = env; *p; ) { const size_t n = std::strlen(p); if (n > 4 && std::strncmp(p, "LRA_", 4) == 0) { b.bytes += (long long)n; b.entries++; } p += n + 1; }
    FreeEnvironmentStringsA(env);
#else
    for (char** e = environ; e && *e; ++e) { const size_t n = std::strlen(*e); if (n > 4 && std::strncmp(*e, "LRA_", 4) == 0) { b.bytes += (long long)n; b.entries++; } }
#endif
    return b;
}

// ---------------------------------------------------------------- per-minute accumulators
struct Acc {
    long long n = 0; double sum = 0, mx = 0; unsigned hist[NBK];
    Acc() { clear(); }
    void clear() { n = 0; sum = 0; mx = 0; for (int i = 0; i < NBK; ++i) hist[i] = 0; }
    void add(double us) { if (!(us == us) || us < 0) return; n++; sum += us; if (us > mx) mx = us; int k = 0; while (k < NBK - 1 && us > EDGE_US[k]) ++k; hist[k]++; }
    double meanMs() const { return n ? sum / (double)n / 1000.0 : -1; }
    double maxMs() const { return n ? mx / 1000.0 : -1; }
    // p95 = the upper edge of the bucket holding the 95th percentile, never above the observed max (conservative, cheap)
    double p95Ms() const {
        if (!n) return -1;
        const long long need = (n * 95 + 99) / 100; long long c = 0;
        for (int k = 0; k < NBK; ++k) { c += hist[k]; if (c >= need) { const double e = EDGE_US[k] < mx ? EDGE_US[k] : mx; return e / 1000.0; } }
        return mx / 1000.0;
    }
};
struct Budget { double calcMs = DEF_CALC_MS, drawMs = DEF_DRAW_MS, memMb = DEF_MEM_MB; };
struct Chart {
    std::string sym; long spb = -1; long bars = -1; long long infoMs = -1, lastSeenMs = -1, lastCalcMs = -1;
    Acc calc, draw, gap;
    long long totCalc = 0, totDraw = 0;
    int overRun = 0, okRun = 0; bool over = false; std::string why;
};
struct State {
    std::mutex m;
    bool dead = false, off = false;
    std::string plugin;
    std::map<const void*, Chart> charts;
    long long minute = -1;                         // current wall minute (UTC seconds / 60)
    std::vector<std::string> pending;
    std::map<std::string, Budget> budgets; long long budgetStamp = -2, budgetCheckedMs = -1;
    std::vector<std::pair<long long, double> > memHist;   // (wall sec, private MB), last 60 minutes
    double lastWriteMs = -1; long long writes = 0, writeFails = 0;
    bool memOver = false; int memOverRun = 0, memOkRun = 0;
    std::atomic<long long> evStamp{0};
    long long faults = 0;
};
inline State& st() { static State s; return s; }

// ---------------------------------------------------------------- budgets
inline std::string budgetPath() { const std::string d = flexDir(); return d.empty() ? std::string() : d + sep() + "LRA-PerfBudget.csv"; }
// parse "plugin,calc_ms,draw_ms,mem_mb" lines; bad lines are skipped (strict numbers, '?' or empty = keep the default)
inline std::map<std::string, Budget> parseBudgets(const std::string& text) {
    std::map<std::string, Budget> out; size_t a = 0; int lines = 0;
    while (a < text.size() && lines < 400) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        std::string ln = text.substr(a, e - a); a = e + 1; lines++;
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        if (ln.empty() || ln[0] == '#') continue;
        std::vector<std::string> f; size_t b = 0;
        while (f.size() < 5) { size_t c = ln.find(',', b); f.push_back(ln.substr(b, c == std::string::npos ? std::string::npos : c - b)); if (c == std::string::npos) break; b = c + 1; }
        if (f.size() < 2 || f[0].empty() || f[0] == "plugin") continue;
        Budget B = out.count("*") ? out["*"] : Budget(); double v;
        if (f.size() > 1 && strictD(f[1], v) && v > 0) B.calcMs = v;
        if (f.size() > 2 && strictD(f[2], v) && v > 0) B.drawMs = v;
        if (f.size() > 3 && strictD(f[3], v)) B.memMb = v;
        out[f[0]] = B;
    }
    return out;
}
inline void refreshBudgets(State& S, long long nowMs) {
    if (S.budgetCheckedMs >= 0 && nowMs >= S.budgetCheckedMs && nowMs - S.budgetCheckedMs < BUDGET_CHECK_MS) return;
    S.budgetCheckedMs = nowMs;
    const std::string p = budgetPath(); const long long stamp = p.empty() ? -1 : fileStamp(p);
    if (stamp == S.budgetStamp) return;
    S.budgetStamp = stamp; S.budgets.clear();
    if (stamp < 0) return;
    std::FILE* f = std::fopen(p.c_str(), "rb"); if (!f) return;
    std::string t; char buf[4096]; size_t k;
    while ((k = std::fread(buf, 1, sizeof(buf), f)) > 0 && t.size() < MAX_BUDGET_BYTES) t.append(buf, k);
    std::fclose(f);
    if (t.size() > MAX_BUDGET_BYTES) t.resize(MAX_BUDGET_BYTES);
    S.budgets = parseBudgets(t);
}
inline Budget budgetOf(const State& S) {
    auto it = S.budgets.find(S.plugin); if (it != S.budgets.end()) return it->second;
    it = S.budgets.find("*"); return it != S.budgets.end() ? it->second : Budget();
}

// ---------------------------------------------------------------- feed lag (OrderFlowFeed.h calls noteEvent per event: one relaxed store)
inline void noteEvent(long long stampSec) { if (stampSec > 0) st().evStamp.store(stampSec, std::memory_order_relaxed); }
// the stamp may be UTC epoch seconds or local wall-clock seconds: take the reading that is plausible (-5 s .. 1 h), else unknown (-1)
inline double lagSec(long long stamp, long long nowUtc) {
    if (stamp <= 0) return -1;
    const long long a = nowUtc - stamp, b = nowUtc + (long long)tzOffsetMin() * 60 - stamp;
    if (a >= -5 && a <= 3600) return a < 0 ? 0 : (double)a;
    if (b >= -5 && b <= 3600) return b < 0 ? 0 : (double)b;
    return -1;
}

// ---------------------------------------------------------------- the leak trend: least-squares MB per hour over the kept samples
inline double slopeMbH(const std::vector<std::pair<long long, double> >& h) {
    if (h.size() < 30) return -1e9;                // unknown: fewer than 30 minutes
    double sx = 0, sy = 0, sxx = 0, sxy = 0; const double n = (double)h.size(), x0 = (double)h[0].first;
    for (const auto& p : h) { const double x = ((double)p.first - x0) / 3600.0; sx += x; sy += p.second; sxx += x * x; sxy += x * p.second; }
    const double d = n * sxx - sx * sx; if (d <= 0) return -1e9;
    return (n * sxy - sx * sy) / d;
}
inline std::string slopeStr(double s) { if (s < -1e8) return "?"; char b[32]; std::snprintf(b, sizeof(b), "%.1f", s); return b; }

// ---------------------------------------------------------------- the file + the board
inline const char* header() {
    return "ts_ct,kind,plugin,sym,spb,chart,bars,calc_n,calc_mean_ms,calc_p95_ms,calc_max_ms,draw_n,draw_mean_ms,draw_p95_ms,draw_max_ms,"
           "gap_p95_s,ws_mb,priv_mb,mem_slope_mb_h,board_kb,board_n,lag_s,over,budget_calc_ms,budget_draw_ms,write_ms,note,v";
}
inline std::string perfPath(const std::string& plugin, long long utc) {
    const std::string d = flexDir(); if (d.empty()) return std::string();
    return d + sep() + "Perf" + sep() + "perf-" + plugin + "-" + dayStr(utc) + ".csv";
}
inline bool appendLines(State& S, long long utc) {
    if (S.pending.empty()) return true;
    const std::string p = perfPath(S.plugin, utc); if (p.empty()) { S.pending.clear(); return false; }
    const long long t0 = steadyMs();
    makeDir(flexDir() + sep() + "Perf");
    const bool fresh = fileStamp(p) < 0;
    std::FILE* f = std::fopen(p.c_str(), "ab");
    if (!f) { S.writeFails++; if (S.pending.size() > MAX_PENDING) S.pending.clear(); return false; }   // keep the rows for the next minute
    std::string blob; if (fresh) { blob += header(); blob += "\r\n"; }
    for (const std::string& l : S.pending) { blob += l; blob += "\r\n"; }
    const bool ok = std::fwrite(blob.data(), 1, blob.size(), f) == blob.size(); const bool closed = std::fclose(f) == 0;
    if (ok && closed) { S.pending.clear(); S.writes++; } else S.writeFails++;
    if (S.pending.size() > MAX_PENDING) S.pending.clear();
    S.lastWriteMs = (double)(steadyMs() - t0);
    return ok && closed;
}
inline std::string chartId(const void* p) { char b[24]; std::snprintf(b, sizeof(b), "%llx", (unsigned long long)(((size_t)p >> 4) & 0xFFFFFF)); return b; }
inline std::string overKey(const std::string& plugin, const std::string& sym) { return "LRA_PERF_OVER_" + clean(plugin.c_str()) + "_" + clean(sym.c_str()); }

// close the minute: one row per active chart, budget judgement, memory, board facts, one batched write
inline void closeMinute(State& S, long long utcNow) {
    const long long minuteUtc = S.minute * 60; const std::string ts = stampStr(minuteUtc);
    const Mem mem = memNow(); const Board bd = boardNow(); const Budget B = budgetOf(S);
    if (mem.privMb >= 0) { S.memHist.push_back(std::make_pair(minuteUtc, mem.privMb)); if (S.memHist.size() > 60) S.memHist.erase(S.memHist.begin()); }
    const double slope = slopeMbH(S.memHist);
    const double lag = lagSec(S.evStamp.load(std::memory_order_relaxed), utcNow);
    double worstCalc = -1, worstDraw = -1, worstGap = -1; long long calls = 0; std::string overList;
    for (auto& kv : S.charts) {
        Chart& c = kv.second;
        if (c.calc.n == 0 && c.draw.n == 0) continue;
        const double cp = c.calc.p95Ms(), dp = c.draw.p95Ms(), gp = c.gap.n ? c.gap.p95Ms() : -1;   // gap acc holds ms*1000 -> p95Ms = seconds
        if (cp > worstCalc) worstCalc = cp;
        if (dp > worstDraw) worstDraw = dp;
        if (gp > worstGap) worstGap = gp;
        calls += c.calc.n + c.draw.n;
        // budget: a minute is over when its calc or draw p95 is above budget
        const bool bad = (cp > B.calcMs) || (dp > B.drawMs);
        std::string note;
        if (bad) { c.overRun++; c.okRun = 0; } else { c.okRun++; c.overRun = 0; }
        if (!c.over && c.overRun >= OVER_AFTER) {
            c.over = true; c.why = cp > B.calcMs ? "calc p95 " + fmt(cp) + "ms > " + fmt(B.calcMs) : "draw p95 " + fmt(dp) + "ms > " + fmt(B.drawMs);
            note = "OVER " + c.why + " (smart features off)";
        } else if (c.over && c.okRun >= CLEAR_AFTER) { c.over = false; note = "CLEAR after " + std::to_string(CLEAR_AFTER) + " good minutes (smart features back on)"; c.why.clear(); }
        if (c.over) { envSet(overKey(S.plugin, c.sym), std::to_string(utcNow) + "|" + c.why); overList += (overList.empty() ? "" : " ") + clean(c.sym.c_str()); }
        else if (!note.empty()) envSet(overKey(S.plugin, c.sym), std::string());
        std::string row = ts + "," + (note.empty() ? "M" : "EVENT") + "," + S.plugin + "," + clean(c.sym.c_str()) + "," + std::to_string(c.spb) + "," + chartId(kv.first)
            + "," + std::to_string(c.bars) + "," + std::to_string(c.calc.n) + "," + fmt(c.calc.meanMs()) + "," + fmt(cp) + "," + fmt(c.calc.maxMs())
            + "," + std::to_string(c.draw.n) + "," + fmt(c.draw.meanMs()) + "," + fmt(dp) + "," + fmt(c.draw.maxMs())
            + "," + fmt(gp) + "," + fmt(mem.wsMb, 6) + "," + fmt(mem.privMb, 6) + "," + slopeStr(slope) + "," + fmt((double)bd.bytes / 1024.0, 5) + "," + std::to_string(bd.entries)
            + "," + fmt(lag) + "," + (c.over ? "1" : "0") + "," + fmt(B.calcMs) + "," + fmt(B.drawMs) + "," + fmt(S.lastWriteMs) + "," + note + "," + VERSION;
        if (S.pending.size() < MAX_PENDING) S.pending.push_back(row);
        c.calc.clear(); c.draw.clear(); c.gap.clear();
    }
    // memory budget (process-wide): private bytes above budget 3 minutes running -> LRA_PERF_MEMOVER; 10 good minutes clear it
    if (B.memMb > 0 && mem.privMb >= 0) {
        if (mem.privMb > B.memMb) { S.memOverRun++; S.memOkRun = 0; } else { S.memOkRun++; S.memOverRun = 0; }
        std::string note;
        if (!S.memOver && S.memOverRun >= OVER_AFTER) { S.memOver = true; note = "MEMOVER private " + fmt(mem.privMb, 6) + "MB > " + fmt(B.memMb, 6); }
        else if (S.memOver && S.memOkRun >= CLEAR_AFTER) { S.memOver = false; note = "MEMCLEAR"; envSet("LRA_PERF_MEMOVER", std::string()); }
        if (S.memOver) envSet("LRA_PERF_MEMOVER", std::to_string(utcNow) + "|private " + fmt(mem.privMb, 6) + "MB > " + fmt(B.memMb, 6));
        if (!note.empty() && S.pending.size() < MAX_PENDING)
            S.pending.push_back(ts + ",EVENT," + S.plugin + ",*,-1,-,-1,0,?,?,?,0,?,?,?,?," + fmt(mem.wsMb, 6) + "," + fmt(mem.privMb, 6) + "," + slopeStr(slope)
                                + "," + fmt((double)bd.bytes / 1024.0, 5) + "," + std::to_string(bd.entries) + "," + fmt(lag) + ",0,?,?," + fmt(S.lastWriteMs) + "," + note + "," + VERSION);
    }
    // board summary (MarketState encoding: "<ms>|<minute>|k=v;...")
    std::string v = std::to_string(steadyMs()) + "|" + std::to_string(S.minute) + "|M.calc_p95_ms=" + fmt(worstCalc) + ";M.draw_p95_ms=" + fmt(worstDraw)
        + ";M.gap_p95_s=" + fmt(worstGap) + ";M.calls_min=" + std::to_string(calls) + ";M.ws_mb=" + fmt(mem.wsMb, 6) + ";M.priv_mb=" + fmt(mem.privMb, 6)
        + ";M.mem_slope_mb_h=" + slopeStr(slope) + ";M.board_kb=" + fmt((double)bd.bytes / 1024.0, 5) + ";M.lag_s=" + fmt(lag) + ";M.write_ms=" + fmt(S.lastWriteMs)
        + ";over=" + overList + ";v=" + VERSION + ";";
    envSet("LRA_PERF_" + clean(S.plugin.c_str()), v);
    appendLines(S, utcNow);
    // drop charts not seen for 10 minutes (removed / pointer re-used)
    const long long now = steadyMs();
    for (auto it = S.charts.begin(); it != S.charts.end(); ) {
        if (it->second.lastSeenMs >= 0 && now - it->second.lastSeenMs > STALE_MS) { if (it->second.over) envSet(overKey(S.plugin, it->second.sym), std::string()); it = S.charts.erase(it); }
        else ++it;
    }
}

// ---------------------------------------------------------------- the one recording entry point (the Scope calls this; tests call it directly)
// kind: 'c' calc, 'p' preCalc (timed with calc, no gap), 'd' draw
inline void record(const char* plugin, char kind, const void* self, double us, const std::string* sym, long spb, long bars) {
    State& S = st();
    std::lock_guard<std::mutex> g(S.m);
    if (S.dead || S.off) return;
    try {
        if (S.plugin.empty()) { S.plugin = clean(plugin, 40); if (envGet("LRA_PERF_DISABLE") == "1") { S.off = true; return; } }
        const long long now = steadyMs(), utc = wallSec(), minute = utc >= 0 ? utc / 60 : (utc - 59) / 60;
        refreshBudgets(S, now);
        if (S.minute < 0) S.minute = minute;
        if (minute != S.minute) { closeMinute(S, utc); S.minute = minute; }
        auto it = S.charts.find(self);
        if (it == S.charts.end()) { if (S.charts.size() >= MAX_CHARTS) return; it = S.charts.insert(std::make_pair(self, Chart())).first; }
        Chart& c = it->second; c.lastSeenMs = now;
        if (sym) { c.sym = *sym; c.spb = spb; c.bars = bars; c.infoMs = now; }
        if (kind == 'd') { c.draw.add(us); c.totDraw++; }
        else {
            c.calc.add(us); c.totCalc++;
            if (kind == 'p') return;
            if (c.lastCalcMs >= 0 && now >= c.lastCalcMs) c.gap.add((double)(now - c.lastCalcMs));   // ms stored in the us slot: p95Ms() = seconds
            c.lastCalcMs = now;
        }
    } catch (...) { S.faults++; S.dead = true; }
}
// does this chart need its symbol / period / bar count refreshed? (cheap check before calling the SDK getters)
inline bool needInfo(const void* self) {
    State& S = st(); std::lock_guard<std::mutex> g(S.m);
    auto it = S.charts.find(self); const long long now = steadyMs();
    return it == S.charts.end() || it->second.infoMs < 0 || now - it->second.infoMs >= INFO_MS || now < it->second.infoMs;
}

// the SDK getters, if the class has them (cppExtension has all three; a test mock may not: then "?" / -1)
inline size_t lenCap(const char* s, size_t n) { size_t k = 0; while (k < n && s[k]) ++k; return k; }
template <class T> auto symOf(T* t, int) -> decltype((void)t->getSymbol(), std::string()) { const char* s = t->getSymbol(); return s ? std::string(s, lenCap(s, 40)) : std::string(); }
template <class T> std::string symOf(T*, long) { return std::string(); }
template <class T> auto spbOf(T* t, int) -> decltype((void)t->getSecondsPerBar(), 0L) { return (long)t->getSecondsPerBar(); }
template <class T> long spbOf(T*, long) { return -1; }
template <class T> auto barsOf(T* t, int) -> decltype((void)t->getBarCount(), 0L) { return (long)t->getBarCount(); }
template <class T> long barsOf(T*, long) { return -1; }

// RAII timer. Works with any class (cppExtension, or a test mock without the getters).
class Scope {
public:
    template <class T> Scope(const char* plugin, const char* fn, T* self) : plugin_(plugin), self_((const void*)self), kind_('c'), has_(false), spb_(-1), bars_(-1) {
        try {
            if (fn && fn[0] == 'd') kind_ = 'd'; else if (fn && fn[0] == 'p') kind_ = 'p';
            if (self && needInfo(self_)) {
                sym_ = symOf(self, 0); spb_ = spbOf(self, 0); bars_ = barsOf(self, 0); has_ = true;
            }
        } catch (...) { has_ = false; }
        t0_ = std::chrono::steady_clock::now();
    }
    ~Scope() {
        try {
            const double us = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0_).count();
            record(plugin_, kind_, self_, us, has_ ? &sym_ : nullptr, spb_, bars_);
        } catch (...) {}
    }
private:
    const char* plugin_; const void* self_; char kind_; bool has_; long spb_, bars_; std::string sym_;
    std::chrono::steady_clock::time_point t0_;
    Scope(const Scope&); Scope& operator=(const Scope&);
};

// ---------------------------------------------------------------- the switch the smart layers ask before their extra work
// false = this indicator is over its time budget on this market, or IRT is over its memory budget: skip the SMART features
// (Market Awareness links, self-awareness, ask); keep the basic function. A flag older than 15 minutes is ignored (its writer stopped).
inline bool flagFresh(const std::string& v, long long utc) {
    if (v.empty()) return false;
    const size_t c = v.find('|'); char* e = nullptr;
    const std::string head = v.substr(0, c);                        // kept alive while e points into it
    const long long t = std::strtoll(head.c_str(), &e, 10);
    return e && *e == 0 && t > 0 && utc - t >= -60 && utc - t <= FLAG_FRESH_S;
}
inline bool smartAllowed(const std::string& plugin, const std::string& sym) {
    try {
        const long long utc = wallSec();
        if (flagFresh(envGet("LRA_PERF_MEMOVER"), utc)) return false;
        return !flagFresh(envGet(overKey(plugin, sym)), utc);
    } catch (...) { return true; }
}
// for the status lines: "perf ok" / "perf OVER calc p95 31ms > 20 (smart off)"
inline std::string statusOf(const void* self) {
    State& S = st(); std::lock_guard<std::mutex> g(S.m);
    auto it = S.charts.find(self); if (it == S.charts.end()) return "perf ?";
    return it->second.over ? "perf OVER " + it->second.why + " (smart off)" : "perf ok";
}
// test hooks
inline void resetForTest() { State& S = st(); std::lock_guard<std::mutex> g(S.m); S.dead = S.off = false; S.plugin.clear(); S.charts.clear(); S.minute = -1; S.pending.clear();
    S.budgets.clear(); S.budgetStamp = -2; S.budgetCheckedMs = -1; S.memHist.clear(); S.lastWriteMs = -1; S.writes = S.writeFails = 0; S.memOver = false; S.memOverRun = S.memOkRun = 0; S.evStamp.store(0); S.faults = 0; }

}   // namespace lsperf

#define LSPERF_CAT2(a, b) a##b
#define LSPERF_CAT(a, b) LSPERF_CAT2(a, b)
#define LSPERF_SCOPE(plugin, fn) lsperf::Scope LSPERF_CAT(_lsperf_scope_, __LINE__)(plugin, fn, this)

#endif   // LSPERF_H
