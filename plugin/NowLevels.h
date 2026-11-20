// NowLevels.h -- the HOD / LOD "Now" line as a level map (lsHodLod 2.36.0), SDK-free
//
// (2.36.0, Rassul 2026-11-19 18:35-18:39 CT "i want to update the now to be more focused on levels indicating the last level broken, then
// next level and if it is looking to tst any level that it already broke - this will tell me where to look for a test and a potential
// reversal trade. the next level will tell me where to look for a break or sweep trade", "I dont have that much vertical space, use
// horizonal", "empasize watch so i know what kind of trade and where", "remove now, the order should be in what is most important to find
// a trade"; mockup now-box-levels-mockup.html): ONE line, left to right, most important first:
//   (1.1.0, 22:46 CT) TESTING follows the chart's Tst rules (LevelMarks.h): after a Bk with its FT, the pullback overlaps the Bk candle AND is
//   38.2%..100% back (origin -> the move's extreme), not past the Bk candle's extreme, at most 2 Tsts per move, void on a close beyond the
//   origin or an opposite Bk. Under 38.2% it stays the gold WATCH (2.36.0 had dropped the 38.2% minimum).
//   (1.1.0, 22:42 CT) TESTING reads "TESTING: <level> BkTst <arrow>" (no Rev while testing); WATCH keeps "WATCH: <level> BkTst Rev <arrow>"
//   a Tst coming / in progress:  [WATCH / TESTING: <level> · BkTst Rev <arrow>] │ Next ↓ <level> · ↑ <level> │ Last Bk <arrow> <level>
//   (18:52 "as price is going to PDL it would say Watch: PDL BkTstRev": WATCH once the move topped and the pullback heads back toward the
//   broken level; TESTING once it trades into the Bk candle (<= 100% back, never past its extreme); 18:53 every WATCH badge is gold, only
//   TESTING is red / green by side)
//   a live break / sweep: [BREAK / SWEEP: <level> <arrow>] │ Next <arrow> <level> │ Last Bk <arrow> <level>    (18:54, red / green; TESTING outranks it)
//   one badge: TESTING > BREAK / SWEEP > WATCH BkTst > WATCH Bk / Sw. Only signals at named levels; nothing from a Cob (19:02); no REV state
//   (19:04 "remove for the Rev candle"): after a BkTst Rev the line simply moves on to the next WATCH.
//   else:               [WATCH: <level> ↓ · <level> ↑ · Bk / Sw] │ Last Bk <arrow> <level>    (19:06; BkTst: "WATCH: <level> BkTst Rev <arrow>", 19:03)
//   (18:48 TESTING replaces the WATCH badge, red / green by side; 18:47 one badge at a time; 18:41 no Tst = nothing for it; 18:50 "if the
//   levels are already mentioned in the watch they shouldnt be mentioned again": the WATCH badge carries the next levels with their arrows,
//   no separate Next part; no level is named twice on the line - a Next level or the Last Bk level already named is left out)
// Named levels only (18:42-18:43): no numbers at all (prices, distances, strength, %, time). When the line does not fit, the Last Bk part drops off the right end.
// Testing = the Tst rules of LevelMarks lvm::seq (Pb rules) evaluated LIVE on the forming swing: after a Bk that had its FT, the pullback from
// the move's extreme overlaps the Bk candle, is 38.2-100% back, never past the Bk candle's extreme, no close beyond the move origin, no Bk the
// other way since; several Bks in one move: the furthest-back one the pullback reaches is named.
// Next = the nearest named key level above / below the live price (the marks' level set: no POC / Open / VWAP bands / gamma) + the prior 3 full
// clock hours' highs / lows (PHH / PHL, 2hH / 2hL, 3hH / 3hL).
#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <algorithm>

namespace nowl {

static const char* const VERSION = "1.1.0";   // (1.1.0 = lsHodLod 2.37.1, Rassul 2026-11-19 22:42 CT "how can you have a BkTst Rev when you are testing a level?"): TESTING no longer names the Rev (it has not printed yet) - "TESTING: <level> BkTst <arrow>", e.g. "TESTING: PFL BkTst" + down arrow; no "Rev", no middle dot (Level then Signal, as WATCH); WATCH stays "WATCH: <level> BkTst Rev <arrow>"   // was "1.0.0"
static const long long E1904 = 2082844800LL;
static const double TST_MIN = 0.382;   // (1.1.0, Rassul 22:46 CT "after it breaks, and continues, you can then have the test of the level after it has the minimum required retracement") = LevelMarks.h's Tst minimum
static const size_t MAX_BYTES = 512 * 1024;

struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0, v = 0; };       // t = candle END, local secs (unix-naive local)
struct Mark { long long t = 0; std::string code, lv, st; int dir = 0; double px = 0; };   // Bk / FT from the wy file
struct Key { std::string name; double px = 0; };

// colours (BGR like the host uses COLORREF 0x00BBGGRR)
// colours as the host's COLOR (0x00RRGGBB, as hl::C_GREEN): labels grey, levels gold, bearish red, bullish green
static const unsigned C_GREY = 0x0094A3B8, C_GOLD = 0x00F0C040, C_GREEN = 0x0022C55E, C_RED = 0x00EF4444, C_WHITE = 0x00E5E7EB, C_BLACK = 0x00000000, C_SEP = 0x00475569, C_BG = 0x0015181D, C_BORDER = 0x003A3F47;

// one piece of the line: s, then an arrow (+1 up / -1 down, drawn with pen strokes - IRT's drawText is cp1252, no arrow glyphs), then s2;
// sep = the thin vertical separator between parts; badge = a filled box (fill) with black bold text
struct Seg { std::string s, s2; int arrow = 0; unsigned col = C_WHITE; bool bold = false; bool badge = false; bool sep = false; unsigned fill = 0; int part = 0; int drop = 0; };   // drop: 0 keep, 2 the Last Bk part

inline bool toLL(const std::string& s, long long& v) { if (s.empty() || s.size() > 20) return false; errno = 0; char* e = nullptr; const long long x = std::strtoll(s.c_str(), &e, 10); if (errno || !e || *e) return false; v = x; return true; }
inline bool toD(const std::string& s, double& v) { if (s.empty() || s.size() > 30) return false; errno = 0; char* e = nullptr; const double x = std::strtod(s.c_str(), &e); if (errno || !e || *e || !std::isfinite(x)) return false; v = x; return true; }
inline std::vector<std::string> split(const std::string& s, char d) { std::vector<std::string> o; std::string cur; for (char c : s) { if (c == d) { o.push_back(cur); cur.clear(); } else if (c != '\r') cur += c; } o.push_back(cur); return o; }

// the marks' level names (LevelMarks pri >= 0, short names) - anything else (DPoc, WPoc, Open, VWAP, gamma) is not a level here
inline int pri(const std::string& n) {
    static const char* const S0[] = { "ONL", "ONH", "LLO", "LHI", "LonLO", "LonHI", "ALO", "AHI", "AsiaLO", "AsiaHI" };
    static const char* const S1[] = { "PDH", "PDL" }; static const char* const S2[] = { "PFH", "PFL" }; static const char* const S3[] = { "VAH", "VAL" };
    static const char* const S4[] = { "PWH", "PWL", "CWH", "CWL" };
    for (const char* x : S0) { if (n == x) return 0; }
    for (const char* x : S1) { if (n == x) return 10; }
    for (const char* x : S2) { if (n == x) return 20; }
    for (const char* x : S3) { if (n == x) return 30; }
    for (const char* x : S4) { if (n == x) return 40; }
    if (n == "PHH" || n == "PHL") return 50;
    if (n.size() == 3 && n[0] >= '2' && n[0] <= '9' && n[1] == 'h' && (n[2] == 'H' || n[2] == 'L')) return 60;
    return -1;
}
inline std::string shortName(const std::string& n) {
    if (n == "LonLO") return "LLO";
    if (n == "LonHI") return "LHI";
    if (n == "AsiaLO") return "ALO";
    if (n == "AsiaHI") return "AHI";
    return n;
}

// the wy file's Bk and FT lines (lsRevRead 0.20.x: "stamp|Bk|dir|lv|px|st", "stamp|FT|dir"); px in ticks
inline std::vector<Mark> parseWy(const std::string& text, double tick, long long from, long long to) {
    std::vector<Mark> out; if (text.size() > MAX_BYTES || !(tick > 0)) return out;
    for (const std::string& ln : split(text, '\n')) {
        const std::vector<std::string> f = split(ln, '|'); if (f.size() < 3) continue;
        long long st = 0, d = 0; if (!toLL(f[0], st) || !toLL(f[2], d) || (d != 1 && d != -1)) continue;
        const long long t = st - E1904; if (t < from || t > to) continue;
        Mark m; m.t = t; m.code = f[1]; m.dir = (int)d;
        if (m.code == "Bk") { if (f.size() < 5) continue; long long px = 0; if (!toLL(f[4], px) || px <= 0) continue; m.lv = f[3]; m.px = (double)px * tick; m.st = f.size() > 5 ? f[5] : std::string(); if (pri(m.lv) < 0) continue; }
        else if (m.code != "FT") continue;
        out.push_back(m);
    }
    std::stable_sort(out.begin(), out.end(), [](const Mark& a, const Mark& b) { return a.t < b.t; });
    return out;
}

inline long long hourOf(long long tEnd) { return (tEnd - 1) / 3600; }
// the prior 3 full clock hours' highs / lows before the hour of tNow (PHH / PHL, 2hH / 2hL, 3hH / 3hL)
inline std::vector<Key> hourKeys(const std::vector<Bar>& B, long long tNow) {
    std::vector<Key> out; const long long hNow = hourOf(tNow);
    for (int k = 1; k <= 3; k++) {
        const long long hh = hNow - k; if (B.empty() || hh <= hourOf(B[0].t)) break;   // as LevelMarks: not the session's first (partial) hour
        double hi = -1e300, lo = 1e300; int n = 0;
        for (const Bar& b : B) if (hourOf(b.t) == hh) { hi = std::max(hi, b.h); lo = std::min(lo, b.l); n++; }
        if (!n) continue;
        out.push_back(Key{ k == 1 ? "PHH" : std::to_string(k) + "hH", hi }); out.push_back(Key{ k == 1 ? "PHL" : std::to_string(k) + "hL", lo });
    }
    return out;
}

struct Test { bool ok = false; int state = 0; int dir = 0;   // state 1 WATCH (a test is coming), 2 TESTING (in the Bk candle)
    std::string lv; double px = 0; int pct = 0; std::string why; };
struct Next { bool ok = false; std::string name; double px = 0, pts = 0; };
// (18:54 "there could also be a break so it would be green or red") the forming candle against the levels watched before it (the nearest
// above / below the last close): state 1 BREAK = it closes through the level now in the break's colour; 2 SWEEP = it went through and closes
// back now in the Sw colour (a swept low = green, a swept high = red); live, settled at the candle's close
struct Live { int state = 0; int dir = 0; std::string name; double px = 0; };
struct Out { bool ok = false; bool haveBk = false; Mark bk; Test tst; Live bs; Next up, dn; double live = NAN; };

// 3-bar High / Low ZigZag pivots over B (as LevelMarks / the chart ZigZag): (index, side +1 low / -1 high)
struct Piv { int first = 0, second = 0, at = 0; };   // the pivot candle, side (+1 low / -1 high), the candle that confirmed it
inline std::vector<Piv> pivots(const std::vector<Bar>& B) {
    std::vector<Piv> P; const int n = (int)B.size(); if (n < 2) return P;
    int zd = B[1].h >= B[0].h ? 1 : -1, zext = 0;
    for (int i = 1; i < n; i++) {
        const int a = std::max(0, i - 3); double pl = 1e300, ph = -1e300; for (int j = a; j < i; j++) { pl = std::min(pl, B[(size_t)j].l); ph = std::max(ph, B[(size_t)j].h); }
        if (zd == 1) { if (B[(size_t)i].h > B[(size_t)zext].h) zext = i; else if (B[(size_t)i].l < pl && i != zext) { P.push_back(Piv{ zext, -1, i }); zext = i; zd = -1; } }
        else { if (B[(size_t)i].l < B[(size_t)zext].l) zext = i; else if (B[(size_t)i].h > ph && i != zext) { P.push_back(Piv{ zext, +1, i }); zext = i; zd = 1; } }
    }
    return P;
}

// FT (the BO's FT rule, as LevelMarks): within 3 candles after the Bk, a candle of the break's colour that closes beyond the Bk candle's
// extreme (or at it, with a new extreme); only closed candles (index < nClosed)
inline bool hasFT(const std::vector<Bar>& B, int k, int dr, int nClosed) {
    const Bar& K = B[(size_t)k];
    for (int j = k + 1; j < std::min(nClosed, k + 4); j++) { const Bar& b = B[(size_t)j];
        if (dr > 0 && b.c > b.o && (b.c > K.h || (b.c == K.h && b.h > K.h))) return true;
        if (dr < 0 && b.c < b.o && (b.c < K.l || (b.c == K.l && b.l < K.l))) return true; }
    return false;
}
// the Bk candle rules (as the marks, 16:32): range >= the prior candle's, and range OR volume up on it
inline bool bkOk(const std::vector<Bar>& B, int k) { if (k < 1) return true; const double r0 = B[(size_t)k].h - B[(size_t)k].l, r1 = B[(size_t)k - 1].h - B[(size_t)k - 1].l; return r0 >= r1 && (r0 > r1 || B[(size_t)k].v > B[(size_t)k - 1].v); }

// B = the session's bars INCLUDING the forming one last; marks = this session's Bk / FT; keys = the named key levels; live = last price
inline Out compute(const std::vector<Bar>& B, const std::vector<Mark>& marksIn, const std::vector<Key>& keysIn, double live, double tick) {
    Out o; o.live = live; if (B.size() < 3 || !std::isfinite(live) || !(tick > 0)) return o; o.ok = true;
    const int n = (int)B.size();
    auto idxOf = [&](long long t) { for (int i = n - 2; i >= 0; i--) if (B[(size_t)i].t == t) return i; return -1; };   // closed candles only
    std::vector<Mark> marks; for (const Mark& m : marksIn) { if (m.code != "Bk") continue; const int k = idxOf(m.t); if (k >= 0 && bkOk(B, k)) marks.push_back(m); }
    // last Bk of the session
    for (const Mark& m : marks) if (m.code == "Bk" && idxOf(m.t) >= 0) { o.bk = m; o.haveBk = true; }
    // (18:52, three states) the Bks of the current move, furthest back first: TESTING = the pullback trades into a Bk candle's range (<= 100%
    // back, never past its extreme, no close beyond the move origin) - the furthest-back Bk it reaches is named; WATCH = the move topped
    // and the pullback is heading back but has not reached the Bk candle yet - the nearest Bk ahead (the latest) is named; else neither
    if (o.haveBk) {
        const std::vector<Piv> P = pivots(B);
        const int dr = o.bk.dir; std::string why = "no pullback yet";
        long long lastOpp = -1; for (const Mark& m : marks) if (m.dir == -dr) lastOpp = m.t;   // only the Bks since the last Bk the other way
        Test testing, watching; bool pending = false;
        for (const Mark& bm : marks) {
            if (bm.dir != dr || bm.t <= lastOpp) continue;
            const int k = idxOf(bm.t);
            if (!hasFT(B, k, dr, n - 1)) { pending = k + 3 > n - 2; why = pending ? "waiting for the FT" : "no FT after the break"; continue; }   // the newest Bk's FT window still open: no WATCH on an older one
            pending = false;
            int oi = -1; for (const auto& p : P) if (p.first <= k && p.second == dr) oi = p.first;   // the move's origin: the last pivot against it before the Bk
            if (oi < 0) continue;
            const double origin = dr < 0 ? B[(size_t)oi].h : B[(size_t)oi].l;
            int e = k; for (int i = k; i < n; i++) if (dr < 0 ? B[(size_t)i].l < B[(size_t)e].l : B[(size_t)i].h > B[(size_t)e].h) e = i;   // the move's extreme so far
            if (e >= n - 1) { why = "still extending"; continue; }
            const Piv* tp = nullptr; for (const Piv& p : P) if (p.first > e && p.second == dr) { tp = &p; break; }   // the pullback leg already turned (its pivot confirmed)
            if (tp) { why = "the pullback already turned"; continue; }
            bool closedBeyond = false; for (int i = k; i < n - 1; i++) if (dr < 0 ? B[(size_t)i].c > origin : B[(size_t)i].c < origin) closedBeyond = true;
            if (closedBeyond) { why = "closed back beyond the move origin"; continue; }
            double pb = dr < 0 ? -1e300 : 1e300; for (int i = e + 1; i < n; i++) pb = dr < 0 ? std::max(pb, B[(size_t)i].h) : std::min(pb, B[(size_t)i].l);
            const double top = dr < 0 ? B[(size_t)e].l : B[(size_t)e].h, span = std::fabs(origin - top); if (!(span > 0)) continue;
            const double ret = std::fabs(pb - top) / span;
            const Bar& bk = B[(size_t)k];
            {   // (1.1.0, Rassul 22:46 CT) the chart's Tst rules (LevelMarks.h seq): at most 2 Tsts per move - count the finished pullbacks of this
                // move that already made a Tst (first turn after the Bk = the top; each later pullback pivot: overlaps the Bk candle, 38.2%..100%
                // back, not past the Bk candle's extreme, the 2nd further back than the 1st; none after a close beyond the origin)
                int ti = -1; for (const Piv& p : P) if (p.first >= k && p.second == -dr) { ti = p.first; break; }
                int done = 0; double firstExt = 0;
                if (ti >= 0 && ti < e) {
                    const double top0 = dr < 0 ? B[(size_t)ti].l : B[(size_t)ti].h;
                    for (const Piv& p : P) {
                        if (p.first <= ti || p.first >= e || p.second != dr || done >= 2) continue;
                        const Bar& q = B[(size_t)p.first];
                        const bool overl = q.l <= bk.h && q.h >= bk.l; const double ext = dr > 0 ? q.l : q.h;
                        const double r0 = top0 != origin ? (top0 - ext) / (top0 - origin) : 0.0;
                        if (!overl || r0 < TST_MIN || r0 > 1.0) continue;
                        if (dr < 0 ? q.h > bk.h : q.l < bk.l) break;
                        if (done == 1 && !(dr < 0 ? ext > firstExt : ext < firstExt)) continue;
                        if (done == 0) firstExt = ext;
                        ++done;
                    }
                }
                if (done >= 2) { why = "2 Tsts already in this move"; continue; }
            }
            if (dr < 0 ? pb > bk.h : pb < bk.l) { why = std::string("took out the Bk candle ") + (dr < 0 ? "high" : "low"); continue; }
            if (ret > 1.0) { why = "past the move origin"; continue; }
            Test t; t.ok = true; t.dir = dr; t.lv = bm.lv; t.px = bm.px; t.pct = (int)std::lround(ret * 100);
            if (dr < 0 ? pb < bk.l : pb > bk.h) { if (e <= n - 3) { t.state = 1; watching = t; } else why = "just topped"; continue; }   // heading back (a closed candle since the extreme did not extend it), not in the Bk candle yet: the latest such Bk is the next one ahead
            if (ret < TST_MIN) { t.state = 1; watching = t; continue; }   // (1.1.0, 22:46 CT) in the Bk candle but under 38.2% back: not a Tst yet - the gold WATCH, as the chart's Tst marks
            t.state = 2; if (!testing.ok) testing = t;   // in the Bk candle and 38.2%+ back: the furthest-back one reached
        }
        if (testing.ok) o.tst = testing; else if (watching.ok && !pending) o.tst = watching; else o.tst.why = pending ? "waiting for the FT" : why;
    } else o.tst.why = "no break yet";
    // Next: the nearest named level above and below the live price
    std::vector<Key> keys; for (const Key& kk : keysIn) { const std::string s = shortName(kk.name); if (pri(s) >= 0 && pri(s) < 50 && kk.px > 0) keys.push_back(Key{ s, kk.px }); }   // the hour levels come live from the bars below, not the key file's snapshot
    for (const Key& kk : hourKeys(std::vector<Bar>(B.begin(), B.end() - 1), B.back().t)) keys.push_back(kk);
    const double eps = tick * 0.5;
    {   // the live BREAK / SWEEP of a watched level by the forming candle (18:54)
        const Bar& f = B.back(); const double ref = B[(size_t)n - 2].c;
        Key wu, wd; bool hu = false, hd = false;
        for (const Key& kk : keys) {
            if (kk.px > ref + eps && (!hu || kk.px < wu.px - eps || (std::fabs(kk.px - wu.px) <= eps && pri(kk.name) < pri(wu.name)))) { wu = kk; hu = true; }
            if (kk.px < ref - eps && (!hd || kk.px > wd.px + eps || (std::fabs(kk.px - wd.px) <= eps && pri(kk.name) < pri(wd.name)))) { wd = kk; hd = true; }
        }
        const double c = live;
        if (hu && f.h > wu.px + eps) {   // through the level above
            if (c > wu.px + eps && c > f.o) { o.bs.state = 1; o.bs.dir = 1; }               // BREAK up: closing above it, green
            else if (c < wu.px - eps && c < f.o) { o.bs.state = 2; o.bs.dir = -1; }         // SWEEP of a high: back below it, red
            if (o.bs.state) { o.bs.name = wu.name; o.bs.px = wu.px; }
        }
        if (!o.bs.state && hd && f.l < wd.px - eps) {   // through the level below
            if (c < wd.px - eps && c < f.o) { o.bs.state = 1; o.bs.dir = -1; }              // BREAK down: closing below it, red
            else if (c > wd.px + eps && c > f.o) { o.bs.state = 2; o.bs.dir = 1; }          // SWEEP of a low: back above it, green
            if (o.bs.state) { o.bs.name = wd.name; o.bs.px = wd.px; }
        }
    }
    for (const Key& kk : keys) {
        if (o.tst.ok && std::fabs(kk.px - o.tst.px) <= eps) continue;   // the level being tested is already on the line
        if (o.bs.state && std::fabs(kk.px - o.bs.px) <= eps) continue;   // the level broken / swept now is already on the line
        if (kk.px > live + eps) { if (!o.up.ok || kk.px < o.up.px - eps || (std::fabs(kk.px - o.up.px) <= eps && pri(kk.name) < pri(o.up.name))) { o.up.ok = true; o.up.name = kk.name; o.up.px = kk.px; } }
        else if (kk.px < live - eps) { if (!o.dn.ok || kk.px > o.dn.px + eps || (std::fabs(kk.px - o.dn.px) <= eps && pri(kk.name) < pri(o.dn.name))) { o.dn.ok = true; o.dn.name = kk.name; o.dn.px = kk.px; } }
    }
    if (o.up.ok) o.up.pts = o.up.px - live;
    if (o.dn.ok) o.dn.pts = live - o.dn.px;
    return o;
}

inline std::string fmtPx(double v, int dec) { char b[40]; std::snprintf(b, sizeof b, "%.*f", dec, v); std::string s = b, ip = s, fp; const size_t d = s.find('.'); if (d != std::string::npos) { ip = s.substr(0, d); fp = s.substr(d); }
    bool neg = !ip.empty() && ip[0] == '-'; if (neg) ip = ip.substr(1); std::string r; for (size_t i = 0; i < ip.size(); i++) { if (i && (ip.size() - i) % 3 == 0) r += ','; r += ip[i]; } return (neg ? "-" : "") + r + fp; }
inline std::string clock12(long long t) { const long long s = ((t % 86400) + 86400) % 86400; int h = (int)(s / 3600), m = (int)(s % 3600 / 60); const int h12 = h % 12 == 0 ? 12 : h % 12; char b[16]; std::snprintf(b, sizeof b, "%d:%02d", h12, m); return b; }

// the segments, most important first; named levels only (Rassul 18:42 "get rid of the values, just keep the named levels"): no prices, no
// distances, no strength, no %, no time (18:43) - no numbers at all. drop = 2: the Last Bk part, dropped first when the line does not fit.
// Consecutive badge pieces of one part are ONE filled badge.
inline std::vector<Seg> segments(const Out& o) {
    std::vector<Seg> v; if (!o.ok) return v;
    std::vector<std::string> named;   // (18:50) no level named twice
    auto has = [&](const std::string& n) { return std::find(named.begin(), named.end(), n) != named.end(); };
    auto add = [&](const std::string& s, int arrow, const std::string& s2, unsigned col, bool bold, int part, int drop = 0) { Seg g; g.s = s; g.arrow = arrow; g.s2 = s2; g.col = col; g.bold = bold; g.part = part; g.drop = drop; v.push_back(g); };
    auto badge = [&](const std::string& s, int arrow, const std::string& s2, unsigned fill, int part) { Seg g; g.s = s; g.arrow = arrow; g.s2 = s2; g.col = C_BLACK; g.bold = true; g.badge = true; g.fill = fill; g.part = part; v.push_back(g); };
    auto sep = [&](int part, int drop = 0) { if (v.empty()) return; Seg g; g.sep = true; g.col = C_SEP; g.part = part; g.drop = drop; v.push_back(g); };
    const std::string dot = "\xB7";   // cp1252 middle dot
    const Next dn = o.dn, up = o.up;
    const bool testing = o.tst.ok && o.tst.state == 2;
    if (!testing && o.bs.state) {   // 0. a live BREAK / SWEEP (18:54), red / green by side; then the next level that way only
        badge(std::string(o.bs.state == 1 ? "BREAK: " : "SWEEP: ") + o.bs.name, o.bs.dir, "", o.bs.dir < 0 ? C_RED : C_GREEN, 1); named.push_back(o.bs.name);
        const Next& nx = o.bs.dir < 0 ? dn : up;
        if (nx.ok && !has(nx.name)) { sep(2); add("Next", 0, "", C_GREY, false, 2); add("", o.bs.dir, nx.name, C_GOLD, true, 2); named.push_back(nx.name); }
    } else if (o.tst.ok) {   // 1. a BkTst coming / in progress: "WATCH: <level> · BkTst Rev <arrow>" (18:52, gold) / "TESTING: ..." (18:48, red / green by side)
        badge(o.tst.state == 2 ? "TESTING: " + o.tst.lv + " BkTst" : "WATCH: " + o.tst.lv + " BkTst Rev", o.tst.dir < 0 ? -1 : 1, "", o.tst.state != 2 ? C_GOLD : o.tst.dir < 0 ? C_RED : C_GREEN, 1);   // (18:53) every WATCH is gold; only TESTING is red / green
        named.push_back(o.tst.lv);
        const bool d = dn.ok && !has(dn.name), u = up.ok && !has(up.name) && !(d && up.name == dn.name);
        if (d || u) {   // then the next levels, plainly (not in a badge: one badge at a time, 18:47)
            sep(2); add("Next", 0, "", C_GREY, false, 2);
            if (d) { add("", -1, dn.name, C_GOLD, true, 2); named.push_back(dn.name); }
            if (d && u) add(dot, 0, "", C_GREY, false, 2);
            if (u) { add("", 1, up.name, C_GOLD, true, 2); named.push_back(up.name); }
        }
    } else if (dn.ok || up.ok) {   // 2. else the gold WATCH badge with the next levels and their arrows (18:50): "WATCH: Bk / Sw at ↓ PDL · ↑ ONL"
        // (19:03 "for watch it should be in this format : Watch Level Signal") "WATCH: PDL Bk / Sw ↓ · ONL Bk / Sw ↑" - below first, then above
        // (19:06 "update to WATCH: PDL ↓ · ONL↑ · Bk / Sw") the levels with their arrows, then the signal once at the end
        if (dn.ok) { badge("WATCH: " + dn.name, -1, "", C_GOLD, 2); named.push_back(dn.name); }
        if (up.ok) { badge((dn.ok ? dot + " " : std::string("WATCH: ")) + up.name, 1, "", C_GOLD, 2); named.push_back(up.name); }
        badge(dot + " Bk / Sw", 0, "", C_GOLD, 2);
    }
    if (o.haveBk && !has(o.bk.lv)) {   // 3. the last Bk (context, dropped first) - only when its level is not on the line already (18:50)
        sep(3, 2); add("Last Bk", 0, "", C_GREY, false, 3, 2);
        add("", o.bk.dir < 0 ? -1 : 1, o.bk.lv, o.bk.dir < 0 ? C_RED : C_GREEN, true, 3, 2);
    }
    return v;
}
// what fits: level 0 = all, 1 = without the Last Bk part
inline std::vector<Seg> fit(const std::vector<Seg>& all, int level) { std::vector<Seg> v; for (const Seg& s : all) { if (level >= 1 && s.drop == 2) continue; v.push_back(s); } return v; }
// is piece q the first / last of its badge (consecutive badge pieces of one part = one badge)
inline bool badgeFirst(const std::vector<Seg>& v, size_t q) { return v[q].badge && (q == 0 || !v[q - 1].badge || v[q - 1].part != v[q].part); }
inline bool badgeLast(const std::vector<Seg>& v, size_t q) { return v[q].badge && (q + 1 == v.size() || !v[q + 1].badge || v[q + 1].part != v[q].part); }
// the line as text, for the tests and the status file (UTF-8 arrows / separator, the dot back to UTF-8); a badge in [ ]
inline std::string plain(const std::vector<Seg>& v) {
    auto u = [](std::string s) { std::string o; for (char c : s) { if (c == '\xB7') o += "\xC2\xB7"; else o += c; } return o; };
    std::string t;
    for (size_t q = 0; q < v.size(); q++) { const Seg& g = v[q];
        std::string x = g.sep ? std::string("\xE2\x94\x82") : u(g.s);
        if (g.arrow) { if (!x.empty()) x += ' '; x += g.arrow > 0 ? "\xE2\x86\x91" : "\xE2\x86\x93"; }
        if (!g.s2.empty()) x += " " + u(g.s2);
        if (badgeFirst(v, q)) x = "[" + x;
        if (badgeLast(v, q)) x += "]";
        if (!t.empty()) t += ' ';
        t += x;
    }
    return t;
}

// the drawing, SDK-free: the host measures text (meas(s, bold)) and draws the items in order. One line centred on cx with its bottom at
// yBottom, inside [xMin, xMax]; the Last Bk part drops first, then pieces drop off the right end (never a dangling separator) until it fits.
struct Item { int k = 0; int l = 0, t = 0, r = 0, b = 0; unsigned col = 0, fill = 0; bool bold = false; std::string s; };   // k: 0 filled box (col = border), 1 text (x = l, row t..b), 2 line
struct Draw { bool ok = false; int l = 0, t = 0, r = 0, b = 0; int level = 0; int pieces = 0, shown = 0; std::vector<Item> items; };
static const int ARW = 9, GAP = 5, PADX = 5, SEPW = 13;
template <class M> inline int segW(const std::vector<Seg>& v, size_t q, M meas) {
    const Seg& g = v[q]; if (g.sep) return SEPW;
    int w = 0; if (!g.s.empty()) w += meas(g.s, g.bold);
    if (g.arrow) w += (w ? 4 : 0) + ARW;
    if (!g.s2.empty()) w += (w ? 4 : 0) + meas(g.s2, g.bold);
    return w + (badgeFirst(v, q) ? PADX : 0) + (badgeLast(v, q) ? PADX : 0);
}
template <class M> inline Draw layout(const std::vector<Seg>& all, M meas, int cx, int yBottom, int xMin, int xMax, int fs) {
    Draw D; if (all.empty() || xMax - xMin < 40) return D;
    const int h = fs + 8, padL = 6;
    auto width = [&](const std::vector<Seg>& v) { int w = 2 * padL; for (size_t q = 0; q < v.size(); q++) w += segW(v, q, meas) + (q ? GAP : 0); return w; };
    std::vector<Seg> v = fit(all, 0); int level = 0;
    if (width(v) > xMax - xMin) { v = fit(all, 1); level = 1;
        while (!v.empty() && width(v) > xMax - xMin) { v.pop_back(); while (!v.empty() && v.back().sep) v.pop_back(); } }
    if (v.empty()) return D;
    const int tw = width(v);
    D.ok = true; D.level = level; D.pieces = (int)all.size(); D.shown = (int)v.size();
    D.l = cx - tw / 2; if (D.l + tw > xMax) D.l = xMax - tw; if (D.l < xMin) D.l = xMin; D.r = D.l + tw; D.b = yBottom; D.t = yBottom - h;
    Item bg; bg.k = 0; bg.l = D.l; bg.t = D.t; bg.r = D.r; bg.b = D.b; bg.fill = C_BG; bg.col = C_BORDER; D.items.push_back(bg);
    const int t = D.t + 2, b = D.b - 2, ym = (D.t + D.b) / 2;
    std::vector<int> xs(v.size()); { int x = D.l + padL; for (size_t q = 0; q < v.size(); q++) { if (q) x += GAP; xs[q] = x; x += segW(v, q, meas); } }
    for (size_t q = 0; q < v.size(); q++) if (badgeFirst(v, q)) {   // the badge fills first, under their text
        size_t e = q; while (!badgeLast(v, e)) e++;
        Item F; F.k = 0; F.l = xs[q]; F.t = t; F.r = xs[e] + segW(v, e, meas); F.b = b; F.fill = v[q].fill; F.col = v[q].fill; D.items.push_back(F); }
    for (size_t q = 0; q < v.size(); q++) { const Seg& g = v[q];
        if (g.sep) { Item L2; L2.k = 2; L2.l = L2.r = xs[q] + SEPW / 2; L2.t = t + 1; L2.b = b - 1; L2.col = C_SEP; D.items.push_back(L2); continue; }
        int xx = xs[q] + (badgeFirst(v, q) ? PADX : 0);
        if (!g.s.empty()) { Item T; T.k = 1; T.l = xx; T.t = t; T.b = b; T.col = g.col; T.bold = g.bold; T.s = g.s; D.items.push_back(T); xx += meas(g.s, g.bold) + (g.arrow || !g.s2.empty() ? 4 : 0); }
        if (g.arrow) { const int ax = xx + ARW / 2, d = g.arrow > 0 ? -1 : 1;   // a shaft + two head strokes; d = +1 points down
            Item a1; a1.k = 2; a1.l = a1.r = ax; a1.t = ym - 4 * d; a1.b = ym + 4 * d; a1.col = g.col; D.items.push_back(a1);
            Item a2 = a1; a2.l = ax - 3; a2.t = ym + d; a2.r = ax; a2.b = ym + 4 * d; D.items.push_back(a2);
            Item a3 = a2; a3.l = ax + 3; D.items.push_back(a3);
            xx += ARW + (!g.s2.empty() ? 4 : 0); }
        if (!g.s2.empty()) { Item T; T.k = 1; T.l = xx; T.t = t; T.b = b; T.col = g.col; T.bold = g.bold; T.s = g.s2; D.items.push_back(T); }
    }
    return D;
}

} // namespace nowl
