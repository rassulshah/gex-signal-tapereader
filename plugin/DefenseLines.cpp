/********************************************************************************
 *  DefenseLines.cpp  --  Investor/RT RTX extension  lsDefenseLines  1.0.0  (2026-10-20)
 *  Rassul 11:37-12:22 CT (defense-refill-level-pane-mockup.png; 12:22 "first build the lines, do it now"): the PRICE CHART half of the
 *  Defense pane's pivot turns. lsBookFlow (Defense) decides each turn at the close and writes it to
 *  lsFlexLevels\BookFlow\<MKT>-<spb>-turns.csv (+ the forming candle's "?" in <MKT>-<spb>-turns-live.txt); this overlay only READS and draws:
 *   - a small triangle at the turn candle (above the high at a top, below the low at a bottom) when "Turn marks" is on;
 *   - the REFILL LINE: at the price near the pivot where the defenders re-posted the most lots after a hit. It starts at the turn candle and
 *     runs at most "Line minutes" (15), or until the next line starts, or until a candle CLOSES through it (then it stops there, dimmed,
 *     with "x"). Label: R (where the refill was) + E (the attack died at the extreme: pace after it <= half the pace before) + A
 *     (absorption: the attackers out-hit the defenders and still lost the close) + "?" (unproven - TURN-TEST.md 2026-10-20).
 *   - the forming candle's "?": a dotted line, gone if the candle does not confirm (forming candle only; confirmed lines never change).
 *  Green at lows, red at highs (colours in the settings window, per market, shared with the Defense pane's marks). Logic: DefenseTurn.h.
 *  Nothing heavy: a file stat at most every 2 s from the draw; drawing touches only the visible bars.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#include "HostSlot.h"
#include "DefenseTurn.h"
#include "DefenseLinesSettings.h"
#include "SettingsHost.h"
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
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

static const char* DL_VERSION = "1.0.1";   // 1.0.1 (2026-10-20 13:37 CT Rassul "remove the exhaustion indicators for now"): no E on the labels
static long long dlSteadyMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string dlFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string(); }
static void dlMakeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0700);
#endif
}
static bool dlStat(const std::string& p, long long& mt, long long& sz) {
    struct stat st; if (p.empty() || stat(p.c_str(), &st) != 0) return false;
    mt = (long long)st.st_mtime; sz = (long long)st.st_size; return true;
}
static long long dlCivil(int y, int mo, int d, int h, int mi, int s) {
    y -= mo <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (unsigned)(mo + (mo > 2 ? -3 : 9)) + 2) / 5 + (unsigned)d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + (long long)doe - 719468) * 86400 + h * 3600 + mi * 60 + s;
}

struct DlState {
    std::string mkt; int spb = 0;
    struct File { std::string path; long long mt = -1, sz = -1; };
    File turns, live;
    std::vector<dt::Line> lines;                  // decided turns, sorted by bar key
    bool haveLive = false; dt::Line liveLine;
    long long lastCheckMs = -100000, reads = 0, bad = 0;
    ss::Store setStore; ss::Values set = ss::defaults(dlset::spec(), -1);
    std::string statusBody;
};
static ss::Ids& dlIds() { static ss::Ids I; return I; }

class DefenseLines : public cppExtension {
public:
    HostSlot<DlState> slot_;
    virtual int draw(void);
    virtual int parmsLoad(void) {
        if (!ss::inDialog(this)) return RTX_OK;
        ss::Store* st = dialogStore(); if (!st) return RTX_OK;
        ss::fillDialog(this, dlset::spec(), dlIds(), st->load(ss::marketIndex(dlgMkt_), dlSteadyMs(), true), snap_);
        return RTX_OK;
    }
    virtual int parmsApply(void) { saveSettings(); return RTX_OK; }
    virtual int parmsUpdt(unsigned int i) { ss::onUpdate(this, dlset::spec(), dlIds(), (int)i, snap_); saveSettings(); return RTX_OK; }
    DlState* stateFor() { return slot_.get(this, false); }   // tests
    std::string chartMarket() { char b[64]; std::memset(b, 0, sizeof(b)); const char* rs = getRootSymbol(b); const int m = ss::marketOfRoot(rs ? rs : ""); if (m < 0) { const char* sy = getSymbol(); const int m2 = ss::marketOfRoot(sy ? sy : ""); return m2 < 0 ? std::string() : ss::MARKETS[m2]; } return ss::MARKETS[m]; }
    long long localSec(RTDATE d) { struct tm t; std::memset(&t, 0, sizeof(t)); if (!getLocaltime(d, &t)) return 0; return dlCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec); }
    void refresh(DlState& s);
    void writeStatus(DlState& s, size_t drawn);
    void tri(int cx, int tipY, bool down, COLOR col) {
        setPen(col, 1, P_SOLID);
        for (int r = 0; r < 5; ++r) { const int y = down ? tipY - r : tipY + r; PNT a; a.set(0, 0.0f); a.h = (short)(cx - r); a.v = (short)y; a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = (short)(cx + r + 1); b.v = (short)y; b.drawLineTo(); }
    }
    void label(int x, int yMid, const std::string& t, COLOR col) {
        FONT f; f.id = HELVETICA; f.size = 8; f.style = BOLD; setFont(f); setTextColor(col);
        const int w = (int)getTextWidth(t.c_str(), -1); RCT rc; rc.set((short)x, (short)(yMid - 6), (short)(x + w + 2), (short)(yMid + 6)); rc.drawText(t.c_str(), false, false);
    }
private:
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        const std::string mkt = chartMarket(); const std::string p = dlset::settingsPath(dlFlex(), mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &dlset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, dlset::spec(), dlIds(), st->load(ss::marketIndex(dlgMkt_), dlSteadyMs(), true), snap_);
        dlMakeDir(dlFlex());
        if (st->save(V)) snap_.take(ss::dialogNow(this, dlset::spec(), dlIds(), V));
    }
};

// re-read the turns file / the live file only when their size or time changed (at most every 2 s); a half-written line is not a record
void DefenseLines::refresh(DlState& s) {
    const long long now = dlSteadyMs();
    if (now - s.lastCheckMs < 2000 && now >= s.lastCheckMs) return;
    s.lastCheckMs = now;
    const std::string f = dlFlex();
    const std::string tp = dlset::turnsPath(f, s.mkt, s.spb), lp = dlset::livePath(f, s.mkt, s.spb);
    const std::string sp = dlset::settingsPath(f, s.mkt);
    if (s.setStore.path != sp) { s.setStore = ss::Store(); s.setStore.S = &dlset::spec(); s.setStore.path = sp; }
    s.set = sp.empty() ? ss::defaults(dlset::spec(), -1) : s.setStore.load(ss::marketIndex(s.mkt), now);
    long long mt = -1, sz = -1;
    if (tp != s.turns.path) { s.turns = DlState::File(); s.turns.path = tp; s.lines.clear(); }
    if (dlStat(tp, mt, sz) && (mt != s.turns.mt || sz != s.turns.sz) && sz < 4 * 1024 * 1024) {
        std::ifstream in(tp.c_str(), std::ios::binary); std::string ln; std::vector<dt::Line> v; long long bad = 0;
        while (std::getline(in, ln)) { dt::Line L; if (dt::parseLine(ln, L) && L.kind == 'T') v.push_back(L); else if (!ln.empty()) ++bad; }
        std::stable_sort(v.begin(), v.end(), [](const dt::Line& a, const dt::Line& b) { return a.t < b.t; });
        if (v.size() >= s.lines.size()) s.lines.swap(v);                     // append-only: a torn / shorter read never removes a drawn line
        s.turns.mt = mt; s.turns.sz = sz; s.reads++; s.bad = bad;
    }
    if (lp != s.live.path) { s.live = DlState::File(); s.live.path = lp; s.haveLive = false; }
    if (dlStat(lp, mt, sz) && (mt != s.live.mt || sz != s.live.sz) && sz < 4096) {
        std::ifstream in(lp.c_str(), std::ios::binary); std::string ln; std::getline(in, ln);
        dt::Line L; s.haveLive = dt::parseLine(ln, L) && L.kind == 'L'; if (s.haveLive) s.liveLine = L;
        s.live.mt = mt; s.live.sz = sz;
    }
}

void DefenseLines::writeStatus(DlState& s, size_t drawn) {
    const std::string d = dlFlex(); if (d.empty() || s.mkt.empty()) return;
    std::ostringstream o;
    o << "VERSION," << DL_VERSION << "\nMARKET," << s.mkt << "\nBAR_SECONDS," << s.spb << "\nTURNS_FILE," << s.turns.path << "," << s.lines.size() << " turns, "
      << s.bad << " unreadable lines\nLIVE," << (s.haveLive ? "? on the forming candle" : "none") << "\nDRAWN_NOW," << drawn
      << "\nSETTINGS," << s.set.why << ",lines " << (s.set.b(dlset::K_SHOW_LINES) ? "on" : "off") << ",marks " << (s.set.b(dlset::K_SHOW_MARKS) ? "on" : "off")
      << ",minutes " << s.set.i(dlset::K_MAX_MINUTES) << "\n";
    const std::string body = o.str(); if (body == s.statusBody) return;
    std::ofstream f((d + "\\DefenseLines.status-" + s.mkt + ".txt").c_str(), std::ios::binary | std::ios::trunc);
    if (f.is_open()) { f << body; s.statusBody = body; }
}

int DefenseLines::draw(void) {
    DlState* s = slot_.get(this, true);
    if (!s) return RTX_OK;
    try {
        s->mkt = chartMarket(); s->spb = (int)getSecondsPerBar();
        if (s->mkt.empty() || s->spb <= 0) return RTX_OK;
        refresh(*s);
        const ss::Values& V = s->set;
        const bool showLines = V.b(dlset::K_SHOW_LINES), showMarks = V.b(dlset::K_SHOW_MARKS);
        const int maxSec = std::max(3, V.i(dlset::K_MAX_MINUTES)) * 60;
        const long n = getBarCount(); if (n <= 2) { writeStatus(*s, 0); return RTX_OK; }
        int v0 = 0, v1 = (int)n - 1;
        if (getVisibleBars(&v0, &v1) != RTX_OK) { v0 = std::max(0, (int)n - 200); v1 = (int)n - 1; }
        if (v1 >= n) v1 = (int)n - 1;
        if (v0 < 0) v0 = 0;
        RTARRAYI dtA(barDateTime); RTARRAY hiA(barHigh), loA(barLow), clA(barClose);
        if (dtA.count < n || hiA.count < n || loA.count < n || clA.count < n) return RTX_OK;
        int ppb = getPixelsPerBar(); if (ppb < 1) ppb = 1;
        auto indexOf = [&](long long key) -> int {                          // bar keys ascend: binary search
            int a = 0, b = (int)n - 1;
            while (a <= b) { const int m = (a + b) / 2; const long long t = (long long)dtA[m]; if (t == key) return m; if (t < key) a = m + 1; else b = m - 1; }
            return -1;
        };
        const COLOR colLow = (COLOR)V.c(dlset::K_COL_LOW), colHigh = (COLOR)V.c(dlset::K_COL_HIGH);
        auto dim = [](COLOR c) { return (COLOR)(((c >> 1) & 0x7F7F7Ful) + 0x101010ul); };
        size_t drawn = 0;
        const int lookBack = std::max(1, maxSec / std::max(1, s->spb)) + 2;
        for (size_t k = 0; k < s->lines.size(); ++k) {
            const dt::Line& L = s->lines[k];
            const int i = indexOf(L.t); if (i < 0 || i > v1 || i < v0 - lookBack) continue;
            const COLOR col = L.side == dt::HIGH ? colHigh : colLow;
            if (showMarks && i >= v0) {
                const float px = L.side == dt::HIGH ? hiA[i] : loA[i];
                if (std::isfinite(px) && px > 0) { PNT p; p.set(i, px, kBarCenter); tri(p.h, L.side == dt::HIGH ? p.v - 4 : p.v + 4, L.side == dt::HIGH, col); }
            }
            if (!showLines) continue;
            // the line's life: the candles after the turn candle, the next line's start, the minutes cap
            const long long t0 = localSec((RTDATE)dtA[i]);
            long long nextT = 0;
            if (k + 1 < s->lines.size()) { const int ni = indexOf(s->lines[k + 1].t); if (ni > i) nextT = localSec((RTDATE)dtA[ni]); }
            std::vector<double> closes; std::vector<long long> times;
            for (int j = i + 1; j < n && (int)closes.size() < lookBack + 2; ++j) { closes.push_back((double)clA[j]); times.push_back(localSec((RTDATE)dtA[j])); }
            const dt::Life life = dt::lifeOf(L.side, L.linePx, t0, closes, times, nextT, maxSec);
            const int j = i + std::max(life.bars, 0);
            if (j < v0) continue;
            PNT a; a.set(i, (float)L.linePx, kBarCenter); PNT b; b.set(j, (float)L.linePx, kBarCenter);
            const int xa = a.h - ppb / 2, xb = life.bars > 0 ? b.h + ppb / 2 : a.h + ppb;
            const COLOR c = life.broken ? dim(col) : col;
            setPen(c, 2, P_SOLID);
            { PNT p0; p0.set(0, 0.0f); p0.h = (short)xa; p0.v = a.v; p0.setDrawPosition(); PNT p1; p1.set(0, 0.0f); p1.h = (short)xb; p1.v = a.v; p1.drawLineTo(); }
            label(xb + 3, a.v, (life.broken ? std::string("x ") : std::string()) + dt::tagsOf(L), c);
            ++drawn;
        }
        if (s->haveLive && showLines) {                                      // the forming candle's "?": dotted, 2 bars long
            const dt::Line& L = s->liveLine; const int i = indexOf(L.t);
            if (i >= 0 && i >= (int)n - 2 && i <= v1 + 1) {
                const COLOR col = L.side == dt::HIGH ? colHigh : colLow;
                PNT a; a.set(i, (float)L.linePx, kBarCenter);
                setPen(col, 1, P_DOT);
                { PNT p0; p0.set(0, 0.0f); p0.h = (short)(a.h - ppb / 2); p0.v = a.v; p0.setDrawPosition(); PNT p1; p1.set(0, 0.0f); p1.h = (short)(a.h + 2 * ppb); p1.v = a.v; p1.drawLineTo(); }
                label(a.h + 2 * ppb + 3, a.v, "? " + dt::tagsOf(L).substr(0, dt::tagsOf(L).size() - 2), col);
                ++drawn;
            }
        }
        writeStatus(*s, drawn);
    } catch (...) {}
    return RTX_OK;
}

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { return RTX_OK; }
int cppExtension::destroy(void) { static_cast<DefenseLines*>(this)->slot_.release(this); return RTX_OK; }
int cppExtension::setup(void) { ss::registerAll(this, dlset::spec(), dlIds()); return RTX_OK; }
int cppExtension::calc(int) { return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    DefenseLines* p = new DefenseLines();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA Defense Lines - the Defense pane's pivot turns on the price chart. A TURN = your pivot rule (the candle making the new high / low, or the next one trading back into it) + the defending side's Defense bar bigger (offers red at a high, bids green at a low) + the candle closing in the defending colour. Triangle = the turn candle. LINE = the price near the pivot where the defenders re-posted the most of what was eaten; it runs up to 15 minutes, until the next line, or until a candle closes through it (then dimmed with x). Label: R = refill location; A = absorption (attackers out-hit the defenders and still lost the close); ? = not proven yet (2026-10-20 test: no edge over a plain defending close; scored nightly). Dotted ? line = the forming candle, gone if it does not confirm. Example: 10:21 AM ES high, offers +23% bigger than bids, red close -> red line at 7853.25 'R ?'. Needs lsBookFlow (Defense) on the same chart.");
    p->setVersion("1.0.1");   // DL_VERSION
    return p;
}
