// HodLodLast5.h 1.0.0 (2026-11-26 15:16 CT, Rassul "build") - the SDK-free LAST 5 DAYS panel of lsHodLod 2.54.0 (Linux-tested).
// Requests (CT, 2026-11-26): 14:59 "a side panel that shows the last 5 days and the hod and lod levels that were swept"; 15:01 "it should be
// vertical and left justified, no black space between the left side and it"; 15:02 "if there are multiple levels that were swept, show at most
// two"; 15:06 "put the date below and the day above it"; 15:07 "vertically center it"; 15:08 "some space above and below the separator ..
// add a little more"; 15:12 "i just want to see it for the last 5 days swept box"; 15:18 order: session, daily, volume, weekly, gamma.
// Mockup: LastFive-Vertical-mockup.html. Data: the nightly's L5 rows in LRA-HodLodSweeps-<MKT>.csv (hodlod_levels 1.15.0):
//   L5,<session YYYY-MM-DD>,<HOD|LOD = the 1st extreme>,<HOD CT min>,<levels or ->,<LOD CT min>,<levels or ->   (newest first, <= 2 names)
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

namespace l5 {
static const char* const VERSION = "1.0.1";   // (1.0.1) the calendar height read from "PLACE,top left" or "PLACE,top centre"
static const int MAX_DAYS = 5, MAX_NAMES = 2, ROW = 17, PX = 5, GAP = 6, SEP = 8;   // SEP = the space above and below a day separator
static const unsigned C_BG = 0x000B0F14, C_EDGE = 0x00444444, C_SEPV = 0x002A2F36, C_HEAD = 0x009A9A9A, C_TXT = 0x00E5E7EB,
                      C_RED = 0x00EF4444, C_GREEN = 0x0022C55E, C_AMBER = 0x00F59E0B, C_TAG = 0x006B5212;

struct Day { std::string date, first, hodLv, lodLv; int hodT = -1, lodT = -1; };

inline bool num(const std::string& s, long lo, long hi, int& out) {
    if (s.empty()) return false;
    char* e = nullptr; const long v = std::strtol(s.c_str(), &e, 10);
    if (!e || *e || v < lo || v > hi) return false;
    out = (int)v; return true;
}
inline bool isDate(const std::string& d) {
    if (d.size() != 10 || d[4] != '-' || d[7] != '-') return false;
    for (size_t i = 0; i < 10; i++) if (i != 4 && i != 7 && (d[i] < '0' || d[i] > '9')) return false;
    const int mo = std::atoi(d.substr(5, 2).c_str()), dy = std::atoi(d.substr(8, 2).c_str());
    return mo >= 1 && mo <= 12 && dy >= 1 && dy <= 31;
}
// "-" = none; else 1-2 names of letters / digits, one space between, each <= 12 chars
inline bool levels(const std::string& s, std::string& out) {
    if (s == "-") { out.clear(); return true; }
    if (s.empty() || s.size() > 26) return false;
    int names = 0; size_t i = 0;
    while (i < s.size()) {
        size_t j = i; while (j < s.size() && s[j] != ' ') { const char c = s[j]; if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return false; j++; }
        if (j == i || j - i > 12) return false;
        if (++names > MAX_NAMES) return false;
        i = j; if (i < s.size()) { i++; if (i >= s.size()) return false; }
    }
    out = s; return true;
}
// one L5 row (already split on ','): true = good
inline bool parseRow(const std::vector<std::string>& f, Day& d) {
    if (f.size() < 7 || f[0] != "L5" || !isDate(f[1]) || (f[2] != "HOD" && f[2] != "LOD")) return false;
    Day x; x.date = f[1]; x.first = f[2];
    if (!num(f[3], 0, 1439, x.hodT) || !num(f[5], 0, 1439, x.lodT)) return false;
    if (!levels(f[4], x.hodLv) || !levels(f[6], x.lodLv)) return false;
    d = x; return true;
}
// "Wed" (Sakamoto's day of the week)
inline std::string dow(const std::string& date) {
    if (!isDate(date)) return "";
    static const int T[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 }; static const char* const N[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    int y = std::atoi(date.substr(0, 4).c_str()); const int m = std::atoi(date.substr(5, 2).c_str()), d = std::atoi(date.substr(8, 2).c_str());
    if (m < 3) y -= 1;
    return N[(y + y / 4 - y / 100 + y / 400 + T[m - 1] + d) % 7];
}
inline std::string md(const std::string& date) {   // "11/25"
    if (!isDate(date)) return "";
    return std::to_string(std::atoi(date.substr(5, 2).c_str())) + "/" + date.substr(8, 2);
}
inline std::string clk(int mod) { char b[16]; const int h = (mod / 60) % 24; std::snprintf(b, sizeof b, "%d:%02d", (h + 11) % 12 + 1, mod % 60); return b; }
// lsEventCal's status line "PLACE,top left <l>,<t> <w>x<h>" -> its height in px (-1 = not drawn / unreadable)
inline int calHeight(const std::string& status) {
    const std::string key = "\nPLACE,top ";   // (2.57.2) "top left" (lsEventCal 1.1.0) or "top centre" (1.2.0, 11-27 08:40 "move the calendar to the top center")
    const size_t p = ("\n" + status).find(key); if (p == std::string::npos) return -1;
    const std::string rest = ("\n" + status).substr(p + key.size());
    const size_t x = rest.find('x'), sp = rest.find(' '), nl = rest.find('\n');
    if (x == std::string::npos || sp == std::string::npos || x < sp || (nl != std::string::npos && x > nl)) return -1;
    char* e = nullptr; const long h = std::strtol(rest.c_str() + x + 1, &e, 10);
    if (!e || (*e && *e != '\n' && *e != '\r') || h <= 0 || h > 400) return -1;
    return (int)h;
}

struct Item { int k = 0; int l = 0, t = 0, r = 0, b = 0; std::string s; unsigned col = 0, fill = 0; bool bold = false; bool right = false; };   // k 0 box, 1 text, 2 line
struct Draw { bool ok = false; int l = 0, t = 0, r = 0, b = 0, shown = 0; std::vector<Item> items; std::string text, why; };

inline int height(int n) { return n <= 0 ? 0 : 3 + ROW + 3 + n * 2 * ROW + (n - 1) * (2 * SEP + 1) + 3; }

// M(text, bold) = pixel width at the panel font. left = the pane's left edge (docked flush: the panel's left border sits just off it);
// [top, bottom] = the free band (under the Event Calendar, over the Level line / HOD-LOD box); right = the last bar. Centred vertically
// in the band; the oldest days drop when the band is too short.
inline Draw layout(const std::vector<Day>& days, const std::function<int(const std::string&, bool)>& M, int left, int top, int bottom, int right) {
    Draw D;
    if (days.empty()) { D.why = "no L5 rows (nightly 1.15.0 not run yet)"; return D; }
    int n = (std::min)((int)days.size(), MAX_DAYS);
    while (n > 0 && height(n) > bottom - top) n--;
    if (n <= 0) { D.why = "no room (" + std::to_string(bottom - top) + " px tall)"; return D; }
    const std::string title = "Last 5 days swept";
    int wDay = 0, wSide = (std::max)(M("HOD", true), M("LOD", true)), wT = 0, wLv = 0; const int wTag = M("1st", false) + 6, tagGap = 4;
    for (int i = 0; i < n; i++) {
        const Day& d = days[(size_t)i];
        wDay = (std::max)(wDay, (std::max)(M(dow(d.date), false), M(md(d.date), false)));
        wT = (std::max)(wT, (std::max)(M(clk(d.hodT), false), M(clk(d.lodT), false)));
        for (int k = 0; k < 2; k++) {
            const std::string& lv = k == 0 ? d.hodLv : d.lodLv; const bool f1 = (k == 0) == (d.first == "HOD");
            wLv = (std::max)(wLv, M(lv.empty() ? "-" : lv, false) + (f1 ? tagGap + wTag : 0));
        }
    }
    const int W = (std::max)(PX + wDay + GAP + wSide + GAP + wT + GAP + wLv + PX, PX + M(title, false) + PX);
    if (left + W > right) { D.why = "no room (" + std::to_string(W) + " px wide)"; return D; }
    const int H = height(n);
    D.l = left - 1; D.r = left + W; D.t = top + (bottom - top - H) / 2; D.b = D.t + H; D.shown = n;
    D.items.push_back(Item{ 0, D.l, D.t, D.r, D.b, "", C_EDGE, C_BG, false, false });
    int y = D.t + 3;
    D.items.push_back(Item{ 1, left + PX, y, left + PX + M(title, false), y + ROW, title, C_HEAD, 0, false, false });   // text boxes = their measured width (the host centres text in its box)
    y += ROW + 1; D.items.push_back(Item{ 2, left + PX, y, D.r - PX, y, "", C_SEPV, 0, false, false }); y += 2;
    const int xS = left + PX + wDay + GAP, xT = xS + wSide + GAP, xL = xT + wT + GAP;
    D.text = title;
    for (int i = 0; i < n; i++) {
        const Day& d = days[(size_t)i];
        if (i > 0) { y += SEP; D.items.push_back(Item{ 2, left + PX, y, D.r - PX, y, "", C_SEPV, 0, false, false }); y += 1 + SEP; }
        D.text += " ; " + dow(d.date) + " " + md(d.date);
        for (int k = 0; k < 2; k++) {
            const bool hi = k == 0; const std::string& lv = hi ? d.hodLv : d.lodLv; const int t = hi ? d.hodT : d.lodT;
            const bool f1 = hi == (d.first == "HOD");
            { const std::string dl = hi ? dow(d.date) : md(d.date); D.items.push_back(Item{ 1, left + PX, y, left + PX + M(dl, false), y + ROW, dl, C_HEAD, 0, false, false }); }
            D.items.push_back(Item{ 1, xS, y, xS + M(hi ? "HOD" : "LOD", true), y + ROW, hi ? "HOD" : "LOD", hi ? C_RED : C_GREEN, 0, true, false });
            { const int wt = M(clk(t), false); D.items.push_back(Item{ 1, xT + wT - wt, y, xT + wT, y + ROW, clk(t), C_HEAD, 0, false, true }); }   // right-aligned
            const std::string s = lv.empty() ? "-" : lv; const int ws = M(s, false);
            D.items.push_back(Item{ 1, xL, y, xL + ws, y + ROW, s, lv.empty() ? C_HEAD : C_TXT, 0, false, false });
            if (f1) {
                const int tl = xL + ws + tagGap;
                D.items.push_back(Item{ 0, tl, y + 2, tl + wTag, y + ROW - 2, "", C_TAG, C_BG, false, false });
                D.items.push_back(Item{ 1, tl, y, tl + wTag, y + ROW, "1st", C_AMBER, 0, false, false });
            }
            D.text += std::string(hi ? " HOD " : " LOD ") + clk(t) + " " + s + (f1 ? " [1st]" : "");
            y += ROW;
        }
    }
    D.ok = true; return D;
}
}  // namespace l5
