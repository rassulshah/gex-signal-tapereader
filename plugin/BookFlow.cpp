/********************************************************************************
 *  BookFlow.cpp  --  Investor/RT RTX extension  lsBookFlow  1.5.0  (2026-10-19; 1.0.0 2026-10-16)
 *  1.5.0 (2026-10-19, Rassul 11:05 "build", 11:19 "show Refill Effort, Refilled X and Pace. The rest can be there but invisible ... keep the
 *        read below", 11:20 "remove the questions", 11:15 "use 4.5x instead of percentage", 11:53 "lets just build and replace the pace.
 *        also on the labels"): the box shows REFILL EFFORT (the net = added - pulled, vs its 20-candle average; was "Net") and REFILLED
 *        (lots re-posted at the exact prices that were hit, within 1 s, / lots eaten there, / the market's normal (12:22 CT): "2.0x STRONG";
 *        STRONG / weak = the market's top / bottom quarter, bf::REFILL_CUTS) + the Read. Eaten, Added, Pulled and Pace are computed, recorded and used by the Read but
 *        not shown. The bright bars' second label is Refilled ("R4.3") instead of the pace. The Read leads with the REFILL read when the
 *        shown candle made a new 10-bar low / high (bf::refillRead: "C+R ?" busy + defending side 2x + strong; strong; weak = "defense
 *        failing, don't fade ?"), else the 1.4.0 stacking read. Bars, pull markers and every record unchanged.
 *        (12:35 CT, Rassul "in the settings for bookflow, show these two questions as well as these example for each using numerical
 *        values and how to read") the description: Refill Effort / Refilled / Read, each with its question, how, a numeric example, how to read.
 *  1.4.0 (2026-10-18 23:48 CT, Rassul approved bookflow-read-mockup.png): box rows Net, Pace, READ - one word (BULLISH / BEARISH / NEUTRAL)
 *        + up to 2 lines from the Net ratios (bids / offers arriving, leaving or flat; the stronger side decides; bf::bookRead), wrapped to
 *        the box (BoxReadDraw.h). The live forming candle's Net now carries its ratio so far (bf::liveNetStat, never a decision).
 *        (23:56 CT, bookflow-pull2-mockup.png) + PULL MARKERS: a closed candle whose side pulled >= 2.0x its plain 20-candle average of
 *        pulled lots AND >= 1.2x what it added gets a block in the side's colour + a yellow "P {ratio}" in the bottom strip (decided once
 *        at the close, kept; never on the forming candle); box rows Net, Added, Pulled (their plain ratios), Pace, Read; the Read names
 *        pulling first (bf::bookRead). Calibration ES Fri 10-16 RTH: 0 markers at 2.0x / 1.2x (pulled / added max 0.89) - reported.
 *  1.3.4 (2026-10-18 23:19 CT): the box follows the mouse only while the cursor is inside this pane (PaneCursor.h), else Live.
 *  1.3.0 (2026-10-18, Rassul approved the audit-A fixes; colour by side decided by Rassul tonight):
 *   - BRIGHT + LABEL FROM THE NET THAT IS DRAWN: a side is a spike when its |net| is >= "Spike vs average" (2.0x) the plain average of
 *     that side's |net| over the prior "Ratio bars" (20) closed candles - THE spike rule of both order-flow panes (Aggression the same),
 *     decided once at the close (BookFlowLogic.h netStat). The "1.5x the prior biggest" jump is gone from the drawn rule (it stays in the
 *     old StackStats / PullStats record words only). Was: the added-only / pulled-only part's spike, so +900 / -880 drew a tiny +20 bright.
 *   - NO REPAINT: the forming candle is drawn dim with no label until it closes.
 *   - COLOUR BY SIDE: bid-side bars green (buyers), offer-side bars red (sellers), above (arriving) or below (leaving) the grey zero line;
 *     the meaning is in the description. Label: the ratio (yellow) and, for size arriving, the pace (grey) with no "p", outside the bar.
 *   - Settings: "Spike vs prior biggest", "Spike lookback" and "Box shows" (did nothing) removed (parameter version 3); the box Net row
 *     shows the net and its ratio. Description rewritten.
 *   - MARKET STATE (MarketState.h): SRC "BOOK" facts of the last closed candle (bid_net, ask_net, bid_net_x, ask_net_x, bid_state,
 *     ask_state, spike, rebuilt_pct) posted once per closed candle, re-posted unchanged every 15 s, cleared on removal. Behind the scenes.
 *   The pane draws itself (draw() below) instead of OrderFlowFeed's drawNetBars (shared header untouched).
 *  Rassul 2026-10-16 07:45-07:49 CT: "give me a design that combines the pulling and stacking indicator" / mockup / "1 net bar. make the
 *  zero line gray so i can clearly see the separation. dont keep the old panes, replace them. lets build this".
 *
 *  WHO IS ARRIVING AND WHO IS LEAVING, in one pane (replaces lsPullStats 1.7.1 and lsStackStats 1.5.1).
 *  Per candle, per side: ONE NET BAR = lots ADDED near price minus lots PULLED near price (market-by-order feed, the same engine and the
 *  same "near price" band as before). Above the grey zero line = size arriving (stacking); below = size leaving (pulling). Bids green (left
 *  bar), offers red (right bar). Bright = a spike of the bigger part (added when up, pulled when down; same spike rule as the old panes).
 *  Spike label: ratio / pace / lots (pace only for added). Letters and the refill dash are not drawn; the records keep them.
 *  Top: the Order Flow read line (the louder of the two old reads, "Also: ..." when the other agrees), the blue options line when fresh,
 *  the pane title "Book Flow - Who is arriving or leaving?". Settings: as lsPullStats (Ratio bars, Position, Spike vs average, Spike vs
 *  prior biggest, Spike lookback, Near ticks, Record order feed, Offset X / Y, Box shows).
 *
 *  KEPT WORKING FOR THE TAPE READER AND THE NIGHTLY STUDIES (nothing else had to change):
 *   - lsFlexLevels\StackStats\<MKT>-<CONTRACT>-<spb>-<session>.csv and lsFlexLevels\PullStats\... session records, written in the old format
 *     (same columns, same message words) from the same candle numbers; the pull record's pace column is the ADDED pace (no pull pace here).
 *   - lsFlexLevels\PullStats\...-zoneread-<session>.csv: the arrival read after price reaches a key / gamma level (ZoneRead.h), shown as
 *     the read line only once it locks (3 min), always "?". The old stacking read at a level ("bids stacking 2.3x normal ?") is not shown.
 *   - "Record order feed" (default ON): every order-by-order event to data\irt\<session>\<MKT>\mbo_HH.csv (MboRecord.h) - the nightly MBO archive.
 *   - lsFlexLevels\BookFlow\... : its own record (same columns) and status-<MKT>-<CONTRACT>-<spb>.txt.
 *  Rules: BookFlowLogic.h; engine, feed, drawing, params, records: OrderFlowFeed.h 1.7.0.
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
#include "HostSlot.h"
#define OFF_SDK 1
#include "OrderFlowFeed.h"
#include "BookFlowLogic.h"
#include "OrderFlowPaneSettings.h"   // (1.3.2) the settings window table (SettingsSpec.h)
#include "SettingsHost.h"
static ss::Ids& bfIds() { static ss::Ids I; return I; }   // the row ids IRT gave the table (one DLL = one parameter list, written once in setup)
#include "MboRecord.h"              // Record order feed
#include "MarketState.h"            // (1.3.0) SRC "BOOK" facts for the read

#define BF_VERSION "1.5.0"          // keep equal to bf::VERSION and the setVersion literal below; (1.3.4, 2026-10-18 23:19 CT) the box follows the mouse only inside this pane, else Live = the forming candle
#include "PaneCursor.h"           // (1.3.4)
#include "BoxReadDraw.h"          // (1.4.0, Rassul approved 23:48 CT bookflow-read-mockup.png) the box rows Net, Pace, Read

// (1.3.0) the dialog rows (parameter version 3): every row is read by the chart
// (1.3.2) the window rows come from OrderFlowPaneSettings.h (no index constants)

class BookFlow : public off::MboPane<bf::Logic> {
public:
    BookFlow() : off::MboPane<bf::Logic>() {}
    virtual bool barMarks() const { return false; }                        // no letters / dash drawn; hover + records keep them
    virtual bool netBars() const { return true; }
    virtual bool textOff() const { return false; }
    virtual std::string boxKeep() const { return "Refill Effort;Refilled"; }   // (1.5.0, Rassul 11:19 / 11:53) the two defense rows; the rest hidden   // (1.4.0, 23:56 CT) + Added / Pulled (their plain 20-candle ratios, as the pull marker)            // (1.2.1, Rassul 22:54) the box: Net and Pace only (1.3.0: Net = "+120 (2.4x)")
    virtual bool boxOnly() const { return true; }                         // (1.2.0, Rassul 2026-10-16 22:00) the left-justified numbers box in the pane, no read / options lines                          // (1.1.0, Rassul 2026-10-16 18:14) no read / numbers / options lines on the pane; the records and the tape reader keep every number
    virtual long long netOf(const off::Rec& r, int s) const { return bf::netOf(r, s); }
    // the label numbers of one net bar: ratio / pace / lots of the part that spiked (up = added, down = pulled; pace only for added)
    virtual void netLook(const off::PaneBar& b, int s, off::BarLook& k) const {
        const bool up = bf::netOf(b.r, s) >= 0;
        k.ratioOk[s] = up ? b.m[0].ratioOk[s] : b.m[1].ratioOk[s];
        k.ratio[s] = up ? b.m[0].ratio[s] : b.m[1].ratio[s];
        k.p100[s] = up ? b.r.pace100[s] : 0;
        k.q[s] = up ? b.r.addC[s] : b.r.pullC[s];
    }
    virtual int zonePane() const { return zread::P_PULLSTATS; }            // the arrival read (kept in PullStats\ so the nightly keeps reading it)
    virtual std::string zoneDirName() const { return "PullStats"; }
    virtual int parmsTitle(char* pStr, int size) { return off::copyTitle(pStr, size, bf::Logic::title(), BF_VERSION) ? RTX_OK : RTX_FAIL; }
    // Record order feed: the recorder and the two legacy session records live in this chart's pane state
    virtual bool recorderPane() const { return true; }
    struct Ext : off::PaneExt {
        mrec::Recorder r; off::SessionFile stack, pull;
        // (1.3.0) the net decisions of closed candles (decided once; only a Ratio bars / Spike vs average change re-decides), the chart
        // label + identity they belong to, the Market State facts posted
        std::map<long long, bf::NetStat> net; int netRb = -1; long long netAvg = -1; std::string identity, chart;
        long long mstBar = 0, mstCt = 0, mstMs = -1, mstPosts = 0; std::map<std::string, std::string> mstKv;
        pc::Tracker cur; long long liveEv = -1, liveMs = -1;
        br::Read lastRead; std::vector<std::string> lastReadLines;        // (1.4.0) the Read last drawn (tests)
        std::map<long long, bf::PullStat> pulls; long long markersDrawn = 0; // (1.4.0) closed candles' pull decisions (decided once, never cleared: no repaint)               // (1.3.4) the pane on the screen; the live candle's events last drawn
    };
    Ext* ext(S_& S) {
        if (!S.ext) S.ext.reset(new (std::nothrow) Ext());
        return static_cast<Ext*>(S.ext.get());
    }
    mrec::Recorder* rec(S_& S) {
        Ext* e = ext(S); if (!e) return nullptr;
        e->r.setOn(S.set.record != 0);
        if (!S.market.empty()) e->r.setMarket(S.market);
        return &e->r;
    }
    virtual void onRaw(S_& S, const DEPTH_ORDER& o) {
        mrec::Recorder* r = rec(S); if (!r || !r->on) return;
        if (!o.timestamp) { ++r->dropped; return; }
        if (o.timestamp != r->tsKey) {                                     // one clock conversion per second, not per event
            struct tm t; std::memset(&t, 0, sizeof(t));
            if (!getLocaltime(o.timestamp, &t)) { ++r->dropped; return; }
            r->tsKey = o.timestamp; r->tsTm = t;
        }
        const struct tm& t = r->tsTm;
        mrec::Ev e; e.y = t.tm_year + 1900; e.mo = t.tm_mon + 1; e.d = t.tm_mday; e.h = t.tm_hour; e.mi = t.tm_min; e.s = t.tm_sec;
        e.action = o.action; e.price = (double)o.price; e.prev = (double)o.previousPrice; e.size = (unsigned long)o.size;
        e.oid = (long long)o.orderID; e.aid = (long long)o.aggressorOrderID; e.side = o.buySell == 1 ? 1 : o.buySell == 0 ? 0 : -1;
        e.bid = (double)o.bid; e.ask = (double)o.ask;
        r->add(e);
    }
    virtual void afterPump(S_& S, long long now, bool fromDraw) { mrec::Recorder* r = rec(S); if (r) r->tick(now, fromDraw); post(S, now); }
    virtual void afterTimer(S_& S, long long now) { mrec::Recorder* r = rec(S); if (r) r->tick(now, false); }
    virtual void statusExtra(S_& S, std::ostream& o) {
        mrec::Recorder* r = rec(S); if (r) r->status(o); else o << "\nMBO_RECORD,unavailable (out of memory)";
        Ext* e = ext(S);
        o << "\nSPIKE_RULE,|net| >= " << off::tenthsText(avgT(S)) << (avgT(S) > 0 ? "" : " (off)") << " the plain " << S.ser.ratioBars << "-candle average of that side's |net|, decided at the close; forming candle dim, no label; colour by side"
          << "\nMARKET_STATE,BOOK " << (!e || e->chart.empty() ? std::string("waiting for the chart label") : "bar " + std::to_string(e->mstBar ? e->mstCt : 0) + ",posts " + std::to_string(e->mstPosts) + ",keys " + std::to_string(e->mstKv.size()));
    }
    // the old panes' session records (StackStats\ and PullStats\), opened with the session, one row per decided candle, saved with the main record
    virtual void onLoad(S_& S, const std::string& flex, const std::string& stem) {
        Ext* e = ext(S); if (!e || flex.empty()) return;
        const std::string cols = std::string("\nbar_time|ct_time|dbo|") + off::REC_COLS + "|letters_left|letters_right|spike_left|spike_right|dash_left|dash_right|implication|level|message|numbers\n";
        off::makeDir(flex);
        const std::string ds = flex + "\\StackStats", dp = flex + "\\PullStats";
        off::makeDir(ds); off::makeDir(dp);
        const std::string tail = " " + S.market + " " + S.symbol + " " + std::to_string(S.spb);
        e->stack.open(S.curSession, ds + "\\" + stem + "-" + S.curSession + ".csv", std::string(), std::string("# StackStats ") + st::VERSION + tail + cols, (size_t)off::REC_NUMS);
        e->pull.open(S.curSession, dp + "\\" + stem + "-" + S.curSession + ".csv", std::string(), std::string("# PullStats ") + ps::VERSION + tail + cols, (size_t)off::REC_NUMS);
        e->stack.load(); e->pull.load();
    }
    static off::Row legacyRow(const off::Row& base, const off::PaneBar& pb) {
        off::Row r = base;
        for (int s = 0; s < 2; ++s) { r.let[s] = pb.let[s]; r.spike[s] = pb.spike[s] ? 1 : 0; r.dash[s] = pb.dash[s]; }
        r.impl = pb.msg.impl; r.line1 = pb.msg.line1; r.line2 = pb.msg.line2;
        return r;
    }
    virtual void onDecide(S_& S, long long key, const off::PaneBar& nb, const off::Row& base) {
        Ext* e = ext(S); if (!e || e->stack.path.empty()) { decidedNet(S, key); return; }
        const off::Params& P = S.params.load.p;
        off::PaneBar sk = bf::stackView(nb); st::Logic::decide(sk, P, false); e->stack.put(legacyRow(base, sk));
        off::PaneBar pk = bf::pullView(nb);  ps::Logic::decide(pk, P, false); e->pull.put(legacyRow(base, pk));
        decidedNet(S, key);                                                // (1.3.0) the net spike -> the BookFlow record + the Market State
    }
    virtual void onSave(S_& S) { Ext* e = ext(S); if (!e) return; e->stack.save(); e->pull.save(); }

    // ---- (1.3.0) the net spike, decided once per closed candle -------------------------------------------------------------------
    // the spike threshold in force: the nightly params file wins where it sets it, else the dialog's (0 = off) - as Aggression
    static long long avgT(const S_& S) { const off::Params& P = S.params.load.p; return P.file("spikeAvgX") ? P.spikeAvgT : S.dlgSpike.avgT; }
    void sync(S_& S, Ext& e) {                                             // a new chart identity or a settings change: start the decisions over
        if (e.identity != S.identity) {
            if (!e.chart.empty() && !e.identity.empty() && !e.mstKv.empty()) mst::clear("BOOK", symOf(e.identity), spbOf(e.identity), e.chart);
            e.identity = S.identity; e.net.clear(); e.mstBar = 0; e.mstCt = 0; e.mstMs = -1; e.mstKv.clear();
        }
        if (e.netRb != S.ser.ratioBars || e.netAvg != avgT(S)) { e.net.clear(); e.netRb = S.ser.ratioBars; e.netAvg = avgT(S); }
    }
    static std::string symOf(const std::string& id) { const size_t a = id.find('|'), b = a == std::string::npos ? a : id.find('|', a + 1); return b == std::string::npos ? std::string() : id.substr(a + 1, b - a - 1); }
    static long spbOf(const std::string& id) { const size_t c = id.rfind('|'); return c == std::string::npos ? 0 : std::atol(id.c_str() + c + 1); }
    const bf::NetStat* netAt(S_& S, long long t) {                         // a closed candle's decision (computed once, then kept)
        Ext* e = ext(S); if (!e) return nullptr;
        sync(S, *e);
        std::map<long long, bf::NetStat>::const_iterator it = e->net.find(t);
        if (it != e->net.end()) return &it->second;
        const bf::NetStat n = bf::netStatAt(S.ser.bars, t, S.ser.ratioBars, avgT(S));
        if (!n.ok) return nullptr;
        while (e->net.size() > 6000) e->net.erase(e->net.begin());
        return &(e->net[t] = n);
    }
    // (1.4.0) a closed candle's pull decision: computed once from the decided candles before it, then kept (never re-decided)
    const bf::PullStat* pullAt(S_& S, long long t) {
        Ext* e = ext(S); if (!e) return nullptr;
        std::map<long long, bf::PullStat>::const_iterator it = e->pulls.find(t);
        if (it != e->pulls.end()) return &it->second;
        std::map<long long, off::PaneBar>::const_iterator b = S.ser.bars.find(t);
        if (b == S.ser.bars.end() || !b->second.ok) return nullptr;
        while (e->pulls.size() > 6000) e->pulls.erase(e->pulls.begin());
        return &(e->pulls[t] = bf::pullStatAt(S.ser.bars, t, b->second.r));
    }
    // a closed candle was decided: its net spike goes into the record row (spike_left / spike_right) and the Market State facts
    void decidedNet(S_& S, long long key) {
        Ext* e = ext(S); if (!e) return;
        const bf::NetStat* n = netAt(S, key); if (!n) return;
        std::map<long long, off::PaneBar>::iterator b = S.ser.bars.find(key);
        if (b != S.ser.bars.end()) for (int s = 0; s < 2; ++s) b->second.spike[s] = n->spike[s];
        std::map<long long, off::Row>::iterator rw = S.file.rows.find(key);
        if (rw != S.file.rows.end()) for (int s = 0; s < 2; ++s) rw->second.spike[s] = n->spike[s] ? 1 : 0;
        if (b != S.ser.bars.end() && key >= e->mstBar) facts(S, *e, key, b->second, *n);
    }
    void facts(S_& S, Ext& e, long long key, const off::PaneBar& b, const bf::NetStat& n) {
        (void)S; struct tm x; std::memset(&x, 0, sizeof(x));
        if (!getLocaltime((RTDATE)key, &x)) return;
        e.mstKv = bf::bookFacts(b, n); e.mstBar = key; e.mstMs = -1;
        e.mstCt = off::civilDays(x.tm_year + 1900, (unsigned)x.tm_mon + 1, (unsigned)x.tm_mday) * 86400 + x.tm_hour * 3600 + x.tm_min * 60 + x.tm_sec;
    }
    // SRC BOOK: posted when new, re-posted unchanged every 15 s (readers ignore an entry older than 60 s)
    void post(S_& S, long long now) {
        Ext* e = ext(S); if (!e || e->chart.empty() || S.symbol.empty()) return;
        sync(S, *e);
        if (e->mstKv.empty() && S.lastClosedKey > 0) {                     // after a restart: the last closed candle read back from the record
            std::map<long long, off::PaneBar>::const_iterator b = S.ser.bars.find(S.lastClosedKey);
            const bf::NetStat* n = b != S.ser.bars.end() && b->second.ok ? netAt(S, S.lastClosedKey) : nullptr;
            if (n) facts(S, *e, S.lastClosedKey, b->second, *n);
        }
        if (e->mstKv.empty()) return;
        if (e->mstMs >= 0 && now >= e->mstMs && now - e->mstMs < 15000) return;
        e->mstMs = now; e->mstPosts++;
        mst::publish("BOOK", S.symbol, S.spb, e->chart, now, e->mstCt, e->mstKv);
    }
    void release() {                                                       // the Market State entry goes with the indicator
        S_* S = testState();
        if (S) { Ext* e = static_cast<Ext*>(S->ext.get()); if (e && !e->chart.empty() && !S->symbol.empty()) mst::clear("BOOK", S->symbol, S->spb, e->chart); }
        off::MboPane<bf::Logic>::release();
    }

    // ---- (1.3.0) the settings window: Ratio bars, Position, Spike vs average, Near ticks, Record order feed, Offset X, Offset Y ---------
    // The per-market file keeps OrderFlowFeed's layout (paneSettingsLine): the removed rows' fields (jump, lookback, Box shows) carry the
    // file's values unchanged. Loading never saves; a control he did not touch keeps the file's value (DlgSnap).
    ss::Snap snapBf_;                                                      // (1.3.2) SettingsSpec.h snapshot
    std::string placeFile(const std::string& mkt) { const std::string f = off::flexDir(); return f.empty() || mkt.empty() ? std::string() : f + "\\BookFlow.place-" + mkt + ".txt"; }
    // (1.3.2, settings audit 2026-10-18) the window from ofset::bookFlowSpec() through SettingsHost.h (rows, order and file unchanged)
    static ss::Values bfValues(const off::PaneSettings& f) {
        using namespace ofset::bfk;
        ss::Values V = ss::defaults(ofset::bookFlowSpec(), -1);
        V.v[K_RATIO_BARS] = f.ratioBars; V.v[K_POSITION] = f.pos; V.v[K_SPIKE_AVG] = f.spike.avgT / 10.0; V.v[K_NEAR_TICKS] = f.nearTicks;

        return V;
    }
    virtual int parmsLoad(void) {
        if (!inDialog()) return RTX_OK;
        ss::fillDialog(this, ofset::bookFlowSpec(), bfIds(), bfValues(fileSettings(chartMarket())), snapBf_);
        return RTX_OK;
    }
    void saveBf() {
        using namespace ofset::bfk;
        if (!inDialog()) return;
        const std::string mkt = chartMarket();
        const off::PaneSettings fs = fileSettings(mkt);
        const ss::Values V = ss::readDialog(this, ofset::bookFlowSpec(), bfIds(), bfValues(fs), snapBf_);   // untouched -> the file's value; invalid never saved
        off::PaneSettings s = fs;                                          // jump / lookback / Box shows: the file's values
        s.ratioBars = V.i(K_RATIO_BARS); s.pos = V.i(K_POSITION); s.spike.avgT = ofset::tenths(V.f(K_SPIKE_AVG)); s.nearTicks = V.i(K_NEAR_TICKS); s.record = 1;   // (1.3.3) recording always on (his rule)
        s.dx = 0; s.dy = 0;                                               // (1.3.3) the 9-spot Position only
        off::PaneSettings chk;
        if (s.spike.avgT < 0 || s.nearTicks < 0 || !off::parsePaneSettings(off::paneSettingsLine(s), chk)) return;   // never overwrite with garbage
        const std::string p = placeFile(mkt); if (p.empty()) return;
        off::makeDir(off::flexDir());
        off::publishFile(p, off::paneSettingsLine(s) + "\n");
        snapBf_.take(ss::dialogNow(this, ofset::bookFlowSpec(), bfIds(), V));
        S_* S = testState();
        if (S) { S->settingsCheckMs = -1; S->settingsStamp = -1 - 3; }    // re-read on the next pass (not "first")
    }
    virtual int parmsApply(void) { saveBf(); return RTX_OK; }
    virtual int parmsUpdt(unsigned int i) { ss::onUpdate(this, ofset::bookFlowSpec(), bfIds(), (int)i, snapBf_); saveBf(); return RTX_OK; }

    // ---- (1.3.0) the pane: the numbers box (Net, Pace) + one net bar per side per candle, coloured by side ----------------------------
    virtual int draw(void) {
        {   S_* S0 = testState();                                          // the chart label (Market State key), read in the draw as BigBidAsk
            if (S0) { Ext* e = ext(*S0); const char* cl = getChartLabel(); const std::string c = cl ? cl : "";
                      if (e && !c.empty() && c != e->chart) { if (!e->chart.empty() && !S0->symbol.empty()) mst::clear("BOOK", S0->symbol, S0->spb, e->chart); e->chart = c; e->mstMs = -1; } } }
        pump(true);                                                        // from the draw: a recorder buffers, never writes
        S_* S = testState();
        if (S) paneLeft(*S);                                               // (1.3.4) the candle under the mouse only while the cursor is inside this pane
        if (S) { try { renderNet(*S); } catch (...) { S->state = "draw error"; } }
        return RTX_OK;
    }
    // (1.3.4, Rassul 23:19 CT "when i move the mouse outside the aggression pane the updated values should still be displayed") IRT sends
    // mouse events only over this pane and nothing when the mouse leaves: the hovered candle stayed latched (HoverTrack::left clears only
    // off the chart window) and followed other panes' candle (HoverBoard). Now: inside = the cursor over the last event's window AND
    // within the pane rectangle located then (PaneCursor.h), checked on every draw / calc; anywhere else = Live (the forming candle).
    bool paneLeft(S_& S) {
        Ext* e = ext(S); if (!e) return false;
        int cx = 0, cy = 0; const bool ok = pc::cursorPos(cx, cy);
        if (e->cur.inside(off::cursorWindow(), ok, cx, cy)) return false;
        bool ch = S.hov.hover.set(-1);
        if (S.inBox) { S.inBox = false; ch = true; }
        return ch;
    }
    virtual int mouse(RTX_EVENT* ev) {
        S_* S = testState();
        if (S && ev && (ev->type == E_MOUSE_MOVE || ev->type == E_MOUSE_HOVER)) {
            Ext* e = ext(*S); if (e) { int cx = 0, cy = 0; const bool ok = pc::cursorPos(cx, cy); e->cur.onEvent(off::cursorWindow(), ok, cx, cy, ev->v.mouse.h, ev->v.mouse.v); }
        }
        const int r = off::MboPane<bf::Logic>::mouse(ev);
        if (S && ev && (ev->type == E_MOUSE_MOVE || ev->type == E_MOUSE_HOVER)) {
            Ext* e = ext(*S); if (e && !e->cur.eventInside(ev->v.mouse.h, ev->v.mouse.v) && S->hov.hover.set(-1)) invalidateChart(false);
        }
        return r;
    }
    void bfCalc() {                                                        // (1.3.4) in place of calcHook (no HoverBoard following)
        pump(); S_* S = testState(); if (!S) return;
        bool ch = paneLeft(*S);
        Ext* e = ext(*S);
        if (e && S->hov.hover.bar < 0) {                                   // Live: redraw when the forming candle got new order events (<= 4 a second)
            std::map<long long, off::Acc>::const_iterator it = S->accs.find(S->formingKey);
            const long long ev = it != S->accs.end() ? (long long)it->second.events : -1; const long long now = off::steadyMs();
            if (ev != e->liveEv && (e->liveMs < 0 || now < e->liveMs || now - e->liveMs >= 250)) { e->liveEv = ev; e->liveMs = now; ch = true; }
        }
        if (ch) invalidateChart(false);
    }
    void renderNet(S_& S) {
        RCT pane; pane.getPaneRect(false);
        if (Ext* e = ext(S)) { e->cur.pane.l = pane.left; e->cur.pane.t = pane.top; e->cur.pane.r = pane.right; e->cur.pane.b = pane.bottom; }
        const long count = getBarCount();
        if (count < 2 || pane.bottom - pane.top < 24 || pane.right - pane.left < 60) return;
        RTARRAYI times(barDateTime);
        if (times.count < count) return;
        const int last = (int)count - 1;
        auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
        int v0 = 0, v1 = last;
        if (getVisibleBars(&v0, &v1) != RTX_OK || v0 < 0 || v1 < v0 || v1 > last) { v1 = last; v0 = last; while (v0 > 0 && xOf(v0 - 1) >= pane.left) --v0; }
        int slot = (int)getPixelsPerBar();
        if (slot <= 0 || slot > 400) slot = v1 > v0 ? std::max(2, std::abs(xOf(v1) - xOf(v1 - 1))) : 8;
        off::PaneBar lb; bool haveLive = false;                            // the forming candle: its numbers as of now (drawn dim, never judged)
        {   std::map<long long, off::Acc>::const_iterator it = S.accs.find(S.formingKey);
            if (it != S.accs.end() && it->second.events > 0 && (long long)times[last] == S.formingKey) { lb = S.ser.make(S.formingKey, S.e.numbers(it->second, S.formingKey == S.firstBarKey, S.params.load.p)); haveLive = lb.ok; } }
        auto barOf = [&](int i) -> const off::PaneBar* {
            if (i == last) return haveLive ? &lb : nullptr;
            std::map<long long, off::PaneBar>::const_iterator it = S.ser.bars.find((long long)times[i]);
            return it != S.ser.bars.end() && it->second.ok ? &it->second : nullptr;
        };
        long long top = 0;
        for (int i = v0; i <= v1; ++i) { const off::PaneBar* b = barOf(i); if (b) top = std::max(top, magOf(b->r)); }
        if (S.axisTop > top) top = S.axisTop;
        S.paneH = pane.bottom - pane.top;
        const off::PaneGeo g = off::paneGeo(this, gmode(S.set.mode));
        const int hov = S.hov.hover.bar >= 0 && S.hov.hover.bar < last ? S.hov.hover.bar : -1;
        const int mb = hov >= 0 ? hov : last - 1;
        const long long mt = (long long)times[mb];
        std::map<long long, off::PaneBar>::const_iterator mi = S.ser.bars.find(mt);
        const off::PaneBar* mbar = mi != S.ser.bars.end() && mi->second.ok ? &mi->second : nullptr;
        const bool liveBox = hov < 0 && haveLive;                          // (1.3.4) Live = the forming candle's numbers so far (no net decision yet)
        if (liveBox) mbar = &lb;
        bf::NetStat liveNs; if (liveBox) liveNs = bf::liveNetStat(S.ser.bars, S.formingKey, lb.r, S.ser.ratioBars, avgT(S));   // (1.4.0) its Net ratio so far
        const bf::NetStat* shownNs = mbar ? (liveBox ? &liveNs : netAt(S, mt)) : nullptr;
        bf::PullStat livePs; if (liveBox) livePs = bf::pullStatAt(S.ser.bars, S.formingKey, lb.r);   // (1.4.0) the forming candle: shown, never a marker
        const bf::PullStat* shownPs = mbar ? (liveBox ? &livePs : pullAt(S, mt)) : nullptr;
        off::Info info = bf::info(mbar, S.params.load.p, shownNs, S.market);   // (1.5.0) the market picks the Refilled cut-offs
        {   const std::string keep = ";" + boxKeep() + ";"; std::vector<off::InfoRow> kept;   // (1.2.1) Net and Pace only
            for (size_t k = 0; k < info.rows.size(); ++k) if (keep.find(";" + info.rows[k].label + ";") != std::string::npos) kept.push_back(info.rows[k]);
            info.rows.swap(kept); }
        if (shownPs && shownPs->ok) for (size_t k = 0; k < info.rows.size(); ++k) {   // (1.4.0) Added / Pulled with the plain 20-candle ratios (the marker's yardstick)
            const bool ad = info.rows[k].label == "Added", pu = info.rows[k].label == "Pulled"; if (!ad && !pu) continue;
            for (int s = 0; s < 2; ++s) info.rows[k].v[s] = std::to_string(ad ? shownPs->add[s] : shownPs->pull[s]) + " (" + off::ratioOr(ad ? shownPs->addOk[s] : shownPs->pullOk[s], ad ? shownPs->addR[s] : shownPs->pullR[s]) + ")";
        }
        { struct tm x; std::memset(&x, 0, sizeof(x)); info.head = hov < 0 ? std::string("Live") : (mt > 0 && getLocaltime((RTDATE)mt, &x) ? off::clock12(x.tm_hour, x.tm_min) : std::string("n/a")); }
        bp::Place pl; pl.pos = S.set.pos; pl.dx = S.set.dx; pl.dy = S.set.dy;
        const bp::Box area = bp::mk(g.left + 3, g.top + 3, g.edge - 3, g.bottom - 3);
        const zread::Line zl = S.zr.e.line(S.zr.pane, off::steadyMs());
        off::PanelPlan P;                                                  // (1.4.0) + the Read row: the box grows to hold it (BoxReadDraw.h)
        br::Read rd = bf::bookRead(shownNs, shownPs);
        if (mbar) {                                                        // (1.5.0) at a new 10-bar low / high the REFILL read leads
            br::Read rr; if (bf::refillRead(mbar->r, S.market, spotAt(liveBox ? last : mb), shownNs, rr)) rd = rr;
        }
        const off::TextW tw = off::measureWith(this);
        const br::Plan rp = br::addAndPlan(info, rd, tw, [&](const off::Info& I) { return off::planPanel(area, g.top, off::MODE_NUMBERS, pl, I, std::string(), std::string(), g.reserve, tw); }, P);
        if (Ext* e = ext(S)) { e->lastRead = rd; e->lastReadLines = rp.lines; }
        S.lastBox = P.ok ? P.box : bp::Box();
        std::vector<off::LBox> avoid;
        if (P.ok) avoid.push_back(off::LBox{ P.box.l, P.box.t, P.box.r, P.box.b, 0, 0 });
        for (size_t k = 0; k < P.textRects.size(); ++k) avoid.push_back(off::LBox{ P.textRects[k].l, P.textRects[k].t, P.textRects[k].r, P.textRects[k].b, 0, 0 });
        pullMarkers(S, g, v0, v1, last, slot, times, xOf, avoid, P.ok ? &P.box : nullptr);   // (1.4.0) before the bars' labels: they avoid the markers
        drawNet(S, g, v0, v1, last, slot, top, xOf, barOf, avoid);
        const COLOR vcol[2] = { 0x0022C55E, 0x00EF4444 };                 // values: bids (buyers) green, offers (sellers) red
        off::drawPanel(this, P, info, vcol, off::implOf(zl), false);
        br::draw(this, P, info, rp, rd);                                  // (1.4.0) the Read word + lines over its rows
    }
    // (1.5.0) where candle i sits: a new 10-bar low / high and its volume vs the 20 before it (bf::spotOf), from the chart's own bars
    bf::Spot spotAt(int i) {
        bf::Spot none; if (i < 9) return none;
        RTARRAY lo(barLow), hi(barHigh); RTARRAYI vol(barVolume);
        const long count = getBarCount();
        if (i >= count || lo.count < count || hi.count < count || vol.count < count) return none;
        const int from = i >= 20 ? i - 20 : i - 9;
        std::vector<double> L, H; std::vector<long long> V;
        for (int k = from; k <= i; ++k) {
            const double l = (double)lo[k], h = (double)hi[k];
            if (!(l == l) || !(h == h) || l <= 0 || h <= 0) return none;
            L.push_back(l); H.push_back(h); V.push_back((long long)vol[k]);
        }
        return bf::spotOf(L, H, V);
    }
    // the net bars (OrderFlowFeed drawNetBars' geometry, 1.3.0 rules): the grey zero line, one bar per side, colour BY SIDE (bid green,
    // offer red), bright + label only for a CLOSED candle's net spike; labels biggest first, a label that would sit on another is skipped
    // (1.4.0, Rassul approved 23:56 CT bookflow-pull2-mockup.png) PULL MARKERS: a closed candle whose side pulled (bf::PullStat pulling) gets
    // a small solid block in the side's colour (bids green / offers red) under its bar column + a yellow "P {ratio}" label, at a fixed strip
    // at the bottom of the pane (below the deepest net bar: netGeoFor keeps LABEL_ROOM there), never over the info box or another marker
    // label (older candles first, a later one that would overlap is skipped). The forming candle never gets one.
    template <class XOf>
    void pullMarkers(S_& S, const off::PaneGeo& g, int v0, int v1, int last, int slot, RTARRAYI& times, XOf xOf, std::vector<off::LBox>& avoid, const bp::Box* box) {
        Ext* e = ext(S); if (!e) return;
        const off::NetGeo n = off::netGeoFor(g.bottom - g.top, g.reserve);
        const int yLab = g.top + n.yBot - off::LABEL_LH, yBlk = yLab - 8;
        if (yBlk <= g.top + n.y0 + 4) return;                                // pane too short
        const COLOR col[2] = { off::C_GREEN_SPIKE, off::C_RED_SPIKE };
        const off::TextW M = off::measureWith(this);
        std::vector<off::LBox> mine;
        for (int i = std::max(0, v0); i <= v1 && i < last; ++i) {
            const bf::PullStat* p = pullAt(S, (long long)times[i]); if (!p || (!p->pulling[0] && !p->pulling[1])) continue;
            const int xc = xOf(i); if (xc < g.left || xc > g.edge) continue;
            const off::Pair pr = off::pairIn(xc, slot);
            for (int s = 0; s < 2; ++s) {
                if (!p->pulling[s]) continue;
                const std::string lab = bf::pullLabel(*p, s); const int w = M(lab, 9, true), cx = (pr.l[s] + pr.r[s]) / 2;
                int lx = cx - w / 2; if (lx < g.left + 1) lx = g.left + 1; if (lx + w > g.edge - 1) lx = g.edge - 1 - w;
                const off::LBox blk = { std::min(pr.l[s], cx - 2), yBlk, std::max(pr.r[s], cx + 2), yBlk + 6, 0, 0 }, lb = { lx, yLab, lx + w + 1, yLab + off::LABEL_LH - 1, 0, 0 };
                bool hit = box && ((lb.r >= box->l && lb.l <= box->r && lb.b >= box->t && lb.t <= box->b) || (blk.r >= box->l && blk.l <= box->r && blk.b >= box->t && blk.t <= box->b));
                for (size_t q = 0; q < mine.size() && !hit; ++q) hit = off::overlaps(lb, mine[q]);
                if (hit) continue;
                mine.push_back(lb); avoid.push_back(lb); avoid.push_back(blk);
                off::fillFlat(this, blk.l, blk.t, blk.r, blk.b, col[s]);
                off::textAt(this, lx, yLab, off::LABEL_LH - 1, lab, off::C_LBL_RATIO, 9, true);
                ++e->markersDrawn;
            }
        }
    }
    template <class XOf, class BarOf>
    void drawNet(S_& S, const off::PaneGeo& g, int v0, int v1, int last, int slot, long long top, XOf xOf, BarOf barOf, const std::vector<off::LBox>& avoid) {
        if (top <= 0) return;
        const COLOR normal[2] = { off::C_GREEN_BAR, off::C_RED_BAR }, spike[2] = { off::C_GREEN_SPIKE, off::C_RED_SPIKE };
        const off::NetGeo n = off::netGeoFor(g.bottom - g.top, g.reserve);
        const int y0 = g.top + n.y0;
        if (g.edge > g.left + 4) off::fillFlat(this, g.left, y0, g.edge, y0 + 1, off::C_NET_ZERO);
        struct Cand { int cx = 0, hb = 0; bool up = true; long long mag = 0; std::string a, b; };
        std::vector<Cand> cands;
        RTARRAYI times(barDateTime);
        for (int i = v0; i <= v1; ++i) {
            const off::PaneBar* pb = barOf(i); if (!pb) continue;
            const int xc = xOf(i); if (xc < g.left || xc > g.right) continue;
            const bool closed = i != last;
            const bf::NetStat* ns = closed ? netAt(S, (long long)times[i]) : nullptr;
            const off::Pair p = off::pairIn(xc, slot);
            for (int s = 0; s < 2; ++s) {
                const bf::NetLook k = bf::netLookOf(pb, ns, s, closed, S.market);   // (1.5.0) R = Refilled vs the market's normal
                if (k.v == 0) continue;
                const bool up = k.v > 0; const long long mg = up ? k.v : -k.v;
                const int h = off::barHeight(mg, top, n.maxPx);
                const COLOR c = k.bright ? spike[k.col] : normal[k.col];
                if (up) off::fillFlat(this, p.l[s], y0 - h, p.r[s], y0 - 1, c); else off::fillFlat(this, p.l[s], y0 + 2, p.r[s], y0 + 1 + h, c);
                if (k.bright && (!k.ratio.empty() || !k.pace.empty())) { Cand q; q.cx = (p.l[s] + p.r[s]) / 2; q.hb = h; q.up = up; q.mag = mg; q.a = k.ratio; q.b = k.pace; cands.push_back(q); }
            }
        }
        if (cands.empty()) return;
        std::stable_sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.mag > b.mag; });
        FONT sf; sf.id = HELVETICA; sf.size = off::LABEL_PT; sf.style = PLAIN; setFont(sf);
        std::vector<off::LBox> placed(avoid);
        for (size_t k = 0; k < cands.size(); ++k) {
            const Cand& c = cands[k];
            std::string lr[2]; COLOR lc[2] = { off::C_LBL_RATIO, off::C_LBL_PACE }; int nR = 0;
            if (!c.a.empty()) { lr[nR] = c.a; lc[nR++] = off::C_LBL_RATIO; }
            if (!c.b.empty()) { lr[nR] = c.b; lc[nR++] = off::C_LBL_PACE; }
            const int yEdge = c.up ? y0 - c.hb : y0 + 1 + c.hb;
            const int t0 = c.up ? yEdge - 2 - nR * off::LABEL_LH : yEdge + 2, b0 = t0 + nR * off::LABEL_LH;
            if (t0 < g.top + g.reserve || b0 > g.bottom) continue;
            int w = 0; for (int r = 0; r < nR; ++r) w = std::max(w, (int)getTextWidth(lr[r].c_str(), -1));
            const off::LBox me = { c.cx - w / 2 - 1, t0, c.cx - w / 2 + w + 1, b0 - 1, 0, 0 };
            if (me.l < g.left + 1 || me.r > g.edge - 1) continue;
            bool hit = false; for (size_t o = 0; o < placed.size() && !hit; ++o) hit = off::overlaps(me, placed[o]);
            if (hit) continue;
            placed.push_back(me);
            for (int r = 0; r < nR; ++r) {
                setTextColor(lc[r]); const int tw = (int)getTextWidth(lr[r].c_str(), -1); const int y = t0 + r * off::LABEL_LH;
                RCT rc; rc.set(off::coord(c.cx - tw / 2), off::coord(y), off::coord(c.cx - tw / 2 + tw + 2), off::coord(y + off::LABEL_LH)); rc.drawText(lr[r].c_str(), false, false);
            }
        }
    }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<BookFlow*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<BookFlow*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) {
    // (1.3.0) parameter version 3: "Spike vs prior biggest", "Spike lookback" (not part of the spike rule any more) and "Box shows" (did
    // nothing) removed; rows BF_RATIO .. BF_OFFY, all read by the chart (via the per-market file)
    // (1.3.1) Spike vs average on its own line (the first line was ~458 px, cut off at ~400); (1.3.2) the same 7 rows in the same order
    // from ofset::bookFlowSpec() through SettingsHost.h (parameter version 3 kept: saved values keep their rows)
    ss::registerAll(this, ofset::bookFlowSpec(), bfIds());
    return RTX_OK;
}
int cppExtension::calc(int) { static_cast<BookFlow*>(this)->bfCalc(); return RTX_OK; }   // (1.3.4)

extern "C" cppExtension* CreateExtension(void) {
    BookFlow* p = new BookFlow();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE);                   // its own pane, draws itself; mouse events even when not selected
    p->setExtendedFlags(CALL_CONTINUOUSLY);                            // the order feed keeps coming while no trade prints
    p->setDescription("LRA Book Flow - how the resting side DEFENDS near price (market-by-order feed). Green = bids (buyers below price), red = offers (sellers above price). TWO MEASURES: 1) REFILL EFFORT - Question: how much defense is there compared with normal? How: lots ADDED minus lots PULLED near price this candle, per side, compared with that side's average of the last 20 candles; it is the bar and the first box row. Example: offers +2,587 (4.8x) = sellers put back 2,587 more offers than they pulled, 4.8 times their usual. How to read: 1.0x = normal; 2.0x or more = heavy defense (the bar turns bright); a bar below the zero line = that side pulled more than it added. 2) REFILLED - Question: did they put back what was taken, compared with normal for this market? How: lots re-posted at the exact prices that were just hit, within 1 second, divided by the lots eaten there, divided by that market's usual level; it is the second box row and the grey R label on bright bars. Example: buyers took 2,010 offers, sellers re-posted 8,624 at those prices = 4.3 times what was taken; ES normal is 2.2 times, so Refilled = 2.0x STRONG. How to read: 1.0x = the usual refill; STRONG = the market's top quarter (ES about 1.25x and up) = defense holding; weak = its bottom quarter (ES about 0.75x and down) = defense failing. READ (under the box) - Question: is the defense holding? At a new 10-bar low or high: C+R ? = a busy candle (2x volume) + Refill Effort 2.0x or more + Refilled STRONG on the defending side (bids at a low, offers at a high): 53% of lows / 46% of highs turned vs about 20% normally; strong refill alone: 34% / 28%; weak refill = defense failing, don't fade ?: only 10% / 8% turned. Example: 5:39 AM ES new high, offers Refill Effort 4.8x, Refilled 2.0x STRONG -> BEARISH ? C+R, price fell from 7,752 to 7,733. Measured on 7 markets and one week of order-book data: every refill read shows ? until it holds on new days. Box: the candle under the mouse while inside this pane, else the live candle. A closed candle whose side pulled at least 2.0x its 20-candle average and at least 1.2x what it added gets a pull marker at the bottom. Settings: Position (the box), Near ticks (0 = auto: how close to price counts).");
    p->setVersion("1.5.0");   // BF_VERSION
    return p;
}
