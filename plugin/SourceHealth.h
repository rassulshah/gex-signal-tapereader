#ifndef LS_SOURCEHEALTH_H
#define LS_SOURCEHEALTH_H
// SourceHealth.h 0.1.0 (2026-10-26) -- awareness_plus item 5, GRACEFUL FAILURE: a heartbeat / freshness verdict per source per market
// so a stale, crashed or missing source is a TEAM FACT (not a silent hole): excluded from reads, its links fall back to isolated,
// the nightly team-health judge scores it and the morning summary reports it. SDK-free, unit-tested on Linux.
//
//   state     rule (per source; numbers from the market profile SRC rows, defaults below)
//   ok        entry decodes, written <= freshS ago, its bar is within 2 bars of the chart's last closed bar
//   lagging   fresh writes but its bar is more than 2 bars behind (calc stuck / a chart not updating)
//   stale     the entry is there but written > freshS ago (the indicator stopped, crashed or was removed)
//   gone      seen earlier this session, now empty (cleared on removal)
//   missing   the market file says this source is EXPECTED on this chart and it has never posted
//   off       not expected and never seen (not a problem; not listed)
// Recording (outside IRT): READER = the IRT Reader heartbeat file (age, ERROR text), MBO = the order-by-order recording flag the
// hourly health job writes (on / off / n/a / ?). A recording problem is posted the same way (team fact, morning summary).
#include <map>
#include <set>
#include <string>
#include <vector>
#include "MarketProfile.h"

namespace aph {

static const char* const VERSION = "0.1.0";

struct Seen { bool present = false; long long ms = 0, bar = 0; };   // raw board entry (decoded WITHOUT the 60 s freshness cut)

struct Verdict { std::string state = "off", why; long long ageS = -1, barsBehind = -1; };

inline Verdict judge(const Seen& s, bool everSeen, bool expected, long long nowMs, long long t, long spb, int freshS) {
    Verdict V;
    if (!s.present) {
        if (everSeen) { V.state = "gone"; V.why = "posted earlier this session, now empty"; }
        else if (expected) { V.state = "missing"; V.why = "expected on this market, never posted"; }
        return V;
    }
    V.ageS = nowMs >= s.ms ? (nowMs - s.ms) / 1000 : 0;
    V.barsBehind = (spb > 0 && t > 0 && s.bar > 0 && t > s.bar) ? (t - s.bar) / spb : 0;
    if (V.ageS > freshS) { V.state = "stale"; V.why = "last write " + std::to_string(V.ageS) + " s ago"; }
    else if (V.barsBehind > 2) { V.state = "lagging"; V.why = std::to_string(V.barsBehind) + " bars behind"; }
    else V.state = "ok";
    return V;
}

inline bool bad(const std::string& st) { return st == "stale" || st == "gone" || st == "missing" || st == "lagging"; }

struct Monitor {
    std::set<std::string> everSeen;
    std::map<std::string, Verdict> last;
    std::map<std::string, std::string> kv;           // HEALTH facts
    std::set<std::string> excluded;                  // sources a read must not use now (their links fall back to isolated)
    int changes = 0;                                 // state changes (for the record line)
    std::vector<std::string> changeLines;            // "SRC old->new why" since the last call

    // seen: every source this chart could have (board + AW teammates); reader / mbo: recording states ("ok" / "silent" / "?" ...)
    void update(const apm::Profile& P, const std::map<std::string, Seen>& seen, long long nowMs, long long t, long spb,
                const std::string& reader, const std::string& mbo) {
        changeLines.clear(); excluded.clear();
        std::set<std::string> names; for (const auto& kv0 : seen) names.insert(kv0.first); for (const auto& r : P.src) names.insert(r.first);
        std::string bads, oks;
        int nOk = 0, nBad = 0;
        for (const auto& n : names) {
            auto it = seen.find(n); const Seen s = it == seen.end() ? Seen() : it->second;
            auto ru = P.src.find(n); const bool expected = ru != P.src.end() && ru->second.expected;
            const int freshS = ru != P.src.end() ? ru->second.freshS : 60;
            if (s.present) everSeen.insert(n);
            const Verdict V = judge(s, everSeen.count(n) > 0, expected, nowMs, t, spb, freshS);
            auto old = last.find(n);
            const std::string was = old == last.end() ? "off" : old->second.state;
            if (was != V.state) { ++changes; changeLines.push_back(n + " " + was + "->" + V.state + (V.why.empty() ? "" : " (" + V.why + ")")); }
            last[n] = V;
            if (V.state == "off") continue;
            if (bad(V.state)) { ++nBad; excluded.insert(n); if (!bads.empty()) bads += ","; bads += n + ":" + V.state; }
            else { ++nOk; if (!oks.empty()) oks += ","; oks += n; }
        }
        kv.clear();
        kv["class"] = "METRIC"; kv["ok"] = oks.empty() ? "-" : oks; kv["bad"] = bads.empty() ? "-" : bads;
        kv["n_ok"] = std::to_string(nOk); kv["n_bad"] = std::to_string(nBad);
        kv["reader"] = reader.empty() ? "?" : reader; kv["mbo"] = mbo.empty() ? "?" : mbo;
        const bool recBad = reader == "silent" || reader == "error" || mbo == "off";
        kv["state"] = (nBad == 0 && !recBad) ? "ok" : "degraded";
        std::string ex; for (const auto& e : excluded) { if (!ex.empty()) ex += ","; ex += e; }
        kv["excluded"] = ex.empty() ? "-" : ex;
    }
    bool usable(const std::string& src) const { return excluded.count(src) == 0; }
};

// IRT Reader heartbeat (lsFlexLevels\IRTReader.status-<MKT>.txt): ok when < 180 s old and no ERROR text; ? when there is no file
inline std::string readerState(bool fileThere, long long ageS, const std::string& text) {
    if (!fileThere) return "?";
    if (text.find("ERROR") != std::string::npos) return "error";
    return ageS >= 0 && ageS < 180 ? "ok" : "silent";
}
// MBO recording flag file (lsFlexLevels\LRA-Recording.csv, written by the hourly health job):  REC,<MKT>,<fp state>,<mbo state>,<epoch s>
// older than 2 h -> '?' (the job is not running: say unknown, never 'on')
inline std::string mboState(const std::string& text, const std::string& market, long long nowEpochS) {
    size_t a = 0;
    while (a < text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        const auto f = apm::splitCsv(text.substr(a, e - a)); a = e + 1;
        if (f.size() >= 5 && f[0] == "REC" && f[1] == market) {
            double ts = 0; if (!apm::toD(f[4], ts)) return "?";
            if (nowEpochS - (long long)ts > 7200 || (long long)ts - nowEpochS > 300) return "?";
            return f[3].empty() ? "?" : f[3];
        }
    }
    return "?";
}

}  // namespace aph
#endif
