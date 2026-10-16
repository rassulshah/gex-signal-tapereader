#include "BigFlow.h"
#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>
int bad=0; void ck(bool c,const char*m){ if(!c){++bad; printf("FAIL %s\n",m);} }
int main(int argc,char**argv){
 // synthetic: one aggressor sweeping 3 prices = one order
 bfl::Mkt m; m.onFill(9,bfl::ASK,400,30,1000); m.onFill(9,bfl::ASK,399,20,1000); m.onFill(9,bfl::ASK,398,10,1000);
 m.onFill(10,bfl::BID,401,2,1000);
 m.step(1500); ck(m.open.size()==2,"still open before 2 s");
 m.step(3100); ck(m.open.empty()&&m.nAll==2,"completed after 2 s");
 ck(m.done.size()==1 && m.done[0].second.lots==60 && m.done[0].second.lo==398 && m.done[0].second.hi==400,"sweep = one 60-lot order");
 auto mk=m.marks(3100,50); ck(mk.size()==1 && mk[0].side==bfl::ASK && mk[0].px==399,"mark at vwap price");
 // iceberg: shows 5, filled 5, reloads 5 (x3), traded 20
 bfl::Ice ic; ic.onNew(77,bfl::BID,500,5);
 for(int k=0;k<3;++k){ ic.onFill(77,500,5,100+k); ic.onMod(77,bfl::BID,500,0,100+k); ic.onMod(77,bfl::BID,500,5,100+k); }
 ic.onFill(77,500,5,200);
 auto lv=ic.live(10); ck(lv.size()==1 && lv[0].second.traded==20 && lv[0].second.reloads==3,"iceberg live: 20 traded, 3 reloads");
 // size sync after a fill is not a reload
 bfl::Ice i2; i2.onNew(5,bfl::ASK,600,50); i2.onFill(5,600,10,1); i2.onMod(5,bfl::ASK,600,40,1); i2.onFill(5,600,40,2);
 ck(i2.live(1).empty(),"plain fills are not an iceberg");
 ic.end(77,300); ck(ic.live(1).empty() && ic.ended(400,10).size()==1,"ended iceberg becomes a mark");
 if(argc>1){ // replay: zcat file | test_bigflow replay
   bfl::Mkt M; bfl::Ice I; std::string ln; long long rows=0; std::getline(std::cin,ln);
   while(std::getline(std::cin,ln)){
     ++rows; std::stringstream ss(ln); std::string f[10]; for(int k=0;k<10&&std::getline(ss,f[k],'|');++k);
     int h=std::stoi(f[0].substr(11,2)),mi=std::stoi(f[0].substr(14,2)),se=std::stoi(f[0].substr(17,2)); long long t=(h*3600LL+mi*60+se)*1000;
     long long px=std::llround(std::stod(f[2])*4), sz=std::stoll(f[4]), id=std::stoll(f[5]), ag=std::stoll(f[6]); int side=f[7]=="B"?bfl::BID:bfl::ASK;
     if(f[1]=="FILL"){ M.onFill(ag,side,px,sz,t); I.onFill(id,px,sz,t); }
     else if(f[1]=="NEW") I.onNew(id,side,px,sz);
     else if(f[1]=="MOD") I.onMod(id,side,px,sz,t);
     else if(f[1]=="DEL") I.end(id,t);
     if(rows%2000==0){ M.step(t); I.step(t); }
   }
   M.step(1LL<<50);
   double mu,v[3]; bool ok=M.stats(mu,v);
   printf("replay rows %lld market orders %lld start %lld big %zu stats %d mu %.0f 1SD %.0f 2SD %.0f 3SD %.0f; reloads %lld icebergs ended(60m) %zu\n",
     rows,M.nAll,M.start(),M.big.size(),ok,mu,v[0],v[1],v[2],I.reloadsSeen,I.done.size());
 }
 printf("bad %d\n",bad); return bad; }
