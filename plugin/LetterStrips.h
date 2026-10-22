#ifndef LS_LETTER_STRIPS_H
#define LS_LETTER_STRIPS_H
// LetterStrips.h 1.0.0 (2026-10-22, Rassul approved layout-stages-2strips.png 11:41-11:46 CT, build ordered 15:20 CT) - SDK-free rules of the
// TWO LETTER STRIPS on the attacker pane (lsBidAskStats 1.28.0, "Who is aggressive?") and the Defense pane (lsBookFlow 1.23.0, "Who is
// defending?"), and the shared letter record the Reversal Stage pane (lsReversalStage 1.0.0) reads. Unit-tested on Linux
// (tests_strips/test_strips.cpp).
//
//   Attacker pane   strip 0 "Attack"          (stage 1)     S > K > L          (S sweep, K big aggressor, L large order)
//                   strip 1 "Running out"     (stages 3-4)  R > Tr > G         (R stop run reclaimed, Tr trapped, G giving up)
//   Defense pane    strip 0 "Holding"         (stages 2-3)  A > Q > W > P > B  (A absorbed, Q queue - only when the queue meter's record
//                                                                               exists, W withdrew at the level, P pressed in, B stepped back)
//                   strip 1 "Turn & re-test"  (stages 4-5)  H > L > X          (H hit back, L re-test reloaded, X re-test empty)
//   ONE letter per strip per candle (the strongest by the fixed priority above); the others go in that candle's hover.
//   Colour = the side that ACTS: green = buyers, red = sellers. Attacker letters: the attackers (buyers at a high = green, sellers at a low =
//   red; L by the order's side; R by the run's side). Defense letters: the defenders (bids at a low = green, offers at a high = red); P / B
//   the book side that moved (bids green, offers red). Warnings (W, B, X) in the colour of the side they hurt (= the defenders' colour).
//   The shared A is drawn on the Defense pane only (no longer on the attacker pane).
//   Neighbouring candles: placeStrip() puts each letter centred on its candle on the upper half-row; a letter that would touch the one
//   before it drops to the lower half-row; if it still touches, it moves right just enough (at most a third of its width) or is not drawn
//   (its hover stays on the candle's other strip). Rectangles never touch (>= 1 px gap).
//
// THE LETTER RECORD (append-only, one line per candle + letter + pivot side, first line wins, never rewritten):
//   lsFlexLevels\Strips\ATK-<MKT>-<spb>.csv   written by lsBidAskStats   (S K L R Tr G)
//   lsFlexLevels\Strips\DEF-<MKT>-<spb>.csv   written by lsBookFlow      (A W P B H L X)
//   lsFlexLevels\Strips\QUEUE-<MKT>-<spb>.csv written by the queue meter (Q) - the contract for that agent: the same line format, pane 'Q'
//   LT|1|<pane A/D/Q>|<candle raw key>|<letter>|<pivot 0 low / 1 high>|<book -1 none, 0 bids, 1 offers>|<key numbers, no '|'>
// The candle raw key is IRT's bar time (bar END) as the chart reports it (RTDATE), the same key every pane's own record uses.
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <string>
#include <vector>

namespace lst {

static const char* const VERSION = "1.0.0";
enum { LOW = 0, HIGH = 1 };                    // the pivot being built (= dt::LOW / dt::HIGH): LOW = a low pivot (green defenders)
enum { GREEN = 0, RED = 1 };                   // the colour of the side that acts: buyers green, sellers red
enum Pane { ATK = 0, DEF = 1, QUE = 2 };       // QUE = the queue meter's letters, drawn on the Defense pane's Holding strip
static const int STRIPS = 2;
static const int ROW_H = 22;                   // one strip: two half-rows of 11 px (the letters are 8 pt)
static const int SUB_H = ROW_H / 2;
static const int LABEL_W = 74;                 // the strip label column at the pane's left ("Turn & re-test" at 8 pt)
static const int GAP = 1;                      // letters never touch: >= 1 px between rectangles in the same half-row

inline const char* stripName(int pane, int strip) {
    if (pane == ATK) return strip == 0 ? "Attack" : "Running out";
    return strip == 0 ? "Holding" : "Turn & re-test";
}
// the strip of a letter on its pane (-1 = not a strip letter on that pane). 'L' is the large order on the attacker pane and the
// re-test reloaded on the Defense pane.
inline int stripOf(int pane, const std::string& c) {
    if (pane == ATK) { if (c == "S" || c == "K" || c == "L") return 0; if (c == "R" || c == "Tr" || c == "G") return 1; return -1; }
    if (c == "A" || c == "Q" || c == "W" || c == "P" || c == "B") return 0;
    if (c == "H" || c == "L" || c == "X") return 1;
    return -1;
}
// the fixed priority inside a strip: 0 = strongest. Approved 2026-10-22: Defense A > W > P > B (Q right after A), then H > L > X;
// Attacker S > K > L, then R > Tr > G.
inline int rankOf(int pane, const std::string& c) {
    static const char* const atk0[] = { "S", "K", "L" }; static const char* const atk1[] = { "R", "Tr", "G" };
    static const char* const def0[] = { "A", "Q", "W", "P", "B" }; static const char* const def1[] = { "H", "L", "X" };
    const int s = stripOf(pane, c); if (s < 0) return 99;
    const char* const* v = pane == ATK ? (s == 0 ? atk0 : atk1) : (s == 0 ? def0 : def1);
    const int n = pane == ATK ? 3 : (s == 0 ? 5 : 3);
    for (int k = 0; k < n; ++k) if (c == v[k]) return k;
    return 99;
}
inline bool isWarning(int pane, const std::string& c) { return pane != ATK && (c == "W" || c == "B" || c == "X"); }

// one letter decided on a candle
struct Lt {
    std::string code; int pivot = LOW; int book = -1;   // book: the BOOK side that moved (P / B), the order's resting side (attacker L: 0 = bids hit, 1 = offers lifted); -1 none
    int colour = GREEN; std::string nums;                // nums: the key numbers for the Reversal Stage hover ("1,940 lots 2.6x")
};
// the colour of the side that acts (see the header)
inline int colourOf(int pane, const std::string& c, int pivot, int book) {
    if (pane == ATK) {
        if (c == "L" && (book == 0 || book == 1)) return book == 1 ? GREEN : RED;   // offers lifted = buyers (green), bids hit = sellers (red)
        return pivot == HIGH ? GREEN : RED;              // the attackers: buyers into a high, sellers into a low (R: the run's side)
    }
    if ((c == "P" || c == "B") && (book == 0 || book == 1)) return book == 0 ? GREEN : RED;   // the book side that moved
    return pivot == LOW ? GREEN : RED;                   // the defenders (warnings: the side they hurt = the defenders)
}
inline Lt make(int pane, const std::string& c, int pivot, int book, const std::string& nums = std::string()) {
    Lt l; l.code = c; l.pivot = pivot == HIGH ? HIGH : LOW; l.book = book; l.colour = colourOf(pane, c, l.pivot, book); l.nums = nums; return l;
}

// the pick: per strip the strongest letter shown, the rest (strongest first) for the hover. Ties (the same letter at both pivots) keep the
// first given.
struct Pick { int shown[STRIPS] = { -1, -1 }; std::vector<int> order[STRIPS]; };   // indexes into the letters given
inline Pick pick(int pane, const std::vector<Lt>& v) {
    Pick p;
    for (int s = 0; s < STRIPS; ++s) {
        std::vector<int>& o = p.order[s];
        for (size_t k = 0; k < v.size(); ++k) if (stripOf(pane == QUE ? DEF : pane, v[k].code) == s) o.push_back((int)k);
        std::stable_sort(o.begin(), o.end(), [&](int a, int b) { return rankOf(pane == QUE ? DEF : pane, v[(size_t)a].code) < rankOf(pane == QUE ? DEF : pane, v[(size_t)b].code); });
        if (!o.empty()) p.shown[s] = o.front();
    }
    return p;
}

// ---- placement on a strip: x = the candle's centre, w = the letter's width (px); returns per item its left edge and half-row (0 upper,
// 1 lower), or drawn = false. Items in candle order (left to right). [lo, hi) = the free x range (right of the label, left of the edge).
struct Item { int xc = 0, w = 0; };
struct Placed { int l = 0, r = 0, sub = 0; bool drawn = false; };
inline std::vector<Placed> placeStrip(const std::vector<Item>& it, int lo, int hi) {
    std::vector<Placed> out(it.size());
    int lastR[2] = { INT_MIN / 2, INT_MIN / 2 };
    for (size_t k = 0; k < it.size(); ++k) {
        const int w = std::max(1, it[k].w); const int l0 = it[k].xc - w / 2;
        Placed p; bool done = false;
        for (int sub = 0; sub < 2 && !done; ++sub) {
            if (l0 >= lastR[sub] + GAP + 1 && l0 >= lo && l0 + w <= hi) { p.l = l0; p.sub = sub; done = true; }
        }
        if (!done) {                                         // shift right a little on the half-row with the most room
            const int sub = lastR[0] <= lastR[1] ? 0 : 1; const int l = std::max(l0, std::max(lo, lastR[sub] + GAP + 1));
            if (l - l0 <= std::max(2, w / 3) && l + w <= hi) { p.l = l; p.sub = sub; done = true; }
        }
        if (done) { p.r = p.l + w; p.drawn = true; lastR[p.sub] = p.r; }
        out[k] = p;
    }
    return out;
}
// true when two placed letters touch or overlap (the same half-row, rectangles closer than GAP)
inline bool touches(const Placed& a, const Placed& b) { return a.drawn && b.drawn && a.sub == b.sub && a.l <= b.r + GAP && b.l <= a.r + GAP; }

// ---- the hover header of one strip on one candle: "Holding: A   10:09 AM   (also on this candle: W, P)"
inline std::string header(int pane, int strip, const std::string& shown, const std::string& when, const std::vector<std::string>& others) {
    std::string t = std::string(stripName(pane, strip)) + ": " + shown;
    if (!when.empty()) t += "   " + when;
    if (!others.empty()) { t += "   (also on this candle: "; for (size_t k = 0; k < others.size(); ++k) t += (k ? ", " : "") + others[k]; t += ")"; }
    return t;
}
// the strip's question rows at the top of its hover (his format: the question, how the strip answers it, how to use it, an example)
inline std::vector<std::string> stripQuestion(int pane, int strip) {
    std::vector<std::string> r;
    if (pane == ATK && strip == 0) {
        r.push_back("Question: who is attacking this extreme, and how big? (stage 1 of the pivot)");
        r.push_back("   How: one letter per candle - S the biggest sweep of the leg > K one order >= S10 > L one order >= S20");
        r.push_back("   Use: the attack is the fuel - watch the Defense pane for an A on the same candle. Example: red K at a low = a big seller hit it");
    } else if (pane == ATK) {
        r.push_back("Question: are the attackers running out? (stages 3-4 of the pivot)");
        r.push_back("   How: one letter per candle - R stop run reclaimed > Tr trapped > G attackers pulling their orders");
        r.push_back("   Use: after an A, a Running-out letter is the exhaustion step. Example: red G at a low = the sellers gave up");
    } else if (strip == 0) {
        r.push_back("Question: are the defenders holding this extreme? (stages 2-3 of the pivot)");
        r.push_back("   How: one letter per candle - A absorbed > Q queue > W withdrew (warning) > P pressed in > B stepped back (warning)");
        r.push_back("   Use: A is the hold; W / B are warnings in the colour of the side they hurt. Example: green A at a low = bids absorbed the sellers");
    } else {
        r.push_back("Question: did the defenders turn it and hold the re-test? (stages 4-5 of the pivot)");
        r.push_back("   How: one letter per candle - H hit back > L re-test reloaded > X re-test empty (warning)");
        r.push_back("   Use: H = the turn; L = the re-test held (entry zone); X = the re-test is empty. Example: green L = bids reloaded at the low");
    }
    return r;
}

// ---- the record ---------------------------------------------------------------------------------------------------------------------
inline char paneChar(int pane) { return pane == ATK ? 'A' : pane == DEF ? 'D' : 'Q'; }
inline std::string fileName(int pane, const std::string& mkt, int spb) {
    const char* p = pane == ATK ? "ATK-" : pane == DEF ? "DEF-" : "QUEUE-";
    return std::string(p) + (mkt == "6E" ? std::string("EU") : mkt) + "-" + std::to_string(spb) + ".csv";
}
inline std::string clean(const std::string& s) { std::string o; for (char c : s) if (c != '|' && c != '\r' && c != '\n') o += c; return o.size() > 120 ? o.substr(0, 120) : o; }
inline std::string line(int pane, long long key, const Lt& l) {
    return "LT|1|" + std::string(1, paneChar(pane)) + "|" + std::to_string(key) + "|" + l.code + "|" + std::to_string(l.pivot) + "|" + std::to_string(l.book) + "|" + clean(l.nums);
}
inline bool fullLL(const std::string& f, long long& v) {
    if (f.empty() || f.size() > 20) return false;
    errno = 0; char* e = nullptr; const long long x = std::strtoll(f.c_str(), &e, 10);
    if (errno != 0 || !e || *e != '\0') return false;
    v = x; return true;
}
inline bool validCode(int pane, const std::string& c) { return stripOf(pane == QUE ? DEF : pane, c) >= 0 && (pane != QUE || c == "Q") && (pane == QUE || c != "Q"); }
struct Rec { int pane = ATK; long long key = 0; Lt l; };
inline bool parse(const std::string& s0, Rec& r) {
    std::string s = s0; while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) s.pop_back();
    std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t b = s.find('|', a); f.push_back(s.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; if (f.size() > 8) return false; }
    if (f.size() != 8 || f[0] != "LT" || f[1] != "1" || f[2].size() != 1) return false;
    const char pc = f[2][0]; const int pane = pc == 'A' ? ATK : pc == 'D' ? DEF : pc == 'Q' ? QUE : -1; if (pane < 0) return false;
    long long key = 0, piv = 0, book = 0;
    if (!fullLL(f[3], key) || key <= 0 || !fullLL(f[5], piv) || (piv != 0 && piv != 1) || !fullLL(f[6], book) || book < -1 || book > 1) return false;
    if (!validCode(pane, f[4])) return false;
    r.pane = pane; r.key = key; r.l = make(pane == QUE ? DEF : pane, f[4], (int)piv, (int)book, f[7]);
    return true;
}
// the identity of a record line (first line wins): pane, candle, letter, pivot
inline std::string idOf(int pane, long long key, const std::string& code, int pivot) { return std::string(1, paneChar(pane)) + std::to_string(key) + "|" + code + "|" + std::to_string(pivot); }

// the market of a chart's root symbol - the same names as OrderFlowFeed.h off::marketName (lsBidAskStats / lsBookFlow write with these)
inline std::string marketName(std::string r) {
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    std::string out; const std::string s = r.substr(0, 8);
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) { const unsigned char c = (unsigned char)s[i]; out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_'); }
    return out.empty() ? "UNKNOWN" : out;
}

} // namespace lst
#endif
