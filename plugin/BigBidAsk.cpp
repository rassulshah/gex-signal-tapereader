/********************************************************************************
 *  BigBidAsk.cpp  --  Investor/RT RTX extension  lsBigBidAsk  1.0.0  (2026-10-14 night)
 *
 *  BIGGEST BID / ASK ON EVERY CANDLE. Rassul approved the mockup at 20:41 CT 2026-10-14 (mockup-biggest-bid-ask.png v4; the thin
 *  absorption circle as drawn in v5). Put ONE on the PRICE pane of each chart.
 *    - RED number LEFT of each candle, at the price where the most lots traded AT THE BID in that candle (sellers hitting); the
 *      number = that price's bid lots. GREEN number RIGHT of the candle, at the price where the most lots traded AT THE ASK (buyers
 *      lifting). Ties go to the price nearest the close. No dash or tick mark, just the number (Font size, default 11).
 *    - BOLD = at least "Bold at" x (default 2.0x) the median of the same side's biggest over the prior 20 candles (past only).
 *    - ABSORPTION = a thin 1 px rounded outline round the number, in its own colour, decided once at the candle's close and never
 *      repainted: bullish = bold red at / below the halfway mark and the candle closes green above it; bearish mirror. The forming
 *      candle never gets one.
 *    - Hover a number: "Biggest bid 7,741.25: 410 lots sold at the bid (3.1x normal) . biggest ask 7,743.50: 160 lots (1.2x)" and
 *      what it answers / why / how to use it / an example; a circled number explains the absorption rule.
 *    - No overlap: own row, one row up, one row down, never on a candle, another number, the left Liquidity column or the right-side
 *      columns (ColumnBoard.h - read only, this plugin publishes no column); no room = not drawn, except bold ones.
 *  DATA: the chart's own volume at price per bar (barVolumeProfile, VAP_REQUIRED - the same source as lsDeltaProfile): per tick,
 *  never bucketed; bid lots = VOLPROFILE.sellVolume, ask lots = buyVolume; whole-tick keys (llround(price / SYM_TICKINCR)).
 *  NEVER REPAINTS: each closed candle's facts (prices, lots, ratios) are frozen at its close and recorded to
 *  lsFlexLevels\BigBidAsk\<MKT>-<CONTRACT>-<sec>-<session>.csv; a restart redraws from the record. Rules: BigBidAskLogic.h (tested).
 *  Settings (controls only, per market, lsFlexLevels\BigBidAsk.settings-<MKT>.txt, written only from OK / Apply): Font size,
 *  Bold at (x normal), Absorption circle, Show non-bold numbers. Status: lsFlexLevels\BigBidAsk.status-<MKT>-<CONTRACT>-<sec>.txt.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "BigBidAskLogic.h"
#include "ColumnBoard.h"                 // where the price area ends (the right-side columns) and the left Liquidity column - read only
#include "HoverBoard.h"                  // hb::hbCursor (GetCursorPos + WindowFromPoint; carries the user32 pragma)
#include "BoxPlace.h"                    // bp::DlgSnap (a control he did not touch keeps the file's value)
#include "HostSlot.h"
#include "ZoneNear.h"              // (1.2.0) location first: bold / circles away from a zone are dimmed
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")      // HoverBoard.h's GetCursorPos / WindowFromPoint
#endif
#include <algorithm>
#include <cctype>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#if !defined(_WIN32)
#include <unistd.h>
#endif

#define BBA_VERSION "1.2.0"          // keep equal to bba::VERSION and the setVersion literal below

#if defined(_WIN32)
#define BBA_SEP "\\"
#else
#define BBA_SEP "/"
#endif

static const COLOR C_BUY = 0x0022C55E, C_SELL = 0x00EF4444;
static const COLOR C_BUY_DIM = 0x00157A3D, C_SELL_DIM = 0x00933030;   // (1.1.0, Rassul 10-15 "like the 96") normal-size numbers dim, the big ones full colour           // buyers green, sellers red (the Delta Profile's colours)
static const COLOR C_HOVBG = 0x00111827, C_HOVEDGE = 0x009CA3AF, C_INK = 0x00F3F4F6;
static const int LEAD = 30;                 // candles laid out left of the view, so a scroll does not re-decide what stays in view
static const int READS_MAX = 900;           // VAP bars read per pass (the rest next pass)
static const int FAILS_FINAL = 5;           // a closed candle IRT has no VAP for after 5 passes is finalised as "no data"
static const size_t REC_FILE_MAX = 2 * 1024 * 1024, REC_LINES_MAX = 20000;
static const int P_FONT = 0, P_BOLD = 1, P_CIRCLE = 2, P_NONBOLD = 3;

static long long steadyMs() {
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
static std::string flexDir() {
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + BBA_SEP "InvestorRT" BBA_SEP "rtx" BBA_SEP "lsFlexLevels";
}
static void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
// the file's stamp: last-write time to 100 ns / 1 ns + size (a second write within the same second still changes it)
static bool statFile(const std::string& p, long long& stamp) {
#if defined(_WIN32)
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExA(p.c_str(), GetFileExInfoStandard, &a) || (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) return false;
    const unsigned long long w = ((unsigned long long)a.ftLastWriteTime.dwHighDateTime << 32) | a.ftLastWriteTime.dwLowDateTime;
    stamp = (long long)(w & 0x3FFFFFFFFFFFULL) * 1009LL + (long long)a.nFileSizeLow; return true;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
    if ((st.st_mode & S_IFMT) != S_IFREG) return false;
    stamp = ((long long)st.st_mtim.tv_sec * 1000000000LL + (long long)st.st_mtim.tv_nsec) % 4000000000000000LL * 1009LL / 1000 + (long long)st.st_size; return true;
#endif
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
    if (!moveOver(tmp, path)) { std::remove(tmp.c_str()); return false; }
    return true;
}
static bool readSmall(const std::string& path, size_t cap, std::string& out) {
    out.clear();
    std::ifstream in(path.c_str(), std::ios::binary); if (!in) return false;
    in.seekg(0, std::ios::end); const std::streamoff size = in.tellg();
    if (size < 0 || size > (std::streamoff)cap) return false;
    in.seekg(0, std::ios::beg); out.resize((size_t)size);
    if (size > 0) in.read(&out[0], size);
    return (bool)in || in.eof();
}
static std::string fileKey(const std::string& s) {
    std::string out;
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
    return fileKey(r.substr(0, 8));
}
static long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
// checklist #24: the chart's SYM_TICKINCR, made exact (it arrives as a float: 0.00005f = 4.99999987e-05)
static double cleanTick(float raw) {
    const double d = static_cast<double>(raw);
    if (!std::isfinite(d) || d <= 0 || d >= 1000) return 0;
    char b[32]; std::snprintf(b, sizeof(b), "%.6g", d);
    const double t = std::strtod(b, NULL);
    return std::isfinite(t) && t > 0 && t < 1000 ? t : 0;
}
static bool keyOf(double price, double tick, long long& key) {
    if (!std::isfinite(price) || tick <= 0) return false;
    const double q = price / tick;
    if (!std::isfinite(q) || std::fabs(q) >= 9.0e15) return false;
    key = (long long)std::llround(q); return true;
}

struct HoverT { bba::Box box; long long t = 0; int side = -1; };
struct BBState {
    bool busy = false;
    std::string identity, market, symbol, state = "starting", lastStatusText, settingsWhy = "missing (defaults)", chartKey;
    int spb = 0; long long session = -1; double tick = 0;
    bba::Settings set; long long setStamp = -2, setCheckMs = -1;
    std::map<long long, bba::Facts> fin;            // closed candles, final (from the record or decided here) - never changed
    std::map<long long, bba::Facts> raw;            // closed candles' prices / lots read from the chart (ratios not yet final)
    struct Fail { int n = 0; long long firstMs = 0; };
    std::map<long long, Fail> fails;                // closed candles with no VAP yet: passes failed, since when
    std::set<long long> recorded;                   // times already in the record
    std::vector<std::pair<long long, bba::Facts>> pending;   // (session, facts) to append
    bba::Facts forming; bool haveForming = false;
    long lastCount = -1; long long lastFirst = 0, lastLast = 0, builtMs = -1;
    long long missing = 0, mismatch = 0, recFails = 0, faults = 0, decided = 0; bool faulted = false;
    long long lastStatusMs = -1, lastRecordMs = -1;
    // draw
    cb::Peers peers; int priceRight = 0, leftLimit = 0; bb::Beat trBeat; bool trOn = false; bba::Box trBox;
    bba::LayoutStats lay; int circles = 0, circlesHidden = 0, zoneFull = 0, zoneDim = 0;
    std::vector<HoverT> targets; bool msHas = false; int msH = 0, msV = 0; unsigned long long msWnd = 0; std::string hovKey;
};

class BigBidAsk : public cppExtension {
public:
    BigBidAsk() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int parmsLoad(void);
    virtual int parmsApply(void) { saveSettings(); return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { saveSettings(); return RTX_OK; }
    void pump(bool fromDraw);
    void release() { BBState* S = slot_.get(this, false); if (S && !S->busy) { try { S->lastRecordMs = -1; flush(*S); } catch (...) {} } slot_.release(this); }   // what is decided is written before the state goes
    BBState* testState() { return slot_.get(this, false); }
private:
    HostSlot<BBState> slot_;
    bp::DlgSnap snap_;
    long long ctSec(RTDATE d);
    long long sessionOf(long long barTime) { const long long cs = ctSec((RTDATE)barTime); return cs > 0 ? (cs - 17LL * 3600) / 86400LL + 1 : -1; }
    std::string clockOf(long long t);
    void identify(BBState& S);
    void settings(BBState& S, long long now);
    std::string market();
    std::string settingsPath(const std::string& mkt) { const std::string f = flexDir(); return f.empty() ? f : f + BBA_SEP "BigBidAsk.settings-" + mkt + ".txt"; }
    bba::Settings readSettingsFile(const std::string& mkt, std::string& why);
    std::vector<long long> dialogNow();
    void saveSettings();
    int readStats(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    int readRows(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
    bool readBar(BBState& S, RTARRAYP& vap, int i, long long t, bool closed, float o, float h, float l, float c, bba::Facts& F);
    void compute(BBState& S, bool fromDraw);
    void load(BBState& S);
    bool flush(BBState& S);
    void status(BBState& S, long long now);
    void render(BBState& S);
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + BBA_SEP "BigBidAsk"; }
    std::string stem(BBState& S) { return S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb); }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<BigBidAsk*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<BigBidAsk*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) {
    // controls only (his rule); the chart reads the per-market file, the dialog is filled from it (BUILD-LESSONS #42 / #44)
    setParameterVersion(1);
    setParameterDialogHeight(3);
    setIntegerParameter("Font size", 11, 50);                                       // 0
    setFloatParameter("Bold at (x normal)", 2.0f, 50, kParmAppendSameLine);        // 1
    setBoolParameter("Absorption circle", true);                                    // 2
    setBoolParameter("Show non-bold numbers", true, kParmAppendSameLine);           // 3
    return RTX_OK;
}
int cppExtension::calc(int) { static_cast<BigBidAsk*>(this)->pump(false); return RTX_OK; }
int BigBidAsk::draw(void) {
    pump(true);
    BBState* S = slot_.get(this, false);
    if (S) { try { render(*S); } catch (...) { S->state = "draw error"; } }
    return RTX_OK;
}
long long BigBidAsk::ctSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    return civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
}
std::string BigBidAsk::clockOf(long long t) {
    struct tm m; std::memset(&m, 0, sizeof(m));
    if (t <= 0 || !getLocaltime((RTDATE)t, &m)) return "?";
    return bba::clock12(m.tm_hour, m.tm_min);
}
std::string BigBidAsk::market() {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    return marketName(root.empty() ? sym : root);
}
void BigBidAsk::identify(BBState& S) {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    const std::string id = root + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity) return;
    BBState fresh; fresh.identity = id; fresh.symbol = sym; fresh.spb = spb; fresh.market = marketName(root.empty() ? sym : root);
    fresh.msHas = S.msHas; fresh.msH = S.msH; fresh.msV = S.msV; fresh.msWnd = S.msWnd; fresh.busy = S.busy;
    S = fresh;                                                       // nothing of another symbol / bar size is kept
}
// ---- settings: the chart reads the file only (IRT hands the chart copy every box unchecked) ---------------------------------------
bba::Settings BigBidAsk::readSettingsFile(const std::string& mkt, std::string& why) {
    const std::string p = settingsPath(mkt);
    long long st = -1;
    if (p.empty() || !statFile(p, st)) { why = "missing (defaults)"; return bba::Settings(); }
    std::string text;
    if (!readSmall(p, 4096, text)) { why = "unreadable or too big (defaults)"; return bba::Settings(); }
    bba::Settings s = bba::parseSettings(text, why);
    if (why.empty()) why = "file";
    return s;
}
void BigBidAsk::settings(BBState& S, long long now) {
    if (S.setCheckMs >= 0 && now >= S.setCheckMs && now - S.setCheckMs < 1000) return;
    S.setCheckMs = now;
    const std::string p = settingsPath(S.market);
    long long st = -1;
    if (p.empty() || !statFile(p, st)) { if (S.setStamp != -1) { S.set = bba::Settings(); S.settingsWhy = "missing (defaults)"; S.setStamp = -1; } return; }
    if (st == S.setStamp) return;
    std::string why; const bba::Settings s = readSettingsFile(S.market, why);
    long long st2 = -1; if (!statFile(p, st2) || st2 != st) return;                // torn: next check
    S.set = s; S.settingsWhy = why; S.setStamp = st;
}
static bool inDialog(cppExtension* x) { const char* pl = x->getPeriodicityLabel(); return !(pl && pl[0]); }   // the dialog's copy has no bar size
std::vector<long long> BigBidAsk::dialogNow() {
    std::vector<long long> v;
    v.push_back(getIntegerValue(P_FONT)); v.push_back(bba::toQ((double)getFloatValue(P_BOLD)));
    v.push_back(isBoxChecked(P_CIRCLE) ? 1 : 0); v.push_back(isBoxChecked(P_NONBOLD) ? 1 : 0);
    return v;
}
int BigBidAsk::parmsLoad(void) {
    try {
        if (!inDialog(this)) return RTX_OK;
        const std::string mkt = market(); if (mkt.empty()) return RTX_OK;
        std::string why; const bba::Settings s = readSettingsFile(mkt, why);   // opening never saves
        setIntegerValue(P_FONT, s.font); setFloatValue(P_BOLD, (float)(s.boldQ / 100.0));
        checkBox(P_CIRCLE, s.circle); checkBox(P_NONBOLD, s.nonBold);
        snap_.take(dialogNow());
    } catch (...) {}
    return RTX_OK;
}
void BigBidAsk::saveSettings() {
    try {
        if (!inDialog(this)) return;                                  // never from the chart copy
        const std::string mkt = market(); if (mkt.empty()) return;
        const std::string p = settingsPath(mkt); if (p.empty()) return;
        std::string why; const bba::Settings F = readSettingsFile(mkt, why);
        const std::vector<long long> now = dialogNow();
        const long long fv[4] = { F.font, F.boldQ, F.circle ? 1 : 0, F.nonBold ? 1 : 0 };
        long long v[4]; for (size_t k = 0; k < 4; ++k) v[k] = snap_.pick(k, now[k], fv[k]);   // a control he did not touch keeps the file's value
        bba::Settings s = F;
        s.font = bba::fontOk(v[0]) ? (int)v[0] : F.font;                          // garbage never reaches the file
        s.boldQ = bba::boldOk(v[1]) ? (int)v[1] : F.boldQ;
        s.circle = v[2] != 0; s.nonBold = v[3] != 0;
        makeDir(flexDir());
        if (publishFile(p, bba::formatSettings(s) + "\n")) snap_.take(now);
    } catch (...) {}
}
// ---- the chart's volume at price (only trivial values live in these MSVC SEH helpers) ---------------------------------------------
int BigBidAsk::readStats(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || !stats || bar < 0) return -2;
        std::memset(stats, 0, sizeof(*stats));
        if (vap->getBarStatistics(bar, *stats) != RTX_OK) return -2;
        if (stats->prices < 0) return -2;
        return stats->prices;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
int BigBidAsk::readRows(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) {
            std::memset(&rows[i], 0, sizeof(rows[i]));
            if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2;
        }
        return count;                                                  // any row failure rejects the whole bar, never a prefix
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// one bar's facts from the chart; false = no usable VAP for it yet (F.valid false)
bool BigBidAsk::readBar(BBState& S, RTARRAYP& vap, int i, long long t, bool closed, float o, float h, float l, float c, bba::Facts& F) {
    long long ko = 0, kh = 0, kl = 0, kc = 0;
    F = bba::Facts(); F.t = t; F.closed = closed;
    if (!keyOf(o, S.tick, ko) || !keyOf(h, S.tick, kh) || !keyOf(l, S.tick, kl) || !keyOf(c, S.tick, kc)) return false;
    BARSTATISTICS st;
    const int n = readStats(&vap, i, &st);
    if (n == -1) { S.faulted = true; ++S.faults; return false; }
    if (n <= 0) return false;
    std::vector<VOLPROFILE> rows((size_t)n);
    const int got = readRows(&vap, i, &rows[0], n);
    if (got == -1) { S.faulted = true; ++S.faults; return false; }
    if (got != n) return false;
    std::vector<bba::Row> R; R.reserve(rows.size());
    long long bids = 0, asks = 0;
    for (const VOLPROFILE& v : rows) {
        long long k = 0;
        if (!keyOf((double)v.price, S.tick, k) || v.buyVolume < 0 || v.sellVolume < 0) { ++S.mismatch; continue; }
        bba::Row r; r.key = k; r.bid = (long long)v.sellVolume; r.ask = (long long)v.buyVolume;
        bids += r.bid; asks += r.ask; R.push_back(r);
    }
    if (bids != (long long)st.sellVolume || asks != (long long)st.buyVolume) ++S.mismatch;   // counted, never blanks the bar
    F = bba::facts(t, closed, ko, kh, kl, kc, bba::mergeRows(R), true);
    return true;
}
// the facts of the candles in view (and the 20 + LEAD before it): closed candles final once, the forming one live
void BigBidAsk::compute(BBState& S, bool fromDraw) {
    const long count = getBarCount();
    if (count < 2) { S.state = "waiting for bars"; return; }
    if (S.faulted) { S.state = "VAP fault: remove and re-add the indicator to retry"; return; }
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    RTARRAYP vap(barVolumeProfile);
    if (times.count < count || op.count < count || hi.count < count || lo.count < count || cl.count < count || vap.count < count) { S.state = "chart data arrays unavailable"; return; }
    const long long first = (long long)times[0], last = (long long)times[(int)count - 1];
    const long long now = steadyMs();
    const bool changed = count != S.lastCount || first != S.lastFirst || last != S.lastLast;
    if (!changed && S.builtMs >= 0 && now >= S.builtMs && now - S.builtMs < 1000) return;   // at most once a second unless a bar came
    S.builtMs = now; S.lastCount = count; S.lastFirst = first; S.lastLast = last;
    int vs = 0, ve = -1;
    if (!fromDraw || getVisibleBars(&vs, &ve) != RTX_OK || vs < 0 || ve < vs) { vs = std::max(0L, count - 400); ve = (int)count - 1; }
    const int from = std::max(0, vs - LEAD), lo0 = std::max(0, from - bba::LOOKBACK);
    std::vector<bba::Facts> seq; seq.reserve((size_t)(count - lo0));
    int reads = 0; bool capped = false;
    for (int i = lo0; i < (int)count; ++i) {
        const long long t = (long long)times[i];
        const bool closed = i < (int)count - 1;
        bba::Facts F;
        auto fi = S.fin.find(t);
        if (closed && fi != S.fin.end()) { seq.push_back(fi->second); continue; }
        auto ri = S.raw.find(t);
        if (closed && ri != S.raw.end()) { seq.push_back(ri->second); continue; }
        if (reads >= READS_MAX) { capped = true; F.t = t; F.closed = closed; seq.push_back(F); continue; }
        ++reads;
        const bool ok = readBar(S, vap, i, t, closed, op[i], hi[i], lo[i], cl[i], F);
        if (S.faulted) { S.state = "VAP fault: remove and re-add the indicator to retry"; return; }
        if (closed) {
            if (ok) { S.raw[t] = F; S.fails.erase(t); }
            else {
                ++S.missing;
                BBState::Fail& fl = S.fails[t]; if (fl.n++ == 0) fl.firstMs = now;
                if (fl.n >= FAILS_FINAL && now - fl.firstMs >= 60000) { F.valid = false; S.raw[t] = F; S.fails.erase(t); }   // no data from IRT for a minute: "no numbers" (not recorded)
            }
        }
        seq.push_back(F);
    }
    // ratios (past only) and the final facts of every closed candle whose 20 priors are all known
    for (size_t k = 0; k < seq.size(); ++k) {
        const int i = lo0 + (int)k;
        if (i < from) continue;
        const long long t = seq[k].t;
        if (seq[k].closed && S.fin.count(t)) continue;
        bool priorsKnown = true;
        for (size_t j = k > (size_t)bba::LOOKBACK ? k - bba::LOOKBACK : 0; j < k; ++j) if (!S.fin.count(seq[j].t) && !S.raw.count(seq[j].t)) priorsKnown = false;
        if (k < (size_t)bba::LOOKBACK && lo0 > 0) priorsKnown = false;          // the priors start before what was read
        bba::rate(seq, k);
        if (!seq[k].closed) { S.forming = seq[k]; S.haveForming = true; continue; }
        if (!S.raw.count(t) || !priorsKnown) continue;                           // not final yet: retried next pass
        S.fin[t] = seq[k]; S.raw.erase(t); ++S.decided;
        const long long sess = sessionOf(t);
        if (seq[k].valid && sess >= S.session - 2 && sess <= S.session && sess > 0 && !S.recorded.count(t) && S.pending.size() < REC_LINES_MAX) S.pending.push_back(std::make_pair(sess, seq[k]));
    }
    if (S.raw.size() > 50000) S.raw.clear();                                        // bounded (re-read if needed)
    S.state = capped ? "reading history (continues next pass)" : "ready";
}
void BigBidAsk::load(BBState& S) {
    const std::string d = dir(); if (d.empty()) return;
    for (long long k = S.session - 2; k <= S.session; ++k) {
        const std::string path = d + BBA_SEP + stem(S) + "-" + std::to_string(k) + ".csv";
        long long st = -1; if (!statFile(path, st)) continue;
        std::string text; if (!readSmall(path, REC_FILE_MAX, text)) { ++S.recFails; continue; }
        std::istringstream in(text); std::string line; bool head = true; size_t n = 0;
        while (std::getline(in, line) && ++n <= REC_LINES_MAX) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (head) { head = false; if (line != bba::REC_HEADER) break; continue; }
            bba::Facts F; if (!bba::parseRec(line, F)) continue;
            if (!S.fin.count(F.t)) S.fin[F.t] = F;                     // first wins: a candle's line never changes
            S.recorded.insert(F.t); S.raw.erase(F.t);
        }
    }
}
bool BigBidAsk::flush(BBState& S) {
    if (S.pending.empty()) return true;
    const long long now = steadyMs();
    if (S.lastRecordMs >= 0 && now >= S.lastRecordMs && now - S.lastRecordMs < 1000) return false;   // a failed publish is retried after 1 s
    const std::string d = dir(); if (d.empty()) { ++S.recFails; S.lastRecordMs = now; return false; }
    makeDir(flexDir()); makeDir(d);
    std::map<long long, std::vector<size_t>> groups;
    for (size_t i = 0; i < S.pending.size(); ++i) groups[S.pending[i].first].push_back(i);
    std::vector<char> done(S.pending.size(), 0);
    for (const auto& g : groups) {
        const std::string path = d + BBA_SEP + stem(S) + "-" + std::to_string(g.first) + ".csv";
        std::string old; long long st = -1; std::vector<std::string> keep; std::set<long long> seen;
        if (statFile(path, st)) {
            if (!readSmall(path, REC_FILE_MAX, old)) { ++S.recFails; continue; }      // never rewrite a file we could not read
            std::istringstream in(old); std::string line; bool head = true, bad = false;
            while (std::getline(in, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (head) { head = false; if (line != bba::REC_HEADER) { bad = true; break; } continue; }
                bba::Facts F; if (bba::parseRec(line, F) && seen.insert(F.t).second) keep.push_back(bba::recLine(F));
            }
            if (bad) { ++S.recFails; continue; }
        }
        std::string text = std::string(bba::REC_HEADER) + "\n";
        for (const auto& k : keep) text += k + "\n";
        for (size_t j : g.second) { const bba::Facts& F = S.pending[j].second; if (seen.insert(F.t).second) text += bba::recLine(F) + "\n"; }
        if (text.size() > REC_FILE_MAX || !publishFile(path, text)) { ++S.recFails; continue; }
        for (size_t j : g.second) { S.recorded.insert(S.pending[j].second.t); done[j] = 1; }
    }
    std::vector<std::pair<long long, bba::Facts>> retry;
    for (size_t i = 0; i < S.pending.size(); ++i) if (!done[i]) retry.push_back(S.pending[i]);
    S.pending.swap(retry);
    S.lastRecordMs = S.pending.empty() ? -1 : now;
    return S.pending.empty();
}
void BigBidAsk::status(BBState& S, long long now) {
    if (S.lastStatusMs >= 0 && now >= S.lastStatusMs && now - S.lastStatusMs < 2000) return;
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << BBA_VERSION << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nSTATE," << S.state
      << "\nTICK," << S.tick
      << "\nSETTINGS,font " << S.set.font << ",bold " << bba::ratioText(S.set.boldQ) << ",circle " << (S.set.circle ? 1 : 0) << ",non-bold " << (S.set.nonBold ? 1 : 0) << "," << S.settingsWhy
      << "\nCANDLES,final " << S.fin.size() << ",recorded " << S.recorded.size() << ",pending " << S.pending.size() << ",record fails " << S.recFails
      << "\nVAP,missing reads " << S.missing << ",bid/ask vs bar totals mismatch " << S.mismatch << ",faults " << S.faults
      << "\nDRAWN,numbers " << S.lay.shown << ",nudged " << S.lay.nudged << ",bold without room " << S.lay.crowded << ",not drawn (no room) " << S.lay.dropped << ",circles " << S.circles << ",circles without room " << S.circlesHidden << ",bold at a zone " << S.zoneFull << ",bold mid-range (dimmed) " << S.zoneDim
      << "\nAREA,left " << S.leftLimit << ",price right " << S.priceRight << ",columns levels " << (S.peers.on[cb::LEVELS] ? "on" : "off") << " delta " << (S.peers.on[cb::DELTA] ? "on" : "off")
      << " dealer " << (S.peers.on[cb::DEALER] ? "on" : "off") << " liq " << (S.peers.on[cb::LIQ] ? "on" : "off") << ",tape reader banner " << (S.trOn ? "avoided" : "none") << "\n";
    const std::string text = o.str();
    if (text == S.lastStatusText && S.lastStatusMs >= 0 && now - S.lastStatusMs < 30000) return;
    S.lastStatusMs = now;
    const std::string f = flexDir(); if (f.empty()) return;
    if (publishFile(f + BBA_SEP "BigBidAsk.status-" + stem(S) + ".txt", text)) S.lastStatusText = text;
}
void BigBidAsk::pump(bool fromDraw) {
    BBState* P = slot_.get(this, true);
    if (!P || P->busy) return;
    BBState& S = *P; S.busy = true;
    struct Unbusy { BBState* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
    try {
        identify(S);
        const double tick = cleanTick(getProperty(SYM_TICKINCR));
        if (tick <= 0) { S.state = "no tick size (SYM_TICKINCR)"; return; }
        if (tick != S.tick) { S.fin.clear(); S.raw.clear(); S.fails.clear(); S.recorded.clear(); S.pending.clear(); S.haveForming = false; S.session = -1; S.lastCount = -1; S.tick = tick; }
        const long long now = steadyMs();
        const long long ct = ctSec(currentDate());
        if (ct > 0) { const long long key = (ct - 17LL * 3600) / 86400LL + 1; if (key != S.session) { S.session = key; load(S); } }
        settings(S, now);
        compute(S, fromDraw);
        flush(S);
        status(S, now);
    } catch (...) { S.state = "calculation error"; }
}
template <class Times> static int indexOf(Times& times, long count, long long t) {
    long lo = 0, hi = count - 1;
    while (lo <= hi) {
        const long mid = lo + (hi - lo) / 2; const long long v = (long long)times[(int)mid];
        if (v == t) return (int)mid;
        if (v < t) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}
void BigBidAsk::render(BBState& S) {
    S.targets.clear(); S.circles = 0; S.circlesHidden = 0; S.zoneFull = 0; S.zoneDim = 0;
    const long count = getBarCount();
    if (count < 2 || S.tick <= 0) return;
    RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
    int axisR = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) axisR = scale.left - 2;
    if (pane.bottom - pane.top < 60 || axisR - pane.left < 120) { S.state = "pane too small: nothing drawn"; return; }
    // where the price area ends: the right-side columns (ColumnBoard.h, the same layout the profiles compute), the left Liquidity column
    {   const char* sy = getSymbol(); const char* cl = getChartLabel();
        const std::string sym = sy ? sy : "", chart = cl ? cl : "";
        S.chartKey = chart;
        S.peers.step(sym, getSecondsPerBar(), chart, steadyMs(), 0u);
        cb::In in; in.paneL = pane.left; in.paneT = pane.top; in.paneB = pane.bottom; in.axisR = axisR;
        in.levels = S.peers.on[cb::LEVELS]; in.levelsW = S.peers.w[cb::LEVELS]; in.delta = S.peers.on[cb::DELTA]; in.deltaW = S.peers.w[cb::DELTA];
        in.dealer = S.peers.on[cb::DEALER]; in.dealerW = S.peers.w[cb::DEALER];
        in.big = S.peers.on[cb::BIG]; in.bigW = S.peers.w[cb::BIG];   // (1.1.0) the BIG column
        const cb::Layout L = cb::layout(in);
        S.priceRight = (L.ok && L.margin > 0) ? L.priceRight : axisR;
        S.leftLimit = pane.left + (S.peers.on[cb::LIQ] ? S.peers.w[cb::LIQ] + 2 : 2);
        // the Tape Reader's banner (BannerBoard.h) - the one other indicator box this plugin can see: never covered
        bb::Rect r; long long ms = -1; const bool have = bb::peekBanner(sym, getSecondsPerBar(), chart, r, ms);
        S.trOn = S.trBeat.step(have, ms, steadyMs());
        if (S.trOn) { S.trBox.l = r.l; S.trBox.t = r.t; S.trBox.r = r.r; S.trBox.b = r.b; }
    }
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow);
    if (times.count < count || hi.count < count || lo.count < count) return;
    int vs = 0, ve = -1;
    if (getVisibleBars(&vs, &ve) != RTX_OK || vs < 0 || ve < vs) { vs = std::max(0L, count - 200); ve = (int)count - 1; }
    ve = std::min(ve, (int)count - 1);
    const int from = std::max(0, vs - LEAD);
    const bba::Settings set = S.set;
    // (1.2.0, Rassul approved mockup-location-first-v1 08:22 CT) LOCATION FIRST: bold numbers and circles at a high / low / pivot /
    // key or MenthorQ level are full strength; the same marks mid-range are dimmed (never hidden, never moved) - ZoneNear.h
    zn::Ctx Z; { const int zn0 = std::max(0, from - 600);
        for (int i = 0; i <= ve; ++i) { Z.hi.push_back(i < zn0 ? 0.0 : (double)hi[i]); Z.lo.push_back(i < zn0 ? 0.0 : (double)lo[i]); Z.sess.push_back(i < zn0 ? -1 : sessionOf((long long)times[i])); }
        Z.band = zn::bandFrom(Z.hi, Z.lo, std::max(0, (int)count - 2 <= ve ? (int)count - 2 : ve));
        const char* sy = getSymbol(); Z.levels = zn::levels(flexDir(), S.market, sy ? sy : "", steadyMs()); }
    std::vector<double> candPx;
    FONT fp; fp.id = HELVETICA; fp.size = (short)set.font; fp.style = PLAIN;
    FONT fb = fp; fb.style = BOLD; fb.size = (short)(set.font + 3);   // (1.1.0) the really large numbers bold AND bigger, like the mockup 96
    int lead = 0, asc = 0, desc = 0; setFont(fp); getFontMetrics(&lead, &asc, &desc);
    const int th = std::max(set.font + 2, asc + desc);
    int leadB = 0, ascB = 0, descB = 0; setFont(fb); getFontMetrics(&leadB, &ascB, &descB); setFont(fp);
    const int thB = std::max(set.font + 5, ascB + descB);
    bba::Box area; area.l = S.leftLimit; area.t = pane.top + 20; area.r = std::min(S.priceRight, axisR) - 1; area.b = pane.bottom - 1;   // the IRT title row stays clear
    std::vector<bba::Box> candles; std::vector<bba::Cand> cands; std::vector<const bba::Facts*> src; std::vector<std::string> texts;
    for (int i = from; i <= ve; ++i) {
        PNT a; a.set(i, hi[i], kBarLeft); PNT b; b.set(i, lo[i], kBarRight);
        bba::Box cb_; cb_.l = std::min(a.h, b.h); cb_.r = std::max(a.h, b.h); cb_.t = std::min(a.v, b.v); cb_.b = std::max(a.v, b.v);
        candles.push_back(cb_);
        const long long t = (long long)times[i];
        const bba::Facts* F = nullptr;
        if (i < (int)count - 1) { auto it = S.fin.find(t); if (it != S.fin.end()) F = &it->second; }
        else if (S.haveForming && S.forming.t == t) F = &S.forming;
        if (!F || !F->valid) continue;
        const int ab = set.circle ? bba::absorption(*F, set.boldQ) : 0;
        for (int sd = -1; sd <= 1; sd += 2) {
            const bba::Pick& p = sd < 0 ? F->bid : F->ask;
            if (!p.ok) continue;
            const bool bold = bba::bold(sd < 0 ? F->bidQ : F->askQ, set.boldQ);
            if (!bold && !set.nonBold) continue;
            const std::string tx = bba::lots(p.vol);
            setFont(bold ? fb : fp);
            PNT pt; pt.set(i, (float)((double)p.key * S.tick), kBarCenter);
            bba::Cand c; c.bar = i; c.side = sd; c.x = sd < 0 ? cb_.l : cb_.r; c.y = pt.v; c.w = getTextWidth(tx.c_str(), -1) + 2; c.h = bold ? thB : th;
            c.bold = bold; c.circle = (sd < 0 && ab > 0) || (sd > 0 && ab < 0); c.first = bold && F->closed;
            cands.push_back(c); src.push_back(F); texts.push_back(tx); candPx.push_back((double)p.key * S.tick);
        }
    }
    std::vector<bba::Box> hard; if (S.trOn) hard.push_back(S.trBox);
    const std::vector<bba::Placed> P = bba::place(cands, candles, area, &S.lay, hard);
    for (size_t k = 0; k < cands.size(); ++k) {
        if (!P[k].shown) { if (cands[k].circle && cands[k].bar >= vs) ++S.circlesHidden; continue; }
        const bba::Cand& c = cands[k]; const bba::Box& t = P[k].text;
        if (c.bar < vs) continue;                                       // laid out (lead-in) but left of the view
        const bool zone = zn::atZone(Z, c.bar, candPx[k]);
        const COLOR col = zone ? (c.side < 0 ? C_SELL : C_BUY) : getBlendedColor(c.side < 0 ? C_SELL : C_BUY, 0x00000000, 55.0f);   // (1.2.0) mid-range = dimmed
        if (c.bold) { if (zone) ++S.zoneFull; else ++S.zoneDim; }
        setFont(c.bold ? fb : fp); setTextColor(c.bold ? col : (c.side < 0 ? C_SELL_DIM : C_BUY_DIM));
        RCT r; r.set(coord(t.l), coord(t.t), coord(t.r), coord(t.b)); r.drawText(texts[k].c_str(), true, false);
        if (c.circle) {                                                  // the thin 1 px rounded outline hugging the number, its own colour
            const bba::Box& f = P[k].foot;
            setPen(col, 1, P_SOLID); CBRUSH br(col, PAT_HOLLOW); br.set();
            RCT ring; ring.set(coord(f.l), coord(f.t), coord(f.r), coord(f.b)); ring.drawRounded(f.b - f.t, f.b - f.t);
            ++S.circles;
        }
        HoverT H; H.box = P[k].foot; H.t = src[k]->t; H.side = c.side; S.targets.push_back(H);
    }
    // the hover: only while the cursor is over this chart's window and over a number
    if (!S.msHas || S.msWnd == 0 || hb::hbCursor() != S.msWnd) { S.hovKey.clear(); return; }
    const HoverT* T = nullptr;
    for (const auto& h : S.targets) if (S.msH >= h.box.l && S.msH <= h.box.r && S.msV >= h.box.t && S.msV <= h.box.b) { T = &h; break; }
    S.hovKey = T ? std::to_string(T->t) + (T->side < 0 ? "b" : "a") : std::string();
    if (!T) return;
    const bba::Facts* F = nullptr;
    { auto it = S.fin.find(T->t); if (it != S.fin.end()) F = &it->second; else if (S.haveForming && S.forming.t == T->t) F = &S.forming; }
    if (!F) return;
    const std::vector<std::string> L = bba::hover(*F, S.tick, set.boldQ, clockOf(F->t), T->side, set.circle);   // circle off: no absorption lines
    FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
    int w = 0; for (const auto& s : L) w = std::max(w, getTextWidth(s.c_str(), -1));
    w += 16; const int lh = 15, h = (int)L.size() * lh + 10;
    int l = S.msH + 14, t = S.msV + 14;
    if (l + w > axisR - 2) l = S.msH - 14 - w;
    if (l < pane.left + 2) l = pane.left + 2;
    if (t + h > pane.bottom - 2) t = pane.bottom - 2 - h;
    if (t < pane.top + 2) t = pane.top + 2;
    RCT bg; bg.set(coord(l), coord(t), coord(l + w), coord(t + h)); bg.draw(1, C_HOVEDGE, C_HOVBG, DRAW_OPAQUE, PAT_SOLID);
    for (size_t i = 0; i < L.size(); ++i) {
        setTextColor(i == 0 ? (T->side < 0 ? C_SELL : C_BUY) : C_INK);
        RCT tr; tr.set(coord(l + 8), coord(t + 5 + (int)i * lh), coord(l + w - 4), coord(t + 5 + (int)(i + 1) * lh)); tr.drawText(L[i].c_str(), false, false);
    }
}
// the mouse: remember where it is; redraw only when the hovered number changes. Never swallows the event.
int BigBidAsk::mouse(RTX_EVENT* e) {
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    BBState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    S->msHas = true; S->msH = e->v.mouse.h; S->msV = e->v.mouse.v; S->msWnd = hb::hbCursor();
    std::string k;
    for (const auto& t : S->targets) if (S->msH >= t.box.l && S->msH <= t.box.r && S->msV >= t.box.t && S->msV <= t.box.b) { k = std::to_string(t.t) + (t.side < 0 ? "b" : "a"); break; }
    if (k != S->hovKey) { S->hovKey = k; invalidateChart(false); }
    return RTX_FAIL;
}

extern "C" cppExtension* CreateExtension(void) {
    BigBidAsk* p = new BigBidAsk();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE | VAP_REQUIRED);
    p->setDescription("LRA Big Bid / Ask v1.2.0: on every candle, the RED number left of the candle sits at the price where the most lots traded AT THE BID (sellers hitting) and shows those lots; the GREEN number right of the candle sits at the price where the most lots traded AT THE ASK (buyers lifting). Ties go to the price nearest the close. Bold = at least 'Bold at' x the median of the same side's biggest over the prior 20 candles. A thin circle = absorption, decided at the candle's close and never redrawn: bold red at or below the candle's halfway mark and the candle closes green above it (bullish), or bold green at or above the halfway mark and the candle closes red below it (bearish). Hover a number for the prices, lots, ratios and how to use it. From the chart's own volume at price (as the Delta Profile). Settings: Font size, Bold at (x normal), Absorption circle, Show non-bold numbers.");
    p->setVersion("1.2.0");   // BBA_VERSION
    return p;
}
