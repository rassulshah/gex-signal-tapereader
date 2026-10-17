// lsBigBidAsk 1.14.x settings / defaults tests. Build (from tests_bigba): g++ -std=c++17 -fsanitize=address,undefined -I.. test_bigba_114.cpp -o t && ./t
#include "BigBidAskLogic.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
using namespace bba;
static int fails=0, n=0;
#define CHECK(c) do{ ++n; if(!(c)){ ++fails; std::cout<<"FAIL line "<<__LINE__<<": "#c<<"\n";} }while(0)
static bool same(const Settings& a,const Settings& b){ return formatSettings(a)==formatSettings(b); }
static std::vector<std::string> fields(const std::string& t){ std::vector<std::string> f; std::string c; for(char ch:t){ if(ch=='|'){f.push_back(c);c.clear();} else c+=ch;} f.push_back(c); return f; }
static std::string join(const std::vector<std::string>& f){ std::string o; for(size_t i=0;i<f.size();++i){ if(i) o+="|"; o+=f[i]; } return o; }
int main(){
  std::string why; Settings d;
  // ---- (1.14.0) defaults (Rassul 16:35 / 16:46)
  CHECK(d.div==0&&d.box&&!d.dim&&!d.circle&&d.boxBid==0x00EF4444&&d.boxAsk==0x0022C55E);
  for(int k=0;k<4;++k) CHECK(d.cat[k].on&&d.cat[k].font==8);
  CHECK(d.cat[0].n==5&&d.cat[1].n==10&&d.cat[2].n==20);
  CHECK(d.cat[0].bid==0x00FFFF&&d.cat[0].ask==0x00FFFF&&d.cat[3].bid==0x00FFFF&&d.cat[3].ask==0x00FFFF);
  CHECK(d.cat[1].bid==0xFFFFFF&&d.cat[1].ask==0xFFFFFF&&d.cat[2].bid==0xFFFFFF&&d.cat[2].ask==0xFFFFFF);
  CHECK(d.cat[0].show==SH_ALL&&d.cat[1].show==SH_PBBO&&d.cat[2].show==SH_PBBO&&d.cat[3].show==SH_ALL);
  CHECK(d.skipFirst==15&&d.skipLast==15&&!d.keyOn&&!d.gamOn&&!d.volOn&&d.keyMask==0&&d.gamMask==0&&d.volMask==0&&!d.rthOnly);
  struct M{const char*m;int l;} c4[]={{"ES",1000},{"NQ",450},{"CL",95},{"GC",105},{"HG",90},{"NG",295},{"6E",490},{"EU",490}};
  for(auto& x:c4){ CHECK(cat4For(x.m)==x.l); CHECK(defaultsFor(x.m).cat[3].lots==x.l); }
  // ---- format / parse
  const std::string base=formatSettings(d); auto bf=fields(base); CHECK(bf.size()==NFIELDS); CHECK(bf.size()==49);
  Settings r=parseSettings(base+"\r\n",why); CHECK(same(r,d)&&why.empty());
  Settings s=d; s.div=25; s.box=false; s.dim=true; s.circle=true; s.circleCol=0x123456; s.boxBid=0x010101; s.boxAsk=0xFEFEFE;
  for(int k=0;k<4;++k){ s.cat[k].on=k%2; s.cat[k].font=6+k*4; s.cat[k].bid=0x010203*k; s.cat[k].ask=0xFFFFFF-k; s.cat[k].show=k<3?k*2:0; }
  s.cat[0].n=7; s.cat[1].lots=40; s.cat[3].lots=2500;
  s.keyOn=true; s.gamOn=true; s.volOn=false; s.keyMask=0x0F0F; s.gamMask=0x3; s.volMask=0x21; s.statsChart=false; s.statsPos=1; s.rthOnly=true;
  r=parseSettings(formatSettings(s),why); CHECK(why.empty()); CHECK(same(r,s));
  for(int sh=0;sh<=6;++sh){ Settings q=d; q.cat[1].show=sh; r=parseSettings(formatSettings(q),why); CHECK(r.cat[1].show==sh&&why.empty()); }
  // garbage = the market's defaults
  for(std::string b:std::vector<std::string>{"","x","BBA7","BBA7|0","\x01\x02","BBA8|"+base.substr(5),"BBA6|"+base.substr(5)}) { r=parseSettings(b,why,"NQ"); CHECK(same(r,defaultsFor("NQ"))); CHECK(!why.empty()); }
  for(auto o:{"BBA1|7|200|1|0","BBA2|7|200|1|1|0|2278750|15680580","BBA3|8|3|0|0|0|0|16777215|1|2|3|4|5|6","BBA4|8|0|0|0|0|16777215|1|5|0|0|1|1|0|10|0|0|1|1|0|20|0|0|1|1"}){ r=parseSettings(o,why,"CL"); CHECK(same(r,defaultsFor("CL"))); CHECK(r.cat[3].lots==95); CHECK(why.find("older")!=std::string::npos); }
  // every field with junk: only that field falls back, everything stays in range
  const char* junk[]={"","-1","abc","1.5"," 3","99999999999999","100001","16777216","2","7","101","241","2001","65536","0x10","10"};
  int fieldFallbacks=0;
  for(size_t i=1;i<bf.size();++i) for(auto j:junk){ auto f=bf; f[i]=j; r=parseSettings(join(f),why,"ES");
     CHECK(r.div==0||(r.div>=1&&r.div<=1000)); CHECK(r.circleCol<=0xFFFFFF&&r.boxBid<=0xFFFFFF&&r.boxAsk<=0xFFFFFF);
     for(int k=0;k<4;++k){ const Cat& c=r.cat[k]; CHECK((k<3?(c.n>=1&&c.n<=100):c.n==0)&&c.font>=6&&c.font<=20&&c.bid<=0xFFFFFF&&c.ask<=0xFFFFFF&&(k<3?(c.lots>=0&&c.lots<=100000):(c.lots>=1&&c.lots<=100000))&&c.show>=0&&c.show<=6&&(k<3||c.show==0)); }
     CHECK(r.skipFirst>=0&&r.skipFirst<=240&&r.skipLast>=0&&r.skipLast<=240&&r.atrUpQ>=0&&r.atrUpQ<=2000&&r.atrDnQ>=0&&r.atrDnQ<=2000);
     CHECK(r.keyMask<(1u<<16)&&r.gamMask<(1u<<10)&&r.volMask<(1u<<6)&&r.statsPos>=1&&r.statsPos<=9);
     if(!why.empty()) ++fieldFallbacks; }
  CHECK(fieldFallbacks>0);
  { auto f=bf; f[8+1]="0"; r=parseSettings(join(f),why); CHECK(r.cat[0].n==5&&why.find("Big Lot")!=std::string::npos); }
  { auto f=bf; f[8+7+4]="7"; r=parseSettings(join(f),why); CHECK(r.cat[1].show==SH_PBBO&&why.find("show")!=std::string::npos); }
  { auto f=bf; f[8+21+4]="3"; r=parseSettings(join(f),why); CHECK(r.cat[3].show==0); }   // category 4 never filters
  { auto f=bf; f[8+21+2]="0"; r=parseSettings(join(f),why,"GC"); CHECK(r.cat[3].lots==105&&why.find("category 4 lots")!=std::string::npos); }
  { auto f=bf; f[48]="2"; r=parseSettings(join(f),why); CHECK(!r.rthOnly&&why.find("RTH")!=std::string::npos); }
  // ---- carried over: a 1.13 (BBA6) file and a 1.12 (BBA5) file
  { // BBA6: cat1 pivot only, cat2 level + setup Pullback, cat3 pivot+level, cat4 on 2500 with pivot (ignored); colours kept as TEXT colours
    std::string b6="BBA6|25|1|0|1|1193046|1|7|0|9|1|0|15680580|2278750|0|1|10|0|8|0|1|111|222|1|1|20|0|8|1|1|15680580|2278750|0|1|0|2500|8|1|0|15680580|2278750|3|15|5|250|50|1|1|1|65535|1023|63|0|3|1";
    CHECK(fields(b6).size()==55);
    r=parseSettings(b6,why,"ES"); CHECK(why.find("carried over")!=std::string::npos); CHECK(why.find("bad")==std::string::npos);
    CHECK(r.div==25&&r.box&&r.circle&&r.circleCol==1193046&&r.boxBid==0x00EF4444&&r.boxAsk==0x0022C55E);
    CHECK(r.cat[0].on&&r.cat[0].n==7&&r.cat[0].font==9&&r.cat[0].show==SH_PIVOT&&r.cat[0].bid==15680580);
    CHECK(r.cat[1].show==SH_PB&&r.cat[1].bid==111&&r.cat[1].ask==222);
    CHECK(r.cat[2].show==SH_PIVLEVEL);
    CHECK(r.cat[3].on&&r.cat[3].lots==2500&&r.cat[3].show==0);
    CHECK(r.skipFirst==15&&r.skipLast==5&&r.atrUpQ==250&&r.keyMask==65535&&!r.statsChart&&r.statsPos==3&&r.rthOnly);
    auto f=fields(b6); f[14]="3"; r=parseSettings(join(f),why); CHECK(r.cat[0].show==SH_BO);
    f[14]="2"; r=parseSettings(join(f),why); CHECK(r.cat[0].show==SH_PIVLEVEL);   // reversal at a level + pivot
    f[10]="0"; r=parseSettings(join(f),why); CHECK(r.cat[0].show==SH_LEVEL);
    f[14]="0"; r=parseSettings(join(f),why); CHECK(r.cat[0].show==SH_ALL); }
  { std::string b5="BBA5|25|1|0|1|1193046|1|7|0|9|1|0|15680580|2278750|0|10|0|8|0|0|15680580|2278750|0|20|0|8|0|0|15680580|2278750|1|0|2500|8|0|0|15680580|2278750|15|0|250|50|1|1|1|65535|1023|63|0|3";
    r=parseSettings(b5,why); CHECK(why.find("carried")!=std::string::npos); CHECK(r.div==25&&r.box&&r.circle&&r.cat[0].n==7&&r.cat[0].font==9&&r.cat[0].show==SH_PIVOT&&r.cat[3].on&&r.cat[3].lots==2500&&r.skipFirst==15&&r.atrUpQ==250&&!r.statsChart&&r.statsPos==3&&!r.rthOnly); }
  // breakout through the higher of the last 2 pivots
  { // tops at 105 (bar 2) then 103 (bar 6); bar 10 crosses 105 -> 2 (the higher one 2 back); bar 9 crossing only 103 -> no (103 < 105 means P = 105)
    std::vector<double> H={100,103,105,102,101,102,103,101,100,104,106,104}, L={ 99,101,103,100, 99,100,101, 99, 98,101,104,102};
    std::vector<signed char> up,dn; breakFlags(H,L,(int)H.size(),up,dn);
    CHECK(up[9]==0); CHECK(up[10]==2);
    // only one top so far: bar crossing it -> 1
    std::vector<double> H2={100,103,105,102,101,106}, L2={99,101,103,100,99,104}; breakFlags(H2,L2,6,up,dn); CHECK(up[5]==1);
    // a candle entirely above the pivot (gapped) is not the breakout candle
    std::vector<double> H3={100,103,105,102,101,108}, L3={99,101,103,100,99,106}; breakFlags(H3,L3,6,up,dn); CHECK(up[5]==0);
    // breakdown mirror: bots 95 (b2) then 97 (b6) -> P = 95 (lower 2 back)
    std::vector<double> H4={100,98,96,99,100,99,98,100,101,100,96}, L4={98,96,95,97,98,97,96.9,98,99,97,94}; breakFlags(H4,L4,(int)H4.size(),up,dn);
    CHECK(dn[10]==2); std::vector<double> H5=H4, L5=L4; L5[10]=96; H5[10]=98; breakFlags(H5,L5,11,up,dn); CHECK(dn[10]==0); }
  // decimals for the ATR boxes
  CHECK(toHundredths(2.0)==200&&toHundredths(0.5)==50&&toHundredths(2.5)==250&&toHundredths(1.25)==125&&toHundredths(0)==0&&toHundredths(20)==2000);
  CHECK(toHundredths(-0.1)==-1&&toHundredths(20.5)==-1&&toHundredths(NAN)==-1);
  // ---- the divider: market defaults and his own
  struct D{const char*m; int def;} mk[]={{"ES",10},{"CL",10},{"NG",1},{"EU",1},{"NQ",1},{"GC",1},{"HG",1}};
  for(auto& x:mk) CHECK(divFor(x.m)==x.def);
  CHECK(shown(1318,10)=="132"); CHECK(shown(1315,10)=="132"); CHECK(shown(1314,10)=="131"); CHECK(shown(3599,10)=="360"); CHECK(shown(4,10)=="1"); CHECK(shown(5,10)=="1");
  CHECK(shown(92,1)=="92"); CHECK(shown(1318,1)=="1,318"||shown(1318,1)=="1318"); CHECK(shown(1318,25)=="53"); CHECK(shown(1318,1000)=="1");
  std::cout<<"divider samples: ES 1318->"<<shown(1318,divFor("ES"))<<"  NQ 92->"<<shown(92,divFor("NQ"))<<"  CL 455->"<<shown(455,divFor("CL"))<<"  GC 43->"<<shown(43,divFor("GC"))<<"  HG 23->"<<shown(23,divFor("HG"))<<"  NG 96->"<<shown(96,divFor("NG"))<<"  6E 124->"<<shown(124,divFor("EU"))<<"\n";
  // ---- near a level + sweeps
  { std::vector<double> H={100,101,102,103,104,106,105,104}, L={99,100,101,102,103,104,103,102}, C={100,101,102,103,104,104.5,103.5,102.5};
    std::vector<NamedLevel> lv={{"PDH",105.0}};
    // candle 5 high 106 swept PDH 105, closes back below (104.5): a print at 105.75 is a sweep print
    CHECK(nearLevel(H,L,C,5,105.75,lv,0.5,200,100)!=nullptr);
    // with ATR 0.5: zone 105 - 0.5 .. 105 + 1.0 -> 103.0 is not near, and no sweep below
    CHECK(nearLevel(H,L,C,5,103.0,lv,0.5,200,100)==nullptr);
    CHECK(nearLevel(H,L,C,5,104.6,lv,0.5,200,100)!=nullptr);   // inside 1 ATR below
    // breakout that holds: 3 closes above the level -> the sweep stops counting
    std::vector<double> H2={104,106,107,108,109}, L2={103,104,105.5,106,107}, C2={104,105.5,106.5,107.5,108};
    CHECK(nearLevel(H2,L2,C2,4,108.5,lv,0.5,200,100)==nullptr);
    CHECK(sweptHere(H2,L2,C2,1,105.0,105.6)); }
  // ---- value area
  { std::map<long long,long long> p={{1,10},{2,20},{3,50},{4,15},{5,5}}; VA v=valueArea(p); CHECK(v.ok&&v.poc==3&&v.val<=3&&v.vah>=3);
    long long in=0; for(auto& kv:p) if(kv.first>=v.val&&kv.first<=v.vah) in+=kv.second; CHECK(in>=70); CHECK(!valueArea({}).ok); }
  // ---- statistics: nth and the outlier drop
  CHECK(nth({5,9,1,7,3},2)==7); CHECK(nth({5},2)==0);
  { Avg a=avgDropOutlier({1000,1050,980,1020,2100}); CHECK(a.dropped==4&&a.used==4&&a.avg==1013); }
  { Avg a=avgDropOutlier({1000,1050,980,1020,1100}); CHECK(a.dropped==-1&&a.used==5); }
  { Avg a=avgDropOutlier({1000,1050,0,400}); CHECK(a.used>=2); }
  { Avg a=avgDropOutlier({}); CHECK(a.used==0&&a.avg==0); }
  // ---- gamma file
  { std::string f="SYMBOL,PRICE,LABEL\nEPZ26,7795.000000,CR 0D / GW 0D,1\nEPZ26,7700,PS 0D,1\nEPZ26,7780,HVL 0D,1\nEPZ26,7750,G1,1\nEPZ26,7725,G2,1\nENQZ26,20000,CR 0D,1\nEPH27,7900,CR,1\ngarbage\n";
    auto g=parseGamma(f,"ES","ESZ6",0.25); CHECK(g.size()==5);
    bool cr=false; for(auto& x:g) if(x.name=="CR 0DTE"&&x.px==7795) cr=true; CHECK(cr);
    CHECK(parseGamma(f,"NQ","NQZ6",0.25).size()==1); CHECK(parseGamma(f,"ES","ESH7",0.25).size()==1); }
  std::cout<<n-fails<<"/"<<n<<" passed\n"; return fails?1:0; }
