// BigLotTiers.h  1.0.0  (2026-10-18, lsBidAskStats 1.14.0 - Rassul 17:26 CT "implement Big lot 5 style on the aggression indicator. I want
// to see if it will help identify spikes near the hod and lod")  -  SDK-free, unit-tested on Linux (tests_aggbook/test_aggbook_114.cpp).
//
// The Big Lot sizes BL5 / BL10 / BL20 for ONE print quantity (the most lots at one price, one side, one candle - the Aggression bar, the
// same quantity as a lsBigBidAsk print), the lsBigBidAsk 1.16 box rule copied here (BigBidAskLogic.h nth / avgDropOutlier / typeNeed /
// the day rule - COPIED, not included, so the two plugins never break each other's build):
//   - per session TYPE: RTH ("day" = after the RTH open to an hour after the RTH close, svl::rthOf per market) and overnight (the rest);
//   - a session's BLn = its n-th biggest print of that type (both sides of every closed candle of that type);
//   - the tier of a session = the average of BLn over the latest 5 COMPLETE earlier sessions of that type (3/4 of its candles), one
//     outlier session dropped (> 1.5x or < 2/3 of the median of the others); fewer than 3 complete sessions = "learning" (no mark);
//   - decided ONCE per session and type (Book::tierAt keeps it; ok tiers are written to the history file and read back after a restart),
//     never from the session itself or a later one - so a mark can never appear / disappear later (no repaint).
#pragma once
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace blt {

static const char* const VERSION = "1.0.0";
static const int NS[3] = { 5, 10, 20 };                 // BL5 / BL10 / BL20
static const int MAX_SESS = 5, MIN_SESS = 3, KEEP_SESS = 40;
static const char* const CHOICES = "Off;BL5;BL10;BL20";  // the "Big Lot mark" setting: 0 Off, 1 BL5 (default), 2 BL10, 3 BL20
static const int CHOICE_DEFAULT = 1;
enum { NIGHT = 0, DAY = 1 };

inline long long nth(std::vector<long long> v, int n) {
    if (n < 1 || (int)v.size() < n) return 0;
    std::nth_element(v.begin(), v.begin() + (n - 1), v.end(), std::greater<long long>()); return v[(size_t)n - 1];
}
struct Avg { long long avg = 0; int used = 0; int dropped = -1; double ratio = 0; };
inline double medianOf(std::vector<double> v) { if (v.empty()) return 0; std::sort(v.begin(), v.end()); const size_t m = v.size() / 2; return v.size() % 2 ? v[m] : (v[m - 1] + v[m]) / 2; }
inline Avg avgDropOutlier(const std::vector<long long>& x) {
    Avg a; std::vector<int> ok; for (int i = 0; i < (int)x.size(); ++i) if (x[(size_t)i] > 0) ok.push_back(i);
    if (ok.empty()) return a;
    int drop = -1; double worst = 1.0;
    if (ok.size() >= 4) for (int i : ok) {
        std::vector<double> others; for (int j : ok) if (j != i) others.push_back((double)x[(size_t)j]);
        const double m = medianOf(others); if (m <= 0) continue;
        const double r = (double)x[(size_t)i] / m, dev = r >= 1 ? r : 1.0 / r;
        if ((r > 1.5 || r < 2.0 / 3.0) && dev > worst) { worst = dev; drop = i; a.ratio = r; }
    }
    double s = 0; int n = 0; for (int i : ok) if (i != drop) { s += (double)x[(size_t)i]; ++n; }
    a.avg = n ? (long long)((s / n) + 0.5) : 0; a.used = n; a.dropped = drop; return a;
}
// day = after the RTH open to an hour after the RTH close (minute of the bar's CLOSE stamp), as lsBigBidAsk dayOf
inline int typeOf(int closeMinute, int openMin, int closeMin) { return closeMinute > openMin && closeMinute <= closeMin + 60 ? DAY : NIGHT; }
// candles of a complete session of one type: 3/4 of them (lsBigBidAsk typeNeed)
inline int typeNeed(int type, int openMin, int closeMin, int per) {
    if (per <= 0) per = 180;
    const int dayMin = std::max(60, closeMin + 60 - openMin), nightMin = std::max(60, 1380 - dayMin);
    return std::max(20, (int)((long long)(type == NIGHT ? nightMin : dayMin) * 60 / per * 3 / 4));
}

// one session of one type: its candle count and its 5th / 10th / 20th biggest print
struct Sess { int candles = 0; long long n[3] = { 0, 0, 0 }; };
inline Sess sessOf(const std::vector<long long>& prints, int candles) { Sess s; s.candles = candles; for (int q = 0; q < 3; ++q) s.n[q] = nth(prints, NS[q]); return s; }

struct Tier { bool ok = false; int nSess = 0; long long bl[3] = { 0, 0, 0 }; std::string why = "learning"; };
// the tier of session `sess`: complete sessions BEFORE it only (never itself, never a later one)
inline Tier tierFor(long sess, const std::map<long, Sess>& hist, int need) {
    Tier T; std::vector<long> used;
    for (std::map<long, Sess>::const_reverse_iterator it = hist.rbegin(); it != hist.rend() && (int)used.size() < MAX_SESS; ++it)
        if (it->first < sess && it->second.candles >= need) used.push_back(it->first);
    T.nSess = (int)used.size();
    for (int q = 0; q < 3; ++q) { std::vector<long long> xs; for (long s : used) xs.push_back(hist.at(s).n[q]); T.bl[q] = avgDropOutlier(xs).avg; }
    T.ok = T.nSess >= MIN_SESS && T.bl[0] > 0;
    T.why = T.ok ? std::to_string(T.nSess) + " complete sessions" : "learning (" + std::to_string(T.nSess) + " of " + std::to_string(MIN_SESS) + " complete sessions)";
    return T;
}
// the size the setting marks (0 = none: Off, learning or unknown)
inline long long markSize(const Tier& T, int choice) { return (choice >= 1 && choice <= 3 && T.ok) ? T.bl[choice - 1] : 0; }
inline bool hit(long long lots, const Tier& T, int choice) { const long long z = markSize(T, choice); return z > 0 && lots >= z; }
// the highest tier a print reached ("BL5" > "BL10" > "BL20"; "" = none)
inline std::string reached(long long lots, const Tier& T) {
    if (!T.ok) return std::string();
    for (int q = 0; q < 3; ++q) if (T.bl[q] > 0 && lots >= T.bl[q]) return "BL" + std::to_string(NS[q]);
    return std::string();
}
inline std::string label(int choice) { return choice >= 1 && choice <= 3 ? "BL" + std::to_string(NS[choice - 1]) : std::string(); }

// strict whole-number token (BUILD-LESSONS 4)
inline bool num(const std::string& s, long long lo, long long hi, long long& out) {
    if (s.empty() || s.size() > 19) return false;
    char* e = nullptr; errno = 0; const long long v = std::strtoll(s.c_str(), &e, 10);
    if (errno || !e || *e || v < lo || v > hi) return false;
    out = v; return true;
}
inline std::vector<std::string> fields(const std::string& s) {
    std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t c = s.find('|', a); f.push_back(s.substr(a, c == std::string::npos ? std::string::npos : c - a)); if (c == std::string::npos) break; a = c + 1; }
    return f;
}

// The history of one market and bar size: complete sessions (S lines) and the tiers decided (T lines), per type.
//   S|sess|type|candles|n5|n10|n20      T|sess|type|nSess|bl5|bl10|bl20
struct Book {
    std::map<long, Sess> hist[2];
    std::map<long, Tier> tiers[2];
    int rejected = 0;
    // a session seen on the chart: kept when it has at least as many candles as the one known (a partly loaded session never overwrites)
    bool put(long sess, int type, const Sess& s) {
        if (type != DAY && type != NIGHT) return false;
        std::map<long, Sess>::iterator it = hist[type].find(sess);
        if (it != hist[type].end() && (it->second.candles > s.candles || (it->second.candles == s.candles && std::equal(s.n, s.n + 3, it->second.n)))) return false;
        hist[type][sess] = s; return true;
    }
    // decided once: the stored tier, else (ready = the history before it is fully read) decide now and keep it
    const Tier* tierAt(long sess, int type, int need, bool ready) {
        if (type != DAY && type != NIGHT) return nullptr;
        std::map<long, Tier>::const_iterator it = tiers[type].find(sess);
        if (it != tiers[type].end()) return &it->second;
        if (!ready) return nullptr;
        Tier& T = tiers[type][sess]; T = tierFor(sess, hist[type], need); return &T;
    }
    std::string text() const {
        std::string o = std::string("# BigLotTiers ") + VERSION + "\n";
        for (int t = 0; t < 2; ++t) {
            int skip = (int)hist[t].size() - KEEP_SESS;
            for (const auto& kv : hist[t]) { if (skip-- > 0) continue;
                o += "S|" + std::to_string(kv.first) + "|" + std::to_string(t) + "|" + std::to_string(kv.second.candles);
                for (int q = 0; q < 3; ++q) { o += "|" + std::to_string(kv.second.n[q]); }
                o += "\n"; }
            skip = (int)tiers[t].size() - KEEP_SESS;
            for (const auto& kv : tiers[t]) { if (skip-- > 0) continue; if (!kv.second.ok) continue;   // learning is never kept (decided again after a restart)
                o += "T|" + std::to_string(kv.first) + "|" + std::to_string(t) + "|" + std::to_string(kv.second.nSess);
                for (int q = 0; q < 3; ++q) { o += "|" + std::to_string(kv.second.bl[q]); }
                o += "\n"; }
        }
        return o;
    }
    void load(const std::string& text) {
        size_t p = 0;
        while (p < text.size()) {
            size_t e = text.find('\n', p); if (e == std::string::npos) e = text.size();
            std::string ln = text.substr(p, e - p); p = e + 1;
            if (!ln.empty() && ln.back() == '\r') ln.pop_back();
            if (ln.empty() || ln[0] == '#') continue;
            const std::vector<std::string> f = fields(ln);
            long long s = 0, t = 0, a = 0, v[3] = { 0, 0, 0 };
            bool ok = f.size() == 7 && (f[0] == "S" || f[0] == "T") && num(f[1], 1, 10000000, s) && num(f[2], 0, 1, t) && num(f[3], 0, 100000, a);
            for (int q = 0; ok && q < 3; ++q) ok = num(f[(size_t)4 + q], 0, 100000000, v[q]);
            if (!ok) { ++rejected; continue; }
            if (f[0] == "S") { Sess x; x.candles = (int)a; for (int q = 0; q < 3; ++q) x.n[q] = v[q]; put((long)s, (int)t, x); }
            else { if (a < MIN_SESS || v[0] <= 0) { ++rejected; continue; }
                   Tier T; T.ok = true; T.nSess = (int)a; for (int q = 0; q < 3; ++q) T.bl[q] = v[q]; T.why = std::to_string(a) + " complete sessions (decided earlier)";
                   tiers[t].insert(std::make_pair((long)s, T)); }   // the first decision stays
        }
    }
};

// One closed candle's Big Lot read, decided once at its close: its session / type, the bar's lots (the bigger side = the drawn bar),
// the tier in force (frozen copy), so a later settings change (BL5 -> BL10) re-reads frozen numbers and nothing is ever recomputed.
struct Mark { bool decided = false; long sess = 0; int type = NIGHT; int side = 0; long long lots = 0; Tier tier; };
inline bool shows(const Mark& m, int choice) { return m.decided && hit(m.lots, m.tier, choice); }

} // namespace blt
