/*******************************************************************************
 * 2.0.2 (2026-10-14, Rassul approved the external-audit display list at 18:25 CT, section 13): RETIRED - the marks are no longer drawn
 * (still decided, journalled and counted); the lines and the header stay.
 * LRA Exhaustion V2 native adapter (research only) -- 2.0.1 (2.0.0 from the outside review, its own indicator lsExhaustionV2 since 2026-10-12; lsExhaustion keeps the reversal read)
 *
 * This adapter deliberately has no legacy engine code or legacy normalization
 * fallback.  It sends completed chart bars to ex2::Engine and renders only
 * values/events already cached by calc().  See WRAPPER_NOTES.md for the SDK
 * integration boundary, civil-time/DST limitation, and calibration contract.
 ******************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "ExhaustionV2"
#endif
#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG: the settings window never faults, never opens blank
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "ExhaustionV2Logic.h"       // ex2 core supplied with this v2 wrapper
#include "HostSlot.h"              // existing host integration dependency; not vendored here

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#define EX2_VERSION "2.0.3"   // 2.0.2 (2026-10-14 audit display list) marks hidden - retired, log only;   // 2.0.1 (LRA integration 2026-10-11): real barVolume array, 2% VAP/bar-volume tolerance, frontier resync by bar time, his header format

static const COLOR C_BUY_FADE = 0x00EF4444; // red: buyers fading at a high
static const COLOR C_SELL_FADE = 0x0022C55E; // green: sellers fading at a low
static const COLOR C_GREY = 0x009CA3AF;
static const COLOR C_AXIS = 0x00334155;
static const int EX2_BAR_SECONDS = 180;
static const long EX2_BARS_MAX = 1500;
static const int EX2_SESSION_START_MINUTE = 17 * 60;
static std::mutex g_fileMutex;

static bool finiteD(double v) { return std::isfinite(v) != 0; }
static double nanD() { return std::numeric_limits<double>::quiet_NaN(); }
static long long steadyMs() {
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
static short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
static std::string flexDir() {
    const char* up = std::getenv("USERPROFILE");
    return (up && up[0]) ? std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string();
}
static void makeDir(const std::string& p) {
#if defined(_WIN32)
    if (!p.empty()) CreateDirectoryA(p.c_str(), NULL);
#else
    (void)p;
#endif
}
static bool statFile(const std::string& p) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return false;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
#endif
    return (st.st_mode & S_IFMT) == S_IFREG;
}
static std::string trim(const std::string& in) {
    size_t a = 0, b = in.size();
    while (a < b && std::isspace((unsigned char)in[a])) ++a;
    while (b > a && std::isspace((unsigned char)in[b - 1])) --b;
    return in.substr(a, b - a);
}
static std::string upper(std::string s) {
    for (size_t i = 0; i < s.size(); ++i) s[i] = (char)std::toupper((unsigned char)s[i]);
    return s;
}
static bool parseD(const std::string& text, double& out) {
    const std::string s = trim(text);
    if (s.empty() || s.size() > 80) return false;
    errno = 0; char* end = 0; const double v = std::strtod(s.c_str(), &end);
    if (errno || end != s.c_str() + s.size() || !finiteD(v)) return false;
    out = v; return true;
}
static bool parseI(const std::string& text, int& out) {
    const std::string s = trim(text);
    if (s.empty() || s.size() > 32) return false;
    errno = 0; char* end = 0; const long v = std::strtol(s.c_str(), &end, 10);
    if (errno || end != s.c_str() + s.size() || v < INT_MIN || v > INT_MAX) return false;
    out = (int)v; return true;
}
static bool safeField(const std::string& s, size_t maxLen) {
    if (s.empty() || s.size() > maxLen) return false;
    for (size_t i = 0; i < s.size(); ++i) {
        const unsigned char c = (unsigned char)s[i];
        if (c < 32 || c == ',' || c == '"' || c == '\\' || c == '|') return false;
    }
    return true;
}
static std::string safeFilePart(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        const unsigned char c = (unsigned char)s[i];
        out += (std::isalnum(c) || c == '-' || c == '_') ? (char)c : '_';
    }
    return out.empty() ? "unknown" : out;
}
static std::vector<std::string> csv(const std::string& line, bool& ok) {
    std::vector<std::string> out; std::string field; bool quoted = false; ok = true;
    for (size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (quoted) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') { field += c; ++i; }
                else quoted = false;
            } else field += c;
        } else if (c == ',') { out.push_back(field); field.clear(); }
        else if (c == '"') { if (!field.empty()) { ok = false; return out; } quoted = true; }
        else field += c;
    }
    if (quoted) { ok = false; return out; }
    out.push_back(field); return out;
}
static long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
static void civilFromDays(long long z, int& y, unsigned& m, unsigned& d) {
    z += 719468;
    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = (int)yoe + (int)era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp + (mp < 10 ? 3 : -9);
    y += (m <= 2);
}
static std::string dayLabel(long long days) {
    int y = 0; unsigned m = 0, d = 0; civilFromDays(days, y, m, d);
    char b[16]; std::snprintf(b, sizeof(b), "%04d-%02u-%02u", y, m, d); return b;
}
static bool parseDay(const std::string& s, long long& days) {
    int y = 0, m = 0, d = 0; char extra = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%d%c", &y, &m, &d, &extra) != 3 || m < 1 || m > 12 || d < 1 || d > 31) return false;
    const long long candidate = civilDays(y, (unsigned)m, (unsigned)d);
    if (dayLabel(candidate) != s) return false;
    days = candidate; return true;
}

struct CivilBar {
    long long raw = 0;             // raw RTDATE key; never treated as Unix/UTC here
    long long endCivil = 0;        // local civil seconds only
    long long startCivil = 0;      // end label minus exactly 180 civil seconds
    long long sessionDays = 0;     // America/Chicago civil trading-date label
    int slotSeconds = -1;          // seconds from local 17:00 session start
    bool rth = false;
    bool valid = false;
};

// The host's getLocaltime is intentionally used only for chart-local civil
// labels.  It is not an UTC conversion.  The fallback repeated hour is rejected
// below because this SDK boundary does not expose an offset/fold identifier.
static bool makeCivilBar(cppExtension& host, RTDATE raw, int barSeconds, int rthOpen, int rthClose, CivilBar& out) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!host.getLocaltime(raw, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31 ||
        t.tm_hour < 0 || t.tm_hour > 23 || t.tm_min < 0 || t.tm_min > 59 || t.tm_sec < 0 || t.tm_sec > 59) return false;
    const long long day = civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday);
    out.raw = (long long)raw;
    out.endCivil = day * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
    out.startCivil = out.endCivil - barSeconds;
    const long long startDay = out.startCivil >= 0 ? out.startCivil / 86400LL : (out.startCivil - 86399LL) / 86400LL;
    const int startSecond = (int)(out.startCivil - startDay * 86400LL);
    const int endSecond = (int)(out.endCivil - day * 86400LL);
    const int startMinute = startSecond / 60;
    const int endMinute = endSecond / 60;
    out.sessionDays = startDay + (startMinute >= EX2_SESSION_START_MINUTE ? 1 : 0);
    out.slotSeconds = (startSecond - EX2_SESSION_START_MINUTE * 60 + 86400) % 86400;
    out.rth = startDay == day && startMinute >= rthOpen && endMinute <= rthClose;
    out.valid = true;
    return true;
}
static std::string civilLabel(long long civilSeconds) {
    const long long days = civilSeconds >= 0 ? civilSeconds / 86400LL : (civilSeconds - 86399LL) / 86400LL;
    const int sec = (int)(civilSeconds - days * 86400LL);
    char b[32]; std::snprintf(b, sizeof(b), "%sT%02d:%02d:%02d", dayLabel(days).c_str(), sec / 3600, (sec / 60) % 60, sec % 60);
    return b;
}

struct MarketSpec { const char* market; double tick; int rthOpen; int rthClose; };
static bool marketSpec(const std::string& rawRoot, const std::string& rawContract, MarketSpec& spec) {
    const std::string root = upper(trim(rawRoot));
    const std::string contract = upper(trim(rawContract));
    // Reject explicit micro roots/contracts rather than folding them into full-size research.
    if (root == "MES" || root == "MNQ" || root == "MCL" || root == "MGC" || root == "MNG" || root == "MHG" || root == "M6E" ||
        contract.compare(0, 3, "MES") == 0 || contract.compare(0, 3, "MNQ") == 0 || contract.compare(0, 3, "MCL") == 0 ||
        contract.compare(0, 3, "MGC") == 0 || contract.compare(0, 3, "MNG") == 0 || contract.compare(0, 3, "MHG") == 0 || contract.compare(0, 3, "M6E") == 0) return false;
    if (root == "ES" || root == "EP") { spec = { "ES", .25, 510, 900 }; return true; }
    if (root == "NQ") { spec = { "NQ", .25, 510, 900 }; return true; }
    if (root == "CL") { spec = { "CL", .01, 480, 810 }; return true; }
    if (root == "NG") { spec = { "NG", .001, 480, 810 }; return true; }
    if (root == "GC") { spec = { "GC", .1, 440, 750 }; return true; }
    if (root == "HG" || root == "CP") { spec = { "HG", .0005, 430, 750 }; return true; }
    if (root == "EU" || root == "6E" || root == "E6") { spec = { "EU", .00005, 440, 840 }; return true; }
    return false;
}
static bool tickMatches(double supplied, double required) {
    return finiteD(supplied) && supplied > 0.0 && std::fabs(supplied - required) <= std::max(1e-10, required * 1e-7);
}

struct SlotRef { double buy = nanD(), sell = nanD(), total = nanD(); int n = 0; };
struct Calibration {
    bool loaded = false;
    ex2::Config config;
    std::string modelId;
    long long cutoffSession = 0;
    int minimumBaselineSessions = 5;
    std::map<int, SlotRef> slots;
    std::string problem;
};


// Calibrations are all-or-nothing.  This is deliberately the line-oriented
// EXH2 protocol produced by export_native_calibration.py, not a legacy norm
// file and not a generic CSV table.  A malformed or mismatched file supplies
// no references at all.
static Calibration readCalibration(const std::string& path, const std::string& market, const std::string& contract,
                                   double hostTick, int expectedRthOpen, int expectedRthClose) {
    Calibration result;
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) { result.problem = "CALIBRATING"; return result; }
    std::set<std::string> meta, params;
    std::string line; bool ended = false, ok = false;
    ex2::Config cfg; long long cutoff = 0; std::string model;
    int minimum = 0, rthOpen = -1, rthClose = -1; double fileTick = nanD();
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || ended) { result.problem = "CALIBRATING"; return result; }
        const std::vector<std::string> f = csv(line, ok);
        if (!ok || f.empty()) { result.problem = "CALIBRATING"; return result; }
        const std::string tag = trim(f[0]);
        if (tag == "END") { if (f.size() != 2 || trim(f[1]) != "EXH2") { result.problem = "CALIBRATING"; return result; } ended = true; continue; }
        if (tag == "PARAM") {
            if (f.size() != 3 || !params.insert(trim(f[1])).second) { result.problem = "CALIBRATING"; return result; }
            const std::string k = trim(f[1]), v = trim(f[2]); double d = 0.0; int n = 0;
            if (k == "n" && parseI(v, n)) cfg.n = n;
            else if (k == "smooth" && parseI(v, n)) cfg.smooth = n;
            else if (k == "look" && parseI(v, n)) cfg.look = n;
            else if (k == "min_peak_age" && parseI(v, n)) cfg.min_peak_age = n;
            else if (k == "rearm_bars" && parseI(v, n)) cfg.rearm_bars = n;
            else if (k == "confirmation_bars" && parseI(v, n)) cfg.confirmation_bars = n;
            else if (k == "pullback_n" && parseI(v, n)) cfg.pullback_n = n;
            else if (k == "trend_ema" && parseI(v, n)) cfg.trend_ema = n;
            else if (k == "trend_slope_bars" && parseI(v, n)) cfg.trend_slope_bars = n;
            else if (k == "fade_threshold" && parseD(v, d)) cfg.fade_threshold = d;
            else if (k == "min_peak_norm" && parseD(v, d)) cfg.min_peak_norm = d;
            else if (k == "min_current_norm" && parseD(v, d)) cfg.min_current_norm = d;
            else if (k == "max_current_norm" && parseD(v, d)) cfg.max_current_norm = d;
            else if (k == "min_total_norm" && parseD(v, d)) cfg.min_total_norm = d;
            else if (k == "min_coverage" && parseD(v, d)) cfg.min_coverage = d;
            else if (k == "min_leg_atr" && parseD(v, d)) cfg.min_leg_atr = d;
            else if (k == "min_rejection" && parseD(v, d)) cfg.min_rejection = d;
            else if (k == "max_extension_atr" && parseD(v, d)) cfg.max_extension_atr = d;
            else if (k == "rearm_fraction" && parseD(v, d)) cfg.rearm_fraction = d;
            else if (k == "confirm_close_fraction" && parseD(v, d)) cfg.confirm_close_fraction = d;
            else if (k == "confirm_opposing_share" && parseD(v, d)) cfg.confirm_opposing_share = d;
            else if (k == "trend_min_slope_atr" && parseD(v, d)) cfg.trend_min_slope_atr = d;
            else if (k == "mode" && (v == "pullback" || v == "reversal")) cfg.mode = v;
            else if (k == "require_confirmation" && (v == "1" || v == "0")) cfg.require_confirmation = v == "1";
            else { result.problem = "CALIBRATING"; return result; }
            continue;
        }
        if (tag == "BASE") {
            if (f.size() != 6) { result.problem = "CALIBRATING"; return result; }
            int slotMinute = 0, n = 0; double buy = nanD(), sell = nanD(), total = nanD();
            if (minimum < 1 || !parseI(f[1], slotMinute) || !parseI(f[5], n) || slotMinute < 0 || slotMinute >= 1440 || n < 0 || result.slots.count(slotMinute * 60)) { result.problem = "CALIBRATING"; return result; }
            const bool missing = trim(f[2]) == "NaN" && trim(f[3]) == "NaN" && trim(f[4]) == "NaN";
            if (!missing && (!parseD(f[2], buy) || !parseD(f[3], sell) || !parseD(f[4], total))) { result.problem = "CALIBRATING"; return result; }
            SlotRef ref; ref.n = n;
            if (n >= minimum && buy > 0.0 && sell > 0.0 && total > 0.0) { ref.buy = buy; ref.sell = sell; ref.total = total; }
            result.slots[slotMinute * 60] = ref;
            continue;
        }
        if (f.size() != 2 || !meta.insert(tag).second) { result.problem = "CALIBRATING"; return result; }
        const std::string v = trim(f[1]); int n = 0;
        if (tag == "SCHEMA" ? v != "EXH2" :
            tag == "MARKET" ? v != market :
            tag == "CONTRACT" ? v != contract :
            tag == "BAR_SECONDS" ? (!parseI(v, n) || n != EX2_BAR_SECONDS) :
            tag == "TIMEZONE" ? v != "America/Chicago" :
            tag == "TICK" ? (!parseD(v, fileTick) || !tickMatches(fileTick, hostTick)) :
            tag == "CUTOFF_SESSION" ? !parseDay(v, cutoff) :
            tag == "MODEL_ID" ? !safeField(v, 120) :
            tag == "STATUS" ? v != "research_only" :
            tag == "MIN_BASELINE_SESSIONS" ? (!parseI(v, minimum) || minimum < 1) :
            tag == "RTH_OPEN" ? (!parseI(v, rthOpen) || rthOpen != expectedRthOpen) :
            tag == "RTH_CLOSE" ? (!parseI(v, rthClose) || rthClose != expectedRthClose) :
            tag == "ELIGIBILITY" ? v != "rth" : true) { result.problem = "CALIBRATING"; return result; }
        if (tag == "MODEL_ID") model = v;
    }
    const char* requiredMeta[] = { "SCHEMA", "MARKET", "CONTRACT", "BAR_SECONDS", "TIMEZONE", "TICK", "CUTOFF_SESSION", "MODEL_ID", "STATUS", "MIN_BASELINE_SESSIONS", "RTH_OPEN", "RTH_CLOSE", "ELIGIBILITY" };
    const char* requiredParams[] = { "n", "smooth", "look", "fade_threshold", "min_peak_norm", "min_current_norm", "max_current_norm", "min_total_norm", "min_coverage", "min_peak_age", "min_leg_atr", "min_rejection", "max_extension_atr", "rearm_bars", "rearm_fraction", "confirmation_bars", "confirm_close_fraction", "confirm_opposing_share", "require_confirmation", "mode", "pullback_n", "trend_ema", "trend_slope_bars", "trend_min_slope_atr" };
    for (size_t i = 0; i < sizeof(requiredMeta)/sizeof(requiredMeta[0]); ++i) if (!meta.count(requiredMeta[i])) { result.problem = "CALIBRATING"; return result; }
    for (size_t i = 0; i < sizeof(requiredParams)/sizeof(requiredParams[0]); ++i) if (!params.count(requiredParams[i])) { result.problem = "CALIBRATING"; return result; }
    std::string why;
    if (!ended || result.slots.empty() || !cfg.validate(&why)) { result.problem = "CALIBRATING"; return result; }
    result.loaded = true; result.config = cfg; result.modelId = model; result.cutoffSession = cutoff; result.minimumBaselineSessions = minimum; result.problem.clear();
    return result;
}

struct Flow {
    double buy = nanD(), sell = nanD(), total = nanD();
    bool sideKnown = false, totalKnown = false, known = false;
};
struct Point { double buy = nanD(), sell = nanD(); };
struct Marker { std::string text; COLOR color = C_GREY; };
#ifdef EX2_TEST_HOOKS
void ex2_test_trace(const ex2::Bar& b);   // tests only: every bar fed to the engine
#endif
struct RuntimeState {
    std::recursive_mutex mutex;
    bool identified = false, sourceFault = false, dstAmbiguous = false;
    std::string identity, market, contract, state = "CALIBRATING", coreStatus;
    double tick = nanD(); int spb = 0, rthOpen = 0, rthClose = 0;
    Calibration active, staged;
    std::unique_ptr<ex2::Engine> engine;
    long nextClosedIndex = -1;          // immutable index frontier, never reset for a config poll
    long initialFirst = -1;
    long long lastCommittedRaw = 0, previousEndCivil = 0;
    long long currentSession = std::numeric_limits<long long>::min();
    long long run = 0;
    bool havePreviousCivil = false;
    std::vector<double> priorTR;
    double previousClose = nanD();
    std::vector<double> trendSeed, trendEmaHistory;
    double trendEma = nanD(), trendPreviousClose = nanD();
    int trend = 0;
    std::map<long long, Point> points;
    std::map<long long, std::vector<Marker> > markers;
    std::set<std::string> journalKnown, journalLoaded;
    long long lastCalibrationReadMs = -1;
    long long lastStatusMs = -1; std::string lastStatusText;   // LRA: status file for the nightly calibration + IRT health checks
    long long journalFailures = 0;
    std::vector<ex2::Event> retryEvents;
};

class Exhaustion : public cppExtension {
public:
    SS_SAFE_DIALOG(Exhaustion, "ExhaustionV2")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    Exhaustion() : cppExtension() {}
    virtual int draw(void);
    virtual int parmsLoadBody(void) { return RTX_OK; }
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    std::string stateText() { RuntimeState* r = slot_.get(this, false); return r ? r->state : std::string(); }   // status / tests
    void pump();
    void release() { slot_.release(this); }
private:
    HostSlot<RuntimeState> slot_;
    void identify(RuntimeState& s);
    bool readFlow(RTARRAYP& vap, int bar, double authoritativeTotal, Flow& out);
    void maybeReadCalibration(RuntimeState& s, bool force);
    void startEngine(RuntimeState& s, const Calibration& cal);
    void calcBars(RuntimeState& s, long count);
    void handleEvent(RuntimeState& s, const ex2::Event& e);
    void flushJournals(RuntimeState& s, const std::vector<ex2::Event>& events);
    void render(const RuntimeState& s);
    std::string dir() const { const std::string f = flexDir(); return f.empty() ? f : f + "\\Exhaustion"; }
    std::string calPath(const RuntimeState& s) const { const std::string f = flexDir(); return f.empty() ? f : f + "\\LRA-ExhaustionV2-" + s.market + "-" + safeFilePart(s.contract) + ".csv"; }
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { static_cast<Exhaustion*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<Exhaustion*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(2); return RTX_OK; }
int cppExtension::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<Exhaustion*>(this)->pump(); return RTX_OK; }
int Exhaustion::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    RuntimeState* s = slot_.get(this, false);
    if (!s) return RTX_OK;
    std::unique_lock<std::recursive_mutex> lock(s->mutex, std::try_to_lock);
    if (!lock.owns_lock()) return RTX_OK;
    try { render(*s); } catch (...) { /* drawing must neither calculate nor write files */ }
    return RTX_OK;
}

void Exhaustion::identify(RuntimeState& s) {
    char rootBuffer[64]; std::memset(rootBuffer, 0, sizeof(rootBuffer));
    const char* rootPtr = getRootSymbol(rootBuffer); const char* contractPtr = getSymbol();
    const std::string root = rootPtr ? trim(rootPtr) : std::string();
    const std::string contract = contractPtr ? trim(contractPtr) : std::string();
    const int spb = getSecondsPerBar();
    const double tick = (double)getProperty(SYM_TICKINCR);
    const std::string id = root + "|" + contract + "|" + std::to_string(spb) + "|" + std::to_string(tick);
    if (s.identified && id == s.identity) return;
    // Do not assign RuntimeState: it owns the currently locked mutex.  An
    // identity/source change is an explicit reset, not a calibration rewind.
    s.sourceFault = false; s.dstAmbiguous = false; s.active = Calibration(); s.staged = Calibration(); s.engine.reset();
    s.nextClosedIndex = s.initialFirst = -1; s.lastCommittedRaw = s.previousEndCivil = 0;
    s.currentSession = std::numeric_limits<long long>::min(); s.run = 0; s.havePreviousCivil = false; s.priorTR.clear(); s.previousClose = nanD();
    s.trendSeed.clear(); s.trendEmaHistory.clear(); s.trendEma = s.trendPreviousClose = nanD(); s.trend = 0;
    s.points.clear(); s.markers.clear(); s.journalKnown.clear(); s.journalLoaded.clear(); s.lastCalibrationReadMs = -1; s.journalFailures = 0; s.retryEvents.clear(); s.coreStatus.clear();
    s.identified = true; s.identity = id; s.contract = contract; s.spb = spb; s.tick = tick;
    if (contract.empty() || !safeField(contract, 120)) { s.sourceFault = true; s.state = "UNKNOWN MARKET"; return; }
    if (spb != EX2_BAR_SECONDS) { s.sourceFault = true; s.state = "UNSUPPORTED TIMEFRAME"; return; }
    MarketSpec spec;
    if (!marketSpec(root, contract, spec)) { s.sourceFault = true; s.state = "UNKNOWN MARKET"; return; }
    if (!tickMatches(tick, spec.tick)) { s.sourceFault = true; s.state = "UNKNOWN MARKET"; return; }
    s.market = spec.market; s.rthOpen = spec.rthOpen; s.rthClose = spec.rthClose;
    maybeReadCalibration(s, true);
    startEngine(s, s.active);
}

// A failed VAP read or unavailable authoritative total remains NaN/invalid; it
// never becomes a zero-volume tape observation or an artificial 100% coverage.
bool Exhaustion::readFlow(RTARRAYP& vap, int bar, double authoritativeTotal, Flow& out) {
    out = Flow(); BARSTATISTICS st; std::memset(&st, 0, sizeof(st));
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (vap.getBarStatistics(bar, st) != RTX_OK) return false;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
    const double buy = (double)st.buyVolume, sell = (double)st.sellVolume;
    if (!finiteD(buy) || !finiteD(sell) || buy < 0.0 || sell < 0.0) return false;
    out.buy = buy; out.sell = sell; out.sideKnown = true;
    // Do not infer total from classified flow: that would conceal unknown side
    // volume and render min_coverage meaningless.
    if (!finiteD(authoritativeTotal) || authoritativeTotal < 0.0) return false;
    // LRA: the chart's bar volume and its volume-at-price can differ by a few contracts (late prints); up to 2% over is the same
    // trades counted twice, not unknown side - use the classified total there. More than that is a real mismatch: not known.
    if (buy + sell > authoritativeTotal + 1e-9) { if (buy + sell > authoritativeTotal * 1.02 + 1.0) return false; authoritativeTotal = buy + sell; }
    out.total = authoritativeTotal; out.totalKnown = true; out.known = true;
    return out.known;
}

void Exhaustion::maybeReadCalibration(RuntimeState& s, bool force) {
    const long long now = steadyMs();
    if (!force && s.lastCalibrationReadMs >= 0 && now >= s.lastCalibrationReadMs && now - s.lastCalibrationReadMs < 60000) return;
    s.lastCalibrationReadMs = now;
    const Calibration candidate = readCalibration(calPath(s), s.market, s.contract, s.tick, s.rthOpen, s.rthClose);
    if (force) s.active = candidate;
    else s.staged = candidate; // applied only on the next explicit session/source start
}
void Exhaustion::startEngine(RuntimeState& s, const Calibration& cal) {
    ex2::Config cfg = cal.loaded ? cal.config : ex2::Config();
    std::string why;
    if (!cfg.validate(&why) || !finiteD(s.tick) || s.tick <= 0.0) { s.engine.reset(); s.state = "CALIBRATING"; return; }
    s.engine.reset(new ex2::Engine(cfg, s.tick));
    if (!cal.loaded) s.state = "CALIBRATING";
}

static bool validOhlc(double o, double h, double l, double c) {
    return finiteD(o) && finiteD(h) && finiteD(l) && finiteD(c) && h >= l && h >= std::max(o, c) && l <= std::min(o, c);
}
static double atrPrior(const RuntimeState& s) {
    if (s.priorTR.size() != 14) return nanD();
    double sum = 0.0; for (size_t i = 0; i < s.priorTR.size(); ++i) sum += s.priorTR[i];
    return finiteD(sum) ? sum / (double)s.priorTR.size() : nanD();
}
static void addTR(RuntimeState& s, const ex2::Bar& b) {
    if (!validOhlc(b.o,b.h,b.l,b.c)) { s.priorTR.clear();s.previousClose=nanD();return; }
    const double tr = finiteD(s.previousClose) ? std::max(b.h - b.l, std::max(std::fabs(b.h - s.previousClose), std::fabs(b.l - s.previousClose))) : b.h-b.l;
    s.previousClose = b.c;
    if (!finiteD(tr) || tr < 0.0) { s.priorTR.clear(); return; }
    s.priorTR.push_back(tr); if (s.priorTR.size() > 14) s.priorTR.erase(s.priorTR.begin());
}

// This is evaluated before the current bar reaches the engine.  trendEma and
// its slope history therefore contain only completed *previous* closes.
static int trendBefore(const RuntimeState& s, double atr) {
    const ex2::Config& c = s.active.config;
    if (!finiteD(s.trendEma) || !finiteD(s.trendPreviousClose) || !finiteD(atr) || atr <= 0.0 ||
        s.trendEmaHistory.size() <= (size_t)c.trend_slope_bars) return 0;
    const double old = s.trendEmaHistory[s.trendEmaHistory.size() - 1 - (size_t)c.trend_slope_bars];
    const double slope = (s.trendEma - old) / atr;
    const bool bullStart = slope >= c.trend_min_slope_atr && s.trendPreviousClose >= s.trendEma;
    const bool bearStart = slope <= -c.trend_min_slope_atr && s.trendPreviousClose <= s.trendEma;
    int bias = s.trend;
    if (bias > 0 && (slope <= -c.trend_min_slope_atr || s.trendPreviousClose < s.trendEma - atr)) bias = 0;
    else if (bias < 0 && (slope >= c.trend_min_slope_atr || s.trendPreviousClose > s.trendEma + atr)) bias = 0;
    if (bias == 0) return bullStart ? 1 : bearStart ? -1 : 0;
    return bias; // slow/ambiguous slope preserves an established directional bias
}
static void updateTrend(RuntimeState& s, const ex2::Bar& b) {
    const ex2::Config& c = s.active.config;
    if (!validOhlc(b.o,b.h,b.l,b.c)) { s.trendSeed.clear(); s.trendEmaHistory.clear(); s.trendEma = s.trendPreviousClose = nanD(); s.trend = 0; return; }
    s.trendPreviousClose = b.c;
    if (!finiteD(s.trendEma)) {
        s.trendSeed.push_back(b.c);
        if (s.trendSeed.size() < (size_t)c.trend_ema) return;
        double sum = 0.0; for (size_t i = 0; i < s.trendSeed.size(); ++i) sum += s.trendSeed[i];
        s.trendEma = sum / (double)s.trendSeed.size(); s.trendSeed.clear(); s.trendEmaHistory.push_back(s.trendEma);
    } else {
        const double alpha = 2.0 / (1.0 + (double)c.trend_ema);
        s.trendEma = alpha * b.c + (1.0 - alpha) * s.trendEma;
        s.trendEmaHistory.push_back(s.trendEma);
    }
    const size_t keep = (size_t)c.trend_slope_bars + 1;
    while (s.trendEmaHistory.size() > keep) s.trendEmaHistory.erase(s.trendEmaHistory.begin());
}

void Exhaustion::calcBars(RuntimeState& s, long count) {
    if (!s.engine || s.sourceFault || count < 2) return;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYP vap(barVolumeProfile);
    RTARRAYI totalVolume(barVolume);                     // LRA: verified in the real irtsdk.h (iARRAY barVolume, "vol[i] references the volume of the i-th bar")
    if (times.count < count || op.count < count || hi.count < count || lo.count < count || cl.count < count || vap.count < count) return;
    if (totalVolume.count < count) return;
    const long lastClosed = count - 2;                  // Host convention: final array bar is forming; never feed it.
    if (s.nextClosedIndex < 0) {
        s.initialFirst = std::max(0L, lastClosed - EX2_BARS_MAX + 1);
        s.nextClosedIndex = s.initialFirst;
    }
    // LRA: IRT may trim the oldest bars or reload history; the frontier is found again by its bar TIME (never re-feeding a bar).
    // If the last committed bar is gone entirely, the indicator starts over from the chart's own history (identify() reset).
    if (s.lastCommittedRaw && (s.nextClosedIndex <= 0 || s.nextClosedIndex - 1 >= count || (long long)times[(int)(s.nextClosedIndex-1)] != s.lastCommittedRaw)) {
        long found = -1;
        for (long j = std::min(count - 1, lastClosed); j >= 0; --j) { const long long t = (long long)times[(int)j]; if (t == s.lastCommittedRaw) { found = j; break; } if (t < s.lastCommittedRaw) break; }
        if (found < 0) { s.identified = false; s.state = "RELOADING HISTORY"; return; }
        s.nextClosedIndex = found + 1;
    }
    if (lastClosed < s.nextClosedIndex) {flushJournals(s,s.retryEvents);return;}
    // A shortened/reloaded chart is a source reset, never a loop that revises prior cached marks.
    if (s.lastCommittedRaw && s.nextClosedIndex >= count) { s.identified = false; s.state = "RELOADING HISTORY"; return; }
    std::vector<ex2::Event> events;
    for (long i = s.nextClosedIndex; i <= lastClosed; ++i) {
        const RTDATE raw = (RTDATE)times[(int)i];
        CivilBar civil;
        if (!makeCivilBar(*this, raw, s.spb, s.rthOpen, s.rthOpen==s.rthClose?s.rthOpen+1:s.rthClose, civil)) { s.sourceFault=true;s.state="INVALID BAR TIME - reload required";return; }
        if (s.lastCommittedRaw && civil.raw <= s.lastCommittedRaw) { s.sourceFault = true; s.state = "SOURCE RESET REQUIRED"; return; }
        // A repeated fall-back civil label cannot be ordered without an offset-aware SDK timestamp.
        if (s.havePreviousCivil && civil.endCivil <= s.previousEndCivil) s.dstAmbiguous = true;
        // Overnight bars remain in this signal run as context.  Only an actual
        // session/gap (or source identity change in identify()) resets ATR,
        // EMA/trend, smoothing, and the core.  b.eligible below still confines
        // pullback/watch decisions to the current RTH closed bar.
        const bool boundary = !s.havePreviousCivil || civil.sessionDays != s.currentSession ||
                              (s.havePreviousCivil && civil.endCivil - s.previousEndCivil != s.spb);
        if (boundary) {
            ++s.run; s.priorTR.clear(); s.previousClose = nanD();
            s.trendSeed.clear(); s.trendEmaHistory.clear(); s.trendEma = s.trendPreviousClose = nanD(); s.trend = 0;
            if (civil.sessionDays != s.currentSession) {
                const bool hadSession = s.currentSession != std::numeric_limits<long long>::min();
                const bool oldLoaded = s.active.loaded;
                const std::string oldModel = s.active.modelId;
                const long long oldCutoff = s.active.cutoffSession;
                // Files are read only at a source/session start.  No timer or
                // draw path may apply changed parameters to an existing run.
                if(s.retryEvents.empty()) maybeReadCalibration(s, true);
                else s.state="CALIBRATION UPDATE DEFERRED - journal retry pending";
                const bool changed = oldLoaded != s.active.loaded || oldModel != s.active.modelId || oldCutoff != s.active.cutoffSession;
                if (hadSession && changed) {
                    startEngine(s, s.active); // explicit session reset; no index rewind/replay
                    if (!s.engine) return;
                }
                s.currentSession = civil.sessionDays;
            }
        }
        s.havePreviousCivil = true; s.previousEndCivil = civil.endCivil;
        const bool ohlc = validOhlc(op[(int)i], hi[(int)i], lo[(int)i], cl[(int)i]);
        double authoritativeTotal = nanD();
        authoritativeTotal = (double)totalVolume[(int)i];
        Flow flow; const bool haveFlow = readFlow(vap, (int)i, authoritativeTotal, flow);
        ex2::Bar b;
        b.time = civil.raw; b.run = s.run; b.eligible = civil.rth;
        // LRA: IRT hands prices as float (90.55 -> 90.5500030...); snap them back onto the tick grid in double so equal prices compare
        // equal and tick arithmetic matches the research code (it reads the same prices on the grid)
        const auto snap = [&s](double v) { return std::round(v / s.tick) * s.tick; };
        b.o = ohlc ? snap(op[(int)i]) : nanD(); b.h = ohlc ? snap(hi[(int)i]) : nanD(); b.l = ohlc ? snap(lo[(int)i]) : nanD(); b.c = ohlc ? snap(cl[(int)i]) : nanD();
        b.v = haveFlow ? flow.total : nanD(); b.buy = haveFlow ? flow.buy : nanD(); b.sell = haveFlow ? flow.sell : nanD();
        b.buyRef = b.sellRef = b.totalRef = nanD(); b.atrPrior = atrPrior(s);
        const std::map<int, SlotRef>::const_iterator ref = s.active.slots.find(civil.slotSeconds);
        if (s.active.loaded && civil.sessionDays > s.active.cutoffSession && ref != s.active.slots.end() && ref->second.n >= s.active.minimumBaselineSessions) {
            b.buyRef = ref->second.buy; b.sellRef = ref->second.sell; b.totalRef = ref->second.total;
        }
        b.valid = !s.dstAmbiguous && civil.valid && ohlc && haveFlow && flow.total > 0.0 && flow.buy >= 0.0 && flow.sell >= 0.0 && flow.buy + flow.sell <= flow.total + 1e-9;
        if (!b.valid) { b.buyRef = b.sellRef = b.totalRef = nanD(); b.atrPrior = nanD(); }
        b.trend = trendBefore(s, b.atrPrior);
#ifdef EX2_TEST_HOOKS
        ex2_test_trace(b);
#endif
        ex2::Result r = s.engine->update(b);
        Point point; point.buy = finiteD(r.fadeBuy) ? r.fadeBuy : nanD(); point.sell = finiteD(r.fadeSell) ? r.fadeSell : nanD(); s.points[civil.raw] = point;
        if (!r.status.empty()) s.coreStatus = r.status;
        for(size_t j=0;j<r.events.size();++j) handleEvent(s,r.events[j]);
        s.retryEvents.insert(s.retryEvents.end(),r.events.begin(),r.events.end());
        flushJournals(s,s.retryEvents);
        s.retryEvents.erase(std::remove_if(s.retryEvents.begin(),s.retryEvents.end(),[&s](const ex2::Event& e){
            return s.journalKnown.count(s.active.modelId+"|"+s.contract+"|"+e.id+"|"+e.kind+"|"+std::to_string(e.time))>0;
        }),s.retryEvents.end());
        addTR(s, b);
        s.trend = b.trend;
        updateTrend(s, b);
        s.lastCommittedRaw = civil.raw;
        s.nextClosedIndex = i + 1; // commit exactly once; forming-bar updates never overwrite it.
        if (s.dstAmbiguous) s.state = "DST TIME UNSUPPORTED";
        else if (!s.active.loaded || civil.sessionDays <= s.active.cutoffSession) s.state = "CALIBRATING";
        else if (!flow.sideKnown) s.state = "UNKNOWN SIDE";
        else if (!flow.totalKnown) s.state = "UNKNOWN TOTAL VOLUME";
        else if (flow.total == 0.0) s.state = "TAPE INACTIVE";
        else if (!b.valid) s.state = "UNKNOWN SIDE";
        else if (!finiteD(b.buyRef) || !finiteD(b.sellRef) || !finiteD(b.totalRef)) s.state="CALIBRATING - missing time-slot references";
        else if ((b.buy+b.sell)/b.v<s.active.config.min_coverage) s.state="LOW SIDE COVERAGE";
        else if (!finiteD(b.atrPrior) || s.trendEmaHistory.size()<=(size_t)s.active.config.trend_slope_bars) s.state="BUILDING BAR CONTEXT";
        else s.state = s.coreStatus=="OK" ? "READY - no qualified setup" : s.coreStatus;
    }
    for (size_t i = 0; i < events.size(); ++i) handleEvent(s, events[i]);
    s.retryEvents.insert(s.retryEvents.end(),events.begin(),events.end());
    flushJournals(s,s.retryEvents);
    s.retryEvents.erase(std::remove_if(s.retryEvents.begin(),s.retryEvents.end(),[&s](const ex2::Event& e){
        const std::string key=s.active.modelId+"|"+s.contract+"|"+e.id+"|"+e.kind+"|"+std::to_string(e.time);
        return s.journalKnown.count(key)>0;
    }),s.retryEvents.end());
    if(s.retryEvents.size()>10000){s.state="JOURNAL BACKLOG - check storage";s.retryEvents.resize(10000);}
    while (s.points.size() > 5000) s.points.erase(s.points.begin());
    while (s.markers.size() > 5000) s.markers.erase(s.markers.begin());
}

void Exhaustion::handleEvent(RuntimeState& s, const ex2::Event& e) {
    if (!finiteD(e.time == 0 ? nanD() : 1.0) || e.time == 0) return;
    Marker m;
    if (e.kind == "WATCH") m.text = "E?";
    else if (e.kind == "CONFIRM") m.text = "R?";
    else if (e.kind == "EXIT_WARN") m.text = "EXIT?";
    else if (e.kind == "INVALIDATE" || e.kind == "EXPIRE") m.text = "x";
    else return;
    m.color = e.side < 0 ? C_BUY_FADE : C_SELL_FADE;
    s.markers[e.time].push_back(m);
}

static bool validEvent(const ex2::Event& e) {
    return safeField(e.id, 160) && safeField(e.kind, 20) && safeField(e.reason, 160) &&
        (e.kind == "WATCH" || e.kind == "CONFIRM" || e.kind == "INVALIDATE" || e.kind == "EXPIRE" || e.kind == "EXIT_WARN") &&
        (e.side == -1 || e.side == 1) && e.time != 0 && e.watchTime != 0 && finiteD(e.extreme) && finiteD(e.stop) && finiteD(e.trigger) &&
        finiteD(e.fade) && finiteD(e.peakNorm) && finiteD(e.currentNorm) && finiteD(e.coverage);
}
void Exhaustion::flushJournals(RuntimeState& s, const std::vector<ex2::Event>& events) {
    if (events.empty() || !s.active.loaded) return;
    const std::string root = dir(); if (root.empty()) return;
    makeDir(flexDir()); makeDir(root);

    struct Pending { std::string path, key, line; };
    std::vector<Pending> pending;
    const char* header = "schema,engine_version,model_id,market,contract,session,event_id,kind,setup_type,trend,side,watch_time_raw,event_time_raw,watch_label_ct,event_label_ct,extreme,stop,trigger,fade,peak_norm,current_norm,coverage,reason";
    std::unique_lock<std::mutex> fileLock(g_fileMutex);
    for (size_t i = 0; i < events.size(); ++i) {
        const ex2::Event& e = events[i];
        if (!validEvent(e) || !safeField(e.setup_type, 40) || (e.trend < -1 || e.trend > 1)) { ++s.journalFailures; continue; }
        CivilBar civil;
        if (!makeCivilBar(*this, (RTDATE)e.time, s.spb, s.rthOpen, s.rthClose, civil)) { ++s.journalFailures; continue; }
        const std::string session = dayLabel(civil.sessionDays);
        const std::string path = root + "\\LRA-ExhaustionV2-events-" + EX2_VERSION + "-" + s.market + "-" + safeFilePart(s.contract) + "-" + session + ".csv";
        const std::string key = s.active.modelId + "|" + s.contract + "|" + e.id + "|" + e.kind + "|" + std::to_string(e.time);
        if (!s.journalLoaded.count(path)) {
            s.journalLoaded.insert(path);
            if (statFile(path)) {
                std::ifstream prior(path.c_str(), std::ios::binary); std::string row; bool localOk = false;
                if (!std::getline(prior, row) || trim(row) != header) { ++s.journalFailures; continue; }
                while (std::getline(prior, row)) {
                    if(prior.eof()) break; // a truncated record without its newline is not committed
                    if (!row.empty() && row.back() == '\r') row.pop_back();
                    const std::vector<std::string> fields = csv(row, localOk);
                    if (!localOk || fields.size() != 23 || fields[4] != s.contract || fields[2] != s.active.modelId || !safeField(fields[6], 160) || !safeField(fields[7], 20)) { ++s.journalFailures; continue; }
                    s.journalKnown.insert(fields[2] + "|" + fields[4] + "|" + fields[6] + "|" + fields[7] + "|" + fields[12]);
                }
            }
        }
        if (s.journalKnown.count(key)) continue;
        CivilBar watchCivil;
        if (!makeCivilBar(*this, (RTDATE)e.watchTime, s.spb, s.rthOpen, s.rthClose, watchCivil)) { ++s.journalFailures; continue; }
        std::ostringstream row; row.imbue(std::locale::classic()); row << std::setprecision(std::numeric_limits<double>::max_digits10);
        row << "EXH2EVENT," << EX2_VERSION << ',' << s.active.modelId << ',' << s.market << ',' << s.contract << ',' << session << ','
            << e.id << ',' << e.kind << ',' << e.setup_type << ',' << e.trend << ',' << e.side << ',' << e.watchTime << ',' << e.time << ','
            << civilLabel(watchCivil.endCivil) << ',' << civilLabel(civil.endCivil) << ',' << e.extreme << ',' << e.stop << ',' << e.trigger << ','
            << e.fade << ',' << e.peakNorm << ',' << e.currentNorm << ',' << e.coverage << ',' << e.reason;
        if(std::none_of(pending.begin(),pending.end(),[&key](const Pending& item){return item.key==key;})) pending.push_back({ path, key, row.str() });
    }
    for (size_t begin = 0; begin < pending.size();) {
        size_t end = begin + 1; while (end < pending.size() && pending[end].path == pending[begin].path) ++end;
        const bool fresh = !statFile(pending[begin].path);
        bool separateTail=false;
        if(!fresh){std::ifstream tail(pending[begin].path.c_str(),std::ios::binary);tail.seekg(0,std::ios::end);if(tail.tellg()>0){tail.seekg(-1,std::ios::end);char ch=0;tail.get(ch);separateTail=ch!='\n';}}
        std::ofstream out(pending[begin].path.c_str(), std::ios::binary | std::ios::app);
        if (!out) { s.journalFailures += (long long)(end - begin); begin = end; continue; }
        if (fresh) out << header << '\n';
        else if(separateTail) out << '\n';
        for (size_t j = begin; j < end; ++j) out << pending[j].line << '\n';
        out.flush();
        if (out) for (size_t j = begin; j < end; ++j) s.journalKnown.insert(pending[j].key);
        else s.journalFailures += (long long)(end - begin);
        begin = end;
    }
}

void Exhaustion::render(const RuntimeState& s) {
    RCT pane; pane.getPaneRect(false);
    const long count = getBarCount();
    if (count < 1 || pane.bottom - pane.top < 45 || pane.right - pane.left < 160) return;
    FONT font; font.id = HELVETICA; font.size = 9; font.style = BOLD; setFont(font);
    const auto text = [&](int x, int y, const std::string& value, COLOR colour) {
        setTextColor(colour); const int w = getTextWidth(value.c_str(), -1); RCT r; r.set(coord(x), coord(y), coord(x + w + 2), coord(y + 12)); r.drawText(value.c_str(), false, false); return x + w;
    };
    // LRA header (his readout format "Metric: now (line)", no explanatory text on the chart - the settings description has it)
    const auto xs = [](double v) { if (!finiteD(v)) return std::string("-"); char b[24]; std::snprintf(b, sizeof(b), "%.1fx", v); return std::string(b); };
    const double line = s.active.loaded ? s.active.config.fade_threshold : nanD();
    double nowBuy = nanD(), nowSell = nanD();
    if (!s.points.empty()) { nowBuy = s.points.rbegin()->second.buy; nowSell = s.points.rbegin()->second.sell; }
    {
        int x = text(pane.left + 4, pane.top + 1, "EXHAUSTION   ", C_GREY);
        x = text(x, pane.top + 1, s.trend > 0 ? "Uptrend   " : s.trend < 0 ? "Downtrend   " : "No trend   ", s.trend > 0 ? C_SELL_FADE : s.trend < 0 ? C_BUY_FADE : C_GREY);
        x = text(x, pane.top + 1, "Buyers fading: ", C_GREY); x = text(x, pane.top + 1, xs(nowBuy), C_BUY_FADE); x = text(x, pane.top + 1, " (" + xs(line) + ")   ", C_GREY);
        x = text(x, pane.top + 1, "Sellers fading: ", C_GREY); x = text(x, pane.top + 1, xs(nowSell), C_SELL_FADE); text(x, pane.top + 1, " (" + xs(line) + ")", C_GREY);
        const std::string tag = "testing"; const int w = getTextWidth(tag.c_str(), -1); text(pane.right - w - 8, pane.top + 1, tag, C_GREY);
    }
    if (!s.state.empty() && s.state.compare(0, 5, "READY") != 0) text(pane.left + 4, pane.top + 14, s.state, C_GREY);   // only when something is wrong / warming up
    RTARRAYI times(barDateTime); if (times.count < count) return;
    const int last = (int)count - 1;
    const auto xOf = [](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
    std::vector<int> visible;
    for (int i = last; i >= 0; --i) { const int x = xOf(i); if (x < pane.left) break; if (x <= pane.right) visible.push_back(i); }
    std::reverse(visible.begin(), visible.end());
    const auto point = [&](int bar, bool buy) {
        const std::map<long long, Point>::const_iterator p = s.points.find((long long)times[bar]);
        return p == s.points.end() ? nanD() : (buy ? p->second.buy : p->second.sell);
    };
    double topValue = 3.0;
    for (size_t i = 0; i < visible.size(); ++i) {
        const double a = point(visible[i], true), b = point(visible[i], false);
        if (finiteD(a)) topValue = std::max(topValue, a);
        if (finiteD(b)) topValue = std::max(topValue, b);
    }
    topValue = std::min(10.0, topValue * 1.2);
    const int top = pane.top + 30, bottom = pane.bottom - 34;
    const auto yOf = [&](double v) { return bottom - (int)((bottom - top) * std::min(1.0, std::max(0.0, v / topValue))); };
    const double threshold = s.active.loaded ? s.active.config.fade_threshold : nanD();
    if (finiteD(threshold)) {
        setPen(C_AXIS, 1, P_SOLID); const int y = yOf(threshold);
        PNT a; a.set(0, 0.0f); a.h = coord(pane.left); a.v = coord(y); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(pane.right); b.v = coord(y); b.drawLineTo();
        text(pane.right - 34, y - 13, xs(threshold), C_GREY);
    }
    for (int side = 0; side < 2; ++side) {
        const bool buy = side == 0; setPen(buy ? C_BUY_FADE : C_SELL_FADE, 2, P_SOLID); bool started = false;
        for (size_t k = 0; k < visible.size(); ++k) {
            const double value = point(visible[k], buy);
            if (!finiteD(value)) { started = false; continue; } // missing normalization is a visible gap, never a 1.0 substitute
            PNT p; p.set(0, 0.0f); p.h = coord(xOf(visible[k])); p.v = coord(yOf(value));
            if (!started) { p.setDrawPosition(); started = true; } else p.drawLineTo();
        }
    }
    // marks never overlap (his rule): each goes in the first of 3 rows where it clears the mark before it
    int rowEnd[3] = { INT_MIN, INT_MIN, INT_MIN };
    // (2.0.2, 2026-10-14 18:25 CT audit display list: Exhaustion V2 is retired - log only) the marks are still decided, journalled
    // (EXH2EVENT) and counted in the status file, never drawn
    static const bool kDrawMarks = false;
    for (size_t i = 0; kDrawMarks && i < visible.size(); ++i) {
        const std::map<long long, std::vector<Marker> >::const_iterator it = s.markers.find((long long)times[visible[i]]);
        if (it == s.markers.end()) continue;
        for (size_t m = 0; m < it->second.size(); ++m) {
            const int w = getTextWidth(it->second[m].text.c_str(), -1), x0 = xOf(visible[i]) - w / 2;
            int row = 0; while (row < 2 && x0 <= rowEnd[row] + 2) ++row;
            text(x0, bottom + 2 + row * 10, it->second[m].text, it->second[m].color); rowEnd[row] = x0 + w;
        }
    }
}


// LRA: a small status file (like every LRA indicator): the nightly calibration reads SYMBOL from it, the IRT health check reads STATE.
// Written only when the text changed and at most every 2 s; tmp + rename, never a partial file.
static void writeStatus(RuntimeState& s) {
    const long long now = steadyMs();
    if (s.lastStatusMs >= 0 && now >= s.lastStatusMs && now - s.lastStatusMs < 2000) return;
    const std::string f = flexDir(); if (f.empty() || s.market.empty()) return;
    std::ostringstream o; o.imbue(std::locale::classic());
    double b = nanD(), se = nanD(); if (!s.points.empty()) { b = s.points.rbegin()->second.buy; se = s.points.rbegin()->second.sell; }
    o << "VERSION," << EX2_VERSION << "\nMARKET," << s.market << "\nSYMBOL," << s.contract << "\nSPB," << s.spb << "\nSTATE," << s.state
      << "\nCALIBRATION," << (s.active.loaded ? "loaded" : "missing") << "," << s.active.modelId << ",cutoff " << s.active.cutoffSession
      << "\nTREND," << s.trend << "\nNOW (last closed bar),buyers_fading " << b << ",sellers_fading " << se << "\nMARKS," << s.markers.size() << "\n";
    const std::string text = o.str(); s.lastStatusMs = now; if (text == s.lastStatusText) return;
    const std::string path = f + "\\ExhaustionV2.status-" + s.market + "-" + std::to_string(s.spb) + ".txt", tmp = path + ".tmp";
    { std::ofstream out(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!out) return; out << text; if (!out) return; }
#if defined(_WIN32)
    if (!MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) return;
#else
    if (std::rename(tmp.c_str(), path.c_str()) != 0) return;
#endif
    s.lastStatusText = text;
}

void Exhaustion::pump() {
    RuntimeState* state = slot_.get(this, true);
    if (!state) return;
    std::unique_lock<std::recursive_mutex> lock(state->mutex, std::try_to_lock);
    if (!lock.owns_lock()) return;
    try {
        identify(*state);
        if (!state->sourceFault) calcBars(*state, getBarCount());
        writeStatus(*state);
    } catch (...) { state->state = "UNKNOWN SIDE"; }
}

extern "C" cppExtension* CreateExtension(void) {
    Exhaustion* p = new Exhaustion();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED);
    p->setDescription("LRA Exhaustion V2 - pullback read (research only): red buyer-fade is an uptrend exit warning; green seller-fade is a pullback setup only after a causal confirmation. Completed 180-second VAP bars only; calibration is exact-contract and prior-session-only.");
    p->setVersion("2.0.3");   // keep equal to EX2_VERSION (the build log reads this literal)
    return p;
}
