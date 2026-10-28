#ifndef LS_SIGNALBOARD_H
#define LS_SIGNALBOARD_H
// SignalBoard.h 1.3.0 (1.3.0, 2026-10-27: kind 'L' = lsBigBidAsk BO letters as drawn: bar, side +1 above / -1 below the candle, val = pixels from
// the candle's extreme (high above / low below) to the letter's OUTER edge - lsLiquidityProfile stacks its Ex letter straight against it)
// SignalBoard.h 1.2.0 (1.2.0, 2026-10-21: kind 'O' = lsBidAskStats ORDER-BOOK ABSORPTION per decided candle, up to 160 events)
// SignalBoard.h 1.1.0 (1.0.0 2026-10-15, Rassul "how can you better ensure integration between the indicators and signals so they are aware
// of each other") - the per-chart SIGNAL board: each indicator posts its latest decided events for THIS chart (process environment,
// as BannerBoard / ColumnBoard), any other indicator reads them within a second. SDK-free.
// 1.1.0 (2026-10-18, audit B finding 1): kind 'A' carries flag 1 = FINAL / 2 = TENTATIVE; read() hands out only final A events
//   (existing callers such as lsLiquidityProfile keep working and no longer see a tentative A); readAll() returns every event.
//   kind 'A'  lsDeltaProfile   absorption A: bar, price, side (+1 support / -1 resistance), val = strength (x),
//                              flag 1 = FINAL (its swing pivot is confirmed; the record has its D line; it never changes again)
//                              flag 2 = TENTATIVE (close rule + pivot so far hold, pivot not confirmed: it may vanish / come back)
//                              flag 0 = written by lsDeltaProfile 2.16.0 or older (no flag): treated as final (finalA) for compatibility
//            readers that want only final A's: sb::read('A', ...) (filters) or sb::finalA(e) / sb::nearestFinal(...) on readAll() output
//   kind 'B'  lsBigBidAsk      big print: bar, price, side (-1 bid hit / +1 ask lifted), val = lots; flag bit 0 = absorption circle,
//                              bits 1-3 category (1 - 4), 4 at a level, 5 sweep, 6 pivot rule passed, 7 (1.15.0+) pivot CONFIRMED,
//                              8-14 Big Lot N (0 = category 4). Closed candles only, from the record - never from what is on screen.
//   kind 'F'  lsLiquidityProfile a big resting order FILLED: bar, price, side (-1 bid / +1 ask), val = lots traded
//   kind 'O'  lsBidAskStats    (1.2.0) order-book absorption at one resting price of a decided candle: bar, price, side = the RESTING
//                              side (-1 bids absorbed sellers / +1 offers absorbed buyers), val = lots traded there, flag = shown x 100 +
//                              refills (capped 99): traded >= 2x the most ever shown there AND >= 2 refills (OrderFlowFeed obAbsorbed)
// bar = the chart bar's stamp as Central wall-clock seconds (every plugin converts the same RTDATE the same way). Entries older than
// 60 s (the publisher stopped drawing / was removed) are ignored. At most 40 events per kind, newest kept.
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "BannerBoard.h"
namespace sb {
static const char* const VERSION = "1.3.0";
static const long long FRESH_MS = 60000; static const size_t CAP = 40;
struct Ev { long long bar = 0; double px = 0; int side = 0; double val = 0; int flag = 0; };
inline const char* prefix(char k) { return k == 'A' ? "LRA_SGA_" : k == 'B' ? "LRA_SGB_" : k == 'O' ? "LRA_SGO_" : k == 'L' ? "LRA_SGL_" : "LRA_SGF_"; }
static const size_t CAP_O = 160;   // (1.2.0) kind 'O' carries several prices per candle
inline size_t capOf(char k) { return k == 'O' ? CAP_O : CAP; }
inline int obFlag(long long shown, int refills) { if (shown < 0) shown = 0; if (shown > 20000000) shown = 20000000; return (int)(shown * 100 + (refills > 99 ? 99 : refills < 0 ? 0 : refills)); }
inline long long obShown(int flag) { return flag / 100; }
inline int obRefills(int flag) { return flag % 100; }
inline std::string enc(long long ms, const std::vector<Ev>& v, size_t cap = CAP)
{
    std::string o = std::to_string(ms) + "|"; const size_t from = v.size() > cap ? v.size() - cap : 0;
    for (size_t i = from; i < v.size(); ++i) { char b[96]; std::snprintf(b, sizeof(b), "%lld,%.10g,%d,%.6g,%d;", v[i].bar, v[i].px, v[i].side, v[i].val, v[i].flag); o += b; }
    return o;
}
inline bool dec(const std::string& s, long long nowMs, std::vector<Ev>& out, size_t cap = CAP)
{
    out.clear(); if (s.empty() || s.size() > 16000) return false;
    const size_t bar = s.find('|'); if (bar == std::string::npos) return false;
    long long ms = 0; if (!bb::num(s.substr(0, bar), ms)) return false;
    if (nowMs >= 0 && (nowMs < ms || nowMs - ms > FRESH_MS)) return false;
    size_t a = bar + 1;
    while (a < s.size() && out.size() < cap) {
        const size_t e = s.find(';', a); if (e == std::string::npos) break;
        Ev v; long long b2 = 0; double px = 0, val = 0; int side = 0, flag = 0;
        if (std::sscanf(s.substr(a, e - a).c_str(), "%lld,%lf,%d,%lf,%d", &b2, &px, &side, &val, &flag) == 5 && std::isfinite(px) && std::isfinite(val)) {
            v.bar = b2; v.px = px; v.side = side; v.val = val; v.flag = flag; out.push_back(v); }
        a = e + 1;
    }
    return true;
}
inline void publish(char k, const std::string& sym, long spb, const std::string& chart, long long ms, const std::vector<Ev>& v)
{ if (!sym.empty()) bb::bbSet(bb::key(prefix(k), sym, spb, chart), enc(ms, v, capOf(k))); }
// (1.1.0) an A event that is final: flag 1, or 0 (a pre-1.1.0 publisher that set no flag). flag 2 = tentative.
static const int A_FINAL = 1, A_TENT = 2;
inline bool finalA(const Ev& e) { return e.flag != A_TENT; }
// every event of a kind, tentative A's included (a reader that wants to show "A?" uses this and checks e.flag)
inline bool readAll(char k, const std::string& sym, long spb, const std::string& chart, long long nowMs, std::vector<Ev>& out)
{ std::string v; out.clear(); return !sym.empty() && bb::bbGet(bb::key(prefix(k), sym, spb, chart), v) && dec(v, nowMs, out, capOf(k)); }
// (1.1.0) the events a decision may use: for kind 'A' only FINAL ones (a tentative A can still vanish), other kinds unchanged
inline bool read(char k, const std::string& sym, long spb, const std::string& chart, long long nowMs, std::vector<Ev>& out)
{
    if (!readAll(k, sym, spb, chart, nowMs, out)) return false;
    if (k == 'A') { std::vector<Ev> f; for (size_t i = 0; i < out.size(); ++i) if (finalA(out[i])) f.push_back(out[i]); out.swap(f); }
    return true;
}
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
// (1.1.0) as nearestEv, but a tentative A (flag 2) never counts - for readers that use readAll()
inline const Ev* nearestFinal(const std::vector<Ev>& v, double p, double tol, long long b0, long long b1, int side)
{
    const Ev* best = nullptr;
    for (size_t i = 0; i < v.size(); ++i) {
        if (!finalA(v[i]) || std::fabs(v[i].px - p) > tol || v[i].bar < b0 || v[i].bar > b1 || (side != 0 && v[i].side != side)) continue;
        if (!best || v[i].val > best->val) best = &v[i];
    }
    return best;
}
}  // namespace sb
#endif
