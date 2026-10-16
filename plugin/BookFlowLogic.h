#ifndef LS_BOOKFLOW_LOGIC_H
#define LS_BOOKFLOW_LOGIC_H
// BookFlowLogic.h (1.0.0, 2026-10-16) -- SDK-free rules of lsBookFlow: who is ARRIVING and who is LEAVING, in ONE pane.
// Rassul 2026-10-16 07:45-07:49 CT: "give me a design that combines the pulling and stacking indicator" ... "1 net bar. make the zero line
// gray so i can clearly see the separation. dont keep the old panes, replace them. lets build this" (mockup: Book Flow pane).
//
// Per candle, per side (bids green, offers red):  NET = lots ADDED near price - lots PULLED near price.
//   above the grey zero line = size arriving (stacking), below = size leaving (pulling). One bar per side.
//   bright bar = a spike of the bigger part (added when the net is up, pulled when it is down), same spike rule as the old panes
//   (>= 2.0x that side's average AND >= 1.5x the biggest bar of either side in the prior 10 candles; the same-slot yardstick).
//   spike label (3 rows): ratio / pace / lots - the lots of the part that spiked (added when up, pulled when down); the pace only for added.
//   Letters and the refill dash are not drawn (they stay in the hover and the records).
// The read line (top) is the louder of the two old reads: StackStats' message ("Buyers added 42 bids totaling 150 lots (4.4x) ...") or
// PullStats' ("Sellers pulled 38 offers totaling 95 lots (3.6x) ... resistance leaving"); when both sides of the book spoke the same way
// ("bids stacked" and "offers pulled") the quieter one is added as "Also: ...".
// Everything the Tape Reader and the nightly studies read keeps being written in the OLD formats: the same session records as
// StackStats\ and PullStats\ (built from the same numbers, same message words), plus the PullStats zone-read record. BookFlow\ holds its own.
#include "OrderFlowFeed.h"
#include "StackStatsLogic.h"
#include "PullStatsLogic.h"

namespace bf {

static const char* const VERSION = "1.0.0";
using off::BID; using off::ASK;

inline long long netOf(const off::Rec& r, int s) { return r.addC[s] - r.pullC[s]; }
inline std::string signedLots(long long v) { return std::string(v < 0 ? "-" : "+") + std::to_string(v < 0 ? -v : v); }

// the two old panes' views of one BookFlow candle: metric 0 = the bars the old pane drew, metric 1 = its refills
inline off::PaneBar stackView(const off::PaneBar& b) { off::PaneBar k = b; k.m[0] = b.m[0]; k.m[1] = b.m[2]; return k; }
inline off::PaneBar pullView(const off::PaneBar& b) {
    off::PaneBar k = b; k.m[0] = b.m[1];
    for (int s = 0; s < 2; ++s) { k.r.pace100[s] = 0; }            // the stored pace is the ADDED pace: the pull view carries none
    return k;
}
// how loud a side spoke in one metric (0 = not at all)
inline double loud(const off::MStat& m, int s) { return off::high(m, s) ? (m.ratioOk[s] ? m.ratio[s] : off::HIGH_X) : 0.0; }
inline double loudest(const off::MStat& m) { return std::max(loud(m, BID), loud(m, ASK)); }

inline std::string netNums(const off::PaneBar& b) {
    std::string o;
    for (int s = 0; s < 2; ++s) {
        const long long a = b.r.addC[s], p = b.r.pullC[s];
        if (s == 1) o += "   |   ";
        o += std::string(s == BID ? "Bids " : "Offers ") + signedLots(a - p) + " (added " + std::to_string(a) + ", pulled " + std::to_string(p) + ")";
    }
    if (b.r.partial) o += "   (partial candle)";
    return o;
}
inline nl::Msg message(const off::PaneBar& b, const off::Params& P, int* sideOut = nullptr, int* kindOut = nullptr) {
    nl::Msg m; if (sideOut) *sideOut = -1; if (kindOut) *kindOut = 0;
    if (!b.ok) { m.line1 = "No order data for this candle"; return m; }
    const off::PaneBar sk = stackView(b), pk = pullView(b);
    int ss = -1, pp = -1, pkind = 0;
    const nl::Msg ms = st::message(sk, P, &ss), mp = ps::message(pk, P, &pp, &pkind);
    const double ls = loudest(sk.m[0]), lp = loudest(pk.m[0]);
    m.line2 = netNums(b);
    if (ls <= 0 && lp <= 0) { m.line1 = "Normal activity - no spike"; return m; }
    const bool usePull = lp > ls;
    const nl::Msg& pick = usePull ? mp : ms;
    m.line1 = pick.line1; m.impl = pick.impl;
    if (sideOut) *sideOut = usePull ? pp : ss;
    if (kindOut) *kindOut = usePull ? pkind : 0;
    // the quieter read, when it points the same way and is a spike too: "Also: sellers pulled 388 lots (3.0x)."
    const nl::Msg& other = usePull ? ms : mp; const int os = usePull ? ss : pp;
    if (os >= 0 && (usePull ? ls : lp) > 0 && other.impl == pick.impl && other.impl != nl::NEUTRAL) {
        const off::PaneBar& ob = usePull ? sk : pk;
        const long long lots = usePull ? ob.r.addC[os] : ob.r.pullC[os];
        m.line1 += std::string(" Also: ") + (os == BID ? "buyers " : "sellers ") + (usePull ? "added " : "pulled ") + std::to_string(lots)
                 + " lots (" + off::ratioOr(ob.m[0].ratioOk[os], ob.m[0].ratio[os]) + ").";
    }
    return m;
}

// the info box rows (Bids | Offers): Net, Added, Pulled, Rebuilt, Pace, Crowd
inline off::Info info(const off::PaneBar* b, const off::Params& P) {
    off::Info I; I.colHead[0] = "Bids"; I.colHead[1] = "Offers"; I.tmpl = off::CROWD_TMPL;
    const char* labels[6] = { "Net", "Added", "Pulled", "Rebuilt", "Pace", "Crowd" };
    for (int k = 0; k < 6; ++k) {
        off::InfoRow r; r.label = labels[k];
        for (int s = 0; s < 2; ++s) {
            if (!b || !b->ok) { r.v[s] = "n/a"; continue; }
            const off::Rec& x = b->r;
            if (k == 0) r.v[s] = signedLots(netOf(x, s));
            else if (k == 1) r.v[s] = std::to_string(x.addC[s]) + " (" + off::ratioOr(b->m[0].ratioOk[s], b->m[0].ratio[s]) + ")";
            else if (k == 2) r.v[s] = std::to_string(x.pullC[s]) + " (" + off::ratioOr(b->m[1].ratioOk[s], b->m[1].ratio[s]) + ")";
            else if (k == 3) { const long long rb = off::rebuiltPctOf(x.rebuildC[s], x.hitC[s]); r.v[s] = rb < 0 ? std::string("-") : std::to_string(rb) + "%" + (P.gate[ofo::G_REBUILT] == ofo::GATE_ON ? "" : " ?"); }
            else if (k == 4) r.v[s] = off::paceHover(x.pace100[s], P.paceFastT, x.paceGate);
            else r.v[s] = off::crowdText(x.addN[s], x.addC[s], x.addTop[s]);
        }
        I.rows.push_back(r);
    }
    return I;
}

struct Logic {
    static const char* name() { return "BookFlow"; }
    static const char* title() { return "Book Flow - Who is arriving or leaving"; }
    static const char* version() { return VERSION; }
    static int metrics() { return 3; }                                   // 0 added lots, 1 pulled lots, 2 refills (count)
    static int paceMetric() { return off::PM_ADD; }
    static int optGate() { return ofo::G_OSTACK; }
    static std::string optLine(const ofo::Snap& s, int gate, const std::string&) { return ofo::stackLine(s, gate); }
    static bool resets() { return false; }
    static std::string hoverQuestion() { return "Who arrived or left?"; }
    static const char* const* hoverVerbs() { static const char* const v[2] = { "Buyers added or pulled bids", "Sellers added or pulled offers" }; return v; }
    static off::Info info(const off::PaneBar* b, const off::Params& P) { return bf::info(b, P); }
    static long long value(const off::Rec& r, int metric, int s) { return metric == 0 ? r.addC[s] : metric == 1 ? r.pullC[s] : r.refN[s]; }
    static void decide(off::PaneBar& b, const off::Params& P, bool) {
        for (int s = 0; s < 2; ++s) {
            b.spike[s] = b.ok && (netOf(b.r, s) >= 0 ? b.m[0].spike[s] : b.m[1].spike[s]);
            b.dash[s] = -1; b.let[s].clear();
        }
        b.msg = message(b, P, &b.msgSide, &b.kind);
    }
};

} // namespace bf
#endif
