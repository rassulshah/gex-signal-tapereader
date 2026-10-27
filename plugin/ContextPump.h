#ifndef LS_CONTEXTPUMP_H
#define LS_CONTEXTPUMP_H
// ContextPump.h 0.2.0 (2026-10-26; 0.2.0 CTX posted class-tagged, TEAM posted as class READ) -- what lsContext does on every calc / 250 ms tick, SDK-free (unit-tested on Linux with HB_FAKE).
// lsContext.cpp only copies the chart's closed bars in and calls step(); everything that decides is here.
//
//   step(now):  t = the last CLOSED bar (copied in at the last calc)
//               present = MarketState sources with fresh facts on this chart; posted = the bar each last decided
//               Collector: build CTX(t) when every present source is at t, or after the timeout (late ones listed)
//               CTX(t) posted ONCE per bar (decided once; re-posted unchanged every 10 s so readers see it)
//               TEAM(t) from the AW<src> reads decided for t, posted once they have all arrived or at the timeout
//               record lines CTX|... and TEAM|... returned for the caller to append to its file (one per bar each)
// Links OFF (the default) changes nothing an indicator shows: CTX / TEAM are posted and recorded, nothing is drawn.
#include <map>
#include <set>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include "ContextLogic.h"
#include "ContextRead.h"
#include "AwarePlus.h"     // (0.3.0) awareness_plus: expectations, news / shock, source health, market personality

namespace cp {

static const char* const VERSION = "0.3.0";   // (0.3.0, 2026-10-26) awareness_plus (ap::Plus): EXP / NEWS / HEALTH, METRIC fields in CTX, nothing drawn changes
// the teammates whose aware reads make up TEAM (P3: the first four hooks)
static const char* const AW_SOURCES[] = { "AWSTAGE", "AWDP", "AWAGG", "AWHODLOD", nullptr };
// the sources lsContext waits for besides MarketState::SOURCES (posted by the P3 hooks / later sources)
static const char* const EXTRA_SOURCES[] = { "STAGE", "TREND1H", nullptr };

// RTH open, CT minute of day - the same numbers as HodLodLogic.h hl::MKTS (the studies' sessions)
inline int rthOpenOf(const std::string& m) {
    if (m == "CL" || m == "NG") return 480;
    if (m == "GC" || m == "HG" || m == "EU") return 440;
    return 510;
}
// the market of a chart's root symbol (the same names as LetterStrips.h lst::marketName)
inline std::string marketOf(std::string r) {
    for (char& c : r) c = (char)std::toupper((unsigned char)c);
    auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    return has("NQ") ? "NQ" : has("GC") ? "GC" : (has("CL") || r == "QM") ? "CL" : (has("HG") || has("CP")) ? "HG"
         : (has("NG") || r == "QG") ? "NG" : (has("EU") || has("6E") || has("E6")) ? "EU" : (has("ES") || has("EP")) ? "ES" : "UNKNOWN";
}
// the nightly trust table LRA-Context-trust-<MKT>.csv: VERSION,1 / MARKET,<MKT> / TRUST,<src>,<weight 0..1>; a wrong market or bad line -> empty
inline std::map<std::string, double> parseTrust(const std::string& text, const std::string& market) {
    std::map<std::string, double> T; std::string mkt; size_t a = 0;
    while (a < text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        std::string line = text.substr(a, e - a); a = e + 1; if (!line.empty() && line.back() == '\r') line.pop_back();
        std::vector<std::string> f; size_t b = 0; while (true) { size_t c = line.find(',', b); f.push_back(line.substr(b, c == std::string::npos ? std::string::npos : c - b)); if (c == std::string::npos) break; b = c + 1; }
        if (f.size() >= 2 && f[0] == "MARKET") mkt = f[1];
        if (f.size() >= 3 && f[0] == "TRUST") { char* x = nullptr; const double w = std::strtod(f[2].c_str(), &x); if (x && *x == 0 && w >= 0 && w <= 1) T[f[1]] = w; }
    }
    if (mkt != market) T.clear();
    return T;
}

inline long daysFromCivil(int y, int m, int d) {
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long)doe - 719468;
}
// the bar id every MarketState source uses: CT wall-clock seconds of the bar's END stamp (SignalBoard convention)
inline long long barId(int y, int mo, int d, int hh, int mi, int ss) { return (long long)daysFromCivil(y, mo, d) * 86400LL + hh * 3600LL + mi * 60LL + ss; }

// key levels from the facts already on the board (bar <= t only): VWAP (key), Delta Profile node (volume), resting zone (bigorder),
// gamma band edges (gamma). More types arrive when the Key Levels / FlexLevels source posts (LEVELS, action 2d).
inline std::vector<ctx::Level> levelsFromFacts(const std::map<std::string, mst::Facts>& F, long long t) {
    std::vector<ctx::Level> L;
    auto add = [&](const char* src, const char* k, const char* name, const char* type) {
        auto it = F.find(src); if (it == F.end() || !it->second.ok || it->second.bar > t) return;
        const double px = mst::getD(it->second, k, 0); if (px > 0) L.push_back(ctx::Level{ px, name, type });
    };
    add("VWAP", "vwap", "vwap", "key");
    add("DP", "node_px", "dp node", "volume");
    add("LIQ", "level", "resting zone", "bigorder");
    add("HODLOD", "gex_band_hi", "gamma hi", "gamma");
    add("HODLOD", "gex_band_lo", "gamma lo", "gamma");
    return L;
}

struct Out { bool ctxPosted = false, teamPosted = false; std::string ctxLine, teamLine, state; std::vector<std::string> plusLines; };

struct Pump {
    std::string sym, chart, market; long spb = 0;
    std::vector<ctx::Bar> bars;            // closed bars, oldest first (copied in by the shell)
    double tick = 0.25; int rthOpenMin = 510;
    cr::Links links;                       // per-market switches (all OFF by default); timeout from the file
    std::map<std::string, double> trust;   // nightly trust table (empty = no side on conflicts)
    ctx::Collector col;
    long long ctxBar = -1, teamBar = -1, ctxMs = -1, teamMs = -1, teamOpenMs = -1;
    std::map<std::string, std::string> ctxKv, teamKv;
    ap::Plus plus;                         // (0.3.0) awareness_plus (the shell loads its market file + switch)

    long long lastClosed() const { return bars.empty() ? -1 : bars.back().t; }

    Out step(long long nowMs) {
        Out o; const long long t = lastClosed();
        if (t < 0 || sym.empty()) { o.state = "no bars"; return o; }
        col.timeoutMs = links.timeoutMs;
        // what is on the board for this chart
        std::map<std::string, mst::Facts> F = mst::readAll(sym, spb, chart, nowMs);
        for (const char* const* s = EXTRA_SOURCES; *s; ++s) { mst::Facts f = mst::read(*s, sym, spb, chart, nowMs); if (f.ok) F[*s] = f; }
        // (0.3.0) graceful failure: a stale / lagging / missing source is a team fact and is left out of CTX(t) (and TEAM below)
        { const std::set<std::string> excl = plus.preCtx(sym, spb, chart, nowMs, bars); for (const auto& e : excl) F.erase(e); }
        std::set<std::string> present; std::map<std::string, long long> posted;
        for (const auto& kv : F) { present.insert(kv.first); posted[kv.first] = kv.second.bar; }
        // 1. CTX(t), decided once
        if (ctxBar != t) {
            std::vector<std::string> late;
            bool go = col.ready(t, nowMs, present, posted, late);
            if (go && present.empty() && nowMs - col.openedMs < col.timeoutMs) go = false;   // an empty board (start-up) waits the timeout too
            if (go) {
                ctx::Inputs in; in.bars = bars; in.facts = F; in.levels = levelsFromFacts(F, t); in.tick = tick; in.rthOpenMin = rthOpenMin;
                in.present = present; in.late = std::set<std::string>(late.begin(), late.end());
                ctxKv = ctx::buildCtx(in, t);
                { const ctx::Bar& lb = bars.back(); ctxKv["bar_h"] = mst::num(lb.h, 12); ctxKv["bar_l"] = mst::num(lb.l, 12); ctxKv["bar_c"] = mst::num(lb.c, 12); }   // the nightly link test scores reads from these
                ctxKv = ctx::tagCtx(ctxKv);                      // vocabulary v2: D. / M. facts, R. opinions (value~conf~source)
                plus.foldCtx(ctxKv);                             // (0.3.0) M.news / M.shock / M.src_* / M.exp_* / D.mkt_* (METRIC, never READ)
                plus.foldNext(ctxKv, sym, spb, nowMs, bars, rthOpenMin);   // (0.3.0) memory beyond today + the other timeframes (keyed tf<spb>_)
                ctxBar = t; ctxMs = -1; teamOpenMs = nowMs; teamBar = -1;
                o.ctxLine = "CTX|" + std::to_string(t) + "|" + kvLine(ctxKv);
            } else { o.state = "waiting"; }
        }
        if (ctxBar == t && (ctxMs < 0 || nowMs - ctxMs >= 10000 || nowMs < ctxMs)) { mst::publish("CTX", sym, spb, chart, nowMs, t, ctxKv); ctxMs = nowMs; o.ctxPosted = true; }
        // 2. TEAM(t) once the aware reads for t are in (or the timeout after CTX(t))
        if (ctxBar == t && teamBar != t) {
            std::vector<ctx::AwareRead> R; int presentAw = 0, atT = 0;
            for (const char* const* s = AW_SOURCES; *s; ++s) {
                mst::Facts f = mst::read(*s, sym, spb, chart, nowMs); if (!f.ok) continue;
                if (!plus.mon.usable(*s) || !plus.mon.usable(std::string(*s).substr(2))) continue;   // (0.3.0) a stale source is not a teammate now
                ++presentAw; if (f.bar == t) ++atT;
                if (f.bar > t) continue;                      // never a read decided after t
                ctx::AwareRead a; a.src = std::string(*s).substr(2); a.side = (int)mst::getD(f, "side", 0); a.conf = (int)mst::getD(f, "conf", 0); a.bar = f.bar;
                R.push_back(a);
            }
            if ((presentAw > 0 && atT == presentAw) || nowMs - teamOpenMs >= links.timeoutMs) {   // nobody posting reads yet -> wait the timeout
                teamKv = ctx::teamRead(t, R, trust); teamKv["class"] = "READ"; teamKv["aw_present"] = std::to_string(presentAw); teamKv["aw_at_t"] = std::to_string(atT);
                plus.foldTeam(teamKv, ctxKv);                // (0.3.0) conf_x + news flag + excluded sources + agreement across timeframes
                teamBar = t; teamMs = -1;
                plus.postTeam(sym, spb, chart, nowMs, bars, ctxKv);   // (0.3.0) each aware read registers what it expects next
                o.teamLine = "TEAM|" + std::to_string(t) + "|" + kvLine(teamKv);
            }
        }
        if (teamBar == t && (teamMs < 0 || nowMs - teamMs >= 10000 || nowMs < teamMs)) { mst::publish("TEAM", sym, spb, chart, nowMs, t, teamKv); teamMs = nowMs; o.teamPosted = true; }
        if (ctxBar == t) plus.publish(sym, spb, chart, nowMs);   // (0.3.0) EXP / NEWS / HEALTH, every 10 s
        if (teamBar == t) plus.publishTf(sym, spb, nowMs);       // (0.3.0) this chart for the other timeframes (TF, chart-agnostic key)
        o.plusLines = plus.take();
        if (o.state.empty()) o.state = ctxBar == t ? (teamBar == t ? "ok" : "ctx ok, team waiting") : "waiting";
        return o;
    }
    void clear() { if (!sym.empty()) { mst::clear("CTX", sym, spb, chart); mst::clear("TEAM", sym, spb, chart); plus.clear(sym, spb, chart); } ctxBar = teamBar = -1; col.reset(); }
    static std::string kvLine(const std::map<std::string, std::string>& kv) {
        std::string s; for (const auto& p : kv) { if (!s.empty()) s += ";"; s += p.first + "=" + mst::clean(p.second, 200); } return s;
    }
};

}  // namespace cp
#endif
