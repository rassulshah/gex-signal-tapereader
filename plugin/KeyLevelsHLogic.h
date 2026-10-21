// KeyLevelsHLogic.h (lsKeyLevels 1.1.0, 2026-10-21: layoutStack - one right edge, a crowded group shrinks its font, then spreads; was 1.0.0, 2026-10-20): the pure logic of the Key Levels indicator for hourly charts - which levels show,
// their names and colours, and where the right-justified badges go. No SDK calls (tested on Linux: tests_keylevels/).
//  - Key / volume levels come from lsFlexLevels\LRA-KeyLevels-<MKT>.csv, written by lsDealerProfile on the market's 3-min chart
//    ("KEYLV,1,<unix>" then "L|<name>|<price>|<pivot>"; merged names "PFH/PDH"). The same numbers as the 3-min chart, to the tick.
//  - "Open" is never drawn (Rassul: "without the open"); hourly swings (HrHI / HrLO) and pivot-only rows are never drawn.
//  - A merged level keeps only the names whose group is ticked ("PFH/PDH" with PF off -> "PDH"); its colour = the first name left.
//  - MenthorQ levels (off by default): lsFlexLevels\MenthorQLevels.csv at MenthorQ's own prices, the whole intraday set (his rule:
//    100% consistent with MenthorQ, never hidden); "CR 0D / GW 0D" -> "CR0"; "MQ " dropped; a STALE set keeps its STALE tag.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include "KeyLevelsHSettings.h"

namespace klh {
enum Group { G_PF, G_PD, G_PW, G_CW, G_ON, G_LON, G_ASIA, G_DPOC, G_VA, G_WPOC, G_MQ, G_NONE };

// the settings rows (show, colour) of each key-level group
inline int showKey(int g) { static const int k[] = { klhset::K_SHOW_PF, klhset::K_SHOW_PD, klhset::K_SHOW_PW, klhset::K_SHOW_CW, klhset::K_SHOW_ON, klhset::K_SHOW_LON, klhset::K_SHOW_ASIA, klhset::K_SHOW_DPOC, klhset::K_SHOW_VA, klhset::K_SHOW_WPOC, klhset::K_SHOW_MQ }; return g >= 0 && g <= G_MQ ? k[g] : -1; }
inline int colKey(int g) { static const int k[] = { klhset::K_COL_PF, klhset::K_COL_PD, klhset::K_COL_PW, klhset::K_COL_CW, klhset::K_COL_ON, klhset::K_COL_LON, klhset::K_COL_ASIA, klhset::K_COL_DPOC, klhset::K_COL_VA, klhset::K_COL_WPOC }; return g >= 0 && g < G_MQ ? k[g] : -1; }

inline int groupOf(const std::string& n) {
    if (n == "PFH" || n == "PFL") return G_PF;
    if (n == "PDH" || n == "PDL") return G_PD;
    if (n == "PWH" || n == "PWL") return G_PW;
    if (n == "CWH" || n == "CWL") return G_CW;
    if (n == "ONH" || n == "ONL") return G_ON;
    if (n == "LonHI" || n == "LonLO") return G_LON;
    if (n == "AsiaHI" || n == "AsiaLO") return G_ASIA;
    if (n == "DPoc") return G_DPOC;
    if (n == "VAH" || n == "VAL") return G_VA;
    if (n == "WPoc") return G_WPOC;
    return G_NONE;                                  // Open, HrHI / HrLO, anything unknown: never drawn
}

struct KeyIn { std::string name; double price = 0; bool pivot = false; };   // a row of LRA-KeyLevels-<MKT>.csv
struct MqIn { std::string sym, label; double price = 0; };                  // a row of MenthorQLevels.csv
struct Line { std::string name; double price = 0; int group = G_NONE; unsigned long col = 0; };

// LRA-KeyLevels-<MKT>.csv. Accepted while under maxAgeSec old (the file is rewritten every new 3-min bar; 80 h covers a weekend).
inline bool parseKey(const std::string& text, long long nowUnix, long long maxAgeSec, std::vector<KeyIn>& out, long long& at) {
    out.clear(); at = 0; std::stringstream ss(text); std::string ln; bool head = false;
    while (std::getline(ss, ln)) {
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        if (!head) {
            if (ln.compare(0, 8, "KEYLV,1,") != 0) return false;
            char* e = nullptr; const long long t = std::strtoll(ln.c_str() + 8, &e, 10);
            if (!e || *e || t <= 0 || nowUnix - t > maxAgeSec || t - nowUnix > 600) return false;
            at = t; head = true; continue;
        }
        std::vector<std::string> c; std::stringstream ls(ln); std::string x; while (std::getline(ls, x, '|')) c.push_back(x);
        if (c.size() < 4 || c[0] != "L" || c[1].empty() || c[1].size() > 40) continue;
        char* e = nullptr; KeyIn k; k.name = c[1]; k.price = std::strtod(c[2].c_str(), &e);
        if (!e || *e || !std::isfinite(k.price) || k.price <= 0) continue;
        k.pivot = c[3] == "1"; out.push_back(k);
    }
    return head;
}

// MenthorQLevels.csv: SYMBOL,PRICE,LABEL,... (header optional)
inline void parseMq(const std::string& text, std::vector<MqIn>& out) {
    out.clear(); std::stringstream ss(text); std::string ln;
    while (std::getline(ss, ln)) {
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        if (ln.compare(0, 6, "SYMBOL") == 0) continue;
        std::vector<std::string> c; std::stringstream ls(ln); std::string x; while (std::getline(ls, x, ',')) c.push_back(x);
        if (c.size() < 3 || c[0].empty() || c[2].empty()) continue;
        char* e = nullptr; MqIn q; q.sym = c[0]; q.label = c[2]; q.price = std::strtod(c[1].c_str(), &e);
        if (!e || *e || !std::isfinite(q.price) || q.price <= 0) continue;
        out.push_back(q);
    }
}

// the MenthorQ badge name: "MQ " dropped, "CR 0D / GW 0D" -> "CR0", " / " -> " · ", STALE kept ("CR0 STALE")
inline std::string mqName(const std::string& label) {
    std::string n = label, stale;
    for (size_t p = n.find("MQ "); p != std::string::npos; p = n.find("MQ ", p)) { if (p == 0 || n[p - 1] == ' ' || n[p - 1] == '/') n.erase(p, 3); else p += 3; }
    const size_t st = n.find(" STALE"); if (st != std::string::npos) { stale = " STALE"; n = n.substr(0, st); }
    while (!n.empty() && n[0] == ' ') n.erase(0, 1);
    while (!n.empty() && n[n.size() - 1] == ' ') n.erase(n.size() - 1);
    if (n.find("CR 0D") != std::string::npos && n.find("GW 0D") != std::string::npos) n = "CR0";
    std::string r; for (size_t i = 0; i < n.size(); ++i) { if (n.compare(i, 3, " / ") == 0) { r += " \xB7 "; i += 2; } else r += n[i]; }
    return r + stale;
}
// green = support (PS), red = resistance (CR), yellow = HVL, cyan = the other gamma levels (the Dealer Profile's colours)
inline unsigned long mqColour(const std::string& label) {
    std::string f; size_t i = 0;
    while (i < label.size() && label[i] == ' ') ++i;
    if (label.compare(i, 3, "MQ ") == 0) i += 3;
    while (i < label.size() && label[i] != ' ' && label[i] != '/') f += (char)std::toupper((unsigned char)label[i++]);
    if (f == "PS") return 0x22C55E;
    if (f == "CR") return 0xEF4444;
    if (f == "HVL") return 0xFACC15;
    return 0x22D3EE;
}

// the lines to draw, sorted by price (low to high)
inline std::vector<Line> build(const std::vector<KeyIn>& key, const std::vector<MqIn>& mq, const ss::Values& V) {
    std::vector<Line> out;
    for (const KeyIn& k : key) {
        if (k.pivot) continue;
        std::string nm; int g0 = G_NONE; std::stringstream ss(k.name); std::string part;
        while (std::getline(ss, part, '/')) {
            const int g = groupOf(part);
            if (g == G_NONE || !V.b(showKey(g))) continue;
            if (g0 == G_NONE) g0 = g;
            nm += (nm.empty() ? "" : " \xB7 ") + part;
        }
        if (g0 == G_NONE) continue;
        Line L; L.name = nm; L.price = k.price; L.group = g0; L.col = V.c(colKey(g0)); out.push_back(L);
    }
    if (V.b(klhset::K_SHOW_MQ))
        for (const MqIn& q : mq) { Line L; L.name = mqName(q.label); if (L.name.empty()) continue; L.price = q.price; L.group = G_MQ; L.col = mqColour(q.label); out.push_back(L); }
    std::stable_sort(out.begin(), out.end(), [](const Line& a, const Line& b) { return a.price < b.price; });
    return out;
}

// right-justified badges: each badge's right edge at rightX; a badge whose row overlaps the one already placed nearest to it
// (|dy| < h) steps LEFT of it (gap px). y = the badge's centre, w = its width. Returns the left x of each badge.
inline std::vector<int> placeRight(const std::vector<int>& y, const std::vector<int>& w, int rightX, int h, int gap) {
    const size_t n = y.size(); std::vector<int> x(n, 0);
    std::vector<size_t> order(n); for (size_t i = 0; i < n; ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return y[a] < y[b]; });
    std::vector<size_t> placed;
    for (size_t oi = 0; oi < n; ++oi) {
        const size_t i = order[oi];
        int right = rightX;
        bool moved = true;
        while (moved) {                                  // step left past every placed badge this one would overlap
            moved = false;
            for (size_t p : placed) {
                if (std::abs(y[p] - y[i]) >= h) continue;
                const int pl = x[p], pr = x[p] + w[p];
                const int l = right - w[i];
                if (l < pr + gap && right > pl - gap) { right = pl - gap; moved = true; }
            }
        }
        x[i] = right - w[i]; placed.push_back(i);
    }
    return x;
}

// (1.1.0, Rassul 2026-10-21 09:26 CT "sometimes the levels are close to each other so you need to make the badges dynamic so they not
// move left and right messing up the alignment, instead you can make the badge smaller or the font smaller and adjust as necessary")
// ONE right edge for every badge (never stepped left). Badges whose rows would overlap form a group; the group shrinks its font
// (base -> minFont) until its rows fit at their own lines; only if they still cannot fit at minFont are the group's badges spread up /
// down just enough (centred on the group, inside the pane) - each stays beside its own line. Badges away from a crowd keep the base size.
struct Slot { int y = 0; int font = 0; int h = 0; };
inline int badgeH(int font, int base) { return font + (font >= base ? 6 : 4); }
inline std::vector<Slot> layoutStack(const std::vector<int>& yIn, int baseFont, int minFont, int top, int bottom) {
    const size_t n = yIn.size(); std::vector<Slot> out(n);
    if (!n) return out;
    if (minFont > baseFont) minFont = baseFont;
    std::vector<size_t> ord(n); for (size_t i = 0; i < n; ++i) ord[i] = i;
    std::stable_sort(ord.begin(), ord.end(), [&](size_t a, size_t b) { return yIn[a] < yIn[b]; });
    const int hBase = badgeH(baseFont, baseFont);
    size_t a = 0;
    while (a < n) {                                        // a group = a chain of badges closer than one base badge height
        size_t b = a;
        while (b + 1 < n && yIn[ord[b + 1]] - yIn[ord[b]] < hBase) ++b;
        int font = baseFont;
        for (; font > minFont; --font) {                   // the largest font whose rows fit at their own lines
            const int hh = badgeH(font, baseFont); bool fits = true;
            for (size_t k = a; k < b; ++k) if (yIn[ord[k + 1]] - yIn[ord[k]] < hh) { fits = false; break; }
            if (fits) break;
        }
        const int hh = badgeH(font, baseFont);
        std::vector<int> yy; for (size_t k = a; k <= b; ++k) yy.push_back(yIn[ord[k]]);
        bool fits = true; for (size_t k = 1; k < yy.size(); ++k) if (yy[k] - yy[k - 1] < hh) { fits = false; break; }
        if (!fits) {                                       // still overlapping at the smallest font: spread up / down, centred on the group
            std::vector<int> p(yy.size()); p[0] = yy[0];
            for (size_t k = 1; k < yy.size(); ++k) p[k] = std::max(yy[k], p[k - 1] + hh);
            long long shiftSum = 0; for (size_t k = 0; k < yy.size(); ++k) shiftSum += p[k] - yy[k];
            int shift = (int)(shiftSum / (long long)yy.size());
            for (size_t k = 0; k < p.size(); ++k) p[k] -= shift;
            if (p.front() - hh / 2 < top) { const int d = top - (p.front() - hh / 2); for (int& v : p) v += d; }
            if (p.back() + hh / 2 > bottom) { const int d = p.back() + hh / 2 - bottom; for (int& v : p) v -= d; }
            for (size_t k = 1; k < p.size(); ++k) if (p[k] < p[k - 1] + hh) p[k] = p[k - 1] + hh;   // keep the order after a clamp
            yy = p;
        }
        for (size_t k = a; k <= b; ++k) { Slot& o = out[ord[k]]; o.y = yy[k - a]; o.font = font; o.h = hh; }
        a = b + 1;
    }
    return out;
}
} // namespace klh
