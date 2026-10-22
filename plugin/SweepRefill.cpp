/********************************************************************************
 *  SweepRefill.cpp  --  Investor/RT RTX extension  lsSweepRefill  1.0.0  (2026-10-22)
 *
 *  THE SWEEP-REFILLED MARK "Sr" on the PRICE chart (MBO Roadmap "Did the swept prices refill fast after a sweep?").
 *  Rassul 16:16 CT (build approved; "Sr only happens when a level or pivot high or low was taken out"; "you can put it on top of the
 *  bar for a bullish reclaim"), 16:18 CT ("it should not be on adjacent candles"), 16:28 CT (the red Sr at NQ 9:55 / 10:03 are
 *  valid - draw bearish too; widen the pivot rule; mockup v3 sweep-refill-mockup-v3.png):
 *    - green "Sr" ABOVE the candle: sellers swept 8+ bid prices through a level / pivot low at a local low and the bids came back
 *      (70%+ of the pre-sweep resting size within 3 s);
 *    - red "Sr" BELOW the candle: the mirror (buyers swept the offers through a level / pivot high, the offers came back);
 *    - never on two adjacent candles (per side): the later one is recorded only; the first one is never moved or removed;
 *    - short text, no box, no border; decided once (4 s after the sweep), recorded, redrawn from the record - never repainted.
 *  The hover (mouse over the mark): header "Sweep refilled?" ("?" = not proven), the read, the numbers, then
 *  question / what it is / why it matters / how to use / example.
 *  Rules: SweepRefillLogic.h (SDK-free, unit-tested; replay on real MBO in tests_sr/replay_sr.cpp).
 *
 *  FEED: one DEPTH_BY_ORDER per chart, kept alive for the chart's life, opened from calc / draw only behind a crash-guard file,
 *  drained <= 5,000 events / ~8 ms per call (CALL_CONTINUOUSLY keeps calc coming while the chart is quiet). Every NEW / MOD / DEL /
 *  FILL keeps the order book; the event clock = the exchange second + the arrival ms inside it (swr::Clock). There is NO historical
 *  order-by-order data: Sr exists from the moment the indicator is on the chart; the session record brings the marks back.
 *  Levels (read-only, the same files his other plugins read): lsFlexLevels\LRA-KeyLevels-<MKT>.csv (ONH/ONL PDH/PDL PFH/PFL LonHI/
 *  LonLO), LRA-Dealer-<MKT>.csv and LRA-Levels-<MKT>.csv (MenthorQ), fresh <= 18 h, MenthorQ only for the chart's contract.
 *  FILES (lsFlexLevels\SweepRefill\):  <MKT>-<spb>-<yyyymmdd>.csv  every candidate (refilled or not), one SR line each, append-only
 *                                      SweepRefill.status-<MKT>-<spb>.txt  the status, on change or every 30 s
 *  Settings: lsFlexLevels\SweepRefill.settings-<MKT>.txt via SettingsSpec / SettingsHost (SweepRefillSettings.h).
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
#include "SweepRefillLogic.h"
#include "NearLevels.h"
#include "HostSlot.h"
#include "SweepRefillSettings.h"
#include "SettingsHost.h"
#include "SettingsGuard.h"
#include "NewsWindow.h"   // (2026-10-22 evening) news / settle window "?" + the feed check (record only) - shared, SDK-free
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <new>
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

#define SWR_VERSION "1.0.0"     // = swr::VERSION = swrset::PLUGIN_VERSION = the setVersion literal

#ifdef SWR_TEST_CLOCK
long long swrTestClockMs();
static long long swrMs() { return swrTestClockMs(); }
#else
static long long swrMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
#endif

// bDboSubscribed is private: a pointer-to-member through explicit instantiation (as MboCheck.cpp), never by offset
template <class Tag, typename Tag::type M> struct SwrRob { friend typename Tag::type swrMember(Tag) { return M; } };
struct SwrDboFlag { typedef RTBOOL cppExtension::DEPTH_BY_ORDER::*type; friend type swrMember(SwrDboFlag); };
template struct SwrRob<SwrDboFlag, &cppExtension::DEPTH_BY_ORDER::bDboSubscribed>;

static const int SLICE_MAX = 5000; static const long long SLICE_BUDGET_MS = 8;
static const int HOVER_PT = 10, HOVER_LH = 14;
static const int LANES_RESERVED = 2;     // the first two text lanes above / below a candle belong to lsBigBidAsk's BO / BD and Rly

static std::string swrFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + SWR_SEP "InvestorRT" SWR_SEP "rtx" SWR_SEP "lsFlexLevels" : std::string(); }
static void swrMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool swrExists(const std::string& p) { struct stat st; return !p.empty() && stat(p.c_str(), &st) == 0; }

struct HitBox { int l, t, r, b; size_t idx; };

struct SwrState {
    // identity
    std::string identity, market, symbol; int spb = 0; double tick = 0;
    // feed
    cppExtension::DEPTH_BY_ORDER* dbo = nullptr; bool dboDead = false, blocked = false; int attempt = 0; int flag = -1;
    long long events = 0, fillsLots = 0, lastResubMs = -1, backlogSlices = 0, lastStampSec = 0, garbage = 0;
    // detector + bars
    swr::Detector D; swr::Clock clk; bool primed = false; long long lastClosedT = 0;
    // levels
    nl::Source keySrc, dealerSrc, mqSrc; long long levelCheckMs = -100000; std::string levelNote = "not read yet";
    // record
    std::vector<swr::Event> marks; std::string session, recPath; long long recFails = 0, recLines = 0, recBad = 0; bool loaded = false;
    // settings
    ss::Store setStore; ss::Values set = ss::defaults(swrset::spec(), -1); long long setCheckMs = -100000;
    // hover
    std::vector<HitBox> hits; int hover = -1;
    // status
    std::string statusBody; long long statusMs = -100000; long long busy = 0;
    ~SwrState() { delete dbo; }
};
static ss::Ids& swrIds() { static ss::Ids I; return I; }

class SweepRefill : public cppExtension {
public:
    SS_SAFE_DIALOG(SweepRefill, "SweepRefill")
    HostSlot<SwrState> slot_;
    SweepRefill() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int parmsLoadBody(void) { ss::guardedCall(this, &SweepRefill::loadBody, 0u); return RTX_OK; }
    virtual int parmsApplyBody(void) { ss::guardedCall(this, &SweepRefill::applyBody, 0u); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::guardedCall(this, &SweepRefill::updtBody, i); return RTX_OK; }
    void loadBody(unsigned) {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        ss::fillDialog(this, swrset::spec(), swrIds(), st->load(ss::marketIndex(dlgMkt_), swrMs(), true), snap_);
    }
    void applyBody(unsigned) { saveSettings(); }
    void updtBody(unsigned i) { ss::onUpdate(this, swrset::spec(), swrIds(), (int)i, snap_); saveSettings(); }
    void pump(bool fromDraw);
    SwrState* testState() { return slot_.get(this, false); }
private:
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        const std::string mkt = ss::safeMarket(this); const std::string p = swrset::settingsPath(swrFlex(), mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &swrset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, swrset::spec(), swrIds(), st->load(ss::marketIndex(dlgMkt_), swrMs(), true), snap_);
        swrMkdir(swrFlex());
        if (st->save(V)) snap_.take(ss::dialogNow(this, swrset::spec(), swrIds(), V));
    }
    void identify(SwrState& S);
    void settings(SwrState& S, long long now);
    void levels(SwrState& S, long long now);
    void openFeed(SwrState& S);
    void drain(SwrState& S, long long now);
    void bars(SwrState& S);
    void loadRecord(SwrState& S);
    void append(SwrState& S, const std::string& line);
    void collect(SwrState& S);
    void status(SwrState& S, long long now);
    std::string ctText(long long stamp);
    std::string sessionOf(RTDATE stamp);
    long long sessionKey(long long stamp);
    void render(SwrState& S);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { static_cast<SweepRefill*>(this)->slot_.release(this); return RTX_OK; }      // symbol / period change: start clean
int cppExtension::destroy(void) { static_cast<SweepRefill*>(this)->slot_.release(this); return RTX_OK; }
int cppExtension::setup(void) { ss::registerAll(this, swrset::spec(), swrIds()); return RTX_OK; }
int cppExtension::calc(int) { static_cast<SweepRefill*>(this)->pump(false); return RTX_OK; }

std::string SweepRefill::ctText(long long stamp) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (stamp <= 0 || !getLocaltime((RTDATE)stamp, &t)) return std::string();      // IRT's local clock = Central on his PC
    const int h12 = t.tm_hour % 12 == 0 ? 12 : t.tm_hour % 12;
    char b[64]; std::snprintf(b, sizeof b, "%d:%02d:%02d %s", h12, t.tm_min, t.tm_sec, t.tm_hour < 12 ? "AM" : "PM"); return b;
}
// the 17:00 CT trading day: an event at or after 17:00 belongs to the next day's session
std::string SweepRefill::sessionOf(RTDATE stamp) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(stamp, &t)) return std::string();
    struct tm d = t; d.tm_hour = 12; d.tm_min = d.tm_sec = 0; d.tm_isdst = -1;
    std::time_t day = std::mktime(&d); if (day == (std::time_t)-1) return std::string();
    if (t.tm_hour >= 17) day += 86400;
    struct tm sd; std::memset(&sd, 0, sizeof sd);
#if defined(_WIN32)
    localtime_s(&sd, &day);
#else
    localtime_r(&day, &sd);
#endif
    char b[48]; std::snprintf(b, sizeof b, "%04d%02d%02d", sd.tm_year + 1900, sd.tm_mon + 1, sd.tm_mday); return b;
}
long long SweepRefill::sessionKey(long long stamp) { const std::string s = sessionOf((RTDATE)stamp); return s.empty() ? 0 : std::strtoll(s.c_str(), nullptr, 10); }

void SweepRefill::identify(SwrState& S) {
    const std::string mkt = ss::safeMarket(this);
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR); if (!swr::finite(tick) || tick <= 0 || tick >= 1000) tick = 0;
    const std::string id = mkt + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity && tick == S.tick) return;
    delete S.dbo; S.dbo = nullptr;
    S.identity = id; S.market = mkt; S.symbol = sym; S.spb = spb; S.tick = tick;
    S.dboDead = S.blocked = false; S.attempt = 0; S.flag = -1; S.events = S.fillsLots = S.garbage = 0; S.lastResubMs = -1; S.lastStampSec = 0;
    S.D = swr::Detector(); S.clk = swr::Clock(); S.primed = false; S.lastClosedT = 0;
    S.keySrc = nl::Source(); S.dealerSrc = nl::Source(); S.mqSrc = nl::Source(); S.levelCheckMs = -100000;
    S.marks.clear(); S.session.clear(); S.recPath.clear(); S.loaded = false;
    S.setStore = ss::Store(); S.setCheckMs = -100000; S.hits.clear(); S.hover = -1; S.statusBody.clear();
}

void SweepRefill::settings(SwrState& S, long long now) {
    if (now - S.setCheckMs < 1000 && now >= S.setCheckMs) return;
    S.setCheckMs = now;
    const std::string sp = swrset::settingsPath(swrFlex(), S.market);
    if (S.setStore.path != sp) { S.setStore = ss::Store(); S.setStore.S = &swrset::spec(); S.setStore.path = sp; }
    S.set = sp.empty() ? ss::defaults(swrset::spec(), ss::marketIndex(S.market)) : S.setStore.load(ss::marketIndex(S.market), now);
    swr::Params& P = S.D.P;   // a change applies from now on; decided marks never change
    P.minPrices = S.set.i(swrset::K_MIN_PRICES); P.minLots = S.set.i(swrset::K_MIN_LOTS); P.refillPct = (double)S.set.i(swrset::K_REFILL_PCT);
    P.refillMs = 1000 * S.set.i(swrset::K_WINDOW_S); P.gapBars = S.set.i(swrset::K_GAP_BARS); P.spb = S.spb > 0 ? S.spb : 180;
}

// key levels (his session levels only) + MenthorQ (dealer / levels files, the chart's contract only), as NearLevels.h reads them
void SweepRefill::levels(SwrState& S, long long now) {
    if (now - S.levelCheckMs < 5000 && now >= S.levelCheckMs) return;
    S.levelCheckMs = now;
    const std::string f = swrFlex(); if (f.empty() || S.market.empty() || !(S.tick > 0)) return;
    nl::loadSource(f + SWR_SEP "LRA-KeyLevels-" + S.market + ".csv", S.keySrc, 0);
    nl::loadSource(f + SWR_SEP "LRA-Dealer-" + S.market + ".csv", S.dealerSrc, 1);
    nl::loadSource(f + SWR_SEP "LRA-Levels-" + S.market + ".csv", S.mqSrc, 2);
    const long long wall = (long long)std::time(nullptr);
    auto fresh = [&](const nl::Source& X) { return X.sig >= 0 && X.at > 0 && wall - X.at <= 18 * 3600 && X.at - wall <= 600; };
    std::vector<swr::Level> v; int nk = 0, nm = 0;
    if (fresh(S.keySrc)) for (size_t i = 0; i < S.keySrc.v.size(); ++i) {
        swr::Level L; L.name = S.keySrc.v[i].name; L.dir = swr::keyDir(L.name); L.kind = swr::REF_KEY;
        if (L.dir == 0 || !swr::toTicks(S.keySrc.v[i].px, S.tick, L.px)) continue;
        v.push_back(L); ++nk;
    }
    const nl::Source* mq[2] = { &S.dealerSrc, &S.mqSrc };
    for (int k = 0; k < 2; ++k) {
        const nl::Source& X = *mq[k];
        if (!fresh(X) || X.stale || !nl::sameContract(S.symbol, X.sym)) continue;
        for (size_t i = 0; i < X.v.size(); ++i) {
            swr::Level L; L.name = X.v[i].name; L.kind = swr::REF_MQ; L.dir = 0;
            if (!swr::toTicks(X.v[i].px, S.tick, L.px)) { const double t = X.v[i].px / S.tick; if (!swr::finite(t) || t <= 0 || t > 9e15) continue; L.px = (long long)std::llround(t); }
            bool dup = false; for (size_t j = 0; j < v.size() && !dup; ++j) dup = v[j].kind == swr::REF_MQ && v[j].px == L.px;
            if (!dup) { v.push_back(L); ++nm; }
        }
    }
    S.D.levels.swap(v);
    S.levelNote = std::to_string(nk) + " key, " + std::to_string(nm) + " MenthorQ" + (S.keySrc.sig < 0 ? " (key file missing - open lsDealerProfile on a 3-min chart)" : "");
}

// chart context only, behind the crash guard; 1st try the chart's symbol, 2nd try NULL (= this chart)
void SweepRefill::openFeed(SwrState& S) {
    if (S.dbo || S.dboDead || S.blocked || S.attempt >= 2) return;
    const std::string dir = swrFlex() + SWR_SEP "SweepRefill";
    const std::string guard = swrFlex().empty() ? std::string() : dir + SWR_SEP "_opening-" + S.market + "-" + std::to_string(S.spb) + ".txt";
    if (S.attempt == 0 && swrExists(guard)) { S.blocked = true; return; }
    if (!guard.empty()) { swrMkdir(swrFlex()); swrMkdir(dir); std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening DEPTH_BY_ORDER\n"; }
    try { S.dbo = new (std::nothrow) DEPTH_BY_ORDER(S.attempt == 0 && !S.symbol.empty() ? S.symbol.c_str() : NULL); }
    catch (...) { S.dboDead = true; }
    ++S.attempt;
    if (!guard.empty()) std::remove(guard.c_str());
}

void SweepRefill::drain(SwrState& S, long long now) {
    if (!S.dbo || S.dboDead || !(S.tick > 0)) { S.D.advance(S.clk.idle(now)); return; }
    if (S.flag != 1 && S.lastResubMs >= 0 && now - S.lastResubMs < 1000 && now >= S.lastResubMs) return;
    const long long t0 = swrMs(); int n = 0; bool cap = false;
    try {
        if (!(S.dbo->*swrMember(SwrDboFlag()))) S.lastResubMs = now;
        while (true) {
            if (n >= SLICE_MAX) { cap = true; break; }
            if ((n & 511) == 511 && swrMs() - t0 > SLICE_BUDGET_MS) { cap = true; break; }
            if (S.dbo->getNext() != RTX_OK) break;
            ++n; ++S.events;
            const DEPTH_ORDER& o = S.dbo->dbo;
            swr::Ev e;
            e.action = o.action <= 3 ? (int)o.action : 99;
            e.side = o.buySell == 1 ? swr::BID : o.buySell == 0 ? swr::ASK : -1;
            if (!swr::toTicks((double)o.price, S.tick, e.px)) { ++S.garbage; continue; }
            e.size = (long long)o.size; e.id = (long long)o.orderID;
            long long sec = (long long)o.timestamp; if (sec < S.lastStampSec && S.lastStampSec - sec < 1800) sec = S.lastStampSec;   // a stale stamp (ArrivalStamp.h)
            S.lastStampSec = sec; e.sec = sec; e.ms = S.clk.at(sec, now);
            if (e.action == swr::FILL) S.fillsLots += e.size;
            S.D.onEvent(e);
        }
        S.flag = (S.dbo->*swrMember(SwrDboFlag())) ? 1 : 0;
    } catch (...) { S.dboDead = true; }                                              // never re-enter a faulting call
    if (cap) ++S.backlogSlices;
    else S.D.advance(S.clk.idle(now));                                               // the feed is caught up: time passes
}

// closed candles (a later bar exists), strictly in order, each once; history at the first pass gives the pivots / ATR context
void SweepRefill::bars(SwrState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    if ((long)times.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    auto bar = [&](int i, swr::Bar& b) { b.t = (long long)times[i]; b.session = sessionKey(b.t - 1); return swr::toTicks(hi[i], S.tick, b.h) && swr::toTicks(lo[i], S.tick, b.l) && swr::toTicks(cl[i], S.tick, b.c); };
    const int lastClosed = (int)n - 2;
    int from;
    if (!S.primed) { S.primed = true; from = std::max(0, lastClosed - 60); }
    else { if ((long long)times[lastClosed] <= S.lastClosedT) return; from = lastClosed; while (from > 0 && (long long)times[from - 1] > S.lastClosedT) --from; }
    for (int i = from; i <= lastClosed; ++i) { swr::Bar b; if (!bar(i, b)) continue; S.D.barClose(b); }
    S.lastClosedT = (long long)times[lastClosed];
}

void SweepRefill::loadRecord(SwrState& S) {
    const std::string sess = sessionOf(currentDate());
    if (sess.empty() || S.market.empty()) return;
    if (S.loaded && sess == S.session) return;
    S.session = sess; S.loaded = true; S.marks.clear();
    const std::string dir = swrFlex() + SWR_SEP "SweepRefill";
    S.recPath = swrFlex().empty() ? std::string() : dir + SWR_SEP + S.market + "-" + std::to_string(S.spb) + "-" + sess + ".csv";
    std::vector<std::string> lines;
    if (S.recPath.empty() || !nl::readBoundedLines(S.recPath, lines, 4 * 1024 * 1024, 20000, 1024)) return;
    S.marks = swr::parseRecord(lines, S.tick, &S.recBad);
    for (size_t i = 0; i < S.marks.size(); ++i) {                 // the gap rule continues from the record (a restart never draws an adjacent Sr)
        const swr::Event& e = S.marks[i];
        if (e.refilled && e.gapOk && e.barT > S.D.lastDrawnBar[e.side]) S.D.lastDrawnBar[e.side] = e.barT;
    }
}

// one flushed append per line; a failure is counted (never retried in a loop)
void SweepRefill::append(SwrState& S, const std::string& line) {
    if (S.recPath.empty()) { ++S.recFails; return; }
    swrMkdir(swrFlex()); swrMkdir(swrFlex() + SWR_SEP "SweepRefill");
    const bool fresh = !swrExists(S.recPath);
    std::ofstream f(S.recPath.c_str(), std::ios::binary | std::ios::app);
    if (!f) { ++S.recFails; return; }
    if (fresh) f << swr::REC_HEAD << "\n";
    f << line << "\n"; f.flush();
    if (!f) { ++S.recFails; return; }
    ++S.recLines;
}

// decided candidates -> marks + record (once per key)
void SweepRefill::collect(SwrState& S) {
    for (size_t i = 0; i < S.D.decided.size(); ++i) {
        swr::Event e = S.D.decided[i]; e.ct = ctText(e.sec);
        bool dup = false; for (size_t k = 0; k < S.marks.size() && !dup; ++k) dup = S.marks[k].key == e.key;
        if (dup) continue;
        S.marks.push_back(e); append(S, swr::recLine(e, S.tick));
    }
    S.D.decided.clear();
    if (S.marks.size() > 5000) S.marks.erase(S.marks.begin(), S.marks.begin() + (long)(S.marks.size() - 5000));
}

void SweepRefill::status(SwrState& S, long long now) {
    const std::string d = swrFlex(); if (d.empty() || S.market.empty()) return;
    long long gb = 0, rb = 0, rec = 0; for (size_t i = 0; i < S.marks.size(); ++i) { const swr::Event& e = S.marks[i]; if (e.refilled && e.gapOk) { if (swr::bullish(e)) ++gb; else ++rb; } else ++rec; }
    std::ostringstream o;
    o << "VERSION," << SWR_VERSION << "\nMARKET," << S.market << ",SYMBOL," << S.symbol << ",SECONDS_PER_BAR," << S.spb << "\n";
    o << "STATUS,lsSweepRefill v" << SWR_VERSION << " | MBO " << (S.blocked ? "not opened (crash guard file present)" : S.dboDead ? "faulted" : S.flag == 1 ? "subscribed" : S.flag == 0 ? "not subscribed" : "opening")
      << " | " << S.events << " events, " << S.fillsLots << " lots filled, book " << S.D.book.size() << " orders | sweeps " << S.D.sweeps << ", big " << S.D.bigSweeps
      << ", at a local extreme " << S.D.atExtreme << ", took out a level / pivot " << S.D.tookOut << " | Sr today green " << gb << ", red " << rb << ", recorded only " << rec << "\n";
    o << "LEVELS," << S.levelNote << ",pivots " << S.D.pivots.size() << "\nRECORD," << S.recPath << ",lines " << S.recLines << ",failed " << S.recFails << ",bad lines " << S.recBad << "\n";
    o << "SETTINGS," << S.set.why << ",bullish " << (S.set.b(swrset::K_SHOW_BULL) ? "on" : "off") << ",bearish " << (S.set.b(swrset::K_SHOW_BEAR) ? "on" : "off") << ",min prices " << S.D.P.minPrices
      << ",min lots " << S.D.P.minLots << ",refill " << S.D.P.refillPct << "%,within " << S.D.P.refillMs / 1000 << " s,gap " << S.D.P.gapBars << " candles\nBACKLOG_SLICES," << S.backlogSlices
      << ",garbage " << S.garbage << ",book resets " << S.D.bookResets << "\n";
    const std::string body = o.str();
    if (body == S.statusBody && now - S.statusMs < 30000 && now >= S.statusMs) return;
    if (now - S.statusMs < 1000 && now >= S.statusMs) return;
    S.statusMs = now; S.statusBody = body;
    swrMkdir(d); swrMkdir(d + SWR_SEP "SweepRefill");
    const std::string p = d + SWR_SEP "SweepRefill" SWR_SEP "SweepRefill.status-" + S.market + "-" + std::to_string(S.spb) + ".txt", tmp = p + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return; f << body; f.flush(); if (!f) return; }
    std::remove(p.c_str()); std::rename(tmp.c_str(), p.c_str());
}

void SweepRefill::pump(bool fromDraw) {
    SwrState* S = slot_.get(this, true);
    if (!S || S->busy) return;
    S->busy = 1;
    struct Unbusy { SwrState* s; ~Unbusy() { s->busy = 0; } } unbusy = { S };
    (void)fromDraw;
    try {
        const long long now = swrMs();
        identify(*S);
        if (S->market.empty() || !(S->tick > 0) || S->spb <= 0) return;
        settings(*S, now);
        levels(*S, now);
        loadRecord(*S);
        bars(*S);
        openFeed(*S);
        drain(*S, now);
        collect(*S);
        status(*S, now);
    } catch (...) {}
}

int SweepRefill::mouse(RTX_EVENT* e) {
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    SwrState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    int hit = -1;
    for (size_t k = 0; k < S->hits.size() && hit < 0; ++k) { const HitBox& h = S->hits[k]; if (e->v.mouse.h >= h.l && e->v.mouse.h <= h.r && e->v.mouse.v >= h.t && e->v.mouse.v <= h.b) hit = (int)h.idx; }
    if (hit != S->hover) { S->hover = hit; invalidateChart(false); }
    return RTX_FAIL;                                                                  // never swallow the chart's own mouse behaviour
}

static short swrC(int v) { return (short)(v < -32768 ? -32768 : v > 32767 ? 32767 : v); }

void SweepRefill::render(SwrState& S) {
    S.hits.clear();
    const bool showBull = S.set.b(swrset::K_SHOW_BULL), showBear = S.set.b(swrset::K_SHOW_BEAR);
    if ((!showBull && !showBear) || S.marks.empty() || !(S.tick > 0)) return;
    const long n = getBarCount(); if (n < 1) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow); if ((long)times.count < n || (long)hi.count < n || (long)lo.count < n) return;
    RCT pane; pane.getPaneRect(false);
    const int fpt = std::max(8, std::min(14, S.set.i(swrset::K_FONT)));
    FONT f; f.id = HELVETICA; f.size = (short)fpt; f.style = BOLD; setFont(f);
    const int th = fpt + 4, lane = th + 2;
    for (size_t k = 0; k < S.marks.size(); ++k) {
        const swr::Event& e = S.marks[k]; if (!swr::shown(e, showBull, showBear)) continue;
        long l0 = 0, h0 = n - 1, idx = -1;      // the candle: the bar whose time (IRT bar END) == the mark's bar key
        while (l0 <= h0) { const long mid = (l0 + h0) / 2; if ((long long)times[(int)mid] >= e.barT) { idx = mid; h0 = mid - 1; } else l0 = mid + 1; }
        if (idx < 0 || (long long)times[(int)idx] != e.barT) continue;
        const bool up = swr::bullish(e);
        PNT p; p.set((int)idx, up ? hi[(int)idx] : lo[(int)idx]); const int x = p.h;
        // past the reserved lanes (lsBigBidAsk's BO / BD and Rly sit right at the candle): green above the high, red below the low
        const int yEdge = up ? p.v - 2 - LANES_RESERVED * lane : p.v + 2 + LANES_RESERVED * lane;
        const int t = up ? yEdge - th : yEdge, b = t + th;
        if (t <= pane.top + 1 || b >= pane.bottom - 1 || x <= pane.left || x >= pane.right) continue;
        struct tm tkNw; std::memset(&tkNw, 0, sizeof tkNw); const nw::Gate ng = getLocaltime((RTDATE)e.barT, &tkNw) ? nw::atTm(S.market, tkNw) : nw::Gate(); if (ng.feedBad) continue;   // NewsWindow.h
        const std::string txtNw = nw::q("Sr", ng); const char* txt = txtNw.c_str();
        const int w = (int)getTextWidth(txt, -1) + 4, x0 = x - w / 2;
        setTextColor((COLOR)(up ? S.set.c(swrset::K_COL_BULL) : S.set.c(swrset::K_COL_BEAR)));
        RCT r; r.set(swrC(x0), swrC(t), swrC(x0 + w), swrC(b)); r.drawText(txt, true, false);
        HitBox hb; hb.l = x0; hb.t = t; hb.r = x0 + w; hb.b = b; hb.idx = k; S.hits.push_back(hb);
    }
    // the hover box of the mark under the mouse: fixed font, solid border, inside the pane
    if (S.hover >= 0 && (size_t)S.hover < S.marks.size()) {
        const HitBox* at = nullptr; for (size_t k = 0; k < S.hits.size(); ++k) if ((int)S.hits[k].idx == S.hover) at = &S.hits[k];
        if (!at) return;
        const swr::Event& e = S.marks[(size_t)S.hover];
        const std::vector<std::string> rows = swr::hoverRows(e, S.tick, S.D.P.refillMs / 1000, S.D.P.refillPct);
        FONT hf; hf.id = HELVETICA; hf.size = HOVER_PT; hf.style = PLAIN; setFont(hf);
        int w = 0; for (size_t k = 0; k < rows.size(); ++k) w = std::max(w, (int)getTextWidth(rows[k].c_str(), -1));
        w += 12; const int h = (int)rows.size() * HOVER_LH + 8;
        int x0 = at->r + 8, y0 = at->t - h / 2;
        if (x0 + w > pane.right - 2) x0 = at->l - 8 - w;
        if (x0 < pane.left + 2) x0 = pane.left + 2;
        if (y0 + h > pane.bottom - 2) y0 = pane.bottom - 2 - h;
        if (y0 < pane.top + 2) y0 = pane.top + 2;
        const COLOR col = (COLOR)(swr::bullish(e) ? S.set.c(swrset::K_COL_BULL) : S.set.c(swrset::K_COL_BEAR));
        const COLOR bg = getBackgroundColor();
        RCT box; box.set(swrC(x0), swrC(y0), swrC(x0 + w), swrC(y0 + h)); box.draw(1, col, bg, DRAW_OPAQUE, PAT_SOLID);
        for (size_t k = 0; k < rows.size(); ++k) {
            setTextColor(k == 0 ? col : (COLOR)0xE5E7EBul);
            RCT r; r.set(swrC(x0 + 6), swrC(y0 + 4 + (int)k * HOVER_LH), swrC(x0 + w - 4), swrC(y0 + 4 + (int)(k + 1) * HOVER_LH)); r.drawText(rows[k].c_str(), false, false);
        }
    }
}

int SweepRefill::draw(void) {
    pump(true);
    SwrState* S = slot_.get(this, false);
    if (S) { try { render(*S); } catch (...) {} }
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    SweepRefill* p = new SweepRefill();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE);
    p->setExtendedFlags(CALL_CONTINUOUSLY);   // the 3 s refill window is decided and the feed drained while the chart is quiet
    p->setDescription("LRA Sweep Refill v1.0.0 - Sr on the price chart. Green Sr above the candle: sellers swept 8+ bid prices through a level or pivot low at a local low, and the bids came right back (70%+ of the size that rested there, within 3 s) - the sweep was absorbed. Red Sr below the candle: the mirror at a high. Never on two adjacent candles. Decided once, never repainted; every sweep is recorded for the nightly test. Hover a mark for the numbers. Needs the market-by-order feed; works from the moment it is on the chart. Example: NQ 8:55 AM, 19 prices swept through G1 31000, 87% back in 3 s -> green Sr.");
    p->setVersion("1.0.0");   // SWR_VERSION
    return p;
}
