// LevelPctLogic.h  1.0.0  (2026-10-21, lsDealerProfile 2.19.0)  -  the level % badges, Option B (Rassul approved the mockup 2026-10-21).
// Next to every key level's badge in the Levels column: one compact block "33%  /  66%" (two spaces each side of the slash):
//   1st % = of the days whose 1st extreme was the LOD (the level is BELOW today's RTH open) / the HOD (ABOVE the open), how often that
//           1st extreme came within 0.25 ADR of the level (swept it by up to 0.25 ADR or stopped up to 0.25 ADR short)
//   2nd % = the same for the day's FINAL LOD / HOD, over every day with the level on that side
// Outline: GREEN = the level beats a random price the same distance from the open (the nightly's null control); GREY = no better than
// random. No "?" anywhere, no sub-text on the chart; the hover says what it is, both numbers with n, the random comparison, what it
// means, how to use it, an example. A number with n < 20 is a dash; a level with neither number has no block.
// Source: lsFlexLevels\LRA-LevelBadges-<MKT>.csv, written by the nightly (lra.hodlod_levels 1.2.0) per market from that market's own
// days - never one market's numbers on another (MARKET must match). Missing / garbled / wrong market = no blocks, the reason in the
// Dealer Profile status file (LEVEL_PCT line). The plugin only looks the numbers up and draws them (IRT-native, no run-time service).
// Pure C++ (no SDK) - tested in tests/test_levelpct_logic.cpp.
#pragma once
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace lpct {
static const char* const VERSION = "1.0.0";
static const int MIN_N = 20;
static const size_t MAX_BYTES = 65536, MAX_LINES = 512;

struct Row { std::string name, side; long n1 = 0, n2 = 0; int p1 = -1, r1 = -1, p2 = -1, r2 = -1; int beats = 0; };   // -1 = no number
struct Table { std::string market, built, first, last; double zone = 0.25; std::vector<Row> rows; bool ok = false; };

inline std::vector<std::string> splitc(const std::string& s, char c)
{
    std::vector<std::string> o; size_t a = 0;
    for (;;) { size_t e = s.find(c, a); if (e == std::string::npos) { o.push_back(s.substr(a)); break; } o.push_back(s.substr(a, e - a)); a = e + 1; }
    return o;
}
// a whole number in [lo, hi], the full token, nothing else
inline bool toLong(const std::string& t, long lo, long hi, long& v)
{
    if (t.empty() || t.size() > 9) return false;
    for (size_t i = 0; i < t.size(); ++i) if (t[i] < '0' || t[i] > '9') return false;
    errno = 0; char* e = nullptr; const long long x = std::strtoll(t.c_str(), &e, 10);
    if (errno || !e || *e || x < lo || x > hi) return false;
    v = (long)x; return true;
}
// a percent 0-100, or empty = no number (-1)
inline bool toPct(const std::string& t, int& v)
{
    if (t.empty()) { v = -1; return true; }
    long x = 0; if (!toLong(t, 0, 100, x)) return false;
    v = (int)x; return true;
}
inline bool nameOk(const std::string& n)
{
    if (n.empty() || n.size() > 12) return false;
    for (size_t i = 0; i < n.size(); ++i) { const char c = n[i]; if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return false; }
    return true;
}
// strict: VERSION,1 first, MARKET must be the chart's market, every L row whole; anything wrong = false + the reason
inline bool parse(const std::string& text, const std::string& mkt, Table& T, std::string& why)
{
    T = Table();
    if (text.empty()) { why = "empty file"; return false; }
    if (text.size() > MAX_BYTES) { why = "file too big"; return false; }
    std::vector<std::string> L = splitc(text, '\n');
    if (L.size() > MAX_LINES + 1) { why = "too many lines"; return false; }
    if (text[text.size() - 1] != '\n') { why = "torn (no final newline)"; return false; }
    bool ver = false, mk = false; size_t ln = 0;
    for (size_t i = 0; i < L.size(); ++i) {
        std::string s = L[i]; while (!s.empty() && (s[s.size() - 1] == '\r' || s[s.size() - 1] == ' ')) s.erase(s.size() - 1);
        if (s.empty() || s[0] == '#') continue;
        const std::vector<std::string> f = splitc(s, ',');
        ++ln;
        if (ln == 1) { if (f.size() != 2 || f[0] != "VERSION" || f[1] != "1") { why = "VERSION,1 must be the first line"; return false; } ver = true; continue; }
        if (f[0] == "MARKET") { if (f.size() != 2 || f[1] != mkt) { why = "MARKET " + (f.size() > 1 ? f[1] : std::string("?")) + " is not " + mkt; return false; } mk = true; T.market = f[1]; continue; }
        if (!mk) { why = "MARKET must be the second line"; return false; }
        if (f[0] == "BUILT" && f.size() == 2) { T.built = f[1]; continue; }
        if (f[0] == "DAYS" && f.size() == 3) { T.first = f[1]; T.last = f[2]; continue; }
        if (f[0] == "ZONE_ADR" && f.size() == 2) {
            char* e = nullptr; const double z = std::strtod(f[1].c_str(), &e);
            if (!e || *e || !std::isfinite(z) || z <= 0 || z > 1) { why = "bad ZONE_ADR"; return false; }
            T.zone = z; continue;
        }
        if (f[0] == "L") {
            if (f.size() != 10) { why = "an L row without 10 fields"; return false; }
            Row r; r.name = f[1]; r.side = f[2]; long b = 0;
            if (!nameOk(r.name) || (r.side != "LOD" && r.side != "HOD")) { why = "bad level name / side"; return false; }
            if (!toLong(f[3], 0, 100000, r.n1) || !toPct(f[4], r.p1) || !toPct(f[5], r.r1) || !toLong(f[6], 0, 100000, r.n2) ||
                !toPct(f[7], r.p2) || !toPct(f[8], r.r2) || !toLong(f[9], 0, 3, b)) { why = "bad number in the " + r.name + " " + r.side + " row"; return false; }
            if ((r.p1 >= 0) != (r.r1 >= 0) || (r.p2 >= 0) != (r.r2 >= 0)) { why = "a number without its random control (" + r.name + ")"; return false; }
            if (r.n1 < MIN_N) { r.p1 = r.r1 = -1; }                       // never show a number on fewer than 20 days
            if (r.n2 < MIN_N) { r.p2 = r.r2 = -1; }
            r.beats = (int)b;
            if (r.p1 < 0) r.beats &= ~1;
            if (r.p2 < 0) r.beats &= ~2;
            if (r.p1 < 0 && r.p2 < 0) continue;
            T.rows.push_back(r); continue;
        }
        // other keys (SOURCE, MIN_N, ...) are informational
    }
    if (!ver || !mk) { why = "no VERSION / MARKET"; return false; }
    T.ok = true; why.clear();
    return true;
}

// the code a badge name maps to: the compact Dealer Profile names ("CR0", "LonLO", "DPoc", "1DMin"); a merged key level
// "PFH/PDH" or a gamma merge "PDL/HVL" is split on '/', each part looked up in order (the first one with a number wins)
inline const Row* find(const Table& T, const std::string& code, const std::string& side)
{
    for (size_t i = 0; i < T.rows.size(); ++i) if (T.rows[i].name == code && T.rows[i].side == side) return &T.rows[i];
    return nullptr;
}
inline const Row* lookup(const Table& T, const std::string& merged, const std::string& side, std::string& used)
{
    used.clear(); if (!T.ok) return nullptr;
    const std::vector<std::string> parts = splitc(merged, '/');
    for (size_t i = 0; i < parts.size(); ++i) {
        std::string p = parts[i]; while (!p.empty() && p[0] == ' ') p.erase(0, 1); while (!p.empty() && p[p.size() - 1] == ' ') p.erase(p.size() - 1);
        const Row* r = find(T, p, side);
        if (r) { used = p; return r; }
    }
    return nullptr;
}
// which side a level is on: below the RTH open = LOD (support), above = HOD; before the open (no Open yet) the last price stands in
inline std::string sideOf(double level, double open, double lastPx)
{
    const double ref = (std::isfinite(open) && open > 0) ? open : lastPx;
    if (!std::isfinite(ref) || !(ref > 0) || !std::isfinite(level)) return std::string();
    if (level < ref) return "LOD";
    if (level > ref) return "HOD";
    return std::string();
}
inline std::string pctTxt(int p) { if (p < 0) return "-"; char b[16]; std::snprintf(b, sizeof(b), "%d%%", p); return b; }
// the badge text, exactly "33%  /  66%" (a dash for a number with n < 20)
inline std::string text(const Row& r) { return pctTxt(r.p1) + "  /  " + pctTxt(r.p2); }
inline bool green(const Row& r) { return r.beats != 0; }
// outline colours (0x00RRGGBB, as every LRA plugin writes them): his bullish green (= C_BUY), the muted grey (C_MUTED)
static const unsigned long C_GREEN = 0x0022C55EUL, C_GREY = 0x009CA3AFUL;
inline unsigned long outline(const Row& r) { return green(r) ? C_GREEN : C_GREY; }

struct Hover { std::string title; std::vector<std::string> lines; };
// the hover in plain words: what it is, both numbers with n, the random comparison, what it means, how to use it, an example
inline Hover hover(const std::string& mkt, const std::string& shownName, const std::string& used, const std::string& priceTxt,
                   const Row& r, const Table& T)
{
    Hover h; const bool lod = r.side == "LOD";
    const std::string ext = lod ? "low" : "high", EXT = lod ? "LOD" : "HOD", where = lod ? "below" : "above";
    char z[16]; std::snprintf(z, sizeof(z), "%.2f", T.zone);
    h.title = shownName + " " + priceTxt + " \xB7 " + text(r) + " \xB7 does the " + mkt + " " + ext + " of the day end up here?";
    h.lines.push_back("What: how often " + mkt + "'s " + EXT + " came within " + z + " ADR of " + used + " (swept it or stopped just short) on days "
                      "it sat " + where + " the RTH open. " + mkt + " days only" + (T.first.empty() ? std::string(".") : " (" + T.first + " to " + T.last + ")."));
    char a[200];
    if (r.p1 >= 0) std::snprintf(a, sizeof(a), "1st number %d%%: of %ld days whose FIRST extreme was the %s, that %s came here on %d%%.", r.p1, r.n1, EXT.c_str(), ext.c_str(), r.p1);
    else std::snprintf(a, sizeof(a), "1st number -: only %ld days whose first extreme was the %s - too few (20 needed).", r.n1, EXT.c_str());
    h.lines.push_back(a);
    if (r.p2 >= 0) std::snprintf(a, sizeof(a), "2nd number %d%%: of %ld days with %s %s the open, the day's FINAL %s came here on %d%%.", r.p2, r.n2, used.c_str(), where.c_str(), EXT.c_str(), r.p2);
    else std::snprintf(a, sizeof(a), "2nd number -: only %ld days - too few (20 needed).", r.n2);
    h.lines.push_back(a);
    h.lines.push_back("A random price the same distance away: " + pctTxt(r.r1) + "  /  " + pctTxt(r.r2) + ".");
    if (green(r)) h.lines.push_back(std::string("Means: GREEN = ") + used + " beats a random price (" + ((r.beats & 1) ? "1st" : "") +
                                    ((r.beats & 3) == 3 ? " and " : "") + ((r.beats & 2) ? "final" : "") + " " + EXT + ") - the level itself draws the turn, not just its distance.");
    else h.lines.push_back("Means: GREY = no better than a random price at that distance - the odds come from how far it is from the open, not from the level.");
    h.lines.push_back(std::string("How to use: ") + (green(r) ? "a green level is a place to look for the " + EXT + " - still wait for your sweep + RB / CoB trigger." :
                                                          "do not trade a grey level on its name; take the turn only on your sweep + RB / CoB trigger and the tape."));
    std::snprintf(a, sizeof(a), "Example: \"%s\" = the first %s landed here on %s of those days, the final %s on %s.", text(r).c_str(), ext.c_str(), pctTxt(r.p1).c_str(), ext.c_str(), pctTxt(r.p2).c_str());
    h.lines.push_back(a);
    return h;
}

} // namespace lpct
