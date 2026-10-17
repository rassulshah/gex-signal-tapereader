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

static const char* const VERSION = "1.13.0";   // (1.13.0) a Setup per category (Any / Pullback with trend / Reversal at a level / Breakout through the higher of the last 2 pivots), RTH only option; (1.12.0) hover, print log + nightly score, Liquidity Profile read, NG / 6E divider 1; (1.11.0) (1.11.0) category 4 (>= lots), a font per category, time filter, near a level (ATR above / below + sweeps; key / gamma / volume levels), statistics; (1.10.0) Big Lot categories with his own N (1 - 100), per-category pivot filter and colours, day = 8:30 AM - 4:00 PM; (1.9.0) Big Lot S20 / S10 / S5 tiers, one font size (8), a colour per tier and side, box / circle / dim switches (all off), circle colour; (1.8.0) cyan digits = Big Lot S5 (the size reached ~5 times a session, day / night) instead of 3SD; (1.7.3) status counts the cyan 3SD+ prints drawn; (1.7.2) 3SD+ prints in cyan; (1.7.1) FIX: numbers never drew (y / width / height of every number were commented out); (1.7.0) sizes are Big Lot S20 / S10 / S5 (labels);   // (1.6.1) numbers that poke out of the area slide back in instead of vanishing; area fallback; drop reasons on the status file;   // (1.6.0) ON the candle: white number, box = big print / circle = absorption print in the side colour, 3SD+ bigger;   // (1.5.1) nothing under 1SD (1SD+ bold, 3SD+ outlined), circle from 1SD; non-bold only while learning;   // (1.5.0) prints on the big-lot SD sizes: 1SD faded, 2SD bold, 3SD bold + outline, nothing under 1SD;   // (1.4.1) a circled number centred in its ring; "absorption print" naming; candles as body + wick (82 of 112 numbers had no room); 1 px gap
static const int LOOKBACK = 20;        // prior candles in the median
static const int MIN_PRIOR = 10;       // fewer prior candles with data = no ratio (never bold)

// ---- settings (all real: read on every draw from the per-market file, written only from the settings window) ----------------
// ---- (1.11.0, Rassul 2026-10-17 15:06 mockup v2 approved 15:10) the settings ------------------------------------------------------
static const int NKEY = 16, NGAM = 10, NVOL = 6;
static const char* const KEY_NAMES[NKEY] = { "ONH", "ONL", "PDH", "PDL", "PFH", "PFL", "LonHI", "LonLO", "PWH", "PWL", "HOD", "LOD", "AHI", "ALO", "CWH", "CWL" };
static const char* const GAM_NAMES[NGAM] = { "CR", "PS", "HVL", "CR 0DTE", "PS 0DTE", "G1", "G2", "G3", "G4", "G5" };
static const char* const VOL_NAMES[NVOL] = { "PD POC", "PD VAH", "PD VAL", "Wk POC", "Wk VAH", "Wk VAL" };
static const char* const SETUP_NAMES = "Any;Pullback with trend;Reversal at a level;Breakout";
static const char* const SESSION_NAMES = "RTH + overnight;RTH only";
static const char* const POS_NAMES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";
struct Cat {   // one Big Lot category (1 - 3: Big Lot N + an optional minimum; 4: a fixed size, >= lots)
    bool on = false;
    int n = 5;                // Big Lot N (categories 1 - 3): the size reached ~N times a session; 1 - 100
    int lots = 0;             // categories 1 - 3: the optional minimum (0 = none); category 4: the size, >= lots (1 - 100,000)
    int font = 8;             // 6 - 20 pt
    bool pivot = false;       // only on a candle that passes the Delta Profile pivot rule
    bool level = false;       // only near a selected level (or a sweep of it)
    int setup = 0;            // (1.13.0) 0 Any, 1 Pullback with trend, 2 Reversal at a level, 3 Breakout
    unsigned bid = 0x00EF4444, ask = 0x0022C55E;   // IRT COLOR = 0x00RRGGBB
};
struct Settings {
    int div = 0;              // divider: 0 = the market's default, else 1 - 1000
    bool box = false, dim = false, circle = false;
    unsigned circleCol = 0x00FFFFFF;
    Cat cat[4];
    int skipFirst = 0, skipLast = 0;      // minutes (0 - 240) after each open / before each close
    int atrUpQ = 200, atrDnQ = 100;       // the level zone in hundredths of a 3-min ATR above / below the level (0 - 2000)
    bool keyOn = true, gamOn = true, volOn = true;
    unsigned keyMask = (1u << NKEY) - 1, gamMask = (1u << NGAM) - 1, volMask = (1u << NVOL) - 1;
    bool statsChart = true; int statsPos = 9;   // the statistics box on the chart: 1 - 9 (top left .. bottom right)
    bool rthOnly = false;     // (1.13.0, Rassul 15:29) RTH only, or RTH + overnight
    int boldQ = 200;          // internal (hover text only)
    Settings() { cat[0].on = true; cat[0].n = 5; cat[1].n = 10; cat[2].n = 20; cat[3].n = 0; cat[3].lots = 1000; }
};
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
// strict whole number (digits only, no sign, no blanks)
inline bool fullInt(const std::string& t, long long& out) {
    if (t.empty() || t.size() > 12) return false;
    for (size_t i = 0; i < t.size(); ++i) if (t[i] < '0' || t[i] > '9') return false;
    char* end = nullptr; errno = 0; const long long v = std::strtoll(t.c_str(), &end, 10);
    if (errno != 0 || !end || *end != '\0') return false;
    out = v; return true;
}
// (1.13.0) one line, 55 fields: "BBA6|div|box|dim|circle|circleCol" + 4 x "|on|N|lots|font|pivot|level|bid|ask|setup"
//   + "|skipFirst|skipLast|atrUpQ|atrDnQ|keyOn|gamOn|volOn|keyMask|gamMask|volMask|statsChart|statsPos|rthOnly"
static const size_t NFIELDS = 1 + 5 + 4 * 9 + 13;
inline std::string formatSettings(const Settings& s) {
    auto b = [](bool x) { return std::string(x ? "1" : "0"); };
    auto n = [](long long x) { return std::to_string(x); };
    std::string o = "BBA6|" + n(s.div) + "|" + b(s.box) + "|" + b(s.dim) + "|" + b(s.circle) + "|" + n(s.circleCol & 0x00FFFFFF);
    for (int k = 0; k < 4; ++k) { const Cat& c = s.cat[k];
        o += "|" + b(c.on) + "|" + n(c.n) + "|" + n(c.lots) + "|" + n(c.font) + "|" + b(c.pivot) + "|" + b(c.level) + "|" + n(c.bid & 0x00FFFFFF) + "|" + n(c.ask & 0x00FFFFFF) + "|" + n(c.setup); }
    o += "|" + n(s.skipFirst) + "|" + n(s.skipLast) + "|" + n(s.atrUpQ) + "|" + n(s.atrDnQ) + "|" + b(s.keyOn) + "|" + b(s.gamOn) + "|" + b(s.volOn)
       + "|" + n(s.keyMask) + "|" + n(s.gamMask) + "|" + n(s.volMask) + "|" + b(s.statsChart) + "|" + n(s.statsPos) + "|" + b(s.rthOnly);
    return o;
}
// every field checked on its own: a bad field falls back to ITS default (why names it); a wrong layout = all defaults;
// files from before 1.11.0 (BBA1 - BBA4) = the defaults
inline Settings parseSettings(const std::string& text, std::string& why) {
    Settings s, d; why.clear();
    std::string line = text;
    const size_t nl = line.find('\n'); if (nl != std::string::npos) line = line.substr(0, nl);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::vector<std::string> f; { std::string cur; for (char c : line) { if (c == '|') { f.push_back(cur); cur.clear(); } else cur.push_back(c); } f.push_back(cur); }
    if (!f.empty() && f[0].size() == 4 && f[0].compare(0, 3, "BBA") == 0 && f[0][3] >= '1' && f[0][3] <= '4') { why = "older file (defaults)"; return Settings(); }
    if (f.size() == 50 && f[0] == "BBA5") {   // (1.13.0) a 1.11 / 1.12 file: kept, with setup Any and RTH + overnight added
        std::vector<std::string> g; g.push_back("BBA6"); for (int i = 1; i <= 5; ++i) g.push_back(f[i]);
        for (int k = 0; k < 4; ++k) { for (int j = 0; j < 8; ++j) g.push_back(f[6 + 8 * k + j]); g.push_back("0"); }
        for (int i = 38; i < 50; ++i) g.push_back(f[i]); g.push_back("0");
        f.swap(g);
    }
    if (f.size() != NFIELDS || f[0] != "BBA6") { why = "garbled (defaults)"; return Settings(); }
    std::vector<std::string> bad;
    long long v = 0; size_t i = 1;
    auto flag = [&](bool& out, bool def, const std::string& nm) { if (fullInt(f[i], v) && v <= 1) out = v == 1; else { out = def; bad.push_back(nm); } ++i; };
    auto num = [&](int& out, int def, bool (*ok)(long long), const std::string& nm) { if (fullInt(f[i], v) && ok(v)) out = (int)v; else { out = def; bad.push_back(nm); } ++i; };
    auto col = [&](unsigned& out, unsigned def, const std::string& nm) { if (fullInt(f[i], v) && colOk(v)) out = (unsigned)v; else { out = def; bad.push_back(nm); } ++i; };
    auto mask = [&](unsigned& out, unsigned def, int bits, const std::string& nm) { if (fullInt(f[i], v) && v >= 0 && v < (1LL << bits)) out = (unsigned)v; else { out = def; bad.push_back(nm); } ++i; };
    if (fullInt(f[i], v) && (v == 0 || divOk(v))) s.div = (int)v; else { s.div = d.div; bad.push_back("divider"); } ++i;
    flag(s.box, d.box, "box"); flag(s.dim, d.dim, "dim"); flag(s.circle, d.circle, "circle"); col(s.circleCol, d.circleCol, "circle colour");
    for (int k = 0; k < 4; ++k) { Cat& c = s.cat[k]; const Cat& dc = d.cat[k]; const std::string nm = "category " + std::to_string(k + 1);
        flag(c.on, dc.on, nm + " on");
        if (k < 3) num(c.n, dc.n, nOk, nm + " Big Lot"); else { c.n = 0; ++i; }
        if (k < 3) num(c.lots, dc.lots, lotsOk, nm + " min lots");
        else { if (fullInt(f[i], v) && v >= 1 && v <= 100000) c.lots = (int)v; else { c.lots = dc.lots; bad.push_back(nm + " lots"); } ++i; }
        num(c.font, dc.font, fontOk, nm + " font");
        flag(c.pivot, dc.pivot, nm + " pivot"); flag(c.level, dc.level, nm + " level");
        col(c.bid, dc.bid, nm + " bid colour"); col(c.ask, dc.ask, nm + " ask colour"); num(c.setup, dc.setup, setupOk, nm + " setup"); }
    num(s.skipFirst, d.skipFirst, minOk, "skip first"); num(s.skipLast, d.skipLast, minOk, "skip last");
    num(s.atrUpQ, d.atrUpQ, atrOk, "ATR above"); num(s.atrDnQ, d.atrDnQ, atrOk, "ATR below");
    flag(s.keyOn, d.keyOn, "key levels"); flag(s.gamOn, d.gamOn, "gamma levels"); flag(s.volOn, d.volOn, "volume levels");
    mask(s.keyMask, d.keyMask, NKEY, "key level boxes"); mask(s.gamMask, d.gamMask, NGAM, "gamma level boxes"); mask(s.volMask, d.volMask, NVOL, "volume level boxes");
    flag(s.statsChart, d.statsChart, "statistics on the chart"); num(s.statsPos, d.statsPos, posOk, "statistics position");
    flag(s.rthOnly, d.rthOnly, "RTH only");
    if (!bad.empty()) { why = "bad value (default used):"; for (const auto& x : bad) why += " " + x; }
    return s;
}
// a typed decimal (the dialog's float box) -> hundredths, or -1 when it is not a sane number
inline int toHundredths(double x) { if (!std::isfinite(x) || x < 0 || x > 20.0001) return -1; return (int)std::floor(x * 100.0 + 0.5); }

// (1.13.0, Rassul 15:29 / 15:35) BREAKOUT through the higher of the last two HIGH pivots (his 1-bar ZigZag, pivots confirmed by the bars
// BEFORE the candle): P = the prior high pivot, or the one 2 back when it is higher; the candle breaks out when its LOW is below P and
// its HIGH above P (it is the candle that went through). up[i] = 1 (the prior pivot) / 2 (the higher one 2 back) / 0 none.
// BREAKDOWN mirror with the lower of the last two LOW pivots: dn[i]. Decided on the candle's own bars - never repainted.
template <class H, class L>
inline void breakFlags(H& high, L& low, int n, std::vector<signed char>& up, std::vector<signed char>& dn) {
    up.assign(n > 0 ? n : 0, 0); dn.assign(n > 0 ? n : 0, 0);
    if (n < 2) return;
    auto hi = [&](int i) { return (double)high[i]; }; auto lo = [&](int i) { return (double)low[i]; };
    std::vector<double> tops, bots;
    int dirn = hi(1) >= hi(0) ? 1 : -1, ext = 0;
    for (int i = 1; i < n; ++i) {
        if (!std::isfinite(hi(i)) || !std::isfinite(lo(i)) || hi(i) < lo(i)) continue;
        if (!tops.empty()) { double P = tops.back(); signed char w = 1; if (tops.size() >= 2 && tops[tops.size() - 2] > P) { P = tops[tops.size() - 2]; w = 2; }
            if (lo(i) < P && hi(i) > P) up[i] = w; }
        if (!bots.empty()) { double Q = bots.back(); signed char w = 1; if (bots.size() >= 2 && bots[bots.size() - 2] < Q) { Q = bots[bots.size() - 2]; w = 2; }
            if (lo(i) < Q && hi(i) > Q) dn[i] = w; }
        if (dirn == 1) {
            if (hi(i) > hi(ext)) ext = i;
            else if (lo(i) < lo(i - 1) && i != ext) { tops.push_back(hi(ext)); ext = i; dirn = -1; }
        } else {
            if (lo(i) < lo(ext)) ext = i;
            else if (hi(i) > hi(i - 1) && i != ext) { bots.push_back(lo(ext)); ext = i; dirn = 1; }
        }
    }
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
    if (st("ENQ")) return "NQ"; if (st("EP")) return "ES"; if (st("CLE")) return "CL"; if (st("GCE")) return "GC";
    if (st("CPE")) return "HG"; if (st("NGE")) return "NG"; if (st("EU6")) return "EU"; return "";
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

} // namespace bba
#endif
