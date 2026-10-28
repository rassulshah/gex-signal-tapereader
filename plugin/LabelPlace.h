#ifndef LS_LABELPLACE_H
#define LS_LABELPLACE_H
// LabelPlace.h 1.0.0 (2026-10-28 09:05 CT, Rassul: "it would be horrible if ... the limit F is showing it at the wrong location ..
// we know there is a mismatch"). A price label is drawn AT ITS PRICE or not at all - never pushed to another row.
// Before: LMT F stacked colliding labels downward (t0 += row + 1, up to 6 times), so on ES 10-28 the "2x 1571" bid print
// at 7773.00 was drawn about 2 points low (~7771) and the 7772.75 / 7766 prints walked further down.
// Rule: candidates are taken BIGGEST first; one is kept only if its row at its own y overlaps neither a kept label nor a
// band already used (busy); the rest are not drawn (their facts are still posted). SDK-free, C++11.
#include <vector>
#include <algorithm>
#include <utility>
namespace lpl {
static const char* const VERSION = "1.0.0";
struct Cand { int y; long long weight; };   // y = the price's pixel row centre
// returns the indices of the candidates to draw, each at its own y (top = y - h/2, bottom = top + h)
inline std::vector<size_t> place(const std::vector<Cand>& c, int h, const std::vector<std::pair<int,int> >& busy = std::vector<std::pair<int,int> >()) {
    std::vector<size_t> ord(c.size()); for (size_t i = 0; i < c.size(); ++i) ord[i] = i;
    std::stable_sort(ord.begin(), ord.end(), [&](size_t a, size_t b) { return c[a].weight > c[b].weight; });
    std::vector<std::pair<int,int> > used(busy); std::vector<size_t> out;
    for (size_t k = 0; k < ord.size(); ++k) {
        const int t0 = c[ord[k]].y - h / 2, b0 = t0 + h; bool ok = true;
        for (size_t u = 0; u < used.size() && ok; ++u) if (!(b0 < used[u].first || t0 > used[u].second)) ok = false;
        if (!ok) continue;
        used.push_back(std::make_pair(t0, b0)); out.push_back(ord[k]);
    }
    std::sort(out.begin(), out.end());
    return out;
}
}
#endif
