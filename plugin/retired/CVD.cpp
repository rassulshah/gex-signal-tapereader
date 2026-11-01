/*******************************************************************************
 * lsCVD 1.2.0 (2026-10-14, Rassul approved the external-audit display list at 18:25 CT, section 13): the shown (confirmed) marks read
 * "Div?" / "Exh?" / "BR?" - a small test sample (+0.26R on 99). Filled box, colours, placement and the record unchanged.
 * lsCVD 1.1.0 -- one pane: CVD line + delta % histogram, with Div / Exh / BR marks on the CVD line (Rassul 2026-10-12
 * "implement new cvd delta indicator", from the outside package CVD Signals v0.1 + its approved mockup).
 *
 *   CvdEngine.h  the reviewed signal core (whole-tick compares, stable IDs)       CvdFeed.h  sessions, volume, levels, per-market scale
 *   Closed bars only; each bar is fed to the engine exactly once and nothing already drawn ever changes (BUILD-LESSONS #11, #27, #28).
 *   Green = bullish setup (drawn below the line), red = bearish (above). 'Div?' outlined = warning, 'Div' filled = price + flow
 *   confirmed, grey 'Div x' = expired / invalidated. All four reads are TESTING (study 2026-10-12: confirmed +0.04R, n 213).
 *   Writes: lsFlexLevels\CVD.status-<MKT>-<spb>.txt and lsFlexLevels\CVD\LRA-CVD2-events-<MKT>-<spb>-<contract>-<session>.csv (the study log).
 *   1.1.0 (2026-10-12 'i want it simplified and improved'): the chart shows only CONFIRMED marks at prior-day / prior-session levels
 *   with a stop within 4.0x the median bar range (CvdFeed.h 'Shown': held on unseen days, +0.26R on 99 vs -0.07R for the rest).
 *   No '?' warnings and no grey x on the chart - every read is still logged. Abs off. Reads lsFlexLevels\LRA-CVD-ranges.csv (nightly).
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
#define LSPERF_PLUGIN "CVD"
#endif
#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG: the settings window never faults, never opens blank
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "CvdFeed.h"
#include "HostSlot.h"
#include "CVDSettings.h"     // (1.3.0) the settings table (show / hide + colour per drawn element)
#include "SettingsHost.h"

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

#define CVD_VERSION "1.3.1"   // 1.3.0 (2026-10-18, Rassul "I'll go with your recommendation") a settings window: CVD line / Delta % bars / Marks each with Show + colour(s), kept per market in CVD.settings-<MKT>.txt (CVDSettings.h / SettingsHost.h); defaults = what 1.2.0 drew;  // 1.2.0 (2026-10-14 audit display list) the confirmed marks read Div? / Exh? / BR?

static const COLOR C_BULL = 0x0034D399;       // green  (bullish setup)
static const COLOR C_BEAR = 0x00F87171;       // red    (bearish setup)
static const COLOR C_GREY = 0x009CA3AF;
static const COLOR C_LINE = 0x0060A5FA;       // CVD line, neutral blue
static const COLOR C_HBULL = 0x00205C49;      // delta % bars, muted
static const COLOR C_HBEAR = 0x005E2B35;
static const COLOR C_AXIS = 0x00334155;
static const COLOR C_DARK = 0x000B1220;       // text on a filled (confirmed) label
static const long PASS_BARS = 3000;           // work cap per calc pass (BUILD-LESSONS #40)
static std::mutex g_fileMutex;

static long long steadyMs() {
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static short coord(long v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
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
static bool statFile(const std::string& p, long long* stamp) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return false;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
#endif
    if ((st.st_mode & S_IFMT) != S_IFREG) return false;
    if (stamp) *stamp = (long long)st.st_mtime * 1000000LL + (long long)st.st_size;
    return true;
}
static std::string trim(const std::string& in) {
    size_t a = 0, b = in.size();
    while (a < b && std::isspace((unsigned char)in[a])) ++a;
    while (b > a && std::isspace((unsigned char)in[b - 1])) --b;
    return in.substr(a, b - a);
}
static std::string upper(std::string s) { for (size_t i = 0; i < s.size(); ++i) s[i] = (char)std::toupper((unsigned char)s[i]); return s; }
static bool parseD(const std::string& text, double& out) {
    const std::string s = trim(text);
    if (s.empty() || s.size() > 40) return false;
    errno = 0; char* end = 0; const double v = std::strtod(s.c_str(), &end);
    if (errno || end != s.c_str() + s.size() || !std::isfinite(v)) return false;
    out = v; return true;
}
static std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> v; std::string f;
    for (size_t i = 0; i < s.size(); ++i) { if (s[i] == d) { v.push_back(f); f.clear(); } else f += s[i]; }
    v.push_back(f); return v;
}
static std::string safeFilePart(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) { const unsigned char c = (unsigned char)s[i]; out += (std::isalnum(c) || c == '-' || c == '_') ? (char)c : '_'; }
    return out.empty() ? "unknown" : out;
}
// the chart's market from its root symbol (Rithmic / CQG roots; micros are not supported - their volume is a different tape)
static std::string marketOf(const std::string& rawRoot) {
    const std::string r = upper(trim(rawRoot));
    if (r == "ES" || r == "EP") return "ES";
    if (r == "NQ" || r == "ENQ") return "NQ";
    if (r == "CL" || r == "CLE" || r == "QCL") return "CL";
    if (r == "GC" || r == "GCE" || r == "QGC") return "GC";
    if (r == "HG" || r == "CPE" || r == "QHG" || r == "CP") return "HG";
    if (r == "NG" || r == "NGE") return "NG";
    if (r == "6E" || r == "EU" || r == "E6" || r == "EU6") return "EU";
    return "";
}
// the market of a MenthorQLevels.csv symbol (same table as lsLiquidityProfile 0.11.2)
static std::string mqMarket(const std::string& sym) {
    static const char* const map[][2] = { {"ENQ", "NQ"}, {"EP", "ES"}, {"CLE", "CL"}, {"GCE", "GC"}, {"CPE", "HG"}, {"NGE", "NG"}, {"EU6", "EU"},
                                          {"NQ", "NQ"}, {"ES", "ES"}, {"CL", "CL"}, {"QCL", "CL"}, {"GC", "GC"}, {"QGC", "GC"}, {"HG", "HG"}, {"QHG", "HG"},
                                          {"NG", "NG"}, {"6E", "EU"} };
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); ++i) {
        const std::string r = map[i][0];
        if (sym.size() >= r.size() + 2 && sym.compare(0, r.size(), r) == 0 && std::strchr("FGHJKMNQUVXZ", sym[r.size()])) return map[i][1];
    }
    return "";
}
static bool gamma1(const std::string& part) {
    const std::string p = trim(part);
    return p.compare(0, 2, "CR") == 0 || p.compare(0, 2, "PS") == 0 || p.compare(0, 3, "HVL") == 0;
}
static std::string withSign(double v, int dec = 0) {
    if (!std::isfinite(v)) return "-";
    char b[40]; std::snprintf(b, sizeof(b), "%.*f", dec, std::fabs(v));
    std::string s = b, ip = s.substr(0, s.find('.')), fp = s.find('.') == std::string::npos ? "" : s.substr(s.find('.'));
    std::string g; for (size_t i = 0; i < ip.size(); ++i) { if (i && (ip.size() - i) % 3 == 0) g += ','; g += ip[i]; }
    return (v > 0 ? "+" : v < 0 ? "-" : "") + g + fp;
}

struct RuntimeState {
    std::recursive_mutex mutex;
    bool identified = false, fault = false;
    std::string identity, market, contract, state = "STARTING";
    double tick = 0; int spb = 0;
    cvdf::Feed feed;
    long nextIndex = -1; long long lastRaw = 0;
    long long failRaw = 0, failSinceMs = 0;
    long long mqStamp = -2; long newestSess = -1;
    std::set<std::string> journalKnown, journalLoaded; long long journalFailures = 0;
    std::vector<cvdf::Mark> journalRetry;
    long long rangeStamp = -2, verdictStamp = -2; std::string verdict = "testing";   // lra.cvd_nightly: testing / proven / retired
    long long lastStatusMs = -1; std::string lastStatusText;
    ss::Store setStore; ss::Values set = ss::defaults(cvdset::spec(), -1);   // (1.3.0) CVD.settings-<MKT>.txt (re-read by stamp, at most once a second)
};
static ss::Ids& cvdIds() { static ss::Ids I; return I; }   // the row ids IRT gave the table (one DLL = one parameter list, written once in setup)
static std::string cvdSettingsPath(const std::string& mkt) { const std::string f = flexDir(); return f.empty() || mkt.empty() ? std::string() : f + "\\CVD.settings-" + mkt + ".txt"; }

class CVD : public cppExtension {
public:
    SS_SAFE_DIALOG(CVD, "CVD")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    CVD() : cppExtension() {}
    virtual int draw(void);
    // (1.3.0) the settings window: filled from the market's file (opening never saves); Apply / a change saves; the chart copy never saves
    virtual int parmsLoadBody(void) {
        if (!ss::inDialog(this)) return RTX_OK;
        ss::Store* st = dialogStore(); if (!st) return RTX_OK;
        ss::fillDialog(this, cvdset::spec(), cvdIds(), st->load(ss::marketIndex(dlgMkt_), steadyMs(), true), snap_);
        return RTX_OK;
    }
    virtual int parmsApplyBody(void) { saveSettings(); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::onUpdate(this, cvdset::spec(), cvdIds(), (int)i, snap_); saveSettings(); return RTX_OK; }
    void pump();
    void release() { slot_.release(this); }
    RuntimeState* stateFor() { return slot_.get(this, false); }   // tests
private:
    HostSlot<RuntimeState> slot_;
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        char b[64]; std::memset(b, 0, sizeof(b)); const char* rs = getRootSymbol(b); const std::string mkt = marketOf(rs ? rs : "");
        const std::string p = cvdSettingsPath(mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &cvdset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, cvdset::spec(), cvdIds(), st->load(ss::marketIndex(dlgMkt_), steadyMs(), true), snap_);   // untouched -> the file's value
        makeDir(flexDir());
        if (st->save(V)) snap_.take(ss::dialogNow(this, cvdset::spec(), cvdIds(), V));
    }
    void loadSettings(RuntimeState& s);
    void identify(RuntimeState& s);
    bool civil(RTDATE raw, long& days, int& endSec);
    bool readFlow(RTARRAYP& vap, int bar, double& buy, double& sell);
    void loadMq(RuntimeState& s);
    void loadRanges(RuntimeState& s);
    void loadVerdict(RuntimeState& s);
    void calcBars(RuntimeState& s, long count);
    void journal(RuntimeState& s);
    void writeStatus(RuntimeState& s);
    void render(const RuntimeState& s);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { static_cast<CVD*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<CVD*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) { ss::registerAll(this, cvdset::spec(), cvdIds()); return RTX_OK; }   // (1.3.0) the rows from CVDSettings.h
int cppExtension::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<CVD*>(this)->pump(); return RTX_OK; }
int CVD::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    RuntimeState* s = slot_.get(this, false);
    if (!s) return RTX_OK;
    std::unique_lock<std::recursive_mutex> lock(s->mutex, std::try_to_lock);
    if (!lock.owns_lock()) return RTX_OK;
    try { render(*s); } catch (...) { }   // drawing never calculates and never writes files
    return RTX_OK;
}

bool CVD::civil(RTDATE raw, long& days, int& endSec) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(raw, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31 || t.tm_hour < 0 || t.tm_hour > 23 ||
        t.tm_min < 0 || t.tm_min > 59 || t.tm_sec < 0 || t.tm_sec > 59) return false;
    days = svl::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday); endSec = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
    return true;
}

void CVD::identify(RuntimeState& s) {
    char rootBuffer[64]; std::memset(rootBuffer, 0, sizeof(rootBuffer));
    const char* rootPtr = getRootSymbol(rootBuffer); const char* contractPtr = getSymbol();
    const std::string root = rootPtr ? trim(rootPtr) : std::string(), contract = contractPtr ? trim(contractPtr) : std::string();
    const int spb = getSecondsPerBar(); const double tick = (double)getProperty(SYM_TICKINCR);
    const std::string id = root + "|" + contract + "|" + std::to_string(spb) + "|" + std::to_string(tick);
    if (s.identified && id == s.identity) return;
    // a new symbol / timeframe / reload: start over from the chart's own history
    s.identified = true; s.identity = id; s.fault = false; s.feed = cvdf::Feed(); s.nextIndex = -1; s.lastRaw = 0; s.failRaw = 0;
    s.mqStamp = -2; s.rangeStamp = -2; s.verdictStamp = -2; s.verdict = "testing"; s.newestSess = -1; s.journalKnown.clear(); s.journalLoaded.clear(); s.journalRetry.clear(); s.lastStatusText.clear();
    s.market = marketOf(root); s.contract = contract; s.spb = spb; s.tick = tick;
    if (s.market.empty()) { s.fault = true; s.state = "UNKNOWN MARKET " + root; return; }
    if (!(std::isfinite(tick) && tick > 0)) { s.fault = true; s.state = "NO TICK SIZE"; return; }
    if (spb <= 0 || spb > 3600) { s.fault = true; s.state = "NEEDS A TIME CHART (1 s - 60 min)"; return; }
    if (!s.feed.init(s.market, tick, spb)) { s.fault = true; s.state = "UNKNOWN MARKET " + root; return; }
    s.state = "LOADING";
}

// buy / sell = ask-traded / bid-traded volume of the bar (volume-at-price statistics). False = not read (retried, never zero).
bool CVD::readFlow(RTARRAYP& vap, int bar, double& buy, double& sell) {
    BARSTATISTICS st; std::memset(&st, 0, sizeof(st));
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (vap.getBarStatistics(bar, st) != RTX_OK) return false;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
    buy = (double)(long long)st.buyVolume; sell = (double)(long long)st.sellVolume;
    return buy >= 0 && sell >= 0;
}

// MenthorQ CR / PS / HVL for this market; they belong to the newest session on the chart (read only when the file changes)
void CVD::loadMq(RuntimeState& s) {
    const std::string f = flexDir(); if (f.empty()) return;
    const std::string p = f + "\\MenthorQLevels.csv"; long long st = -1;
    if (!statFile(p, &st)) { if (s.mqStamp != -1) { s.mqStamp = -1; s.feed.mq.clear(); } return; }
    if (st == s.mqStamp && s.feed.mqSess == s.newestSess) return;
    s.mqStamp = st; s.feed.mq.clear(); s.feed.mqSess = s.newestSess;
    std::ifstream in(p.c_str()); std::string line; int n = 0;
    while (std::getline(in, line) && ++n < 2000) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> c = split(line, ',');
        if (c.size() < 3 || mqMarket(trim(c[0])) != s.market) continue;
        double px = 0; if (!parseD(c[1], px) || px <= 0) continue;
        const std::vector<std::string> parts = split(c[2], '/');
        for (size_t k = 0; k < parts.size(); ++k) if (gamma1(parts[k])) {
            std::string code = trim(parts[k]); if (code.size() > 12) code.resize(12);
            for (size_t j = 0; j < code.size(); ++j) if (code[j] == ',' || code[j] == '|' || code[j] == '@' || code[j] == '"') code[j] = '_';
            s.feed.mq.push_back(cvdf::Lvl{code, px}); break;
        }
    }
}

// the nightly per-market bar range (lra.cvd_nightly): rows "MKT,YYYY-MM-DD,ticks" - the session date a row first applies to.
// Only future sessions get a new row, so a reload never changes a session already drawn. Read only when the file changes.
void CVD::loadRanges(RuntimeState& s) {
    const std::string f = flexDir(); if (f.empty()) return;
    const std::string p = f + "\\LRA-CVD-ranges.csv"; long long st = -1;
    if (!statFile(p, &st)) { if (s.rangeStamp != -1) { s.rangeStamp = -1; s.feed.rangeRows.clear(); } return; }
    if (st == s.rangeStamp) return;
    s.rangeStamp = st; s.feed.rangeRows.clear();
    std::ifstream in(p.c_str()); std::string line; int n = 0;
    while (std::getline(in, line) && ++n < 5000) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> c = split(line, ',');
        if (c.size() != 3 || trim(c[0]) != s.market) continue;
        int y = 0, m = 0, d = 0; char extra = 0; double ticks = 0;
        if (std::sscanf(trim(c[1]).c_str(), "%d-%d-%d%c", &y, &m, &d, &extra) != 3 || m < 1 || m > 12 || d < 1 || d > 31) continue;
        if (!parseD(c[2], ticks) || ticks < 1 || ticks > 5000) continue;
        s.feed.rangeRows.push_back(std::make_pair(svl::daysFromCivil(y, m, d), ticks));
    }
    std::sort(s.feed.rangeRows.begin(), s.feed.rangeRows.end());
}

// the nightly verdict on the shown reads (lra.cvd_nightly): "VERDICT,testing|proven|retired". Anything else = testing.
void CVD::loadVerdict(RuntimeState& s) {
    const std::string f = flexDir(); if (f.empty()) return;
    const std::string p = f + "\\LRA-CVD-verdict.csv"; long long st = -1;
    if (!statFile(p, &st)) { s.verdictStamp = -1; s.verdict = "testing"; return; }
    if (st == s.verdictStamp) return;
    s.verdictStamp = st; s.verdict = "testing";
    std::ifstream in(p.c_str()); std::string line;
    if (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> c = split(line, ',');
        if (c.size() == 2 && trim(c[0]) == "VERDICT" && (trim(c[1]) == "proven" || trim(c[1]) == "retired")) s.verdict = trim(c[1]);
    }
}

void CVD::calcBars(RuntimeState& s, long count) {
    if (count < 3) return;
    RTARRAYI times(barDateTime), vol(barVolume); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYP vap(barVolumeProfile);
    if (times.count < count || vol.count < count || op.count < count || hi.count < count || lo.count < count || cl.count < count || vap.count < count) return;
    const long lastClosed = count - 2;                        // the last array bar is forming: never read
    { long d = 0; int e = 0; if (civil((RTDATE)times[(int)lastClosed], d, e)) s.newestSess = svl::classify(d, e, s.spb, s.feed.openMin, s.feed.closeMin).sess; }
    loadMq(s);
    loadRanges(s);
    loadVerdict(s);
    if (s.nextIndex < 0) {                                    // start ~8 days back: 5 prior sessions for the range + the prior session's levels
        const long long newest = (long long)times[(int)lastClosed];
        long first = lastClosed;
        while (first > 0 && newest - (long long)times[(int)(first - 1)] <= 8LL * 86400LL) --first;
        s.nextIndex = first;
    }
    // IRT may trim old bars or reload history: find the frontier again by bar time; never feed a bar twice
    if (s.lastRaw && (s.nextIndex <= 0 || s.nextIndex - 1 > lastClosed || (long long)times[(int)(s.nextIndex - 1)] != s.lastRaw)) {
        long found = -1;
        for (long j = lastClosed; j >= 0; --j) { const long long t = (long long)times[(int)j]; if (t == s.lastRaw) { found = j; break; } if (t < s.lastRaw) break; }
        if (found < 0) { s.identified = false; s.state = "RELOADING HISTORY"; return; }
        s.nextIndex = found + 1;
    }
    long done = 0;
    for (long i = s.nextIndex; i <= lastClosed && done < PASS_BARS; ++i, ++done) {
        const RTDATE raw = (RTDATE)times[(int)i];
        if (s.lastRaw && (long long)raw <= s.lastRaw) { s.identified = false; s.state = "RELOADING HISTORY"; return; }
        cvdf::In x; x.raw = (long long)raw;
        if (!civil(raw, x.days, x.endSec)) { s.fault = true; s.state = "BAD BAR TIME - reload the chart"; return; }
        x.o = op[(int)i]; x.h = hi[(int)i]; x.l = lo[(int)i]; x.c = cl[(int)i]; x.total = (double)(long long)vol[(int)i];
        double buy = NAN, sell = NAN;
        if (!readFlow(vap, (int)i, buy, sell)) {
            // the newest bars: volume-at-price may not be loaded yet - wait up to 10 s, then count the bar as missing
            const long long now = steadyMs();
            if (lastClosed - i < 3) {
                if (s.failRaw != (long long)raw) { s.failRaw = (long long)raw; s.failSinceMs = now; }
                if (now - s.failSinceMs < 10000) { s.state = "WAITING FOR ORDER-FLOW DATA"; break; }
            }
        } else { x.buy = buy; x.sell = sell; }
        const std::vector<cvd::Event> ev = s.feed.push(x);
        for (size_t k = 0; k < ev.size(); ++k) s.journalRetry.push_back(s.feed.marks[s.feed.marks.size() - ev.size() + k]);
        s.lastRaw = (long long)raw; s.nextIndex = i + 1; s.failRaw = 0;
        s.state = s.feed.state;
    }
    journal(s);
    while (s.feed.out.size() > 20000) s.feed.out.erase(s.feed.out.begin());
    if (s.feed.marks.size() > 5000) s.feed.marks.erase(s.feed.marks.begin(), s.feed.marks.begin() + (s.feed.marks.size() - 5000));
}

// the study log: one line per event, once (key = event id + state + bar time), only marked written after a good flush (#6)
void CVD::journal(RuntimeState& s) {
    if (s.journalRetry.empty()) return;
    const std::string f = flexDir(); if (f.empty()) { s.journalRetry.clear(); return; }
    const std::string dir = f + "\\CVD"; makeDir(f); makeDir(dir);
    const char* header = "schema,version,market,contract,session,bar_time_ct,event_id,type,direction,state,label,reason,warning_seq,source_seq,level_id,level,trigger,invalidation,cvd,range_ticks,risk,shown";
    std::lock_guard<std::mutex> g(g_fileMutex);
    std::vector<cvdf::Mark> keep;
    for (size_t i = 0; i < s.journalRetry.size(); ++i) {
        const cvd::Event& e = s.journalRetry[i].e;
        const std::string path = dir + "\\LRA-CVD2-events-" + s.market + "-" + std::to_string(s.spb) + "-" + safeFilePart(s.contract) + "-" + e.anchor + ".csv";
        if (!s.journalLoaded.count(path)) {
            s.journalLoaded.insert(path);
            std::ifstream prior(path.c_str()); std::string row; std::getline(prior, row);
            while (std::getline(prior, row)) {
                if (prior.eof()) break;                                   // a last line without its newline was never committed
                const std::vector<std::string> c = split(row, ',');
                if (c.size() == 22) s.journalKnown.insert(c[6] + "|" + c[9] + "|" + c[5]);
            }
        }
        const std::string& barTime = e.timestamp;
        const std::string k2 = e.id + "|" + cvd::name(e.state) + "|" + barTime;
        if (s.journalKnown.count(k2)) continue;
        long long st = -1; const bool fresh = !statFile(path, &st);
        std::ofstream out(path.c_str(), std::ios::binary | std::ios::app);
        if (!out) { ++s.journalFailures; if (keep.size() < 2000) keep.push_back(s.journalRetry[i]); continue; }
        if (fresh) out << header << '\n';
        std::ostringstream row; row.imbue(std::locale::classic()); row << std::setprecision(12);
        row << "CVD2," << CVD_VERSION << ',' << s.market << ',' << s.contract << ',' << e.anchor << ',' << barTime << ',' << e.id << ','
            << cvd::name(e.type) << ',' << (e.direction == cvd::Direction::Bull ? "bull" : "bear") << ',' << cvd::name(e.state) << ','
            << e.label << ',' << e.reason << ',' << e.warning_seq << ',' << e.source_seq << ',' << e.level_id << ',' << e.level << ','
            << e.trigger << ',' << e.invalidation << ',' << e.cvd << ',' << s.feed.rangeTicks << ',' << s.journalRetry[i].risk << ',' << (s.journalRetry[i].shown ? 1 : 0);
        out << row.str() << '\n'; out.flush();
        if (out) s.journalKnown.insert(k2); else { ++s.journalFailures; if (keep.size() < 2000) keep.push_back(s.journalRetry[i]); }
    }
    s.journalRetry.swap(keep);
}

// status for the IRT health check: on change, at most every 30 s (BUILD-LESSONS #31); tmp + rename
void CVD::writeStatus(RuntimeState& s) {
    const long long now = steadyMs();
    if (s.lastStatusMs >= 0 && now - s.lastStatusMs < 30000) return;
    const std::string f = flexDir(); if (f.empty() || s.market.empty()) return;
    s.lastStatusMs = now;
    double cvdNow = NAN, pctNow = NAN;
    for (std::map<long long, cvdf::Out>::const_reverse_iterator it = s.feed.out.rbegin(); it != s.feed.out.rend(); ++it) if (it->second.counted) { cvdNow = it->second.cvd; pctNow = it->second.pct; break; }
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << CVD_VERSION << "\nMARKET," << s.market << "\nSYMBOL," << s.contract << "\nSPB," << s.spb << "\nSTATE," << s.state
      << "\nRANGE_TICKS," << s.feed.rangeTicks << "\nCVD," << cvdNow << "\nDELTA_PCT," << pctNow << "\nLEVELS," << s.feed.levels().size() / 2
      << "\nMQ_LEVELS," << s.feed.mq.size() << "\nMARKS," << s.feed.marks.size() << "\nSHOWN," << s.feed.shownCount << "\nVERDICT," << s.verdict << "\nBAD_BARS," << s.feed.badBars << "\nOVER_TOTAL," << s.feed.overTotal
      << "\nJOURNAL_FAILURES," << s.journalFailures << "\n";
    const std::string text = o.str(); if (text == s.lastStatusText) return;
    const std::string path = f + "\\CVD.status-" + s.market + "-" + std::to_string(s.spb) + ".txt", tmp = path + ".tmp";
    { std::ofstream out(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!out) return; out << text; if (!out) return; }
#if defined(_WIN32)
    if (!MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) return;
#else
    if (std::rename(tmp.c_str(), path.c_str()) != 0) return;
#endif
    s.lastStatusText = text;
}

// (1.3.0) the market's settings file (keyed); a missing / bad file = the defaults (what 1.2.0 drew)
void CVD::loadSettings(RuntimeState& s) {
    const std::string p = cvdSettingsPath(s.market);
    if (p.empty()) { s.set = ss::defaults(cvdset::spec(), -1); return; }
    if (s.setStore.path != p) { s.setStore = ss::Store(); s.setStore.S = &cvdset::spec(); s.setStore.path = p; }
    s.set = s.setStore.load(ss::marketIndex(s.market), steadyMs());
}

void CVD::render(const RuntimeState& s) {
    using namespace cvdset;
    const ss::Values& V = s.set;
    RCT pane; pane.getPaneRect(false);
    const long count = getBarCount();
    if (pane.bottom - pane.top < 60 || pane.right - pane.left < 200) return;   // pane too short: draw nothing rather than overlap (#35)
    FONT font; font.id = HELVETICA; font.size = 9; font.style = BOLD; setFont(font);
    const auto text = [&](long x, long y, const std::string& v, COLOR c) {
        setTextColor(c); const int w = getTextWidth(v.c_str(), -1); RCT r; r.set(coord(x), coord(y), coord(x + w + 2), coord(y + 12)); r.drawText(v.c_str(), false, false); return x + w;
    };
    // header: "Metric: now" readouts, no explanations on the chart
    double cvdNow = NAN, pctNow = NAN;
    for (std::map<long long, cvdf::Out>::const_reverse_iterator it = s.feed.out.rbegin(); it != s.feed.out.rend(); ++it) if (it->second.counted) { cvdNow = it->second.cvd; pctNow = it->second.pct; break; }
    {
        long x = text(pane.left + 4, pane.top + 1, "CVD: ", C_GREY); x = text(x, pane.top + 1, withSign(cvdNow) + "   ", !std::isfinite(cvdNow) ? C_GREY : cvdNow >= 0 ? C_BULL : C_BEAR);
        x = text(x, pane.top + 1, "Delta: ", C_GREY); text(x, pane.top + 1, withSign(pctNow) + "%", !std::isfinite(pctNow) ? C_GREY : pctNow >= 0 ? C_BULL : C_BEAR);
        const std::string tag = s.verdict == "proven" ? "" : s.verdict == "retired" ? "marks off - stopped working" : "testing";
        if (!tag.empty()) text(pane.right - getTextWidth(tag.c_str(), -1) - 8, pane.top + 1, tag, C_GREY);
    }
    if (s.state.compare(0, 5, "READY") != 0) text(pane.left + 4, pane.top + 14, s.state, C_GREY);   // only when something is not right
    if (count < 2) return;
    RTARRAYI times(barDateTime); if (times.count < count) return;
    const auto xOf = [](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (long)p.h; };
    std::vector<int> vis;
    for (int i = (int)count - 1; i >= 0; --i) { const long x = xOf(i); if (x < pane.left) break; if (x <= pane.right) vis.push_back(i); }
    std::reverse(vis.begin(), vis.end());
    if (vis.empty()) return;
    const auto outOf = [&](int bar) -> const cvdf::Out* { std::map<long long, cvdf::Out>::const_iterator it = s.feed.out.find((long long)times[bar]); return it == s.feed.out.end() || !it->second.counted ? nullptr : &it->second; };
    double lo = 1e300, hi = -1e300, pmax = 25;
    for (size_t k = 0; k < vis.size(); ++k) {
        const cvdf::Out* o = outOf(vis[k]); if (!o) continue;
        if (std::isfinite(o->cvd)) { lo = std::min(lo, o->cvd); hi = std::max(hi, o->cvd); }
        if (std::isfinite(o->pct)) pmax = std::max(pmax, std::fabs(o->pct));
    }
    const long top = pane.top + 30, bottom = pane.bottom - 30;               // bands above / below for the labels
    if (bottom - top < 30) return;
    if (!(hi >= lo)) { lo = -1; hi = 1; }
    if (hi - lo < 1) { hi += 0.5; lo -= 0.5; }
    const auto yC = [&](double v) { return bottom - (long)((bottom - top) * (v - lo) / (hi - lo)); };
    const long mid = (top + bottom) / 2, half = (bottom - top) / 2;
    const auto yP = [&](double p) { return mid - (long)(half * std::max(-1.0, std::min(1.0, p / pmax))); };
    // delta % histogram (right scale, its own zero in the middle) - muted, behind the line
    long spacing = vis.size() > 1 ? std::max(1L, xOf(vis[1]) - xOf(vis[0])) : 6;
    const long bw = std::max(1L, spacing * 6 / 10);
    const COLOR barUp = (COLOR)V.c(K_BAR_UP), barDn = (COLOR)V.c(K_BAR_DN);
    for (size_t k = 0; k < vis.size() && V.b(K_SHOW_BARS); ++k) {
        const cvdf::Out* o = outOf(vis[k]); if (!o || !std::isfinite(o->pct)) continue;
        const long x = xOf(vis[k]), y = yP(o->pct); RCT r;
        r.set(coord(x - bw / 2), coord(std::min(y, mid)), coord(x - bw / 2 + bw), coord(std::max(y, mid) + 1));
        r.draw(0, o->pct >= 0 ? barUp : barDn, o->pct >= 0 ? barUp : barDn, DRAW_OPAQUE, PAT_SOLID);
    }
    setPen(C_AXIS, 1, P_SOLID);
    { PNT a; a.set(0, 0.0f); a.h = coord(pane.left); a.v = coord(mid); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(pane.right); b.v = coord(mid); b.drawLineTo(); }
    text(pane.right - 40, top - 13, "+" + std::to_string((int)std::lround(pmax)) + "%", C_GREY);
    text(pane.right - 40, bottom + 1, "-" + std::to_string((int)std::lround(pmax)) + "%", C_GREY);
    text(pane.left + 4, top - 13, withSign(hi), C_GREY); text(pane.left + 4, bottom + 1, withSign(lo), C_GREY);
    // the CVD line (gaps where a session has no CVD; a new session starts a new line)
    setPen((COLOR)V.c(K_LINE_COLOR), 2, P_SOLID); bool started = false;
    for (size_t k = 0; k < vis.size() && V.b(K_SHOW_LINE); ++k) {
        const cvdf::Out* o = outOf(vis[k]);
        if (!o || !std::isfinite(o->cvd)) { started = false; continue; }
        PNT p; p.set(0, 0.0f); p.h = coord(xOf(vis[k])); p.v = coord(yC(o->cvd));
        if (!started) { p.setDrawPosition(); started = true; } else p.drawLineTo();
    }
    // marks: at the bar that printed them, on that bar's CVD value; bullish below, bearish above; 3 lanes each side, never overlapping
    std::map<long long, int> barOf; for (size_t k = 0; k < vis.size(); ++k) barOf[(long long)times[vis[k]]] = vis[k];
    // every placed label box; a new label tries 4 heights on its own side and is skipped (its dot stays) if all would touch one
    struct Box { long l, t, r, b; }; std::vector<Box> placed;
    for (size_t m = 0; m < s.feed.marks.size() && V.b(K_SHOW_MARKS); ++m) {
        const cvdf::Mark& mk = s.feed.marks[m];
        if (!mk.shown || s.verdict == "retired") continue;        // 1.1.0: only the confirmed reads that held up; none once retired
        std::map<long long, int>::const_iterator b = barOf.find(mk.raw); if (b == barOf.end()) continue;
        const cvdf::Out* o = outOf(b->second); if (!o || !std::isfinite(o->cvd)) continue;
        const bool bull = mk.e.direction == cvd::Direction::Bull;
        const COLOR col = (COLOR)(bull ? V.c(K_MARK_BULL) : V.c(K_MARK_BEAR));
        std::string label = cvd::name(mk.e.type);
        if (mk.e.type != cvd::Type::Abs) label += "?";   // (1.2.0, audit list 18:25 CT: small test sample, +0.26R on 99) "Div?" / "Exh?" / "BR?"
        const long x = xOf(b->second), y = yC(o->cvd), w = getTextWidth(label.c_str(), -1) + 8, x0 = x - w / 2;
        RCT dot; dot.set(coord(x - 3), coord(y - 3), coord(x + 4), coord(y + 4)); dot.draw(1, col, col, DRAW_OPAQUE, PAT_SOLID);
        long boxTop = LONG_MIN;
        for (int lane = 0; lane < 4 && boxTop == LONG_MIN; ++lane) {
            const long t = bull ? y + 10 + lane * 16 : y - 24 - lane * 16;
            if (t < pane.top + 14 || t + 14 > pane.bottom - 1) continue;          // stays inside the pane, under the header
            bool clear = true;
            for (size_t k = 0; k < placed.size() && clear; ++k)
                if (x0 - 2 < placed[k].r && placed[k].l < x0 + w + 2 && t - 1 < placed[k].b && placed[k].t < t + 15) clear = false;
            if (clear) boxTop = t;
        }
        if (boxTop == LONG_MIN) continue;
        placed.push_back(Box{ x0, boxTop, x0 + w, boxTop + 14 });
        setPen(col, 1, P_SOLID);
        { PNT a; a.set(0, 0.0f); a.h = coord(x); a.v = coord(bull ? y + 4 : y - 4); a.setDrawPosition(); PNT c; c.set(0, 0.0f); c.h = coord(x); c.v = coord(bull ? boxTop : boxTop + 14); c.drawLineTo(); }
        RCT box; box.set(coord(x0), coord(boxTop), coord(x0 + w), coord(boxTop + 14));
        box.draw(1, col, col, DRAW_OPAQUE, PAT_SOLID);
        text(x0 + 4, boxTop + 1, label, C_DARK);
    }
}

void CVD::pump() {
    RuntimeState* s = slot_.get(this, true);
    if (!s) return;
    std::unique_lock<std::recursive_mutex> lock(s->mutex, std::try_to_lock);
    if (!lock.owns_lock()) return;
    try {
        identify(*s);
        if (!s->identified) identify(*s);
        loadSettings(*s);
        if (!s->fault) calcBars(*s, getBarCount());
        writeStatus(*s);
    } catch (...) { s->fault = true; s->state = "STOPPED - error, reload the chart"; }
}

extern "C" cppExtension* CreateExtension(void) {
    CVD* p = new CVD();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED);
    p->setDescription("LRA CVD (testing): blue line = cumulative delta (ask-traded minus bid-traded volume) since the 5:00 PM session start; bars = each bar's delta as a % of its volume. Marks sit on the CVD line where a reversal or breakout was confirmed by price and flow at a prior-day or prior-session high / low (PDH PDL PFH PFL): Div = price made a new high / low but CVD did not; Exh = the pushing side faded at the new extreme; BR = broke the level, retested it and held. Green below the line = bullish, red above = bearish. Only setups with a stop within 4x the normal bar range are shown. Closed bars only; nothing drawn ever changes.");
    p->setVersion("1.3.1");   // keep equal to CVD_VERSION (the build log reads this literal)
    return p;
}
