// CobCheck.h - the Cob Reader's read of the 3-hour-sweep CoB (lsZzoCombo 3.0.0, 2026-11-15), SDK-free.
// Rassul 2026-11-15 17:26-17:28 CT "did you update the zzo wave to the new cob finder and did you add the checklist , have you updated the
// check list to what makes the cob work like what we discussed and are you aware of the warning that make it not work" / "go .. and make
// sure you build the new cob finder indicator"; 20:55 "just build them".
// THE COBS are lsCobSignal's own (CobSignalLogic.h cobs::Engine on the same bars, htfSweep): a completed 3-bar ZigZag wave whose origin is
// the 3-hour swing's extreme beyond a standing S1 / S2 (R1 / R2), back through it, 1.9x, 3+ bars, room, the 5:00 PM - 11:00 AM window,
// drawn at ZZO count 10+. The pane tags exactly the Cobs and Lvps the price chart shows.
// THE CHECKLIST (decided when the CoB is; year test 2025-10-27 .. 2026-10-27, 6 markets, 274 Cobs, win = 1.33 before the origin):
//   3row  3 or more candles of the Cob's colour in a row inside the wave            58% with, 48% without
//   SMA   the 50 SMA under the 100 SMA at the origin for a bullish Cob (above: bear) 59% / 47%
//   S1 / R1  the swept level is S1 (R1) alone, not S2 / R2                            59% / 49%
// AFTER IT:
//   Lvp   lsCobSignal's low-volume pullback (one of the first 3 pullbacks, <= 1/3 of the wave's volume)
//   Warn CW  the first counter wave carries >= 0.8x of the Cob wave's volume (running: it can switch on, never off)
// WIN (the read box) - (1.0.1) from the nightly lra.cob3h_score's first run, 18 months of 3-min bars, 7 markets, 467 drawn CoBs:
//   [checks under 3, 3 of 3][waiting, Lvp, CW]: 52 / 54 / 33 and 55 / 63 / 32 (cases 188 / 111 / 33 and 279 / 174 / 44). "Waiting" is every CoB
//   of its group (its pullbacks are not known yet). On this longer sample the three checks together did NOT separate (55% vs 52%; the
//   one-year, 6-market test had 62% vs 50%); the Lvp after 3 of 3 (63%) and the counter-wave warning (32-33%) did. The nightly re-checks
//   every cell and reports one that is 10+ points off (20+ cases).
//   A rate of reaching the 1.33 extension before the origin, before costs - a read, not a trade result.
// NOT in the checklist (did not hold up on the year test): bearish breaks a prior pivot low, Cob bar 40+, climax at the origin, the
//   heaviest candle at the Cob pivot, a counter bar bigger than any since the origin.
// CB / SR (the price-chart overlay lsZzoComboMarks, labels only): CB = the wave's first candle of its colour (origin .. origin + 2) that
//   beats 40+ earlier candles of its colour on range or volume ("CB40", 99+ "CB99"); SR = the first candle of the run into the origin
//   that traded through the swept 3-hour level.
#ifndef COB_CHECK_H
#define COB_CHECK_H
#include "CobSignalLogic.h"
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace cck {
static const char* const COMBO_VERSION = "3.0.0";
static const char* const VERSION = "1.0.1";   // 1.0.1: the Win table from the nightly's first run (18 months, 7 markets, 467 drawn CoBs) replaces the one-year table; parseMarks (the overlay reads the pane's status, no engine of its own)
static const int ROW_MIN = 3, SMA_FAST = 50, SMA_SLOW = 100, CB_MIN = 40, CB_CAP = 150, CB_LOOK = 600, SR_LOOK = 60, ZZ_REV = 3;
static const double CW_X = 0.8;
static const int WIN[2][3] = { { 52, 54, 33 }, { 55, 63, 32 } };   // [checks < 3, 3 of 3][waiting, Lvp, CW] (1.0.1: the 18-month run)

struct Item {
    long long t = 0, tOrg = 0, tEnd = 0; int side = 0, idx = -1, p0 = -1, p1 = -1, cnt = -1; long sess = 0;
    std::string lv, tag; double lvPx = NAN;
    bool row3 = false, sma = false, first = false; int checks = 0;
    std::vector<long long> lvpT; long long lvpAt = 0;         // every shown Lvp's pivot bar END; the first one's
    bool cw = false, cwDone = false; double cwX = 0;           // the counter wave: >= CW_X, complete, its volume / the Cob wave's
    int win = 0;
    long long cbT = 0; int cbRank = 0; long long srT = 0;
};

inline bool up(const cobs::Bar& b) { return b.c >= b.o; }
inline int state(const Item& x) { return x.cw ? 2 : x.lvpAt ? 1 : 0; }

// every DRAWN CoB of the engine, oldest first
inline std::vector<Item> scan(const cobs::Engine& E)
{
    std::vector<Item> out; const std::vector<cobs::Bar>& B = E.B; const int n = (int)B.size();
    for (const cobs::Mark& m : E.marks) {
        if (!m.show || !m.zOk) continue;
        const int p0 = m.legStart, p1 = m.legEnd, j = m.idx, d = m.side;
        if (p0 < 0 || p1 <= p0 || p1 >= n || j < p1 || j >= n || (d != 1 && d != -1)) continue;
        Item x; x.t = m.t; x.side = d; x.idx = j; x.p0 = p0; x.p1 = p1; x.cnt = m.zCnt; x.tOrg = B[(size_t)p0].t; x.tEnd = B[(size_t)p1].t;
        if ((size_t)j < E.cls.size()) x.sess = E.cls[(size_t)j].sess;
        // the swept level: its name (the first when two), its price as it stood at the origin, a named key level on the same price
        x.lv = m.levels.substr(0, m.levels.find('/'));
        if ((size_t)p0 < E.hs.size()) { const cobs::HSnap& h = E.hs[(size_t)p0]; x.lvPx = x.lv == "R1" ? h.r1 : x.lv == "R2" ? h.r2 : x.lv == "S1" ? h.s1 : x.lv == "S2" ? h.s2 : NAN; }
        const bool htfLv = x.lv == "R1" || x.lv == "R2" || x.lv == "S1" || x.lv == "S2";
        if (!htfLv) { const auto s = E.S.find(x.sess); if (s != E.S.end()) { const auto kv = s->second.L.find(x.lv); if (kv != s->second.L.end()) x.lvPx = kv->second; } }   // (lsCobSignal 1.9.0) a named key level swept: its own price, no tag
        if (htfLv && std::isfinite(x.lvPx)) { const auto s = E.S.find(x.sess); double best = 2.5 * E.tick;
            if (s != E.S.end()) for (const auto& kv : s->second.L) { const double dd = std::fabs(kv.second - x.lvPx); if (dd < best) { best = dd; x.tag = klv::shownToken(kv.first); } } }
        // 3row
        { int run = 0, mx = 0; for (int q = p0 + 1; q <= p1; ++q) { const bool own = d > 0 ? up(B[(size_t)q]) : !up(B[(size_t)q]); run = own ? run + 1 : 0; mx = std::max(mx, run); } x.row3 = mx >= ROW_MIN; }
        // SMA side at the origin
        if (p0 >= SMA_SLOW - 1) { double a = 0, b = 0; for (int q = p0 - SMA_SLOW + 1; q <= p0; ++q) { b += B[(size_t)q].c; if (q > p0 - SMA_FAST) a += B[(size_t)q].c; }
            a /= SMA_FAST; b /= SMA_SLOW; x.sma = d > 0 ? a < b : a > b; }
        x.first = (x.lv == "S1" || x.lv == "R1") && m.levels.find("S2") == std::string::npos && m.levels.find("R2") == std::string::npos;   // (1.9.0) the levels may carry key names after the 3-hour ones
        x.checks = (x.row3 ? 1 : 0) + (x.sma ? 1 : 0) + (x.first ? 1 : 0);
        // Lvp (the signal's own)
        for (const cobs::LvpMark& L : E.lvp.marks) if (L.cobT == m.t && L.show && L.side == d) { x.lvpT.push_back(L.t); if (!x.lvpAt || L.t < x.lvpAt) x.lvpAt = L.t; }
        // the first counter wave (3-bar ZigZag from the Cob pivot): its volume so far / the Cob wave's
        { double wv = 0; for (int q = p0 + 1; q <= p1; ++q) wv += B[(size_t)q].v;
          // as the ZigZag walks it: the Cob wave ends when a bar takes out the 3 bars before it (the counter wave starts on that bar), the
          // counter wave extends on every new extreme and is complete when a bar takes out the 3 bars before it the other way
          int ext = -1; bool done = false;
          auto lowBefore = [&](int i) { double v = 1e300; for (int q = std::max(0, i - ZZ_REV); q < i; ++q) v = std::min(v, B[(size_t)q].l); return v; };
          auto highBefore = [&](int i) { double v = -1e300; for (int q = std::max(0, i - ZZ_REV); q < i; ++q) v = std::max(v, B[(size_t)q].h); return v; };
          for (int i = p1 + 1; i < n && !done; ++i) {
              const cobs::Bar& c = B[(size_t)i];
              if (ext < 0) { if (d > 0 ? c.l < lowBefore(i) : c.h > highBefore(i)) ext = i; continue; }
              if (d > 0 ? c.l < B[(size_t)ext].l : c.h > B[(size_t)ext].h) ext = i;
              else if (d > 0 ? c.h > highBefore(i) : c.l < lowBefore(i)) done = true;
          }
          double cv = 0; for (int q = p1 + 1; ext >= 0 && q <= ext && q < n; ++q) cv += B[(size_t)q].v;
          x.cwDone = done; x.cwX = wv > 0 ? cv / wv : 0; x.cw = wv > 0 && cv >= CW_X * wv; }
        x.win = WIN[x.checks >= 3 ? 1 : 0][state(x)];
        // CB: the wave's first candle of its colour, 40+
        { auto since = [&](int i, bool range) { int c = 0; const double v = range ? B[(size_t)i].h - B[(size_t)i].l : B[(size_t)i].v;
              for (int q = i - 1; q >= 0 && q > i - CB_LOOK; --q) { const bool own = d > 0 ? up(B[(size_t)q]) : !up(B[(size_t)q]); if (!own) continue;
                  if ((range ? B[(size_t)q].h - B[(size_t)q].l : B[(size_t)q].v) >= v) break;
                  if (++c >= CB_CAP) break; }
              return c; };
          for (int q = p0; q <= std::min(p0 + 2, p1); ++q) { const bool own = d > 0 ? up(B[(size_t)q]) : !up(B[(size_t)q]); if (!own) continue;
              const int rk = std::max(since(q, true), since(q, false)); if (rk >= CB_MIN) { x.cbT = B[(size_t)q].t; x.cbRank = rk; } break; } }
        // SR: the first candle of the run into the origin that traded through the swept level
        if (std::isfinite(x.lvPx)) { int q = p0; auto thru = [&](int i) { return d > 0 ? B[(size_t)i].l < x.lvPx : B[(size_t)i].h > x.lvPx; };
            if (thru(q)) { while (q - 1 >= 0 && q - 1 > p0 - SR_LOOK && thru(q - 1)) --q; x.srT = B[(size_t)q].t; } }
        out.push_back(x);
    }
    return out;
}

inline std::string clock12(long long t) {
    const long long dday = cobs::floorDiv(t, 86400); const int mn = (int)((t - dday * 86400) / 60); int h = mn / 60; const int mi = mn % 60; const bool pm = h >= 12; h = h % 12; if (h == 0) h = 12;
    char b[24]; std::snprintf(b, sizeof b, "%d:%02d %s", h, mi, pm ? "PM" : "AM"); return b;
}
inline std::string priceText(double px, double tick) {
    int dec = 0; double t = tick; while (dec < 6 && std::fabs(t - std::round(t)) > 1e-9) { t *= 10; ++dec; }
    char b[40]; std::snprintf(b, sizeof b, "%.*f", dec, px); std::string s = b; const size_t dot = s.find('.'); size_t e = dot == std::string::npos ? s.size() : dot;
    for (int k = (int)e - 3; k > (s[0] == '-' ? 1 : 0); k -= 3) s.insert((size_t)k, ",");
    return s;
}
// the read box: the newest drawn CoB of the newest bar's session. Row colour: 0 white, 1 the up colour, 2 the down colour.
struct Row { std::string k, v; int col = 0; };
struct Read { bool any = false; int dir = 0; std::vector<Row> rows; };
inline Read read(const cobs::Engine& E, const std::vector<Item>& items)
{
    Read R; const Item* x = nullptr; const long sess = E.cls.empty() ? 0 : E.cls.back().sess;
    for (size_t i = items.size(); i-- > 0;) if (items[i].sess == sess) { x = &items[i]; break; }
    auto row = [&](const std::string& k, const std::string& v, int c) { Row r; r.k = k; r.v = v; r.col = c; R.rows.push_back(r); };
    if (!x) { row("Cob", "-", 0); row("Lvl", "-", 0); row("Check", "-", 0); row("Lvp", "-", 0); row("Warn", "-", 0); row("Win", "-", 0); return R; }
    R.any = true; R.dir = x->side; char b[64];
    std::snprintf(b, sizeof b, "%s %s (%d)", x->side > 0 ? "Up" : "Dn", clock12(x->t).c_str(), x->cnt); row("Cob", b, x->side > 0 ? 1 : 2);
    row("Lvl", x->lv + (std::isfinite(x->lvPx) ? " " + priceText(x->lvPx, E.tick) : std::string()) + (x->tag.empty() ? std::string() : " " + x->tag), 0);
    { std::string v = std::to_string(x->checks) + "/3"; if (x->row3) v += " 3row"; if (x->sma) v += " SMA"; if (x->first) v += " " + x->lv; row("Check", v, x->checks >= 3 ? 1 : 0); }
    row("Lvp", x->lvpAt ? clock12(x->lvpAt) : std::string("-"), 0);
    if (x->cw) { std::snprintf(b, sizeof b, "CW %.1fx", x->cwX); row("Warn (1)", b, 2); } else row("Warn (0)", "none", 0);
    { std::snprintf(b, sizeof b, "%d%%", x->win); std::string v = b; if (state(*x) == 0) { std::snprintf(b, sizeof b, "  Lvp %d%%", WIN[x->checks >= 3 ? 1 : 0][1]); v += b; } row("Win", v, 0); }
    return R;
}
inline std::string statusText(const cobs::Engine& E, const std::vector<Item>& items)
{
    std::string o = std::string("COBCHECK,") + VERSION + ",Cobs = lsCobSignal " + cobs::VERSION + " rule (3-hour S / R sweep) drawn " + std::to_string(items.size()) +
        ",checks 3row (3+ in a row) SMA (50 vs 100 at the origin) S1/R1 (not S2/R2),warn CW (first counter wave >= 0.8x volume),win [checks<3 | 3/3][wait Lvp CW] 52 54 33 | 55 63 32 (lra.cob3h_score, 18 months: 1.33 before the origin, before costs)\n";
    const size_t a = items.size() > 12 ? items.size() - 12 : 0; char b[320];
    for (size_t i = a; i < items.size(); ++i) { const Item& x = items[i];
        std::snprintf(b, sizeof b, "CK,%lld,%s,%s,%.10g,%s,count %d,3row %d,SMA %d,first %d,checks %d,Lvp %lld,CW %d %.2f %s,win %d,CB %lld %d,SR %lld\n", x.t, x.side > 0 ? "BULL" : "BEAR", x.lv.c_str(),
                      std::isfinite(x.lvPx) ? x.lvPx : 0.0, x.tag.empty() ? "-" : x.tag.c_str(), x.cnt, x.row3 ? 1 : 0, x.sma ? 1 : 0, x.first ? 1 : 0, x.checks, x.lvpAt, x.cw ? 1 : 0, x.cwX, x.cwDone ? "done" : "running", x.win, x.cbT, x.cbRank, x.srT);
        o += b; }
    (void)E; return o;
}
// (1.0.1, independent review: three identical engines per chart) lsZzoComboMarks runs no engine: it reads the CK lines of the pane's status
// file (the last 12 drawn CoBs) - side, CB bar + rank, SR bar. Strict: a line that does not parse is skipped.
inline std::vector<Item> parseMarks(const std::string& text)
{
    std::vector<Item> out; size_t a = 0;
    while (a < text.size()) {
        size_t b = text.find('\n', a); if (b == std::string::npos) b = text.size();
        const std::string ln = text.substr(a, b - a); a = b + 1;
        if (ln.compare(0, 3, "CK,") != 0) continue;
        Item x; char* e = nullptr; x.t = std::strtoll(ln.c_str() + 3, &e, 10); if (!e || *e != ',' || x.t <= 0) continue;
        if (std::strncmp(e + 1, "BULL,", 5) == 0) x.side = 1; else if (std::strncmp(e + 1, "BEAR,", 5) == 0) x.side = -1; else continue;
        const size_t c = ln.find(",CB "), r = ln.find(",SR ");
        if (c == std::string::npos || r == std::string::npos || r < c) continue;
        char* e2 = nullptr; x.cbT = std::strtoll(ln.c_str() + c + 4, &e2, 10); if (!e2 || *e2 != ' ') continue;
        x.cbRank = (int)std::strtol(e2 + 1, &e2, 10); if (!e2 || *e2 != ',') continue;
        x.srT = std::strtoll(ln.c_str() + r + 4, &e2, 10); if (!e2 || (*e2 != 0 && *e2 != '\r')) continue;
        if (x.cbT < 0 || x.srT < 0 || x.cbRank < 0) continue;
        out.push_back(x);
    }
    return out;
}
} // namespace cck
#endif
