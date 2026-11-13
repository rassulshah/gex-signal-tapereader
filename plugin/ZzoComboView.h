// ZzoComboView.h  (2.3.0, 2026-11-12: + the CoB finder's Cob / Lvp tags from ZzoCobFinder.h; the old CoB-after-sweep tag is logged only)
// ZzoComboView.h - lsZzoCombo 2.1.1 (2026-11-03, Rassul "build it all"): the wave COUNT is drawn INSIDE the wave's bars - centred on the
//   wave's second-to-last bar, just below that bar's top, white with a 1 px black outline (reads on green or red); a bar too short for the
//   text gets it just above its top (moved above the ratio label if they would touch). The ratio label is unchanged.
// was 2.1.0 (2026-11-03 14:00 CT, Rassul 13:59-14:03): waves from the 3-bar High/Low ZigZag (ZzoDeltaLogic.h
//   COMBO_REV_BARS); the count compares the wave's volume ratio (the ramp height) with PRIOR waves of its own colour only; "weak" is no
//   longer drawn ("remove weak"); the count is drawn one bar further LEFT ("move the count 1 more to the left": its right edge = the
//   wave's last bar's right edge - one bar width). The ratio label stays centred on the wave's last bar.
// was 2.0.1 (2026-11-03 09:20 CT: labels and tags drawn after every column - the next wave's columns covered
//   the "weak" tag on Rassul's ES chart at 08:33) 2.0.0 (2026-11-03, Rassul "build" after the ZZO Combo Mockup v19): the drawing of ZZO Combo, SDK-free
// (tests_zzodelta/test_zzocombo.cpp). One row in its own pane:
//   - a volume RAMP per wave: one column per bar, height = the wave's running volume / the median volume of the last 5 same-way waves
//     ("wave on top"), green up waves / red down waves; the forming wave darker with a white top edge (it repaints with every tick)
//   - waves of 2.0x+ volume: the ratio above the peak ("3.2x") and the COUNT (2.1.1: inside the second-to-last bar, below its top; was
//     right-justified against the wave's right edge in the top quarter) ("12" = the biggest wave of its colour in the last 12 of its colour; 0 hidden; "50+" cap)
//   - tags at the bottom centre of the wave, stacked up: Cmx / AR / ST, CoB (2.1.0: weak not drawn), and "Ab 4.9" (absorption with a heavy extreme and its
//     effort / result) in the colour of the side that wins the turn - only where the market's showAb is on (ES at build)
//   - tags only on completed waves (a wave completes when its bar closes and the next bar confirms the pivot)
//   - every tag box has a dark 1 px edge (readable on a ramp of its own colour)
//   - short pane: below 70 px no ratio labels, below 50 px no counts; tags always (when a tag fits)
#ifndef ZZO_COMBO_VIEW_H
#define ZZO_COMBO_VIEW_H
#include "ZzoDeltaLogic.h"
#include "ZzoCobFinder.h"   // (2.3.0) the CoB finder: Cob / Lvp tags

namespace zzd {

static const int COUNT_PAD = 2;                  // (2.1.1) px between the bar's top and the count text inside it
static const unsigned long OUTLINE = 0x000000;  // (2.1.1) the count's 1 px outline
static const bool SHOW_WAVE_TAGS = false;   // (2.9.4) Cmx / AR / ST / Ab tags of a wave: logged, not drawn
struct ComboScene { std::vector<Prim> p; int cols = 0, ratios = 0, counts = 0, countsIn = 0, countsAbove = 0, countsDropped = 0, ab = 0, cmx = 0, ar = 0, st = 0, cob = 0, weak = 0, lvp = 0, cob2 = 0, cb = 0, sr = 0, boxRows = 0, boxL = 0, boxT = 0, boxR = 0, boxB = 0; double hi = 0; std::vector<std::pair<std::string, std::string>> box; };

inline std::string countText(const Wave& w) { if (w.cntCap) return std::to_string(CNT_CAP) + "+"; return std::to_string(w.cnt); }

// (2.4.0, Rassul 2026-11-12 11:27 "I want a read box on the left of the indicator pane", 11:35 "try to reuse rows like lvp/hrly so it
// doesn't take up too much vertical space", 11:38 "in the Lvl you can add SR+ in the beginning") the read box: the newest Cob / Cob2 of
// the session. Rows by importance: Cob | Lvl | Q "4/6" (2.5.1, Rassul 12:26 "get rid of count because i already see it on the wave, and keep Q and label it like 4/6") | Lvp / Hr | Hold. Fixed width; rows that do not fit a short pane drop from
// the bottom; under title + 2 rows no box (the plain title as before).
inline std::string clock12(long long t) {
    const long long d = floorDiv(t, 86400); const int mn = (int)((t - d * 86400) / 60); int h = mn / 60; const int m = mn % 60; const bool pm = h >= 12; h = h % 12; if (h == 0) h = 12;
    char b[24]; std::snprintf(b, sizeof b, "%d:%02d %s", h, m, pm ? "PM" : "AM"); return b;
}
inline std::string lvShort(int mask, int dir, int maxN = 3) {
    std::string t; int n = 0; for (int i = 0; i < zcf::NLV && n < maxN; ++i) if (mask & (1 << i)) { t += " "; t += dir > 0 ? zcf::LV_LO[i] : zcf::LV_HI[i]; ++n; }
    return n ? "SR+" + t : "-";
}
inline std::vector<std::pair<std::string, std::string>> readRows(const zcf::Read& R) {
    std::vector<std::pair<std::string, std::string>> r;
    if (!R.any) { r.push_back({ "Cob", "-" }); r.push_back({ "Lvl", "-" }); r.push_back({ "Q / Rng", "-" }); r.push_back({ "Lvp / Hr", "-" }); r.push_back({ "Hold", "-" }); r.push_back({ "Warn", "-" }); return r; }
    char b[48];
    r.push_back({ R.kind == 2 ? "Cob2" : "Cob", std::string(R.dir > 0 ? "Up " : "Dn ") + clock12(R.tCf) });
    r.push_back({ "Lvl", lvShort(R.lv, R.dir) });
    if (R.q < 0) std::snprintf(b, sizeof b, "-"); else std::snprintf(b, sizeof b, "%d%%", (100 * R.q + zcf::Q_MAX / 2) / zcf::Q_MAX);   // (2.8.1) quality as a percentage of its 6 points
    { char g[24]; if (R.rng < 0) std::snprintf(g, sizeof g, "-"); else std::snprintf(g, sizeof g, "%d.%d", R.rng / 100, (R.rng % 100) / 10);   // (2.8.0) day range used, in ADRs
      r.push_back({ "Q / Rng", std::string(b) + " / " + g }); }
    r.push_back({ "Lvp / Hr", std::string(R.lvpSt == 1 ? "yes" : R.lvpSt == 2 ? "no" : "wait") + " / " + (R.hr ? "yes" : "no") });
    std::snprintf(b, sizeof b, "%d%%", R.hold); r.push_back({ "Hold", b });
    std::snprintf(b, sizeof b, "Warn (%d)", zcf::warnCount(R.warn)); r.push_back({ b, zcf::warnText(R.warn) });   // (2.9.0) the warnings it carries; red at WARN_RED
    return r;
}

inline ComboScene layoutCombo(const Engine& E, const std::vector<Wave>& waves, long long formVol, const View& v, const std::string& head, const zcf::Finder* cf = nullptr) {
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
    // (2.7.0) the stop-run bars of the visible Cobs / Cob2s: bar time -> the Cob's direction
    std::map<long long, int> srAt;
    if (cf) for (const Wave& w : waves) { if (w.live || w.e < 0 || (size_t)w.e >= E.B.size()) continue; if (const zcf::Tag* tg = cf->find(E.B[(size_t)w.e].t, w.dir)) if (tg->tSr && (tg->cob || (tg->cob2 && zcf::SHOW_COB2))) srAt[tg->tSr] = tg->cob ? tg->cob : tg->cob2; }
    std::vector<Prim> over;   // (2.0.1) every label and tag, drawn after ALL columns so a neighbouring wave never covers them
    std::vector<std::vector<std::pair<int, int>>> levels;
    for (const Wave& w : waves) {
        if (!w.hasV || w.e < v.first || w.s > v.last) continue;
        const bool up = w.dir > 0; const unsigned long col = w.live ? (up ? upLive : downLive) : (up ? C.up : C.down);
        double run = 0; int yPeak = bot;
        // (2.6.0) the Cob bar of this wave (zcf::Finder, decided with the Cob / Cob2): "CB" in a box of the wave's colour over its column;
        long long tCb = 0; if (cf && !w.live && w.e >= 0 && (size_t)w.e < E.B.size()) { if (const zcf::Tag* tg = cf->find(E.B[(size_t)w.e].t, w.dir)) tCb = tg->tCb; }
        for (int i = w.s; i <= w.e; ++i) {
            run += volAt(i) / w.medV;
            if (i < v.first || i > v.last) continue;
            const double xc = v.X(i); const int y = Y(run);
            if (i >= 0 && i < nB) {
                const long long tb = E.B[(size_t)i].t;
                // (2.7.2, Rassul 2026-11-12 13:02 "Cb and sr should only be on the price chart") counted for the status file, NOT drawn in the
                // pane: lsZzoComboMarks draws them on the candles
                if (tCb && tb == tCb) ++S.cb;
                if (srAt.count(tb)) ++S.sr;
            }
            Prim p; p.k = Prim::FILL; p.x0 = ci(xc - half); p.x1 = std::max(p.x0, ci(xc + half) - 1); p.y0 = std::min(y, bot - 1); p.y1 = bot - 1; p.col = col;
            if (p.x1 >= v.l && p.x0 <= v.r) { S.p.push_back(p); ++S.cols;
                if (w.live) { Prim e = p; e.y1 = e.y0; e.col = white; S.p.push_back(e); } }
            yPeak = std::min(yPeak, y);
        }
        const double Vr = (double)w.vol / w.medV;
        const double xm = (v.X(std::max(w.s, v.first)) + v.X(std::min(w.e, v.last))) / 2.0;   // its visible centre
        if (Vr >= CNT_MIN_VOL && w.e >= v.first && w.e <= v.last) {
            Prim rp; bool hasRp = false;   // the ratio label's box (the count never sits on it)
            if (showRatio) {   // ratio above the peak, centred on the last column
                char b[24]; std::snprintf(b, sizeof b, "%.1fx", Vr); const std::string t = b; const int tw = v.textW(t);
                int x0 = ci(v.X(w.e) - tw / 2.0); x0 = std::max(v.l + 1, std::min(x0, v.r - 1 - tw));
                Prim p; p.k = Prim::TEXT; p.x0 = x0; p.x1 = x0 + tw; p.y1 = std::max(yPeak - 1, top + TH); p.y0 = p.y1 - TH; p.col = up ? C.up : C.down; p.text = t;
                over.push_back(p); ++S.ratios; rp = p; hasRp = true;
            }
            if (showCount && w.cnt > 0) {
                // (2.1.1, Rassul 2026-11-03) the count INSIDE the wave: centred on the wave's second-to-last bar, just below that bar's top,
                // white with a 1 px black outline; a bar too short for the text gets it just above its top (never on the ratio label)
                const int k = std::max(w.s, w.e - 1);
                double runK = 0; for (int i = w.s; i <= k; ++i) runK += volAt(i) / w.medV;
                const int yK = Y(runK);
                const std::string t = countText(w); const int tw = v.textW(t);
                int x0 = ci(v.X(k) - tw / 2.0); x0 = std::max(v.l + 1, std::min(x0, v.r - 1 - tw));
                Prim p; p.k = Prim::TEXT; p.x0 = x0; p.x1 = x0 + tw; p.col = white; p.text = t;
                bool ok = true, inside = true;
                p.y0 = yK + COUNT_PAD; p.y1 = p.y0 + TH;
                if (p.y1 > bot - 1) {   // too short: just above the bar's top
                    inside = false; p.y1 = yK - 1; p.y0 = p.y1 - TH;
                    if (hasRp && p.x0 <= rp.x1 + 1 && rp.x0 <= p.x1 + 1 && p.y0 <= rp.y1 && rp.y0 <= p.y1) { p.y1 = rp.y0 - 1; p.y0 = p.y1 - TH; }   // above the ratio label
                    if (p.y0 < top) ok = false;
                }
                if (ok) {
                    static const int OX[8] = { -1, 0, 1, -1, 1, -1, 0, 1 }, OY[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
                    for (int o = 0; o < 8; ++o) { Prim q = p; q.x0 += OX[o]; q.x1 += OX[o]; q.y0 += OY[o]; q.y1 += OY[o]; q.col = OUTLINE; over.push_back(q); }   // 1 px black outline
                    over.push_back(p); ++S.counts; if (inside) ++S.countsIn; else ++S.countsAbove;
                } else ++S.countsDropped;
            }
        }
        if (w.live) continue;   // tags only once the wave is complete
        std::vector<std::pair<std::string, unsigned long>> tags;
        // (2.9.4, Rassul 2026-11-12 20:37 "the cob reader has AB and other signals, it should only have cobs and lvp") Cmx / AR / ST / Ab
        // are still decided and written to the status file, but no longer drawn: the pane's tags are Cob and Lvp only.
        if (SHOW_WAVE_TAGS && w.cmx) tags.push_back({ "Cmx", up ? C.up : C.down });
        if (SHOW_WAVE_TAGS && w.cob) tags.push_back({ "AR", up ? C.up : C.down });
        if (SHOW_WAVE_TAGS && w.st) tags.push_back({ "ST", up ? C.down : C.up });          // the side expected to take over
        // (2.3.0, Rassul 2026-11-12 CoB finder) the old "CoB after a sweep" tag (cobTag 1) is no longer drawn (status file only); "Cob" and
        // "Lvp" come from zcf::Finder, decided once when the wave completed. Lvp takes its Cob's colour.
        if (cf && w.e >= 0 && (size_t)w.e < E.B.size()) { if (const zcf::Tag* tg = cf->find(E.B[(size_t)w.e].t, w.dir)) {
            if (tg->cob) tags.push_back({ "Cob", tg->cob > 0 ? C.up : C.down });
            if (tg->cob2 && zcf::SHOW_COB2) tags.push_back({ "Cob2", tg->cob2 > 0 ? C.up : C.down });
            if (tg->lvp) { const zcf::Tag* par = cf->find(tg->tCob, tg->lvp); if (zcf::SHOW_COB2 || (par && par->cob)) tags.push_back({ "Lvp", tg->lvp > 0 ? C.up : C.down }); } } }   // (2.9.3) the Lvp of a Cob2 is not drawn
        // (2.1.0, Rassul 14:02 CT "remove weak") cobTag 2 = weak stays in the status file only, never drawn
        double er = 0;
        if (SHOW_WAVE_TAGS && E.showAb && abHeavy(w) && effortResult(w, er)) { char b[24]; std::snprintf(b, sizeof b, "Ab %.1f", er); tags.push_back({ b, up ? C.down : C.up }); }   // winner's colour
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
            if (n.rfind("Ab", 0) == 0) ++S.ab; else if (n == "Cmx") ++S.cmx; else if (n == "AR") ++S.ar; else if (n == "ST") ++S.st; else if (n == "Cob") ++S.cob; else if (n == "Cob2") ++S.cob2; else if (n == "Lvp") ++S.lvp; else ++S.weak;
            ++lvl;
        }
    }
    S.p.insert(S.p.end(), over.begin(), over.end());
    bool boxed = false;
    if (cf && cf->ok) {
        const zcf::Read R = cf->read(); const std::vector<std::pair<std::string, std::string>> rows = readRows(R);
        const int rh = TH + 2, kw = v.textW("Lvp / Hr"), vw = std::max(v.textW("SR+ AsiaHI LonHI DPoc"), v.textW("Rng 1st Deep Q Hr")), bw = std::max(kw + vw + 22, v.textW(head) + 12);
        const int x0 = v.l + 2, y0 = v.t + 1, fit = std::min((int)rows.size(), (H - 4 - (TH + 4)) / rh);
        if (fit >= 2 && x0 + bw < v.r) {
            boxed = true; const int y1 = y0 + TH + 4 + fit * rh + 2;
            Prim e; e.k = Prim::FILL; e.x0 = x0; e.x1 = x0 + bw; e.y0 = y0; e.y1 = y1; e.col = 0x6B7280; S.p.push_back(e);
            Prim f = e; f.x0 += 1; f.x1 -= 1; f.y0 += 1; f.y1 -= 1; f.col = 0x0B0F17; S.p.push_back(f);
            const int hw = v.textW(head);
            { Prim p; p.k = Prim::TEXT; p.x0 = x0 + (bw - hw) / 2; p.x1 = p.x0 + hw; p.y0 = y0 + 2; p.y1 = p.y0 + TH; p.col = white; p.text = head; S.p.push_back(p); }
            for (int i = 0; i < fit; ++i) {
                const int y = y0 + TH + 4 + i * rh; const int w2 = v.textW(rows[(size_t)i].second);
                Prim k; k.k = Prim::TEXT; k.x0 = x0 + 6; k.x1 = k.x0 + v.textW(rows[(size_t)i].first); k.y0 = y; k.y1 = y + TH; k.col = C.grey; k.text = rows[(size_t)i].first; S.p.push_back(k);
                Prim q; q.k = Prim::TEXT; q.x1 = x0 + bw - 6; q.x0 = q.x1 - w2; q.y0 = y; q.y1 = y + TH; q.text = rows[(size_t)i].second;
                q.col = (i == 0 && R.any) ? (R.dir > 0 ? C.up : C.down) : (R.any && rows[(size_t)i].first.rfind("Warn", 0) == 0 && zcf::warnCount(R.warn) >= zcf::WARN_RED) ? C.down : white; S.p.push_back(q);
                S.box.push_back(rows[(size_t)i]);
            }
            S.boxRows = fit; S.boxL = x0; S.boxT = y0; S.boxR = x0 + bw; S.boxB = y1;
        }
    }
    if (!boxed) { Prim p; p.k = Prim::TEXT; p.x0 = v.l + 4; p.y0 = v.t + 2; p.x1 = p.x0 + v.textW(head); p.y1 = p.y0 + TH; p.col = C.grey; p.text = head; S.p.push_back(p); }
    return S;
}

inline std::string comboStatus(const Engine& E, const std::string& mkt, int spb, const std::string& vap) {
    std::string s = std::string("lsZzoCombo ") + zcf::COMBO_VERSION + " (engine " + VERSION + ")\nMARKET," + mkt + "," + std::to_string(spb) +
        "\nZIGZAG," + std::to_string(E.revBars) + "-bar reversal, High/Low (IRT ZIGZAG Min Price Change = " + std::to_string(E.revBars) + " bars)" +
        "\nSHOW_AB," + (E.showAb ? "on" : "off - logged only until the nightly proves it here") +
        "\nRULES,volume ramp per wave vs the median of the last 5 same-way waves; ratio + count on 2.0x+ waves (count = its volume ratio beats the last N waves of its colour in a row, cap 50; weak logged, not drawn); Ab = absorption rule + pivot extreme 2 ticks >= 4% of the bar, shown with effort / result\n" +
        "BARS," + std::to_string(E.B.size()) + ",no VAP " + std::to_string(E.noVap) + ",VAP " + vap + "\n";
    int n = 0;
    for (size_t k = E.W.size(); k-- > 0 && n < 30; ++n) {
        const Wave& w = E.W[k]; if (w.s < 0) break;
        double er = 0; const bool hasEr = effortResult(w, er);
        char b[320]; std::snprintf(b, sizeof b, "WAVE,%lld,%s,vol %s,count %s,E/R %s,extreme %s%s%s%s%s%s%s\n", E.B[(size_t)w.e].t, w.dir > 0 ? "UP" : "DN", volText(w).c_str(),
            countText(w).c_str(), hasEr ? (std::to_string((int)std::lround(er * 10) / 10) + "." + std::to_string((int)std::lround(er * 10) % 10)).c_str() : "-",
            w.xsh < 0 ? "-" : (std::to_string((int)std::lround(w.xsh * 1000) / 10) + "." + std::to_string((int)std::lround(w.xsh * 1000) % 10) + "%").c_str(),
            w.ab ? (abHeavy(w) ? ",Ab" : ",Ab? (logged)") : "", w.cmx ? ",Cmx" : "", w.cob ? ",AR" : "", w.st ? ",ST" : "", w.cobTag == 1 ? ",CoB after sweep (logged)" : "", w.cobTag == 2 ? ",weak" : "");
        s += b;
    }
    return s;
}
}  // namespace zzd
#endif
