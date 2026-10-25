#include <algorithm>
#include "RallyLogic.h"
#include "RallyLogic_100.h"
#include <fstream>
#include <iostream>
#include <cassert>
static long civ(int y,unsigned m,unsigned d){ y-=m<=2; const long era=(y>=0?y:y-399)/400; const unsigned yoe=(unsigned)(y-era*400); const unsigned doy=(153*(m+(m>2?-3:9))+2)/5+d-1; const unsigned doe=yoe*365+yoe/4-yoe/100+doy; return era*146097+(long)doe-719468; }
template<class B> std::vector<B> load(const char* f){ std::ifstream in(f); std::string l; std::getline(in,l); std::vector<B> v;
  while(std::getline(in,l)){ int y,mo,d,h,mi; double o,hi,lo,c,vo; if(sscanf(l.c_str(),"%d-%d-%d %d:%d|%lf|%lf|%lf|%lf|%lf",&y,&mo,&d,&h,&mi,&o,&hi,&lo,&c,&vo)!=10) continue;
    B b; b.days=civ(y,mo,d); b.endSec=h*3600+mi*60; b.o=o;b.h=hi;b.l=lo;b.c=c;b.v=vo; v.push_back(b);} return v; }
int main(int argc,char**argv){ int fails=0;
  for(int a=1;a<argc;++a){
    auto B=load<rly::Bar>(argv[a]); auto B0=load<rly0::Bar>(argv[a]);
    auto W=rly::run(B,180,510,900); auto W0=rly0::run(B0,180,510,900);
    if(W.size()!=W0.size()){ std::cout<<"FAIL wave count\n"; ++fails; continue; }
    int same=0,added=0,via=0;
    for(size_t k=0;k<W.size();++k){ const auto&w=W[k];const auto&w0=W0[k];
      if(w.ge!=w0.ge||w.dir!=w0.dir){++fails;std::cout<<"FAIL wave mismatch\n";continue;}
      if(w0.fired && !w.fired){++fails;std::cout<<"FAIL lost a fired wave\n";}
      if(w0.fired && w.via30){++fails;std::cout<<"FAIL via30 on a tagged wave\n";}
      if(w.via30 && !(w.c[2] && !w0.c[2])){++fails;std::cout<<"FAIL via30 flag\n";}
      for(int c=0;c<5;++c) if(c!=2 && w.c[c]!=w0.c[c]){++fails;std::cout<<"FAIL other condition changed\n";}
      if(w.fired&&w0.fired)++same; if(w.fired&&!w0.fired){++added; const auto&b=B[(size_t)w.ge]; printf("  NEW Rly %s day %ld bar end %02d:%02d %s %d bars %.1f ATR failed='%s'\n",argv[a],b.days,b.endSec/3600,b.endSec/60%60,w.dir>0?"up":"down",w.nb,w.ratio,rly::failed(w).c_str()); }
      if(w.via30)++via;
      // record round trip
      rly::Rec R; std::string ln=rly::line(w,1000+k,1,2,"t"); if(!rly::parseLine(ln,R)||R.via30!=(w.via30&&w.fired)){++fails;std::cout<<"FAIL record round trip\n";}
      if(w.fired){ auto L=rly::hover(R,"9:24 AM","a","b","c"); if(L.empty()){++fails;} else if(w.via30) printf("  hover: %s\n",L[0].c_str()); }
    }
    printf("%s: waves %zu, Rly before %d, kept %d, new %d (via 30-bar %d)\n",argv[a],W.size(),(int)std::count_if(W0.begin(),W0.end(),[](auto&x){return x.fired;}),same,added,via);
  }
  // old record line (no via30) still parses as not via30
  rly::Rec R; assert(rly::parseLine("RLY|10|1|2|1|1|4|10.0000|3.0000|3.33|1|1.0000|11.0000|5.0000|6.0000|-|1.24.0",R) && !R.via30 && R.fired);
  printf(fails?"FAILED %d\n":"ALL PASS\n",fails); return fails!=0; }
