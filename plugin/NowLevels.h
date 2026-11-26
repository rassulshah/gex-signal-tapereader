// NowLevels.h -- the HOD / LOD "Now" line as a level map (lsHodLod 2.36.0), SDK-free
//
// (1.2.0 = lsHodLod 2.37.2, Rassul 2026-11-19 23:02 CT, NQ screenshot "SWEEP: PHL" while the forming candle was only trading at PHL: "why
// does it say sweep when there is no sweep yet, its interacting with the level so its just testing it until there is a break or a sweep"):
//   - while the FORMING candle trades at or through a watched level: the gold "TESTING: <level> Bk / Sw" (no arrow - the side is not known)
//   - BREAK / SWEEP only at the candle's CLOSE and only from the chart's own marks on that closed candle (the wy file): BREAK = a Bk mark that
//     passes the BO candle rule (range >= the prior's, range or volume up); SWEEP = a Sw mark of strength 10+ (V / R) in the right candle
//     colour (a bullish Sw closes green, a bearish one red). Shown, red / green by side, while that candle is the last closed one (until the
//     next close); a close without a mark = no BREAK / SWEEP, the line goes back to WATCH.
//   - order: TESTING BkTst > BREAK / SWEEP (closed) > TESTING Bk / Sw (live) > WATCH BkTst > WATCH Bk / Sw
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

static const char* const VERSION = "1.8.0";   // (1.8.0 = lsHodLod 2.55.0, Rassul 16:08 CT) the gold "VOB(T) WATCH" word reads "Next"; the host centres the line on the HOD / LOD box
// was "1.7.0";   // (1.7.0 = lsHodLod 2.47.0, Rassul 2026-11-26 02:30-02:32 CT "make the level line look better . make it look nice", "add it ... i'll go with your recommendation", mockup Level-Line-mockup.html option A) LEVEL CHIPS: the label "VOB(T) WATCH" in gold text (no block), each watched level its own chip - a low below price green (a VOB can form there), a high above red (a VOT) - "Bk / VOB(T)" dropped; the state words (TESTING / VOB / VOT / BREAK / WATCH BkTst Rev) stay filled badges with the level in a chip after them; NEXT / LAST BK as grey labels, the arrow green / red, the level white. Same rules and content
// was "1.6.1";   // (1.6.1 = lsHodLod 2.46.1, Rassul 2026-11-25 16:46-17:00 CT "high volume rejection from an attempted break of a level", both sides (LevelMarks 1.25.0)) VOB(T) WATCH names the nearest untouched major levels above and below again, any kind (1.5.1's highs-above / lows-below filter is off: NL_SIDE_FILTER); TESTING reads "Bk / VOB(T)"
// was "1.6.0";   // (1.6.0 = lsHodLod 2.46.0, Rassul 2026-11-25 14:28-14:29 CT "i want to relabel sweep  to VOB  (volume off the bottom) and VOT (Volume off the top)", "yes   VOB(T) WATCH    .   for the  voice just use the letters"; mockup vob-vot-mockup.html) a sweep is named VOB (bullish, off a low) / VOT (bearish, off a high) on the line: "VOB: CWL", "VOB(T) WATCH: ...", "TESTING: ONL Bk / VOB"
// was "1.5.1";   // (1.5.1 = lsHodLod 2.45.1, Rassul 2026-11-25 13:11 CT "bearish sweeps use highs and bullish sweeps use lows", LevelMarks 1.23.0) SWEEP WATCH names only highs above price and only lows below it
// was "1.5.0";   // (1.5.0 = lsHodLod 2.45.0, Rassul 2026-11-25 11:41 CT "update the level line to look for potential sweep levels", 11:47 "rename to Sweep Watch also . go", mockup watch-sweep-levels-mockup.html) the Next levels are only major levels NOT yet traded through in the current session part (day = since the RTH opening candle, as LevelMarks 1.22.0; overnight = since the session start); an empty side stays empty; the badge reads "SWEEP WATCH: ..."
// was "1.4.0";   // (1.4.0 = lsHodLod 2.44.0, Rassul 2026-11-23 14:09 CT GC screenshot, the line read "WATCH: PHL .. 2hL" with PFH 3638.1 / ONH 3636.4 right under price: "why isnt the level line catching the most obvious levels"; 14:15 "and for levels, you are also using the same levels as SW right" - yes; "build") the line names MAJOR levels only, the Sw list of LevelMarks 1.14.0 (major(): session ON / London / Asia, PD, PF, weekly, the 3-hour S / R R1 R2 S1 S2 the host hands in as keys): the prior hours' highs / lows (PHH / PHL, 2h, 3h) and VAH / VAL are no longer WATCH / Next levels, and a Bk / Sw mark on a level that is not major gives no BREAK / SWEEP badge, no BkTst state and no Last Bk part (its badge stays on the candle)
// was "1.3.1";   // (1.3.1 = lsHodLod 2.39.1, Rassul 2026-11-20 09:32 CT "fix the arrows because they are overlapping") every arrow owns a slot ASP + ARW + ASP (4 | 9 px ink incl. the 2-px pen | 4) in the measured width; text items carry their measured box [l, r] and the host draws them centred in it (IRT's left-justified text sits ~4 px right of rect.left and ran into the arrows)
// was "1.3.0";   // (1.3.0 = lsHodLod 2.38.0, Rassul 2026-11-20 06:04 CT "ok lets do this", mockup now-box-levels-mockup.html gold PROPOSED 06:02 rows) ONE short liquidity phrase for the level the badge names, from lsLimitProfile 2.68.0's DealerProfile\levelliq-<MKT>-<spb>.txt - only the states that PASSED its scoring (shown = 1); "Bids building" / "Offers leaving" / "Bids holding" / "Bids pulled", the label grey, the word in the defender's colour when it helps the defence (bids green, offers red) and the attacker's when it fails; a file older than 2 min or no state = no phrase; on a narrow pane the phrase drops FIRST (before Next / Last Bk)
// was "1.2.0";   // (1.2.0 = lsHodLod 2.37.2, 23:02 CT) live = gold "TESTING: <level> Bk / Sw"; BREAK / SWEEP only at the close, from the chart's Bk / Sw marks   // was "1.1.0";   // (1.1.0 = lsHodLod 2.37.1, Rassul 2026-11-19 22:42 CT "how can you have a BkTst Rev when you are testing a level?"): TESTING no longer names the Rev (it has not printed yet) - "TESTING: <level> BkTst <arrow>", e.g. "TESTING: PFL BkTst" + down arrow; no "Rev", no middle dot (Level then Signal, as WATCH); WATCH stays "WATCH: <level> BkTst Rev <arrow>"   // was "1.0.0"
static const long long E1904 = 2082844800LL;
static const int SW_MIN = 10;          // (1.2.0) a Sw mark counts with strength 10+ (BVMax or BRMax, "V12" / "R58" / "V99+"), as the spoken alerts
static const double TST_MIN = 0.382;   // (1.1.0, Rassul 22:46 CT "after it breaks, and continues, you can then have the test of the level after it has the minimum required retracement") = LevelMarks.h's Tst minimum
static const size_t MAX_BYTES = 512 * 1024;

struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0, v = 0; };       // t = candle END, local secs (unix-naive local)
struct Mark { long long t = 0; std::string code, lv, st; int dir = 0; double px = 0; };   // Bk / FT from the wy file
struct Key { std::string name; double px = 0; };

// colours (BGR like the host uses COLORREF 0x00BBGGRR)
// colours as the host's COLOR (0x00RRGGBB, as hl::C_GREEN): labels grey, levels gold, bearish red, bullish green
// (1.7.0) the chips: text / border / fill - a low (green), a high (red), a level with no side yet (gold)
static const unsigned CH_DN_T = 0x0086EFAC, CH_DN_B = 0x001F6B3A, CH_DN_F = 0x000D2617, CH_UP_T = 0x00FCA5A5, CH_UP_B = 0x007A2626, CH_UP_F = 0x002A1010, CH_GD_T = 0x00F0C040, CH_GD_B = 0x006B5A1F, CH_GD_F = 0x00261F0D;
static const unsigned C_GREY = 0x0094A3B8, C_GOLD = 0x00F0C040, C_GREEN = 0x0022C55E, C_RED = 0x00EF4444, C_WHITE = 0x00E5E7EB, C_BLACK = 0x00000000, C_SEP = 0x00475569, C_BG = 0x0015181D, C_BORDER = 0x003A3F47;

// one piece of the line: s, then an arrow (+1 up / -1 down, drawn with pen strokes - IRT's drawText is cp1252, no arrow glyphs), then s2;
// sep = the thin vertical separator between parts; badge = a filled box (fill) with black bold text
struct Seg { std::string s, s2; int arrow = 0; unsigned col = C_WHITE; bool bold = false; bool badge = false; bool sep = false; unsigned fill = 0; int part = 0; int drop = 0; bool chip = false; unsigned bcol = 0; };   // (1.7.0) chip = one level in its own outlined box (fill + border bcol)   // drop: 0 keep, 2 the Last Bk part, 3 (1.3.0) the liquidity phrase (dropped first)
// (1.3.0) one row of lsLimitProfile's level liquidity file: "L|name|level_tick|B or O|state|shown|since_ct_s|conf"
struct LiqRow { std::string name; bool bid = true; std::string st; bool shown = false; long long since = 0; };
struct Liq { bool ok = false; std::vector<LiqRow> rows; std::string why = "no file"; };
static const long long LIQ_STALE_S = 120;   // a file older than 2 min = no phrase (the Limit Profile rewrites it at least every 60 s)

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
    if (n == "R1" || n == "R2" || n == "S1" || n == "S2") return 45;   // (1.4.0) the 3-hour S / R
    if (n == "PHH" || n == "PHL") return 50;
    if (n.size() == 3 && n[0] >= '2' && n[0] <= '9' && n[1] == 'h' && (n[2] == 'H' || n[2] == 'L')) return 60;
    return -1;
}
// (1.4.0) the levels the line may name = the levels that may name a Sw (LevelMarks swMajor)
inline bool major(const std::string& n) { const int p = pri(n); return p == 0 || p == 10 || p == 20 || p == 40 || p == 45; }
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
        if (m.code == "Bk" || m.code == "Sw") { if (f.size() < 5) continue; long long px = 0; if (!toLL(f[4], px) || px <= 0) continue; m.lv = f[3]; m.px = (double)px * tick; m.st = f.size() > 5 ? f[5] : std::string(); if (pri(m.lv) < 0) continue; }   // (1.2.0) Sw marks too (the closed SWEEP)
        else if (m.code != "FT") continue;
        out.push_back(m);
    }
    std::stable_sort(out.begin(), out.end(), [](const Mark& a, const Mark& b) { return a.t < b.t; });
    return out;
}

// (1.2.0) a mark's strength: "V45" / "R99+" -> 45 / 99; "" or garbage -> 0
inline int strengthOf(const std::string& s) {
    if (s.size() < 2 || (s[0] != 'V' && s[0] != 'R')) return 0;
    std::string d = s.substr(1); if (!d.empty() && d.back() == '+') d.pop_back();
    long long n = 0; if (!toLL(d, n) || n < 0 || n > 999) return 0; return (int)n;
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
// (1.2.0, 23:02 CT) state 1 BREAK / 2 SWEEP = the LAST CLOSED candle carries the chart's Bk / Sw mark (Bk: the BO candle rule; Sw: strength
// 10+, right colour), shown until the next close; state 3 TESTING = the forming candle trades at / through a watched level (the nearest
// above / below the last close), gold, no side. (2.36.0 had called BREAK / SWEEP live from the forming candle.)
struct Live { int state = 0; int dir = 0; std::string name; double px = 0; std::string why; };
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
// (1.5.1 / 1.6.0) the side of a short level name: +1 a high (ONH LHI AHI PDH PFH PWH CWH R1 R2), -1 a low, 0 neither
inline int sideOfName(const std::string& s) {
    if (s == "R1" || s == "R2") return 1;
    if (s == "S1" || s == "S2") return -1;
    const size_t L = s.size();
    if (L >= 2 && s.compare(L - 2, 2, "HI") == 0) return 1;
    if (L >= 2 && s.compare(L - 2, 2, "LO") == 0) return -1;
    if (L >= 1 && s[L - 1] == 'H') return 1;
    if (L >= 1 && s[L - 1] == 'L') return -1;
    return 0; }
static const bool NL_SIDE_FILTER = false;   // (1.6.1) both sides count (LevelMarks 1.25.0): any major level can give a VOB / VOT
// (1.5.0) RTH open minute per market (CT) = lvm::rthOpenMin (LevelMarks.h; t_hl2450 checks parity); -1 = unknown market (whole session counts)
inline int rthOpenMin(const std::string& mkt) {
    if (mkt == "ES" || mkt == "NQ" || mkt == "MES" || mkt == "MNQ") return 8 * 60 + 30;
    if (mkt == "CL" || mkt == "NG" || mkt == "MCL") return 8 * 60;
    if (mkt == "GC" || mkt == "EU" || mkt == "6E" || mkt == "MGC") return 7 * 60 + 20;
    if (mkt == "HG") return 7 * 60 + 10;
    return -1; }
// (1.5.0) the first closed candle of the current session part: in the day session (candle END after the RTH open and at / before 5 PM CT) the RTH
// opening candle (same rule as lvm::dayStart), else 0 (the session's first candle; B holds the current session only)
inline int partStart(const std::vector<Bar>& B, int f, int openMin) {
    if (openMin < 0 || f < 0 || f >= (int)B.size()) return 0;
    auto inDay = [&](int k) { long long q = B[(size_t)k].t % 86400; if (q < 0) q += 86400; const long long m = q / 60; return m > openMin && m <= 17 * 60 && q > (long long)openMin * 60; };
    if (!inDay(f)) return 0;
    int k = f; while (k > 0 && inDay(k - 1) && B[(size_t)k].t - B[(size_t)k - 1].t < 6 * 3600) --k;
    return k; }
inline Out compute(const std::vector<Bar>& B, const std::vector<Mark>& marksIn, const std::vector<Key>& keysIn, double live, double tick, int openMin = -2) {
    Out o; o.live = live; if (B.size() < 3 || !std::isfinite(live) || !(tick > 0)) return o; o.ok = true;
    const int n = (int)B.size();
    auto idxOf = [&](long long t) { for (int i = n - 2; i >= 0; i--) if (B[(size_t)i].t == t) return i; return -1; };   // closed candles only
    std::vector<Mark> marks; for (const Mark& m : marksIn) { if (m.code != "Bk" || !major(m.lv)) continue; const int k = idxOf(m.t); if (k >= 0 && bkOk(B, k)) marks.push_back(m); }   // (1.4.0) breaks of major levels only
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
    std::vector<Key> keys; for (const Key& kk : keysIn) { const std::string s = shortName(kk.name); if (major(s) && kk.px > 0) keys.push_back(Key{ s, kk.px }); }   // (1.4.0) was pri 0..49 + the hour levels below   // the hour levels come live from the bars below, not the key file's snapshot
    // (1.4.0) no hour levels: was  for (const Key& kk : hourKeys(std::vector<Bar>(B.begin(), B.end() - 1), B.back().t)) keys.push_back(kk);
    const double eps = tick * 0.5;
    {   // (1.2.0) 1. the closed candle's own marks: BREAK = a Bk mark passing the BO candle rule, SWEEP = a Sw mark of strength 10+ in the
        // right colour - shown while that candle is the last closed one (until the next close)
        const int lc = n - 2; const Bar& c0 = B[(size_t)lc];
        for (const Mark& m : marksIn) {
            if (m.t != c0.t || !major(m.lv)) continue;
            if (m.code == "Bk" && bkOk(B, lc)) { o.bs.state = 1; o.bs.dir = m.dir; o.bs.name = m.lv; o.bs.px = m.px; break; }
        }
        if (!o.bs.state) for (const Mark& m : marksIn) {
            if (m.t != c0.t || m.code != "Sw" || !major(m.lv)) continue;
            if (strengthOf(m.st) < SW_MIN) { o.bs.why = "Sw mark under strength 10"; continue; }
            if (m.dir > 0 ? !(c0.c > c0.o) : !(c0.c < c0.o)) { o.bs.why = "Sw mark in the wrong candle colour"; continue; }
            o.bs.state = 2; o.bs.dir = m.dir; o.bs.name = m.lv; o.bs.px = m.px; break;
        }
    }
    if (!o.bs.state) {   // (1.2.0) 2. the forming candle at / through a watched level (the nearest above / below the last close): TESTING, gold
        const Bar& f = B.back(); const double ref = B[(size_t)n - 2].c;
        Key wu, wd; bool hu = false, hd = false;
        for (const Key& kk : keys) {
            if (kk.px > ref + eps && (!hu || kk.px < wu.px - eps || (std::fabs(kk.px - wu.px) <= eps && pri(kk.name) < pri(wu.name)))) { wu = kk; hu = true; }
            if (kk.px < ref - eps && (!hd || kk.px > wd.px + eps || (std::fabs(kk.px - wd.px) <= eps && pri(kk.name) < pri(wd.name)))) { wd = kk; hd = true; }
        }
        const bool au = hu && f.h >= wu.px - eps, ad = hd && f.l <= wd.px + eps;   // at or through
        const bool pickU = au && (!ad || std::fabs(live - wu.px) <= std::fabs(live - wd.px));
        if (pickU) { o.bs.state = 3; o.bs.name = wu.name; o.bs.px = wu.px; }
        else if (ad) { o.bs.state = 3; o.bs.name = wd.name; o.bs.px = wd.px; }
    }
    // (1.5.0) sweep candidates: a level a closed candle of the current session part already traded through is skipped (openMin -2 = old callers: no filter)
    double hiP = -INFINITY, loP = INFINITY;
    if (openMin != -2) for (int k = partStart(B, n - 2, openMin); k <= n - 2; k++) { hiP = std::max(hiP, B[(size_t)k].h); loP = std::min(loP, B[(size_t)k].l); }
    // (1.5.1) the side a sweep can come from: above price only a HIGH (a bearish Sw), below only a LOW (a bullish Sw)

    for (const Key& kk : keys) {
        if (NL_SIDE_FILTER && openMin != -2 && kk.px > live + eps && sideOfName(kk.name) != 1) continue;    // (1.5.1) above: highs only (off since 1.6.1)
        if (NL_SIDE_FILTER && openMin != -2 && kk.px < live - eps && sideOfName(kk.name) != -1) continue;   // (1.5.1) below: lows only (off since 1.6.1)
        if (kk.px > live + eps && hiP > kk.px + eps) continue;   // (1.5.0) above, already traded through
        if (kk.px < live - eps && loP < kk.px - eps) continue;   // (1.5.0) below, already traded through
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

// (1.3.0) the level liquidity file: header "LEVELLIQ,1,<ver>", MARKET|<mkt>, SPB|<spb>, STAMP|<CT s>, then rows. Wrong market / bar size,
// garbled, too big or older than LIQ_STALE_S = nothing.
inline Liq parseLiq(const std::string& text, const std::string& mkt, long long spb, long long nowCt) {
    Liq q; if (text.empty()) return q; if (text.size() > 64 * 1024) { q.why = "too big"; return q; }
    const std::vector<std::string> L = split(text, '\n');
    if (L.empty() || L[0].compare(0, 11, "LEVELLIQ,1,") != 0) { q.why = "bad header"; return q; }
    std::string m; long long sp = -1, st = -1;
    for (const std::string& ln : L) { const std::vector<std::string> f = split(ln, '|');
        if (f.size() == 2 && f[0] == "MARKET") m = f[1];
        else if (f.size() == 2 && f[0] == "SPB") { if (!toLL(f[1], sp)) sp = -1; }
        else if (f.size() == 2 && f[0] == "STAMP") { if (!toLL(f[1], st)) st = -1; }
        else if (f.size() >= 7 && f[0] == "L" && (f[3] == "B" || f[3] == "O") && (f[5] == "0" || f[5] == "1") && q.rows.size() < 200) {
            LiqRow r; r.name = f[1]; r.bid = f[3] == "B"; r.st = f[4]; r.shown = f[5] == "1"; long long s0 = 0; if (toLL(f[6], s0)) r.since = s0; q.rows.push_back(r); } }
    if (m != mkt) { q.rows.clear(); q.why = "other market"; return q; }
    if (sp != spb) { q.rows.clear(); q.why = "other bar size"; return q; }
    if (st < 0 || nowCt - st > LIQ_STALE_S || st - nowCt > LIQ_STALE_S) { q.rows.clear(); q.why = "stale"; return q; }
    q.ok = true; q.why = "ok"; return q;
}
// does a file row name this (short) level? (a merged row "PFL\xB7PDL" / "PFL/PDL" names each)
inline bool liqNames(const std::string& row, const std::string& lv) {
    std::string cur; for (size_t i = 0; i <= row.size(); ++i) { const char c = i < row.size() ? row[i] : '/';
        if (c == '/' || c == '\xB7') { if (!cur.empty() && shortName(cur) == shortName(lv)) return true; cur.clear(); } else cur += c; }
    return false;
}
// the phrase word for a SHOWN state: building / leaving / holding (X or H) / pulled (P); colour: helps the defence = the defender's colour
// (bids green, offers red), fails = the attacker's (bids red, offers green)
inline bool liqWord(const LiqRow& r, std::string& w, unsigned& col) {
    if (!r.shown) return false;
    bool helps = true;
    if (r.st == "building") w = "building"; else if (r.st == "leaving") { w = "leaving"; helps = false; } else if (r.st == "X" || r.st == "H") w = "holding";
    else if (r.st == "P") { w = "pulled"; helps = false; } else return false;
    col = (helps == r.bid) ? C_GREEN : C_RED; return true;
}
// the row for the badge's level(s): the newest shown row of the first level (in badge order) that has one
inline bool liqPick(const Liq& q, const std::vector<std::string>& lvs, LiqRow& out, std::string& lvName) {
    if (!q.ok) return false;
    for (const std::string& lv : lvs) { const LiqRow* best = nullptr; std::string w; unsigned c = 0;
        for (const LiqRow& r : q.rows) if (liqNames(r.name, lv) && liqWord(r, w, c) && (!best || r.since > best->since)) best = &r;
        if (best) { out = *best; lvName = lv; return true; } }
    return false;
}

// the segments, most important first; named levels only (Rassul 18:42 "get rid of the values, just keep the named levels"): no prices, no
// distances, no strength, no %, no time (18:43) - no numbers at all. drop = 2: the Last Bk part, dropped first when the line does not fit.
// Consecutive badge pieces of one part are ONE filled badge.
inline std::vector<Seg> segments(const Out& o, const Liq& lq = Liq()) {
    std::vector<Seg> v; if (!o.ok) return v;
    std::vector<std::string> named;   // (18:50) no level named twice
    auto has = [&](const std::string& n) { return std::find(named.begin(), named.end(), n) != named.end(); };
    auto add = [&](const std::string& s, int arrow, const std::string& s2, unsigned col, bool bold, int part, int drop = 0) { Seg g; g.s = s; g.arrow = arrow; g.s2 = s2; g.col = col; g.bold = bold; g.part = part; g.drop = drop; v.push_back(g); };
    auto badge = [&](const std::string& s, int arrow, const std::string& s2, unsigned fill, int part) { Seg g; g.s = s; g.arrow = arrow; g.s2 = s2; g.col = C_BLACK; g.bold = true; g.badge = true; g.fill = fill; g.part = part; v.push_back(g); };
    auto sep = [&](int part, int drop = 0) { if (v.empty()) return; Seg g; g.sep = true; g.col = C_SEP; g.part = part; g.drop = drop; v.push_back(g); };
    // (1.7.0) a level chip: kind -1 = green (a low / bullish), +1 = red (a high / bearish), 0 = gold (no side yet); arrow in the chip before the name
    auto chip = [&](int arrow, const std::string& name, int kind, int part, int drop = 0) { Seg g; g.arrow = arrow; g.s2 = name; g.bold = true; g.chip = true; g.part = part; g.drop = drop;
        g.col = kind < 0 ? CH_DN_T : kind > 0 ? CH_UP_T : CH_GD_T; g.bcol = kind < 0 ? CH_DN_B : kind > 0 ? CH_UP_B : CH_GD_B; g.fill = kind < 0 ? CH_DN_F : kind > 0 ? CH_UP_F : CH_GD_F; v.push_back(g); };
    // (1.7.0) a level after a grey label: the arrow green (up) / red (down), the name white
    auto arrowLv = [&](int arrow, const std::string& name, int part, int drop = 0) { add("", arrow, "", arrow > 0 ? C_GREEN : C_RED, true, part, drop); add(name, 0, "", C_WHITE, true, part, drop); };
    const std::string dot = "\xB7";   // cp1252 middle dot
    // (1.3.0) the liquidity phrase for the badge's level(s), right after the badge (part 4, dropped first); one level named: "Bids building",
    // several (the WATCH badge): "<level> bids leaving"
    auto phrase = [&](const std::vector<std::string>& lvs) { LiqRow r; std::string lvN; if (!liqPick(lq, lvs, r, lvN)) return; std::string w; unsigned c = 0; if (!liqWord(r, w, c)) return;
        Seg g; g.sep = true; g.col = C_SEP; g.part = 4; g.drop = 3; v.push_back(g);
        add(lvs.size() > 1 ? lvN + (r.bid ? " bids" : " offers") : std::string(r.bid ? "Bids" : "Offers"), 0, "", C_GREY, false, 4, 3); add(w, 0, "", c, true, 4, 3); };
    const Next dn = o.dn, up = o.up;
    const bool testing = o.tst.ok && o.tst.state == 2;
    if (!testing && (o.bs.state == 1 || o.bs.state == 2)) {   // 0. BREAK / SWEEP of the closed candle (1.2.0: from its mark), red / green by side; then the next level that way only
        badge(std::string(o.bs.state == 1 ? "BREAK" : o.bs.dir > 0 ? "VOB" : "VOT"),   /* (1.7.0) the word, then the level in a chip; (1.6.0) was "SWEEP: " */ 0, "", o.bs.dir < 0 ? C_RED : C_GREEN, 1);
        chip(o.bs.dir, o.bs.name, o.bs.dir < 0 ? 1 : -1, 1); named.push_back(o.bs.name); phrase({ o.bs.name });
        const Next& nx = o.bs.dir < 0 ? dn : up;
        if (nx.ok && !has(nx.name)) { sep(2); add("NEXT", 0, "", C_GREY, false, 2); arrowLv(o.bs.dir, nx.name, 2); named.push_back(nx.name); }
    } else if (!testing && o.bs.state == 3) {   // (1.2.0, 23:02 CT) the forming candle at / through a watched level: gold, no side yet; then the next levels
        badge("TESTING", 0,   /* (1.7.0) the word, then the level in a gold chip (either side can reject the attempt); (1.6.1) was "TESTING: <lv> Bk / VOB(T)" */ "", C_GOLD, 1);
        chip(0, o.bs.name, 0, 1); named.push_back(o.bs.name); phrase({ o.bs.name });
        const bool d = dn.ok && !has(dn.name), u = up.ok && !has(up.name) && !(d && up.name == dn.name);
        if (d || u) {
            sep(2); add("NEXT", 0, "", C_GREY, false, 2);
            if (d) { arrowLv(-1, dn.name, 2); named.push_back(dn.name); }
            if (d && u) add(dot, 0, "", C_GREY, false, 2);
            if (u) { arrowLv(1, up.name, 2); named.push_back(up.name); }
        }
    } else if (o.tst.ok) {   // 1. a BkTst coming / in progress: "WATCH: <level> · BkTst Rev <arrow>" (18:52, gold) / "TESTING: ..." (18:48, red / green by side)
        badge(o.tst.state == 2 ? "TESTING BkTst" : "WATCH BkTst Rev", 0, "", o.tst.state != 2 ? C_GOLD : o.tst.dir < 0 ? C_RED : C_GREEN, 1);   // (18:53) every WATCH is gold; only TESTING is red / green; (1.7.0) the level in a chip after it
        chip(o.tst.dir < 0 ? -1 : 1, o.tst.lv, o.tst.dir < 0 ? 1 : -1, 1);
        named.push_back(o.tst.lv); phrase({ o.tst.lv });
        const bool d = dn.ok && !has(dn.name), u = up.ok && !has(up.name) && !(d && up.name == dn.name);
        if (d || u) {   // then the next levels, plainly (not in a badge: one badge at a time, 18:47)
            sep(2); add("NEXT", 0, "", C_GREY, false, 2);
            if (d) { arrowLv(-1, dn.name, 2); named.push_back(dn.name); }
            if (d && u) add(dot, 0, "", C_GREY, false, 2);
            if (u) { arrowLv(1, up.name, 2); named.push_back(up.name); }
        }
    } else if (dn.ok || up.ok) {   // 2. else the gold WATCH badge with the next levels and their arrows (18:50): "WATCH: Bk / Sw at ↓ PDL · ↑ ONL"
        // (19:03 "for watch it should be in this format : Watch Level Signal") "WATCH: PDL Bk / Sw ↓ · ONL Bk / Sw ↑" - below first, then above
        // (19:06 "update to WATCH: PDL ↓ · ONL↑ · Bk / Sw") the levels with their arrows, then the signal once at the end
        // (1.5.0, 11:47 "rename to Sweep Watch") was "WATCH: "
        // (1.6.0, 14:29 "VOB(T) WATCH") was "SWEEP WATCH: " ... "Bk / Sw"
        // (1.7.0, option A) "VOB(T) WATCH" as gold text, each level its own chip: the low below price green, the high above red; no "Bk / VOB(T)"
        add("Next", 0, "", C_GOLD, true, 2);   // (1.8.0 = lsHodLod 2.55.0, Rassul 2026-11-26 16:08 CT "update Vob Watch to Next and center it") was "VOB(T) WATCH"
        if (dn.ok) { chip(-1, dn.name, -1, 2); named.push_back(dn.name); }
        if (up.ok) { chip(1, up.name, 1, 2); named.push_back(up.name); }
        { std::vector<std::string> lvs; if (dn.ok) lvs.push_back(dn.name); if (up.ok) lvs.push_back(up.name); phrase(lvs); }
    }
    if (o.haveBk && !has(o.bk.lv)) {   // 3. the last Bk (context, dropped first) - only when its level is not on the line already (18:50)
        sep(3, 2); add("LAST BK", 0, "", C_GREY, false, 3, 2);
        arrowLv(o.bk.dir < 0 ? -1 : 1, o.bk.lv, 3, 2);   // (1.7.0) the arrow green / red, the level white
    }
    return v;
}
// what fits: level 0 = all, 3 = without the liquidity phrase (1.3.0, dropped first), 1 = also without the Last Bk part
inline std::vector<Seg> fit(const std::vector<Seg>& all, int level) { std::vector<Seg> v; for (const Seg& s : all) { if (level >= 1 && s.drop == 3) continue; if (level == 1 && s.drop == 2) continue; v.push_back(s); } return v; }   // (1.3.0) level 3 = without the liquidity phrase only; level 1 (as before) = without the Last Bk part (and the phrase)
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
        if (!g.s2.empty()) x += (x.empty() ? std::string() : std::string(" ")) + u(g.s2);   // (1.7.0) a chip with no arrow: no leading space
        if (g.chip) x = "{" + x + "}";   // (1.7.0) a level chip
        if (badgeFirst(v, q)) x = "[" + x;
        if (badgeLast(v, q)) x += "]";
        if (!t.empty()) t += ' ';
        t += x;
    }
    return t;
}

// the drawing, SDK-free: the host measures text (meas(s, bold)) and draws the items in order. One line centred on cx with its bottom at
// yBottom, inside [xMin, xMax]; the Last Bk part drops first, then pieces drop off the right end (never a dangling separator) until it fits.
struct Item { int k = 0; int l = 0, t = 0, r = 0, b = 0; unsigned col = 0, fill = 0; bool bold = false, arw = false; std::string s; };   // k: 0 filled box (col = border), 1 text (measured box l..r, row t..b), 2 line (arw = an arrow stroke)
struct Draw { bool ok = false; int l = 0, t = 0, r = 0, b = 0; int level = 0; int pieces = 0, shown = 0; std::vector<Item> items; };
static const int ARW = 9, ASP = 4, GAP = 5, PADX = 5, SEPW = 13;   // (1.3.1) ARW = the arrow's ink width with the 2-px pen; ASP = clear space each side
template <class M> inline int segW(const std::vector<Seg>& v, size_t q, M meas) {
    const Seg& g = v[q]; if (g.sep) return SEPW;
    int w = 0; if (!g.s.empty()) w += meas(g.s, g.bold);
    if (g.arrow) w += ASP + ARW + ASP;
    if (!g.s2.empty()) w += (g.arrow ? 0 : (w ? 4 : 0)) + meas(g.s2, g.bold);
    return w + (badgeFirst(v, q) ? PADX : 0) + (badgeLast(v, q) ? PADX : 0) + (g.chip ? 2 * PADX : 0);
}
template <class M> inline Draw layout(const std::vector<Seg>& all, M meas, int cx, int yBottom, int xMin, int xMax, int fs) {
    Draw D; if (all.empty() || xMax - xMin < 40) return D;
    const int h = fs + 8, padL = 6;
    auto width = [&](const std::vector<Seg>& v) { int w = 2 * padL; for (size_t q = 0; q < v.size(); q++) w += segW(v, q, meas) + (q ? GAP : 0); return w; };
    std::vector<Seg> v = fit(all, 0); int level = 0;
    if (width(v) > xMax - xMin && fit(all, 3).size() < v.size()) { v = fit(all, 3); level = 3; }
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
    for (size_t q = 0; q < v.size(); q++) if (v[q].chip) {   // (1.7.0) the chips: a dark tint inside a 1-px solid border of their colour
        Item F; F.k = 0; F.l = xs[q]; F.t = t; F.r = xs[q] + segW(v, q, meas); F.b = b; F.fill = v[q].fill; F.col = v[q].bcol; D.items.push_back(F); }
    for (size_t q = 0; q < v.size(); q++) { const Seg& g = v[q];
        if (g.sep) { Item L2; L2.k = 2; L2.l = L2.r = xs[q] + SEPW / 2; L2.t = t + 1; L2.b = b - 1; L2.col = C_SEP; D.items.push_back(L2); continue; }
        int xx = xs[q] + (badgeFirst(v, q) ? PADX : 0) + (g.chip ? PADX : 0);
        if (!g.s.empty()) { Item T; T.k = 1; T.l = xx; T.t = t; T.b = b; T.col = g.col; T.bold = g.bold; T.s = g.s; T.r = xx + meas(g.s, g.bold); D.items.push_back(T); xx = T.r + (!g.arrow && !g.s2.empty() ? 4 : 0); }
        if (g.arrow) { const int ax = xx + ASP + ARW / 2, d = g.arrow > 0 ? -1 : 1;   // a shaft + two head strokes in the arrow's own slot; d = +1 points down
            Item a1; a1.k = 2; a1.arw = true; a1.l = a1.r = ax; a1.t = ym - 4 * d; a1.b = ym + 4 * d; a1.col = g.col; D.items.push_back(a1);
            Item a2 = a1; a2.l = ax - 3; a2.t = ym + d; a2.r = ax; a2.b = ym + 4 * d; D.items.push_back(a2);
            Item a3 = a2; a3.l = ax + 3; D.items.push_back(a3);
            xx += ASP + ARW + ASP; }
        if (!g.s2.empty()) { Item T; T.k = 1; T.l = xx; T.t = t; T.b = b; T.col = g.col; T.bold = g.bold; T.s = g.s2; T.r = xx + meas(g.s2, g.bold); D.items.push_back(T); }
    }
    return D;
}

} // namespace nowl
