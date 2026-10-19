#ifndef LS_BOOKFLOW_LOGIC_H
#define LS_BOOKFLOW_LOGIC_H
// BookFlowLogic.h (1.3.0 2026-10-18; 1.0.0, 2026-10-16) -- SDK-free rules of lsBookFlow: who is ARRIVING and who is LEAVING, in ONE pane.
// Rassul 2026-10-16 07:45-07:49 CT: "give me a design that combines the pulling and stacking indicator" ... "1 net bar. make the zero line
// gray so i can clearly see the separation. dont keep the old panes, replace them. lets build this" (mockup: Book Flow pane).
//
// Per candle, per side:  NET = lots ADDED near price - lots PULLED near price.
//   above the grey zero line = size arriving (stacking), below = size leaving (pulling). One bar per side.
//   (1.3.0, Rassul 2026-10-18) colour BY SIDE: bid-side bars green (buyers), offer-side bars red (sellers), up or down - the hover explains
//   the meaning. BRIGHT + label = THE spike rule of both order-flow panes, on the NET that is drawn: |net| >= "Spike vs average" (2.0x) the
//   plain average of that side's |net| over the prior "Ratio bars" (20) closed candles (floored at 1 lot), decided once at the close; the
//   forming candle is drawn dim with no label. Label: the ratio (yellow) and, for size arriving, the pace (grey, no "p") outside the bar's end.
//   (Was: the added-only or pulled-only part's spike with the 1.5x jump - a +900 / -880 candle drew a tiny +20 bar bright.)
//   Letters and the refill dash are not drawn (they stay in the records).
// The candle message (records only, not drawn) is the louder of the two old reads: StackStats' ("Buyers added 42 bids totaling 150 lots
// (4.4x) ...") or PullStats' ("Sellers pulled 38 offers ... resistance leaving"), "Also: ..." when the other agrees.
// Everything the Tape Reader and the nightly studies read keeps being written in the OLD formats: the same session records as
// StackStats\ and PullStats\ (built from the same numbers, same message words), plus the PullStats zone-read record. BookFlow\ holds its own.
#include "OrderFlowFeed.h"
#include "StackStatsLogic.h"
#include "PullStatsLogic.h"
#include <algorithm>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace bf {

static const char* const VERSION = "1.3.4";   // (1.3.4) box follows the mouse only inside the pane, else Live = the forming candle; // (1.3.3, 2026-10-18 his decision list) window: Position + Near ticks (Offset X / Y and Record removed - recording always on; Ratio bars / Spike hidden);  (1.3.2, 2026-10-18 settings audit) the window from OrderFlowPaneSettings.h through SettingsHost.h (same rows / file); (1.3.1, 2026-10-18 settings audit) Spike vs average on its own settings line (was cut off); rows / file unchanged // keep equal to BF_VERSION / setVersion in BookFlow.cpp (1.3.0: net spike, colour by side, Market State BOOK; 1.2.1 box Net + Pace)
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
struct NetStat;
inline off::Info info(const off::PaneBar* b, const off::Params& P, const NetStat* ns);
inline off::Info info(const off::PaneBar* b, const off::Params& P) { return info(b, P, nullptr); }
inline std::string netCell(const off::PaneBar& b, int s, const NetStat* ns);
inline off::Info info(const off::PaneBar* b, const off::Params& P, const NetStat* ns) {
    off::Info I; I.colHead[0] = "Bids"; I.colHead[1] = "Offers"; I.tmpl = off::CROWD_TMPL;
    const char* labels[6] = { "Net", "Added", "Pulled", "Rebuilt", "Pace", "Crowd" };
    for (int k = 0; k < 6; ++k) {
        off::InfoRow r; r.label = labels[k];
        for (int s = 0; s < 2; ++s) {
            if (!b || !b->ok) { r.v[s] = "n/a"; continue; }
            const off::Rec& x = b->r;
            if (k == 0) r.v[s] = netCell(*b, s, ns);                          // (1.3.0) "+120 (2.4x)": the net and its ratio
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

// =====================================================================================================================
// (1.3.0, 2026-10-18) THE NET SPIKE, what is drawn, and the Market State facts (SRC "BOOK", MARKET-STATE-SPEC.md 1.0)
// =====================================================================================================================
struct NetStat {
    bool ok = false;                                                       // decided (a closed candle with order data)
    long long net[2] = {0, 0};
    bool ratioOk[2] = {false, false}; double ratio[2] = {0, 0}, mean[2] = {0, 0}; bool spike[2] = {false, false};
};
// |net| of one side
inline long long absNet(const off::Rec& r, int s) { const long long v = netOf(r, s); return v < 0 ? -v : v; }
// the decision from the prior |net| values (oldest first): ratio vs their plain mean over the last ratioBars (>= 5 needed, mean floored at
// 1 lot as the other order-feed counts), spike = net != 0 and 10 |net| >= avgT x mean (avgT tenths, 0 = off). A partial candle (the one the
// order feed opened in) is shown but never a spike.
inline NetStat netStat(const std::vector<long long>& pb, const std::vector<long long>& pa, const off::Rec& r, int ratioBars, long long avgT) {
    NetStat n; n.ok = true;
    for (int s = 0; s < 2; ++s) {
        n.net[s] = netOf(r, s);
        const long long a = n.net[s] < 0 ? -n.net[s] : n.net[s];
        n.ratio[s] = off::ratioOf(s == BID ? pb : pa, a, ratioBars, n.ratioOk[s], &n.mean[s], 1.0);
        n.spike[s] = !r.partial && avgT > 0 && a > 0 && n.ratioOk[s] && (double)a * 10.0 >= (double)avgT * n.mean[s];
    }
    return n;
}
// the prior |net| values before t from the decided candles (valid, not partial), oldest first, at most `need`
inline void priorAbsNet(const std::map<long long, off::PaneBar>& bars, long long t, size_t need, std::vector<long long>& pb, std::vector<long long>& pa) {
    pb.clear(); pa.clear();
    std::map<long long, off::PaneBar>::const_iterator it = bars.lower_bound(t);
    while (it != bars.begin() && pb.size() < need) {
        --it;
        if (it->second.ok && !it->second.r.partial) { pb.push_back(absNet(it->second.r, BID)); pa.push_back(absNet(it->second.r, ASK)); }
    }
    std::reverse(pb.begin(), pb.end()); std::reverse(pa.begin(), pa.end());
}
inline NetStat netStatAt(const std::map<long long, off::PaneBar>& bars, long long t, int ratioBars, long long avgT) {
    NetStat n; std::map<long long, off::PaneBar>::const_iterator me = bars.find(t);
    if (me == bars.end() || !me->second.ok) return n;
    std::vector<long long> pb, pa; priorAbsNet(bars, t, (size_t)(ratioBars > off::RATIO_HI ? off::RATIO_HI : (ratioBars < off::RATIO_LO ? off::RATIO_LO : ratioBars)), pb, pa);
    return netStat(pb, pa, me->second.r, ratioBars, avgT);
}
inline std::string netCell(const off::PaneBar& b, int s, const NetStat* ns) {
    std::string o = signedLots(netOf(b.r, s));
    if (ns && ns->ok && ns->ratioOk[s]) o += " (" + off::ratioText(ns->ratio[s]) + ")";
    return o;
}
// one side of one candle as drawn. closed = a later bar exists (only closed candles are judged). col = 0 green (bid side, buyers),
// 1 red (offer side, sellers): BY SIDE, never by meaning. ratio / pace = the label rows ("" = none); the pace only for size arriving.
inline std::string dropX(std::string t) { if (!t.empty() && t[t.size() - 1] == 'x') t.erase(t.size() - 1); return t; }   // "2.4x" -> "2.4"
struct NetLook { long long v = 0; bool bright = false; int col = 0; std::string ratio, pace; };
inline NetLook netLookOf(const off::PaneBar* b, const NetStat* ns, int s, bool closed) {
    NetLook k; k.col = s == BID ? 0 : 1;
    if (!b || !b->ok) return k;
    k.v = netOf(b->r, s);
    if (!closed || !ns || !ns->ok || !ns->spike[s]) return k;
    k.bright = true;
    k.ratio = ns->ratioOk[s] ? dropX(off::ratioText(ns->ratio[s])) : std::string();
    k.pace = k.v > 0 && b->r.pace100[s] > 0 ? dropX(off::paceX(b->r.pace100[s])) : std::string();
    return k;
}
inline std::string fmt1(double x) { if (!(x == x) || x < 0) return "?"; char t[24]; std::snprintf(t, sizeof(t), "%.1f", x > 9999 ? 9999.0 : x); return t; }
// SRC BOOK: bid_net / ask_net (added - pulled, lots), bid_net_x / ask_net_x (|net| vs its plain average), bid_state / ask_state
// (arriving = net > 0, leaving = net < 0, flat = 0), spike (the side + direction of the spike; both = the bigger ratio), rebuilt_pct
// (both sides: lots re-added at the hit prices / lots hit, "?" with no hits)
inline std::map<std::string, std::string> bookFacts(const off::PaneBar& b, const NetStat& ns) {
    std::map<std::string, std::string> kv;
    const char* nm[2] = { "bid", "ask" };
    if (!b.ok || !ns.ok) {
        for (int s = 0; s < 2; ++s) { kv[std::string(nm[s]) + "_net"] = "?"; kv[std::string(nm[s]) + "_net_x"] = "?"; kv[std::string(nm[s]) + "_state"] = "?"; }
        kv["spike"] = "?"; kv["rebuilt_pct"] = "?"; return kv;
    }
    int sp = -1;
    for (int s = 0; s < 2; ++s) {
        const std::string n = nm[s];
        kv[n + "_net"] = std::to_string(ns.net[s]);
        kv[n + "_net_x"] = ns.ratioOk[s] ? fmt1(ns.ratio[s]) : "?";
        kv[n + "_state"] = ns.net[s] > 0 ? "arriving" : ns.net[s] < 0 ? "leaving" : "flat";
        if (ns.spike[s] && (sp < 0 || ns.ratio[s] > ns.ratio[sp])) sp = s;
    }
    kv["spike"] = sp < 0 ? "none" : std::string(nm[sp]) + (ns.net[sp] > 0 ? "_arriving" : "_leaving");
    const long long rb = off::rebuiltPctOf(b.r.rebuildC[0] + b.r.rebuildC[1], b.r.hitC[0] + b.r.hitC[1]);
    kv["rebuilt_pct"] = rb < 0 ? "?" : std::to_string(rb);
    return kv;
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
            b.spike[s] = false;                                            // (1.3.0) the NET spike is decided by the pane at the close (netStatAt: it needs the series)
            b.dash[s] = -1; b.let[s].clear();
        }
        b.msg = message(b, P, &b.msgSide, &b.kind);
    }
};

} // namespace bf
#endif
