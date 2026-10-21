// BoxReadDraw.h  1.0.0  (2026-10-18) - lays out and draws the "Read" row (BoxRead.h) inside an order-flow pane's numbers box
// (lsBidAskStats 1.15.0, lsBookFlow 1.4.0). Include after OrderFlowFeed.h. The row "Read" + one empty row per text line are added to
// the box's Info, so the box GROWS to hold them (planPanel / numGeom size it); the text is wrapped to the box's inner width measured
// with the chart's font and drawn over those rows - never wider than the box, never cut (BoxRead.h fit). A pane too short for every
// row keeps the lines that fit (the clauses dropped first).
#pragma once
#include "BoxRead.h"
#include <algorithm>

namespace br {

struct Plan { int readRow = -1; std::vector<std::string> lines; int width = 0; };

// I gets the Read rows; plan(I) lays the box out (the caller's planPanel); P = the final layout
template <class PlanFn>
inline Plan addAndPlan(off::Info& I, const Read& r, const off::TextW& M, PlanFn plan, off::PanelPlan& P) {
    const off::Info base = I; Plan out; out.readRow = (int)base.rows.size();
    const Width W = [&](const std::string& t) { return M(t, TEXT_PT, false); };
    int n = MAX_LINES;
    for (int pass = 0; pass < 3; ++pass) {
        I = base;
        off::InfoRow h; h.label = "Read"; h.side[0] = h.side[1] = -1; I.rows.push_back(h);
        for (int k = 0; k < n; ++k) { off::InfoRow e; e.side[0] = e.side[1] = -1; I.rows.push_back(e); }
        P = plan(I);
        if (!P.ok) { out.lines.clear(); return out; }
        out.width = P.box.w() - 2 * off::BOX_PAD;
        const int room = std::max(1, P.num.rows - out.readRow - 1);        // the line rows the pane has room for
        out.lines = fit(r, out.width, std::min(std::max(n, MAX_LINES), room), W);
        if ((int)out.lines.size() == n) break;
        n = std::max(1, (int)out.lines.size());
    }
    return out;
}
// the y of a box row (0 = the first data row; two-column boxes have the column-head row under the header)
inline int rowY(const off::PanelPlan& P, const off::Info& I, int row) { return P.box.t + off::BOX_PAD - 1 + off::NUM_LH * (1 + (I.twoCol ? 1 : 0) + I.topLines + row); }   // (2026-10-21) + the question line

template <class X>
inline void draw(X* x, const off::PanelPlan& P, const off::Info& I, const Plan& pl, const Read& r) {
    if (!P.ok || pl.readRow < 0 || pl.readRow >= P.num.rows) return;
    const int lx = P.box.l + off::BOX_PAD;
    off::textAt(x, lx + P.num.lw + 8, rowY(P, I, pl.readRow), off::NUM_LH - 1, r.word(), (COLOR)r.color(), off::NUM_PT, true);
    for (size_t k = 0; k < pl.lines.size(); ++k) {
        const int row = pl.readRow + 1 + (int)k; if (row >= P.num.rows) break;
        off::textAt(x, lx, rowY(P, I, row), off::NUM_LH - 1, pl.lines[k], (COLOR)r.color(), TEXT_PT, false);
    }
}

} // namespace br
