// BreakoutHide.h -- lsBreakout / lsBreakout1 1.11.1 (2026-11-09, Rassul HG 10:13 "if there is a cob you dont need to show a rly and if there is
// an lvp, you dont need to show a pb, otherwise they start overlapping"): DISPLAY-only suppression. lsCobSignal's rule runs natively on the
// same closed bars (cobs::Engine, as Wave POC runs it); a DRAWN Cob (shown + ZZO 10+) hides the Rly letter on its extreme (the Cob bar +- 1
// bar, the Rly drawn on the same side of the candle: a bull Cob under the low = a down Rly's end); a drawn Lvp hides the Pb on its pivot bar
// (same side). Records, Rev, Wave POC qualification and PB Rev scoring are unchanged (the Rly / Pb decisions stay).
#pragma once
#include "BreakoutLogic.h"
#include "CobSignalLogic.h"
#include <functional>
namespace bkh {
static const int COB_RLY_BARS = 6;   // (1.16.1) a Rly ending up to 6 bars before a drawn Cob is not drawn
struct Cob { cobs::Engine CB; bool ok = false; long long first = 0; int fed = 0; };
// B = the chart bars given to bko::Engine::update (the last one forming); loc = bar stamp -> local (CT) civil seconds (cobs::Bar::t)
inline void hide(bko::Engine& E, Cob& C, const std::string& mkt, double tick, int spb, const std::vector<bko::In>& B, const std::function<long long(long long)>& loc) {
    const int closed = (int)B.size() - 1; if (closed < 3) return;
    if (!C.ok || C.first != B[0].t || C.fed > closed) { C.CB.reset(); C.ok = C.CB.setMarket(mkt, tick, spb); C.first = B[0].t; C.fed = 0; }
    if (!C.ok) { E.hideRly.clear(); E.hidePb.clear(); return; }
    for (int i = C.fed; i < closed; ++i) {
        cobs::Bar b; b.t = loc(B[(size_t)i].t); b.o = B[(size_t)i].o; b.h = B[(size_t)i].h; b.l = B[(size_t)i].l; b.c = B[(size_t)i].c; b.v = (double)B[(size_t)i].vol;
        if (b.h < b.l) std::swap(b.h, b.l);
        C.CB.addBar(b); ++C.fed;
        if (C.CB.trimmed > 0) { C.CB.trimmed = 0; C.CB.reset(); C.CB.setMarket(mkt, tick, spb); C.fed = 0; return; }
    }
    C.CB.out.clear(); C.CB.lvpOut.clear();   // the records belong to lsCobSignal
    E.hideRly.clear(); E.hidePb.clear();
    for (const cobs::Mark& m : C.CB.marks) {
        if (!(m.show && m.zOk) || m.idx < 0 || m.idx >= closed) continue;
        // (1.16.1, Rassul 2026-11-10 09:17 CL "you dont have to show Rly when there is a Cob ... they start overlapping") ANY Rly (either
        // direction) whose extreme is within COB_RLY_BARS before the Cob bar to 1 after it: the CL 9:06 bull Cob sat 3 bars after the up Rly's
        // 8:57 high and both labels were drawn at the top (1.11.1 only hid the opposite-direction Rly on the Cob bar +- 1)
        for (int d = -COB_RLY_BARS; d <= 1; ++d) { const int k = m.idx + d; if (k >= 0 && k < closed) { E.hideRly.insert(std::make_pair(B[(size_t)k].t, -1)); E.hideRly.insert(std::make_pair(B[(size_t)k].t, 1)); } }
    }
    for (const cobs::LvpMark& m : C.CB.lvp.marks) { if (!m.show || m.idx < 0 || m.idx >= closed) continue; E.hidePb.insert(std::make_pair(B[(size_t)m.idx].t, m.side)); }
}
} // namespace bkh
