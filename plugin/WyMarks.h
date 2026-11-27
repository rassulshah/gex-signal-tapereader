// WyMarks.h 2.0.0 (2026-11-19) -- the Reversal Read marks on the price candles (lsRevReadMarks 0.19.0). SDK-free, tested (tests_wymarks).
// 1.0.0 (2026-11-18 17:06 / 17:11 CT, Rassul "I also want the I indicator on the candle, put it on top of green candle and below red candles"):
// the I tag. 2.0.0 (2026-11-19 10:20-10:40 CT, mockup level-marks-mockup.html v8, "go"): Bk / Sw / Cx too (LevelMarks.h), each tag = the code on
// line 1 and ONE short level name on line 2 ("I / PHL", "Sw / PDL"); over a green candle, under a red one; the I first, the level mark stacked
// outward; tags never cover a candle (wick to wick) or another tag: rows outward, a few pixels sideways, then the other side; a short dashed
// line at the level price.
// Source: lsFlexLevels\RevRead\wy-<MKT>-<secs>.txt, written by the lsRevRead pane once per closed candle: header WY|1|MKT|secs|version, then
// stamp|letter|dir[|level|levelPx] lines (3 fields = the old file; 5 = with the level). Decided letters only: a mark never changes (no repaint).
#pragma once
#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace wyi {
static const char* const VERSION = "2.10.0";   // (2.10.0, Rassul 2026-11-27 12:38-12:44 CT, the cyan thrust dot; lsRevRead 0.44.0) + code D = "TD" (stamp|TD|dir): no badge, drawn as a dot
// was static const char* const VERSION = "2.9.0";   // (2.9.0, Rassul 2026-11-27 09:50-10:00 CT "consolidate AR and Vob (t) into a new BI (Break In) signal"; lsRevRead 0.43.0 / LevelMarks 1.31.0): + the BI (code N, file "BI": stamp|BI|dir|level|px|strength), drawn like a Rev with its level - over a bullish candle, under a bearish one
// was static const char* const VERSION = "2.8.0";   // (2.8.0, Rassul 2026-11-24 12:51 CT "2. do the test" / "3. yes"): the Abs badge's P line always shows once known ("P0" .. "P12+"; blank only when the order feed did not see the candle); + parseTeq (lsTurn 1.6.0 turnexq file) and the Tst line-3 decision file (tl3Text / parseTl3, lsRevRead 0.28.0)
// was static const char* const VERSION = "2.7.0";   // (2.7.0, Rassul 2026-11-24 11:12 CT "ok. build this", abs-badge-mockup.html v15): + the Abs badge (code Q: "Abs" / position / P) and its next-candle "Ex" (code E), drawn at the absorbed end; + parseTurnAbs (lsTurn's RevRead\\turnabs-<MKT>-<spb>.txt: the biggest print P and the same-candle Ex?)
// was static const char* const VERSION = "2.6.0";   // (2.6.0, Rassul 2026-11-23 23:20 CT "go" on the live Cx?): + parseLive (the wylive file of lsRevRead 0.25.0)
// was static const char* const VERSION = "2.5.0";   // (2.5.0, Rassul 2026-11-23 09:25-09:34 CT "implement the AR ... automatic rally bar that occurs after a cx", "the AR bar should also have a strength line", "go"; lsRevRead 0.23.0 / LevelMarks 1.13.0, mockup ar-mockup.html): + the AR badge - "AR" and its strength line (stamp|AR|dir||0|R51), over a green candle, under a red one, as Bk / Sw / Cx
// was VERSION = "2.4.0";   // (2.4.0, Rassul 2026-11-20 09:20 CT "add it ... add the badge for it also and include the level and strength line", 09:24 "build"; lsRevRead 0.21.2, mockup i-badge-mockup.html): the I badge is back - letter / level / strength like Bk / Sw; a bullish (green) I goes UNDER its candle, a bearish (red) I OVER it; iLine() writes the I line, with a 7th field "Sw" when the same candle is a Sw (recorded for the nightly, never drawn - parseAll reads 3 / 5 / 6 fields only)
// was VERSION = "2.3.0";   // (2.3.0, Rassul 2026-11-19 15:35-16:58 CT, price-pane-cleanup-mockup.html, "ok"): + FT / Tst / Rev after a Bk (LevelMarks 1.3.0 seq): "FT" (a plain letter), "Tst" (level + %), "Rev" (+ strength); a tag can ask for its side (FT / Rev with the break, Tst on the pivot side)
// was VERSION = "2.2.0";   // (2.2.0, Rassul 11:30-11:35 CT: the strength line) mark lines may carry a 6th field = the strength ("V45" / "R99+" / ""); 3 / 5 / 6 fields read
// was VERSION = "2.1.0";   // (2.1.0, Rassul 10:50-10:51 CT "ok build", "remember placement of other signals like cmx so you have to coordinate everything"): the key levels published in the file (L|px|name lines, read by the BO indicator for its Rly level); the layout also keeps clear of boxes other indicators occupy (the BO indicator's letters from SignalBoard 'L')
static const int TAG_H = 13, PAD_X = 3, GAP = 3, CX_STEP = 16;
static const int LINE1_H = 13, LINE2_H = 11;   // (2.0.0) a two-line tag: 13 px for the code, 11 px for the level name
static const size_t MAX_BYTES = 1024 * 1024;

struct M { char code = 0; int dir = 0; std::string lv; long long px = 0; std::string st; };   // (2.2.0) st = the strength line ("" = none)   // code 'I' / 'B' (Bk) / 'S' (Sw) / 'C' (Cx)
inline char codeOf(const std::string& s) { return s == "I" ? 'I' : s == "Bk" ? 'B' : s == "Sw" ? 'S' : s == "Cx" ? 'C' : s == "FT" ? 'F' : s == "Tst" ? 'T' : s == "Rev" ? 'R' : s == "AR" ? 'A' : s == "BI" ? 'N' : s == "TD" ? 'D' : s == "Abs" ? 'Q' : s == "Ex" ? 'E' : 0; }   // (2.7.0) 'Q' = the Abs badge, 'E' = its next-candle Ex   // (2.5.0) 'A' = AR (the file's "A" lines - absorbed - are another letter: not a mark here)
inline const char* textOf(char c) { return c == 'I' ? "I" : c == 'B' ? "Bk" : c == 'S' ? "Sw" : c == 'C' ? "Cx" : c == 'F' ? "FT" : c == 'T' ? "Tst" : c == 'R' ? "Rev" : c == 'A' ? "AR" : c == 'N' ? "BI" : c == 'D' ? "TD" : c == 'Q' ? "Abs" : c == 'E' ? "Ex" : ""; }
// (2.3.0) where a tag wants to sit: 0 = by the candle (over green, under red); +1 over / -1 under. FT and Rev go with the break (over for a
// bullish one), a Tst on its pivot's side (under a bullish Tst's pivot low, over a bearish one's high) - as the BO indicator's FT / Pb / Rev
inline int sideOf(const M& m) { return m.code == 'F' || m.code == 'R' || m.code == 'N' ? m.dir : m.code == 'T' || m.code == 'I' || m.code == 'Q' || m.code == 'E' ? -m.dir : 0; }   // (2.7.0) Abs / Ex: at the end that was absorbed (under a bullish one, over a bearish one)   // (2.4.0) I: under a bullish one, over a bearish one
// (2.4.0) one I line of the wy file: stamp|I|dir|lv|px|st (6 fields; the level is always named since 0.19.4); underSw = the same candle is a Sw of
// the same direction -> a 7th field "Sw": kept for the nightly score, skipped by every reader that draws (parseAll, VolRead, HodLod, TopSignals)
inline std::string iLine(long long t, int dir, const std::string& lv, long long px, const std::string& st, bool underSw) {
    return std::to_string(t) + "|I|" + std::to_string(dir) + "|" + lv + "|" + std::to_string(px) + "|" + st + (underSw ? "|Sw" : ""); }
inline bool plainOf(const M& m) { return m.code == 'F'; }   // (2.3.0) FT stays a plain coloured letter; every other mark is a badge

// every candle mark of the file: candle stamp -> its marks (the I first). -1 = not a letters file (torn / foreign: keep the last good set).
inline int parseAll(const std::string& txt, std::map<long long, std::vector<M> >& out) {
    out.clear(); size_t a = 0; int n = 0; bool head = false;
    while (a < txt.size()) {
        size_t e = txt.find('\n', a); if (e == std::string::npos) e = txt.size();
        std::string l = txt.substr(a, e - a); a = e + 1;
        if (!l.empty() && l[l.size() - 1] == '\r') l.erase(l.size() - 1);
        if (l.empty()) continue;
        if (!head) { if (l.compare(0, 5, "WY|1|") != 0) return -1; head = true; continue; }
        std::vector<std::string> f; size_t p = 0; while (true) { const size_t q = l.find('|', p); f.push_back(l.substr(p, q == std::string::npos ? std::string::npos : q - p)); if (q == std::string::npos) break; p = q + 1; }
        if (f.size() == 3 && f[0] == "L") continue;   // (2.1.0) a level line (parseLevels)
        if (f.size() != 3 && f.size() != 5 && f.size() != 6) continue;   // (2.2.0) 6 = + the strength
        char* end = nullptr; const long long t = std::strtoll(f[0].c_str(), &end, 10); if (!end || *end != 0 || t <= 0) continue;
        const char c = codeOf(f[1]); if (!c) continue;
        char* e2 = nullptr; const long d = std::strtol(f[2].c_str(), &e2, 10); if (!e2 || *e2 != 0 || (d != 1 && d != -1)) continue;
        M m; m.code = c; m.dir = (int)d;
        if (f.size() >= 5) { if (f[3].size() > 8) continue; m.lv = f[3]; char* e3 = nullptr; const long long px = std::strtoll(f[4].c_str(), &e3, 10); if (!e3 || *e3 != 0 || px < 0) continue; m.px = px; }
        if (f.size() == 6) { if (f[5].size() > 5) continue; m.st = f[5]; }   // (2.2.0)
        std::vector<M>& v = out[t]; bool dup = false; for (const M& x : v) if (x.code == m.code) dup = true; if (dup) continue;
        if (c == 'I') v.insert(v.begin(), m); else v.push_back(m); ++n;
    }
    return head ? n : -1;
}

// (2.6.0) the live Cx? file (RevRead\\wylive-<MKT>-<spb>.txt, lsRevRead 0.25.0): header WYL|1|<MKT>|..., then at most one "stamp|Cx?|dir" line.
// true + t / dir when there is one for this market; false for none, a foreign header or a garbled line
inline bool parseLive(const std::string& txt, const std::string& mkt, long long& t, int& dir) {
    t = 0; dir = 0; size_t a = 0; bool head = false;
    while (a < txt.size()) {
        size_t e = txt.find('\n', a); if (e == std::string::npos) e = txt.size();
        std::string l = txt.substr(a, e - a); a = e + 1;
        if (!l.empty() && l[l.size() - 1] == '\r') l.erase(l.size() - 1);
        if (l.empty()) continue;
        if (!head) { if (l.compare(0, 6, "WYL|1|") != 0 || l.compare(6, mkt.size() + 1, mkt + "|") != 0 || mkt.empty()) return false; head = true; continue; }
        const size_t p1 = l.find('|'); if (p1 == std::string::npos || l.compare(p1, 5, "|Cx?|") != 0) return false;
        char* e1 = nullptr; const long long tt = std::strtoll(l.c_str(), &e1, 10); if (!e1 || *e1 != '|' || tt <= 0) return false;
        const std::string ds = l.substr(p1 + 5); if (ds != "1" && ds != "-1") return false;
        t = tt; dir = ds == "1" ? 1 : -1; return true;
    }
    return false;
}

// (2.1.0) the key / volume levels the pane published: L|px(ticks)|name lines (names <= 24 chars)
inline int parseLevels(const std::string& txt, std::vector<std::pair<long long, std::string> >& out) {
    out.clear(); size_t a = 0; bool head = false; int n = 0;
    while (a < txt.size()) {
        size_t e = txt.find('\n', a); if (e == std::string::npos) e = txt.size();
        std::string l = txt.substr(a, e - a); a = e + 1;
        if (!l.empty() && l[l.size() - 1] == '\r') l.erase(l.size() - 1);
        if (l.empty()) continue;
        if (!head) { if (l.compare(0, 5, "WY|1|") != 0) return -1; head = true; continue; }
        if (l.compare(0, 2, "L|") != 0) continue;
        const size_t p2 = l.find('|', 2); if (p2 == std::string::npos) continue;
        char* end = nullptr; const long long px = std::strtoll(l.c_str() + 2, &end, 10); if (!end || *end != '|' || px <= 0) continue;
        const std::string nm = l.substr(p2 + 1); if (nm.empty() || nm.size() > 24 || nm.find('|') != std::string::npos) continue;
        out.push_back(std::make_pair(px, nm)); ++n;
    }
    return head ? n : -1;
}
// (2.1.0) Central wall-clock seconds of a candle (days since 1970-01-01 x 86400 + seconds of the day) - the SignalBoard bar key every plugin uses
inline long long civilDays(int y, unsigned m, unsigned d) { y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; return era * 146097 + (long long)doe - 719468; }

// (1.0.0 interface, kept) the I letters only: candle stamp -> +1 / -1
inline int parse(const std::string& txt, std::map<long long, int>& out) {
    out.clear(); std::map<long long, std::vector<M> > all; const int n = parseAll(txt, all); if (n < 0) return -1; int k = 0;
    for (const auto& kv : all) for (const M& m : kv.second) if (m.code == 'I') { out[kv.first] = m.dir; ++k; }
    return k;
}

// (1.0.0) where an I tag goes: OVER a green candle (close >= open), UNDER a red one
struct Tag { int l = 0, t = 0, r = 0, b = 0; bool ok = false; bool over = false; };
inline Tag place(int x, int yHigh, int yLow, bool greenCandle, int textW, bool cxSameSide, int paneTop, int paneBottom) {
    Tag g; g.over = greenCandle; const int w = (textW > 0 ? textW : 4) + 2 * PAD_X; const int step = cxSameSide ? CX_STEP : 0;
    g.l = x - w / 2; g.r = g.l + w;
    if (greenCandle) { g.b = yHigh - GAP - step; g.t = g.b - TAG_H; } else { g.t = yLow + GAP + step; g.b = g.t + TAG_H; }
    g.ok = g.t >= paneTop + 1 && g.b <= paneBottom - 1;
    return g;
}

// (2.0.0) the layout of every tag in view, so none overlaps a candle or another tag
struct Candle { int x = 0, yHigh = 0, yLow = 0, halfW = 2; bool green = true; };
struct Want { int candle = 0; int w = 0, h = TAG_H; int side = 0; };   // (2.3.0) side: 0 = by the candle colour, +1 over, -1 under            // one tag of a candle (the I first, then the level mark), its size in px
struct Box { int l = 0, t = 0, r = 0, b = 0; bool ok = false; };
inline bool hit(const Box& a, const Box& b) { return !(a.r < b.l || a.l > b.r || a.b < b.t || a.t > b.b); }
inline std::vector<Box> layout(const std::vector<Candle>& C, const std::vector<Want>& W, int paneTop, int paneBottom, int xRight, const std::vector<Box>& taken = std::vector<Box>()) {
    std::vector<Box> placed(taken); placed.reserve(taken.size() + C.size() + W.size());   // (2.1.0) taken = other indicators' tags: never covered
    for (const Candle& c : C) { Box b; b.l = c.x - c.halfW; b.r = c.x + c.halfW; b.t = std::min(c.yHigh, c.yLow); b.b = std::max(c.yHigh, c.yLow); placed.push_back(b); }
    std::vector<Box> out(W.size());
    for (size_t k = 0; k < W.size(); ++k) {
        const Want& w = W[k]; if (w.candle < 0 || w.candle >= (int)C.size()) continue; const Candle& c = C[(size_t)w.candle];
        Box got; bool found = false;
        for (int sidePass = 0; sidePass < 2 && !found; ++sidePass) {
            const bool pref = w.side == 0 ? c.green : w.side > 0; const bool over = sidePass == 0 ? pref : !pref;
            for (int r = 0; r < 14 && !found; ++r) {
                const int t = over ? std::min(c.yHigh, c.yLow) - GAP - w.h - r * (w.h + 3) : std::max(c.yHigh, c.yLow) + GAP + r * (w.h + 3);
                if (t < paneTop + 1 || t + w.h > paneBottom - 1) continue;
                for (int nx : { 0, -5, 5, -9, 9 }) {
                    Box b; b.l = c.x + nx - w.w / 2; b.r = b.l + w.w; b.t = t; b.b = t + w.h; if (b.r > xRight) continue;
                    bool clear = true; for (const Box& p : placed) if (hit(b, p)) { clear = false; break; }
                    if (clear) { got = b; got.ok = true; found = true; break; } } } }
        if (found) { placed.push_back(got); out[k] = got; }
    }
    return out;
}

// (2.7.0) lsTurn's line per Abs candle (RevRead\\turnabs-<MKT>-<spb>.txt): header "TAB|1|<MKT>|<spb>|<ver>", then "stamp|dir|P|plus|exq":
// P = the biggest single attacking order inside the absorbed end quarter was the biggest of its side in P candles (-1 = not known: no order
// feed for that candle); plus 1 = the pane's history ran out first ("P12+"); exq 1 = the attackers' last minute fell to 0.5x or less (Ex?)
struct TurnAbs { int dir = 0; int p = -1; bool plus = false; bool exq = false; };
inline bool parseTurnAbs(const std::string& txt, const std::string& mkt, std::map<long long, TurnAbs>& out) {
    out.clear(); size_t a = 0; bool head = false;
    while (a < txt.size()) {
        size_t e = txt.find('\n', a); if (e == std::string::npos) e = txt.size();
        std::string l = txt.substr(a, e - a); a = e + 1;
        if (!l.empty() && l[l.size() - 1] == '\r') l.erase(l.size() - 1);
        if (l.empty()) continue;
        std::vector<std::string> f; size_t p = 0; while (true) { const size_t q = l.find('|', p); f.push_back(l.substr(p, q == std::string::npos ? std::string::npos : q - p)); if (q == std::string::npos) break; p = q + 1; }
        if (!head) { if (f.size() < 3 || f[0] != "TAB" || f[1] != "1" || f[2] != mkt) return false; head = true; continue; }
        if (f.size() != 5) continue;
        char* x = nullptr; const long long t = std::strtoll(f[0].c_str(), &x, 10); if (!x || *x || t <= 0) continue;
        const long d = std::strtol(f[1].c_str(), &x, 10); if (!x || *x || (d != 1 && d != -1)) continue;
        const long pv = std::strtol(f[2].c_str(), &x, 10); if (!x || *x || pv < -1 || pv > 100000) continue;
        if ((f[3] != "0" && f[3] != "1") || (f[4] != "0" && f[4] != "1")) continue;
        TurnAbs z; z.dir = (int)d; z.p = (int)pv; z.plus = f[3] == "1"; z.exq = f[4] == "1"; out[t] = z;
    }
    return head;
}
// (2.7.0) the badge's three lines: "Abs" or "Abs/Ex?" / the position / "P16" (left off under P5, as agreed: no "P0")
inline std::string absLine1(const TurnAbs* z) { return z && z->exq ? "Abs/Ex?" : "Abs"; }
inline std::string absLine3(const TurnAbs* z) { if (!z || z->p < 0) return std::string(); return "P" + std::to_string(z->p) + (z->plus ? "+" : ""); }   // (2.8.0, Rassul 12:51 CT "3. yes") always shown once known (was: left off under P5)
// (2.8.0) lsTurn 1.6.0's Ex? of every candle it read whole (RevRead\\turnexq-<MKT>-<spb>.txt): "TEQ|1|<MKT>|<spb>|<ver>", then "stamp|sellers|buyers"
// (1 = the attackers' last minute 0.5x or less of their first 2 minutes). false = a missing / foreign / garbled header (no lines kept)
inline bool parseTeq(const std::string& txt, const std::string& mkt, long long spb, std::map<long long, std::pair<int, int> >& out) {
    out.clear(); size_t a = 0; bool head = false;
    while (a < txt.size()) {
        size_t e = txt.find('\n', a); if (e == std::string::npos) e = txt.size();
        std::string l = txt.substr(a, e - a); a = e + 1;
        if (!l.empty() && l[l.size() - 1] == '\r') l.erase(l.size() - 1);
        if (l.empty()) continue;
        std::vector<std::string> f; size_t p = 0; while (true) { const size_t q = l.find('|', p); f.push_back(l.substr(p, q == std::string::npos ? std::string::npos : q - p)); if (q == std::string::npos) break; p = q + 1; }
        if (!head) { if (f.size() < 4 || f[0] != "TEQ" || f[1] != "1" || f[2] != mkt || f[3] != std::to_string(spb)) return false; head = true; continue; }
        if (f.size() != 3 || (f[1] != "0" && f[1] != "1") || (f[2] != "0" && f[2] != "1")) continue;
        char* x = nullptr; const long long t = std::strtoll(f[0].c_str(), &x, 10); if (!x || *x || t <= 0) continue;
        out[t] = std::make_pair(f[1] == "1" ? 1 : 0, f[2] == "1" ? 1 : 0);
    }
    return head;
}
// (2.8.0) the Tst line-3 decisions lsRevRead made (RevRead\\tstl3-<MKT>-<spb>.txt): "TL3|1|<MKT>|<spb>|<ver>", then "stamp|code" with code
// A+E / E / A! / - (blank). The first decision of a Tst is kept for good (no repaint across restarts); the newest 2,000 are kept.
inline bool tl3Ok(const std::string& c) { return c == "A+E" || c == "E" || c == "A!" || c.empty(); }
inline std::string tl3Text(const std::map<long long, std::string>& m, const std::string& mkt, long long spb, const std::string& ver) {
    std::string t = "TL3|1|" + mkt + "|" + std::to_string(spb) + "|" + ver + "\n"; size_t skip = m.size() > 2000 ? m.size() - 2000 : 0;
    for (const auto& kv : m) { if (skip) { --skip; continue; } t += std::to_string(kv.first) + "|" + (kv.second.empty() ? std::string("-") : kv.second) + "\n"; }
    return t; }
inline bool parseTl3(const std::string& txt, const std::string& mkt, long long spb, std::map<long long, std::string>& out) {
    out.clear(); size_t a = 0; bool head = false;
    while (a < txt.size()) {
        size_t e = txt.find('\n', a); if (e == std::string::npos) e = txt.size();
        std::string l = txt.substr(a, e - a); a = e + 1;
        if (!l.empty() && l[l.size() - 1] == '\r') l.erase(l.size() - 1);
        if (l.empty()) continue;
        if (!head) { if (l.compare(0, 6, "TL3|1|") != 0 || l.compare(6, mkt.size() + std::to_string(spb).size() + 2, mkt + "|" + std::to_string(spb) + "|") != 0) return false; head = true; continue; }
        const size_t p = l.find('|'); if (p == std::string::npos) continue;
        char* x = nullptr; const long long t = std::strtoll(l.c_str(), &x, 10); if (!x || *x != '|' || t <= 0) continue;
        std::string c = l.substr(p + 1); if (c == "-") c.clear(); if (!tl3Ok(c)) continue;
        out[t] = c;
    }
    return head;
}
} // namespace wyi
