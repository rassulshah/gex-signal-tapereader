#ifndef LS_ENTRYFACTS_H
#define LS_ENTRYFACTS_H
// EntryFacts.h 1.0.0 (2026-11-09, Rassul 16:34 CT "Ok build" - the entry / exit liquidity badges in the Limit Profile; constraints 16:39 CT)
// lsBreakout 1.14.0 posts its DECIDED pullback labels as SignalBoard fact kind 'E' so lsDealerProfile / lsLimitProfile can put the ENT / EXIT /
// 1/2 tags on its Key Levels / Liquidity rows. Only labels the indicator actually DRAWS: decided Pb / Pboc / Pdoc after a BO / BD (E.pb with its
// boc decision; a Pb whose VAP is still loading is not drawn -> no fact) and the no-BO Rly's Pboc / Pdoc (E.rpbBoc 1 / 2). A label hidden by an
// Lvp (E.hidePb) gets no fact. Provisional Pbs (E.pbProv) never. Nothing here changes a BO / Rly / Pb / Pdoc rule or any drawing.
// Fact: bar = the label's DECISION bar as Central wall-clock seconds (the host's localSec of the IRT bar stamp, as every SignalBoard fact), side = trade direction (+1 long
// after a BO / up Rly, -1 short), lo = the PB POC (developing pullback POC) when drawn Pdoc else 0, hi = the stop (the pivot bar's low - 1 tick /
// high + 1 tick), val = the target (the wave's extreme between its start and end pivots), code = 1 Pb / 2 Pboc / 3 Pdoc (+10 = a no-BO Rly
// pullback), state 1 (decided), proven 0. SDK-free.
#include "BreakoutLogic.h"
#include "SignalBoard.h"
#include <algorithm>
#include <functional>
#include <map>
#include <vector>
namespace efx {
static const char* const VERSION = "1.0.0";
static const size_t MAX_FACTS = 40;
inline bool waveExt(const bko::Engine& E, const std::map<long long, size_t>& at, long long t0, long long t1, int dir, double& ext) {
    const auto a = at.find(std::min(t0, t1)), b = at.find(std::max(t0, t1)); if (a == at.end() || b == at.end()) return false;
    bool any = false;
    for (size_t k = a->second; k <= b->second && k < E.bars.size(); ++k) { const bko::In& x = E.bars[k];
        const double v = dir > 0 ? x.h : x.l; if (!std::isfinite(v)) continue;
        if (!any || (dir > 0 ? v > ext : v < ext)) { ext = v; any = true; } }
    return any;
}
inline bool one(const bko::Engine& E, const std::map<long long, size_t>& at, const bko::PbMark& P, int code, double pbPoc, const std::function<long long(long long)>& ct, sb::Fact& f) {
    const auto pi = at.find(P.tp); if (pi == at.end() || !(E.tick > 0)) return false;
    const bko::In& pv = E.bars[pi->second]; if (!std::isfinite(pv.h) || !std::isfinite(pv.l)) return false;
    double tgt = 0; if (!waveExt(E, at, P.tw0, P.tw1, P.dir, tgt)) return false;
    f = sb::Fact(); f.bar = ct ? ct(P.td) : P.td; f.side = P.dir > 0 ? 1 : -1; f.lo = (code % 10 == 3 && pbPoc > 0) ? pbPoc : 0.0;
    f.hi = P.dir > 0 ? pv.l - E.tick : pv.h + E.tick; f.val = tgt; f.code = code; f.state = 1; f.proven = 0;
    return f.bar > 0;
}
// the drawn, decided pullback labels, oldest first (at most MAX_FACTS, newest kept)
inline std::vector<sb::Fact> facts(const bko::Engine& E, const std::function<long long(long long)>& ct) {
    std::vector<sb::Fact> v; std::map<long long, size_t> at; for (size_t k = 0; k < E.bars.size(); ++k) at[E.bars[k].t] = k;
    for (const auto& kv : E.pb) { const bko::PbMark& P = kv.second;
        if (E.hidePb.count(std::make_pair(P.tp, P.dir))) continue;                 // an Lvp hides it: no badge
        const auto bc = E.boc.find(P.tp);
        if (E.bocLive && bc == E.boc.end()) continue;                              // VAP not loaded: the label is not drawn yet
        const int lab = bc == E.boc.end() ? 1 : bc->second == 1 ? 2 : bc->second == 2 ? 3 : 1;
        const auto dp = E.dpoc.find(P.tp); sb::Fact f;
        if (one(E, at, P, lab, dp == E.dpoc.end() ? 0.0 : dp->second, ct, f)) v.push_back(f); }
    for (const auto& kv : E.rpbBoc) { if (kv.second != 1 && kv.second != 2) continue;   // a no-BO Rly pullback is drawn only as Pboc / Pdoc
        const auto mk = E.rpb.find(kv.first); if (mk == E.rpb.end()) continue; const bko::PbMark& P = mk->second;
        if (E.hidePb.count(std::make_pair(P.tp, P.dir))) continue;
        const auto dp = E.dpoc.find(P.tp); sb::Fact f;
        if (one(E, at, P, kv.second == 2 ? 13 : 12, dp == E.dpoc.end() ? 0.0 : dp->second, ct, f)) v.push_back(f); }
    std::sort(v.begin(), v.end(), [](const sb::Fact& a, const sb::Fact& b) { return a.bar < b.bar; });
    if (v.size() > MAX_FACTS) v.erase(v.begin(), v.end() - (long)MAX_FACTS);
    return v;
}
}  // namespace efx
#endif
