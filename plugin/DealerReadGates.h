// DealerReadGates.h -- lsDealerRead 4.6.0 (2026-10-14): the peer-review claim gates applied to a parsed Dealer file (SDK-free, tested on
// Linux by test_dealerread_460.cpp). Rassul approved the hide list 15:07 CT 10/14; ClaimGates.h holds the switches (0 = hidden).
#pragma once
#include "DealerLogic.h"
#include "ClaimGates.h"
#include <functional>
#include <string>
#include <vector>
namespace drg {
// (4.7.0, 2026-10-14 18:25 CT audit display list) the header's leading control word + tally ("DEMAND 3 of 4", "SUPPLY? 2 of 3",
// "BALANCED (1 vs 1)") is dropped; the facts after it stay ("LONG at PDL 6,012 - low 9:12 AM ..."). Segments are " - " or " \xB7 ".
inline bool isControlSeg(const std::string& g0)
{
    size_t a = g0.find_first_not_of(' '); if (a == std::string::npos) return false;
    const std::string g = g0.substr(a);
    return g.compare(0, 6, "DEMAND") == 0 || g.compare(0, 6, "SUPPLY") == 0 || g.compare(0, 8, "BALANCED") == 0;
}
inline std::string stripControl(const std::string& h)
{
    std::string o = h;
    for (int guard = 0; guard < 4; ++guard) {
        size_t a = o.find_first_not_of(' '); if (a == std::string::npos) return std::string();
        if (!isControlSeg(o)) break;
        size_t b1 = o.find(" - ", a), b2 = o.find(" \xB7 ", a);
        size_t b = b1 == std::string::npos ? b2 : b2 == std::string::npos ? b1 : (b1 < b2 ? b1 : b2);
        if (b == std::string::npos) return std::string();
        o = o.substr(b + 3);
    }
    return o;
}
// (4.7.0) the FUTURES column: every chip gets its "?" ("Dem?") and the point ends "context" (a footprint fact, not the cause)
static const char* const CONTEXT_TAIL = " - context";
inline std::string contextText(const std::string& t)
{
    if (t.empty()) return t;
    const std::string tail = CONTEXT_TAIL;
    if (t.size() >= tail.size() && t.compare(t.size() - tail.size(), tail.size(), tail) == 0) return t;
    return t + tail;
}
// a point cut to 2 lines keeps its " - context" tail: the last line is trimmed word by word until "... - context" fits in restW
inline void keepTail(std::vector<std::string>& ln, const std::string& full, int restW, const std::function<int(const std::string&)>& W)
{
    const std::string tail = CONTEXT_TAIL;
    if (ln.empty() || full.size() < tail.size() || full.compare(full.size() - tail.size(), tail.size(), tail) != 0) return;
    std::string& L = ln.back();
    if (L.size() >= tail.size() && L.compare(L.size() - tail.size(), tail.size(), tail) == 0) return;
    while (L.size() >= 3 && L.compare(L.size() - 3, 3, "...") == 0) L.erase(L.size() - 3);
    const std::string end = "..." + tail;                     // "... - context"
    while (!L.empty() && W(L + end) > restW) { size_t sp = L.find_last_of(' '); if (sp == std::string::npos) { L.clear(); break; } L.erase(sp); }
    L += end;
}
// (4.6.0, 2026-10-14, Rassul 15:07 CT approval of the peer-review hide list - ClaimGates.h) applied to every parsed Dealer file, so an
// older file (written before the bridge's own gates) shows nothing hidden either: no VERDICT (the Strength STRONG / OK / WEAK / BROKEN
// and its floor / pocket text), no forced-futures FSUM / FWIN numbers, no OPTIONS-column reasons (rows with no section = options), and
// no footprint "Absorbed" point as a cause. Restore = /D LRA_SHOW_<GATE>=1 and rebuild.
inline void apply(dl::Data& D)
{
    if (!cgate::kShowStrength) { D.vLabel.clear(); D.vText.clear(); D.vPts = -1; D.vOf = 0; }
    if (!cgate::kShowOptionsCauses) { D.hasFsum = false; D.fwin.clear(); }
    std::vector<dl::Data::TRow> keep;
    for (size_t i = 0; i < D.trows.size(); i++) {
        const dl::Data::TRow& r = D.trows[i];
        if (r.sec != 'F' && !cgate::kShowOptionsCauses) continue;
        if (r.sec == 'F' && !cgate::kShowAbsorptionReason && cgate::isAbsorptionTag(r.tag)) continue;
        keep.push_back(r);
    }
    D.trows.swap(keep);
    // (4.7.0, 2026-10-14 18:25 CT audit display list)
    if (!cgate::kShowControlWord) D.turnHead = stripControl(D.turnHead);
    if (!cgate::kShowAnalyst) { D.aDeal.clear(); D.aMag.clear(); D.aNow.clear(); D.aTime.clear(); D.aBias.clear(); D.aBiasCol = 'N'; D.scn.clear(); }
    for (size_t i = 0; i < D.trows.size(); i++) if (D.trows[i].sec == 'F') D.trows[i].tag = cgate::markQ(D.trows[i].tag);   // the " - context" tail goes on each drawn point (renderTurn)
    // the no-level REASONS row (Dealer Sig codes): options codes off with the options gate, absorption off with its gate, footprint codes "?"
    std::vector<dl::Data::Sig> sk;
    for (size_t i = 0; i < D.sigs.size(); i++) {
        dl::Data::Sig g = D.sigs[i];
        if (!cgate::kShowOptionsCauses && cgate::isOptionsCode(g.code)) continue;
        if (!cgate::kShowAbsorptionReason && cgate::isAbsorptionTag(g.code)) continue;
        if (!cgate::isOptionsCode(g.code)) g.code = cgate::markQ(g.code);
        sk.push_back(g);
    }
    D.sigs.swap(sk);
    if (!cgate::kShowOptionsCauses) D.lsigs.clear();
}
}   // namespace drg
