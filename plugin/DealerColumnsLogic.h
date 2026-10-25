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
#include "Fresh.h"   // (1.2.0) the one freshness rule (the header hover states it)

namespace dcol {
static const char* const VERSION = "1.2.0";   // 1.2.0 (DP 2.17.0, 2026-10-18): the Market State GEX facts (mstGex), the header hover states the Fresh.h rule // 1.1.0 (2026-10-14 18:25 CT audit display list): model-not-forecast hovers, "HL?", reach-only odds, the locked trigger in "How to use"
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
    return n + " lots";   // (2.16.0, Rassul 2026-10-16 22:16 "just give the number followed by lots") the node colour says buy (teal) / sell (orange)
}
// (2.16.0) a merged level name ("PDH/CR") -> the key level ("PDH") and its gamma name ("CR"), so the badge reads NAME | price | GAMMA | ratio
inline bool isGammaTok(const std::string& s)
{
    if (s.empty()) return false;
    if (s.compare(0, 3, "HVL") == 0 || s.compare(0, 2, "CR") == 0 || s.compare(0, 2, "1D") == 0 || s.compare(0, 2, "GW") == 0) return true;
    if (s.compare(0, 2, "PS") == 0 && (s.size() == 2 || (s[2] >= '0' && s[2] <= '9'))) return true;
    if (s[0] == 'G' && s.size() >= 2 && s[1] >= '0' && s[1] <= '9') return true;
    return false;
}
inline bool splitGamma(const std::string& name, std::string& key, std::string& gamma)
{
    key.clear(); gamma.clear(); size_t a = 0;
    while (a <= name.size()) {
        size_t e = name.find('/', a); if (e == std::string::npos) e = name.size();
        const std::string tok = name.substr(a, e - a);
        if (!tok.empty()) { std::string& d = isGammaTok(tok) ? gamma : key; if (!d.empty()) d += "/"; d += tok; }
        a = e + 1;
    }
    return !key.empty() && !gamma.empty();
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
// (DP 2.12.0, 2026-10-14 18:25 CT, Rassul approved the external-audit display list, section 5) the bar hover says what the vendor model
// shows, not what dealers will do: "Teal = MenthorQ model shows + net gamma here (vendor model, data N min old). Not a prediction it
// holds." The B / S hedge line ("futures dealers must BUY 1,200 ...") stays - his wording (KEEP, his call 10-14).
inline std::string ageText(double ageMin) { return ageMin >= 0 ? "data " + std::to_string((long long)(ageMin + 0.5)) + " min old" : std::string("data age unknown"); }
inline dpaudit::Hover nodeHover(const std::string& mkt, const dpaudit::NodeX& x, double net, double absMax, double pxNow, bool textShown, double ageMin = -1)
{
    dpaudit::Hover h = dpaudit::nodeHover(mkt, x, net, absMax, pxNow);
    const std::string K0 = dpaudit::fmtStrike(mkt, x.k);
    for (size_t i = 0; i < h.lines.size(); ++i) {
        std::string& s = h.lines[i];
        if (s.compare(0, 10, "W = WALL: ") == 0) s = "Teal = MenthorQ model shows + net gamma here (vendor model, " + ageText(ageMin) + "). Not a prediction it holds.";
        else if (s.compare(0, 10, "F = FUEL: ") == 0) s = "Orange = MenthorQ model shows - net gamma here (vendor model, " + ageText(ageMin) + "). Not a prediction it breaks.";
        else if (s.compare(0, 15, "Why it matters:") == 0) s = "Why it matters: the model's gamma at " + K0 + " - a place to watch price and the tape, not a turn forecast.";
        else if (s.compare(0, 11, "How to use:") == 0) s = "How to use: wait for your trigger - a sweep through the exact level, then RB or CoB, before 11:00 AM CT.";
        else if (s.compare(0, 8, "Example:") == 0 && (s.find("absorbed") != std::string::npos || s.find("doing its job") != std::string::npos))
            s = "Example: price sweeps " + K0 + " by a few ticks and a reversal bar (RB) or CoB closes back - that is your trigger here, not the node.";
        else if (s.compare(0, 8, "Example:") == 0) {   // the hedge example keeps his "dealers buying N" wording; the hold / run forecast goes
            const char* const tails[2] = { "the wall is where a short can set up.", "the fuel is where it can run." };
            for (int t = 0; t < 2; ++t) { const size_t q = s.find(tails[t]); if (q != std::string::npos) s.replace(q, std::string(tails[t]).size(), "your trigger there is still a sweep, then RB or CoB."); }
        }
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
    if (f == "AsiaHI" || f == "AsiaLO") return std::string("the Asia session's (6:00 PM - 1:00 AM CT) ") + (f == "AsiaHI" ? "high" : "low");
    if (f == "Open") return "today's RTH open (the first trade of the day session)";                                   // (DP 2.18.0)
    if (f == "DPoc") return "the previous full session's volume point of control (the price that traded the most)";
    if (f == "VAH" || f == "VAL") return std::string("the previous full session's value area ") + (f == "VAH" ? "high" : "low") + " (70% of its volume traded inside)";
    if (f == "WPoc") return "the previous week's volume point of control (the price that traded the most)";
    const char k = dpaudit::mqKind(name);
    if (f == "CR0") return "today's 0DTE call resistance (the biggest call wall that expires today)";
    if (k == 'R') return "call resistance: the strike with the most call gamma (MenthorQ model)";            // (DP 2.12.0) no "dealers sell into rallies" claim
    if (k == 'G') return "put support: the strike with the most put gamma (MenthorQ model)";
    if (k == 'Y') return "HVL, the MenthorQ model's gamma flip level";
    return "a MenthorQ gamma level: a strike where dealers' options hedging is heavy";
}
inline bool isKeyLevel(const std::string& name)
{
    const std::string f = name.substr(0, name.find_first_of("/ "));
    static const char* const K[] = { "PFH", "PFL", "PDH", "PDL", "PWH", "PWL", "CWH", "CWL", "ONH", "ONL", "LonHI", "LonLO", "AsiaHI", "AsiaLO", "Open", "DPoc", "VAH", "VAL", "WPoc" };   // (DP 2.18.0) + Open / profile levels
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
                                       : "Why it matters: MenthorQ model level, about 6 to 8 min delayed. Watch price and tape here; it isn't a turn forecast.");   // (DP 2.12.0, audit list)
    if (!odds.empty()) {
        const bool proven = odds.find('?') == std::string::npos;
        h.lines.push_back("Touch odds: " + odds + (odds.find('/') != std::string::npos ? "  (within 90 min  /  by the 3:00 PM CT close)" : "") +
                          (proven ? "." : " - '?' = not proven yet for this market.") + " A chance to REACH only - not hold or reverse.");   // (DP 2.12.0, audit list)
    } else h.lines.push_back("Touch odds: none right now (the odds file is stale or the level is too far).");
    std::string lq = liq; while (!lq.empty() && lq[0] == ' ') lq.erase(0, 1);
    if (!lq.empty()) h.lines.push_back("Liquidity resting here: " + lq + ". HL? on the badge = resting size 2.5x+ this market's normal (green = bids below price, red = offers above); whether they hold is unknown.");   // (DP 2.12.0, audit list)
    const std::string sx = staleSuffix(name0);
    if (!sx.empty()) h.lines.push_back("Data: " + sx + " - the options book behind this level has not updated; treat it as older.");
    if (pick) h.lines.push_back("White border = today's pick ?: the level the reach model rates most likely to be touched (a chance to REACH, not proven).");   // (DP 2.12.0) MARK ?
    h.lines.push_back("How to use: wait for your trigger - a sweep through the exact level, then RB or CoB, before 11:00 AM CT.");   // (DP 2.12.0, audit list: the locked trigger)
    h.lines.push_back(above ? "Example: price pokes above " + disp + " by a few ticks and a reversal bar (RB) or CoB closes back below before 11:00 AM CT - a short trigger at the level."
                            : "Example: price sweeps " + disp + " by a few ticks and a reversal bar (RB) or CoB closes back above before 11:00 AM CT - a long trigger at the level.");
    return h;
}


// ---- (DP 2.15.0, Rassul 2026-10-15 13:21-13:28 "make the hover over on the levels more compact as well as the hover over on the dealer
// profile and its nodes", mockup-compact-hovers-v2.png) the COMPACT hovers: the same question / what / numbers / use / example, one
// short line each (the long forms above stay for the tests and the audit list)
inline dpaudit::Hover levelHoverC(const std::string& mkt, const std::string& name0, const std::string& priceTxt, const std::string& odds,
                                  const std::string& liq, bool pick, bool above)
{
    dpaudit::Hover h;
    std::string full = name0; const size_t st = full.find(" STALE"); if (st != std::string::npos) full = full.substr(0, st);
    const std::string cn = compactName(full);                              // "CR 0D/GW 0D" -> "CR0/GW0" (the badge's own name)
    std::string disp; for (char c : cn) { if (c == '/') disp += " + "; else disp += c; }
    const bool key = isKeyLevel(full);
    h.title = disp + " " + priceTxt + " \xB7 will " + mkt + " react here?";
    h.lines.push_back("What: " + (cn.compare(0, 3, "CR0") == 0 ? levelWhat("CR0") : levelWhat(full)) + (key ? std::string() : std::string(" \xB7 ~7 min delayed")));
    if (!odds.empty()) {
        const bool proven = odds.find('?') == std::string::npos;
        h.lines.push_back("Touch odds: " + odds + (odds.find('/') != std::string::npos ? "  (90 min  /  by close)" : "") + " \xB7 reach only" +
                          (proven ? std::string() : std::string(" \xB7 ? = not proven yet")));
    } else h.lines.push_back("Touch odds: none now (stale or too far)");
    std::string lq = liq; while (!lq.empty() && lq[0] == ' ') lq.erase(0, 1);
    if (!lq.empty()) h.lines.push_back("Resting here: " + lq + (above ? " (offers above price)" : " (bids below price)"));
    const std::string sx = staleSuffix(name0);
    if (!sx.empty()) h.lines.push_back("Data: " + sx + " \xB7 the book behind it has not updated");
    if (pick) h.lines.push_back("White border = today's pick ? (most likely to be touched)");
    h.lines.push_back(std::string("Use: sweep through it, then RB or CoB closes back ") + (above ? "below = short" : "above = long"));
    h.lines.push_back(std::string("e.g. pokes a few ticks ") + (above ? "above" : "below") + ", RB closes back " + (above ? "below = short trigger" : "above = long trigger"));
    return h;
}
inline dpaudit::Hover nodeHoverC(const std::string& mkt, const dpaudit::NodeX& x, double net, double pxNow, double ageMin = -1)
{
    dpaudit::Hover h; const std::string K = dpaudit::fmtStrike(mkt, x.k);
    const bool w = net >= 0, up = x.k > pxNow;
    h.title = K + " \xB7 what do dealers do if " + mkt + " gets here?";
    std::string l1 = std::string(w ? "Teal = + net gamma (wall) " : "Orange = - net gamma (fuel) ") + dpaudit::usdShort(x.absG) + " per pt";
    if (x.hasChg) l1 += " \xB7 " + dpaudit::pctChg(x.chg) + " since the open (" + (x.chg >= 0 ? "building" : "fading") + ")";
    if (dpaudit::mutedNode(x.absG, net)) l1 += " \xB7 calls and puts nearly cancel";
    h.lines.push_back(l1);
    if (x.hasD && std::fabs(x.d) >= 1.0) {
        const std::string t = nodeText(x.hasD, x.d);
        h.lines.push_back((t.empty() ? std::string() : "\"" + t + "\" = ") + "dealers must " + (x.d > 0 ? "BUY " : "SELL ") + dpaudit::deltaNum(std::fabs(x.d)) + " " + mkt +
                          " (~" + dpaudit::usdShort(std::fabs(x.dUsd)) + ") on the way " + (up ? "up" : "down") + " to " + K);
    }
    h.lines.push_back("MenthorQ model, " + (ageMin >= 0 ? std::to_string((long long)(ageMin + 0.5)) + " min old" : std::string("age unknown")) + " \xB7 a place to watch, not a forecast");
    h.lines.push_back("Use: sweep through it, then RB or CoB closes back = trigger");
    h.lines.push_back(std::string("e.g. pokes a few ticks ") + (up ? "above " : "below ") + K + ", RB closes back = " + (up ? "short" : "long") + " trigger");
    return h;
}
// 4 lines: the level hover borrows the first 2 (DP inserts begin .. end - 2 under the title)
inline dpaudit::Hover hvlHoverC(const std::string& mkt, double hvl)
{
    dpaudit::Hover h; const std::string P = dpaudit::fmtStrike(mkt, hvl);
    h.title = "Which gamma regime is " + mkt + " in?";
    h.lines.push_back("HVL " + P + " = where dealers flip from long to short gamma");
    h.lines.push_back("Above: long gamma, moves fade \xB7 below: short gamma, moves run");
    h.lines.push_back("Use: above, fade at walls \xB7 below, respect breakouts");
    h.lines.push_back("e.g. drops through " + P + " = range widens, walls below give way faster");
    return h;
}
inline dpaudit::Hover headerHoverC(const std::string& ver, const dpaudit::BookAsOf& a, double age, bool stale)
{
    dpaudit::Hover h;
    h.title = stale ? "Why is the dealer data STALE?" : "How fresh is the dealer data?";
    h.lines.push_back("Grid " + (a.ok() ? dpaudit::clock12(a.so) : std::string("time unknown")) + (age >= 0 ? " (" + dpaudit::ageShort(age) + " ago)" : std::string()) + " \xB7 Dealer Profile v" + ver);
    h.lines.push_back("STALE = over 20 min old while MenthorQ updates (older than the last close while it pauses): nodes fade, don't trade them");   // (1.2.0) Fresh.h, the same rule as the HOD / LOD box
    h.lines.push_back("Use: when STALE, trade key levels + footprint only");
    return h;
}
// (DP 2.15.0, Rassul 13:25 "make the sizing of the hover dynamic ... so the lines dont take up the next line with only one or a few
// words") the box width: the fewest wrapped rows, then the fewest short leftover rows (< 40% of the width), then the narrowest
template <class Wrap> inline int hoverWidth(int lo, int hi, Wrap rowsAt)
{
    int best = hi; long long bestScore = -1;
    for (int w = lo; w <= hi; w += 10) {
        int rows = 0, widows = 0; rowsAt(w, rows, widows);
        const long long sc = (long long)rows * 100000 + (long long)widows * 1000 + w;
        if (bestScore < 0 || sc < bestScore) { bestScore = sc; best = w; }
    }
    return best;
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
inline std::string liqBlockText(long long n, bool hl) { const std::string k = lotsK(n); return k.empty() ? k : (hl ? "HL? " + k : k); }   // (DP 2.12.0, audit list) "HL?" - the 2.5x line is not proven
// ---- (1.2.0, DP 2.17.0) the MARKET STATE facts, SRC "GEX" (MARKET-STATE-SPEC 1.0): regime, hvl, cr, ps, near_level, near_dist_ticks,
// near_lots, data_age_min, stale. DEX stays context inside the gamma read (Rassul 2026-10-18): no DEX fact / signal here. SDK-free (tested).
struct MqLvl { std::string label; double px = 0; };          // a MenthorQ level as drawn on this chart (chart price)
struct HedgeNode { double px = 0; bool hasD = false; double d = 0; };   // a strike (chart price) and the hedge lots there (+ buy / - sell)
// "CR 0D / GW 0D" -> tokens {"CR0", "GW0"}; "HVL 0D" -> {"HVL0"}; "G1" -> {"G1"}; "MQ CR 0D" -> {"CR0"}
inline std::vector<std::string> mstTokens(const std::string& label)
{
    std::vector<std::string> out; const std::string r = dpaudit::stripMQ(label); size_t a = 0;
    while (a <= r.size()) {
        const size_t b = r.find('/', a); std::string t = r.substr(a, b == std::string::npos ? std::string::npos : b - a);
        std::string w; for (char c : t) if (c != ' ') w += (char)std::toupper((unsigned char)c);
        const size_t st = w.find("STALE"); if (st != std::string::npos) w = w.substr(0, st);
        if (w.size() > 2 && w.compare(w.size() - 2, 2, "0D") == 0) w = w.substr(0, w.size() - 1);   // "CR0D" -> "CR0"
        if (!w.empty() && w.size() <= 12) out.push_back(w);
        if (b == std::string::npos) break;
        a = b + 1;
    }
    return out;
}
inline std::string mstPx(double v, int dec) { if (!std::isfinite(v) || !(v > 0)) return "?"; char b[48]; std::snprintf(b, sizeof b, "%.*f", dec < 0 ? 2 : (dec > 6 ? 6 : dec), v); return b; }
inline std::map<std::string, std::string> mstGex(const std::vector<MqLvl>& lv, const std::vector<HedgeNode>& nodes, double close, double tick, int dec,
                                                 double ageMin, bool stale)
{
    std::map<std::string, std::string> kv;
    double hvl = NAN, cr = NAN, ps = NAN;
    for (const MqLvl& L : lv) for (const std::string& t : mstTokens(L.label)) {
        if (t.compare(0, 3, "HVL") == 0 && !std::isfinite(hvl)) hvl = L.px;
        else if (t.compare(0, 2, "CR") == 0 && !std::isfinite(cr)) cr = L.px;
        else if (t.compare(0, 2, "PS") == 0 && !std::isfinite(ps)) ps = L.px;
    }
    const bool px = std::isfinite(close) && close > 0;
    kv["regime"] = (!stale && px && std::isfinite(hvl)) ? (close >= hvl ? "pos" : "neg") : "?";   // never a stale regime as live
    kv["hvl"] = mstPx(hvl, dec); kv["cr"] = mstPx(cr, dec); kv["ps"] = mstPx(ps, dec);
    // (never "near" / "far" as names: windows.h macros, lesson 3) the MenthorQ level nearest the last closed close (G2-G10 included: they are levels too), its distance in ticks and the hedge there
    const MqLvl* nl = nullptr; double best = 1e300;
    if (px) for (const MqLvl& L : lv) { const double d = std::fabs(L.px - close); if (std::isfinite(L.px) && L.px > 0 && d < best) { best = d; nl = &L; } }
    if (nl) {
        const std::vector<std::string> t = mstTokens(nl->label); std::string nm; for (size_t i = 0; i < t.size(); ++i) nm += (i ? "/" : "") + t[i];
        kv["near_level"] = nm.empty() ? "?" : nm + "@" + mstPx(nl->px, dec);
        kv["near_dist_ticks"] = tick > 0 ? std::to_string((long long)std::llround(best / tick)) : "?";
        const HedgeNode* hn = nullptr; double hb = 1e300;
        for (const HedgeNode& n : nodes) { const double d = std::fabs(n.px - nl->px); if (n.hasD && std::isfinite(n.d) && d < hb) { hb = d; hn = &n; } }
        kv["near_lots"] = (hn && (tick <= 0 || hb <= tick * 0.5 + 1e-9) && !stale) ? std::to_string((long long)std::llround(hn->d)) : "?";
    } else { kv["near_level"] = "?"; kv["near_dist_ticks"] = "?"; kv["near_lots"] = "?"; }
    kv["data_age_min"] = ageMin >= 0 ? std::to_string((long long)std::floor(ageMin)) : "?";
    kv["stale"] = stale ? "1" : "0";
    return kv;
}
// (1.2.0, DP 2.17.0, audit C finding 10) the per-chart status file: DealerProfile.status-<MKT>-<spb>.txt ("" = no market / bar size yet)
inline std::string statusName(const std::string& mkt, long spb)
{
    if (mkt.empty() || spb <= 0 || mkt.size() > 8) return std::string();
    for (char c : mkt) if (!std::isalnum((unsigned char)c)) return std::string();
    return "DealerProfile.status-" + mkt + "-" + std::to_string(spb) + ".txt";
}
} // namespace dcol
#endif
