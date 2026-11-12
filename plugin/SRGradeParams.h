#ifndef LS_SR_GRADE_PARAMS_H
#define LS_SR_GRADE_PARAMS_H
// SRGradeParams.h (1.0.0, 2026-11-12) -- per-market values of the stop-run STRENGTH GRADE of lsSR 1.1.0 (SRLogic.h). Written by hand from
// the 2026-11-12 replay of the engine over 41 sessions x 7 markets (3,479 runs that closed back: grade 0 -> 3% became a Cob / Cob2,
// grade 3-4 -> 17%, the same in both halves of the days and in every market), then ONLY by the nightly lra.cob_reader (a change is
// accepted on days that did not choose it).
//   bigA  = an aggressor order this big (lots, all its fills beyond the level) is a "big buyer"   (the 99.5th percentile of a session's orders)
//   bigP  = a resting order filled for this many lots beyond the level is a "big bid"             (the same percentile of resting orders)
//   share = big buyers' part of the defenders' volume;  bids = big bids needed;  deep = ATRs beyond the level;  heavy = normal candles sold
//   strong = the grade drawn on a run that is not gold;  draw 0 = logged only, 1 = drawn with "?", 2 = proven (no "?")
#include <string>
namespace srg {
struct Mkt { const char* name; double bigA, bigP, share; int bids; double deep, heavy; int strong, draw; };
inline const Mkt& forMarket(const std::string& m) {
    static const Mkt T[] = {
        { "ES", 36, 35, 0.03, 3, 1.0, 1.0, 3, 1 },
        { "NQ", 12, 11, 0.03, 3, 1.0, 1.0, 3, 1 },
        { "CL", 20, 20, 0.03, 3, 1.0, 1.0, 3, 1 },
        { "GC", 14, 14, 0.03, 3, 1.0, 1.0, 3, 1 },
        { "HG", 12, 11, 0.03, 3, 1.0, 1.0, 3, 1 },
        { "NG", 25, 25, 0.03, 3, 1.0, 1.0, 3, 1 },
        { "EU", 60, 58, 0.03, 3, 1.0, 1.0, 3, 1 },
        { "",    0,  0, 0.03, 3, 1.0, 1.0, 3, 0 } };
    for (int i = 0; i < 7; ++i) if (m == T[i].name) return T[i];
    return T[7];
}
} // namespace srg
#endif
