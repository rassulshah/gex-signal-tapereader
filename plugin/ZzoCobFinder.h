// ZzoCobFinder.h - the CoB finder of lsZzoCombo 2.4.0 (2026-11-12), SDK-free. Used by ZzoCombo.cpp / ZzoComboView.h only.
//
// Rassul 2026-11-12: "the wave zzo would be upgraded to show the waves with count labels but also identify cobs" / "cob should be near
// the extremes, near the hod or lod for the Rth session" / "basically a sweep of a key level first" / "make sure you have the zzo count
// like we do today and now you can add the lvp" / "a cob is a strong wave that typically leads to change from a down trend to an
// uptrend and vice versa" / 08:56 "we are just upgrading the zzo wave ... do not change anything beyond the indicator".
//
// RULE (bullish; bearish = mirror), decided ONCE on the bar that completes the wave (the 3-bar reversal), never repainted:
//   1. RTH: the wave's origin bar and its completing bar are RTH bars of the same session (svl::rthOf hours of the market).
//   2. Near the extreme: the origin low is at or below the HIGH of the bar that made the RTH low so far (his LOD-bar rule; the RTH low
//      itself qualifies).
//   3. Key level first: the origin went below a key low (ONL, AsiaLO, LonLO, PhrL, PFL, PDL) or the prior day's POC / VAH / VAL, the wave came back over it and the completing
//      bar CLOSED back above it. (PDL = the prior session's RTH low.)
//   4. A strong wave: ZZO count >= 10 (the pane's own count) and the completing bar has given back <= 0.46 of the wave. It does NOT have
//      to take out the prior pivot (Rassul 09:07 "it doesnt necessarily need to be the wave that breaks the downtrend, it only
//      kickstarts it"; measured: the break adds 2 pts of held and nothing on the 1.33 rule) - whether it did is logged (bo).
//   THE COUNT: the pane's own count (what is drawn) AND the count of the same wave with the market's RTH split (zzd::Engine M below)
//   must both be >= 10. On charts whose root has no session hours in the engine (QGC / QHG / QCL / NGX6 / EU6) the pane's count has
//   no overnight / RTH split and runs high: Cobs passing on the pane's count alone held 53-60% (GC / HG / CL / NG, 400 sessions)
//   against 66-85% for those passing both. On ES / NQ the two counts are the same number.
//   Lvp: among the first 3 pullback waves after a Cob that stay inside the Cob wave (none below its origin, no wave above its top), the
//   first with <= 1/3 of the Cob wave's volume; tagged on that pullback wave when it completes.
// MEASURED (lra study, 3-min bars 2025-04-28 .. 2026-11-11, 7 markets, ~2,700 market-sessions): 0.44 Cobs / session, the origin held to
//   the RTH close 71% (thirds 73 / 70 / 69), reached the 1.33 extension before a close beyond the origin 62%; with an Lvp 79% / 68%,
//   without one 49% / 49%; no key level 63% / 56%; Cobs that qualify ONLY on DPoc / VAH / VAL 76% / 67% (131);
//   his RTL Cob signal on the same bars 53% / 54%. About break-even after costs at the completing bar's close: a read, not an entry.
// Levels here are the finder's own (bars only): overnight = bar END in (17:00, RTH open], Asia (18:00, 01:00], London (02:00,
//   min(07:30, RTH open)], prior hour (open - 60 min, open], prior full session, prior session's RTH. The prior week does not qualify (56% reached 1.33).
// (1.2.0, lsZzoCombo 2.4.0, Rassul 2026-11-12 11:25 "Cob2 is fine" / 11:27 "I want a read box on the left of the indicator pane" / 11:32
//   "usually when a cob occurs it turns the hourly as well" / 11:38 "in the Lvl you can add SR+ in the beginning" / 11:40 "cob's can have
//   deep retracements" / 11:42 "there should be no requirement for giveback because cob can test the origin bar": the 0.46 limit of
//   rule 4 is REMOVED for the Cob (it stays the Cob2 limit). Measured: the 193 Cobs it adds (retraced > 0.46 at the completing bar)
//   held 38% (58% with an Lvp, 26% without) against 70% for the rest - shown by the read box's Hold row, HOLD_DEEP):
//   COB2 (the quiet turn; a wave that passed rules 1 and 2 and the 0.46 limit and is NOT a Cob - no count, no key level needed):
//     a. within 2 bars of the bar that made the RTH low (the pivot of the down-wave that made it) a bar closed above that bar's high,
//     b. the completing bar closes above the high of the 10 bars before that RTH-low bar,
//     c. the completing bar's close is >= 5 bar ranges above the RTH low (range = the median of the 20 bars before the RTH-low bar),
//     d. the completing bar is at least 60 minutes after the RTH open.
//     MEASURED (same history): 525 Cob2 that are not a Cob (0.19 / session), held 76% (thirds 72 / 82 / 73), reached 1.33 66%.
//   HR (the hourly turn, decided ONCE on the completing bar): its close is above the high of the clock hour BEFORE the hour of the
//     origin (below the low for a bearish wave). Cobs: held 80% with it (526), 61% without (577); it adds nothing to a Cob2 (76 / 77).
//   SR+ = a key level was swept and closed back over (rule 3); every Cob has it, a Cob2 only when it happened.
//   HOLD (the read box): held-to-the-RTH-close rate of the state, from the same history - HOLD_COB / HOLD_COB2 below
//     [Hr no / yes][Lvp still possible / Lvp printed / no Lvp].
// (1.3.0, lsZzoCombo 2.5.0, Rassul 2026-11-12 11:45 "score the quality of the bars in the cob ... the number of consecutive green bars,
//   the number of big green bars that are above the atr, the volume of the bars", 12:09 "ok. go", 12:10 "you should also check for high
//   end closes, they also expose the quality of the cob"): Q = the QUALITY of the wave's own bars (origin + 1 .. the top), 0..6, one
//   point each (bullish; bearish = mirror; ATR = Wilder 14 as of the bar before the origin):
//     1. 14 or more bars            2. the wave is 8 ATR or more          3. no dip inside the wave deeper than 0.3 of the wave
//     4. 5 or more up bars with a range >= 1 ATR      5. a run of 4 or more up bars      6. 6 or more up bars closing in their top third
//   MEASURED (1,296 Cobs, same history): Q 4-6 held 85% / 89% with an Lvp (last third 83%); Q 3 68%; Q 0-2 57%. Volume per bar, the
//   share of up bars and the wick at the top did not separate. Q is logged on every C / Q line and drives the read box's Hold row.
#pragma once
#include "ZzoDeltaLogic.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace zcf {
static const char* const COMBO_VERSION = "2.5.0";   // (2.5.0) + the quality score Q; (2.4.0) + Cob2, the hourly turn, the read box   // lsZzoCombo (the setVersion literal in ZzoCombo.cpp)
static const char* const VERSION = "1.3.0";         // the finder rule (1.3.0: + Q; 1.2.0: + Cob2, Hr; 1.1.0: + the prior full session's POC / VAH / VAL)
static const int MIN_CNT = 10;                       // ZZO count ("zzo 10+", as his CoB filter today)
static const int WATCH_CNT = 5;                      // logged only (challenger)
static const double MAX_RETR = 0.46;                 // (1.2.0) no longer a Cob requirement: beyond it a Cob is "deep" (its own Hold rate); still the Cob2 limit
static const int HOLD_DEEP[3] = { 38, 58, 26 };      // a Cob that had retraced > 0.46 at the completing bar [Lvp pending, yes, none]
static const int LVP_WAVES = 3;                      // pullback waves that can carry the Lvp
static const double COB2_DIST = 5.0;                 // Cob2: bar ranges from the RTH extreme to the completing close
static const int COB2_MIN_OPEN = 60;                 // Cob2: minutes after the RTH open
static const int COB2_PRE = 10, COB2_MR = 20;        // Cob2: bars before the extreme bar (the high to close beyond; the median range)
static const int Q_BARS = 14, Q_BIG = 5, Q_RUN = 4, Q_HEC = 6, ATR_N = 14; static const double Q_SIZE = 8.0, Q_PULL = 0.3;   // the six Q rules
// held % [Q band 0-2, 3, 4-6][Hr no, yes][Lvp pending, yes, none]; a cell under 20 cases takes its band's rate
static const int HOLD_COB[3][2][3] = { { { 54, 65, 38 }, { 65, 77, 44 } }, { { 59, 64, 50 }, { 77, 80, 68 } }, { { 80, 85, 57 }, { 87, 90, 63 } } };
static const int HOLD_COB2[3][3] = { { 67, 79, 56 }, { 74, 74, 74 }, { 85, 90, 58 } };   // [Q band][Lvp]; the hourly turn adds nothing to a Cob2
static const int NLV = 9;                              // 0..5 = a high / low pair; 6..8 = one price, swept from either side
static const char* const LV_HI[NLV] = { "ONH", "AsiaHI", "LonHI", "PhrH", "PFH", "PDH", "DPoc", "VAH", "VAL" };
static const char* const LV_LO[NLV] = { "ONL", "AsiaLO", "LonLO", "PhrL", "PFL", "PDL", "DPoc", "VAH", "VAL" };

struct FBar { long long t = 0, h = 0, l = 0, c = 0, sess = 0; bool rth = false; long long o = 0; double atr = 0; };   // (1.3.0) o = the open, atr = Wilder ATR(14) through this bar
// (1.1.0, Rassul 2026-11-12 09:32 "Add prior day poc vah Val also") the prior FULL session's volume profile, as KeyLevelsLogic.h profileOf
// (1-tick rows; POC = the biggest row, a tie to the row nearest the middle then the lower; 70% value area by the CBOT two-row rule) -
// but built from BARS (each bar's volume spread evenly over its ticks), the only data the 400-session test had; it can sit a few
// ticks from the VAP-built DPoc / VAH / VAL lines of lsKeyLevels.
struct VA { bool ok = false; long long poc = 0, vah = 0, val = 0; };
inline VA profileOf(const std::map<long long, double>& rows, double vaPct = 0.70) {
    VA P; std::vector<long long> key; std::vector<double> vol; double tot = 0;
    for (const auto& kv : rows) if (kv.second > 0) { key.push_back(kv.first); vol.push_back(kv.second); tot += kv.second; }
    if (key.empty()) return P;
    const double mid = 0.5 * ((double)key.front() + (double)key.back()); size_t pi = 0;
    for (size_t i = 1; i < vol.size(); ++i) {
        if (vol[i] > vol[pi] + 1e-9) pi = i;
        else if (std::fabs(vol[i] - vol[pi]) <= 1e-9 && std::fabs((double)key[i] - mid) < std::fabs((double)key[pi] - mid) - 1e-9) pi = i;
    }
    size_t lo = pi, hi = pi; double cum = vol[pi]; const double need = vaPct * tot;
    while (cum + 1e-9 < need && (lo > 0 || hi + 1 < vol.size())) {
        double up = 0, dn = 0; size_t nu = 0, nd = 0;
        for (size_t j = hi + 1; j < vol.size() && nu < 2; ++j, ++nu) up += vol[j];
        for (size_t j = lo; j > 0 && nd < 2; --j, ++nd) dn += vol[j - 1];
        if (nu == 0) { lo -= nd; cum += dn; continue; }
        if (nd == 0) { hi += nu; cum += up; continue; }
        if (up > dn + 1e-9) { hi += nu; cum += up; }
        else if (dn > up + 1e-9) { lo -= nd; cum += dn; }
        else { hi += nu; lo -= nd; cum += up + dn; }
    }
    P.poc = key[pi]; P.vah = key[hi]; P.val = key[lo]; P.ok = true; return P;
}
struct HL { bool any = false; long long h = 0, l = 0; void add(long long hi, long long lo) { if (!any || hi > h) h = hi; if (!any || lo < l) l = lo; any = true; } };
// one decision, keyed by the wave it sits on (its pivot bar's END time and its direction)
struct Tag { int cob = 0;            // +1 / -1 = a Cob on this wave
             int lvp = 0;            // +1 / -1 = the Lvp of a bullish / bearish Cob, on this (pullback) wave
             int watch = 0;          // logged only: 1 = no key level, 2 = count 5..9
             int cob2 = 0;           // (1.2.0) +1 / -1 = a Cob2 on this wave (never together with cob)
             bool hr = false;        // (1.2.0) the hourly turn, decided on the completing bar
             int q = -1;             // (1.3.0) the quality score 0..6 (-1 = not scored)
             int lvpSt = 0;          // (1.2.0) of a Cob / Cob2: 0 = an Lvp can still come, 1 = it printed, 2 = none
             int lv = 0, cnt = 0; bool atExt = false; double retr = 0;
             long long tWave = 0, tCf = 0, origin = 0, top = 0, close = 0, tCob = 0; int dir = 0; };

inline std::string lvText(int mask, int dir) {
    std::string t; for (int i = 0; i < NLV; ++i) if (mask & (1 << i)) { if (!t.empty()) t += "/"; t += dir > 0 ? LV_LO[i] : LV_HI[i]; }
    return t.empty() ? "-" : t;
}

struct Read { bool any = false; int kind = 0, dir = 0, lv = 0, cnt = 0, lvpSt = 0, hold = 0, q = -1; bool hr = false; double retr = 0; long long tCf = 0; };
struct HrBar { long long id = 0, h = 0, l = 0; };

struct Finder {
    int openMin = -1, closeMin = -1; bool ok = false;
    std::vector<FBar> F;                                   // aligned with zzd::Engine::B
    zzd::Engine M;                                         // the same walk with the MARKET's session hours (its count only)
    bool haveSess = false; long long curSess = 0;
    HL on, as, lon, phr, full, rth, prevFull, prevRth;
    std::map<long long, double> prof; VA prevVA;           // this session's bar-spread profile; the prior session's POC / VAH / VAL
    static const long long MAX_BAR_TICKS = 20000;          // a bar wider than this (bad data) is not spread
    std::vector<HrBar> hrs;                                // (1.2.0) the last clock hours (id = hour of the bar END - 1 s)
    bool rthAny = false; long long rLo = 0, rHi = 0, rLoBarH = 0, rHiBarL = 0;   // RTH extremes so far; the high of the LOD bar, the low of the HOD bar
    std::map<long long, Tag> tags;
    struct Act { long long key = 0, origin = 0, top = 0, vol = 0, sess = 0; int dir = 0, n = 0; };
    std::vector<Act> act;
    std::vector<std::string> out;                          // record lines not yet taken by the host

    static long long key(long long tWave, int dir) { return tWave * 2 + (dir > 0 ? 1 : 0); }
    void reset() { M.reset(); F.clear(); haveSess = false; on = as = lon = phr = full = rth = prevFull = prevRth = HL(); prof.clear(); prevVA = VA(); rthAny = false; tags.clear(); act.clear(); out.clear(); hrs.clear(); }
    bool setMarket(const std::string& root, int spb = 180) { const std::string m = mkey::norm(root); ok = svl::rthOf(m.c_str(), openMin, closeMin);
        M.revBars = zzd::COMBO_REV_BARS; M.setMarket(m, spb); return ok; }
    // the trading session of a bar END stamp (local secs): bars after 17:00 belong to the next day
    static long long sessOf(long long t) { const long long days = zzd::floorDiv(t - 1, 86400); const long long sec = (t - 1) - days * 86400; return sec >= 17 * 3600 ? days + 1 : days; }

    // o = the bar's open (LLONG_MIN = not given: the previous close)
    void addBar(const zzd::Bar& eb, long long c, long long o = LLONG_MIN) {
        const long long t = eb.t, h = eb.h, l = eb.l;
        M.addBar(eb); M.trimmed = 0;                        // trims itself at the same bar counts as the pane's engine
        FBar b; b.t = t; b.h = h; b.l = l; b.c = c; b.sess = sessOf(t);
        b.o = o != LLONG_MIN ? o : (F.empty() ? c : F.back().c); if (b.o > h) b.o = h; if (b.o < l) b.o = l;
        if (F.empty()) b.atr = (double)(h - l);
        else { const long long pc = F.back().c; const double tr = (double)std::max(h - l, std::max(std::llabs(h - pc), std::llabs(l - pc))); b.atr = F.back().atr + (tr - F.back().atr) / (double)ATR_N; }
        const long long days = zzd::floorDiv(t, 86400); const int mn = (int)((t - days * 86400) / 60);
        if (!haveSess || b.sess != curSess) {
            if (haveSess) { prevFull = full; prevRth = rth; prevVA = profileOf(prof); }
            for (const Act& A : act) closeAct(A, 2);
            haveSess = true; curSess = b.sess; on = as = lon = phr = full = rth = HL(); prof.clear(); rthAny = false; act.clear();
        }
        full.add(h, l);
        { const long long id = zzd::floorDiv(t - 1, 3600);
          if (hrs.empty() || hrs.back().id != id) { HrBar x; x.id = id; x.h = h; x.l = l; hrs.push_back(x); if (hrs.size() > 96) hrs.erase(hrs.begin(), hrs.begin() + 48); }
          else { if (h > hrs.back().h) hrs.back().h = h; if (l < hrs.back().l) hrs.back().l = l; } }
        if (eb.vol > 0 && h >= l && h - l < MAX_BAR_TICKS) { const double v = (double)eb.vol / (double)(h - l + 1); for (long long p = l; p <= h; ++p) prof[p] += v; }
        if (ok) {
            if (mn > 1020 || mn <= openMin) on.add(h, l);
            if (mn > 1080 || mn <= 60) as.add(h, l);
            if (mn > 120 && mn <= std::min(450, openMin)) lon.add(h, l);
            if (mn > openMin - 60 && mn <= openMin) phr.add(h, l);
            b.rth = mn > openMin && mn <= closeMin;
            if (b.rth) {
                rth.add(h, l);
                if (!rthAny || l < rLo) { rLo = l; rLoBarH = h; }
                if (!rthAny || h > rHi) { rHi = h; rHiBarL = l; }
                rthAny = true;
            }
        }
        F.push_back(b);
    }
    void trim(long long k) { if (k > 0) { const long long n = std::min<long long>(k, (long long)F.size()); F.erase(F.begin(), F.begin() + (size_t)n); } }
    void closeAct(const Act& A, int st) { auto it = tags.find(A.key); if (it != tags.end() && it->second.lvpSt == 0) it->second.lvpSt = st; }
    // (1.2.0) the hourly turn: the close against the clock hour before the hour of the origin bar
    bool hourTurn(long long tOrigin, int d, long long c0) const {
        const long long id = zzd::floorDiv(tOrigin - 1, 3600);
        for (size_t i = hrs.size(); i-- > 1;) if (hrs[i].id == id) return d > 0 ? c0 > hrs[i - 1].h : c0 < hrs[i - 1].l;
        return false;
    }
    // (1.3.0) Q: the quality of the wave's own bars io + 1 .. e (io = the origin bar, e = the top bar); -1 when it cannot be scored
    int quality(int d, int io, int e) const {
        if (io < 1 || e <= io || e >= (int)F.size()) return -1;
        const double atr = F[(size_t)io - 1].atr; if (!(atr > 0)) return -1;
        auto Hm = [&](int i) { return d > 0 ? F[(size_t)i].h : -F[(size_t)i].l; }; auto Lm = [&](int i) { return d > 0 ? F[(size_t)i].l : -F[(size_t)i].h; };
        const long long leg = Hm(e) - Lm(io); if (leg <= 0) return -1;
        int n = 0, run = 0, best = 0, big = 0, hec = 0; long long mx = Hm(io + 1), dip = 0;
        for (int i = io + 1; i <= e; ++i) {
            ++n; const long long hh = Hm(i), ll = Lm(i), r = hh - ll; const bool up = (long long)d * (F[(size_t)i].c - F[(size_t)i].o) > 0;
            run = up ? run + 1 : 0; if (run > best) best = run;
            if (up && (double)r >= atr) ++big;
            if (up && r > 0 && 3 * ((long long)d * F[(size_t)i].c - ll) >= 2 * r) ++hec;
            if (hh > mx) mx = hh;
            if (mx - ll > dip) dip = mx - ll;
        }
        return (n >= Q_BARS) + ((double)leg / atr >= Q_SIZE) + ((double)dip / (double)leg < Q_PULL) + (big >= Q_BIG) + (best >= Q_RUN) + (hec >= Q_HEC);
    }
    // (1.2.0) Cob2 rules a..d (the caller has checked RTH, near the extreme and the 0.46 limit); j = the completing bar
    // The "bar that made the RTH low" = the pivot bar of the nearest of the last 5 down-waves whose low IS the RTH low so far (as
    // tested; an RTH low that is not such a pivot was not in the test and gets no Cob2).
    bool cob2Test(int d, int j, const std::vector<zzd::Wave>& W) const {
        const FBar& cf = F[(size_t)j]; long long ix = -1; const long long ext = d > 0 ? rLo : rHi;
        for (int k = 1; k <= 9 && k < (int)W.size(); k += 2) {
            const zzd::Wave& x = W[W.size() - 1 - (size_t)k];
            if (x.dir != -d || x.e < 0 || x.e > j) break;
            if (x.ext == ext && F[(size_t)x.e].rth && F[(size_t)x.e].sess == cf.sess) { ix = x.e; break; }
        }
        if (ix < COB2_MR || ix >= j) return false;
        const long long days = zzd::floorDiv(cf.t, 86400); const int mn = (int)((cf.t - days * 86400) / 60);
        if (mn - openMin < COB2_MIN_OPEN) return false;
        auto Hm = [&](long long i) { return d > 0 ? F[(size_t)i].h : -F[(size_t)i].l; }; auto Lm = [&](long long i) { return d > 0 ? F[(size_t)i].l : -F[(size_t)i].h; };
        auto Cm = [&](long long i) { return (long long)d * F[(size_t)i].c; };
        bool ft = false; for (long long i = ix + 1; i <= ix + 2 && i <= j; ++i) if (Cm(i) > Hm(ix)) ft = true;
        if (!ft) return false;
        long long pre = Hm(ix - 1); for (long long i = ix - COB2_PRE; i < ix; ++i) if (Hm(i) > pre) pre = Hm(i);
        if (!(Cm(j) > pre)) return false;
        std::vector<long long> rg; for (long long i = ix - COB2_MR; i < ix; ++i) rg.push_back(F[(size_t)i].h - F[(size_t)i].l);
        std::sort(rg.begin(), rg.end()); const double mr = 0.5 * (double)(rg[COB2_MR / 2 - 1] + rg[COB2_MR / 2]);
        return mr > 0 && (double)(Cm(j) - Lm(ix)) / mr >= COB2_DIST;
    }

    // call right after the engine completed a wave on the newest bar (W.back() = that wave; F is aligned with E.B)
    void onWave(const std::vector<zzd::Wave>& W) {
        if (!ok || W.empty() || F.empty()) return;
        const size_t nW = W.size(); const zzd::Wave& w = W[nW - 1]; const int j = (int)F.size() - 1;
        if (w.s < 0 || w.e < 0 || w.e > j) return;
        const FBar& cf = F[(size_t)j]; const long long tWave = F[(size_t)w.e].t; const int d = w.dir;
        // ---- Lvp of the Cobs still open
        for (size_t a = 0; a < act.size();) {
            Act& A = act[a]; bool drop = false;
            if (cf.sess != A.sess) { drop = true; closeAct(A, 2); }
            else if (d == -A.dir) {
                ++A.n;
                if ((long long)A.dir * (w.ext - A.origin) < 0) { drop = true; closeAct(A, 2); }  // below the Cob's origin
                else if (w.vol * 3 <= A.vol) {
                    Tag& t = tags[key(tWave, d)]; t.lvp = A.dir; t.tWave = tWave; t.tCf = cf.t; t.tCob = A.key / 2; if (!t.dir) t.dir = d;
                    out.push_back("L|" + std::string(VERSION) + "|" + std::to_string(tWave) + "|" + std::to_string(cf.t) + "|" + (A.dir > 0 ? "UP" : "DN") + "|" + std::to_string(A.key / 2));
                    drop = true; closeAct(A, 1);
                } else if (A.n >= LVP_WAVES) { drop = true; closeAct(A, 2); }
            } else if ((long long)A.dir * (w.ext - A.top) > 0) { drop = true; closeAct(A, 2); }   // a wave beyond the Cob's top
            if (drop) act.erase(act.begin() + (long)a); else ++a;
        }
        // ---- the Cob test
        if (nW < 2) return;
        const zzd::Wave& pw = W[nW - 2];
        if (pw.dir == d || pw.e < 0 || pw.e > j) return;
        const FBar& ob = F[(size_t)pw.e];
        const long long origin = pw.ext, top = w.ext, leg = (long long)d * (top - origin), c0 = cf.c;
        if (leg <= 0 || !cf.rth || !ob.rth || ob.sess != cf.sess || !rthAny) return;
        if ((long long)d * (c0 - origin) <= 0) return;                                      // already closed beyond the origin
        const bool nearExt = d > 0 ? origin <= rLoBarH : origin >= rHiBarL;
        if (!nearExt) return;
        const bool bo = nW >= 3 && (long long)d * (top - W[nW - 3].ext) > 0;
        const double retr = (double)((long long)d * (top - c0)) / (double)leg;
        const HL* L[6] = { &on, &as, &lon, &phr, &prevFull, &prevRth }; const long long V[3] = { prevVA.poc, prevVA.vah, prevVA.val }; int mask = 0;
        for (int i = 0; i < NLV; ++i) {
            if (i < 6 ? !L[i]->any : !prevVA.ok) continue;
            const long long p = i < 6 ? (d > 0 ? L[i]->l : L[i]->h) : V[i - 6];
            if ((long long)d * (p - origin) > 0 && (long long)d * (top - p) >= 0 && (long long)d * (c0 - p) > 0) mask |= 1 << i;
        }
        int cnt = w.cntCap ? zzd::CNT_CAP : w.cnt; const int cntPane = cnt; int cntMkt = -1;
        if (!M.W.empty() && M.W.back().e == w.e && M.W.back().dir == d && M.B.size() == F.size()) { const zzd::Wave& mw = M.W.back(); cntMkt = mw.cntCap ? zzd::CNT_CAP : mw.cnt; }
        const bool cntOk = cntMkt >= 0;                                                      // the two walks disagree: never guess a count
        cnt = cntOk ? std::min(cntPane, cntMkt) : 0;
        const bool cob = cntOk && mask != 0 && cnt >= MIN_CNT;
        const int watch = cob || !cntOk ? 0 : (mask == 0 && cnt >= MIN_CNT) ? 1 : (mask != 0 && cnt >= WATCH_CNT) ? 2 : 0;
        const bool cob2 = !cob && retr <= MAX_RETR && cob2Test(d, j, W);
        if (!cob && !watch && !cob2) return;
        const bool hr = hourTurn(ob.t, d, c0);
        Tag& t = tags[key(tWave, d)];
        t.dir = d; t.tWave = tWave; t.tCf = cf.t; t.origin = origin; t.top = top; t.close = c0; t.lv = mask; t.cnt = cnt; t.retr = retr;
        t.atExt = d > 0 ? origin == rLo : origin == rHi;
        t.hr = hr; t.q = quality(d, pw.e, w.e);
        if (cob2) t.cob2 = d;
        if (cob || cob2) { if (cob) t.cob = d; Act A; A.key = key(tWave, d); A.origin = origin; A.top = top; A.vol = w.vol; A.sess = cf.sess; A.dir = d; act.push_back(A); }
        if (!cob) t.watch = watch;
        char b[96]; std::snprintf(b, sizeof b, "|%d|%.2f|%s|%s|%d|%d", cnt, retr, t.atExt ? "at" : "near", bo ? "bo" : "-", cntPane, cntMkt);
        const std::string body = std::string(VERSION) + "|" + std::to_string(tWave) + "|" + std::to_string(cf.t) + "|" + (d > 0 ? "UP" : "DN") + "|" + std::to_string(origin) + "|" +
                      std::to_string(top) + "|" + std::to_string(c0) + "|" + lvText(mask, d) + b;
        const std::string qs = "|" + std::to_string(t.q);
        if (cob) out.push_back("C|" + body + (hr ? "|H" : "|-") + qs);
        else if (watch) out.push_back("W|" + body + (watch == 1 ? "|no level" : "|count under 10"));
        if (cob2) out.push_back("Q|" + body + (hr ? "|H" : "|-") + qs);
    }
    // (1.2.0) the read box: the newest Cob / Cob2 of the CURRENT session
    Read read() const {
        Read R; int n = 0;
        for (auto it = tags.rbegin(); it != tags.rend() && n < 400; ++it, ++n) {
            const Tag& t = it->second; if (!t.cob && !t.cob2) continue;
            if (!haveSess || sessOf(t.tCf) != curSess) { if (t.tCf < (F.empty() ? 0 : F.back().t) - 2 * 86400) break; continue; }
            if (R.any && t.tCf <= R.tCf) continue;
            R.any = true; R.kind = t.cob ? 1 : 2; R.dir = t.cob ? t.cob : t.cob2; R.lv = t.lv; R.cnt = t.cnt; R.lvpSt = t.lvpSt; R.hr = t.hr; R.retr = t.retr; R.tCf = t.tCf; R.q = t.q;
            const int ls = t.lvpSt < 0 || t.lvpSt > 2 ? 0 : t.lvpSt;
            const int qb = t.q >= 4 ? 2 : t.q == 3 ? 1 : 0;
            R.hold = t.cob && t.retr > MAX_RETR ? HOLD_DEEP[ls] : t.cob ? HOLD_COB[qb][t.hr ? 1 : 0][ls] : HOLD_COB2[qb][ls];
        }
        return R;
    }
    const Tag* find(long long tWave, int dir) const { auto it = tags.find(key(tWave, dir)); return it == tags.end() ? nullptr : &it->second; }
    // keep the map bounded: drop tags older than the oldest bar held
    void prune() { if (F.empty() || tags.size() < 4000) return; const long long t0 = F.front().t; for (auto it = tags.begin(); it != tags.end();) { if (it->second.tWave < t0) it = tags.erase(it); else ++it; } }
    std::string statusText() const {
        std::string s = std::string("COBFINDER,") + VERSION + (ok ? ",RTH " + std::to_string(openMin) + "-" + std::to_string(closeMin) : ",off - no RTH hours for this market") +
            ",rule near RTH extreme + key level swept and closed back + count >= " + std::to_string(MIN_CNT) + " (no retracement limit); Cob2 = quiet turn (close back over the extreme bar in 2 bars + close beyond the 10 bars before it + 5 ranges + 60 min after the open)\n";
        int n = 0;
        for (auto it = tags.rbegin(); it != tags.rend() && n < 12; ++it) {
            const Tag& t = it->second; if (!t.cob && !t.lvp && !t.cob2) continue; ++n;
            if (t.cob2) { char b[200]; std::snprintf(b, sizeof b, "COB2,%lld,%s,%s,quality %d,gave back %.2f,hourly %s\n", t.tWave, t.cob2 > 0 ? "UP" : "DN", lvText(t.lv, t.cob2).c_str(), t.q, t.retr, t.hr ? "turned" : "not turned"); s += b; }
            if (t.cob) { char b[200]; std::snprintf(b, sizeof b, "COB,%lld,%s,%s,count %d,quality %d,gave back %.2f,%s\n", t.tWave, t.cob > 0 ? "UP" : "DN", lvText(t.lv, t.cob).c_str(), t.cnt, t.q, t.retr, t.atExt ? "at the extreme" : "near the extreme"); s += b; }
            if (t.lvp) { char b[120]; std::snprintf(b, sizeof b, "LVP,%lld,of the %s Cob at %lld\n", t.tWave, t.lvp > 0 ? "UP" : "DN", t.tCob); s += b; }
        }
        return s;
    }
};
}  // namespace zcf
