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

static const char* const VERSION = "1.0.0";

struct In {
    std::string mkt, when;               // the market key (ES, NQ, CL, GC, HG, NG, EU) and the candle's time ("2:27 PM")
    bool bookOk = false;                 // the candle has a Book Flow record
    long long add[2] = { 0, 0 }, pull[2] = { 0, 0 }, pullNear[2] = { 0, 0 }, ref[2] = { 0, 0 }, ice[2] = { 0, 0 }, hit[2] = { 0, 0 };   // [0] = bids, [1] = offers; hit[0] = lots SOLD into the bids, hit[1] = lots BOUGHT from the offers
    long long bigBuy = 0, bigSell = 0;   // the biggest single market order each way
    bool deltaOk = false;                // the candle and the one before have volume at price
    long long dl = 0, dlPrev = 0;        // this candle's delta and the one before
    double dlX = 0, chgX = 0;            // |delta| and |change| x their medians over the 20 candles before (0 = not known)
};
struct Row { std::string lab, a, b; int ca = 0, cb = 0; bool head = false; };   // ca / cb: +1 green, -1 red, 0 white
struct Out { int verdict = 0; std::string head, why; std::vector<std::string> ex; std::vector<Row> rows; };   // verdict +1 / -1 / 2 mixed / 0 quiet

inline double stackMin(const std::string& m) { return (m == "ES" || m == "EU") ? 1.15 : (m == "CL" || m == "GC") ? 1.5 : 1.3; }
inline bool changeVotes(const std::string& m) { return m != "EU"; }
static const double CHG_MIN = 2.0;       // a delta change that counts: 2x normal or more
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
    if (o.verdict == 2) o.ex.push_back(bk > 0 ? "Watch the bids: if they hold while sellers hit, that is absorption (bullish); if they pull, sellers take over."
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
inline double changeLabel(long long chg, double medAbsChg) { if (!(medAbsChg > 0)) return 0; const double v = (double)std::llabs(chg) / medAbsChg; return v >= CHG_MIN ? std::min(99.9, v) : 0; }

// the median of |v| (0 when empty)
inline double medAbs(std::vector<long long> v) { if (v.empty()) return 0; for (long long& q : v) q = std::llabs(q); std::sort(v.begin(), v.end());
    const size_t n = v.size(); return n % 2 ? (double)v[n / 2] : 0.5 * ((double)v[n / 2 - 1] + (double)v[n / 2]); }

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
