/********************************************************************************
 *  GammaProfileLogic.h  —  the DECISIONS lsGammaProfile makes, with no drawing
 *  and no Investor/RT SDK, so they can be unit-tested (test_gammaprofile_logic.cpp)
 *  with any C++ compiler and pinned against the doctrine fixtures.
 *
 *  (v0.45, 2026-09-16) Extracted from GammaProfile.cpp after three days in which every
 *  defect in this plugin was found by the operator's eyes and none by a test.
 *
 *  Doctrine sources (the gate): skylit-docs/patternpedia/pattern-the-gatekeeper.md,
 *  skylit-docs/learn/air-pockets-velocity.md, skylit-docs/learn/heatseeker-patterns.md,
 *  skylit-docs/FINDINGS.md S6 (stacks), core-concepts §Gatekeeper / §Air Pockets.
 ********************************************************************************/
#ifndef GAMMA_PROFILE_LOGIC_H
#define GAMMA_PROFILE_LOGIC_H

#include <vector>
#include <cstdio>
#include <string>
#include <cmath>
#include <algorithm>

namespace gpl {

// One STRIKE row after the contract offset has been applied (price = chart scale).
struct Node {
    float price;        // chart-scale price (ES on this chart)
    float pct;          // signed %King, -100..100
    int   rank;         // 1 = biggest |pct|
    bool  king;
    float spx;          // raw SPX strike (0 if the panel predates it)
    std::string type;   // "", RUG, RRUG, PIKA, PIKAM, BARNEY, BARNEYM (from the panel)
};

// ---- thresholds (mirror the panel: gatekeeper() ranks by magnitude; REGIME_SIG_PCT=20;
//      FINDINGS S6: a stack member is >= 30% of the King) ----
static const float GK_MIN_PCT   = 30.0f;   // a Gatekeeper must be at least this % of the King
static const float AIR_THIN_PCT = 8.0f;    // a strike under this |%| is "thin" (part of a pocket)
static const float AIR_EDGE_PCT = 20.0f;   // a pocket only exists between nodes at least this big
static const int   AIR_MIN_RUN  = 3;       // and at least this many thin strikes wide

// ---- roles ---------------------------------------------------------------
// 0 none · 1 KING · 2 CEIL (biggest |node| above spot, King excluded) · 3 FLOOR (biggest below)
// 4 GATE (the ONE dominant blocker strictly between spot and the King, >= GK_MIN_PCT)
// 5 GATE+CEIL (the gatekeeper is also the ceiling) · 6 GATE+FLOOR
enum Role { R_NONE=0, R_KING=1, R_CEIL=2, R_FLOOR=3, R_GATE=4, R_GATE_CEIL=5, R_GATE_FLOOR=6 };

struct Roles {
    int kIdx, cIdx, fIdx, gIdx;
    std::vector<int> role;
};

inline Roles roles(const std::vector<Node>& s, float spot, bool hasSpot, float gkMinPct = GK_MIN_PCT)
{
    Roles R; R.kIdx = R.cIdx = R.fIdx = R.gIdx = -1; R.role.assign(s.size(), R_NONE);
    float sp = hasSpot ? spot : 0.0f;
    for (size_t i = 0; i < s.size(); i++) if (s[i].king) { R.kIdx = (int)i; if (!hasSpot) sp = s[i].price; }
    float fBest = -1, cBest = -1;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i].king) continue;
        float a = std::fabs(s[i].pct);
        if      (s[i].price < sp) { if (a > fBest) { fBest = a; R.fIdx = (int)i; } }
        else if (s[i].price > sp) { if (a > cBest) { cBest = a; R.cIdx = (int)i; } }
    }
    if (R.kIdx >= 0) R.role[R.kIdx] = R_KING;
    if (R.cIdx >= 0) R.role[R.cIdx] = R_CEIL;
    if (R.fIdx >= 0) R.role[R.fIdx] = R_FLOOR;
    if (R.kIdx >= 0) {
        float kp = s[R.kIdx].price, lo = sp < kp ? sp : kp, hi = sp < kp ? kp : sp, gBest = -1;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i].king) continue;
            float a = std::fabs(s[i].pct);
            if (s[i].price > lo && s[i].price < hi && a >= gkMinPct && a > gBest) { gBest = a; R.gIdx = (int)i; }
        }
        if (R.gIdx >= 0) {
            // the gatekeeper keeps its ceiling/floor identity when it is the same node ("G·C" / "G·F")
            R.role[R.gIdx] = (R.gIdx == R.cIdx) ? R_GATE_CEIL : (R.gIdx == R.fIdx ? R_GATE_FLOOR : R_GATE);
        }
    }
    return R;
}

inline const char* roleTag(int role, const char* kingLabel)
{
    switch (role) {
        case R_KING:       return kingLabel;
        case R_CEIL:       return "C";
        case R_FLOOR:      return "F";
        case R_GATE:       return "G";
        case R_GATE_CEIL:  return "G\xB7" "C";   // G·C (ANSI middle dot, renders in IRT's text)
        case R_GATE_FLOOR: return "G\xB7" "F";
        default:           return "";
    }
}

// ---- air pockets ---------------------------------------------------------
// A thin run (>= minRun strikes under thinPct) bounded on BOTH sides by a node >= edgePct.
// Indices refer to the price-ascending order of `s` (returned in `ord`).
struct Pocket { float lo, hi; int n; bool neg; };

inline std::vector<Pocket> airPockets(const std::vector<Node>& s,
                                      float thinPct = AIR_THIN_PCT, float edgePct = AIR_EDGE_PCT, int minRun = AIR_MIN_RUN)
{
    std::vector<Pocket> out;
    if (s.size() < 3) return out;
    std::vector<int> ord(s.size());
    for (size_t i = 0; i < s.size(); i++) ord[i] = (int)i;
    std::sort(ord.begin(), ord.end(), [&](int a, int b){ return s[a].price < s[b].price; });
    size_t i = 0;
    while (i < ord.size()) {
        if (std::fabs(s[ord[i]].pct) < thinPct) {
            size_t j = i; float runSum = 0.0f;
            while (j + 1 < ord.size() && std::fabs(s[ord[j+1]].pct) < thinPct) j++;
            for (size_t q = i; q <= j; q++) runSum += s[ord[q]].pct;
            bool lowEdge  = (i > 0)              && std::fabs(s[ord[i-1]].pct) >= edgePct;
            bool highEdge = (j + 1 < ord.size()) && std::fabs(s[ord[j+1]].pct) >= edgePct;
            if ((int)(j - i + 1) >= minRun && lowEdge && highEdge) {
                Pocket p; p.lo = s[ord[i]].price; p.hi = s[ord[j]].price; p.n = (int)(j - i + 1); p.neg = runSum < 0;
                out.push_back(p);
            }
            i = j + 1;
        } else i++;
    }
    return out;
}

// ---- stacks (pika / barney) -------------------------------------------------
// The panel names a stack ONCE (PIKA / BARNEY on the biggest member) and marks the other members
// (PIKAM / BARNEYM). A stack = the contiguous run (in price order) of same-family tags.
struct Stack { float lo, hi; int n; bool pos; int namedIdx; };

inline bool isStackTag(const std::string& t, bool& pos, bool& named)
{
    if (t == "PIKA")    { pos = true;  named = true;  return true; }
    if (t == "PIKAM")   { pos = true;  named = false; return true; }
    if (t == "BARNEY")  { pos = false; named = true;  return true; }
    if (t == "BARNEYM") { pos = false; named = false; return true; }
    return false;
}

inline std::vector<Stack> stacks(const std::vector<Node>& s)
{
    std::vector<Stack> out;
    std::vector<int> ord(s.size());
    for (size_t i = 0; i < s.size(); i++) ord[i] = (int)i;
    std::sort(ord.begin(), ord.end(), [&](int a, int b){ return s[a].price < s[b].price; });
    size_t i = 0;
    while (i < ord.size()) {
        bool pos = false, named = false;
        if (!isStackTag(s[ord[i]].type, pos, named)) { i++; continue; }
        Stack st; st.lo = s[ord[i]].price; st.hi = st.lo; st.n = 1; st.pos = pos; st.namedIdx = named ? ord[i] : -1;
        size_t j = i;
        while (j + 1 < ord.size()) {
            bool p2, n2;
            if (!isStackTag(s[ord[j+1]].type, p2, n2) || p2 != pos) break;
            // a second NAMED member starts a new stack (two adjacent stacks of the same family)
            if (n2 && st.namedIdx >= 0) break;
            j++; st.hi = s[ord[j]].price; st.n++; if (n2) st.namedIdx = ord[j];
        }
        out.push_back(st);
        i = j + 1;
    }
    return out;
}

// Short tag for the bar: the named member carries P / B, the rug's yellow node R / RR; members carry nothing
// (v0.45: the bracket marks the run — the operator chose the bracket over a letter on every member).
inline const char* patternTag(const std::string& t)
{
    if (t == "PIKA")   return "P";
    if (t == "BARNEY") return "B";
    if (t == "RUG")    return "R";
    if (t == "RRUG" || t == "RREV" || t == "REVRUG") return "RR";
    if (t == "GK" || t == "GATEKEEPER") return "G";
    return "";
}

// ---- level -> node matching ------------------------------------------------
// A wall (CW / PW) tags the node with the same SPX strike; without a strike, the node within 1 pt.
inline int levelNode(const std::vector<Node>& s, float lvlPrice, float lvlSpx)
{
    for (size_t i = 0; i < s.size(); i++) {
        bool hit = (lvlSpx > 0.0f && s[i].spx > 0.0f) ? (std::fabs(lvlSpx - s[i].spx) < 0.01f)
                                                       : (std::fabs(lvlPrice - s[i].price) < 1.0f);
        if (hit) return (int)i;
    }
    return -1;
}

// ---- contract offset -------------------------------------------------------
// off = chartClose - anchor, anchor = SCALEREF (the front ES price the ladder is scaled to) else SPOT.
// Clamped to +-300: a wrong chart (NQ) or a bad anchor must never fling the book.
inline bool contractOffset(float chartClose, bool hasScaleRef, float scaleRef, bool hasSpot, float spot, float& off)
{
    off = 0.0f;
    if (!(chartClose > 0.0f)) return false;
    float anchor;
    if (hasScaleRef && scaleRef > 0.0f) anchor = scaleRef;
    else if (hasSpot)                   anchor = spot;
    else return false;
    off = chartClose - anchor;
    if (off < -300.0f || off > 300.0f) { off = 0.0f; return false; }
    return true;
}

// ---- (v0.59) the rank the rail draws with -----------------------------------
// Rank = Book: the within-book rank (field 4). Rank = Atlas merge: the POOLED rank (field 8, panel 16.40) — the row's
// place in the SPY + SPXW pool Atlas draws on the ES chart, each book scaled to its own King. A row the panel did not
// pool (older panel, or the IF book) gets NO_RANK, so it draws grey and unbadged — never a within-book rank in disguise.
static const int NO_RANK = 9999;
inline int effectiveRank(int bookRank, int pooledRank, bool atlasMerge) { return atlasMerge ? (pooledRank >= 1 ? pooledRank : NO_RANK) : bookRank; }

// ---- top-N / primary -------------------------------------------------------
inline int topNFor(int filter) { return (filter==0)?3 : (filter==1)?5 : (filter==2)?8 : (filter==3)?10 : 0; }
inline bool isPrimary(const Node& n, int filter, int thresh)
{
    int topN = topNFor(filter);
    if (filter <= 3) return (n.rank >= 1 && n.rank <= topN);
    if (filter == 4) return std::fabs(n.pct) >= (float)thresh;
    return true;
}

// ---- regime line text --------------------------------------------------------
struct RegimeText { std::string line; bool neg; bool at; bool na; };
inline RegimeText regimeLine(bool hasRow, const std::string& sign, const std::string& type, bool conflict, float netSum)
{
    RegimeText r; r.neg = false; r.at = false; r.na = false;
    if (hasRow) {
        r.neg = (sign == "NEG"); r.at = (sign == "AT"); r.na = !(r.neg || r.at || sign == "POS");
        std::string tact = (type.find("TREND") == 0) ? "FOLLOW, don't fade"
                         : (type == "WHIPSAW")       ? "fade EXTREMES only / sit out"
                         : (type == "RANGE")         ? "FADE the extremes"
                         :                             "no edge - wait";
        std::string sg = r.neg ? "-gamma" : (sign == "POS" ? "+gamma" : (r.at ? "AT flip" : "flip n/a"));
        std::string ty = type.empty() ? "FORMING" : type;
        if (ty == "TREND_UP") ty = "TREND UP"; else if (ty == "TREND_DN") ty = "TREND DOWN";
        r.line = "REGIME  " + sg + "  |  " + ty + "  |  " + tact + (conflict ? "  !CONFLICT" : "");
    } else {
        r.neg = netSum < 0;
        r.line = r.neg ? "REGIME (sum, no row)  FOLLOW / don't fade  (-gamma)"
                       : "REGIME (sum, no row)  FADE extremes  (+gamma)";
    }
    return r;
}

// (v0.60) THE STATUS LINE — what THIS instance did on its last draw, written to GammaProfile.status-<Book>-<Side>.txt
// beside the CSV, so two rails can be verified from outside (operator, 2026-09-17: "it displays one or the other but
// not both" — the file says whether the second instance loaded, where it anchored and how many bars it drew).
// Grammar:  GPSTATUS,<book Auto|SPX|SPY|IF>,<side Right|Left>,<file>,<strikes>,<rank Book|Atlas>,<paneL>,<paneR>,<anchor>,<colW>,<width>,<primary>,<drawn>,<rendered 1|0>
inline const char* bookName(int book) { return book == 1 ? "SPX" : book == 2 ? "SPY" : book == 3 ? "IF" : book == 4 ? "Both" : "Auto"; }
inline const char* bookFile(int book) { return book == 2 ? "GammaProfile-SPY.csv" : (book == 3 ? "GammaProfile-IF.csv" : "GammaProfile.csv"); }   // Both (4): the SPX file is the main rail; the SPY file is loaded beside it

// (v0.61) BOOK = BOTH — one instance, two rails (operator, 2026-09-17: "was there any reason why you didn't build an
// option to have both profiles so I don't have to add another gamma profile indicator" — no good reason). The main rail
// is the SPX book on the RIGHT (Side is ignored), the SPY rail on the LEFT with its own width; levels, regime and the
// read panel draw once, from the SPX file. Hide % under and every other setting are shared.
struct RailLayout { bool both; int mainSide; int spySide; int spyWidth; };
inline RailLayout railLayout(int book, int side, int width, int spyWidth)
{
    RailLayout r; r.both = (book == 4);
    r.mainSide = r.both ? 0 : side;                       // 0 = Right, 1 = Left
    r.spySide  = 1;
    r.spyWidth = r.both ? (spyWidth >= 40 ? (spyWidth <= 400 ? spyWidth : 400) : (spyWidth > 0 ? 40 : width)) : 0;   // 0 = no SPY rail
    (void)width;
    return r;
}
inline std::string statusLine(int book, int side, int strikes, bool atlasMerge, int paneL, int paneR, int anchor, int colW, int width,
                              int primary, int drawn, bool rendered)
{
    char buf[200];
    snprintf(buf, sizeof(buf), "GPSTATUS,%s,%s,%s,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d", bookName(book), side == 1 ? "Left" : "Right", bookFile(book),
             strikes, atlasMerge ? "Atlas" : "Book", paneL, paneR, anchor, colW, width, primary, drawn, rendered ? 1 : 0);
    return std::string(buf);
}

} // namespace gpl
#endif
