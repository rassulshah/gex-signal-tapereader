#ifndef LS_AWAREPLUS_H
#define LS_AWAREPLUS_H
// AwarePlus.h 0.1.0 (2026-10-26) -- the awareness_plus glue lsContext calls (items 1 expectations, 4 news / shock, 5 graceful failure,
// 10 per-market personality). SDK-free, unit-tested on Linux with HB_FAKE. Ships OFF: it posts facts + record lines and folds METRIC
// fields into CTX; nothing drawn changes, and the only effect on TEAM (internal, no box) is: excluded sources' reads are dropped and
// TEAM carries the news multiplier conf_x + news flag (both recorded so the nightly can score them). Turned off entirely with the
// market params file line  PLUS,0  (LRA-Context-<MKT>.csv; default ON for recording since nothing visible changes).
//
//   per closed bar t, in the pump:
//     preCtx(t)   health of every source (raw board entries, no 60 s cut) -> HEALTH facts, excluded set
//                 news window + shock for bar t                             -> NEWS facts, conf multiplier, news flag
//                 open expectations checked against bar t (registered at bars < t only)  -> EXP facts, EXP| record lines
//     foldCtx     adds M.news / M.news_window / M.news_event / M.shock / M.conf_x / M.src_bad / M.excluded / M.reader / M.mbo /
//                 M.exp_open / M.exp_last + D.mkt_* personality numbers to CTX(t) (after the class tagging)
//     postTeam(t) every AW read decided for t that implies an expectation registers one (K / N per market x read x context)
//   board sources posted:  EXP, NEWS, HEALTH  (MarketState entries, re-posted every 10 s with CTX)
//   record lines (Context-<MKT>-<spb>s-<day>.rec, same file as CTX / TEAM):  EXP|1|..., NEWS|1|<t>|kv (on change), HEALTH|1|<t>|kv (on change)
#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "MarketState.h"
#include "ContextLogic.h"
#include "MarketProfile.h"
#include "Expectations.h"
#include "NewsGate.h"
#include "SourceHealth.h"
#include "Memory.h"
#include "CrossTf.h"

namespace ap {

static const char* const VERSION = "0.1.0";

// every source the health monitor looks at on a chart (board facts + the teammates' aware reads)
static const char* const WATCH[] = { "AGG", "BOOK", "LOTS", "DP", "LIQ", "GEX", "HODLOD", "COB", "VWAP", "TREND", "READ", "STAGE", "TREND1H",
                                     "AWSTAGE", "AWDP", "AWAGG", "AWHODLOD", nullptr };

inline void civil(long long days, int& y, int& m, int& d) {   // Howard Hinnant's civil_from_days
    days += 719468; const long long era = (days >= 0 ? days : days - 146096) / 146097; const unsigned doe = (unsigned)(days - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const long long yy = (long long)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); const unsigned mp_ = (5 * doy + 2) / 153;
    d = (int)(doy - (153 * mp_ + 2) / 5 + 1); m = (int)(mp_ < 10 ? mp_ + 3 : mp_ - 9); y = (int)(yy + (m <= 2));
}
inline apn::Clock clockOf(long long t) {
    apn::Clock c; long long days = t / 86400; long long sod = t % 86400; if (sod < 0) { sod += 86400; --days; }
    civil(days, c.y, c.mo, c.d); c.wd = apn::weekday(c.y, c.mo, c.d); c.minOfDay = (int)(sod / 60); return c;
}

// value of a tagged CTX field (R. values are value~conf~src -> value); "?" when absent
inline std::string tagged(const std::map<std::string, std::string>& kv, const std::string& k) {
    for (const char* p : { "D.", "M.", "R." }) { auto it = kv.find(std::string(p) + k); if (it != kv.end()) { const size_t q = it->second.find('~'); return q == std::string::npos ? it->second : it->second.substr(0, q); } }
    auto it = kv.find(k); return it == kv.end() ? std::string("?") : it->second;
}

// what an aware read expects. Built in for the first four teammates; any teammate may declare its own with
// exp_kind (leave / no_new_ext / hold_above / hold_below), exp_read, exp_ref, exp_ext in its AW facts.
struct Want { bool ok = false; apx::Kind kind = apx::Kind::LEAVE; std::string read; double ref = 0, ext = 0; bool refSet = false, extSet = false; };
inline Want wantOf(const std::string& src, const mst::Facts& f) {
    Want W; const int side = (int)mst::getD(f, "side", 0); const std::string word = mst::getS(f, "word");
    const std::string k = mst::getS(f, "exp_kind");
    if (!k.empty()) {
        W.ok = true; W.read = mst::getS(f, "exp_read", src);
        W.kind = k == "no_new_ext" ? apx::Kind::NO_NEW_EXTREME : k == "hold_above" ? apx::Kind::HOLD_ABOVE : k == "hold_below" ? apx::Kind::HOLD_BELOW : apx::Kind::LEAVE;
        if (f.kv.count("exp_ref")) { W.ref = mst::getD(f, "exp_ref", 0); W.refSet = true; }
        if (f.kv.count("exp_ext")) { W.ext = mst::getD(f, "exp_ext", 0); W.extSet = true; }
        if ((W.kind == apx::Kind::HOLD_ABOVE || W.kind == apx::Kind::HOLD_BELOW) && !W.refSet) W.ok = false;   // a hold needs its price
        return W;
    }
    if (side == 0) return W;
    if (src == "STAGE" && word.rfind("stage ", 0) == 0) {
        const int st = std::atoi(word.c_str() + 6);
        W.ok = st >= 2; W.kind = st == 3 ? apx::Kind::NO_NEW_EXTREME : apx::Kind::LEAVE;
        W.read = st == 2 ? "collision" : st == 3 ? "stage3" : "confirm";   // stages: 1 Trap, 2 Collision, 3 Turn (holds = no new extreme), 4 = C confirm
    } else if (src == "DP") { W.ok = word.find("final") != std::string::npos || word == "final A"; W.read = "absorb"; }
    return W;
}

// graceful failure for LINKS: a link that would use teammate <src> must fall back to the isolated read while CTX(t) lists the source
// (or its AW read) in M.excluded. Every link rule calls this before using a teammate (links are all OFF today: nothing changes yet).
inline bool sourceExcluded(const std::map<std::string, std::string>& ctxKv, const std::string& src) {
    auto it = ctxKv.find("M.excluded"); if (it == ctxKv.end() || it->second.empty() || it->second == "-" || it->second == "?") return false;
    const std::string& v = it->second; size_t a = 0;
    while (a <= v.size()) { size_t e = v.find(',', a); if (e == std::string::npos) e = v.size(); const std::string x = v.substr(a, e - a); if (x == src || x == "AW" + src) return true; a = e + 1; }
    return false;
}

// the market params file (LRA-Context-<MKT>.csv) may carry  PLUS,0  to switch awareness_plus off for that market (default on:
// it only posts facts and records; nothing drawn changes)
inline bool plusOn(const std::string& paramsText) {
    size_t a = 0;
    while (a < paramsText.size()) {
        size_t e = paramsText.find('\n', a); if (e == std::string::npos) e = paramsText.size();
        const auto f = apm::splitCsv(paramsText.substr(a, e - a)); a = e + 1;
        if (f.size() >= 2 && f[0] == "PLUS") return f[1] != "0";
    }
    return true;
}

struct Plus {
    bool enabled = true;
    std::string market; double tick = 0.25;
    apm::Profile prof;
    apn::Gate gate; aph::Monitor mon; apx::Book book;
    std::string reader = "?", mbo = "?";               // recording states (the shell reads the heartbeat / flag files)
    apmem::Mem mem; std::string memText; bool memOnce = false;   // NEXT 2: memory beyond today (LRA-Memory-<MKT>.csv)
    std::map<std::string, std::string> tfSummary; long long tfBar = -1;   // NEXT 6: this chart's summary for the other timeframes
    long long ctxT = -1, teamT = -1, boardMs = -1, tfMs = -1;
    std::map<std::string, std::string> expKv, lastNewsKv, lastHealthKv;
    std::vector<std::string> lines;                    // record lines produced since the caller last took them

    bool loadedOnce = false; std::string srcText;
    // the shell re-reads the file every 30 s; parse only when it changed (or the market did)
    void loadIfChanged(const std::string& mkt, const std::string& profileText, double tk) {
        if (loadedOnce && mkt == market && profileText == srcText) { if (tk > 0) tick = tk; return; }
        load(mkt, profileText, tk); srcText = profileText; loadedOnce = true;
    }
    void loadMemoryIfChanged(const std::string& text) {
        if (memOnce && text == memText && mem.market == market) return;
        mem = apmem::parse(text, market); memText = text; memOnce = true;
    }
    void load(const std::string& mkt, const std::string& profileText, double tk) {
        market = mkt; tick = tk > 0 ? tk : 0.25;
        prof = profileText.empty() ? apm::defaults(mkt) : apm::parse(profileText, mkt);
        if (prof.has("tick") && prof.get("tick", 0) > 0 && !(tk > 0)) tick = prof.get("tick", tick);
    }

    // the raw board entries (no freshness cut) for the health judge
    static std::map<std::string, aph::Seen> scan(const std::string& sym, long spb, const std::string& chart) {
        std::map<std::string, aph::Seen> out;
        for (const char* const* s = WATCH; *s; ++s) {
            std::string v; mst::Facts F;
            aph::Seen x; if (bb::bbGet(mst::keyOf(*s, sym, spb, chart), v) && mst::decode(v, -1, F)) { x.present = true; x.ms = F.ms; x.bar = F.bar; }
            out[*s] = x;
        }
        return out;
    }

    // 1. before CTX(t) is built. bars: the chart's closed bars (oldest first, last = t). Returns the sources to leave out of CTX(t).
    std::set<std::string> preCtx(const std::string& sym, long spb, const std::string& chart, long long nowMs, const std::vector<ctx::Bar>& bars) {
        if (!enabled || bars.empty()) return {};
        const ctx::Bar& b = bars.back(); const long long t = b.t;
        if (t == ctxT) return mon.excluded;
        ctxT = t;
        mon.update(prof, scan(sym, spb, chart), nowMs, t, spb, reader, mbo);
        if (mon.kv != lastHealthKv) { lines.push_back("HEALTH|1|" + std::to_string(t) + "|" + kvLine(mon.kv) + (mon.changeLines.empty() ? "" : ";changes=" + join(mon.changeLines, " / "))); lastHealthKv = mon.kv; }
        const apn::Clock c = clockOf(t);
        gate.onBar(prof, c, t, tick > 0 ? (b.h - b.l) / tick : -1, b.v > 0 ? b.v : -1, spb);
        { auto a = gate.kv, z = lastNewsKv; a.erase("rng_x"); a.erase("vol_x"); a.erase("mins_to"); a.erase("mins_since"); z.erase("rng_x"); z.erase("vol_x"); z.erase("mins_to"); z.erase("mins_since");
          if (a != z) lines.push_back("NEWS|1|" + std::to_string(t) + "|" + kvLine(gate.kv));
          lastNewsKv = gate.kv; }
        apx::Bar eb; eb.t = t; eb.h = b.h; eb.l = b.l; eb.c = b.c;
        for (const apx::Exp& E : book.onBar(eb)) lines.push_back(apx::recordLine(E));
        expKv = book.kv();
        return mon.excluded;
    }

    // 2. after tagging CTX(t): fold the METRIC facts in (never READ: these are measured, not opinions)
    void foldCtx(std::map<std::string, std::string>& kv) const {
        if (!enabled) return;
        auto g = [&](const std::map<std::string, std::string>& m, const char* k) { auto it = m.find(k); return it == m.end() ? std::string("?") : it->second; };
        kv["M.news"] = g(gate.kv, "news"); kv["M.news_window"] = g(gate.kv, "window"); kv["M.news_event"] = g(gate.kv, "event");
        kv["M.shock"] = g(gate.kv, "shock"); kv["M.shock_hot"] = g(gate.kv, "shock_hot"); kv["M.conf_x"] = g(gate.kv, "conf_x");
        kv["M.src_state"] = g(mon.kv, "state"); kv["M.src_bad"] = g(mon.kv, "bad"); kv["M.excluded"] = g(mon.kv, "excluded");
        kv["M.reader"] = g(mon.kv, "reader"); kv["M.mbo"] = g(mon.kv, "mbo");
        kv["M.exp_open"] = g(expKv, "exp_open"); kv["M.exp_last"] = g(expKv, "last");
        kv["D.mkt_profile"] = prof.loaded ? "file" : "defaults";
        for (const char* k : { "aggr_p90", "aggr_p99", "shown_med", "implied_share", "refill_share", "lon_vs_rth_vol", "atr3_med" })
            kv[std::string("D.mkt_") + k] = prof.has(k) ? mst::num(prof.get(k, 0), 4) : std::string("?");
    }

    // 2b. memory beyond today + the other timeframes (NEXT 2 + 6), also after tagging; data <= t only
    void foldNext(std::map<std::string, std::string>& kv, const std::string& sym, long spb, long long nowMs, const std::vector<ctx::Bar>& bars, int rthOpenMin) const {
        if (!enabled || bars.empty()) return;
        const long long t = bars.back().t;
        apn::Clock c = clockOf(t);
        long days = apmem::daysFrom(apn::dateStr(c));
        if (c.minOfDay >= 17 * 60) { c = clockOf(t + 86400); days += 1; }        // the Globex session date (17:00 opens the next day)
        double atr = 0; { const std::string a = tagged(kv, "atr"); if (a != "?") atr = std::atof(a.c_str()); }
        apmem::fold(kv, mem, bars, days, tick, atr, rthOpenMin, apn::dateStr(c));
        kv["M.mem_file"] = mem.loaded ? "file" : "none";
        aptf::fold(kv, sym, spb, nowMs, t);
    }

    // the context cell of a read for its K / N: stretched / with_trend / all (the seed cells; the nightly may add more)
    static std::string cellOf(const std::map<std::string, std::string>& ctxKv, int side) {
        if (tagged(ctxKv, "stretched") == "1") return "stretched";
        const std::string tr = tagged(ctxKv, "trend_3m");
        if ((tr == "1" && side > 0) || (tr == "-1" && side < 0)) return "with_trend";
        return "all";
    }

    // 3. once TEAM(t) is decided: register what each aware read decided for t expects. ctxKv = the tagged CTX(t).
    void postTeam(const std::string& sym, long spb, const std::string& chart, long long nowMs, const std::vector<ctx::Bar>& bars,
                  const std::map<std::string, std::string>& ctxKv) {
        if (!enabled || bars.empty()) return;
        const long long t = bars.back().t; if (t == teamT) return; teamT = t;
        double atr = 0; { const std::string a = tagged(ctxKv, "atr"); if (a != "?") atr = std::atof(a.c_str()); }
        // the read's extreme: the lowest low / highest high of the last 3 closed bars (t included; nothing after t)
        double lo = bars.back().l, hi = bars.back().h;
        for (size_t i = bars.size() >= 3 ? bars.size() - 3 : 0; i < bars.size(); ++i) { lo = std::min(lo, bars[i].l); hi = std::max(hi, bars[i].h); }
        const bool news = gate.news;
        for (const char* const* s = WATCH; *s; ++s) {
            const std::string name = *s; if (name.rfind("AW", 0) != 0) continue;
            const std::string src = name.substr(2);
            if (!mon.usable(name) || !mon.usable(src)) continue;          // a stale source's read registers nothing
            const mst::Facts f = mst::read(name, sym, spb, chart, nowMs);
            if (!f.ok || f.bar != t) continue;
            const Want W = wantOf(src, f); if (!W.ok) continue;
            const int side = (int)mst::getD(f, "side", 0);
            const std::string cell = cellOf(ctxKv, side);
            const apm::KN kn = apm::knFor(prof, W.read, cell);
            apx::Exp E;
            const double ref = W.refSet ? W.ref : bars.back().c;
            const double ext = W.extSet ? W.ext : (side > 0 ? lo : hi);
            if (apx::make(E, src, W.read, cell, W.kind, side, t, ref, ext, atr, kn.K, kn.N, news) && book.add(E))
                lines.push_back("EXPREG|1|" + src + "|" + W.read + "|" + cell + "|" + apx::kindName(W.kind) + "|" + std::to_string(side) + "|" + std::to_string(t) + "|" +
                                std::to_string(kn.K) + "|" + apx::fmt(kn.N) + "|" + apx::fmt(ref) + "|" + apx::fmt(ext) + "|" + apx::fmt(atr) + "|" + (news ? "1" : "0"));
        }
        expKv = book.kv();
    }

    // the TEAM read under the gate: excluded sources' reads were dropped by the caller; TEAM carries no conf of its own (side by
    // agreement / trust), so the news multiplier is posted beside it (conf_x) for every reader to apply, plus the news flag
    void foldTeam(std::map<std::string, std::string>& teamKv, const std::map<std::string, std::string>& ctxKv) {
        if (!enabled) return;
        aptf::agree(teamKv, ctxKv);                                    // NEXT 6: agreement with the other timeframes' team sides
        tfSummary = aptf::summary(ctxKv, teamKv); tfBar = ctxT;        // ... and this chart's summary for them (posted with the board facts)
        tfMs = -1;
        auto cx = gate.kv.find("conf_x");
        teamKv["conf_x"] = cx == gate.kv.end() ? "1.00" : cx->second;
        teamKv["news"] = gate.news ? "1" : "0";
        auto ex_ = mon.kv.find("excluded"); teamKv["excluded"] = ex_ == mon.kv.end() ? "-" : ex_->second;
    }

    // post EXP / NEWS / HEALTH on the board (every 10 s, with CTX)
    void publish(const std::string& sym, long spb, const std::string& chart, long long nowMs) {
        if (!enabled || ctxT < 0) return;
        if (boardMs >= 0 && nowMs >= boardMs && nowMs - boardMs < 10000) return;
        mst::publish("EXP", sym, spb, chart, nowMs, ctxT, expKv);
        mst::publish("NEWS", sym, spb, chart, nowMs, ctxT, gate.kv);
        mst::publish("HEALTH", sym, spb, chart, nowMs, ctxT, mon.kv);
        boardMs = nowMs;
    }
    // this chart's summary for the other timeframes: at once when TEAM(t) is decided, then every 10 s (fresh for the 60 s rule)
    void publishTf(const std::string& sym, long spb, long long nowMs) {
        if (!enabled || tfBar < 0) return;
        if (tfMs >= 0 && nowMs >= tfMs && nowMs - tfMs < 10000) return;
        aptf::publish(sym, spb, nowMs, tfBar, tfSummary); tfMs = nowMs;
    }
    void clear(const std::string& sym, long spb, const std::string& chart) {
        if (sym.empty()) return;
        mst::clear("EXP", sym, spb, chart); mst::clear("TF", sym, spb, "any"); mst::clear("NEWS", sym, spb, chart); mst::clear("HEALTH", sym, spb, chart);
    }
    std::vector<std::string> take() { std::vector<std::string> o; o.swap(lines); return o; }

    static std::string kvLine(const std::map<std::string, std::string>& kv) {
        std::string s; for (const auto& p : kv) { if (!s.empty()) s += ";"; s += p.first + "=" + mst::clean(p.second, 200); } return s;
    }
    static std::string join(const std::vector<std::string>& v, const char* sep) { std::string s; for (const auto& x : v) { if (!s.empty()) s += sep; s += x; } return mst::clean(s, 300); }
};

}  // namespace ap
#endif
