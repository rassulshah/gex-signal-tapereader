#ifndef LS_ZONENEAR_H
#define LS_ZONENEAR_H
// ZoneNear.h 1.0.0 (2026-10-15, Rassul approved mockup-location-first-v1, 08:22 CT) - "location first": is a price AT a zone on a
// given bar, using only what was known at that bar. SDK-free; shared by lsBigBidAsk 1.2.0 and lsDeltaProfile 2.9.0.
// A zone = within `band` of: (1) a key / MenthorQ level (LRA-KeyLevels, LRA-Dealer, LRA-Levels files), (2) the session high or low
// so far (this session's bars up to and including the bar), (3) a pivot confirmed by that bar (2 bars each side, last 60 bars),
// (4) a forming pivot (the bar's own high is the highest / low the lowest of the last 10 bars).
// band = 1.5 x the average 3-min range of the last 14 closed bars (about 0.06 ADR on E-S). Unknown inputs = AT a zone (never dim
// what cannot be judged). Marks away from a zone are DIMMED, never hidden, never moved (no repaint).
#include <vector>
#include <string>
#include <cmath>
#include <ctime>
#include <cctype>
#include <cstring>
#include <fstream>
#include <map>
#include <algorithm>
namespace zn {
static const char* const VERSION = "1.1.0";   // (1.1.0) level NAMES too (levelsNamed / nearest) for the banner
struct Ctx { std::vector<double> hi, lo; std::vector<long long> sess; std::vector<double> levels; double band = 0; };
inline double bandFrom(const std::vector<double>& hi, const std::vector<double>& lo, int lastClosed)
{
    double s = 0; int n = 0;
    for (int k = lastClosed; k >= 0 && n < 14; --k) { const double r = hi[k] - lo[k]; if (r > 0 && std::isfinite(r)) { s += r; ++n; } }
    return n >= 5 ? 1.5 * s / n : 0.0;
}
inline bool atZone(const Ctx& c, int i, double p)
{
    const int n = (int)c.hi.size();
    if (i < 0 || i >= n || (int)c.lo.size() != n || (int)c.sess.size() != n || !(c.band > 0) || !std::isfinite(p)) return true;
    const double b = c.band;
    for (size_t k = 0; k < c.levels.size(); ++k) if (std::fabs(p - c.levels[k]) <= b) return true;
    double sh = -1e300, sl = 1e300;
    for (int k = i; k >= 0 && c.sess[k] == c.sess[i]; --k) { sh = std::max(sh, c.hi[k]); sl = std::min(sl, c.lo[k]); }
    if (std::fabs(p - sh) <= b || std::fabs(p - sl) <= b) return true;
    for (int k = i - 2; k >= 2 && k >= i - 60; --k) {
        const bool ph = c.hi[k] >= c.hi[k - 1] && c.hi[k] >= c.hi[k - 2] && c.hi[k] > c.hi[k + 1] && c.hi[k] > c.hi[k + 2];
        const bool pl = c.lo[k] <= c.lo[k - 1] && c.lo[k] <= c.lo[k - 2] && c.lo[k] < c.lo[k + 1] && c.lo[k] < c.lo[k + 2];
        if ((ph && std::fabs(p - c.hi[k]) <= b) || (pl && std::fabs(p - c.lo[k]) <= b)) return true;
    }
    bool hiF = true, loF = true;
    for (int k = i - 1; k >= 0 && k >= i - 9; --k) { if (c.hi[k] > c.hi[i]) hiF = false; if (c.lo[k] < c.lo[i]) loF = false; }
    if ((hiF && std::fabs(p - c.hi[i]) <= b) || (loF && std::fabs(p - c.lo[i]) <= b)) return true;
    return false;
}
// ---- the level files (bounded, cached 5 s per market); MenthorQ files only for the chart's own contract month ----
inline bool sameMonth(const std::string& a0, const std::string& b0)
{
    if (a0.empty() || b0.empty()) return true;
    std::string a, b; for (char ch : a0) a += (char)std::toupper((unsigned char)ch); for (char ch : b0) b += (char)std::toupper((unsigned char)ch);
    auto my = [](const std::string& s, char& m, char& y) {
        const size_t n = s.size(); auto isM = [](char c) { return c && std::strchr("FGHJKMNQUVXZ", c) != NULL; };
        if (n >= 3 && isM(s[n - 2]) && std::isdigit((unsigned char)s[n - 1])) { m = s[n - 2]; y = s[n - 1]; return true; }
        if (n >= 4 && isM(s[n - 3]) && std::isdigit((unsigned char)s[n - 2]) && std::isdigit((unsigned char)s[n - 1])) { m = s[n - 3]; y = s[n - 1]; return true; }
        return false; };
    char ma = 0, ya = 0, mb = 0, yb = 0;
    if (!my(a, ma, ya) || !my(b, mb, yb)) return true;
    return ma == mb && ya == yb;
}
inline std::vector<std::string> split(const std::string& s, char d)
{ std::vector<std::string> f; std::string c; for (char ch : s) { if (ch == '\r') continue; if (ch == d) { f.push_back(c); c.clear(); } else c += ch; } f.push_back(c); return f; }
struct NL { std::string name; double px = 0; };
inline void readFile(const std::string& path, int kind, const std::string& contract, std::vector<NL>& out)
{
    std::ifstream f(path.c_str(), std::ios::binary); if (!f) return;
    std::string ln; int n = 0; std::string sym; std::vector<NL> v; bool stale = false;
    while (n < 4096 && std::getline(f, ln)) {
        ++n; if (ln.size() > 4096) continue;
        const std::vector<std::string> c = split(ln, '|');
        char* e = nullptr; NL x;
        if (kind == 0 && c.size() >= 3 && c[0] == "L") { x.px = std::strtod(c[2].c_str(), &e); if (e != c[2].c_str() && x.px > 0 && std::isfinite(x.px)) { x.name = c[1].substr(0, 24); v.push_back(x); } }
        if (kind != 0 && c.size() >= 2 && c[0] == "SYMBOL") sym = c[1];
        if (kind != 0 && c.size() >= 2 && c[0] == "STATE" && c[1] == "STALE") stale = true;
        if (kind == 1 && c.size() >= 3 && c[0] == "LEVEL" && c[1] != "NONE") { x.px = std::strtod(c[2].c_str(), &e); if (e != c[2].c_str() && x.px > 0 && std::isfinite(x.px)) { x.name = c[1].substr(0, 24); v.push_back(x); } }
        if (kind == 2 && c.size() >= 4 && c[0] == "LEVEL") { x.px = std::strtod(c[3].c_str(), &e); if (e != c[3].c_str() && x.px > 0 && std::isfinite(x.px)) { x.name = c[1].substr(0, 24); v.push_back(x); } }
    }
    if (kind != 0 && (stale || !sameMonth(contract, sym))) return;     // a roll / another contract: not used (no offset guessed)
    out.insert(out.end(), v.begin(), v.end());
}
struct LevelCache { std::vector<NL> v; std::vector<double> px; long long at = -1; };
inline LevelCache& cache(const std::string& dir, const std::string& market, const std::string& contract, long long nowMs)
{
    static std::map<std::string, LevelCache> m; LevelCache& C = m[market + "|" + contract];
    if (C.at >= 0 && nowMs >= C.at && nowMs - C.at < 5000) return C;
    C.at = nowMs; C.v.clear(); C.px.clear();
    if (dir.empty() || market.empty()) return C;
    std::string d = dir; if (d.back() != '\\' && d.back() != '/') d += "\\";
    readFile(d + "LRA-KeyLevels-" + market + ".csv", 0, contract, C.v);
    readFile(d + "LRA-Dealer-" + market + ".csv", 1, contract, C.v);
    readFile(d + "LRA-Levels-" + market + ".csv", 2, contract, C.v);
    for (size_t i = 0; i < C.v.size(); ++i) C.px.push_back(C.v[i].px);
    return C;
}
inline const std::vector<double>& levels(const std::string& dir, const std::string& market, const std::string& contract, long long nowMs)
{ return cache(dir, market, contract, nowMs).px; }
inline const std::vector<NL>& levelsNamed(const std::string& dir, const std::string& market, const std::string& contract, long long nowMs)
{ return cache(dir, market, contract, nowMs).v; }
// the named level nearest p within `band` (empty name = none)
inline NL nearest(const std::vector<NL>& v, double p, double band)
{ NL best; double d = band; for (size_t i = 0; i < v.size(); ++i) { const double x = std::fabs(v[i].px - p); if (x <= d) { d = x; best = v[i]; } } return best; }
}  // namespace zn
#endif
