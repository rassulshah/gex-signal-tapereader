#pragma once
// TapeFlow wrapper support; standard C++11 only. No Investor/RT SDK assumptions.
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <type_traits>

namespace tf_support {

struct TradeIdentity {
    long long second;
    double rawTime;
    int price, bid, ask;
    long long quantity;
    bool operator==(const TradeIdentity& other) const {
        return second == other.second && rawTime == other.rawTime &&
               price == other.price && bid == other.bid && ask == other.ask &&
               quantity == other.quantity;
    }
};

// This cursor requires each overlapping query to include the WHOLE last second.
// Equal print tuples are legitimate distinct executions; their multiplicity is kept.
// It verifies the already-consumed prefix instead of guessing when history changes.
struct SnapshotCursor {
    long long lastSec = -1;
    std::vector<TradeIdentity> consumed;

    void reset() { lastSec = -1; consumed.clear(); }

    bool plan(const std::vector<TradeIdentity>& data, std::size_t* begin,
              std::string* error) const {
        if (!begin || !error) return false;
        *begin = 0;
        error->clear();
        for (std::size_t i = 0; i < data.size(); ++i) {
            if (!std::isfinite(data[i].rawTime) || data[i].quantity <= 0) {
                *error = "invalid trade record in snapshot";
                return false;
            }
            if (i && data[i].second < data[i-1].second) {
                *error = "snapshot time is not monotonic (late data / clock discontinuity)";
                return false;
            }
        }
        if (data.empty() || lastSec < 0) return true;
        std::size_t i = 0;
        while (i < data.size() && data[i].second < lastSec) ++i;
        if (i == data.size()) { *begin = data.size(); return true; }
        if (data[i].second != lastSec) {
            *error = "overlap query omitted the cursor second; cannot prove continuity";
            return false;
        }
        for (std::size_t n = 0; n < consumed.size(); ++n) {
            if (i + n >= data.size() || !(data[i+n] == consumed[n])) {
                *error = "consumed snapshot prefix changed / shrank; resynchronization required";
                return false;
            }
        }
        *begin = i + consumed.size();
        return true;
    }

    void commit(const TradeIdentity& record) {
        if (record.second != lastSec) {
            lastSec = record.second;
            consumed.clear();
        }
        consumed.push_back(record);
    }
};

// (1.1.4) a multi-day back-fill from IRT can come back not strictly in time order (the DTN history and the CQG live recording
// are stitched per day / per request). A fresh back-fill replay may safely be put in time order first; returns how many were moved.
// The LIVE cursor never sorts (it must prove continuity), only a back-fill does.
inline std::size_t sortByTime(std::vector<TradeIdentity>& v) {
    std::size_t inv = 0;
    for (std::size_t i = 1; i < v.size(); ++i) if (v[i].second < v[i-1].second || (v[i].second == v[i-1].second && v[i].rawTime < v[i-1].rawTime)) ++inv;
    if (inv) std::stable_sort(v.begin(), v.end(), [](const TradeIdentity& a, const TradeIdentity& b) {
        return a.second != b.second ? a.second < b.second : a.rawTime < b.rawTime; });
    return inv;
}

inline std::string eventKey(const std::string& line) {
    // First four pipe-separated fields: time | episode | kind | direction.
    // Include the remaining payload in caller-side validation, not in identity.
    std::size_t from = 0;
    for (int field = 0; field < 4; ++field) {
        const std::size_t at = line.find('|', from);
        if (at == std::string::npos) return line;
        if (field == 3) return line.substr(0, at);
        from = at + 1;
    }
    return line;
}

inline std::string safeField(std::string text) {
    for (char& c : text) if (c == '|' || c == '\n' || c == '\r') c = ' ';
    return text;
}

inline bool completeEventLine(const std::string& line) {
    if (line.empty() || line.back() != '\n') return false;
    if (line.size() < 20 || line[4] != '-' || line[7] != '-' || line[10] != ' ') return false;
    if (std::count(line.begin(), line.end(), '|') != 19) return false;
    return line.size() >= 15 && line.compare(line.size()-15, 15, "quote-at-trade\n") == 0;
}

inline std::uint64_t fingerprint(const std::string& bytes) {
    std::uint64_t value = UINT64_C(14695981039346656037);
    for (unsigned char c : bytes) {
        value ^= c;
        value *= UINT64_C(1099511628211);
    }
    return value;
}

inline bool toTicks(double price, double increment, int* result, double tolerance=1e-6) {
    if (!result || !(increment > 0) || !std::isfinite(increment) || !std::isfinite(price)) return false;
    const double scaled = price / increment;
    if (!std::isfinite(scaled) || scaled < static_cast<double>(std::numeric_limits<int>::min()) + 1 ||
        scaled > static_cast<double>(std::numeric_limits<int>::max()) - 1) return false;
    const long long nearest = std::llround(scaled);
    // At most one millionth of a tick of binary conversion error. No silent off-grid rounding.
    if (!std::isfinite(tolerance) || tolerance<0 || tolerance>0.05 || std::fabs(scaled - static_cast<double>(nearest)) > tolerance) return false;
    *result = static_cast<int>(nearest);
    return true;
}

template<typename Number>
inline bool nativeToTicks(Number price,double increment,int* result) {
    typedef typename std::remove_cv<typename std::remove_reference<Number>::type>::type Scalar;
    const double tolerance=std::max(1e-6,std::fabs((double)price/increment)*std::numeric_limits<Scalar>::epsilon()*2);
    return toTicks((double)price,increment,result,tolerance);
}

// Prefix sums built once per render, independent of pixels and SDK draw calls.
// They avoid one reverse history traversal per subbar and are exact for quantities.
struct RenderIndex {
    std::vector<long long> time;
    std::vector<long double> buys, sells, unknown, activity;
    std::vector<std::size_t> quality, activityCount;
    template<class Records>
    void build(const Records& records, unsigned qualityFlag) {
        const std::size_t n = records.size();
        time.resize(n);
        buys.assign(n+1, 0); sells.assign(n+1, 0); unknown.assign(n+1, 0); activity.assign(n+1, 0);
        quality.assign(n+1, 0); activityCount.assign(n+1, 0);
        for (std::size_t i=0; i<n; ++i) {
            const auto& r = records[i];
            time[i] = r.t;
            buys[i+1] = buys[i] + r.b; sells[i+1] = sells[i] + r.s; unknown[i+1] = unknown[i] + r.u;
            const bool valid = std::isfinite(r.a);
            activity[i+1] = activity[i] + (valid ? r.a : 0);
            activityCount[i+1] = activityCount[i] + (valid ? 1 : 0);
            quality[i+1] = quality[i] + ((r.flags & qualityFlag) ? 1 : 0);
        }
    }
    std::size_t end(long long t) const {
        return static_cast<std::size_t>(std::upper_bound(time.begin(), time.end(), t)-time.begin());
    }
    std::size_t start(long long t) const {
        return static_cast<std::size_t>(std::lower_bound(time.begin(), time.end(), t)-time.begin());
    }
};

} // namespace tf_support
