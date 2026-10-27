#ifndef LS_SELFAWARE_H
#define LS_SELFAWARE_H
// SelfAware.h 0.1.0 (2026-10-26) -- INDICATOR SELF-AWARENESS, SDK-free (unit-tested on Linux). Part of the Market Awareness Layer.
// Rassul 2026-10-26 13:41 CT: "build more self awareness ... tracking their own values to see where they are correct and incorrect.
// its a way for the indicators to self improve as well as to understand their own context ... the delta profile should be aware of
// itself and its values since the open and then how to improve its own read". 13:43: "go on self awareness".
//
// What one indicator keeps about ITSELF (per chart, per session; design SELF-AWARE.md):
//  1. its own VALUES since the open (one main metric, e.g. Delta Profile node ratio): n, mean, min, max, where the latest sits
//     (percentile of TODAY's values) - "is 3.1x big for today?"
//  2. its own SIGNALS since the open and their OUTCOMES, resolved only by bars that closed AFTER the signal (no look-ahead):
//     HIT = price moved 1 ATR the signal's way before 1 ATR against within HORIZON bars; MISS = the other way; EXPIRED = neither
//  3. its TRACK RECORD: today per context bucket + overall, and the LONG-RUN rate per context from the nightly knowledge base
//     (LRA-Self-<IND>-<MKT>.csv: PRIOR,<ctx>,<hits>,<n>)
//  4. a SELF-ADJUSTED confidence for each NEW signal, from the record BEFORE it (shrunk toward the long run: (h + K p) / (n + K));
//     applied only when SWITCH,1 (ships OFF: adjusted = base); a decided signal is never re-scored or repainted
//  5. its REASONS (inputs) recorded with every signal, and its self-state posted to MarketState as SELF<IND> with class-tagged keys:
//     M.* = METRIC facts (rates, counts, percentiles: factual, no opinion), D.* = DATA (counts / last value)
// Record lines (lsFlexLevels\Self-<IND>-<MKT>-<spb>s-<yyyymmdd>.rec, read by lra.self_aware nightly):
//   SELF|1|<ind>|<bar>|<ctx>|<side>|<base conf>|<adj conf>|<on>|<value>|<value pctile>|<today hits>/<today n>|<prior p>|<inputs>
//   SELFO|1|<ind>|<signal bar>|<resolved bar>|<HIT|MISS|EXP>|<bars>|<ctx>|<side>|<base>|<adj>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <deque>
#include <map>
#include <string>
#include <vector>
#include "MarketState.h"

namespace sa {

static const char* const VERSION = "0.1.0";
static const size_t MAX_SIGS = 300;      // light by design (PerfGuard): bounded buffers, only new bars are touched

struct Params {
    bool on = false;                 // SWITCH,1 = use the self-adjusted confidence (default OFF until the nightly proves it)
    double K = 10.0;                 // shrink strength toward the long run (cases)
    int horizon = 20;                // bars to resolve a signal
    int atrLen = 14;
    double minFactor = 0.5, maxFactor = 1.5;
    std::map<std::string, std::pair<double, double>> prior;   // ctx -> (hits, n) long run (from the nightly)
    std::string note = "defaults (self-awareness OFF)";
};
// LRA-Self-<IND>-<MKT>.csv: VERSION,1 / MARKET,ES / IND,DP / SWITCH,0|1 / PARAM,K,10 / PARAM,HORIZON,20 / PRIOR,<ctx>,<hits>,<n>
inline Params parseParams(const std::string& text, const std::string& market, const std::string& ind) {
    Params P; if (text.empty()) return P;
    std::string mkt, in; int good = 0; size_t a = 0;
    while (a <= text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        std::string line = text.substr(a, e - a); a = e + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> f = bb::split(line, ',', 5);
        auto dbl = [](const std::string& s, double& x) { char* q = nullptr; x = std::strtod(s.c_str(), &q); return q && *q == 0 && x == x; };
        double x = 0, y = 0;
        if (f.size() >= 2 && f[0] == "MARKET") mkt = f[1];
        else if (f.size() >= 2 && f[0] == "IND") in = f[1];
        else if (f.size() >= 2 && f[0] == "SWITCH") { P.on = f[1] == "1"; ++good; }
        else if (f.size() >= 3 && f[0] == "PARAM" && dbl(f[2], x)) {
            if (f[1] == "K" && x >= 0 && x <= 200) P.K = x;
            else if (f[1] == "HORIZON" && x >= 3 && x <= 100) P.horizon = (int)x;
            else if (f[1] == "MIN_FACTOR" && x >= 0.1 && x <= 1) P.minFactor = x;
            else if (f[1] == "MAX_FACTOR" && x >= 1 && x <= 3) P.maxFactor = x;
            ++good;
        }
        else if (f.size() >= 4 && f[0] == "PRIOR" && dbl(f[2], x) && dbl(f[3], y) && y > 0 && x >= 0 && x <= y) { P.prior[f[1]] = { x, y }; ++good; }
        if (e >= text.size()) break;
    }
    if ((!mkt.empty() && mkt != market) || (!in.empty() && in != ind)) { Params D; D.note = "file is for " + mkt + "/" + in + " - defaults"; return D; }
    P.note = std::to_string(good) + " lines" + (P.on ? " (ON)" : " (OFF)");
    return P;
}

struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0; };
struct Signal {
    long long t = 0; int side = 0; std::string ctx, inputs; double entry = 0, atr = 0, value = 0, pct = -1;
    int base = 0, adj = 0, bars = 0; int outcome = 9;   // 9 pending, 1 HIT, -1 MISS, 0 EXP
};

// time-of-day bucket from the bar's CT minute-of-day (a context known natively by every indicator)
inline std::string todBucket(int minOfDay) {
    if (minOfDay < 0 || minOfDay >= 24 * 60) return "?";
    if (minOfDay < 8 * 60 + 30) return "on";             // overnight / Globex before the cash open
    if (minOfDay < 9 * 60 + 30) return "open";           // first hour
    if (minOfDay < 11 * 60) return "am";
    if (minOfDay < 13 * 60) return "mid";
    if (minOfDay < 15 * 60 + 15) return "pm";
    return "on";
}

struct Tracker {
    std::string ind;                       // "DP", "LIQ", "AGG", ...
    Params P;
    std::string day;                       // session key (yyyymmdd of the session); a new key resets "today"
    std::deque<Bar> bars;                  // closed bars this session (capped)
    long long lastBar = 0;
    std::vector<double> values;            // own main metric values this session
    std::vector<Signal> sigs;              // this session's signals (decided once)
    std::map<std::string, std::pair<int, int>> todayCtx;   // ctx -> (hits, resolved)
    int hits = 0, resolved = 0, misses = 0, expired = 0, capped = 0;
    std::vector<std::string> pendingLines;                 // record lines not yet written (SELF / SELFO)

    std::string outLine(const Signal& s, long long tRes, int out) const {
        return "SELFO|1|" + ind + "|" + std::to_string(s.t) + "|" + std::to_string(tRes) + "|" + (out == 1 ? "HIT" : out == -1 ? "MISS" : "EXP") + "|" +
               std::to_string(s.bars) + "|" + mst::clean(s.ctx, 24) + "|" + std::to_string(s.side) + "|" + std::to_string(s.base) + "|" + std::to_string(s.adj);
    }
    void newSession(const std::string& d) {
        for (const Signal& s : sigs) if (s.outcome == 9) pendingLines.push_back(outLine(s, lastBar, 0));   // session ended unresolved = EXP
        day = d; bars.clear(); lastBar = 0; values.clear(); sigs.clear(); todayCtx.clear(); hits = resolved = misses = expired = capped = 0;
    }
    int pendingN() const { int n = 0; for (const Signal& s : sigs) n += s.outcome == 9; return n; }
    double atr() const {
        const int n = (int)bars.size(); if (n < 2) return 0;
        const int k = std::min(P.atrLen, n - 1); double s = 0;
        for (int i = n - k; i < n; ++i) { const Bar& b = bars[i]; const double pc = bars[i - 1].c;
            s += std::max(b.h - b.l, std::max(std::fabs(b.h - pc), std::fabs(b.l - pc))); }
        return k > 0 ? s / k : 0;
    }
    // where x sits among TODAY's own values (0..100); -1 = fewer than 5 values
    double pctile(double x) const {
        if (values.size() < 5) return -1;
        int below = 0; for (double v : values) below += v < x; return 100.0 * below / values.size();
    }
    double priorP(const std::string& ctx) const {
        auto it = P.prior.find(ctx); if (it != P.prior.end() && it->second.second > 0) return it->second.first / it->second.second;
        it = P.prior.find("all"); if (it != P.prior.end() && it->second.second > 0) return it->second.first / it->second.second;
        return -1;                                         // unknown long run
    }
    // 1. a CLOSED bar (call once per bar, in order). Resolves pending signals with bars strictly after their own bar.
    void onBar(const std::string& session, const Bar& b) {
        if (session != day) newSession(session);
        if (b.t <= lastBar) return;                        // each bar once, in order (a re-fed bar never changes anything)
        lastBar = b.t; bars.push_back(b); if (bars.size() > 600) bars.pop_front();
        if (pendingN() == 0) return;
        for (Signal& s : sigs) {
            if (s.outcome != 9 || b.t <= s.t) continue;
            ++s.bars;
            const double up = b.h - s.entry, dn = s.entry - b.l, R = s.atr;
            bool hit = false, miss = false;
            if (R > 0) { if (s.side > 0) { hit = up >= R; miss = dn >= R; } else { hit = dn >= R; miss = up >= R; } }
            if (hit && miss) { const bool closeWay = s.side > 0 ? b.c >= s.entry : b.c <= s.entry; hit = closeWay; miss = !closeWay; }   // both in one bar: the close decides (conservative)
            int out = hit ? 1 : miss ? -1 : (s.bars >= P.horizon ? 0 : 9);
            if (out == 9) continue;
            s.outcome = out;
            if (out == 1) ++hits; else if (out == -1) ++misses; else ++expired;
            if (out != 0) { ++resolved; auto& c = todayCtx[s.ctx]; c.second += 1; if (out == 1) c.first += 1; }
            pendingLines.push_back(outLine(s, b.t, out));
        }
    }
    // 2. the main metric value of a closed bar (e.g. the biggest node ratio)
    void onValue(double v) { if (v == v && std::isfinite(v)) { values.push_back(v); if (values.size() > 4000) values.erase(values.begin(), values.begin() + 1000); } }
    // 3. a NEW signal decided at bar t (side +1 / -1, base confidence 0-100). Returns the confidence to use: the self-adjusted one
    //    when SWITCH is on, else the base. Uses ONLY the record resolved before this call.
    int onSignal(long long t, int side, const std::string& ctx, int base, double value, const std::string& inputs, double entry) {
        Signal s; s.t = t; s.side = side > 0 ? 1 : -1; s.ctx = ctx.empty() ? "?" : ctx; s.inputs = inputs; s.entry = entry; s.atr = atr();
        s.value = value; s.pct = pctile(value); s.base = std::max(0, std::min(100, base));
        for (const Signal& o : sigs) if (o.t == t && o.side == s.side) return P.on ? o.adj : o.base;   // decided once per bar and side
        if (sigs.size() >= MAX_SIGS) { ++capped; return s.base; }   // a runaway signal source: stop tracking today, never slow the chart
        const double p = priorP(s.ctx);
        s.adj = s.base;
        if (p > 0) {
            const auto it = todayCtx.find(s.ctx); const double h = it == todayCtx.end() ? 0 : it->second.first, n = it == todayCtx.end() ? 0 : it->second.second;
            const double shrunk = (h + P.K * p) / (n + P.K);
            const double f = std::max(P.minFactor, std::min(P.maxFactor, shrunk / p));
            s.adj = (int)std::lround(std::max(0.0, std::min(100.0, s.base * f)));
        }
        const auto tc = todayCtx.find(s.ctx);
        const std::string rec = tc == todayCtx.end() ? "0/0" : std::to_string(tc->second.first) + "/" + std::to_string(tc->second.second);
        pendingLines.push_back("SELF|1|" + ind + "|" + std::to_string(t) + "|" + mst::clean(s.ctx, 24) + "|" + std::to_string(s.side) + "|" + std::to_string(s.base) + "|" +
                               std::to_string(s.adj) + "|" + (P.on ? "1" : "0") + "|" + mst::num(value) + "|" + (s.pct < 0 ? std::string("?") : mst::num(s.pct, 3)) + "|" +
                               rec + "|" + (p < 0 ? std::string("?") : mst::num(p, 3)) + "|" + mst::clean(inputs, 120));
        if (s.atr <= 0) { s.outcome = 0; ++expired; pendingLines.push_back(outLine(s, t, 0)); }     // no ATR yet (first bars): cannot be judged, never counted as a miss
        sigs.push_back(s);
        return P.on ? s.adj : s.base;
    }
    // 4. the self-state facts (class-tagged: M. = METRIC, D. = DATA; unknown = "?", never 0)
    std::map<std::string, std::string> facts(double lastValue) const {
        std::map<std::string, std::string> kv;
        int pending = 0; for (const Signal& s : sigs) pending += s.outcome == 9;
        kv["D.signals_today"] = std::to_string(sigs.size());
        kv["D.pending"] = std::to_string(pending);
        kv["M.today_hit"] = std::to_string(hits) + "/" + std::to_string(resolved);
        kv["M.today_rate"] = resolved > 0 ? mst::num((double)hits / resolved, 3) : "?";
        const double pa = priorP("all"); kv["M.longrun_rate"] = pa < 0 ? "?" : mst::num(pa, 3);
        kv["M.values_today"] = std::to_string(values.size());
        kv["D.value_last"] = lastValue == lastValue ? mst::num(lastValue) : "?";
        const double pc = lastValue == lastValue ? pctile(lastValue) : -1; kv["M.value_pctile"] = pc < 0 ? "?" : mst::num(pc, 3);
        if (!values.empty()) { double s = 0; for (double v : values) s += v; kv["M.value_mean"] = mst::num(s / values.size()); }
        else kv["M.value_mean"] = "?";
        if (!sigs.empty()) {                               // the latest signal: what it was worth, by its own base and its self-adjusted conf
            const Signal& z = sigs.back();
            kv["D.last_t"] = std::to_string(z.t); kv["D.last_side"] = z.side > 0 ? "bull" : "bear"; kv["D.last_ctx"] = z.ctx;
            kv["M.last_base"] = std::to_string(z.base); kv["M.last_adj"] = std::to_string(z.adj);
            const auto it = todayCtx.find(z.ctx); kv["M.ctx_today"] = it == todayCtx.end() ? "0/0" : std::to_string(it->second.first) + "/" + std::to_string(it->second.second);
            const double pc = priorP(z.ctx); kv["M.ctx_longrun"] = pc < 0 ? "?" : mst::num(pc, 3);
            kv["D.last_why"] = z.inputs.empty() ? "-" : z.inputs;
        }
        kv["M.self_on"] = P.on ? "1" : "0";
        if (capped) kv["D.capped"] = std::to_string(capped);
        kv["class"] = "METRIC";
        kv["v"] = VERSION;
        return kv;
    }
    std::vector<std::string> takeLines() { std::vector<std::string> o; o.swap(pendingLines); return o; }
};

// the session a Central wall-clock bar stamp belongs to: Globex day rolls at 17:00 CT (Sunday 17:00 opens Monday's session)
inline std::string sessionKey(long long tCt) {
    long long z = (tCt + 7LL * 3600) / 86400; if (tCt + 7LL * 3600 < 0) return "?";
    z += 719468; const long long era = z / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; long long y = (long long)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); const unsigned mp = (5 * doy + 2) / 153;
    const unsigned d = doy - (153 * mp + 2) / 5 + 1; const unsigned m = mp < 10 ? mp + 3 : mp - 9; y += m <= 2;
    char b[48]; std::snprintf(b, sizeof(b), "%04lld%02u%02u", y, m, d); return b;
}
inline int minuteOfDay(long long tCt) { long long r = tCt % 86400; if (r < 0) r += 86400; return (int)(r / 60); }
inline std::string recName(const std::string& ind, const std::string& market, long spb, const std::string& day) {
    return "Self-" + ind + "-" + market + "-" + std::to_string(spb) + "s-" + day + ".rec";
}
inline std::string paramsName(const std::string& ind, const std::string& market) { return "LRA-Self-" + ind + "-" + market + ".csv"; }
inline bool appendLines(const std::string& path, const std::vector<std::string>& lines) {
    if (lines.empty()) return true;
    FILE* f = std::fopen(path.c_str(), "ab"); if (!f) return false;
    bool ok = true; for (const std::string& l : lines) ok = std::fprintf(f, "%s\r\n", l.c_str()) > 0 && ok;
    return std::fclose(f) == 0 && ok;
}
inline std::string readText(const std::string& path) {
    std::string o; FILE* f = std::fopen(path.c_str(), "rb"); if (!f) return o; char b[4096]; size_t n;
    while ((n = std::fread(b, 1, sizeof(b), f)) > 0 && o.size() < 200000) o.append(b, n);
    std::fclose(f); return o;
}
// the board source name of an indicator's self-state ("SELFDP"); class METRIC (facts about its own record, not an opinion)
inline std::string boardSrc(const std::string& ind) { return "SELF" + ind; }

// ---- the per-indicator glue (one member per chart; every indicator uses the same 3 calls) ----
inline std::string flexDir() {
#if defined(_WIN32) && !defined(HB_FAKE)
    const char* up = std::getenv("USERPROFILE"); return (up && up[0]) ? std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string();
#else
    const char* d = std::getenv("LRA_FLEX_DIR"); return d ? std::string(d) : std::string();
#endif
}
inline std::string pathSep() {
#if defined(_WIN32) && !defined(HB_FAKE)
    return "\\";
#else
    return "/";
#endif
}
struct Hook {
    std::string ind, dir;                  // ind "DP"; dir empty = flexDir()
    Tracker tr;
    std::string loadedFor;                 // session + market the params were loaded for (re-read once per session)
    std::string forSym, forMkt;            // the symbol / market this memory is about: a symbol change starts a fresh memory
    long long postMs = -1, lastFed = 0;
    double lastValue = std::nan("");
    long writes = 0, writeFails = 0;
    std::string lastEvent;                 // fromFacts ON_CHANGE: the event value already turned into a signal
    std::string where() const { return dir.empty() ? flexDir() : dir; }
    void flush(const std::string& market, long spb) {
        std::vector<std::string> L = tr.takeLines(); if (L.empty()) return;
        const std::string d = where(); if (d.empty() || market.empty()) return;
        if (appendLines(d + pathSep() + recName(ind, market, spb, tr.day.empty() ? std::string("unknown") : tr.day), L)) writes += (long)L.size(); else ++writeFails;
    }
    // 1. each pass: the last CLOSED bar and this indicator's main value on it (NaN = none). Only a NEW bar does any work.
    void bar(const std::string& sym, long spb, const std::string& chart, const std::string& market, long long nowMs, const Bar& b, double value) {
        tr.ind = ind;
        if (sym != forSym) {                                // another symbol on this chart: its own memory (the old one's pending close as EXP)
            if (!forSym.empty()) { const std::string d = tr.day; tr.newSession(d); tr.day = d; flush(forMkt, spb); }
            const std::string keepInd = ind; tr = Tracker(); tr.ind = keepInd; loadedFor.clear(); lastFed = 0; lastEvent.clear(); forSym = sym; forMkt = market;
            lastValue = std::nan("");
        }
        if (b.t > lastFed) {
            const std::string sess = sessionKey(b.t);
            if (loadedFor != sess + "|" + market) {
                const std::string d = where();
                tr.P = parseParams(d.empty() ? std::string() : readText(d + pathSep() + paramsName(ind, market)), market, ind);
                loadedFor = sess + "|" + market;
            }
            tr.onBar(sess, b); tr.onValue(value); lastFed = b.t;
            if (value == value) lastValue = value;
            flush(market, spb);
        }
        if (postMs < 0 || nowMs < postMs || nowMs - postMs >= 15000) { mst::publish(boardSrc(ind), sym, spb, chart, nowMs, lastFed, tr.facts(lastValue)); postMs = nowMs; }
    }
    // 2. a NEW signal decided on closed bar t (once). ctx empty = time-of-day bucket. Returns the confidence to show.
    int signal(const std::string& market, long spb, long long t, int side, const std::string& ctx, int base, double value, const std::string& inputs, double entry) {
        const int c = tr.onSignal(t, side, ctx.empty() ? todBucket(minuteOfDay(t)) : ctx, base, value, inputs, entry);
        flush(market, spb); postMs = -1;   // re-post on the next pass
        return c;
    }
    void clear(const std::string& sym, long spb, const std::string& chart) { mst::clear(boardSrc(ind), sym, spb, chart); }
};

// ---- declarative glue for indicators that already post MarketState facts once per closed bar (SELF-AWARE.md section 6) ----
// a side word -> +1 / -1 / 0 (unknown words, "?", "none" -> 0)
inline int sideOf(const std::string& w0) {
    std::string w; for (char c : w0) w += (char)std::tolower((unsigned char)c);
    static const char* const UP[] = { "bull", "buy", "bid", "support", "up", "long", "lod", "above", "above1", "above2", "1", "+1", "buyers", nullptr };
    static const char* const DN[] = { "bear", "sell", "ask", "resistance", "resist", "down", "short", "hod", "below", "below1", "below2", "-1", "sellers", nullptr };
    for (const char* const* u = UP; *u; ++u) if (w == *u || w.rfind(std::string(*u) + "_", 0) == 0) return 1;
    for (const char* const* d = DN; *d; ++d) if (w == *d || w.rfind(std::string(*d) + "_", 0) == 0) return -1;
    return 0;
}
struct Spec {
    std::string valueKey;                  // the indicator's main value on each closed bar ("?" = none)
    std::string sideKey;                   // where the signal's side is read
    std::string eventKey;                  // ON_CHANGE: a new value here = a new signal (empty = sideKey)
    bool onChange = false;                 // false: a signal on every closed bar whose side word is a side; true: only when eventKey changes
    std::string confKey;                   // optional base confidence (0-1 or 0-100); empty / "?" = 50
    std::vector<std::string> why;          // keys copied into the signal's reasons
    bool invert = false;                   // a side word that means the OPPOSITE price direction (e.g. a trap of buyers = down)
    std::string gateKey;                   // optional: a signal only while this fact is known and not "none" (e.g. LIQ zone_step)
    std::map<std::string, int> sideMap;    // optional: exact side words of this indicator -> +1 / -1 (checked before sideOf)
    bool rearm = false;                    // ON_CHANGE: an unknown / "none" event re-arms (the same value later is a NEW signal)
    bool allowLate = false;                // the facts of the SAME closed bar may still gain the signal (a record written seconds after the close)
};
inline double factNum(const std::map<std::string, std::string>& kv, const std::string& k) {
    auto it = kv.find(k); if (k.empty() || it == kv.end() || it->second.empty() || it->second == "?") return std::nan("");
    char* e = nullptr; const double v = std::strtod(it->second.c_str(), &e); return (e && (*e == 0 || *e == 'x')) ? v : std::nan("");
}
inline std::string factStr(const std::map<std::string, std::string>& kv, const std::string& k) {
    auto it = kv.find(k); return it == kv.end() ? std::string("?") : it->second;
}
// feed one closed bar + the facts the indicator posted FOR THAT BAR. Returns the confidence of a new signal, or -1 when none.
inline int fromFacts(Hook& h, const Spec& sp, const std::string& sym, long spb, const std::string& chart, const std::string& market, long long nowMs,
                     const Bar& b, const std::map<std::string, std::string>& kv) {
    const bool fresh = b.t > h.lastFed, same = b.t > 0 && b.t == h.lastFed;
    h.bar(sym, spb, chart, market, nowMs, b, factNum(kv, sp.valueKey));
    if (!fresh && !(sp.allowLate && same)) return -1;                  // never an older bar; the same bar only when allowed
    if (sp.onChange && sp.rearm) { const std::string ev = factStr(kv, sp.eventKey.empty() ? sp.sideKey : sp.eventKey); if (ev == "?" || ev.empty() || ev == "none") h.lastEvent.clear(); }
    const std::string sw = factStr(kv, sp.sideKey); const auto sm = sp.sideMap.find(sw);
    int side = sm != sp.sideMap.end() ? sm->second : sideOf(sw); if (sp.invert) side = -side;
    if (side == 0) { if (sp.onChange && sp.rearm) h.lastEvent.clear(); return -1; }   // a no-side bar re-arms (rearm)
    if (!sp.gateKey.empty()) { const std::string g = factStr(kv, sp.gateKey); if (g == "?" || g == "none" || g.empty()) return -1; }
    if (sp.onChange) {
        const std::string ev = factStr(kv, sp.eventKey.empty() ? sp.sideKey : sp.eventKey);
        if (ev == "?" || ev.empty() || ev == "none" || ev == h.lastEvent) return -1;
        h.lastEvent = ev;
    }
    double c = factNum(kv, sp.confKey); int base = 50;
    if (c == c) base = (int)std::lround(c <= 1.0 ? c * 100.0 : c);
    std::string why = sp.sideKey + "=" + factStr(kv, sp.sideKey);
    for (const std::string& k : sp.why) why += " " + k + "=" + factStr(kv, k);
    return h.signal(market, spb, b.t, side, std::string(), base, factNum(kv, sp.valueKey), why, b.c);
}

}  // namespace sa
#endif
