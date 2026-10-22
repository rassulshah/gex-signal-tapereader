/********************************************************************************
 *  ReversalStage.cpp  --  Investor/RT RTX extension  lsReversalStage  1.0.0  (2026-10-22)
 *  Rassul approved layout-stages-2strips.png (11:41-11:46 CT, MBO-UI-LAYOUT-PLAN.md option A + the stage view), build ordered 15:20 CT.
 *
 *  "WHICH STAGE IS THE PIVOT IN?" - its own pane. A STAIRCASE per closed candle for the current LOCAL PIVOT ATTEMPT (a 5-candle low /
 *  high): 1 Attack, 2 Absorbed, 3 Exhausted, 4 Turn, 5 Re-test, 6 Expansion (stage names at the right of the last candle). Green steps =
 *  a LOW pivot being built, red = a HIGH pivot; dim = stage 1 only; a step dropping back to a short stub = the attempt failed.
 *  The steps come from the letters the two order-flow panes recorded (LetterStrips.h record, natively computed in IRT):
 *    lsFlexLevels\Strips\ATK-<MKT>-<spb>.csv (lsBidAskStats 1.28.0: S K L R Tr G), DEF-<MKT>-<spb>.csv (lsBookFlow 1.23.0: A W P B H L X),
 *    QUEUE-<MKT>-<spb>.csv (the queue meter's Q, when that record exists).
 *  The rules: ReversalStageLogic.h (SDK-free, unit-tested). Each candle is decided ONCE when it is closed and settled (a later candle has
 *  closed, or 20 s after its close) and appended to lsFlexLevels\ReversalStage\RS-<MKT>-<spb>.csv (first line per candle wins, another
 *  chart's line is adopted); the pane draws only from that record - never repainted, the same after a restart; the nightly reads it.
 *  The NOW box (9-spot Position): "Now 12:42 PM", the attempt (LOW / HIGH), the stage, the chain ("K A . Tr"), "Odds ?  (T-20261022-39)"
 *  - the stage odds are not proven yet. Hover a step: the stage, the letters behind it and their numbers, use, example.
 *  Settings (ReversalStageSettings.h): Steps (show, Low pivot / High pivot colours), Stage names, Now box (show, Position).
 *  Status: lsFlexLevels\ReversalStage\status-<MKT>-<spb>.txt.
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
#include "ReversalStageLogic.h"
#include "HostSlot.h"
#include "SettingsGuard.h"
#include "ReversalStageSettings.h"
#include "SettingsHost.h"
#include "NewsWindow.h"          // (1.1.0) news / settle window "?" + the feed check

#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32) && defined(_MSC_VER)
#pragma comment(lib, "user32.lib")   // GetCursorPos / WindowFromPoint (the hover stops when the cursor leaves the pane)
#endif

#define RS_VERSION "1.1.0"   // 1.1.0 (2026-10-22 evening, Rassul approved 17:39 CT): a step decided inside a news / settle window gets a "?" over its column; a failed nightly feed check (LRA-FeedCheck-<MKT>.csv) = recorded only, not drawn (NewsWindow.h)
// 1.0.0:   // keep equal to rsl::VERSION, rsset::PLUGIN_VERSION and the setVersion literal below

static const COLOR C_GRID = 0x00374151, C_LABEL = 0x009CA3AF, C_TEXT = 0x00E5E7EB, C_BOX_BG = 0x000F1520, C_TIP_BG = 0x00111827;
static const long long SETTLE_SEC = 20;      // a candle is decided 20 s after its close (or once the next candle has closed)
static const int WINDOW = 600, WARM = 40;    // the staircase is re-run over the last 600 closed candles; the first 40 only warm it up

#ifdef RS_TEST_CLOCK
extern long long rs_test_now_ms; extern void* rs_test_cursor_wnd; extern int rs_test_cx, rs_test_cy; extern bool rs_test_cursor_ok;
static long long steadyMs() { return rs_test_now_ms; }
static void* cursorWindow() { return rs_test_cursor_wnd; }
static bool cursorPos(int& x, int& y) { x = rs_test_cx; y = rs_test_cy; return rs_test_cursor_ok; }
#else
static long long steadyMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
#if defined(_WIN32)
static void* cursorWindow() { POINT p; if (!GetCursorPos(&p)) return NULL; return (void*)WindowFromPoint(p); }
static bool cursorPos(int& x, int& y) { POINT p; if (!GetCursorPos(&p)) return false; x = (int)p.x; y = (int)p.y; return true; }
#else
static void* cursorWindow() { return NULL; }
static bool cursorPos(int& x, int& y) { x = y = 0; return false; }
#endif
#endif
static short coord(long v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
static std::string flexDir() { const char* up = std::getenv("USERPROFILE"); return (up && up[0]) ? std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string(); }
static void makeDir(const std::string& p) {
#if defined(_WIN32)
    if (!p.empty()) CreateDirectoryA(p.c_str(), NULL);
#else
    (void)p;
#endif
}
static std::string settingsPath(const std::string& mkt) { const std::string f = flexDir(); return f.empty() || mkt.empty() ? std::string() : f + "\\ReversalStage.settings-" + mkt + ".txt"; }
// the new whole lines of path from off0 (bounded to the newest maxRead bytes; a shorter file = read again from the start)
template <class Fn> static bool tailRead(const std::string& path, long long& off0, Fn fn, long long maxRead = 4LL * 1024 * 1024) {
    if (path.empty()) return false;
    std::ifstream in(path.c_str(), std::ios::binary); if (!in.is_open()) return false;
    in.seekg(0, std::ios::end); const long long size = (long long)in.tellg(); if (size < 0) return false;
    if (size < off0) off0 = 0;
    if (size == off0) return false;
    if (size - off0 > maxRead) off0 = size - maxRead;
    in.seekg(off0, std::ios::beg); std::string buf((size_t)(size - off0), '\0'); in.read(&buf[0], (std::streamsize)buf.size());
    if (!in && in.gcount() != (std::streamsize)buf.size()) return false;
    const size_t last = buf.rfind('\n'); if (last == std::string::npos) return false;
    off0 += (long long)last + 1;
    size_t a = 0;
    while (a <= last) { const size_t b = buf.find('\n', a); std::string ln = buf.substr(a, b - a); a = b + 1; if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (!ln.empty()) fn(ln); }
    return true;
}

struct RSState {
    std::recursive_mutex mutex;
    std::string identity, market, symbol; int spb = 0; double tick = 0; bool fault = false; std::string state = "starting";
    ss::Store setStore; ss::Values set = ss::defaults(rsset::spec(), -1);
    // the letters (LetterStrips record lines), first line per id wins
    std::multimap<long long, rsl::Ev> evs; std::set<std::string> evIds; long long offA = 0, offD = 0, offQ = 0, letBad = 0;
    std::string pathA, pathD, pathQ;
    // the record: decided candles [raw key] -> step; the newest key
    std::map<long long, rsl::Step> steps; long long recOff = 0, recBad = 0, written = 0, writeFail = 0, adopted = 0; std::string recPath; bool dirMade = false;
    long long lastPassMs = -1; long lastCount = -1; long long lastLetters = -1;
    rsl::Step now; bool haveNow = false;
    // drawing + hover
    struct Hit { int l = 0, t = 0, r = 0, b = 0; long long key = 0; };
    std::vector<Hit> hits; long long hoverKey = 0; void* hoverWnd = nullptr; int dx = 0, dy = 0; bool haveWnd = false;
    int paneL = 0, paneT = 0, paneR = 0, paneB = 0; int boxL = 0, boxT = 0, boxR = 0, boxB = 0; bool boxOn = false;
    long long drawn = 0, statusMs = -1; std::string statusText;
};

static ss::Ids& rsIds() { static ss::Ids I; return I; }

class ReversalStage : public cppExtension {
public:
    SS_SAFE_DIALOG(ReversalStage, "ReversalStage")
    ReversalStage() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int scale(int, int, double* dMin, double* dMax) { if (dMin) *dMin = 0; if (dMax) *dMax = rsl::AXIS_MAX; return RTX_OK; }   // the pane's own axis: stages 0 - 6
    virtual int parmsTitle(char* pStr, int size) { if (!pStr || size < 2) return RTX_FAIL; std::snprintf(pStr, (size_t)size, "Reversal stage v%s", RS_VERSION); return RTX_OK; }
    virtual int parmsLoadBody(void) {
        if (!ss::inDialog(this)) return RTX_OK;
        ss::Store* st = dialogStore(); if (!st) return RTX_OK;
        ss::fillDialog(this, rsset::spec(), rsIds(), st->load(ss::marketIndex(dlgMkt_), steadyMs(), true), snap_);
        return RTX_OK;
    }
    virtual int parmsApplyBody(void) { saveSettings(); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::onUpdate(this, rsset::spec(), rsIds(), (int)i, snap_); saveSettings(); return RTX_OK; }
    void pump();
    void release() { slot_.release(this); }
    RSState* stateFor() { return slot_.get(this, false); }   // tests
private:
    HostSlot<RSState> slot_;
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        char b[64]; std::memset(b, 0, sizeof(b)); const char* rs = getRootSymbol(b); const std::string mkt = lst::marketName(rs ? rs : "");
        const std::string p = settingsPath(mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &rsset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, rsset::spec(), rsIds(), st->load(ss::marketIndex(dlgMkt_), steadyMs(), true), snap_);
        makeDir(flexDir());
        if (st->save(V)) snap_.take(ss::dialogNow(this, rsset::spec(), rsIds(), V));
    }
    void identify(RSState& s);
    void readLetters(RSState& s);
    void readRecord(RSState& s);
    void decide(RSState& s);
    void writeStatus(RSState& s, long long now);
    long long civil(long long raw);
    std::string clockOf(long long raw);
    void render(RSState& s);
    void tip(RSState& s, int top, int bottom, int left, int right);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { static_cast<ReversalStage*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<ReversalStage*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) { ss::registerAll(this, rsset::spec(), rsIds()); return RTX_OK; }
int cppExtension::calc(int) { static_cast<ReversalStage*>(this)->pump(); return RTX_OK; }

long long ReversalStage::civil(long long raw) {
    struct tm x; std::memset(&x, 0, sizeof(x)); if (raw <= 0 || !getLocaltime((RTDATE)raw, &x)) return LLONG_MIN;
    const int y = x.tm_year + 1900 - (x.tm_mon < 2 ? 1 : 0); const int era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned mp = (unsigned)((x.tm_mon + 1 + 9) % 12), doy = (153 * mp + 2) / 5 + (unsigned)x.tm_mday - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = (long long)era * 146097 + (long long)doe - 719468;
    return days * 86400 + x.tm_hour * 3600 + x.tm_min * 60 + x.tm_sec;
}
std::string ReversalStage::clockOf(long long raw) {
    struct tm x; std::memset(&x, 0, sizeof(x)); if (raw <= 0 || !getLocaltime((RTDATE)raw, &x)) return std::string();
    char b[16]; std::snprintf(b, sizeof b, "%d:%02d %s", x.tm_hour % 12 == 0 ? 12 : x.tm_hour % 12, x.tm_min, x.tm_hour < 12 ? "AM" : "PM"); return b;
}

void ReversalStage::identify(RSState& s) {
    char rb[64]; std::memset(rb, 0, sizeof(rb)); const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar(); const double tick = (double)getProperty(SYM_TICKINCR);
    const std::string id = root + "|" + sym + "|" + std::to_string(spb) + "|" + std::to_string(tick);
    if (id == s.identity) return;
    s.identity = id; s.symbol = sym; s.spb = spb; s.tick = tick; s.fault = false; s.market = lst::marketName(root.empty() ? sym : root);
    s.evs.clear(); s.evIds.clear(); s.offA = s.offD = s.offQ = 0; s.letBad = 0; s.steps.clear(); s.recOff = 0; s.recBad = 0; s.recPath.clear(); s.dirMade = false;
    s.lastPassMs = -1; s.lastCount = -1; s.lastLetters = -1; s.haveNow = false; s.hoverKey = 0; s.hits.clear(); s.statusText.clear();
    if (!(std::isfinite(tick) && tick > 0 && tick < 1000)) { s.fault = true; s.state = "NO TICK SIZE"; return; }
    if (spb <= 0 || spb > 3600) { s.fault = true; s.state = "NEEDS A TIME CHART (1 s - 60 min)"; return; }
    s.state = "running";
}
void ReversalStage::readLetters(RSState& s) {
    const std::string f = flexDir(); if (f.empty()) return;
    s.pathA = f + "\\Strips\\" + lst::fileName(lst::ATK, s.market, s.spb);
    s.pathD = f + "\\Strips\\" + lst::fileName(lst::DEF, s.market, s.spb);
    s.pathQ = f + "\\Strips\\" + lst::fileName(lst::QUE, s.market, s.spb);
    auto add = [&](const std::string& ln, int want) {
        lst::Rec r; if (!lst::parse(ln, r) || r.pane != want) { ++s.letBad; return; }
        const std::string id = lst::idOf(r.pane, r.key, r.l.code, r.l.pivot); if (s.evIds.count(id)) return;
        s.evIds.insert(id); rsl::Ev e; e.key = r.key; e.pane = r.pane; e.l = r.l; s.evs.insert(std::make_pair(r.key, e));
    };
    tailRead(s.pathA, s.offA, [&](const std::string& ln) { add(ln, lst::ATK); });
    tailRead(s.pathD, s.offD, [&](const std::string& ln) { add(ln, lst::DEF); });
    tailRead(s.pathQ, s.offQ, [&](const std::string& ln) { add(ln, lst::QUE); });
    while (s.evs.size() > 40000) { const long long k = s.evs.begin()->first; s.evs.erase(k); }
    if (s.evIds.size() > 60000) s.evIds.clear();   // ids only guard duplicates of lines already read; the offsets keep the reads incremental
}
void ReversalStage::readRecord(RSState& s) {
    const std::string f = flexDir(); if (f.empty()) return;
    const std::string d = f + "\\ReversalStage";
    if (!s.dirMade) { makeDir(f); makeDir(d); s.dirMade = true; }
    const std::string p = d + "\\" + rsl::fileName(s.market, s.spb);
    if (p != s.recPath) { s.recPath = p; s.recOff = 0; s.steps.clear(); }
    tailRead(p, s.recOff, [&](const std::string& ln) { rsl::Step st; if (!rsl::parse(ln, st)) { ++s.recBad; return; } if (!s.steps.count(st.key)) { s.steps[st.key] = st; ++s.adopted; } });
    while (s.steps.size() > 20000) s.steps.erase(s.steps.begin());
}
// decide every closed, settled candle of the window that is not in the record (the staircase re-run from the closed candles - lesson 136)
void ReversalStage::decide(RSState& s) {
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    const long count = getBarCount();
    const int n = (int)std::min<long>(count, std::min(std::min((long)times.count, (long)hi.count), std::min((long)lo.count, (long)cl.count)));
    if (n < 2) return;
    const int last = n - 1, from = std::max(0, last - WINDOW);
    std::vector<rsl::Bar> B; B.reserve((size_t)(last - from));
    for (int i = from; i < last; ++i) {
        rsl::Bar b; b.key = (long long)times[i];
        const double h = (double)hi[i], l = (double)lo[i], c = (double)cl[i];
        b.ok = b.key > 0 && std::isfinite(h) && std::isfinite(l) && std::isfinite(c) && h >= l;
        if (b.ok) { b.h = (long long)std::floor(h / s.tick + 0.5); b.l = (long long)std::floor(l / s.tick + 0.5); b.c = (long long)std::floor(c / s.tick + 0.5); }
        B.push_back(b);
    }
    if (B.empty()) return;
    const std::vector<rsl::Step> st = rsl::run(B, s.evs);
    const long long nowC = civil((long long)currentDate());
    long long newest = 0; for (std::map<long long, rsl::Step>::const_reverse_iterator it = s.steps.rbegin(); it != s.steps.rend(); ++it) { newest = it->first; break; }
    std::string out; std::vector<rsl::Step> add;
    for (size_t k = (from == 0 ? 0 : (size_t)WARM); k < st.size(); ++k) {
        const long long key = st[k].key; if (key <= 0 || s.steps.count(key) || key <= newest) continue;
        const bool settled = k + 1 < st.size() || (nowC != LLONG_MIN && civil(key) != LLONG_MIN && nowC >= civil(key) + SETTLE_SEC);
        if (!settled) break;
        out += rsl::line(st[k]) + "\n"; add.push_back(st[k]);
    }
    if (add.empty() || s.recPath.empty()) return;
    std::ofstream o(s.recPath.c_str(), std::ios::binary | std::ios::app); if (o) { o.write(out.data(), (std::streamsize)out.size()); o.flush(); }
    if (o) { for (size_t k = 0; k < add.size(); ++k) s.steps[add[k].key] = add[k]; s.written += (long long)add.size(); }   // read back later = the same lines (first wins)
    else ++s.writeFail;
}
void ReversalStage::writeStatus(RSState& s, long long now) {
    const std::string f = flexDir(); if (f.empty() || s.market.empty()) return;
    std::ostringstream o;
    o << "VERSION," << RS_VERSION << "\nMARKET," << s.market << "\nSYMBOL," << s.symbol << "\nSPB," << s.spb << "\nSTATE," << s.state
      << "\nLETTERS," << s.evs.size() << " read (" << s.letBad << " bad lines), files " << s.pathA << " | " << s.pathD << " | " << s.pathQ
      << "\nRECORD," << s.steps.size() << " candles (" << s.written << " written here, " << s.writeFail << " write fails, " << s.recBad << " bad lines), file " << s.recPath
      << "\nNOW," << (s.haveNow ? rsl::sideWord(s.now.side) + " stage " + std::to_string(s.now.stage) + " " + rsl::statusName(s.now.status) + " chain " + s.now.chain : std::string("none"))
      << "\nDRAWN," << s.drawn << "\nODDS,? not proven yet (" << rsl::ODDS_TICKET << ")\n";
    const std::string t = o.str();
    if (t == s.statusText && s.statusMs >= 0 && now - s.statusMs < 30000) return;
    s.statusMs = now;
    const std::string d = f + "\\ReversalStage", p = d + "\\status-" + s.market + "-" + std::to_string(s.spb) + ".txt", tmp = p + ".tmp";
    makeDir(f); makeDir(d);
    { std::ofstream w(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!w) return; w << t; w.flush(); if (!w) return; }
    std::remove(p.c_str()); if (std::rename(tmp.c_str(), p.c_str()) == 0) s.statusText = t;
}
void ReversalStage::pump() {
    RSState* s = slot_.get(this, true); if (!s) return;
    std::unique_lock<std::recursive_mutex> lock(s->mutex, std::try_to_lock); if (!lock.owns_lock()) return;
    try {
        identify(*s);
        { const std::string p = settingsPath(s->market); if (p.empty()) s->set = ss::defaults(rsset::spec(), -1);
          else { if (s->setStore.path != p) { s->setStore = ss::Store(); s->setStore.S = &rsset::spec(); s->setStore.path = p; } s->set = s->setStore.load(ss::marketIndex(s->market), steadyMs()); } }
        if (s->fault) return;
        const long long now = steadyMs(); const long count = getBarCount();
        if (s->lastPassMs >= 0 && now >= s->lastPassMs && now - s->lastPassMs < 1000 && count == s->lastCount) return;   // at most once a second unless a bar closed
        s->lastPassMs = now; s->lastCount = count;
        readLetters(*s); readRecord(*s); decide(*s);
        if (!s->steps.empty()) { s->now = s->steps.rbegin()->second; s->haveNow = true; }
        writeStatus(*s, now);
    } catch (...) { s->fault = true; s->state = "STOPPED - error, reload the chart"; }
}

int ReversalStage::mouse(RTX_EVENT* e) {
    if (!e) return RTX_FAIL;
    RSState* s = slot_.get(this, false); if (!s) return RTX_FAIL;
    if (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER) return RTX_FAIL;
    std::unique_lock<std::recursive_mutex> lock(s->mutex, std::try_to_lock); if (!lock.owns_lock()) return RTX_FAIL;
    const int h = e->v.mouse.h, v = e->v.mouse.v;
    { int cx = 0, cy = 0; void* w = cursorWindow(); if (w && cursorPos(cx, cy)) { s->hoverWnd = w; s->dx = cx - h; s->dy = cy - v; s->haveWnd = true; } else s->haveWnd = false; }
    long long key = 0;
    for (size_t k = 0; k < s->hits.size() && !key; ++k) { const RSState::Hit& x = s->hits[k]; if (h >= x.l && h < x.r && v >= x.t && v < x.b) key = x.key; }
    if (key != s->hoverKey) { s->hoverKey = key; invalidateChart(false); }
    return RTX_FAIL;                                                       // never swallow the event
}

int ReversalStage::draw(void) {
    RSState* s = slot_.get(this, false); if (!s) return RTX_OK;
    std::unique_lock<std::recursive_mutex> lock(s->mutex, std::try_to_lock); if (!lock.owns_lock()) return RTX_OK;
    try { render(*s); } catch (...) {}
    return RTX_OK;
}

void ReversalStage::render(RSState& s) {
    using namespace rsset;
    const ss::Values& V = s.set;
    RCT pane; pane.getPaneRect(false);
    s.paneL = pane.left; s.paneT = pane.top; s.paneR = pane.right; s.paneB = pane.bottom;
    s.hits.clear(); s.boxOn = false;
    const long count = getBarCount();
    if (count < 2 || pane.bottom - pane.top < 30 || pane.right - pane.left < 60) return;
    RTARRAYI times(barDateTime); if (times.count < count) return;
    const int last = (int)count - 1;
    auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
    int v0 = 0, v1 = last;
    if (getVisibleBars(&v0, &v1) != RTX_OK || v0 < 0 || v1 < v0 || v1 > last) { v1 = last; v0 = last; while (v0 > 0 && xOf(v0 - 1) >= pane.left) --v0; }
    int slot = (int)getPixelsPerBar(); if (slot <= 0 || slot > 400) slot = v1 > v0 ? std::max(2, std::abs(xOf(v1) - xOf(v1 - 1))) : 8;
    const int top = pane.top + 2, bot = pane.bottom - 2;
    const int lastR = std::min((int)pane.right, xOf(last) + slot / 2 + 2);  // the layout ends at the last candle
    // the faint stage lines and the stage names right of the last candle (full names when there is room, else the numbers)
    FONT f; f.id = HELVETICA; f.size = 8; f.style = PLAIN; setFont(f);
    for (int st = 1; st <= 6; ++st) { const int y = rsl::yOf(st, top, bot); setPen(C_GRID, 1, P_SOLID); RCT g; g.set(coord(pane.left), coord(y), coord(lastR), coord(y)); g.draw(0, C_GRID, C_GRID, DRAW_OPAQUE, PAT_SOLID); }
    if (V.b(K_SHOW_NAMES)) {
        const int room = pane.right - lastR - 4; int wFull = 0; for (int st = 1; st <= 6; ++st) { char b[24]; std::snprintf(b, sizeof b, "%d %s", st, rsl::stageName(st)); wFull = std::max(wFull, getTextWidth(b, -1)); }
        const bool full = room >= wFull + 2, digits = room >= 10;
        setTextColor(C_LABEL);
        if (full || digits) for (int st = 1; st <= 6; ++st) { char b[24]; if (full) std::snprintf(b, sizeof b, "%d %s", st, rsl::stageName(st)); else std::snprintf(b, sizeof b, "%d", st);
            const int y = rsl::yOf(st, top, bot); RCT r; r.set(coord(lastR + 4), coord(y - 6), coord(pane.right - 1), coord(y + 6)); r.drawText(b, false, false); }
    }
    // the steps, from the record only (closed, decided candles)
    if (V.b(K_SHOW_STEPS)) {
        const int w = std::max(2, std::min(slot - 2, (int)std::lround(slot * 0.7)));
        for (int i = std::max(0, v0); i <= v1 && i < last; ++i) {
            std::map<long long, rsl::Step>::const_iterator it = s.steps.find((long long)times[i]); if (it == s.steps.end()) continue;
            const rsl::Look k = rsl::lookOf(it->second); if (k.height <= 0 && !k.stub) continue;
            const int xc = xOf(i); if (xc < pane.left || xc > lastR) continue;
            nw::Gate ng; { struct tm tk; std::memset(&tk, 0, sizeof(tk)); if (getLocaltime((RTDATE)times[i], &tk)) ng = nw::atTm(s.market, tk); }   // (1.1.0)
            if (ng.feedBad) continue;                                      // (1.1.0) feed check failed: recorded only
            COLOR c = (COLOR)(k.colour == lst::GREEN ? V.c(K_COL_LOW) : V.c(K_COL_HIGH));
            if (k.dim) c = (COLOR)((((c >> 16) & 0xFF) * 55 / 100) << 16 | (((c >> 8) & 0xFF) * 55 / 100) << 8 | ((c & 0xFF) * 55 / 100));   // stage 1 / a failed stub: the same hue, darker
            const int yTop = k.stub ? bot - std::max(3, (bot - top) / 26) : rsl::yOf(k.height, top, bot);
            setPen(c, 1, P_SOLID); RCT r; r.set(coord(xc - w / 2), coord(yTop), coord(xc - w / 2 + w), coord(bot)); r.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID);
            if (ng.news && yTop - 13 > top) { setTextColor(c); RCT qr; qr.set(coord(xc - 6), coord(yTop - 13), coord(xc + 7), coord(yTop - 1)); qr.drawText("?", true, false); setFont(f); }   // (1.1.0) news / settle window
            RSState::Hit h; h.l = xc - std::max(w, slot) / 2; h.r = h.l + std::max(w, slot); h.t = top; h.b = bot + 1; h.key = it->first; s.hits.push_back(h);
            ++s.drawn;
        }
    }
    // the Now box
    if (V.b(K_SHOW_NOW)) {
        const std::vector<std::string> rows = rsl::nowRows(s.haveNow ? s.now : rsl::Step(), s.haveNow ? clockOf(s.now.key) : std::string("-"));
        FONT fb = f; fb.style = BOLD; fb.size = 9; FONT fp = fb; fp.style = PLAIN;
        const int lh = 13, bw = 168, bh = (int)rows.size() * lh + 8; int x = 0, y = 0;
        if (rsl::boxAt(V.i(K_POSITION), pane.left, pane.top, lastR, pane.bottom, bw, bh, 4, x, y)) {
            setPen(C_LABEL, 1, P_SOLID); RCT b; b.set(coord(x), coord(y), coord(x + bw), coord(y + bh)); b.draw(1, C_LABEL, C_BOX_BG, DRAW_OPAQUE, PAT_SOLID);
            for (size_t k = 0; k < rows.size(); ++k) {
                setFont(k == 0 ? fb : fp);
                COLOR c = C_TEXT;
                if (k == 1 && s.haveNow && s.now.side >= 0 && rows[k] != "No attempt") c = (COLOR)(s.now.side == rsl::LOW ? V.c(K_COL_LOW) : V.c(K_COL_HIGH));
                if (k + 1 == rows.size()) c = C_LABEL;
                setTextColor(c); RCT r; r.set(coord(x + 6), coord(y + 4 + (int)k * lh), coord(x + bw - 4), coord(y + 4 + (int)(k + 1) * lh)); r.drawText(rows[k].c_str(), false, false);
            }
            s.boxL = x; s.boxT = y; s.boxR = x + bw; s.boxB = y + bh; s.boxOn = true;
            for (size_t k = 0; k < s.hits.size();) { const RSState::Hit& h = s.hits[k]; if (h.l < s.boxR && h.r > s.boxL) s.hits.erase(s.hits.begin() + (long)k); else ++k; }   // no hover under the box
        }
    }
    // the hover: only while the cursor is still over this pane
    if (s.hoverKey) {
        int cx = 0, cy = 0; const bool ok = cursorPos(cx, cy); void* w = cursorWindow();
        const bool inside = s.haveWnd && ok && w == s.hoverWnd && cx - s.dx >= pane.left && cx - s.dx < pane.right && cy - s.dy >= pane.top && cy - s.dy < pane.bottom;
        if (!inside) s.hoverKey = 0; else tip(s, pane.top, pane.bottom, pane.left, pane.right);   // the hover may use the whole pane width
    }
}
// the step's hover box: inside the pane, left of the price axis, beside the step (right of it, else left), never over the Now box
void ReversalStage::tip(RSState& s, int top, int bottom, int left, int right) {
    std::map<long long, rsl::Step>::const_iterator it = s.steps.find(s.hoverKey); if (it == s.steps.end()) return;
    int hx = -1; for (size_t k = 0; k < s.hits.size(); ++k) if (s.hits[k].key == s.hoverKey) hx = s.hits[k].r;
    if (hx < 0) return;
    const std::vector<std::string> all = rsl::hoverRows(it->second, clockOf(it->second.key));
    // the fit cascade: every row -> without Use / Example -> also without the Odds row; 9 then 8 pt; one column, then two
    for (int lv = 0; lv < 3; ++lv) for (int pt = 9; pt >= 8; --pt) for (int cols = 1; cols <= 2; ++cols) {
        std::vector<std::string> keep;
        for (const std::string& r : all) { if (lv >= 1 && (r.compare(0, 7, "   Use:") == 0 || r.compare(0, 11, "   Example:") == 0)) continue; if (lv >= 2 && r.compare(0, 8, "   Odds:") == 0) continue; keep.push_back(r); }
        const std::vector<std::string> rows = rsl::wrap(keep, cols == 1 ? 78 : 60);
        FONT fb; fb.id = HELVETICA; fb.size = pt; fb.style = BOLD; FONT fp = fb; fp.style = PLAIN;
        const size_t per = (rows.size() + (size_t)cols - 1) / (size_t)cols;
        std::vector<int> cw((size_t)cols, 0); for (size_t k = 0; k < rows.size(); ++k) { setFont(k == 0 ? fb : fp); int& w = cw[k / per]; w = std::max(w, getTextWidth(rows[k].c_str(), -1)); }
        const int lh = pt + 4; int bw = 12; for (int c = 0; c < cols; ++c) bw += cw[(size_t)c] + (c ? 16 : 0);
        const int bh = (int)per * lh + 10;
        if (bh > bottom - top - 2) continue;
        int x = hx + 6; if (x + bw > right - 2) x = hx - 6 - bw - 12;
        if (x < left + 2) x = left + 2;
        if (x + bw > right - 2) continue;
        int y = top + 2;
        if (s.boxOn && x < s.boxR && x + bw > s.boxL && y < s.boxB && y + bh > s.boxT) { y = s.boxB + 2; if (y + bh > bottom - 1) { y = top + 2; x = s.boxR + 4; if (x + bw > right - 2) continue; } }
        setPen(C_LABEL, 1, P_SOLID); RCT b; b.set(coord(x), coord(y), coord(x + bw), coord(y + bh)); b.draw(1, C_LABEL, C_TIP_BG, DRAW_OPAQUE, PAT_SOLID);
        int cx = x + 6;
        for (int c = 0; c < cols; ++c) {
            for (size_t k = (size_t)c * per; k < rows.size() && k < (size_t)(c + 1) * per; ++k) { const int row = (int)(k - (size_t)c * per);
                setFont(k == 0 ? fb : fp); setTextColor(C_TEXT); RCT r; r.set(coord(cx), coord(y + 5 + row * lh), coord(cx + cw[(size_t)c] + 4), coord(y + 5 + (row + 1) * lh)); r.drawText(rows[k].c_str(), false, false); }
            cx += cw[(size_t)c] + 16;
        }
        return;
    }
}

extern "C" cppExtension* CreateExtension(void) {
    ReversalStage* p = new ReversalStage();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE);
    p->setDescription("LRA Reversal Stage: which stage is the local pivot in? A staircase per closed candle: 1 Attack, 2 Absorbed, 3 Exhausted, 4 Turn, 5 Re-test, 6 Expansion - green = a low pivot being built, red = a high pivot, a step dropping back = the attempt failed. Built from the letters lsBidAskStats and lsBookFlow record; decided once per closed candle, never repainted. Odds: not proven yet.");
    p->setVersion("1.1.0");   // RS_VERSION
    return p;
}
