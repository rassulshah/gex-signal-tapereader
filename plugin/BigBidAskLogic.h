#ifndef LS_BIGBIDASK_LOGIC_H
#define LS_BIGBIDASK_LOGIC_H
// BigBidAskLogic.h 1.0.0 (2026-10-14 night) -- the SDK-free rules of lsBigBidAsk (BigBidAsk.cpp). Tested on Linux by
// tests_bigba/test_bigba_logic.cpp (ASan / UBSan). Rassul approved the mockup at 20:41 CT (mockup-biggest-bid-ask.png v4; the
// circle as in v5): on the price chart, every candle,
//   RED number LEFT of the candle  = at the price where the most lots traded AT THE BID in that candle (sellers hitting), the lots;
//   GREEN number RIGHT of the candle = at the price where the most lots traded AT THE ASK (buyers lifting), the lots.
// Ties go to the price nearest the candle's close (still equal: the bid side takes the lower price, the ask side the higher).
// BOLD = at least <threshold> x (default 2.0x) the median of the same side's biggest over the prior 20 candles (past candles only,
// at least 10 with data; ratios kept in hundredths so the chart and a rebuild from the record never differ).
// ABSORPTION (thin 1 px circle round the number, its own colour), decided at the candle's close from frozen facts, never repainted:
//   bullish: the red number is bold, its price is at or below the halfway mark ((high + low) / 2), and the candle closes GREEN
//            (close > open) above that price;
//   bearish: the green number is bold, its price is at or above the halfway mark, and the candle closes RED below that price.
//   The forming candle never gets a circle. Prices are compared in whole ticks only (2 x key vs high + low keys).
// No overlap: each number tries its own row, then one row up, then one row down, never touching a candle, another number, the
// right-side columns or the pane edges; a number with no room is not drawn, except a bold one, which is still drawn on its own row
// when it touches no other number ("if there is still no room, show only the bold ones"). The closed candles' bold numbers (the
// circles among them) are laid out first, then the plain ones and the forming candle's, each group in time order from older ones
// only: a newer number never takes a row from an older one of its group, and a plain number never takes one from a bold one.
#include <algorithm>
#include <functional>
#include <cctype>
#include <map>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace bba {

static const char* const VERSION = "1.26.0";   // (1.26.0, 2026-10-22 Rassul 18:01 CT "its as simple as that"; built on 1.25.1 = the boxes LOG ONLY, approved 17:39) no OB / BO boxes: the BO pivot = the last 3 INTACT swings of his 1-bar ZigZag (swingBreaksPx), BO letter for both sides (green above a breakout, red below a breakdown), OB = the biggest bid / ask cell near the strong wave's pivot (OrderBlockLogic.h 2.0.0) with the OB letter;   // (1.25.0, 2026-10-22 agreed with Rassul 16:00-16:01 CT) OB / BO boxes of strong waves (OrderBlockLogic.h 1.0.0), recorded to lsFlexLevels\OrderBlock, hover on the box; (1.24.0, 2026-10-22 Rassul approved the Rly mockup v0.2 15:10-15:15 CT) Rly (rally) on 3-min charts from RallyLogic.h 1.0.0 (rly2.py v0.2 port), recorded to lsFlexLevels\Rly, hover on the Rly text; (1.23.0, 2026-10-22 Rassul approved BO / BD rules) BO only on a GREEN candle, BD only on a RED candle; SAME-WAVE rule: a wick-only break of the pivot is confirmed when the breakout candle OR a later candle of the same up-wave (BO) / down-wave (BD) CLOSES beyond the pivot - the mark goes on the confirming candle, decided on closed candles, never repainted (boWaves); failed wick breaks (the wave ended first) are LOGGED only (reason wick_wave_ended), no FB mark; record line BBO| 24 fields (t0 = breakout candle, t = mark candle), pre-1.23 lines ignored; (1.22.0, 2026-10-22 Rassul 06:43 "then do it" / 06:49 "then require it") the circle is drawn ONLY on a candle with the SHARED absorption A of its side (SharedAbsorb.h 1.0.0, lsFlexLevels\SharedAbsorb\SAB-<MKT>-<spb>.csv) that closed in the defenders' colour - bid print at a bullish A (green candle), ask print at a bearish A (red candle); the 1.20.0 order-book circle rule is retired (the book is the shared A hover's upgrade); skip first / last 15 min stays the default (07:08 "first 15 min needs to be skipped"); (1.21.0, 2026-10-21, Rassul approved breakout-mockup-v2 20:10 CT "no dashed pivot line. yes build") BREAKOUT / BREAKDOWN: BO / BD letter + the breakout print (lots, biggest buy / sell cell) ON the breakout candle, decided once at the close, BBO| record per session, setting "Breakout / breakdown (BO / BD)"; (1.20.0, 2026-10-21, MBO Roadmap Top Defender #1, Rassul approved mockup v2) a drawn print in the candle's WICK at a price the order book shows ABSORBED (lsBidAskStats / SignalBoard O: traded >= 2x shown + >= 2 refills) gets a circle in the WINNER's colour; decided once per closed candle, kept in BigBidAsk.abs-<MKT>-<spb>.txt; no order feed = no circle; setting "Circle absorbed prints (order book)"; board B flag bit 0 = absorbed; (1.19.2) the settings window behind a fault guard (IRT faulted in parmsLoad / parmsApply on a copy with no chart); the market read safely (SettingsGuard.h); (1.19.1) number settings saved as the window shows them (his font 10 was lost); dialog log; (1.19.0) (1.19.0, Rassul 21:47-21:51) the number = candles back it is the biggest of its side (cap 50); settings: Biggest in (default 10), skip first / last minutes (15 / 15), font, number / bid / ask colours (BigBidAskSettings.h); (1.18.0, Rassul 2026-10-20 21:26-21:32) ONE print rule: the biggest cell of its side bigger than that side's biggest cell on each of the 10 candles before it (biggestOfLast, TOP_N); no Big Lot tier / pivot / level / trend filter on the chart, board or print log; font 7 (was 8, "slightly smaller"), every number white; first / last 15 minutes of every session still skipped; (1.17.1, Rassul 23:37 CT 2026-10-18 "get rid of the hover over on the gex levels and other places") hovers off in BigBidAsk.cpp (BBA_HOVERS), logic unchanged; (1.17.0, Rassul approved 19:57 CT) no user settings: fixedSettings() (BL5 all / BL10 + BL20 Trend PB + BO, no category 4); (1.16.2, 2026-10-18) the Big Lot box = the HOD/LOD TABLE's height when docked beside it (BoxDock.h placeMatched, lsHodLod 2.4.4 publishes the table alone), rows spread evenly; a blank window read is never saved (looksBlank + SettingsHost 1.1.0); (1.16.1, 2026-10-18 settings audit) the settings window from BigBidAskSettings.h / SettingsHost.h: same 87 rows / order / BBA7 file, the volume levels on two lines of three (the line was ~422 px, "Wk VAL" cut off), group masters through the layer (compared with their last state);  (1.16.0, 2026-10-18, Rassul approved 15:03 CT) the statistics box becomes the compact Big Lot box: BL5 / BL10 / BL20 = the rolling average of the CURRENT session type (RTH / overnight), docked to the HOD/LOD table (BoxDock.h), everything else in its hover (typeAvg / boxRows / boxHover); category order 1 > 4 > 2 / 3 (his category-4 size wins over Big Lot 10 / 20: catOrder / firstCat); (1.15.0, 2026-10-18 audit B finding 2) the Big Lot tiers are decided ONCE PER SESSION from complete earlier sessions only (3 - 5, each with 3/4 of its day candles; today's partial session and partly read sessions never count), and only once the chart history before the session is fully read: a print's tier is fixed for its candle and never re-judged by a later recompute or by what is in view (TierSet / tiersFor); the facts are read from the chart's full recent history (8 sessions), not from the view, and recorded for 8 sessions; the SignalBoard B events come from the record for the last 120 closed candles whatever is on screen (+ bit 7 = pivot confirmed); the 5-s log-normal curve fit is gone (status shows the S-tiers); (1.14.1, Rassul 17:19-17:37) Trend PB band checked from the start of the pullback THROUGH the print's own candle (his 96 at 12:42 Oct 16); Trend BO only on a candle in the up-wave (down-wave for a breakdown); ES category 4 = 1,000; (1.14.0) (1.14.0, Rassul 2026-10-17 16:29-16:47) one "Show" choice per category (All prints / Near pivot (either direction) / Near a level / Pivot near a level / Trend PB / Trend BO / Trend PB + BO), category 4 = every print of its size (time filter only), the pivot judged against the candle's own swing (may repaint until the pivot is confirmed), the Trend PB VWAP band rule, Trend BO needs the trend, box colours of their own, new defaults, category 4 size per market; (1.13.0) a Setup per category (Any / Pullback with trend / Reversal at a level / Breakout through the higher of the last 2 pivots), RTH only option; (1.12.0) hover, print log + nightly score, Liquidity Profile read, NG / 6E divider 1; (1.11.0) (1.11.0) category 4 (>= lots), a font per category, time filter, near a level (ATR above / below + sweeps; key / gamma / volume levels), statistics; (1.10.0) Big Lot categories with his own N (1 - 100), per-category pivot filter and colours, day = 8:30 AM - 4:00 PM; (1.9.0) Big Lot S20 / S10 / S5 tiers, one font size (8), a colour per tier and side, box / circle / dim switches (all off), circle colour; (1.8.0) cyan digits = Big Lot S5 (the size reached ~5 times a session, day / night) instead of 3SD; (1.7.3) status counts the cyan 3SD+ prints drawn; (1.7.2) 3SD+ prints in cyan; (1.7.1) FIX: numbers never drew (y / width / height of every number were commented out); (1.7.0) sizes are Big Lot S20 / S10 / S5 (labels);   // (1.6.1) numbers that poke out of the area slide back in instead of vanishing; area fallback; drop reasons on the status file;   // (1.6.0) ON the candle: white number, box = big print / circle = absorption print in the side colour, 3SD+ bigger;   // (1.5.1) nothing under 1SD (1SD+ bold, 3SD+ outlined), circle from 1SD; non-bold only while learning;   // (1.5.0) prints on the big-lot SD sizes: 1SD faded, 2SD bold, 3SD bold + outline, nothing under 1SD;   // (1.4.1) a circled number centred in its ring; "absorption print" naming; candles as body + wick (82 of 112 numbers had no room); 1 px gap
static const int LOOKBACK = 20;        // prior candles in the median
static const int MIN_PRIOR = 10;       // fewer prior candles with data = no ratio (never bold)
// (1.18.0) THE print rule: v beats EVERY one of the n candles before it (same side); fewer than n known = no print
static const int TOP_N = 10;
inline bool biggestOfLast(const std::vector<long long>& prev, long long v, int n = TOP_N) {
    if (v <= 0 || n <= 0 || (int)prev.size() < n) return false;
    for (int k = 0; k < n; ++k) if (v <= prev[(size_t)k]) return false;
    return true;
}

// ---- settings (all real: read on every draw from the per-market file, written only from the settings window) ----------------
// ---- (1.11.0, Rassul 2026-10-17 15:06 mockup v2 approved 15:10) the settings ------------------------------------------------------
static const int NKEY = 16, NGAM = 10, NVOL = 6;
static const char* const KEY_NAMES[NKEY] = { "ONH", "ONL", "PDH", "PDL", "PFH", "PFL", "LonHI", "LonLO", "PWH", "PWL", "HOD", "LOD", "AHI", "ALO", "CWH", "CWL" };
static const char* const GAM_NAMES[NGAM] = { "CR", "PS", "HVL", "CR 0DTE", "PS 0DTE", "G1", "G2", "G3", "G4", "G5" };
static const char* const VOL_NAMES[NVOL] = { "PD POC", "PD VAH", "PD VAL", "Wk POC", "Wk VAH", "Wk VAL" };
// (1.14.0, Rassul 16:41 "near pivot should be exclusive ... near level is also mutually exclusive ... a third option would be pivot near
// level. Trend PB and Trend BO would also be mutually exclusive of everything else"; 16:00 "you can select one or the other or both")
static const char* const SHOW_NAMES = "All prints;Near pivot;Near a level;Pivot near a level;Trend PB;Trend BO;Trend PB + BO";
enum { SH_ALL = 0, SH_PIVOT = 1, SH_LEVEL = 2, SH_PIVLEVEL = 3, SH_PB = 4, SH_BO = 5, SH_PBBO = 6 };
static const char* const SESSION_NAMES = "RTH + overnight;RTH only";
static const char* const POS_NAMES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";
struct Cat {   // one Big Lot category (1 - 3: Big Lot N + an optional minimum; 4: a fixed size, >= lots)
    bool on = false;
    int n = 5;                // Big Lot N (categories 1 - 3): the size reached ~N times a session; 1 - 100
    int lots = 0;             // categories 1 - 3: the optional minimum (0 = none); category 4: the size, >= lots (1 - 100,000)
    int font = 8;             // 6 - 20 pt
    int show = SH_ALL;        // (1.14.0) the one filter of categories 1 - 3 (SHOW_NAMES); category 4 has none (time filter only)
    unsigned bid = 0x00FFFFFF, ask = 0x00FFFFFF;   // the NUMBER colour at the bid / ask (IRT COLOR = 0x00RRGGBB)
};
struct Settings {
    int div = 0;              // divider: 0 = the market's default, else 1 - 1000
    bool box = true, dim = false, circle = false;
    unsigned circleCol = 0x00FFFFFF;
    unsigned boxBid = 0x00EF4444, boxAsk = 0x0022C55E;   // (1.14.0) the box colour by side (red = sold at the bid, green = bought at the ask)
    Cat cat[4];
    int skipFirst = 15, skipLast = 15;    // minutes (0 - 240) after each open / before each close
    int atrUpQ = 200, atrDnQ = 100;       // the level zone in hundredths of a 3-min ATR above / below the level (0 - 2000)
    bool keyOn = false, gamOn = false, volOn = false;   // (1.14.0, Rassul 16:35 "do not select any levels")
    unsigned keyMask = 0, gamMask = 0, volMask = 0;
    bool statsChart = true; int statsPos = 9;   // the statistics box on the chart: 1 - 9 (top left .. bottom right)
    bool rthOnly = false;     // (1.13.0, Rassul 15:29) RTH only, or RTH + overnight
    int boldQ = 200;          // internal (hover text only)
    // (1.14.0, Rassul 16:35 / 16:46 defaults) categories 1 - 4 on; Big Lot 5 and category 4 cyan, Big Lot 10 / 20 white; 2 and 3 Trend PB + BO
    Settings() { for (int k = 0; k < 4; ++k) cat[k].on = true;
        cat[0].n = 5; cat[1].n = 10; cat[2].n = 20; cat[3].n = 0; cat[3].lots = 1350;
        cat[0].bid = cat[0].ask = cat[3].bid = cat[3].ask = 0x0000FFFF;
        cat[1].show = cat[2].show = SH_PBBO; }
};
// (1.14.0, Rassul 16:47 "figure out what Cat 4 should be for all the markets") the category 4 size per market: about 3 prints a
// regular session (8:45 AM - 2:45 PM, his 15-min time filter) at or above it - the exceptional print, rarer than Big Lot 5 so it does
// not stand in for the filtered categories. From his recorded prints (ES Oct 13-15) and the Oct 13 footprint (NQ / CL / GC / NG / 6E);
// HG scaled from GC by the Big Lot S5 ratio (47 / 56). Lots at one price on one side in a 3-min candle.
inline int cat4For(const std::string& m) {
    return m == "ES" ? 1000 : m == "NQ" ? 450 : m == "CL" ? 95 : m == "GC" ? 105 : m == "HG" ? 90 : m == "NG" ? 295
         : (m == "6E" || m == "EU") ? 490 : 1000;
}
inline Settings defaultsFor(const std::string& m) { Settings s; s.cat[3].lots = cat4For(m); return s; }
// (1.17.0, Rassul approved 19:57 CT "go") THE fixed behaviour - no settings window, no settings file. Exactly what his ES chart showed
// (BigBidAsk.settings-ES.txt 17:15): category 1 Big Lot 5 all prints cyan; 2 Big Lot 10 / 3 Big Lot 20 Trend PB + BO white; font 8;
// boxes on (bid EF4444 / ask 22C55E); no circle / dim / level filters; skip first / last 15 min; divider = the market default
// (div 0); the Big Lot box on (position 9 without a HOD/LOD table). Category 4 is REMOVED (off, size 0: never drawn).
inline Settings fixedSettings(const std::string& m) {
    Settings s = defaultsFor(m);
    s.div = 0; s.box = true; s.dim = false; s.circle = false; s.boxBid = 0x00EF4444; s.boxAsk = 0x0022C55E;
    const int n[3] = { 5, 10, 20 }, sh[3] = { SH_ALL, SH_PBBO, SH_PBBO }; const unsigned col[3] = { 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF };   // (1.18.0) one rule, one colour (was cyan for Big Lot 5)
    for (int k = 0; k < 3; ++k) { s.cat[k].on = true; s.cat[k].n = n[k]; s.cat[k].lots = 0; s.cat[k].font = 7;   // (1.18.0) 7, was 8
                                  s.cat[k].show = sh[k]; s.cat[k].bid = s.cat[k].ask = col[k]; }
    s.cat[3].on = false; s.cat[3].lots = 0; s.cat[3].n = 0; s.cat[3].font = 8;
    s.skipFirst = 15; s.skipLast = 15; s.rthOnly = false;
    s.keyOn = s.gamOn = s.volOn = false; s.keyMask = s.gamMask = s.volMask = 0;
    s.statsChart = true; s.statsPos = 9;
    return s;
}
// (1.4.0, Rassul 11:51 "a divider ... so the value always looks like a 1 or 2 digit value; for ES 10 by default") per market, from the
// typical biggest lots at one price in a 3-min candle (ES ~300-1,500, CL / NG / 6E ~100-500, NQ / GC / HG ~20-150)
inline int divFor(const std::string& m) {
    if (m == "ES" || m == "CL") return 10;      // (1.12.0, Rassul 15:31 "change ng and 6e divider to 1": their big prints are ~70 - 160 lots)
    return 1;                                   // NQ, GC, HG, NG, 6E: already 2-3 digits
}
inline bool divOk(long long v) { return v >= 1 && v <= 1000; }
inline bool fontOk(long long v) { return v >= 6 && v <= 20; }
inline bool boldOk(long long q) { return q >= 120 && q <= 1000; }
inline int toQ(double x) { if (!std::isfinite(x)) return -1; const double q = std::floor(x * 100.0 + 0.5); return q < 0 || q > 1e9 ? -1 : (int)q; }
inline bool nOk(long long v) { return v >= 1 && v <= 100; }
inline bool lotsOk(long long v) { return v >= 0 && v <= 100000; }
inline bool colOk(long long v) { return v >= 0 && v <= 0xFFFFFF; }
inline bool minOk(long long v) { return v >= 0 && v <= 240; }
inline bool atrOk(long long q) { return q >= 0 && q <= 2000; }
inline bool posOk(long long v) { return v >= 1 && v <= 9; }
inline bool setupOk(long long v) { return v >= 0 && v <= 3; }
inline bool showOk(long long v) { return v >= 0 && v <= 6; }
// strict whole number (digits only, no sign, no blanks)
inline bool fullInt(const std::string& t, long long& out) {
    if (t.empty() || t.size() > 12) return false;
    for (size_t i = 0; i < t.size(); ++i) if (t[i] < '0' || t[i] > '9') return false;
    char* end = nullptr; errno = 0; const long long v = std::strtoll(t.c_str(), &end, 10);
    if (errno != 0 || !end || *end != '\0') return false;
    out = v; return true;
}
// (1.14.0) one line, 49 fields: "BBA7|div|box|dim|circle|circleCol|boxBid|boxAsk" + 4 x "|on|N|lots|font|show|bid|ask"
//   + "|skipFirst|skipLast|atrUpQ|atrDnQ|keyOn|gamOn|volOn|keyMask|gamMask|volMask|statsChart|statsPos|rthOnly"
static const size_t NFIELDS = 1 + 7 + 4 * 7 + 13;
inline std::string formatSettings(const Settings& s) {
    auto b = [](bool x) { return std::string(x ? "1" : "0"); };
    auto n = [](long long x) { return std::to_string(x); };
    std::string o = "BBA7|" + n(s.div) + "|" + b(s.box) + "|" + b(s.dim) + "|" + b(s.circle) + "|" + n(s.circleCol & 0x00FFFFFF) + "|" + n(s.boxBid & 0x00FFFFFF) + "|" + n(s.boxAsk & 0x00FFFFFF);
    for (int k = 0; k < 4; ++k) { const Cat& c = s.cat[k];
        o += "|" + b(c.on) + "|" + n(c.n) + "|" + n(c.lots) + "|" + n(c.font) + "|" + n(c.show) + "|" + n(c.bid & 0x00FFFFFF) + "|" + n(c.ask & 0x00FFFFFF); }
    o += "|" + n(s.skipFirst) + "|" + n(s.skipLast) + "|" + n(s.atrUpQ) + "|" + n(s.atrDnQ) + "|" + b(s.keyOn) + "|" + b(s.gamOn) + "|" + b(s.volOn)
       + "|" + n(s.keyMask) + "|" + n(s.gamMask) + "|" + n(s.volMask) + "|" + b(s.statsChart) + "|" + n(s.statsPos) + "|" + b(s.rthOnly);
    return o;
}
// every field checked on its own: a bad field falls back to ITS default (why names it); a wrong layout = all defaults;
// files from before 1.11.0 (BBA1 - BBA4) = the defaults; BBA5 / BBA6 are carried over (1.14.0: their pivot / level / setup -> Show)
inline Settings parseSettings(const std::string& text, std::string& why, const std::string& mkt = "") {
    Settings s = defaultsFor(mkt), d = defaultsFor(mkt); why.clear();
    std::string line = text;
    const size_t nl = line.find('\n'); if (nl != std::string::npos) line = line.substr(0, nl);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::vector<std::string> f; { std::string cur; for (char c : line) { if (c == '|') { f.push_back(cur); cur.clear(); } else cur.push_back(c); } f.push_back(cur); }
    if (!f.empty() && f[0].size() == 4 && f[0].compare(0, 3, "BBA") == 0 && f[0][3] >= '1' && f[0][3] <= '4') { why = "older file (defaults)"; return d; }
    if (f.size() == 50 && f[0] == "BBA5") {   // (1.13.0) a 1.11 / 1.12 file: kept, with setup Any and RTH + overnight added
        std::vector<std::string> g; g.push_back("BBA6"); for (int i = 1; i <= 5; ++i) g.push_back(f[i]);
        for (int k = 0; k < 4; ++k) { for (int j = 0; j < 8; ++j) g.push_back(f[6 + 8 * k + j]); g.push_back("0"); }
        for (int i = 38; i < 50; ++i) { g.push_back(f[i]); }
        g.push_back("0");
        f.swap(g);
    }
    if (f.size() == 55 && f[0] == "BBA6") {   // (1.14.0) a 1.13 file: pivot / level / setup -> one Show choice; box colours default
        std::vector<std::string> g; g.push_back("BBA7"); for (int i = 1; i <= 5; ++i) g.push_back(f[i]);
        g.push_back(std::to_string(d.boxBid)); g.push_back(std::to_string(d.boxAsk));
        for (int k = 0; k < 4; ++k) { const size_t b0 = 6 + 9 * k;
            const bool pv = f[b0 + 4] == "1", lv = f[b0 + 5] == "1"; const std::string& su = f[b0 + 8];
            int sh = su == "1" ? SH_PB : su == "3" ? SH_BO : su == "2" ? (pv ? SH_PIVLEVEL : SH_LEVEL) : pv && lv ? SH_PIVLEVEL : lv ? SH_LEVEL : pv ? SH_PIVOT : SH_ALL;
            if (k == 3) sh = SH_ALL;
            for (size_t j = 0; j < 4; ++j) g.push_back(f[b0 + j]);
            g.push_back(std::to_string(sh)); g.push_back(f[b0 + 6]); g.push_back(f[b0 + 7]); }
        for (size_t i = 42; i < 55; ++i) g.push_back(f[i]);
        f.swap(g); why = "carried over from 1.13";
    }
    if (f.size() != NFIELDS || f[0] != "BBA7") { why = "garbled (defaults)"; return d; }
    const std::string carried = why; why.clear();
    std::vector<std::string> bad;
    long long v = 0; size_t i = 1;
    auto flag = [&](bool& out, bool def, const std::string& nm) { if (fullInt(f[i], v) && v <= 1) out = v == 1; else { out = def; bad.push_back(nm); } ++i; };
    auto num = [&](int& out, int def, bool (*ok)(long long), const std::string& nm) { if (fullInt(f[i], v) && ok(v)) out = (int)v; else { out = def; bad.push_back(nm); } ++i; };
    auto col = [&](unsigned& out, unsigned def, const std::string& nm) { if (fullInt(f[i], v) && colOk(v)) out = (unsigned)v; else { out = def; bad.push_back(nm); } ++i; };
    auto mask = [&](unsigned& out, unsigned def, int bits, const std::string& nm) { if (fullInt(f[i], v) && v >= 0 && v < (1LL << bits)) out = (unsigned)v; else { out = def; bad.push_back(nm); } ++i; };
    if (fullInt(f[i], v) && (v == 0 || divOk(v))) s.div = (int)v; else { s.div = d.div; bad.push_back("divider"); } ++i;
    flag(s.box, d.box, "box"); flag(s.dim, d.dim, "dim"); flag(s.circle, d.circle, "circle"); col(s.circleCol, d.circleCol, "circle colour");
    col(s.boxBid, d.boxBid, "box bid colour"); col(s.boxAsk, d.boxAsk, "box ask colour");
    for (int k = 0; k < 4; ++k) { Cat& c = s.cat[k]; const Cat& dc = d.cat[k]; const std::string nm = "category " + std::to_string(k + 1);
        flag(c.on, dc.on, nm + " on");
        if (k < 3) num(c.n, dc.n, nOk, nm + " Big Lot"); else { c.n = 0; ++i; }
        if (k < 3) num(c.lots, dc.lots, lotsOk, nm + " min lots");
        else { if (fullInt(f[i], v) && v >= 1 && v <= 100000) c.lots = (int)v; else { c.lots = dc.lots; bad.push_back(nm + " lots"); } ++i; }
        num(c.font, dc.font, fontOk, nm + " font");
        num(c.show, dc.show, showOk, nm + " show"); if (k == 3) c.show = SH_ALL;
        col(c.bid, dc.bid, nm + " bid colour"); col(c.ask, dc.ask, nm + " ask colour"); }
    num(s.skipFirst, d.skipFirst, minOk, "skip first"); num(s.skipLast, d.skipLast, minOk, "skip last");
    num(s.atrUpQ, d.atrUpQ, atrOk, "ATR above"); num(s.atrDnQ, d.atrDnQ, atrOk, "ATR below");
    flag(s.keyOn, d.keyOn, "key levels"); flag(s.gamOn, d.gamOn, "gamma levels"); flag(s.volOn, d.volOn, "volume levels");
    mask(s.keyMask, d.keyMask, NKEY, "key level boxes"); mask(s.gamMask, d.gamMask, NGAM, "gamma level boxes"); mask(s.volMask, d.volMask, NVOL, "volume level boxes");
    flag(s.statsChart, d.statsChart, "statistics on the chart"); num(s.statsPos, d.statsPos, posOk, "statistics position");
    flag(s.rthOnly, d.rthOnly, "RTH only");
    if (!bad.empty()) { why = "bad value (default used):"; for (const auto& x : bad) why += " " + x; }
    if (!carried.empty()) why = why.empty() ? carried : carried + "; " + why;
    return s;
}
// a typed decimal (the dialog's float box) -> hundredths, or -1 when it is not a sane number
inline int toHundredths(double x) { if (!std::isfinite(x) || x < 0 || x > 20.0001) return -1; return (int)std::floor(x * 100.0 + 0.5); }

// (1.13.0, Rassul 15:29 / 15:35) BREAKOUT through the higher of the last two HIGH pivots (his 1-bar ZigZag, pivots confirmed by the bars
// BEFORE the candle): P = the prior high pivot, or the one 2 back when it is higher; the candle breaks out when its LOW is below P and
// its HIGH above P (it is the candle that went through). up[i] = 1 (the prior pivot) / 2 (the higher one 2 back) / 0 none.
// BREAKDOWN mirror with the lower of the last two LOW pivots: dn[i]. Decided on the candle's own bars - never repainted.
// (1.21.0) the same walk, with the pivot price broken (upPx / dnPx; 0 = none) for the BREAKOUT / BREAKDOWN signal; breakFlags below is
// this walk without the prices (unchanged rule).
template <class H, class L>
inline void breakFlagsPx(H& high, L& low, int n, std::vector<signed char>& up, std::vector<signed char>& dn, std::vector<double>* upPx, std::vector<double>* dnPx) {
    up.assign(n > 0 ? n : 0, 0); dn.assign(n > 0 ? n : 0, 0);
    if (upPx) upPx->assign(n > 0 ? n : 0, 0.0);
    if (dnPx) dnPx->assign(n > 0 ? n : 0, 0.0);
    if (n < 2) return;
    auto hi = [&](int i) { return (double)high[i]; }; auto lo = [&](int i) { return (double)low[i]; };
    std::vector<double> tops, bots;
    int dirn = hi(1) >= hi(0) ? 1 : -1, ext = 0;
    for (int i = 1; i < n; ++i) {
        if (!std::isfinite(hi(i)) || !std::isfinite(lo(i)) || hi(i) < lo(i)) continue;
        if (!tops.empty()) { double P = tops.back(); signed char w = 1; if (tops.size() >= 2 && tops[tops.size() - 2] > P) { P = tops[tops.size() - 2]; w = 2; }
            if (lo(i) < P && hi(i) > P) { up[i] = w; if (upPx) (*upPx)[i] = P; } }
        if (!bots.empty()) { double Q = bots.back(); signed char w = 1; if (bots.size() >= 2 && bots[bots.size() - 2] < Q) { Q = bots[bots.size() - 2]; w = 2; }
            if (lo(i) < Q && hi(i) > Q) { dn[i] = w; if (dnPx) (*dnPx)[i] = Q; } }
        if (dirn == 1) {
            if (hi(i) > hi(ext)) ext = i;
            else if (lo(i) < lo(i - 1) && i != ext) { tops.push_back(hi(ext)); ext = i; dirn = -1; }
        } else {
            if (lo(i) < lo(ext)) ext = i;
            else if (hi(i) > hi(i - 1) && i != ext) { bots.push_back(lo(ext)); ext = i; dirn = 1; }
        }
    }
}
// (1.26.0, Rassul 18:01 CT 2026-10-22 "the bo ... using the pivot rule") the BO pivot = an INTACT SWING: his 1-bar ZigZag (the same walk,
// pivots confirmed by the bars BEFORE the candle); a swing high stays intact until a later candle's HIGH goes above it (a swing low:
// a LOW below it). Only the LAST 3 intact swings of each side count (the Sr v4 / SwingRef rule). The candle breaks out when its LOW is
// below and its HIGH above one of them (it went through it); when it goes through more than one, the highest high (lowest low) is the
// pivot. up[i] / dn[i] = 1 .. 3 = which of the 3 (1 = the most recent), 0 none; upPx / dnPx = its price. Every swing the candle's high
// (low) passed is no longer intact afterwards. A pure function of the bars up to the candle - never repainted.
static const int SWING_KEEP = 3;
template <class H, class L>
inline void swingBreaksPx(H& high, L& low, int n, std::vector<signed char>& up, std::vector<signed char>& dn, std::vector<double>& upPx, std::vector<double>& dnPx) {
    up.assign(n > 0 ? n : 0, 0); dn.assign(n > 0 ? n : 0, 0); upPx.assign(n > 0 ? n : 0, 0.0); dnPx.assign(n > 0 ? n : 0, 0.0);
    if (n < 2) return;
    auto hi = [&](int i) { return (double)high[i]; }; auto lo = [&](int i) { return (double)low[i]; };
    std::vector<double> tops, bots;                                    // intact swings, oldest first
    int dirn = hi(1) >= hi(0) ? 1 : -1, ext = 0;
    for (int i = 1; i < n; ++i) {
        if (!std::isfinite(hi(i)) || !std::isfinite(lo(i)) || hi(i) < lo(i)) continue;
        {   const int a = std::max(0, (int)tops.size() - SWING_KEEP); double best = 0; int w = 0;
            for (int k = (int)tops.size() - 1; k >= a; --k) { const double P = tops[(size_t)k]; if (lo(i) < P && hi(i) > P && (w == 0 || P > best)) { best = P; w = (int)tops.size() - k; } }
            if (w) { up[(size_t)i] = (signed char)w; upPx[(size_t)i] = best; }
            for (size_t k = 0; k < tops.size();) { if (hi(i) > tops[k]) tops.erase(tops.begin() + (long)k); else ++k; } }
        {   const int a = std::max(0, (int)bots.size() - SWING_KEEP); double best = 0; int w = 0;
            for (int k = (int)bots.size() - 1; k >= a; --k) { const double Q = bots[(size_t)k]; if (lo(i) < Q && hi(i) > Q && (w == 0 || Q < best)) { best = Q; w = (int)bots.size() - k; } }
            if (w) { dn[(size_t)i] = (signed char)w; dnPx[(size_t)i] = best; }
            for (size_t k = 0; k < bots.size();) { if (lo(i) < bots[k]) bots.erase(bots.begin() + (long)k); else ++k; } }
        if (dirn == 1) {
            if (hi(i) > hi(ext)) ext = i;
            else if (lo(i) < lo(i - 1) && i != ext) { tops.push_back(hi(ext)); ext = i; dirn = -1; }
        } else {
            if (lo(i) < lo(ext)) ext = i;
            else if (hi(i) > hi(i - 1) && i != ext) { bots.push_back(lo(ext)); ext = i; dirn = 1; }
        }
        if (tops.size() > 64) tops.erase(tops.begin());
        if (bots.size() > 64) bots.erase(bots.begin());
    }
}
template <class H, class L>
inline void breakFlags(H& high, L& low, int n, std::vector<signed char>& up, std::vector<signed char>& dn) {
    breakFlagsPx(high, low, n, up, dn, (std::vector<double>*)nullptr, (std::vector<double>*)nullptr);
}

// ---- (1.11.0) near a level --------------------------------------------------------------------------------------------------------
// A print at price p on candle i is NEAR level L when p is within [L - dn x ATR, L + up x ATR] (ATR = the average range of the 14
// closed 3-min candles before i), OR the print is part of a SWEEP of L: the candle or one of the 2 before it traded through L, the
// print sits between L and that sweep's extreme, and price has not CLOSED beyond L for more than 2 candles in a row (a sweep comes
// back; a breakout that holds stops counting). Only data up to candle i is used (no repaint).
struct NamedLevel { std::string name; double px = 0; };
template <class H, class L, class C>
inline double atr14(H& hi, L& lo, C& cl, int i) {
    double s = 0; int n = 0;
    for (int k = i - 1; k >= 1 && n < 14; --k) { const double tr = std::max((double)hi[k], (double)cl[k - 1]) - std::min((double)lo[k], (double)cl[k - 1]); if (std::isfinite(tr) && tr > 0) { s += tr; ++n; } }
    return n >= 5 ? s / n : 0.0;
}
template <class H, class L, class C>
inline bool sweptHere(H& hi, L& lo, C& cl, int i, double lv, double p) {
    for (int k = i; k >= 0 && k >= i - 2; --k) {
        const double h = hi[k], l = lo[k];
        if (h > lv && p >= lv && p <= h) {                        // swept above: count the closes above in a row, ending at i
            int run = 0; for (int q = i; q >= 0 && (double)cl[q] > lv; --q) ++run;
            if (run <= 2) return true;
        }
        if (l < lv && p <= lv && p >= l) {                        // swept below
            int run = 0; for (int q = i; q >= 0 && (double)cl[q] < lv; --q) ++run;
            if (run <= 2) return true;
        }
    }
    return false;
}
template <class H, class L, class C>
inline const NamedLevel* nearLevel(H& hi, L& lo, C& cl, int i, double p, const std::vector<NamedLevel>& lv, double atr, int upQ, int dnQ) {
    const NamedLevel* best = nullptr; double bd = 1e300;
    for (const NamedLevel& x : lv) {
        if (!std::isfinite(x.px)) continue;
        const double up = atr * upQ / 100.0, dn = atr * dnQ / 100.0;
        const bool in = (atr > 0 && p >= x.px - dn && p <= x.px + up) || sweptHere(hi, lo, cl, i, x.px, p);
        if (in && std::fabs(p - x.px) < bd) { bd = std::fabs(p - x.px); best = &x; }
    }
    return best;
}
// ---- (1.11.0) the gamma levels from MenthorQLevels.csv (SYMBOL,PRICE,LABEL,...): this market, the chart's contract month only;
// labels split on '/', "CR 0D" -> "CR 0DTE", "PS 0D" -> "PS 0DTE", "HVL" / "HVL 0D" -> "HVL", "G1" - "G5"; GW and others ignored
inline std::string mqMarket(const std::string& sym) {
    auto st = [&](const char* p) { return sym.compare(0, std::strlen(p), p) == 0; };
    static const char* const P[7][2] = { {"ENQ", "NQ"}, {"EP", "ES"}, {"CLE", "CL"}, {"GCE", "GC"}, {"CPE", "HG"}, {"NGE", "NG"}, {"EU6", "EU"} };
    for (int k = 0; k < 7; ++k) { if (st(P[k][0])) return P[k][1]; }
    return "";
}
inline bool monthOf(const std::string& s, char& m, char& y) {
    const size_t n = s.size(); auto isM = [](char c) { return c && std::strchr("FGHJKMNQUVXZ", c) != NULL; };
    if (n >= 3 && isM(s[n - 2]) && std::isdigit((unsigned char)s[n - 1])) { m = s[n - 2]; y = s[n - 1]; return true; }
    if (n >= 4 && isM(s[n - 3]) && std::isdigit((unsigned char)s[n - 2]) && std::isdigit((unsigned char)s[n - 1])) { m = s[n - 3]; y = s[n - 1]; return true; }
    return false;
}
inline std::vector<NamedLevel> parseGamma(const std::string& text, const std::string& mkt, const std::string& chartSym, double tick) {
    std::vector<NamedLevel> out; if (mkt.empty() || !(tick > 0) || text.size() > 64 * 1024) return out;
    char cm = 0, cy = 0; const bool haveM = monthOf(chartSym, cm, cy);
    size_t a = 0;
    while (a < text.size() && out.size() < 40) {
        size_t b = text.find('\n', a); if (b == std::string::npos) b = text.size();
        std::string ln = text.substr(a, b - a); a = b + 1;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln.empty() || ln.size() > 512 || ln.compare(0, 6, "SYMBOL") == 0) continue;
        std::vector<std::string> c; size_t p = 0;
        while (c.size() < 3) { const size_t q = ln.find(',', p); c.push_back(ln.substr(p, q == std::string::npos ? std::string::npos : q - p)); if (q == std::string::npos) break; p = q + 1; }
        if (c.size() < 3) continue;
        std::string sym; for (char ch : c[0]) sym += (char)std::toupper((unsigned char)ch);
        if (mqMarket(sym) != mkt) continue;
        char fm = 0, fy = 0; if (haveM && monthOf(sym, fm, fy) && (fm != cm || fy != cy)) continue;   // another contract month: not used
        char* e = nullptr; const double px = std::strtod(c[1].c_str(), &e); if (e == c[1].c_str() || !(px > 0) || !std::isfinite(px)) continue;
        const double pr = std::round(px / tick) * tick;
        size_t s0 = 0; const std::string& lab = c[2];
        while (s0 <= lab.size()) {
            const size_t s1 = lab.find('/', s0); std::string tok = lab.substr(s0, s1 == std::string::npos ? std::string::npos : s1 - s0);
            const size_t i0 = tok.find_first_not_of(' '); tok = i0 == std::string::npos ? "" : tok.substr(i0);
            while (!tok.empty() && tok.back() == ' ') tok.pop_back();
            std::string w = tok, rest; const size_t sp = tok.find(' '); if (sp != std::string::npos) { w = tok.substr(0, sp); rest = tok.substr(sp + 1); }
            const bool zero = rest == "0D" || rest == "0DTE";
            std::string nm;
            if (w == "CR") nm = zero ? "CR 0DTE" : "CR"; else if (w == "PS") nm = zero ? "PS 0DTE" : "PS"; else if (w == "HVL") nm = "HVL";
            else if (w.size() == 2 && w[0] == 'G' && w[1] >= '1' && w[1] <= '5') nm = w;
            if (!nm.empty()) out.push_back(NamedLevel{ nm, pr });
            if (s1 == std::string::npos) break;
            s0 = s1 + 1;
        }
    }
    return out;
}
// ---- (1.11.0) the volume levels: POC and the 70% value area of a profile (price key -> volume) ---------------------------------
struct VA { bool ok = false; long long poc = 0, vah = 0, val = 0; };
inline VA valueArea(const std::map<long long, long long>& prof) {
    VA r; if (prof.empty()) return r;
    long long tot = 0, best = -1; for (const auto& kv : prof) { tot += kv.second; if (kv.second > best) { best = kv.second; r.poc = kv.first; } }
    if (tot <= 0) return r;
    std::vector<std::pair<long long, long long>> v(prof.begin(), prof.end());
    size_t pi = 0; while (pi < v.size() && v[pi].first != r.poc) ++pi;
    size_t lo = pi, hi = pi; long long in = v[pi].second; const long long need = (long long)std::ceil(tot * 0.70);
    while (in < need && (lo > 0 || hi + 1 < v.size())) {
        const long long up = hi + 1 < v.size() ? v[hi + 1].second : -1, dn = lo > 0 ? v[lo - 1].second : -1;
        if (up >= dn) { ++hi; in += v[hi].second; } else { --lo; in += v[lo].second; }
    }
    r.vah = v[hi].first; r.val = v[lo].first; r.ok = true; return r;
}
// ---- (1.11.0) the statistics: one session's Big Lot N = its N-th biggest day print; the average of up to 5 sessions drops ONE
// outlier (a session more than 1.5x, or under 2/3 of, the median of the others) -------------------------------------------------
inline long long nth(std::vector<long long> v, int n) { if (n < 1 || (int)v.size() < n) return 0; std::nth_element(v.begin(), v.begin() + (n - 1), v.end(), std::greater<long long>()); return v[n - 1]; }
struct Avg { long long avg = 0; int used = 0; int dropped = -1; double ratio = 0; };
inline double medianOf(std::vector<double> v) { if (v.empty()) return 0; std::sort(v.begin(), v.end()); const size_t m = v.size() / 2; return v.size() % 2 ? v[m] : (v[m - 1] + v[m]) / 2; }
inline Avg avgDropOutlier(const std::vector<long long>& x) {
    Avg a; std::vector<int> ok; for (int i = 0; i < (int)x.size(); ++i) if (x[i] > 0) ok.push_back(i);
    if (ok.empty()) return a;
    int drop = -1; double worst = 1.0;
    if (ok.size() >= 4) for (int i : ok) {
        std::vector<double> others; for (int j : ok) if (j != i) others.push_back((double)x[j]);
        const double m = medianOf(others); if (m <= 0) continue;
        const double r = x[i] / m, dev = r >= 1 ? r : 1.0 / r;
        if ((r > 1.5 || r < 2.0 / 3.0) && dev > worst) { worst = dev; drop = i; a.ratio = r; }
    }
    double s = 0; int n = 0; for (int i : ok) if (i != drop) { s += (double)x[i]; ++n; }
    a.avg = n ? (long long)std::llround(s / n) : 0; a.used = n; a.dropped = drop; return a;
}
// (1.10.0, Rassul 14:22 "only see near pivot ... like delta profiles pivot requirement") his 1-bar ZigZag over bars 0 .. i in ONE pass
// (the same walk as DeltaProfileCore pivotOk, decided on the bars up to the candle - later bars never change it):
// bull[i] = bar i makes the down pivot or its LOW is below the HIGH of the latest down-pivot bar (for bid / seller prints);
// bear[i] = bar i makes the up pivot or its HIGH is above the LOW of the latest up-pivot bar (for ask / buyer prints)
template <class H, class L>
inline void pivotFlags(H& high, L& low, int n, std::vector<char>& bull, std::vector<char>& bear) {
    bull.assign(n > 0 ? n : 0, 0); bear.assign(n > 0 ? n : 0, 0);
    if (n < 2) return;
    auto hi = [&](int i) { return (double)high[i]; }; auto lo = [&](int i) { return (double)low[i]; };
    int dirn = hi(1) >= hi(0) ? 1 : -1, ext = 0, start = 0, bad = -1;
    for (int i = 1; i < n; ++i) {
        if (!std::isfinite(hi(i)) || !std::isfinite(lo(i)) || hi(i) < lo(i)) { bad = i; continue; }
        if (dirn == 1) {
            if (hi(i) > hi(ext)) ext = i;
            else if (lo(i) < lo(i - 1) && i != ext) { start = ext; ext = i; dirn = -1; }
        } else {
            if (lo(i) < lo(ext)) ext = i;
            else if (hi(i) > hi(i - 1) && i != ext) { start = ext; ext = i; dirn = 1; }
        }
        if (bad >= 0 && i - bad < 50) continue;   // a broken bar recently: no claim
        const int dp = dirn == -1 ? ext : start, up = dirn == 1 ? ext : start;
        bull[i] = (dp == i || lo(i) < hi(dp)) ? 1 : 0;
        bear[i] = (up == i || hi(i) > lo(up)) ? 1 : 0;
    }
}

// ---- (1.14.0, Rassul 16:0x "it is ok to repaint when the pivot requirement is there"; "the bar must be the dnpivot or the bars low is
// below the high of the down pivot bar, and vice versa") the pivot of the candle's OWN swing: his 1-bar ZigZag over ALL the bars;
// the candle belongs to the leg (A, B] between two turns. Its down pivot = the low end of that leg, its up pivot = the high end. The
// last leg ends at the extreme so far (not confirmed yet): a candle there may be shown, hidden and shown again until its turn is
// confirmed - conf[i] says it is final. bull[i] = i is its down pivot or its LOW is below that pivot bar's HIGH; bear[i] = i is its up
// pivot or its HIGH is above that pivot bar's LOW.
struct Swing {
    std::vector<int> a, b;            // the leg of bar i: (a, b]; b = the turn it runs into (-1 = none yet)
    std::vector<int> dp, up;          // the down / up pivot bar of bar i's swing (-1 = none)
    std::vector<char> bull, bear, conf;
    int lastConfirmed = -1;           // the last confirmed turn (bars up to it are final)
};
template <class H, class L>
inline Swing swing(H& high, L& low, int n) {
    Swing s; const size_t N = n > 0 ? (size_t)n : 0;
    s.a.assign(N, -1); s.b.assign(N, -1); s.dp.assign(N, -1); s.up.assign(N, -1); s.bull.assign(N, 0); s.bear.assign(N, 0); s.conf.assign(N, 0);
    if (n < 3) return s;
    auto hi = [&](int i) { return (double)high[i]; }; auto lo = [&](int i) { return (double)low[i]; };
    auto good = [&](int i) { return std::isfinite(hi(i)) && std::isfinite(lo(i)) && hi(i) >= lo(i); };
    std::vector<int> piv; std::vector<signed char> typ;              // the turns in order: +1 a top (up pivot), -1 a bottom (down pivot)
    int dirn = hi(1) >= hi(0) ? 1 : -1, ext = 0, bad = -1;
    for (int i = 1; i < n; ++i) {
        if (!good(i)) { bad = i; continue; }
        if (dirn == 1) {
            if (hi(i) > hi(ext)) ext = i;
            else if (lo(i) < lo(i - 1) && i != ext) { piv.push_back(ext); typ.push_back(1); ext = i; dirn = -1; }
        } else {
            if (lo(i) < lo(ext)) ext = i;
            else if (hi(i) > hi(i - 1) && i != ext) { piv.push_back(ext); typ.push_back(-1); ext = i; dirn = 1; }
        }
    }
    const size_t nConf = piv.size();
    if (nConf) s.lastConfirmed = piv.back();
    piv.push_back(ext); typ.push_back(dirn == 1 ? 1 : -1);          // the leg still forming ends at the extreme so far
    for (size_t k = 0; k < piv.size(); ++k) {
        const int A = k ? piv[k - 1] : -1, B = piv[k];
        const int D = typ[k] < 0 ? B : (k ? piv[k - 1] : -1), U = typ[k] > 0 ? B : (k ? piv[k - 1] : -1);
        for (int i = A + 1; i <= B && i < n; ++i) {
            s.a[i] = A; s.b[i] = B; s.dp[i] = D; s.up[i] = U; s.conf[i] = k < nConf ? 1 : 0;
            if (!good(i) || (bad >= 0 && i > bad && i - bad < 50)) continue;   // a broken bar recently: no claim
            s.bull[i] = D >= 0 && (D == i || lo(i) < hi(D)) ? 1 : 0;
            s.bear[i] = U >= 0 && (U == i || hi(i) > lo(U)) ? 1 : 0;
        }
    }
    // the bars after the extreme so far: tentatively a leg the other way, to the most extreme bar among them, then one more back
    const int E = piv.back();
    if (E + 1 < n) {
        const bool down = typ.back() > 0;                             // after a top: falling to the lowest low since
        int T = E + 1; for (int i = E + 2; i < n; ++i) if (good(i) && (down ? lo(i) < lo(T) : hi(i) > hi(T))) T = i;
        for (int i = E + 1; i < n; ++i) {
            const bool first = i <= T;
            s.a[i] = first ? E : T; s.b[i] = first ? T : -1;
            const int D = down ? T : (first ? E : -1), U = down ? (first ? E : -1) : T;
            s.dp[i] = D; s.up[i] = U;
            if (!good(i) || (bad >= 0 && i > bad && i - bad < 50)) continue;
            s.bull[i] = D >= 0 && (D == i || lo(i) < hi(D)) ? 1 : 0;
            s.bear[i] = U >= 0 && (U == i || hi(i) > lo(U)) ? 1 : 0;
        }
    }
    return s;
}
// (1.14.1, Rassul 17:19 "a breakout is during the upwave") the candle's own swing leg runs UP (toward a top) / DOWN (toward a bottom)
inline bool upWave(const Swing& s, int i) { return i >= 0 && i < (int)s.b.size() && s.b[i] >= 0 && s.b[i] == s.up[i]; }
inline bool downWave(const Swing& s, int i) { return i >= 0 && i < (int)s.b.size() && s.b[i] >= 0 && s.b[i] == s.dp[i]; }
// (1.14.0, Rassul 15:5x "in a strong uptrend, the pullback low needs to touch or go below the 1sd, for a trend it needs to be the vwap.
// in both cases it cannot be -1 sd below vwap") the Trend PB band rule on the pullback leg: in an uptrend the leg falling INTO the
// candle's down pivot (from the top before it to that pivot); Uptrend / Strong Uptrend: some bar's low <= its VWAP + 1SD (touching
// +1SD or VWAP, 16:56);
// and no bar of the leg reaches VWAP - 1SD. Downtrend the mirror (the leg rising into the up pivot, highs vs VWAP - 1SD / VWAP, never
// VWAP + 1SD). Each bar against its own session's bands at that bar; bars without bands are skipped (none at all = not met).
// vw / sd per bar (sd 0 = no band). Returns 1 met, 0 not.
template <class H, class L>
inline int pbBands(H& high, L& low, const std::vector<double>& vw, const std::vector<double>& sd, const Swing& s, int i, int trend) {
    if (trend == 0 || i < 0 || i >= (int)s.dp.size()) return 0;
    const int P = trend > 0 ? s.dp[i] : s.up[i]; if (P < 0) return 0;
    int from = P - 1;                                                // back to the turn before the pivot (the start of the pullback)
    while (from >= 0 && from < (int)s.a.size() && s.b[from] == P) --from;
    from = std::max(0, from + 1);
    // (1.14.1, Rassul 17:29 "the 96 touched a band, so wouldn't it be a pullback") through the print's OWN candle too: an outside bar
    // after the ZigZag turn can make the pullback's real extreme (Oct 16 12:42: the turn was at 12:36, the 96 candle touched -1SD)
    const int to = std::max(P, i);
    bool touched = false;
    for (int k = from; k <= to && k < (int)vw.size() && k < (int)sd.size(); ++k) {
        if (!(vw[k] > 0) || !(sd[k] > 0) || !std::isfinite(vw[k]) || !std::isfinite(sd[k])) continue;
        const double h = (double)high[k], l = (double)low[k];
        // (Rassul 16:56 "update Trend PB so it can be 1sd or vwap"): Uptrend or Strong Uptrend - touching +1SD or VWAP both count
        // (VWAP is below +1SD, so: the low reaches +1SD or lower); never VWAP - 1SD. Downtrends the mirror.
        if (trend > 0) { if (l <= vw[k] - sd[k]) return 0; if (l <= vw[k] + sd[k]) touched = true; }
        else           { if (h >= vw[k] + sd[k]) return 0; if (h >= vw[k] - sd[k]) touched = true; }
    }
    return touched ? 1 : 0;
}

// ---- per-price rows and the biggest bid / ask ---------------------------------------------------------------------------------
struct Row { long long key = 0; long long bid = 0, ask = 0; };     // bid = lots sold AT THE BID (VOLPROFILE sellVolume); ask = bought AT THE ASK (buyVolume)
struct Pick { bool ok = false; long long key = 0, vol = 0; };
// side -1 = bid (red), +1 = ask (green); closeKey = the candle's close (the forming candle: its last price)
inline Pick biggest(const std::vector<Row>& rows, int side, long long closeKey) {
    Pick p;
    for (const Row& r : rows) {
        const long long v = side < 0 ? r.bid : r.ask;
        if (v <= 0) continue;
        if (!p.ok || v > p.vol) { p.ok = true; p.key = r.key; p.vol = v; continue; }
        if (v < p.vol) continue;
        const long long dn = std::llabs(r.key - closeKey), dp = std::llabs(p.key - closeKey);
        if (dn < dp || (dn == dp && (side < 0 ? r.key < p.key : r.key > p.key))) { p.key = r.key; }
    }
    return p;
}
// merge rows with the same key (a float price can arrive twice for one tick), drop empty ones; sorted by key
inline std::vector<Row> mergeRows(std::vector<Row> rows) {
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.key < b.key; });
    std::vector<Row> out;
    for (const Row& r : rows) {
        if (r.bid < 0 || r.ask < 0) continue;
        if (!out.empty() && out.back().key == r.key) { out.back().bid += r.bid; out.back().ask += r.ask; }
        else out.push_back(r);
    }
    out.erase(std::remove_if(out.begin(), out.end(), [](const Row& r) { return r.bid == 0 && r.ask == 0; }), out.end());
    return out;
}

// ---- the facts of one candle (frozen in the record once the candle has closed) -------------------------------------------------
struct Facts {
    long long t = 0;                   // the bar time (IRT bar END)
    bool valid = false;                // the chart had volume at price for it
    bool closed = false;
    long long o = 0, h = 0, l = 0, c = 0;   // whole-tick keys
    Pick bid, ask;
    int bidQ = -1, askQ = -1;          // ratio x 100 vs the prior median; -1 = n/a
    int seg = -1;                      // (1.3.0) time of day: 0 overnight, 1 pre-open 7:00-8:30, 2 RTH 8:30-15:00 CT (set by the host)
};
inline Facts facts(long long t, bool closed, long long o, long long h, long long l, long long c, const std::vector<Row>& rows, bool valid) {
    Facts F; F.t = t; F.closed = closed; F.o = o; F.h = std::max(h, l); F.l = std::min(h, l); F.c = c; F.valid = valid;
    if (valid) { F.bid = biggest(rows, -1, c); F.ask = biggest(rows, +1, c); }
    return F;
}
inline double median(std::vector<long long> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    return n % 2 ? (double)v[n / 2] : 0.5 * ((double)v[n / 2 - 1] + (double)v[n / 2]);
}
// the median of the same side's biggest over the PRIOR LOOKBACK candles (indexes < i only; valid ones; 0 = not enough)
inline double priorMedian(const std::vector<Facts>& v, size_t i, int side, int* nOut = nullptr) {
    std::vector<long long> xs;
    if (v[i].seg >= 0) {   // (1.3.0, Rassul 09:05 "too many bold after the open") normal = the same TIME OF DAY: the prior 20 candles of the
        // same segment (overnight / pre-open / RTH); not enough yet (the first bars after the open) = the BIGGEST of the prior 20 (strict)
        for (size_t k = i; k-- > 0 && xs.size() < (size_t)LOOKBACK;) if (v[k].valid && v[k].seg == v[i].seg) xs.push_back(side < 0 ? (v[k].bid.ok ? v[k].bid.vol : 0) : (v[k].ask.ok ? v[k].ask.vol : 0));
        if ((int)xs.size() >= MIN_PRIOR) { if (nOut) *nOut = (int)xs.size(); return median(xs); }
        xs.clear(); long long mx = 0; int n = 0;
        const size_t from = i > (size_t)LOOKBACK ? i - LOOKBACK : 0;
        for (size_t k = from; k < i; ++k) if (v[k].valid) { mx = std::max(mx, side < 0 ? (v[k].bid.ok ? v[k].bid.vol : 0) : (v[k].ask.ok ? v[k].ask.vol : 0)); ++n; }
        if (nOut) *nOut = n;
        return n >= MIN_PRIOR ? (double)mx : 0;
    }
    const size_t from = i > (size_t)LOOKBACK ? i - LOOKBACK : 0;
    for (size_t k = from; k < i; ++k) if (v[k].valid) xs.push_back(side < 0 ? (v[k].bid.ok ? v[k].bid.vol : 0) : (v[k].ask.ok ? v[k].ask.vol : 0));
    if (nOut) *nOut = (int)xs.size();
    if ((int)xs.size() < MIN_PRIOR) return 0;
    return median(xs);
}
inline int ratioQ(long long vol, double med) { if (med <= 0 || vol <= 0) return -1; return toQ((double)vol / med); }
// fill bidQ / askQ of v[i] from the candles before it (past only)
inline void rate(std::vector<Facts>& v, size_t i) {
    Facts& F = v[i];
    F.bidQ = F.valid && F.bid.ok ? ratioQ(F.bid.vol, priorMedian(v, i, -1)) : -1;
    F.askQ = F.valid && F.ask.ok ? ratioQ(F.ask.vol, priorMedian(v, i, +1)) : -1;
}
inline bool bold(int q, int boldQ) { return q >= 0 && q >= boldQ; }
// +1 = bullish absorption (circle the RED number), -1 = bearish (circle the GREEN number), 0 = none. Closed candles only.
// (1.5.0) the same absorption rule with a size floor (lots) instead of the bold ratio - the big-lot 1SD from the Liquidity Profile
inline int absorptionMin(const Facts& F, long long minVol) {
    if (!F.closed || !F.valid || minVol <= 0) return 0;
    const long long mid2 = F.h + F.l;
    if (F.bid.ok && F.bid.vol >= minVol && 2 * F.bid.key <= mid2 && F.c > F.o && F.c > F.bid.key) return +1;
    if (F.ask.ok && F.ask.vol >= minVol && 2 * F.ask.key >= mid2 && F.c < F.o && F.c < F.ask.key) return -1;
    return 0;
}
// (1.5.0, Rassul 16:15-16:17 "doesn't our distribution consider one price level in the candle or 1 large order" -> its own
// distribution) THE PRINT DISTRIBUTION: each closed candle's biggest print per side (lots traded at one price by one side), in log
// buckets (1.15^k lots); on the LOG of the size: Big mu = e^mean, 1SD / 2SD / 3SD = e^(mean + k SD). Learned per chart (market and
// bar size) from the candles on it, day (8:30 AM - 3:00 PM CT) and night apart.
struct PrintDist {
    static const int NB = 64;
    long long c[NB];
    PrintDist() { for (int i = 0; i < NB; ++i) c[i] = 0; }
    static int bucket(long long s) { if (s < 1) return -1; const int b = (int)std::floor(std::log((double)s) / std::log(1.15)); return b < 0 ? 0 : b >= NB ? NB - 1 : b; }
    static double centre(int b) { return std::pow(1.15, b + 0.5); }
    void add(long long s) { const int b = bucket(s); if (b >= 0) c[b]++; }
    bool stats(long long minN, long long& n, double& mu, long long v[3]) const {
        double sw = 0, s1 = 0, s2 = 0;
        for (int i = 0; i < NB; ++i) { if (c[i] <= 0) continue; const double x = std::log(centre(i)); sw += (double)c[i]; s1 += c[i] * x; s2 += c[i] * x * x; }
        n = (long long)sw; if (n < minN || n < 2) return false;
        const double m = s1 / sw, sd = std::sqrt(std::max(0.0, s2 / sw - m * m));
        mu = std::exp(m); for (int k = 0; k < 3; ++k) v[k] = std::max(1LL, (long long)std::llround(std::exp(m + (k + 1) * sd)));
        return v[1] >= v[0] && v[2] >= v[1];
    }
};
// "BSD1,ok,n,mu,1sd,2sd,3sd" -> the three sizes (lots); false = learning / garbled (the bold-ratio rule stays)
inline bool parseBsd(const std::string& line, long long sd[3]) {
    std::vector<std::string> f; size_t a = 0; std::string ln = line; while (!ln.empty() && (ln.back() == '\r' || ln.back() == '\n')) ln.pop_back();
    for (size_t i = 0; i <= ln.size(); ++i) if (i == ln.size() || ln[i] == ',') { f.push_back(ln.substr(a, i - a)); a = i + 1; }
    if (f.size() != 7 || f[0] != "BSD1" || f[1] != "1") return false;
    for (int k = 0; k < 3; ++k) { const std::string& t = f[4 + k]; if (t.empty() || t.size() > 9) return false; for (char ch : t) if (ch < '0' || ch > '9') return false; sd[k] = std::atoll(t.c_str()); }
    return sd[0] > 0 && sd[1] >= sd[0] && sd[2] >= sd[1];
}
inline int absorption(const Facts& F, int boldQ) {
    if (!F.closed || !F.valid) return 0;
    const long long mid2 = F.h + F.l;                                      // twice the halfway mark, in ticks
    if (F.bid.ok && bold(F.bidQ, boldQ) && 2 * F.bid.key <= mid2 && F.c > F.o && F.c > F.bid.key) return +1;
    if (F.ask.ok && bold(F.askQ, boldQ) && 2 * F.ask.key >= mid2 && F.c < F.o && F.c < F.ask.key) return -1;
    return 0;
}

// ---- the record line (one per closed candle; what the chart draws for it after a restart) -------------------------------------
static const char* const REC_HEADER = "time|valid|open|high|low|close|bid_key|bid_lots|bid_q|ask_key|ask_lots|ask_q";
inline std::string recLine(const Facts& F) {
    std::ostringstream o;
    o << F.t << '|' << (F.valid ? 1 : 0) << '|' << F.o << '|' << F.h << '|' << F.l << '|' << F.c << '|'
      << (F.bid.ok ? F.bid.key : 0) << '|' << (F.bid.ok ? F.bid.vol : 0) << '|' << F.bidQ << '|'
      << (F.ask.ok ? F.ask.key : 0) << '|' << (F.ask.ok ? F.ask.vol : 0) << '|' << F.askQ;
    return o.str();
}
inline bool sInt(const std::string& t, long long& out) {
    if (t.empty() || t.size() > 20) return false;
    size_t i = 0; if (t[0] == '-') { if (t.size() == 1) return false; i = 1; }
    for (; i < t.size(); ++i) if (t[i] < '0' || t[i] > '9') return false;
    char* end = nullptr; errno = 0; const long long v = std::strtoll(t.c_str(), &end, 10);
    if (errno != 0 || !end || *end != '\0') return false;
    out = v; return true;
}
inline bool parseRec(const std::string& line, Facts& F) {
    std::vector<std::string> f; { std::string cur; for (char c : line) { if (c == '|') { f.push_back(cur); cur.clear(); } else cur.push_back(c); } f.push_back(cur); }
    if (f.size() != 12) return false;
    long long x[12];
    for (int k = 0; k < 12; ++k) if (!sInt(f[(size_t)k], x[k])) return false;
    if (x[0] <= 0 || (x[1] != 0 && x[1] != 1) || x[3] < x[4] || x[7] < 0 || x[10] < 0 || x[8] < -1 || x[11] < -1) return false;
    F = Facts(); F.t = x[0]; F.valid = x[1] == 1; F.closed = true; F.o = x[2]; F.h = x[3]; F.l = x[4]; F.c = x[5];
    if (x[7] > 0) { F.bid.ok = true; F.bid.key = x[6]; F.bid.vol = x[7]; }
    if (x[10] > 0) { F.ask.ok = true; F.ask.key = x[9]; F.ask.vol = x[10]; }
    F.bidQ = (int)x[8]; F.askQ = (int)x[11];
    if ((F.bidQ > 0 && !F.bid.ok) || (F.askQ > 0 && !F.ask.ok)) return false;
    return true;
}

// ---- text ----------------------------------------------------------------------------------------------------------------------
inline std::string lots(long long v) { return v < 100000 ? std::to_string(v) : std::to_string(v / 1000) + "k"; }
// (1.4.0) the number on the chart: lots / divider, rounded, never 0 for a real print (the hover keeps the real lots)
inline std::string shown(long long v, int div) {
    if (div <= 1) return lots(v);
    const long long r = (v + div / 2) / div; return lots(r < 1 && v > 0 ? 1 : r);
}
inline std::string commas(long long v) {
    std::string s = std::to_string(v < 0 ? -v : v), o;
    for (size_t i = 0; i < s.size(); ++i) { if (i && (s.size() - i) % 3 == 0) o.push_back(','); o.push_back(s[i]); }
    return v < 0 ? "-" + o : o;
}
// decimals of a tick: 0.25 -> 2, 0.1 -> 1, 0.0005 -> 4, 0.00005 -> 5, 1 -> 0
inline int decimalsOf(double tick) {
    for (int d = 0; d <= 8; ++d) { const double m = tick * std::pow(10.0, d); if (std::fabs(m - std::floor(m + 0.5)) < 1e-6) return d; }
    return 8;
}
// "7,741.25" from a whole-tick key (exact: integer arithmetic on the key's tick multiple)
inline std::string price(long long key, double tick) {
    const int d = decimalsOf(tick);
    long long scale = 1; for (int k = 0; k < d; ++k) scale *= 10;
    const long long tk = (long long)std::llround(tick * (double)scale);    // the tick in units of 10^-d
    const long long units = key * tk;
    const bool neg = units < 0; const long long a = neg ? -units : units;
    std::string s = (neg ? "-" : "") + commas(a / scale);
    if (d > 0) { std::string fr = std::to_string(a % scale); while ((int)fr.size() < d) fr = "0" + fr; s += "." + fr; }
    return s;
}
inline std::string ratioText(int q) {
    if (q < 0) return "n/a";
    char b[32]; std::snprintf(b, sizeof(b), "%.1fx", q / 100.0); return b;
}
// the hover: line 1 the numbers (Rassul's own format), then what / why / how / example; absorption explains the rule
inline std::vector<std::string> hover(const Facts& F, double tick, int boldQ, const std::string& clock, int hoveredSide, bool circleOn = true) {
    std::vector<std::string> L;
    const std::string mid = "\xB7";   // the middle dot (Windows-1252), as the other panes
    std::string a = F.bid.ok ? "Biggest bid " + price(F.bid.key, tick) + ": " + commas(F.bid.vol) + " lots sold at the bid (" + ratioText(F.bidQ) + " normal)"
                             : std::string("No lots sold at the bid");
    std::string b = F.ask.ok ? "biggest ask " + price(F.ask.key, tick) + ": " + commas(F.ask.vol) + " lots (" + ratioText(F.askQ) + ")"
                             : std::string("no lots bought at the ask");
    L.push_back(a + " " + mid + " " + b);
    L.push_back("Candle " + clock + (F.closed ? " (closed)" : " (forming - may still change)") + (F.bidQ < 0 && F.askQ < 0 ? ", ratio n/a: under 10 prior candles" : ""));
    const int ab = circleOn ? absorption(F, boldQ) : 0;
    if (ab > 0) {
        L.push_back("ABSORPTION PRINT (large lot absorption, circled): bold sell at the bid at/below the halfway mark, candle closed GREEN above it.");
        L.push_back("Sellers hit hard and price still closed up: buyers absorbed them. Decided at the close, never redrawn.");
    } else if (ab < 0) {
        L.push_back("ABSORPTION PRINT (large lot absorption, circled): bold buy at the ask at/above the halfway mark, candle closed RED below it.");
        L.push_back("Buyers lifted hard and price still closed down: sellers absorbed them. Decided at the close, never redrawn.");
    }
    L.push_back(hoveredSide < 0 ? "What: red = the price where the most lots were SOLD AT THE BID (sellers hitting) in this candle."
                                : "What: green = the price where the most lots were BOUGHT AT THE ASK (buyers lifting) in this candle.");
    L.push_back("Question: where did sellers hit hardest, and buyers lift hardest, inside this candle?");
    L.push_back("Bold = at least " + ratioText(boldQ) + " the median of the same side's biggest over the prior 20 candles.");
    L.push_back("Why: a big hit that does not move price is absorption - the other side is soaking it up.");
    L.push_back("Use: bold red at a low and the candle closes green = sellers absorbed; watch for a turn up at a level.");
    L.push_back("Example: 410 sold at 7,741.25 at the low (3.1x), candle closes green above it = bullish absorption.");
    return L;
}

// ---- layout: the numbers, never overlapping -------------------------------------------------------------------------------------
struct Box { int l = 0, t = 0, r = 0, b = 0; };
inline bool touch(const Box& a, const Box& b) { return a.l <= b.r && b.l <= a.r && a.t <= b.b && b.t <= a.b; }   // sharing an edge counts
inline bool inside(const Box& a, const Box& in) { return a.l >= in.l && a.r <= in.r && a.t >= in.t && a.b <= in.b; }
struct Cand {
    int bar = 0, side = -1;            // side -1 red (left of the candle), +1 green (right)
    int x = 0, y = 0;                  // x = the candle's left edge (red) / right edge (green); y = the price's pixel row
    int w = 0, h = 0;                  // the text's size
    bool bold = false, circle = false;
    bool first = false;                // a CLOSED candle's bold number (frozen): laid out before every plain / forming number
    bool huge = false;                 // (1.5.0) 3SD or more: a white outline round the number (unused since 1.9.0)
    int tier = 0;                      // (1.10.0) the category (0 - 2) that gave this print its colour
};
struct Placed { bool shown = false, crowded = false; Box text, foot; int dy = 0; };
static const int GAP = 1;              // px between the candle and its number (1.4.1: 2 -> 1, Rassul "bring the prints closer")
static const int RING_X = 3, RING_Y = 1;   // the circle hugs the number
inline Box textBox(const Cand& c, int dy) {
    Box b;
    if (c.side < 0) { b.r = c.x - GAP - (c.circle ? RING_X : 0); b.l = b.r - c.w; }
    else { b.l = c.x + GAP + (c.circle ? RING_X : 0); b.r = b.l + c.w; }
    b.t = c.y + dy - c.h / 2; b.b = b.t + c.h;
    return b;
}
inline Box footprint(const Cand& c, const Box& t) {
    Box f = t; if (c.circle) { f.l -= RING_X; f.r += RING_X; f.t -= RING_Y; f.b += RING_Y; } return f;
}
struct LayoutStats { int shown = 0, dropped = 0, crowded = 0, nudged = 0; };
// (1.17.0, Rassul 19:58 CT screenshot: cyan "125" / "130" drawn THROUGH the HOD/LOD table) the keep-out boxes - the HOD/LOD table + its
// strip (LRA_HLB_) and the Big Lot box: a number that would touch one slides up or down out of it (at most maxSlide px, staying in the
// area), else it is not drawn. Returns 0 = clear as it was, 1 = slid (f moved), -1 = blocked (not drawn). Edges inclusive.
inline bool hits(const Box& a, const Box& b) { return !(a.r < b.l || a.l > b.r || a.b < b.t || a.t > b.b); }
inline int keepOut(Box& f, const std::vector<Box>& ko, const Box& area, int maxSlide = 40) {
    auto anyHit = [&](const Box& x) { for (const Box& k : ko) if (k.r > k.l && k.b > k.t && hits(x, k)) return true; return false; };
    if (!anyHit(f)) return 0;
    int best = 0; bool found = false;
    for (const Box& k : ko) {
        if (!(k.r > k.l && k.b > k.t)) continue;
        const int c[2] = { k.t - 1 - f.b, k.b + 1 - f.t };   // just above / just below this box
        for (int dy : c) {
            if (std::abs(dy) > maxSlide) continue;
            Box g = f; g.t += dy; g.b += dy;
            if (g.t < area.t || g.b > area.b || anyHit(g)) continue;
            if (!found || std::abs(dy) < std::abs(best)) { best = dy; found = true; }
        }
    }
    if (!found) return -1;
    f.t += best; f.b += best; return 1;
}
// cands in time order (older first; within a candle red then green); obstacles = the candles; hard = other indicators' boxes this
// plugin can see (the Tape Reader banner) - never covered, not even by a bold number; area = where numbers may be (pane minus the
// title row, minus the left Liquidity column, up to where the right-side columns start)
inline std::vector<Placed> place(const std::vector<Cand>& cands, const std::vector<Box>& obstacles, const Box& area, LayoutStats* st = nullptr,
                                 const std::vector<Box>& hard = std::vector<Box>()) {
    std::vector<Placed> out(cands.size());
    std::vector<Box> taken;
    LayoutStats S;
    std::vector<size_t> order;                           // pass 1: closed candles' bold numbers, pass 2: the rest - each in time order
    for (size_t i = 0; i < cands.size(); ++i) if (cands[i].first) order.push_back(i);
    for (size_t i = 0; i < cands.size(); ++i) if (!cands[i].first) order.push_back(i);
    for (size_t oi = 0; oi < order.size(); ++oi) {
        const size_t i = order[oi];
        const Cand& c = cands[i];
        const int row = c.h + 1;
        const int tries[3] = { 0, -row, +row };          // its own row, one row up, one row down
        bool ok = false;
        for (int k = 0; k < 3 && !ok; ++k) {
            const Box t = textBox(c, tries[k]), f = footprint(c, t);
            if (!inside(f, area)) continue;
            bool hit = false;
            for (const Box& o : obstacles) if (touch(f, o)) { hit = true; break; }
            for (const Box& o : hard) if (!hit && touch(f, o)) hit = true;
            for (size_t j = 0; !hit && j < taken.size(); ++j) if (touch(f, taken[j])) hit = true;
            if (hit) continue;
            out[i].shown = true; out[i].text = t; out[i].foot = f; out[i].dy = tries[k]; ok = true;
            if (k) ++S.nudged;
        }
        if (!ok && c.bold) {                                // no room: a bold number still shows on its own row if it touches no number
            const Box t = textBox(c, 0), f = footprint(c, t);
            bool hit = !inside(f, area);
            for (const Box& o : hard) if (!hit && touch(f, o)) hit = true;
            for (size_t j = 0; !hit && j < taken.size(); ++j) if (touch(f, taken[j])) hit = true;
            if (!hit) { out[i].shown = true; out[i].crowded = true; out[i].text = t; out[i].foot = f; ok = true; ++S.crowded; }
        }
        if (ok) { taken.push_back(out[i].foot); ++S.shown; } else ++S.dropped;
    }
    if (st) *st = S;
    return out;
}

// ---- 12-hour Central clock ------------------------------------------------------------------------------------------------------
inline std::string clock12(int h, int m) {
    if (h < 0 || h > 23 || m < 0 || m > 59) return "?";
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, h < 12 ? "AM" : "PM"); return b;
}

// ---- (1.15.0, audit B finding 2) Big Lot tiers per SESSION, decided once ------------------------------------------------------------
// The prints of one session (svl session key): the biggest print per side of each closed candle, per phase (0 night, 1 day), and how
// many DAY candles it has (a complete session has at least 3/4 of them).
struct SessPrints { int dayCandles = 0; std::vector<long long> pv[2]; };
// The tiers in force for every candle of ONE session: the pooled prints (biggest first) of the complete sessions before it.
struct TierSet { bool ok = false; int nSess = 0; std::vector<long long> pv[2]; std::vector<long> used; std::string why = "learning"; };
// sess = the session to judge; bySess = every session's prints known so far; need = day candles of a complete session.
// Only sessions BEFORE sess that are complete count (never today's partial one); the latest maxSess of them are pooled; fewer than
// minSess = not ok ("learning": nothing is drawn by size). The caller decides WHEN to call it (history read) and keeps the result.
inline TierSet tiersFor(long sess, const std::map<long, SessPrints>& bySess, int need, int maxSess = 5, int minSess = 3) {
    TierSet T;
    for (std::map<long, SessPrints>::const_reverse_iterator it = bySess.rbegin(); it != bySess.rend() && (int)T.used.size() < maxSess; ++it) {
        if (it->first >= sess || it->second.dayCandles < need) continue;
        T.used.push_back(it->first);
        for (int ph = 0; ph < 2; ++ph) T.pv[ph].insert(T.pv[ph].end(), it->second.pv[ph].begin(), it->second.pv[ph].end());
    }
    T.nSess = (int)T.used.size();
    for (int ph = 0; ph < 2; ++ph) std::sort(T.pv[ph].begin(), T.pv[ph].end(), std::greater<long long>());
    T.ok = T.nSess >= minSess;
    T.why = T.ok ? std::to_string(T.nSess) + " complete sessions" : "learning (" + std::to_string(T.nSess) + " of " + std::to_string(minSess) + " complete sessions)";
    return T;
}
// Big Lot N of a tier set: the size at rank N x sessions (the size reached about N times a session); 0 = not known
inline long long bigLotOf(const TierSet& T, int ph, int n) {
    if (!T.ok || ph < 0 || ph > 1 || n < 1) return 0;
    const size_t r = (size_t)n * (size_t)T.nSess;
    return (r >= 1 && T.pv[ph].size() >= std::max<size_t>(r, 30)) ? T.pv[ph][r - 1] : 0;
}
// a category's size: category 4 (n <= 0) = its fixed lots; 1 - 3 = Big Lot N, never under the category's minimum; 0 = learning
inline long long catSizeOf(const TierSet& T, int ph, const Cat& c) {
    if (c.n <= 0) return (long long)c.lots;
    const long long z = bigLotOf(T, ph, c.n);
    return z > 0 ? std::max(z, (long long)c.lots) : 0;
}
// ---- (1.16.0, Rassul 15:10 settings-window screenshot) the window and the chart -------------------------------------------------
// what the window shows: the divider as used (0 in the file = the market default: ES / CL 10, others 1)
// (1.16.2, 2026-10-18: his BigBidAsk.settings-ES.txt was saved at 15:07 CT with every box off and every colour 0 -> no big prints drawn)
// the shape of a blank window read: never written over his file
inline bool looksBlank(const Settings& s) {
    if (s.box || s.circle || s.dim || s.statsChart) return false;
    for (int k = 0; k < 4; ++k) if (s.cat[k].on || s.cat[k].bid || s.cat[k].ask) return false;
    return s.boxBid == 0 && s.boxAsk == 0 && s.circleCol == 0;
}
inline Settings shownSettings(Settings s, const std::string& mkt) { if (s.div <= 0) s.div = divFor(mkt); return s; }
// what the chart uses: a level group that is off uses none of its levels (its boxes may still be ticked in an old file)
inline Settings effective(Settings s) { if (!s.keyOn) s.keyMask = 0; if (!s.gamOn) s.gamMask = 0; if (!s.volOn) s.volMask = 0; return s; }
// a swatch at (near) black: numbers / boxes would vanish on his black chart - the saved colour is kept
inline bool nearBlack(unsigned c) { return ((c >> 16) & 0xFF) < 0x18 && ((c >> 8) & 0xFF) < 0x18 && (c & 0xFF) < 0x18; }
// ---- (1.16.0, Rassul 2026-10-18 15:03 CT approved: "make it simple like BL 5 1434 and just have a compact box of the averages
// connected to the hod lod box ... so it doesn't overlap"; "no, just the avg") the compact Big Lot box: BL5 / BL10 / BL20 = the rolling
// average (latest 5 complete sessions, one outlier session dropped) of each session's 5th / 10th / 20th biggest print of the CURRENT
// session type (RTH during RTH, overnight overnight). Everything else (prior session, today's count, the other type, the outlier,
// scoring) is in the box's hover.
static const int BOX_N[3] = { 5, 10, 20 };
struct TypeAvg { bool night = false; std::vector<long> done; long long prior[3] = { 0, 0, 0 }; Avg avg[3]; int nSess = 0;
                 long droppedSess = -1; double droppedRatio = 0; int today[3] = { 0, 0, 0 }; long long thr[3] = { 0, 0, 0 }; };
// candles of a complete session of one type: 3/4 of its candles. Day = the RTH open to an hour after the RTH close; night = the rest
// of the 23-hour session (17:00 - the open, an hour after the close - 16:00). The day rule = the 1.11 statistics' rule.
inline int typeNeed(bool night, int openMin, int closeMin, int per) {
    if (per <= 0) per = 180;
    const int dayMin = std::max(60, closeMin + 60 - openMin), nightMin = std::max(60, 1380 - dayMin);
    return std::max(20, (int)((long long)(night ? nightMin : dayMin) * 60 / per * 3 / 4));
}
// prints: per session, that type's biggest bid and biggest ask lots of each candle; candles: that type's candle count per session;
// cur = the current session (never averaged); thr = today's tiers (BL5 / 10 / 20 of the session's tier set) for "today N of ~N"
inline TypeAvg typeAvg(const std::map<long, std::vector<long long>>& prints, const std::map<long, int>& candles, long cur, int need,
                       const long long thr[3], bool night) {
    TypeAvg A; A.night = night;
    for (const auto& kv : candles) if (kv.first < cur && kv.second >= need) A.done.push_back(kv.first);
    static const std::vector<long long> none;
    auto of = [&](long s) -> const std::vector<long long>& { auto it = prints.find(s); return it == prints.end() ? none : it->second; };
    for (int q = 0; q < 3; ++q) {
        const int N = BOX_N[q];
        A.prior[q] = A.done.empty() ? 0 : nth(of(A.done.back()), N);
        std::vector<long long> xs; std::vector<long> ss;
        for (int k = (int)A.done.size() - 1; k >= 0 && (int)xs.size() < 5; --k) { xs.push_back(nth(of(A.done[k]), N)); ss.push_back(A.done[k]); }
        A.avg[q] = avgDropOutlier(xs);
        if (A.avg[q].dropped >= 0 && A.droppedSess < 0) { A.droppedSess = ss[(size_t)A.avg[q].dropped]; A.droppedRatio = A.avg[q].ratio; }
        A.nSess = std::max(A.nSess, A.avg[q].used);
        A.thr[q] = thr ? thr[q] : 0;
        if (A.thr[q] > 0) for (long long v : of(cur)) if (v >= A.thr[q]) ++A.today[q];
    }
    return A;
}
inline std::string boxValue(const TypeAvg& A, int q) { return (q >= 0 && q < 3 && A.avg[q].used > 0 && A.avg[q].avg > 0) ? commas(A.avg[q].avg) : std::string("--"); }
// the box: exactly 3 rows "BL5" / "1,434" ("--" while learning)
inline std::vector<std::pair<std::string, std::string>> boxRows(const TypeAvg& A) {
    std::vector<std::pair<std::string, std::string>> r;
    for (int q = 0; q < 3; ++q) r.push_back({ "BL" + std::to_string(BOX_N[q]), boxValue(A, q) });
    return r;
}
// the hover: what it is, the question, how, the numbers removed from the box, why, how to use it, an example
// tiers = the session's tier line (what the candles use), dropped = "Oct 14" of the outlier session (or ""), score = the nightly line
inline std::vector<std::string> boxHover(const TypeAvg& A, const TypeAvg& O, const std::string& market, const std::string& tiers,
                                         const std::string& dropped, const std::string& score) {
    const std::string mid = " \xB7 ", ty = A.night ? "overnight" : "RTH", oty = A.night ? "RTH" : "overnight";
    const std::string n = std::to_string(A.nSess);
    std::vector<std::string> L;
    L.push_back("Big Lot sizes " + mid + ty + mid + (A.nSess > 0 ? n + "-session average" : std::string("learning")) + mid + market);
    L.push_back("Question: how big is a really big print in this market right now - which numbers on the candles are the rare ones?");
    L.push_back("What: BL5 = the size of one print (lots at one price, one side, one candle) reached about 5 times a session; BL10 ~10, BL20 ~20 times.");
    L.push_back("How: each complete earlier session's 5th / 10th / 20th biggest " + ty + " print, averaged over the latest " + (A.nSess > 0 ? n : std::string("5")) + " sessions (an outlier session is dropped).");
    for (int q = 0; q < 3; ++q) {
        const int N = BOX_N[q];
        L.push_back("BL" + std::to_string(N) + ":  prior session " + (A.prior[q] > 0 ? commas(A.prior[q]) : std::string("learning")) + mid
                    + n + "-session avg " + boxValue(A, q) + mid + "today " + std::to_string(A.today[q]) + " of ~" + std::to_string(N));
    }
    L.push_back("The " + oty + " averages: BL5 / 10 / 20 = " + boxValue(O, 0) + " / " + boxValue(O, 1) + " / " + boxValue(O, 2) + " lots.");
    if (!tiers.empty()) L.push_back(tiers);
    L.push_back(A.droppedSess >= 0 && !dropped.empty() ? [&]() { char r[16]; std::snprintf(r, sizeof(r), "%.1f", A.droppedRatio);
                    return "Outlier dropped: " + dropped + " (" + r + "x the other sessions)."; }() : std::string("No session dropped as an outlier."));
    if (!score.empty()) L.push_back(score);
    L.push_back("Why: big sizes differ by market and between RTH and overnight; at or above BL5 a print is one of the ~5 biggest of a normal session.");
    L.push_back("Use: a print >= BL5 at a key level = a big trader acting - absorption if price does not move, initiative if it breaks and holds.");
    L.push_back("Example: ES RTH BL5 1,434 - 1,500 lots sold at LonLO that fail to push lower is a top-5 print of the day: watch for the turn.");
    return L;
}
// ---- (1.16.0, Rassul 2026-10-18 15:04-15:07: "the 1000 lots that are part of category 4 are not in cyan", "cat 1 and 4 should be
// cyan", "i defined cat 4 because it is a user input") the ORDER a print is judged in: category 1 first, then category 4 (HIS fixed
// size wins over categories 2 / 3), then categories 2 / 3 biggest size first. 1.14 - 1.15 sorted all four by size, so a 1,000 - 1,389
// lot ES print took category 2 / 3 (BL10 1,190 / BL20 1,007, white) before his category 4 (>= 1,000, cyan). The chart, the print log
// and the board events all go through choose(), which uses this order. zz = each category's size now (0 = off / learning).
inline void catOrder(const long long zz[4], int ord[4]) {
    ord[0] = 0; ord[1] = 3;
    if (zz[2] > zz[1]) { ord[2] = 2; ord[3] = 1; } else { ord[2] = 1; ord[3] = 2; }
}
// the first category (0 - 3) a print of vol lots reaches whose Show filter passes (pass[k]; category 4 has no filter), -1 = none
inline int firstCat(const long long zz[4], long long vol, const bool pass[4]) {
    int ord[4]; catOrder(zz, ord);
    for (int q = 0; q < 4; ++q) { const int k = ord[q]; if (zz[k] <= 0 || vol < zz[k]) continue; if (k == 3 || pass[k]) return k; }
    return -1;
}
// ---- (1.21.0, Rassul approved breakout-mockup-v2 at 20:10 CT 2026-10-21: "no dashed pivot line. yes build") BREAKOUT / BREAKDOWN -----
// Decided ONCE on the candle's close, never repainted (kept in <stem>-breakouts-<session>.csv as BBO| lines):
//   BO needs all of: (1) the candle trades THROUGH the higher of the last two pivot highs (breakFlags: low below P, high above P - no
//   close beyond needed); (2) its high is the highest high of the last 10 candles (itself included); (3) its range >= the prior
//   candle's range; (4) its volume > the prior candle's volume OR its range > the prior candle's range.
//   BD = the mirror (the lower of the last two pivot lows, a 10-candle low). Both on one outside candle: BO is judged first.
//   THE PRINT: the candle's biggest BUY cell (lots bought at the ask at one price) for BO / biggest SELL cell (at the bid) for BD; drawn
//   only when bigger than the prior candle's biggest cell of that side; its "biggest in N" = consecutive prior candles whose biggest
//   same-side cell it beats, capped at 50. A breakout with no qualifying print gets the letter alone.
static const int BO_LOOK = 10, BO_CAP = 50;
enum BoReason { BO_R_NONE = 0, BO_R_FIRED = 1, BO_R_NOT_EXTREME = 2, BO_R_RANGE = 3, BO_R_VOLRANGE = 4, BO_R_TIME = 5, BO_R_WAVE_ENDED = 6, BO_R_SAME_WAVE = 7 };
static const int BO_R_LAST = BO_R_SAME_WAVE;
inline const char* boReasonName(int r) {
    return r == BO_R_FIRED ? "fired" : r == BO_R_NOT_EXTREME ? "not_10bar_extreme" : r == BO_R_RANGE ? "range_below_prior"
         : r == BO_R_VOLRANGE ? "no_volume_or_range_up" : r == BO_R_TIME ? "time_filter" : r == BO_R_WAVE_ENDED ? "wick_wave_ended"
         : r == BO_R_SAME_WAVE ? "same_wave_already" : "none";
}
// one candle's inputs: whole-tick high / low keys, the bar volume, the biggest cell per side (0 = sell at the bid, 1 = buy at the ask)
struct BoIn { bool known = false; long long hk = 0, lk = 0, vol = 0; long long big[2] = { 0, 0 }, bigKey[2] = { 0, 0 }; };
struct BoDec {
    long long t = 0;
    int dir = 0;                 // +1 BO, -1 BD, 0 = no pivot traded through (not a candidate)
    int reason = BO_R_NONE;      // BO_R_FIRED or why not
    bool fired = false;
    double piv = 0; int which = 0;        // the pivot traded through; 1 = the prior pivot, 2 = the one 2 back (higher / lower)
    long long hk = 0, lk = 0, range = 0, prange = 0, vol = 0, pvol = 0;   // ticks / lots
    long long lots = 0, key = 0, prior = 0; int n = 0;                       // the print: lots at key, the prior candle's biggest, biggest in N
    bool print = false;          // the print qualifies (lots > the prior candle's same-side biggest)
    // (1.23.0) the same-wave rule: t0 = the breakout candle (the candidate), t = the candle the decision is ON (the mark candle when
    // fired; the wave-end candle for wick_wave_ended); wick = confirmed by a LATER candle's close; later = candles after t0; ck = the
    // close key of candle t. i0 / im = bar indices (not recorded).
    long long t0 = 0, ck = 0; int wick = 0, later = 0; int i0 = -1, im = -1;
};
// candle i from in[] (known inputs) and the breakFlagsPx vectors; wait = some needed input not known yet (decide later)
inline BoDec breakoutAt(const std::vector<BoIn>& in, const std::vector<signed char>& up, const std::vector<signed char>& dn,
                        const std::vector<double>& upPx, const std::vector<double>& dnPx, int i, bool& wait) {
    BoDec D; wait = false;
    const int n = (int)in.size();
    if (i < 1 || i >= n || i >= (int)up.size() || i >= (int)dn.size()) return D;
    const int u = up[(size_t)i], d = dn[(size_t)i];
    if (!u && !d) return D;                                                  // not a candidate: nothing traded through
    if (!in[(size_t)i].known || !in[(size_t)(i - 1)].known) { wait = true; return D; }
    const BoIn& c = in[(size_t)i]; const BoIn& p = in[(size_t)(i - 1)];
    D.hk = c.hk; D.lk = c.lk; D.range = c.hk - c.lk; D.prange = p.hk - p.lk; D.vol = c.vol; D.pvol = p.vol;
    const bool base = D.range >= D.prange && (D.vol > D.pvol || D.range > D.prange);
    auto extreme = [&](int dir) {
        for (int k = i - 1; k >= 0 && k > i - BO_LOOK; --k) { if (!in[(size_t)k].known) return -1; if (dir > 0 ? in[(size_t)k].hk > c.hk : in[(size_t)k].lk < c.lk) return 0; }
        return i - (BO_LOOK - 1) >= 0 ? 1 : 0;                                // fewer than 10 candles on the chart: no 10-candle extreme
    };
    int exU = 0, exD = 0;
    if (u) { exU = extreme(+1); if (exU < 0) { wait = true; return D; } }
    if (d) { exD = extreme(-1); if (exD < 0) { wait = true; return D; } }
    if (u && exU == 1 && base) D.dir = +1;                                   // BO judged first
    else if (d && exD == 1 && base) D.dir = -1;
    if (D.dir) { D.fired = true; D.reason = BO_R_FIRED; }
    else {                                                                   // why not: named for the side traded through (BO first,
        D.dir = u ? +1 : -1;                                                 // unless only the BD side made its 10-candle extreme)
        if (u && d && exU != 1 && exD == 1) D.dir = -1;
        const int ex = D.dir > 0 ? exU : exD;
        D.reason = ex != 1 ? BO_R_NOT_EXTREME : D.range < D.prange ? BO_R_RANGE : BO_R_VOLRANGE;
    }
    D.which = D.dir > 0 ? u : d; D.piv = D.dir > 0 ? upPx[(size_t)i] : dnPx[(size_t)i];
    const int sd = D.dir > 0 ? 1 : 0;
    D.lots = c.big[sd]; D.key = c.bigKey[sd]; D.prior = p.big[sd];
    if (D.lots > 0) {
        for (int k = i - 1; k >= 0 && D.n < BO_CAP; --k) {
            if (!in[(size_t)k].known) { if (D.fired) { wait = true; return BoDec(); } break; }
            if (in[(size_t)k].big[sd] >= D.lots) break;
            ++D.n;
        }
    }
    D.print = D.lots > 0 && D.lots > D.prior;                                // == biggest in N >= 1
    return D;
}
// ---- (1.23.0, Rassul 2026-10-22 approved) GREEN / RED + the SAME-WAVE rule ----------------------------------------------------------
//   BO only on a GREEN candle (close > open), BD only on a RED candle (close < open).
//   A qualifying breakout candle (breakoutAt fired: through the pivot, 10-candle extreme, range / volume) that does not CLOSE beyond the
//   pivot (a wick-only break) - or closes beyond it in the wrong colour - waits: the breakout candle OR any LATER candle of the SAME
//   up-wave (BO) / down-wave (BD) that closes beyond the pivot in the right colour confirms it, and the mark goes ON that confirming
//   candle. "As long as there is a close in the same upwave." The wave = his 1-bar ZigZag walk (the one breakFlagsPx uses) decided bar
//   by bar on closed candles: the up-wave ends on the candle that flips the walk down (that candle belongs to the new down-wave and
//   cannot confirm). When the wave ends first the wick break FAILED: logged only (reason wick_wave_ended), no mark is drawn.
//   One break per wave and side: a later qualifying candle in the same wave while one is waiting / after one fired is logged
//   same_wave_already (the waiting one may still be confirmed by it). Context, not an entry: the BO / BD test (research-artifacts
//   learning, 2026-10-22) found close-beyond and same-wave breaks both continue about 49% - the same as a random candle of the wave.
// the walk's direction after each bar (+1 up-wave, -1 down-wave; a broken bar keeps the last direction) - same walk as breakFlagsPx
template <class H, class L>
inline void zigDir(H& high, L& low, int n, std::vector<signed char>& dir) {
    dir.assign(n > 0 ? n : 0, 0);
    if (n < 2) return;
    auto hi = [&](int i) { return (double)high[i]; }; auto lo = [&](int i) { return (double)low[i]; };
    int dirn = hi(1) >= hi(0) ? 1 : -1, ext = 0;
    for (int i = 1; i < n; ++i) {
        if (std::isfinite(hi(i)) && std::isfinite(lo(i)) && hi(i) >= lo(i)) {
            if (dirn == 1) {
                if (hi(i) > hi(ext)) ext = i;
                else if (lo(i) < lo(i - 1) && i != ext) { ext = i; dirn = -1; }
            } else {
                if (lo(i) < lo(ext)) ext = i;
                else if (hi(i) > hi(i - 1) && i != ext) { ext = i; dirn = 1; }
            }
        }
        dir[(size_t)i] = (signed char)dirn;
    }
}
// one closed candle's open / close in whole ticks (ok = finite and on the chart)
struct BoBar { bool ok = false; long long o = 0, c = 0; };
// the print of candle j for side sd (1 buy / 0 sell): fills lots / key / prior / n / print; false = an input not known yet (wait)
inline bool boPrintAt(const std::vector<BoIn>& in, int j, int sd, BoDec& D) {
    if (j < 1 || j >= (int)in.size() || !in[(size_t)j].known || !in[(size_t)(j - 1)].known) return false;
    D.lots = in[(size_t)j].big[sd]; D.key = in[(size_t)j].bigKey[sd]; D.prior = in[(size_t)(j - 1)].big[sd]; D.n = 0;
    if (D.lots > 0) for (int k = j - 1; k >= 0 && D.n < BO_CAP; --k) { if (!in[(size_t)k].known) return false; if (in[(size_t)k].big[sd] >= D.lots) break; ++D.n; }
    D.print = D.lots > 0 && D.lots > D.prior;
    return true;
}
struct BoRun {
    std::vector<BoDec> ev;         // final decisions in order (fired, not qualified, wick_wave_ended, same_wave_already, time_filter)
    int stop = -1;                 // the first bar NOT processed (an input not known yet); -1 = all processed
    bool pendOn[2] = { false, false }; BoDec pend[2];   // [1] BO / [0] BD waiting for a close in the same wave (not recorded)
};
// candles from .. end-1 (all CLOSED), in order; skip[j] = candle j is in the time filter (a mark there is logged time_filter, not
// drawn). The run is a pure function of the closed candles, so a recompute (or a restart) gives the same decisions. Candles before
// emitFrom only build the state (a wave that started earlier): an input not known there is skipped, and nothing before it is emitted.
inline BoRun boWaves(const std::vector<BoIn>& in, const std::vector<BoBar>& bar, const std::vector<signed char>& zdir,
                     const std::vector<signed char>& up, const std::vector<signed char>& dn, const std::vector<double>& upPx,
                     const std::vector<double>& dnPx, const std::vector<char>& skip, double tick, int from, int end, int emitFrom = 0) {
    BoRun R; bool done[2] = { false, false };
    if (!(tick > 0) || !std::isfinite(tick)) { R.stop = from; return R; }
    end = std::min(end, std::min((int)in.size(), std::min((int)bar.size(), (int)zdir.size())));
    from = std::max(1, from);
    auto beyond = [&](int j, int sd, double piv) {
        const long long pk = (long long)std::llround(piv / tick); const BoBar& b = bar[(size_t)j];
        return sd ? b.c > pk : b.c < pk; };
    auto colour = [&](int j, int sd) { const BoBar& b = bar[(size_t)j]; return sd ? b.c > b.o : b.c < b.o; };
    for (int j = from; j < end; ++j) {
        if (!bar[(size_t)j].ok) { R.stop = j; break; }
        std::vector<BoDec> out; bool pOn[2] = { R.pendOn[0], R.pendOn[1] }; BoDec pD[2] = { R.pend[0], R.pend[1] }; bool dn2[2] = { done[0], done[1] };
        bool wait = false; const bool skipJ = j < (int)skip.size() && skip[(size_t)j], quiet = j < emitFrom;
        auto fire = [&](BoDec E, int sd) -> bool {        // E fires ON candle j
            BoDec P = E; if (!boPrintAt(in, j, sd, P)) { if (!quiet) return false; P.lots = P.key = P.prior = 0; P.n = 0; P.print = false; }
            P.im = j; P.wick = j != P.i0 ? 1 : 0; P.later = j - P.i0; P.ck = bar[(size_t)j].c;
            if (skipJ) { P.fired = false; P.reason = BO_R_TIME; } else { P.fired = true; P.reason = BO_R_FIRED; }
            out.push_back(P); return true; };
        // 1. the waiting wick breaks: the wave ended -> failed (logged); a right-colour close beyond -> confirmed on this candle
        for (int sd = 1; sd >= 0 && !wait; --sd) {
            const int sg = sd ? 1 : -1;
            if (zdir[(size_t)j] != sg) dn2[sd] = false;                         // a new wave of the other side: the side may break again
            if (!pOn[sd]) continue;
            if (zdir[(size_t)j] != sg) { BoDec E = pD[sd]; E.fired = false; E.reason = BO_R_WAVE_ENDED; E.im = j; E.later = j - E.i0; E.ck = bar[(size_t)j].c; E.wick = 0; out.push_back(E); pOn[sd] = false; continue; }
            if (beyond(j, sd, pD[sd].piv) && colour(j, sd)) { if (!fire(pD[sd], sd)) { wait = true; break; } pOn[sd] = false; dn2[sd] = true; }
        }
        // 2. this candle as a breakout candidate (breakoutAt: the 1.21.0 rule, unchanged)
        if (!wait) {
            bool w = false; BoDec D = breakoutAt(in, up, dn, upPx, dnPx, j, w);
            if (w) { if (!quiet) wait = true; }
            else if (D.dir != 0) {
                D.i0 = j; D.im = j; D.ck = bar[(size_t)j].c;
                const int sd = D.dir > 0 ? 1 : 0, sg = D.dir;
                if (!D.fired) out.push_back(D);                                       // not a qualifying breakout: logged as before
                else if (pOn[sd] || dn2[sd]) { D.fired = false; D.reason = BO_R_SAME_WAVE; out.push_back(D); }
                else if (beyond(j, sd, D.piv) && colour(j, sd)) { if (!fire(D, sd)) wait = true; else dn2[sd] = true; }
                else if (zdir[(size_t)j] != sg) { D.fired = false; D.reason = BO_R_WAVE_ENDED; out.push_back(D); }   // not in its wave: nothing can confirm it
                else { D.fired = false; D.reason = BO_R_NONE; pD[sd] = D; pOn[sd] = true; }   // a wick break: waits for a close in the wave
            }
        }
        if (wait) { R.stop = j; break; }
        for (const BoDec& E : out) if (E.im >= emitFrom) R.ev.push_back(E);
        for (int sd = 0; sd < 2; ++sd) { R.pendOn[sd] = pOn[sd]; R.pend[sd] = pD[sd]; done[sd] = dn2[sd]; }
    }
    return R;
}
// the record line (1.23.0, 24 fields; the 20-field 1.21 / 1.22 lines are ignored on load - the old rule's letters are not redrawn):
// BBO|t|t0|dir|fired|reason|pivot|which|high_key|low_key|range|prior_range|vol|prior_vol|lots|key|prior_big|n|print|wick|later|close_key|rth|version
static const char* const BO_HEADER = "BBO|t|t0|dir|fired|reason|pivot|which|high_key|low_key|range|prior_range|vol|prior_vol|lots|key|prior_big|n|print|wick|later|close_key|rth|version";
inline std::string boLine(const BoDec& D, int rth, const char* ver) {
    char b[480]; std::snprintf(b, sizeof(b), "BBO|%lld|%lld|%d|%d|%s|%.10g|%d|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%d|%d|%d|%d|%lld|%d|%s",
        D.t, D.t0, D.dir, D.fired ? 1 : 0, boReasonName(D.reason), D.piv, D.which, D.hk, D.lk, D.range, D.prange, D.vol, D.pvol, D.lots, D.key, D.prior, D.n, D.print ? 1 : 0, D.wick, D.later, D.ck, rth, ver);
    return b;
}
inline bool parseBoLine(const std::string& line0, BoDec& D) {
    std::string line = line0; while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
    std::vector<std::string> f; { std::string cur; for (char c : line) { if (c == '|') { f.push_back(cur); cur.clear(); } else cur.push_back(c); } f.push_back(cur); }
    if (f.size() != 24 || f[0] != "BBO") return false;
    long long x = 0; D = BoDec();
    auto I = [&](size_t k, long long& o) { return sInt(f[k], o); };
    if (!I(1, D.t) || D.t <= 0 || !I(2, D.t0) || D.t0 <= 0 || D.t0 > D.t) return false;
    if (!I(3, x) || (x != 1 && x != -1)) return false;
    D.dir = (int)x;
    if (!I(4, x) || (x != 0 && x != 1)) return false;
    D.fired = x == 1;
    D.reason = BO_R_NONE; for (int r = 1; r <= BO_R_LAST; ++r) if (f[5] == boReasonName(r)) D.reason = r;
    if (D.reason == BO_R_NONE || (D.fired != (D.reason == BO_R_FIRED))) return false;
    { char* e = nullptr; D.piv = std::strtod(f[6].c_str(), &e); if (!e || *e || !std::isfinite(D.piv)) return false; }
    if (!I(7, x) || x < 1 || x > SWING_KEEP) return false;   // (1.26.0) 1 - 3 = which of the last 3 intact swings
    D.which = (int)x;
    if (!I(8, D.hk) || !I(9, D.lk) || !I(10, D.range) || !I(11, D.prange) || !I(12, D.vol) || !I(13, D.pvol) || !I(14, D.lots) || !I(15, D.key) || !I(16, D.prior)) return false;
    if (!I(17, x) || x < 0 || x > BO_CAP) return false;
    D.n = (int)x;
    if (!I(18, x) || (x != 0 && x != 1)) return false;
    D.print = x == 1;
    if (!I(19, x) || (x != 0 && x != 1)) return false;
    D.wick = (int)x;
    if (!I(20, x) || x < 0 || x > 100000) return false;
    D.later = (int)x;
    if (!I(21, D.ck)) return false;
    if (D.hk < D.lk || D.lots < 0 || D.vol < 0) return false;
    if ((D.wick && D.t <= D.t0) || (D.fired && !D.wick && D.t != D.t0)) return false;   // confirmed later = a later candle; same candle = t0
    return true;
}
// the details line (the status file; the hovers are off since 1.17.1) in his order: the question, what happened, how to use it
inline std::string boDetail(const BoDec& D, double tick, const std::string& clock) {
    auto x = [](long long a, long long b) { char r[24]; if (b <= 0) return std::string(a > 0 ? "new" : "n/a"); std::snprintf(r, sizeof(r), "%.1fx", (double)a / (double)b); return std::string(r); };
    const bool bo = D.dir > 0;
    std::string s = clock + "," + (bo ? "BO up" : "BO down") + "," + (D.fired ? "fired" : std::string("not fired: ") + boReasonName(D.reason));
    s += ",Q: did " + std::string(bo ? "buyers break out" : "sellers break down") + " with real size?";
    s += ",pivot " + price((long long)std::llround(D.piv / tick), tick) + (D.which >= 2 ? std::string(" (intact swing ") + std::to_string(D.which) + " back)" : std::string(" (latest intact swing)")) + " traded through";
    s += std::string(",") + (bo ? "10-candle high " : "10-candle low ") + price(bo ? D.hk : D.lk, tick);
    s += ",range " + x(D.range, D.prange) + " prior,volume " + x(D.vol, D.pvol) + " prior";
    if (D.lots > 0) s += ",print " + commas(D.lots) + " lots " + (bo ? "bought" : "sold") + " at " + price(D.key, tick) + " (prior candle's biggest " + commas(D.prior) + ")" + (D.print ? ",biggest in " + std::to_string(D.n) : ",not bigger than the prior candle: letter only");
    else s += ",no print: letter only";
    if (D.fired) s += D.wick ? ",confirmed " + std::to_string(D.later) + " candle" + (D.later == 1 ? "" : "s") + " after the wick break by a " + (bo ? "green" : "red") + " close " + (bo ? "above" : "below") + " the pivot in the same " + (bo ? "up" : "down") + "-wave"
                            : std::string(",") + (bo ? "green candle closed above" : "red candle closed below") + " the pivot";
    else if (D.reason == BO_R_WAVE_ENDED) s += std::string(",failed wick break: the ") + (bo ? "up" : "down") + "-wave ended before a close beyond the pivot (logged, not drawn)";
    s += bo ? ",use: context - a confirmed break up; don't fade it (don't short against it) while price holds above " + price((long long)std::llround(D.piv / tick), tick) + ". Not an entry: it continued about 49% in the test - the same as any candle in the wave"
            : ",use: context - a confirmed break down; don't fade it (don't buy against it) while price holds below " + price((long long)std::llround(D.piv / tick), tick) + ". Not an entry: it continued about 49% in the test - the same as any candle in the wave";
    s += ",status: decided on closed candles, never repainted";
    return s;
}

} // namespace bba
#endif
