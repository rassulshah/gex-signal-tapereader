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
//       side with no new extreme first; (1.8.0: no volume gate - the Sw candle's own BVMax / BRMax >= 20 instead). dir = up for a swept low.
//   Cx  a ZigZag (3-bar High / Low) pivot candle, or the candle before it, with the biggest volume in 20+; decided when the pivot is confirmed;
//       green at a pivot low, red at a pivot high; its levels = the levels inside the candle.
//   One mark per candle (Sw, then Bk, then Cx - first come), one level per mark by priority: session (ON, London, Asia, Open) > prior day >
//   prior full session > volume > weekly > hours (prior hour PHL / PHH, then 2h / 3h back). MenthorQ / gamma names do not label a mark.
//   Repeats cut: a code shows at most once per level per session unless price went 1+ ATR(13) away from the level and came back.
// Pure function of the closed candles and the levels: the same candles give the same marks (no repaint).
#pragma once
#include "WyRead.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace lvm {
static const char* const VERSION = "1.10.1";   // (1.10.1, Rassul 2026-11-20 20:27 CT "first do manual testing": the hand check found EU 10-15 10:03 marked a high sweep of 1hL when the candle before closed exactly ON the level - a close on the level has no side, so no Sw from it)   // was "1.10.0";   // (1.10.0, Rassul 2026-11-20 16:44 CT "yes, use strict rule"): a Sw needs a level nobody TRADED through since it formed (wicks count, one attempt per level - the 1.8.0 rule), and the mark is LABELLED with the best key level its candle crossed (wick through, close back) even if that key level was touched earlier (GC 11-20 9:00: swept the untouched 1-hour high 3,582.8, crossed PhrH 3,582.4 -> "PhrH"); 1.9.0's formation start, combined names, repeat fallback and 1hH / PhrH names stay
// was VERSION = "1.9.0";   // (1.9.0, Rassul 2026-11-20 14:20 CT "the 9:09 was a lonHi" - NQ 9:00 swept LonHI 26,447.75 but was labelled PHH): a SESSION level (ON / London / Asia / the pre-open hour / Open) counts "unbroken" from the candle that FORMED it (its high / low = the level), not from 5 PM - Asia trading above the later LonHI hid it; combined names ("ONL/PHL") take the best part's priority and short name instead of being dropped
// was VERSION = "1.8.0";   // (1.8.0, Rassul 2026-11-20 10:34-10:37 CT "look at the sw signal, shouldnt it meet a threshold of 20", "the red sweep bar doesnt meet the threshold.. fix", "I should only be on key levels that are named like overnight etc.. and we should have a 20 threshold just like sweeps"; lsRevRead 0.21.3): the Sw needs the Sw CANDLE ITSELF at BVMax or BRMax >= 20 and its strength line is that candle's own (was the max over the attempt candles - HG 11-20 08:03 a weak red Sw got V99+ from the green candle before it); the attempt-volume gate is gone (11:00 CT "shouldnt this be a sweep", ES 09:00 ONH); iPick() names only session / PD / PF / volume / weekly levels - never an hour level (PHH / PHL / 2h / 3h); I_MIN 20 = the I candle's own BVMax / BRMax minimum
// was VERSION = "1.7.0";   // (1.7.0, Rassul 2026-11-20 09:20 CT "add [the I] ... if both sweep and i are true, only show sweep", 09:24 "build"; lsRevRead 0.21.2): iPick() = the ONE level an I names, by the Bk / Sw priority (session > PD > PF > volume > weekly > hours; a tie = the furthest reclaimed); iUnderSw() = the I candle is also a Sw of the same direction (only the Sw shows); no Bk / Sw / Cx rule change
// was VERSION = "1.6.0";   // (1.6.0, Rassul 2026-11-20 07:50 CT "update the sweep threshold to 20": a Sw needs its attempt's BVMax or BRMax >= 20 (SW_MIN), was 10; the strength line still shows from 10 up (ST_MIN unchanged))   // was "1.5.0"   // (1.5.0, Rassul 2026-11-19 18:29 CT "lets keep it at 10 so the BRmax or BVmax should be at least 10": a Sw needs its attempt's BVMax or BRMax >= 10 (SW_MIN), else no Sw)   // was "1.4.0"   // (1.4.0, Rassul 2026-11-19 18:19 CT "switch to 10": the strength line shows from 10 up, was 20)   // was "1.3.0"   // (1.3.0, Rassul 2026-11-19 15:35-16:58 CT, mockup price-pane-cleanup-mockup.html, "1. ok 2. ok 3. ok"): Bk needs the BO candle rule (range >= the prior candle's and range or volume up); a bullish Sw is a GREEN candle, a bearish Sw a RED one; DPoc / WPoc / Open are not levels any more; + seq(): FT / Tst / Rev after a Bk (the BO indicator's FT, Pb and PB Rev rules on the Bk)
// was VERSION = "1.2.0";   // (1.2.0, Rassul 2026-11-19 11:22-11:30 CT "sweeps of key levels on big volume or range are very meaningfull", "maybe consider sw and a second line like v50 or r50, same with breaks", "use the max numbers that we have ... biggest in green bars, red bars, consecutive", "ok.. good build it"): Bk / Sw carry a STRENGTH = his BVMax / BRMax (the Volume / Range panes' Max number) - "V<N>" / "R<N>" from 20 up, "99+" from 99; a Sw takes the max over its attempt's candles
// was VERSION = "1.1.0";   // (1.1.0, Rassul 10:50 CT "ok build": the BO indicator's Rly gets its level too - levelNear(); same priority / short names)

struct Mark { char code = 0; int dir = 0; long long px = 0; std::string lv; long long t0 = 0; std::string st; };   // (1.2.0) t0 = the first candle the strength looks at (a Sw: its attempt's first candle; else the mark's own); st = "V45" / "R99+" / ""   // code 'B' Bk / 'S' Sw / 'C' Cx; lv = the short level name ("" = none)
inline const char* codeText(char c) { return c == 'B' ? "Bk" : c == 'S' ? "Sw" : c == 'C' ? "Cx" : ""; }

// the priority of a level name (lower = shown first); -1 = this name never labels a mark (MenthorQ, gamma, VWAP ...)
inline int pri(const std::string& n, int hb);
// (1.9.0) a combined name "ONL/PHL" / "LonHI/PFL": the best priority of its parts (a part that labels nothing is skipped); -1 = none
inline int priParts(const std::string& n, std::string* bestPart = nullptr) {
    int best = -1; size_t a = 0;
    for (;;) { const size_t p = n.find('/', a); const std::string part = n.substr(a, p == std::string::npos ? std::string::npos : p - a);
        const int q = part.empty() || part == n ? -1 : pri(part, 0); if (q >= 0 && (best < 0 || q < best)) { best = q; if (bestPart) *bestPart = part; }
        if (p == std::string::npos) break;
        a = p + 1; }
    return best;
}
// (1.9.0) a level formed INSIDE the session (ON, London, Asia, the pre-open hour PHH / PHL of the level file, Open), alone or in a combined name
inline bool sessFormed(const std::string& n) {
    static const char* const S[] = { "ONH", "ONL", "LonHI", "LonLO", "AsiaHI", "AsiaLO", "PHH", "PHL", "Open" };
    size_t a = 0; for (;;) { const size_t p = n.find('/', a); const std::string part = n.substr(a, p == std::string::npos ? std::string::npos : p - a);
        for (const char* x : S) { if (part == x) return true; }
        if (p == std::string::npos) break;
        a = p + 1; }
    return false;
}
// (1.9.0) the clock window a session level is printed in (candle END, local): Asia (6 PM, 1 AM], London (2 AM, 7:30 AM], ON (5 PM, 8:30 AM],
// the pre-open hour PHH / PHL (6:20 AM, 8:30 AM], Open (7:20 AM, 8:40 AM]; a combined name: any of its parts
inline bool formWindow(const std::string& n, long long tEnd) {
    long long s = tEnd % 86400; if (s < 0) s += 86400; const int m = (int)(s / 60);
    size_t a = 0;
    for (;;) { const size_t p = n.find('/', a); const std::string q = n.substr(a, p == std::string::npos ? std::string::npos : p - a);
        if ((q == "AsiaHI" || q == "AsiaLO") && (m > 18 * 60 || m <= 60)) return true;
        if ((q == "LonHI" || q == "LonLO") && m > 120 && m <= 450) return true;
        if ((q == "ONH" || q == "ONL") && (m > 17 * 60 || m <= 510)) return true;
        if ((q == "PHH" || q == "PHL") && m > 380 && m <= 510) return true;
        if (q == "Open" && m > 440 && m <= 520) return true;
        if (p == std::string::npos) break;
        a = p + 1; }
    return false;
}
// (1.9.0) the window's end (minutes after midnight, candle END) of a session level's best-known part; -1 = not a session level
inline int formEnd(const std::string& n) {
    int e = -1; size_t a = 0;
    for (;;) { const size_t p = n.find('/', a); const std::string q = n.substr(a, p == std::string::npos ? std::string::npos : p - a);
        const int x = (q == "AsiaHI" || q == "AsiaLO") ? 60 : (q == "LonHI" || q == "LonLO") ? 450 : (q == "ONH" || q == "ONL" || q == "PHH" || q == "PHL") ? 510 : q == "Open" ? 520 : -1;
        if (x > e) e = x;
        if (p == std::string::npos) break;
        a = p + 1; }
    return e;
}
inline int pri(const std::string& n, int hb) {
    if (hb > 0) return hb == 1 ? 50 : 60;
    static const char* const S0[] = { "ONL", "ONH", "ONH/LonHI", "ONL/LonLO", "LonLO", "LonHI", "AsiaLO", "AsiaHI" };   // (1.3.0, 16:34 "just remove it") no Open
    static const char* const S1[] = { "PDH", "PDL", "PFL/PDL", "PFH/PDH" };
    static const char* const S2[] = { "PFH", "PFL" };
    static const char* const S3[] = { "VAH", "VAL" };   // (1.3.0, 16:30 "remove poc") no DPoc / WPoc
    static const char* const S4[] = { "PWH", "PWL", "CWH", "CWL" };
    static const char* const S5[] = { "PHL", "PHH" };
    for (const char* x : S0) if (n == x) return 0;
    for (const char* x : S1) if (n == x) return 10;
    for (const char* x : S2) if (n == x) return 20;
    for (const char* x : S3) if (n == x) return 30;
    for (const char* x : S4) if (n == x) return 40;
    for (const char* x : S5) if (n == x) return 50;
    if (n.find('/') != std::string::npos) return priParts(n);   // (1.9.0) a combined name not listed above
    return -1;
}
inline std::string shortName(const std::string& n, int hb, bool high) {
    if (hb > 0) return std::to_string(hb) + "h" + (high ? "H" : "L");   // (1.9.0, Rassul 14:26 CT "the sweep label changes") the prior hour = 1hH / 1hL (was PHH / PHL)
    static const std::pair<const char*, const char*> M[] = { { "PHH", "PhrH" }, { "PHL", "PhrL" },   // (1.9.0) the level file's pre-open hour, as his chart names it
        { "Open", "Op" }, { "LonLO", "LLO" }, { "LonHI", "LHI" }, { "ONH/LonHI", "ONH" },
        { "ONL/LonLO", "ONL" }, { "AsiaLO", "ALO" }, { "AsiaHI", "AHI" }, { "PFL/PDL", "PDL" }, { "PFH/PDH", "PDH" }, { "DPoc", "DP" }, { "WPoc", "WP" } };
    for (const auto& m : M) if (n == m.first) return m.second;
    if (n.find('/') != std::string::npos) { std::string b; if (priParts(n, &b) >= 0 && !b.empty()) return shortName(b, 0, high); }   // (1.9.0) the best part's short name
    return n;
}

struct Lv { long long px = 0; std::string name; int from = 0; int hb = 0; bool high = false; };
// the levels at candle i: the key / volume levels (from the session start) + the high / low of the 3 full clock hours before i's hour
inline std::vector<Lv> levelsAt(const std::vector<wyr::Bar>& B, int i, const std::vector<wyr::Lvl>& key) {
    std::vector<Lv> v; for (const wyr::Lvl& z : key) { if (z.px <= 0 || pri(z.name, 0) < 0) continue;   // MenthorQ / gamma / VWAP never make or label a mark
        Lv a; a.px = z.px; a.name = z.name; a.from = 0;
        // (1.9.0, Rassul 14:20 CT "the 9:09 was a lonHi") a SESSION level (ON / London / Asia / the pre-open hour / Open) exists from the candle
        // that printed it (the first candle whose high or low is the level): before it, it is not a level (no Sw / Bk / Cx of it); its "unbroken"
        // check starts after that candle - trading beyond it earlier (Asia above the later LonHI) does not use it up
        if (sessFormed(z.name)) {
            int fk = -1; for (int k = 0; k < i && k < (int)B.size(); ++k) if ((B[(size_t)k].h == z.px || B[(size_t)k].l == z.px) && formWindow(z.name, B[(size_t)k].t)) { fk = k; break; }
            if (fk < 0) {   // no candle printed it exactly (another data source): it exists from the first candle after its window
                const int e = formEnd(z.name);
                for (int k = 0; k < i && k < (int)B.size(); ++k) { long long q = B[(size_t)k].t % 86400; if (q < 0) q += 86400; const int mm = (int)(q / 60); if (mm > e && mm < 17 * 60) { fk = k - 1; break; } }
                if (fk < -1 || fk == -1) { bool found = false; for (int k = 0; k < i && k < (int)B.size(); ++k) { long long q = B[(size_t)k].t % 86400; if (q < 0) q += 86400; const int mm = (int)(q / 60); if (mm > e && mm < 17 * 60) { found = true; break; } }
                    if (!found) continue; } }
            a.from = fk + 1; }
        v.push_back(a); }
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
    std::map<int, int> rawDir; std::map<int, int> rawA;   // (1.2.0) the mark candle -> the first candle of its Sw attempt
    // Sw: one attempt per level
    { std::set<std::pair<long long, std::string> > done;
      for (int f = 5; f < n; ++f) {
        const std::vector<Lv> L = levelsAt(B, f, key);
        for (const Lv& z : L) {
            const std::pair<long long, std::string> id(z.px, z.name); if (done.count(id)) continue;
            if (B[(size_t)f - 1].c == z.px) continue;   // (1.10.1) the close before sits ON the level: no side, so no sweep from it
            const bool sup = B[(size_t)f - 1].c > z.px;
            auto thr = [&](int k) { return sup ? B[(size_t)k].l < z.px : B[(size_t)k].h > z.px; };
            if (!thr(f)) continue;
            const int from = std::max(0, z.from);   // (1.9.0) a session level: after the candle that printed it (levelsAt)
            // (1.10.0, "use strict rule") nobody TRADED through it since it formed (a wick counts); the first candle through is its one attempt
            bool before = false; for (int k = from; k < f; ++k) if (thr(k)) { before = true; break; }
            if (before) continue;
            done.insert(id);
            std::vector<int> att(1, f); while (att.back() + 1 < n && att.size() < 3 && thr(att.back() + 1)) att.push_back(att.back() + 1);
            int dpt = att[0]; for (int k : att) if (sup ? B[(size_t)k].l < B[(size_t)dpt].l : B[(size_t)k].h > B[(size_t)dpt].h) dpt = k;
            int back = -1; for (int k = dpt; k < std::min(n, dpt + 3); ++k) if (sup ? B[(size_t)k].c > z.px : B[(size_t)k].c < z.px) { back = k; break; }
            if (back < 0) continue;
            bool newExt = false; for (int k = dpt + 1; k <= back; ++k) if (sup ? B[(size_t)k].l < B[(size_t)dpt].l : B[(size_t)k].h > B[(size_t)dpt].h) newExt = true;
            if (newExt) continue;
            double am = 0; for (int k : att) am = std::max(am, v[(size_t)k]);
            (void)am;   // (1.8.0, Rassul 11:00 CT "shouldnt this be a sweep" - ES 11-20 09:00 ONH: the attempt's volume was under the 08:54 breakout candle's, so no Sw) no volume gate any more: his sweep rule has none ("no volume requirement", 2026-10-26); the Sw candle's own BVMax / BRMax >= 20 (fillStrength) is the strength test
            if (sup ? !(B[(size_t)dpt].c > B[(size_t)dpt].o) : !(B[(size_t)dpt].c < B[(size_t)dpt].o)) continue;   // (1.3.0, 16:42 "a bullish sweep should be green and vice versa")
            Cand c; c.px = z.px; c.name = z.name; c.hb = z.hb; c.high = z.high;
            auto it = raw.find(dpt);
            if (it != raw.end()) { if (it->second.first == 'S') { it->second.second.push_back(c); rawA[dpt] = std::min(rawA[dpt], att[0]); } continue; }
            raw[dpt] = std::make_pair('S', std::vector<Cand>(1, c)); rawDir[dpt] = sup ? 1 : -1; rawA[dpt] = att[0];
        } } }
    // Bk: closes through a level
    for (int i = 5; i < n; ++i) {
        const bool big = v[(size_t)i] >= maxV(i - 5, i);
        { const long long r0 = B[(size_t)i].h - B[(size_t)i].l, r1 = B[(size_t)i - 1].h - B[(size_t)i - 1].l;   // (1.3.0, 16:32 "implement the range rule for breaks also and the either rule") the BO candle rule
          if (!(r0 >= r1 && (r0 > r1 || v[(size_t)i] > v[(size_t)i - 1]))) continue; }
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
    // (1.10.0) a Sw's label may be any KEY level its mark candle crossed (wick through it, close back on the other side), touched earlier or not:
    // the untouched level (often an hour high / low) decides THAT it is a sweep, the key level it crossed names it
    for (auto& kv : raw) { if (kv.second.first != 'S') continue; const int i = kv.first; const int d = rawDir[i]; const wyr::Bar& b = B[(size_t)i];
        for (const Lv& z : levelsAt(B, i, key)) { if (z.hb != 0 || pri(z.name, 0) < 0) continue;
            const int a0 = rawA.count(i) ? rawA[i] : i; const long long pc = a0 > 0 ? B[(size_t)a0 - 1].c : b.o;   // the close before the attempt: still on the level's original side
            const bool crossed = d > 0 ? (pc > z.px && b.l < z.px && b.c > z.px) : (pc < z.px && b.h > z.px && b.c < z.px); if (!crossed) continue;
            bool have = false; for (const Cand& x : kv.second.second) if (x.px == z.px && x.name == z.name) have = true;
            if (!have) { Cand c; c.px = z.px; c.name = z.name; c.hb = 0; c.high = d < 0; kv.second.second.push_back(c); } } }
    // one level each, repeats cut (per code + level, in candle order)
    std::map<std::pair<char, std::string>, std::pair<int, long long> > last;
    for (const auto& kv : raw) {
        const int i = kv.first; Mark m; m.code = kv.second.first; m.dir = rawDir[i];
        std::vector<Cand> cs = kv.second.second;
        if (m.code == 'S') std::stable_sort(cs.begin(), cs.end(), [&](const Cand& x, const Cand& y) { if ((x.hb == 0) != (y.hb == 0)) return x.hb == 0; return m.dir > 0 ? x.px < y.px : x.px > y.px; });   // (1.9.0) a tie in priority: a named level before a candle-hour level (GC 9:00 PhrH over the prior hour), then the deepest swept (ES 9:00 ONH over AsiaHI)
        auto isRepeat = [&](const std::string& lv, long long px) {
            const auto it = last.find(std::make_pair(m.code, lv)); if (it == last.end()) return false;
            for (int k = it->second.first + 1; k < i; ++k) if (std::fabs((double)(B[(size_t)k].c - px)) >= atr[(size_t)k] && atr[(size_t)k] > 0) return false;   // went 1+ ATR away and came back
            return true; };
        if (m.code == 'S') {   // (1.9.0) the best level that is not a repeat: a sweep is never dropped because its best level was swept earlier (it names the next one)
            std::vector<Cand> ord; for (const Cand& z : cs) if (pri(z.name, z.hb) >= 0) ord.push_back(z);
            std::stable_sort(ord.begin(), ord.end(), [](const Cand& x, const Cand& y) { return pri(x.name, x.hb) < pri(y.name, y.hb); });
            bool got = false; for (const Cand& z : ord) { const std::string lv = shortName(z.name, z.hb, z.high); if (isRepeat(lv, z.px)) continue; m.px = z.px; m.lv = lv; got = true; break; }
            if (!got) continue;
            last[std::make_pair(m.code, m.lv)] = std::make_pair(i, m.px);
        } else {
            Cand c; if (pick(cs, c)) { m.px = c.px; m.lv = shortName(c.name, c.hb, c.high); }
            if (!m.lv.empty()) { if (isRepeat(m.lv, m.px)) continue; last[std::make_pair(m.code, m.lv)] = std::make_pair(i, m.px); }
        }
        const auto ra = rawA.find(i); m.t0 = B[(size_t)(ra != rawA.end() ? ra->second : i)].t;   // (1.2.0)
        out[B[(size_t)i].t] = m;
    }
    return out;
}


// ---- (1.3.0) the Bk sequence: Bk -> FT -> Tst -> Rev (Rassul 2026-11-19 15:35-16:58 CT "just keep BK because it focuses on levels", "Bk and its
// Tst, which is currently the pb but for BO", "just keep it like the Pb because it works .. also have FT bar just like how we have it for BO",
// "the second tst for a bearish must be higher than the first", "the tst shouldnt take out the bearish breakout bar high", "no, it must overlap",
// "update the Tst so it also mentions the lvl"; mockup price-pane-cleanup-mockup.html). Bullish shown, bearish = mirror:
//   FT   within the 3 candles after the Bk, a GREEN candle closing above the Bk candle's high (or at it with a higher high).
//   Tst  a pivot LOW on the 3-bar High / Low ZigZag on the pullback after the Bk move topped (the first pivot high at / after the Bk); only after a
//        Bk that had its FT; the pivot candle OVERLAPS the Bk candle; it retraced 38.2% - 100% of the move (move = the last pivot low at / before
//        the Bk -> the top); it never takes out the Bk candle's LOW (if it does, that Bk gets no more Tsts); at most 2 per move and the 2nd reaches
//        further back (a lower low) than the 1st; none after a close below the move's origin or a Bk the other way; one Tst per pivot - the
//        earliest Bk of the move (the furthest-back break) keeps it and names its level. Shown "Tst / level / 62%".
//   Rev  from the Tst pivot candle on (12 candles at most), the first GREEN candle whose low is below the pivot candle's high, with range > 0.8 x
//        ATR(13) of the candle before and a close above the prior candle's high, or range >= 1.2 x ATR; when the prior candle has the same colour
//        its range must be bigger (rule 7) - the PB Rev candle rules.
// Pure function of the closed candles and the Bk marks: no repaint.
struct SMark { char code = 0; int dir = 0; std::string lv; long long px = 0; int pct = 0; long long bkT = 0; std::string st; };   // st = the Rev's strength line (filled by the host, as fillStrength)   // 'F' FT / 'T' Tst / 'R' Rev
inline const char* seqText(char c) { return c == 'F' ? "FT" : c == 'T' ? "Tst" : c == 'R' ? "Rev" : ""; }
inline std::map<long long, std::vector<SMark> > seq(const std::vector<wyr::Bar>& B, const std::map<long long, Mark>& marks) {
    std::map<long long, std::vector<SMark> > out; const int n = (int)B.size(); if (n < 6) return out;
    std::map<long long, int> at; for (int i = 0; i < n; ++i) at[B[(size_t)i].t] = i;
    const std::vector<double> atr = wyr::atr13(B);
    std::vector<std::pair<int, int> > piv;   // pivot candle, side (+1 low / -1 high), in order of confirmation
    { const std::vector<wyr::Leg> LG = wyr::legs(B); int prevA = -1;
      for (int i = 0; i < n; ++i) { const int a = LG[(size_t)i].a; if (a != prevA && i > 0) { bool seen = false; for (const auto& p : piv) if (p.first == a) seen = true; if (!seen) piv.push_back(std::make_pair(a, LG[(size_t)i].dir == 1 ? 1 : -1)); } prevA = a; } }
    std::vector<std::pair<int, const Mark*> > bks; for (const auto& kv : marks) if (kv.second.code == 'B') { const auto it = at.find(kv.first); if (it != at.end()) bks.push_back(std::make_pair(it->second, &kv.second)); }
    std::set<int> tstAt; std::set<std::pair<int, char> > put;
    auto add = [&](int i, char code, int dir, const std::string& lv, long long px, int pct, long long bkT) {
        if (put.count(std::make_pair(i, code))) return;
        put.insert(std::make_pair(i, code));
        SMark m; m.code = code; m.dir = dir; m.lv = lv; m.px = px; m.pct = pct; m.bkT = bkT; out[B[(size_t)i].t].push_back(m); };
    for (const auto& bk : bks) {
        const int k = bk.first, dr = bk.second->dir;
        bool ft = false;
        for (int j = k + 1; j < std::min(n, k + 4); ++j) { const wyr::Bar& b = B[(size_t)j];
            if (dr > 0 && b.c > b.o && (b.c > B[(size_t)k].h || (b.c == B[(size_t)k].h && b.h > B[(size_t)k].h))) { add(j, 'F', 1, "", 0, 0, B[(size_t)k].t); ft = true; break; }
            if (dr < 0 && b.c < b.o && (b.c < B[(size_t)k].l || (b.c == B[(size_t)k].l && b.l < B[(size_t)k].l))) { add(j, 'F', -1, "", 0, 0, B[(size_t)k].t); ft = true; break; } }
        if (!ft) continue;
        int oi = -1; for (const auto& p : piv) if (p.first <= k && p.second == dr) oi = p.first;
        if (oi < 0) continue;
        const long long origin = dr > 0 ? B[(size_t)oi].l : B[(size_t)oi].h;
        int ti = -1; for (const auto& p : piv) if (p.first >= k && p.second == -dr) { ti = p.first; break; }
        if (ti < 0) continue;
        const long long top = dr > 0 ? B[(size_t)ti].h : B[(size_t)ti].l;
        int nt = 0; long long firstExt = 0;
        for (const auto& p : piv) {
            if (p.first <= ti || p.second != dr) continue;
            if (nt >= 2) break;
            const int pi = p.first;
            bool stop = false; for (int j = k; j <= pi; ++j) if (dr > 0 ? B[(size_t)j].c < origin : B[(size_t)j].c > origin) { stop = true; break; }
            if (stop) break;
            for (const auto& o : bks) if (o.second->dir == -dr && o.first > k && o.first <= pi) { stop = true; break; }
            if (stop) break;
            const bool overl = B[(size_t)pi].l <= B[(size_t)k].h && B[(size_t)pi].h >= B[(size_t)k].l;
            const long long ext = dr > 0 ? B[(size_t)pi].l : B[(size_t)pi].h;
            const double ret = top != origin ? (double)(top - ext) / (double)(top - origin) : 0.0;
            if (!overl || ret < 0.382 || ret > 1.0) continue;
            if (dr < 0 ? B[(size_t)pi].h > B[(size_t)k].h : B[(size_t)pi].l < B[(size_t)k].l) break;   // took out the Bk candle's extreme: the break failed
            if (nt == 1 && !(dr < 0 ? ext > firstExt : ext < firstExt)) continue;                      // the 2nd reaches further back
            if (nt == 0) firstExt = ext;
            ++nt;
            if (tstAt.count(pi)) continue;                                                             // the earliest Bk of the move keeps it (still counted here)
            tstAt.insert(pi);
            add(pi, 'T', dr, bk.second->lv, bk.second->px, (int)std::lround(ret * 100.0), B[(size_t)k].t);
            for (int j = pi; j < std::min(n, pi + 12); ++j) { if (j < 1) continue; const wyr::Bar& b = B[(size_t)j]; const wyr::Bar& q = B[(size_t)j - 1];
                if (!(dr > 0 ? b.c > b.o : b.c < b.o)) continue;
                const bool back = dr > 0 ? b.l < B[(size_t)pi].h : b.h > B[(size_t)pi].l;
                const double rng = (double)(b.h - b.l), a = atr[(size_t)j - 1];
                const bool beyond = dr > 0 ? b.c > q.h : b.c < q.l;
                const bool same = (q.c > q.o) == (b.c > b.o); const bool r7 = !same || rng > (double)(q.h - q.l);
                if (back && r7 && ((rng > 0.8 * a && beyond) || rng >= 1.2 * a)) { add(j, 'R', dr, "", 0, 0, B[(size_t)k].t); break; } }
        }
    }
    return out;
}

// (1.2.0) the strength line: his BVMax / BRMax, exactly as the Volume / Range panes count it (VolRangeLogic.h Engine::statFor): for candle i,
// BV = how many candles in a row before it were SMALLER (stops at the first as big or bigger), BVG / BVR = the same over the candles of its own
// colour only; Max = the larger; counts stop at 99 ("99+"), look back at most 1,500 candles. V = volume, R = range in whole ticks.
static const int ST_LOOK = 1500, ST_CAP = 99, ST_MIN = 10;   // (1.4.0) (Rassul 2026-11-19 18:19 CT "switch to 10": the strength line shows from 10 up, was 20)
inline int stCount(const std::vector<double>& V, const std::vector<char>& G, int i) {
    if (i < 0 || i >= (int)V.size() || !(V[(size_t)i] > 0)) return 0;
    const double x = V[(size_t)i]; const bool g = G[(size_t)i] != 0; int all = 0, same = 0; bool da = false, ds = false;
    for (int j = i - 1; j >= std::max(0, i - ST_LOOK) && !(da && ds); --j) {
        const bool big = V[(size_t)j] >= x;
        if (!da) { if (big || all >= ST_CAP) da = true; else ++all; }
        if (!ds && (G[(size_t)j] != 0) == g) { if (big || same >= ST_CAP) ds = true; else ++same; }
    }
    return std::max(all, same);
}
// the strength over candles a..b (a Sw's attempt; a Bk: a == b): "V<N>" / "R<N>" for the larger count (volume wins a tie), "" under 20
inline std::string strength(const std::vector<double>& vol, const std::vector<double>& rng, const std::vector<char>& green, int a, int b) {
    int nv = 0, nr = 0; if (a < 0) a = 0;
    for (int k = a; k <= b && k < (int)vol.size(); ++k) { nv = std::max(nv, stCount(vol, green, k)); if (k < (int)rng.size()) nr = std::max(nr, stCount(rng, green, k)); }
    const int n = std::max(nv, nr); if (n < ST_MIN) return std::string();
    return std::string(nv >= nr ? "V" : "R") + (n >= ST_CAP ? std::string("99+") : std::to_string(n));
}
// (1.2.0, Rassul 11:35 CT "will you add this strength line to the I also") the strength of ONE candle by its stamp (the I candle); "" = none / under 20
inline std::string strengthAt(const std::vector<long long>& T, const std::vector<double>& V, const std::vector<double>& R, const std::vector<char>& G, long long t) {
    const auto it = std::lower_bound(T.begin(), T.end(), t); if (it == T.end() || *it != t) return std::string();
    const int i = (int)(it - T.begin()); return strength(V, R, G, i, i);
}
// the range value as the Range pane counts it: high - low in whole ticks (0 = none)
inline double rangeTicks(double h, double l, double tick) { const double r = h - l; if (!std::isfinite(r) || r <= 0) return 0.0; return tick > 0 ? (double)std::llround(r / tick) : r; }
// fill the strength of every Bk / Sw in M from the chart's closed candles (T = candle stamps, ascending; V volume; R range ticks; G green)
// (1.5.0, Rassul 2026-11-19 18:29 CT "lets keep it at 10 so the BRmax or BVmax should be at least 10") a Sw only counts when its attempt's
// BVMax or BRMax (max over the attempt candles, the strength line's measure) is at least SW_MIN; a weaker Sw is removed here
// (1.6.0, Rassul 2026-11-20 07:50 CT "update the sweep threshold to 20") SW_MIN 10 -> 20; ST_MIN (the strength line) stays 10
static const int SW_MIN = 20;
inline int strengthN(const std::vector<double>& vol, const std::vector<double>& rng, const std::vector<char>& green, int a, int b) {
    int nv = 0, nr = 0; if (a < 0) a = 0;
    for (int k = a; k <= b && k < (int)vol.size(); ++k) { nv = std::max(nv, stCount(vol, green, k)); if (k < (int)rng.size()) nr = std::max(nr, stCount(rng, green, k)); }
    return std::max(nv, nr);
}
inline int fillStrength(std::map<long long, Mark>& M, const std::vector<long long>& T, const std::vector<double>& V, const std::vector<double>& R, const std::vector<char>& G) {
    int n = 0;
    auto at = [&](long long t) { const auto it = std::lower_bound(T.begin(), T.end(), t); return it != T.end() && *it == t ? (int)(it - T.begin()) : -1; };
    for (auto it = M.begin(); it != M.end(); ) { Mark& m = it->second; m.st.clear(); if (m.code != 'B' && m.code != 'S') { ++it; continue; }
        const int b = at(it->first); if (b < 0) { ++it; continue; } int a = m.t0 > 0 ? at(m.t0) : b; if (a < 0 || a > b) a = b;
        if (m.code == 'S') a = b;   // (1.8.0, Rassul 10:35 CT "the red sweep bar doesnt meet the threshold.. fix") a Sw is judged and labelled by ITS OWN candle, not the attempt's biggest
        if (m.code == 'S' && strengthN(V, R, G, a, b) < SW_MIN) { it = M.erase(it); continue; }   // (1.5.0; 1.6.0: 20) too weak for a Sw
        m.st = strength(V, R, G, a, b); if (!m.st.empty()) ++n; ++it; }
    return n;
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
// (1.7.0) a level the I (and Bk / Sw) may use: a NAMED key level (0.19.4 "I should not print if there is no level") - no VWAP / its bands (they
// move), no HBO line, no MenthorQ / gamma names, no merged "a|b" names; px > 0
inline bool keyForI(const std::string& name, long long px) { return px > 0 && name.compare(0, 4, "VWAP") != 0 && name != "the HBO test line" && pri(name, 0) >= 0 && name.find('|') == std::string::npos; }
// (1.7.0) the I's (and Sp / Ut's) level list leaves out the level FILE's PHH / PHL: the file holds the prior hour as of NOW, and the live session
// is re-read every candle, so an earlier I tied to it would vanish when the hour turns (a repaint). The prior hours' highs / lows still count -
// WyRead takes them from the candles themselves (no look-ahead), with the same short names (PHH / PHL / 2hH ...).
inline bool levelForI(const std::string& name, long long px) { return keyForI(name, px) && name != "PHH" && name != "PHL"; }
// (1.7.0) the level an I names: the reclaimed level with the best Bk / Sw priority (lower pri first); a tie = the furthest reclaimed (the
// highest for a bullish I, side +1). Unnamed levels (MenthorQ, gamma, VWAP ...) never label it. false = no named level (no I).
// (1.8.0, Rassul 10:37 CT "I should only be on key levels that are named like overnight etc..") an I's level: session (ON / London / Asia),
// PD, PF, VAH / VAL, weekly - priority 0..40; the hour levels (PHH / PHL, 2h / 3h back, priority 50 / 60) never name an I
inline bool iKey(int p) { return p >= 0 && p < 50; }
// (1.8.0, "we should have a 20 threshold just like sweeps") the I candle's own BVMax or BRMax must be at least I_MIN
static const int I_MIN = 20;
inline bool iPick(const std::vector<wyr::Lvl>& c, int side, wyr::Lvl& out) {
    int best = -1; bool got = false;
    for (const wyr::Lvl& z : c) { const int p = pri(z.name, z.hb); if (!iKey(p)) continue;   // (1.8.0) no hour level
        if (!got || p < best || (p == best && (side > 0 ? z.px > out.px : z.px < out.px))) { out = z; best = p; got = true; } }
    return got;
}
// (1.7.0) the I candle is also a Sw of the same direction: only the Sw is drawn (badge and strip); the I is still recorded (flagged)
inline bool iUnderSw(const std::map<long long, Mark>& lv, long long t, int dir) { const auto it = lv.find(t); return it != lv.end() && it->second.code == 'S' && it->second.dir == dir; }
inline std::string iLevel(const std::string& name, int hb, bool high) { if (pri(name, hb) < 0) return ""; return shortName(name, hb, high); }
} // namespace lvm
