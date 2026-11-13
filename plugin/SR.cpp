// SR.cpp (1.1.3, 2026-11-13 Rassul 10:30 "remove the 3/4, it is too noisy": the strength grade ("3/4?") is no longer drawn - still graded and recorded; 1.1.2, 2026-11-12: no grey lines across the pane; 1.1.1: independent review - one writer per CHART, run ids over every line, idle candles close; 1.1.0: + the strength grade of a run, SRGradeParams.h; 1.0.0, 2026-11-10) -- lsSR, the "SR" stop-run pane (Rassul 2026-11-10 18:24-18:50 CT, mockup v10
// claude.ai/artifact/EctbmGWXrGd5cWxA7JSGKv; "call the new indicator SR instead of StopRun"). Its own pane on the order feed
// (off::MboPane, as lsZoneDefense): every order event goes once through onRaw into sr::Engine (SRLogic.h, the ONE copy of the rules,
// checked against the study: tests_sr2/). Every decision is one journal line (lsFlexLevels\SR\<MKT>-<spb>-<session>.csv, append-only);
// the pane draws ONLY from the journal, so a restart shows the same bars / gold runs / arrows (no repaint).
//   per candle: buyers' burst up (green), sellers' burst down (red); faded < 2.0x; bright + ratio at 2.0x on a 5-bar high / low;
//   GOLD + ratio = a stop run through a key / MenthorQ level; arrow = the candle that closes back across it (drawn where SRParams
//   drawRev = 1: GC, HG; recorded only elsewhere); gray diamond = the attackers' own side was pulling on that candle.
// Times: IRT stamps order events to the whole second; inside a second the events are spaced by their ARRIVAL (steady clock), which is
// what the 300 ms concentration test uses live (the study used exchange times) - the nightly lra.sr_score measures the difference.
// No hovers, no text on the chart, no settings (the base dialog stays empty by design: 2026-10-27 "no settings, just defaults").
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
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "SR"
#endif
#include "PerfGuard.h"
#include "SettingsGuard.h"
#include "HostSlot.h"
#include "SettingsSpec.h"
#include "SettingsHost.h"
#include "SRLogic.h"
#include "SRParams.h"
#include "SRGradeParams.h"
#include "MarketKey.h"
#include "NearLevels.h"
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

#define SR_VERSION "1.1.3"
static const bool SR_DRAW_GRADE = false;   // (1.1.3, Rassul 2026-11-13 10:30 "remove the 3/4, it is too noisy") the grade text is not drawn; the grade is still decided, recorded and scored nightly   // = sr::VERSION = the one setVersion literal

namespace srh {
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
inline std::string journalDir(const std::string& flex) { return flex.empty() ? std::string() : flex + "\\SR"; }
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
    std::string path; long long off = 0, mt = -1, sz = -1; sr::Journal J; std::string partial;
    bool poll() {
        long long m = 0, s = 0; if (path.empty() || !stat2(path, m, s)) return false;
        if (s < off) { off = 0; J = sr::Journal(); partial.clear(); }                 // a new / rewritten file
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
inline long long wallNowLocal(long long offS, long long stamp) { return stamp > 0 ? stamp + offS : 0; }
} // namespace srh

#define OFF_SDK 1
#include "OrderFlowFeed.h"
namespace sr {
struct PaneLogic {   // the MboPane contract (the base's own bars / records are not drawn by this pane)
    static const char* name() { return "SR"; }
    static const char* title() { return "SR"; }
    static const char* version() { return SR_VERSION; }
    static int metrics() { return 3; }
    static int paceMetric() { return off::PM_ADD; }
    static int optGate() { return ofo::G_OSTACK; }
    static std::string optLine(const ofo::Snap& s, int gate, const std::string&) { return ofo::stackLine(s, gate); }
    static bool resets() { return false; }
    static std::string hoverQuestion() { return "Were stops run through a level?"; }
    static const char* const* hoverVerbs() { static const char* const v[2] = { "Buyers", "Sellers" }; return v; }
    static off::Info info(const off::PaneBar*, const off::Params&) { return off::Info(); }
    static long long value(const off::Rec& r, int metric, int s) { return metric == 0 ? r.addC[s] : metric == 1 ? r.pullC[s] : r.refN[s]; }
    static void decide(off::PaneBar& b, const off::Params&, bool) { for (int s = 0; s < 2; ++s) { b.spike[s] = false; b.dash[s] = -1; b.let[s].clear(); } b.msg = decltype(b.msg)(); }
};
} // namespace sr

class SRPane : public off::MboPane<sr::PaneLogic> {
public:
    SS_SAFE_DIALOG(SRPane, "SR")
    SRPane() : off::MboPane<sr::PaneLogic>() {}
    virtual bool barMarks() const { return false; }
    virtual bool textOff() const { return true; }
    virtual int parmsTitle(char* pStr, int size) { return off::copyTitle(pStr, size, sr::PaneLogic::title(), SR_VERSION) ? RTX_OK : RTX_FAIL; }
    int parmsLoadBody(void) { return RTX_OK; }            // no settings (defaults only)
    int parmsApplyBody(void) { return RTX_OK; }
    int parmsUpdtBody(unsigned int) { return RTX_OK; }
    struct Ext : off::PaneExt {
        std::unique_ptr<sr::Engine> eng; std::string mkt, session, path, note, symbol, lvlNote; int spb = 0; double tick = 0;
        size_t written = 0; long long writeFails = 0, events = 0, skipped = 0, sessions = 0, fills = 0;
        long long stamp = -1, localSec = 0, offS = 0, secArr0 = 0; bool haveOff = false;
        srh::JFile jf; nl::Cache lvc; long long lvlSig = -1; size_t nLevels = 0;
        bool writer = false; void* lock = nullptr; long long tornFixed = 0, drawn = 0, lastTick = 0;
    };
    Ext* ext(S_& S) { if (!S.ext) S.ext.reset(new (std::nothrow) Ext()); return static_cast<Ext*>(S.ext.get()); }
    std::string mktOf(S_& S) { return srh::marketOf(S.market); }
    static long long daysOfTm(const struct tm& t) { long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, d = (unsigned)t.tm_mday; y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
        const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; return era * 146097 + (long long)doe - 719468; }
    void refreshLevels(S_& S, Ext& e) {
        const std::string flex = srh::flexDir(); if (flex.empty() || !(e.tick > 0) || !e.eng) return;
        const std::vector<nl::Level> v = nl::levels(e.lvc, flex + "\\", e.mkt, S.symbol, off::steadyMs(), srh::wallNow());
        std::vector<sr::Level> out; unsigned long long sig = 1469598103ULL;
        for (size_t i = 0; i < v.size(); ++i) { if (!(v[i].px > 0) || v[i].name.empty()) continue;
            sr::Level l; l.name = v[i].name; for (size_t c = 0; c < l.name.size(); ++c) if (l.name[c] == '|' || l.name[c] == ',' || l.name[c] == '\n' || l.name[c] == '\r') l.name[c] = ' ';
            l.px = (long long)std::llround(v[i].px / e.tick); l.side = sr::Engine::sideOfName(l.name); out.push_back(l);
            for (size_t c = 0; c < l.name.size(); ++c) sig = sig * 131 + (unsigned char)l.name[c];
            sig = sig * 1000003ULL + (unsigned long long)l.px; }
        e.lvlNote = e.lvc.note;
        const long long sg = (long long)(sig & 0x3FFFFFFFFFFFFFFFULL); if (sg == e.lvlSig && out.size() == e.nLevels) return;
        e.lvlSig = sg; e.nLevels = out.size(); e.eng->setLevels(out);
    }
    virtual void onRaw(S_& S, const DEPTH_ORDER& o) {
        Ext* e = ext(S); if (!e) return;
        const std::string mkt = mktOf(S);
        if (mkt.empty()) { e->note = "market not supported (" + S.market + ")"; ++e->skipped; return; }
        if (S.spb <= 0 || S.spb > 3600 || 3600 % S.spb != 0) { e->note = "needs 1-60 min bars that divide the hour"; ++e->skipped; return; }
        if (!(S.tick > 0) || !o.timestamp) { ++e->skipped; return; }
        e->note.clear();
        const long long arr = off::steadyMs();
        if ((long long)o.timestamp != e->stamp) {                          // one clock conversion per second
            struct tm t; std::memset(&t, 0, sizeof t);
            if (!getLocaltime(o.timestamp, &t)) { ++e->skipped; return; }
            e->stamp = (long long)o.timestamp; e->localSec = daysOfTm(t) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec; e->offS = e->localSec - e->stamp; e->haveOff = true; e->secArr0 = arr;
            const std::string sess = srh::sessionOf(t);
            if (!e->eng || sess != e->session || mkt != e->mkt || (int)S.spb != e->spb) openSession(S, *e, sess, mkt);
            if (e->eng) refreshLevels(S, *e);
        }
        if (!e->eng || !e->writer) return;
        off::Ev x = off::evOf(o, S.tick, arr);
        if (x.action > 3 || x.px <= 0) { ++e->skipped; return; }
        long long ms = arr - e->secArr0; if (ms < 0) ms = 0;
        if (ms > 999) ms = 999;
        long long tMs = e->localSec * 1000 + ms; if (tMs < e->lastTick) tMs = e->lastTick;   // never backwards (a re-sent row keeps its arrival order)
        e->lastTick = tMs; ++e->events;
        if (x.action == off::FILL) { if (x.size <= 0) return;
            if (x.implied || x.side < 0) { e->eng->onVolume(tMs, x.size); return; }   // (1.1.0) an implied fill still counts in the candle's volume
            ++e->fills;
            e->eng->onFill(tMs, (long long)x.px, x.size, x.side == off::BID ? -1 : 1, (unsigned long long)(x.aggr ? x.aggr : x.id), (unsigned long long)(x.aggr ? x.id : 0)); }
        else if (x.action == off::NEW) e->eng->onBook(tMs, x.side, true, (long long)x.px, x.size);
        else if (x.action == off::DEL) e->eng->onBook(tMs, x.side, false, (long long)x.px, x.size);
        else e->eng->advance(tMs);
    }
    void openSession(S_& S, Ext& e, const std::string& sess, const std::string& mkt) {
        if (e.eng) { if (e.haveOff && e.writer) e.eng->advance(e.localSec * 1000); flush(e); }   // (1.1.1) the old session's last candle and open runs are closed before the journal changes
        if (e.writer) srh::unclaim(e.path, &e, e.lock);
        e.mkt = mkt; e.spb = (int)S.spb; e.tick = S.tick; e.session = sess; ++e.sessions;
        e.eng.reset(new (std::nothrow) sr::Engine()); if (!e.eng) return;
        const srp::Mkt& M = srp::forMarket(mkt); e.eng->spb = e.spb; e.eng->rthOpenSec = M.rthOpen * 60LL; e.eng->rthCloseSec = M.rthClose * 60LL; e.eng->P.recBars = M.recBars; e.eng->P.drawRev = M.drawRev; e.eng->P.burstX = M.burstX;
        { const srg::Mkt& G = srg::forMarket(mkt); e.eng->P.bigA = G.bigA; e.eng->P.bigP = G.bigP; e.eng->P.gShare = G.share; e.eng->P.gBids = G.bids; e.eng->P.gDeep = G.deep; e.eng->P.gHeavy = G.heavy; e.eng->P.gStrong = G.strong; }   // (1.1.0) the grade
        e.written = 0; e.lvlSig = -1; e.nLevels = 0; e.lastTick = 0;
        const std::string flex = srh::flexDir(); srh::makeDir(flex); srh::makeDir(srh::journalDir(flex));
        e.path = srh::journalPath(flex, e.mkt, e.spb, sess);
        e.writer = srh::claim(e.path, &e, e.lock);   // (1.1.1) the owner is the CHART (its Ext), not the shared extension object
        e.jf = srh::JFile(); e.jf.path = e.path; e.jf.poll();
        const int maxId = e.jf.J.maxId;   // (1.1.1) over every R / G line, not only the gold runs
        e.eng->setNextId(maxId + 1000);                                   // run ids of this start never collide with a journal id
        if (!e.writer || e.path.empty()) return;
        long long mt = 0, sz = 0;
        if (!srh::stat2(e.path, mt, sz)) { std::ofstream f(e.path.c_str(), std::ios::binary | std::ios::app); char b[64]; std::snprintf(b, sizeof b, "H|%.10g|%s\n", e.tick, SR_VERSION); f << b; return; }
        { std::ifstream in(e.path.c_str(), std::ios::binary); if (sz > 0) { in.seekg(sz - 1); char c = 0; in.get(c); if (c != '\n') { std::ofstream f(e.path.c_str(), std::ios::binary | std::ios::app); f << "\n"; ++e.tornFixed; } } }
    }
    void stopAll() { S_* S = testState(); if (!S) return; Ext* e = ext(*S); if (!e) return; if (e->eng && e->writer) flush(*e); if (e->writer) srh::unclaim(e->path, e, e->lock); e->writer = false; }
    void flush(Ext& e) {
        if (!e.eng || !e.writer || e.path.empty() || e.written >= e.eng->journal.size()) return;
        std::ofstream f(e.path.c_str(), std::ios::binary | std::ios::app);
        if (!f.is_open()) { ++e.writeFails; return; }
        for (; e.written < e.eng->journal.size(); ++e.written) { const std::string& l = e.eng->journal[e.written];
            if (!e.jf.J.bars.empty() && l.size() > 2 && l[0] == 'B') { const long long end = std::atoll(l.c_str() + 2); if (e.jf.J.bars.count(end)) continue; }   // a candle decided before the restart stays as it was
            f << l << "\n"; }
        f.flush(); if (!f.good()) ++e.writeFails;
        if (e.eng->journal.size() > 20000) { e.eng->journal.clear(); e.written = 0; }
    }
    virtual void afterPump(S_& S, long long, bool) { Ext* e = ext(S); if (e) { if (e->eng && e->haveOff) { const long long idle = off::steadyMs() - e->secArr0; if (idle > IDLE_MS + 1000) e->eng->advance(e->localSec * 1000 + std::min<long long>(idle - IDLE_MS, 3600000LL)); }   // (1.1.1) a quiet feed: time still passes (candles close, pending runs are decided), IDLE_MS behind the clock so a late event never lands in a closed candle
             flush(*e); e->jf.poll(); } }
    virtual void afterTimer(S_& S, long long) { Ext* e = ext(S); if (e) flush(*e); }
    virtual void statusExtra(S_& S, std::ostream& o) {
        Ext* e = ext(S); if (!e) { o << "\nSR,unavailable"; return; }
        const srp::Mkt& M = srp::forMarket(e->mkt.empty() ? S.market : e->mkt); const srg::Mkt& G = srg::forMarket(e->mkt.empty() ? S.market : e->mkt);
        o << "\nSR,version " << SR_VERSION << ",market " << e->mkt << (e->writer ? ",writer" : ",read-only (another chart writes this journal)") << ",session " << e->session << ",events " << e->events << ",fills " << e->fills << ",skipped " << e->skipped
          << ",levels " << e->nLevels << " (" << e->lvlNote << ")" << (e->note.empty() ? std::string() : ",note " + e->note)
          << ",journal " << e->path << ",lines " << e->jf.J.lines << ",bad " << e->jf.J.bad << ",gold runs " << e->jf.J.gold.size() << ",reversals " << e->jf.J.rev.size() << ",write fails " << e->writeFails << ",torn lines fixed " << e->tornFixed
          << "\nSR_PARAMS,burstX " << M.burstX << ",recBars " << M.recBars << ",drawRev " << M.drawRev << ",rth " << M.rthOpen << "-" << M.rthClose << ",atr " << (e->eng ? e->eng->atr() : 0)
          << "\nSR_GRADE,graded runs " << e->jf.J.grade.size() << ",open " << (e->eng ? e->eng->openGrades() : 0) << ",big buyer " << G.bigA << " lots,big bid " << G.bigP << " lots,share " << G.share << ",bids " << G.bids << ",deep " << G.deep << " ATR,heavy " << G.heavy
          << "x,drawn from grade " << G.strong << (G.draw == 0 ? ",logged only" : G.draw == 1 ? ",drawn with ?" : ",proven") << ",ATR14 " << (e->eng ? e->eng->atr14() : 0) << ",normal candle volume " << (e->eng ? e->eng->medVol() : 0)
          << "\nSR_DRAW,draws " << e->drawn;
    }
    void srCalc() { pump(); S_* S = testState(); if (!S) return; Ext* e = ext(*S); if (e && e->jf.poll()) invalidateChart(false); }
    virtual int scale(int, int, double* dMin, double* dMax) { if (dMin) *dMin = 0; if (dMax) *dMax = 1; return RTX_OK; }
    virtual int draw(void) {
        LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
        pump(true);
        S_* S = testState(); if (!S) return RTX_OK;
        Ext* e = ext(*S); if (!e) return RTX_OK;
        try { render(*S, *e); } catch (...) { S->state = "draw error"; }
        return RTX_OK;
    }
    static const long long IDLE_MS = 5000;
    static const COLOR C_GOLD = 0x0038A5E0;   // BGR of #E0A538
    static COLOR dim(COLOR c) { return srh::scaleCol(c, 0.32); }
    // (1.1.0) dir -1 = a label above its bar, +1 = below: a label that would sit on one already drawn in this pass moves one text row
    // further out (up to 4 rows), kept inside the pane - neighbouring candles' ratios and grades never overlap
    std::vector<int> lab_; int labTop_ = 0, labBot_ = 0;
    void label(const std::string& t, int cx, int y, COLOR c, int dir = 0) {
        const int w = getTextWidth(t.c_str(), -1), x0 = cx - w / 2, x1 = x0 + w;
        if (dir != 0) for (int k = 0; k < 4; ++k) {
            bool hit = false; for (size_t i = 0; i + 3 < lab_.size(); i += 4) if (x0 < lab_[i + 2] + 1 && x1 > lab_[i] - 1 && y < lab_[i + 3] && y + 11 > lab_[i + 1]) { hit = true; break; }
            if (!hit) break;
            const int ny = y + dir * 11; if (ny < labTop_ || ny + 11 > labBot_) break;
            y = ny; }
        lab_.push_back(x0); lab_.push_back(y); lab_.push_back(x1); lab_.push_back(y + 11);
        setTextColor(c); RCT r; r.set(off::coord(x0), off::coord(y), off::coord(x1), off::coord(y + 11)); r.drawText(t.c_str(), false, false);
    }
    void render(S_& S, Ext& e) {
        const std::string mkt = mktOf(S);
        const long count = getBarCount(); if (count < 2) return;
        RTARRAYI times(barDateTime); if (times.count < count) return;
        if (e.jf.path.empty() && !mkt.empty() && S.spb > 0) {              // no order event yet (a restart): the session of the last candle
            struct tm t; std::memset(&t, 0, sizeof t);
            if (getLocaltime((RTDATE)times[(int)count - 1] - 1, &t)) { e.jf.path = srh::journalPath(srh::flexDir(), mkt, (int)S.spb, srh::sessionOf(t));
                if (!e.haveOff) { e.offS = daysOfTm(t) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec - ((long long)times[(int)count - 1] - 1); e.haveOff = true; } }
        }
        e.jf.poll(); ++e.drawn;
        RCT pane; pane.getPaneRect(false);
        int v0 = 0, v1 = 0; if (getVisibleBars(&v0, &v1) != RTX_OK) return; v0 = std::max(0, v0); v1 = std::min((int)count - 1, v1);
        const int top = pane.top + 14, bottom = pane.bottom - 14, mid = (top + bottom) / 2, half = (bottom - top) / 2; if (half < 8) return;
        const double CAP = 8.0, burstX = srp::forMarket(mkt).burstX; const bool drawRev = srp::forMarket(mkt).drawRev != 0; const int gDraw = srg::forMarket(mkt).draw, gStrong = srg::forMarket(mkt).strong;
        const int ppb = std::max(1, (int)getPixelsPerBar()), w = std::max(1, ppb * 6 / 10);
        // (1.1.2, Rassul 2026-11-12 20:58 "remove the gray lines") the three grey lines across the pane (the zero line and the two burst limits) are no longer drawn
        FONT f; f.id = HELVETICA; f.size = 9; f.style = BOLD; setFont(f);
        lab_.clear(); labTop_ = pane.top + 1; labBot_ = pane.bottom - 1;
        const sr::Journal& J = e.jf.J;
        for (int i = v0; i <= v1; ++i) {
            const long long k = (long long)times[i] + e.offS; PNT p; p.set(i, 0.0f, kBarCenter); const int cx = p.h, x0 = cx - w / 2;
            std::map<long long, sr::JBar>::const_iterator b = J.bars.find(k); std::map<long long, sr::JRun>::const_iterator g = J.gold.find(k); std::map<long long, sr::JRev>::const_iterator r = J.rev.find(k);
            int hU = 0, hD = 0; bool ratU = false, ratD = false;
            if (b != J.bars.end()) {
                const bool brU = b->second.xU >= burstX && b->second.hiN, brD = b->second.xD >= burstX && b->second.loN;
                hU = (int)(half * std::min(b->second.xU, CAP) / CAP); hD = (int)(half * std::min(b->second.xD, CAP) / CAP);
                if (hU > 0) off::fillFlat(this, x0, mid - hU, x0 + w, mid, brU ? srh::C_GREEN : dim(srh::C_GREEN));
                if (hD > 0) off::fillFlat(this, x0, mid + 1, x0 + w, mid + 1 + hD, brD ? srh::C_RED : dim(srh::C_RED));
                char t[24];
                if (brU && !(g != J.gold.end() && g->second.side > 0)) { std::snprintf(t, sizeof t, "%.1f", b->second.xU); label(t, cx, mid - hU - 12, srh::C_GREEN, -1); ratU = true; }
                if (brD && !(g != J.gold.end() && g->second.side < 0)) { std::snprintf(t, sizeof t, "%.1f", b->second.xD); label(t, cx, mid + hD + 2, srh::C_RED, 1); ratD = true; }
            }
            if (g != J.gold.end()) {                                       // the stop run through a level: gold, its ratio just outside the bar
                const int h = std::max((int)(half * burstX / CAP), (int)(half * std::min(g->second.x, CAP) / CAP)); char t[24]; std::snprintf(t, sizeof t, "%.1f", g->second.x);
                if (g->second.side > 0) { off::fillFlat(this, x0, mid - h, x0 + w, mid, C_GOLD); label(t, cx, mid - h - 12, C_GOLD, -1); hU = std::max(hU, h); ratU = true; }
                else { off::fillFlat(this, x0, mid + 1, x0 + w, mid + 1 + h, C_GOLD); label(t, cx, mid + h + 2, C_GOLD, 1); hD = std::max(hD, h); ratD = true; }
            }
            if (SR_DRAW_GRADE && gDraw > 0) for (int sd = 1; sd >= -1; sd -= 2) {           // (1.1.0) the strength grade of the run through a level on this candle: one text row outside the ratio
                const sr::JGrade* q = J.gradeAt(k, sd); if (!q) continue;
                const bool isGold = g != J.gold.end() && g->second.side == sd; if (!isGold && q->score < gStrong) continue;
                char t[16]; std::snprintf(t, sizeof t, "%d/4%s", q->score, gDraw == 1 ? "?" : "");
                int y = sd > 0 ? mid - hU - 12 - (ratU ? 11 : 0) : mid + hD + 2 + (ratD ? 11 : 0);
                if (y < pane.top + 1) y = pane.top + 1;
                if (y > pane.bottom - 11) y = pane.bottom - 11;
                label(t, cx, y, C_GOLD, sd > 0 ? -1 : 1); if (sd > 0) hU += 11; else hD += 11;   // the arrow, if any, goes outside it
            }
            if (r != J.rev.end() && drawRev) {                             // the reversal: an arrow on that candle's bar (red down after a high, green up after a low)
                const bool dn = r->second.side > 0; const COLOR c = dn ? srh::C_RED : srh::C_GREEN; const int a = std::max(3, std::min(6, w / 2 + 1));
                const int y = dn ? mid + hD + (g != J.gold.end() && g->second.side < 0 ? 16 : 4) : mid - hU - (g != J.gold.end() && g->second.side > 0 ? 16 : 4);
                for (int j = 0; j < a; ++j) { const int yy = dn ? y + 8 - j + a - 1 - (a - 1) : y - 8 + j; (void)yy; }
                if (dn) { off::fillFlat(this, cx - 1, y, cx + 2, y + 6, c); for (int j = 0; j < a; ++j) off::fillFlat(this, cx - (a - 1 - j), y + 6 + j, cx + (a - 1 - j) + 1, y + 7 + j, c); }
                else { off::fillFlat(this, cx - 1, y - 6, cx + 2, y, c); for (int j = 0; j < a; ++j) off::fillFlat(this, cx - (a - 1 - j), y - 7 - j, cx + (a - 1 - j) + 1, y - 6 - j, c); }
                if (r->second.pull) { const int yd = dn ? y + 6 + a + 5 : y - 6 - a - 5; for (int j = 0; j <= 3; ++j) { off::fillFlat(this, cx - (3 - j), yd - 3 + j, cx + (3 - j) + 1, yd - 2 + j, srh::C_GRAY); off::fillFlat(this, cx - (3 - j), yd + 3 - j, cx + (3 - j) + 1, yd + 4 - j, srh::C_GRAY); } }
            }
        }
        if (!e.note.empty() || mkt.empty()) { FONT fp; fp.id = HELVETICA; fp.size = 9; fp.style = PLAIN; setFont(fp); const std::string t = mkt.empty() ? "market not supported (" + S.market + ")" : e.note; setTextColor(srh::C_GRAY); RCT rr; rr.set(off::coord(pane.left + 6), off::coord(pane.top + 2), off::coord(pane.left + 6 + getTextWidth(t.c_str(), -1)), off::coord(pane.top + 14)); rr.drawText(t.c_str(), false, false); }
    }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<SRPane*>(this)->stopAll(); static_cast<SRPane*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<SRPane*>(this)->stopAll(); static_cast<SRPane*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void)   { return RTX_OK; }
int cppExtension::calc(int)     { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<SRPane*>(this)->srCalc(); return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    SRPane* p = new SRPane();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI);                                    // its own pane, draws itself; no hovers
    p->setExtendedFlags(CALL_CONTINUOUSLY);                              // the order feed keeps coming while no trade prints
    p->setDescription("LRA SR - stop runs (market-by-order feed). Per candle: the strongest one-second burst of small market orders, buyers up (green) and sellers down (red), as a ratio of the local normal; faded below 2.0x, bright with its ratio at 2.0x on a 5-bar high / low. GOLD with its ratio = a stop run through a key / MenthorQ level. 3/4 = the strength grade of a run through a level once a candle closes back over it (big buyers, big bids filled, 1 ATR deep, a candle of selling beyond the level). Arrow = the candle that closes back across that level (red down = short after a high, green up = long after a low). Gray diamond = the side that ran the stops was pulling its own orders on that candle.");
    p->setVersion("1.1.3");
    return p;
}
