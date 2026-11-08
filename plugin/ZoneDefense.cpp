/********************************************************************************
 *  ZoneDefense.cpp  --  Investor/RT RTX extensions  lsZoneDefense 1.0.1  +  lsZoneDefenseMarks 1.0.1  (2026-11-08)
 *  1.0.1 (Rassul 2026-11-08, mockup claude.ai/artifact/MJ9YYwjeMAUQztNZrCiP1G "before / after"): the Read block gets the Zone line and
 *  every line is NUMBERED in event order - 1. Zone, 2. Queue, 3. Pressure (was "Reload"), 4. Refill, 5. Sweep - in aligned columns (labels left, the
 *  verdict word bold at one x, the detail in a calmer tint on the same line, times right-aligned); the pane bars are dimmed.
 *  Rassul 2026-11-08 "Zone Defense is good. lets build it" - design approved on claude.ai/artifact/MJ9YYwjeMAUQztNZrCiP1G.
 *
 *  lsZoneDefense (this file as is) - its own pane, "Zone Defense v1.0.1", on the live market-by-order feed (the same order-feed path
 *  as lsBookFlow: off::MboPane, the 250 ms clock, every event once through onRaw). Every event goes to zd::Engine
 *  (ZoneDefenseLogic.h, SDK-free, the same code the jumpstart / nightly ran over 71 market-days of raw MBO). Each decision is a
 *  journal line in lsFlexLevels\ZoneDefense\<MKT>-<spb>-<session>.csv, written once; the pane and the price chart draw ONLY from
 *  the journal (ZoneDefenseRead.h) - nothing repaints, a restart redraws the same picture.
 *    The pane: 4 Read lines for the latest touched zone (verdict first, green bullish / red bearish / white neutral, the time
 *    right-justified, 12-hour) - Zone, Queue, Pressure, Refill, Sweep (1.0.1) - and per candle the lots sold into the zone (red bar) and the bid
 *    (offer) left in it (green dot). Mirror for resistance / sellers. No hovers.
 *  lsZoneDefenseMarks (ZoneDefenseMarks.cpp: #define ZD_MARKS 1 + #include this file) - the price-chart overlay: the zone band
 *    (green bids / red offers, brightness = resting / peak, ends when broken, faded below 50% or 60 min untouched), its one label
 *    "Zone lo-hi · lots (Nx) · oldest Nm · held/broken", at most ONE big-order line per zone, the gray reload DIAMOND on the touch
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

#define ZD_VERSION "1.0.1"   // = zdset::PLUGIN_VERSION = zd::VERSION = the one setVersion literal (zdh::stamp)


namespace zdh {
// the version of both DLLs: the ONE setVersion literal (the settings contract counts it; gex-build reads it)
inline void stamp(cppExtension* p) { p->setVersion("1.0.1"); }
// the one settings window of both plugins (registered once per DLL)
inline void registerWindow(cppExtension* x, ss::Ids& ids) { ss::registerAll(x, zdset::spec(), ids); }
// file helpers shared by the two plugins
inline short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
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
inline std::string marketOf(std::string root) {
    for (size_t i = 0; i < root.size(); ++i) root[i] = (char)std::toupper((unsigned char)root[i]);
    if (root == "6E" || root == "EU" || root == "M6E") return "EU";
    static const char* const K[] = { "MES", "ES", "MNQ", "NQ", "MCL", "CL", "QG", "NG", "MGC", "GC", "MHG", "HG" };
    for (int i = 0; i < 12; i += 2) { if (root.compare(0, std::strlen(K[i]), K[i]) == 0) return K[i + 1]; }
    for (int i = 1; i < 12; i += 2) { if (root.compare(0, std::strlen(K[i]), K[i]) == 0) return K[i]; }
    return std::string();
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
// (1.0.1) the detail tint of each line colour (the same hue, calmer) and the dimmed pane bars (never fight the text)
static const COLOR C_GREEN_T = 0x008FD6AB, C_RED_T = 0x00F2A0A0, C_WHITE_T = 0x00A9B2BD, C_BAR_RED = 0x006E2A2A, C_DOT_GREEN = 0x003F8F5F;
inline COLOR tintOf(COLOR c) { return c == C_GREEN ? C_GREEN_T : c == C_RED ? C_RED_T : C_WHITE_T; }
inline COLOR scaleCol(COLOR c, double f) { if (f < 0) f = 0; if (f > 1) f = 1; const int r = (int)(((c >> 16) & 255) * f), g = (int)(((c >> 8) & 255) * f), b = (int)((c & 255) * f); return (COLOR)((r << 16) | (g << 8) | b); }
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
    };
    Ext* ext(S_& S) { if (!S.ext) S.ext.reset(new (std::nothrow) Ext()); return static_cast<Ext*>(S.ext.get()); }
    // ---- the feed: every order event, once, in order ------------------------------------------------------------------
    virtual void onRaw(S_& S, const DEPTH_ORDER& o) {
        Ext* e = ext(S); if (!e || S.market.empty() || !(S.tick > 0) || S.spb <= 0 || !o.timestamp) { if (e) ++e->skipped; return; }
        const long long nowMs = off::steadyMs();
        if ((long long)o.timestamp != e->stamp) {                         // one clock conversion per second
            struct tm t; std::memset(&t, 0, sizeof t);
            if (!getLocaltime(o.timestamp, &t)) { ++e->skipped; return; }
            e->stamp = (long long)o.timestamp; e->stampMs = nowMs;
            const std::string sess = zdh::sessionOf(t);
            if (!e->eng || sess != e->session || S.market != e->mkt || (int)S.spb != e->spb) openSession(S, *e, sess);
            const long long utcMin = e->stamp / 60; e->ctOff = (int)(((t.tm_hour * 60 + t.tm_min) - (utcMin % 1440) + 2880) % 1440);
            if (e->eng) e->eng->ctOffsetMin = e->ctOff;
        }
        if (!e->eng) return;
        const off::Ev x = off::evOf(o, S.tick, nowMs);
        if (x.action > 3 || x.px <= 0) { ++e->skipped; return; }
        zd::In in; in.t = e->stamp * 1000 + std::min(999LL, std::max(0LL, nowMs - e->stampMs)); in.act = x.action; in.side = x.side; in.px = x.px; in.sz = x.size; in.oid = x.id;
        e->eng->feed(in); ++e->events;
    }
    void openSession(S_& S, Ext& e, const std::string& sess) {
        if (e.eng) { e.eng->finish(e.eng->lastT + 1); flush(e); }
        e.mkt = S.market; e.spb = (int)S.spb; e.tick = S.tick; e.session = sess; ++e.sessions;
        e.eng.reset(new (std::nothrow) zd::Engine(zdp::forMarket(e.mkt))); if (!e.eng) return;
        e.eng->barSec = e.spb; e.written = 0;
        const std::string flex = zdh::flexDir(); zdh::makeDir(flex); zdh::makeDir(zdh::journalDir(flex));
        e.path = zdh::journalPath(flex, e.mkt, e.spb, sess);
        e.jf = zdh::JFile(); e.jf.path = e.path; e.jf.poll();             // a restart: what was decided before stays (no repaint)
        int maxId = 0; for (std::map<int, zdr::ZoneView>::const_iterator it = e.jf.J.z.begin(); it != e.jf.J.z.end(); ++it) maxId = std::max(maxId, it->first);
        e.eng->nextId = maxId + 1;                                         // new zones never reuse a journal id
        long long mt = 0, sz = 0;
        if (!e.path.empty() && !zdh::stat2(e.path, mt, sz)) { std::ofstream f(e.path.c_str(), std::ios::binary | std::ios::app); char b[64]; std::snprintf(b, sizeof b, "H,%.10g,%s\n", e.tick, ZD_VERSION); f << b; }
    }
    void flush(Ext& e) {
        if (!e.eng || e.path.empty() || e.written >= e.eng->out.size()) return;
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
        o << "\nZONE_DEFENSE,version " << ZD_VERSION << ",market " << e->mkt << ",session " << e->session << ",events " << e->events << ",skipped " << e->skipped
          << ",zones " << (e->eng ? e->eng->zones.size() : 0) << ",journal " << e->path << ",lines " << e->jf.J.lines << ",bad " << e->jf.J.bad << ",write fails " << e->writeFails
          << "\nZONE_DEFENSE_PARAMS,pressureX " << M.per[0].reloadX << ",signs pressure " << M.reloadSign << " pull " << M.pullSign << " sweep " << M.sweepSign << ",liftX " << zd::LIFT_X
          << "\nZONE_DEFENSE_DRAW,lines drawn " << e->drawn;
        for (size_t i = 0; i < e->lastLines.size(); ++i) o << " | " << e->lastLines[i].text;
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
        if (e.jf.path.empty() && !S.market.empty() && S.spb > 0) {           // no order event yet (a restart): the session of the last candle
            const long count = getBarCount(); RTARRAYI times(barDateTime);
            if (count > 0 && times.count >= count) { struct tm t; std::memset(&t, 0, sizeof t);
                if (getLocaltime((RTDATE)times[(int)count - 1] - 1, &t)) e.jf.path = zdh::journalPath(zdh::flexDir(), S.market, (int)S.spb, zdh::sessionOf(t)); }
        }
        e.jf.poll();
        RCT pane; pane.getPaneRect(false);
        const ss::Values& V = settings(e, S.market);
        const int left = pane.left + 6, right = pane.right - 6;
        int y = pane.top + 3;
        const zdr::ZoneView* z = e.jf.J.latestTouched();
        if (V.b(zdset::K_SHOW_LINES)) {
            if (!z) { std::vector<zdr::RLine> w(1); zdr::setLine(w[0], "Zone", "waiting", "for a zone touch"); drawBlock(w, left, right, y); y += LINE_H; e.lastLines.clear(); }
            else {
                e.lastLines = zdr::readLines(*z, [this](long long t) { return clockOf(t); }, zdh::signsOf(S.market), S.tick);
                drawBlock(e.lastLines, left, right, y); y += LINE_H * (int)e.lastLines.size();
                ++e.drawn;
            }
        }
        if (V.b(zdset::K_SHOW_BARS)) drawBars(S, e, y + 4, pane);
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
        const int xV = left + labW + 12, xT = right - timeW, lim = xT - 10;
        for (size_t i = 0; i < L.size(); ++i) {
            const int y = y0 + (int)i * LINE_H; const COLOR c = zdh::colOf(L[i].col);
            setFont(fp); put(std::to_string(i + 1) + ". " + L[i].label, left, y, zdh::C_GRAY, lim);
            setFont(fb); const int vw = put(L[i].verdict, xV, y, c, lim);
            setFont(fp); if (!L[i].detail.empty() && xV + vw + 8 < lim) put(L[i].detail, xV + vw + 8, y, zdh::tintOf(c), lim);
            if (!L[i].time.empty()) put(L[i].time, right - getTextWidth(L[i].time.c_str(), -1), y, zdh::C_GRAY, right + 1);
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
        std::map<long long, long long> sold, left;
        for (std::map<int, zdr::ZoneView>::const_iterator it = e.jf.J.z.begin(); it != e.jf.J.z.end(); ++it) {
            for (std::map<long long, long long>::const_iterator b = it->second.barSold.begin(); b != it->second.barSold.end(); ++b) sold[b->first] += b->second;
            for (std::map<long long, long long>::const_iterator b = it->second.barLeft.begin(); b != it->second.barLeft.end(); ++b) left[b->first] = b->second;   // the latest zone wins
        }
        long long mx = 1;
        for (int i = v0; i <= v1; ++i) { const long long k = (long long)times[i]; if (sold.count(k)) mx = std::max(mx, sold[k]); if (left.count(k)) mx = std::max(mx, left[k]); }
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
int cppExtension::done(void)    { static_cast<ZoneDefense*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<ZoneDefense*>(this)->release(); return RTX_OK; }
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
    int barOf(RTARRAYI& times, int count, long long tMs) {              // the candle holding feed time t (bar END >= t)
        const long long t = tMs / 1000; int lo = 0, hi = count - 1;
        if (count <= 0) return -1;
        if ((long long)times[hi] < t) return hi;
        while (lo < hi) { const int m = (lo + hi) / 2; if ((long long)times[m] >= t) hi = m; else lo = m + 1; }
        return lo;
    }
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
    std::vector<zdh::JFile> keep;
    for (size_t i = 0; i < want.size(); ++i) { zdh::JFile f; for (size_t k = 0; k < s.files.size(); ++k) if (s.files[k].path == want[i]) f = s.files[k]; f.path = want[i]; f.poll(); keep.push_back(f); }
    s.files.swap(keep);
}

int ZoneDefenseMarks::draw(void) {
    LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    ZmState* sp = slot_.get(this, true); if (!sp) return RTX_OK; ZmState& s = *sp;
    char rb[64]; std::memset(rb, 0, sizeof rb); const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    s.mkt = zdh::marketOf(rs && *rs ? rs : (sy ? sy : "")); s.spb = getSecondsPerBar(); if (s.mkt.empty() || s.spb <= 0) return RTX_OK;
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

void ZoneDefenseMarks::zoneDraw(ZmState& s, const zdr::ZoneView& v, double tick, const ss::Values& V, int count, RTARRAYI& times, int, int) {
    const bool bid = v.side == 0; const COLOR base = bid ? zdh::C_GREEN : zdh::C_RED;
    const int a = barOf(times, count, v.T), b = v.ended ? barOf(times, count, v.endT) : count - 1;
    // brightness: resting now / peak (the latest decided number)
    long long now = v.size; if (v.touched) now = v.atTouch; if (v.reloaded) now = v.left; if (!v.barLeft.empty()) now = v.barLeft.rbegin()->second; if (v.ended) now = v.endResting;
    const double fr = v.size > 0 ? std::min(1.0, (double)now / v.size) : 0;
    const COLOR fill = zdh::scaleCol(base, v.ended ? 0.22 : 0.25 + 0.45 * fr);
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
            const std::string t = zdr::zoneLabel(v, tick); const int tw = getTextWidth(t.c_str(), -1);
            const int x = r.left + 2, y = bid ? r.bottom + 1 : r.top - 12;       // bids: under the band, offers: over it
            ZmState::Box bx = { x, y, x + tw, y + 11 }; bool hit = false;
            for (size_t i = 0; i < s.labels.size(); ++i) { const ZmState::Box& o = s.labels[i]; if (bx.l < o.r && bx.r > o.l && bx.t < o.b && bx.b > o.t) { hit = true; break; } }
            if (!hit) { setTextColor(base); RCT q; q.set(zdh::coord(x), zdh::coord(y), zdh::coord(x + tw + 2), zdh::coord(y + 11)); q.drawText(t.c_str(), false, false); s.labels.push_back(bx); }
            else ++s.labelsSkipped;                                           // never overlap: the older label gives way
        }
    }
    // the gray refill DIAMOND on the touch candle, at the zone's edge
    if (V.b(zdset::K_SHOW_DIAMOND) && v.reloaded) {
        const int tb = barOf(times, count, v.touchT); PNT p; p.set(tb, (float)((bid ? v.hi : v.lo) * tick), kBarCenter);
        setPen(zdh::C_GRAY, 1, P_SOLID);
        for (int r = 0; r <= 4; ++r) { const int w = 4 - r; PNT x0, x1; x0.h = (short)(p.h - w); x1.h = (short)(p.h + w + 1); x0.v = x1.v = (short)(p.v - r); x0.setDrawPosition(); x1.drawLineTo(); x0.v = x1.v = (short)(p.v + r); x0.setDrawPosition(); x1.drawLineTo(); }
    }
    // the SWEEP triangle under the wick of the first stop run (offers: over it); green up = swept, red down = weak
    if (V.b(zdset::K_SHOW_SWEEP) && v.swept && v.runT > 0) {
        const int rbar = barOf(times, count, v.runT); RTARRAY hiA(barHigh), loA(barLow); if (hiA.count <= rbar || loA.count <= rbar) return;
        PNT c; c.set(rbar, bid ? (float)loA[rbar] : (float)hiA[rbar], kBarCenter);
        struct { int top, bottom; } w = { c.v, c.v };
        const bool good = v.sweep == 2; const COLOR col = (good == bid) ? zdh::C_GREEN : zdh::C_RED; const bool up = good == bid;
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
    p->setDescription("LRA Zone Defense marks (price chart) - the zones lsZoneDefense decided from the order feed: the band (green bids under price / red offers over it, brighter = more still resting; it ends when broken, faded below half or 60 min untouched), one label 'Zone lo-hi · lots (Nx) · oldest Nm · held/broken', one big-order line, the gray refill diamond on the touch candle and the sweep triangle on the first stop run (green up = swept: the run was lifted 1.0x+ within 5 s and price came back into the zone; red down = weak). Needs lsZoneDefense on the same chart (it writes the journal this reads). Never repainted.");
    zdh::stamp(p);   // ZD_VERSION
    return p;
}
#endif
