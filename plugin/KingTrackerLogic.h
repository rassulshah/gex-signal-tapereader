// KingTrackerLogic.h — lsKingTracker's decisions on plain data, no SDK. Pinned by plugin/test_kingtracker_logic.cpp.
//   · the KINGTRACK / KINGNOW row grammar
//   · (v0.6) history re-derived in the CURRENT scale: strike × (nowPx / nowStrike), so a mid-session roll of Skylit's
//     ES1 (2026-09-16: 7688 → 7757 for the same SPX 7685) does not leave a phantom step
//   · the anchor for the contract offset (SCALEREF first, SPOT as the fallback) and its clamp
//   · the stale age with the overnight wrap (shared shape with lsDayModel)
//   · (v0.11) the Source switch: Skylit's tape books or IF's Magnet books, one or the other
// The anchor BAR itself is ContractOffsetLogic.h (shared with lsGammaProfile).
#pragma once
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cfloat>
#include <climits>
#include <cmath>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>

namespace ktl {

struct Step { double so; float px; int strike; int pct; Step() : so(0), px(0), strike(0), pct(0) {} };   // (v0.13) pct = the step's polarity (+/-100), 0 when the row did not carry it
struct Book { std::vector<Step> steps; bool hasNow; float nowPx; int nowStrike, nowPct; Book() : hasNow(false), nowPx(0), nowStrike(0), nowPct(0) {} };

// CSV is an external boundary.  Do not let atoi/atof silently convert malformed
// fields (or NaN/Inf) to prices, clocks, and contract offsets.
inline bool parseFiniteDouble(const std::string& text, double& value)
{
    if (text.empty()) return false;
    const char* begin = text.c_str();
    char* end = 0;
    errno = 0;
    double parsed = std::strtod(begin, &end);
    while (end && *end && std::isspace((unsigned char)*end)) ++end;
    if (end == begin || !end || *end != '\0' || errno == ERANGE || !std::isfinite(parsed)) return false;
    value = parsed;
    return true;
}
inline bool parseInt(const std::string& text, int& value)
{
    if (text.empty()) return false;
    const char* begin = text.c_str();
    char* end = 0;
    errno = 0;
    long parsed = std::strtol(begin, &end, 10);
    while (end && *end && std::isspace((unsigned char)*end)) ++end;
    if (end == begin || !end || *end != '\0' || errno == ERANGE || parsed < INT_MIN || parsed > INT_MAX) return false;
    value = (int)parsed;
    return true;
}
inline bool parsePositiveFloat(const std::string& text, float& value)
{
    double parsed = 0.0;
    if (!parseFiniteDouble(text, parsed) || !(parsed > 0.0) || parsed > FLT_MAX) return false;
    value = (float)parsed;
    return true;
}
inline bool validSecOfDay(double value) { return std::isfinite(value) && value >= 0.0 && value < 86400.0; }
inline bool validFamily(const std::string& family) { return family == "ES" || family == "NQ"; }
inline bool parseYmd(const std::string& text, int& year, int& month, int& day)
{
    size_t first = text.find('-');
    size_t second = first == std::string::npos ? std::string::npos : text.find('-', first + 1);
    if (first == std::string::npos || second == std::string::npos || text.find('-', second + 1) != std::string::npos ||
        !parseInt(text.substr(0, first), year) || !parseInt(text.substr(first + 1, second - first - 1), month) ||
        !parseInt(text.substr(second + 1), day) || year <= 0 || month < 1 || month > 12 || day < 1) return false;
    static const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int days = daysInMonth[month - 1];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) days = 29;
    return day <= days;
}

// KINGTRACK,<fam>,<book>,<so>,<px>,<strike>  → a step;  KINGNOW,<fam>,<book>,<px>,<strike>[,<pct>]  → the current King
inline bool parseTrack(const std::vector<std::string>& t, std::string& fam, std::string& book, Step& st)
{
    if (t.size() < 6 || t[0] != "KINGTRACK") return false;
    double so = 0.0; float px = 0.0f; int strike = 0, pct = 0;
    if (!validFamily(t[1]) || t[2].empty() || !parseFiniteDouble(t[3], so) || !validSecOfDay(so) ||
        !parsePositiveFloat(t[4], px) || !parseInt(t[5], strike) || strike <= 0 ||
        (t.size() > 6 && !t[6].empty() && !parseInt(t[6], pct))) return false;
    fam = t[1]; book = t[2]; st.so = so; st.px = px; st.strike = strike; st.pct = pct;   // (v0.13) panel 16.39 writes the polarity; older rows leave it 0
    return true;
}
inline bool parseNow(const std::vector<std::string>& t, std::string& fam, std::string& book, Book& b)
{
    if (t.size() < 5 || t[0] != "KINGNOW") return false;
    float nowPx = 0.0f; int nowStrike = 0, nowPct = 0;
    if (!validFamily(t[1]) || t[2].empty() || !parsePositiveFloat(t[3], nowPx) || !parseInt(t[4], nowStrike) || nowStrike <= 0 ||
        (t.size() > 5 && !t[5].empty() && !parseInt(t[5], nowPct))) return false;
    fam = t[1]; book = t[2]; b.nowPx = nowPx; b.nowStrike = nowStrike; b.nowPct = nowPct; b.hasNow = true; return true;
}

// The file is external and can be rewritten out of order.  Rendering assumes
// ascending time, so make the step path deterministic before it reaches draw().
inline void sortSteps(Book& b)
{
    std::sort(b.steps.begin(), b.steps.end(), [](const Step& a, const Step& z) {
        if (a.so != z.so) return a.so < z.so;
        if (a.strike != z.strike) return a.strike < z.strike;
        if (a.px != z.px) return a.px < z.px;
        return a.pct < z.pct;
    });
}

// every step's price re-derived as strike × (nowPx / nowStrike) — only when KINGNOW gives the ratio
inline void rederive(Book& b)
{
    if (!(b.hasNow && b.nowStrike > 0 && b.nowPx > 0.0f && std::isfinite(b.nowPx))) return;
    double r = (double)b.nowPx / (double)b.nowStrike;
    if (!(r > 0.0) || !std::isfinite(r)) return;
    for (size_t i = 0; i < b.steps.size(); i++) if (b.steps[i].strike > 0) {
        double derived = (double)b.steps[i].strike * r;
        if (derived > 0.0 && derived <= FLT_MAX && std::isfinite(derived)) b.steps[i].px = (float)derived;
    }
}

// the anchor the offset is measured against: SCALEREF when present, else SPOT; false when neither
inline bool anchorPrice(bool hasScaleRef, float scaleRef, bool hasSpot, float spotPx, float& anchor)
{
    if (hasScaleRef && scaleRef > 0.0f && std::isfinite(scaleRef)) { anchor = scaleRef; return true; }
    if (hasSpot && spotPx > 0.0f && std::isfinite(spotPx)) { anchor = spotPx; return true; }
    return false;
}
inline bool offsetFor(float chartClose, float anchor, float& off)
{
    if (!(chartClose > 0.0f) || !(anchor > 0.0f) || !std::isfinite(chartClose) || !std::isfinite(anchor)) return false;
    off = chartClose - anchor;
    if (!std::isfinite(off) || off < -300.0f || off > 300.0f) return false;   // implausible (incl. the ES/NQ cross)
    if (off > -0.01f && off < 0.01f) return false;      // already aligned
    return true;
}
inline void shift(Book& b, float off) { if (!std::isfinite(off)) return; for (size_t i = 0; i < b.steps.size(); i++) b.steps[i].px += off; if (b.hasNow) b.nowPx += off; }

// (v0.11) THE SOURCE SWITCH — Skylit (the tape King: SPX/SPY on ES, QQQ/NDX on NQ) or IF (InsiderFinance's 0DTE Magnet,
// the panel's `IF` book on ES and `IFQ` on NQ). One or the other, never both — the operator's rule for this indicator
// ("switch back and forth between IF and Skylit"). A book of the other chart family is never drawn either way.
inline bool bookDrawn(const std::string& book, const std::string& bookFam, const std::string& chartFam, bool sourceIF)
{
    if (bookFam != chartFam) return false;
    bool isIF = (book == "IF" || book == "IFQ");
    bool isSkylit = (book == "SPX" || book == "SPY" || book == "QQQ" || book == "NDX");
    return sourceIF ? isIF : isSkylit;
}

// (v0.13) THE IF MAGNET'S COLOUR IS ITS POLARITY (operator: "colour should depend on whether it is positive or negative
// gamma") — the gamma profile's own gold / magenta. Each step is coloured by the polarity it was sampled with; a step
// from a pre-16.39 row (pct 0) and the live right edge take KINGNOW's polarity; nothing known -> positive (gold).
inline int polarity(int stepPct, const Book& b)
{
    if (stepPct < 0) return -1;
    if (stepPct > 0) return 1;
    if (b.hasNow && b.nowPct < 0) return -1;
    return 1;
}

// (v0.14) THE STATUS LINE — what this draw put on the chart, one line per book, written to KingTracker.status.txt beside
// the CSV so the toggle can be VERIFIED from outside (the operator flips Source, the file says which journey is drawn).
// Grammar:  KTSTATUS,<source Skylit|IF>,<chartFam>,<book>,<drawn 1|0>,<steps>,<nowStrike>,<nowPct>,<offset>
inline std::string statusLine(bool sourceIF, const std::string& chartFam, const std::string& book, const std::string& bookFam, const Book& b, float off)
{
    char buf[160];
    bool drawn = bookDrawn(book, bookFam, chartFam, sourceIF) && (!b.steps.empty() || b.hasNow);
    snprintf(buf, sizeof(buf), "KTSTATUS,%s,%s,%s,%d,%d,%d,%d,%.2f", sourceIF ? "IF" : "Skylit", chartFam.c_str(), book.c_str(), drawn ? 1 : 0,
             (int)b.steps.size(), b.hasNow ? b.nowStrike : 0, b.hasNow ? b.nowPct : 0, off);
    return std::string(buf);
}

inline double staleAge(double asofSo, double localSo)
{
    if (!validSecOfDay(asofSo) || !validSecOfDay(localSo)) return -1.0;
    return (asofSo > localSo + 300.0) ? ((86400.0 - asofSo) + localSo) / 60.0 : (localSo - asofSo) / 60.0;
}

} // namespace ktl
