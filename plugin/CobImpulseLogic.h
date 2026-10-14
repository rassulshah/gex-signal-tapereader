#ifndef LS_COBIMPULSE_LOGIC_H
#define LS_COBIMPULSE_LOGIC_H
// CobImpulseLogic.h (0.3.0, 2026-10-14; 0.2.0 adds +C, 0.3.0 adds the gamma part) -- SDK-free logic of lsCobImpulse (CobImpulse.cpp),
// unit-tested on Linux.
//
// 0.3.0 (Rassul 15:47 / 15:51 CT 10/14 "identify if a gamma level is driving the CoB", approved format Cob+<R/V/C>+<gamma name>?,
//   e.g. Cob+R+CR?, Cob+RC+PS?, Cob+V+HVL?, Cob+CR?, Cob+GM?; spec research-artifacts/cob-gamma/REPORT-2026-10-14.md, last section):
//   ONE gamma name per CoB, priority ORIGIN > CROSS > TARGET, nearest wins, a named level beats GM on a tie (lra.cob_gamma.drivers).
//   Book = lsFlexLevels\GammaProfile-MQ-<MKT>.csv as read at the CoB bar close (CW -> CR, PW -> PS, FLIP -> HVL, STRIKE types CR0 /
//   PS0 / HVL0 / GW0, rank 1-3 within 1 EM and outside every named zone -> GM, EM = (EMH - EML) / 2). No G when the book is missing /
//   for another session / newer than the CoB / older than 35 min at a CoB in RTH, or when (EMH + EML) / 2 is more than 0.20 ADR from the
//   chart's prior-session 15:00 CT close. Always "?" (LEARNING: the study found NO edge from a gamma driver; a node before 2R hurt).
//   Decided once at the CoB close, recorded (Rec 0.3.0, 23 fields) and never repainted; GAMMA,0 hides it (still computed and logged).
//
// Rassul 2026-10-14 12:07 CT (approved, mockup cob/mockup-cob-impulse.png): "tag CoB signals that have an IMPULSE at their origin".
//   Bull CoB (his cob_bull_3m_1b): a bar from the CoB leg's pivot-LOW bar up to 2 bars after it whose LOW is below the HIGH of the
//   pivot-low bar and whose range (high - low) is the biggest in >= 35 bars -> "+R"; the same window / condition with the biggest
//   VOLUME in >= 35 bars -> "+V"; both -> "+RV". Bear = mirror (pivot-HIGH bar, impulse bar's HIGH above the pivot bar's LOW).
//   Label "Cob+R?" / "Cob+V?" / "Cob+RV?" ("?" until the study proves it - PROVEN,1 in the market file drops it), a thin 1 px white
//   box around the impulse bar, no numbers on the chart; the details (bar time 12-hour CT, "biggest range in N bars", 2.4x the
//   average) in the hover. Plain "Cob" when there is no impulse.
// Coordinator 2026-10-14 (study: the impulse tag alone flipped sign BUILD -> VALIDATE; the only consistent lead is Abs on the pivot
//   + impulse, LEARNING): a per-market display setting "Impulse tag" Off / Only with Abs on pivot / Always (default Only with Abs on
//   pivot). The tag is ALWAYS computed and logged; the mode only decides whether it is shown.
//
// 0.2.0 (Rassul 13:31 CT) "+C" = a CLIMAX at the origin: bull - a RED bar (close < open) from the pivot-low bar -2 .. +2 that is the
//   biggest VOLUME in >= 35 bars (a selling climax), and afterwards, by the CoB bar, a bar that CLOSES ABOVE that climax bar's HIGH;
//   bear = a GREEN biggest-volume bar near the pivot high, then a close below its LOW. Tags in fixed order R, V, C ("Cob+RVC?").
//   Logged separately (the earlier screen found "climax into the extreme" weaker than "thrust with the reversal").
// Decision time only: everything here reads bars 0 .. trig (the CoB bar, closed); bars after it are never read.
// "Biggest in N bars" = the platform's own "since" convention (BGB_Since / BV_Since): N = bars back to the last STRICTLY bigger bar
//   (a tie is not bigger, so a tie keeps the count going); no bigger bar in the history = the bars available (i + 1); capped at 100.
//   A bar whose value is unknown (missing / zero volume, broken bar) stops the count - unknown is never counted as smaller.
// The CoB leg's pivot = Rassul's 1-bar ZigZag start of the leg that ended on the bar before the CoB (trade_def.cob_live /
//   zigzag, matched to the platform's pivots on 29,185 of 29,187 ES pivots).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <string>
#include <vector>

namespace cimp {

static const char* const VERSION = "0.3.1";   // 0.3.1 (2026-10-14 18:25 CT audit display list): the gamma hover says which MenthorQ level is NEAR the CoB start, "(model, data N min old)", never "driving" / "behind"; the CoB / RB marks and the tags are unchanged
enum { MODE_OFF = 0, MODE_ABS = 1, MODE_ALWAYS = 2 };

struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0, v = 0; };   // t = the chart's bar stamp (bar END, IRT)

struct Params {
    int minBars = 35;          // "biggest in >= 35 bars"
    int window = 2;            // the pivot bar .. 2 bars after it
    int cap = 100;             // "since" counts stop at 100 (as the platform's)
    int avgBars = 20;          // the average the ratio (2.4x) is taken against: the 20 bars before the impulse bar
    int mode = MODE_ABS;       // display: Off / Only with Abs on pivot / Always
    bool proven = false;       // true = the study proved it for this market: the "?" is dropped
    bool plain = true;         // draw plain "Cob" labels too (the plugin is the one CoB label source once IRT's marker text is cleared)
    bool gamma = true;         // (0.3.0) GAMMA,1 = show the gamma part of the tag (always computed and logged)
    int gammaW = 6;            // (0.3.0) GAMMA_W: zone half-width, hundredths of the ADR (3-10)
    int gammaEM = 100;         // (0.3.0) GAMMA_EM: how far ahead a target node may be, % of the expected move (25-150)
};

inline bool fin(double x) { return x == x && x < 1e300 && x > -1e300; }
inline bool barOk(const Bar& b) { return fin(b.h) && fin(b.l) && b.h >= b.l; }
inline bool volOk(const Bar& b) { return fin(b.v) && b.v > 0; }

// N for bar i: bars back to the last strictly bigger (or unknown) bar; none = i + 1; capped. 0 = this bar's value is unknown.
template <class Val, class Ok>
inline int sinceBigger(const std::vector<Bar>& B, int i, int cap, Val val, Ok ok)
{
    if (i < 0 || i >= (int)B.size() || !ok(B[i])) return 0;
    const double x = val(B[i]);
    for (int j = i - 1; j >= 0 && i - j < cap; --j) {
        if (!ok(B[j]) || val(B[j]) > x) return i - j;
    }
    return std::min(cap, i + 1);
}
inline int rangeSince(const std::vector<Bar>& B, int i, int cap) {
    return sinceBigger(B, i, cap, [](const Bar& b) { return b.h - b.l; }, barOk);
}
inline int volSince(const std::vector<Bar>& B, int i, int cap) {
    return sinceBigger(B, i, cap, [](const Bar& b) { return b.v; }, volOk);
}
// value / the average of up to n known bars before i (at least 5 needed, else 0)
template <class Val, class Ok>
inline double ratioToAvg(const std::vector<Bar>& B, int i, int n, Val val, Ok ok)
{
    if (i < 0 || i >= (int)B.size() || !ok(B[i])) return 0;
    double s = 0; int k = 0;
    for (int j = i - 1; j >= 0 && k < n; --j) if (ok(B[j])) { s += val(B[j]); ++k; }
    if (k < 5 || !(s > 0)) return 0;
    return val(B[i]) / (s / k);
}

// The CoB leg's pivot for a CoB on bar trig: walk the 1-bar ZigZag over bars 0 .. trig-1 (as trade_def.cob_live does) and return the
// start of the current leg when it runs the CoB's way (up for bull, side +1). -1 = the leg does not run that way (no tag).
inline int legPivot(const std::vector<Bar>& B, int trig, int side, int* legEnd = nullptr)
{
    if (trig < 2 || trig >= (int)B.size()) return -1;
    for (int i = 0; i < trig; ++i) if (!barOk(B[i])) return -1;
    int dirn = B[1].h >= B[0].h ? 1 : -1, ext = 0, start = 0;
    for (int i = 1; i < trig; ++i) {
        if (dirn == 1) {
            if (B[i].h > B[ext].h) ext = i;
            else if (B[i].l < B[i - 1].l && i != ext) { start = ext; ext = i; dirn = -1; }
        } else {
            if (B[i].l < B[ext].l) ext = i;
            else if (B[i].h > B[i - 1].h && i != ext) { start = ext; ext = i; dirn = 1; }
        }
    }
    if (dirn != (side > 0 ? 1 : -1)) return -1;
    if (legEnd) *legEnd = ext;
    return start;
}

struct Impulse { int bar = -1; int n = 0; double x = 0; };
struct Tag {
    int side = 0, trig = -1, pivot = -1, legEnd = -1;
    bool abs = false;            // Abs (his Abs_Bull / Abs_Bear) on the pivot bar
    Impulse r, v;                // range / volume thrust (bar -1 = none)
    Impulse c; int cConf = -1;   // (0.2.0) climax bar and the bar whose close confirmed it
    std::string why;             // why no tag ("no pivot", "no thrust", ...)
    bool any() const { return r.bar >= 0 || v.bar >= 0 || c.bar >= 0; }
};

// the tag of a CoB on bar trig (closed). absOnPivot = his Abs flag on the pivot bar, read at the CoB's close.
inline Tag evaluate(const std::vector<Bar>& B, int trig, int side, const Params& P, bool absOnPivot)
{
    Tag T; T.side = side > 0 ? 1 : -1; T.trig = trig;
    int le = -1;
    const int p = legPivot(B, trig, T.side, &le);
    if (p < 0) { T.why = "no leg pivot"; return T; }
    T.pivot = p; T.legEnd = le; T.abs = absOnPivot;
    const int last = std::min(trig, p + std::max(0, P.window));
    for (int k = p; k <= last; ++k) {
        if (!barOk(B[k])) continue;
        const bool inside = T.side > 0 ? B[k].l < B[p].h : B[k].h > B[p].l;
        if (!inside) continue;
        const int nr = rangeSince(B, k, P.cap);
        if (nr >= P.minBars && nr > T.r.n) { T.r.bar = k; T.r.n = nr; }
        const int nv = volSince(B, k, P.cap);
        if (nv >= P.minBars && nv > T.v.n) { T.v.bar = k; T.v.n = nv; }
    }
    // (0.2.0) +C: an opposite-colour biggest-volume bar from pivot-2 .. pivot+2, then a close beyond it by the CoB bar
    for (int k = std::max(0, p - 2); k <= std::min(trig, p + 2); ++k) {
        if (!barOk(B[k]) || !fin(B[k].o) || !fin(B[k].c)) continue;
        const bool against = T.side > 0 ? B[k].c < B[k].o : B[k].c > B[k].o;
        if (!against) continue;
        const int nv = volSince(B, k, P.cap);
        if (nv < P.minBars || nv <= T.c.n) continue;
        int conf = -1;
        for (int m = k + 1; m <= trig; ++m) {
            if (!fin(B[m].c)) continue;
            if (T.side > 0 ? B[m].c > B[k].h : B[m].c < B[k].l) { conf = m; break; }
        }
        if (conf < 0) continue;
        T.c.bar = k; T.c.n = nv; T.cConf = conf;
    }
    if (T.c.bar >= 0) T.c.x = ratioToAvg(B, T.c.bar, P.avgBars, [](const Bar& b) { return b.v; }, volOk);
    if (T.r.bar >= 0) T.r.x = ratioToAvg(B, T.r.bar, P.avgBars, [](const Bar& b) { return b.h - b.l; }, barOk);
    if (T.v.bar >= 0) T.v.x = ratioToAvg(B, T.v.bar, P.avgBars, [](const Bar& b) { return b.v; }, volOk);
    if (!T.any()) T.why = "no thrust";
    return T;
}

inline std::string code(bool r, bool v, bool c = false) { return std::string(r ? "R" : "") + (v ? "V" : "") + (c ? "C" : ""); }
// is the tag shown in this mode?
inline bool shown(bool any, bool abs, int mode) { return any && (mode == MODE_ALWAYS || (mode == MODE_ABS && abs)); }
// the chart / Reader label: "Cob", "Cob+R?", "Cob+V?", "Cob+C?", "Cob+RV?", "Cob+RVC?" ... ("?" dropped when proven)
// (0.3.0) + the gamma name g: "Cob+R+CR?", "Cob+RC+PS?", "Cob+CR?", "Cob+GM?" - the gamma part always carries "?" (never proven)
inline std::string label(bool r, bool v, bool abs, const Params& P, bool c = false, const std::string& g = std::string())
{
    const bool imp = shown(r || v || c, abs, P.mode), gs = P.gamma && !g.empty();
    if (!imp && !gs) return "Cob";
    std::string s = "Cob";
    if (imp) s += "+" + code(r, v, c);
    if (gs) s += "+" + g;
    return s + ((gs || !P.proven) ? "?" : "");
}

// strict full-token parsers (shared by the market file, the book and the record)
inline bool parseInt(const std::string& s, long lo, long hi, int& out)
{
    if (s.empty() || s.size() > 6) return false;
    errno = 0; char* e = nullptr; const long v = std::strtol(s.c_str(), &e, 10);
    if (errno || e != s.c_str() + s.size() || v < lo || v > hi) return false;
    out = (int)v; return true;
}
inline std::string trim(const std::string& s)
{
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return s.substr(a, b - a);
}
// ======================================================================================= (0.3.0) the gamma part of the tag
// Same rules as lra.cob_gamma.drivers (the study) and lra.cob_tag.gamma_part (the Reader's TRADE NOW) - parity-tested on random cases.
static const double ALIGN_TOL = 0.20;            // lra.cob_gamma.ALIGN_TOL: band midpoint vs the prior 15:00 CT close, in ADR
static const long long STALE_SEC = 35LL * 60;    // a book older than 35 min at a CoB in RTH = stale
static const int GM_RANK = 3;                    // rank 1-3 (abs GEX) = a GM node
inline int namePri(const std::string& n)
{
    static const char* const N[] = { "CR", "PS", "HVL", "GW0", "CR0", "PS0", "HVL0" };   // lra.cob_gamma.NAME_PRI (GM and the rest = 9)
    for (int i = 0; i < 7; ++i) if (n == N[i]) return i;
    return 9;
}
inline bool isBig(const std::string& n) { return n == "CR" || n == "PS" || n == "CR0" || n == "PS0" || n == "GW0" || n == "GM"; }
inline bool isGName(const std::string& n) { return n == "CR" || n == "PS" || n == "HVL" || n == "CR0" || n == "PS0" || n == "HVL0" || n == "GW0" || n == "GM"; }
inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
inline long long civil(int y, unsigned m, unsigned d)
{
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
// the session (trading day) of a bar END stamp in CT seconds: the 17:03 bar belongs to the next day (trade_def.prepare's sdate)
inline long long sessDayOf(long long ctSec) { return floorDiv(ctSec - 60 + 7 * 3600, 86400); }
inline int minuteOf(long long ctSec) { return (int)(floorDiv(ctSec, 60) - floorDiv(ctSec, 86400) * 1440); }
// RTH bar END window (minute of day) per market - lra.markets rth_first / rth_last
inline bool rthWindow(const std::string& m, int& first, int& last)
{
    if (m == "ES" || m == "NQ") { first = 8 * 60 + 33; last = 15 * 60; return true; }
    if (m == "CL" || m == "NG") { first = 8 * 60 + 3; last = 13 * 60 + 30; return true; }
    if (m == "GC" || m == "HG") { first = 7 * 60 + 24; last = 12 * 60 + 30; return true; }
    if (m == "EU") { first = 7 * 60 + 24; last = 14 * 60; return true; }
    return false;
}
inline bool parseDate(const std::string& s, long long& days)
{
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (int i = 0; i < 10; ++i) if (i != 4 && i != 7 && (s[(size_t)i] < '0' || s[(size_t)i] > '9')) return false;
    const int y = std::atoi(s.substr(0, 4).c_str()), m = std::atoi(s.substr(5, 2).c_str()), d = std::atoi(s.substr(8, 2).c_str());
    if (y < 2000 || y > 2100 || m < 1 || m > 12 || d < 1 || d > 31) return false;
    days = civil(y, (unsigned)m, (unsigned)d); return true;
}
inline bool parseNum(const std::string& s, double& out)       // finite, any sign, full token, plain decimal (no hex / inf / nan)
{
    if (s.empty() || s.size() > 24) return false;
    for (char ch : s) if (!((ch >= '0' && ch <= '9') || ch == '+' || ch == '-' || ch == '.' || ch == 'e' || ch == 'E')) return false;
    errno = 0; char* e = nullptr; const double v = std::strtod(s.c_str(), &e);
    if (errno || e != s.c_str() + s.size() || !fin(v)) return false;
    out = v; return true;
}

// ---- the book  lsFlexLevels\GammaProfile-MQ-<MKT>.csv (written by the bridge from MenthorQ's intraday / EOD book)
struct GStrike { double px = 0, pct = 0; int rank = 0; std::string types; };
struct GBook {
    bool ok = false; std::string why;             // why the book was rejected ("" = ok)
    long long sessDay = 0, asof = 0;              // SESSION (civil days), ASOF (CT seconds = civil days x 86400 + seconds of day)
    double emh = 0, eml = 0, cw = 0, pw = 0, flip = 0; bool hasCW = false, hasPW = false, hasFlip = false;
    std::vector<GStrike> strikes;
};
inline std::vector<std::string> splitC(const std::string& s)
{
    std::vector<std::string> f; size_t a = 0;
    for (size_t i = 0; i <= s.size(); ++i) if (i == s.size() || s[i] == ',') { f.push_back(trim(s.substr(a, i - a))); a = i + 1; }
    return f;
}
// strict: MARKET must match, SESSION / ASOF / EMH > EML required, numbers full-token and finite; unknown keys are skipped
inline GBook parseBook(const std::string& text, const std::string& market)
{
    GBook B, bad;
    if (text.size() > 64 * 1024) { bad.why = "book over 64 KB"; return bad; }
    bool sM = false, sS = false, sA = false, sH = false, sL = false; size_t a = 0; int n = 0;
    while (a <= text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        const std::string line = trim(text.substr(a, e - a)); a = e + 1;
        if (!line.empty() && line[0] != '#') {
            if (++n > 600) { bad.why = "book has too many lines"; return bad; }
            const std::vector<std::string> f = splitC(line); const std::string& k = f[0]; double x = 0;
            if (k == "MARKET") { if (f.size() < 2 || f[1] != market) { bad.why = "book MARKET is not " + market; return bad; } sM = true; }
            else if (k == "SESSION") { if (f.size() < 2 || !parseDate(f[1], B.sessDay)) { bad.why = "bad SESSION"; return bad; } sS = true; }
            else if (k == "ASOF") {
                long long d = 0; int sec = 0;
                if (f.size() < 3 || !parseInt(f[1], 0, 86399, sec) || !parseDate(f[2], d)) { bad.why = "bad ASOF"; return bad; }
                B.asof = d * 86400 + sec; sA = true;
            }
            else if (k == "EMH") { if (f.size() < 2 || !parseNum(f[1], B.emh)) { bad.why = "bad EMH"; return bad; } sH = true; }
            else if (k == "EML") { if (f.size() < 2 || !parseNum(f[1], B.eml)) { bad.why = "bad EML"; return bad; } sL = true; }
            else if (k == "CW") { if (f.size() < 2 || !parseNum(f[1], x)) { bad.why = "bad CW"; return bad; } B.cw = x; B.hasCW = true; }
            else if (k == "PW") { if (f.size() < 2 || !parseNum(f[1], x)) { bad.why = "bad PW"; return bad; } B.pw = x; B.hasPW = true; }
            else if (k == "FLIP") { if (f.size() < 2 || !parseNum(f[1], x)) { bad.why = "bad FLIP"; return bad; } B.flip = x; B.hasFlip = true; }
            else if (k == "STRIKE") {
                GStrike s;
                if (f.size() < 7 || !parseNum(f[1], s.px) || !parseNum(f[2], s.pct) || !parseInt(f[3], 0, 100000, s.rank)) { bad.why = "bad STRIKE line"; return bad; }
                s.types = f[5];
                if (B.strikes.size() >= 400) { bad.why = "too many STRIKE lines"; return bad; }
                B.strikes.push_back(s);
            }
        }
        if (e == text.size()) break;
    }
    if (!sM || !sS || !sA || !sH || !sL) { bad.why = "incomplete book"; return bad; }
    if (!(B.emh > B.eml)) { bad.why = "EMH not above EML"; return bad; }
    B.ok = true; return B;
}

struct GLevel { std::string name; double px = 0; int sign = 0; };   // sign: +1 dealers long gamma, -1 short (model), 0 unknown
inline int signAt(const GBook& b, double px)
{
    for (const auto& s : b.strikes) if (std::fabs(s.px - px) < 1e-9) return s.pct > 0 ? 1 : s.pct < 0 ? -1 : 0;
    return 0;
}
// the levels as the study's Book.at builds them: named (CR PS HVL, then the strikes' CR0 PS0 HVL0 GW0 in file order), then GM = a rank 1-3
// strike within 1 EM of the entry that is outside every named level's zone (one node: the named level wins), no duplicate GM price
inline std::vector<GLevel> levels(const GBook& b, double entry, double w, double em)
{
    std::vector<GLevel> L;
    if (b.hasCW) L.push_back(GLevel{ "CR", b.cw, signAt(b, b.cw) });
    if (b.hasPW) L.push_back(GLevel{ "PS", b.pw, signAt(b, b.pw) });
    if (b.hasFlip) L.push_back(GLevel{ "HVL", b.flip, signAt(b, b.flip) });
    for (const auto& s : b.strikes) {
        size_t a = 0;
        for (size_t i = 0; i <= s.types.size(); ++i) if (i == s.types.size() || s.types[i] == '+') {
            const std::string t = trim(s.types.substr(a, i - a)); a = i + 1;
            if (t == "CR0" || t == "PS0" || t == "HVL0" || t == "GW0") L.push_back(GLevel{ t, s.px, s.pct > 0 ? 1 : s.pct < 0 ? -1 : 0 });
        }
    }
    const size_t nNamed = L.size();
    for (const auto& s : b.strikes) {
        if (s.rank < 1 || s.rank > GM_RANK || !(std::fabs(s.px - entry) <= em)) continue;
        bool skip = false;
        for (size_t i = 0; i < nNamed && !skip; ++i) if (std::fabs(s.px - L[i].px) <= w) skip = true;
        for (size_t i = nNamed; i < L.size() && !skip; ++i) if (std::fabs(L[i].px - s.px) < 1e-9) skip = true;
        if (!skip) L.push_back(GLevel{ "GM", s.px, s.pct > 0 ? 1 : s.pct < 0 ? -1 : 0 });
    }
    return L;
}
// lra.cob_gamma.drivers, line for line: em here = the target reach (EM x GAMMA_EM / 100); rthHi / rthLo = the RTH range up to the CoB bar
// (no RTH bar yet: rthHi = -inf, rthLo = +inf, nothing is "visited")
struct GHit { bool has = false; GLevel lv; double dist = 0; };
struct GDrv { GHit origin, cross, target; int shortg = -1; bool hasHvl = false; double hvl = 0; };
inline GDrv drivers(bool bull, double ext, double entry, const std::vector<GLevel>& L, double em, double w, double rthHi, double rthLo)
{
    GDrv o;
    if (L.empty() || !(em > 0) || !(w > 0)) return o;
    for (const auto& l : L) if (l.name == "HVL" && fin(l.px)) {
        o.hasHvl = true; o.hvl = l.px; o.shortg = entry < l.px ? 1 : 0;
        if (bull ? (ext < l.px && l.px < entry) : (ext > l.px && l.px > entry)) { o.cross.has = true; o.cross.lv = GLevel{ "HVL", l.px, l.sign }; o.cross.dist = std::fabs(entry - l.px); }
        break;
    }
    double bd = 0; int bp = 0;
    for (const auto& l : L) {
        const double d = std::fabs(ext - l.px);
        if (!(d <= w + 1e-12)) continue;
        const int p = namePri(l.name);
        if (!o.origin.has || d < bd || (d == bd && p < bp)) { o.origin.has = true; o.origin.lv = l; o.origin.dist = d; bd = d; bp = p; }
    }
    double td = 0; int tp = 0;
    for (const auto& l : L) {
        if (!isBig(l.name)) continue;
        const double px = l.px;
        const bool ahead = bull ? (px - w > entry) : (px + w < entry);
        const double dist = bull ? (px - entry) : (entry - px);
        if (!ahead || dist > em) continue;
        if ((px - w <= rthHi) && (px + w >= rthLo)) continue;      // traded into this RTH session already
        const int p = namePri(l.name);
        if (!o.target.has || dist < td || (dist == td && p < tp)) { o.target.has = true; o.target.lv = l; o.target.dist = dist; td = dist; tp = p; }
    }
    return o;
}

// the decision inputs (all read at the CoB bar close from the chart's own bars) and the decided gamma part
struct GIn {
    bool bull = true; double ext = 0, entry = 0, tick = 0;  // ext = the leg pivot's low (bull) / high (bear); entry = the CoB close
    long long trigCt = 0, sessDay = 0; bool inRth = false;  // the CoB bar END in CT seconds, its session, inside RTH
    double adr = 0; bool hasClose15 = false; double close15 = 0;  // 20-day ADR (as trade_def), the prior session's 15:00 CT close
    double rthHi = -1e300, rthLo = 1e300;                   // the RTH range up to and including the CoB bar (none: nothing visited)
};
struct GTag {
    bool computed = false;                 // false = a pre-0.3.0 record (no gamma part was decided)
    std::string why;                       // "" = the book was read; else no book / wrong session / late book / stale / no ADR / misaligned
    std::string code, kind;                // CR PS HVL CR0 PS0 HVL0 GW0 GM ('' = none); origin / cross / target
    double px = 0; int sign = 0; long long distTicks = 0; char hedge = 0;   // hedge: W with the CoB, A against, U unknown (model)
    long long asof = 0;                    // the book's ASOF (CT seconds), 0 = none
    // hover context (the .gamma.csv sidecar): zone, EM, HVL regime, the nearest unvisited big node ahead (even when not the driver)
    int gw = 0; double w = 0, em = 0; bool hasHvl = false; double hvl = 0; int below = -1;
    bool hasT = false; std::string tName; double tPx = 0; int tSign = 0; double tR = 0, tEm = 0;
};
// the stored precision (%.10g): every number the record / sidecar keeps is rounded to it AT the decision, so the hover drawn then and
// the hover rebuilt from the files after a restart are identical (1.7499999 -> "1.75" -> 1.8R otherwise)
inline double q10(double x) { char b[40]; std::snprintf(b, sizeof(b), "%.10g", x); return std::strtod(b, nullptr); }
inline const char* const* whyList() { static const char* const W[] = { "", "no book", "wrong session", "late book", "stale", "no ADR", "misaligned", "pre 0.3.0", nullptr }; return W; }
inline bool okWhy(const std::string& s) { for (const char* const* w = whyList(); *w; ++w) if (s == *w) return true; return false; }
inline GTag gammaTag(const GIn& in, const GBook* book, const Params& P)
{
    GTag g; g.computed = true; g.gw = P.gammaW;
    if (!book || !book->ok) { g.why = "no book"; return g; }
    g.asof = book->asof;
    if (book->sessDay != in.sessDay) { g.why = "wrong session"; return g; }
    if (book->asof > in.trigCt) { g.why = "late book"; return g; }
    if (in.inRth && in.trigCt - book->asof > STALE_SEC) { g.why = "stale"; return g; }
    if (!(fin(in.adr) && in.adr > 0)) { g.why = "no ADR"; return g; }
    const double mid = (book->emh + book->eml) / 2;
    if (!in.hasClose15 || !fin(in.close15) || !(std::fabs(in.close15 - mid) <= ALIGN_TOL * in.adr)) { g.why = "misaligned"; return g; }
    const double em = (book->emh - book->eml) / 2, w = P.gammaW / 100.0 * in.adr;
    g.w = q10(w); g.em = q10(em);
    const std::vector<GLevel> L = levels(*book, in.entry, w, em);
    const GDrv D = drivers(in.bull, in.ext, in.entry, L, em * P.gammaEM / 100.0, w, in.rthHi, in.rthLo);
    g.hasHvl = D.hasHvl; g.hvl = q10(D.hvl); g.below = D.hasHvl ? (in.entry < D.hvl ? 1 : 0) : -1;
    const double risk = in.bull ? in.entry - (in.ext - in.tick) : (in.ext + in.tick) - in.entry;
    if (D.target.has) {
        g.hasT = true; g.tName = D.target.lv.name; g.tPx = q10(D.target.lv.px); g.tSign = D.target.lv.sign;
        g.tR = q10(risk > 0 ? D.target.dist / risk : 0); g.tEm = q10(em > 0 ? D.target.dist / em : 0);
    }
    const GHit* h = D.origin.has ? &D.origin : D.cross.has ? &D.cross : D.target.has ? &D.target : nullptr;
    if (!h) return g;
    g.code = h->lv.name; g.kind = h == &D.origin ? "origin" : h == &D.cross ? "cross" : "target";
    g.px = q10(h->lv.px); g.sign = h->lv.sign;
    g.distTicks = in.tick > 0 ? (long long)std::llround(h->dist / in.tick) : 0;
    if (g.kind == "origin" && (g.code == "HVL" || g.code == "HVL0")) g.hedge = D.hasHvl ? (in.entry < D.hvl ? 'W' : 'A') : 'U';
    else if (g.kind == "cross") g.hedge = in.bull ? 'A' : 'W';   // a bull CoB is now above the flip (dealers lean against), a bear below (chase)
    else g.hedge = g.sign < 0 ? 'W' : g.sign > 0 ? 'A' : 'U';
    return g;
}

// prices at the market's tick precision with thousands separators, trailing zeros dropped - lra.cob_gamma.fmt_px
inline std::string fmtPx(double px, double tick)
{
    char tb[64]; std::snprintf(tb, sizeof(tb), "%f", tick > 0 && tick < 1000 ? tick : 0.01);
    std::string t = tb; while (!t.empty() && t.back() == '0') t.pop_back();
    const size_t dot = t.find('.'); const int dec = dot == std::string::npos ? 0 : (int)(t.size() - dot - 1);
    char b[64]; std::snprintf(b, sizeof(b), "%.*f", dec, px);
    std::string s = b, sign; if (!s.empty() && s[0] == '-') { sign = "-"; s = s.substr(1); }
    const size_t d2 = s.find('.'); std::string ip = s.substr(0, d2), fp = d2 == std::string::npos ? "" : s.substr(d2);
    std::string o; for (size_t i = 0; i < ip.size(); ++i) { if (i && (ip.size() - i) % 3 == 0) o += ','; o += ip[i]; }
    o = sign + o + fp;
    if (o.find('.') != std::string::npos) { while (o.back() == '0') o.pop_back(); if (o.back() == '.') o.pop_back(); }
    return o;
}
inline std::string fmt1(double x) { char b[32]; std::snprintf(b, sizeof(b), "%.1f", x); return b; }
inline std::string whyText(const std::string& w)
{
    if (w == "no book") return "no usable MenthorQ book for this market at the CoB close";
    if (w == "wrong session") return "the MenthorQ book was for another session";
    if (w == "late book") return "the only book arrived after this CoB (decided late)";
    if (w == "stale") return "the MenthorQ book was over 35 min old at the CoB close";
    if (w == "no ADR") return "not enough chart history for the 20-day ADR";
    if (w == "misaligned") return "the book's expected-move band is more than 0.20 ADR from this chart's prior 15:00 CT close (contract check)";
    return w;
}
// the gamma hover lines (one per row; every dealer line says "model"). showRegime = the hvl_cushion claim gate (ClaimGates.h, default
// hidden): the HVL regime cause text is left out until the external audit.
inline std::vector<std::string> gammaLines(const GTag& g, bool bull, double tick, bool showRegime, long long ageS = -1)
{
    std::vector<std::string> L;
    if (!g.computed) return L;
    if (!g.why.empty()) { L.push_back("Gamma: not read - " + whyText(g.why) + "."); return L; }
    const std::string regime = !g.hasHvl ? std::string() : g.below == 1
        ? "Regime (model): price below HVL " + fmtPx(g.hvl, tick) + " - dealers chase moves = WITH the CoB."
        : "Regime (model): price above HVL " + fmtPx(g.hvl, tick) + " - dealers lean against moves = AGAINST the CoB.";
    if (g.code.empty()) {
        L.push_back("Gamma: no gamma level at the origin, crossed, or ahead within reach.");
        if (showRegime && !regime.empty()) L.push_back(regime);
        return L;
    }
    const std::string px = fmtPx(g.px, tick);
    char zb[16]; std::snprintf(zb, sizeof(zb), "%.2f", g.gw / 100.0);
    // (0.3.1, 2026-10-14 18:25 CT audit display list) it says which MenthorQ level is NEAR the CoB start - not that it drove the move
    const std::string age = ageS < 0 ? std::string("model") : "model, data " + std::to_string((ageS + 30) / 60) + " min old";
    if (g.kind == "origin") {
        L.push_back("MenthorQ level near the CoB start: " + g.code + " " + px + " (" + age + ").");
        L.push_back("Zone: the leg started inside +-" + std::string(zb) + " ADR (+-" + fmtPx(g.w, tick) + " pts) of it.");
    }
    else if (g.kind == "cross") L.push_back("Gamma: the leg crossed HVL " + px + " (the gamma flip) on its way to the CoB close.");
    else L.push_back("Gamma: " + g.code + " " + px + " ahead, " + fmt1(g.tEm) + " EM away, not traded into this session.");
    if ((g.kind == "origin" && (g.code == "HVL" || g.code == "HVL0")) || g.kind == "cross")
        L.push_back(showRegime ? "Dealer hedge (model): the gamma flip - below it dealers chase moves, above it they lean against them."
                               : "Dealer hedge (model): the gamma flip (HVL) - the dealer side changes here.");
    else if (g.kind == "origin" && g.sign < 0)
        L.push_back("Dealer hedge (model): short gamma at " + px + (bull ? " - dealers sold into the low and buy as price rises = WITH the CoB."
                                                                         : " - dealers bought into the high and sell as price falls = WITH the CoB."));
    else if (g.kind == "origin" && g.sign > 0)
        L.push_back("Dealer hedge (model): long gamma at " + px + (bull ? " - dealers bought the dip into it (helps the turn, leans against the run beyond it)."
                                                                        : " - dealers sold the rip into it (helps the turn, leans against the run beyond it)."));
    else if (g.kind == "target" && g.sign > 0) L.push_back("Dealer hedge (model): long-gamma node ahead - can stall price at it.");
    else if (g.kind == "target" && g.sign < 0) L.push_back("Dealer hedge (model): short-gamma node ahead - dealers chase price through it.");
    else L.push_back("Dealer hedge (model): dealer side unknown.");
    if (showRegime && !regime.empty()) L.push_back(regime);
    if (g.hasT && g.tR > 0 && g.tR < 2)
        L.push_back("Ahead: " + g.tName + " " + fmtPx(g.tPx, tick) + " is " + fmt1(g.tR) + "R away, before 2R - big nodes before 2R stalled CoBs (-0.15R vs +0.18R, n 33, LEARNING).");
    L.push_back("? = still testing: CoBs that start at a MenthorQ level have NOT made more R than plain CoBs so far (94 CoBs, 34 days).");
    L.push_back("Example: Cob+CR? = the CoB started at Call Resistance; Cob+GM? = it started at a high gamma node (top-3 abs GEX).");
    return L;
}

// the chart's own sessions -> the 20-day ADR (trade_def: full-session true range, 20 simple, min 10, one day back; the first session
// has no prior close -> no TR) and each session's 15:00 CT close (the last bar ending 14:00-15:00 CT on the session's date)
struct DayAgg { long long day = 0; double h = 0, l = 0, c = 0; bool has15 = false; double c15 = 0; };
inline std::vector<DayAgg> dailyTable(const std::vector<long long>& ct, const std::vector<Bar>& B)
{
    std::vector<DayAgg> D;
    for (size_t i = 0; i < B.size() && i < ct.size(); ++i) {
        if (!barOk(B[i]) || !fin(B[i].c)) continue;
        const long long sd = sessDayOf(ct[i]);
        if (D.empty() || D.back().day != sd) {
            if (!D.empty() && sd < D.back().day) continue;                 // out of order: ignored
            DayAgg a; a.day = sd; a.h = B[i].h; a.l = B[i].l; a.c = B[i].c; D.push_back(a);
        } else { DayAgg& a = D.back(); a.h = std::max(a.h, B[i].h); a.l = std::min(a.l, B[i].l); a.c = B[i].c; }
        const int mod = minuteOf(ct[i]);
        if (floorDiv(ct[i], 86400) == sd && mod >= 14 * 60 && mod <= 15 * 60) { D.back().has15 = true; D.back().c15 = B[i].c; }
    }
    return D;
}
inline double adrAt(const std::vector<DayAgg>& D, size_t k)
{
    double s = 0; int n = 0;
    for (size_t i = k >= 20 ? k - 20 : 0; i < k; ++i) {
        if (i == 0) continue;
        const double pc = D[i - 1].c;
        s += std::max(D[i].h - D[i].l, std::max(std::fabs(D[i].h - pc), std::fabs(D[i].l - pc))); ++n;
    }
    return n >= 10 ? s / n : 0;
}

// 12-hour Central clock from hour / minute ("8:36 AM")
inline std::string clock12(int hh, int mm)
{
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59) return "?";
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", hh % 12 == 0 ? 12 : hh % 12, mm, hh < 12 ? "AM" : "PM"); return b;
}
inline std::string ratio(double x) { char b[16]; std::snprintf(b, sizeof(b), "%.1fx", x); return b; }

// the hover (question / how / why / use / example), one line per row; times already as 12-hour CT text
struct HoverIn {
    bool r = false, v = false, abs = false, c = false; int rn = 0, vn = 0, cn = 0; double rx = 0, vx = 0, cx = 0; std::string rTime, vTime, cTime, cConfTime; int side = 1;
    GTag g; double tick = 0; bool showRegime = false;    // (0.3.0) the gamma part (g.computed false = a pre-0.3.0 record)
    long long bookAgeS = -1;                             // (0.3.1) how old the MenthorQ book was at the CoB close (s); -1 unknown
};
inline std::vector<std::string> hover(const HoverIn& h, const Params& P)
{
    std::vector<std::string> L;
    const bool bull = h.side > 0, imp = h.r || h.v || h.c, gh = !h.g.code.empty();
    const std::vector<std::string> G = gammaLines(h.g, bull, h.tick, h.showRegime, h.bookAgeS);
    const std::string lab = label(h.r, h.v, h.abs, P, h.c, h.g.code);
    if (!imp && !gh) {
        L.push_back("Cob - no impulse at its start");
        L.push_back("Did the CoB leg start with force? No bar near its " + std::string(bull ? "low" : "high") + " was the biggest in 35+ bars, and no climax there.");
        L.insert(L.end(), G.begin(), G.end());
        return L;
    }
    std::string head = lab;
    if (imp && !shown(true, h.abs, P.mode)) head += "  (tag hidden by the Impulse tag setting)";
    if (gh && !P.gamma) head += "  (gamma part hidden by the GAMMA setting)";
    head += imp && gh ? " - did the CoB leg start with force, and is a MenthorQ level near its start?" : imp ? " - did the CoB leg start with force?" : " - is a MenthorQ level near this CoB's start?";   // (0.3.1) not "behind" / "driving"
    L.push_back(head);
    if (imp) {
        if (h.r) L.push_back("Range: the " + h.rTime + " bar was the biggest range in " + std::to_string(h.rn) + (h.rn >= P.cap ? "+" : "") + " bars"
                             + (h.rx > 0 ? ", " + ratio(h.rx) + " the average" : "") + ".");
        if (h.v) L.push_back("Volume: the " + h.vTime + " bar was the biggest volume in " + std::to_string(h.vn) + (h.vn >= P.cap ? "+" : "") + " bars"
                             + (h.vx > 0 ? ", " + ratio(h.vx) + " the average" : "") + ".");
        if (h.c) L.push_back(std::string("Climax: the ") + h.cTime + (bull ? " red" : " green") + " bar was the biggest volume in " + std::to_string(h.cn) + (h.cn >= P.cap ? "+" : "")
                             + " bars" + (h.cx > 0 ? " (" + ratio(h.cx) + ")" : "") + " - the last " + (bull ? "sellers" : "buyers") + " - and the " + h.cConfTime
                             + " bar closed " + (bull ? "above its high" : "below its low") + ".");
        L.push_back(std::string("Abs on the pivot: ") + (h.abs ? "yes" : "no") + ".  Why it matters: a thrust right at the " + (bull ? "low" : "high")
                    + " shows " + (bull ? "buyers" : "sellers") + " stepping in hard where the leg began.");
        L.push_back(std::string(P.proven ? "Proven for this market." : "? = still testing: the study has not yet shown it adds edge over a plain Cob.")
                    + " Example: Cob+R? at the " + (bull ? "LOD" : "HOD") + " = a range thrust started the leg.");
    }
    L.insert(L.end(), G.begin(), G.end());
    return L;
}

// ---- the per-market file  lsFlexLevels\LRA-CobImpulse-<MKT>.csv  (KEY,VALUE lines; the nightly study / Rassul edits it)
//   VERSION,1   MARKET,NQ   MODE,abs|always|off   PROVEN,0|1   PLAIN,0|1   MIN_BARS,35 (10-100)   WINDOW,2 (0-5)
//   (0.3.0) GAMMA,0|1 (show the gamma part)   GAMMA_W,6 (3-10, hundredths of the ADR)   GAMMA_EM,100 (25-150, % of the expected move)
// Strict: VERSION first and 1, MARKET must match; any bad line -> the defaults + the reason (status file).
inline bool parseMode(const std::string& v, int& m)
{
    if (v == "off") { m = MODE_OFF; return true; }
    if (v == "abs") { m = MODE_ABS; return true; }
    if (v == "always") { m = MODE_ALWAYS; return true; }
    return false;
}
inline const char* modeName(int m) { return m == MODE_OFF ? "off" : m == MODE_ALWAYS ? "always" : "abs"; }
// returns the params to use; reason = "" (file used), "missing" (defaults) or why it was rejected (defaults)
inline Params parseFile(const std::string& text, const std::string& market, std::string& reason)
{
    Params D, P; reason.clear();
    if (text.size() > 64 * 1024) { reason = "file over 64 KB"; return D; }
    size_t a = 0; int n = 0; bool sawVersion = false, sawMarket = false;
    while (a <= text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        const std::string line = trim(text.substr(a, e - a)); a = e + 1;
        if (line.empty() || line[0] == '#') { if (e == text.size()) break; continue; }
        if (++n > 64) { reason = "too many lines"; return D; }
        const size_t c = line.find(',');
        if (c == std::string::npos) { reason = "bad line: " + line.substr(0, 40); return D; }
        const std::string k = trim(line.substr(0, c)), v = trim(line.substr(c + 1));
        if (!sawVersion) { if (k != "VERSION" || v != "1") { reason = "VERSION,1 must come first"; return D; } sawVersion = true; continue; }
        int iv = 0;
        if (k == "MARKET") { if (v != market) { reason = "MARKET " + v.substr(0, 8) + " is not " + market; return D; } sawMarket = true; }
        else if (k == "MODE") { if (!parseMode(v, P.mode)) { reason = "bad MODE"; return D; } }
        else if (k == "PROVEN") { if (!parseInt(v, 0, 1, iv)) { reason = "bad PROVEN"; return D; } P.proven = iv == 1; }
        else if (k == "PLAIN") { if (!parseInt(v, 0, 1, iv)) { reason = "bad PLAIN"; return D; } P.plain = iv == 1; }
        else if (k == "MIN_BARS") { if (!parseInt(v, 10, 100, iv)) { reason = "bad MIN_BARS"; return D; } P.minBars = iv; }
        else if (k == "WINDOW") { if (!parseInt(v, 0, 5, iv)) { reason = "bad WINDOW"; return D; } P.window = iv; }
        else if (k == "GAMMA") { if (!parseInt(v, 0, 1, iv)) { reason = "bad GAMMA"; return D; } P.gamma = iv == 1; }
        else if (k == "GAMMA_W") { if (!parseInt(v, 3, 10, iv)) { reason = "bad GAMMA_W"; return D; } P.gammaW = iv; }
        else if (k == "GAMMA_EM") { if (!parseInt(v, 25, 150, iv)) { reason = "bad GAMMA_EM"; return D; } P.gammaEM = iv; }
        else { reason = "unknown key " + k.substr(0, 16); return D; }
        if (e == text.size()) break;
    }
    if (!sawVersion) { reason = "missing"; return D; }
    if (!sawMarket) { reason = "MARKET line missing"; return D; }
    return P;
}

// ---- the record (one line per decided CoB; never rewritten)
//   trig_time|side|pivot_time|leg_end_time|r_time|r_n|r_x|v_time|v_n|v_x|abs|c_time|c_n|c_x|c_conf_time   (0.2.0 adds the 4 C fields;
//   a 0.1.0 line - 11 fields - still parses, with no C)
//   (0.3.0) + |g_code|g_kind|g_px|g_sign|g_dist_ticks|hedge|book_asof_sec|g_why  = 23 fields. g_code CR PS HVL CR0 PS0 HVL0 GW0 GM or '';
//   g_kind origin / cross / target or ''; g_sign 1 / -1 / 0 (unknown); hedge W (with the CoB) / A (against) / U (unknown) / '' (model);
//   book_asof_sec = the book's ASOF in CT seconds (civil days x 86400 + seconds of day), 0 = none; g_why '' / no book / wrong session /
//   late book / stale / no ADR / misaligned (pre 0.3.0 = an older line rewritten: no gamma part was decided). 11 / 15-field lines parse
//   with no G.
struct Rec {
    long long trig = 0; int side = 0; long long pivot = 0, legEnd = 0, rT = 0, vT = 0, cT = 0, cConf = 0; int rn = 0, vn = 0, cn = 0;
    double rx = 0, vx = 0, cx = 0; bool abs = false;
    std::string gCode, gKind, gWhy = "pre 0.3.0"; double gPx = 0; int gSign = 0; long long gDist = 0, gAsof = 0; char hedge = 0;
    bool r() const { return rT > 0; }
    bool v() const { return vT > 0; }
    bool c() const { return cT > 0; }
    bool gComputed() const { return gWhy != "pre 0.3.0"; }
};
static const char* const REC_HEADER = "trig_time|side|pivot_time|leg_end_time|r_time|r_n|r_x|v_time|v_n|v_x|abs|c_time|c_n|c_x|c_conf_time|g_code|g_kind|g_px|g_sign|g_dist_ticks|hedge|book_asof_sec|g_why";
static const char* const REC_HEADER_020 = "trig_time|side|pivot_time|leg_end_time|r_time|r_n|r_x|v_time|v_n|v_x|abs|c_time|c_n|c_x|c_conf_time";
static const char* const REC_HEADER_010 = "trig_time|side|pivot_time|leg_end_time|r_time|r_n|r_x|v_time|v_n|v_x|abs";
inline bool knownHeader(const std::string& h) { return h == REC_HEADER || h == REC_HEADER_020 || h == REC_HEADER_010; }
inline std::string g10(double x) { char b[40]; std::snprintf(b, sizeof(b), "%.10g", x); return b; }
inline std::string recLine(const Rec& r)
{
    char b[320];
    std::snprintf(b, sizeof(b), "%lld|%s|%lld|%lld|%lld|%d|%.2f|%lld|%d|%.2f|%d|%lld|%d|%.2f|%lld", r.trig, r.side > 0 ? "bull" : "bear", r.pivot, r.legEnd,
                  r.rT, r.rn, r.rx, r.vT, r.vn, r.vx, r.abs ? 1 : 0, r.cT, r.cn, r.cx, r.cConf);
    return std::string(b) + "|" + r.gCode + "|" + r.gKind + "|" + g10(r.gPx) + "|" + std::to_string(r.gSign) + "|" + std::to_string(r.gDist) + "|"
           + (r.hedge ? std::string(1, r.hedge) : std::string()) + "|" + std::to_string(r.gAsof) + "|" + r.gWhy;
}
inline bool parseLL(const std::string& s, long long& out)
{
    if (s.empty() || s.size() > 20) return false;
    errno = 0; char* e = nullptr; const long long v = std::strtoll(s.c_str(), &e, 10);
    if (errno || e != s.c_str() + s.size() || v < 0) return false;
    out = v; return true;
}
inline bool parseD(const std::string& s, double& out)
{
    if (s.empty() || s.size() > 24) return false;
    errno = 0; char* e = nullptr; const double v = std::strtod(s.c_str(), &e);
    if (errno || e != s.c_str() + s.size() || !fin(v) || v < 0) return false;
    out = v; return true;
}
inline std::vector<std::string> splitBar(const std::string& line)
{
    std::vector<std::string> f; size_t a = 0;
    for (size_t i = 0; i <= line.size(); ++i) if (i == line.size() || line[i] == '|') { f.push_back(line.substr(a, i - a)); a = i + 1; }
    return f;
}
inline bool parseRec(const std::string& line, Rec& out)
{
    if (line.empty() || line.size() > 480) return false;
    const std::vector<std::string> f = splitBar(line);
    if (f.size() != 11 && f.size() != 15 && f.size() != 23) return false;
    Rec r; long long n = 0;
    if (!parseLL(f[0], r.trig) || r.trig <= 0) return false;
    if (f[1] == "bull") r.side = 1; else if (f[1] == "bear") r.side = -1; else return false;
    if (!parseLL(f[2], r.pivot) || !parseLL(f[3], r.legEnd) || !parseLL(f[4], r.rT)) return false;
    if (!parseLL(f[5], n) || n > 1000) return false;
    r.rn = (int)n;
    if (!parseD(f[6], r.rx) || !parseLL(f[7], r.vT)) return false;
    if (!parseLL(f[8], n) || n > 1000) return false;
    r.vn = (int)n;
    if (!parseD(f[9], r.vx)) return false;
    if (f[10] != "0" && f[10] != "1") return false;
    r.abs = f[10] == "1";
    if (f.size() >= 15) {
        if (!parseLL(f[11], r.cT)) return false;
        if (!parseLL(f[12], n) || n > 1000) return false;
        r.cn = (int)n;
        if (!parseD(f[13], r.cx) || !parseLL(f[14], r.cConf)) return false;
    }
    if (f.size() == 23) {
        if (!(f[15].empty() || isGName(f[15]))) return false;
        if (!(f[16].empty() || f[16] == "origin" || f[16] == "cross" || f[16] == "target") || f[15].empty() != f[16].empty()) return false;
        if (!parseD(f[17], r.gPx)) return false;
        if (f[18] == "1") r.gSign = 1; else if (f[18] == "-1") r.gSign = -1; else if (f[18] == "0") r.gSign = 0; else return false;
        if (!parseLL(f[19], r.gDist) || !parseLL(f[21], r.gAsof)) return false;
        if (f[20].empty()) r.hedge = 0; else if (f[20] == "W" || f[20] == "A" || f[20] == "U") r.hedge = f[20][0]; else return false;
        if (!okWhy(f[22])) return false;
        r.gCode = f[15]; r.gKind = f[16]; r.gWhy = f[22];
    }
    out = r; return true;
}
inline Rec toRec(const Tag& T, const std::vector<Bar>& B, const GTag* g = nullptr)
{
    Rec r; r.trig = B[T.trig].t; r.side = T.side; r.abs = T.abs;
    if (T.pivot >= 0) r.pivot = B[T.pivot].t;
    if (T.legEnd >= 0) r.legEnd = B[T.legEnd].t;
    if (T.r.bar >= 0) { r.rT = B[T.r.bar].t; r.rn = T.r.n; r.rx = T.r.x; }
    if (T.v.bar >= 0) { r.vT = B[T.v.bar].t; r.vn = T.v.n; r.vx = T.v.x; }
    if (T.c.bar >= 0) { r.cT = B[T.c.bar].t; r.cn = T.c.n; r.cx = T.c.x; if (T.cConf >= 0) r.cConf = B[T.cConf].t; }
    if (g && g->computed) { r.gCode = g->code; r.gKind = g->kind; r.gPx = g->px; r.gSign = g->sign; r.gDist = g->distTicks; r.hedge = g->hedge; r.gAsof = g->asof; r.gWhy = g->why; }
    return r;
}

// ---- (0.3.0) the hover context sidecar  <stem>-<session>.gamma.csv  (one line per decided CoB that read a book; never rewritten):
//   trig_time|side|gamma_w|w|em|hvl|below|t_name|t_px|t_sign|t_r|t_em   (hvl / t_* empty when none; below 1 / 0 / -1)
static const char* const GCTX_HEADER = "trig_time|side|gamma_w|w|em|hvl|below|t_name|t_px|t_sign|t_r|t_em";
struct GCtx { long long trig = 0; int side = 0; int gw = 0; double w = 0, em = 0, hvl = 0, tPx = 0, tR = 0, tEm = 0; bool hasHvl = false, hasT = false; int below = -1, tSign = 0; std::string tName; };
inline GCtx toCtx(const GTag& g, long long trig, int side)
{
    GCtx c; c.trig = trig; c.side = side; c.gw = g.gw; c.w = g.w; c.em = g.em; c.hasHvl = g.hasHvl; c.hvl = g.hvl; c.below = g.below;
    c.hasT = g.hasT; c.tName = g.tName; c.tPx = g.tPx; c.tSign = g.tSign; c.tR = g.tR; c.tEm = g.tEm; return c;
}
inline std::string ctxLine(const GCtx& c)
{
    return std::to_string(c.trig) + "|" + (c.side > 0 ? "bull" : "bear") + "|" + std::to_string(c.gw) + "|" + g10(c.w) + "|" + g10(c.em) + "|"
           + (c.hasHvl ? g10(c.hvl) : std::string()) + "|" + std::to_string(c.below) + "|" + (c.hasT ? c.tName : std::string()) + "|"
           + (c.hasT ? g10(c.tPx) : std::string()) + "|" + (c.hasT ? std::to_string(c.tSign) : std::string()) + "|"
           + (c.hasT ? g10(c.tR) : std::string()) + "|" + (c.hasT ? g10(c.tEm) : std::string());
}
inline bool parseCtx(const std::string& line, GCtx& out)
{
    if (line.empty() || line.size() > 400) return false;
    const std::vector<std::string> f = splitBar(line);
    if (f.size() != 12) return false;
    GCtx c;
    if (!parseLL(f[0], c.trig) || c.trig <= 0) return false;
    if (f[1] == "bull") c.side = 1; else if (f[1] == "bear") c.side = -1; else return false;
    if (!parseInt(f[2], 0, 100, c.gw) || !parseD(f[3], c.w) || !parseD(f[4], c.em)) return false;
    c.hasHvl = !f[5].empty(); if (c.hasHvl && !parseD(f[5], c.hvl)) return false;
    if (f[6] == "1") c.below = 1; else if (f[6] == "0") c.below = 0; else if (f[6] == "-1") c.below = -1; else return false;
    c.hasT = !f[7].empty();
    if (c.hasT) {
        if (!isBig(f[7]) || !parseD(f[8], c.tPx) || !parseD(f[10], c.tR) || !parseD(f[11], c.tEm)) return false;
        if (f[9] == "1") c.tSign = 1; else if (f[9] == "-1") c.tSign = -1; else if (f[9] == "0") c.tSign = 0; else return false;
        c.tName = f[7];
    } else if (!f[8].empty() || !f[9].empty() || !f[10].empty() || !f[11].empty()) return false;
    out = c; return true;
}
// the gamma part as decided, rebuilt from the record (+ its sidecar line when present) for the label and the hover
inline GTag fromRec(const Rec& r, const GCtx* c)
{
    GTag g; g.computed = r.gComputed(); if (!g.computed) return g;
    g.why = r.gWhy; g.code = r.gCode; g.kind = r.gKind; g.px = r.gPx; g.sign = r.gSign; g.distTicks = r.gDist; g.hedge = r.hedge; g.asof = r.gAsof;
    if (c) { g.gw = c->gw; g.w = c->w; g.em = c->em; g.hasHvl = c->hasHvl; g.hvl = c->hvl; g.below = c->below; g.hasT = c->hasT; g.tName = c->tName;
             g.tPx = c->tPx; g.tSign = c->tSign; g.tR = c->tR; g.tEm = c->tEm; }
    return g;
}

// ---- placement: the label sits above the leg's high (bull) / below its low (bear), clear of IRT's two zigzag text rows, kept inside
// the pane; the box is the impulse bar's full range +-2 px.
struct Box { int l = 0, t = 0, r = 0, b = 0; };
inline Box labelBox(int x, int yExtreme, int side, int w, int h, int gap, int paneTop, int paneBottom)
{
    Box B; B.l = x - w / 2; B.r = B.l + w;
    if (side > 0) { B.b = yExtreme - gap; B.t = B.b - h; } else { B.t = yExtreme + gap; B.b = B.t + h; }
    if (B.t < paneTop + 1) { B.t = paneTop + 1; B.b = B.t + h; }
    if (B.b > paneBottom - 1) { B.b = paneBottom - 1; B.t = B.b - h; }
    return B;
}

} // namespace cimp
#endif
