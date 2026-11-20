// GammaLine.h -- lsHodLod 2.39.0 (GammaLine 1.0.0): the GAMMA line between the Now line and the HOD / LOD box, SDK-free
//
// (2.39.0, Rassul 2026-11-20 07:45 CT "build", mockups gamma-line-mockup.html + hodlod-new-box-mockup.html):
//   top to bottom: the Now line (unchanged) -> the GAMMA line -> the HOD / LOD box; both lines centred over the box. The 2.37.x regime pill
//   ("-GAMMA \xB7 Trending \xB7 Bk / BkTst" red / "+GAMMA \xB7 Range \xB7 Sw / Cob" green) moves from the Now row to the START of the gamma line;
//   "0DTE Exp 3:00 PM" moves to the far left of the gamma line's row.
//   GAMMA line = regime pill | state badge | G V C | Next
//   - significant strikes: MenthorQ's intraday book (lsFlexLevels\GammaProfile-MQ-<MKT>.csv STRIKE rows), |% of the biggest| >= 50; the
//     nearest below / above the last closed close are WATCHED. Name = the MenthorQ level code the strike carries (PS, CR, HVL, GW, CW, PW -
//     from the book's type column, its CW / PW rows and the LRA-GexNodes-<MKT>.csv STR names), else the plain price: "PS 7,100" / "7,150".
//     Drawn exactly at MenthorQ's strike (never shifted).
//   - sign at the strike: negative = dealers SHORT gamma there = they chase = expect "Break"; positive = LONG = they lean against = "Sweep".
//   - states (as the Now line): WATCH gold "GAMMA WATCH: <below> \x{2193} \xB7 <above> \x{2191} \xB7 Break|Sweep" (the word of the nearer strike);
//     TESTING gold (the forming candle trades at / through a watched strike) "GAMMA TESTING: <name> \xB7 Break|Sweep"; at the candle CLOSE:
//     BREAK (closes through AND the gamma pressure points the same way) / SWEEP (traded through, closed back AND the pressure points the
//     other way), red / green by side, shown while that candle is the last closed one; close and pressure disagree = back to WATCH.
//     Each closed candle is decided ONCE and recorded (session file); never re-decided (no repaint).
//   - G = the direction dealers must trade from gamma right now (short gamma: with the move, long gamma: against it). GAUGE (no absolute
//     dealer contracts exist natively - the book holds % of the biggest strike only): g(p) = sum of the book's signed % / 100 weighted by a
//     Gaussian of the distance (sigma = half the median strike spacing); flow = -g x the move (points) of the candle (the forming candle:
//     live - last close; a decision: close - prior close for a break, the extreme - prior close for a sweep); its NORMAL = the mean |flow|
//     of the 20 closed candles before it (>= 5 needed). Arrow when |g| >= 0.10 (10% of the biggest strike's gamma at price) and the flow is
//     >= 10% of its normal, else a grey dash; "1.4x" (flow / normal) when >= 1.2x.
//   - V / C: "\x96" until the MenthorQ integration writes lsFlexLevels\OptFlow-<MKT>.csv (hook below); read when present and <= 10 min old.
//   - Next: the next significant strikes beyond the watched ones (below first); TESTING / BREAK: beyond the strike that way; SWEEP: the next
//     one in the sweep direction.
//   Narrow pane (layout): the 0DTE text drops first, then Next, then G V C, then the pill's setup words, then its regime word; the state badge
//   always stays (no badge room = no line).
#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <algorithm>
#include "NowLevels.h"

namespace gml {

static const char* const VERSION = "1.0.0";
static const int SIG_PCT = 50;               // significant = |% of the biggest| >= 50
static const double G_MIN = 0.10;            // gamma at price >= 10% of the biggest strike's
static const double FLOW_MIN = 0.10;         // flow >= 10% of its own 20-candle normal
static const double RATIO_SHOW = 1.2;        // "1.4x" shown from 1.2x
static const int NORM_N = 20, NORM_MIN = 5;
static const size_t MAX_BYTES = 64 * 1024, REC_MAX = 1024 * 1024, FLOW_MAX = 8 * 1024;
static const long long FLOW_STALE_S = 600, FUTURE_S = 120;
static const unsigned C_GREY = nowl::C_GREY, C_GOLD = nowl::C_GOLD, C_GREEN = nowl::C_GREEN, C_RED = nowl::C_RED, C_BLACK = nowl::C_BLACK,
                      C_SEP = nowl::C_SEP, C_BG = nowl::C_BG, C_BORDER = nowl::C_BORDER;
enum { WATCH = 0, TESTING = 1, BREAK = 2, SWEEP = 3 };
inline const char* stateWord(int s) { return s == TESTING ? "TESTING" : s == BREAK ? "BREAK" : s == SWEEP ? "SWEEP" : "WATCH"; }

using nowl::Bar;
inline bool fullD(const std::string& t, double& v) { if (t.empty() || t.size() > 32) return false; errno = 0; char* e = nullptr; const double x = std::strtod(t.c_str(), &e); if (errno || !e || *e || !std::isfinite(x)) return false; v = x; return true; }
inline bool fullL(const std::string& t, long long& v) { if (t.empty() || t.size() > 20) return false; errno = 0; char* e = nullptr; const long long x = std::strtoll(t.c_str(), &e, 10); if (errno || !e || *e) return false; v = x; return true; }
inline std::vector<std::string> split(const std::string& s, char d) { std::vector<std::string> o; std::string c; for (char ch : s) { if (ch == d) { o.push_back(c); c.clear(); } else if (ch != '\r') c += ch; } o.push_back(c); return o; }
inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
// civil date <-> days since 1970-01-01 (H. Hinnant)
inline long long daysFromCivil(long long y, unsigned m, unsigned d) { y -= m <= 2; const long long era = floorDiv(y, 400); const unsigned yoe = (unsigned)(y - era * 400); const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; return era * 146097 + (long long)doe - 719468; }
inline std::string ymd(long long z) { z += 719468; const long long era = floorDiv(z, 146097); const unsigned doe = (unsigned)(z - era * 146097); const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long long y = (long long)yoe + era * 400; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); const unsigned mp = (5 * doy + 2) / 153; const unsigned d = doy - (153 * mp + 2) / 5 + 1; const unsigned m = mp < 10 ? mp + 3 : mp - 9; y += m <= 2;
    char b[64]; std::snprintf(b, sizeof b, "%04lld-%02u-%02u", y, m, d); return b; }
inline bool parseYmd(const std::string& s, long long& day) { if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false; long long y, m, d; if (!fullL(s.substr(0, 4), y) || !fullL(s.substr(5, 2), m) || !fullL(s.substr(8, 2), d)) return false;
    if (y < 2000 || y > 2100 || m < 1 || m > 12 || d < 1 || d > 31) { return false; }
    day = daysFromCivil(y, (unsigned)m, (unsigned)d); return true; }
// the trading session a candle END belongs to (sessions start 17:00 CT): its date
inline std::string sessionYmd(long long tEnd) { return ymd(floorDiv(tEnd - 1 - 17 * 3600, 86400) + 1); }

// ---------------------------------------------------------------------------------------------------------------------------------------
// the MenthorQ intraday book: GammaProfile-MQ-<MKT>.csv
//   MARKET,<mkt>  SESSION,<date>  STRIKE,<price>,<pct signed>,<rank>,<isKing>,<type>,<strike>,,<first-seen>  KING / CW / PW / FLIP,<price>  ASOF,<CT s>,<date>
struct Strike { double px = 0; int pct = 0; int rank = 0; bool king = false; std::string type; };
struct Book { bool ok = false; std::string why = "no file", session; std::vector<Strike> s; double king = NAN, cw = NAN, pw = NAN, flip = NAN; long long asofCt = 0, arrivedCt = 0; int bad = 0; };
inline Book parseBook(const std::string& text, const std::string& market) {
    Book B; if (text.empty()) return B; if (text.size() > MAX_BYTES) { B.why = "over 64 KiB"; return B; }
    bool mk = false; long long sessDay = -1, asofSec = -1, asofDay = -1; size_t a = 0; int lines = 0;
    while (a < text.size() && lines++ < 400) {
        size_t e = text.find('\n', a); std::string ln = text.substr(a, e == std::string::npos ? std::string::npos : e - a); a = e == std::string::npos ? text.size() : e + 1;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln.empty() || ln[0] == '#' || ln.size() > 300) continue;
        const std::vector<std::string> c = split(ln, ',');
        double v = 0; long long x = 0;
        if (c[0] == "MARKET") { if (c.size() < 2 || c[1] != market) { B.why = "book is for another market"; return B; } mk = true; }
        else if (c[0] == "SESSION" && c.size() >= 2) { if (parseYmd(c[1], sessDay)) B.session = c[1]; }
        else if ((c[0] == "KING" || c[0] == "CW" || c[0] == "PW" || c[0] == "FLIP") && c.size() >= 2) { if (fullD(c[1], v) && v > 0) (c[0] == "KING" ? B.king : c[0] == "CW" ? B.cw : c[0] == "PW" ? B.pw : B.flip) = v; }
        else if (c[0] == "ASOF" && c.size() >= 2) { if (fullL(c[1], x) && x >= 0 && x < 86400) asofSec = x; if (c.size() >= 3) parseYmd(c[2], asofDay); }
        else if (c[0] == "STRIKE") {
            Strike k; long long p = 0, r = 0, kg = 0;
            const bool ok = c.size() >= 7 && fullD(c[1], k.px) && k.px > 0 && fullL(c[2], p) && p >= -100 && p <= 100 && fullL(c[3], r) && r >= 1 && r <= 999
                         && fullL(c[4], kg) && (kg == 0 || kg == 1) && c[5].size() <= 24;
            if (!ok || B.s.size() >= 200) { B.bad++; continue; }
            k.pct = (int)p; k.rank = (int)r; k.king = kg == 1; k.type = c[5];
            bool dup = false; for (const Strike& o : B.s) if (std::fabs(o.px - k.px) < 1e-9) dup = true;
            if (dup) { B.bad++; continue; }
            B.s.push_back(k);
        }
    }
    if (!mk) { B.why = "no MARKET row"; return B; }
    if (asofSec < 0) { B.why = "no ASOF row"; return B; }
    if (asofDay < 0 && sessDay >= 0) asofDay = asofSec >= 17 * 3600 ? sessDay - 1 : sessDay;
    if (asofDay < 0) { B.why = "no ASOF / SESSION date"; return B; }
    B.asofCt = asofDay * 86400 + asofSec;
    if (B.s.empty()) { B.why = "no STRIKE rows"; return B; }
    std::sort(B.s.begin(), B.s.end(), [](const Strike& p, const Strike& q) { return p.px < q.px; });
    B.ok = true; B.why = B.bad ? std::to_string(B.bad) + " bad rows skipped" : std::string(); return B;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// names: MenthorQ level codes only (PS, CR, HVL, GW, CW, PW), most important first; G1 .. / KING SLICE are not names (the plain price shows)
struct NameAt { double px = 0; std::string raw; };   // a LRA-GexNodes STR row: chart price + MenthorQ name ("PS/PS0", "CR0+GW0", "G1")
inline int codeRank(const std::string& c) { static const char* const R[] = { "PS", "CR", "HVL", "GW", "CW", "PW" }; for (int i = 0; i < 6; i++) if (c == R[i]) return i; return -1; }
inline std::string bestCode(const std::string& raw) {   // "PS/PS0" -> PS, "CR0+GW0" -> CR, "HVL0" -> HVL, "G1" / "KING SLICE" -> ""
    std::string best, tok; int br = 99;
    auto take = [&]() { std::string b = tok; while (!b.empty() && b.back() >= '0' && b.back() <= '9') b.pop_back(); const int r = codeRank(b); if (r >= 0 && r < br) { br = r; best = b; } tok.clear(); };
    for (char ch : raw) { if (ch == '/' || ch == '+' || ch == ' ' || ch == '|' || ch == ';') take(); else tok += ch; }
    take(); return best;
}
inline std::string fmtPx(double v, int dec) {   // 7100 -> "7,100", 58.5 -> "58.5", 1.165 -> "1.165"
    dec = (std::max)(0, (std::min)(6, dec));
    char b[48]; std::snprintf(b, sizeof b, "%.*f", dec, v); std::string s = b, ip = s, fp; const size_t d = s.find('.');
    if (d != std::string::npos) { ip = s.substr(0, d); fp = s.substr(d); while (!fp.empty() && fp.back() == '0') fp.pop_back(); if (fp == ".") fp.clear(); }
    std::string neg; if (!ip.empty() && ip[0] == '-') { neg = "-"; ip = ip.substr(1); }
    std::string o; for (size_t i = 0; i < ip.size(); i++) { if (i && (ip.size() - i) % 3 == 0) o += ','; o += ip[i]; }
    return neg + o + fp;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
struct Ctx { Book b; std::vector<NameAt> names; double tick = 0.25, eps = 0.125, sigma = 25; int dec = 2; std::vector<int> sig; };
inline Ctx makeCtx(const Book& b, const std::vector<NameAt>& names, double tick, int dec) {
    Ctx c; c.b = b; c.names = names; c.tick = tick > 0 && std::isfinite(tick) ? tick : 0.25; c.eps = c.tick / 2; c.dec = dec;
    std::vector<double> gaps; for (size_t i = 1; i < b.s.size(); i++) gaps.push_back(b.s[i].px - b.s[i - 1].px);
    double med = 0; if (!gaps.empty()) { std::sort(gaps.begin(), gaps.end()); med = gaps[gaps.size() / 2]; }
    c.sigma = (std::max)(med / 2, 4 * c.tick);
    for (size_t i = 0; i < b.s.size(); i++) if (std::abs(b.s[i].pct) >= SIG_PCT) c.sig.push_back((int)i);
    return c;
}
inline std::string nameOf(const Ctx& c, double px) {
    std::string best; int br = 99;
    auto cand = [&](const std::string& code) { const int r = codeRank(code); if (r >= 0 && r < br) { br = r; best = code; } };
    for (const Strike& k : c.b.s) if (std::fabs(k.px - px) <= c.eps) cand(bestCode(k.type));
    for (const NameAt& n : c.names) if (std::fabs(n.px - px) <= c.eps) cand(bestCode(n.raw));
    if (std::isfinite(c.b.cw) && std::fabs(c.b.cw - px) <= c.eps) cand("CW");
    if (std::isfinite(c.b.pw) && std::fabs(c.b.pw - px) <= c.eps) cand("PW");
    return best;
}
struct Lv { bool ok = false; double px = NAN; int pct = 0; std::string name, label; int expect = 0; };   // expect -1 Break (short gamma), +1 Sweep (long)
inline Lv lvAt(const Ctx& c, int i) { Lv L; const Strike& k = c.b.s[(size_t)i]; L.ok = true; L.px = k.px; L.pct = k.pct; L.name = nameOf(c, k.px); L.label = (L.name.empty() ? "" : L.name + " ") + fmtPx(k.px, c.dec); L.expect = k.pct < 0 ? -1 : 1; return L; }
inline Lv lvFor(const Ctx& c, double px) { for (int i : c.sig) if (std::fabs(c.b.s[(size_t)i].px - px) <= c.eps) return lvAt(c, i); Lv L; L.ok = true; L.px = px; L.name = nameOf(c, px); L.label = (L.name.empty() ? "" : L.name + " ") + fmtPx(px, c.dec); return L; }
inline const char* expectWord(int e) { return e < 0 ? "Break" : "Sweep"; }
// the watched strikes: the nearest significant at / below ref, and above it
inline void watch(const Ctx& c, double ref, Lv& dn, Lv& up) {
    dn = Lv(); up = Lv(); int d = -1, u = -1;
    for (int i : c.sig) { const double p = c.b.s[(size_t)i].px; if (p <= ref + c.eps) d = i; else if (u < 0) u = i; }
    if (d >= 0) { dn = lvAt(c, d); }
    if (u >= 0) { up = lvAt(c, u); }
}
inline Lv nextBeyond(const Ctx& c, double px, int dir) {
    int best = -1; for (int i : c.sig) { const double p = c.b.s[(size_t)i].px; if (dir < 0 ? p < px - c.eps : p > px + c.eps) { if (best < 0 || (dir < 0 ? p > c.b.s[(size_t)best].px : p < c.b.s[(size_t)best].px)) best = i; } }
    return best >= 0 ? lvAt(c, best) : Lv();
}
// the strike-weighted gamma at a price, in units of the biggest strike (sign: + long, - short)
inline double gammaAt(const Ctx& c, double p) { double g = 0; for (const Strike& k : c.b.s) { const double z = (p - k.px) / c.sigma; if (std::fabs(z) < 6) g += k.pct / 100.0 * std::exp(-0.5 * z * z); } return g; }
// the normal |flow| of the closed candles [k - 20, k - 1] (each: -g(mid) x close-to-close move); NAN when fewer than NORM_MIN
inline double normalFlow(const Ctx& c, const std::vector<Bar>& B, int k) {
    double s = 0; int n = 0;
    for (int i = (std::max)(1, k - NORM_N); i < k && i < (int)B.size(); i++) { const double mv = B[(size_t)i].c - B[(size_t)i - 1].c; s += std::fabs(gammaAt(c, (B[(size_t)i].c + B[(size_t)i - 1].c) / 2) * mv); n++; }
    return n >= NORM_MIN ? s / n : NAN;
}
struct Press { int dir = 0; double g = 0, flow = 0, normal = NAN, ratio = NAN; };   // dir +1 dealers buy, -1 sell, 0 under the floor
inline Press press(double g, double move, double normal) {
    Press P; P.g = g; P.flow = -g * move; P.normal = normal;
    if (std::isfinite(normal) && normal > 0) P.ratio = std::fabs(P.flow) / normal;
    const bool big = std::fabs(g) >= G_MIN && std::fabs(P.flow) > 0 && (!std::isfinite(P.ratio) || P.ratio >= FLOW_MIN);
    if (big) P.dir = P.flow > 0 ? 1 : -1;
    return P;
}
inline std::string ratioTxt(double r) { if (!std::isfinite(r) || r < RATIO_SHOW) return std::string(); char b[24]; std::snprintf(b, sizeof b, "%.1fx", r > 99.9 ? 99.9 : r); return b; }

// ---------------------------------------------------------------------------------------------------------------------------------------
// one closed candle, decided once (B[k] closed, k >= 1): WATCH (not at a watched strike), TESTING (at / through, no BREAK / SWEEP), BREAK, SWEEP
struct Ev { long long t = 0; int state = WATCH; int dir = 0; double px = NAN; std::string name, label; int expect = 0; int pdir = 0; double ratio = NAN, g = 0, close = NAN; long long asof = 0; };
inline Ev evalSide(const Ctx& c, const std::vector<Bar>& B, int k, const Lv& L, int side, double normal) {   // side -1: the strike is below the prior close
    const Bar& b = B[(size_t)k]; const double ref = B[(size_t)k - 1].c, K = L.px, g = gammaAt(c, K);
    Ev e; e.t = b.t; e.state = TESTING; e.px = K; e.name = L.name; e.label = L.label; e.expect = L.expect; e.close = b.c; e.g = g;
    const bool through = side < 0 ? b.c < K - c.eps : b.c > K + c.eps;
    const bool traded = side < 0 ? b.l < K - c.eps : b.h > K + c.eps;
    Press P;
    if (through) { P = press(g, b.c - ref, normal); if (P.dir == side) { e.state = BREAK; e.dir = side; } }
    else if (traded) { P = press(g, (side < 0 ? b.l : b.h) - ref, normal); if (P.dir == -side) { e.state = SWEEP; e.dir = -side; } }
    else P = press(g, b.c - ref, normal);
    e.pdir = P.dir; e.ratio = P.ratio; return e;
}
inline Ev decide(const Ctx& c, const std::vector<Bar>& B, int k) {
    Ev e; if (k < 1 || k >= (int)B.size()) return e;
    const Bar& b = B[(size_t)k]; e.t = b.t; e.close = b.c; e.asof = c.b.asofCt;
    Lv dn, up; watch(c, B[(size_t)k - 1].c, dn, up);
    const double normal = normalFlow(c, B, k);
    const bool td = dn.ok && b.l <= dn.px + c.eps, tu = up.ok && b.h >= up.px - c.eps;
    std::vector<Ev> v; if (td) v.push_back(evalSide(c, B, k, dn, -1, normal)); if (tu) v.push_back(evalSide(c, B, k, up, 1, normal));
    if (v.empty()) { const Press P = press(gammaAt(c, b.c), b.c - B[(size_t)k - 1].c, normal); e.pdir = P.dir; e.ratio = P.ratio; e.g = P.g; return e; }
    size_t pick = 0;
    for (size_t i = 1; i < v.size(); i++) { const bool di = v[i].state >= BREAK, dp = v[pick].state >= BREAK;
        if ((di && !dp) || (di == dp && std::fabs(v[i].px - b.c) < std::fabs(v[pick].px - b.c))) pick = i; }
    v[pick].asof = c.b.asofCt; return v[pick];
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the session record (append-only, one line per decided closed candle; the nightly scores TESTING / BREAK / SWEEP, WATCH = decided, nothing)
//   lsFlexLevels\HodLod\GammaLine-<MKT>-<spb>s-<session date>.csv
inline std::string recHeader() { return "# lsHodLod gamma line, one row per closed candle decided once: ct_end_s,clock,market,spb,state,dir,strike,name,expect,pressure_dir,pressure_ratio,gamma_at_strike,close,book_asof_ct_s,version"; }
inline std::string recPath(const std::string& mkt, long long spb, long long tEnd) { return "GammaLine-" + mkt + "-" + std::to_string(spb) + "s-" + sessionYmd(tEnd) + ".csv"; }
inline std::string recLine(const Ev& e, const std::string& mkt, long long spb, int dec, const char* ver) {
    char g[32]; std::snprintf(g, sizeof g, "%.3f", e.g); char r[24]; if (std::isfinite(e.ratio)) std::snprintf(r, sizeof r, "%.2f", e.ratio); else std::snprintf(r, sizeof r, "-");
    char px[48]; if (std::isfinite(e.px)) std::snprintf(px, sizeof px, "%.*f", dec, e.px); else std::snprintf(px, sizeof px, "-");
    char cl[48]; std::snprintf(cl, sizeof cl, "%.*f", dec, std::isfinite(e.close) ? e.close : 0.0);
    return std::to_string(e.t) + "," + nowl::clock12(e.t) + "," + mkt + "," + std::to_string(spb) + "," + stateWord(e.state) + "," + std::to_string(e.dir) + "," + px + ","
         + (e.name.empty() ? std::string("-") : e.name) + "," + (e.state == WATCH ? "-" : expectWord(e.expect)) + "," + std::to_string(e.pdir) + "," + r + "," + g + "," + cl + "," + std::to_string(e.asof) + "," + ver + "\n";
}
// the rows of this market / bar size (bad rows skipped); the first row for a candle wins (another chart may have appended the same candle)
inline std::vector<Ev> parseRec(const std::string& text, const std::string& mkt, long long spb, int dec) {
    std::vector<Ev> o; if (text.size() > REC_MAX) return o; size_t a = 0;
    while (a < text.size()) {
        size_t e = text.find('\n', a); std::string ln = text.substr(a, e == std::string::npos ? std::string::npos : e - a); a = e == std::string::npos ? text.size() : e + 1;
        if (ln.empty() || ln[0] == '#' || ln.size() > 400 || e == std::string::npos) continue;   // a torn last line (no newline yet) is skipped
        const std::vector<std::string> c = split(ln, ',');
        if (c.size() < 15 || c[2] != mkt || c[3] != std::to_string(spb)) continue;
        Ev v; long long t = 0, d = 0, pd = 0, as = 0; double r = NAN, g = 0;
        if (!fullL(c[0], t) || !fullL(c[5], d) || d < -1 || d > 1 || !fullL(c[9], pd) || pd < -1 || pd > 1 || !fullD(c[11], g) || !fullL(c[13], as)) continue;
        int st = -1; for (int s = 0; s < 4; s++) if (c[4] == stateWord(s)) st = s; if (st < 0) continue;
        if (c[10] != "-" && !fullD(c[10], r)) continue;
        double px = NAN; if (c[6] != "-" && !fullD(c[6], px)) continue;
        bool dup = false; for (const Ev& x : o) if (x.t == t) dup = true; if (dup) continue;
        v.t = t; v.state = st; v.dir = (int)d; v.px = px; v.name = c[7] == "-" ? "" : c[7]; v.expect = c[8] == "Break" ? -1 : c[8] == "Sweep" ? 1 : 0; v.pdir = (int)pd; v.ratio = r; v.g = g; v.asof = as;
        double cl = NAN; if (fullD(c[12], cl)) v.close = cl;
        v.label = (v.name.empty() ? "" : v.name + " ") + (std::isfinite(px) ? fmtPx(px, dec) : std::string());
        o.push_back(v);
    }
    return o;
}

// the last closed candle's decision, once: found in the record (another run / chart / before a restart decided it) or decided now (append
// the line; the host marks it decided only after a good append). B = closed candles + the forming one last.
struct StepR { bool have = false; Ev ev; bool append = false; std::string line; };
inline StepR closedStep(const Ctx& c, const std::vector<Bar>& B, const std::string& recText, const std::string& mkt, long long spb, int dec, const char* ver) {
    StepR r; const int k = (int)B.size() - 2; if (k < 1) return r;
    const long long t = B[(size_t)k].t;
    for (const Ev& e : parseRec(recText, mkt, spb, dec)) if (e.t == t) { r.have = true; r.ev = e; return r; }
    r.have = true; r.ev = decide(c, B, k); r.append = true; r.line = recLine(r.ev, mkt, spb, dec, ver); return r;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// V / C hook for the MenthorQ integration (vanna / charm): lsFlexLevels\OptFlow-<MKT>.csv
//   OPTFLOW,1   MARKET|<mkt>   STAMP|<CT wall s>   V|<-1 / 0 / 1>|<ratio or ->   C|<-1 / 0 / 1>|<ratio or ->
struct Flow { bool ok = false; std::string why = "no file"; int v = 0, c = 0; double vr = NAN, cr = NAN; };
inline Flow parseFlow(const std::string& text, const std::string& mkt, long long nowCt) {
    Flow F; if (text.empty()) return F; if (text.size() > FLOW_MAX) { F.why = "too big"; return F; }
    const std::vector<std::string> L = split(text, '\n'); if (L.empty() || L[0].compare(0, 9, "OPTFLOW,1") != 0) { F.why = "bad header"; return F; }
    bool mk = false; long long st = -1;
    for (size_t i = 1; i < L.size(); i++) { const std::vector<std::string> c = split(L[i], '|'); if (c.size() < 2) continue;
        if (c[0] == "MARKET") { if (c[1] != mkt) { F.why = "other market"; return F; } mk = true; }
        else if (c[0] == "STAMP") { long long x; if (fullL(c[1], x)) st = x; }
        else if ((c[0] == "V" || c[0] == "C") && c.size() >= 3) { long long d; double r = NAN; if (!fullL(c[1], d) || d < -1 || d > 1) continue; if (c[2] != "-" && (!fullD(c[2], r) || r < 0)) continue;
            if (c[0] == "V") { F.v = (int)d; F.vr = r; } else { F.c = (int)d; F.cr = r; } } }
    if (!mk) { F.why = "no MARKET"; F.v = F.c = 0; return F; }
    if (st < 0 || st > nowCt + FUTURE_S || nowCt - st > FLOW_STALE_S) { F.why = "stale"; F.v = F.c = 0; F.vr = F.cr = NAN; return F; }
    F.ok = true; F.why = "ok"; return F;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the live line: B = this session's closed candles + the forming one LAST (its h / l / c already widened to the live price)
struct Out { bool ok = false; std::string why; int state = WATCH; int dir = 0; Lv lv, dn, up, nDn, nUp; Press g; Flow f; };
inline Out live(const Ctx& c, const std::vector<Bar>& B, double livePx, const Ev* lastClosed) {
    Out o; const int n = (int)B.size();
    if (n < 2) { o.why = "too few candles"; return o; }
    if (c.sig.empty()) { o.why = "no significant strike"; return o; }
    const double px = std::isfinite(livePx) && livePx > 0 ? livePx : B.back().c, ref = B[(size_t)n - 2].c;
    o.g = press(gammaAt(c, px), px - ref, normalFlow(c, B, n - 1));
    if (lastClosed && lastClosed->t == B[(size_t)n - 2].t && (lastClosed->state == BREAK || lastClosed->state == SWEEP) && std::isfinite(lastClosed->px)) {
        o.ok = true; o.state = lastClosed->state; o.dir = lastClosed->dir; o.lv = lvFor(c, lastClosed->px);
        if (o.lv.name.empty() && !lastClosed->name.empty()) { o.lv.name = lastClosed->name; o.lv.label = lastClosed->label; }
        o.g = Press(); o.g.dir = lastClosed->pdir; o.g.ratio = lastClosed->ratio; o.g.g = lastClosed->g;   // the decided candle's pressure (no repaint)
        (o.dir < 0 ? o.nDn : o.nUp) = nextBeyond(c, lastClosed->px, o.dir);
        return o;
    }
    watch(c, ref, o.dn, o.up);
    if (!o.dn.ok && !o.up.ok) { o.why = "no watched strike"; return o; }
    o.ok = true;
    const Bar& f = B.back();
    const bool td = o.dn.ok && f.l <= o.dn.px + c.eps, tu = o.up.ok && f.h >= o.up.px - c.eps;
    if (td || tu) {
        const bool pickU = tu && (!td || std::fabs(px - o.up.px) <= std::fabs(px - o.dn.px));
        o.state = TESTING; o.lv = pickU ? o.up : o.dn; const int side = pickU ? 1 : -1;
        (side < 0 ? o.nDn : o.nUp) = nextBeyond(c, o.lv.px, side);
        return o;
    }
    if (o.dn.ok) o.nDn = nextBeyond(c, o.dn.px, -1);
    if (o.up.ok) o.nUp = nextBeyond(c, o.up.px, 1);
    return o;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// segments. part: 0 the pill, 1 the state badge, 2 G V C, 3 Next. Consecutive badge pieces = ONE filled badge (black bold text).
struct Seg { std::string s, s2; int arrow = 0; unsigned col = C_GREY, acol = C_GREY; bool bold = false, badge = false, sep = false, pill = false; unsigned fill = 0; int part = 0; };
inline std::vector<Seg> badgeSegs(const Out& o) {
    std::vector<Seg> v; if (!o.ok) return v;
    const std::string dot = "\xB7";
    auto bd = [&](const std::string& s, int arrow, unsigned fill) { Seg g; g.s = s; g.arrow = arrow; g.col = g.acol = C_BLACK; g.bold = true; g.badge = true; g.fill = fill; g.part = 1; v.push_back(g); };
    if (o.state == BREAK || o.state == SWEEP) { bd(std::string(o.state == BREAK ? "GAMMA BREAK: " : "GAMMA SWEEP: ") + o.lv.label, o.dir, o.dir < 0 ? C_RED : C_GREEN); return v; }
    if (o.state == TESTING) { bd("GAMMA TESTING: " + o.lv.label + " " + dot + " " + expectWord(o.lv.expect), 0, C_GOLD); return v; }
    if (o.dn.ok) bd("GAMMA WATCH: " + o.dn.label, -1, C_GOLD);
    if (o.up.ok) bd((o.dn.ok ? dot + " " : std::string("GAMMA WATCH: ")) + o.up.label, 1, C_GOLD);
    return v;
}
// the WATCH word = the nearer watched strike's expectation (needs the live price)
inline void addWatchWord(std::vector<Seg>& v, const Out& o, double px) {
    if (!o.ok || o.state != WATCH) return;
    const Lv& n = !o.up.ok ? o.dn : !o.dn.ok ? o.up : (std::fabs(px - o.up.px) < std::fabs(px - o.dn.px) ? o.up : o.dn);
    Seg g; g.s = std::string("\xB7 ") + expectWord(n.expect); g.col = g.acol = C_BLACK; g.bold = true; g.badge = true; g.fill = C_GOLD; g.part = 1; v.push_back(g);
}
inline std::vector<Seg> gvcSegs(const Out& o) {
    std::vector<Seg> v;
    auto one = [&](const char* L, int dir, double ratio) { Seg g; g.s = L; g.col = C_GREY; g.part = 2;
        if (dir) { g.arrow = dir; g.acol = dir > 0 ? C_GREEN : C_RED; g.s2 = ratioTxt(ratio); g.bold = true; } else { g.s2 = "\x96"; g.acol = C_GREY; } v.push_back(g); };
    one("G", o.g.dir, o.g.ratio);
    one("V", o.f.ok ? o.f.v : 0, o.f.vr);
    one("C", o.f.ok ? o.f.c : 0, o.f.cr);
    return v;
}
inline std::vector<Seg> nextSegs(const Out& o) {
    std::vector<Seg> v; if (!o.ok || (!o.nDn.ok && !o.nUp.ok)) return v;
    Seg h; h.s = "Next"; h.col = C_GREY; h.part = 3; v.push_back(h);
    auto lv = [&](int a, const std::string& s) { Seg g; g.arrow = a; g.s2 = s; g.col = g.acol = C_GOLD; g.bold = true; g.part = 3; v.push_back(g); };
    if (o.nDn.ok) lv(-1, o.nDn.label);
    if (o.nDn.ok && o.nUp.ok) { Seg d; d.s = "\xB7"; d.col = C_GREY; d.part = 3; v.push_back(d); }
    if (o.nUp.ok) lv(1, o.nUp.label);
    return v;
}
inline Seg pillSeg(const std::string& s, unsigned col, unsigned fill) { Seg g; g.s = s; g.col = g.acol = col; g.bold = true; g.pill = true; g.fill = fill; g.part = 0; return g; }

inline bool badgeFirst(const std::vector<Seg>& v, size_t q) { return v[q].badge && (q == 0 || !v[q - 1].badge || v[q - 1].part != v[q].part); }
inline bool badgeLast(const std::vector<Seg>& v, size_t q) { return v[q].badge && (q + 1 == v.size() || !v[q + 1].badge || v[q + 1].part != v[q].part); }
inline std::string utf8(const std::string& s) { std::string o; for (char ch : s) { if (ch == '\xB7') o += "\xC2\xB7"; else if (ch == '\x96') o += "\xE2\x80\x93"; else o += ch; } return o; }
inline std::string plain(const std::vector<Seg>& v) {   // for the tests / status: badges in [ ], the pill in ( ), UTF-8 arrows
    std::string t;
    for (size_t q = 0; q < v.size(); q++) { const Seg& g = v[q];
        std::string x = g.sep ? std::string("\xE2\x94\x82") : utf8(g.s);
        if (g.arrow) { if (!x.empty() && !(g.part == 2)) x += ' '; x += g.arrow > 0 ? "\xE2\x86\x91" : "\xE2\x86\x93"; }
        if (!g.s2.empty()) x += (g.part == 2 && !g.arrow ? "" : " ") + utf8(g.s2);
        if (g.pill) x = "(" + x + ")";
        if (badgeFirst(v, q)) x = "[" + x;
        if (badgeLast(v, q)) x += "]";
        if (!t.empty()) t += ' ';
        t += x;
    }
    return t;
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// the drawing. One line centred on cx, bottom at yBottom, inside [xMin, xMax]; the 0DTE text at xExp (the box's left edge) on the same row,
// only while it stays >= 10 px left of the line. Levels: 0 all, 1 no 0DTE, 2 no Next, 3 no G V C, 4 pill without the setups, 5 the pill's
// +/-GAMMA word only, 6 the badge only. Nothing fits = not drawn.
struct Item { int k = 0; int l = 0, t = 0, r = 0, b = 0; unsigned col = 0, fill = 0; bool bold = false; std::string s; };   // k: 0 filled box (col = border), 1 text, 2 line
struct Draw { bool ok = false; int l = 0, t = 0, r = 0, b = 0; int level = -1, pillForm = -1; bool exp = false; int expL = 0, expR = 0; std::vector<Item> items; std::string text; };
static const int ARW = 9, GAP = 5, PADX = 5, PILLX = 6, SEPW = 13, PADL = 6, EXP_GAP = 10;
template <class M> inline int segW(const std::vector<Seg>& v, size_t q, M meas) {
    const Seg& g = v[q]; if (g.sep) return SEPW;
    int w = 0; if (!g.s.empty()) w += meas(g.s, g.bold || g.badge || g.pill);
    if (g.arrow) w += (w ? (g.part == 2 ? 1 : 4) : 0) + ARW;
    if (!g.s2.empty()) w += (w ? (g.part == 2 ? 2 : 4) : 0) + meas(g.s2, g.bold);
    if (g.pill) w += 2 * PILLX;
    return w + (badgeFirst(v, q) ? PADX : 0) + (badgeLast(v, q) ? PADX : 0);
}
inline std::vector<Seg> join(const std::vector<std::vector<Seg>>& parts) {
    std::vector<Seg> v; for (const std::vector<Seg>& p : parts) { if (p.empty()) continue; if (!v.empty()) { Seg s; s.sep = true; s.col = C_SEP; s.part = p[0].part; v.push_back(s); } v.insert(v.end(), p.begin(), p.end()); } return v;
}
template <class M> inline Draw layout(const std::vector<std::string>& pillForms, unsigned pillCol, unsigned pillFill, const std::vector<Seg>& badge, const std::vector<Seg>& gvc,
                                      const std::vector<Seg>& next, const std::string& exp, unsigned expCol, M meas, int cx, int yBottom, int xMin, int xMax, int xExp, int fs) {
    Draw D; if (badge.empty() || xMax - xMin < 40) return D;
    const int h = fs + 8;
    auto pill = [&](int form) { std::vector<Seg> p; if (form >= 0 && form < (int)pillForms.size()) p.push_back(pillSeg(pillForms[(size_t)form], pillCol, pillFill)); return p; };
    auto width = [&](const std::vector<Seg>& v) { int w = 2 * PADL; for (size_t q = 0; q < v.size(); q++) w += segW(v, q, meas) + (q ? GAP : 0); return w; };
    const int expW = exp.empty() ? 0 : meas(exp, false);
    const int nf = (int)pillForms.size();
    struct Cand { int level, form; bool gv, nx, ex; };
    std::vector<Cand> C = { { 0, 0, true, true, true }, { 1, 0, true, true, false }, { 2, 0, true, false, false }, { 3, 0, false, false, false },
                            { 4, 1, false, false, false }, { 5, 2, false, false, false }, { 6, -1, false, false, false } };
    for (const Cand& cd : C) {
        if (cd.ex && !expW) continue;
        int form = cd.form; if (form >= nf) { if (cd.level == 4 || cd.level == 5) continue; form = nf - 1; }
        if (nf == 0 && cd.form >= 0 && cd.level >= 4) continue;
        std::vector<Seg> v = join({ pill(form), badge, cd.gv ? gvc : std::vector<Seg>(), cd.nx ? next : std::vector<Seg>() });
        const int tw = width(v); if (tw > xMax - xMin) continue;
        int l = cx - tw / 2; if (l + tw > xMax) l = xMax - tw; if (l < xMin) l = xMin;
        if (cd.ex && xExp + expW + EXP_GAP > l) continue;
        D.ok = true; D.level = cd.level; D.pillForm = form; D.exp = cd.ex; D.l = l; D.r = l + tw; D.b = yBottom; D.t = yBottom - h;
        Item bg; bg.k = 0; bg.l = D.l; bg.t = D.t; bg.r = D.r; bg.b = D.b; bg.fill = C_BG; bg.col = C_BORDER; D.items.push_back(bg);
        const int t = D.t + 2, b = D.b - 2, ym = (D.t + D.b) / 2;
        std::vector<int> xs(v.size()); { int x = D.l + PADL; for (size_t q = 0; q < v.size(); q++) { if (q) x += GAP; xs[q] = x; x += segW(v, q, meas); } }
        for (size_t q = 0; q < v.size(); q++) {
            if (v[q].pill) { Item P; P.k = 0; P.l = xs[q]; P.t = t; P.r = xs[q] + segW(v, q, meas); P.b = b; P.col = v[q].col; P.fill = v[q].fill; D.items.push_back(P); }
            if (badgeFirst(v, q)) { size_t e = q; while (!badgeLast(v, e)) e++; Item F; F.k = 0; F.l = xs[q]; F.t = t; F.r = xs[e] + segW(v, e, meas); F.b = b; F.fill = v[q].fill; F.col = v[q].fill; D.items.push_back(F); }
        }
        for (size_t q = 0; q < v.size(); q++) { const Seg& g = v[q];
            if (g.sep) { Item L2; L2.k = 2; L2.l = L2.r = xs[q] + SEPW / 2; L2.t = t + 1; L2.b = b - 1; L2.col = C_SEP; D.items.push_back(L2); continue; }
            int xx = xs[q] + (badgeFirst(v, q) ? PADX : 0) + (g.pill ? PILLX : 0);
            if (!g.s.empty()) { Item T; T.k = 1; T.l = xx; T.t = t; T.b = b; T.col = g.col; T.bold = g.bold || g.badge || g.pill; T.s = g.s; T.r = xx + meas(g.s, T.bold); D.items.push_back(T); xx = T.r + (g.arrow || !g.s2.empty() ? (g.part == 2 ? (g.arrow ? 1 : 2) : 4) : 0); }
            if (g.arrow) { const int ax = xx + ARW / 2, d = g.arrow > 0 ? -1 : 1;
                Item a1; a1.k = 2; a1.l = a1.r = ax; a1.t = ym - 4 * d; a1.b = ym + 4 * d; a1.col = g.acol; D.items.push_back(a1);
                Item a2 = a1; a2.l = ax - 3; a2.t = ym + d; a2.r = ax; a2.b = ym + 4 * d; D.items.push_back(a2);
                Item a3 = a2; a3.l = ax + 3; D.items.push_back(a3);
                xx += ARW + (!g.s2.empty() ? (g.part == 2 ? 2 : 4) : 0); }
            if (!g.s2.empty()) { Item T; T.k = 1; T.l = xx; T.t = t; T.b = b; T.col = g.acol; T.bold = g.bold; T.s = g.s2; T.r = xx + meas(g.s2, g.bold); D.items.push_back(T); }
        }
        if (cd.ex) { Item T; T.k = 1; T.l = xExp; T.r = xExp + expW; T.t = t; T.b = b; T.col = expCol; T.s = exp; D.items.push_back(T); D.expL = xExp; D.expR = xExp + expW; }
        D.text = (cd.ex ? utf8(exp) + " | " : std::string()) + plain(v);
        return D;
    }
    return D;
}

// the Now line's bottom when the gamma line sits under it (3 px apart); no gamma line = the old spot
inline int nowBottom(const Draw& g, int anchorBottom) { return g.ok ? g.t - 3 : anchorBottom; }

} // namespace gml
