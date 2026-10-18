#ifndef LS_COBODDS_H
#define LS_COBODDS_H
// CobOdds.h 1.0.0 (2026-10-18, lsCobImpulse 0.4.0) -- the live odds of a CoB at its close, from the CoB Lab's model (lra.cob_lab, question
// COB_1R in lsFlexLevels\LRA-Odds-<MKT>.csv, read with OddsTable.h). SDK-free, unit-tested on Linux.
// Rassul 2026-10-17 23:14: "I should know the probability of a cob working on the fly when it happens".
// The facts are the lab's own, decision time only (bars 0 .. the CoB bar), the same buckets as lra.cob_lab.features():
//   side   long / short
//   risk   stop distance (1 tick beyond the RTH extreme of the day) / ATR(20 bars, incl. the CoB bar): < 1.5 tight, < 3 normal, else wide
//   legb   bars in the CoB leg (= his zzobars): < 4, 4-5, 6-10, 11+
//   hv     the RTH extreme's bar was the biggest volume in 35+ bars, or >= 2x its 20-bar average volume: yes / no
//   imp    a bar from the extreme to 3 bars after it with range >= 1.5x ATR(20) closing the CoB's way: yes / no
//   tod    first30 (< 33 min after the RTH open), am (to 11:00 CT), mid (to 1:00 PM), late
//   ru     RTH range so far / the average RTH range of the 20 prior sessions (10+): < 0.4 low, < 0.7 mid, else high
// The table decides which of these it uses (its keys); a key we cannot compute is '?' and the lookup widens past it.
// Recorded once at the CoB close in a sidecar <stem>-<session>.odds.csv (never rewritten) -> never repaints.
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include "CobImpulseLogic.h"
#include "OddsTable.h"

namespace codds {

static const char* const VERSION = "1.0.0";
static const char* const QID = "COB_1R";
static const char* const SIDECAR_HEADER = "trig_time|side|p|n|base|live|key|facts";

inline double trAt(const std::vector<cimp::Bar>& A, long i) {
    const cimp::Bar& b = A[(size_t)i];
    if (i <= 0) return b.h - b.l;
    const double pc = A[(size_t)i - 1].c;
    return std::max(b.h - b.l, std::max(std::fabs(b.h - pc), std::fabs(b.l - pc)));
}
inline double atr20(const std::vector<cimp::Bar>& A, long i) {
    if (i < 19) return 0;
    double s = 0;
    for (long k = i - 19; k <= i; ++k) { if (!cimp::barOk(A[(size_t)k])) return 0; s += trAt(A, k); }
    return s / 20.0;
}
inline double vma20(const std::vector<cimp::Bar>& A, long i) {
    if (i < 19) return 0;
    double s = 0;
    for (long k = i - 19; k <= i; ++k) s += cimp::fin(A[(size_t)k].v) ? A[(size_t)k].v : 0;
    return s / 20.0;
}
inline std::string bucket(double x, double c1, double c2, const char* a, const char* b, const char* c) { return x < c1 ? a : (x < c2 ? b : c); }

struct Facts { bool ok = false; std::string why; std::map<std::string, std::string> kv; double riskAtr = 0, ru = -1; int leg = -1; long ext = -1; };

// A / ct = the chart's closed bars and their Central seconds; ai = the CoB bar; legBars = leg end - pivot (cimp::Tag), -1 unknown
inline Facts facts(const std::vector<cimp::Bar>& A, const std::vector<long long>& ct, long ai, int side, double tick, int rthFirst, int rthLast, int legBars)
{
    Facts F; const bool bull = side > 0;
    if (ai < 20 || ai >= (long)A.size() || ai >= (long)ct.size() || ct[(size_t)ai] <= 0) { F.why = "not enough bars"; return F; }
    F.kv["side"] = bull ? "long" : "short";
    const long long sd = cimp::sessDayOf(ct[(size_t)ai]);
    const int modJ = cimp::minuteOf(ct[(size_t)ai]);
    // the RTH extreme of the day so far (first occurrence, as numpy argmin / argmax)
    double hi = -1e300, lo = 1e300; long ext = -1;
    for (long a = ai; a >= 0 && cimp::sessDayOf(ct[(size_t)a]) == sd; --a) {
        const int m = cimp::minuteOf(ct[(size_t)a]);
        if (m < rthFirst || m > rthLast || !cimp::barOk(A[(size_t)a])) continue;
        hi = std::max(hi, A[(size_t)a].h); lo = std::min(lo, A[(size_t)a].l);
        if (bull ? (A[(size_t)a].l <= (ext < 0 ? 1e300 : A[(size_t)ext].l)) : (A[(size_t)a].h >= (ext < 0 ? -1e300 : A[(size_t)ext].h))) ext = a;
    }
    if (ext < 0) { F.why = "no RTH bars"; return F; }
    F.ext = ext;
    const double atr = atr20(A, ai);
    if (atr > 0) {
        const double stop = bull ? A[(size_t)ext].l - tick : A[(size_t)ext].h + tick;
        const double R = bull ? A[(size_t)ai].c - stop : stop - A[(size_t)ai].c;
        F.riskAtr = R / atr; F.kv["risk"] = bucket(F.riskAtr, 1.5, 3.0, "tight", "normal", "wide");
    }
    if (legBars >= 0) { F.leg = legBars; F.kv["legb"] = legBars < 4 ? "<4" : legBars < 6 ? "4-5" : legBars < 11 ? "6-10" : "11+"; }
    { const int nv = cimp::volSince(A, (int)ext, 100); const double vm = vma20(A, ext);
      F.kv["hv"] = (nv >= 35 || (vm > 0 && A[(size_t)ext].v >= 2 * vm)) ? "yes" : "no"; }
    { bool imp = false;
      for (long k = ext; k <= std::min(ext + 3, ai); ++k) { const double a = atr20(A, k); const cimp::Bar& b = A[(size_t)k];
          if (a > 0 && (b.h - b.l) >= 1.5 * a && (bull ? b.c > b.o : b.c < b.o)) imp = true; }
      F.kv["imp"] = imp ? "yes" : "no"; }
    F.kv["tod"] = modJ - rthFirst < 33 ? "first30" : modJ <= 11 * 60 ? "am" : modJ <= 13 * 60 ? "mid" : "late";
    // the average RTH range of the 20 sessions before this one (10+ needed)
    std::vector<double> rr; long long cur = -1; double h = -1e300, l = 1e300;
    for (long a = 0; a < ai && a < (long)ct.size(); ++a) {
        const long long d = cimp::sessDayOf(ct[(size_t)a]); if (d >= sd) break;
        const int m = cimp::minuteOf(ct[(size_t)a]); if (m < rthFirst || m > rthLast || !cimp::barOk(A[(size_t)a])) continue;
        if (d != cur) { if (cur >= 0 && h > l) rr.push_back(h - l); cur = d; h = -1e300; l = 1e300; }
        h = std::max(h, A[(size_t)a].h); l = std::min(l, A[(size_t)a].l);
    }
    if (cur >= 0 && h > l) rr.push_back(h - l);
    if (rr.size() >= 10) {
        double s = 0; const size_t from = rr.size() > 20 ? rr.size() - 20 : 0;
        for (size_t k = from; k < rr.size(); ++k) s += rr[k];
        const double avg = s / (double)(rr.size() - from);
        if (avg > 0) { F.ru = (hi - lo) / avg; F.kv["ru"] = bucket(F.ru, 0.4, 0.7, "low", "mid", "high"); }
    }
    F.ok = true; return F;
}

struct Odds { bool decided = false; double p = -1, base = 0; int n = 0; bool live = false; std::string key, facts; };

inline Odds decide(const odt::Table& T, const Facts& F)
{
    Odds O; O.decided = true;
    std::string fs; for (const auto& kv : F.kv) { if (!fs.empty()) fs += ";"; fs += kv.first + "=" + kv.second; }
    O.facts = fs;
    if (!T.ok || !F.ok) return O;
    auto it = T.qs.find(QID); if (it == T.qs.end()) return O;
    std::vector<std::string> vals;
    for (const auto& k : it->second.keys) { auto f = F.kv.find(k); vals.push_back(f == F.kv.end() ? "?" : f->second); }
    const odt::Answer A = odt::lookup(T, QID, vals);
    O.base = A.base; O.live = A.live;
    if (A.ok) { O.p = A.p; O.n = A.n; O.key = A.matched; }
    return O;
}
// the chart label suffix: " 63%" only when known and LIVE (else nothing: the hover says why)
inline std::string labelSuffix(const Odds& O, bool show) { return (show && O.decided && O.p >= 0 && O.live) ? " " + std::to_string(odt::pct(O.p)) + "%" : std::string(); }
inline std::string hoverLine(const Odds& O)
{
    if (!O.decided) return "Odds: not decided (recorded before odds existed).";
    if (O.p < 0) return "Odds: ? - fewer than 20 past CoBs like this one (or no odds file yet).";
    std::string s = "Odds: " + std::to_string(odt::pct(O.p)) + "% reach 1R before the stop (usual " + std::to_string(odt::pct(O.base)) + "%, "
                    + std::to_string(O.n) + " past CoBs like this).";
    if (!O.live) s += " Still learning - not shown on the chart.";
    return s;
}
inline std::string fmt4(double x) { char b[32]; std::snprintf(b, sizeof(b), "%.4f", x); return b; }
inline std::string sidecarLine(long long trig, int side, const Odds& O)
{
    return std::to_string(trig) + "|" + (side > 0 ? "bull" : "bear") + "|" + (O.p < 0 ? std::string("-1") : fmt4(O.p)) + "|" + std::to_string(O.n) + "|"
           + fmt4(O.base) + "|" + (O.live ? "1" : "0") + "|" + O.key + "|" + O.facts;
}
inline bool parseSidecar(const std::string& line, long long& trig, int& side, Odds& O)
{
    std::vector<std::string> f = odt::split(line, '|');
    if (f.size() < 8) return false;
    // key holds '|'-joined values: the facts are always last, live is field 5
    char* e = nullptr; trig = std::strtoll(f[0].c_str(), &e, 10); if (!e || *e || trig <= 0) return false;
    if (f[1] == "bull") side = 1; else if (f[1] == "bear") side = -1; else return false;
    double p = 0, b = 0; int n = 0;
    if (!odt::num(f[2], p) || !odt::inum(f[3], n) || !odt::num(f[4], b) || (f[5] != "0" && f[5] != "1")) return false;
    O = Odds(); O.decided = true; O.p = p < 0 ? -1 : p; O.n = n; O.base = b; O.live = f[5] == "1";
    O.key = odt::join(f, 6, f.size() - 1); O.facts = f.back();
    return true;
}

}   // namespace codds
#endif
