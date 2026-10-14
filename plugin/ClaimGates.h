// ClaimGates.h -- one switch per claim the 2026-10-14 internal peer review found unproven or invalid
// (research-artifacts/review/PEER-REVIEW-2026-10-14-A..E.md). Rassul approved 15:07 CT 10/14: HIDE them on every chart until the
// external audit; the code stays. 0 = hidden (default), 1 = restored. Same four gates as the Python side (analytics/lra/claim_gates.py,
// config/claim_gates.json). Restore one by defining it before this header (or with /D on the compile line) and rebuilding:
//   LRA_SHOW_STRENGTH           Dealer Read VERDICT (STRONG / OK / WEAK / BROKEN, floor / pocket text)
//   LRA_SHOW_HVL_CUSHION        "above the HVL = dealers long gamma, a cushion" cause text
//   LRA_SHOW_OPTIONS_CAUSES     options reasons shown as confirmed causes (Dealer Read OPTIONS column, forced-futures rows)
//   LRA_SHOW_ABSORPTION_REASON  the Delta Profile A (absorption) used as a REASON / vote (Tape Reader FLOW vote + "Absorbing at"
//                               message, Dealer Read footprint "Absorbed" points). The A circle on the chart is NOT gated - it stays.
// SDK-free, header-only, no state.
#pragma once
#include <string>
#ifndef LRA_SHOW_STRENGTH
#define LRA_SHOW_STRENGTH 0
#endif
#ifndef LRA_SHOW_HVL_CUSHION
#define LRA_SHOW_HVL_CUSHION 0
#endif
#ifndef LRA_SHOW_OPTIONS_CAUSES
#define LRA_SHOW_OPTIONS_CAUSES 0
#endif
#ifndef LRA_SHOW_ABSORPTION_REASON
#define LRA_SHOW_ABSORPTION_REASON 0
#endif
namespace cgate {
static const char* const VERSION = "1.0.0";   // 2026-10-14
static const bool kShowStrength = LRA_SHOW_STRENGTH != 0;
static const bool kShowHvlCushion = LRA_SHOW_HVL_CUSHION != 0;
static const bool kShowOptionsCauses = LRA_SHOW_OPTIONS_CAUSES != 0;
static const bool kShowAbsorptionReason = LRA_SHOW_ABSORPTION_REASON != 0;
// a footprint point / tag that states absorption as the cause ("Absorbed", "Absorption (FP)", "Abs", "Ab")
inline bool isAbsorptionTag(const std::string& t)
{
    if (t == "Ab" || t == "Abs" || t == "A") return true;
    std::string l; l.reserve(t.size()); for (char c : t) l += (char)((c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c);
    return l.compare(0, 5, "absor") == 0;
}
}   // namespace cgate
