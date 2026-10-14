#ifndef LS_COBIMPULSE_LOGIC_H
#define LS_COBIMPULSE_LOGIC_H
// CobImpulseLogic.h (0.2.0, 2026-10-14; 0.2.0 adds +C) -- SDK-free logic of lsCobImpulse (CobImpulse.cpp), unit-tested on Linux.
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

static const char* const VERSION = "0.2.0";
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
inline std::string label(bool r, bool v, bool abs, const Params& P, bool c = false)
{
    if (!shown(r || v || c, abs, P.mode)) return "Cob";
    return "Cob+" + code(r, v, c) + (P.proven ? "" : "?");
}

// 12-hour Central clock from hour / minute ("8:36 AM")
inline std::string clock12(int hh, int mm)
{
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59) return "?";
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", hh % 12 == 0 ? 12 : hh % 12, mm, hh < 12 ? "AM" : "PM"); return b;
}
inline std::string ratio(double x) { char b[16]; std::snprintf(b, sizeof(b), "%.1fx", x); return b; }

// the hover (question / how / why / use / example), one line per row; times already as 12-hour CT text
struct HoverIn { bool r = false, v = false, abs = false, c = false; int rn = 0, vn = 0, cn = 0; double rx = 0, vx = 0, cx = 0; std::string rTime, vTime, cTime, cConfTime; int side = 1; };
inline std::vector<std::string> hover(const HoverIn& h, const Params& P)
{
    std::vector<std::string> L;
    const bool bull = h.side > 0;
    const std::string lab = label(h.r, h.v, h.abs, P, h.c);
    if (!(h.r || h.v || h.c)) {
        L.push_back("Cob - no impulse at its start");
        L.push_back("Did the CoB leg start with force? No bar near its " + std::string(bull ? "low" : "high") + " was the biggest in 35+ bars, and no climax there.");
        return L;
    }
    L.push_back(lab + (shown(true, h.abs, P.mode) ? "" : "  (tag hidden by the Impulse tag setting)") + " - did the CoB leg start with force?");
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
    return L;
}

// ---- the per-market file  lsFlexLevels\LRA-CobImpulse-<MKT>.csv  (KEY,VALUE lines; the nightly study / Rassul edits it)
//   VERSION,1   MARKET,NQ   MODE,abs|always|off   PROVEN,0|1   PLAIN,0|1   MIN_BARS,35 (10-100)   WINDOW,2 (0-5)
// Strict: VERSION first and 1, MARKET must match; any bad line -> the defaults + the reason (status file).
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
struct Rec {
    long long trig = 0; int side = 0; long long pivot = 0, legEnd = 0, rT = 0, vT = 0, cT = 0, cConf = 0; int rn = 0, vn = 0, cn = 0;
    double rx = 0, vx = 0, cx = 0; bool abs = false;
    bool r() const { return rT > 0; }
    bool v() const { return vT > 0; }
    bool c() const { return cT > 0; }
};
static const char* const REC_HEADER = "trig_time|side|pivot_time|leg_end_time|r_time|r_n|r_x|v_time|v_n|v_x|abs|c_time|c_n|c_x|c_conf_time";
static const char* const REC_HEADER_010 = "trig_time|side|pivot_time|leg_end_time|r_time|r_n|r_x|v_time|v_n|v_x|abs";
inline std::string recLine(const Rec& r)
{
    char b[320];
    std::snprintf(b, sizeof(b), "%lld|%s|%lld|%lld|%lld|%d|%.2f|%lld|%d|%.2f|%d|%lld|%d|%.2f|%lld", r.trig, r.side > 0 ? "bull" : "bear", r.pivot, r.legEnd,
                  r.rT, r.rn, r.rx, r.vT, r.vn, r.vx, r.abs ? 1 : 0, r.cT, r.cn, r.cx, r.cConf);
    return b;
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
inline bool parseRec(const std::string& line, Rec& out)
{
    if (line.empty() || line.size() > 400) return false;
    std::vector<std::string> f; size_t a = 0;
    for (size_t i = 0; i <= line.size(); ++i) if (i == line.size() || line[i] == '|') { f.push_back(line.substr(a, i - a)); a = i + 1; }
    if (f.size() != 11 && f.size() != 15) return false;
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
    if (f.size() == 15) {
        if (!parseLL(f[11], r.cT)) return false;
        if (!parseLL(f[12], n) || n > 1000) return false;
        r.cn = (int)n;
        if (!parseD(f[13], r.cx) || !parseLL(f[14], r.cConf)) return false;
    }
    out = r; return true;
}
inline Rec toRec(const Tag& T, const std::vector<Bar>& B)
{
    Rec r; r.trig = B[T.trig].t; r.side = T.side; r.abs = T.abs;
    if (T.pivot >= 0) r.pivot = B[T.pivot].t;
    if (T.legEnd >= 0) r.legEnd = B[T.legEnd].t;
    if (T.r.bar >= 0) { r.rT = B[T.r.bar].t; r.rn = T.r.n; r.rx = T.r.x; }
    if (T.v.bar >= 0) { r.vT = B[T.v.bar].t; r.vn = T.v.n; r.vx = T.v.x; }
    if (T.c.bar >= 0) { r.cT = B[T.c.bar].t; r.cn = T.c.n; r.cx = T.c.x; if (T.cConf >= 0) r.cConf = B[T.cConf].t; }
    return r;
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
