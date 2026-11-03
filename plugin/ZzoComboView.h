// ZzoComboView.h - lsZzoCombo 2.0.1 (2026-11-03 09:20 CT: labels and tags drawn after every column - the next wave's columns covered
//   the "weak" tag on Rassul's ES chart at 08:33) 2.0.0 (2026-11-03, Rassul "build" after the ZZO Combo Mockup v19): the drawing of ZZO Combo, SDK-free
// (tests_zzodelta/test_zzocombo.cpp). One row in its own pane:
//   - a volume RAMP per wave: one column per bar, height = the wave's running volume / the median volume of the last 5 same-way waves
//     ("wave on top"), green up waves / red down waves; the forming wave darker with a white top edge (it repaints with every tick)
//   - waves of 2.0x+ volume: the ratio above the peak ("3.2x") and the COUNT right-justified against the wave's right edge in the top
//     quarter ("12" = the biggest wave of its colour in the last 12 of its colour; 0 hidden; "50+" cap)
//   - tags at the bottom centre of the wave, stacked up: Cmx / AR / ST, CoB / weak, and "Ab 4.9" (absorption with a heavy extreme and its
//     effort / result) in the colour of the side that wins the turn - only where the market's showAb is on (ES at build)
//   - tags only on completed waves (a wave completes when its bar closes and the next bar confirms the pivot)
//   - every tag box has a dark 1 px edge (readable on a ramp of its own colour)
//   - short pane: below 70 px no ratio labels, below 50 px no counts; tags always (when a tag fits)
#ifndef ZZO_COMBO_VIEW_H
#define ZZO_COMBO_VIEW_H
#include "ZzoDeltaLogic.h"

namespace zzd {

struct ComboScene { std::vector<Prim> p; int cols = 0, ratios = 0, counts = 0, ab = 0, cmx = 0, ar = 0, st = 0, cob = 0, weak = 0; double hi = 0; };

inline std::string countText(const Wave& w) { if (w.cntCap) return std::to_string(CNT_CAP) + "+"; return std::to_string(w.cnt); }

inline ComboScene layoutCombo(const Engine& E, const std::vector<Wave>& waves, long long formVol, const View& v, const std::string& head) {
    ComboScene S; const Colours C;
    const unsigned long upLive = 0x15803D, downLive = 0xB91C1C, white = 0xFFFFFF;
    const int TH = v.textH, H = v.b - v.t;
    const int top = v.t + TH + 4, bot = v.b - 2;
    const bool showRatio = H >= 70, showCount = H >= 50;
    const int nB = (int)E.B.size();
    const double half = std::max(0.5, v.bw * 0.5);
    auto volAt = [&](int i) { return i < nB ? (double)E.B[(size_t)i].vol : (double)formVol; };   // the forming bar = index nB
    // scale: the tallest visible running ratio, capped at 4x (so one opening wave does not flatten the day), at least 1x
    double hi = 1.0;
    for (const Wave& w : waves) {
        if (!w.hasV || w.e < v.first || w.s > v.last) continue;
        double run = 0; for (int i = w.s; i <= w.e; ++i) { run += volAt(i) / w.medV; if (i >= v.first && i <= v.last) hi = std::max(hi, run); }
    }
    hi = std::min(hi, 4.0); S.hi = hi;
    const int span = std::max(1, bot - top - TH - 2);   // leave a text row above the tallest ramp for the ratio label
    auto Y = [&](double r) { return bot - (int)std::lround(std::min(r, hi) / hi * span); };
    { Prim p; p.k = Prim::FILL; p.x0 = v.l; p.x1 = v.r; p.y0 = p.y1 = bot; p.col = C.zero; S.p.push_back(p); }
    std::vector<Prim> over;   // (2.0.1) every label and tag, drawn after ALL columns so a neighbouring wave never covers them
    std::vector<std::vector<std::pair<int, int>>> levels;
    for (const Wave& w : waves) {
        if (!w.hasV || w.e < v.first || w.s > v.last) continue;
        const bool up = w.dir > 0; const unsigned long col = w.live ? (up ? upLive : downLive) : (up ? C.up : C.down);
        double run = 0; int yPeak = bot;
        for (int i = w.s; i <= w.e; ++i) {
            run += volAt(i) / w.medV;
            if (i < v.first || i > v.last) continue;
            const double xc = v.X(i); const int y = Y(run);
            Prim p; p.k = Prim::FILL; p.x0 = ci(xc - half); p.x1 = std::max(p.x0, ci(xc + half) - 1); p.y0 = std::min(y, bot - 1); p.y1 = bot - 1; p.col = col;
            if (p.x1 >= v.l && p.x0 <= v.r) { S.p.push_back(p); ++S.cols;
                if (w.live) { Prim e = p; e.y1 = e.y0; e.col = white; S.p.push_back(e); } }
            yPeak = std::min(yPeak, y);
        }
        const double Vr = (double)w.vol / w.medV;
        const double xr = v.X(w.e) + half;          // the wave's right edge
        const double xm = (v.X(std::max(w.s, v.first)) + v.X(std::min(w.e, v.last))) / 2.0;   // its visible centre
        if (Vr >= CNT_MIN_VOL && w.e >= v.first && w.e <= v.last) {
            if (showRatio) {   // ratio above the peak, centred on the last column
                char b[24]; std::snprintf(b, sizeof b, "%.1fx", Vr); const std::string t = b; const int tw = v.textW(t);
                int x0 = ci(v.X(w.e) - tw / 2.0); x0 = std::max(v.l + 1, std::min(x0, v.r - 1 - tw));
                Prim p; p.k = Prim::TEXT; p.x0 = x0; p.x1 = x0 + tw; p.y1 = std::max(yPeak - 1, top + TH); p.y0 = p.y1 - TH; p.col = up ? C.up : C.down; p.text = t;
                over.push_back(p); ++S.ratios;
            }
            if (showCount && w.cnt > 0 && bot - yPeak >= TH + 4) {   // count right-justified, top quarter of the ramp
                const std::string t = countText(w); const int tw = v.textW(t);
                const int x1 = ci(xr) - 1; const int y0 = yPeak + (bot - yPeak) / 4 - TH / 2;
                Prim p; p.k = Prim::TEXT; p.x0 = std::max(v.l + 1, x1 - tw); p.x1 = x1; p.y0 = std::max(y0, yPeak + 1); p.y1 = p.y0 + TH; p.col = white; p.text = t;
                if (p.y1 <= bot - 1) { over.push_back(p); ++S.counts; }
            }
        }
        if (w.live) continue;   // tags only once the wave is complete
        std::vector<std::pair<std::string, unsigned long>> tags;
        if (w.cmx) tags.push_back({ "Cmx", up ? C.up : C.down });
        if (w.cob) tags.push_back({ "AR", up ? C.up : C.down });
        if (w.st) tags.push_back({ "ST", up ? C.down : C.up });          // the side expected to take over
        if (w.cobTag == 1) tags.push_back({ "CoB", up ? C.up : C.down });
        if (w.cobTag == 2) tags.push_back({ "weak", C.grey });
        double er = 0;
        if (E.showAb && abHeavy(w) && effortResult(w, er)) { char b[24]; std::snprintf(b, sizeof b, "Ab %.1f", er); tags.push_back({ b, up ? C.down : C.up }); }   // winner's colour
        int lvl = 0;
        for (const auto& tg : tags) {
            const int aw = v.textW(tg.first) + 6; int ax = ci(xm - aw / 2.0); ax = std::max(v.l + 1, std::min(ax, v.r - 1 - aw));
            // the lowest free level for this box (never on top of a neighbour's tag)
            for (;; ++lvl) {
                if ((int)levels.size() <= lvl) levels.resize((size_t)lvl + 1);
                bool clash = false; for (const auto& r : levels[(size_t)lvl]) if (!(ax + aw + 1 < r.first || ax > r.second + 1)) { clash = true; break; }
                if (!clash) break;
            }
            Prim bx; bx.k = Prim::FILL; bx.x0 = ax; bx.x1 = ax + aw; bx.y1 = bot - 2 - lvl * (TH + 2); bx.y0 = bx.y1 - TH; bx.col = tg.second;
            if (bx.y0 < top) break;   // no room left in a short pane
            levels[(size_t)lvl].push_back({ ax, ax + aw });
            { Prim o = bx; o.x0 -= 1; o.x1 += 1; o.y0 -= 1; o.y1 += 1; o.col = 0x0B1220; over.push_back(o); }   // dark edge: readable on a ramp of the same colour
            over.push_back(bx); Prim tx; tx.k = Prim::TEXT; tx.x0 = ax + 3; tx.x1 = ax + aw - 3; tx.y0 = bx.y0; tx.y1 = bx.y1; tx.col = white; tx.text = tg.first; over.push_back(tx);
            const std::string& n = tg.first;
            if (n.rfind("Ab", 0) == 0) ++S.ab; else if (n == "Cmx") ++S.cmx; else if (n == "AR") ++S.ar; else if (n == "ST") ++S.st; else if (n == "CoB") ++S.cob; else ++S.weak;
            ++lvl;
        }
    }
    S.p.insert(S.p.end(), over.begin(), over.end());
    { Prim p; p.k = Prim::TEXT; p.x0 = v.l + 4; p.y0 = v.t + 2; p.x1 = p.x0 + v.textW(head); p.y1 = p.y0 + TH; p.col = C.grey; p.text = head; S.p.push_back(p); }
    return S;
}

inline std::string comboStatus(const Engine& E, const std::string& mkt, int spb, const std::string& vap) {
    std::string s = std::string("lsZzoCombo ") + COMBO_VERSION + " (engine " + VERSION + ")\nMARKET," + mkt + "," + std::to_string(spb) +
        "\nSHOW_AB," + (E.showAb ? "on" : "off - logged only until the nightly proves it here") +
        "\nRULES,volume ramp per wave vs the median of the last 5 same-way waves; ratio + count on 2.0x+ waves (count = biggest of its colour in the last N of its colour, cap 50); Ab = absorption rule + pivot extreme 2 ticks >= 4% of the bar, shown with effort / result\n" +
        "BARS," + std::to_string(E.B.size()) + ",no VAP " + std::to_string(E.noVap) + ",VAP " + vap + "\n";
    int n = 0;
    for (size_t k = E.W.size(); k-- > 0 && n < 30; ++n) {
        const Wave& w = E.W[k]; if (w.s < 0) break;
        double er = 0; const bool hasEr = effortResult(w, er);
        char b[320]; std::snprintf(b, sizeof b, "WAVE,%lld,%s,vol %s,count %s,E/R %s,extreme %s%s%s%s%s%s%s\n", E.B[(size_t)w.e].t, w.dir > 0 ? "UP" : "DN", volText(w).c_str(),
            countText(w).c_str(), hasEr ? (std::to_string((int)std::lround(er * 10) / 10) + "." + std::to_string((int)std::lround(er * 10) % 10)).c_str() : "-",
            w.xsh < 0 ? "-" : (std::to_string((int)std::lround(w.xsh * 1000) / 10) + "." + std::to_string((int)std::lround(w.xsh * 1000) % 10) + "%").c_str(),
            w.ab ? (abHeavy(w) ? ",Ab" : ",Ab? (logged)") : "", w.cmx ? ",Cmx" : "", w.cob ? ",AR" : "", w.st ? ",ST" : "", w.cobTag == 1 ? ",CoB" : "", w.cobTag == 2 ? ",weak" : "");
        s += b;
    }
    return s;
}
}  // namespace zzd
#endif
