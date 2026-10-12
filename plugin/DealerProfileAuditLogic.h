/********************************************************************************
 * DealerProfileAuditLogic.h -- small, SDK-free boundaries owned by DealerProfile.
 *
 * This is deliberately not part of DealerLogic.h: MenthorQ/Touch/Levels inputs and
 * export temporary naming are used only by DealerProfile.
 ********************************************************************************/
#pragma once
#include <sstream>

#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include <ctime>
#include "DealerLogic.h"

namespace dpaudit {

inline bool finite(float v) { return std::isfinite((double)v) != 0; }
inline bool finite(double v) { return std::isfinite(v) != 0; }

// CSV fields may have surrounding whitespace, but no partial, non-finite,
// underflowed/overflowed, zero, or negative price is meaningful to drawing.
inline bool parsePositiveFinite(const std::string& text, float& out)
{
    const char* first = text.c_str();
    while (*first && std::isspace((unsigned char)*first)) ++first;
    if (!*first) return false;
    errno = 0;
    char* end = 0;
    const float value = std::strtof(first, &end);
    if (end == first || errno == ERANGE || !finite(value)) return false;
    while (*end && std::isspace((unsigned char)*end)) ++end;
    if (*end || value <= 0.0f) return false;
    out = value;
    return true;
}

// User data is scoped by the IRT host to the current chart. Its identity keeps
// two simultaneous chart exports from sharing one fixed ".tmp" pathname.
inline std::string exportTempPath(const std::string& target, std::uintptr_t stateIdentity)
{
    char suffix[5 + sizeof(std::uintptr_t) * 2 + 1] = {0};
    std::snprintf(suffix, sizeof(suffix), ".tmp.%llX", static_cast<unsigned long long>(stateIdentity));
    return target + suffix;
}


// ---- (2.7.4) on-chart label placement --------------------------------------------------------------------------------
// A box already on the chart: another label, or a price bar's high-low range at its x (so a label never covers price).
struct LblBox { short l, t, r, b; bool bar = false; };   // bar = a price bar's range (yields to badges when nothing is free)
inline short placeLabelPass(short want, short w, short t, short b, const std::vector<LblBox>& taken, short minL, short maxR,
                            bool withBars, bool& ok)
{
    std::vector<LblBox> row;
    for (size_t i = 0; i < taken.size(); ++i)
        if ((withBars || !taken[i].bar) && !(b <= taken[i].t || t >= taken[i].b)) row.push_back(taken[i]);
    auto clash = [&](int l) {
        for (size_t i = 0; i < row.size(); ++i) if (!(l + w + 3 <= row[i].l || l >= row[i].r + 3)) return true;   // 3 px gap
        return false;
    };
    auto fits = [&](int l) { return l >= minL && l + w <= maxR; };
    int c0 = want;
    if (c0 + w > maxR) c0 = maxR - w;
    if (c0 < minL) c0 = minL;
    ok = true;
    if (fits(c0) && !clash(c0)) return (short)c0;
    int bestR = 1 << 30, bestL = -(1 << 30);
    for (size_t i = 0; i < row.size(); ++i) {
        int r = row[i].r + 6, l = row[i].l - 6 - w;
        if (r >= c0 && r < bestR && fits(r) && !clash(r)) bestR = r;
        if (l <= c0 && l > bestL && fits(l) && !clash(l)) bestL = l;
    }
    if (bestR != (1 << 30)) return (short)bestR;
    if (bestL != -(1 << 30)) return (short)bestL;
    ok = false;
    return (short)c0;
}
// The label (width w, rows t..b) goes at `want` (centred on the chart) when that spot is free; otherwise beside the nearest
// box on its row - right first (2.7.3), then left - never left of minL (the Liquidity Profile column) or past maxR (the
// Delta / Dealer Profiles). Price bars are avoided too; when price fills the whole row the badge still never covers another
// badge, and only when even that is impossible does it stay at `want` (clamped).
inline short placeLabel(short want, short w, short t, short b, const std::vector<LblBox>& taken, short minL, short maxR)
{
    bool ok = false;
    short l = placeLabelPass(want, w, t, b, taken, minL, maxR, true, ok);
    if (ok) return l;
    return placeLabelPass(want, w, t, b, taken, minL, maxR, false, ok);
}

// (2.8.1, Rassul 11:18 "the levels move like the LonLO moves left then back to center then left again") a badge keeps the x it
// was drawn at while that spot stays free (inside the bounds, clear of every other badge and price bar); it only moves when
// something now covers it - and then stays at its new spot (never "back to the centre" on its own)
inline bool spotFree(short l, short w, short t, short b, const std::vector<LblBox>& taken, short minL, short maxR)
{
    if (l < minL || l + w > maxR) return false;
    for (size_t i = 0; i < taken.size(); ++i) {
        if (b <= taken[i].t || t >= taken[i].b) continue;
        if (!(l + w + 3 <= taken[i].l || l >= taken[i].r + 3)) return false;
    }
    return true;
}

// ---- (2.7.4) price text: the same decimals for the MenthorQ and the key-level badges ------------------------------------
inline int priceDecimals(const std::string& m) { return m == "GC" ? 1 : m == "HG" ? 4 : m == "EU" ? 5 : m == "NG" ? 3 : 2; }
inline std::string priceText(const std::string& m, double px)
{
    char b[48]; std::snprintf(b, sizeof(b), "%.*f", priceDecimals(m), px); return b;
}

// every level badge reads "<price> <NAME>  <odds>" (Rassul 2026-10-12: the key-level badges had no price), e.g.
// "4221.7 PDH  2%  /  4%?" - the price in the MenthorQ badge's format for the market, the name drawn bold
struct Badge { std::string price, name, odds; std::string all() const { return price + name + odds; } };
inline Badge badge(const std::string& m, double px, const std::string& name, const std::string& odds)
{
    Badge b; b.price = priceText(m, px) + " "; b.name = name; b.odds = odds.empty() ? "" : "  " + odds; return b;
}

// ---- (2.8.0, Rassul 2026-10-12 10:16 "modify the level badges to indicate if there is liquidity at those levels within a zone",
// 10:19 "even if you have CR or other gamma levels indicate Liq", 10:23 "also add the number of contracts")
// LRA-LiqZones-<MKT>.csv (lsLiquidityProfile 0.12.0+): "LIQZ,1,<unix>,<tick>" then "Z,<S|R>,<lo>,<hi>,<bx>,<contracts>".
// A level gets "Liq 977 (2.6x)" when a zone covers it (the zone +/- half its width) at 2.0x+ the book around it - the Liquidity
// Profile's own line - kept down to 1.7x once shown so it does not blink; green = resting bids (S), red = resting offers (R).
// The file is ignored when it is more than 20 s old (the Liquidity Profile is not running / no depth): no tags then.
struct LiqZ { bool bid = true; double lo = 0, hi = 0, bx = 0; long long n = 0; };
// (2.8.3) the Liquidity Profile 0.13+ publishes only the zones it draws (shown at 2.0x, kept to 1.7x): every one of them is tagged
static const double LIQ_SHOW_X = 1.7, LIQ_KEEP_X = 1.7; static const long long LIQ_STALE_S = 20;
inline bool parseLiqZones(const std::string& text, long long nowUnix, std::vector<LiqZ>& out, std::string& why, int* colRight = nullptr)
{
    out.clear(); if (colRight) *colRight = 0;
    std::stringstream ss(text); std::string ln; bool head = false;
    while (std::getline(ss, ln)) {
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        std::vector<std::string> c; std::stringstream ls(ln); std::string x;
        while (std::getline(ls, x, ',')) c.push_back(x);
        if (!head) {
            if (c.size() < 4 || c[0] != "LIQZ" || c[1] != "1") { why = "no LIQZ header"; return false; }
            char* e = nullptr; long long at = std::strtoll(c[2].c_str(), &e, 10);
            if (!e || *e || at <= 0) { why = "bad stamp"; return false; }
            if (nowUnix - at > LIQ_STALE_S || at - nowUnix > LIQ_STALE_S) { why = "stale"; return false; }
            if (colRight && c.size() >= 5) { char* e5 = nullptr; long v = std::strtol(c[4].c_str(), &e5, 10); if (e5 && !*e5 && v > 0 && v < 1200) *colRight = (int)v; }   // (2.8.6)
            head = true; continue;
        }
        if (c.size() < 6 || c[0] != "Z" || (c[1] != "S" && c[1] != "R")) continue;
        LiqZ z; z.bid = c[1] == "S"; char* e1 = nullptr; char* e2 = nullptr; char* e3 = nullptr; char* e4 = nullptr;
        z.lo = std::strtod(c[2].c_str(), &e1); z.hi = std::strtod(c[3].c_str(), &e2); z.bx = std::strtod(c[4].c_str(), &e3); z.n = std::strtoll(c[5].c_str(), &e4, 10);
        if (*e1 || *e2 || *e3 || *e4 || !std::isfinite(z.lo) || !std::isfinite(z.hi) || !std::isfinite(z.bx) || z.hi < z.lo || z.lo <= 0 || z.bx <= 0 || z.n < 0) continue;
        out.push_back(z);
    }
    if (!head) { why = "empty"; return false; }
    why.clear(); return true;
}
// the strongest zone that covers the level; null when none reaches the line (2.0x, or 1.7x when this level showed one last time)
inline const LiqZ* liqAt(const std::vector<LiqZ>& Z, double px, bool wasShown)
{
    // (2.8.5, Rassul 13:27 "they dont match the current liquidity") the zone that HOLDS the level's price, else the nearest within
    // half a band (inside first, then the nearer centre) - the Liquidity Profile 0.13.1 names a level on that same one zone
    const LiqZ* best = nullptr; double bestD = 0;
    for (size_t i = 0; i < Z.size(); i++) {
        const double w = Z[i].hi - Z[i].lo, pad = w / 2;
        if (px < Z[i].lo - pad - 1e-9 || px > Z[i].hi + pad + 1e-9) continue;
        const double out = px < Z[i].lo ? Z[i].lo - px : px > Z[i].hi ? px - Z[i].hi : 0.0;
        const double d = out * 1000.0 + std::fabs(2 * px - Z[i].lo - Z[i].hi);
        if (!best || d < bestD - 1e-12) { best = &Z[i]; bestD = d; }
    }
    if (!best || best->bx < (wasShown ? LIQ_KEEP_X : LIQ_SHOW_X)) return nullptr;
    return best;
}
inline std::string liqTag(const LiqZ& z)
{
    char b[64]; std::snprintf(b, sizeof(b), "  Liq %lld (%.1fx)", z.n, z.bx); return b;
}

// ---- (2.8.0, Rassul 10:29 "add just the levels to the hourly chart ... use our levels instead") the key levels a 3-min chart
// computed, shared with charts of 15-min+ bars (whose bars cannot split 8:30 from 8:00): LRA-KeyLevels-<MKT>.csv =
// "KEYLV,1,<unix>" then "L|<name>|<price>|<pivot 0/1>". A reader takes it while it is under 18 h old (written every new bar).
struct KeyLv { std::string name; double price = 0; bool pivot = false; };
inline std::string keyLevelsText(const std::vector<KeyLv>& v, long long nowUnix)
{
    std::string o = "KEYLV,1," + std::to_string(nowUnix) + "\n"; char b[160];
    for (size_t i = 0; i < v.size(); i++) { std::snprintf(b, sizeof(b), "L|%s|%.6f|%d\n", v[i].name.c_str(), v[i].price, v[i].pivot ? 1 : 0); o += b; }
    return o;
}
inline bool parseKeyLevels(const std::string& text, long long nowUnix, std::vector<KeyLv>& out)
{
    out.clear(); std::stringstream ss(text); std::string ln; bool head = false;
    while (std::getline(ss, ln)) {
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        if (!head) {
            if (ln.compare(0, 8, "KEYLV,1,") != 0) return false;
            char* e = nullptr; long long at = std::strtoll(ln.c_str() + 8, &e, 10);
            if (!e || *e || at <= 0 || nowUnix - at > 18 * 3600 || at - nowUnix > 600) return false;
            head = true; continue;
        }
        std::vector<std::string> c; std::stringstream ls(ln); std::string x; while (std::getline(ls, x, '|')) c.push_back(x);
        if (c.size() < 4 || c[0] != "L" || c[1].empty() || c[1].size() > 40) continue;
        char* e = nullptr; KeyLv k; k.name = c[1]; k.price = std::strtod(c[2].c_str(), &e);
        if (!e || *e || !std::isfinite(k.price) || k.price <= 0) continue;
        k.pivot = c[3] == "1"; out.push_back(k);
    }
    return head;
}
// the chart's bar size: the smallest positive gap between the last 40 stamps (0 = unknown)
inline int barSeconds(const std::vector<long long>& stamps)
{
    int per = 0;
    for (size_t i = stamps.size(); i-- > 1 && stamps.size() - i < 40;) { long long d = stamps[i] - stamps[i - 1]; if (d > 0 && (per == 0 || d < per)) per = (int)d; }
    return per;
}

// ---- (2.7.4) MenthorQ level colour: green = support (PS), red = resistance (CR), yellow = HVL, cyan = the other gamma levels
// 'G' green, 'R' red, 'Y' yellow, 'C' cyan - decided by the FIRST name of the label ("CR 0D / GW 0D" -> CR -> red)
inline char mqKind(const std::string& label)
{
    std::string f; size_t i = 0;
    while (i < label.size() && label[i] == ' ') ++i;
    if (label.compare(i, 3, "MQ ") == 0) i += 3;
    while (i < label.size() && label[i] != ' ' && label[i] != '/') f += (char)std::toupper((unsigned char)label[i++]);
    if (f == "PS" || f == "PUT" || f == "PUTSUPPORT") return 'G';
    if (f == "CR" || f == "CALL" || f == "CALLRESISTANCE") return 'R';
    if (f == "HVL") return 'Y';
    return 'C';
}
// level names never carry "MQ" (Rassul 2026-10-06): "MQ CR 0D" -> "CR 0D"
inline std::string stripMQ(const std::string& label)
{
    std::string r = label;
    for (size_t p = r.find("MQ "); p != std::string::npos; p = r.find("MQ ", p))
        if (p == 0 || r[p - 1] == ' ' || r[p - 1] == '/') r.erase(p, 3); else p += 3;
    while (!r.empty() && r[0] == ' ') r.erase(0, 1);
    return r;
}
// text in a dark line colour (e.g. his pure blue PW) is lifted toward white so it reads on the black chart; same hue
inline unsigned long liftForText(unsigned long c)
{
    int r = (int)((c >> 16) & 0xFF), g = (int)((c >> 8) & 0xFF), b = (int)(c & 0xFF);
    double lum = 0.299 * r + 0.587 * g + 0.114 * b;
    if (lum >= 120.0) return c & 0x00FFFFFFUL;
    double t = 0.45;
    r = (int)(r + (255 - r) * t); g = (int)(g + (255 - g) * t); b = (int)(b + (255 - b) * t);
    return ((unsigned long)r << 16) | ((unsigned long)g << 8) | (unsigned long)b;
}

// ---- (2.7.4) which MenthorQLevels.csv symbol a chart uses -------------------------------------------------------------
// 1 the chart's own symbol; 2 a dated chart: the same contract under another feed's spelling (ESZ6 = EPZ26), else (a roll)
// the dealer file's contract, else the nearest expiry; 3 a continuous chart: the dealer file's contract, else the nearest
// expiry (before 2.7.4 the alphabetically first one - EPH27 before EPZ26 - i.e. possibly the BACK month).
// shift = the levels belong to another contract than the chart: they are drawn at price + the contract offset.
inline std::string pickMQSymbol(const std::vector<std::string>& keys, const std::string& root, const std::string& mkt,
                                const std::string& fileSym, bool& shift)
{
    shift = false;
    for (size_t i = 0; i < keys.size(); ++i) if (keys[i] == root) return keys[i];
    std::vector<std::string> c;
    for (size_t i = 0; i < keys.size(); ++i) if (!mkt.empty() && dl::marketForRoot(keys[i]) == mkt) c.push_back(keys[i]);
    if (c.empty()) return "";
    char m = 0; int y = 0;
    if (dl::monthYear(root, m, y)) for (size_t i = 0; i < c.size(); ++i) if (dl::sameContract(root, c[i])) return c[i];
    std::string pick;
    for (size_t i = 0; i < c.size(); ++i) if (c[i] == fileSym) pick = c[i];
    if (pick.empty()) {
        static const char* MON = "FGHJKMNQUVXZ";
        int best = 1 << 30;
        for (size_t i = 0; i < c.size(); ++i) {
            char mm = 0; int yy = 0; int k = 1 << 29;
            if (dl::monthYear(c[i], mm, yy)) k = yy * 12 + (int)(std::strchr(MON, mm) - MON);
            if (k < best) { best = k; pick = c[i]; }
        }
    }
    shift = !dl::sameContract(root, pick);
    return pick;
}

// ---- (2.7.4) the status file is rewritten only when its content changes (or once a minute), not on every repaint --------
struct WriteGate {
    std::string last; std::time_t at = 0;
    bool due(const std::string& content, std::time_t now)
    {
        if (content == last && now - at < 60) return false;
        last = content; at = now; return true;
    }
};

} // namespace dpaudit
