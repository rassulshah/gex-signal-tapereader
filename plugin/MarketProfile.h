#ifndef LS_MARKETPROFILE_H
#define LS_MARKETPROFILE_H
// MarketProfile.h 0.1.0 (2026-10-26) -- the PERSONALITY of one market (awareness_plus item 10), SDK-free (unit-tested on Linux).
// Rassul 2026-10-26 13:46: "es is a thicker market and might be totally different than euro" -> every threshold, expectation K/N,
// news window and expected source is PER MARKET, read from one file the nightly writes (partial pooling done there, never here):
//
//   lsFlexLevels\LRA-MarketProfile-<MKT>.csv     (missing / bad file -> the built-in defaults below, note says so; never a crash)
//     VERSION,1
//     MARKET,<MKT>
//     P,<key>,<value>                       personality numbers (tick, aggr_p90, aggr_p99, shown_med, refill_share, implied_share,
//                                           lon_vs_rth_vol, atr3_med, adr_med, shock_rng_x, shock_vol_x, news_conf_x, ...)
//     SLOT,<min of day>,<rng_med ticks>,<rng_p95 ticks>,<vol_med>,<vol_p95>     30-min slot normals of a 3-min bar (shock detector)
//     KN,<read>,<ctx>,<K bars>,<N atr>      expectation deadline + distance per read x context (ctx 'all' = the fallback)
//     NEWS,<yyyy-mm-dd | wd0..wd6>,<HHMM CT>,<event>,<minutes before>,<minutes after>   scheduled windows (dated rows, or weekday rules)
//     SRC,<source>,<expected 0|1>,<fresh s>  which board sources this market is expected to have (and how fresh they must be)
// Unknown numbers stay unknown: get() returns the default you pass, has() says whether the market file had it.
#include <map>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>

namespace apm {

static const char* const VERSION = "0.1.0";

struct Slot { int min = 0; double rngMed = -1, rngP95 = -1, volMed = -1, volP95 = -1; };
struct KN { int K = 0; double N = 0; };
struct NewsRule { std::string when; int hhmm = 0; std::string event; int before = 5, after = 15; };   // when: "2026-10-28" or "wd3"
struct SrcRule { bool expected = false; int freshS = 60; };

struct Profile {
    std::string market, note = "defaults (no market profile file)";
    bool loaded = false;
    std::map<std::string, double> p;
    std::vector<Slot> slots;                       // sorted by min
    std::map<std::string, KN> kn;                  // "read|ctx"
    std::vector<NewsRule> news;
    std::map<std::string, SrcRule> src;

    bool has(const std::string& k) const { return p.count(k) > 0; }
    double get(const std::string& k, double def) const { auto it = p.find(k); return it == p.end() ? def : it->second; }
    // the slot normal for a minute of day (the 30-min slot that contains it); nullptr = unknown (no shock call without a normal)
    const Slot* slotFor(int minOfDay) const {
        const Slot* best = nullptr;
        for (const auto& s : slots) if (s.min <= minOfDay && minOfDay < s.min + 30) best = &s;
        return best;
    }
};

inline std::vector<std::string> splitCsv(const std::string& line) {
    std::vector<std::string> f; std::string cur;
    for (char c : line) { if (c == ',') { f.push_back(cur); cur.clear(); } else if (c != '\r' && c != '\n') cur += c; }
    f.push_back(cur);
    for (auto& s : f) { size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t"); s = a == std::string::npos ? std::string() : s.substr(a, b - a + 1); }
    return f;
}
inline bool toD(const std::string& s, double& x) { if (s.empty() || s == "?") return false; char* e = nullptr; x = std::strtod(s.c_str(), &e); return e && *e == 0; }
inline bool toI(const std::string& s, int& x) { double d; if (!toD(s, d)) return false; x = (int)d; return (double)x == d; }

// built-in defaults: conservative, the same for every market (the file is what makes a market itself)
inline Profile defaults(const std::string& market) {
    Profile P; P.market = market;
    P.p["shock_rng_x"] = 3.0; P.p["shock_vol_x"] = 2.5; P.p["news_conf_x"] = 0.5;
    const char* kinds[] = { "turn", "collision", "trap", "d2_turn", "confirm", "absorb", "stage3", "held", nullptr };
    const int Ks[] = { 3, 5, 5, 5, 3, 5, 8, 3 }; const double Ns[] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0, 0 };
    for (int i = 0; kinds[i]; ++i) P.kn[std::string(kinds[i]) + "|all"] = KN{ Ks[i], Ns[i] };
    // weekday rules every market file normally repeats (wd: 0 Sun .. 6 Sat): EIA crude Wed 09:30 (CL), EIA gas Thu 09:30 (NG)
    if (market == "CL") P.news.push_back(NewsRule{ "wd3", 930, "EIA_CRUDE", 5, 30 });
    if (market == "NG") P.news.push_back(NewsRule{ "wd4", 930, "EIA_GAS", 5, 30 });
    return P;
}

inline Profile parse(const std::string& text, const std::string& market) {
    Profile P = defaults(market), F; F.market = market;
    bool v1 = false, mk = false; int lines = 0, bad = 0;
    size_t a = 0;
    while (a <= text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        const std::string line = text.substr(a, e - a); a = e + 1;
        if (line.empty() || line[0] == '#') { if (e == text.size()) break; continue; }
        const auto f = splitCsv(line); ++lines;
        if (f[0] == "VERSION") v1 = f.size() > 1 && f[1] == "1";
        else if (f[0] == "MARKET") mk = f.size() > 1 && f[1] == market;
        else if (f[0] == "P" && f.size() >= 3) { double x; if (toD(f[2], x)) F.p[f[1]] = x; else ++bad; }
        else if (f[0] == "SLOT" && f.size() >= 6) { Slot s; if (toI(f[1], s.min) && toD(f[2], s.rngMed) && toD(f[3], s.rngP95) && toD(f[4], s.volMed) && toD(f[5], s.volP95)) F.slots.push_back(s); else ++bad; }
        else if (f[0] == "KN" && f.size() >= 5) { KN k; if (toI(f[3], k.K) && toD(f[4], k.N) && k.K > 0 && k.K <= 60) F.kn[f[1] + "|" + f[2]] = k; else ++bad; }
        else if (f[0] == "NEWS" && f.size() >= 6) { NewsRule r; r.when = f[1]; r.event = f[3]; if (toI(f[2], r.hhmm) && toI(f[4], r.before) && toI(f[5], r.after)) F.news.push_back(r); else ++bad; }
        else if (f[0] == "SRC" && f.size() >= 4) { SrcRule r; int x = 0, fr = 0; if (toI(f[2], x) && toI(f[3], fr)) { r.expected = x == 1; r.freshS = fr > 0 ? fr : 60; F.src[f[1]] = r; } else ++bad; }
        else ++bad;
        if (e == text.size()) break;
    }
    if (!v1 || !mk) { P.note = !v1 ? "defaults (profile file has no VERSION,1)" : "defaults (profile file is for another market)"; return P; }
    // the file wins key by key; anything it lacks keeps the default
    for (const auto& kv : F.p) P.p[kv.first] = kv.second;
    for (const auto& kv : F.kn) P.kn[kv.first] = kv.second;
    if (!F.news.empty()) P.news = F.news;
    P.slots = F.slots; P.src = F.src; P.loaded = true;
    char b[96]; std::snprintf(b, sizeof(b), "file (%d lines, %d bad, %d slots, %d K/N, %d news)", lines, bad, (int)P.slots.size(), (int)F.kn.size(), (int)F.news.size());
    P.note = b;
    return P;
}

// K/N for a read in a context: read|ctx -> read|all -> the generic default
inline KN knFor(const Profile& P, const std::string& read, const std::string& ctx) {
    auto it = P.kn.find(read + "|" + ctx); if (it != P.kn.end()) return it->second;
    it = P.kn.find(read + "|all"); if (it != P.kn.end()) return it->second;
    return KN{ 5, 1.0 };
}

}  // namespace apm
#endif
