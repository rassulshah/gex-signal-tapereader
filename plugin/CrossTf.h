#ifndef LS_CROSSTF_H
#define LS_CROSSTF_H
// CrossTf.h 0.1.0 (2026-10-26) -- awareness_plus NEXT item 6, CROSS-TIMEFRAME teamwork: the 30-s / 1-min / 3-min / hourly charts of the
// same market share one board, keyed per timeframe. SDK-free, unit-tested on Linux.
//   publish  after TEAM(t) every lsContext posts a short summary of its own chart on a CHART-AGNOSTIC key (source TF, chart "any"):
//            LRA_MS_TF_<symbol>_<spb>_any = <ms>|<bar t>|team=..;trend=..;wave=..;leg=..;stretched=..;at_level=..;news=..;atr=..
//            (two charts of one symbol x bar size write the same thing; the last writer wins)
//   fold     when CTX(t) is built, every OTHER timeframe's summary is read and copied in with its class kept and the timeframe in the key:
//            R.tf3600_team = <side>~?~TF3600 (another chart's team read is an OPINION), R.tf3600_trend, M.tf3600_stretched,
//            M.tf3600_age = bars of that timeframe since its summary bar
//            No look-ahead: a summary whose bar END is after t is NOT used (its value becomes '?', reason 'ahead'); a summary older
//            than 2 of its own bars is stale ('?', reason 'stale').
//   agree    after TEAM(t): tf_agree / tf_against = how many other timeframes' team sides agree / disagree with this chart's (TEAM keys)
// Nothing here changes a read: links stay OFF; the nightly tests whether agreement across timeframes separates outcomes.
#include <map>
#include <string>
#include <vector>
#include "MarketState.h"

namespace aptf {

static const char* const VERSION = "0.1.0";
static const long TFS[] = { 30, 60, 180, 3600, 0 };

inline std::string get(const std::map<std::string, std::string>& kv, const std::string& k) {   // tagged or plain, R. value without ~conf~src
    for (const char* p : { "D.", "M.", "R." }) { auto it = kv.find(std::string(p) + k); if (it != kv.end()) { const size_t q = it->second.find('~'); return q == std::string::npos ? it->second : it->second.substr(0, q); } }
    auto it = kv.find(k); return it == kv.end() ? std::string("?") : it->second;
}

inline std::map<std::string, std::string> summary(const std::map<std::string, std::string>& ctxKv, const std::map<std::string, std::string>& teamKv) {
    std::map<std::string, std::string> s;
    auto t = teamKv.find("side"); s["team"] = t == teamKv.end() ? "?" : t->second;
    auto c = teamKv.find("conflict"); s["conflict"] = c == teamKv.end() ? "?" : c->second;
    s["trend"] = get(ctxKv, "trend_3m"); s["wave"] = get(ctxKv, "wave_dir"); s["leg"] = get(ctxKv, "leg_no"); s["stretched"] = get(ctxKv, "stretched");
    s["at_level"] = get(ctxKv, "at_level"); s["news"] = get(ctxKv, "news"); s["atr"] = get(ctxKv, "atr"); s["vwap_pos"] = get(ctxKv, "vwap_pos");
    return s;
}

inline void publish(const std::string& sym, long spb, long long nowMs, long long t, const std::map<std::string, std::string>& s) {
    mst::publish("TF", sym, spb, "any", nowMs, t, s);
}

// the class each summary field carries into another chart's CTX
inline char clsOf(const std::string& k) {
    if (k == "team" || k == "conflict" || k == "trend" || k == "vwap_pos") return 'R';
    if (k == "at_level") return 'D';
    return 'M';
}

// copy the other timeframes into CTX(t); returns the timeframes used
inline int fold(std::map<std::string, std::string>& kv, const std::string& sym, long ownSpb, long long nowMs, long long t) {
    int used = 0; std::string seen;
    for (const long* p = TFS; *p; ++p) {
        const long spb = *p; if (spb == ownSpb) continue;
        const mst::Facts f = mst::read("TF", sym, spb, "any", nowMs);
        const std::string pre = "tf" + std::to_string(spb) + "_";
        if (!f.ok) continue;                                            // that timeframe is not open: no keys (absent = not part of the team)
        std::string why;
        if (f.bar > t) why = "ahead";                                   // decided after t: never used for CTX(t)
        else if ((t - f.bar) / spb > 2) why = "stale";
        if (!seen.empty()) seen += ",";
        seen += std::to_string(spb) + (why.empty() ? "" : ":" + why);
        kv["M." + pre + "age"] = why.empty() ? std::to_string((t - f.bar) / spb) : "?";
        for (const auto& e : f.kv) {
            const char c = clsOf(e.first);
            std::string v = why.empty() ? e.second : std::string("?");
            if (c == 'R' && v != "?") v += "~?~TF" + std::to_string(spb);
            kv[std::string(1, c) + "." + pre + e.first] = v;
        }
        if (why.empty()) ++used;
    }
    kv["M.tf_seen"] = seen.empty() ? "-" : seen;
    return used;
}

// agreement of this chart's TEAM side with the other timeframes' team sides (already folded into CTX(t))
inline void agree(std::map<std::string, std::string>& teamKv, const std::map<std::string, std::string>& ctxKv) {
    auto s = teamKv.find("side"); const int side = s == teamKv.end() ? 0 : std::atoi(s->second.c_str());
    int a = 0, d = 0; std::string who;
    for (const long* p = TFS; *p; ++p) {
        auto it = ctxKv.find("R.tf" + std::to_string(*p) + "_team"); if (it == ctxKv.end()) continue;
        const int o = std::atoi(it->second.substr(0, it->second.find('~')).c_str()); if (o == 0 || side == 0) continue;
        if (o == side) { ++a; who += std::to_string(*p) + "+ "; } else { ++d; who += std::to_string(*p) + "- "; }
    }
    teamKv["tf_agree"] = std::to_string(a); teamKv["tf_against"] = std::to_string(d); teamKv["tf_who"] = who.empty() ? "-" : who.substr(0, who.size() - 1);
}

}  // namespace aptf
#endif
