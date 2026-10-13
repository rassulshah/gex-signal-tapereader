/********************************************************************************
 *  StackStats.cpp  --  Investor/RT RTX extension  lsStackStats  1.0.0  (2026-10-13)
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
 *  Letters (white box, black letter, bottom of the side's bar, priority R then L, at most 2): R = notable refills + iceberg confirmed
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

#define SS_VERSION "1.0.0"          // keep equal to st::VERSION and the setVersion literal below

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
    p->setDescription("LRA StackStats: who is ARRIVING. Per candle, how many orders were ADDED near price (market-by-order feed). Green = bids added (buyers building support), red = offers added (sellers building a wall). White dash in a bar = notable refills (new orders right after a fill at the same price - reloading). Bright = a spike: >= Spike vs average x that side's average AND >= Spike vs prior biggest x the biggest bar of either side in the prior Spike lookback candles. R = refills with an iceberg confirmed, L = one large order added. Spike labels: the ratio over the number of orders. Top: a plain-English message (green = bullish, red = bearish) and the info box with every value, for the candle under the mouse (else the last closed candle). Settings: Ratio bars, Position, Spike vs average, Spike vs prior biggest, Spike lookback (0 = off), Near ticks (0 = auto).");
    p->setVersion("1.0.0");   // SS_VERSION
    return p;
}
