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
static const char* const VERSION = "1.0.12";   // 1.0.12 (22:51 CT): SR mark back to Moderate+ runs (1.0.11's Strong+ removed 9:09); 1.0.11 1.0.11 (22:49 CT "yes go"): SR mark only for Strong+ runs (1.5x+ their typical); the Stop Run line still shows Moderate+; 1.0.10 1.0.10 (22:37 CT): a red bullish-absorption candle is confirmed by a close above its high (not any green candle); 1.0.9 1.0.9 (22:35 CT): A confirmed by its own green close, else the first green candle of the next 2 (no new low first); 1.0.8 1.0.8 (22:30 CT): A = an absorption at the pivot, confirmed by a close beyond the candle in the next 2, one per pivot; 1.0.7 1.0.7 (22:22 CT): SR pivot must end a move (low under the 2 lows before), SR candle closes in its direction; 1.0.6 1.0.6 (22:03 CT): SR only on the first bar since the down / up pivot whose low is below the pivot candle's high (mirror); 1.0.5 1.0.5 (2026-11-27 21:58 CT): each kind of stop run vs its own typical run ("1.1x typical"), Moderate+ only, local 2+ ticks past; Tags (SR / A marks); LocalTable; 1.0.4 1.0.4 (2026-11-27 20:19 CT "call Context: Stop Run instead"): the label is "Stop Run: ", so the text drops the words "stop run"; 1.0.3 1.0.3 (2026-11-27 20:17 CT): context = opposite-side stop runs only (lows for a bullish read, highs for bearish); flip effect = the price move; 1.0.2: 1.0.2 (2026-11-27 20:12 CT): "... below <level> at <time> with N.Nx burst"; 1.0.1 (20:11): the run candle's time added
static const int NORM_N = 14;
static const double LABEL_X = 1.9;
static const double KEY_TYP0 = 1.7;
static const double SR_MIN_REL = 0.75;   // (1.0.12, Rassul 22:51 CT "i don't have the 9:09 signal") back to Moderate+; was 1.5   // (1.0.11) the SR mark needs a Strong+ run (1.5x its typical): 7-market check 11-11..11-27, 1,345 pivots - Strong+ held ~75%, Moderate 64.5%, no run 60%   // the usual key-level stop-run burst when a session has fewer than 5 runs so far

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
    double cpos = 0.5;                                     // where the candle closed in its range (0 = low, 1 = high)
};

// the candle i vs the NORM_N candles before it (gaps skipped, at most 40 back); needs 5+ to be ok
inline Metrics metrics(const std::vector<Cdl>& C, int i) {
    Metrics m; if (i < 1 || i >= (int)C.size() || !C[(size_t)i].ok || !C[(size_t)i - 1].ok) return m;
    const Cdl& x = C[(size_t)i];
    m.d = x.d(); m.pd = C[(size_t)i - 1].d(); m.chg = m.d - m.pd; m.mv = x.c - x.o; m.cpos = x.h > x.l ? (x.c - x.l) / (x.h - x.l) : 0.5;
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
    if (k == FLIP) {   // (Rassul 2026-11-27 20:14 CT: the effect must be what PRICE did, not the flip again; no lot numbers)
        const std::string w = mag(kx); r.cause = (w.empty() ? std::string("Delta") : w + " delta") + " flip - " + attl + " took over";
        const double mx = m.aR > 0 ? std::fabs(m.mv) / m.aR : 0; const std::string pw = mag(mx);
        const std::string dirw = m.mv > 0 ? "up move" : m.mv < 0 ? "down move" : "no move";
        std::string e = pw.empty() ? std::string(1, (char)std::toupper((unsigned char)dirw[0])) + dirw.substr(1) : pw + " " + dirw;
        if (m.mv > 0 && m.cpos >= 0.8) e += ", closed at the high"; else if (m.mv < 0 && m.cpos <= 0.2) e += ", closed at the low";
        else if (psd != 0 && psd != asd) e += " against the flip";
        r.effect = e; r.cc = asd; r.ec = psd; return r; }
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
inline Local localRun(const std::vector<Cdl>& C, int k, int only = 0) {   // only = -1 lows, +1 highs, 0 both
    Local best; if (k < 5 || k >= (int)C.size() || !C[(size_t)k].ok) return best;
    for (int p = k - 3; p >= 2 && p >= k - 30; --p) for (int sd = -1; sd <= 1; sd += 2) {
        if (only != 0 && sd != only) continue;
        if (sd < 0 ? !swingLow(C, p) : !swingHigh(C, p)) continue;
        const double px = sd < 0 ? C[(size_t)p].l : C[(size_t)p].h; bool broken = false;
        for (int j = p + 1; j < k && !broken; ++j) if (C[(size_t)j].ok && (sd < 0 ? C[(size_t)j].l < px : C[(size_t)j].h > px)) broken = true;
        if (broken) continue;
        if (sd < 0 ? C[(size_t)k].l < px : C[(size_t)k].h > px) { if (best.k < 0 || std::fabs(px - C[(size_t)k].c) < std::fabs(best.px - C[(size_t)k].c)) { best.k = k; best.p = p; best.side = sd; best.px = px; } } }
    return best;
}
// "reclaimed" when a candle from k to i closes back across px
inline bool reclaimed(const std::vector<Cdl>& C, int k, int i, int side, double px) { for (int j = k; j <= i && j < (int)C.size(); ++j) if (C[(size_t)j].ok && (side < 0 ? C[(size_t)j].c > px : C[(size_t)j].c < px)) return true; return false; }

// The local swing runs of every candle, computed once per pass (the plugin's draw): lo[k] / hi[k] = the run of candle k below a swing low /
// above a swing high (or none), x[k] = its burst (journal candle line, stops' side), typ[k] = the TYPICAL local run before k = the median burst
// of the local runs in the 400 candles before (the last 60, 5+ needed, else 1.0).
struct LocalTable { std::vector<Local> lo, hi; std::vector<double> xlo, xhi, typ; };
template <class Burst>
inline LocalTable localTable(const std::vector<Cdl>& C, const std::vector<long long>& keys, Burst burst) {
    LocalTable T; const size_t n = C.size(); T.lo.resize(n); T.hi.resize(n); T.xlo.assign(n, -1); T.xhi.assign(n, -1); T.typ.assign(n, 1.0);
    std::vector<std::pair<int, double> > all;   // (candle, burst) of every local run, oldest first
    for (int k = 0; k < (int)n; ++k) {
        T.lo[(size_t)k] = localRun(C, k, -1); T.hi[(size_t)k] = localRun(C, k, 1);
        if (T.lo[(size_t)k].k >= 0) T.xlo[(size_t)k] = burst(keys[(size_t)k], -1);
        if (T.hi[(size_t)k].k >= 0) T.xhi[(size_t)k] = burst(keys[(size_t)k], 1);
        // the typical run before k: from the runs already listed (candles < k)
        std::vector<double> v; for (size_t q = all.size(); q-- > 0 && v.size() < 60;) { if (all[q].first < k - 400) break; v.push_back(all[q].second); }
        if (v.size() >= 5) T.typ[(size_t)k] = median(v);
        const Local B = localRun(C, k);   // the run the old rule counted (either side, nearest to the close)
        if (B.k >= 0) { const double bx = burst(keys[(size_t)k], B.side); if (bx > 0) all.push_back(std::make_pair(k, bx)); }
    }
    return T;
}

// i = the candle read; keys[i] = its time key; runs = every key-level run decision (any grade); burst(end, side) = that candle's strongest
// one-second small-order burst on the side (-1 sellers, +1 buyers) from the journal, < 0 = unknown; clock(i) = "9:00"; tick = the tick size.
// dir = the read's direction (+1 bullish, -1 bearish, 0 none): a bullish read shows only stop runs BELOW lows, a bearish read only ABOVE highs
// (Rassul 2026-11-27 20:17 CT "obviously you would need a stop run of lows to explain bullish reversals and vice versa"); dir 0 = no line.
// 1.0.5 (Rassul 21:57-21:58 CT "the measurement of sr against key levels vs local levels .. shouldn't they be measured differently" - "do it"):
// each kind is measured against ITS OWN typical run (key-level runs vs the key-level runs before, local vs local) and the number shown is that
// ratio ("1.1x typical"); a run needs Moderate+ (0.75x its typical or more) - a key run also passes lsSR's full test (burst 1.5x+), a local run
// 2+ ticks past the swing with a known burst; rec = price closed back across the run's level by candle i (SR mark only then).
struct Ctx { bool ok = false; std::string text; int col = 0; bool rec = false; int side = 0; int k = -1; double rel = 0; double px = 0; long long vol = 0; bool key = false; std::string lvl; };   // px = the level / swing price; vol = the stop lots (key runs)   // k = the run candle
template <class Clock>
inline Ctx context(const std::vector<Cdl>& C, const std::vector<long long>& keys, int i, const std::vector<KeyRun>& runs, const LocalTable& T, Clock clock, int dir, double tick) {
    Ctx out; if (i < 0 || i >= (int)C.size() || dir == 0 || T.lo.size() != C.size()) return out; const int want = dir > 0 ? -1 : 1;
    // typical key-level run: the real key-level runs (burst 1x+) decided before this candle (5+), else 1.0
    std::vector<double> kx; for (const KeyRun& r : runs) if (r.end < keys[(size_t)i] && r.x >= 1.0) kx.push_back(r.x);
    while (kx.size() < 5) kx.push_back(KEY_TYP0);   // too few runs yet: pad with the usual key-level run (journals 11-20 / 11-27: ES 1.5-1.7, NQ 1.4)
    const double kTyp = median(kx);
    const KeyRun* best = nullptr; int bk = -1;
    for (int k = i; k >= 0 && k >= i - 3; --k) for (const KeyRun& r : runs)
        if (r.end == keys[(size_t)k] && r.side == want && r.x >= 1.5 && r.x / kTyp >= 0.75 && (!best || r.x > best->x)) { best = &r; bk = k; }
    if (best) {
        const double rel = best->x / kTyp; const int sd = best->side;
        out.ok = true; out.side = sd; out.k = bk; out.rel = rel; out.px = best->px; out.vol = best->vol; out.key = true; out.lvl = levelName(best->lvl); out.rec = reclaimed(C, bk, i, sd, best->px);
        out.text = srWord(rel) + (sd < 0 ? " below " : " above ") + levelName(best->lvl) + " at " + clock(bk) + ", " + fx(rel, 1) + "x typical";
        out.col = out.rec ? (sd < 0 ? 1 : -1) : (sd < 0 ? -1 : 1); return out;
    }
    for (int k = i; k >= 0 && k >= i - 3; --k) {
        const Local& L = want < 0 ? T.lo[(size_t)k] : T.hi[(size_t)k]; if (L.k < 0) continue;
        const double x = want < 0 ? T.xlo[(size_t)k] : T.xhi[(size_t)k]; if (x <= 0) continue;
        const double typ = T.typ[(size_t)k] > 0 ? T.typ[(size_t)k] : 1.0, rel = x / typ; if (rel < 0.75) continue;
        const double past = L.side < 0 ? L.px - C[(size_t)k].l : C[(size_t)k].h - L.px; if (past < 2 * tick - 1e-9) continue;
        out.ok = true; out.side = L.side; out.k = k; out.rel = rel; out.px = L.px; out.key = false; out.lvl = "the " + clock(L.p) + (L.side < 0 ? " low" : " high"); out.rec = reclaimed(C, k, i, L.side, L.px);
        out.text = srWord(rel) + " local" + (L.side < 0 ? " below the " : " above the ") + clock(L.p) + (L.side < 0 ? " low" : " high")
            + " at " + clock(k) + ", " + fx(rel, 1) + "x typical";
        out.col = out.rec ? (L.side < 0 ? 1 : -1) : (L.side < 0 ? -1 : 1); return out;
    }
    return out;
}

// A on candle i (bullish; bearish = mirror) - Rassul 22:30 CT "add the other signals, first lets start with A. similar logic, pivot bar etc":
// the read of candle i is an ABSORPTION by the bids (sellers' hits 1.5x+ absorbed - bids building or bid icebergs), the DOWN PIVOT = the lowest
// candle of i-2..i ended a down move (its low under the lows of the 2 candles before it), candle i's low is below the pivot candle's high
// (at the turn), a candle of the next 2 closes above candle i's high (confirmed - the A is decided then and never changes), and no candle from
// the pivot to i-1 already had an A (one A per pivot). conf = the candle that confirmed it (the A shows from that candle's close on).
inline int absorbSide(const std::vector<Cdl>& C, int i) {   // +1 bids absorbed sellers, -1 offers absorbed buyers, 0 no absorption read
    const Read r = read(metrics(C, i)); if (!r.ok || r.cause.find("absorbing") == std::string::npos) return 0; return r.ec != 0 ? r.ec : r.cc; }
inline bool aBasic(const std::vector<Cdl>& C, int i, int dir, int* pivot, int* conf) {
    if (i < 2 || i >= (int)C.size() || !C[(size_t)i].ok || absorbSide(C, i) != dir) return false;
    int p = -1; for (int j = i - 2; j <= i; ++j) if (C[(size_t)j].ok && (p < 0 || (dir > 0 ? C[(size_t)j].l < C[(size_t)p].l : C[(size_t)j].h > C[(size_t)p].h))) p = j;
    if (p < 0) return false;
    for (int j = p - 1, n = 0; j >= 0 && n < 2; --j) { if (!C[(size_t)j].ok) continue; ++n; if (dir > 0 ? C[(size_t)j].l <= C[(size_t)p].l : C[(size_t)j].h >= C[(size_t)p].h) return false; }
    if (dir > 0 ? !(C[(size_t)i].l < C[(size_t)p].h) : !(C[(size_t)i].h > C[(size_t)p].l)) return false;
    // 1.0.9 (Rassul 22:35 CT "if the candle is green you would show the A, if it is red, wait for a green candle and then show the A"):
    // confirmed by the candle itself when it closes up (green); a red one by a close ABOVE ITS HIGH in the next 2 (Rassul 22:37 CT correction:
    // "requiring a candle close above the high to confirm absorbtion on a prior red candle") - and price must not trade below the pivot low
    // before it (mirror for bearish)
    int cf = -1;
    for (int j = i; j <= i + 2 && j < (int)C.size(); ++j) { const Cdl& q = C[(size_t)j]; if (!q.ok) continue;
        if (j > i && (dir > 0 ? q.l < C[(size_t)p].l : q.h > C[(size_t)p].h)) break;
        if (j == i ? (dir > 0 ? q.c > q.o : q.c < q.o) : (dir > 0 ? q.c > C[(size_t)i].h : q.c < C[(size_t)i].l)) { cf = j; break; } }   // 1.0.10: a red one needs a close above its high
    if (cf < 0) return false;
    if (pivot) *pivot = p;
    if (conf) *conf = cf;
    return true;
}
inline bool aMark(const std::vector<Cdl>& C, int i, int dir, int last) {   // last = the newest closed candle (the confirmation must be closed)
    int p = -1, cf = -1; if (!aBasic(C, i, dir, &p, &cf) || cf > last) return false;
    for (int j = std::max(0, p); j < i; ++j) { int pj = -1, cj = -1; if (aBasic(C, j, dir, &pj, &cj) && cj <= last && pj == p) return false; }
    return true;
}

// The marks over a candle's ratios (Rassul 21:39 CT "add an SR on top of the ratio", 21:58 "add an A for absorbtion signals on bars where
// appropriate"): srUp = a reclaimed stop run of LOWS (for a green ratio), srDn = of HIGHS (red ratio); a = +1 / -1 when the read is an
// absorption (bids / icebergs absorbing the sellers = +1, offers absorbing the buyers = -1), 0 none. Closed candles only.
// the two bar heights' ratios for candle i (0 = no bar), with the passive side's sign
struct BarPair { double agg = 0; int aSide = 0; double pas = 0; int pSide = 0; };
inline BarPair bars(const std::vector<Cdl>& C, int i) { BarPair b; const Metrics m = metrics(C, i); if (!m.ok) return b; b.agg = m.dr; b.aSide = m.d >= 0 ? 1 : -1; if (m.pi) { b.pas = m.bs; b.pSide = m.imb >= 0 ? 1 : -1; } return b; }
struct Tags { bool srUp = false, srDn = false; int a = 0; };   // a = +1 / -1 an A (aMark) on the candle
// SR on candle i for a bullish ratio (dir +1; bearish = mirror) - Rassul 22:03 CT "the bar low should be below the high of the dn pivot and only
// one SR on a bar since the dn pivot": a reclaimed stop run of lows (this candle or the 3 before), the candle has a green ratio label (1.9x+),
// the DOWN PIVOT = the lowest candle from the run candle to i, the candle's low is below that pivot candle's high (still at the turn), and no
// candle from the pivot to i-1 already qualified (one SR per pivot; a new lower low = a new pivot, a new SR allowed). Decided at i's close.
template <class Clock>
inline bool srBasic(const std::vector<Cdl>& C, const std::vector<long long>& keys, int i, const std::vector<KeyRun>& runs, const LocalTable& T, Clock clock, double tick, int dir, int* pivot) {
    if (i < 0 || i >= (int)C.size() || !C[(size_t)i].ok) return false;
    const BarPair bp = bars(C, i);
    const bool lab = (bp.agg >= LABEL_X && bp.aSide == dir) || (bp.pas >= LABEL_X && bp.pSide == dir); if (!lab) return false;
    const Ctx x = context(C, keys, i, runs, T, clock, dir, tick); if (!x.ok || !x.rec || x.k < 0 || x.rel < SR_MIN_REL) return false;
    int p = x.k; for (int j = x.k; j <= i; ++j) if (C[(size_t)j].ok && (dir > 0 ? C[(size_t)j].l < C[(size_t)p].l : C[(size_t)j].h > C[(size_t)p].h)) p = j;
    if (pivot) *pivot = p;
    // 1.0.7 (Rassul 22:22 CT "the point of the stop run is to distinguish the reversals ... coming from the pivot"): the pivot must END a move
    // (its low under the lows of the 2 candles before it; high over the highs for a bearish SR) and the SR candle must be a reversal candle
    // (closes up for green, down for red)
    for (int j = p - 1, n = 0; j >= 0 && n < 2; --j) { if (!C[(size_t)j].ok) continue; ++n; if (dir > 0 ? C[(size_t)j].l <= C[(size_t)p].l : C[(size_t)j].h >= C[(size_t)p].h) return false; }
    if (dir > 0 ? !(C[(size_t)i].c > C[(size_t)i].o) : !(C[(size_t)i].c < C[(size_t)i].o)) return false;
    return dir > 0 ? C[(size_t)i].l < C[(size_t)p].h : C[(size_t)i].h > C[(size_t)p].l;
}
template <class Clock>
inline bool srMark(const std::vector<Cdl>& C, const std::vector<long long>& keys, int i, const std::vector<KeyRun>& runs, const LocalTable& T, Clock clock, double tick, int dir) {
    int p = -1; if (!srBasic(C, keys, i, runs, T, clock, tick, dir, &p)) return false;
    for (int j = std::max(0, p); j < i; ++j) { int pj = -1; if (srBasic(C, keys, j, runs, T, clock, tick, dir, &pj) && pj == p) return false; }
    return true;
}
// The marks over a candle's ratios (Rassul 21:39 CT "add an SR on top of the ratio", 21:58 "add an A for absorbtion signals on bars where
// appropriate"): srUp / srDn as srMark above; a = +1 / -1 when the read is an absorption (bids / icebergs absorbing the sellers = +1, offers
// absorbing the buyers = -1), 0 none. Closed candles only.
template <class Clock>
inline Tags tags(const std::vector<Cdl>& C, const std::vector<long long>& keys, int i, const std::vector<KeyRun>& runs, const LocalTable& T, Clock clock, double tick) {
    Tags g; const int last = (int)C.size() - 1;   // C holds closed candles only
    g.a = aMark(C, i, 1, last) ? 1 : aMark(C, i, -1, last) ? -1 : 0;
    g.srUp = srMark(C, keys, i, runs, T, clock, tick, 1); g.srDn = srMark(C, keys, i, runs, T, clock, tick, -1); return g;
}

} // namespace prd
