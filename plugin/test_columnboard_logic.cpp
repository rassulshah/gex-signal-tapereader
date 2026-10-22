// test_columnboard_logic.cpp -- (CR5200) the SDK-free logic of the right-side columns (ColumnBoard.h 1.0.0) and of the Dealer Profile's
// column texts / level badges with liquidity (DealerColumnsLogic.h 1.0.0). Rassul approved 2026-10-14 11:00 CT (mockup-columns.png),
// 11:02 (Liquidity node text), 11:39 (level badges with liquidity, design B).
// Build (Linux, from plugin/): g++ -std=c++17 -g -Wall -Wextra -Werror -Wno-misleading-indentation -fsanitize=address,undefined -I. test_columnboard_logic.cpp -o t && ./t
#include "ColumnBoard.h"
#include "DealerColumnsLogic.h"
#include <cstdio>
#include <string>
#include <vector>
#include <set>
#include <cstdlib>

static int PASS = 0, FAIL = 0;
static void check(bool c, const std::string& m) { if (c) ++PASS; else { ++FAIL; std::printf("FAIL %s\n", m.c_str()); } }
static int w6(const std::string& s) { return (int)s.size() * 6; }

static void ranges()
{
    check(cb::range(cb::LIQ).lo == 20 && cb::range(cb::LIQ).hi == 200 && cb::range(cb::LIQ).def == 60, "Liquidity 20-200, default 60");
    check(cb::range(cb::LEVELS).lo == 30 && cb::range(cb::LEVELS).hi == 250 && cb::range(cb::LEVELS).def == 78, "Levels 30-250 (ColumnBoard 1.3.0: 150 + the DP 2.19.0 % block), default 78 (11:39 CT)");
    check(cb::range(cb::DELTA).lo == 40 && cb::range(cb::DELTA).hi == 250 && cb::range(cb::DELTA).def == 80, "Delta 40-250, default 80");
    check(cb::range(cb::DEALER).lo == 40 && cb::range(cb::DEALER).hi == 250 && cb::range(cb::DEALER).def == 80, "Dealer 40-250, default 80");
    int v = 0;
    check(cb::parseWidth(cb::DELTA, "80", v) && v == 80 && cb::parseWidth(cb::DELTA, "250", v) && !cb::parseWidth(cb::DELTA, "251", v) && !cb::parseWidth(cb::DELTA, "39", v), "width parse: range");
    const char* bad[] = { "", "-80", "+80", " 80", "80 ", "8e1", "0x50", "80.0", "nan", "99999", "1e3" };
    bool any = false; for (const char* b : bad) if (cb::parseWidth(cb::DELTA, b, v)) any = true;
    check(!any, "width parse: garbage refused");
    check(cb::cleanWidth(cb::LIQ, 5) == 60 && cb::cleanWidth(cb::LIQ, 999) == 60 && cb::cleanWidth(cb::LIQ, 20) == 20 && cb::cleanWidth(cb::LEVELS, -1) == 78, "cleanWidth: out of range = default");
}

static cb::In inp(int paneW, bool lv, bool dl, bool dg, int lw, int dw, int gw)
{
    cb::In in; in.paneL = 0; in.paneT = 0; in.paneB = 800; in.axisR = paneW; in.levels = lv; in.delta = dl; in.dealer = dg; in.levelsW = lw; in.deltaW = dw; in.dealerW = gw; return in;
}
static void layouts()
{
    // the approved default: 20 gap | Levels 78 | Delta 80 | Dealer 80 | axis
    cb::Layout L = cb::layout(inp(1600, true, true, true, 78, 80, 80));
    check(L.ok && !L.squeezed && L.dealer.l == 1520 && L.dealer.r == 1600 && L.delta.l == 1440 && L.delta.r == 1520 && L.levels.l == 1362 && L.levels.r == 1440, "default layout: boxes left to right from the axis");
    check(L.priceRight == 1342 && L.margin == 258 && cb::columnsClean(L), "default: price ends 20 px before Levels; margin = 20 + 78 + 80 + 80 = 258");
    // min / max / mixed widths
    int bad = 0, n = 0;
    const int LW[] = { 30, 78, 150 }, DW[] = { 40, 80, 250 }, GW[] = { 40, 80, 250 };
    for (int a : LW) for (int b : DW) for (int c : GW) for (int pw : { 900, 1200, 1600, 2560 }) {
        cb::Layout X = cb::layout(inp(pw, true, true, true, a, b, c)); ++n;
        const int sum = X.w[cb::LEVELS] + X.w[cb::DELTA] + X.w[cb::DEALER];
        if (!X.ok || !cb::columnsClean(X) || X.margin != cb::GAP_PX + sum || X.dealer.r != pw || X.levels.l - cb::GAP_PX != X.priceRight) ++bad;
        if (!X.squeezed && (X.w[cb::LEVELS] != a || X.w[cb::DELTA] != b || X.w[cb::DEALER] != c)) ++bad;
        if (X.priceRight - 0 < cb::MIN_PRICE_PX && !X.squeezed) ++bad;
    }
    check(bad == 0, "min / max / mixed widths x 4 chart widths (" + std::to_string(n) + "): no overlap, margin = gap + columns, abutting");
    // absent columns: no Delta Profile, no Dealer Profile, levels only (an hourly chart)
    cb::Layout A = cb::layout(inp(1600, true, false, true, 78, 80, 80));
    check(A.delta.w() == 0 && A.levels.r == A.dealer.l && A.margin == 20 + 78 + 80 && cb::columnsClean(A), "no Delta Profile: Levels sits next to Dealer, margin without it");
    cb::Layout B = cb::layout(inp(1600, false, true, false, 78, 80, 80));
    check(B.dealer.w() == 0 && B.delta.r == 1600 && B.margin == 100 && B.priceRight == 1500, "Delta alone: at the axis, margin 20 + 80");
    cb::Layout C = cb::layout(inp(1600, false, false, false, 78, 80, 80));
    check(C.margin == 0 && C.priceRight == 1600, "no columns: no margin");
    // a narrow chart: shrink in proportion, never below the minimums; no room at all = not drawn (ok false)
    cb::Layout N = cb::layout(inp(500, true, true, true, 150, 250, 250));
    check(N.squeezed && N.ok && N.priceRight >= cb::MIN_PRICE_PX - 1 && N.w[cb::LEVELS] >= 30 && N.w[cb::DELTA] >= 40 && N.w[cb::DEALER] >= 40 && cb::columnsClean(N), "narrow chart: squeezed, price keeps 160 px, minimums kept");
    cb::Layout T = cb::layout(inp(150, true, true, true, 78, 80, 80));
    check(!T.ok, "tiny chart: not drawn");
    // garbage widths in = defaults
    cb::Layout G = cb::layout(inp(1600, true, true, true, -5, 99999, 0));
    check(G.w[cb::LEVELS] == 78 && G.w[cb::DELTA] == 80 && G.w[cb::DEALER] == 80, "garbage widths: defaults");
}

static void margins()
{
    check(cb::marginValue(258, 5, cb::BARS) == 52 && cb::marginValue(260, 5, cb::BARS) == 52 && cb::marginValue(261, 5, cb::BARS) == 53, "bars: rounded up (258 px at 5 px/bar = 52 bars)");
    check(cb::marginValue(258, 5, cb::PIXELS) == 258 && cb::marginValue(-3, 5, cb::PIXELS) == 0 && cb::marginValue(100, 0, cb::BARS) == 100, "pixels as is; bad ppb = 1");
    int under = 0; for (int px = 0; px < 600; ++px) for (int ppb = 1; ppb < 40; ++ppb) if (cb::marginPx(cb::marginValue(px, ppb, cb::BARS), ppb, cb::BARS) < px) ++under;
    check(under == 0, "the margin in bars never ends short of the columns (every px x ppb)");
    // the unit probe: IRT really in pixels (gap ~= value), or in bars (gap ~= value * ppb); ambiguous ignored
    cb::UnitProbe P; check(P.units == cb::BARS, "default unit: bars (the safe side)");
    P.observe(258, 5, 259); check(P.units == cb::BARS, "one pixel vote is not enough");
    P.observe(258, 5, 257); check(P.units == cb::PIXELS, "two agreeing votes: pixels");
    P.observe(52, 5, 261); P.observe(52, 5, 262); check(P.units == cb::BARS, "then two bar votes: bars");
    cb::UnitProbe Q; Q.observe(0, 5, 0); Q.observe(40, 1, 40); Q.observe(40, 1, 40); Q.observe(30, 6, 90); Q.observe(30, 6, 90);
    check(Q.units == cb::BARS && Q.votePx == 0, "value 0, 1 px per bar, or a gap matching neither: ignored");
    // the controller: grows at once, shrinks only after 60 s; never asks twice in 2 s; no change = nothing
    cb::MarginCtl M;
    check(M.want(258, 5, cb::BARS, 0, 1000) == 52, "first: ask 52 bars");
    check(M.want(258, 5, cb::BARS, 0, 1500) == -1, "asked 0.5 s ago: wait");
    check(M.want(258, 5, cb::BARS, 52, 2000) == -1, "the chart has it: nothing");
    check(M.want(338, 5, cb::BARS, 52, 3000) == 68, "a wider column: grows at once");
    check(M.want(258, 5, cb::BARS, 68, 4000) == -1 && M.want(258, 5, cb::BARS, 68, 50000) == -1, "narrower: held (no shake)");
    check(M.want(258, 5, cb::BARS, 68, 64001) == 52, "narrower for 60 s: shrinks");
    int flips = 0, last = -2; cb::MarginCtl F; int cur = 0;
    for (int i = 0; i < 400; ++i) { const int need = (i % 2) ? 258 : 338; const int v = F.want(need, 5, cb::BARS, cur, 1000 + i * 100); if (v >= 0) { cur = v; if (v != last) ++flips; last = v; } }
    check(flips <= 1, "a need flipping every draw (two charts / a Delta Profile coming and going): one change, no shake (" + std::to_string(flips) + ")");
}

static void board()
{
    const std::string sym = "ESZ6", ch = "ES LTF 3m"; const long spb = 180;
    cb::publish(cb::DELTA, sym, spb, ch, 1000, 80);
    int w = 0; long long ms = -1;
    check(cb::peek(cb::DELTA, sym, spb, ch, w, ms) && w == 80 && ms == 1000, "publish / peek a width");
    check(!cb::peek(cb::DELTA, sym, spb, "another chart", w, ms) && !cb::peek(cb::DELTA, "NQZ6", spb, ch, w, ms) && !cb::peek(cb::DELTA, sym, 60, ch, w, ms), "per chart, symbol and bar size");
    cb::publish(cb::DELTA, sym, spb, ch, 1001, 9999);
    check(cb::peek(cb::DELTA, sym, spb, ch, w, ms) && w == 80, "a garbage width is published as the default");
    bb::bbSet(bb::key(cb::prefix(cb::DEALER), sym, spb, ch), "123|abc");
    check(!cb::peek(cb::DEALER, sym, spb, ch, w, ms), "a garbled entry is not read");
    cb::Peers P; cb::publish(cb::DEALER, sym, spb, ch, 2000, 120); cb::publish(cb::LEVELS, sym, spb, ch, 2000, 78);
    P.step(sym, spb, ch, 2000, 1u << cb::DELTA);
    check(P.on[cb::DEALER] && P.w[cb::DEALER] == 120 && P.on[cb::LEVELS] && P.w[cb::LEVELS] == 78 && !P.on[cb::DELTA], "peers: the others' widths, never my own");
    // the other plugin removed: its entry stops moving -> gone after 3 of my draws once 5 s old
    bool gone = false; for (int i = 1; i <= 4; ++i) { P.step(sym, spb, ch, 2000 + 6000 * i, 1u << cb::DELTA); if (!P.on[cb::DEALER]) gone = true; }
    check(gone && P.w[cb::DEALER] == 0, "a removed Dealer Profile closes its column within seconds");
    cb::clear(cb::DEALER, sym, spb, ch); check(!cb::peek(cb::DEALER, sym, spb, ch, w, ms), "clear removes the entry");
    cb::clear(cb::DELTA, sym, spb, ch); cb::clear(cb::LEVELS, sym, spb, ch);
}

static void stacking()
{
    // many close levels: 30 lines within 20 px, badges 15 px tall -> stacked in price order, never overlapping, inside the pane
    std::vector<cb::Item> v; for (int i = 0; i < 30; ++i) { cb::Item it; it.want = 400 + (i * 7) % 20; it.h = 15; v.push_back(it); }
    const int drop = cb::stack(v, 0, 800, 400);
    int shown = 0, ov = 0, outside = 0;
    for (size_t i = 0; i < v.size(); ++i) { if (!v[i].shown) continue; ++shown;
        const int t = v[i].y - 7, b = t + 15; if (t < 0 || b > 800) ++outside;
        for (size_t j = i + 1; j < v.size(); ++j) if (v[j].shown) { const int t2 = v[j].y - 7, b2 = t2 + 15; if (t < b2 && t2 < b) ++ov; } }
    check(drop == 0 && shown == 30 && ov == 0 && outside == 0, "30 close levels: all stacked, no overlap, inside the pane");
    // order kept (a higher line keeps a higher badge)
    bool ordered = true; for (size_t i = 0; i < v.size(); ++i) for (size_t j = 0; j < v.size(); ++j) if (v[i].want < v[j].want && v[i].y > v[j].y) ordered = false;
    check(ordered, "badges keep the levels' price order");
    // too many for the pane: the farthest from price are dropped (counted), never overlapped
    std::vector<cb::Item> w(80); for (int i = 0; i < 80; ++i) { w[i].want = i * 10; w[i].h = 17; }
    const int d2 = cb::stack(w, 0, 600, 300);
    int ov2 = 0; for (size_t i = 0; i < w.size(); ++i) for (size_t j = i + 1; j < w.size(); ++j) if (w[i].shown && w[j].shown && std::abs(w[i].y - w[j].y) < 17) ++ov2;
    check(d2 > 0 && ov2 == 0 && w[30].shown, "too many badges: some dropped (the far ones), none overlap, the one at price kept (" + std::to_string(d2) + " dropped)");
    // levels at the very edges are pulled inside
    std::vector<cb::Item> e(2); e[0].want = -5; e[0].h = 15; e[1].want = 805; e[1].h = 15; cb::stack(e, 0, 800, 400);
    check(e[0].y - 7 >= 0 && e[1].y - 7 + 15 <= 800, "badges at the pane edges stay inside");
    // a stable result: the same input gives the same output (no flicker between draws)
    std::vector<cb::Item> a1 = v, a2 = v; cb::stack(a1, 0, 800, 400); cb::stack(a2, 0, 800, 400);
    bool same = true; for (size_t i = 0; i < a1.size(); ++i) if (a1[i].y != a2[i].y || a1[i].shown != a2[i].shown) same = false;
    check(same, "deterministic: no flicker");
    // names that fit the column
    check(cb::fitName("LonLO", 40, w6) == "LonLO" && cb::fitName("ONH/AsiaHI", 30, w6) == "ONH+" && cb::fitName("PFH/PDH", 100, w6) == "PFH/PDH", "fitName: name, else first + '+'");
    check(cb::fitName("AsiaHI", 20, w6) == "Asi" && cb::fitName("AsiaHI", 5, w6) == "" && cb::fitName("", 50, w6) == "", "fitName: cut to fit, never garbage");
}

static void dealerTexts()
{
    check(dcol::nodeText(true, 1234) == "1.2K lots" && dcol::nodeText(true, -980) == "980 lots" && dcol::nodeText(true, -12755) == "12.8K lots", "node text: contracts + lots");
    check(dcol::nodeText(false, 500).empty() && dcol::nodeText(true, 0.2).empty() && dcol::nodeText(true, std::nan("")).empty() && dcol::nodeText(true, 1.0 / 0.0).empty(), "node text: none when missing / zero / NaN / inf (no garbage)");
    check(dcol::textPx(10) == 14 && dcol::textPx(9) == 12, "text height = pt x 4/3 px");
    check(dcol::nodeTextFits(40, 46, 16, 10) && !dcol::nodeTextFits(40, 45, 16, 10) && !dcol::nodeTextFits(40, 80, 15, 10) && !dcol::nodeTextFits(0, 80, 20, 10), "node text fits only inside (3 px each side, the text height + 1 px above and below)");
    check(dcol::nodeShown(3e6, 100e6) && !dcol::nodeShown(2.9e6, 100e6) && !dcol::nodeShown(0, 100e6) && !dcol::nodeShown(5, 0), "a strike under 3% of the biggest wall is not drawn (no slivers)");
    check(dcol::dropHourly("PDH/HrHI") == "PDH" && dcol::dropHourly("HrHI").empty() && dcol::dropHourly("HrLO/PFL/PDL") == "PFL/PDL" && dcol::dropHourly("ONH/AsiaHI") == "ONH/AsiaHI", "HrHI / HrLO never shown (removed in 2.10.0)");
    check(dcol::compactName("CR 0D / GW 0D STALE 24m") == "CR0/GW0" && dcol::compactName("CR0 STALE 24m") == "CR0" && dcol::compactName("HVL 0D") == "HVL" && dcol::compactName("1D Max") == "1DMax" && dcol::compactName("PS 0D") == "PS0", "compact badge names");
    check(dcol::staleSuffix("CR0 STALE 24m") == "STALE 24m" && dcol::staleSuffix("CR0").empty(), "STALE goes to the hover");
    dpaudit::Hover h = dcol::levelHover("ES", "LonLO", "7866.25", "33%  /  50%", "900 contracts resting bids (2.7x the book behind)", false, false);
    bool odds = false, liq = false, how = false, ex = false, what = false;
    for (const std::string& l : h.lines) { if (l.find("33%  /  50%") != std::string::npos) odds = true; if (l.find("900 contracts") != std::string::npos) liq = true;
        if (l.rfind("How to use", 0) == 0) how = true; if (l.rfind("Example: price sweeps", 0) == 0) ex = true; if (l.find("London session's low") != std::string::npos) what = true; }
    check(h.title == "Will ES react at LonLO 7866.25?" && odds && liq && how && ex && what, "level hover: question, what, odds, liquidity, how, example");
    dpaudit::Hover h2 = dcol::levelHover("ES", "CR0 STALE 24m", "7870.00", "1h 68%?", "", true, true);
    bool stale = false, pick = false, short_ = false, unproven = false;
    for (const std::string& l : h2.lines) { if (l.find("STALE 24m") != std::string::npos) stale = true; if (l.find("pick") != std::string::npos) pick = true;
        if (l.rfind("Example: price pokes above", 0) == 0) short_ = true; if (l.find("not proven") != std::string::npos) unproven = true; }
    check(stale && pick && short_ && unproven && h2.title.find("STALE") == std::string::npos, "level hover: STALE / age, pick, a short above price, '?' explained");
    dpaudit::NodeX x; x.k = 7900; x.absG = 27e6; x.hasD = true; x.d = -8591; x.dUsd = -3.39e9;
    dpaudit::Hover n = dcol::nodeHover("ES", x, 26e6, 40e6, 7857, true);
    check(!n.lines.empty() && n.lines[0].find("\"8.6K lots\" = futures dealers must SELL 8.6K ES contracts (about $3.4B)") != std::string::npos, "node hover: the node text + its $");   // (DP 2.16.0, Rassul 2026-10-16 22:16 "just give the number followed by lots") the node text is "8.6K lots", no B / S letter
    bool teal = false; for (const std::string& l : n.lines) if (l.rfind("Teal = MenthorQ model shows + net gamma here (vendor model, ", 0) == 0 && l.find("Not a prediction it holds.") != std::string::npos) teal = true;
    check(teal, "(DP 2.12.0) node hover: 'Teal = MenthorQ model shows + net gamma here (vendor model, data N min old). Not a prediction it holds.'");
}

static void liquidityBadges()
{
    const long long now = 1791990000;
    const std::string good = "LIQL,1," + std::to_string(now) + ",0.25,2.5,2.1,4\nL,31465,S,900,2.7\nL,31480,R,310,1.1\nL,31482,R,0,3\nL,x,S,5,1\nL,31490,Q,5,1\nL,31491.5,S,5,1\nL,31492,S,nan,1\n";
    dcol::LiqLevels L; std::string why;
    check(dcol::parseLiqLevels(good, now + 3, L, why) && L.rows.size() == 2 && L.onX == 2.5 && L.offX == 2.1, "LiqLevels parsed; zero / garbled rows dropped");
    const dcol::LiqRow* r = dcol::liqRowAt(L, 7866.25);
    check(r && r->bid && r->n == 900 && r->bx == 2.7, "the row at a level's price (tick grid)");
    check(dcol::liqRowAt(L, 7866.30) && !dcol::liqRowAt(L, 7866.40) && dcol::liqRowAt(L, 7870.0) && !dcol::liqRowAt(L, 7871.0), "a price is matched on the nearest tick; no size there = none");
    check(!dcol::parseLiqLevels(good, now + 21, L, why) && why == "stale", "a file over 20 s old = no data");
    check(!dcol::parseLiqLevels("garbage", now, L, why) && !dcol::parseLiqLevels("", now, L, why) && !dcol::parseLiqLevels("LIQL,1,now,0.25,2.5,2.1,4", now, L, why), "garbage headers refused");
    check(!dcol::parseLiqLevels("LIQL,1," + std::to_string(now) + ",0.25,2.0,2.4,4\n", now, L, why), "an off line above the on line refused");
    check(dcol::lotsK(310) == "310" && dcol::lotsK(900) == "900" && dcol::lotsK(1234) == "1.2K" && dcol::lotsK(1500) == "1.5K" && dcol::lotsK(15400) == "15K" && dcol::lotsK(0).empty() && dcol::lotsK(-4).empty(), "contracts: 310 / 1.2K / 15K, never 0");
    check(dcol::liqBlockText(900, true) == "HL? 900" && dcol::liqBlockText(310, false) == "310" && dcol::liqBlockText(0, true).empty(), "(DP 2.12.0) block text: HL? 900 / 310 / none");
    // HL hysteresis: on at 2.5x, held while >= 2.1x, and at least 30 s; ratios around 2.5 never flicker
    dcol::HLState h; long long t = 0; int flips = 0; bool prev = false;
    const double seq[] = { 2.4, 2.5, 2.45, 2.55, 2.49, 2.51, 2.3, 2.2, 2.6, 2.4 };
    for (int i = 0; i < 200; ++i) { const bool on = dcol::hlStep(h, true, true, seq[i % 10], 2.5, 2.1, t); if (on != prev) ++flips; prev = on; t += 500; }
    check(flips == 1 && prev, "HL around 2.5x for 100 s: on once, never flickers off (" + std::to_string(flips) + " changes)");
    dcol::HLState g; check(dcol::hlStep(g, true, true, 2.6, 2.5, 2.1, 0), "HL on at 2.6x");
    check(dcol::hlStep(g, true, true, 1.5, 2.5, 2.1, 10000), "under 2.1x after 10 s: still on (30 s hold)");
    check(!dcol::hlStep(g, true, true, 1.5, 2.5, 2.1, 30000), "under 2.1x after 30 s: off");
    check(!dcol::hlStep(g, true, true, 2.4, 2.5, 2.1, 31000) && dcol::hlStep(g, true, true, 2.5, 2.5, 2.1, 32000), "back on only at 2.5x");
    check(!dcol::hlStep(g, true, false, 3.0, 2.5, 2.1, 33000) || g.side == -1, "price crossed the level (bids -> offers): starts over on the new side");
    dcol::HLState k; dcol::hlStep(k, true, true, 3.0, 2.5, 2.1, 0);
    check(!dcol::hlStep(k, false, true, 3.0, 2.5, 2.1, 1000) && !dcol::hlStep(k, true, true, -1, 2.5, 2.1, 2000), "no data / no book behind = off (name-only or a grey block, never HL)");
    dcol::HLState s; dcol::hlStep(s, true, true, 3.0, 2.5, 2.1, 0); const bool flip = dcol::hlStep(s, true, false, 3.0, 2.5, 2.1, 100);
    check(flip && s.side == -1, "a side flip with offers at 3.0x: HL again on the offer side (red)");
}

int main()
{
    ranges(); layouts(); margins(); board(); stacking(); dealerTexts(); liquidityBadges();
    std::printf("RESULT %d passed, %d failed\n", PASS, FAIL);
    return FAIL ? 1 : 0;
}
