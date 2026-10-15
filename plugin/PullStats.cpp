/********************************************************************************
 *  PullStats.cpp  --  Investor/RT RTX extension  lsPullStats  1.6.0  (2026-10-14 night)
 *  1.6.0 (2026-10-14 night, Rassul approved 22:37 CT, mockup-one-bar.png v2): ONE BAR PER CANDLE (OrderFlowFeed.h 1.6.0): the bigger
 *        side's bar (green = bids pulled, red = offers pulled), its lots as the height; tie = the close direction's side, else red;
 *        bright = the winner spiked (label above), dim otherwise; a white tick at the smaller side's lots ONLY when both sides spiked
 *        (both pulled - the book thinning); letters on the single bar for the winner. Pane title "Pull Stats - Who is Leaving v1.6.0"
 *        (parmsTitle). Hover, the locked arrival read line (ZoneRead.h 1.1.0), Record order feed, settings, records, axis: unchanged.
 *  1.5.0 (2026-10-14 night; Rassul approved 20:04 CT "merge lsPulling's read into lsPullStats and retire lsPulling", then changed the
 *        design: "the PullStats line shows ONLY state 3, the locked arrival read after the 3-min window ... It emphasises the pull";
 *        "draw NO pre-touch line and NO measuring line; nothing shows until the read locks"; "still compute and record the pre-touch
 *        % ... and put it in the hover as a secondary fact"): the ONE zone read line (ZoneRead.h 1.1.0) is now lsPulling 0.3.0's
 *        ARRIVAL read - in the first 3 minutes after price reached a key / gamma level, was the size resting AT the level on the
 *        defending side cancelled or traded (this chart's own order ids)? Drawn only once it locks at 3:00:
 *          "ES 7750 - bids pulled, not filled (62% in 3 min) - less support ?"   red (green for offers pulled at resistance)
 *          "ES 7750 - bids traded, held (2.1x normal) - support holding ?"         green at support (red for offers at resistance)
 *          "ES 7750 - bids: 30% pulled, 40% traded in 3 min ?"                     grey (mixed)
 *        always "?", never repainted. Hover: the arrival cancelled / traded numbers and the normal, the pre-touch share (secondary),
 *        what / why / how / an example, and both study caveats (arrival: held ~19 pp less often, association only, NOT YET proven;
 *        pre-touch: no clear sign yet). The session record (PullStats\<stem>-zoneread-<session>.csv) gets an "arrived" row at arrival
 *        and a "frozen" row at the lock, with the pre-touch part in four new columns at the end (an existing 1.4.0 file's header is
 *        upgraded once; its rows are still read). A restart inside the 3 minutes cannot follow the orders: nothing until the window
 *        ends, then a grey "bids: arrival read not known (restarted during the 3 min) ?". Bars, spike labels, letters, the feed
 *        clock and the recorder: unchanged. lsPulling is a retired stub from tonight.
 *  1.4.0 (2026-10-14 night, Rassul via the coordinator "every order-flow pane gets its own ONE read line in place of its box"): the
 *        READ box is gone (OrderFlowFeed.h 1.5.0). Box shows: Read (default) = ONE line at the box's Position / Offset, only while price
 *        is inside a key-level or gamma zone (ZoneRead.h 1.0.0): PRE-TOUCH pulling - of the size resting at the level 5 minutes before
 *        price arrived, the share cancelled before it got there: "ES 7750 - bids pulled before touch (45%) ?" (z >= 1 vs the market's
 *        same-30-min-slot normal; else "bids stayed before touch (12% pulled) ?"), locked at the arrival, always "?"; the hover says the
 *        study showed no clear sign yet. The candle message ("... - support leaving") is no longer drawn (still in the session record).
 *        Spikes, labels, letters, the feed clock and the "Record order feed" recorder: unchanged.
 *  1.3.0 (2026-10-14 evening; Rassul's standing order "always record IRT data and keep the MBO files updated"; LIQ-DAY-2026-10-14:
 *        mbo_HH.csv held only 30-45% of the ES volume in busy periods): the order feed is read by a 250 ms clock as well as calc / draw
 *        (OrderFlowFeed.h 1.4.0 - IRT keeps only the last few thousand events per subscriber and drew this chart every 6-10 s), every
 *        event goes to the recorder as it is read and the clock flushes the recorder once a second (so recording continues while the
 *        layout tab is hidden); the status gains FEED_CLOCK and MBO_COVERAGE (fills read / bar volume per closed candle). The recorder
 *        reuses the event's clock time within a second (one getLocaltime per second, not per event) - MboRecord.h 1.1.0.
 *  1.2.2 (2026-10-14, CR5300; Rassul 11:59 CT CLZ6 3-min "the label for pace and lots on the red spike goes away but it is there on
 *        the green spike. fix it"): spike labels never lose their pace / lots because of another spike (OrderFlowFeed.h 1.3.1: labels
 *        decided in time order from older labels only, no shrink pass; the hover grid drawn over them, not re-laying them out;
 *        candles left of the view in the layout so a scroll keeps them). Seller and buyer labels behave the same.
 *  1.2.1 (2026-10-14, CR4800; Rassul 07:02-07:22): the left box is a READ box (the read, max 3 lines, + the blue options line; the
 *        numbers on its hover) - "Box shows: Read / Numbers"; Offset X / Y on the 9-spot Position; letters white text with no box; one
 *        spike label per candle never over the read; L only for a rare large order (OrderFlowFeed.h 1.3.0). Settings from the per-market
 *        file on every draw, the dialog filled from it.
 *  1.2.0 (2026-10-14, "Record order feed", approved by Rassul 2026-10-13; settings audit): a check box (default ON) that appends every
 *        order-by-order event this pane drains to C:\Dev\level-reversal-analytics\data\irt\<session>\<MKT>\mbo_HH.csv in lra.mbo_live's
 *        format (time|action|price|prev_price|size|order_id|aggressor_id|side|bid|ask; session = the 17:00 CT day, HH = the CT hour).
 *        Buffered, written once a second from the calc pass, one writer per market, stops when another program writes the same file
 *        (lsPulling), paused under 2 GB free - MboRecord.h. Status: MBO_RECORD line. Unticked = nothing is written.
 *  1.1.0 (2026-10-13, approved by Rassul 4:49 PM CT "build"; SPEC /home/claude/aud/of140/SPEC.md, mockup orderflow-contracts-mockup.png):
 *        bars = LOTS pulled near price (were orders); PACE (pulled lots / s vs the same-time norm); 3-row spike labels (ratio / pace /
 *        lots); the SPEC message grammar ("Buyers pulled 64 bids totaling 410 lots (3.1x) fast (p 2.6x) - support leaving at PDL.",
 *        "resistance leaving" for offers, "bait gone" white, and the LATER "Bids back within 40 s of the 11:05 pull - reset, not a
 *        retreat at PDL." posted only when known); hover rows Pulled / Pace / Orders / Top order / Traded first / Before price; the
 *        blue oPull line (LRA-Options-<MKT>.csv) when fresh. Rules: PullStatsLogic.h, OrderFlowFeed.h 1.2.0.
 *  1.0.1 (2026-10-13, Rassul 1:43 PM CT): no white outline on the bars or the letter boxes (same-colour pen); hovered candle not outlined.
 *
 *  WHO IS LEAVING. Rassul 2026-10-13: "i also want to know if there is the way to use the mbo data which tells you the number of
 *  bids and ask pulled like a histogram" / "keep seperate" (its own pane) / "include L for Bid Ask Stacks and Pull" /
 *  "i want to identify spikes". Approved mockups: final-signals-mockup.png, three-panes-messages-mockup.png.
 *
 *  Its own pane, two bars per candle (as lsBidAskStats 1.2.0): the number of resting ORDERS PULLED near price - cancelled without
 *  being filled (a DEL after a FILL of that order is not a pull; a MOD to a new price = a pull at the old price + an add at the new,
 *  each only when near; a MOD size cut counts in contracts):
 *      left  green = bids pulled  (buyers leaving)        right red = offers pulled (sellers leaving)
 *  Normal #1F9D55 / #962D2D; a SPIKE (>= 2.0x that side's average as of the candle AND >= 1.5x the biggest bar of EITHER side in the
 *  prior 10 candles) bright #00FF00 / #FF3B3B. Closed candles are decided once (colour, letter, message final; no repaint).
 *  Letter L (white box, black letter, bottom of the side's bar) = one resting order pulled with size >= largeLotResting.
 *  ONE message line centred at the top for the candle under the mouse (else the last closed candle), coloured by implication:
 *      "10:30  Sellers pulled their offers above price (4.4x) incl. one 80-lot - attackers leaving"     (green = bullish)
 *  INFO BOX (Rassul 12:05 "all values go in the info box, which I should be able to see as I move my mouse over different bars"):
 *    Position (9 spots, default Top left), fixed size, "Live" or the candle's time; Bids (green) | Offers (red) columns with every
 *    value (Pulled n (x), Contracts, Just before, Large).
 *  SPIKE LABELS (Rassul 12:03 "i want the quantity and a ratio on top of the spikes"): ratio over quantity above each spike bar.
 *  Rules: PullStatsLogic.h; order engine, feed, drawing, record and params: OrderFlowFeed.h (shared with lsStackStats and
 *  lsBidAskStats 1.3.0). Record: lsFlexLevels\PullStats\<MKT>-<CONTRACT>-<spb>-<session>.csv; params:
 *  lsFlexLevels\LRA-OrderFlow-<MKT>.csv; status: lsFlexLevels\PullStats\status-<MKT>-<CONTRACT>-<spb>.txt.
 *  Settings (controls only): Ratio bars, Position, Spike vs average, Spike vs prior biggest, Spike lookback (0 = off), Near ticks
 *  (0 = auto); kept per market in lsFlexLevels\PullStats.place-<MKT>.txt, written only from OK / Apply.
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
#include "PullStatsLogic.h"
#include "MboRecord.h"              // (1.2.0) Record order feed

#define PS_VERSION "1.6.2"          // keep equal to ps::VERSION and the setVersion literal below

class PullStats : public off::MboPane<ps::Logic> {
public:
    PullStats() : off::MboPane<ps::Logic>() {}
    virtual bool barMarks() const { return false; }   // (1.6.1, Rassul 09:55 "get rid of the labeling like S L") no letters drawn; hover + record keep them
    virtual int oneBarBull() const { return 0; }                       // (1.6.0) one bar per candle; buyers = side 0 (bids, green)
    virtual int parmsTitle(char* pStr, int size) { return off::copyTitle(pStr, size, ps::Logic::title(), PS_VERSION) ? RTX_OK : RTX_FAIL; }   // (1.6.0) the pane title
    // (1.2.0) Record order feed: the recorder lives in this chart's pane state (deleted with it - the destructor writes what is left)
    virtual bool recorderPane() const { return true; }
    mrec::Recorder* rec(S_& S) {
        if (!S.ext) S.ext.reset(new (std::nothrow) RecExt());
        RecExt* r = static_cast<RecExt*>(S.ext.get());
        if (!r) return nullptr;
        r->r.setOn(S.set.record != 0);                                     // -1 (an older 6-field settings file) = the default: on
        if (!S.market.empty()) r->r.setMarket(S.market);
        return &r->r;
    }
    virtual void onRaw(S_& S, const DEPTH_ORDER& o) {
        mrec::Recorder* r = rec(S); if (!r || !r->on) return;
        if (!o.timestamp) { ++r->dropped; return; }
        if (o.timestamp != r->tsKey) {                                     // (1.3.0) one clock conversion per second, not per event
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
    virtual void afterPump(S_& S, long long now, bool fromDraw) { mrec::Recorder* r = rec(S); if (r) r->tick(now, fromDraw); }
    virtual void afterTimer(S_& S, long long now) { mrec::Recorder* r = rec(S); if (r) r->tick(now, false); }   // (1.3.0) the clock writes too
    virtual void statusExtra(S_& S, std::ostream& o) { mrec::Recorder* r = rec(S); if (r) r->status(o); else o << "\nMBO_RECORD,unavailable (out of memory)"; }
private:
    struct RecExt : off::PaneExt { mrec::Recorder r; };
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<PullStats*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<PullStats*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) {
    // (1.2.1, CR4800 2026-10-14) + Offset X / Y and "Box shows", appended; version 2 = IRT resets the dialog rows once (the chart reads
    // the per-market file; the dialog is filled from it when it opens)
    setParameterVersion(2);
    setParameterDialogHeight(4);
    setIntegerParameter("Ratio bars", off::RATIO_DEFAULT, 50);
    setListParameter("Position", (short)off::POS_DEFAULT, off::POS_CHOICES, 110, kParmAppendSameLine);
    setFloatParameter("Spike vs average", 2.0f, 50);
    setFloatParameter("Spike vs prior biggest", 1.5f, 50, kParmAppendSameLine);
    setIntegerParameter("Spike lookback", 10, 50, kParmAppendSameLine);
    setIntegerParameter("Near ticks (0 = auto)", 0, 50);
    setBoolParameter("Record order feed", true, kParmAppendSameLine);  // (1.2.0) parameter 6: data\irt\<session>\<MKT>\mbo_HH.csv
    setIntegerParameter("Offset X", 0, 50);                                // (1.2.1) parameter 7
    setIntegerParameter("Offset Y", 0, 50, kParmAppendSameLine);           // 8
    setListParameter("Box shows", (short)off::MODE_READ, off::MODE_CHOICES, 90, kParmAppendSameLine);   // 9
    return RTX_OK;
}
int cppExtension::calc(int) { static_cast<PullStats*>(this)->calcHook(); return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    PullStats* p = new PullStats();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE);                   // its own pane, draws itself; mouse events even when not selected
    p->setExtendedFlags(CALL_CONTINUOUSLY);                            // the order feed keeps coming while no trade prints
    p->setDescription("LRA PullStats: who is LEAVING. Per candle, the LOTS of resting orders PULLED near price (cancelled without being filled; market-by-order feed). One bar per candle = the bigger side: green = bids pulled (buyers leaving), red = offers pulled (sellers leaving); a white tick at the other side's lots when both sides spiked. Bright = a spike: >= Spike vs average x that side's average AND >= Spike vs prior biggest x the biggest bar of either side in the prior Spike lookback candles. L = one large resting order pulled. Spike labels: the ratio, the pace (p = pulled lots per second vs the same time of day) and the lots. The read line (only at a key / gamma level, shown once it locks 3 minutes after price reached the level): was the size resting at the level cancelled or traded - red = the defence pulled at support (green at resistance), green = traded and held at support, grey = mixed; always '?'. Its hover: the numbers, the normal, the share pulled before the touch, and the study caveats. Settings: Ratio bars, Position, Spike vs average, Spike vs prior biggest, Spike lookback (0 = off), Near ticks (0 = auto), Record order feed (every order-by-order event to the LRA data folder for the nightly MBO archive), Offset X / Y, Box shows.");
    p->setVersion("1.6.2");   // PS_VERSION
    return p;
}
