/********************************************************************************
 *  ZoneDefense.cpp  --  Investor/RT RTX extensions  lsZoneDefense 1.0.9  +  lsZoneDefenseMarks 1.0.9  (2026-11-10)
 *  1.0.9 (Rassul 08:19 CT): a held zone whose orders were pulled shows "held -> pulled" (was stuck on "held"); 60 min untouched = "expired"
 *  1.0.8 (Rassul 15:27 CT "if mbo is quiet, display a message on the chart somewhere"; mockup LPhVzXwuELSZaCFPmZzwEV): "MBO QUIET 12m" in the
 *  top-right of the pane when no order event arrived for 2 min (07:00-15:00 CT) / 5 min (overnight) while Globex is open (MboQuiet.h);
 *  clears when events resume. A live status, nothing recorded.
 *  1.0.7 (Rassul 09:35 ES live "the zone text is too long, make the zone slightly lighter"; picked panel 4 of
 *  claude.ai/artifact/EaKHxKQXTnxgg3ZMU9aeRV): the label is short - lots + the level the zone is AT ("301 PS0", "301" when none) -
 *  same place (under a bid band / over an offer band, 8 px); the band ~22% lighter. Diamond / triangle unchanged.
 *  1.0.6 (Rassul 09:03 CT, NQ live): the bars at full pane height with the read on top in an opaque box sized to the text; every
 *  drawn text plain ASCII ("|", "-", "->" - his pane showed mojibake for the middle dot / dash / arrow).
 *  1.0.5 (Rassul 2026-11-09, option (a)): the floors come from the LIVE recorded books (the nightly's live-floor step, p60 of the zones
 *  the live book shows, last 10 recorded sessions; never below 0.3x the research floor) - the research floors (complete Databento books)
 *  were 2-3x what a since-connect Rithmic book can show, so ES drew 0 zones. Levels fix: the level files are judged on the real UTC clock.
 *  1.0.4 (live bug, Sun 19:51 CT): the Central-clock check read "UTC+0" on a Central PC - the order stamps are already local inside
 *  IRT. The PC zone now comes from GetTimeZoneInformation; unknown = no warning.
 *  1.0.3 (Rassul approved 2026-11-08): a zone AT his levels (key, volume, MenthorQ within the level reach) needs less size - the
 *  market's research floor or levelFloorX x the chart floor (his rule 0.5x), whichever is higher (ZoneDefenseParams.h). The noise
 *  check (13 fixture days) added > 10 zones/day in ES NQ CL GC HG NG at 0.5-0.8x, so those markets ship CAPPED (levelFloorX 1.0 =
 *  the chart floor, as 1.0.2); EU is on at 0.5x. Bar sizes (L4), the Central-clock warning (L6), LRA-VolLevels.
 *  1.0.2 (independent code review, Rassul "make the code solid"): one clock for live and research (whole seconds, no arrival ms -
 *  the IRT stamps are seconds and its reads come in 250 ms batches); markets through mkey::norm (QGC, @ES, EP, QCL ...); a stopped /
 *  removed / restarted pane closes its open zones (ZE code 5) and a reopened journal closes orphans; ONE writer per journal file
 *  (process registry + lock file), other copies read-only; reconnect / symbol change resets the book; the book is kept across the
 *  17:00 roll, a cold start warms 10 min; implied fills ignored; a price-moving MOD counts as a cancel at the old price; a torn last
 *  journal line is closed; Zone state agrees with Pressure (forming / held / tested / broken / pulled); the pane bars show the latest
 *  touched zone only; the marks map a time to the bar whose END is after it (IRT stamps bar ENDS inside the plugin), honour the
 *  sweep sign and draw the diamond only where the zone refilled; clear messages for no order feed / unknown market / no bar time.
 *  1.0.2 levels (Rassul 2026-11-08 "zones at key levels, volume levels and MenthorQ"): the zone label adds "at VAL + PS0" when the
 *  zone sits at his levels (overlapping, or out to levelReach x the 3-min ATR beyond a support / resistance level - his 10-27 rule;
 *  the nightly tunes the reach per market). Live levels = the lsFlexLevels files (LRA-KeyLevels / LRA-Dealer / LRA-Levels, <= 18 h,
 *  the chart's contract), re-read every 5 s. Zone states (held / reclaimed / broken / broken -> reclaimed, 10-bar window). Memory: only
 *  orders that rested 30 s+ are kept after they leave (the only ones the queue read can count) - NQ 1,000,000 -> 12,000, same output.
 *  1.0.1 (Rassul 2026-11-08, mockup claude.ai/artifact/MJ9YYwjeMAUQztNZrCiP1G "before / after"): the Read block gets the Zone line and
 *  every line is NUMBERED in event order - 1. Zone, 2. Queue, 3. Pressure (was "Reload"), 4. Refill, 5. Sweep - in aligned columns (labels left, the
 *  verdict word bold at one x, the detail in a calmer tint on the same line, times right-aligned); the pane bars are dimmed.
 *  Rassul 2026-11-08 "Zone Defense is good. lets build it" - design approved on claude.ai/artifact/MJ9YYwjeMAUQztNZrCiP1G.
 *
 *  lsZoneDefense (this file as is) - its own pane, "Zone Defense v1.0.7", on the live market-by-order feed (the same order-feed path
 *  as lsBookFlow: off::MboPane, the 250 ms clock, every event once through onRaw). Every event goes to zd::Engine
 *  (ZoneDefenseLogic.h, SDK-free, the same code the jumpstart / nightly ran over 71 market-days of raw MBO). Each decision is a
 *  journal line in lsFlexLevels\ZoneDefense\<MKT>-<spb>-<session>.csv, written once; the pane and the price chart draw ONLY from
 *  the journal (ZoneDefenseRead.h) - nothing repaints, a restart redraws the same picture.
 *    The pane: 4 Read lines for the latest touched zone (verdict first, green bullish / red bearish / white neutral, the time
 *    right-justified, 12-hour) - Zone, Queue, Pressure, Refill, Sweep (1.0.1) - and per candle the lots sold into the zone (red bar) and the bid
 *    (offer) left in it (green dot). Mirror for resistance / sellers. No hovers.
 *  lsZoneDefenseMarks (ZoneDefenseMarks.cpp: #define ZD_MARKS 1 + #include this file) - the price-chart overlay: the zone band
 *    (green bids / red offers, brightness = resting / peak, ends when broken, faded below 50% or 60 min untouched), its one label
 *    "Zone lo-hi | lots (Nx) | oldest Nm | held/broken", at most ONE big-order line per zone, the gray reload DIAMOND on the touch
 *    candle, the SWEEP triangle on the first stop run (green up = swept, red down = weak; offers mirrored). Labels never overlap.
 *  Colours follow the measured signs in ZoneDefenseParams.h (0 = not proven = white). Settings (one window, both plugins):
 *  lsFlexLevels\ZoneDefense.settings-<MKT>.txt (zdset::spec, version below = the plugin version).
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#ifndef LSPERF_PLUGIN
#ifdef ZD_MARKS
#define LSPERF_PLUGIN "ZoneDefenseMarks"
#else
#define LSPERF_PLUGIN "ZoneDefense"
#endif
#endif
#include "PerfGuard.h"
#include "SettingsGuard.h"
#include "HostSlot.h"
#include "SettingsSpec.h"
#include "SettingsHost.h"
#include "ZoneDefenseLogic.h"
#include "ZoneDefenseRead.h"
#include "ZoneDefenseSettings.h"
#include "MarketKey.h"
#include "MboQuiet.h"   // (1.0.8) the MBO quiet notice
#include "NearLevels.h"   // (1.0.2 levels) the key / MenthorQ level files, read as lsDeltaProfile reads them
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <chrono>
#include <climits>

#define ZD_VERSION "1.0.9"   // = zdset::PLUGIN_VERSION = zd::VERSION = the one setVersion literal (zdh::stamp)


namespace zdh {
// the version of both DLLs: the ONE setVersion literal (the settings contract counts it; gex-build reads it)
inline void stamp(cppExtension* p) {   // (1.0.8) the literal on its own line: gex-build :ver reads the text between the first ( and )
    p->setVersion("1.0.9");
}
// the one settings window of both plugins (registered once per DLL)
inline void registerWindow(cppExtension* x, ss::Ids& ids) { ss::registerAll(x, zdset::spec(), ids); }
// file helpers shared by the two plugins
inline short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
// (1.0.4) the PC's UTC offset in minutes (local - UTC) now, from the Windows time-zone API; false = unknown (then nothing is shown).
// Off Windows (the mock-host tests) the value comes from testTz() - unknown unless a test sets it.
struct PcTz { bool known = false; int offMin = 0; };
inline PcTz& testTz() { static PcTz t; return t; }
inline bool pcOffsetMin(int& out) {
#if defined(_WIN32)
    TIME_ZONE_INFORMATION z; std::memset(&z, 0, sizeof z); const DWORD r = GetTimeZoneInformation(&z);
    if (r == TIME_ZONE_ID_INVALID) return false;
    const long b = z.Bias + (r == TIME_ZONE_ID_DAYLIGHT ? z.DaylightBias : r == TIME_ZONE_ID_STANDARD ? z.StandardBias : 0);
    if (b < -14 * 60 || b > 14 * 60) return false;
    out = -(int)b; return true;
#else
    if (!testTz().known) return false; out = testTz().offMin; return true;
#endif
}
// (1.0.5 levels fix) the level files' freshness is judged on the PC's real UTC clock (file times are UTC); the order stamps are
// CT-as-epoch inside IRT, 5-6 h behind, so every file looked "from the future" -> "levels 0 (0 levels from 0 files)". Tests can pin it.
inline long long& testWall() { static long long w = 0; return w; }
inline long long wallNow() { return testWall() > 0 ? testWall() : (long long)std::time(nullptr); }
inline bool& clockCheck() { static bool on = true; return on; }   // (L6) the Central-time check (the mock-host tests whose clock is UTC turn it off)
inline std::string flexDir() { const char* u = std::getenv("USERPROFILE"); return u && *u ? std::string(u) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string(); }
inline void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0700);
#endif
}
inline bool stat2(const std::string& p, long long& mt, long long& sz) { struct stat s; if (stat(p.c_str(), &s) != 0) return false; mt = (long long)s.st_mtime; sz = (long long)s.st_size; return true; }
inline std::string journalDir(const std::string& flex) { return flex.empty() ? std::string() : flex + "\\ZoneDefense"; }
inline std::string journalPath(const std::string& flex, const std::string& mkt, int spb, const std::string& session) {
    return flex.empty() || mkt.empty() || spb <= 0 || session.empty() ? std::string() : journalDir(flex) + "\\" + mkt + "-" + std::to_string(spb) + "-" + session + ".csv";
}
// (1.0.2, review H1) the market of a chart: MarketKey.h mkey::norm (QGC = GC, @ES / EP = ES, QCL / CLE = CL, QHG / CPE = HG, 6E = EU,
// contract roots NGX6 ...) on the root, else on the symbol; "" = not one of his 7 markets (the pane says so)
inline bool known(const std::string& m) { static const char* const K[] = { "ES", "NQ", "CL", "NG", "GC", "HG", "EU" }; for (const char* k : K) if (m == k) return true; return false; }
inline std::string upper(std::string r) { for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]); return r; }
inline std::string marketOf(const std::string& root, const std::string& symbol = std::string()) {
    std::string m = mkey::norm(upper(root)); if (known(m)) return m;
    m = mkey::norm(upper(symbol)); if (known(m)) return m;
    const std::string s = upper(symbol); if (s.size() > 3) { m = mkey::norm(s.substr(0, s.size() - 2)); if (known(m)) return m; }   // ESZ6 -> ES
    return std::string();
}
// (1.0.2, review H4) ONE WRITER per journal file: a process registry (two charts in one IRT) + a lock file held open while writing
// (two IRTs). The others only read the journal and draw it.
struct Owners { std::map<std::string, const void*> by; static Owners& get() { static Owners o; return o; } };
inline bool claim(const std::string& path, const void* who, void*& handle) {
    if (path.empty()) return false;
    std::map<std::string, const void*>::iterator it = Owners::get().by.find(path);
    if (it != Owners::get().by.end()) return it->second == who;
#if defined(_WIN32)
    HANDLE h = CreateFileA((path + ".lock").c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;                  // another program writes this journal
    handle = (void*)h;
#else
    handle = nullptr;
#endif
    Owners::get().by[path] = who; return true;
}
inline void unclaim(const std::string& path, const void* who, void*& handle) {
    std::map<std::string, const void*>::iterator it = Owners::get().by.find(path);
    if (it != Owners::get().by.end() && it->second == who) Owners::get().by.erase(it);
#if defined(_WIN32)
    if (handle) CloseHandle((HANDLE)handle);
#endif
    handle = nullptr;
}
// the session a local time belongs to: 17:00 CT starts the next trade date
inline std::string sessionOf(const struct tm& t) {
    struct tm x = t; x.tm_isdst = -1; if (x.tm_hour >= 17) x.tm_mday += 1; x.tm_hour = 12; x.tm_min = 0; x.tm_sec = 0;
    std::time_t tt = std::mktime(&x); struct tm y; std::memset(&y, 0, sizeof y);
#if defined(_WIN32)
    localtime_s(&y, &tt);
#else
    localtime_r(&tt, &y);
#endif
    char b[40]; std::snprintf(b, sizeof b, "%04d-%02d-%02d", y.tm_year + 1900, y.tm_mon + 1, y.tm_mday); return b;
}
inline std::string clock12(int h, int m) { char b[16]; std::snprintf(b, sizeof b, "%d:%02d %s", (h % 12) == 0 ? 12 : h % 12, m, h < 12 ? "AM" : "PM"); return b; }
inline zdr::Signs signsOf(const std::string& mkt) { const zdp::Mkt& M = zdp::forMarket(mkt); zdr::Signs s; s.reload = M.reloadSign; s.pull = M.pullSign; s.sweep = M.sweepSign; return s; }
static const COLOR C_GREEN = 0x0022C55E, C_RED = 0x00EF4444, C_WHITE = 0x00E5E7EB, C_GRAY = 0x009CA3AF, C_DIM = 0x006B7280;
inline COLOR colOf(int c) { return c > 0 ? C_GREEN : c < 0 ? C_RED : C_WHITE; }
inline COLOR colOfSign(int c, int sign) { return sign == 0 ? C_WHITE : colOf(c); }
// (1.0.1) the detail tint of each line colour (the same hue, calmer) and the dimmed pane bars (never fight the text)
static const COLOR C_GREEN_T = 0x008FD6AB, C_RED_T = 0x00F2A0A0, C_WHITE_T = 0x00A9B2BD, C_BAR_RED = 0x006E2A2A, C_DOT_GREEN = 0x003F8F5F;
static const COLOR C_BOX_BG = 0x00111827;   // (1.0.6) the read box: opaque near-black behind the text (the bars stay full height)
inline COLOR tintOf(COLOR c) { return c == C_GREEN ? C_GREEN_T : c == C_RED ? C_RED_T : C_WHITE_T; }
inline COLOR scaleCol(COLOR c, double f) { if (f < 0) f = 0; if (f > 1) f = 1; const int r = (int)(((c >> 16) & 255) * f), g = (int)(((c >> 8) & 255) * f), b = (int)((c & 255) * f); return (COLOR)((r << 16) | (g << 8) | b); }
// the candle holding feed time t: IRT stamps bar ENDS inside the plugin (OrderFlowFeed eventKey, BUILD-LESSONS #154: the in-plugin
// barDateTime is the END; the ChartView / IRT bars CSV export writes the START), so the FIRST bar whose end is AFTER t (strict >); a
// time exactly on a bar end belongs to the NEXT bar (review M6)
template <class A> int barOfEnd(A& times, int count, long long tMs) {
    const long long t = tMs / 1000; int lo = 0, hi = count - 1;
    if (count <= 0) return -1;
    if ((long long)times[hi] <= t) return hi;
    while (lo < hi) { const int m = (lo + hi) / 2; if ((long long)times[m] > t) hi = m; else lo = m + 1; }
    return lo;
}
// a journal file read incrementally (append-only); the model only grows
struct JFile {
    std::string path; long long off = 0, mt = -1, sz = -1; zdr::Journal J; std::string partial;
    bool poll() {
        long long m = 0, s = 0; if (path.empty() || !stat2(path, m, s)) return false;
        if (s < off) { off = 0; J = zdr::Journal(); partial.clear(); }                 // a new / rewritten file
        if (s == sz && m == mt) return false;
        std::ifstream in(path.c_str(), std::ios::binary); if (!in.is_open()) return false;
        in.seekg(off); std::string chunk((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        off += (long long)chunk.size(); mt = m; sz = s;
        std::string all = partial + chunk; size_t a = 0, nl;
        while ((nl = all.find('\n', a)) != std::string::npos) { J.add(all.substr(a, nl - a)); a = nl + 1; }
        partial = all.substr(a);                                                            // a half-written line is not a record
        if (partial.size() > 4096) partial.clear();                                         // never grows on garbage
        return true;
    }
};
} // namespace zdh

#ifndef ZD_MARKS
// =====================================================================================================================
// lsZoneDefense - the pane
// =====================================================================================================================
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#define OFF_SDK 1
#include "OrderFlowFeed.h"
namespace zd {
struct PaneLogic {   // the MboPane contract (the base's own bars / records are not drawn by this pane)
    static const char* name() { return "ZoneDefense"; }
    static const char* title() { return "Zone Defense"; }
    static const char* version() { return ZD_VERSION; }
    static int metrics() { return 3; }
    static int paceMetric() { return off::PM_ADD; }
    static int optGate() { return ofo::G_OSTACK; }
    static std::string optLine(const ofo::Snap& s, int gate, const std::string&) { return ofo::stackLine(s, gate); }
    static bool resets() { return false; }
    static std::string hoverQuestion() { return "Is the zone defended?"; }
    static const char* const* hoverVerbs() { static const char* const v[2] = { "Bids defending", "Offers defending" }; return v; }
    static off::Info info(const off::PaneBar*, const off::Params&) { return off::Info(); }
    static long long value(const off::Rec& r, int metric, int s) { return metric == 0 ? r.addC[s] : metric == 1 ? r.pullC[s] : r.refN[s]; }
    static void decide(off::PaneBar& b, const off::Params&, bool) { for (int s = 0; s < 2; ++s) { b.spike[s] = false; b.dash[s] = -1; b.let[s].clear(); } b.msg = decltype(b.msg)(); }
};
} // namespace zd
static ss::Ids& zdIds() { static ss::Ids I; return I; }

class ZoneDefense : public off::MboPane<zd::PaneLogic> {
public:
    SS_SAFE_DIALOG(ZoneDefense, "ZoneDefense")
    ZoneDefense() : off::MboPane<zd::PaneLogic>() {}
    virtual bool barMarks() const { return false; }
    virtual bool textOff() const { return true; }
    virtual int parmsTitle(char* pStr, int size) { return off::copyTitle(pStr, size, zd::PaneLogic::title(), ZD_VERSION) ? RTX_OK : RTX_FAIL; }
    struct Ext : off::PaneExt {
        std::unique_ptr<zd::Engine> eng; std::string mkt, session, path; int spb = 0; double tick = 0;
        size_t written = 0; long long writeFails = 0, events = 0, skipped = 0, sessions = 0;
        long long stamp = -1, stampMs = 0, lastT = 0; int ctOff = 0; long long ctKey = -1;
        zdh::JFile jf; ss::Store store; ss::Values set = ss::defaults(zdset::spec(), -1); long long setMs = -100000;
        std::vector<zdr::RLine> lastLines; long long drawn = 0;
        std::vector<nl::Level> vol; std::string volDate, volSym; long long volSig = -2, volChecked = -1;   // (1.0.2) LRA-VolLevels-<MKT>.csv
        std::string clockWarn;                                          // (L6) "" = the PC is on Central time
        nl::Cache lvc; long long lvlSig = -1; size_t nLevels = 0; std::string lvlNote;   // (1.0.2 levels)
        bool writer = false; void* lock = nullptr; long long reconnects = -1; std::string symbol, note; long long tornFixed = 0, orphansClosed = 0;
        long long lastEvWall = 0, firstWall = 0; std::string quiet;                        // (1.0.8) the real clock of the last order event / of the first draw
    };
    Ext* ext(S_& S) { if (!S.ext) S.ext.reset(new (std::nothrow) Ext()); return static_cast<Ext*>(S.ext.get()); }
    // ---- the feed: every order event, once, in order ------------------------------------------------------------------
    std::string mktOf(S_& S) { const std::string m = zdh::marketOf(S.market); return m; }
    // (1.0.2 levels) his key / MenthorQ levels from lsFlexLevels (fresh <= 18 h, the chart's contract) -> the engine (a change only)
    void refreshLevels(S_& S, Ext& e) {
        const std::string flex = zdh::flexDir(); if (flex.empty() || !(e.tick > 0)) return;
        std::vector<nl::Level> v = nl::levels(e.lvc, flex + "\\", e.mkt, S.symbol, off::steadyMs(), zdh::wallNow());   // (1.0.5) the real UTC clock (was e.stamp = CT-as-epoch -> 0 levels)
        // (1.0.2) the volume levels (DPoc / VAH / VAL / WPoc) the nightly writes for the NEXT session: LRA-VolLevels-<MKT>.csv
        // "SYMBOL|GCZ6", "DATE|2026-11-06", "LEVEL|<code>|<what>|<price>"; used only on that session and that contract
        const long long nowMs = off::steadyMs();
        if (e.volChecked < 0 || nowMs - e.volChecked >= 5000 || nowMs < e.volChecked) {
            e.volChecked = nowMs; const std::string path = flex + "\\LRA-VolLevels-" + e.mkt + ".csv"; const long long sig = nl::fileSig(path);
            if (sig != e.volSig) {
                e.volSig = sig; e.vol.clear(); e.volDate.clear(); e.volSym.clear(); std::vector<std::string> L;
                if (sig >= 0 && nl::readBoundedLines(path, L, 64 * 1024, 256, 512))
                    for (size_t i = 0; i < L.size(); ++i) { const std::vector<std::string> c = nl::fields(L[i]); double px;
                        if (c.size() >= 2 && c[0] == "SYMBOL") e.volSym = c[1];
                        else if (c.size() >= 2 && c[0] == "DATE") e.volDate = c[1];
                        else if (c.size() >= 4 && c[0] == "LEVEL" && !c[1].empty() && nl::parseReal(c[3], px) && px > 0) e.vol.push_back(nl::Level{ c[1].substr(0, 40), px }); }
            }
        }
        const bool volOk = !e.vol.empty() && e.volDate == e.session && nl::sameContract(S.symbol, e.volSym);
        if (volOk) v.insert(v.end(), e.vol.begin(), e.vol.end());
        std::vector<zd::Lvl> out; long long sig = 1469598103LL;
        for (size_t i = 0; i < v.size(); ++i) {
            if (!(v[i].px > 0)) continue;
            const long long px = (long long)std::llround(v[i].px / e.tick);
            const std::vector<zd::Lvl> L = zd::levelsOf(v[i].name, px);
            for (size_t j = 0; j < L.size(); ++j) { out.push_back(L[j]); for (size_t c = 0; c < L[j].name.size(); ++c) sig = sig * 131 + (unsigned char)L[j].name[c]; sig = sig * 1000003 + L[j].px; }
        }
        e.lvlNote = e.lvc.note + (volOk ? ", volume levels" : e.vol.empty() ? ", no volume file" : ", volume file not for this session / contract");
        if (sig == e.lvlSig && out.size() == e.nLevels) return;
        e.lvlSig = sig; e.nLevels = out.size(); e.eng->setLevels(out);
    }
    virtual void onRaw(S_& S, const DEPTH_ORDER& o) {
        Ext* e = ext(S); if (!e) return;
        const std::string mkt = mktOf(S);
        if (mkt.empty()) { e->note = "market not supported (" + S.market + ")"; ++e->skipped; return; }
        if (S.spb <= 0) { e->note = "needs a time-based chart (seconds per bar unknown)"; ++e->skipped; return; }
        if (!zd::Engine::barSizeOk(S.spb)) { e->note = "unsupported bar size (" + std::to_string((int)S.spb) + " s): use 1-60 min bars that divide the hour"; ++e->skipped; return; }   // (L4)
        if (!(S.tick > 0) || !o.timestamp) { ++e->skipped; return; }
        e->note.clear();
        e->lastEvWall = zdh::wallNow();                                  // (1.0.8) the feed is alive
        if ((long long)o.timestamp != e->stamp) {                         // one clock conversion per second
            struct tm t; std::memset(&t, 0, sizeof t);
            if (!getLocaltime(o.timestamp, &t)) { ++e->skipped; return; }
            e->stamp = (long long)o.timestamp;
            const std::string sess = zdh::sessionOf(t);
            if (!e->eng || sess != e->session || mkt != e->mkt || (int)S.spb != e->spb) openSession(S, *e, sess, mkt);
            const long long utcMin = e->stamp / 60; e->ctOff = (int)(((t.tm_hour * 60 + t.tm_min) - (utcMin % 1440) + 2880) % 1440);
            if (e->eng) e->eng->ctOffsetMin = e->ctOff;
            // (L6, 1.0.4 fix) the PC's zone from the Windows API (the order stamps are already local inside IRT, so local - stamp = 0 was a
            // FALSE "UTC+0"); compared with America/Chicago at the real UTC now; an unknown zone = no warning (never a false one)
            if (zdh::clockCheck()) { int have = 0; const long long nowUtc = (long long)std::time(nullptr);
              if (!zdh::pcOffsetMin(have)) e->clockWarn.clear();
              else { const int want = zd::Engine::chicagoOffsetMin(nowUtc);
                e->clockWarn = have == want ? std::string() : "PC clock is UTC" + std::string(have >= 0 ? "+" : "-") + std::to_string(std::abs(have) / 60) + (have % 60 ? ":" + std::to_string(std::abs(have % 60)) : std::string()) + ", not Central - sessions and times are off"; } }
            if (e->eng) refreshLevels(S, *e);                              // (1.0.2 levels) the files are re-read at most every 5 s (nl::Cache)
        }
        if (!e->eng || !e->writer) return;                                 // a read-only copy (another chart writes this journal)
        // (M1) a reconnect or a new contract on the chart: the book is re-sent - close the open zones (5) and start the book again
        if (e->reconnects < 0) { e->reconnects = S.feed.reconnects; e->symbol = S.symbol; }
        if (S.feed.reconnects != e->reconnects || S.symbol != e->symbol) { e->eng->reset(e->stamp * 1000); e->reconnects = S.feed.reconnects; e->symbol = S.symbol; }
        off::Ev x = off::evOf(o, S.tick, 0);
        if (x.action > 3 || x.px <= 0) { ++e->skipped; return; }
        if (x.implied) x.side = -1;                                        // (M3) implied fills are not counted (as the research)
        zd::In in; in.t = e->stamp * 1000; in.act = x.action; in.side = x.side; in.px = x.px; in.sz = x.size; in.oid = x.id;   // (H2) whole seconds
        e->eng->feed(in); ++e->events;
    }
    void openSession(S_& S, Ext& e, const std::string& sess, const std::string& mkt) {
        std::unique_ptr<zd::Engine> old;
        if (e.eng) { e.eng->finish(e.eng->lastT + 1, 4); flush(e); old.swap(e.eng); }
        if (e.writer) zdh::unclaim(e.path, this, e.lock);
        const bool sameMarket = old && e.mkt == mkt && e.spb == (int)S.spb;
        e.mkt = mkt; e.spb = (int)S.spb; e.tick = S.tick; e.session = sess; ++e.sessions;
        e.eng.reset(new (std::nothrow) zd::Engine(zdp::forMarket(e.mkt))); if (!e.eng) return;
        e.eng->barSec = e.spb; e.written = 0; e.lvlSig = -1; e.nLevels = 0;   // (1.0.2 levels) a new engine gets the levels again
        if (sameMarket) e.eng->carryBook(*old);                            // (M2) the 17:00 roll keeps the resting book (no warm-up)
        const std::string flex = zdh::flexDir(); zdh::makeDir(flex); zdh::makeDir(zdh::journalDir(flex));
        e.path = zdh::journalPath(flex, e.mkt, e.spb, sess);
        e.writer = zdh::claim(e.path, this, e.lock);
        e.jf = zdh::JFile(); e.jf.path = e.path; e.jf.poll();             // a restart: what was decided before stays (no repaint)
        int maxId = 0; for (std::map<int, zdr::ZoneView>::const_iterator it = e.jf.J.z.begin(); it != e.jf.J.z.end(); ++it) maxId = std::max(maxId, it->first);
        e.eng->nextId = maxId + 1;                                         // new zones never reuse a journal id
        if (!e.writer || e.path.empty()) return;
        long long mt = 0, sz = 0;
        if (!zdh::stat2(e.path, mt, sz)) { std::ofstream f(e.path.c_str(), std::ios::binary | std::ios::app); char b[64]; std::snprintf(b, sizeof b, "H,%.10g,%s\n", e.tick, ZD_VERSION); f << b; return; }
        // (M7) a torn last line (IRT died mid-write): end it so the next record starts on its own line (the fragment never parses)
        { std::ifstream in(e.path.c_str(), std::ios::binary); if (sz > 0) { in.seekg(sz - 1); char c = 0; in.get(c); if (c != '\n') { std::ofstream f(e.path.c_str(), std::ios::binary | std::ios::app); f << "\n"; ++e.tornFixed; } } }
        // (H3) zones the journal left open (IRT closed / the pane removed before they finished) are closed now: ZE code 5, never resumed
        std::ofstream f(e.path.c_str(), std::ios::binary | std::ios::app);
        for (std::map<int, zdr::ZoneView>::const_iterator it = e.jf.J.z.begin(); it != e.jf.J.z.end(); ++it)
            if (!it->second.ended) { f << "ZE," << it->first << "," << (e.stamp > 0 ? e.stamp * 1000 : 0) << ",5,0\n"; ++e.orphansClosed; }
    }
    // (H3) the pane is removed / IRT closes / the chart is reloaded: close what is open (ZE 5), write it, give up the journal
    void stopAll() {
        S_* S = testState(); if (!S) return; Ext* e = ext(*S); if (!e) return;
        if (e->eng && e->writer) { e->eng->finish(e->eng->lastT + 1, 5); flush(*e); }
        if (e->writer) zdh::unclaim(e->path, this, e->lock);
        e->writer = false;
    }
    void flush(Ext& e) {
        if (!e.eng || !e.writer || e.path.empty() || e.written >= e.eng->out.size()) return;
        std::ofstream f(e.path.c_str(), std::ios::binary | std::ios::app);
        if (!f.is_open()) { ++e.writeFails; return; }
        for (; e.written < e.eng->out.size(); ++e.written) f << e.eng->out[e.written].text << "\n";
        f.flush(); if (!f.good()) ++e.writeFails;
        if (e.eng->out.size() > 20000) { e.eng->out.clear(); e.written = 0; }
    }
    virtual void afterPump(S_& S, long long, bool) { Ext* e = ext(S); if (e) { flush(*e); e->jf.poll(); } }
    virtual void afterTimer(S_& S, long long) { Ext* e = ext(S); if (e) flush(*e); }
    virtual void statusExtra(S_& S, std::ostream& o) {
        Ext* e = ext(S); if (!e) { o << "\nZONE_DEFENSE,unavailable"; return; }
        const zdp::Mkt& M = zdp::forMarket(e->mkt.empty() ? S.market : e->mkt);
        o << "\nZONE_DEFENSE,version " << ZD_VERSION << ",market " << e->mkt << (e->writer ? ",writer" : ",read-only (another chart writes this journal)") << ",session " << e->session << ",events " << e->events << ",skipped " << e->skipped
          << (e->clockWarn.empty() ? std::string() : ",WARNING " + e->clockWarn) 
          << ",levels " << e->nLevels << " (" << e->lvlNote << ")"
          << ",resets " << (e->eng ? e->eng->resets : 0) << ",ghosts dropped " << (e->eng ? e->eng->ghosts : 0) << ",implied skipped " << (e->eng ? e->eng->implied : 0) << ",moves " << (e->eng ? e->eng->modMoves : 0)
          << ",orphans closed " << e->orphansClosed << ",torn lines fixed " << e->tornFixed << (e->note.empty() ? std::string() : ",note " + e->note)
          << ",zones " << (e->eng ? e->eng->zones.size() : 0) << ",journal " << e->path << ",lines " << e->jf.J.lines << ",bad " << e->jf.J.bad << ",write fails " << e->writeFails
          << "\nZONE_DEFENSE_PARAMS,pressureX " << M.per[0].reloadX << ",signs pressure " << M.reloadSign << " pull " << M.pullSign << " sweep " << M.sweepSign << ",liftX " << zd::LIFT_X
          << "\nZONE_DEFENSE_MEMORY,book " << (e->eng ? e->eng->bookSize() : 0) << ",max book " << (e->eng ? e->eng->maxBook : 0) << ",max ended " << (e->eng ? e->eng->maxEnded : 0) << ",max live zones " << (e->eng ? e->eng->maxLive : 0)
          << "\nZONE_DEFENSE_DRAW,lines drawn " << e->drawn;
        for (size_t i = 0; i < e->lastLines.size(); ++i) o << " | " << e->lastLines[i].text;
        {   const long long now = zdh::wallNow();                          // (1.0.8) the MBO quiet notice
            o << "\nMBO_QUIET," << mbq::VERSION << "," << (e->quiet.empty() ? std::string("feed alive") : e->quiet) << ",last order event "
              << (e->lastEvWall > 0 ? std::to_string(now - e->lastEvWall) + " s ago" : std::string("none yet")) << ",threshold 2 min 07:00-15:00 CT / 5 min overnight"; }
    }
    // ---- settings (one window with lsZoneDefenseMarks) --------------------------------------------------------------
    virtual int parmsLoadBody(void) { if (!ss::inDialog(this)) return RTX_OK; ss::Store* st = dialogStore(); if (st) ss::fillDialog(this, zdset::spec(), zdIds(), st->load(ss::marketIndex(dlgMkt_), off::steadyMs(), true), snap_); return RTX_OK; }
    virtual int parmsApplyBody(void) { save(); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::onUpdate(this, zdset::spec(), zdIds(), (int)i, snap_); save(); return RTX_OK; }
    void save() { if (!ss::inDialog(this)) return; ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, zdset::spec(), zdIds(), st->load(ss::marketIndex(dlgMkt_), off::steadyMs(), true), snap_);
        zdh::makeDir(zdh::flexDir()); if (st->save(V)) snap_.take(ss::dialogNow(this, zdset::spec(), zdIds(), V)); }
    ss::Store* dialogStore() { const std::string mkt = ss::safeMarket(this); const std::string p = zdset::settingsPath(zdh::flexDir(), mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &zdset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; } return &dlgStore_; }
    const ss::Values& settings(Ext& e, const std::string& mkt) {
        const long long now = off::steadyMs(); if (now - e.setMs < 2000 && now >= e.setMs) return e.set; e.setMs = now;
        const std::string p = zdset::settingsPath(zdh::flexDir(), mkt);
        if (e.store.path != p) { e.store = ss::Store(); e.store.S = &zdset::spec(); e.store.path = p; }
        e.set = p.empty() ? ss::defaults(zdset::spec(), -1) : e.store.load(ss::marketIndex(mkt), now); return e.set;
    }
    // ---- calc / draw ---------------------------------------------------------------------------------------------------
    void zdCalc() { pump(); S_* S = testState(); if (!S) return; Ext* e = ext(*S); if (e && e->jf.poll()) invalidateChart(false); }
    virtual int scale(int, int, double* dMin, double* dMax) { if (dMin) *dMin = 0; if (dMax) *dMax = 1; return RTX_OK; }
    std::string clockOf(long long tMs) { if (tMs <= 0) return std::string(); struct tm t; std::memset(&t, 0, sizeof t); if (!getLocaltime((RTDATE)(tMs / 1000), &t)) return std::string(); return zdh::clock12(t.tm_hour, t.tm_min); }
    virtual int draw(void) {
        LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
        pump(true);
        S_* S = testState(); if (!S) return RTX_OK;
        Ext* e = ext(*S); if (!e) return RTX_OK;
        try { render(*S, *e); } catch (...) { S->state = "draw error"; }
        return RTX_OK;
    }
    static const int LINE_H = 14;
    void render(S_& S, Ext& e) {
        const std::string mkt = mktOf(S);
        if (e.jf.path.empty() && !mkt.empty() && S.spb > 0) {           // no order event yet (a restart): the session of the last candle
            const long count = getBarCount(); RTARRAYI times(barDateTime);
            if (count > 0 && times.count >= count) { struct tm t; std::memset(&t, 0, sizeof t);
                if (getLocaltime((RTDATE)times[(int)count - 1] - 1, &t)) e.jf.path = zdh::journalPath(zdh::flexDir(), mkt, (int)S.spb, zdh::sessionOf(t)); }
        }
        e.jf.poll();
        RCT pane; pane.getPaneRect(false);
        const ss::Values& V = settings(e, mkt);
        const int left = pane.left + 6, right = pane.right - 6;
        // (1.0.6, Rassul 2026-11-09 "make the bars regular size and just block out the read area") the bars use the FULL pane height
        // (drawn first); the read lines go on top in an opaque dark box sized to their text, hiding the bars only where the text is.
        if (V.b(zdset::K_SHOW_BARS)) drawBars(S, e, pane.top + 2, pane);
        std::vector<zdr::RLine> L;
        const zdr::ZoneView* z = e.jf.J.latestTouched();
        if (!e.clockWarn.empty()) { zdr::RLine w; zdr::setLine(w, "Clock", "warning", e.clockWarn); w.col = 0; L.push_back(w); }   // (L6) only when wrong
        if (V.b(zdset::K_SHOW_LINES)) {
            if (!z) {                                                          // (L3) say why nothing is shown
                std::string why = "for a zone touch";
                if (mkt.empty()) why = "market not supported (" + S.market + ")";
                else if (S.spb <= 0) why = "needs a time-based chart";
                else if (!zd::Engine::barSizeOk(S.spb)) why = "unsupported bar size: use 1-60 min bars that divide the hour";   // (L4)
                else { const std::string fs = S.feed.stateText(off::steadyMs()); if (fs != "ready" && !fs.empty()) why = "no order feed - " + fs; }
                zdr::RLine w; zdr::setLine(w, "Zone", "waiting", why); L.push_back(w); e.lastLines.clear(); }
            else {
                e.lastLines = zdr::readLines(*z, [this](long long t) { return clockOf(t); }, zdh::signsOf(mkt), S.tick);
                L.insert(L.end(), e.lastLines.begin(), e.lastLines.end());
                ++e.drawn;
            }
        }
        if (!L.empty()) drawBlock(L, left, right, pane.top + 3);
        drawQuiet(e, right, pane.top + 3);                                  // (1.0.8) top-right, never renumbers the lines
    }
    // (1.0.8) "MBO QUIET 12m": white bold on the pane's dark box at the top-right; the PC clock is Central (else the Clock line warns)
    std::string quietText(Ext& e) {
        const long long now = zdh::wallNow(); if (now <= 0) return std::string();
        if (e.firstWall <= 0) e.firstWall = now;
        const std::time_t tt = (std::time_t)now; struct tm t; std::memset(&t, 0, sizeof t);
#ifdef _WIN32
        if (localtime_s(&t, &tt) != 0) return std::string();
#else
        if (!localtime_r(&tt, &t)) return std::string();
#endif
        return mbq::text(e.lastEvWall > 0 ? e.lastEvWall : e.firstWall, now, t.tm_hour * 60 + t.tm_min, t.tm_wday);
    }
    void drawQuiet(Ext& e, int right, int y) {
        const std::string q = quietText(e); e.quiet = q; if (q.empty()) return;
        FONT fb; fb.id = HELVETICA; fb.size = 9; fb.style = BOLD; setFont(fb);
        const int w = getTextWidth(q.c_str(), -1), x = right - w;
        off::fillFlat(this, x - 6, y - 2, right + 4, y + LINE_H + 2, zdh::C_BOX_BG);
        put(q, x, y, zdh::C_WHITE, right + 1);
    }
    // (1.0.1, Rassul 2026-11-08 "look nice") one aligned block: "1. Zone" labels in a fixed left column (grey), the verdict word at ONE x
    // (its colour, bold), the detail on the same line in a calmer tint of that colour, the times in one right-aligned column (grey).
    // Every text is drawn left-aligned in a rectangle exactly its own width (no overlaps; the tests read the rectangles).
    void drawBlock(const std::vector<zdr::RLine>& L, int left, int right, int y0) {
        FONT fp; fp.id = HELVETICA; fp.size = 9; fp.style = PLAIN; FONT fb = fp; fb.style = BOLD;
        setFont(fp);
        int labW = 0, timeW = 0;
        for (size_t i = 0; i < L.size(); ++i) {
            const std::string lab = std::to_string(i + 1) + ". " + L[i].label; labW = std::max(labW, getTextWidth(lab.c_str(), -1));
            if (!L[i].time.empty()) timeW = std::max(timeW, getTextWidth(L[i].time.c_str(), -1));
        }
        const int xV = left + labW + 12, limAll = right - (timeW > 0 ? timeW + 10 : 0);
        // (1.0.6) measure first: the widest verdict + detail decides where the time column sits (fit width, like ReadBox 2.22.1)
        int textEnd = xV;
        for (size_t i = 0; i < L.size(); ++i) {
            setFont(fb); const int vw = getTextWidth(L[i].verdict.c_str(), -1); setFont(fp);
            const int dw = L[i].detail.empty() ? 0 : getTextWidth(L[i].detail.c_str(), -1) + 8;
            textEnd = std::max(textEnd, std::min(limAll, xV + vw + dw));
        }
        const int xT = timeW > 0 ? std::min(right - timeW, textEnd + 12) : textEnd, lim = timeW > 0 ? xT - 10 : right;
        const int boxR = std::min(right + 4, (timeW > 0 ? xT + timeW : textEnd) + 6);
        off::fillFlat(this, left - 6, y0 - 2, boxR, y0 + (int)L.size() * LINE_H + 2, zdh::C_BOX_BG);   // opaque: the bars behind the text are hidden
        for (size_t i = 0; i < L.size(); ++i) {
            const int y = y0 + (int)i * LINE_H; const COLOR c = zdh::colOf(L[i].col);
            setFont(fp); put(std::to_string(i + 1) + ". " + L[i].label, left, y, zdh::C_GRAY, lim);
            setFont(fb); const int vw = put(L[i].verdict, xV, y, c, lim);
            setFont(fp); if (!L[i].detail.empty() && xV + vw + 8 < lim) put(L[i].detail, xV + vw + 8, y, zdh::tintOf(c), lim);
            if (!L[i].time.empty()) put(L[i].time, xT + timeW - getTextWidth(L[i].time.c_str(), -1), y, zdh::C_GRAY, xT + timeW + 1);
        }
    }
    // one text at (x, y), cut with "..." to end before lim; returns its drawn width
    int put(std::string t, int x, int y, COLOR c, int lim) {
        int w = getTextWidth(t.c_str(), -1);
        while (t.size() > 3 && x + w > lim) { t = t.substr(0, t.size() - 4) + "..."; w = getTextWidth(t.c_str(), -1); }
        if (x + w > lim) return 0;
        setTextColor(c); RCT r; r.set(off::coord(x), off::coord(y), off::coord(x + w), off::coord(y + LINE_H - 1)); r.drawText(t.c_str(), false, false);
        return w;
    }
    // per candle: lots sold into the zone (red bar up from the bottom), the bid / offer left in it (green dot)
    void drawBars(S_&, Ext& e, int top, const RCT& pane) {
        const long count = getBarCount(); if (count < 2) return;
        RTARRAYI times(barDateTime); if (times.count < count) return;
        int v0 = 0, v1 = 0; if (getVisibleBars(&v0, &v1) != RTX_OK) return; v0 = std::max(0, v0); v1 = std::min((int)count - 1, v1);
        const int bottom = pane.bottom - 2; if (bottom - top < 12) return;
        // (1.0.2, L1) the bars of the zone the lines read (the latest touched one): its side, its lots
        const zdr::ZoneView* z = e.jf.J.latestTouched(); if (!z) return;
        const std::map<long long, long long>& sold = z->barSold; const std::map<long long, long long>& left = z->barLeft;
        long long mx = 1;
        for (int i = v0; i <= v1; ++i) { const long long k = (long long)times[i]; std::map<long long, long long>::const_iterator a = sold.find(k), b = left.find(k); if (a != sold.end()) mx = std::max(mx, a->second); if (b != left.end()) mx = std::max(mx, b->second); }
        const int ppb = std::max(1, (int)getPixelsPerBar()), w = std::max(1, ppb * 6 / 10);
        for (int i = v0; i <= v1; ++i) {
            const long long k = (long long)times[i]; PNT p; p.set(i, 0.0f, kBarCenter); const int cx = p.h;
            std::map<long long, long long>::const_iterator s = sold.find(k), l = left.find(k);
            if (s != sold.end() && s->second > 0) { const int h = (int)((bottom - top) * (double)s->second / mx); off::fillFlat(this, cx - w / 2, bottom - h, cx - w / 2 + w, bottom, zdh::C_BAR_RED); }
            if (l != left.end()) { const int yy = bottom - (int)((bottom - top) * (double)l->second / mx); off::fillFlat(this, cx - 1, yy - 1, cx + 2, yy + 2, zdh::C_DOT_GREEN); }
        }
    }
private:
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<ZoneDefense*>(this)->stopAll(); static_cast<ZoneDefense*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<ZoneDefense*>(this)->stopAll(); static_cast<ZoneDefense*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void)   { zdh::registerWindow(this, zdIds()); return RTX_OK; }
int cppExtension::calc(int)     { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<ZoneDefense*>(this)->zdCalc(); return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    ZoneDefense* p = new ZoneDefense();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI);                                    // its own pane, draws itself; no hovers
    p->setExtendedFlags(CALL_CONTINUOUSLY);                              // the order feed keeps coming while no trade prints
    p->setDescription("LRA Zone Defense - is the densest band of resting bids (support) or offers (resistance) near price being DEFENDED? (market-by-order feed). Five numbered lines for the latest touched zone (1. Zone, 2. Queue, 3. Pressure, 4. Refill, 5. Sweep), verdict first, green bullish / red bearish / white not proven: ZONE (the band, its lots and oldest order: held / broken), QUEUE (orders that had rested 30 s+ at the zone cancelled in the 60 s before the touch vs normal), PRESSURE (touch + 60 s: broke through or sold heavily into = exhausted; light selling and still there = holding), REFILL (re-adds at the hit prices, lots absorbed, what is left), SWEEP (the first stop run through the zone: lift = opposite lots in 5 s / run lots; swept = lift 1.0x+ and back in the zone). Bars: lots sold into the zone per candle (red) and what is left (green dot). Thresholds per market and period from 71 days of raw MBO (ZoneDefenseParams.h, refined nightly). The zones, the refill diamond and the sweep triangle on the price chart: add lsZoneDefenseMarks.");
    zdh::stamp(p);   // ZD_VERSION
    return p;
}

#else
// =====================================================================================================================
// lsZoneDefenseMarks - the price-chart overlay (reads the pane's journal; draws only decided lines)
// =====================================================================================================================
static ss::Ids& zdmIds() { static ss::Ids I; return I; }
struct ZmState { std::string mkt; int spb = 0; std::vector<zdh::JFile> files; std::vector<std::string> sessions; long long checkMs = -100000;
                 ss::Store store; ss::Values set = ss::defaults(zdset::spec(), -1); long long zonesDrawn = 0, labelsSkipped = 0;
                 struct Box { int l, t, r, b; }; std::vector<Box> labels; };
inline long long zmSteadyMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

class ZoneDefenseMarks : public cppExtension {
public:
    SS_SAFE_DIALOG(ZoneDefenseMarks, "ZoneDefenseMarks")
    HostSlot<ZmState> slot_;
    ZmState* stateFor() { return slot_.get(this, false); }
    virtual int parmsLoadBody(void) { if (!ss::inDialog(this)) return RTX_OK; ss::Store* st = dialogStore(); if (st) ss::fillDialog(this, zdset::spec(), zdmIds(), st->load(ss::marketIndex(dlgMkt_), zmSteadyMs(), true), snap_); return RTX_OK; }
    virtual int parmsApplyBody(void) { save(); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::onUpdate(this, zdset::spec(), zdmIds(), (int)i, snap_); save(); return RTX_OK; }
    virtual int draw(void);
    void refresh(ZmState& s);
    void zoneDraw(ZmState& s, const zdr::ZoneView& v, double tick, const ss::Values& V, int count, RTARRAYI& times, int v0, int v1);
    int barOf(RTARRAYI& times, int count, long long tMs) { return zdh::barOfEnd(times, count, tMs); }
private:
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    void save() { if (!ss::inDialog(this)) return; ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, zdset::spec(), zdmIds(), st->load(ss::marketIndex(dlgMkt_), zmSteadyMs(), true), snap_);
        zdh::makeDir(zdh::flexDir()); if (st->save(V)) snap_.take(ss::dialogNow(this, zdset::spec(), zdmIds(), V)); }
    ss::Store* dialogStore() { const std::string mkt = ss::safeMarket(this); const std::string p = zdset::settingsPath(zdh::flexDir(), mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &zdset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; } return &dlgStore_; }
};

// the journals of the last 3 sessions of this market / bar size (a stat every 2 s; append-only reads)
void ZoneDefenseMarks::refresh(ZmState& s) {
    const long long now = zmSteadyMs(); if (now - s.checkMs < 2000 && now >= s.checkMs) return; s.checkMs = now;
    const std::string flex = zdh::flexDir();
    const std::string sp = zdset::settingsPath(flex, s.mkt);
    if (s.store.path != sp) { s.store = ss::Store(); s.store.S = &zdset::spec(); s.store.path = sp; }
    s.set = sp.empty() ? ss::defaults(zdset::spec(), -1) : s.store.load(ss::marketIndex(s.mkt), now);
    std::vector<std::string> want; std::time_t t = std::time(NULL);
    for (int d = 0; d < 6 && want.size() < 3; ++d) {                       // today's session and the 2 before (weekends skipped by absence)
        std::time_t x = t - (std::time_t)d * 86400; struct tm lt; std::memset(&lt, 0, sizeof lt);
#if defined(_WIN32)
        localtime_s(&lt, &x);
#else
        localtime_r(&x, &lt);
#endif
        const std::string ses = zdh::sessionOf(lt); const std::string p = zdh::journalPath(flex, s.mkt, s.spb, ses);
        long long mt = 0, sz = 0; if (std::find(want.begin(), want.end(), p) == want.end() && zdh::stat2(p, mt, sz)) want.push_back(p);
    }
    // (L2) the journals are kept in place (no copy every 2 s): the ones no longer wanted are dropped, the others polled
    for (size_t k = s.files.size(); k-- > 0;) if (std::find(want.begin(), want.end(), s.files[k].path) == want.end()) s.files.erase(s.files.begin() + (long)k);
    for (size_t i = 0; i < want.size(); ++i) { bool have = false; for (size_t k = 0; k < s.files.size(); ++k) if (s.files[k].path == want[i]) { have = true; s.files[k].poll(); }
        if (!have) { s.files.push_back(zdh::JFile()); s.files.back().path = want[i]; s.files.back().poll(); } }
}

int ZoneDefenseMarks::draw(void) {
    LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    ZmState* sp = slot_.get(this, true); if (!sp) return RTX_OK; ZmState& s = *sp;
    char rb[64]; std::memset(rb, 0, sizeof rb); const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    s.mkt = zdh::marketOf(rs ? rs : "", sy ? sy : ""); s.spb = getSecondsPerBar(); if (s.mkt.empty() || s.spb <= 0 || !zd::Engine::barSizeOk(s.spb)) return RTX_OK;   // (L4)
    refresh(s);
    const long count = getBarCount(); if (count < 2) return RTX_OK;
    RTARRAYI times(barDateTime); if (times.count < count) return RTX_OK;
    int v0 = 0, v1 = 0; if (getVisibleBars(&v0, &v1) != RTX_OK) return RTX_OK; v0 = std::max(0, v0); v1 = std::min((int)count - 1, v1);
    const ss::Values& V = s.set; s.labels.clear(); s.zonesDrawn = 0;
    const int maxZ = std::max(1, V.i(zdset::K_MAX_ZONES));
    for (int side = 0; side < 2; ++side) {
        std::vector<const zdr::ZoneView*> pick; double tick = 0;
        for (size_t fi = s.files.size(); fi-- > 0;) {
            const zdr::Journal& J = s.files[fi].J; if (J.tick > 0) tick = J.tick;
            for (std::map<int, zdr::ZoneView>::const_reverse_iterator it = J.z.rbegin(); it != J.z.rend() && (int)pick.size() < maxZ * 4; ++it) {
                const zdr::ZoneView& v = it->second; if (v.side != side) continue;
                if (!v.touched && v.ended) continue;                         // never touched and gone: not drawn
                const int a = barOf(times, (int)count, v.T), b = v.ended ? barOf(times, (int)count, v.endT) : (int)count - 1;
                if (b < v0 || a > v1) continue;
                pick.push_back(&v);
            }
        }
        std::sort(pick.begin(), pick.end(), [](const zdr::ZoneView* a, const zdr::ZoneView* b) { return a->T > b->T; });
        if ((int)pick.size() > maxZ) pick.resize(maxZ);
        for (size_t k = 0; k < pick.size(); ++k) if (tick > 0) zoneDraw(s, *pick[k], tick, V, (int)count, times, v0, v1);
    }
    return RTX_OK;
}

static const double ZONE_LIGHT = 1.22;   // (1.0.7, Rassul 2026-11-09 09:35 "make the zone slightly lighter") the band brightness x1.22
void ZoneDefenseMarks::zoneDraw(ZmState& s, const zdr::ZoneView& v, double tick, const ss::Values& V, int count, RTARRAYI& times, int, int) {
    const bool bid = v.side == 0; const COLOR base = bid ? zdh::C_GREEN : zdh::C_RED;
    const int a = barOf(times, count, v.T), b = v.ended ? barOf(times, count, v.state == 5 ? v.stateT : v.endT) : count - 1;   // (1.0.2) the band continues to a reclaim
    // brightness: resting now / peak (the latest decided number)
    long long now = v.size; if (v.touched) now = v.atTouch; if (v.reloaded) now = v.left; if (!v.barLeft.empty()) now = v.barLeft.rbegin()->second; if (v.ended) now = v.endResting;
    const double fr = v.size > 0 ? std::min(1.0, (double)now / v.size) : 0;
    const COLOR fill = zdh::scaleCol(base, ZONE_LIGHT * (v.ended ? 0.22 : 0.25 + 0.45 * fr));   // (1.0.7) ~22% lighter (Rassul 09:35)
    if (V.b(zdset::K_SHOW_ZONES)) {
        const int half = std::max(1, (int)getPixelsPerBar() / 2);
        PNT pa; pa.set(a, (float)((v.hi + 0.5) * tick), kBarCenter); PNT pb; pb.set(b, (float)((v.lo - 0.5) * tick), kBarCenter);
        RCT r; r.set(zdh::coord(pa.h - half), zdh::coord(pa.v), zdh::coord(pb.h + half), zdh::coord(std::max((int)pb.v, pa.v + 2)));
        setPen(fill, 1, P_SOLID); r.draw(0, fill, fill, DRAW_OPAQUE, PAT_SOLID);
        ++s.zonesDrawn;
        if (V.b(zdset::K_SHOW_BIG) && v.bigSz > 0 && v.bigPx >= v.lo && v.bigPx <= v.hi) {    // at most ONE big-order line per zone
            PNT p0; p0.set(a, (float)(v.bigPx * tick), kBarCenter); PNT p1; p1.set(b, (float)(v.bigPx * tick), kBarCenter);
            p0.h = zdh::coord(p0.h - half); p1.h = zdh::coord(p1.h + half); setPen(base, 1, P_SOLID); p0.setDrawPosition(); p1.drawLineTo();
        }
        if (V.b(zdset::K_SHOW_LABELS)) {
            FONT f; f.id = HELVETICA; f.size = 8; f.style = PLAIN; setFont(f);
            const std::string t = zdr::zoneShortLabel(v); const int tw = getTextWidth(t.c_str(), -1);   // (1.0.7) "301 PS0" (Rassul panel 4)
            const int x = r.left + 2, y = bid ? r.bottom + 1 : r.top - 12;       // bids: under the band, offers: over it
            ZmState::Box bx = { x, y, x + tw, y + 11 }; bool hit = false;
            for (size_t i = 0; i < s.labels.size(); ++i) { const ZmState::Box& o = s.labels[i]; if (bx.l < o.r && bx.r > o.l && bx.t < o.b && bx.b > o.t) { hit = true; break; } }
            if (!hit) { setTextColor(base); RCT q; q.set(zdh::coord(x), zdh::coord(y), zdh::coord(x + tw + 2), zdh::coord(y + 11)); q.drawText(t.c_str(), false, false); s.labels.push_back(bx); }
            else ++s.labelsSkipped;                                           // never overlap: the older label gives way
        }
    }
    // the gray refill DIAMOND on the touch candle, at the zone's edge
    if (V.b(zdset::K_SHOW_DIAMOND) && v.reloaded && v.refills > 0) {     // (L5) only where the zone refilled
        const int tb = barOf(times, count, v.touchT); PNT p; p.set(tb, (float)((bid ? v.hi : v.lo) * tick), kBarCenter);
        setPen(zdh::C_GRAY, 1, P_SOLID);
        for (int r = 0; r <= 4; ++r) { const int w = 4 - r; PNT x0, x1; x0.h = (short)(p.h - w); x1.h = (short)(p.h + w + 1); x0.v = x1.v = (short)(p.v - r); x0.setDrawPosition(); x1.drawLineTo(); x0.v = x1.v = (short)(p.v + r); x0.setDrawPosition(); x1.drawLineTo(); }
    }
    // the SWEEP triangle under the wick of the first stop run (offers: over it); green up = swept, red down = weak
    if (V.b(zdset::K_SHOW_SWEEP) && v.swept && v.runT > 0) {
        const int rbar = barOf(times, count, v.runT); RTARRAY hiA(barHigh), loA(barLow); if (hiA.count <= rbar || loA.count <= rbar) return;
        PNT c; c.set(rbar, bid ? (float)loA[rbar] : (float)hiA[rbar], kBarCenter);
        struct { int top, bottom; } w = { c.v, c.v };
        const bool good = v.sweep == 2; const COLOR col = zdh::colOfSign((good == bid) ? 1 : -1, zdp::forMarket(s.mkt).sweepSign); const bool up = good == bid;   // (L5) white where the sweep is not proven
        const int baseY = bid ? w.bottom + 3 : w.top - 3; setPen(col, 1, P_SOLID);
        for (int r = 0; r < 6; ++r) {
            const int y = bid ? (up ? baseY + r : baseY + 5 - r) : (up ? baseY - 5 + r : baseY - r);
            PNT x0, x1; x0.h = (short)(c.h - r); x1.h = (short)(c.h + r + 1); x0.v = x1.v = (short)y; x0.setDrawPosition(); x1.drawLineTo();
        }
    }
}

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { static_cast<ZoneDefenseMarks*>(this)->slot_.release(this); return RTX_OK; }
int cppExtension::setup(void)   { zdh::registerWindow(this, zdmIds()); return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    ZoneDefenseMarks* p = new ZoneDefenseMarks();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA Zone Defense marks (price chart) - the zones lsZoneDefense decided from the order feed: the band (green bids under price / red offers over it, brighter = more still resting; it ends when broken, faded below half or 60 min untouched), one label 'Zone lo-hi | lots (Nx) | oldest Nm | held/broken', one big-order line, the gray refill diamond on the touch candle and the sweep triangle on the first stop run (green up = swept: the run was lifted 1.0x+ within 5 s and price came back into the zone; red down = weak). Needs lsZoneDefense on the same chart (it writes the journal this reads). Never repainted.");
    zdh::stamp(p);   // ZD_VERSION
    return p;
}
#endif
