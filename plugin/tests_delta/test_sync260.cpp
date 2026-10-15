#include "../DeltaProfile.cpp"
#include <iostream>
#include <functional>
#include <cstdlib>
namespace mock { Host* current=NULL; }
static int passed=0,failedN=0;
static void check(bool x,const char* why){if(!x){++failedN;std::cerr<<"FAIL "<<why<<"\n";return;}++passed;}   // (2.8.4) counts, no abort
static ChartState& state(mock::Host& h){return *static_cast<ChartState*>(h.data);}
static mock::Host fixture(bool green=false){
    mock::Host h;h.seconds=180;h.tick=.25f;
    const unsigned long start=1791820800UL+18*3600;
    for(int i=0;i<45;++i){mock::Bar b;b.time=start+i*180;b.low=100;b.high=125;b.open=104;b.close=green?106:104;
        b.rows={mock::Price(120,1,0,1),mock::Price(121,1,0,1),mock::Price(122,1,0,1),mock::Price(123,1,0,1),mock::Price(124,1,0,1)};b.volume=5;h.bars.push_back(b);}
    h.bars[20].rows.push_back(mock::Price(100,0,10000,10000));h.bars[20].volume+=10000;
    h.bars[20].low=99;h.bars[20].open=100;h.bars[20].close=100;
    if(green){h.bars[20].open=99;h.bars[20].close=101;}   // (2.8.4 test update, 2.6.2 rule) the PEAK bar itself closes green above the node
    return h;
}
static int rings(mock::Host& h){int n=0;for(const mock::Draw& d:h.draws)if(d.type=="ring")++n;return n;}
static void cleanup(DeltaProfile& p,mock::Host& h){mock::current=&h;p.destroy();latchBooks().clear();}
static void draw(DeltaProfile& p,mock::Host& h){mock::current=&h;h.draws.clear();if(h.data)state(h).builtAt-=1001;check(p.draw()==RTX_OK,"draw failed");}
static Latch& first(mock::Host& h){return latchBooks()[state(h).book+"|180"].v[0];}
int main(){
 setenv("TZ","UTC",1);tzset();unsetenv("USERPROFILE");
 {
  mock::Host h=fixture();DeltaProfile p;draw(p,h);
  check(first(h).code=="I?","(2.6.2) doji peak bar: absorption ruled out, pending I?");check(rings(h)==0,"I? must not bubble");
  mock::Bar r=h.bars.back();r.time+=180;r.open=99;r.close=97;r.low=96;h.bars.push_back(r);r.time+=180;h.bars.push_back(r);draw(p,h);
  check(first(h).code=="I","pending resolves I");check(rings(h)==0,"I must not bubble");
  bool I=false;for(auto& d:h.draws)if(d.type=="text"&&d.text.find("I ")==0)I=true;
  check(!I,"(2.8.1) the I is recorded, not drawn on the profile");cleanup(p,h);
 }
 {
  mock::Host h=fixture(true);DeltaProfile p;draw(p,h);const long long id=first(h).id;   // (2.8.4) the peak bar decides A
  mock::Bar g=h.bars.back();g.time+=180;g.open=104;g.close=107;h.bars.push_back(g);g.time+=180;h.bars.push_back(g);draw(p,h);
  check(first(h).code=="A"&&first(h).support,"confirmed A bullish");check(rings(h)==1,"A gets one bubble");
  check(state(h).bubbleState[id]=="drawn","bubble shares event id");check(state(h).profileState[id]=="shown:A","profile shares event id");
  check(state(h).pairedAbsorptions==1,"paired accounting");
  bool color=true;for(auto& d:h.draws)if((d.type=="ring")||(d.type=="text"&&d.text.find("A ")==0))color &= d.color==C_SUPPORT;
  check(color,"A bubble and label same final bullish color");
  Latch old=first(h);int count=h.bars.size();for(int j=0;j<32;++j){g.time+=180;h.bars.push_back(g);}draw(p,h);
  check(state(h).snapshot.nodes.empty(),"old node outside 90min");check(rings(h)==1,"historical A remains after node leaves");
  check(first(h).price==old.price&&first(h).peakTime==old.peakTime&&first(h).support==old.support,"historical A immutable");cleanup(p,h);(void)count;
 }
 {
  mock::Host h=fixture(true);DeltaProfile p;draw(p,h);Latch old=first(h);
  // New peak at the SAME price/side, not the same event. Dojis leave it pending.
  for(int i=0;i<32;++i){mock::Bar b=h.bars.back();b.time+=180;b.open=b.close=104;b.rows={mock::Price(120,1,0,1),mock::Price(121,1,0,1),mock::Price(122,1,0,1),mock::Price(123,1,0,1),mock::Price(124,1,0,1)};b.volume=5;h.bars.push_back(b);}
  auto& peak=h.bars.back();peak.rows.push_back(mock::Price(100,0,12000,12000));peak.volume+=12000;
  draw(p,h);check(!state(h).snapshot.nodes.empty(),"new peak candidate");
  auto n=state(h).snapshot.nodes[0];check(n.state.eventId!=old.id,"new same-price peak has different identity");
  check(n.state.code=="A?","new pending node does not inherit stale A");check(rings(h)==1,"only historical confirmed A bubble retained");cleanup(p,h);
 }
 {
  // Earlier red close AFTER peak but BEFORE last material contribution must NOT decide.
  mock::Host h=fixture();h.bars[25].rows.push_back(mock::Price(100,0,4000,4000));h.bars[25].volume+=4000;
  h.bars[23].open=99;h.bars[23].close=97;h.bars[23].low=96;
  DeltaProfile p;draw(p,h);check(first(h).code!="A"&&first(h).code!="I","no false decision between peak and formation");
  check(first(h).formedTime==h.bars[25].time,"frozen formation time recorded");check(rings(h)==0,"premature peak-scan bubble absent");cleanup(p,h);
 }
 {
  mock::Host h=fixture(true);h.bars[35].failStats=1;DeltaProfile p;draw(p,h);
  check(state(h).shortHistory,"missing closed VAP is provisional");check(latchBooks()[state(h).book+"|180"].v.empty(),"partial window cannot create event");
  check(state(h).snapshot.nodes.empty()&&rings(h)==0,"partial window cannot publish signal/bubble");
  h.bars[35].failStats=0;draw(p,h);check(first(h).code=="A"&&rings(h)==1,"complete backfill admits confirmed A");
  h.bars[35].failStats=1;draw(p,h);check(rings(h)==1,"partial now retains prior confirmed A");cleanup(p,h);
 }
 {
  mock::Host h=fixture(true);DeltaProfile p;draw(p,h);check(rings(h)==1,"fixture confirmed A before outage");
  for(auto& b:h.bars)b.failStats=1;
  draw(p,h);check(!state(h).ready,"profile unavailable after full VAP outage");
  check(rings(h)==1,"recorded confirmed A survives full VAP outage");cleanup(p,h);
 }
 {
  mock::Host h=fixture(true);DeltaProfile p;draw(p,h);Latch a=first(h);
  auto& bk=latchBooks()[state(h).book+"|180"];
  Latch b=a;b.id=bk.nextId++;b.price=a.price+.25;b.low=a.low+1;b.high=a.high+1;bk.v.push_back(b);draw(p,h);
  // (2.8.5 intended rule change, Rassul 2026-10-14 23:05 CT) ONE bubble per candle per side: bullish at the LOWEST price, bearish at
  // the HIGHEST (2.6.0-2.8.4: "two distinct A locations on same candle both draw")
  check(rings(h)==1,"2.8.5: two A locations on the same candle and side draw ONE bubble");
  const Latch& keep=a.support?(a.price<b.price?a:b):(a.price>b.price?a:b), &drop=&keep==&a?b:a;
  int rx=-1,ry=-1;for(auto& d:h.draws)if(d.type=="ring"){rx=d.x;ry=d.y;}
  mock::current=&h;cppExtension::PNT want;want.set(20,static_cast<float>(keep.price),cppExtension::kBarCenter);
  check(state(h).bubbleState[keep.id]=="drawn"&&state(h).bubbleState[drop.id]=="merged:"+std::to_string(keep.id),"both events auditable: one drawn, the other merged into it");
  check(ry==want.v&&rx==want.h,"the bubble sits on the kept event (bullish lowest / bearish highest)");cleanup(p,h);
 }
 {
  // Seeder: old-color metadata cannot recolor final confirmed A; I and pending never ring.
  mock::Host h=fixture();DeltaProfile p;draw(p,h);Latch a=first(h);a.code="A";a.support=false;a.sell=false;a.tDecided=h.bars.back().time;a.decidedBy=h.bars[30].time;
  LatchBook bk;seedFromText(recordLine(a,'F',.25,"ESZ26")+"\n"+recordLine(a,'D',.25,"ESZ26")+"\n","ESZ26",.25,bk);
  check(bk.v.size()==1&&confirmedAbsorption(bk.v[0]),"restart restores only final A semantics");
  latchBooks()[state(h).book+"|180"].v=bk.v;draw(p,h);
  bool red=false;for(auto& d:h.draws)if(d.type=="ring")red=d.color==C_RESISTANCE;
  check(red,"final bearish A restored in final direction");cleanup(p,h);
 }
 {
  Latch L;L.low=400;L.high=401;L.sell=true;L.zone=false;L.peakTime=200;
  check(sameEvent(L,400,401,true,false,200),"exact identity accepted");
  check(!sameEvent(L,400,402,true,false,200),"new width not stale match");
  check(!sameEvent(L,400,401,true,false,201),"new peak not stale match");
  check(!sameEvent(L,400,401,true,true,200),"zone not same node event");
 }
 {
  Latch L;L.low=400;L.high=401;L.sell=true;L.zone=false;L.peakTime=200;L.formedTime=210;L.formedT=210;
  L.id=0;L.tFirst=220;L.peakT=200;L.price=100.125;L.code="A?";L.support=true;
  L.mult=3.9499999999999;L.net=-1234;
  RecordEvent r;check(parseRecordLine(recordLine(L,'F',.25,"ESZ26"),.25,r),"full precision record parses");
  check(r.latch.mult==L.mult&&r.latch.price==L.price,"journal round trips ratio and price exactly");
  L.code="A";L.tDecided=250;L.decidedBy=240;
  Latch changed=L;changed.price=100.25;
  LatchBook bk;seedFromText(recordLine(L,'F',.25,"ESZ26")+"\n"+recordLine(changed,'D',.25,"ESZ26")+"\n","ESZ26",.25,bk);
  check(bk.v.size()==1&&!bk.v[0].logged&&bk.rejected==1,"altered immutable decision is rejected");
 }
 std::cout<<"RESULT "<<passed<<" checks passed, "<<failedN<<" failed\n";
 return failedN?1:0;
}
