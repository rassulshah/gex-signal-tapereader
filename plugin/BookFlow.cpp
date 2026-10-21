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
#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG: the settings window never faults, never opens blank
#include <deque>
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
#include "EffortReward.h"   // (1.6.0) the Reward row
#include "OrderFlowPaneSettings.h"   // (1.3.2) the settings window table (SettingsSpec.h)
#include "SettingsHost.h"
static ss::Ids& bfIds() { static ss::Ids I; return I; }   // the row ids IRT gave the table (one DLL = one parameter list, written once in setup)
#include "MboRecord.h"              // Record order feed
#include "MarketState.h"            // (1.3.0) SRC "BOOK" facts for the read

#define BF_VERSION "1.19.0"          // keep equal to bf::VERSION and the setVersion literal below; (1.3.4, 2026-10-18 23:19 CT) the box follows the mouse only inside this pane, else Live = the forming candle
#include "PaneCursor.h"           // (1.3.4)
#include "BoxReadDraw.h"          // (1.4.0, Rassul approved 23:48 CT bookflow-read-mockup.png) the box rows Net, Pace, Read
#include "DefenseTurn.h"          // (1.12.0) the pivot turn marks + the refill line decisions (drawn on the price chart by lsDefenseLines)
#include "DefenseLinesSettings.h" // (1.12.0) their show / colour settings (one file with lsDefenseLines)
#include "TrapLogic.h"           // (1.19.0, Middle Attacker #1) the Delta Profile's TR record (read only)
#include <fstream>
#include <set>

// (1.3.0) the dialog rows (parameter version 3): every row is read by the chart
// (1.3.2) the window rows come from OrderFlowPaneSettings.h (no index constants)

class BookFlow : public off::MboPane<bf::Logic> {
public:
    SS_SAFE_DIALOG(BookFlow, "BookFlow")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    BookFlow() : off::MboPane<bf::Logic>() {}
    virtual bool barMarks() const { return false; }                        // no letters / dash drawn; hover + records keep them
    virtual bool netBars() const { return true; }
    virtual bool textOff() const { return false; }
    virtual std::string boxKeep() const { return "Refill Effort;Refilled;Counter;Re-test;Moved;Attackers;At level"; }   // (1.19.0) + Attackers (Top Attacker #2 G) + At level (Top Defender #2 W)   // (1.17.0) + Moved (Top Defender #3)   // (1.16.0) + Re-test (Top Defender #4)   // (1.15.0) + Counter (Top Defender #5)   // (1.8.0, Rassul 19:19) Reward removed - it repeats Aggression's Reward;   // (1.6.0, Rassul 17:42 "build it already") + Reward (EffortReward.h)    // (1.5.0, Rassul 11:19 / 11:53) the two defense rows; the rest hidden   // (1.4.0, 23:56 CT) + Added / Pulled (their plain 20-candle ratios, as the pull marker)            // (1.2.1, Rassul 22:54) the box: Net and Pace only (1.3.0: Net = "+120 (2.4x)")
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
        off::IconRect infoIcon; bool infoTipOn = false;                    // (1.10.0) the info icon (pane pixels) and its hover tip
        // (1.3.0) the net decisions of closed candles (decided once; only a Ratio bars / Spike vs average change re-decides), the chart
        // label + identity they belong to, the Market State facts posted
        std::map<long long, bf::NetStat> net; int netRb = -1; long long netAvg = -1; std::string identity, chart;
        long long mstBar = 0, mstCt = 0, mstMs = -1, mstPosts = 0; std::map<std::string, std::string> mstKv;
        pc::Tracker cur; long long liveEv = -1, liveMs = -1;
        br::Read lastRead; std::vector<std::string> lastReadLines;        // (1.4.0) the Read last drawn (tests)
        // (1.12.0) pivot turns: the candles (OHLC ticks, Defense / Aggression, refill price, attack timing), the decided turns (one per
        // pivot, never re-decided), the turns file, the live "?" line and the shared settings file
        std::map<long long, dt::Candle> dtc; std::set<std::pair<int, long long> > dtUsed; std::map<std::pair<int, long long>, dt::Turn> dtTurns;
        std::string dtPath, dtLivePath, dtLiveText, dtMkt; int dtSpb = 0; bool dtLoaded = false; long long dtWritten = 0, dtFileFails = 0;
        std::vector<std::pair<int, long long> > dtPendX; std::set<std::pair<int, long long> > dtXDone; long long dtXWritten = 0;   // (1.13.0) turns waiting for their exhaustion decision (60 s after the touches), done
        // (1.15.0) H: per turn with an A, did the defenders hit back (DefenseTurn counterDecide)? decided once, written as an H line
        std::map<std::pair<int, long long>, dt::Counter> dtH; std::vector<std::pair<int, long long> > dtPendH; long long dtHWritten = 0, dtHNoFeed = 0;
        long long badgesDrawn = 0;                                          // (1.15.0) badge-row draws (status / tests)
        // (1.16.0) L / X: per turn, the RE-TEST of its R line (DefenseTurn retestStep) - watched while pending, decided once, written as a Q line
        std::map<std::pair<int, long long>, dt::Retest> dtQ, dtQPend; long long dtQWritten = 0, dtQNoBook = 0, dtQOver = 0;
        std::deque<double> dtNorm[2];                                       // (1.16.0) the market normal: median resting lots of the 8 biggest prices per side, per candle close
        dt::Turn dtLive; bool dtHaveLive = false; ss::Store dlStore; ss::Values dlSet = ss::defaults(dlset::spec(), -1);
        // (1.17.0, Top Defender #3) order MOVES (MoveLogic.h): each candle's confirmed moves, the read at each local extreme (decided once, M
        // lines), the market normal (moved lots per candle per 30-min slot, kept in BookFlow\<MKT>-moves-norm.txt across restarts)
        std::map<long long, std::vector<mvl::Move> > mvByKey; std::map<std::pair<int, long long>, mvl::Read> dtM; mvl::Norm mvNorm; std::string mvNormPath;
        long long dtMWritten = 0, mvNoFeed = 0, mvNormAdds = 0;
        // (1.19.0, Top Attacker #2) G at each 5-candle high / low (DefenseTurn giveUpDecide), decided once, G lines
        std::map<std::pair<int, long long>, dt::GiveUp> dtG; long long dtGWritten = 0, gUndecided = 0;
        // (1.19.0, Middle Attacker #1) Tr: the Delta Profile's TR lines (its session record), read only; [side LOW / HIGH, candle key]
        std::map<std::pair<int, long long>, trapl::Rec> dtTr; std::map<std::string, long long> trOff; long long trMs = -1, trRead = 0, trBad = 0; std::string trPath;
        // (1.19.0, Top Defender #2) W: the key-level watch (fed by the engine's BookSink), the decided reads [candle key, level ticks], W lines
        struct LwSink : off::BookSink { dt::LevelWatch* w = nullptr; void onBook(int kind, int side, off::Tick px, long long size, long long ts, long long key) { if (w) w->onBook(kind, side, (long long)px, size, ts, key); } };
        dt::LevelWatch lw; LwSink lwSink; std::vector<dt::WLevel> lwLevels; std::map<std::pair<long long, long long>, dt::WRead> dtW; long long dtWWritten = 0;
        // (1.19.0) the badges' hover: each badge's rectangle and its text (letters, numbers, the letters with no room); the one under the mouse
        std::vector<std::pair<off::IconRect, std::string> > badgeHit; int badgeTip = -1; long long badgeDropped = 0;
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
        trackOn(S);                                                        // (1.17.0) before the engine sees the event
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
    virtual void afterPump(S_& S, long long now, bool fromDraw) { trackOn(S); mrec::Recorder* r = rec(S); if (r) r->tick(now, fromDraw); post(S, now); try { checkX(S); } catch (...) {}
        try { checkW(S); } catch (...) {} try { Ext* e = ext(S); if (e) readTraps(S, *e, now); } catch (...) {} }   // (1.19.0) W reads at touch + 60 s; the Delta Profile's TR lines
    // (1.13.0) the gray EXHAUSTION line of each decided turn: once every touch that started on its pivot / turn candle has finished (60 s),
    // the defending side's touch nearest the extreme where the attack died (DefenseTurn.h exhaustionOf) -> one X line, decided once
    void checkX(S_& S) {
        Ext* e = ext(S); if (!e || S.market.empty()) return;
        S.e.adMedian = dt::adMedianOf(S.market);
        if (e->dtPendX.empty()) return;
        S.e.finishTouches(S.e.lastS);
        for (size_t k = 0; k < e->dtPendX.size();) {
            const std::pair<int, long long> pk = e->dtPendX[k];
            std::map<std::pair<int, long long>, dt::Turn>::const_iterator it = e->dtTurns.find(pk);
            if (it == e->dtTurns.end() || e->dtXDone.count(pk)) { e->dtPendX.erase(e->dtPendX.begin() + (long)k); continue; }
            const dt::Turn& t = it->second;
            bool waiting = false; std::vector<dt::TouchV> tv;
            for (size_t q = S.e.touches.size(); q-- > 0;) {
                const off::Engine::Touch& x = S.e.touches[q];
                if (x.key != t.pivotT && x.key != t.t) { if (x.key < t.pivotT) break; continue; }
                if (!x.done) { waiting = true; break; }
                dt::TouchV v; v.s = x.s; v.px = x.px; v.key = x.key; v.ad = x.ad; tv.push_back(v);
            }
            if (waiting) { ++k; continue; }
            long long px = 0, at = 0;
            if (dt::exhaustionOf(tv, t.side, t.pivotT, t.t, t.pivotPx, px, at) && !e->dtPath.empty()) {
                std::ofstream f(e->dtPath.c_str(), std::ios::binary | std::ios::app);
                if (f.is_open()) { f << dt::xLineOf(t.t, t.side, px, S.tick, at) << "\n"; f.flush(); if (f.good()) ++e->dtXWritten; else ++e->dtFileFails; } else ++e->dtFileFails;
            }
            e->dtXDone.insert(pk); e->dtPendX.erase(e->dtPendX.begin() + (long)k);
        }
    }
    virtual void afterTimer(S_& S, long long now) { mrec::Recorder* r = rec(S); if (r) r->tick(now, false); }
    virtual void statusExtra(S_& S, std::ostream& o) {
        mrec::Recorder* r = rec(S); if (r) r->status(o); else o << "\nMBO_RECORD,unavailable (out of memory)";
        Ext* e = ext(S);
        o << "\nSPIKE_RULE,|net| >= " << off::tenthsText(avgT(S)) << (avgT(S) > 0 ? "" : " (off)") << " the plain " << S.ser.ratioBars << "-candle average of that side's |net|, decided at the close; forming candle dim, no label; colour by side"
          << "\nTURNS," << (e ? std::to_string(e->dtTurns.size()) + " decided, " + std::to_string(e->dtXWritten) + " exhaustion lines, " + std::to_string(e->dtPendX.size()) + " waiting, touches " + std::to_string(S.e.touches.size()) + ", " + std::to_string(e->dtWritten) + " written, " + std::to_string(e->dtFileFails) + " write fails, H " + std::to_string(e->dtH.size()) + " decided (" + std::to_string(e->dtHWritten) + " written, " + std::to_string(e->dtPendH.size()) + " waiting, " + std::to_string(e->dtHNoFeed) + " no feed), re-test " + std::to_string(e->dtQ.size()) + " decided (" + std::to_string(e->dtQWritten) + " written, " + std::to_string(e->dtQPend.size()) + " watching, " + std::to_string(e->dtQNoBook) + " no book, " + std::to_string(e->dtQOver) + " over), live " + (e->dtHaveLive ? "?" : "none") + ", file " + e->dtPath : std::string("n/a"))
          << "\nMOVES," << (e ? std::to_string(e->dtM.size()) + " reads (" + std::to_string(e->dtMWritten) + " written, " + std::to_string(e->mvNoFeed) + " candles without the order feed), seen modify " + std::to_string(S.e.mvt.seenM) + " / re-post " + std::to_string(S.e.mvt.seenR) + ", dropped following " + std::to_string(S.e.mvt.following) + " / ladder " + std::to_string(S.e.mvt.ladder) + " / short rest " + std::to_string(S.e.mvt.shortRest) + " / small " + std::to_string(S.e.mvt.small) + " / ambiguous " + std::to_string(S.e.mvt.ambiguous) + ", confirmed " + std::to_string(S.e.mvt.confirmed) + ", min lots " + std::to_string(S.e.mvt.minLots) + ", normal candles " + std::to_string(e->mvNorm.n[0]) + ", P drawn only (B / attacker cases recorded)" : std::string("n/a"))
          << "\nGIVE_UP," << (e ? std::to_string(e->dtG.size()) + " decided at 5-candle extremes (" + std::to_string(e->dtGWritten) + " written, " + std::to_string(e->gUndecided) + " without the feed / < 20 s after the extreme), G drawn: pulled >= 2x normal net of moves, pulled > added, attack died, no initiative candle" : std::string("n/a"))
          << "\nTRAPS," << (e ? std::to_string(e->dtTr.size()) + " Delta Profile TR lines read (" + std::to_string(e->trBad) + " bad), file " + e->trPath : std::string("n/a"))
          << "\nAT_LEVEL," << (e ? std::to_string(e->lwLevels.size()) + " key levels watched, " + std::to_string(e->lw.touchesSeen) + " touches (" + std::to_string(e->lw.deduped) + " within 15 min), " + std::to_string(e->lw.open.size()) + " measuring, " + std::to_string(e->dtW.size()) + " decided (" + std::to_string(e->dtWWritten) + " written), W drawn at supports only" : std::string("n/a"))
          << "\nBADGES," << (e ? std::to_string(e->badgesDrawn) + " row draws, " + std::to_string(e->badgeDropped) + " badges with letters moved to the hover (max " + std::to_string(dt::BADGE_MAX) + ")" : std::string("n/a"))
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
        try { decideTurns(S, key, nb); } catch (...) {}                    // (1.12.0) the pivot turn of this closed candle, decided once
    }
    virtual void onAccClosed(S_& S, long long key, const off::Acc& a) { trackOn(S); Ext* e = ext(S); if (e) { fillAcc(e->dtc[key], a); e->dtc[key].t = key;
        e->mvByKey[key] = a.moves; while (e->mvByKey.size() > 60) e->mvByKey.erase(e->mvByKey.begin()); } }   // (1.17.0) the candle's confirmed moves
    // (1.17.0) the shared engine tracks order moves only for this pane (MoveLogic.h; size floor = the market's q90 of moves)
    void trackOn(S_& S) { S.e.trackMoves = true; S.e.mvt.minLots = mvl::minLotsOf(S.market);
        Ext* e = ext(S); if (e) { e->lwSink.w = &e->lw; S.e.sink = &e->lwSink; } }   // (1.19.0) the key-level watch sees every classified book event
    // (1.12.0) from a candle's raw accumulator: the price per side where the most lots were re-posted after a hit (Ice.rebuilt), and the
    // attack at the candle's low / high (lots hit before / after it printed, seconds)
    static void fillAcc(dt::Candle& q, const off::Acc& a) {
        for (int s = 0; s < 2; ++s) {
            long long best = 0, px = 0;
            for (std::map<off::Tick, off::Ice>::const_iterator it = a.ice[s].begin(); it != a.ice[s].end(); ++it)
                if (it->second.rebuilt > best) { best = it->second.rebuilt; px = it->first; }
            q.refPx[s] = px; q.refLots[s] = best; q.hit[s] = a.hitC[s];
        }
        q.span = a.tFirst >= 0 && a.tLast >= a.tFirst ? a.tLast - a.tFirst + 1 : 0;
        if (a.tLo >= 0 && a.tFirst >= 0) { q.before[dt::LOW] = a.cumLo[0]; q.secsBefore[dt::LOW] = a.tLo - a.tFirst + 1; q.after[dt::LOW] = a.hitC[0] - a.cumLo[0]; q.secsAfter[dt::LOW] = a.tLast - a.tLo; }
        if (a.tHi >= 0 && a.tFirst >= 0) { q.before[dt::HIGH] = a.cumHi[1]; q.secsBefore[dt::HIGH] = a.tHi - a.tFirst + 1; q.after[dt::HIGH] = a.hitC[1] - a.cumHi[1]; q.secsAfter[dt::HIGH] = a.tLast - a.tHi; }
        // (1.19.0, G) the attackers' own resting orders after the extreme: at the high the BIDS (buyers), at the low the OFFERS (sellers)
        for (int sd = 0; sd < 2; ++sd) {
            const bool hiSide = sd == dt::HIGH; if ((hiSide ? a.tHi : a.tLo) < 0) continue;
            const int rs = hiSide ? off::BID : off::ASK;
            q.apull[sd] = std::max(0LL, a.pullC[rs] - (hiSide ? a.pullHi[rs] : a.pullLo[rs]));
            q.aadd[sd] = std::max(0LL, a.addC[rs] - (hiSide ? a.addHi[rs] : a.addLo[rs]));
            long long mv = 0; for (size_t k = hiSide ? a.movHi : a.movLo; k < a.moves.size(); ++k) if (a.moves[k].side == rs) mv += a.moves[k].size;
            q.amov[sd] = mv; q.gOk[sd] = true;
        }
    }
    // the chart's candle at index i (OHLC in ticks) + its pane numbers; the raw parts (refill, attack) only if this run saw the candle form
    bool candleAt(S_& S, int i, RTARRAYI& times, RTARRAY& op, RTARRAY& hi, RTARRAY& lo, RTARRAY& cl, const off::Rec* r, dt::Candle& out) {
        Ext* e = ext(S); if (!e || i < 0 || i >= times.count || i >= op.count || i >= hi.count || i >= lo.count || i >= cl.count || !(S.tick > 0)) return false;
        const long long key = (long long)times[i];
        std::map<long long, dt::Candle>::const_iterator it = e->dtc.find(key);
        dt::Candle q; if (it != e->dtc.end()) q = it->second;
        off::Tick o = 0, h = 0, l = 0, c = 0;
        if (!off::toTick((double)op[i], S.tick, o) || !off::toTick((double)hi[i], S.tick, h) || !off::toTick((double)lo[i], S.tick, l) || !off::toTick((double)cl[i], S.tick, c)) return false;
        q.t = key; q.o = o; q.h = h; q.l = l; q.c = c;
        if (!r) { std::map<long long, off::PaneBar>::const_iterator b = S.ser.bars.find(key); if (b == S.ser.bars.end() || !b->second.ok) return false; r = &b->second.r; }
        for (int s = 0; s < 2; ++s) { q.net[s] = bf::netOf(*r, s); q.aggr[s] = r->aggrC[s]; }
        q.ok = true; out = q; return true;
    }
    void dtFiles(S_& S, Ext& e) {
        const std::string f = off::flexDir();
        const std::string p = dlset::turnsPath(f, S.market, (int)S.spb);
        if (p == e.dtPath && e.dtLoaded) return;
        e.dtPath = p; e.dtLivePath = dlset::livePath(f, S.market, (int)S.spb); e.dtLoaded = true; e.dtMkt = S.market; e.dtSpb = (int)S.spb;
        e.dtUsed.clear(); e.dtTurns.clear(); e.dtLiveText.clear(); e.dtPendX.clear(); e.dtXDone.clear();
        if (p.empty()) return;
        off::makeDir(f); off::makeDir(f + "\\BookFlow");
        std::ifstream in(p.c_str(), std::ios::binary); std::string ln; std::vector<std::string> keep;
        e.dtH.clear(); e.dtPendH.clear(); e.dtQ.clear(); e.dtQPend.clear(); e.dtM.clear(); e.dtG.clear(); e.dtW.clear();
        while (std::getline(in, ln)) { dt::XLine X; if (dt::parseX(ln, X)) { keep.push_back(ln); e.dtXDone.insert(std::make_pair(X.side, X.t)); continue; }
            dt::Counter HC; if (dt::parseH(ln, HC)) { keep.push_back(ln); e.dtH[std::make_pair(HC.side, HC.t)] = HC; continue; }   // (1.15.0) decided H: redrawn the same
            dt::Retest QR; if (dt::parseQ(ln, QR)) { keep.push_back(ln); e.dtQ[std::make_pair(QR.side, QR.t)] = QR; continue; }
            mvl::Read MR; if (mvl::parseM(ln, MR)) { keep.push_back(ln); e.dtM[std::make_pair(MR.ext, MR.t)] = MR; continue; }
            dt::GiveUp GU; if (dt::parseG(ln, GU)) { keep.push_back(ln); e.dtG[std::make_pair(GU.side, GU.t)] = GU; continue; }   // (1.19.0) decided G: redrawn the same
            dt::WRead WR; if (dt::parseW(ln, WR, S.market)) { keep.push_back(ln); e.dtW[std::make_pair(WR.key, WR.px)] = WR; continue; }   // (1.19.0) decided W: redrawn the same   // (1.17.0) decided moves: redrawn the same    // (1.16.0) decided L / X: redrawn the same
            dt::Line L; if (!dt::parseLine(ln, L) || L.kind != 'T') continue; keep.push_back(ln);
            dt::Turn t; t.t = L.t; t.side = L.side; t.marginPct = L.margin; t.E = L.E; t.A = L.A; t.defNet = L.defNet; t.attNet = L.attNet;
            off::Tick a = 0, b = 0; if (off::toTick(L.pivotPx, L.tick, a)) t.pivotPx = a; if (off::toTick(L.linePx, L.tick, b)) t.linePx = b;
            e.dtTurns[std::make_pair(t.side, t.t)] = t; }
        // the pivot key is not in the file: a reloaded turn blocks its own candle and the one before (the two possible pivots)
        for (std::map<std::pair<int, long long>, dt::Turn>::const_iterator it = e.dtTurns.begin(); it != e.dtTurns.end(); ++it) e.dtUsed.insert(std::make_pair(it->first.first, it->first.second));
        if (keep.size() > 3000) {                                          // bounded: keep the newest 2,000 turns
            std::string o; for (size_t k = keep.size() - 2000; k < keep.size(); ++k) o += keep[k] + "\n";
            off::publishFile(p, o);
        }
    }
    void decideTurns(S_& S, long long key, const off::PaneBar& nb) {
        Ext* e = ext(S); if (!e || S.market.empty()) return;
        dtFiles(S, *e);
        std::map<long long, int>::const_iterator bi = S.barEnds.find(key); if (bi == S.barEnds.end() || bi->second < 2) return;
        RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
        const int i = bi->second;
        if (i >= times.count || (long long)times[i] != key) return;
        std::vector<dt::Candle> c(3);
        if (!candleAt(S, i - 2, times, op, hi, lo, cl, nullptr, c[0]) || !candleAt(S, i - 1, times, op, hi, lo, cl, nullptr, c[1]) || !candleAt(S, i, times, op, hi, lo, cl, &nb.r, c[2])) return;
        e->dtc[key] = c[2];
        while (e->dtc.size() > 400) e->dtc.erase(e->dtc.begin());
        auto used = [&](int sd, long long t) { return e->dtUsed.count(std::make_pair(sd, t)) > 0; };
        for (int sd = 0; sd < 2; ++sd) {
            if (e->dtTurns.count(std::make_pair(sd, key))) continue;       // decided once
            dt::Turn t; if (!dt::decide(c, 2, sd, false, used, t)) continue;
            e->dtUsed.insert(std::make_pair(sd, t.pivotT)); e->dtUsed.insert(std::make_pair(sd, t.t));
            e->dtTurns[std::make_pair(sd, key)] = t;
            if (!e->dtPath.empty()) { std::ofstream f(e->dtPath.c_str(), std::ios::binary | std::ios::app); if (f.is_open()) { f << dt::lineOf('T', t, S.tick) << "\n"; f.flush(); if (f.good()) ++e->dtWritten; else ++e->dtFileFails; } else ++e->dtFileFails; }
            e->dtPendX.push_back(std::make_pair(sd, key));                 // (1.13.0) its exhaustion is decided once the touches near it finish
            if (t.A) e->dtPendH.push_back(std::make_pair(sd, key));        // (1.15.0) its counter (H) is decided COUNTER_BARS candles later
            { dt::Retest q; q.t = key; q.side = sd; q.linePx = t.linePx ? t.linePx : t.pivotPx; e->dtQPend[std::make_pair(sd, key)] = q; }   // (1.16.0) its re-test is watched from the next candle
        }
        decideCounters(S, i, times, op, hi, lo, cl, nb);
        decideRetests(S, i, times, op, hi, lo, cl, nb);
        decideMoves(S, i, times, op, hi, lo, cl);
        decideGiveUp(S, i, times, hi, lo, c[2]);                           // (1.19.0) Top Attacker #2
    }
    // (1.19.0, Top Attacker #2 G, Rassul approved 14:20 CT) at a 5-candle high / low (the research universe): are the ATTACKERS giving up?
    // (DefenseTurn giveUpDecide: their own resting orders pulled >= 2x normal after the extreme, net of their moves, more pulled than added,
    // attack died; no G on an initiative candle). Decided once at this close, one G line per decided extreme (G or not: the nightly's
    // controls); a candle without the order feed or with < 20 s after its extreme = nothing (never guessed).
    void decideGiveUp(S_& S, int i, RTARRAYI& times, RTARRAY& hi, RTARRAY& lo, const dt::Candle& x) {
        Ext* e = ext(S); if (!e || i < 5 || S.market.empty() || !(S.tick > 0)) return;
        const long long key = (long long)times[i];
        bool isHigh = true, isLow = true;
        for (int j = i - 5; j < i; ++j) { off::Tick hj = 0, lj = 0; if (!off::toTick((double)hi[j], S.tick, hj) || !off::toTick((double)lo[j], S.tick, lj)) return; if (hj > x.h) isHigh = false; if (lj < x.l) isLow = false; }
        if (!isHigh && !isLow) return;
        std::vector<long long> prior;                                       // the deltas of the 20 candles before (initiative veto)
        for (int j = std::max(0, i - 20); j < i; ++j) { std::map<long long, off::PaneBar>::const_iterator b = S.ser.bars.find((long long)times[j]); if (b != S.ser.bars.end() && b->second.ok) prior.push_back(b->second.r.aggrC[1] - b->second.r.aggrC[0]); }
        for (int sd = 0; sd < 2; ++sd) {
            if (!(sd == dt::HIGH ? isHigh : isLow) || e->dtG.count(std::make_pair(sd, key))) continue;
            dt::GiveUp g; if (!dt::giveUpDecide(x, sd, dt::giveUpNormal(S.market, sd), dt::initiativeOf(x, sd, prior), g)) { ++e->gUndecided; continue; }
            e->dtG[std::make_pair(sd, key)] = g;
            if (!e->dtPath.empty()) { std::ofstream f(e->dtPath.c_str(), std::ios::binary | std::ios::app); if (f.is_open()) { f << dt::gLineOf(g) << "\n"; f.flush(); if (f.good()) ++e->dtGWritten; else ++e->dtFileFails; } else ++e->dtFileFails; }
        }
        while (e->dtG.size() > 3000) e->dtG.erase(e->dtG.begin());
    }
    // (1.19.0, Top Defender #2 W, Rassul approved 14:22 CT) the key levels (the zone read's key list, PDH / PDL / PFH / PFL / ONH / ONL / LonHI /
    // LonLO only) into the level watch; each read finished at touch + 60 s decided once (DefenseTurn wDecide), written as a W line
    void checkW(S_& S) {
        Ext* e = ext(S); if (!e || S.market.empty() || !(S.tick > 0)) return;
        std::vector<dt::WLevel> L;
        for (size_t k = 0; k < S.zr.e.lv.size(); ++k) { const zread::LevelZone& z = S.zr.e.lv[k]; if (z.kind != 0 || !dt::wKeyName(z.name)) continue; dt::WLevel w; w.name = z.name; w.px = (long long)z.c; L.push_back(w); }
        bool same = L.size() == e->lwLevels.size(); for (size_t k = 0; same && k < L.size(); ++k) same = L[k].px == e->lwLevels[k].px && L[k].name == e->lwLevels[k].name;
        if (!same) { e->lw.tol = dt::wTolOf(S.market); e->lw.rearm = S.zone.hwMax; e->lw.setLevels(L); e->lwLevels = L; }
        e->lw.step(S.e.lastS);
        if (e->lw.done.empty()) return;
        dtFiles(S, *e);
        for (size_t k = 0; k < e->lw.done.size(); ++k) {
            dt::WRead r = e->lw.done[k]; const std::pair<long long, long long> wk(r.key, r.px);
            if (e->dtW.count(wk) || r.key <= 0) continue;                    // decided once
            dt::wDecide(r, S.market); e->dtW[wk] = r;
            if (!e->dtPath.empty()) { std::ofstream f(e->dtPath.c_str(), std::ios::binary | std::ios::app); if (f.is_open()) { f << dt::wLineOf(r, S.tick) << "\n"; f.flush(); if (f.good()) ++e->dtWWritten; else ++e->dtFileFails; } else ++e->dtFileFails; }
        }
        e->lw.done.clear();
        while (e->dtW.size() > 2000) e->dtW.erase(e->dtW.begin());
    }
    // (1.19.0, Middle Attacker #1 Tr) the Delta Profile's TR lines from its session record (lsFlexLevels\DeltaProfile\<MKT>-<contract>-<spb>-
    // signals-<session>.csv, this session and the one before), read every 3 s from where the last read stopped (append-only; a shorter file =
    // rewritten, read again). Read only: never re-decided here. First line per candle + side wins (as the Delta Profile itself).
    static std::string contractTok(const std::string& sym) {
        std::string out;
        for (size_t i = 0; i < sym.size() && out.size() < 64; ++i) { char c = sym[i]; if (c == '|' || c == ',' || c == '\n' || c == '\r') c = ' ';
            const unsigned char u = (unsigned char)c; const bool keep = (u >= 'A' && u <= 'Z') || (u >= 'a' && u <= 'z') || (u >= '0' && u <= '9') || u == '_' || u == '-' || u == '.';
            out += keep ? c : '_'; }
        return out.empty() ? std::string("unknown-contract") : out;
    }
    std::string trapPathFor(S_& S, long long sessOff) {
        const std::string f = off::flexDir(); if (f.empty() || S.market.empty() || S.symbol.empty() || S.spb <= 0 || S.formingKey <= 0) return std::string();
        struct tm t; std::memset(&t, 0, sizeof(t)); if (!getLocaltime((RTDATE)S.formingKey, &t)) return std::string();
        const long long sess = off::civilDays(t.tm_year + 1900, (unsigned)t.tm_mon + 1, (unsigned)t.tm_mday) + (t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec >= 17 * 3600 ? 1 : 0) - sessOff;
        return f + "\\DeltaProfile\\" + S.market + "-" + contractTok(S.symbol) + "-" + std::to_string(S.spb) + "-signals-" + std::to_string(sess) + ".csv";
    }
    void readTraps(S_& S, Ext& e, long long now) {
        if (e.trMs >= 0 && now >= e.trMs && now - e.trMs < 3000) return;
        e.trMs = now;
        for (long long so = 1; so >= 0; --so) {
            const std::string p = trapPathFor(S, so); if (p.empty()) continue;
            if (so == 0) e.trPath = p;
            std::ifstream in(p.c_str(), std::ios::binary); if (!in.is_open()) continue;
            in.seekg(0, std::ios::end); const long long size = (long long)in.tellg(); if (size < 0) continue;
            long long& off0 = e.trOff[p]; if (size < off0) off0 = 0; if (size == off0) continue;
            if (size - off0 > 8 * 1024 * 1024) { off0 = size; continue; }      // bounded: a runaway file is skipped, never loaded whole
            in.seekg(off0, std::ios::beg); std::string buf((size_t)(size - off0), '\0'); in.read(&buf[0], (std::streamsize)buf.size());
            const size_t last = buf.rfind('\n'); if (last == std::string::npos) continue;   // only whole lines
            off0 += (long long)last + 1;
            size_t a = 0;
            while (a <= last) { const size_t b = buf.find('\n', a); std::string ln = buf.substr(a, b - a); a = b + 1;
                if (!ln.empty() && ln.back() == '\r') ln.pop_back();
                if (ln.compare(0, 3, "TR|") != 0) continue;
                trapl::Rec r; if (!trapl::parse(ln, r)) { ++e.trBad; continue; }
                const std::pair<int, long long> k(r.top ? dt::HIGH : dt::LOW, (long long)r.barRaw);
                if (!e.dtTr.count(k)) { e.dtTr[k] = r; ++e.trRead; } }
        }
        while (e.trOff.size() > 8) e.trOff.erase(e.trOff.begin());
        while (e.dtTr.size() > 2000) e.dtTr.erase(e.dtTr.begin());
    }
    // (1.17.0, Top Defender #3) at a LOCAL EXTREME (this candle's low / high beyond the 5 candles before it): the order moves in the zone
    // (max(2 ticks, 0.5 ATR) around the extreme) in this candle and the one before -> the four cases (MoveLogic readAt), decided once,
    // written as an M line; then this candle's moved lots join the market normal (after the decision: no look-ahead). No order feed for
    // the candle = nothing decided (never guessed).
    void loadMoveNorm(S_& S, Ext& e) {
        const std::string f = off::flexDir(); const std::string p = f.empty() || S.market.empty() ? std::string() : f + "\\BookFlow\\" + S.market + "-moves-norm.txt";
        if (p == e.mvNormPath) return;
        e.mvNormPath = p; e.mvNorm = mvl::Norm();
        if (p.empty()) return;
        std::ifstream in(p.c_str(), std::ios::binary); std::string ln; if (std::getline(in, ln)) { mvl::Norm z; if (z.parse(ln)) e.mvNorm = z; }
    }
    void decideMoves(S_& S, int i, RTARRAYI& times, RTARRAY& op, RTARRAY& hi, RTARRAY& lo, RTARRAY& cl) {
        Ext* e = ext(S); if (!e || i < 6 || S.market.empty() || !(S.tick > 0)) return;
        loadMoveNorm(S, *e);
        const long long key = (long long)times[i], prev = (long long)times[i - 1];
        std::map<long long, std::vector<mvl::Move> >::const_iterator cur = e->mvByKey.find(key);
        if (cur == e->mvByKey.end()) { ++e->mvNoFeed; return; }
        std::vector<mvl::Move> win; std::map<long long, std::vector<mvl::Move> >::const_iterator pv = e->mvByKey.find(prev);
        if (pv != e->mvByKey.end()) win = pv->second;
        win.insert(win.end(), cur->second.begin(), cur->second.end());
        off::Tick h = 0, l = 0; if (!off::toTick((double)hi[i], S.tick, h) || !off::toTick((double)lo[i], S.tick, l)) return;
        bool isLow = true, isHigh = true;
        for (int j = i - 5; j < i; ++j) { off::Tick hj = 0, lj = 0; if (!off::toTick((double)hi[j], S.tick, hj) || !off::toTick((double)lo[j], S.tick, lj)) return; if (lj < l) isLow = false; if (hj > h) isHigh = false; }
        std::vector<dt::Candle> back;
        for (int j = std::max(0, i - 15); j <= i; ++j) { dt::Candle b; off::Tick o = 0, hh = 0, ll = 0, c = 0;
            if (j < op.count && off::toTick((double)op[j], S.tick, o) && off::toTick((double)hi[j], S.tick, hh) && off::toTick((double)lo[j], S.tick, ll) && off::toTick((double)cl[j], S.tick, c)) { b.o = o; b.h = hh; b.l = ll; b.c = c; b.ok = true; }
            back.push_back(b); }
        const long long zone = mvl::zoneOf(dt::atrOf(back));
        const int sl = off::slotOfKey(key);
        const double norm[2] = { e->mvNorm.of(0, sl), e->mvNorm.of(1, sl) };
        for (int ext = 0; ext < 2; ++ext) {
            if (!(ext == 0 ? isLow : isHigh) || e->dtM.count(std::make_pair(ext, key))) continue;
            mvl::Read r = mvl::readAt(key, ext, ext == 0 ? l : h, mvl::sumIn(win, ext == 0 ? l : h, zone), norm); r.tick = S.tick;
            if (r.s.in[0] + r.s.out[0] + r.s.in[1] + r.s.out[1] == 0) continue;   // no move in the zone: nothing recorded (the nightly's control = no M line)
            e->dtM[std::make_pair(ext, key)] = r;
            if (!e->dtPath.empty()) { std::ofstream f(e->dtPath.c_str(), std::ios::binary | std::ios::app); if (f.is_open()) { f << mvl::mLineOf(r) << "\n"; f.flush(); if (f.good()) ++e->dtMWritten; else ++e->dtFileFails; } else ++e->dtFileFails; }
        }
        while (e->dtM.size() > 3000) e->dtM.erase(e->dtM.begin());
        long long lots[2] = { 0, 0 }; for (size_t k = 0; k < cur->second.size(); ++k) lots[cur->second[k].side] += cur->second[k].size;
        e->mvNorm.add(sl, lots); ++e->mvNormAdds;
        if (!e->mvNormPath.empty()) { off::makeDir(off::flexDir() + "\\BookFlow"); off::publishFile(e->mvNormPath, e->mvNorm.text() + "\n"); }
    }
    // (1.16.0, Top Defender #4) the resting lots per price now (the engine's book) -> the market normal, and each pending re-test stepped once
    // per closed candle: price left the line by 0.5 ATR, then came back into the zone -> L / X / N from the lots resting there at the close
    // BEFORE the return (no look-ahead). Decided once, written as a Q line. A turn decided this candle starts on the next one.
    long long restIn(S_& S, int side, long long lo, long long hi) {
        long long s = 0; for (long long p = lo; p <= hi; ++p) s += std::max(0LL, S.e.restAt(side == dt::HIGH ? off::ASK : off::BID, (off::Tick)p)); return s;
    }
    void decideRetests(S_& S, int i, RTARRAYI& times, RTARRAY& op, RTARRAY& hi, RTARRAY& lo, RTARRAY& cl, const off::PaneBar& nb) {
        Ext* e = ext(S); if (!e) return;
        for (int sd = 0; sd < 2; ++sd) {                                   // the normal: median lots of the 8 biggest resting prices on this side
            const std::unordered_map<off::Tick, long long>& rm = S.e.rest[sd == dt::HIGH ? off::ASK : off::BID];
            std::vector<long long> v; for (std::unordered_map<off::Tick, long long>::const_iterator it = rm.begin(); it != rm.end(); ++it) if (it->second > 0) v.push_back(it->second);
            if (v.size() >= 3) { std::sort(v.rbegin(), v.rend()); if (v.size() > 8) v.resize(8); e->dtNorm[sd].push_back((double)v[v.size() / 2]); while (e->dtNorm[sd].size() > 200) e->dtNorm[sd].pop_front(); }
        }
        auto normalOf = [&](int sd) { if (e->dtNorm[sd].size() < 5) return 0.0; std::vector<double> v(e->dtNorm[sd].begin(), e->dtNorm[sd].end()); std::nth_element(v.begin(), v.begin() + (long)v.size() / 2, v.end()); return v[v.size() / 2]; };
        const long long key = (long long)times[i];
        dt::Candle q; if (!candleAt(S, i, times, op, hi, lo, cl, &nb.r, q)) return;
        std::vector<dt::Candle> back;
        for (int j = std::max(0, i - 15); j <= i; ++j) { dt::Candle b; off::Tick o = 0, h = 0, l = 0, c = 0;
            if (j < op.count && off::toTick((double)op[j], S.tick, o) && off::toTick((double)hi[j], S.tick, h) && off::toTick((double)lo[j], S.tick, l) && off::toTick((double)cl[j], S.tick, c)) { b.o = o; b.h = h; b.l = l; b.c = c; b.ok = true; }
            back.push_back(b); }
        const double atr = dt::atrOf(back);
        for (std::map<std::pair<int, long long>, dt::Retest>::iterator it = e->dtQPend.begin(); it != e->dtQPend.end();) {
            dt::Retest& r = it->second;
            if (e->dtQ.count(it->first)) { it = e->dtQPend.erase(it); continue; }
            if (r.t >= key) { ++it; continue; }
            long long a = 0, b = 0; dt::rtRange(r, atr, a, b);
            const int res = dt::retestStep(r, q, atr, restIn(S, r.side, a, b), normalOf(r.side));
            if (res == 0) { ++it; continue; }
            if (res == 1) {
                e->dtQ[it->first] = r;
                if (!e->dtPath.empty()) { std::ofstream f(e->dtPath.c_str(), std::ios::binary | std::ios::app); if (f.is_open()) { f << dt::qLineOf(r) << "\n"; f.flush(); if (f.good()) ++e->dtQWritten; else ++e->dtFileFails; } else ++e->dtFileFails; }
            } else if (res == -2) ++e->dtQNoBook; else ++e->dtQOver;
            it = e->dtQPend.erase(it);
        }
    }
    // (1.15.0, Top Defender #5) each turn with an A waiting for its counter: the candles after it (this run's order feed) -> H decided once
    void decideCounters(S_& S, int i, RTARRAYI& times, RTARRAY& op, RTARRAY& hi, RTARRAY& lo, RTARRAY& cl, const off::PaneBar& nb) {
        Ext* e = ext(S); if (!e) return;
        for (size_t k = 0; k < e->dtPendH.size();) {
            const std::pair<int, long long> pk = e->dtPendH[k];
            std::map<std::pair<int, long long>, dt::Turn>::const_iterator it = e->dtTurns.find(pk);
            std::map<long long, int>::const_iterator ti = S.barEnds.find(pk.second);
            if (it == e->dtTurns.end() || e->dtH.count(pk) || ti == S.barEnds.end()) { e->dtPendH.erase(e->dtPendH.begin() + (long)k); continue; }
            const int t0 = ti->second;
            std::vector<dt::Candle> after;
            for (int j = t0 + 1; j <= i; ++j) { dt::Candle q; if (!candleAt(S, j, times, op, hi, lo, cl, j == i ? &nb.r : nullptr, q)) q.ok = false; after.push_back(q); }
            std::vector<dt::Candle> back;                                    // the ATR: the 15 candles up to the turn (OHLC only)
            for (int j = std::max(0, t0 - 15); j <= t0; ++j) { dt::Candle q; off::Tick o = 0, h = 0, l = 0, c = 0;
                if (j < op.count && off::toTick((double)op[j], S.tick, o) && off::toTick((double)hi[j], S.tick, h) && off::toTick((double)lo[j], S.tick, l) && off::toTick((double)cl[j], S.tick, c)) { q.o = o; q.h = h; q.l = l; q.c = c; q.ok = true; }
                back.push_back(q); }
            dt::Counter C; const int r = dt::counterDecide(it->second, after, dt::atrOf(back), C);
            if (r == 0 && (int)after.size() < dt::COUNTER_BARS + 3) { ++k; continue; }
            if (r == 1) {
                e->dtH[pk] = C;
                if (!e->dtPath.empty()) { std::ofstream f(e->dtPath.c_str(), std::ios::binary | std::ios::app); if (f.is_open()) { f << dt::hLineOf(C) << "\n"; f.flush(); if (f.good()) ++e->dtHWritten; else ++e->dtFileFails; } else ++e->dtFileFails; }
            } else if (r == -2 || r == 0) ++e->dtHNoFeed;                   // a window candle without the order feed: no H decided (never guessed)
            e->dtPendH.erase(e->dtPendH.begin() + (long)k);
        }
    }
    // the forming candle's "?" (pivot rule + the defending Defense bar bigger so far); published for lsDefenseLines only when it changes
    void liveTurn(S_& S, int last, RTARRAYI& times, const off::PaneBar* lb) {
        Ext* e = ext(S); if (!e) return;
        e->dtHaveLive = false;
        if (lb && last >= 2 && !S.market.empty()) {
            dtFiles(S, *e);
            RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
            std::vector<dt::Candle> c(3);
            if (candleAt(S, last - 2, times, op, hi, lo, cl, nullptr, c[0]) && candleAt(S, last - 1, times, op, hi, lo, cl, nullptr, c[1]) && candleAt(S, last, times, op, hi, lo, cl, &lb->r, c[2])) {
                std::map<long long, off::Acc>::const_iterator ac = S.accs.find(S.formingKey);
                if (ac != S.accs.end()) fillAcc(c[2], ac->second);
                auto used = [&](int sd, long long t) { return e->dtUsed.count(std::make_pair(sd, t)) > 0; };
                for (int sd = 0; sd < 2 && !e->dtHaveLive; ++sd) { dt::Turn t; if (dt::decide(c, 2, sd, true, used, t)) { e->dtLive = t; e->dtHaveLive = true; } }
            }
        }
        const std::string txt = e->dtHaveLive ? dt::lineOf('L', e->dtLive, S.tick) + "\n" : std::string("N\n");
        if (txt != e->dtLiveText && !e->dtLivePath.empty()) { if (off::publishFile(e->dtLivePath, txt)) e->dtLiveText = txt; }
    }
    // (1.15.0, Rassul 11:22 / 11:25 CT, defender5-mockup-v2 "we could also have multiple like A|H and adjust font size as needed") the
    // BADGE ROW: a thin solid strip at the top of the pane; on each closed candle ONE badge with the behaviours decided on it ("A", "H",
    // "A|H"), letters only + "?" until proven, green where buyers defend a low / red where sellers defend a high, the font shrinking (9 -> 6
    // pt) to fit the candle's slot; never over the box; drawn only when a badge is on screen. dt::badgeText decides the text.
    // (1.19.0) the badge's hover text: the badge, one line per letter (what it means + its numbers - numbers never on the chart), the letters
    // with no room on the badge, the "?" note. Drawn as the info icon's tip box while the mouse is on the badge.
    std::string badgeTipText(S_& S, Ext& e, long long key, const dt::BadgeIn& in, const std::string& badge, const std::vector<std::string>& dropped) {
        std::string t = badge;
        const char* dn = in.defSide == dt::HIGH ? "sellers (offers)" : "buyers (bids)";
        if (in.A) t += std::string("\nA  absorbed: attackers out-hit the ") + dn + ", lost the close";
        if (in.H) t += "\nH  the defenders hit back after the A";
        if (in.tr >= 0) { std::map<std::pair<int, long long>, trapl::Rec>::const_iterator r = e.dtTr.find(std::make_pair(in.tr, key));
            if (r != e.dtTr.end()) t += "\nTr  " + dt::trapHover(r->second.top, r->second.lots, r->second.x); }
        if (in.g >= 0) { std::map<std::pair<int, long long>, dt::GiveUp>::const_iterator g = e.dtG.find(std::make_pair(in.g, key));
            if (g != e.dtG.end()) { char b[220]; std::snprintf(b, sizeof b, "\nG  attackers giving up (%s at the %s):\n    %s pulled %.1fx normal, %lld pulled vs %lld added, pace %.1fx",
                in.g == dt::HIGH ? "buyers" : "sellers", in.g == dt::HIGH ? "high" : "low", in.g == dt::HIGH ? "bids" : "offers", g->second.x, g->second.pull, g->second.add, g->second.pace < 0 ? 0.0 : g->second.pace); t += b; } }
        if (in.w) for (std::map<std::pair<long long, long long>, dt::WRead>::const_iterator w = e.dtW.lower_bound(std::make_pair(key, (long long)LLONG_MIN)); w != e.dtW.end() && w->first.first == key; ++w) {
            if (!dt::wDrawn(w->second)) continue;
            char b[220]; std::snprintf(b, sizeof b, "\nW  defenders withdrew at %s %s:\n    %lld bid lots pulled (net) vs %lld traded in 1 min", w->second.name.c_str(), zread::pxText(w->second.px, S.tick).c_str(),
                std::max(0LL, w->second.pulled - w->second.added), w->second.filled); t += b; }
        if (in.bidMove) t += std::string("\n") + in.bidMove + (in.bidMove == 'P' ? "  bids pressed in (moved toward price)" : "  bids stepped back (moved away)");
        if (in.offerMove) t += std::string("\n") + in.offerMove + (in.offerMove == 'P' ? "  offers pressed in (moved toward price)" : "  offers stepped back (moved away)");
        if (in.Q == 'L') t += "\nL  reloaded on the re-test"; else if (in.Q == 'X') t += "\nX  empty on the re-test";
        if (!dropped.empty()) { std::string d; for (size_t k = 0; k < dropped.size(); ++k) d += (k ? ", " : "") + dropped[k]; t += "\nNot on the badge (no room): " + d; }
        t += "\n? = not proven yet for this market";
        return t;
    }
    // (1.15.0, Rassul 11:22 / 11:25 CT, defender5-mockup-v2 "we could also have multiple like A|H and adjust font size as needed") the
    // BADGE ROW: a thin solid strip at the top of the pane; on each closed candle ONE badge with the behaviours decided on it ("A", "H",
    // "A|H"), letters only + "?" until proven, green where buyers defend a low / red where sellers defend a high, the font shrinking (9 -> 6
    // pt) to fit the candle's slot; never over the box; drawn only when a badge is on screen. (1.19.0) dt::badgeSegs7: + Tr / G / W, at most
    // 4 letters (A, H, Tr, G, W, P/B, L/X - the rest in the hover); every badge's rectangle + hover text kept for the mouse.
    template <class XOf>
    void badgeRow(S_& S, Ext& e, const off::PaneGeo& g, int v0, int v1, int last, int slot, RTARRAYI& times, XOf xOf, const bp::Box* box) {
        struct B { int x; std::vector<dt::BadgeSeg> segs; std::string s, tip; };   // (1.18.0) coloured parts per letter
        std::vector<B> bs;
        e.badgeHit.clear();
        for (int i = std::max(0, v0); i <= v1 && i < last; ++i) {
            const long long key = (long long)times[i];
            dt::BadgeIn in; int side = -1;
            for (int sd = 0; sd < 2; ++sd) { std::map<std::pair<int, long long>, dt::Turn>::const_iterator it = e.dtTurns.find(std::make_pair(sd, key)); if (it != e.dtTurns.end() && it->second.A) { in.A = true; side = sd; } }
            for (std::map<std::pair<int, long long>, dt::Counter>::const_iterator h = e.dtH.begin(); h != e.dtH.end(); ++h) if (h->second.H && h->second.at == key) { in.H = true; if (side < 0) side = h->second.side; }
            for (std::map<std::pair<int, long long>, dt::Retest>::const_iterator qq = e.dtQ.begin(); qq != e.dtQ.end(); ++qq) if (qq->second.at == key && (qq->second.res == 'L' || qq->second.res == 'X')) { in.Q = qq->second.res; if (side < 0) side = qq->second.side; }   // (1.16.0)
            for (int sd = 0; sd < 2; ++sd) { std::map<std::pair<int, long long>, mvl::Read>::const_iterator m = e.dtM.find(std::make_pair(sd, key)); if (m == e.dtM.end()) continue;   // (1.18.0) every move case, per BOOK side
                const char lb = mvl::letterOf(m->second, mvl::BIDS), lo = mvl::letterOf(m->second, mvl::OFFERS); if (lb && !in.bidMove) in.bidMove = lb; if (lo && !in.offerMove) in.offerMove = lo; if ((lb || lo) && side < 0) side = sd; }
            for (int sd = 1; sd >= 0 && in.tr < 0; --sd) if (e.dtTr.count(std::make_pair(sd, key))) in.tr = sd;                      // (1.19.0) Tr: the Delta Profile's decision
            for (int sd = 1; sd >= 0 && in.g < 0; --sd) { std::map<std::pair<int, long long>, dt::GiveUp>::const_iterator gg = e.dtG.find(std::make_pair(sd, key)); if (gg != e.dtG.end() && gg->second.G) in.g = sd; }   // (1.19.0) G
            for (std::map<std::pair<long long, long long>, dt::WRead>::const_iterator w = e.dtW.lower_bound(std::make_pair(key, (long long)LLONG_MIN)); w != e.dtW.end() && w->first.first == key && !in.w; ++w) if (dt::wDrawn(w->second)) in.w = true;   // (1.19.0) W (supports)
            if (side < 0) side = in.tr >= 0 ? in.tr : (in.g >= 0 ? in.g : dt::LOW);
            in.defSide = side;
            std::vector<std::string> dropped;
            const std::vector<dt::BadgeSeg> segs = dt::badgeSegs7(in, &dropped); if (segs.empty()) continue;   // (1.16.0) X alone: the opposite colour
            const int xc = xOf(i); if (xc < g.left || xc > g.edge) continue;
            if (!dropped.empty()) ++e.badgeDropped;
            const std::string txt = dt::segsText(segs);
            bs.push_back(B{ xc, segs, txt, badgeTipText(S, e, key, in, txt, dropped) });
        }
        if (bs.empty()) return;
        const int y0 = g.top + g.reserve + 1, h = 15, y1 = y0 + h;
        if (y1 + 10 > g.bottom) return;                                    // pane too short
        off::fillFlat(this, g.left, y0, g.edge, y1, (COLOR)0x000B0F15);
        off::fillFlat(this, g.left, y1, g.edge, y1, (COLOR)0x00374151);
        const int maxW = std::max(12, slot - 2);
        int prevR = INT_MIN;                                               // (1.19.0) a badge wider than its slot never covers the one before it
        for (size_t k = 0; k < bs.size(); ++k) {
            auto colOf = [&](int key) { return (COLOR)(key == dt::HIGH ? e.dlSet.c(dlset::K_COL_HIGH) : e.dlSet.c(dlset::K_COL_LOW)); };
            const COLOR col = colOf(bs[k].segs.front().colourKey);         // the border: the first letter's colour
            FONT f; f.id = HELVETICA; f.style = BOLD;
            const int fs = dt::fitFont([&](int pt) { f.size = pt; setFont(f); return (int)getTextWidth(bs[k].s.c_str(), -1) + 4; }, maxW, 9, 6);
            f.size = fs; setFont(f); const int w = (int)getTextWidth(bs[k].s.c_str(), -1) + 4;
            int l = bs[k].x - w / 2; if (prevR != INT_MIN && l <= prevR + 1) l = prevR + 2;   // shifted right past the badge before it
            const int r = l + w;
            if (box && l < box->r && r > box->l && y0 < box->b && y1 > box->t) continue;   // never over the box
            if (r > g.edge) continue;                                         // no room left on the pane
            prevR = r;
            off::fillFlat(this, l, y0 + 1, r, y0 + 1, col); off::fillFlat(this, l, y1 - 1, r, y1 - 1, col);
            off::fillFlat(this, l, y0 + 1, l, y1 - 1, col); off::fillFlat(this, r, y0 + 1, r, y1 - 1, col);
            int xt = l + 2;                                                    // (1.18.0) each letter in its side's colour
            for (size_t q = 0; q < bs[k].segs.size(); ++q) { const std::string& part = bs[k].segs[q].s; const int pw = (int)getTextWidth(part.c_str(), -1);
                setTextColor(colOf(bs[k].segs[q].colourKey)); RCT rc; rc.set(off::coord(xt), off::coord(y0 + 1), off::coord(std::min(r - 1, xt + pw + 1)), off::coord(y1 - 1)); rc.drawText(part.c_str(), false, false); xt += pw; }
            off::IconRect ir; ir.l = l; ir.t = y0 + 1; ir.r = r; ir.b = y1 - 1; e.badgeHit.push_back(std::make_pair(ir, bs[k].tip));   // (1.19.0) the hover
        }
        if (e.badgeTip >= (int)e.badgeHit.size()) e.badgeTip = -1;
        ++e.badgesDrawn;
    }
    // a small filled triangle (pointing down = at a high, up = at a low), tip at (cx, tipY)
    void triangle(int cx, int tipY, bool down, COLOR col) {
        for (int r = 0; r < 5; ++r) { const int y = down ? tipY - r : tipY + r; off::fillFlat(this, cx - r, y, cx + r, y, col); }
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
    virtual int parmsLoadBody(void) {
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
    virtual int parmsApplyBody(void) { saveBf(); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::onUpdate(this, ofset::bookFlowSpec(), bfIds(), (int)i, snapBf_); saveBf(); return RTX_OK; }

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
            Ext* e = ext(*S); if (e) { int cx = 0, cy = 0; const bool ok = pc::cursorPos(cx, cy); e->cur.onEvent(off::cursorWindow(), ok, cx, cy, ev->v.mouse.h, ev->v.mouse.v);
                                       if (off::infoTip(e->infoIcon, e->infoTipOn, ev->v.mouse.h, ev->v.mouse.v, bf::INFO_TIP)) invalidateChart(false);   // (1.10.0) the info icon's description; (1.11.0) drawn as a box
                                       if (badgeHover(*e, ev->v.mouse.h, ev->v.mouse.v)) invalidateChart(false); }   // (1.19.0) the badge's letters + numbers
        }
        const int r = off::MboPane<bf::Logic>::mouse(ev);
        if (S && ev && (ev->type == E_MOUSE_MOVE || ev->type == E_MOUSE_HOVER)) {
            Ext* e = ext(*S); if (e && !e->cur.eventInside(ev->v.mouse.h, ev->v.mouse.v) && S->hov.hover.set(-1)) invalidateChart(false);
        }
        return r;
    }
    // (1.19.0) the badge under the mouse (its hover text drawn as a tip box); true = changed
    static bool badgeHover(Ext& e, int h, int v) {
        int hit = -1; for (size_t k = 0; k < e.badgeHit.size() && hit < 0; ++k) if (e.badgeHit[k].first.has(h, v)) hit = (int)k;
        if (hit == e.badgeTip) return false;
        e.badgeTip = hit; return true;
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
        long long top = 0, up = 0, down = 0;
        std::vector<long long> mags;
        for (int i = v0; i <= v1; ++i) { const off::PaneBar* b = barOf(i); if (!b) continue; top = std::max(top, magOf(b->r)); for (int s = 0; s < 2; ++s) { const long long v = netOf(b->r, s); if (v > up) up = v; if (-v > down) down = -v; mags.push_back(v < 0 ? -v : v); } }
        { const long long cp = off::capTop(mags); up = off::capped(up, cp); down = off::capped(down, cp); top = off::capped(top, cp); }   // (1.7.0, Rassul 18:26) the scale cap
        if (S.axisTop > top) top = S.axisTop;
        up = std::max(up, S.axisUp); down = std::max(down, S.axisDown);   // (1.5.1) the axis IRT was given (same bars)
        S.netUp = up; S.netDown = down;
        S.paneH = pane.bottom - pane.top;
        off::PaneGeo g = off::paneGeo(this, gmode(S.set.mode));
        g.reserve += off::QUESTION_H;                                      // (1.8.0) the question line on top
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
        {   // (1.6.0, EffortReward.h) REWARD per side: how far the attack INTO that side moved price per 1,000 lots it took (ticks) vs the market's
            // normal - bids column = the sellers hitting the bids (down), offers column = the buyers lifting the offers (up); "absorbed" <= 0.5x
            off::InfoRow rw; rw.label = "Reward"; rw.v[0] = rw.v[1] = "n/a";
            const int ci = liveBox ? last : mb;
            RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
            if (mbar && ci >= 1 && op.count > ci && hi.count > ci && lo.count > ci && cl.count > ci)
                for (int q = 0; q < 2; ++q) {
                    er::Side sd; sd.rewOk = er::rewardX(op[ci], hi[ci], lo[ci], cl[ci - 1], S.tick, mbar->r.hitC[q], q, S.market, sd.rew);
                    std::vector<long long> pr;                                   // (1.7.0) the attack's effort: lots taken vs the 20 candles before
                    std::map<long long, off::PaneBar>::const_iterator it = S.ser.bars.lower_bound(liveBox ? (long long)times[last] : mt);
                    while (it != S.ser.bars.begin() && (int)pr.size() < er::EFFORT_BARS) { --it; if (it->second.ok) pr.push_back(it->second.r.hitC[q]); }
                    sd.effOk = er::effortX(mbar->r.hitC[q], pr, sd.eff);
                    er::guard(sd); rw.v[q] = er::rewardCellOf(sd);
                }
            info.rows.push_back(rw);
        }
        {   // (1.15.0, Top Defender #5) the COUNTER row: the latest turn with an A within 12 candles of the shown candle - in the defending side's
            // column "Hit back 2.4x" (decided), "Absorbed only" (decided, no counter) or "Watching" (its window is still open); else "-"
            off::InfoRow cr; cr.label = "Counter"; cr.v[0] = cr.v[1] = "-";
            if (Ext* e = ext(S)) {
                const long long sk = liveBox ? (long long)times[last] : mt;
                std::map<long long, int>::const_iterator si = S.barEnds.find(sk);
                const int sIdx = si != S.barEnds.end() ? si->second : (liveBox ? last : -1);
                const dt::Turn* best = nullptr; int bIdx = -1;
                for (std::map<std::pair<int, long long>, dt::Turn>::const_iterator it = e->dtTurns.begin(); it != e->dtTurns.end(); ++it) {
                    const dt::Turn& t = it->second; if (!t.A || t.t > sk) continue;
                    std::map<long long, int>::const_iterator ti = S.barEnds.find(t.t); if (ti == S.barEnds.end() || sIdx < 0 || sIdx - ti->second > 12) continue;
                    if (!best || ti->second > bIdx) { best = &t; bIdx = ti->second; }
                }
                if (best) {
                    std::map<std::pair<int, long long>, dt::Counter>::const_iterator h = e->dtH.find(std::make_pair(best->side, best->t));
                    const int col = best->side == dt::HIGH ? 1 : 0;
                    cr.v[col] = h != e->dtH.end() ? dt::counterText(h->second) : std::string(sIdx - bIdx <= dt::COUNTER_BARS ? "Watching" : "-");
                }
            }
            info.rows.push_back(cr);
        }
        {   // (1.16.0, Top Defender #4) the RE-TEST row: the latest re-test decided within 12 candles of the shown candle - in the defending side's
            // column "Reloaded 1.9x" / "Empty 0.2x" / "0.6x" (between); else "-"
            off::InfoRow qr; qr.label = "Re-test"; qr.v[0] = qr.v[1] = "-";
            if (Ext* e = ext(S)) {
                const long long sk = liveBox ? (long long)times[last] : mt;
                std::map<long long, int>::const_iterator si = S.barEnds.find(sk);
                const int sIdx = si != S.barEnds.end() ? si->second : (liveBox ? last : -1);
                const dt::Retest* best = nullptr; int bIdx = -1;
                for (std::map<std::pair<int, long long>, dt::Retest>::const_iterator it = e->dtQ.begin(); it != e->dtQ.end(); ++it) {
                    const dt::Retest& r = it->second; if (r.at > sk) continue;
                    std::map<long long, int>::const_iterator ri = S.barEnds.find(r.at); if (ri == S.barEnds.end() || sIdx < 0 || sIdx - ri->second > 12) continue;
                    if (!best || ri->second > bIdx) { best = &r; bIdx = ri->second; }
                }
                if (best) qr.v[best->side == dt::HIGH ? 1 : 0] = dt::retestText(*best);
            }
            info.rows.push_back(qr);
        }
        {   // (1.17.0, Top Defender #3) the MOVED row: the latest move read within 12 candles of the shown candle where a case qualified - per
            // side (its column), "In 2t 2.3x" (moved toward price) / "Back 5t 2.6x" (away); else "-"
            off::InfoRow mr; mr.label = "Moved"; mr.v[0] = mr.v[1] = "-";
            if (Ext* e = ext(S)) {
                const long long sk = liveBox ? (long long)times[last] : mt;
                std::map<long long, int>::const_iterator si = S.barEnds.find(sk);
                const int sIdx = si != S.barEnds.end() ? si->second : (liveBox ? last : -1);
                const mvl::Read* best = nullptr; int bIdx = -1;
                for (std::map<std::pair<int, long long>, mvl::Read>::const_iterator it = e->dtM.begin(); it != e->dtM.end(); ++it) {
                    const mvl::Read& r = it->second; if (r.t > sk || !(r.q[0] || r.q[1] || r.q[2] || r.q[3])) continue;
                    std::map<long long, int>::const_iterator ri = S.barEnds.find(r.t); if (ri == S.barEnds.end() || sIdx < 0 || sIdx - ri->second > 12) continue;
                    if (!best || ri->second > bIdx) { best = &r; bIdx = ri->second; }
                }
                if (best) { mr.v[0] = mvl::movedText(*best, mvl::BIDS); mr.v[1] = mvl::movedText(*best, mvl::OFFERS); }
            }
            info.rows.push_back(mr);
        }
        {   // (1.19.0, Top Attacker #2) the ATTACKERS row: the latest G decision (at a 5-candle high / low) within 12 candles of the shown candle, in
            // the attackers' column (buyers at a high, sellers at a low), coloured by meaning: "Pulling 2.9x" red at a high / green at a low (G),
            // "Still in 0.6x" the other colour (decided, no G); else "-"
            off::InfoRow ar; ar.label = "Attackers"; ar.v[0] = ar.v[1] = "-";
            if (Ext* e = ext(S)) {
                const long long sk = liveBox ? (long long)times[last] : mt;
                std::map<long long, int>::const_iterator si = S.barEnds.find(sk);
                const int sIdx = si != S.barEnds.end() ? si->second : (liveBox ? last : -1);
                const dt::GiveUp* best = nullptr; int bIdx = -1;
                for (std::map<std::pair<int, long long>, dt::GiveUp>::const_iterator it = e->dtG.begin(); it != e->dtG.end(); ++it) {
                    const dt::GiveUp& gq = it->second; if (gq.t > sk) continue;
                    std::map<long long, int>::const_iterator gi = S.barEnds.find(gq.t); if (gi == S.barEnds.end() || sIdx < 0 || sIdx - gi->second > 12) continue;
                    if (!best || gi->second > bIdx || (gi->second == bIdx && gq.G && !best->G)) { best = &gq; bIdx = gi->second; }
                }
                if (best) { const int col = best->side == dt::HIGH ? 0 : 1; ar.v[col] = dt::attackersText(*best);
                            const int bad = best->side == dt::HIGH ? 1 : 0; ar.side[col] = best->G ? bad : 1 - bad; }
            }
            info.rows.push_back(ar);
        }
        {   // (1.19.0, Top Defender #2) the AT LEVEL row: the latest key-level SUPPORT touch read (touch + 60 s) within 12 candles of the shown
            // candle, in the buyers' column: "Pulled 88%" red (W) / "Filled 2.1x" grey / "-" (resistances: recorded only)
            off::InfoRow lr; lr.label = "At level"; lr.v[0] = lr.v[1] = "-";
            if (Ext* e = ext(S)) {
                const long long sk = liveBox ? (long long)times[last] : mt;
                std::map<long long, int>::const_iterator si = S.barEnds.find(sk);
                const int sIdx = si != S.barEnds.end() ? si->second : (liveBox ? last : -1);
                const dt::WRead* best = nullptr; int bIdx = -1;
                for (std::map<std::pair<long long, long long>, dt::WRead>::const_iterator it = e->dtW.begin(); it != e->dtW.end(); ++it) {
                    const dt::WRead& w = it->second; if (w.side != dt::LOW || w.key > sk) continue;
                    std::map<long long, int>::const_iterator wi = S.barEnds.find(w.key); if (wi == S.barEnds.end() || sIdx < 0 || sIdx - wi->second > 12) continue;
                    if (!best || wi->second >= bIdx) { best = &w; bIdx = wi->second; }
                }
                if (best) { lr.v[0] = dt::atLevelText(*best); lr.side[0] = best->W ? 1 : (best->F ? 2 : -1); }
            }
            info.rows.push_back(lr);
        }
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
        if (liveBox && haveLive) {                                         // (1.14.0) the early-warning read while the candle forms (bf::earlyRead)
            bf::EwTouch tw0;
            for (size_t k = S.e.touches.size(); k-- > 0;) {
                const off::Engine::Touch& t = S.e.touches[k];
                if (t.key != S.formingKey) { if (t.key < S.formingKey) break; continue; }
                if (t.h1 <= 0) continue;
                tw0.ok = true; tw0.s = t.s; tw0.px = (double)t.px * S.tick; tw0.elapsed = S.e.lastS - t.t0; tw0.h1 = t.h1; tw0.h2 = t.h2; break;
            }
            br::Read er; if (bf::earlyRead(tw0, lb.r, S.market, S.e.adMedian, S.tick, er)) rd = er;
        }
        if (S.feed.silentNow(off::steadyMs())) { rd = br::Read(); rd.head = off::silentRead(S.feed.silentFromWall, S.feed.reconnects); }   // (1.7.0) the watchdog says so
        const off::TextW tw = off::measureWith(this);
        info.topLines = 1; info.topW = off::questionW(tw, bf::QUESTION);   // (1.15.0, Rassul 11:30) the question + info icon: the box's first line
        const br::Plan rp = br::addAndPlan(info, rd, tw, [&](const off::Info& I) { return off::planPanel(area, g.top, off::MODE_NUMBERS, pl, I, std::string(), std::string(), g.reserve, tw); }, P);
        if (Ext* e = ext(S)) { e->lastRead = rd; e->lastReadLines = rp.lines; }
        S.lastBox = P.ok ? P.box : bp::Box();
        std::vector<off::LBox> avoid; off::ValY vyNow;
        if (P.ok) avoid.push_back(off::LBox{ P.box.l, P.box.t, P.box.r, P.box.b, 0, 0 });
        for (size_t k = 0; k < P.textRects.size(); ++k) avoid.push_back(off::LBox{ P.textRects[k].l, P.textRects[k].t, P.textRects[k].r, P.textRects[k].b, 0, 0 });
        // (1.8.0) the pull markers' bottom strip is gone with the pair bars (the pull stats stay in the record and the status)
        {   // (1.8.0, Rassul 19:02 / 19:09) PAIR bars: bids (buyers defending) green LEFT, offers (sellers defending) red RIGHT, both above zero
            const COLOR normal[2] = { off::C_GREEN_BAR, off::C_RED_BAR }, spike[2] = { off::C_GREEN_SPIKE, off::C_RED_SPIKE };
            vyNow = off::paneValY(v0, top, g);                               // (1.14.0, Rassul 08:19 / 14:22) the y-axis drives the bars
            off::drawPairBars(this, g, v0, v1, slot, top, normal, spike, xOf, [&](int i) {
                off::BarLook k; const off::PaneBar* pb = barOf(i); if (!pb) return k;
                const bool closed = i != last; const bf::NetStat* ns = closed ? netAt(S, (long long)times[i]) : nullptr;
                k.ok = true;
                for (int s = 0; s < 2; ++s) { const bf::NetLook nk = bf::netLookOf(pb, ns, s, closed, S.market); k.v[s] = nk.v; k.spike[s] = nk.bright; k.lab1[s] = nk.ratio; k.lab2[s] = nk.pace; }
                return k;
            }, avoid, 0, &vyNow);
        }
        try { liveTurn(S, last, times, haveLive ? &lb : nullptr); } catch (...) {}
        if (Ext* e = ext(S)) {                                             // (1.12.0) the turn marks over the defending bar: confirmed "+NN%", forming "+NN% ?"
            if (!S.market.empty()) { const std::string sp = dlset::settingsPath(off::flexDir(), S.market); if (e->dlStore.path != sp) { e->dlStore = ss::Store(); e->dlStore.S = &dlset::spec(); e->dlStore.path = sp; }
                                     if (!sp.empty()) e->dlSet = e->dlStore.load(ss::marketIndex(S.market), off::steadyMs()); }
            if (e->dlSet.b(dlset::K_SHOW_MARKS) && top > 0) {
                FONT mf; mf.id = HELVETICA; mf.size = off::LABEL_PT; mf.style = PLAIN; setFont(mf);
                auto mark = [&](int i, const dt::Turn& t, bool live) {
                    const off::PaneBar* pb = barOf(i); if (!pb) return;
                    const int xc = xOf(i); if (xc < g.left || xc > g.edge) return;
                    const off::Pair pr = off::pairIn(xc, slot); const int s = t.side == dt::HIGH ? 1 : 0;
                    const long long v = netOf(pb->r, s); int yb = g.bottom - 1; const int h = off::pairBarH(&vyNow, g, top, v, yb);   // (1.14.0) on the axis like the bar
                    const bf::NetStat* ns = live ? nullptr : netAt(S, (long long)times[i]);
                    const bool lab = off::PAIR_LABELS && ns && ns->spike[s];   // (1.14.0) bars carry no labels now: the mark sits right on the bar
                    const int cx = (pr.l[s] + pr.r[s]) / 2; int tip = yb - h - 3 - (lab ? 2 * off::LABEL_LH : 0);
                    const COLOR col = (COLOR)(t.side == dt::HIGH ? e->dlSet.c(dlset::K_COL_HIGH) : e->dlSet.c(dlset::K_COL_LOW));
                    if (tip - 5 - off::LABEL_LH < g.top + g.reserve) tip = g.top + g.reserve + 5 + off::LABEL_LH;
                    triangle(cx, tip, t.side == dt::HIGH, col);
                    const std::string txt = dt::marginText(t.marginPct) + (live ? " ?" : "");
                    setTextColor(col); const int w = (int)getTextWidth(txt.c_str(), -1); const int y = tip - 5 - off::LABEL_LH;
                    RCT rc; rc.set(off::coord(cx - w / 2), off::coord(y), off::coord(cx - w / 2 + w + 2), off::coord(y + off::LABEL_LH)); rc.drawText(txt.c_str(), false, false);
                };
                for (int i = v0; i <= v1 && i < last; ++i)
                    for (int sd = 0; sd < 2; ++sd) { std::map<std::pair<int, long long>, dt::Turn>::const_iterator it = e->dtTurns.find(std::make_pair(sd, (long long)times[i])); if (it != e->dtTurns.end()) mark(i, it->second, false); }
                if (e->dtHaveLive && last <= v1) mark(last, e->dtLive, true);
            }
            if (top > 0) badgeRow(S, *e, g, v0, v1, last, slot, times, xOf, P.ok ? &P.box : nullptr);   // (1.15.0) the badge row
        }
        const COLOR vcol[2] = { 0x0022C55E, 0x00EF4444 };                 // values: bids (buyers) green, offers (sellers) red
        off::drawPanel(this, P, info, vcol, off::implOf(zl), false);
        br::draw(this, P, info, rp, rd);                                  // (1.4.0) the Read word + lines over its rows
        { Ext* ex = ext(S); off::IconRect tmp; if (P.ok) off::drawQuestionIn(this, P.box, bf::QUESTION, ex ? &ex->infoIcon : &tmp); else if (ex) ex->infoIcon = off::IconRect();   // (1.15.0) inside the box, upper left   // (1.8.0) the question; (1.10.0) + the info icon
          if (ex && ex->infoTipOn) off::drawInfoTip(this, g, ex->infoIcon, bf::INFO_TIP);   // (1.11.0) the description box while the mouse is on the icon
          if (ex && !ex->infoTipOn && ex->badgeTip >= 0 && ex->badgeTip < (int)ex->badgeHit.size()) { off::PaneGeo tg = g; tg.right = std::min(g.right, g.edge);   // (1.19.0) the badge's hover, kept left of the price axis
              off::drawInfoTip(this, tg, ex->badgeHit[(size_t)ex->badgeTip].first, ex->badgeHit[(size_t)ex->badgeTip].second.c_str()); } }
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
        const off::NetGeo n = off::netGeoFor(g.bottom - g.top, g.reserve, S.netUp, S.netDown);   // (1.5.1) zero line where the data needs it
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
        const off::NetGeo n = off::netGeoFor(g.bottom - g.top, g.reserve, S.netUp, S.netDown);   // (1.5.1) zero line where the data needs it
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
                const int h = up ? off::barHeight(mg, n.upTop, n.maxPx) : off::barHeight(mg, n.downTop, n.downPx);   // (1.5.1) one scale, up / down
                if (h > off::CAP_TIP_PX && mg > (up ? n.upTop : n.downTop)) {   // (1.7.0) a capped bar: the white tip at its outer end (drawn after the bar)
                    const COLOR c0 = k.bright ? spike[k.col] : normal[k.col];
                    if (up) { off::fillFlat(this, p.l[s], y0 - h, p.r[s], y0 - 1, c0); off::fillFlat(this, p.l[s], y0 - h, p.r[s], y0 - h + off::CAP_TIP_PX - 1, off::C_WHITE); }
                    else { off::fillFlat(this, p.l[s], y0 + 2, p.r[s], y0 + 1 + h, c0); off::fillFlat(this, p.l[s], y0 + 2 + h - off::CAP_TIP_PX, p.r[s], y0 + 1 + h, off::C_WHITE); }
                    if (k.bright && (!k.ratio.empty() || !k.pace.empty())) { Cand q; q.cx = (p.l[s] + p.r[s]) / 2; q.hb = h; q.up = up; q.mag = mg; q.a = k.ratio; q.b = k.pace; cands.push_back(q); }
                    continue;
                }
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
    p->setVersion("1.19.0");   // BF_VERSION
    return p;
}
