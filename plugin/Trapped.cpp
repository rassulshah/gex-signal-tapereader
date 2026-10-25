/********************************************************************************
 *  Trapped.cpp  --  Investor/RT RTX extensions  lsTrapped 1.0.0 (its own pane)  and  lsTrappedMarks 1.0.0 (TrappedMarks.cpp:
 *  the same source with TRP_MARKS, the "Tr" letter on the PRICE chart)  (2026-10-24)
 *
 *  Rassul 23:28 CT 2026-10-24: "istrapped is a go" - Option A of the trapped-traders mockup (trap/opt-A.png, logic trap/opts.py):
 *    - per trap a SPIKE on the Tr candle = trapped lots vs normal (sellers trapped up / green, buyers trapped down / red), BRIGHT at
 *      2.0x+ (setting) else MUTED (a darker shade of the same colour); "3,625 \xB7 8.1x" next to bright spikes only;
 *    - a thin FUEL-LEFT line after each spike (bright for big traps, faint for small) until the trap is freed / spent;
 *    - one message line top-left for the most recent big trap ("9:03 Sellers trapped 3,140 lots (7.1x) below the 8:48 low -
 *      fuel left 2,830 (est.) - buying to come.");
 *    - lsTrappedMarks: "Tr" under the sweep candle (sellers trapped, green) / over it (buyers trapped, red), bright traps bigger, small
 *      traps muted; in the 4th text lane (the first three belong to lsBigBidAsk BO / BD, Rly and lsSweepRefill Sr), never over the
 *      HOD/LOD table (KeepOutDraw.h); no boxes, no dashed borders, no grey sub-text.
 *    - hover on a spike / Tr: lots, share, avg, ratio vs normal, ticks + ATR underwater, % still trapped, pain $, fuel left, status,
 *      then question / what / how / use / example.
 *  An IRT indicator draws in ONE pane, so the pane and the price-chart letter are two DLLs from one source (as lsLiquidityProfile /
 *  lsLiquidityFlow); both run the same engine on the chart's own candles; only the pane writes the record and the status file.
 *  Rules: TrappedLogic.h (SDK-free, tests_trapped/test_trapped_logic.cpp on the real ES 10-05 / NQ 10-09 days).
 *
 *  DATA: the chart's candles + volume at price per RTH candle (VAP_REQUIRED: VOLPROFILE buyVolume = ask / bought, sellVolume =
 *  bid / sold), SYM_TICKINCR (the only tick source), SYM_VALTICK (pain $; the per-market table when the chart has none). No other
 *  input at run time. The last 20 sessions on the chart are back-filled (<= 400 volume-at-price reads per pass) so the normal is
 *  ready on day one; a candle whose volume at price is not loaded yet is retried (20 passes, then fed without it).
 *  FILES (lsFlexLevels\Trapped\):  <MKT>-<CONTRACT>-<spb>-<session>.csv   TRP / TRS / TRD lines, append-only (the record wins)
 *                                  Trapped.status-<MKT>-<spb>.txt          plain-words status, on change or every 30 s (keyed by
 *                                  market and bar size - lesson 9: two charts must not overwrite each other)
 *  Settings: lsFlexLevels\Trapped.settings-<MKT>.txt / TrappedMarks.settings-<MKT>.txt (TrappedSettings.h, SettingsSpec layer).
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
#include "TrappedLogic.h"
#include "SwingRef.h"          // swg::rthOf / swg::isRth only (the RTH table shared with lsSweepRefill / lsStopRun)
#include "HostSlot.h"
#include "TrappedSettings.h"
#include "SettingsHost.h"
#include "SettingsGuard.h"
#include "KeepOutDraw.h"       // the HOD/LOD table keep-out (price chart letters)
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <new>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#if !defined(_WIN32)
#include <dirent.h>
#endif

#define TRP_VERSION "1.0.0"     // = trp::VERSION = trpset::PLUGIN_VERSION = the setVersion literal
#ifdef TRP_MARKS
#define TRP_SPEC trpset::marksSpec
#define TRP_NAME "TrappedMarks"
#else
#define TRP_SPEC trpset::paneSpec
#define TRP_NAME "Trapped"
#endif

static const int HOVER_PT = 10, HOVER_LH = 14;
static const int LANES_RESERVED = 3;     // above / below a candle: lsBigBidAsk BO / BD, Rly, then lsSweepRefill Sr - Tr takes the 4th lane
static const int MAX_BARS = 20 * 480;    // ~20 sessions of 3-min candles
static const int VAP_READS_PER_PASS = 400, VAP_RETRY_PASSES = 20;

static long long trpMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string trpFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + TRP_SEP "InvestorRT" TRP_SEP "rtx" TRP_SEP "lsFlexLevels" : std::string(); }
static void trpMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool trpExists(const std::string& p) { struct stat st; return !p.empty() && stat(p.c_str(), &st) == 0; }
static std::string fileKey(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) { const unsigned char c = (unsigned char)s[i]; out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_'); }
    return out.empty() ? "UNKNOWN" : out;
}
// bounded read of a text file (cap bytes / lines), lines without CR
static bool readLines(const std::string& p, std::vector<std::string>& out, size_t capBytes, size_t capLines) {
    out.clear(); std::ifstream f(p.c_str(), std::ios::binary); if (!f) return false;
    f.seekg(0, std::ios::end); const std::streamoff sz = f.tellg(); if (sz < 0 || sz > (std::streamoff)capBytes) return false; f.seekg(0, std::ios::beg);
    std::string ln; while (std::getline(f, ln) && out.size() < capLines) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); out.push_back(ln); }
    return true;
}
// this market's record files for this bar size: <MKT>-<CONTRACT>-<spb>-<session>.csv -> (session, path)
static std::vector<std::pair<long long, std::string> > listRecords(const std::string& dir, const std::string& mkt, int spb) {
    std::vector<std::pair<long long, std::string> > out; std::vector<std::string> names;
#if defined(_WIN32)
    WIN32_FIND_DATAA fd; const std::string pat = dir + TRP_SEP + mkt + "-*-" + std::to_string(spb) + "-*.csv";
    HANDLE h = FindFirstFileA(pat.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) { do { if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) names.push_back(fd.cFileName); } while (FindNextFileA(h, &fd) && names.size() < 4000); FindClose(h); }
#else
    if (DIR* d = opendir(dir.c_str())) { while (dirent* e = readdir(d)) { names.push_back(e->d_name); if (names.size() >= 4000) break; } closedir(d); }
#endif
    const std::string pre = mkt + "-", mid = "-" + std::to_string(spb) + "-";
    for (size_t i = 0; i < names.size(); ++i) {
        const std::string& n = names[i];
        if (n.size() < pre.size() + 8 || n.compare(0, pre.size(), pre) != 0 || n.compare(n.size() - 4, 4, ".csv") != 0) continue;
        const size_t dash = n.rfind('-', n.size() - 5); if (dash == std::string::npos || dash < pre.size()) continue;
        long long s = 0; if (!trp::fullLL(n.substr(dash + 1, n.size() - 5 - dash), s) || s <= 0) continue;
        const size_t d2 = n.rfind('-', dash - 1); if (d2 == std::string::npos || n.substr(d2, dash - d2 + 1) != mid) continue;
        out.push_back(std::make_pair(s, dir + TRP_SEP + n));
    }
    std::sort(out.begin(), out.end());
    return out;
}

// ---- the chart's volume at price (only trivial values live in these MSVC SEH helpers, as lsSweepRefill) ----------------------------
static int trpStats(cppExtension::RTARRAYP* vap, int bar, cppExtension::BARSTATISTICS* st) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || !st || bar < 0) return -2;
        std::memset(st, 0, sizeof(*st));
        if (vap->getBarStatistics(bar, *st) != RTX_OK) return -2;
        if (st->prices < 0) return -2;
        return st->prices;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
static int trpRowsRaw(cppExtension::RTARRAYP* vap, int bar, cppExtension::VOLPROFILE* rows, int count) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) { std::memset(&rows[i], 0, sizeof(rows[i])); if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2; }
        return count;                                                  // any row failure rejects the whole bar, never a prefix
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// one candle's cells; -1 = a fault (stop reading VAP), 0 = not loaded yet, 1 = ok
static int trpVap(cppExtension::RTARRAYP& vap, int bar, double tick, std::vector<trp::Cell>& out) {
    out.clear();
    cppExtension::BARSTATISTICS st; const int n = trpStats(&vap, bar, &st);
    if (n == -1) return -1;
    if (n <= 0 || n > 5000) return 0;
    std::vector<cppExtension::VOLPROFILE> rows((size_t)n);
    const int got = trpRowsRaw(&vap, bar, &rows[0], n);
    if (got == -1) return -1;
    if (got != n) return 0;
    std::map<long long, trp::Cell> m;
    for (size_t i = 0; i < rows.size(); ++i) {
        long long k = 0;
        if (!trp::toTicks((double)rows[i].price, tick, k) || rows[i].buyVolume < 0 || rows[i].sellVolume < 0) continue;
        trp::Cell& c = m[k]; c.px = k; c.ask += (long long)rows[i].buyVolume; c.bid += (long long)rows[i].sellVolume;
    }
    for (std::map<long long, trp::Cell>::const_iterator it = m.begin(); it != m.end(); ++it) out.push_back(it->second);
    return out.empty() ? 0 : 1;
}

struct HitBox { int l, t, r, b; size_t idx; };

struct TrpState {
    std::string identity, market, symbol; int spb = 0; double tick = 0, tickValue = 0;
    trp::Engine E; bool loaded = false; long long lastFedT = 0; long long retryT = 0; int retries = 0; bool vapFault = false;
    long long fed = 0, fedRth = 0, noVap = 0; long long recLines = 0, recFails = 0, recBad = 0, recOld = 0, recOther = 0, files = 0;
    std::vector<trp::Trap> shown;                 // the engine's traps + record-only ones (what is drawn)
    ss::Store setStore; ss::Values set = ss::defaults(TRP_SPEC(), -1); long long setCheckMs = -100000;
    std::vector<HitBox> hits; int hover = -1; double R = 4.0; int paneH = 0;
    bdk::TableWatch keep;
    std::string statusBody; long long statusMs = -100000; int busy = 0;
};
static ss::Ids& trpIds() { static ss::Ids I; return I; }

class Trapped : public cppExtension {
public:
    SS_SAFE_DIALOG(Trapped, TRP_NAME)
    HostSlot<TrpState> slot_;
    Trapped() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
#ifndef TRP_MARKS
    virtual int scale(int iStartBar, int iEndBar, double* dMin, double* dMax);
#endif
    virtual int parmsLoadBody(void) { ss::guardedCall(this, &Trapped::loadBody, 0u); return RTX_OK; }
    virtual int parmsApplyBody(void) { ss::guardedCall(this, &Trapped::applyBody, 0u); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::guardedCall(this, &Trapped::updtBody, i); return RTX_OK; }
    void loadBody(unsigned) {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        ss::fillDialog(this, TRP_SPEC(), trpIds(), st->load(ss::marketIndex(dlgMkt_), trpMs(), true), snap_);
    }
    void applyBody(unsigned) { saveSettings(); }
    void updtBody(unsigned i) { ss::onUpdate(this, TRP_SPEC(), trpIds(), (int)i, snap_); saveSettings(); }
    void pump();
private:
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        const std::string mkt = ss::safeMarket(this); const std::string p = trpset::settingsPath(trpFlex(), TRP_NAME, mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &TRP_SPEC(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, TRP_SPEC(), trpIds(), st->load(ss::marketIndex(dlgMkt_), trpMs(), true), snap_);
        trpMkdir(trpFlex());
        if (st->save(V)) snap_.take(ss::dialogNow(this, TRP_SPEC(), trpIds(), V));
    }
    void identify(TrpState& S);
    void settings(TrpState& S, long long now);
    void loadFiles(TrpState& S);
    void bars(TrpState& S);
    void flush(TrpState& S);
    void status(TrpState& S, long long now);
    long long ctSec(long long t);
    std::string hm(long long t);
    std::string recPath(const TrpState& S, long long sess);
    double bigX(const TrpState& S) const;
    void hoverBox(TrpState& S, const RCT& pane);
#ifdef TRP_MARKS
    void renderMarks(TrpState& S);
#else
    void renderPane(TrpState& S);
    double rangeFor(TrpState& S, int i0, int i1);
#endif
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { static_cast<Trapped*>(this)->slot_.release(this); return RTX_OK; }      // symbol / period change: start clean
int cppExtension::destroy(void) { static_cast<Trapped*>(this)->slot_.release(this); return RTX_OK; }
int cppExtension::setup(void) { ss::registerAll(this, TRP_SPEC(), trpIds()); return RTX_OK; }
int cppExtension::calc(int) { static_cast<Trapped*>(this)->pump(); return RTX_OK; }

// IRT's local clock (Central on his PC) as civil seconds
long long Trapped::ctSec(long long t) {
    struct tm m; std::memset(&m, 0, sizeof m);
    if (t <= 0 || !getLocaltime((RTDATE)t, &m)) return 0;
    int y = m.tm_year + 1900; const unsigned mo = (unsigned)m.tm_mon + 1, d = (unsigned)m.tm_mday;
    y -= mo <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + (long long)doe - 719468) * 86400 + m.tm_hour * 3600 + m.tm_min * 60 + m.tm_sec;
}
// "9:03" (12-hour Central, the candle END as the approved mockup)
std::string Trapped::hm(long long t) {
    struct tm m; std::memset(&m, 0, sizeof m);
    if (t <= 0 || !getLocaltime((RTDATE)t, &m)) return "?";
    char b[16]; std::snprintf(b, sizeof b, "%d:%02d", m.tm_hour % 12 == 0 ? 12 : m.tm_hour % 12, m.tm_min); return b;
}
std::string Trapped::recPath(const TrpState& S, long long sess) {
    const std::string f = trpFlex(); if (f.empty() || S.market.empty()) return std::string();
    return f + TRP_SEP "Trapped" TRP_SEP + S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb) + "-" + std::to_string(sess) + ".csv";
}
double Trapped::bigX(const TrpState& S) const {
#ifdef TRP_MARKS
    return S.set.f(trpset::K_TR_BIG_X);
#else
    return S.set.f(trpset::K_BIG_X);
#endif
}

void Trapped::identify(TrpState& S) {
    const std::string mkt = ss::safeMarket(this);
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!trp::finite(tick) || tick <= 0 || tick >= 1000) tick = 0;
    else { char b[32]; std::snprintf(b, sizeof b, "%.6g", tick); tick = std::strtod(b, nullptr); }   // the float made exact (0.00005f)
    const std::string id = mkt + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity && tick == S.tick) return;
    S.identity = id; S.market = mkt; S.symbol = sym; S.spb = spb; S.tick = tick;
    double tv = (double)getProperty(SYM_VALTICK); S.tickValue = trp::finite(tv) && tv > 0 && tv < 100000 ? tv : trp::tickValueOf(mkt);
    S.E = trp::Engine(); S.E.tickForLines = tick > 0 ? tick : 0.25; S.loaded = false; S.lastFedT = 0; S.retryT = 0; S.retries = 0; S.vapFault = false;
    S.fed = S.fedRth = S.noVap = 0; S.recLines = S.recFails = S.recBad = S.recOld = S.recOther = S.files = 0; S.shown.clear();
    S.setStore = ss::Store(); S.setCheckMs = -100000; S.hits.clear(); S.hover = -1; S.statusBody.clear();
}

void Trapped::settings(TrpState& S, long long now) {
    if (now - S.setCheckMs < 1000 && now >= S.setCheckMs) return;
    S.setCheckMs = now;
    const std::string sp = trpset::settingsPath(trpFlex(), TRP_NAME, S.market);
    if (S.setStore.path != sp) { S.setStore = ss::Store(); S.setStore.S = &TRP_SPEC(); S.setStore.path = sp; }
    S.set = sp.empty() ? ss::defaults(TRP_SPEC(), ss::marketIndex(S.market)) : S.setStore.load(ss::marketIndex(S.market), now);
}

// the record files of this market / bar size (any contract): the normal history + what was decided (the record wins)
void Trapped::loadFiles(TrpState& S) {
    if (S.loaded) return;
    S.loaded = true;
    const std::string f = trpFlex(); if (f.empty()) return;
    std::vector<std::pair<long long, std::string> > L = listRecords(f + TRP_SEP "Trapped", S.market, S.spb);
    if (L.size() > 60) L.erase(L.begin(), L.begin() + (long)(L.size() - 60));      // the newest 60 session files
    std::map<long long, std::vector<long long> > hist; std::set<long long> present;
    for (size_t i = 0; i < L.size(); ++i) {
        std::vector<std::string> lines; if (!readLines(L[i].second, lines, 2 * 1024 * 1024, 20000)) continue;
        ++S.files;
        trp::Record R; trp::parseLines(lines, S.tick, R);
        S.recBad += R.bad; S.recOld += R.old; S.recOther += R.otherKind;
        S.E.loadRecord(R);
        bool any = !R.doneSess.empty();
        for (size_t k = 0; k < R.traps.size(); ++k) { hist[R.traps[k].sess].push_back(R.traps[k].lots); any = true; }
        if (any) present.insert(L[i].first);
    }
    for (std::set<long long>::const_iterator it = present.begin(); it != present.end(); ++it) S.E.H.setSession(*it, hist[*it]);
}

// closed candles (a later bar exists), strictly in order, each once; the volume at price of RTH candles
void Trapped::bars(TrpState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYP vap(barVolumeProfile);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const bool haveVap = (long)vap.count >= n;
    int oMin = 0, cMin = 0; const bool haveRth = swg::rthOf(S.market, oMin, cMin);
    const int lastClosed = (int)n - 2;
    if ((long long)times[lastClosed] <= S.lastFedT) return;
    int from = lastClosed; while (from > 0 && (long long)times[from - 1] > S.lastFedT && lastClosed - from < MAX_BARS) --from;
    int reads = 0;
    for (int i = from; i <= lastClosed; ++i) {
        const long long t = (long long)times[i]; if (t <= S.lastFedT) continue;
        trp::Bar b; b.t = t;
        const long long cs = ctSec(t); if (cs <= 0) { S.lastFedT = t; continue; }
        b.sess = (cs - 1 - 17LL * 3600) / 86400 + 1;
        const int sod = (int)(((cs % 86400) + 86400) % 86400);
        b.rth = haveRth && swg::isRth(sod, S.spb, oMin, cMin);
        if (!trp::toTicks(op[i], S.tick, b.o) || !trp::toTicks(hi[i], S.tick, b.h) || !trp::toTicks(lo[i], S.tick, b.l) || !trp::toTicks(cl[i], S.tick, b.c)) { S.lastFedT = t; continue; }
        if (b.rth && haveVap && !S.vapFault) {
            if (reads >= VAP_READS_PER_PASS) break;                        // the rest next pass (lesson 18 / 40)
            ++reads;
            const int r = trpVap(vap, i, S.tick, b.cells);
            if (r < 0) S.vapFault = true;
            else if (r == 0) {                                             // not loaded yet: retry, then feed without it (lesson 61)
                if (S.retryT != t) { S.retryT = t; S.retries = 0; }
                if (++S.retries < VAP_RETRY_PASSES) break;
            }
            b.cellsOk = r > 0;
        }
        if (b.rth && !b.cellsOk) ++S.noVap;
        S.E.feed(b); S.lastFedT = t; ++S.fed; if (b.rth) ++S.fedRth;
    }
    S.shown = S.E.traps;
    const std::vector<trp::Trap> ro = S.E.recordOnly(); S.shown.insert(S.shown.end(), ro.begin(), ro.end());
}

// the engine's new lines -> the session's record file (the pane only; one flushed append per line, failures counted)
void Trapped::flush(TrpState& S) {
#ifdef TRP_MARKS
    S.E.newLines.clear();                                                  // the letters never write (the pane owns the record)
#else
    if (S.E.newLines.empty()) return;
    const std::string f = trpFlex();
    if (!f.empty()) { trpMkdir(f); trpMkdir(f + TRP_SEP "Trapped"); }
    for (size_t i = 0; i < S.E.newLines.size(); ++i) {
        const std::string& line = S.E.newLines[i];
        const std::vector<std::string> p = trp::split(line, '|');
        long long sess = 0; const size_t at = p.size() > 0 && p[0] == "TRD" ? 2 : 3;
        if (p.size() <= at || !trp::fullLL(p[at], sess) || sess <= 0) { ++S.recFails; continue; }
        const std::string path = recPath(S, sess); if (path.empty()) { ++S.recFails; continue; }
        const bool fresh = !trpExists(path);
        std::ofstream o(path.c_str(), std::ios::binary | std::ios::app);
        if (!o) { ++S.recFails; continue; }
        if (fresh) o << trp::REC_HEAD << "\n";
        o << line << "\n"; o.flush();
        if (!o) { ++S.recFails; continue; }
        ++S.recLines;
    }
    S.E.newLines.clear();
#endif
}

void Trapped::status(TrpState& S, long long now) {
#ifdef TRP_MARKS
    (void)S; (void)now;
#else
    const std::string d = trpFlex(); if (d.empty() || S.market.empty()) return;
    std::ostringstream o; char b[160];
    o << "VERSION," << TRP_VERSION << "\nMARKET," << S.market << ",SYMBOL," << S.symbol << ",SECONDS_PER_BAR," << S.spb << ",TICK," << trp::g12(S.tick) << ",TICK_VALUE $" << trp::g12(S.tickValue) << "\n";
    o << "STATUS,lsTrapped v" << TRP_VERSION << " | candles " << S.fed << " (RTH " << S.fedRth << ", without volume at price " << S.noVap << ")"
      << (S.vapFault ? " | VAP FAULT - remove and re-add the indicator" : "") << " | sweeps " << S.E.candidates << ", not qualified " << S.E.unqualified << "\n";
    const long long sess = S.E.sess;
    const int nh = S.E.H.sessionsBefore(sess, S.E.P);
    if (nh >= S.E.P.minSessions) { std::pair<long long, bool> nm = S.E.H.normalFor(sess, std::vector<long long>(), S.E.P); o << "NORMAL," << trp::commas(nm.first) << " lots = the median trap of the last " << nh << " sessions with a record\n"; }
    else o << "NORMAL,building - " << nh << " of 5 sessions of records; today's traps so far are the normal\n";
    int today = 0, big = 0; std::string list;
    for (size_t i = 0; i < S.shown.size(); ++i) {
        const trp::Trap& T = S.shown[i]; if (T.sess != sess) continue;
        ++today; if (T.ratio >= bigX(S)) ++big;
        if (today <= 12) { std::snprintf(b, sizeof b, "%s %s %s (%.1fx) %s%s; ", hm(T.trT).c_str(), T.side == trp::SELLERS ? "sellers" : "buyers", trp::commas(T.lots).c_str(), T.ratio,
                                         trp::statusName(T.status), T.endT ? (" " + hm(T.endT)).c_str() : ""); list += b; }
    }
    o << "TODAY,session " << sess << " traps " << today << " (big " << big << "): " << list << "\n";
    o << "LAST_REASON," << S.E.lastReason << "\n";
    o << "RECORD,files read " << S.files << ",lines appended " << S.recLines << ",failed " << S.recFails << ",bad lines " << S.recBad << ",old " << S.recOld << ",other kinds " << S.recOther << "\n";
    o << "SETTINGS," << S.set.why << ",fuel line " << (S.set.b(trpset::K_SHOW_FUEL) ? "on" : "off") << ",labels " << (S.set.b(trpset::K_SHOW_LABELS) ? "on" : "off")
      << ",message " << (S.set.b(trpset::K_SHOW_MSG) ? "on" : "off") << ",big " << trp::g12(bigX(S)) << "x\n";
    const std::string body = o.str();
    if (body == S.statusBody && now - S.statusMs < 30000 && now >= S.statusMs) return;
    if (now - S.statusMs < 1000 && now >= S.statusMs) return;
    S.statusMs = now; S.statusBody = body;
    trpMkdir(d); trpMkdir(d + TRP_SEP "Trapped");
    const std::string p = d + TRP_SEP "Trapped" TRP_SEP "Trapped.status-" + S.market + "-" + std::to_string(S.spb) + ".txt", tmp = p + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return; f << body; f.flush(); if (!f) return; }
    std::remove(p.c_str()); std::rename(tmp.c_str(), p.c_str());
#endif
}

void Trapped::pump() {
    TrpState* S = slot_.get(this, true);
    if (!S || S->busy) return;
    S->busy = 1;
    struct Unbusy { TrpState* s; ~Unbusy() { s->busy = 0; } } unbusy = { S };
    try {
        const long long now = trpMs();
        identify(*S);
        if (S->market.empty() || !(S->tick > 0) || S->spb <= 0) return;
        settings(*S, now);
        loadFiles(*S);
        bars(*S);
        flush(*S);
        status(*S, now);
    } catch (...) {}
}

int Trapped::mouse(RTX_EVENT* e) {
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    TrpState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    int hit = -1;
    for (size_t k = 0; k < S->hits.size() && hit < 0; ++k) { const HitBox& h = S->hits[k]; if (e->v.mouse.h >= h.l && e->v.mouse.h <= h.r && e->v.mouse.v >= h.t && e->v.mouse.v <= h.b) hit = (int)h.idx; }
    if (hit != S->hover) { S->hover = hit; invalidateChart(false); }
    return RTX_FAIL;                                                       // never swallow the chart's own mouse behaviour
}

static short trpC(int v) { return (short)(v < -32768 ? -32768 : v > 32767 ? 32767 : v); }
static COLOR muted(unsigned long c) {                                     // a darker shade of the same colour (small traps)
    const unsigned long r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
    return (COLOR)((((r * 45) / 100) << 16) | (((g * 45) / 100) << 8) | ((b * 45) / 100));
}

// the hover box of the trap under the mouse: fixed font, solid border, inside the pane
void Trapped::hoverBox(TrpState& S, const RCT& pane) {
    if (S.hover < 0 || (size_t)S.hover >= S.shown.size()) return;
    const HitBox* at = nullptr; for (size_t k = 0; k < S.hits.size(); ++k) if ((int)S.hits[k].idx == S.hover) at = &S.hits[k];
    if (!at) return;
    const trp::Trap& T = S.shown[(size_t)S.hover];
    Trapped* me = this;
    const trp::TimeFn fn = [me](long long t) { return me->hm(t); };
    const std::vector<std::string> rows = trp::hoverRows(T, S.tick, S.tickValue, fn, bigX(S));
    FONT hf; hf.id = HELVETICA; hf.size = HOVER_PT; hf.style = PLAIN; setFont(hf);
    int w = 0; for (size_t k = 0; k < rows.size(); ++k) w = std::max(w, (int)getTextWidth(rows[k].c_str(), -1));
    w += 12; const int h = (int)rows.size() * HOVER_LH + 8;
    int x0 = at->r + 8, y0 = at->t - h / 2;
    if (x0 + w > pane.right - 2) x0 = at->l - 8 - w;
    if (x0 < pane.left + 2) x0 = pane.left + 2;
    if (y0 + h > pane.bottom - 2) y0 = pane.bottom - 2 - h;
    if (y0 < pane.top + 2) y0 = pane.top + 2;
#ifdef TRP_MARKS
    const unsigned long base = T.side == trp::SELLERS ? S.set.c(trpset::K_TR_COL_BULL) : S.set.c(trpset::K_TR_COL_BEAR);
#else
    const unsigned long base = T.side == trp::SELLERS ? S.set.c(trpset::K_COL_BULL) : S.set.c(trpset::K_COL_BEAR);
#endif
    const COLOR col = (COLOR)base, bg = getBackgroundColor();
    RCT box; box.set(trpC(x0), trpC(y0), trpC(x0 + w), trpC(y0 + h)); box.draw(1, col, bg, DRAW_OPAQUE, PAT_SOLID);
    for (size_t k = 0; k < rows.size(); ++k) {
        setTextColor(k == 0 ? col : (COLOR)0xE5E7EBul);
        RCT r; r.set(trpC(x0 + 6), trpC(y0 + 4 + (int)k * HOVER_LH), trpC(x0 + w - 4), trpC(y0 + 4 + (int)(k + 1) * HOVER_LH)); r.drawText(rows[k].c_str(), false, false);
    }
}

#ifdef TRP_MARKS
// "Tr" under the sweep candle (sellers trapped, green) / over it (buyers trapped, red); big = bigger + full colour, small = muted
void Trapped::renderMarks(TrpState& S) {
    S.hits.clear();
    if (!S.set.b(trpset::K_SHOW_TR) || S.shown.empty() || !(S.tick > 0)) return;
    const long n = getBarCount(); if (n < 1) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow); if ((long)times.count < n || (long)hi.count < n || (long)lo.count < n) return;
    RCT pane; pane.getPaneRect(false);
    S.keep.refresh(getSymbol(), getSecondsPerBar(), getChartLabel());
    bdk::KeepScope keepScope(S.keep);
    auto barIdx = [&](long long t) -> long {
        long l0 = 0, h0 = n - 1, idx = -1;
        while (l0 <= h0) { const long mid = (l0 + h0) / 2; if ((long long)times[(int)mid] >= t) { idx = mid; h0 = mid - 1; } else l0 = mid + 1; }
        return idx >= 0 && (long long)times[(int)idx] == t ? idx : -1;
    };
    const double bx = bigX(S); const bool showSmall = S.set.b(trpset::K_SHOW_SMALL);
    const int lane = 16;                                                   // the lane height of lsSweepRefill (10 pt + 4 + 2)
    for (size_t k = 0; k < S.shown.size(); ++k) {
        const trp::Trap& T = S.shown[k];
        const bool big = T.ratio >= bx - 1e-9; if (!big && !showSmall) continue;
        const long idx = barIdx(T.sweepT); if (idx < 0) continue;
        const bool up = T.side == trp::SELLERS;
        const unsigned long base = up ? S.set.c(trpset::K_TR_COL_BULL) : S.set.c(trpset::K_TR_COL_BEAR);
        const COLOR col = big ? (COLOR)base : muted(base);
        FONT f; f.id = HELVETICA; f.size = (short)(big ? 11 : 9); f.style = BOLD; setFont(f);
        const int th = (big ? 11 : 9) + 4;
        PNT p; p.set((int)idx, up ? lo[(int)idx] : hi[(int)idx]); const int x = p.h;
        const int yEdge = up ? p.v + 2 + LANES_RESERVED * lane : p.v - 2 - LANES_RESERVED * lane;
        const int t = up ? yEdge : yEdge - th, b = t + th;
        if (t <= pane.top + 1 || b >= pane.bottom - 1 || x <= pane.left || x >= pane.right) continue;
        const int w = (int)getTextWidth("Tr", -1) + 4, x0 = x - w / 2;
        setTextColor(col);
        RCT r; r.set(trpC(x0), trpC(t), trpC(x0 + w), trpC(b));
        if (ko::off(r)) continue;                                          // never over the HOD/LOD table
        r.drawText("Tr", true, false);
        HitBox hb; hb.l = x0; hb.t = t; hb.r = x0 + w; hb.b = b; hb.idx = k; S.hits.push_back(hb);
    }
    hoverBox(S, pane);
}
#else
// the pane's value range (x normal), centred zero: the tallest spike / fuel point in view + room for its label and the message
double Trapped::rangeFor(TrpState& S, int i0, int i1) {
    const long n = getBarCount(); double mx = 0;
    RTARRAYI times(barDateTime);
    if (n > 0 && (long)times.count >= n) {
        const long long t0 = (long long)times[std::max(0, std::min((int)n - 1, i0))], t1 = (long long)times[std::max(0, std::min((int)n - 1, i1))];
        for (size_t k = 0; k < S.shown.size(); ++k) {
            const trp::Trap& T = S.shown[k]; if (T.trT < t0 - 86400 || T.sweepT > t1) continue;
            if (T.trT >= t0 && T.trT <= t1) mx = std::max(mx, T.ratio);
            for (size_t j = 0; j < T.fuel.size(); ++j) if (T.fuel[j].t >= t0 && T.fuel[j].t <= t1 && T.normal > 0) mx = std::max(mx, T.fuel[j].fuel / (double)T.normal);
        }
    }
    double R = std::max(4.0, mx * 1.25);
    const int half = S.paneH / 2;
    if (half > 50) R = std::max(R, mx * (double)half / (double)(half - 34));   // 34 px above the tallest spike: its label + the message line
    return R;
}
int Trapped::scale(int iStartBar, int iEndBar, double* dMin, double* dMax) {
    double R = 4.0;
    TrpState* S = slot_.get(this, false);
    if (S) { R = rangeFor(*S, iStartBar, iEndBar); S->R = R; }
    if (dMin) *dMin = -R;
    if (dMax) *dMax = R;
    return RTX_OK;
}
void Trapped::renderPane(TrpState& S) {
    S.hits.clear();
    RCT pane; pane.getPaneRect(false);
    S.paneH = pane.bottom - pane.top;
    if (S.paneH < 60 || pane.right - pane.left < 120) return;               // pane too short: draw nothing rather than overlap (#35)
    const long n = getBarCount(); if (n < 1) return;
    RTARRAYI times(barDateTime); if ((long)times.count < n) return;
    int v0 = 0, v1 = (int)n - 1; if (getVisibleBars(&v0, &v1) != RTX_OK || v0 < 0 || v1 < v0) { v0 = std::max(0, (int)n - 200); v1 = (int)n - 1; }
    v1 = std::min(v1, (int)n - 1);
    const double R = rangeFor(S, v0, v1); S.R = R;
    const int top = pane.top, bot = pane.bottom, mid = (top + bot) / 2;
    auto yOf = [&](double v) { return (int)std::lround((double)top + (R - v) / (2.0 * R) * (double)(bot - top)); };
    auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f); return (int)p.h; };
    std::map<long long, int> idxOf; for (int i = std::max(0, v0 - 1); i <= v1; ++i) idxOf[(long long)times[i]] = i;
    const int spacing = v1 > v0 ? std::max(1, (xOf(v1) - xOf(v0)) / std::max(1, v1 - v0)) : 6;
    const int bw = std::max(2, spacing * 6 / 10);
    // the zero line
    setPen((COLOR)0x4B5563ul, 1, P_SOLID);
    { PNT a; a.set(0, 0.0f); a.h = trpC(pane.left); a.v = trpC(mid); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = trpC(pane.right); b.v = trpC(mid); b.drawLineTo(); }
    const double bx = bigX(S);
    const unsigned long cBull = S.set.c(trpset::K_COL_BULL), cBear = S.set.c(trpset::K_COL_BEAR);
    // the message line (top-left, below IRT's title row): the version + the most recent big trap of the session
    struct Box { int l, t, r, b; }; std::vector<Box> placed;
    FONT mf; mf.id = HELVETICA; mf.size = 10; mf.style = BOLD; setFont(mf);
    {
        const int y = top + 15; int x = pane.left + 4;
        const std::string ver = "Trapped v" TRP_VERSION;
        setTextColor((COLOR)0xE5E7EBul); { RCT r; const int w = (int)getTextWidth(ver.c_str(), -1); r.set(trpC(x), trpC(y), trpC(x + w + 2), trpC(y + 13)); r.drawText(ver.c_str(), false, false); x += w + 12; }
        const trp::Trap* last = nullptr;
        for (size_t k = 0; k < S.shown.size(); ++k) { const trp::Trap& T = S.shown[k]; if (T.sess == S.E.sess && T.ratio >= bx - 1e-9 && (!last || T.trT >= last->trT)) last = &T; }
        if (S.set.b(trpset::K_SHOW_MSG) && last) {
            Trapped* me = this; const std::string msg = trp::message(*last, [me](long long t) { return me->hm(t); });
            setTextColor((COLOR)(last->side == trp::SELLERS ? cBull : cBear));
            const int w = (int)getTextWidth(msg.c_str(), -1); RCT r; r.set(trpC(x), trpC(y), trpC(std::min((int)pane.right - 2, x + w + 2)), trpC(y + 13)); r.drawText(msg.c_str(), false, false);
            x += w;
        }
        Box mb; mb.l = pane.left; mb.t = y - 1; mb.r = x + 4; mb.b = y + 14; placed.push_back(mb);
    }
    // fuel lines first (behind the spikes), then spikes and labels in time order (an older label is never moved - lesson 49)
    for (size_t k = 0; k < S.shown.size() && S.set.b(trpset::K_SHOW_FUEL); ++k) {
        const trp::Trap& T = S.shown[k]; if (T.normal <= 0 || T.fuel.empty()) continue;
        const bool big = T.ratio >= bx - 1e-9; const unsigned long base = T.side == trp::SELLERS ? cBull : cBear;
        setPen(big ? (COLOR)base : muted(base), big ? 2 : 1, P_SOLID);
        bool started = false;
        for (size_t j = 0; j < T.fuel.size(); ++j) {
            if (T.fuel[j].t < T.trT) continue;                              // from the Tr candle on
            std::map<long long, int>::const_iterator it = idxOf.find(T.fuel[j].t); if (it == idxOf.end()) { started = false; continue; }
            PNT p; p.set(0, 0.0f); p.h = trpC(xOf(it->second)); p.v = trpC(yOf((double)T.sg() * T.fuel[j].fuel / (double)T.normal));
            if (!started) { p.setDrawPosition(); started = true; } else p.drawLineTo();
        }
    }
    FONT lf; lf.id = HELVETICA; lf.size = 10; lf.style = BOLD;
    for (size_t k = 0; k < S.shown.size(); ++k) {
        const trp::Trap& T = S.shown[k];
        std::map<long long, int>::const_iterator it = idxOf.find(T.trT); if (it == idxOf.end()) continue;
        const bool big = T.ratio >= bx - 1e-9; const unsigned long base = T.side == trp::SELLERS ? cBull : cBear;
        const COLOR col = big ? (COLOR)base : muted(base);
        const int x = xOf(it->second), y0 = mid, y1 = yOf((double)T.sg() * std::max(0.1, T.ratio));
        RCT r; r.set(trpC(x - bw / 2), trpC(std::min(y0, y1)), trpC(x - bw / 2 + bw), trpC(std::max(y0, y1) + 1)); r.draw(0, col, col, DRAW_OPAQUE, PAT_SOLID);
        HitBox hb; hb.l = x - std::max(bw / 2, 3); hb.r = x + std::max(bw / 2, 3); hb.t = std::min(y0, y1) - 2; hb.b = std::max(y0, y1) + 2; hb.idx = k; S.hits.push_back(hb);
        if (!big || !S.set.b(trpset::K_SHOW_LABELS)) continue;
        setFont(lf);
        const std::string lab = trp::spikeLabel(T); const int w = (int)getTextWidth(lab.c_str(), -1) + 4, h = 13;
        const int lt = T.side == trp::SELLERS ? y1 - h - 2 : y1 + 2;
        Box lb; lb.l = x - w / 2; lb.r = lb.l + w; lb.t = lt; lb.b = lt + h;
        if (lb.t <= top + 1 || lb.b >= bot - 1 || lb.l <= pane.left || lb.r >= pane.right) continue;
        bool clash = false; for (size_t q = 0; q < placed.size() && !clash; ++q) clash = !(lb.r + 2 < placed[q].l || lb.l > placed[q].r + 2 || lb.b + 2 < placed[q].t || lb.t > placed[q].b + 2);
        if (clash) continue;
        placed.push_back(lb);
        setTextColor(col); RCT tr; tr.set(trpC(lb.l), trpC(lb.t), trpC(lb.r), trpC(lb.b)); tr.drawText(lab.c_str(), true, false);
    }
    hoverBox(S, pane);
}
#endif

int Trapped::draw(void) {
    pump();
    TrpState* S = slot_.get(this, false);
    if (S) {
        try {
#ifdef TRP_MARKS
            renderMarks(*S);
#else
            renderPane(*S);
#endif
        } catch (...) {}
    }
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    Trapped* p = new Trapped();
    p->setArrayCount(1);
#ifdef TRP_MARKS
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE | VAP_REQUIRED);
    p->setDescription("LRA Trapped Marks v1.0.0 - Tr on the price chart. A candle swept one of the last 3 intact 1-bar pivots and closed back the other way: green Tr under it = sellers trapped (they sold the breakdown below the swing low, the candle closed green back above it, and closed 0.25 ATR above their average fill on that or the next candle) - their covering is buying still to come; red Tr over it = buyers trapped. Bigger and brighter at 2.0x the normal trap (the market's median over 20 sessions), muted when small. Decided once on closed candles, never repainted. Hover a Tr for lots, average, share, ratio, ticks underwater, pain and fuel left. Add lsTrapped below the chart for the spikes and the fuel line. Example: ES Oct 5 9:03, sellers trapped 3,140 lots below the 8:48 low.");
#else
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE | VAP_REQUIRED);    // its own pane, draws itself; mouse events even when not selected
    p->setDescription("LRA Trapped v1.0.0 - trapped traders and their fuel. A spike on the Tr candle = the lots trapped by a failed sweep vs the normal trap (the market's median over 20 sessions): green up = sellers trapped below a swing low (buying to come as they cover), red down = buyers trapped above a swing high. Bright at 2.0x+, muted when small; '3,140 \xB7 7.1x' on big spikes. The thin line after a spike = fuel left: the trapped lots still offside, burned by their estimated covering; it ends when they are freed (a close back beyond their average) or the fuel is spent. Decided once on closed candles from the chart's own volume at price, recorded, never repainted. Hover a spike for the numbers. Add lsTrappedMarks to the price chart for the Tr letters. Example: ES Oct 5 9:03, sellers trapped 3,140 lots (7.1x) below the 8:48 low.");
#endif
    p->setVersion("1.0.0");   // TRP_VERSION
    return p;
}
