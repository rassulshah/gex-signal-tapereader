// KingTrackerLogic.h — lsKingTracker's decisions on plain data, no SDK. Pinned by plugin/test_kingtracker_logic.cpp.
//   · the KINGTRACK / KINGNOW row grammar
//   · (v0.6) history re-derived in the CURRENT scale: strike × (nowPx / nowStrike), so a mid-session roll of Skylit's
//     ES1 (2026-09-16: 7688 → 7757 for the same SPX 7685) does not leave a phantom step
//   · the anchor for the contract offset (SCALEREF first, SPOT as the fallback) and its clamp
//   · the stale age with the overnight wrap (shared shape with lsDayModel)
// The anchor BAR itself is ContractOffsetLogic.h (shared with lsGammaProfile).
#pragma once
#include <string>
#include <vector>
#include <cstdlib>

namespace ktl {

struct Step { double so; float px; int strike; };
struct Book { std::vector<Step> steps; bool hasNow; float nowPx; int nowStrike, nowPct; Book() : hasNow(false), nowPx(0), nowStrike(0), nowPct(0) {} };

// KINGTRACK,<fam>,<book>,<so>,<px>,<strike>  → a step;  KINGNOW,<fam>,<book>,<px>,<strike>[,<pct>]  → the current King
inline bool parseTrack(const std::vector<std::string>& t, std::string& fam, std::string& book, Step& st)
{
    if (t.size() < 6 || t[0] != "KINGTRACK") return false;
    fam = t[1]; book = t[2]; st.so = atof(t[3].c_str()); st.px = (float)atof(t[4].c_str()); st.strike = atoi(t[5].c_str()); return true;
}
inline bool parseNow(const std::vector<std::string>& t, std::string& fam, std::string& book, Book& b)
{
    if (t.size() < 5 || t[0] != "KINGNOW") return false;
    fam = t[1]; book = t[2]; b.nowPx = (float)atof(t[3].c_str()); b.nowStrike = atoi(t[4].c_str()); b.nowPct = (t.size() > 5) ? atoi(t[5].c_str()) : 0; b.hasNow = true; return true;
}

// every step's price re-derived as strike × (nowPx / nowStrike) — only when KINGNOW gives the ratio
inline void rederive(Book& b)
{
    if (!(b.hasNow && b.nowStrike > 0 && b.nowPx > 0.0f)) return;
    float r = b.nowPx / (float)b.nowStrike;
    for (size_t i = 0; i < b.steps.size(); i++) if (b.steps[i].strike > 0) b.steps[i].px = (float)b.steps[i].strike * r;
}

// the anchor the offset is measured against: SCALEREF when present, else SPOT; false when neither
inline bool anchorPrice(bool hasScaleRef, float scaleRef, bool hasSpot, float spotPx, float& anchor)
{
    if (hasScaleRef && scaleRef > 0.0f) { anchor = scaleRef; return true; }
    if (hasSpot) { anchor = spotPx; return true; }
    return false;
}
inline bool offsetFor(float chartClose, float anchor, float& off)
{
    off = chartClose - anchor;
    if (off < -300.0f || off > 300.0f) return false;   // implausible (incl. the ES/NQ cross)
    if (off > -0.01f && off < 0.01f) return false;      // already aligned
    return true;
}
inline void shift(Book& b, float off) { for (size_t i = 0; i < b.steps.size(); i++) b.steps[i].px += off; if (b.hasNow) b.nowPx += off; }

inline double staleAge(double asofSo, double localSo)
{
    if (asofSo < 0) return -1.0;
    return (asofSo > localSo + 300.0) ? ((86400.0 - asofSo) + localSo) / 60.0 : (localSo - asofSo) / 60.0;
}

} // namespace ktl
