#ifndef LS_ZONE_RULE_H
#define LS_ZONE_RULE_H
// ZoneRule.h (1.0.0, 2026-10-14 night) -- SDK-free. Rassul's ZONE rule for levels, ported natively from the level-confluence study
// (lra.level_confluence LCF v1.1 zone_outcome / zone_entries, docs\study-methods\level-confluence.md), shared by lsLiquidityProfile /
// lsLiquidityFlow (the Held / Broke labels) and the order-flow panes' read lines (ZoneRead.h: which zone price is in).
//   Rassul 2026-10-14 15:56: "7750 was the key. You see it as a break based on touches, I see it as a support where price reversed".
//   Levels are ZONES price trades into, then reverses or continues; a first touch or a tick through is NEVER a break.
//   ZONE   level (or a liquidity zone's centre) +/- W x ADR, W = 0.06 (the study's PRIMARY width)
//   ADR    the daily TRUE range of the full 17:00 CT session, mean of the prior 20 sessions (min 10), one day back (lra.trade_def):
//          TR = max(H - L, |H - prevC|, |L - prevC|), from the chart's OWN bars (no outside data), frozen for the session
//   REV    (held / reversed) the first close >= X = 0.10 ADR beyond the NEAR edge (back out on the side price came from)
//   CONT   (break) a close >= X beyond the FAR edge with NO close back inside the zone (or beyond it toward the entry side) within
//          the reclaim window (5 x 3-min bars = 15 min); a close back inside = a FAILED break (counted), the zone stays open
//   PIN    neither within the horizon (40 x 3-min bars = 2 h): in zone / pinned, no label
//   CENSORED a bar gap > 6 min inside the window (feed gap): never scored
// No repaint: closed bars only; an outcome, once final, never changes; a CONT is known only when its reclaim window has passed.
#include <cmath>
#include <cstdlib>
#include <cerrno>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

namespace zrule {

static const char* const VERSION = "1.0.0";
static const double ZONE_W = 0.06, RES_X = 0.10;      // the study's PRIMARY_W and RES_X (ADR units)
static const int ADR_DAYS = 20, ADR_MIN = 10;         // rolling(20, min_periods=10), shift(1)
static const long long ACCEPT_SEC = 900, HORIZON_SEC = 7200, GAP_SEC = 360;   // 5 / 40 bars of 3 min, gap > 6 min

inline bool finite(double v) { return v == v && v < 1e300 && v > -1e300; }

// ---- the lagged ADR from the chart's own bars ---------------------------------------------------------------------------------
// add() every bar in time order with its 17:00-CT session key; adrFor(session) = the mean true range of up to ADR_DAYS COMPLETE
// sessions before it (min ADR_MIN). The current session is never used (look-ahead free).
class Adr {
public:
    struct Day { long long key = 0; double h = 0, l = 0, c = 0; bool any = false; };
    std::vector<Day> days;                              // ascending session keys
    void clear() { days.clear(); }
    void add(long long key, double h, double l, double c) {
        if (!finite(h) || !finite(l) || !finite(c) || h < l || h <= 0 || l <= 0) return;
        if (days.empty() || key > days.back().key) { Day d; d.key = key; d.h = h; d.l = l; d.c = c; d.any = true; days.push_back(d); }
        else if (key == days.back().key) { Day& d = days.back(); d.h = std::max(d.h, h); d.l = std::min(d.l, l); d.c = c; }
        // an older key (out of order) is ignored: bars come in time order
        if (days.size() > 400) days.erase(days.begin());
    }
    // nUsed = sessions averaged; minDays = ADR_MIN (study) - a caller may pass a smaller floor and must label it
    double adrFor(long long session, int& nUsed, int minDays = ADR_MIN) const {
        nUsed = 0;
        std::vector<double> tr;
        for (size_t i = 0; i < days.size(); ++i) {
            if (days[i].key >= session) break;
            if (i == 0) continue;                           // no previous close: NaN in the study (np.maximum), not counted
            const double pc = days[i - 1].c;                // prevC = the previous session's close (shift on the session table)
            tr.push_back(std::max(days[i].h - days[i].l, std::max(std::fabs(days[i].h - pc), std::fabs(days[i].l - pc))));
        }
        if ((int)tr.size() < std::max(1, minDays)) return -1;
        const size_t n = std::min(tr.size(), (size_t)ADR_DAYS);
        double s = 0; for (size_t k = tr.size() - n; k < tr.size(); ++k) s += tr[k];
        nUsed = (int)n; return s / (double)n;
    }
};

// the 17:00-CT session key of a local CT clock (seconds since the epoch on the local wall clock): 17:00 starts the next day
inline long long sessionKey(long long ctSec) { const long long a = ctSec - 17LL * 3600; return (a >= 0 ? a / 86400LL : -((-a + 86399) / 86400LL)) + 1; }

// ---- the zone outcome, bar by bar (lra.level_confluence.zone_outcome, incremental) ------------------------------------------------
enum Res { PENDING = 0, REV = 1, CONT = 2, PIN = 3, CENSORED = 4 };
inline const char* resName(int r) { return r == REV ? "REV" : r == CONT ? "CONT" : r == PIN ? "PIN" : r == CENSORED ? "CENSORED" : "OPEN"; }
struct Rule { int accept = 5, horizon = 40; long long gapSec = GAP_SEC; };
// bar counts for a chart of spb seconds per bar: the 15-min reclaim window and the 2-h horizon in that chart's bars
inline Rule ruleFor(int spb) {
    Rule r; if (spb <= 0) return r;
    r.accept = (int)((ACCEPT_SEC + spb - 1) / spb); if (r.accept < 1) r.accept = 1;
    r.horizon = (int)((HORIZON_SEC + spb - 1) / spb); if (r.horizon < r.accept) r.horizon = r.accept;
    r.gapSec = std::max((long long)GAP_SEC, 2LL * spb);
    return r;
}
// support = price came DOWN into the zone (study side 'above'); resistance = came UP ('below')
struct Outcome {
    bool support = true; double lo = 0, hi = 0, x = 0; Rule rule;
    int res = PENDING, n = -1, kb = -1, failed = 0, nRes = -1;   // n = closes seen (0 = the arrival bar); kb = the accepted-break candidate
    long long lastEnd = 0;                                      // the last close's bar end (s), for the gap check
    bool resolved() const { return res != PENDING; }
    // one CLOSED bar (in order, starting with the arrival bar). Returns true when this close resolved it.
    bool onClose(double c, long long barEndSec) {
        if (res != PENDING || !finite(c)) return false;
        if (lastEnd > 0 && barEndSec > lastEnd && barEndSec - lastEnd > rule.gapSec) { res = CENSORED; nRes = n + 1; n++; lastEnd = barEndSec; return true; }
        lastEnd = barEndSec; n++;
        const bool up = !support;                               // came up into the zone = a resistance test
        const double nearE = up ? lo : hi, farE = up ? hi : lo;
        if (kb >= 0) {                                          // inside the reclaim window of a break candidate
            const bool back = up ? c <= hi : c >= lo;
            if (back) { failed++; kb = -1; if (n >= rule.horizon) { res = PIN; nRes = n; return true; } return false; }
            if (n - kb >= rule.accept) { res = CONT; nRes = n; return true; }   // known only now; the break bar was kb
            return false;
        }
        if (up ? c <= nearE - x : c >= nearE + x) { res = REV; nRes = n; return true; }
        if (up ? c >= farE + x : c <= farE - x) { kb = n; if (rule.accept <= 0) { res = CONT; nRes = n; return true; } return false; }
        if (n >= rule.horizon) { res = PIN; nRes = n; return true; }
        return false;
    }
    // the session ended (the study's i_end): a break candidate counts (no bar left to reclaim it), else pinned
    bool endSession() { if (res != PENDING) return false; res = kb >= 0 ? CONT : PIN; nRes = n; return true; }
};
inline Outcome makeOutcome(bool support, double centre, double adr, int spb) {
    Outcome o; o.support = support; o.rule = ruleFor(spb);
    const double w = ZONE_W * adr; o.lo = centre - w; o.hi = centre + w; o.x = RES_X * adr;
    return o;
}

// the reset (debounce) after a resolution: RESET_BARS consecutive closes at least the half-width beyond an edge before the same
// zone can start a new encounter (zone_entries)
struct Reset {
    double lo = 0, hi = 0, dist = 0; int run = 0; bool done = false;
    void onClose(double c) { if (done) return; const bool out = c >= hi + dist || c <= lo - dist; run = out ? run + 1 : 0; if (run >= 2) done = true; }
};

// ---- levels for the read lines: key (LRA-KeyLevels), MenthorQ (LRA-Levels), gamma (LRA-GexNodes) ----------------------------------
struct Level { std::string name; double px = 0; int kind = 0; };     // kind 0 key, 1 MenthorQ level, 2 gamma node
inline std::vector<std::string> splitBar(const std::string& s) {
    std::vector<std::string> f; size_t a = 0;
    for (size_t i = 0; i <= s.size(); ++i) if (i == s.size() || s[i] == '|') { f.push_back(s.substr(a, i - a)); a = i + 1; }
    return f;
}
inline bool numTok(const std::string& t, double& v) {
    if (t.empty() || t.size() > 40) return false;
    errno = 0; char* e = nullptr; v = std::strtod(t.c_str(), &e);
    return !errno && e == t.c_str() + t.size() && finite(v);
}
inline std::string cleanName(const std::string& s) { std::string o; for (char c : s) { if (o.size() >= 16) break; if ((unsigned char)c >= 32 && c != '|' && c != ',') o += c; } return o; }
inline std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> L; std::string cur;
    for (size_t i = 0; i < text.size() && L.size() < 4096; ++i) { const char c = text[i]; if (c == '\n') { if (!cur.empty() && cur.back() == '\r') cur.pop_back(); L.push_back(cur); cur.clear(); } else if (cur.size() < 4096) cur += c; }
    if (!cur.empty()) { if (cur.back() == '\r') cur.pop_back(); L.push_back(cur); }
    return L;
}
// LRA-KeyLevels-<MKT>.csv: "KEYLV,1,<unix>" then L|name|price|pivot; older than maxAgeSec = not used
inline bool parseKey(const std::string& text, long long nowUnix, long long maxAgeSec, std::vector<Level>& out) {
    out.clear(); const std::vector<std::string> L = lines(text);
    if (L.empty() || L[0].compare(0, 8, "KEYLV,1,") != 0) return false;
    char* e = nullptr; errno = 0; const long long at = std::strtoll(L[0].c_str() + 8, &e, 10);
    if (errno || !e || *e || at <= 0) return false;
    if (nowUnix > 0 && (nowUnix - at > maxAgeSec || at - nowUnix > 3600)) return true;   // stale: valid file, no levels
    for (size_t i = 1; i < L.size() && out.size() < 64; ++i) {
        const std::vector<std::string> f = splitBar(L[i]); double px = 0;
        if (f.size() < 3 || f[0] != "L" || !numTok(f[2], px) || px <= 0) continue;
        Level v; v.name = cleanName(f[1]); v.px = px; v.kind = 0; out.push_back(v);
    }
    return true;
}
// LRA-Levels-<MKT>.csv: LEVEL|code|name|price|kind|...; STATE|STALE = none; MARKET must match
inline bool parseMq(const std::string& text, const std::string& market, std::vector<Level>& out) {
    out.clear(); const std::vector<std::string> L = lines(text);
    bool mk = false, stale = false;
    for (size_t i = 0; i < L.size(); ++i) {
        const std::vector<std::string> f = splitBar(L[i]);
        if (f.size() >= 2 && f[0] == "MARKET") { if (f[1] != market) return false; mk = true; }
        else if (f.size() >= 2 && f[0] == "STATE" && f[1] == "STALE") stale = true;
    }
    if (!mk) return false;
    if (stale) return true;
    for (size_t i = 0; i < L.size() && out.size() < 64; ++i) {
        const std::vector<std::string> f = splitBar(L[i]); double px = 0;
        if (f.size() < 5 || f[0] != "LEVEL" || !numTok(f[3], px) || px <= 0) continue;
        if (f[4] == "vwap") continue;                               // VWAP bands move: not a fixed level zone
        Level v; v.name = cleanName(f[1]); v.px = px; v.kind = f[4] == "gamma" ? 2 : 1; out.push_back(v);
    }
    return true;
}
// LRA-GexNodes-<MKT>.csv (gex_nodes 2.0.0): VERSION|2, MARKET|<m>, ASOF|<unix>|..., STR|loc|strike|price|...|label|rank
inline bool parseGex(const std::string& text, const std::string& market, long long nowUnix, long long maxAgeSec, std::vector<Level>& out) {
    out.clear(); const std::vector<std::string> L = lines(text);
    bool mk = false; long long at = 0;
    for (size_t i = 0; i < L.size(); ++i) {
        const std::vector<std::string> f = splitBar(L[i]);
        if (f.size() >= 2 && f[0] == "MARKET") { if (f[1] != market) return false; mk = true; }
        else if (f.size() >= 2 && f[0] == "ASOF") { char* e = nullptr; errno = 0; at = std::strtoll(f[1].c_str(), &e, 10); if (errno || !e || *e) at = 0; }
    }
    if (!mk) return false;
    if (nowUnix > 0 && (at <= 0 || nowUnix - at > maxAgeSec || at - nowUnix > 3600)) return true;   // stale / no stamp: no nodes
    for (size_t i = 0; i < L.size() && out.size() < 32; ++i) {
        const std::vector<std::string> f = splitBar(L[i]); double px = 0;
        if (f.size() < 4 || f[0] != "STR" || !numTok(f[3], px) || px <= 0) continue;
        Level v; v.px = px; v.kind = 2;
        const std::string lab = f.size() >= 10 ? cleanName(f[9]) : std::string();
        char b[32]; std::snprintf(b, sizeof(b), "G %g", px); v.name = lab.empty() || lab == "-" ? std::string(b) : lab;
        out.push_back(v);
    }
    return true;
}
// one list: key levels first (the study's primary attribution KEY > MenthorQ > GEX), levels within mergeTol of an earlier one join it
inline std::vector<Level> mergeLevels(const std::vector<Level>& key, const std::vector<Level>& mq, const std::vector<Level>& gex, double mergeTol) {
    std::vector<Level> all;
    const std::vector<Level>* src[3] = { &key, &mq, &gex };
    for (int s = 0; s < 3; ++s) for (size_t i = 0; i < src[s]->size(); ++i) {
        const Level& v = (*src[s])[i]; bool joined = false;
        for (size_t k = 0; k < all.size() && !joined; ++k) if (std::fabs(all[k].px - v.px) <= mergeTol) {
            if (all[k].name.find(v.name) == std::string::npos && all[k].name.size() + v.name.size() < 24) all[k].name += "/" + v.name;
            joined = true;
        }
        if (!joined) all.push_back(v);
    }
    return all;
}

} // namespace zrule
#endif
