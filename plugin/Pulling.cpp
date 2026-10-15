/********************************************************************************
 *  Pulling.cpp  --  Investor/RT RTX extension  lsPulling  (0.3.0, 2026-10-14 night)
 *  0.3.0 (2026-10-14 night, Rassul via the coordinator: "replace its box with ONE read line, shown only while price is inside a
 *        key-level or gamma zone"; tonight's MBO study research-artifacts\review\MBO-LIQUIDITY-READS-2026-10-14.md Q2v): the header
 *        readout ("Buyers pulled: 0.8x (1.5x)  Sellers pulled: ...") is replaced by ONE read line after "PULLING" (ZoneRead.h 1.0.0):
 *        in the first 3 minutes after price reaches a key / gamma level (zone = level +/- 0.06 ADR, the Liquidity Profile zone rule;
 *        arrival = the first trade within the level band), was the size resting AT the level on the defending side (bids at support,
 *        offers at resistance) CANCELLED or TRADED? "ES 7750 - bids pulled, not filled (62% in 3 min) - less support ?" (red; offers
 *        at resistance mirrored, green) / "ES 7750 - bids traded, held (2.1x normal) - support holding ?" (green; red at resistance) /
 *        "bids: 30% pulled, 40% traded in 3 min ?" (grey) - the study's z >= 1 line vs this market's same-30-min-slot normal
 *        (lsFlexLevels\LRA-ZoneReadNorms-<MKT>.csv), always "?", locked when the 3 minutes end (Pulling\<stem>-zoneread-<session>.csv,
 *        read back on load: no repaint); outside every zone: nothing. Mouse on the line (TRACK_MOUSE): its detail box - the numbers,
 *        the normal, what / why / how / an example, the study result (held ~19 pp less often) and its caveat (NOT YET proven).
 *        Measured natively from this chart's order-by-order feed (the orders' own ids: cancels vs fills), so no proxy is needed; an
 *        order that rested before the indicator loaded is unknown (the hover says "partial"). The two lines, dots, X and MBO % stay.
 *        Description reworded (audit 2026-10-14 s.7): "buyers cancelled bids near price (0.8x normal)" - no "price drops when ...".
 *
 *  PULLING: who pulled their resting orders near price, bar by bar, from the market-by-order feed (Rithmic DBO through IRT's
 *  DEPTH_BY_ORDER). Rassul 2026-10-11 20:50-20:57: "a simple two line approach that is looking at pulling of orders by buyers and
 *  sellers ... it would show why price went up or down and who pulled orders", "call it pulling", "it should use mbo data".
 *  Its own pane, two lines (mockup option A + the B cross, 20:50):
 *    red   = BUYERS pulled (bids cancelled near price)   -> bearish: price drops when buyers step away   (0.1.1: coloured by meaning)
 *    green = SELLERS pulled (offers cancelled near price) -> bullish: price lifts when sellers step away
 *  each as x the normal pulling per side over the 20 bars before (one shared scale), a dot per bar, the value written over a dot at or
 *  over the line (1.5x until the nightly study sets the market's own), and a yellow X where the side that pulled more over the last 10
 *  bars changed. Header: "PULLING  Buyers pulled: 0.8x (1.5x)  Sellers pulled: 2.0x (1.5x)" for the bar in progress, and on the right
 *  how complete the order feed is: "MBO 99%" = the fills it saw / the bar volume over the last 10 closed bars (red under 90%).
 *  What counts as a pull (PullingLogic.h, checked on his own recordings): a cancel (DEL that did not follow a fill emptying it), a
 *  smaller size at the same price, or an order moved AWAY from the market - only bids within `near` ticks under the best bid and
 *  offers within `near` ticks over the best offer. A fill is never a pull.
 *  (0.2.0) MBO RECORDER: every order-by-order event it drains, raw, to C:\Dev\level-reversal-analytics\data\irt\<session>\<MKT>\mbo_HH.csv
 *  (the IRT Reader's dbo format), archived nightly by lra.mbo_live - the reader's own recording stopped after a minute or two on ES / NQ.
 *  Record: lsFlexLevels\Pulling\<MKT>-<sec/bar>-<session>.csv (one line per closed bar), re-read on load for this session and the two
 *  before, so a restart or a scroll back shows what was drawn (the order feed has no history before IRT started).
 *  No settings. Per-market near / line from lsFlexLevels\LRA-PullNorms-<MKT>.csv (PARAM,near,8 / PARAM,line,1.5) when the study
 *  writes one. Per-chart state: HostSlot.h. The DBO stream is opened like lsMboCheck (chart symbol, crash guard, drained in slices).
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
#include "PullingLogic.h"
#include "HostSlot.h"
#include "NearLevels.h"           // (0.3.0) the per-market level band (the Liquidity Profile zone half-width)
#include "ZoneRead.h"             // (0.3.0) the zone read line
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <limits>
#include <map>
#include <new>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#define PL_VERSION "0.3.0"          // keep equal to pl::VERSION and the setVersion literal below

static const COLOR C_BUY = 0x0022C55E, C_SELL = 0x00EF4444, C_GREY = 0x009CA3AF, C_AXIS = 0x00334155, C_X = 0x003CC8FA;
static const int SLICE_MAX = 20000;            // events per call at most (never blocks IRT)
static const long long SLICE_BUDGET_MS = 12;   // and about this many ms per call

#ifdef PL_TEST_CLOCK
extern long long pl_test_now_ms;                                        // (0.3.0) host tests drive the 3-minute window
static long long steadyMs() { return pl_test_now_ms; }
#else
static long long steadyMs() {
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
#endif
static short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
static std::string flexDir() {
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels";
}
static void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    (void)p;
#endif
}
static bool statFile(const std::string& p) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return false;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
#endif
    return (st.st_mode & S_IFMT) == S_IFREG;
}
static bool moveOver(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
static bool publishFile(const std::string& path, const std::string& contents) {
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false;
      f.write(contents.data(), (std::streamsize)contents.size()); f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    const bool hadOld = statFile(path);
    if (hadOld && !moveOver(path, bak)) { std::remove(tmp.c_str()); return false; }
    if (!moveOver(tmp, path)) { if (hadOld) moveOver(bak, path); std::remove(tmp.c_str()); return false; }
    return true;
}
static bool readFileBounded(const std::string& path, std::string& out, size_t cap) {
    std::ifstream f(path.c_str(), std::ios::binary); if (!f) return false;
    f.seekg(0, std::ios::end); const std::streamoff n = f.tellg();
    if (n < 0 || (unsigned long long)n > (unsigned long long)cap) return false;
    f.seekg(0, std::ios::beg); out.assign((size_t)n, '\0');
    return !n || (f.read(&out[0], n) && (size_t)f.gcount() == (size_t)n);
}
static bool appendLines(const std::string& path, const std::string& header, const std::vector<std::string>& lines) {
    // A closed-bar publication is small and infrequent.  Rebuild a bounded file
    // through publishFile so a reader sees either its old complete contents or
    // the new complete contents, never a partially appended final record.
    static const size_t RECORD_CAP = 4u * 1024u * 1024u;
    if (lines.empty()) return true;
    std::string contents;
    if (statFile(path)) {
        if (!readFileBounded(path, contents, RECORD_CAP)) return false;
    } else {
        if (header.size() + 1 > RECORD_CAP) return false;
        contents = header + "\n";
    }
    for (size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].size() > 512 || contents.size() > RECORD_CAP - lines[i].size() - 1) return false;
        contents += lines[i]; contents += "\n";
    }
    return publishFile(path, contents);
}
static std::vector<std::string> split(const std::string& s, char c) {
    std::vector<std::string> f; size_t a = 0;
    for (size_t i = 0; i <= s.size(); ++i) if (i == s.size() || s[i] == c) { f.push_back(s.substr(a, i - a)); a = i + 1; }
    return f;
}
static bool parseD(const std::string& t, double& out) {
    if (t.empty() || t.size() > 40) return false;
    errno = 0; char* e = nullptr; const double v = std::strtod(t.c_str(), &e);
    if (errno || e != t.c_str() + t.size() || !pl::finite(v)) return false;
    out = v; return true;
}
static bool parseLL(const std::string& t, long long& out) {
    if (t.empty() || t.size() > 24) return false;
    errno = 0; char* e = nullptr; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno || e != t.c_str() + t.size()) return false;
    out = v; return true;
}
static std::string marketName(std::string r) {
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return r.substr(0, 8);
}
static std::string filePart(const std::string& text) {
    std::string out;
    for (size_t i = 0; i < text.size() && out.size() < 24; ++i) {
        const unsigned char c = (unsigned char)text[i];
        out += (std::isalnum(c) || c == '_' || c == '-') ? (char)c : '_';
    }
    return out.empty() ? "UNKNOWN" : out;
}
static long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}

struct PLState {
    pl::Engine e;
    cppExtension::DEPTH_BY_ORDER* dbo = nullptr;
    cppExtension::MARKET_DEPTH* md = nullptr;
    bool busy = false, dboDead = false, mdDead = false, blocked = false, faulted = false, depthWasAvailable = false;
    std::string identity, market, symbol, state = "starting", lastStatusText, params = "standard";
    int spb = 0; double tick = 0;
    long count = 0;                               // bars known at the last pump
    long long session = -1, openedMs = -1, lastEventMs = -1, lastDepthMs = -1, lastStatusMs = -1, lastNormsMs = -1;
    long long slices = 0, capped = 0, recFails = 0, mboRows = 0, mboFails = 0;   // (0.2.0) the raw order-by-order recorder
    std::map<long long, pl::Bar> hist;            // the record: bar time -> what was counted (this session and the two before)
    std::map<long long, double> vol;              // bar time -> the chart's volume of that bar (the coverage check)
    std::map<long long, int> barEnds;             // chart bar end time -> index; native event timestamps route to their bar
    long long chartLastEnd = 0;
    std::vector<std::string> pending;             // closed bars not yet written
    zread::Host zr; long zrCount = -1; long long zrSession = -1; double atr = 0;   // (0.3.0) the zone read line
    int lineL = 0, lineT = 0, lineR = 0, lineB = 0; bool hoverOn = false;          // (0.3.0) where the line was drawn; the mouse on it
    ~PLState() { delete dbo; delete md; }
};

class Pulling : public cppExtension {
public:
    Pulling() : cppExtension() {}
    virtual int draw(void);
    virtual int parmsLoad(void)  { return RTX_OK; }          // no settings
    virtual int parmsApply(void) { return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { return RTX_OK; }
    virtual int mouse(RTX_EVENT* e);                                    // (0.3.0) the read line's hover
    void pump();
    void release() { slot_.release(this); }
    PLState* testState() { return slot_.get(this, false); }
private:
    HostSlot<PLState> slot_;
    long long ctSec(RTDATE d);
    void identify(PLState& S);
    void open(PLState& S);
    void drain(PLState& S, long long now);
    void record(PLState& S, const std::map<std::string, std::string>& byFile);
    void readBest(PLState& S, long long now);
    void bars(PLState& S, long count, bool finalize);
    int eventBar(const PLState& S, RTDATE stamp) const;
    void load(PLState& S);
    void norms(PLState& S);
    void status(PLState& S, long long now);
    void render(PLState& S);
    void zone(PLState& S, long count, long long now);                   // (0.3.0)
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + "\\Pulling"; }
    // Market alone is not a contract identity: ESZ and ESH must not share a record/status file.
    std::string stem(PLState& S) { return S.market + "-" + filePart(S.symbol) + "-" + std::to_string(S.spb); }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<Pulling*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<Pulling*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void)   { setParameterVersion(1); return RTX_OK; }
int cppExtension::calc(int)     { static_cast<Pulling*>(this)->pump(); return RTX_OK; }
int Pulling::draw(void) {
    pump();
    PLState* S = slot_.get(this, false);
    if (S && !S->faulted) { try { render(*S); } catch (...) { S->state = "draw error"; } }
    return RTX_OK;
}

long long Pulling::ctSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    return civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
}

void Pulling::identify(PLState& S) {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!pl::finite(tick) || tick <= 0 || tick >= 1000) tick = 0;
    const std::string id = root + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity && tick == S.tick) return;
    delete S.dbo; S.dbo = nullptr; delete S.md; S.md = nullptr;              // a new symbol / bar size: start clean
    S.e = pl::Engine(); S.dboDead = S.mdDead = S.blocked = S.depthWasAvailable = false; S.hist.clear(); S.vol.clear(); S.barEnds.clear(); S.pending.clear();
    S.zr = zread::Host(); S.zrCount = -1; S.zrSession = -1; S.atr = 0; S.hoverOn = false;   // (0.3.0)
    S.identity = id; S.symbol = sym; S.spb = spb; S.tick = tick; S.market = marketName(root.empty() ? sym : root);
    S.count = 0; S.chartLastEnd = 0; S.session = -1; S.openedMs = S.lastEventMs = S.lastDepthMs = S.lastNormsMs = S.lastStatusMs = -1; S.lastStatusText.clear();
    pl::defaults(S.market, S.e.P); S.params = "standard";
}

// the order stream (and the depth book for the best bid / offer) - chart context only, behind a crash guard
void Pulling::open(PLState& S) {
    if (S.dbo || S.dboDead || S.blocked) return;
    const std::string d = dir();
    const std::string guard = d.empty() ? std::string() : d + "\\_opening-" + stem(S) + ".txt";
    if (!guard.empty() && statFile(guard)) { S.blocked = true; S.state = "opening the order feed crashed IRT before: delete " + guard + " to retry"; return; }
    if (!guard.empty()) { makeDir(flexDir()); makeDir(d); std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening DEPTH_BY_ORDER\n"; }
    try {
        S.dbo = new (std::nothrow) DEPTH_BY_ORDER(S.symbol.empty() ? NULL : S.symbol.c_str());
        if (!S.md && !S.mdDead) S.md = new (std::nothrow) MARKET_DEPTH(NULL);
    } catch (...) { S.dboDead = true; }
    if (!guard.empty()) std::remove(guard.c_str());
    S.openedMs = steadyMs();
}

void Pulling::readBest(PLState& S, long long now) {
    if (!S.md || S.mdDead || S.tick <= 0) return;
    if (S.lastDepthMs >= 0 && now - S.lastDepthMs < 250 && now >= S.lastDepthMs) return;
    S.lastDepthMs = now;
    try {
        if (!S.md->available() || S.md->maxLevels < 1) {
            // DEPTH_BY_ORDER exposes neither a snapshot marker nor a sequence or
            // reconnect callback in the supplied real SDK.  A depth outage is
            // the observable reset boundary: discard identities so stale/replayed
            // DELs cannot turn into pulling after a reconnect.
            if (S.depthWasAvailable) { S.e.invalidateBook(); S.depthWasAvailable = false; }
            return;
        }
        S.depthWasAvailable = true;
        const DEPTH_LEVEL L = (*S.md)[0u];
        pl::Tick bid = 0, ask = 0;
        if (pl::toTick(L.bid, S.tick, bid) && pl::toTick(L.ask, S.tick, ask) && bid > 0 && ask > bid) S.e.setBest(bid, ask);
    } catch (...) { if (S.depthWasAvailable) S.e.invalidateBook(); S.depthWasAvailable = false; S.mdDead = true; }
}

void Pulling::drain(PLState& S, long long now) {
    if (!S.dbo || S.dboDead || S.tick <= 0) return;
    const long long t0 = steadyMs(); int n = 0;
    std::map<std::string, std::string> rec;                              // (0.2.0) file -> this slice's raw lines (one append per file)
    try {
        while (true) {
            if (n >= SLICE_MAX || ((n & 511) == 511 && steadyMs() - t0 > SLICE_BUDGET_MS)) { S.capped++; break; }
            if (S.dbo->getNext() != RTX_OK) break;
            const DEPTH_ORDER& o = S.dbo->dbo;
            {   // (0.2.0, Rassul 21:43 "i think there is edge in mbo data and i would like to identify and use it") every event, raw, in
                // the IRT Reader's dbo format, to data\irt\<session>\<MKT>\mbo_HH.csv - lra.mbo_live archives it nightly
                struct tm t; std::memset(&t, 0, sizeof(t));
                if (getLocaltime(o.timestamp, &t)) {
                    static const char* ACT[4] = { "NEW", "MOD", "DEL", "FILL" };
                    long long dd = civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) + (t.tm_hour >= 17 ? 1 : 0);
                    for (int k = 0; k < 3 && ((dd + 4) % 7 + 7) % 7 % 6 == 0; ++k) dd++;         // Saturday / Sunday -> Monday (the reader's naming)
                    const long long z = dd + 719468, era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
                    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
                    const unsigned mp = (5 * doy + 2) / 153, day = doy - (153 * mp + 2) / 5 + 1, mon = mp < 10 ? mp + 3 : mp - 9; const long long yr = (long long)yoe + era * 400 + (mon <= 2);
                    char path[300], line[220];
                    std::snprintf(path, sizeof(path), "C:\\Dev\\level-reversal-analytics\\data\\irt\\%04lld-%02u-%02u\\%s\\mbo_%02d.csv", yr, mon, day, S.market.c_str(), t.tm_hour);
                    std::snprintf(line, sizeof(line), "%04d-%02d-%02d %02d:%02d:%02d|%s|%.10g|%.10g|%lu|%d|%d|%c|%.10g|%.10g\n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
                                  o.action < 4 ? ACT[o.action] : "?", (double)o.price, (double)o.previousPrice, (unsigned long)o.size, o.orderID, o.aggressorOrderID,
                                  o.buySell == 1 ? 'B' : o.buySell == 0 ? 'S' : '?', (double)o.bid, (double)o.ask);
                    rec[path] += line;
                }
            }
            if (o.action > 3 || o.buySell > 1 || !pl::finite(o.price) || o.price <= 0) { n++; continue; }
            pl::Ev x; x.action = o.action; x.side = o.buySell == 1 ? pl::BID : pl::OFFER;
            if (!pl::toTick(o.price, S.tick, x.px)) { n++; continue; }
            if (pl::finite(o.previousPrice) && o.previousPrice > 0) pl::toTick(o.previousPrice, S.tick, x.prev);
            x.size = (double)o.size; x.id = (long long)o.orderID;
            if (pl::finite(o.bid) && pl::finite(o.ask) && o.bid > 0 && o.ask > o.bid) {
                pl::Tick bid = 0, ask = 0;
                if (pl::toTick(o.bid, S.tick, bid) && pl::toTick(o.ask, S.tick, ask) && ask > bid) { x.bid = bid; x.ask = ask; }
            }
            S.zr.e.onEvent(now, (int)x.action, x.side == pl::BID ? zread::BID : zread::ASK, x.id, x.px, (long long)x.size);   // (0.3.0) in order, to the zone read
            const int originalBar = S.e.cur;
            const int target = o.timestamp ? eventBar(S, o.timestamp) : originalBar;
            // Only the prior forming bar and newer bars are still pending durable
            // closure.  Do not make old queued data repaint a closed record.
            if (target < 0 || (S.count > 0 && target < (int)S.count - 1)) { n++; continue; }
            if (target != originalBar) S.e.setBar(target);
            S.e.onEvent(x); n++;
            if (target != originalBar) S.e.setBar(originalBar);
        }
    } catch (...) { S.dboDead = true; S.state = "order feed read fault"; }
    if (n > 0) { S.lastEventMs = now; S.slices++; }
    if (!rec.empty()) record(S, rec);
}

// (0.2.0) one append per hour file per slice; the folder is made on the first write of a session / market; a failed write is counted,
// its lines dropped (never retried in a loop - the feed keeps coming)
void Pulling::record(PLState& S, const std::map<std::string, std::string>& byFile) {
#if defined(_WIN32)
    for (std::map<std::string, std::string>::const_iterator i = byFile.begin(); i != byFile.end(); ++i) {
        const std::string& p = i->first; const size_t a = p.rfind('\\'); if (a == std::string::npos) continue;
        const std::string md = p.substr(0, a), sd = md.substr(0, md.rfind('\\'));
        CreateDirectoryA(sd.c_str(), NULL); CreateDirectoryA(md.c_str(), NULL);
        const bool fresh = !statFile(p);
        std::ofstream f(p.c_str(), std::ios::binary | std::ios::app);
        if (!f) { S.mboFails++; continue; }
        if (fresh) f << "time|action|price|prev_price|size|order_id|aggressor_id|side|bid|ask\n";
        f << i->second; f.flush();
        if (!f) { S.mboFails++; continue; }
        S.mboRows += (long long)std::count(i->second.begin(), i->second.end(), '\n');
    }
#else
    for (std::map<std::string, std::string>::const_iterator i = byFile.begin(); i != byFile.end(); ++i) S.mboRows += (long long)std::count(i->second.begin(), i->second.end(), '\n');
#endif
}

// Prepare the forming bar before draining, then finalize only after draining.
// IRT bar timestamps are bar ends, so an event exactly at an end belongs to the
// next bar (upper_bound), while an event one second before it belongs to the bar
// that is about to close.
int Pulling::eventBar(const PLState& S, RTDATE stamp) const {
    const std::map<long long, int>::const_iterator i = S.barEnds.upper_bound((long long)stamp);
    if (i == S.barEnds.end() || S.spb <= 0 || i->first - (long long)stamp > S.spb) return -1;
    return i->second;
}

// A new bar closes the former forming bar.  The two-phase call lets queued MBO
// messages for that former bar reach its record before the record is published.
void Pulling::bars(PLState& S, long count, bool finalize) {
    if (count < 1) return;
    RTARRAYI times(barDateTime), vols(barVolume);
    if (times.count < count) return;
    if (!finalize) {
        bool rebuild = S.count == 0;
        if (!rebuild && (count < S.count || S.chartLastEnd != (long long)times[(int)S.count - 1])) {
            // The chart was rebuilt/rebased without a symbol change.  Indices are
            // no longer identities; restore persisted values by bar end instead.
            S.e.bars.clear(); S.barEnds.clear(); S.count = 0; rebuild = true;
        }
        const long first = rebuild ? std::max(0L, count - 1500) : std::max(0L, S.count - 1);
        for (long i = first; i < count; ++i) {
            const long long bt = (long long)times[(int)i];
            S.barEnds[bt] = (int)i;
            if (rebuild && i < count - 1) {
                std::map<long long, pl::Bar>::const_iterator h = S.hist.find(bt);
                if (h != S.hist.end()) S.e.bars[(int)i] = h->second;
            }
            if (vols.count >= count && i < count - 1) S.vol[bt] = (double)vols[(int)i];
        }
        while (S.barEnds.size() > 1500) S.barEnds.erase(S.barEnds.begin());
        S.chartLastEnd = (long long)times[(int)count - 1];
        S.e.setBar((int)count - 1);
        return;
    }
    if (S.count > 0 && count > S.count) {
        for (long i = S.count - 1; i < count - 1; ++i) {
            if (i < 0) continue;
            const long long bt = (long long)times[(int)i];
            const double v = vols.count >= count ? (double)vols[(int)i] : 0; S.vol[bt] = v;
            std::map<int, pl::Bar>::const_iterator b = S.e.bars.find((int)i);
            if (b == S.e.bars.end() || S.hist.count(bt)) continue;
            S.hist[bt] = b->second;
            char line[200]; std::snprintf(line, sizeof(line), "%lld|%.0f|%.0f|%.0f|%.0f|%.0f|%.0f|%lld", bt, b->second.pull[pl::BID], b->second.pull[pl::OFFER],
                b->second.stack[pl::BID], b->second.stack[pl::OFFER], b->second.filled, v, b->second.events);
            S.pending.push_back(line);
        }
    }
    S.count = count;
    S.e.setBar((int)count - 1);
    if (!S.pending.empty() && S.session >= 0) {
        const std::string d = dir(); makeDir(flexDir()); makeDir(d);
        if (!d.empty() && appendLines(d + "\\" + stem(S) + "-" + std::to_string(S.session) + ".csv",
                "bar_time|bids_pulled|offers_pulled|bids_stacked|offers_stacked|filled|bar_volume|events", S.pending)) S.pending.clear();
        else S.recFails++;
    }
    while (S.hist.size() > 3000) S.hist.erase(S.hist.begin());
    while (S.vol.size() > 3000) S.vol.erase(S.vol.begin());
}

void Pulling::load(PLState& S) {
    const std::string d = dir(); if (d.empty()) return;
    for (long long k = S.session - 2; k <= S.session; ++k) {
        std::ifstream in((d + "\\" + stem(S) + "-" + std::to_string(k) + ".csv").c_str());
        std::string line; int n = 0;
        while (std::getline(in, line) && ++n < 20000) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const std::vector<std::string> c = split(line, '|');
            long long bt = 0, ev = 0; double v[6];
            if (c.size() != 8 || !parseLL(c[0], bt) || bt <= 0 || !parseLL(c[7], ev)) continue;
            bool ok = true; for (int i = 0; i < 6; ++i) if (!parseD(c[i + 1], v[i]) || v[i] < 0) ok = false;
            if (!ok) continue;
            pl::Bar b; b.pull[pl::BID] = v[0]; b.pull[pl::OFFER] = v[1]; b.stack[pl::BID] = v[2]; b.stack[pl::OFFER] = v[3]; b.filled = v[4]; b.events = ev;
            S.hist[bt] = b; S.vol[bt] = v[5];
        }
    }
}

// the market's own near / line once the nightly study writes them
void Pulling::norms(PLState& S) {
    const std::string f = flexDir(); if (f.empty()) return;
    std::ifstream in((f + "\\LRA-PullNorms-" + S.market + ".csv").c_str()); if (!in) return;
    std::string line, used; int n = 0;
    while (std::getline(in, line) && ++n < 200) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> c = split(line, ',');
        double v = 0; if (c.size() < 3 || c[0] != "PARAM" || !parseD(c[2], v)) continue;
        if (c[1] == "near" && v >= 1 && v <= 200) { S.e.P.near = (int)v; used += " near " + c[2]; }
        else if (c[1] == "line" && v >= 0.5 && v <= 10) { S.e.P.line = v; used += " line " + c[2]; }
    }
    if (!used.empty()) S.params = "data:" + used;
}

void Pulling::status(PLState& S, long long now) {
    if (S.lastStatusMs >= 0 && now - S.lastStatusMs < 1000 && now >= S.lastStatusMs) return;
    S.lastStatusMs = now;
    std::ostringstream o; o.imbue(std::locale::classic());
    const int b = (int)S.count - 1;
    o << "VERSION," << PL_VERSION << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nTICK," << S.tick
      << "\nSTATE," << S.state << "\nFEED," << (S.blocked ? "blocked" : S.dboDead ? "fault" : !S.dbo ? "not opened" : S.lastEventMs < 0 ? "no events yet" : "events")
      << ",events " << S.e.events << ",unknown " << S.e.unknown << ",slices " << S.slices << ",capped " << S.capped << ",book " << S.e.book.size()
      << "\nBOOK,resets " << S.e.bookResets << ",suppressed_unknown " << S.e.suppressed << ",unseen_delete_counting " << (S.e.countUnseenDeletes ? "initial-only" : "off-after-reset")
      << "\nBEST," << (S.e.haveBest ? (double)S.e.bestBid * S.tick : 0) << "," << (S.e.haveBest ? (double)S.e.bestAsk * S.tick : 0)
      << "\nPARAMS,near " << S.e.P.near << ",line " << S.e.P.line << "," << S.params
      << "\nNOW,bids_pulled_x " << S.e.x(b, pl::BID) << ",offers_pulled_x " << S.e.x(b, pl::OFFER) << ",leader " << S.e.leader(b)
      << "\nRECORD,bars " << S.hist.size() << ",fails " << S.recFails << "\nMBO_RECORDER,rows " << S.mboRows << ",fails " << S.mboFails << ",data\\irt\\<session>\\" << S.market << "\\mbo_HH.csv\n"
      << S.zr.status() << "\n";                                           // (0.3.0)
    const std::string text = o.str();
    if (text == S.lastStatusText) return;
    const std::string f = flexDir(); if (f.empty()) return;
    if (publishFile(f + "\\Pulling.status-" + stem(S) + ".txt", text)) S.lastStatusText = text;
}

void Pulling::pump() {
    PLState* P = slot_.get(this, true);
    if (!P || P->busy) return;
    PLState& S = *P;
    S.busy = true;
    struct Unbusy { PLState* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
    try {
        identify(S);
        if (S.tick <= 0) { S.state = "no tick size from IRT"; return; }
        const long long now = steadyMs();
        const long long ct = ctSec(currentDate());
        if (ct > 0) {
            const long long key = (ct - 17LL * 3600) / 86400LL + 1;      // 17:00 CT starts the next day's session
            if (key != S.session) {
                S.session = key; load(S); S.count = 0; S.chartLastEnd = 0;
                S.e.bars.clear(); S.barEnds.clear();
            }
        }
        if (S.lastNormsMs < 0 || now - S.lastNormsMs >= 60000) { S.lastNormsMs = now; norms(S); }
        open(S);
        const long count = getBarCount();
        bars(S, count, false);                                           // index/forming bar before draining native events
        readBest(S, now);
        drain(S, now);
        bars(S, count, true);                                            // only now close/persist the former forming bar
        zone(S, count, now);                                             // (0.3.0) the zone read line
        S.state = S.blocked ? S.state : S.dboDead ? "order feed fault" : !S.dbo ? "order feed not opened"
                : S.lastEventMs < 0 ? (now - S.openedMs < 60000 ? "waiting for the order feed" : "no order feed (MBO) on this chart") : "ready";
        status(S, now);
    } catch (...) { S.state = "calculation error"; }
}

// (0.3.0) the zone read: levels / normals (files), the lagged ADR + ATR + closes (the chart's bars, on a new bar), this session's
// record (read back on load), the chart's close as the price when no fill came for 5 s, then the clock (CT minute -> the normal's slot)
void Pulling::zone(PLState& S, long count, long long now) {
    S.zr.pane = zread::P_PULLING; S.zr.e.tick = S.tick;
    const std::string fd = flexDir();
    S.zr.files(fd.empty() ? fd : fd + "\\", S.market, now, (long long)std::time(NULL));
    if (count != S.zrCount && count >= 2) {
        RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
        if (times.count >= count && hi.count >= count && lo.count >= count && cl.count >= count) {
            S.zrCount = count;
            std::vector<double> rg;
            for (int i = (int)count - 2; i >= 0 && rg.size() < 20; --i) rg.push_back((double)hi[i] - (double)lo[i]);
            S.atr = nl::atrOf(rg, S.spb);
            std::vector<zread::BarRow> rows; rows.reserve((size_t)std::min(count, 12000L));
            for (long i = std::max(0L, count - 12000L); i < count - 1; ++i) {
                const long long cs = ctSec((RTDATE)times[(int)i]); if (cs <= 0) continue;
                zread::BarRow r; r.session = zrule::sessionKey(cs); r.endSec = cs; r.h = (double)hi[(int)i]; r.l = (double)lo[(int)i]; r.c = (double)cl[(int)i]; rows.push_back(r);
            }
            const long long cs = ctSec((RTDATE)times[(int)count - 1]);
            if (cs > 0) { S.zrSession = zrule::sessionKey(cs); S.zr.bars(rows, S.zrSession, nl::halfWidth(nl::zoneFor(S.market), S.atr, S.tick)); }
        }
    }
    const long long wall = ctSec(currentDate());
    const int minute = wall > 0 ? (int)((wall % 86400 + 86400) % 86400 / 60) : -1;
    const std::string d = dir();
    if (S.zrSession >= 0 && !d.empty()) { makeDir(fd); makeDir(d); S.zr.newSession(S.zrSession, d + "\\" + stem(S) + "-zoneread-" + std::to_string(S.zrSession) + ".csv", now, wall); }
    if (S.zr.e.lastTradeMs < 0 || now - S.zr.e.lastTradeMs > 5000 || now < S.zr.e.lastTradeMs) {
        RTARRAY cl(barClose); pl::Tick px = 0;
        if (cl.count >= count && count > 0 && pl::toTick((double)cl[(int)count - 1], S.tick, px)) S.zr.e.onPrice(now, px);
    }
    S.zr.e.step(now, wall > 0 ? wall : 0, minute);
    S.zr.flush();
}

// (0.3.0) the mouse on the read line shows its detail box; never swallows the event
int Pulling::mouse(RTX_EVENT* e) {
    if (!e) return RTX_FAIL;
    PLState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    if (e->type == E_MOUSE_MOVE || e->type == E_MOUSE_HOVER) {
        const int h = e->v.mouse.h, v = e->v.mouse.v;
        const bool in = S->lineR > S->lineL && h >= S->lineL && h <= S->lineR && v >= S->lineT && v <= S->lineB;
        if (in != S->hoverOn) { S->hoverOn = in; invalidateChart(false); }
    }
    return RTX_FAIL;
}

void Pulling::render(PLState& S) {
    RCT pane; pane.getPaneRect(false);
    const long count = getBarCount();
    if (count < 2 || pane.bottom - pane.top < 40 || pane.right - pane.left < 120) return;
    const int lastBar = (int)count - 1;
    FONT f; f.id = HELVETICA; f.size = 9; f.style = BOLD; setFont(f);
    auto txt = [&](int x, int y, const std::string& t, COLOR c) { setTextColor(c); const int w = getTextWidth(t.c_str(), -1); RCT r; r.set(coord(x), coord(y), coord(x + w + 2), coord(y + 12)); r.drawText(t.c_str(), false, false); return x + w; };
    auto xs = [](double v) { char b[16]; if (v < 0) std::snprintf(b, sizeof(b), "-"); else std::snprintf(b, sizeof(b), "%.1fx", v); return std::string(b); };
    auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
    const double line = S.e.P.line;
    // header: "PULLING", then (0.3.0) the ONE zone read line (only inside a key / gamma zone; else nothing), the order feed's
    // completeness on the right (MBO %)
    zread::Line zl = S.zr.e.line(zread::P_PULLING, steadyMs());
    std::vector<std::string> hov; int hovX = 0, hovY = 0, hovW = 0;
    {
        std::string feed; COLOR fc = C_GREY;
        if (S.lastEventMs < 0) { feed = S.state == "ready" ? "MBO" : S.state; fc = S.state.find("waiting") == 0 ? C_GREY : C_SELL; }
        else {
            RTARRAYI times(barDateTime); double fil = 0, vol = 0; int n = 0;
            for (int i = lastBar - 1; i >= 0 && n < 10 && times.count > i; --i) {
                std::map<int, pl::Bar>::const_iterator b = S.e.bars.find(i);
                std::map<long long, double>::const_iterator v = S.vol.find((long long)times[i]);
                if (b == S.e.bars.end() || v == S.vol.end() || v->second <= 0) continue;
                fil += b->second.filled; vol += v->second; ++n;
            }
            if (n == 0 || vol <= 0) feed = "MBO";
            else { char b[40]; const double c = std::min(1.0, fil / vol); std::snprintf(b, sizeof(b), c >= 0.9 ? "MBO %.0f%%" : "MBO gaps %.0f%%", c * 100.0); feed = b; fc = c >= 0.9 ? C_GREY : C_SELL; }
        }
        const int fw = getTextWidth(feed.c_str(), -1);
        txt(pane.right - fw - 8, pane.top + 1, feed, fc);
        const int x = txt(pane.left + 4, pane.top + 1, "PULLING   ", C_GREY);
        S.lineL = S.lineR = 0;
        if (zl.show) {
            FONT lf; lf.id = HELVETICA; lf.size = 10; lf.style = BOLD; setFont(lf);
            const int room = pane.right - fw - 16 - x;
            std::string t = zl.text;
            if (getTextWidth(t.c_str(), -1) > room) { while (t.size() > 4 && getTextWidth((t + "...").c_str(), -1) > room) t.pop_back(); t += "..."; }
            if (room > 40) {
                const int w = txt(x, pane.top + 1, t, zl.impl > 0 ? C_BUY : zl.impl < 0 ? C_SELL : C_GREY) - x;
                S.lineL = x; S.lineT = pane.top; S.lineR = x + w + 2; S.lineB = pane.top + 14;
                if (S.hoverOn && !zl.hover.empty()) {                  // the detail box: under the line, inside the pane, drawn last
                    FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
                    const int maxW = std::max(120, std::min(700, (int)(pane.right - pane.left) - 16));
                    for (size_t i = 0; i < zl.hover.size(); ++i) {
                        std::string rest = zl.hover[i];
                        while (!rest.empty() && hov.size() < 24) {
                            size_t cut = rest.size();
                            while (cut > 1 && getTextWidth(rest.substr(0, cut).c_str(), -1) > maxW - 12) { const size_t sp = rest.rfind(' ', cut - 1); cut = sp == std::string::npos || sp == 0 ? cut - 1 : sp; }
                            hov.push_back(rest.substr(0, cut)); rest = cut < rest.size() ? rest.substr(cut + (rest[cut] == ' ' ? 1 : 0)) : std::string();
                        }
                    }
                    for (size_t i = 0; i < hov.size(); ++i) hovW = std::max(hovW, (int)getTextWidth(hov[i].c_str(), -1));
                    hovW += 12; hovX = std::max((int)pane.left + 2, std::min(x, (int)pane.right - hovW - 2)); hovY = pane.top + 16;
                }
            }
            setFont(f);
        } else S.hoverOn = false;
    }
    const int top = pane.top + 16, bottom = pane.bottom - 4;
    // the visible bars and the scale (at least 3x)
    std::vector<int> vis;
    for (int i = lastBar; i >= 0; --i) { const int x = xOf(i); if (x < pane.left) break; if (x <= pane.right) vis.push_back(i); }
    std::reverse(vis.begin(), vis.end());
    double mx = std::max(3.0, line * 1.6);
    for (size_t k = 0; k < vis.size(); ++k) for (int s = 0; s < 2; ++s) mx = std::max(mx, S.e.x(vis[k], s));
    mx = std::min(mx * 1.2, 12.0);                                     // headroom for the value over the highest dot
    auto yOf = [&](double v) { return bottom - (int)((bottom - top) * std::min(1.0, std::max(0.0, v / mx))); };
    setPen(C_AXIS, 1, P_SOLID);
    { const int y = yOf(line); PNT a; a.set(0, 0.0f); a.h = coord(pane.left); a.v = coord(y); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(pane.right); b.v = coord(y); b.drawLineTo();
      txt(pane.right - 34, y - 13, xs(line), C_GREY); }
    // the two lines, a dot per bar, the value over a dot at or over the line. (0.1.1, Rassul 21:11 "if buyers pulled it should be red, if
    // sellers pulled it should be green") coloured by what it means for price: buyers pulling bids = bearish RED, sellers pulling
    // offers = bullish GREEN
    for (int s = 0; s < 2; ++s) {
        const int side = s == 0 ? pl::OFFER : pl::BID; const COLOR c = side == pl::BID ? C_SELL : C_BUY;
        setPen(c, 2, P_SOLID); bool started = false;
        for (size_t k = 0; k < vis.size(); ++k) {
            const double v = S.e.x(vis[k], side); if (v < 0) { started = false; continue; }
            PNT p; p.set(0, 0.0f); p.h = coord(xOf(vis[k])); p.v = coord(yOf(v));
            if (!started) { p.setDrawPosition(); started = true; } else p.drawLineTo();
        }
        for (size_t k = 0; k < vis.size(); ++k) {
            const double v = S.e.x(vis[k], side); if (v < 0) continue;
            const int x = xOf(vis[k]), y = yOf(v);
            RCT d; d.set(coord(x - 2), coord(y - 2), coord(x + 2), coord(y + 2)); d.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID);
            if (v + 1e-9 >= line) { const std::string t = xs(v); const int w = getTextWidth(t.c_str(), -1); txt(x - w / 2, y - 15, t, c); }
        }
    }
    // the yellow X: the side that pulled more over the last 10 bars changed here
    for (size_t k = 0; k < vis.size(); ++k) {
        if (!S.e.cross(vis[k])) continue;
        const int x = xOf(vis[k]);
        setPen(C_X, 1, P_SOLID);
        { PNT a; a.set(0, 0.0f); a.h = coord(x); a.v = coord(top); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(x); b.v = coord(bottom); b.drawLineTo(); }
        txt(x - 3, top, "X", C_X);
    }
    if (!hov.empty()) {                                                  // (0.3.0) the read line's detail box, over the lines
        FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
        const int H = (int)hov.size() * 14 + 8, y1 = std::min((int)pane.bottom - 2, hovY + H);
        setPen(C_GREY, 1, P_SOLID);
        RCT bx; bx.set(coord(hovX), coord(hovY), coord(hovX + hovW), coord(y1)); bx.draw(1, C_GREY, 0x00000000, DRAW_OPAQUE, PAT_SOLID);
        for (size_t i = 0; i < hov.size() && hovY + 4 + (int)i * 14 + 12 <= y1; ++i) txt(hovX + 6, hovY + 4 + (int)i * 14, hov[i], i == 0 ? (zl.impl > 0 ? C_BUY : zl.impl < 0 ? C_SELL : C_GREY) : (COLOR)0x00E2E8F0);
    }
}

extern "C" cppExtension* CreateExtension(void) {
    Pulling* p = new Pulling();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE);                     // its own pane, draws itself; (0.3.0) the read line's hover
    p->setExtendedFlags(CALL_CONTINUOUSLY);                              // the order feed keeps coming while no trade prints
    p->setDescription("LRA Pulling: who pulled their resting orders near price, per bar, from the order-by-order (MBO) feed. Red = buyers cancelled bids near price (e.g. 0.8x normal), green = sellers cancelled offers near price, each x the normal pulling of the 20 bars before; the value shows at or over the line; a yellow X = the side pulling more over the last 10 bars changed. Fills never count as pulls. MBO % (top right) = how much of the bar volume the order feed showed. The read line (only while price is in a key / gamma zone): in the first 3 minutes at the level, was the size resting there cancelled or traded - always '?', locked after the 3 minutes; hover it for the numbers and the study caveat.");
    p->setVersion(PL_VERSION);
    return p;
}
