#ifndef LS_HODLOD_GEX_H
#define LS_HODLOD_GEX_H
// HodLodGex.h (lsHodLod 2.1.0, 2026-10-14) -- the GEX ? line of the HOD / LOD box. SDK-free (unit-tested on Linux).
// Rassul 2026-10-14 16:01-16:04 CT: "a line in the HOD/LOD indicator box that identifies the key GEX levels where a reaction can
// occur so it is front and center - this will be where I look for trades"; evening rule (via the coordinator): strikes stay
// separate - grouping is only for COUNTING LOCATIONS so the line reaches further big strikes (ES 10-14: 7750, not 7790 / 7780 / 7775
// next to 7800). The rule runs in the bridge (lra.gex_nodes 2.0.0 LOCATION rule, frozen before its 13-day test):
//   sticky top-5 strikes by their own abs GEX within 2 expected moves (received grids only), price vs session VWAP picks the side
//   price is moving toward, walk that side's strikes biggest first (a strike within one zone width of a chosen one shares its
//   location), 5 locations + the nearest on the other side.
// Input: lsFlexLevels\LRA-GexNodes-<MKT>.csv v2:
//   VERSION|2  MARKET|ES  ASOF|<unix s of the grid's ARRIVAL>|<12-hour CT>|<stamp>  STATE|LIVE|STALE|PAUSED|WAIT|MISMATCH|BASIS_UNKNOWN|why
//   VWAP|<v>|down|up|none   BASIS|VERIFIED|APPLIED|UNKNOWN|...   WIDTH|zw|cap|pad|adr|src|step
//   LOC|ord|T/O/B/A|lo|hi|W/F/M|grow 0/1|members       STR|ord|strike|chart|share%|absM|W/F/M|x_open|g_open%|name|rank
// The row: "GEX ?   7780-7800 F ^  .  7750 F  .  7700 F   |   7825 M" (toward side nearest first, then the other side's nearest).
// Shown only for a grid RECEIVED by now on a verified / applied chart basis (external audit 2026-10-14); '?' = model context until a
// forward study proves it; stale (> 20 min in MenthorQ hours) = dimmed + "STALE 25m"; no file / no location = no row.
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <climits>
#include <utility>
#include "HodLodLogic.h"
#include "Fresh.h"                 // (HodLod 2.4.0) the one freshness / STALE rule (shared with the Dealer Profile)

namespace hlg {

static const char* const VERSION = "2.1.0";   // 2.1.0 (HodLod 2.4.0, 2026-10-18): the row's age by Fresh.h (also outside MenthorQ hours: older than the last close = STALE); the regime's HVL0 book parser (parseHvl)   // 2.0.0: the file v2 LOCATION rule; 1.1.0: chart basis, receipt time, model wording
static const int FILE_VERSION = 2;
static const size_t MAX_BYTES = 64 * 1024;
static const int MAX_LOCS = 8;
static const int MAX_MEMBERS = 12;
static const int STALE_MIN = 20;
static const int G_NA = INT_MIN;           // a growth that has no base (no grid then / not in it)

struct Mem { std::string strikeTxt, chartTxt, name; double share = NAN, absM = NAN, xOpen = NAN; int gOpen = G_NA, rank = 0; char sign = 'M'; };
struct Loc { int ord = 0; char side = 'T'; std::string loTxt, hiTxt; char sign = 'M'; bool grow = false; int n = 0; std::vector<Mem> mem; };
struct File {
    bool ok = false; std::string why, market, state, asofTxt, stampTxt, basis = "-", basisWhy, side = "none"; long long asof = 0; double vwap = NAN, basisApplied = NAN;
    long long arrived = 0;   // (HodLod 2.9.2, Fresh.h 1.1.0) when this file was written here (unix s, its mtime); 0 = unknown
    std::vector<Loc> locs;
    // (3.0.0 file / HodLod 2.3.0) the BAND row: BAND|open|kind|top|bottom|max_1d|min_1d|ext|at|src (kind RTH / ON / ON*)
    bool band = false; double bandOpen = NAN, bandTop = NAN, bandBot = NAN, bandMax = NAN, bandMin = NAN, bandExt = NAN; std::string bandKind, bandAt, bandSrc, bandOpenTxt, bandTopTxt, bandBotTxt, bandMaxTxt, bandMinTxt;
};

inline std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> o; size_t a = 0;
    for (;;) { size_t b = s.find(d, a); o.push_back(s.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
    return o;
}
inline bool fullD(const std::string& t, double& v) {      // strict: the whole token, finite
    if (t.empty() || t.size() > 32) return false;
    char* e = nullptr; v = std::strtod(t.c_str(), &e); return e && *e == 0 && std::isfinite(v);
}
inline bool fullL(const std::string& t, long long& v) {
    if (t.empty() || t.size() > 20) return false;
    char* e = nullptr; v = std::strtoll(t.c_str(), &e, 10); return e && *e == 0;
}
inline bool optD(const std::string& t, double& v) { if (t == "-") { v = NAN; return true; } return fullD(t, v); }
inline bool optI(const std::string& t, int& v) { if (t == "-") { v = G_NA; return true; } long long x; if (!fullL(t, x) || x < -100000 || x > 100000) return false; v = (int)x; return true; }
inline bool sgn(const std::string& t) { return t == "W" || t == "F" || t == "M"; }
inline bool numTxt(const std::string& t) { double v; return t.size() <= 16 && fullD(t, v); }

// strict parse; VERSION 2, MARKET must match; a bad LOC / STR row is skipped (counted in why), a bad header = not ok
inline File parse(const std::string& text, const std::string& market) {
    File F; if (text.size() > MAX_BYTES) { F.why = "over 64 KiB"; return F; }
    bool ver = false; int bad = 0; size_t a = 0; int lines = 0;
    while (a < text.size() && lines++ < 600) {
        size_t b = text.find('\n', a); std::string ln = text.substr(a, b == std::string::npos ? std::string::npos : b - a); a = b == std::string::npos ? text.size() : b + 1;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln.empty() || ln[0] == '#' || ln.size() > 600) continue;
        std::vector<std::string> c = split(ln, '|');
        if (c[0] == "VERSION") { long long v; if (c.size() < 2 || !fullL(c[1], v) || v != FILE_VERSION) { F.why = "unknown file version"; return F; } ver = true; }
        else if (c[0] == "MARKET") { if (c.size() < 2 || c[1] != market) { F.why = "file is for another market"; return F; } F.market = c[1]; }
        else if (c[0] == "ASOF") { if (c.size() < 4 || !fullL(c[1], F.asof) || F.asof < 0) { F.why = "bad ASOF"; return F; } F.asofTxt = c[2]; F.stampTxt = c[3]; }
        else if (c[0] == "STATE") { if (c.size() < 2) { F.why = "bad STATE"; return F; } F.state = c[1]; }
        else if (c[0] == "VWAP") { if (c.size() >= 3) { optD(c[1], F.vwap); F.side = c[2]; } }
        else if (c[0] == "BASIS") { if (c.size() >= 3 && (c[1] == "VERIFIED" || c[1] == "APPLIED" || c[1] == "UNKNOWN")) { F.basis = c[1]; optD(c[2], F.basisApplied); if (c.size() >= 7) F.basisWhy = c[6]; } }
        else if (c[0] == "BAND") {   // a bad BAND row = no band (the row falls back to the price split)
            File B = F; bool ok = c.size() >= 8 && fullD(c[1], B.bandOpen) && c[2].size() <= 4 && fullD(c[3], B.bandTop) && fullD(c[4], B.bandBot)
                && fullD(c[5], B.bandMax) && fullD(c[6], B.bandMin) && fullD(c[7], B.bandExt) && B.bandTop >= B.bandOpen && B.bandBot <= B.bandOpen;
            if (ok) { F.band = true; F.bandOpen = B.bandOpen; F.bandTop = B.bandTop; F.bandBot = B.bandBot; F.bandMax = B.bandMax; F.bandMin = B.bandMin; F.bandExt = B.bandExt;
                      F.bandKind = c[2]; F.bandOpenTxt = c[1]; F.bandTopTxt = c[3]; F.bandBotTxt = c[4]; F.bandMaxTxt = c[5]; F.bandMinTxt = c[6];
                      if (c.size() >= 9 && c[8].size() <= 8) F.bandAt = c[8];
                      if (c.size() >= 10 && c[9].size() <= 24) F.bandSrc = c[9]; }
            else bad++;
        }
        else if (c[0] == "LOC") {
            long long o = 0, n = 0; Loc L;
            bool ok = c.size() == 8 && fullL(c[1], o) && o >= 1 && o <= 99 && c[2].size() == 1 && std::string("TOBA").find(c[2][0]) != std::string::npos
                && numTxt(c[3]) && numTxt(c[4]) && sgn(c[5]) && (c[6] == "0" || c[6] == "1") && fullL(c[7], n) && n >= 1 && n <= MAX_MEMBERS;
            if (!ok || (int)F.locs.size() >= MAX_LOCS) { bad++; continue; }
            L.ord = (int)o; L.side = c[2][0]; L.loTxt = c[3]; L.hiTxt = c[4]; L.sign = c[5][0]; L.grow = c[6] == "1"; L.n = (int)n;
            F.locs.push_back(L);
        }
        else if (c[0] == "STR") {
            long long o = 0, rk = 0; Mem m;
            bool ok = c.size() == 11 && fullL(c[1], o) && numTxt(c[2]) && numTxt(c[3]) && fullD(c[4], m.share) && m.share >= 0 && m.share <= 100
                && fullD(c[5], m.absM) && sgn(c[6]) && optD(c[7], m.xOpen) && optI(c[8], m.gOpen) && c[9].size() <= 24 && fullL(c[10], rk) && rk >= 1 && rk <= 999;
            Loc* L = nullptr; for (Loc& x : F.locs) if (x.ord == (int)o) L = &x;
            if (!ok || !L || (int)L->mem.size() >= MAX_MEMBERS) { bad++; continue; }
            m.strikeTxt = c[2]; m.chartTxt = c[3]; m.sign = c[6][0]; m.name = c[9] == "-" ? "" : c[9]; m.rank = (int)rk;
            L->mem.push_back(m);
        }
    }
    if (!ver) { F.why = "no VERSION row"; return F; }
    if (F.market.empty()) { F.why = "no MARKET row"; return F; }
    if (F.state.empty()) { F.why = "no STATE row"; return F; }
    std::vector<Loc> keep; for (const Loc& L : F.locs) if (!L.mem.empty()) keep.push_back(L); else bad++;   // a location needs its strikes
    F.locs.swap(keep);
    F.ok = true; F.why = bad ? std::to_string(bad) + " bad LOC / STR rows skipped" : "";
    return F;
}

// MenthorQ updates futures 21:30 -> 16:00 CT, Sunday evening to Friday (lra.menthorq._update_hours); wd 0 = Sunday, min = CT minute
inline bool updateHours(int wd, int min) {
    if (wd == 6 || (wd == 5 && min > 960) || (wd == 0 && min < 1300)) return false;
    return !(min > 960 && min < 1300);
}
// "" (fresh / paused / no data) or "STALE 25m" / "STALE 3h"; asof and now are unix seconds
inline std::string staleText(const File& F, long long now, bool inUpdateHours) {
    if (!F.ok || F.asof <= 0 || !inUpdateHours) return "";
    if (F.state == "STALE" || now - F.asof > STALE_MIN * 60) {
        long long m = (now - F.asof) / 60; if (m < STALE_MIN) m = STALE_MIN;
        char b[32]; if (m < 60) snprintf(b, sizeof b, "STALE %lldm", m); else snprintf(b, sizeof b, "STALE %lldh", m / 60);
        return b;
    }
    return "";
}
// shown only when the gates pass (audit: a basis that is not verified / applied hides the row - unknown is not zero)
inline bool showable(const File& F) {
    return F.ok && !F.locs.empty() && (F.state == "LIVE" || F.state == "STALE" || F.state == "PAUSED") && (F.basis == "VERIFIED" || F.basis == "APPLIED");
}
inline std::string rangeTxt(const Loc& L) { return L.loTxt == L.hiTxt ? L.loTxt : L.loTxt + "\x96" + L.hiTxt; }   // en dash (cp1252)

// ---- hover text (cp1252 only; '\xB7' middle dot, '\x96' en dash; the arrow on the row is drawn as lines)
inline std::string ratioX(double v) { if (!std::isfinite(v)) return "n/a"; char b[24]; snprintf(b, sizeof b, "%.1fx", v); return b; }
inline std::string pct1(double v) { char b[24]; snprintf(b, sizeof b, "%.1f%%", v); return b; }
// the vendor-model gamma sign in Rassul's words ("dealers forced to buy / sell" stays - his 2026-10-14 correction), '?' until proven
inline std::string signWord(char s) { return s == 'W' ? "WALL ?" : (s == 'F' ? "FUEL ?" : "MIXED ?"); }
inline std::string signHow(char s) {
    return s == 'W' ? "net long gamma (model): dealers forced to sell rallies into it / buy dips into it - a cushion"
         : (s == 'F' ? "net short gamma (model): dealers forced to sell drops / buy rallies through it - moves can run"
                     : "long and short gamma about equal (model) - no clear forced side");
}
struct HLine { std::string s; int role; };
// (2.3.0) "Band: open 7,703.00 (RTH 8:30) - up to 7,800.00 (1.38x to 1D Max 7,770.00), down to 7,650.00 (1.38x to 1D Min 7,660.00); biggest abs GEX, no neighbours"
inline std::string bandLine(const File& F) {
    const std::string k = F.bandKind == "RTH" ? "RTH open" : (F.bandKind == "ON" ? "5 PM open (before the RTH open)" : (F.bandKind == "ON*" ? "first overnight bar (before the RTH open)" : "open"));
    char e[16]; snprintf(e, sizeof e, "%gx", std::isfinite(F.bandExt) ? F.bandExt : 1.38);
    return "Band from the " + k + " " + F.bandOpenTxt + (F.bandAt.empty() ? "" : " (" + F.bandAt + ")") + ": up to " + F.bandTopTxt + " (" + e + " to 1D Max " + F.bandMaxTxt + "), down to " + F.bandBotTxt + " (" + e + " to 1D Min " + F.bandMinTxt + "); the 3 biggest abs-GEX strikes each side, never two neighbouring";
}     // role 0 = title, 1 = location head, 2 = detail, 3 = guide
inline std::string ageText(long long sec) { if (sec < 0) return "-"; char b[32]; long long m = sec / 60; if (m < 60) snprintf(b, sizeof b, "%lldm old", m); else snprintf(b, sizeof b, "%lldh %lldm old", m / 60, m % 60); return b; }
inline std::vector<HLine> hover(const File& F, const std::string& stale, long long now = 0, int minCT = -1) {
    // (2.1.1, Rassul 22:23 "a huge hover text box") compact: one title line + one line per strike, nothing else
    (void)minCT;
    std::vector<HLine> o;
    o.push_back({ "GEX strikes ? (MenthorQ " + (F.asofTxt.empty() ? std::string("-") : F.asofTxt) + (stale.empty() ? "" : ", " + stale) + ")", 0 });
    if (F.band) o.push_back({ bandLine(F), 2 });   // (2.3.0) where the strikes were picked
    (void)now;
    for (const Loc& L : F.locs)
        for (const Mem& m : L.mem) {
            std::string g = m.gOpen == G_NA ? "" : (std::string("  \xB7  ") + (m.gOpen >= 0 ? "+" : "") + std::to_string(m.gOpen) + "%");
            o.push_back({ m.chartTxt + (m.name.empty() ? "" : " " + m.name) + "  \xB7  " + pct1(m.share) + g, 2 });   // (2.1.1) no WALL / FUEL
        }
    return o;
}

// the View's GEX row + hover from the file and the clock (now = unix s; wd / min = CT weekday, minute). (2.2.4) price splits the row: 3 above, the price, 3 below
inline void toView(hl::View& v, const File& F, double price, long long now, int wd, int min, int dec = 2)
{
    v.gexItems.clear(); v.gexStale.clear(); v.gexHover.clear();
    if (!showable(F)) return;
    if (F.asof > now + 120) return;                           // (audit) received in the future = clock / file wrong: never shown
    {   // (2.1.0, audit C finding 7; 2.9.2 Fresh.h 1.1.0) one rule: STALE in MenthorQ hours when it ARRIVED over 20 min ago in RTH / 45 min overnight, and outside them when older than the last close
        (void)wd;
        const long long off = fr::offsetFromClock(now, min);
        const fr::Verdict fv = fr::judgeArrived(fr::K_GEX, fr::ctFromUnix(F.asof, off), F.arrived > 0 ? fr::ctFromUnix(F.arrived, off) : 0, fr::ctFromUnix(now, off), false);   // (2.9.2) by arrival
        v.gexStale = fv.stale() ? fv.tag : ((F.state == "STALE" && fv.st == fr::LIVE) ? staleText(F, now, true) : std::string());
    }
    bool bandMode = false; for (const Loc& L : F.locs) if (L.side == 'A' || L.side == 'B') bandMode = true;
    if (bandMode) {   // (2.3.0, Rassul 2026-10-17 19:53 / 19:55 + mockup v2) the BAND rule: side A = above the RTH open (red, an up triangle first),
        // B = below it (green, a down triangle first); each side nearest the open first: "^ 7765 . 7780 . 7800 | 7703.00 | v 7700 . 7675 . 7650"
        auto firstNum = [](const std::string& t) { std::string o; for (char c : t) if ((c >= '0' && c <= '9') || c == '.') o += c; else if (!o.empty()) break; char* e = nullptr; const double d = std::strtod(o.c_str(), &e); return (o.empty() || !e || *e) ? NAN : d; };
        std::vector<std::pair<double, const Loc*> > up, dn;
        for (const Loc& L : F.locs) { if (L.side != 'A' && L.side != 'B') continue; const double md = firstNum(L.loTxt); (L.side == 'A' ? up : dn).push_back(std::make_pair(md, &L)); }
        std::stable_sort(up.begin(), up.end(), [](const std::pair<double, const Loc*>& a, const std::pair<double, const Loc*>& b) { return a.first < b.first; });
        std::stable_sort(dn.begin(), dn.end(), [](const std::pair<double, const Loc*>& a, const std::pair<double, const Loc*>& b) { return a.first > b.first; });
        if (up.size() > 3) up.resize(3);
        if (dn.size() > 3) dn.resize(3);
        auto push = [&](const Loc* L, bool above, bool first) { hl::View::GexItem g; g.strike = rangeTxt(*L); g.name = L->mem.empty() ? std::string() : L->mem[0].name; g.grow = L->grow; g.above = above; g.mark = first ? (above ? 'U' : 'D') : 0; v.gexItems.push_back(g); };
        for (size_t i = 0; i < up.size(); i++) push(up[i].second, true, i == 0);
        if (std::isfinite(price) && (!up.empty() || !dn.empty())) {
            char pb[48]; snprintf(pb, sizeof pb, "%.*f", dec < 0 ? 2 : (dec > 6 ? 6 : dec), price);
            hl::View::GexItem m; m.strike = pb; m.mid = true; m.above = false; v.gexItems.push_back(m);
        }
        for (size_t i = 0; i < dn.size(); i++) push(dn[i].second, false, i == 0);
    } else
    {   // (2.2.4, Rassul 2026-10-17 "put 3 above first then 3 below ... above are red (resistance), below green (support)", "name before the level"):
        // the 3 nearest locations ABOVE the price (highest first), the current price, then the 3 nearest BELOW (nearest first); the name = the location's best-ranked MenthorQ name
        struct Cand { double mid; const Loc* L; bool above; };
        std::vector<Cand> up, dn;
        auto firstNum = [](const std::string& t) { std::string o; for (char c : t) if ((c >= '0' && c <= '9') || c == '.') o += c; else if (!o.empty()) break; char* e = nullptr; const double d = std::strtod(o.c_str(), &e); return (o.empty() || !e || *e) ? NAN : d; };
        for (const Loc& L : F.locs) {
            const double lo = firstNum(L.loTxt), hi = firstNum(L.hiTxt), md = (std::isfinite(lo) && std::isfinite(hi)) ? 0.5 * (lo + hi) : NAN;
            const bool oldAbove = L.side == 'A' || (F.side == "down" && L.side == 'O') || (F.side == "up" && L.side == 'T');
            const bool ab = (std::isfinite(price) && std::isfinite(md)) ? md > price : oldAbove;
            Cand c = { md, &L, ab }; (ab ? up : dn).push_back(c);
        }
        const auto byDist = [&](const Cand& a, const Cand& b) { const double da = std::isfinite(a.mid) && std::isfinite(price) ? std::fabs(a.mid - price) : 1e18, db = std::isfinite(b.mid) && std::isfinite(price) ? std::fabs(b.mid - price) : 1e18; return da < db; };
        std::stable_sort(up.begin(), up.end(), byDist); std::stable_sort(dn.begin(), dn.end(), byDist);
        if (up.size() > 3) up.resize(3);
        if (dn.size() > 3) dn.resize(3);
        std::reverse(up.begin(), up.end());                      // highest first, the nearest to the price last
        auto push = [&](const Cand& c) { hl::View::GexItem g; g.strike = rangeTxt(*c.L); g.name = c.L->mem.empty() ? std::string() : c.L->mem[0].name; g.grow = c.L->grow; g.other = false; g.above = c.above; v.gexItems.push_back(g); };
        for (size_t i = 0; i < up.size(); i++) push(up[i]);
        if (std::isfinite(price) && (!up.empty() || !dn.empty())) {
            char pb[48]; snprintf(pb, sizeof pb, "%.*f", dec < 0 ? 2 : (dec > 6 ? 6 : dec), price);
            hl::View::GexItem m; m.strike = pb; m.mid = true; m.above = false; v.gexItems.push_back(m);
        }
        for (size_t i = 0; i < dn.size(); i++) push(dn[i]);
    }
    // (HodLod 2.2.1, Rassul 12:18 "do you see the levels mentioned?") the Gex S/R strike nearest the projected target (within 10% of the
    // projected range) joins the projection's level text, in the table and under the flag
    if (v.projOk && std::isfinite(v.projPx) && v.projRange > 0) {
        auto num = [](std::string t) { std::string o; for (char c : t) if ((c >= '0' && c <= '9') || c == '.' || c == '-') o += c; char* e = nullptr; const double d = std::strtod(o.c_str(), &e); return (o.empty() || !e || *e) ? NAN : d; };
        double best = 0.10 * v.projRange; std::string bs;
        for (const Loc& L : F.locs) {
            const double lo = num(L.loTxt), hi = num(L.hiTxt); if (!std::isfinite(lo) || !std::isfinite(hi)) continue;
            const double d = v.projPx < (std::min)(lo, hi) ? (std::min)(lo, hi) - v.projPx : (v.projPx > (std::max)(lo, hi) ? v.projPx - (std::max)(lo, hi) : 0.0);
            if (d <= best) { best = d; bs = "S/R " + rangeTxt(L); }
        }
        if (!bs.empty() && !v.projHit && v.projLv.find(bs) == std::string::npos) {   // (2.2.2) a reached target names no level
            v.projLv += (v.projLv.empty() ? "" : " \xB7 ") + bs;
            for (size_t q = 0; q < v.headLeftSegs.size(); q++) if (v.headLeftSegs[q].s.find(v.projTxt) != std::string::npos) v.headLeftSegs[q].s = "   " + v.projTxt + "  " + v.projLv;
        }
    }
    int last = -1;
    for (const HLine& h : hover(F, v.gexStale, now, min)) {   // an empty Seg = a 5 px spacer before each location and before the guide
        if ((h.role == 1 || (h.role == 3 && last != 3)) && last >= 0) v.gexHover.push_back(hl::Seg{ "", hl::C_GREY });
        v.gexHover.push_back(hl::Seg{ h.s, h.role <= 1 ? hl::C_SLATE : hl::C_GREY }); last = h.role;
    }
}
// one line for the status file: "GEX,<state>,asof <t>,shown <..>,basis <b>,..."
inline std::string statusLine(const File& F, const hl::View& v, const std::string& loadWhy)
{
    std::string sh; for (size_t i = 0; i < v.gexItems.size(); i++) if (!v.gexItems[i].mid) sh += (sh.empty() ? "" : " ; ") + v.gexItems[i].strike + " " + v.gexItems[i].name + (v.gexItems[i].grow ? " up" : "") + (v.gexItems[i].other ? " (other)" : "");
    for (char& ch : sh) if ((unsigned char)ch == 0x96) ch = '-';
    return "GEX," + (F.ok ? F.state : std::string("none")) + ",asof " + (F.asofTxt.empty() ? std::string("-") : F.asofTxt) + ",shown " + (sh.empty() ? std::string("none") : sh)
         + ",basis " + F.basis + ",side " + F.side + (v.gexStale.empty() ? "" : "," + v.gexStale) + ",locs " + std::to_string(F.locs.size()) + (F.band ? ",band " + F.bandKind + " open " + F.bandOpenTxt + " up " + F.bandTopTxt + " down " + F.bandBotTxt : std::string(",band none")) + (loadWhy.empty() ? "" : "," + loadWhy) + "\n";
}

// ---- (2.1.0 / HodLod 2.4.0, audit C finding 8) the regime's ONE MenthorQ source: the HVL0 strike of lsFlexLevels\GammaProfile-MQ-<MKT>.csv
// = the same MenthorQ snapshot whose "HVL 0D" line the Dealer Profile draws (MenthorQLevels.csv). The book's FLIP row is MenthorQ's
// all-expiry HVL (ES 10-16: FLIP 7820 vs HVL0 7780; CL: 87.25 vs 94.75) - the bridge's LRA-Session GAM uses FLIP, which is why the box
// said "Reg: Neg" while the HVL badge said price was above HVL. FLIP is kept only for the status line (not used).
//   STRIKE,<price>,<pct>,<rank>,<isKing>,<type CR0+GW0 / HVL0 / PS0+HVL0 ...>,<strike>,,<first-seen>   MARKET,<mkt>   SESSION,<date>
//   FLIP,<price>   ASOF,<CT sec of day>[,<date>]
struct HvlBook { bool ok = false; std::string why, hvlTxt, flipTxt, session; double hvl = NAN, flip = NAN; long long asofCt = 0; long long arrivedCt = 0; };   // (2.9.2) arrivedCt = the file's write time (CT wall s), 0 = unknown
inline HvlBook parseHvl(const std::string& text, const std::string& market)
{
    HvlBook B; if (text.size() > MAX_BYTES) { B.why = "over 64 KiB"; return B; }
    bool mk = false; long long sessDay = -1, asofSec = -1, asofDay = -1; size_t a = 0; int lines = 0;
    while (a < text.size() && lines++ < 400) {
        size_t b = text.find('\n', a); std::string ln = text.substr(a, b == std::string::npos ? std::string::npos : b - a); a = b == std::string::npos ? text.size() : b + 1;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln.empty() || ln[0] == '#' || ln.size() > 300) continue;
        const std::vector<std::string> c = split(ln, ',');
        if (c[0] == "MARKET") { if (c.size() < 2 || c[1] != market) { B.why = "book is for another market"; return B; } mk = true; }
        else if (c[0] == "SESSION" && c.size() >= 2) { if (fr::parseYmd(c[1], sessDay)) B.session = c[1]; }
        else if (c[0] == "FLIP" && c.size() >= 2) { double v; if (fullD(c[1], v) && v > 0) { B.flip = v; B.flipTxt = c[1]; } }
        else if (c[0] == "ASOF" && c.size() >= 2) { long long s2; if (fullL(c[1], s2) && s2 >= 0 && s2 < 86400) asofSec = s2; if (c.size() >= 3) fr::parseYmd(c[2], asofDay); }
        else if (c[0] == "STRIKE" && c.size() >= 7) {
            bool hv = false; for (const std::string& t : split(c[5], '+')) if (t == "HVL0") hv = true;
            double v; if (hv && fullD(c[1], v) && v > 0 && !std::isfinite(B.hvl)) { B.hvl = v; B.hvlTxt = c[1]; }
        }
    }
    if (!mk) { B.why = "no MARKET row"; return B; }
    if (asofSec < 0) { B.why = "no ASOF row"; return B; }
    // the snapshot's CT time: ASOF date + seconds; an older file without the date: the session date (an evening snapshot >= 17:00 CT belongs to the day before)
    if (asofDay < 0 && sessDay >= 0) asofDay = asofSec >= 17 * 3600 ? sessDay - 1 : sessDay;
    if (asofDay < 0) { B.why = "no ASOF / SESSION date"; return B; }
    B.asofCt = asofDay * 86400 + asofSec;
    if (!std::isfinite(B.hvl)) { B.why = "no HVL0 strike in the book"; return B; }
    B.ok = true; return B;
}
// the regime from that HVL and the last closed bar's close: +1 = price at / above HVL (positive gamma), -1 below, 0 = unknown
inline int regimeSign(const HvlBook& B, double close) { if (!B.ok || !std::isfinite(close) || !(close > 0)) return 0; return close >= B.hvl ? 1 : -1; }
// (HodLod 2.5.0, Rassul 2026-10-21 10:40 "i also dont see the regime") the HVL0 strike named in the Gex S/R file (LRA-GexNodes-<MKT>.csv,
// the same MenthorQ intraday names) - used for the regime only while that file is LIVE and the HVL book is stale / missing. NAN = none.
inline double hvlFromGex(const File& F)
{
    if (!F.ok || F.state != "LIVE") return NAN;
    for (const Loc& L : F.locs) for (const Mem& m : L.mem) {
        if (m.name.find("HVL0") == std::string::npos) continue;
        char* e = nullptr; const double d = std::strtod(m.chartTxt.c_str(), &e);
        if (e != m.chartTxt.c_str() && std::isfinite(d) && d > 0) return d;
    }
    return NAN;
}

} // namespace hlg
#endif
