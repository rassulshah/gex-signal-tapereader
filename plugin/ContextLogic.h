#ifndef LS_CONTEXTLOGIC_H
#define LS_CONTEXTLOGIC_H
// ContextLogic.h 0.1.0 (2026-10-26) -- the SHARED CONTEXT (L2) and TEAM READ (L4) logic of lsContext, SDK-free (unit-tested on Linux).
// Rassul 2026-10-26 12:25-12:53 CT: "create a shared context ... make sure the indicators are aware of this context and ... aware of the
// others' read" / "build the architecture, its layers .. then build the team and the processes of communication". Design: TEAM-DESIGN.md.
//
// Per chart, per closed bar t:
//   1. every L1 detector posts its facts for t to MarketState (mst::publish, bar = t, decided once)
//   2. Collector decides when CTX(t) may be built: all PRESENT sources (heartbeat) have posted bar >= t, or the timeout passed
//      (then the late ones are listed). Facts with bar > t are NEVER used for CTX(t) (no look-ahead).
//   3. buildCtx() -> CTX(t) (key/value, UNKNOWN = "?", never 0), posted as source "CTX" (+ a symbol-only key for other charts)
//   4. indicators post AWARE.<src> for t (ContextRead.h); teamRead() -> TEAM(t)
// Everything decided for t is final; anything decided at L3/L4 for t can affect only t+1 of a lower layer.
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "MarketState.h"

namespace ctx {

static const char* const VERSION = "0.2.0";   // 0.2.0: vocabulary v2 classes DATA / METRIC / READ (Rassul 13:34)
static const char* const UNKNOWN = "?";

// ---------------------------------------------------------------- 1. collector: when may CTX(t) be built
struct Collector {
    long long bar = -1;            // the bar being collected
    long long openedMs = -1;       // when this bar was first seen closed (own clock)
    long long timeoutMs = 3000;
    void reset() { bar = -1; openedMs = -1; }
    // present = sources whose heartbeat says they are on this chart; posted = source -> the bar it last decided
    // returns true when CTX(t) can be built; late = present sources still not at t
    bool ready(long long t, long long nowMs, const std::set<std::string>& present, const std::map<std::string, long long>& posted,
               std::vector<std::string>& late) {
        if (t != bar) { bar = t; openedMs = nowMs; }
        late.clear();
        for (const auto& s : present) { auto it = posted.find(s); if (it == posted.end() || it->second < t) late.push_back(s); }
        if (late.empty()) return true;
        return nowMs - openedMs >= timeoutMs;
    }
};

// ---------------------------------------------------------------- 2. inputs
struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0, v = 0; int minOfDay = 0; };   // closed bars, oldest first
struct Level { double px = 0; std::string name, type; };                                       // type: key / volume / gamma / bigorder
struct Inputs {
    std::vector<Bar> bars;                          // closed bars up to and including t (more is ignored)
    std::map<std::string, mst::Facts> facts;        // source -> facts (any bar; only bar <= t is used)
    std::vector<Level> levels;
    double tick = 0.25;
    int rthOpenMin = 510;                           // 08:30 CT (HG 430)
    double atrTol = 0.25;                           // "at a level" = within atrTol x ATR
    std::set<std::string> present, late;            // data quality
};

inline double atrOf(const std::vector<Bar>& b, size_t upto, int n = 14) {
    if (upto == 0) return 0;
    double s = 0; int k = 0;
    for (size_t i = upto; i-- > 1 && k < n; ++k) { const double tr = std::max(b[i].h, b[i - 1].c) - std::min(b[i].l, b[i - 1].c); s += tr; }
    return k ? s / k : 0;
}
inline std::string n2(double x, int p = 4) { if (!(x == x)) return UNKNOWN; return mst::num(x, p); }

// legs from a simple ATR zigzag on closes (decided bars only)
struct Leg { int dir = 0; double from = 0, to = 0; size_t bars = 0; };
inline std::vector<Leg> legs(const std::vector<Bar>& b, size_t upto, double atr, double k = 1.5) {
    std::vector<Leg> L; if (upto < 2 || atr <= 0) return L;
    int dir = 0; double ext = b[0].c, start = b[0].c; size_t extI = 0, startI = 0;
    for (size_t i = 1; i <= upto && i < b.size(); ++i) {
        const double c = b[i].c;
        if (dir >= 0 && c > ext) { ext = c; extI = i; if (dir == 0 && c - start >= k * atr) dir = 1; }
        else if (dir <= 0 && c < ext) { ext = c; extI = i; if (dir == 0 && start - c >= k * atr) dir = -1; }
        if (dir == 1 && ext - c >= k * atr) { L.push_back({1, start, ext, extI - startI}); start = ext; startI = extI; dir = -1; ext = c; extI = i; }
        else if (dir == -1 && c - ext >= k * atr) { L.push_back({-1, start, ext, extI - startI}); start = ext; startI = extI; dir = 1; ext = c; extI = i; }
    }
    if (dir != 0) L.push_back({dir, start, ext, extI - startI});   // the open leg
    return L;
}

// ---------------------------------------------------------------- 3. CTX(t)
inline std::map<std::string, std::string> buildCtx(const Inputs& in, long long t) {
    std::map<std::string, std::string> kv;
    // decided bars <= t only
    size_t upto = 0; bool have = false;
    for (size_t i = 0; i < in.bars.size(); ++i) if (in.bars[i].t <= t) { upto = i; have = true; }
    kv["v"] = VERSION;
    if (!have) { kv["state"] = "no_bars"; return kv; }
    const Bar& B = in.bars[upto];
    const double atr = atrOf(in.bars, upto);
    // facts of bar <= t only (look-ahead guard)
    auto F = [&](const std::string& s) -> const mst::Facts* { auto it = in.facts.find(s); return (it != in.facts.end() && it->second.ok && it->second.bar <= t) ? &it->second : nullptr; };
    auto S = [&](const std::string& s, const std::string& k) { const mst::Facts* f = F(s); return f ? mst::getS(*f, k, UNKNOWN) : std::string(UNKNOWN); };

    // time
    kv["min_from_open"] = std::to_string(B.minOfDay - in.rthOpenMin);
    kv["slot30"] = std::to_string(B.minOfDay / 30 * 30);
    kv["session"] = B.minOfDay < 120 ? "asia" : B.minOfDay < in.rthOpenMin - 60 ? "london" : B.minOfDay < in.rthOpenMin ? "preopen"
                  : B.minOfDay < in.rthOpenMin + 60 ? "open" : B.minOfDay < 690 ? "am" : B.minOfDay < 780 ? "lunch" : B.minOfDay < 870 ? "pm" : "close";
    // session story (HODLOD source)
    for (const char* k : {"hod", "lod", "hod_time", "lod_time", "range_used", "first_extreme", "proj_hi", "proj_lo", "regime"}) kv[k] = S("HODLOD", k);
    // trend (TREND source; hourly UNKNOWN until an hourly chart posts)
    kv["trend_3m"] = S("TREND", "trend"); kv["trend_word"] = S("TREND", "trend_word"); kv["trend_1h"] = S("TREND1H", "trend");
    // VWAP
    kv["vwap_pos"] = S("VWAP", "pos"); kv["vwap_slope"] = S("VWAP", "slope");
    // regime
    kv["atr"] = atr > 0 ? n2(atr) : std::string(UNKNOWN);          // no ATR yet (too few bars) = unknown, never 0
    { double s = 0; int k = 0; for (size_t i = upto; i-- > 0 && k < 100; ++k) s += atrOf(in.bars, i); const double base = k ? s / k : 0;
      kv["atr_x"] = base > 0 ? n2(atr / base, 3) : UNKNOWN; }
    // wave
    const std::vector<Leg> L = legs(in.bars, upto, atr);
    if (!L.empty()) {
        const Leg& cur = L.back();
        std::vector<double> prior; for (size_t i = 0; i + 1 < L.size(); ++i) prior.push_back(std::fabs(L[i].to - L[i].from));
        double med = 0; if (!prior.empty()) { std::vector<double> p = prior; std::nth_element(p.begin(), p.begin() + p.size() / 2, p.end()); med = p[p.size() / 2]; }
        const double sz = std::fabs(cur.to - cur.from);
        kv["wave_dir"] = std::to_string(cur.dir); kv["leg_no"] = std::to_string(L.size());
        kv["leg_x"] = med > 0 ? n2(sz / med, 3) : UNKNOWN;
        // stretched = top third of prior legs
        if (prior.size() >= 3) { std::vector<double> p = prior; std::sort(p.begin(), p.end()); kv["stretched"] = sz >= p[p.size() * 2 / 3] ? "1" : "0"; }
        else kv["stretched"] = UNKNOWN;
        const double prog = sz > 0 ? std::fabs(B.c - cur.from) / sz : 0;
        kv["wave_pos"] = prog < 0.33 ? "start" : prog < 0.8 ? "mid" : "top";
    } else { kv["wave_dir"] = UNKNOWN; kv["leg_no"] = UNKNOWN; kv["leg_x"] = UNKNOWN; kv["stretched"] = UNKNOWN; kv["wave_pos"] = UNKNOWN; }
    // pivot forming: bar t is a 5-bar extreme (decided-bar rule; confirmation is a later bar)
    if (upto >= 4) {
        bool hi = true, lo = true; for (size_t i = upto - 4; i < upto; ++i) { hi = hi && B.h >= in.bars[i].h; lo = lo && B.l <= in.bars[i].l; }
        kv["pivot_forming"] = hi ? "high" : lo ? "low" : "none";
    } else kv["pivot_forming"] = UNKNOWN;
    // location
    std::string at = "none", atName; double best = 1e18, nextDist = 1e18;
    for (const Level& lv : in.levels) {
        const double d = std::fabs(B.c - lv.px);
        if (atr > 0 && d <= in.atrTol * atr) { if (d < best) { best = d; at = lv.type; atName = lv.name; } }
        else nextDist = std::min(nextDist, d);
    }
    int nAt = 0; for (const Level& lv : in.levels) if (atr > 0 && std::fabs(B.c - lv.px) <= in.atrTol * atr) ++nAt;
    kv["at_level"] = nAt >= 2 ? "confluence" : at; kv["level_name"] = atName.empty() ? "-" : mst::clean(atName, 24);
    kv["dist_next_ticks"] = (nextDist < 1e17 && in.tick > 0) ? std::to_string((long long)std::llround(nextDist / in.tick)) : UNKNOWN;
    // order-book events of bar t (AGG / BOOK / LIQ)
    kv["sweep"] = S("AGG", "sweep"); kv["spike"] = S("AGG", "spike"); kv["big_aggr"] = S("AGG", "big_aggr"); kv["pace_x"] = S("AGG", "pace_x");
    kv["book_spike"] = S("BOOK", "spike"); kv["pull_x"] = S("BOOK", "pullx"); kv["rebuilt_pct"] = S("BOOK", "rebuilt_pct");
    kv["zone_side"] = S("LIQ", "zone_side"); kv["zone_x"] = S("LIQ", "zone_x"); kv["absorb_lots"] = S("LIQ", "absorb_lots");
    kv["dp_a"] = S("DP", "a"); kv["dp_a_side"] = S("DP", "a_side");
    // data quality
    std::string pres, lat; for (const auto& s : in.present) pres += s + ","; for (const auto& s : in.late) lat += s + ",";
    kv["present"] = pres.empty() ? "-" : pres.substr(0, pres.size() - 1);
    kv["late"] = lat.empty() ? "-" : lat.substr(0, lat.size() - 1);
    kv["state"] = in.late.empty() ? "ok" : "partial";
    return kv;
}

// ---------------------------------------------------------------- 3b. VOCABULARY v2: every posted fact has a CLASS (Rassul 13:34)
// DATA   (D) raw, uninterpreted facts: lots at a price, order count, delta value, volume, price, time, big-lot size, levels swept
// METRIC (M) factual computed measures WITH their formula / normal: x vs same-slot normal, ATR, range used %, pace x
// READ   (R) interpretations (A held, stage, absorption, trapped, trend, team side) - posted with a confidence and the source,
//            used only as clearly labelled OPINIONS, never presented as facts (ContextRead.h field() refuses them)
// CTX keys are posted as "<class>.<key>"; a READ value is "<value>~<conf or ?>~<source>". Meta keys (v, cls_v, state, present,
// late) carry no class. A key missing from the table is posted as a READ (never as a fact).
static const char* const CLS_VERSION = "2";
struct FieldDef { const char* key; char cls; const char* src; const char* formula; };
static const FieldDef FIELDS[] = {
    { "bar_h", 'D', "bars", "high of bar t" }, { "bar_l", 'D', "bars", "low of bar t" }, { "bar_c", 'D', "bars", "close of bar t" },
    { "min_from_open", 'D', "bars", "minutes from the RTH open (bar end stamp)" }, { "slot30", 'D', "bars", "30-min slot of the bar end, minute of day" },
    { "hod", 'D', "HODLOD", "session high" }, { "lod", 'D', "HODLOD", "session low" }, { "hod_time", 'D', "HODLOD", "time of the HOD" }, { "lod_time", 'D', "HODLOD", "time of the LOD" },
    { "sweep", 'D', "AGG", "price levels one order swept" }, { "big_aggr", 'D', "AGG", "biggest aggressive lot on the bar" },
    { "zone_side", 'D', "LIQ", "side of the resting-order zone at price" }, { "absorb_lots", 'D', "LIQ", "lots filled at the zone" },
    { "session", 'M', "lsContext", "time bucket of the bar end: asia / london / preopen / open (+60) / am / lunch (11:30-13:00) / pm / close (14:30+)" },
    { "range_used", 'M', "HODLOD", "(HOD - LOD) / expected day range" },
    { "vwap_pos", 'M', "VWAP", "close vs session VWAP SD bands" }, { "vwap_slope", 'M', "VWAP", "VWAP change over 5 bars in SD" },
    { "atr", 'M', "lsContext", "ATR(14) of closed bars" }, { "atr_x", 'M', "lsContext", "ATR / mean ATR of the prior 100 bars" },
    { "wave_dir", 'M', "lsContext", "direction of the current ATR zigzag leg (k 1.5 ATR)" }, { "leg_no", 'M', "lsContext", "legs since the RTH open" },
    { "leg_x", 'M', "lsContext", "current leg size / median prior leg" }, { "stretched", 'M', "lsContext", "leg_x in the top third of prior legs" },
    { "wave_pos", 'M', "lsContext", "close within the current leg: start / mid / top thirds" },
    { "pivot_forming", 'M', "lsContext", "bar t is the high / low of the last 5 bars" },
    { "at_level", 'M', "lsContext", "within 0.25 ATR of a level (type; confluence = 2+)" }, { "level_name", 'M', "lsContext", "the nearest level within 0.25 ATR" },
    { "dist_next_ticks", 'M', "lsContext", "ticks to the next level" },
    { "spike", 'M', "AGG", "aggressor volume x same-slot normal" }, { "pace_x", 'M', "AGG", "trades per second x normal" },
    { "book_spike", 'M', "BOOK", "book change x normal" }, { "pull_x", 'M', "BOOK", "pulled size x normal" }, { "rebuilt_pct", 'M', "BOOK", "% of pulled size rebuilt" },
    { "zone_x", 'M', "LIQ", "zone lots x the market's floor" },
    { "first_extreme", 'R', "HODLOD", "" }, { "proj_hi", 'R', "HODLOD", "" }, { "proj_lo", 'R', "HODLOD", "" }, { "regime", 'R', "HODLOD", "" },
    { "trend_3m", 'R', "TREND", "" }, { "trend_word", 'R', "TREND", "" }, { "trend_1h", 'R', "TREND1H", "" },
    { "dp_a", 'R', "DP", "" }, { "dp_a_side", 'R', "DP", "" },
    { nullptr, 0, nullptr, nullptr } };
inline bool isMeta(const std::string& k) { return k == "v" || k == "cls_v" || k == "state" || k == "present" || k == "late"; }
inline const FieldDef* fieldDef(const std::string& k) { for (const FieldDef* f = FIELDS; f->key; ++f) if (k == f->key) return f; return nullptr; }
inline char classOf(const std::string& k) { const FieldDef* f = fieldDef(k); return f ? f->cls : 'R'; }
// plain CTX (buildCtx) -> the posted, class-tagged CTX. conf: source -> confidence of its READs ("?" when the source gives none)
inline std::map<std::string, std::string> tagCtx(const std::map<std::string, std::string>& kv, const std::map<std::string, std::string>& conf = {}) {
    std::map<std::string, std::string> out; out["cls_v"] = CLS_VERSION;
    for (const auto& p : kv) {
        if (isMeta(p.first)) { out[p.first] = p.second; continue; }
        const FieldDef* f = fieldDef(p.first); const char c = f ? f->cls : 'R';
        std::string v = p.second;
        if (c == 'R' && v != UNKNOWN) { const std::string src = f ? f->src : "?"; auto it = conf.find(src); v += "~" + (it == conf.end() ? std::string(UNKNOWN) : it->second) + "~" + src; }
        out[std::string(1, c) + "." + p.first] = v;
    }
    return out;
}

// ---------------------------------------------------------------- 4. TEAM(t) from the AWARE reads
struct AwareRead { std::string src; int side = 0; int conf = 0; long long bar = -1; };
// trust: source -> weight for this market x context cell (learned nightly; empty = no trust yet -> conflicts take no side)
inline std::map<std::string, std::string> teamRead(long long t, const std::vector<AwareRead>& reads, const std::map<std::string, double>& trust) {
    std::map<std::string, std::string> kv; std::string agreeUp, agreeDn; int nUp = 0, nDn = 0; double wUp = 0, wDn = 0;
    for (const AwareRead& r : reads) {
        if (r.bar != t || r.side == 0) continue;                 // only reads decided for t
        const auto it = trust.find(r.src); const double w = it == trust.end() ? 0 : it->second;
        if (r.side > 0) { ++nUp; wUp += w; agreeUp += r.src + ","; } else { ++nDn; wDn += w; agreeDn += r.src + ","; }
    }
    const bool conflict = nUp > 0 && nDn > 0;
    int side = 0;
    if (!conflict) side = nUp > 0 ? 1 : nDn > 0 ? -1 : 0;
    else if (wUp + wDn > 0 && std::fabs(wUp - wDn) > 0.1 * (wUp + wDn)) side = wUp > wDn ? 1 : -1;   // trust decides, else no side
    kv["v"] = VERSION; kv["side"] = std::to_string(side); kv["conflict"] = conflict ? "1" : "0";
    kv["up"] = agreeUp.empty() ? "-" : agreeUp.substr(0, agreeUp.size() - 1);
    kv["down"] = agreeDn.empty() ? "-" : agreeDn.substr(0, agreeDn.size() - 1);
    kv["n"] = std::to_string(nUp + nDn);
    return kv;
}

}  // namespace ctx
#endif
