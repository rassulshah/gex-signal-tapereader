#include "../PressRead.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cassert>
int main(){
  std::vector<prd::Cdl> C; std::vector<long long> K; std::vector<std::string> L;
  std::ifstream f("cdl.txt"); std::string line;
  while(std::getline(f,line)){ std::istringstream s(line); prd::Cdl c; long long key; std::string lab; int bk;
    s>>key>>lab>>c.o>>c.h>>c.l>>c.c>>c.buy>>c.sell>>bk>>c.nb>>c.na>>c.iceB>>c.iceA; c.ok=true; c.book=bk; C.push_back(c); K.push_back(key); L.push_back(lab); }
  std::vector<prd::KeyRun> R; { std::ifstream g("runs.txt"); while(std::getline(g,line)){ std::istringstream s(line); prd::KeyRun r; s>>r.end>>r.lvl>>r.px>>r.side>>r.x>>r.vol>>r.ovol>>r.reach; R.push_back(r);} }
  std::map<long long,std::pair<double,double>> BU; { std::ifstream g("burst.txt"); while(std::getline(g,line)){ std::istringstream s(line); long long e; double u,d; s>>e>>u>>d; BU[e]={u,d}; } }
  auto burst=[&](long long e,int side)->double{ auto it=BU.find(e); if(it==BU.end()) return -1; return side<0?it->second.second:it->second.first; };
  auto clock=[&](int k){ return L[(size_t)k]+std::string(L[(size_t)k].size()<=5 && std::stoi(L[(size_t)k])>=7 && std::stoi(L[(size_t)k])<12 ? " AM" : " PM"); };
  int n=0;
  for(size_t i=0;i<C.size();++i){ if(i+1<C.size() && false) {}
    const std::string& t=L[i]; bool want = (K[i]>=1795770000-1800) && (t=="10:18"||t=="11:57"||t=="8:33"||t=="9:09"||t=="11:51"||t=="11:54"||t=="10:36");
    if(!want || K[i]<1795766000) continue;
    prd::Metrics m=prd::metrics(C,(int)i); prd::Read r=prd::read(m); prd::Ctx cx=prd::context(C,K,(int)i,R,burst,clock);
    std::cout<<t<<"\n  "<<(cx.ok?"Context: "+cx.text:"(no context)")<<"\n  Cause: "<<r.cause<<"  =>  Effect: "<<r.effect<<" ["<<r.cc<<","<<r.ec<<"]\n";
    for(auto& row: prd::rows(m)) std::cout<<"    | "<<row.t<<"\n";
    ++n; }
  std::cout<<n<<" samples\n";
}
