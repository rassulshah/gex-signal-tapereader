// FlowRead.h  --  the order flow + pulling / stacking read of the Reversal Read pane (lsRevRead 0.12.0). SDK-free.
// (1.0.0, 2026-11-17 Rassul 06:03-06:07 CT "for the bookflow pane, move the climax , remove what is there currently now and give me more orderflow +
// pulling and stacking infomration", "i need a line clarly interpreting this information", "i like it"; 07:02 "build all , make sure that as i move
// my mouse on different bars i get that bars / waves information"; mockup claude.ai/artifact/UGwP9JwqGKAREEH3TaiK3y)
// One candle (the one under the mouse, else the last closed one), sides 0 = bids, 1 = offers:
//   Stacking  resting lots added - pulled            Pulled / Refilled  lots, and x the median of the 20 candles before
//   Traded    market orders: hit[0] = sells into the bids, hit[1] = buys into the offers      Icebergs  refilling prices seen
// The line on top: "Bearish ?: resistance building, support leaving" - every verdict carries "?" (UNPROVEN: the cut-offs are starting values,
// scored nightly before the "?" may go).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace flr {

static const char* const VERSION = "1.1.0";   // (1.1.0, 2026-11-17 Rassul 08:35-08:44 CT "a split with the left having a verdict and human readable explanation and the right side to have the data points": explain() = the plain-English sentences)
static const double STACK_X = 1.5;      // a side's net stacking is "building" at 1.5x the other side's (or the other side is not stacking at all)
static const double PULL_X = 2.0;       // "leaving": that side pulled 2x its normal or more AND stacked less than the other side
static const double ABS_X = 1.5;        // "absorbed": market orders into a side 1.5x the other way ...
static const double ABS_REFILL = 0.5;   // ... while that side refilled half of what was hit or more and did not shrink
static const int NORM_N = 20, NORM_MIN = 5;

struct In { long long add[2] = {0, 0}, pull[2] = {0, 0}, ref[2] = {0, 0}, hit[2] = {0, 0}, ice[2] = {0, 0}; double pullX[2] = {0, 0}, refX[2] = {0, 0}; };
struct Out { int verdict = 0; std::string head, why; };   // verdict: +1 bullish, -1 bearish, 2 mixed, 0 quiet

inline std::string commas(long long v) { const bool neg = v < 0; unsigned long long u = neg ? (unsigned long long)(-(v + 1)) + 1ULL : (unsigned long long)v; std::string s = std::to_string(u); for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert((size_t)i, ","); return neg ? "-" + s : s; }
inline std::string signedCommas(long long v) { return v > 0 ? "+" + commas(v) : commas(v); }
// x the median of the values before (needs NORM_MIN of them); 0 = not known
inline double timesNormal(long long now, std::vector<long long> before) {
    if ((int)before.size() < NORM_MIN) return 0;
    std::sort(before.begin(), before.end());
    const size_t n = before.size(); const double med = n % 2 ? (double)before[n / 2] : ((double)before[n / 2 - 1] + (double)before[n / 2]) / 2.0;
    return med > 0 ? (double)now / med : 0;
}
inline Out read(const In& a) {
    struct Why { int dir; const char* t; }; std::vector<Why> W;
    const long long nb = a.add[0] - a.pull[0], na = a.add[1] - a.pull[1];
    if (na > 0 && (nb <= 0 || (double)na >= STACK_X * (double)nb)) W.push_back(Why{ -1, "resistance building" });
    if (nb > 0 && (na <= 0 || (double)nb >= STACK_X * (double)na)) W.push_back(Why{ 1, "support building" });
    if (a.pullX[0] >= PULL_X && nb < na) W.push_back(Why{ -1, "support leaving" });
    if (a.pullX[1] >= PULL_X && na < nb) W.push_back(Why{ 1, "resistance leaving" });
    if (a.hit[1] > 0 && (double)a.hit[1] >= ABS_X * (double)a.hit[0] && na >= 0 && (double)a.ref[1] >= ABS_REFILL * (double)a.hit[1]) W.push_back(Why{ -1, "buyers absorbed" });
    if (a.hit[0] > 0 && (double)a.hit[0] >= ABS_X * (double)a.hit[1] && nb >= 0 && (double)a.ref[0] >= ABS_REFILL * (double)a.hit[0]) W.push_back(Why{ 1, "sellers absorbed" });
    int up = 0, dn = 0; for (const Why& w : W) (w.dir > 0 ? up : dn)++;
    Out o;
    if (!up && !dn) { o.verdict = 0; o.head = "Quiet"; return o; }
    o.verdict = up && dn ? 2 : up ? 1 : -1; o.head = o.verdict == 2 ? "Mixed ?:" : o.verdict > 0 ? "Bullish ?:" : "Bearish ?:";
    if (o.verdict == 2) { const char* u = nullptr; const char* d = nullptr; for (const Why& w : W) { if (w.dir > 0 && !u) u = w.t; if (w.dir < 0 && !d) d = w.t; } o.why = std::string(u) + ", " + d; }
    else { int n = 0; for (const Why& w : W) { if (n == 2) break; o.why += (n ? ", " : "") + std::string(w.t); ++n; } }
    return o;
}
// (1.1.0) the plain-English sentences under the verdict, most important first (the left column of the split read)
inline std::string ratio(double v) { char t[24]; std::snprintf(t, sizeof t, "%.1fx", (std::min)(v, 99.9)); return t; }
inline std::vector<std::string> explain(const In& a) {
    std::vector<std::string> E; const long long nb = a.add[0] - a.pull[0], na = a.add[1] - a.pull[1], dl = a.hit[1] - a.hit[0];
    const bool absBuy = a.hit[1] > 0 && (double)a.hit[1] >= ABS_X * (double)a.hit[0] && na >= 0 && (double)a.ref[1] >= ABS_REFILL * (double)a.hit[1];
    const bool absSell = a.hit[0] > 0 && (double)a.hit[0] >= ABS_X * (double)a.hit[1] && nb >= 0 && (double)a.ref[0] >= ABS_REFILL * (double)a.hit[0];
    if (absBuy) E.push_back("Buyers lifted " + commas(a.hit[1]) + " lots (delta " + signedCommas(dl) + ") but the offers refilled " + commas(a.ref[1]) + " - the sellers soaked it up.");
    if (absSell) E.push_back("Sellers hit " + commas(a.hit[0]) + " lots (delta " + signedCommas(dl) + ") but the bids refilled " + commas(a.ref[0]) + " - the buyers soaked it up.");
    if (na > 0 && (nb <= 0 || (double)na >= STACK_X * (double)nb))
        E.push_back(nb > 0 ? "Sellers stacked " + ratio((double)na / (double)nb) + " more offers than buyers stacked bids - resistance is building above." : "Sellers added offers while the bids shrank - resistance is building above.");
    if (nb > 0 && (na <= 0 || (double)nb >= STACK_X * (double)na))
        E.push_back(na > 0 ? "Buyers stacked " + ratio((double)nb / (double)na) + " more bids than sellers stacked offers - support is building below." : "Buyers added bids while the offers shrank - support is building below.");
    if (a.pullX[0] >= PULL_X && nb < na) E.push_back("Bids are being pulled (" + ratio(a.pullX[0]) + " normal) - support is leaving.");
    if (a.pullX[1] >= PULL_X && na < nb) E.push_back("Offers are being pulled (" + ratio(a.pullX[1]) + " normal) - resistance is leaving.");
    if (a.ice[1] > 0 && a.ice[1] >= 2 * a.ice[0]) E.push_back("Icebergs refilled on the offer side - hidden sellers.");
    else if (a.ice[0] > 0 && a.ice[0] >= 2 * a.ice[1]) E.push_back("Icebergs refilled on the bid side - hidden buyers.");
    if (!absBuy && !absSell && (a.hit[0] > 0 || a.hit[1] > 0)) {
        const bool more = dl != 0; E.push_back(more ? std::string(dl > 0 ? "Buyers" : "Sellers") + " were the more aggressive side: " + commas(a.hit[1]) + " lifted vs " + commas(a.hit[0]) + " hit (delta " + signedCommas(dl) + ")." : "Buying and selling at market were even.");
    }
    if (E.empty()) E.push_back("Both sides added and pulled about evenly - no one is leaning on the book.");
    else if (E.size() == 1 && !absBuy && !absSell && na <= 0 && nb <= 0) E.push_back("The book itself was balanced - neither side stacked more.");
    return E;
}
// the table: label, bids text, offers text, note (+1 green / -1 red / 0 white)
struct Row { std::string lab, b, a, note; int noteCol = 0; };
inline std::string withX(long long v, double x) { char t[24]; if (x > 0) { std::snprintf(t, sizeof t, "  %.1fx", (std::min)(x, 99.9)); return commas(v) + t; } return commas(v); }
inline std::vector<Row> rows(const In& a) {
    std::vector<Row> R; const long long nb = a.add[0] - a.pull[0], na = a.add[1] - a.pull[1];
    { Row r; r.lab = "Stacking"; r.b = signedCommas(nb); r.a = signedCommas(na);
      const long long big = (std::max)(nb, na), sml = (std::min)(nb, na);
      if (big > 0 && big != sml) { r.noteCol = nb > na ? 1 : -1; r.note = nb > na ? "Bids" : "Offers"; if (sml > 0) { char t[24]; std::snprintf(t, sizeof t, " %.1fx", (std::min)((double)big / (double)sml, 99.9)); r.note += t; } }
      R.push_back(r); }
    { Row r; r.lab = "Pulled"; r.b = withX(a.pull[0], a.pullX[0]); r.a = withX(a.pull[1], a.pullX[1]); R.push_back(r); }
    { Row r; r.lab = "Traded"; r.b = commas(a.hit[0]) + " hit"; r.a = commas(a.hit[1]) + " lifted"; const long long dl = a.hit[1] - a.hit[0]; r.note = "Delta " + signedCommas(dl); r.noteCol = dl > 0 ? 1 : dl < 0 ? -1 : 0; R.push_back(r); }
    { Row r; r.lab = "Refilled"; r.b = withX(a.ref[0], a.refX[0]); r.a = withX(a.ref[1], a.refX[1]); R.push_back(r); }
    { Row r; r.lab = "Icebergs"; r.b = commas(a.ice[0]); r.a = commas(a.ice[1]); R.push_back(r); }
    return R;   // by importance: a low pane drops rows from the end
}

} // namespace flr
