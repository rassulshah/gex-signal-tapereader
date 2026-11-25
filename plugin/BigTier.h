// BigTier.h -- Rassul's Big Lot S20 / S10 / S5 tiers for MARKET (aggressor) orders, per market, RTH vs overnight. SDK-free.
// (2026-11-24 20:00 CT "Go": layer 4 of his volume model - magnitude - graded, so "Biggest 41" reads "Biggest 41 (S20)".)
// Tiers = the sizes at or above which ~20 / 10 / 5 aggressor orders occur per session (his FINAL method of 2026-10-17, agreed values):
//   ES RTH 273 / 369 / 499, ON 124 / 170 / 234   NQ 92 / 122 / 162, 55 / 73 / 97   CL 70 / 101 / 144, 57 / 80 / 113   GC 32 / 43 / 56, 39 / 51 / 67
//   HG 23 / 33 / 47, 20 / 28 / 39   NG 69 / 96 / 132, 54 / 75 / 104   6E (EU) 95 / 124 / 160, 99 / 128 / 165
// RTH = the candle STARTS 8:30 - 14:59 CT. An unknown market = no tier (the size is shown alone).
#pragma once
#include <string>

namespace btr {

static const char* const VERSION = "1.0.0";
struct Tiers { long long s20 = 0, s10 = 0, s5 = 0; bool ok = false; };
inline Tiers mkt(const std::string& m, bool rth) {
    struct R { const char* n; long long r[3], o[3]; };
    static const R T[] = { { "ES", { 273, 369, 499 }, { 124, 170, 234 } }, { "NQ", { 92, 122, 162 }, { 55, 73, 97 } }, { "CL", { 70, 101, 144 }, { 57, 80, 113 } },
                           { "GC", { 32, 43, 56 }, { 39, 51, 67 } }, { "HG", { 23, 33, 47 }, { 20, 28, 39 } }, { "NG", { 69, 96, 132 }, { 54, 75, 104 } },
                           { "6E", { 95, 124, 160 }, { 99, 128, 165 } }, { "EU", { 95, 124, 160 }, { 99, 128, 165 } } };
    Tiers t; for (const R& x : T) if (m == x.n) { const long long* v = rth ? x.r : x.o; t.s20 = v[0]; t.s10 = v[1]; t.s5 = v[2]; t.ok = true; break; }
    return t;
}
// "S5" / "S10" / "S20" / "" - the highest tier the order reaches
inline std::string tier(long long lots, const std::string& m, bool rth) {
    const Tiers t = mkt(m, rth); if (!t.ok || lots <= 0) return std::string();
    return lots >= t.s5 ? "S5" : lots >= t.s10 ? "S10" : lots >= t.s20 ? "S20" : std::string();
}
// "41 (S20)" or "11"
inline std::string lotsText(long long lots, const std::string& m, bool rth) { const std::string t = tier(lots, m, rth); return std::to_string(lots) + (t.empty() ? std::string() : " (" + t + ")"); }
// a candle that starts 8:30 - 14:59 CT; localEndSec = the candle's end as local (CT) seconds, spb = seconds per bar
inline bool rthOf(long long localEndSec, long long spb) { const long long s = localEndSec - (spb > 0 ? spb : 0); const long long m = (((s % 86400) + 86400) % 86400) / 60; return m >= 510 && m < 900; }

} // namespace btr
