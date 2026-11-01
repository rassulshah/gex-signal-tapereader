/********************************************************************************
 *  ReversalStage.cpp  --  Investor/RT RTX extension  lsReversalStage  2.0.0  (2026-10-23)
 *  2.0.0 (Rassul approved the 4-step design 05:47-05:50 CT, build ordered option B 05:55 CT): a GATE (the last 3 intact swing lows / highs
 *  of today's RTH ZigZag - SwingRef.h - or a key / MenthorQ level from the same files lsSweepRefill reads, or a typed held queue at the
 *  extreme) then a 4-step staircase TRAP / COLLISION / TRAPPED / RELEASE and a RESULT line (1 ATR). The Now box reads like
 *  "LOW at 7,696 (swing low 8:48 AM) / Trap: bids 2.3x, Q 143 / Collision: 1,840 sold, held (A, Q) / Trapped: sellers -6 ticks, D /
 *  Release: waiting for H / R". New record RS3 (2.0.0 steps mean something else; the 1.x RS records are not adopted).
 *  (history) lsReversalStage  1.0.0  (2026-10-22)
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
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "ReversalStage"
#endif
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "ReversalStageLogic.h"
#include "ReversalStageStream.h"
#include "HostSlot.h"
#include "SettingsGuard.h"
#include "ReversalStageSettings.h"
#include "SettingsHost.h"
#include "NewsWindow.h"          // (1.1.0) news / settle window "?" + the feed check
#include "SwingRef.h"            // (2.0.0) the gate: the last 3 intact swings (the ZigZag rule lsSweepRefill / lsStopRun use)
#include "NearLevels.h"          // (2.0.0) the gate: key / MenthorQ levels (the files lsSweepRefill reads)
#include "AwareHook.h"          // (2.2.0) the smart-read hook (layer L3) + SRC STAGE
#include "SelfAware.h"           // (2.3.0) self-awareness
#include "StageBox.h"            // (2.1.0) the stage info box + letters 1 2 3 C

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

#define RS_VERSION "2.3.0"   // (2.3.0, 2026-10-26) self-awareness SELFRS (SelfAware.h, SELF-AWARE.md; ships OFF per market) // (2.2.0, 2026-10-26) smart-read hook (AwareHook.h 0.1.0, TEAM-DESIGN.md): posts AWSTAGE for each closed bar once lsContext has posted CTX for it (or after 3 s), records the isolated + aware read in lsFlexLevels\\Context-AWR-<MKT>-<spb>s-<day>.rec; links OFF per market (LRA-Context-<MKT>.csv) so aware = isolated and nothing drawn changes; also posts SRC STAGE (stage, side, status) for each decided candle // (2.1.1, 2026-10-26) the C row reads the H ratio as lots / attack when the evidence carries no ratio // (2.1.0) (2.1.0, 2026-10-25, Rassul approved 22:51 CT stage-box-mockup.html) display only: the pane shows the letters 1 2 3 C (Trap / Collision / Turn / Confirm) on each pivot bar instead of the staircase, and the STAGE INFO BOX for the latest pivot (StageBox.h 1.0.0: stage # / name / what to look for / check / value (threshold); 3 Turn checked by the defenders' close = the HELD trigger, trapped and wick volume description only from lsFlexLevels\LRA-StageBox-<MKT>.csv; C = Hb >= 1.25x + follow-through; amber F? row); the engine and the RS3 record unchanged // (2.0.0) the 4-step model: gate + Trap / Collision / Trapped / Release + result // (1.3.0) typed MBO observations, arrival journal, immutable post-decision checkpoints
// 1.0.0:   // keep equal to rsl::VERSION, rsset::PLUGIN_VERSION and the setVersion literal below

static const COLOR C_GRID = 0x00374151, C_LABEL = 0x009CA3AF, C_TEXT = 0x00E5E7EB, C_BOX_BG = 0x000F1520, C_TIP_BG = 0x00111827;
static const long long SETTLE_SEC = 20;      // a candle is decided 20 s after its close (or once the next candle has closed)
static const int WINDOW = 600;              // price reference window; recorded post-bar states are authoritative

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
    rsh::Inbox inbox; std::string journalPath; bool provisional = true;
    // the record: decided candles [raw key] -> step; the newest key
    awh::Hook aw; std::string awChart; sa::Hook self;   // (2.2.0) the smart-read hook
    std::map<long long, rsl::Step> steps; long long recOff = 0, recBad = 0, written = 0, writeFail = 0, adopted = 0; std::string recPath; bool dirMade = false;
    long long lastPassMs = -1; long lastCount = -1; long long lastLetters = -1;
    rsl::Step now; bool haveNow = false;
    // (2.0.0) the gate: key / MenthorQ level sources (re-read at most every 5 s), the gate list of the last pass, counts for the status
    nl::Source keySrc, dealerSrc, mqSrc; long long levelCheckMs = -100000; std::vector<rsl::Gate> levelGates; std::string levelNote = "not read yet";
    std::vector<rsl::Gate> gates; long long gateSwings = 0;
    // drawing + hover
    struct Hit { int l = 0, t = 0, r = 0, b = 0; long long key = 0; };
    std::vector<Hit> hits; long long hoverKey = 0; void* hoverWnd = nullptr; int dx = 0, dy = 0; bool haveWnd = false;
    int paneL = 0, paneT = 0, paneR = 0, paneB = 0; int boxL = 0, boxT = 0, boxR = 0, boxB = 0; bool boxOn = false;
    long long drawn = 0, statusMs = -1; std::string statusText;
    // (2.1.0) the stage info box: params (re-read every 60 s), VAP sums per closed candle (immutable: cached by key), the boxes
    sbx::Params par; std::string parPath; long long parMs = -100000, parMt = -2; std::map<long long, sbx::Bar> vapCache; std::vector<sbx::Box> boxes; long long vapFails = 0;
};

static ss::Ids& rsIds() { static ss::Ids I; return I; }

class ReversalStage : public cppExtension {
public:
    SS_SAFE_DIALOG(ReversalStage, "ReversalStage")
    ReversalStage() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int scale(int, int, double* dMin, double* dMax) { if (dMin) *dMin = 0; if (dMax) *dMax = rsl::AXIS_MAX; return RTX_OK; }   // the pane's own axis: steps 0 - 4
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
    void levels(RSState& s, long long now);
    void writeStatus(RSState& s, long long now);
    long long civil(long long raw);
    void aware(RSState& s, long long now);
    std::string clockOf(long long raw);
    void render(RSState& s);
    void tip(RSState& s, int top, int bottom, int left, int right);
    void stageBoxes(RSState& s, long long now);   // (2.1.0)
    static int readStats(RTARRAYP* vap, int bar, BARSTATISTICS* st);
    static int readRow(RTARRAYP* vap, int bar, int k, VOLPROFILE* row);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { static_cast<ReversalStage*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<ReversalStage*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) { ss::registerAll(this, rsset::spec(), rsIds()); return RTX_OK; }
int cppExtension::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<ReversalStage*>(this)->pump(); return RTX_OK; }

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
    s.inbox = rsh::Inbox(); s.journalPath.clear(); s.provisional = true;
    if (!(std::isfinite(tick) && tick > 0 && tick < 1000)) { s.fault = true; s.state = "NO TICK SIZE"; return; }
    if (spb <= 0 || spb > 3600) { s.fault = true; s.state = "NEEDS A TIME CHART (1 s - 60 min)"; return; }
    s.state = "running";
}
static void addAssignment(RSState& s, const rsh::Assignment& a) {
    if (!s.evIds.insert(a.event.m.id).second) return;
    rsl::Ev e; e.key = a.key; e.sourceKey = a.event.key; e.received = a.received;
    e.pane = a.event.pane; e.l = a.event.l; e.m = a.event.m;
    s.evs.insert(std::make_pair(e.key, e));
}
void ReversalStage::readLetters(RSState& s) {
    const std::string f = flexDir(); if (f.empty()) return;
    RTARRAYI times(barDateTime);
    const int n = (int)std::min<long>(getBarCount(), (long)times.count); if (n < 2 || s.journalPath.empty()) return;
    const long long latest = (long long)times[n - 2], oldest = (long long)times[std::max(0, n - rsl::MAX_BARS - 3)];
    s.inbox.bind(f + "\\Strips", s.market, s.symbol, s.spb, s.journalPath);
    s.inbox.read((long long)currentDate(), latest, oldest, !s.steps.empty());
    for (const rsh::Assignment& a : s.inbox.takeRestored()) addAssignment(s, a);
    s.pathA = s.inbox.paths[lst::ATK]; s.pathD = s.inbox.paths[lst::DEF]; s.pathQ = s.inbox.paths[lst::QUE];
    s.letBad = s.inbox.bad;
    while (s.evs.size() > 40000) { const long long k = s.evs.begin()->first; s.evs.erase(k); }
}
void ReversalStage::readRecord(RSState& s) {
    const std::string f = flexDir(); if (f.empty()) return;
    const std::string d = f + "\\ReversalStage";
    if (!s.dirMade) { makeDir(f); makeDir(d); s.dirMade = true; }
    std::ostringstream tick; tick.precision(17); tick << s.tick;
    const std::string binding = std::string(RS_VERSION) + "-" + rse::token(s.market) + "-" + rse::token(s.symbol) + "-T" + rse::token(tick.str()) + "-" + std::to_string(s.spb);
    const std::string p = d + "\\RS3-" + binding + ".csv";   // (2.0.0) new record: the 4-step meaning
    s.journalPath = d + "\\RJ-" + binding + ".csv";
    if (p != s.recPath) { s.recPath = p; s.recOff = 0; s.steps.clear(); }
    rsh::tail(p, s.recOff, [&](const std::string& ln) { rsl::Step st; if (ln.compare(0, 5, "RS|3|") != 0 || !rsl::parse(ln, st)) { ++s.recBad; return; } if (st.decided > (long long)currentDate()) { ++s.recBad; return; } if (!s.steps.count(st.key)) { s.steps[st.key] = st; ++s.adopted; } });
    while (s.steps.size() > 20000) s.steps.erase(s.steps.begin());
}
// (2.0.0) key levels (his session levels: ONL PDL PFL LonLO PWL AsiaLO and the highs) + MenthorQ (dealer / levels files, the chart's
// contract only) - the same files and freshness rule as lsSweepRefill (SweepRefill::levels)
void ReversalStage::levels(RSState& s, long long now) {
    if (now - s.levelCheckMs < 5000 && now >= s.levelCheckMs) return;
    s.levelCheckMs = now;
    const std::string f = flexDir(); if (f.empty() || s.market.empty() || !(s.tick > 0)) return;
    nl::loadSource(f + "\\LRA-KeyLevels-" + s.market + ".csv", s.keySrc, 0);
    nl::loadSource(f + "\\LRA-Dealer-" + s.market + ".csv", s.dealerSrc, 1);
    nl::loadSource(f + "\\LRA-Levels-" + s.market + ".csv", s.mqSrc, 2);
    const long long wall = (long long)std::time(nullptr);
    auto fresh = [&](const nl::Source& X) { return X.sig >= 0 && X.at > 0 && wall - X.at <= 18 * 3600 && X.at - wall <= 600; };
    auto ticks = [&](double px, long long& out) { const double q = px / s.tick; if (!std::isfinite(q) || q <= 0 || q > 9e15) return false; out = (long long)std::llround(q); return true; };
    std::vector<rsl::Gate> v; int nk = 0, nm = 0;
    if (fresh(s.keySrc)) for (size_t i = 0; i < s.keySrc.v.size(); ++i) {
        rsl::Gate g; g.label = s.keySrc.v[i].name; g.dir = swg::keyDir(g.label); g.kind = rsl::G_KEY;
        if (g.dir == 0 || !ticks(s.keySrc.v[i].px, g.px)) continue;
        v.push_back(g); ++nk;
    }
    const nl::Source* mq[2] = { &s.dealerSrc, &s.mqSrc };
    for (int k = 0; k < 2; ++k) {
        const nl::Source& X = *mq[k];
        if (!fresh(X) || X.stale || !nl::sameContract(s.symbol, X.sym)) continue;
        for (size_t i = 0; i < X.v.size(); ++i) {
            rsl::Gate g; g.label = "MQ " + X.v[i].name; g.kind = rsl::G_MQ; g.dir = 0;
            if (!ticks(X.v[i].px, g.px)) continue;
            bool dup = false; for (size_t j = 0; j < v.size() && !dup; ++j) dup = v[j].kind == rsl::G_MQ && v[j].px == g.px;
            if (!dup) { v.push_back(g); ++nm; }
        }
    }
    s.levelGates.swap(v);
    s.levelNote = std::to_string(nk) + " key, " + std::to_string(nm) + " MenthorQ" + (s.keySrc.sig < 0 ? " (key file missing)" : "");
}
// Only a new latest closed decision is admitted. Older prices warm the reference;
// new letters never pass through frozen historical decisions.
void ReversalStage::decide(RSState& s) {
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose), op(barOpen);
    const long count = getBarCount();
    const int n = (int)std::min<long>(std::min(count, (long)op.count), std::min(std::min((long)times.count, (long)hi.count), std::min((long)lo.count, (long)cl.count)));
    if (n < 2) return;
    const int last = n - 1, from = std::max(0, last - WINDOW);
    std::vector<rsl::Bar> B; B.reserve((size_t)(last - from));
    for (int i = from; i < last; ++i) {
        rsl::Bar b; b.key = (long long)times[i];
        const double h = (double)hi[i], l = (double)lo[i], c = (double)cl[i];
        b.ok = b.key > 0 && std::isfinite(h) && std::isfinite(l) && std::isfinite(c) && h >= l && c >= l && c <= h;
        // A finite positive tick is not enough on its own: a corrupt tiny value can make
        // price/tick non-finite or outside long long before the cast below.  Treat that
        // candle as unavailable rather than invoking an undefined floating-point cast.
        const auto ticks = [&](double price, long long& out) {
            const double q = price / s.tick, hiTick = (double)LLONG_MAX - 1024.0;
            if (!std::isfinite(q) || q < (double)LLONG_MIN || q > hiTick) return false;
            const double rounded = std::floor(q + 0.5);
            if (!std::isfinite(rounded) || rounded < (double)LLONG_MIN || rounded > hiTick) return false;
            out = (long long)rounded;
            return true;
        };
        if (b.ok && (!ticks(h, b.h) || !ticks(l, b.l) || !ticks(c, b.c))) b.ok = false;
        b.quality = s.inbox.quality(b.key);
        B.push_back(b);
    }
    if (B.empty()) return;
    {   // (2.0.0) THE GATE: today's RTH ZigZag swings (SwingRef.h, closed candles only) + the key / MenthorQ levels
        swg::ZigZag Z; Z.spb = s.spb; int oMin = 0, cMin = 0; const bool haveRth = swg::rthOf(s.market, oMin, cMin);
        Z.rthLenSec = haveRth ? (cMin - oMin) * 60 : 0;
        for (int i = from; i < last; ++i) {
            const rsl::Bar& rb = B[(size_t)(i - from)]; if (!rb.ok || !haveRth) continue;
            struct tm t; std::memset(&t, 0, sizeof t); if (!getLocaltime((RTDATE)times[i], &t)) continue;
            swg::ZBar z; z.t = rb.key; z.h = rb.h; z.l = rb.l; z.c = rb.c; z.day = (long long)(t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday;
            const double o = (double)op[i] / s.tick; if (!std::isfinite(o)) continue; z.o = (long long)std::llround(o);
            z.rth = swg::isRth(t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec, s.spb, oMin, cMin);
            Z.add(z);
        }
        s.gates = s.levelGates; s.gateSwings = 0;
        for (size_t k = 0; k < Z.swings.size(); ++k) {
            const swg::Swing& w = Z.swings[k]; if (w.confirmIdx <= 0 || (size_t)w.confirmIdx >= Z.bars.size()) continue;
            rsl::Gate g; g.kind = rsl::G_SWING; g.dir = w.dir; g.px = w.px; g.pivKey = w.barT; g.fromKey = Z.bars[(size_t)w.confirmIdx].t;
            g.label = std::string(w.dir < 0 ? "swing low " : "swing high ") + clockOf(w.barT);
            s.gates.push_back(g); ++s.gateSwings;
        }
    }
    const long long rawNow = (long long)currentDate(), nowC = civil(rawNow), key = B.back().key;
    const bool settled = nowC != LLONG_MIN && civil(key) != LLONG_MIN && nowC >= civil(key) + SETTLE_SEC;
    const bool fresh = !s.steps.count(key) && (s.steps.empty() || key > s.steps.rbegin()->first);
    const bool ready = fresh && settled && s.inbox.terminal(key);
    if (ready) {
        std::vector<rsh::Assignment> admitted;
        if (!s.inbox.admit(key, rawNow, admitted)) { s.state = "WAITING - evidence journal write failed"; return; }
        for (const rsh::Assignment& a : admitted) addAssignment(s, a);
    }
    const std::vector<rsl::Step> st = rsl::run(B, s.evs, &s.steps, &s.gates); if (st.empty()) return;
    s.now = st.back(); s.haveNow = true; s.provisional = !s.steps.count(key);
    if (!fresh) { s.provisional = false; s.state = "running - confirmed journal"; return; }
    if (!ready) { s.state = s.inbox.terminal(key) ? "WAITING - candle settlement" : "WAITING - typed ATK/DEF source acknowledgement"; return; }
    rsl::Step decided = st.back(); decided.decided = rawNow;
    if (s.recPath.empty()) return;
    const std::string record = rsl::line(decided); rsl::Step durable;
    if (!rsl::parse(record, durable)) { ++s.writeFail; s.state = "WAITING - invalid stage serialization"; return; }
    if (rsh::append(s.recPath, record + "\n")) {
        s.steps[durable.key] = durable; ++s.written; s.now = durable; s.provisional = false;
        s.state = decided.quality == rse::COMPLETE ? "running - MBO evidence confirmed" : "BLOCKED - incomplete/unavailable MBO";
    } else { ++s.writeFail; s.state = "WAITING - stage journal write failed"; }
}
// (2.2.0) SRC STAGE + the smart-read hook for the last CLOSED candle once its step is decided (durable); links OFF: aware = isolated
void ReversalStage::aware(RSState& s, long long now) {
    if (s.symbol.empty() || s.market.empty()) return;
    if (!lsperf::smartAllowed(LSPERF_PLUGIN, s.symbol)) return;          // over its perf budget: the smart step is off, the stage pane runs
    RTARRAYI tt(barDateTime); const long c = getBarCount(); if (c < 2 || (long)tt.count < c) return;
    const long long key = (long long)tt[(int)c - 2]; const std::map<long long, rsl::Step>::const_iterator it = s.steps.find(key);
    if (it == s.steps.end()) return;
    const long long bar = civil(key); if (bar == LLONG_MIN || bar <= 0) return;
    const char* cl = getChartLabel(); const std::string chart = cl ? cl : "";
    if (!s.awChart.empty() && s.awChart != chart) { mst::clear("STAGE", s.symbol, s.spb, s.awChart); s.self.clear(s.symbol, s.spb, s.awChart); s.aw.clear(s.symbol, s.spb, s.awChart); }
    s.awChart = chart;
    const rsl::Step& st = it->second; s.aw.src = "STAGE";
    if (s.aw.t != bar || s.aw.postMs < 0 || now - s.aw.postMs >= 15000 || now < s.aw.postMs) {
        std::map<std::string, std::string> kv; kv["v"] = RS_VERSION; kv["stage"] = std::to_string(st.stage); kv["side"] = st.side == rsl::LOW ? "low" : "high"; kv["status"] = std::to_string(st.status);
        mst::publish("STAGE", s.symbol, s.spb, chart, now, bar, kv);
    }
    s.aw.tick(s.symbol, s.spb, chart, s.market, now, bar, awh::stageRead(st.stage, st.side == rsl::LOW, st.status));
    if (lsperf::smartAllowed(LSPERF_PLUGIN, s.symbol)) {   // (2.3.0) PerfGuard: self-awareness only while this indicator is within budget
    {   // (2.3.0) SELF-AWARENESS (SelfAware.h, SELF-AWARE.md): the stage's own turn call (the same side the smart read takes: a stage-2/3 pivot
        // low = up, high = down, a failed attempt = none) is a signal once per new call; HIT = 1 ATR its way before 1 ATR against (later
        // bars only). Value = the stage. Display unchanged (SWITCH OFF per market).
        RTARRAY op(barOpen), hi(barHigh), lo(barLow), cz(barClose); const int i = (int)c - 2;
        if (op.count > i && hi.count > i && lo.count > i && cz.count > i) {
            const int sd = awh::stageRead(st.stage, st.side == rsl::LOW, st.status).side;
            std::map<std::string, std::string> kv; kv["stage"] = std::to_string(st.stage); kv["status"] = std::to_string(st.status);
            kv["side"] = st.side == rsl::LOW ? "low" : "high"; kv["turn"] = sd > 0 ? "up" : sd < 0 ? "down" : "none";
            static const sa::Spec SP = [] { sa::Spec x; x.valueKey = "stage"; x.sideKey = "turn"; x.onChange = true; x.rearm = true;
                x.why = { "stage", "status", "side" }; return x; }();
            s.self.ind = "RS";
            sa::fromFacts(s.self, SP, s.symbol, s.spb, chart, s.market, now, sa::Bar{ bar, op[i], hi[i], lo[i], cz[i] }, kv);
        }
    }
    }
}
void ReversalStage::writeStatus(RSState& s, long long now) {
    const std::string f = flexDir(); if (f.empty() || s.market.empty()) return;
    int liveQuality = s.haveNow ? s.inbox.quality(s.now.key) : rse::PENDING;
    struct tm current; std::memset(&current, 0, sizeof(current));
    if (getLocaltime((RTDATE)currentDate(), &current) && nw::atTm(s.market, current).feedBad) liveQuality = rse::UNAVAILABLE;
    std::ostringstream o;
    o << "VERSION," << RS_VERSION << "\nMARKET," << s.market << "\nSYMBOL," << s.symbol << "\nSPB," << s.spb << "\nSTATE," << s.state
      << "\nLETTERS," << s.evs.size() << " read (" << s.letBad << " bad lines), files " << s.pathA << " | " << s.pathD << " | " << s.pathQ
      << "\nRECORD," << s.steps.size() << " candles (" << s.written << " written here, " << s.writeFail << " write fails, " << s.recBad << " bad lines), file " << s.recPath
      << "\nNOW," << (s.haveNow ? rsl::sideWord(s.now.side) + " stage " + std::to_string(s.now.stage) + " " + rsl::statusName(s.now.status) + " chain " + s.now.chain : std::string("none"))
      << "\nQUALITY," << rse::qualityName(liveQuality) << "\nACHIEVED_QUALITY," << (s.haveNow ? rse::qualityName(s.now.quality) : "pending")
      << "\nREADINESS," << rsl::readinessName(rsl::readinessOf(s.now.flags, s.now.stage, liveQuality, false)) << "\nPROVISIONAL," << (s.provisional ? 1 : 0)
      << "\nJOURNAL," << s.journalPath << "\nPENDING_EVIDENCE," << s.inbox.pendingCount() << "\nIGNORED_HISTORY," << s.inbox.ignoredHistory
      << "\nWRONG_CONTRACT," << s.inbox.wrongContract << "\nEVIDENCE_WRITE_FAILURES," << s.inbox.writeFails << "\nDROPPED_EVIDENCE," << s.inbox.dropped
      << "\nGATE," << s.gates.size() << " locations (" << s.gateSwings << " swings, levels " << s.levelNote << ", SwingRef " << swg::VERSION << ")"
      << "\nSTAGE_BOX," << sbx::VERSION << "," << s.boxes.size() << " pivots,latest " << (s.boxes.empty() ? std::string("none") : (s.boxes.back().letters.empty() ? std::string("-") : s.boxes.back().letters)) << ",params " << s.par.why << " trapped " << s.par.trapThr << "x wick " << s.par.wickThr << "x hb " << s.par.hbMin << "x,vap fails " << s.vapFails
      << "\nAWARENESS," << awh::VERSION << " AWSTAGE decided " << s.aw.writes << " records" << (lsperf::smartAllowed(LSPERF_PLUGIN, s.symbol) ? "" : " (smart off)") << "\nPERF," << lsperf::statusOf(this)   // (2.2.0)
      << "\nDRAWN," << s.drawn << "\nODDS,? not proven yet (" << rsl::ODDS_TICKET << ")\n";
    const std::string t = o.str();
    if (t == s.statusText && s.statusMs >= 0 && now - s.statusMs < 30000) return;
    s.statusMs = now;
    const std::string d = f + "\\ReversalStage", p = s.recPath + ".status.txt", tmp = p + ".tmp";
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
        static std::mutex journalMutex; std::lock_guard<std::mutex> journalLock(journalMutex);
        readRecord(*s); readLetters(*s); levels(*s, now); decide(*s); stageBoxes(*s, now);
        aware(*s, now);   // (2.2.0) L3, links OFF
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

int ReversalStage::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    RSState* s = slot_.get(this, false); if (!s) return RTX_OK;
    std::unique_lock<std::recursive_mutex> lock(s->mutex, std::try_to_lock); if (!lock.owns_lock()) return RTX_OK;
    try { render(*s); } catch (...) {}
    return RTX_OK;
}

// (2.1.0) THE STAGE INFO BOX (StageBox.h): params, the VAP sums of the closed candles (cached by key - a closed candle never changes),
// the attempts from the record, the letters from the typed evidence; recomputed from closed data only, so a box never changes once decided
int ReversalStage::readStats(RTARRAYP* vap, int bar, BARSTATISTICS* st) {
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!vap || !st || bar < 0) return -2;
        std::memset(st, 0, sizeof(*st));
        if (vap->getBarStatistics(bar, *st) != RTX_OK) return -2;
        return st->prices < 0 ? -2 : st->prices;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
int ReversalStage::readRow(RTARRAYP* vap, int bar, int k, VOLPROFILE* row) {
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!vap || !row) return -2;
        std::memset(row, 0, sizeof(*row));
        return vap->getVolumeProfile(bar, k, *row) == RTX_OK ? 0 : -2;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
void ReversalStage::stageBoxes(RSState& s, long long now) {
    if (s.market.empty() || !(s.tick > 0)) return;
    {   // params: lsFlexLevels\LRA-StageBox-<MKT>.csv (missing = the built-in TRAPPED.md defaults)
        const std::string f = flexDir(); const std::string pp = f.empty() ? std::string() : f + "\\LRA-StageBox-" + s.market + ".csv";
        if (pp != s.parPath || now - s.parMs >= 60000 || now < s.parMs) {
            s.parPath = pp; s.parMs = now; sbx::Params P = sbx::defaultsFor(s.market);
            std::ifstream in(pp.c_str(), std::ios::binary); int used = 0;
            if (in.is_open()) { std::string ln; int n = 0; while (std::getline(in, ln) && ++n < 200) if (sbx::parseParam(ln, P)) ++used; }
            P.why = in.is_open() ? (used ? "file (" + std::to_string(used) + " values)" : std::string("file (no values) - defaults")) : std::string("defaults (no file)");
            s.par = P;
        }
    }
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose), op(barOpen); RTARRAYP vap(barVolumeProfile);
    const long count = getBarCount();
    const int n = (int)std::min<long>(count, std::min(std::min((long)times.count, (long)hi.count), std::min(std::min((long)lo.count, (long)cl.count), (long)op.count)));
    if (n < 3) return;
    const int last = n - 1, from = std::max(0, last - 400);              // the forming candle (last) is never used
    std::vector<sbx::Bar> B; B.reserve((size_t)(last - from));
    for (int i = from; i < last; ++i) {
        const long long key = (long long)times[i];
        std::map<long long, sbx::Bar>::const_iterator c = s.vapCache.find(key);
        if (c != s.vapCache.end()) { B.push_back(c->second); continue; }
        sbx::Bar b; b.key = key;
        const double o = (double)op[i] / s.tick, h = (double)hi[i] / s.tick, l = (double)lo[i] / s.tick, cc = (double)cl[i] / s.tick;
        b.ok = key > 0 && std::isfinite(o) && std::isfinite(h) && std::isfinite(l) && std::isfinite(cc) && std::fabs(h) < 1e15 && std::fabs(l) < 1e15 && h >= l;
        if (b.ok) { b.o = std::llround(o); b.h = std::llround(h); b.l = std::llround(l); b.c = std::llround(cc); }
        if (b.ok) {
            BARSTATISTICS st; const int np = readStats(&vap, i, &st);
            if (np > 0 && np < 5000) {
                std::vector<sbx::Row> rows; rows.reserve((size_t)np); bool bad = false;
                for (int k = 0; k < np; ++k) { VOLPROFILE v; if (readRow(&vap, i, k, &v) != 0) { bad = true; break; } const double q = (double)v.price / s.tick; if (!std::isfinite(q)) { bad = true; break; }
                    sbx::Row r; r.px = std::llround(q); r.buy = (double)v.buyVolume; r.sell = (double)v.sellVolume; r.total = (double)v.totalVolume; rows.push_back(r); }
                if (!bad) sbx::fillVap(b, rows); else ++s.vapFails;
            }
        }
        if (s.vapCache.size() > 3000) s.vapCache.erase(s.vapCache.begin());
        s.vapCache[key] = b; B.push_back(b);
    }
    std::vector<sbx::Letter> L; L.reserve(s.evs.size());
    for (std::multimap<long long, rsl::Ev>::const_iterator it = s.evs.begin(); it != s.evs.end(); ++it) {
        sbx::Letter x; x.key = it->first; x.pane = it->second.pane; x.code = it->second.l.code; x.nums = it->second.l.nums; x.ratio = it->second.m.ratio; x.pace100 = it->second.m.pace100; x.lots = it->second.m.lots;
        if (!(x.ratio > 0) && x.code == "H" && it->second.m.auxLots > 0 && it->second.m.lots >= 0) x.ratio = (double)it->second.m.lots / (double)it->second.m.auxLots;   // H: lots / the attack
        L.push_back(x);
    }
    const std::vector<sbx::Attempt> A = sbx::attemptsOf(s.steps);
    std::vector<sbx::Box> out;
    const long long oldest = B.empty() ? 0 : B.front().key;
    for (size_t k = A.size() > 40 ? A.size() - 40 : 0; k < A.size(); ++k) {
        if (A[k].lastKey < oldest) continue;
        sbx::Box X = sbx::build(A[k], B, L, s.par, s.tick); if (!X.on) continue;
        X.title = std::string("YOUR 3 REVERSAL STAGES · ") + (X.side == sbx::LOW ? "low " : "high ") + sbx::px(X.ext, s.tick) + " (pivot bar " + clockOf(X.pivKey) + ")" + (X.side == sbx::LOW ? " · bullish" : " · bearish");
        out.push_back(X);
    }
    s.boxes.swap(out);
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
    FONT f; f.id = HELVETICA; f.size = 10; f.style = BOLD; setFont(f);   // 10 pt minimum (his rule)
    // (2.1.0) the legend right of the last candle (when there is room)
    if (V.b(K_SHOW_NAMES)) {
        static const char* const LEG[] = { "1 Trap", "2 Collision", "3 Turn", "C Confirm" };
        const int room = pane.right - lastR - 4; int wl = 0; for (int k = 0; k < 4; ++k) wl = std::max(wl, getTextWidth(LEG[k], -1));
        if (room >= wl + 2 && bot - top >= 4 * 13) { setTextColor(C_LABEL); for (int k = 0; k < 4; ++k) { RCT r; r.set(coord(lastR + 4), coord(top + k * 13), coord(pane.right - 1), coord(top + (k + 1) * 13)); r.drawText(LEG[k], false, false); } }
    }
    // (2.1.0) the letters 1 2 3 C on each pivot bar (closed data only), green = a low, red = a high; an F? pivot gets an amber "F?"
    std::map<long long, int> barOf; for (int i = std::max(0, v0 - 2); i <= v1 && i < last; ++i) barOf[(long long)times[i]] = i;
    if (V.b(K_SHOW_STEPS)) {
        std::vector<std::pair<int, int> > taken;                           // (x left, x right) of letters already drawn
        for (size_t k = 0; k < s.boxes.size(); ++k) {
            const sbx::Box& X = s.boxes[k];
            std::map<long long, int>::const_iterator b = barOf.find(X.pivKey); if (b == barOf.end()) continue;
            const int xc = xOf(b->second); if (xc < pane.left || xc > lastR) continue;
            nw::Gate ng; { struct tm tk; std::memset(&tk, 0, sizeof(tk)); if (getLocaltime((RTDATE)X.pivKey, &tk)) ng = nw::atTm(s.market, tk); }
            if (ng.feedBad) continue;                                      // feed check failed: recorded only
            bool warn = false; for (const sbx::BoxRow& r : X.rows) if (r.state == sbx::WARN) warn = true;
            const std::string txt = X.letters.empty() ? (warn ? std::string("F?") : std::string()) : X.letters;
            if (txt.empty()) continue;
            const int tw = getTextWidth(txt.c_str(), -1) + 4, lh = 13;
            int xl = xc - tw / 2; for (size_t q = 0; q < taken.size(); ++q) if (xl < taken[q].second + 2 && xl + tw > taken[q].first - 2) xl = taken[q].second + 3;
            if (xl + tw > pane.right - 1) continue;
            const int yt = std::max(top, bot - lh);                          // the bottom row of the pane (the info box sits at the top)
            const COLOR c = (warn && X.letters.empty()) ? (COLOR)V.c(K_COL_WARN) : (COLOR)(X.side == sbx::LOW ? V.c(K_COL_LOW) : V.c(K_COL_HIGH));
            setTextColor(c); RCT r; r.set(coord(xl), coord(yt), coord(xl + tw), coord(yt + lh)); r.drawText(txt.c_str(), false, false);
            if (warn && !X.letters.empty() && yt - lh >= top) { setTextColor((COLOR)V.c(K_COL_WARN)); RCT w; w.set(coord(xl), coord(yt - lh), coord(xl + tw), coord(yt)); w.drawText("F?", false, false); }
            taken.push_back(std::make_pair(xl, xl + tw));
            RSState::Hit h; h.l = xl; h.r = xl + tw; h.t = yt - (warn ? lh : 0); h.b = bot + 1; h.key = X.pivKey; s.hits.push_back(h);
            ++s.drawn;
        }
    }
    // (2.1.0) THE STAGE INFO BOX: the latest pivot. Rows: code (side colour / grey / amber), stage name, what to look for, check / dash, value
    if (V.b(K_SHOW_NOW) && !s.boxes.empty()) {
        const sbx::Box& X = s.boxes.back();
        const COLOR side = (COLOR)(X.side == sbx::LOW ? V.c(K_COL_LOW) : V.c(K_COL_HIGH)), warnC = (COLOR)V.c(K_COL_WARN);
        FONT fb = f; FONT fp = f; fp.style = PLAIN;
        const int lh = 13, n = (int)X.rows.size() + 1;
        int wCode = 0, wName = 0, wLook = 0, wVal = 0;
        setFont(fb); for (const sbx::BoxRow& r : X.rows) { wCode = std::max(wCode, getTextWidth(r.code.c_str(), -1)); wName = std::max(wName, getTextWidth(r.name.c_str(), -1)); }
        setFont(fp); for (const sbx::BoxRow& r : X.rows) { wLook = std::max(wLook, getTextWidth(r.look.c_str(), -1)); wVal = std::max(wVal, getTextWidth(r.value.c_str(), -1)); }
        setFont(fb); const int wTitle = getTextWidth(X.title.c_str(), -1);
        int boxR = lastR; if (boxR - pane.left - 8 < 420) boxR = pane.right;   // a short chart: the box may use the whole pane width
        const int avail = boxR - pane.left - 8;
        bool showLook = true; int bw = 12 + wCode + 10 + wName + 10 + wLook + 10 + 14 + 10 + wVal;
        if (bw > avail) { showLook = false; bw = 12 + wCode + 10 + wName + 10 + 14 + 10 + wVal; }   // a narrow pane: the "what to look for" column goes to the hover
        bw = std::max(bw, wTitle + 12);
        const int bh = n * lh + 8; int x = 0, y = 0;
        if (bh <= pane.bottom - pane.top && bw <= avail + 8 && rsl::boxAt(V.i(K_POSITION), pane.left, pane.top, boxR, pane.bottom, bw, bh, 4, x, y)) {
            setPen(C_LABEL, 1, P_SOLID); RCT b; b.set(coord(x), coord(y), coord(x + bw), coord(y + bh)); b.draw(1, C_LABEL, C_BOX_BG, DRAW_OPAQUE, PAT_SOLID);
            setFont(fb); setTextColor(C_TEXT); { RCT r; r.set(coord(x + 6), coord(y + 4), coord(x + bw - 4), coord(y + 4 + lh)); r.drawText(X.title.c_str(), false, false); }
            for (int k = 0; k < (int)X.rows.size(); ++k) {
                const sbx::BoxRow& R = X.rows[(size_t)k]; const int yy = y + 4 + (k + 1) * lh; int cx = x + 6;
                const COLOR c = R.state == sbx::WARN ? warnC : R.state == sbx::SEEN ? side : C_LABEL;
                setFont(fb); setTextColor(c); { RCT r; r.set(coord(cx), coord(yy), coord(cx + wCode + 4), coord(yy + lh)); r.drawText(R.code.c_str(), false, false); } cx += wCode + 10;
                setTextColor(C_TEXT); { RCT r; r.set(coord(cx), coord(yy), coord(cx + wName + 4), coord(yy + lh)); r.drawText(R.name.c_str(), false, false); } cx += wName + 10;
                if (showLook) { setFont(fp); setTextColor(C_TEXT); RCT r; r.set(coord(cx), coord(yy), coord(cx + wLook + 4), coord(yy + lh)); r.drawText(R.look.c_str(), false, false); cx += wLook + 10; }
                if (R.state == sbx::SEEN) {                                  // a drawn check mark (no font glyph needed)
                    setPen(c, 2, P_SOLID); PNT a1; a1.set(0, 0.0f); a1.h = coord(cx + 1); a1.v = coord(yy + 7); a1.setDrawPosition(); PNT a2; a2.set(0, 0.0f); a2.h = coord(cx + 5); a2.v = coord(yy + 11); a2.drawLineTo();
                    PNT a3; a3.set(0, 0.0f); a3.h = coord(cx + 12); a3.v = coord(yy + 2); a3.drawLineTo();
                } else { setFont(fb); setTextColor(c); RCT r; r.set(coord(cx), coord(yy), coord(cx + 14), coord(yy + lh)); r.drawText(R.state == sbx::WARN ? "!" : "\x96", false, false); }   // "\x96" = en dash (cp1252)
                cx += 14 + 10; setFont(fb);
                setTextColor(c); { RCT r; r.set(coord(cx), coord(yy), coord(x + bw - 4), coord(yy + lh)); r.drawText(R.value.c_str(), false, false); }
            }
            s.boxL = x; s.boxT = y; s.boxR = x + bw; s.boxB = y + bh; s.boxOn = true;
            RSState::Hit bh2; bh2.l = x; bh2.r = x + bw; bh2.t = y; bh2.b = y + bh; bh2.key = X.pivKey;
            for (size_t k = 0; k < s.hits.size();) { const RSState::Hit& h = s.hits[k]; if (h.l < s.boxR && h.r > s.boxL && h.t < s.boxB && h.b > s.boxT) s.hits.erase(s.hits.begin() + (long)k); else ++k; }   // no letter hover under the box
            s.hits.push_back(bh2);                                       // the box itself hovers the latest pivot
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
    int hx = -1; for (size_t k = 0; k < s.hits.size(); ++k) if (s.hits[k].key == s.hoverKey) hx = s.hits[k].r;
    if (hx < 0) return;
    std::vector<std::string> all;
    for (size_t k = 0; k < s.boxes.size(); ++k) if (s.boxes[k].pivKey == s.hoverKey) {   // (2.1.0) the pivot's stage box in his format
        const sbx::Box& X = s.boxes[k]; all.push_back(X.title);
        for (const std::string& h : X.hover) all.push_back("   " + h);
        for (const sbx::BoxRow& r : X.rows) all.push_back("   " + r.code + (r.state == sbx::SEEN ? " seen: " : r.state == sbx::WARN ? " warning: " : " not seen: ") + r.value);
        break;
    }
    if (all.empty()) { std::map<long long, rsl::Step>::const_iterator it = s.steps.find(s.hoverKey); if (it == s.steps.end()) return; all = rsl::hoverRows(it->second, clockOf(it->second.key), s.tick); }
    // the fit cascade: every row -> without Use / Example -> also without the Odds row; 9 then 8 pt; one column, then two
    for (int lv = 0; lv < 3; ++lv) for (int pt = 9; pt >= 8; --pt) for (int cols = 1; cols <= 3; ++cols) {   // (2.1.0) up to 3 columns (his ~100 px panes)
        std::vector<std::string> keep;
        for (const std::string& r : all) { if (lv >= 1 && (r.compare(0, 7, "   Use:") == 0 || r.compare(0, 11, "   Example:") == 0 || r.compare(0, 14, "   How to use:") == 0)) continue; if (lv >= 2 && r.compare(0, 8, "   Odds:") == 0) continue; keep.push_back(r); }
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
    p->setDescription("LRA Reversal Stage: at a swing / key / MenthorQ level or queue zone, typed MBO evidence for 4 steps - Trap (stacked floor), Collision (attack met by absorption / a held queue), Trapped (pace fade, D divergence, attackers underwater) and Release (hit back / reclaim, re-test held) - then the 1 ATR result. Closed decisions are recorded and never repainted. Odds remain unproven.");
    p->setVersion("2.3.0");   // RS_VERSION
    return p;
}
