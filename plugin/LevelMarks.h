// LevelMarks.h 1.0.0 (2026-11-19) -- Bk / Sw / Cx on the price candles, each with ONE level (lsRevRead / lsRevReadMarks 0.19.0). SDK-free, tested
// (tests_levelmarks). Rassul 2026-11-19 10:20 CT "lets show bk sw sr and cx and the level they apply to . but be mindfull of not overlapping one
// another and putting lettes on top of green and below red as well as indicating the key level they apply to .. show mockup"; 10:26 "if you showed
// it without the price could you put it in", "but on 2nd line", "and abbreviate like Op or LLO or LHI"; 10:31 "you can also mention other levels
// that it is breaking even the highs like PDH"; 10:34 "to simplify, we can mention only 1 level giving priority first to session levels like
// overnight, then pd levels, pf, then volume levels"; 10:37 "ok, cut repeats"; 10:40 "go" (mockup level-marks-mockup.html v8).
//   Bk  a candle CLOSES through a level (the close before was on the other side). Key / volume / weekly levels: any such close; hour levels: the
//       candle's volume is the biggest in 5+. Not when the same level was crossed on a close in the 10 candles before (chop). dir = the close's way.
//   Sw  one failed ATTEMPT: from the first candle that trades through a level nobody broke since it formed, while the next candles keep trading
//       through it (3 candles at most); marked on the attempt's deepest candle when a candle at / after it (within 3) closes back on the original
//       side with no new extreme first; the attempt's biggest volume >= the biggest of the 5 candles before it. dir = up for a swept low.
//   Cx  a ZigZag (3-bar High / Low) pivot candle, or the candle before it, with the biggest volume in 20+; decided when the pivot is confirmed;
//       green at a pivot low, red at a pivot high; its levels = the levels inside the candle.
//   One mark per candle (Sw, then Bk, then Cx - first come), one level per mark by priority: session (ON, London, Asia, Open) > prior day >
//   prior full session > volume > weekly > hours (prior hour PHL / PHH, then 2h / 3h back). MenthorQ / gamma names do not label a mark.
//   Repeats cut: a code shows at most once per level per session unless price went 1+ ATR(13) away from the level and came back.
// Pure function of the closed candles and the levels: the same candles give the same marks (no repaint).
#pragma once
#include "WyRead.h"
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace lvm {
static const char* const VERSION = "1.1.0";   // (1.1.0, Rassul 10:50 CT "ok build": the BO indicator's Rly gets its level too - levelNear(); same priority / short names)

struct Mark { char code = 0; int dir = 0; long long px = 0; std::string lv; };   // code 'B' Bk / 'S' Sw / 'C' Cx; lv = the short level name ("" = none)
inline const char* codeText(char c) { return c == 'B' ? "Bk" : c == 'S' ? "Sw" : c == 'C' ? "Cx" : ""; }

// the priority of a level name (lower = shown first); -1 = this name never labels a mark (MenthorQ, gamma, VWAP ...)
inline int pri(const std::string& n, int hb) {
    if (hb > 0) return hb == 1 ? 50 : 60;
    static const char* const S0[] = { "ONL", "ONH", "ONH/LonHI", "ONL/LonLO", "LonLO", "LonHI", "AsiaLO", "AsiaHI", "Open" };
    static const char* const S1[] = { "PDH", "PDL", "PFL/PDL", "PFH/PDH" };
    static const char* const S2[] = { "PFH", "PFL" };
    static const char* const S3[] = { "VAH", "VAL", "DPoc", "WPoc" };
    static const char* const S4[] = { "PWH", "PWL", "CWH", "CWL" };
    static const char* const S5[] = { "PHL", "PHH" };
    for (const char* x : S0) if (n == x) return 0;
    for (const char* x : S1) if (n == x) return 10;
    for (const char* x : S2) if (n == x) return 20;
    for (const char* x : S3) if (n == x) return 30;
    for (const char* x : S4) if (n == x) return 40;
    for (const char* x : S5) if (n == x) return 50;
    return -1;
}
inline std::string shortName(const std::string& n, int hb, bool high) {
    if (hb > 0) return hb == 1 ? (high ? "PHH" : "PHL") : std::to_string(hb) + "h" + (high ? "H" : "L");
    static const std::pair<const char*, const char*> M[] = { { "Open", "Op" }, { "LonLO", "LLO" }, { "LonHI", "LHI" }, { "ONH/LonHI", "ONH" },
        { "ONL/LonLO", "ONL" }, { "AsiaLO", "ALO" }, { "AsiaHI", "AHI" }, { "PFL/PDL", "PDL" }, { "PFH/PDH", "PDH" }, { "DPoc", "DP" }, { "WPoc", "WP" } };
    for (const auto& m : M) if (n == m.first) return m.second;
    return n;
}

struct Lv { long long px = 0; std::string name; int from = 0; int hb = 0; bool high = false; };
// the levels at candle i: the key / volume levels (from the session start) + the high / low of the 3 full clock hours before i's hour
inline std::vector<Lv> levelsAt(const std::vector<wyr::Bar>& B, int i, const std::vector<wyr::Lvl>& key) {
    std::vector<Lv> v; for (const wyr::Lvl& z : key) { if (z.px <= 0 || pri(z.name, 0) < 0) continue;   // MenthorQ / gamma / VWAP never make or label a mark
        Lv a; a.px = z.px; a.name = z.name; a.from = 0; v.push_back(a); }
    if (B.empty() || i <= 0) return v;
    const long long hp = wyr::hourOf(B[(size_t)i].t), h0 = wyr::hourOf(B[0].t);
    for (long long hh = hp - 1; hh >= hp - 3; --hh) {
        if (hh <= h0) break;
        long long lo = 0, hi = 0; int last = -1;
        for (int k = 0; k < i; ++k) if (wyr::hourOf(B[(size_t)k].t) == hh) { if (last < 0) { lo = B[(size_t)k].l; hi = B[(size_t)k].h; } else { lo = std::min(lo, B[(size_t)k].l); hi = std::max(hi, B[(size_t)k].h); } last = k; }
        if (last < 0) continue;
        Lv a; a.px = lo; a.from = last + 1; a.hb = (int)(hp - hh); a.high = false; a.name = wyr::hourName(hh) + " hour low"; v.push_back(a);
        Lv b = a; b.px = hi; b.high = true; b.name = wyr::hourName(hh) + " hour high"; v.push_back(b);
    }
    return v;
}

struct Cand { long long px = 0; std::string name; int hb = 0; bool high = false; };
// the one level a mark shows: the best priority among its candidates (ties: the first given); none labels -> empty
inline bool pick(const std::vector<Cand>& c, Cand& out) {
    int best = 1 << 30; bool ok = false;
    for (const Cand& z : c) { const int p = pri(z.name, z.hb); if (p < 0) continue; if (p < best) { best = p; out = z; ok = true; } }
    return ok;
}

inline std::map<long long, Mark> run(const std::vector<wyr::Bar>& B, const std::vector<wyr::Lvl>& key) {
    std::map<long long, Mark> out; const int n = (int)B.size(); if (n < 6) return out;
    std::vector<double> v((size_t)n); for (int i = 0; i < n; ++i) v[(size_t)i] = (double)(B[(size_t)i].buy + B[(size_t)i].sell);
    auto maxV = [&](int a, int b) { double m = 0; for (int k = std::max(0, a); k < b; ++k) m = std::max(m, v[(size_t)k]); return m; };
    const std::vector<double> atr = wyr::atr13(B);
    std::map<int, std::pair<char, std::vector<Cand> > > raw;   // candle -> code + its candidate levels (mockup order: Sw, then Bk, then Cx)
    std::map<int, int> rawDir;
    // Sw: one attempt per level
    { std::set<std::pair<long long, std::string> > done;
      for (int f = 5; f < n; ++f) {
        const std::vector<Lv> L = levelsAt(B, f, key);
        for (const Lv& z : L) {
            const std::pair<long long, std::string> id(z.px, z.name); if (done.count(id)) continue;
            const bool sup = B[(size_t)f - 1].c > z.px;
            auto thr = [&](int k) { return sup ? B[(size_t)k].l < z.px : B[(size_t)k].h > z.px; };
            if (!thr(f)) continue;
            bool before = false; for (int k = std::max(0, z.from); k < f; ++k) if (thr(k)) { before = true; break; }
            if (before) continue;
            done.insert(id);
            std::vector<int> att(1, f); while (att.back() + 1 < n && att.size() < 3 && thr(att.back() + 1)) att.push_back(att.back() + 1);
            int dpt = att[0]; for (int k : att) if (sup ? B[(size_t)k].l < B[(size_t)dpt].l : B[(size_t)k].h > B[(size_t)dpt].h) dpt = k;
            int back = -1; for (int k = dpt; k < std::min(n, dpt + 3); ++k) if (sup ? B[(size_t)k].c > z.px : B[(size_t)k].c < z.px) { back = k; break; }
            if (back < 0) continue;
            bool newExt = false; for (int k = dpt + 1; k <= back; ++k) if (sup ? B[(size_t)k].l < B[(size_t)dpt].l : B[(size_t)k].h > B[(size_t)dpt].h) newExt = true;
            if (newExt) continue;
            double am = 0; for (int k : att) am = std::max(am, v[(size_t)k]);
            if (am < maxV(f - 5, f)) continue;
            Cand c; c.px = z.px; c.name = z.name; c.hb = z.hb; c.high = z.high;
            auto it = raw.find(dpt);
            if (it != raw.end()) { if (it->second.first == 'S') it->second.second.push_back(c); continue; }
            raw[dpt] = std::make_pair('S', std::vector<Cand>(1, c)); rawDir[dpt] = sup ? 1 : -1;
        } } }
    // Bk: closes through a level
    for (int i = 5; i < n; ++i) {
        const bool big = v[(size_t)i] >= maxV(i - 5, i);
        std::vector<std::pair<long long, Cand> > crossed;   // distance from the close before -> level
        for (const Lv& z : levelsAt(B, i, key)) {
            const long long pc = B[(size_t)i - 1].c, c = B[(size_t)i].c;
            if (!((pc > z.px && c < z.px) || (pc < z.px && c > z.px))) continue;
            if (z.hb > 0 && !big) continue;
            bool chop = false; for (int k = std::max(1, i - 10); k < i; ++k) if ((B[(size_t)k - 1].c > z.px) != (B[(size_t)k].c > z.px)) { chop = true; break; }
            if (chop) continue;
            Cand cd; cd.px = z.px; cd.name = z.name; cd.hb = z.hb; cd.high = z.high; crossed.push_back(std::make_pair(std::llabs(z.px - pc), cd));
        }
        if (crossed.empty() || raw.count(i)) continue;
        std::stable_sort(crossed.begin(), crossed.end(), [](const std::pair<long long, Cand>& a, const std::pair<long long, Cand>& b) { return a.first > b.first; });
        std::vector<Cand> cs; for (const auto& x : crossed) { bool dup = false; for (const Cand& y : cs) if (y.name == x.second.name) dup = true; if (!dup) cs.push_back(x.second); }
        if (cs.size() > 4) cs.resize(4);
        raw[i] = std::make_pair('B', cs); rawDir[i] = B[(size_t)i].c < B[(size_t)i - 1].c ? -1 : 1;
    }
    // Cx: pivots confirmed (ZigZag leg starts), the pivot candle or the one before, biggest volume in 20+
    { const std::vector<wyr::Leg> LG = wyr::legs(B); int prevA = -1;
      std::vector<std::pair<int, int> > piv;   // pivot candle, side (+1 low / -1 high), in order of confirmation
      for (int i = 0; i < n; ++i) { const int a = LG[(size_t)i].a; if (a != prevA && i > 0) { bool seen = false; for (const auto& p : piv) if (p.first == a) seen = true; if (!seen) piv.push_back(std::make_pair(a, LG[(size_t)i].dir == 1 ? 1 : -1)); } prevA = a; }
      for (const auto& p : piv) { const int k = p.first, side = p.second;
        for (int kk : { k, k - 1 }) {
            if (kk < 20 || v[(size_t)kk] < maxV(kk - 20, kk) || raw.count(kk)) continue;
            const long long ext = side > 0 ? B[(size_t)k].l : B[(size_t)k].h;
            std::vector<std::pair<long long, Cand> > in;
            for (const Lv& z : levelsAt(B, kk, key)) if (B[(size_t)kk].l <= z.px && z.px <= B[(size_t)kk].h) { Cand cd; cd.px = z.px; cd.name = z.name; cd.hb = z.hb; cd.high = z.high; in.push_back(std::make_pair(std::llabs(z.px - ext), cd)); }
            std::stable_sort(in.begin(), in.end(), [](const std::pair<long long, Cand>& a, const std::pair<long long, Cand>& b) { return a.first < b.first; });
            std::vector<Cand> cs; for (size_t q = 0; q < in.size() && q < 4; ++q) cs.push_back(in[q].second);
            raw[kk] = std::make_pair('C', cs); rawDir[kk] = side; break; } } }
    // one level each, repeats cut (per code + level, in candle order)
    std::map<std::pair<char, std::string>, std::pair<int, long long> > last;
    for (const auto& kv : raw) {
        const int i = kv.first; Mark m; m.code = kv.second.first; m.dir = rawDir[i];
        Cand c; if (pick(kv.second.second, c)) { m.px = c.px; m.lv = shortName(c.name, c.hb, c.high); }
        if (!m.lv.empty()) {
            const std::pair<char, std::string> id(m.code, m.lv); auto it = last.find(id);
            if (it != last.end()) { bool away = false; for (int k = it->second.first + 1; k < i; ++k) if (std::fabs((double)(B[(size_t)k].c - m.px)) >= atr[(size_t)k] && atr[(size_t)k] > 0) { away = true; break; }
                if (!away) continue; }
            last[id] = std::make_pair(i, m.px);
        }
        out[B[(size_t)i].t] = m;
    }
    return out;
}


// (1.1.0) the one level AT candle i for another indicator's tag (the BO indicator's Rly): a level inside the candle's range or within tol ticks of
// its extreme ext; same priority / short names as the marks; false = none
inline bool levelNear(const std::vector<wyr::Bar>& B, int i, const std::vector<wyr::Lvl>& key, long long ext, long long tol, std::string& shortOut, long long& pxOut) {
    if (i < 0 || i >= (int)B.size()) return false;
    std::vector<std::pair<long long, Cand> > in;
    for (const Lv& z : levelsAt(B, i, key)) {
        const bool inside = B[(size_t)i].l <= z.px && z.px <= B[(size_t)i].h, closeBy = std::llabs(z.px - ext) <= tol;   // (never "near": a Windows macro)
        if (!inside && !closeBy) continue;
        Cand c; c.px = z.px; c.name = z.name; c.hb = z.hb; c.high = z.high; in.push_back(std::make_pair(std::llabs(z.px - ext), c)); }
    std::stable_sort(in.begin(), in.end(), [](const std::pair<long long, Cand>& a, const std::pair<long long, Cand>& b) { return a.first < b.first; });
    std::vector<Cand> cs; for (const auto& x : in) cs.push_back(x.second);
    Cand c; if (!pick(cs, c)) return false;
    shortOut = shortName(c.name, c.hb, c.high); pxOut = c.px; return true;
}

// the I's level as a short name ("" when its level does not label marks)
inline std::string iLevel(const std::string& name, int hb, bool high) { if (pri(name, hb) < 0) return ""; return shortName(name, hb, high); }
} // namespace lvm
