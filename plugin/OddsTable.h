#ifndef LS_ODDSTABLE_H
#define LS_ODDSTABLE_H
// OddsTable.h 1.0.0 (2026-10-18) -- the probability engine's answers inside IRT, native, no run-time dependency. SDK-free (unit-tested on Linux).
// Rassul 2026-10-17 23:14: "a probability driven application ... I should know the probability of a cob working on the fly when it happens".
// Input: lsFlexLevels\LRA-Odds-<MKT>.csv, written nightly by the labs (lra.odds_table):
//   VERSION|1  MARKET|ES  BUILT|<stamp>|<who>
//   Q|<qid>|<words>|base|<p>|n|<n>|keys|<k1,k2,..>|status|LIVE/LEARNING/OFF|calib|<gap>
//   P|<qid>|<v1>|..|<vk>|p|<p>|n|<n>|lo|<lo>|hi|<hi>        ('*' = any)
// Lookup = the most specific row with n >= MIN_N: exact, then the last key widened to '*', then the one before ... Nothing -> unknown ('?').
// A question that is not LIVE answers unknown on the chart (the number is still in the status file for checking).
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <cstdio>

namespace odt {

static const char* const VERSION = "1.0.0";
static const int MIN_N = 20;
static const size_t MAX_BYTES = 512 * 1024;

struct Row { double p = 0, lo = 0, hi = 1; int n = 0; };
struct Q {
    std::string id, words, status; double base = 0, calib = -1; int n = 0;
    std::vector<std::string> keys;
    std::map<std::string, Row> rows;           // key values joined by '|'
};
struct Answer { bool ok = false, live = false; double p = 0, base = 0, lo = 0, hi = 1; int n = 0; std::string matched; };
struct Table { bool ok = false; std::string why, market, built; std::map<std::string, Q> qs; };

inline std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> o; size_t a = 0;
    while (true) { size_t b = s.find(d, a); o.push_back(s.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
    return o;
}
inline bool num(const std::string& t, double& x) { if (t.empty()) return false; char* e = nullptr; x = std::strtod(t.c_str(), &e); return e && *e == 0; }
inline bool inum(const std::string& t, int& x) { double d; if (!num(t, d) || d < 0 || d > 1e9) return false; x = (int)d; return true; }
inline std::string join(const std::vector<std::string>& v, size_t from, size_t to) { std::string o; for (size_t i = from; i < to; i++) { if (i > from) o += '|'; o += v[i]; } return o; }

inline Table parse(const std::string& text, const std::string& market) {
    Table T; if (text.size() > MAX_BYTES) { T.why = "over 512 KiB"; return T; }
    bool ver = false; size_t a = 0; int lines = 0;
    while (a < text.size() && lines++ < 20000) {
        size_t b = text.find('\n', a); std::string ln = text.substr(a, b == std::string::npos ? std::string::npos : b - a); a = b == std::string::npos ? text.size() : b + 1;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln.empty() || ln[0] == '#') continue;
        std::vector<std::string> c = split(ln, '|');
        if (c[0] == "VERSION") { if (c.size() < 2 || c[1] != "1") { T.why = "unknown file version"; return T; } ver = true; }
        else if (c[0] == "MARKET") { if (c.size() < 2 || c[1] != market) { T.why = "file is for another market"; return T; } T.market = c[1]; }
        else if (c[0] == "BUILT") { if (c.size() >= 2) T.built = c[1]; }
        else if (c[0] == "Q" && c.size() >= 3) {
            Q q; q.id = c[1]; q.words = c[2];
            for (size_t i = 3; i + 1 < c.size(); i += 2) {
                const std::string& k = c[i]; const std::string& v = c[i + 1];
                if (k == "base") num(v, q.base); else if (k == "n") inum(v, q.n); else if (k == "keys") q.keys = split(v, ',');
                else if (k == "status") q.status = v; else if (k == "calib") { double x; if (num(v, x)) q.calib = x; }
            }
            if (!q.id.empty() && !q.keys.empty()) T.qs[q.id] = q;
        }
        else if (c[0] == "P" && c.size() >= 2) {
            auto it = T.qs.find(c[1]); if (it == T.qs.end()) continue;
            Q& q = it->second; const size_t nk = q.keys.size(); if (c.size() < 2 + nk) continue;
            Row r; bool hasP = false;
            for (size_t i = 2 + nk; i + 1 < c.size(); i += 2) {
                const std::string& k = c[i]; const std::string& v = c[i + 1];
                if (k == "p") hasP = num(v, r.p); else if (k == "n") inum(v, r.n); else if (k == "lo") num(v, r.lo); else if (k == "hi") num(v, r.hi);
            }
            if (hasP && r.p >= 0 && r.p <= 1) q.rows[join(c, 2, 2 + nk)] = r;
        }
    }
    if (!ver) { T.why = "no VERSION row"; return T; }
    if (T.market.empty()) { T.why = "no MARKET row"; return T; }
    T.ok = true; return T;
}

// vals in the question's key order; back-off from the last key
inline Answer lookup(const Table& T, const std::string& qid, std::vector<std::string> vals) {
    Answer A; if (!T.ok) return A;
    auto it = T.qs.find(qid); if (it == T.qs.end()) return A;
    const Q& q = it->second; A.base = q.base; A.live = q.status == "LIVE";
    if (vals.size() != q.keys.size()) return A;
    for (size_t widen = 0; widen <= vals.size(); widen++) {
        if (widen > 0) vals[vals.size() - widen] = "*";
        std::string k = join(vals, 0, vals.size());
        auto r = q.rows.find(k);
        if (r != q.rows.end() && r->second.n >= MIN_N) { A.ok = true; A.p = r->second.p; A.lo = r->second.lo; A.hi = r->second.hi; A.n = r->second.n; A.matched = k; return A; }
    }
    return A;
}
inline int pct(double p) { return (int)(p * 100.0 + 0.5); }
// chart text: "60%" when LIVE and known, "?" otherwise
inline std::string chartText(const Answer& A) { if (!A.ok || !A.live) return "?"; return std::to_string(pct(A.p)) + "%"; }
// hover: "60% reach 1R (usual 46%, 71 cases)"
inline std::string hoverText(const Answer& A, const std::string& what) {
    if (!A.ok) return "? " + what + " (fewer than 20 past cases like this)";
    std::string s = std::to_string(pct(A.p)) + "% " + what + " (usual " + std::to_string(pct(A.base)) + "%, " + std::to_string(A.n) + " cases)";
    if (!A.live) s += " - still learning, not shown on the chart";
    return s;
}

}   // namespace odt
#endif
