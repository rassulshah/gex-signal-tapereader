// run_study.cpp -- replays the LRA 3-min store (data/irt/bars/<MKT>/3m_*.csv.gz, unzipped to CSV) through cvdf::Feed (the plugin's
// own code) and writes every event plus every bar, for study.py. Build: g++ -std=c++17 -O2 -I ../plugin run_study.cpp -o run_study
// Run: ./run_study <MKT> <tick> <out_prefix> <bars.csv> [more csv ...]     (t_ct = bar START, Central; bought / sold / v)
#include "CvdFeed.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

int main(int argc, char** argv) {
    if (argc < 5) { std::cerr << "usage\n"; return 2; }
    const std::string mkt = argv[1]; const double tick = std::atof(argv[2]); const std::string outp = argv[3];
    cvdf::Feed f; if (!f.init(mkt, tick, 180)) { std::cerr << "bad market\n"; return 2; }
    std::ofstream bars((outp + "_bars.csv").c_str()), evs((outp + "_events.csv").c_str());
    bars << "raw,session,o,h,l,c,buy,sell,total,cvd,pct,tainted,range_ticks\n";
    evs << "raw,id,type,dir,state,label,reason,seq,warning_seq,level_id,level,trigger,invalidation,cvd,range_ticks,tol,inv_ticks\n";
    std::map<long long, cvdf::In> rows;
    for (int a = 4; a < argc; ++a) {
        std::ifstream in(argv[a]); std::string line; std::getline(in, line);
        while (std::getline(in, line)) {
            std::stringstream ss(line); std::string c[9]; for (int i = 0; i < 9; ++i) std::getline(ss, c[i], ',');
            int Y, M, D, h, m, s; if (std::sscanf(c[0].c_str(), "%d-%d-%d %d:%d:%d", &Y, &M, &D, &h, &m, &s) != 6) continue;
            long long st = (long long)svl::daysFromCivil(Y, M, D) * 86400LL + h * 3600 + m * 60 + s, en = st + 180;
            cvdf::In x; x.raw = en; x.days = (long)(en / 86400); x.endSec = (int)(en % 86400);
            x.o = std::atof(c[1].c_str()); x.h = std::atof(c[2].c_str()); x.l = std::atof(c[3].c_str()); x.c = std::atof(c[4].c_str());
            x.total = c[5].empty() ? NAN : std::atof(c[5].c_str()); x.buy = c[6].empty() ? NAN : std::atof(c[6].c_str()); x.sell = c[7].empty() ? NAN : std::atof(c[7].c_str());
            rows[en] = x;
        }
    }
    for (auto& kv : rows) {
        const cvdf::In& x = kv.second;
        std::vector<cvd::Event> ev = f.push(x);
        const cvdf::Out& o = f.out[x.raw];
        bars << x.raw << ',' << f.sess << ',' << x.o << ',' << x.h << ',' << x.l << ',' << x.c << ',' << x.buy << ',' << x.sell << ',' << x.total << ','
             << o.cvd << ',' << o.pct << ',' << o.tainted << ',' << f.rangeTicks << '\n';
        for (const cvd::Event& e : ev)
            evs << x.raw << ',' << e.id << ',' << cvd::name(e.type) << ',' << (e.direction == cvd::Direction::Bull ? "bull" : "bear") << ','
                << cvd::name(e.state) << ',' << e.label << ',' << e.reason << ',' << e.seq << ',' << e.warning_seq << ',' << e.level_id << ','
                << std::setprecision(10) << e.level << ',' << e.trigger << ',' << e.invalidation << ',' << e.cvd << ',' << f.rangeTicks << ','
                << f.cfg.level_tolerance_ticks << ',' << f.cfg.invalidate_ticks << '\n';
    }
    std::cerr << mkt << " bars " << rows.size() << " marks " << f.marks.size() << " bad " << f.badBars << " over " << f.overTotal << "\n";
    return 0;
}
