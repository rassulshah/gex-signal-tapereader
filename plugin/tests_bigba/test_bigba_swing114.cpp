// lsBigBidAsk 1.14.x swing pivot, waves + Trend PB band rule tests. Build (from tests_bigba): g++ -std=c++17 -fsanitize=address,undefined -I.. test_bigba_swing114.cpp -o t && ./t
#include "BigBidAskLogic.h"
#include <iostream>
#include <random>
using namespace bba;
static int fails=0,n=0;
#define CHECK(c) do{ ++n; if(!(c)){ ++fails; std::cout<<"FAIL line "<<__LINE__<<": "#c<<"\n";} }while(0)
int main(){
  std::vector<double> H={10,11,12,13,12.5,11,10,10.5,11.5,12}, L={9,10,11,12,11.5,10,9,9.5,10.5,11};
  Swing s=swing(H,L,10);
  CHECK(s.lastConfirmed==6);
  int bull[10]={0,0,0,0,0,0,1,1,0,0}, bear[10]={0,0,0,1,1,0,0,0,1,1}, conf[10]={1,1,1,1,1,1,1,0,0,0};
  for(int i=0;i<10;++i){ CHECK(s.bull[i]==bull[i]); CHECK(s.bear[i]==bear[i]); CHECK(s.conf[i]==conf[i]); }
  CHECK(s.dp[5]==6&&s.up[5]==3&&s.dp[8]==6&&s.up[8]==9);
  // the down pivot is decided by a LATER bar: bar 5 is judged against bar 6 (repaint allowed), the non-repainting rule could not
  { std::vector<double> H2=H, L2=L; H2.resize(6); L2.resize(6); Swing a=swing(H2,L2,6);   // bars 0..5 only: bar 5 is the low so far
    CHECK(a.bull[5]==1&&a.conf[5]==0); }                                              // shown while it is the low ...
  CHECK(s.bull[5]==0&&s.conf[5]==1);                                                  // ... gone once bar 6 made a lower low (final)
  // the forming top at 9 confirmed by bar 10
  { std::vector<double> H3=H, L3=L; H3.push_back(11); L3.push_back(10); Swing a=swing(H3,L3,11);
    CHECK(a.lastConfirmed==9&&a.conf[9]==1&&a.conf[7]==1&&a.conf[10]==0&&a.bear[9]==1&&a.dp[10]==10&&a.bull[10]==1); }
  // bars after the extreme without a turn yet (lower high, low not lower): a tentative leg down to them
  { std::vector<double> H4=H, L4=L; H4.push_back(11.8); L4.push_back(11.2); Swing a=swing(H4,L4,11);
    CHECK(a.lastConfirmed==6&&a.dp[10]==10&&a.up[10]==9&&a.bull[10]==1&&a.conf[10]==0&&a.bear[10]==1); }
  // ---- the Trend PB band rule
  std::vector<double> vw(10,10.5), sd(10,1.0);
  CHECK(pbBands(H,L,vw,sd,s,7,1)==0);                   // bar 6 low 9 <= VWAP - 1SD (9.5): too deep
  sd.assign(10,2.0); CHECK(pbBands(H,L,vw,sd,s,7,1)==1); // -1SD 8.5 not reached, VWAP 10.5 touched
  CHECK(pbBands(H,L,vw,sd,s,7,2)==1);                    // strong: +1SD 12.5 touched too
  vw.assign(10,8.0); sd.assign(10,1.0);
  CHECK(pbBands(H,L,vw,sd,s,7,1)==1);                   // (16:56) plain uptrend: +1SD 9 touched also counts
  CHECK(pbBands(H,L,vw,sd,s,7,2)==1);                   // strong: touched +1SD 9 (bar 6 low 9)
  CHECK(pbBands(H,L,vw,sd,s,7,0)==0);                   // no trend: never
  // downtrend: the leg rising into the up pivot 3 (bars 0..3)
  vw.assign(10,12.0); sd.assign(10,0.5); CHECK(pbBands(H,L,vw,sd,s,4,-1)==0);   // bar 3 high 13 >= +1SD 12.5
  sd.assign(10,2.0); CHECK(pbBands(H,L,vw,sd,s,4,-1)==1);                       // touched VWAP 12, never +1SD 14
  vw.assign(10,15.0); sd.assign(10,1.0); CHECK(pbBands(H,L,vw,sd,s,4,-1)==0); CHECK(pbBands(H,L,vw,sd,s,4,-2)==0);  // highs below -1SD 14: not touched
  vw.assign(10,13.5); sd.assign(10,0.5); CHECK(pbBands(H,L,vw,sd,s,4,-1)==1); CHECK(pbBands(H,L,vw,sd,s,4,-2)==1);   // -1SD 13 touched (VWAP 13.5 not): both count
  vw.assign(10,0.0); CHECK(pbBands(H,L,vw,sd,s,7,1)==0);  // no bands at all: not met
  // ---- (1.14.1) waves: bars 0..3 up leg, 4..6 down leg, 7..9 up (forming)
  for(int i=0;i<=3;++i) CHECK(upWave(s,i)&&!downWave(s,i));
  for(int i=4;i<=6;++i) CHECK(downWave(s,i)&&!upWave(s,i));
  for(int i=7;i<=9;++i) CHECK(upWave(s,i));
  // ---- (1.14.1) the band through the print's own candle: an outside bar AFTER the turn makes the bounce's real high (Oct 16 12:42)
  { // down leg into bottom 3, bounce 4..6 (top 6), bar 7 lower low (turn down), bar 8 outside bar: higher high AND lower low
    std::vector<double> h={20,18,16,14,15,16,17,16.5,18.5,15}, l={18,16,14,12,13,14,15,14.5,14.0,13};
    Swing w=swing(h,l,10); CHECK(w.up[8]==6 && downWave(w,8));
    std::vector<double> v(10,21.0), q(10,3.0);   // -1SD = 18: the bounce top (bar 6, 17) misses it, the outside bar 8 (18.5) touches it
    CHECK(pbBands(h,l,v,q,w,6,-2)==0);           // judged at the top: not touched yet
    CHECK(pbBands(h,l,v,q,w,8,-2)==1);           // the print on the outside bar: touched -1SD, never +1SD (24)
    std::vector<double> v2(10,15.0); CHECK(pbBands(h,l,v2,q,w,8,-2)==0);   // +1SD = 18: the outside bar (18.5) went to the far band: not a pullback
  }
  // ---- fuzz: random walks never crash, flags only 0/1, dp/up inside the bars, conf only on a prefix
  std::mt19937 rng(7);
  for(int t=0;t<3000;++t){ int m=1+rng()%300; std::vector<double> h(m),l(m); double p=100;
    for(int i=0;i<m;++i){ p+= (int)(rng()%9)-4; double a=p+(rng()%4), b=p-(rng()%4); if(rng()%50==0){a=NAN;} h[i]=a; l[i]=b; }
    Swing w=swing(h,l,m); bool tail=false;
    for(int i=0;i<m;++i){ CHECK(w.dp[i]<m&&w.up[i]<m); if(!w.conf[i]) tail=true; else CHECK(!tail);
      std::vector<double> v(m,p), q(m,2.0); int r=pbBands(h,l,v,q,w,i,(int)(rng()%5)-2); CHECK(r==0||r==1); } }
  std::cout<<n-fails<<"/"<<n<<" passed\n"; return fails?1:0; }
