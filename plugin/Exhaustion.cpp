/********************************************************************************
 *  0.4.0 (2026-10-29, Rassul 07:08 CT approved "implement and then show mockup", his sketch: the attacker's pace rising then rolling over
 *        a slower yellow signal line, MACD-style): everything 0.3.2 draws stays (now in the top 55% of the pane); a PACE band below
 *        (ExhaustionPace.h): the side attacking the current 20-bar push (low before high = buyers, else sellers) - its pace (aggressive
 *        volume, S-bar average) and SIGNAL = EMA(pace, 9). Marks for pushes >= 2.0 x ATR(14 bar ranges), decided on the close:
 *        SLOWED (pace > signal, pace below the bar before after rising) = grey dot; TURNED (pace crosses under signal) = triangle + "T".
 *        Recorded in lsFlexLevels\Exhaustion\<MKT>-<CONTRACT>-<spb>-pace-<session>.csv (time, side, pace, signal, push, ATR, bars from the
 *        push extreme). Header + "Pace (buyers|sellers): rising / slowed / turned". Status PACE line.
 *  0.3.2 (2026-10-29, Rassul 06:04 CT "because its following the pivot rule it is not showing a lot of exhaustion signals - take off the
 *        pivot rule"): the lines show the 0.2.4 measure again on every closed bar (ex::compute: at a new N-bar high, buyers' peak S-bar
 *        pace since the lowest low of the last 20 bars / their pace on this bar; sellers the mirror; 1.0 otherwise), drawn as the 0.3.1
 *        lines: 0 on every bar, rising to the ratio on a bar at / over 2.0x with the value at the peak. Header = the last closed bar.
 *        The pivots record file is still written (cheap, for the nightly) but pivots are no longer drawn. E records unchanged, no E?.
 *  0.3.1 (2026-10-29, Rassul 05:39 CT "change histogram back to line"): drawing only - two connected lines as 0.2.4 (red = buyers fading,
 *        green = sellers fading), each at 0 on every bar and rising to the ratio on a pivot bar at / over 2.0x, value at the peak; no
 *        boxes. Pivot rule, 2.0x show rule, records, header and colours unchanged from 0.3.0.
 *  0.3.0 (2026-10-29, Rassul 05:11 CT "update the exhaustion indicator so it follows the pivot rule and only bars above 2x are shown and
 *        draw it like a histogram instead of connected lines"): the pane is now a HISTOGRAM AT PIVOTS. Pivots = IRT's ZIGZAG, Min Price
 *        Change 3 bars, Use High/Low (the lsWavePOC 1.3.2 rule, copied into ExhaustionPivots.h). At each confirmed pivot HIGH: buyers
 *        fading = buyers' peak pace in the up leg into it / their pace on the pivot bar (red bar, bearish); at each pivot LOW: sellers
 *        fading, the mirror (green bar, bullish). One bar on the pivot bar from 0 up to the ratio with "2.9x" on top, drawn only at
 *        >= 2.0x, once the pivot is confirmed (the reversal bar closed) - never repainted. Every pivot (also < 2x) is recorded in
 *        lsFlexLevels\Exhaustion\<MKT>-<CONTRACT>-<spb>-pivots-<session>.csv (pivot time, side, ratio, high, low, conf / leg start time,
 *        shown|logged). No connecting lines, no E? on the chart (the E records are still written exactly as 0.2.4). No settings.
 *  0.2.3 (2026-10-14, Rassul approved the external-audit display list at 18:25 CT, section 13): the mark reads "E?" (was "E") - untested
 *        as a standalone signal; recorded and drawn exactly as before otherwise.
 *  Exhaustion.cpp  --  Investor/RT RTX extension  lsExhaustion  (0.2.2, 2026-10-13: engineering fixes ported from the 0.3.1 audit - a failed VAP read rejects the snapshot instead of reading as zero volume, prices on the chart tick grid, RTH edge in exact seconds, records published atomically per contract (marks shown only once recorded), status file replaced atomically, bounded norms / record parsing; signal, thresholds and norms unchanged. 0.2.1, 2026-10-12: the reversal read kept as Rassul uses it - "it is showing good reversals" in crude - with the bar in progress no longer read; the pullback read is the separate lsExhaustionV2. 0.2.0, 2026-10-11; 0.2.0: overnight E only at the study's own overnight line T_overnight when both halves agree, else none - Rassul 22:05 "customize the thresholds for time of day .. overnight vs rth")
 *
 *  EXHAUSTION: is the side pushing price running out? Rassul 2026-10-11 21:17-21:28 (option A, "build exhaustion indicator").
 *  Its own pane, two lines coloured by what they mean for price:
 *    red   = BUYERS FADING at a new high (bearish): the buyers' peak pace since the swing low / their pace now
 *    green = SELLERS FADING at a new low (bullish): the mirror
 *  1.0 when price is not at a new N-bar extreme (quiet in chop). The line (T) is drawn; a value at or over it is written over its dot; E
 *  (red at tops, green at bottoms) marks the first closed RTH bar that reaches it - drawn once, recorded, never repainted.
 *  Header: "EXHAUSTION  Buyers fading: 1.0x (3.0x)  Sellers fading: 1.0x (3.0x)" for the last CLOSED bar (0.2.1).
 *  Per market N / S / T (new-extreme bars, pace bars, line) from lra.exh_study (ExhaustionLogic.h defaults; LRA-ExhNorms-<MKT>.csv
 *  PARAM,N / S / T when the nightly study writes it). Native: the chart's own bars and volume at price (buyers = volume at the ask).
 *  Record: lsFlexLevels\Exhaustion\<MKT>-<CONTRACT>-<sec/bar>-<session>.csv (0.2.2: contract-qualified) (each E: bar time, side, fade, high / low). No settings.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "Exhaustion"
#endif
#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG: the settings window never faults, never opens blank
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "ExhaustionLogic.h"
#include "ExhaustionPace.h"   // (0.4.0) attacker pace vs its 9-bar signal
#include "ExhaustionPivots.h"   // (0.3.0) IRT zigzag pivots + fade per pivot
#include "HostSlot.h"
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
#include <map>
#include <set>
#include <new>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#define EX_VERSION "0.4.0"          // keep equal to ex::VERSION and the setVersion literal below

static const COLOR C_SIG = 0x00FACC15, C_SIG_DIM = 0x00A16207, C_DOT = 0x009CA3AF;   // (0.4.0) signal yellow (sellers dimmer), slowed dot grey
static const COLOR C_BUY = 0x0022C55E, C_SELL = 0x00EF4444, C_GREY = 0x009CA3AF, C_AXIS = 0x00334155;
static const int BARS_MAX = 1500;
static inline bool finiteD(double v) { return v == v && v < 1e300 && v > -1e300; }
namespace pl { inline bool finite(double v) { return finiteD(v); } }
// (0.2.2) IRT prices arrive as float while a chart defines its own native tick.  Round once to that grid rather
// than comparing nearby binary float representations.
static bool tickPrice(double price, double tick, double& out) {
    if (!finiteD(price) || !finiteD(tick) || tick <= 0) return false;
    const double q = price / tick;
    if (!finiteD(q) || q > (double)LLONG_MAX - 1 || q < (double)LLONG_MIN + 1) return false;
    const double n = q >= 0 ? std::floor(q + .5) : std::ceil(q - .5);
    out = n * tick; return finiteD(out);
}
static long long steadyMs() {
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
static std::string flexDir() {
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels";
}
static void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
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
static bool moveOver(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
static bool publishFile(const std::string& path, const std::string& contents) {
    const std::string tmp = path + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false;
      f.write(contents.data(), (std::streamsize)contents.size()); f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    // (0.2.2) rename/MoveFileEx(REPLACE_EXISTING) publishes the complete temporary file in one replacement;
    // moving the old file away first left an observable no-file interval for readers.
    if (!moveOver(tmp, path)) { std::remove(tmp.c_str()); return false; }
    return true;
}
static std::vector<std::string> split(const std::string& s, char c) {
    std::vector<std::string> f; size_t a = 0;
    for (size_t i = 0; i <= s.size(); ++i) if (i == s.size() || s[i] == c) { f.push_back(s.substr(a, i - a)); a = i + 1; }
    return f;
}
static bool parseD(const std::string& t, double& out) {
    if (t.empty() || t.size() > 40) return false;
    errno = 0; char* e = nullptr; const double v = std::strtod(t.c_str(), &e);
    if (errno || e != t.c_str() + t.size() || !pl::finite(v)) return false;
    out = v; return true;
}
static bool parseLL(const std::string& t, long long& out) {
    if (t.empty() || t.size() > 24) return false;
    errno = 0; char* e = nullptr; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno || e != t.c_str() + t.size()) return false;
    out = v; return true;
}
// (0.2.2) records: whole-file atomic publication (replaces the append, which could tear), bounded parsing
static const char* const RECORD_HEADER = "bar_time|side|fade|high|low|params";
static const size_t RECORD_FILE_MAX = 512 * 1024, RECORD_LINE_MAX = 512, RECORD_LINES_MAX = 5000;
struct DiskRecord { long long bar; int side; double val; std::string line; };
struct PendingRecord { long long session, bar; int side; double val; std::string line; int kind = 0; };   // kind 0 = E, 1 = pivot (0.3.0)

static bool parseRecord(const std::string& line, DiskRecord& out) {
    if (line.empty() || line.size() > RECORD_LINE_MAX) return false;
    const std::vector<std::string> c = split(line, '|'); long long bt = 0; double v = 0, hi = 0, lo = 0;
    if (c.size() != 6 || !parseLL(c[0], bt) || bt <= 0 || (c[1] != "top" && c[1] != "bottom") || !parseD(c[2], v)
        || !parseD(c[3], hi) || !parseD(c[4], lo)) return false;
    out.bar = bt; out.side = c[1] == "top" ? -1 : 1; out.val = v; out.line = line; return true;
}
// A malformed/torn data line is ignored, but a bad header, I/O error, or excessive file is never rewritten.
static bool readRecords(const std::string& path, std::vector<DiskRecord>& out) {
    out.clear(); if (!statFile(path)) return true;
    std::ifstream in(path.c_str(), std::ios::binary); if (!in) return false;
    in.seekg(0, std::ios::end); const std::streamoff size = in.tellg();
    if (size < 0 || size > (std::streamoff)RECORD_FILE_MAX) return false;
    in.seekg(0, std::ios::beg); std::string line;
    if (!std::getline(in, line)) return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != RECORD_HEADER) return false;
    size_t n = 0;
    while (std::getline(in, line)) {
        if (++n > RECORD_LINES_MAX) return false;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        DiskRecord r; if (parseRecord(line, r)) out.push_back(r);
    }
    return !in.bad();
}
static bool publishRecords(const std::string& path, const std::vector<const PendingRecord*>& add) {
    std::vector<DiskRecord> old; if (!readRecords(path, old)) return false;
    std::set<std::pair<long long, int> > seen; std::ostringstream text; text.imbue(std::locale::classic()); text << RECORD_HEADER << "\n";
    for (size_t i = 0; i < old.size(); ++i) if (seen.insert(std::make_pair(old[i].bar, old[i].side)).second) text << old[i].line << "\n";
    for (size_t i = 0; i < add.size(); ++i) if (seen.insert(std::make_pair(add[i]->bar, add[i]->side)).second) text << add[i]->line << "\n";
    if (text.tellp() < 0 || text.tellp() > (std::streampos)RECORD_FILE_MAX) return false;
    return publishFile(path, text.str());
}
static bool wholeNumber(double v) { return std::floor(v) == v; }
static std::string fileKey(const std::string& s) {
    std::string out; out.reserve(std::min((size_t)24, s.size()));
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) {
        const unsigned char c = (unsigned char)s[i];
        out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_');
    }
    return out.empty() ? "UNKNOWN" : out;
}
static std::string marketName(std::string r) {
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return r.substr(0, 8);
}
static long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}


// RTH (Central minutes) per market - E prints in RTH only, as the study measured it
static bool rthOf(const std::string& m, int& o, int& c) {
    if (m == "ES" || m == "NQ") { o = 510; c = 900; return true; }
    if (m == "CL" || m == "NG") { o = 480; c = 810; return true; }
    if (m == "GC") { o = 440; c = 750; return true; }
    if (m == "HG") { o = 430; c = 750; return true; }
    if (m == "EU") { o = 440; c = 840; return true; }
    return false;
}

struct EXState {
    ex::Params P; std::string params = "study 2026-10-11", proven = "testing";
    double tOvernight = 0;                                  // (0.2.0) the overnight line (0 = no E overnight) - only when the study agrees on one   // proven: the nightly STATUS line (confirmed / testing)
    bool busy = false, faulted = false;
    std::string identity, market, symbol, state = "starting", lastStatusText;
    int spb = 0; double tick = 0; int rthO = -1, rthC = -1;
    long count = 0; long long session = -1, lastNormsMs = -1, lastStatusMs = -1, lastRecordAttemptMs = -1;
    std::map<long long, std::pair<double, double>> vol;     // bar time -> (buyers, sellers) of CLOSED bars (read once)
    std::map<long long, int> marks;                         // bar time -> +1 bottom E / -1 top E (recorded, never repainted)
    std::map<long long, double> markVal;
    std::vector<PendingRecord> pending; long long recFails = 0;
    ex::Out out; int outFirst = 0; double liveB = 1, liveS = 1;
    // (0.3.0) confirmed pivots: bar time -> (+1 high = buyers fading / -1 low = sellers fading, ratio); added once, never changed
    std::map<long long, std::pair<int, double>> piv; long long pivSeen = 0; long pivLogged = 0;
    // (0.4.0) pace / signal of the closed bars (same window as out) + the stage marks: bar time -> (push dir +1 / -1, SLOWED / TURNED)
    pace::Out po; std::map<long long, std::pair<int, int>> paceMarks; long long paceSeen = 0;
};

class Exhaustion : public cppExtension {
public:
    SS_SAFE_DIALOG(Exhaustion, "Exhaustion")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    Exhaustion() : cppExtension() {}
    virtual int draw(void);
    virtual int parmsLoadBody(void)  { return RTX_OK; }
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    void pump();
    void release() { slot_.release(this); }
    EXState* testState() { return slot_.get(this, false); }
private:
    HostSlot<EXState> slot_;
    long long ctSec(RTDATE d);
    void identify(EXState& S);
    bool barVol(EXState& S, RTARRAYP& vap, int bar, std::pair<double, double>& out);
    void calcBars(EXState& S, long count);
    bool flushRecords(EXState& S);
    void load(EXState& S);
    void norms(EXState& S);
    void status(EXState& S, long long now);
    void render(EXState& S);
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + "\\Exhaustion"; }
    std::string stem(EXState& S) { return S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb); }   // (0.2.2) contract-qualified
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<Exhaustion*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<Exhaustion*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void)   { setParameterVersion(1); return RTX_OK; }
int cppExtension::calc(int)     { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<Exhaustion*>(this)->pump(); return RTX_OK; }
int Exhaustion::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    pump();
    EXState* S = slot_.get(this, false);
    if (S && !S->faulted) { try { render(*S); } catch (...) { S->state = "draw error"; } }
    return RTX_OK;
}
long long Exhaustion::ctSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    return civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
}
void Exhaustion::identify(EXState& S) {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR); if (!finiteD(tick) || tick <= 0 || tick >= 1000) tick = 0;
    const std::string id = root + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity && tick == S.tick) return;
    S.identity = id; S.symbol = sym; S.spb = spb; S.tick = tick; S.market = marketName(root.empty() ? sym : root);
    S.P = ex::defaults(S.market); S.params = "study 2026-10-11"; S.vol.clear(); S.marks.clear(); S.markVal.clear(); S.pending.clear(); S.piv.clear(); S.pivSeen = 0; S.paceMarks.clear(); S.paceSeen = 0;
    S.count = 0; S.session = -1; S.lastNormsMs = -1; S.lastRecordAttemptMs = -1; S.rthO = S.rthC = -1; rthOf(S.market, S.rthO, S.rthC);
}
// the buyers / sellers of one bar from its volume at price (guarded like lsLiquidityProfile: a failed read is "not known")
bool Exhaustion::barVol(EXState& S, RTARRAYP& vap, int bar, std::pair<double, double>& out) {
    (void)S;
    BARSTATISTICS st; std::memset(&st, 0, sizeof(st));
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (vap.getBarStatistics(bar, st) != RTX_OK) return false;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
    if (st.buyVolume < 0 || st.sellVolume < 0) return false;
    out = std::make_pair((double)st.buyVolume, (double)st.sellVolume); return true;
}
void Exhaustion::calcBars(EXState& S, long count) {
    if (count < 3 || S.tick <= 0) return;
    if (count == S.count && !S.out.fb.empty()) { flushRecords(S); return; } // (0.2.2) draw() pumps too: keep the closed-bar snapshot, no VAP re-read
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow); RTARRAYP vap(barVolumeProfile);
    if (times.count < count || hi.count < count || lo.count < count || vap.count < count) return;
    const int first = (int)std::max(0L, count - BARS_MAX);
    std::vector<double> h, l, b, s; h.reserve(count - first); l.reserve(count - first); b.reserve(count - first); s.reserve(count - first);
    // (0.2.1) CLOSED bars only - the bar in progress (count - 1) is never read: its half-built volume showed a huge fade (GC 7.5x) that
    // then changed (Rassul 2026-10-11 22:26). Everything else is exactly 0.2.0.
    for (int i = first; i < count - 1; ++i) {
        const long long bt = (long long)times[i];
        double hh = 0, ll = 0;                                          // (0.2.2) on the chart's tick grid; a bad bar rejects the snapshot
        if (bt <= 0 || !tickPrice((double)hi[i], S.tick, hh) || !tickPrice((double)lo[i], S.tick, ll) || hh < ll) return;
        std::pair<double, double> v(0, 0);
        std::map<long long, std::pair<double, double>>::const_iterator c = S.vol.find(bt);
        if (c != S.vol.end()) v = c->second;
        else if (barVol(S, vap, i, v)) S.vol[bt] = v;
        else return;                                                    // (0.2.2) never turn an unavailable VAP bar into zero-volume exhaustion evidence
        h.push_back(hh); l.push_back(ll); b.push_back(v.first); s.push_back(v.second);
    }
    S.out = ex::compute(h, l, b, s, S.P); S.outFirst = first;
    // (0.4.0) pace / signal / stage marks; a mark is taken only on a bar NEWER than the last processed one (the sliding window's EMA seed
    // can never add or move an old mark), queued once to the pace record, and kept as decided
    S.po = pace::compute(h, l, b, s, S.P.S);
    if (!S.po.pb.empty()) {
        long long newest = S.paceSeen;
        for (int k = 0; k < (int)S.po.pb.size(); ++k) {
            const int bi = first + k; const long long bt = (long long)times[bi];
            if (bt <= S.paceSeen) continue;
            newest = std::max(newest, bt);
            const int m = S.po.mark[(size_t)k]; if (m == pace::NONE || S.paceMarks.count(bt)) continue;
            const int dir = S.po.dir[(size_t)k]; S.paceMarks[bt] = std::make_pair(dir, m);
            const long long cs = ctSec((RTDATE)times[bi]); const long long ses = cs > 0 ? (cs - 17LL * 3600) / 86400LL + 1 : -1;
            if (ses < 0 || S.pending.size() >= RECORD_LINES_MAX) { ++S.recFails; continue; }
            const double pc = dir > 0 ? S.po.pb[(size_t)k] : S.po.ps[(size_t)k], sg = dir > 0 ? S.po.sb[(size_t)k] : S.po.ss[(size_t)k];
            char line[220]; std::snprintf(line, sizeof(line), "%lld|%s|%.2f|%.6g|%.6g|pace %s %s sig %.2f push %.6g atr %.4g dist %d S%d", bt, dir > 0 ? "top" : "bottom", pc,
                (double)hi[bi], (double)lo[bi], m == pace::TURNED ? "turned" : "slowed", dir > 0 ? "buyers" : "sellers", sg, S.po.push[(size_t)k], S.po.atr[(size_t)k], S.po.dist[(size_t)k], S.P.S);
            PendingRecord rec; rec.session = ses; rec.bar = bt; rec.side = dir > 0 ? -1 : 1; rec.val = pc; rec.line = line; rec.kind = 2; S.pending.push_back(rec);
        }
        S.paceSeen = newest;
        while (S.paceMarks.size() > 4000) S.paceMarks.erase(S.paceMarks.begin());
    }
    if (S.out.fb.empty()) return;
    const int f = (int)count - 2 - first;
    S.liveB = S.out.fb[f]; S.liveS = S.out.fs[f];
    // closed bars: an E in RTH is recorded once (new closed bars only after the first pass)
    const long from = S.count == 0 ? first : std::max((long)first, S.count - 1);
    for (long i = from; i < count - 1; ++i) {
        const int k = (int)(i - first);
        const long long bt = (long long)times[(int)i];
        bool queued = false; for (size_t p = 0; p < S.pending.size(); ++p) if (S.pending[p].bar == bt && S.pending[p].kind == 0) { queued = true; break; }
        if (S.marks.count(bt) || queued) continue;
        struct tm t; std::memset(&t, 0, sizeof(t));
        if (!getLocaltime((RTDATE)times[(int)i], &t)) continue;
        const int endSec = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec, startSec = endSec - std::max(1, S.spb);   // (0.2.2) exact seconds
        const bool rth = S.rthO < 0 || (startSec >= S.rthO * 60 && endSec <= S.rthC * 60);   // IRT stamps the bar END
        int side = 0; double val = 0;
        if (rth) {                                                       // RTH: the market's line (eTop / eBot)
            if (S.out.eTop[k]) { side = -1; val = S.out.fb[k]; } else if (S.out.eBot[k]) { side = 1; val = S.out.fs[k]; }
        } else if (S.tOvernight > 0 && k > 0) {                          // (0.2.0) overnight: only at the study's overnight line
            const double T = S.tOvernight;
            if (S.out.newH[k] && S.out.fb[k] >= T && !(S.out.newH[k - 1] && S.out.fb[k - 1] >= T)) { side = -1; val = S.out.fb[k]; }
            else if (S.out.newL[k] && S.out.fs[k] >= T && !(S.out.newL[k - 1] && S.out.fs[k - 1] >= T)) { side = 1; val = S.out.fs[k]; }
        }
        if (!side) continue;
        // (0.2.2) queued once, shown as an E only after its record is published (atomically, in the bar's own session file)
        const long long cs = ctSec((RTDATE)times[(int)i]);
        const long long barSession = cs > 0 ? (cs - 17LL * 3600) / 86400LL + 1 : -1;
        if (barSession < 0 || S.pending.size() >= RECORD_LINES_MAX) { ++S.recFails; continue; }
        char line[160]; std::snprintf(line, sizeof(line), "%lld|%s|%.2f|%.6g|%.6g|N%d S%d T%.2f%s", bt, side < 0 ? "top" : "bottom", val, (double)hi[(int)i], (double)lo[(int)i], S.P.N, S.P.S, rth ? S.P.T : S.tOvernight, rth ? "" : " overnight");
        PendingRecord rec; rec.session = barSession; rec.bar = bt; rec.side = side; rec.val = val; rec.line = line; S.pending.push_back(rec);
    }
    // (0.3.0) pivots: IRT zigzag on the same closed bars; a pivot is taken only when its confirmation (reversal) bar is NEWER than the
    // last bar already processed, so a sliding window can never add / move an older pivot (no repaint); restart = the recorded ones
    {
        const std::vector<exp3::PivotEx> pv = exp3::pivots(h, l, b, s, S.P.S);
        long long newest = S.pivSeen;
        for (size_t q = 0; q < pv.size(); ++q) {
            const int bi = first + pv[q].i, ci = first + pv[q].conf, si = first + pv[q].start;
            const long long pt = (long long)times[bi], ctm = (long long)times[ci];
            newest = std::max(newest, ctm);
            if (ctm <= S.pivSeen || S.piv.count(pt)) continue;
            const double r = std::floor(pv[q].ratio * 100.0 + 0.5) / 100.0;
            S.piv[pt] = std::make_pair(pv[q].type, r); ++S.pivLogged;
            const long long cs = ctSec((RTDATE)times[bi]);
            const long long ses = cs > 0 ? (cs - 17LL * 3600) / 86400LL + 1 : -1;
            if (ses < 0 || S.pending.size() >= RECORD_LINES_MAX) { ++S.recFails; continue; }
            char line[200]; std::snprintf(line, sizeof(line), "%lld|%s|%.2f|%.6g|%.6g|pivot N%d S%d conf %lld leg %lld %s", pt, pv[q].type > 0 ? "top" : "bottom", r,
                (double)hi[bi], (double)lo[bi], exp3::ZZ_N, S.P.S, ctm, (long long)times[si], r + 1e-9 >= exp3::SHOW ? "shown" : "logged");
            PendingRecord rec; rec.session = ses; rec.bar = pt; rec.side = pv[q].type > 0 ? -1 : 1; rec.val = r; rec.line = line; rec.kind = 1; S.pending.push_back(rec);
        }
        S.pivSeen = newest;
        while (S.piv.size() > 4000) S.piv.erase(S.piv.begin());
    }
    S.count = count;
    flushRecords(S);
    while (S.vol.size() > 3000) S.vol.erase(S.vol.begin());
}
bool Exhaustion::flushRecords(EXState& S) {
    if (S.pending.empty()) return true;
    const long long now = steadyMs();
    if (S.lastRecordAttemptMs >= 0 && now >= S.lastRecordAttemptMs && now - S.lastRecordAttemptMs < 1000) return false;
    S.lastRecordAttemptMs = now;
    const std::string d = dir();
    if (d.empty()) { ++S.recFails; return false; }
    makeDir(flexDir()); makeDir(d);
    std::map<long long, std::vector<size_t> > groups;                    // key = session * 2 + kind (0.3.0: E file / pivot file)
    for (size_t i = 0; i < S.pending.size(); ++i) groups[S.pending[i].session * 3 + S.pending[i].kind].push_back(i);
    std::vector<char> committed(S.pending.size(), 0);
    for (std::map<long long, std::vector<size_t> >::const_iterator g = groups.begin(); g != groups.end(); ++g) {
        std::vector<const PendingRecord*> add; add.reserve(g->second.size());
        for (size_t j = 0; j < g->second.size(); ++j) add.push_back(&S.pending[g->second[j]]);
        const long long ses = g->first / 3; const int kind = (int)(g->first % 3); const bool pivot = kind != 0;   // (0.4.0) 0 E, 1 pivots, 2 pace
        if (!publishRecords(d + "\\" + stem(S) + (kind == 1 ? "-pivots-" : kind == 2 ? "-pace-" : "-") + std::to_string(ses) + ".csv", add)) { ++S.recFails; continue; }
        for (size_t j = 0; j < g->second.size(); ++j) {
            const size_t k = g->second[j];
            if (pivot) { committed[k] = 1; continue; } S.marks[S.pending[k].bar] = S.pending[k].side; S.markVal[S.pending[k].bar] = S.pending[k].val; committed[k] = 1;
        }
    }
    std::vector<PendingRecord> retry; retry.reserve(S.pending.size());
    for (size_t i = 0; i < S.pending.size(); ++i) if (!committed[i]) retry.push_back(S.pending[i]);
    S.pending.swap(retry);
    return S.pending.empty();
}
void Exhaustion::load(EXState& S) {
    const std::string d = dir(); if (d.empty()) return;
    for (long long k = S.session - 9; k <= S.session; ++k) {
        std::vector<DiskRecord> records;                                // (0.2.2) bounded, header-checked
        if (!readRecords(d + "\\" + stem(S) + "-" + std::to_string(k) + ".csv", records)) { ++S.recFails; continue; }
        for (size_t i = 0; i < records.size(); ++i) { S.marks[records[i].bar] = records[i].side; S.markVal[records[i].bar] = records[i].val; }
        {   // (0.4.0) the recorded pace marks redraw exactly
            std::vector<DiskRecord> pm;
            if (readRecords(d + "\\" + stem(S) + "-pace-" + std::to_string(k) + ".csv", pm)) {
                for (size_t i = 0; i < pm.size(); ++i) {
                    S.paceMarks[pm[i].bar] = std::make_pair(pm[i].side < 0 ? 1 : -1, pm[i].line.find("|pace turned ") != std::string::npos ? (int)pace::TURNED : (int)pace::SLOWED);
                    S.paceSeen = std::max(S.paceSeen, pm[i].bar);
                }
            } else ++S.recFails;
        }
        std::vector<DiskRecord> pr;                                     // (0.3.0) the recorded pivots redraw exactly (never recomputed)
        if (!readRecords(d + "\\" + stem(S) + "-pivots-" + std::to_string(k) + ".csv", pr)) { ++S.recFails; continue; }
        for (size_t i = 0; i < pr.size(); ++i) {
            S.piv[pr[i].bar] = std::make_pair(pr[i].side < 0 ? 1 : -1, pr[i].val);
            const size_t c = pr[i].line.find(" conf "); long long ct = 0;
            if (c != std::string::npos) { const size_t e = pr[i].line.find(' ', c + 6); if (parseLL(pr[i].line.substr(c + 6, e == std::string::npos ? std::string::npos : e - c - 6), ct)) S.pivSeen = std::max(S.pivSeen, ct); }
        }
    }
}
void Exhaustion::norms(EXState& S) {
    const std::string f = flexDir(); if (f.empty()) return;
    std::ifstream in((f + "\\LRA-ExhNorms-" + S.market + ".csv").c_str(), std::ios::binary); if (!in) return;
    in.seekg(0, std::ios::end); const std::streamoff size = in.tellg();   // (0.2.2) bounded read
    if (size < 0 || size > 64 * 1024) return;
    in.seekg(0, std::ios::beg);
    std::string line, used; int n = 0; ex::Params p = S.P; double on = 0;
    while (std::getline(in, line) && ++n < 200) {
        if (line.size() > 256) return;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> c = split(line, ','); double v = 0;
        if (c.size() >= 2 && c[0] == "STATUS") { S.proven = c[1] == "confirmed" ? "confirmed" : "testing"; continue; }
        if (c.size() < 3 || c[0] != "PARAM" || !parseD(c[2], v)) continue;
        if (c[1] == "N" && wholeNumber(v) && v >= 2 && v <= 60) { p.N = (int)v; used += " N " + c[2]; }
        else if (c[1] == "S" && wholeNumber(v) && v >= 1 && v <= 5) { p.S = (int)v; used += " S " + c[2]; }
        else if (c[1] == "T" && v >= 1.1 && v <= 10) { p.T = v; used += " T " + c[2]; }
        else if (c[1] == "T_overnight" && v >= 1.1 && v <= 10) { on = v; used += " T_overnight " + c[2]; }
    }
    if (on != S.tOvernight) { S.tOvernight = on; S.count = 0; }
    if (!used.empty() && (p.N != S.P.N || p.S != S.P.S || p.T != S.P.T)) { S.P = p; S.count = 0; }
    if (!used.empty()) S.params = "data:" + used;
}
void Exhaustion::status(EXState& S, long long now) {
    if (S.lastStatusMs >= 0 && now - S.lastStatusMs < 1000 && now >= S.lastStatusMs) return;
    S.lastStatusMs = now;
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << EX_VERSION << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nSTATE," << S.state
      << "\nPARAMS,N " << S.P.N << ",S " << S.P.S << ",T " << S.P.T << "," << S.params << "\nRTH," << S.rthO << "," << S.rthC << "\nOVERNIGHT_LINE," << S.tOvernight
      << "\nPROVEN," << S.proven << "\nNOW,buyers_fading " << S.liveB << ",sellers_fading " << S.liveS << "\nMARKS," << S.marks.size() << ",fails " << S.recFails << "\n";
    {   // (0.3.0) the pivot histogram
        long shown = 0; long long lh = 0, ll = 0; double rh = 0, rl = 0;
        for (std::map<long long, std::pair<int, double>>::const_iterator it = S.piv.begin(); it != S.piv.end(); ++it) {
            if (it->second.second + 1e-9 >= exp3::SHOW) ++shown;
            if (it->second.first > 0) { lh = it->first; rh = it->second.second; } else { ll = it->first; rl = it->second.second; }
        }
        {
            const size_t L = S.po.pb.size(); long nt = 0, ns = 0; for (std::map<long long, std::pair<int, int>>::const_iterator it = S.paceMarks.begin(); it != S.paceMarks.end(); ++it) (it->second.second == pace::TURNED ? nt : ns)++;
            if (L) { const size_t k = L - 1; const bool up = S.po.dir[k] > 0;
                o << "PACE,side " << (up ? "buyers" : "sellers") << ",pace " << (up ? S.po.pb[k] : S.po.ps[k]) << ",signal " << (up ? S.po.sb[k] : S.po.ss[k]) << ",push " << S.po.push[k] << ",atr " << S.po.atr[k] << ",marks slowed " << ns << " turned " << nt << "\n"; }
        }
        o << "PIVOTS,zigzag N " << exp3::ZZ_N << ",show " << exp3::SHOW << ",pivots " << S.piv.size() << ",shown " << shown << ",last_high " << lh << " " << rh << ",last_low " << ll << " " << rl << "\n";
    }
    const std::string text = o.str(); if (text == S.lastStatusText) return;
    const std::string f = flexDir(); if (f.empty()) return;
    if (publishFile(f + "\\Exhaustion.status-" + stem(S) + ".txt", text)) S.lastStatusText = text;
}
void Exhaustion::pump() {
    EXState* P = slot_.get(this, true);
    if (!P || P->busy) return;
    EXState& S = *P; S.busy = true;
    struct Unbusy { EXState* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
    try {
        identify(S);
        const long long now = steadyMs();
        const long long ct = ctSec(currentDate());
        if (ct > 0) { const long long key = (ct - 17LL * 3600) / 86400LL + 1; if (key != S.session) { S.session = key; S.marks.clear(); S.markVal.clear(); S.piv.clear(); S.paceMarks.clear(); load(S); } }
        if (S.lastNormsMs < 0 || now - S.lastNormsMs >= 60000) { S.lastNormsMs = now; norms(S); }
        const long count = getBarCount();
        calcBars(S, count);
        S.state = count < S.P.N + 2 ? "waiting for bars" : "ready";
        status(S, now);
    } catch (...) { S.state = "calculation error"; }
}
void Exhaustion::render(EXState& S) {
    RCT pane; pane.getPaneRect(false);
    const long count = getBarCount();
    if (count < 2 || pane.bottom - pane.top < 40 || pane.right - pane.left < 120 || S.out.fb.empty()) return;
    const int lastBar = (int)count - 2;                                   // (0.2.1) the last CLOSED bar
    FONT f; f.id = HELVETICA; f.size = 9; f.style = BOLD; setFont(f);
    auto txt = [&](int x, int y, const std::string& t, COLOR c) { setTextColor(c); const int w = getTextWidth(t.c_str(), -1); RCT r; r.set(coord(x), coord(y), coord(x + w + 2), coord(y + 12)); r.drawText(t.c_str(), false, false); return x + w; };
    auto xs = [](double v) { char b[16]; std::snprintf(b, sizeof(b), "%.1fx", v); return std::string(b); };
    auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
    const double T = exp3::SHOW;                                          // (0.3.0) the drawn line: 2.0x
    // (0.3.2, Rassul 06:04 "take off the pivot rule") the 0.2.4 measure on every closed bar (ex::compute); header = the last closed bar
    {
        int x = txt(pane.left + 4, pane.top + 1, "EXHAUSTION   ", C_GREY);
        x = txt(x, pane.top + 1, "Buyers fading: ", C_GREY); x = txt(x, pane.top + 1, xs(S.liveB), C_SELL); x = txt(x, pane.top + 1, " (" + xs(T) + ")   ", C_GREY);
        x = txt(x, pane.top + 1, "Sellers fading: ", C_GREY); x = txt(x, pane.top + 1, xs(S.liveS), C_BUY); x = txt(x, pane.top + 1, " (" + xs(T) + ")   ", C_GREY);
        // (0.4.0) the attacker's pace stage on the last closed bar: turned (pace under its signal) / slowed (above it, falling) / rising
        if (!S.po.pb.empty()) {
            const size_t k = S.po.pb.size() - 1; const bool up = S.po.dir[k] > 0;
            const std::vector<double>& pp = up ? S.po.pb : S.po.ps; const std::vector<double>& ss = up ? S.po.sb : S.po.ss;
            const char* st = pp[k] < ss[k] ? "turned" : (k > 0 && pp[k] < pp[k - 1]) ? "slowed" : "rising";
            x = txt(x, pane.top + 1, std::string("Pace (") + (up ? "buyers" : "sellers") + "): ", C_GREY); txt(x, pane.top + 1, st, up ? C_SELL : C_BUY);
        }
        // whether the nightly study has confirmed this market's E on his trade (unseen days): "testing" until it has
        const int w = getTextWidth(S.proven.c_str(), -1); txt(pane.right - w - 8, pane.top + 1, S.proven, C_GREY);
    }
    // (0.4.0) the pane is split: ratio spikes on top (0.3.2, unchanged), the attacker's PACE / SIGNAL band below on its own scale
    const int split = pane.top + 30 + (pane.bottom - pane.top - 34) * 55 / 100;
    const int top = pane.top + 30, bottom = split;
    auto val = [&](int bar, bool buyers) { const int k = bar - S.outFirst; if (k < 0 || k >= (int)S.out.fb.size()) return 1.0; return buyers ? S.out.fb[k] : S.out.fs[k]; };
    auto shown = [&](int bar, bool buyers) { const double v = val(bar, buyers); return v + 1e-9 >= T ? v : 0.0; };   // < 2x = 0 (still in the status / data)
    std::vector<int> vis;
    for (int i = lastBar; i >= S.outFirst; --i) { const int x = xOf(i); if (x < pane.left) break; if (x <= pane.right) vis.push_back(i); }
    std::reverse(vis.begin(), vis.end());
    double mx = 3.0; for (size_t k = 0; k < vis.size(); ++k) mx = std::max(mx, std::max(shown(vis[k], true), shown(vis[k], false)));
    mx = std::min(mx * 1.15, 12.0);
    auto yOf = [&](double v) { return bottom - (int)((bottom - top) * std::min(1.0, std::max(0.0, v / mx))); };
    setPen(C_AXIS, 1, P_SOLID);
    { const int y = yOf(T); PNT a; a.set(0, 0.0f); a.h = coord(pane.left); a.v = coord(y); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(pane.right); b.v = coord(y); b.drawLineTo();
      txt(pane.right - 34, y - 13, xs(T), C_GREY); }
    // (0.3.1) two connected lines, one per side: 0 on every bar, rising to the ratio on a bar at / over 2.0x, its value at the peak
    for (int side = 0; side < 2; ++side) {
        const bool buyers = side == 0; const COLOR c = buyers ? C_SELL : C_BUY;   // buyers fading = bearish red, sellers fading = bullish green
        setPen(c, 2, P_SOLID); bool started = false;
        for (size_t k = 0; k < vis.size(); ++k) {
            PNT q; q.set(0, 0.0f); q.h = coord(xOf(vis[k])); q.v = coord(yOf(shown(vis[k], buyers)));
            if (!started) { q.setDrawPosition(); started = true; } else q.drawLineTo();
        }
        for (size_t k = 0; k < vis.size(); ++k) {
            const double v = shown(vis[k], buyers); if (v <= 0) continue;
            const int x = xOf(vis[k]), y = yOf(v);
            const std::string t = xs(v); const int w = getTextWidth(t.c_str(), -1); txt(x - w / 2, y - 14, t, c);
        }
    }
    // (0.4.0) PACE band: the side attacking the current push (last closed bar) - its pace (red buyers / green sellers, 2 px) and its
    // 9-bar EMA signal (yellow, sellers dimmer, 1 px); stage marks of that side: SLOWED = grey dot on the pace line, TURNED = triangle
    // in the side's colour at the cross + "T". Marks are the recorded ones (decided on the close, never repainted).
    if (!S.po.pb.empty() && (int)S.po.pb.size() == (int)S.out.fb.size()) {
        const int pTop = split + 6, pBot = pane.bottom - 4;
        const size_t L = S.po.pb.size() - 1; const bool up = S.po.dir[L] > 0;
        const std::vector<double>& pp = up ? S.po.pb : S.po.ps; const std::vector<double>& sg = up ? S.po.sb : S.po.ss;
        const COLOR cp = up ? C_SELL : C_BUY, cs = up ? C_SIG : C_SIG_DIM;
        double pm = 1.0; for (size_t k = 0; k < vis.size(); ++k) { const int j = vis[k] - S.outFirst; if (j >= 0 && j < (int)pp.size()) pm = std::max(pm, std::max(pp[(size_t)j], sg[(size_t)j])); }
        pm *= 1.1;
        auto yP = [&](double v) { return pBot - (int)((pBot - pTop) * std::min(1.0, std::max(0.0, v / pm))); };
        setPen(C_AXIS, 1, P_SOLID);
        { PNT a; a.set(0, 0.0f); a.h = coord(pane.left); a.v = coord(split + 2); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(pane.right); b.v = coord(split + 2); b.drawLineTo(); }
        for (int line = 0; line < 2; ++line) {
            const std::vector<double>& v = line == 0 ? sg : pp; setPen(line == 0 ? cs : cp, line == 0 ? 1 : 2, P_SOLID); bool started = false;
            for (size_t k = 0; k < vis.size(); ++k) {
                const int j = vis[k] - S.outFirst; if (j < 0 || j >= (int)v.size()) continue;
                PNT q; q.set(0, 0.0f); q.h = coord(xOf(vis[k])); q.v = coord(yP(v[(size_t)j]));
                if (!started) { q.setDrawPosition(); started = true; } else q.drawLineTo();
            }
        }
        RTARRAYI tms(barDateTime);
        if (tms.count >= count) for (size_t k = 0; k < vis.size(); ++k) {
            std::map<long long, std::pair<int, int>>::const_iterator m = S.paceMarks.find((long long)tms[vis[k]]);
            if (m == S.paceMarks.end() || m->second.first != (up ? 1 : -1)) continue;
            const int j = vis[k] - S.outFirst; if (j < 0 || j >= (int)pp.size()) continue;
            const int x = xOf(vis[k]), y = yP(pp[(size_t)j]);
            if (m->second.second == pace::SLOWED) { RCT d; d.set(coord(x - 2), coord(y - 2), coord(x + 3), coord(y + 3)); d.draw(0, C_DOT, C_DOT, DRAW_OPAQUE, PAT_SOLID); }
            else {
                setPen(cp, 1, P_SOLID);
                PNT a; a.set(0, 0.0f); a.h = coord(x); a.v = coord(y + 2); a.setDrawPosition();
                PNT b; b.set(0, 0.0f); b.h = coord(x - 4); b.v = coord(y + 9); b.drawLineTo();
                PNT c; c.set(0, 0.0f); c.h = coord(x + 4); c.v = coord(y + 9); c.drawLineTo();
                PNT e; e.set(0, 0.0f); e.h = coord(x); e.v = coord(y + 2); e.drawLineTo();
                txt(x - 3, y + 10, "T", cp);
            }
        }
    }
}

extern "C" cppExtension* CreateExtension(void) {
    Exhaustion* p = new Exhaustion();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED);
    p->setDescription("LRA Exhaustion (0.4.0): top - two lines at 0 that rise on a closed bar at / over 2.0x (red = buyers fading at a new N-bar high: their peak pace in the push since the lowest low of the last 20 bars / their pace on this bar; green = sellers fading at a new N-bar low). Bottom - the attacking side's PACE (aggressive volume averaged over S bars) and its 9-bar EMA SIGNAL (yellow): grey dot = slowed, T = pace crossed under its signal (pushes of 2 ATR or more). Closed bars only, never repainted.");
    p->setVersion("0.4.0");   // EX_VERSION
    return p;
}
