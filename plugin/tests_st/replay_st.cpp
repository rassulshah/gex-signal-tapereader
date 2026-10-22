// replay_st.cpp -- lsStopRun 1.2.0 REPLAY on real Databento MBO (T = aggressor trades, F = resting fills), 3-min candles.
// usage: replay_st <csv> <levels> <tick> <market> <out_events.csv> [LRA-StopSweep-<MKT>.csv]
//   csv: ms,sec_ct,act,side,px,sz,oid ; levels: the replay_sr format ("K <name> <px>" session levels, "M <ct_sec> <code> <px>" MenthorQ
//   snapshots, the latest at or before the moment) or the 1.1 format "NAME PX AVAIL_SEC".
// The SAME stl::Detector + SwingRef.h the plugin runs (ZigZag from the RTH open, volume profile from the T prints). Databento T rows
// carry no aggressor order id -> the "distinct aggressor ids" rule is not applied here (the plugin applies it when IRT names it).
#include "../StopRunLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>
#include <deque>

int main(int argc, char** argv) {
    if (argc < 6) { std::fprintf(stderr, "usage: replay_st <csv> <levels> <tick> <market> <out.csv> [params.csv]\n"); return 2; }
    const double tick = std::atof(argv[3]); const std::string mkt = argv[4];
    std::vector<std::pair<swg::Lvl, long long> > keys; std::vector<std::pair<long long, std::vector<swg::Lvl> > > mq;
    { std::ifstream f(argv[2]); std::string ln;
      while (std::getline(f, ln)) {
          std::istringstream s(ln); std::string a; s >> a; if (a.empty()) continue;
          int rank = 0;
          if (a == "K") { std::string n; double px; s >> n >> px; swg::Lvl L; L.name = n; const int d = stl::keyDir(n, rank); if (d == 0 || d == 2 || !stl::toTicks(px, tick, L.px)) continue; L.dir = d; L.kind = swg::KEY; keys.push_back(std::make_pair(L, 0LL)); }
          else if (a == "M") { long long ct; std::string n; double px; s >> ct >> n >> px; swg::Lvl L; L.name = n; L.kind = swg::MQ; L.dir = 0; if (!stl::toTicks(px, tick, L.px)) { const double t = px / tick; L.px = (long long)llround(t); }
              if (mq.empty() || mq.back().first != ct) { mq.push_back(std::make_pair(ct, std::vector<swg::Lvl>())); }
              mq.back().second.push_back(L); }
          else { double px; long long t; s >> px >> t; swg::Lvl L; L.name = a; const int d = stl::keyDir(a, rank); if (d == 0 || !stl::toTicks(px, tick, L.px)) continue; L.dir = d == 2 ? 0 : d; L.kind = d == 2 ? swg::MQ : swg::KEY; keys.push_back(std::make_pair(L, t)); }
      } }
    stl::Detector D; const stl::MarketDefaults md = stl::marketDefaults(mkt);
    swg::StopSweep X; X.stNearTicks = md.nearTicks; X.staBigLots = md.bigLots;
    if (argc > 6) { std::ifstream f(argv[6]); std::vector<std::string> L; std::string l; while (std::getline(f, l)) L.push_back(l); X = swg::parseStopSweep(L, mkt); std::fprintf(stderr, "params %s: %s\n", argv[6], X.why.c_str()); }
    stl::applyParams(D.P, X); D.P.seedLots = md.seedLots;
    int oMin = 510, cMin = 900; swg::rthOf(mkt, oMin, cMin); D.setRth(oMin, cMin, 180);
    std::ifstream in(argv[1]); std::string line;
    const long long SPB = 180;
    long long formEnd = 0; stl::Bar forming; bool haveForm = false; std::map<long long, double> formRows;
    std::ofstream out(argv[5]);
    out << "key,fired_ms,fired_sec,bar_t,side,ref,ref_kind,ref_px,st_px,lots,fills,norm,ratio,ref_t,ref_back,outcome,big_ids,big_max,reclaimed,decided_sec,show_fire,show\n";
    std::map<std::string, stl::Event> fired;
    long long rows = 0; size_t mqAt = 0; long long mqCur = -1;
    auto flush = [&]() {
        for (size_t i = 0; i < D.fired.size(); ++i) fired[D.fired[i].key] = D.fired[i];
        D.fired.clear();
        for (size_t i = 0; i < D.decided.size(); ++i) {
            const stl::Event& e = D.decided[i];
            out << e.key << "," << e.firedMs << "," << e.firedSec << "," << e.barT << "," << (e.side < 0 ? "down" : "up") << "," << e.ref << "," << stl::kindText(e.refKind) << ","
                << e.refPx * tick << "," << e.stPx * tick << "," << e.lots << "," << e.fills << "," << e.norm << "," << e.ratio << "," << e.refT << "," << e.refBack << ","
                << (e.outcome == stl::ABSORBED ? "absorbed" : "not") << "," << e.bigIds << "," << e.bigLotsMax << "," << (e.reclaimed ? 1 : 0) << "," << e.decidedSec << ","
                << swg::showText(e.showFire) << "," << swg::showText(e.show) << "\n";
        }
        D.decided.clear();
    };
    while (std::getline(in, line)) {
        ++rows;
        std::stringstream ss(line); std::string f[7]; for (int k = 0; k < 7; ++k) std::getline(ss, f[k], ',');
        const long long ms = std::atoll(f[0].c_str()), sec = std::atoll(f[1].c_str());
        const char act = f[2].empty() ? '?' : f[2][0], side = f[3].empty() ? '?' : f[3][0];
        long long px = 0; if (!stl::toTicks(std::atof(f[4].c_str()), tick, px)) continue;
        const long long sz = std::atoll(f[5].c_str()), oid = std::atoll(f[6].c_str());
        D.tick(ms, sec); flush();
        if (act == 'T') {
            const long long end = (sec / SPB + 1) * SPB;
            if (!haveForm || end > formEnd) {
                if (haveForm) {
                    std::vector<std::pair<long long, double> > r(formRows.begin(), formRows.end()); formRows.clear();
                    D.barClose(forming, &r, (forming.t - 1 + 7 * 3600) / 86400); flush();
                }
                forming = stl::Bar(); forming.t = end; forming.o = forming.h = forming.l = forming.c = px; forming.day = end / 86400;
                forming.rth = swg::isRth((int)(end % 86400), (int)SPB, oMin, cMin); formEnd = end; haveForm = true;
                D.formingT = end;
                while (mqAt < mq.size() && mq[mqAt].first <= sec) { mqCur = (long long)mqAt; ++mqAt; }
                D.levels.clear(); for (size_t k = 0; k < keys.size(); ++k) if (sec >= keys[k].second) D.levels.push_back(keys[k].first);
                if (mqCur >= 0) D.levels.insert(D.levels.end(), mq[(size_t)mqCur].second.begin(), mq[(size_t)mqCur].second.end());
            }
            forming.h = std::max(forming.h, px); forming.l = std::min(forming.l, px); forming.c = px; formRows[px] += (double)sz;
            D.minuteOfDay = (int)(((sec % 86400) + 86400) % 86400 / 60);
            if (side == 'B' || side == 'A') D.aggFill(ms, sec, side == 'B' ? stl::UP : stl::DOWN, px, sz, 0);
        } else if (act == 'F') {
            if (side == 'B' || side == 'A') D.restFill(ms, side == 'B' ? stl::DOWN : stl::UP, px, sz, oid);
        }
        flush();
    }
    std::fprintf(stderr, "%s rows %lld bursts %lld qualifying %lld near-level %lld sized %lld runs %lld pending-at-end %zu\n", argv[1], rows, D.bursts, D.qualifying, D.nearLevel, D.sized, D.runs, D.pending.size());
    return 0;
}
