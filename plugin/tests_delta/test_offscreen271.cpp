// (2.7.1, 2026-10-13) an A whose peak bar is off screen (scrolled back, or its stamp not on the chart) gets NO profile label: the old
// "<code> 2.0x  +N [offscreen]" label sat on circles and swing labels. The event stays recorded and auditable (suppressed:offscreen);
// an on-screen A keeps its circle and label.
#include "../DeltaProfile.cpp"
#include <iostream>
#include <functional>
#include <cstdlib>
namespace mock { Host* current=NULL; }
static int passed=0;
static void check(bool x,const char* why){if(!x)throw std::runtime_error(why);++passed;}
static ChartState& state(mock::Host& h){return *static_cast<ChartState*>(h.data);}
static mock::Host fixture(){   // a selling node whose peak bar (44, the forming bar) then closes green above it -> confirmed bullish A
    mock::Host h;h.seconds=180;h.tick=.25f;
    const unsigned long start=1791820800UL+18*3600;
    for(int i=0;i<45;++i){mock::Bar b;b.time=start+i*180;b.low=100;b.high=125;b.open=104;b.close=104;
        b.rows={mock::Price(120,1,0,1),mock::Price(121,1,0,1),mock::Price(122,1,0,1),mock::Price(123,1,0,1),mock::Price(124,1,0,1)};b.volume=5;h.bars.push_back(b);}
    h.bars[44].rows.push_back(mock::Price(100,0,10000,10000));h.bars[44].volume+=10000;h.bars[44].low=99;h.bars[44].open=99;h.bars[44].close=101;return h;
}
static int rings(mock::Host& h){int n=0;for(const mock::Draw& d:h.draws)if(d.type=="ring")++n;return n;}
static int aLabels(mock::Host& h,bool offscreenOnly){int n=0;for(const mock::Draw& d:h.draws)if(d.type=="text"&&(offscreenOnly?d.text.find("offscreen")!=std::string::npos:(d.text.find("A ")==0||d.text.find("AE")==0)))++n;return n;}
static void draw(DeltaProfile& p,mock::Host& h){mock::current=&h;h.draws.clear();if(h.data)state(h).builtAt-=1001;check(p.draw()==RTX_OK,"draw failed");}
static Latch& first(mock::Host& h){return latchBooks()[state(h).book+"|180"].v[0];}
int main(){
 setenv("TZ","UTC",1);tzset();unsetenv("USERPROFILE");
 mock::Host h=fixture();DeltaProfile p;draw(p,h);
 mock::Bar g=h.bars.back();g.time+=180;g.open=104;g.close=107;h.bars.push_back(g);g.time+=180;h.bars.push_back(g);draw(p,h);
 const long long id=first(h).id;
 check(first(h).code=="A","fixture: confirmed A");
 check(rings(h)==1&&aLabels(h,false)>=1,"on screen: the circle and its profile label are drawn");
 check(state(h).bubbleState[id]=="drawn","on screen: bubble drawn");
 // scroll back so the A's peak bar is off the left of the pane (bar 44 is at x = 490 in the mock; the pane now starts at 497)
 const int peakX=50+44*10;check(peakX==490,"fixture: peak bar x");h.paneLeft=peakX+7;draw(p,h);
 check(state(h).bubbleState[id]=="offscreen"||state(h).bubbleState[id]=="","fixture: the A's circle is offscreen");
 bool anyOn=false;for(auto& kv:state(h).bubbleState)if(kv.second=="drawn"||kv.second=="shared")anyOn=true;
 check(!anyOn,"fixture: no A circle on screen");
 check(aLabels(h,true)==0,"2.7.1: no [offscreen] text is drawn");
 check(aLabels(h,false)==0,"2.7.1: no A label at the profile for an offscreen A");
 bool sup=false;for(auto& kv:state(h).profileState)if(kv.second=="suppressed:offscreen")sup=true;
 check(sup,"2.7.1: the current node's A recorded as suppressed:offscreen (auditable)");
 check(rings(h)==0,"no circle for an offscreen A");
 h.paneLeft=0;draw(p,h);
 check(rings(h)>=1&&aLabels(h,false)>=1,"back on screen: circle + label return (the events never changed)");
 mock::current=&h;p.destroy();latchBooks().clear();
 std::cout<<"RESULT "<<passed<<" passed, 0 failed\n";return 0;
}
