// ZzoCobFinder.h - the CoB finder of lsZzoCombo 2.3.0 (2026-11-12), SDK-free. Used by ZzoCombo.cpp / ZzoComboView.h only.
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
//   3. Key level first: the origin went below a key low (ONL, AsiaLO, LonLO, PhrL, PFL, PDL), the wave came back over it and the completing
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
// MEASURED (lra study, 3-min bars 2025-04-28 .. 2026-11-11, 7 markets, ~2,700 market-sessions): 0.39 Cobs / session, the origin held to
//   the RTH close 70% (thirds 72 / 70 / 69), reached the 1.33 extension before a close beyond the origin 61%; with an Lvp 78% / 68%,
//   without one 49% / 48%; no key level 66% / 59%;
//   his RTL Cob signal on the same bars 53% / 54%. About break-even after costs at the completing bar's close: a read, not an entry.
// Levels here are the finder's own (bars only): overnight = bar END in (17:00, RTH open], Asia (18:00, 01:00], London (02:00,
//   min(07:30, RTH open)], prior hour (open - 60 min, open], prior full session, prior session's RTH. The prior week does not qualify (56% reached 1.33).
#pragma once
#include "ZzoDeltaLogic.h"
#include <climits>
#include <map>
#include <string>
#include <vector>

namespace zcf {
static const char* const COMBO_VERSION = "2.3.0";   // lsZzoCombo (the setVersion literal in ZzoCombo.cpp)
static const char* const VERSION = "1.0.0";         // the finder rule
static const int MIN_CNT = 10;                       // ZZO count ("zzo 10+", as his CoB filter today)
static const int WATCH_CNT = 5;                      // logged only (challenger)
static const double MAX_RETR = 0.46;                 // given back at the completing bar
static const int LVP_WAVES = 3;                      // pullback waves that can carry the Lvp
static const int NLV = 6;
static const char* const LV_HI[NLV] = { "ONH", "AsiaHI", "LonHI", "PhrH", "PFH", "PDH" };
static const char* const LV_LO[NLV] = { "ONL", "AsiaLO", "LonLO", "PhrL", "PFL", "PDL" };

struct FBar { long long t = 0, h = 0, l = 0, c = 0, sess = 0; bool rth = false; };
struct HL { bool any = false; long long h = 0, l = 0; void add(long long hi, long long lo) { if (!any || hi > h) h = hi; if (!any || lo < l) l = lo; any = true; } };
// one decision, keyed by the wave it sits on (its pivot bar's END time and its direction)
struct Tag { int cob = 0;            // +1 / -1 = a Cob on this wave
             int lvp = 0;            // +1 / -1 = the Lvp of a bullish / bearish Cob, on this (pullback) wave
             int watch = 0;          // logged only: 1 = no key level, 2 = count 5..9
             int lv = 0, cnt = 0; bool atExt = false; double retr = 0;
             long long tWave = 0, tCf = 0, origin = 0, top = 0, close = 0, tCob = 0; int dir = 0; };

inline std::string lvText(int mask, int dir) {
    std::string t; for (int i = 0; i < NLV; ++i) if (mask & (1 << i)) { if (!t.empty()) t += "/"; t += dir > 0 ? LV_LO[i] : LV_HI[i]; }
    return t.empty() ? "-" : t;
}

struct Finder {
    int openMin = -1, closeMin = -1; bool ok = false;
    std::vector<FBar> F;                                   // aligned with zzd::Engine::B
    zzd::Engine M;                                         // the same walk with the MARKET's session hours (its count only)
    bool haveSess = false; long long curSess = 0;
    HL on, as, lon, phr, full, rth, prevFull, prevRth;
    bool rthAny = false; long long rLo = 0, rHi = 0, rLoBarH = 0, rHiBarL = 0;   // RTH extremes so far; the high of the LOD bar, the low of the HOD bar
    std::map<long long, Tag> tags;
    struct Act { long long key = 0, origin = 0, top = 0, vol = 0, sess = 0; int dir = 0, n = 0; };
    std::vector<Act> act;
    std::vector<std::string> out;                          // record lines not yet taken by the host

    static long long key(long long tWave, int dir) { return tWave * 2 + (dir > 0 ? 1 : 0); }
    void reset() { M.reset(); F.clear(); haveSess = false; on = as = lon = phr = full = rth = prevFull = prevRth = HL(); rthAny = false; tags.clear(); act.clear(); out.clear(); }
    bool setMarket(const std::string& root, int spb = 180) { const std::string m = mkey::norm(root); ok = svl::rthOf(m.c_str(), openMin, closeMin);
        M.revBars = zzd::COMBO_REV_BARS; M.setMarket(m, spb); return ok; }
    // the trading session of a bar END stamp (local secs): bars after 17:00 belong to the next day
    static long long sessOf(long long t) { const long long days = zzd::floorDiv(t - 1, 86400); const long long sec = (t - 1) - days * 86400; return sec >= 17 * 3600 ? days + 1 : days; }

    void addBar(const zzd::Bar& eb, long long c) {
        const long long t = eb.t, h = eb.h, l = eb.l;
        M.addBar(eb); M.trimmed = 0;                        // trims itself at the same bar counts as the pane's engine
        FBar b; b.t = t; b.h = h; b.l = l; b.c = c; b.sess = sessOf(t);
        const long long days = zzd::floorDiv(t, 86400); const int mn = (int)((t - days * 86400) / 60);
        if (!haveSess || b.sess != curSess) {
            if (haveSess) { prevFull = full; prevRth = rth; }
            haveSess = true; curSess = b.sess; on = as = lon = phr = full = rth = HL(); rthAny = false; act.clear();
        }
        full.add(h, l);
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
    void trim(long long k) { if (k > 0) F.erase(F.begin(), F.begin() + (size_t)std::min<long long>(k, (long long)F.size())); }

    // call right after the engine completed a wave on the newest bar (W.back() = that wave; F is aligned with E.B)
    void onWave(const std::vector<zzd::Wave>& W) {
        if (!ok || W.empty() || F.empty()) return;
        const size_t nW = W.size(); const zzd::Wave& w = W[nW - 1]; const int j = (int)F.size() - 1;
        if (w.s < 0 || w.e < 0 || w.e > j) return;
        const FBar& cf = F[(size_t)j]; const long long tWave = F[(size_t)w.e].t; const int d = w.dir;
        // ---- Lvp of the Cobs still open
        for (size_t a = 0; a < act.size();) {
            Act& A = act[a]; bool drop = false;
            if (cf.sess != A.sess) drop = true;
            else if (d == -A.dir) {
                ++A.n;
                if ((long long)A.dir * (w.ext - A.origin) < 0) drop = true;                 // below the Cob's origin
                else if (w.vol * 3 <= A.vol) {
                    Tag& t = tags[key(tWave, d)]; t.lvp = A.dir; t.tWave = tWave; t.tCf = cf.t; t.tCob = A.key / 2; if (!t.dir) t.dir = d;
                    out.push_back("L|" + std::string(VERSION) + "|" + std::to_string(tWave) + "|" + std::to_string(cf.t) + "|" + (A.dir > 0 ? "UP" : "DN") + "|" + std::to_string(A.key / 2));
                    drop = true;
                } else if (A.n >= LVP_WAVES) drop = true;
            } else if ((long long)A.dir * (w.ext - A.top) > 0) drop = true;                   // a wave beyond the Cob's top
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
        if (!(retr <= MAX_RETR)) return;
        const HL* L[NLV] = { &on, &as, &lon, &phr, &prevFull, &prevRth }; int mask = 0;
        for (int i = 0; i < NLV; ++i) {
            if (!L[i]->any) continue;
            const long long p = d > 0 ? L[i]->l : L[i]->h;
            if ((long long)d * (p - origin) > 0 && (long long)d * (top - p) >= 0 && (long long)d * (c0 - p) > 0) mask |= 1 << i;
        }
        int cnt = w.cntCap ? zzd::CNT_CAP : w.cnt; const int cntPane = cnt; int cntMkt = -1;
        if (!M.W.empty() && M.W.back().e == w.e && M.W.back().dir == d && M.B.size() == F.size()) { const zzd::Wave& mw = M.W.back(); cntMkt = mw.cntCap ? zzd::CNT_CAP : mw.cnt; }
        if (cntMkt < 0) return;                                                              // the two walks disagree: never guess
        cnt = std::min(cntPane, cntMkt);
        const bool cob = mask != 0 && cnt >= MIN_CNT;
        const int watch = cob ? 0 : (mask == 0 && cnt >= MIN_CNT) ? 1 : (mask != 0 && cnt >= WATCH_CNT) ? 2 : 0;
        if (!cob && !watch) return;
        Tag& t = tags[key(tWave, d)];
        t.dir = d; t.tWave = tWave; t.tCf = cf.t; t.origin = origin; t.top = top; t.close = c0; t.lv = mask; t.cnt = cnt; t.retr = retr;
        t.atExt = d > 0 ? origin == rLo : origin == rHi;
        if (cob) { t.cob = d; Act A; A.key = key(tWave, d); A.origin = origin; A.top = top; A.vol = w.vol; A.sess = cf.sess; A.dir = d; act.push_back(A); }
        else t.watch = watch;
        char b[96]; std::snprintf(b, sizeof b, "|%d|%.2f|%s|%s|%d|%d", cnt, retr, t.atExt ? "at" : "near", bo ? "bo" : "-", cntPane, cntMkt);
        out.push_back(std::string(cob ? "C|" : "W|") + VERSION + "|" + std::to_string(tWave) + "|" + std::to_string(cf.t) + "|" + (d > 0 ? "UP" : "DN") + "|" + std::to_string(origin) + "|" +
                      std::to_string(top) + "|" + std::to_string(c0) + "|" + lvText(mask, d) + b + (cob ? "" : watch == 1 ? "|no level" : "|count under 10"));
    }
    const Tag* find(long long tWave, int dir) const { auto it = tags.find(key(tWave, dir)); return it == tags.end() ? nullptr : &it->second; }
    // keep the map bounded: drop tags older than the oldest bar held
    void prune() { if (F.empty() || tags.size() < 4000) return; const long long t0 = F.front().t; for (auto it = tags.begin(); it != tags.end();) { if (it->second.tWave < t0) it = tags.erase(it); else ++it; } }
    std::string statusText() const {
        std::string s = std::string("COBFINDER,") + VERSION + (ok ? ",RTH " + std::to_string(openMin) + "-" + std::to_string(closeMin) : ",off - no RTH hours for this market") +
            ",rule near RTH extreme + key level swept and closed back + count >= " + std::to_string(MIN_CNT) + " + given back <= 0.46\n";
        int n = 0;
        for (auto it = tags.rbegin(); it != tags.rend() && n < 12; ++it) {
            const Tag& t = it->second; if (!t.cob && !t.lvp) continue; ++n;
            if (t.cob) { char b[200]; std::snprintf(b, sizeof b, "COB,%lld,%s,%s,count %d,gave back %.2f,%s\n", t.tWave, t.cob > 0 ? "UP" : "DN", lvText(t.lv, t.cob).c_str(), t.cnt, t.retr, t.atExt ? "at the extreme" : "near the extreme"); s += b; }
            if (t.lvp) { char b[120]; std::snprintf(b, sizeof b, "LVP,%lld,of the %s Cob at %lld\n", t.tWave, t.lvp > 0 ? "UP" : "DN", t.tCob); s += b; }
        }
        return s;
    }
};
}  // namespace zcf
