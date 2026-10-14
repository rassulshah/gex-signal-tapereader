/********************************************************************************
 *  StackStats.cpp  --  Investor/RT RTX extension  lsStackStats  1.1.0  (2026-10-13)
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

#define SS_VERSION "1.1.0"          // keep equal to st::VERSION and the setVersion literal below

class StackStats : public off::MboPane<st::Logic> {
public:
    StackStats() : off::MboPane<st::Logic>() {}
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<StackStats*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<StackStats*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) {
    setParameterVersion(1);
    setParameterDialogHeight(3);
    setIntegerParameter("Ratio bars", off::RATIO_DEFAULT, 50);
    setListParameter("Position", (short)off::POS_DEFAULT, off::POS_CHOICES, 110, kParmAppendSameLine);
    setFloatParameter("Spike vs average", 2.0f, 50);
    setFloatParameter("Spike vs prior biggest", 1.5f, 50, kParmAppendSameLine);
    setIntegerParameter("Spike lookback", 10, 50, kParmAppendSameLine);
    setIntegerParameter("Near ticks (0 = auto)", 0, 50);
    return RTX_OK;
}
int cppExtension::calc(int) { static_cast<StackStats*>(this)->calcHook(); return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    StackStats* p = new StackStats();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE);                   // its own pane, draws itself; mouse events even when not selected
    p->setExtendedFlags(CALL_CONTINUOUSLY);                            // the order feed keeps coming while no trade prints
    p->setDescription("LRA StackStats: who is ARRIVING. Per candle, the LOTS of orders ADDED near price (market-by-order feed). Green = bids added, red = offers added. Bright = a spike: >= Spike vs average x that side's average AND >= Spike vs prior biggest x the biggest bar of either side in the prior Spike lookback candles. White dash = notable refills (re-added at a price right after it traded). I = refills + an iceberg confirmed, L = one large order added. Spike labels: the ratio, the pace (p = added lots per second vs the same time of day) and the lots. Top: a plain-English message (green = bullish, red = bearish, white = one order is most of it, may not reload ?), a blue options line when fresh (oStack), and the info box (Bids | Offers: added lots, pace, orders, top order, rebuilt %, refills) for the candle under the mouse (else the last closed candle). Settings: Ratio bars, Position, Spike vs average, Spike vs prior biggest, Spike lookback (0 = off), Near ticks (0 = auto).");
    p->setVersion("1.1.0");   // SS_VERSION
    return p;
}
