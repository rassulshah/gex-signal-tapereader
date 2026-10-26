/********************************************************************************
 *  CobImpulse.cpp  --  Investor/RT RTX extension  lsCobImpulse  (0.3.1, 2026-10-14; 0.4.1, 2026-10-18)
 *  0.4.1 (2026-10-18 night, Rassul approved the Market State): posts SRC "COB" (MarketState.h, MARKET-STATE-SPEC 1.0) for the last
 *        CLOSED bar from the record it already keeps: cob (bull / bear / none on that bar), cob_odds (the decided p, only when the odds
 *        are LIVE - what the label shows - else ?), cob_odds_base, cob_odds_n, cob_tag (the recorded R / V / C letters), cob_gamma (the
 *        recorded gamma name); re-posted every 10 s, cleared on removal. The odds logic (CobOdds.h, 0.4.0) and everything drawn are
 *        unchanged; cimp::VERSION (the logic) stays 0.4.0.
 *  0.3.1 (2026-10-14, Rassul approved the external-audit display list at 18:25 CT, section 1): the gamma hover reads "MenthorQ level near
 *        the CoB start: CR 6,020 (model, data 7 min old)." - no "driving" / "behind". RB / CoB marks (his locked trade), the Cob+R? / V? /
 *        C? tags, the gamma part of the tag and the white impulse box are unchanged.
 *  0.3.0 (Rassul 15:47 / 15:51 CT "identify if a gamma level is driving the CoB", approved format Cob+<R/V/C>+<gamma name>?, e.g.
 *        Cob+R+CR?, Cob+RC+PS?, Cob+V+HVL?, Cob+CR?, Cob+GM?): the GAMMA PART of the tag (CobImpulseLogic.h 0.3.0 = lra.cob_gamma.drivers).
 *        At the CoB close it reads lsFlexLevels\GammaProfile-MQ-<MKT>.csv (no G when missing / another session / newer than the CoB /
 *        > 35 min old in RTH, or the book's band is > 0.20 ADR from the chart's prior 15:00 CT close - 20-day ADR from the chart's own
 *        bars), names ONE level (origin > cross > target, nearest, named before GM) and records it with the CoB (23-field record + a
 *        .gamma.csv hover sidecar) - never repainted. Market file: GAMMA,1 show / 0 hide (always logged), GAMMA_W,6, GAMMA_EM,100.
 *        The hover adds the gamma lines (every dealer line says "model"; the HVL regime text follows the hvl_cushion claim gate).
 *  0.2.0 (Rassul 13:31 CT): "+C" climax at the origin - bull: a RED biggest-volume (35+ bars) bar from the pivot-low bar -2 .. +2, then a
 *        close above its high by the CoB bar (bear mirror); tags in order R, V, C ("Cob+RVC?"); C fields added to the record (a 0.1.0
 *        record file is read and rewritten in the new layout, its CoBs keep their 0.1.0 tags).
 *
 *  COB IMPULSE: did the CoB leg start with force? Rassul 2026-10-14 12:07 CT (approved; mockup cob/mockup-cob-impulse.png):
 *  "tag CoB signals that have an IMPULSE at their origin". Put ONE on the price pane of each 3-min LTF chart (ES NQ CL GC HG NG 6E).
 *    - It reads WHERE his CoB fired from his own RTL (cob_bull_3m_1b / cob_bear_3m_1b, read by name like the Dealer Profile reads
 *      cob_bull) and his Abs_Bull / Abs_Bear, on each new bar, for CLOSED bars only.
 *    - It finds the CoB leg's pivot (his 1-bar ZigZag, native) and looks at the pivot bar .. 2 bars after it: a bar inside the pivot
 *      bar (bull: its low below the pivot bar's high; bear: its high above the pivot bar's low) with the biggest RANGE in 35+ bars
 *      = R, the biggest VOLUME in 35+ bars = V (the chart's own bars and volume; nothing external at run time).
 *    - Label "Cob" / "Cob+R?" / "Cob+V?" / "Cob+RV?" above the leg's high (bull) / below its low (bear), a thin 1 px white box around
 *      the impulse bar, no numbers on the chart; the details (bar time 12-hour CT, "biggest range in N bars", 2.4x the average) in
 *      the hover over the label or the box.
 *    - Impulse tag display per market (coordinator 2026-10-14: the tag alone flipped sign BUILD -> VALIDATE; Abs on the pivot +
 *      impulse is the only consistent lead): lsFlexLevels\LRA-CobImpulse-<MKT>.csv MODE,off | abs | always (default abs = shown only
 *      with Abs on the pivot); PROVEN,1 drops the "?"; PLAIN,0 stops drawing plain "Cob". The tag is always recorded.
 *  NEVER REPAINTS: each CoB is decided once at its close and recorded (lsFlexLevels\CobImpulse\<MKT>-<CONTRACT>-<sec>-<session>.csv);
 *  labels and boxes are drawn from the records only. Status: lsFlexLevels\CobImpulse.status-<MKT>-<CONTRACT>-<sec>.txt.
 *  (0.5.0) Settings window: Cob labels / Impulse bar box, each Show + colour (CobImpulseSettings.h, CobImpulse.settings-<MKT>.txt); the logic
 *  settings stay in the market file. Logic without the SDK: CobImpulseLogic.h (tested).
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
#define LSPERF_PLUGIN "CobImpulse"
#endif
#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG: the settings window never faults, never opens blank
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "CobImpulseLogic.h"
#include "CobOdds.h"                    // (0.4.0) the live odds (OddsTable.h + the lab's facts)
#include "HoverBoard.h"                  // hb::hbCursor (GetCursorPos + WindowFromPoint; carries the user32 pragma)
#include "HostSlot.h"
#include "KeepOutDraw.h"   // (0.5.2) BoxDock.h 1.2.0 keep-out: nothing painted over the HOD/LOD table
#include "ClaimGates.h"                 // (0.3.0) cgate::kShowHvlCushion - the HVL regime text in the gamma hover
#include "MarketState.h"                // (0.4.1) SRC COB for the last closed bar
#include "CobImpulseSettings.h"          // (0.5.0) the settings table (show / hide + colour: label, impulse box)
#include "SettingsHost.h"
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")      // HoverBoard.h's GetCursorPos / WindowFromPoint
#endif
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
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#define CI_VERSION "0.5.3"          // (0.5.2, 2026-10-21 16:05 CT Rassul "fix this, it should be solid") a Cob label or impulse box that would touch the lsHodLod table is not painted there (records unchanged; KeepOutDraw.h; status UNDER_HODLOD) // (0.5.1, Rassul 23:37 CT 2026-10-18 "get rid of the hover over on the gex levels and other places") the Cob label / impulse box hover is OFF (CI_HOVERS; code kept), no TRACK_MOUSE; (0.5.0, 2026-10-18, Rassul "I'll go with your recommendation") a settings window: Cob labels + Impulse bar box, each Show + colour, per market (CobImpulseSettings.h); defaults = what 0.4.1 drew; the plugin = the setVersion literal below; (0.4.1) cimp::VERSION is the LOGIC version (0.4.0, unchanged - only the Market State post was added)
static const bool CI_HOVERS = false;   // (0.5.1) true brings the hover back

static const COLOR C_LABEL = 0x0080FFFF;   // his IRT Cob label colour (light cyan), unchanged
static const COLOR C_BOX = 0x00FFFFFF, C_HOVBG = 0x00111827, C_HOVEDGE = 0x009CA3AF, C_INK = 0x00F3F4F6;
static const int BARS_MAX = 1500;
static const long ALL_MAX = 20000;          // (0.3.0) bars read for the 20-day ADR / prior close / RTH range (his LTF charts hold 15 days)
static const char* const NAME_BULL = "cob_bull_3m_1b";
static const char* const NAME_BEAR = "cob_bear_3m_1b";
static const char* const NAME_ABULL = "Abs_Bull";
static const char* const NAME_ABEAR = "Abs_Bear";

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
static bool statFile(const std::string& p, long long& stamp) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return false;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
#endif
    if ((st.st_mode & S_IFMT) != S_IFREG) return false;
    stamp = (long long)st.st_mtime * 1000003LL + (long long)st.st_size; return true;
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
// bounded whole-file read (64 KB / 512 KB caps); false = missing, unreadable or too big
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
    return r.substr(0, 8);
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
static const char* const REC_FILE_HEADER = cimp::REC_HEADER;
static const size_t REC_FILE_MAX = 512 * 1024, REC_LINES_MAX = 5000;

struct HoverT { std::string key; int l = 0, t = 0, r = 0, b = 0; cimp::Rec rec; };
struct CIState {
    bdk::TableWatch keep; std::string keepNote;   // (0.5.2) the HOD/LOD table on this chart (keep-out) + its status line
    bool busy = false;
    std::string identity, market, symbol, state = "starting", lastStatusText, flags = "not read yet", paramsWhy = "missing";
    int spb = 0; long long session = -1;
    cimp::Params P; long long paramsStamp = -2, paramsCheckMs = -1, lastStatusMs = -1, lastRecordMs = -1;
    long lastCount = 0; long long decidedThrough = 0;        // bar time of the last decided closed bar
    std::map<std::pair<long long, int>, cimp::Rec> recs;     // recorded CoBs (drawn)
    std::vector<std::pair<long long, cimp::Rec>> pending;    // (session, record) waiting to be published
    long long recFails = 0, decided = 0;
    // (0.3.0) the gamma part: the book (cached on its file stamp), the hover sidecar, the chart's tick
    cimp::GBook book; long long bookStamp = -2; std::string bookWhy = "not read yet"; double tick = 0;
    std::map<std::pair<long long, int>, cimp::GCtx> ctx, pendCtx;
    // (0.4.0) the odds table (cached on its file stamp) and the odds decided at each CoB close
    odt::Table odds; long long oddsStamp = -2; std::string oddsWhy = "not read yet";
    std::map<std::pair<long long, int>, codds::Odds> oddsRec, pendOdds;
    // hover
    std::vector<HoverT> targets; bool msHas = false; int msH = 0, msV = 0; unsigned long long msWnd = 0; std::string hovKey;
    // (0.4.1) the Market State entry of this chart
    bool mstPosted = false; std::string mstSym, mstChart; long mstSpb = 0; long long mstBar = -1, mstMs = -1; std::string mstSig;
    ss::Store setStore; ss::Values set = ss::defaults(ciset::spec(), -1);   // (0.5.0) CobImpulse.settings-<MKT>.txt (re-read by stamp, at most once a second)
};
static ss::Ids& ciIds() { static ss::Ids I; return I; }   // the row ids IRT gave the table (one DLL = one parameter list, written once in setup)
static std::string ciSettingsPath(const std::string& mkt) { const std::string f = flexDir(); return f.empty() || mkt.empty() ? std::string() : f + "\\CobImpulse.settings-" + mkt + ".txt"; }

class CobImpulse : public cppExtension {
public:
    SS_SAFE_DIALOG(CobImpulse, "CobImpulse")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    CobImpulse() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    // (0.5.0) the settings window: filled from the market's file (opening never saves); Apply / a change saves; the chart copy never saves
    virtual int parmsLoadBody(void) {
        if (!ss::inDialog(this)) return RTX_OK;
        ss::Store* st = dialogStore(); if (!st) return RTX_OK;
        ss::fillDialog(this, ciset::spec(), ciIds(), st->load(ss::marketIndex(dlgMkt_), steadyMs(), true), snap_);
        return RTX_OK;
    }
    virtual int parmsApplyBody(void) { saveSettings(); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::onUpdate(this, ciset::spec(), ciIds(), (int)i, snap_); saveSettings(); return RTX_OK; }
    void pump(bool mayCalc);
    void release() {   // (0.4.1) the Market State entry leaves the board first
        CIState* S = slot_.get(this, false);
        if (S && S->mstPosted) { mst::clear("COB", S->mstSym, S->mstSpb, S->mstChart); S->mstPosted = false; }
        slot_.release(this);
    }
    CIState* testState() { return slot_.get(this, false); }
private:
    HostSlot<CIState> slot_;
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        char b[64]; std::memset(b, 0, sizeof(b)); const char* rs = getRootSymbol(b); const char* sy = getSymbol();
        const std::string root = rs ? rs : "", mkt = marketName(root.empty() ? (sy ? sy : "") : root);   // as identify()
        const std::string p = ciSettingsPath(mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &ciset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, ciset::spec(), ciIds(), st->load(ss::marketIndex(dlgMkt_), steadyMs(), true), snap_);   // untouched -> the file's value
        makeDir(flexDir());
        if (st->save(V)) snap_.take(ss::dialogNow(this, ciset::spec(), ciIds(), V));
    }
    void loadSettings(CIState& S) {   // (0.5.0) the market's settings file (keyed); missing / bad = the defaults (what 0.4.1 drew)
        const std::string p = ciSettingsPath(S.market);
        if (p.empty()) { S.set = ss::defaults(ciset::spec(), -1); return; }
        if (S.setStore.path != p) { S.setStore = ss::Store(); S.setStore.S = &ciset::spec(); S.setStore.path = p; }
        S.set = S.setStore.load(ss::marketIndex(S.market), steadyMs());
    }
    long long ctSec(RTDATE d);
    long long sessionOf(long long barTime) { const long long cs = ctSec((RTDATE)barTime); return cs > 0 ? (cs - 17LL * 3600) / 86400LL + 1 : -1; }
    void identify(CIState& S);
    void params(CIState& S, long long now);
    const cimp::GBook* readBook(CIState& S);
    const odt::Table* readOdds(CIState& S);
    bool readFlags(const char* name, long count, std::vector<float>& out);
    void decide(CIState& S, long count);
    bool flush(CIState& S);
    void load(CIState& S);
    void status(CIState& S, long long now);
    void render(CIState& S);
    void postMst(CIState& S, long count);     // (0.4.1) SRC COB
    std::string clockOf(long long t);
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + "\\CobImpulse"; }
    std::string stem(CIState& S) { return S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb); }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<CobImpulse*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<CobImpulse*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void)   { ss::registerAll(this, ciset::spec(), ciIds()); return RTX_OK; }   // (0.5.0) the rows from CobImpulseSettings.h
int cppExtension::calc(int)     { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<CobImpulse*>(this)->pump(true); return RTX_OK; }
int CobImpulse::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    pump(false);
    CIState* S = slot_.get(this, false);
    if (S) { try {
        S->keep.refresh(getSymbol(), getSecondsPerBar(), getChartLabel());   // (0.5.2) the HOD/LOD table: nothing painted over it this draw
        bdk::KeepScope keepScope(S->keep);
        render(*S); S->keepNote = bdk::keepLine();
    } catch (...) { S->state = "draw error"; } }
    return RTX_OK;
}
long long CobImpulse::ctSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    return civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
}
std::string CobImpulse::clockOf(long long t) {
    struct tm m; std::memset(&m, 0, sizeof(m));
    if (t <= 0 || !getLocaltime((RTDATE)t, &m)) return "?";
    return cimp::clock12(m.tm_hour, m.tm_min);
}
void CobImpulse::identify(CIState& S) {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    const std::string id = root + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity) return;
    S.identity = id; S.symbol = sym; S.spb = spb; S.market = marketName(root.empty() ? sym : root);
    S.recs.clear(); S.pending.clear(); S.ctx.clear(); S.pendCtx.clear(); S.oddsRec.clear(); S.pendOdds.clear(); S.oddsStamp = -2; S.oddsWhy = "not read yet"; S.odds = odt::Table(); S.bookStamp = -2; S.bookWhy = "not read yet"; S.lastCount = 0; S.decidedThrough = 0; S.session = -1; S.paramsStamp = -2; S.paramsCheckMs = -1;
    S.targets.clear(); S.hovKey.clear();
}
void CobImpulse::params(CIState& S, long long now) {
    if (S.paramsCheckMs >= 0 && now >= S.paramsCheckMs && now - S.paramsCheckMs < 10000) return;
    S.paramsCheckMs = now;
    const std::string f = flexDir(); if (f.empty()) return;
    const std::string p = f + "\\LRA-CobImpulse-" + S.market + ".csv";
    long long st = -1;
    if (!statFile(p, st)) { if (S.paramsStamp != -1) { S.P = cimp::Params(); S.paramsWhy = "missing (defaults)"; S.paramsStamp = -1; } return; }
    if (st == S.paramsStamp) return;
    std::string text, why;
    if (!readSmall(p, 64 * 1024, text)) { S.paramsWhy = "unreadable or over 64 KB (kept the last good)"; return; }   // retried on the next check
    long long st2 = -1; if (!statFile(p, st2) || st2 != st) return;                                                  // torn: next check
    S.P = cimp::parseFile(text, S.market, why);
    S.paramsWhy = why.empty() ? "file" : why + " (defaults)"; S.paramsStamp = st;
}
// (0.3.0) the MenthorQ book as it is NOW (read at the CoB close): bounded, torn-safe, cached on the file stamp; nullptr = none usable
const cimp::GBook* CobImpulse::readBook(CIState& S) {
    const std::string f = flexDir(); if (f.empty()) { S.bookWhy = "no lsFlexLevels folder"; return nullptr; }
    const std::string p = f + "\\GammaProfile-MQ-" + S.market + ".csv";
    long long st = -1;
    if (!statFile(p, st)) { S.book = cimp::GBook(); S.bookStamp = -1; S.bookWhy = "missing"; return nullptr; }
    if (st != S.bookStamp) {
        std::string text;
        if (!readSmall(p, 64 * 1024, text)) { S.bookWhy = "unreadable or over 64 KB"; return S.book.ok ? &S.book : nullptr; }
        long long st2 = -1; if (!statFile(p, st2) || st2 != st) { S.bookWhy = "being written (kept the last good)"; return S.book.ok ? &S.book : nullptr; }
        cimp::GBook b = cimp::parseBook(text, S.market);
        if (b.ok) { S.book = b; S.bookWhy = "ok"; } else { S.book = cimp::GBook(); S.bookWhy = b.why; }
        S.bookStamp = st;
    }
    return S.book.ok ? &S.book : nullptr;
}
// (0.4.0) the CoB Lab's odds as they are NOW (read at the CoB close): bounded, torn-safe, cached on the file stamp; nullptr = none usable
const odt::Table* CobImpulse::readOdds(CIState& S) {
    const std::string f = flexDir(); if (f.empty()) { S.oddsWhy = "no lsFlexLevels folder"; return nullptr; }
    const std::string p = f + "\\LRA-Odds-" + S.market + ".csv";
    long long st = -1;
    if (!statFile(p, st)) { S.odds = odt::Table(); S.oddsStamp = -1; S.oddsWhy = "missing"; return nullptr; }
    if (st != S.oddsStamp) {
        std::string text;
        if (!readSmall(p, odt::MAX_BYTES, text)) { S.oddsWhy = "unreadable or over 512 KB"; return S.odds.ok ? &S.odds : nullptr; }
        long long st2 = -1; if (!statFile(p, st2) || st2 != st) { S.oddsWhy = "being written (kept the last good)"; return S.odds.ok ? &S.odds : nullptr; }
        odt::Table t = odt::parse(text, S.market);
        if (t.ok) { S.odds = t; S.oddsWhy = std::string("ok, built ") + t.built; } else { S.odds = odt::Table(); S.oddsWhy = t.why; }
        S.oddsStamp = st;
    }
    return S.odds.ok ? &S.odds : nullptr;
}
// his RTL by name, aligned so element i is chart bar i (the array ends on the chart's last bar); false = not readable
bool CobImpulse::readFlags(const char* name, long count, std::vector<float>& out) {
    out.assign((size_t)count, 0.0f);
    char nb[96]; std::memset(nb, 0, sizeof(nb)); std::strncpy(nb, name, sizeof(nb) - 1);
    RTARRAY a(fEmptyArray);
    if (a.getCustomIndicator(nb, 1, true) != RTX_OK || a.count <= 0 || a.count > count) return false;
    const long shift = count - (long)a.count;
    for (long i = shift; i < count; ++i) { const float v = a[(int)(i - shift)]; out[(size_t)i] = (v == v) ? v : 0.0f; }
    return true;
}
void CobImpulse::decide(CIState& S, long count) {
    if (count < 4) return;
    RTARRAYI times(barDateTime), vol(barVolume); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if (times.count < count || vol.count < count || hi.count < count || lo.count < count || op.count < count || cl.count < count) { S.state = "bars not ready"; return; }
    const long lastClosed = count - 2;
    if (S.decidedThrough > 0 && (long long)times[(int)lastClosed] <= S.decidedThrough) return;
    std::vector<float> fb, fs, ab, as;
    const bool okB = readFlags(NAME_BULL, count, fb), okS = readFlags(NAME_BEAR, count, fs);
    const bool okAB = readFlags(NAME_ABULL, count, ab), okAS = readFlags(NAME_ABEAR, count, as);
    S.flags = std::string(NAME_BULL) + (okB ? " ok" : " NOT FOUND") + ", " + NAME_BEAR + (okS ? " ok" : " NOT FOUND") + ", "
            + NAME_ABULL + (okAB ? " ok" : " NOT FOUND") + ", " + NAME_ABEAR + (okAS ? " ok" : " NOT FOUND");
    if (!okB && !okS) { S.state = "CoB RTL not readable - nothing decided (retried next bar)"; return; }   // never advance blind
    const long first = std::max(0L, count - 1 - BARS_MAX);
    std::vector<cimp::Bar> B; B.reserve((size_t)(count - first));
    for (long i = first; i <= lastClosed; ++i) {
        cimp::Bar b; b.t = (long long)times[(int)i]; b.o = op[(int)i]; b.h = hi[(int)i]; b.l = lo[(int)i]; b.c = cl[(int)i];
        b.v = (double)vol[(int)i]; B.push_back(b);
    }
    // (0.3.0) the gamma inputs come from the whole chart (closed bars): built once, only when a CoB is to be decided
    bool gReady = false; long firstAll = 0; std::vector<cimp::Bar> A; std::vector<long long> ct; std::vector<cimp::DayAgg> D; const cimp::GBook* book = nullptr;
    int rthFirst = 0, rthLast = 0; const bool hasRth = cimp::rthWindow(S.market, rthFirst, rthLast);
    for (long i = first + 2; i <= lastClosed; ++i) {
        const long long bt = (long long)times[(int)i];
        if (bt <= S.decidedThrough) continue;
        for (int s = 0; s < 2; ++s) {
            const int side = s == 0 ? 1 : -1;
            const bool okFlag = side > 0 ? okB : okS;
            if (!okFlag || (side > 0 ? fb : fs)[(size_t)i] < 0.5f) continue;
            if (S.recs.count(std::make_pair(bt, side))) continue;
            bool queued = false; for (size_t q = 0; q < S.pending.size(); ++q) if (S.pending[q].second.trig == bt && S.pending[q].second.side == side) queued = true;
            if (queued) continue;
            const int k = (int)(i - first);
            const int p = cimp::legPivot(B, k, side);
            bool abs = false;
            if (p >= 0) { const std::vector<float>& Af = side > 0 ? ab : as; const bool okA = side > 0 ? okAB : okAS; abs = okA && Af[(size_t)(p + first)] >= 0.5f; }
            const cimp::Tag T = cimp::evaluate(B, k, side, S.P, abs);
            // (0.3.0) the gamma part, decided now (the CoB bar has just closed) from the book as it is now
            if (!gReady) {
                gReady = true; firstAll = std::max(0L, count - 1 - ALL_MAX);
                A.reserve((size_t)(lastClosed - firstAll + 1)); ct.reserve(A.capacity());
                for (long a = firstAll; a <= lastClosed; ++a) {
                    cimp::Bar b; b.t = (long long)times[(int)a]; b.o = op[(int)a]; b.h = hi[(int)a]; b.l = lo[(int)a]; b.c = cl[(int)a]; b.v = (double)vol[(int)a];
                    A.push_back(b); ct.push_back(ctSec((RTDATE)b.t));
                }
                D = cimp::dailyTable(ct, A);
                book = readBook(S);
            }
            cimp::GTag G;
            const long ai = i - firstAll;
            if (p < 0 || ai < 0 || ai >= (long)A.size() || ct[(size_t)ai] <= 0) { G.computed = true; G.why = "no pivot"; }
            else {
                cimp::GIn in; in.bull = side > 0; in.ext = side > 0 ? B[(size_t)p].l : B[(size_t)p].h; in.entry = B[(size_t)k].c; in.tick = S.tick;
                in.trigCt = ct[(size_t)ai]; in.sessDay = cimp::sessDayOf(in.trigCt);
                const int mod = cimp::minuteOf(in.trigCt);
                in.inRth = hasRth && mod >= rthFirst && mod <= rthLast;
                size_t di = D.size();
                for (size_t q = 0; q < D.size(); ++q) if (D[q].day == in.sessDay) { di = q; break; }
                if (di < D.size()) {
                    in.adr = cimp::adrAt(D, di);
                    if (di > 0 && D[di - 1].has15) { in.hasClose15 = true; in.close15 = D[di - 1].c15; }
                }
                if (hasRth) for (long a = ai; a >= 0 && cimp::sessDayOf(ct[(size_t)a]) == in.sessDay; --a) {
                    const int m2 = cimp::minuteOf(ct[(size_t)a]);
                    if (m2 < rthFirst || m2 > rthLast || !cimp::barOk(A[(size_t)a])) continue;
                    in.rthHi = std::max(in.rthHi, A[(size_t)a].h); in.rthLo = std::min(in.rthLo, A[(size_t)a].l);
                }
                G = cimp::gammaTag(in, book, S.P);
            }
            // (0.4.0) the live odds, decided now from the lab's table as it is now (same facts as lra.cob_lab.features)
            codds::Odds O; O.decided = true;
            if (ai >= 0 && ai < (long)A.size() && hasRth) {
                const int legBars = (T.pivot >= 0 && T.legEnd >= T.pivot) ? T.legEnd - T.pivot : -1;
                const codds::Facts F = codds::facts(A, ct, ai, side, S.tick, rthFirst, rthLast, legBars);
                const odt::Table* ot = readOdds(S);
                if (ot && !codds::afterBuilt(*ot, ct[(size_t)ai])) O.decided = false;   // (0.4.0) an older CoB: no in-sample odds
                else O = codds::decide(ot ? *ot : odt::Table(), F);
            }
            const long long sess = sessionOf(bt);
            if (sess < 0 || S.pending.size() >= REC_LINES_MAX) { ++S.recFails; continue; }
            S.pending.push_back(std::make_pair(sess, cimp::toRec(T, B, &G))); ++S.decided;
            if (G.computed && G.why.empty()) S.pendCtx[std::make_pair(bt, side)] = cimp::toCtx(G, bt, side);
            S.pendOdds[std::make_pair(bt, side)] = O;
        }
    }
    S.decidedThrough = (long long)times[(int)lastClosed];
    S.state = "ready";
    flush(S);
}
// (0.3.0) merge-publish the hover sidecar of one session: old lines kept (first wins), the new ones appended; false = not written
static bool publishCtx(const std::string& path, const std::vector<cimp::GCtx>& add) {
    std::string old; long long st = -1; std::vector<std::string> keep; std::set<std::pair<long long, int>> seen;
    if (statFile(path, st)) {
        if (!readSmall(path, REC_FILE_MAX, old)) return false;
        std::istringstream in(old); std::string line; bool head = true;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (head) { head = false; if (line != cimp::GCTX_HEADER) return false; continue; }
            cimp::GCtx c; if (cimp::parseCtx(line, c) && seen.insert(std::make_pair(c.trig, c.side)).second) keep.push_back(cimp::ctxLine(c));
        }
        if (head && !old.empty()) return false;
    }
    std::string text = std::string(cimp::GCTX_HEADER) + "\n";
    for (const auto& k : keep) text += k + "\n";
    for (const auto& c : add) if (seen.insert(std::make_pair(c.trig, c.side)).second) text += cimp::ctxLine(c) + "\n";
    return text.size() <= REC_FILE_MAX && publishFile(path, text);
}
// (0.4.0) merge-publish the odds sidecar of one session: old lines kept (first wins), the new ones appended; false = not written
static bool publishOdds(const std::string& path, const std::vector<std::pair<std::pair<long long, int>, codds::Odds>>& add) {
    std::string old; long long st = -1; std::vector<std::string> keep; std::set<std::pair<long long, int>> seen;
    if (statFile(path, st)) {
        if (!readSmall(path, REC_FILE_MAX, old)) return false;
        std::istringstream in(old); std::string line; bool head = true;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (head) { head = false; if (line != codds::SIDECAR_HEADER) return false; continue; }
            long long t = 0; int s = 0; codds::Odds o; if (codds::parseSidecar(line, t, s, o) && seen.insert(std::make_pair(t, s)).second) keep.push_back(codds::sidecarLine(t, s, o));
        }
        if (head && !old.empty()) return false;
    }
    std::string text = std::string(codds::SIDECAR_HEADER) + "\n";
    for (const auto& k : keep) text += k + "\n";
    for (const auto& a : add) if (seen.insert(a.first).second) text += codds::sidecarLine(a.first.first, a.first.second, a.second) + "\n";
    return text.size() <= REC_FILE_MAX && publishFile(path, text);
}
bool CobImpulse::flush(CIState& S) {
    if (S.pending.empty()) return true;
    const long long now = steadyMs();
    if (S.lastRecordMs >= 0 && now >= S.lastRecordMs && now - S.lastRecordMs < 1000) return false;   // a failed publish is retried after 1 s
    const std::string d = dir(); if (d.empty()) { ++S.recFails; S.lastRecordMs = now; return false; }
    makeDir(flexDir()); makeDir(d);
    std::map<long long, std::vector<size_t>> groups;
    for (size_t i = 0; i < S.pending.size(); ++i) groups[S.pending[i].first].push_back(i);
    std::vector<char> done(S.pending.size(), 0);
    for (const auto& g : groups) {
        const std::string path = d + "\\" + stem(S) + "-" + std::to_string(g.first) + ".csv";
        std::string old; long long st = -1; std::vector<std::string> keep; std::set<std::pair<long long, int>> seen;
        if (statFile(path, st)) {
            if (!readSmall(path, REC_FILE_MAX, old)) { ++S.recFails; continue; }   // never rewrite a file we could not read
            std::istringstream in(old); std::string line; bool head = true, badHead = false;
            while (std::getline(in, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (head) { head = false; if (!cimp::knownHeader(line)) { badHead = true; break; } continue; }
                cimp::Rec r; if (cimp::parseRec(line, r) && seen.insert(std::make_pair(r.trig, r.side)).second) keep.push_back(cimp::recLine(r));
            }
            if (badHead || (head && !old.empty())) { ++S.recFails; continue; }   // never rewrite a file we do not understand
        }
        std::string text = std::string(REC_FILE_HEADER) + "\n";
        for (const auto& k : keep) text += k + "\n";
        for (size_t j : g.second) { const cimp::Rec& r = S.pending[j].second; if (seen.insert(std::make_pair(r.trig, r.side)).second) text += cimp::recLine(r) + "\n"; }
        if (text.size() > REC_FILE_MAX || !publishFile(path, text)) { ++S.recFails; continue; }
        // (0.3.0) the hover sidecar of the same session; a CoB is done only when both are written (a retry rewrites the same lines)
        std::vector<cimp::GCtx> add;
        for (size_t j : g.second) { const cimp::Rec& r = S.pending[j].second; const auto it = S.pendCtx.find(std::make_pair(r.trig, r.side)); if (it != S.pendCtx.end()) add.push_back(it->second); }
        if (!add.empty() && !publishCtx(d + "\\" + stem(S) + "-" + std::to_string(g.first) + ".gamma.csv", add)) { ++S.recFails; continue; }
        // (0.4.0) the odds sidecar of the same session
        std::vector<std::pair<std::pair<long long, int>, codds::Odds>> addO;
        for (size_t j : g.second) { const cimp::Rec& r = S.pending[j].second; const auto it = S.pendOdds.find(std::make_pair(r.trig, r.side)); if (it != S.pendOdds.end()) addO.push_back(*it); }
        if (!addO.empty() && !publishOdds(d + "\\" + stem(S) + "-" + std::to_string(g.first) + ".odds.csv", addO)) { ++S.recFails; continue; }
        for (size_t j : g.second) {
            const cimp::Rec& r = S.pending[j].second; const std::pair<long long, int> key(r.trig, r.side);
            if (!S.recs.count(key)) {
                S.recs[key] = r;
                const auto it = S.pendCtx.find(key); if (it != S.pendCtx.end() && !S.ctx.count(key)) S.ctx[key] = it->second;
                const auto io = S.pendOdds.find(key); if (io != S.pendOdds.end() && !S.oddsRec.count(key)) S.oddsRec[key] = io->second;
            }
            S.pendCtx.erase(key); S.pendOdds.erase(key); done[j] = 1;
        }
    }
    std::vector<std::pair<long long, cimp::Rec>> retry;
    for (size_t i = 0; i < S.pending.size(); ++i) if (!done[i]) retry.push_back(S.pending[i]);
    S.pending.swap(retry);
    S.lastRecordMs = S.pending.empty() ? -1 : now;
    return S.pending.empty();
}
void CobImpulse::load(CIState& S) {
    const std::string d = dir(); if (d.empty()) return;
    for (long long k = S.session - 2; k <= S.session; ++k) {
        const std::string path = d + "\\" + stem(S) + "-" + std::to_string(k) + ".csv";
        long long st = -1; if (!statFile(path, st)) continue;
        std::string text; if (!readSmall(path, REC_FILE_MAX, text)) { ++S.recFails; continue; }
        std::istringstream in(text); std::string line; bool head = true; size_t n = 0;
        while (std::getline(in, line) && ++n <= REC_LINES_MAX) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (head) { head = false; if (!cimp::knownHeader(line)) break; continue; }
            cimp::Rec r; if (cimp::parseRec(line, r)) S.recs.insert(std::make_pair(std::make_pair(r.trig, r.side), r));
        }
        // (0.4.0) the odds sidecar
        { const std::string op = d + "\\" + stem(S) + "-" + std::to_string(k) + ".odds.csv"; long long so = -1; std::string ot;
          if (statFile(op, so) && readSmall(op, REC_FILE_MAX, ot)) {
              std::istringstream oi(ot); std::string ol; bool oh = true; size_t on = 0;
              while (std::getline(oi, ol) && ++on <= REC_LINES_MAX) {
                  if (!ol.empty() && ol.back() == '\r') ol.pop_back();
                  if (oh) { oh = false; if (ol != codds::SIDECAR_HEADER) break; continue; }
                  long long t = 0; int s = 0; codds::Odds o; if (codds::parseSidecar(ol, t, s, o)) S.oddsRec.insert(std::make_pair(std::make_pair(t, s), o));
              }
          } }
        // (0.3.0) the hover sidecar
        const std::string cp = d + "\\" + stem(S) + "-" + std::to_string(k) + ".gamma.csv";
        if (!statFile(cp, st)) continue;
        if (!readSmall(cp, REC_FILE_MAX, text)) { ++S.recFails; continue; }
        std::istringstream ci(text); head = true; n = 0;
        while (std::getline(ci, line) && ++n <= REC_LINES_MAX) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (head) { head = false; if (line != cimp::GCTX_HEADER) break; continue; }
            cimp::GCtx c; if (cimp::parseCtx(line, c)) S.ctx.insert(std::make_pair(std::make_pair(c.trig, c.side), c));
        }
    }
}
void CobImpulse::status(CIState& S, long long now) {
    if (S.lastStatusMs >= 0 && now >= S.lastStatusMs && now - S.lastStatusMs < 1000) return;
    S.lastStatusMs = now;
    int tagged = 0, shownN = 0, gTagged = 0;
    for (const auto& kv : S.recs) { if (kv.second.r() || kv.second.v() || kv.second.c()) { ++tagged; if (cimp::shown(true, kv.second.abs, S.P.mode)) ++shownN; }
                                    if (!kv.second.gCode.empty()) ++gTagged; }
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << CI_VERSION << "\n" << S.keepNote << "MARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nSTATE," << S.state
      << "\nFLAGS," << S.flags << "\nPARAMS,mode " << cimp::modeName(S.P.mode) << ",proven " << (S.P.proven ? 1 : 0) << ",plain " << (S.P.plain ? 1 : 0)
      << ",min_bars " << S.P.minBars << ",window " << S.P.window << ",gamma " << (S.P.gamma ? 1 : 0) << ",gamma_w " << S.P.gammaW
      << ",gamma_em " << S.P.gammaEM << "," << S.paramsWhy
      << "\nBOOK,GammaProfile-MQ-" << S.market << ".csv," << S.bookWhy << ",asof " << (S.book.ok ? S.book.asof : 0) << ",tick " << S.tick
      << "\nODDS,LRA-Odds-" << S.market << ".csv," << S.oddsWhy << ",show " << (S.P.odds ? 1 : 0) << ",decided " << S.oddsRec.size()
      << "\nRECORDS," << S.recs.size() << ",tagged " << tagged << ",tag shown " << shownN << ",gamma " << gTagged << ",pending " << S.pending.size() << ",fails " << S.recFails
      << "\nDECIDED_THROUGH," << S.decidedThrough << "\n";
    const std::string text = o.str(); if (text == S.lastStatusText) return;
    const std::string f = flexDir(); if (f.empty()) return;
    if (publishFile(f + "\\CobImpulse.status-" + stem(S) + ".txt", text)) S.lastStatusText = text;
}
void CobImpulse::pump(bool mayCalc) {
    CIState* P = slot_.get(this, true);
    if (!P || P->busy) return;
    CIState& S = *P; S.busy = true;
    struct Unbusy { CIState* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
    try {
        identify(S);
        S.tick = cleanTick(getProperty(SYM_TICKINCR));          // (0.3.0) SYM_TICKINCR is the only tick source
        const long long now = steadyMs();
        const long long ct = ctSec(currentDate());
        if (ct > 0) { const long long key = (ct - 17LL * 3600) / 86400LL + 1; if (key != S.session) { S.session = key; load(S); } }
        params(S, now);
        loadSettings(S);
        const long count = getBarCount();
        // his RTL is read (and recalculated by IRT) only from calc on a new bar, like the Dealer Profile / Chart View
        if (mayCalc && count != S.lastCount) { decide(S, count); S.lastCount = count; }
        else flush(S);
        status(S, now);
        postMst(S, count);                                     // (0.4.1) the Market State (behind the scenes)
    } catch (...) { S.state = "calculation error"; }
}
// (0.4.1) MARKET STATE, SRC "COB": the facts of the last CLOSED bar from the records (a CoB is decided at its close and recorded once:
// no repaint); the same facts re-posted every 10 s (MarketState.h drops entries older than 60 s), at once on a new bar / a new record
static std::string ciNum4(double x) { char b[32]; std::snprintf(b, sizeof(b), "%.4g", x); return b; }
typedef std::map<std::pair<long long, int>, cimp::Rec> CiRecs;
typedef std::map<std::pair<long long, int>, codds::Odds> CiOdds;
// the facts for bar time t (the chart's RTDATE stamp, as the records key it): a recorded (or queued) CoB on that bar, bull first
static std::map<std::string, std::string> ciMstFacts(const CiRecs& recs, const std::vector<std::pair<long long, cimp::Rec>>& pending,
                                                     const CiOdds& odds, const CiOdds& pendOdds, long long t) {
    std::map<std::string, std::string> kv;
    const cimp::Rec* r = nullptr;
    for (int side = 1; side >= -1 && !r; side -= 2) {
        const auto it = recs.find(std::make_pair(t, side)); if (it != recs.end()) { r = &it->second; break; }
        for (const auto& p : pending) if (p.second.trig == t && p.second.side == side) { r = &p.second; break; }
    }
    kv["cob"] = r ? (r->side > 0 ? "bull" : "bear") : "none";
    const codds::Odds* o = nullptr;
    if (r) { const auto k = std::make_pair(r->trig, r->side); auto io = odds.find(k); if (io != odds.end()) o = &io->second; else { io = pendOdds.find(k); if (io != pendOdds.end()) o = &io->second; } }
    const bool known = o && o->decided && o->p >= 0;
    kv["cob_odds"] = (known && o->live) ? ciNum4(o->p) : "?";          // only what the label shows (LIVE); learning / pooled-only = '?'
    kv["cob_odds_base"] = (o && o->decided && o->base > 0) ? ciNum4(o->base) : "?";
    kv["cob_odds_n"] = known ? std::to_string(o->n) : "?";
    std::string tag; if (r) { if (r->r()) tag += "R"; if (r->v()) tag += "V"; if (r->c()) tag += "C"; }
    kv["cob_tag"] = tag;
    kv["cob_gamma"] = r ? r->gCode : std::string();
    return kv;
}
void CobImpulse::postMst(CIState& S, long count) {
    if (count < 3 || S.symbol.empty()) return;
    RTARRAYI times(barDateTime); if (times.count < count) return;
    const long long t = (long long)times[(int)count - 2];
    const long long bar = ctSec((RTDATE)t); if (bar <= 0) return;
    const char* cl = getChartLabel(); const std::string chart = cl ? cl : "";
    const std::map<std::string, std::string> kv = ciMstFacts(S.recs, S.pending, S.oddsRec, S.pendOdds, t);
    std::string sig; for (const auto& p : kv) sig += p.first + "=" + p.second + ";";
    const long long now = steadyMs();
    if (S.mstPosted && (S.symbol != S.mstSym || S.spb != S.mstSpb || chart != S.mstChart)) { mst::clear("COB", S.mstSym, S.mstSpb, S.mstChart); S.mstPosted = false; }
    const bool changed = bar != S.mstBar || sig != S.mstSig;
    if (!changed && S.mstPosted && S.mstMs >= 0 && now >= S.mstMs && now - S.mstMs < 10000) return;
    mst::publish("COB", S.symbol, S.spb, chart, now, bar, kv);
    S.mstPosted = true; S.mstSym = S.symbol; S.mstSpb = S.spb; S.mstChart = chart; S.mstBar = bar; S.mstSig = sig; S.mstMs = now;
}
// the bar index whose time is t (closed bars only), -1 = none
template <class Times> static int indexOf(Times& times, long count, long long t) {
    long lo = 0, hi = count - 2;
    while (lo <= hi) {
        const long mid = lo + (hi - lo) / 2; const long long v = (long long)times[(int)mid];
        if (v == t) return (int)mid;
        if (v < t) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}
void CobImpulse::render(CIState& S) {
    S.targets.clear();
    RCT pane; pane.getPaneRect(false);
    const long count = getBarCount();
    if (count < 3 || S.recs.empty() || pane.bottom - pane.top < 40) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow);
    if (times.count < count || hi.count < count || lo.count < count) return;
    FONT f; f.id = HELVETICA; f.size = 10; f.style = BOLD; setFont(f);
    for (const auto& kv : S.recs) {
        const cimp::Rec& r = kv.second;
        const bool tag = cimp::shown(r.r() || r.v() || r.c(), r.abs, S.P.mode);   // the impulse part is shown (its bar gets the box)
        const std::string base = cimp::label(r.r(), r.v(), r.abs, S.P, r.c(), r.gCode);   // (0.3.0) + the gamma name
        if (base == "Cob" && !S.P.plain) continue;
        const auto io = S.oddsRec.find(kv.first);
        const std::string text = base + (io != S.oddsRec.end() ? codds::labelSuffix(io->second, S.P.odds) : std::string());   // (0.4.0) "Cob 63%"
        const int ti = indexOf(times, count, r.trig); if (ti < 0) continue;
        int ai = r.legEnd > 0 ? indexOf(times, count, r.legEnd) : -1; if (ai < 0) ai = ti > 0 ? ti - 1 : ti;
        PNT x0; x0.set(ai, 0.0f, kBarCenter); if (x0.h < pane.left - 40 || x0.h > pane.right + 40) continue;
        const float ext = r.side > 0 ? hi[ai] : lo[ai];
        PNT pe; pe.set(ai, ext, kBarCenter);
        const int w = getTextWidth(text.c_str(), -1) + 4, h = 14;
        const cimp::Box lb = cimp::labelBox(pe.h, pe.v, r.side, w, h, 30, pane.top, pane.bottom);
        HoverT H; H.key = std::to_string(r.trig) + (r.side > 0 ? "b" : "s"); H.l = lb.l; H.t = lb.t; H.r = lb.r; H.b = lb.b; H.rec = r;
        if (S.set.b(ciset::K_SHOW_LABELS)) {                              // (0.5.0) Cob labels: Show + colour
            setTextColor((COLOR)S.set.c(ciset::K_LABEL_COLOR));
            RCT tr; tr.set(coord(lb.l), coord(lb.t), coord(lb.r), coord(lb.b));
            if (!ko::off(tr)) { tr.drawText(text.c_str(), true, false); S.targets.push_back(H); }   // (0.5.2) under the HOD/LOD table: not painted (still recorded)
        }
        if (!tag || !S.set.b(ciset::K_SHOW_BOX)) continue;                // (0.5.0) Impulse bar box: Show + colour
        // the thin white box around the impulse bar(s)
        const long long bars[3] = { r.rT, r.vT, r.cT };
        for (int k = 0; k < 3; ++k) {
            if (bars[k] <= 0 || (k >= 1 && bars[k] == bars[0]) || (k == 2 && bars[2] == bars[1])) continue;
            const int bi = indexOf(times, count, bars[k]); if (bi < 0) continue;
            PNT a; a.set(bi, hi[bi], kBarLeft); PNT b; b.set(bi, lo[bi], kBarRight);
            const int l = std::min(a.h, b.h) - 2, rr = std::max(a.h, b.h) + 2, t = std::min(a.v, b.v) - 2, bt = std::max(a.v, b.v) + 2;
            RCT box; box.set(coord(l), coord(t), coord(rr), coord(bt)); if (ko::off(box)) continue;   // (0.5.2) under the HOD/LOD table: not painted
            box.draw(1, (COLOR)S.set.c(ciset::K_BOX_COLOR), (COLOR)S.set.c(ciset::K_BOX_COLOR), DRAW_OPAQUE, PAT_HOLLOW);
            HoverT HB = H; HB.l = l; HB.t = t; HB.r = rr; HB.b = bt; S.targets.push_back(HB);
        }
    }
    // the hover: only while the cursor is over this chart's window and over a label / box
    if (!CI_HOVERS || !S.msHas || S.msWnd == 0 || hb::hbCursor() != S.msWnd) { S.hovKey.clear(); return; }   // (0.5.1) CI_HOVERS
    const HoverT* T = nullptr;
    for (const auto& t : S.targets) if (S.msH >= t.l && S.msH <= t.r && S.msV >= t.t && S.msV <= t.b) { T = &t; break; }
    S.hovKey = T ? T->key : std::string();
    if (!T) return;
    cimp::HoverIn in; in.side = T->rec.side; in.r = T->rec.r(); in.v = T->rec.v(); in.abs = T->rec.abs;
    in.rn = T->rec.rn; in.vn = T->rec.vn; in.rx = T->rec.rx; in.vx = T->rec.vx; in.rTime = clockOf(T->rec.rT); in.vTime = clockOf(T->rec.vT);
    in.c = T->rec.c(); in.cn = T->rec.cn; in.cx = T->rec.cx; in.cTime = clockOf(T->rec.cT); in.cConfTime = clockOf(T->rec.cConf);
    { const auto it = S.ctx.find(std::make_pair(T->rec.trig, T->rec.side)); in.g = cimp::fromRec(T->rec, it != S.ctx.end() ? &it->second : nullptr); }
    in.tick = S.tick; in.showRegime = cgate::kShowHvlCushion;       // (0.3.0) the gamma lines; the HVL regime text follows the claim gate
    if (in.g.asof > 0) {                                              // (0.3.1) the book's age at the CoB close, "data N min old" (CT seconds both)
        struct tm m; std::memset(&m, 0, sizeof(m));
        if (T->rec.trig > 0 && getLocaltime((RTDATE)T->rec.trig, &m)) {
            const long long tct = cimp::civil(m.tm_year + 1900, (unsigned)(m.tm_mon + 1), (unsigned)m.tm_mday) * 86400LL + m.tm_hour * 3600 + m.tm_min * 60 + m.tm_sec;
            if (tct >= in.g.asof && tct - in.g.asof < 86400) in.bookAgeS = tct - in.g.asof;
        }
    }
    std::vector<std::string> L = cimp::hover(in, S.P);
    L.insert(L.begin() + 1, std::string("CoB ") + (T->rec.side > 0 ? "long" : "short") + " at " + clockOf(T->rec.trig) + " (leg from the " + clockOf(T->rec.pivot) + " bar).");
    { const auto io = S.oddsRec.find(std::make_pair(T->rec.trig, T->rec.side));   // (0.4.0) the live odds, as decided at the CoB close
      L.insert(L.begin() + 2, io != S.oddsRec.end() ? codds::hoverLine(io->second) : std::string("Odds: not decided (this CoB was recorded before odds existed).")); }
    FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
    int w = 0; for (const auto& s : L) w = std::max(w, getTextWidth(s.c_str(), -1));
    w += 16; const int lh = 15, h = (int)L.size() * lh + 10;
    int l = S.msH + 14, t = S.msV + 14;
    if (l + w > pane.right - 2) l = S.msH - 14 - w;
    if (l < pane.left + 2) l = pane.left + 2;
    if (t + h > pane.bottom - 2) t = pane.bottom - 2 - h;
    if (t < pane.top + 2) t = pane.top + 2;
    RCT bg; bg.set(coord(l), coord(t), coord(l + w), coord(t + h)); bg.draw(1, C_HOVEDGE, C_HOVBG, DRAW_OPAQUE, PAT_SOLID);
    for (size_t i = 0; i < L.size(); ++i) {
        setTextColor(i == 0 ? C_LABEL : C_INK);
        RCT tr; tr.set(coord(l + 8), coord(t + 5 + (int)i * lh), coord(l + w - 4), coord(t + 5 + (int)(i + 1) * lh)); tr.drawText(L[i].c_str(), false, false);
    }
}
// the mouse: remember where it is; redraw only when the hovered label / box changes. Never swallows the event.
int CobImpulse::mouse(RTX_EVENT* e) {
    if (!CI_HOVERS) return RTX_FAIL;                                    // (0.5.1) hovers off: the mouse is never used
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    CIState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    S->msHas = true; S->msH = e->v.mouse.h; S->msV = e->v.mouse.v; S->msWnd = hb::hbCursor();
    std::string k;
    for (const auto& t : S->targets) if (S->msH >= t.l && S->msH <= t.r && S->msV >= t.t && S->msV <= t.b) { k = t.key; break; }
    if (k != S->hovKey) { S->hovKey = k; invalidateChart(false); }
    return RTX_FAIL;
}

extern "C" cppExtension* CreateExtension(void) {
    CobImpulse* p = new CobImpulse();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | (CI_HOVERS ? TRACK_MOUSE : 0) | CALC_LAST);   // (0.5.1) no TRACK_MOUSE while the hovers are off   // CALC_LAST: it reads his RTL results
    p->setDescription("LRA Cob Impulse v0.4.1: what are the odds this CoB reaches 1R (shown after the label, e.g. Cob 63%, from the nightly CoB Lab, LRA-Odds-<MKT>.csv; ODDS 0 hides it), did the CoB leg start with force, and is a MenthorQ level near its start? Labels each CoB (his cob_bull_3m_1b / cob_bear_3m_1b) Cob, Cob+ R / V / C and / or + a gamma name (e.g. Cob+RV?, Cob+R+CR?, Cob+GM?): R = a bar from the leg's pivot to 2 bars after it, inside the pivot bar, with the biggest range in 35+ bars; V = the biggest volume in 35+ bars; C = a climax at the origin (an opposite-colour biggest-volume bar from 2 bars before to 2 after the pivot, then a close beyond it). Gamma = ONE MenthorQ level from GammaProfile-MQ-<MKT>.csv at the CoB close: the leg started in its zone, crossed the HVL, or a big node is ahead within the expected move (CR PS HVL CR0 PS0 HVL0 GW0, GM = a top-3 gamma strike); always ? (still testing). A thin white box marks the impulse bar. Per-market file LRA-CobImpulse-<MKT>.csv: MODE off / abs / always; PROVEN 1 drops the impulse ?; GAMMA 1 / 0, GAMMA_W, GAMMA_EM. Decided at the CoB's close, recorded, never repainted.");
    p->setVersion("0.5.3");   // CI_VERSION
    return p;
}
