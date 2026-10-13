/********************************************************************************
 *  PullStats.cpp  --  Investor/RT RTX extension  lsPullStats  1.0.1  (2026-10-13)
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

#define PS_VERSION "1.0.1"          // keep equal to ps::VERSION and the setVersion literal below

class PullStats : public off::MboPane<ps::Logic> {
public:
    PullStats() : off::MboPane<ps::Logic>() {}
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<PullStats*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<PullStats*>(this)->release(); return RTX_OK; }
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
int cppExtension::calc(int) { static_cast<PullStats*>(this)->calcHook(); return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    PullStats* p = new PullStats();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE);                   // its own pane, draws itself; mouse events even when not selected
    p->setExtendedFlags(CALL_CONTINUOUSLY);                            // the order feed keeps coming while no trade prints
    p->setDescription("LRA PullStats: who is LEAVING. Per candle, how many resting orders were PULLED near price (cancelled without being filled; market-by-order feed). Green = bids pulled (buyers leaving), red = offers pulled (sellers leaving). Bright = a spike: >= Spike vs average x that side's average AND >= Spike vs prior biggest x the biggest bar of either side in the prior Spike lookback candles. L = one large resting order pulled. Spike labels: the ratio over the number of orders. Top: a plain-English message (green = bullish, red = bearish) and the info box with every value, for the candle under the mouse (else the last closed candle). Settings: Ratio bars, Position, Spike vs average, Spike vs prior biggest, Spike lookback (0 = off), Near ticks (0 = auto).");
    p->setVersion("1.0.1");   // PS_VERSION
    return p;
}
