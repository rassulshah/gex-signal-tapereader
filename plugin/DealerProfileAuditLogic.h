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
#include <climits>
#include <cmath>
#include <ctime>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <ctime>
#include "DealerLogic.h"

namespace dpaudit {

inline bool finite(float v) { return std::isfinite((double)v) != 0; }
inline bool finite(double v) { return std::isfinite(v) != 0; }

inline bool parseInt(const std::string& text, int& out)
{
    const char* first = text.c_str();
    while (*first && std::isspace((unsigned char)*first)) ++first;
    if (!*first) return false;
    errno = 0;
    char* end = 0;
    const long value = std::strtol(first, &end, 10);
    if (end == first || errno == ERANGE || value < INT_MIN || value > INT_MAX) return false;
    while (*end && std::isspace((unsigned char)*end)) ++end;
    if (*end) return false;
    out = (int)value;
    return true;
}

// Price equality from independently written CSV files must use the instrument's
// native tick grid. A relative epsilon can merge distinct high-price NQ levels.
inline bool sameTick(float a, float b, double tick)
{
    if (!finite(a) || !finite(b) || !finite(tick) || !(tick > 0.0)) return false;
    const double ta = (double)a / tick, tb = (double)b / tick;
    if (!finite(ta) || !finite(tb) || std::fabs(ta) > (double)LLONG_MAX - 0.5 || std::fabs(tb) > (double)LLONG_MAX - 0.5) return false;
    return std::llround(ta) == std::llround(tb);
}

enum BoundedRead { READ_OK, READ_OPEN_FAILED, READ_TOO_LARGE, READ_IO_ERROR };

// Read complete lines without allocating in proportion to an untrusted file.
// Callers retain their previously parsed snapshot unless this returns READ_OK.
inline BoundedRead readBoundedLines(const std::string& path, std::vector<std::string>& lines,
                                    size_t maxBytes = 256 * 1024, size_t maxLines = 4096, size_t maxLineBytes = 4096)
{
    lines.clear();
    if (maxBytes == 0 || maxLines == 0 || maxLineBytes == 0) return READ_TOO_LARGE;
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in.is_open()) return READ_OPEN_FAILED;
    std::string line;
    char ch = 0;
    size_t bytes = 0;
    while (in.get(ch)) {
        if (bytes >= maxBytes) { lines.clear(); return READ_TOO_LARGE; }
        ++bytes;
        if (ch == '\n') {
            if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
            if (lines.size() >= maxLines) { lines.clear(); return READ_TOO_LARGE; }
            lines.push_back(line);
            line.clear();
        } else {
            if (line.size() >= maxLineBytes) { lines.clear(); return READ_TOO_LARGE; }
            line += ch;
        }
    }
    if (in.bad()) { lines.clear(); return READ_IO_ERROR; }
    if (!line.empty()) {
        if (lines.size() >= maxLines) { lines.clear(); return READ_TOO_LARGE; }
        lines.push_back(line);
    }
    return READ_OK;
}

// Status publication is a cache: write when content changes, at startup, or
// after a bounded heartbeat interval; a clock rollback also forces a refresh.
inline bool publicationDue(const std::string& next, const std::string& previous,
                           std::time_t now, std::time_t last, std::time_t interval = 30)
{
    return next != previous || last <= 0 || now < last || now - last >= interval;
}

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


// =====================================================================================================================
// (2.9.0, Rassul approved 2026-10-13 18:05 - mockup dealer-profile-mockup.png) the Profile's node detail, header, stale flag,
// HVL regime tags, the MenthorQ level clean-up and the hovers. SDK-free and tested (test_dealerprofile_290.cpp).
// =====================================================================================================================
// NODEX|<strike>|<absolute GEX $/pt>|<% change since the session's first grid or ''>|<hedge contracts, + = BUY or ''>|<hedge $ or ''>
struct NodeX { float k = 0; double absG = 0; bool hasChg = false; int chg = 0; bool hasD = false; double d = 0, dUsd = 0; };
inline bool parseStrictDouble(const std::string& s, double& out)
{
    if (s.empty() || s.size() > 40) return false;
    const char* b = s.c_str(); char* e = nullptr; errno = 0;
    const double v = std::strtod(b, &e);
    if (errno == ERANGE || e == b || *e != '\0' || !std::isfinite(v)) return false;
    out = v; return true;
}
inline bool parseNodeX(const std::string& line, NodeX& n)
{
    std::vector<std::string> t = dl::split(line);
    if (t.size() < 6 || t[0] != "NODEX") return false;
    NodeX x; double k = 0;
    if (!parseStrictDouble(t[1], k) || k <= 0 || k > 1e9) return false;
    x.k = (float)k;
    if (!parseStrictDouble(t[2], x.absG) || x.absG < 0) return false;
    if (!t[3].empty()) { double c = 0; if (!parseStrictDouble(t[3], c) || c < -1e6 || c > 1e6) return false; x.hasChg = true; x.chg = (int)std::lround(c); }
    if (!t[4].empty()) { if (!parseStrictDouble(t[4], x.d)) return false; x.hasD = true; }
    if (!t[5].empty()) { if (!parseStrictDouble(t[5], x.dUsd)) return false; }
    n = x; return true;
}
// BOOKASOF|<CT sec-of-day>|<yyyy-mm-dd>: when MenthorQ's strike grid behind the file was taken ("BOOKASOF||" = none)
struct BookAsOf { int so = -1, y = 0, mo = 0, d = 0; bool ok() const { return so >= 0; } };
inline bool parseBookAsOf(const std::string& line, BookAsOf& b)
{
    std::vector<std::string> t = dl::split(line);
    if (t.size() < 2 || t[0] != "BOOKASOF") return false;
    BookAsOf r;
    if (!t[1].empty()) {
        int so = 0; if (!parseInt(t[1], so) || so < 0 || so >= 86400) return false;
        r.so = so;
        if (t.size() >= 3 && !t[2].empty()) { int a = 0, m = 0, d = 0; if (std::sscanf(t[2].c_str(), "%d-%d-%d", &a, &m, &d) != 3 || a < 2000 || m < 1 || m > 12 || d < 1 || d > 31) return false; r.y = a; r.mo = m; r.d = d; }
    }
    b = r; return true;
}
// "4:55 AM" - his 12-hour Central clock
inline std::string clock12(int so)
{
    if (so < 0) return "";
    so %= 86400; int h = so / 3600, m = (so / 60) % 60; const char* ap = h < 12 ? "AM" : "PM"; int h12 = h % 12 == 0 ? 12 : h % 12;
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", h12, m, ap); return b;
}
// minutes from a CT (date, sec-of-day) to now (local wall clock = CT on his PC, + clock offset), -1 = unknown
inline double ageMinutes(const BookAsOf& a, const std::tm& nowLocal, int clockOffsetMin)
{
    if (!a.ok()) return -1;
    std::tm t0; std::memset(&t0, 0, sizeof(t0));
    if (a.y > 2000) { t0.tm_year = a.y - 1900; t0.tm_mon = a.mo - 1; t0.tm_mday = a.d; }
    else { t0.tm_year = nowLocal.tm_year; t0.tm_mon = nowLocal.tm_mon; t0.tm_mday = nowLocal.tm_mday; }
    t0.tm_hour = a.so / 3600; t0.tm_min = (a.so / 60) % 60; t0.tm_sec = a.so % 60; t0.tm_isdst = -1;
    std::tm n = nowLocal; n.tm_isdst = -1;
    const std::time_t A = std::mktime(&t0), N = std::mktime(&n);
    if (A == (std::time_t)-1 || N == (std::time_t)-1) return -1;
    double d = std::difftime(N, A) / 60.0 - clockOffsetMin;
    if (a.y <= 2000 && d < -60) d += 1440;                              // no date: yesterday's grid seen after midnight
    return d < 0 ? 0 : d;
}
// MenthorQ updates futures Sunday 21:40 -> Friday 16:00 CT, paused 16:00-21:40 every evening (menthorq._update_hours)
inline bool mqUpdateHours(int wday, int minuteOfDay)
{
    if (wday == 6 || (wday == 5 && minuteOfDay > 960) || (wday == 0 && minuteOfDay < 1300)) return false;
    return !(minuteOfDay > 960 && minuteOfDay < 1300);
}
static const double BOOK_STALE_MIN = 20.0;                              // the mockup: "older than 20 min" while MenthorQ is updating
inline bool bookStale(double ageMin, bool updateHours) { return updateHours && ageMin > BOOK_STALE_MIN; }
inline std::string ageShort(double m)
{
    char b[24];
    if (m < 0) return "";
    if (m >= 2880) std::snprintf(b, sizeof(b), "%dd", (int)(m / 1440 + 0.5));
    else if (m >= 90) std::snprintf(b, sizeof(b), "%dh", (int)(m / 60 + 0.5));
    else std::snprintf(b, sizeof(b), "%dm", (int)(m + 0.5));
    return b;
}
// the header's second line: "STALE 10h · 4:55 AM" when stale, else "book 9:55 AM" / "SPX book 9:55 AM" (text is ASCII: '-')
inline std::string bookLine(const BookAsOf& a, double ageMin, bool stale, const std::string& src, const std::string& mkt)
{
    if (!a.ok()) return stale ? "STALE" : "";
    const std::string t = clock12(a.so);
    if (stale) return "STALE " + ageShort(ageMin) + " - " + t;
    return (!src.empty() && src != mkt ? src + " book " : std::string("book ")) + t;
}
// node look: W = net long gamma (holds / pins), F = net short (accelerant); muted = big absolute, near-zero net (calls and puts cancel)
inline char wfLetter(double net) { return net >= 0 ? 'W' : 'F'; }
inline bool mutedNode(double absG, double net) { return absG <= 0 || std::fabs(net) < 0.25 * absG; }
static const double DELTA_SHOW_SHARE = 0.35;                            // hedge numbers only on strikes >= 35% of the biggest wall
inline bool showDelta(const NodeX& x, double absMax) { return x.hasD && std::fabs(x.d) >= 1.0 && absMax > 0 && x.absG >= DELTA_SHOW_SHARE * absMax; }
// "-599" red = dealers SELL, "599" green = dealers BUY; thousands as "5.3K", "-12.8K", "128K"
inline std::string deltaNum(double d)
{
    const double a = std::fabs(d); char b[24];
    if (a < 999.5) std::snprintf(b, sizeof(b), "%ld", std::lround(d));
    else if (a < 99950) std::snprintf(b, sizeof(b), "%s%.1fK", d < 0 ? "-" : "", a / 1000.0);
    else std::snprintf(b, sizeof(b), "%s%.0fK", d < 0 ? "-" : "", a / 1000.0);
    return b;
}
inline std::string pctChg(int c) { char b[16]; if (c > 0) std::snprintf(b, sizeof(b), "+%d%%", c); else std::snprintf(b, sizeof(b), "%d%%", c); return b; }
// "$27.2M" / "$2.1B" / "$950K"
inline std::string usdShort(double v)
{
    const double a = std::fabs(v); char b[24]; const char* s = v < 0 ? "-" : "";
    if (a >= 1e9) std::snprintf(b, sizeof(b), "%s$%.1fB", s, a / 1e9);
    else if (a >= 1e6) std::snprintf(b, sizeof(b), "%s$%.1fM", s, a / 1e6);
    else if (a >= 1e3) std::snprintf(b, sizeof(b), "%s$%.0fK", s, a / 1e3);
    else std::snprintf(b, sizeof(b), "%s$%.0f", s, a);
    return b;
}
// the 3 biggest absolute-GEX strikes among the given indexes (ties: the lower strike first) - they carry the % change
inline std::vector<size_t> top3(const std::vector<NodeX>& v, const std::vector<size_t>& idx)
{
    std::vector<size_t> o(idx);
    for (size_t i = 0; i < o.size(); ++i) for (size_t j = i + 1; j < o.size(); ++j)
        if (v[o[j]].absG > v[o[i]].absG || (v[o[j]].absG == v[o[i]].absG && v[o[j]].k < v[o[i]].k)) std::swap(o[i], o[j]);
    if (o.size() > 3) o.resize(3);
    return o;
}
// right-to-left placement of a node's label pieces between the bar's left end and the column's left limit: a piece that would
// cross the limit is dropped (with everything after it) - the labels never reach the Delta Profile / Liquidity columns
struct Piece { int w = 0; int x = 0; bool shown = false; };
inline void placePieces(std::vector<Piece>& P, int rightX, int leftLimit, int gap)
{
    int x = rightX; bool stop = false;
    for (size_t i = 0; i < P.size(); ++i) {
        if (P[i].w <= 0) { P[i].shown = false; continue; }
        if (stop || x - P[i].w < leftLimit) { P[i].shown = false; stop = true; continue; }
        P[i].x = x - P[i].w; P[i].shown = true; x = P[i].x - gap;
    }
}
// ---- MenthorQ level clean-up: "CR 0D / GW 0D" (one line) -> "CR0"; G2-G10 hidden unless they sit on a key level
inline std::string mqShowName(const std::string& name0)
{
    std::string name = name0, suffix;
    const size_t st = name.find(" STALE");
    if (st != std::string::npos) { suffix = name.substr(st); name = name.substr(0, st); }
    if (name.find("CR 0D") != std::string::npos && name.find("GW 0D") != std::string::npos) return "CR0" + suffix;
    return name0;
}
inline bool mqHiddenGex(const std::string& name0)
{
    std::string n = name0; const size_t st = n.find(" STALE"); if (st != std::string::npos) n = n.substr(0, st);
    while (!n.empty() && n[n.size() - 1] == ' ') n.erase(n.size() - 1);
    if (n.size() < 2 || n[0] != 'G' || n.find('/') != std::string::npos) return false;
    for (size_t i = 1; i < n.size(); ++i) if (!std::isdigit((unsigned char)n[i])) return false;
    const int g = std::atoi(n.c_str() + 1);
    return g >= 2 && g <= 10;
}
inline bool onKeyLevel(double px, const std::vector<double>& keys, double tick)
{
    if (!(tick > 0)) tick = 0.25;
    for (size_t i = 0; i < keys.size(); ++i) if (std::fabs(keys[i] - px) <= tick * 0.5 + 1e-9) return true;
    return false;
}
// ---- the hovers: the question it answers, how the number answers it, how to use it, an example (Rassul 2026-10-01 / 10-03)
struct Hover { std::string title; std::vector<std::string> lines; };
inline std::string fmtStrike(const std::string& mkt, double k) { (void)mkt; return dl::strikeTxt((float)k); }
inline Hover nodeHover(const std::string& mkt, const NodeX& x, double net, double absMax, double pxNow)
{
    Hover h; const std::string K = fmtStrike(mkt, x.k);
    const bool w = net >= 0, mut = mutedNode(x.absG, net), up = x.k > pxNow;
    h.title = "What do dealers do if " + mkt + " gets to " + K + "?";
    std::string what = std::string(w ? "W = WALL: " : "F = FUEL: ") + "dealers are net " + (w ? "LONG" : "SHORT") + " gamma here, " + usdShort(x.absG) +
                       " of gamma per 1-pt move (net " + usdShort(net) + ").";
    if (mut) what += " Calls and puts nearly cancel, so the node is drawn thin.";
    h.lines.push_back(what);
    h.lines.push_back(w ? "Why it matters: a wall makes dealers sell rallies into it and buy dips under it - price tends to stall or pin."
                        : "Why it matters: fuel makes dealers chase price through it - moves into it tend to speed up, not stop.");
    if (x.hasD && std::fabs(x.d) >= 1.0) {
        const bool buy = x.d > 0;
        h.lines.push_back(std::string("Hedge: on the way ") + (up ? "up" : "down") + " to " + K + " dealers must " + (buy ? "BUY " : "SELL ") +
                          deltaNum(std::fabs(x.d)) + " " + mkt + " (about " + usdShort(std::fabs(x.dUsd)) + ") to stay hedged" +
                          (absMax > 0 && x.absG >= DELTA_SHOW_SHARE * absMax ? " - the number on the chart." : " (too small to print on the chart)."));
    }
    if (x.hasChg) h.lines.push_back("Since the first grid of the session this strike's gamma is " + pctChg(x.chg) + (x.chg >= 0 ? " - building." : " - fading."));
    h.lines.push_back(w ? "How to use: at this strike look for absorption / exhaustion (A, Ex?) to fade the move; a clean break with dealers chasing = the wall failed."
                        : "How to use: do not fade into fuel; trade with the move, or wait for exhaustion (Ex?) / CoB after it runs.");
    if (x.hasD && std::fabs(x.d) >= 1.0)
        h.lines.push_back(std::string("Example: price rallies toward ") + K + " - dealers " + (x.d > 0 ? "buying " : "selling ") + deltaNum(std::fabs(x.d)) +
                          (x.d > 0 ? " adds push; " : " leans against it; ") + (w ? "the wall is where a short can set up." : "the fuel is where it can run."));
    else
        h.lines.push_back(std::string("Example: ") + K + (w ? " holding twice with sellers absorbed = a wall doing its job." : " breaking with volume = fuel doing its job."));
    return h;
}
inline Hover hvlHover(const std::string& mkt, double hvl)
{
    Hover h; const std::string P = fmtStrike(mkt, hvl);
    h.title = "Which gamma regime is " + mkt + " in?";
    h.lines.push_back("HVL " + P + " is where dealers flip from long to short gamma.");
    h.lines.push_back("Above it they are LONG gamma: they sell rips and buy dips, so moves fade. Below it they are SHORT gamma: they chase, so moves run.");
    h.lines.push_back("How to use: above HVL favour fades at walls; below HVL respect breakouts and size down on fades.");
    h.lines.push_back("Example: price drops through " + P + " - expect the range to widen and walls below to give way faster.");
    return h;
}
inline Hover headerHover(const std::string& ver, const BookAsOf& a, double age, bool stale)
{
    Hover h;
    h.title = stale ? "Why is the dealer data STALE?" : "How fresh is the dealer data?";
    h.lines.push_back("Dealer Profile v" + ver + ". The nodes come from MenthorQ's strike grid taken at " + (a.ok() ? clock12(a.so) : std::string("an unknown time")) +
                      (age >= 0 ? " (" + ageShort(age) + " ago)." : "."));
    h.lines.push_back("STALE = the grid is over 20 min old while MenthorQ is updating: the nodes fade and should not be trusted for a trade.");
    h.lines.push_back("How to use: when STALE, use the key levels and the footprint only until the grid refreshes.");
    h.lines.push_back("Example: \"STALE 10h - 4:55 AM\" = the feed stopped at 4:55 AM; every wall / fuel size is from then.");
    return h;
}
// greedy word wrap by a width function (pixels)
template <class W> inline std::vector<std::string> wrapPx(const std::string& text, int maxW, W width)
{
    std::vector<std::string> out; std::string cur;
    std::istringstream in(text); std::string w;
    while (in >> w) {
        const std::string t = cur.empty() ? w : cur + " " + w;
        if (!cur.empty() && width(t) > maxW) { out.push_back(cur); cur = w; } else cur = t;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

} // namespace dpaudit
