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
#include "HodLodLogic.h"

namespace hlg {

static const char* const VERSION = "2.0.0";   // 2.0.0: the file v2 LOCATION rule; 1.1.0: chart basis, receipt time, model wording
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
    std::vector<Loc> locs;
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
struct HLine { std::string s; int role; };     // role 0 = title, 1 = location head, 2 = detail, 3 = guide
inline std::string ageText(long long sec) { if (sec < 0) return "-"; char b[32]; long long m = sec / 60; if (m < 60) snprintf(b, sizeof b, "%lldm old", m); else snprintf(b, sizeof b, "%lldh %lldm old", m / 60, m % 60); return b; }
inline std::vector<HLine> hover(const File& F, const std::string& stale, long long now = 0, int minCT = -1) {
    // (2.1.1, Rassul 22:23 "a huge hover text box") compact: one title line + one line per strike, nothing else
    (void)minCT;
    std::vector<HLine> o;
    o.push_back({ "GEX strikes ? (MenthorQ " + (F.asofTxt.empty() ? std::string("-") : F.asofTxt) + (stale.empty() ? "" : ", " + stale) + ")", 0 });
    (void)now;
    for (const Loc& L : F.locs)
        for (const Mem& m : L.mem) {
            std::string g = m.gOpen == G_NA ? "" : (std::string("  \xB7  ") + (m.gOpen >= 0 ? "+" : "") + std::to_string(m.gOpen) + "%");
            o.push_back({ m.chartTxt + (m.name.empty() ? "" : " " + m.name) + "  \xB7  " + pct1(m.share) + g, 2 });   // (2.1.1) no WALL / FUEL
        }
    return o;
}

// the View's GEX row + hover from the file and the clock (now = unix s; wd / min = CT weekday, minute). price is not needed: the
// bridge ordered the locations for the price it read every 30 s
inline void toView(hl::View& v, const File& F, double /*price*/, long long now, int wd, int min)
{
    v.gexItems.clear(); v.gexStale.clear(); v.gexHover.clear();
    if (!showable(F)) return;
    if (F.asof > now + 120) return;                           // (audit) received in the future = clock / file wrong: never shown
    v.gexStale = staleText(F, now, updateHours(wd, min));
    for (const Loc& L : F.locs) { hl::View::GexItem g; g.strike = rangeTxt(L); g.name.clear(); g.grow = L.grow; g.other = L.side == 'O' || L.side == 'A'; g.above = L.side == 'A' || (F.side == "down" && L.side == 'O') || (F.side == "up" && L.side == 'T'); v.gexItems.push_back(g); }   // (2.1.1, Rassul 22:26) no W / F / M on the row
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
        if (!bs.empty() && v.projLv.find(bs) == std::string::npos) {
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
    std::string sh; for (size_t i = 0; i < v.gexItems.size(); i++) sh += (i ? " ; " : "") + v.gexItems[i].strike + " " + v.gexItems[i].name + (v.gexItems[i].grow ? " up" : "") + (v.gexItems[i].other ? " (other)" : "");
    for (char& ch : sh) if ((unsigned char)ch == 0x96) ch = '-';
    return "GEX," + (F.ok ? F.state : std::string("none")) + ",asof " + (F.asofTxt.empty() ? std::string("-") : F.asofTxt) + ",shown " + (sh.empty() ? std::string("none") : sh)
         + ",basis " + F.basis + ",side " + F.side + (v.gexStale.empty() ? "" : "," + v.gexStale) + ",locs " + std::to_string(F.locs.size()) + (loadWhy.empty() ? "" : "," + loadWhy) + "\n";
}

} // namespace hlg
#endif
