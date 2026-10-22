// replay_st.cpp -- lsStopRun 1.0.0 REPLAY on real Databento MBO (T = aggressor trades, F = resting fills), 3-min candles.
// usage: replay_st <csv> <levels> <tick> <market> <out_events.csv>   (csv: ms,sec_ct,act,side,px,sz,oid ; levels: NAME PX AVAIL_SEC)
// The SAME stl::Detector the plugin runs. Databento T rows carry no aggressor order id -> the "distinct aggressor ids" rule is not
// applied here (the plugin applies it when IRT names the aggressor).
#include "../StopRunLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>
#include <deque>

int main(int argc, char** argv) {
    if (argc < 6) { std::fprintf(stderr, "usage\n"); return 2; }
    const double tick = std::atof(argv[3]); const std::string mkt = argv[4];
    std::vector<std::pair<stl::Ref, long long> > lv;
    { std::ifstream f(argv[2]); std::string n; double px; long long t; while (f >> n >> px >> t) { stl::Ref r; r.name = n; stl::toTicks(px, tick, r.px); r.dir = stl::keyDir(n, r.rank); if (r.dir) lv.push_back(std::make_pair(r, t)); } }
    stl::Detector D; const stl::MarketDefaults md = stl::marketDefaults(mkt);
    D.P.nearTicks = md.nearTicks; D.P.bigLots = md.bigLots; D.P.seedLots = md.seedLots;
    if (argc > 6) D.P.windowMs = std::atoi(argv[6]);
    std::ifstream in(argv[1]); std::string line;
    const long long SPB = 180;
    long long formEnd = 0; stl::Bar forming; bool haveForm = false; std::deque<stl::Bar> closed;
    std::ofstream out(argv[5]);
    out << "key,fired_ms,fired_sec,bar_t,side,ref,ref_kind,ref_px,st_px,lots,fills,norm,ratio,minute,atr_ticks,outcome,bars,big_ids,exc_atr,reclaimed,decided_t,decided_close\n";
    std::map<std::string, double> atrAt; std::map<std::string, int> minAt;
    long long rows = 0, qualLots = 0; std::vector<long long> qual;
    auto flush = [&]() {
        for (size_t i = 0; i < D.fired.size(); ++i) { atrAt[D.fired[i].key] = D.atrTicks; minAt[D.fired[i].key] = D.minuteOfDay; }
        D.fired.clear();
        for (size_t i = 0; i < D.decided.size(); ++i) {
            const stl::Event& e = D.decided[i];
            out << e.key << "," << e.firedMs << "," << e.firedSec << "," << e.barT << "," << (e.side < 0 ? "down" : "up") << "," << e.ref << "," << e.refKind << ","
                << e.refPx * tick << "," << e.stPx * tick << "," << e.lots << "," << e.fills << "," << e.norm << "," << e.ratio << "," << minAt[e.key] << "," << atrAt[e.key] << ","
                << (e.outcome == stl::ABSORBED ? "absorbed" : "not") << "," << e.bars << "," << e.bigIds << "," << e.excAtr << "," << (e.reclaimed ? 1 : 0) << "," << e.decidedT << ","
                << (closed.empty() ? 0.0 : closed.back().c * tick) << "\n";
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
        D.tick(ms, sec);
        if (act == 'T') {
            const long long end = (sec / SPB + 1) * SPB;
            if (!haveForm || end > formEnd) {
                if (haveForm) {
                    D.tick(ms, sec + 2);                       // the burst in the old candle is over before the close is judged
                    D.barClose(forming); closed.push_back(forming); if (closed.size() > 20) closed.pop_front();
                    D.prior = forming; D.havePrior = true;
                    if (closed.size() >= 5) { D.haveX5 = true; D.x5hi = 0; D.x5lo = (long long)9e18; for (size_t k = closed.size() - 5; k < closed.size(); ++k) { D.x5hi = std::max(D.x5hi, closed[k].h); D.x5lo = std::min(D.x5lo, closed[k].l); } }
                    double s = 0; int n = 0; for (size_t k = closed.size() > 14 ? closed.size() - 14 : 0; k < closed.size(); ++k) { s += (double)(closed[k].h - closed[k].l); ++n; }
                    D.atrTicks = n ? s / n : 0;
                    flush();
                }
                forming = stl::Bar(); forming.t = end; forming.h = forming.l = forming.c = px; formEnd = end; haveForm = true;
                D.formingT = end;
                D.keys.clear(); for (size_t k = 0; k < lv.size(); ++k) if (sec >= lv[k].second) D.keys.push_back(lv[k].first);
            }
            forming.h = std::max(forming.h, px); forming.l = std::min(forming.l, px); forming.c = px;
            D.minuteOfDay = (int)(((sec % 86400) + 86400) % 86400 / 60);
            if (side == 'B' || side == 'A') D.aggFill(ms, sec, side == 'B' ? stl::UP : stl::DOWN, px, sz, 0);
        } else if (act == 'F') {
            if (side == 'B' || side == 'A') D.restFill(ms, side == 'B' ? stl::DOWN : stl::UP, px, sz, oid);
        }
        flush();
    }
    std::vector<long long> all; for (std::map<int, std::vector<long long> >::iterator it = D.slotLots.begin(); it != D.slotLots.end(); ++it) all.insert(all.end(), it->second.begin(), it->second.end());
    std::fprintf(stderr, "%s rows %lld bursts %lld qualifying %lld (median lots %lld) near-level %lld sized %lld runs %lld pending-at-end %zu\n", argv[1], rows, D.bursts, D.qualifying, stl::median(all), D.nearLevel, D.sized, D.runs, D.pending.size());
    (void)qualLots; (void)qual;
    return 0;
}
