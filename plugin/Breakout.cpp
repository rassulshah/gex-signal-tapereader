/********************************************************************************
 *  Breakout.cpp  --  Investor/RT RTX extension  lsBreakout  "BO"  1.5.0  (2026-11-04)
 *  1.14.0 (2026-11-09, Rassul 16:34 CT "Ok build" - the entry / exit liquidity badges): lsBreakout (the 3-bar build only) posts its drawn,
 *        decided pullback labels as SignalBoard fact kind E (EntryFacts.h 1.0.0: decision bar, side, PB POC, stop, wave target, Pb / Pboc / Pdoc)
 *        for lsDealerProfile / lsLimitProfile 2.56.0 (EntryBadge.h). Hidden (Lvp) and provisional labels never; no rule or drawing changed. Status ENTRY_FACTS.
 *  1.13.0 (2026-11-09, Rassul 14:09-14:12 CT "pdoc where d represent the developing poc"): PDOC. A Pb / no-BO Rly pullback whose pivot
 *        bar straddles the pullback's DEVELOPING volume POC (lsWavePOC's wave PB POC: >= 38.2% retrace, its value at the decision bar) and
 *        NOT the wave POC is drawn "Pdoc" (both -> Pboc). Same no-repaint rule (no VAP yet: nothing, then drawn once). Records BPB + pdoc /
 *        pb_poc, BPD| (Rly Pdoc). Status PDOC. Spec NOTES-BO1130.md.
 *  1.12.0 (2026-11-09, Rassul 12:22 CT "if a pb tags the wave poc ... show pboc", mockup approved): PBOC. lsBreakout reads IRT's VAP
 *        (VAP_REQUIRED, lsWavePOC's SEH-guarded reader) and computes each wave's volume POC with lsWavePOC's own rule (WavePOCLogic.h
 *        rowTicksFor / buildRows / median2 / pickNodes, natively - no file read). (1) A Pb whose pivot bar straddles its BO / BD wave's POC
 *        (BO: low <= POC < high; BD: high >= POC > low) is drawn "Pboc" (same place / colour). (2) A fired Rly wave WITHOUT a BO / BD of its
 *        direction: its first 2 confirmed pullback pivots (none after a trade beyond the origin or the next BO / BD / Rly) are drawn "Pboc"
 *        when the POC is at / below the wave's 50% (Rly down: at / above) and the pivot bar straddles it; otherwise no label. A pivot whose
 *        wave has no VAP yet draws NOTHING until it has, then Pb or Pboc once (never switched). An Lvp still hides it. Records: BPB| + boc,
 *        wave_poc (16 fields; written once boc is known); BPO| (Rly Pboc) and BRO| (the Rev rule's thrust off a Rly Pboc - recorded, not
 *        drawn) in the -pb- file. Pb / Rev decisions unchanged. Status lines PBOC / RLY_PBOC.
 *  1.9.1 (2026-11-06 12:15 CT, Rassul "lets simplify the pb rev .. remove the stop line, t1 and t2 ... 1.2*ATR(13)"): Rev = range >= 1.2 x ATR(13)
 *        only; the biggest-of-5 alternative and the Stop / T1 / T2 lines removed; BRV| 14 fields.
 *  1.9.0 (2026-11-06, Rassul "PB Rev ... a thrust after a pullback", mockup pb-rev v0.1, "yes build"): + "Rev" (BreakoutLogic.h 1.9.0
 *        revPass / layout kind 4): after a Pb, the first green bar (BD: red) from the pivot bar on whose low is below the pivot bar's high
 *        (BD: high above its low) with range >= 1.2 x ATR(14) or bigger than each of the last 5 green (red) bars; one per wave; green
 *        below / red above the bar, + (10:14 "lets add 1.618 as the 2nd and final target") lines Stop / Rly (T1) / 1.618 (T2) from the Rev bar
 *        (bko::revLines, Scene::lines, drawn under the letters); records BRV| (20 fields) in the -pb- file; status KEPT / DRAWN carry "Rev". No new includes, no settings.
 *  1.5.0 (2026-11-04 after 15:00 CT, Rassul approved the BreakIn mockup v0.3 and said "build"): + "BI" (BreakIn = a FAILED BO / BD):
 *        a red candle with breakout range (range >= the average of the previous 10 + volume or range up) that closes back BELOW the BO
 *        pivot within the attempt candle + 10 (a fired BO: the 10 after its follow-through) = BI DOWN, red "BI" just above its high; BI UP
 *        mirrors a failed BD (green, below the low). One per pivot, decided at the close, never repainted; a BI while the BO / BD still
 *        waits for its follow-through cancels it (not drawn, no Pb, record fired 0 / bi_cancel). BreakoutLogic.h 1.5.0 (biPass / layout);
 *        host: version literals, description, status RECORDS "bi 1". Records: BBI| lines in ...-breakouts-<daykey>.csv. No new includes.
 *  1.4.0 (2026-11-04, Rassul "BO 5PM - yes"): the BO pivot ZigZag carries across the 17:00 CT session break (BreakoutLogic.h 1.4.0,
 *        bba::swingBreaksPx carry) - no host change; status RECORDS line and record files add "zz_carry 1".
 *  1.3.0 (2026-11-03 evening, Rassul removed lsBigBidAsk from his charts at 19:45 CT and said "do it"):
 *    RECORDS - lsBreakout writes the BO / BD, Rly and Pb decision records from its own 3-bar decisions (BreakoutLogic.h recPlan /
 *      recMerge; line layout = lsBigBidAsk's bba::boLine / rly::line + a BPB| Pb line), one line per decision, deduped by key against
 *      the file before every append (restart / recompute / two charts never repeat a line), last 3 sessions:
 *        lsFlexLevels\Breakout\<MKT>-<SYM>-<spb>-breakouts-<daykey>.csv, lsFlexLevels\Breakout\Rly\<MKT>-<SYM>-<spb>-<daykey>.csv,
 *        lsFlexLevels\Breakout\<MKT>-<SYM>-<spb>-pb-<daykey>.csv  (first line "# lsBreakout 1.3.0 | ... | zz_rev_bars 3 | ft_tie 1").
 *    FOLLOW-THROUGH TIE - the follow-through close may equal the BO / BD candle's close (bba::boWaves ftTie; lsBigBidAsk unchanged).
 *  1.2.0 (2026-11-03, Rassul "go" after the 1-bar vs 3-bar week check): BO / BD pivots + wave and Rly waves on the 3-bar High / Low
 *         ZigZag (the Pb / lsZzoCombo 2.1.0 turn rule, BreakoutLogic.h ZZ_REV_BARS); lsBigBidAsk keeps 1-bar. Host code unchanged.
 *
 *  1.1.1 (2026-11-03, Rassul's screenshot: the green "Pb" sat on the zigzag wave-time labels "9m" / "16m" at the pivot's wick): Pb moved
 *    TWO rows out - below a pivot low its top = Y(low) + 3 + 32, above a pivot high its bottom = Y(high) - 3 - 32 - clear of the wave-time
 *    label row (at the wick) and lsCandleAbsorb's A / Ae row (one row out, +16 .. +30). Same collision rule; BO / BD / Rly unchanged.
 *  1.1.0 (2026-11-03, Rassul approved the Pb mockup - ES Oct 30 - and said "build"): + "Pb", a pullback that TESTS the BO / BD candle:
 *    every confirmed 3-bar High / Low ZigZag pivot LOW after a BO whose bar overlaps the BO candle gets a green "Pb" just below its low
 *    (pivot HIGHs after a BD: red, just above the high), until the next BO / BD or price trades beyond the BO / BD wave's origin pivot.
 *    Same small font as BO / BD, never on another letter, drawn when the pivot is confirmed and never changed (BreakoutLogic.h 1.1.0).
 *    Pb is now drawn ONLY here (lsWavePOC 1.8.3 stopped drawing its POC-pullback Pb).
 *  1.0.0 (2026-11-03, Rassul approved the mockup bo_mockup.html - ES Oct 30, the real decisions lsBigBidAsk 1.31.3 recorded that day -
 *    and said "build"): a separate, letters-only indicator on the price chart:
 *      BO  green, just above the breakout candle's high      BD  red, just below the breakdown candle's low   (small, ~9 px)
 *      Rly green above the end of a rally / red below the end of a decline, one row further out (3-min charts only)
 *    Nothing else: no prints, lots, numbers, boxes, hovers or info box. No settings window rows (SS_SAFE_DIALOG, as lsCandleAbsorb).
 *    THE SAME DECISIONS as lsBigBidAsk: BO / BD through bba::boFromCandles (BigBidAskLogic.h 1.32.0, the function lsBigBidAsk's
 *    boPass calls), Rly through rly::run (RallyLogic.h 1.1.0). lsBigBidAsk 1.32.0 stopped drawing these letters (it still decides and
 *    records them for the nightly and lsLiquidityProfile).
 *  Calculated natively from the chart's bars (OHLC + volume); no file is read at run time. Closed candles only; a decision is kept for
 *  the chart's life and never changes or disappears (no repaint). Posts SignalBoard kind 'L' (the BO letters as drawn) for
 *  lsLiquidityProfile's Ex letter, as lsBigBidAsk 1.31.1 did.
 *  FILES: lsFlexLevels\Breakout.status-<MKT>-<spb>.txt  version, rules, today's counts, drawn counts (on change / 30 s; write only)
 *         (1.3.0) lsFlexLevels\Breakout\...-breakouts-<daykey>.csv, Breakout\Rly\...-<daykey>.csv, Breakout\...-pb-<daykey>.csv  the decision
 *         records (appended once per decision; read back only to skip keys already there - never used for a decision)
 *  Logic + layout: BreakoutLogic.h (SDK-free, tests_bo100). Per chart state in the host slot (HostSlot.h).
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
#include "BreakoutLogic.h"
#include "BreakoutHide.h"   // (1.11.1) lsCobSignal's Cob / Lvp natively: which Rly / Pb letters to hide (display only)
#include "HostSlot.h"
#include "SettingsGuard.h"   // SS_SAFE_DIALOG: the chart-less Add / settings copy never faults
#include "ColumnBoard.h"     // cb::leftKeep - nothing under the left Liquidity / Key Levels columns (as lsBigBidAsk 1.31.3)
#include "BoxDock.h"         // bdk::peekHodLod - never over the HOD/LOD table
#include "EntryFacts.h"      // (1.14.0) efx::facts - the drawn decided Pb / Pboc / Pdoc as fact kind E (the Limit Profile's ENT / EXIT tags)
#include "SignalBoard.h"     // sb::publish('L', ...) - the BO letters as drawn (lsLiquidityProfile reads them)
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <sys/stat.h>

#if defined(_WIN32)
#define BKO_SEP "\\"
#else
#define BKO_SEP "/"
#endif

#define BKO_VERSION "1.23.0"   // (1.23.0) book gate off; RevA   // was "1.22.0"   // (1.22.0) a Rev needs the book to agree (lsBookFlow fact F)   // was "1.21.0"   // (1.21.0) Rly only inside an hourly breakout bar
// was "1.20.0"   // (1.20.0) the retracement % under every decided pullback label
// was "1.19.0"   // (1.19.0) Rev: range >= 1.2 x ATR(13) OR a close beyond the prior bar; drawn after every pullback label incl. the Rly Pboc
// was "1.18.0"   // (1.18.0) the origin is broken by a CLOSE; PbT / PbocT (a pullback that tested a Step Trend level)
// was "1.17.0"   // (1.17.0) Pb / Pboc only (Pdoc not drawn)
// was "1.16.1"   // (1.16.1) no Rly near a drawn Cob
// was "1.16.0"   // (1.16.0) every Pb / Pboc / Pdoc pivot must retrace >= 38.2% of its wave   // was "1.15.0":   // (1.15.0) Pdoc only when the PB POC is >= 38.2% deep   // was "1.14.0":   // = bko::VERSION = the setVersion literal below (test_bo100_host checks)

static long long bkoMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string bkoFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + BKO_SEP "InvestorRT" BKO_SEP "rtx" BKO_SEP "lsFlexLevels" : std::string(); }
static void bkoMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool bkoExists(const std::string& p) { struct stat st; return !p.empty() && stat(p.c_str(), &st) == 0; }
static bool bkoPublish(const std::string& path, const std::string& text) {   // .tmp -> replace (atomic on Windows), .bak dance elsewhere
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false; f.write(text.data(), (std::streamsize)text.size()); f.flush(); if (!f.good()) { f.close(); std::remove(tmp.c_str()); return false; } }
#if defined(_WIN32)
    if (::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    std::remove(tmp.c_str()); return false;
#else
    const bool had = bkoExists(path);
    if (had) { std::remove(bak.c_str()); if (std::rename(path.c_str(), bak.c_str()) != 0) { std::remove(tmp.c_str()); return false; } }
    if (std::rename(tmp.c_str(), path.c_str()) != 0) { if (had) std::rename(bak.c_str(), path.c_str()); std::remove(tmp.c_str()); return false; }
    if (had) std::remove(bak.c_str());
    return true;
#endif
}
// (1.3.0) a bounded read of a record file (false = could not be read or bigger than cap: never rewrite what could not be read)
static const size_t BKO_REC_MAX = 4u * 1024u * 1024u;
static bool bkoRead(const std::string& path, std::string& out) {
    out.clear(); std::ifstream in(path.c_str(), std::ios::binary); if (!in) return false;
    in.seekg(0, std::ios::end); const std::streamoff size = in.tellg();
    if (size < 0 || size > (std::streamoff)BKO_REC_MAX) return false;
    in.seekg(0, std::ios::beg); out.resize((size_t)size); if (size > 0) in.read(&out[0], size);
    return (bool)in || in.eof();
}
static short bkoC(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }

struct BState {
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; bool keyed = false;
    int openMin = 510, closeMin = 900;
    bko::Engine E; bko::Scene last;
    std::string statusPath, statusBody; long long statusMs = -1;
    bb::Beat trBeat, hlBeat;   // the Tape Reader banner / HOD/LOD table present while their stamps keep moving (as lsBigBidAsk)
    // (1.3.0) records: lsBigBidAsk's market / contract names; keys known to be in each file; the engine run last written; counters
    std::string recMkt, recSym; std::map<std::string, std::set<std::string>> recHave; int recRuns = -1; long long recMs = -1;
    long long recAdded = 0, recFails = 0; long recPend = 0;
    bkh::Cob cob;   // (1.11.1) the Cob / Lvp letters lsCobSignal draws on this chart
    long long bkMs = -1; int bkFacts = -1, bkDecided = 0;   // (1.22.0) fact kind F (lsBookFlow's defend ratios): last read (steady ms), facts read (-1 = none on this chart), Revs decided
    long long eMs = -1; size_t ePosted = 0;   // (1.14.0) fact kind E: last post (steady ms) and how many
    bool vapFault = false;   // (1.12.0) a faulting VAP read is never re-entered (no Pboc then: the Pbs wait - status VAP)
};

class Breakout : public cppExtension {
public:
    SS_SAFE_DIALOG(Breakout, "Breakout")   // every plugin without settings: the Add / settings window copy has NO chart (lsCandleAbsorb 1.0.1)
    Breakout() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }          // no settings: nothing to load or save
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<BState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(BState& S);
    void feed(BState& S);
    void feedCob(BState& S, int first, const std::vector<bko::In>& B);
    int vapRows(BState& S, int chartBar, std::vector<wpc::VRow>& out);   // (1.12.0) lsWavePOC's reader
    static int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    static int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
    long long localSec(RTDATE d);
    void status(BState& S, long long now);
    void records(BState& S, long long now);
    void postEntry(BState& S, long long now);   // (1.14.0)
    void readBook(BState& S, long long now);    // (1.22.0)
    bool parts(RTDATE d, long& days, int& endSec);
    BState* testState() { return slot_.get(this, false); }
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }   // no settings (as lsCandleAbsorb / lsChartView)
int cppExtension::done(void) { static_cast<Breakout*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<Breakout*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { Breakout* me = static_cast<Breakout*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }   // bars read here only

// the bar stamp as the local (Central) civil day + second of the day (IRT stamps = the bar END)
bool Breakout::parts(RTDATE d, long& days, int& endSec) {
    struct tm m; std::memset(&m, 0, sizeof m);
    if (d == 0 || !getLocaltime(d, &m) || m.tm_mon < 0 || m.tm_mon > 11 || m.tm_mday < 1 || m.tm_mday > 31) return false;
    days = svl::daysFromCivil(m.tm_year + 1900, m.tm_mon + 1, m.tm_mday); endSec = m.tm_hour * 3600 + m.tm_min * 60 + m.tm_sec; return true;
}

bool Breakout::identify(BState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    const double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) return false;   // SYM_TICKINCR only (never a guessed tick)
    if (mkt.empty()) return false;
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey) {   // a new market / symbol / period: start over
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true; S.E.reset();
        S.openMin = 510; S.closeMin = 900; { int o = 0, c = 0; if (svl::rthOf(mkey::norm(mkt).c_str(), o, c)) { S.openMin = o; S.closeMin = c; } }
        const std::string d = bkoFlex();
        S.statusPath = d.empty() ? std::string() : d + BKO_SEP + (BKO_ONE_BAR ? "Breakout1" : "Breakout") + ".status-" + mkt + "-" + std::to_string(spb) + ".txt";
        S.statusBody.clear(); S.statusMs = -1;
        S.recMkt = bko::recMarket(rs ? rs : ""); S.recSym = bko::recFileKey(sym); S.recHave.clear(); S.recRuns = -1; S.recMs = -1; S.recPend = 0;   // (1.3.0)
    }
    return true;
}

void Breakout::feed(BState& S) {
    const long n = getBarCount(); if (n < 3) return;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYI vo(barVolume);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n || (long)vo.count < n) return;
    const long long lastClosed = (long long)times[(int)n - 2];
    if (S.E.count == n && S.E.lastClosedT == lastClosed && S.E.firstT == (long long)times[0]) return;   // nothing new closed (closed bars are final)
    const int first = std::max(0, (int)n - 1 - bko::MAX_BARS - 400);
    std::vector<bko::In> B((size_t)(n - first));
    for (int k = first; k < (int)n; ++k) {
        bko::In& b = B[(size_t)(k - first)]; b.t = (long long)times[k];
        b.o = op[k]; b.h = hi[k]; b.l = lo[k]; b.c = cl[k]; b.vol = (long long)vo[k];
        b.ok = parts((RTDATE)times[k], b.days, b.endSec);
    }
    S.E.bocLive = !S.vapFault;   // (1.12.0) Pboc: VAP of engine bar j = chart bar j + first, read only inside this update
    if (!S.vapFault) S.E.vapAt = [this, &S, first](int j, std::vector<wpc::VRow>& out) { return vapRows(S, j + first, out); };
    try { S.E.update(B, n, (long long)times[0], S.tick, S.spb, S.openMin, S.closeMin); } catch (...) { S.E.vapAt = nullptr; throw; }
    S.E.vapAt = nullptr;
    try { feedCob(S, first, B); } catch (...) { S.E.hideRly.clear(); S.E.hidePb.clear(); }   // (1.11.1) display only: a fault hides nothing
}

// (1.11.1, Rassul HG 10:13 "if there is a cob you dont need to show a rly and if there is an lvp, you dont need to show a pb, otherwise
// they start overlapping") lsCobSignal's rule on the same closed bars (cobs::Engine, as Wave POC runs it): a DRAWN Cob (shown + ZZO 10+)
// hides the Rly letter on its extreme (the Cob bar +- 1 bar, the Rly on the same side of the candle: a bull Cob under the low = a down
// Rly's end); a drawn Lvp hides the Pb on its pivot bar (same side). Display only - Rly / Pb records, Rev and PB Rev scoring unchanged.
void Breakout::feedCob(BState& S, int first, const std::vector<bko::In>& B) {
    bkh::hide(S.E, S.cob, mkey::norm(S.mkt), S.tick, S.spb, B, [this](long long t) { return localSec((RTDATE)t); });
    (void)first;
}
// (1.12.0) SEH-guarded VAP reads, copied from lsWavePOC 1.10.1 (only trivial values inside, /EHsc compatible)
int Breakout::readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
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
int Breakout::readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) { std::memset(&rows[i], 0, sizeof(rows[i])); if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2; }
        return count;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// one bar's volume at price -> rows in ticks: 1 = read, 0 = no VAP for it (yet), -1 = fault (never re-entered) - lsWavePOC::vapRows
int Breakout::vapRows(BState& S, int chartBar, std::vector<wpc::VRow>& out) {
    out.clear();
    if (S.vapFault) return -1;
    RTARRAYP vap(barVolumeProfile);
    if (vap.count <= chartBar) return 0;
    BARSTATISTICS st; const int prices = readStatistics(&vap, chartBar, &st);
    if (prices == -1) { S.vapFault = true; return -1; }
    if (prices <= 0 || prices > 100000) return 0;
    std::vector<VOLPROFILE> rows((size_t)prices);
    const int got = readPrices(&vap, chartBar, &rows[0], prices);
    if (got == -1) { S.vapFault = true; return -1; }
    if (got < 0) return 0;
    for (const VOLPROFILE& v : rows) {
        if (!std::isfinite((double)v.price) || v.totalVolume < 0) continue;
        double vol = (double)v.totalVolume; const double bs = (double)v.buyVolume + (double)v.sellVolume; if (bs > vol + 0.5) vol = bs;
        if (vol <= 0) continue;
        wpc::VRow r; if (!wpc::toTicks((double)v.price, S.tick, r.tick)) continue;
        r.vol = (long long)std::llround(vol); r.dlt = (long long)std::llround((double)v.buyVolume - (double)v.sellVolume); out.push_back(r);
    }
    return out.empty() ? 0 : 1;
}
// (1.22.0) the book gate's input: lsBookFlow's DECIDED fact F candles of this chart, once a second, keyed to the engine's bars by the
// Central wall second of the bar (days * 86400 + endSec = lsBookFlow's ctOf). No lsBookFlow on the chart = nothing handed in.
void Breakout::readBook(BState& S, long long now) {
    if (!bko::REV_NEEDS_BOOK) return;
    if (S.bkMs >= 0 && now >= S.bkMs && now - S.bkMs < 1000) return;
    S.bkMs = now;
    const char* sy = getSymbol(); const char* chl = getChartLabel(); if (!sy || !chl || !*sy) return;
    const long long ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    std::vector<sb::Fact> v; std::map<long long, int> bits;
    if (!sb::readFacts('F', sy, getSecondsPerBar(), chl, ms, v, true)) { S.bkFacts = -1; S.bkDecided += S.E.setBook(bits); return; }
    S.bkFacts = (int)v.size();
    if (!v.empty()) {
        std::map<long long, long long> ct; const size_t n = S.E.bars.size(), from = n > 400 ? n - 400 : 0;
        for (size_t k = from; k < n; ++k) { const bko::In& b = S.E.bars[k]; if (b.ok) ct[(long long)b.days * 86400LL + b.endSec] = b.t; }
        for (const sb::Fact& f : v) { const auto it = ct.find(f.bar); if (it != ct.end()) bits[it->second] = f.code; }
    }
    S.bkDecided += S.E.setBook(bits);
}
// (1.14.0) fact kind E once a second (SignalBoard facts are fresh for 60 s): the 3-bar build only (one producer per chart, ChainSpec.h)
void Breakout::postEntry(BState& S, long long now) {
    if (BKO_ONE_BAR) return;
    if (S.eMs >= 0 && now >= S.eMs && now - S.eMs < 1000) return;
    const char* sy = getSymbol(); const char* chl = getChartLabel(); const std::string sym = sy ? sy : "", chart = chl ? chl : "";
    if (sym.empty()) return;
    const std::vector<sb::Fact> v = efx::facts(S.E, [this](long long t) { return localSec((RTDATE)t); });
    const long long ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    sb::postFacts('E', sym, getSecondsPerBar(), chart, ms, v); S.eMs = now; S.ePosted = v.size();
}

long long Breakout::localSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, dd = (unsigned)t.tm_mday;
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

void Breakout::status(BState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;
    const std::string body = bko::statusText(S.E, S.mkt, S.sym, S.spb, S.last)
        + "RECORDS,lsFlexLevels\\Breakout (" + S.recMkt + "-" + S.recSym + "-" + std::to_string(S.spb) + "),zz_rev_bars " + std::to_string(S.E.zzRev) + ",ft_tie " + (S.E.tieOn() ? "1" : "0") + ",zz_carry " + (S.E.carryOn() ? "1" : "0") + (bko::SHOW_BI ? ",bi 1" : ",bi 0")
        + ",lines written " + std::to_string(S.recAdded) + ",pending " + std::to_string(S.recPend) + ",write fails " + std::to_string(S.recFails) + "\n"
        + "ENTRY_FACTS,EntryFacts " + efx::VERSION + "," + (BKO_ONE_BAR ? std::string("off (lsBreakout1: lsBreakout posts kind E)") : "posted " + std::to_string(S.ePosted) + " drawn decided Pb / Pboc / Pdoc as kind E (the Limit Profile ENT / EXIT tags)") + "\n";   // (1.3.0) (1.14.0)
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    S.statusMs = now; S.statusBody = body;
    bkoMkdir(bkoFlex()); bkoPublish(S.statusPath, body);
}

// (1.3.0) RECORDS: after a new engine run (or while lines wait after a failed write, retried once a second), every record line of the last
// 3 sessions whose key is not yet known to be in its file: the file is read, the lines whose key is already there are dropped (written by
// an earlier run, another chart or before a restart), the rest appended in one atomic replace. A key is "known" only after a good write
// or once seen in the file, so nothing is written twice and nothing is lost on a failed write.
void Breakout::records(BState& S, long long now) {
    if (S.E.runs == S.recRuns && S.recPend == 0) return;
    if (S.recPend > 0 && S.E.runs == S.recRuns && S.recMs >= 0 && now - S.recMs < 1000) return;
    const std::string f = bkoFlex(); if (f.empty() || S.recMkt.empty()) return;
    S.recRuns = S.E.runs; S.recMs = now;
    const std::vector<bko::RecFile> G = bko::recPlan(S.E, S.recMkt, S.recSym, S.spb);
    long pend = 0; std::set<std::string> live;
    for (const bko::RecFile& F : G) {
        const std::string dir = f + BKO_SEP + F.dir, path = dir + BKO_SEP + F.file; live.insert(path);
        std::set<std::string>& have = S.recHave[path];
        std::vector<std::string> want; for (const std::string& l : F.lines) { const std::string k = bko::recKey(l); if (!k.empty() && !have.count(k)) want.push_back(l); }
        if (want.empty()) continue;
        std::string old; if (bkoExists(path) && !bkoRead(path, old)) { ++S.recFails; pend += (long)want.size(); continue; }
        std::set<std::string> inFile; int added = 0; const std::string text = bko::recMerge(old, F.head, want, inFile, added);
        if (added > 0) {
            bkoMkdir(f); bkoMkdir(f + BKO_SEP BKO_REC_DIR); bkoMkdir(dir);
            if (!bkoPublish(path, text)) { ++S.recFails; pend += (long)want.size(); continue; }
            S.recAdded += added;
        }
        have.swap(inFile);
    }
    for (auto it = S.recHave.begin(); it != S.recHave.end();) { if (live.count(it->first)) ++it; else it = S.recHave.erase(it); }   // older sessions: forget
    S.recPend = pend;
}

void Breakout::pump() {
    BState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); const long long now = bkoMs(); readBook(*S, now); records(*S, now); postEntry(*S, now); status(*S, now);
}

int Breakout::draw(void) {
    BState* S = slot_.get(this, false);
    if (!S || S->E.bars.size() < 3) return RTX_OK;   // draw never reads bars for decisions (calc does)
    try {
        const long n = getBarCount(); if (n < 3) return RTX_OK;
        const int off = (int)n - (int)S->E.bars.size();   // chart index of E.bars[0]
        if (off < 0) return RTX_OK;
        RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
        int axisR = pane.right;
        if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) axisR = scale.left - 2;
        if (pane.bottom - pane.top < 60 || axisR - pane.left < 60) return RTX_OK;
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK || vs < 0 || ve < vs) { vs = std::max(0, (int)n - 200); ve = (int)n - 1; }
        const char* sy = getSymbol(); const char* chl = getChartLabel();
        const std::string sym = sy ? sy : "", chart = chl ? chl : ""; const long long ms = bkoMs();
        bko::View v; v.first = vs - off; v.last = ve - off;
        v.area.l = std::max((int)pane.left + 2, cb::leftKeep(sym, getSecondsPerBar(), chart, (int)pane.left, ms));
        v.area.t = pane.top + 20; v.area.r = axisR - 1; v.area.b = pane.bottom - 1;   // the IRT title row stays clear
        {   bb::Rect r; long long bms = -1; const bool hb = bb::peekBanner(sym, getSecondsPerBar(), chart, r, bms);
            if (S->trBeat.step(hb, bms, ms)) { bko::Box k; k.l = r.l; k.t = r.t; k.r = r.r; k.b = r.b; v.keepOut.push_back(k); }
            bdk::Box h; long long hms = -1; const bool hh = bdk::peekHodLod(sym, getSecondsPerBar(), chart, h, hms);
            if (S->hlBeat.step(hh, hms, ms)) { bko::Box k; k.l = h.l - 2; k.t = h.t - 2; k.r = h.r + 2; k.b = h.b + 2; v.keepOut.push_back(k); } }
        v.X = [this, off](int j) { PNT p; p.set(j + off, 0.0f, kBarCenter); return (int)p.h; };
        v.Y = [this](double px) { return (int)getVerticalDrawPosition((float)px); };
        FONT f0; f0.id = HELVETICA; f0.size = bko::FONT_BO; f0.style = BOLD;
        FONT f1 = f0; f1.size = bko::FONT_RLY;
        int lh[2] = { 0, 0 }, lift[2] = { 1, 1 };
        for (int k = 0; k < 2; ++k) { FONT& f = k ? f1 : f0; setFont(f); int ll = 0, la = 0, ld = 0; getFontMetrics(&ll, &la, &ld);
            lh[k] = std::max((int)f.size + 5, la + ld); lift[k] = std::max(1, (ll + ld + 1) / 2); }
        v.lh[0] = lh[0]; v.lh[1] = lh[1];
        v.textW = [this, &f0, &f1](const char* s, int font) { setFont(font == bko::FONT_RLY ? f1 : f0); return (int)getTextWidth(s, -1); };
        S->last = bko::layout(S->E, v);
        for (const bko::Txt& T : S->last.txt) {   // lsBigBidAsk's letter drawing: a 1 px black edge, then the colour
            const int k = T.kind == 1 ? 1 : 0; setFont(k ? f1 : f0); const int lf = lift[k];
            setTextColor(0x00000000);
            for (int d = 0; d < 4; ++d) { const int dx = d == 0 ? -1 : d == 1 ? 1 : 0, dy = d == 2 ? -1 : d == 3 ? 1 : 0;
                RCT r; r.set(bkoC(T.f.l + dx), bkoC(T.f.t - lf + dy), bkoC(T.f.r + dx), bkoC(T.f.b - lf + dy)); r.drawText(T.s.c_str(), true, false); }
            setTextColor((COLOR)T.col);
            RCT r; r.set(bkoC(T.f.l), bkoC(T.f.t - lf), bkoC(T.f.r), bkoC(T.f.b - lf)); r.drawText(T.s.c_str(), true, false);
        }
        std::vector<sb::Ev> ev;   // SignalBoard 'L' (SignalBoard.h 1.3.0): bar (CT wall seconds), side, px from the extreme to the outer edge
        for (const bko::Board& b : S->last.board) { long d = 0; int e = 0; if (!parts((RTDATE)b.t, d, e)) continue;
            sb::Ev x; x.bar = (long long)d * 86400LL + e; x.side = b.side; x.val = (double)b.val; ev.push_back(x); }
        sb::publish('L', sym, getSecondsPerBar(), chart, ms, ev);
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    Breakout* p = new Breakout();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | VAP_REQUIRED);   // (1.12.0) VAP for the wave POC (Pboc)
#if BKO_ONE_BAR
    p->setDescription("LRA BO 1-bar v1.14.0 - lsBreakout on the 1-bar High/Low ZigZag (for higher timeframes): letters only on the price chart: BO (green, above) / BD (red, below) on a confirmed breakout candle, shown when the follow-through candle closes; Rly at the end of a rally / decline; Pb on a pullback pivot (a low confirmed by the next green close / a high by the next red close) whose bar overlaps the BO / BD candle, at most 2 per BO / BD. Rev (green above the bar / red below) = the thrust out of a Pb: the first green bar (BD: red) from the pivot bar on whose low is below the pivot bar's high (BD: high above its low) with range >= 1.2 x ATR(13), one per wave. Pboc = a Pb (or a no-BO Rly wave's pullback past its 50%) whose pivot bar tags the wave's volume POC; Pdoc = it tags the pullback's developing POC (Wave POC's PB POC) instead. Same rules as lsBreakout, pivots from the 1-bar ZigZag. Records to lsFlexLevels\\Breakout1 for the nightly. No settings.");
#else
    p->setDescription("LRA BO v1.14.0 - letters only on the price chart: BO (green, above) / BD (red, below) on a confirmed breakout candle (the 3-bar High/Low ZigZag, which runs on across the 5 PM CT session break, shown when the follow-through candle closes; its close may equal the BO candle's), Rly at the end of a rally / decline (3-min; RTH on the session VWAP, overnight 17:00 CT to the open on its own Globex VWAP), Pb on a pullback pivot (a low confirmed by the next green close / a high by the next red close) whose bar overlaps the BO / BD candle, at most 2 per BO / BD. Rev (green above the bar / red below) = the thrust out of a Pb: the first green bar (BD: red) from the pivot bar on whose low is below the pivot bar's high (BD: high above its low) with range >= 1.2 x ATR(13), one per wave. Pboc = a Pb (or a no-BO Rly wave's pullback past its 50%) whose pivot bar tags the wave's volume POC (IRT VAP, Wave POC's rule); Pdoc = it tags the pullback's developing POC (Wave POC's PB POC) instead. For higher timeframes use lsBreakout1 (1-bar ZigZag). Records to lsFlexLevels\\Breakout for the nightly. No settings.");
#endif
    p->setVersion("1.23.0");   // = BKO_VERSION (gex-build reads this literal)
    return p;
}
