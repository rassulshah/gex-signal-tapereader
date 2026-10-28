/********************************************************************************
 *  BookFlow.cpp  --  Investor/RT RTX extension  lsBookFlow  1.5.0  (2026-10-19; 1.0.0 2026-10-16)
 *  1.23.0 (2026-10-22, Rassul approved layout-stages-2strips.png 11:41-11:46 CT, build ordered 15:20 CT): the badge row becomes TWO LETTER
 *        STRIPS (LetterStrips.h 1.0.0), each its own row with a small left label: "Holding" (stages 2-3: A > Q > W > P > B; Q only when the
 *        queue meter's record lsFlexLevels\Strips\QUEUE-<MKT>-<spb>.csv exists) and "Turn & re-test" (stages 4-5: H > L > X). ONE letter per
 *        strip per candle (the strongest; the others in the hover), coloured by the side that ACTS (defenders: bids at a low green, offers at a
 *        high red; P / B the book side that moved; warnings W / B / X in the colour of the side they hurt), no boxes / borders; a letter that
 *        would touch its neighbour drops to the lower half-row. Hovers per strip. Every letter appended to lsFlexLevels\Strips\DEF-<MKT>-<spb>.csv
 *        for lsReversalStage 1.0.0. The net bars sit below the strips (scale() keeps their rows). Bars, rows box, records: unchanged.
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
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "BookFlow"
#endif
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
#include "SelfAware.h"           // (1.29.0) self-awareness
#include "MarketState.h"            // (1.3.0) SRC "BOOK" facts for the read
#include <chrono>
#include "SignalBoard.h"            // (1.31.0) kind D facts (decided candles' defense)

#define BF_VERSION "1.31.0"          // 1.31.0 (2026-10-28 overnight) the decided candle's defense posted on the SignalBoard as kind D (bf::defenseFact; layer 1a, one owner)
// prev:         // 1.30.0 (2026-10-26, Rassul 18:33 CT) the letter strips are not drawn (letters recorded / exported / in the status), their rows go to the bars; the turn % marks stay
// prev:          // (1.29.0, 2026-10-26) self-awareness SELFDEF (SelfAware.h, SELF-AWARE.md; ships OFF per market) // 1.28.0 (2026-10-26, BUILD-SPEC Defense pane, Rassul approved 22:29 CT 2026-10-25): F? (amber, FMirror.h 1.0.0: the pivot likely dies at once - attackers stacking in front + defenders pulling behind in the pivot bar's last 60 s, decided at its close, F lines in BookFlow\<MKT>-<spb>-fmirror.csv, drawn only with the per-market thresholds from lsFlexLevels\LRA-FMirror-<MKT>.csv); the shared A shows as grey "Ab", H as "Hb?" (defender colour) only after an Ab at the same pivot within 10 candles, L / X as "L?" / "X?" with the re-test hover (same order / new big order / small orders only, lots vs normal); letters coloured by meaning (bullish green / bearish red: W, B, X, Q! at a low = red); at most 2 codes per candle (one per strip); records unchanged // 1.27.0 (2026-10-23 4-step stage revamp) Defense strips renamed Floor / Release (LetterStrips 1.3.0); // was 1.26.0:          // 1.25.0 (2026-10-22 evening fix build) compact strips, bars >= 50% of the pane, one-line Implied row, the Q hover hides the anchor token;          // keep equal to bf::VERSION and the setVersion literal below; (1.3.4, 2026-10-18 23:19 CT) the box follows the mouse only inside this pane, else Live = the forming candle
#include "PaneCursor.h"           // (1.3.4)
#include "BoxReadDraw.h"          // (1.4.0, Rassul approved 23:48 CT bookflow-read-mockup.png) the box rows Net, Pace, Read
#include "FMirror.h"              // (1.28.0) F? the pivot dies at once
#include "OrderLinesLogic.h"       // (1.28.0) the line floor = a "big order" in the re-test hover
#include <set>
struct BfZoneIds { std::set<long long> big; long long bigId = 0, bigLots = 0, maxLots = 0, n = 0; };   // (1.28.0) the re-test zone by order id
#include "DefenseTurn.h"          // (1.12.0) the pivot turn marks + the refill line decisions (drawn on the price chart by lsDefenseLines)
#include "DefenseLinesSettings.h" // (1.12.0) their show / colour settings (one file with lsDefenseLines)
#include "SharedAbsorb.h"        // (1.22.0) the SHARED absorption A: the Defense badge A read from the one shared record
#include "TrapLogic.h"           // (1.19.0, Middle Attacker #1) the Delta Profile's TR record (read only)
#include "LetterStrips.h"        // (1.23.0) the two letter strips + the letter record for lsReversalStage
#include "ReversalEvidence.h"    // additive typed sidecar; legacy LT strips stay independent
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
    int topReserve() const { return 0; }   // (1.29.0, Rassul 18:33 CT "remove all the signal letters") no letter strips drawn: the rows go to the bars; was lst::STRIPS * lst::ROW_H + 2   // (1.23.0) the two letter strips under the question line (the net bars below them)
    // (1.23.0) the pane's axis = the base axis with the strips' rows kept free at the top (the same px the drawing uses: g.reserve + topReserve)
    virtual int scale(int iStartBar, int iEndBar, double* dMin, double* dMax) {
        const int r = off::MboPane<bf::Logic>::scale(iStartBar, iEndBar, dMin, dMax);
        S_* S = static_cast<S_*>(getUserData());
        if (S && dMin && dMax) { double mn = 0, mx = 10; off::axisFor(S->axisTop, S->paneH, off::pairPx(S->paneH, off::textReserve(S->paneH, gmode(S->set.mode)) + off::QUESTION_H + topReserve()), mn, mx); *dMin = mn; *dMax = mx; }
        return r;
    }
    virtual bool textOff() const { return false; }
    virtual std::string boxKeep() const { return "Refill Effort;Refilled;Counter;Re-test;Moved;At level"; }   // (1.21.0, Rassul 20:11-20:23 CT) the Attackers row removed (G moved to the attacker pane)   // (1.19.0) + Attackers (Top Attacker #2 G) + At level (Top Defender #2 W)   // (1.17.0) + Moved (Top Defender #3)   // (1.16.0) + Re-test (Top Defender #4)   // (1.15.0) + Counter (Top Defender #5)   // (1.8.0, Rassul 19:19) Reward removed - it repeats Aggression's Reward;   // (1.6.0, Rassul 17:42 "build it already") + Reward (EffortReward.h)    // (1.5.0, Rassul 11:19 / 11:53) the two defense rows; the rest hidden   // (1.4.0, 23:56 CT) + Added / Pulled (their plain 20-candle ratios, as the pull marker)            // (1.2.1, Rassul 22:54) the box: Net and Pace only (1.3.0: Net = "+120 (2.4x)")
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
        sab::Host sab; long long sabX = 0;                                  // (1.22.0) the shared A record (read only here) + pull extras written
        mrec::Recorder r; off::SessionFile stack, pull;
        off::IconRect infoIcon; bool infoTipOn = false;                    // (1.10.0) the info icon (pane pixels) and its hover tip
        // (1.3.0) the net decisions of closed candles (decided once; only a Ratio bars / Spike vs average change re-decides), the chart
        // label + identity they belong to, the Market State facts posted
        std::map<long long, bf::NetStat> net; int netRb = -1; long long netAvg = -1; std::string identity, chart;
        std::map<long long, sb::Fact> dFacts; long long dPostMs = -100000; size_t dPosted = 0;   // (1.31.0) SignalBoard D
        long long mstBar = 0, mstCt = 0, mstMs = -1, mstPosts = 0; std::map<std::string, std::string> mstKv; sa::Hook self;
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
        struct LwSink : off::BookSink { dt::LevelWatch* w = nullptr; fmr::Acc* f = nullptr; void onBook(int kind, int side, off::Tick px, long long size, long long ts, long long key) { if (w) w->onBook(kind, side, (long long)px, size, ts, key); if (f) f->on(kind, side, (long long)px, size, ts); } };   // (1.28.0) + the F? window
        // (1.28.0) F?: the classified adds / pulls of the last 2 minutes, the params, the decided reads [candle key], the F file
        fmr::Acc fAcc; fmr::Params fPar; std::string fParPath, fPath; long long fParMs = -100000, fWritten = 0, fFails = 0, fArmed = 0; std::map<long long, fmr::Read> fReads;
        // (1.28.0) the re-test hover: per pending re-test the big order ids in its zone at the turn, the decided classification [side, turn key]
        std::map<std::pair<int, long long>, std::set<long long> > rtIdsAtTurn; std::map<std::pair<int, long long>, BfZoneIds> rtSnap; std::map<std::pair<int, long long>, std::string> rtInfo; std::string rtPath;
        dt::LevelWatch lw; LwSink lwSink; std::vector<dt::WLevel> lwLevels; std::map<std::pair<long long, long long>, dt::WRead> dtW; long long dtWWritten = 0;
        // (1.19.0) the badges' hover: each badge's rectangle and its text (letters, numbers, the letters with no room); the one under the mouse
        std::vector<std::pair<off::IconRect, std::string> > badgeHit; int badgeTip = -1; long long badgeDropped = 0;
        long long badgeTipNoRoom = 0, badgeTipCompact = 0; dt::TipRect lastTip; int lastTipLevel = -1;                   // (1.20.0) hovers with no free spot (status); the last drawn hover box (tests)
        // (1.23.0) the letter strips: the DEF record ids already written, the queue meter's Q letters [candle raw key], counters
        std::set<std::string> stripIds; std::string stripPath, stripQPath; long long stripMs = -1, stripQOff = 0, stripQRead = 0, stripQBad = 0, stripWritten = 0, stripFail = 0, stripsDrawn = 0, stripSkipped = 0;
        std::map<long long, std::vector<lst::Lt> > stripQ;
        rse::Publisher evidence; std::string evidencePath; long long evidenceWritten = 0, evidenceFail = 0; // one publisher per pane host
        std::map<long long, int> evidenceNative; // closed-bar MBO coverage retained before the accumulator is dropped
        std::map<long long, std::map<off::Tick, off::Ice> > evidenceIce[2]; // exact-price execution provenance only; last 64 bars
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
        try { checkW(S); } catch (...) {} try { Ext* e = ext(S); if (e) readTraps(S, *e, now); } catch (...) {} try { sharedA(S, now); } catch (...) {} try { strips(S, now); } catch (...) {} }
    // (1.22.0, 2026-10-22 Rassul 06:43 "then do it") THE SHARED A (SharedAbsorb.h): this pane no longer decides its own A - the badge A comes
    // from the one shared record lsBidAskStats / lsDeltaProfile decide into (the same A on every pane and the price chart). This pane adds
    // what only it knows: the attackers pulling their own orders after the extreme (its G read, x normal) as an SABX extra, once per A.
    void sharedA(S_& S, long long now) {
        Ext* e = ext(S); if (!e || S.market.empty() || S.spb <= 0) return;
        const std::string f = off::flexDir(); if (f.empty()) return;
        e->sab.bind(S.market, (int)S.spb, f + "\\SharedAbsorb\\" + sab::fileName(S.market, (int)S.spb), f + "\\LRA-Badges-" + sab::canon(S.market) + ".csv");
        e->sab.poll(now);
        for (std::map<std::pair<int, long long>, sab::Rec>::const_iterator it = e->sab.st.byKey.begin(); it != e->sab.st.byKey.end(); ++it) {
            const sab::Rec& r = it->second; if (r.raw <= 0 || r.pullx >= 0) continue;
            std::map<std::pair<int, long long>, dt::GiveUp>::const_iterator g = e->dtG.find(std::make_pair(r.side, r.raw)); if (g == e->dtG.end()) continue;
            sab::Extra x; x.mkt = S.market; x.spb = (int)S.spb; x.key = r.key; x.side = r.side; x.by = "BF"; x.kv["pullx"] = sab::fmt(g->second.x, 2);
            if (e->sab.putX(x)) ++e->sabX;
        }
    }   // (1.19.0) W reads at touch + 60 s; the Delta Profile's TR lines
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
          << "\n" << (e ? e->sab.statusLine() + ",pull extras written " + std::to_string(e->sabX) + " (this pane reads the A; the T line A flag is recorded, not drawn)" : std::string("SHARED_A,n/a"))
          << "\nTURNS," << (e ? std::to_string(e->dtTurns.size()) + " decided, " + std::to_string(e->dtXWritten) + " exhaustion lines, " + std::to_string(e->dtPendX.size()) + " waiting, touches " + std::to_string(S.e.touches.size()) + ", " + std::to_string(e->dtWritten) + " written, " + std::to_string(e->dtFileFails) + " write fails, H " + std::to_string(e->dtH.size()) + " decided (" + std::to_string(e->dtHWritten) + " written, " + std::to_string(e->dtPendH.size()) + " waiting, " + std::to_string(e->dtHNoFeed) + " no feed), re-test " + std::to_string(e->dtQ.size()) + " decided (" + std::to_string(e->dtQWritten) + " written, " + std::to_string(e->dtQPend.size()) + " watching, " + std::to_string(e->dtQNoBook) + " no book, " + std::to_string(e->dtQOver) + " over), live " + (e->dtHaveLive ? "?" : "none") + ", file " + e->dtPath : std::string("n/a"))
          << "\nMOVES," << (e ? std::to_string(e->dtM.size()) + " reads (" + std::to_string(e->dtMWritten) + " written, " + std::to_string(e->mvNoFeed) + " candles without the order feed), seen modify " + std::to_string(S.e.mvt.seenM) + " / re-post " + std::to_string(S.e.mvt.seenR) + ", dropped following " + std::to_string(S.e.mvt.following) + " / ladder " + std::to_string(S.e.mvt.ladder) + " / short rest " + std::to_string(S.e.mvt.shortRest) + " / small " + std::to_string(S.e.mvt.smallSz) + " / ambiguous " + std::to_string(S.e.mvt.ambiguous) + ", confirmed " + std::to_string(S.e.mvt.confirmed) + ", min lots " + std::to_string(S.e.mvt.minLots) + ", normal candles " + std::to_string(e->mvNorm.n[0]) + ", P drawn only (B / attacker cases recorded)" : std::string("n/a"))
          << "\nGIVE_UP," << (e ? std::to_string(e->dtG.size()) + " decided at 5-candle extremes (" + std::to_string(e->dtGWritten) + " written, " + std::to_string(e->gUndecided) + " without the feed / < 20 s after the extreme), G drawn: pulled >= 2x normal net of moves, pulled > added, attack died, no initiative candle" : std::string("n/a"))
          << "\nTRAPS," << (e ? std::to_string(e->dtTr.size()) + " Delta Profile TR lines read (" + std::to_string(e->trBad) + " bad), file " + e->trPath : std::string("n/a"))
          << "\nAT_LEVEL," << (e ? std::to_string(e->lwLevels.size()) + " key levels watched, " + std::to_string(e->lw.touchesSeen) + " touches (" + std::to_string(e->lw.deduped) + " within 15 min), " + std::to_string(e->lw.open.size()) + " measuring, " + std::to_string(e->dtW.size()) + " decided (" + std::to_string(e->dtWWritten) + " written), W drawn at supports only" : std::string("n/a"))
          << "\nSTRIPS," << (e ? std::string(lst::VERSION) + ",drawn " + std::to_string(e->stripsDrawn) + ",skipped " + std::to_string(e->stripSkipped) + ",exported " + std::to_string(e->stripWritten) + ",export_fail " + std::to_string(e->stripFail) + ",queue Q read " + std::to_string(e->stripQRead) + " (" + std::to_string(e->stripQBad) + " bad) from " + e->stripQPath + ",file " + e->stripPath : std::string("n/a"))
          << "\nBADGES," << (e ? std::to_string(e->badgesDrawn) + " row draws, " + std::to_string(e->badgeDropped) + " badges with letters moved to the hover (max " + std::to_string(dt::BADGE_MAX) + "), hover no room " + std::to_string(e->badgeTipNoRoom) : std::string("n/a"))
          << "\nFACTS,SignalBoard " << sb::VERSION << ",D posted " << (e ? std::to_string(e->dPosted) : std::string("n/a"))
          << "\nF_MIRROR," << fmr::VERSION << "," << (e ? std::to_string(e->fReads.size()) + " pivot reads (" + std::to_string(e->fWritten) + " written here, " + std::to_string(e->fFails) + " write fails), F? armed " + std::to_string(e->fArmed) + ",thresholds " + (e->fPar.set() ? "front " + std::to_string(e->fPar.front).substr(0, 5) + " behind " + std::to_string(e->fPar.behind).substr(0, 5) : std::string("NOT SET (recorded, not drawn)")) + ",params " + e->fPar.why + ",file " + e->fPath : std::string("n/a"))
          << "\nRETEST_IDS," << (e ? std::to_string(e->rtInfo.size()) + " classified (same order / new big order / small only), file " + e->rtPath : std::string("n/a"))
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
        try { decideF(S, key); } catch (...) {}                            // (1.28.0) F? at the pivot-bar close, decided once
    }
    virtual void onAccClosed(S_& S, long long key, const off::Acc& a) { trackOn(S); Ext* e = ext(S); if (e) { fillAcc(e->dtc[key], a); e->dtc[key].t = key;
        // The aggregate record does not persist the cap / missing-aggressor flags.  Keep
        // them only for sidecar provenance while the host is alive; no old/reloaded record
        // is promoted to COMPLETE without this direct native coverage observation.
        try {
        e->evidenceNative[key] = (a.orderCap || a.aggrCap || a.aggrUnknown > 0) ? rse::PARTIAL : rse::COMPLETE;
        while (e->evidenceNative.size() > 5000) e->evidenceNative.erase(e->evidenceNative.begin());
        for (int side = 0; side < 2; ++side) { e->evidenceIce[side][key] = a.ice[side]; while (e->evidenceIce[side].size() > 64) e->evidenceIce[side].erase(e->evidenceIce[side].begin()); }
        } catch (...) { e->evidenceNative.erase(key); e->evidenceIce[0].erase(key); e->evidenceIce[1].erase(key); }
        e->mvByKey[key] = a.moves; while (e->mvByKey.size() > 60) e->mvByKey.erase(e->mvByKey.begin()); } }   // (1.17.0) the candle's confirmed moves
    // (1.17.0) the shared engine tracks order moves only for this pane (MoveLogic.h; size floor = the market's q90 of moves)
    void trackOn(S_& S) { S.e.trackMoves = true; S.e.mvt.minLots = mvl::minLotsOf(S.market);
        Ext* e = ext(S); if (e) { e->lwSink.w = &e->lw; e->lwSink.f = &e->fAcc; S.e.sink = &e->lwSink; } }   // (1.19.0) the key-level watch sees every classified book event
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
    // (1.28.0) F? (FMirror.h): params (re-read every 60 s), the F file (read back once per market / bar size), one read per pivot-bar close
    void fFiles(S_& S, Ext& e, long long nowMs) {
        const std::string f = off::flexDir(); if (f.empty() || S.market.empty() || S.spb <= 0) return;
        const std::string pp = f + "\\LRA-FMirror-" + S.market + ".csv";
        if (pp != e.fParPath || nowMs - e.fParMs >= 60000 || nowMs < e.fParMs) {
            e.fParPath = pp; e.fParMs = nowMs; fmr::Params P; std::ifstream in(pp.c_str(), std::ios::binary); int used = 0;
            if (in.is_open()) { std::string ln; int n = 0; while (std::getline(in, ln) && ++n < 200) if (fmr::parseParam(ln, P)) ++used; }
            P.why = in.is_open() ? "file (" + std::to_string(used) + " values)" : std::string("no file");
            e.fPar = P;
        }
        const std::string fp = f + "\\BookFlow\\" + S.market + "-" + std::to_string((long long)S.spb) + "-fmirror.csv";
        if (fp != e.fPath) {
            e.fPath = fp; e.fReads.clear(); off::makeDir(f + "\\BookFlow");
            std::ifstream in(fp.c_str(), std::ios::binary); std::string ln; int n = 0;
            while (in.is_open() && std::getline(in, ln) && ++n < 200000) { fmr::Read r; if (fmr::parse(ln, r) && !e.fReads.count(r.key)) e.fReads[r.key] = r; }
        }
    }
    void decideF(S_& S, long long key) {
        Ext* e = ext(S); if (!e || S.market.empty() || !(S.tick > 0)) return;
        fFiles(S, *e, off::steadyMs());
        if (e->fReads.count(key) || e->fPath.empty()) return;              // decided once (also across restarts)
        std::map<long long, int>::const_iterator bi = S.barEnds.find(key); if (bi == S.barEnds.end() || bi->second < fmr::EXT_BARS) return;
        RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
        const int i = bi->second; if (i >= times.count || (long long)times[i] != key) return;
        std::vector<fmr::Bar> B; const int from = std::max(0, i - 20);
        for (int j = from; j <= i; ++j) { fmr::Bar b; off::Tick o = 0, h = 0, l = 0, c = 0; b.key = (long long)times[j];
            b.ok = j < op.count && j < hi.count && j < lo.count && j < cl.count && off::toTick((double)op[j], S.tick, o) && off::toTick((double)hi[j], S.tick, h) && off::toTick((double)lo[j], S.tick, l) && off::toTick((double)cl[j], S.tick, c);
            b.o = o; b.h = h; b.l = l; b.c = c; B.push_back(b); }
        fmr::Read r; if (!fmr::decide(B, B.size() - 1, e->fAcc, S.market, e->fPar, r)) return;
        std::ofstream f(e->fPath.c_str(), std::ios::binary | std::ios::app);
        if (f.is_open()) { f << fmr::lineOf(r) << "\n"; f.flush(); if (f.good()) { e->fReads[key] = r; ++e->fWritten; if (r.armed == 1) ++e->fArmed; } else ++e->fFails; } else ++e->fFails;
        while (e->fReads.size() > 6000) e->fReads.erase(e->fReads.begin());
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
    // (1.28.0) the re-test hover: the resting orders in a zone by id - the big ones (>= the market's line floor, OrderLinesLogic) and the biggest
    BfZoneIds zoneIds(S_& S, int side, long long lo, long long hi) {
        BfZoneIds z; const int s = side == dt::HIGH ? off::ASK : off::BID; const long long floor = oln::ruleFor(S.market).floorLots;
        for (std::unordered_map<long long, off::Engine::Ord>::const_iterator it = S.e.book.begin(); it != S.e.book.end(); ++it) {
            const off::Engine::Ord& o = it->second; if (!o.known || o.side != s || o.size <= 0 || (long long)o.px < lo || (long long)o.px > hi) continue;
            ++z.n; if (o.size > z.maxLots) z.maxLots = o.size;
            if (o.size >= floor) { z.big.insert(it->first); if (o.size > z.bigLots) { z.bigLots = o.size; z.bigId = it->first; } }
        }
        return z;
    }
    void rtFile(S_& S, Ext& e) {
        const std::string f = off::flexDir(); if (f.empty() || S.market.empty() || S.spb <= 0) return;
        const std::string p = f + "\\BookFlow\\" + S.market + "-" + std::to_string((long long)S.spb) + "-retest-ids.csv";
        if (p == e.rtPath) return;
        e.rtPath = p; e.rtInfo.clear(); std::ifstream in(p.c_str(), std::ios::binary); std::string ln; int n = 0;
        while (in.is_open() && std::getline(in, ln) && ++n < 100000) {   // R|side|turn key|text
            if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (ln.size() < 6 || ln[0] != 'R' || ln[1] != '|') continue;
            const size_t a1 = ln.find('|', 2), a2 = a1 == std::string::npos ? a1 : ln.find('|', a1 + 1); if (a2 == std::string::npos) continue;
            const int sd = std::atoi(ln.substr(2, a1 - 2).c_str()); const long long t = std::atoll(ln.substr(a1 + 1, a2 - a1 - 1).c_str());
            if ((sd == dt::LOW || sd == dt::HIGH) && t > 0 && !e.rtInfo.count(std::make_pair(sd, t))) e.rtInfo[std::make_pair(sd, t)] = ln.substr(a2 + 1);
        }
    }
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
            const BfZoneIds zNow = zoneIds(S, r.side, a, b);                 // (1.28.0) the ids resting in the zone at this close
            if (!e->rtIdsAtTurn.count(it->first)) e->rtIdsAtTurn[it->first] = zNow.big;   // the big ids at the turn (first close after it)
            const BfZoneIds zBefore = e->rtSnap.count(it->first) ? e->rtSnap[it->first] : zNow;
            const int res = dt::retestStep(r, q, atr, restIn(S, r.side, a, b), normalOf(r.side));
            e->rtSnap[it->first] = zNow;                                   // the snapshot the next candle's decision uses (= restBefore)
            if (res == 0) { ++it; continue; }
            if (res == 1) {
                e->dtQ[it->first] = r;
                {   // (1.28.0) same order / new big order / small orders only, lots vs normal -> R line (read back after a restart)
                    const std::set<long long>& at = e->rtIdsAtTurn[it->first]; char tb[200];
                    if (zBefore.bigId && at.count(zBefore.bigId)) std::snprintf(tb, sizeof tb, "same order still there (%lld lots)", zBefore.bigLots);
                    else if (zBefore.bigId) std::snprintf(tb, sizeof tb, "new big order (%lld lots, %zu big ids)", zBefore.bigLots, zBefore.big.size());
                    else std::snprintf(tb, sizeof tb, "small orders only (%lld orders, biggest %lld lots)", zBefore.n, zBefore.maxLots);
                    char tv[96]; std::snprintf(tv, sizeof tv, "; %lld lots resting = %.1fx normal", r.rest, r.ratio);
                    const std::string info = std::string(tb) + tv; e->rtInfo[it->first] = info; rtFile(S, *e);
                    if (!e->rtPath.empty()) { std::ofstream rf(e->rtPath.c_str(), std::ios::binary | std::ios::app); if (rf.is_open()) rf << "R|" << it->first.first << "|" << it->first.second << "|" << info << "\n"; }
                }
                if (!e->dtPath.empty()) { std::ofstream f(e->dtPath.c_str(), std::ios::binary | std::ios::app); if (f.is_open()) { f << dt::qLineOf(r) << "\n"; f.flush(); if (f.good()) ++e->dtQWritten; else ++e->dtFileFails; } else ++e->dtFileFails; }
            } else if (res == -2) ++e->dtQNoBook; else ++e->dtQOver;
            e->rtSnap.erase(it->first); e->rtIdsAtTurn.erase(it->first);
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
        // (1.20.0, Rassul 19:48 CT) one block per letter (every letter on the candle, also the ones with no room), his hover format, the
        // numbers from the records; proof status in the hover (no "?" on the badge). n = how many of that letter this chart has decided.
        const bool proven = false;                                         // no Defense letter is proven yet (TURN-TEST / nightly); the badge had "?" on all
        std::vector<dt::HoverBlock> blocks;
        const std::vector<dt::BadgeLetter> letters = dt::badgeLetters(in);
        for (size_t k = 0; k < letters.size(); ++k) {
            const std::string& L = letters[k].s;
            if (L == "A") { for (int sd = 0; sd < 2; ++sd) { const sab::Rec* rr = e.sab.st.atRaw(sd, key); if (rr) { dt::HoverBlock b; b.letter = "A"; b.rows = sab::hover(*rr, e.sab.P); blocks.push_back(b); break; } } }   // (1.22.0) the shared A block
            else if (L == "H") { long long n = 0; const dt::Counter* c = nullptr;
                for (std::map<std::pair<int, long long>, dt::Counter>::const_iterator h = e.dtH.begin(); h != e.dtH.end(); ++h) if (h->second.H) { ++n; if (!c && h->second.at == key) c = &h->second; }
                if (c) blocks.push_back(dt::hoverH(*c, proven, n)); }
            else if (L == "Tr") { std::map<std::pair<int, long long>, trapl::Rec>::const_iterator r = e.dtTr.find(std::make_pair(in.tr, key));
                if (r != e.dtTr.end()) blocks.push_back(dt::hoverTr(r->second.top, r->second.lots, r->second.x, proven, (long long)e.dtTr.size())); }
            else if (L == "G") { long long n = 0; for (std::map<std::pair<int, long long>, dt::GiveUp>::const_iterator it = e.dtG.begin(); it != e.dtG.end(); ++it) n += it->second.G ? 1 : 0;
                std::map<std::pair<int, long long>, dt::GiveUp>::const_iterator g = e.dtG.find(std::make_pair(in.g, key));
                if (g != e.dtG.end()) blocks.push_back(dt::hoverG(g->second, proven, n)); }
            else if (L == "W") { long long n = 0; for (std::map<std::pair<long long, long long>, dt::WRead>::const_iterator it = e.dtW.begin(); it != e.dtW.end(); ++it) n += dt::wDrawn(it->second) ? 1 : 0;
                for (std::map<std::pair<long long, long long>, dt::WRead>::const_iterator w = e.dtW.lower_bound(std::make_pair(key, (long long)LLONG_MIN)); w != e.dtW.end() && w->first.first == key; ++w)
                    if (dt::wDrawn(w->second)) blocks.push_back(dt::hoverW(w->second, S.tick, proven, n)); }
            else if (L == "P" || L == "B") {
                const int book = letters[k].colourKey == dt::HIGH ? mvl::OFFERS : mvl::BIDS;   // P / B colour = the BOOK side that moved
                long long n = 0; for (std::map<std::pair<int, long long>, mvl::Read>::const_iterator it = e.dtM.begin(); it != e.dtM.end(); ++it) n += mvl::letterOf(it->second, book) == L[0] ? 1 : 0;
                for (int sd = 0; sd < 2; ++sd) { std::map<std::pair<int, long long>, mvl::Read>::const_iterator m = e.dtM.find(std::make_pair(sd, key)); if (m == e.dtM.end() || mvl::letterOf(m->second, book) != L[0]) continue;
                    const mvl::Read& R = m->second; const int def = R.ext == 0 ? mvl::BIDS : mvl::OFFERS; const bool in_ = L == "P";
                    const int cs = book == def ? (in_ ? mvl::C_DEF_IN : mvl::C_DEF_OUT) : (in_ ? mvl::C_ATT_IN : mvl::C_ATT_OUT);
                    blocks.push_back(dt::hoverMove(L[0], book, in_ ? R.s.in[book] : R.s.out[book], in_ ? R.s.ticksIn[book] : R.s.ticksOut[book], R.x[cs], proven, n)); break; } }
            else if (L == "L" || L == "X") { long long n = 0; const dt::Retest* q = nullptr;
                for (std::map<std::pair<int, long long>, dt::Retest>::const_iterator it = e.dtQ.begin(); it != e.dtQ.end(); ++it) if (it->second.res == L[0]) { ++n; if (!q && it->second.at == key) q = &it->second; }
                if (q) { std::map<std::pair<int, long long>, dt::Turn>::const_iterator t = e.dtTurns.find(std::make_pair(q->side, q->t));
                         blocks.push_back(dt::hoverQ(*q, t != e.dtTurns.end() ? (double)t->second.linePx * S.tick : 0.0, S.tick, proven, n)); } }
        }
        return dt::hoverText(badge, dropped, blocks);
    }
    // (1.20.0) the badge's hover box: measured like the info tip (first line bold, columns when the pane is short), placed by dt::tipSpot
    // below the badge, inside the pane and left of the price axis, never over the info box; nothing drawn when no spot is free (counted)
    struct TipLay { std::vector<std::string> lines; std::vector<int> w; std::vector<off::TipCol> cols; int boxW = 0, boxH = 0, pt = 9; };
    TipLay tipLayout(const std::string& text, int top, int bottom, int pt = 9) {
        TipLay T; { std::string cur; for (size_t k = 0; k < text.size(); ++k) { if (text[k] == '\n') { T.lines.push_back(cur); cur.clear(); } else cur += text[k]; } T.lines.push_back(cur); }
        FONT fb; fb.id = HELVETICA; fb.size = pt; fb.style = BOLD; FONT fp = fb; fp.style = PLAIN;
        T.w.assign(T.lines.size(), 0);
        for (size_t k = 0; k < T.lines.size(); ++k) { setFont(k == 0 ? fb : fp); T.w[k] = T.lines[k].empty() ? 0 : (int)getTextWidth(T.lines[k].c_str(), -1); }
        const int lh = pt + 4, pad = 6, gap = 18, rows = std::max(1, (bottom - 2 - top - 2 * pad) / lh);
        T.cols = off::tipColumns(T.lines, T.w, rows);
        int used = 0; T.boxW = 2 * pad; for (size_t c = 0; c < T.cols.size(); ++c) { T.boxW += T.cols[c].w + (c ? gap : 0); used = std::max(used, (int)(T.cols[c].to - T.cols[c].from)); }
        T.boxH = 2 * pad + used * lh; T.pt = pt; return T;
    }
    void drawBadgeTip(Ext& e, const off::PaneGeo& g, const off::IconRect& badge, const std::string& text, const bp::Box* box) {
        const int R = std::min(g.right, g.edge);
        dt::TipRect br; br.l = badge.l; br.t = badge.t; br.r = badge.r; br.b = badge.b;
        dt::TipRect bx; if (box) { bx.l = box->l; bx.t = box->t; bx.r = box->r; bx.b = box->b; }
        dt::TipRect at; std::string shown; int level = -1;
        for (int lv = 0; lv < 4 && level < 0; ++lv) {                      // fit cascade: all rows -> without "Use" -> without "Use" / "Status" -> that at 8 pt (lesson 54: decided here, tested)
            const std::string t = dt::hoverCompact(text, std::min(lv, 2)); const int pt = lv < 3 ? 9 : 8;
            if (dt::tipSpot(g.left, g.top, R, g.bottom, br, box ? &bx : nullptr, [&](int top, int& w, int& h) { const TipLay T = tipLayout(t, top, g.bottom, pt); w = T.boxW; h = T.boxH; }, at)) { level = lv; shown = t; }
        }
        if (level < 0) { ++e.badgeTipNoRoom; e.lastTip = dt::TipRect(); e.lastTipLevel = -1; return; }
        e.lastTipLevel = level; if (level > 0) ++e.badgeTipCompact;
        const TipLay T = tipLayout(shown, at.t, g.bottom, level < 3 ? 9 : 8);
        setPen(off::C_TIP_BORDER, 1, P_SOLID);
        off::fillRect(at.l, at.t, at.r, at.b, off::C_TIP_BORDER, off::C_TIP_BG, 1);
        setTextColor(off::C_TIP_TEXT);
        FONT fb; fb.id = HELVETICA; fb.size = T.pt; fb.style = BOLD; FONT fp = fb; fp.style = PLAIN;
        int cx = at.l + 6; const int lh = T.pt + 4;
        for (size_t c = 0; c < T.cols.size(); ++c) {
            int y = at.t + 6;
            for (size_t k = T.cols[c].from; k < T.cols[c].to; ++k, y += lh) {
                if (T.lines[k].empty()) continue;
                setFont(k == 0 ? fb : fp);
                RCT t; t.set(off::coord(cx), off::coord(y), off::coord(std::min(at.r - 2, cx + T.cols[c].w + 4)), off::coord(y + lh)); t.drawText(T.lines[k].c_str(), false, false);
            }
            cx += T.cols[c].w + 18;
        }
        e.lastTip = at;
    }
    // ==== (1.23.0, 2026-10-22 Rassul approved layout-stages-2strips.png 11:41-11:46 CT, build 15:20 CT) THE TWO LETTER STRIPS (LetterStrips.h):
    // "Holding" A > Q > W > P > B and "Turn & re-test" H > L > X, one letter per strip per closed candle in the colour of the side that acts,
    // no boxes / borders, hovers per strip; every letter appended to lsFlexLevels\Strips\DEF-<MKT>-<spb>.csv for lsReversalStage; the queue
    // meter's Q read from lsFlexLevels\Strips\QUEUE-<MKT>-<spb>.csv when that record exists (else no Q).
    // Typed sidecar helpers.  They inspect completed records only; they never participate in
    // finalization, renderNet, bar ratios, normalizers, scaling, or legacy strip priority.
    static long long evidenceLots(long long v) { return v >= 0 && v <= 1000000000000LL ? v : -1; }
    static long long evidencePace(long long v) { return v >= 0 && v <= 100000000 ? v : -1; }
    static double evidenceRatio(double v) { return std::isfinite(v) && v >= 0 && v <= 1000000000.0 ? v : -1; }
    static int evidenceQuality(const S_& S, const Ext& e, long long key) {
        typename std::map<long long, off::PaneBar>::const_iterator b = S.ser.bars.find(key);
        if (b != S.ser.bars.end() && b->second.ok) {
            if (b->second.r.partial) return rse::PARTIAL;
            typename std::map<long long, int>::const_iterator n = e.evidenceNative.find(key);
            // A reloaded record has no retained native cap / aggressor-ID coverage fields.
            return n != e.evidenceNative.end() ? n->second : rse::UNVERIFIED;
        }
        return S.accs.count(key) || S.closedAt.count(key) ? rse::PENDING : rse::UNAVAILABLE;
    }
    static int evidenceWindowQuality(const S_& S, const Ext& e, long long cause, long long decision) {
        if (cause <= 0 || decision < cause) return rse::UNVERIFIED;
        typename std::map<long long, int>::const_iterator p = S.barEnds.lower_bound(cause); bool any = false, partial = false, unverified = false;
        for (; p != S.barEnds.end() && p->first <= decision; ++p) {
            any = true; const int q = evidenceQuality(S, e, p->first);
            if (q == rse::PENDING || q == rse::UNAVAILABLE) return q;
            partial = partial || q == rse::PARTIAL;
            unverified = unverified || q == rse::UNVERIFIED;
        }
        return !any || unverified ? rse::UNVERIFIED : partial ? rse::PARTIAL : rse::COMPLETE;
    }
    static long long evidenceWDecisionKey(const S_& S, const dt::WRead& r) {
        // r.t0 is the native feed second.  Select the actual chart bar END containing
        // the known 60-second verdict horizon; no civil timestamp is synthesized.
        if (r.t0 <= 0 || r.t0 > LLONG_MAX - dt::W_SEC) return 0;
        typename std::map<long long, int>::const_iterator b = S.barEnds.lower_bound(r.t0 + dt::W_SEC);
        return b == S.barEnds.end() ? 0 : b->first;
    }
    static long long evidenceAbsPrice(const sab::Rec& r, double tick) {
        off::Tick p = 0; return tick > 0 && std::isfinite(r.extPx) && r.extPx > 0 && off::toTick(r.extPx, tick, p) ? (long long)p : LLONG_MIN;
    }
    void defEvidence(S_& S, Ext& e, long long legacyKey, const lst::Lt& l, long long published, std::vector<rse::Event>& out) {
        if (legacyKey <= 0 || published <= 0 || S.symbol.empty()) return;
        auto put = [&](long long decision, long long cause, int basis, int quality, long long price, long long lots, long long aux, long long pace, double ratio, const std::string& suffix) {
            if (decision <= 0) return;
            rse::Event x; x.key = decision; x.pane = lst::DEF; x.l = l; x.m.typed = true; x.m.basis = basis; x.m.quality = quality;
            x.m.cause = cause > 0 ? cause : 0; x.m.published = published; x.m.priceTicks = price; x.m.lots = lots; x.m.auxLots = aux; x.m.pace100 = pace; x.m.ratio = ratio;
            x.m.contract = S.symbol; x.m.id = rse::eventId(x.pane, x.key, x.l, x.m.cause) + suffix; out.push_back(x);
        };
        if (l.code == "A") {
            const sab::Rec* r = e.sab.st.atRaw(l.pivot, legacyKey);
            if (r) put(legacyKey, r->raw, r->book == 1 ? rse::MBO : rse::HYBRID, r->book == 1 ? evidenceQuality(S, e, legacyKey) : rse::COMPLETE, evidenceAbsPrice(*r, S.tick), evidenceLots(r->att), r->book == 1 ? evidenceLots(r->bookLots) : -1, evidencePace(r->refills), evidenceRatio(r->effort), "-A");
        } else if (l.code == "W") {
            for (std::map<std::pair<long long, long long>, dt::WRead>::const_iterator w = e.dtW.lower_bound(std::make_pair(legacyKey, (long long)LLONG_MIN)); w != e.dtW.end() && w->first.first == legacyKey; ++w) {
                const dt::WRead& r = w->second; const long long decision = evidenceWDecisionKey(S, r);
                if (dt::wDrawn(r) && r.side == l.pivot && decision > 0) { put(decision, r.key, rse::MBO, r.partial ? rse::PARTIAL : evidenceQuality(S, e, decision), r.px, evidenceLots(r.pulled), evidenceLots(r.added), evidencePace(r.filled), evidenceRatio(r.fillX), "-W-" + std::to_string(r.px) + "-" + std::to_string(r.t0)); break; }
            }
        } else if (l.code == "P" || l.code == "B") {
            std::map<std::pair<int, long long>, mvl::Read>::const_iterator m = e.dtM.find(std::make_pair(l.pivot, legacyKey));
            if (m != e.dtM.end() && (l.book == mvl::BIDS || l.book == mvl::OFFERS) && mvl::letterOf(m->second, l.book) == l.code[0]) {
                const bool in = l.code == "P"; const int def = m->second.ext == 0 ? mvl::BIDS : mvl::OFFERS;
                const int c = l.book == def ? (in ? mvl::C_DEF_IN : mvl::C_DEF_OUT) : (in ? mvl::C_ATT_IN : mvl::C_ATT_OUT);
                put(legacyKey, m->second.t, rse::MBO, evidenceQuality(S, e, legacyKey), m->second.px, evidenceLots(in ? m->second.s.in[l.book] : m->second.s.out[l.book]), -1, -1, evidenceRatio(m->second.x[c]), std::string("-") + l.code + "-" + std::to_string(l.book));
            }
        } else if (l.code == "H") {
            for (std::map<std::pair<int, long long>, dt::Counter>::const_iterator h = e.dtH.begin(); h != e.dtH.end(); ++h) if (h->second.H && h->second.at == legacyKey && h->second.side == l.pivot) {
                const dt::Counter& r = h->second; std::map<std::pair<int, long long>, dt::Turn>::const_iterator t = e.dtTurns.find(std::make_pair(r.side, r.t));
                const long long line = t != e.dtTurns.end() && t->second.linePx > 0 ? t->second.linePx : LLONG_MIN;
                put(r.at, r.t, rse::MBO, evidenceWindowQuality(S, e, r.t, r.at), line, evidenceLots(r.lots), evidenceLots(r.attack), -1, r.attack > 0 ? evidenceRatio((double)r.lots / (double)r.attack) : -1, "-H");
                if (!out.empty() && t != e.dtTurns.end()) out.back().m.parentCause = t->second.pivotT;
                break;
            }
        } else if (l.code == "L" || l.code == "X") {
            for (std::map<std::pair<int, long long>, dt::Retest>::const_iterator q = e.dtQ.begin(); q != e.dtQ.end(); ++q) if (q->second.at == legacyKey && q->second.res == l.code[0] && q->second.side == l.pivot) {
                const dt::Retest& r = q->second; long long line = r.linePx;
                std::map<std::pair<int, long long>, dt::Turn>::const_iterator t = e.dtTurns.find(std::make_pair(r.side, r.t)); if (line <= 0 && t != e.dtTurns.end()) line = t->second.linePx;
                long long attack = -1;
                const int resting = r.side == dt::HIGH ? off::ASK : off::BID;
                const auto bar = e.evidenceIce[resting].find(r.at);
                if (bar != e.evidenceIce[resting].end() && evidenceQuality(S, e, r.at) == rse::COMPLETE) {
                    const auto at = bar->second.find(line);
                    if (at != bar->second.end() && !at->second.unknown) attack = evidenceLots(at->second.traded);
                }
                put(r.at, r.t, rse::MBO, evidenceWindowQuality(S, e, r.t, r.at), line > 0 ? line : LLONG_MIN, evidenceLots(r.rest), attack, -1, evidenceRatio(r.ratio), std::string("-") + l.code + "-" + std::to_string(r.t)); break;
            }
        }
    }
    // the Defense letters of one candle (raw key) from this pane's records, with their pivot, book side and key numbers
    void defLetters(S_& S, Ext& e, long long key, std::vector<lst::Lt>& v) {
        (void)S; v.clear(); char b[96];
        for (int sd = 0; sd < 2; ++sd) { const sab::Rec* rr = e.sab.st.atRaw(sd, key); if (rr) { std::snprintf(b, sizeof b, "%lld lots %.1fx", rr->att, rr->effort); v.push_back(lst::make(lst::DEF, "A", sd, -1, b)); break; } }
        std::map<long long, std::vector<lst::Lt> >::const_iterator q = e.stripQ.find(key);
        if (q != e.stripQ.end()) for (size_t k = 0; k < q->second.size(); ++k) {
            lst::Lt value = q->second[k];
            if (value.nums.compare(0, 5, "broke") == 0 || value.nums.compare(0, 7, "drained") == 0) value.code = "Q!";
            v.push_back(value);
        }
        for (std::map<std::pair<long long, long long>, dt::WRead>::const_iterator w = e.dtW.lower_bound(std::make_pair(key, (long long)LLONG_MIN)); w != e.dtW.end() && w->first.first == key; ++w)
            if (dt::wDrawn(w->second)) { const long long net = std::max<long long>(0, w->second.pulled - w->second.added); std::snprintf(b, sizeof b, "%s %lld lots pulled", w->second.name.c_str(), net); v.push_back(lst::make(lst::DEF, "W", w->second.side, -1, b)); break; }
        for (int sd = 0; sd < 2; ++sd) { std::map<std::pair<int, long long>, mvl::Read>::const_iterator m = e.dtM.find(std::make_pair(sd, key)); if (m == e.dtM.end()) continue;
            for (int book = 0; book < 2; ++book) { const char L = mvl::letterOf(m->second, book); if (!L) continue;
                const bool in_ = L == 'P'; std::snprintf(b, sizeof b, "%s %lld lots %lldt", book == 0 ? "bids" : "offers", in_ ? m->second.s.in[book] : m->second.s.out[book], in_ ? m->second.s.ticksIn[book] : m->second.s.ticksOut[book]);
                v.push_back(lst::make(lst::DEF, std::string(1, L), m->second.ext == 0 ? lst::LOW : lst::HIGH, book, b)); } }
        for (std::map<std::pair<int, long long>, dt::Counter>::const_iterator h = e.dtH.begin(); h != e.dtH.end(); ++h) if (h->second.H && h->second.at == key) {
            std::snprintf(b, sizeof b, "%lld lots %.1fx", h->second.lots, h->second.attack > 0 ? (double)h->second.lots / (double)h->second.attack : 0.0); v.push_back(lst::make(lst::DEF, "H", h->second.side == dt::HIGH ? lst::HIGH : lst::LOW, -1, b)); break; }
        for (std::map<std::pair<int, long long>, dt::Retest>::const_iterator r = e.dtQ.begin(); r != e.dtQ.end(); ++r) if (r->second.at == key && (r->second.res == 'L' || r->second.res == 'X')) {
            std::snprintf(b, sizeof b, "%lld lots %.1fx", r->second.rest, r->second.ratio); lst::Lt lt = lst::make(lst::DEF, std::string(1, r->second.res), r->second.side == dt::HIGH ? lst::HIGH : lst::LOW, -1, b);
            std::map<std::pair<int, long long>, std::string>::const_iterator ri = e.rtInfo.find(r->first); if (ri != e.rtInfo.end()) lt.nums += " - " + ri->second;   // (1.28.0) same / new big / small only
            v.push_back(lt); break; }
        {   // (1.28.0) F?: the pivot likely dies at once (drawn only with the market's thresholds)
            std::map<long long, fmr::Read>::const_iterator fi = e.fReads.find(key);
            if (fi != e.fReads.end() && fi->second.armed == 1 && e.fPar.show) { std::snprintf(b, sizeof b, "front %.1f behind %.1f floors", fi->second.front, fi->second.behind); v.push_back(lst::make(lst::DEF, "F?", fi->second.side == fmr::HIGH ? lst::HIGH : lst::LOW, -1, b)); }
        }
    }
    // every 3 s: read the queue meter's new Q lines; append the letters not yet in the DEF record (first line per candle + letter + pivot wins)
    void strips(S_& S, long long now) {
        Ext* e = ext(S); if (!e || S.market.empty() || S.spb <= 0) return;
        if (e->stripMs >= 0 && now >= e->stripMs && now - e->stripMs < 3000) return;
        e->stripMs = now;
        const std::string f = off::flexDir(); if (f.empty()) return;
        const std::string d = f + "\\Strips", p = d + "\\" + lst::fileName(lst::DEF, S.market, (int)S.spb), qp = d + "\\" + lst::fileName(lst::QUE, S.market, (int)S.spb);
        if (p != e->stripPath) { e->stripPath = p; e->stripIds.clear(); e->stripQ.clear(); e->stripQOff = 0; off::makeDir(d);
            long long o0 = 0; stripTail(p, o0, [&](const std::string& ln) { lst::Rec r; if (lst::parse(ln, r) && r.pane == lst::DEF) e->stripIds.insert(lst::idOf(lst::DEF, r.key, r.l.code, r.l.pivot)); }, 4LL * 1024 * 1024); }
        e->stripQPath = qp;
        stripTail(qp, e->stripQOff, [&](const std::string& ln) { lst::Rec r; if (!lst::parse(ln, r) || r.pane != lst::QUE) { ++e->stripQBad; return; }
            std::vector<lst::Lt>& v = e->stripQ[r.key]; for (size_t k = 0; k < v.size(); ++k) if (v[k].pivot == r.l.pivot) return; v.push_back(r.l); ++e->stripQRead; }, 4LL * 1024 * 1024);
        while (e->stripQ.size() > 4000) e->stripQ.erase(e->stripQ.begin());
        std::set<long long> keys;
        for (std::map<std::pair<int, long long>, sab::Rec>::const_iterator it = e->sab.st.byKey.begin(); it != e->sab.st.byKey.end(); ++it) if (it->second.raw > 0) keys.insert(it->second.raw);
        for (std::map<std::pair<long long, long long>, dt::WRead>::const_iterator it = e->dtW.begin(); it != e->dtW.end(); ++it) keys.insert(it->first.first);
        for (std::map<std::pair<int, long long>, mvl::Read>::const_iterator it = e->dtM.begin(); it != e->dtM.end(); ++it) keys.insert(it->first.second);
        for (std::map<std::pair<int, long long>, dt::Counter>::const_iterator it = e->dtH.begin(); it != e->dtH.end(); ++it) if (it->second.H) keys.insert(it->second.at);
        for (std::map<std::pair<int, long long>, dt::Retest>::const_iterator it = e->dtQ.begin(); it != e->dtQ.end(); ++it) if (it->second.at > 0) keys.insert(it->second.at);
        for (std::map<long long, fmr::Read>::const_iterator it = e->fReads.begin(); it != e->fReads.end(); ++it) if (it->second.armed == 1) keys.insert(it->first);   // (1.28.0)
        const long long published = (long long)currentDate();
        std::string out; std::vector<std::string> ids; std::vector<rse::Event> evidence; std::vector<rse::Frame> frames;
        if (published > 0 && !S.symbol.empty()) {
            RTARRAYI times(barDateTime); const int n = (int)times.count;
            for (int i = std::max(0, n - 601); i <= n - 2; ++i) { const long long key = (long long)times[i]; if (key > 0) { rse::Frame f0; f0.key = key; f0.pane = lst::DEF; f0.quality = evidenceQuality(S, *e, key); f0.contract = S.symbol; f0.published = published; frames.push_back(f0); } }
        }
        for (std::set<long long>::const_iterator k = keys.begin(); k != keys.end(); ++k) {
            if (*k <= 0) continue;
            std::vector<lst::Lt> v; defLetters(S, *e, *k, v);
            if (published > 0) for (size_t q = 0; q < v.size(); ++q) if (v[q].code != "Q") defEvidence(S, *e, *k, v[q], published, evidence);
            for (size_t q = 0; q < v.size(); ++q) { if (v[q].code == "Q") continue;   // Q is the queue meter's own record
                const std::string id = lst::idOf(lst::DEF, *k, v[q].code, v[q].pivot); if (e->stripIds.count(id) || std::find(ids.begin(), ids.end(), id) != ids.end()) continue;
                out += lst::line(lst::DEF, *k, v[q]) + "\n"; ids.push_back(id); }
        }
        if (!out.empty()) {
            std::ofstream o(p.c_str(), std::ios::binary | std::ios::app); if (o) { o.write(out.data(), (std::streamsize)out.size()); o.flush(); }
            if (o) { for (size_t k = 0; k < ids.size(); ++k) e->stripIds.insert(ids[k]); e->stripWritten += (long long)ids.size(); } else ++e->stripFail;
        }
        if (published > 0 && (!evidence.empty() || !frames.empty())) {
            const std::string ep = d + "\\" + rse::fileName(lst::DEF, S.market, S.symbol, (int)S.spb); e->evidencePath = ep;
            if (e->evidence.append(ep, evidence, frames)) e->evidenceWritten += (long long)evidence.size(); else ++e->evidenceFail;
        }
        while (e->stripIds.size() > 20000) e->stripIds.erase(e->stripIds.begin());
    }
    // the new whole lines of path from off0 (bounded; a shorter file = read again from the start)
    template <class Fn> static void stripTail(const std::string& path, long long& off0, Fn fn, long long maxRead) {
        std::ifstream in(path.c_str(), std::ios::binary); if (!in.is_open()) return;
        in.seekg(0, std::ios::end); const long long size = (long long)in.tellg(); if (size < 0) return;
        if (size < off0) off0 = 0; if (size == off0) return;
        if (size - off0 > maxRead) off0 = size - maxRead;
        in.seekg(off0, std::ios::beg); std::string buf((size_t)(size - off0), '\0'); in.read(&buf[0], (std::streamsize)buf.size());
        if (!in && in.gcount() != (std::streamsize)buf.size()) return;
        const size_t last = buf.rfind('\n'); if (last == std::string::npos) return;
        off0 += (long long)last + 1;
        size_t a = 0;
        while (a <= last) { const size_t b = buf.find('\n', a); std::string ln = buf.substr(a, b - a); a = b + 1; if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (!ln.empty()) fn(ln); }
    }
    // the hover of one strip on one candle: the strip header, its question rows, the letters' blocks (strongest first)
    std::string stripTipText(S_& S, Ext& e, long long key, int strip, const std::vector<lst::Lt>& v, const lst::Pick& pk) {
        dt::BadgeIn in; in.defSide = dt::LOW; std::vector<std::string> others; std::vector<std::string> extra;
        for (size_t q = 0; q < pk.order[strip].size(); ++q) {
            const lst::Lt& l = v[(size_t)pk.order[strip][q]];
            if (q > 0) others.push_back(l.code);
            if (l.code == "A") { in.A = true; in.defSide = l.pivot; } else if (l.code == "H") { in.H = true; in.defSide = l.pivot; }
            else if (l.code == "L" || l.code == "X") { in.Q = l.code[0]; in.defSide = l.pivot; } else if (l.code == "W") in.w = true;
            else if (l.code == "P" || l.code == "B") {                       // each move letter its own block (both pivots' reads on one candle may give P and B)
                const int book = l.book == 1 ? mvl::OFFERS : mvl::BIDS; long long n = 0;
                for (std::map<std::pair<int, long long>, mvl::Read>::const_iterator it = e.dtM.begin(); it != e.dtM.end(); ++it) n += mvl::letterOf(it->second, book) == l.code[0] ? 1 : 0;
                std::map<std::pair<int, long long>, mvl::Read>::const_iterator m = e.dtM.find(std::make_pair(l.pivot, key));
                if (m != e.dtM.end() && mvl::letterOf(m->second, book) == l.code[0]) {
                    const mvl::Read& R = m->second; const int def = R.ext == 0 ? mvl::BIDS : mvl::OFFERS; const bool in_ = l.code == "P";
                    const int cs = book == def ? (in_ ? mvl::C_DEF_IN : mvl::C_DEF_OUT) : (in_ ? mvl::C_ATT_IN : mvl::C_ATT_OUT);
                    const dt::HoverBlock hb = dt::hoverMove(l.code[0], book, in_ ? R.s.in[book] : R.s.out[book], in_ ? R.s.ticksIn[book] : R.s.ticksOut[book], R.x[cs], false, n);
                    std::string blk; for (size_t r = 0; r < hb.rows.size(); ++r) blk += (r ? "\n" : "") + dt::wrapRow(hb.rows[r]); extra.push_back(blk); } }
            else if (l.code == "Q") extra.push_back("Q  Queue - is the queue at the defenders' price holding (refilling faster than it is eaten)?\n   " + (l.nums.empty() ? std::string("queue meter read") : (l.nums.rfind(" tk") != std::string::npos ? l.nums.substr(0, l.nums.rfind(" tk")) : l.nums)) + "\n   Use: the queue is still there - the hold has fuel\n   Status: still learning (the queue meter's record)");
        }
        const std::string full = badgeTipText(S, e, key, in, std::string(), std::vector<std::string>());
        std::vector<std::string> parts; { size_t a = 0; for (;;) { const size_t b = full.find("\n\n", a); parts.push_back(full.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 2; } }
        std::vector<std::string> blocks; for (size_t k = 1; k < parts.size(); ++k) if (!parts[k].empty()) blocks.push_back(parts[k]);
        for (size_t k = 0; k < extra.size(); ++k) blocks.push_back(extra[k]);
        auto codeOf = [](const std::string& blk) { const size_t sp = blk.find(' '); return blk.substr(0, sp); };
        std::stable_sort(blocks.begin(), blocks.end(), [&](const std::string& a, const std::string& b) { return lst::rankOf(lst::DEF, codeOf(a)) < lst::rankOf(lst::DEF, codeOf(b)); });
        struct tm x; std::memset(&x, 0, sizeof(x));
        const std::string when = key > 0 && getLocaltime((RTDATE)key, &x) ? off::clock12(x.tm_hour, x.tm_min) : std::string();
        std::string t = lst::header(lst::DEF, strip, v[(size_t)pk.shown[strip]].code, when, others);
        const std::vector<std::string> qr = lst::stripQuestion(lst::DEF, strip);
        t += "\n"; for (size_t k = 0; k < qr.size(); ++k) t += "\n" + dt::wrapRow(qr[k]);
        for (size_t k = 0; k < blocks.size(); ++k) t += "\n\n" + blocks[k];
        return t;
    }
    // (1.28.0) the Defense pane's display names: A -> Ab (absorbing), H -> Hb? (hit back), L / X -> L? / X? (re-test); the records keep A / H / L / X
    static std::string dispCode(const std::string& c) { if (c == "A") return "Ab"; if (c == "H") return "Hb?"; if (c == "L") return "L?"; if (c == "X") return "X?"; return c; }
    // (1.28.0) colour by MEANING (green bullish / red bearish): the defenders' reads in their colour (a low = green); warnings that hurt them
    // (W, X, Q!) and B (the book stepping back) in the opposite colour; Ab grey (not a counter-attack yet); F? amber (skip / wait)
    template <class ColOf> static COLOR meaningCol(const lst::Lt& l, ColOf colOf) {
        if (l.code == "A") return (COLOR)0x009CA3AF;
        if (l.code == "F?") return (COLOR)0x00F59E0B;
        if (l.code == "W" || l.code == "X" || l.code == "Q!" || l.code == "B") return colOf(l.colour == lst::GREEN ? lst::RED : lst::GREEN);
        return colOf(l.colour);
    }
    template <class XOf>
    void badgeRow(S_& S, Ext& e, const off::PaneGeo& g, int v0, int v1, int last, int slot, RTARRAYI& times, XOf xOf, const bp::Box* box) {
        (void)slot;
        e.badgeHit.clear();
        const int y0 = g.top + g.reserve + 1;
        if (y0 + lst::STRIPS * lst::ROW_H + 10 > g.bottom) { e.badgeTip = -1; return; }   // pane too short
        struct C { long long key; int xc; std::vector<lst::Lt> v; lst::Pick pk; };
        std::vector<C> cs;
        for (int i = std::max(0, v0); i <= v1 && i < last; ++i) {
            const long long key = (long long)times[i];
            const int xc = xOf(i); if (xc < g.left || xc > g.edge) continue;
            C c; c.key = key; c.xc = xc; defLetters(S, e, key, c.v);
            {   // (1.28.0) Hb? only after an Ab (the shared A) at the same pivot within 10 candles; display only (records unchanged)
                for (size_t k = 0; k < c.v.size();) {
                    if (c.v[k].code != "H") { ++k; continue; }
                    bool ab = false; int n = 0; const int sd = c.v[k].pivot == lst::HIGH ? 1 : 0;
                    for (std::map<long long, off::PaneBar>::const_iterator bi = S.ser.bars.find(key); n <= 10; ++n) {
                        if (bi == S.ser.bars.end()) break;
                        if (e.sab.st.atRaw(sd, bi->first)) { ab = true; break; }
                        if (bi == S.ser.bars.begin()) break; --bi;
                    }
                    if (!ab) c.v.erase(c.v.begin() + (long)k); else ++k;
                }
            }
            if (c.v.empty()) continue;
            c.pk = lst::pick(lst::DEF, c.v); cs.push_back(c);
        }
        off::fillFlat(this, g.left, y0, g.edge, y0 + lst::STRIPS * lst::ROW_H, (COLOR)0x000B0F15);
        off::fillFlat(this, g.left, y0 + lst::STRIPS * lst::ROW_H, g.edge, y0 + lst::STRIPS * lst::ROW_H, (COLOR)0x00374151);
        FONT f; f.id = HELVETICA; f.size = 8; f.style = BOLD;
        auto colOf = [&](int c) { return (COLOR)(c == lst::GREEN ? e.dlSet.c(dlset::K_COL_LOW) : e.dlSet.c(dlset::K_COL_HIGH)); };
        for (int s = 0; s < lst::STRIPS; ++s) {
            const int yr = y0 + s * lst::ROW_H;
            FONT fl = f; fl.style = PLAIN; setFont(fl); setTextColor((COLOR)0x009CA3AF);
            { RCT rc; rc.set(off::coord(g.left + 3), off::coord(yr), off::coord(g.left + lst::LABEL_W), off::coord(yr + lst::ROW_H)); rc.drawText(lst::stripName(lst::DEF, s), false, false); }
            setFont(f);
            std::vector<lst::Item> items; std::vector<size_t> idx;
            for (size_t k = 0; k < cs.size(); ++k) { if (cs[k].pk.shown[s] < 0) continue; lst::Item it; it.xc = cs[k].xc; it.w = (int)getTextWidth(dispCode(cs[k].v[(size_t)cs[k].pk.shown[s]].code).c_str(), -1) + 1; items.push_back(it); idx.push_back(k); }
            const std::vector<lst::Placed> pl = lst::placeStrip(items, g.left + lst::LABEL_W, g.edge);
            for (size_t q = 0; q < pl.size(); ++q) {
                if (!pl[q].drawn) { ++e.stripSkipped; continue; }
                const C& c = cs[idx[q]]; const lst::Lt& l = c.v[(size_t)c.pk.shown[s]];
                const int t = yr + pl[q].sub * lst::SUB_H, b = t + lst::SUB_H;
                if (box && pl[q].l < box->r && pl[q].r > box->l && t < box->b && b > box->t) { ++e.stripSkipped; continue; }   // never over the box
                setTextColor(meaningCol(l, colOf));                          // (1.28.0) colour by meaning, Ab grey, F? amber
                const std::string shown = dispCode(l.code);
                RCT rc; rc.set(off::coord(pl[q].l), off::coord(t), off::coord(pl[q].r + 1), off::coord(b)); rc.drawText(shown.c_str(), false, false);
                std::string tip = stripTipText(S, e, c.key, s, c.v, c.pk);
                for (const lst::Lt& x : c.v) {                                // (1.28.0) the new reads' hovers in his format ("   Use:" rows drop first when space is short)
                    if (lst::stripOf(lst::DEF, x.code) != s) continue;           // only this strip's letters
                    if (x.code == "F?") { std::map<long long, fmr::Read>::const_iterator fi = e.fReads.find(c.key); if (fi != e.fReads.end()) {
                        const std::string h = fmr::hover(fi->second, e.fPar); std::vector<std::string> L; size_t a0 = 0; for (;;) { const size_t b0 = h.find('\n', a0); L.push_back(h.substr(a0, b0 == std::string::npos ? std::string::npos : b0 - a0)); if (b0 == std::string::npos) break; a0 = b0 + 1; }
                        tip += "\n\nF?  Pivot likely dies at once (amber)";
                        for (size_t k = 0; k < L.size(); ++k) tip += "\n" + dt::wrapRow(std::string(k >= 2 ? "   Use: " : "   ") + L[k]); } }
                    if (x.code == "L" || x.code == "X") { tip += "\n\n" + dt::wrapRow(std::string(x.code == "L" ? "L?  Re-test reloaded: " : "X?  Re-test empty: ") + x.nums);
                        tip += "\n" + dt::wrapRow("   Use: L? = the entry zone is stocked (same order / new big order); X? = only small orders or nothing - skip the entry there. Example: ES 10-22 1:59 PM L? 210 lots at 7,682 (2.7x), 2:01 PM X? small orders only, then the break."); }
                    if (x.code == "A") tip += "\n" + dt::wrapRow("   Use: Ab (grey) = absorbing - the attack is taken at the level, no counter-attack yet; Hb? follows only when the defenders hit back (>= 1.25x, price off the level).");
                }
                off::IconRect ir; ir.l = pl[q].l; ir.t = t; ir.r = pl[q].r; ir.b = b; e.badgeHit.push_back(std::make_pair(ir, tip));
                ++e.stripsDrawn;
            }
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
            if (!e.chart.empty() && !e.identity.empty() && !e.mstKv.empty()) mst::clear("BOOK", symOf(e.identity), spbOf(e.identity), e.chart); if (!e.chart.empty()) e.self.clear(symOf(e.identity), spbOf(e.identity), e.chart);
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
    // (1.29.0) SELF-AWARENESS (SelfAware.h, SELF-AWARE.md): a decided candle with a defense spike is a signal - bids arriving / offers
    // leaving = up, bids leaving / offers arriving = down; HIT = 1 ATR that way before 1 ATR against (later bars only). Value = the
    // rebuilt % (lots re-added at the hit prices). Display unchanged (SWITCH OFF per market).
    void selfAware(S_& S, Ext& e, long long now) {
        if (S.symbol.empty() || S.market.empty() || e.chart.empty() || e.mstBar == 0 || e.mstCt <= 0) return;
        if (!lsperf::smartAllowed(LSPERF_PLUGIN, S.symbol)) return;                    // PerfGuard: smart features OFF while slow
        std::map<long long, int>::const_iterator bi = S.barEnds.find(e.mstBar); if (bi == S.barEnds.end()) return;
        const int k = bi->second; RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
        if (k < 0 || op.count <= k || hi.count <= k || lo.count <= k || cl.count <= k) return;
        const sa::Bar b{ e.mstCt, op[k], hi[k], lo[k], cl[k] };
        if (!std::isfinite(b.o) || !std::isfinite(b.h) || !std::isfinite(b.l) || !std::isfinite(b.c)) return;
        static const sa::Spec SP = [] { sa::Spec s; s.valueKey = "rebuilt_pct"; s.sideKey = "spike";
            s.sideMap = { { "bid_arriving", 1 }, { "ask_leaving", 1 }, { "bid_leaving", -1 }, { "ask_arriving", -1 } };
            s.why = { "spike", "bid_net", "ask_net", "bid_net_x", "ask_net_x", "rebuilt_pct" }; return s; }();
        e.self.ind = "DEF";
        sa::fromFacts(e.self, SP, S.symbol, S.spb, e.chart, S.market, now, b, e.mstKv);
    }
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
        selfAware(S, *e, now);   // (1.29.0) self-awareness
        postD(S, *e, now);       // (1.31.0)
    }
    // (1.31.0) SignalBoard 1.4.0 kind D: every decided candle's defense (kept 200), posted with the steady clock beside SRC BOOK
    void postD(S_& S, Ext& e, long long) {
        const bf::DFact d = bf::defenseFact(e.mstCt, e.mstKv);
        if (d.ok) { sb::Fact f; f.bar = d.bar; f.lo = d.lo; f.hi = d.hi; f.side = d.side; f.val = d.val; f.code = d.code; f.state = 1; f.proven = 2; e.dFacts[d.bar] = f; }
        while (e.dFacts.size() > sb::FACT_CAP) e.dFacts.erase(e.dFacts.begin());
        if (e.dFacts.empty()) return;
        std::vector<sb::Fact> v; v.reserve(e.dFacts.size()); for (const auto& kv : e.dFacts) v.push_back(kv.second);
        const long long ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        sb::postFacts('D', S.symbol, S.spb, e.chart, ms, v); e.dPostMs = ms; e.dPosted = v.size();
    }
    void release() {                                                       // the Market State entry goes with the indicator
        S_* S = testState();
        if (S) { Ext* e = static_cast<Ext*>(S->ext.get()); if (e && !e->chart.empty() && !S->symbol.empty()) mst::clear("BOOK", S->symbol, S->spb, e->chart); if (e) e->self.clear(S->symbol, S->spb, e->chart); }
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
    virtual int draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
        {   S_* S0 = testState();                                          // the chart label (Market State key), read in the draw as BigBidAsk
            if (S0) { Ext* e = ext(*S0); const char* cl = getChartLabel(); const std::string c = cl ? cl : "";
                      if (e && !c.empty() && c != e->chart) { if (!e->chart.empty() && !S0->symbol.empty()) mst::clear("BOOK", S0->symbol, S0->spb, e->chart); e->self.clear(S0->symbol, S0->spb, e->chart); e->chart = c; e->mstMs = -1; } } }
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
        if (e->badgeTip >= 0) { e->badgeTip = -1; ch = true; }             // (1.20.0) the badge hover only while the cursor is inside this pane
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
        const int stripRes = g.reserve;                                    // (1.23.0) the two letter strips start here; the net bars below them
        g.reserve += topReserve();
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
        if (false) {   // (1.21.0, Rassul 20:11-20:23 CT "no") REMOVED - G moved to the attacker pane, its numbers in the attacker hover. (1.19.0, Top Attacker #2) the ATTACKERS row: the latest G decision (at a 5-candle high / low) within 12 candles of the shown candle, in
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
        {   // (1.24.0, IMPLIED FILLS option B) "Implied: 23% of this candle's volume" - the order feed's side "?" fills of the candle shown
            off::InfoRow ir; if (mbar && off::impliedRow(S.e.ip, mbar->r.fillAll, mbar->r.fillImpl, ir)) info.rows.push_back(ir);
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
            { off::PaneGeo gs = g; gs.reserve = stripRes; (void)gs; e->badgeHit.clear(); e->badgeTip = -1;   /* (1.29.0) the letter strips are not drawn (still decided + exported) */ }   // (1.23.0) the two letter strips
        }
        const COLOR vcol[2] = { 0x0022C55E, 0x00EF4444 };                 // values: bids (buyers) green, offers (sellers) red
        off::drawPanel(this, P, info, vcol, off::implOf(zl), false);
        br::draw(this, P, info, rp, rd);                                  // (1.4.0) the Read word + lines over its rows
        { Ext* ex = ext(S); off::IconRect tmp; if (P.ok) off::drawQuestionIn(this, P.box, bf::QUESTION, ex ? &ex->infoIcon : &tmp); else if (ex) ex->infoIcon = off::IconRect();   // (1.15.0) inside the box, upper left   // (1.8.0) the question; (1.10.0) + the info icon
          if (ex && ex->infoTipOn) off::drawInfoTip(this, g, ex->infoIcon, bf::INFO_TIP);   // (1.11.0) the description box while the mouse is on the icon
          if (ex) ex->lastTip = dt::TipRect();
          if (ex && !ex->infoTipOn && ex->badgeTip >= 0 && ex->badgeTip < (int)ex->badgeHit.size())   // (1.19.0) the badge's hover; (1.20.0) per letter, never over the box
              drawBadgeTip(*ex, g, ex->badgeHit[(size_t)ex->badgeTip].first, ex->badgeHit[(size_t)ex->badgeTip].second, P.ok ? &P.box : nullptr); }
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
int cppExtension::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<BookFlow*>(this)->bfCalc(); return RTX_OK; }   // (1.3.4)

extern "C" cppExtension* CreateExtension(void) {
    BookFlow* p = new BookFlow();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE);                   // its own pane, draws itself; mouse events even when not selected
    p->setExtendedFlags(CALL_CONTINUOUSLY);                            // the order feed keeps coming while no trade prints
    p->setDescription("LRA Book Flow - how the resting side DEFENDS near price (market-by-order feed). Green = bids (buyers below price), red = offers (sellers above price). TWO MEASURES: 1) REFILL EFFORT - Question: how much defense is there compared with normal? How: lots ADDED minus lots PULLED near price this candle, per side, compared with that side's average of the last 20 candles; it is the bar and the first box row. Example: offers +2,587 (4.8x) = sellers put back 2,587 more offers than they pulled, 4.8 times their usual. How to read: 1.0x = normal; 2.0x or more = heavy defense (the bar turns bright); a bar below the zero line = that side pulled more than it added. 2) REFILLED - Question: did they put back what was taken, compared with normal for this market? How: lots re-posted at the exact prices that were just hit, within 1 second, divided by the lots eaten there, divided by that market's usual level; it is the second box row and the grey R label on bright bars. Example: buyers took 2,010 offers, sellers re-posted 8,624 at those prices = 4.3 times what was taken; ES normal is 2.2 times, so Refilled = 2.0x STRONG. How to read: 1.0x = the usual refill; STRONG = the market's top quarter (ES about 1.25x and up) = defense holding; weak = its bottom quarter (ES about 0.75x and down) = defense failing. READ (under the box) - Question: is the defense holding? At a new 10-bar low or high: C+R ? = a busy candle (2x volume) + Refill Effort 2.0x or more + Refilled STRONG on the defending side (bids at a low, offers at a high): 53% of lows / 46% of highs turned vs about 20% normally; strong refill alone: 34% / 28%; weak refill = defense failing, don't fade ?: only 10% / 8% turned. Example: 5:39 AM ES new high, offers Refill Effort 4.8x, Refilled 2.0x STRONG -> BEARISH ? C+R, price fell from 7,752 to 7,733. Measured on 7 markets and one week of order-book data: every refill read shows ? until it holds on new days. Box: the candle under the mouse while inside this pane, else the live candle. A closed candle whose side pulled at least 2.0x its 20-candle average and at least 1.2x what it added gets a pull marker at the bottom. Settings: Position (the box), Near ticks (0 = auto: how close to price counts).");
    p->setVersion("1.31.0");   // BF_VERSION
    return p;
}
