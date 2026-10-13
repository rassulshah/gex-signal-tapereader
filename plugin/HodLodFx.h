// HodLodFx.h - the 1st Extreme model inside the HOD / LOD indicator (2.0.0, Rassul 2026-10-12 21:47 "combine the hod lod indicator
// and the first extreme. it should just be the hod lod indicator").
// The model itself is unchanged: FirstExtremeLogic.h / FirstExtremeView.h (namespace fx2, the reviewed v2 research model, locked
// policy, immutable first call - parity-tested against the research package). This file only turns its View into
//   * the table's 3rd header line, and
//   * the LOD? / HOD? marker on the called candle, placed clear of the table and the other HOD / LOD blocks.
// SHADOW MODE: the call is labelled with the time it was made; a later break of the level marks it FAILED; RIGHT / WRONG after the close.
#pragma once
#include "FirstExtremeView.h"
#include "HodLodLogic.h"

namespace hlfx {

inline int fxMarket(const std::string& code) { return fx2::mktIndex(code); }   // fx2 orders the markets differently from hl

// the 3rd header line; empty when the model cannot run on this chart at all (not 3-min, no volume) - the reason goes to the status
inline std::vector<hl::Seg> headerLine(const fx2::View& V, bool have)
{
    std::vector<hl::Seg> L;
    if (!have) return L;
    const unsigned G = hl::C_GREY;
    L.push_back(hl::Seg{ "1st extreme  ", G });
    switch (V.state) {
        case fx2::ST_UNAVAILABLE: L.push_back(hl::Seg{ V.why.empty() ? std::string("no reading") : V.why, G }); return L;
        case fx2::ST_WAITING:     L.push_back(hl::Seg{ V.why.empty() ? std::string("waiting") : V.why, G }); return L;
        case fx2::ST_WATCH:
            L.push_back(hl::Seg{ "WATCH  ", hl::C_AMBER });
            L.push_back(hl::Seg{ "LOD " + fx2::pct(V.pLow), hl::C_GREEN });
            L.push_back(hl::Seg{ " \xB7 ", G });
            L.push_back(hl::Seg{ "HOD " + fx2::pct(V.pHigh), hl::C_RED });
            L.push_back(hl::Seg{ "  (70% by " + fx2::clk(V.proxy + V.deadline) + ")", G });
            break;
        case fx2::ST_NOCALL:
            L.push_back(hl::Seg{ "NO CALL", G });
            L.push_back(hl::Seg{ "  (none reached 70% by " + fx2::clk(V.proxy + V.deadline) + ")", G });
            break;
        case fx2::ST_CALLED:
            L.push_back(hl::Seg{ (V.side == 1 ? "LOD? " : "HOD? ") + fx2::pct(V.prob), V.side == 1 ? hl::C_GREEN : hl::C_RED });
            L.push_back(hl::Seg{ "  " + fx2::px(V.price, V.m) + " \xB7 formed " + fx2::clk(V.proxy + V.formed) + " \xB7 called " + fx2::clk(V.proxy + V.declared), G });
            break;
        case fx2::ST_FAILED:
            L.push_back(hl::Seg{ std::string(V.side == 1 ? "LOD?" : "HOD?") + " FAILED", G });
            L.push_back(hl::Seg{ "  " + fx2::px(V.price, V.m) + " broke " + fx2::clk(V.proxy + V.failedAt) + " \xB7 called " + fx2::clk(V.proxy + V.declared), G });
            break;
        default: break;
    }
    if (V.verdictKnown)
        L.push_back(V.verdict == 1 ? hl::Seg{ "  RIGHT", hl::C_GREEN } : V.verdict == -1 ? hl::Seg{ "  WRONG", hl::C_RED } : hl::Seg{ "  UNRESOLVED", G });
    return L;
}

// the marker on the called candle: a tick at the called price + a small box "LOD? 74%" / "called 9:42" below the low (above the
// high); if that box would touch the table or another HOD / LOD block, beside the candle (right, then left); never over a block
inline void marker(hl::Layout& L, const fx2::View& V, const hl::Box& pane, int markerX, const std::function<int(double)>& yOf, const hl::Measure& M)
{
    if (!V.called || markerX < pane.l || markerX > pane.r) return;
    const bool low = V.side == 1; const unsigned c = V.failed ? hl::C_DIM : (low ? hl::C_GREEN : hl::C_RED);
    std::string l1 = (low ? "LOD? " : "HOD? ") + (V.failed ? std::string("\xD7") : fx2::pct(V.prob));
    std::string l2 = V.failed ? "broke " + fx2::clk(V.proxy + V.failedAt) : "called " + fx2::clk(V.proxy + V.declared);
    const int FS = 9;
    int yp = yOf(V.price), mw = (std::max)(M(l1, FS, true), M(l2, FS, false)) + 10, mh = 30;
    auto clash = [&](int l, int t) {
        if (t < pane.t + 2 || t + mh > pane.b - 2 || l < pane.l + 2 || l + mw > pane.r - 2) return true;
        hl::Box me = { l - 4, t - 4, l + mw + 4, t + mh + 4 };
        for (const auto& b : L.blocks) if (hl::overlap(me, b)) return true;
        if (L.tableOk && hl::overlap(me, L.table)) return true;
        return false;
    };
    int mx = markerX - mw / 2, my = low ? yp + 10 : yp - 10 - mh; bool beside = false;
    if (clash(mx, my)) {
        int bx = markerX + 14, by = yp - mh / 2;
        if (clash(bx, by)) bx = markerX - 14 - mw;
        if (clash(bx, by)) return;                      // nowhere clear: no box (the header line still carries the call)
        mx = bx; my = by; beside = true;
    }
    hl::addLine(L, markerX - 9, yp, markerX + 9, yp, c, 2, hl::R_LABEL);
    if (beside) hl::addLine(L, mx > markerX ? markerX + 9 : mx + mw, yp, mx > markerX ? mx : markerX - 9, yp, c, 1, hl::R_LABEL);
    else hl::addLine(L, markerX, low ? yp + 1 : my + mh, markerX, low ? my : yp - 1, c, 1, hl::R_LABEL);
    hl::addRect(L, mx, my, mx + mw, my + mh, c, hl::C_PANEL, 1, hl::R_LABEL);
    hl::addText(L, mx + 5, my + 2, my + 15, l1, c, FS, true, mw - 10, hl::R_LABEL, -1);
    hl::addText(L, mx + 5, my + 15, my + 28, l2, hl::C_GREY, FS, false, mw - 10, hl::R_LABEL, -1);
    L.blocks.push_back(hl::Box{ mx, my, mx + mw, my + mh });
}

}  // namespace hlfx
