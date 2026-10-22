#ifndef LS_IMPLIEDFILL_H
#define LS_IMPLIEDFILL_H
// ImpliedFill.h 1.0.0 (2026-10-22 evening, Rassul 17:16 CT approved "IMPLIED FILLS option B") - SDK-free, shared.
// WHAT: implied (spread-driven) fills arrive in the live Rithmic order feed (DEPTH_BY_ORDER) as FILL with a side that is neither bid (1)
//   nor offer (0) - the MBO recorder writes it as side "?" (MboRecord.h). The nightly feed check matched them to Databento's implied
//   trades (CL 8.44% live = Databento). Share of volume: CL 10%, NG 5.4%, HG 3.3%, 6E 2.6%, GC 0.7%, ES / NQ 0.
// RULE: an implied fill is NOT an aggressive buyer or seller hitting the book. It is EXCLUDED from delta, big-lot counts (S20 / S10 / S5
//   and the Big Bid / Ask prints), St / Sta bursts, Sr sweeps and the attacker / defense strip letters; it is KEPT in total volume.
// PARAMS: lsFlexLevels\LRA-Implied-<MKT>.csv (written by the nightly): PARAM,exclude,1 and PARAM,live_code,? (VERSION / MARKET / ASOF
//   lines optional; a MARKET line must match). Missing file = exclude anyway (safe) and NO hover row; garbled = exclude + reason.
// HOVER: "Implied: 23% of this candle's volume" in the candle stats hovers (lsBidAskStats / lsBookFlow), only with the file present.
#include <cstdlib>
#include <string>
#include <vector>

namespace imp {
static const char* const VERSION = "1.0.0";

// the raw DEPTH_ORDER buySell of a FILL: 1 bid, 0 offer, anything else = the recorder's "?" (implied)
inline bool unknownSide(long long buySell) { return buySell != 0 && buySell != 1; }
// the recorder's side character for a raw buySell (MboRecord.h writes the same)
inline char sideCode(long long buySell) { return buySell == 1 ? 'B' : buySell == 0 ? 'S' : '?'; }

struct Params {
    bool exclude = true;        // exclude implied fills from aggressor counts (safe default)
    bool fromFile = false;      // the nightly file was read and is valid (only then the hover row shows)
    char code = '?';            // the live side code that marks an implied fill
    std::string why = "no LRA-Implied file - side '?' fills excluded (safe default)";
};
// is this FILL (raw buySell) implied under these params? (only the '?' code is defined live; any other code = nothing is implied)
inline bool isImplied(const Params& P, long long buySell) { return P.code == '?' && unknownSide(buySell); }

inline std::string trimS(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '"' || s[a] == '\'')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '"' || s[b - 1] == '\'')) --b;
    return s.substr(a, b - a);
}
inline std::vector<std::string> cells(const std::string& line) {
    std::vector<std::string> c; std::string cur;
    for (char ch : line) { if (ch == ',') { c.push_back(trimS(cur)); cur.clear(); } else cur += ch; }
    c.push_back(trimS(cur)); return c;
}
inline std::string upper(std::string s) { for (char& ch : s) if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A'); return s; }
// the market names the nightly may use for one chart market (the charts call 6E "EU")
inline bool sameMarket(const std::string& a, const std::string& b) {
    const std::string x = upper(a), y = upper(b);
    if (x == y) return true;
    return (x == "EU" && y == "6E") || (x == "6E" && y == "EU");
}
// strict parse: unknown PARAM names are ignored (forward compatible), a bad value = garbled (defaults, reason)
inline Params parse(bool found, bool readOk, const std::vector<std::string>& lines, const std::string& market) {
    Params p;
    if (!found) return p;
    if (!readOk) { p.why = "LRA-Implied file unreadable - side '?' fills excluded"; return p; }
    bool haveExclude = false, haveCode = false;
    Params q;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string ln = trimS(lines[i]);
        if (ln.empty() || ln[0] == '#') continue;
        const std::vector<std::string> c = cells(ln);
        const std::string k = upper(c[0]);
        if (k == "VERSION" || k == "ASOF") continue;
        if (k == "MARKET") {
            if (c.size() < 2 || !sameMarket(c[1], market)) { p.why = "LRA-Implied file is for another market - side '?' fills excluded"; return p; }
            continue;
        }
        if (k != "PARAM" || c.size() < 3) continue;
        const std::string name = upper(c[1]), v = c[2];
        if (name == "EXCLUDE") {
            if (v == "1") q.exclude = true; else if (v == "0") q.exclude = false;
            else { p.why = "garbled exclude value '" + v.substr(0, 8) + "' - side '?' fills excluded"; return p; }
            haveExclude = true;
        } else if (name == "LIVE_CODE") {
            if (v.size() != 1) { p.why = "garbled live_code '" + v.substr(0, 8) + "' - side '?' fills excluded"; return p; }
            q.code = v[0]; haveCode = true;
        }
    }
    if (!haveExclude) { p.why = "LRA-Implied file has no PARAM,exclude - side '?' fills excluded"; return p; }
    q.fromFile = true;
    q.why = std::string("LRA-Implied file: exclude ") + (q.exclude ? "1" : "0") + ", live_code " + q.code + (haveCode ? "" : " (default)");
    return q;
}
// "23%" of this candle's volume (implied lots / all filled lots incl. implied); false = no row (no file, no volume)
inline bool hoverPct(const Params& P, long long implied, long long total, std::string& out) {
    if (!P.fromFile || total <= 0 || implied < 0) return false;
    if (implied > total) implied = total;
    const long long pct = (implied * 100 + total / 2) / total;
    out = std::to_string(pct) + "%";
    return true;
}
inline std::string hoverText(const std::string& pct) { return "Implied: " + pct + " of this candle's volume"; }
}  // namespace imp
#endif
