/********************************************************************************
 *  DealerHedge.cpp  --  Investor/RT RTX extension  lsDealerHedge  0.1.1  (2026-10-22; 0.1.1 the same day before any use: a feed snapshot stamped with old times is not flow)
 *  "oH" DEALER-HEDGING ZONES. Rassul agreed the design 15:23-15:24 CT, ordered the build 15:31 CT (MBO Roadmap "Are dealers hedging
 *  in the futures book at the gamma level?", T-20261022-27). Put ONE on the PRICE pane of each futures chart (any bar size).
 *    - Each gamma level near price (the Dealer Profile's MenthorQLevels.csv set: CR / PS / HVL / 0DTE / GW / G1-G10, + the CME gamma
 *      nodes of LRA-GexNodes-<MKT>.csv) gets a zone = level +/- 0.5 ATR(14) of this chart's bars. INVISIBLE until hedging activity
 *      shows inside it (the chart's own order-by-order feed, DEPTH_BY_ORDER NEW / MOD / DEL / FILL - DealerHedgeLogic.h has the rule).
 *    - Then it LIGHTS UP and PULSES (green = dealers buying, red = selling) while the activity is live, and settles to a steady faint
 *      band. Never removed or changed; it pulses again when activity restarts. "oH" below the candle where it was first decided.
 *    - Hover: level + price, what the book showed, dealer gamma (long / short, which side they must trade, agrees or not), size in
 *      lots / $ (+ the model gamma hedge when a node gives |GEX|), 0DTE time left, question / what / why / how / example; "?" = not proven.
 *  PULSE: a 250 ms IRT timer (createTimer) asks for a redraw ONLY while a zone is live; the brightness comes from the wall clock
 *  (a ~1 s breath, DealerHedgeLogic.h pulse()). Refused timer = it pulses on each redraw (in steps in a quiet market).
 *  NEVER REPAINTS: each zone's first decision (band, side, candle) is appended to lsFlexLevels\DealerHedge\<MKT>-<SYM>-<sec>-<session>.csv
 *  and redrawn from it after a restart. Status: lsFlexLevels\DealerHedge.status-<MKT>-<SYM>-<sec>.txt.
 *  The order feed is opened like lsMboCheck / OrderFlowFeed.h off::Conn (one object per chart kept alive, a crash-guard file, the
 *  private subscribe flag read through a pointer-to-member, drained in slices) - re-implemented here so this plugin does not pull in
 *  the shared order-flow headers other builds are editing.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#include "SettingsGuard.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "DealerHedgeLogic.h"
#include "HoverBoard.h"                  // hb::hbCursor (GetCursorPos + WindowFromPoint; carries the user32 pragma)
#include "HostSlot.h"
#include "KeepOutDraw.h"                 // nothing painted over the lsHodLod table
#include "DealerHedgeSettings.h"
#include "SettingsHost.h"
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")      // HoverBoard.h's GetCursorPos / WindowFromPoint
#endif
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <new>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#define DH_VERSION "0.2.0"   // = setVersion literal = dhset::PLUGIN_VERSION = dh::VERSION

static long long steadyMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static long long wallMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
static short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
static std::string flexDir() { const char* up = std::getenv("USERPROFILE"); if (!up || !up[0]) return std::string(); return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels"; }
static void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    (void)p;
#endif
}
static long long fileStamp(const std::string& p) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return -1;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return -1;
#endif
    if ((st.st_mode & S_IFMT) != S_IFREG) return -1;
    return (long long)st.st_mtime * 1000003LL + (long long)st.st_size;
}
static bool readSmall(const std::string& path, size_t cap, std::string& out) {
    out.clear(); std::ifstream in(path.c_str(), std::ios::binary); if (!in) return false;
    in.seekg(0, std::ios::end); const std::streamoff size = in.tellg();
    if (size < 0 || size > (std::streamoff)cap) return false;
    in.seekg(0, std::ios::beg); out.resize((size_t)size); if (size > 0) in.read(&out[0], size);
    return (bool)in || in.eof();
}
static bool moveOver(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
static bool publishFile(const std::string& path, const std::string& text) {
    const std::string tmp = path + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false; f.write(text.data(), (std::streamsize)text.size()); f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    if (!moveOver(tmp, path)) { std::remove(tmp.c_str()); return false; }
    return true;
}
static bool appendLines(const std::string& path, const std::vector<std::string>& v, const char* header) {
    if (v.empty()) return true;
    const bool fresh = fileStamp(path) < 0;
    std::ofstream f(path.c_str(), std::ios::binary | std::ios::app); if (!f) return false;
    if (fresh && header) f << header << "\n";
    for (const std::string& l : v) f << l << "\n";
    f.flush(); return (bool)f;
}
static std::string fileKey(const std::string& s) {
    std::string o; for (size_t i = 0; i < s.size() && o.size() < 24; ++i) { const unsigned char c = (unsigned char)s[i]; o.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_'); }
    return o.empty() ? "UNKNOWN" : o;
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
static double cleanTick(float raw) {
    const double d = (double)raw; if (!std::isfinite(d) || d <= 0 || d >= 1000) return 0;
    char b[32]; std::snprintf(b, sizeof(b), "%.6g", d); const double t = std::strtod(b, NULL);
    return std::isfinite(t) && t > 0 && t < 1000 ? t : 0;
}

// ---- the order feed (as lsMboCheck 1.1.0 / OrderFlowFeed.h off::Conn) -------------------------------------------------------------
template <class Tag, typename Tag::type M> struct DhRob { friend typename Tag::type dhRobbed(Tag) { return M; } };
struct DhDboFlag { typedef RTBOOL cppExtension::DEPTH_BY_ORDER::*type; friend type dhRobbed(DhDboFlag); };
template struct DhRob<DhDboFlag, &cppExtension::DEPTH_BY_ORDER::bDboSubscribed>;
static int dhSafeNext(cppExtension::DEPTH_BY_ORDER* d) {
#if defined(_MSC_VER)
    __try { return d->getNext() == RTX_OK ? 1 : 0; } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#else
    try { return d->getNext() == RTX_OK ? 1 : 0; } catch (...) { return -1; }
#endif
}
static const int FEED_SLICE_MAX = 20000;          // events per call at most
static const long long FEED_SLICE_MS = 12;        // or about this long
static const long long FEED_WAIT_MS = 60000;      // no event 60 s after the named open: one 2nd try with NULL
static const long long TIMER_MS = 250;            // the pulse clock (only redraws while a zone is live)

struct HoverT { std::string key; int l = 0, t = 0, r = 0, b = 0; };
struct DHState {
    bool busy = false;
    std::string identity, market, symbol, root, chart, state = "starting";
    long spb = 0; double tick = 0; long long session = -1;
    dh::Engine E; bool engineOn = false;
    // inputs
    dh::ParamLoad PL; long long paramsStamp = -2, paramsCheck = -1;
    std::vector<dh::Level> levels; long long mqStamp = -2, nodesStamp = -2, levelsCheck = -1; std::string levelsWhy = "not read yet", nodesWhy = "not read yet"; int mqN = 0, nodeN = 0;
    ss::Store setStore; ss::Values set = ss::defaults(dhset::spec(), -1);
    // feed
    cppExtension::DEPTH_BY_ORDER* dbo = nullptr; int attempt = 0, flag = -1; bool dead = false, blocked = false;
    long long openMs = -1, lastEventMs = -1, events = 0, faults = 0, caps = 0; std::string via;
    RTDATE stampCache = 0; long long stampMs = 0;
    // record
    std::string recPath, normPath; long long recRows = 0, recFails = 0; int restored = 0;
    // pulse timer
    bool timerTried = false, timerOn = false; long long timerTicks = 0, redrawsAsked = 0;
    // draw / hover
    std::vector<HoverT> targets; bool msHas = false; int msH = 0, msV = 0; unsigned long long msWnd = 0; std::string hovKey;
    bdk::TableWatch keep; std::string keepNote;
    long long lastStatusMs = -1; std::string lastStatus; double atr = 0; long long barKey = 0;
    ~DHState() { delete dbo; }
};
static ss::Ids& dhIds() { static ss::Ids I; return I; }
static std::string dhSettingsPath(const std::string& mkt) { const std::string f = flexDir(); return f.empty() || mkt.empty() ? std::string() : f + "\\DealerHedge.settings-" + mkt + ".txt"; }
static int dhTimerId(const void* me) { return 9311 + (int)(((uintptr_t)me >> 4) % 100000); }

class DealerHedge : public cppExtension {
public:
    SS_SAFE_DIALOG(DealerHedge, "DealerHedge")
    DealerHedge() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int timer(RTX_EVENT* e);
    virtual int parmsLoadBody(void) {
        if (!ss::inDialog(this)) return RTX_OK;
        ss::Store* st = dialogStore(); if (!st) return RTX_OK;
        ss::fillDialog(this, dhset::spec(), dhIds(), st->load(ss::marketIndex(dlgMkt_), steadyMs(), true), snap_);
        return RTX_OK;
    }
    virtual int parmsApplyBody(void) { saveSettings(); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::onUpdate(this, dhset::spec(), dhIds(), (int)i, snap_); saveSettings(); return RTX_OK; }
    void pump(bool fromDraw);
    void release() {
        DHState* S = slot_.get(this, false);
        if (S && S->timerOn) { destroyTimer(dhTimerId(S)); S->timerOn = false; }
        if (S) { flushRecord(*S); writeStatus(*S, true); }
        slot_.release(this);
    }
    DHState* testState() { return slot_.get(this, false); }
private:
    HostSlot<DHState> slot_;
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        char b[64]; std::memset(b, 0, sizeof(b)); const char* rs = getRootSymbol(b); const char* sy = getSymbol();
        const std::string root = rs ? rs : "", mkt = marketName(root.empty() ? (sy ? sy : "") : root);
        const std::string p = dhSettingsPath(mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &dhset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, dhset::spec(), dhIds(), st->load(ss::marketIndex(dlgMkt_), steadyMs(), true), snap_);
        makeDir(flexDir());
        if (st->save(V)) snap_.take(ss::dialogNow(this, dhset::spec(), dhIds(), V));
    }
    long long ctSec(RTDATE d) {
        struct tm t; std::memset(&t, 0, sizeof(t));
        if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
        return civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
    }
    bool identify(DHState& S);
    void loadSettings(DHState& S);
    void loadParams(DHState& S, long long now);
    void loadLevels(DHState& S, long long now);
    void openSession(DHState& S, long long session);
    void feed(DHState& S, long long now);
    void flushRecord(DHState& S);
    void writeStatus(DHState& S, bool force);
    void render(DHState& S);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<DealerHedge*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<DealerHedge*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void)   { ss::registerAll(this, dhset::spec(), dhIds()); return RTX_OK; }
int cppExtension::calc(int)     { static_cast<DealerHedge*>(this)->pump(false); return RTX_OK; }

int DealerHedge::draw(void) {
    pump(true);
    DHState* S = slot_.get(this, false);
    if (S) { try {
        S->keep.refresh(getSymbol(), getSecondsPerBar(), getChartLabel());
        bdk::KeepScope keepScope(S->keep);
        render(*S); S->keepNote = bdk::keepLine();
    } catch (...) { S->state = "draw error"; } }
    return RTX_OK;
}
// the pulse clock: a redraw is asked only on this chart's own timer, only while a zone is LIVE
int DealerHedge::timer(RTX_EVENT* e) {
    DHState* S = slot_.get(this, false);
    if (!S || !e || e->v.timer.id != dhTimerId(S)) return RTX_FAIL;
    ++S->timerTicks;
    bool live = false; for (const dh::Zone& z : S->E.zones) if (z.state == dh::LIVE) { live = true; break; }
    if (live && S->E.P.show != 0 && S->set.b(dhset::K_SHOW_ZONES) && S->set.b(dhset::K_PULSE)) { ++S->redrawsAsked; invalidateChart(false); }
    return RTX_OK;
}

bool DealerHedge::identify(DHState& S) {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol(); const char* cl = getChartLabel();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const long spb = getSecondsPerBar();
    const double tick = cleanTick(getProperty(SYM_TICKINCR));
    const std::string id = root + "|" + sym + "|" + std::to_string(spb) + "|" + std::to_string(tick);
    if (sym.empty() || !(tick > 0) || spb <= 0) { S.state = "no symbol / tick / bar size yet"; return false; }
    if (id == S.identity) return true;
    // a new chart identity: start clean (the old feed unsubscribes in delete)
    flushRecord(S);
    delete S.dbo; S.dbo = nullptr; S.attempt = 0; S.flag = -1; S.dead = S.blocked = false; S.openMs = S.lastEventMs = -1; S.via.clear();
    S.identity = id; S.root = root; S.symbol = sym; S.spb = spb; S.tick = tick; S.chart = cl ? cl : "";
    S.market = marketName(root.empty() ? sym : root);
    S.E = dh::Engine(); S.E.market = S.market; S.E.tick = tick; S.engineOn = true; S.session = -1;
    S.paramsStamp = -2; S.mqStamp = -2; S.nodesStamp = -2; S.setStore = ss::Store();
    return true;
}
void DealerHedge::loadSettings(DHState& S) {
    const std::string p = dhSettingsPath(S.market);
    if (p.empty()) { S.set = ss::defaults(dhset::spec(), ss::marketIndex(S.market)); return; }
    if (S.setStore.path != p) { S.setStore = ss::Store(); S.setStore.S = &dhset::spec(); S.setStore.path = p; }
    S.set = S.setStore.load(ss::marketIndex(S.market), steadyMs());
}
// the params file (nightly) -> the engine; the settings window's numbers win for width / flow / repeats / refills
void DealerHedge::loadParams(DHState& S, long long now) {
    if (S.paramsCheck < 0 || now - S.paramsCheck >= 5000 || S.paramsStamp == -2) {
        S.paramsCheck = now;
        const std::string f = flexDir(); const std::string p = f.empty() ? std::string() : f + "\\LRA-DealerHedge-" + S.market + ".csv";
        const long long st = p.empty() ? -1 : fileStamp(p);
        if (st != S.paramsStamp) {
            std::string text; const bool found = st >= 0 && readSmall(p, 64 * 1024, text);
            if (st < 0 || found) { S.PL = dh::parseParams(text, found, S.market); S.paramsStamp = st; }
        }
    }
    dh::Params P = S.PL.p;
    P.widthAtr = S.set.f(dhset::K_WIDTH_ATR); P.flowX = S.set.f(dhset::K_FLOW_X); P.repN = S.set.i(dhset::K_REP_N); P.refN = S.set.i(dhset::K_REF_N);
    if (!(P.widthAtr >= 0.1 && P.widthAtr <= 2.0)) P.widthAtr = S.PL.p.widthAtr;
    if (!(P.flowX >= 1.0 && P.flowX <= 10.0)) P.flowX = S.PL.p.flowX;
    if (P.repN < 2 || P.repN > 20) P.repN = S.PL.p.repN;
    if (P.refN < 2 || P.refN > 20) P.refN = S.PL.p.refN;
    const bool widthChanged = std::fabs(P.widthAtr - S.E.P.widthAtr) > 1e-9;
    S.E.P = P;
    for (const auto& kv : S.PL.norms) S.E.slotNorm[kv.first] = kv.second;     // the nightly's slot rows win
    if (widthChanged) S.E.rebuild();                                             // hidden zones only: lit bands never change
}
// MenthorQLevels.csv (the Dealer Profile's source) + LRA-GexNodes-<MKT>.csv, re-read when a stamp changes (checked every 5 s)
void DealerHedge::loadLevels(DHState& S, long long now) {
    if (S.levelsCheck >= 0 && now - S.levelsCheck < 5000) return;
    S.levelsCheck = now;
    const std::string f = flexDir(); if (f.empty()) { S.levelsWhy = "no USERPROFILE"; return; }
    const std::string pm = f + "\\MenthorQLevels.csv", pn = f + "\\LRA-GexNodes-" + S.market + ".csv";
    const long long sm = fileStamp(pm), sn = fileStamp(pn);
    if (sm == S.mqStamp && sn == S.nodesStamp) return;
    std::vector<dh::Level> mq, nodes; std::string text;
    if (sm >= 0 && readSmall(pm, 64 * 1024, text) && fileStamp(pm) == sm) { mq = dh::parseMq(text, S.market, S.symbol, S.tick); S.levelsWhy = mq.empty() ? "MenthorQLevels.csv: no " + S.market + " levels for this contract" : "ok"; }
    else if (sm >= 0) return;                                         // caught mid-write: keep the last good set, try again in 5 s
    else S.levelsWhy = "MenthorQLevels.csv missing";
    if (sn >= 0 && readSmall(pn, 64 * 1024, text) && fileStamp(pn) == sn) { if (!dh::parseNodes(text, S.market, (long long)std::time(NULL), 3 * 3600, S.tick, nodes, S.nodesWhy)) nodes.clear(); }
    else S.nodesWhy = sn >= 0 ? "nodes file unreadable" : "no LRA-GexNodes file";
    S.mqStamp = sm; S.nodesStamp = sn; S.mqN = (int)mq.size(); S.nodeN = (int)nodes.size();
    S.levels = dh::mergeNodes(mq, nodes, S.tick);
    S.E.setLevels(S.levels);
}
// a session's record back (no repaint) + the norms history; the engine starts the session clean
void DealerHedge::openSession(DHState& S, long long session) {
    flushRecord(S);
    S.session = session; S.E.newSession(session);
    const std::string f = flexDir(); if (f.empty()) { S.recPath.clear(); return; }
    const std::string d = f + "\\DealerHedge"; makeDir(f); makeDir(d);
    S.recPath = d + "\\" + S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb) + "-" + std::to_string(session) + ".csv";
    S.normPath = d + "\\norms-" + S.market + ".csv";
    std::string text;
    if (readSmall(S.recPath, 1024 * 1024, text)) S.restored = S.E.restore(dh::lines(text)); else S.restored = 0;
    if (readSmall(S.normPath, 1024 * 1024, text)) { std::vector<std::string> ls = dh::lines(text); if (ls.size() > 3000) ls.erase(ls.begin(), ls.end() - 3000); S.E.loadNormHistory(ls, session); }
    for (const auto& kv : S.PL.norms) S.E.slotNorm[kv.first] = kv.second;
}
void DealerHedge::flushRecord(DHState& S) {
    if (!S.recPath.empty() && !S.E.rows.empty()) {
        if (appendLines(S.recPath, S.E.rows, "# lsDealerHedge " DH_VERSION " record: Z = a zone's first decision (never changed), L = a live episode")) { S.recRows += (long long)S.E.rows.size(); S.E.rows.clear(); }
        else ++S.recFails;
    }
    if (!S.normPath.empty() && !S.E.normRows.empty()) { if (appendLines(S.normPath, S.E.normRows, "# lsDealerHedge norms: N|session|slot|buy adds|sell adds|buy fills|sell fills (per minute x100)|minutes")) S.E.normRows.clear(); }
}
// chart context only: open once (crash-guarded), drain in slices; every event -> the engine on the CT clock
void DealerHedge::feed(DHState& S, long long now) {
    if (S.blocked || S.dead) return;
    const std::string f = flexDir(); const std::string guard = f.empty() ? std::string() : f + "\\DealerHedge\\_opening-dbo-" + S.market + "-" + std::to_string(S.spb) + ".txt";
    if (!S.dbo && S.attempt < 2 && (S.attempt == 0 || (S.lastEventMs < 0 && S.openMs >= 0 && now - S.openMs >= FEED_WAIT_MS))) {
        if (S.attempt == 0 && !guard.empty() && fileStamp(guard) >= 0) { S.blocked = true; S.state = "IRT closed while opening the order feed last time: delete " + guard + " to retry"; return; }
        if (!guard.empty()) { makeDir(f); makeDir(f + "\\DealerHedge"); std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening DEPTH_BY_ORDER try " << (S.attempt + 1) << "\n"; }
        try { S.dbo = new (std::nothrow) DEPTH_BY_ORDER(S.attempt == 0 ? S.symbol.c_str() : NULL); } catch (...) { S.dead = true; ++S.faults; }
        S.via = S.attempt == 0 ? "chart symbol " + S.symbol : "default (NULL = this chart)"; S.attempt++; S.openMs = now;
        if (!guard.empty()) std::remove(guard.c_str());
        if (!S.dbo) { S.dead = true; S.state = "order feed unavailable"; return; }
    } else if (S.dbo && S.attempt == 1 && S.lastEventMs < 0 && now - S.openMs >= FEED_WAIT_MS) {
        delete S.dbo; S.dbo = nullptr; return;                        // one 2nd try with NULL on the next call
    }
    if (!S.dbo) return;
    RTARRAYI times(barDateTime); const long cnt = getBarCount();
    if (cnt > 0 && times.count >= cnt) S.barKey = (long long)times[(int)cnt - 1];
    const long long t0 = steadyMs(); int n = 0;
    for (;;) {
        if (n >= FEED_SLICE_MAX || ((n & 511) == 511 && steadyMs() - t0 > FEED_SLICE_MS)) { ++S.caps; break; }
        const int r = dhSafeNext(S.dbo);
        if (r < 0) { S.dead = true; ++S.faults; S.state = "order feed fault: reload the indicator"; break; }
        if (r == 0) break;
        ++n;
        const DEPTH_ORDER& o = S.dbo->dbo;
        if (o.timestamp != S.stampCache) { S.stampCache = o.timestamp; S.stampMs = ctSec(o.timestamp) * 1000LL; }
        if (S.stampMs <= 0 || !std::isfinite((double)o.price) || o.price <= 0) continue;
        const dh::Tick px = (dh::Tick)std::llround((double)o.price / S.tick);
        int side = o.buySell == 1 ? 0 : o.buySell == 0 ? 1 : -1;      // NEW / MOD / DEL: the order's side; FILL: the aggressor's side
        S.E.onEvent((int)o.action, side, px, (long long)o.size, (long long)o.orderID, S.stampMs);
        if (o.action == 3) S.E.trade(px);
        if ((n & 1023) == 1023) S.E.evaluate(S.stampMs, S.barKey);       // a backlog still decides second by second
    }
    try { S.flag = (S.dbo->*dhRobbed(DhDboFlag())) ? 1 : 0; } catch (...) { }
    if (n > 0) { S.lastEventMs = now; S.events += n; }
}
void DealerHedge::pump(bool fromDraw) {
    DHState* S = slot_.get(this, true);
    if (!S || S->busy) return;
    S->busy = true; struct Unbusy { DHState* s; ~Unbusy() { s->busy = false; } } ub = { S };
    try {
        if (!identify(*S)) return;
        const long long now = steadyMs();
        loadSettings(*S); loadParams(*S, now);
        if (!S->timerTried) { S->timerTried = true; S->timerOn = createTimer(dhTimerId(S), (int)TIMER_MS) == RTX_OK; }
        // the chart's bars: ATR(14) of the closed bars, the last price, the CT clock
        const long cnt = getBarCount();
        RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
        if (cnt < 3 || times.count < cnt || hi.count < cnt || lo.count < cnt || cl.count < cnt) { S->state = "waiting for bars"; writeStatus(*S, false); return; }
        const int need = S->E.P.atrLen + 1, from = std::max(0, (int)cnt - 1 - need);
        std::vector<double> H, L, C; for (int i = from; i < (int)cnt - 1; ++i) { H.push_back(hi[i]); L.push_back(lo[i]); C.push_back(cl[i]); }
        const double atr = dh::atrOf(H, L, C, S->E.P.atrLen);
        long long ctNow = ctSec(currentDate()) * 1000LL; const long long lastBar = ctSec((RTDATE)times[(int)cnt - 1]) * 1000LL;
        if (ctNow <= 0 || (lastBar > 0 && ctNow < lastBar - 86400000LL)) ctNow = lastBar;
        if (ctNow <= 0) { S->state = "no clock"; return; }
        const long long session = dh::sessionOf(ctNow);
        if (session != S->session) openSession(*S, session);
        loadLevels(*S, now);
        if (atr > 0) { S->atr = atr; S->E.setAtr(atr); }
        if (S->E.lastPx <= 0 || S->E.tally.events == 0) S->E.trade((dh::Tick)std::llround(cl[(int)cnt - 1] / S->tick));
        S->barKey = (long long)times[(int)cnt - 1];
        S->E.clock(ctNow);                                           // (0.1.1) the chart clock first: snapshot stamps are stale
        feed(*S, now);
        S->E.evaluate(std::max(ctNow, S->E.nowMs), S->barKey);
        flushRecord(*S);
        S->state = S->dead ? "order feed fault: reload the indicator" : S->blocked ? S->state : S->flag == 0 ? "MBO: not subscribed" : S->lastEventMs < 0 ? "MBO: waiting" : S->E.normSrc == "learning" ? "learning the normal (10 min)" : "ready";
        writeStatus(*S, false);
    } catch (...) { S->state = "error"; }
    (void)fromDraw;
}
void DealerHedge::writeStatus(DHState& S, bool force) {
    const long long now = steadyMs();
    if (!force && S.lastStatusMs >= 0 && now - S.lastStatusMs < 30000) return;
    S.lastStatusMs = now;
    const std::string f = flexDir(); if (f.empty() || S.market.empty()) return;
    int lit = 0, live = 0, watched = 0; for (const dh::Zone& z : S.E.zones) { if (z.state == dh::HIDDEN) ++watched; else ++lit; if (z.state == dh::LIVE) ++live; }
    std::ostringstream o;
    o << "VERSION," << DH_VERSION << "\n" << S.keepNote << "MARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nSTATE," << S.state
      << "\nSTATUS_LINE,oH " << DH_VERSION << " " << S.state << " | levels " << S.mqN << " + nodes " << S.nodeN << " | zones lit " << lit << " (live " << live << ") of " << (lit + watched)
      << " | ATR " << S.atr << " | normal " << S.E.normSrc << " | events " << S.events
      << "\nFEED," << (S.via.empty() ? "not opened" : S.via) << " flag " << S.flag << " events " << S.events << " slices capped " << S.caps << " faults " << S.faults
      << "\nLEVELS," << S.levelsWhy << " (" << S.mqN << ") | nodes " << S.nodesWhy << " (" << S.nodeN << ")\nPARAMS," << S.PL.source;
    for (const std::string& n : S.PL.notes) o << " | " << n;
    o << "\nSHOW," << S.E.P.show << (S.E.P.show == 0 ? " (LOG ONLY - recorded, nothing drawn)" : S.E.P.show == 1 ? " (drawn with ?)" : " (drawn)") << "\nTHRESHOLDS,width " << S.E.P.widthAtr << " ATR, flow " << S.E.P.flowX << "x, min lots " << S.E.P.minLots << ", repeats " << S.E.P.repN << " (cv < " << S.E.P.repCv << ", >= " << S.E.P.repMinLots << " lots), refills " << S.E.P.refN << ", burst " << S.E.P.burstX << "x, window " << S.E.P.winS << " s, live hold " << S.E.P.liveHoldS << " s"
      << "\nNORMAL," << S.E.normSrc << " (" << S.E.slotNorm.size() << " slot rows)\nRECORD," << S.recPath << " rows " << S.recRows << " fails " << S.recFails << " restored " << S.restored
      << "\nPULSE," << (S.timerOn ? "timer 250 ms" : "timer refused: pulses on redraws only") << " ticks " << S.timerTicks << " redraws asked " << S.redrawsAsked
      << "\nDECIDED,buy " << S.E.tally.decided[0] << " sell " << S.E.tally.decided[1] << " episodes " << S.E.tally.episodes << " paths a " << S.E.tally.pathA << " b " << S.E.tally.pathB << " c " << S.E.tally.pathC << "\n";
    for (const dh::Zone& z : S.E.zones) if (z.state != dh::HIDDEN)
        o << "ZONE," << z.name << " " << dh::priceText(z.lvl * S.tick, S.tick) << " " << (z.side == 0 ? "BUY" : "SELL") << " " << dh::clock12(z.firstMs) << " " << (z.state == dh::LIVE ? "LIVE" : "settled") << " flow " << dh::ratioText(z.flowX) << " paths " << z.paths << "\n";
    const std::string text = o.str(); if (!force && text == S.lastStatus) return;
    makeDir(f);
    if (publishFile(f + "\\DealerHedge.status-" + S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb) + ".txt", text)) S.lastStatus = text;
}
// the bar index whose time is t, -1 = not on the chart
template <class Times> static int indexOf(Times& times, long count, long long t) {
    long lo = 0, hi = count - 1;
    while (lo <= hi) { const long mid = lo + (hi - lo) / 2; const long long v = (long long)times[(int)mid]; if (v == t) return (int)mid; if (v < t) lo = mid + 1; else hi = mid - 1; }
    return -1;
}
void DealerHedge::render(DHState& S) {
    S.targets.clear();
    if (S.E.zones.empty() || S.E.P.show == 0) return;                    // (0.2.0) PARAM,SHOW 0 = LOG ONLY: recorded, nothing drawn, no hover
    RCT pane; pane.getPaneRect(false);
    const long count = getBarCount(); if (count < 2 || pane.bottom - pane.top < 40) return;
    RTARRAYI times(barDateTime); RTARRAY lo(barLow); if (times.count < count || lo.count < count) return;
    RCT scl; scl.getScaleRect(); int edge = pane.right; if (scl.left > pane.left && scl.left < edge) edge = scl.left - 1;
    PNT last; last.set((int)count - 1, 0.0f, kBarRight); if (last.h > pane.left && last.h < edge) edge = last.h;
    const unsigned long bg = (unsigned long)getBackgroundColor() & 0xFFFFFFul;
    const long long wms = wallMs();
    const unsigned long col[2] = { S.set.c(dhset::K_BUY_COLOR), S.set.c(dhset::K_SELL_COLOR) };
    const bool showZ = S.set.b(dhset::K_SHOW_ZONES), pulseOn = S.set.b(dhset::K_PULSE), showL = S.set.b(dhset::K_SHOW_LABELS);
    std::map<long long, int> labelSide;                     // one "oH" per candle: the first zone decided on it
    for (const dh::Zone& z : S.E.zones) {
        if (z.state == dh::HIDDEN) continue;
        const int bi = indexOf(times, count, z.barKey);
        PNT a; a.set(bi >= 0 ? bi : 0, (float)(z.hi * S.tick + S.tick / 2), kBarLeft);
        PNT b; b.set(bi >= 0 ? bi : 0, (float)(z.lo * S.tick - S.tick / 2), kBarLeft);
        int x0 = bi >= 0 ? a.h : pane.left; if (x0 > edge - 16) x0 = edge - 16;                // a zone decided on the last candle: a 16 px stub
        const int y0 = std::min(a.v, b.v), y1 = std::max(a.v, b.v);
        if (y1 < pane.top || y0 > pane.bottom) continue;
        HoverT H; H.key = z.key; H.l = x0; H.t = y0; H.r = edge; H.b = y1;
        if (showZ) {
            const int sd = z.state == dh::LIVE ? (z.liveSide == 1 ? 1 : 0) : (z.side == 1 ? 1 : 0);
            const double k = dh::intensity(z.state, wms, pulseOn, S.set.i(dhset::K_FAINT_PCT));
            const COLOR fc = (COLOR)dh::blend(col[sd], bg, k);
            setPen(fc, 1, P_SOLID);
            RCT r; r.set(coord(x0), coord(std::max(y0, (int)pane.top)), coord(edge), coord(std::min(y1, (int)pane.bottom)));
            ko::fill(r, (short)0, fc, fc, DRAW_TRANSLUCENT, PAT_SOLID);
            S.targets.push_back(H);
        }
        if (bi >= 0 && !labelSide.count(z.barKey)) labelSide[z.barKey] = z.side;
    }
    if (showL) {
        FONT f; f.id = HELVETICA; f.size = 10; f.style = BOLD; setFont(f);
        for (const auto& kv : labelSide) {
            const int bi = indexOf(times, count, kv.first); if (bi < 0) continue;
            PNT p; p.set(bi, lo[bi], kBarCenter); if (p.h < pane.left || p.h > edge) continue;
            const char* lab = S.E.P.show == 1 ? "oH?" : "oH";   // (0.2.0) SHOW 1 = drawn with "?"
            const int w = getTextWidth(lab, -1) + 4, t = p.v + 4;
            RCT r; r.set(coord(p.h - w / 2), coord(t), coord(p.h + w / 2 + 1), coord(t + 14));
            if (r.bottom > pane.bottom) continue;
            setTextColor((COLOR)col[kv.second == 1 ? 1 : 0]); ko::text(r, lab, true, false);
            for (const dh::Zone& z : S.E.zones) if (z.state != dh::HIDDEN && z.barKey == kv.first) { HoverT H; H.key = z.key; H.l = r.left; H.t = r.top; H.r = r.right; H.b = r.bottom; S.targets.insert(S.targets.begin(), H); break; }
        }
    }
    // the hover: only while the cursor is over this chart and over a band / label
    if (!S.set.b(dhset::K_SHOW_HOVER) || !S.msHas || S.msWnd == 0 || hb::hbCursor() != S.msWnd) { S.hovKey.clear(); return; }
    const HoverT* T = nullptr; for (const HoverT& t : S.targets) if (S.msH >= t.l && S.msH <= t.r && S.msV >= t.t && S.msV <= t.b) { T = &t; break; }
    S.hovKey = T ? T->key : std::string(); if (!T) return;
    const dh::Zone* Z = nullptr; for (const dh::Zone& z : S.E.zones) if (z.key == T->key && z.state != dh::HIDDEN) Z = &z;
    if (!Z) return;
    const std::vector<dh::HoverLine> L = dh::hover(*Z, S.market, S.tick, S.E.nowMs, S.E.lastPx * S.tick);
    FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
    int w = 0; for (const auto& l : L) w = std::max(w, getTextWidth(l.text.c_str(), -1));
    w += 16; const int lh = 15, h = (int)L.size() * lh + 10;
    int l = S.msH + 14, t = S.msV + 14;
    if (l + w > edge - 2) l = S.msH - 14 - w;
    if (l < pane.left + 2) l = pane.left + 2;
    if (t + h > pane.bottom - 2) t = pane.bottom - 2 - h;
    if (t < pane.top + 2) t = pane.top + 2;
    setPen(0x009CA3AF, 1, P_SOLID);
    RCT bgr; bgr.set(coord(l), coord(t), coord(l + w), coord(t + h)); bgr.draw(1, 0x009CA3AF, 0x00111827, DRAW_OPAQUE, PAT_SOLID);
    for (size_t i = 0; i < L.size(); ++i) {
        const int tone = L[i].tone;
        setTextColor(tone == 0 ? (COLOR)col[Z->side == 1 ? 1 : 0] : tone == 2 ? (COLOR)col[0] : tone == 3 ? (COLOR)col[1] : tone == 4 ? (COLOR)0x009CA3AF : (COLOR)0x00F3F4F6);
        RCT tr; tr.set(coord(l + 8), coord(t + 5 + (int)i * lh), coord(l + w - 4), coord(t + 5 + (int)(i + 1) * lh)); tr.drawText(L[i].text.c_str(), false, false);
    }
}
// the mouse: remember where it is; redraw only when the hovered band / label changes. Never swallows the event.
int DealerHedge::mouse(RTX_EVENT* e) {
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    DHState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    S->msHas = true; S->msH = e->v.mouse.h; S->msV = e->v.mouse.v; S->msWnd = hb::hbCursor();
    std::string k; for (const HoverT& t : S->targets) if (S->msH >= t.l && S->msH <= t.r && S->msV >= t.t && S->msV <= t.b) { k = t.key; break; }
    if (k != S->hovKey) { S->hovKey = k; invalidateChart(false); }
    return RTX_FAIL;
}

extern "C" cppExtension* CreateExtension(void) {
    DealerHedge* p = new DealerHedge();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE);
    p->setExtendedFlags(CALL_CONTINUOUSLY);                  // the order feed is drained while no trade prints
    p->setDescription("LRA Dealer Hedge (oH) v0.2.0 (LOG ONLY by default: the nightly test found no edge, so zones are recorded but not drawn until LRA-DealerHedge-<MKT>.csv PARAM,SHOW is 1 or 2): are dealers hedging in the futures book at a gamma level? Each MenthorQ level near price (CR / PS / HVL / 0DTE / gamma walls / G levels, + CME gamma nodes) gets a zone of +/- 0.5 ATR, invisible until the order book shows programmatic flow inside it (2x normal + same-size repeats, refills or a burst). It then pulses green (dealers buying) or red (selling) while live and settles to a faint band - never removed. oH below the candle where it was first seen; hover for the details. ? = not proven yet.");
    p->setVersion("0.2.0");   // DH_VERSION
    return p;
}
