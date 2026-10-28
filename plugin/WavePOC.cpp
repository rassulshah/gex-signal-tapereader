/********************************************************************************
 *  WavePOC.cpp  --  Investor/RT RTX extension  lsWavePOC  "Wave POC"  1.0.0  (2026-10-28)
 *
 *  Rassul 2026-10-28 (idea "Wave POC" -> mockups v1-v3 -> approved 10:59 CT "just build now"): every BIG wave's own volume profile,
 *  its separate high-volume nodes as pullback targets.
 *    - waves = legs of the N-bar reversal zigzag (N = "Reversal bars", default 3), up AND down; a wave counts when it ends at a 20-bar
 *      high / low and is "bigger than the average wave making a new 20 bar high (or low)" (rolling 20 per direction per market, no
 *      look-ahead, kept in lsFlexLevels\WavePOC\<MKT>-<spb>.csv);
 *    - its profile (IRT's own volume at price, VAP_REQUIRED) drawn at the wave start pointing right, muted slate, behind the candles;
 *    - every separate node >= 2.0x the wave's normal row, max 5, highlighted ("also you need to highlight the node"): the biggest =
 *      WPOC, bright amber filled row + 3 px amber line; others bright light-grey row + 1 px line (no outline boxes, Rassul 11:21 CT);
 *    - lines run right from the wave's extreme: DASHED while provisional (a new extreme moves it), SOLID from the confirmation bar
 *      (N bars later), never repainted after; a line stops at the first touch (circle), the next qualifying wave or a close beyond the
 *      wave's start (end tick);
 *    - right-edge price tags "WPOC 7700.25 6.3x" / "7693.75 2.5x" only (right of the last bar, never over candles), stacked;
 *    - optional zigzag (off by default); header "Wave POC v1.0.0" small top-left.
 *  Rules, files and layout: WavePOCLogic.h (SDK-free, tested: test_wavepoc_logic.cpp on the Oct 22 ES data of the approved mockup;
 *  host: test_wavepoc_host.cpp on a mock SDK). Settings: WavePOCSettings.h, read from IRT's own per-indicator parameters
 *  (getParameters - the SessionVWAP 1.8.1 native pattern; refused / invalid = the defaults).
 *  FILES (lsFlexLevels\WavePOC\):  <MKT>-<spb>.csv          the candidate waves (rolling averages), atomic writes, garbled = rebuilt
 *                                  <MKT>-<spb>-nodes.csv    every qualified wave + nodes + stops (append-only; redrawn from it)
 *          lsFlexLevels\WavePOC.status-<MKT>-<spb>.txt      version, N, waves, averages, VAP, nodes, stops (on change / 30 s)
 *  Per chart state in the host slot (one DLL serves every chart); each closed bar is processed once (no recompute per repaint);
 *  VAP of a wave is read once when it qualifies (missing bars -> retried every 10 min).
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>     // BEFORE irtsdk.h (MSVC C2371 RTBOOL clash otherwise)
#endif
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "WavePOCLogic.h"
#include "WavePOCSettings.h"
#include "SettingsHost.h"
#include "HostSlot.h"
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#if !defined(_WIN32)
#include <sys/types.h>
#endif

#if defined(_WIN32)
#define WP_SEP "\\"
#else
#define WP_SEP "/"
#endif

static const char* WP_VERSION = "1.0.0";   // = wpc::VERSION = wpset::PLUGIN_VERSION = the setVersion literal (test_wavepoc_host checks)
static const int WP_MAX_BARS = 20000;      // closed bars replayed at most (history)
static const long long WP_RETRY_MS = 10LL * 60 * 1000;   // VAP-missing waves retried every 10 min

static ss::Ids& wpIds() { static ss::Ids I; return I; }

static long long wpMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string wpFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + WP_SEP "InvestorRT" WP_SEP "rtx" WP_SEP "lsFlexLevels" : std::string(); }
static void wpMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool wpExists(const std::string& p) { struct stat st; return !p.empty() && stat(p.c_str(), &st) == 0; }
// atomic publish: .tmp -> old to .bak -> rename -> the .bak back on failure
static bool wpPublish(const std::string& path, const std::string& text) {
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false; f.write(text.data(), (std::streamsize)text.size()); f.flush(); if (!f.good()) { f.close(); std::remove(tmp.c_str()); return false; } }
#if defined(_WIN32)
    if (::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    std::remove(tmp.c_str()); return false;
#else
    const bool had = wpExists(path);
    if (had) { std::remove(bak.c_str()); if (std::rename(path.c_str(), bak.c_str()) != 0) { std::remove(tmp.c_str()); return false; } }
    if (std::rename(tmp.c_str(), path.c_str()) != 0) { if (had) std::rename(bak.c_str(), path.c_str()); std::remove(tmp.c_str()); return false; }
    if (had) std::remove(bak.c_str());
    return true;
#endif
}
static bool wpReadSmall(const std::string& path, std::string& out, size_t cap) {
    std::ifstream f(path.c_str(), std::ios::binary); if (!f) return false;
    f.seekg(0, std::ios::end); const std::streamoff n = f.tellg(); if (n < 0 || (size_t)n > cap) return false;
    f.seekg(0); out.assign((size_t)n, '\0'); if (n > 0) f.read(&out[0], n); return (bool)f;
}

// the parameter set IRT keeps for this indicator: one token per settings row, in row order
namespace wpp {
struct Parms { int revBars; RTBOOL zigzag; };
static const char* const TEMPLATE = "IB";
static const char* const DEFAULTS = "3,0";
inline bool valid(const Parms& p, std::string& why) {
    if (p.revBars < wpc::N_MIN || p.revBars > wpc::N_MAX) { why = "reversal bars out of range"; return false; }
    if (p.zigzag != 0 && p.zigzag != 1) { why = "box not 0/1"; return false; }
    why = "ok"; return true;
}
} // namespace wpp

struct WState {
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; int N = wpc::N_DEF; bool zz = false; std::string parms = "none";
    bool keyed = false;
    wpc::Store store; std::string storeWhy = "not loaded", storePath, recPath, statusPath, recWhy = "not loaded"; int recBad = 0;
    wpc::Engine E;
    std::vector<RTDATE> raw;   // the chart stamps fed, chart index off + j
    int off = 0;
    long long lastRetry = -1; int retries = 0;
    int provAt = -2; bool provOk = false; wpc::Wave prov;
    bool vapFault = false, sawPre = false, sawPost = false; int preCalls = 0;
    long long statusMs = -1; std::string statusBody; long long storeSaveMs = -1; int recFails = 0;
    wpc::Scene last;
};

class WavePOC : public cppExtension {
public:
    WavePOC() : cppExtension() {}
    virtual int draw(void);
    // the window: IRT shows and keeps this indicator's own values (native parameters); only a copy IRT hands no valid set gets the
    // table's defaults. Nothing is saved by the plugin (IRT stores the window's OK with the indicator) - the settings layer calls are kept
    // so the settings contract checks this window like every other.
    virtual int parmsLoad(void) {
        if (!ss::inDialog(this) || !wpIds().ok) return RTX_OK;
        wpp::Parms p; std::string why; if (nativeParms(p, why)) return RTX_OK;
        ss::fillDialog(this, wpset::spec(), wpIds(), ss::defaults(wpset::spec(), -1), snap_);
        return RTX_OK;
    }
    virtual int parmsApply(void) {
        if (!ss::inDialog(this) || !wpIds().ok) return RTX_OK;
        const ss::Values V = ss::readDialog(this, wpset::spec(), wpIds(), ss::defaults(wpset::spec(), -1), snap_);   // read only (diagnostic)
        lastWindowN_ = V.i(wpset::K_REV_BARS);
        return RTX_OK;
    }
    virtual int parmsUpdt(unsigned int i) { if (wpIds().ok) ss::onUpdate(this, wpset::spec(), wpIds(), (int)i, snap_); return RTX_OK; }
    ss::Snap snap_; int lastWindowN_ = 0;
    bool nativeParms(wpp::Parms& p, std::string& why);
    HostSlot<WState> slot_;
    void pump();
    void clear() { slot_.release(this); }
    void readSettings(WState& S);
    bool identify(WState& S);
    void loadFiles(WState& S);
    void feed(WState& S);
    void flush(WState& S, long long now);
    void status(WState& S, long long now);
    int vapRows(WState& S, int chartBar, std::vector<wpc::VRow>& out);
    long long localSec(RTDATE d);
    static int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    static int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
    void render(WState& S, bool back, bool front);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { static_cast<WavePOC*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<WavePOC*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { WavePOC* me = static_cast<WavePOC*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }
int cppExtension::setup(void) {
    ss::registerAll(this, wpset::spec(), wpIds());   // the window: Reversal bars, Show zigzag (controls only)
    return RTX_OK;
}

// SEH-guarded VAP reads (only trivial values inside, /EHsc compatible) - as lsDeltaProfile
int WavePOC::readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
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
int WavePOC::readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) { std::memset(&rows[i], 0, sizeof(rows[i])); if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2; }
        return count;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// one bar's volume at price -> rows in ticks: 1 = read, 0 = no VAP for it (yet), -1 = fault (never re-entered)
int WavePOC::vapRows(WState& S, int chartBar, std::vector<wpc::VRow>& out) {
    out.clear();
    if (S.vapFault) return -1;
    RTARRAYP vap(barVolumeProfile);
    if (vap.count <= chartBar) return 0;
    BARSTATISTICS st; const int prices = readStatistics(&vap, chartBar, &st);
    if (prices == -1) { S.vapFault = true; return -1; }
    if (prices <= 0) return 0;
    if (prices > 100000) return 0;
    std::vector<VOLPROFILE> rows((size_t)prices);
    const int got = readPrices(&vap, chartBar, &rows[0], prices);
    if (got == -1) { S.vapFault = true; return -1; }
    if (got < 0) return 0;
    for (const VOLPROFILE& v : rows) {
        if (!std::isfinite((double)v.price) || v.totalVolume < 0) continue;
        double vol = (double)v.totalVolume; const double bs = (double)v.buyVolume + (double)v.sellVolume; if (bs > vol + 0.5) vol = bs;
        if (vol <= 0) continue;
        wpc::VRow r; if (!wpc::toTicks((double)v.price, S.tick, r.tick)) continue;
        r.vol = (long long)std::llround(vol); out.push_back(r);
    }
    return out.empty() ? 0 : 1;
}

long long WavePOC::localSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, dd = (unsigned)t.tm_mday;
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

// the chart's own settings, from IRT's parameter set for this indicator (every chart keeps its own); refused / invalid = the table's defaults
bool WavePOC::nativeParms(wpp::Parms& p, std::string& why) {
    std::memset(&p, 0, sizeof p);
    if (getParameters(&p, sizeof p, wpp::TEMPLATE, wpp::DEFAULTS) != RTX_OK) { why = "refused"; return false; }
    return wpp::valid(p, why);
}
void WavePOC::readSettings(WState& S) {
    const ss::Spec& T = wpset::spec();
    wpp::Parms p; std::string why;
    if (!nativeParms(p, why)) { S.N = wpc::clampN((long long)T.rows[wpset::K_REV_BARS].def); S.zz = T.rows[wpset::K_ZIGZAG].def != 0; S.parms = "defaults," + why; return; }
    S.N = wpc::clampN(p.revBars); S.zz = p.zigzag != 0; S.parms = "native,ok";
}

bool WavePOC::identify(WState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb);
    const std::string mkt = wpc::marketForRoot(rs ? rs : "");
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) tick = getTickIncrement();
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) return false;
    if (mkt.empty()) return false;
    readSettings(S);
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey || S.N != S.E.N) {
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true;
        S.E.reset(); S.E.N = S.N; S.E.tick = tick; S.E.store = &S.store; S.raw.clear(); S.off = 0; S.provAt = -2; S.provOk = false;
        loadFiles(S);
    }
    return true;
}

void WavePOC::loadFiles(WState& S) {
    const std::string d = wpFlex(); if (d.empty()) { S.storeWhy = "no USERPROFILE"; return; }
    const std::string dir = d + WP_SEP "WavePOC", key = S.mkt + "-" + std::to_string(S.spb);
    S.storePath = dir + WP_SEP + key + ".csv"; S.recPath = dir + WP_SEP + key + "-nodes.csv";
    S.statusPath = d + WP_SEP "WavePOC.status-" + key + ".txt";
    std::string text; S.store = wpc::Store(); S.store.n = S.N; S.store.tickKey = S.tickKey;
    if (!wpReadSmall(S.storePath, text, 512 * 1024)) S.storeWhy = wpExists(S.storePath) ? "unreadable - rebuilt from the chart" : "new - built from the chart";
    else { wpc::Store got; std::string why; if (wpc::storeFromText(text, S.mkt, S.spb, S.N, S.tickKey, got, why)) { S.store = got; S.storeWhy = "loaded"; } else S.storeWhy = why + " - rebuilt from the chart"; }
    S.store.dirty = true;
    std::vector<std::string> lines; std::string rt;
    if (wpReadSmall(S.recPath, rt, 8 * 1024 * 1024)) { std::istringstream in(rt); std::string l; while (std::getline(in, l) && lines.size() < 200000) { if (!l.empty() && l.back() == '\r') l.pop_back(); lines.push_back(l); } S.recWhy = "loaded"; }
    else S.recWhy = wpExists(S.recPath) ? "unreadable" : "new";
    S.E.rec = wpc::parseRecord(lines, S.N, S.tick, S.recBad);
}

// closed bars in order, each once; a changed history (first stamp, fewer bars, a stamp moved) = replay from the start
void WavePOC::feed(WState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const int closed = (int)n - 1;                        // bars 0 .. n-2 are closed (a later bar exists)
    const int fed = (int)S.raw.size();
    bool redo = false;
    if (fed > 0) {
        const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) redo = true;
    }
    if (redo) {
        S.E.reset(); S.E.N = S.N; S.E.tick = S.tick; S.E.store = &S.store; S.raw.clear(); S.provAt = -2;   // the store + record stay (superset)
    }
    if (S.raw.empty()) S.off = std::max(0, closed - WP_MAX_BARS);
    wpc::VapFn vap = [this, &S](int j, std::vector<wpc::VRow>& out) { return vapRows(S, j + S.off, out); };
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        wpc::Bar b; b.t = localSec((RTDATE)times[i]);
        if (!wpc::toTicks(op[i], S.tick, b.o) || !wpc::toTicks(hi[i], S.tick, b.h) || !wpc::toTicks(lo[i], S.tick, b.l) || !wpc::toTicks(cl[i], S.tick, b.c)) { b.o = b.h = b.l = b.c = S.E.B.empty() ? 0 : S.E.B.back().c; }
        if (b.h < b.l) std::swap(b.h, b.l);
        S.E.addBar(b, vap); S.raw.push_back((RTDATE)times[i]);
    }
    const long long now = wpMs();
    if (S.E.pendingCount() > 0 && (S.lastRetry < 0 || now - S.lastRetry >= WP_RETRY_MS)) {
        if (S.lastRetry >= 0) { ++S.retries; S.E.retryPending(vap); }
        S.lastRetry = now;
    }
    const int k = S.E.lastBar();
    if (k != S.provAt) { S.provAt = k; S.provOk = S.E.provisional(S.prov, vap); }
}

void WavePOC::flush(WState& S, long long now) {
    const std::string d = wpFlex(); if (d.empty()) return;
    if (!S.E.out.empty()) {
        wpMkdir(d); wpMkdir(d + WP_SEP "WavePOC");
        const bool fresh = !wpExists(S.recPath);
        std::ofstream f(S.recPath.c_str(), std::ios::binary | std::ios::app);
        if (f) { if (fresh) f << wpc::REC_HEAD << "\n"; for (const std::string& l : S.E.out) f << l << "\n"; f.flush(); }
        if (f.good()) S.E.out.clear(); else ++S.recFails;     // kept for the next try (never lost silently)
        if (S.E.out.size() > 5000) S.E.out.clear();
    }
    if (S.store.dirty && (S.storeSaveMs < 0 || now - S.storeSaveMs >= 5000)) {
        wpMkdir(d); wpMkdir(d + WP_SEP "WavePOC");
        S.storeSaveMs = now;
        if (wpPublish(S.storePath, S.store.toText(S.mkt, S.spb))) S.store.dirty = false;
    }
}

void WavePOC::status(WState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;              // calc comes on every tick: build the text at most once a second
    const std::string body = wpc::statusText(S.E, S.mkt, S.spb, S.parms, S.storeWhy, S.recWhy + (S.recFails ? ", append failures " + std::to_string(S.recFails) : std::string()), S.recBad, &S.last, S.provOk, S.retries)
                           + "SETTINGS,show zigzag " + (S.zz ? "on" : "off") + "\nDRAWING," + (S.sawPre ? "profile behind the candles (pre-draw)" : "profile drawn after the candles (no pre-draw call seen)") + "\n"
                           + (S.vapFault ? "VAP_FAULT,remove and re-add the indicator\n" : "");
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    S.statusMs = now; S.statusBody = body;
    wpMkdir(wpFlex()); wpPublish(S.statusPath, body);
}

void WavePOC::pump() {
    WState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S);
    const long long now = wpMs();
    flush(*S, now);
    status(*S, now);
}

static short wpC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

void WavePOC::render(WState& S, bool back, bool front) {
    const long n = getBarCount(); if (n < 2 || S.E.B.empty()) return;
    RCT pane; pane.getPaneRect(false);
    int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
    RTARRAY hi(barHigh), lo(barLow);
    wpc::View v; v.l = pane.left; v.t = pane.top; v.r = pane.right; v.b = pane.bottom; v.tick = S.tick; v.zigzag = S.zz;
    v.first = vs - S.off; v.last = ve - S.off; v.lastBar = (int)n - 1 - S.off;
    v.bw = std::max(1, (int)getPixelsPerBar());
    const int off = S.off; const double tick = S.tick;
    v.X = [this, off](int j) { PNT p; p.set(j + off, 0.0f, kBarCenter); return (double)p.h; };
    v.Y = [this, tick](double tk) { return (double)getVerticalDrawPosition((float)(tk * tick)); };
    FONT f9; f9.id = HELVETICA; f9.size = 9; f9.style = PLAIN; FONT f9b; f9b.id = HELVETICA; f9b.size = 9; f9b.style = BOLD;
    v.textW = [this, &f9, &f9b](const std::string& s, bool bold) { setFont(bold ? f9b : f9); return (int)getTextWidth(s.c_str(), -1); };
    if ((long)hi.count >= n && (long)lo.count >= n) { long long h = 0, l = 0; if (wpc::toTicks(hi[(int)n - 1], tick, h) && wpc::toTicks(lo[(int)n - 1], tick, l)) { v.formH = std::max(h, l); v.formL = std::min(h, l); v.haveForming = true; } }
    S.last = wpc::layout(S.E, S.provOk ? &S.prov : nullptr, v);
    const int L = pane.left, T = pane.top, R = pane.right, Bt = pane.bottom;
    for (const wpc::Prim& p : S.last.p) {
        if (p.back ? !back : !front) continue;
        switch (p.k) {
        case wpc::Prim::FILL: case wpc::Prim::BOX: case wpc::Prim::OVAL: {
            const int x0 = std::max(L, std::min(p.x0, p.x1)), x1 = std::min(R, std::max(p.x0, p.x1)), y0 = std::max(T, std::min(p.y0, p.y1)), y1 = std::min(Bt, std::max(p.y0, p.y1));
            if (x1 <= x0 || y1 < y0) break;
            RCT r; r.set(wpC(x0), wpC(y0), wpC(x1), wpC(y1 == y0 ? y0 + 1 : y1));
            if (p.k == wpc::Prim::FILL) r.draw(0, (COLOR)p.col, (COLOR)p.col, DRAW_OPAQUE, PAT_SOLID);
            else { setPen((COLOR)p.col, (short)p.w, P_SOLID); CBRUSH br((COLOR)p.col, PAT_HOLLOW); br.set();
                   if (p.k == wpc::Prim::BOX) r.draw((short)p.w, (COLOR)p.col, (COLOR)p.col, DRAW_OPAQUE, PAT_HOLLOW); else r.drawOval(DRAW_OPAQUE); }
            break; }
        case wpc::Prim::LINE: {
            int x0 = p.x0, x1 = p.x1, y0 = p.y0, y1 = p.y1;
            if (y0 == y1) { if (y0 < T || y0 > Bt) break; x0 = std::max(L, std::min(R, x0)); x1 = std::max(L, std::min(R, x1)); if (x0 == x1) break; }
            else if (x0 == x1) { if (x0 < L || x0 > R) break; y0 = std::max(T, std::min(Bt, y0)); y1 = std::max(T, std::min(Bt, y1)); }
            else if (x0 < L || x0 > R || x1 < L || x1 > R || y0 < T || y0 > Bt || y1 < T || y1 > Bt) break;
            setPen((COLOR)p.col, (short)p.w, P_SOLID);
            PNT a; a.set(0, 0.0f); a.h = wpC(x0); a.v = wpC(y0); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = wpC(x1); b.v = wpC(y1); b.drawLineTo();
            break; }
        case wpc::Prim::TEXT: {
            if (p.y0 < T || p.y1 > Bt || p.x0 < L || p.x1 > R) break;
            setFont(p.bold ? f9b : f9); setTextColor((COLOR)p.col);
            RCT r; r.set(wpC(p.x0), wpC(p.y0), wpC(p.x1 + 2), wpC(p.y1)); r.drawText(p.text.c_str(), false, false);
            break; }
        }
    }
}

int WavePOC::draw(void) {
    WState* S = slot_.get(this, false);
    if (!S || S->E.B.empty()) { try { pump(); } catch (...) {} S = slot_.get(this, false); }
    if (!S) return RTX_OK;
    try {
        const bool pre = getReason() == REASON_PREDRAW;
        if (pre) { S->sawPre = true; ++S->preCalls; render(*S, true, !S->sawPost && S->preCalls > 3); }   // the profile + nodes + lines behind the candles
        else { S->sawPost = true; render(*S, !S->sawPre, true); }                                         // tags, zigzag, header (+ the back layer if IRT never pre-draws)
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    WavePOC* p = new WavePOC();
    p->setArrayCount(1);
    p->setFlags(PRE_DRAWING | POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | VAP_REQUIRED);
    p->setDescription("LRA Wave POC v1.0.0 - each big wave's own volume profile at its start and its high-volume nodes (WPOC amber) as lines to the right until touched.");
    p->setVersion("1.0.0");   // = WP_VERSION (gex-build reads this literal)
    return p;
}
