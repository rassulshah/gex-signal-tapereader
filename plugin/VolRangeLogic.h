// VolRangeLogic.h (1.0.0, 2026-11-15) - SDK-free logic of lsVolRead / lsRangeRead (VolRead.cpp, RangeRead.cpp): Rassul's "little reader
// for the volume and range" (2026-11-14 21:58 CT "BVG , biggest green bar volume in x many green bars, BVR, biggest red bar volume in x many
// red bars , BV which is the biggest volume in x consecutive bars. and BVMax which is the max number of all three. i will use bvmax to
// display on top of the green or red bars when it is a high number"; 21:59 "put the max stat on top then Bv then BV green annd then BVR";
// 2026-11-15 16:07 "i also want to implement the new volume and range indicators that you are showing in the mockup", 17:06 "go build").
//   value   VOLUME pane: the bar's volume.  RANGE pane: high - low in ticks.
//   colour  green = close >= open, red = close < open.
//   BV / BR   = how many bars in a row before this one had a SMALLER value (the count stops at the first bar as big or bigger).
//   BVG / BRG = the same count over GREEN bars only (for a green bar);  BVR / BRR over RED bars only (for a red bar).
//   Max       = the larger of the two. Counts stop at 99 ("99+").
//   On the bars: the Max number over every CLOSED bar whose Max is 20 or more (decided at the bar's close, never repainted).
//   Read box (left of the pane): title, Max, BV, BVG, BVR for the last CLOSED bar; the other colour's row shows "-".
#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "VRRead.h"           // (1.4.0) the pane reads: verdict / candle / two behaviours + the numbers (SDK-free, tests_volrange/t_vrread)
#include "SplitText.h"        // (1.5.0) the split read: verdict + plain-English sentences left, numbers right (SDK-free)
#include "HtfSr.h"          // (1.3.0) the 3-hour R1 / R2 / S1 / S2 walk (SDK-free; = lsLimitProfile / lsCobSignal)

namespace vrr {

static const char* const VERSION = "1.18.1";   // (1.18.1, Rassul 2026-11-27 11:49 CT "the box should always be on the right") scrolled back: the read sits right of the last VISIBLE candle (host); no room there: the small box at the pane's right edge (was the left edge)
// was static const char* const VERSION = "1.18.0";   // (1.18.0, Rassul 2026-11-27 11:03-11:11 CT, Thrust-mockup.html, "build") + Thrust / PIC on the Range read, VolTH / Change on the Volume read (14 candles), gold, above BRMax / BVMax
// was static const char* const VERSION = "1.17.0";   // (1.17.0, Rassul 2026-11-27 09:50-10:00 CT "consolidate AR and Vob (t) into a new BI (Break In) signal") the price chart's BI ("t|BI|dir|lv|px|st") is marked "BI" on BOTH panes (in the AR's place, first after CB); the file's Sw (VOB / VOT) and AR lines are no longer marked
// was static const char* const VERSION = "1.16.0";   // (1.16.0, Rassul 2026-11-25 14:28-14:29 CT "relabel sweep to VOB (volume off the bottom) and VOT (Volume off the top)") the Sw code over a bar shows as VOB (bullish) / VOT (bearish)
// was static const char* const VERSION = "1.15.0";   // (1.15.0, Rassul 2026-11-24 21:23-22:39 CT "everything else is a go" on reads-conclude-mockup.html v30) the Volume read concludes: title = the overall call of the three panes (lsTurn vread + lsRevRead bsv files), effort vs result with price agreement, large orders, who led, sliced / pulled (VRRead 1.6.0)
// was static const char* const VERSION = "1.14.0";   // (1.14.0, Rassul 2026-11-24 20:46 / 20:54) the Volume read: a grey column with the volume-model question of each line (VRRead 1.5.0); full questions when the read has room, short ones when narrow, none under 150 px
// was static const char* const VERSION = "1.13.0";   // (1.13.0, Rassul 2026-11-24 20:00 CT "Go") the Volume read's biggest orders with their Big Lot tier (VRRead 1.4.0, BigTier.h): Engine.mkt, Ctx.mkt / rth
// was static const char* const VERSION = "1.12.0";   // (1.12.0, Rassul 2026-11-24 19:44 CT "Go" on panes-read-mockup.html) the Volume read = how much / the biggest orders and sweeps / icebergs / a neutral note, no Bullish / Bearish verdict (VRRead 1.3.0 volPlain); bars and the Range read unchanged
// was static const char* const VERSION = "1.11.0";   // (1.11.0, Rassul 2026-11-23 09:25-09:34 CT "i want you to implement the AR which is an automatic rally bar that occurs after a cx ... has a BRMax of at least 10", "the AR bar should also have a strength line", "go"; mockup ar-mockup.html): the Range pane shows AR on the candle the price chart marked AR (the AR lines of the same marks file, lsRevRead 0.23.0), with its number from 10 up like a Bk / Sw bar; the Volume pane shows no AR (it is a range bar); a Bk / Sw on the same candle keeps the label   // was "1.10.0";   // (1.10.0, Rassul 2026-11-23 07:26 CT "are the rules for the cx badge and the cx label on the volume different", 07:32 "go with badge for both", 07:33 "why is cx on the range ?"; mockup cx-badge-label-mockup.html): the Volume pane no longer works out Cx itself - it shows Cx exactly on the candles the price chart marked (the Cx lines of lsRevRead's wy-<MKT>-<secs>.txt, as Bk / Sw since 1.8.0), so badge and label can never disagree; no file = no Cx. The Range pane shows no Cx at all (Cx = climactic VOLUME; its own "biggest range at a pivot" carried the same letters). The pane's own pivot rule is gone (it passed a candle that was only the biggest of its own colour in 20+: NQ 2026-11-23 6:21 AM "Cx 31" without a badge, 6:48 AM "Cx 41" where the badge sat on 6:45). The Max numbers, Bk / Sw, CB / SR / Ct, the bars' colours and the read are unchanged; the read's "Climax volume at the high / low" sentence follows the file's Cx too (Volume pane)   // was "1.9.0";   // (1.9.0, Rassul 2026-11-19 18:31-18:35 CT "those info boxes for the range and vol are missing. add them back", "ok but color so it looks nice. build"): the numbers (Max / consecutive / green / red) back next to the 3-line read, coloured (grey labels, the Max gold from 10, green / red rows); dropped when there is no room   // was "1.8.0";   // (1.8.0, Rassul 2026-11-19 18:18 CT "ok" + 18:19 "switch to 10": the panes no longer work out Bk / Sw themselves - they show Bk / Sw exactly on the candles the price chart marked them (lsRevRead's wy-<MKT>-<secs>.txt, the same file lsRevReadMarks draws from), so the panes and the price chart can never disagree; no file = no Bk / Sw. A Bk / Sw bar shows its Max count from 10 up (other bars: from 20 up as before). A Bk / Sw wins over an SR on the same bar)   // was "1.7.0"   // (1.7.0, Rassul 2026-11-19 16:54-16:58 CT "1. ok": the panes' Bk / Sw on the price chart's rules (LevelMarks 1.3.0): a Bk bar's range >= the prior bar's and range or volume up (the BO candle rule); a Sw bar is green for a swept low, red for a swept high; DPoc / WPoc / Open are not levels)
// was VERSION = "1.6.0";   // (1.6.0, 2026-11-17 21:17 CT) the read = 3 plain lines (who is winning / why / what to watch, VRRead.h 1.2.0 toPlain), no numbers table   // was "1.5.3"   // (1.5.3, 2026-11-17 14:25 CT) the verdict starts with the candle time ("2:21 PM Quiet", "Now" for the forming candle)   // was "1.5.2"   // (1.5.2, his 11:18 CT screenshot) the verdict wraps; tick decimals fixed (HG 0.011000 -> 0.0110); range size words; reason names   // was "1.5.1"   // (1.5.1, 2026-11-17 09:32 CT screenshot: the mouse right of the last candle reads the last closed candle; the sentences behind the verdict first)   // was "1.5.0"   // (1.5.0, 2026-11-17 Rassul 08:35-08:44 CT: the split read - verdict + plain-English sentences on the left, the numbers on the right)   // was "1.4.0"   // (1.4.0, 2026-11-17 Rassul 06:54-07:02 CT: the pane reads right of the last candle - verdict / candle / two behaviours + the numbers, for the candle under the mouse; Volume looks inside the candle through lsBookFlow's record; VRRead.h)   // was "1.3.5";   // (1.3.5, 2026-11-16 21:17 CT Rassul "lets not color the volume or range based on just being a rev bar . volume and range should only be colored because the volume or range is significaant": the Rev mark is gone)   // was "1.3.4";   // (1.3.4, 2026-11-16 21:13 CT Rassul "use Cx instead of P, to represent climactic volume or a minor climax": the pivot mark reads Cx)   // was "1.3.3";   // (1.3.3, 2026-11-16 12:53 CT Rassul "the color of the bars must match the color of the bars": a marked bar is drawn in its candle's colour - a red candle is red even when its mark is bullish)   // was "1.3.2";   // (1.3.2, 2026-11-16 12:52 CT Rassul "dont keep the range and volume versions in the box. we have limited verstical space": the read box has no title row - its four rows only)   // was "1.3.1";   // (1.3.1, 2026-11-16 10:58 CT: the read box sits right of the last candle when there is room; the bars then use the whole pane)   // was "1.3.0";   // (1.3.0, 2026-11-16 05:55-06:18 CT Rassul "make sure to highlight it if it has a notable high value on a pivot as well as a reversal , and especially if it is a cob bar near the origin",
// "what about an increas on a break/sweep of a keylevel, vol level", "go", "if there is a bar that is big like biggest range or volume in 50+ , lets highlight it, this includes variants like biggest green or red";
// mockup Dr2SDbU3idUeUYD5c2KAVX) BOTH panes: bars grey, a marked bar green (bullish) / red (bearish) with ONE short code over it, by priority:
//   CB  the Cob bar near the origin (the Cob Reader's CB bar)                      colour = the Cob's side
//   SR  the stop-run bar into the Cob origin (the Cob Reader's SR bar)             colour = the Cob's side
//   Bk  a bar that CLOSES beyond a key / volume / 3-hour S-R level, biggest in 5+  colour = the break's direction
//   Sw  a bar that trades through such a level and closes back, biggest in 5+      colour = against the sweep (a swept high = red)
//   Rev a PB Rev / CoB pullback Rev bar (as 1.1.0, now in the Volume pane too)     colour = the Rev's side
//   Ct  after a Cob: a counter-colour bar bigger than every bar since the origin   colour = against the Cob
//   P   a ZigZag (3-bar High / Low) pivot bar - or the bar before it - biggest in 20+   pivot high = red, pivot low = green
//   (no code) a bar that is the biggest in 50+ bars (all bars or its own colour)   colour = the candle's;  Range only: BO >= 1.2 x ATR (as 1.1.0)
// Every mark is decided from closed bars and the records as they stood, and never changes afterwards.
// was "1.2.0";   // (1.2.0, 2026-11-16 05:49 CT Rassul "the values in the infobox should change as my mouse hovers on different bars in the pane": layout(..., pick) - the read box reads the bar under the mouse (pick = its index; the forming bar = the bar count), the last closed bar when the mouse is not in the pane)   // was "1.1.1";   // (1.1.1, 2026-11-15 independent review: lsBreakout's record files are found by lsBreakout's own file key (boMarket / boFileKey); an unreadable bar no longer inflates the ATR)   // was "1.1.0";   // (1.1.0, 2026-11-15 20:38-20:47 CT Rassul "for the range indicator , when there is a bo that has a range of 1.2*ATR or more as well
// as a pb rev bar ... i want it highlighted in green or red ddepending on whether it is bullish or bearish", "for PB rev do it whenever there is a rev signal also", mockup
// FNJNiY1fR9JBJsU18aZvg4 v2, "build") RANGE pane: bars grey; a BO / BD candle (lsBreakout's record) whose range is >= 1.2 x ATR(13) of the bar before, every PB Rev bar
// (lsBreakout's record) and every CoB pullback Rev bar (lsCobSignal's record) in green (bullish) / red (bearish). A Rev wins over a BO on the same bar. Volume pane unchanged.
static const double HL_ATR_X = 1.2; static const int HL_ATR_N = 13;
enum HlKind { HL_BO = 1, HL_PBREV = 2, HL_COBREV = 3 };
struct Hl { int dir = 0, kind = 0; };
inline std::vector<std::string> hlSplit(const std::string& l, char sep, size_t maxN) {
    std::vector<std::string> f; size_t p = 0;
    while (p <= l.size() && f.size() < maxN) { size_t q = l.find(sep, p); if (q == std::string::npos) q = l.size(); f.push_back(l.substr(p, q - p)); p = q + 1; }
    return f;
}
inline bool hlNum(const std::string& s, long long& v) { if (s.empty()) return false; char* e = nullptr; v = std::strtoll(s.c_str(), &e, 10); return e && *e == 0; }
template <class F> inline void hlLines(const std::string& text, F fn) {
    size_t a = 0;
    while (a < text.size()) { size_t b = text.find('\n', a); if (b == std::string::npos) b = text.size(); std::string l = text.substr(a, b - a); a = b + 1; if (!l.empty() && l.back() == '\r') l.pop_back(); fn(l); }
}
// "BBO|t|t0|dir|fired|..." (lsBreakout breakouts file): a FIRED breakout -> the BO candle t0 (chart time as IRT stamps it)
inline void parseBo(const std::string& text, std::map<long long, Hl>& out) {
    hlLines(text, [&](const std::string& l) {
        if (l.rfind("BBO|", 0) != 0) return;
        const std::vector<std::string> f = hlSplit(l, '|', 6); long long t0 = 0, d = 0;
        if (f.size() < 5 || f[4] != "1" || !hlNum(f[2], t0) || t0 <= 0 || !hlNum(f[3], d) || (d != 1 && d != -1)) return;
        if (!out.count(t0)) { Hl h; h.dir = (int)d; h.kind = HL_BO; out[t0] = h; }
    });
}
// "BRV|t_rev|side|..." (lsBreakout pb file): every PB Rev bar
inline void parsePbRev(const std::string& text, std::map<long long, Hl>& out) {
    hlLines(text, [&](const std::string& l) {
        if (l.rfind("BRV|", 0) != 0) return;
        const std::vector<std::string> f = hlSplit(l, '|', 4); long long t = 0, d = 0;
        if (f.size() < 3 || !hlNum(f[1], t) || t <= 0 || !hlNum(f[2], d) || (d != 1 && d != -1)) return;
        Hl h; h.dir = (int)d; h.kind = HL_PBREV; out[t] = h;
    });
}
// "REV,<bar end, local secs>,BULL|BEAR,..." (lsCobSignal record): every CoB pullback Rev bar, keyed by LOCAL seconds
inline void parseCobRev(const std::string& text, std::map<long long, Hl>& out) {
    hlLines(text, [&](const std::string& l) {
        if (l.rfind("REV,", 0) != 0) return;
        const std::vector<std::string> f = hlSplit(l, ',', 4); long long t = 0;
        if (f.size() < 3 || !hlNum(f[1], t) || t <= 0 || (f[2] != "BULL" && f[2] != "BEAR")) return;
        Hl h; h.dir = f[2] == "BULL" ? 1 : -1; h.kind = HL_COBREV; out[t] = h;
    });
}
static const int CAP = 99;          // counts are shown as "99+" from here
static const int SHOW_MIN = 20;     // the Max number goes on a bar from this count
static const int SHOW_MIN_LV = 10;  // (1.8.0, Rassul 18:19 CT "switch to 10") ... and on a Bk / Sw bar from this count
static const int LV_MIN = 5, PV_MIN = 20, BIG_MIN = 50, LV_COOL = 10, CT_BACK = 40, CT_FWD = 60;   // (1.3.0) the mark thresholds
enum MkCode { MK_NONE = 0, MK_BO, MK_BIG, MK_P, MK_CT, MK_REV, MK_AR, MK_SW, MK_BK, MK_SR, MK_CB };   // (1.11.0) MK_AR; MK_CB stays the last code (arrays are sized MK_CB + 1)
struct Mk { int code = 0, dir = 0; };
inline const char* mkText(int c) { switch (c) { case MK_P: return "Cx"; case MK_CT: return "Ct"; case MK_REV: return "Rev"; case MK_AR: return "BI"; case MK_SW: return "Sw"; case MK_BK: return "Bk"; case MK_SR: return "SR"; case MK_CB: return "CB"; default: return ""; } }
struct CobIt { long long t = 0, cbT = 0, srT = 0; int side = 0; };
inline long long lvFloorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
inline long long lvCivil(long long y, unsigned m, unsigned d) { y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; return era * 146097 + (long long)doe - 719468; }
inline long long sessionDay(long long localT) { return lvFloorDiv(localT + 7 * 3600 - 1, 86400); }       // the session starts 17:00 CT: its RTH date as civil days
inline bool lvNameOk(const std::string& name) {                                                          // not the prior-hour levels (they move every hour) and not a VWAP
    bool any = false; size_t a = 0;
    while (a <= name.size()) { size_t b = name.find('/', a); if (b == std::string::npos) b = name.size(); const std::string t = name.substr(a, b - a); a = b + 1;
        const bool skip = t.empty() || t == "PHH" || t == "PHL" || t == "PhrH" || t == "PhrL" || t.compare(0, 4, "VWAP") == 0 || t == "DPoc" || t == "WPoc" || t == "Open";   // (1.7.0) no POC / Open levels
        if (!skip) any = true; }
    return any;
}
inline bool lvPx(const std::string& s, double& v) { if (s.empty()) return false; char* e = nullptr; v = std::strtod(s.c_str(), &e); return e && *e == 0 && std::isfinite(v) && v > 0; }
// "YYYY-MM-DD|unix|name=px;name=px..." (lsLimitProfile's LRA-KeyLevels-<MKT>-hist.csv, one line per session)
inline void parseLvHist(const std::string& text, std::map<long long, std::vector<double>>& out) {
    hlLines(text, [&](const std::string& l) {
        const std::vector<std::string> f = hlSplit(l, '|', 4); if (f.size() != 3 || f[0].size() != 10 || f[0][4] != '-' || f[0][7] != '-') return;
        long long y = 0, m = 0, d = 0; if (!hlNum(f[0].substr(0, 4), y) || !hlNum(f[0].substr(5, 2), m) || !hlNum(f[0].substr(8, 2), d) || m < 1 || m > 12 || d < 1 || d > 31) return;
        std::vector<double> v;
        for (const std::string& kv : hlSplit(f[2], ';', 64)) { const size_t q = kv.find('='); double px = 0; if (q == std::string::npos || !lvNameOk(kv.substr(0, q)) || !lvPx(kv.substr(q + 1), px)) continue; v.push_back(px); }
        if (!v.empty()) out[lvCivil(y, (unsigned)m, (unsigned)d)] = v;
    });
}
// "KEYLV,1,<unix>" then "L|name|px|pivot" (lsLimitProfile's LRA-KeyLevels-<MKT>.csv, rewritten while its 3-min chart runs)
inline bool parseLvLive(const std::string& text, long long& at, std::vector<double>& out) {
    out.clear(); at = 0; bool head = false, bad = false;
    hlLines(text, [&](const std::string& l) {
        if (bad) return;
        if (!head) { if (l.compare(0, 8, "KEYLV,1,") != 0 || !hlNum(l.substr(8), at) || at <= 0) { bad = true; return; } head = true; return; }
        const std::vector<std::string> f = hlSplit(l, '|', 5); double px = 0; if (f.size() < 4 || f[0] != "L" || !lvNameOk(f[1]) || !lvPx(f[2], px)) return;
        out.push_back(px);
    });
    if (bad || !head) { out.clear(); return false; }
    return true;
}
// "CK,<t>,BULL|BEAR,...,CB <t> <rank>,SR <t>" (the Cob Reader pane's status, lsZzoCombo 3.0.0 / CobCheck.h)
inline void parseCk(const std::string& text, std::vector<CobIt>& out) {
    hlLines(text, [&](const std::string& l) {
        if (l.rfind("CK,", 0) != 0) return;
        const std::vector<std::string> f = hlSplit(l, ',', 4); CobIt x; if (f.size() < 3 || !hlNum(f[1], x.t) || x.t <= 0 || (f[2] != "BULL" && f[2] != "BEAR")) return;
        x.side = f[2] == "BULL" ? 1 : -1;
        const size_t c = l.rfind(",CB "), r = l.rfind(",SR "); if (c == std::string::npos || r == std::string::npos || r < c) return;
        char* e = nullptr; x.cbT = std::strtoll(l.c_str() + c + 4, &e, 10); if (!e || *e != ' ') return;
        x.srT = std::strtoll(l.c_str() + r + 4, &e, 10); if (!e || *e != 0 || x.cbT < 0 || x.srT < 0) return;
        out.push_back(x);
    });
}
// (1.8.0) "t|Bk|dir|lv|px|st" / "t|Sw|dir|..." (lsRevRead 0.20.0 wy-<MKT>-<secs>.txt): the price chart's Bk / Sw candles by chart time; every other line ignored
// (1.10.0) + "t|Cx|dir|lv|px|" (lv may be empty): the price chart's Cx candles (MK_P; dir +1 = at a pivot low, -1 = at a pivot high). The price chart puts ONE of
// Sw / Bk / Cx on a candle; should a file ever carry two, a Bk / Sw is never replaced by a Cx. Returns the number of Bk / Sw lines (as before); Cx lines are not counted in it.
// (1.11.0) + "t|AR|dir||0|st": the price chart's AR candles go to *ar (candle -> +1 green / -1 red) when it is given - kept apart from out because an AR may share its
// candle with a Bk / Sw / Cx; not counted either.
inline int parseWy(const std::string& text, std::map<long long, Mk>& out, std::map<long long, int>* ar = nullptr) {
    if (text.compare(0, 5, "WY|1|") != 0) return -1;
    int n = 0;
    hlLines(text, [&](const std::string& l) {
        const std::vector<std::string> f = hlSplit(l, '|', 4); long long t = 0, d = 0;
        if (f.size() >= 2 && (f[1] == "Sw" || f[1] == "AR")) return;   // (1.17.0) VOB / VOT and AR: replaced by the BI
        const bool cx = f.size() >= 2 && f[1] == "Cx", isAr = f.size() >= 2 && f[1] == "BI";   // (1.17.0) the BI takes the AR's slot
        if (f.size() < 3 || (f[1] != "Bk" && f[1] != "Sw" && !cx && !isAr) || !hlNum(f[0], t) || t <= 0 || !hlNum(f[2], d) || (d != 1 && d != -1)) return;
        if (isAr) { if (ar) (*ar)[t] = (int)d; return; }
        if (cx) { if (!out.count(t)) { Mk m; m.code = MK_P; m.dir = (int)d; out[t] = m; } return; }
        Mk m; m.code = f[1] == "Bk" ? MK_BK : MK_SW; m.dir = (int)d; out[t] = m; ++n;
    });
    return n;
}
static const bool LV_FROM_WY = true;   // (1.8.0)
static const bool CX_FROM_WY = true;   // (1.10.0) the Volume pane's Cx = the price chart's Cx (the same file); the Range pane has none
static const int LOOK = 1500;       // bars looked back at most (a count can only reach CAP)
enum Mode { VOL = 0, RANGE = 1 };

struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0, v = 0; };
struct Stat { int all = 0, same = 0; bool green = false; int mx() const { return std::max(all, same); } };
inline std::string fm(int n) { return n >= CAP ? std::string("99+") : std::to_string(n); }

// (1.1.1) lsBreakout's record file name parts, the same rules as BreakoutLogic.h bko::recFileKey / bko::recMarket (tests_volrange compares them)
inline std::string boFileKey(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) { const unsigned char c = (unsigned char)s[i];
        out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_'); }
    return out.empty() ? "UNKNOWN" : out;
}
inline std::string boMarket(std::string r) {
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return boFileKey(r.substr(0, 8));
}
struct Engine {
    int mode = VOL; double tick = 0.25; std::string mkt;   // (1.13.0) the market for the Big Lot tier (the host sets it)
    std::vector<double> V; std::vector<char> G; std::vector<long long> T; std::vector<Stat> S;   // closed bars only
    // (1.1.0) the highlight: per closed bar its price range, Wilder ATR(13) and its local time; the signal bars come from the host (files)
    std::vector<double> PR, ATR; std::vector<long long> LT; double prevC = NAN, trSum = 0; bool hlOn = false;
    std::map<long long, Hl> hlRaw, hlLocal;            // by chart time (as T) / by local seconds (as LT)
    // (1.3.0) the marks: per closed bar its high / low / close, the pivot mark, the level mark and the 3-hour levels standing BEFORE it
    int spb = 0; std::vector<double> Hh, Ll, Cc; std::vector<signed char> lvC, lvD; std::vector<double> htf;   // htf: 4 per bar (NAN = none)   // (1.10.0) no pv: the pane has no pivot rule of its own any more
    hsr::Walk hw; hsr::Snap hsn;
    std::map<long long, std::vector<double>> lvDay; int lvDone = 0; std::map<long long, int> lvLast;
    std::map<long long, tvr::Row> tv; std::map<long long, std::pair<int, int> > bsv; int tvRows = -1, bsvRows = -1;   // (1.15.0) lsTurn's word on each closed candle (TurnPane\\vread) and lsRevRead's buy / sell verdict + trapped side (RevRead\\bsv), by chart time; -1 = no file
    std::vector<double> Oo, Vol; std::map<long long, vrd::Inside> inside;   // (1.4.0) per closed bar its open and volume; lsBookFlow's per-candle order facts by chart time (the host fills it)
    std::vector<CobIt> cobs; std::map<long long, int> cbAt, srAt, ctAt; int cobN = -1;                    // by bar time (local seconds or chart time)
    std::map<long long, Mk> wy; int wyLines = -1;                                                         // (1.8.0) the price chart's Bk / Sw by chart time (-1 = no file); (1.10.0) and its Cx (MK_P)
    int cxAt(int i) const { if (!CX_FROM_WY || mode != VOL || i < 0 || i >= (int)T.size()) return 0; const auto w = wy.find(T[(size_t)i]); return w != wy.end() && w->second.code == MK_P ? w->second.dir : 0; }   // (1.10.0) the price chart's Cx on closed bar i: +1 at a pivot low, -1 at a pivot high, 0 none (always 0 in the Range pane)
    int wyCx() const { int k = 0; for (const auto& kv : wy) if (kv.second.code == MK_P) ++k; return k; }
    std::map<long long, int> wyAr;                                                                        // (1.11.0) the price chart's AR candles by chart time: +1 green / -1 red
    int arAt(int i) const { if (i < 0 || i >= (int)T.size()) return 0; const auto w = wyAr.find(T[(size_t)i]); return w != wyAr.end() ? w->second : 0; }   // (1.11.0) the AR on closed bar i - the Range pane only (BRMax 10+ made it)
    void reset() { Oo.clear(); Vol.clear(); V.clear(); G.clear(); T.clear(); S.clear(); PR.clear(); ATR.clear(); LT.clear(); prevC = NAN; trSum = 0;
        Hh.clear(); Ll.clear(); Cc.clear(); lvC.clear(); lvD.clear(); htf.clear(); hw.reset(); hsn = hsr::Snap(); lvDone = 0; lvLast.clear(); cbAt.clear(); srAt.clear(); ctAt.clear(); cobN = -1; }
    bool deadBar(int i) const { return Hh[(size_t)i] == 0 && Ll[(size_t)i] == 0 && Cc[(size_t)i] == 0; }
    // (1.10.0) the pane's own ZigZag pivot rule (zigStep, 1.3.0: a confirmed pivot bar, or the bar before, biggest in 20+ of all bars OR of its own colour) is gone:
    // Cx comes from the price chart's file (cxAt), whose rule is the biggest volume of ALL the 20 candles before (LevelMarks.h)
    // the level marks of the bars not yet looked at, with the levels as they stand now (the host calls it after it read the level files)
    void markLevels() {
        const int n = (int)V.size();
        if (LV_FROM_WY) { lvDone = n; return; }   // (1.8.0) the Bk / Sw come from the price chart's marks (wyAt), not from the levels here
        for (int i = std::max(1, lvDone); i < n; ++i) {
            if (deadBar(i) || deadBar(i - 1) || S[(size_t)i].mx() < LV_MIN || tick <= 0) continue;
            const double pc = Cc[(size_t)i - 1], h = Hh[(size_t)i], l = Ll[(size_t)i], c = Cc[(size_t)i]; int code = 0, dir = 0;
            auto one = [&](double p) {
                if (!std::isfinite(p) || p <= 0) return;
                int cd = 0, d = 0;
                if (pc < p && h > p) { if (c > p) { cd = MK_BK; d = 1; } else { cd = MK_SW; d = -1; } }
                else if (pc > p && l < p) { if (c < p) { cd = MK_BK; d = -1; } else { cd = MK_SW; d = 1; } }
                if (!cd) return;
                if (cd == MK_BK) { const double r0 = h - l, r1 = Hh[(size_t)i - 1] - Ll[(size_t)i - 1]; if (!(r0 >= r1 && (r0 > r1 || V[(size_t)i] > V[(size_t)i - 1]))) return; }   // (1.7.0) the BO candle rule
                if (cd == MK_SW && (d > 0) != (G[(size_t)i] != 0)) return;                                                                                                 // (1.7.0) a Sw bar has the rejection's colour
                const long long key = std::llround(p / tick); const auto f = lvLast.find(key); const bool cool = f != lvLast.end() && i - f->second < LV_COOL; lvLast[key] = i;
                if (cool) return;
                if (!code || (cd == MK_BK && code == MK_SW)) { code = cd; dir = d; }
            };
            if (i < (int)LT.size()) { const auto f = lvDay.find(sessionDay(LT[(size_t)i])); if (f != lvDay.end()) for (double p : f->second) one(p); }
            for (int k = 0; k < 4; ++k) one(htf[(size_t)i * 4 + (size_t)k]);
            lvC[(size_t)i] = (signed char)code; lvD[(size_t)i] = (signed char)dir;
        }
        lvDone = n;
    }
    int indexOfTime(long long t) const {
        if (t <= 0) return -1;
        auto a = std::lower_bound(LT.begin(), LT.end(), t); if (a != LT.end() && *a == t) return (int)(a - LT.begin());
        auto b = std::lower_bound(T.begin(), T.end(), t); if (b != T.end() && *b == t) return (int)(b - T.begin());
        return -1;
    }
    int byTime(const std::map<long long, int>& m, int i) const {
        if (m.empty()) return 0;
        if (i < (int)LT.size()) { const auto a = m.find(LT[(size_t)i]); if (a != m.end()) return a->second; }
        const auto b = m.find(T[(size_t)i]); return b != m.end() ? b->second : 0;
    }
    // the Cob marks (CB / SR bars from the Cob Reader, the counter bars found here); marks are only ever added
    void setCobs(const std::vector<CobIt>& c) { cobs = c; cobN = -1; }
    void cobMarks() {
        const int n = (int)V.size(); if (n == cobN) return; cobN = n;
        for (const CobIt& x : cobs) {
            if (x.cbT > 0 && !cbAt.count(x.cbT)) cbAt[x.cbT] = x.side;
            if (x.srT > 0 && !srAt.count(x.srT)) srAt[x.srT] = x.side;
            const int at = indexOfTime(x.t); if (at < 1) continue;
            int org = at; for (int j = at; j >= std::max(0, at - CT_BACK); --j) { if (deadBar(j)) continue; if (x.side > 0 ? Ll[(size_t)j] < Ll[(size_t)org] : Hh[(size_t)j] > Hh[(size_t)org]) org = j; }
            double mxv = 0; for (int j = org + 1; j <= at && j < n; ++j) mxv = std::max(mxv, V[(size_t)j]);
            for (int j = at + 1; j < n && j <= at + CT_FWD; ++j) {
                if (deadBar(j)) continue;
                if (x.side > 0 ? Ll[(size_t)j] < Ll[(size_t)org] : Hh[(size_t)j] > Hh[(size_t)org]) break;          // the origin is gone
                const bool counter = (G[(size_t)j] != 0) != (x.side > 0);
                if (counter && V[(size_t)j] > mxv && mxv > 0) { const long long key = j < (int)LT.size() && LT[(size_t)j] > 0 ? LT[(size_t)j] : T[(size_t)j]; if (!ctAt.count(key)) ctAt[key] = -x.side; }
                mxv = std::max(mxv, V[(size_t)j]);
            }
        }
    }
    int revDir(int i) const {
        int rev = 0;
        const auto a = hlRaw.find(T[(size_t)i]); if (a != hlRaw.end() && a->second.kind != HL_BO) rev = a->second.dir;
        if (!rev && i < (int)LT.size()) { const auto b = hlLocal.find(LT[(size_t)i]); if (b != hlLocal.end() && b->second.kind != HL_BO) rev = b->second.dir; }
        return rev;
    }
    // the ONE mark of closed bar i (code + direction: +1 green, -1 red), by priority
    Mk markOf(int i) const {
        Mk m; if (!hlOn || i < 0 || i >= (int)T.size()) return m;
        int d = 0;
        if ((d = byTime(cbAt, i)) != 0) { m.code = MK_CB; m.dir = d; return m; }
        if ((d = arAt(i)) != 0) { m.code = MK_AR; m.dir = d; return m; }                                 // (1.17.0) the price chart's BI, both panes, before Bk / Cx
        if (LV_FROM_WY) { const auto w = wy.find(T[(size_t)i]); if (w != wy.end() && w->second.code != MK_P) return w->second; }   // (1.8.0) the price chart's Bk / Sw (wins over an SR on the same bar); (1.10.0) its Cx keeps the Cx place below
        if ((d = byTime(srAt, i)) != 0) { m.code = MK_SR; m.dir = d; return m; }
        if (!LV_FROM_WY && i < (int)lvC.size() && lvC[(size_t)i]) { m.code = lvC[(size_t)i]; m.dir = lvD[(size_t)i]; return m; }
        /* (1.3.5) no Rev mark: a bar is coloured only because its volume / range is significant */
        if ((d = byTime(ctAt, i)) != 0) { m.code = MK_CT; m.dir = d; return m; }
        if ((d = cxAt(i)) != 0) { m.code = MK_P; m.dir = d; return m; }                                  // (1.10.0) the price chart's Cx (Volume pane only)
        if (S[(size_t)i].mx() >= BIG_MIN) { m.code = MK_BIG; m.dir = S[(size_t)i].green ? 1 : -1; return m; }
        if (mode == RANGE && (d = highlight(i)) != 0) { m.code = MK_BO; m.dir = d; return m; }
        return m;
    }
    // +1 green, -1 red, 0 grey for closed bar i: a Rev (PB Rev or CoB Rev) always; a BO / BD only with range >= HL_ATR_X x ATR(13) of the bar before
    int highlight(int i) const {
        if (!hlOn || i < 0 || i >= (int)T.size()) return 0;
        int bo = 0, rev = 0;
        const auto a = hlRaw.find(T[(size_t)i]); if (a != hlRaw.end()) { if (a->second.kind == HL_BO) bo = a->second.dir; else rev = a->second.dir; }
        if (i < (int)LT.size()) { const auto b = hlLocal.find(LT[(size_t)i]); if (b != hlLocal.end()) { if (b->second.kind == HL_BO) { if (!bo) bo = b->second.dir; } else if (!rev) rev = b->second.dir; } }
        (void)rev;   // (1.3.5) a Rev alone no longer colours a bar
        if (bo && i >= 1 && ATR[(size_t)i - 1] > 0 && PR[(size_t)i] >= HL_ATR_X * ATR[(size_t)i - 1] - 1e-12) return bo;
        return 0;
    }
    double valueOf(const Bar& b) const {
        if (mode == VOL) return std::isfinite(b.v) && b.v > 0 ? b.v : 0.0;
        const double r = b.h - b.l; if (!std::isfinite(r) || r <= 0) return 0.0;
        return tick > 0 ? (double)std::llround(r / tick) : r;                    // whole ticks: equal ranges compare equal
    }
    // the counts of a bar with value x and colour g against the closed bars before index n
    Stat statFor(double x, bool g, int n) const {
        Stat s; s.green = g; if (!(x > 0)) return s;
        bool da = false, ds = false; const int lo = std::max(0, n - LOOK);
        for (int j = n - 1; j >= lo && !(da && ds); --j) {
            const bool big = V[(size_t)j] >= x;
            if (!da) { if (big || s.all >= CAP) da = true; else ++s.all; }
            if (!ds && (G[(size_t)j] != 0) == g) { if (big || s.same >= CAP) ds = true; else ++s.same; }
        }
        return s;
    }
    void addBar(const Bar& b, long long localT = 0) { const double x = valueOf(b); const bool g = b.c >= b.o; S.push_back(statFor(x, g, (int)V.size())); V.push_back(x); G.push_back(g ? 1 : 0); T.push_back(b.t);
        const bool dead = b.h == 0 && b.l == 0 && b.c == 0;            // (1.1.1) an unreadable bar (the host zeroes it): no true range, the last real close stays
        const double r = std::isfinite(b.h - b.l) && b.h > b.l ? b.h - b.l : 0.0; const double tr = dead ? 0.0 : std::isfinite(prevC) ? std::max(r, std::max(std::fabs(b.h - prevC), std::fabs(b.l - prevC))) : r;
        const int n = (int)PR.size(); double a = 0;                    // Wilder: the mean of the first HL_ATR_N true ranges, then (prev * (N - 1) + TR) / N
        if (n < HL_ATR_N) { trSum += tr; a = n == HL_ATR_N - 1 ? trSum / HL_ATR_N : 0; } else a = (ATR[(size_t)n - 1] * (HL_ATR_N - 1) + tr) / HL_ATR_N;
        PR.push_back(r); ATR.push_back(a); LT.push_back(localT); if (!dead && std::isfinite(b.c)) prevC = b.c;
        Oo.push_back(b.o); Vol.push_back(b.v);   // (1.4.0)
        Hh.push_back(b.h); Ll.push_back(b.l); Cc.push_back(b.c); lvC.push_back(0); lvD.push_back(0);                 // (1.3.0)
        { double q[4] = { NAN, NAN, NAN, NAN }; if (tick > 0 && spb > 0 && spb < hsr::SECS && hsr::periodOk(spb)) { int k = 0; for (const hsr::Lv& x : hsr::standing(hsn, tick)) if (k < 4) q[k++] = x.px;
              if (!dead && localT > 0) { hsr::Bar hb; hb.t = localT; hb.h = b.h; hb.l = b.l; hb.c = b.c; const hsr::Snap s2 = hw.add(hb, spb); if (s2.dir != 0) hsn = s2; } }
          htf.insert(htf.end(), q, q + 4); } }
    Stat forming(const Bar& b) const { return statFor(valueOf(b), b.c >= b.o, (int)V.size()); }
};

// ---- the drawing (SDK-free)
struct View { int readX = -1;   /* (1.3.1) the x just right of the last candle when it is on screen: the read box goes there; -1 = the left edge */ int l = 0, t = 0, r = 800, b = 100, first = 0, last = 0, bw = 6, textH = 11; std::function<double(int)> X; std::function<int(const std::string&)> textW; };
struct Prim { int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned col = 0; std::string text; bool fill = false, bold = false; };
static const unsigned C_GREY = 0x006B6B6B;
static const unsigned C_UP = 0x0022C55E, C_DN = 0x00EF4444, C_TXT = 0x00FFFFFF, C_BOXBG = 0x000D0D0D, C_BOXLN = 0x00888888;   // 0x00RRGGBB
static const int BOX_W = 92, BOX_PAD = 3;
static const unsigned C_LABEL = 0x0094A3B8, C_GOLD = 0x00F0C040;   // (1.9.0) the numbers column: grey labels, a Max of 10+ in gold

// E = the closed bars; form / haveForm = the forming bar (drawn as a bar, never numbered); v.first / v.last = visible bar indexes
// (the forming bar's index = E.V.size())
// (1.18.0, Rassul 2026-11-27 11:03-11:11 CT "a stat that measures the HLR/ATR ... called thrust ... in the range infobox above BRMAX", "also add this for
// volume which is VO/VMA ... above BVMAX", "up to 2 decimals", "next to thrust i also want a close value (CL-CL.1) / avg difference in closes", "it needs
// to be ABS value", "lets add this for vol as well abs(vo-vo.1) / average differences in vol", "PIC for progress in the close", "VolTH for vol Thrust",
// "instead of 20, lets use 14 for both the range and vol. build"; mockup Thrust-mockup.html). Candle k = closed bar index (k == n: the forming bar f):
//   Thrust = (high - low) / ATR(14)              ATR = the average true range of the 14 candles BEFORE k
//   PIC    = |close - prior close| / the average |close - prior close| of the 14 candles before k   (progress in close)
//   VolTH  = volume / VMA(14)                    VMA = the average volume of the 14 candles before k
//   Change = |volume - prior volume| / the average |volume - prior volume| of the 14 candles before k
// Dead (empty) candles are skipped; fewer than 14 usable candles before k = "-". Text "1.72x" (2 decimals).
static const int TH_N = 14;
struct ThStat { double a = NAN, b = NAN; };   // Range: a = Thrust, b = PIC; Volume: a = VolTH, b = Change
inline ThStat thStat(const Engine& E, int k, const Bar* f) {
    ThStat o; const int n = (int)E.Cc.size(); if (k < 0 || k > n || (k == n && !f) || (int)E.Vol.size() != n) return o;
    const double h = k == n ? f->h : E.Hh[(size_t)k], l = k == n ? f->l : E.Ll[(size_t)k], c = k == n ? f->c : E.Cc[(size_t)k], vo = k == n ? f->v : E.Vol[(size_t)k];
    std::vector<int> J; for (int j = k - 1; j >= 1 && (int)J.size() < TH_N + 1; --j) if (!E.deadBar(j)) J.push_back(j);   // newest first: J[0] = the prior candle
    if ((int)J.size() < TH_N + 1) return o;
    double tr = 0, dc = 0, vm = 0, dv = 0;
    for (int q = 0; q < TH_N; ++q) { const int j = J[(size_t)q], p = J[(size_t)q + 1];
        const double pc = E.Cc[(size_t)p]; tr += std::max(E.Hh[(size_t)j] - E.Ll[(size_t)j], std::max(std::fabs(E.Hh[(size_t)j] - pc), std::fabs(E.Ll[(size_t)j] - pc)));
        dc += std::fabs(E.Cc[(size_t)j] - pc); vm += E.Vol[(size_t)j]; dv += std::fabs(E.Vol[(size_t)j] - E.Vol[(size_t)p]); }
    tr /= TH_N; dc /= TH_N; vm /= TH_N; dv /= TH_N;
    const double pcK = E.Cc[(size_t)J[0]], pvK = E.Vol[(size_t)J[0]];
    if (E.mode == VOL) { if (vm > 0) o.a = vo / vm; if (dv > 0) o.b = std::fabs(vo - pvK) / dv; }
    else { if (tr > 0) o.a = (h - l) / tr; if (dc > 0) o.b = std::fabs(c - pcK) / dc; }
    return o;
}
inline std::string thText(double x) { if (!std::isfinite(x) || x < 0) return "-"; char b[24]; std::snprintf(b, sizeof b, "%.2fx", std::min(x, 999.99)); return b; }
inline std::vector<Prim> layout(const Engine& E, const View& v, const Bar* form, int pick = -1)
{
    std::vector<Prim> P; const int n = (int)E.V.size(), W = v.r - v.l, H = v.b - v.t; if (W < 40 || H < 12 || !v.X) return P;
    const bool box = W >= BOX_W + 60; const bool right = box && v.readX >= 0 && v.readX + BOX_W + 2 <= v.r;   // (1.3.1, Rassul 2026-11-16 10:58 CT "move others reads to the right") right of the last candle when there is room
    const int bxL = right ? v.readX : v.r - 2 - BOX_W, bx1 = bxL + BOX_W; const bool lbox = box && !right;   /* (1.18.1, Rassul 2026-11-27 11:49 CT "the box should always be on the right") no room right of the last candle: the box sits at the RIGHT edge of the pane and the bars under it are not drawn (was the left edge) */
    const int th = std::max(8, v.textH), top = v.t + (E.hlOn ? 2 : 1) * th + 2;          // room for the number (and, 1.3.0, the code) over the tallest bar
    const double fv = form ? E.valueOf(*form) : 0.0;
    const int a = std::max(0, v.first), z = std::min(n - 1 + (form ? 1 : 0), v.last);
    auto valAt = [&](int i) { return i < n ? E.V[(size_t)i] : fv; };
    double mx = 0; for (int i = a; i <= z; ++i) { if (lbox && v.X(i) + v.bw / 2.0 > bxL - 1) continue; mx = std::max(mx, valAt(i)); }
    const int hw = std::max(1, (int)(v.bw * 0.8) / 2);
    if (mx > 0 && v.b - top >= 4) for (int i = a; i <= z; ++i) {
        const double x = v.X(i); const int x0 = (int)std::lround(x) - hw, x1 = (int)std::lround(x) + hw; if (x0 < v.l || x1 > v.r || (lbox && x1 > bxL - 1)) continue;
        const double val = valAt(i); if (!(val > 0)) continue;
        const bool g = i < n ? E.G[(size_t)i] != 0 : (form->c >= form->o);
        const int y = v.b - std::max(1, (int)std::lround(val / mx * (v.b - top)));
        unsigned col = g ? C_UP : C_DN;
        Mk mk; if (E.hlOn) { if (i < n) mk = E.markOf(i); col = mk.code ? (g ? C_UP : C_DN) : C_GREY; }   // (1.3.0) grey unless a marked bar; (1.3.3) a marked bar takes its CANDLE's colour (green up, red down), not the mark's direction
        Prim f; f.fill = true; f.x0 = x0; f.x1 = x1; f.y0 = y; f.y1 = v.b; f.col = col; P.push_back(f);
        int yTop = y - 1;                                                                // texts stack upward from the bar's top
        auto put = [&](const std::string& s, unsigned c, bool bold) {
            const int w = v.textW ? v.textW(s) : (int)s.size() * 6;
            Prim t; t.text = s; t.col = c; t.bold = bold; t.x0 = (int)std::lround(x - w / 2.0); t.x1 = t.x0 + w; t.y1 = yTop; t.y0 = t.y1 - th;
            if (t.y0 < v.t) { t.y0 = v.t; t.y1 = v.t + th; }
            bool ok = t.x0 >= v.l && t.x1 <= (lbox ? bxL - 1 : v.r);
            for (int up = 0; ok && up < 3; ++up) {                                       // never over another text: one row up (twice at most), else not drawn
                bool clash = false; for (const Prim& q : P) if (!q.fill && q.x0 < t.x1 + 2 && t.x0 < q.x1 + 2 && q.y0 < t.y1 && t.y0 < q.y1) { clash = true; break; }
                if (!clash) break;
                if (up == 2 || t.y0 - th < v.t) { ok = false; break; }
                t.y0 -= th; t.y1 -= th;
            }
            if (ok) { P.push_back(t); yTop = t.y0; }
        };
        if (i < n && E.S[(size_t)i].mx() >= ((mk.code == MK_BK || mk.code == MK_SW || mk.code == MK_AR) ? SHOW_MIN_LV : SHOW_MIN)) put(fm(E.S[(size_t)i].mx()), C_TXT, false);          // closed bars only: decided once; (1.8.0) a Bk / Sw bar from 10 up
        if (mk.code >= MK_P) put(mk.code == MK_SW ? (mk.dir > 0 ? "VOB" : "VOT") : mkText(mk.code), col, true);   /* (1.16.0, Rassul 2026-11-25 14:28-14:29 CT "relabel sweep to VOB (volume off the bottom) and VOT (Volume off the top)") was "Sw" */                                          // (1.3.0) the ONE code of a marked bar, in its colour
    }
    bool newRead = false;
    if (v.readX >= 0 && n > 0 && (int)E.Oo.size() == n && (int)E.Vol.size() == n) {
        // (1.4.0, Rassul 2026-11-17 06:54-07:02 CT; mockups RLzNyNZo4rDHuJA2vKwdfY / NdxMKEmswYJHTCjYKW57bi) THE READ, right of the last candle, no frame: four
        // lines (verdict / candle / two behaviours), then the numbers beside them - about the candle under the mouse, else the last closed one
        const bool onForm = pick == n && form != nullptr; const int at = pick >= 0 && pick < n ? pick : n - 1;
        vrd::Ctx x; x.dec = vrd::decOf(E.tick);
        for (int j = std::max(0, at - 24); j <= at; ++j) { if (E.deadBar(j)) continue; vrd::Cd k; k.o = E.Oo[(size_t)j]; k.h = E.Hh[(size_t)j]; k.l = E.Ll[(size_t)j]; k.c = E.Cc[(size_t)j]; k.vol = E.Vol[(size_t)j];
            if (j < (int)E.LT.size()) { k.when = vrd::clock12(E.LT[(size_t)j]); }
            x.c.push_back(k); }
        if (onForm) { vrd::Cd k; k.o = form->o; k.h = form->h; k.l = form->l; k.c = form->c; k.vol = form->v; k.when = "now"; x.c.push_back(k); }
        const Stat s = onForm ? E.forming(*form) : E.S[(size_t)at]; x.cntAll = s.all; x.cntSame = s.same;
        x.mkt = E.mkt; if (at >= 0 && at < (int)E.LT.size() && E.LT[(size_t)at] > 0) x.rth = btr::rthOf(E.LT[(size_t)at] + (onForm ? E.spb : 0), E.spb);   // (1.13.0)
        { const int ai = onForm ? at : at - 1; if (ai >= 0 && ai < (int)E.ATR.size()) x.atr = E.ATR[(size_t)ai]; }
        if (!onForm) { x.cx = E.cxAt(at) ? E.cxAt(at) : E.cxAt(at - 1);                                // (1.10.0) the price chart's Cx on this candle or the one before (0 in the Range pane)
            const auto it = E.inside.find(E.T[(size_t)at]); if (it != E.inside.end()) x.in = it->second; }
        if (!onForm && E.mode == VOL && at >= 0 && at < (int)E.T.size()) {   // (1.15.0) the other panes' word, the candles after it, who led the large orders
            x.tick = E.tick; const long long t0 = E.T[(size_t)at];
            const auto ti = E.tv.find(t0); if (ti != E.tv.end()) { x.tvOk = true; x.tv = ti->second; }
            const auto bi = E.bsv.find(t0); if (bi != E.bsv.end()) { x.bsOk = true; x.bsV = bi->second.first; x.bsTrap = bi->second.second; }
            for (int j = at + 1; j < n && (int)x.nx.size() < 10; ++j) { if (E.deadBar(j)) break; vrd::Cd q; q.o = E.Oo[(size_t)j]; q.h = E.Hh[(size_t)j]; q.l = E.Ll[(size_t)j]; q.c = E.Cc[(size_t)j]; x.nx.push_back(q); }
            if (x.tvOk && x.tv.kind == 1) for (int j = at - 1; j >= 0 && j >= at - 12; --j) { const auto tj = E.tv.find(E.T[(size_t)j]); if (tj != E.tv.end() && tj->second.kind == 1 && tj->second.end == x.tv.end) { x.again = true; break; } }
            { std::vector<const tvr::Row*> rows; for (int j = at; j >= 0 && j >= at - 30; --j) { const auto tj = E.tv.find(E.T[(size_t)j]); rows.push_back(tj == E.tv.end() ? nullptr : &tj->second); if (!rows.back()) break; } vrd::leadOf(rows, x); } }
        vrd::Out o = (!onForm && E.deadBar(at)) ? vrd::Out() : (E.mode == VOL ? vrd::volPlain(x, vrd::volumeRead(x)) : vrd::toPlain(vrd::rangeRead(x), true));   // (1.12.0) the Volume read: how much / orders / icebergs, no verdict (VRRead 1.3.0)
        // (1.9.0, Rassul 2026-11-19 18:31-18:35 CT "those info boxes for the range and vol are missing. add them back", "ok but color so it looks nice. build")
        // the numbers next to the read again, in his 11-14 order - Max on top, the consecutive count, green, red; the other colour's row shows "-"
        if (o.ok) { const std::string pr = E.mode == VOL ? "BV" : "BR"; const bool dead = !onForm && E.deadBar(at);
            auto row = [](const std::string& l, const std::string& a, int c) { vrd::Row r; r.lab = l; r.a = a; r.ca = c; return r; };
            o.rows.clear();
            {   const ThStat ts = dead ? ThStat() : thStat(E, onForm ? n : at, onForm ? form : nullptr);   // (1.18.0) Thrust + PIC / VolTH + Change on top, gold
                const bool vm = E.mode == VOL;
                // two rows (a "PIC 2.62x" cell beside Thrust made the numbers column too wide for a ~300 px read - it vanished at 284 px)
                o.rows.push_back(row(vm ? "VolTH" : "Thrust", thText(ts.a), std::isfinite(ts.a) ? 3 : 2));
                o.rows.push_back(row(vm ? "Change" : "PIC", thText(ts.b), std::isfinite(ts.b) ? 3 : 2)); }
            o.rows.push_back(row(pr + "Max", dead ? "-" : fm(s.mx()), dead ? 2 : (s.mx() >= SHOW_MIN_LV ? 3 : 0)));
            o.rows.push_back(row(pr, dead ? "-" : fm(s.all), dead ? 2 : 0));
            // (1.18.0) the pane fits 4 rows (66 px): the new Thrust row would push BRR / BVR out, so the candle's OWN colour row is shown alone - a green
            // candle "BRG <n>", a red one "BRR <n>" (the other colour's row was always "-"); nothing is lost
            o.rows.push_back(s.green ? row(pr + "G", !dead ? fm(s.same) : "-", !dead ? 1 : 2) : row(pr + "R", !dead ? fm(s.same) : "-", !dead ? -1 : 2)); }   // (1.6.0, Rassul 21:15-21:17 CT "more human readable and simplified", "lets do this") three plain lines, no numbers
        if (o.ok) {   // (1.5.0, Rassul 2026-11-17 08:35-08:44 CT "a split with the left having a verdict and human readable explanation and the right side to have
            // the data points") LEFT: the verdict line, then plain-English sentences word-wrapped (most important first); RIGHT: the numbers
            auto tw = [&](const std::string& q) { return v.textW ? v.textW(q) : (int)q.size() * 6; };
            int rh = th + 2; if ((H - 2) / rh < (int)o.rows.size() && (H - 2) / (th + 1) >= (int)o.rows.size()) rh = th + 1;   /* (1.18.0) 1 px tighter when that is what fits the 5 number rows */
            const int y0 = v.t + 1, fit = (H - 2) / rh;
            const unsigned vc = o.verdict == 1 ? C_UP : o.verdict == -1 ? C_DN : o.verdict == 2 ? C_TXT : C_BOXLN;
            const int headW = tw(o.head) + (o.why.empty() ? 0 : 6 + tw(o.why));
            int wk = 0, wa2 = 0, wb2 = 0; for (const vrd::Row& r : o.rows) { wk = std::max(wk, tw(r.lab)); wa2 = std::max(wa2, tw(r.a)); wb2 = std::max(wb2, tw(r.b)); }
            const int dataW = o.rows.empty() ? 0 : wk + 12 + wa2 + 14 + wb2;
            const int x0 = v.readX; const spt::Cols cl = spt::cols(v.r - x0 - 4, dataW, headW);
            if (fit >= 1 && cl.leftW > 0) {
                newRead = true;
                auto put2 = [&](const std::string& t, int xx2, int y, unsigned col, bool bold) { if (t.empty()) return 0; Prim q; q.bold = bold; q.text = t; q.col = col; q.x0 = xx2; q.x1 = xx2 + tw(t); q.y0 = y; q.y1 = y + rh; P.push_back(q); return tw(t); };
                // (1.5.2) the verdict wraps (at most 2 lines) so a long one no longer pushes the numbers out
                // (1.5.3, Rassul 2026-11-17 14:23-14:25 CT "are all of these indicators defaulting to the current" -> "i'll go with your recommendation") the verdict
                // starts with the candle's time, like the Reversal Read ("2:21 PM Quiet"), so it is plain which candle is read; the forming candle reads "Now"
                const std::string pre = onForm ? std::string("Now ") : (at < (int)E.LT.size() && E.LT[(size_t)at] > 0 ? vrd::clock12(E.LT[(size_t)at]) + " " : std::string());
                const std::string hd = pre + o.head;
                // (1.14.0) the volume-model question column (Volume read only): full when the read keeps 220 px, short when it keeps 150, else none
                const bool hasQ = !o.headQ.empty() && o.exQ.size() == o.ex.size();
                int qW = 0; bool qFull = false;
                if (hasQ) { int wf = tw(o.headQ), ws = tw(vrd::shortQ(o.headQ)); for (const std::string& q : o.exQ) { wf = std::max(wf, tw(q)); ws = std::max(ws, tw(vrd::shortQ(q))); }
                    if (cl.leftW - (wf + 8) >= 220) { qW = wf + 8; qFull = true; } else if (cl.leftW - (ws + 8) >= 150) qW = ws + 8; }
                const int xT = x0 + qW, lw = cl.leftW - qW; std::string lastQ;
                auto tagQ = [&](const std::string& q, int y) { if (!qW || q.empty() || q == lastQ) return; lastQ = q; put2(qFull ? q : vrd::shortQ(q), x0, y, C_LABEL, false); };
                const std::vector<std::string> HL = spt::headLines(hd, o.why, lw, std::max(1, std::min(2, fit - 1)), tw); const int nh = (int)HL.size();
                for (int i = 0; i < nh; ++i) { const int y = y0 + i * rh; const std::string& h = HL[(size_t)i];
                    if (i == 0) tagQ(o.headQ, y);
                    if (i == 0 && h.compare(0, hd.size(), hd) == 0) { put2(pre, xT, y, C_TXT, true); put2(o.head, xT + tw(pre), y, vc, true); if (h.size() > hd.size() + 1) put2(h.substr(hd.size() + 1), xT + tw(hd + " "), y, C_TXT, true); }
                    else put2(h, xT, y, C_TXT, true); }
                std::vector<std::string> EX;
                if (!hasQ) { EX = spt::wrap(o.ex, lw, fit - nh, tw); for (size_t i = 0; i < EX.size(); ++i) put2(EX[i], xT, y0 + (int)(i + (size_t)nh) * rh, C_TXT, false); }
                else for (size_t k = 0; k < o.ex.size(); ++k) {   // sentence by sentence, so each question sits on the first line of its sentence
                    const int left = fit - nh - (int)EX.size(); if (left <= 0) break;
                    const std::vector<std::string> W = spt::wrap(std::vector<std::string>(1, o.ex[k]), lw, left, tw); if (W.empty()) break;
                    for (size_t i = 0; i < W.size(); ++i) { const int y = y0 + (int)(EX.size() + (size_t)nh) * rh; if (i == 0) tagQ(o.exQ[k], y); put2(W[i], xT, y, C_TXT, false); EX.push_back(W[i]); } }
                if (cl.dataOn) {
                    const int xb = x0 + cl.leftW + spt::GAP, ca = xb + wk + 12, cb = ca + wa2 + 14;
                    auto colOf = [](int c) -> unsigned { return c == 1 ? C_UP : c == -1 ? C_DN : c == 2 ? C_LABEL : c == 3 ? C_GOLD : C_TXT; };   // (1.9.0) 3 = gold (a Max of 10+)
                    { Prim e; e.fill = true; e.x0 = xb - 13; e.x1 = xb - 13; e.y0 = y0 + 2; e.y1 = std::min(v.b - 3, y0 + std::max(nh + (int)EX.size(), std::min(fit, (int)o.rows.size())) * rh); e.col = 0x00374151; P.push_back(e); }
                    for (size_t i = 0; i < o.rows.size() && (int)i < fit; ++i) { const vrd::Row& r = o.rows[i]; const int y = y0 + (int)i * rh;
                        put2(r.lab, xb, y, C_LABEL, false); put2(r.a, ca, y, colOf(r.ca), i == 0 || r.ca != 2); put2(r.b, cb, y, colOf(r.cb), true); }   // (1.9.0) grey labels; values bold (a grey - plain)
                }
            }
        }
    }
    if (box && !newRead) {                                                              // the 1.2.0 read box (a narrow pane / the chart scrolled back): the last CLOSED bar
        const int rows = 6, rh = std::max(8, std::min(th + 1, (H - 2 * BOX_PAD) / rows));   /* (1.18.0) 6 rows: + Thrust / PIC (VolTH / Change) */ const int bh = rows * rh + 2 * BOX_PAD;
        const int x0 = bxL, y0 = v.t + std::max(0, (H - bh) / 2), x1 = bx1, y1 = std::min(v.b, y0 + bh);
        Prim bg; bg.fill = true; bg.x0 = x0; bg.y0 = y0; bg.x1 = x1; bg.y1 = y1; bg.col = C_BOXBG; P.push_back(bg);
        const int E4[4][4] = { {x0, y0, x1, y0}, {x0, y1, x1, y1}, {x0, y0, x0, y1}, {x1, y0, x1, y1} };
        for (const auto& e : E4) { Prim ln; ln.fill = true; ln.x0 = e[0]; ln.y0 = e[1]; ln.x1 = e[2]; ln.y1 = e[3]; ln.col = C_BOXLN; P.push_back(ln); }
        const bool vol = E.mode == VOL; const std::string pre = vol ? "BV" : "BR";
        // (1.2.0) the bar the box reads: the one under the mouse (a closed bar, or the forming bar = index n), else the last closed bar
        const bool onForm = pick == n && form != nullptr; const int at = pick >= 0 && pick < n ? pick : n - 1;
        const bool have = onForm || at >= 0; const Stat s = onForm ? E.forming(*form) : at >= 0 ? E.S[(size_t)at] : Stat();
        struct Row { std::string k, val; unsigned col; };
        const ThStat ts = have ? thStat(E, onForm ? n : at, onForm ? form : nullptr) : ThStat();   // (1.18.0)
        const Row R[6] = { { vol ? "VolTH" : "Thrust", thText(ts.a), C_GOLD }, { vol ? "Change" : "PIC", thText(ts.b), C_GOLD },
                           { pre + "Max", have ? fm(s.mx()) : "-", s.green ? C_UP : C_DN }, { pre, have ? fm(s.all) : "-", C_TXT },
                           { pre + "G", have && s.green ? fm(s.same) : "-", C_UP }, { pre + "R", have && !s.green ? fm(s.same) : "-", C_DN } };
        auto tw = [&](const std::string& q) { return v.textW ? v.textW(q) : (int)q.size() * 6; };
        for (int k = 0; k < 6; ++k) {
            const int y = y0 + BOX_PAD + k * rh; if (y + rh > y1) break;
            Prim a2; a2.text = R[k].k; a2.col = C_TXT; a2.x0 = x0 + 5; a2.x1 = a2.x0 + tw(R[k].k); a2.y0 = y; a2.y1 = y + rh; P.push_back(a2);
            Prim b2; b2.bold = true; b2.text = R[k].val; b2.col = R[k].col; b2.x1 = x1 - 5; b2.x0 = b2.x1 - tw(R[k].val); b2.y0 = y; b2.y1 = y + rh; P.push_back(b2);
        }
    }
    return P;
}

inline std::string statusText(const Engine& E, const std::string& mkt, const std::string& sym, int spb)
{
    const bool vol = E.mode == VOL; const std::string pre = vol ? "BV" : "BR"; char b[200];
    std::string o = std::string("VERSION,") + VERSION + "\nPANE," + (vol ? "Volume" : "Range") + "\nMARKET," + mkt + "\nSYMBOL," + sym + "\nBAR_SECONDS," + std::to_string(spb) + "\nBARS," + std::to_string(E.V.size()) +
        "\nRULES,count of bars in a row before the bar with a smaller value (all bars / same colour); Max = the larger; cap 99+; number on closed bars with Max >= " + std::to_string(SHOW_MIN) + "; box = the last closed bar\n";
    const int n = (int)E.V.size(); int big = 0; for (const Stat& s : E.S) if (s.mx() >= SHOW_MIN) ++big;
    o += "NUMBERED," + std::to_string(big) + "\n";
    if (E.hlOn) { int cnt[MK_CB + 1] = { 0 }; std::string last; int shown = 0;
        for (int i = n - 1; i >= 0; --i) { const Mk m = E.markOf(i); if (!m.code) continue; ++cnt[m.code]; if (m.code >= MK_P && shown < 8) { ++shown; last = "MARK," + std::to_string(E.T[(size_t)i]) + "," + std::to_string(i < (int)E.LT.size() ? E.LT[(size_t)i] : 0) + "," + mkText(m.code) + "," + (m.dir > 0 ? "G" : "R") + "\n" + last; } }
        o += "MARKS,1.3.0,CB " + std::to_string(cnt[MK_CB]) + ",SR " + std::to_string(cnt[MK_SR]) + ",Bk " + std::to_string(cnt[MK_BK]) + ",Sw " + std::to_string(cnt[MK_SW]) + ",Rev " + std::to_string(cnt[MK_REV]) + ",Ct " + std::to_string(cnt[MK_CT]) + ",P " + std::to_string(cnt[MK_P]) +
             ",big50 " + std::to_string(cnt[MK_BIG]) + ",BO " + std::to_string(cnt[MK_BO]) + ",level days " + std::to_string(E.lvDay.size()) + ",Cobs read " + std::to_string(E.cobs.size()) + "\n" + "WY,1.8.0 Bk / Sw from the price chart's marks file," + (E.wyLines < 0 ? std::string("no file - no Bk / Sw") : std::to_string(E.wyLines) + " Bk / Sw read") + "\n"
             + (vol ? "OTHERS,1.15.0 lsTurn vread " + (E.tvRows < 0 ? std::string("no file") : std::to_string(E.tvRows) + " candles") + ",lsRevRead bsv " + (E.bsvRows < 0 ? std::string("no file") : std::to_string(E.bsvRows) + " candles") + "\n" : std::string())
             + "CX,1.10.0 " + (vol ? std::string("Cx from the price chart's marks file,") + (E.wyLines < 0 ? std::string("no file - no Cx") : std::to_string(E.wyCx()) + " Cx read") : std::string("no Cx in the Range pane (Cx = climactic volume)")) + "\n"
             + "AR,1.11.0 " + (vol ? std::string("no AR in the Volume pane (AR = a range bar)") : std::string("AR from the price chart's marks file,") + (E.wyLines < 0 ? std::string("no file - no AR") : std::to_string(E.wyAr.size()) + " AR read")) + "\n" + last; }
    if (E.hlOn) { int g = 0, r = 0; for (int i = 0; i < n; ++i) { const int h = E.highlight(i); if (h > 0) ++g; else if (h < 0) ++r; }
        o += "HIGHLIGHT,1.1.0 grey bars; BO / BD with range >= 1.2 x ATR(13) of the bar before, every PB Rev and CoB Rev bar green / red,signals read " + std::to_string(E.hlRaw.size()) + " + " + std::to_string(E.hlLocal.size()) + ",green " + std::to_string(g) + ",red " + std::to_string(r) + "\n"; }
    for (int i = std::max(0, n - 5); i < n; ++i) { const Stat& s = E.S[(size_t)i]; std::snprintf(b, sizeof b, "LAST,%lld,%s,%.10g,%sMax=%s,%s=%s,%s%s=%s\n", E.T[(size_t)i], s.green ? "G" : "R", E.V[(size_t)i], pre.c_str(), fm(s.mx()).c_str(), pre.c_str(), fm(s.all).c_str(), pre.c_str(), s.green ? "G" : "R", fm(s.same).c_str()); o += b; }
    return o;
}

} // namespace vrr
