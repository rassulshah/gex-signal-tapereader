// TurnBox.h (1.0.0, 2026-11-27) - SDK-free logic of the Reversal Read pane's INFO BOX (lsRevRead 0.47.0).
// Rassul 2026-11-27 23:11-23:25 CT: "i need meaningfull insight data there only not just repetitive information or ratios or numbers";
// "start with stops, then trapped (loosers), then which ever side is the winning side"; "remove watch"; "The purpose of Trapped is to
// identify if we have fuel for a reversal"; the Stops line "is telling me what i already see on the chart"; "1. dont need them" (the gold
// rows and the old buy / sell read lines go); "2. ok" (full box on a turn, only lines with something real otherwise); "3. no ?" (no "?").
// Four lines, bullish turn (a bearish turn is the mirror):
//   Stops   - are the stops USED UP? the run candle's stop lots (key-level runs: lsSR's run lots), then every later push beyond the level
//             before the turn candle and its one-second small-order burst: all Light (< 0.75x) = used up; any >= 1.0x = still firing.
//   Trapped - FUEL: the market-order selling below the level from the run candle to the turn candle (IRT's volume at price: the bid-side
//             lots at each price beyond the level), its average price, how far under water at the close, its size vs a normal candle's
//             selling (Light < 0.5, Moderate < 1.5, Big < 3, Huge), and how much is covered (net market buying since the run candle vs the
//             trapped lots: < 25% little, < 75% partly, else mostly). Not under water = no fuel.
//   Buyers  - the winning side's DEFENSE at the low: resting bids of big-lot size (>= the S20 tier in force) priced from the pivot low to the
//             level, alive from the run candle to the turn: lots traded into them vs lots pulled from them.
//   Sellers - the losing side on the turn candle: its biggest aggressive order (>= the S20 tier = still swinging, it could not stop the
//             turn; else stepping back).
// No numbers / ratios beyond what each sentence needs; colours: green = good for the turn, red = against it.
#pragma once
#include "PressRead.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace tbx {
static const char* const VERSION = "1.0.0";

// ---- big resting orders, followed order by order (the host feeds every event) ----
struct BigO { int side = 0; long long px = 0, first = 0, orig = 0, rem = 0, traded = 0, pulled = 0, k0 = 0, k1 = 0; bool lastFill = false; };   // px in ticks; k0 / k1 = candle end keys (RTDATE)
struct BigTrack {
    std::unordered_map<long long, BigO> open; std::deque<BigO> done;
    static long long keyOf(long long sec, int spb) { return spb > 0 ? (sec / spb + 1) * (long long)spb : sec; }
    void finish(std::unordered_map<long long, BigO>::iterator it) { done.push_back(it->second); open.erase(it); while (done.size() > 4000) done.pop_front(); }
    // action 0 NEW, 1 MOD, 2 DEL, 3 FILL (off:: order); side 0 / 1 (bid / ask), -1 unknown; thr = the big-lot size in force (0 = unknown: nothing new followed)
    void onEvent(long long sec, int spb, int action, int side, long long px, long long size, long long id, long long thr) {
        if (id == 0) return;
        const long long k = keyOf(sec, spb);
        if (action == 0) { if (thr > 0 && side >= 0 && size >= thr && open.size() < 20000) { BigO o; o.side = side; o.px = px; o.first = o.orig = o.rem = size; o.k0 = o.k1 = k; open[id] = o; } return; }
        auto it = open.find(id); if (it == open.end()) return; BigO& o = it->second; o.k1 = k;
        // the feed's rules (OrderFlowFeed.h): a MOD after a FILL = the rest; a MOD to a smaller size not after a fill = a partial pull; a bigger
        // size = a reload; a DEL right after a FILL = the trade took it (not a pull)
        if (action == 1) { if (size > o.rem) o.orig += size - o.rem; else if (size < o.rem && !o.lastFill) o.pulled += o.rem - size;
            o.rem = size; o.lastFill = false; if (px > 0) o.px = px; if (o.rem <= 0) finish(it); return; }
        if (action == 3) { if (size > 0) { o.traded += size; o.rem -= size; } o.lastFill = true; if (o.rem <= 0) { o.rem = 0; finish(it); } return; }
        if (action == 2) { if (o.rem > 0 && !o.lastFill) o.pulled += o.rem; o.rem = 0; finish(it); return; }
    }
    void reset() { open.clear(); done.clear(); }
};
struct Defense { int n = 0; long long maxLots = 0, traded = 0, pulled = 0, resting = 0; };
// the big orders of `side` priced in [pxLo, pxHi] (ticks) alive at any time in candles (kFrom, kTo] (end keys)
inline Defense defense(const BigTrack& T, int side, long long pxLo, long long pxHi, long long kFrom, long long kTo) {
    Defense d; auto take = [&](const BigO& o, bool isOpen) { if (o.side != side || o.px < pxLo || o.px > pxHi || o.k1 <= kFrom || o.k0 > kTo) return;
        ++d.n; d.maxLots = std::max(d.maxLots, o.first); d.traded += o.traded; d.pulled += o.pulled; if (isOpen) d.resting += o.rem; };
    for (const BigO& o : T.done) take(o, false);
    for (const auto& kv : T.open) take(kv.second, true);
    return d;
}

struct Line { std::string lab, text; int col = 0; };   // col +1 green, -1 red, 0 white
struct Box { bool ok = false; std::string head; int dir = 0; std::vector<Line> lines; };

inline std::string lots(long long v) { char b[32]; if (v >= 1000) std::snprintf(b, sizeof b, "%s", prd::grp(v, false).c_str()); else std::snprintf(b, sizeof b, "%lld", v); return b; }
inline std::string about(long long v) { long long r = v >= 1000 ? (v + 50) / 100 * 100 : v >= 100 ? (v + 5) / 10 * 10 : v; return "about " + prd::grp(r, false); }
inline std::string fuelWord(double x) { return x >= 3 ? "Huge" : x >= 1.5 ? "Big" : x >= 0.5 ? "Moderate" : "Light"; }

// i = the read's candle; cx = its stop-run context (prd::context for the read's direction); burst(key, side); sellBeyond(key, pxTick, side,
// &lots, &pxLots) = the losing side's market-order lots at prices beyond pxTick in that candle (IRT volume at price) and the sum of lots x
// price; aggrBig(key, side) = the biggest aggressive order of `side` (+1 buyer / -1 seller) in that candle (0 = unknown); thr = big-lot size
template <class Burst, class Beyond, class AggrBig, class Clock>
inline Box box(const std::vector<prd::Cdl>& C, const std::vector<long long>& keys, const std::vector<long long>& rtKeys, int i, const prd::Ctx& cx,
               int pivot, double tick, Burst burst, Beyond beyond, AggrBig aggrBig, const BigTrack* big, long long thr, Clock clock) {
    Box b; if (!cx.ok || cx.k < 0 || i < cx.k || i >= (int)C.size() || !(tick > 0)) return b;
    const int sd = cx.side, win = -sd;             // sd = the stops' side (-1 below a low); the winning side of the turn = the other
    b.ok = true; b.dir = win; const bool bull = win > 0;
    b.head = clock(i) + (cx.rec ? (bull ? " - Bullish turn" : " - Bearish turn") : (bull ? " - Stop run below " : " - Stop run above ") + cx.lvl);
    const std::string stopW = bull ? "sell stops" : "buy stops", loser = bull ? "sellers" : "buyers", Loser = bull ? "Sellers" : "Buyers", Winner = bull ? "Buyers" : "Sellers";
    // ---- Stops: used up? ----
    {   std::vector<int> push; double maxX = 0; int strong = -1;
        for (int j = cx.k + 1; j < i; ++j) { if (!C[(size_t)j].ok) continue; if (sd < 0 ? C[(size_t)j].l < cx.px : C[(size_t)j].h > cx.px) { push.push_back(j);
            const double x = burst(keys[(size_t)j], sd); if (x > maxX) maxX = x; if (x >= 1.0 && strong < 0) strong = j; } }
        std::string fired = (cx.key && cx.vol > 0 ? about(cx.vol) + " lots of " + stopW : stopW) + " fired at " + clock(cx.k);
        Line L; L.lab = "Stops";
        if (push.empty()) { L.text = fired + "; no push beyond it since"; L.col = 0; }
        else { std::string pp; for (size_t q = 0; q < push.size() && q < 3; ++q) pp += (q ? (q + 1 == push.size() || q == 2 ? " and " : ", ") : "") + clock(push[q]);
            if (strong >= 0) { L.text = "still firing - the " + clock(strong) + " push " + (bull ? "lower" : "higher") + " fired more " + stopW; L.col = -1; }
            else if (maxX >= 0.75) { L.text = "mostly used up - " + fired + "; the " + pp + " push" + (push.size() > 1 ? "es" : "") + " found a few more"; L.col = 1; }
            else { L.text = "used up - " + fired + "; the " + pp + " push" + (push.size() > 1 ? "es " : " ") + (bull ? "lower" : "higher") + " found almost none"; L.col = 1; } }
        b.lines.push_back(L); }
    // ---- Trapped: fuel ----
    {   long long tl = 0; double tpx = 0;
        const long long lvT = (long long)std::llround(cx.px / tick);
        for (int j = cx.k; j <= i; ++j) { long long a = 0; double ap = 0; if (beyond(rtKeys[(size_t)j], lvT, sd, a, ap)) { tl += a; tpx += ap; } }
        Line L; L.lab = "Trapped";
        if (tl <= 0) { L.text = "no fuel - no " + loser + " traded beyond " + cx.lvl; L.col = 0; }
        else { const double avg = tpx / (double)tl; const double under = bull ? C[(size_t)i].c - avg : avg - C[(size_t)i].c;
            double sN = 0; int nN = 0; for (int j = cx.k - 1; j >= 0 && j >= cx.k - 40 && nN < prd::NORM_N; --j) if (C[(size_t)j].ok) { sN += (double)(bull ? C[(size_t)j].sell : C[(size_t)j].buy); ++nN; }
            const double norm = nN >= 5 ? sN / nN : 0; long long net = 0; for (int j = cx.k + 1; j <= i; ++j) if (C[(size_t)j].ok) net += bull ? C[(size_t)j].d() : -C[(size_t)j].d();
            const double cov = (double)std::max(0LL, net) / (double)tl;
            const std::string w = norm > 0 ? fuelWord((double)tl / norm) + " fuel - " : std::string();
            const std::string covW = cov < 0.25 ? "little covered yet" : cov < 0.75 ? "partly covered" : "mostly covered";
            if (under <= 0) { L.text = "no fuel - " + about(tl) + " lots " + (bull ? "sold under " : "bought over ") + cx.lvl + " are not under water"; L.col = -1; }
            else { L.text = w + about(tl) + " lots " + (bull ? "sold under " : "bought over ") + cx.lvl + ", " + prd::pxs(std::round(under / tick) * tick) + " pts under water, " + covW; L.col = cov < 0.75 ? 1 : 0; } }
        b.lines.push_back(L); }
    if (!cx.rec) return b;                          // the turn is not under way: Stops and Trapped only
    // ---- Winner: defense at the extreme ----
    if (big) {
        const int pv = pivot >= 0 && pivot < (int)C.size() ? pivot : i;
        const long long lvT = (long long)std::llround(cx.px / tick), exT = (long long)std::llround((bull ? C[(size_t)pv].l : C[(size_t)pv].h) / tick);
        const long long lo = std::min(lvT, exT), hi = std::max(lvT, exT);
        const long long kFrom = cx.k > 0 ? rtKeys[(size_t)cx.k - 1] : rtKeys[(size_t)cx.k] - 1, kTo = rtKeys[(size_t)i];
        const Defense d = defense(*big, bull ? 0 : 1, lo, hi, kFrom, kTo);
        Line L; L.lab = Winner; const std::string ord = bull ? "bids" : "offers";
        if (thr <= 0) { L.text = "big-order size not known yet"; L.col = 0; }
        else if (d.n == 0) { L.text = "no big " + ord + " at the " + (bull ? "low" : "high") + " - small orders took the " + (bull ? "selling" : "buying"); L.col = 0; }
        else if (d.pulled > d.traded) { L.text = "big " + ord + " pulled at the " + (bull ? "low" : "high") + " - " + about(d.pulled) + " lots pulled, " + about(d.traded) + " traded"; L.col = -1; }
        else { L.text = "absorbed the " + std::string(bull ? "flush" : "squeeze") + " - big " + ord + " (up to " + lots(d.maxLots) + " lots) took " + about(d.traded) + " lots" + (d.pulled > 0 ? ", " + about(d.pulled) + " pulled" : ", none pulled"); L.col = 1; }
        b.lines.push_back(L); }
    // ---- Loser on the turn candle ----
    {   const long long a = aggrBig(rtKeys[(size_t)i], sd < 0 ? -1 : 1);
        Line L; L.lab = Loser;
        if (a <= 0) return b;
        if (thr > 0 && a >= thr) { L.text = "still swinging - a " + lots(a) + "-lot " + (bull ? "sell" : "buy") + " order could not stop the turn"; L.col = 1; }
        else { L.text = "stepping back - no big " + std::string(bull ? "sell" : "buy") + " orders on the turn (largest " + lots(a) + " lots)"; L.col = 1; }
        b.lines.push_back(L); }
    return b;
}
} // namespace tbx
