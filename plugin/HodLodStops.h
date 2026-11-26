// HodLodStops.h 1.0.0 (2026-11-26 16:14 CT, Rassul "ok.. this looks good .. build") - the SDK-free STOPS RESTING / STOPS RUN section of
// lsHodLod 2.55.0, to the right of the HOD / LOD box (Linux-tested). Requests (CT, 2026-11-26): 15:53 "a section that identifies the levels
// that were already swept"; 16:00 "my intention is to identify highs and lows that have had their stops run"; 16:02 "switch positions so
// resting is on top"; 16:09 "stops run should be below the divider"; 16:11 no held / back ("once it is in the stops run, it means the level
// was broken"); 16:12 "if there are more levels that get taken make sure there is space". Mockup: StopsBusy-mockup.html.
//   Levels = the HIGHS and LOWS where stops sit (from the chart's LRA-KeyLevels file): session PhrH / PhrL, ONH / ONL, LonHI / LonLO, AsiaHI /
//   AsiaLO; daily PDH / PDL, PFH / PFL; weekly PWH / PWL, CWH / CWL. Volume / gamma levels are left out (no stop pool behind them).
//   RUN = a bar after the level formed traded at least 1 tick THROUGH it (high >= level + 1 tick; low <= level - 1 tick). A level formed in a
//   window counts only after the window: Asia after 1:00 AM, London after 7:30 AM (or the open if earlier), the pre-open hour and the overnight
//   from the RTH open. Prior-day and weekly levels count from the session start (5:00 PM).
//   RESTING = the nearest 2 not run: highs above the last price, lows below it.
//   Display: "Stops resting" (title, line), "↑" + up to 2 names, "↓" + up to 2 names; a line; "Stops run", "Highs" / "Lows" with 2 names a
//   line, every run level shown (the section grows UPWARD; its bottom stays on the pane's bottom edge).
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <cmath>

namespace stp {
static const char* const VERSION = "1.0.0";
static const int ROW = 16, PX = 8, GAP = 6;
static const unsigned C_BG = 0x000B0F14, C_EDGE = 0x00444444, C_SEPV = 0x002A2F36, C_HEAD = 0x009A9A9A, C_TXT = 0x00E5E7EB,
                      C_RED = 0x00EF4444, C_GREEN = 0x0022C55E;
static const char* const UP = "\xE2\x86\x91", * const DN = "\xE2\x86\x93";

struct Lv { std::string name; double px = 0; };
struct Bar { long long t = 0; double h = 0, l = 0, c = 0; };      // t = bar END, local CT seconds since 1970
struct Def { const char* key; const char* show; bool hi; int form; };   // form: -1 session start, -2 RTH open, -3 London end, else CT minute
// order = session, daily, weekly (Rassul 15:18 "session levels first, then daily ... then weekly")
static const Def DEFS[16] = {
    { "PHH", "PhrH", true, -2 }, { "ONH", "ONH", true, -2 }, { "LonHI", "LonHI", true, -3 }, { "AsiaHI", "AsiaHI", true, 60 },
    { "PDH", "PDH", true, -1 }, { "PFH", "PFH", true, -1 }, { "PWH", "PWH", true, -1 }, { "CWH", "CWH", true, -1 },
    { "PHL", "PhrL", false, -2 }, { "ONL", "ONL", false, -2 }, { "LonLO", "LonLO", false, -3 }, { "AsiaLO", "AsiaLO", false, 60 },
    { "PDL", "PDL", false, -1 }, { "PFL", "PFL", false, -1 }, { "PWL", "PWL", false, -1 }, { "CWL", "CWL", false, -1 } };
inline int defOf(const std::string& n) { for (int i = 0; i < 16; i++) if (n == DEFS[i].key || n == DEFS[i].show) return i; return -1; }
inline int sessMin(int mod) { return ((mod - 17 * 60) % 1440 + 1440) % 1440; }   // minutes since the 5:00 PM session start

struct Lvl { std::string name; double px = 0; bool hi = true; int order = 0; bool run = false; long long runT = 0; };
struct Out { std::vector<Lvl> runHi, runLo, restUp, restDn; std::string why; bool ok = false; };

// keys: the chart's key levels (names may repeat at one price); bars: this session's bars from 5:00 PM (closed + forming), oldest first;
// openMin = the RTH open (CT minute); last = the last price
inline Out compute(const std::vector<Lv>& keys, const std::vector<Bar>& bars, int openMin, double tick, double last) {
    Out O;
    if (!(tick > 0) || !std::isfinite(tick)) { O.why = "no tick size"; return O; }
    if (keys.empty()) { O.why = "no key levels (LRA-KeyLevels missing / stale)"; return O; }
    if (bars.empty()) { O.why = "no bars this session"; return O; }
    const int lonEnd = (std::min)(7 * 60 + 30, openMin);
    std::vector<Lvl> L;
    for (const Lv& k : keys) {
        const int d = defOf(k.name); if (d < 0 || !(k.px > 0) || !std::isfinite(k.px)) continue;
        bool dup = false; for (const Lvl& x : L) if (x.order == d) dup = true;   // one per name
        if (dup) continue;
        const Def& D = DEFS[d]; Lvl x; x.name = D.show; x.px = k.px; x.hi = D.hi; x.order = d;
        const int f = D.form == -1 ? -1 : D.form == -2 ? openMin : D.form == -3 ? lonEnd : D.form;
        const int fS = f < 0 ? -1 : sessMin(f);
        for (const Bar& b : bars) {
            const int mod = (int)(((b.t % 86400) + 86400) % 86400 / 60), bs = sessMin(mod);
            if (fS >= 0 && bs <= fS) continue;   // the bar ENDS inside / before the level's window: the level is not formed yet
            if (D.hi ? (b.h >= k.px + tick * 0.999) : (b.l <= k.px - tick * 0.999)) { x.run = true; x.runT = b.t; break; }
        }
        L.push_back(x);
    }
    std::sort(L.begin(), L.end(), [](const Lvl& a, const Lvl& b) { return a.order < b.order; });
    for (const Lvl& x : L) if (x.run) (x.hi ? O.runHi : O.runLo).push_back(x);
    std::vector<Lvl> up, dn;
    if (std::isfinite(last)) for (const Lvl& x : L) { if (x.run) continue; if (x.hi && x.px > last) up.push_back(x); if (!x.hi && x.px < last) dn.push_back(x); }
    std::stable_sort(up.begin(), up.end(), [](const Lvl& a, const Lvl& b) { return a.px < b.px || (a.px == b.px && a.order < b.order); });
    std::stable_sort(dn.begin(), dn.end(), [](const Lvl& a, const Lvl& b) { return a.px > b.px || (a.px == b.px && a.order < b.order); });
    for (size_t i = 0; i < up.size() && i < 2; i++) O.restUp.push_back(up[i]);
    for (size_t i = 0; i < dn.size() && i < 2; i++) O.restDn.push_back(dn[i]);
    O.ok = true; return O;
}

struct Item { int k = 0; int l = 0, t = 0, r = 0, b = 0; std::string s; unsigned col = 0, fill = 0; bool bold = false; };   // k 0 box, 1 text, 2 line
struct Draw { bool ok = false; int l = 0, t = 0, r = 0, b = 0; std::vector<Item> items; std::string text, why; };

inline std::vector<std::string> pairs(const std::vector<Lvl>& v) {
    std::vector<std::string> out;
    for (size_t i = 0; i < v.size(); i += 2) out.push_back(v[i].name + (i + 1 < v.size() ? " " + v[i + 1].name : std::string()));
    return out;
}
inline std::string names(const std::vector<Lvl>& v) { std::string s; for (const Lvl& x : v) s += (s.empty() ? "" : " ") + x.name; return s.empty() ? "-" : s; }

// left = the HOD / LOD box's right edge (shared border); bottom = the pane's bottom; minH = the box's height (at least); top = the highest
// the section may reach; right = the last bar
inline Draw layout(const Out& O, const std::function<int(const std::string&, bool)>& M, int left, int bottom, int minH, int top, int right) {
    Draw D; if (!O.ok) { D.why = O.why.empty() ? "no data" : O.why; return D; }
    const std::string tR = "Stops resting", tN = "Stops run", lH = "Highs", lL = "Lows";
    const std::vector<std::string> pH = pairs(O.runHi), pL = pairs(O.runLo);
    const std::string up = names(O.restUp), dn = names(O.restDn);
    const int la = (std::max)(M(UP, false), M(DN, false));
    const int ll = (std::max)(M(lH, true), M(lL, true));
    int wN = M("none yet", false); for (const std::string& s : pH) wN = (std::max)(wN, M(s, false)); for (const std::string& s : pL) wN = (std::max)(wN, M(s, false));
    const int W = PX + (std::max)((std::max)(M(tR, false), M(tN, false)), (std::max)(la + GAP + (std::max)(M(up, false), M(dn, false)), ll + GAP + wN)) + PX;
    const int nRun = (int)(pH.size() + pL.size()), runRows = nRun ? nRun : 1;
    const int Hc = 3 + ROW + 3 + 2 * ROW + 4 + ROW + 2 + runRows * ROW + 3;
    const int H = (std::max)(Hc, minH);
    if (left + W > right) { D.why = "no room (" + std::to_string(W) + " px wide, " + std::to_string(right - left) + " px free)"; return D; }
    if (bottom - H < top) { D.why = "no room (" + std::to_string(H) + " px tall)"; return D; }
    D.l = left - 1; D.r = left + W; D.b = bottom; D.t = bottom - H;
    D.items.push_back(Item{ 0, D.l, D.t, D.r, D.b, "", C_EDGE, C_BG, false });
    auto text = [&](int x, int y, const std::string& s, unsigned col, bool bold) { const int w = M(s, bold); D.items.push_back(Item{ 1, x, y, x + w, y + ROW, s, col, 0, bold }); return w; };
    const int x0 = left + PX; int y = D.t + 3;
    text(x0, y, tR, C_HEAD, false); y += ROW + 1; D.items.push_back(Item{ 2, x0, y, D.r - PX, y, "", C_SEPV, 0, false }); y += 2;
    text(x0, y, UP, C_RED, false); text(x0 + la + GAP, y, up, O.restUp.empty() ? C_HEAD : C_TXT, false); y += ROW;
    text(x0, y, DN, C_GREEN, false); text(x0 + la + GAP, y, dn, O.restDn.empty() ? C_HEAD : C_TXT, false); y += ROW;
    y += 2; D.items.push_back(Item{ 2, x0, y, D.r - PX, y, "", C_SEPV, 0, false }); y += 2;   // the line ABOVE "Stops run" (16:09)
    text(x0, y, tN, C_HEAD, false); y += ROW + 2;
    if (!nRun) { text(x0, y, "none yet", C_HEAD, false); }
    for (size_t i = 0; i < pH.size(); i++) { if (!i) text(x0, y, lH, C_RED, true); text(x0 + ll + GAP, y, pH[i], C_TXT, false); y += ROW; }
    for (size_t i = 0; i < pL.size(); i++) { if (!i) text(x0, y, lL, C_GREEN, true); text(x0 + ll + GAP, y, pL[i], C_TXT, false); y += ROW; }
    D.text = std::string("resting up ") + up + " ; down " + dn + " ; run highs " + names(O.runHi) + " ; lows " + names(O.runLo);
    D.ok = true; return D;
}
}  // namespace stp
