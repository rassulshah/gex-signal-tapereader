#ifndef LS_DEALER_COLUMNS_LOGIC_H
#define LS_DEALER_COLUMNS_LOGIC_H
// DealerColumnsLogic.h 1.0.0 (2026-10-14, CR5200) -- lsDealerProfile 2.11.0's column texts and hovers, SDK-free (tested by
// test_dealerprofile_2110.cpp). Rassul approved 2026-10-14 11:00 CT (mockup-columns.png):
//   * the Dealer Profile is one-sided, every bar anchored on the column's right edge and growing left toward price: + teal
//     (dealers net LONG gamma, a wall), - orange (net SHORT gamma, fuel)
//   * inside each node: what futures dealers must trade to stay hedged at that price, "B 1.2K" (buy 1,200 contracts) / "S 980"
//     (sell 980) - hidden when the node is too short for it or the hedge is missing (no garbage values); the hover adds the $
//   * the Levels column: compact badges ("ONH", "CR0", "HVL", "LonLO") - the price, the touch odds "33%  /  50%", the liquidity
//     resting there and STALE / age move to the badge's hover (what it is, why it matters, the question, how the number answers
//     it, how to use it, an example)
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <map>
#include "DealerProfileAuditLogic.h"

namespace dcol {
static const char* const VERSION = "1.0.0";
static const unsigned long C_POS = 0x0014B8A6UL;   // teal: + net gamma (dealers long gamma: they lean against moves)
static const unsigned long C_NEG = 0x00F97316UL;   // orange: - net gamma (dealers short gamma: they chase moves)
static const int BADGE_FONT_MIN = 8, BADGE_FONT_MAX = 14, BADGE_FONT_DEF = 9;
inline int cleanBadgeFont(long long v) { return v >= BADGE_FONT_MIN && v <= BADGE_FONT_MAX ? (int)v : BADGE_FONT_DEF; }

// "B 1.2K" = dealers must BUY 1,200 contracts, "S 980" = SELL 980; "" when there is no real hedge number
inline std::string nodeText(bool hasD, double d)
{
    if (!hasD || !std::isfinite(d) || std::fabs(d) < 0.5) return std::string();
    const std::string n = dpaudit::deltaNum(std::fabs(d));
    if (n == "0") return std::string();
    return std::string(d > 0 ? "B " : "S ") + n;
}
// the text goes inside the node only when it fits: 3 px each side, and the node at least the text's height
// (14:37 CT review) the text's real height is pt x 4/3 px (96 dpi): the node must hold it + 1 px each side, else it is hidden
inline int textPx(int fontPt) { return (fontPt * 4 + 2) / 3; }
inline bool nodeTextFits(int textW, int nodeLen, int nodeH, int fontPt) { return textW > 0 && textW + 6 <= nodeLen && nodeH >= textPx(fontPt) + 2; }
// (14:37 CT, Rassul "F everywhere, makes no sense") a strike under 3% of the biggest wall on screen is not drawn at all (no slivers)
static const double NODE_MIN_SHARE = 0.03;
inline bool nodeShown(double absG, double absMax) { return absMax > 0 && absG > 0 && absG >= NODE_MIN_SHARE * absMax; }
// (14:37 CT) the hourly swings HrHI / HrLO were removed in 2.10.0: a merged name from an older shared file drops them ("PDH/HrHI" -> "PDH",
// "HrHI" -> "" = not drawn)
inline std::string dropHourly(const std::string& name)
{
    std::string out, part; size_t a = 0;
    while (a <= name.size()) {
        size_t b = name.find('/', a); if (b == std::string::npos) b = name.size();
        part = name.substr(a, b - a);
        if (part != "HrHI" && part != "HrLO" && !part.empty()) out += (out.empty() ? "" : "/") + part;
        a = b + 1;
    }
    return out;
}

// the node's hover: dpaudit::nodeHover (tested since 2.9.0) re-worded for the 2.11.0 face (teal + / orange - and B / S in the node)
inline dpaudit::Hover nodeHover(const std::string& mkt, const dpaudit::NodeX& x, double net, double absMax, double pxNow, bool textShown)
{
    dpaudit::Hover h = dpaudit::nodeHover(mkt, x, net, absMax, pxNow);
    for (size_t i = 0; i < h.lines.size(); ++i) {
        std::string& s = h.lines[i];
        if (s.compare(0, 10, "W = WALL: ") == 0) s = "Teal (+) = WALL: " + s.substr(10);
        else if (s.compare(0, 10, "F = FUEL: ") == 0) s = "Orange (-) = FUEL: " + s.substr(10);
        const std::string a = " - the number on the chart.", b = " (too small to print on the chart).";
        size_t p = s.find(a); if (p != std::string::npos) s.replace(p, a.size(), textShown ? " - the B / S text in the node." : ".");
        p = s.find(b); if (p != std::string::npos) s.replace(p, b.size(), textShown ? " - the B / S text in the node." : ".");
    }
    const std::string t = nodeText(x.hasD, x.d);
    if (!t.empty()) {
        const std::string K = dpaudit::fmtStrike(mkt, x.k);
        h.lines.insert(h.lines.begin(), "In the node: \"" + t + "\" = futures dealers must " + (x.d > 0 ? "BUY " : "SELL ") +
                       dpaudit::deltaNum(std::fabs(x.d)) + " " + mkt + " contracts (about " + dpaudit::usdShort(std::fabs(x.dUsd)) +
                       ") to stay hedged as price gets to " + K + ".");
    }
    return h;
}

// ---- the Levels column ---------------------------------------------------------------------------------------------------------
// the compact name: no " STALE ..." suffix, "CR 0D" -> "CR0", spaces out ("1D Max" -> "1DMax"); the full name is in the hover
inline std::string staleSuffix(const std::string& name)
{
    const size_t p = name.find(" STALE"); return p == std::string::npos ? std::string() : name.substr(p + 1);
}
inline std::string compactName(const std::string& name0)
{
    std::string n = name0; const size_t st = n.find(" STALE"); if (st != std::string::npos) n = n.substr(0, st);
    std::string o; o.reserve(n.size());
    for (size_t i = 0; i < n.size(); ++i) {
        if (n.compare(i, 3, " 0D") == 0 && (i + 3 == n.size() || n[i + 3] == ' ' || n[i + 3] == '/')) { o += '0'; i += 2; continue; }
        if (n[i] == ' ') continue;
        o += n[i];
    }
    if (o.compare(0, 4, "HVL0") == 0) o = "HVL" + o.substr(4);   // the flip level: "HVL" (mockup), its expiry is in the hover
    return o;
}
// what a level is, by its first name (key levels from the chart; MenthorQ levels from the options book)
inline std::string levelWhat(const std::string& name)
{
    std::string f = name.substr(0, name.find_first_of("/ "));
    if (f == "PFH" || f == "PFL") return std::string("the previous full session's (overnight + day) ") + (f == "PFH" ? "high" : "low");
    if (f == "PDH" || f == "PDL") return std::string("the previous day session's (8:30 AM - 3:00 PM CT) ") + (f == "PDH" ? "high" : "low");
    if (f == "PWH" || f == "PWL") return std::string("the previous week's ") + (f == "PWH" ? "high" : "low");
    if (f == "CWH" || f == "CWL") return std::string("this week's ") + (f == "CWH" ? "high" : "low") + " set before yesterday";
    if (f == "ONH" || f == "ONL") return std::string("tonight's overnight session ") + (f == "ONH" ? "high" : "low");
    if (f == "LonHI" || f == "LonLO") return std::string("the London session's ") + (f == "LonHI" ? "high" : "low");
    if (f == "AsiaHI" || f == "AsiaLO") return std::string("the Asia session's (6:00 PM - 2:00 AM CT) ") + (f == "AsiaHI" ? "high" : "low");
    const char k = dpaudit::mqKind(name);
    if (f == "CR0") return "today's 0DTE call resistance (the biggest call wall that expires today)";
    if (k == 'R') return "call resistance: the strike with the most call gamma - dealers sell into rallies there";
    if (k == 'G') return "put support: the strike with the most put gamma - dealers buy dips there";
    if (k == 'Y') return "HVL, the gamma flip: above it dealers are long gamma (moves fade), below it short gamma (moves run)";
    return "a MenthorQ gamma level: a strike where dealers' options hedging is heavy";
}
inline bool isKeyLevel(const std::string& name)
{
    const std::string f = name.substr(0, name.find_first_of("/ "));
    static const char* const K[] = { "PFH", "PFL", "PDH", "PDL", "PWH", "PWL", "CWH", "CWL", "ONH", "ONL", "LonHI", "LonLO", "AsiaHI", "AsiaLO" };
    for (size_t i = 0; i < sizeof(K) / sizeof(K[0]); ++i) if (f == K[i]) return true;
    return false;
}
// the badge's hover: question, what, why, how the number answers it, how to use it, an example (Rassul 2026-10-01 / 10-03)
inline dpaudit::Hover levelHover(const std::string& mkt, const std::string& name0, const std::string& priceTxt, const std::string& odds,
                                 const std::string& liq, bool pick, bool above)
{
    dpaudit::Hover h;
    std::string full = name0; const size_t st = full.find(" STALE"); if (st != std::string::npos) full = full.substr(0, st);
    std::string disp; for (char c : full) { if (c == '/') disp += " + "; else disp += c; }
    h.title = "Will " + mkt + " react at " + disp + " " + priceTxt + "?";
    h.lines.push_back("What: " + disp + " " + priceTxt + " = " + levelWhat(full) + ".");
    h.lines.push_back(isKeyLevel(full) ? "Why it matters: stops and resting orders gather at prior highs / lows - reversals start where a level is swept and reclaimed."
                                       : "Why it matters: dealers' options positions make them buy or sell futures here, so price often stalls or turns at it.");
    if (!odds.empty()) {
        const bool proven = odds.find('?') == std::string::npos;
        h.lines.push_back("Touch odds: " + odds + (odds.find('/') != std::string::npos ? "  (within 90 min  /  by the 3:00 PM CT close)" : "") +
                          (proven ? "." : " - '?' = not proven yet for this market."));
    } else h.lines.push_back("Touch odds: none right now (the odds file is stale or the level is too far).");
    std::string lq = liq; while (!lq.empty() && lq[0] == ' ') lq.erase(0, 1);
    if (!lq.empty()) h.lines.push_back("Liquidity resting here: " + lq + ". HL on the badge = 2.5x+ (this market's confirmation line): green = bids below price defending it as support, red = offers above defending it as resistance.");
    const std::string sx = staleSuffix(name0);
    if (!sx.empty()) h.lines.push_back("Data: " + sx + " - the options book behind this level has not updated; treat it as older.");
    if (pick) h.lines.push_back("White border = today's pick: the level the model rates most likely to be touched.");
    h.lines.push_back("How to use: watch the footprint as price reaches it - absorption (A) or exhaustion (Ex?) at the level is the turn; a clean break with volume = no turn.");
    h.lines.push_back(above ? "Example: price pokes above " + disp + " by a few ticks, buyers are absorbed and a red bar closes back below - a short against the level."
                            : "Example: price sweeps " + disp + " by a few ticks, sellers are absorbed and a green bar closes back above - a long against the level.");
    return h;
}

// ---- (Rassul 11:39 CT, design B, mockup-hl.png) the liquidity block of a level badge -------------------------------------------
// LRA-LiqLevels-<MKT>.csv (lsLiquidityProfile 0.16.0, native in IRT, written about once a second while it has a live book):
//   "LIQL,1,<unix>,<tick>,<confirmX>,<confirmKeepX>,<hw>" then "L,<tick index>,<S|R>,<contracts>,<bx>" for every price within its
//   reach that holds size: the contracts resting in the Liquidity Profile's own band around that price (2*hw+1 ticks) on the side
//   price would hit (S = bids below price, R = offers above), bx = that band x the median of the 8 same-width bands behind it (the
//   prominence the zones are drawn by; -1 = no book behind it to compare). A file over 20 s old = no liquidity (name-only badges).
struct LiqRow { bool bid = true; long long n = 0; double bx = -1; };
struct LiqLevels { double tick = 0, onX = 2.5, offX = 2.1; long long at = 0; int hw = 0; std::map<long long, LiqRow> rows; };
static const long long LIQL_STALE_S = 20;
inline bool parseLiqLevels(const std::string& text, long long nowUnix, LiqLevels& out, std::string& why)
{
    LiqLevels r; size_t a = 0; bool head = false; int n = 0;
    while (a < text.size() && n < 20000) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        std::string ln = text.substr(a, e - a); a = e + 1; ++n;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln.empty()) continue;
        std::vector<std::string> c; { size_t p = 0; while (true) { const size_t k = ln.find(',', p); c.push_back(ln.substr(p, k == std::string::npos ? std::string::npos : k - p)); if (k == std::string::npos) break; p = k + 1; } }
        if (!head) {
            if (c.size() < 7 || c[0] != "LIQL" || c[1] != "1") { why = "no LIQL header"; return false; }
            double at = 0, tk = 0, on = 0, off = 0, hw = 0;
            if (!dpaudit::parseStrictDouble(c[2], at) || !dpaudit::parseStrictDouble(c[3], tk) || !dpaudit::parseStrictDouble(c[4], on) ||
                !dpaudit::parseStrictDouble(c[5], off) || !dpaudit::parseStrictDouble(c[6], hw)) { why = "bad header"; return false; }
            if (!(tk > 0) || !(on > 1.0) || !(off > 0.5) || off > on || at <= 0 || hw < 0 || hw > 1000) { why = "bad header values"; return false; }
            if (nowUnix - (long long)at > LIQL_STALE_S || (long long)at - nowUnix > LIQL_STALE_S) { why = "stale"; return false; }
            r.at = (long long)at; r.tick = tk; r.onX = on; r.offX = off; r.hw = (int)hw; head = true; continue;
        }
        if (c.size() < 5 || c[0] != "L" || (c[2] != "S" && c[2] != "R")) continue;
        double ti = 0, lots = 0, bx = 0;
        if (!dpaudit::parseStrictDouble(c[1], ti) || !dpaudit::parseStrictDouble(c[3], lots) || !dpaudit::parseStrictDouble(c[4], bx)) continue;
        if (ti != std::floor(ti) || std::fabs(ti) > 9e15 || lots < 1 || lots > 1e9 || bx > 1e6) continue;
        LiqRow row; row.bid = c[2] == "S"; row.n = (long long)std::llround(lots); row.bx = bx > 0 ? bx : -1;
        r.rows[(long long)ti] = row;
    }
    if (!head) { why = "empty"; return false; }
    out = r; why.clear(); return true;
}
// the row for a level's price on the file's tick grid (null = no size there / no file)
inline const LiqRow* liqRowAt(const LiqLevels& L, double px)
{
    if (!(L.tick > 0) || !std::isfinite(px)) return nullptr;
    const double ti = px / L.tick; if (std::fabs(ti) > 9e15) return nullptr;
    std::map<long long, LiqRow>::const_iterator it = L.rows.find((long long)std::llround(ti));
    return it == L.rows.end() ? nullptr : &it->second;
}
// HL = high liquidity: on at the market's confirmation line (2.5x), off only under the keep line (2.1x) AND after 30 s on; a side
// change (price crossed the level: bids <-> offers) starts over; no data = off
struct HLState { bool on = false; long long since = -1; int side = 0; };
static const long long HL_HOLD_MS = 30000;
inline bool hlStep(HLState& h, bool have, bool bid, double bx, double onX, double offX, long long nowMs)
{
    const int side = bid ? 1 : -1;
    if (!have || !(bx > 0)) { h.on = false; h.since = -1; h.side = have ? side : 0; return false; }
    if (h.side != side) { h.on = false; h.since = -1; h.side = side; }
    if (!h.on) { if (bx >= onX) { h.on = true; h.since = nowMs; } }
    else if (bx < offX && h.since >= 0 && nowMs - h.since >= HL_HOLD_MS) { h.on = false; h.since = -1; }
    return h.on;
}
// "310", "1.2K", "15K" - contracts in a few characters (never "0": a level with nothing resting shows no block)
inline std::string lotsK(long long n)
{
    char b[24];
    if (n < 1) return std::string();
    if (n < 1000) std::snprintf(b, sizeof(b), "%lld", n);
    else if (n < 9950) std::snprintf(b, sizeof(b), "%.1fK", n / 1000.0);
    else std::snprintf(b, sizeof(b), "%.0fK", n / 1000.0);
    return b;
}
inline std::string liqBlockText(long long n, bool hl) { const std::string k = lotsK(n); return k.empty() ? k : (hl ? "HL " + k : k); }
} // namespace dcol
#endif
