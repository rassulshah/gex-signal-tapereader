/********************************************************************************
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
#include "SignalBoard.h"   // (0.3.0) reads lsLiquidityProfile 0.68.0's order-line HELD facts (kind G) for the T mark
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

#define EX_VERSION "0.4.0"          // (0.4.0, 2026-10-29 Rassul approved 11:18 CT, mockup artifact FzpVHQZ8ykBaoz4q6LMsHk) THE NET DISPLAY: a histogram of net aggression per closed candle (lots at the ask - lots at the bid; green >= 0 / red < 0, own symmetric scale, 0 line); the fading ratios on NET (peak 2-bar net in the push / now, floor 10% of the peak, cap 10x, ExhaustionLogic computeNet); labels >= 2.0x, one per run at its newest bar (red top = buyers fading, green bottom = sellers fading), only when the push peak >= the median |net| of the last 100 bars (noise cap; else logged); header "EXHAUSTION (net)  Buyers fading: X.Xx (2.0x) \xB7 Sellers fading: X.Xx (2.0x)" + an (i) info box (lines <= 60 chars, one draw call per line, inside the pane); hover on a bar = buy / sell / net / fading; the T mark now uses the NET ratio; every final net label (and the old gross ratio beside it) recorded to <stem>-net-<session>.csv; the old lines / E? are no longer drawn (E still recorded)   // was 0.3.1: (0.3.1, 2026-10-29 Rassul approved 10:23 CT; 0.3.1 because the reverted 05:11 CT pivot / histogram build was 0.3.0) + the T mark at the bottom of the pane: an order-line HELD (lsLiquidityProfile 0.68.0, SignalBoard G) + exhaustion >= 2.0x of the attacking side within the 5 bars before + a close back away within 3 bars after (ex::tMark); red T = a top (offer HELD), green T = a bottom (bid HELD); recorded to <stem>-T-<session>.csv, read back, never repainted   // was 0.2.4: keep equal to ex::VERSION and the setVersion literal below

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
struct PendingRecord { long long session, bar; int side; double val; std::string line; };

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
    std::vector<double> closes;                             // (0.3.0) the closed bars' closes (same index as out)
    std::map<long long, int> tmarks; std::map<long long, double> tVal; std::set<std::string> tDone;   // (0.3.0) T: bar time -> side; HELD facts already decided
    long long tFacts = 0, tWrites = 0, tFails = 0; bool tBoard = false;
    ex::NetOut net; std::vector<double> buyV, sellV;        // (0.4.0) the NET display (same index as out)
    std::set<long long> netDone; long long netWrites = 0, netFails = 0, netNoisy = 0;   // final net labels recorded (bar time)
    int iconL = 0, iconT = 0, iconR = -1, iconB = -1; bool tipOn = false; int hovBar = -1, hovH = 0, hovV = 0;   // (0.4.0) the (i) icon + the bar hover
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
    void tCheck(EXState& S, long count);   // (0.3.0) the T mark
    void netRecord(EXState& S, long count);   // (0.4.0) final net labels -> <stem>-net-<session>.csv
public:
    virtual int mouse(RTX_EVENT* ev);         // (0.4.0) the (i) box + the bar hover
private:
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
    S.P = ex::defaults(S.market); S.params = "study 2026-10-11"; S.vol.clear(); S.marks.clear(); S.markVal.clear(); S.pending.clear();
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
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), clo(barClose); RTARRAYP vap(barVolumeProfile);
    if (times.count < count || hi.count < count || lo.count < count || vap.count < count || clo.count < count) return;
    const int first = (int)std::max(0L, count - BARS_MAX);
    std::vector<double> h, l, b, s, cc; h.reserve(count - first); cc.reserve(count - first); l.reserve(count - first); b.reserve(count - first); s.reserve(count - first);
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
        h.push_back(hh); l.push_back(ll); b.push_back(v.first); s.push_back(v.second); cc.push_back((double)clo[i]);
    }
    S.out = ex::compute(h, l, b, s, S.P); S.outFirst = first; S.closes.swap(cc);
    S.net = ex::computeNet(h, l, b, s); S.buyV = b; S.sellV = s;   // (0.4.0)
    if (S.out.fb.empty()) return;
    const int f = (int)count - 2 - first;
    S.liveB = S.out.fb[f]; S.liveS = S.out.fs[f];
    if ((int)S.net.fb.size() > f) { S.liveB = S.net.fb[f]; S.liveS = S.net.fs[f]; }   // (0.4.0) the header shows the NET ratios
    // closed bars: an E in RTH is recorded once (new closed bars only after the first pass)
    const long from = S.count == 0 ? first : std::max((long)first, S.count - 1);
    for (long i = from; i < count - 1; ++i) {
        const int k = (int)(i - first);
        const long long bt = (long long)times[(int)i];
        bool queued = false; for (size_t p = 0; p < S.pending.size(); ++p) if (S.pending[p].bar == bt) { queued = true; break; }
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
    netRecord(S, count);   // (0.4.0)
    tCheck(S, count);   // (0.3.0)
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
    std::map<long long, std::vector<size_t> > groups;
    for (size_t i = 0; i < S.pending.size(); ++i) groups[S.pending[i].session].push_back(i);
    std::vector<char> committed(S.pending.size(), 0);
    for (std::map<long long, std::vector<size_t> >::const_iterator g = groups.begin(); g != groups.end(); ++g) {
        std::vector<const PendingRecord*> add; add.reserve(g->second.size());
        for (size_t j = 0; j < g->second.size(); ++j) add.push_back(&S.pending[g->second[j]]);
        if (!publishRecords(d + "\\" + stem(S) + "-" + std::to_string(g->first) + ".csv", add)) { ++S.recFails; continue; }
        for (size_t j = 0; j < g->second.size(); ++j) {
            const size_t k = g->second[j]; S.marks[S.pending[k].bar] = S.pending[k].side; S.markVal[S.pending[k].bar] = S.pending[k].val; committed[k] = 1;
        }
    }
    std::vector<PendingRecord> retry; retry.reserve(S.pending.size());
    for (size_t i = 0; i < S.pending.size(); ++i) if (!committed[i]) retry.push_back(S.pending[i]);
    S.pending.swap(retry);
    return S.pending.empty();
}
// (0.3.0) the T mark: each order-line HELD fact (lsLiquidityProfile, kind G, decided) mapped to its decision bar (fact bar = the bar END in
// CT seconds, the same clock as ctSec(bar stamp)); ex::tMark on CLOSED bars; a decided T is appended to <stem>-T-<session>.csv and shown
// only after a good append (never repainted); a HELD that can no longer qualify (its windows closed) is marked done (no more checks)
void Exhaustion::tCheck(EXState& S, long count) {
    std::vector<sb::Fact> G; const char* clk = getChartLabel();
    S.tBoard = sb::readFacts('G', S.symbol, S.spb, clk ? std::string(clk) : std::string(), steadyMs(), G, true);
    if (G.empty() || S.out.fb.empty() || S.closes.size() != S.out.fb.size()) return;
    RTARRAYI times(barDateTime); if (times.count < count) return;
    std::map<long long, int> idx; for (int k = 0; k < (int)S.closes.size(); ++k) { const long long cs = ctSec((RTDATE)times[S.outFirst + k]); if (cs > 0) idx[cs] = k; }
    const int lastClosed = (int)S.closes.size() - 1;
    for (const sb::Fact& f : G) {
        char kb[96]; std::snprintf(kb, sizeof(kb), "%lld|%d|%.6f", f.bar, f.side, f.lo); const std::string key = kb;
        if (S.tDone.count(key)) continue;
        std::map<long long, int>::const_iterator it = idx.find(f.bar); if (it == idx.end()) continue;
        ++S.tFacts;
        ex::Held H; H.bar = it->second; H.side = f.side; H.px = f.lo; ex::TMark M;
        ex::Out nr; nr.fb = S.net.fb; nr.fs = S.net.fs;   // (0.4.0) T uses the NET fading ratio
        if (nr.fb.size() != S.closes.size()) continue;
        if (!ex::tMark(S.closes, nr, H, lastClosed, M)) { if (lastClosed >= H.bar + ex::T_AFTER) S.tDone.insert(key); continue; }   // every window is over
        const long long bt = (long long)times[S.outFirst + M.bar]; const long long cs = ctSec((RTDATE)bt);
        const long long sess = cs > 0 ? (cs - 17LL * 3600) / 86400LL + 1 : S.session;
        const std::string d = dir(); if (d.empty()) { ++S.tFails; continue; }
        makeDir(flexDir()); makeDir(d);
        char line[200]; std::snprintf(line, sizeof(line), "%lld|%s|%.2f|%.6g|%lld\n", bt, M.side < 0 ? "top" : "bottom", M.exh, f.lo, f.bar);
        std::ofstream o((d + "\\" + stem(S) + "-T-" + std::to_string(sess) + ".csv").c_str(), std::ios::binary | std::ios::app);
        if (o) { o << line; o.flush(); }
        if (!o) { ++S.tFails; continue; }
        S.tmarks[bt] = M.side; S.tVal[bt] = M.exh; S.tDone.insert(key); ++S.tWrites;
    }
    while (S.tDone.size() > 2000) S.tDone.erase(S.tDone.begin());
}
// (0.4.0) every FINAL net label (its run ended: a later closed bar did not continue it) - labelled or held back by the noise cap - with the
// old GROSS ratio of the same bar beside it (the nightly compares them): bar|top/bottom|net_x|gross_x|peak_net|median_abs_net|labelled
void Exhaustion::netRecord(EXState& S, long count) {
    if (S.net.fb.empty() || S.net.fb.size() != S.out.fb.size()) return;
    RTARRAYI times(barDateTime); if (times.count < count) return;
    const std::string d = dir(); if (d.empty()) return;
    for (int k = 0; k < (int)S.net.fb.size(); ++k) {
        for (int side = 0; side < 2; ++side) {
            const bool fin = side == 0 ? S.net.runEndB[k] : S.net.runEndS[k]; if (!fin) continue;
            const bool lab = side == 0 ? S.net.labB[k] : S.net.labS[k];
            const long long bt = (long long)times[S.outFirst + k]; const long long key = bt * 2 + side;
            if (S.netDone.count(key)) continue;
            const long long cs = ctSec((RTDATE)bt); const long long sess = cs > 0 ? (cs - 17LL * 3600) / 86400LL + 1 : S.session;
            makeDir(flexDir()); makeDir(d);
            char line[200]; std::snprintf(line, sizeof(line), "%lld|%s|%.2f|%.2f|%.0f|%.0f|%d\n", bt, side == 0 ? "top" : "bottom", side == 0 ? S.net.fb[k] : S.net.fs[k],
                side == 0 ? S.out.fb[k] : S.out.fs[k], side == 0 ? S.net.peakB[k] : S.net.peakS[k], S.net.med[k], lab ? 1 : 0);
            std::ofstream o((d + "\\" + stem(S) + "-net-" + std::to_string(sess) + ".csv").c_str(), std::ios::binary | std::ios::app);
            if (o) { o << line; o.flush(); }
            if (!o) { ++S.netFails; continue; }
            S.netDone.insert(key); ++S.netWrites; if (!lab) ++S.netNoisy;
        }
    }
    while (S.netDone.size() > 6000) S.netDone.erase(S.netDone.begin());
}
void Exhaustion::load(EXState& S) {
    const std::string d = dir(); if (d.empty()) return;
    for (long long k = S.session - 9; k <= S.session; ++k) {
        std::vector<DiskRecord> records;                                // (0.2.2) bounded, header-checked
        if (!readRecords(d + "\\" + stem(S) + "-" + std::to_string(k) + ".csv", records)) { ++S.recFails; continue; }
        for (size_t i = 0; i < records.size(); ++i) { S.marks[records[i].bar] = records[i].side; S.markVal[records[i].bar] = records[i].val; }
    }
    for (long long k = S.session - 9; k <= S.session; ++k) {   // (0.3.0) the T marks (bounded: 2,000 lines)
        std::ifstream in((d + "\\" + stem(S) + "-T-" + std::to_string(k) + ".csv").c_str(), std::ios::binary); std::string ln; int n = 0;
        while (in && n++ < 2000 && std::getline(in, ln)) { const size_t a = ln.find('|'); if (a == std::string::npos) continue; char* e = nullptr; const long long bt = std::strtoll(ln.c_str(), &e, 10);
            if (e != ln.c_str() + a || bt <= 0) continue;
            const bool top = ln.compare(a + 1, 3, "top") == 0, bot = ln.compare(a + 1, 6, "bottom") == 0;
            if (top || bot) S.tmarks[bt] = top ? -1 : 1; }
    }
    for (long long k = S.session - 1; k <= S.session; ++k) {   // (0.4.0) final net labels already recorded (never written twice)
        std::ifstream in((d + "\\" + stem(S) + "-net-" + std::to_string(k) + ".csv").c_str(), std::ios::binary); std::string ln; int n = 0;
        while (in && n++ < 6000 && std::getline(in, ln)) { const size_t a = ln.find('|'); if (a == std::string::npos) continue; const long long bt = std::strtoll(ln.c_str(), nullptr, 10);
            if (bt > 0) S.netDone.insert(bt * 2 + (ln.compare(a + 1, 3, "top") == 0 ? 0 : 1)); }
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
      << "\nPROVEN," << S.proven << "\nNOW,buyers_fading " << S.liveB << ",sellers_fading " << S.liveS << "\nMARKS," << S.marks.size() << ",fails " << S.recFails << "\n"
      << "NET,display net aggression (ask - bid lots),labels >= " << ex::NET_LABEL << "x one per run,noise cap = median |net| of the last " << ex::NET_MED_BARS << " bars = " << (S.net.med.empty() ? 0.0 : S.net.med.back()) << " lots,final labels recorded " << S.netWrites << " (held back by the noise cap " << S.netNoisy << ") fails " << S.netFails << "\n"
      << "T_MARK,HELD facts (SignalBoard G) " << (S.tBoard ? "read" : "none") << ",checked " << S.tFacts << ",T recorded " << S.tWrites << " fails " << S.tFails << ",shown marks " << S.tmarks.size() << ",rule exh >= " << ex::T_EXH << "x within " << ex::T_BEFORE << " bars before + close back within " << ex::T_AFTER << " bars" << "\n";
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
        if (ct > 0) { const long long key = (ct - 17LL * 3600) / 86400LL + 1; if (key != S.session) { S.session = key; S.marks.clear(); S.markVal.clear(); S.tmarks.clear(); S.tVal.clear(); S.netDone.clear(); load(S); } }
        if (S.lastNormsMs < 0 || now - S.lastNormsMs >= 60000) { S.lastNormsMs = now; norms(S); }
        const long count = getBarCount();
        calcBars(S, count);
        S.state = count < S.P.N + 2 ? "waiting for bars" : "ready";
        status(S, now);
    } catch (...) { S.state = "calculation error"; }
}
void Exhaustion::render(EXState& S) {
    // (0.4.0) THE NET DISPLAY: header + (i), the net histogram (own symmetric scale, 0 line), the 2.0x+ labels (one per run, red top /
    // green bottom), the T marks at the bottom, the (i) box and the bar hover - every text line its own draw call, <= 60 characters
    RCT pane; pane.getPaneRect(false);
    const long count = getBarCount();
    S.iconR = -1;
    if (count < 2 || pane.bottom - pane.top < 40 || pane.right - pane.left < 120 || S.net.net.empty()) return;
    const int lastBar = (int)count - 2;                                   // the last CLOSED bar
    FONT f; f.id = HELVETICA; f.size = 10; f.style = BOLD; setFont(f);
    auto txt = [&](int x, int y, const std::string& t, COLOR c) { setTextColor(c); const int w = getTextWidth(t.c_str(), -1); RCT r; r.set(coord(x), coord(y), coord(x + w + 2), coord(y + 13)); r.drawText(t.c_str(), false, false); return x + w; };
    auto xs = [](double v) { char b[16]; std::snprintf(b, sizeof(b), "%.1fx", v); return std::string(b); };
    auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
    {   int x = txt(pane.left + 4, pane.top + 1, "EXHAUSTION (net)   ", C_GREY);
        x = txt(x, pane.top + 1, "Buyers fading: ", C_GREY); x = txt(x, pane.top + 1, xs(S.liveB), C_SELL); x = txt(x, pane.top + 1, " (2.0x)  \xB7  ", C_GREY);
        x = txt(x, pane.top + 1, "Sellers fading: ", C_GREY); x = txt(x, pane.top + 1, xs(S.liveS), C_BUY); x = txt(x, pane.top + 1, " (2.0x)", C_GREY);
        const int il = x + 10, it = pane.top + 2, ip = 12;                // the (i) icon (lsBookFlow's look: light grey dot, black i)
        if (il + ip < pane.right - 80) {
            setPen(0x009CA3AF, 1, P_SOLID); CBRUSH br(0x009CA3AF, PAT_SOLID); br.set();
            RCT d; d.set(coord(il), coord(it), coord(il + ip), coord(it + ip)); d.drawOval(DRAW_OPAQUE);
            FONT fi = f; fi.size = 9; setFont(fi); setTextColor(0x00000000); const int iw = getTextWidth("i", -1);
            RCT ti; ti.set(coord(il + (ip - iw) / 2), coord(it - 1), coord(il + ip), coord(it + ip)); ti.drawText("i", false, false); setFont(f);
            S.iconL = il; S.iconT = it; S.iconR = il + ip; S.iconB = it + ip; }
        const std::string vr = std::string("v") + EX_VERSION + "  " + S.proven; const int w = getTextWidth(vr.c_str(), -1); txt(pane.right - w - 8, pane.top + 1, vr, C_GREY);
    }
    const int top = pane.top + 30, bottom = pane.bottom - 30;            // label rows above / below (top: buyers, bottom: sellers, then T)
    if (bottom - top < 20) return;
    std::vector<int> vis;
    for (int i = lastBar; i >= S.outFirst; --i) { const int x = xOf(i); if (x < pane.left) break; if (x <= pane.right) vis.push_back(i); }
    std::reverse(vis.begin(), vis.end());
    auto K = [&](int bar) { return bar - S.outFirst; };
    double mx = 1; for (int i : vis) { const int k = K(i); if (k >= 0 && k < (int)S.net.net.size()) mx = std::max(mx, std::fabs(S.net.net[k])); }
    const int zero = (top + bottom) / 2, half = (bottom - top) / 2;
    auto yOf = [&](double v) { return zero - (int)std::lround((double)half * std::max(-1.0, std::min(1.0, v / mx))); };
    setPen(C_AXIS, 1, P_SOLID);
    { PNT a; a.set(0, 0.0f); a.h = coord(pane.left); a.v = coord(zero); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(pane.right); b.v = coord(zero); b.drawLineTo(); }
    int bw = 3; if (vis.size() >= 2) bw = std::max(1, (int)((xOf(vis.back()) - xOf(vis.front())) / (int)(vis.size() - 1) * 0.6 / 2));
    for (int i : vis) {
        const int k = K(i); if (k < 0 || k >= (int)S.net.net.size()) continue;
        const double v = S.net.net[k]; const COLOR c = v >= 0 ? C_BUY : C_SELL; const int x = xOf(i), y = yOf(v);
        RCT r; r.set(coord(x - bw), coord(std::min(y, zero)), coord(x + bw), coord(std::max(y, zero) + (y == zero ? 1 : 0))); r.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID);
    }
    for (int i : vis) {                                                    // the labels: one per run at its newest bar, noise-capped
        const int k = K(i); if (k < 0 || k >= (int)S.net.net.size()) continue;
        if (S.net.labB[k]) { const std::string t = xs(S.net.fb[k]); const int w = getTextWidth(t.c_str(), -1); txt(xOf(i) - w / 2, top - 14, t, C_SELL); }
        if (S.net.labS[k]) { const std::string t = xs(S.net.fs[k]); const int w = getTextWidth(t.c_str(), -1); txt(xOf(i) - w / 2, bottom + 1, t, C_BUY); }
    }
    RTARRAYI times(barDateTime);                                          // T: the recorded marks at the bottom of the pane (never repainted)
    if (times.count >= count) for (int i : vis) {
        std::map<long long, int>::const_iterator m = S.tmarks.find((long long)times[i]); if (m == S.tmarks.end()) continue;
        txt(xOf(i) - 3, bottom + 14, "T", m->second < 0 ? C_SELL : C_BUY);
    }
    // a multi-line box: one draw call per line (never one long string - IRT breaks it), sized to the text, kept inside the pane
    auto box = [&](int ax, int ay, const std::vector<std::string>& L) {
        if (L.empty()) return;
        FONT fp = f; fp.size = 9; fp.style = PLAIN; setFont(fp);
        int w = 0; for (const std::string& t : L) w = std::max(w, (int)getTextWidth(t.c_str(), -1));
        const int lh = 13, pad = 6, bw2 = w + 2 * pad + 2, bh = (int)L.size() * lh + 2 * pad;
        int l = std::min(ax, (int)pane.right - 2 - bw2); l = std::max((int)pane.left + 2, l);
        int t = ay; if (t + bh > pane.bottom - 2) t = std::max((int)pane.top + 2, (int)pane.bottom - 2 - bh);
        const int r = std::min((int)pane.right - 2, l + bw2), b = std::min((int)pane.bottom - 2, t + bh);
        setPen(0x009CA3AF, 1, P_SOLID); RCT bg; bg.set(coord(l), coord(t), coord(r), coord(b)); bg.draw(1, 0x009CA3AF, 0x00111827, DRAW_OPAQUE, PAT_SOLID);
        setTextColor(0x00E5E7EB); int y = t + pad;
        for (const std::string& line : L) { if (y + lh > b) break; if (!line.empty()) { RCT tr; tr.set(coord(l + pad), coord(y), coord(r - 2), coord(y + lh)); tr.drawText(line.c_str(), false, false); } y += lh; }
        setFont(f); };
    if (S.tipOn && S.iconR > S.iconL) box(S.iconL, S.iconB + 3, ex::wrap(ex::INFO_TEXT, 60));
    else if (S.hovBar >= S.outFirst && S.hovBar <= lastBar) {
        const int k = K(S.hovBar);
        if (k >= 0 && k < (int)S.net.net.size() && k < (int)S.buyV.size()) {
            char b1[64], b2[64], b3[64], b4[96]; std::string when;
            if (times.count > S.hovBar) { struct tm t; std::memset(&t, 0, sizeof(t)); if (getLocaltime((RTDATE)times[S.hovBar], &t)) { char w[32]; std::snprintf(w, sizeof(w), "Candle ending %d:%02d %s", t.tm_hour % 12 == 0 ? 12 : t.tm_hour % 12, t.tm_min, t.tm_hour < 12 ? "AM" : "PM"); when = w; } }
            std::snprintf(b1, sizeof(b1), "Bought at the ask: %.0f lots", S.buyV[k]); std::snprintf(b2, sizeof(b2), "Sold at the bid: %.0f lots", S.sellV[k]);
            std::snprintf(b3, sizeof(b3), "Net aggression: %+.0f lots", S.net.net[k]);
            if (S.net.newH[k]) std::snprintf(b4, sizeof(b4), "New high - buyers fading: %.1fx (2.0x)", S.net.fb[k]);
            else if (S.net.newL[k]) std::snprintf(b4, sizeof(b4), "New low - sellers fading: %.1fx (2.0x)", S.net.fs[k]);
            else std::snprintf(b4, sizeof(b4), "Not at a new 5-bar high or low: no fading read");
            std::vector<std::string> L; if (!when.empty()) L.push_back(when); for (const char* q : { b1, b2, b3, b4 }) { std::vector<std::string> w = ex::wrap(q, 60); L.insert(L.end(), w.begin(), w.end()); }
            box(S.hovH + 12, S.hovV + 12, L);
        }
    }
}
// (0.4.0) the mouse: the (i) icon shows the info box; on a candle, that candle's numbers. Returns RTX_FAIL so the event is not swallowed.
int Exhaustion::mouse(RTX_EVENT* ev) {
    EXState* S = slot_.get(this, false);
    if (!S || !ev || (ev->type != E_MOUSE_MOVE && ev->type != E_MOUSE_HOVER)) return RTX_FAIL;
    const int h = ev->v.mouse.h, v = ev->v.mouse.v;
    const bool on = S->iconR > S->iconL && h >= S->iconL - 2 && h <= S->iconR + 2 && v >= S->iconT - 2 && v <= S->iconB + 2;
    const int bar = (int)ev->v.mouse.bar;
    const bool changed = on != S->tipOn || bar != S->hovBar;
    S->tipOn = on; S->hovBar = bar; S->hovH = h; S->hovV = v;
    if (changed) invalidateChart(false);
    return RTX_FAIL;
}

extern "C" cppExtension* CreateExtension(void) {
    Exhaustion* p = new Exhaustion();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED | TRACK_MOUSE);   // (0.4.0) TRACK_MOUSE: the (i) box and the bar hover
    p->setDescription("LRA Exhaustion: is the side pushing price running out? Red = buyers fading at a new high (bearish), green = sellers fading at a new low (bullish): their peak pace in the push / their pace now, 1.0 when price is not at a new extreme. E = the first RTH bar at or over the market's line (from the nightly study), recorded and never repainted.");
    p->setVersion("0.4.0");   // EX_VERSION
    return p;
}
