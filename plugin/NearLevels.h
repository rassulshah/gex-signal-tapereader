#ifndef LS_NEAR_LEVELS_H
#define LS_NEAR_LEVELS_H
// NearLevels.h (1.0.0, 2026-10-13) -- SDK-free helpers shared by lsPullStats / lsStackStats 1.0.0 and lsBidAskStats 1.3.0:
//   * the Liquidity Profile 0.14.0 NEAR band (ATR of closed bars, per-market zone half-width)
//   * the key / gamma levels, read exactly as lsDeltaProfile 2.7.0 reads them (lsFlexLevels\):
//       LRA-KeyLevels-<MKT>.csv  "KEYLV,1,<unix>" then L|<name>|<price>|<pivot>; older than 18 h = not used
//       LRA-Dealer-<MKT>.csv     LEVEL|<label>|<px>|... and the 3 largest-|gamma| NODE|<strike>|<gamma>|... (>= 8 fields) as "G <strike>";
//                                SYMBOL must be the chart's contract (ESZ6 == EPZ26; a continuous chart / no SYMBOL = ok); mtime <= 18 h
//       LRA-Levels-<MKT>.csv     LEVEL|<code>|<name>|<price>|... (STATE|STALE = not used); same contract rule; mtime <= 18 h
//     every file bounded (256 KB, 4,096 lines of <= 4,096 chars); a malformed file keeps the last good snapshot.
//   * the TOP MESSAGE helpers (Rassul 11:15 / 12:05): the plain-English message line coloured by implication, cut to fit (never over
//     the price axis); the numbers go in the info box (12:05) and the session record.
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

namespace nl {

inline bool finite(double v) { return v == v && v < 1e300 && v > -1e300; }

// ---- the near band (Liquidity Profile 0.14.0) ----------------------------------------------------------------------------
struct Zone { double k = 0.25; int hwMin = 4, hwMax = 8; };
inline Zone zoneFor(const std::string& m) {           // LiquidityProfile.cpp per-market defaults
    Zone z;
    if (m == "ES") { z.k = 0.30; z.hwMin = 4; z.hwMax = 6; }
    else if (m == "NQ") { z.k = 0.11; z.hwMin = 8; z.hwMax = 12; }
    else if (m == "GC") { z.k = 0.25; z.hwMin = 8; z.hwMax = 15; }
    else if (m == "HG") { z.k = 0.30; z.hwMin = 2; z.hwMax = 5; }
    else if (m == "NG") { z.k = 0.30; z.hwMin = 2; z.hwMax = 4; }
    else if (m == "EU") { z.k = 0.40; z.hwMin = 2; z.hwMax = 3; }
    return z;                                          // CL and anything else: 0.25 / 4 / 8
}
inline int halfWidth(const Zone& z, double atr, double tick) {
    const double a = atr > 0 && finite(atr) ? atr : 15 * tick;
    const double raw = z.k * a / tick;
    if (!finite(raw) || raw >= z.hwMax) return z.hwMax;
    if (raw <= z.hwMin) return z.hwMin;
    const int r = (int)std::llround(raw);
    return r < z.hwMin ? z.hwMin : r > z.hwMax ? z.hwMax : r;
}
static const int NEAR_MAX = 2000;
// "Near ticks" > 0 wins; 0 = auto: max(round(1.0 x ATR / tick), 4 x half-width)
inline long long nearBand(double atr, double tick, const Zone& z, int setting) {
    if (setting > 0) return setting > NEAR_MAX ? NEAR_MAX : setting;
    if (!(tick > 0) || !finite(tick)) return 0;
    const double a = atr > 0 && finite(atr) ? atr : 15 * tick;
    const double raw = 1.0 * a / tick;
    const long long n = finite(raw) && raw < 1e9 ? (long long)std::llround(raw) : 1000000000LL;
    const long long hw4 = (long long)(4 * halfWidth(z, atr, tick));
    return n > hw4 ? n : hw4;
}
// ATR of closed bars: ranges newest first (high - low), the last 20, scaled to a 3-min bar; 0 = none
inline double atrOf(const std::vector<double>& ranges, int spb) {
    double s = 0; int m = 0;
    for (size_t i = 0; i < ranges.size() && m < 20; ++i) { if (!finite(ranges[i]) || ranges[i] < 0) continue; s += ranges[i]; ++m; }
    if (m == 0) return 0;
    double a = s / m;
    if (spb > 0 && spb != 180) a *= std::sqrt(180.0 / (double)spb);
    return a;
}

// ---- levels ---------------------------------------------------------------------------------------------------------------
struct Level { std::string name; double px; };
inline std::vector<std::string> fields(const std::string& line) {
    std::vector<std::string> c; size_t a = 0;
    for (;;) { const size_t p = line.find('|', a); c.push_back(line.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; }
    return c;
}
inline bool parseReal(const std::string& t, double& out) {
    if (t.empty() || t.size() > 40 || std::isspace((unsigned char)t[0])) return false;
    errno = 0; char* e = NULL; const double v = std::strtod(t.c_str(), &e);
    if (errno != 0 || e != t.c_str() + t.size() || !finite(v)) return false;
    out = v; return true;
}
inline bool parseWhole(const std::string& t, long long lo, long long hi, long long& out) {
    if (t.empty() || t.size() > 24 || std::isspace((unsigned char)t[0])) return false;
    errno = 0; char* e = NULL; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno != 0 || e != t.c_str() + t.size() || v < lo || v > hi) return false;
    out = v; return true;
}
inline bool readBoundedLines(const std::string& path, std::vector<std::string>& lines, size_t maxBytes, size_t maxLines, size_t maxLine) {
    lines.clear();
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in.is_open()) return false;
    std::string line; char ch = 0; size_t bytes = 0;
    while (in.get(ch)) {
        if (++bytes > maxBytes) { lines.clear(); return false; }
        if (ch == '\n') {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (lines.size() >= maxLines) { lines.clear(); return false; }
            lines.push_back(line); line.clear();
        } else { if (line.size() >= maxLine) { lines.clear(); return false; } line += ch; }
    }
    if (in.bad()) { lines.clear(); return false; }
    if (!line.empty()) { if (lines.size() >= maxLines) { lines.clear(); return false; } lines.push_back(line); }
    return true;
}
inline long long fileMtime(const std::string& path) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(path.c_str(), &st) != 0) return -1;
#else
    struct stat st; if (stat(path.c_str(), &st) != 0) return -1;
#endif
    if ((st.st_mode & S_IFMT) != S_IFREG) return -1;
    return (long long)st.st_mtime;
}
inline long long fileSig(const std::string& path) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(path.c_str(), &st) != 0) return -1;
#else
    struct stat st; if (stat(path.c_str(), &st) != 0) return -1;
#endif
    if ((st.st_mode & S_IFMT) != S_IFREG) return -1;
    return (long long)st.st_mtime * 1000003LL + (long long)st.st_size;
}
inline bool monthYearOf(const std::string& s, char& mon, int& year) {
    const size_t n = s.size();
    auto isM = [](char c) { return c != 0 && std::strchr("FGHJKMNQUVXZ", c) != NULL; };
    auto dig = [](char c) { return c >= '0' && c <= '9'; };
    if (n >= 4 && isM(s[n - 3]) && dig(s[n - 2]) && dig(s[n - 1])) { mon = s[n - 3]; year = (s[n - 1] - '0'); return true; }
    if (n >= 4 && isM(s[n - 2]) && dig(s[n - 1]) && !dig(s[n - 3])) { mon = s[n - 2]; year = s[n - 1] - '0'; return true; }
    return false;
}
inline bool sameContract(const std::string& chart, const std::string& file) {
    if (chart.empty() || file.empty()) return true;
    std::string a, b;
    for (char ch : chart) a += (char)std::toupper((unsigned char)ch);
    for (char ch : file) b += (char)std::toupper((unsigned char)ch);
    if (a == b) return true;
    char ma = 0, mb = 0; int ya = 0, yb = 0;
    if (!monthYearOf(a, ma, ya) || !monthYearOf(b, mb, yb)) return true;
    return ma == mb && ya == yb;
}
// one source parsed from its lines; kind 0 = KeyLevels, 1 = Dealer, 2 = Levels. false = malformed (keep the last good snapshot)
struct Source { std::vector<Level> v; long long sig = -2, at = 0; std::string sym; bool stale = false; };
inline bool parseSource(const std::vector<std::string>& lines, int kind, long long mtime, Source& out) {
    Source n; n.at = mtime;
    std::vector<std::pair<double, Level> > nodes;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& ln = lines[i];
        if (kind == 0) {
            if (i == 0) { long long t = 0; if (ln.compare(0, 8, "KEYLV,1,") != 0 || !parseWhole(ln.substr(8), 1, 32503680000LL, t)) return false; n.at = t; continue; }
            const std::vector<std::string> c = fields(ln); double px;
            if (c.size() >= 4 && c[0] == "L" && !c[1].empty() && c[1].size() <= 40 && parseReal(c[2], px) && px > 0) n.v.push_back(Level{ c[1], px });
            continue;
        }
        const std::vector<std::string> c = fields(ln); double px, g;
        if (c.size() >= 2 && c[0] == "SYMBOL") n.sym = c[1];
        if (c.size() >= 2 && c[0] == "STATE" && c[1] == "STALE") n.stale = true;
        if (kind == 1 && c.size() >= 3 && c[0] == "LEVEL" && c[1] != "NONE" && parseReal(c[2], px) && px > 0) n.v.push_back(Level{ c[1].substr(0, 40), px });
        if (kind == 1 && c.size() >= 8 && c[0] == "NODE" && parseReal(c[1], px) && px > 0 && parseReal(c[2], g)) nodes.push_back(std::make_pair(std::fabs(g), Level{ "G " + c[1].substr(0, 20), px }));
        if (kind == 2 && c.size() >= 4 && c[0] == "LEVEL" && !c[1].empty() && parseReal(c[3], px) && px > 0) n.v.push_back(Level{ c[1].substr(0, 40), px });
    }
    std::stable_sort(nodes.begin(), nodes.end(), [](const std::pair<double, Level>& a, const std::pair<double, Level>& b) { return a.first > b.first; });
    for (size_t i = 0; i < nodes.size() && i < 3; ++i) if (nodes[i].first > 0) n.v.push_back(nodes[i].second);
    if (kind == 0 && lines.empty()) return false;
    out = n; return true;
}
inline void loadSource(const std::string& path, Source& S, int kind) {
    const long long sig = fileSig(path);
    if (sig == S.sig) return;
    if (sig < 0) { S = Source(); S.sig = sig; return; }
    std::vector<std::string> lines;
    if (!readBoundedLines(path, lines, 256 * 1024, 4096, 4096) || fileSig(path) != sig) return;
    Source n;
    if (!parseSource(lines, kind, fileMtime(path), n)) return;
    n.sig = sig; S = n;
}
// the levels of a market for a contract, re-read at most every 5 s (steady ms); wall = time(NULL)
struct Cache { Source key, dealer, mq; long long checked = -1; std::string note; };
inline std::vector<Level> levels(Cache& C, const std::string& dir, const std::string& market, const std::string& contract, long long nowMs, long long wall) {
    std::vector<Level> out;
    if (dir.empty() || market.empty()) { C.note = "no level folder"; return out; }
    if (C.checked < 0 || nowMs - C.checked >= 5000 || nowMs < C.checked) {
        C.checked = nowMs;
        loadSource(dir + "LRA-KeyLevels-" + market + ".csv", C.key, 0);
        loadSource(dir + "LRA-Dealer-" + market + ".csv", C.dealer, 1);
        loadSource(dir + "LRA-Levels-" + market + ".csv", C.mq, 2);
    }
    const long long FRESH = 18 * 3600;
    auto fresh = [&](const Source& S) { return S.sig >= 0 && S.at > 0 && wall - S.at <= FRESH && S.at - wall <= 600; };
    int used = 0;
    if (fresh(C.key)) { out.insert(out.end(), C.key.v.begin(), C.key.v.end()); ++used; }
    const bool contractOk = sameContract(contract, C.dealer.sym);
    if (contractOk && fresh(C.dealer)) { out.insert(out.end(), C.dealer.v.begin(), C.dealer.v.end()); ++used; }
    if (contractOk && sameContract(contract, C.mq.sym) && fresh(C.mq) && !C.mq.stale) { out.insert(out.end(), C.mq.v.begin(), C.mq.v.end()); ++used; }
    C.note = !contractOk ? "MenthorQ levels skipped (file is " + C.dealer.sym + ")" : std::to_string(out.size()) + " levels from " + std::to_string(used) + " files";
    return out;
}
// the nearest level to price within band (price units); "" = none. Equal distance: the first listed (key levels first).
inline std::string nearestLevel(const std::vector<Level>& v, double price, double band) {
    if (!finite(price) || !(band >= 0)) return std::string();
    double best = band; std::string name; bool have = false;
    for (size_t i = 0; i < v.size(); ++i) {
        const double d = std::fabs(v[i].px - price);
        if (!finite(d)) continue;
        if (d <= best + 1e-9 && (!have || d < best - 1e-9)) { best = d; name = v[i].name; have = true; }
    }
    return name;
}

// ---- the top message ------------------------------------------------------------------------------------------------------
enum Impl { NEUTRAL = 0, BULL = 1, BEAR = 2 };
struct Msg { std::string line1, line2; int impl = NEUTRAL; };
// "10:30" (12-hour CT, no AM / PM - the mockup)
inline std::string clockShort(int hour, int minute) {
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return std::string();
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d", hour % 12 == 0 ? 12 : hour % 12, minute); return b;
}
inline std::string ratioText(double r) { if (!finite(r) || r < 0) return "n/a"; char b[24]; std::snprintf(b, sizeof(b), "%.1fx", r > 9999 ? 9999.0 : r); return b; }
// the text cut to fit maxW pixels (width = the host's text measure), ending in "..."; "" when not even "..." fits
template <class W> std::string fitText(const std::string& s, int maxW, W width) {
    if (maxW <= 0) return std::string();
    if (width(s) <= maxW) return s;
    std::string t = s;
    while (!t.empty()) {
        t.pop_back();
        while (!t.empty() && t.back() == ' ') t.pop_back();
        if (width(t + "...") <= maxW) return t + "...";
    }
    return width("...") <= maxW ? std::string("...") : std::string();
}
// the session message log line: bar_time|ct_time|implication|level|message|numbers (| and newlines never inside a field)
inline std::string clean(const std::string& s) { std::string o; for (char c : s) o += (c == '|' || c == '\n' || c == '\r') ? ' ' : c; return o; }
inline std::string msgLine(long long t, const std::string& ct, const std::string& level, const Msg& m) {
    return std::to_string(t) + "|" + clean(ct) + "|" + (m.impl == BULL ? "bull" : m.impl == BEAR ? "bear" : "neutral") + "|" + clean(level) + "|" + clean(m.line1) + "|" + clean(m.line2);
}
static const char* const MSG_HEADER = "bar_time|ct_time|implication|level|message|numbers";
// a log line back: its bar time and level (the rest is for the nightly); false = not a message line
inline bool parseMsgLine(const std::string& ln, long long& t, std::string& level) {
    const std::vector<std::string> c = fields(ln);
    if (c.size() != 6 || c[0].empty() || c[0].size() > 13) return false;
    for (char ch : c[0]) if (ch < '0' || ch > '9') return false;
    if (c[3].size() > 40 || (c[2] != "bull" && c[2] != "bear" && c[2] != "neutral")) return false;
    t = std::strtoll(c[0].c_str(), nullptr, 10); level = c[3]; return t > 0;
}

} // namespace nl
#endif
