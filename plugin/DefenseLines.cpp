/********************************************************************************
 *  DefenseLines.cpp  --  Investor/RT RTX extension  lsDefenseLines  1.2.0  (2026-10-20)
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
#include "MoveLogic.h"   // (1.5.0) the M lines: order moves (Top Defender #3)
#include "DefenseLinesSettings.h"
#include "SettingsHost.h"
#include "SettingsGuard.h"   // (1.2.2) the window calls behind a fault guard (IRT faulted in parmsLoad / parmsApply of 1.1.0 on a copy with no chart)
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

static const char* DL_VERSION = "1.6.0";   // 1.6.0 (2026-10-21 14:16-14:18 CT Rassul "i want you to draw them", mockup v4): every move case drawn - per BOOK side the largest qualifying move (P: in, B: out), hollow box at the old price, solid box + lots at the new price, in the side's colour (bids green, offers red); both sides on one candle side by side, never overlapping;   // 1.5.0 (2026-10-21, Rassul approved defender3-mockup-v3 11:50 CT, Top Defender #3): on a candle where the Defense pane drew P (the defenders PRESSED IN), the largest such move on the price chart: a HOLLOW box at the old price, a SOLID box at the new price with only its lot size (M lines, MoveLogic.h); never repainted;   // 1.4.0 (2026-10-21, Top Defender #4): the turns file's Q lines (the re-test L / X, DefenseTurn 1.4.0) are read and kept out of the bad-line count; the line label stays "R A H ?" - L / X are in the Defense pane's badge row (Rassul 11:26 "if the refill line gets too complicated ... they are in the pane"; the line is usually gone by the re-test: at most 3 candles);   // 1.3.0 (2026-10-21, Rassul approved defender5-mockup-v2 11:25 CT): the line label adds H when lsBookFlow decided the defenders hit back after the A ("R A H ?"; DefenseTurn 1.3.0 H lines);    // 1.2.2 (2026-10-20 night): the settings window behind a fault guard (SettingsGuard.h) - IRT faulted inside parmsLoad / parmsApply of 1.1.0 on a window copy with no chart; 1.2.1 (2026-10-20 evening, Rassul "implement" the audit): stricter turns-file parsing, stale live-line fix, WATCH waits for the full 20-60 s window, touch freshness / stale-event guards, incomplete order totals marked partial, invalid effort / reward inputs rejected; // 1.2.0 (2026-10-20 14:19-14:23 CT Rassul): a turn needs the close beyond the R line; a line a candle closes through is removed (no dimmed x); 1.1.0 (2026-10-20 13:54 CT Rassul "okay build", defense-E-R-mockup.png): gray exhaustion line E, yellow dot on the candle each line refers to, lines at most 3 candles (or the next line), no price triangles;   // 1.0.1 (2026-10-20 13:37 CT Rassul "remove the exhaustion indicators for now"): no E on the labels
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
    std::map<std::pair<int, long long>, dt::XLine> xs; // (1.1.0) the exhaustion line of each turn (side, turn key)
    std::map<std::pair<int, long long>, bool> hs;      // (1.3.0) H decided per turn (side, turn key): true = the defenders hit back
    std::map<std::pair<int, long long>, mvl::Read> ms; // (1.5.0) the move read per (extreme side, candle): P draws the moved-order boxes
    long long boxesDrawn = 0;
    struct BoxRec { int side, l, r, ot, ob, nt, nb; };   // (1.6.0) the boxes of the last draw (tests: never overlapping)
    std::vector<BoxRec> lastBoxes;
    bool haveLive = false; dt::Line liveLine;
    long long lastCheckMs = -100000, reads = 0, bad = 0;
    ss::Store setStore; ss::Values set = ss::defaults(dlset::spec(), -1);
    std::string statusBody;
};
static ss::Ids& dlIds() { static ss::Ids I; return I; }

class DefenseLines : public cppExtension {
public:
    SS_SAFE_DIALOG(DefenseLines, "DefenseLines")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    HostSlot<DlState> slot_;
    virtual int draw(void);
    virtual int parmsLoadBody(void) { ss::guardedCall(this, &DefenseLines::loadBody, 0u); return RTX_OK; }
    virtual int parmsApplyBody(void) { ss::guardedCall(this, &DefenseLines::applyBody, 0u); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::guardedCall(this, &DefenseLines::updtBody, i); return RTX_OK; }
    void loadBody(unsigned) {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        ss::fillDialog(this, dlset::spec(), dlIds(), st->load(ss::marketIndex(dlgMkt_), dlSteadyMs(), true), snap_);
    }
    void applyBody(unsigned) { saveSettings(); }
    void updtBody(unsigned i) { ss::onUpdate(this, dlset::spec(), dlIds(), (int)i, snap_); saveSettings(); }
    DlState* stateFor() { return slot_.get(this, false); }   // tests
    std::string chartMarket() { return ss::safeMarket(this); }   // (1.2.2) a copy with no chart = "" (never faults)
    long long localSec(RTDATE d) { struct tm t; std::memset(&t, 0, sizeof(t)); if (!getLocaltime(d, &t)) return 0; return dlCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec); }
    void refresh(DlState& s);
    void writeStatus(DlState& s, size_t drawn, size_t failed = 0);
    void tri(int cx, int tipY, bool down, COLOR col) {
        setPen(col, 1, P_SOLID);
        for (int r = 0; r < 5; ++r) { const int y = down ? tipY - r : tipY + r; PNT a; a.set(0, 0.0f); a.h = (short)(cx - r); a.v = (short)y; a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = (short)(cx + r + 1); b.v = (short)y; b.drawLineTo(); }
    }
    void dot(int cx, int cy, COLOR col) {                                    // (1.1.0) the candle a line refers to: a small filled dot (7 px)
        setPen(col, 1, P_SOLID);
        static const int W[7] = { 1, 2, 3, 3, 3, 2, 1 };
        for (int r = -3; r <= 3; ++r) { const int w = W[r + 3]; PNT a; a.set(0, 0.0f); a.h = (short)(cx - w); a.v = (short)(cy + r); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = (short)(cx + w + 1); b.v = (short)(cy + r); b.drawLineTo(); }
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

// Re-read turns only when metadata changed; parse the volatile live file on every two-second poll. A half-written line is not a record.
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
    if (tp != s.turns.path) { s.turns = DlState::File(); s.turns.path = tp; s.lines.clear(); s.xs.clear(); }
    if (dlStat(tp, mt, sz) && (mt != s.turns.mt || sz != s.turns.sz) && sz < 4 * 1024 * 1024) {
        std::ifstream in(tp.c_str(), std::ios::binary); std::string ln; std::vector<dt::Line> v; long long bad = 0;
        std::map<std::pair<int, long long>, dt::XLine> xs; std::map<std::pair<int, long long>, bool> hs; std::map<std::pair<int, long long>, mvl::Read> ms;
        while (std::getline(in, ln)) { dt::Line L; dt::XLine X; dt::Counter HC; mvl::Read MR; if (dt::parseLine(ln, L) && L.kind == 'T') v.push_back(L); else if (dt::parseX(ln, X)) xs[std::make_pair(X.side, X.t)] = X; else if (dt::parseH(ln, HC)) hs[std::make_pair(HC.side, HC.t)] = HC.H; else if (ln.size() > 1 && ln[0] == 'Q' && ln[1] == '|') { dt::Retest QR; if (!dt::parseQ(ln, QR)) ++bad; } else if (ln.size() > 1 && ln[0] == 'M' && ln[1] == '|') { if (mvl::parseM(ln, MR)) ms[std::make_pair(MR.ext, MR.t)] = MR; else ++bad; } else if (ln.size() > 2 && ln[0] == 'M' && ln[1] == 'N') {} else if (!ln.empty()) ++bad; }
        std::stable_sort(v.begin(), v.end(), [](const dt::Line& a, const dt::Line& b) { return a.t < b.t; });
        if (v.size() >= s.lines.size()) s.lines.swap(v);                     // append-only: a torn / shorter read never removes a drawn line
        for (std::map<std::pair<int, long long>, dt::XLine>::const_iterator it = xs.begin(); it != xs.end(); ++it) s.xs[it->first] = it->second;
        for (std::map<std::pair<int, long long>, bool>::const_iterator it = hs.begin(); it != hs.end(); ++it) s.hs[it->first] = it->second;
        for (std::map<std::pair<int, long long>, mvl::Read>::const_iterator it = ms.begin(); it != ms.end(); ++it) s.ms[it->first] = it->second;   // (1.5.0) append-only: never removed
        s.turns.mt = mt; s.turns.sz = sz; s.reads++; s.bad = bad;
    }
    if (lp != s.live.path) { s.live = DlState::File(); s.live.path = lp; s.haveLive = false; s.liveLine = dt::Line(); }
    // This is a tiny, volatile file: re-read on each two-second poll. mtime is only second-granularity on some hosts, so a same-size
    // overwrite could otherwise leave a stale forming "?" on the chart. Missing / oversized / malformed live input fails closed.
    if (dlStat(lp, mt, sz) && sz < 4096) {
        std::ifstream in(lp.c_str(), std::ios::binary); std::string ln; std::getline(in, ln);
        dt::Line L; s.haveLive = dt::parseLine(ln, L) && L.kind == 'L'; if (s.haveLive) s.liveLine = L; else s.liveLine = dt::Line();
        s.live.mt = mt; s.live.sz = sz;
    } else { s.haveLive = false; s.liveLine = dt::Line(); s.live.mt = mt; s.live.sz = sz; }
}

void DefenseLines::writeStatus(DlState& s, size_t drawn, size_t failed) {
    const std::string d = dlFlex(); if (d.empty() || s.mkt.empty()) return;
    std::ostringstream o;
    o << "VERSION," << DL_VERSION << "\nMARKET," << s.mkt << "\nBAR_SECONDS," << s.spb << "\nTURNS_FILE," << s.turns.path << "," << s.lines.size() << " turns, "
      << s.bad << " unreadable lines\nLIVE," << (s.haveLive ? "? on the forming candle" : "none") << "\nDRAWN_NOW," << drawn << ",failed lines removed " << failed
      << "\nSETTINGS," << s.set.why << ",lines " << (s.set.b(dlset::K_SHOW_LINES) ? "on" : "off") << ",E " << (s.set.b(dlset::K_SHOW_E) ? "on" : "off") << ",dots " << (s.set.b(dlset::K_SHOW_DOTS) ? "on" : "off")
      << ",pane marks " << (s.set.b(dlset::K_SHOW_MARKS) ? "on" : "off") << ",candles " << s.set.i(dlset::K_MAX_CANDLES) << ",exhaustion lines " << s.xs.size() << "\n";
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
        const bool showLines = V.b(dlset::K_SHOW_LINES), showE = V.b(dlset::K_SHOW_E), showDots = V.b(dlset::K_SHOW_DOTS);
        const int maxBars = std::max(1, V.i(dlset::K_MAX_CANDLES));
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
        const COLOR colLow = (COLOR)V.c(dlset::K_COL_LOW), colHigh = (COLOR)V.c(dlset::K_COL_HIGH), colE = (COLOR)V.c(dlset::K_COL_E), colDot = (COLOR)V.c(dlset::K_COL_DOT);
        auto traded = [&](int k, double px) { return k >= 0 && k < n && (double)loA[k] <= px + 1e-9 && px <= (double)hiA[k] + 1e-9; };
        size_t drawn = 0, failed = 0;
        s->lastBoxes.clear();
        // one line: from the candle it refers to (yellow dot), at most maxBars candles, until the next turn's lines, or a close through it
        // (1.2.0, Rassul 14:20 CT "once a refill on any line fails, remove it to get rid of the clutter"): a line a candle CLOSES through is
        // removed (line, label, dot); a failed R takes its E with it. Lines that ended by time / the next line stay. The turns file keeps all.
        auto lifeAt = [&](int st, int side, double px, long long nextKey) {
            std::vector<double> closes; std::vector<long long> times;
            for (int j = st + 1; j < n && (int)closes.size() < maxBars + 1; ++j) { closes.push_back((double)clA[j]); times.push_back((long long)dtA[j]); }
            return dt::lifeBars(side, px, closes, times, nextKey, maxBars);
        };
        auto drawLine = [&](int st, int side, double px, long long nextKey, COLOR col, const std::string& tag, int labelDy) {
            if (st < 0 || st > v1 || st < v0 - maxBars) return;
            const dt::Life life = lifeAt(st, side, px, nextKey);
            if (life.broken) return;                                          // failed: removed
            const int j = st + life.bars;
            if (j < v0) return;
            PNT a; a.set(st, (float)px, kBarCenter); PNT b; b.set(j, (float)px, kBarCenter);
            const int xa = a.h - ppb / 2, xb = b.h + ppb / 2;
            setPen(col, 2, P_SOLID);
            { PNT p0; p0.set(0, 0.0f); p0.h = (short)xa; p0.v = a.v; p0.setDrawPosition(); PNT p1; p1.set(0, 0.0f); p1.h = (short)xb; p1.v = a.v; p1.drawLineTo(); }
            label(xb + 3, a.v + labelDy, tag, col);
            if (showDots && st >= v0) dot(a.h, a.v, colDot);
            ++drawn;
        };
        for (size_t k = 0; k < s->lines.size(); ++k) {
            const dt::Line& L = s->lines[k];
            const int i = indexOf(L.t); if (i < 0 || i > v1 || i < v0 - maxBars - 1) continue;
            const long long nextKey = k + 1 < s->lines.size() ? s->lines[k + 1].t : 0;
            const int piv = L.pivotT > 0 ? indexOf(L.pivotT) : -1;
            const int st = traded(piv, L.linePx) ? piv : (traded(i - 1, L.linePx) && !traded(i, L.linePx) ? i - 1 : i);
            std::map<std::pair<int, long long>, dt::XLine>::const_iterator xi = s->xs.find(std::make_pair(L.side, L.t));
            const bool haveX = xi != s->xs.end() && showE;
            const bool close = haveX && std::fabs(xi->second.px - L.linePx) < 1.5 * L.tick;   // within a tick: stack the labels
            if (st >= 0 && lifeAt(st, L.side, L.linePx, nextKey).broken) { ++failed; continue; }   // the R failed: R and its E removed
            if (showLines) drawLine(st, L.side, L.linePx, nextKey, L.side == dt::HIGH ? colHigh : colLow, dt::tagsOfH(L, s->hs.count(std::make_pair(L.side, L.t)) && s->hs[std::make_pair(L.side, L.t)]), close ? (L.side == dt::HIGH ? 6 : -6) : 0);
            if (haveX) {
                const int xs_ = indexOf(xi->second.at);
                const int sx = traded(xs_, xi->second.px) ? xs_ : (traded(piv, xi->second.px) ? piv : i);
                drawLine(sx, L.side, xi->second.px, nextKey, colE, "E ?", close ? (L.side == dt::HIGH ? -6 : 6) : 0);
            }
        }
        // (1.5.0, Top Defender #3) the defenders PRESSED IN on a candle (the pane's P): the largest such move - hollow box at the old
        // price, solid box at the new price with only its lot size; in the defending colour; inside the visible candles only
        // (1.6.0) every qualified case: per BOOK side the largest move in the direction it qualified; bids green (LOW colour), offers red
        // (HIGH colour); two sides on one candle: bids left of the candle centre, offers right, so the four boxes never overlap
        for (std::map<std::pair<int, long long>, mvl::Read>::const_iterator it = s->ms.begin(); it != s->ms.end(); ++it) {
            const mvl::Read& R = it->second; if (!mvl::anyDrawn(R)) continue;
            const int i = indexOf(R.t); if (i < v0 || i > v1) continue;
            const double tk = R.tick;                                         // the M line carries its tick size
            if (!(tk > 0)) continue;
            long long o[2] = {0, 0}, nw[2] = {0, 0}, sz[2] = {0, 0}; bool has[2] = {false, false};
            for (int sd = 0; sd < 2; ++sd) has[sd] = mvl::chartBoxOf(R, sd, o[sd], nw[sd], sz[sd]);
            const bool both = has[0] && has[1];
            FONT f; f.id = HELVETICA; f.size = 7; f.style = BOLD; setFont(f);
            for (int d = 0; d < 2; ++d) {
                if (!has[d]) continue;
                const COLOR col = d == mvl::OFFERS ? colHigh : colLow;
                const std::string txt = std::to_string(sz[d]);
                const int tw = (int)getTextWidth(txt.c_str(), -1) + 6, w = std::max(ppb, tw), hh = 10;
                PNT po; po.set(i, (float)((double)o[d] * tk), kBarCenter); PNT pn; pn.set(i, (float)((double)nw[d] * tk), kBarCenter);
                const int l = both ? (d == mvl::BIDS ? po.h - w - 1 : po.h + 1) : po.h - w / 2, r = l + w;
                if (std::abs(po.v - pn.v) < hh) po.v = (short)(pn.v + ((po.v > pn.v || (po.v == pn.v && d == mvl::BIDS)) ? hh : -hh));   // a 1-tick move on a tight scale: the hollow box sits against the solid one on the old price's side (never on top of it)
                setPen(col, 1, P_SOLID);
                auto hl = [&](int x0, int x1, int y) { PNT a; a.set(0, 0.0f); a.h = (short)x0; a.v = (short)y; a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = (short)x1; b.v = (short)y; b.drawLineTo(); };
                auto vl = [&](int x, int y0, int y1) { PNT a; a.set(0, 0.0f); a.h = (short)x; a.v = (short)y0; a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = (short)x; b.v = (short)y1; b.drawLineTo(); };
                { const int t = po.v - hh / 2, b = t + hh; hl(l, r, t); hl(l, r, b); vl(l, t, b); vl(r, t, b); }      // the old price: hollow
                { const int t = pn.v - hh / 2, b = t + hh; for (int y = t; y <= b; ++y) hl(l, r, y);                     // the new price: solid + lots
                  setTextColor((COLOR)0x00000000); RCT rc; rc.set((short)(l + 2), (short)t, (short)(r - 1), (short)(b + 1)); rc.drawText(txt.c_str(), false, false); }
                s->lastBoxes.push_back(DlState::BoxRec{ d, l, r, po.v - hh / 2, po.v + hh / 2, pn.v - hh / 2, pn.v + hh / 2 });
                ++s->boxesDrawn; ++drawn;
            }
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
        writeStatus(*s, drawn, failed);
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
    p->setDescription("LRA Defense Lines - the Defense pane's pivot turns on the price chart. A TURN = your pivot rule (the candle making the new high / low, or the next one trading back into it) + the defending side's Defense bar bigger (offers red at a high, bids green at a low) + the candle closing in the defending colour AND beyond the R line (above it at a low, below it at a high). R LINE (red at a high, green at a low) = the price near the pivot where the defenders re-posted the most of what was eaten. E LINE (gray) = exhaustion: the price near the pivot where the attack died AT the price (the attackers' hits there in seconds 20-60 slowed to the market's median or less vs seconds 0-20, and at least half the resting size stayed); decided about a minute after the close; only on turns that have an R. Yellow dot = the candle each line refers to. Each line runs at most 3 candles (setting), until the next turn's lines, or until a candle closes through it (then it is removed). A = absorption (attackers out-hit the defenders and still lost the close); ? = not proven yet (scored nightly). Dotted ? line = the forming candle, gone if it does not confirm. Example: 8:48 AM ES low: green R at 7827.25, gray E at 7826.75, dots on the 8:48 candle. Needs lsBookFlow (Defense) on the same chart.");
    p->setVersion("1.6.0");   // DL_VERSION
    return p;
}
