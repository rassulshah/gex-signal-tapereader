#ifndef LS_LEVELS_COLUMN_H
#define LS_LEVELS_COLUMN_H
// LevelsColumn.h 1.1.0 (1.1.0: attach reachAtr (0.5 ATR), rank top 10) 1.0.0 (2026-10-27, Rassul approved 13:30-13:39 CT, mockup levels_final.png "i think this is a cleaner approach. build")
// SDK-free logic of lsDealerProfile 2.28.0's combined LEVELS + LIQUIDITY column (tested by tests_lvc/test_levelscolumn.cpp):
//   one row per price: [rank] [liquidity: the big limit orders at the level, "800 + 50"] [level chips] [price] [odds]
//   * every level type in one column: key price levels, volume levels, gamma levels (+ big unnamed gamma strikes "GEX")
//   * chips ALWAYS in his order (13:37 CT "always show key price level first, then volume level and then gamma level")
//   * levels within max(2 ticks, 0.3 ATR) share one row (key first)
//   * the liquidity = lsLiquidityProfile 0.60.0's live order lines tagged to a level (lsFlexLevels\LRA-LmtLevels-<MKT>-<spb>.txt);
//     orders at the same level share one bar ("52 + 54"); the RANK (13:30 "lets remove it [the ratio] and how can you show rank instead",
//     13:31 "put the rank in front") = 1 for the row with the most resting lots of the rows on screen, a tie to the row nearer price
// No ratio, no dealers column (13:38 "lets just get rid of it"), no hovers.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include "LineLevelTag.h"

namespace lvc {
static const char* const VERSION = "1.1.0";   // (1.1.0, 2026-10-27 14:55-14:58 CT Rassul "rank up to top 10 liquidity levels", "liquidity close to the level" -> 0.5 ATR) attach reach + top-10 rank
static const double TAG_ATR = 0;          // (15:4x CT Rassul chose HIS rule over 0.5 ATR) 0 = no extra reach: the order's level comes from lsLiquidityProfile's LineLevelTag - ONE-SIDED, from the level out to 1 ATR BEYOND it (PDH up to PDH + 1 ATR, PDL down to PDL - 1 ATR), never inside
static const int MAX_RANK = 10;           // ranks 1-10; an 11th+ row keeps its bar in the column but no number
enum Fam { KEY = 0, VOLUME = 1, GAMMA = 2 };
static const double MERGE_ATR = 0.3;      // PARAM (mockup: "about 1 point in ES")
static const double MERGE_TICKS = 2.0;    // PARAM (floor)
static const long long LMT_STALE_S = 20;  // the order file older than this = no liquidity (the LP chart closed / stopped)

struct In {                 // one level as the Dealer Profile found it (chart prices)
    double px = 0; std::string name; int src = 0;   // src 0 = key / volume levels (chart), 1 = MenthorQ, 2 = big unnamed gamma strike
    unsigned long col = 0; bool pick = false, stale = false;
    size_t idx = 0;         // back to the caller's own record (its odds, hover, ...)
};
struct Chip { int fam = KEY; std::string text; unsigned long col = 0; size_t member = 0; };
struct Order { bool bid = true; long long lots = 0; std::string label, level; double levelPx = 0, px = 0; };
struct Row {
    double px = 0;                        // the row's price = its first chip's level (key first)
    std::vector<size_t> members;          // indexes into the In vector
    std::vector<Chip> chips;              // KEY chips, then VOLUME, then GAMMA
    std::string liq; long long lots = 0; bool bid = true; int rank = 0;
};

inline std::vector<std::string> tokens(const std::string& name)
{   // "ONH/LonHI" -> ONH, LonHI; "ONH \xB7 AsiaHI" (the key-level merge dot) / " / " / "+" separators too
    std::vector<std::string> out; std::string cur;
    auto flush = [&]() { std::string t; for (char c : cur) if (c != ' ') t += c; if (!t.empty()) out.push_back(t); cur.clear(); };
    for (size_t i = 0; i < name.size(); ++i) {
        const unsigned char c = (unsigned char)name[i];
        if (c == '/' || c == '+' || c == 0xB7) { flush(); continue; }
        if (c == 0xC2 && i + 1 < name.size() && (unsigned char)name[i + 1] == 0xB7) { flush(); ++i; continue; }   // UTF-8 middle dot
        cur += name[i];
    }
    flush();
    return out;
}
inline int famOf(const std::string& token, int src)
{
    if (src != 0) return GAMMA;                                  // MenthorQ / a big gamma strike
    const llt::Kind k = llt::kindOf(token);
    return k == llt::VOLUME ? VOLUME : k == llt::GAMMA ? GAMMA : KEY;
}
inline double mergeTol(double tick, double atr)
{
    const double a = (std::isfinite(atr) && atr > 0) ? MERGE_ATR * atr : 0.0;
    return std::max(MERGE_TICKS * (tick > 0 ? tick : 0.0), a);
}
// rows, top (highest price) first; neighbours within tol (of the row's first member) join it
inline std::vector<Row> group(const std::vector<In>& in, double tick, double atr)
{
    std::vector<size_t> ord; for (size_t i = 0; i < in.size(); ++i) if (std::isfinite(in[i].px) && in[i].px > 0 && !in[i].name.empty()) ord.push_back(i);
    std::stable_sort(ord.begin(), ord.end(), [&](size_t a, size_t b) { return in[a].px > in[b].px; });
    const double tol = mergeTol(tick, atr) + (tick > 0 ? tick * 1e-6 : 1e-9);
    std::vector<Row> rows;
    for (size_t k = 0; k < ord.size(); ++k) {
        const size_t i = ord[k];
        if (!rows.empty() && std::fabs(in[rows.back().members.front()].px - in[i].px) <= tol) rows.back().members.push_back(i);
        else { Row r; r.members.push_back(i); rows.push_back(r); }
    }
    for (size_t r = 0; r < rows.size(); ++r) {
        Row& R = rows[r];
        std::vector<std::string> seen;
        for (int fam = KEY; fam <= GAMMA; ++fam)
            for (size_t m = 0; m < R.members.size(); ++m) {
                const In& L = in[R.members[m]];
                std::string txt;                                  // this member's tokens of this family, kept together ("ONH/LonHI")
                for (const std::string& t : tokens(L.name)) {
                    if (famOf(t, L.src) != fam) continue;
                    if (std::find(seen.begin(), seen.end(), t) != seen.end()) continue;   // the same name from two sources = once
                    seen.push_back(t); txt += (txt.empty() ? "" : "/") + t;
                }
                if (txt.empty()) continue;
                Chip c; c.fam = fam; c.text = txt; c.col = L.col; c.member = R.members[m]; R.chips.push_back(c);
            }
        R.px = R.chips.empty() ? in[R.members.front()].px : in[R.chips.front().member].px;
    }
    rows.erase(std::remove_if(rows.begin(), rows.end(), [](const Row& R) { return R.chips.empty(); }), rows.end());
    return rows;
}
// the order file: "LMTL,1,<unix>,<tick>" then "O,<B|A>,<lots>,<label>,<level>,<levelPx>,<orderPx>" (lsLiquidityProfile 0.60.0)
inline bool parseOrders(const std::string& text, long long nowUnix, std::vector<Order>& out, std::string& why)
{
    out.clear(); why.clear();
    size_t p = 0; bool head = false;
    while (p < text.size()) {
        size_t e = text.find('\n', p); if (e == std::string::npos) e = text.size();
        std::string ln = text.substr(p, e - p); p = e + 1;
        while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
        if (ln.empty()) continue;
        std::vector<std::string> f; { std::string c; for (char ch : ln) { if (ch == ',') { f.push_back(c); c.clear(); } else c += ch; } f.push_back(c); }
        if (!head) {
            if (f.size() < 4 || f[0] != "LMTL" || f[1] != "1") { why = "bad header"; return false; }
            const long long at = std::atoll(f[2].c_str());
            if (at <= 0 || (nowUnix > 0 && (nowUnix - at > LMT_STALE_S || at - nowUnix > 60))) { why = "stale"; return false; }
            head = true; continue;
        }
        if (f.size() != 7 || f[0] != "O" || (f[1] != "B" && f[1] != "A")) continue;   // a torn / foreign line is skipped
        Order o; o.bid = f[1] == "B"; o.lots = std::atoll(f[2].c_str()); o.label = f[3]; o.level = f[4];
        o.levelPx = std::atof(f[5].c_str()); o.px = std::atof(f[6].c_str());
        if (o.lots <= 0 || o.label.empty() || o.label.size() > 24 || !(o.levelPx > 0) || !(o.px > 0)) continue;
        out.push_back(o);
    }
    if (!head) { why = "empty"; return false; }
    return true;
}
// each order joins the row holding its level (same price on the tick grid, else its level's name, else within the merge tolerance)
inline void attach(std::vector<Row>& rows, const std::vector<In>& in, const std::vector<Order>& ords, double tick, double atr, double lastPx, double reachAtr = 0)
{
    const double half = (tick > 0 ? tick : 0.01) * 0.5, tol = mergeTol(tick, atr);
    // (1.1.0) reachAtr > 0: the order must sit within reachAtr x ATR (at least 2 ticks) of the level it is joined to
    const double reach = reachAtr > 0 ? std::max(2.0 * (tick > 0 ? tick : 0.0), reachAtr * ((std::isfinite(atr) && atr > 0) ? atr : 0.0)) + (tick > 0 ? tick * 1e-6 : 1e-9) : 1e300;
    std::vector<std::vector<size_t> > by(rows.size());
    for (size_t o = 0; o < ords.size(); ++o) {
        if (reachAtr > 0 && std::fabs(ords[o].px - ords[o].levelPx) > reach) continue;
        int best = -1; double bestD = 1e300;
        for (size_t r = 0; r < rows.size() && best < 0; ++r)
            for (size_t m : rows[r].members) if (std::fabs(in[m].px - ords[o].levelPx) <= half) { best = (int)r; break; }
        for (size_t r = 0; r < rows.size() && best < 0; ++r)
            for (size_t m : rows[r].members) for (const std::string& t : tokens(in[m].name)) if (t == ords[o].level) { best = (int)r; break; }
        if (best < 0) for (size_t r = 0; r < rows.size(); ++r) { const double d = std::fabs(rows[r].px - ords[o].levelPx); if (d <= tol && d < bestD) { bestD = d; best = (int)r; } }
        if (best >= 0) by[(size_t)best].push_back(o);
    }
    for (size_t r = 0; r < rows.size(); ++r) {
        Row& R = rows[r]; R.liq.clear(); R.lots = 0; R.rank = 0;
        if (by[r].empty()) continue;
        std::vector<size_t> v = by[r];
        std::sort(v.begin(), v.end(), [&](size_t a, size_t b) { return ords[a].lots > ords[b].lots; });   // the biggest first ("800 + 50")
        for (size_t k = 0; k < v.size(); ++k) { R.liq += (k ? " + " : "") + ords[v[k]].label; R.lots += ords[v[k]].lots; }
        long long lb = 0, la = 0; for (size_t k : v) (ords[k].bid ? lb : la) += ords[k].lots;
        R.bid = lb >= la;
    }
    (void)lastPx;
}
// rank 1 = the most lots among the rows given (the caller passes the rows on screen); a tie goes to the row nearer price
inline void rank(std::vector<Row>& rows, const std::vector<size_t>& onScreen, double lastPx)
{
    std::vector<size_t> v; for (size_t r : onScreen) if (r < rows.size() && rows[r].lots > 0) v.push_back(r);
    std::stable_sort(v.begin(), v.end(), [&](size_t a, size_t b) {
        if (rows[a].lots != rows[b].lots) return rows[a].lots > rows[b].lots;
        return std::fabs(rows[a].px - lastPx) < std::fabs(rows[b].px - lastPx); });
    for (size_t r = 0; r < rows.size(); ++r) rows[r].rank = 0;
    for (size_t k = 0; k < v.size() && (int)k < MAX_RANK; ++k) rows[v[k]].rank = (int)k + 1;   // (1.1.0) top 10 only
}
// the nearest level off screen above / below ("^ 1DH 7,783.75 (+30.00)")
inline std::string offText(const std::string& name, const std::string& pxTxt, double dist, int dec)
{
    char d[32]; std::snprintf(d, sizeof(d), "%s%.*f", dist >= 0 ? "+" : "-", dec < 0 ? 2 : dec, std::fabs(dist));
    return name + " " + pxTxt + "  (" + d + ")";
}
}  // namespace lvc
#endif
