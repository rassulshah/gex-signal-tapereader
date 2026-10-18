#ifndef LS_HODLOD_Q_H
#define LS_HODLOD_Q_H
// HodLodQ.h (lsHodLod 2.3.0, 2026-10-17) -- MenthorQ's Q-Scores in the HOD / LOD box header. SDK-free (unit-tested on Linux).
// Rassul 2026-10-17 20:04-20:13 CT: "add / Qscore for momentum, option and volatility, where you get the qscore from menthorQ";
// "keep the Q scores to the right of the box. bullish green bearish red"; "put a colon after Q and a space"; "switch positions of
// qscore and Reg trend" (Q top left, Reg / trend top right); the hover text as in mockup hodlod-box-qscore-mockup-v2.png ("keep this in
// the hover over text for it"). Volatility is not bullish / bearish: always white (option (a), my default - he may change it).
// Input: lsFlexLevels\LRA-QScore-<MKT>.csv, written by the bridge (lra.gex_nodes.qscore_flex from the gateway's daily metrics list):
//   VERSION|1  MARKET|ES  DATE|2026-10-16  Q|M|O|V|S ('-' = none)  PREV|date|M|O|V|S  RECV|unix
// Scores are end of day (0 - 5): DATE = the session they were made at the close of.
#include <string>
#include <vector>
#include <cstdio>
#include "HodLodLogic.h"
#include "HodLodGex.h"
#include "Fresh.h"                 // (1.1.0) the one freshness rule

namespace hlq {

static const char* const VERSION = "1.1.0";   // 1.1.0 (HodLod 2.4.0, 2026-10-18): age check (Fresh.h judgeDaily) - stale scores read "Q: STALE", never as today's
static const size_t MAX_BYTES = 8 * 1024;
static const unsigned C_WHITE = 0x00FFFFFFu;

struct File {
    bool ok = false; std::string why, market, date, prevDate;
    int m = -1, o = -1, v = -1, s = -1;            // 0 - 5; -1 = none
    int pm = -1, po = -1, pv = -1;                 // the day before
    long long recv = 0;
};

inline bool score(const std::string& t, int& out) {
    if (t == "-") { out = -1; return true; }
    long long x; if (!hlg::fullL(t, x) || x < 0 || x > 5) return false; out = (int)x; return true;
}
inline bool dateOk(const std::string& d) {
    if (d.size() != 10 || d[4] != '-' || d[7] != '-') return false;
    for (size_t i = 0; i < d.size(); i++) if (i != 4 && i != 7 && (d[i] < '0' || d[i] > '9')) return false;
    return true;
}
// strict: VERSION 1, MARKET must match, DATE + Q required; a bad PREV / RECV is ignored
inline File parse(const std::string& text, const std::string& market) {
    File F; if (text.size() > MAX_BYTES) { F.why = "over 8 KiB"; return F; }
    bool ver = false, q = false; size_t a = 0; int lines = 0;
    while (a < text.size() && lines++ < 100) {
        size_t b = text.find('\n', a); std::string ln = text.substr(a, b == std::string::npos ? std::string::npos : b - a); a = b == std::string::npos ? text.size() : b + 1;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln.empty() || ln[0] == '#' || ln.size() > 300) continue;
        std::vector<std::string> c = hlg::split(ln, '|');
        if (c[0] == "VERSION") { if (c.size() < 2 || c[1] != "1") { F.why = "unknown file version"; return F; } ver = true; }
        else if (c[0] == "MARKET") { if (c.size() < 2 || c[1] != market) { F.why = "file is for another market"; return F; } F.market = c[1]; }
        else if (c[0] == "DATE") { if (c.size() < 2 || !dateOk(c[1])) { F.why = "bad DATE"; return F; } F.date = c[1]; }
        else if (c[0] == "Q") { if (c.size() < 5 || !score(c[1], F.m) || !score(c[2], F.o) || !score(c[3], F.v)) { F.why = "bad Q row"; return F; }
                                 if (!score(c[4], F.s)) F.s = -1;
                                 q = true; }
        else if (c[0] == "PREV") { int pm, po, pv; if (c.size() >= 5 && dateOk(c[1]) && score(c[2], pm) && score(c[3], po) && score(c[4], pv)) { F.prevDate = c[1]; F.pm = pm; F.po = po; F.pv = pv; } }
        else if (c[0] == "RECV") { long long r; if (c.size() >= 2 && hlg::fullL(c[1], r) && r >= 0) F.recv = r; }
    }
    if (!ver) { F.why = "no VERSION row"; return F; }
    if (F.market.empty()) { F.why = "no MARKET row"; return F; }
    if (F.date.empty() || !q) { F.why = "no scores"; return F; }
    if (F.m < 0 && F.o < 0 && F.v < 0) { F.why = "no scores"; return F; }
    F.ok = true; return F;
}
// bullish green (4 - 5), bearish red (0 - 1), neutral white (2 - 3); volatility always white (not a direction)
inline unsigned dirCol(int x) { return x < 0 ? hl::C_GREY : (x >= 4 ? hl::C_GREEN : (x <= 1 ? hl::C_RED : C_WHITE)); }
inline std::string num(int x) { return x < 0 ? std::string("-") : std::to_string(x); }
inline std::string mWord(int x) { return x < 0 ? "no score" : (x <= 1 ? "trend under pressure" : (x >= 4 ? "strong, well-supported trend" : "neutral / mixed trend")); }
inline std::string oWord(int x) { return x < 0 ? "no score" : (x <= 1 ? "options flow leaning bearish: put demand / downside hedging" : (x >= 4 ? "options flow leaning bullish: call buying / upside positioning" : "options flow neutral / mixed")); }
inline std::string vHead(int x) { return x < 0 ? "no score" : (x >= 4 ? "implied vol rich" : (x <= 2 ? "implied vol cheap" : "implied vol fair")); }
inline std::string vTail(int x) { return x < 0 ? "" : (x >= 4 ? ": bigger swings likely" : (x <= 2 ? ": calmer, swings can expand" : ": normal swings")); }
inline std::string mmdd(const std::string& d) {   // "2026-10-16" -> "Oct 16"
    static const char* M[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    if (!dateOk(d)) return d;
    int mo = (d[5] - '0') * 10 + (d[6] - '0'), dy = (d[8] - '0') * 10 + (d[9] - '0');
    if (mo < 1 || mo > 12) return d;
    return std::string(M[mo - 1]) + " " + std::to_string(dy);
}
// the header segments "Q: " + "M1" + " O1" + " V4" and the hover (the mockup's text, with the live numbers). trendWord = the box's
// trend ("S. Dntrend" / "Uptrend" / ...), "" when not ready; trendLvl -2..+2
// (1.1.0, audit C finding 7: "if the bridge stops, a week-old Q-Score shows as today's") the scores' age: the DATE must be the latest
// weekday whose scores were due by now (Fresh.h judgeDaily, due 22:00 CT); nowCt = CT wall-clock seconds, <= 0 = not judged (tests)
inline fr::Verdict fresh(const File& F, long long nowCt) {
    long long day = -1; if (!F.ok || !fr::parseYmd(F.date, day)) return fr::staleV(-1, false);
    return fr::judgeDaily(day, nowCt);
}
inline void toView(hl::View& v, const File& F, const std::string& trendWord, int trendLvl, const fr::Verdict* fv = nullptr) {
    v.qSegs.clear(); v.qHover.clear();
    if (!F.ok) return;
    if (fv && fv->stale()) {   // (1.1.0) stale: "Q: STALE" (grey), the old scores only in the hover with their date
        v.qSegs.push_back(hl::Seg{ "Q: ", C_WHITE });
        v.qSegs.push_back(hl::Seg{ "STALE", hl::C_GREY });
        hl::Line L0; L0.push_back(hl::Seg{ "MenthorQ Q-Scores: STALE - the newest on file are from the " + mmdd(F.date) + " close (no newer scores received).", C_WHITE }); v.qHover.push_back(L0);
        hl::Line L1; L1.push_back(hl::Seg{ "Then: M" + num(F.m) + "  O" + num(F.o) + "  V" + num(F.v) + " - not shown in the box until the bridge sends the current scores.", hl::C_GREY }); v.qHover.push_back(L1);
        return;
    }
    const unsigned W = C_WHITE, vc = F.v < 0 ? hl::C_GREY : C_WHITE;   // volatility: white (no direction)
    v.qSegs.push_back(hl::Seg{ "Q: ", W });
    v.qSegs.push_back(hl::Seg{ "M" + num(F.m), dirCol(F.m) });
    v.qSegs.push_back(hl::Seg{ "  O" + num(F.o), dirCol(F.o) });
    v.qSegs.push_back(hl::Seg{ "  V" + num(F.v), vc });
    // the mockup's hover (v2, approved 20:20): title, M / O / V (the code in its colour, the meaning white), how to use, an example
    auto line = [&](const std::string& code, unsigned col, const std::string& rest) { hl::Line L; L.push_back(hl::Seg{ code, col }); L.push_back(hl::Seg{ rest, W }); v.qHover.push_back(L); };
    { hl::Line L; L.push_back(hl::Seg{ "MenthorQ Q-Scores (end of day, 0 - 5, as of " + mmdd(F.date) + " close)", W }); v.qHover.push_back(L); }
    line("M" + num(F.m), dirCol(F.m), " Momentum " + num(F.m) + " - " + mWord(F.m) + " (0 - 1 bearish, 2 - 3 neutral, 4 - 5 bullish).");
    line("O" + num(F.o), dirCol(F.o), " Option " + num(F.o) + " - " + oWord(F.o) + " (0 bearish, 3 neutral, 5 bullish).");
    line("V" + num(F.v), vc, " Volatility " + num(F.v) + " - " + vHead(F.v) + " (4+ overpriced, 2 or less cheap)" + vTail(F.v) + ".");
    { hl::Line L; L.push_back(hl::Seg{ "How to use: M and O agreeing with the trend = trade with it; against it = take turns at levels more seriously.", W }); v.qHover.push_back(L); }
    // the example: the live trend and scores when they agree on a side, else the mockup's example
    const bool bear = F.m >= 0 && F.m <= 1 && F.o >= 0 && F.o <= 1, bull = F.m >= 4 && F.o >= 4;
    std::string ex;
    if (bear && !trendWord.empty() && trendLvl < 0) ex = "Example: " + trendWord + " with M" + num(F.m) + " O" + num(F.o) + " = sellers in control - favour shorts on pullbacks into Gex S/R above (red).";
    else if (bull && !trendWord.empty() && trendLvl > 0) ex = "Example: " + trendWord + " with M" + num(F.m) + " O" + num(F.o) + " = buyers in control - favour longs on pullbacks into Gex S/R below (green).";
    else ex = "Example: S. Dntrend with M1 O1 = sellers in control - favour shorts on pullbacks into Gex S/R above (red).";
    { hl::Line L; L.push_back(hl::Seg{ ex, W }); v.qHover.push_back(L); }
}
inline std::string statusLine(const File& F, const std::string& why, const fr::Verdict* fv = nullptr) {
    if (!F.ok) return "QSCORE,none," + (why.empty() ? F.why : why) + "\n";
    return "QSCORE," + F.date + ",M" + num(F.m) + ",O" + num(F.o) + ",V" + num(F.v) + ",S" + num(F.s) + (fv ? std::string(",") + fr::word(fv->st) : std::string()) + "\n";
}

}   // namespace hlq
#endif
