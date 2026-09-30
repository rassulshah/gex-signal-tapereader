/********************************************************************************
 *  DealerLogic.h  --  the testable half of lsDealerProfile + lsDealerRead (no IRT SDK)
 *
 *  Both plugins read %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Dealer-<MKT>.csv, written every 5 min by the
 *  LRA Reader (level-reversal-analytics: analytics/lra/dealer_irt.py). Every decision is made there; the plugins only
 *  draw. Fields are separated by '|'. Rows:
 *     VERSION|1.0            ASOF|<CT sec-of-day>|<yyyy-mm-dd>      MARKET|GC      PRICE|<px>|<em>
 *     BOOK|<gamma at price, fut per 0.1 EM>|FUEL|WALL|short|long
 *     NODE|<strike>|<gamma on arrival>|<lo>|<hi>|<snapshot>|<delta flow, + = BUY>|<far 0/1>
 *     LEVEL|<key label>|<px>|<phase 0..4>|<context text>|S|R           (LEVEL|NONE = nothing near)
 *     WALLHDR|<title>|<score>|<G/R/A/Y/N>     WALLROW|<name>|<meter -100..100 or NA>|<label>|ok|bad|watch|wait|<why>
 *     FUELHDR|FUEL|<score>|<col>               FUELROW|... (as WALLROW)
 *     TRADE|<key>|<value>|<col>   FLAG|<text>|<col>   DO|<text>   BOOKLINE|<text>   LEARN|1
 ********************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cctype>

namespace dl {

struct Node { float k = 0, g = 0, lo = 0, hi = 0, snap = 0, d = 0; bool far = false; };
struct Row  { std::string n, lab, st, why; bool hasV = false; float v = 0; };
struct KV   { std::string k, v; char col = 'W'; };

struct Data {
    std::string ver, market;
    double asofSo = -1; int y = 0, mo = 0, d = 0;
    float px = 0, em = 0; bool hasPrice = false;
    float book = 0; bool hasBook = false;
    std::vector<Node> nodes;
    bool hasLevel = false; std::string lvlLabel, ctx; float lvlPx = 0; int phase = 0; char side = 'R';
    std::string wallTitle, wallScore; char wallCol = 'N'; std::vector<Row> wall;
    std::string fuelScore; char fuelCol = 'N'; std::vector<Row> fuel;
    std::vector<KV> trade; std::vector<KV> flags;
    std::string doText, bookLine; bool learn = false;
};

inline std::vector<std::string> split(const std::string& line, char sep = '|')
{
    std::vector<std::string> t; std::string cur;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == '\r' || c == '\n') continue;
        if (c == sep) { t.push_back(cur); cur.clear(); } else cur += c;
    }
    t.push_back(cur);
    return t;
}
inline float f(const std::string& s) { return (float)atof(s.c_str()); }
inline char col1(const std::string& s) { return s.empty() ? 'W' : s[0]; }

inline bool parseRow(const std::vector<std::string>& t, Row& r)
{
    if (t.size() < 6) return false;
    r.n = t[1]; r.hasV = !(t[2] == "NA" || t[2].empty()); r.v = r.hasV ? f(t[2]) : 0.0f;
    if (r.v > 100) r.v = 100;
    if (r.v < -100) r.v = -100;
    r.lab = t[3]; r.st = t[4]; r.why = t[5];
    return true;
}

// one line into the data; returns false for a line it does not know (ignored)
inline bool parseLine(Data& D, const std::string& line)
{
    if (line.empty() || line[0] == '#') return false;
    std::vector<std::string> t = split(line);
    const std::string& k = t[0];
    if (k == "VERSION" && t.size() >= 2) { D.ver = t[1]; return true; }
    if (k == "MARKET" && t.size() >= 2) { D.market = t[1]; return true; }
    if (k == "ASOF" && t.size() >= 2) {
        D.asofSo = atof(t[1].c_str());
        if (t.size() >= 3) { int a = 0, b = 0, c = 0; if (sscanf(t[2].c_str(), "%d-%d-%d", &a, &b, &c) == 3) { D.y = a; D.mo = b; D.d = c; } }
        return true;
    }
    if (k == "PRICE" && t.size() >= 3) { D.px = f(t[1]); D.em = f(t[2]); D.hasPrice = D.px > 0 && D.em > 0; return true; }
    if (k == "BOOK" && t.size() >= 2) { D.book = f(t[1]); D.hasBook = true; return true; }
    if (k == "NODE" && t.size() >= 8) {
        Node n; n.k = f(t[1]); n.g = f(t[2]); n.lo = f(t[3]); n.hi = f(t[4]); n.snap = f(t[5]); n.d = f(t[6]); n.far = t[7] == "1";
        if (n.k > 0) { D.nodes.push_back(n); return true; }
        return false;
    }
    if (k == "LEVEL") {
        if (t.size() >= 2 && t[1] == "NONE") { D.hasLevel = false; return true; }
        if (t.size() < 6) return false;
        D.hasLevel = true; D.lvlLabel = t[1]; D.lvlPx = f(t[2]); D.phase = atoi(t[3].c_str()); D.ctx = t[4];
        if (D.phase < 0) D.phase = 0;
        if (D.phase > 4) D.phase = 4;
        D.side = (t.size() >= 6 && !t[5].empty()) ? t[5][0] : 'R';
        return true;
    }
    if (k == "WALLHDR" && t.size() >= 4) { D.wallTitle = t[1]; D.wallScore = t[2]; D.wallCol = col1(t[3]); return true; }
    if (k == "FUELHDR" && t.size() >= 4) { D.fuelScore = t[2]; D.fuelCol = col1(t[3]); return true; }
    if (k == "WALLROW") { Row r; if (!parseRow(t, r)) return false; D.wall.push_back(r); return true; }
    if (k == "FUELROW") { Row r; if (!parseRow(t, r)) return false; D.fuel.push_back(r); return true; }
    if (k == "TRADE" && t.size() >= 4) { KV v; v.k = t[1]; v.v = t[2]; v.col = col1(t[3]); D.trade.push_back(v); return true; }
    if (k == "FLAG" && t.size() >= 3) { KV v; v.k = t[1]; v.col = col1(t[2]); D.flags.push_back(v); return true; }
    if (k == "DO" && t.size() >= 2) { D.doText = t[1]; return true; }
    if (k == "BOOKLINE" && t.size() >= 2) { D.bookLine = t[1]; return true; }
    if (k == "LEARN" && t.size() >= 2) { D.learn = t[1] == "1"; return true; }
    return false;
}

inline Data parseText(const std::string& text)
{
    Data D; std::stringstream ss(text); std::string line;
    while (std::getline(ss, line)) parseLine(D, line);
    return D;
}

// The chart's market from IRT's root symbol (EPZ26 -> "EP", GCEZ26 -> "GCE" ...). Markets: ES NQ CL GC HG NG EU.
inline std::string marketForRoot(const std::string& rootIn)
{
    std::string r; for (size_t i = 0; i < rootIn.size(); i++) r += (char)toupper((unsigned char)rootIn[i]);
    // (1.0.1) "contains", checked in this order: his GC chart's root is "QGC" - 1.0 matched its "QG" prefix to NG
    auto has = [&](const char* k) { return r.find(k) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return "";
}
static const char* MARKETS[] = { "Auto", "ES", "NQ", "CL", "GC", "HG", "NG", "EU" };
inline std::string marketFor(int setting, const std::string& root) { return (setting >= 1 && setting <= 7) ? MARKETS[setting] : marketForRoot(root); }

// Minutes since the file was written (CT sec-of-day vs the chart's clock); wraps over midnight
inline double staleMin(double asofSo, double localSo)
{
    if (asofSo < 0) return 0;
    double d = localSo - asofSo; if (d < -600) d += 86400; if (d < 0) d = 0;
    return d / 60.0;
}

// Contract offset: the chart may be another contract than MenthorQ's front future. Offset = the chart's close at the
// file's minute - the file's PRICE, refused when more than 3% (a wrong market).
inline bool offsetFor(float chartClose, float filePx, float& off)
{
    off = 0;
    if (!(chartClose > 0) || !(filePx > 0)) return false;
    float d = chartClose - filePx;
    if (std::fabs(d) > 0.03f * filePx) return false;
    off = d; return true;
}

// Profile scale: the largest |gamma| drawn (on arrival, whisker top, snapshot) and the largest |delta|
inline void scales(const Data& D, float& gmax, float& dmax)
{
    gmax = 1e-6f; dmax = 1e-6f;
    for (size_t i = 0; i < D.nodes.size(); i++) {
        const Node& n = D.nodes[i];
        float a = std::fabs(n.g); if (std::fabs(n.hi) > a) a = std::fabs(n.hi); if (std::fabs(n.lo) > a) a = std::fabs(n.lo); if (std::fabs(n.snap) > a) a = std::fabs(n.snap);
        if (a > gmax) gmax = a;
        if (std::fabs(n.d) > dmax) dmax = std::fabs(n.d);
    }
}
inline int barLen(float v, float vmax, int width) { if (vmax <= 0) return 0; float r = std::fabs(v) / vmax; if (r > 1) r = 1; return (int)(r * width + 0.5f); }

// Text inside a node only when the node is long enough to hold it
inline bool fits(int textW, int nodeLen, int pad = 8) { return nodeLen >= textW + pad; }

// Node labels: "4215 +33 (29-56)" on the gamma node, "SELL 82" on the delta node
inline std::string strikeTxt(float k) { char b[32]; if (std::fabs(k - std::floor(k + 0.5f)) < 1e-4f) snprintf(b, sizeof(b), "%d", (int)std::floor(k + 0.5f)); else snprintf(b, sizeof(b), "%g", k); return b; }
inline std::string gammaLabel(const Node& n)
{
    char b[80]; snprintf(b, sizeof(b), "%s  %+.0f  (%+.0f..%+.0f)", strikeTxt(n.k).c_str(), n.g, n.lo, n.hi); return b;
}
inline std::string deltaLabel(const Node& n) { char b[32]; snprintf(b, sizeof(b), "%s %.0f", n.d > 0 ? "BUY" : "SELL", std::fabs(n.d)); return b; }

// Price text with thousands separators, decimals by market (HG / EU 4, NG 3, else 2; a whole strike has none)
inline std::string fmtPx(float v, const std::string& m, bool strike = false)
{
    int dec = (m == "HG" || m == "EU") ? 4 : (m == "NG" ? 3 : 2);
    if (strike && dec == 2 && std::fabs(v - std::floor(v + 0.5f)) < 1e-3f) dec = 0;
    char b[48]; snprintf(b, sizeof(b), "%.*f", dec, (double)v);
    std::string s = b, ip = s, fp;
    size_t dot = s.find('.'); if (dot != std::string::npos) { ip = s.substr(0, dot); fp = s.substr(dot); }
    bool neg = !ip.empty() && ip[0] == '-'; if (neg) ip = ip.substr(1);
    std::string o; int c = 0;
    for (int i = (int)ip.size() - 1; i >= 0; i--) { o.insert(o.begin(), ip[(size_t)i]); if (++c % 3 == 0 && i > 0) o.insert(o.begin(), ','); }
    return (neg ? "-" : "") + o + fp;
}

// Word-wrap into lines no wider than maxW, using a width function (the plugin passes getTextWidth)
template <class W> inline std::vector<std::string> wrap(const std::string& text, int maxW, W width)
{
    std::vector<std::string> out; std::string line, word; std::stringstream ss(text);
    while (ss >> word) {
        std::string t = line.empty() ? word : line + " " + word;
        if (!line.empty() && width(t) > maxW) { out.push_back(line); line = word; } else line = t;
    }
    if (!line.empty()) out.push_back(line);
    return out;
}

inline const char* phaseName(int i) { static const char* P[] = { "APPROACH", "SWEEP", "RECLAIM", "RETEST", "TRADE" }; return (i >= 0 && i < 5) ? P[i] : ""; }

}  // namespace dl
