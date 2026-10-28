// WavePOCLiq.h  1.1.0  (2026-10-28)  -  lsWavePOC 1.1.0 LIQUIDITY NODES, SDK-free (tested in tests_wavepoc\test_wavepoc_logic.cpp).
// Rassul approved 11:27 CT (mockups wavepoc_lpoc_w1 / w2, reference wp8.py): the resting real-order liquidity of each wave, drawn as
// compact bars pointing LEFT from the wave start, nodes only.
//   BOOK      IRT's live order-by-order book (DEPTH_BY_ORDER): NEW / MOD / DEL / FILL keep every resting order (price, side, lots).
//             MARKET-MAKER orders are removed: an order re-priced >= 2 times within 5 minutes does not count while that holds.
//   SAMPLE    once per second: the resting lots per price and side (within +-300 ticks of the book's middle) added to the forming
//             bar's sums; the bar's sample count kept. A wave's liquidity = the average resting lots per profile row over the wave's
//             bars (sum of samples / number of samples). Bars with no samples (before the indicator loaded) = no book: a wave needs
//             samples on >= 90% of its bars, otherwise it simply has no liquidity nodes (no error).
//   NODES     rows >= 2.0x the wave's median non-zero row; a peak on the RAW rows (a row >= both neighbours, edges included); the
//             separation exactly as the volume nodes (>= 3 rows apart, 3-row smoothed valley between <= 0.7x the smaller); max 5;
//             the biggest = LPOC. Side = bids if bids are at least half of the row's lots, else offers.
#pragma once
#include <algorithm>
#include <climits>
#include <cmath>
#include <deque>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

namespace wpl {

static const char* const VERSION = "1.1.0";
static const long long MM_WINDOW_MS = 5LL * 60 * 1000;   // re-priced >= 2x in 5 min = market maker
static const int MM_REPRICES = 2;
static const long long SAMPLE_MS = 1000;
static const long long BAND_TICKS = 300;                  // sampled prices around the book's middle
static const size_t BAR_CAP = 1500;                       // bars of samples kept
static const size_t BOOK_CAP = 400000;
static const double COVER_MIN = 0.9;                      // share of a wave's bars that must have samples

enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
struct Ev { int action = -1; int side = -1; long long tick = 0; long long size = 0; long long id = 0; };   // side 1 bid, 0 offer

struct Ord { int side = 0; long long tick = 0; long long size = 0; std::deque<long long> reprices; };

class Book {
public:
    std::unordered_map<long long, Ord> book;
    long long events = 0, bad = 0, resets = 0;
    void clear() { book.clear(); ++resets; }
    void onEvent(const Ev& e, long long nowMs) {
        ++events;
        if (e.action < NEW || e.action > FILL || e.size < 0 || e.size > 10000000) { ++bad; return; }
        auto it = book.find(e.id);
        switch (e.action) {
        case NEW: {
            if (e.side != 0 && e.side != 1) { ++bad; return; }
            if (book.size() >= BOOK_CAP && it == book.end()) clear();
            Ord o; o.side = e.side; o.tick = e.tick; o.size = e.size; book[e.id] = o; break; }
        case MOD: {
            if (it == book.end()) { if (e.side != 0 && e.side != 1) return; if (book.size() >= BOOK_CAP) clear(); Ord o; o.side = e.side; o.tick = e.tick; o.size = e.size; book[e.id] = o; break; }
            Ord& o = it->second;
            if (e.tick != o.tick) { o.reprices.push_back(nowMs); o.tick = e.tick; }
            o.size = e.size;
            while (!o.reprices.empty() && nowMs - o.reprices.front() > MM_WINDOW_MS) o.reprices.pop_front();
            if (o.reprices.size() > 8) o.reprices.pop_front();
            break; }
        case DEL: if (it != book.end()) book.erase(it); break;
        case FILL: if (it != book.end()) { Ord& o = it->second; o.size -= std::min(o.size, e.size); if (o.size <= 0) book.erase(it); } break;
        }
    }
    static bool marketMaker(const Ord& o, long long nowMs) {
        int n = 0; for (long long t : o.reprices) if (nowMs - t <= MM_WINDOW_MS) ++n;
        return n >= MM_REPRICES;
    }
    // resting real-order lots per (tick, side) right now, market makers removed
    void snapshot(long long nowMs, std::map<long long, std::pair<long long, long long> >& out, long long& mmOrders) const {   // tick -> (bid lots, offer lots)
        out.clear(); mmOrders = 0;
        long long bestBid = LLONG_MIN, bestAsk = LLONG_MAX;
        for (const auto& kv : book) { const Ord& o = kv.second; if (o.size <= 0) continue; if (o.side == 1) bestBid = std::max(bestBid, o.tick); else bestAsk = std::min(bestAsk, o.tick); }
        long long mid = bestBid != LLONG_MIN && bestAsk != LLONG_MAX ? (bestBid + bestAsk) / 2 : bestBid != LLONG_MIN ? bestBid : bestAsk;
        if (mid == LLONG_MIN || mid == LLONG_MAX) return;
        for (const auto& kv : book) {
            const Ord& o = kv.second; if (o.size <= 0) continue;
            if (o.tick < mid - BAND_TICKS || o.tick > mid + BAND_TICKS) continue;
            if (marketMaker(o, nowMs)) { ++mmOrders; continue; }
            auto& p = out[o.tick]; if (o.side == 1) p.first += o.size; else p.second += o.size;
        }
    }
};

// per-bar sums of the once-a-second samples (key = the bar's stamp, the same t the engine's bars carry)
struct BarSum { long long samples = 0; std::map<long long, std::pair<double, double> > lots; };   // tick -> (bid lot-samples, offer lot-samples)
class Sampler {
public:
    std::map<long long, BarSum> bars;
    long long lastMs = -1, samples = 0, mmSeen = 0;
    // one sample (at most once a second) into the forming bar t
    bool sample(const Book& b, long long barT, long long nowMs) {
        if (lastMs >= 0 && nowMs - lastMs < SAMPLE_MS && nowMs >= lastMs) return false;
        lastMs = nowMs;
        std::map<long long, std::pair<long long, long long> > snap; long long mm = 0; b.snapshot(nowMs, snap, mm);
        if (snap.empty()) return false;
        BarSum& s = bars[barT]; ++s.samples; ++samples; mmSeen = mm;
        for (const auto& kv : snap) { auto& p = s.lots[kv.first]; p.first += (double)kv.second.first; p.second += (double)kv.second.second; }
        while (bars.size() > BAR_CAP) bars.erase(bars.begin());
        return true;
    }
    // the average resting lots per row (and the bid part) over bars with stamps ts; false = no book for this wave
    bool average(const std::vector<long long>& ts, long long e0, int rowTicks, size_t nRows, std::vector<double>& rows, std::vector<double>& bids) const {
        rows.assign(nRows, 0.0); bids.assign(nRows, 0.0);
        if (ts.empty() || rowTicks < 1 || nRows == 0) return false;
        long long n = 0; size_t covered = 0;
        for (long long t : ts) { auto it = bars.find(t); if (it == bars.end() || it->second.samples <= 0) continue; ++covered; n += it->second.samples; }
        if (n <= 0 || (double)covered < COVER_MIN * (double)ts.size()) return false;
        for (long long t : ts) {
            auto it = bars.find(t); if (it == bars.end()) continue;
            for (const auto& kv : it->second.lots) {
                const long long d = kv.first - e0; if (d < 0) continue;
                const long long k = d / rowTicks; if (k >= (long long)nRows) continue;
                rows[(size_t)k] += (kv.second.first + kv.second.second) / (double)n; bids[(size_t)k] += kv.second.first / (double)n;
            }
        }
        for (double v : rows) if (v > 0) return true;
        return false;
    }
};

// the liquidity nodes of a wave's rows (wp8.pick_raw): indexes, biggest first; med = the median of the non-zero rows
inline std::vector<int> pickRaw(const std::vector<double>& v, double& med) {
    std::vector<int> out; med = 0; const int n = (int)v.size(); if (n < 1) return out;
    std::vector<double> nz; for (double x : v) if (x > 0) nz.push_back(x);
    if (nz.empty()) return out;
    std::sort(nz.begin(), nz.end()); const size_t m = nz.size(); med = m % 2 ? nz[m / 2] : (nz[m / 2 - 1] + nz[m / 2]) / 2;
    std::vector<double> s((size_t)n, 0);
    for (int j = 0; j < n; ++j) s[(size_t)j] = ((j > 0 ? v[(size_t)j - 1] : 0) + v[(size_t)j] + (j + 1 < n ? v[(size_t)j + 1] : 0)) / 3.0;
    std::vector<int> peaks;
    for (int j = 0; j < n; ++j) if (v[(size_t)j] >= v[(size_t)std::max(0, j - 1)] && v[(size_t)j] >= v[(size_t)std::min(n - 1, j + 1)] && v[(size_t)j] >= 2 * med && v[(size_t)j] > 0) peaks.push_back(j);
    std::stable_sort(peaks.begin(), peaks.end(), [&](int a, int b) { return v[(size_t)a] > v[(size_t)b]; });
    for (int j : peaks) {
        bool ok = true;
        for (int c : out) {
            if (std::abs(j - c) < 3) { ok = false; break; }
            double mn = s[(size_t)std::min(j, c)]; for (int k = std::min(j, c); k <= std::max(j, c); ++k) mn = std::min(mn, s[(size_t)k]);
            if (mn > 0.7 * std::min(s[(size_t)j], s[(size_t)c]) + 1e-12) { ok = false; break; }
        }
        if (ok) out.push_back(j);
        if ((int)out.size() == 5) break;
    }
    return out;
}

} // namespace wpl
