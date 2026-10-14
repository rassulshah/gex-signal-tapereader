#ifndef LS_PULLSTATS_LOGIC_H
#define LS_PULLSTATS_LOGIC_H
// PullStatsLogic.h (1.1.0, 2026-10-13) -- SDK-free rules of lsPullStats: who is LEAVING. Unit-tested on Linux.
// 1.1.0 (2026-10-13, approved by Rassul 4:49 PM CT "build"; SPEC /home/claude/aud/of140/SPEC.md, mockup orderflow-contracts-mockup.png):
//   Bars = LOTS pulled near price (were order counts); spike label ratio / pace / lots; PACE (pulled lots / s vs the same-time norm);
//   the message grammar of the SPEC (exact):
//     "Buyers pulled 64 bids totaling 410 lots (3.1x) fast (p 2.6x) - support leaving at PDL."                     red (bearish)
//     "Sellers pulled 38 offers totaling 95 lots (3.6x) fast (p 2.8x) - resistance leaving at PDH."                green (bullish)
//        (resting sellers are passive: "resistance leaving", never "attackers")
//     "Buyers pulled 4 bids totaling 120 lots (2.4x) - one 100-lot pulled before it traded - bait gone at PDL."    white
//        (the biggest pulled order holds >= baitPct (70%) of the side's pulled lots and never traded)
//     LATER, only when known (never back-dated): "Bids back within 40 s of the 11:05 pull - reset, not a retreat at PDL." (white,
//        OrderFlowFeed.h resetStep)
//   Hover rows (Bids | Offers): Pulled lots (ratio), Pace x (threshold), Orders N · avg X lots, Top order NN%, Traded first yes/no,
//   Before price yes/no ("just before price got there": >= 1/4 of the side's pulled orders, >= 2, within closeTicks of the last trade).
//   Letter L unchanged (one resting order pulled with size >= largeLotResting). Level named only within the near band.
// 1.0.0 / 1.0.1 (2026-10-13): Rassul "the number of bids and ask pulled like a histogram" / "keep seperate" / "include L ...".
#include "OrderFlowFeed.h"

namespace ps {

static const char* const VERSION = "1.2.0";   // 1.2.0 (2026-10-14): Record order feed (MboRecord.h, host only); 1.1.1 (2026-10-13 9:40 PM): row order + Crowd, 10 pt labels, chart-wide hover; 1.1.0 (2026-10-13): lots, pace, SPEC message grammar, reset, hover rows; 1.0.1: no white outline
using off::BID; using off::ASK;

inline std::string letters(const off::Rec& r, int s) { return r.thrRest > 0 && r.pullMax[s] >= r.thrRest ? "L" : ""; }
inline bool justBefore(const off::Rec& r, int s) { return r.pullClose[s] >= 2 && r.pullClose[s] * 4 >= r.pullN[s]; }
inline bool baitGone(const off::Rec& r, int s, const off::Params& P) { return r.pullC[s] > 0 && r.pullMax[s] > 0 && r.pullMax[s] * 100 >= P.baitPct * r.pullC[s] && r.pullTraded[s] == 0; }
// the record's numbers line (the Tape Reader reads "BidPull <lots> (<ratio>)" / "AskPull ..."):
// "AskPull 95 (3.6x)  AskPace 2.8x  38 orders  Top 22%  Traded first no  Before price yes   |   BidPull 41 (0.5x) ..."
inline std::string sideNums(const off::PaneBar& b, int s) {
    const char* S = s == BID ? "Bid" : "Ask";
    std::string o = std::string(S) + "Pull " + std::to_string(b.r.pullC[s]) + " (" + off::ratioOr(b.m[0].ratioOk[s], b.m[0].ratio[s]) + ")";
    if (b.r.pace100[s] > 0) o += "  " + std::string(S) + "Pace " + off::paceX(b.r.pace100[s]);
    o += "  " + std::to_string(b.r.pullN[s]) + (b.r.pullN[s] == 1 ? " order" : " orders");
    if (b.r.pullC[s] > 0) o += "  Top " + off::pctOf(b.r.pullMax[s], b.r.pullC[s]) + "  Traded first " + (b.r.pullTraded[s] ? "yes" : "no");
    o += std::string("  Before price ") + (justBefore(b.r, s) ? "yes" : "no");
    if (b.let[s].find('L') != std::string::npos) o += "  Large " + std::to_string(b.r.pullMax[s]);
    return o;
}
inline std::string plural(long long n, const char* one) { return std::to_string(n) + " " + one + (n == 1 ? "" : "s"); }
inline nl::Msg message(const off::PaneBar& b, const off::Params& P, int* sideOut = nullptr, int* kindOut = nullptr) {
    nl::Msg m; if (sideOut) *sideOut = -1; if (kindOut) *kindOut = 0;
    if (!b.ok) { m.line1 = "No order data for this candle"; return m; }
    double sc[2] = {0, 0};
    for (int s = 0; s < 2; ++s) if (off::high(b.m[0], s)) sc[s] = b.m[0].ratioOk[s] ? b.m[0].ratio[s] : off::HIGH_X;
    int side = -1;
    if (sc[BID] > 0 || sc[ASK] > 0) {
        if (sc[BID] > sc[ASK]) side = BID; else if (sc[ASK] > sc[BID]) side = ASK;
        else side = b.r.pullC[BID] > b.r.pullC[ASK] ? BID : b.r.pullC[ASK] > b.r.pullC[BID] ? ASK : -1;
    }
    const int first = side >= 0 ? side : (b.r.pullC[ASK] > b.r.pullC[BID] ? ASK : BID);
    m.line2 = sideNums(b, first) + "   |   " + sideNums(b, 1 - first) + (b.r.partial ? "   (partial candle)" : "");
    if (side < 0) { m.line1 = sc[BID] > 0 ? "Both sides pulling evenly (" + off::ratioText(sc[BID]) + ")" : "Normal activity - no spike"; return m; }
    const std::string at = b.level.empty() ? std::string() : " at " + b.level;
    const bool bid = side == BID;
    const std::string head = std::string(bid ? "Buyers" : "Sellers") + " pulled " + plural(b.r.pullN[side], bid ? "bid" : "offer") + " totaling "
        + std::to_string(b.r.pullC[side]) + " lots (" + off::ratioOr(b.m[0].ratioOk[side], b.m[0].ratio[side]) + ")"
        + off::paceWords(b.r.paceNote[side], b.r.pace100[side], b.r.paceGate);
    if (sideOut) *sideOut = side;
    if (baitGone(b.r, side, P)) {
        m.line1 = head + " - one " + std::to_string(b.r.pullMax[side]) + "-lot pulled before it traded - bait gone" + at + ".";
        m.impl = nl::NEUTRAL; if (kindOut) *kindOut = 2;
        return m;
    }
    m.line1 = head + (bid ? " - support leaving" : " - resistance leaving") + at + ".";
    m.impl = bid ? nl::BEAR : nl::BULL; if (kindOut) *kindOut = 1;
    return m;
}

// the info box (hover): Bids (green) | Offers (red)
inline off::Info info(const off::PaneBar* b, const off::Params& P) {
    off::Info I; I.colHead[0] = "Bids"; I.colHead[1] = "Offers"; I.tmpl = off::CROWD_TMPL;
    // (1.1.1, Rassul 13 Oct "just do it") row order Pulled, Before price, Traded first, Pace, Crowd (orders \xB7 avg \xB7 top share)
    const char* labels[5] = { "Pulled", "Before price", "Traded first", "Pace", "Crowd" };
    for (int k = 0; k < 5; ++k) {
        off::InfoRow r; r.label = labels[k];
        for (int s = 0; s < 2; ++s) {
            if (!b || !b->ok) { r.v[s] = "n/a"; continue; }
            const off::Rec& x = b->r;
            if (k == 0) r.v[s] = std::to_string(x.pullC[s]) + " (" + off::ratioOr(b->m[0].ratioOk[s], b->m[0].ratio[s]) + ")";
            else if (k == 1) r.v[s] = x.pullN[s] > 0 ? (justBefore(x, s) ? "yes" : "no") : "-";
            else if (k == 2) r.v[s] = x.pullN[s] > 0 && x.pullMax[s] > 0 ? (x.pullTraded[s] ? "yes" : "no") : "-";
            else if (k == 3) r.v[s] = off::paceHover(x.pace100[s], P.paceFastT, x.paceGate);
            else r.v[s] = off::crowdText(x.pullN[s], x.pullC[s], x.pullMax[s]);
        }
        I.rows.push_back(r);
    }
    return I;
}

struct Logic {
    static const char* name() { return "PullStats"; }
    static const char* version() { return VERSION; }
    static int metrics() { return 1; }
    static int paceMetric() { return off::PM_PULL; }
    static int optGate() { return ofo::G_OPULL; }
    static std::string optLine(const ofo::Snap& s, int gate, const std::string&) { return ofo::pullLine(s, gate); }
    static bool resets() { return true; }
    static off::Info info(const off::PaneBar* b, const off::Params& P) { return ps::info(b, P); }
    static long long value(const off::Rec& r, int, int s) { return r.pullC[s]; }          // (1.1.0) lots pulled
    // a closed candle's display, decided once (live = the forming candle, recomputed every draw, never recorded)
    static void decide(off::PaneBar& b, const off::Params& P, bool) {
        for (int s = 0; s < 2; ++s) { b.spike[s] = b.ok && b.m[0].spike[s]; b.dash[s] = -1; b.let[s] = b.ok ? letters(b.r, s) : std::string(); }
        b.msg = message(b, P, &b.msgSide, &b.kind);
    }
};

} // namespace ps
#endif
