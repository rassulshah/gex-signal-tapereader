// replay_sr.cpp -- lsSweepRefill 1.1.0 replay on real CME MBO through the SAME SweepRefillLogic.h the plugin runs.
// Input: a binary of Databento MBO records for ONE contract (replay/export.py: ts_ns, ct_sec, px_ticks, oid, sz, act, side) and a
// levels file (replay/levels.py): "K <name> <px>" key levels for the session, "M <ct_sec> <name> <px>" MenthorQ intraday snapshots.
// Databento -> IRT DEPTH_BY_ORDER semantics: A = NEW; M = MOD (new size / price); C = a cancel of `size` lots -> MOD (the rest) or DEL;
// F = FILL of the RESTING order (the book is changed by the C that follows it, as IRT's FILL then MOD / DEL); T / N = ignored;
// R = book clear. 3-min bars are built from the F prints (bar END keys, CT seconds; the session starts 17:00 CT).
// Output (stdout): one CSV line per decided candidate + the outcome: which came first after the sweep end, extreme +1 ATR or -1 ATR.
#include "../SweepRefillLogic.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#pragma pack(push, 1)
struct Rec { long long ts, ct, px; unsigned long long oid; int sz; unsigned char act, side; unsigned short pad; };
#pragma pack(pop)
static_assert(sizeof(Rec) == 40, "record size");

struct Snap { long long ct; std::vector<swr::Level> lv; };
struct Out { swr::Event e; long long up = 0, dn = 0; int first = 0; long long decidedCt = 0; };   // first +1 = up first, -1 = down, 0 = none

int main(int argc, char** argv) {
    if (argc < 4) { std::fprintf(stderr, "usage: replay_sr <bin> <levels> <tag> [LRA-StopSweep-<MKT>.csv]\n"); return 2; }
    swr::Detector D;
    if (argc > 4) {   // the per-market params file (as the plugin reads it); missing = defaults with every mark LOG
        std::ifstream f(argv[4]); std::vector<std::string> L; std::string ln; while (std::getline(f, ln)) L.push_back(ln);
        const swg::StopSweep X = swg::parseStopSweep(L, std::string(argv[3]).substr(0, 2)); swr::applyParams(D.P, X);
        std::fprintf(stderr, "params %s: %s, min prices %d, refill %.0f%% within %d s + %d s, pivots %d\n", argv[4], X.why.c_str(), D.P.minPrices, D.P.refillPct, D.P.windowS, D.P.stayS, D.P.refPivots);
    }
    const std::string tag = argv[3];
    { int o = 0, c = 0; if (!swg::rthOf(tag.substr(0, 2), o, c)) { o = 510; c = 900; } D.setRth(o, c); }
    int rthO = 510, rthC = 900; swg::rthOf(tag.substr(0, 2), rthO, rthC);
    // levels
    std::vector<swr::Level> keys; std::vector<Snap> mq;
    { std::ifstream f(argv[2]); std::string ln;
      while (std::getline(f, ln)) {
          std::istringstream s(ln); std::string k; s >> k;
          if (k == "K") { swr::Level L; double px; s >> L.name >> px; L.px = (long long)llround(px / 0.25); L.kind = swr::REF_KEY; L.dir = swr::keyDir(L.name); if (L.dir) keys.push_back(L); }
          else if (k == "M") { long long ct; std::string n; double px; s >> ct >> n >> px; if (mq.empty() || mq.back().ct != ct) { Snap z; z.ct = ct; mq.push_back(z); }
              swr::Level L; L.name = n; L.px = (long long)llround(px / 0.25); L.kind = swr::REF_MQ; L.dir = 0; mq.back().lv.push_back(L); }
      } }
    // no-repaint restart check (always on): after every candle close a FRESH ZigZag built from all candles so far (a restart) must
    // equal the running one, and every record line written so far must read back byte-identical (the record is what a restart redraws)
    std::vector<swr::Bar> allBars; std::vector<std::vector<std::pair<long long, double> > > allRows; std::map<long long, double> formRows; std::vector<std::string> recLines; long long restartChecks = 0, restartFails = 0;
    size_t mqAt = 0; long long mqCur = -1;
    auto setLevels = [&](long long ct) {
        while (mqAt < mq.size() && mq[mqAt].ct <= ct) { mqCur = (long long)mqAt; ++mqAt; }
        D.levels = keys; if (mqCur >= 0) D.levels.insert(D.levels.end(), mq[(size_t)mqCur].lv.begin(), mq[(size_t)mqCur].lv.end());
    };
    // book of the Databento side (to turn C into MOD / DEL)
    std::unordered_map<unsigned long long, std::pair<long long, long long> > db;   // oid -> (px, size)
    std::FILE* in = std::fopen(argv[1], "rb"); if (!in) { std::perror("open"); return 2; }
    std::vector<Rec> buf(1 << 16);
    long long barEnd = 0, bo = 0, bh = 0, bl = 0, bc = 0, lastCt = 0; bool haveBar = false; long long nRec = 0;
    std::vector<Out> outs; std::vector<size_t> open;   // decided candidates still waiting for their 1 ATR outcome
    long long lastLevelsCt = -1;
    auto sessionOf = [](long long ct) { return (ct + 7 * 3600) / 86400; };
    auto collect = [&](long long ct) {
        for (size_t i = 0; i < D.decided.size(); ++i) {
            Out o; o.e = D.decided[i]; o.decidedCt = ct; recLines.push_back(swr::recLine(o.e, 0.25));
            const long long a = (long long)llround(o.e.atrTicks > 0 ? o.e.atrTicks : 1), ext = swr::bullish(o.e) ? o.e.loPx : o.e.hiPx;
            o.up = ext + a; o.dn = ext - a; outs.push_back(o); open.push_back(outs.size() - 1);
        }
        D.decided.clear();
    };
    size_t got;
    while ((got = std::fread(buf.data(), sizeof(Rec), buf.size(), in)) > 0) {
        for (size_t k = 0; k < got; ++k) {
            const Rec& r = buf[k]; ++nRec;
            const long long ms = r.ts / 1000000, ct = r.ct;
            if (ct / 5 != lastLevelsCt) { lastLevelsCt = ct / 5; setLevels(ct); }
            // bars: a print at / after the forming bar's END closes it
            if (r.act == 'F' && r.px > 0) {
                const long long end = (ct / D.P.spb + 1) * D.P.spb;
                if (haveBar && end != barEnd) {
                    swr::Bar b; b.t = barEnd; b.o = bo; b.h = bh; b.l = bl; b.c = bc; b.session = sessionOf(barEnd - 1);
                    b.day = barEnd / 86400; b.rth = swg::isRth((int)(barEnd % 86400), (int)D.P.spb, rthO, rthC); D.advance(ms); collect(ct);
                    std::vector<std::pair<long long, double> > rows(formRows.begin(), formRows.end()); formRows.clear();
                    D.barClose(b, &rows); haveBar = false;
                    allBars.push_back(b); allRows.push_back(rows);
                    { swr::Detector R; R.P = D.P; R.Z.rthLenSec = D.Z.rthLenSec; for (size_t q = 0; q < allBars.size(); ++q) if (allBars[q].t > allBars.back().t - 86400) R.barClose(allBars[q], &allRows[q]);
                      bool same = R.Z.swings.size() == D.Z.swings.size();
                      for (size_t q = 0; same && q < R.Z.swings.size(); ++q) same = R.Z.swings[q].px == D.Z.swings[q].px && R.Z.swings[q].barT == D.Z.swings[q].barT && R.Z.swings[q].dir == D.Z.swings[q].dir;
                      same = same && R.V.now.poc == D.V.now.poc && R.V.now.vah == D.V.now.vah && R.V.now.val == D.V.now.val;
                      const std::vector<swr::Event> back = swr::parseRecord(recLines, 0.25);
                      same = same && back.size() == recLines.size();
                      for (size_t q = 0; same && q < back.size(); ++q) same = swr::recLine(back[q], 0.25) == recLines[q];
                      ++restartChecks; if (!same) ++restartFails; }
                }
                if (!haveBar) { haveBar = true; barEnd = end; bo = bh = bl = bc = r.px; }
                bh = std::max(bh, (long long)r.px); bl = std::min(bl, (long long)r.px); bc = r.px; formRows[r.px] += r.sz;
                for (size_t q = 0; q < open.size();) {
                    Out& o = outs[open[q]];
                    if (r.px >= o.up) o.first = 1; else if (r.px <= o.dn) o.first = -1;
                    if (o.first != 0) open.erase(open.begin() + (long)q); else ++q;
                }
            }
            lastCt = ct;
            swr::Ev e; e.ms = ms; e.sec = ct; e.id = (long long)r.oid; e.px = r.px;
            const int side = r.side == 'B' ? swr::BID : r.side == 'A' ? swr::ASK : -1;
            switch (r.act) {
            case 'A': e.action = swr::NEW; e.side = side; e.size = r.sz; db[r.oid] = std::make_pair(r.px, (long long)r.sz); D.onEvent(e); break;
            case 'M': e.action = swr::MOD; e.side = side; e.size = r.sz; db[r.oid] = std::make_pair(r.px, (long long)r.sz); D.onEvent(e); break;
            case 'C': { auto it = db.find(r.oid); if (it == db.end()) { e.action = swr::DEL; D.onEvent(e); break; }
                        const long long rest = it->second.second - r.sz;
                        if (rest <= 0) { db.erase(it); e.action = swr::DEL; D.onEvent(e); }
                        else { it->second.second = rest; e.action = swr::MOD; e.side = side; e.size = rest; e.px = it->second.first; D.onEvent(e); }
                        break; }
            case 'F': e.action = swr::FILL; e.side = side; e.size = r.sz; D.onEvent(e); break;
            case 'R': db.clear(); D.clearBook(); break;
            default: D.advance(ms); break;
            }
            if (!D.decided.empty()) collect(ct);
        }
    }
    std::fclose(in);
    D.advance(D.nowMs + 10000); collect(lastCt);
    std::printf("tag,day_ct,sec,bar_t,side,prices,lots,pre,post,refill,refilled,gap_ok,ref_kind,ref,ref_px,ttr_ms,atr,first,ref_t,ref_back,show,peak\n");
    for (size_t i = 0; i < outs.size(); ++i) {
        const swr::Event& e = outs[i].e;
        std::printf("%s,%lld,%lld,%lld,%s,%d,%lld,%lld,%lld,%.1f,%d,%d,%d,%s,%.2f,%lld,%.2f,%d,%lld,%d,%s,%.1f\n", tag.c_str(), (e.sec + 7 * 3600) / 86400, e.sec, e.barT,
                    swr::bullish(e) ? "bull" : "bear", e.prices, e.lots, e.pre, e.post, e.refill, e.refilled ? 1 : 0, e.gapOk ? 1 : 0, e.refKind, e.ref.c_str(), (double)e.refPx * 0.25,
                    e.ttrMs, e.atrTicks * 0.25, outs[i].first, e.refT, e.refBack, swg::showText(e.show).c_str(), e.peak);
    }
    std::fprintf(stderr, "%s: %lld records, fills %lld, sweeps %lld, big %lld, took out %lld, candidates %lld, refilled %lld, drawn %lld, book resets %lld, unknown fills %lld\n",
                 tag.c_str(), nRec, D.fills, D.sweeps, D.bigSweeps, D.tookOut, D.candidates, D.refilledN, D.drawnN, D.bookResets, D.unknownFills);
    std::fprintf(stderr, "%s: restart checks %lld, failed %lld\n", tag.c_str(), restartChecks, restartFails);
    return restartFails ? 1 : 0;
}
