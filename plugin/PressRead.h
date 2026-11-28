// PressRead.h (1.0.0, 2026-11-27) - SDK-free logic of the Reversal Read pane's PRESSURE bars and its one-line read (lsRevRead 0.45.0).
// Rassul 2026-11-27 15:06-19:31 CT (mockup DeltaChange-mockup.html v41): "replace [buy / sell volume] with change in delta", "use red and green
// for both agressive and passive", "only show 1.9 or more", "i want the format to indicate the cause first and then the effect", "the relevant
// cause is what should be indicated", "i want the most relevant, direct cause for price movement", "add the word bullish or bearish before
// absorbtion", "for effect just keep the value after the arrow", "put cause and effect on the same line with a => in between", "spell out cause and
// effect", "it can have a context line that mentions if there was a nearby stoprun", "an adjective describing the magnitude of the stop run like
// Moderate", "key levels and local stop runs need to be compared differently because key levels will have more stops".
//   * bars: per candle two ratios vs that candle's 14 candles before - LEFT aggressive = |delta| (bright green / red by sign), RIGHT passive =
//     |bids net added - offers net added| from the order record (dark green = bids built more / dark red = offers); a label at 1.9x+.
//   * read: "Cause: ... => Effect: ..." - the one direct cause of the candle's price move (absorption / flip / limit orders / one side in control /
//     stalling / balanced), magnitude words Strong 1.5x / Big 2.5x / Huge 4x.
//   * context (only near a stop run, this candle or the 3 before): a key-level run from lsSR's journal (burst >= 1.5x) or else a local 3-bar swing
//     run (burst from the journal's candle line); the word compares a run with the TYPICAL run of its own kind (key vs local) - Light / Moderate /
//     Strong / Big / Huge; "reclaimed" once a candle closes back across the level, else "holding".
// No settings. The forming candle is never read (closed candles only) - nothing repaints but the reclaimed / holding word.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace prd {
static const char* const VERSION = "1.0.0";
static const int NORM_N = 14;
static const double LABEL_X = 1.9;

struct Cdl {                       // one closed candle as the pane sees it
    bool ok = false; double o = 0, h = 0, l = 0, c = 0; long long buy = 0, sell = 0;
    bool book = false; long long nb = 0, na = 0;   // bids / offers net added (added - pulled) in lots
    long long iceB = 0, iceA = 0;                  // lots traded through the biggest bid / offer iceberg of the candle
    long long d() const { return buy - sell; }
};

inline std::string fx(double v, int dec) { char b[32]; std::snprintf(b, sizeof b, "%.*f", dec, v); return b; }
inline std::string grp(long long v, bool sign) {   // 7497 -> "7,497"; sign -> "+7,497"
    const bool neg = v < 0; unsigned long long a = (unsigned long long)(neg ? -v : v); std::string d = std::to_string(a), o;
    for (size_t i = 0; i < d.size(); ++i) { if (i && (d.size() - i) % 3 == 0) o += ','; o += d[i]; }
    return (neg ? "-" : sign ? "+" : "") + o;
}
inline std::string pxs(double v) {                // a price / points value without trailing zeros: 6.75, 19.25, 2
    char b[32]; std::snprintf(b, sizeof b, "%.6f", v); std::string s = b;
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
    return s;
}
inline std::string spx(double v) { return (v > 0 ? "+" : "") + pxs(v); }
inline std::string mag(double x) { return x >= 4 ? "Huge" : x >= 2.5 ? "Big" : x >= 1.5 ? "Strong" : ""; }
inline std::string lower(std::string s) { for (char& ch : s) ch = (char)std::tolower((unsigned char)ch); return s; }

struct Metrics {
    bool ok = false;
    long long d = 0, pd = 0, chg = 0; double dr = 0, dch = 0, avgChg = 0, aR = 0, mv = 0, ppk = 0;
    bool pi = false; double bs = 0; long long imb = 0;      // book ratio and its sign (imb = nb - na)
    double ix = 0; long long iceLots = 0; bool iceOk = false;   // icebergs on the side hit vs normal
    long long nb = 0, na = 0;
};

// the candle i vs the NORM_N candles before it (gaps skipped, at most 40 back); needs 5+ to be ok
inline Metrics metrics(const std::vector<Cdl>& C, int i) {
    Metrics m; if (i < 1 || i >= (int)C.size() || !C[(size_t)i].ok || !C[(size_t)i - 1].ok) return m;
    const Cdl& x = C[(size_t)i];
    m.d = x.d(); m.pd = C[(size_t)i - 1].d(); m.chg = m.d - m.pd; m.mv = x.c - x.o;
    double sD = 0, sC = 0, sR = 0; int nD = 0, nC = 0;
    double sB = 0; int nB = 0; double sI = 0; int nI = 0; const bool hitBids = m.d < 0;
    for (int k = i - 1; k >= 0 && k >= i - 40 && nD < NORM_N; --k) { const Cdl& a = C[(size_t)k]; if (!a.ok) continue;
        sD += (double)std::llabs(a.d()); sR += a.h - a.l; ++nD;
        if (k >= 1 && C[(size_t)k - 1].ok) { sC += (double)std::llabs(a.d() - C[(size_t)k - 1].d()); ++nC; }
        if (a.book) { sB += (double)std::llabs(a.nb - a.na); ++nB; sI += (double)(hitBids ? a.iceB : a.iceA); ++nI; } }
    if (nD < 5) return m;
    const double aD = sD / nD; m.aR = sR / nD; m.avgChg = nC ? sC / nC : 0;
    m.dr = aD > 0 ? (double)std::llabs(m.d) / aD : 0; m.dch = m.avgChg > 0 ? (double)std::llabs(m.chg) / m.avgChg : 0;
    m.ppk = m.d != 0 ? m.mv / (double)std::llabs(m.d) * 1000.0 : 0;
    if (x.book && nB >= 5 && sB > 0) { m.pi = true; m.imb = x.nb - x.na; m.bs = (double)std::llabs(m.imb) / (sB / nB); m.nb = x.nb; m.na = x.na;
        m.iceLots = hitBids ? x.iceB : x.iceA; if (nI >= 5 && sI > 0) { m.iceOk = true; m.ix = (double)m.iceLots / (sI / nI); } }
    m.ok = true; return m;
}

struct Read { bool ok = false; std::string cause, effect; int cc = 0, ec = 0; };   // colours: +1 green, -1 red, 0 gray

inline Read read(const Metrics& m) {
    Read r; if (!m.ok) return r; r.ok = true;
    const int dside = m.d >= 0 ? 1 : -1, asd = m.d > 0 ? 1 : -1, psd = m.mv > 0 ? 1 : m.mv < 0 ? -1 : 0;
    const int bside = m.imb >= 0 ? 1 : -1, bsd = m.imb > 0 ? 1 : -1;
    const std::string att = asd > 0 ? "Buyers" : "Sellers", attl = asd > 0 ? "buyers" : "sellers";
    auto bk = [](int s, bool cap) { return std::string(s > 0 ? (cap ? "Bids" : "bids") : (cap ? "Offers" : "offers")); };
    auto withMag = [&](double x, int s) { const std::string w = mag(x); return w.empty() ? bk(s, true) : w + " " + bk(s, false); };
    // the case (same order as the mockup)
    enum K { ABS, BRK, FLIP, LIM, CTRL, BAL } k = BAL; double kx = 0;
    const bool noResult = dside * m.mv <= 0.25 * m.aR;
    if (m.dr >= 1.5 && noResult && ((m.pi && bside == -dside && m.bs >= 1.0) || (m.iceOk && m.ix >= 1.5))) { k = ABS; kx = m.dr; }
    else if (m.pi && m.dr >= 1.5 && m.bs >= 1.5 && dside != bside) { k = BRK; kx = m.dr; }
    else if ((double)m.pd * (double)m.d < 0 && m.dch >= 1.9) { k = FLIP; kx = m.dch; }
    else if (m.pi && m.dr < 0.75 && m.bs >= 1.9) { k = LIM; kx = m.bs; }
    else if (m.dr >= 1.9 && (!m.pi || bside == dside)) { k = CTRL; kx = m.dr; }
    if (k == BAL) { r.cause = "Neither side pushed (attack " + fx(m.dr, 1) + "x, book " + fx(m.bs, 1) + "x)"; r.effect = "Balanced"; return r; }
    const bool stall = k == CTRL && std::fabs(m.mv) < 0.75 * m.aR && m.dr >= 2.5;
    if (k == ABS || stall) {
        if (m.pi && bsd == -asd && m.bs >= 1.0) r.cause = withMag(m.bs, bsd) + " built " + fx(m.bs, 1) + "x, absorbing " + attl + "' " + fx(m.dr, 1) + "x hits";
        else if (m.iceOk && m.ix >= 1.5) { const std::string w = mag(m.ix); r.cause = (w.empty() ? std::string() : w + " ") + (asd < 0 ? "bid" : "offer") + " icebergs " + fx(m.ix, 1) + "x absorbing " + attl + "' " + fx(m.dr, 1) + "x hits"; }
        else r.cause = att + " hit " + fx(m.dr, 1) + "x";
        r.cc = asd < 0 ? 1 : -1;
        if (stall) { r.effect = mag(m.dr) + (asd > 0 ? " buying" : " selling") + " - little result (stalling)"; r.ec = 0; }
        else { r.effect = mag(kx) + (asd < 0 ? " bullish" : " bearish") + " absorption"; r.ec = asd < 0 ? 1 : -1; }
        return r;
    }
    if (k == FLIP) { r.cause = att + " took over: delta " + grp(m.pd, true) + " to " + grp(m.d, true); r.effect = mag(kx) + (m.d > 0 ? " flip up" : " flip down"); r.cc = r.ec = m.d > 0 ? 1 : -1; return r; }
    const bool aggOk = asd == psd, pasOk = m.pi && bsd == psd;
    if (pasOk && (!aggOk || m.bs > m.dr)) { const std::string w = mag(m.bs); r.cause = withMag(m.bs, bsd) + " built " + fx(m.bs, 1) + "x"; r.effect = (w.empty() ? std::string() : w + " ") + "move by limit orders"; r.cc = r.ec = bsd; return r; }
    const std::string w = mag(m.dr); r.cause = att + " hit " + fx(m.dr, 1) + "x"; r.effect = (w.empty() ? std::string() : w + " ") + (asd > 0 ? "buying - buyers in control" : "selling - sellers in control"); r.cc = r.ec = asd; return r;
}

// the gold rows over the read
struct Row { std::string t; int col = 0; };   // col 2 = gold, +1 / -1 green / red
inline std::vector<Row> rows(const Metrics& m) {
    std::vector<Row> R; if (!m.ok) return R;
    R.push_back({ "Delta " + grp(m.d, true) + " = " + fx(m.dr, 2) + "x (prior " + grp(m.pd, true) + ")", 2 });
    R.push_back({ "DCh " + fx(m.dch, 2), 2 });
    R.push_back({ "Change " + grp(m.chg, true) + " (avg " + grp((long long)std::llround(m.avgChg), false) + ")", 2 });
    R.push_back({ "Pts/1k " + std::string(m.ppk > 0 ? "+" : "") + fx(m.ppk, 1), 2 });
    if (m.iceOk) R.push_back({ std::string("Icebergs ") + (m.d < 0 ? "bids " : "offers ") + grp(m.iceLots, false) + " lots = " + fx(m.ix, 1) + "x", 2 });
    if (m.pi) R.push_back({ "Book " + fx(m.bs, 2) + "x bids " + grp(m.nb, true) + " offers " + grp(m.na, true), m.imb >= 0 ? 1 : -1 });
    return R;
}

// ---- stop runs (context) ----
struct KeyRun { long long end = 0; std::string lvl; double px = 0; int side = 0; double x = 0; long long vol = 0, ovol = 0; double reach = 0; };   // end = chart time key
struct Ctx { bool ok = false; std::string text; int col = 0; };
inline std::string levelName(const std::string& raw) {
    std::string n = raw; size_t p = n.find_first_of("/+"); if (p != std::string::npos) n = n.substr(0, p);
    while (!n.empty() && n.back() == ' ') n.pop_back();
    if (n == "LL") return "LonLO";
    if (n == "LH") return "LonHI";
    if (n == "PHL") return "PhrL";
    if (n == "PHH") return "PhrH";
    return n;
}
inline std::string srWord(double rel) { return rel >= 4 ? "Huge" : rel >= 2.5 ? "Big" : rel >= 1.5 ? "Strong" : rel >= 0.75 ? "Moderate" : "Light"; }
inline double median(std::vector<double> v) { if (v.empty()) return 0; std::sort(v.begin(), v.end()); const size_t n = v.size(); return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]); }
// a 3-bar swing low / high at p (2 candles each side); confirmed once p + 2 closed
inline bool swingLow(const std::vector<Cdl>& C, int p) { if (p < 2 || p + 2 >= (int)C.size()) return false; for (int k = p - 2; k <= p + 2; ++k) if (!C[(size_t)k].ok) return false;
    return C[(size_t)p].l < std::min(C[(size_t)p - 1].l, C[(size_t)p - 2].l) && C[(size_t)p].l <= std::min(C[(size_t)p + 1].l, C[(size_t)p + 2].l); }
inline bool swingHigh(const std::vector<Cdl>& C, int p) { if (p < 2 || p + 2 >= (int)C.size()) return false; for (int k = p - 2; k <= p + 2; ++k) if (!C[(size_t)k].ok) return false;
    return C[(size_t)p].h > std::max(C[(size_t)p - 1].h, C[(size_t)p - 2].h) && C[(size_t)p].h >= std::max(C[(size_t)p + 1].h, C[(size_t)p + 2].h); }
struct Local { int k = -1, p = -1, side = 0; double px = 0; };
// the local swing (low for side -1) the candle k trades through: unbroken since it formed, at most 30 candles old; the one nearest k's close
inline Local localRun(const std::vector<Cdl>& C, int k) {
    Local best; if (k < 5 || k >= (int)C.size() || !C[(size_t)k].ok) return best;
    for (int p = k - 3; p >= 2 && p >= k - 30; --p) for (int sd = -1; sd <= 1; sd += 2) {
        if (sd < 0 ? !swingLow(C, p) : !swingHigh(C, p)) continue;
        const double px = sd < 0 ? C[(size_t)p].l : C[(size_t)p].h; bool broken = false;
        for (int j = p + 1; j < k && !broken; ++j) if (C[(size_t)j].ok && (sd < 0 ? C[(size_t)j].l < px : C[(size_t)j].h > px)) broken = true;
        if (broken) continue;
        if (sd < 0 ? C[(size_t)k].l < px : C[(size_t)k].h > px) { if (best.k < 0 || std::fabs(px - C[(size_t)k].c) < std::fabs(best.px - C[(size_t)k].c)) { best.k = k; best.p = p; best.side = sd; best.px = px; } } }
    return best;
}
// "reclaimed" when a candle from k to i closes back across px
inline bool reclaimed(const std::vector<Cdl>& C, int k, int i, int side, double px) { for (int j = k; j <= i && j < (int)C.size(); ++j) if (C[(size_t)j].ok && (side < 0 ? C[(size_t)j].c > px : C[(size_t)j].c < px)) return true; return false; }

// i = the candle read; keys[i] = its time key; runs = every key-level run decision (any grade); burst(end, side) = that candle's strongest
// one-second small-order burst on the side (-1 sellers, +1 buyers) from the journal, < 0 = unknown; clock(i) = "9:00"; tick for points
template <class Burst, class Clock>
inline Ctx context(const std::vector<Cdl>& C, const std::vector<long long>& keys, int i, const std::vector<KeyRun>& runs, Burst burst, Clock clock) {
    Ctx out; if (i < 0 || i >= (int)C.size()) return out;
    // typical key-level run burst: the runs decided before this candle (5+), else 1.0
    std::vector<double> kx; for (const KeyRun& r : runs) if (r.end < keys[(size_t)i] && r.x >= 1.0) kx.push_back(r.x);   // the real key-level stop runs before (burst 1x+)
    const double kTyp = kx.size() >= 5 ? median(kx) : 1.0;
    // the strongest key run (>= 1.5x) of this candle or the 3 before
    const KeyRun* best = nullptr; int bk = -1;
    for (int k = i; k >= 0 && k >= i - 3; --k) for (const KeyRun& r : runs) if (r.end == keys[(size_t)k] && r.x >= 1.5 && (!best || r.x > best->x)) { best = &r; bk = k; }
    if (best) {
        const int sd = best->side; const bool rec = reclaimed(C, bk, i, sd, best->px);
        out.ok = true;
        out.text = srWord(best->x / (kTyp > 0 ? kTyp : 1.0)) + " stop run " + (sd < 0 ? "below " : "above ") + levelName(best->lvl) + " with " + fx(best->x, 1) + "x burst";
        out.col = rec ? (sd < 0 ? 1 : -1) : (sd < 0 ? -1 : 1); return out;
    }
    // a local swing run: this candle or the 3 before (newest first); its word vs the typical local run burst (the local runs before it, 5+)
    for (int k = i; k >= 0 && k >= i - 3; --k) {
        const Local L = localRun(C, k); if (L.k < 0) continue;
        const double x = burst(keys[(size_t)k], L.side);
        std::vector<double> lx; for (int q = k - 1; q >= 5 && q >= k - 400 && lx.size() < 60; --q) { const Local Q = localRun(C, q); if (Q.k < 0) continue; const double b = burst(keys[(size_t)q], Q.side); if (b > 0) lx.push_back(b); }
        const double lTyp = lx.size() >= 5 ? median(lx) : 1.0;
        const bool rec = reclaimed(C, k, i, L.side, L.px); const double past = L.side < 0 ? L.px - C[(size_t)k].l : C[(size_t)k].h - L.px;
        out.ok = true;
        out.text = (x > 0 ? srWord(x / (lTyp > 0 ? lTyp : 1.0)) + " local" : std::string("Local")) + " stop run " + (L.side < 0 ? "below the " : "above the ") + clock(L.p) + (L.side < 0 ? " low" : " high")
            + (x > 0 ? " with " + fx(x, 1) + "x burst" : std::string());
        (void)past;
        out.col = rec ? (L.side < 0 ? 1 : -1) : (L.side < 0 ? -1 : 1); return out;
    }
    return out;
}

// the two bar heights' ratios for candle i (0 = no bar), with the passive side's sign
struct BarPair { double agg = 0; int aSide = 0; double pas = 0; int pSide = 0; };
inline BarPair bars(const std::vector<Cdl>& C, int i) { BarPair b; const Metrics m = metrics(C, i); if (!m.ok) return b; b.agg = m.dr; b.aSide = m.d >= 0 ? 1 : -1; if (m.pi) { b.pas = m.bs; b.pSide = m.imb >= 0 ? 1 : -1; } return b; }
} // namespace prd
