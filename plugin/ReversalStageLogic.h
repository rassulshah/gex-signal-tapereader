#ifndef LS_REVERSAL_STAGE_LOGIC_H
#define LS_REVERSAL_STAGE_LOGIC_H
// ReversalStageLogic.h 1.0.0 (2026-10-22, Rassul approved layout-stages-2strips.png 11:41-11:46 CT, build ordered 15:20 CT) - SDK-free logic of
// the REVERSAL STAGE pane (lsReversalStage 1.0.0): "Which stage is the pivot in?". Unit-tested on Linux (tests_strips/test_stage.cpp).
//
// The unit is a LOCAL PIVOT ATTEMPT: it starts on a 5-candle low (high) - beyond the 5 candles before it, the same extreme rule as the
// attacker / Defense letters - and climbs a staircase from the letters the two order-flow panes recorded (LetterStrips.h record):
//   1 Attack     S / K / L (attacker pane)            4 Turn        H (Defense) / R stop run reclaimed (attacker pane)
//   2 Absorbed   A / P (defenders' book) / Q (queue)  5 Re-test     L re-test reloaded (Defense)
//   3 Exhausted  G / Tr (attacker pane)               6 Expansion   the close >= 1 ATR (14 candles) away from the extreme
// The stage is the highest step reached (steps may be skipped). It FAILS on B (the defenders' own book stepped back), X (re-test empty) or
// the extreme taken out once the attempt has reached stage 2+ (taken out at stage 0-1 = the attack is still going: the extreme moves).
// Order gate (stepOf): H needs stage 2+, the re-test L / X need stage 4+ (else they are an older turn's re-test), B needs stage 2+.
// W (bids withdrew at the level) is a warning: hover only, no step (the code's W is a withdrawal, not a wall). P / B on the ATTACKERS'
// book: hover only. An attempt at stage 0-1 gives way to a new extreme on the other side; one at stage 2+ does not. It expires after
// 5 candles at stage 0 or 30 candles without completing. Green steps = a LOW pivot being built, red = a HIGH pivot.
// Decided once per CLOSED candle as a pure function of the closed candles and the letters keyed to them (run(), lesson 136): the host
// appends a candle to its record only when it is closed and settled, and draws from the record - never repainted.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "LetterStrips.h"

namespace rsl {

static const char* const VERSION = "1.2.0";   // (1.2.0, 2026-10-22 evening fix build: "NOW HIGH stage 2 failed chain Q" - stages climbed on queue letters alone) a Q steps (stage 2) only when its anchor (the "tk<N>" token lsLiquidityProfile 0.51.0 writes) is AT the attempt's own extreme: within max(4 ticks, 0.3 ATR); a Q without an anchor or elsewhere = hover only
   // (1.1.0) the host adds the news / settle "?" and the feed check (NewsWindow.h); the stage logic is unchanged
enum { LOW = lst::LOW, HIGH = lst::HIGH };
enum Status { NONE = 0, FORMING = 1, FAILED = 2, COMPLETE = 3, EXPIRED = 4 };
static const int EXT_BARS = 5, ATR_BARS = 14, IDLE_BARS = 5, MAX_BARS = 30, FAIL = -1;
static const char* const ODDS_TICKET = "T-20261022-39";

inline const char* stageName(int s) {
    static const char* const N[] = { "", "Attack", "Absorbed", "Exhausted", "Turn", "Re-test", "Expansion" };
    return s >= 1 && s <= 6 ? N[s] : "";
}
inline const char* statusName(int s) { return s == FORMING ? "forming" : s == FAILED ? "failed" : s == COMPLETE ? "complete" : s == EXPIRED ? "expired" : ""; }

struct Bar { long long key = 0, h = 0, l = 0, c = 0; bool ok = false; };            // prices in ticks
struct Ev { long long key = 0; int pane = lst::ATK; lst::Lt l; };
// the step a letter gives (FAIL = the attempt failed, 0 = hover only), ignoring order - see stepOf for the gate
inline int stageOf(const Ev& e) {
    const std::string& c = e.l.code;
    if (e.pane == lst::ATK) { if (c == "S" || c == "K" || c == "L") return 1; if (c == "G" || c == "Tr") return 3; if (c == "R") return 4; return 0; }
    if (e.pane == lst::QUE) return c == "Q" ? 2 : 0;
    const int defBook = e.l.pivot == LOW ? 0 : 1;                                     // bids defend a low, offers a high
    if (c == "A") return 2;
    if (c == "P") return e.l.book == defBook ? 2 : 0;
    if (c == "B") return e.l.book == defBook ? FAIL : 0;
    if (c == "H") return 4;
    if (c == "L") return 5;
    if (c == "X") return FAIL;
    return 0;                                                                          // W: warning, hover only
}

// the step a letter gives to an attempt now at stage `cur` (replay ES 2026-10-22: re-test letters of OLDER turns landed on fresh extremes
// and jumped them to stage 5 / failed them): H needs the absorption first (stage 2+), the re-test L / X need the turn first (stage 4+),
// B needs the hold first (stage 2+); otherwise the letter is hover only (0)
static const int NEED_H = 2, NEED_RETEST = 4, NEED_B = 2;
inline int stepOf(const Ev& e, int cur) {
    const int st = stageOf(e); const std::string& c = e.l.code;
    if (e.pane == lst::DEF && c == "H" && cur < NEED_H) return 0;
    if (e.pane == lst::DEF && (c == "L" || c == "X") && cur < NEED_RETEST) return 0;
    if (e.pane == lst::DEF && c == "B" && st == FAIL && cur < NEED_B) return 0;
    return st;
}

// (1.2.0) the Q's anchor in ticks from its key numbers ("held 120% orig 97% q0 130 tk30826"); 0 = not given (older records)
inline long long qAnchorOf(const std::string& nums) {
    const size_t p = nums.rfind(" tk"); if (p == std::string::npos) return 0;
    long long v = 0; size_t k = p + 3; if (k >= nums.size()) return 0;
    for (; k < nums.size(); ++k) { const char c = nums[k]; if (c < '0' || c > '9') return 0; v = v * 10 + (c - '0'); if (v > 4000000000000LL) return 0; }
    return v;
}
inline std::string numsShown(const std::string& n) { const size_t p = n.rfind(" tk"); return p != std::string::npos && qAnchorOf(n) > 0 ? n.substr(0, p) : n; }
inline long long qTolTicks(double atr) { return std::max(4LL, (long long)std::llround(0.3 * atr)); }
// (1.2.0) the step of a letter for THIS attempt: a queue Q only at the attempt's own extreme
inline int stepFor(const Ev& e, int cur, long long ext, double atr) {
    if (e.pane == lst::QUE) {
        if (e.l.code != "Q") return 0;
        const long long at = qAnchorOf(e.l.nums); if (at <= 0) return 0;
        return std::llabs(at - ext) <= qTolTicks(atr) ? 2 : 0;
    }
    return stepOf(e, cur);
}

// one candle's step (what the pane draws and records)
struct Step {
    long long key = 0; int side = -1, stage = 0, status = NONE;
    long long ext = 0, start = 0;                       // the attempt's extreme (ticks) and its first candle
    int failedStage = 0; std::string why;               // a failed / expired attempt on this candle: its stage and the reason
    std::string chain;                                  // the attempt's letters so far, candles separated by " . " ("K . A Tr")
    std::string here;                                   // the letters applied on this candle with their numbers ("A 1,940 lots 2.6x; Tr ...")
    bool operator==(const Step& o) const { return key == o.key && side == o.side && stage == o.stage && status == o.status && ext == o.ext && start == o.start && failedStage == o.failedStage && why == o.why && chain == o.chain && here == o.here; }
};

inline double atrAt(const std::vector<Bar>& b, size_t i) {
    double s = 0; int k = 0;
    for (size_t j = i + 1; j-- > 1 && k < ATR_BARS;) { if (!b[j].ok || !b[j - 1].ok) continue;
        s += (double)std::max(b[j].h - b[j].l, std::max(std::llabs(b[j].h - b[j - 1].c), std::llabs(b[j].l - b[j - 1].c))); ++k; }
    return k >= 5 ? s / k : 0.0;
}
// a 5-candle low / high: beyond each of the 5 candles before it (all must be known)
inline void extremeOf(const std::vector<Bar>& b, size_t i, bool& isLow, bool& isHigh) {
    isLow = isHigh = false; if (i < (size_t)EXT_BARS || !b[i].ok) return;
    long long mn = b[i - 1].l, mx = b[i - 1].h;
    for (size_t j = i - EXT_BARS; j < i; ++j) { if (!b[j].ok) return; mn = std::min(mn, b[j].l); mx = std::max(mx, b[j].h); }
    isLow = b[i].l < mn; isHigh = b[i].h > mx;
}

struct Attempt { bool on = false; int side = LOW, stage = 0; long long ext = 0, start = 0; size_t startIdx = 0; std::string chain; };

// run the staircase over closed candles b (oldest first) with the letters keyed to them; one Step per candle
inline std::vector<Step> run(const std::vector<Bar>& b, const std::multimap<long long, Ev>& evs) {
    std::vector<Step> out; out.reserve(b.size());
    Attempt A;
    for (size_t i = 0; i < b.size(); ++i) {
        Step s; s.key = b[i].key;
        std::vector<const Ev*> here[2];
        for (std::multimap<long long, Ev>::const_iterator it = evs.lower_bound(b[i].key); it != evs.end() && it->first == b[i].key; ++it) here[it->second.l.pivot == HIGH ? 1 : 0].push_back(&it->second);
        auto best = [&](int sd) { int m = 0; for (const Ev* e : here[sd]) m = std::max(m, stageOf(*e)); return m; };
        bool isLow = false, isHigh = false; extremeOf(b, i, isLow, isHigh);
        const double atr = atrAt(b, i);
        auto endAttempt = [&](int status, const std::string& why) { s.status = status; s.failedStage = A.stage; s.why = why; s.side = A.side; s.ext = A.ext; s.start = A.start; s.chain = A.chain; A = Attempt(); };
        auto lettersText = [&](int sd, int cur, std::string& chainPart, std::string& hereTxt) {
            std::vector<const Ev*> v = here[sd];
            std::stable_sort(v.begin(), v.end(), [](const Ev* a, const Ev* b) { const int x = stageOf(*a), y = stageOf(*b); return (x == FAIL ? 99 : x) < (y == FAIL ? 99 : y); });
            for (const Ev* e : v) {
                const int st = stepFor(*e, cur, A.ext, atr); if (st > 0) cur = std::max(cur, st);
                const std::string tag = e->l.code + (st == FAIL ? "!" : st == 0 ? "*" : "");
                chainPart += (chainPart.empty() ? "" : " ") + tag;
                hereTxt += (hereTxt.empty() ? "" : "; ") + e->l.code + (e->l.nums.empty() ? std::string() : " " + numsShown(e->l.nums)) + (st == 0 ? " (hover only)" : st == FAIL ? " (failed it)" : "");
            }
        };
        if (!b[i].ok) { out.push_back(s); continue; }
        // 1) the active attempt: its extreme taken out, a new extreme on the other side, expiry
        if (A.on) {
            const bool takenOut = A.side == LOW ? b[i].l < A.ext : b[i].h > A.ext;
            if (takenOut && A.stage >= 2) endAttempt(FAILED, A.side == LOW ? "the low was taken out" : "the high was taken out");
            else if (takenOut) { A.ext = A.side == LOW ? b[i].l : b[i].h; }        // stage 0-1: the attack is still going - the extreme moves
            else {
                const int other = 1 - A.side; const bool otherExt = other == LOW ? isLow : isHigh;
                if (otherExt && A.stage <= 1) endAttempt(EXPIRED, "a new extreme on the other side");
                else if ((A.stage == 0 && i - A.startIdx >= (size_t)IDLE_BARS) || i - A.startIdx >= (size_t)MAX_BARS) endAttempt(EXPIRED, "no progress in time");
            }
        }
        const bool ended = s.status != NONE;
        // 2) a new attempt on a 5-candle extreme (the failed / expired candle may start the next one)
        if (!A.on) {
            int sd = -1;
            if (isLow && isHigh) { const int bl = best(LOW), bh = best(HIGH); sd = bl > bh ? LOW : bh > bl ? HIGH : -1; }
            else if (isLow) sd = LOW; else if (isHigh) sd = HIGH;
            if (sd >= 0) {
                A.on = true; A.side = sd; A.stage = 0; A.ext = sd == LOW ? b[i].l : b[i].h; A.start = b[i].key; A.startIdx = i; A.chain.clear();
            }
        }
        // 3) the letters of this candle for the attempt's side
        if (A.on) {
            std::string part, hereTxt; lettersText(A.side, A.stage, part, hereTxt);
            if (!part.empty()) A.chain += (A.chain.empty() ? "" : " . ") + part;
            bool failed = false; int m = A.stage;
            {   std::vector<const Ev*> v = here[A.side];             // the candle's letters in step order: A before H before L / X
                std::stable_sort(v.begin(), v.end(), [](const Ev* a, const Ev* b) { const int x = stageOf(*a), y = stageOf(*b); return (x == FAIL ? 99 : x) < (y == FAIL ? 99 : y); });
                for (const Ev* e : v) { const int st = stepFor(*e, m, A.ext, atr); if (st == FAIL) failed = true; else m = std::max(m, st); } }
            if (failed && m >= 1) {                                  // (the stage this candle reached before the warning)
                A.stage = m;
                const std::string fwhy = "the defenders' warning letter (" + part + ")";
                s.here = hereTxt; endAttempt(FAILED, fwhy);
            } else {
                A.stage = m;
                // 4) expansion: the close 1 ATR away from the extreme
                if (atr > 0 && A.stage >= 1 && (A.side == LOW ? (double)(b[i].c - A.ext) : (double)(A.ext - b[i].c)) >= atr) {
                    s.here = hereTxt; A.stage = 6; s.stage = 6; s.status = COMPLETE; s.side = A.side; s.ext = A.ext; s.start = A.start; s.chain = A.chain + " . 1 ATR";
                    s.failedStage = 0; s.why = "the close left the extreme by 1 ATR"; A = Attempt();
                } else if (!ended) {
                    s.side = A.side; s.stage = A.stage; s.status = A.stage > 0 ? FORMING : NONE; s.ext = A.ext; s.start = A.start; s.chain = A.chain; s.here = hereTxt;
                } else if (A.stage > 0) {                       // the candle that ended the old attempt shows the new one's first step
                    s.stage = A.stage; s.here = hereTxt + (hereTxt.empty() ? "" : "; ") + "new " + (A.side == LOW ? "LOW" : "HIGH") + " attempt";
                    if (s.side != A.side) { s.why += " - now a " + std::string(A.side == LOW ? "LOW" : "HIGH") + " attempt"; s.side = A.side; }
                }
            }
        }
        if (s.side < 0) s.status = NONE;
        out.push_back(s);
    }
    return out;
}

// ---- the record: one RS line per decided candle (also empty ones, so a restart knows where it stopped) ---------------------------------
// RS|1|<key>|<side -1/0/1>|<stage 0-6>|<status 0-4>|<ext ticks>|<start key>|<failed stage>|<why>|<chain>|<here>
inline std::string cleanF(const std::string& s) { std::string o; for (char c : s) if (c != '|' && c != '\r' && c != '\n') o += c; return o.size() > 200 ? o.substr(0, 200) : o; }
inline std::string line(const Step& s) {
    char b[160]; std::snprintf(b, sizeof b, "RS|1|%lld|%d|%d|%d|%lld|%lld|%d|", s.key, s.side, s.stage, s.status, s.ext, s.start, s.failedStage);
    return std::string(b) + cleanF(s.why) + "|" + cleanF(s.chain) + "|" + cleanF(s.here);
}
inline bool parse(const std::string& s0, Step& s) {
    std::string t = s0; while (!t.empty() && (t.back() == '\r' || t.back() == '\n')) t.pop_back();
    std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t p = t.find('|', a); f.push_back(t.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; if (f.size() > 12) return false; }
    if (f.size() != 12 || f[0] != "RS" || f[1] != "1") return false;
    long long key = 0, side = 0, stage = 0, st = 0, ext = 0, start = 0, fs = 0;
    if (!lst::fullLL(f[2], key) || key <= 0 || !lst::fullLL(f[3], side) || side < -1 || side > 1 || !lst::fullLL(f[4], stage) || stage < 0 || stage > 6
        || !lst::fullLL(f[5], st) || st < 0 || st > 4 || !lst::fullLL(f[6], ext) || !lst::fullLL(f[7], start) || start < 0 || !lst::fullLL(f[8], fs) || fs < 0 || fs > 6) return false;
    if (side < 0 && (stage != 0 || st != NONE)) return false;
    Step x; x.key = key; x.side = (int)side; x.stage = (int)stage; x.status = (int)st; x.ext = ext; x.start = start; x.failedStage = (int)fs; x.why = f[9]; x.chain = f[10]; x.here = f[11];
    s = x; return true;
}
inline std::string fileName(const std::string& mkt, int spb) { return "RS-" + (mkt == "6E" ? std::string("EU") : mkt) + "-" + std::to_string(spb) + ".csv"; }

// ---- what the pane draws for a recorded step: the bar height (stage, 0 = nothing; a failed step with no new attempt = a stub) ---------
struct Look { int height = 0; int colour = lst::GREEN; bool dim = false; bool stub = false; };
inline Look lookOf(const Step& s) {
    Look k; if (s.side < 0) return k;
    k.colour = s.side == LOW ? lst::GREEN : lst::RED;
    if (s.status == FAILED && s.stage == 0) { k.stub = true; k.dim = true; k.height = 0; return k; }   // the step dropped back
    if (s.status == EXPIRED && s.stage == 0) return k;
    k.height = s.stage; k.dim = s.stage <= 1; return k;
}

// ---- the Now box: fixed size, at one of 9 spots (0 top left .. 8 bottom right) inside [l, r) x [t, b) with a margin; false = no room
inline bool boxAt(int pos, int l, int t, int r, int b, int w, int h, int margin, int& x, int& y) {
    if (pos < 0 || pos > 8) pos = 0;
    if (r - l < w + 2 * margin || b - t < h + 2 * margin) return false;
    const int col = pos % 3, row = pos / 3;
    x = col == 0 ? l + margin : col == 1 ? l + (r - l - w) / 2 : r - margin - w;
    y = row == 0 ? t + margin : row == 1 ? t + (b - t - h) / 2 : b - margin - h;
    return true;
}
// the pixel row of stage value v in a pane [top, bottom] whose axis is 0 .. AXIS_MAX (the same mapping IRT uses for scale())
static const double AXIS_MAX = 6.5;
inline int yOf(double v, int top, int bottom) { return bottom - (int)std::lround(v / AXIS_MAX * (double)(bottom - top)); }

// ---- texts ----------------------------------------------------------------------------------------------------------------------------
inline std::string sideWord(int side) { return side == LOW ? "LOW" : side == HIGH ? "HIGH" : "-"; }
// the "Now" box rows: current attempt, stage, chain, odds
inline std::vector<std::string> nowRows(const Step& s, const std::string& when) {
    std::vector<std::string> r;
    r.push_back("Now " + when);
    if (s.side < 0 || (s.stage == 0 && s.status != FAILED)) { r.push_back("No attempt"); r.push_back("Stage -"); r.push_back("Chain -"); }
    else {
        r.push_back(sideWord(s.side) + " attempt");
        if (s.status == FAILED && s.stage == 0) r.push_back("Stage " + std::to_string(s.failedStage) + ", failed");
        else if (s.status == COMPLETE) r.push_back("Stage 6, complete");
        else r.push_back("Stage " + std::to_string(s.stage) + " " + stageName(s.stage) + (s.status == FAILED ? " (last failed)" : ""));
        r.push_back("Chain " + (s.chain.empty() ? std::string("-") : s.chain));
    }
    r.push_back(std::string("Odds ?  (") + ODDS_TICKET + ")");
    return r;
}
// the hover of one step: his format - the question, how the step answers it (the letters and numbers), how to use it, an example
inline std::vector<std::string> hoverRows(const Step& s, const std::string& when) {
    std::vector<std::string> r;
    const std::string piv = s.side == LOW ? "LOW" : "HIGH";
    std::string head = "Reversal stage  " + when + "  -  " + piv + " attempt";
    if (s.status == FAILED && s.stage == 0) head += ", failed at stage " + std::to_string(s.failedStage);
    else head += ", stage " + std::to_string(s.stage) + " " + stageName(s.stage) + (s.status == COMPLETE ? " (complete)" : s.status == FAILED ? " (the attempt before failed)" : "");
    r.push_back(head);
    r.push_back(std::string("Question: which stage is this ") + (s.side == LOW ? "low" : "high") + " pivot in - attack, absorbed, exhausted, turn, re-test, expansion?");
    r.push_back("   Letters behind it: " + (s.chain.empty() ? std::string("none yet (the 5-candle extreme only)") : s.chain));
    if (!s.here.empty()) r.push_back("   On this candle: " + s.here);
    if (!s.why.empty()) r.push_back("   " + std::string(s.status == FAILED ? "Failed: " : s.status == EXPIRED ? "Ended: " : "") + s.why);
    r.push_back("   Use: the higher the step, the further the pivot got; a step dropping back = the attempt failed - wait for the next one");
    r.push_back(std::string("   Example: green K . A Tr . H = sellers hit the low, bids absorbed, sellers trapped, buyers hit back (stage 4)"));
    r.push_back(std::string("   Odds: ? - not proven yet (") + ODDS_TICKET + ")");
    return r;
}

// rows wrapped at word boundaries to <= width characters; a continuation is indented 5 spaces (the hover stays narrow on small panes)
inline std::vector<std::string> wrap(const std::vector<std::string>& rows, size_t width = 78) {
    std::vector<std::string> o;
    for (const std::string& r0 : rows) {
        std::string r = r0; bool first = true;
        while (r.size() > width) {
            size_t cut = r.rfind(' ', width); if (cut == std::string::npos || cut < 10) cut = width;
            o.push_back(r.substr(0, cut)); r = r.substr(cut); while (!r.empty() && r[0] == ' ') r.erase(0, 1); r = "     " + r; first = false;
        }
        (void)first; o.push_back(r);
    }
    return o;
}
} // namespace rsl
#endif
