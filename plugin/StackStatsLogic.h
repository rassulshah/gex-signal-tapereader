#ifndef LS_STACKSTATS_LOGIC_H
#define LS_STACKSTATS_LOGIC_H
// StackStatsLogic.h (1.1.0, 2026-10-13) -- SDK-free rules of lsStackStats: who is ARRIVING. Unit-tested on Linux.
// 1.1.0 (2026-10-13, approved by Rassul 4:49 PM CT "build"; SPEC /home/claude/aud/of140/SPEC.md, mockup orderflow-contracts-mockup.png):
//   Bars = LOTS added near price (were order counts); spike label ratio / pace / lots; PACE (added lots / s vs the same-time norm);
//   the message grammar of the SPEC (exact):
//     "Buyers added 42 bids totaling 150 lots (4.4x) fast (p 3.4x) - rebuilt 90% after hits - defending PDL."     green
//     "Sellers added 3 offers totaling 140 lots (3.9x) - one order is 85% of it - may not reload ? at PDH."        white
//   (6:40 PM CT coordinator / book-makeup study: one-order zones were pulled LESS and filled MORE, so "may be bait" is not supported;
//    the clause is now ONE_ORDER_TAIL below - change the words there, never in the logic. PullStats "bait gone" is unchanged.)
//     "Buyers added 18 bids totaling 60 lots (2.2x) slowly (p 0.7x) - support building at PDL."                    green
//     "Sellers added 51 offers totaling 210 lots (3.3x) fast (p 2.9x) - wall building above at PDH."               red
//   REBUILT % (lra passive_aggr's restore, PA100): the lots re-added at a price after it was hit / the lots hit there, this candle;
//   in the message from rebuiltPct (90%), '?' until REBUILT_SHOW,on. Refills (I) keep their count / ratio (hover "Refills (I)").
//   Hover rows (Bids | Offers): Added lots (ratio), Pace x (threshold), Orders N · avg X lots, Top order NN%, Rebuilt NN%, Refills (I).
//   Letters unchanged: I (notable refills + an iceberg confirmed at a refill price), L (one order added >= largeLotResting).
//   Dash: inside the side's bar at min(refilled lots, added lots) when the refills are notable.
// 1.0.0 / 1.0.1 (2026-10-13): Rassul "add a stacking indicator" / "keep seperate" / "include L ..." / "use a dash in the bar"; I (was R).
#include "OrderFlowFeed.h"

namespace st {

static const char* const VERSION = "1.1.1";   // 1.1.1 (2026-10-13 9:40 PM): rebuilt % <= 100%, row order, Crowd row, 10 pt labels, chart-wide hover; 1.1.0 (2026-10-13): lots, pace, rebuilt %, SPEC message grammar, hover rows; 1.0.1: letter I
using off::BID; using off::ASK;
// the words after "one order is NN% of it - " (white / neutral). The Tape Reader keys on the same text (TapeReaderLogic.h tr::STACK_ONE_ORDER,
// checked equal by trtests/test_tr_ofcompat.cpp) and still recognises the old "may be bait" in records written before 6:40 PM 13 Oct.
static const char* const ONE_ORDER_TAIL = "may not reload ?";

inline bool notable(const off::PaneBar& b, int s, const off::Params& P) {
    return b.ok && b.r.refN[s] > 0 && b.m[1].ratioOk[s] && b.m[1].ratio[s] * 10.0 >= (double)P.refillNotableT - 1e-9;
}
inline std::string letters(const off::PaneBar& b, int s, const off::Params& P) {
    std::string o;
    if (notable(b, s, P) && b.r.ice[s] > 0) o += "I";   // I = iceberg / refill (1.0.1; was R)
    if (b.r.thrRest > 0 && b.r.addMax[s] >= b.r.thrRest) o += "L";
    return o;
}
// "1 order = 12%  avg 3.1 cts" ("" without contracts) - kept for the record's numbers line
inline std::string concText(long long top, long long cts, long long n) {
    if (cts <= 0 || top <= 0) return std::string();
    const long long pct = (top * 100 + cts / 2) / cts;
    std::string s = "1 order = " + std::to_string(pct > 100 ? 100 : pct) + "%";
    if (n > 0) { char b[32]; std::snprintf(b, sizeof(b), "  avg %.1f cts", (double)cts / (double)n); s += b; }
    return s;
}
inline long long rebuilt(const off::Rec& r, int s) { return off::rebuiltPctOf(r.rebuildC[s], r.hitC[s]); }
// the record's numbers line (the Tape Reader reads "BidStack <lots> (<ratio>)" / "BidRefill <n> (<ratio>)"):
// "BidStack 150 (4.4x)  BidPace 3.4x  42 orders  BidRefill 44 (5.2x)  1 order = 12%  avg 3.6 cts  Rebuilt 90%   |   AskStack 12 (0.5x) ..."
inline std::string sideNums(const off::PaneBar& b, int s, bool full) {
    const char* S = s == BID ? "Bid" : "Ask";
    std::string o = std::string(S) + "Stack " + std::to_string(b.r.addC[s]) + " (" + off::ratioOr(b.m[0].ratioOk[s], b.m[0].ratio[s]) + ")";
    if (b.r.pace100[s] > 0) o += "  " + std::string(S) + "Pace " + off::paceX(b.r.pace100[s]);
    o += "  " + std::to_string(b.r.addN[s]) + (b.r.addN[s] == 1 ? " order" : " orders");
    if (full) {
        o += "  " + std::string(S) + "Refill " + std::to_string(b.r.refN[s]) + " (" + off::ratioOr(b.m[1].ratioOk[s], b.m[1].ratio[s]) + ")";
        const std::string c = concText(b.r.addTop[s], b.r.addC[s], b.r.addN[s]); if (!c.empty()) o += "  " + c;
        const long long rb = rebuilt(b.r, s); if (rb >= 0) o += "  Rebuilt " + std::to_string(rb) + "%";
    }
    if (b.let[s].find('L') != std::string::npos) o += "  Large " + std::to_string(b.r.addMax[s]);
    return o;
}
inline std::string plural(long long n, const char* one) { return std::to_string(n) + " " + one + (n == 1 ? "" : "s"); }
inline nl::Msg message(const off::PaneBar& b, const off::Params& P, int* sideOut = nullptr) {
    nl::Msg m; if (sideOut) *sideOut = -1;
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
        else side = b.r.addC[BID] > b.r.addC[ASK] ? BID : b.r.addC[ASK] > b.r.addC[BID] ? ASK : -1;
    }
    const int first = side >= 0 ? side : (b.r.addC[ASK] > b.r.addC[BID] ? ASK : BID);
    m.line2 = sideNums(b, first, true) + "   |   " + sideNums(b, 1 - first, false) + (b.r.partial ? "   (partial candle)" : "");
    if (side < 0) { m.line1 = sc[BID] > 0 ? "Both sides adding evenly (" + off::ratioText(sc[BID]) + ")" : "Normal activity - no spike"; return m; }
    if (sideOut) *sideOut = side;
    const bool bid = side == BID;
    const std::string at = b.level.empty() ? std::string() : " at " + b.level;
    const std::string who = bid ? "Buyers" : "Sellers";
    if (!hA[side]) {                                                       // the refills alone are notable
        m.line1 = who + " reloaded " + std::string(bid ? "bids " : "offers ") + std::to_string(b.r.refN[side]) + " times (" + off::ratioOr(b.m[1].ratioOk[side], b.m[1].ratio[side]) + ") after hits"
                + (b.r.ice[side] > 0 ? ", iceberg" : "") + (bid ? " - hidden support" : " - hidden wall above") + at + ".";
        m.impl = bid ? nl::BULL : nl::BEAR;
        return m;
    }
    const std::string head = who + " added " + plural(b.r.addN[side], bid ? "bid" : "offer") + " totaling " + std::to_string(b.r.addC[side]) + " lots ("
        + off::ratioOr(b.m[0].ratioOk[side], b.m[0].ratio[side]) + ")" + off::paceWords(b.r.paceNote[side], b.r.pace100[side], b.r.paceGate);
    const long long top = b.r.addTop[side], cts = b.r.addC[side];
    if (cts > 0 && top * 100 >= P.baitPct * cts) {                         // mostly one order: may not reload ? (neutral)
        m.line1 = head + " - one order is " + std::to_string(std::min(100LL, (top * 100 + cts / 2) / cts)) + "% of it - " + ONE_ORDER_TAIL + at + ".";
        m.impl = nl::NEUTRAL;
        return m;
    }
    const long long rb = rebuilt(b.r, side); const int rg = P.gate[ofo::G_REBUILT];
    if (rb >= 0 && rb >= P.rebuiltPct && rg != ofo::GATE_OFF) {
        m.line1 = head + " - rebuilt " + std::to_string(rb) + "% after hits" + (rg == ofo::GATE_ON ? "" : " ?")
                + (b.level.empty() ? (bid ? " - real support" : " - real wall above") : " - defending " + b.level) + ".";
    } else m.line1 = head + (bid ? " - support building" : " - wall building above") + at + ".";
    m.impl = bid ? nl::BULL : nl::BEAR;
    return m;
}

// the info box (hover): Bids (green) | Offers (red)
inline off::Info info(const off::PaneBar* b, const off::Params& P) {
    off::Info I; I.colHead[0] = "Bids"; I.colHead[1] = "Offers"; I.tmpl = off::CROWD_TMPL;
    // (1.1.1, Rassul 13 Oct "just do it") row order Added, Rebuilt, Pace, Crowd (orders \xB7 avg \xB7 top share), Refills (I)
    const char* labels[5] = { "Added", "Rebuilt", "Pace", "Crowd", "Refills (I)" };
    for (int k = 0; k < 5; ++k) {
        off::InfoRow r; r.label = labels[k];
        for (int s = 0; s < 2; ++s) {
            if (!b || !b->ok) { r.v[s] = "n/a"; continue; }
            const off::Rec& x = b->r;
            if (k == 0) r.v[s] = std::to_string(x.addC[s]) + " (" + off::ratioOr(b->m[0].ratioOk[s], b->m[0].ratio[s]) + ")";
            else if (k == 1) { const long long rb = rebuilt(x, s); r.v[s] = rb < 0 ? std::string("-") : std::to_string(rb) + "%" + (P.gate[ofo::G_REBUILT] == ofo::GATE_ON ? "" : " ?"); }
            else if (k == 2) r.v[s] = off::paceHover(x.pace100[s], P.paceFastT, x.paceGate);
            else if (k == 3) r.v[s] = off::crowdText(x.addN[s], x.addC[s], x.addTop[s]);
            else r.v[s] = std::to_string(x.refN[s]) + " (" + off::ratioOr(b->m[1].ratioOk[s], b->m[1].ratio[s]) + ")";
        }
        I.rows.push_back(r);
    }
    return I;
}

struct Logic {
    static const char* name() { return "StackStats"; }
    static const char* version() { return VERSION; }
    static int metrics() { return 2; }
    static int paceMetric() { return off::PM_ADD; }
    static int optGate() { return ofo::G_OSTACK; }
    static std::string optLine(const ofo::Snap& s, int gate, const std::string&) { return ofo::stackLine(s, gate); }
    static bool resets() { return false; }
    static off::Info info(const off::PaneBar* b, const off::Params& P) { return st::info(b, P); }
    static long long value(const off::Rec& r, int metric, int s) { return metric == 0 ? r.addC[s] : r.refN[s]; }   // (1.1.0) lots added; refills = count
    static void decide(off::PaneBar& b, const off::Params& P, bool) {
        for (int s = 0; s < 2; ++s) {
            b.spike[s] = b.ok && b.m[0].spike[s];
            b.dash[s] = notable(b, s, P) ? (b.r.refC[s] < b.r.addC[s] ? b.r.refC[s] : b.r.addC[s]) : -1;   // min(refilled lots, added lots)
            b.let[s] = b.ok ? letters(b, s, P) : std::string();
        }
        b.msg = message(b, P, &b.msgSide);
    }
};

} // namespace st
#endif
