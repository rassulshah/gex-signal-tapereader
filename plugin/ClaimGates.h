// ClaimGates.h -- one switch per claim the 2026-10-14 internal peer review found unproven or invalid
// (research-artifacts/review/PEER-REVIEW-2026-10-14-A..E.md). Rassul approved 15:07 CT 10/14: HIDE them on every chart until the
// external audit; the code stays. 0 = hidden (default), 1 = restored. Same four gates as the Python side (analytics/lra/claim_gates.py,
// config/claim_gates.json). Restore one by defining it before this header (or with /D on the compile line) and rebuilding:
//   LRA_SHOW_STRENGTH           Dealer Read VERDICT (STRONG / OK / WEAK / BROKEN, floor / pocket text)
//   LRA_SHOW_HVL_CUSHION        "above the HVL = dealers long gamma, a cushion" cause text
//   LRA_SHOW_OPTIONS_CAUSES     options reasons shown as confirmed causes (Dealer Read OPTIONS column, forced-futures rows)
//   LRA_SHOW_ABSORPTION_REASON  the Delta Profile A (absorption) used as a REASON / vote (Tape Reader FLOW vote + "Absorbing at"
//                               message, Dealer Read footprint "Absorbed" points). The A circle on the chart is NOT gated - it stays.
// (1.1.0, 2026-10-14 18:25 CT, Rassul approved the external-audit display list - APPROVAL-LIST-2026-10-14.md) five more switches, the
// same rule (0 = hidden, default; the value is still computed and logged):
//   LRA_SHOW_CONTROL_WORD       Dealer Read header DEMAND / SUPPLY / BALANCED and its "n of m" tally (and the Trade Manager READY /
//                               "Read: YES / NO" item that leaned on it)
//   LRA_SHOW_ANALYST            Dealer Read Magnet / Pinned / Agrees / Fights lines and the Analyst pin / magnet / bias text
//   LRA_SHOW_TAPE_BIAS          Tape Reader line 1 "BULLISH n of 4" / "BEARISH n of 4" / "MIXED" and the "Against:" list
//   LRA_SHOW_TURN_ODDS          Tape Reader "Turn up at PDL 62%" / "Swing 40+ ticks 55%" / the "(58%)" on Tgt - shown as "Turn odds learning"
//   LRA_SHOW_READ_EXIT          Trade Manager in-trade HOLD / EXIT? from the Read
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
#ifndef LRA_SHOW_CONTROL_WORD
#define LRA_SHOW_CONTROL_WORD 0
#endif
#ifndef LRA_SHOW_ANALYST
#define LRA_SHOW_ANALYST 0
#endif
#ifndef LRA_SHOW_TAPE_BIAS
#define LRA_SHOW_TAPE_BIAS 0
#endif
#ifndef LRA_SHOW_TURN_ODDS
#define LRA_SHOW_TURN_ODDS 0
#endif
#ifndef LRA_SHOW_READ_EXIT
#define LRA_SHOW_READ_EXIT 0
#endif
namespace cgate {
static const char* const VERSION = "1.1.0";   // 2026-10-14 (1.0.0 15:07 CT; 1.1.0 18:25 CT audit display list)
static const bool kShowStrength = LRA_SHOW_STRENGTH != 0;
static const bool kShowHvlCushion = LRA_SHOW_HVL_CUSHION != 0;
static const bool kShowOptionsCauses = LRA_SHOW_OPTIONS_CAUSES != 0;
static const bool kShowAbsorptionReason = LRA_SHOW_ABSORPTION_REASON != 0;
static const bool kShowControlWord = LRA_SHOW_CONTROL_WORD != 0;
static const bool kShowAnalyst = LRA_SHOW_ANALYST != 0;
static const bool kShowTapeBias = LRA_SHOW_TAPE_BIAS != 0;
static const bool kShowTurnOdds = LRA_SHOW_TURN_ODDS != 0;
static const bool kShowReadExit = LRA_SHOW_READ_EXIT != 0;
// (1.1.0) a Dealer Sig / Dealer Read code that is an OPTIONS reason: the "o" prefix (Dealer Sig 1.6.0 convention) or a known options
// code from the older files (SC, Van, Chm, PutS, CallB, CallS, PutB, V+ / V-, F+ / F-, Pn, 0D, G+ / G-, Cush, Lvl, IV, Gm, Px, Np, Pt,
// Sh, Pin, PinX, NB). Footprint codes (Dem / Sup / Dm / Sp / Tr / Ex / Exh / RevBar ...) are not options codes.
inline bool isOptionsCode(const std::string& c0)
{
    std::string c = c0; while (!c.empty() && c[c.size() - 1] == '?') c.erase(c.size() - 1);
    if (c.size() >= 2 && c[0] == 'o' && c[1] >= 'A' && c[1] <= 'Z') return true;
    static const char* const K[] = { "SC", "Van", "Chm", "PutS", "CallB", "CallS", "PutB", "V+", "V-", "F+", "F-", "Pn", "0D", "G+", "G-",
                                     "Cush", "Lvl", "IV", "Gm", "Px", "Np", "Pt", "PT", "Sh", "Pin", "PinX", "NB" };
    for (size_t i = 0; i < sizeof(K) / sizeof(K[0]); ++i) if (c == K[i]) return true;
    return false;
}
// (1.1.0, audit "MARK ?": footprint facts, not proven as the turn's cause) a footprint code / chip with its "?": "Dem" -> "Dem?"
inline std::string markQ(const std::string& c) { return c.empty() || c[c.size() - 1] == '?' ? c : c + "?"; }
// a footprint point / tag that states absorption as the cause ("Absorbed", "Absorption (FP)", "Abs", "Ab")
inline bool isAbsorptionTag(const std::string& t)
{
    if (t == "Ab" || t == "Abs" || t == "A") return true;
    std::string l; l.reserve(t.size()); for (char c : t) l += (char)((c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c);
    return l.compare(0, 5, "absor") == 0;
}
}   // namespace cgate
