/********************************************************************************
 *  StackStats.cpp  --  Investor/RT RTX extension  lsStackStats  1.4.0  (2026-10-14 night)
 *  1.4.0 (2026-10-14 night, Rassul approved 22:37 CT, mockup-one-bar.png v2): ONE BAR PER CANDLE (OrderFlowFeed.h 1.6.0): the bigger
 *        side's bar (green = bids added, red = offers added), its lots as the height; tie = the close direction's side, else red; bright
 *        = the winner spiked (label above), dim otherwise; a white tick at the smaller side's lots ONLY when both sides spiked - its only
 *        white mark: (Rassul 23:00 CT) NO letters (I? / L) and NO refill dash drawn; the hover and the records keep them. Pane title "Stack Stats - Who is Arriving v1.4.0" (parmsTitle). Hover, read line, settings,
 *        records, axis: unchanged.
 *  1.3.0 (2026-10-14 night, Rassul via the coordinator "every order-flow pane gets its own ONE read line in place of its box"; approval
 *        list s.7): the READ box is gone (OrderFlowFeed.h 1.5.0). Box shows: Read (default) = ONE line at the box's Position / Offset,
 *        only while price is inside a key-level or gamma zone (ZoneRead.h 1.0.0): size ADDED on the defending side at the level in the
 *        3 minutes after price arrived - "ES 7750 - bids stacking 2.3x normal ?" / "ES 7750 - no new bids ?", locked when the window
 *        closes; the hover says stacking / refill size alone mostly reflects a weak attack (read it with BidAsk). The refill / iceberg
 *        letter I is drawn "I?" (inferred). The candle message ("... - defending PDL.") is no longer drawn (still in the record).
 *  1.2.0 (2026-10-14 evening): complete order feed - the 250 ms clock (OrderFlowFeed.h 1.4.0), MBO_COVERAGE in the status.
 *  1.1.3 (2026-10-14, CR5300; Rassul 11:59 CT CLZ6 3-min "the label for pace and lots on the red spike goes away but it is there on
 *        the green spike. fix it"): spike labels never lose their pace / lots because of another spike (OrderFlowFeed.h 1.3.1: labels
 *        decided in time order from older labels only, no shrink pass; the hover grid drawn over them, not re-laying them out;
 *        candles left of the view in the layout so a scroll keeps them). Seller and buyer labels behave the same.
 *  1.1.2 (2026-10-14, CR4800; Rassul 07:02-07:22): the left box is a READ box (the read, max 3 lines, + the blue options line; the
 *        numbers on its hover) - "Box shows: Read / Numbers"; Offset X / Y on the 9-spot Position; letters white text with no box; one
 *        spike label per candle never over the read; L only for a rare large order (OrderFlowFeed.h 1.3.0).
 *  1.1.0 (2026-10-13, approved by Rassul 4:49 PM CT "build"; SPEC /home/claude/aud/of140/SPEC.md, mockup orderflow-contracts-mockup.png):
 *        bars = LOTS added near price (were orders); PACE (added lots / s vs the same-time norm); 3-row spike labels; the SPEC message
 *        grammar ("Buyers added 42 bids totaling 150 lots (4.4x) fast (p 3.4x) - rebuilt 90% after hits - defending PDL." ...);
 *        REBUILT % (lots re-added at a price after it was hit / lots hit, lra passive_aggr's restore); hover rows Added / Pace /
 *        Orders / Top order / Rebuilt / Refills (I); the blue oStack line (LRA-Options-<MKT>.csv) when fresh. Rules: StackStatsLogic.h.
 *  1.0.1 (2026-10-13, Rassul 1:30 PM CT): the refill / iceberg letter is I (was R); R is reserved for a future stop-run signal.
 *        (1:43 PM) no white outline on the bars or the letter boxes (same-colour pen); hovered candle not outlined.
 *
 *  WHO IS ARRIVING. Rassul 2026-10-13: "add a stacking indicator" / "keep seperate" (its own pane, its own DLL) / "include L for Bid
 *  Ask Stacks and Pull" / "why dont you just use a dash in the bar" / "i want to identify spikes". Approved mockups:
 *  final-signals-mockup.png, stackstats-dash-crop.png.
 *
 *  Its own pane, two bars per candle: the number of ORDERS ADDED near price (NEW; a MOD to a new price near = an add; a MOD to a
 *  bigger size counts in contracts):  left green = bids added (buyers building support)   right red = offers added (a wall building).
 *  REFILLS (after a FILL at a price, a NEW / size increase on the same side at the same price within refillWindowMs): a WHITE
 *  horizontal DASH inside that side's bar at the height of min(refills, bar) - only when the refills are notable (>= 2.0x their
 *  average as of the candle). Spikes as lsBidAskStats 1.2.0 (bright #00FF00 / #FF3B3B; normal #1F9D55 / #962D2D).
 *  Letters (white box, black letter, bottom of the side's bar, priority I then L, at most 2): I = notable refills + iceberg confirmed
 *  (more traded at a refill price than was ever displayed there); L = one order added with size >= largeLotResting.
 *  ONE message line centred at the top for the candle under the mouse (else the last closed candle), coloured by implication:
 *      "10:21  Buyers adding + reloading bids at LonLO (refills 5.2x, iceberg) - real support"      (green = bullish)
 *  INFO BOX (Rassul 12:05 "all values go in the info box, which I should be able to see as I move my mouse over different bars"):
 *    Position (9 spots, default Top left), fixed size, "Live" or the candle's time; Bids (green) | Offers (red) columns with every
 *    value (Added n (x), Refills n (x), Iceberg shown / traded, 1 order %, Avg size, Large).
 *  SPIKE LABELS (Rassul 12:03 "i want the quantity and a ratio on top of the spikes"): ratio over quantity above each spike bar.
 *  Rules: StackStatsLogic.h; order engine, feed, drawing, record and params: OrderFlowFeed.h. Record:
 *  lsFlexLevels\StackStats\<MKT>-<CONTRACT>-<spb>-<session>.csv; params: lsFlexLevels\LRA-OrderFlow-<MKT>.csv; status:
 *  lsFlexLevels\StackStats\status-<MKT>-<CONTRACT>-<spb>.txt. Settings as lsPullStats (StackStats.place-<MKT>.txt).
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
#include "StackStatsLogic.h"

#define SS_VERSION "1.5.0"          // keep equal to st::VERSION and the setVersion literal below

class StackStats : public off::MboPane<st::Logic> {
public:
    StackStats() : off::MboPane<st::Logic>() {}
    virtual int oneBarBull() const { return 0; }                       // (1.4.0) one bar per candle; buyers = side 0 (bids, green)
    virtual bool barMarks() const { return false; }                    // (1.4.0, Rassul 23:00 CT) no letters (I? / L) and no refill dash on the chart; hover + record keep them
    virtual int parmsTitle(char* pStr, int size) { return off::copyTitle(pStr, size, st::Logic::title(), SS_VERSION) ? RTX_OK : RTX_FAIL; }   // (1.4.0) the pane title
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<StackStats*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<StackStats*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) {
    // (1.1.2, CR4800 2026-10-14) + Offset X / Y (px, -600..600) and "Box shows" Read / Numbers, appended; version 2 = IRT resets the
    // dialog rows once - harmless: the chart reads the per-market file and the dialog is filled from it when it opens
    setParameterVersion(2);
    setParameterDialogHeight(4);
    setIntegerParameter("Ratio bars", off::RATIO_DEFAULT, 50);
    setListParameter("Position", (short)off::POS_DEFAULT, off::POS_CHOICES, 110, kParmAppendSameLine);
    setFloatParameter("Spike vs average", 2.0f, 50);
    setFloatParameter("Spike vs prior biggest", 1.5f, 50, kParmAppendSameLine);
    setIntegerParameter("Spike lookback", 10, 50, kParmAppendSameLine);
    setIntegerParameter("Near ticks (0 = auto)", 0, 50);
    setIntegerParameter("Offset X", 0, 50);                                // (1.1.2) parameter 6
    setIntegerParameter("Offset Y", 0, 50, kParmAppendSameLine);           // 7
    setListParameter("Box shows", (short)off::MODE_READ, off::MODE_CHOICES, 90, kParmAppendSameLine);   // 8
    return RTX_OK;
}
int cppExtension::calc(int) { static_cast<StackStats*>(this)->calcHook(); return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    StackStats* p = new StackStats();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE);                   // its own pane, draws itself; mouse events even when not selected
    p->setExtendedFlags(CALL_CONTINUOUSLY);                            // the order feed keeps coming while no trade prints
    p->setDescription("LRA StackStats: who is ARRIVING. Per candle, the LOTS of orders ADDED near price (market-by-order feed). One bar per candle = the bigger side: green = bids added, red = offers added; a white tick at the other side's lots when both sides spiked. Bright = a spike: >= Spike vs average x that side's average AND >= Spike vs prior biggest x the biggest bar of either side in the prior Spike lookback candles. Refills (re-added at a price right after it traded), I (refills + an iceberg confirmed) and L (one large order added) are in the hover, not drawn. Spike labels: the ratio, the pace (p = added lots per second vs the same time of day) and the lots. Top: a plain-English message (green = bullish, red = bearish, white = one order is most of it, may not reload ?), a blue options line when fresh (oStack), and the info box (Bids | Offers: added lots, rebuilt %, pace, crowd = orders, avg size, top order share, refills) for the candle under the mouse anywhere on the chart (else the last closed candle). Settings: Ratio bars, Position, Spike vs average, Spike vs prior biggest, Spike lookback (0 = off), Near ticks (0 = auto).");
    p->setVersion("1.5.0");   // SS_VERSION
    return p;
}
