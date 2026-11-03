// CobSignalLogic.h (1.0.0, 2026-11-01) - SDK-free logic of lsCobSignal (CobSignal.cpp): Rassul's CoB computed natively in IRT, so his
// own RTL CoB indicators (cob_bull / cob_bear) can come off the chart. Rassul 22:12-22:24 CT: "i want you to build the cob signals
// instead of me having my own", "consider the new levels also PHH and PHL", labels "Cob , Cob+R , Cob+V" (mockup cob_ba_mockup v2 "build").
//
// His locked rules (memory: trading.md, cob_bull / cob_bear RTL 2026-09-30 + 10-17 + 10-24), on CLOSED bars only:
//   ZigZag  his 1-bar ZigZag (the same walk as CobImpulseLogic::legPivot / trade_def.cob_live): up while highs extend, turns down on
//           a lower low (and the mirror). A leg = pivot to pivot.
//   bull    on bar j the up leg s -> e just ended with e = j - 1 (Ups#1 = 1), j an RTH bar ending at or before 11:00 CT, and
//           - the low of day (RTH, through j) SWEPT a level (ONL / PDL / PFL / LonLO / PHL): the LOD is below the level and the
//             up-leg high H[e] is back above it
//           - dratio = the leg / the average of the previous 20 up legs >= 1.9 (at least 5 earlier legs)
//           - the LOD printed after the previous up pivot (LOD < Ups#2) and is the 50-bar low (REF(LO,LOD) = MIN(LO,50))
//           - the leg has >= 3 bars (2026-10-24)
//           - room: close[j] - the leg's start low >= 0.1 ADR;  H[e] - LOD < 0.4 ADR
//   bear    the mirror with ONH / PDH / PFH / LonHI / PHH, the HOD, the down leg's low below the previous down pivot's low, no room
//           line and no time cutoff (his cob_bear).
//   ADR     20-day full-session true range (trade_def, min 10 sessions) - as CobImpulseLogic::adrAt.
//   levels  KeyLevelsLogic::compute (the Dealer Profile's own levels) as of each session's first RTH bar: PFH / PFL, PDH / PDL,
//           ONH / ONL, LonHI / LonLO and PHH / PHL (2026-11-01).
//   tag     CobImpulseLogic::evaluate (his approved +R / +V rule, 2026-10-14): a bar from the leg's pivot to 2 bars after it, inside
//           the pivot bar, with the biggest RANGE (R) / VOLUME (V) in >= 35 bars. Label Cob / Cob+R / Cob+V / Cob+RV.
// Decided once at the CoB bar's close; every decision is a record line; nothing after the bar is read (no repaint).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "KeyLevelsLogic.h"
#include "CobImpulseLogic.h"

namespace cobs {

static const char* const VERSION = "1.2.0";   // (1.2.0, Rassul 2026-11-02 19:07-19:19 CT, his cob_bull_3m_1b / cob_bear_3m_1b + "11:00 am cutoff" + "build it so i can add it") = his two signals: + PWL / AsiaLO (bull) and PWH / AsiaHI (bear) as swept levels, PHL / PHH kept as extras; BOTH sides by 11:00 CT; the bear gets his room line (the leg-start high - close >= 0.1 ADR)   // was "1.1.0":   // (1.1.0, Rassul 2026-11-02 10:14 CT "ok.. lets add it .. build") a CoB whose ONLY swept level is PHH / PHL
// is drawn in ES / HG / GC (the year test: +0.61 / +0.18 / +0.16 R) and recorded but NOT drawn in NQ / CL / NG / 6E (+0.01 / -0.22 / -0.53 / -0.15 R) until the
// nightly proves it there; a CoB that also swept another level is drawn everywhere as before   // was "1.0.0"
inline bool phShown(const std::string& mkt) { return mkt == "ES" || mkt == "HG" || mkt == "GC"; }
#ifndef COBS_MAX_BARS
#define COBS_MAX_BARS 40000
#endif
static const int MAX_BARS = COBS_MAX_BARS;            // a longer history restarts the engine from the newest bars

struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0, v = 0; };   // t = bar END, local (CT) seconds
struct Mark {
    long long t = 0; int idx = -1; int side = 0;     // side +1 bull, -1 bear
    std::string levels; double dratio = 0, ext = 0, adr = 0; int legStart = -1, legEnd = -1;
    bool r = false, v = false; int rBar = -1, vBar = -1, rN = 0, vN = 0;
    bool phOnly = false, show = true;   // (1.1.0) PHH / PHL the only level swept; drawn (false = recorded only)
    std::string label() const { std::string s = "Cob"; if (r || v) s += "+"; if (r) s += "R"; if (v) s += "V"; return s; }
};
struct Leg { int s = -1, e = -1, dir = 0; double size = 0; };

inline std::string mktKey(const std::string& root) { return root == "6E" ? std::string("EU") : root; }
inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }

struct Engine {
    std::string mkt; double tick = 0.25; int openMin = 510, closeMin = 900, perSec = 180;
    std::vector<Bar> B; std::vector<klv::Bar> K; std::vector<cimp::Bar> CB; std::vector<svl::Cls> cls;
    // ZigZag state (the legPivot walk, incremental)
    int dirn = 0, ext = 0, start = 0; std::vector<Leg> legs;
    // per session
    struct Sess { double h = -1e300, l = 1e300, c = 0; bool rth = false; double rh = -1e300, rl = 1e300; int rhI = -1, rlI = -1; bool lv = false; std::map<std::string, double> L; double adr = 0; };
    std::map<long, Sess> S;
    std::vector<long> doneOrder;                    // completed sessions in order (for the ADR)
    std::vector<Mark> marks; std::vector<std::string> out;   // decisions + record lines not yet written
    long long trimmed = 0; std::string lastWhy;
    void reset() { B.clear(); K.clear(); CB.clear(); cls.clear(); dirn = 0; ext = 0; start = 0; legs.clear(); lastLowPivot = -1; S.clear(); doneOrder.clear(); marks.clear(); out.clear(); lastWhy.clear(); }
    bool setMarket(const std::string& m, double tk, int spb) {
        mkt = mktKey(m); tick = tk > 0 ? tk : 0.25; perSec = spb > 0 ? spb : 180;
        return svl::rthOf(mkt.c_str(), openMin, closeMin);
    }
    double adrFor(long sess) const {               // 20 prior completed sessions' true range (>= 10)
        std::vector<long> prior; for (long s : doneOrder) if (s < sess) prior.push_back(s);
        if (prior.size() < 11) return 0;
        double sum = 0; int n = 0;
        for (size_t i = prior.size() >= 21 ? prior.size() - 20 : 1; i < prior.size(); ++i) {
            const Sess& a = S.at(prior[i]); const double pc = S.at(prior[i - 1]).c;
            sum += std::max(a.h - a.l, std::max(std::fabs(a.h - pc), std::fabs(a.l - pc))); ++n;
        }
        return n >= 10 ? sum / n : 0;
    }
    double avgPrev(int dir, size_t upto) const {     // the average of the previous 20 same-direction legs before legs[upto]
        double s = 0; int n = 0;
        for (size_t k = upto; k-- > 0 && n < 20;) if (legs[k].dir == dir) { s += legs[k].size; ++n; }
        return n >= 5 ? s / n : 0;
    }
    int prevPivotEnd(int dir, size_t upto) const { for (size_t k = upto; k-- > 0;) if (legs[k].dir == dir) return legs[k].e; return -1; }
    void addBar(const Bar& b) {
        if ((int)B.size() >= MAX_BARS) { trimmed += (long long)B.size(); reset(); }
        B.push_back(b);
        klv::Bar kb; kb.days = (long)floorDiv(b.t, 86400); kb.endSec = (int)(b.t - (long long)kb.days * 86400); kb.h = (float)b.h; kb.l = (float)b.l; kb.c = (float)b.c; kb.o = (float)b.o; kb.v = b.v;
        K.push_back(kb);
        cimp::Bar cb; cb.t = b.t; cb.o = b.o; cb.h = b.h; cb.l = b.l; cb.c = b.c; cb.v = b.v; CB.push_back(cb);
        const svl::Cls c = svl::classify(kb.days, kb.endSec, perSec, openMin, closeMin); cls.push_back(c);
        const int j = (int)B.size() - 1;
        // sessions: full high / low / last close; a new session closes the previous one (ADR)
        if (c.kind != svl::NONE) {
            if (!S.count(c.sess) && !S.empty()) { const long prev = S.rbegin()->first; if (prev < c.sess && (doneOrder.empty() || doneOrder.back() != prev)) doneOrder.push_back(prev); }
            Sess& s = S[c.sess]; s.h = std::max(s.h, b.h); s.l = std::min(s.l, b.l); s.c = b.c;
            if (c.kind == svl::RTH) {
                if (!s.rth) {                         // the first RTH bar: the session's key levels (as the Dealer Profile computes them), its ADR
                    s.rth = true; s.adr = adrFor(c.sess);
                    const std::vector<klv::Level> lv = klv::compute(K, perSec, openMin, closeMin, tick);
                    for (const klv::Level& x : lv) s.L[x.code] = x.price;
                    s.lv = true;
                }
                if (b.h > s.rh) { s.rh = b.h; s.rhI = j; }
                if (b.l < s.rl) { s.rl = b.l; s.rlI = j; }
            }
        }
        // 1) the CoB check on this bar with the ZigZag as it stood after the previous bar (trade_def.cob_live, 95% parity with his
        //    platform flags): Ups#1 = 1 = the bar before made the developing leg's extreme and this bar did not exceed it
        if (j >= 2) check(j, 1), check(j, -1);
        // 2) the ZigZag (his 1-bar walk) with this bar
        if (j == 1) { dirn = B[1].h >= B[0].h ? 1 : -1; ext = 0; start = 0; }
        if (j >= 1) {
            if (dirn == 1) {
                if (B[j].h > B[ext].h) ext = j;
                else if (B[j].l < B[j - 1].l && j != ext) { Leg L; L.s = start; L.e = ext; L.dir = 1; L.size = B[ext].h - B[start].l; legs.push_back(L); start = ext; ext = j; dirn = -1; }
            } else {
                if (B[j].l < B[ext].l) ext = j;
                else if (B[j].h > B[j - 1].h && j != ext) { Leg L; L.s = start; L.e = ext; L.dir = -1; L.size = B[start].h - B[ext].l; legs.push_back(L); lastLowPivot = ext; start = ext; ext = j; dirn = 1; }
            }
        }
    }
    int lastLowPivot = -1;
    bool useNew = true;                                         // PHH / PHL (2026-11-01); false = his RTL level set only (parity tests)
    bool legacy = false;                                        // (1.2.0) parity with his OLD signals (recorded flags): bear without room / cutoff
    bool hisSet = true;                                         // (1.2.0) with useNew false: true = his 6 levels (+ PWx / Asia), false = the old 4
    void check(int j, int side) {
        if (dirn != side || ext != j - 1) return;
        const Bar& bj = B[(size_t)j];
        if (side > 0 ? !(bj.h <= B[(size_t)ext].h) : !(bj.l >= B[(size_t)ext].l)) return;
        const svl::Cls& c = cls[(size_t)j];
        if (c.kind != svl::RTH || !S.count(c.sess)) return;
        const Sess& s = S[c.sess]; if (!s.lv) return;
        const double adr = s.adr; if (!(adr > 0)) return;
        int nPrev = 0; double sum = 0;
        for (size_t k = legs.size(); k-- > 0 && nPrev < 20;) if (legs[k].dir == side) { sum += legs[k].size; ++nPrev; }
        int nAll = 0; for (const Leg& L : legs) if (L.dir == side) { ++nAll; if (nAll >= 5) break; }
        if (nAll < 5 || nPrev == 0) return;
        const int p0 = start, p1 = ext;
        if (cls[(size_t)p0].sess != c.sess) return;                 // the leg starts in this session
        const double size = side > 0 ? B[(size_t)p1].h - B[(size_t)p0].l : B[(size_t)p0].h - B[(size_t)p1].l;
        const double dr = size / (sum / nPrev); if (dr < 1.9) return;
        if (p1 - p0 < 3) return;                                    // >= 3 bars (2026-10-24)
        const int mod = (int)((bj.t % 86400 + 86400) % 86400) / 60;
        auto lvl = [&](const char* n, double& v) { auto it = s.L.find(n); if (it == s.L.end()) return false; v = it->second; return true; };
        std::string sw; auto add = [&](const char* n) { if (!sw.empty()) sw += "/"; sw += n; };
        Mark m; m.t = bj.t; m.idx = j; m.side = side; m.dratio = dr; m.adr = adr; m.legStart = p0; m.legEnd = p1;
        double mn = 1e300, mx = -1e300; for (int k = std::max(0, j - 49); k <= j; ++k) { mn = std::min(mn, B[(size_t)k].l); mx = std::max(mx, B[(size_t)k].h); }
        if (side > 0) {
            const double lod = s.rl;
            if (!(B[(size_t)p0].l <= lod)) return;                  // the leg starts at the LOD
            const double hi = B[(size_t)p1].h;
            const char* N[] = { "ONL", "PDL", "PFL", "LonLO", "PWL", "AsiaLO", "PHL" };   // (1.2.0) his set (6) + PHL
            for (int q = 0; q < (useNew ? 7 : (hisSet ? 6 : 4)); ++q) { double v; if (lvl(N[q], v) && lod < v && v < hi) add(N[q]); }
            if (sw.empty()) return;
            if (!(bj.c - B[(size_t)p0].l >= 0.1 * adr)) return;     // room
            if (!(hi - lod < 0.4 * adr)) return;
            if (mod > 11 * 60) return;                              // by 11:00 CT
            if (std::fabs(lod - mn) > 1e-9) return;                 // LOD = MIN(LO,50)
            m.ext = lod;
        } else {
            const double hod = s.rh;
            if (!(B[(size_t)p0].h >= hod)) return;                  // the leg starts at the HOD
            const double lo = B[(size_t)p1].l;
            const char* N[] = { "ONH", "PDH", "PFH", "LonHI", "PWH", "AsiaHI", "PHH" };   // (1.2.0) his set (6) + PHH
            for (int q = 0; q < (useNew ? 7 : (hisSet ? 6 : 4)); ++q) { double v; if (lvl(N[q], v) && lo < v && v < hod) add(N[q]); }
            if (sw.empty()) return;
            if (!legacy && !(B[(size_t)p0].h - bj.c >= 0.1 * adr)) return;     // (1.2.0) his bear room: the leg-start high - close >= 0.1 ADR
            if (!legacy && mod > 11 * 60) return;                              // (1.2.0) by 11:00 CT (his TIME <= 1100), both sides
            if (!(lastLowPivot >= 0 && lo < B[(size_t)lastLowPivot].l)) return;   // below the previous down pivot
            if (!(hod - lo < 0.4 * adr)) return;
            if (std::fabs(hod - mx) > 1e-9) return;                 // HOD = MAX(HI,50)
            m.ext = hod;
        }
        m.levels = sw;
        m.phOnly = sw == "PHH" || sw == "PHL"; m.show = !m.phOnly || phShown(mkt);   // (1.1.0)
        cimp::Params P; const cimp::Tag T = cimp::evaluate(CB, j, side, P, false);
        m.r = T.r.bar >= 0; m.v = T.v.bar >= 0; m.rBar = T.r.bar; m.vBar = T.v.bar; m.rN = T.r.n; m.vN = T.v.n;
        marks.push_back(m); out.push_back(recLine(m));
    }
    std::string recLine(const Mark& m) const {
        char b[320];
        std::snprintf(b, sizeof b, "COB,%lld,%s,%s,%s,%.2f,%.6g,%.6g,%lld,%lld,%d,%d,%s", m.t, m.side > 0 ? "BULL" : "BEAR", m.label().c_str(), m.levels.c_str(), m.dratio, m.ext, m.adr,
                      B[(size_t)m.legStart].t, B[(size_t)m.legEnd].t, m.rN, m.vN, m.show ? "SHOW" : "LOG");   // (1.1.0) SHOW = drawn, LOG = recorded only
        return b;
    }
};

// ---- the drawing (SDK-free): the label under a bull CoB bar's low / over a bear's high, bull green / bear red, never over another label
struct View { int l = 0, t = 0, r = 800, b = 600, first = 0, last = 0; std::function<double(int)> X; std::function<double(double)> Y; std::function<int(const std::string&)> textW; };
struct Prim { int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned col = 0; std::string text; };
static const unsigned C_BULL = 0x0022C55E, C_BEAR = 0x00EF4444;   // 0x00RRGGBB
static const int GAP = 18;                                     // px from the bar's extreme (clear of Candle Absorb's codes on pivot bars)
inline std::vector<Prim> layout(const Engine& E, const View& v)
{
    std::vector<Prim> P; std::vector<std::pair<int, int>> used;   // label boxes (x0, y0) taken
    for (const Mark& m : E.marks) {
        if (!m.show || m.idx < v.first || m.idx > v.last || m.idx >= (int)E.B.size()) continue;   // (1.1.0) LOG CoBs are not drawn
        const Bar& b = E.B[(size_t)m.idx];
        const std::string s = m.label(); const int w = v.textW ? v.textW(s) : (int)s.size() * 7, h = 14;
        const double x = v.X(m.idx);
        int y = m.side > 0 ? (int)std::lround(v.Y(b.l)) + GAP : (int)std::lround(v.Y(b.h)) - GAP - h;
        for (int k = 0; k < 4; ++k) {                              // step away from another label at the same spot
            bool clash = false; for (auto& u : used) if (std::abs(u.first - (int)x) < w && std::abs(u.second - y) < h) clash = true;
            if (!clash) break; y += m.side > 0 ? h : -h;
        }
        if (y < v.t || y + h > v.b) continue;
        Prim p; p.x0 = (int)std::lround(x - w / 2.0); p.x1 = p.x0 + w; p.y0 = y; p.y1 = y + h; p.col = m.side > 0 ? C_BULL : C_BEAR; p.text = s;
        P.push_back(p); used.push_back(std::make_pair((int)x, y));
    }
    return P;
}

inline std::string statusText(const Engine& E, const std::string& sym, int spb, const std::string& rec)
{
    std::string o = std::string("VERSION,") + VERSION + "\nMARKET," + E.mkt + "\nSYMBOL," + sym + "\nBAR_SECONDS," + std::to_string(spb) +
                     "\nRTH," + std::to_string(E.openMin) + "-" + std::to_string(E.closeMin) + "\nBARS," + std::to_string(E.B.size()) + "\nLEGS," + std::to_string(E.legs.size()) +
                     "\nRULES,1-bar ZigZag; sweep ONH/ONL PDH/PDL PFH/PFL LonHI/LonLO PHH/PHL; dratio >= 1.9 (20 legs); >= 3 bars; 50-bar extreme; < 0.4 ADR; bull room 0.1 ADR, bull <= 11:00; PHH/PHL-only CoBs drawn in ES/HG/GC, recorded only elsewhere\n";
    o += "RECORD," + rec + "\nCOBS," + std::to_string(E.marks.size()) + "\n";
    const size_t n = E.marks.size();
    for (size_t k = n > 8 ? n - 8 : 0; k < n; ++k) o += "LAST," + E.recLine(E.marks[k]) + "\n";
    return o;
}

} // namespace cobs
