#ifndef LS_PULLSTATS_LOGIC_H
#define LS_PULLSTATS_LOGIC_H
// PullStatsLogic.h (1.0.0, 2026-10-13) -- SDK-free rules of lsPullStats: who is LEAVING. Unit-tested on Linux.
// Rassul 2026-10-13: "i also want to know if there is the way to use the mbo data which tells you the number of bids and ask pulled
// like a histogram" / "keep seperate" / "include L for Bid Ask Stacks and Pull". Approved: final-signals-mockup.png,
// three-panes-messages-mockup.png. The order engine (what is a pull) is OrderFlowFeed.h.
//   Bars   = the number of resting ORDERS pulled near price in the candle: left green = bids pulled (buyers leaving),
//            right red = offers pulled (sellers leaving). Bright = a spike (as lsBidAskStats 1.2.0).
//   Letter = L at the bottom of a side's bar: one resting order pulled with size >= largeLotResting (default the 99th percentile of
//            the resting sizes added near price this session before the candle, at least 20 lots).
//   Message (coloured by IMPLICATION): offers pulled = bullish green "Sellers pulled their offers above price (4.4x) incl. one 80-lot
//            - attackers leaving"; bids pulled = bearish red "Buyers pulled their bids under price (3.1x) - support leaving";
//            ", just before price got there" when >= 1/4 of that side's pulled orders (and >= 2) were within closeTicks (2) of the
//            last trade; " at <LEVEL>" when a key / gamma level is within the near band of the candle's close.
//   Numbers line (session record): "AskPull 92 (4.4x)  356 cts  Large 80   |   BidPull 10 (0.5x)  41 cts"; on the chart every value is in
//   the info box (12:05) and the ratio / quantity sit above each spike (12:03).
#include "OrderFlowFeed.h"

namespace ps {

static const char* const VERSION = "1.0.0";
using off::BID; using off::ASK;

inline std::string letters(const off::Rec& r, int s) { return r.thrRest > 0 && r.pullMax[s] >= r.thrRest ? "L" : ""; }
inline std::string sideNums(const off::PaneBar& b, int s) {
    std::string o = std::string(s == BID ? "BidPull " : "AskPull ") + std::to_string(b.r.pullN[s]) + " (" + off::ratioOr(b.m[0].ratioOk[s], b.m[0].ratio[s]) + ")  "
        + std::to_string(b.r.pullC[s]) + " cts";
    if (b.let[s].find('L') != std::string::npos) o += "  Large " + std::to_string(b.r.pullMax[s]);
    return o;
}
inline bool justBefore(const off::Rec& r, int s) { return r.pullClose[s] >= 2 && r.pullClose[s] * 4 >= r.pullN[s]; }
inline nl::Msg message(const off::PaneBar& b) {
    nl::Msg m;
    if (!b.ok) { m.line1 = "No order data for this candle"; return m; }
    double sc[2] = {0, 0};
    for (int s = 0; s < 2; ++s) if (off::high(b.m[0], s)) sc[s] = b.m[0].ratioOk[s] ? b.m[0].ratio[s] : off::HIGH_X;
    int side = -1;
    if (sc[BID] > 0 || sc[ASK] > 0) {
        if (sc[BID] > sc[ASK]) side = BID; else if (sc[ASK] > sc[BID]) side = ASK;
        else side = b.r.pullN[BID] > b.r.pullN[ASK] ? BID : b.r.pullN[ASK] > b.r.pullN[BID] ? ASK : -1;
    }
    const int first = side >= 0 ? side : (b.r.pullN[ASK] > b.r.pullN[BID] ? ASK : BID);
    m.line2 = sideNums(b, first) + "   |   " + sideNums(b, 1 - first) + (b.r.partial ? "   (partial candle)" : "");
    if (side < 0) { m.line1 = sc[BID] > 0 ? "Both sides pulling evenly (" + off::ratioText(sc[BID]) + ")" : "Normal activity - no spike"; return m; }
    const std::string at = b.level.empty() ? std::string() : " at " + b.level;
    std::string extra;
    if (b.let[side].find('L') != std::string::npos) extra += " incl. one " + std::to_string(b.r.pullMax[side]) + "-lot";
    if (justBefore(b.r, side)) extra += ", just before price got there";
    const std::string r = off::ratioText(b.m[0].ratio[side]);
    if (side == ASK) { m.line1 = "Sellers pulled their offers above price" + at + " (" + r + ")" + extra + " - attackers leaving"; m.impl = nl::BULL; }
    else { m.line1 = "Buyers pulled their bids under price" + at + " (" + r + ")" + extra + " - support leaving"; m.impl = nl::BEAR; }
    return m;
}

// (12:05) the info box: every value of the candle, bids (green) | offers (red)
inline off::Info info(const off::PaneBar* b) {
    off::Info I; I.colHead[0] = "Bids"; I.colHead[1] = "Offers"; I.tmpl = "9999 (99.9x)";
    const char* labels[4] = { "Pulled", "Contracts", "Just before", "Large" };
    for (int k = 0; k < 4; ++k) {
        off::InfoRow r; r.label = labels[k];
        for (int s = 0; s < 2; ++s) {
            if (!b || !b->ok) { r.v[s] = "n/a"; continue; }
            if (k == 0) r.v[s] = std::to_string(b->r.pullN[s]) + " (" + off::ratioOr(b->m[0].ratioOk[s], b->m[0].ratio[s]) + ")";
            else if (k == 1) r.v[s] = std::to_string(b->r.pullC[s]);
            else if (k == 2) r.v[s] = std::to_string(b->r.pullClose[s]);
            else r.v[s] = b->let[s].find('L') != std::string::npos ? std::to_string(b->r.pullMax[s]) : std::string("-");
        }
        I.rows.push_back(r);
    }
    return I;
}

struct Logic {
    static const char* name() { return "PullStats"; }
    static const char* version() { return VERSION; }
    static int metrics() { return 1; }
    static off::Info info(const off::PaneBar* b) { return ps::info(b); }
    static long long value(const off::Rec& r, int, int s) { return r.pullN[s]; }
    // a closed candle's display, decided once (live = the forming candle, recomputed every draw, never recorded)
    static void decide(off::PaneBar& b, const off::Params&, bool) {
        for (int s = 0; s < 2; ++s) { b.spike[s] = b.ok && b.m[0].spike[s]; b.dash[s] = -1; b.let[s] = b.ok ? letters(b.r, s) : std::string(); }
        b.msg = message(b);
    }
};

} // namespace ps
#endif
