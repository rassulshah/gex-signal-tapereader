#ifndef LS_SR_LOGIC_H
#define LS_SR_LOGIC_H
// SRLogic.h (1.0.0, 2026-11-10) -- SDK-free rules of lsSR, the "SR" stop-run pane. Unit-tested on Linux (tests_sr2/).
// Rassul 2026-11-10: 18:24 CT "i'll use this as my stop run indicator. put the run and rev indications on the indicator";
// 18:30 "just put the ratio outside the stop run bar and then the arrow on the reversal bar so it is more compact";
// 18:35 "it should be at least a 5 bar high or low for it to be highlighted"; 18:37 "add the ratio for the highlighted bars";
// 18:43 "build it ... add it to the overnight process to automatically track and improve over time for all markets";
// 18:50 "call the new indicator SR instead of StopRun". Design: claude.ai/artifact/EctbmGWXrGd5cWxA7JSGKv (mockup v10).
// Study: level-reversal-analytics lra.stoprun_levels / lra.sweep_feats (frozen 2026-11-10 16:45 CT; GC + HG pay, ES / NQ / CL do not yet).
//
// WHAT IS DRAWN (one pane; everything from the journal, written once per decision - never repaints)
//   * per candle two bars: the strongest ONE-SECOND burst of small market orders (<= smallMax lots per order) of the buyers (up, green)
//     and of the sellers (down, red), as a ratio of the local normal (median of the prior 10 candles' strongest second, that side);
//     only seconds where that side out-trades the other 2 to 1 count. Faded below burstX (2.0x); BRIGHT with its ratio when >= burstX
//     AND the candle makes a hiLoBars-bar (5) high (buyers) / low (sellers).
//   * GOLD bar with its ratio: a STOP RUN through a key / MenthorQ level - at the first trade 1+ tick beyond the level, in the window
//     [t - 200 ms, t + 1000 ms]: small attacking orders >= burstX x normal, attackers' volume >= sideX x the other side, >= concShare
//     of it inside concMs (300 ms), and it reaches >= reachTicks beyond the level. Decided once, 1 second after the crossing.
//   * the REVERSAL ARROW on the candle that CLOSES BACK across the level after a gold run that made a new RTH extreme: within
//     recBars candles of the run candle and before the sweep exceeds penAtr x ATR. Red down (short) after a high, green up after a low.
//     drawRev = 0 (per market, from the nightly) = the arrow is recorded, not drawn.
//   * a gray diamond next to the arrow: on that candle the attackers' own resting side (bids after a run through a high) had more
//     pulled than added within zone ticks of price.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
namespace sr {
static const char* const VERSION = "1.0.0";
struct Params {
    double burstX = 2.0, sideX = 2.0, concShare = 0.5, penAtr = 1.6, zoneAtr = 0.25, atrTicks = 0;   // atrTicks 0 = the mean range of the session's closed RTH candles so far
    int smallMax = 5, concMs = 300, preMs = 200, postMs = 1000, reachTicks = 2, recBars = 6, hiLoBars = 5, normBars = 10, maxVisits = 4, drawRev = 0, zoneMin = 4, skipOpenBars = 3;   // skipOpenBars: runs in the first candles of RTH get no arrow (the study never scored them)
};
struct Level { std::string name; long long px = 0; int side = 0; long long fromSec = 0; };   // side +1 = a high (swept upward), -1 = a low
struct Bar { long long end = 0; long long o = 0, h = 0, l = 0, c = 0; int nU = 0, nD = 0; double xU = 0, xD = 0; bool hiN = false, loN = false; long long add[2] = { 0, 0 }, can[2] = { 0, 0 }; bool rth = false; };
struct Run { int id = 0; std::string lvl; long long px = 0; int side = 0, visit = 0; long long tMs = 0, barEnd = 0; double x = 0, conc = 0; long long vol = 0, ovol = 0, reach = 0; int nSmall = 0; bool gold = false, newExt = false;
             int state = 0; /* 0 watching, 1 reversed, 2 cancelled (too deep / too late / not a new extreme) */ long long ext = 0, revBarEnd = 0; bool pull = false; double penAtr = 0; int backBars = -1; };
class Engine {
public:
    Params P; int spb = 180; long long rthOpenSec = 0, rthCloseSec = 0;   // seconds of the (CT-as-epoch) day; set by the host per session
    std::vector<std::string> journal;                                     // append-only decision lines
    std::vector<Bar> bars; std::vector<Run> runs;
    // levels may be re-sent (the dealer file updates intraday): a level that did not change keeps its state and visit count
    void setLevels(const std::vector<Level>& L) {
        std::vector<LS> old; old.swap(lv_);
        for (size_t i = 0; i < L.size(); ++i) { LS s; s.L = L[i]; lv_.push_back(s); }
        merge();
        for (size_t i = 0; i < lv_.size(); ++i) for (size_t k = 0; k < old.size(); ++k) if (old[k].L.name == lv_[i].L.name && old[k].L.px == lv_[i].L.px) { lv_[i].st = old[k].st; lv_[i].visits = old[k].visits; if (lv_[i].L.side == 0) lv_[i].L.side = old[k].L.side; break; }
    }
    size_t levelCount() const { return lv_.size(); }
    void setNextId(int n) { if (n > nextId_) nextId_ = n; }
    // the side of a level from its name: ...H / ...HI = a high (+1), ...L / ...LO = a low (-1), anything else 0 = decided at the first RTH trade (above it = +1)
    static int sideOfName(const std::string& n) { const size_t z = n.size(); if (z >= 2 && (n.compare(z - 2, 2, "HI") == 0)) return 1; if (z >= 2 && n.compare(z - 2, 2, "LO") == 0) return -1; if (z >= 1 && n[z - 1] == 'H') return 1; if (z >= 1 && n[z - 1] == 'L') return -1; return 0; }
    // ---- feed -----------------------------------------------------------------------------------------------------------
    // one aggressive fill: tMs = feed time in ms (CT-as-epoch), px in ticks, ag +1 buyer / -1 seller, id = the aggressor order
    void onFill(long long tMs, long long px, long long sz, int ag, unsigned long long id) {
        advance(tMs); if (sz <= 0 || ag == 0) return;
        Bar& b = cur(tMs, px); if (px > b.h) b.h = px; if (px < b.l) b.l = px; b.c = px; last_ = px; haveLast_ = true;
        const long long sec = tMs / 1000; if (sec != sec_) closeSecond(sec);
        std::unordered_map<unsigned long long, long long>& m = ord_[ag > 0 ? 0 : 1]; m[id] += sz; vol_[ag > 0 ? 0 : 1] += sz;
        win_.push_back(F{ tMs, px, sz, ag, id }); while (!win_.empty() && win_.front().t < tMs - 5000) win_.erase(win_.begin());
        cross(tMs, px);
    }
    // a resting-order change near price: side 0 bid / 1 ask, add (true) or pull (false), sz lots, px ticks
    void onBook(long long tMs, int side, bool add, long long px, long long sz) {
        advance(tMs); if (!haveLast_ || sz <= 0 || side < 0 || side > 1 || bars.empty()) return;
        const long long d = side == 0 ? last_ - px : px - last_; if (d < 0 || d > zone()) return;
        Bar& b = bars.back(); (add ? b.add[side] : b.can[side]) += sz;
    }
    // time passes (also called by the host once a second with no events): closes seconds, candles, decides pending runs
    void advance(long long tMs) {
        if (tMs <= now_) return;
        now_ = tMs;
        for (size_t i = 0; i < pend_.size();) { if (tMs > pend_[i].t0 + P.postMs) { decide(pend_[i]); pend_.erase(pend_.begin() + i); } else ++i; }
        if (!bars.empty() && tMs / 1000 >= bars.back().end) closeBars(tMs / 1000);
    }
    double atr() const { if (P.atrTicks > 0) return P.atrTicks; return nRth_ > 0 ? sumRng_ / nRth_ : 0; }
    long long zone() const { const long long z = (long long)std::llround(P.zoneAtr * atr()); return z < P.zoneMin ? P.zoneMin : z; }
private:
    struct F { long long t, px, sz; int ag; unsigned long long id; };
    struct LS { Level L; int st = -1; int visits = 0; };            // st -1 unknown, 0 inside, 1 outside (beyond)
    struct Pend { long long t0; size_t lv; int visit; long long barEnd; };
    std::vector<LS> lv_; std::vector<F> win_; std::vector<Pend> pend_;
    std::unordered_map<unsigned long long, long long> ord_[2]; long long vol_[2] = { 0, 0 }; long long sec_ = -1, now_ = 0, last_ = 0; bool haveLast_ = false;
    int secN_[2] = { 0, 0 }; double sumRng_ = 0; int nRth_ = 0; long long rthHi_ = 0, rthLo_ = 0; bool haveRth_ = false; int nextId_ = 1;
    std::vector<int> pmax_[2];                                        // per closed candle: strongest second (all seconds), for the normal
    void merge() {                                                    // levels within 2 ticks on the same side = one (names joined)
        std::sort(lv_.begin(), lv_.end(), [](const LS& a, const LS& b) { return a.L.side != b.L.side ? a.L.side < b.L.side : a.L.px < b.L.px; });
        std::vector<LS> out; for (size_t i = 0; i < lv_.size(); ++i) { if (!out.empty() && out.back().L.side == lv_[i].L.side && std::llabs(out.back().L.px - lv_[i].L.px) <= 2 && out.back().L.fromSec == lv_[i].L.fromSec) out.back().L.name += "+" + lv_[i].L.name; else out.push_back(lv_[i]); }
        lv_.swap(out);
    }
    bool inRth(long long sec) const { const long long d = ((sec % 86400) + 86400) % 86400; return rthCloseSec > rthOpenSec ? (d >= rthOpenSec && d < rthCloseSec) : true; }
    Bar& cur(long long tMs, long long px) {
        const long long sec = tMs / 1000, end = (sec / spb + 1) * spb;
        if (bars.empty() || bars.back().end != end) { if (!bars.empty()) closeBars(sec); Bar b; b.end = end; b.o = b.h = b.l = b.c = px; b.rth = inRth(end - spb); bars.push_back(b); curMax_[0] = curMax_[1] = 0; curBest_[0] = curBest_[1] = 0; }
        return bars.back();
    }
    int curMax_[2] = { 0, 0 }, curBest_[2] = { 0, 0 }; size_t closed_ = 0;
    void closeSecond(long long sec) {
        if (sec_ >= 0) for (int s = 0; s < 2; ++s) {
            int n = 0; for (std::unordered_map<unsigned long long, long long>::const_iterator it = ord_[s].begin(); it != ord_[s].end(); ++it) if (it->second <= P.smallMax) ++n;
            if (n > curMax_[s]) curMax_[s] = n;
            if (vol_[s] >= P.sideX * vol_[1 - s] && n > curBest_[s]) curBest_[s] = n;
        }
        ord_[0].clear(); ord_[1].clear(); vol_[0] = vol_[1] = 0; sec_ = sec;
    }
    double normOf(int s, size_t nClosed) const {                      // median of the prior normBars closed candles' strongest second (> 0 only)
        std::vector<int> v; for (size_t i = nClosed; i-- > 0 && (int)(nClosed - i) <= P.normBars;) if (pmax_[s][i] > 0) v.push_back(pmax_[s][i]);
        if (v.empty()) return 1.0;
        std::sort(v.begin(), v.end()); const size_t n = v.size(); const double m = n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]); return m < 1.0 ? 1.0 : m;
    }
    void closeBars(long long nowSec) {
        while (closed_ < bars.size() && bars[closed_].end <= nowSec) {
            if (closed_ + 1 == bars.size()) closeSecond(bars[closed_].end);            // the last second of the candle
            Bar& b = bars[closed_]; const size_t i = closed_;
            b.nU = curBest_[0]; b.nD = curBest_[1]; b.xU = b.nU / normOf(0, i); b.xD = b.nD / normOf(1, i);
            pmax_[0].push_back(curMax_[0]); pmax_[1].push_back(curMax_[1]); curMax_[0] = curMax_[1] = curBest_[0] = curBest_[1] = 0;
            bool hi = i > 0, lo = i > 0; for (int k = 1; k < P.hiLoBars && (int)i - k >= 0; ++k) { if (bars[i - k].h > b.h) hi = false; if (bars[i - k].l < b.l) lo = false; }
            b.hiN = hi; b.loN = lo;
            if (b.rth) { sumRng_ += (double)(b.h - b.l); ++nRth_; if (!haveRth_) { rthHi_ = b.h; rthLo_ = b.l; haveRth_ = true; } }
            char z[256]; std::snprintf(z, sizeof z, "B|%lld|%lld|%lld|%lld|%lld|%d|%d|%.2f|%.2f|%d|%d", b.end, b.o, b.h, b.l, b.c, b.nU, b.nD, b.xU, b.xD, (int)b.hiN, (int)b.loN); journal.push_back(z);
            for (size_t r = 0; r < runs.size(); ++r) follow(runs[r], b, i);
            if (b.rth) { if (b.h > rthHi_) rthHi_ = b.h; if (b.l < rthLo_) rthLo_ = b.l; }
            ++closed_;
        }
    }
    void cross(long long tMs, long long px) {
        const long long sec = tMs / 1000; if (!inRth(sec)) { for (size_t i = 0; i < lv_.size(); ++i) if (lv_[i].L.side != 0) lv_[i].st = (px - lv_[i].L.px) * lv_[i].L.side >= 1 ? 1 : 0; return; }
        const long long z = zone();
        for (size_t i = 0; i < lv_.size(); ++i) { LS& s = lv_[i]; if (sec < s.L.fromSec) continue; if (s.L.side == 0) { if (s.L.px == px) continue; s.L.side = s.L.px > px ? 1 : -1; s.st = 0; continue; } const long long rel = (px - s.L.px) * s.L.side;
            if (s.st < 0) { s.st = rel >= 1 ? 1 : 0; continue; }
            if (s.st == 1) { if (rel <= -z) s.st = 0; continue; }
            if (rel >= 1 && s.visits < P.maxVisits) { s.st = 1; ++s.visits; pend_.push_back(Pend{ tMs, i, s.visits, bars.back().end }); }
        }
    }
    void decide(const Pend& p) {
        const LS& s = lv_[p.lv]; const int sd = s.L.side; Run r; r.id = nextId_++; r.lvl = s.L.name; r.px = s.L.px; r.side = sd; r.visit = p.visit; r.tMs = p.t0; r.barEnd = p.barEnd;
        std::unordered_map<unsigned long long, long long> ov; std::vector<std::pair<long long, long long> > at;
        for (size_t i = 0; i < win_.size(); ++i) { const F& f = win_[i]; if (f.t < p.t0 - P.preMs || f.t > p.t0 + P.postMs) continue;
            if (f.ag == sd) { r.vol += f.sz; ov[f.id] += f.sz; at.push_back(std::make_pair(f.t, f.sz)); const long long rc = (f.px - s.L.px) * sd; if (rc > r.reach) r.reach = rc; } else r.ovol += f.sz; }
        for (std::unordered_map<unsigned long long, long long>::const_iterator it = ov.begin(); it != ov.end(); ++it) if (it->second <= P.smallMax) ++r.nSmall;
        long long best = 0; for (size_t a = 0, b = 0, sum = 0; a < at.size(); ++a) { (void)sum; long long w = 0; for (b = a; b < at.size() && at[b].first <= at[a].first + P.concMs; ++b) w += at[b].second; if (w > best) best = w; }
        r.conc = r.vol > 0 ? (double)best / (double)r.vol : 0;
        size_t nClosed = 0; while (nClosed < bars.size() && bars[nClosed].end < p.barEnd) ++nClosed; if (nClosed > pmax_[sd > 0 ? 0 : 1].size()) nClosed = pmax_[sd > 0 ? 0 : 1].size();
        r.x = r.nSmall / normOf(sd > 0 ? 0 : 1, nClosed);
        r.gold = r.vol > 0 && r.x >= P.burstX && (double)r.vol >= P.sideX * (double)r.ovol && r.conc >= P.concShare && r.reach >= P.reachTicks;
        r.ext = s.L.px + sd * r.reach; r.state = r.gold ? 0 : 2;
        char z[320]; std::snprintf(z, sizeof z, "R|%d|%lld|%lld|%s|%lld|%d|%d|%.2f|%.2f|%lld|%lld|%lld|%d|%d", r.id, r.tMs, r.barEnd, r.lvl.c_str(), r.px, r.side, r.visit, r.x, r.conc, r.vol, r.ovol, r.reach, r.nSmall, (int)r.gold); journal.push_back(z);
        if (r.gold) runs.push_back(r);
    }
    void follow(Run& r, const Bar& b, size_t i) {
        if (r.state != 0 || b.end < r.barEnd) return;
        const int sd = r.side; const long long e = sd > 0 ? b.h : b.l; if ((e - r.ext) * sd > 0) r.ext = e;
        if (b.end == r.barEnd) r.newExt = haveRth_ ? (sd > 0 ? b.h >= rthHi_ : b.l <= rthLo_) : true;
        else if (!r.newExt && haveRth_) r.newExt = sd > 0 ? r.ext >= rthHi_ : r.ext <= rthLo_;
        const double a = atr(); r.penAtr = a > 0 ? (double)((r.ext - r.px) * sd) / a : 0; const int back = (int)((b.end - r.barEnd) / spb);
        int why = 0;
        if (rthOpenSec > 0 && ((((r.barEnd - spb) % 86400) + 86400) % 86400) < rthOpenSec + (long long)P.skipOpenBars * spb) why = 4;   // too early in RTH
        else if (a > 0 && r.penAtr > P.penAtr) why = 1;                                   // too deep
        else if ((b.c - r.px) * sd <= 0) { if (!r.newExt) why = 3; else { r.state = 1; r.revBarEnd = b.end; r.backBars = back; const int own = sd > 0 ? 0 : 1; r.pull = b.can[own] > b.add[own];
            char z[200]; std::snprintf(z, sizeof z, "V|%d|%lld|%s|%lld|%d|%d|%.2f|%d|%lld|%lld", r.id, b.end, r.lvl.c_str(), r.px, r.side, r.backBars, r.penAtr, (int)r.pull, r.ext, b.c); journal.push_back(z); return; } }
        else if (back >= P.recBars) why = 2;                                         // too late
        if (why) { r.state = 2; char z[120]; std::snprintf(z, sizeof z, "X|%d|%lld|%d|%.2f", r.id, b.end, why, r.penAtr); journal.push_back(z); }
        (void)i;
    }
};
// ---- the journal as the pane reads it (append-only; a restart reloads it: nothing repaints) -------------------------------------
struct JBar { double xU = 0, xD = 0; bool hiN = false, loN = false; };
struct JRun { int id = 0, side = 0; double x = 0; long long barEnd = 0; std::string lvl; };
struct JRev { int side = 0; bool pull = false; long long barEnd = 0; };
struct Journal {
    std::map<long long, JBar> bars; std::map<long long, JRun> gold; std::map<long long, JRev> rev; std::map<int, JRun> byId; long long lines = 0, bad = 0;
    static std::vector<std::string> split(const std::string& l) { std::vector<std::string> c; size_t a = 0; for (;;) { const size_t p = l.find('|', a); c.push_back(l.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; } return c; }
    void add(const std::string& l) {
        if (l.empty() || l[0] == 'H') return;
        const std::vector<std::string> c = split(l); ++lines;
        if (c[0] == "B" && c.size() >= 12) { JBar b; b.xU = std::atof(c[8].c_str()); b.xD = std::atof(c[9].c_str()); b.hiN = c[10] == "1"; b.loN = c[11] == "1"; bars[std::atoll(c[1].c_str())] = b; }
        else if (c[0] == "R" && c.size() >= 15) { if (c[14] != "1") return; JRun r; r.id = std::atoi(c[1].c_str()); r.barEnd = std::atoll(c[3].c_str()); r.lvl = c[4]; r.side = std::atoi(c[6].c_str()); r.x = std::atof(c[8].c_str());
            byId[r.id] = r; std::map<long long, JRun>::iterator it = gold.find(r.barEnd); if (it == gold.end() || r.x > it->second.x) gold[r.barEnd] = r; }
        else if (c[0] == "V" && c.size() >= 9) { JRev v; v.barEnd = std::atoll(c[2].c_str()); v.side = std::atoi(c[5].c_str()); v.pull = c[8] == "1"; rev[v.barEnd] = v; }
        else if (c[0] == "X") { }
        else ++bad;
    }
};
} // namespace sr
#endif
