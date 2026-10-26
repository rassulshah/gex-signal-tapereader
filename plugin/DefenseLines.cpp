/********************************************************************************
 *  (1.13.0, 2026-10-26, Rassul 17:55 CT "get rid of the exhaustion lines - the gray ones") the gray EXHAUSTION (E) line is NOT DRAWN
 *  and its settings rows are gone (parameter version 5). lsBookFlow still decides and writes each turn's X line (turns file) for the
 *  nightly; this overlay still reads and counts them. (17:59 CT "remove the pressed in boxes") the P / B MOVE boxes (hollow at the old
 *  price, solid + lots at the new price) are not drawn either; the M lines are still read. R lines, dots and trails unchanged.
 *  DefenseLines.cpp  --  Investor/RT RTX extension  lsDefenseLines  1.10.0  (2026-10-22)
 *  (1.10.0, Rassul ordered 15:20 CT 2026-10-22 "ROLLING BIDS / OFFERS DURING THE MOVE", MBO Roadmap "Is a big order following price during
 *  the move?" T-20261022-17) TRAILS: one big resting order (one order id, >= the market's Big Lot S20 of resting orders) re-priced again and
 *  again BEHIND price during the wave - a bid stepping up under a rally, an offer stepping down above a selloff. lsBookFlow's order feed
 *  writes its steps (MoveLogic 1.2.0) to lsFlexLevels\BookFlow\<MKT>-<spb>-trails.csv; this overlay draws a thin STEPPED line (green bid,
 *  red offer) joining its prices from the first step to now, ending ONCE: a filled dot (it traded), x (pulled), "B?" (it STOPPED FOLLOWING:
 *  price ran >= 0.5 ATR on and it did not follow within N candles, or it stepped back - support / resistance quitting, the stage 3
 *  exhaustion warning). At most 2 trails per side. Never repainted: each end is decided once at a candle close and recorded to
 *  <MKT>-<spb>-trails-rec.csv (the nightly scores whether B? precedes the turn); a restart redraws from that record. Hover a trail: size,
 *  steps, ticks, how long it followed, the end, question / what / why / use / example. Settings: Trails checkbox, Bid / Offer colours,
 *  Stop-following gap (ATR), Candles to follow (N).
 *  (1.9.0, Rassul 19:48 CT "for the exhaustion line remove the E and the ? and the refill line remove any characters like R A etc. keep them
 *  both clean") the lines carry NO text: no "R A H ?", no "E ?", no "? R" on the forming line - the letters live in the Defense pane's
 *  badge row (hover there for the numbers). Lines, colours, dots, boxes and the records are unchanged.
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
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "DefenseLines"
#endif
#include "HostSlot.h"
#include "DefenseTurn.h"
#include "KeepOutDraw.h"   // (1.8.0) BoxDock.h 1.2.0 keep-out: nothing painted over the HOD/LOD table
#include "MoveLogic.h"   // (1.5.0) the M lines: order moves (Top Defender #3)
#include "DefenseLinesSettings.h"
#include "SettingsHost.h"
#include "HoverBoard.h"     // (1.10.0) hbCursor: the trail hover only while the cursor is over this chart
#include "MarkParams.h"     // (1.11.0) LRA-Trails-<MKT>.csv PARAM,SHOW / SHOW_B (the nightly's verdict; LOG = recorded, not drawn)
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
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")   // (1.10.0) GetCursorPos / WindowFromPoint via HoverBoard.h
#endif

static const bool kDrawMoveBoxes = false;   // (1.13.0, Rassul 2026-10-26 17:59 CT "remove the pressed in boxes") the P / B move boxes are not drawn; M lines kept
static const char* DL_VERSION = "1.13.0";   // 1.13.0 (2026-10-26, Rassul 17:55 CT) the gray exhaustion (E) line and the P / B move boxes are not drawn, E rows removed (X / M lines still read / counted); 1.12.0 (2026-10-25, BUILD-SPEC "Defense Lines vs order lines at the same price", Rassul approved 22:29 CT): the line is THINNER (1 px, setting "Line width"), still no label; its yellow dot is NOT drawn on a candle where lsLiquidityProfile drew an order fill dot (lsFlexLevels\LRA-OrderFills-<MKT>-<spb>.txt, LP 0.55.1; setting "No dot where an order fill dot is"); the forming candle's line is solid 1 px (no dotted / dashed lines); the order line goes on top by chart order (lsLiquidityProfile listed after lsDefenseLines); 1.11.0 (2026-10-22 evening, VALIDATE-CONTEXT-MARKS TR-2, Rassul approved 17:39 CT): the trails and B? are LOG ONLY by default in every market (B? = coin flip vs a matched-extension control): drawn only when lsFlexLevels\LRA-Trails-<MKT>.csv says PARAM,SHOW (lines, dots, x) / PARAM,SHOW_B (B?); every end still decided and recorded (trails-rec.csv); 1.10.0 (2026-10-22 15:20 CT Rassul, T-20261022-17): TRAILS of big orders following price (MoveLogic 1.2.0), decided / recorded once, hover; TRACK_MOUSE for the trail hover only; 1.9.0 (2026-10-21 19:48 CT Rassul "keep them both clean"): no text on the refill / exhaustion / forming lines (the letters are in the Defense pane's badge + its hover); 1.8.0 (2026-10-21 16:05 CT, Rassul "fix this, it should be solid"): lines stop at the lsHodLod table's edges, labels / dots / boxes that would touch it are not painted (still in the turns file and counted; KeepOutDraw.h); 1.7.0 (2026-10-21, lsBookFlow 1.19.0): the turns file's new G (attackers giving up) and W (defenders withdrew at a key level) lines are read and kept out of the bad-line count; nothing new drawn on the price chart (the letters are in the Defense pane's badge);   // 1.6.0 (2026-10-21 14:16-14:18 CT Rassul "i want you to draw them", mockup v4): every move case drawn - per BOOK side the largest qualifying move (P: in, B: out), hollow box at the old price, solid box + lots at the new price, in the side's colour (bids green, offers red); both sides on one candle side by side, never overlapping;   // 1.5.0 (2026-10-21, Rassul approved defender3-mockup-v3 11:50 CT, Top Defender #3): on a candle where the Defense pane drew P (the defenders PRESSED IN), the largest such move on the price chart: a HOLLOW box at the old price, a SOLID box at the new price with only its lot size (M lines, MoveLogic.h); never repainted;   // 1.4.0 (2026-10-21, Top Defender #4): the turns file's Q lines (the re-test L / X, DefenseTurn 1.4.0) are read and kept out of the bad-line count; the line label stays "R A H ?" - L / X are in the Defense pane's badge row (Rassul 11:26 "if the refill line gets too complicated ... they are in the pane"; the line is usually gone by the re-test: at most 3 candles);   // 1.3.0 (2026-10-21, Rassul approved defender5-mockup-v2 11:25 CT): the line label adds H when lsBookFlow decided the defenders hit back after the A ("R A H ?"; DefenseTurn 1.3.0 H lines);    // 1.2.2 (2026-10-20 night): the settings window behind a fault guard (SettingsGuard.h) - IRT faulted inside parmsLoad / parmsApply of 1.1.0 on a window copy with no chart; 1.2.1 (2026-10-20 evening, Rassul "implement" the audit): stricter turns-file parsing, stale live-line fix, WATCH waits for the full 20-60 s window, touch freshness / stale-event guards, incomplete order totals marked partial, invalid effort / reward inputs rejected; // 1.2.0 (2026-10-20 14:19-14:23 CT Rassul): a turn needs the close beyond the R line; a line a candle closes through is removed (no dimmed x); 1.1.0 (2026-10-20 13:54 CT Rassul "okay build", defense-E-R-mockup.png): gray exhaustion line E, yellow dot on the candle each line refers to, lines at most 3 candles (or the next line), no price triangles;   // 1.0.1 (2026-10-20 13:37 CT Rassul "remove the exhaustion indicators for now"): no E on the labels
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
    bdk::TableWatch keep;                         // (1.8.0) the HOD/LOD table on this chart (keep-out)
    // (1.10.0) TRAILS: the trail file (read incrementally: append-only), the record of decided ends, the hover
    mvl::TrailSet trails; std::string trailPath, recPath; long long trailOff = 0, trailSz = -1, trailMt = -1;
    std::map<std::pair<long long, long long>, mvl::TrEnd> recEnds; std::set<std::pair<long long, long long> > recDone;
    long long lastN = -1, nChangedMs = 0, recWritten = 0, recFails = 0, trailsDrawn = 0;
    struct TrHit { size_t idx; int l, t, r, b; };
    int trShowP = mp::LOG, trShowB = mp::LOG;    // (1.11.0) LRA-Trails-<MKT>.csv SHOW / SHOW_B
    std::vector<TrHit> trHits;                    // the drawn trails' segments (+ 3 px), for the hover
    bool msHas = false; int msH = 0, msV = 0; unsigned long long msWnd = 0; std::string hovKey;
    File fills; std::set<long long> fillKeys; long long dotsSkipped = 0;   // (1.12.0) LP's fill-dot candles (bar datetime keys)
};
static ss::Ids& dlIds() { static ss::Ids I; return I; }

class DefenseLines : public cppExtension {
public:
    SS_SAFE_DIALOG(DefenseLines, "DefenseLines")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    HostSlot<DlState> slot_;
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);              // (1.10.0) the trail hover
    void drawTrails(DlState& s, const ss::Values& V, int n, int v0, int v1, RTARRAYI& dtA, RTARRAY& hiA, RTARRAY& loA, RTARRAY& clA, int ppb, size_t& drawn);
    void refreshTrails(DlState& s);
    std::string clockOf(long long key) { const long long t = localSec((RTDATE)key) - (long long)getSecondsPerBar(); if (t <= 0) return "?"; const int h = (int)(t / 3600 % 24), m = (int)(t / 60 % 60); char b[16]; std::snprintf(b, sizeof b, "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, h < 12 ? "AM" : "PM"); return b; }
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
        if (bdk::hide(cx - 5, down ? tipY - 4 : tipY, cx + 6, down ? tipY : tipY + 4)) return;   // (1.8.0) a mark under the HOD/LOD table: not painted
        for (int r = 0; r < 5; ++r) { const int y = down ? tipY - r : tipY + r; ko::rawLine(cx - r, y, cx + r + 1, y); }
    }
    void dot(int cx, int cy, COLOR col) {                                    // (1.1.0) the candle a line refers to: a small filled dot (7 px)
        setPen(col, 1, P_SOLID);
        static const int W[7] = { 1, 2, 3, 3, 3, 2, 1 };
        if (bdk::hide(cx - 3, cy - 3, cx + 4, cy + 3)) return;   // (1.8.0)
        for (int r = -3; r <= 3; ++r) { const int w = W[r + 3]; PNT a; a.set(0, 0.0f); a.h = (short)(cx - w); a.v = (short)(cy + r); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = (short)(cx + w + 1); b.v = (short)(cy + r); b.drawLineTo(); }
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
    if (tp != s.turns.path) { s.turns = DlState::File(); s.turns.path = tp; s.lines.clear(); s.xs.clear(); s.hs.clear(); s.ms.clear(); }   // (1.9.0) H / M too: a new market / bar size never draws the old file's move boxes
    if (dlStat(tp, mt, sz) && (mt != s.turns.mt || sz != s.turns.sz) && sz < 4 * 1024 * 1024) {
        std::ifstream in(tp.c_str(), std::ios::binary); std::string ln; std::vector<dt::Line> v; long long bad = 0;
        std::map<std::pair<int, long long>, dt::XLine> xs; std::map<std::pair<int, long long>, bool> hs; std::map<std::pair<int, long long>, mvl::Read> ms;
        while (std::getline(in, ln)) { dt::Line L; dt::XLine X; dt::Counter HC; mvl::Read MR; if (dt::parseLine(ln, L) && L.kind == 'T') v.push_back(L); else if (dt::parseX(ln, X)) xs[std::make_pair(X.side, X.t)] = X; else if (dt::parseH(ln, HC)) hs[std::make_pair(HC.side, HC.t)] = HC.H; else if (ln.size() > 1 && ln[0] == 'Q' && ln[1] == '|') { dt::Retest QR; if (!dt::parseQ(ln, QR)) ++bad; } else if (ln.size() > 1 && ln[0] == 'M' && ln[1] == '|') { if (mvl::parseM(ln, MR)) ms[std::make_pair(MR.ext, MR.t)] = MR; else ++bad; } else if (ln.size() > 2 && ln[0] == 'M' && ln[1] == 'N') {} else if (ln.size() > 1 && ln[0] == 'G' && ln[1] == '|') { dt::GiveUp GU; if (!dt::parseG(ln, GU)) ++bad; } else if (ln.size() > 1 && ln[0] == 'W' && ln[1] == '|') { dt::WRead WR; if (!dt::parseW(ln, WR, s.mkt)) ++bad; } else if (!ln.empty()) ++bad; }   // (1.7.0) G / W lines: the Defense pane's badge only (not drawn here), kept out of the bad count
        std::stable_sort(v.begin(), v.end(), [](const dt::Line& a, const dt::Line& b) { return a.t < b.t; });
        if (v.size() >= s.lines.size()) s.lines.swap(v);                     // append-only: a torn / shorter read never removes a drawn line
        for (std::map<std::pair<int, long long>, dt::XLine>::const_iterator it = xs.begin(); it != xs.end(); ++it) s.xs[it->first] = it->second;
        for (std::map<std::pair<int, long long>, bool>::const_iterator it = hs.begin(); it != hs.end(); ++it) s.hs[it->first] = it->second;
        for (std::map<std::pair<int, long long>, mvl::Read>::const_iterator it = ms.begin(); it != ms.end(); ++it) s.ms[it->first] = it->second;   // (1.5.0) append-only: never removed
        s.turns.mt = mt; s.turns.sz = sz; s.reads++; s.bad = bad;
    }
    refreshTrails(s);                                                     // (1.10.0)
    {   // (1.12.0) the candles where lsLiquidityProfile drew an order fill dot: our yellow dot is skipped there
        const std::string fp = f.empty() || s.mkt.empty() ? std::string() : f + "\\LRA-OrderFills-" + s.mkt + "-" + std::to_string(s.spb) + ".txt";
        if (fp != s.fills.path) { s.fills = DlState::File(); s.fills.path = fp; s.fillKeys.clear(); }
        long long fmt = -1, fsz = -1;
        if (!dlStat(fp, fmt, fsz)) s.fillKeys.clear();                    // no LP order lines on this market: every dot drawn
        else if ((fmt != s.fills.mt || fsz != s.fills.sz) && fsz < 1024 * 1024) {
            std::ifstream in(fp.c_str(), std::ios::binary); std::string ln; std::set<long long> k;
            while (std::getline(in, ln)) { if (ln.size() > 2 && ln[0] == 'F' && ln[1] == ',') { char* e = nullptr; const long long v = std::strtoll(ln.c_str() + 2, &e, 10); if (e && e != ln.c_str() + 2 && v > 0) k.insert(v); } }
            s.fillKeys.swap(k); s.fills.mt = fmt; s.fills.sz = fsz;
        }
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
      << s.bad << " unreadable lines\nLIVE," << (s.haveLive ? "solid 1 px line on the forming candle" : "none") << "\nDRAWN_NOW," << drawn << ",failed lines removed " << failed << "\nFILL_DOTS," << s.fills.path << "," << s.fillKeys.size() << " order-fill candles,yellow dots skipped " << s.dotsSkipped
      << "\nSETTINGS," << s.set.why << ",lines " << (s.set.b(dlset::K_SHOW_LINES) ? "on" : "off") << ",E not drawn (1.13.0)" << ",dots " << (s.set.b(dlset::K_SHOW_DOTS) ? "on" : "off")
      << ",pane marks " << (s.set.b(dlset::K_SHOW_MARKS) ? "on" : "off") << ",candles " << s.set.i(dlset::K_MAX_CANDLES) << ",exhaustion lines " << s.xs.size() << "\nLABELS,none (1.9.0: clean lines; the letters are in the Defense pane's badge)\n"
      << "TRAILS," << (s.set.b(dlset::K_SHOW_TRAILS) ? (s.trShowP == mp::LOG ? "log only" : "on") : "off") << "," << s.trails.v.size() << " trails (" << s.trails.bad << " unreadable lines, " << s.trails.restarts << " feed starts),drawn " << s.trailsDrawn
      << ",recorded " << s.recDone.size() << " (" << s.recWritten << " this run, " << s.recFails << " write fails),gap " << s.set.f(dlset::K_TRAIL_GAP) << " ATR,N " << s.set.i(dlset::K_TRAIL_N) << "\n" << bdk::keepLine();
    const std::string body = o.str(); if (body == s.statusBody) return;
    std::ofstream f((d + "\\DefenseLines.status-" + s.mkt + ".txt").c_str(), std::ios::binary | std::ios::trunc);
    if (f.is_open()) { f << body; s.statusBody = body; }
}

// (1.10.0) the trail file: append-only, read from where the last read stopped (whole lines only); a shorter file = rotated: read again.
// The record of decided ends is read once per market / bar size: a recorded end always wins (never repainted).
void DefenseLines::refreshTrails(DlState& s) {
    const std::string f = dlFlex();
    const std::string tp = mvl::trailPathOf(f, s.mkt, s.spb), rp = mvl::trailRecPathOf(f, s.mkt, s.spb);
    if (rp != s.recPath) {
        s.recPath = rp; s.recEnds.clear(); s.recDone.clear(); s.recWritten = 0; s.recFails = 0;
        long long mt = -1, sz = -1;
        if (dlStat(rp, mt, sz) && sz < 8 * 1024 * 1024) {
            std::ifstream in(rp.c_str(), std::ios::binary); std::string ln;
            while (std::getline(in, ln)) { long long id = 0, k0 = 0; mvl::TrEnd E; if (mvl::parseTR(ln, id, k0, E)) { s.recEnds[std::make_pair(id, k0)] = E; s.recDone.insert(std::make_pair(id, k0)); } }
        }
    }
    if (tp != s.trailPath) { s.trailPath = tp; s.trails.clear(); s.trailOff = 0; s.trailSz = -1; s.trailMt = -1; }
    long long mt = -1, sz = -1;
    if (!dlStat(tp, mt, sz)) return;
    if (sz == s.trailSz && mt == s.trailMt) return;
    if (sz < s.trailOff) { s.trails.clear(); s.trailOff = 0; }              // rotated (MoveLogic keeps it under 3 MB): read the new file
    if (sz - s.trailOff > 8 * 1024 * 1024) return;                          // never a huge read from the draw
    std::ifstream in(tp.c_str(), std::ios::binary); if (!in.is_open()) return;
    in.seekg((std::streamoff)s.trailOff);
    std::string buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const size_t lastNl = buf.rfind('\n'); if (lastNl == std::string::npos) { s.trailSz = -1; return; }   // only a half-written line: try again
    size_t a = 0;
    while (a <= lastNl) { const size_t b = buf.find('\n', a); s.trails.line(buf.substr(a, b - a)); a = b + 1; }
    s.trailOff += (long long)lastNl + 1; s.trailSz = sz; s.trailMt = mt;
    for (size_t k = 0; k < s.trails.v.size(); ++k) {                        // a recorded end wins
        mvl::Trail& R = s.trails.v[k]; if (R.fromRec) continue;
        std::map<std::pair<long long, long long>, mvl::TrEnd>::const_iterator it = s.recEnds.find(std::make_pair(R.id, R.key0));
        if (it == s.recEnds.end()) continue;
        R.end = it->second.end; R.endKey = it->second.endKey; R.endMs = it->second.endMs; R.endPx = it->second.endPx; R.endLots = it->second.endLots;
        R.atrAt = it->second.atr; R.gapUsed = it->second.gap; R.nUsed = it->second.n; R.fromRec = true; R.recorded = true; R.cutAfterEnd();
    }
}

// (1.10.0) decide the stop-following B? on closed candles (3 s after a new candle appeared, so the trail file has caught up), record each
// final end once, then draw at most 2 trails per side: a thin stepped line, dot / x / "B?" at the end
void DefenseLines::drawTrails(DlState& s, const ss::Values& V, int n, int v0, int v1, RTARRAYI& dtA, RTARRAY& hiA, RTARRAY& loA, RTARRAY& clA, int ppb, size_t& drawn) {
    s.trHits.clear(); s.trailsDrawn = 0;
    const double tick = s.trails.tick; if (!(tick > 0) || s.trails.v.empty() || n < 3) return;
    const long long now = dlSteadyMs();
    if (n != s.lastN) { s.lastN = n; s.nChangedMs = now; }
    const int first = std::max(0, n - 3000);
    std::vector<mvl::TBar> bars; bars.reserve((size_t)(n - first));
    for (int i = first; i < n; ++i) { mvl::TBar b; b.key = (long long)dtA[i]; b.hi = (long long)std::llround((double)hiA[i] / tick); b.lo = (long long)std::llround((double)loA[i] / tick); b.cl = (long long)std::llround((double)clA[i] / tick); bars.push_back(b); }
    const int lastDec = (now - s.nChangedMs >= 3000 ? n - 2 : n - 3) - first;   // closed candles only; the newest one after a 3 s grace
    mvl::decideStops(s.trails, bars, lastDec, std::max(0.1, V.f(dlset::K_TRAIL_GAP)), std::max(1, V.i(dlset::K_TRAIL_N)));
    for (size_t k = 0; k < s.trails.v.size(); ++k) {                         // record each final end once (flushed good append = recorded)
        mvl::Trail& R = s.trails.v[k]; if (R.recorded || !mvl::finalEnd(R, bars, lastDec)) continue;
        const std::pair<long long, long long> id = std::make_pair(R.id, R.key0);
        if (s.recDone.count(id)) { R.recorded = true; continue; }
        std::ofstream f(s.recPath.c_str(), std::ios::binary | std::ios::app);
        if (f.is_open()) { f << mvl::trLineOf(R, tick) << "\n"; f.flush(); }
        if (f.is_open() && f.good()) { R.recorded = true; R.fromRec = true; s.recDone.insert(id); ++s.recWritten; } else ++s.recFails;
    }
    const mp::Params TP = mp::get("Trails", s.mkt); s.trShowP = mp::mode(TP, "SHOW", mp::LOG); s.trShowB = mp::mode(TP, "SHOW_B", mp::LOG);   // (1.11.0)
    if (!V.b(dlset::K_SHOW_TRAILS) || s.trShowP == mp::LOG) return;     // (1.11.0) LOG ONLY: decided + recorded above, nothing drawn
    const long long kLo = (long long)dtA[v0], kHi = (long long)dtA[v1];
    const std::vector<size_t> show = mvl::shownTrails(s.trails, kLo, kHi);
    auto idxOf = [&](long long key) -> int { const int j = mvl::barIdx(bars, key); return j < 0 ? -1 : j + first; };
    auto exactIdxOf = [&](long long key) -> int { const int j = mvl::barIdx(bars, key); return j < 0 || bars[(size_t)j].key != key ? -1 : j + first; };
    auto xOf = [&](int i) { PNT p; p.set(i, 0.0f, kBarCenter); return (int)p.h; };
    auto yOf = [&](long long px) { PNT p; p.set(v0, (float)((double)px * tick), kBarCenter); return (int)p.v; };
    for (size_t q = 0; q < show.size(); ++q) {
        const mvl::Trail& R = s.trails.v[show[q]];
        const COLOR col = (COLOR)(R.side == mvl::BIDS ? V.c(dlset::K_COL_TRAIL_BID) : V.c(dlset::K_COL_TRAIL_OFF));
        int a = idxOf(R.key0); if (a < 0) continue;
        const char end = mvl::finalEnd(R, bars, lastDec) ? R.end : 0;        // an end not final yet is drawn as still following (never repainted)
        // `barIdx` deliberately clips a path to the next bar, but an immutable end must never be moved
        // across an RTH/session/data gap.  The path is safer absent than an F/X/B decision at a false time.
        int endI = n - 1;
        if (end) { endI = exactIdxOf(R.endKey); if (endI < 0) continue; }
        // the path: (bar, price) corners
        std::vector<std::pair<int, long long> > P; P.push_back(std::make_pair(a, R.px0));
        long long cur = R.px0;
        for (size_t k = 0; k < R.steps.size(); ++k) {
            int b = idxOf(R.steps[k].key); if (b < 0) b = n - 1; if (end && b > endI) break;
            P.push_back(std::make_pair(b, cur)); cur = R.steps[k].px; P.push_back(std::make_pair(b, cur));
        }
        P.push_back(std::make_pair(std::max(endI, P.back().first), cur));
        setPen(col, 1, P_SOLID);
        bool any = false;
        for (size_t k = 0; k + 1 < P.size(); ++k) {
            const int b0 = P[k].first, b1 = P[k + 1].first;
            if (P[k].second == P[k + 1].second) {                             // horizontal: clipped to the visible candles
                const int c0 = std::max(b0, v0), c1 = std::min(b1, v1); if (c0 > c1 || (c0 == c1 && b0 != b1)) continue;
                const int x0 = xOf(c0), x1 = xOf(c1), y = yOf(P[k].second); if (x1 <= x0) continue;
                ko::line(x0, y, x1, y); s.trHits.push_back(DlState::TrHit{ show[q], x0, y - 3, x1, y + 3 }); any = true;
            } else if (b0 >= v0 && b0 <= v1) {                                // vertical: the step
                const int x = xOf(b0), y0 = yOf(P[k].second), y1 = yOf(P[k + 1].second);
                ko::line(x, y0, x, y1); s.trHits.push_back(DlState::TrHit{ show[q], x - 3, std::min(y0, y1), x + 3, std::max(y0, y1) }); any = true;
            }
        }
        if (end && endI >= v0 && endI <= v1) {                                // the end mark
            const int x = xOf(endI), y = yOf(end == 'F' ? R.endPx : cur);
            if (end == 'F') {                                               // filled dot (7 px)
                static const int W[7] = { 1, 2, 3, 3, 3, 2, 1 };
                if (!bdk::hide(x - 3, y - 3, x + 4, y + 4)) for (int r = -3; r <= 3; ++r) ko::rawLine(x - W[r + 3], y + r, x + W[r + 3] + 1, y + r);
            } else if (end == 'X') {
                if (!bdk::hide(x - 4, y - 4, x + 5, y + 5)) { ko::rawLine(x - 3, y - 3, x + 4, y + 4); ko::rawLine(x - 3, y + 3, x + 4, y - 4); }
            } else if (R.stopLetter() && s.trShowB != mp::LOG) {          // (1.11.0) B? only when SHOW_B
                FONT fo; fo.id = HELVETICA; fo.size = 10; fo.style = BOLD; setFont(fo);
                const int tw = (int)getTextWidth("B?", -1) + 2, l = x + std::max(3, ppb / 2), t = R.side == mvl::BIDS ? y - 15 : y + 1;
                if (!bdk::hide(l, t, l + tw, t + 14)) { setTextColor(col); RCT rc; rc.set((short)l, (short)t, (short)(l + tw), (short)(t + 14)); rc.drawText("B?", false, false); s.trHits.push_back(DlState::TrHit{ show[q], l, t, l + tw, t + 14 }); }
            }
            any = true;
        }
        if (any) { ++s.trailsDrawn; ++drawn; }
    }
    // the hover: only while the cursor is over this chart's window and on a drawn trail
    if (!s.msHas || s.msWnd == 0 || hb::hbCursor() != s.msWnd) return;
    const DlState::TrHit* H = nullptr;
    for (size_t k = 0; k < s.trHits.size(); ++k) { const DlState::TrHit& h = s.trHits[k]; if (s.msH >= h.l && s.msH <= h.r && s.msV >= h.t && s.msV <= h.b) { H = &h; break; } }
    if (!H) return;
    mvl::Trail R = s.trails.v[H->idx]; if (!mvl::finalEnd(R, bars, lastDec)) R.end = 0;   // the hover says what the chart shows
    auto pxText = [&](long long px) { char b[48]; const double v = (double)px * tick; const int dec = tick >= 1 ? 0 : tick >= 0.1 ? 1 : tick >= 0.01 ? 2 : tick >= 0.001 ? 3 : tick >= 0.0001 ? 4 : 5; std::snprintf(b, sizeof b, "%.*f", dec, v); return std::string(b); };
    const std::vector<std::string> L = mvl::trailHover(R, pxText(R.px0), pxText(R.lastPx()), pxText(R.end == 'F' ? R.endPx : R.lastPx()), clockOf(R.key0), R.end ? clockOf(R.endKey) : std::string("now"));
    RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
    int axisR = pane.right; if (scale.left > pane.left && scale.left < pane.right) axisR = scale.left - 2;
    FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
    int w = 0; for (size_t i = 0; i < L.size(); ++i) w = std::max(w, (int)getTextWidth(L[i].c_str(), -1));
    w += 16; const int lh = 15, hh = (int)L.size() * lh + 10;
    int l = s.msH + 14, t = s.msV + 14;
    if (l + w > axisR - 2) l = s.msH - 14 - w;
    if (l < pane.left + 2) l = pane.left + 2;
    if (t + hh > pane.bottom - 2) t = pane.bottom - 2 - hh;
    if (t < pane.top + 2) t = pane.top + 2;
    RCT bg; bg.set((short)l, (short)t, (short)(l + w), (short)(t + hh)); bg.draw(1, (COLOR)0x00606060, (COLOR)0x00FFFFFF, DRAW_OPAQUE, PAT_SOLID);
    for (size_t i = 0; i < L.size(); ++i) {
        setTextColor(i == 0 ? (COLOR)(R.side == mvl::BIDS ? V.c(dlset::K_COL_TRAIL_BID) : V.c(dlset::K_COL_TRAIL_OFF)) : (COLOR)0x00202020);
        RCT tr; tr.set((short)(l + 8), (short)(t + 5 + (int)i * lh), (short)(l + w - 4), (short)(t + 5 + (int)(i + 1) * lh)); tr.drawText(L[i].c_str(), false, false);
    }
}

// (1.10.0) the mouse: remember where it is; redraw only when the hovered trail changes. Never swallows the event.
int DefenseLines::mouse(RTX_EVENT* e) {
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    DlState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    if (S->trHits.empty() && S->hovKey.empty()) return RTX_FAIL;
    S->msHas = true; S->msH = e->v.mouse.h; S->msV = e->v.mouse.v; S->msWnd = hb::hbCursor();
    std::string k;
    for (size_t i = 0; i < S->trHits.size(); ++i) { const DlState::TrHit& h = S->trHits[i]; if (S->msH >= h.l && S->msH <= h.r && S->msV >= h.t && S->msV <= h.b) { k = std::to_string(h.idx); break; } }
    if (k != S->hovKey) { S->hovKey = k; invalidateChart(false); }
    return RTX_FAIL;
}

int DefenseLines::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    DlState* s = slot_.get(this, true);
    if (!s) return RTX_OK;
    try {
        s->mkt = chartMarket(); s->spb = (int)getSecondsPerBar();
        if (s->mkt.empty() || s->spb <= 0) return RTX_OK;
        refresh(*s);
        s->keep.refresh(getSymbol(), getSecondsPerBar(), getChartLabel());   // (1.8.0) the HOD/LOD table: nothing painted over it this draw
        bdk::KeepScope keepScope(s->keep);
        const ss::Values& V = s->set;
        const bool showLines = V.b(dlset::K_SHOW_LINES), showE = false /* (1.13.0) the gray E line is retired */, showDots = V.b(dlset::K_SHOW_DOTS);
        const int maxBars = std::max(1, V.i(dlset::K_MAX_CANDLES));
        const int lineW = std::max(1, std::min(3, V.i(dlset::K_LINE_W))); const bool skipFill = V.b(dlset::K_DOT_SKIP_FILL);   // (1.12.0)
        s->dotsSkipped = 0;
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
        const COLOR colLow = (COLOR)V.c(dlset::K_COL_LOW), colHigh = (COLOR)V.c(dlset::K_COL_HIGH), colE = (COLOR)0x00A3A3A3 /* (1.13.0) unused: E not drawn */, colDot = (COLOR)V.c(dlset::K_COL_DOT);
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
        auto drawLine = [&](int st, int side, double px, long long nextKey, COLOR col) {   // (1.9.0) a clean line: no label
            if (st < 0 || st > v1 || st < v0 - maxBars) return;
            const dt::Life life = lifeAt(st, side, px, nextKey);
            if (life.broken) return;                                          // failed: removed
            const int j = st + life.bars;
            if (j < v0) return;
            PNT a; a.set(st, (float)px, kBarCenter); PNT b; b.set(j, (float)px, kBarCenter);
            const int xa = a.h - ppb / 2, xb = b.h + ppb / 2;
            setPen(col, lineW, P_SOLID);                                      // (1.12.0) thinner (was 2)
            ko::line(xa, a.v, xb, a.v);                                       // (1.8.0) cut at the HOD/LOD table
            if (showDots && st >= v0) { if (skipFill && s->fillKeys.count((long long)dtA[st])) ++s->dotsSkipped; else dot(a.h, a.v, colDot); }   // (1.12.0) the order fill dot wins its candle
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
            if (st >= 0 && lifeAt(st, L.side, L.linePx, nextKey).broken) { ++failed; continue; }   // the R failed: R and its E removed
            if (showLines) drawLine(st, L.side, L.linePx, nextKey, L.side == dt::HIGH ? colHigh : colLow);
            if (haveX) {
                const int xs_ = indexOf(xi->second.at);
                const int sx = traded(xs_, xi->second.px) ? xs_ : (traded(piv, xi->second.px) ? piv : i);
                drawLine(sx, L.side, xi->second.px, nextKey, colE);
            }
        }
        // (1.5.0, Top Defender #3) the defenders PRESSED IN on a candle (the pane's P): the largest such move - hollow box at the old
        // price, solid box at the new price with only its lot size; in the defending colour; inside the visible candles only
        // (1.6.0) every qualified case: per BOOK side the largest move in the direction it qualified; bids green (LOW colour), offers red
        // (HIGH colour); two sides on one candle: bids left of the candle centre, offers right, so the four boxes never overlap
        for (std::map<std::pair<int, long long>, mvl::Read>::const_iterator it = s->ms.begin(); it != s->ms.end(); ++it) {
            const mvl::Read& R = it->second; if (!kDrawMoveBoxes || !mvl::anyDrawn(R)) continue;   // (1.13.0) move boxes retired
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
                if (bdk::hide(l, std::min(po.v, pn.v) - hh / 2, r, std::max(po.v, pn.v) + hh / 2 + 1)) continue;   // (1.8.0) a box under the HOD/LOD table: not painted (still in the M line)
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
        drawTrails(*s, V, (int)n, v0, v1, dtA, hiA, loA, clA, ppb, drawn);   // (1.10.0)
        if (s->haveLive && showLines) {                                      // the forming candle: 2 bars long, no text (1.9.0); (1.12.0) solid 1 px - no dotted / dashed lines
            const dt::Line& L = s->liveLine; const int i = indexOf(L.t);
            if (i >= 0 && i >= (int)n - 2 && i <= v1 + 1) {
                const COLOR col = L.side == dt::HIGH ? colHigh : colLow;
                PNT a; a.set(i, (float)L.linePx, kBarCenter);
                setPen(col, 1, P_SOLID);
                ko::line(a.h - ppb / 2, a.v, a.h + 2 * ppb, a.v);
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
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE);   // (1.10.0) TRACK_MOUSE for the trail hover only (never swallows an event)
    p->setDescription("LRA Defense Lines - the Defense pane's pivot turns on the price chart. A TURN = your pivot rule (the candle making the new high / low, or the next one trading back into it) + the defending side's Defense bar bigger (offers red at a high, bids green at a low) + the candle closing in the defending colour AND beyond the R line (above it at a low, below it at a high). R LINE (red at a high, green at a low) = the price near the pivot where the defenders re-posted the most of what was eaten. (1.13.0: the gray E exhaustion line is no longer drawn; it is still decided and recorded for the nightly.) Yellow dot = the candle each line refers to. The lines carry no text: the letters (A absorbed, H hit back, L / X re-test ...) are in the Defense pane's badge row - hover a badge there for its numbers. Each line runs at most 3 candles (setting), until the next turn's lines, or until a candle closes through it (then it is removed). Dotted line = the forming candle, gone if it does not confirm. Example: 8:48 AM ES low: green R at 7827.25, dot on the 8:48 candle. TRAILS (1.10.0): a thin stepped line = ONE big resting order (one order id) re-priced again and again behind price during the move - green = a bid stepping up under a rally, red = an offer stepping down above a selloff; it ends once: dot = it traded, x = pulled, B? = it stopped following (price ran 0.5 ATR on and it did not follow in 2 candles) or stepped back - support / resistance quitting; hover a trail for its numbers. At most 2 per side. Needs lsBookFlow (Defense) on the same chart.");
    p->setVersion("1.13.0");   // DL_VERSION
    return p;
}
