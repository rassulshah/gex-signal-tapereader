#include "CvdEngine.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> out; std::stringstream in(s); std::string v;
    while(std::getline(in,v,',')) out.push_back(v);
    if(!s.empty() && s.back()==',') out.push_back("");
    return out;
}
int main(int argc,char** argv) {
    if(argc<2||argc>3) { std::cerr<<"Usage: cvd_replay bars.csv [tick_size]\n"; return 2; }
    try {
        cvd::Config config; if(argc==3) config.tick_size=std::stod(argv[2]);
        cvd::Engine engine(config); std::ifstream in(argv[1]);
        if(!in) throw std::runtime_error("cannot open input");
        std::string line; std::getline(in,line);
        const std::string header="seq,timestamp,anchor,open,high,low,close,buy,sell,total,seconds,complete,quality,level_id,level_price,level_support,level_available_seq";
        if(!line.empty()&&line.back()=='\r') line.pop_back();
        if(line!=header) throw std::runtime_error("unexpected CSV header; see examples/bars.csv");
        std::cout<<"event_id,seq,timestamp,anchor,type,direction,state,label,warning_seq,source_seq,cvd,level_id,level,trigger,invalidation,plot,reason\n";
        while(std::getline(in,line)) {
            if(!line.empty()&&line.back()=='\r') line.pop_back();
            if(line.empty()) continue;
            auto v=split(line); if(v.size()!=17) throw std::runtime_error("expected 17 columns");
            cvd::Bar b; b.seq=std::stoll(v[0]); b.timestamp=v[1]; b.anchor=v[2];
            b.open=std::stod(v[3]); b.high=std::stod(v[4]); b.low=std::stod(v[5]); b.close=std::stod(v[6]);
            b.buy=std::stod(v[7]); b.sell=std::stod(v[8]); b.total=std::stod(v[9]); b.seconds=std::stod(v[10]);
            if((v[11]!="0"&&v[11]!="1")||(v[12]!="0"&&v[12]!="1")) throw std::runtime_error("booleans must be 0/1");
            b.complete=v[11]=="1"; b.quality=v[12]=="1";
            if(!v[13].empty()) {
                if(v[15]!="0"&&v[15]!="1") throw std::runtime_error("level_support must be 0/1");
                b.levels.push_back({v[13],std::stod(v[14]),v[15]=="1",std::stoll(v[16])});
            }
            auto out=engine.push(b);
            for(const auto& e:out.events) {
                std::cout<<std::setprecision(12)<<e.id<<','<<e.seq<<','<<e.timestamp<<','<<e.anchor<<','<<cvd::name(e.type)
                    <<','<<(e.direction==cvd::Direction::Bull?"bull":"bear")<<','<<cvd::name(e.state)<<','<<e.label
                    <<','<<e.warning_seq<<','<<e.source_seq<<','<<e.cvd<<','<<e.level_id<<','<<e.level<<','<<e.trigger
                    <<','<<e.invalidation<<','<<(e.plot?1:0)<<','<<e.reason<<'\n';
            }
        }
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
