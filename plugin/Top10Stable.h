#ifndef LS_TOP10STABLE_H
#define LS_TOP10STABLE_H
// Top10Stable.h 1.0.0 (2026-10-28, Rassul 06:38 CT "the liquidity profile ... still moves some and it shows more than the top10")
// A steady TOP-N (default 10) of resting big orders for the Dealer Profile's left LIQUIDITY column. SDK-free, C++11, no files.
//   ENTER : a price (side + tick) must be seen for >= persistMs (30 s) and, when the list is full, beat the weakest member by >= enterX (1.20x)
//   LEAVE : a member drops when its order is gone for > graceMs (10 s), or when a qualified challenger beats it (it is the weakest)
//   RANK  : on the size smoothed over smoothMs (20 s); members are re-ordered only when a lower-ranked one is >= swapX (1.25x) the one above it
//   CAP   : never more than N members; members off screen are kept in the list but not counted as shown (the caller draws on-screen ones only)
// No look-ahead: each call uses only what is in the book now and what this object saw before. Bounded: tracks at most maxTrack prices.
#include <vector>
#include <algorithm>
#include <cmath>
namespace t10 {
static const char* const VERSION = "1.0.0";
struct Params { int n = 10; long long persistMs = 30000, graceMs = 10000; double enterX = 1.20, swapX = 1.25; long long smoothMs = 20000; size_t maxTrack = 400; };
struct In  { long long key; double px; long long lots; bool bid; bool onScreen; };          // key = side * 1e9 + price in ticks
struct Out { long long key; double px; long long lots; bool bid; int rank; bool onScreen; };
class Board {
public:
    explicit Board(const Params& p = Params()) : P(p) {}
    void reset() { T.clear(); M.clear(); }
    // feed the current book (aggregated per price and side); returns the members in rank order (rank 1..n)
    std::vector<Out> update(long long nowMs, const std::vector<In>& book) {
        for (Tr& t : T) t.seenNow = false;
        for (const In& b : book) {
            Tr* t = find(b.key);
            if (!t) { if (T.size() >= P.maxTrack) prune(nowMs); Tr n; n.key = b.key; n.first = nowMs; T.push_back(n); t = &T.back(); }
            t->px = b.px; t->lots = b.lots;
            if (t->smT < 0) t->sm = (double)b.lots; else { const double a = 1.0 - std::exp(-(double)(nowMs - t->smT) / (double)std::max(1LL, P.smoothMs)); t->sm += a * ((double)b.lots - t->sm); } t->smT = nowMs; t->bid = b.bid; t->on = b.onScreen; t->last = nowMs; t->seenNow = true;
        }
        // members gone longer than the grace drop out
        M.erase(std::remove_if(M.begin(), M.end(), [&](long long k) { const Tr* t = find(k); return !t || (!t->seenNow && nowMs - t->last > P.graceMs) || !t->on; }), M.end());
        // challengers: persisted, on screen, not a member - strongest first
        std::vector<const Tr*> ch;
        for (const Tr& t : T) if (t.seenNow && t.on && nowMs - t.first >= P.persistMs && !isMember(t.key)) ch.push_back(&t);
        std::sort(ch.begin(), ch.end(), [](const Tr* a, const Tr* b) { return a->lots > b->lots; });
        for (const Tr* c : ch) {
            if ((int)M.size() < P.n) { insertSorted(c->key); continue; }
            size_t w = weakest(); const Tr* wt = find(M[w]);
            if (wt && (double)c->lots >= P.enterX * (double)effLots(*wt)) { M.erase(M.begin() + (long)w); insertSorted(c->key); } else break;   // sorted: the rest are weaker
        }
        // ranking: first time = by lots; then adjacent swaps only when clearly bigger
        for (int pass = 0; pass < P.n; ++pass) { bool sw = false;
            for (size_t i = 1; i < M.size(); ++i) if (ordLots(*find(M[i])) >= P.swapX * ordLots(*find(M[i - 1]))) { std::swap(M[i], M[i - 1]); sw = true; }
            if (!sw) break; }
        std::vector<Out> o;
        for (size_t i = 0; i < M.size(); ++i) { const Tr* t = find(M[i]); o.push_back(Out{ t->key, t->px, t->lots, t->bid, (int)i + 1, t->on }); }
        return o;
    }
    size_t tracked() const { return T.size(); }
private:
    struct Tr { long long key = 0; double px = 0; long long lots = 0; double sm = 0; long long smT = -1; bool bid = true, on = true, seenNow = false; long long first = 0, last = 0; };
    Params P; std::vector<Tr> T; std::vector<long long> M;
    Tr* find(long long k) { for (Tr& t : T) if (t.key == k) return &t; return nullptr; }
    const Tr* find(long long k) const { for (const Tr& t : T) if (t.key == k) return &t; return nullptr; }
    bool isMember(long long k) const { return std::find(M.begin(), M.end(), k) != M.end(); }
    long long effLots(const Tr& t) const { return t.seenNow ? (long long)std::llround(t.sm) : 0; }       // a member in its grace period counts as 0 for ranking
    double ordLots(const Tr& t) const { return t.sm; }                          // ordering keeps a member's last smoothed size through its grace
    void insertSorted(long long k) { const double v = ordLots(*find(k)); size_t i = 0; while (i < M.size() && ordLots(*find(M[i])) >= v) ++i; M.insert(M.begin() + (long)i, k); }
    size_t weakest() const { size_t w = 0; for (size_t i = 1; i < M.size(); ++i) if (effLots(*find(M[i])) < effLots(*find(M[w]))) w = i; return w; }
    void prune(long long nowMs) { T.erase(std::remove_if(T.begin(), T.end(), [&](const Tr& t) { return !isMember(t.key) && nowMs - t.last > P.graceMs; }), T.end());
        if (T.size() >= P.maxTrack) { std::sort(T.begin(), T.end(), [](const Tr& a, const Tr& b) { return a.last > b.last; }); while (T.size() >= P.maxTrack && !T.empty()) { if (isMember(T.back().key)) break; T.pop_back(); } } }
};
} // namespace t10
#endif
