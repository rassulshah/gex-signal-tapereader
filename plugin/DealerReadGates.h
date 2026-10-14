// DealerReadGates.h -- lsDealerRead 4.6.0 (2026-10-14): the peer-review claim gates applied to a parsed Dealer file (SDK-free, tested on
// Linux by test_dealerread_460.cpp). Rassul approved the hide list 15:07 CT 10/14; ClaimGates.h holds the switches (0 = hidden).
#pragma once
#include "DealerLogic.h"
#include "ClaimGates.h"
#include <vector>
namespace drg {
// (4.6.0, 2026-10-14, Rassul 15:07 CT approval of the peer-review hide list - ClaimGates.h) applied to every parsed Dealer file, so an
// older file (written before the bridge's own gates) shows nothing hidden either: no VERDICT (the Strength STRONG / OK / WEAK / BROKEN
// and its floor / pocket text), no forced-futures FSUM / FWIN numbers, no OPTIONS-column reasons (rows with no section = options), and
// no footprint "Absorbed" point as a cause. Restore = /D LRA_SHOW_<GATE>=1 and rebuild.
inline void apply(dl::Data& D)
{
    if (!cgate::kShowStrength) { D.vLabel.clear(); D.vText.clear(); D.vPts = -1; D.vOf = 0; }
    if (!cgate::kShowOptionsCauses) { D.hasFsum = false; D.fwin.clear(); }
    std::vector<dl::Data::TRow> keep;
    for (size_t i = 0; i < D.trows.size(); i++) {
        const dl::Data::TRow& r = D.trows[i];
        if (r.sec != 'F' && !cgate::kShowOptionsCauses) continue;
        if (r.sec == 'F' && !cgate::kShowAbsorptionReason && cgate::isAbsorptionTag(r.tag)) continue;
        keep.push_back(r);
    }
    D.trows.swap(keep);
}
}   // namespace drg
