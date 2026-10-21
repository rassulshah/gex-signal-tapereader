#ifndef LS_TRAPLOGIC_H
#define LS_TRAPLOGIC_H
// TrapLogic.h 1.0.0 (2026-10-21, Rassul approved delta-profile-trapped-mockup-v2.png 11:43 CT "ok.. approve.. build"). SDK-free,
// unit-tested (tests_trap/test_trap.cpp). Used by lsDeltaProfile 2.21.0: the TRAPPED TRADERS read (Tr / A|Tr on the profile).
//
// Why (Delta-Profile-research-2026-10-21.md; 7 markets, 11 sessions, 16,449 local highs / lows, 3-min, decision-time "held" = a further
// 1 ATR after the candle's close before a close beyond its extreme; base 45.5%): trapped alone held 57% (+11 pts, CI +8..+14, both
// halves +12), trapped + his A 67% (+23, n 79 pooled, under 20 per market = LEARNING), the bigger the trap the further the move
// (~1.1x 53% / 1.21 ATR, ~3.3x 62% / 1.44 ATR); GC weak (47%). Thresholds as tested, never tuned.
//
// The rule, decided ONCE at the candle's close, never repainted:
//   1. the candle makes a NEW 20-candle extreme: its high > the highest high of the 20 candles before it (top) / its low < the lowest low;
//   2. the attackers' lots traded BEYOND that old extreme in this candle (bought above the old high / sold below the old low) are
//      >= minX (1.0) x the median attack of the 20 candles before it (attack = all lots the attackers traded in a candle);
//   3. the SAME candle closes back inside: close <= the old high (top) / close >= the old low.
// Top = trapped BUYERS (bearish, red); bottom = trapped SELLERS (bullish, green). The size (x) goes to the record / status / hover only.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace trapl {
static const char* const VERSION = "1.0.0";
static const int LOOKBACK = 20;        // candles for the old extreme and the median attack
static const int MIN_PRIOR = 10;       // fewer prior candles with footprint = no decision (learning)
static const double MIN_X = 1.0;       // the research threshold (trapped lots >= 1.0 x the median attack)

struct Row { long long key = 0; double bought = 0, sold = 0; };
struct Candle { long long high = 0, low = 0, close = 0; std::vector<Row> rows; };   // whole-tick keys

inline double median(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}
// all lots the attackers of a side traded in one candle (top = buyers, bottom = sellers)
inline double attackOf(const Candle& c, bool top) { double a = 0; for (const Row& r : c.rows) a += top ? r.bought : r.sold; return a; }

struct Result {
    bool fired = false; bool broke = false, backIn = false; double lots = 0, med = 0, x = 0; int prior = 0;
    long long lo = 0, hi = 0;       // the trapped rows (keys beyond the old extreme, up to the candle's extreme)
    std::string why;
};
// c = the closed candle; prevX = the old 20-candle extreme (key); priorAttack = the attack of each earlier candle with footprint
// (same side), at most LOOKBACK of them
inline Result decide(const Candle& c, bool top, long long prevX, const std::vector<double>& priorAttack, double minX = MIN_X) {
    Result r; r.prior = (int)priorAttack.size();
    r.broke = top ? c.high > prevX : c.low < prevX;
    if (!r.broke) { r.why = "no new 20-candle extreme"; return r; }
    r.lo = top ? prevX + 1 : c.low; r.hi = top ? c.high : prevX - 1;
    for (const Row& w : c.rows) if (top ? w.key > prevX : w.key < prevX) r.lots += top ? w.bought : w.sold;
    r.backIn = top ? c.close <= prevX : c.close >= prevX;
    if (r.prior < MIN_PRIOR) { r.why = "learning: fewer than 10 earlier candles with footprint"; return r; }
    r.med = median(priorAttack);
    r.x = r.lots / std::max(r.med, 1.0);
    if (r.lots <= 0) { r.why = "no attackers' lots beyond the old extreme"; return r; }
    if (!r.backIn) { r.why = "breakout accepted (closed beyond the old extreme)"; return r; }
    if (r.x < minX) { r.why = "trapped lots under the median attack"; return r; }
    r.fired = true; r.why = "trapped"; return r;
}

// ---- the record line (the Delta Profile's session file, beside F / D / E / AE / TA lines), written once at the decision:
//   TR|version|root|id|side|bar_t|bar_raw|lo_ticks|hi_ticks|tick|lots|median_attack|x|old_extreme_ticks|extreme_ticks|close_ticks   (16)
// side: top = trapped buyers (bearish), bottom = trapped sellers (bullish). bar_t = the candle's CT stamp (= the D line's decided_by key).
struct Rec { long long id = -1; bool top = false; long long barT = 0; unsigned long long barRaw = 0; long long lo = 0, hi = 0;
             double tick = 0, lots = 0, med = 0, x = 0; long long prevX = 0, extreme = 0, closeK = 0; std::string version, root; };
static const int TR_FIELDS = 16;
inline std::string num(double v) { char b[48]; std::snprintf(b, sizeof(b), "%.10g", v); return b; }
inline std::string line(const Rec& r) {
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "TR|" << r.version << '|' << r.root << '|' << r.id << '|' << (r.top ? "top" : "bottom") << '|' << r.barT << '|' << r.barRaw << '|'
      << r.lo << '|' << r.hi << '|' << num(r.tick) << '|' << num(r.lots) << '|' << num(r.med) << '|' << num(r.x) << '|' << r.prevX << '|' << r.extreme << '|' << r.closeK;
    return o.str();
}
inline bool wholeOf(const std::string& t, long long& v) {
    if (t.empty() || t.size() > 24) return false; char* e = nullptr; v = std::strtoll(t.c_str(), &e, 10); return e && !*e;
}
inline bool realOf(const std::string& t, double& v) {
    if (t.empty() || t.size() > 40) return false; char* e = nullptr; v = std::strtod(t.c_str(), &e); return e && !*e && std::isfinite(v);
}
inline bool parse(const std::string& ln, Rec& r) {
    std::vector<std::string> c; size_t a = 0;
    for (;;) { const size_t p = ln.find('|', a); c.push_back(ln.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; }
    if ((int)c.size() != TR_FIELDS || c[0] != "TR" || c[2].empty() || (c[4] != "top" && c[4] != "bottom")) return false;
    long long id, bt, br, lo, hi, px, ex, ck; double tk, lots, med, x;
    if (!wholeOf(c[3], id) || !wholeOf(c[5], bt) || !wholeOf(c[6], br) || !wholeOf(c[7], lo) || !wholeOf(c[8], hi) || !wholeOf(c[13], px) ||
        !wholeOf(c[14], ex) || !wholeOf(c[15], ck) || !realOf(c[9], tk) || !realOf(c[10], lots) || !realOf(c[11], med) || !realOf(c[12], x)) return false;
    if (id < 0 || bt <= 0 || br <= 0 || lo > hi || !(tk > 0) || lots < 0 || med < 0 || x < 0) return false;
    r.version = c[1].substr(0, 16); r.root = c[2]; r.id = id; r.top = c[4] == "top"; r.barT = bt; r.barRaw = (unsigned long long)br; r.lo = lo; r.hi = hi;
    r.tick = tk; r.lots = lots; r.med = med; r.x = x; r.prevX = px; r.extreme = ex; r.closeK = ck;
    return true;
}
// the badge text: "Tr?" alone, "A|Tr?" when his A is on the same candle and the same side (bullish A with trapped sellers)
inline std::string badge(bool withA) { return withA ? "A|Tr?" : "Tr?"; }
}  // namespace trapl
#endif
