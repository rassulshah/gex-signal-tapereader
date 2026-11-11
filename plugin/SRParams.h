#ifndef LS_SR_PARAMS_H
#define LS_SR_PARAMS_H
// SRParams.h (1.0.0, 2026-11-10) -- per-market values of lsSR. Written by hand from the 2026-11-10 study (lra.sweep_feats / rev2), then
// ONLY by the nightly lra.sr_score (a change is accepted on days that did not choose it). recBars = the close-back window (P90 of real
// swept turns); drawRev 1 = the reversal arrow is DRAWN (the fade paid after costs: GC +0.44R / 39, HG +0.92R / 19), 0 = recorded only
// (CL -0.06R / 20, ES -0.23R / 9, NQ -0.02R / 23; NG and 6E not studied yet). rthOpen / rthClose = minutes of the CT day.
#include <string>
namespace srp {
struct Mkt { const char* name; int rthOpen, rthClose, recBars, drawRev; double burstX; };
inline const Mkt& forMarket(const std::string& m) {
    static const Mkt T[] = {
        { "GC", 440, 750,  8, 1, 2.0 }, { "HG", 430, 750,  6, 1, 2.0 }, { "CL", 480, 810, 11, 0, 2.0 }, { "ES", 510, 900, 6, 0, 2.0 },
        { "NQ", 510, 900,  7, 0, 2.0 }, { "NG", 480, 810,  8, 0, 2.0 }, { "EU", 440, 840,  8, 0, 2.0 }, { "",   510, 900, 6, 0, 2.0 } };
    for (int i = 0; i < 7; ++i) if (m == T[i].name) return T[i];
    return T[7];
}
} // namespace srp
#endif
