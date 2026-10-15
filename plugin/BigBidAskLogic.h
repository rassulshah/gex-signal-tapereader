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
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace bba {

static const char* const VERSION = "1.6.0";   // (1.6.0) ON the candle: white number, box = big print / circle = absorption print in the side colour, 3SD+ bigger;   // (1.5.1) nothing under 1SD (1SD+ bold, 3SD+ outlined), circle from 1SD; non-bold only while learning;   // (1.5.0) prints on the big-lot SD sizes: 1SD faded, 2SD bold, 3SD bold + outline, nothing under 1SD;   // (1.4.1) a circled number centred in its ring; "absorption print" naming; candles as body + wick (82 of 112 numbers had no room); 1 px gap
static const int LOOKBACK = 20;        // prior candles in the median
static const int MIN_PRIOR = 10;       // fewer prior candles with data = no ratio (never bold)

// ---- settings (all real: read on every draw from the per-market file, written only from the settings window) ----------------
struct Settings {
    int font = 7;             // 6 - 20 pt   (1.4.0, Rassul 11:51 "size should be 7 by default")
    int boldQ = 200;          // bold threshold in hundredths of x (2.00x); 1.20x - 10.00x
    bool circle = true;       // draw the absorption circle (on by default)
    bool nonBold = false;     // (1.5.1) only while learning (no SD sizes yet): draw the numbers that are not bold
    int div = 0;              // (1.4.0) divider: 0 = the market's default, else 1 - 1000 (500 lots / 10 = "50")
    unsigned buy = 0x0022C55E, sell = 0x00EF4444;   // (1.4.0) buyers (at the ask) green, sellers (at the bid) red - his colours
};
// (1.4.0, Rassul 11:51 "a divider ... so the value always looks like a 1 or 2 digit value; for ES 10 by default") per market, from the
// typical biggest lots at one price in a 3-min candle (ES ~300-1,500, CL / NG / 6E ~100-500, NQ / GC / HG ~20-150)
inline int divFor(const std::string& m) {
    if (m == "ES" || m == "CL" || m == "NG" || m == "EU") return 10;
    return 1;                                   // NQ, GC, HG: already 1-2 digits
}
inline bool divOk(long long v) { return v >= 1 && v <= 1000; }
inline bool fontOk(long long v) { return v >= 6 && v <= 20; }
inline bool boldOk(long long q) { return q >= 120 && q <= 1000; }
inline int toQ(double x) { if (!std::isfinite(x)) return -1; const double q = std::floor(x * 100.0 + 0.5); return q < 0 || q > 1e9 ? -1 : (int)q; }
inline Settings clean(Settings s) {
    Settings d;
    if (!fontOk(s.font)) s.font = d.font;
    if (!boldOk(s.boldQ)) s.boldQ = d.boldQ;
    return s;
}
// one line "BBA1|font|boldQ|circle|nonBold"; anything else = defaults (why says so)
inline bool fullInt(const std::string& t, long long& out) {
    if (t.empty() || t.size() > 12) return false;
    for (size_t i = 0; i < t.size(); ++i) if (t[i] < '0' || t[i] > '9') return false;
    char* end = nullptr; errno = 0; const long long v = std::strtoll(t.c_str(), &end, 10);
    if (errno != 0 || !end || *end != '\0') return false;
    out = v; return true;
}
inline std::string formatSettings(const Settings& s) {
    return "BBA2|" + std::to_string(s.font) + "|" + std::to_string(s.boldQ) + "|" + (s.circle ? "1" : "0") + "|" + (s.nonBold ? "1" : "0")
         + "|" + std::to_string(s.div) + "|" + std::to_string(s.buy & 0x00FFFFFF) + "|" + std::to_string(s.sell & 0x00FFFFFF);
}
inline Settings parseSettings(const std::string& text, std::string& why) {
    Settings s; why.clear();
    std::string line = text;
    const size_t nl = line.find('\n'); if (nl != std::string::npos) line = line.substr(0, nl);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::vector<std::string> f; { std::string cur; for (char c : line) { if (c == '|') { f.push_back(cur); cur.clear(); } else cur.push_back(c); } f.push_back(cur); }
    const bool v1 = f.size() == 5 && f[0] == "BBA1", v2 = f.size() == 8 && f[0] == "BBA2";
    if (!v1 && !v2) { why = "garbled (defaults)"; return Settings(); }
    long long a = 0, b = 0, c = 0, d = 0;
    if (!fullInt(f[1], a) || !fullInt(f[2], b) || !fullInt(f[3], c) || !fullInt(f[4], d) || c > 1 || d > 1) { why = "garbled (defaults)"; return Settings(); }
    if (fontOk(a)) s.font = (int)a; else why = "font out of range (default)";
    if (boldOk(b)) s.boldQ = (int)b; else why = "bold threshold out of range (default)";
    s.circle = c == 1; s.nonBold = d == 1;
    if (v1) {   // (1.4.0) the 1.3.x file: font 7, the circle on, a bold line over 5.00x (a font typed into the wrong box) back to 2.00x
        s.font = Settings().font; s.circle = true; if (s.boldQ > 500) s.boldQ = Settings().boldQ;
        why = "1.3 file migrated (font 7, circle on" + std::string(b > 500 ? ", bold 2.00x" : "") + ")";
        return s;
    }
    long long e = 0, g = 0, h = 0;
    if (!fullInt(f[5], e) || !fullInt(f[6], g) || !fullInt(f[7], h) || g > 0xFFFFFF || h > 0xFFFFFF) { why = "garbled (defaults)"; return Settings(); }
    s.div = (e == 0 || divOk(e)) ? (int)e : 0; if (e != 0 && !divOk(e)) why = "divider out of range (market default)";
    s.buy = (unsigned)g; s.sell = (unsigned)h;
    if (s.buy == s.sell) { s.buy = Settings().buy; s.sell = Settings().sell; why = "same colour for both sides (defaults)"; }
    return s;
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
    bool huge = false;                 // (1.5.0) 3SD or more: a white outline round the number
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
