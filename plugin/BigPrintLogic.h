// BigPrintLogic.h (1.0.0, 2026-11-01) - SDK-free logic of lsBigPrint (BigPrint.cpp): ONE pane that replaces his BV_Since / BVGB / BVRB /
// BRB_Since / BGB_Since panes. Rassul 22:12-22:24 CT: "an indicator that replaces big print ... will show the biggest bid and the biggest
// ask in", "a small number above / below each bar (not candle) because its in its own indicator pane", "yes" to carrying the volume and
// green / red bar counts, "it should be on the same positive side (both the green and red bars)" (mockup cob_ba_mockup v2), "build".
//   per bar: a GREEN bar = volume bought at the ask, a RED bar = volume sold at the bid, side by side, both up from the baseline;
//   the number over the green bar = how many bars back this bar's BIGGEST ASK PRINT (the most volume bought at one price) is the biggest;
//   over the red bar = the same for the BIGGEST BID PRINT; shown from 10, bold and bright from 35, capped at 100;
//   "V42" at the top = the biggest-volume bar in 42 bars (from 35);
//   a BRIGHT green / red bar = a green / red candle whose range is the biggest in 35+ bars (his BGB_Since / BRB_Since), dim otherwise.
// "Biggest in N bars" = the platform's own "since" convention (CobImpulseLogic::sinceBigger, BGB_Since / BV_Since): N = bars back to the
// last STRICTLY bigger bar, capped at 100; an unknown value (no VAP / volume) stops the count. Closed bars are fixed; the forming bar
// updates until it closes.
#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace bpr {

static const char* const VERSION = "1.0.1";   // (1.0.1, Rassul 2026-11-02 05:02 / 05:23 CT "the big print indicator doesn't match the signal") the ask / bid numbers = lsBigBidAsk's "biggest in N" box exactly: split rows of one price merged, N = the run of earlier candles whose biggest is SMALLER (an equal one stops it), cap 50   // was "1.0.0":
static const int CAP = 100, SHOW = 10, STRONG = 35, MAX_BARS = 20000;
static const int PRINT_CAP = 50;   // (1.0.1) lsBigBidAsk's cap (bbaset::CAP)

struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0, v = 0; bool vap = false; double buy = 0, sell = 0, bigAsk = 0, bigBid = 0; };
struct Counts { int ask = 0, bid = 0, vol = 0, green = 0, red = 0; };

// N = bars back to the last STRICTLY bigger bar (the platform's BV_Since / BGB_Since: the bar before bigger = 1); none = i + 1;
// capped at CAP. ok() false on an earlier bar = unknown: stops the count there (never counted as smaller); skip() = not compared
// (his BGB_Since compares green bars with green bars only: other bars are skipped, as trade_def._since_bigger's NaN rows).
template <class Val, class Ok, class Skip>
inline int since(const std::vector<Bar>& B, int i, const Bar& cur, Val val, Ok ok, Skip skip)
{
    if (!ok(cur) || skip(cur)) return 0;
    const double x = val(cur);
    for (int k = i - 1; k >= 0 && i - k < CAP; --k) { const Bar& b = B[(size_t)k]; if (skip(b)) continue; if (!ok(b) || val(b) > x) return i - k; }
    return std::min(CAP, i + 1);
}
// (1.0.1) THE PRINT COUNT = lsBigBidAsk::backCount: how many earlier candles in a row have a SMALLER biggest print of the same side
// (an equal or bigger one ends the run; a candle with no volume at price ends it too), capped at PRINT_CAP. No print = 0.
template <class Val>
inline int printRun(const std::vector<Bar>& B, int i, const Bar& cur, Val val)
{
    if (!cur.vap) return 0;
    const double x = val(cur); if (!(x > 0)) return 0;
    int n = 0;
    for (int k = i - 1; k >= 0 && n < PRINT_CAP; --k) { const Bar& b = B[(size_t)k]; if (!b.vap || val(b) >= x) break; ++n; }
    return n;
}
inline Counts countsAt(const std::vector<Bar>& B, int i, const Bar& cur)   // cur = B[i] or the forming bar (i = B.size())
{
    Counts c;
    auto none = [](const Bar&) { return false; };
    c.ask = printRun(B, i, cur, [](const Bar& b) { return b.bigAsk; });   // (1.0.1) = the lsBigBidAsk box
    c.bid = printRun(B, i, cur, [](const Bar& b) { return b.bigBid; });
    c.vol = since(B, i, cur, [](const Bar& b) { return b.v; }, [](const Bar& b) { return b.v > 0; }, none);
    auto rng = [](const Bar& b) { return b.h - b.l; }; auto okR = [](const Bar& b) { return b.h >= b.l; };
    c.green = since(B, i, cur, rng, okR, [](const Bar& b) { return !(b.c > b.o); });
    c.red = since(B, i, cur, rng, okR, [](const Bar& b) { return !(b.c < b.o); });
    return c;
}

struct Engine {
    std::vector<Bar> B; std::vector<Counts> C; long long trimmed = 0;
    void reset() { B.clear(); C.clear(); }
    void addBar(const Bar& b) {
        if ((int)B.size() >= MAX_BARS) { const size_t cut = (size_t)MAX_BARS / 4; B.erase(B.begin(), B.begin() + (long)cut); C.erase(C.begin(), C.begin() + (long)cut); trimmed += (long long)cut; }
        C.push_back(countsAt(B, (int)B.size(), b)); B.push_back(b);
    }
};

// ---- the drawing (SDK-free)
struct View { int l = 0, t = 0, r = 800, b = 200, first = 0, last = 0; double bw = 6; std::function<double(int)> X; std::function<int(const std::string&, bool)> textW;
              bool haveForm = false; Bar form; Counts formC; int formIdx = -1; };
struct Prim { enum K { FILL, TEXT } k = FILL; int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned col = 0; bool bold = false; std::string text; };
static const unsigned C_G = 0x0022C55E, C_R = 0x00EF4444, C_GD = 0x00145A32, C_RD = 0x006E2328, C_W = 0x00E5E7EB, C_BASE = 0x00374151, C_GT = 0x0016803C, C_RT = 0x00A33232;
struct Scene { std::vector<Prim> p; int numbers = 0, vlabels = 0, bars = 0; double scaleMax = 0; };
inline Scene layout(const Engine& E, const View& v)
{
    Scene S;
    const int n = (int)E.B.size();
    auto barAt = [&](int j, Bar& b, Counts& c) { if (j >= 0 && j < n) { b = E.B[(size_t)j]; c = E.C[(size_t)j]; return true; } if (v.haveForm && j == v.formIdx) { b = v.form; c = v.formC; return true; } return false; };
    // the scale: the 95th percentile of the visible bars (one settlement / news bar must not flatten the rest); taller bars are clipped
    std::vector<double> vals;
    for (int j = v.first; j <= v.last; ++j) { Bar b; Counts c; if (barAt(j, b, c) && b.vap) { vals.push_back(b.buy); vals.push_back(b.sell); } }
    double mx = 0;
    if (!vals.empty()) { std::sort(vals.begin(), vals.end()); mx = vals.size() >= 20 ? vals[(size_t)((vals.size() - 1) * 0.95)] : vals.back(); }
    S.scaleMax = mx; if (!(mx > 0)) return S;
    const int top = v.t + 16, base = v.b - 2;                   // the V row at the top, the baseline at the bottom
    const int avail = base - top - 26;                          // room for two number rows above the tallest bar
    if (avail < 10) return S;
    const double hw = std::max(1.0, std::min(v.bw * 0.4, 8.0));   // each bar's width (two per slot)
    double lastV = -1e9;
    for (int j = v.first; j <= v.last; ++j) {
        Bar b; Counts c; if (!barAt(j, b, c)) continue;
        const double x = v.X(j); if (x < v.l - 20 || x > v.r + 20) continue;
        if (b.vap) {
            const int hb = (int)std::lround(avail * std::min(1.0, b.buy / mx)), hs = (int)std::lround(avail * std::min(1.0, b.sell / mx));
            Prim g; g.x0 = (int)std::lround(x - hw); g.x1 = (int)std::lround(x) - 1; g.y0 = base - hb; g.y1 = base; g.col = c.green >= STRONG ? C_G : C_GD;
            Prim r; r.x0 = (int)std::lround(x); r.x1 = (int)std::lround(x + hw) - 1; r.y0 = base - hs; r.y1 = base; r.col = c.red >= STRONG ? C_R : C_RD;
            if (g.x1 < g.x0) g.x1 = g.x0; if (r.x1 < r.x0) r.x1 = r.x0;
            if (hb > 0) S.p.push_back(g); if (hs > 0) S.p.push_back(r); ++S.bars;
            // the numbers: over each bar; when both would sit at the same height the higher one moves up a row
            const int h = 12; int ya = base - hb - h - 1, yb = base - hs - h - 1;
            const bool sa = c.ask >= SHOW, sb = c.bid >= SHOW;
            if (sa && sb && std::abs(ya - yb) < h) { if (ya <= yb) ya = yb - h; else yb = ya - h; }
            auto num = [&](int val, int y, bool ask) {
                const std::string s = std::to_string(val); const bool strong = val >= STRONG;
                const int w = v.textW ? v.textW(s, strong) : (int)s.size() * 6;
                Prim t; t.k = Prim::TEXT; t.x0 = (int)std::lround(x - w / 2.0); t.x1 = t.x0 + w; t.y0 = std::max(top, y); t.y1 = t.y0 + h; t.bold = strong;
                t.col = ask ? (strong ? C_G : C_GT) : (strong ? C_R : C_RT); t.text = s; S.p.push_back(t); ++S.numbers;
            };
            if (sa) num(c.ask, ya, true);
            if (sb) num(c.bid, yb, false);
        }
        if (c.vol >= STRONG && x - lastV > 30) {
            lastV = x; const std::string s = "V" + std::to_string(c.vol); const int w = v.textW ? v.textW(s, true) : (int)s.size() * 7;
            Prim t; t.k = Prim::TEXT; t.x0 = (int)std::lround(x - w / 2.0); t.x1 = t.x0 + w; t.y0 = v.t + 2; t.y1 = t.y0 + 12; t.col = C_W; t.bold = true; t.text = s; S.p.push_back(t); ++S.vlabels;
        }
    }
    Prim bl; bl.x0 = v.l; bl.x1 = v.r; bl.y0 = base; bl.y1 = base; bl.col = C_BASE; S.p.insert(S.p.begin(), bl);
    return S;
}

inline std::string statusText(const Engine& E, const std::string& mkt, const std::string& sym, int spb, long long noVap, const Scene& last)
{
    std::string o = std::string("VERSION,") + VERSION + "\nMARKET," + mkt + "\nSYMBOL," + sym + "\nBAR_SECONDS," + std::to_string(spb) + "\nBARS," + std::to_string(E.B.size()) +
                    "\nNO_VAP," + std::to_string(noVap) + "\nRULES,ask / bid = lsBigBidAsk biggest in N (smaller prints in a row; shown >= 10, bold >= 35, cap 50); V >= 35 (cap 100); bright bar = BGB / BRB >= 35\n";
    const int n = (int)E.B.size();
    if (n > 0) { const Counts& c = E.C.back(); const Bar& b = E.B.back();
        char buf[200]; std::snprintf(buf, sizeof buf, "LAST,buy %.0f,sell %.0f,ask in %d,bid in %d,V in %d,BGB %d,BRB %d\n", b.buy, b.sell, c.ask, c.bid, c.vol, c.green, c.red); o += buf; }
    o += "DRAWN,bars " + std::to_string(last.bars) + ",numbers " + std::to_string(last.numbers) + ",V " + std::to_string(last.vlabels) + "\n";
    return o;
}

} // namespace bpr
