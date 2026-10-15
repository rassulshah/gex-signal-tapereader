// test_dp_bubble_halfway.cpp (was test_dp283.cpp) -- lsDeltaProfile 2.8.3/2.8.4 (2026-10-14): the coordinator's 2.8.1 chart rule (Rassul 18:30-18:39 CT "only focused on
// identifying absorption, consistent with the bubbles", "no AE", "keep it simple, implement A and its bubble, never repaint the bubble
// after it's plotted") + the external-audit display list (18:25 CT, section 4). Mock SDK (tests_delta/irtsdk.h).
// Build (from tests_delta/): g++ -std=c++14 -O1 -g -I. -fsanitize=address,undefined test_dp283.cpp
#include "../DeltaProfile.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
namespace mock { Host* current=NULL; }
static int passed=0,failed=0;
static void check(bool x,const std::string& why){if(x)++passed;else{++failed;std::cout<<"FAIL "<<why<<'\n';}}
static ChartState& state(mock::Host& h){return *static_cast<ChartState*>(h.data);}
static mock::Bar bar(unsigned long t){mock::Bar b;b.time=t;b.low=100;b.high=125;b.open=104;b.close=104;
    b.rows={mock::Price(120,1,0,1),mock::Price(121,1,0,1),mock::Price(122,1,0,1),mock::Price(123,1,0,1),mock::Price(124,1,0,1)};b.volume=5;return b;}
static mock::Host fixture(){   // a selling node whose peak bar (44) then closes green above it -> a confirmed bullish A (as test_offscreen271)
    mock::Host h;h.seconds=180;h.tick=.25f;
    const unsigned long start=1791820800UL+18*3600;
    for(int i=0;i<45;++i)h.bars.push_back(bar(start+i*180));
    h.bars[44].rows.push_back(mock::Price(100,0,10000,10000));h.bars[44].volume+=10000;h.bars[44].low=99;h.bars[44].open=99;h.bars[44].close=101;return h;
}
static void draw(mock::Host& h,DeltaProfile& p){mock::current=&h;h.draws.clear();if(h.data)state(h).builtAt-=1001;check(p.draw()==RTX_OK,"draw ok");}
static std::vector<std::pair<int,int> > rings(const mock::Host& h){std::vector<std::pair<int,int> > r;for(const mock::Draw& d:h.draws)if(d.type=="ring")r.push_back(std::make_pair(d.x,d.y));return r;}
static bool starts(const std::string& s,const char* k){return s.compare(0,std::strlen(k),k)==0;}
static int textsStarting(const mock::Host& h,const char* k){int n=0;for(const mock::Draw& d:h.draws)if(d.type=="text"&&starts(d.text,k))++n;return n;}
static bool isAmount(const std::string& t){if(t.size()<2||(t[0]!='+'&&t[0]!='-'))return false;for(size_t i=1;i<t.size();++i)if(!isdigit((unsigned char)t[i])&&t[i]!=','&&t[i]!='.'&&t[i]!='K'&&t[i]!='k'&&t[i]!='M')return false;return true;}
static int deltaTexts(const mock::Host& h){int n=0;for(const mock::Draw& d:h.draws)if(d.type=="text"&&isAmount(d.text))++n;return n;}
static void greenBar(mock::Host& h){mock::Bar g=h.bars.back();g.time+=180;g.rows=bar(0).rows;g.volume=5;g.low=100;g.high=125;g.open=104;g.close=107;h.bars.push_back(g);}
// (2.8.2 halfway rule, Rassul 19:44) a peak bar 99..125 (halfway 112): the node's middle must be at / below it for a bullish A (selling
// node), at / above it for a bearish A (buying node); the close rule is met in every case so only the half decides
static int halfCase(double nodePx,bool sellNode,float open,float close){
    mock::Host h;h.seconds=180;h.tick=.25f;const unsigned long start=1791820800UL+18*3600;
    for(int i=0;i<45;++i)h.bars.push_back(bar(start+i*180));
    mock::Bar& p=h.bars[44];
    p.rows.push_back(sellNode?mock::Price(nodePx,0,10000,10000):mock::Price(nodePx,10000,0,10000));p.volume+=10000;p.low=99;p.high=125;p.open=open;p.close=close;
    DeltaProfile d;draw(h,d);mock::Bar g=h.bars.back();g.time+=180;g.rows=bar(0).rows;g.volume=5;g.open=g.close=sellNode?close+1:close-1;g.low=99;g.high=125;h.bars.push_back(g);g.time+=180;h.bars.push_back(g);draw(h,d);
    const int n=static_cast<int>(rings(h).size());mock::current=&h;d.destroy();latchBooks().clear();return n;
}
int main(){
    setenv("TZ","UTC",1);tzset();unsetenv("USERPROFILE");
    check(std::string(DLT_VERSION)=="2.8.4","version 2.8.4");   // (2.8.4 night verification: version bumped)
    {   std::ifstream f("../DeltaProfile.cpp");std::stringstream ss;ss<<f.rdbuf();
        check(ss.str().find(std::string("p->setVersion(\"")+DLT_VERSION+"\")")!=std::string::npos,"the setVersion literal = DLT_VERSION (2.8.1-2.8.3 shipped setVersion 2.8.0)");
        check(ss.str().find("(cfg.p.proven ? \"Ex\" : \"Ex?\")")==std::string::npos,"2.8.3: the recorded Ex mark no longer switches to plain Ex when PROVEN"); }
    mock::Host h=fixture();DeltaProfile p;draw(h,p);
    greenBar(h);greenBar(h);draw(h,p);
    const std::vector<std::pair<int,int> > r0=rings(h);
    check(r0.size()==1,"the confirmed A: one bubble");
    check(textsStarting(h,"A ")>=1||textsStarting(h,"A")>=1,"the confirmed A: its profile label");
    // 2.8.1: no AE / AE? / A? / I / I? labels and no Ex? marks on the chart
    check(textsStarting(h,"AE")==0&&textsStarting(h,"A?")==0&&textsStarting(h,"I ")==0&&textsStarting(h,"I?")==0&&textsStarting(h,"Ex")==0,"2.8.1: no AE / A? / I / I? labels, no Ex marks");
    int aLabels=0;for(const mock::Draw& d:h.draws)if(d.type=="text"&&(d.text=="A"||starts(d.text,"A ")))++aLabels;
    check(deltaTexts(h)<=aLabels,"2.8.1: delta text only on the labelled A nodes ("+std::to_string(deltaTexts(h))+" texts, "+std::to_string(aLabels)+" A labels)");
    {   // with every row carrying delta (the fixture's 120-124 rows), the pre-2.8.1 per-row text would exceed the A labels
        check(state(h).drawnRows>aLabels,"fixture: more delta rows than A labels (the 2.8.1 rule is exercised)"); }
    // never repaint: 10 more bars (quiet), the bubble keeps its bar and price - same pixel, same count
    for(int i=0;i<10;++i){greenBar(h);draw(h,p);
        const std::vector<std::pair<int,int> > r=rings(h);
        if(!(r.size()==1&&r[0]==r0[0])){check(false,"bar "+std::to_string(i)+": the bubble moved or vanished");break;}}
    check(rings(h).size()==1&&rings(h)[0]==r0[0],"10 later bars: the A bubble never moved or vanished");
    // a NEW A event (a second selling node, then a green close): the first bubble stays exactly where it was
    {   mock::Bar s=h.bars.back();s.time+=180;s.rows=bar(0).rows;s.rows.push_back(mock::Price(110,0,12000,12000));s.volume=12005;s.low=109;s.high=125;s.open=111;s.close=110;h.bars.push_back(s);
        draw(h,p);greenBar(h);draw(h,p);greenBar(h);draw(h,p);
        const std::vector<std::pair<int,int> > r=rings(h);bool kept=false;for(size_t i=0;i<r.size();++i)if(r[i]==r0[0])kept=true;
        check(kept,"a new A event: the first bubble is still drawn at the same bar / price");
        check(r.size()>=1,"bubbles after a new A event: "+std::to_string(r.size()));
        check(textsStarting(h,"AE")==0&&textsStarting(h,"Ex")==0,"still no AE / Ex marks after the new event"); }
    // a restart (new plugin object, same records): the same bubble at the same place
    {   const std::vector<std::pair<int,int> > before=rings(h);mock::current=&h;p.destroy();latchBooks().clear();
        DeltaProfile q;draw(h,q);bool same=rings(h)==before;check(same,"a restart redraws the same bubbles at the same places");
        // only scrolling hides it: the pane starts right of the first A's bar -> not drawn; back -> drawn again at the same pixel
        h.paneLeft=r0[0].first+7;draw(h,q);bool gone=true;for(const auto& x:rings(h))if(x==r0[0])gone=false;check(gone,"scrolled off the left: hidden");
        h.paneLeft=0;draw(h,q);bool back=false;for(const auto& x:rings(h))if(x==r0[0])back=true;check(back,"scrolled back: the same bubble, same pixel");
        mock::current=&h;q.destroy();latchBooks().clear(); }
    // the halfway cases (2.8.2): lower-half selling node = A; upper-half selling node = not A; the mirror for buying nodes
    check(halfCase(100,true,99,101)==1,"2.8.2: a selling node in the LOWER half (100 <= 112), green close above it = A");
    check(halfCase(120,true,99,121)==0,"2.8.2: a selling node in the UPPER half (120 > 112), green close above it = NOT A");
    check(halfCase(124,false,125,123)==1,"2.8.2: a buying node in the UPPER half (124 >= 112), red close below it = A");
    check(halfCase(105,false,125,104)==0,"2.8.2: a buying node in the LOWER half (105 < 112), red close below it = NOT A");
    check(halfCase(112,true,99,113)==1&&halfCase(112,false,125,111)==1,"2.8.2: a node exactly at the halfway mark counts for both sides");
    std::cout<<"RESULT "<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
}
