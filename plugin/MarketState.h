#ifndef LS_MARKETSTATE_H
#define LS_MARKETSTATE_H
// MarketState.h 1.0.0 (2026-10-18) -- the per-chart MARKET STATE every indicator posts its facts to, so the indicators are aware of
// each other and the read (Order Flow / Liquidity Read) judges with the whole picture. SDK-free (unit-tested on Linux).
// Rassul 2026-10-17 23:18: "the indicators must first be working together and be aware of each other ... its more about building
// awareness so each indicator is eventually integrated into a system and the liquidity reader can better judge the situation."
// Design (mockup market-state-mockup.png): Indicators -> Market State (facts) -> zone story -> odds -> one-line read + co-pilot.
//
// One entry per SOURCE per chart in the process environment (as BannerBoard / SignalBoard - shared by every plugin DLL in the IRT
// process, no files):   LRA_MS_<SRC>_<symbol>_<spb>_<chart> = "<ms>|<bar>|k=v;k=v;..."
//   ms   = the publisher's steady clock when written (an entry older than FRESH_MS is ignored: the indicator was removed / stopped)
//   bar  = the chart bar the facts belong to (Central wall-clock seconds of its stamp; every plugin converts RTDATE the same way)
//   k=v  = the facts of the last CLOSED bar (decided once, never changed for that bar) plus slow-moving state (e.g. tiers, regime).
// A snapshot of all sources is written to lsFlexLevels\MarketState\<MKT>-<spb>.json every 2 s by ONE writer per chart (the read
// plugin) for Claude's live tools (co-pilot: "is NQ confirming?"); IRT itself never needs the file.
// Values never contain ';' '=' '|' or newlines (cleaned). Numbers are written plainly (ratios as 2.4, lots as integers).
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include "BannerBoard.h"

namespace mst {

static const char* const VERSION = "1.0.0";
static const long long FRESH_MS = 60000;
static const size_t MAX_ENTRY = 6000;          // one source's encoded facts (env values up to 32,767)

// the sources (short, fixed names: the co-pilot and the read look them up by these)
static const char* const SOURCES[] = { "AGG", "BOOK", "LOTS", "DP", "LIQ", "GEX", "HODLOD", "COB", "VWAP", "TREND", "READ", nullptr };

struct Facts { bool ok = false; std::string src; long long ms = 0, bar = 0; std::map<std::string, std::string> kv; };

inline std::string clean(const std::string& s, size_t cap = 160) {
    std::string o; for (char c : s) { if (o.size() >= cap) break; o += (c == ';' || c == '=' || c == '|' || c == '\n' || c == '\r') ? ' ' : c; } return o;
}
inline std::string num(double x, int prec = 4) { char b[48]; std::snprintf(b, sizeof(b), "%.*g", prec, x); return b; }
inline std::string encode(long long ms, long long bar, const std::map<std::string, std::string>& kv) {
    std::string o = std::to_string(ms) + "|" + std::to_string(bar) + "|";
    for (const auto& p : kv) {
        const std::string item = clean(p.first, 24) + "=" + clean(p.second) + ";";
        if (o.size() + item.size() > MAX_ENTRY) break;
        o += item;
    }
    return o;
}
inline bool decode(const std::string& s, long long nowMs, Facts& F) {
    F = Facts(); if (s.empty() || s.size() > MAX_ENTRY + 64) return false;
    const std::vector<std::string> f = bb::split(s, '|', 3); if (f.size() != 3) return false;
    long long ms = 0, bar = 0; if (!bb::num(f[0], ms) || !bb::num(f[1], bar) || ms < 0) return false;
    if (nowMs >= 0 && (nowMs < ms || nowMs - ms > FRESH_MS)) return false;
    size_t a = 0; const std::string& b = f[2];
    while (a < b.size()) {
        size_t e = b.find(';', a); if (e == std::string::npos) e = b.size();
        const std::string it = b.substr(a, e - a); a = e + 1;
        const size_t q = it.find('='); if (q == std::string::npos || q == 0) continue;
        F.kv[it.substr(0, q)] = it.substr(q + 1);
    }
    F.ms = ms; F.bar = bar; F.ok = true; return true;
}
inline std::string keyOf(const std::string& src, const std::string& sym, long spb, const std::string& chart) {
    return bb::key(("LRA_MS_" + src + "_").c_str(), sym, spb, chart);
}
inline void publish(const std::string& src, const std::string& sym, long spb, const std::string& chart, long long ms, long long bar,
                    const std::map<std::string, std::string>& kv) {
    if (!sym.empty()) bb::bbSet(keyOf(src, sym, spb, chart), encode(ms, bar, kv));
}
inline void clear(const std::string& src, const std::string& sym, long spb, const std::string& chart) { if (!sym.empty()) bb::bbSet(keyOf(src, sym, spb, chart), std::string()); }
inline Facts read(const std::string& src, const std::string& sym, long spb, const std::string& chart, long long nowMs) {
    Facts F; std::string v;
    if (!sym.empty() && bb::bbGet(keyOf(src, sym, spb, chart), v) && decode(v, nowMs, F)) F.src = src;
    return F;
}
inline std::map<std::string, Facts> readAll(const std::string& sym, long spb, const std::string& chart, long long nowMs) {
    std::map<std::string, Facts> out;
    for (const char* const* s = SOURCES; *s; ++s) { Facts F = read(*s, sym, spb, chart, nowMs); if (F.ok) out[*s] = F; }
    return out;
}
// typed getters (missing / bad -> the default)
inline double getD(const Facts& F, const std::string& k, double def = 0) {
    auto it = F.kv.find(k); if (it == F.kv.end() || it->second.empty()) return def;
    char* e = nullptr; const double x = std::strtod(it->second.c_str(), &e); return (e && *e == 0 && x == x) ? x : def;
}
inline std::string getS(const Facts& F, const std::string& k, const std::string& def = std::string()) { auto it = F.kv.find(k); return it == F.kv.end() ? def : it->second; }

// ---- the snapshot for Claude (JSON, no library): {"version":..,"market":..,"spb":..,"written_ct":..,"sources":{"AGG":{"age_ms":..,"bar":..,"facts":{..}}}}
inline std::string jsonEsc(const std::string& s) {
    std::string o; for (unsigned char c : s) { if (c == '"' || c == '\\') { o += '\\'; o += (char)c; } else if (c < 0x20) o += ' '; else o += (char)c; } return o;
}
inline std::string snapshotJson(const std::string& market, const std::string& sym, long spb, const std::string& writtenCt, long long nowMs,
                                const std::map<std::string, Facts>& all) {
    std::string o = "{\"version\":\"" + std::string(VERSION) + "\",\"market\":\"" + jsonEsc(market) + "\",\"symbol\":\"" + jsonEsc(sym) + "\",\"spb\":" + std::to_string(spb)
                  + ",\"written_ct\":\"" + jsonEsc(writtenCt) + "\",\"sources\":{";
    bool first = true;
    for (const auto& p : all) {
        if (!first) o += ",";
        first = false;
        o += "\"" + jsonEsc(p.first) + "\":{\"age_ms\":" + std::to_string(nowMs >= p.second.ms ? nowMs - p.second.ms : 0) + ",\"bar\":" + std::to_string(p.second.bar) + ",\"facts\":{";
        bool f2 = true;
        for (const auto& kv : p.second.kv) { if (!f2) { o += ","; } f2 = false; o += "\"" + jsonEsc(kv.first) + "\":\"" + jsonEsc(kv.second) + "\""; }
        o += "}}";
    }
    return o + "}}";
}

}   // namespace mst
#endif
