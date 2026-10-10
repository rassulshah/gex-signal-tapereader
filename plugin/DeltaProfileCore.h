#ifndef LS_DELTA_PROFILE_CORE_H
#define LS_DELTA_PROFILE_CORE_H

// DeltaProfileCore.h 2.5.4 -- SDK-independent C++11 calculation layer. No drawing, files, static chart state,
// wall clock, market aliases, or pixel coordinates influence these results.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace delta_profile {
typedef std::int64_t Tick;
struct Row {
    Tick key;
    double delta, volume;
    Row(Tick k = 0, double d = 0, double v = 0) : key(k), delta(d), volume(v) {}
};
struct Bar {
    int index;
    std::int64_t time; // chart-local epoch seconds; must be nondecreasing
    bool closed;      // an unverified/forming candle cannot confirm a signal
    double open, high, low, close;
    std::vector<Row> rows; // normalized, unique tick keys, ascending
    Bar() : index(-1), time(0), closed(false), open(0), high(0), low(0), close(0) {}
};
struct State {
    std::string code;
    bool support;
    int peakBar;
    int decidedBar;   // (2.5.0) the bar whose close decided it (-1 = undecided)
    State() : support(false), peakBar(-1), decidedBar(-1) {}
    bool valid() const { return !code.empty(); }
};
struct Bucket {
    Tick key, low, high;
    double delta, volume, ratio;
    Bucket() : key(0), low(0), high(0), delta(0), volume(0), ratio(0) {}
};
struct Node { Tick bucket; double price, ratio; State state; };
struct Zone { Tick low, high; std::string code; bool support; int share; int peakBar = -1;   // (2.4.3) peakBar: where the zone's absorption happened, for its circle
              double net = 0; int decidedBar = -1; };                                    // (2.5.4) for the signal record only: the zone's net delta and the bar whose close decided it
struct Snapshot {
    double tick, meanAbsDelta, maxAbsDelta, maxVolume;
    Tick ticksPerRow;
    std::vector<Bucket> buckets;
    std::vector<Node> nodes; // at most three, descending magnitude, deterministic ties
    std::vector<Zone> zones;
    Snapshot() : tick(0), meanAbsDelta(0), maxAbsDelta(0), maxVolume(0), ticksPerRow(1) {}
};
inline bool finite(double x) { return std::isfinite(x); }
inline Tick priceKey(double price, double tick) {
    if (!finite(price) || !finite(tick) || tick <= 0)
        throw std::invalid_argument("invalid price/tick");
    const double q = price / tick;
    // Conservative bound also leaves headroom for group-edge arithmetic.
    if (!finite(q) || std::fabs(q) >= 9.0e15)
        throw std::overflow_error("price/tick outside supported integer range");
    return static_cast<Tick>(std::llround(q));
}
inline Tick floorDiv(Tick a, Tick b) {
    if (b <= 0) throw std::invalid_argument("nonpositive group size");
    Tick q = a / b, r = a % b;
    return q - (r < 0 ? 1 : 0);
}
inline void normalize(Bar& bar) {
    std::map<Tick, Row> merged;
    for (std::size_t i = 0; i < bar.rows.size(); ++i) {
        const Row& r = bar.rows[i];
        if (!finite(r.delta) || !finite(r.volume) || r.volume < 0 ||
            std::fabs(r.delta) > r.volume + 1e-6 * std::max(1.0, r.volume))
            throw std::invalid_argument("invalid volume-at-price row");
        Row& m = merged[r.key]; m.key = r.key;
        m.delta += r.delta; m.volume += r.volume;
        if (!finite(m.delta) || !finite(m.volume))
            throw std::overflow_error("volume aggregation overflow");
    }
    bar.rows.clear(); bar.rows.reserve(merged.size());
    for (std::map<Tick, Row>::const_iterator i = merged.begin(); i != merged.end(); ++i)
        bar.rows.push_back(i->second);
}
inline double regionalDelta(const Bar& b, Tick low, Tick high) {
    std::vector<Row>::const_iterator i = std::lower_bound(
        b.rows.begin(), b.rows.end(), low,
        [](const Row& r, Tick k) { return r.key < k; });
    double d = 0;
    for (; i != b.rows.end() && i->key <= high; ++i) d += i->delta;
    return d;
}
inline State classify(const std::vector<Bar>& bars, Tick low, Tick high,
                      double net, double tick) {
    State out;
    if (bars.empty() || net == 0 || !finite(net) || !finite(tick) || tick <= 0 || low > high)
        return out;
    const bool sell = net < 0;
    int formed = -1, peak = -1;
    double peakMagnitude = -1;
    for (std::size_t i = 0; i < bars.size(); ++i) {
        const double d = regionalDelta(bars[i], low, high);
        if (d != 0 && (d < 0) == sell && std::fabs(d) >= 0.1 * std::fabs(net)) {
            formed = static_cast<int>(i);
            if (std::fabs(d) > peakMagnitude) { peakMagnitude = std::fabs(d); peak = formed; }
        }
    }
    if (formed < 0 || peak < 0) return out;
    out.peakBar = bars[static_cast<std::size_t>(peak)].index;
    const double lo = static_cast<double>(low) * tick, hi = static_cast<double>(high) * tick;
    const double eps = tick * 1e-6;
    // (2.5.0, Rassul 2026-10-09 13:48 "lets simplify the rules to a close below the bearish accumulation and vice versa for
    // bullish") the FIRST closed bar after the node formed that closes beyond it decides, and the decision is final:
    //   a RED bar closing below the node -> price went down from it;  a GREEN bar closing above -> price went up from it.
    // A heavy-BUYING node that sees a close below = A (buyers absorbed, bearish); a close above = I (buyers won).
    // A heavy-SELLING node: close above = A (sellers absorbed, bullish); close below = I (sellers won).
    // A forming (unclosed) bar never decides.
    int winner = 0; // +1 upward, -1 downward, 0 undecided
    for (std::size_t i = static_cast<std::size_t>(formed + 1); i < bars.size(); ++i) {
        const Bar& b = bars[i];
        if (!b.closed) continue;
        // (2.5.0, Rassul 13:55 "a red bar close below for bearish absorption and a green bar above for a bullish absorption")
        // the deciding bar must close beyond the node in its own colour: red (close < open) below, green (close > open) above
        if (b.close < lo - eps && b.close < b.open) { winner = -1; out.decidedBar = b.index; break; }
        if (b.close > hi + eps && b.close > b.open) { winner = 1; out.decidedBar = b.index; break; }
    }
    if (winner != 0) {
        out.code = ((winner == 1) == sell) ? "A" : "I";
        out.support = winner == 1;
    } else {
        out.code = "A?"; out.support = sell;
        const Bar& last = bars.back();
        const bool initiative = sell ? last.low < lo - 0.5 * tick - eps : last.high > hi + 0.5 * tick + eps;
        if (static_cast<std::size_t>(formed + 1) < bars.size() && initiative) {
            out.code = "I?"; out.support = !sell;
        }
    }
    return out;
}
inline Snapshot build(const std::vector<Bar>& bars, double tick, bool allowConfirmation = true) {
    if (!finite(tick) || tick <= 0) throw std::invalid_argument("native tick size unavailable");
    Snapshot s; s.tick = tick;
    if (bars.empty()) return s;
    std::map<Tick, Row> profile;
    for (std::size_t i = 0; i < bars.size(); ++i) {
        const Bar& b = bars[i];
        if ((i && b.time < bars[i - 1].time) || !finite(b.open) || !finite(b.high) ||
            !finite(b.low) || !finite(b.close) || b.high < b.low ||
            b.open < b.low || b.open > b.high || b.close < b.low || b.close > b.high)
            throw std::invalid_argument("invalid/nonchronological OHLC bars");
        for (std::size_t j = 0; j < b.rows.size(); ++j) {
            const Row& r = b.rows[j];
            if ((j && r.key <= b.rows[j - 1].key) || !finite(r.delta) || !finite(r.volume) || r.volume < 0 ||
                std::fabs(r.delta) > r.volume + 1e-6 * std::max(1.0, r.volume))
                throw std::invalid_argument("rows must be valid and normalized");
            Row& p = profile[r.key]; p.key = r.key; p.delta += r.delta; p.volume += r.volume;
            if (!finite(p.delta) || !finite(p.volume)) throw std::overflow_error("profile overflow");
        }
    }
    if (profile.empty()) return s;
    const Tick span = profile.rbegin()->first - profile.begin()->first;
    s.ticksPerRow = std::max<Tick>(1, span / 60 + (span % 60 != 0));
    std::map<Tick, Bucket> grouped;
    for (std::map<Tick, Row>::const_iterator i = profile.begin(); i != profile.end(); ++i) {
        const Tick key = floorDiv(i->first, s.ticksPerRow);
        Bucket& b = grouped[key];
        if (b.volume == 0) { b.low = i->first; b.high = i->first; }
        b.key = key; b.low = std::min(b.low, i->first); b.high = std::max(b.high, i->first);
        b.delta += i->second.delta; b.volume += i->second.volume;
    }
    std::size_t nonzero = 0;
    for (std::map<Tick, Bucket>::const_iterator i = grouped.begin(); i != grouped.end(); ++i) {
        s.buckets.push_back(i->second);
        const double a = std::fabs(i->second.delta);
        if (a > 0) { s.meanAbsDelta += a; ++nonzero; }
        s.maxAbsDelta = std::max(s.maxAbsDelta, a); s.maxVolume = std::max(s.maxVolume, i->second.volume);
    }
    s.meanAbsDelta = nonzero ? s.meanAbsDelta / static_cast<double>(nonzero) : 0;
    std::vector<std::size_t> ranked;
    for (std::size_t i = 0; i < s.buckets.size(); ++i) {
        s.buckets[i].ratio = s.meanAbsDelta > 0 ? std::fabs(s.buckets[i].delta) / s.meanAbsDelta : 0;
        ranked.push_back(i);
    }
    std::sort(ranked.begin(), ranked.end(), [&s](std::size_t a, std::size_t b) {
        const double x = std::fabs(s.buckets[a].delta), y = std::fabs(s.buckets[b].delta);
        return x != y ? x > y : s.buckets[a].key < s.buckets[b].key;
    });
    for (std::size_t i = 0; i < ranked.size() && i < 10 && s.nodes.size() < 3; ++i) {
        const Bucket& b = s.buckets[ranked[i]];
        if (b.ratio < 2.0) break;
        State state = classify(bars, b.low, b.high, b.delta, tick);
        if (!state.valid()) continue;
        if (!allowConfirmation && (state.code == "A" || state.code == "I")) state.code += "?";
        Node n; n.bucket = b.key; n.price = (static_cast<double>(b.low) + static_cast<double>(b.high)) * 0.5 * tick;
        n.ratio = b.ratio; n.state = state; s.nodes.push_back(n);
    }
    // Zones use exactly the SAME 60-minute subset for both net delta and state.
    std::vector<Bar> zoneBars;
    std::map<Tick, double> zdelta;
    const std::int64_t cutoff = bars.back().time - 3600;
    for (std::size_t i = 0; i < bars.size(); ++i) if (bars[i].time > cutoff) {
        zoneBars.push_back(bars[i]);
        for (std::size_t j = 0; j < bars[i].rows.size(); ++j)
            zdelta[bars[i].rows[j].key] += bars[i].rows[j].delta;
    }
    if (zdelta.size() < 5) return s;
    double largest = 0, sellTotal = 0, buyTotal = 0;
    for (std::map<Tick, double>::const_iterator i = zdelta.begin(); i != zdelta.end(); ++i) {
        largest = std::max(largest, std::fabs(i->second));
        if (i->second < 0) sellTotal += -i->second; else buyTotal += i->second;
    }
    if (largest == 0) return s;
    const Tick bottom = zdelta.begin()->first, top = zdelta.rbegin()->first;
    const double range = std::max(1.0, static_cast<double>(top - bottom));
    std::vector<std::pair<Tick, double> > run;
    const auto finish = [&]() {
        if (run.size() >= 2) {
            const bool sell = run.front().second < 0;
            double net = 0; for (std::size_t i = 0; i < run.size(); ++i) net += run[i].second;
            const double total = sell ? sellTotal : buyTotal;
            const double share = total > 0 ? std::fabs(net) / total : 0;
            const bool edge = sell ? static_cast<double>(run.back().first) >= static_cast<double>(top) - 0.25 * range :
                static_cast<double>(run.front().first) <= static_cast<double>(bottom) + 0.25 * range;
            if (share >= 0.4 && edge) {
                const State state = classify(zoneBars, run.front().first, run.back().first, net, tick);
                const std::string name = sell ? "Dst" : "Acc";
                Zone z; z.low = run.front().first; z.high = run.back().first;
                z.share = static_cast<int>(std::floor(share * 100 + 0.5));
                z.peakBar = state.peakBar;
                z.net = net; z.decidedBar = state.decidedBar;   // (2.5.4) recorded, never drawn
                z.code = state.code == "A" ? "A" : state.code == "I" ? name : name + "?";
                z.support = (state.code == "A" || state.code == "I") ? state.support : !sell;
                if (!allowConfirmation && z.code.back() != '?') z.code += "?";
                s.zones.push_back(z);
            }
        }
        run.clear();
    };
    for (std::map<Tick, double>::const_iterator i = zdelta.begin(); i != zdelta.end(); ++i) {
        const bool heavy = i->second != 0 && std::fabs(i->second) >= 0.25 * largest;
        if (!heavy || (!run.empty() && ((i->second < 0) != (run.back().second < 0) || i->first - run.back().first > 2)))
            finish();
        if (heavy) run.push_back(*i);
    }
    finish();
    std::sort(s.zones.begin(), s.zones.end(), [](const Zone& a, const Zone& b) {
        return a.share != b.share ? a.share > b.share : a.low < b.low;
    });
    if (s.zones.size() > 2) s.zones.resize(2);
    return s;
}
} // namespace delta_profile
#endif
