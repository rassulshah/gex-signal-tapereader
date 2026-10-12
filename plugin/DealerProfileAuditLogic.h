/********************************************************************************
 * DealerProfileAuditLogic.h -- small, SDK-free boundaries owned by DealerProfile.
 *
 * This is deliberately not part of DealerLogic.h: MenthorQ/Touch/Levels inputs and
 * export temporary naming are used only by DealerProfile.
 ********************************************************************************/
#pragma once

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
