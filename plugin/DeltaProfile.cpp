/********************************************************************************
 * DeltaProfile.cpp -- lsDeltaProfile 2.5.4
 * Native chart VAP; rolling (latest bar close - 90 min, latest bar close].
 * Right placement, left-facing one-sided bars, automatic price-anchored rows.
 * Only Width / Gap / Font are configurable. Delta = ask/buy minus bid/sell.
 * A/A? = absorption heuristic; I/I? = initiative heuristic; rings only for A/A?.
 * Every absorption candidate is recorded (2.5.4) to lsFlexLevels\DeltaProfile\<MKT>-<sec/bar>-signals-<session>.csv when
 * it first appears and when it is decided; the record is re-read after an IRT restart, so circles and letters survive it.
 * This revision deliberately requires Investor/RT replay acceptance before use.
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
// The SDK selects its old Mac branch solely by __GNUC__. Clang Windows checking
// can use this override; the production MSVC build uses the unmodified header.
#if defined(DELTA_WINDOWS_SDK_CHECK) && defined(__GNUC__)
#pragma push_macro("__GNUC__")
#undef __GNUC__
#include "irtsdk.h"
#pragma pop_macro("__GNUC__")
#else
#include "irtsdk.h"
#endif
#include "DeltaProfileCore.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <sys/stat.h>
#if defined(_WIN32)
#include <windows.h>
#endif

namespace dp = delta_profile;
static const char* const DLT_VERSION = "2.5.4";   // (2.5.4) every absorption candidate is written to a per-session record file (DeltaProfile\<MKT>-<sec/bar>-signals-<session>.csv) when it first appears (?) and when it is decided (A / I), append-only; after an IRT restart the session record is re-read so circles and letters survive it; nothing is drawn differently   // (2.5.3) the session record clears at 17:00 Central, not at 17:00 UTC (= noon CT) - RA1009 audit   // (2.5.2) every absorption candidate is tracked from its first appearance with a frozen zone and decided by the chart's own closed bars - top-3 changes no longer change a signal; off-profile signals keep their letter   // (2.5.1) the session record survives chart-state resets and is matched by bar time - a decided A and its circle can no longer vanish   // (2.5.0) confirmation = the first red (below) / green (above) 3-min close beyond the node (no 15-min hold); Acc / Dst zones no longer drawn (absorption only); a confirmed signal and its circle are LOCKED for the session (no repainting)   // (2.4.3) a circle on the bar of EVERY absorption (A / A?) - nodes whose letter does not fit and absorption zones too   // (2.4.2) 2.4.1-review (external audit) + live-safety fixes: USERPROFILE paths, host-state fallback, tolerant VAP reads, sticky A, 2-tick zones
static const COLOR C_BUY = 0x0022C55E, C_SELL = 0x00EF4444;
static const COLOR C_VOL = 0x00243040, C_AXIS = 0x00334155;
static const COLOR C_INK = 0x00E5E7EB, C_MUTED = 0x009CA3AF;
static const COLOR C_SUPPORT = 0x0086EFAC, C_RESISTANCE = 0x00FCA5A5;
static const int WIDTH_INDEX = 0, GAP_INDEX = 1, FONT_INDEX = 2;
struct Layout {
    int width, gap, font;
    Layout() : width(50), gap(0), font(9) {}
};
static void validateLayout(Layout& s) {
    s.width = s.width < 30 ? 50 : std::min(s.width, 600);
    s.gap = std::max(-300, std::min(s.gap, 400));
    s.font = s.font < 6 ? 9 : std::min(s.font, 18);
}
static std::int64_t steadyMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
static std::string safeField(std::string s) {
    for (std::size_t i = 0; i < s.size(); ++i)
        if (s[i] == '|' || s[i] == ',' || s[i] == '\n' || s[i] == '\r') s[i] = ' ';
    return s;
}
static std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string token;
    std::istringstream in(s);
    while (std::getline(in, token, sep)) {
        if (!token.empty() && token.back() == '\r') token.pop_back();
        out.push_back(token);
    }
    return out;
}
static bool integer(const std::string& s, int& out) {
    std::istringstream in(s); long long value;
    if (!(in >> value)) return false;
    in >> std::ws;
    if (!in.eof() || value < INT_MIN || value > INT_MAX) return false;
    out = static_cast<int>(value); return true;
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
static bool atomicWrite(const std::string& path, const std::string& contents) {
    const std::string temporary = path + ".tmp";
    {
        std::ofstream f(temporary.c_str(), std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << contents; f.flush(); if (!f) return false;
    }
#if defined(_WIN32)
    if (MoveFileExA(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
#else
    if (std::rename(temporary.c_str(), path.c_str()) == 0) return true;
#endif
    std::remove(temporary.c_str()); return false;
}
// Market alias is only for existing Dealer Profile layout interoperability.
// It NEVER chooses the tick size or drives delta classification.
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
    return r;
}
static double marketTick(const std::string& m) {
    return m == "ES" || m == "NQ" ? 0.25 : m == "CL" ? 0.01 : m == "NG" ? 0.001 : m == "GC" ? 0.1 :
           m == "HG" ? 0.0005 : m == "EU" ? 0.00005 : 0.0;
}
// (2.5.0, Rassul 2026-10-09 13:44 "why does the code change signals and take away the circle once the rules have been
// satisfied") a decided signal is written to this session's record and never undone by the sliding 90-min profile:
// later builds that see the same price zone undecided show the recorded result, and an absorption's circle stays
// drawn even when the node drops out of the profile. Cleared at the 17:00 session change.
struct Latch {
    dp::Tick low, high; std::string code; bool support; RTDATE peakTime; double price; bool sell;
    // (2.5.4) the record only - never read by the drawing: id (creation order), CT-clock bar ends (epoch-as-UTC, TapeFlow's
    // convention), the node's net delta and multiple, where it came from, and whether its decided line is already queued
    long long id, tFirst, tDecided, decidedBy, peakT; double net, mult; bool zone, logged;
    Latch() : low(0), high(0), support(false), peakTime(0), price(0), sell(false), id(-1), tFirst(0), tDecided(0), decidedBy(0),
        peakT(0), net(0), mult(0), zone(false), logged(false) {}
};
// (2.5.1) the record lives OUTSIDE the per-chart state, keyed by symbol and bar size: a chart-state reset (IRT's host
// switching contexts, a periodicity label read empty, two charts sharing one object) no longer wipes it, and the circle is
// stored by bar TIME (re-found on every draw) so a chart reload that shifts bar numbers cannot move it.
struct LatchBook {
    std::vector<Latch> v; long long session = -1;
    // (2.5.4) the session's record file: lines waiting to be appended (kept until a flushed, good write), next id, diagnostics
    std::vector<std::string> journal; std::string path, note; long long nextId = 0, written = 0, loaded = 0, rejected = 0;
};
// ---- (2.5.4) the session signal record. One pipe-separated line per event, append-only, never rewritten:
//   F = the candidate first appeared (code "A?"), D = it was decided (A / I / Acc / Dst). A line is complete only when it ends
//   in a newline; a torn last line is ended before the next append and ignored when read. Several roots of one market (ES and
//   MES) may share a file: every line names its root and its id within that root's book.
static const int REC_FIELDS = 22;
static const std::size_t REC_MAX_BYTES = 4u * 1024u * 1024u;
static const std::size_t REC_MAX_LATCHES = 200;
static const char* recordHeader() {
    return "t_first|t_decided|code|side|bias|zone_lo|zone_hi|tick|net_delta|multiple|decided_by_t|version|event|root|id|peak_t|peak_raw|price|lo_ticks|hi_ticks|node_side|source";
}
static std::string recordName(const std::string& market, int secondsPerBar, long long session) {
    return market + "-" + std::to_string(secondsPerBar) + "-signals-" + std::to_string(session) + ".csv";
}
static std::string num(double v, const char* format) { char b[64]; std::snprintf(b, sizeof(b), format, v); return b; }
static std::string recordLine(const Latch& L, char event, double tick, const std::string& root) {
    const bool decidedLine = event == 'D';
    std::string code = L.code;
    if (!decidedLine && (code.empty() || code.back() != '?')) code += '?';   // the F line is always the candidate ("A?")
    std::ostringstream o; o.imbue(std::locale::classic());
    o << (L.tFirst > 0 ? L.tFirst : 0) << '|' << (decidedLine ? L.tDecided : 0) << '|' << code << '|'
      << (L.support ? "support" : "resistance") << '|' << (L.support ? "bullish" : "bearish") << '|'
      << num(static_cast<double>(L.low) * tick, "%.10g") << '|' << num(static_cast<double>(L.high) * tick, "%.10g") << '|'
      << num(tick, "%.10g") << '|' << num(L.net, "%.0f") << '|' << num(L.mult, "%.2f") << '|' << (decidedLine ? L.decidedBy : 0) << '|'
      << DLT_VERSION << '|' << event << '|' << root << '|' << L.id << '|' << L.peakT << '|'
      << static_cast<unsigned long long>(L.peakTime) << '|' << num(L.price, "%.10g") << '|' << L.low << '|' << L.high << '|'
      << (L.sell ? "sell" : "buy") << '|' << (L.zone ? "zone" : "node");
    return o.str();
}
// strict full-token parsing (checklist #4): no leading blanks, no trailing junk, in range, finite
static bool parseWhole(const std::string& t, long long lo, long long hi, long long& out) {
    if (t.empty() || t.size() > 24 || std::isspace(static_cast<unsigned char>(t[0]))) return false;
    errno = 0; char* e = NULL; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno != 0 || e != t.c_str() + t.size() || v < lo || v > hi) return false;
    out = v; return true;
}
static bool parseReal(const std::string& t, double& out) {
    if (t.empty() || t.size() > 40 || std::isspace(static_cast<unsigned char>(t[0]))) return false;
    errno = 0; char* e = NULL; const double v = std::strtod(t.c_str(), &e);
    if (errno != 0 || e != t.c_str() + t.size() || !dp::finite(v)) return false;
    out = v; return true;
}
static std::vector<std::string> fields(const std::string& line) {
    std::vector<std::string> c; std::size_t a = 0;
    for (;;) {
        const std::size_t p = line.find('|', a);
        c.push_back(line.substr(a, p == std::string::npos ? std::string::npos : p - a));
        if (p == std::string::npos) break;
        a = p + 1;
    }
    return c;
}
// One record line -> its parts. false = a malformed line (counted, skipped).
struct RecordEvent { char event; Latch latch; std::string root; };
static bool parseRecordLine(const std::string& line, double chartTick, RecordEvent& r) {
    const std::vector<std::string> c = fields(line);
    if (static_cast<int>(c.size()) != REC_FIELDS || c[12].size() != 1 || (c[12][0] != 'F' && c[12][0] != 'D')) return false;
    Latch& L = r.latch; r.event = c[12][0]; r.root = c[13];
    if (r.root.empty()) return false;
    long long tf, td, by, id, pt, praw, lo, hi;
    const long long T_MAX = 32503680000LL;                                  // year 3000: CT-clock epoch seconds
    const unsigned long long rawMax = static_cast<unsigned long long>(std::numeric_limits<RTDATE>::max());
    const long long rawLimit = rawMax > static_cast<unsigned long long>(LLONG_MAX) ? LLONG_MAX : static_cast<long long>(rawMax);
    if (!parseWhole(c[0], 0, T_MAX, tf) || !parseWhole(c[1], 0, T_MAX, td) || !parseWhole(c[10], 0, T_MAX, by) ||
        !parseWhole(c[14], 0, 1000000, id) || !parseWhole(c[15], 0, T_MAX, pt) ||
        !parseWhole(c[16], 1, rawLimit, praw) ||
        !parseWhole(c[18], -9000000000000000LL, 9000000000000000LL, lo) || !parseWhole(c[19], -9000000000000000LL, 9000000000000000LL, hi) || lo > hi)
        return false;
    double zl, zh, tk, net, mult, price;
    if (!parseReal(c[5], zl) || !parseReal(c[6], zh) || !parseReal(c[7], tk) || !parseReal(c[8], net) || !parseReal(c[9], mult) ||
        !parseReal(c[17], price) || tk <= 0 || tk >= 1000 || mult < 0 || zl > zh) return false;
    const std::string& code = c[2];
    if (r.event == 'F' ? code != "A?" : (code != "A" && code != "I" && code != "Acc" && code != "Dst")) return false;
    if (r.event == 'D' ? (td == 0 || by == 0) : (td != 0 || by != 0)) return false;
    const bool support = c[3] == "support";
    if ((!support && c[3] != "resistance") || c[4] != (support ? "bullish" : "bearish")) return false;
    if ((c[20] != "buy" && c[20] != "sell") || (c[21] != "node" && c[21] != "zone")) return false;
    L.code = code; L.support = support; L.tFirst = tf; L.tDecided = td; L.decidedBy = by; L.id = id; L.peakT = pt;
    L.peakTime = static_cast<RTDATE>(praw); L.price = price; L.net = net; L.mult = mult; L.sell = c[20] == "sell"; L.zone = c[21] == "zone";
    L.low = lo; L.high = hi;
    if (std::fabs(tk - chartTick) > 1e-9 * std::max(tk, chartTick)) {      // written on another tick size: the prices decide the zone
        try { L.low = dp::priceKey(zl, chartTick); L.high = dp::priceKey(zh, chartTick); } catch (...) { return false; }
        if (L.low > L.high) return false;
    }
    return true;
}
// The session record text -> this root's latches in creation order. Only lines ending in a newline count; a duplicated line
// (a retried append) is ignored; the FIRST decided line of an id wins (a decided signal never changes).
static void seedFromText(const std::string& text, const std::string& root, double chartTick, LatchBook& bk) {
    const std::size_t end = text.rfind('\n');
    if (end == std::string::npos) return;
    std::map<long long, Latch> byId;
    std::istringstream in(text.substr(0, end + 1)); std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#' || line.compare(0, 8, "t_first|") == 0) continue;
        RecordEvent r;
        if (!parseRecordLine(line, chartTick, r)) { ++bk.rejected; continue; }
        if (r.root != root) continue;
        std::map<long long, Latch>::iterator at = byId.find(r.latch.id);
        if (r.event == 'F') {
            if (at != byId.end() || byId.size() >= REC_MAX_LATCHES) continue;
            r.latch.tDecided = 0; r.latch.decidedBy = 0; r.latch.logged = false; byId[r.latch.id] = r.latch; ++bk.loaded;
        } else {
            if (at == byId.end()) { ++bk.rejected; continue; }
            Latch& L = at->second;
            if (L.logged) continue;
            if (L.low != r.latch.low || L.high != r.latch.high || L.sell != r.latch.sell || L.peakTime != r.latch.peakTime) { ++bk.rejected; continue; }
            L.code = r.latch.code; L.support = r.latch.support; L.tDecided = r.latch.tDecided; L.decidedBy = r.latch.decidedBy; L.logged = true;
        }
    }
    for (std::map<long long, Latch>::const_iterator i = byId.begin(); i != byId.end(); ++i) {
        bk.v.push_back(i->second); bk.nextId = std::max(bk.nextId, i->first + 1);
    }
}
static bool readWholeFile(const std::string& path, std::string& text, bool& tooLarge) {
    tooLarge = false; text.clear();
    std::ifstream f(path.c_str(), std::ios::binary | std::ios::ate);
    if (!f.is_open()) return false;
    const std::streamoff size = static_cast<std::streamoff>(f.tellg());
    if (size < 0) return false;
    if (static_cast<unsigned long long>(size) > REC_MAX_BYTES) { tooLarge = true; return false; }
    text.resize(static_cast<std::size_t>(size)); f.seekg(0);
    if (size > 0 && !f.read(&text[0], size)) { text.clear(); return false; }
    return true;
}
// Append whole lines in ONE write: a header on a new file, a newline first if a crash left a torn last line.
static bool appendRecord(const std::string& path, const std::vector<std::string>& lines) {
    if (lines.empty()) return true;
    bool fresh = true, torn = false;
    {
        std::ifstream t(path.c_str(), std::ios::binary | std::ios::ate);
        if (t.is_open()) {
            const std::streamoff size = static_cast<std::streamoff>(t.tellg());
            if (size > 0) { fresh = false; t.seekg(-1, std::ios::end); char ch = 0; if (t.get(ch) && ch != '\n') torn = true; }
        }
    }
    std::string text;
    if (fresh) { text += "# lsDeltaProfile signals - one line per event (F first seen, D decided); times = Central clock as epoch seconds\n"; text += recordHeader(); text += '\n'; }
    else if (torn) text += '\n';
    for (std::size_t i = 0; i < lines.size(); ++i) { text += lines[i]; text += '\n'; }
    std::ofstream o(path.c_str(), std::ios::binary | std::ios::app);
    if (!o.is_open()) return false;
    o.write(text.data(), static_cast<std::streamsize>(text.size())); o.flush();
    return o.good();
}
static void makeDirectory(const std::string& path) {
#if defined(_WIN32)
    CreateDirectoryA(path.c_str(), NULL);
#else
    mkdir(path.c_str(), 0777);
#endif
}
// (2.5.3, RA1009 audit) the Globex session a bar END stamp belongs to, from the chart's local wall clock: a bar ending at or after
// 17:00 belongs to the next day's session (the same >= edge 2.5.x used). Falls back to the old epoch rule if the stamp cannot be read.
static long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long long>(doe) - 719468;
}
static std::map<std::string, LatchBook>& latchBooks() { static std::map<std::string, LatchBook> m; return m; }
struct ChartState {
    std::string identity, root, market, asof, state;
    Layout layout;
    dp::Snapshot snapshot;
    int barCount;
    RTDATE latestStamp, firstStamp;
    double tick;
    std::int64_t builtAt, statusAt, dealerAt, settingsAt, touchedAt;
    long long settingsStamp;
    int dealerReach, drawnRows, missingVap, vapMismatch;
    bool dirty, faulted, ready, shortHistory, coarseBoundary;
    ChartState() : barCount(0), latestStamp(0), firstStamp(0), tick(0), builtAt(-1),
        statusAt(-1), dealerAt(-1), settingsAt(-1), touchedAt(-1), settingsStamp(-2),
        dealerReach(0), drawnRows(0), missingVap(0), vapMismatch(0), dirty(true), faulted(false), ready(false), shortHistory(false), coarseBoundary(false) {}
};
class DeltaProfile : public cppExtension {
public:
    ~DeltaProfile() { delete fallback_; }
    int draw() override;
    int calc(int) override;
    int done() override;
    int destroy() override;
    int parmsLoad() override;
    int parmsApply() override;
    int parmsUpdt(unsigned int) override;
private:
    ChartState* fallback_ = NULL;
    ChartState& chartState();
    std::string storageDirectory();
    std::string rootSymbol();
    void syncLayout(ChartState& s, bool apply);
    int dealerWidth(ChartState& s);
    bool buildNative(ChartState& s);
    bool render(ChartState& s);
    void status(ChartState& s);
    static int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    static int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
    bool epoch(RTDATE value, std::int64_t& out);
    long long localSessionKey(RTDATE value, std::int64_t fallbackEpoch);
    long long localSec(RTDATE value);
    std::string recordDirectory();
    int yOf(int lastBar, double price);
    int textWidth(const std::string& text, int font, bool bold);
    void textRight(int right, int y, const std::string& text, COLOR color, int font, bool bold);
    void textLeft(int left, int y, const std::string& text, COLOR color, int font, bool bold);
    void stroke(int x1, int y1, int x2, int y2, COLOR color, int width);
    void ringAt(int bar, double price, COLOR color, int left, int right, int top, int bottom);
    void box(int left, int top, int right, int bottom, COLOR color);
};

int cppExtension::init() { return RTX_OK; }
int cppExtension::calc(int) { return RTX_OK; }
int cppExtension::done() { return RTX_OK; }
int cppExtension::destroy() { return RTX_OK; }
int cppExtension::setup() {
    setParameterVersion(11); // exactly the 2.4.0 three-parameter layout
    setParameterDialogHeight(3);
    setIntegerParameter("Width px", 50, 50);
    setIntegerParameter("Gap px", 0, 50, kParmAppendSameLine);
    setIntegerParameter("Font pt", 9, 50, kParmAppendSameLine);
    return RTX_OK;
}
std::string DeltaProfile::rootSymbol() {
    char buffer[64] = {0}; const char* p = getRootSymbol(buffer); return p ? safeField(p) : "";
}
ChartState& DeltaProfile::chartState() {
    // SDK persistent user-data belongs to the current host context, unlike this
    // DLL's shared cppExtension object. Destruction uses matching delete.
    ChartState* p = static_cast<ChartState*>(getUserData());
    if (!p) {
        std::unique_ptr<ChartState> fresh(new ChartState());
        setUserData(fresh.get());
        if (getUserData() == fresh.get()) p = fresh.release();
        else {                                    // (2.4.2) host keeps no per-chart slot: one shared state, as before 2.4.1
            if (!fallback_) fallback_ = new ChartState();
            p = fallback_;
        }
    }
    const std::string root = rootSymbol();
    const char* symbol = getSymbol();
    const std::string symbolText = symbol ? symbol : "";
    const std::string periodText = std::to_string(getSecondsPerBar());   // (2.5.1) bar size, not the label (it can read empty)
    const std::string id = safeField(symbolText + "|" + periodText);
    if (p->identity != id || p->root != root) {
        *p = ChartState(); p->identity = id; p->root = root; p->market = marketName(root);
    }
    return *p;
}
int DeltaProfile::calc(int) {
    ChartState* p = static_cast<ChartState*>(getUserData());
    if (p) p->dirty = true; // historical corrections/backfills invalidate the snapshot
    return RTX_OK;
}
int DeltaProfile::done() {
    ChartState* p = static_cast<ChartState*>(getUserData());
    if (p) *p = ChartState(); // symbol/periodicity changes safely release fault state
    return RTX_OK;
}
int DeltaProfile::destroy() {
    ChartState* p = static_cast<ChartState*>(getUserData());
    if (p && p != fallback_) { delete p; setUserData(NULL); }
    return RTX_OK;
}
std::string DeltaProfile::storageDirectory() {
    // (2.4.2) the same folder every other LRA plugin reads (Dealer Profile / Dealer Read find this status there)
    const char* p = std::getenv("USERPROFILE");
    if (!p || !p[0]) return "";
    std::string base(p);
    if (base.back() != '\\' && base.back() != '/') base += DIR_SEPARATOR_CHR;
    base += "InvestorRT"; base += DIR_SEPARATOR_CHR; base += "rtx"; base += DIR_SEPARATOR_CHR; base += "lsFlexLevels";
#if defined(_WIN32)
    // Existing RTX folder normally exists. This is layout/settings storage only.
    CreateDirectoryA(base.c_str(), NULL);
#endif
    base += DIR_SEPARATOR_CHR; return base;
}
void DeltaProfile::syncLayout(ChartState& s, bool apply) {
    const std::string directory = storageDirectory();
    const std::string path = directory + "DeltaProfile.settings.txt";
    if (apply) {
        Layout desired; desired.width = getIntegerValue(WIDTH_INDEX);
        desired.gap = getIntegerValue(GAP_INDEX); desired.font = getIntegerValue(FONT_INDEX);
        validateLayout(desired); s.layout = desired;
        setIntegerValue(WIDTH_INDEX, desired.width); setIntegerValue(GAP_INDEX, desired.gap);
        setIntegerValue(FONT_INDEX, desired.font);
        if (directory.empty() || s.root.empty()) return;
        // Preserve the legacy per-root layout file and its field positions. All
        // nonlayout options remain standardized; no retired list APIs are called.
        std::map<std::string, std::string> records;
        std::ifstream in(path.c_str()); std::string line;
        while (std::getline(in, line)) {
            const std::size_t at = line.find('|');
            if (at != std::string::npos) records[line.substr(0, at)] = line;
        }
        std::ostringstream row;
        row << s.root << "|0|" << desired.width << '|' << desired.gap << '|' << desired.font
            << "|1|0|0|0|0|1|0|90|3|1";
        records[s.root] = row.str();
        std::ostringstream out;
        for (std::map<std::string, std::string>::const_iterator i = records.begin(); i != records.end(); ++i)
            out << i->second << '\n';
        if (!atomicWrite(path, out.str())) s.state = "settings write failed";
        s.settingsStamp = -2; s.settingsAt = -1; return;
    }
    const std::int64_t now = steadyMs();
    if (s.settingsAt >= 0 && now - s.settingsAt < 1000) return;
    s.settingsAt = now;
    if (directory.empty()) return;
    const long long stamp = fileStamp(path);
    if (stamp == s.settingsStamp) return;
    s.settingsStamp = stamp;
    s.layout = Layout(); // no inherited layout from another root/chart
    std::ifstream in(path.c_str()); std::string line;
    while (std::getline(in, line)) {
        const std::vector<std::string> tokens = split(line, '|');
        if (tokens.size() < 5 || tokens[0] != s.root) continue;
        Layout next;
        if (integer(tokens[2], next.width) && integer(tokens[3], next.gap) && integer(tokens[4], next.font)) {
            validateLayout(next); s.layout = next;
        }
        break;
    }
}
int DeltaProfile::parmsLoad() {
    try {
        ChartState& s = chartState(); syncLayout(s, false);
        // Populate the dialog. Merely opening preferences NEVER saves a row.
        setIntegerValue(WIDTH_INDEX, s.layout.width); setIntegerValue(GAP_INDEX, s.layout.gap);
        setIntegerValue(FONT_INDEX, s.layout.font); return RTX_OK;
    } catch (...) { return RTX_FAIL; }
}
int DeltaProfile::parmsApply() {
    try { ChartState& s = chartState(); syncLayout(s, true); return RTX_OK; }
    catch (...) { return RTX_FAIL; }
}
int DeltaProfile::parmsUpdt(unsigned int) { return RTX_OK; } // wait for Apply/OK
int DeltaProfile::dealerWidth(ChartState& s) {
    const std::int64_t now = steadyMs();
    if (s.dealerAt >= 0 && now - s.dealerAt < 1000) return s.dealerReach;
    s.dealerAt = now;
    const std::string directory = storageDirectory(); if (directory.empty()) return s.dealerReach;
    std::ifstream in((directory + "DealerProfile.status.txt").c_str());
    std::string line, market; int reach = 0;
    while (std::getline(in, line)) {
        const std::vector<std::string> tokens = split(line, ',');
        if (tokens.size() < 2) continue;
        if (tokens[0] == "MARKET") market = tokens[1];
        else if (tokens[0] == "REACH") integer(tokens[1], reach);
    }
    // Shared legacy status has no chart ID. Never adopt another market's value;
    // retaining this chart's last good reach avoids jitter, but cannot disambiguate
    // two same-market Dealer Profiles with different widths (documented limit).
    if (market == s.market && reach > 0 && reach < 1200) s.dealerReach = reach;
    return s.dealerReach;
}
int DeltaProfile::readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
    // Only trivial values live in these MSVC SEH helpers (/EHsc compatible).
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!vap || !stats || bar < 0) return -2;
        std::memset(stats, 0, sizeof(*stats));
        if (vap->getBarStatistics(bar, *stats) != RTX_OK) return -2;
        if (stats->prices < 0) return -2;
        return stats->prices;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
int DeltaProfile::readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count) {
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) {
            std::memset(&rows[i], 0, sizeof(rows[i]));
            if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2;
        }
        return count; // ANY row failure rejects the whole snapshot, never a prefix
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
long long DeltaProfile::localSessionKey(RTDATE value, std::int64_t fallbackEpoch) {
    struct tm t = {};
    if (!getLocaltime(value, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31)
        return (fallbackEpoch - 17LL * 3600 + 86400LL * 4) / 86400LL;
    const long long days = civilDays(t.tm_year + 1900, static_cast<unsigned>(t.tm_mon + 1), static_cast<unsigned>(t.tm_mday));
    const int sec = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
    return days + (sec >= 17 * 3600 ? 1 : 0);
}
// (2.5.4) a bar stamp as the chart's Central wall clock encoded as epoch seconds (TapeFlow's convention); 0 = unreadable
long long DeltaProfile::localSec(RTDATE value) {
    struct tm t = {};
    if (!getLocaltime(value, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    return civilDays(t.tm_year + 1900, static_cast<unsigned>(t.tm_mon + 1), static_cast<unsigned>(t.tm_mday)) * 86400LL +
        t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
}
std::string DeltaProfile::recordDirectory() {
    const std::string base = storageDirectory();
    return base.empty() ? base : base + "DeltaProfile" + DIR_SEPARATOR_CHR;
}
bool DeltaProfile::epoch(RTDATE value, std::int64_t& out) {
    struct tm t = {};
    if (!getLocaltime(value, &t)) return false;
    t.tm_isdst = -1;
    const time_t result = std::mktime(&t);
    if (result == static_cast<time_t>(-1)) return false;
    out = static_cast<std::int64_t>(result); return true;
}
bool DeltaProfile::buildNative(ChartState& s) {
    if (s.faulted) { s.ready = false; s.state = "VAP fault: reload indicator to retry"; return false; }
    const long count = getBarCount();
    if (count < 2 || count > INT_MAX) { s.ready = false; s.state = "insufficient chart bars"; return false; }
    RTARRAYI times(barDateTime);
    if (times.count < count) { s.ready = false; s.state = "timestamp array unavailable"; return false; }
    const RTDATE latest = static_cast<RTDATE>(times[static_cast<int>(count) - 1]);
    const RTDATE first = static_cast<RTDATE>(times[0]);
    double tick = static_cast<double>(getProperty(SYM_TICKINCR));
    if (!dp::finite(tick) || tick <= 0 || tick >= 1000) tick = marketTick(s.market);   // (2.4.2) IRT's own first, the exchange's as fallback
    if (!dp::finite(tick) || tick <= 0) {
        s.ready = false; s.state = "native tick size unavailable"; return false;
    }
    const std::int64_t now = steadyMs();
    const bool changed = count != s.barCount || latest != s.latestStamp || first != s.firstStamp || tick != s.tick;
    // At most once/second per host context; new bars and tick changes rebuild now.
    // Even unchanged total volume is re-read periodically: bid/ask corrections
    // and OHLC changes do not necessarily change barVolume.
    if (!changed && s.builtAt >= 0 && now - s.builtAt < 1000) return s.ready;
    s.builtAt = now; s.barCount = static_cast<int>(count);
    s.latestStamp = latest; s.firstStamp = first; s.tick = tick; s.dirty = false;
    std::int64_t end;
    if (!epoch(latest, end)) { s.ready = false; s.state = "invalid chart timestamp"; return false; }
    const std::int64_t cutoff = end - 90 * 60;
    RTARRAYP vap(barVolumeProfile);
    RTARRAY open(barOpen), high(barHigh), low(barLow), close(barClose);
    RTARRAYI volume(barVolume);
    if (vap.count < count || open.count < count || high.count < count || low.count < count ||
        close.count < count || volume.count < count) {
        s.ready = false; s.state = "chart data arrays unavailable"; return false;
    }
    std::vector<dp::Bar> bars;
    std::vector<VOLPROFILE> rows;
    std::size_t examinedPrices = 0;
    const std::size_t MAX_WINDOW_PRICES = 2000000; // fail explicitly, not silently truncate
    bool reachedBoundary = false, coarseBar = false;
    int missing = 0, mismatched = 0;
    std::int64_t nextTime = end;
    for (int i = static_cast<int>(count) - 1; i >= 0; --i) {
        std::int64_t stamp;
        if (!epoch(static_cast<RTDATE>(times[i]), stamp)) {
            s.ready = false; s.state = "invalid chart timestamp"; return false;
        }
        if (stamp > nextTime) stamp = nextTime;   // (2.4.2) DST fall-back repeats an hour of local stamps: keep order, never fail
        if (stamp <= cutoff) { reachedBoundary = true; break; }
        nextTime = stamp;
        BARSTATISTICS stats;
        const int prices = readStatistics(&vap, i, &stats);
        if (prices == -1) { s.faulted = true; s.ready = false; s.state = "VAP fault: reload indicator to retry"; return false; }
        if (prices < 0) { if (i != static_cast<int>(count) - 1) ++missing; continue; }   // (2.4.2) a bar IRT has no VAP for yet (forming / back-filling): skip it, flag PARTIAL
        examinedPrices += static_cast<std::size_t>(prices);
        if (examinedPrices > MAX_WINDOW_PRICES) { s.ready = false; s.state = "VAP window exceeds safety budget"; return false; }
        rows.resize(static_cast<std::size_t>(prices));
        const int read = readPrices(&vap, i, rows.empty() ? NULL : &rows[0], prices);
        if (read == -1) { s.faulted = true; s.ready = false; s.state = "VAP fault: reload indicator to retry"; return false; }
        if (read < 0 || (prices == 0 && volume[i] > 0)) { if (i != static_cast<int>(count) - 1) ++missing; continue; }   // (2.4.2) skip + flag, never blank the profile
        dp::Bar bar;
        bar.index = i; bar.time = stamp; bar.closed = i < static_cast<int>(count) - 1;
        bar.open = open[i]; bar.high = high[i]; bar.low = low[i]; bar.close = close[i];
        bar.rows.reserve(rows.size());
        double total = 0, buys = 0, sells = 0;
        for (std::size_t k = 0; k < rows.size(); ++k) {
            const VOLPROFILE& v = rows[k];
            if (!dp::finite(v.price) || v.buyVolume < 0 || v.sellVolume < 0 || v.totalVolume < 0) { ++mismatched; continue; }
            const double buy = static_cast<double>(v.buyVolume), sell = static_cast<double>(v.sellVolume);
            double vol = static_cast<double>(v.totalVolume);
            if (buy + sell > vol + 0.5) { ++mismatched; vol = buy + sell; }   // (2.4.2) count it, keep the row
            total += vol; buys += buy; sells += sell;
            if (vol == 0 && buy == 0 && sell == 0) continue;
            bar.rows.push_back(dp::Row(dp::priceKey(v.price, tick), buy - sell, vol));
        }
        // Reject inconsistent statistics or snapshots read while VAP is mutating.
        // A subsequent periodic draw retries; no incomplete signal is published.
        // (2.4.2) the bar's own totals vs its rows: a mismatch is COUNTED (status VAP_MISMATCH) - IRT's statistics may
        // aggregate differently, and a forming bar moves between the two reads; neither may blank the whole profile.
        if (std::fabs(total - static_cast<double>(stats.totalVolume)) > 0.5 ||
            std::fabs(buys - static_cast<double>(stats.buyVolume)) > 0.5 ||
            std::fabs(sells - static_cast<double>(stats.sellVolume)) > 0.5) ++mismatched;
        dp::normalize(bar); bars.push_back(std::move(bar));
    }
    std::reverse(bars.begin(), bars.end());
    // Chart-bar VAP has no within-bar timestamps. A long bar crossing the window
    // edge is included whole (close-time policy), not falsely prorated to 90 min.
    const int seconds = getSecondsPerBar();
    if (seconds > 0 && !bars.empty()) coarseBar = bars.front().time - seconds < cutoff;
    s.shortHistory = !reachedBoundary || missing > 0;
    s.missingVap = missing; s.vapMismatch = mismatched;
    s.coarseBoundary = coarseBar;
    dp::Snapshot candidate = dp::build(bars, tick, !s.shortHistory && !coarseBar);
    if (candidate.buckets.empty()) { s.ready = false; s.state = "no native volume at price"; return false; }
    {   // (2.5.2, Rassul 14:37 "if the top 3 change, the signal changes") every node is TRACKED from the moment it first
        // shows as an absorption candidate: its price zone and bar are frozen, and it is decided by the chart's own bars
        // (a RED close below a buying node / a GREEN close above a selling node = A; the opposite = I), whether or not it
        // is still one of the profile's top 3. A decided result never changes. Cleared at the 17:00 session change.
        // (2.5.3, RA1009 audit) the session key from the chart's LOCAL (Central) clock. bars[].time is a true UTC epoch (mktime),
        // so "epoch - 17 h" changed day at 17:00 UTC = 12:00 CDT / 11:00 CST and wiped the record (A's and their circles) at noon.
        const long long sess = localSessionKey(static_cast<RTDATE>(times[bars.back().index]), bars.back().time);
        const int spb = getSecondsPerBar();
        LatchBook& bk = latchBooks()[s.root + "|" + std::to_string(spb)];
        if (sess != bk.session) {
            if (!bk.journal.empty() && !bk.path.empty()) appendRecord(bk.path, bk.journal);   // (2.5.4) last try for the old session's lines
            bk.v.clear(); bk.session = sess;
            bk.journal.clear(); bk.path.clear(); bk.note.clear(); bk.nextId = 0; bk.written = bk.loaded = bk.rejected = 0;
            // (2.5.4) after an IRT restart (or the first chart of the day) the session's record seeds the book: what was shown stays
            const std::string folder = recordDirectory();
            if (!folder.empty() && !s.root.empty()) {
                bk.path = folder + recordName(s.market, spb, sess);
                std::string text; bool tooLarge = false;
                if (readWholeFile(bk.path, text, tooLarge)) seedFromText(text, s.root, tick, bk);
                else if (tooLarge) { bk.note = "record too large - not read, not appended"; bk.path.clear(); }
            }
        }
        std::vector<Latch>& latches = bk.v;
        const long long nowT = localSec(static_cast<RTDATE>(times[static_cast<int>(count) - 1]));   // the chart's last bar end
        auto barEnd = [&](int index) -> long long { return index >= 0 && index < static_cast<int>(count) ? localSec(static_cast<RTDATE>(times[index])) : 0; };
        auto decided = [](const std::string& c) { return !c.empty() && c.back() != '?'; };
        auto sellNode = [](const std::string& c, bool sup) { return c[0] == 'A' && c.compare(0, 3, "Acc") != 0 ? sup : c[0] == 'I' ? !sup : c.compare(0, 3, "Dst") == 0; };
        const bool allowedNow = !s.shortHistory && !coarseBar;
        auto track = [&](dp::Tick lo, dp::Tick hi, std::string& code, bool& support, int peakBar, double price, int decidedBar, double net, double mult, bool zone) {
            if (code.empty()) return;
            const bool mySell = sellNode(code, support);
            for (std::size_t k = 0; k < latches.size(); ++k)
                if (latches[k].low <= hi && lo <= latches[k].high && latches[k].sell == mySell) {      // already tracked
                    if (!decided(latches[k].code) && decided(code) && allowedNow) { latches[k].code = code; latches[k].support = support; latches[k].decidedBy = barEnd(decidedBar); }
                    code = latches[k].code; support = latches[k].support; return;
                }
            if (code[0] != 'A' || code.compare(0, 3, "Acc") == 0 || latches.size() >= REC_MAX_LATCHES || peakBar < 0 || peakBar >= static_cast<int>(count)) return;   // only absorption candidates start a record
            Latch L; L.low = lo; L.high = hi; L.code = allowedNow ? code : (decided(code) ? code + "?" : code); L.support = support;
            L.peakTime = static_cast<RTDATE>(times[peakBar]); L.price = price; L.sell = mySell;
            L.id = bk.nextId++; L.tFirst = nowT; L.peakT = barEnd(peakBar); L.net = net; L.mult = mult; L.zone = zone;   // (2.5.4)
            if (decided(L.code)) L.decidedBy = barEnd(decidedBar);
            latches.push_back(L);
            bk.journal.push_back(recordLine(L, 'F', tick, s.root));                                 // (2.5.4) first seen
        };
        const dp::Tick tpr = candidate.ticksPerRow;
        for (std::size_t k = 0; k < candidate.nodes.size(); ++k) {
            dp::Node& n = candidate.nodes[k];
            double net = 0;
            for (std::size_t j = 0; j < candidate.buckets.size(); ++j) if (candidate.buckets[j].key == n.bucket) { net = candidate.buckets[j].delta; break; }
            track(n.bucket * tpr, n.bucket * tpr + tpr - 1, n.state.code, n.state.support, n.state.peakBar, n.price, n.state.decidedBar, net, n.ratio, false);
        }
        for (std::size_t k = 0; k < candidate.zones.size(); ++k) {
            dp::Zone& z = candidate.zones[k];
            const double mult = candidate.meanAbsDelta > 0 ? std::fabs(z.net) / candidate.meanAbsDelta : 0;   // vs the average row
            track(z.low, z.high, z.code, z.support, z.peakBar, (static_cast<double>(z.low) + static_cast<double>(z.high)) * 0.5 * tick, z.decidedBar, z.net, mult, true);
        }
        // decide the undecided ones from the chart's CLOSED bars after their absorption bar (the last bar is still forming)
        const double eps = tick * 1e-6;
        for (std::size_t k = 0; k < latches.size(); ++k) {
            Latch& L = latches[k];
            if (decided(L.code)) continue;
            int lo = 0, hi = static_cast<int>(count) - 1, at = -1;
            while (lo <= hi) { const int mid = (lo + hi) / 2; const RTDATE t = static_cast<RTDATE>(times[mid]);
                if (t == L.peakTime) { at = mid; break; } if (t < L.peakTime) lo = mid + 1; else hi = mid - 1; }
            if (at < 0) continue;
            const double zl = static_cast<double>(L.low) * tick, zh = static_cast<double>(L.high) * tick;
            for (int i = at + 1; i < static_cast<int>(count) - 1; ++i) {
                const double o = open[i], c = close[i];
                if (!dp::finite(o) || !dp::finite(c)) continue;
                const bool redBelow = c < zl - eps && c < o, greenAbove = c > zh + eps && c > o;
                if (!redBelow && !greenAbove) continue;
                // a selling node: green above = sellers absorbed (A, support); red below = sellers won (I, resistance)
                // a buying node:  red below = buyers absorbed (A, resistance); green above = buyers won (I, support)
                if (L.sell) { L.code = greenAbove ? "A" : "I"; L.support = greenAbove; }
                else        { L.code = redBelow ? "A" : "I"; L.support = !redBelow; }
                L.decidedBy = barEnd(i);                                                          // (2.5.4)
                break;
            }
        }
        // (2.5.4) a decision is recorded once; lines are marked written only after a flushed, good append (checklist #6)
        for (std::size_t k = 0; k < latches.size(); ++k) {
            Latch& L = latches[k];
            if (!decided(L.code) || L.logged) continue;
            L.tDecided = nowT; L.logged = true;
            bk.journal.push_back(recordLine(L, 'D', tick, s.root));
        }
        if (!bk.journal.empty()) {
            if (bk.path.empty()) bk.journal.clear();                                                 // no storage: nothing to keep
            else {
                makeDirectory(recordDirectory());
                if (appendRecord(bk.path, bk.journal)) { bk.written += static_cast<long long>(bk.journal.size()); bk.journal.clear(); bk.note.clear(); }
                else bk.note = "record write failed - kept, retried on the next change";
            }
        }
        // show the recorded result on any current node that is the same zone
        for (std::size_t k = 0; k < candidate.nodes.size(); ++k) {
            dp::Node& n = candidate.nodes[k];
            for (std::size_t j = 0; j < latches.size(); ++j) {
                const dp::Tick lo = n.bucket * tpr, hi = n.bucket * tpr + tpr - 1;
                if (latches[j].low <= hi && lo <= latches[j].high && !n.state.code.empty() && latches[j].sell == sellNode(n.state.code, n.state.support)) {
                    n.state.code = latches[j].code; n.state.support = latches[j].support; break;
                }
            }
        }
    }
    s.snapshot = std::move(candidate); s.ready = true;
    s.state = s.shortHistory ? "short chart history: provisional signals" : coarseBar ?
        "boundary bar crosses window: provisional signals" : "ready";
    struct tm t = {};
    if (getLocaltime(latest, &t)) {
        char b[32]; std::strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S", &t); s.asof = b;
    }
    return true;
}
static short coordinate(int v) { return static_cast<short>(std::max<int>(SHRT_MIN, std::min<int>(SHRT_MAX, v))); }
int DeltaProfile::yOf(int lastBar, double price) {
    PNT point; point.set(lastBar, static_cast<float>(price)); return point.v;
}
int DeltaProfile::textWidth(const std::string& text, int font, bool bold) {
    FONT f; f.id = HELVETICA; f.size = coordinate(font); f.style = bold ? BOLD : PLAIN; setFont(f);
    return getTextWidth(text.c_str(), -1);
}
void DeltaProfile::textRight(int right, int y, const std::string& text, COLOR color, int font, bool bold) {
    if (text.empty()) return;
    FONT f; f.id = HELVETICA; f.size = coordinate(font); f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(color);
    const int yy = y - static_cast<int>(font * 0.45 + 0.5);
    RCT rect; rect.set(coordinate(right - 400), coordinate(yy - font), coordinate(right), coordinate(yy + font));
    rect.drawText(text.c_str(), false, true);
}
void DeltaProfile::textLeft(int left, int y, const std::string& text, COLOR color, int font, bool bold) {
    if (text.empty()) return;
    FONT f; f.id = HELVETICA; f.size = coordinate(font); f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(color);
    const int yy = y - static_cast<int>(font * 0.45 + 0.5);
    RCT rect; rect.set(coordinate(left), coordinate(yy - font), coordinate(left + 400), coordinate(yy + font));
    rect.drawText(text.c_str(), false, false);
}
void DeltaProfile::stroke(int x1, int y1, int x2, int y2, COLOR color, int width) {
    setPen(color, coordinate(width), P_SOLID);
    PNT a; a.set(0, 0.0f); a.h = coordinate(x1); a.v = coordinate(y1); a.setDrawPosition();
    PNT b; b.set(0, 0.0f); b.h = coordinate(x2); b.v = coordinate(y2); b.drawLineTo();
}
// (2.4.3) the absorption circle: on the bar where the node's delta peaked, at the node's price
void DeltaProfile::ringAt(int bar, double price, COLOR color, int left, int right, int top, int bottom) {
    PNT point; point.set(bar, static_cast<float>(price), kBarCenter);
    if (point.h - 6 < left || point.h + 6 > right || point.v - 6 < top || point.v + 6 > bottom) return;
    setPen(color, 2, P_SOLID); CBRUSH brush(color, PAT_HOLLOW); brush.set();
    RCT ring; ring.set(coordinate(point.h - 6), coordinate(point.v - 6), coordinate(point.h + 6), coordinate(point.v + 6));
    ring.drawOval(DRAW_OPAQUE);
}
void DeltaProfile::box(int left, int top, int right, int bottom, COLOR color) {
    if (right <= left || bottom <= top) return;
    RCT rect; rect.set(coordinate(left), coordinate(top), coordinate(right), coordinate(bottom));
    rect.draw(0, color, color, DRAW_OPAQUE, PAT_SOLID);
}
static std::string amount(double d) {
    std::ostringstream out; out << (d > 0 ? '+' : '-');
    if (std::fabs(d) >= 1000) out << std::fixed << std::setprecision(1) << std::fabs(d) / 1000 << 'k';
    else out << std::fixed << std::setprecision(0) << std::fabs(d);
    return out.str();
}
static std::string ratio(double x) {
    std::ostringstream out; out << std::fixed << std::setprecision(x < 10 ? 1 : 0) << x;
    std::string s = out.str();
    if (s.size() > 2 && s.substr(s.size() - 2) == ".0") s.resize(s.size() - 2);
    return s + 'x';
}
bool DeltaProfile::render(ChartState& s) {
    RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
    int paneRight = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) paneRight = scale.left - 2;
    const int base = paneRight - 2 - dealerWidth(s) - s.layout.gap;
    const int left = base - s.layout.width;
    const int top = pane.top + 16, bottom = pane.bottom;
    if (left <= pane.left + 40 || base >= paneRight || bottom <= top) {
        s.state = "not drawn: insufficient pane room or overlapping gap"; return false;
    }
    textLeft(left + 2, pane.top + 8, "DLT", C_MUTED, 8, true);
    if (!s.ready) { textRight(base - 2, top + 12, "NO VAP", C_SELL, 8, true); return false; }
    if (s.shortHistory || s.state.find("provisional") != std::string::npos)
        textRight(base - 2, top + 12, "PARTIAL", C_MUTED, 8, true);
    stroke(base, top, base, bottom, C_AXIS, 1);
    const dp::Snapshot& profile = s.snapshot;
    const int lastBar = s.barCount - 1, full = s.layout.width - 2;
    std::vector<std::size_t> ranked;
    for (std::size_t i = 0; i < profile.buckets.size(); ++i) ranked.push_back(i);
    std::sort(ranked.begin(), ranked.end(), [&profile](std::size_t a, std::size_t b) {
        const double x = std::fabs(profile.buckets[a].delta), y = std::fabs(profile.buckets[b].delta);
        return x != y ? x > y : profile.buckets[a].key < profile.buckets[b].key;
    });
    std::map<dp::Tick, int> edge;
    struct VisibleNode { const dp::Node* node; int y, edge; };
    std::vector<VisibleNode> nodes;
    s.drawnRows = 0;
    for (std::size_t i = 0; i < profile.buckets.size(); ++i) {
        const dp::Bucket& b = profile.buckets[i];
        const double lower = (static_cast<double>(b.key) * static_cast<double>(profile.ticksPerRow) - 0.5) * profile.tick;
        const double upper = (static_cast<double>(b.key + 1) * static_cast<double>(profile.ticksPerRow) - 0.5) * profile.tick;
        int yt = yOf(lastBar, upper), yb = yOf(lastBar, lower);
        if (yb <= yt) yb = yt + 1;
        if (yb <= top || yt >= bottom) continue;
        yt = std::max(top, yt); yb = std::min(bottom, yb);
        ++s.drawnRows;
        const int dl = profile.maxAbsDelta > 0 ? std::min(full, std::max(b.delta != 0 ? 1 : 0,
            static_cast<int>(std::fabs(b.delta) / profile.maxAbsDelta * full))) : 0;
        const int vl = profile.maxVolume > 0 ? std::min(full, static_cast<int>(b.volume / profile.maxVolume * full)) : 0;
        box(base - vl, yt, base, yb, C_VOL);
        if (b.delta != 0) box(base - dl, yt, base, yb, b.delta > 0 ? C_BUY : C_SELL);
        int e = base - dl;
        const dp::Node* node = NULL;
        for (std::size_t j = 0; j < profile.nodes.size(); ++j) if (profile.nodes[j].bucket == b.key) node = &profile.nodes[j];
        bool showAmount = node != NULL;
        for (std::size_t j = 0; j < ranked.size() && j < 3; ++j) if (ranked[j] == i) showAmount = true;
        if (showAmount && b.delta != 0) {
            const std::string label = amount(b.delta);
            const int width = textWidth(label, s.layout.font - 1, false);
            if (e - 2 - width >= pane.left + 2) {
                textRight(e - 2, (yt + yb) / 2, label, C_INK, s.layout.font - 1, false);
                e -= width + 2;
            }
        }
        edge[b.key] = e;
        if (node) { VisibleNode v; v.node = node; v.y = (yt + yb) / 2; v.edge = e; nodes.push_back(v); }
    }
    // Native key/bucket lookup only. Pixel Y NEVER assigns a node to a bucket.
    std::sort(nodes.begin(), nodes.end(), [](const VisibleNode& a, const VisibleNode& b) { return a.y < b.y; });
    std::vector<int> used;
    std::vector<int> ringed;   // (2.5.0) bars already circled this pass
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const VisibleNode& v = nodes[i]; const dp::Node& n = *v.node;
        // (2.4.3, Rassul 2026-10-09 08:24 "are you making sure that you draw circles wherever there is absorption on the irt chart")
        // the circle goes on the bar where the absorption happened for EVERY absorbing node, even when its letter does not fit
        if ((n.state.code == "A" || n.state.code == "A?") && n.state.peakBar >= 0 && n.state.peakBar < s.barCount)
            ringed.push_back(n.state.peakBar), ringAt(n.state.peakBar, n.price, n.state.support ? C_SUPPORT : C_RESISTANCE, pane.left, paneRight, top, bottom);
        const bool absorbing = n.state.code == "A" || n.state.code == "A?";
        std::string label = n.state.code + " " + ratio(n.ratio);
        int width = textWidth(label, s.layout.font, true);
        if (v.edge - 4 - width < pane.left + 2 && absorbing) { label = n.state.code; width = textWidth(label, s.layout.font, true); }   // (2.4.3) the letter always shows with its circle
        if (v.edge - 4 - width < pane.left + 2) continue;
        int y = v.y;
        if (!used.empty() && y - used.back() < s.layout.font + 2) y = used.back() + s.layout.font + 2;
        if (y + s.layout.font / 2 >= bottom || y - s.layout.font / 2 <= top) { if (!absorbing) continue; y = std::max(top + s.layout.font / 2 + 1, std::min(bottom - s.layout.font / 2 - 1, v.y)); }
        const COLOR color = n.state.support ? C_SUPPORT : C_RESISTANCE;
        textRight(v.edge - 4, y, label, color, s.layout.font, true); used.push_back(y);
        if (y != v.y) stroke(v.edge - 2, v.y, v.edge - 2, y, color, 1); // visible association when labels stack
    }
    // Zones supplement, rather than suppress, node letters. Avoid both overlap
    // and invisible bucket defaults by considering ONLY rendered bucket edges.
    for (std::size_t i = 0; i < profile.zones.size(); ++i) {
        const dp::Zone& z = profile.zones[i];
        if ((z.code == "A" || z.code == "A?") && z.peakBar >= 0 && z.peakBar < s.barCount)       // (2.4.3) zone absorption gets its circle too
            ringed.push_back(z.peakBar), ringAt(z.peakBar, (static_cast<double>(z.low) + static_cast<double>(z.high)) * 0.5 * profile.tick, z.support ? C_SUPPORT : C_RESISTANCE,
                   pane.left, paneRight, top, bottom);
        if (z.code != "A" && z.code != "A?") continue;   // (2.5.0, Rassul 13:59 "remove the accumulation and distribution ... only keep absorption")
        int yt = yOf(lastBar, (static_cast<double>(z.high) + 0.5) * profile.tick);
        int yb = yOf(lastBar, (static_cast<double>(z.low) - 0.5) * profile.tick);
        if (yb <= top || yt >= bottom) continue;
        yt = std::max(top, yt); yb = std::min(bottom, yb); if (yb <= yt) continue;
        int e = base; bool any = false;
        for (std::size_t j = 0; j < profile.buckets.size(); ++j) {
            const dp::Bucket& b = profile.buckets[j];
            if (b.high < z.low || b.low > z.high) continue;
            const std::map<dp::Tick, int>::const_iterator at = edge.find(b.key);
            if (at != edge.end()) { e = any ? std::min(e, at->second) : at->second; any = true; }
        }
        if (!any) continue;
        std::ostringstream out; out << z.code;
        if (z.code != "A" && z.code != "A?") out << ' ' << z.share << '%';
        const std::string label = out.str(); const int width = textWidth(label, s.layout.font, true);
        int y = (yt + yb) / 2;
        bool collision = false;
        for (std::size_t j = 0; j < used.size(); ++j) if (std::abs(y - used[j]) < s.layout.font + 2) collision = true;
        if (collision && (z.code == "A" || z.code == "A?")) {          // (2.4.3) an absorption zone always keeps its letter (it has a circle): move it clear
            for (int tries = 0; tries < 6 && collision; ++tries) {
                y += (tries % 2 ? -1 : 1) * (tries / 2 + 1) * (s.layout.font + 2);
                collision = false;
                for (std::size_t j = 0; j < used.size(); ++j) if (std::abs(y - used[j]) < s.layout.font + 2) collision = true;
            }
            collision = false;
        }
        if (collision || e - 7 - width < pane.left + 2) continue;
        const COLOR color = z.support ? C_SUPPORT : C_RESISTANCE;
        stroke(e - 4, yt, e - 4, yb, color, 1); stroke(e - 4, yt, e - 1, yt, color, 1);
        stroke(e - 4, yb, e - 1, yb, color, 1);
        textRight(e - 7, y, label, color, s.layout.font, true); used.push_back(y);
    }
    // (2.5.0) a recorded absorption keeps its circle for the session, even after its node left the sliding profile
    {
        const std::vector<Latch>& latches = latchBooks()[s.root + "|" + std::to_string(getSecondsPerBar())].v;
        RTARRAYI times(barDateTime);
        for (std::size_t k = 0; k < latches.size(); ++k) {
            const Latch& L = latches[k];
            if ((L.code != "A" && L.code != "A?") || L.peakTime == 0 || times.count < s.barCount || s.barCount < 1) continue;
            int lo = 0, hi = s.barCount - 1, at = -1;                   // the bar with this time (times ascend)
            while (lo <= hi) { const int mid = (lo + hi) / 2; const RTDATE t = static_cast<RTDATE>(times[mid]);
                if (t == L.peakTime) { at = mid; break; } if (t < L.peakTime) lo = mid + 1; else hi = mid - 1; }
            if (at < 0 || std::find(ringed.begin(), ringed.end(), at) != ringed.end()) continue;
            ringed.push_back(at);
            ringAt(at, L.price, L.support ? C_SUPPORT : C_RESISTANCE, pane.left, paneRight, top, bottom);
        }
        // (2.5.2) a tracked signal that is no longer one of the profile's top 3 keeps its letter, just left of the profile
        std::vector<int> offUsed;
        for (std::size_t k = 0; k < latches.size(); ++k) {
            const Latch& L = latches[k];
            bool onNode = false;
            for (std::size_t j = 0; j < profile.nodes.size(); ++j) {
                const dp::Tick nl = profile.nodes[j].bucket * profile.ticksPerRow, nh = nl + profile.ticksPerRow - 1;
                if (L.low <= nh && nl <= L.high) { onNode = true; break; }
            }
            if (onNode) continue;
            const int y = yOf(lastBar, L.price);
            if (y <= top + s.layout.font / 2 || y >= bottom - s.layout.font / 2) continue;
            bool clash = false; for (std::size_t j = 0; j < offUsed.size(); ++j) if (std::abs(offUsed[j] - y) < s.layout.font + 2) clash = true;
            if (clash) continue;
            textRight(left - 4, y, L.code, L.support ? C_SUPPORT : C_RESISTANCE, s.layout.font, true);
            offUsed.push_back(y);
        }
    }
    return true;
}
void DeltaProfile::status(ChartState& s) {
    const std::int64_t now = steadyMs();
    if (s.statusAt >= 0 && now - s.statusAt < 1000) return;
    s.statusAt = now;
    const std::string directory = storageDirectory(); if (directory.empty()) return;
    std::ostringstream out;
    out << "VERSION," << DLT_VERSION << "\nROOT," << s.root << "\nMARKET," << s.market
        << "\nCHART," << s.identity << "\nROWS," << (s.ready ? s.snapshot.buckets.size() : 0)
        << "\nASOF," << s.asof << "\nRANGE,Last 90 min LIVE (chart-bar close window)\nDRAWN_ROWS," << s.drawnRows
        << "\nBADGE,\nWIDTH," << s.layout.width << "\nSIDES,One\nFACE,Left\nPLACE,Right\nSTATE," << safeField(s.state)
        << "\nTICK," << std::setprecision(12) << s.tick << "\nMISSING_VAP_BARS," << s.missingVap
        << "\nVAP_MISMATCH," << s.vapMismatch << '\n';
    {   // (2.5.0) the session record: what was decided, where (for ChartView and the nightly scoring)
        const LatchBook& bk = latchBooks()[s.root + "|" + std::to_string(getSecondsPerBar())];
        const std::vector<Latch>& latches = bk.v;
        out << "RECORD," << safeField(bk.path) << ",session " << bk.session << ",loaded " << bk.loaded << ",written " << bk.written
            << ",pending " << bk.journal.size() << ",rejected " << bk.rejected << ',' << safeField(bk.note) << '\n';   // (2.5.4)
        for (std::size_t k = 0; k < latches.size(); ++k)
            out << "SIGNAL," << latches[k].code << "," << (latches[k].support ? "support" : "resistance") << "," << latches[k].low * s.tick << "," << latches[k].high * s.tick << ",t " << latches[k].peakTime << '\n';
    }
    atomicWrite(directory + "DeltaProfile.status.txt", out.str());
}
int DeltaProfile::draw() {
    ChartState* current = NULL;
    try {
        ChartState& s = chartState(); current = &s; s.drawnRows = 0;
        syncLayout(s, false);
        buildNative(s);
        if (s.ready) s.state = s.shortHistory ? "short chart history: provisional signals" :
            s.coarseBoundary ? "boundary bar crosses window: provisional signals" : "ready";
        const bool drawn = render(s);
        if (drawn && s.state == "ready") s.state = "drawn";
        status(s); return RTX_OK;
    } catch (const std::exception& e) {
        if (current) {
            current->ready = false; current->drawnRows = 0;
            try { current->state = std::string("calculation error: ") + e.what(); status(*current); } catch (...) {}
        }
        return RTX_FAIL;
    } catch (...) {
        if (current) { current->ready = false; current->drawnRows = 0; }
        return RTX_FAIL;
    }
}
extern "C" cppExtension* CreateExtension() {
    DeltaProfile* p = new DeltaProfile();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | VAP_REQUIRED);
    p->setDescription("LRA Delta Profile (DLT)"); p->setVersion(DLT_VERSION);
    return p;
}
