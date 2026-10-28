#ifndef LS_LEVELFADE_H
#define LS_LEVELFADE_H
// LevelFade.h 1.0.0 (2026-10-28, Rassul 09:43 / 09:46 CT) the liquidity column's FADE rule, tracked BY LEVEL (side + price tick). SDK-free, C++11.
//   MOVING / QUOTING : a level whose order re-priced (lsLiquidityProfile f.state = MOVED count of that order id in 5 min) >= P.quoteMoves
//                      -> dimmed with "~" (never dropped; it keeps its rank in the top 10)
//   SAME LEVEL       : the same size (within P.sizeTol) re-added at the same price under a NEW order id is the same level, not a move:
//                      it keeps its first-seen time and its quoting flag (the flag lasts P.windowMs after the last move was seen)
//   KING             : unchanged from 2.31.0 (Rassul 09:46 "I like the way the king node is now") = rank 1 of the steady top 10;
//                      a king that is itself moving stays king and is marked "~"
// Params are a table the nightly can refit (per market); no look-ahead: only what was seen up to now.
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
namespace lfd {
static const char* const VERSION = "1.0.0";
struct Params { int quoteMoves = 1; long long windowMs = 300000; double sizeTol = 0.10; size_t maxTrack = 400; };
// per-market table (nightly refit target). quoteMoves 1 = any re-price of the order dims it (LP counts MOVED per order id)
struct MarketRow { const char* mkt; int quoteMoves; long long windowMs; double sizeTol; };
static const MarketRow TABLE[] = { { "ES", 1, 300000, 0.10 }, { "NQ", 1, 300000, 0.10 }, { "CL", 1, 300000, 0.10 }, { "GC", 1, 300000, 0.10 },
                                   { "HG", 1, 300000, 0.10 }, { "NG", 1, 300000, 0.10 }, { "EU", 1, 300000, 0.10 } };
inline Params forMarket(const std::string& m) {
    Params p; for (const MarketRow& r : TABLE) if (m.find(r.mkt) != std::string::npos || (std::string(r.mkt) == "EU" && m.find("6E") != std::string::npos)) { p.quoteMoves = r.quoteMoves; p.windowMs = r.windowMs; p.sizeTol = r.sizeTol; break; }
    return p; }
struct In  { long long key; long long lots; int moves; };
struct Tag { bool quoting = false; long long ageMs = 0; };
class Tracker {
public:
    explicit Tracker(const Params& p = Params()) : P(p) {}
    void setParams(const Params& p) { P = p; }
    // feed every level seen now; returns one Tag per input (same order)
    std::vector<Tag> update(long long nowMs, const std::vector<In>& now) {
        std::vector<Tag> out(now.size());
        for (size_t i = 0; i < now.size(); ++i) {
            const In& b = now[i]; Lv* L = find(b.key);
            if (!L) { if (T.size() >= P.maxTrack) prune(nowMs); Lv n; n.key = b.key; n.first = nowMs; n.lots = b.lots; T.push_back(n); L = &T.back(); }
            else if (nowMs - L->last > 1500 && !sameSize(L->lots, b.lots)) { L->first = nowMs; L->quoteMs = -1; }   // gone and back with another size = a new level
            // same size back at the same price (any order id) = the same level: first-seen and quoting memory kept, nothing counted as a move
            if (b.moves >= P.quoteMoves && b.moves > 0) L->quoteMs = nowMs;
            L->lots = b.lots; L->last = nowMs;
            out[i].quoting = L->quoteMs >= 0 && nowMs - L->quoteMs <= P.windowMs; out[i].ageMs = nowMs - L->first;
        }
        return out;
    }
    size_t tracked() const { return T.size(); }
private:
    struct Lv { long long key = 0, lots = 0, first = 0, last = 0, quoteMs = -1; };
    Params P; std::vector<Lv> T;
    bool sameSize(long long a, long long b) const { const double m = (double)std::max(a, b); return m <= 0 || std::fabs((double)(a - b)) <= P.sizeTol * m + 1e-9; }
    Lv* find(long long k) { for (Lv& l : T) if (l.key == k) return &l; return nullptr; }
    void prune(long long nowMs) { std::vector<Lv> k; for (const Lv& l : T) if (nowMs - l.last <= P.windowMs) k.push_back(l); T.swap(k);
        if (T.size() >= P.maxTrack) T.erase(T.begin(), T.begin() + (long)(T.size() / 4)); }
};
} // namespace lfd
#endif
