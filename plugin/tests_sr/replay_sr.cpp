// replay_sr.cpp -- lsSweepRefill 1.0.0 replay on real CME MBO through the SAME SweepRefillLogic.h the plugin runs.
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
    if (argc < 4) { std::fprintf(stderr, "usage: replay_sr <bin> <levels> <tag> [minPrices] [minLots] [refillPct] [refillMs] [gap]\n"); return 2; }
    swr::Detector D;
    if (argc > 4) D.P.minPrices = std::atoi(argv[4]);
    if (argc > 5) D.P.minLots = std::atoi(argv[5]);
    if (argc > 6) D.P.refillPct = std::atof(argv[6]);
    if (argc > 7) D.P.refillMs = std::atoi(argv[7]);
    if (argc > 8) D.P.gapBars = std::atoi(argv[8]);
    const std::string tag = argv[3];
    // levels
    std::vector<swr::Level> keys; std::vector<Snap> mq;
    { std::ifstream f(argv[2]); std::string ln;
      while (std::getline(f, ln)) {
          std::istringstream s(ln); std::string k; s >> k;
          if (k == "K") { swr::Level L; double px; s >> L.name >> px; L.px = (long long)llround(px / 0.25); L.kind = swr::REF_KEY; L.dir = swr::keyDir(L.name); if (L.dir) keys.push_back(L); }
          else if (k == "M") { long long ct; std::string n; double px; s >> ct >> n >> px; if (mq.empty() || mq.back().ct != ct) { Snap z; z.ct = ct; mq.push_back(z); }
              swr::Level L; L.name = n; L.px = (long long)llround(px / 0.25); L.kind = swr::REF_MQ; L.dir = 0; mq.back().lv.push_back(L); }
      } }
    size_t mqAt = 0; long long mqCur = -1;
    auto setLevels = [&](long long ct) {
        while (mqAt < mq.size() && mq[mqAt].ct <= ct) { mqCur = (long long)mqAt; ++mqAt; }
        D.levels = keys; if (mqCur >= 0) D.levels.insert(D.levels.end(), mq[(size_t)mqCur].lv.begin(), mq[(size_t)mqCur].lv.end());
    };
    // book of the Databento side (to turn C into MOD / DEL)
    std::unordered_map<unsigned long long, std::pair<long long, long long> > db;   // oid -> (px, size)
    std::FILE* in = std::fopen(argv[1], "rb"); if (!in) { std::perror("open"); return 2; }
    std::vector<Rec> buf(1 << 16);
    long long barEnd = 0, bh = 0, bl = 0, bc = 0, lastCt = 0; bool haveBar = false; long long nRec = 0;
    std::vector<Out> outs; std::vector<size_t> open;   // decided candidates still waiting for their 1 ATR outcome
    long long lastLevelsCt = -1;
    auto sessionOf = [](long long ct) { return (ct + 7 * 3600) / 86400; };
    auto collect = [&](long long ct) {
        for (size_t i = 0; i < D.decided.size(); ++i) {
            Out o; o.e = D.decided[i]; o.decidedCt = ct;
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
                    swr::Bar b; b.t = barEnd; b.h = bh; b.l = bl; b.c = bc; b.session = sessionOf(barEnd - 1); D.advance(ms); collect(ct); D.barClose(b); haveBar = false;
                }
                if (!haveBar) { haveBar = true; barEnd = end; bh = bl = bc = r.px; }
                bh = std::max(bh, (long long)r.px); bl = std::min(bl, (long long)r.px); bc = r.px;
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
    std::printf("tag,day_ct,sec,bar_t,side,prices,lots,pre,post,refill,refilled,gap_ok,ref_kind,ref,ref_px,ttr_ms,atr,first\n");
    for (size_t i = 0; i < outs.size(); ++i) {
        const swr::Event& e = outs[i].e;
        std::printf("%s,%lld,%lld,%lld,%s,%d,%lld,%lld,%lld,%.1f,%d,%d,%d,%s,%.2f,%lld,%.2f,%d\n", tag.c_str(), (e.sec + 7 * 3600) / 86400, e.sec, e.barT,
                    swr::bullish(e) ? "bull" : "bear", e.prices, e.lots, e.pre, e.post, e.refill, e.refilled ? 1 : 0, e.gapOk ? 1 : 0, e.refKind, e.ref.c_str(), (double)e.refPx * 0.25,
                    e.ttrMs, e.atrTicks * 0.25, outs[i].first);
    }
    std::fprintf(stderr, "%s: %lld records, fills %lld, sweeps %lld, big %lld, at extreme %lld, took out %lld, candidates %lld, refilled %lld, drawn %lld, book resets %lld, unknown fills %lld\n",
                 tag.c_str(), nRec, D.fills, D.sweeps, D.bigSweeps, D.atExtreme, D.tookOut, D.candidates, D.refilledN, D.drawnN, D.bookResets, D.unknownFills);
    return 0;
}
