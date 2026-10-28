// HodLodLogic.h - the HOD / LOD session candle + table, as plain C++ (no SDK): definitions, levels, the read inputs, formatting,
// layout (a draw list the plugin executes and the preview renderer draws), the status / read-log text and the safe file writes.
// Used by HodLod.cpp (lsHodLod) and SessionInfo.cpp (the E-HOD / E-LOD ticks). C++14 (MSVC builds without /std:c++17).
//
// DEFINITIONS (Rassul 2026-10-09, mockup v5). Closed bars only (a bar is closed when a later bar exists); IRT bar times are bar END.
//   RTH bar      bar START >= the market's open and bar END <= its close, on the session's calendar day (seconds, not minutes)
//   open         the first RTH bar's open;  its START is the clock every "took" / "wick" is measured from
//   HOD / LOD    the running high / low of the closed RTH bars; ties keep the FIRST bar that printed the price
//   first        whichever of HOD / LOD printed on the earlier bar; the same bar = not clear yet
//   took         open -> the first extreme's bar END                size  |first extreme - open| (points and $)
//   reclaim      the first strictly later closed bar whose close is at or through the open (>= open after a LOD, <= after a HOD)
//   wick         open -> the reclaim bar's END
//   second       the other extreme; PRINTED once the reclaim precedes it (the research's path rule) or the RTH is complete,
//                otherwise "pending"
//   gap          first -> second extreme (bar ENDs)                  range  HOD - LOD (points and $)
//   swept level  the most significant level the extreme traded THROUGH while the RTH open was still on the other side
//                (open >= level > LOD, or open <= level < HOD): PF (prior full Globex session) > PD (prior RTH) > ON (17:00 ->
//                RTH open) > Lon (02:00-07:30 CT). A level counts only when the chart holds its whole window and the window
//                ended before the extreme's bar began; otherwise nothing is named (never guessed).
#ifndef HODLOD_LOGIC_H
#define HODLOD_LOGIC_H
// (HL105, Rassul "let's get rid of the candle for now. I may ask for it back later") the session-candle column, OFF. All its code
// and tests stay; set this to 1 to bring it back (the test build compiles both ways).
#ifndef HODLOD_SHOW_CANDLE
#define HODLOD_SHOW_CANDLE 0
#endif

#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <climits>
#include <limits>
#include <algorithm>
#include <functional>
#include <fstream>
#include <map>
#include "HodLodExpected.h"
#include "HodLodExpectedV2.h"
#include "HodLodEvents.h"         // (HL101) the economic calendar (CPI / NFP / FOMC / ISM, EIA for CL / NG), generated nightly
#include "HodLodMedians.h"        // the typical (median) first-extreme timing per market (v12 E row)     // hle 2.0.0: the studies agent's conditional expected model (range, close, times, gap fill)

#if defined(__has_include) && !defined(HL_FORCE_NO_READ)
#  if __has_include("HodLodReadTables.h")
#    include "HodLodReadTables.h"
#    define HL_HAVE_READ 1
#  endif
#endif
#ifndef HL_HAVE_READ
#  define HL_HAVE_READ 0
#endif

namespace hl {

static const char* const HL_VERSION = "2.9.1";   // (2.9.1, 2026-10-28) the table keeps right of the left Liquidity column (ColumnBoard leftKeep)   // (2.9.0, 2026-10-26) self-awareness SELFHL (SelfAware.h, SELF-AWARE.md; ships OFF per market) // (2.8.0, 2026-10-26) smart-read hook (AwareHook.h 0.1.0, TEAM-DESIGN.md): posts AWHODLOD for each closed bar once lsContext has posted CTX for it (or after 3 s), records the isolated + aware read in lsFlexLevels\\Context-AWR-<MKT>-<spb>s-<day>.rec; links OFF per market (LRA-Context-<MKT>.csv) so aware = isolated and nothing drawn changes (facts only)   // (2.7.1, 2026-10-22 evening fix build) the day-type log creates lsFlexLevels\\HodLod first ("log fails 17");   // (2.7.0, 2026-10-22 evening, Rassul approved 17:39 CT) the DAY TYPE folded into the header trend ("Uptrend \xB7 Trend day ?" green / "Dntrend \xB7 Trend day ?" red / "Balance \xB7 Rotation day ?" plain; DayTypeLogic.h, price only, live from closed bars; hover = what fired); the NEWS / SETTLE window note row and the "Feed ?" row (NewsWindow.h); the Pre row is LOG ONLY by default (LRA-PreOpen-<MKT>.csv PARAM,SHOW - VALIDATE-CONTEXT-MARKS 2026-10-22: the call never fired, 0 of 42 sessions);   // (2.6.0, 2026-10-22, Rassul 15:57 CT "yes hod lod box", MBO Roadmap T-20261022-29) the PRE-OPEN BOOK row "Pre" under Est (PreOpenLogic.h): from 30 min before the RTH open to 60 min after, the MBO book near price in the last N minutes before the open (bids vs offers, % that stayed, the opening imbalance) and an untested lean "1st LOW early ?"; its own hover (question / what / why / how / example + the numbers);   // (2.5.0, 2026-10-21 10:56 CT, Rassul approved mockup v1 "ok.. build") the Gex S/R LADDER low -> high (supports, the spot with its distance to the nearest S / R, resistances) as the card's top band; the header band with a +GAMMA / -GAMMA pill and the trend in full ("Strong Uptrend"); the regime from the LIVE Gex S/R file's HVL0 when the HVL book is stale; row dividers; footer "Options exp 3:00 PM" . version right;   // (2.4.6, 2026-10-18 20:28 CT, Rassul "remove the hoverover on gext strikes and the hod lod box") the Gex S/R strip and header hovers are off (HodLod.cpp HL_HOVERS);  // (2.4.5, 2026-10-18 19:13 CT, Rassul "right justify the 2nd column" + outside RTH "just keep it blank" - "prior RTH 10/16 ... doesnt make any sense") the 2nd column (heading, Exp, Act, Est) is right-aligned to the table edge; the Est row stays (same height) but is empty while the box shows the prior session;  // (2.4.4, 2026-10-18, Rassul approved 17:13 CT "build") the COMPACT table (~350 px, columns sized to their text; Exp / Act / Est; header "ES M2 O0 V2  Neg . S.Dn"; footer "Exp 3:00 PM . v2.4.4") + the Gex S/R on its own strip ABOVE it (R green / S red); the removed words in the hovers; Offset X / Y removed (9-spot Position only);   // (2.4.3, 2026-10-18 settings audit) the Dealer Profile banner read from the keyed DealerProfile.settings-<MKT>.txt (dprset), the old positional column 9 only through the Dealer Profile's own legacy reader;  (2.4.2, 2026-10-18 settings audit) settings from HodLodSettings.h / SettingsSpec.h (keyed HodLod.settings-<MKT>.txt; colours via setParameterColor / getParameterColor - were black swatches, never saved; Show boxes grey their colours);  (2.4.1, 2026-10-18, Rassul approved 15:03 CT) the table rectangle is published for the docked Big Lot box (BoxDock.h, lsBigBidAsk 1.16.0); nothing drawn changes; (2.4.0, 2026-10-18 night, Rassul approved the audit-C fixes) Reg from ONE MenthorQ source: the HVL0 of GammaProfile-MQ-<MKT>.csv (= the HVL the Dealer Profile draws) vs the last closed close, aged by Fresh.h - STALE shows "Reg: STALE", never a stale regime as live (the LRA-Session GAM / FLIP is no longer used); Q-Scores and the Gex S/R row get the same age check ("Q: STALE"); the Tape Reader slot only while its status file is fresh (120 s); Market State posts HODLOD + TREND once per closed bar (MarketState.h)
// (2.3.0)   // (2.3.0, Rassul 2026-10-17 19:53-20:20) MenthorQ Q-Scores "Q: M1 O1 V4" top left (hover = the meaning), "Reg: Neg / S. Dntrend" top right; Gex S/R = the BAND rule: 3 biggest abs-GEX strikes from the RTH open up to the 1.38 extension of the 1D Max (red, a drawn up triangle first) | price | 3 down to the 1.38 extension of the 1D Min (green, a down triangle first)
// (2.2.4)   // (2.2.4, Rassul 2026-10-17) Gex S/R row: the 3 nearest levels ABOVE price first (red), the current price, then the 3 nearest BELOW (green), MenthorQ name BEFORE each price (blue);   // (2.2.3) header "Reg: Neg / S. Uptrend" (shorter);   // (2.2.2) a reached projection: grey line + flag, no callout, table "Proj HOD x hit h:mm"; (2.2.1) projection levels within 10% of the range, with their prices, + the nearest Gex S/R strike //   // (2.2.0, Rassul 10-15 12:01) Gex S/R row above Estimate; the price / gamma level at the projection (table + under the flag) //   // (2.1.2, Rassul 10-15) no "|" in the GEX row - one separator for all strikes //   // (2.1.1, Rassul 22:23) 2.2.0 audit wording reverted - "Regime: Neg", no "in test" / "?" additions; GEX line kept //   // (2.1.0, 2026-10-14) the GEX line (HodLodGex.h)
#ifndef HL_SHOW_CALLED
#define HL_SHOW_CALLED 1   // (1.1.9, Rassul 2026-10-12 19:52 "approved" the mockup) "called h:mm" after the IN read + "Range used"
#endif

// ------------------------------------------------------------------------------------------------ markets
// open / close = RTH minute of day CT, as the read model (hlr::kOpen / kClose) and the studies (hle::kOpen): every market's
// numbers are measured on the same session.
// Order = hlr::Market (ES NQ CL GC HG NG EU).
struct Mkt { const char* code; int open, close; double pv; int dec; };
static const Mkt MKTS[7] = {
    { "ES", 510, 900, 50.0,     2 },
    { "NQ", 510, 900, 20.0,     2 },
    { "CL", 480, 810, 1000.0,   2 },
    { "GC", 440, 750, 100.0,    1 },
    { "HG", 440, 750, 25000.0,  4 },     // 07:20 per the studies + the read model (07:10 matched the platform's day only 57-61%)
    { "NG", 480, 810, 10000.0,  3 },
    { "EU", 440, 840, 125000.0, 5 },
};
inline int mktIndex(const std::string& m) { for (int i = 0; i < 7; i++) if (m == MKTS[i].code) return i; return -1; }

// The chart's market from IRT's root symbol (the same mapping as DealerLogic.h marketForRoot, kept here so lsHodLod does not
// depend on DealerLogic.h): EPZ26 -> ES, QGC -> GC, CPE -> HG, EU6 / 6E -> EU ...
inline std::string marketForRoot(const std::string& rootIn)
{
    std::string r; for (size_t i = 0; i < rootIn.size(); i++) r += (char)toupper((unsigned char)rootIn[i]);
    auto has = [&](const char* k) { return r.find(k) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return "";
}

// ------------------------------------------------------------------------------------------------ time
inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) q--; return q; }
// days since 1970-01-01 of a civil date (Howard Hinnant)
inline long long daysFromCivil(int y, int m, int d)
{
    y -= m <= 2; long long era = (y >= 0 ? y : y - 399) / 400; unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
inline void civilFromDays(long long z, int& y, int& m, int& d)
{
    z += 719468; long long era = (z >= 0 ? z : z - 146096) / 146097; unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; long long yy = (long long)yoe + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); unsigned mp = (5 * doy + 2) / 153;
    d = (int)(doy - (153 * mp + 2) / 5 + 1); m = (int)(mp < 10 ? mp + 3 : mp - 9); y = (int)(yy + (m <= 2));
}
inline int weekday(long long days) { long long w = floorDiv(days + 4, 7); return (int)(days + 4 - w * 7); }   // 0 = Sunday
inline std::string dateTxt(long long days) { int y, m, d; civilFromDays(days, y, m, d); char b[40]; snprintf(b, sizeof b, "%04d-%02d-%02d", y, m, d); return b; }

inline std::string clk(int mod)          // 13:54 -> "1:54"
{ mod = ((mod % 1440) + 1440) % 1440; int h = mod / 60; char b[16]; snprintf(b, sizeof b, "%d:%02d", h % 12 == 0 ? 12 : h % 12, mod % 60); return b; }
inline std::string clkPm(int mod)        // "1:54pm" in the afternoon, "8:51" in the morning (mockup v5)
{ mod = ((mod % 1440) + 1440) % 1440; return clk(mod) + (mod >= 720 ? "pm" : ""); }
inline std::string clkAmPm(int mod)      // "1:00 PM"
{ mod = ((mod % 1440) + 1440) % 1440; return clk(mod) + (mod >= 720 ? " PM" : " AM"); }
inline std::string hhmm(int mod) { mod = ((mod % 1440) + 1440) % 1440; char b[8]; snprintf(b, sizeof b, "%02d:%02d", mod / 60, mod % 60); return b; }
inline std::string durC(long long m)     // candle: "5h03m" / "21m"
{ if (m < 0) m = 0; char b[24]; if (m >= 60) snprintf(b, sizeof b, "%lldh%02lldm", m / 60, m % 60); else snprintf(b, sizeof b, "%lldm", m); return b; }
inline std::string durT(long long m)     // table: "5h03" / "39m"
{ if (m < 0) m = 0; char b[24]; if (m >= 60) snprintf(b, sizeof b, "%lldh%02lld", m / 60, m % 60); else snprintf(b, sizeof b, "%lldm", m); return b; }

// ------------------------------------------------------------------------------------------------ prices
inline bool toTicks(double px, double tick, long long& out)
{
    if (!std::isfinite(px) || !std::isfinite(tick) || !(tick > 0)) return false;
    double q = px / tick;
    if (!std::isfinite(q) || std::fabs(q) > 1e15) return false;
    const long long rounded = (long long)std::llround(q);
    const double canonical = (double)rounded * tick;
    // Prices arrive from the IRT SDK as float, so compare against the canonical tick with
    // a bounded float-representation allowance, not an arbitrary price epsilon.  This
    // rejects a genuinely off-grid OHLC value instead of silently moving it to a tick.
    const double scale = (std::max)(tick, (std::max)(std::fabs(px), std::fabs(canonical)));
    const double nativeFloatAllowance = 2.0 * (double)std::numeric_limits<float>::epsilon() * scale;
    if (std::fabs(px - canonical) > nativeFloatAllowance) return false;
    out = rounded; return true;
}
inline std::string trimNum(double v, int dec)
{
    char b[48]; snprintf(b, sizeof b, "%.*f", dec, v); std::string s(b);
    if (s.find('.') != std::string::npos) { while (!s.empty() && s[s.size() - 1] == '0') s.erase(s.size() - 1); if (!s.empty() && s[s.size() - 1] == '.') s.erase(s.size() - 1); }
    if (s == "-0") s = "0";
    return s;
}
inline std::string commas(long long v)
{
    bool neg = v < 0; unsigned long long u = neg ? (unsigned long long)(-(v + 1)) + 1ULL : (unsigned long long)v;
    std::string d = std::to_string(u), o; int c = 0;
    for (int i = (int)d.size() - 1; i >= 0; i--) { o.insert(o.begin(), d[(size_t)i]); if (++c % 3 == 0 && i > 0) o.insert(o.begin(), ','); }
    return (neg ? "-" : "") + o;
}
inline std::string usd(double v) { return "$" + commas((long long)std::llround(v)); }
inline std::string pxTxt(double v, int dec)          // 7826.75 -> "7,826.75" (status / tests)
{
    char b[48]; snprintf(b, sizeof b, "%.*f", dec, std::fabs(v)); std::string s(b), ip = s, fp;
    size_t dot = s.find('.'); if (dot != std::string::npos) { ip = s.substr(0, dot); fp = s.substr(dot); }
    long long iv = std::strtoll(ip.c_str(), nullptr, 10);
    return (v < 0 ? "-" : "") + commas(iv) + fp;
}

// ------------------------------------------------------------------------------------------------ bars + levels
struct Bar { long long absEnd; double o, h, l, c; double v = NAN; };   // absEnd = local CT seconds since 1970-01-01 (wall clock); v = volume (NaN = none)

enum { LV_PF = 0, LV_PD = 1, LV_ON = 2, LV_LON = 3 };
static const char* const LV_LOW[4]  = { "PFL", "PDL", "ONL", "LonLO" };
static const char* const LV_HIGH[4] = { "PFH", "PDH", "ONH", "LonHI" };
static const unsigned LV_COL[4] = { 0x00008A8A, 0x00FFA500, 0x00808080, 0x00F59E0B };   // his SessionPrices colours, 0x00RRGGBB

struct Lvl { bool ok = false; int n = 0; long long hi = 0, lo = 0; void add(long long h, long long l) { if (!n) { hi = h; lo = l; } else { if (h > hi) hi = h; if (l < lo) lo = l; } n++; } };

struct Ext { bool set = false; long long px = 0, seq = -1, endAbs = 0; int lvl = -1; long long lvlPx = 0; };   // lvlPx = the swept level's price (ticks)
struct Mark { long long seq = -1, endAbs = 0; };

// ---- (HL104) MenthorQ levels: his selected set, short codes only, in priority order (CR/PS, then GW, HVL, G1, G2)
// Source: %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\MenthorQLevels.csv - the file the MenthorQ bridge writes and lsDealerProfile
// already reads (same columns: SYMBOL,PRICE,LABEL,PENCOLOR,PENWIDTH,...; the colour is the one IRT draws the level in). It is the
// ONE external input of lsHodLod; missing / oversized / stale / garbage -> the key levels only, never guessed.
static const char* const MQ_CODES[6] = { "CR", "PS", "GW", "HVL", "G1", "G2" };
static const size_t MQ_MAX_BYTES = 64 * 1024, MQ_MAX_LINE = 512, MQ_MAX_LEVELS = 32;
struct MqLevel { int code = -1; long long px = 0; unsigned col = 0; };
struct MqSnap { long long atAbs = 0; std::vector<MqLevel> lv; };   // atAbs = the file's write time (local CT wall-clock seconds)
inline bool strictDouble(const std::string& t, double& v) { if (t.empty() || t.size() > 40) return false; char* e = nullptr; v = std::strtod(t.c_str(), &e); return e && *e == 0 && std::isfinite(v); }
inline bool strictLong(const std::string& t, long long& v) { if (t.empty() || t.size() > 20) return false; char* e = nullptr; v = std::strtoll(t.c_str(), &e, 10); return e && *e == 0; }
// a level price for its label: "7,838.25", "7,850" (all-zero decimals dropped - "keep it simple ... very clean")
inline std::string lvlPxTxt(double v, int dec)
{
    std::string t = pxTxt(v, dec); size_t d = t.find('.');
    if (d != std::string::npos && t.find_first_not_of('0', d + 1) == std::string::npos) t.erase(d);
    return t;
}
// the chart's market's levels; a symbol equal to the chart root wins, else the first symbol of the same market (a rolled contract)
inline bool parseMq(const std::string& text, const std::string& mkt, const std::string& root, double tick, std::vector<MqLevel>& out, std::string& why)
{
    out.clear();
    if (text.size() > MQ_MAX_BYTES) { why = "MenthorQLevels.csv over 64 KiB: ignored"; return false; }
    if (mkt.empty() || !(tick > 0)) { why = "no market / tick"; return false; }
    std::string R; for (size_t i = 0; i < root.size(); i++) R += (char)toupper((unsigned char)root[i]);
    std::vector<MqLevel> exact, same; std::string sameSym;
    size_t a = 0; int bad = 0;
    while (a < text.size()) {
        size_t b = text.find('\n', a); if (b == std::string::npos) b = text.size();
        std::string ln = text.substr(a, b - a); a = b + 1;
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        if (ln.empty() || ln.size() > MQ_MAX_LINE || ln.compare(0, 6, "SYMBOL") == 0) { if (ln.size() > MQ_MAX_LINE) bad++; continue; }
        std::vector<std::string> c; size_t p = 0;
        while (c.size() < 6) { size_t q = ln.find(',', p); c.push_back(ln.substr(p, q == std::string::npos ? std::string::npos : q - p)); if (q == std::string::npos) break; p = q + 1; }
        if (c.size() < 5 || c[0].empty()) { bad++; continue; }
        std::string sym; for (size_t i = 0; i < c[0].size(); i++) sym += (char)toupper((unsigned char)c[0][i]);
        if (marketForRoot(sym) != mkt) continue;
        double px; long long tk, colv = 0;
        if (!strictDouble(c[1], px) || !(px > 0) || !toTicks(px, tick, tk)) { bad++; continue; }
        unsigned col = (strictLong(c[3], colv) && colv > 0 && colv <= 0xFFFFFF) ? (unsigned)colv : 0x0022D3EEu;   // as lsDealerProfile: never black
        // "CR 0D / GW 0D" = two levels at one price: one entry each (never shown combined)
        std::string lab = c[2]; size_t s0 = 0;
        while (s0 <= lab.size()) {
            size_t s1 = lab.find('/', s0); std::string tok = lab.substr(s0, s1 == std::string::npos ? std::string::npos : s1 - s0);
            size_t i0 = tok.find_first_not_of(' '); std::string w = i0 == std::string::npos ? "" : tok.substr(i0);
            size_t sp = w.find(' '); if (sp != std::string::npos) w = w.substr(0, sp);
            for (int k = 0; k < 6; k++) if (w == MQ_CODES[k]) {
                MqLevel L; L.code = k; L.px = tk; L.col = col;
                if (sym == R) { if (exact.size() < MQ_MAX_LEVELS) exact.push_back(L); }
                else if (sameSym.empty() || sameSym == sym) { sameSym = sym; if (same.size() < MQ_MAX_LEVELS) same.push_back(L); }
            }
            if (s1 == std::string::npos) break;
            s0 = s1 + 1;
        }
    }
    out = !exact.empty() ? exact : same;
    if (out.empty()) { why = bad ? "MenthorQLevels.csv: no valid " + mkt + " levels (" + std::to_string(bad) + " bad lines)" : "MenthorQLevels.csv: no " + mkt + " levels"; return false; }
    if (bad) why = std::to_string(bad) + " bad lines ignored";
    return true;
}

struct Rth {
    bool any = false, complete = false;
    long long sid = 0, open = 0, openStartAbs = 0, lastClose = 0, lastEndAbs = 0, lastSeq = -1;
    int n = 0;
    Ext hod, lod;
    Mark hodRecl, lodRecl;                 // reclaim of each side (strictly later bar, close at / through the open)
    long long hodStrict = -1, lodStrict = -1;   // read-model "recl" split: any close STRICTLY beyond the open since the extreme (incl. it)
    bool orSet = false; long long orHi = 0, orLo = 0;   // read model's opening range (bar END - read open <= N minutes)
    long long rngAt[hle::kMaxMarks];   // hle: RTH range (ticks) of the bars with mso <= 30(k+1), set once at the first closed bar
    bool rngSet[hle::kMaxMarks];       //      with mso >= 30(k+1), never changed after (-1 = not yet)
    long long firstEndAbs = 0;
    bool bothRed = false;              // hle's tie rule: when one bar printed both extremes, a red bar = the HOD came first
    // (HL101) hlr::timing() inputs, exactly as hodlod_read.py tstates(): the RTH-anchored VWAP of (H+L+C)/3 with volume
    double sv = 0, svtp = 0, svtp2 = 0, stp = 0; int ntp = 0; bool volMissing = false;
    struct TB { double h, l, c, vw, sd; };
    TB ring[20]; int ringN = 0, ringAt = 0;            // the last 20 RTH bars with their VWAP / SD (the vwapTrend look-back)
    bool ibSet = false; long long ibHi = 0, ibLo = 0;  // the IB (bars with minutes-since-open <= 60), so far
    Rth() { for (int k = 0; k < hle::kMaxMarks; k++) { rngAt[k] = -1; rngSet[k] = false; } }
};

struct Cfg {
    int mkt = 0;            // index into MKTS
    long long per = 180;    // seconds per bar
    double tick = 0.25;
    int readOpen = 510, readN = 30;   // the read model's own open (minute of day) and N (minutes) - from hlr when it is built in
};

// the read model's inputs at one closed RTH bar
struct ReadIn {
    bool ok = false; int market = 0, mso = 0, side = 0, now = 0, split = 0; double dist = 0, norm = 0, rangeSoFar = 0;
    int t1Mod = -1, reclMod = -1;   // the read side's 1st extreme bar END / its opening reclaim (minute of day, -1 = none yet)
    double depthAtr = NAN;          // |open - 1st extreme| / ATR (the read model's ATR) - hlr::stage2
    unsigned evBits = 0; bool evKnown = false;   // (HL101) the session's economic calendar (HodLodEvents.h); known = inside the file
    double f[14];                   // (HL101) hlr::timing features (tfirst, rng, dopp, posr, since_opp, trend_al, trend_ag, ib_ext,
    bool fOk = false;               //         ib_done, ib_atr, fomc, amnews, opex, gap) - hodlod_read.py tstates(), exactly
    std::string fWhy;               //         why they are not available
    ReadIn() { for (int i = 0; i < 14; i++) f[i] = NAN; }
    std::string why;        // why ok is false
};

class Tracker {
public:
    Cfg C;
    long long seq = -1, firstAbsStart = 0;
    bool haveSid = false; long long curSid = 0;
    Lvl curFull, curOn, curLon;
    Lvl curOnMid;           // hle onRange as fitted (studies.py): the bars from 00:00 CT to the RTH open (not from 17:00)
    Lvl pf, pd;             // prior full session / prior RTH (ok = the chart holds the whole window)
    Rth cur, last;          // the current session's RTH / the latest completed session that had RTH bars
    std::vector<long long> prevRanges;   // RTH range (ticks) of every prior session with RTH bars (read model ATR)
    bool havePrevClose = false; long long prevClose = 0, prevRange = -1;

    explicit Tracker(const Cfg& c) : C(c) {}

    long long rthA(long long sid) const { return sid * 86400 + (long long)MKTS[C.mkt].open * 60; }
    long long rthB(long long sid) const { return sid * 86400 + (long long)MKTS[C.mkt].close * 60; }
    long long onA(long long sid) const { return (sid - 1) * 86400 + 17 * 3600; }
    long long lonA(long long sid) const { return sid * 86400 + 2 * 3600; }
    long long lonB(long long sid) const { return sid * 86400 + 7 * 3600 + 1800; }

    static long long sidOf(long long absStart)
    {
        long long d = floorDiv(absStart, 86400), sod = absStart - d * 86400;
        return d + (sod >= 17 * 3600 ? 1 : 0);
    }

    // the most significant level the extreme traded through (see the header comment), -1 = none
    int swept(bool low, long long px, long long barStartAbs) const { long long lp = 0; return swept(low, px, barStartAbs, lp); }
    int swept(bool low, long long px, long long barStartAbs, long long& lvlPx) const
    {
        const Lvl* L[4] = { &pf, &pd, &curOn, &curLon };
        for (int i = 0; i < 4; i++) {
            const Lvl& v = *L[i];
            if (!v.ok || v.n <= 0) continue;
            if (i == LV_ON && !(firstAbsStart <= onA(curSid))) continue;
            if (i == LV_LON && !(firstAbsStart <= lonA(curSid) && lonB(curSid) <= barStartAbs)) continue;
            if (low) { if (px < v.lo && cur.open >= v.lo) { lvlPx = v.lo; return i; } }
            else     { if (px > v.hi && cur.open <= v.hi) { lvlPx = v.hi; return i; } }
        }
        return -1;
    }

    void rollover(long long sid)
    {
        // A session that has not reached its RTH close is only a partial observation. It must not
        // become the prior session, PD/PF source, previous close/range, or an ATR sample: doing so
        // lets a chart opened late manufacture a completed day and contaminate the next day's model.
        const bool rthComplete = cur.any && cur.complete;
        if (rthComplete) {
            prevRanges.push_back(cur.hod.px - cur.lod.px);
            if (prevRanges.size() > 64) prevRanges.erase(prevRanges.begin());
            havePrevClose = true; prevClose = cur.lastClose; prevRange = cur.hod.px - cur.lod.px;
            pd = Lvl(); pd.n = 1; pd.hi = cur.hod.px; pd.lo = cur.lod.px; pd.ok = firstAbsStart <= rthA(curSid);
            last = cur;
        } else {
            pd = Lvl();
        }
        pf = curFull; pf.ok = rthComplete && curFull.n > 0 && firstAbsStart <= onA(curSid);
        curFull = Lvl(); curOn = Lvl(); curLon = Lvl(); curOnMid = Lvl(); cur = Rth();
        curSid = sid;
    }

    // one CLOSED bar, in chart order. Returns false (and ignores it) when its prices are not finite / not on the tick grid.
    bool push(const Bar& b)
    {
        long long o, h, l, c;
        if (!toTicks(b.o, C.tick, o) || !toTicks(b.h, C.tick, h) || !toTicks(b.l, C.tick, l) || !toTicks(b.c, C.tick, c)) return false;
        if (h < l || o > h || o < l || c > h || c < l) return false;
        long long absStart = b.absEnd - C.per;
        long long sid = sidOf(absStart);
        seq++;
        if (!haveSid) { haveSid = true; curSid = sid; firstAbsStart = absStart; }
        else if (sid != curSid) rollover(sid);
        curFull.add(h, l); curFull.ok = true;
        if (b.absEnd <= rthA(sid)) { curOn.add(h, l); curOn.ok = true; }
        if (b.absEnd >= sid * 86400 && absStart < rthA(sid)) { curOnMid.add(h, l); curOnMid.ok = true; }   // bar END 00:00 .. every bar before the first RTH bar (as studies.py)
        if (absStart >= lonA(sid) && b.absEnd <= lonB(sid)) { curLon.add(h, l); curLon.ok = true; }
        if (absStart >= rthA(sid) && b.absEnd <= rthB(sid)) rthBar(o, h, l, c, absStart, b.absEnd, b.v);
        else if (cur.any && b.absEnd > rthB(sid)) cur.complete = true;   // a closed bar after the RTH close
        return true;
    }

    void rthBar(long long o, long long h, long long l, long long c, long long absStart, long long absEnd, double vol = NAN)
    {
        Rth& R = cur;
        if (!R.any) { R.any = true; R.sid = curSid; R.open = o; R.openStartAbs = absStart; R.firstEndAbs = absEnd; }
        {   // (HL101) hlr::timing inputs: VWAP / SD of the typical price with volume (hodlod_read.py _vwap_trend), the IB so far
            const double t = C.tick, H = (double)h * t, L = (double)l * t, Cc = (double)c * t, tp = (H + L + Cc) / 3.0;
            double v = vol;
            if (!std::isfinite(v)) { v = 1.0; R.volMissing = true; }      // no volume on the chart: equal weights (as the model's fallback)
            if (v < 0) v = 0;
            R.sv += v; R.svtp += v * tp; R.svtp2 += v * tp * tp; R.stp += tp; R.ntp++;
            double vw = R.sv > 0 ? R.svtp / R.sv : R.stp / (double)R.ntp;
            double var = R.sv > 0 ? R.svtp2 / R.sv - vw * vw : 0.0;
            Rth::TB tb = { H, L, Cc, vw, std::sqrt(var > 0 ? var : 0.0) };
            R.ring[R.ringAt] = tb; R.ringAt = (R.ringAt + 1) % 20; if (R.ringN < 20) R.ringN++;
            if ((absEnd - rthA(curSid)) / 60 <= 60) {
                if (!R.ibSet) { R.ibSet = true; R.ibHi = h; R.ibLo = l; } else { R.ibHi = (std::max)(R.ibHi, h); R.ibLo = (std::min)(R.ibLo, l); }
            }
        }
        {   // hle rngAt[k]: the range of the bars with mso <= 30(k+1), fixed at the first bar reaching that mark (no repaint)
            long long mso = (absEnd - rthA(curSid)) / 60;
            long long hiB = R.n > 0 ? R.hod.px : LLONG_MIN, loB = R.n > 0 ? R.lod.px : LLONG_MAX;   // before this bar
            long long hiI = R.n > 0 ? (std::max)(hiB, h) : h, loI = R.n > 0 ? (std::min)(loB, l) : l;  // including it
            for (int k = 0; k < hle::kMaxMarks; k++) {
                long long mark = 30LL * (k + 1);
                if (R.rngSet[k] || mso < mark) continue;
                R.rngSet[k] = true;
                R.rngAt[k] = mso == mark ? hiI - loI : (R.n > 0 ? hiB - loB : -1);
            }
        }
        R.n++; R.lastClose = c; R.lastEndAbs = absEnd; R.lastSeq = seq;
        if (absEnd - (curSid * 86400 + (long long)C.readOpen * 60) <= (long long)C.readN * 60) {
            if (!R.orSet) { R.orSet = true; R.orHi = h; R.orLo = l; } else { R.orHi = (std::max)(R.orHi, h); R.orLo = (std::min)(R.orLo, l); }
        }
        if ((!R.lod.set || l < R.lod.px) && (!R.hod.set || h > R.hod.px)) R.bothRed = c < o;
        if (!R.lod.set || l < R.lod.px) {
            R.lod.set = true; R.lod.px = l; R.lod.seq = seq; R.lod.endAbs = absEnd; R.lod.lvl = swept(true, l, absStart, R.lod.lvlPx);
            R.lodRecl = Mark(); R.lodStrict = c > R.open ? seq : -1;
        } else {
            if (R.lodRecl.seq < 0 && c >= R.open) { R.lodRecl.seq = seq; R.lodRecl.endAbs = absEnd; }
            if (R.lodStrict < 0 && c > R.open) R.lodStrict = seq;
        }
        if (!R.hod.set || h > R.hod.px) {
            R.hod.set = true; R.hod.px = h; R.hod.seq = seq; R.hod.endAbs = absEnd; R.hod.lvl = swept(false, h, absStart, R.hod.lvlPx);
            R.hodRecl = Mark(); R.hodStrict = c < R.open ? seq : -1;
        } else {
            if (R.hodRecl.seq < 0 && c <= R.open) { R.hodRecl.seq = seq; R.hodRecl.endAbs = absEnd; }
            if (R.hodStrict < 0 && c < R.open) R.hodStrict = seq;
        }
        if (absEnd >= rthB(curSid)) R.complete = true;
    }

    // the read model's ATR: the mean RTH range of the last (up to) 10 prior sessions, at least 5 (hodlod_read.py sessions())
    double readAtr() const
    {
        if (prevRanges.size() < 5) return NAN;
        size_t k = (std::min)(prevRanges.size(), (size_t)10); double a = 0;
        for (size_t i = prevRanges.size() - k; i < prevRanges.size(); i++) a += (double)prevRanges[i] * C.tick;
        return a / (double)k;
    }
    // (HL101) hlr::timing's 14 features at the latest RTH bar, as hodlod_read.py tstates() computes them (3-min bars)
    void timingFeatures(ReadIn& r, bool lodFirst) const
    {
        const Rth& R = cur; const double t = C.tick;
        double atr = readAtr();
        if (!std::isfinite(atr) || atr <= 0) { r.fWhy = "needs 5 prior sessions on the chart (ATR)"; return; }
        if (C.per != 180) { r.fWhy = "fitted on 3-min bars"; return; }
        long long day = R.sid * 86400;
        int now = (int)((R.lastEndAbs - day) / 60), mso = now - MKTS[C.mkt].open;
        double runH = (double)R.hod.px * t, runL = (double)R.lod.px * t, Cc = (double)R.lastClose * t, rng = runH - runL;
        int tH = (int)((R.hod.endAbs - day) / 60), tL = (int)((R.lod.endAbs - day) / 60);
        // vwapTrend over the last 20 RTH bars (incl. this one)
        bool anyHi2 = false, anyLo2 = false, anySd = false, anyLo1 = false, anyHi1 = false; int nAbove = 0, nBelow = 0, n = R.ringN;
        for (int k = 0; k < n; k++) {
            const Rth::TB& b = R.ring[(R.ringAt - 1 - k + 40) % 20];
            if (b.h >= b.vw + 2 * b.sd) anyHi2 = true;
            if (b.l <= b.vw - 2 * b.sd) anyLo2 = true;
            if (b.sd > 0) anySd = true;
            if (b.l <= b.vw - b.sd) anyLo1 = true;
            if (b.h >= b.vw + b.sd) anyHi1 = true;
            if (b.c > b.vw) nAbove++;
            if (b.c < b.vw) nBelow++;
        }
        int up = ((anyHi2 && anySd) || (n > 0 && (double)nAbove / n > 0.5 && !anyLo1)) ? 1 : 0;
        int dn = ((anyLo2 && anySd) || (n > 0 && (double)nBelow / n > 0.5 && !anyHi1)) ? 1 : 0;
        bool ibDone = mso > 60;
        double ibH = (double)R.ibHi * t, ibL = (double)R.ibLo * t;
        double dfirst, dopp, ibExt; int tFirst, tOpp; int al, ag;
        if (lodFirst) { tFirst = tL; tOpp = tH; dfirst = Cc - runL; dopp = runH - Cc; ibExt = ibDone ? (std::max)(0.0, runH - ibH) / atr : 0.0; al = up; ag = dn; }
        else          { tFirst = tH; tOpp = tL; dfirst = runH - Cc; dopp = Cc - runL; ibExt = ibDone ? (std::max)(0.0, ibL - runL) / atr : 0.0; al = dn; ag = up; }
        int y, m, d; civilFromDays(R.sid, y, m, d);
        r.f[0] = (now - tFirst) / 60.0;
        r.f[1] = rng / atr;
        r.f[2] = dopp / atr;
        r.f[3] = rng > 0 ? dfirst / rng : 0.0;
        r.f[4] = (now - tOpp) / 60.0;
        r.f[5] = al; r.f[6] = ag;
        r.f[7] = ibExt;
        r.f[8] = ibDone ? 1.0 : 0.0;
        r.f[9] = R.ibSet ? (double)(R.ibHi - R.ibLo) * t / atr : 0.0;
        r.f[10] = hlev::fomcDay(r.evBits) ? 1.0 : 0.0;
        r.f[11] = hlev::amNews(r.evBits) ? 1.0 : 0.0;
        r.f[12] = (weekday(R.sid) == 5 && d >= 15 && d <= 21) ? 1.0 : 0.0;
        r.f[13] = havePrevClose ? std::fabs((double)R.open * t - (double)prevClose * t) / atr : 0.0;
        r.fOk = true;
        if (!r.evKnown) r.fWhy = "calendar unknown for this date (fomc / amnews = 0)";
        if (R.volMissing) r.fWhy += std::string(r.fWhy.empty() ? "" : "; ") + "no volume on the chart (VWAP trend equal-weighted)";
    }

    // the read model's inputs after the latest pushed bar (only meaningful when that bar was an RTH bar of `cur`)
    ReadIn readIn(int normKind, int splitKind) const
    {
        ReadIn r; r.market = C.mkt;
        const Rth& R = cur;
        if (!R.any || R.lastSeq != seq) { r.why = "not an RTH bar"; return r; }
        long long day = R.sid * 86400;
        r.now = (int)((R.lastEndAbs - day) / 60);
        r.mso = r.now - C.readOpen;
        bool lodFirst = R.lod.seq < R.hod.seq || (R.lod.seq == R.hod.seq && (R.lastClose - R.lod.px) >= (R.hod.px - R.lastClose));
        r.side = lodFirst ? 0 : 1;
        double t = C.tick;
        r.dist = (double)(lodFirst ? R.lastClose - R.lod.px : R.hod.px - R.lastClose) * t;
        r.rangeSoFar = (double)(R.hod.px - R.lod.px) * t;
        {   // the chained stages' inputs (hlr::stage2 / stage3): the read side's 1st extreme, its reclaim, its depth in ATR
            const Ext& F = lodFirst ? R.lod : R.hod; const Mark& RC = lodFirst ? R.lodRecl : R.hodRecl;
            r.t1Mod = (int)((F.endAbs - day) / 60);
            r.reclMod = RC.seq >= 0 && RC.seq <= seq ? (int)((RC.endAbs - day) / 60) : -1;
            if (prevRanges.size() >= 5) {
                size_t k = (std::min)(prevRanges.size(), (size_t)10); double a = 0;
                for (size_t i = prevRanges.size() - k; i < prevRanges.size(); i++) a += (double)prevRanges[i] * t;
                a /= (double)k;
                if (a > 0) r.depthAtr = (double)(F.px > R.open ? F.px - R.open : R.open - F.px) * t / a;
            }
        }
        int y, m, d; civilFromDays(R.sid, y, m, d);
        r.evBits = hlev::bitsOn(y * 10000 + m * 100 + d, r.evKnown);
        timingFeatures(r, lodFirst);
        if (normKind == 0) { if (!R.orSet || r.mso < C.readN) { r.why = "opening range not complete"; return r; } r.norm = (double)(R.orHi - R.orLo) * t; }
        else if (normKind == 1) {     // hodlod_read.py sessions(): mean RTH range of the last (up to) 10 prior sessions, at least 5
            if (prevRanges.size() < 5) { r.why = "needs 5 prior sessions on the chart"; return r; }
            size_t k = (std::min)(prevRanges.size(), (size_t)10);
            double s = 0; for (size_t i = prevRanges.size() - k; i < prevRanges.size(); i++) s += (double)prevRanges[i] * t;
            r.norm = s / (double)k;
        }
        else if (normKind == 2) r.norm = (double)(R.hod.px - R.lod.px) * t;
        else { r.why = "unknown normaliser"; return r; }
        switch (splitKind) {
            case 0: r.split = 0; break;
            case 1: r.split = lodFirst ? (R.lodStrict >= 0 ? 1 : 0) : (R.hodStrict >= 0 ? 1 : 0); break;
            case 2: r.split = r.side; break;
            case 6: r.split = (weekday(R.sid) == 5 && d >= 15 && d <= 21) ? 1 : 0; break;
            case 7: r.split = weekday(R.sid) == 5 ? 1 : 0; break;
            default: r.why = "split needs data the chart does not have (news / gap cut / OR cut)"; return r;
        }
        r.ok = true; return r;
    }
};

// ------------------------------------------------------------------------------------------------ the expected model (hle)
// hle::State from the chart's own bars. why = what was not available (flagged in the status file)
inline hle::State hleState(const Tracker& T, const Rth& R, std::string& why)
{
    hle::State s; const double t = T.C.tick;
    s.market = T.C.mkt; s.firstSide = -1; s.news = 0;
    {   // (HL101) news from the baked calendar (HodLodEvents.h): CPI / NFP / FOMC / ISM (+ EIA for CL / NG); outside the file -> 0, flagged
        int y, m, d; civilFromDays(R.sid, y, m, d); bool known = false;
        unsigned bits = hlev::bitsOn(y * 10000 + m * 100 + d, known);
        s.news = hlev::newsDay(T.C.mkt, bits) ? 1 : 0;
        if (!known) why += "calendar unknown for " + dateTxt(R.sid) + " (events file ends " + std::to_string(hlev::kLastDay) + "): news = 0; ";
    }
    int wd = weekday(R.sid); s.dow = wd >= 1 && wd <= 5 ? wd - 1 : 0;
    long long day = R.sid * 86400;
    s.mso = R.any ? (int)((R.lastEndAbs - day) / 60) - MKTS[T.C.mkt].open : 0;
    s.open = (double)R.open * t; s.hi = (double)R.hod.px * t; s.lo = (double)R.lod.px * t; s.last = (double)R.lastClose * t;
    for (int k = 0; k < hle::kMaxMarks; k++) s.rngAt[k] = R.rngAt[k] >= 0 ? (double)R.rngAt[k] * t : -1.0;
    s.atr14 = NAN;   // studies.py: the mean RTH range of the prior 14 sessions (any completeness), plain high - low
    if (T.prevRanges.size() >= 14) { double a = 0; for (size_t i = T.prevRanges.size() - 14; i < T.prevRanges.size(); i++) a += (double)T.prevRanges[i] * t; s.atr14 = a / 14.0; }
    else why += "ATR14 needs 14 prior RTH sessions on the chart (has " + std::to_string(T.prevRanges.size()) + "): v1 model shown; ";
    // onRange exactly as the model was fitted: studies.py keeps only the pre-open bars of the CALENDAR day (00:00 -> the open),
    // although hle's header says 17:00 (reported to the studies agent)
    s.onRange = (&R == &T.cur && T.curOnMid.n > 0 && T.firstAbsStart <= T.curSid * 86400 - T.C.per) ? (double)(T.curOnMid.hi - T.curOnMid.lo) * t : -1.0;
    s.prevRange = T.prevRange >= 0 && &R == &T.cur ? (double)T.prevRange * t : -1.0;
    s.prevClose = T.havePrevClose && &R == &T.cur ? (double)T.prevClose * t : -1.0;
    if (s.prevClose > 0 && std::isfinite(s.atr14) && R.any && std::fabs(s.open - s.prevClose) > 4.0 * s.atr14) {
        s.prevClose = -1.0; why += "open-to-prior-close jump > 4 ATR (contract roll): no gap read; ";   // studies.py roll rule
    }
    if (R.any) {
        // fit_expected.py live_state: the earlier extreme; one bar printing both -> a red bar = the HOD first
        s.firstSide = R.lod.seq < R.hod.seq ? 0 : (R.hod.seq < R.lod.seq ? 1 : (R.bothRed ? 1 : 0));
    }
    return s;
}

// ------------------------------------------------------------------------------------------------ the read (hlr)
struct ReadOut {
    int state = 0;          // 0 = read tables not built in yet, 1 = valid, 2 = not yet (before minute N), 3 = n/a (why)
    double pIn = NAN, afterP = NAN, byP = NAN; int afterT = -1, byT = -1, cellN = 0, side = 0;
    double pInShow = NAN; bool afterQ = false; double afterShow = NAN;   // (HL101) hlr's calibrated values
    std::string ver, why;
    // the 2nd-extreme timing call shown (header + E 2nd Time, identical text); one source per state (coordinator 2026-10-09 20:30):
    //   callSrc 2 = hlr::timing before the reclaim: "close push 18% \xB7 else 11:30-1:15" (pClose + the 50% else-window)
    //   callSrc 3 = hlr::stage3(m, t1, reclaim, now) once reclaimed: "after 1:00 80%" (T >= now on every bar, the show %)
    //   hlr::read's afterT (6-18 min lead) is logged for the scorer, not shown.
    int callT = -1, callSrc = 0; double callP = NAN;      // callP = the model's raw P (logged)
    bool callQ = false;     // the call reached the market's calibrated threshold
    double callShow = NAN;  // the % DISPLAYED: the delivered rate (kAfterShow / kShow3) when qualified, else min(raw, that rate)
    bool callLate = false;  // a source was live but no 15-min T is left before the close: "late-day"
    bool callDim = false;   // shown grey: stage3 not qualified, or the market's stage 3 is in test (S3_IN_TEST)
    bool callIn = false;    // the first extreme is called IN (pIn >= IN_THR): the call names the opposite side; else it is side-neutral
    int fomcEnd = -1;       // the minute the FOMC pause ends
    bool fomcPause = false; // (HL101) FOMC day before the market's pause end: no IN / NOT IN call (hlr::fomcPause)
    bool s2 = false; int s2from = -1, s2to = -1; double s2p = NAN; bool s2below = false;   // hlr::stage2: the reclaim window
    double s2show = NAN;    // the % displayed for it (kShow2 when qualified, else min(raw, kShow2))
    bool s2none = false;    // stage2 valid but no window left (w = -1)
    double gapMin = NAN;    // hlr::stage3: the median 1st -> 2nd time
    bool tOk = false; double pLater = NAN, pClose = NAN; int elseFrom = -1, elseTo = -1; std::string tWhy;   // (HL101) hlr::timing (scorer)
};
static const double IN_THR = 0.70;   // the read model's operating point for "IN" (HodLodReadTables.h display rule)
// (Rassul 2026-10-09 "even 75% odds is good enough") "expect <side> after <T>": the LATEST half-hour T at or after now with
// P(2nd extreme after T) >= the threshold. The read model now applies it inside hlr::read (kAfterThr, a 75% floor recalibrated
// nightly, ES 0.78); AFTER_THR is the floor Rassul set, written to the status file with the model's own value.
static const double AFTER_THR = 0.75;
inline int readNormKind(int mkt) {
#if HL_HAVE_READ
    return hlr::kNorm[mkt];
#else
    (void)mkt; return 2;
#endif
}
inline int readSplitKind(int mkt) { (void)mkt; return 0; }   // the current read tables have no split
inline int readOpenFor(int mkt) {
#if HL_HAVE_READ
    return hlr::kOpen[mkt];
#else
    return MKTS[mkt].open;
#endif
}
inline int readNFor(int mkt) {
#if HL_HAVE_READ
    return hlr::kMinMinutes[mkt];
#else
    (void)mkt; return 30;
#endif
}
inline bool readSessionMatches(int mkt) {
#if HL_HAVE_READ
    return hlr::kOpen[mkt] == MKTS[mkt].open && hlr::kClose[mkt] == MKTS[mkt].close;
#else
    (void)mkt; return true;
#endif
}
// (coordinator 2026-10-09 20:30, read model report section 0) ES stage 3 delivered 67% on TEST (< the 75% target): shown, grey,
// "in test" until the nightly scorecard proves it. Index = market (ES NQ CL GC HG NG EU).
static const bool STAGE3_NEEDS_IN = true;   // see evalRead; false = switch at the reclaim of whatever extreme stands first
static const bool S3_IN_TEST[7] = { true, false, false, false, false, false, false };
inline double shownP(bool qualified, double raw, double show)
{   // the % displayed: the delivered rate of a qualifying call; a below-threshold best guess never shows more than that rate
    if (!std::isfinite(raw)) return NAN;
    if (!std::isfinite(show)) return raw;
    return qualified ? show : (std::min)(raw, show);
}
inline ReadOut evalRead(const ReadIn& in)
{
    ReadOut o;
#if HL_HAVE_READ
    o.ver = hlr::version(in.market);
    if (!readSessionMatches(in.market)) { o.state = 3; o.why = "session differs from the read model"; return o; }
    if (in.mso < hlr::kMinMinutes[in.market]) { o.state = 2; return o; }      // before minute N: "read from <time>"
    if (!in.ok) { o.state = 3; o.why = in.why; return o; }
    hlr::Read r = hlr::read(in.market, in.mso, in.side, in.dist, in.norm, in.now, in.rangeSoFar);
    if (!r.valid) { o.state = 2; return o; }
    if (!std::isfinite(r.pIn)) { o.state = 3; o.why = "no value"; return o; }
    o.state = 1; o.pIn = r.pIn; o.pInShow = r.pInShow; o.cellN = r.cellN; o.side = r.side; o.byT = r.byT; o.byP = r.byP;
    o.afterT = r.afterT; o.afterP = r.afterP; o.afterQ = r.afterQualified; o.afterShow = r.afterShow;
    // FOMC days: no IN / NOT IN call before the market's pause end (hlr::fomcPause); the timing call stays side-neutral
    o.fomcPause = hlr::fomcPause(in.market, hlev::fomcDay(in.evBits), in.now);
    switch (in.market) { case 0: o.fomcEnd = hlr::es::kFomcPause; break; case 1: o.fomcEnd = hlr::nq::kFomcPause; break; case 2: o.fomcEnd = hlr::cl::kFomcPause; break;
                         case 3: o.fomcEnd = hlr::gc::kFomcPause; break; case 4: o.fomcEnd = hlr::hg::kFomcPause; break; case 5: o.fomcEnd = hlr::ng::kFomcPause; break;
                         default: o.fomcEnd = hlr::eu::kFomcPause; }
    o.callIn = !o.fomcPause && r.pIn >= IN_THR - 1e-12;
    // (HL101) hlr::timing for the scorer's columns (pLater, pClose, else window)
    if (in.fOk) {
        hlr::Timing tm = hlr::timing(in.market, in.f, in.side, in.now, in.mso);
        o.tOk = tm.valid; o.pLater = tm.pLater; o.pClose = tm.pClose; o.elseFrom = tm.elseFrom; o.elseTo = tm.elseTo;
        o.tWhy = in.fWhy;                 // flags (calendar unknown, no volume) travel with a valid timing too
        if (!tm.valid) o.tWhy = "timing not valid" + std::string(in.fWhy.empty() ? "" : "; " + in.fWhy);
    } else o.tWhy = in.fWhy;
    // the chain: stage2 once the 1st is IN (the reclaim window, conditioned on no reclaim by now); stage3 once reclaimed.
    // Each number has one source: after the reclaim the 2nd-extreme call is stage3's, before it the read's afterT.
    if (o.callIn && std::isfinite(in.depthAtr) && in.t1Mod >= 0 && in.reclMod < 0) {
        hlr::Stage2 s2 = hlr::stage2(in.market, in.t1Mod, in.depthAtr, in.now);
        if (s2.valid) {
            if (s2.w < 0) o.s2none = true;
            else { o.s2 = true; o.s2from = s2.from; o.s2to = s2.to; o.s2p = s2.p; o.s2below = !s2.qualified; o.s2show = shownP(s2.qualified, s2.p, s2.show); }
        }
    }
    // stage 3 only once the standing 1st extreme is called IN (STAGE3_NEEDS_IN): the chain was fitted and validated on the FINAL
    // 1st extreme (hodlod_read.py chain_days, ex-post); live, the standing 1st is "reclaimed" at the first read bar on ~97% of
    // sessions (the opening bar's own extreme), so without this gate stage 3 would speak for an extreme that is often not the 1st.
    if (in.reclMod >= 0 && in.t1Mod >= 0 && (o.callIn || !STAGE3_NEEDS_IN)) {
        hlr::Stage3 s3 = hlr::stage3(in.market, in.t1Mod, in.reclMod, in.now);
        if (s3.valid) {
            o.gapMin = s3.gapMin; o.callSrc = 3;
            if (s3.afterT >= 0 && std::isfinite(s3.afterP)) { o.callT = s3.afterT; o.callP = s3.afterP; o.callQ = s3.qualified; o.callShow = shownP(s3.qualified, s3.afterP, s3.show); }
            else o.callLate = true;
            o.callDim = !o.callQ || S3_IN_TEST[in.market];
            return o;
        }
    }
    if (o.tOk) { o.callSrc = 2; o.callP = o.pClose; o.callShow = o.pClose; }   // the close-push form (pClose is near calibrated, report section 5)
#else
    (void)in;
#endif
    return o;
}

// ------------------------------------------------------------------------------------------------ the view (every value drawn)
static const unsigned C_GREEN = 0x0022C55E, C_RED = 0x00EF4444, C_GREY = 0x00B8BEC8, C_SLATE = 0x00E5E7EB;   // (1.1.2) grey lifted for reading
static const unsigned C_PANEL = 0x000F1520, C_BORDER = 0x00374151;

struct Seg { std::string s; unsigned col; };
typedef std::vector<Seg> Line;
// a table cell: up to three differently coloured parts, "8:51" + " LOD" + "  in" (s2 after one space, s3 after two)
struct LvSeg { std::string s; unsigned col; };
struct Cell { std::string s; unsigned col = 0x009CA3AF; bool bold = false; std::string s2; unsigned col2 = 0; std::string s3; unsigned col3 = 0;
              std::vector<LvSeg> lv; };   // (HL105) the swept levels' short codes after the side word: "LonLO \xB7 CR"
inline Cell mkCell(const std::string& s, unsigned c, bool b = false) { Cell x; x.s = s; x.col = c; x.bold = b; return x; }
inline std::string cellLv(const Cell& c) { std::string t; for (size_t i = 0; i < c.lv.size(); i++) t += (i ? " \xB7 " : " ") + c.lv[i].s; return t; }
inline std::string cellTxt(const Cell& c) { std::string t = c.s; if (!c.s2.empty()) t += " " + c.s2; t += cellLv(c); if (!c.s3.empty()) t += "  " + c.s3; return t; }

struct View {
    bool has = false, prior = false;
    // (2.6.0) the PRE-OPEN BOOK row (PreOpenLogic.h): shown only while preOn; preHover = its hover lines (question / what / why / how / example)
    bool preOn = false; std::string preText; unsigned preCol = 0x00B8BEC8; std::vector<Line> preHover;
    // (2.7.0) the day type (replaces the header trend text while called; empty = the trend word as before) and the note rows under
    // the table (News window, Feed ?): label + one line of text + its hover
    std::string dayText; unsigned dayCol = 0x00B8BEC8; std::vector<Line> dayHover;
    struct Note { std::string label, text; unsigned col = 0x00B8BEC8; std::vector<Line> hover; };
    std::vector<Note> notes;
    std::string market, session;           // session = YYYY-MM-DD of the RTH shown
    int side = 0;                          // 0 not clear, 1 LOD first, 2 HOD first
    bool secondPrinted = false, reclaimed = false, complete = false;
    // (HL101) running vs final: secondPrinted = the path rule (a new opposite extreme after the reclaim) - still a RUNNING high / low
    // until the close; secondFinal only at the close. firstFinal = the read called the standing 1st extreme IN (or the close).
    bool secondFinal = false, firstFinal = false, firstCalledIn = false;
    // values (minutes / ticks); -1 = n/a
    long long open = 0, close = 0, hod = 0, lod = 0;
    int hodMod = -1, lodMod = -1, reclMod = -1, lastMod = -1;
    long long took = -1, recTook = -1, gap = -1;   // recTook (v12 "Rec. Took") = first extreme -> opening reclaim
    long long sizeT = 0, rangeT = 0;
    double sizeUsd = 0, rangeUsd = 0;
    int usedPct = -1;
    int hodLvl = -1, lodLvl = -1;
    long long hodLvlPx = 0, lodLvlPx = 0;          // the swept levels' prices (ticks)
    struct LvlLbl { std::string txt; long long px = 0; unsigned col = 0; bool fin = false; };
    std::vector<LvlLbl> lvlLbls;                   // (HL104) "LonLO 7,838.25", "CR 7,850" ...: right of the candle, each at its own price
    std::vector<long long> levelTicks;             // (2.0.4) every key level + the latest MenthorQ levels (ticks): the rows the Dealer Profile
                                                   //         level badges ("94.00 G1  33%  /  50%") sit on - the projection callout keeps off them
    std::string sweptLod, sweptHod, mqWhy;
    std::vector<LvSeg> lodCodes, hodCodes;         // (HL105) the swept-level codes shown in the A row (candle off)         //         every level each extreme swept (shown + the rest), for the status file
    bool hodFinal = false, lodFinal = false;       //         coloured once that extreme is final (called IN / the close), else grey
    // the model row in use
    bool modelUp = true, modelByShare = false; int sharePct = 0;
    double eLod = NAN, eHod = NAN;         // E-LOD / E-HOD prices (SessionInfo ticks)
    // drawn text
    std::string header, headLeft; unsigned headerCol = C_SLATE;   // headLeft = "ES  10:00" (market + last closed bar)
    std::vector<Line> top, bottom; std::vector<Line> bodyMid, bodyRecl; unsigned bodyCol = C_GREEN;
    Cell E[13], A[13];                   // every value, by key (NALL); the table shows SHOWN (static_assert below)
    int win1a = -1, win1b = -1, win2a = -1, win2b = -1;
    int gapBucket = -2; double gapP25 = NAN, gapP75 = NAN;
    int recWin = -1; double recP = NAN;                    // (v15) the reclaim window (minutes after the 1st extreme) and its share
    int recFrom = -1, recTo = -1;                          // the reclaim window shown (minute of day) - the A tag's reference
    double s1p = NAN, s1pLod = NAN;                        // hlr::stage1: P(1st in its window), P(the LOD prints first)
    // where each E cell comes from (status file): the timing agent's chained stages (hlr::stage1/2/3) are wired here when its
    // header ships them (it does since 19:45); a stage not yet reachable falls back to the static tables (HodLodMedians.h)
    std::string src1st = "static", srcRecl = "static", src2nd = "static";   // (v13) the E Gap's bucket (-1 = all sessions, -2 = n/a) and middle half   // the E windows (minute of day) the A times are tagged against
    bool haveHle = false; hle::Expect ex; std::string hleWhy;    // the v2 expected model at the last closed bar (current session)
    int callT = -1, callSrc = 0; double callP = NAN; bool callIn = false;     // the 2nd-extreme call in the E cell / header
    double callShow = NAN; bool callQ = false, callLate = false, callDim = false;
    std::string headMain, headTail, headTailShort, headLeftTxt; unsigned headTailCol = C_SLATE;
    // (2.0.5, Rassul 2026-10-13 21:01 / 21:20) the header reads "Regime: <Gamma> / <Trend>": the VWAP trend of the RTH shown, computed
    // natively from the chart's CLOSED bars with VwapTrendLogic.h (the rule the Tape Reader TREND vote uses). trendLvl = vtl::level
    // (+2 Strong Uptrend .. -2 Strong Downtrend, 0 = Range); trendReady = 20+ RTH bars (before that no trend is shown)
    int trendLvl = 0; bool trendReady = false; int trendBars = 0; std::string trendVia;
    std::vector<Seg> sessSegs;       // (2.0.2, Rassul 05:52 "keep the RTH header but add the Regime and Options Expiry here") after "ES RTH 8:30-3:00"
    std::vector<Seg> fxSegs;         // (2.0.0, Rassul 2026-10-12 21:47 "combine the hod lod indicator and the first extreme") the 1st Extreme line (3rd header line); empty = none
    std::vector<Seg> headLeftSegs;   // (1.1.2, Rassul 10-12 "add colour") the left half in parts: "ES RTH 8:30-3:00" | "Range 70%" | "Proj LOD ..."   // (HL101) the header in two colours: the read + the 2nd call
    // (1.1.0, Rassul 2026-10-11 16:54) the PROJECTED opposite extreme: the 1st extreme + / - the expected full RTH range (hle median,
    // never below the range so far). Study lra.ext_yardstick: the expected range beat ATR (and ATR + range used) in 6 of 7 markets
    // on unseen days (crude a tie). Drawn as a vertical line from the 1st extreme with a flag + "Proj HOD 7,870.00"; in the header.
    static const int PROJ_WAIT_MIN = 30;   // minutes after the RTH open before the projection shows
    bool projHit = false; int projHitMod = -1;   // (2.2.2) price already traded through the target (the plugin finds the bar)
    bool projOk = false, projUp = true; long long projFrom = 0, projTo = 0, projFromEnd = 0; std::string projTxt, projLv; double projPx = NAN;   // (2.2.0) projLv = the price / gamma level at the target ("ONH \xB7 CR")
    double projRange = NAN; int projStage = -1; bool projV2 = false;   // (1.1.5) for the projection log (HodLod.proj-<MKT>.csv)
    int projLockMod = -1;   // (1.1.6) the minute of day the shown target was set (locked)
    // (2.1.0, Rassul 2026-10-14 16:01 "a line in the HOD/LOD indicator box that identifies the key GEX levels where a reaction can
    // occur so it is front and center") the GEX line under the header: at most 3 strikes from LRA-GexNodes-<MKT>.csv (HodLodGex.h),
    // nearest the price first; grow = the up arrow; gexStale = "STALE 25m" (the strikes dimmed); gexHover = the detail on mouse-over
    struct GexItem { std::string strike, name; bool grow = false, other = false, above = false, mid = false; char mark = 0; };   // (2.3.0) mark 'U' / 'D' = a drawn up / down triangle before the first strike of a side   // (2.2.4) mid = the current price between the 3 above and the 3 below   // (2.1.1) above = above price (red), else below (green)   // (2.1.0 rule 2.0) strike = the location's range, name = W / F / M, other = the other side
    std::vector<GexItem> gexItems; std::string gexStale; std::vector<Seg> gexHover;
    // (2.3.0) MenthorQ Q-Scores (HodLodQ.h): "Q: " + "M1" + " O1" + " V4" top left of the header; qHover = the mouse-over lines, each a run of coloured parts
    std::vector<Seg> qSegs; std::vector<Line> qHover;
};
// (v15) every E / A value has a key; the status file writes all of them, the table draws the keys in SHOWN.
enum { K_LABEL = 0, K_1ST, K_RECL, K_2ND, K_GAP, K_TOOK, K_SIZE, K_RECTOOK, K_ROOM, K_RANGE, K_CLOSE, K_DIR1, K_DIR2, NALL };
static const int NCOL = NALL;
static const char* const COLS[NALL] = { "", "1st Time", "Reclaim", "2nd Time", "Gap", "Took", "Size", "Rec. Took", "Room", "Range", "Close", "1st Dir", "2nd Dir" };
// the table's columns (Rassul 2026-10-09 v15: 1st Time | Reclaim | 2nd Time). Adding Gap back is this one line: { ..., K_2ND, K_GAP }
static const int SHOWN[] = { K_LABEL, K_1ST, K_2ND };   // (2.0.2, Rassul 2026-10-13 05:31 "get rid of the middle reclaim column")
static const int NSHOWN = (int)(sizeof(SHOWN) / sizeof(SHOWN[0]));
// (v12) the first-extreme read is shown as a call only while the read model is in test: "LOD IN (model in test)"; its P is logged
static const bool READ_IN_TEST = false;   // (coordinator 2026-10-09 20:30) the IN read is LIVE: "LOD IN 87%" (pInShow)
static_assert(sizeof(((View*)0)->E) / sizeof(Cell) == NALL, "View cells must match the keys");
static const unsigned C_AMBER = 0x00F59E0B;     // only for an early / late tag
static const unsigned C_BLUE = 0x0060A5FA;      // (1.1.2) the range used while the day still has room (below the expected band)
static const unsigned C_DIM = 0x006B7280;       // (HL101) a call that is not qualified (or in test): shown, with its honest %, dimmer
static const int WIN_HALF = 30;                 // the E 1st-time window: the model's first-extreme time +- 30 min

inline std::string ptsTxt(long long ticks, double tick, int dec) { return trimNum((double)ticks * tick, dec) + "p"; }
inline std::string ptsMean(double pts, double tick, int dec) { double q = std::llround(pts / tick) * tick; return trimNum(q, dec) + "p"; }

// the model (up = LOD first, down = HOD first) for this market and side; side 0 = the more common one, with its share
inline const hlx::Model* modelFor(int mkt, int side, bool& up, bool& byShare, int& sharePct)
{
    const hlx::MarketModels* M = hlx::modelsFor(MKTS[mkt].code);
    if (!M) return nullptr;
    byShare = side == 0;
    if (side == 1) up = true; else if (side == 2) up = false; else up = M->up.rawPaths >= M->down.rawPaths;
    int tot = M->up.rawPaths + M->down.rawPaths;
    sharePct = tot > 0 ? (int)std::llround(100.0 * (up ? M->up.rawPaths : M->down.rawPaths) / tot) : 0;
    return up ? &M->up : &M->down;
}

// the 2nd-extreme call text, the SAME for the header and the E 2nd Time cell: "after 1:00 80%" / "late-day"
inline std::string callTxt(const ReadOut& r)
{
    if (r.callSrc == 2 && std::isfinite(r.pClose)) {
        // (1.1.2, Rassul 10-12 "why does it say close 6%" -> "more readable"): "at close 6% \xB7 likely 12:15-2:30"
        std::string t = "at close " + std::to_string((int)std::llround(r.pClose * 100)) + "%";
        if (r.elseFrom >= 0 && r.elseTo > r.elseFrom) t += " \xB7 likely " + clk(r.elseFrom) + "-" + clk(r.elseTo);
        return t;
    }
    if (r.callSrc != 3) return "";
    if (r.callT >= 0 && std::isfinite(r.callShow)) return "after " + clk(r.callT) + " " + std::to_string((int)std::llround(r.callShow * 100)) + "%";
    if (r.callLate) return "late-day";
    return "";
}
// the header line from the read (or why there is none)
// the header in two parts: main (the 1st-extreme read) + tail (the 2nd-extreme call, the SAME text as the E 2nd Time cell);
// tailDim = the tail is drawn grey (a stage-3 call that is not qualified, or in test)
inline void readParts(const ReadOut& r, int readOpen, int readN, std::string& main, std::string& tail, bool& tailDim)
{
    char b[160];
    std::string c = callTxt(r);
    tail.clear(); tailDim = r.callDim;
    if (!c.empty()) {
        const bool named = r.state == 1 && !r.fomcPause && r.pIn > 0.30;   // (1.1.2) as the header: the opposite side once the 1st is named
        std::string who = r.callIn ? std::string("expect ") + (r.side == 0 ? "HOD" : "LOD") : (named ? std::string(r.side == 0 ? "HOD" : "LOD") : std::string("2nd"));
        tail = std::string("   \xB7   ") + who + (r.callSrc == 2 ? ": " : " ") + c;
    }
    if (r.state == 0) { main = "read: building"; return; }
    if (r.state == 2) { main = "read from " + clkAmPm(readOpen + readN); return; }
    if (r.state == 3) { main = "read: n/a (" + r.why + ")"; return; }
    // first-extreme read (HodLodReadTables.h display rule): IN at pIn >= 70%, NOT IN at <= 30%, between = no call ("?")
    const char* a = r.side == 0 ? "LOD" : "HOD";
    if (r.fomcPause) { snprintf(b, sizeof b, "%s so far \xB7 FOMC pause", a); main = b; return; }   // (coordinator 20:30)
    int p = (int)std::llround((std::isfinite(r.pInShow) ? r.pInShow : r.pIn) * 100);   // the calibrated % (hlr pInShow)
    if (READ_IN_TEST) {       // (v12) the call only, while the model is in test (P is in the logs and the status file)
        if (r.pIn >= IN_THR - 1e-12) snprintf(b, sizeof b, "%s IN  (model in test)", a);
        else if (r.pIn <= 0.30) snprintf(b, sizeof b, "%s NOT IN  (model in test)", a);
        else snprintf(b, sizeof b, "%s IN?  (model in test)", a);
    }
    else if (r.pIn >= IN_THR - 1e-12) snprintf(b, sizeof b, "%s IN %d%%", a, p);
    else if (r.pIn <= 0.30) snprintf(b, sizeof b, "%s NOT IN %d%%", a, 100 - p);
    else snprintf(b, sizeof b, "%s IN %d%%?", a, p);
    main = b;
}
inline std::string readLine(const ReadOut& r, int readOpen, int readN)
{
    std::string m, t; bool d; readParts(r, readOpen, readN, m, t, d); return m + t;
}

// build every drawn value from the tracker. `rd` = the read at the latest closed bar (only used for the live session).
struct ReadRow { std::string session; int barEnd = 0; ReadIn in; ReadOut out; std::string shown; char firstSide = '?'; long long firstPx = 0;
                 long long readPx = 0; bool sameBar = false; };   // (1.1.8) the read side's extreme price; HOD and LOD printed on the same bar

// (1.1.8, review 2026-10-12 "one immutable first call per market/session ... show formation and declaration times distinctly"):
// the day's FIRST IN call - the first closed RTH bar whose read is IN (pIn >= IN_THR, no FOMC pause). Never revised: a later,
// stronger call on the other side does not replace it, and a failed call stays recorded. formed = the bar the called extreme
// printed on (END, minute of day); declared = the bar the call was made on. Same-bar HOD / LOD order is unresolved from OHLC
// and is flagged, not guessed.
struct FirstCall { bool ok = false; std::string session; int side = 0; long long px = 0; int formed = -1, declared = -1;
                   double pIn = NAN, pShow = NAN; bool sameBar = false; std::string readVer; };
inline FirstCall firstCall(const std::vector<ReadRow>& rows)
{
    FirstCall f;
    for (size_t i = 0; i < rows.size(); i++) {
        const ReadRow& r = rows[i];
        if (r.out.state != 1 || !r.out.callIn || r.readPx == 0 || r.in.t1Mod < 0) continue;
        f.ok = true; f.session = r.session; f.side = r.in.side; f.px = r.readPx; f.formed = r.in.t1Mod; f.declared = r.barEnd;
        f.pIn = r.out.pIn; f.pShow = r.out.pInShow; f.sameBar = r.sameBar; f.readVer = r.out.ver;
        return f;
    }
    return f;
}
static const char* const FIRSTCALL_HEADER = "session,side,price,formed,declared,lag_min,p_in,p_shown,threshold,same_bar,read_model,version";

// every drawn value. `rd` = the read at the latest closed bar; `rows` = the session's per-bar reads (once the 2nd extreme printed, the
// last 2nd-extreme call shown before it is frozen into the E cell and the header, so the A tag is judged against what was shown)
// (1.1.6, Rassul 2026-10-12 16:15 "it shouldn't update after the 1st extremity occurs. it should only update if the 1st extremity
// is determined to be incorrect"): the projection is LOCKED to the first extreme it was set from - computed once, at the later of
// (30 min after the open, the bar the current 1st extreme became the 1st extreme), and changed only when price takes that extreme
// out (a new HOD past a HOD-first, a new LOD past a LOD-first, which also flips the side). Built from the closed bars up to the
// lock bar only, so a restart gives the same target (no repaint).
struct ProjLock { bool on = false; bool up = true; long long from = 0, to = 0; double range = NAN; int stage = -1; bool v2 = false; int lockMod = -1; };
inline View buildView(const Tracker& T, const ReadOut& rd, const std::vector<ReadRow>* rows = nullptr, const std::vector<MqSnap>* mq = nullptr, const ProjLock* pl = nullptr)
{
    View v; const Mkt& K = MKTS[T.C.mkt]; v.market = K.code; v.headLeft = K.code;
    const double tick = T.C.tick; const int dec = K.dec; const double tv = tick * K.pv;
    const Rth* R = T.cur.any ? &T.cur : (T.last.any ? &T.last : nullptr);
    if (!R) { v.header = "waiting for the RTH open " + clkAmPm(K.open); v.headerCol = C_GREY; return v; }
    v.has = true; v.prior = R != &T.cur; v.session = dateTxt(R->sid); v.complete = R->complete || v.prior;
    long long day = R->sid * 86400;
    v.open = R->open; v.close = R->lastClose; v.hod = R->hod.px; v.lod = R->lod.px;
    v.hodMod = (int)((R->hod.endAbs - day) / 60); v.lodMod = (int)((R->lod.endAbs - day) / 60); v.lastMod = (int)((R->lastEndAbs - day) / 60);
    v.hodLvl = R->hod.lvl; v.lodLvl = R->lod.lvl; v.hodLvlPx = R->hod.lvlPx; v.lodLvlPx = R->lod.lvlPx;
    {   // (2.0.4, 2026-10-13 Rassul screenshot: the callout's price "94.00" sat on the G1 badge) the level rows the badges sit on
        const Lvl* KL[4] = { &T.pf, &T.pd, &T.curOn, &T.curLon };
        for (int i = 0; i < 4; i++) if (KL[i]->ok && KL[i]->n > 0) { v.levelTicks.push_back(KL[i]->hi); v.levelTicks.push_back(KL[i]->lo); }
        if (mq) {
            const long long sesA0 = (R->sid - 1) * 86400 + 17 * 3600;
            const MqSnap* sn = nullptr;
            for (size_t i = 0; i < mq->size(); i++) { const MqSnap& q = (*mq)[i]; if (q.atAbs >= sesA0 && q.atAbs <= R->lastEndAbs && (!sn || q.atAbs >= sn->atAbs)) sn = &q; }
            if (sn) for (size_t i = 0; i < sn->lv.size(); i++) v.levelTicks.push_back(sn->lv[i].px);
        }
    }

    v.headLeft = std::string(K.code) + "  " + clk(v.lastMod);
    v.side = R->lod.seq < R->hod.seq ? 1 : (R->hod.seq < R->lod.seq ? 2 : 0);
    v.rangeT = R->hod.px - R->lod.px; v.rangeUsd = (double)v.rangeT * tv;
    v.bodyCol = R->lastClose >= R->open ? C_GREEN : C_RED;
    const Ext* F = v.side == 1 ? &R->lod : (v.side == 2 ? &R->hod : nullptr);
    const Ext* S = v.side == 1 ? &R->hod : (v.side == 2 ? &R->lod : nullptr);
    const Mark* RC = v.side == 1 ? &R->lodRecl : (v.side == 2 ? &R->hodRecl : nullptr);
    if (F) {
        v.took = (F->endAbs - R->openStartAbs) / 60;
        v.sizeT = F->px > R->open ? F->px - R->open : R->open - F->px; v.sizeUsd = (double)v.sizeT * tv;
        v.reclaimed = RC->seq >= 0;
        if (v.reclaimed) { v.reclMod = (int)((RC->endAbs - day) / 60); v.recTook = (RC->endAbs - F->endAbs) / 60; }
        v.secondPrinted = v.complete || (v.reclaimed && RC->seq < S->seq);
        if (v.secondPrinted) v.gap = (S->endAbs - F->endAbs) / 60;
        // (HL101) the 1st extreme is "LOD" / "HOD" only once the read called THIS extreme IN (same side, same price) or at the close
        const char fs = v.side == 1 ? 'L' : 'H';
        if (rows && !v.prior) for (size_t i = 0; i < rows->size() && !v.firstCalledIn; i++) {
            const ReadRow& w = (*rows)[i];
            if (w.session == v.session && w.out.callIn && w.firstSide == fs && w.firstPx == F->px) v.firstCalledIn = true;
        }
        if (!v.prior && rd.callIn && (rd.side == 0) == (v.side == 1)) {
            const Ext& RF = rd.side == 0 ? R->lod : R->hod;
            if (RF.px == F->px) v.firstCalledIn = true;
        }
    }
    v.secondFinal = v.complete; v.firstFinal = v.complete || v.firstCalledIn;
    v.lodFinal = v.complete || (v.side == 1 && v.firstFinal); v.hodFinal = v.complete || (v.side == 2 && v.firstFinal);
    {   // (HL104) the levels each extreme swept: the key level (PF / PD / ON / Lon), then his MenthorQ levels in priority order, each
        // its own label at its own price; at most 3 per extreme drawn, all of them in the status file. MenthorQ: the file's version
        // that stood at the extreme's bar (the latest snapshot written at or before it, else the session's first one); a snapshot
        // written before this session (17:00 the day before) is stale and not used.
        long long sesA = (R->sid - 1) * 86400 + 17 * 3600;
        for (int low = 1; low >= 0; low--) {
            const Ext& E = low ? R->lod : R->hod; if (!E.set) continue;
            bool fin = low ? v.lodFinal : v.hodFinal;
            std::vector<View::LvlLbl> got; std::string all;
            int kl = low ? v.lodLvl : v.hodLvl; long long kpx = low ? v.lodLvlPx : v.hodLvlPx;
            if (kl >= 0) { View::LvlLbl a; a.txt = std::string(low ? LV_LOW[kl] : LV_HIGH[kl]) + " " + lvlPxTxt((double)kpx * tick, dec); a.px = kpx; a.col = LV_COL[kl]; a.fin = fin; got.push_back(a); }
            if (mq && !v.prior) {
                const MqSnap* sn = nullptr;
                const MqSnap* first = nullptr;
                for (size_t i = 0; i < mq->size(); i++) {
                    const MqSnap& q = (*mq)[i]; if (q.atAbs < sesA) continue;
                    if (!first || q.atAbs < first->atAbs) first = &q;
                    if (q.atAbs <= E.endAbs && (!sn || q.atAbs >= sn->atAbs)) sn = &q;
                }
                if (!sn) sn = first;
                if (!sn) v.mqWhy = "MenthorQ levels: none for this session (missing or stale file)";
                else for (int code = 0; code < 6; code++) for (size_t i = 0; i < sn->lv.size(); i++) {
                    const MqLevel& q = sn->lv[i]; if (q.code != code) continue;
                    bool sw = low ? (E.px < q.px && R->open >= q.px) : (E.px > q.px && R->open <= q.px);
                    if (!sw) continue;
                    View::LvlLbl a; a.txt = std::string(MQ_CODES[code]) + " " + lvlPxTxt((double)q.px * tick, dec); a.px = q.px; a.col = q.col; a.fin = fin;
                    bool dup = false; for (size_t j = 0; j < got.size(); j++) if (got[j].txt == a.txt) dup = true;
                    if (!dup) got.push_back(a);
                }
            }
            for (size_t i = 0; i < got.size(); i++) { all += (all.empty() ? "" : ";") + got[i].txt; if (i < 3) v.lvlLbls.push_back(got[i]); }
            std::vector<LvSeg>& cs = low ? v.lodCodes : v.hodCodes;               // (HL105) at most 2 short codes for the table's A row
            for (size_t i = 0; i < got.size() && i < 2; i++) { LvSeg g; g.s = got[i].txt.substr(0, got[i].txt.find(' ')); g.col = got[i].fin ? got[i].col : C_GREY; cs.push_back(g); }
            (low ? v.sweptLod : v.sweptHod) = all;
        }
    }
    // the model row: hle 2.0 (the studies' conditional model, current session) where its `sources` bits say v2; the v1 model
    // candles (HodLodExpected.h) for what hle does not model (reclaim, wick) and as hle's own fallback. Times are minutes since
    // THIS market's open (hle's convention; v1 used one 08:30 window for every market).
    const hlx::Model* Mo = modelFor(T.C.mkt, v.side, v.modelUp, v.modelByShare, v.sharePct);
    v.ex.valid = false; v.ex.sources = 0;
    double projRange = NAN;   // (1.1.4) the range the projection and Range % use
    int projStage = -1; bool projV2 = false;
    if (!v.prior) {
        hle::State hs = hleState(T, *R, v.hleWhy);
        v.ex = hle::expect(hs); v.haveHle = v.ex.valid;
        projRange = v.ex.range;
        projStage = v.ex.stage; projV2 = (v.ex.sources & hle::SRC_RANGE) != 0;
        // (1.1.4, Rassul 2026-10-12 "really good ... except cl"): where the v2 range model has no fit for this 30-min stage (CL at
        // 120 and 300 min), hle falls back to the v1 mean range (CL 2.843 = a target 1.8 pts away that jumped at 10:00 and 13:00).
        // Use the latest earlier stage that has a v2 fit instead (same inputs, never below the range so far; no repaint - each
        // bar uses only the closed bars up to it)
        if (v.haveHle && !(v.ex.sources & hle::SRC_RANGE)) {
            for (int back = 30; back < hs.mso; back += 30) {
                hle::State h2 = hs; h2.mso = hs.mso - back;
                hle::Expect e2 = hle::expect(h2);
                if (e2.valid && (e2.sources & hle::SRC_RANGE)) { projRange = std::max(e2.range, (double)v.rangeT * tick); projStage = e2.stage; projV2 = true; v.hleWhy += "range: stage " + std::to_string(v.ex.stage) + " has no v2 fit, used stage " + std::to_string(e2.stage) + "; "; break; }
            }
        }
        if (!v.haveHle && v.hleWhy.empty()) v.hleWhy = "hle not valid for this state; ";
    } else v.hleWhy = "prior session: v1 model; ";
    double eT2 = v.haveHle ? v.ex.t2 : (Mo ? Mo->openToSecond : NAN); (void)eT2;   // used only without the read model
    double eSize = NAN;                                   // set with the E row (hle wick, else the median)
    double eRange = v.haveHle ? projRange : (Mo ? Mo->rangePts : NAN);
    // (Rassul 17:18 "wait at least 30 min before drawing the projection"): from 30 minutes after this market's RTH open
    if (!v.prior && v.haveHle && F && std::isfinite(projRange) && projRange > 0 && v.lastMod - K.open >= View::PROJ_WAIT_MIN) {
        long long rt = (long long)std::llround(projRange / tick);
        if (rt < v.rangeT) rt = v.rangeT;                                    // never inside the range already printed
        v.projOk = true; v.projUp = v.side == 1; v.projFrom = F->px; v.projFromEnd = F->endAbs;
        v.projRange = projRange; v.projStage = projStage; v.projV2 = projV2;
        v.projTo = v.projUp ? F->px + rt : F->px - rt;
        if (pl && pl->on && pl->up == v.projUp && pl->from == F->px) {      // (1.1.6) the locked target for this 1st extreme
            v.projTo = pl->to; v.projRange = pl->range; v.projStage = pl->stage; v.projV2 = pl->v2; v.projLockMod = pl->lockMod;
            eRange = pl->range;                                              // Range % against the target shown
        }
        v.projTxt = std::string(v.projUp ? "Proj HOD " : "Proj LOD ") + pxTxt((double)v.projTo * tick, dec);
        {   // (2.2.0, Rassul 2026-10-15 12:01 "next to the projection add any levels you see near the projection: Proj HOD nnnn <price level>
            // <gamma level>") the nearest key level and the nearest MenthorQ level within 5% of the projected range (4 ticks min)
            // (2.2.1, 12:18 screenshot: CR 7775 sat 6.5 pts from Proj HOD 7,781.50 and was not named) 10% of the projected range, 8 ticks min
            const long long tol = (std::max)(8LL, (long long)std::llround(0.10 * v.projRange / tick));
            v.projPx = (double)v.projTo * tick;
            std::string kn; long long kd = tol + 1;
            const Lvl* KL[4] = { &T.pf, &T.pd, &T.curOn, &T.curLon };
            for (int i = 0; i < 4; i++) if (KL[i]->ok && KL[i]->n > 0) {
                const long long d1 = std::llabs(KL[i]->hi - v.projTo), d2 = std::llabs(KL[i]->lo - v.projTo);
                if (d1 < kd) { kd = d1; kn = std::string(LV_HIGH[i]) + " " + lvlPxTxt((double)KL[i]->hi * tick, dec); }
                if (d2 < kd) { kd = d2; kn = std::string(LV_LOW[i]) + " " + lvlPxTxt((double)KL[i]->lo * tick, dec); }
            }
            std::string gn; long long gd = tol + 1;
            if (mq) {
                const long long sesA0 = (R->sid - 1) * 86400 + 17 * 3600; const MqSnap* sn = nullptr;
                for (size_t i = 0; i < mq->size(); i++) { const MqSnap& q = (*mq)[i]; if (q.atAbs >= sesA0 && q.atAbs <= R->lastEndAbs && (!sn || q.atAbs >= sn->atAbs)) sn = &q; }
                if (sn) for (size_t i = 0; i < sn->lv.size(); i++) {
                    const MqLevel& q = sn->lv[i]; if (q.code < 0 || q.code > 5) continue;
                    const long long d = std::llabs(q.px - v.projTo); if (d < gd) { gd = d; gn = std::string(MQ_CODES[q.code]) + " " + lvlPxTxt((double)q.px * tick, dec); }
                }
            }
            v.projLv = kn + (gn.empty() ? std::string() : (kn.empty() ? std::string() : std::string(" \xB7 ")) + gn);
        }
    }
    if (std::isfinite(eRange) && eRange > 0) v.usedPct = (int)std::llround(100.0 * (double)v.rangeT * tick / eRange);
    // ---- the candle labels (mockup v5)
    auto nameLine = [&](bool isHod, int lvl) {
        Line L; L.push_back(Seg{ isHod ? "HOD" : "LOD", isHod ? C_GREEN : C_RED });
        (void)lvl;   // (HL104) the swept level's name now sits right of the candle at its price, not in this label
        return L;
    };
    auto one = [](const std::string& s, unsigned c) { Line L; L.push_back(Seg{ s, c }); return L; };
    // (HL101) a running extreme (not final yet): "high 11:21 \xB7 PDH" all grey, no side colour
    auto runLine = [&](bool isHod, int mod, int lvl) {
        Line L; std::string t = std::string(isHod ? "high " : "low ") + clkPm(mod);
        (void)lvl;
        L.push_back(Seg{ t, C_GREY }); return L;
    };
    std::vector<Line> hodG, lodG;
    std::string rangeTxt = ptsTxt(v.rangeT, tick, dec), rangeUsdTxt = usd(v.rangeUsd);
    if (v.side == 0) {
        if (v.complete) {
            hodG.push_back(nameLine(true, v.hodLvl)); lodG.push_back(nameLine(false, v.lodLvl));
            hodG.push_back(one(clkPm(v.hodMod), C_GREEN)); lodG.push_back(one(clkPm(v.lodMod), C_RED));
        } else { hodG.push_back(runLine(true, v.hodMod, v.hodLvl)); lodG.push_back(runLine(false, v.lodMod, v.lodLvl)); }
        v.bodyMid.push_back(one(rangeTxt, C_SLATE)); v.bodyMid.push_back(one(rangeUsdTxt, C_SLATE));
    } else {
        bool lodFirst = v.side == 1;
        std::vector<Line>& FG = lodFirst ? lodG : hodG; std::vector<Line>& SG = lodFirst ? hodG : lodG;
        unsigned fc = lodFirst ? C_RED : C_GREEN, sc = lodFirst ? C_GREEN : C_RED;
        int fMod = lodFirst ? v.lodMod : v.hodMod, sMod = lodFirst ? v.hodMod : v.lodMod;
        int fLvl = lodFirst ? v.lodLvl : v.hodLvl, sLvl = lodFirst ? v.hodLvl : v.lodLvl;
        if (v.firstFinal) { FG.push_back(nameLine(!lodFirst, fLvl)); FG.push_back(one(clkPm(fMod), fc)); FG.push_back(one(durC(v.took), fc)); FG.push_back(one(ptsTxt(v.sizeT, tick, dec), fc)); }
        else { FG.push_back(runLine(!lodFirst, fMod, fLvl)); FG.push_back(one(durC(v.took), C_GREY)); FG.push_back(one(ptsTxt(v.sizeT, tick, dec), C_GREY)); }
        if (v.secondFinal) {
            SG.push_back(nameLine(lodFirst, sLvl));
            SG.push_back(one(clkPm(sMod), sc));
            SG.push_back(one(durC(v.gap), sc));
            v.bodyMid.push_back(one(durC(v.gap), C_SLATE));
        } else if (v.secondPrinted) {             // a new opposite extreme after the reclaim: the running high / low so far
            SG.push_back(runLine(lodFirst, sMod, sLvl));
            v.bodyMid.push_back(one("Pending", C_SLATE));
        } else {
            SG.push_back(nameLine(lodFirst, -1));  // the second extreme is the HOD when the LOD came first; a pending one names no level
            SG.push_back(one("Pending", sc));
            v.bodyMid.push_back(one("Pending", C_SLATE));
        }
        v.bodyMid.push_back(one(rangeTxt, C_SLATE)); v.bodyMid.push_back(one(rangeUsdTxt, C_SLATE));
        if (v.reclaimed) { v.bodyRecl.push_back(one(clk(v.reclMod), C_SLATE)); v.bodyRecl.push_back(one(durC(v.recTook), C_SLATE)); }
        else v.bodyRecl.push_back(one("no reclaim", C_SLATE));
    }
    v.top = hodG; v.bottom = lodG;
    // ---- the 2nd-extreme call shown: live (always >= now) until the close; at the close, frozen at the bar before the FINAL 2nd
    ReadOut rh = rd;
    if (rows && !rows->empty() && v.secondFinal && v.side != 0 && !v.prior && rows->front().session == v.session) {
        int m2 = v.side == 1 ? v.hodMod : v.lodMod; const ReadRow* st = nullptr;
        // the stage-3 call that counts: the LAST QUALIFIED "after T" shown before the final 2nd extreme printed (T moves with the
        // clock; the later bars showed a grey best guess, which does not erase the call shown with confidence); else the last
        // stage-3 call; else the last call of any form (close push). The A tag is judged only against a stage-3 "after T".
        for (size_t i = 0; i < rows->size(); i++) { const ReadOut& o = (*rows)[i].out; if ((*rows)[i].barEnd < m2 && o.callSrc == 3 && o.callT >= 0 && o.callQ) st = &(*rows)[i]; }
        if (!st) for (size_t i = 0; i < rows->size(); i++) { const ReadOut& o = (*rows)[i].out; if ((*rows)[i].barEnd < m2 && o.callSrc == 3 && o.callT >= 0) st = &(*rows)[i]; }
        if (!st) for (size_t i = 0; i < rows->size(); i++) { const ReadOut& o = (*rows)[i].out; if ((*rows)[i].barEnd < m2 && o.callSrc != 0 && !callTxt(o).empty()) st = &(*rows)[i]; }
        if (st) {
            const ReadOut& o = st->out;
            rh.callT = o.callT; rh.callP = o.callP; rh.callSrc = o.callSrc; rh.callShow = o.callShow; rh.callQ = o.callQ; rh.callLate = o.callLate;
            rh.callDim = o.callDim; rh.callIn = o.callIn; rh.side = o.side; rh.gapMin = o.gapMin;
            rh.tOk = o.tOk; rh.pClose = o.pClose; rh.elseFrom = o.elseFrom; rh.elseTo = o.elseTo;
        } else { rh.callT = -1; rh.callSrc = 0; rh.callLate = false; rh.callIn = false; }
    }
    if (rows && !rows->empty() && v.reclaimed && !v.prior && rows->front().session == v.session) {
        // the reclaim window that was shown before the reclaim printed (the A tag is judged against it)
        const ReadRow* st = nullptr;
        for (size_t i = 0; i < rows->size(); i++) if ((*rows)[i].barEnd < v.reclMod && (*rows)[i].out.s2) st = &(*rows)[i];
        rh.s2 = st != nullptr; if (st) { rh.s2from = st->out.s2from; rh.s2to = st->out.s2to; rh.s2p = st->out.s2p; rh.s2below = st->out.s2below; rh.s2show = st->out.s2show; }
    }
    v.callT = rh.callT; v.callP = rh.callP; v.callSrc = rh.callSrc; v.callIn = rh.callIn; v.callShow = rh.callShow; v.callQ = rh.callQ; v.callLate = rh.callLate; v.callDim = rh.callDim;
    // ---- the table (mockup v10): E row (the model) above A row (actual)
    //   1st Time | 1st Dir | Took | Size | Reclaim | Wick | 2nd Time | 2nd Dir | Gap | Range
    for (int i = 0; i < NALL; i++) { v.E[i] = mkCell("", C_GREY); v.A[i] = mkCell("", C_SLATE); }
    v.E[K_LABEL].s = "Expected"; v.A[K_LABEL] = mkCell("Actual", C_SLATE, true);   // (1.1.2, Rassul "more readable") words, not E / A
    {
        // TYPICAL values: the studies' medians by first side once it is known (HodLodMedians.h); internally consistent (v12)
        const int op = K.open;
        auto fin = [](double x) { return std::isfinite(x); };
        auto sideCol = [](bool hod) { return hod ? C_GREEN : C_RED; };
        const hlm::MarketMed* MM = hlm::medFor(K.code);
        const hlm::Med* md = MM ? &MM->g[v.side == 1 ? 1 : (v.side == 2 ? 2 : 0)] : nullptr;
        if (v.haveHle && (v.ex.sources & hle::SRC_WICK) && fin(v.ex.wick)) eSize = v.ex.wick; else eSize = md ? md->size : NAN;
        if (std::isfinite(eRange) && eRange > 0 && fin(eSize)) {
            double opx = (double)R->open * tick;
            if (v.modelUp) { v.eLod = opx - eSize; v.eHod = v.eLod + eRange; } else { v.eHod = opx + eSize; v.eLod = v.eHod - eRange; }
        }
        const int firstMod = v.side == 1 ? v.lodMod : (v.side == 2 ? v.hodMod : -1);
        // ---- 1st Time. E "8:51 +-30m  either" (no tested side lean exists yet), A "8:51 LOD  in"
#if HL_HAVE_READ
        hlr::Stage1 s1 = hlr::stage1(T.C.mkt);
#else
        struct { bool valid; } s1 = { false };
#endif
        if (s1.valid) {                                     // hlr::stage1: the window from the open, centre +- half
#if HL_HAVE_READ
            int c = (int)std::llround(s1.centre), hw = (int)std::llround(s1.half);
            v.win1a = s1.from; v.win1b = s1.to; v.src1st = "hlr::stage1";
            (void)c; (void)hw;   // (1.1.7, Rassul 18:25 "ranges instead of +-"): "8:21 - 9:21", not "8:51 \xB1" "30m"
            v.E[K_1ST].s = clk(s1.from) + " - " + clk(s1.to);
            if (std::isfinite(s1.show)) v.E[K_1ST].s += "  " + std::to_string((int)std::llround(s1.show * 100)) + "%";   // the delivered share
            v.E[K_TOOK].s = "~" + durT(c - op);
            if (md && fin(md->recTook)) v.E[K_RECTOOK].s = "~" + durT(std::llround(md->recTook));
            v.s1p = s1.p; v.s1pLod = s1.pLod;
#endif
        } else if (md && fin(md->t1)) {
            int t1 = op + (int)std::llround(md->t1);
            v.win1a = t1 - WIN_HALF; v.win1b = t1 + WIN_HALF;
            v.E[K_1ST].s = clk(t1 - WIN_HALF) + " - " + clk(t1 + WIN_HALF);   // (1.1.7) a range, not +-
            v.E[K_TOOK].s = "~" + durT(std::llround(md->t1));
            if (fin(md->recTook)) v.E[K_RECTOOK].s = "~" + durT(std::llround(md->recTook));
        }
        // the side word: a lean only when the stage says one side prints first on >= 55% of days, else "either"
        // (coordinator 20:30) no market has a validated side lean: always "either" (s1pLod stays in the status file)
        v.E[K_1ST].s3.clear();   // (1.1.2) no "either": a side word only when there is one
        v.E[K_DIR1].s = v.E[K_1ST].s3;
        // ---- Reclaim (v15): a window anchored on the 1st extreme, "<1st> - <1st + W>  P%"; before it prints "<=Wm after 1st  P%"
        if (rh.s2 && firstMod >= 0) {                      // hlr::stage2 (once the 1st is IN): the window by the 1st's time and depth
            v.recFrom = rh.s2from; v.recTo = rh.s2to; v.recP = rh.s2p; v.srcRecl = "hlr::stage2";
            v.E[K_RECL].s = clk(rh.s2from) + " - " + clk(rh.s2to) + "  " + std::to_string((int)std::llround(rh.s2show * 100)) + "%";
            if (rh.s2below) v.E[K_RECL].col = C_DIM;           // not qualified: the honest P, grey
        } else if (rh.s2none && !v.reclaimed) {            // stage2 live but no window left
            v.srcRecl = "hlr::stage2 (no window left)"; v.E[K_RECL].s = "-";
        } else if (MM && MM->rec.w > 0) {
            int W = MM->rec.w; int P = (int)std::llround(MM->rec.p * 100);
            if (firstMod >= 0) { v.recFrom = firstMod; v.recTo = firstMod + W; }
            v.recWin = W; v.recP = MM->rec.p;
            v.E[K_RECL].s = firstMod >= 0 ? clk(firstMod) + " - " + clk(firstMod + W) + "  " + std::to_string(P) + "%"
                                          : "\x3C=" + std::to_string(W) + "m after 1st  " + std::to_string(P) + "%";
        }
        v.E[K_SIZE].s = fin(eSize) ? "~" + ptsMean(eSize, tick, dec) : "-";
        // ---- 2nd Time: the read model's call (one source) + the side word: HOD once the LOD is called IN, else "either"
#if HL_HAVE_READ
        { std::string c = callTxt(rh); v.E[K_2ND].s = c.empty() ? "-" : c; if (rh.callDim) v.E[K_2ND].col = C_DIM; }
#else
        v.E[K_2ND].s = fin(eT2) ? "~" + clk(op + (int)std::llround(eT2)) : "-";    // no read model built in: hle's initial time
#endif
#if HL_HAVE_READ
        v.src2nd = rh.callSrc == 3 ? std::string("hlr::stage3") + (S3_IN_TEST[T.C.mkt] ? " (in test)" : "") : rh.callSrc == 2 ? "hlr::timing close push + else window (before the reclaim)" : "hlr: no call";
#else
        v.src2nd = "static (hle t2 / v1)";
#endif
        // (1.1.2) the 2nd's side once the read names the 1st (IN, or IN ..%?) - as the header, in the named side's colour; no "either"
        if (rh.callIn || (rh.state == 1 && !rh.fomcPause && rh.pIn > 0.30)) { v.E[K_2ND].s3 = rh.side == 0 ? "HOD" : "LOD"; v.E[K_2ND].col3 = rh.side == 0 ? C_GREEN : C_RED; }
        else v.E[K_2ND].s3.clear();
        v.E[K_DIR2] = rh.callIn ? mkCell(rh.side == 0 ? "HOD" : "LOD", sideCol(rh.side == 0)) : mkCell("either", C_GREY);
        // ---- Gap (status / optional column): the median 1st -> 2nd time by the 1st extreme's 30-min bucket once known
        v.gapBucket = -2; v.E[K_GAP].s = "-";
        if (fin(rh.gapMin)) { v.E[K_GAP].s = "~" + durT(std::llround(rh.gapMin)); v.gapBucket = -3; }   // hlr::stage3
        else if (MM) {
            const hlm::Gap* G = &MM->gapAll;
            if (v.side != 0) { int k = hlm::gapBucketOf((double)v.took); if (k >= 0 && MM->gapBucket[k].med >= 0) { G = &MM->gapBucket[k]; v.gapBucket = k; } else v.gapBucket = -1; }
            if (G->med >= 0) { v.E[K_GAP].s = "~" + durT(std::llround(G->med)); v.gapP25 = G->p25; v.gapP75 = G->p75; }
        }
        v.E[K_ROOM].s = v.haveHle && fin(v.ex.roomLeft) ? "~" + ptsMean(v.ex.roomLeft, tick, dec) : "-";
        v.E[K_RANGE].s = fin(eRange) ? "~" + ptsMean(eRange, tick, dec) : "-";
        if (v.haveHle && (v.ex.sources & hle::SRC_RANGE) && v.ex.rangeHi > v.ex.rangeLo)
            v.E[K_RANGE].s += " (" + trimNum(std::llround(v.ex.rangeLo / tick) * tick, dec) + "-" + trimNum(std::llround(v.ex.rangeHi / tick) * tick, dec) + ")";
        if (v.haveHle && (v.ex.sources & hle::SRC_CLOSE) && v.ex.closeSide != 0) {
            int pu = (int)std::llround(v.ex.pUp * 100);
            v.E[K_CLOSE] = v.ex.closeSide > 0 ? mkCell("green " + std::to_string(pu) + "%", C_GREEN) : mkCell("red " + std::to_string(100 - pu) + "%", C_RED);
        }
        // ---- A row
        auto tag = [&](Cell& c, int mod, int wa, int wb) {      // "in" slate, "early" / "late" amber
            if (wa < 0) return;
            if (mod < wa) { c.s3 = "early"; c.col3 = C_AMBER; } else if (mod > wb) { c.s3 = "late"; c.col3 = C_AMBER; } else { c.s3 = "in"; c.col3 = C_SLATE; }
        };
        if (v.side != 0) {
            bool lodFirst = v.side == 1;
            if (v.firstFinal) {
                v.A[K_1ST] = mkCell(clk(firstMod), C_SLATE); v.A[K_1ST].s2 = lodFirst ? "LOD" : "HOD"; v.A[K_1ST].col2 = sideCol(!lodFirst);
                tag(v.A[K_1ST], firstMod, v.win1a, v.win1b);
            } else v.A[K_1ST] = mkCell(std::string(lodFirst ? "LOD" : "HOD") + " so far " + clk(firstMod), C_GREY);   // (1.1.2) named   // (HL101) not called IN yet
            v.A[K_DIR1] = mkCell(lodFirst ? "LOD" : "HOD", sideCol(!lodFirst));
            v.A[K_TOOK].s = durT(v.took);
            v.A[K_SIZE].s = ptsTxt(v.sizeT, tick, dec);
            if (v.reclaimed) {
                v.A[K_RECL] = mkCell(clk(v.reclMod), C_SLATE); v.A[K_RECTOOK].s = durT(v.recTook);
                if (v.recTo >= 0) tag(v.A[K_RECL], v.reclMod, v.recFrom, v.recTo);
            } else v.A[K_RECL] = mkCell(v.complete ? "none" : "Pending", C_SLATE);
            if (v.secondFinal) {                                // against the "after T" call: in = printed after T, early = before
                int m2 = lodFirst ? v.hodMod : v.lodMod;
                v.A[K_2ND] = mkCell(clk(m2), C_SLATE); v.A[K_2ND].s2 = lodFirst ? "HOD" : "LOD"; v.A[K_2ND].col2 = sideCol(lodFirst);
                if (rh.callSrc == 3 && rh.callT >= 0) { if (m2 > rh.callT) { v.A[K_2ND].s3 = "in"; v.A[K_2ND].col3 = C_SLATE; } else { v.A[K_2ND].s3 = "early"; v.A[K_2ND].col3 = C_AMBER; } }
                v.A[K_DIR2] = mkCell(lodFirst ? "HOD" : "LOD", sideCol(lodFirst));
                v.A[K_GAP].s = durT(v.gap);
            } else if (v.secondPrinted) {                       // (HL101) a running high / low: no in / early / late, no side colour
                int m2 = lodFirst ? v.hodMod : v.lodMod;
                v.A[K_2ND] = mkCell(std::string(lodFirst ? "HOD" : "LOD") + " so far " + clk(m2), C_GREY);   // (1.1.2) named
                v.A[K_DIR2] = mkCell("", C_GREY);
                v.A[K_GAP] = mkCell(durT(v.gap) + "...", C_GREY);
            } else {
                v.A[K_2ND] = mkCell("Pending", C_SLATE);   // (05:59 "put pending like Pending")
                v.A[K_GAP] = mkCell(durT(v.lastMod - firstMod) + "...", C_GREY);   // running time since the 1st extreme
            }
        }
        v.A[K_RANGE] = mkCell(rangeTxt + (v.usedPct >= 0 ? "  " + std::to_string(v.usedPct) + "%" : ""), v.secondFinal ? C_SLATE : C_GREY);
    }
    // (HL105) candle off: the swept levels move into the A row, after the side word ("7:57 LOD LonLO \xB7 CR  in")
    if (!HODLOD_SHOW_CANDLE && v.side != 0) {
        bool lf = v.side == 1;
        v.A[K_1ST].lv = lf ? v.lodCodes : v.hodCodes;
        if (v.secondPrinted || v.secondFinal) v.A[K_2ND].lv = lf ? v.hodCodes : v.lodCodes;
    }
    // (coordinator 20:30) the 1st -> 2nd gap has no skill: no table column, only "typical gap" on the candle while the 2nd is not final
    if (v.side != 0 && !v.secondFinal && !v.bodyMid.empty() && v.E[K_GAP].s.size() > 1 && v.E[K_GAP].s[0] == '~') {
        Line L; L.push_back(Seg{ "typical gap " + v.E[K_GAP].s.substr(1), C_GREY }); v.bodyMid[0] = L;
    }
    // ---- header (HL105, Rassul "too much in the header"): "<MKT> RTH <open>-<close>   <the read>", e.g. "ES RTH 8:30-3:00   LOD IN 92%".
    //      No clock, no 2nd-extreme call (that stays in the 2nd Time cell), no "RTH complete / at the close".
    {
        auto h12 = [](int m) { char b[12]; int h = (m / 60) % 12; snprintf(b, sizeof b, "%d:%02d", h == 0 ? 12 : h, m % 60); return std::string(b); };
        std::string rth = std::string(K.code) + " RTH " + h12(K.open) + "-" + h12(K.close) + "   ";
        // (HL105, mockup 9) left-justified "ES RTH 8:30-3:00   Range 62%" | a thin separator | right-justified "LOD IN 99%   HOD after ..."
        std::string rthT = rth.substr(0, rth.size() - 3);
        if (v.prior) {
            int y, m, d; civilFromDays(R->sid, y, m, d); char b[64]; snprintf(b, sizeof b, "prior RTH %d/%02d", m, d);
            v.headLeftTxt = rthT; v.headMain = b; v.headerCol = C_GREY;
            v.headLeftSegs.push_back(Seg{ rthT, C_GREY });
        } else {
            std::string mn, tl; bool dim = false; readParts(rh, T.C.readOpen, T.C.readN, mn, tl, dim);
            // (HL105, mockup 6) the day's range used so far (today's RTH range / the expected full range, hle native)
            v.headLeftTxt = rthT + (v.usedPct >= 0 ? std::string(HL_SHOW_CALLED ? "   Range used " : "   Range ") + std::to_string(v.usedPct) + "%" : std::string())
                          + (v.projOk ? "   " + v.projTxt : std::string());   // (1.1.0) the target after Range
            v.headMain = mn;
#if HL_SHOW_CALLED
            // (1.1.9, approved mockup; review "show formation and declaration times distinctly"): the bar the day's first IN call was made on,
            // next to the read - the 1st extreme's own time (formed) stays in the 1st Time cell and on the projection's start candle
            if (rows && rd.state == 1 && rd.callIn) {
                FirstCall fc = firstCall(*rows);
                if (fc.ok && fc.side == rd.side) v.headMain += " \xB7 called " + clk(fc.declared);
            }
#endif
            v.headerCol = (v.complete || rd.state != 1) ? C_GREY : C_SLATE;
            // (1.1.2, Rassul 10-12 "colour both", "colour the range") the read in its side's colour: the LOD named (LOD IN ..) = green,
            // the HOD named = red; NOT IN / building / FOMC pause stay neutral. Range used: blue while below the expected band's low
            // end (25% of days finish there or lower: the day still has room), amber from there on (the range is typically done).
            const bool named = rd.state == 1 && !rd.fomcPause && rd.pIn > 0.30 && !v.complete;
            if (named) v.headerCol = rd.side == 0 ? C_GREEN : C_RED;
            const unsigned base = v.complete ? C_GREY : C_SLATE;
            v.headLeftSegs.push_back(Seg{ rthT, base });
            if (v.usedPct >= 0) {
                double loPct = (v.haveHle && std::isfinite(v.ex.rangeLo) && std::isfinite(v.ex.range) && v.ex.range > 0) ? 100.0 * v.ex.rangeLo / v.ex.range : 80.0;
                if (loPct > 80.0) loPct = 80.0;   // late in the day the band closes on the range so far: 80% is amber regardless
                unsigned rc = v.complete ? C_GREY : ((double)v.usedPct >= loPct ? C_AMBER : C_BLUE);
                v.headLeftSegs.push_back(Seg{ std::string(HL_SHOW_CALLED ? "   Range used " : "   Range ") + std::to_string(v.usedPct) + "%", rc });
            }
            if (v.projOk) v.headLeftSegs.push_back(Seg{ "   " + v.projTxt + (v.projLv.empty() ? std::string() : "  " + v.projLv), v.projUp ? C_GREEN : C_RED });   // (2.2.0) + its level
        }
        // (HL105, Rassul) + the 2nd-extreme call, the same source / rules as the 2nd Time cell: "   HOD after 9:30 76%" (stage 3),
        // "   HOD: close 14% \xB7 else 11:30-2:30" before it; "2nd" while the 1st is not IN; grey when not qualified / in test
        v.headTail.clear(); v.headTailShort.clear();
        if (!v.prior && !(v.complete && rh.callSrc == 0)) {
            // (1.1.2, Rassul 10-12 "once it says LOD in, instead of 2nd say HOD"): the read names the 1st extreme (IN, or IN ..%?) ->
            // the 2nd is the opposite side, in its colour (HOD green after the LOD, LOD red after the HOD); "2nd" only when it names none
            const bool namedT = rd.state == 1 && !rd.fomcPause && rd.pIn > 0.30;
            std::string who = (rh.callIn || namedT) ? (rh.side == 0 ? "HOD" : "LOD") : "2nd";
            if (rh.callSrc == 3 && rh.callT >= 0 && std::isfinite(rh.callShow)) {
                std::string p = std::to_string((int)std::llround(rh.callShow * 100)) + "%";
                v.headTail = "   " + who + " after " + clk(rh.callT) + " " + p; v.headTailShort = "   " + who + " >" + clk(rh.callT) + " " + p;
            } else if (rh.callSrc == 3 && rh.callLate) { v.headTail = v.headTailShort = "   " + who + " late-day"; }
            else if (rh.callSrc == 2 && std::isfinite(rh.pClose)) {
                std::string c = who + " at close " + std::to_string((int)std::llround(rh.pClose * 100)) + "%";
                v.headTailShort = "   " + c;
                v.headTail = v.headTailShort + (rh.elseFrom >= 0 && rh.elseTo > rh.elseFrom ? " \xB7 likely " + clk(rh.elseFrom) + "-" + clk(rh.elseTo) : "");
            }
            v.headTailCol = v.complete ? C_GREY : ((rh.callIn || namedT) ? (rh.side == 0 ? C_GREEN : C_RED) : (rh.callDim ? C_DIM : v.headerCol));
        }
        v.header = v.headLeftTxt + "   " + v.headMain + v.headTail;
        // (1.1.2, Rassul 10-12 "add colour") the A row follows the header: once the read names the 1st extreme, the 1st and 2nd
        // Time cells take that side's colour (LOD named = green, HOD named = red); the Range cell keeps its rule (grey until the 2nd prints)
        if (!v.prior && !v.complete && rd.state == 1 && !rd.fomcPause && rd.pIn > 0.30) {
            const unsigned sc = rd.side == 0 ? C_GREEN : C_RED;
            for (int k : { (int)K_1ST, (int)K_2ND }) if (!v.A[k].s.empty()) { v.A[k].col = sc; if (!v.A[k].s2.empty()) v.A[k].col2 = sc; }
        }
    }
    return v;
}

// ------------------------------------------------------------------------------------------------ run over a chart
// Feed the closed bars [from, to) of a chart and collect one read row per closed RTH bar of the LATEST session.

// (1.1.6) lockOut: the locked projection of the latest session (see ProjLock) - set at the later of the bar the current 1st
// extreme became the 1st extreme and the first bar 30 min after the open, from the tracker as it stood at that bar
inline Tracker runBars(const Cfg& c, const std::vector<Bar>& bars, size_t nClosed, std::vector<ReadRow>* rows, ProjLock* lockOut = nullptr)
{
    Tracker T(c);
    int nk = readNormKind(c.mkt), sk = readSplitKind(c.mkt);
    long long lkSid = -1; int lkSide = -1; long long lkPx = 0; bool lkPending = false;
    if (lockOut) *lockOut = ProjLock();
    const long long lastDay = (nClosed && nClosed <= bars.size()) ? floorDiv(bars[nClosed - 1].absEnd, 86400) : LLONG_MIN;   // locks only for the latest day
    for (size_t i = 0; i < nClosed && i < bars.size(); i++) {
        if (!T.push(bars[i])) continue;
        if (lockOut && T.cur.any && T.cur.lastSeq == T.seq && T.cur.sid >= lastDay) {
            const Rth& R = T.cur; int sd = R.lod.seq < R.hod.seq ? 1 : (R.hod.seq < R.lod.seq ? 2 : 0);
            long long px = sd == 1 ? R.lod.px : sd == 2 ? R.hod.px : 0;
            if (R.sid != lkSid) { lkSid = R.sid; lkSide = -1; *lockOut = ProjLock(); }
            if (sd != lkSide || px != lkPx) { lkSide = sd; lkPx = px; lkPending = sd != 0; *lockOut = ProjLock(); }
            long long mso = (R.lastEndAbs - T.rthA(R.sid)) / 60;
            if (lkPending && mso >= View::PROJ_WAIT_MIN) {
                View pv = buildView(T, ReadOut(), nullptr, nullptr, nullptr);
                if (pv.projOk && pv.projFrom == px && pv.projUp == (sd == 1)) {
                    lockOut->on = true; lockOut->up = pv.projUp; lockOut->from = pv.projFrom; lockOut->to = pv.projTo; lockOut->range = pv.projRange;
                    lockOut->stage = pv.projStage; lockOut->v2 = pv.projV2; lockOut->lockMod = (int)((R.lastEndAbs - R.sid * 86400) / 60);
                    lkPending = false;
                }
            }
        }
        if (!rows) continue;
        if (T.cur.any && T.cur.lastSeq == T.seq) {
            if (!rows->empty() && rows->front().session != dateTxt(T.cur.sid)) rows->clear();   // only the latest session
            ReadRow r; r.session = dateTxt(T.cur.sid); r.barEnd = (int)((T.cur.lastEndAbs - T.cur.sid * 86400) / 60);
            r.in = T.readIn(nk, sk); r.out = evalRead(r.in);
            r.shown = readLine(r.out, c.readOpen, c.readN);
            const Rth& R = T.cur; int s = R.lod.seq < R.hod.seq ? 1 : (R.hod.seq < R.lod.seq ? 2 : 0);
            r.firstSide = s == 1 ? 'L' : s == 2 ? 'H' : '?'; r.firstPx = s == 1 ? R.lod.px : s == 2 ? R.hod.px : 0;
            r.readPx = r.in.ok || r.in.t1Mod >= 0 ? (r.in.side == 0 ? R.lod.px : R.hod.px) : 0; r.sameBar = R.lod.seq == R.hod.seq;
            rows->push_back(r);
        }
    }
    return T;
}

// ------------------------------------------------------------------------------------------------ the draw list
enum { K_FILL = 0, K_RECT = 1, K_LINE = 2, K_TEXT = 3 };
enum { R_TABLE = 1, R_CANDLE = 2, R_LABEL = 3, R_NOTE = 4, R_PANEL = 5 };
struct Item { int k = 0, l = 0, t = 0, r = 0, b = 0; unsigned c1 = 0, c2 = 0; std::string s; int fs = 9; bool bold = false; int w = 1; int role = 0; int block = -1; bool body = false; int elem = 0; };   // (2.0.5) elem: E_TABLE / E_PROJ / E_MARK - which Settings checkbox + colour it belongs to   // body = a label drawn inside the candle body
struct Box { int l, t, r, b; };
// (2.0.5, Rassul 2026-10-13 "settings must be real": a checkbox and a colour for every drawn element) the three drawn elements
enum { E_TABLE = 0, E_PROJ = 1, E_MARK = 2 };
inline bool overlap(const Box& a, const Box& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
typedef std::function<int(const std::string&, int, bool)> Measure;

struct Layout {
    std::vector<Item> items;
    std::vector<Box> blocks;     // every text block (labels + table) - the overlap tests use these
    Box table = { 0, 0, 0, 0 }, area = { 0, 0, 0, 0 };
    bool tableOk = false, candleOk = false, projOk = false; std::string note;
    int colRight = 0;            // (HL105) the reserved candle column's right edge (px); written to the status file for SessionInfo
    // (2.0.5) other indicators' boxes the table and the projection label keep clear of, and what the placement did
    std::vector<Box> avoid; Box tableWant = { 0, 0, 0, 0 }; bool tableMoved = false, tableClear = true; bool trendCompact = false;
    Box gexRow = { 0, 0, 0, 0 }; bool gexOk = false;          // (2.1.0) the GEX line's row (the mouse-over target)
    Box gexHover = { 0, 0, 0, 0 }; bool gexHoverOk = false;   // (2.1.0) its detail box (drawn only while the mouse is on the row)
    Box qBox = { 0, 0, 0, 0 }; bool qOk = false;               // (2.3.0) the Q-Scores in the header (the mouse-over target)
    Box qHover = { 0, 0, 0, 0 }; bool qHoverOk = false;        // (2.3.0) their detail box
    Box preRow = { 0, 0, 0, 0 }; bool preOk = false;           // (2.6.0) the Pre-open row (the mouse-over target)
    Box preHover = { 0, 0, 0, 0 }; bool preHoverOk = false;    // (2.6.0) its detail box (2.7.0: the detail of whichever row is hovered)
    Box dayBox = { 0, 0, 0, 0 }; bool dayOk = false;           // (2.7.0) the day type text in the header (the mouse-over target)
    std::vector<Box> noteRows;                                  // (2.7.0) the note rows (News / Feed)
};

inline int lineH(int fs) { return (int)(fs * 1.7f + 0.5f); }   // as SessionInfo (a 9 pt font is 12 px at 96 dpi)
inline void addText(Layout& L, int x, int t, int b, const std::string& s, unsigned col, int fs, bool bold, int w, int role, int block)
{ Item it; it.k = K_TEXT; it.l = x; it.t = t; it.r = x + w; it.b = b; it.c1 = col; it.s = s; it.fs = fs; it.bold = bold; it.role = role; it.block = block; L.items.push_back(it); }
inline void addFill(Layout& L, int l, int t, int r, int b, unsigned col, int role) { Item it; it.k = K_FILL; it.l = l; it.t = t; it.r = r; it.b = b; it.c1 = col; it.role = role; L.items.push_back(it); }
inline void addRect(Layout& L, int l, int t, int r, int b, unsigned border, unsigned fill, int w, int role) { Item it; it.k = K_RECT; it.l = l; it.t = t; it.r = r; it.b = b; it.c1 = border; it.c2 = fill; it.w = w; it.role = role; L.items.push_back(it); }
inline void addLine(Layout& L, int x1, int y1, int x2, int y2, unsigned col, int w, int role) { Item it; it.k = K_LINE; it.l = x1; it.t = y1; it.r = x2; it.b = y2; it.c1 = col; it.w = w; it.role = role; L.items.push_back(it); }

// PIECE 2: the table, bottom-left of the pane
// PIECE 2 (mockup v9): the table at the bottom, centred-left: its left edge `left` is clear of the session candle's column
// (HL105, mockup 13 + Rassul "a setting where to put them") the 9 spots, as SessionInfo: Top / Middle / Bottom x Left / Centre / Right
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";
// (2.0.5, Rassul 2026-10-13 16:24 "the table at the TOP of the price pane"; 21:04 "even the location of the hod lod is incorrect")
static const int POS_DEFAULT = 7;   // (2.0.6, Rassul 07:13 "move it DOWN, default Bottom centre") Bottom centre (2.0.5: 1 = Top centre)
static const int POS_PREV_DEFAULT = 1;  // (2.0.6) a place file holding the 2.0.5 default becomes 7 ONCE per market (HodLod.migrate-206.txt)
static const int TITLE_ROW_PX = 22;     // (2.0.5) the IRT chart title row is kept clear (Liquidity 0.14.1 reserves 20 px the same way)
static const int AVOID_GAP = 4;         // px kept between the table and another indicator's box
inline Box placeIn(const Box& area, int W, int H, int pos)
{
    if (pos < 0 || pos > 8) pos = POS_DEFAULT;
    int c = pos % 3, r = pos / 3;
    int x = c == 0 ? area.l : c == 1 ? (area.l + area.r) / 2 - W / 2 : area.r - W;
    int y = r == 0 ? area.t : r == 1 ? (area.t + area.b) / 2 - H / 2 : area.b - H;
    x = (std::max)(area.l, (std::min)(x, area.r - W)); y = (std::max)(area.t, (std::min)(y, area.b - H));
    return Box{ x, y, x + W, y + H };
}
// (2.0.5) the Position spot, then - if another indicator's box is there (the Dealer Profile header / banner, the Tape Reader box) - the
// nearest spot inside the area that touches none of them: the table's edge moved to just past an obstacle's edge (vertically first,
// then sideways, then both). moved / clear say what happened (status file); nothing clear = the Position spot (clear = false).
inline Box offsetIn(const Box& area, const Box& b, int dx, int dy)    // (2.0.6) Offset X / Y, then kept fully inside the area
{
    if (dx < -600 || dx > 600) dx = 0;
    if (dy < -600 || dy > 600) dy = 0;
    const int W = b.r - b.l, H = b.b - b.t;
    int x = b.l + dx, y = b.t + dy;
    x = (std::max)(area.l, (std::min)(x, area.r - W)); y = (std::max)(area.t, (std::min)(y, area.b - H));
    return Box{ x, y, x + W, y + H };
}
inline Box placeAvoid(const Box& area, int W, int H, int pos, const std::vector<Box>& avoid, bool* moved = nullptr, bool* clear = nullptr, int dx = 0, int dy = 0)
{
    const Box b0 = offsetIn(area, placeIn(area, W, H, pos), dx, dy);       // (2.0.6) the Position spot + the offset; avoidance from there
    auto hits = [&](const Box& b) { for (size_t i = 0; i < avoid.size(); i++) if (overlap(b, avoid[i])) return true; return false; };
    if (moved) *moved = false;
    if (clear) *clear = true;
    if (!hits(b0)) return b0;
    std::vector<int> xs, ys; xs.push_back(b0.l); ys.push_back(b0.t);
    for (size_t i = 0; i < avoid.size(); i++) {
        ys.push_back(avoid[i].b + AVOID_GAP); ys.push_back(avoid[i].t - AVOID_GAP - H);
        xs.push_back(avoid[i].r + AVOID_GAP); xs.push_back(avoid[i].l - AVOID_GAP - W);
    }
    bool found = false; Box best = b0; long long bestD = 0;
    for (size_t a = 0; a < xs.size(); a++) for (size_t q = 0; q < ys.size(); q++) {
        const int x = xs[a], y = ys[q];
        if (x < area.l || x + W > area.r || y < area.t || y + H > area.b) continue;
        const Box b = { x, y, x + W, y + H };
        if (hits(b)) continue;
        const long long d = (long long)std::abs(x - b0.l) * 2 + std::abs(y - b0.t);   // sideways costs more: keep the column
        if (!found || d < bestD) { found = true; best = b; bestD = d; }
    }
    if (!found) { if (clear) *clear = false; return b0; }
    if (moved) *moved = true;
    return best;
}
// (2.0.2, Rassul 2026-10-13 05:31-05:44, mockups a-h) the table, top to bottom:
//   the session title ("ES RTH 8:30-3:00")                                          | "move ES RTH value to session box at the top"
//   1st Time | 2nd Time headings
//   Estimate  8:33 - 9:21 (65%)      HOD after 11:30 (63%)   (blue, bold, larger)    | "call the row Estimate", "expected values blue, bigger"
//   Actual    LOD IN 8:39 (71%)      HOD so far 10:27 ONH                            | "LOD IN 10:42 71%", probabilities in (), no "late" tag
//   Range used 70%   Proj HOD 7,913.50                                               | "followed by the range and projection values"
//   1st extreme  LOD? (73%)  7,860.25 . formed 8:39 . called 10:03   (only while the call is live)
// Reclaim column gone; every line larger with more spacing ("make fonts bigger for all lines ... some spacing so the whole thing looks nice").
inline std::string hlParenPct(const std::string& in)
{   // "65%" -> "(65%)", "73%?" -> "(73%?)"; a double space before it becomes one
    std::string o; size_t i = 0;
    while (i < in.size()) {
        if (isdigit((unsigned char)in[i]) && (i == 0 || !isdigit((unsigned char)in[i - 1])) && in[i - (i ? 1 : 0)] != '(') {
            size_t j = i; while (j < in.size() && isdigit((unsigned char)in[j])) j++;
            if (j < in.size() && in[j] == '%') {
                size_t k = j + 1; if (k < in.size() && in[k] == '?') k++;
                while (!o.empty() && o.size() >= 2 && o[o.size() - 1] == ' ' && o[o.size() - 2] == ' ') o.erase(o.size() - 1);
                o += "(" + in.substr(i, k - i) + ")"; i = k; continue;
            }
            o += in.substr(i, j - i); i = j; continue;
        }
        o += in[i++];
    }
    return o;
}
inline std::string hlShortRegime(std::string r)
{   // (05:57 "shorten Regime to Reg: ... shorten Negative to Neg") "Regime: Negative" -> "Reg: Neg", "Regime: Positive" -> "Reg: Pos"
    // (06:05 "spell out regime in the box") "Regime" stays spelled out; only the value is short
    size_t k;
    if ((k = r.find("Negative")) != std::string::npos) r.replace(k, 8, "Neg");
    if ((k = r.find("Positive")) != std::string::npos) r.replace(k, 8, "Pos");
    if (r.compare(0, 6, "Regime") == 0) r.replace(0, 6, "Reg");   // (2.2.3, Rassul 18:51 "make Regime Reg." - less space)
    return r;
}
inline void hlParenCell(Cell& c) { c.s = hlParenPct(c.s); c.s2 = hlParenPct(c.s2); c.s3 = hlParenPct(c.s3); }
// (2.1.1, Rassul 22:32 "remove the percentage (65%)") drop every " (NN%)" / " (NN%?)" / "(NN%, ...)" group from a cell's text
inline std::string hlNoPct(const std::string& in)
{
    std::string o; size_t i = 0;
    while (i < in.size()) {
        if (in[i] == '(') {
            size_t j = i + 1; while (j < in.size() && isdigit((unsigned char)in[j])) j++;
            if (j > i + 1 && j < in.size() && in[j] == '%') {
                size_t k = in.find(')', j);
                if (k != std::string::npos) { while (!o.empty() && o.back() == ' ') o.pop_back(); i = k + 1; continue; }
            }
        }
        o += in[i++];
    }
    return o;
}
inline void hlNoPctCell(Cell& c) { c.s = hlNoPct(c.s); c.s2 = hlNoPct(c.s2); c.s3 = hlNoPct(c.s3); }
// (2.0.5) the trend words = vtl::displayLabel (the strings agreed for Session Info, VwapTrendLogic.h VW110): "Strong Uptrend" / "Uptrend" /
// "Dntrend" / "Strong Dntrend"; no trend = "Range" (Rassul 2026-10-13 21:20). compact = only when the header column cannot fit the full word
inline std::string trendWord(int lvl, bool compact = false)
{
    switch (lvl) {
        case 2: return compact ? "S. Up" : "S. Uptrend";   // (2.2.3, Rassul 18:51 "strong uptrend S. Uptrend")
        case 1: return compact ? "Up" : "Uptrend";
        case -1: return compact ? "Dn" : "Dntrend";
        case -2: return compact ? "S. Dn" : "S. Dntrend";
        default: return "Range";
    }
}
inline unsigned trendCol(int lvl) { return lvl > 0 ? C_GREEN : (lvl < 0 ? C_RED : C_SLATE); }   // Range = the table's white text (no new colour)
// (2.5.0, Rassul 2026-10-21 10:56 "indicate Strong Uptrend, its fine to spell it out, since we have space now") the card's trend, in full
inline std::string trendFull(int lvl)
{
    switch (lvl) { case 2: return "Strong Uptrend"; case 1: return "Uptrend"; case -1: return "Downtrend"; case -2: return "Strong Downtrend"; default: return "Range"; }
}
// (2.5.0) the card's shades: the header band, the spot cell, the row dividers, the regime pill's fills (tints of his green / red - no new colour)
static const unsigned C_HEAD = 0x00111827, C_SPOT = 0x001F2937, C_DIV = 0x001A202C, C_PILL_G = 0x00052E16, C_PILL_R = 0x00450A0A, C_PILL_N = 0x001F2937;
// "+GAMMA" (green) / "-GAMMA" (red) / "GAMMA STALE" (grey) from the header's Regime segment; empty = no regime known
inline Seg gammaPill(const View& v)
{
    for (size_t q = 0; q < v.sessSegs.size(); q++) {
        const std::string& r = v.sessSegs[q].s; if (r.compare(0, 6, "Regime") != 0) continue;
        if (r.find("Positive") != std::string::npos) return Seg{ "+GAMMA", C_GREEN };
        if (r.find("Negative") != std::string::npos) return Seg{ "-GAMMA", C_RED };
        if (r.find("STALE") != std::string::npos) return Seg{ "GAMMA STALE", C_GREY };
    }
    return Seg{ "", C_GREY };
}
// the header's Regime cell as its coloured parts: "Regime: Neg" (gamma colour) + " / Uptrend" (trend colour). No gamma file and a
// trend: "Regime: -- / Uptrend". Trend not ready (< 20 RTH bars): the gamma part only.
inline std::vector<Seg> regimeParts(const View& v, bool compact = false)
{
    std::vector<Seg> out;
    Seg reg{ "", C_GREY };
    for (size_t q = 0; q < v.sessSegs.size(); q++) if (v.sessSegs[q].s.compare(0, 6, "Regime") == 0) reg = Seg{ hlShortRegime(v.sessSegs[q].s), v.sessSegs[q].col };
    if (reg.s.empty() && v.trendReady) reg = Seg{ "Reg: --", C_GREY };
    if (reg.s.empty()) return out;
    out.push_back(reg);
    if (v.trendReady) out.push_back(Seg{ " / " + trendWord(v.trendLvl, compact), trendCol(v.trendLvl) });
    return out;
}
inline std::string regimeText(const View& v, bool compact = false) { std::string s; std::vector<Seg> p = regimeParts(v, compact); for (size_t i = 0; i < p.size(); i++) s += p[i].s; return s; }

// (2.1.0) the GEX line: "7750 G8 ^  .  7775 G7  .  7800 G1" (+ "  STALE 25m"); the up arrow is drawn with lines (no glyph: IRT draws
// text in the chart's code page). Strike white, MenthorQ name blue, separators grey; stale = strikes / names dimmed.
inline int gexArrowW(int fs) { return (int)(fs * 0.8f + 0.5f) + 8; }
inline int gexMarkW(int fs) { return (int)(fs * 0.9f + 0.5f) + 6; }   // (2.3.0) the side triangle + its gap (drawn with lines: no glyph in the chart code page)
// a filled triangle (apex up for U, down for D), centred in [x, x + size] and the row
inline void gexTri(Layout& L, int x, int t, int b, int fs, bool up, unsigned col)
{
    const int h = (std::max)(5, (int)(fs * 0.8f + 0.5f)), yc = (t + b) / 2, y0 = yc - h / 2, cx = x + h / 2 + 1;
    for (int k = 0; k < h; k++) { const int half = (k * h / 2) / (h - 1 > 0 ? h - 1 : 1), y = up ? y0 + k : y0 + h - 1 - k; addLine(L, cx - half, y, cx + half + 1, y, col, 1, R_TABLE); }
}   // (2.2.1, Rassul 12:22 "the arrow is too close to the number") +4 px
inline int gexLineW(const View& v, const Measure& M, int fs, int sfs)
{
    if (v.gexItems.empty()) return 0;
    int w = 0; const int sep = M("  \xB7  ", fs, true), sepB = M("  |  ", fs, true);
    for (size_t i = 0; i < v.gexItems.size(); i++) {
        const View::GexItem& g = v.gexItems[i];
        w += (i ? ((g.mid || v.gexItems[i - 1].mid) ? sepB : sep) : 0) + (g.mark ? gexMarkW(fs) : 0) + (g.name.empty() ? 0 : M(g.name + " ", fs, true)) + M(g.strike, fs, true) + (g.grow ? gexArrowW(fs) : 0);   // (2.2.4) the name comes BEFORE the price
    }
    if (!v.gexStale.empty()) w += M("   " + v.gexStale, sfs, false);
    return w;
}
inline void layoutGexItems(Layout& L, const View& v, int x, int t, int b, const Measure& M, int fs, int sfs)
{
    const bool st = !v.gexStale.empty();
    unsigned cs = 0x00FFFFFFu;   // (2.4.0) the unused "cn" removed (-Werror)   // (2.1.1, Rassul 22:31) set per strike below: above price red, below price green   // (2.1.1, Rassul 22:26) strikes always white
    const std::string sepT = "  \xB7  ";
    for (size_t i = 0; i < v.gexItems.size(); i++) {
        const View::GexItem& g = v.gexItems[i];
        if (i) { const std::string st = (g.mid || v.gexItems[i - 1].mid) ? std::string("  |  ") : sepT; const int sw = M(st, fs, true);
                 addText(L, x, t, b, st, C_GREY, fs, true, sw, R_TABLE, -1); x += sw; }   // (2.2.4) '|' on both sides of the current price
        cs = g.mid ? C_SLATE : (g.above ? C_RED : C_GREEN);   // above price = red (resistance), below = green (support); the price itself white
        int w = 0;
        if (g.mark) { gexTri(L, x, t, b, fs, g.mark == 'U', cs); x += gexMarkW(fs); }   // (2.3.0) the side's triangle, in its colour
        if (!g.name.empty()) { w = M(g.name + " ", fs, true); addText(L, x, t, b, g.name + " ", C_BLUE, fs, true, w, R_TABLE, -1); x += w; }   // (2.2.4) MenthorQ name first, in blue
        w = M(g.strike, fs, true); addText(L, x, t, b, g.strike, cs, fs, true, w, R_TABLE, -1); x += w;
        if (g.grow) {   // an up arrow: shaft + head, centred in the row
            const int aw = gexArrowW(fs), ax = x + aw / 2 + 3, h = (int)(fs * 0.95f + 0.5f), yc = (t + b) / 2, y0 = yc + h / 2, y1 = yc - h / 2;
            addLine(L, ax, y0, ax, y1, cs, 2, R_TABLE); addLine(L, ax - 3, y1 + 3, ax, y1, cs, 2, R_TABLE); addLine(L, ax + 3, y1 + 3, ax, y1, cs, 2, R_TABLE);
            x += aw;
        }
    }
    if (st) { const std::string s = "   " + v.gexStale; addText(L, x, t, b, s, C_GREY, sfs, false, M(s, sfs, false), R_TABLE, -1); }
}

// (2.4.4, 2026-10-18, Rassul approved the mockup 17:13 CT "build": the table was ~790 px, "way too wide") the COMPACT table, every
// column sized to its text (not to the box):
//   [strip, its own one-line box ABOVE the table, same width, 4 px gap, moves with it]  R  PS 7700 . G1 7750 . 7800     S  7650 . 7640 . 7625
//   (review 17:50: R + levels RED = resistance above price, S + levels GREEN = support below, as his old row; a separator between levels)
//   ES M2 O0 V2                        Neg . S.Dn        (market + Q-Scores left; regime . trend right, red / green as before)
//                   1st        2nd                       (grey column headings)
//   Exp             8:33-9:21  late LOD                  (blue, the model's expected times)
//   Act             HOD 8:33   LOD 2:09 PFL
//   Est             prior RTH 10/16
//   Exp 3:00 PM . v2.4.4                                 (grey footer: the options expiry, the version)
// Words shortened (Expected -> Exp, Actual -> Act, Estimate -> Est, Reg: Negative -> Neg, S. Dntrend -> S.Dn); what left the text (the
// RTH hours, the expiry countdown, the full regime / trend words, the current price of the Gex row) is in the hovers: the header row
// (L.qBox) and the strip (L.gexRow). A strip wider than the table widens the table to it (never wrapped or cut).
static const int STRIP_GAP = 0;   // (2.5.0) the ladder sits ON the table: one card (was 4 px apart)
inline std::string hlMarketOf(const std::string& title) { const size_t k = title.find(' '); return k == std::string::npos ? title : title.substr(0, k); }
// "Expiry 3:00 PM (in 21h 51m)" -> "Exp 3:00 PM"; "(expired)" -> "Exp 3:00 PM done"
inline std::string hlShortExpiry(const std::string& s)
{
    if (s.compare(0, 6, "Expiry") != 0) return s;
    std::string o = "Exp" + s.substr(6);
    const size_t p = o.find(" (");
    if (p != std::string::npos) { const bool done = o.find("expired", p) != std::string::npos; o = o.substr(0, p); if (done) o += " done"; }
    return o;
}
// the header's compact regime: "Reg: Neg" + " / S. Dntrend" -> "Neg" + " \xB7 S.Dn" (same colours as regimeParts)
inline std::vector<Seg> compactRegime(const View& v)
{
    std::vector<Seg> p = regimeParts(v, true), o;
    for (size_t i = 0; i < p.size(); i++) {
        std::string s = p[i].s;
        if (s.compare(0, 5, "Reg: ") == 0) s = s.substr(5);
        if (s.compare(0, 3, " / ") == 0) s = "  \xB7  " + s.substr(3);
        size_t k; while ((k = s.find(". ")) != std::string::npos) s.erase(k + 1, 1);   // "S. Dn" -> "S.Dn"
        o.push_back(Seg{ s, p[i].col });
    }
    return o;
}
// the header's left side: "ES" + " M2 O0 V2" (the Q-Scores without "Q: ", in their colours); stale scores " Q STALE" (grey)
inline std::vector<Seg> compactHead(const View& v, const std::string& mkt, unsigned mktCol)
{
    std::vector<Seg> o; o.push_back(Seg{ mkt, mktCol });
    const bool stale = v.qSegs.size() >= 2 && v.qSegs[1].s == "STALE";
    if (stale) { o.push_back(Seg{ "   Q STALE", C_GREY }); return o; }
    for (size_t i = 0; i < v.qSegs.size(); i++) {
        std::string s = v.qSegs[i].s; if (s == "Q: ") continue;
        size_t k = s.find_first_not_of(' '); if (k == std::string::npos) continue;
        o.push_back(Seg{ (o.size() == 1 ? "   " : "  ") + s.substr(k), v.qSegs[i].col });   // (review) "ES   M2  O0  V2": clear gaps
    }
    return o;
}
// the strip's parts: "R" (green) + the levels above price, "S" (red) + the levels below; MenthorQ name before its strike, "\xB7" between
struct StripPart { std::string s; unsigned col; bool arrow; };
inline std::vector<StripPart> stripParts(const View& v)
{
    std::vector<StripPart> o;
    for (int side = 0; side < 2; side++) {
        bool first = true;
        for (size_t i = 0; i < v.gexItems.size(); i++) {
            const View::GexItem& g = v.gexItems[i];
            if (g.mid || (side == 0) != g.above) continue;
            const unsigned sc = side == 0 ? C_RED : C_GREEN;   // (review 17:50) resistance ABOVE price red, support BELOW green (as his old row)
            if (first) { if (!o.empty()) o.push_back(StripPart{ "     ", C_GREY, false });
                         o.push_back(StripPart{ side == 0 ? "R  " : "S  ", sc, false }); first = false; }
            else o.push_back(StripPart{ " \xB7 ", C_GREY, false });   // each level reads on its own
            o.push_back(StripPart{ (g.name.empty() ? std::string() : g.name + " ") + g.strike, sc, g.grow });
        }
    }
    if (!o.empty() && !v.gexStale.empty()) o.push_back(StripPart{ "   " + v.gexStale, C_GREY, false });
    return o;
}
inline int stripW(const std::vector<StripPart>& P, const Measure& M, int fs)
{ int w = 0; for (const StripPart& p : P) w += M(p.s, fs, false) + (p.arrow ? gexArrowW(fs) : 0); return w; }

// (2.5.0, Rassul 2026-10-21 10:37 "put the spot price so i can judge where we are in the context of the next sr levels, also the support
// levels should be before the resistance levels" - mockup v1 approved 10:56) the Gex S/R LADDER, low -> high:
//   [S]  7700  7750      7775  | 7,795.25 |  7800     7825  7850  [R]
//        PS    PS0/HVL0        | -20.25 . +4.75 |  GW0/CR0  G1   CR
// supports green (lowest first), the spot in its own shaded cell with the distance to the nearest support (green) and resistance (red),
// resistances red (nearest first); the MenthorQ name under each strike (grey). Stale data: the age after [R] (grey), as before.
struct Rung { std::string strike, name; bool grow = false; double lo = NAN, hi = NAN; };
struct Ladder { std::vector<Rung> sup, res; std::string spot, distS, distR, stale; bool ok = false; };
inline void numsOf(const std::string& t, double& lo, double& hi)
{
    lo = hi = NAN; std::string cur; std::vector<double> got;
    for (size_t i = 0; i <= t.size(); i++) {
        const char c = i < t.size() ? t[i] : ' ';
        if ((c >= '0' && c <= '9') || c == '.') cur += c;
        else if (c == ',' && !cur.empty()) continue;                   // 7,795.25
        else { if (!cur.empty()) { char* e = nullptr; const double d = std::strtod(cur.c_str(), &e); if (e && !*e) got.push_back(d); cur.clear(); } }
    }
    if (!got.empty()) { lo = got.front(); hi = got.back(); if (lo > hi) std::swap(lo, hi); }
}
inline std::string thousands(const std::string& px)
{
    const size_t dot = px.find('.'); std::string ip = px.substr(0, dot), fp = dot == std::string::npos ? "" : px.substr(dot);
    bool neg = !ip.empty() && ip[0] == '-'; if (neg) ip = ip.substr(1);
    std::string o; for (size_t i = 0; i < ip.size(); i++) { if (i && (ip.size() - i) % 3 == 0) o += ','; o += ip[i]; }
    return (neg ? "-" : "") + o + fp;
}
inline Ladder ladderOf(const View& v)
{
    Ladder Lr; double spot = NAN; int dec = 0;
    for (size_t i = 0; i < v.gexItems.size(); i++) {
        const View::GexItem& g = v.gexItems[i];
        if (g.mid) { Lr.spot = g.strike; double a, b2; numsOf(g.strike, a, b2); spot = a; const size_t d = g.strike.find('.'); dec = d == std::string::npos ? 0 : (int)(g.strike.size() - d - 1); continue; }
        Rung r; r.strike = g.strike; r.name = g.name; r.grow = g.grow; numsOf(g.strike, r.lo, r.hi);
        (g.above ? Lr.res : Lr.sup).push_back(r);
    }
    auto byLo = [](const Rung& a, const Rung& b) { return a.lo < b.lo; };
    std::stable_sort(Lr.sup.begin(), Lr.sup.end(), byLo); std::stable_sort(Lr.res.begin(), Lr.res.end(), byLo);
    if (std::isfinite(spot)) {
        char b[48];
        if (!Lr.sup.empty() && std::isfinite(Lr.sup.back().hi)) { snprintf(b, sizeof b, "-%.*f", dec, std::fabs(spot - Lr.sup.back().hi)); Lr.distS = b; }
        if (!Lr.res.empty() && std::isfinite(Lr.res.front().lo)) { snprintf(b, sizeof b, "+%.*f", dec, std::fabs(Lr.res.front().lo - spot)); Lr.distR = b; }
        Lr.spot = thousands(Lr.spot);
    }
    Lr.stale = v.gexStale;
    Lr.ok = !Lr.sup.empty() || !Lr.res.empty();
    return Lr;
}
// the ladder's geometry at font fs: rung widths, the spot cell, the tags; the total width is 2 x the wider side + the spot (spot centred)
struct LadderGeo { std::vector<int> ws, wr; int tagW = 0, spotW = 0, sideW = 0, staleW = 0, W = 0, H = 0, sfs = 0; };
inline int rungW(const Rung& r, const Measure& M, int fs) { return (std::max)(M(r.strike, fs, true) + (r.grow ? gexArrowW(fs) : 0), r.name.empty() ? 0 : M(r.name, fs, false)) + 12; }
inline LadderGeo ladderGeo(const Ladder& Lr, const Measure& M, int fs)
{
    LadderGeo G; G.sfs = fs + 3;
    int sl = 0, sr = 0;
    for (const Rung& r : Lr.sup) { G.ws.push_back(rungW(r, M, fs)); sl += G.ws.back(); }
    for (const Rung& r : Lr.res) { G.wr.push_back(rungW(r, M, fs)); sr += G.wr.back(); }
    G.tagW = M("S", fs, true) + 8;
    const std::string dist = Lr.distS + (Lr.distS.empty() || Lr.distR.empty() ? "" : " \xB7 ") + Lr.distR;
    G.spotW = (std::max)(M(Lr.spot, G.sfs, true), M(dist, fs, false)) + 24;
    G.staleW = Lr.stale.empty() ? 0 : M("  " + Lr.stale, fs, false);
    G.sideW = (std::max)(8 + G.tagW + 8 + sl, sr + 8 + G.tagW + G.staleW + 8);
    G.W = 2 * G.sideW + G.spotW;
    G.H = 3 + lineH(fs) + lineH(fs) - 4 + 3;
    return G;
}
inline void layoutLadder(Layout& L, const Ladder& Lr, const LadderGeo& G, const Box& Sb, const Measure& M, int fs)
{
    addRect(L, Sb.l, Sb.t, Sb.r, Sb.b, C_BORDER, C_PANEL, 1, R_TABLE);
    const int cx0 = (Sb.l + Sb.r) / 2 - G.spotW / 2, cx1 = cx0 + G.spotW;
    const int t1 = Sb.t + 3, b1 = t1 + lineH(fs), t2 = b1 - 4, b2 = Sb.b - 3;
    // the spot cell
    addFill(L, cx0, Sb.t + 1, cx1, Sb.b - 1, C_SPOT, R_TABLE);
    addLine(L, cx0, Sb.t + 1, cx0, Sb.b - 1, C_BORDER, 1, R_TABLE); addLine(L, cx1, Sb.t + 1, cx1, Sb.b - 1, C_BORDER, 1, R_TABLE);
    if (!Lr.spot.empty()) { const int w = M(Lr.spot, G.sfs, true); addText(L, (cx0 + cx1 - w) / 2, t1 - 1, b1 + 1, Lr.spot, 0x00FFFFFF, G.sfs, true, w, R_TABLE, -1); }
    {   const std::string sep = Lr.distS.empty() || Lr.distR.empty() ? "" : " \xB7 ";
        const int w = M(Lr.distS + sep + Lr.distR, fs, false); int x = (cx0 + cx1 - w) / 2;
        if (!Lr.distS.empty()) { const int ww = M(Lr.distS, fs, false); addText(L, x, t2, b2, Lr.distS, C_GREEN, fs, false, ww, R_TABLE, -1); x += ww; }
        if (!sep.empty()) { const int ww = M(sep, fs, false); addText(L, x, t2, b2, sep, C_GREY, fs, false, ww, R_TABLE, -1); x += ww; }
        if (!Lr.distR.empty()) { const int ww = M(Lr.distR, fs, false); addText(L, x, t2, b2, Lr.distR, C_RED, fs, false, ww, R_TABLE, -1); }
    }
    auto rung = [&](const Rung& r, int x, int w, unsigned col) {
        const int sw = M(r.strike, fs, true) + (r.grow ? gexArrowW(fs) : 0), sx = x + (w - 12 - sw) / 2 + 6;
        addText(L, sx, t1, b1, r.strike, col, fs, true, M(r.strike, fs, true), R_TABLE, -1);
        if (r.grow) { const int aw = gexArrowW(fs), ax = sx + M(r.strike, fs, true) + aw / 2 + 2, h = (int)(fs * 0.9f + 0.5f), yc = (t1 + b1) / 2, y0 = yc + h / 2, y1 = yc - h / 2;
                      addLine(L, ax, y0, ax, y1, col, 2, R_TABLE); addLine(L, ax - 3, y1 + 3, ax, y1, col, 2, R_TABLE); addLine(L, ax + 3, y1 + 3, ax, y1, col, 2, R_TABLE); }
        if (!r.name.empty()) { const int nw = M(r.name, fs, false); addText(L, x + (w - 12 - nw) / 2 + 6, t2, b2, r.name, C_GREY, fs, false, nw, R_TABLE, -1); }
    };
    auto tag = [&](int x, const char* t, unsigned col) { const int w = G.tagW, yc = (Sb.t + Sb.b) / 2, h = lineH(fs) - 2;
        addRect(L, x, yc - h / 2, x + w, yc + h / 2, col, C_PANEL, 1, R_TABLE); addText(L, x + 4, yc - h / 2, yc + h / 2, t, col, fs, true, w - 8, R_TABLE, -1); };
    if (!Lr.sup.empty()) {   // supports: lowest first, ending at the spot cell
        int sum = 0; for (int w : G.ws) sum += w;
        int x = cx0 - 4 - sum; tag(Sb.l + 8, "S", C_GREEN);
        for (size_t i = 0; i < Lr.sup.size(); i++) { rung(Lr.sup[i], x, G.ws[i], C_GREEN); x += G.ws[i]; }
    }
    if (!Lr.res.empty()) {   // resistances: nearest first, from the spot cell
        int x = cx1 + 4;
        for (size_t i = 0; i < Lr.res.size(); i++) { rung(Lr.res[i], x, G.wr[i], C_RED); x += G.wr[i]; }
        tag(Sb.r - 8 - G.staleW - G.tagW, "R", C_RED);
    }
    if (!Lr.stale.empty()) { const std::string st = "  " + Lr.stale; addText(L, Sb.r - 8 - G.staleW, Sb.t + 1, Sb.b - 1, st, C_GREY, fs, false, G.staleW, R_TABLE, -1); }
}

inline void layoutTable(Layout& L, const View& v, const Box& pane, int left, const Measure& M, int freeR = -1, int pos = POS_DEFAULT, int dx = 0, int dy = 0)
{
    if (freeR < 0) freeR = pane.r;
    const int pad = 6;
    const int titleRow = pane.b - pane.t >= 200 ? TITLE_ROW_PX : 0;   // (2.0.5) the IRT chart title row stays clear
    Box area = { left, pane.t + titleRow, (std::min)(freeR, (int)pane.r), pane.b - 1 };
    auto trimL = [](std::string t) { size_t k = t.find_first_not_of(' '); return k == std::string::npos ? std::string() : t.substr(k); };
    // ---- the content
    std::string title; unsigned titleCol = v.headerCol;
    std::vector<Seg> info;                                   // Est: Range used / Proj (or "prior RTH 10/16")
    if (!v.headLeftSegs.empty()) {
        title = trimL(v.headLeftSegs[0].s); titleCol = v.headLeftSegs[0].col;
        for (size_t q = 1; q < v.headLeftSegs.size(); q++) info.push_back(Seg{ trimL(v.headLeftSegs[q].s), v.headLeftSegs[q].col });
    } else title = trimL(v.headLeftTxt);
    // (2.4.5) outside RTH the Est row stays blank (was "prior RTH 10/16")
    Cell eRow[NALL], aRow[NALL];
    for (int c = 0; c < NALL; c++) { eRow[c] = v.E[c]; aRow[c] = v.A[c]; }
    const std::string tail = trimL(v.headTail);
    if (!tail.empty()) eRow[K_2ND] = mkCell(tail, C_BLUE, true);   // the 2nd-extreme call is the Expected 2nd
    for (int c = 1; c < NALL; c++) { Cell& e = eRow[c]; e.col = e.col2 = e.col3 = C_BLUE; e.bold = false; for (size_t q = 0; q < e.lv.size(); q++) e.lv[q].col = C_BLUE; }
    {   // Act 1st = "LOD IN 8:39" once the read names the side and the 1st extreme has printed
        const std::string hm = trimL(v.headMain);
        size_t inPos = hm.find(" IN "), pct = hm.find('%');
        const Cell& a1 = v.A[K_1ST];
        if (!v.prior && inPos != std::string::npos && pct != std::string::npos && pct > inPos && !a1.s.empty() && !a1.s2.empty()) {
            Cell c = a1; size_t e = pct + 1; if (e < hm.size() && hm[e] == '?') e++;
            c.s = hm.substr(0, inPos) + " IN"; c.s2 = a1.s + " " + hm.substr(inPos + 4, e - inPos - 4); c.col2 = a1.col; c.s3.clear();
            aRow[K_1ST] = c;
        }
    }
    for (int c = 1; c < NALL; c++) { if (aRow[c].s3 == "late") aRow[c].s3.clear(); hlParenCell(aRow[c]); hlParenCell(eRow[c]); hlNoPctCell(aRow[c]); hlNoPctCell(eRow[c]); aRow[c].bold = false; }
    // the Est values: one per column; more values than columns join the last column
    const int ncol = NSHOWN - 1;
    std::vector<Seg> est((size_t)ncol, Seg{ "", C_GREY });
    for (size_t q = 0; q < info.size(); q++) { const size_t ci = (std::min)(q, (size_t)(ncol - 1)); if (!est[ci].s.empty()) est[ci].s += "  "; else est[ci].col = info[q].col; est[ci].s += info[q].s; }
    const std::vector<Seg> head = compactHead(v, hlMarketOf(title), titleCol);
    std::string expTxt; unsigned expCol = C_GREY;
    for (size_t q = 0; q < v.sessSegs.size(); q++) if (v.sessSegs[q].s.compare(0, 6, "Regime") != 0) { expTxt = hlShortExpiry(v.sessSegs[q].s); expCol = v.sessSegs[q].col; break; }
    const std::string verTxt = std::string("v") + HL_VERSION;   // his rule: the version on the panel
    const Ladder lad = ladderOf(v);                          // (2.5.0) the Gex S/R ladder (low -> high, the spot in the middle)
    const Seg pill = gammaPill(v);                           // (2.5.0) +GAMMA / -GAMMA / GAMMA STALE
    const std::string trendTxt = !v.dayText.empty() ? v.dayText : (v.trendReady ? trendFull(v.trendLvl) : std::string());   // (2.7.0) the day type, when called
    const unsigned trendC = !v.dayText.empty() ? v.dayCol : trendCol(v.trendLvl);
    const std::string expLong = expTxt.compare(0, 4, "Exp ") == 0 ? "Options exp " + expTxt.substr(4) : expTxt;   // (2.5.0) footer words in full
    static const char* const HDR[3] = { "", "1ST", "2ND" };
    for (int attempt = 0; attempt < 2; attempt++) {
        const int fs = 10 - attempt;                         // 10 pt (his minimum); 9 pt only when 10 cannot fit
        const LadderGeo geo = ladderGeo(lad, M, fs);
        const int rowH = lineH(fs) + 1, footH = lineH(fs), stripH = lad.ok ? geo.H : 0;
        const int pillW = pill.s.empty() ? 0 : M(pill.s, fs, true) + 12;
        auto cellWf = [&](const Cell& C) {
            int w = M(C.s, fs, C.bold);
            if (!C.s2.empty()) w += M(" " + C.s2, fs, C.bold);
            for (size_t q = 0; q < C.lv.size(); q++) w += M((q ? " \xB7 " : " ") + C.lv[q].s, fs, C.bold);
            if (!C.s3.empty()) w += M("  " + C.s3, fs, C.bold);
            return w;
        };
        auto segsW = [&](const std::vector<Seg>& S, bool bold) { int w = 0; for (size_t q = 0; q < S.size(); q++) w += M(S[q].s, fs, bold); return w; };
        const int lw = (std::max)((std::max)((std::max)(M("Exp", fs, false), M("Act", fs, false)), M("Est", fs, false)), (std::max)(v.preOn ? M("Pre", fs, false) : 0, v.notes.empty() ? 0 : (std::max)(M("News", fs, false), M("Feed", fs, false)))) + 14;   // (2.4.5) the Est row is always there; (2.6.0) + Pre
        std::vector<int> cw((size_t)ncol, 0); int sumC = 0;
        for (int j = 1; j < NSHOWN; j++) { const int c = SHOWN[j], k = j - 1;
            cw[(size_t)k] = (std::max)((std::max)(M(HDR[j < 3 ? j : 2], fs, false), cellWf(eRow[c])), (std::max)(cellWf(aRow[c]), est[(size_t)k].s.empty() ? 0 : M(est[(size_t)k].s, fs, false))) + 14;
            sumC += cw[(size_t)k]; }
        const int headW = 10 + segsW(head, true) + 24 + pillW + (trendTxt.empty() ? 0 : 10 + M(trendTxt, fs, true)) + 10;
        const int footW = 10 + M(expLong, fs, false) + 24 + M(verTxt, fs, false) + 10;
        const int sW = lad.ok ? geo.W : 0;
        int W = (std::max)((std::max)(8 + lw + sumC, headW), (std::max)(footW, sW));   // the strip never wraps: the table widens to it
        if (v.preOn) W = (std::max)(W, 8 + lw + M(v.preText, fs, false) + 10);            // (2.6.0) the Pre row never wraps either
        for (size_t q = 0; q < v.notes.size(); q++) W = (std::max)(W, 8 + lw + M(v.notes[q].text, fs, false) + 10);   // (2.7.0) nor the note rows
        cw[(size_t)(ncol - 1)] += W - (8 + lw + sumC);                                   // the spare width goes to the last column
        const int hdrH = rowH + 4;                         // (2.5.0) the header band, a little taller
        const int H = 1 + hdrH + 1 + rowH + rowH + rowH + rowH + (v.preOn ? rowH : 0) + (int)v.notes.size() * rowH + 3 + 1 + footH + 2;   // (2.4.5) the Est row always (blank outside RTH) so the box keeps one height; (2.6.0) + the Pre row while it is shown
        const int Htot = H + (lad.ok ? stripH + STRIP_GAP : 0);
        if (W > area.r - area.l && attempt == 0) continue;
        if (area.l + W > pane.r - pad) continue;
        if (Htot + 40 > pane.b - pane.t) break;
        if (W > area.r - area.l) area.r = (std::min)((int)pane.r, area.l + W);
        L.tableWant = offsetIn(area, placeIn(area, W, Htot, pos), dx, dy);
        const Box B = placeAvoid(area, W, Htot, pos, L.avoid, &L.tableMoved, &L.tableClear, dx, dy);
        const Box T = { B.l, B.b - H, B.r, B.b };
        L.table = T; L.tableOk = true; L.trendCompact = true;
        if (lad.ok) {   // (2.5.0) the Gex S/R ladder: the card's top band, the table's width
            const Box Sb = { B.l, B.t, B.r, B.t + stripH };
            layoutLadder(L, lad, geo, Sb, M, fs);
            L.gexRow = Sb; L.gexOk = true; L.blocks.push_back(Sb);
        }
        addRect(L, T.l, T.t, T.r, T.b, C_BORDER, C_PANEL, 1, R_TABLE);
        int y = T.t + 1;
        {   // (2.5.0) the header band: "ES  M4 O0 V2" left; the gamma pill and the trend in full right ("+GAMMA  Strong Uptrend")
            addFill(L, T.l + 1, y, T.r - 1, y + hdrH, C_HEAD, R_TABLE);
            int x = T.l + 10; for (size_t q = 0; q < head.size(); q++) { const int w = M(head[q].s, fs, true); addText(L, x, y + 2, y + hdrH - 2, head[q].s, head[q].col, fs, true, w, R_TABLE, -1); x += w; }
            int xr = T.r - 10 - (trendTxt.empty() ? 0 : M(trendTxt, fs, true) + 10) - pillW;
            if (!pill.s.empty()) {
                const unsigned fill = pill.col == C_GREEN ? C_PILL_G : (pill.col == C_RED ? C_PILL_R : C_PILL_N);
                addRect(L, xr, y + 3, xr + pillW, y + hdrH - 3, pill.col, fill, 1, R_TABLE);
                addText(L, xr + 6, y + 3, y + hdrH - 3, pill.s, pill.col, fs, true, pillW - 12, R_TABLE, -1); xr += pillW + 10;
            }
            if (!trendTxt.empty()) addText(L, xr, y + 2, y + hdrH - 2, trendTxt, trendC, fs, true, M(trendTxt, fs, true), R_TABLE, -1);
            if (!v.dayText.empty() && !v.dayHover.empty()) { L.dayBox = Box{ xr - 2, y, xr + M(trendTxt, fs, true) + 2, y + hdrH }; L.dayOk = true; }   // (2.7.0)
            L.qBox = Box{ T.l, T.t, T.r, y + hdrH }; L.qOk = true;
        }
        y += hdrH; addLine(L, T.l, y, T.r, y, C_BORDER, 1, R_TABLE); y += 1;
        const int x0 = T.l + 8 + lw;
        auto colX = [&](int k) { int x = x0; for (int i = 0; i < k; i++) x += cw[(size_t)i]; return x; };
        // (2.4.5, Rassul 19:08 "right justify the 2nd column, we have space") the last column's text ends at the table's right padding
        auto colStart = [&](int k, int w) { return k == ncol - 1 ? T.r - 8 - w : colX(k); };
        for (int k = 0; k < ncol; k++) { const int hw = M(HDR[k + 1 < 3 ? k + 1 : 2], fs, false); addText(L, colStart(k, hw), y, y + rowH, HDR[k + 1 < 3 ? k + 1 : 2], C_GREY, fs, false, hw, R_TABLE, -1); }
        y += rowH;
        const Cell* rows[2] = { eRow, aRow }; const char* lab[2] = { "Exp", "Act" };
        for (int r = 0; r < 2; r++) {
            addText(L, T.l + 8, y, y + rowH, lab[r], C_SLATE, fs, false, M(lab[r], fs, false), R_TABLE, -1);
            for (int j = 1; j < NSHOWN; j++) {
                const Cell& C = rows[r][SHOWN[j]]; int xx = colStart(j - 1, cellWf(C));
                std::vector<std::string> part; std::vector<unsigned> pc;
                part.push_back(C.s); pc.push_back(C.col);
                part.push_back(C.s2.empty() ? "" : " " + C.s2); pc.push_back(C.col2 ? C.col2 : C.col);
                for (size_t q = 0; q < C.lv.size(); q++) { part.push_back((q ? " \xB7 " : " ") + C.lv[q].s); pc.push_back(C.lv[q].col); }
                part.push_back(C.s3.empty() ? "" : "  " + C.s3); pc.push_back(C.col3 ? C.col3 : C.col);
                for (size_t k = 0; k < part.size(); k++) { if (part[k].empty()) continue; const int pw = M(part[k], fs, C.bold); addText(L, xx, y, y + rowH, part[k], pc[k], fs, C.bold, pw, R_TABLE, -1); xx += pw; }
            }
            addLine(L, T.l + 1, y, T.r - 1, y, C_DIV, 1, R_TABLE);   // (2.5.0) a thin divider above each row
            y += rowH;
        }
        {   // (2.4.5) always drawn; blank values outside RTH
            addLine(L, T.l + 1, y, T.r - 1, y, C_DIV, 1, R_TABLE);
            addText(L, T.l + 8, y, y + rowH, "Est", C_SLATE, fs, false, M("Est", fs, false), R_TABLE, -1);
            for (int k = 0; k < ncol; k++) if (!est[(size_t)k].s.empty()) addText(L, colStart(k, M(est[(size_t)k].s, fs, false)), y, y + rowH, est[(size_t)k].s, est[(size_t)k].col == C_GREY ? C_SLATE : est[(size_t)k].col, fs, false, M(est[(size_t)k].s, fs, false), R_TABLE, -1);
            y += rowH;
        }
        if (v.preOn) {   // (2.6.0) the PRE-OPEN BOOK row: one line across the columns, green = 1st LOW lean, red = 1st HIGH lean, grey otherwise
            addLine(L, T.l + 1, y, T.r - 1, y, C_DIV, 1, R_TABLE);
            addText(L, T.l + 8, y, y + rowH, "Pre", C_SLATE, fs, false, M("Pre", fs, false), R_TABLE, -1);
            addText(L, x0, y, y + rowH, v.preText, v.preCol, fs, false, M(v.preText, fs, false), R_TABLE, -1);
            L.preRow = Box{ T.l, y, T.r, y + rowH }; L.preOk = true;
            y += rowH;
        }
        L.noteRows.clear();
        for (size_t q = 0; q < v.notes.size(); q++) {   // (2.7.0) the note rows: "News  CPI 7:30 AM . -5 / +15 min . marks ?" / "Feed  ? ..."
            addLine(L, T.l + 1, y, T.r - 1, y, C_DIV, 1, R_TABLE);
            addText(L, T.l + 8, y, y + rowH, v.notes[q].label, C_SLATE, fs, false, M(v.notes[q].label, fs, false), R_TABLE, -1);
            addText(L, x0, y, y + rowH, v.notes[q].text, v.notes[q].col, fs, false, M(v.notes[q].text, fs, false), R_TABLE, -1);
            L.noteRows.push_back(Box{ T.l, y, T.r, y + rowH });
            y += rowH;
        }
        y += 3; addLine(L, T.l, y, T.r, y, C_BORDER, 1, R_TABLE); y += 1;
        {   // (2.5.0) the footer: "Options exp 3:00 PM" left (amber in its last hour, as before), the version right
            if (!expLong.empty()) addText(L, T.l + 10, y, y + footH, expLong, expCol == C_AMBER ? C_AMBER : C_GREY, fs, false, M(expLong, fs, false), R_TABLE, -1);
            const int vw = M(verTxt, fs, false); addText(L, T.r - 10 - vw, y, y + footH, verTxt, C_GREY, fs, false, vw, R_TABLE, -1);
        }
        L.blocks.push_back(T);
        return;
    }
    // too narrow / too short: one line instead of forcing the table (checklist #35)
    std::string s = "HOD/LOD: pane too small for the table"; int fs = 10, w = M(s, fs, false), h = lineH(fs) + 4;
    Box T = { left, pane.b - pad - h, left + w + 8, pane.b - pad };
    if (T.r > pane.r || T.t < pane.t) { L.note = "pane too small"; return; }
    L.table = T; L.note = "table: pane too small";
    addRect(L, T.l, T.t, T.r, T.b, C_BORDER, C_PANEL, 1, R_NOTE);
    addText(L, T.l + 4, T.t, T.b, s, C_GREY, fs, false, w, R_NOTE, -1);
    L.blocks.push_back(T);
}
// (2.4.4) the header hover's first lines: what the compact header left out (the RTH hours, the expiry countdown, the full regime and
// trend words, what Exp / Act / Est mean), then the Q-Scores' meaning
inline std::vector<Line> headerHover(const View& v)
{
    std::vector<Line> o;
    const unsigned W = C_SLATE;
    std::string title = !v.headLeftSegs.empty() ? v.headLeftSegs[0].s : v.headLeftTxt; { size_t k = title.find_first_not_of(' '); title = k == std::string::npos ? std::string() : title.substr(k); }
    std::string exp; for (size_t q = 0; q < v.sessSegs.size(); q++) if (v.sessSegs[q].s.compare(0, 6, "Regime") != 0) exp = v.sessSegs[q].s;
    { Line L; L.push_back(Seg{ "HOD / LOD  " + title + (exp.empty() ? std::string() : "  \xB7  options " + exp), W }); o.push_back(L); }
    std::string rg; unsigned rc = C_GREY;
    for (size_t q = 0; q < v.sessSegs.size(); q++) if (v.sessSegs[q].s.compare(0, 6, "Regime") == 0) { rg = v.sessSegs[q].s; rc = v.sessSegs[q].col; }
    if (!rg.empty() || v.trendReady) {
        Line L; L.push_back(Seg{ "Regime: ", W });
        if (rg.size() > 8) L.push_back(Seg{ rg.substr(8) + (rg.find("Negative") != std::string::npos ? " gamma (dealers chase moves)" : rg.find("Positive") != std::string::npos ? " gamma (dealers dampen moves)" : ""), rc });
        else L.push_back(Seg{ "--", C_GREY });
        if (v.trendReady) { L.push_back(Seg{ "  \xB7  trend: ", W }); std::string t = trendWord(v.trendLvl); if (t == "S. Uptrend") t = "Strong Uptrend"; else if (t == "S. Dntrend") t = "Strong Downtrend"; else if (t == "Dntrend") t = "Downtrend"; L.push_back(Seg{ t, trendCol(v.trendLvl) }); }
        o.push_back(L);
    }
    { Line L; L.push_back(Seg{ "Exp", C_BLUE }); L.push_back(Seg{ " = Expected times of the 1st / 2nd extreme (the model)  \xB7  Act = Actual  \xB7  Est = Estimate (range used, projection).", W }); o.push_back(L); }
    for (size_t i = 0; i < v.qHover.size(); i++) o.push_back(v.qHover[i]);
    return o;
}

// (2.1.0) the GEX line's detail, drawn only while the mouse is on the row: above the table when there is room (the table sits at the
// bottom by default), else below it; inside the pane and left of the last bar; lines word-wrapped to the room (10 pt, his hover size)
inline std::vector<std::string> wrapText(const std::string& s, int maxW, const Measure& M, int fs, bool bold)
{
    std::vector<std::string> out; std::string cur; size_t a = 0;
    while (a <= s.size()) {
        size_t b = s.find(' ', a); std::string w = s.substr(a, b == std::string::npos ? std::string::npos : b - a);
        std::string t = cur.empty() ? w : cur + " " + w;
        if (!cur.empty() && M(t, fs, bold) > maxW) { out.push_back(cur); cur = w; } else cur = t;
        if (b == std::string::npos) break;
        a = b + 1;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}
inline void layoutGexHover(Layout& L, const View& v, const Box& pane, int freeR, const Measure& M)
{
    L.gexHoverOk = false;
    if (!L.gexOk || v.gexHover.empty()) return;
    const int fs = 10, lh = lineH(fs), pad = 8, ind = 12;
    if (freeR <= pane.l + 200) freeR = pane.r;
    const int maxW = (std::min)(760, freeR - pane.l - 2 * pad - 8);
    if (maxW < 220) return;
    struct WL { std::string s; unsigned col; bool bold; int x; };
    std::vector<WL> lines; int W = 0, gaps = 0; const int GAP = 5;
    std::vector<Seg> gh = v.gexHover;   // (2.4.4) the current price left the strip: it is the hover's 2nd line
    for (size_t i = 0; i < v.gexItems.size(); i++) if (v.gexItems[i].mid) { gh.insert(gh.begin() + (gh.empty() ? 0 : 1), Seg{ "Price now " + v.gexItems[i].strike + "  (R = resistance above it, S = support below it)", C_SLATE }); break; }
    for (size_t i = 0; i < gh.size(); i++) {
        const Seg& g = gh[i]; const bool bold = g.col == C_SLATE || i == 0;
        if (g.s.empty()) { lines.push_back(WL{ "", g.col, false, 0 }); gaps++; continue; }   // a spacer
        std::vector<std::string> w = wrapText(g.s, maxW - ind, M, fs, bold);
        for (size_t k = 0; k < w.size(); k++) { lines.push_back(WL{ w[k], g.col, bold, k ? ind : 0 }); W = (std::max)(W, M(w[k], fs, bold) + (k ? ind : 0)); }
    }
    const int top = pane.t + (pane.b - pane.t >= 200 ? TITLE_ROW_PX : 0);
    auto height = [&]() { return ((int)lines.size() - gaps) * lh + gaps * GAP + 2 * pad; };
    while (!lines.empty() && height() > pane.b - 1 - top) { if (lines.back().s.empty()) gaps--; lines.pop_back(); }   // a short pane: the guide lines go first
    while (!lines.empty() && lines.back().s.empty()) { gaps--; lines.pop_back(); }
    if (lines.size() < 2) return;
    W += 2 * pad; const int H = height();
    int x = (std::max)(pane.l + 2, (std::min)(L.table.l, freeR - W - 2));
    int y; const int sideY = (std::max)(top, (std::min)(L.table.b - H, pane.b - 1 - H));
    if (L.table.t - 4 - H >= top) y = L.table.t - 4 - H;                 // above the table (the default Bottom centre)
    else if (L.table.b + 4 + H <= pane.b - 1) y = L.table.b + 4;          // below it (a table at the top)
    else if (H <= pane.b - 1 - top && L.table.r + 4 + W <= freeR) { x = L.table.r + 4; y = sideY; }        // right of it
    else if (H <= pane.b - 1 - top && L.table.l - 4 - W >= pane.l + 2) { x = L.table.l - 4 - W; y = sideY; }   // left of it
    else y = (std::max)(top, pane.b - 1 - H);                             // no room anywhere: over the chart (and the table), inside
    Box B = { x, y, x + W, y + H };
    addRect(L, B.l, B.t, B.r, B.b, C_BORDER, C_PANEL, 1, R_TABLE);
    int yy = B.t + pad;
    for (const WL& l : lines) { if (l.s.empty()) { yy += GAP; continue; } addText(L, B.l + pad + l.x, yy, yy + lh, l.s, l.col, fs, l.bold, M(l.s, fs, l.bold), R_TABLE, -1); yy += lh; }
    L.gexHover = B; L.gexHoverOk = true; L.blocks.push_back(B);
}

// (2.3.0) the Q-Scores' detail (mouse over "Q: M1 O1 V4"): the mockup's lines, each a run of coloured parts ("M1" red + the meaning
// white), word-wrapped to the room; placed like the GEX detail (above the table, else below / beside it)
inline void layoutQHover(Layout& L, const View& v, const Box& pane, int freeR, const Measure& M)
{
    L.qHoverOk = false;
    if (!L.qOk) return;
    const std::vector<Line> HL = headerHover(v);   // (2.4.4) what the compact header left out, then the Q-Scores' meaning
    const int fs = 10, lh = lineH(fs), pad = 8, ind = 12;
    if (freeR <= pane.l + 200) freeR = pane.r;
    const int maxW = (std::min)(900, freeR - pane.l - 2 * pad - 8);
    if (maxW < 220) return;
    struct Run { std::string s; unsigned col; };
    struct WL { std::vector<Run> runs; bool bold; int x; };
    std::vector<WL> lines; int W = 0;
    for (size_t i = 0; i < HL.size(); i++) {
        const Line& ln = HL[i]; const bool bold = i == 0;
        std::string full; std::vector<size_t> segEnd;
        for (size_t k = 0; k < ln.size(); k++) { full += ln[k].s; segEnd.push_back(full.size()); }
        if (full.empty()) continue;
        std::vector<std::string> w = wrapText(full, maxW - ind, M, fs, bold);
        size_t off = 0;
        for (size_t k = 0; k < w.size(); k++) {
            size_t a = full.find(w[k], off); if (a == std::string::npos) a = off;
            const size_t e = a + w[k].size(); off = e;
            WL wl; wl.bold = bold; wl.x = k ? ind : 0;
            size_t c = a;
            for (size_t q = 0; q < segEnd.size() && c < e; q++) {
                if (segEnd[q] <= c) continue;
                const size_t ce = (std::min)(segEnd[q], e);
                wl.runs.push_back(Run{ full.substr(c, ce - c), ln[q].col }); c = ce;
            }
            int tw = 0; for (const Run& r : wl.runs) tw += M(r.s, fs, bold);
            W = (std::max)(W, tw + wl.x);
            lines.push_back(wl);
        }
    }
    const int top = pane.t + (pane.b - pane.t >= 200 ? TITLE_ROW_PX : 0);
    while (!lines.empty() && (int)lines.size() * lh + 2 * pad > pane.b - 1 - top) lines.pop_back();   // a short pane: the last lines go first
    if (lines.size() < 2) return;
    W += 2 * pad; const int H = (int)lines.size() * lh + 2 * pad;
    int x = (std::max)(pane.l + 2, (std::min)(L.table.l, freeR - W - 2));
    int y; const int sideY = (std::max)(top, (std::min)(L.table.b - H, pane.b - 1 - H));
    if (L.table.t - 4 - H >= top) y = L.table.t - 4 - H;
    else if (L.table.b + 4 + H <= pane.b - 1) y = L.table.b + 4;
    else if (H <= pane.b - 1 - top && L.table.r + 4 + W <= freeR) { x = L.table.r + 4; y = sideY; }
    else if (H <= pane.b - 1 - top && L.table.l - 4 - W >= pane.l + 2) { x = L.table.l - 4 - W; y = sideY; }
    else y = (std::max)(top, pane.b - 1 - H);
    Box B = { x, y, x + W, y + H };
    addRect(L, B.l, B.t, B.r, B.b, C_BORDER, C_PANEL, 1, R_TABLE);
    int yy = B.t + pad;
    for (const WL& l : lines) {
        int xx = B.l + pad + l.x;
        for (const Run& r : l.runs) { const int w = M(r.s, fs, l.bold); addText(L, xx, yy, yy + lh, r.s, r.col, fs, l.bold, w, R_TABLE, -1); xx += w; }
        yy += lh;
    }
    L.qHover = B; L.qHoverOk = true; L.blocks.push_back(B);
}

// (2.6.0) the Pre-open row's detail (mouse over "Pre ..."): the question, what it is, why it matters, how to use it, an example, then the
// numbers; word-wrapped, placed like the Q-Scores detail (above the table, else below / beside it)
inline void layoutRowHover(Layout& L, const std::vector<Line>& hov, const Box& pane, int freeR, const Measure& M)
{
    L.preHoverOk = false;
    if (!L.tableOk || hov.empty()) return;
    const int fs = 10, lh = lineH(fs), pad = 8, ind = 12;
    if (freeR <= pane.l + 200) freeR = pane.r;
    const int maxW = (std::min)(720, freeR - pane.l - 2 * pad - 8);
    if (maxW < 220) return;
    struct WL { std::string s; unsigned col; bool bold; int x; };
    std::vector<WL> lines; int W = 0;
    for (size_t i = 0; i < hov.size(); i++) {
        std::string full; unsigned col = C_SLATE; for (size_t k = 0; k < hov[i].size(); k++) { full += hov[i][k].s; if (k == 0) col = hov[i][k].col; }
        if (full.empty()) continue;
        const bool bold = i == 0;
        std::vector<std::string> w = wrapText(full, maxW - ind, M, fs, bold);
        for (size_t k = 0; k < w.size(); k++) { lines.push_back(WL{ w[k], col, bold, k ? ind : 0 }); W = (std::max)(W, M(w[k], fs, bold) + (k ? ind : 0)); }
    }
    const int top = pane.t + (pane.b - pane.t >= 200 ? TITLE_ROW_PX : 0);
    while (!lines.empty() && (int)lines.size() * lh + 2 * pad > pane.b - 1 - top) lines.pop_back();
    if (lines.size() < 2) return;
    W += 2 * pad; const int H = (int)lines.size() * lh + 2 * pad;
    int x = (std::max)(pane.l + 2, (std::min)(L.table.l, freeR - W - 2));
    int y; const int sideY = (std::max)(top, (std::min)(L.table.b - H, pane.b - 1 - H));
    const int aboveT = L.gexOk ? (std::min)(L.table.t, L.gexRow.t) : L.table.t;
    if (aboveT - 4 - H >= top) y = aboveT - 4 - H;
    else if (L.table.b + 4 + H <= pane.b - 1) y = L.table.b + 4;
    else if (H <= pane.b - 1 - top && L.table.r + 4 + W <= freeR) { x = L.table.r + 4; y = sideY; }
    else if (H <= pane.b - 1 - top && L.table.l - 4 - W >= pane.l + 2) { x = L.table.l - 4 - W; y = sideY; }
    else y = (std::max)(top, pane.b - 1 - H);
    Box B = { x, y, x + W, y + H };
    addRect(L, B.l, B.t, B.r, B.b, C_BORDER, C_PANEL, 1, R_TABLE);
    int yy = B.t + pad;
    for (const WL& l : lines) { addText(L, B.l + pad + l.x, yy, yy + lh, l.s, l.col, fs, l.bold, M(l.s, fs, l.bold), R_TABLE, -1); yy += lh; }
    L.preHover = B; L.preHoverOk = true; L.blocks.push_back(B);
}
inline void layoutPreHover(Layout& L, const View& v, const Box& pane, int freeR, const Measure& M) { if (L.preOk) layoutRowHover(L, v.preHover, pane, freeR, M); else L.preHoverOk = false; }

struct Block { std::vector<Line> lines; int fs; int w = 0, h = 0; Box at = { 0, 0, 0, 0 }; bool side = false; };

inline void measureBlock(Block& B, const Measure& M)
{
    B.w = 0; int lh = lineH(B.fs);
    for (size_t i = 0; i < B.lines.size(); i++) { int w = 0; for (size_t k = 0; k < B.lines[i].size(); k++) w += M(B.lines[i][k].s, B.fs, false); if (w > B.w) B.w = w; }
    B.h = (int)B.lines.size() * lh;
}
static const int CANDLE_FS = 9, CANDLE_BODY_W = 78;
// the candle's column (left edge of the pane): wide enough for the body and every label
static const int LVL_FS = 7;    // (HL104) the swept-level label right of the candle
// (HL105, Rassul "cut off extra space used by the candle") the column is exactly its content + 4 px padding each side:
// the candle body (with its in-body labels), the centred HOD / LOD groups and, right of the body, the swept-level labels
inline int candleBodyW(const View& v, const Measure& M)
{
    Block mid{ v.bodyMid, CANDLE_FS }, rec{ v.bodyRecl, CANDLE_FS - 1 }; measureBlock(mid, M); measureBlock(rec, M);
    return (std::max)(CANDLE_BODY_W, (std::max)(mid.w, rec.w) + 10);
}
inline int candleHalfW(const View& v, const Measure& M)      // half the widest centred item (+2 for the label fills)
{
    Block top{ v.top, CANDLE_FS }, bot{ v.bottom, CANDLE_FS }; measureBlock(top, M); measureBlock(bot, M);
    return ((std::max)((std::max)(top.w, bot.w) + 4, candleBodyW(v, M)) + 1) / 2;
}
inline int candleBaseW(const View& v, const Measure& M) { return 2 * candleHalfW(v, M) + 8; }
inline int candleColW(const View& v, const Measure& M)
{
    int half = candleHalfW(v, M), lw = 0;
    for (size_t i = 0; i < v.lvlLbls.size(); i++) lw = (std::max)(lw, M(v.lvlLbls[i].txt, LVL_FS, false));
    int right = 4 + half + (lw > 0 ? (std::max)(half, candleBodyW(v, M) / 2 + 5 + lw + 2) : half) + 4;   // from the column's left edge
    return right;
}

// PIECE 1: the session candle at real prices on the left edge. yOf maps a price (ticks) to a pane y. The table sits to the right
// of the candle's column, so the candle runs to the pane bottom; labels moved to the side stay above the table.
inline void layoutCandle(Layout& L, const View& v, const Box& pane, const std::function<int(long long)>& yOf, const Measure& M)
{
    if (!v.has) return;
    int aT = pane.t + 20, aB = pane.b - 8;
    L.area = Box{ pane.l, aT, pane.r, aB };
    if (aB - aT < 60) { L.note = L.note.empty() ? "candle: pane too short" : L.note + "; candle: pane too short"; return; }
    const int fs = CANDLE_FS, bodyW = CANDLE_BODY_W;
    auto measure = [&](Block& B) { measureBlock(B, M); };
    Block top{ v.top, fs }, bot{ v.bottom, fs }, mid{ v.bodyMid, fs }, rec{ v.bodyRecl, fs - 1 };
    measure(top); measure(bot); measure(mid); measure(rec);
    int colL = pane.l + 8, colW = candleColW(v, M);
    // (HL105, Rassul "give the candle a solid background") an opaque panel with the table's 1 px border behind the whole column:
    // the candle, its labels and the swept-level labels; the chart's bars never show through
    addRect(L, colL, pane.t, colL + colW, pane.b - 1, C_BORDER, C_PANEL, 1, R_PANEL);   // full pane height; the boxes share its right border
    int cx = colL + 4 + candleHalfW(v, M);           // (HL105) 4 px padding, then the candle; the level labels to the right of the body
    auto clampY = [&](int y) { return y < aT ? aT : (y > aB ? aB : y); };
    int yH0 = yOf(v.hod), yL0 = yOf(v.lod), yO = clampY(yOf(v.open)), yC = clampY(yOf(v.close));
    int yH = clampY(yH0), yL = clampY(yL0);
    int bT = (std::min)(yO, yC), bB = (std::max)(yO, yC); if (bB - bT < 2) { bB = bT + 2; if (bB > aB) { bB = aB; bT = aB - 2; } }
    int bw = (std::max)(bodyW, (std::max)(mid.w, rec.w) + 10);       // (HL102) widen the body for its labels (within the column)
    int bl = cx - bw / 2, br = cx + bw / 2;
    // candle
    addLine(L, cx, yH, cx, bT, v.bodyCol, 2, R_CANDLE);
    addLine(L, cx, bB, cx, yL, v.bodyCol, 2, R_CANDLE);
    addRect(L, bl, bT, br, bB, v.bodyCol, 0xFFFFFFFF, 2, R_CANDLE);   // c2 0xFFFFFFFF = hollow
    if (yH0 < aT) { addLine(L, cx - 4, aT + 4, cx, aT, v.bodyCol, 2, R_CANDLE); addLine(L, cx + 4, aT + 4, cx, aT, v.bodyCol, 2, R_CANDLE); }
    if (yL0 > aB) { addLine(L, cx - 4, aB - 4, cx, aB, v.bodyCol, 2, R_CANDLE); addLine(L, cx + 4, aB - 4, cx, aB, v.bodyCol, 2, R_CANDLE); }
    // (HL102) every label stays inside the candle's reserved column [pane.l, colR] - never over the price bars:
    //   HOD group above the top wick (clamped to the area top), LOD group below the bottom wick (clamped to the area bottom),
    //   body labels INSIDE the body, centred; a body too short takes a smaller font (9 -> 7 pt), then fewer lines, then none
    //   (every value is also in the table / status file)
    top.at = Box{ cx - top.w / 2, yH - 3 - top.h, cx + (top.w + 1) / 2, yH - 3 };
    if (top.at.t < aT) { top.at.t = aT; top.at.b = aT + top.h; }
    bot.at = Box{ cx - bot.w / 2, yL + 3, cx + (bot.w + 1) / 2, yL + 3 + bot.h };
    if (bot.at.b > aB) { bot.at.b = aB; bot.at.t = aB - bot.h; }
    if (!top.lines.empty() && !bot.lines.empty() && top.at.b + 2 > bot.at.t) {     // a tiny range: keep each group's first line
        top.lines.resize(1); bot.lines.resize(1); measure(top); measure(bot);
        top.at = Box{ cx - top.w / 2, top.at.t, cx + (top.w + 1) / 2, top.at.t + top.h };
        bot.at = Box{ cx - bot.w / 2, bot.at.b - bot.h, cx + (bot.w + 1) / 2, bot.at.b };
        if (top.at.b + 2 > bot.at.t) { bot.lines.clear(); L.note = L.note.empty() ? "candle: range too small for both labels" : L.note; }
    }
    bool openAtBottom = yO >= yC;
    {
        std::vector<Line> mid0 = mid.lines, rec0 = rec.lines;
        bool placed = false;
        for (int pass = 0; pass < 3 && !placed; pass++) {            // 0 both blocks, 1 the gap/range block only, 2 its first line only
            for (int f = fs; f >= 7 && !placed; f--) {
                mid.lines = mid0; rec.lines = pass == 0 ? rec0 : std::vector<Line>();
                if (pass == 2 && mid.lines.size() > 1) mid.lines.resize(1);
                mid.fs = f; rec.fs = (std::max)(7, f - 1); measure(mid); measure(rec);
                int topLim = (std::max)(bT, top.lines.empty() ? bT : top.at.b + 2), botLim = (std::min)(bB, bot.lines.empty() ? bB : bot.at.t - 2);
                int inner = botLim - topLim - 6;
                int need = mid.h + (rec.lines.empty() ? 0 : rec.h + 4);
                if (need > inner || (std::max)(mid.w, rec.w) + 6 > br - bl) continue;
                if (!rec.lines.empty()) {
                    if (openAtBottom) rec.at = Box{ cx - rec.w / 2, botLim - 3 - rec.h, cx + (rec.w + 1) / 2, botLim - 3 };
                    else              rec.at = Box{ cx - rec.w / 2, topLim + 3, cx + (rec.w + 1) / 2, topLim + 3 + rec.h };
                }
                int fT = openAtBottom || rec.lines.empty() ? topLim + 3 : rec.at.b + 2, fB = !openAtBottom || rec.lines.empty() ? botLim - 3 : rec.at.t - 2;
                int mt = (fT + fB) / 2 - mid.h / 2;
                mid.at = Box{ cx - mid.w / 2, mt, cx + (mid.w + 1) / 2, mt + mid.h };
                placed = true;
            }
        }
        if (!placed) { mid.lines.clear(); rec.lines.clear(); }
    }
    // (HL104, Rassul "just put them next to the candle on the right of the candle") the swept level's name and price, e.g.
    // "LonLO 7,838.25", in a small font right of the body, vertically at the level's price (clamped to the candle area), in the
    // level's SessionPrices colour (grey while that extreme is only "so far"); nudged up / down so it never overlaps a candle
    // label or the other level label; always inside the reserved column (never over the price bars)
    {
        std::vector<Box> taken;
        Block* lb[4] = { &top, &mid, &rec, &bot };
        for (int i = 0; i < 4; i++) if (!lb[i]->lines.empty()) taken.push_back(Box{ lb[i]->at.l - 2, lb[i]->at.t, lb[i]->at.r + 2, lb[i]->at.b });
        const int lh = lineH(LVL_FS), xL = br + 5, xMax = colL + colW;
        // placed from the highest price down, each searching downward first: a stack of close levels keeps price order
        // (the higher price on top), each as near its own price as the candle labels allow
        std::vector<size_t> ord(v.lvlLbls.size()); for (size_t i = 0; i < ord.size(); i++) ord[i] = i;
        std::stable_sort(ord.begin(), ord.end(), [&](size_t x, size_t y) { return v.lvlLbls[x].px > v.lvlLbls[y].px; });
        for (size_t oi = 0; oi < ord.size(); oi++) {
            size_t k = ord[oi];
            const View::LvlLbl& q = v.lvlLbls[k];
            int w = M(q.txt, LVL_FS, false);
            if (xL + w > xMax) continue;                                  // cannot happen (the column is sized for it); never spill
            int yc = yOf(q.px); if (yc < aT + lh / 2) yc = aT + lh / 2; if (yc > aB - lh / 2) yc = aB - lh / 2;
            Box best = { 0, 0, 0, 0 }; bool found = false;
            // first straight down, within 3 lines (keeps a stack of close levels in price order), then the nearest spot either way
            for (int pass = 0; pass < 2 && !found; pass++)
            for (int d = 0; d <= (pass == 0 ? 3 * lh : aB - aT) && !found; d += 2) {
                for (int sgn = 0; sgn < 2 && !found; sgn++) {
                    if ((d == 0 || pass == 0) && sgn == 1) continue;
                    int t = yc - lh / 2 + (sgn == 0 ? d : -d);
                    if (t < aT || t + lh > aB) continue;
                    Box b = { xL - 2, t - 1, xL + w + 2, t + lh + 1 }; bool ok = true;      // the drawn box + 1 px clearance
                    for (size_t j = 0; j < taken.size() && ok; j++) if (overlap(b, taken[j])) ok = false;
                    if (ok) { best = Box{ xL, t, xL + w, t + lh }; found = true; }
                }
            }
            if (!found) continue;
            unsigned col = q.fin ? q.col : C_GREY;
            addFill(L, best.l - 1, best.t, best.r + 1, best.b, C_PANEL, R_LABEL);
            addText(L, best.l, best.t, best.b, q.txt, col, LVL_FS, false, w, R_LABEL, 50 + (int)k);
            L.blocks.push_back(Box{ best.l - 1, best.t, best.r + 1, best.b });
            taken.push_back(Box{ best.l - 1, best.t, best.r + 1, best.b });
        }
    }
    Block* all[4] = { &top, &mid, &rec, &bot };
    int bi = 0;
    for (int i = 0; i < 4; i++) {
        Block& B = *all[i];
        if (B.lines.empty()) continue;
        Box bx = B.at;
        addFill(L, bx.l - 2, bx.t, bx.r + 2, bx.b, C_PANEL, R_LABEL);
        int lh = lineH(B.fs);
        for (size_t k = 0; k < B.lines.size(); k++) {
            int lw = 0; for (size_t s = 0; s < B.lines[k].size(); s++) lw += M(B.lines[k][s].s, B.fs, false);
            int x = B.side ? bx.l : cx - lw / 2;
            for (size_t s = 0; s < B.lines[k].size(); s++) {
                int w = M(B.lines[k][s].s, B.fs, false);
                addText(L, x, bx.t + (int)k * lh, bx.t + (int)(k + 1) * lh, B.lines[k][s].s, B.lines[k][s].col, B.fs, false, w, R_LABEL, bi);
                L.items.back().body = (&B == &mid || &B == &rec);
                x += w;
            }
        }
        L.blocks.push_back(Box{ bx.l - 2, bx.t, bx.r + 2, bx.b });
        bi++;
    }
    L.candleOk = true;
}

// (1.1.2, Rassul 10-12 "locked to the high of the HOD / the low of the LOD", "Follow the live extreme") the projection: a vertical
// line ON the live extreme's candle (Proj LOD from the running HOD's bar, Proj HOD from the running LOD's bar - it moves when a new
// extreme prints) from that price to the projected opposite extreme, a solid flag at the target and the target text beside it.
// Green up, red down. x = that bar's centre (-1 = the bar is off the chart: then just left of the table, as 1.1.0).
// The target is always shown: when it is past the pane's edge the line runs to the edge and the flag + text sit there with
// "(below)" / "(above)"; when the line would cross the table it stops at the table's edge and the flag sits there. Text never
// overlaps the table or another label (right of the flag, else left of the line, else nudged off the table).
static const int PROJ_FLAG_W = 16, PROJ_FLAG_H = 12, PROJ_FS = 11, PROJ_GAP = 8;
// (2.0.4, 2026-10-13 Rassul screenshot: "HOD after 10:45 . 94.34" sat on the left-edge key-level label "/PDH") the callout never goes
// into the LEFT LEVEL-LABEL BAND - the Liquidity Profile column with its level names ("832 1.9x LonLO/ONL") on the pane's left edge:
// 110 px (the column, as layoutAll's LQ_COL) or 15% of the pane, whichever is wider. HOD / LOD does not know the other plugins'
// label positions, so it keeps out of the band; the line and flag still stand on the extreme's bar.
static const int PROJ_LVL_BAND_MIN = 110;
static const int PROJ_LVL_ROW_HALF = 9;   // (2.0.4) half a Dealer Profile badge (10 pt + 6 px = 16 px) + 1 px
inline int projLevelBandR(const Box& pane) { return pane.l + (std::max)(PROJ_LVL_BAND_MIN, (int)((pane.r - pane.l) * 15 / 100)); }
inline bool boxHit(const Box& a, const Box& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
inline void layoutProj(Layout& L, const View& v, const Box& pane, const std::function<int(long long)>& yOf, const Measure& M, int x)
{
    if (!v.projOk || x < pane.l + 2 || x > pane.r - 4) return;
    int y0 = yOf(v.projFrom), y1 = yOf(v.projTo);
    if (y0 == y1) return;
    const int top = pane.t + 1, bot = pane.b - 1;
    auto clampY = [&](int y) { return (std::max)(top, (std::min)(y, bot)); };
    const unsigned col = v.projHit ? C_DIM : v.projUp ? C_GREEN : C_RED;   // (2.2.2) reached: grey
    int a = clampY(y0), e = clampY(y1);
    bool off = y1 < top || y1 > bot;
    if (L.tableOk && x >= L.table.l - 2 && x <= L.table.r + 2) {            // never across the table: stop at its edge
        if (!v.projUp && a < L.table.t && e >= L.table.t) { e = L.table.t - 2; off = true; }
        if (v.projUp && a > L.table.b && e <= L.table.b) { e = L.table.b + 2; off = true; }
        // (2.0.2, the taller table) the extreme itself behind the table: the line starts at the table's edge, never through it
        if (v.projUp && a >= L.table.t - 2 && a <= L.table.b + 2 && e < L.table.t) a = L.table.t - 3;
        if (!v.projUp && a >= L.table.t - 2 && a <= L.table.b + 2 && e > L.table.b) a = L.table.b + 3;
    }
    if (v.projUp ? e >= a : e <= a) return;                                 // nothing left to draw (the extreme is past the edge)
    addLine(L, x, a, x, e, col, 2, R_LABEL);
    // (1.1.2, Rassul 10:25 "a dashed line across" at the projected level) when the target is on the pane: 6 px dashes, 4 px gaps,
    // across the pane in the projection's colour; never through the table
    if (y1 >= top && y1 <= bot && !v.projHit) {                            // (2.2.2) no dashed line across once reached
        for (int dx = pane.l + 2; dx < pane.r - 2; dx += 10) {
            int ex = (std::min)(dx + 6, (int)pane.r - 2);
            if (L.tableOk && y1 >= L.table.t - 1 && y1 <= L.table.b + 1 && ex >= L.table.l && dx <= L.table.r) continue;
            addLine(L, dx, y1, ex, y1, col, 1, R_LABEL);
        }
    }
    // the flag: a solid triangle pointing right; its top edge at the target (up) or its bottom edge at the target (down)
    int ft = v.projUp ? e : e - PROJ_FLAG_H;
    for (int r = 0; r <= PROJ_FLAG_H; r++) {
        int half = PROJ_FLAG_H / 2, d = r <= half ? r : PROJ_FLAG_H - r;
        int len = (int)std::lround((double)PROJ_FLAG_W * d / (double)(half > 0 ? half : 1));
        if (len > 0) addLine(L, x, ft + r, x + len, ft + r, col, 1, R_LABEL);
    }
    // (1.1.3, 13:58 NG check: the "(above)" text at the pane edge landed on level badges / header boxes) a target off the pane
    // gets the line and the flag only - its price is in the table header ("Proj HOD 3.205"), in the same colour
    if (off) { L.projOk = true; return; }
    // (2.2.2, Rassul 13:45 NQ screenshot: price went 90 pts past "HOD after 1:45 30,013.50" and the callout floated over the candles)
    // a reached target keeps only its grey line + flag; the table says "Proj HOD 30,013.50 hit 12:21"
    if (v.projHit) { L.projOk = true; return; }
    // (2.0.4) a text box [yy, yy + hh] that would sit on a level's row (its line +/- PROJ_LVL_ROW_HALF px: the Dealer Profile badge)
    auto onLevelRow = [&](int yy, int hh) {
        for (size_t i = 0; i < v.levelTicks.size(); i++) {
            const int ly = yOf(v.levelTicks[i]);
            if (ly + PROJ_LVL_ROW_HALF > yy && ly - PROJ_LVL_ROW_HALF < yy + hh) return true;
        }
        return false;
    };
    // (2.0.2, Rassul 05:59 "the callout at the top should be center justified HOD with the time and price below it"): two lines centred
    // on the projection line - "HOD" over "after 11:30 . 7,857.00" (the time = the expected 2nd-extreme call, when there is one)
    {
        const std::string l1 = v.projUp ? "HOD" : "LOD";
        const size_t sp = v.projTxt.rfind(' ');
        const std::string price = sp == std::string::npos ? v.projTxt : v.projTxt.substr(sp + 1);
        const std::string l2 = (v.callT >= 0 ? "after " + clk(v.callT) + " \xB7 " : std::string()) + price;
        const std::string l3 = v.projLv;                                       // (2.2.0) the level at the target, under the price
        const int nl = l3.empty() ? 2 : 3;
        const int lh = lineH(PROJ_FS), w1 = M(l1, PROJ_FS, true), w2 = M(l2, PROJ_FS, true), w3 = l3.empty() ? 0 : M(l3, PROJ_FS, true), bw = (std::max)((std::max)(w1, w2), w3);
        // (06:05 "the arrowhead should be connected with the label on top of it"): the callout sits directly on the flag - above it for a
        // HOD target (beyond the line's end), below it for a LOD target; when the pane edge leaves no room there, it sits against the
        // flag's right side, centred on it. Never across the line.
        const int bandR = projLevelBandR(pane);                                // (2.0.4) keep out of the level-label band
        auto clearB = [&](int xx, int yy, int ww, int hh) {
            if (xx < bandR || xx + ww > pane.r - 2 || yy < top || yy + hh > bot) return false;
            if (onLevelRow(yy, hh)) return false;                              // (2.0.4) never on a level badge's row
            Box b = { xx, yy, xx + ww, yy + hh };
            for (size_t k = 0; k < L.blocks.size(); k++) if (boxHit(b, L.blocks[k])) return false;
            for (size_t k = 0; k < L.avoid.size(); k++) if (boxHit(b, L.avoid[k])) return false;   // (2.0.5) other indicators' boxes
            return !(L.tableOk && boxHit(b, L.table));
        };
        auto put = [&](int bx, int by) {
            addRect(L, bx - 2, by, bx + bw + 2, by + nl * lh, C_PANEL, C_PANEL, 1, R_LABEL);   // the dashed target line passes behind it
            addText(L, bx + bw / 2 - w1 / 2, by, by + lh, l1, col, PROJ_FS, true, w1, R_LABEL, -1);
            addText(L, bx + bw / 2 - w2 / 2, by + lh, by + 2 * lh, l2, col, PROJ_FS, true, w2, R_LABEL, -1);
            if (!l3.empty()) addText(L, bx + bw / 2 - w3 / 2, by + 2 * lh, by + 3 * lh, l3, col, PROJ_FS, true, w3, R_LABEL, -1);
            L.blocks.push_back(Box{ bx, by, bx + bw, by + nl * lh });
            L.projOk = true;
        };
        const int fTop = ft, fBot = ft + PROJ_FLAG_H;
        int bx = x - bw / 2 + PROJ_FLAG_W / 2;                                // centred on the flag
        bx = (std::max)((int)pane.l + 2, (std::min)(bx, (int)pane.r - 2 - bw));
        int by = v.projUp ? fTop - nl * lh : fBot;                              // touching the flag
        if (clearB(bx, by, bw, nl * lh)) { put(bx, by); return; }
        int sx = x + PROJ_FLAG_W + 2, sy = (fTop + fBot) / 2 - lh;             // against the flag's right side
        sy = (std::max)(top, (std::min)(sy, bot - nl * lh));                    // at a pane edge: as close to the flag as fits
        if (clearB(sx, sy, bw, nl * lh)) { put(sx, sy); return; }
        int lx = x - 2 - bw;                                                   // against the line's left side
        if (clearB(lx, sy, bw, nl * lh)) { put(lx, sy); return; }
        // (2.0.4) a level badge on the callout's row: the centred callout stepped further from the flag (beyond the line's end), half
        // a line at a time, up to 3 lines
        for (int k = 1; k <= 6; k++) {
            const int yy = v.projUp ? by - k * lh / 2 : by + k * lh / 2;
            if (clearB(bx, yy, bw, nl * lh)) { put(bx, yy); return; }
        }
        // (2.0.4) the flag is in (or next to) the level-label band: the callout just right of the band, on the flag's row, then the
        // rows beside it - never over a level label
        for (int step = 0; step <= 4; step++) {
            const int yy = sy + ((step + 1) / 2) * (nl * lh + 2) * (step % 2 ? -1 : 1);
            if (clearB(bandR + 2, yy, bw, nl * lh)) { put(bandR + 2, yy); return; }
        }
    }
    std::string txt = v.projTxt;   // no room for the centred callout: the one-line text beside the flag (1.1.x placement)
    int w = M(txt, PROJ_FS, true), lh = lineH(PROJ_FS);
    int ty = ft + PROJ_FLAG_H / 2 - lh / 2;
    ty = (std::max)(top, (std::min)(ty, bot - lh));
    const int bandR1 = projLevelBandR(pane);                                // (2.0.4) the one-line text keeps out of the band too
    auto clear = [&](int tx, int yy) {
        if (tx < bandR1 || tx + w > pane.r - 2) return false;
        if (onLevelRow(yy, lh)) return false;                               // (2.0.4) never on a level badge's row
        Box bx = { tx, yy, tx + w, yy + lh };
        for (size_t k = 0; k < L.blocks.size(); k++) if (boxHit(bx, L.blocks[k])) return false;
        for (size_t k = 0; k < L.avoid.size(); k++) if (boxHit(bx, L.avoid[k])) return false;   // (2.0.5)
        return !(L.tableOk && boxHit(bx, L.table));
    };
    int tx = x + PROJ_FLAG_W + 6;
    if (!clear(tx, ty)) {
        int left = x - 6 - w;
        if (clear(left, ty)) tx = left;
        else {                                                              // both sides blocked: step away from the table / labels
            bool done = false;
            for (int step = 1; step <= 6 && !done; step++) {
                int yy = v.projUp ? ty + step * lh : ty - step * lh;
                if (yy < top || yy + lh > bot) break;
                if (clear(tx, yy)) { ty = yy; done = true; } else if (clear(left, yy)) { tx = left; ty = yy; done = true; }
            }
            for (int step = 0; step <= 6 && !done; step++) {                 // (2.0.4) just right of the level-label band
                int yy = ty + ((step + 1) / 2) * lh * (step % 2 ? -1 : 1);
                if (yy < top || yy + lh > bot) continue;
                if (clear(bandR1 + 2, yy)) { tx = bandR1 + 2; ty = yy; done = true; }
            }
            // (2.0.4) no clear spot anywhere: the line + flag only - the target's price is in the table header ("Proj HOD 94.34"), as
            // for a target off the pane; never a text over a level label, a level badge or the table
            if (!done) { L.projOk = true; return; }
        }
    }
    addText(L, tx, ty, ty + lh, txt, col, PROJ_FS, true, w, R_LABEL, -1);
    L.blocks.push_back(Box{ tx, ty, tx + w, ty + lh });
    L.projOk = true;
}

// (2.0.5) the Settings window, all applied here so the tests see exactly what is drawn: Show table + its background colour, Show
// projection + its up (to the HOD) / down (to the LOD) colours, Show 1st-extreme mark + its LOD / HOD colours. A colour of 0 (black /
// unset, IRT returns it for a never-saved colour) keeps the default; hidden table = no table at all (the projection is then not cut).
struct Opts {
    bool table = true, proj = true, mark = true;
    int dx = 0, dy = 0;                                                    // (2.0.6) Offset X / Y of the table (px, -600..600)
    unsigned tableBg = C_PANEL, projUp = C_GREEN, projDn = C_RED, markLod = C_GREEN, markHod = C_RED;
};
inline unsigned optCol(unsigned v, unsigned def) { v &= 0xFFFFFFu; return v ? v : def; }
inline void applyOpts(Layout& L, const Opts& O)
{
    std::vector<Item> keep; keep.reserve(L.items.size());
    for (size_t i = 0; i < L.items.size(); i++) {
        Item it = L.items[i];
        if (it.elem == E_PROJ) {
            if (!O.proj) continue;
            if (it.c1 == C_GREEN) it.c1 = optCol(O.projUp, C_GREEN); else if (it.c1 == C_RED) it.c1 = optCol(O.projDn, C_RED);
        } else if (it.elem == E_MARK) {
            if (!O.mark) continue;
            if (it.c1 == C_GREEN) it.c1 = optCol(O.markLod, C_GREEN); else if (it.c1 == C_RED) it.c1 = optCol(O.markHod, C_RED);
        } else if (it.role == R_TABLE || it.role == R_NOTE) {
            if (!O.table) continue;
            if (it.k == K_RECT && it.c2 == C_PANEL) it.c2 = optCol(O.tableBg, C_PANEL);
            if (it.k == K_FILL && it.c1 == C_PANEL) it.c1 = optCol(O.tableBg, C_PANEL);
        }
        keep.push_back(it);
    }
    L.items.swap(keep);
    if (!O.table) L.tableOk = false;
}
inline void tagElem(Layout& L, size_t from, int elem) { for (size_t i = from; i < L.items.size(); i++) L.items[i].elem = elem; }
// (2.0.5) avoid = other indicators' boxes on this pane (the Dealer Profile header / banner, the Tape Reader box): the table and the
// projection label keep clear of them
inline Layout layoutAll(const View& v, const Box& pane, const std::function<int(long long)>& yOf, const Measure& M, int priceEdge = -1, int pos = POS_DEFAULT, int projX = -1,
                        const std::vector<Box>* avoid = nullptr, const Opts* opt = nullptr)
{
    Layout L;
    if (avoid) L.avoid = *avoid;
    const bool showTable = !opt || opt->table;
    if (pane.r - pane.l < 120 || pane.b - pane.t < 60) { L.note = "pane too small"; return L; }
    // (1.1.2, Rassul 11:20 "make the hod lod not overlap with the liquidity profile") the Liquidity Profile's column on the left
    // edge (bars + "5.1x", 110 px - the Dealer Profile badges keep off it the same way) is never under the table
    const int LQ_COL = 64;   // (2.2.3, Rassul 18:57 "the hod lod should be bottom center justified") the Liquidity column is 60 px now: centre on [its edge, the last bar] (was 110)
    int colR = v.has && HODLOD_SHOW_CANDLE ? pane.l + 8 + candleColW(v, M) : (pane.r - pane.l >= 600 ? pane.l + LQ_COL : pane.l + 2);
    L.colRight = v.has && HODLOD_SHOW_CANDLE ? colR : 0;   // the column panel's right border x (0 = no column: the candle is off)
    // (HL105) bottom centre of the free area right of the candle column (6 px clear of its panel) and 15% before the latest bars
    // the free area ends at the price edge (the last bar's right edge, where the profiles begin) when the chart gives it
    int freeR = priceEdge > 0 ? priceEdge : pane.r - (int)((pane.r - pane.l) * 0.15);
    if (freeR <= colR + 100) freeR = pane.r - (int)((pane.r - pane.l) * 0.15);
    if (showTable) layoutTable(L, v, pane, colR, M, freeR, pos, opt ? opt->dx : 0, opt ? opt->dy : 0);   // (2.0.5) Show table off: no table, the projection runs uncut   // (HL105) placed by Position inside [the pane's left edge (or the column), the last bar]
    if (HODLOD_SHOW_CANDLE) layoutCandle(L, v, pane, yOf, M);
    // (1.1.2) the line stands on the live extreme's candle; only when that bar is off the chart, just left of the table (1.1.0)
    if ((projX < pane.l + 2 || projX > pane.r - 4) && L.tableOk && L.table.l - PROJ_GAP > pane.l + 2) projX = L.table.l - PROJ_GAP;
    const size_t p0 = L.items.size();
    layoutProj(L, v, pane, yOf, M, projX);
    tagElem(L, p0, E_PROJ);
    return L;                                // (2.0.5) the host adds the 1st-extreme mark, then applies the colours / checkboxes once (applyOpts)
}

// ------------------------------------------------------------------------------------------------ status + logs
inline std::string fmtP(double p) { if (!std::isfinite(p)) return ""; char b[32]; snprintf(b, sizeof b, "%.6f", p); return b; }
// (1.1.8) one HodLod.firstcall-<MKT>.csv line (FIRSTCALL_HEADER); times 24-hour CT
inline std::string firstCallLine(const FirstCall& f, double tick, int dec)
{
    char th[16]; snprintf(th, sizeof th, "%.2f", IN_THR);
    return f.session + "," + (f.side == 0 ? "LOD" : "HOD") + "," + trimNum((double)f.px * tick, dec) + "," + hhmm(f.formed) + "," + hhmm(f.declared) + "," +
        std::to_string(f.declared - f.formed) + "," + fmtP(f.pIn) + "," + fmtP(f.pShow) + "," + th + "," + (f.sameBar ? "1" : "0") + "," + f.readVer + "," + HL_VERSION + "\n";
}

inline std::string statusText(const View& v, const Layout& L, const Tracker& T, const ReadOut& rd, int per, const std::string& state)
{
    const Mkt& K = MKTS[T.C.mkt]; double t = T.C.tick; int dec = K.dec;
    std::string o;
    auto kv = [&](const std::string& k, const std::string& val) { o += k + "," + val + "\n"; };
    kv("VERSION", HL_VERSION); kv("MARKET", K.code); kv("PER", std::to_string(per)); kv("STATE", state);
    kv("MODEL_SOURCE", std::string(hlx::HLX_SOURCE_SHA) + " window " + std::to_string(hlx::HLX_WINDOW));
#if HL_HAVE_READ
    kv("READ_TABLES", hlr::version(T.C.mkt));
#else
    kv("READ_TABLES", "not built in (read: building)");
#endif
    kv("SESSION", v.session); kv("PRIOR", v.prior ? "1" : "0"); kv("COMPLETE", v.complete ? "1" : "0");
    if (v.has) {
        kv("OPEN", pxTxt((double)v.open * t, dec)); kv("CLOSE", pxTxt((double)v.close * t, dec));
        kv("HOD", pxTxt((double)v.hod * t, dec) + "," + hhmm(v.hodMod) + "," + (v.hodLvl >= 0 ? LV_HIGH[v.hodLvl] : ""));
        kv("LOD", pxTxt((double)v.lod * t, dec) + "," + hhmm(v.lodMod) + "," + (v.lodLvl >= 0 ? LV_LOW[v.lodLvl] : ""));
        kv("FIRST", v.side == 1 ? "LOD" : v.side == 2 ? "HOD" : "not clear");
        kv("SWEPT_LOD", v.sweptLod); kv("SWEPT_HOD", v.sweptHod);     // (HL104) every swept level (the first 3 per extreme are drawn)
        if (!v.mqWhy.empty()) kv("MQ_LEVELS", v.mqWhy);
        kv("TOOK_MIN", std::to_string(v.took)); kv("SIZE", trimNum((double)v.sizeT * t, dec) + "," + usd(v.sizeUsd));
        kv("RECLAIM", v.reclaimed ? hhmm(v.reclMod) : "none"); kv("REC_TOOK_MIN", std::to_string(v.recTook));
        kv("SECOND", v.secondFinal ? "final" : (v.secondPrinted ? "running" : "pending")); kv("GAP_MIN", std::to_string(v.gap));
        // (HL101) running vs final, both kept for the scorer: the 1st is final once called IN (or at the close), the 2nd at the close
        kv("FIRST_STATE", v.side == 0 ? "not clear" : (v.complete ? "final (close)" : (v.firstCalledIn ? "called IN" : "running")));
        if (v.side != 0) {
            bool lf = v.side == 1; int m2 = lf ? v.hodMod : v.lodMod; long long p2 = lf ? v.hod : v.lod;
            kv("SECOND_STATE", v.secondFinal ? "final" : (v.secondPrinted ? "running" : "pending"));
            kv("SECOND_RUNNING", std::string(lf ? "high," : "low,") + pxTxt((double)p2 * t, dec) + "," + hhmm(m2) + "," + std::to_string(v.secondPrinted ? v.gap : -1));
            kv("SECOND_FINAL", v.secondFinal ? std::string(lf ? "HOD," : "LOD,") + pxTxt((double)p2 * t, dec) + "," + hhmm(m2) + "," + std::to_string(v.gap) : "");
        }
        kv("RANGE", trimNum((double)v.rangeT * t, dec) + "," + usd(v.rangeUsd) + "," + std::to_string(v.usedPct) + "%");
        kv("MODEL", std::string(v.modelUp ? "up" : "down") + (v.modelByShare ? " (more common, " + std::to_string(v.sharePct) + "%)" : ""));
        kv("E_LOD_E_HOD", std::isfinite(v.eLod) ? pxTxt(v.eLod, dec) + "," + pxTxt(v.eHod, dec) : "");
    }
    kv("READ", v.header);
    if (rd.state == 1) kv("READ_VALUES", fmtP(rd.pIn) + "," + std::to_string(rd.afterT) + "," + fmtP(rd.afterP) + "," + std::to_string(rd.byT) + "," + fmtP(rd.byP) + "," + rd.ver +
                          ",pInShow=" + fmtP(rd.pInShow) + ",afterQ=" + (rd.afterQ ? "1" : "0") + ",afterShow=" + fmtP(rd.afterShow) + (rd.fomcPause ? ",fomcPause" : ""));
    kv("TIMING", rd.tOk ? "pLater=" + fmtP(rd.pLater) + ",pClose=" + fmtP(rd.pClose) + ",else=" + std::to_string(rd.elseFrom) + "-" + std::to_string(rd.elseTo) + (rd.tWhy.empty() ? "" : "," + rd.tWhy)
                        : "n/a," + rd.tWhy);
    {   int y, m, d; civilFromDays(T.cur.any ? T.cur.sid : T.curSid, y, m, d); bool known = false;
        unsigned bits = hlev::bitsOn(y * 10000 + m * 100 + d, known);
        static const char* const EVN[6] = { "CPI ", "NFP ", "FOMC ", "ISM_MFG ", "EIA_CRUDE ", "EIA_GAS " };
        std::string e;
        for (int k = 0; k < 6; k++) { if (bits & (1u << k)) e += EVN[k]; }
        kv("EVENTS", known ? (e.empty() ? "none" : e) + ",news=" + (hlev::newsDay(T.C.mkt, bits) ? "1" : "0") + ",fomc=" + (hlev::fomcDay(bits) ? "1" : "0")
                           : "unknown (events file covers " + std::to_string(hlev::kFirstDay) + "-" + std::to_string(hlev::kLastDay) + "),news=0,fomc=0"); }
    for (int i = 0; i < NCOL; i++) kv(std::string("E_") + (i ? COLS[i] : "label"), cellTxt(v.E[i]));
    { char b[96]; snprintf(b, sizeof b, "%d min after the 1st extreme, %.4f of sessions", v.recWin, v.recP); kv("E_RECLAIM_WINDOW", b); }
    kv("E_SOURCES", "1st=" + v.src1st + "; reclaim=" + v.srcRecl + "; 2nd=" + v.src2nd);
    kv("E_GAP", cellTxt(v.E[K_GAP]) + (v.gapBucket == -3 ? ",hlr::stage3" : (v.gapBucket >= 0 ? ",static bucket " + std::to_string(v.gapBucket) : ",static all sessions")) + (std::isfinite(v.gapP25) ? ",middle half " + durT(std::llround(v.gapP25)) + "-" + durT(std::llround(v.gapP75)) : ""));
    kv("E_WINDOW_1ST", clk(v.win1a) + "-" + clk(v.win1b) + " (" + v.src1st + ")");
    { char b[96]; double mt = NAN;
#if HL_HAVE_READ
      switch (T.C.mkt) { case 0: mt = hlr::es::kAfterThr; break; case 1: mt = hlr::nq::kAfterThr; break; case 2: mt = hlr::cl::kAfterThr; break;
                         case 3: mt = hlr::gc::kAfterThr; break; case 4: mt = hlr::hg::kAfterThr; break; case 5: mt = hlr::ng::kAfterThr; break; default: mt = hlr::eu::kAfterThr; }
#endif
      snprintf(b, sizeof b, "%.2f floor (Rassul), model threshold %.2f", AFTER_THR, mt); kv("AFTER_THR", b); }
    kv("HLE", std::string(hle::kVersion) + (v.haveHle ? ",valid" : ",not used: " + v.hleWhy));
    kv("PROJ", v.projOk ? (std::string(v.projUp ? "HOD," : "LOD,") + pxTxt((double)v.projTo * t, dec) + ",from," + pxTxt((double)v.projFrom * t, dec) + (L.projOk ? ",drawn" : ",not drawn")) : std::string("none"));   // (1.1.0)
    if (v.haveHle) {
        const hle::Expect& e = v.ex; char b[400];
        snprintf(b, sizeof b, "sources=%d stage=%d range=%.6g lo=%.6g hi=%.6g pRange1Atr=%.4f roomLeft=%.6g pMore025=%.4f pUp=%.4f closeSide=%d closeLoc=%.4f t1=%.2f t2=%.2f wick=%.6g pGapFill=%.4f",
                 e.sources, e.stage, e.range, e.rangeLo, e.rangeHi, e.pRange1Atr, e.roomLeft, e.pMore025, e.pUp, e.closeSide, e.closeLoc, e.t1, e.t2, e.wick, e.pGapFill);
        kv("HLE_VALUES", b);
        if (!v.hleWhy.empty()) kv("HLE_FLAGS", v.hleWhy);
    }
    kv("HLE_INPUT_FLAGS", "news from HodLodEvents.h (see EVENTS); prevClose not roll-adjusted; onRange = 00:00 -> the RTH open as fitted (studies.py), not the header's 17:00");
    kv("E_CALL_2ND", (v.callT >= 0 ? clk(v.callT) + ",raw=" + fmtP(v.callP) + ",shown=" + fmtP(v.callShow) + (v.callQ ? ",qualified" : ",best below threshold")
                                   : std::string(v.callLate ? "late-day" : "none")) +
                     (v.callSrc == 3 ? ",hlr::stage3" : v.callSrc == 1 ? ",hlr::read" : "") + (v.callIn ? ",first IN" : ",side-neutral") + (v.complete ? ",frozen at the close" : ",live"));
    { char b[64]; snprintf(b, sizeof b, "%.2f", IN_THR); kv("IN_THR", b); }
    for (int i = 0; i < NCOL; i++) kv(std::string("A_") + (i ? COLS[i] : "label"), cellTxt(v.A[i]));
    for (size_t i = 0; i < L.items.size(); i++) {
        const Item& it = L.items[i];
        if (it.k != K_TEXT || it.role != R_LABEL) continue;
        char b[64]; snprintf(b, sizeof b, "%d,%d,%d,%d,", it.l, it.t, it.r, it.b);
        kv("LABEL", b + it.s);
    }
    kv("NOTE", L.note);
    kv("RESERVED_RIGHT_X", std::to_string(L.colRight > 0 ? L.colRight : 0));
    if (L.tableOk) kv("TABLE_BOX", std::to_string(L.table.l) + "," + std::to_string(L.table.t) + "," + std::to_string(L.table.r) + "," + std::to_string(L.table.b));   // SessionInfo stacks clear of it   // (HL105) the candle column's right edge (pane x): SessionInfo centres its box right of it
    // (2.0.5) the trend in the header and where the table went (the Position spot, moved clear of another indicator's box, or not clear)
    kv("TREND", std::string(v.trendReady ? trendWord(v.trendLvl) : "not ready") + "," + std::to_string(v.trendLvl) + ",bars " + std::to_string(v.trendBars) + "," + v.trendVia);
    kv("REGIME_TEXT", regimeText(v, L.trendCompact));
    if (L.tableOk) kv("TABLE_PLACE", std::string(L.tableMoved ? "moved clear" : (L.tableClear ? "position spot" : "position spot, NOT clear")) + ",want " +
                      std::to_string(L.tableWant.l) + " " + std::to_string(L.tableWant.t) + " " + std::to_string(L.tableWant.r) + " " + std::to_string(L.tableWant.b));
    for (size_t i = 0; i < L.avoid.size(); i++) kv("AVOID", std::to_string(L.avoid[i].l) + "," + std::to_string(L.avoid[i].t) + "," + std::to_string(L.avoid[i].r) + "," + std::to_string(L.avoid[i].b));
    return o;
}

// the read log the scorer reads: hlr::kLogHeader (18 columns, hlr-api-1); afterT = -1 when not qualified (the scorer scores shown calls)
//   market,session,bar_end,mso,side,pIn,cellN,afterT,afterP,byT,byP,pLater,pClose,elseFrom,elseTo,pInShow,afterQ,version
inline std::string scorerLine(const std::string& mkt, const ReadRow& r)
{
    char b[400];
    snprintf(b, sizeof b, "%s,%s,%s,%d,%d,%s,%d,%d,%s,%d,%s,%s,%s,%d,%d,%s,%d,%s", mkt.c_str(), r.session.c_str(), hhmm(r.barEnd).c_str(), r.in.mso,
             r.out.side, fmtP(r.out.pIn).c_str(), r.out.cellN, r.out.afterQ ? r.out.afterT : -1, fmtP(r.out.afterP).c_str(), r.out.byT, fmtP(r.out.byP).c_str(),
             r.out.tOk ? fmtP(r.out.pLater).c_str() : "", r.out.tOk ? fmtP(r.out.pClose).c_str() : "", r.out.tOk ? r.out.elseFrom : -1, r.out.tOk ? r.out.elseTo : -1,
             fmtP(r.out.pInShow).c_str(), r.out.afterQ ? 1 : 0, r.out.ver.c_str());
    return b;
}
static const char* const HL_LOG_HEADER = "kind,plugin,market,per,session,bar_end,mode,first_side,first_px,mso,read_state,pIn,afterT,afterP,byT,byP,read_version,shown";
inline std::string csvSafe(std::string s) { for (size_t i = 0; i < s.size(); i++) if (s[i] == ',' || s[i] == '\n' || s[i] == '\r') s[i] = ';'; return s; }
inline std::string readLogLine(const std::string& mkt, int per, const ReadRow& r, bool live, double tick, int dec)
{
    static const char* ST[4] = { "building", "ok", "not_yet", "na" };
    std::string s = "READ," + std::string(HL_VERSION) + "," + mkt + "," + std::to_string(per) + "," + r.session + "," + hhmm(r.barEnd) + "," +
        (live ? "live" : "replay") + "," + std::string(1, r.firstSide) + "," + (r.firstSide == '?' ? "" : trimNum((double)r.firstPx * tick, dec)) + "," +
        std::to_string(r.in.mso) + "," + ST[r.out.state < 0 || r.out.state > 3 ? 0 : r.out.state] + "," + fmtP(r.out.pIn) + "," +
        std::to_string(r.out.afterT) + "," + fmtP(r.out.afterP) + "," + std::to_string(r.out.byT) + "," + fmtP(r.out.byP) + "," +
        csvSafe(r.out.ver) + "," + csvSafe(r.shown);
    return s;
}
inline std::string outcomeLine(const View& v, int per, double tick)
{
    int dec = MKTS[mktIndex(v.market) < 0 ? 0 : mktIndex(v.market)].dec;
    std::string s = "OUTCOME," + std::string(HL_VERSION) + "," + v.market + "," + std::to_string(per) + "," + v.session + "," + hhmm(v.lastMod) + ",final," +
        (v.side == 1 ? "L" : v.side == 2 ? "H" : "?") + "," +
        "hod=" + trimNum((double)v.hod * tick, dec) + "@" + hhmm(v.hodMod) + ";lod=" + trimNum((double)v.lod * tick, dec) + "@" + hhmm(v.lodMod) +
        ";open=" + trimNum((double)v.open * tick, dec) + ";reclaim=" + (v.reclaimed ? hhmm(v.reclMod) : "none") + ";took=" + std::to_string(v.took) +
        ";rec_took=" + std::to_string(v.recTook) + ";gap=" + std::to_string(v.gap) + ";range=" + trimNum((double)v.rangeT * tick, dec) +
        ";used=" + std::to_string(v.usedPct) + ";hod_lvl=" + (v.hodLvl >= 0 ? LV_HIGH[v.hodLvl] : "") + ";lod_lvl=" + (v.lodLvl >= 0 ? LV_LOW[v.lodLvl] : "") +
        ",,,,,,,,," + csvSafe(v.header);
    return s;
}

// ---- file I/O with injectable operations (tests make rename / write fail)
struct FileOps {
    std::function<bool(const std::string&, const std::string&)> rename = [](const std::string& a, const std::string& b) { return std::rename(a.c_str(), b.c_str()) == 0; };
    std::function<bool(const std::string&)> remove = [](const std::string& a) { return std::remove(a.c_str()) == 0; };
    std::function<bool(const std::string&, const std::string&, bool)> write = [](const std::string& p, const std::string& text, bool append) {
        std::ofstream f(p.c_str(), append ? (std::ios::out | std::ios::app | std::ios::binary) : (std::ios::out | std::ios::trunc | std::ios::binary));
        if (!f.is_open()) return false;
        f << text; f.flush(); return f.good();
    };
    std::function<bool(const std::string&)> exists = [](const std::string& p) { std::ifstream f(p.c_str()); return f.good(); };
};
// .tmp -> move the old file to .bak -> rename into place -> restore .bak on failure (checklist #5)
inline bool writeAtomic(const FileOps& F, const std::string& path, const std::string& text)
{
    std::string tmp = path + ".tmp", bak = path + ".bak";
    if (!F.write(tmp, text, false)) { F.remove(tmp); return false; }
    bool had = F.exists(path);
    if (had) { F.remove(bak); if (!F.rename(path, bak)) { F.remove(tmp); return false; } }
    if (!F.rename(tmp, path)) { if (had) F.rename(bak, path); F.remove(tmp); return false; }
    return true;
}
// the last line of a (possibly large) file - only the last 4 KiB are read (checklist #33)
inline std::string lastLine(const std::string& path)
{
    std::ifstream f(path.c_str(), std::ios::binary); if (!f.is_open()) return "";
    f.seekg(0, std::ios::end); std::streamoff n = f.tellg(); if (n <= 0) return "";
    std::streamoff k = n > 4096 ? 4096 : n; f.seekg(n - k); std::string s((size_t)k, '\0'); f.read(&s[0], k); s.resize((size_t)f.gcount());
    while (!s.empty() && (s[s.size() - 1] == '\n' || s[s.size() - 1] == '\r')) s.erase(s.size() - 1);
    size_t p = s.rfind('\n'); return p == std::string::npos ? (k == n ? s : "") : s.substr(p + 1);
}
// the logger: appends the rows newer than what the file already holds; marks progress only after a flushed, good write (#6)
struct LogState { std::string session; int lastBarEnd = -1; bool outcomeDone = false; bool primed = false; };
inline void primeFromFile(LogState& S, const std::string& path, const std::string& session, int sessionField, int barField, const char* kindOutcome)
{
    S.primed = true; S.session = session; S.lastBarEnd = -1; S.outcomeDone = false;
    std::string ln = lastLine(path); if (ln.empty()) return;
    std::vector<std::string> c; size_t a = 0; for (;;) { size_t b = ln.find(',', a); c.push_back(ln.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
    if ((int)c.size() <= (std::max)(sessionField, barField) || c[(size_t)sessionField] != session) return;
    const std::string& t = c[(size_t)barField];
    if (t.size() == 5 && t[2] == ':' && isdigit((unsigned char)t[0]) && isdigit((unsigned char)t[1]) && isdigit((unsigned char)t[3]) && isdigit((unsigned char)t[4])) {
        int h = (t[0] - '0') * 10 + t[1] - '0', m = (t[3] - '0') * 10 + t[4] - '0';
        if (h < 24 && m < 60) S.lastBarEnd = h * 60 + m;
    }
    if (kindOutcome && c[0] == kindOutcome) S.outcomeDone = true;
}

// ---- (2.4.0) the MARKET STATE facts (MARKET-STATE-SPEC.md 1.0: SRC "HODLOD" and "TREND"), decided for the last CLOSED bar; SDK-free so
// they are tested (tests_hodlod/t_hl240.cpp). '?' = not known. Prices with the market's decimals, times "HH:MM" CT (24 h, internal).
struct MstIn {
    double tick = 0; int dec = 2;
    int regime = 0;                  // +1 pos / -1 neg / 0 unknown or STALE (never a stale regime as live)
    long long dataAgeMin = -1;       // the regime source's age (GammaProfile-MQ book), -1 = unknown
    bool qOk = false; int qm = -1, qo = -1, qv = -1;     // Q-Scores, only when fresh
    bool bandOk = false; double bandHi = NAN, bandLo = NAN;   // the Gex S/R band, only when shown and not stale
};
inline std::string mstNum(double v, int dec) { if (!std::isfinite(v)) return "?"; char b[48]; std::snprintf(b, sizeof b, "%.*f", dec < 0 ? 2 : (dec > 6 ? 6 : dec), v); return b; }
inline std::string mstHHMM(int mod) { if (mod < 0 || mod >= 1440) return "?"; char b[8]; std::snprintf(b, sizeof b, "%02d:%02d", mod / 60, mod % 60); return b; }
inline std::map<std::string, std::string> mstHodLod(const View& v, const MstIn& in)
{
    std::map<std::string, std::string> kv;
    const bool px = v.has && in.tick > 0;
    kv["hod"] = px ? mstNum((double)v.hod * in.tick, in.dec) : "?";
    kv["lod"] = px ? mstNum((double)v.lod * in.tick, in.dec) : "?";
    kv["hod_time"] = px ? mstHHMM(v.hodMod) : "?";
    kv["lod_time"] = px ? mstHHMM(v.lodMod) : "?";
    kv["first_extreme"] = (v.has && v.side == 2) ? "hod" : ((v.has && v.side == 1) ? "lod" : "?");
    const bool pj = v.projOk && in.tick > 0;
    kv["proj_hi"] = (pj && v.projUp) ? mstNum((double)v.projTo * in.tick, in.dec) : "?";
    kv["proj_lo"] = (pj && !v.projUp) ? mstNum((double)v.projTo * in.tick, in.dec) : "?";
    kv["range_used"] = (v.has && v.usedPct >= 0) ? mstNum(v.usedPct / 100.0, 2) : "?";
    kv["q_m"] = (in.qOk && in.qm >= 0) ? std::to_string(in.qm) : "?";
    kv["q_o"] = (in.qOk && in.qo >= 0) ? std::to_string(in.qo) : "?";
    kv["q_v"] = (in.qOk && in.qv >= 0) ? std::to_string(in.qv) : "?";
    kv["trend"] = v.trendReady ? std::to_string(v.trendLvl) : "?";
    kv["regime"] = in.regime > 0 ? "pos" : (in.regime < 0 ? "neg" : "?");
    kv["gex_band_hi"] = in.bandOk ? mstNum(in.bandHi, in.dec) : "?";
    kv["gex_band_lo"] = in.bandOk ? mstNum(in.bandLo, in.dec) : "?";
    kv["data_age_min"] = in.dataAgeMin >= 0 ? std::to_string(in.dataAgeMin) : "?";
    return kv;
}
// SRC "TREND": HOD / LOD owns the VWAP trend the box shows (VwapTrendLogic.h on the RTH closed bars); trend_word = the box's word
inline std::map<std::string, std::string> mstTrend(const View& v)
{
    std::map<std::string, std::string> kv;
    kv["trend"] = v.trendReady ? std::to_string(v.trendLvl) : "?";
    kv["trend_word"] = v.trendReady ? trendWord(v.trendLvl) : "?";
    return kv;
}

}  // namespace hl
#endif
