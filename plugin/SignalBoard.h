#ifndef LS_SIGNALBOARD_H
#define LS_SIGNALBOARD_H
// SignalBoard.h 1.0.0 (2026-10-15, Rassul "how can you better ensure integration between the indicators and signals so they are aware
// of each other") - the per-chart SIGNAL board: each indicator posts its latest decided events for THIS chart (process environment,
// as BannerBoard / ColumnBoard), any other indicator reads them within a second. SDK-free.
//   kind 'A'  lsDeltaProfile   confirmed absorption A: bar, price, side (+1 support / -1 resistance), val = strength (x), flag 0
//   kind 'B'  lsBigBidAsk      bold size: bar, price, side (-1 bid hit / +1 ask lifted), val = lots, flag 1 = absorption circle
//   kind 'F'  lsLiquidityProfile a big resting order FILLED: bar, price, side (-1 bid / +1 ask), val = lots traded
// bar = the chart bar's stamp as Central wall-clock seconds (every plugin converts the same RTDATE the same way). Entries older than
// 60 s (the publisher stopped drawing / was removed) are ignored. At most 40 events per kind, newest kept.
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "BannerBoard.h"
namespace sb {
static const char* const VERSION = "1.0.0";
static const long long FRESH_MS = 60000; static const size_t CAP = 40;
struct Ev { long long bar = 0; double px = 0; int side = 0; double val = 0; int flag = 0; };
inline const char* prefix(char k) { return k == 'A' ? "LRA_SGA_" : k == 'B' ? "LRA_SGB_" : "LRA_SGF_"; }
inline std::string enc(long long ms, const std::vector<Ev>& v)
{
    std::string o = std::to_string(ms) + "|"; const size_t from = v.size() > CAP ? v.size() - CAP : 0;
    for (size_t i = from; i < v.size(); ++i) { char b[96]; std::snprintf(b, sizeof(b), "%lld,%.10g,%d,%.6g,%d;", v[i].bar, v[i].px, v[i].side, v[i].val, v[i].flag); o += b; }
    return o;
}
inline bool dec(const std::string& s, long long nowMs, std::vector<Ev>& out)
{
    out.clear(); if (s.empty() || s.size() > 8000) return false;
    const size_t bar = s.find('|'); if (bar == std::string::npos) return false;
    long long ms = 0; if (!bb::num(s.substr(0, bar), ms)) return false;
    if (nowMs >= 0 && (nowMs < ms || nowMs - ms > FRESH_MS)) return false;
    size_t a = bar + 1;
    while (a < s.size() && out.size() < CAP) {
        const size_t e = s.find(';', a); if (e == std::string::npos) break;
        Ev v; long long b2 = 0; double px = 0, val = 0; int side = 0, flag = 0;
        if (std::sscanf(s.substr(a, e - a).c_str(), "%lld,%lf,%d,%lf,%d", &b2, &px, &side, &val, &flag) == 5 && std::isfinite(px) && std::isfinite(val)) {
            v.bar = b2; v.px = px; v.side = side; v.val = val; v.flag = flag; out.push_back(v); }
        a = e + 1;
    }
    return true;
}
inline void publish(char k, const std::string& sym, long spb, const std::string& chart, long long ms, const std::vector<Ev>& v)
{ if (!sym.empty()) bb::bbSet(bb::key(prefix(k), sym, spb, chart), enc(ms, v)); }
inline bool read(char k, const std::string& sym, long spb, const std::string& chart, long long nowMs, std::vector<Ev>& out)
{ std::string v; out.clear(); return !sym.empty() && bb::bbGet(bb::key(prefix(k), sym, spb, chart), v) && dec(v, nowMs, out); }
// the strongest event of a kind near (price, bar window): |px - p| <= tol, bar in [b0, b1], side match (0 = any); nullptr = none
inline const Ev* nearestEv(const std::vector<Ev>& v, double p, double tol, long long b0, long long b1, int side)
{
    const Ev* best = nullptr;
    for (size_t i = 0; i < v.size(); ++i) {
        if (std::fabs(v[i].px - p) > tol || v[i].bar < b0 || v[i].bar > b1 || (side != 0 && v[i].side != side)) continue;
        if (!best || v[i].val > best->val) best = &v[i];
    }
    return best;
}
}  // namespace sb
#endif
