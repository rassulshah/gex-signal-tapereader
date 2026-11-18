// CombRead.h  --  the ONE read of the Reversal Read pane (lsRevRead 0.15.0). SDK-free.
// (1.0.0, 2026-11-17 Rassul 15:24-15:41 CT "splitting the reversal read indicator so that it has bookflow at the top and delta with change in delta at the
// bottom", "i dont want ti to have two seperate reads ... 1 effective read", "so now this indicator has both the aggressive and the passive sides right",
// "ok, mockup", "regargidn the testing /sharpeninng go" / "after that build"; mockup reversal_read_split.html)
// PASSIVE side = Book Flow (resting orders added / pulled / refilled / icebergs); AGGRESSIVE side = delta and its change from the candle before.
// Verdict:  book and delta the same way -> "Buyers / Sellers in control ?";  opposite -> "Mixed ?" + what to watch;  only one stands out -> "Leaning buy / sell ?";
//           nothing -> "Quiet".  No Book Flow record for the candle -> the delta alone, said so.
// Thresholds from the 15:41 CT test (909 pivots, ES 22 days, CL / GC / 6E 10 days; turned = 1 ATR from the call close before the pivot zone broke; base 47%):
//   defenders stacking >= 1.15x: 60% (ES 60%, CL 64% / 82% at 1.5x, GC 60% / 73% at 1.5x)  -> 1.15x ES / 6E, 1.5x CL / GC, 1.3x others
//   delta change >= 2x normal the defenders' way: 55% all (ES 53%, CL 62%, GC 63%; 6E 43% = no edge -> the change does not vote on 6E)
//   book + delta agree: 73% (35 / 48).  "Held vs hit" (refilled / hit) never fell under 1x and did not separate (47% at any level) -> not shown.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace cbr {

static const char* const VERSION = "1.1.1";   // (1.1.1) the delta-change label at 1.33x   // was "1.1.0";   // (1.1.0, Rassul 2026-11-17 15:50-16:03 CT case review, "go") the pivot reads on top of the verdict: trapped, absorption, breakout met, rejection wick

struct In {
    std::string mkt, when;               // the market key (ES, NQ, CL, GC, HG, NG, EU) and the candle's time ("2:27 PM")
    bool bookOk = false;                 // the candle has a Book Flow record
    long long add[2] = { 0, 0 }, pull[2] = { 0, 0 }, pullNear[2] = { 0, 0 }, ref[2] = { 0, 0 }, ice[2] = { 0, 0 }, hit[2] = { 0, 0 };   // [0] = bids, [1] = offers; hit[0] = lots SOLD into the bids, hit[1] = lots BOUGHT from the offers
    long long bigBuy = 0, bigSell = 0;   // the biggest single market order each way
    bool deltaOk = false;                // the candle and the one before have volume at price
    long long dl = 0, dlPrev = 0;        // this candle's delta and the one before
    double dlX = 0, chgX = 0;            // |delta| and |change| x their medians over the 20 candles before (0 = not known)
    // (1.1.0) the pivot reads: the candles up to and including the read candle (oldest first, the read candle LAST), prices in ticks
    struct PC { long long o = 0, h = 0, l = 0, c = 0, dl = 0, bN = 0, bD = 0, sN = 0, sD = 0; bool ok = false; std::string when; };
    std::vector<PC> pc; double tick = 0; int dec = 2;
};
struct Row { std::string lab, a, b; int ca = 0, cb = 0; bool head = false; };   // ca / cb: +1 green, -1 red, 0 white
struct Out { int verdict = 0; std::string head, why; std::vector<std::string> ex; std::vector<Row> rows; };   // verdict +1 / -1 / 2 mixed / 0 quiet

inline double stackMin(const std::string& m) { return (m == "ES" || m == "EU") ? 1.15 : (m == "CL" || m == "GC") ? 1.5 : 1.3; }
inline bool changeVotes(const std::string& m) { return m != "EU"; }
static const double CHG_MIN = 2.0;       // a delta change that counts: 2x normal or more
static const double CHG_LABEL = 1.33;    // (1.1.1, Rassul 18:06 CT "lets go with 1.33x") the ratio over a delta-change bar: 1.33x normal or more (54% turned at pivots vs 53% at 2x, a third more labels); the verdict keeps 2x
static const double DL_MIN = 1.5;        // a delta that counts on its own

inline std::string commas(long long v) { const bool neg = v < 0; unsigned long long u = neg ? (unsigned long long)(-(v + 1)) + 1ULL : (unsigned long long)v; std::string s = std::to_string(u), o;
    for (size_t i = 0; i < s.size(); ++i) { if (i && (s.size() - i) % 3 == 0) o += ','; o += s[i]; } return (neg ? "-" : "") + o; }
inline std::string signedC(long long v) { return (v > 0 ? "+" : "") + commas(v); }
inline std::string xx(double v) { char b[24]; std::snprintf(b, sizeof b, "%.1fx", v); return b; }

// +1 bids building, -1 offers building, 0 neither (book side);  the ratio in r
inline int bookSide(const In& x, double& r) {
    r = 0; if (!x.bookOk) return 0;
    const long long b = x.add[0] - x.pull[0], a = x.add[1] - x.pull[1]; const double mn = stackMin(x.mkt);
    if (b > 0 && (a <= 0 || (double)b >= mn * (double)a)) { r = a > 0 ? (double)b / (double)a : 0; return 1; }
    if (a > 0 && (b <= 0 || (double)a >= mn * (double)b)) { r = b > 0 ? (double)a / (double)b : 0; return -1; }
    if (a > 0 && b > 0) r = (double)std::max(a, b) / (double)std::min(a, b);
    return 0;
}
// +1 buyers pushing, -1 sellers pushing, 0 neither (aggressive side); byChange = the delta change decided it
inline int pushSide(const In& x, bool& byChange) {
    byChange = false; if (!x.deltaOk) return 0;
    const long long chg = x.dl - x.dlPrev;
    if (changeVotes(x.mkt) && x.chgX >= CHG_MIN && chg != 0) { byChange = true; return chg > 0 ? 1 : -1; }
    if (x.dlX >= DL_MIN && x.dl != 0) return x.dl > 0 ? 1 : -1;
    return 0;
}


// ---------------------------------------------------------------------------------------------------------------------------------------------------
// (1.1.0) THE PIVOT READS from the case review with Rassul (2026-11-17 15:50-16:03 CT), scored at the CALL (909 pivots, ES 22 days, CL / GC / 6E 10 days;
// turned = 1 ATR from the call close before the pivot zone broke; base 48%, no read 41%):
//   ABSORPTION  "the circle low in the wick and the green candle closed above it and the high of the red candle"          69% (36/52)  all 4 markets 65-77%
//   TRAPPED     "trapped sellers in the wicks" - attackers' heaviest row in the wick, a close past that candle's other side  65% (95/147) all 4 markets 60-70%
//   REJECTION   "seller in the wick with a bigger wick than the body ... low end close" (the row beyond the body)          60% (54/90)  all 4 markets 56-73%
//   BREAKOUT MET "the breakout was met with selling ... a red candle closing below it"                                     56% (56/100) GC 41%, 2nd half 48%
// The read candle is the CALL candle k; the pivot is candle k-1 (a new 10-candle low / high, the call candle not more than the zone beyond it).
// the median of |v| (0 when empty)
inline double medAbs(std::vector<long long> v) { if (v.empty()) return 0; for (long long& q : v) q = std::llabs(q); std::sort(v.begin(), v.end());
    const size_t n = v.size(); return n % 2 ? (double)v[n / 2] : 0.5 * ((double)v[n / 2 - 1] + (double)v[n / 2]); }
struct Pat { int side = 0; std::string head, why, sent; int kind = 0; };   // side +1 = a low (bullish read), -1 = a high; kind 1 absorption, 2 trapped, 3 rejection, 4 breakout met
inline std::string px(long long t, double tick, int dec) { char b[32]; std::snprintf(b, sizeof b, "%.*f", dec, (double)t * tick); return b; }
inline Pat pivotRead(const In& x) {
    Pat P; const std::vector<In::PC>& c = x.pc; const int n = (int)c.size(); if (n < 14 || !(x.tick > 0)) return P;
    const int k = n - 1, i = k - 1; for (int j = k - 12; j <= k; ++j) if (!c[(size_t)j].ok) return P;
    double atr = 0; for (int j = i - 12; j <= i; ++j) atr += (double)(std::max(c[(size_t)j].h, c[(size_t)j - 1].c) - std::min(c[(size_t)j].l, c[(size_t)j - 1].c)); atr /= 13.0;
    const long long tol = std::max(2LL, (long long)std::llround(0.25 * atr));
    double medD = 0; { std::vector<long long> v; for (int j = std::max(0, i - 20); j < i; ++j) v.push_back(c[(size_t)j].dl); medD = medAbs(v); }   // the trapped side covering: its delta on the close >= 1x the normal |delta|
    long long mn = c[(size_t)i - 1].l, mx = c[(size_t)i - 1].h; for (int j = i - 10; j < i; ++j) { mn = std::min(mn, c[(size_t)j].l); mx = std::max(mx, c[(size_t)j].h); }
    int s = 0; if (c[(size_t)i].l < mn && c[(size_t)k].l >= c[(size_t)i].l - tol) s = 1; else if (c[(size_t)i].h > mx && c[(size_t)k].h <= c[(size_t)i].h + tol) s = -1;
    if (!s) return P;
    auto pos = [&](int j, long long t) { const In::PC& q = c[(size_t)j]; return q.h > q.l ? (double)(t - q.l) / (double)(q.h - q.l) : 0.5; };
    auto defN = [&](int j, long long& t) { const In::PC& q = c[(size_t)j]; if (s > 0 && q.bD > 0) { t = q.bN; return true; } if (s < 0 && q.sD < 0) { t = q.sN; return true; } return false; };   // the defenders' heaviest row (buyers at a low)
    auto attN = [&](int j, long long& t) { const In::PC& q = c[(size_t)j]; if (s > 0 && q.sD < 0) { t = q.sN; return true; } if (s < 0 && q.bD > 0) { t = q.bN; return true; } return false; };   // the attackers' heaviest row
    auto inWick = [&](int j, long long t, double third) { const double p = pos(j, t); return s > 0 ? p <= third : p >= 1.0 - third; };
    auto closedPast = [&](int q, long long lvl) { return s > 0 ? c[(size_t)q].c > lvl : c[(size_t)q].c < lvl; };
    const std::string up = s > 0 ? "up" : "down", A = s > 0 ? "Sellers" : "Buyers", D = s > 0 ? "Buyers" : "Sellers", hiLo = s > 0 ? "high" : "low", above = s > 0 ? "above" : "below";
    std::vector<Pat> F;   // every read found, strongest first
    auto absorption = [&]() -> Pat { Pat P;
    // 1. ABSORPTION: the defenders' heaviest row in the lower (upper) third of candle i-1 or i, then a close past that row AND that candle's high (low)
    for (int j = i - 1; j <= i; ++j) { long long t = 0; if (!defN(j, t) || !inWick(j, t, 1.0 / 3.0)) continue;
        for (int q = j + 1; q <= k; ++q) if (closedPast(q, s > 0 ? std::max(t, c[(size_t)j].h) : std::min(t, c[(size_t)j].l))) {
            P.side = s; P.kind = 1; P.head = s > 0 ? "Buyers absorbed the selling ?" : "Sellers absorbed the buying ?"; P.why = "- Reversal " + up + " ?";
            P.sent = D + " absorbed at " + px(t, x.tick, x.dec) + " in the " + c[(size_t)j].when + " candle's wick; the " + c[(size_t)q].when + " candle closed " + above + " it and that candle's " + hiLo + "."; return P; } }
    return P; };
    auto trapped = [&]() -> Pat { Pat P;
    // 2. TRAPPED: the attackers' heaviest row in the wick (outer 40%) of candle j (i-2 .. i), a later candle up to k closes past j's other side - the earliest close wins
    for (int q = i - 1; q <= k; ++q) for (int j = std::max(i - 2, q - 3); j < q; ++j) { long long t = 0; if (!attN(j, t) || !inWick(j, t, 0.4)) continue;
        if (!closedPast(q, s > 0 ? c[(size_t)j].h : c[(size_t)j].l)) continue;
        P.side = s; P.kind = 2; P.head = A + " trapped ?"; P.why = "- Reversal " + up + " ?";
        P.sent = A + " hit " + px(t, x.tick, x.dec) + " in the wick of the " + c[(size_t)j].when + " candle; the " + c[(size_t)q].when + " candle closed " + above + " its " + hiLo + ".";
        if ((double)(s * c[(size_t)q].dl) >= medD && medD > 0) P.sent += std::string(" ") + (s > 0 ? "Buying" : "Selling") + " came in hard on that close - the trapped side covering.";
        return P; }
    return P; };
    auto rejection = [&]() -> Pat { Pat P;
    // 3. REJECTION: the pivot candle itself - wick bigger than the body, the defenders' row in the wick, a close at the far end
    { const In::PC& q = c[(size_t)i]; const long long rng = q.h - q.l, body = std::llabs(q.c - q.o), wick = s > 0 ? std::min(q.o, q.c) - q.l : q.h - std::max(q.o, q.c); long long t = 0;
      if (rng > 0 && wick > body && defN(i, t) && (s > 0 ? t < std::min(q.o, q.c) : t > std::max(q.o, q.c)) &&   /* in the wick = beyond the body (case 6: 59% at the outer third, 60% of 90 beyond the body) */ (s > 0 ? (double)(q.c - q.l) / (double)rng >= 2.0 / 3.0 : (double)(q.c - q.l) / (double)rng <= 1.0 / 3.0)) {
        P.side = s; P.kind = 3; P.head = "Rejection wick ?"; P.why = "- Reversal " + up + " ?";
        P.sent = "The " + q.when + " candle rejected the " + (s > 0 ? "low" : "high") + ": wick bigger than the body, " + (s > 0 ? "buyers" : "sellers") + " at " + px(t, x.tick, x.dec) + " in the wick, close at the " + (s > 0 ? "top." : "bottom."); return P; } }
    return P; };
    auto breakoutMet = [&]() -> Pat { Pat P;
    // 4. BREAKOUT MET: the pivot candle took out an earlier 3-candle swing (last 40 candles), the defenders' row in its outer third, the call candle closes back past that row
    { long long sw = 0; bool got = false;
      for (int j = std::max(3, i - 40); j < i - 2; ++j) { bool piv = true; for (int a = j - 3; a <= j + 3 && piv; ++a) if (a != j && a < i && (s > 0 ? c[(size_t)a].l < c[(size_t)j].l : c[(size_t)a].h > c[(size_t)j].h)) piv = false;
        if (piv) { const long long v = s > 0 ? c[(size_t)j].l : c[(size_t)j].h; if (s > 0 ? (c[(size_t)i].l < v && v - c[(size_t)i].l <= (long long)atr) : (c[(size_t)i].h > v && c[(size_t)i].h - v <= (long long)atr)) { sw = v; got = true; } } }
      long long t = 0;
      if (got && defN(i, t) && inWick(i, t, 1.0 / 3.0) && closedPast(k, t)) {
        P.side = s; P.kind = 4; P.head = s > 0 ? "Breakdown met with buying ?" : "Breakout met with selling ?"; P.why = "- Reversal " + up + " ?";
        P.sent = std::string("Price ") + (s > 0 ? "broke the " : "broke the ") + px(sw, x.tick, x.dec) + " swing " + (s > 0 ? "low" : "high") + " and " + (s > 0 ? "buyers" : "sellers") + " met it at " + px(t, x.tick, x.dec) + "; the " + c[(size_t)k].when + " candle closed back " + above + "."; return P; } }
    return P; };
    for (const Pat& q : { absorption(), trapped(), rejection(), breakoutMet() }) if (q.side) F.push_back(q);
    if (F.empty()) return P;
    P = F[0]; for (size_t m = 1; m < F.size(); ++m) P.sent += " Also: " + F[m].sent;   // (1.1.0) two reads together scored 64% (41 / 64) - they are named, the strongest leads
    return P;
}

inline Out read(const In& x) {
    Out o; double r = 0; bool byC = false;
    const int bk = bookSide(x, r), ps = pushSide(x, byC);
    const long long chg = x.dl - x.dlPrev;
    if (!x.bookOk && !x.deltaOk) { o.head = "Quiet:"; o.why = "no order data for this candle"; return o; }
    if (bk && ps && bk == ps) { o.verdict = bk; o.head = bk > 0 ? "Buyers in control ?" : "Sellers in control ?"; o.why = "- the book and delta agree"; }
    else if (bk && ps) { o.verdict = 2; o.head = "Mixed ?:"; o.why = bk > 0 ? "bids building, but sellers hitting harder" : "offers building, but buyers hitting harder"; }
    else if (bk) { o.verdict = bk; o.head = bk > 0 ? "Leaning buy ?:" : "Leaning sell ?:"; o.why = bk > 0 ? "bids building" : "offers building"; }
    else if (ps) { o.verdict = ps; o.head = ps > 0 ? "Leaning buy ?:" : "Leaning sell ?:"; o.why = ps > 0 ? "buyers stepped in" : "sellers stepped in"; }
    else { o.verdict = 0; o.head = "Quiet:"; o.why = "nothing stood out"; }
    const Pat pr = pivotRead(x);   // (1.1.0) a pivot read leads: its verdict, its sentence first
    if (pr.side) { o.verdict = pr.side; o.head = pr.head; o.why = pr.why; o.ex.push_back(pr.sent); }
    // the sentences: the book first, then the aggression, then what to watch
    if (!x.bookOk) o.ex.push_back("No Book Flow record for this candle - the read is the delta alone.");
    else if (bk > 0) o.ex.push_back(r > 0 ? "Bids stacked " + xx(r) + " more than offers - buyers are leaving orders under price." : "Bids stacked while offers thinned - buyers are leaving orders under price.");
    else if (bk < 0) o.ex.push_back(r > 0 ? "Offers stacked " + xx(r) + " more than bids - sellers are leaving orders over price." : "Offers stacked while bids thinned - sellers are leaving orders over price.");
    else o.ex.push_back("Bids and offers stacked about the same.");
    if (x.deltaOk) {
        if (byC) o.ex.push_back("Delta went from " + signedC(x.dlPrev) + " to " + signedC(x.dl) + ": " + (chg > 0 ? "buyers" : "sellers") + " stepped in " + xx(x.chgX) + ".");
        else if (ps) o.ex.push_back("Delta " + signedC(x.dl) + " (" + xx(x.dlX) + " normal): " + (ps > 0 ? "buyers" : "sellers") + " pushing.");
        else o.ex.push_back("Delta " + signedC(x.dl) + " - no strong push either way.");
    }
    if (o.verdict == 2 && !pr.side) o.ex.push_back(bk > 0 ? "Watch the bids: if they hold while sellers hit, that is absorption (bullish); if they pull, sellers take over."
                                              : "Watch the offers: if they hold while buyers lift, that is absorption (bearish); if they pull, buyers take over.");
    // the table: the passive side (Bids / Offers), the biggest order (Buy / Sell), the aggressive side (Value / vs normal)
    if (x.bookOk) {
        o.rows.push_back(Row{ "", "Bids", "Offers", 1, -1, true });
        o.rows.push_back(Row{ "Stacking", signedC(x.add[0] - x.pull[0]), signedC(x.add[1] - x.pull[1]), 1, -1 });
        o.rows.push_back(Row{ "Pulled near", commas(x.pullNear[0]), commas(x.pullNear[1]), 1, -1 });
        o.rows.push_back(Row{ "Refilled", commas(x.ref[0]), commas(x.ref[1]), 1, -1 });
        o.rows.push_back(Row{ "Icebergs", commas(x.ice[0]), commas(x.ice[1]), 1, -1 });
        o.rows.push_back(Row{ "Traded", commas(x.hit[0]) + " hit", commas(x.hit[1]) + " lifted", 1, -1 });
        if (x.bigBuy > 0 || x.bigSell > 0) { o.rows.push_back(Row{ "", "Buy", "Sell", 1, -1, true }); o.rows.push_back(Row{ "Biggest order", commas(x.bigBuy) + " lots", commas(x.bigSell) + " lots", 1, -1 }); }
    }
    if (x.deltaOk) {
        o.rows.push_back(Row{ "", "Value", "vs normal", 0, 0, true });
        o.rows.push_back(Row{ "Delta", signedC(x.dl), x.dlX > 0 ? xx(x.dlX) : "-", x.dl > 0 ? 1 : x.dl < 0 ? -1 : 0, 0 });
        o.rows.push_back(Row{ "Change", signedC(chg), x.chgX > 0 ? xx(x.chgX) : "-", chg > 0 ? 1 : chg < 0 ? -1 : 0, 0 });
    }
    return o;
}

// the bottom half's change label: the |change| ratio over the change bar when it is CHG_MIN x normal or more (0 = no label)
inline double changeLabel(long long chg, double medAbsChg) { if (!(medAbsChg > 0)) return 0; const double v = (double)std::llabs(chg) / medAbsChg; return v >= CHG_LABEL ? std::min(99.9, v) : 0; }


// THE CLIMAX MARK (Cx on the price chart). The 15:41 CT test of four rules on 909 pivots (turned / zone held 10 candles; base 47% / 42%):
//   biggest volume in 20+ at the pivot candle or the one before                      58% / 52%  (198)
//   + the next candle closes the other way                                           60% / 59%  (109)   <- the rule (biggest in 40+: 72% / 73%, 36 cases)
//   + the 11-13 colour / close-position rule (red closing near its low)              53% / 48%  - the colour part took winners out (CL 31%), so it is not used
// i = a closed candle that made a new 10-candle low (dir +1, a down-pivot) or high (dir -1, an up-pivot); vol[k], o[k], c[k] for candles i-60 .. i+1
// returns the index of the climax candle (i or i-1) or -1. Decided at the close of candle i+1, never changed after.
inline int biggestRun(const std::vector<double>& vol, int j) { int n = 0; for (int k = j - 1; k >= 0; --k) { if (vol[(size_t)k] < vol[(size_t)j]) ++n; else break; } return n; }
inline int climaxAt(const std::vector<double>& vol, const std::vector<double>& op, const std::vector<double>& cl, int i, int dir, int minRun = 20) {
    if (i < 1 || i + 1 >= (int)vol.size() || (int)op.size() != (int)vol.size() || (int)cl.size() != (int)vol.size()) return -1;
    const int r0 = biggestRun(vol, i), r1 = biggestRun(vol, i - 1); const int j = r0 >= r1 ? i : i - 1;
    if (std::max(r0, r1) < minRun) return -1;
    const bool conf = dir > 0 ? cl[(size_t)i + 1] > op[(size_t)i + 1] : cl[(size_t)i + 1] < op[(size_t)i + 1];
    return conf ? j : -1;
}
} // namespace cbr
