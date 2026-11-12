// rrrun.cpp 1.1.0 (2026-11-12) -- the lsRevRead engine (RevReadLogic.h) as a CLI for lra.rev_read (the nightly) and the jumpstart.
//
// Rassul 2026-11-12 06:08 CT: "go. and also build self improvement so you can better detect these sequences and their behaviors and their
// variants"; 2026-11-11: "jumpstart this indicator for all markets and refine"; "pay attention to session, volatility times, location and context".
//
//   rrrun <MKT> <tick> <rthOpenMin> <rthCloseMin> <configs.txt>   < records   > rows.jsonl
// records (stdin, binary, time order, one market, sessions in order):
//   <q t_ms><b act><b side><q px_ticks><i size><Q order_id><q aggressor_id>     act 0 NEW 1 MOD 2 DEL 3 FILL; 9 = a new session (t_ms = yyyymmdd)
//   act 4 (EXPERIMENTAL, NOT VALIDATED 2026-11-12: its bursts differ from the trades replay; the nightly does not use it) = a Databento "C" after a fill (raw store days): the lots a FILL already took are not cancelled twice; a raw FILL comes with side -1
//   (the engine takes the side from its book and ignores the aggressor's own fill record, whose order never rested)
//   t_ms = the CENTRAL wall clock in ms (the chart's clock); side = the feed's code (0 BID / 1 ASK; on a FILL 0 = the aggressor BOUGHT), -1 unknown
// configs.txt: one line per challenger, "name KEY=value KEY=value ..." (the keys of LRA-RevRead-<MKT>.csv); the first line is the live one.
// Every config runs its own engine over the same stream, with the same context the host gives the plugin (session, volatility window, the
// hourly step read from StepTrendLogic.h, key levels + VWAP + the HBO test line). Out: one JSON row per ended story (k = S), per pullback-absorbed
// candidate (k = P, the first config only) and per control candle (k = X: any candle at a new 5-candle extreme, the first config only), each with
// the truth AFTER its decision times, from the trades that followed:
// and (k = H, the hourly pilot, Rassul 2026-11-12 06:19 / 06:22 "how would this work translate to a htf hourly chart"; "starting with the ES as a
// pilot") one row per hourly candle at a new high / low of the 3 hours before it: its own POC test, and the 3-minute stories (the first config)
// that sat at its extreme; truth = 2R within 4 hours from the hour's close (stop 1 tick beyond the hourly extreme), the extreme held 3 hours.
//   r1 / r2  +1R / +2R reached before the stop (1), the stop first (0), neither inside 60 min (-1); entry = the last trade at the decision,
//            stop = 1 tick beyond the extreme so farPx (his pivot stop)
//   held     no trade beyond the extreme so farPx in the 30 min after; mfe / mae = the best / worst excursion in ticks in those 30 min
#include "../RevReadLogic.h"
#include "../StepTrendLogic.h"
#include <cstdint>
#include <cstring>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

#pragma pack(push, 1)
struct Rec { int64_t t; int8_t act, side; int64_t px; int32_t sz; uint64_t oid; int64_t ag; };
#pragma pack(pop)

struct Out { int r1 = -1, r2 = -1, held = 0; long long mfe = 0, mae = 0, risk = 0, entry = 0; bool ok = false; };
struct Sec { long long lo = 0, hi = 0, last = 0; };

struct Run {
    std::string mkt; double tick = 0.25; int rthO = 510, rthC = 900;
    std::vector<std::string> names; std::vector<std::unique_ptr<rvs::Engine> > E; std::vector<rvs::Params> PP;
    std::vector<std::vector<rvs::Story> > done;                       // per config: the session's ended stories
    // the session
    long long sess = 0, s0 = -1; std::vector<Sec> sec; long long lastPx = 0, lastT = 0;
    // the candle
    long long candle = 0, cLo = 0, cHi = 0, cClose = 0; std::map<long long, std::pair<long long, long long> > rows;
    // (1.1.0) one closed candle with what its own volume profile says about each end ([0] the low, [1] the high): the quiet-turn features
    struct QB { long long t = 0, sess = 0, o = 0, lo = 0, hi = 0, close = 0, total = 0, cdlt = 0; double pos = 0.5; long long e2v[2] = { 0, 0 }, e2d[2] = { 0, 0 }, t3v[2] = { 0, 0 }, t3d[2] = { 0, 0 }; std::string hr, lvl[2]; };
    std::vector<QB> cands; long long cOpen = 0;
    QB mk(const std::map<long long, std::pair<long long, long long> >& rw, long long t, long long o, long long lo, long long hi, long long close) {
        QB q; q.t = t; q.sess = sess; q.o = o; q.lo = lo; q.hi = hi; q.close = close; const long long rg = hi - lo; long long pv = -1, pt = lo;
        for (const auto& kv : rw) { const long long k = kv.first, v = kv.second.first, d = kv.second.second; q.total += v; q.cdlt += d; if (v > pv) { pv = v; pt = k; }
            if (k <= lo + 1) { q.e2v[0] += v; q.e2d[0] += d; } if (k >= hi - 1) { q.e2v[1] += v; q.e2d[1] += d; }
            if (3 * (k - lo) <= rg) { q.t3v[0] += v; q.t3d[0] += d; } if (3 * (hi - k) <= rg) { q.t3v[1] += v; q.t3d[1] += d; } }
        q.pos = rg > 0 ? (double)(pt - lo) / (double)rg : 0.5; q.hr = E[0]->ctx.hourly;
        for (int b = 0; b < 2; ++b) { long long bd = 5; for (const auto& lv : E[0]->levels) { const long long d = std::llabs(lv.first - (b ? hi : lo)); if (d < bd) { bd = d; q.lvl[b] = lv.second; } } }
        return q;
    }
    static double med(std::vector<double> v) { if (v.empty()) return 1; std::nth_element(v.begin(), v.begin() + (long)(v.size() / 2), v.end()); return std::max(1e-9, v[v.size() / 2]); }
    // every candle of `v` (3-minute: this session's; hourly: all) at a new extreme of the `look` candles before it, with its features and the truth
    void emitQ(const char* kind, const std::vector<QB>& v, size_t look, long long H, long long hold, bool hourly) {
        std::ostream& o = std::cout;
        for (size_t j = 20; j < v.size(); ++j) { const QB& c = v[j]; if (c.sess != sess || c.hi <= c.lo) continue;
            bool hi = true, lo = true; for (size_t k = j - look; k < j; ++k) { if (v[k].hi >= c.hi) hi = false; if (v[k].lo <= c.lo) lo = false; }
            if (hi == lo) continue;
            const int b = hi ? 1 : 0, sg = hi ? 1 : -1; std::vector<double> mt, mr, md; for (size_t k = j - 20; k < j; ++k) { mt.push_back((double)v[k].total); mr.push_back((double)(v[k].hi - v[k].lo)); md.push_back((double)std::llabs(v[k].cdlt)); }
            const double mT = med(mt), mR = med(mr), mD = med(md), rg = (double)(c.hi - c.lo);
            long long farPx = hi ? v[j - 1].lo : v[j - 1].hi, cd5 = 0; for (size_t k = j - 10; k < j; ++k) farPx = hi ? std::min(farPx, v[k].lo) : std::max(farPx, v[k].hi); for (size_t k = j - 5; k < j; ++k) cd5 += v[k].cdlt;
            const long long ext = hi ? c.hi : c.lo; int ft = -1; long long t2 = 0;
            if (j + 1 < v.size() && v[j + 1].sess == c.sess) { const QB& n = v[j + 1]; ft = hi ? (n.lo < c.lo && n.close < c.close ? 1 : 0) : (n.hi > c.hi && n.close > c.close ? 1 : 0); t2 = n.t; }
            const int cs = ctSec((c.t - 1) * 1000);
            o << "{\"k\":\"" << kind << "\",\"m\":\"" << mkt << "\",\"sess\":" << sess << ",\"side\":\"" << (hi ? "top" : "bottom") << "\",\"ct\":" << ctSec(c.t * 1000) << ",\"csess\":\"" << rvs::sessionOf(cs, rthO, rthC) << "\""
              << ",\"pos\":" << (hi ? 1.0 - c.pos : c.pos) << ",\"cp\":" << (hi ? (double)(c.hi - c.close) / rg : (double)(c.close - c.lo) / rg) << ",\"wick\":" << (hi ? (double)(c.hi - std::max(c.o, c.close)) / rg : (double)(std::min(c.o, c.close) - c.lo) / rg)
              << ",\"volx\":" << (double)c.total / mT << ",\"rngx\":" << rg / mR << ",\"dx\":" << (double)(sg * c.cdlt) / mD << ",\"e2s\":" << (c.total ? (double)c.e2v[b] / (double)c.total : 0) << ",\"e2dx\":" << (double)(sg * c.e2d[b]) / mD
              << ",\"t3s\":" << (c.total ? (double)c.t3v[b] / (double)c.total : 0) << ",\"t3dx\":" << (double)(sg * c.t3d[b]) / mD << ",\"move\":" << (double)(hi ? c.hi - farPx : farPx - c.lo) / mR << ",\"cd5\":" << (double)(sg * cd5) / mD
              << ",\"hs\":" << rvs::holdScore(hi ? (double)(c.hi - c.close) / rg : (double)(c.close - c.lo) / rg, (double)c.total / mT, rg / mR, (double)(hi ? c.hi - farPx : farPx - c.lo) / mR)
              << ",\"pvolx\":" << (double)v[j - 1].total / mT << ",\"ppos\":" << (hi ? 1.0 - v[j - 1].pos : v[j - 1].pos) << ",\"pt3s\":" << (v[j - 1].total ? (double)v[j - 1].t3v[b] / (double)v[j - 1].total : 0)
              << ",\"over\":" << (double)(hi ? c.hi - v[j - 1].hi : v[j - 1].lo - c.lo) / mR << ",\"vrat\":" << (v[j - 1].total ? (double)c.total / (double)v[j - 1].total : 0) << ",\"pcp\":" << (v[j - 1].hi > v[j - 1].lo ? (hi ? (double)(v[j - 1].hi - v[j - 1].close) : (double)(v[j - 1].close - v[j - 1].lo)) / (double)(v[j - 1].hi - v[j - 1].lo) : 0.5)
              << ",\"ft\":" << ft << ",\"hourly\":\"" << js(c.hr) << "\",\"level\":\"" << js(c.lvl[b]) << "\"";
            if (hourly) { int nC = 0, nA = 0; for (const rvs::Story& st : done[0]) { if (!st.shown || st.bottom == hi || st.t0 / 1000 < c.t - 3600 || st.t0 / 1000 >= c.t || std::llabs(st.extreme - ext) > 8) continue; ++nC; if (st.step >= rvs::S_ABSORBED) ++nA; }
                o << ",\"nC\":" << nC << ",\"nA\":" << nA; }
            put(o, "oA", outcome(c.t - 1, ext, hi ? -1 : 1, H, hold)); if (ft == 1) put(o, "oF", outcome(t2 - 1, ext, hi ? -1 : 1, H, hold)); o << "}\n"; }
    }
    // context
    long long vwN = 0; double vwPV = 0;                               // session VWAP (ticks)
    std::vector<stp::Bar> hb; stp::Bar hcur; long long hk = -1; int hDir = 0, hRot = 0; long long hPx = 0, hTop = 0; bool hBroken = false;
    long long pdh = 0, pdl = 0, onh = 0, onl = 0, lnh = 0, lnl = 0, rh = 0, rl = 0, psh = 0, psl = 0, sh = 0, sl = 0; bool onF = false, lnF = false;
    long long ctxSec = -1;
    std::vector<QB> hcs; std::map<long long, std::pair<long long, long long> > hrows;
    std::unordered_map<unsigned long long, long long> pendFill;       // raw days: lots filled per resting order, waiting for their "C"

    static std::string js(const std::string& s) { std::string o; for (char c : s) { if (c == '"' || c == '\\') o += '\\'; if ((unsigned char)c >= 32) o += c; } return o; }
    int ctSec(long long tMs) const { return (int)((tMs / 1000) % 86400); }
    Sec* at(long long s) { const long long i = s - s0; return i >= 0 && i < (long long)sec.size() ? &sec[(size_t)i] : nullptr; }

    void newSession(long long ymd) {
        endSession();
        sess = ymd; s0 = -1; pendFill.clear(); sec.clear(); cands.clear(); rows.clear(); candle = 0; vwN = 0; vwPV = 0; ctxSec = -1;
        if (rh > 0) { pdh = rh; pdl = rl; } if (sh > 0) { psh = sh; psl = sl; }
        rh = rl = onh = onl = lnh = lnl = sh = sl = 0; onF = lnF = false;
        for (size_t i = 0; i < E.size(); ++i) { E[i].reset(new rvs::Engine()); E[i]->P = PP[i]; E[i]->tick = tick; E[i]->spb = 180; done[i].clear(); }
    }
    void hourClose() {
        if (hk < 0) return;
        { hcs.push_back(mk(hrows, (hk + 1) * 3600, hcur.o, hcur.l, hcur.h, hcur.c)); hrows.clear(); if (hcs.size() > 4000) hcs.erase(hcs.begin(), hcs.begin() + 1000); }
        hb.push_back(hcur); if (hb.size() > 600) hb.erase(hb.begin(), hb.begin() + 100);
        stp::Engine X; X.hourly = true; for (const stp::Bar& b : hb) X.addBar(b);
        X.sealTo(hb.back().t + 3600);
        hDir = 0; hBroken = false; hRot = X.zdir; if (X.S.empty()) return;
        const stp::Step& q = X.S.back(); hDir = q.dir; hPx = q.px; hTop = stp::stepTop(q); hBroken = q.brI >= 0;
    }
    void context(long long tMs) {
        const long long s = tMs / 1000; if (s == ctxSec) return; ctxSec = s;
        const int cs = ctSec(tMs);
        rvs::Ctx c; c.session = rvs::sessionOf(cs, rthO, rthC); c.volWin = rvs::volWindow(cs, rthO, rthC);
        const long long lo = cLo > 0 ? std::min(cLo, lastPx) : lastPx, hi = cHi > 0 ? std::max(cHi, lastPx) : lastPx;
        if (lastPx > 0) c.hourly = rvs::hourlyText(hDir, hRot, hPx, hTop, hBroken, lastPx, lo, hi, 4, tick);
        std::vector<std::pair<long long, std::string> > L;
        auto add = [&](long long p, const char* n) { if (p > 0) L.push_back(std::make_pair(p, std::string(n))); };
        add(pdh, "PDH"); add(pdl, "PDL"); add(psh, "PSH"); add(psl, "PSL");
        const int m = cs / 60;
        if (m >= rthO && m < 1020) { add(onh, "ONH"); add(onl, "ONL"); }                        // the overnight range is known from the RTH open
        if (m >= 450 && m < 1020) { add(lnh, "LonHI"); add(lnl, "LonLO"); }                     // London 02:00-07:30, known from 07:30
        if (vwN > 0) add((long long)std::llround(vwPV / (double)vwN), "VWAP");
        if (hDir != 0 && !hBroken) add(hPx, "the HBO test line");
        char ck[32]; std::snprintf(ck, sizeof ck, "%02d:%02d:%02d", cs / 3600, (cs / 60) % 60, cs % 60);
        for (auto& e : E) { e->ctx = c; e->levels = L; e->clock = ck; }
    }
    void closeCandle() {
        if (candle == 0) return;
        std::vector<rvs::VRow> v; for (const auto& kv : rows) { rvs::VRow r; r.tick = kv.first; r.vol = kv.second.first; r.dlt = kv.second.second; v.push_back(r); }
        const rvs::Poc p = rvs::pocOf(candle, v, cLo, cHi, cClose);
        for (size_t i = 0; i < E.size(); ++i) { E[i]->onCandle(candle, p, cClose); drain(i); }
        cands.push_back(mk(rows, candle, cOpen, cLo, cHi, cClose));
        rows.clear(); cLo = cHi = 0;
    }
    void drain(size_t i) { for (const rvs::Story& s : E[i]->ended) done[i].push_back(s); E[i]->ended.clear(); }

    void onRec(const Rec& r) {
        if (r.act == 9) { newSession(r.t); return; }
        if (sess == 0) return;
        const long long px = (long long)r.px, sz = (long long)r.sz;
        const long long t = r.t, s = t / 1000;
        if (r.act == 3 && sz > 0 && px > 0) {
            if (s0 < 0) { s0 = s; sec.assign(24 * 3600, Sec()); }
            const long long cd = (s / 180 + 1) * 180; if (candle != 0 && cd > candle) closeCandle(); candle = cd;
            const long long k = s / 3600; if (k != hk) { if (hk >= 0) hourClose(); hk = k; hcur = stp::Bar(); hcur.t = k * 3600; hcur.o = hcur.h = hcur.l = hcur.c = px; }
            hcur.h = std::max(hcur.h, px); hcur.l = std::min(hcur.l, px); hcur.c = px;
            if (Sec* q = at(s)) { if (q->last == 0) { q->lo = q->hi = px; } else { q->lo = std::min(q->lo, px); q->hi = std::max(q->hi, px); } q->last = px; }
            if (cLo == 0) { cLo = cHi = px; cOpen = px; } else { cLo = std::min(cLo, px); cHi = std::max(cHi, px); } cClose = px;
            std::pair<long long, long long>& rw = rows[px]; rw.first += sz; rw.second += r.side == 0 ? sz : r.side == 1 ? -sz : 0;
            { std::pair<long long, long long>& hw = hrows[px]; hw.first += sz; hw.second += r.side == 0 ? sz : r.side == 1 ? -sz : 0; }
            vwN += sz; vwPV += (double)px * (double)sz; lastPx = px; lastT = t;
            const int m = ctSec(t) / 60;
            if (sh == 0) { sh = sl = px; } else { sh = std::max(sh, px); sl = std::min(sl, px); }
            if (m >= rthO && m < rthC) { if (rh == 0) { rh = rl = px; } else { rh = std::max(rh, px); rl = std::min(rl, px); } }
            else if (m >= 1020 || m < rthO) { if (onh == 0) { onh = onl = px; } else { onh = std::max(onh, px); onl = std::min(onl, px); } }
            if (m >= 120 && m < 450) { if (lnh == 0) { lnh = lnl = px; } else { lnh = std::max(lnh, px); lnl = std::min(lnl, px); } }
        }
        context(t);
        for (auto& e : E) e->onEvent(t, r.act, r.side, px, sz, (long long)r.oid, r.ag);
    }
    // the truth after a decision at second `ts` for a trade in direction dir (+1 long) with the stop 1 tick beyond `ext`
    Out outcome(long long ts, long long ext, int dir, long long H = 3600, long long hold = 1800) {
        Out o; Sec* a = nullptr; long long i = ts - s0; if (i < 0) i = 0;
        for (long long j = i; j >= 0 && j > i - 120; --j) if (j < (long long)sec.size() && sec[(size_t)j].last > 0) { a = &sec[(size_t)j]; break; }
        if (!a) return o;
        o.entry = a->last; const long long stop = ext - dir; o.risk = dir > 0 ? o.entry - stop : stop - o.entry; if (o.risk <= 0) return o;
        o.ok = true; o.held = 1; bool d1 = false, d2 = false;
        for (long long j = i + 1; j < (long long)sec.size() && j <= i + H; ++j) { const Sec& q = sec[(size_t)j]; if (q.last == 0) continue;
            const long long adv = dir > 0 ? q.hi - o.entry : o.entry - q.lo, agn = dir > 0 ? o.entry - q.lo : q.hi - o.entry;
            const bool stopped = dir > 0 ? q.lo <= stop : q.hi >= stop;
            if (j <= i + hold) { o.mfe = std::max(o.mfe, adv); o.mae = std::max(o.mae, agn); if (dir > 0 ? q.lo < ext : q.hi > ext) o.held = 0; }
            if (stopped) { if (!d1) { o.r1 = 0; d1 = true; } if (!d2) { o.r2 = 0; d2 = true; } }                // a second touching both = the stop
            if (!d1 && adv >= o.risk) { o.r1 = 1; d1 = true; }
            if (!d2 && adv >= 2 * o.risk) { o.r2 = 1; d2 = true; }
            if (d1 && d2 && j > i + hold) break; }
        return o;
    }
    long long extremeSince(long long fromS, long long toS, bool bottom) {
        long long x = 0; for (long long s = fromS; s <= toS; ++s) if (Sec* q = at(s)) { if (q->last == 0) continue; if (x == 0) x = bottom ? q->lo : q->hi; else x = bottom ? std::min(x, q->lo) : std::max(x, q->hi); }
        return x;
    }
    static void put(std::ostream& o, const char* k, const Out& x) { o << ",\"" << k << "\":{\"ok\":" << (x.ok ? 1 : 0) << ",\"r1\":" << x.r1 << ",\"r2\":" << x.r2 << ",\"held\":" << x.held << ",\"mfe\":" << x.mfe << ",\"mae\":" << x.mae << ",\"risk\":" << x.risk << "}"; }

    void endSession() {
        if (sess == 0 || s0 < 0) return;
        closeCandle();
        std::ostream& o = std::cout;
        for (size_t i = 0; i < E.size(); ++i) {
            for (int b = 0; b < 2; ++b) if (E[i]->live[b]) E[i]->finish(E[i]->st[b], b, rvs::E_RESET, lastT);
            drain(i);
            for (const rvs::Story& s : done[i]) {
                if (!s.shown) continue;                                                        // stories under the live threshold are for the plugin's own records, not for this replay
                const bool bt = s.bottom; const int dir = bt ? 1 : -1; const auto g = E[i]->gate.find(s.id);
                const int cs = ctSec(s.t0);
                o << "{\"k\":\"S\",\"cfg\":\"" << names[i] << "\",\"m\":\"" << mkt << "\",\"sess\":" << sess << ",\"id\":" << s.id << ",\"side\":\"" << (bt ? "bottom" : "top") << "\",\"shown\":" << (s.shown ? 1 : 0)
                  << ",\"ct\":" << cs << ",\"step\":" << s.step << ",\"end\":\"" << rvs::endName(s.end) << "\",\"burst\":" << s.burst << ",\"opp\":" << s.opp << ",\"big\":" << s.bigOrder << ",\"p95\":" << s.p95
                  << ",\"range\":" << s.range << ",\"shownsz\":" << s.shownSz << ",\"abs\":" << s.absLots << ",\"absx\":" << s.absR << ",\"fading\":" << (s.fading ? 1 : 0) << ",\"nret\":" << s.returns.size()
                  << ",\"flush\":" << s.flushLots << ",\"init\":" << s.initLots << ",\"initopp\":" << s.initOpp << ",\"merged\":" << s.merged << ",\"zone\":" << (s.zoneHi - s.zoneLo + 1)
                  << ",\"gate\":" << (g == E[i]->gate.end() ? -1 : g->second) << ",\"pocpos\":" << s.pocPos << ",\"dA\":" << (s.tA ? (s.tA - s.t0) / 1000 : -1) << ",\"dE\":" << (s.tE ? (s.tE - s.t0) / 1000 : -1) << ",\"dI\":" << (s.tI ? (s.tI - s.t0) / 1000 : -1)
                  << ",\"csess\":\"" << s.ctx.session << "\",\"volwin\":" << s.ctx.volWin << ",\"hourly\":\"" << js(s.ctx.hourly) << "\",\"level\":\"" << js(s.ctx.level) << "\",\"sweep\":\"" << js(s.sweep) << "\"";
                const long long f = s.t0 / 1000 - 10;
                put(o, "oC", outcome(s.t0 / 1000, extremeSince(f, s.t0 / 1000, bt), dir));
                if (s.tA) put(o, "oA", outcome(s.tA / 1000, extremeSince(f, s.tA / 1000, bt), dir));
                if (s.tE) put(o, "oE", outcome(s.tE / 1000, extremeSince(f, s.tE / 1000, bt), dir));
                if (s.tI) put(o, "oI", outcome(s.tI / 1000, extremeSince(f, s.tI / 1000, bt), dir));
                o << "}\n";
            }
        }
        // pullback-absorbed candidates (the live config) and the control candles
        for (const rvs::Pull& q : E[0]->pulls) { if (q.conf < 0) continue; const int dir = q.top ? -1 : 1;
            o << "{\"k\":\"P\",\"m\":\"" << mkt << "\",\"sess\":" << sess << ",\"id\":" << q.id << ",\"side\":\"" << (q.top ? "top" : "bottom") << "\",\"ct\":" << ctSec(q.t * 1000) << ",\"dlt\":" << q.cdlt << ",\"dx\":" << q.dx
              << ",\"pos\":" << q.pos << ",\"closepos\":" << q.closePos << ",\"total\":" << q.total << ",\"conf\":" << q.conf << ",\"csess\":\"" << q.ctx.session << "\",\"volwin\":" << q.ctx.volWin
              << ",\"hourly\":\"" << js(q.ctx.hourly) << "\",\"level\":\"" << js(q.ctx.level) << "\"";
            put(o, "oA", outcome(q.t - 1, q.ext, dir)); put(o, "oF", outcome(q.t2 - 1, q.ext, dir)); o << "}\n"; }
        emitQ("X", cands, 5, 3600, 1800, false);          // the 3-minute chart: every candle at a new 5-candle extreme (the control and the quiet-turn candidates)
        emitQ("H", hcs, 3, 4 * 3600, 3 * 3600, true);     // the hourly chart: every hour at a new 3-hour extreme
        s0 = -1;
    }
};

int main(int argc, char** argv) {
    if (argc < 6) { std::cerr << "usage: rrrun MKT tick rthOpenMin rthCloseMin configs.txt < records\n"; return 2; }
    Run R; R.mkt = argv[1]; R.tick = std::atof(argv[2]); R.rthO = std::atoi(argv[3]); R.rthC = std::atoi(argv[4]);
    std::ifstream cf(argv[5]); std::string l;
    while (std::getline(cf, l)) { std::stringstream ss(l); std::string nm, kv; if (!(ss >> nm) || nm[0] == '#') continue;
        rvs::Params p = rvs::forMarket(R.mkt);
        while (ss >> kv) { const size_t e = kv.find('='); if (e == std::string::npos) continue; const std::string k = kv.substr(0, e); const double v = std::atof(kv.c_str() + e + 1);
            if (k == "LOG_X") p.logX = v; else if (k == "VOL_X") p.volX = v; else if (!rvs::applyParam(p, k, v)) { std::cerr << "unknown key " << k << "\n"; return 2; } }
        R.names.push_back(nm); R.PP.push_back(p); }
    if (R.names.empty()) { std::cerr << "no configs\n"; return 2; }
    R.E.resize(R.names.size()); R.done.resize(R.names.size());
    std::ios::sync_with_stdio(false);
#ifdef _WIN32
    _setmode(_fileno(stdin), 0x8000);
#endif
    std::vector<Rec> buf(65536); long long n = 0;
    while (true) { const size_t got = std::fread(buf.data(), sizeof(Rec), buf.size(), stdin); if (got == 0) break; for (size_t i = 0; i < got; ++i) { R.onRec(buf[i]); ++n; } }
    R.endSession();
    std::cerr << "rrrun " << R.mkt << " records " << n << "\n";
    return 0;
}
