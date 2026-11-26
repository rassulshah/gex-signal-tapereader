#pragma once
// HodLodSweeps.h 1.0.0 (lsHodLod 2.13.0, Rassul 2026-11-01 18:52-19:06 CT, mockup "HodLod box: 3 sections" approved "go") - SDK-free.
// The HOD / LOD SWEEP sections of the box: for each level, how often the day's 1st extreme swept it by no more than 1/4 ADR and turned
// there (the % from the nightly, per market) + the level's price today (from the Dealer Profile's LRA-KeyLevels-<MKT>.csv - the same
// numbers as the Key Levels column).
//   HOD (1st = HOD): PHH | ONH / LonHI / AsiaHI | DPoc | VAH | PDH | PFH
//   LOD (1st = LOD): PHL | ONL / LonLO / AsiaLO | DPoc | VAL | PDL | PFL
// LRA-HodLodSweeps-<MKT>.csv (lra.hodlod_levels 1.5.0 nightly):  VERSION,1 / MARKET,<MKT> / ROW,<HI|LO>,<key>,<pct>,<swept>,<n> ...
//   key = PHH PHL ON3 DPoc VAH VAL PDH PDL PFH PFL (ON3 = any of the three overnight highs / lows). Missing / garbled = "?" percentages.
// A level on the wrong side of today's RTH open (a HOD level below it, a LOD level above it) cannot be that extreme: shown dimmed with
// "below open" / "above open" instead of its price. Fewer than 20 days behind a % = the % with "?".
#include "HodLodLast5.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace hsw {
static const char* const VERSION = "1.4.0";   // (1.4.0, 2026-11-26 15:16 CT "build") + L5 rows read: the last 5 days panel (HodLodLast5.h)
// was static const char* const VERSION = "1.3.0";   // (1.3.0, 2026-11-26 13:43-13:46 CT) + LIFT rows read (the random rate of each sweep cell) and the ROW order kept, for the HOD / LOD swept lines
// was static const char* const VERSION = "1.2.0";   // (1.2.0, 2026-11-26 13:27 CT "go") + SEC2: the 2nd extreme by the open + 3.5 h hourly (last step 30 min before the close) + last 30
// was static const char* const VERSION = "1.1.0";   // (1.1.0, 2026-11-26) + TIME2: the 1st extreme cumulative to 2 hours + after, with its open
static const size_t MAX_BYTES = 8192;
static const int MIN_N = 20;
struct Cell { int pct = -1, swept = 0, n = 0, rnd = -1; };   // (1.3.0) rnd = random levels at the same distance, % (LIFT row; -1 = none)
struct File { std::vector<l5::Day> l5;   // (1.4.0) newest first, <= l5::MAX_DAYS
    bool ok = false; std::string why; std::map<std::string, Cell> hi, lo; int tm[4] = { -1, -1, -1, -1 }; int tmN = 0; int tm2[6] = { -1, -1, -1, -1, -1, -1 }; int tm2Open = -1, tm2N = 0; std::vector<std::string> hiOrd, loOrd; int secOpen = -1, secK = 0, secN = 0, secMin[8] = { 0 }, secPct[8] = { 0 }, secLast = -1;   // (1.2.0) SEC2   // (1.1.0) TIME2: by +15 / 30 / 60 / 90 / 120 min (cumulative), after 2 h; the open they count from
             double onCut[2] = { NAN, NAN }; int dayTrend[3] = { -1, -1, -1 }, dayChop[3] = { -1, -1, -1 }, dayN[3] = { 0, 0, 0 }, projX100[3] = { -1, -1, -1 }; };   // (2.14.0) ONCUT / DAY / PROJ rows   // tm = % of days the 1st extreme formed 0-15 / 15-30 / 30-60 / 60+ min after the open
struct Lv { std::string name; double px = 0; };   // one name of a LRA-KeyLevels row (merged rows are split by the caller)
struct Row { std::string name, px, pct; bool elig = true; };

inline File parse(const std::string& text, const std::string& mkt)
{
    File F; std::stringstream ss(text); std::string ln; bool head = false, mk = false;
    while (std::getline(ss, ln)) {
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln.empty() || ln[0] == '#') continue;
        std::vector<std::string> f; { std::stringstream ls(ln); std::string x; while (std::getline(ls, x, ',')) f.push_back(x); }
        if (f.size() >= 2 && f[0] == "VERSION") { head = f[1] == "1"; continue; }
        if (f.size() >= 2 && f[0] == "MARKET") { mk = f[1] == mkt; continue; }
        if (f.size() >= 6 && f[0] == "ROW" && (f[1] == "HI" || f[1] == "LO")) {
            Cell c; char* e = nullptr;
            c.pct = (int)std::strtol(f[3].c_str(), &e, 10); if (!e || *e || c.pct < 0 || c.pct > 100) continue;
            c.swept = (int)std::strtol(f[4].c_str(), &e, 10); if (!e || *e) continue;
            c.n = (int)std::strtol(f[5].c_str(), &e, 10); if (!e || *e || c.n < 0) continue;
            { auto& M = f[1] == "HI" ? F.hi : F.lo; auto& O = f[1] == "HI" ? F.hiOrd : F.loOrd; if (!M.count(f[2])) O.push_back(f[2]); const int keep = M.count(f[2]) ? M[f[2]].rnd : -1; M[f[2]] = c; M[f[2]].rnd = keep; }   // (1.3.0) file order kept
        }
        if (f[0] == "L5") { l5::Day d; if (F.l5.size() < (size_t)l5::MAX_DAYS && l5::parseRow(f, d)) F.l5.push_back(d); continue; }   // (1.4.0) a bad row is skipped
        if (f.size() >= 3 && f[0] == "ONCUT") { char* e1 = nullptr; char* e2 = nullptr; const double a = std::strtod(f[1].c_str(), &e1), b = std::strtod(f[2].c_str(), &e2);
            if (e1 && !*e1 && e2 && !*e2 && std::isfinite(a) && std::isfinite(b) && a > 0 && b >= a) { F.onCut[0] = a; F.onCut[1] = b; } continue; }
        if (f.size() >= 5 && f[0] == "DAY") { const int k = std::atoi(f[1].c_str()); if (k >= 0 && k < 3) { F.dayTrend[k] = std::atoi(f[2].c_str()); F.dayChop[k] = std::atoi(f[3].c_str()); F.dayN[k] = std::atoi(f[4].c_str()); } continue; }
        if (f.size() >= 4 && f[0] == "PROJ") { const int k = std::atoi(f[1].c_str()); if (k >= 0 && k < 3) F.projX100[k] = std::atoi(f[2].c_str()); continue; }
        if (f.size() >= 7 && f[0] == "LIFT" && (f[1] == "HI" || f[1] == "LO")) {   // (1.3.0) LIFT,<HI|LO>,<key>,<lift>,<random %>,<q>,<ok>
            char* e = nullptr; const long r = std::strtol(f[4].c_str(), &e, 10);
            if (e && !*e && !f[4].empty() && r >= 0 && r <= 100) { auto& M = f[1] == "HI" ? F.hi : F.lo; M[f[2]].rnd = (int)r; }
            continue;
        }
        if (f.size() >= 7 && f[0] == "SEC2") {   // (1.2.0) SEC2,<open>,<k>,<min 1>,<% 1>,...,<min k>,<% k>,<last 30 %>,<n>
            char* e = nullptr; const long op = std::strtol(f[1].c_str(), &e, 10); bool good = e && !*e && op >= 0 && op < 1440;
            const long k = good ? std::strtol(f[2].c_str(), &e, 10) : 0; good = good && e && !*e && k >= 1 && k <= 8 && (long)f.size() >= 3 + 2 * k + 2;
            int mn[8] = { 0 }, pc[8] = { 0 }, prevM = -1, prevP = -1;
            for (long i = 0; good && i < k; i++) {
                const long a = std::strtol(f[(size_t)(3 + 2 * i)].c_str(), &e, 10); if (!e || *e || a <= prevM || a > 1440) { good = false; break; }
                const long b = std::strtol(f[(size_t)(4 + 2 * i)].c_str(), &e, 10); if (!e || *e || b < prevP || b > 100) { good = false; break; }   // cumulative: never falls
                mn[i] = (int)a; pc[i] = (int)b; prevM = (int)a; prevP = (int)b; }
            const long la = good ? std::strtol(f[(size_t)(3 + 2 * k)].c_str(), &e, 10) : -1; good = good && e && !*e && la >= 0 && la <= 100;
            const long n = good ? std::strtol(f[(size_t)(4 + 2 * k)].c_str(), &e, 10) : 0; good = good && e && !*e && n > 0;
            if (good) { F.secOpen = (int)op; F.secK = (int)k; F.secN = (int)n; F.secLast = (int)la; for (int i = 0; i < 8; i++) { F.secMin[i] = mn[i]; F.secPct[i] = pc[i]; } }
            continue;
        }
        if (f.size() >= 9 && f[0] == "TIME2") {  // (1.1.0, Rassul 2026-11-26 09:37 CT) TIME2,<open CT min>,<by 15>,<30>,<60>,<90>,<120>,<after 2 h>,<n days>
            int v[8]; bool good = true;
            for (int i = 0; i < 8; i++) { char* e = nullptr; v[i] = (int)std::strtol(f[(size_t)i + 1].c_str(), &e, 10); if (!e || *e || v[i] < 0) good = false; }
            if (good && (v[0] < 0 || v[0] >= 1440)) good = false;
            for (int i = 1; good && i <= 6; i++) if (v[i] > 100) good = false;
            for (int i = 2; good && i <= 5; i++) if (v[i] < v[i - 1]) good = false;   // cumulative: never falls
            if (good) { F.tm2Open = v[0]; for (int i = 0; i < 6; i++) F.tm2[i] = v[i + 1]; F.tm2N = v[7]; }
            continue;
        }
        if (f.size() >= 6 && f[0] == "TIME") {   // (2.13.0, Rassul 19:12-19:14 CT) TIME,<0-15>,<15-30>,<30-60>,<60+>,<n days>
            int v[5]; bool good = true;
            for (int i = 0; i < 5; i++) { char* e = nullptr; v[i] = (int)std::strtol(f[(size_t)i + 1].c_str(), &e, 10); if (!e || *e || v[i] < 0 || (i < 4 && v[i] > 100)) good = false; }
            if (good) { for (int i = 0; i < 4; i++) F.tm[i] = v[i]; F.tmN = v[4]; }
        }
    }
    if (!head) { F.why = "no VERSION,1"; return F; }
    if (!mk) { F.why = "another market's file"; return F; }
    if (F.hi.empty() && F.lo.empty()) { F.why = "no rows"; return F; }
    F.ok = true; return F;
}
// LRA-KeyLevels-<MKT>.csv (lsDealerProfile on the market's 3-min chart): "KEYLV,1,<stamp>" then "L|<name>|<price>|<pivot>"; merged names
// "ONL/LonLO" are split. Accepted while under maxAgeSec old (the file is rewritten every new 3-min bar; 80 h covers a weekend).
inline bool parseKeys(const std::string& text, long long nowUnix, long long maxAgeSec, std::vector<Lv>& out)
{
    out.clear(); std::stringstream ss(text); std::string ln; bool head = false;
    while (std::getline(ss, ln)) {
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (!head) {
            if (ln.compare(0, 8, "KEYLV,1,") != 0) return false;
            char* e = nullptr; const long long t = std::strtoll(ln.c_str() + 8, &e, 10);
            if (!e || *e || t <= 0 || nowUnix - t > maxAgeSec || t - nowUnix > 12 * 3600) return false;
            head = true; continue;
        }
        std::vector<std::string> c; { std::stringstream ls(ln); std::string x; while (std::getline(ls, x, '|')) c.push_back(x); }
        if (c.size() < 4 || c[0] != "L" || c[1].empty() || c[1].size() > 60 || c[3] == "1") continue;
        char* e = nullptr; const double p = std::strtod(c[2].c_str(), &e);
        if (!e || *e || !std::isfinite(p) || p <= 0) continue;
        std::stringstream ns(c[1]); std::string nm; while (std::getline(ns, nm, '/')) if (!nm.empty()) out.push_back(Lv{ nm, p });
    }
    return head;
}
inline std::string pctText(const File& F, bool hi, const std::string& key)
{
    if (!F.ok) return "?";
    const std::map<std::string, Cell>& M = hi ? F.hi : F.lo; auto it = M.find(key);
    if (it == M.end()) return "?";
    return std::to_string(it->second.pct) + "%" + (it->second.n < MIN_N ? "?" : "");
}
// "0-15m 43% \xB7 15-30m 15% \xB7 30-60m 17% \xB7 60m+ 25%" (when the day's 1st extreme formed, minutes after the RTH open); "" = no row
inline std::string timeText(const File& F)
{
    if (!F.ok || F.tmN <= 0 || F.tm[0] < 0) return "";
    const char* L[4] = { "0-15m ", "15-30m ", "30-60m ", "60m+ " }; std::string s;
    for (int i = 0; i < 4; i++) s += (i ? " \xB7 " : "") + std::string(L[i]) + std::to_string(F.tm[i]) + "%";
    return s + (F.tmN < MIN_N ? " ?" : "");
}
// (2.14.0) the overnight range tercile (0 small, 1 mid, 2 big; -1 = no cut points / no inputs)
inline int onTercile(const File& F, double onRange, double adr)
{
    if (!F.ok || !std::isfinite(F.onCut[0]) || !(adr > 0) || !std::isfinite(onRange) || onRange < 0) return -1;
    const double r = onRange / adr; return r <= F.onCut[0] ? 0 : (r <= F.onCut[1] ? 1 : 2);
}
// "ON range mid \xBB Trend 22% \xB7 Chop 16%" ("" = no forecast)
inline std::string dayText(const File& F, int t)
{
    if (t < 0 || F.dayTrend[t] < 0) return "";
    static const char* const N[3] = { "small", "mid", "big" };
    const std::string q = F.dayN[t] < MIN_N ? "?" : "";
    return std::string("ON range ") + N[t] + " \xBB Trend " + std::to_string(F.dayTrend[t]) + "%" + q + " \xB7 Chop " + std::to_string(F.dayChop[t]) + "%" + q;
}
// cumulative 1st-extreme time: 0-15 / 0-30 / 0-60 / 60m+ from the window shares
inline bool timeCum(const File& F, int out[4])
{
    if (!F.ok || F.tmN <= 0 || F.tm[0] < 0) return false;
    out[0] = F.tm[0]; out[1] = (std::min)(100, F.tm[0] + F.tm[1]); out[2] = (std::min)(100, F.tm[0] + F.tm[1] + F.tm[2]); out[3] = F.tm[3]; return true;
}
// (1.1.0) the 2-hour table: out[0..4] cumulative by +15 / 30 / 60 / 90 / 120 min, out[5] after 2 h; open = the minute they count from
inline bool timeCum2(const File& F, int out[6], int& open)
{
    if (!F.ok || F.tm2N <= 0 || F.tm2[0] < 0 || F.tm2Open < 0) return false;
    for (int i = 0; i < 6; i++) out[i] = F.tm2[i];
    open = F.tm2Open; return true;
}
// (1.2.0) the 2nd-extreme row: k steps (minutes after the open, cumulative %), the last-30 share, the open they count from
inline bool secRow(const File& F, int& k, int mins[8], int pcts[8], int& last, int& open)
{
    if (!F.ok || F.secK <= 0 || F.secN <= 0 || F.secOpen < 0 || F.secLast < 0) return false;
    k = F.secK; for (int i = 0; i < 8; i++) { mins[i] = F.secMin[i]; pcts[i] = F.secPct[i]; } last = F.secLast; open = F.secOpen; return true;
}
inline std::string pxText(double p, int dec)
{
    char b[48]; std::snprintf(b, sizeof b, "%.*f", dec, p); std::string s = b;   // thousands separator as the box's other prices
    size_t dot = s.find('.'); if (dot == std::string::npos) dot = s.size();
    const size_t st = (!s.empty() && s[0] == '-') ? 1 : 0;
    for (long long i = (long long)dot - 3; i > (long long)st; i -= 3) s.insert((size_t)i, ",");
    return s;
}
// the two sections' rows. open = today's RTH open (<= 0 = not open yet: every level counts, shown with its price)
inline void build(const File& F, const std::vector<Lv>& lv, double open, int dec, std::vector<Row>& hi, std::vector<Row>& lo)
{
    hi.clear(); lo.clear();
    auto find = [&](const char* n, double& p) { for (const Lv& x : lv) if (x.name == n && std::isfinite(x.px) && x.px > 0) { p = x.px; return true; } return false; };
    for (int side = 0; side < 2; side++) {
        const bool H = side == 0;
        struct Def { const char* key; const char* show; const char* a; const char* b; const char* c; };
        const Def D[6] = { { H ? "PHH" : "PHL", H ? "PhrH" : "PhrL", H ? "PHH" : "PHL", nullptr, nullptr },   // (lsHodLod 2.18.0) shown PhrH / PhrL (Rassul 2026-11-04 17:02 CT); key + file name stay PHH / PHL
                           { "ON3", H ? "ONH / LonHI / AsiaHI" : "ONL / LonLO / AsiaLO", H ? "ONH" : "ONL", H ? "LonHI" : "LonLO", H ? "AsiaHI" : "AsiaLO" },
                           { "DPoc", "DPoc", "DPoc", nullptr, nullptr },
                           { H ? "VAH" : "VAL", H ? "VAH" : "VAL", H ? "VAH" : "VAL", nullptr, nullptr },
                           { H ? "PDH" : "PDL", H ? "PDH" : "PDL", H ? "PDH" : "PDL", nullptr, nullptr },
                           { H ? "PFH" : "PFL", H ? "PFH" : "PFL", H ? "PFH" : "PFL", nullptr, nullptr } };
        std::vector<Row>& out = H ? hi : lo;
        for (const Def& d : D) {
            Row r; r.name = d.show; r.pct = pctText(F, H, d.key);
            double lo_ = 0, hi_ = 0; int have = 0, ok = 0;
            const char* nm[3] = { d.a, d.b, d.c };
            for (const char* n : nm) {
                double p = 0; if (!n || !find(n, p)) continue; have++;
                const bool e = !(open > 0) || (H ? p > open : p < open);
                if (!e) continue;
                if (!ok) lo_ = hi_ = p; else { lo_ = std::min(lo_, p); hi_ = std::max(hi_, p); }
                ok++;
            }
            if (!have) { r.px = "-"; r.elig = true; }
            else if (!ok) { r.px = H ? "below open" : "above open"; r.elig = false; }
            else r.px = (ok > 1 && hi_ != lo_) ? pxText(lo_, dec) + " - " + pxText(hi_, dec) : pxText(lo_, dec);
            out.push_back(r);
        }
    }
}
} // namespace hsw
