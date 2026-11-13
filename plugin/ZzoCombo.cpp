/********************************************************************************
 *  ZzoCombo.cpp  --  Investor/RT RTX extension  lsZzoCombo  "Cob Reader" (was "ZZO Combo")  2.9.4  (2026-11-12)
 *
 *  2.9.4 (Rassul 2026-11-12 20:37 "the cob reader has AB and other signals, it should only have cobs and lvp"): the wave tags Cmx / AR / ST / Ab
 *  are no longer drawn (status file only); the pane shows Cob and Lvp, the wave counts and the volume ratios.
 *  2.9.3 (Rassul 2026-11-12 18:19 "this cannot be a cob, there is not even a count, and its a small wave", 18:21 "3. yeah lets go with this"):
 *  Cob2 is no longer shown anywhere (pane tag, its Lvp, read box, SR / CB badges); it stays on the record for the nightly review.
 *  2.9.2 (Rassul 2026-11-12 18:12 "the bearish SR tab should be on top of the candle"): price pane - SR over the candle for a bearish Cob.
 *  2.9.1 (Rassul 2026-11-12 18:02 "work on the warn because the 1st hour of the RTH initiates most cobs", 18:07 "go"): five warnings -
 *  Room merged into Deep (gave back > 0.40); 1st only for a Cob completed in the first 30 minutes, or in the first hour with the hourly not turned.
 *  2.9.0 (Rassul 2026-11-12 17:43 "a line saying warnings which can forecast whether this cob will be a failure"; 17:58 "like Warn (3)"): the
 *  box row "Warn (n)" with the codes Rng 1st Deep Q Hr Room ("none" without any), red at 3 or more.
 *  2.8.2 (Rassul 2026-11-12 16:20 "independent review ... Do it and fix"): the waves to draw are rebuilt only when a bar changed (was a copy
 *  of every bar on each repaint); the day-range history skips a session whose RTH did not start at the open.
 *  2.8.1 (Rassul 2026-11-12 16:18 "For quality give me a percentage instead"): the box row "Q / Rng" shows the score as a percentage of
 *  its 6 points (4 of 6 = "67%").
 *  2.8.0 (Rassul 2026-11-12 15:21 "I want it now. Build all"; 15:32 "I wanted to call this indicator Cob Reader like the reversal reader": the
 *  pane title is "Cob Reader 2.8.0"; the extension keeps its name lsZzoCombo so the charts keep it): the day range used at the Cob (RTH range so far / the median RTH range of
 *  the last 10 sessions) - read box row "Q / Rng" ("4/6 / 0.9") and Hold moved by it (-12 .. +16 points, ZzoCobFinder.h HOLD_RNG).
 *  2.7.3 (Rassul 2026-11-12 13:04 "it really should be the biggest green bar in at least 35 green bars to even qualify"): a Cob bar is
 *    only the biggest (range) or heaviest (volume) of the 35+ up bars before it (finder 1.5.1) - "CB35" / "CB99"; no plain "CB".
 *  2.7.2 (Rassul 2026-11-12 13:02 "Cb and sr should only be on the price chart"): the pane no longer draws the CB / SR badges (still
 *    counted in its status file); lsZzoComboMarks draws them on the candles.
 *  2.7.1 (Rassul 2026-11-12 13:00 "the stoprun badge goes under the bar"): lsZzoComboMarks draws SR always UNDER its candle (red under
 *    a high's stop run too) and CB always over its candle. The pane is unchanged.
 *  2.7.0 (Rassul 2026-11-12 12:51 "usually the cob [bar] is the biggest green bar ... in over 35+ or even 99 green bars", 12:53 "you can
 *    add the stop run badge too like you showed me in the mock up"): + "CB35" / "CB99" (the Cob bar's rank) and + "SR" over the stop-run
 *    bar's column (finder 1.5.0). C / Q lines end with the Cob bar time, its rank, the SR bar time and level - lsZzoComboMarks reads
 *    them to draw SR / CB on the price pane.
 *  2.6.0 (Rassul 2026-11-12 12:43 "a cob bar ... I want you to label it in the cob", 12:47 "I'll go with your recommendation"): + "CB" over
 *    the Cob bar's column inside a Cob / Cob2 wave (finder 1.4.0; label and log only, no quality point). C / Q lines end with its bar time.
 *  2.5.1 (Rassul 2026-11-12 12:26 "get rid of count because i already see it on the wave, and keep Q and label it like 4/6. also the
 *    8ATR or 14 or more candles are excessive ... even 5 or more up candles bigger than ATR is excessive"): Q no longer scores the wave's
 *    size or length (finder 1.3.1: dips, high-end closes, closes above the prior high, the first 3 bars, efficiency, a run of 4); the
 *    box row is "Q 4/6" (no count).
 *  2.5.0 (Rassul 2026-11-12 11:45 "score the quality of the bars in the cob", 12:09 "ok. go", 12:10 "also check for high end closes"):
 *    + the quality score Q (0-6) of the wave's own bars (finder 1.3.0); the box row is now "Cnt / Q" and the Hold row uses Q, Hr and
 *    the Lvp. Opens are read here (barOpen) for it. C and Q record lines end with H / - and the score.
 *  2.4.0 (Rassul 2026-11-12 11:25 "Cob2 is fine", 11:27 "I want a read box on the left of the indicator pane", 11:35 "try to reuse rows
 *    like lvp/hrly", 11:38 "in the Lvl you can add SR+", 11:40 "build and mockup", 11:42 "there should be no requirement for giveback
 *    because cob can test the origin bar"): + the "Cob2" tag (the quiet turn - no count, no level needed), + the READ BOX at the left of
 *    the pane (Cob | Lvl with SR+ | Cnt / Retr | Lvp / Hr | Hold), + the hourly turn (Hr), and the Cob's 0.46 retracement limit is
 *    removed (finder 1.2.0 - ZzoCobFinder.h has every rule and what was measured). + record line Q (Cob2); C and Q end with H / -.
 *  2.3.1 (Rassul 2026-11-12 09:32 "Add prior day poc vah Val also", 09:33 "Go"): the prior full session's POC / VAH / VAL count as key
 *    levels for a Cob, swept from either side (finder 1.1.0; tested first: Cobs qualifying only on them held 76%, 131 cases). Logic only.
 *  2.3.0 (Rassul 2026-11-12 "the wave zzo would be upgraded to show the waves with count labels but also identify cobs", 08:56 "we are
 *    just upgrading the zzo wave to now be the cob finder .. do not change anything beyond the indicator", 09:02 "go"): the CoB FINDER.
 *    "Cob" on a wave that starts at / near the RTH low or high, after a key-level sweep the completing bar closed back over, with a ZZO
 *    count of 10+ and <= 0.46 given back; "Lvp" on the low-volume pullback wave after it (ZzoCobFinder.h has the rule and what was
 *    measured). Decided once on the bar that completes the wave; never repainted. The old "CoB after a sweep" tag is logged only.
 *    Ramps, ratios, counts, Cmx / AR / ST / Ab unchanged. The engine header (ZzoDeltaLogic.h) is NOT changed: closes are read here.
 *    + FILE lsFlexLevels\ZzoCombo\cob-<MKT>-<spb>.csv: one line per decision (C Cob, Q Cob2, L Lvp, W watch = logged only), for the nightly.
 *  2.1.1 (Rassul 2026-11-03 "build it all", ref picture gc_zz.png): the wave COUNT is drawn INSIDE the wave's bars - centred on the
 *    second-to-last bar, just below its top, white with a 1 px black outline; a bar too short for the text gets it just above its top.
 *    The ratio label is unchanged (centred above the last bar). View only (ZzoComboView.h); waves, counts, tags unchanged.
 *  2.1.0 (Rassul 2026-11-03 13:59-14:03 CT): waves from the 3-bar High/Low ZigZag by default ("make it 3bar using high and low by
 *    default" = IRT's ZIGZAG Min Price Change 3 bars, as lsWavePOC); the count compares the wave's volume ratio (the ramp height) with
 *    prior waves of its own colour only; "weak" not drawn ("remove weak"); the count one bar further left. No settings (SS_SAFE_DIALOG):
 *    1-bar vs 3-bar is the compiled constant zzd::COMBO_REV_BARS.
 *  2.0.1 (09:20 CT, Rassul's ES chart): labels / tags drawn after every column (the next wave covered the "weak" tag).
 *  2.0.0 (Rassul 2026-11-03 07:40-08:49 CT, "ZZO Combo Mockup" v19 + "build"): the default ZZO indicator - one row in its own pane:
 *    volume ramp per wave (running volume / the median of the last 5 same-way waves), the 2.0x+ ratio above the peak and the count
 *    ("biggest red in the last 12 red waves", 0 hidden, 50+ cap) right-justified in the top quarter, tags at the bottom centre of the wave
 *    (Cmx / AR / ST, CoB / weak after a key-level sweep, "Ab 4.9" = absorption with a heavy extreme + its effort / result - ES only until
 *    the nightly proves it elsewhere). Delta is computed (VAP) but not drawn: the tests showed only Ab with a heavy extreme beats an
 *    ordinary wave (ES 49% vs 33% reached 2R, both halves). The forming wave grows with every tick (darker, white top edge); tags only
 *    on completed waves. Fits any pane height (labels drop below 70 / 50 px). No settings, no hovers. Rules: ZzoComboLogic.h +
 *    ZzoComboView.h (tests_zzodelta/test_zzocombo.cpp). Thresholds per market: ZzoParams.h (nightly lra.zzo_score).
 *  FILES  lsFlexLevels\ZzoCombo.status-<MKT>-<spb>.txt  version, rules, the last 30 waves (on change / 30 s)
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>     // BEFORE irtsdk.h
#endif
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "ZzoComboView.h"
#include "HostSlot.h"
#include "SettingsGuard.h"   // SS_SAFE_DIALOG: the chart-less Add / settings copy never faults
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include <sys/stat.h>

#ifdef ZC_MARKS
#define ZD_NAME "ZzoComboMarks"   // (2.7.0) lsZzoComboMarks = this file + ZC_MARKS: the price-pane overlay (SR / CB badges on the candles)
#else
#define ZD_NAME "ZzoCombo"
#endif
#if defined(_WIN32)
#define ZD_SEP "\\"
#else
#define ZD_SEP "/"
#endif

static long long zdMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string zdFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + ZD_SEP "InvestorRT" ZD_SEP "rtx" ZD_SEP "lsFlexLevels" : std::string(); }
static void zdMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool zdPublish(const std::string& path, const std::string& text) {
    const std::string tmp = path + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false; f.write(text.data(), (std::streamsize)text.size()); f.flush(); if (!f.good()) { f.close(); std::remove(tmp.c_str()); return false; } }
#if defined(_WIN32)
    if (::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    std::remove(tmp.c_str()); return false;
#else
    std::remove(path.c_str()); return std::rename(tmp.c_str(), path.c_str()) == 0;
#endif
}

struct ZState {
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; bool keyed = false;
    zzd::Engine E; std::vector<RTDATE> raw; int off = 0;
    bool vapFault = false;
    bool haveForm = false; zzd::Bar formBar;
    std::vector<zzd::Wave> vWaves; long long vSig[12] = { 0 }; bool vOk = false;   // (2.8.2) the waves to draw, rebuilt only when a bar or the forming bar changed
    std::string statusPath; long long statusMs = -1; std::string statusBody;
    zzd::ComboScene last;
    zcf::Finder F; long long lastWaveT = -1;                     // (2.3.0) the CoB finder + the pivot time of the last wave it judged
    std::string recPath; std::set<std::string> recSeen; bool recLoaded = false;
};

class ZzoCombo : public cppExtension {
public:
    SS_SAFE_DIALOG(ZzoCombo, "ZzoCombo")
    ZzoCombo() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }          // no settings
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<ZState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(ZState& S);
    void feed(ZState& S);
    int barDelta(ZState& S, int chartBar, long long& d, long long hT = 0, long long lT = 0, long long* xh = nullptr, long long* xl = nullptr);
    long long localSec(RTDATE d);
    void status(ZState& S, long long now);
    void record(ZState& S);
    static int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    static int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<ZzoCombo*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<ZzoCombo*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { ZzoCombo* me = static_cast<ZzoCombo*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }   // bars + VAP read here only

// SEH-guarded VAP reads (as lsCandleAbsorb / lsWavePOC)
int ZzoCombo::readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || !stats || bar < 0) return -2;
        std::memset(stats, 0, sizeof(*stats));
        if (vap->getBarStatistics(bar, *stats) != RTX_OK) return -2;
        if (stats->prices < 0) return -2;
        return stats->prices;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
int ZzoCombo::readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rws, int count) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || !rws || bar < 0 || count <= 0) return -2;
        for (int i = 0; i < count; ++i) { std::memset(&rws[i], 0, sizeof(VOLPROFILE)); if (vap->getVolumeProfile(bar, i, rws[i]) != RTX_OK) return -2; }
        return count;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// one bar's delta (sum of buy - sell over its VAP rows): 1 = read, 0 = no VAP (delta 0, counted), -1 = fault (never read again)
int ZzoCombo::barDelta(ZState& S, int chartBar, long long& d, long long hT, long long lT, long long* xh, long long* xl) {
    d = 0; if (xh) *xh = 0; if (xl) *xl = 0;
    if (S.vapFault) return -1;
    RTARRAYP vap(barVolumeProfile);
    if (vap.count <= chartBar) return 0;
    BARSTATISTICS st; const int prices = readStatistics(&vap, chartBar, &st);
    if (prices == -1) { S.vapFault = true; return -1; }
    if (prices <= 0 || prices > 100000) return 0;
    std::vector<VOLPROFILE> rv((size_t)prices);
    const int got = readPrices(&vap, chartBar, &rv[0], prices);
    if (got == -1) { S.vapFault = true; return -1; }
    if (got < 0) return 0;
    bool any = false;
    for (const VOLPROFILE& v : rv) {
        if (!std::isfinite((double)v.price) || v.buyVolume < 0 || v.sellVolume < 0) continue;
        if (v.buyVolume + v.sellVolume <= 0) continue;
        d += (long long)v.buyVolume - (long long)v.sellVolume; any = true;
        long long pt = 0;   // (2.0.0) the volume in the bar's top / bottom 2 ticks (Ab heavy extreme)
        if ((xh || xl) && zzd::toTicks((double)v.price, S.tick, pt)) { const long long vv = (long long)v.buyVolume + (long long)v.sellVolume;
            if (xh && (pt == hT || pt == hT - 1)) *xh += vv;
            if (xl && (pt == lT || pt == lT + 1)) *xl += vv; }
    }
    return any ? 1 : 0;
}

long long ZzoCombo::localSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, dd = (unsigned)t.tm_mday;
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

bool ZzoCombo::identify(ZState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) return false;   // SYM_TICKINCR only
    if (mkt.empty()) return false;
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey) {
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true;
        S.E.reset(); S.E.revBars = zzd::COMBO_REV_BARS; S.E.setMarket(mkt, spb); S.raw.clear(); S.off = 0; S.haveForm = false;   // (2.1.0) 3-bar ZigZag
        S.F.reset(); S.F.setMarket(mkt, spb); S.lastWaveT = -1; S.recSeen.clear(); S.recLoaded = false;   // (2.3.0)
        const std::string d = zdFlex(); const std::string key = mkt + "-" + std::to_string(spb);
        S.statusPath = d.empty() ? std::string() : d + ZD_SEP ZD_NAME ".status-" + key + ".txt";
#ifdef ZC_MARKS
        S.recPath.clear();   // the overlay never writes the record (lsZzoCombo does)
#else
        S.recPath = d.empty() ? std::string() : d + ZD_SEP ZD_NAME ZD_SEP "cob-" + key + ".csv";
#endif
    }
    return true;
}

// closed bars in order, each once; a changed history (first stamp, fewer bars, a stamp moved) = replay from the start
void ZzoCombo::feed(ZState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime), vol(barVolume); RTARRAY hi(barHigh), lo(barLow);
    if ((long)times.count < n || (long)hi.count < n || (long)lo.count < n) return;
    const int closed = (int)n - 1;
    const int fed = (int)S.raw.size();
    if (fed > 0) { const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) { S.E.reset(); S.E.setMarket(S.mkt, S.spb); S.raw.clear(); S.F.reset(); S.F.setMarket(S.mkt, S.spb); S.lastWaveT = -1; } }
    if (S.raw.empty()) S.off = std::max(0, closed - zzd::MAX_BARS + zzd::TRIM);
    const bool haveVol = (long)vol.count >= n;
    RTARRAY clo(barClose); const bool haveClose = (long)clo.count >= n;
    RTARRAY opn(barOpen); const bool haveOpen = (long)opn.count >= n;   // (2.5.0) opens for the quality score   // (2.3.0) closes for the CoB finder
    auto bar = [&](int i, zzd::Bar& b) {
        b.t = localSec((RTDATE)times[i]);
        b.vol = haveVol ? std::max(0LL, (long long)vol[i]) : 0;   // (1.2.0) bar volume
        if (!zzd::toTicks(hi[i], S.tick, b.h) || !zzd::toTicks(lo[i], S.tick, b.l)) { b.h = b.l = S.E.B.empty() ? 0 : S.E.B.back().h; }
        if (b.h < b.l) std::swap(b.h, b.l);
        long long d = 0, xh = 0, xl = 0; b.vap = barDelta(S, i, d, b.h, b.l, &xh, &xl) > 0; b.dlt = d; b.xh = xh; b.xl = xl; b.xok = b.vap;
    };
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        zzd::Bar b; bar(i, b);
        S.E.addBar(b); S.raw.push_back((RTDATE)times[i]);
        {   // (2.3.0) the finder's copy of the bar, aligned with the engine's
            long long c = 0; if (!haveClose || !zzd::toTicks(clo[i], S.tick, c)) c = b.h;
            if (c > b.h) c = b.h;
            if (c < b.l) c = b.l;
            long long o = 0; if (!haveOpen || !zzd::toTicks(opn[i], S.tick, o)) o = LLONG_MIN;   // not given: the finder uses the previous close
            S.F.addBar(b, c, o);
        }
        if (S.E.trimmed > 0) { S.F.trim(S.E.trimmed); S.off += (int)S.E.trimmed; S.raw.erase(S.raw.begin(), S.raw.begin() + std::min<long long>(S.E.trimmed, (long long)S.raw.size())); S.E.trimmed = 0; }
        if (!S.E.W.empty() && S.F.F.size() == S.E.B.size()) {   // a wave completed on this bar: judge it once
            const zzd::Wave& w = S.E.W.back();
            if (w.s >= 0 && w.e >= 0 && (size_t)w.e < S.F.F.size()) { const long long tw = S.F.F[(size_t)w.e].t * 2 + (w.dir > 0 ? 1 : 0);
                if (tw != S.lastWaveT) { S.lastWaveT = tw; S.F.onWave(S.E.W); } }
        }
    }
    zzd::Bar fb; bar(closed, fb); fb.seg = S.E.segOf(fb.t); S.formBar = fb; S.haveForm = true;   // the forming bar, re-read every calc
    S.F.prune(); record(S);
}

// (2.3.0) the finder's new decisions -> lsFlexLevels\ZzoCombo\cob-<MKT>-<spb>.csv, each once (key = kind | wave time | direction); a line
// counts as written only after a flushed good append
void ZzoCombo::record(ZState& S) {
    if (S.F.out.empty()) return;
    std::vector<std::string> lines; lines.swap(S.F.out);
    if (S.recPath.empty()) return;
    auto keyOf = [](const std::string& ln) { std::vector<std::string> f; size_t a = 0; for (;;) { const size_t b = ln.find('|', a); f.push_back(ln.substr(a, b == std::string::npos ? b : b - a)); if (b == std::string::npos) break; a = b + 1; }
        return f.size() >= 5 ? f[0] + "|" + f[2] + "|" + f[4] : std::string(); };
    if (!S.recLoaded) {
        S.recLoaded = true;
        std::ifstream f(S.recPath.c_str(), std::ios::binary);
        if (f) { f.seekg(0, std::ios::end); const long long sz = (long long)f.tellg(); const long long cap = 512 * 1024; f.seekg(sz > cap ? sz - cap : 0, std::ios::beg);
            std::string ln; if (sz > cap) std::getline(f, ln);
            while (std::getline(f, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); const std::string k = keyOf(ln); if (!k.empty()) S.recSeen.insert(k); } }
    }
    std::string body; std::vector<std::string> keys;
    for (const std::string& ln : lines) { const std::string k = keyOf(ln); if (k.empty() || S.recSeen.count(k)) continue; bool dup = false; for (const std::string& q : keys) if (q == k) { dup = true; break; } if (dup) continue; body += ln + "\n"; keys.push_back(k); }
    if (body.empty()) return;
    zdMkdir(zdFlex()); zdMkdir(zdFlex() + ZD_SEP ZD_NAME);
    std::ofstream o(S.recPath.c_str(), std::ios::binary | std::ios::app);
    if (!o) return;
    o.write(body.data(), (std::streamsize)body.size()); o.flush();
    if (o.good()) for (const std::string& k : keys) S.recSeen.insert(k);
}

void ZzoCombo::status(ZState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;
    const std::string body = zzd::comboStatus(S.E, S.mkt, S.spb, S.vapFault ? "FAULT - remove and re-add the indicator" : "ok") +
        "DRAWN,columns " + std::to_string(S.last.cols) + ",ratios " + std::to_string(S.last.ratios) + ",counts " + std::to_string(S.last.counts) + ",Ab " + std::to_string(S.last.ab) +
        ",Cmx " + std::to_string(S.last.cmx) + ",AR " + std::to_string(S.last.ar) + ",ST " + std::to_string(S.last.st) + ",Cob " + std::to_string(S.last.cob) + ",Lvp " + std::to_string(S.last.lvp) + ",Cob2 " + std::to_string(S.last.cob2) + ",CB " + std::to_string(S.last.cb) + ",SR " + std::to_string(S.last.sr) + ",box rows " + std::to_string(S.last.boxRows) + "\n" + S.F.statusText();
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    S.statusMs = now; S.statusBody = body;
    zdMkdir(zdFlex()); zdPublish(S.statusPath, body);
}

void ZzoCombo::pump() {
    ZState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); status(*S, zdMs());
}

static short zdC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

int ZzoCombo::draw(void) {
    ZState* S = slot_.get(this, false);
    if (!S || S->E.B.empty()) return RTX_OK;   // draw never reads bars / VAP
    try {
        const long n = getBarCount(); if (n < 2) return RTX_OK;
#ifdef ZC_MARKS
        {   // (2.7.0) the price-pane overlay: "SR" under the stop-run candle of a bullish Cob / over it for a bearish one (2.9.2) and "CB" / "CB35" / "CB99" OVER the Cob bar, both in
            // the Cob's colour (2.7.1). Decided by the same finder as the pane; nothing else is drawn.
            RCT pane; pane.getPaneRect(false);
            int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
            std::map<long long, int> sr; std::map<long long, std::pair<int, int>> cb;
            for (const auto& kv : S->F.tags) { const zcf::Tag& t = kv.second; const int d = t.cob ? t.cob : zcf::SHOW_COB2 ? t.cob2 : 0; if (!d) continue; if (t.tSr) sr[t.tSr] = d; if (t.tCb) cb[t.tCb] = std::make_pair(d, t.cbRank); }
            if (sr.empty() && cb.empty()) return RTX_OK;
            FONT fb; fb.id = HELVETICA; fb.size = 9; fb.style = BOLD; setFont(fb); const zzd::Colours C; const int TH = 12;
            const int L = pane.left, T = pane.top, R = pane.right, Bt = pane.bottom; int drawn = 0;
            auto badge = [&](const std::string& txt, int xc, int yEdge, bool below, unsigned long col) {
                const int tw = (int)getTextWidth(txt.c_str(), -1) + 4; const int x0 = xc - tw / 2, y0 = below ? yEdge + 4 : yEdge - 4 - TH, y1 = y0 + TH;
                if (x0 < L || x0 + tw > R || y0 < T || y1 > Bt) return;
                RCT r; r.set(zdC(x0), zdC(y0), zdC(x0 + tw + 1), zdC(y1 + 1)); r.draw(0, (COLOR)col, (COLOR)col, DRAW_OPAQUE, PAT_SOLID);
                setTextColor((COLOR)0xFFFFFF); RCT q; q.set(zdC(x0 + 2), zdC(y0), zdC(x0 + tw), zdC(y1)); q.drawText(txt.c_str(), false, false); ++drawn; };
            for (int i = std::max(vs, S->off); i <= ve; ++i) {
                const int j = i - S->off; if (j < 0 || j >= (int)S->F.F.size()) continue;
                const zcf::FBar& b = S->F.F[(size_t)j]; const auto a = sr.find(b.t); const auto c = cb.find(b.t); if (a == sr.end() && c == cb.end()) continue;
                PNT p; p.set(i, 0.0f, kBarCenter); const int yh = (int)getVerticalDrawPosition((float)((double)b.h * S->tick)), yl = (int)getVerticalDrawPosition((float)((double)b.l * S->tick));
                if (a != sr.end()) { if (a->second > 0) badge("SR", (int)p.h, yl, true, C.up); else badge("SR", (int)p.h, yh - (c != cb.end() ? TH + 2 : 0), false, C.down); }   // (2.9.2, Rassul 18:12 "the bearish SR tab should be on top of the candle") a bullish Cob's stop run under its candle (2.7.1), a bearish one over it - above the CB when both sit on one candle
                if (c != cb.end()) { const int d = c->second.first, rk = c->second.second; badge(rk >= 99 ? "CB99" : "CB40",   /* (2.9.5) a Cob bar always beats 40+ same-colour bars */ (int)p.h, yh, false, d > 0 ? C.up : C.down); }   // (2.7.1) always over the candle
            }
            S->last = zzd::ComboScene(); S->last.sr = (int)sr.size(); S->last.cb = (int)cb.size(); S->last.cols = drawn;
            return RTX_OK;
        }
#endif
        RCT pane; pane.getPaneRect(false);
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
        zzd::View v; v.l = pane.left; v.t = pane.top; v.r = pane.right; v.b = pane.bottom;
        v.first = vs - S->off; v.last = ve - S->off; v.bw = std::max(1, (int)getPixelsPerBar());
        const int off = S->off;
        v.X = [this, off](int j) { PNT p; p.set(j + off, 0.0f, kBarCenter); return (double)p.h; };
        FONT fb; fb.id = HELVETICA; fb.size = 9; fb.style = BOLD;
        v.textW = [this, &fb](const std::string& s) { setFont(fb); return (int)getTextWidth(s.c_str(), -1); };
        v.textH = 12;
        { const zzd::Bar& fb2 = S->formBar; const long long sig[12] = { (long long)S->E.B.size(), S->E.B.empty() ? 0 : S->E.B.back().t, (long long)S->E.W.size(), S->haveForm ? 1 : 0, fb2.t, fb2.h, fb2.l, fb2.vol, fb2.dlt, fb2.xh, fb2.xl, fb2.seg };
          if (!S->vOk || std::memcmp(sig, S->vSig, sizeof sig) != 0) { S->vWaves = S->E.view(S->haveForm ? &S->formBar : nullptr); std::memcpy(S->vSig, sig, sizeof sig); S->vOk = true; } }
        const std::vector<zzd::Wave>& waves = S->vWaves;
        const bool noFoot = !S->E.B.empty() && S->E.noVap >= (long long)S->E.B.size();   // no footprint on this chart: volume still works, Ab cannot
        const std::string head = std::string("Cob Reader ") + zcf::COMBO_VERSION + (noFoot ? "  no footprint - Ab off" : "");
        S->last = zzd::layoutCombo(S->E, waves, S->haveForm ? S->formBar.vol : 0, v, head, &S->F);
        const int L = pane.left, T = pane.top, R = pane.right, Bt = pane.bottom;
        for (const zzd::Prim& p : S->last.p) {
            if (p.k == zzd::Prim::FILL) {
                const int x0 = std::max(L, std::min(p.x0, p.x1)), x1 = std::min(R, std::max(p.x0, p.x1)), y0 = std::max(T, std::min(p.y0, p.y1)), y1 = std::min(Bt, std::max(p.y0, p.y1));
                if (x1 < x0 || y1 < y0) continue;
                RCT r; r.set(zdC(x0), zdC(y0), zdC(x1 + 1), zdC(y1 + 1)); r.draw(0, (COLOR)p.col, (COLOR)p.col, DRAW_OPAQUE, PAT_SOLID);
            } else {
                if (p.y0 < T || p.y1 > Bt || p.x0 < L || p.x1 > R) continue;
                setFont(fb); setTextColor((COLOR)p.col);
                RCT r; r.set(zdC(p.x0), zdC(p.y0), zdC(p.x1 + 2), zdC(p.y1)); r.drawText(p.text.c_str(), false, false);
            }
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    ZzoCombo* p = new ZzoCombo();
    p->setArrayCount(1);
#ifdef ZC_MARKS
    p->setFlags(POST_DRAWING | OVERLAY | INSTRUMENT_SCALE | NO_UI | VAP_REQUIRED);   // on the price pane, on the instrument's scale; no hovers
#else
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED);   // its own pane, draws itself; no mouse tracking (no hovers)
#endif
    p->setDescription("LRA Cob Reader v2.9.5 (was ZZO Combo) - 3-bar High/Low ZigZag waves: volume ramp vs the last 5 same-way waves, 2.0x+ ratio and count, Cmx / AR / ST, Ab; CoB finder: Cob = a count 10+ wave from the RTH low / high after a key-level sweep that closed back, Lvp = its low-volume pullback. No settings.");
    p->setVersion("2.9.5");   // = zcf::COMBO_VERSION (gex-build reads this literal)
    return p;
}
