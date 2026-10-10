/********************************************************************************
 * DeltaProfileHourly.cpp -- lsDeltaProfileHourly 1.0.0 (DLT-H), 2026-10-09, task #302
 * The Delta Profile for the HOURLY chart. A separate plugin: its own files, header guard, namespace, class, DLL and
 * record folder, so it can never collide with the intraday lsDeltaProfile (DeltaProfile.cpp / DeltaProfileCore.h 2.5.4,
 * which this file does not include or touch).
 *
 * What it draws (no settings, no explanatory text on the chart):
 *   - right edge: the delta profile of the last 24 trading hours (the host bars that make them up, each built from its
 *     CLOSED 1-minute bars), green = bought at the ask, red = sold at the bid, slate = volume; the three biggest nodes are
 *     labelled "A 2.3x" / "A?" / "I" exactly as Delta Profile 2.5.4 labels them, absorption zones get their bracket + letter
 *   - on the hourly candle where each absorption candidate's delta peaked: ONE solid circle at its price with its letter
 *     (A?, then A when a later hourly bar closes beyond it in its own colour, or I when the other side won) - circle and
 *     letter are always drawn together (1:1), at most one per bar (the biggest), never on the profile
 *   - header "DLT-H 1.0.0" (+ "TESTING" until the market's hourly A passes the all-market study) and one short badge only
 *     when something is wrong: NO 1M / PARTIAL / STALE / BLOCKED
 * Data: ~2 sessions (48 trading hours) of the chart's own 1-minute bars with volume at price (RTBARS, by the chart's own
 * ticker and session), requested from the TIMER only, one request at a time across charts, behind a crash guard
 * (lsFlexLevels\DeltaProfileHourly\_trying-<MKT>-<spb>-dlth-<base|tail>.txt: a request IRT died in twice / three times is
 * blocked until its _blocked- file is deleted). The profile is rebuilt only when a new minute has CLOSED.
 * Signals never repaint: decided on closed bars only, every candidate and decision appended to
 * lsFlexLevels\DeltaProfileHourly\<MKT>-<spb>-signals-<session>.csv (also the nightly scoring input), and the last 10
 * sessions' records are re-read after a restart and redrawn from the file.
 * Status (for Claude / the nightly): lsFlexLevels\DeltaProfileHourly.status-<MKT>-<spb>s.txt (on change, at most every 5 s,
 * atomic); where a restart resumes: lsFlexLevels\DeltaProfileHourly\<MKT>-<spb>-progress-<root>.txt (every >= 30 s).
 * Tests: plugin\tests_dph (engine, two-chart adapter mock, replay == live, forming bar, MinGW vs the real SDK, ASan/UBSan).
 ********************************************************************************/
#include <string>
#include <ctime>
#include <cstring>
#include <climits>
#include <cfloat>
#include <cctype>
#include <cerrno>
#include <algorithm>
#include <locale>
#include <map>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
// The SDK selects its old Mac branch solely by __GNUC__ (as DeltaProfile.cpp): a MinGW syntax check of the real header
// defines DPH_WINDOWS_SDK_CHECK; the production MSVC build uses the unmodified header.
#if defined(DPH_WINDOWS_SDK_CHECK) && defined(__GNUC__)
#pragma push_macro("__GNUC__")
#undef __GNUC__
#include "irtsdk.h"
#pragma pop_macro("__GNUC__")
#else
#include "irtsdk.h"
#endif
#include "DeltaProfileHourlyEngine.h"
#include "HostSlot.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <utility>
#include <vector>
#include <sys/stat.h>
#if !defined(_WIN32)
#include <sys/types.h>
#endif

namespace dph = delta_profile_hourly;
static const char* const DPH_VERSION_TEXT = dph::DPH_VERSION;
// green = bullish, red = bearish, plus two others: slate (volume, axis) and ink (header, amounts) - the 2.5.4 values
static const COLOR C_BUY = 0x0022C55E, C_SELL = 0x00EF4444;
static const COLOR C_SUPPORT = 0x0086EFAC, C_RESISTANCE = 0x00FCA5A5;
static const COLOR C_SLATE = 0x00243040, C_INK = 0x00E5E7EB;
static const int L_WIDTH = 50, L_GAP = 0, L_FONT = 9;       // fixed layout (no settings): the 2.5.4 defaults
static const int SLICE_MS = 200, MAX_MINUTES_PER_PASS = 400, MAX_STEPS_PER_PASS = 2000;   // both also bounded by the 200-ms slice
static const int BASE_SPACING_S = 120, TAIL_SPACING_S = 20, START_DELAY_S = 20, GUARD_PROVE_S = 30;

// ---------------------------------------------------------------- small helpers (checked I/O, counted for the tests)
static std::atomic<long long> g_reads(0), g_writes(0), g_appends(0);
static std::int64_t steadyMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static std::string safeField(std::string s) {
    for (std::size_t i = 0; i < s.size(); ++i) if (s[i] == '|' || s[i] == ',' || s[i] == '\n' || s[i] == '\r') s[i] = ' ';
    return s;
}
static long long fileStamp(const std::string& path) {
#if defined(_WIN32)
    struct _stat64 st;
    if (_stat64(path.c_str(), &st) != 0) return -1;
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return -1;
#endif
    return static_cast<long long>(st.st_mtime) * 1000003LL + static_cast<long long>(st.st_size);
}
static bool fileExists(const std::string& path) { return fileStamp(path) >= 0; }
static void makeDirectory(const std::string& path) {
#if defined(_WIN32)
    CreateDirectoryA(path.c_str(), NULL);
#else
    mkdir(path.c_str(), 0777);
#endif
}
// test hook: makes the next rename fail (checklist #5 proof); never set in production
static std::atomic<int> g_failRename(0), g_failAppend(0);
static std::atomic<long long> g_testNow(0);          // test hook: the wall clock (0 = the real one)
static time_t wallNow() { const long long t = g_testNow.load(); return t ? static_cast<time_t>(t) : time(0); }
static bool renameOver(const std::string& from, const std::string& to) {
    if (g_failRename.load() > 0) { --g_failRename; return false; }
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
// checklist #5: .tmp, old -> .bak, rename into place, restore .bak on failure
static bool atomicWrite(const std::string& path, const std::string& contents) {
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    {
        std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << contents; f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; }
    }
    const bool had = fileExists(path);
    if (had) { std::remove(bak.c_str()); if (!renameOver(path, bak)) { std::remove(tmp.c_str()); return false; } }
    if (!renameOver(tmp, path)) {
        if (had) renameOver(bak, path);                                         // the old good copy back in place
        std::remove(tmp.c_str()); return false;
    }
    ++g_writes;
    return true;
}
static bool readWholeFile(const std::string& path, std::string& text, bool& tooLarge, std::size_t cap) {
    tooLarge = false; text.clear();
    std::ifstream f(path.c_str(), std::ios::binary | std::ios::ate);
    if (!f.is_open()) return false;
    ++g_reads;
    const std::streamoff size = static_cast<std::streamoff>(f.tellg());
    if (size < 0) return false;
    if (static_cast<unsigned long long>(size) > cap) { tooLarge = true; return false; }
    text.resize(static_cast<std::size_t>(size)); f.seekg(0);
    if (size > 0 && !f.read(&text[0], size)) { text.clear(); return false; }
    return true;
}
// whole lines in ONE write: a header on a new file, a newline first if a crash left a torn last line (2.5.4)
static bool appendRecord(const std::string& path, const std::vector<std::string>& lines) {
    if (lines.empty()) return true;
    if (g_failAppend.load() > 0) { --g_failAppend; return false; }
    bool fresh = true, torn = false;
    {
        std::ifstream t(path.c_str(), std::ios::binary | std::ios::ate);
        if (t.is_open()) {
            const std::streamoff size = static_cast<std::streamoff>(t.tellg());
            if (size > 0) { fresh = false; t.seekg(-1, std::ios::end); char ch = 0; if (t.get(ch) && ch != '\n') torn = true; }
        }
    }
    std::string text;
    if (fresh) { text += "# lsDeltaProfileHourly signals - one line per event (F first seen, D decided); times = Central clock as epoch seconds\n"; text += dph::recordHeader(); text += '\n'; }
    else if (torn) text += '\n';
    for (std::size_t i = 0; i < lines.size(); ++i) { text += lines[i]; text += '\n'; }
    std::ofstream o(path.c_str(), std::ios::binary | std::ios::app);
    if (!o.is_open()) return false;
    o.write(text.data(), static_cast<std::streamsize>(text.size())); o.flush();
    if (!o.good()) return false;
    ++g_appends;
    return true;
}
// Market code for file names and the PROVEN list only - never the tick size (SYM_TICKINCR is the only tick source).
static std::string marketName(std::string r) {
    for (std::size_t i = 0; i < r.size(); ++i) r[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(r[i])));
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return safeField(r);
}
// checklist #24: the chart's SYM_TICKINCR, made exact (it arrives as a float: 0.00005f = 4.99999987e-05)
static double cleanTick(float raw) {
    const double d = static_cast<double>(raw);
    if (!std::isfinite(d) || d <= 0 || d >= 1000) return 0;
    char b[32]; std::snprintf(b, sizeof(b), "%.6g", d);
    const double t = std::strtod(b, NULL);
    return std::isfinite(t) && t > 0 && t < 1000 ? t : 0;
}
static std::string userFolder() {
    const char* p = std::getenv("USERPROFILE");
    if (!p || !p[0]) return "";
    std::string base(p);
    if (base.back() != '\\' && base.back() != '/') base += DIR_SEPARATOR_CHR;
    base += "InvestorRT"; base += DIR_SEPARATOR_CHR; base += "rtx"; base += DIR_SEPARATOR_CHR; base += "lsFlexLevels";
    return base + DIR_SEPARATOR_CHR;
}
static std::string recordFolder() { const std::string b = userFolder(); return b.empty() ? b : b + "DeltaProfileHourly" + DIR_SEPARATOR_CHR; }
static bool proven(const std::string& market) {
    const std::string list = std::string(" ") + dphp::PROVEN_MARKETS + " ";
    return list.find(" " + market + " ") != std::string::npos;
}

// ---------------------------------------------------------------- the shared feed: one per chart symbol | bar size | session
// Two charts of the same symbol, bar size and session share ONE request and ONE engine (one writer of the record file).
// Everything else lives in the host's own slot (HostSlot).
struct Feed {
    std::recursive_mutex mx;
    std::string key, symbol, root, market; int spb = 0; short session = -1; double tick = 0;
    std::unique_ptr<dph::Engine> eng;
    // the host chart's bars, copied by the host's own calc / draw (the timer never asks the host chart)
    std::vector<long long> hostEnds; std::vector<RTDATE> hostRaw; std::vector<double> hostVol; long hostCount = -1;
    time_t hostTickAt = 0;
    // the 1-minute request
    cppExtension::RTBARS* req = nullptr; char reqKind = 0; time_t reqMade = 0, reqGrew = 0; long reqCount = -1, reqNext = 0;
    std::string guardOpen;                 // the _trying- file of the request still being proven
    int baseTries = 0; RTDATE lastRaw = 0; long long failTe = LLONG_MIN; time_t failSince = 0;
    time_t lastIngestAt = 0; long long firstMinute = LLONG_MIN;
    bool guardsRead = false, baseBlocked = false, tailBlocked = false;
    // records, progress, status
    bool seeded = false; std::string recordNote; int writeFails = 0;
    long long progressDone = LLONG_MIN; time_t progressAt = 0;
    std::string statusBody; time_t statusAt = 0;
    // counters for the status file
    long long unmapped = 0, mismatched = 0, missingAccepted = 0, requests = 0, nonMonotonic = 0;
    long long volChecked = 0, volExact = 0; double volMaxErr = 0;
    long long passes = 0; int maxPassMs = 0; std::string err;
    std::string reqNote = "not asked yet";
    ~Feed();
};
struct Pace { time_t start = 0, nextBaseAt = 0, nextTailAt = 0; };
static std::mutex& regMx() { static std::mutex m; return m; }
static std::map<std::string, std::weak_ptr<Feed> >& feedReg() { static std::map<std::string, std::weak_ptr<Feed> > m; return m; }
static Pace& pace() { static Pace p; return p; }
static int nextTimerId() { static std::atomic<int> n(53000); return ++n; }

struct HostState {
    int timerId = nextTimerId();
    bool timerOn = false, timerRefused = false; time_t timerAt = 0, lastTimerTick = 0, lastFallback = 0;
    std::string identity, symbol, root, market; int spb = 0; short session = -1; double tick = 0; std::string state = "starting";
    std::shared_ptr<Feed> feed;
    long barCount = -1; RTDATE firstStamp = 0, lastStamp = 0; std::vector<long long> ends; double lastVol = -1, lastClose = 0;
    int faults = 0; bool quarantined = false; std::string quarantineWhy;
    std::int64_t dealerAt = -1; long long dealerStamp = -2; int dealerReach = 0;
    int ringsDrawn = 0, lettersDrawn = 0, suppressed = 0, drawnRows = 0; long long draws = 0;
};
static std::map<int, HostState*>& timerReg() { static std::map<int, HostState*> m; return m; }

// the minute reads: only trivial values inside the MSVC SEH helpers (/EHsc compatible, as 2.5.4)
static int readStatistics(cppExtension::RTBARS* bars, int bar, cppExtension::BARSTATISTICS* stats) {
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!bars || !stats || bar < 0) return -2;
        std::memset(stats, 0, sizeof(*stats));
        if (bars->getBarStatistics(bar, *stats) != RTX_OK) return -2;
        if (stats->prices < 0) return -2;
        return stats->prices;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
static int readPrices(cppExtension::RTBARS* bars, int bar, cppExtension::VOLPROFILE* rows, int count) {
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!bars || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) {
            std::memset(&rows[i], 0, sizeof(rows[i]));
            if (bars->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2;   // checklist #22: any failure rejects the minute
        }
        return count;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// review S7: the request is made OUTSIDE any __try (MSVC C2712); the crash guard file protects against IRT dying in it
static cppExtension::RTBARS* makeRequest(const char* ticker, RTDATE start, short session) {
    return new (std::nothrow) cppExtension::RTBARS(PD_INTRA, 1, true, ticker, false, start, true, session);
}

static void guardWrite(Feed& f, const char* kind) {
    const std::string dir = recordFolder(); if (dir.empty()) return;
    makeDirectory(userFolder()); makeDirectory(dir);
    const std::string p = dir + "_trying-" + f.market + "-" + std::to_string(f.spb) + "-dlth-" + kind + ".txt";
    int strikes = 0; { std::ifstream g(p.c_str()); if (g.good()) g >> strikes; }
    { std::ofstream o(p.c_str(), std::ios::trunc); o << strikes << "\n"; }
    f.guardOpen = p;
}
static void guardClear(Feed& f) { if (!f.guardOpen.empty()) { std::remove(f.guardOpen.c_str()); f.guardOpen.clear(); } }
// a _trying- file found at start = IRT stopped while that request was unproven. base: blocked after 2, tail: after 3.
static void readGuards(Feed& f) {
    f.guardsRead = true;
    const std::string dir = recordFolder(); if (dir.empty()) return;
    const char* kinds[2] = { "base", "tail" }; const int limit[2] = { 2, 3 };
    for (int k = 0; k < 2; ++k) {
        const std::string stem = f.market + "-" + std::to_string(f.spb) + "-dlth-" + kinds[k] + ".txt";
        bool& blocked = k == 0 ? f.baseBlocked : f.tailBlocked;
        if (fileExists(dir + "_blocked-" + stem)) { blocked = true; continue; }
        std::ifstream g((dir + "_trying-" + stem).c_str());
        if (!g.good()) continue;
        int strikes = 0; g >> strikes; g.close(); ++strikes;
        if (strikes >= limit[k]) {
            std::ofstream o((dir + "_blocked-" + stem).c_str(), std::ios::trunc);
            o << "IRT stopped during this 1-minute request " << strikes << " times - blocked. Delete this file to allow it again.\n";
            std::remove((dir + "_trying-" + stem).c_str()); blocked = true;
        } else { std::ofstream o((dir + "_trying-" + stem).c_str(), std::ios::trunc); o << strikes << "\n"; }
    }
}
static void flushJournal(Feed& f) {
    dph::Engine& e = *f.eng;
    if (e.journal.empty()) return;
    const std::string dir = recordFolder();
    if (dir.empty()) { e.journalWritten(e.journal.size()); f.recordNote = "no USERPROFILE: record not kept"; return; }
    makeDirectory(userFolder()); makeDirectory(dir);
    while (!e.journal.empty()) {                         // grouped by session, oldest first, each group in ONE append
        const long long s = e.journal.front().session; std::vector<std::string> lines;
        for (const dph::JLine& j : e.journal) { if (j.session != s) break; lines.push_back(j.text); }
        if (!appendRecord(dir + dph::recordName(f.market, f.spb, s), lines)) {
            ++f.writeFails; f.recordNote = "record write failed - kept, retried on the next pass"; return;
        }
        e.journalWritten(lines.size()); f.recordNote.clear();
    }
}
static void writeProgress(Feed& f, bool force) {
    if (!f.eng) return;
    const long long p = f.eng->progressTe();
    const time_t now = wallNow();
    if (p == LLONG_MIN || p == f.progressDone || (!force && now - f.progressAt < 30)) return;   // checklist #31: >= 30 s
    const std::string dir = recordFolder(); if (dir.empty()) return;
    makeDirectory(userFolder()); makeDirectory(dir);
    std::ostringstream o; o << "DPH|" << dph::DPH_VERSION << '|' << f.symbol << '|' << p << '\n';
    if (atomicWrite(dir + f.market + "-" + std::to_string(f.spb) + "-progress-" + f.root + ".txt", o.str())) { f.progressDone = p; f.progressAt = now; }
}
Feed::~Feed() {
    try {
        if (eng) { flushJournal(*this); writeProgress(*this, true); }
    } catch (...) {}
    if (req) { delete req; req = nullptr; }
    guardClear(*this);                                   // a normal close is not a crash
}

// ---------------------------------------------------------------- the plugin
class DeltaProfileHourly : public cppExtension {
public:
    HostSlot<HostState> slot_;
    HostState* own_ = nullptr;
    HostState* bind(bool create) { HostState* s = slot_.get(this, create); own_ = s; return s; }
    int draw() override;
    int calc(int) override;
    int done() override;
    int destroy() override;
    int timer(RTX_EVENT* e) override;
    int parmsLoad() override { return RTX_OK; }          // no settings: nothing is loaded or saved (checklist #29)
    int parmsApply() override { return RTX_OK; }
    int parmsUpdt(unsigned int) override { return RTX_OK; }

    long long localSec(RTDATE value);
    bool identify(HostState& h);
    void syncHost(HostState& h);
    void pump(HostState& h, const char* via);
    void pass(HostState& h, bool fromTimer, bool contextOk);
    void requests(Feed& f, HostState& h, time_t now);
    void ingest(Feed& f, std::int64_t deadline);
    void seed(Feed& f);
    void status(Feed& f, HostState& h, bool force);
    void releaseHost(HostState& h);
    int dealerWidth(HostState& h);
    bool render(HostState& h);
    // drawing helpers (2.5.4)
    int yOf(int bar, double price) { PNT p; p.set(bar, static_cast<float>(price)); return p.v; }
    int textWidth(const std::string& t, int font, bool bold) { FONT f; f.id = HELVETICA; f.size = coordinate(font); f.style = bold ? BOLD : PLAIN; setFont(f); return getTextWidth(t.c_str(), -1); }
    void textRight(int right, int y, const std::string& text, COLOR color, int font, bool bold);
    void textLeft(int left, int y, const std::string& text, COLOR color, int font, bool bold);
    void textCentre(int x, int y, const std::string& text, COLOR color, int font, bool bold);
    void stroke(int x1, int y1, int x2, int y2, COLOR color, int width);
    void box(int left, int top, int right, int bottom, COLOR color);
    static short coordinate(int v) { return static_cast<short>(std::max<int>(SHRT_MIN, std::min<int>(SHRT_MAX, v))); }
};

int cppExtension::init() { return RTX_OK; }
int cppExtension::calc(int) { return RTX_OK; }
int cppExtension::done() { return RTX_OK; }
int cppExtension::destroy() { return RTX_OK; }
int cppExtension::setup() {
    setParameterVersion(1);              // no parameters at all: the settings window shows nothing to change
    setParameterDialogHeight(1);
    return RTX_OK;
}
long long DeltaProfileHourly::localSec(RTDATE value) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(value, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31 || t.tm_hour < 0 ||
        t.tm_hour > 23 || t.tm_min < 0 || t.tm_min > 59 || t.tm_sec < 0 || t.tm_sec > 60) return 0;
    return dph::civilSec(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
}
bool DeltaProfileHourly::identify(HostState& h) {
    const char* s = getSymbol(); const std::string symbol = s ? safeField(s) : "";
    char buf[64] = {0}; const char* r = getRootSymbol(buf); const std::string root = r ? safeField(r) : "";
    const int spb = getSecondsPerBar(); const short session = getChartSessionNumber();
    const double tick = cleanTick(getProperty(SYM_TICKINCR));
    const std::string id = symbol + "|" + std::to_string(spb) + "|" + std::to_string(session);
    if (id != h.identity || tick != h.tick) {                // symbol / bar size / session / tick change: start over
        releaseHost(h);
        h.identity = id; h.symbol = symbol; h.root = root; h.market = marketName(root); h.spb = spb; h.session = session; h.tick = tick;
        h.barCount = -1; h.ends.clear(); h.faults = 0; h.quarantined = false; h.quarantineWhy.clear();   // #30: a change lifts it
    }
    if (symbol.empty() || root.empty() || spb <= 0) { h.state = "chart not identified"; return false; }
    if (!(tick > 0)) { h.state = "native tick size unavailable"; return false; }   // #24: no hard-coded fallback
    if (!h.feed) {
        std::lock_guard<std::mutex> g(regMx());
        std::shared_ptr<Feed> f = feedReg()[id].lock();
        if (!f) {
            f = std::make_shared<Feed>(); f->key = id; f->symbol = symbol; f->root = root; f->market = h.market; f->spb = spb;
            f->session = session; f->tick = tick; feedReg()[id] = f;
        }
        h.feed = f;
    }
    return true;
}
void DeltaProfileHourly::releaseHost(HostState& h) {
    if (h.timerOn) { destroyTimer(h.timerId); h.timerOn = false; }
    h.timerRefused = false; h.lastTimerTick = 0;
    { std::lock_guard<std::mutex> g(regMx()); timerReg().erase(h.timerId); }
    h.feed.reset();                                          // the last host of a feed flushes and closes it
    h.identity.clear();
}
// the host chart's bars -> END times (Central clock), raw stamps and volumes; re-read only when the bar list changes
void DeltaProfileHourly::syncHost(HostState& h) {
    const long count = getBarCount();
    if (count < 1 || count > INT_MAX) return;
    RTARRAYI times(barDateTime); RTARRAYI vol(barVolume);
    if (times.count < count || vol.count < count) return;
    const RTDATE first = static_cast<RTDATE>(times[0]), last = static_cast<RTDATE>(times[static_cast<int>(count) - 1]);
    Feed* f = h.feed.get();
    const double lv = static_cast<double>(vol[static_cast<int>(count) - 1]);
    if (f && lv != h.lastVol) { if (h.lastVol >= 0) { std::lock_guard<std::recursive_mutex> g(f->mx); f->hostTickAt = wallNow(); } h.lastVol = lv; }
    if (count == h.barCount && first == h.firstStamp && last == h.lastStamp) return;
    h.barCount = count; h.firstStamp = first; h.lastStamp = last;
    h.ends.assign(static_cast<std::size_t>(count), 0);
    std::vector<RTDATE> raw(static_cast<std::size_t>(count)); std::vector<double> vols(static_cast<std::size_t>(count));
    long long nonMono = 0;
    for (int i = 0; i < static_cast<int>(count); ++i) {
        long long t = localSec(static_cast<RTDATE>(times[i]));
        if (i && t <= h.ends[static_cast<std::size_t>(i) - 1]) { t = h.ends[static_cast<std::size_t>(i) - 1] + 1; ++nonMono; }   // DST repeat / bad stamp: keep order
        h.ends[static_cast<std::size_t>(i)] = t; raw[static_cast<std::size_t>(i)] = static_cast<RTDATE>(times[i]);
        vols[static_cast<std::size_t>(i)] = static_cast<double>(vol[i]);     // #2: promoted before any sum
    }
    if (f) {
        std::lock_guard<std::recursive_mutex> g(f->mx);
        f->hostEnds = h.ends; f->hostRaw.swap(raw); f->hostVol.swap(vols); f->hostCount = count; f->nonMonotonic = nonMono;
    }
}
void DeltaProfileHourly::pump(HostState& h, const char* via) {
    const time_t now = wallNow();
    if (!h.timerOn && !h.timerRefused) {
        if (createTimer(h.timerId, 1000) == RTX_OK) {
            h.timerOn = true; h.timerAt = now;
            std::lock_guard<std::mutex> g(regMx()); timerReg()[h.timerId] = &h;
        } else h.timerRefused = true;
    }
    const bool noTimer = h.timerRefused || (h.timerOn && (h.lastTimerTick ? now - h.lastTimerTick > 60 : now - h.timerAt > 20));
    if (noTimer && std::strcmp(via, "calc") == 0 && now != h.lastFallback) { h.lastFallback = now; pass(h, false, true); }   // never requests
}
int DeltaProfileHourly::timer(RTX_EVENT* e) {
    if (!e) return RTX_FAIL;
    HostState* h = nullptr;
    { std::lock_guard<std::mutex> g(regMx()); std::map<int, HostState*>::iterator it = timerReg().find(e->v.timer.id); if (it != timerReg().end()) h = it->second; }
    if (!h) return RTX_FAIL;
    h->lastTimerTick = wallNow();
    // IRT may call a timer in ANOTHER chart's context: a request is made only when the context is this host's own chart
    const char* s = getSymbol();
    const bool contextOk = s && h->symbol == safeField(s) && getSecondsPerBar() == h->spb;
    pass(*h, true, contextOk);
    h->lastTimerTick = wallNow();
    return RTX_OK;
}
int DeltaProfileHourly::calc(int) {
    HostState* h = bind(true); if (!h) return RTX_OK;
    try {
        if (!identify(*h) || h->quarantined) return RTX_OK;
        syncHost(*h);
        pump(*h, "calc");
    } catch (...) { if (++h->faults >= 3) { h->quarantined = true; h->quarantineWhy = "calc faulted 3 times"; } }
    return RTX_OK;
}
void DeltaProfileHourly::seed(Feed& f) {
    f.seeded = true;
    f.eng.reset(new dph::Engine(f.root, f.market, f.spb, f.tick));
    const std::string dir = recordFolder(); if (dir.empty()) { f.recordNote = "no USERPROFILE: no record"; return; }
    const long long cur = dph::sessionKey(localSec(currentDate()));
    for (long long s = cur - dph::HISTORY_SESSIONS + 1; s <= cur; ++s) {   // the last 10 sessions' records: redrawn, never re-decided
        std::string text; bool tooLarge = false;
        if (readWholeFile(dir + dph::recordName(f.market, f.spb, s), text, tooLarge, dph::REC_MAX_BYTES)) f.eng->seedText(text);
        else if (tooLarge) f.recordNote = "a record file is too large - not read";
    }
    std::string text; bool tooLarge = false;                    // where the last run stopped (same contract only)
    if (readWholeFile(dir + f.market + "-" + std::to_string(f.spb) + "-progress-" + f.root + ".txt", text, tooLarge, 4096)) {
        const std::vector<std::string> c = dph::fields(text.substr(0, text.find('\n')));
        long long te = 0;
        if (c.size() == 4 && c[0] == "DPH" && c[2] == f.symbol && dph::parseWhole(c[3], 1, 32503680000LL, te)) f.eng->resumeAfter = te;
    }
}
// requests: from the timer only, in this chart's own context, one at a time across all charts, behind the crash guard
// each market's RTH (Central, minutes of the day) - the first request is kept to the minimum inside it (checklist #18)
static bool inRth(const std::string& market, long long localNow) {
    const long long days = dph::floorDivLL(localNow, 86400), mod = (localNow - days * 86400) / 60;
    const int dow = static_cast<int>(((days % 7) + 7 + 4) % 7);          // 1970-01-01 was a Thursday: 0 = Sunday
    if (dow == 0 || dow == 6) return false;
    const int o = market == "CL" || market == "NG" ? 480 : (market == "GC" || market == "HG" || market == "EU") ? 440 : 510;
    const int c = market == "CL" || market == "NG" ? 810 : (market == "GC" || market == "HG") ? 750 : market == "EU" ? 840 : 900;
    return mod >= o && mod < c;
}
// walk the chart's own bars back `need` seconds of trading time from bar `from`: the index reached and its span
static std::size_t walkBack(const std::vector<long long>& E, std::size_t from, long long need, int spb, long long& span) {
    long long sum = 0; std::size_t k = from; span = spb;
    for (;;) {
        span = k == 0 ? spb : std::min<long long>(spb, E[k] - E[k - 1]);
        sum += span;
        if (sum >= need || k == 0) return k;
        --k;
    }
}
// requests: from the timer only, in this chart's own context, one at a time across all charts, behind the crash guard.
// The registry lock is never held while IRT builds the bars (IRT may call back into this DLL from inside the request).
void DeltaProfileHourly::requests(Feed& f, HostState& h, time_t now) {
    if (!f.guardOpen.empty() && f.req && now - f.reqMade >= GUARD_PROVE_S) guardClear(f);   // IRT lived through it: proven
    if (f.hostEnds.empty()) { f.reqNote = "waiting for the chart's bars"; return; }
    Pace& P = pace();
    char kind = 0; RTDATE start = 0;
    {
        std::lock_guard<std::mutex> g(regMx());
        if (!P.start) P.start = now;
        if (!f.req && f.eng->lastAdded == LLONG_MIN) {                          // the first request
            if (f.baseBlocked) { f.reqNote = "BLOCKED: IRT stopped during this request before"; return; }
            if (f.baseTries >= 3) { f.reqNote = "IRT returned no 1-minute bars (3 tries this run)"; return; }
            if (now - P.start < START_DELAY_S || now < P.nextBaseAt) { f.reqNote = "waiting for its turn"; return; }
            // how far back: the 24-h window before the first minute that still has to be processed -
            //   after a restart: the minute after the last one processed (the record already holds everything before it)
            //   first time ever, outside RTH: 24 h before the window too (~2 sessions), so the last day's marks are re-played
            //   first time ever, inside RTH: the window only (the smallest request that draws the profile)
            const std::vector<long long>& E = f.hostEnds; const long long win = f.eng->P.windowHours * 3600LL;
            std::size_t from = E.size() - 1; long long need = win + f.spb;
            const long long resume = f.eng->resumeAfter;
            if (resume != LLONG_MIN && resume >= E.back() - 2 * win) {
                from = static_cast<std::size_t>(std::lower_bound(E.begin(), E.end(), resume + 1) - E.begin());
                if (from >= E.size()) from = E.size() - 1;
            } else if (!inRth(f.market, localSec(currentDate()))) need = 2 * win + f.spb;
            long long span = f.spb;
            const std::size_t k = walkBack(E, from, need, f.spb, span);
            start = f.hostRaw[k] > static_cast<RTDATE>(span) ? f.hostRaw[k] - static_cast<RTDATE>(span) : 0;
            if (f.eng->coverageFrom == LLONG_MIN) f.eng->coverageFrom = E[k] - span;
            P.nextBaseAt = now + BASE_SPACING_S; ++f.baseTries; kind = 'B';
        } else if (f.req && f.reqCount < 2 && f.eng->lastAdded == LLONG_MIN && now - f.reqMade > 120) {   // nothing came back
            P.nextBaseAt = now + 300;
        } else {
            // IRT does not always grow a request live (IRTReader 0.5.2): when it has stopped while the chart has new trades,
            // ask again from the last minute held (a few minutes of data)
            const bool stalled = f.req && f.reqCount >= 0 && now - f.reqGrew > 120 && now - f.reqMade > 120 && f.hostTickAt > f.reqMade + 60 &&
                                 f.reqNext >= f.reqCount - 1 && f.lastRaw > 0;
            if (!stalled) return;
            if (f.tailBlocked) { f.reqNote = "BLOCKED (tail): IRT stopped during it before"; return; }
            if (now < P.nextTailAt) return;
            P.nextTailAt = now + TAIL_SPACING_S; kind = 'T';
            start = f.lastRaw > 600 ? f.lastRaw - 600 : 0;
        }
    }
    if (f.req) { delete f.req; f.req = nullptr; guardClear(f); }
    if (!kind) { f.reqNote = "no 1-minute bars came back - asking again"; return; }
    guardWrite(f, kind == 'B' ? "base" : "tail");
    f.req = makeRequest(h.symbol.c_str(), start, f.session);
    f.reqKind = kind; f.reqMade = now; f.reqGrew = now; f.reqCount = -1; f.reqNext = 0; ++f.requests;
    f.reqNote = !f.req ? "request could not be made" : kind == 'B' ? "1-minute bars asked" : "live 1-minute bars asked again (IRT stopped growing the last request)";
    if (!f.req) guardClear(f);
}
// closed minutes (never the last, forming one) -> the engine, in time order, in a bounded slice
void DeltaProfileHourly::ingest(Feed& f, std::int64_t deadline) {
    if (!f.req) return;
    RTBARS& b = *f.req;
    const long count = b.count;
    if (count != f.reqCount) { f.reqCount = count; f.reqGrew = wallNow(); }
    if (count < 2 || !b.dt || !b.op || !b.hi || !b.lo || !b.cl || !b.vo) return;
    std::vector<VOLPROFILE> rows;
    int done = 0;
    for (long i = f.reqNext; i < count - 1; ++i) {
        if (done >= MAX_MINUTES_PER_PASS || (done > 0 && steadyMs() > deadline)) break;   // #18 / #40: bounded slices
        const RTDATE raw = static_cast<RTDATE>((*b.dt)[static_cast<int>(i)]);
        const long long te = localSec(raw);
        if (te <= 0) { ++f.mismatched; f.reqNext = i + 1; continue; }
        if (te <= f.eng->lastAdded) { f.reqNext = i + 1; continue; }                 // already held (an overlapping request)
        // its host bar: the first chart bar ending at or after it; a minute the chart has no bar for yet waits
        const std::vector<long long>& E = f.hostEnds;
        const std::vector<long long>::const_iterator at = std::lower_bound(E.begin(), E.end(), te);
        if (at == E.end()) break;
        const std::size_t k = static_cast<std::size_t>(at - E.begin());
        const long long barStart = std::max(k ? E[k - 1] : LLONG_MIN, *at - static_cast<long long>(f.spb));
        if (te <= barStart) { ++f.unmapped; f.reqNext = i + 1; continue; }            // outside the chart's session: not drawn there
        dph::MinuteIn m; m.te = te; m.bucketEnd = *at;
        BARSTATISTICS stats;
        const int prices = readStatistics(&b, static_cast<int>(i), &stats);
        int read = prices;
        if (prices > 0) { rows.resize(static_cast<std::size_t>(prices)); read = readPrices(&b, static_cast<int>(i), &rows[0], prices); }
        const double vol = static_cast<double>((*b.vo)[static_cast<int>(i)]);
        if (prices == -1 || read == -1) { f.err = "VAP fault on a 1-minute bar"; break; }
        if (prices < 0 || read < 0 || (prices == 0 && vol > 0)) {
            // IRT has no volume at price for it yet: retry for 30 s, then take it as missing (counted, provisional window)
            const time_t now = wallNow();
            if (f.failTe != te) { f.failTe = te; f.failSince = now; }
            if (now - f.failSince < 30) break;
            m.missing = true; ++f.missingAccepted;
        } else {
            m.o = (*b.op)[static_cast<int>(i)]; m.h = (*b.hi)[static_cast<int>(i)]; m.l = (*b.lo)[static_cast<int>(i)]; m.c = (*b.cl)[static_cast<int>(i)];
            m.volume = vol;
            double buys = 0, sells = 0, total = 0;
            for (int k2 = 0; k2 < read; ++k2) {
                const VOLPROFILE& v = rows[static_cast<std::size_t>(k2)];
                if (!std::isfinite(v.price) || v.buyVolume < 0 || v.sellVolume < 0 || v.totalVolume < 0) { ++f.mismatched; continue; }
                const double buy = static_cast<double>(v.buyVolume), sell = static_cast<double>(v.sellVolume);   // #2: long -> double first
                double tv = static_cast<double>(v.totalVolume);
                if (buy + sell > tv + 0.5) { ++f.mismatched; tv = buy + sell; }     // tolerant: counted, row kept (2.4.2)
                buys += buy; sells += sell; total += tv;
                if (tv == 0 && buy == 0 && sell == 0) continue;
                try { m.rows.push_back(dph::Row(dph::priceKey(v.price, f.tick), buy - sell, tv)); }
                catch (...) { ++f.mismatched; }
            }
            if (std::fabs(total - static_cast<double>(stats.totalVolume)) > 0.5 || std::fabs(buys - static_cast<double>(stats.buyVolume)) > 0.5 ||
                std::fabs(sells - static_cast<double>(stats.sellVolume)) > 0.5) ++f.mismatched;
        }
        f.failTe = LLONG_MIN;
        f.eng->add(std::move(m));
        if (f.firstMinute == LLONG_MIN) f.firstMinute = te;
        f.reqNext = i + 1; f.lastRaw = raw; f.lastIngestAt = wallNow(); ++done;
    }
}
void DeltaProfileHourly::pass(HostState& h, bool fromTimer, bool contextOk) {
    if (h.quarantined || !h.feed) return;
    Feed& f = *h.feed;
    std::unique_lock<std::recursive_mutex> lock(f.mx, std::try_to_lock);
    if (!lock.owns_lock()) return;
    const std::int64_t t0 = steadyMs(), deadline = t0 + SLICE_MS;
    try {
        if (!f.guardsRead) readGuards(f);
        if (!f.seeded) seed(f);
        const time_t now = wallNow();
        if (fromTimer && contextOk) requests(f, h, now);
        ingest(f, deadline);
        f.eng->run(MAX_STEPS_PER_PASS, [deadline]() { return steadyMs() <= deadline; });
        flushJournal(f);
        writeProgress(f, false);
        // checklist #12 proof: each CLOSED chart bar's volume vs the sum of its minutes (aggregated while it is in the window)
        f.volChecked = 0; f.volExact = 0; f.volMaxErr = 0;
        for (std::map<long long, dph::Agg>::const_iterator a = f.eng->aggs.begin(); a != f.eng->aggs.end(); ++a) {
            if (a->first > f.eng->cursor || a->second.missing) continue;
            const std::vector<long long>::const_iterator at = std::lower_bound(f.hostEnds.begin(), f.hostEnds.end(), a->first);
            if (at == f.hostEnds.end() || *at != a->first || a->second.firstTe <= f.firstMinute) continue;   // whole bars only
            const double hv = f.hostVol[static_cast<std::size_t>(at - f.hostEnds.begin())], err = std::fabs(hv - a->second.volume);
            ++f.volChecked; if (err < 0.5) ++f.volExact; f.volMaxErr = std::max(f.volMaxErr, err);
        }
        ++f.passes;
        const int ms = static_cast<int>(steadyMs() - t0); f.maxPassMs = std::max(f.maxPassMs, ms);
        status(f, h, false);
        h.faults = 0;
    } catch (const std::exception& e) {
        f.err = std::string("pass failed: ") + e.what();
        if (++h.faults >= 3) { h.quarantined = true; h.quarantineWhy = f.err; }
    } catch (...) {
        f.err = "pass failed";
        if (++h.faults >= 3) { h.quarantined = true; h.quarantineWhy = f.err; }
    }
}
void DeltaProfileHourly::status(Feed& f, HostState& h, bool force) {
    const std::string dir = userFolder(); if (dir.empty() || !f.eng) return;
    const dph::Engine& e = *f.eng;
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << dph::DPH_VERSION << "\nPARAMS," << dphp::PARAMS_ID << "\nROOT," << f.root << "\nMARKET," << f.market
      << "\nSYMBOL," << f.symbol << "\nSPB," << f.spb << "\nSESSION," << f.session << "\nMODE," << (proven(f.market) ? "LIVE" : "TESTING")
      << "\nSTATE," << safeField(e.quarantined ? e.state : h.quarantined ? "quarantined: " + h.quarantineWhy : e.state)
      << "\nREQUEST," << safeField(f.reqNote) << ",kind " << (f.reqKind ? f.reqKind : '-') << ",asked " << f.requests
      << ",bars " << f.reqCount << (f.baseBlocked ? ",BASE BLOCKED" : "") << (f.tailBlocked ? ",TAIL BLOCKED" : "")
      << "\nMINUTES,added " << e.added << ",rejected " << e.rejected << ",missing " << e.missingMinutes << ",unmapped " << f.unmapped
      << ",vap_mismatch " << f.mismatched << ",last " << e.lastAdded << ",processed " << e.cursor << ",waiting " << e.pending.size()
      << "\nWINDOW,bars " << e.winEnds.size() << ",complete " << (e.complete ? 1 : 0) << ",provisional " << (e.provisional ? 1 : 0)
      << ",rows " << e.snap.buckets.size() << ",builds " << e.builds
      << "\nBAR_VOLUME_CHECK,bars " << f.volChecked << ",exact " << f.volExact << ",max_error " << f.volMaxErr
      << "\nRECORD," << safeField(recordFolder()) << ",loaded " << e.seedLoaded << ",rejected " << e.seedRejected << ",F " << e.linesF
      << ",D " << e.linesD << ",pending " << e.journal.size() << ",resume_after " << e.resumeAfter << ',' << safeField(f.recordNote)
      << "\nDRAWN,rows " << h.drawnRows << ",rings " << h.ringsDrawn << ",letters " << h.lettersDrawn << ",suppressed " << h.suppressed
      << "\nTIMING,max_pass_ms " << f.maxPassMs << "\nERROR," << safeField(f.err) << '\n';
    for (const dph::Latch& L : e.latches)
        if (e.live(L)) o << "SIGNAL," << L.code << ',' << (L.support ? "support" : "resistance") << ',' << L.low * f.tick << ','
                         << L.high * f.tick << ",bar " << L.peakEnd << '\n';
    const std::string body = o.str(); const time_t now = wallNow();
    // checklist #31: only when the content changed, and then at most every 5 s (atomic)
    if (!force && (body == f.statusBody || now - f.statusAt < 5)) return;
    if (atomicWrite(dir + "DeltaProfileHourly.status-" + f.market + "-" + std::to_string(f.spb) + "s.txt", body)) { f.statusBody = body; f.statusAt = now; }
}
int DeltaProfileHourly::dealerWidth(HostState& h) {
    const std::int64_t now = steadyMs();
    if (h.dealerAt >= 0 && now - h.dealerAt < 1000) return h.dealerReach;
    h.dealerAt = now;
    const std::string directory = userFolder(); if (directory.empty()) return h.dealerReach;
    const std::string path = directory + "DealerProfile.status.txt";
    const long long stamp = fileStamp(path);
    if (stamp == h.dealerStamp) return h.dealerReach;                                     // #31: parsed only when it changed
    h.dealerStamp = stamp;
    std::string text; bool tooLarge = false;
    if (!readWholeFile(path, text, tooLarge, 64 * 1024)) return h.dealerReach;           // #33: capped
    std::istringstream in(text); std::string line, market; long long reach = 0;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::size_t c = line.find(',');
        if (c == std::string::npos) continue;
        const std::size_t c2 = line.find(',', c + 1);
        const std::string k = line.substr(0, c), v = line.substr(c + 1, c2 == std::string::npos ? std::string::npos : c2 - c - 1);
        if (k == "MARKET") market = v;
        else if (k == "REACH") { long long x = 0; if (dph::parseWhole(v, 0, 1199, x)) reach = x; }
    }
    if (market == h.market && reach > 0) h.dealerReach = static_cast<int>(reach);
    return h.dealerReach;
}
void DeltaProfileHourly::textRight(int right, int y, const std::string& text, COLOR color, int font, bool bold) {
    if (text.empty()) return;
    FONT f; f.id = HELVETICA; f.size = coordinate(font); f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(color);
    const int yy = y - static_cast<int>(font * 0.45 + 0.5);
    RCT rect; rect.set(coordinate(right - 400), coordinate(yy - font), coordinate(right), coordinate(yy + font));
    rect.drawText(text.c_str(), false, true);
}
void DeltaProfileHourly::textLeft(int left, int y, const std::string& text, COLOR color, int font, bool bold) {
    if (text.empty()) return;
    FONT f; f.id = HELVETICA; f.size = coordinate(font); f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(color);
    const int yy = y - static_cast<int>(font * 0.45 + 0.5);
    RCT rect; rect.set(coordinate(left), coordinate(yy - font), coordinate(left + 400), coordinate(yy + font));
    rect.drawText(text.c_str(), false, false);
}
void DeltaProfileHourly::textCentre(int x, int y, const std::string& text, COLOR color, int font, bool bold) {
    if (text.empty()) return;
    FONT f; f.id = HELVETICA; f.size = coordinate(font); f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(color);
    const int w = getTextWidth(text.c_str(), -1);
    RCT rect; rect.set(coordinate(x - w / 2 - 2), coordinate(y - font), coordinate(x + w / 2 + 3), coordinate(y + font));
    rect.drawText(text.c_str(), true, false);
}
void DeltaProfileHourly::stroke(int x1, int y1, int x2, int y2, COLOR color, int width) {
    setPen(color, coordinate(width), P_SOLID);
    PNT a; a.set(0, 0.0f); a.h = coordinate(x1); a.v = coordinate(y1); a.setDrawPosition();
    PNT b; b.set(0, 0.0f); b.h = coordinate(x2); b.v = coordinate(y2); b.drawLineTo();
}
void DeltaProfileHourly::box(int left, int top, int right, int bottom, COLOR color) {
    if (right <= left || bottom <= top) return;
    RCT rect; rect.set(coordinate(left), coordinate(top), coordinate(right), coordinate(bottom));
    rect.draw(0, color, color, DRAW_OPAQUE, PAT_SOLID);
}
static std::string amount(double d) {
    std::ostringstream out; out.imbue(std::locale::classic()); out << (d > 0 ? '+' : '-');
    if (std::fabs(d) >= 1000) out << std::fixed << std::setprecision(1) << std::fabs(d) / 1000 << 'k';
    else out << std::fixed << std::setprecision(0) << std::fabs(d);
    return out.str();
}
static std::string ratioText(double x) {
    std::ostringstream out; out.imbue(std::locale::classic()); out << std::fixed << std::setprecision(x < 10 ? 1 : 0) << x;
    std::string s = out.str();
    if (s.size() > 2 && s.substr(s.size() - 2) == ".0") s.resize(s.size() - 2);
    return s + 'x';
}
struct MarkBox { int l, t, r, b; };
static bool overlaps(const MarkBox& a, const MarkBox& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }

bool DeltaProfileHourly::render(HostState& h) {
    h.ringsDrawn = h.lettersDrawn = h.suppressed = h.drawnRows = 0;
    RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
    int paneRight = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) paneRight = scale.left - 2;
    const int base = paneRight - 2 - dealerWidth(h) - L_GAP;
    const int left = base - L_WIDTH;
    const int top = pane.top + 16, bottom = pane.bottom;
    if (left <= pane.left + 40 || base >= paneRight || bottom - top < 40) { h.state = "pane too small: not drawn"; return false; }   // #35
    Feed* f = h.feed.get();
    const bool live = f && proven(f->market);
    textRight(base, pane.top + 8, std::string("DLT-H ") + DPH_VERSION_TEXT + (live ? "" : " TESTING"), C_INK, 8, true);
    if (!f) { textRight(base - 2, top + 12, "NO 1M", C_SELL, 8, true); return false; }
    std::unique_lock<std::recursive_mutex> lock(f->mx, std::try_to_lock);
    if (!lock.owns_lock() || !f->eng) { textRight(base - 2, top + 12, "NO 1M", C_SELL, 8, true); return false; }
    const dph::Engine& e = *f->eng;
    const char* badge = f->baseBlocked && e.lastAdded == LLONG_MIN ? "BLOCKED" : !e.ready ? "NO 1M" :
        (!e.complete || e.provisional) ? "PARTIAL" :
        (f->hostTickAt && wallNow() - f->hostTickAt < 120 && f->lastIngestAt && wallNow() - f->lastIngestAt > 600) ? "STALE" : "";
    if (badge[0]) textRight(base - 2, top + 12, badge, badge[0] == 'N' || badge[0] == 'B' ? C_SELL : C_INK, 8, true);
    if (!e.ready) return false;
    stroke(base, top, base, bottom, C_SLATE, 1);
    const dph::Snapshot& profile = e.snap;
    const int lastBar = static_cast<int>(h.barCount) - 1, full = L_WIDTH - 2;
    if (lastBar < 0) return false;
    // ---- the profile (2.5.4 drawing)
    std::vector<std::size_t> ranked;
    for (std::size_t i = 0; i < profile.buckets.size(); ++i) ranked.push_back(i);
    std::sort(ranked.begin(), ranked.end(), [&profile](std::size_t a, std::size_t b) {
        const double x = std::fabs(profile.buckets[a].delta), y = std::fabs(profile.buckets[b].delta);
        return x != y ? x > y : profile.buckets[a].key < profile.buckets[b].key;
    });
    std::map<dph::Tick, int> edge;
    struct VisibleNode { const dph::Node* node; int y, edge; };
    std::vector<VisibleNode> nodes;
    for (std::size_t i = 0; i < profile.buckets.size(); ++i) {
        const dph::Bucket& b = profile.buckets[i];
        const double lower = (static_cast<double>(b.key) * static_cast<double>(profile.ticksPerRow) - 0.5) * profile.tick;
        const double upper = (static_cast<double>(b.key + 1) * static_cast<double>(profile.ticksPerRow) - 0.5) * profile.tick;
        int yt = yOf(lastBar, upper), yb = yOf(lastBar, lower);
        if (yb <= yt) yb = yt + 1;
        if (yb <= top || yt >= bottom) continue;
        yt = std::max(top, yt); yb = std::min(bottom, yb);
        ++h.drawnRows;
        const int dl = profile.maxAbsDelta > 0 ? std::min(full, std::max(b.delta != 0 ? 1 : 0,
            static_cast<int>(std::fabs(b.delta) / profile.maxAbsDelta * full))) : 0;
        const int vl = profile.maxVolume > 0 ? std::min(full, static_cast<int>(b.volume / profile.maxVolume * full)) : 0;
        box(base - vl, yt, base, yb, C_SLATE);
        if (b.delta != 0) box(base - dl, yt, base, yb, b.delta > 0 ? C_BUY : C_SELL);
        int ex = base - dl;
        const dph::Node* node = NULL;
        for (std::size_t j = 0; j < profile.nodes.size(); ++j) if (profile.nodes[j].bucket == b.key) node = &profile.nodes[j];
        bool showAmount = node != NULL;
        for (std::size_t j = 0; j < ranked.size() && j < 3; ++j) if (ranked[j] == i) showAmount = true;
        if (showAmount && b.delta != 0) {
            const std::string label = amount(b.delta);
            const int width = textWidth(label, L_FONT - 1, false);
            if (ex - 2 - width >= pane.left + 2) { textRight(ex - 2, (yt + yb) / 2, label, C_INK, L_FONT - 1, false); ex -= width + 2; }
        }
        edge[b.key] = ex;
        if (node) { VisibleNode v; v.node = node; v.y = (yt + yb) / 2; v.edge = ex; nodes.push_back(v); }
    }
    std::vector<MarkBox> taken;                           // every label drawn: nothing may overlap it
    std::sort(nodes.begin(), nodes.end(), [](const VisibleNode& a, const VisibleNode& b) { return a.y < b.y; });
    std::vector<int> used;
    for (const VisibleNode& v : nodes) {
        const dph::Node& n = *v.node;
        const bool absorbing = n.state.code == "A" || n.state.code == "A?";
        std::string label = n.state.code + " " + ratioText(n.ratio);
        int width = textWidth(label, L_FONT, true);
        if (v.edge - 4 - width < pane.left + 2 && absorbing) { label = n.state.code; width = textWidth(label, L_FONT, true); }
        if (v.edge - 4 - width < pane.left + 2) continue;
        int y = v.y;
        if (!used.empty() && y - used.back() < L_FONT + 2) y = used.back() + L_FONT + 2;
        if (y + L_FONT / 2 >= bottom || y - L_FONT / 2 <= top) { if (!absorbing) continue; y = std::max(top + L_FONT / 2 + 1, std::min(bottom - L_FONT / 2 - 1, v.y)); }
        const COLOR color = n.state.support ? C_SUPPORT : C_RESISTANCE;
        textRight(v.edge - 4, y, label, color, L_FONT, true); used.push_back(y);
        taken.push_back(MarkBox{v.edge - 4 - width, y - L_FONT, v.edge - 4, y + L_FONT});
        if (y != v.y) stroke(v.edge - 2, v.y, v.edge - 2, y, color, 1);
    }
    for (const dph::Zone& z : profile.zones) {
        if (z.code != "A" && z.code != "A?") continue;   // 2.5.0: absorption only
        int yt = yOf(lastBar, (static_cast<double>(z.high) + 0.5) * profile.tick);
        int yb = yOf(lastBar, (static_cast<double>(z.low) - 0.5) * profile.tick);
        if (yb <= top || yt >= bottom) continue;
        yt = std::max(top, yt); yb = std::min(bottom, yb); if (yb <= yt) continue;
        int ex = base; bool any = false;
        for (const dph::Bucket& b : profile.buckets) {
            if (b.high < z.low || b.low > z.high) continue;
            const std::map<dph::Tick, int>::const_iterator at = edge.find(b.key);
            if (at != edge.end()) { ex = any ? std::min(ex, at->second) : at->second; any = true; }
        }
        if (!any) continue;
        const std::string label = z.code; const int width = textWidth(label, L_FONT, true);
        int y = (yt + yb) / 2; bool collision = false;
        for (int u : used) if (std::abs(y - u) < L_FONT + 2) collision = true;
        for (int tries = 0; tries < 6 && collision; ++tries) {
            y += (tries % 2 ? -1 : 1) * (tries / 2 + 1) * (L_FONT + 2);
            collision = false; for (int u : used) if (std::abs(y - u) < L_FONT + 2) collision = true;
        }
        if (collision || ex - 7 - width < pane.left + 2 || y - L_FONT / 2 <= top || y + L_FONT / 2 >= bottom) continue;
        const COLOR color = z.support ? C_SUPPORT : C_RESISTANCE;
        stroke(ex - 4, yt, ex - 4, yb, color, 1); stroke(ex - 4, yt, ex - 1, yt, color, 1); stroke(ex - 4, yb, ex - 1, yb, color, 1);
        textRight(ex - 7, y, label, color, L_FONT, true); used.push_back(y);
        taken.push_back(MarkBox{ex - 7 - width, y - L_FONT, ex, y + L_FONT});
    }
    // ---- the chart marks: one solid circle + its letter, on the candle where the candidate's delta peaked (by bar TIME,
    // never by pixel), at most one per bar (the biggest |net delta|), both or neither (1:1), never over the profile
    int v0 = 0, v1 = lastBar;
    if (getVisibleBars(&v0, &v1) != RTX_OK || v0 < 0 || v1 < v0) { v0 = 0; v1 = lastBar; }
    v1 = std::min(v1, lastBar);
    std::map<int, const dph::Latch*> perBar;
    for (const dph::Latch& L : e.latches) {
        if (L.code != "A" && L.code != "A?" && L.code != "I") continue;
        const std::vector<long long>::const_iterator at = std::lower_bound(h.ends.begin(), h.ends.end(), L.peakEnd);
        if (at == h.ends.end() || *at != L.peakEnd) continue;
        const int bar = static_cast<int>(at - h.ends.begin());
        if (bar < v0 || bar > v1) continue;
        const dph::Latch*& best = perBar[bar];
        if (!best) { best = &L; continue; }
        ++h.suppressed;
        const bool better = std::fabs(L.net) != std::fabs(best->net) ? std::fabs(L.net) > std::fabs(best->net) :
            dph::decidedCode(L.code) != dph::decidedCode(best->code) ? dph::decidedCode(L.code) : L.id < best->id;
        if (better) best = &L;
    }
    for (const std::pair<const int, const dph::Latch*>& m : perBar) {
        const int bar = m.first; const dph::Latch& L = *m.second;
        PNT c; c.set(bar, static_cast<float>(L.price), kBarCenter);
        PNT n; n.set(bar > 0 ? bar - 1 : bar + 1, static_cast<float>(L.price), kBarCenter);
        const int spacing = std::max(1, std::abs(static_cast<int>(n.h) - static_cast<int>(c.h)));
        const int r = std::max(2, std::min(6, spacing / 2 - 1));                    // #13: inside its own candle's width
        const int font = 8; const std::string letter = L.code; const int w = textWidth(letter, font, true);
        const int ly = c.v - r - 2 - font / 2 - 1;                                   // footprint marks go ABOVE (options marks below)
        const MarkBox ring{c.h - r, c.v - r, c.h + r + 1, c.v + r + 1}, text{c.h - w / 2 - 1, ly - font / 2 - 1, c.h + w / 2 + 2, ly + font / 2 + 1};
        const MarkBox both{std::min(ring.l, text.l), std::min(ring.t, text.t), std::max(ring.r, text.r), std::max(ring.b, text.b)};
        bool clash = both.l < pane.left || both.r > left - 4 || both.t < top || both.b > bottom;
        for (const MarkBox& t : taken) if (overlaps(both, t)) clash = true;
        if (clash) { ++h.suppressed; continue; }
        const COLOR color = L.support ? C_SUPPORT : C_RESISTANCE;
        setPen(color, 2, P_SOLID); CBRUSH brush(color, PAT_HOLLOW); brush.set();
        RCT o; o.set(coordinate(c.h - r), coordinate(c.v - r), coordinate(c.h + r), coordinate(c.v + r)); o.drawOval(DRAW_OPAQUE);
        textCentre(c.h, ly, letter, color, font, true);
        ++h.ringsDrawn; ++h.lettersDrawn; taken.push_back(both);
    }
    return true;
}
int DeltaProfileHourly::draw() {
    HostState* h = bind(true); if (!h) return RTX_OK;
    try {
        ++h->draws;
        if (!identify(*h)) {
            RCT pane; pane.getPaneRect(false);
            textLeft(pane.left + 4, pane.top + 8, std::string("DLT-H ") + DPH_VERSION_TEXT, C_INK, 8, true);
            return RTX_OK;
        }
        if (h->quarantined) return RTX_OK;
        syncHost(*h);
        render(*h);                       // draw never requests and never rebuilds: it shows the last closed-minute snapshot
        h->faults = 0;
        return RTX_OK;
    } catch (...) {
        if (++h->faults >= 3) { h->quarantined = true; h->quarantineWhy = "draw faulted 3 times"; }
        return RTX_FAIL;
    }
}
int DeltaProfileHourly::done() {                 // symbol / periodicity change: release this chart's feed and timer
    HostState* h = bind(false); if (!h) return RTX_OK;
    releaseHost(*h);
    h->barCount = -1; h->ends.clear(); h->faults = 0; h->quarantined = false;
    return RTX_OK;
}
int DeltaProfileHourly::destroy() {
    HostState* h = bind(false); if (!h) return RTX_OK;
    releaseHost(*h);
    own_ = nullptr;
    slot_.release(this);                         // deletes this chart's state, setUserData(NULL)
    return RTX_OK;
}
extern "C" cppExtension* CreateExtension() {
    DeltaProfileHourly* p = new DeltaProfileHourly();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA Delta Profile Hourly - DLT-H");
    p->setVersion("1.0.0");                        // a literal: gex-build's :ver prints it - the tests check it equals dph::DPH_VERSION
    return p;
}
