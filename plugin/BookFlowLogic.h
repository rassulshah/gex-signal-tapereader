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
#include "BoxRead.h"
#include <algorithm>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace bf {

static const char* const QUESTION = "Are they defending?";
// (1.10.0, Rassul 23:23 / 23:29, info-icon-mockup.png) the info icon's hover text
static const char* const INFO_TIP =
"Defense - Are they defending?\n"
"Shows whether resting buyers (bids) and sellers (offers) near price are adding or\n"
"pulling size, and whether they re-post what gets hit - the passive side holding the line.\n"
"\n"
"How it is calculated (each candle, each side, near price)\n"
"  Refill Effort: lots added - lots pulled, and its size vs the last 20 candles\n"
"  Refilled: lots re-posted at the prices that were hit / lots eaten, vs the\n"
"     market normal; STRONG / weak cut-offs per market\n"
"  Read: at a new 10-bar low / high, strong refill = defense holding ?,\n"
"     weak = defense failing ?\n"
"\n"
"Example - ES 8:18 PM (the low)\n"
"  Buyers added 6,266 and pulled 4,821 lots of bids = +1,445 net, 3.1x a normal candle.\n"
"  Sellers ate 772 lots from the bids; buyers re-posted 2,179 lots at those prices\n"
"  (2.8 per lot eaten) - the bids held and price closed green.";   // (1.9.0, Rassul 19:38; 1.8.0 was "Who is defending - and are they refilling?")
static const char* const VERSION = "1.17.0";   // (1.17.0, 2026-10-21 Rassul approved defender3-mockup-v3 11:50 CT + both sides 11:59) Top Defender #3: order MOVES (MoveLogic.h) - P (the defenders pressed in) in the badge row, box row Moved (per side In / Back, ticks, x normal), M lines for all four cases;   // (1.16.0, 2026-10-21 Rassul approved defender4-mockup-v1, L / X at 11:32 CT) Top Defender #4: the RE-TEST of each turn's R line (DefenseTurn 1.4.0 retestStep: left 0.5 ATR, back in the zone within 15 min; the defenders' resting lots there at the close before the return vs the market normal) -> L (reloaded) / X (empty) in the badge row ("A|H|L"), box row Re-test ("Reloaded 1.9x" / "Empty 0.2x"), Q lines in the turns file;   // (1.15.0, 2026-10-21 Rassul approved defender5-mockup-v2 11:25 CT "build this") Top Defender #5: H = the defenders hit back after an A (DefenseTurn 1.3.0 counterDecide, H lines in the turns file), the BADGE ROW at the top of the pane ("A|H?", one badge per candle, font fits the slot), the box row Counter ("Hit back 2.4x" / "Absorbed only" / "Watching");   // (1.14.1 (2026-10-20 evening, Rassul "implement" the audit): stricter turns-file parsing, stale live-line fix, WATCH waits for the full 20-60 s window, touch freshness / stale-event guards, incomplete order totals marked partial, invalid effort / reward inputs rejected; ) // (1.14.0, 2026-10-20 CT Rassul) the EARLY-WARNING read while the candle forms (09:59 "lets do it": WATCH / CAUTION, bf::earlyRead); percentages without "+" (14:28); a turn needs the close beyond the R line (DefenseTurn 1.2.0: above it at a low, below at a high; the forming "?" only while price is beyond it); the bars and the "+NN%" marks use the pane's y-axis (dragging the axis zooms the bars); // (1.13.0, 2026-10-20 13:54 CT Rassul "okay build") the gray exhaustion line decided per turn (touch rule, X lines in the turns file); // (1.12.0, 2026-10-20 12:22 CT Rassul "first build the lines, do it now") pivot turn marks over the defending bar (+NN%, forming "?"), the turns file for lsDefenseLines (DefenseTurn.h); // (1.11.0, 2026-10-20 07:44 CT Rassul "one long line, instead of a multiline box") the info icon description drawn as a multi-line box in the pane (IRT's help tip showed one line); // (1.10.0, 2026-10-19 23:29 CT Rassul "build", info-icon-mockup.png) question just right of the info box + info icon with the description / calculation / example on hover; the box width template restored ("1.4x STR..." was cut off); // (1.9.0, 2026-10-19 19:31 / 19:38 CT Rassul) title "Defense", question "Are they defending?", box columns Buyers | Sellers (was Bids | Offers); // (1.8.0, 2026-10-19 19:09 CT Rassul "build", side-by-side-question-mockup.png) PAIR bars - bids green left, offers red right, both above zero (negative net = a stub); the question line on top; the pull markers' strip removed (stats kept); the Reward row removed (Rassul 19:19: it repeats Aggression's Reward); // (1.7.0, 2026-10-19 18:28 CT) the same SCALE CAP as Aggression 1.17.0 (OrderFlowFeed capTop): 1.5x the 90th-percentile |net| on screen, capped bars with a white tip; // (1.6.0, 2026-10-19 17:42 CT Rassul "build it already" - Effort vs Reward) box row REWARD under Refilled: ticks the attack into each side moved price per 1,000 lots it took vs the market normal (EffortReward.h), "absorbed" at 0.5x or less, "through" at 2.0x or more; // (1.5.1, 2026-10-19 13:37 CT Rassul "a lot of black space .. get rid of the extra black space") the zero line sits where the data needs it: no negative net on screen = at the bottom (only the pull-marker strip under it), else the height split by size with one scale (OrderFlowFeed netGeoFor). (1.5.0, 2026-10-19 11:05 CT Rassul "build" on bookflow-refill-mockup.png + bookflow-box-questions-mockup.png; 11:07 "why are you removing pace" -> every row kept) (11:19 / 11:20 / 11:53 CT) box rows REFILL EFFORT (was Net) and REFILLED (re-posted at the hit prices / eaten, STRONG / weak by market) + the Read; Eaten, Added, Pulled, Pace hidden (computed, recorded, used by the Read); questions in the description, not the box; the bars' second label = Refilled ("R4.3") instead of the pace; the Read leads with the REFILL read at a new 10-bar high / low (refillRead);
// (1.4.0)   // (1.4.0, 2026-10-18 23:48 / 23:56 CT approved bookflow-read-mockup.png + bookflow-pull2-mockup.png) box rows Net, Added, Pulled, Pace, Read; pull markers (pullStat) (bookRead, BoxRead.h); the live candle's Net carries its ratio; // (1.3.4) box follows the mouse only inside the pane, else Live = the forming candle; // (1.3.3, 2026-10-18 his decision list) window: Position + Near ticks (Offset X / Y and Record removed - recording always on; Ratio bars / Spike hidden);  (1.3.2, 2026-10-18 settings audit) the window from OrderFlowPaneSettings.h through SettingsHost.h (same rows / file); (1.3.1, 2026-10-18 settings audit) Spike vs average on its own settings line (was cut off); rows / file unchanged // keep equal to BF_VERSION / setVersion in BookFlow.cpp (1.3.0: net spike, colour by side, Market State BOOK; 1.2.1 box Net + Pace)
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

// =====================================================================================================================
// (1.5.0, 2026-10-19, Rassul 10:31 "what about actual refill ... buyers ate 1000, refilled 1000, 100%"; test PULLING-COMBO-TEST.md 3)
// REFILL = lots re-posted at the exact price that was just hit, within the refill window (1 s; the feed's refill_cts) / lots hit there
// (hit_cts, "Eaten"). Not capped (2.0x = twice what was taken). STRONG = the market's top 25%, weak = its bottom 25% (Databento MBO
// 7 markets Oct 6-9; BookFlow's own ES record 10-19 gives the same quartiles 1.67 / 2.71). At a new 10-bar low / high:
//   strong refill turned 34% / 28% (base 20% / 19%), weak 10% / 8%; busy candle + defending net 2x + strong ("C+R") 53% / 46%.
// =====================================================================================================================
// (12:22 CT, Rassul "i will go with your recommendation") SHOWN AS A RATIO TO THE MARKET'S NORMAL (1.0x = that market's usual refill):
// normal = the market's median put-back / eaten (Databento MBO Oct 6-9, BookFlow's own ES record 10-19 agrees: 2.19); the cut-offs stay the
// tested raw quartiles (a ratio to the last 20 candles was tested too and lost most of the edge: strong 25% / 16% vs 34% / 28%).
struct RefillCut { const char* mkt; int strongT, weakT, normH; };       // tenths: strong >= strongT / 10, weak <= weakT / 10; normH = normal x 100
static const RefillCut REFILL_CUTS[] = { {"ES", 27, 17, 220}, {"NQ", 45, 19, 257}, {"CL", 23, 12, 180}, {"GC", 20, 12, 157}, {"HG", 10, 3, 60},
                                         {"NG", 15, 7, 108}, {"EU", 33, 13, 219}, {"6E", 33, 13, 219} };
inline const RefillCut* refillCutOf(const std::string& mkt) {
    for (size_t k = 0; k < sizeof(REFILL_CUTS) / sizeof(REFILL_CUTS[0]); ++k) if (mkt == REFILL_CUTS[k].mkt) return &REFILL_CUTS[k];
    return nullptr;
}
enum { RF_NA = -9, RF_WEAK = -1, RF_NORMAL = 0, RF_STRONG = 1 };
struct Refill { long long eaten = 0, put = 0; bool ok = false; double x = 0, rel = 0; bool relOk = false; int cls = RF_NA; };   // x = put / eaten; rel = x / the market's normal
inline Refill refillOf(const off::Rec& r, int s, const std::string& mkt) {
    Refill f; f.eaten = r.hitC[s] > 0 ? r.hitC[s] : 0; f.put = r.refC[s] > 0 ? r.refC[s] : 0;
    if (f.eaten <= 0) return f;
    f.ok = true; f.x = (double)f.put / (double)f.eaten;
    const RefillCut* c = refillCutOf(mkt);
    if (c) { f.cls = f.x * 10.0 >= c->strongT - 1e-9 ? RF_STRONG : (f.x * 10.0 <= c->weakT + 1e-9 ? RF_WEAK : RF_NORMAL);
             if (c->normH > 0) { f.rel = f.x * 100.0 / (double)c->normH; f.relOk = true; } }
    return f;
}
inline std::string refillCell(const off::Rec& r, int s, const std::string& mkt) {
    const Refill f = refillOf(r, s, mkt);
    if (!f.ok) return "-";
    if (!f.relOk) return "?";                                              // a market with no normal yet: nothing to compare with
    char t[24]; std::snprintf(t, sizeof(t), "%.1fx", f.rel > 99.9 ? 99.9 : f.rel);
    return std::string(t) + (f.cls == RF_STRONG ? " STRONG" : f.cls == RF_WEAK ? " weak" : "");
}
// where the shown candle sits: a new 10-bar low / high (its low <= the 9 candles before it), and how busy (volume vs the plain mean of
// the 20 before it); the host fills it from the chart's bars (lows / highs / volumes oldest first, the candle LAST)
struct Spot { bool newLow = false, newHigh = false, busy = false, ok = false; double volX = 0; };
inline Spot spotOf(const std::vector<double>& lo, const std::vector<double>& hi, const std::vector<long long>& vol) {
    Spot p; const size_t n = lo.size();
    if (n < 10 || hi.size() != n || vol.size() != n) return p;
    p.ok = true; const size_t i = n - 1;
    double mn = lo[i], mx = hi[i];
    for (size_t k = i - 9; k < i; ++k) { mn = std::min(mn, lo[k]); mx = std::max(mx, hi[k]); }
    p.newLow = lo[i] <= mn; p.newHigh = hi[i] >= mx;
    if (n >= 21) { double sum = 0; for (size_t k = i - 20; k < i; ++k) sum += (double)vol[k]; const double m = sum / 20.0;
                   if (m > 0) { p.volX = (double)vol[i] / m; p.busy = p.volX >= 2.0; } }
    return p;
}
// THE REFILL READ (only at a new 10-bar low / high; else none -> the stacking read): the defending side = bids at a low, offers at a high
// (both: the side that was hit more).
//   C+R  busy + defending net >= 2.0x (and > 0) + STRONG refill -> "C+R ? New low: sellers took 2,010, buyers refilled 2.0x normal - defense holding"
//   STRONG -> "New low: buyers refilled 1.4x normal (strong) - defense holding ?"
//   weak   -> "New low: buyers refilled only 0.5x normal (weak) - defense failing, don't fade ?"  (BEARISH at a low: the move tends to continue)
//   normal / no cut-offs for the market / no hits -> none
inline std::string commas(long long v) { std::string t = std::to_string(v < 0 ? -v : v); for (int k = (int)t.size() - 3; k > 0; k -= 3) t.insert((size_t)k, ","); return v < 0 ? "-" + t : t; }
// the info box rows (Bids | Offers): Refill Effort (the net), Eaten, Refilled, Added, Pulled, Rebuilt, Pace, Crowd
struct NetStat;
inline off::Info info(const off::PaneBar* b, const off::Params& P, const NetStat* ns, const std::string& mkt = std::string());
inline off::Info info(const off::PaneBar* b, const off::Params& P) { return info(b, P, nullptr); }
inline std::string netCell(const off::PaneBar& b, int s, const NetStat* ns);
inline off::Info info(const off::PaneBar* b, const off::Params& P, const NetStat* ns, const std::string& mkt) {
    off::Info I; I.colHead[0] = "Buyers"; I.colHead[1] = "Sellers"; I.tmpl = off::CROWD_TMPL;   // (1.9.0, Rassul 19:31) was Bids | Offers; (1.10.0) the width template restored (1.9.0 lost it in a comment: "1.4x STR..." was cut off)
    const char* labels[8] = { "Refill Effort", "Eaten", "Refilled", "Added", "Pulled", "Rebuilt", "Pace", "Crowd" };   // (1.5.0) Net -> Refill Effort; + Eaten, Refilled
    for (int k0 = 0; k0 < 8; ++k0) {
        const int k = k0 == 0 ? 0 : k0 <= 2 ? 10 + k0 : k0 - 2;          // 0 Refill Effort (net), 11 Eaten, 12 Refilled, then 1 Added .. 5 Crowd as before
        off::InfoRow r; r.label = labels[k0];
        for (int s = 0; s < 2; ++s) {
            if (!b || !b->ok) { r.v[s] = "n/a"; continue; }
            const off::Rec& x = b->r;
            if (k == 11) { r.v[s] = std::to_string(x.hitC[s]); continue; }   // (1.5.0) lots the attackers took from this side
            if (k == 12) { r.v[s] = refillCell(x, s, mkt); continue; }   // (1.5.0) refill / eaten, STRONG / weak by market
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
// (1.4.0, Rassul approved 23:48 CT) THE READ ROW - what the book is doing near price, from the Net ratios of the candle shown (hovered
// closed, else the live forming one): a side ARRIVES (net > 0) or LEAVES (net < 0) when its |net| is >= 1.0x its average and >= 3 lots,
// else it is FLAT. When both move the same way and one is >= 1.5x the other, the stronger decides (the weaker counts as flat).
//   bids arriving + offers leaving -> BULLISH "Bids stacking {b}x, offers pulled {o}x - path up opening"; + offers flat -> BULLISH
//   "Bids stacking {b}x - support arriving"; bids leaving + offers arriving -> BEARISH "Bids pulled {b}x, offers stacking {o}x - path down
//   opening"; + offers flat -> BEARISH "Bids pulled {b}x - support leaving"; bids flat + offers leaving -> BULLISH "Offers pulled {o}x -
//   resistance leaving"; bids flat + offers arriving -> BEARISH "Sellers stacking offers {o}x, little support under price ({b}x)" (no
//   bid ratio yet: " - resistance building"); both arriving -> NEUTRAL "Both sides stacking ~{r}x - book thickening, wait for the hit";
//   both leaving -> NEUTRAL "Both sides pulling ~{r}x - book thinning"; both flat -> NEUTRAL "Book quiet - nothing building or leaving".
static const long long READ_MIN_LOTS = 3;
// (1.4.0, Rassul approved 23:56 CT bookflow-pull2-mockup.png) THE PULL MARKER: a side PULLED its orders near price when its pulled lots are
// >= 2.0x the plain average of the prior 20 decided candles' pulled lots (>= 5 needed, mean floored at 1 lot) AND >= 1.2x its added lots
// in the candle - real pulling, not re-quoting. Drawn only for a CLOSED candle (decided once at its close, never repainted).
// Calibration (ES Fri 2026-10-16 RTH, his recording, BookFlow's own engine): pulled / added per side-candle p50 0.80, max 0.89 -> 0 markers
// at 1.2x; reported to the coordinator, thresholds NOT changed (PULL_X_T / PULL_ADD_T).
static const int PULL_X_T = 20, PULL_ADD_T = 12, PULL_MENTION_T = 15, PULL_BARS = 20;
struct PullStat { bool ok = false; long long add[2] = {0, 0}, pull[2] = {0, 0}; bool addOk[2] = {false, false}, pullOk[2] = {false, false};
                  double addR[2] = {0, 0}, pullR[2] = {0, 0}, pullMean[2] = {0, 0}; bool pulling[2] = {false, false}; };
// the prior added / pulled lots before t (decided, not partial, newest last, at most `need`)
inline void priorAddPull(const std::map<long long, off::PaneBar>& bars, long long t, size_t need, std::vector<long long> add[2], std::vector<long long> pull[2]) {
    for (int s = 0; s < 2; ++s) { add[s].clear(); pull[s].clear(); }
    std::map<long long, off::PaneBar>::const_iterator it = bars.lower_bound(t);
    while (it != bars.begin() && add[0].size() < need) {
        --it; if (!it->second.ok || it->second.r.partial) continue;
        for (int s = 0; s < 2; ++s) { add[s].push_back(it->second.r.addC[s]); pull[s].push_back(it->second.r.pullC[s]); }
    }
    for (int s = 0; s < 2; ++s) { std::reverse(add[s].begin(), add[s].end()); std::reverse(pull[s].begin(), pull[s].end()); }
}
// pulling = the marker's condition (pulled >= 2.0x its average AND >= 1.2x added); a partial candle never pulls
inline PullStat pullStatOf(const std::vector<long long> add[2], const std::vector<long long> pull[2], const off::Rec& r) {
    PullStat p; p.ok = true;
    for (int s = 0; s < 2; ++s) {
        p.add[s] = r.addC[s]; p.pull[s] = r.pullC[s];
        p.addR[s] = off::ratioOf(add[s], p.add[s], PULL_BARS, p.addOk[s], nullptr, 1.0);
        p.pullR[s] = off::ratioOf(pull[s], p.pull[s], PULL_BARS, p.pullOk[s], &p.pullMean[s], 1.0);
        p.pulling[s] = !r.partial && p.pullOk[s] && p.pull[s] > 0 && (double)p.pull[s] * 10.0 >= (double)PULL_X_T * p.pullMean[s] && p.pull[s] * 10 >= (long long)PULL_ADD_T * p.add[s];
    }
    return p;
}
inline PullStat pullStatAt(const std::map<long long, off::PaneBar>& bars, long long t, const off::Rec& r) {
    std::vector<long long> a[2], p[2]; priorAddPull(bars, t, (size_t)PULL_BARS, a, p); return pullStatOf(a, p, r);
}
inline std::string pullLabel(const PullStat& p, int s) { return "P " + br::x1(p.pullR[s]); }
enum { RD_FLAT = 0, RD_ARRIVE = 1, RD_LEAVE = 2 };
inline int sideState(const NetStat& n, int s) {
    const long long a = n.net[s] < 0 ? -n.net[s] : n.net[s];
    if (!n.ratioOk[s] || n.ratio[s] < 1.0 || a < READ_MIN_LOTS) return RD_FLAT;
    return n.net[s] > 0 ? RD_ARRIVE : RD_LEAVE;
}
// (1.4.0, 23:56 CT) PULLING decides first: both sides pulling -> NEUTRAL "Both sides pulling - book thinning, a fast move is likely";
// bids pulling + offers stacking -> BEARISH "Sellers stacking offers {o}x, bids pulled {p}x - support leaving"; bids pulling -> BEARISH
// "Bids pulled {p}x, more than added - support leaving under price"; offers pulling + bids stacking -> BULLISH "Bids stacking {b}x, offers
// pulled {p}x - path up opening"; offers pulling -> BULLISH "Offers pulled {p}x, more than added - resistance leaving, path up opening";
// else the stacking rules below. A side pulling >= 1.5x its average is mentioned (", bids pulling {p}x") when the text does not say it.
inline br::Read bookRead(const NetStat* n, const PullStat* pl = nullptr) {
    br::Read r;
    if (!n || !n->ok) { r.head = "No order data for this candle"; return r; }
    int st[2] = { sideState(*n, BID), sideState(*n, ASK) };
    const double b = n->ratio[BID], o = n->ratio[ASK];
    if (st[0] == st[1] && st[0] != RD_FLAT) { if (b >= 1.5 * o) st[1] = RD_FLAT; else if (o >= 1.5 * b) st[0] = RD_FLAT; }
    const std::string bx = br::x1(b) + "x", ox = br::x1(o) + "x";
    const bool pull[2] = { pl && pl->ok && pl->pulling[BID], pl && pl->ok && pl->pulling[ASK] };
    const std::string pbx = pl ? br::x1(pl->pullR[BID]) + "x" : std::string(), pox = pl ? br::x1(pl->pullR[ASK]) + "x" : std::string();
    if (pull[0] && pull[1]) { r.head = "Both sides pulling - book thinning, a fast move is likely"; return r; }
    if (pull[0]) { r.impl = br::BEAR; r.head = st[1] == RD_ARRIVE ? "Sellers stacking offers " + ox + ", bids pulled " + pbx + " - support leaving" : "Bids pulled " + pbx + ", more than added - support leaving under price"; }
    else if (pull[1]) { r.impl = br::BULL; r.head = st[0] == RD_ARRIVE ? "Bids stacking " + bx + ", offers pulled " + pox + " - path up opening" : "Offers pulled " + pox + ", more than added - resistance leaving, path up opening"; }
    else if (st[0] == RD_ARRIVE && st[1] == RD_LEAVE) { r.impl = br::BULL; r.head = "Bids stacking " + bx + ", offers pulled " + ox + " - path up opening"; }
    else if (st[0] == RD_ARRIVE && st[1] == RD_FLAT) { r.impl = br::BULL; r.head = "Bids stacking " + bx + " - support arriving"; }
    else if (st[0] == RD_LEAVE && st[1] == RD_ARRIVE) { r.impl = br::BEAR; r.head = "Bids pulled " + bx + ", offers stacking " + ox + " - path down opening"; }
    else if (st[0] == RD_LEAVE && st[1] == RD_FLAT) { r.impl = br::BEAR; r.head = "Bids pulled " + bx + " - support leaving"; }
    else if (st[0] == RD_FLAT && st[1] == RD_LEAVE) { r.impl = br::BULL; r.head = "Offers pulled " + ox + " - resistance leaving"; }
    else if (st[0] == RD_FLAT && st[1] == RD_ARRIVE) { r.impl = br::BEAR; r.head = "Sellers stacking offers " + ox; r.opt.push_back(n->ratioOk[BID] ? ", little support under price (" + bx + ")" : std::string(" - resistance building")); }
    else if (st[0] == RD_ARRIVE && st[1] == RD_ARRIVE) r.head = "Both sides stacking ~" + br::approx((b + o) / 2.0) + "x - book thickening, wait for the hit";
    else if (st[0] == RD_LEAVE && st[1] == RD_LEAVE) r.head = "Both sides pulling ~" + br::approx((b + o) / 2.0) + "x - book thinning";
    else r.head = "Book quiet - nothing building or leaving";
    if (pl && pl->ok) for (int s = 0; s < 2; ++s)                       // pulling >= 1.5x its average is said even when it does not decide
        if (!pull[s] && pl->pullOk[s] && pl->pullR[s] * 10.0 >= PULL_MENTION_T - 1e-9 && r.head.find(s == BID ? "ids pulled" : "ffers pulled") == std::string::npos)
            r.opt.push_back(std::string(s == BID ? ", bids pulling " : ", offers pulling ") + br::x1(pl->pullR[s]) + "x");
    return r;
}
// the live (forming) candle's Net ratios against the decided candles before it (the same rule as a closed candle; never a decision)
inline NetStat liveNetStat(const std::map<long long, off::PaneBar>& bars, long long t, const off::Rec& r, int ratioBars, long long avgT) {
    std::vector<long long> pb, pa; priorAbsNet(bars, t, (size_t)(ratioBars > off::RATIO_HI ? off::RATIO_HI : (ratioBars < off::RATIO_LO ? off::RATIO_LO : ratioBars)), pb, pa);
    NetStat n = netStat(pb, pa, r, ratioBars, avgT); for (int s = 0; s < 2; ++s) n.spike[s] = false; return n;
}
inline std::string netCell(const off::PaneBar& b, int s, const NetStat* ns) {
    std::string o = signedLots(netOf(b.r, s));
    if (ns && ns->ok && ns->ratioOk[s]) o += " (" + off::ratioText(ns->ratio[s]) + ")";
    return o;
}
// one side of one candle as drawn. closed = a later bar exists (only closed candles are judged). col = 0 green (bid side, buyers),
// 1 red (offer side, sellers): BY SIDE, never by meaning. ratio / pace = the label rows ("" = none); the pace only for size arriving.
inline std::string dropX(std::string t) { if (!t.empty() && t[t.size() - 1] == 'x') t.erase(t.size() - 1); return t; }   // "2.4x" -> "2.4"
struct NetLook { long long v = 0; bool bright = false; int col = 0; std::string ratio, pace; };   // (1.5.0) pace = the Refilled label ("R4.3")
inline NetLook netLookOf(const off::PaneBar* b, const NetStat* ns, int s, bool closed, const std::string& mkt = std::string()) {
    NetLook k; k.col = s == BID ? 0 : 1;
    if (!b || !b->ok) return k;
    k.v = netOf(b->r, s);
    if (!closed || !ns || !ns->ok || !ns->spike[s]) return k;
    k.bright = true;
    k.ratio = ns->ratioOk[s] ? dropX(off::ratioText(ns->ratio[s])) : std::string();
    // (1.5.0, Rassul 11:53 "replace the pace ... also on the labels") the second label = REFILLED (re-posted at the hit prices / eaten)
    { const Refill f = refillOf(b->r, s, mkt); if (f.ok && f.relOk) { char t[24]; std::snprintf(t, sizeof(t), "R%.1f", f.rel > 99.9 ? 99.9 : f.rel); k.pace = t; } }   // (12:22) vs the market's normal
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

inline bool refillRead(const off::Rec& r, const std::string& mkt, const Spot& sp, const NetStat* ns, br::Read& out) {
    if (!sp.ok || (!sp.newLow && !sp.newHigh)) return false;
    int d = sp.newLow ? BID : ASK;
    if (sp.newLow && sp.newHigh) d = r.hitC[BID] >= r.hitC[ASK] ? BID : ASK;
    const Refill f = refillOf(r, d, mkt);
    if (!f.ok || f.cls == RF_NA || f.cls == RF_NORMAL) return false;
    const bool low = d == BID;
    const std::string where = low ? "New low: " : "New high: ", def = low ? "buyers" : "sellers", att = low ? "sellers" : "buyers";
    if (!f.relOk) return false;
    const std::string fx = br::x1(f.rel) + "x normal";
    br::Read rd;
    if (f.cls == RF_STRONG) {
        rd.impl = low ? br::BULL : br::BEAR;
        const bool cr = sp.busy && ns && ns->ok && ns->ratioOk[d] && ns->net[d] > 0 && ns->ratio[d] >= 2.0;
        if (cr) rd.head = "C+R ? " + where + att + " took " + commas(f.eaten) + ", " + def + " refilled " + fx + " - defense holding";
        else rd.head = where + def + " refilled " + fx + " (strong) - defense holding ?";
    } else {
        rd.impl = low ? br::BEAR : br::BULL;
        rd.head = where + def + " refilled only " + fx + " (weak) - defense failing, don't fade ?";
    }
    out = rd; return true;
}

// ---- (1.14.0, Rassul 2026-10-20 09:59 "lets do it") the EARLY-WARNING READ while the candle forms, at a FRESH price (untraded on that side
// for 5 min - the engine's touch tracker). Text only, never a mark; the normal Read returns once the candle closes. WATCH (defenders'
// colour) needs all three: (1) the attack SLOWING at that price over the complete 20--60 s phase (lots per second in that full 40 s vs the
// first 20 s <= the market's median slowdown, the exhaustion rule) or stopped; (2) the defending side's Refill Effort so far this candle
// is STRICTLY BIGGER than the other side's (no 1.5x gate); (3) a STRONG refill at the hit prices (the market's Refilled cut-offs). CAUTION
// (the other colour), available from 25 s independently of the WATCH-only 60 s slowdown window: the other side's positive Refill Effort is
// STRICTLY BIGGER than the defenders' (Rassul 15:33: "just greater than", both reads; equality is neither read) - the level is being hit
// while the other side stacks. Both carry "?" (unproven; scored nightly).
struct EwTouch { bool ok = false; int s = BID; double px = 0; long long elapsed = 0, h1 = 0, h2 = 0; };
inline std::string ewPx(double px, double tick) {
    int d = 0; for (double t = tick; d < 6 && std::fabs(t - std::floor(t + 1e-9)) > 1e-9; t *= 10) ++d;
    char b[40]; std::snprintf(b, sizeof(b), "%.*f", d, px); return b;
}
inline bool earlyRead(const EwTouch& t, const off::Rec& r, const std::string& mkt, double median, double tick, br::Read& out) {
    if (!t.ok || (t.s != BID && t.s != ASK) || t.h1 <= 0 || t.h2 < 0 || t.elapsed < 25 || !off::finite(median) || median < 0 || !off::finite(t.px) || !off::finite(tick) || tick <= 0) return false; // a real attack
    const int d = t.s, o = 1 - t.s; const bool bids = d == BID;
    const long long nd = netOf(r, d), no = netOf(r, o);
    const std::string D = bids ? "Bids" : "Offers", O = bids ? "offers" : "bids", def = bids ? "buyers" : "sellers", att = bids ? "sellers" : "buyers";
    const std::string at = ewPx(t.px, tick);
    if (no > 0 && no > nd) {                                               // CAUTION: the other side stacking more (Rassul 15:33: bigger is enough, was 1.5x)
        br::Read rd; rd.impl = bids ? br::BEAR : br::BULL; rd.tag = "CAUTION";
        rd.head = D + " hit at " + at + ", but " + O + " stacking " + br::x1((double)no / (double)std::max<long long>(nd, 1)) + "x more " + (bids ? "above" : "below") + " - don't lean on it ?";
        out = rd; return true;
    }
    if (t.elapsed < 60) return false;                                      // WATCH needs the complete 20--60 s slowdown window
    const double r1 = (double)t.h1 / 20.0, r2 = (double)t.h2 / 40.0;
    const bool stopped = t.h2 == 0, slowing = stopped || r2 / (r1 + 0.05) <= median + 1e-12;
    if (!slowing) return false;
    if (nd <= 0 || nd <= no) return false;                                // (Rassul 15:33 "it should just be greater than") the defenders bigger, no 1.5x needed
    const Refill f = refillOf(r, d, mkt);
    if (!f.ok || f.cls != RF_STRONG || !f.relOk) return false;
    br::Read rd; rd.impl = bids ? br::BULL : br::BEAR; rd.tag = "WATCH";
    rd.head = D + " defending at " + at + ": " + att + (stopped ? " stopped hitting, " : " slowing, ") + def + " " + br::x1((double)nd / (double)std::max<long long>(no, 1)) + "x more than " + att + ", refilled " + br::x1(f.rel) + "x - watch the close ?";
    out = rd; return true;
}

struct Logic {
    static const char* name() { return "BookFlow"; }
    static const char* title() { return "Defense"; }   // (1.9.0, Rassul 19:31) was "Book Flow - Who is arriving or leaving"
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
