#ifndef LS_STACKSTATS_LOGIC_H
#define LS_STACKSTATS_LOGIC_H
// StackStatsLogic.h (1.0.0, 2026-10-13) -- SDK-free rules of lsStackStats: who is ARRIVING. Unit-tested on Linux.
// Rassul 2026-10-13: "add a stacking indicator" / "keep seperate" / "include L for Bid Ask Stacks and Pull" / "why dont you just use a
// dash in the bar". Approved: final-signals-mockup.png, stackstats-dash-crop.png. The order engine is OrderFlowFeed.h.
//   Bars   = the number of ORDERS added near price (NEW; a MOD to a bigger size counts in contracts only): left green = bids added,
//            right red = offers added. Bright = a spike (as lsBidAskStats 1.2.0).
//   Dash   = REFILLS (a NEW / size increase on the same side at the same price within refillWindowMs after a FILL there): a WHITE
//            horizontal dash inside that side's bar at the height of min(refills, bar) - ONLY when the refills are notable
//            (>= refillNotableX = 2.0 x their average as of the candle). No separate refill bar.
//   Letters (priority R, L; at most 2): R = notable refills AND, at one of those refill prices, more traded than was ever displayed
//            there (iceberg confirmed); L = one order added with size >= largeLotResting.
//   Message: bids added / refilled = bullish green "Buyers adding + reloading bids at LonLO (refills 5.2x, iceberg) - real support";
//            offers added = bearish red "Sellers adding offers (3.8x) - wall building above"; " - mostly one order, may be bait"
//            when the largest single order holds >= baitPct (70%) of that side's added contracts.
//   Numbers line (session record): "BidStack 105 (4.1x)  BidRefill 44 (5.2x)  1 order = 12%  avg 3.1 cts   |   AskStack 12 (0.5x)"; on the
//   chart every value is in the info box (12:05) and the ratio / quantity sit above each spike (12:03).
#include "OrderFlowFeed.h"

namespace st {

static const char* const VERSION = "1.0.0";
using off::BID; using off::ASK;

inline bool notable(const off::PaneBar& b, int s, const off::Params& P) {
    return b.ok && b.r.refN[s] > 0 && b.m[1].ratioOk[s] && b.m[1].ratio[s] * 10.0 >= (double)P.refillNotableT - 1e-9;
}
inline std::string letters(const off::PaneBar& b, int s, const off::Params& P) {
    std::string o;
    if (notable(b, s, P) && b.r.ice[s] > 0) o += "R";
    if (b.r.thrRest > 0 && b.r.addMax[s] >= b.r.thrRest) o += "L";
    return o;
}
// "1 order = 12%  avg 3.1 cts" ("" without contracts)
inline std::string concText(long long top, long long cts, long long n) {
    if (cts <= 0 || top <= 0) return std::string();
    const long long pct = (top * 100 + cts / 2) / cts;
    std::string s = "1 order = " + std::to_string(pct > 100 ? 100 : pct) + "%";
    if (n > 0) { char b[32]; std::snprintf(b, sizeof(b), "  avg %.1f cts", (double)cts / (double)n); s += b; }
    return s;
}
inline std::string sideNums(const off::PaneBar& b, int s, bool full) {
    const char* S = s == BID ? "Bid" : "Ask";
    std::string o = std::string(S) + "Stack " + std::to_string(b.r.addN[s]) + " (" + off::ratioOr(b.m[0].ratioOk[s], b.m[0].ratio[s]) + ")";
    if (full) {
        o += "  " + std::string(S) + "Refill " + std::to_string(b.r.refN[s]) + " (" + off::ratioOr(b.m[1].ratioOk[s], b.m[1].ratio[s]) + ")";
        const std::string c = concText(b.r.addTop[s], b.r.addC[s], b.r.addN[s]); if (!c.empty()) o += "  " + c;
    }
    if (b.let[s].find('L') != std::string::npos) o += "  Large " + std::to_string(b.r.addMax[s]);
    return o;
}
inline nl::Msg message(const off::PaneBar& b, const off::Params& P) {
    nl::Msg m;
    if (!b.ok) { m.line1 = "No order data for this candle"; return m; }
    double sc[2] = {0, 0}; bool hA[2] = {false, false}, hR[2] = {false, false};
    for (int s = 0; s < 2; ++s) {
        hA[s] = off::high(b.m[0], s); hR[s] = notable(b, s, P);
        if (hA[s]) sc[s] = b.m[0].ratioOk[s] ? b.m[0].ratio[s] : off::HIGH_X;
        if (hR[s] && b.m[1].ratio[s] > sc[s]) sc[s] = b.m[1].ratio[s];
    }
    int side = -1;
    if (sc[BID] > 0 || sc[ASK] > 0) {
        if (sc[BID] > sc[ASK]) side = BID; else if (sc[ASK] > sc[BID]) side = ASK;
        else side = b.r.addN[BID] > b.r.addN[ASK] ? BID : b.r.addN[ASK] > b.r.addN[BID] ? ASK : -1;
    }
    const int first = side >= 0 ? side : (b.r.addN[ASK] > b.r.addN[BID] ? ASK : BID);
    m.line2 = sideNums(b, first, true) + "   |   " + sideNums(b, 1 - first, false) + (b.r.partial ? "   (partial candle)" : "");
    if (side < 0) { m.line1 = sc[BID] > 0 ? "Both sides adding evenly (" + off::ratioText(sc[BID]) + ")" : "Normal activity - no spike"; return m; }
    const std::string at = b.level.empty() ? std::string() : " at " + b.level;
    const std::string refills = "(refills " + off::ratioOr(b.m[1].ratioOk[side], b.m[1].ratio[side]) + (b.r.ice[side] > 0 ? ", iceberg)" : ")");
    const std::string r0 = "(" + off::ratioOr(b.m[0].ratioOk[side], b.m[0].ratio[side]) + ")";
    if (side == BID) {
        m.impl = nl::BULL;
        if (hA[side] && hR[side]) m.line1 = "Buyers adding + reloading bids" + at + " " + refills + " - real support";
        else if (hA[side]) m.line1 = "Buyers adding bids" + at + " " + r0 + " - support building";
        else m.line1 = "Buyers reloading bids" + at + " " + refills + " - hidden support";
    } else {
        m.impl = nl::BEAR;
        if (hA[side] && hR[side]) m.line1 = "Sellers adding + reloading offers" + at + " " + refills + " - real wall above";
        else if (hA[side]) m.line1 = "Sellers adding offers" + at + " " + r0 + " - wall building above";
        else m.line1 = "Sellers reloading offers" + at + " " + refills + " - hidden wall above";
    }
    const long long top = b.r.addTop[side], cts = b.r.addC[side];
    if (cts > 0 && top * 100 >= P.baitPct * cts) m.line1 += " - mostly one order, may be bait";
    return m;
}

// (12:05) the info box: every value of the candle, bids (green) | offers (red)
inline off::Info info(const off::PaneBar* b) {
    off::Info I; I.colHead[0] = "Bids"; I.colHead[1] = "Offers"; I.tmpl = "9999 (99.9x)";
    const char* labels[6] = { "Added", "Refills", "Iceberg", "1 order", "Avg size", "Large" };
    for (int k = 0; k < 6; ++k) {
        off::InfoRow r; r.label = labels[k];
        for (int s = 0; s < 2; ++s) {
            if (!b || !b->ok) { r.v[s] = "n/a"; continue; }
            const off::Rec& x = b->r;
            if (k == 0) r.v[s] = std::to_string(x.addN[s]) + " (" + off::ratioOr(b->m[0].ratioOk[s], b->m[0].ratio[s]) + ")";
            else if (k == 1) r.v[s] = std::to_string(x.refN[s]) + " (" + off::ratioOr(b->m[1].ratioOk[s], b->m[1].ratio[s]) + ")";
            else if (k == 2) r.v[s] = x.ice[s] > 0 ? std::to_string(x.iceShown[s]) + " / " + std::to_string(x.iceTraded[s]) : std::string("-");   // shown / traded
            else if (k == 3) r.v[s] = x.addC[s] > 0 && x.addTop[s] > 0 ? std::to_string(std::min(100LL, (x.addTop[s] * 100 + x.addC[s] / 2) / x.addC[s])) + "%" : std::string("-");
            else if (k == 4) { if (x.addN[s] > 0) { char t[32]; std::snprintf(t, sizeof(t), "%.1f", (double)x.addC[s] / (double)x.addN[s]); r.v[s] = t; } else r.v[s] = "-"; }
            else r.v[s] = b->let[s].find('L') != std::string::npos ? std::to_string(x.addMax[s]) : std::string("-");
        }
        I.rows.push_back(r);
    }
    return I;
}

struct Logic {
    static const char* name() { return "StackStats"; }
    static const char* version() { return VERSION; }
    static int metrics() { return 2; }
    static off::Info info(const off::PaneBar* b) { return st::info(b); }
    static long long value(const off::Rec& r, int metric, int s) { return metric == 0 ? r.addN[s] : r.refN[s]; }
    static void decide(off::PaneBar& b, const off::Params& P, bool) {
        for (int s = 0; s < 2; ++s) {
            b.spike[s] = b.ok && b.m[0].spike[s];
            b.dash[s] = notable(b, s, P) ? (b.r.refN[s] < b.r.addN[s] ? b.r.refN[s] : b.r.addN[s]) : -1;   // min(refills, bar)
            b.let[s] = b.ok ? letters(b, s, P) : std::string();
        }
        b.msg = message(b, P);
    }
};

} // namespace st
#endif
