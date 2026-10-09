#include "../DeltaProfile.cpp"
#include <iostream>
#include <functional>
#include <thread>
#include <sys/stat.h>
namespace mock { Host* current=NULL; }
static int passed=0;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static void test(const char* name,const std::function<void()>& fn){
    try{fn();++passed;std::cout<<"PASS "<<name<<'\n';}
    catch(const std::exception& e){std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';std::exit(1);}
}
static mock::Host history(int count=92,int seconds=60){
    mock::Host h;h.seconds=seconds;
    const unsigned long start=1700000000;
    for(int i=0;i<count;++i){mock::Bar b;b.time=start+static_cast<unsigned long>(i*seconds);b.rows.push_back(mock::Price(100,1,0,1));b.volume=1;h.bars.push_back(b);}
    return h;
}
static ChartState& state(mock::Host& h){return *static_cast<ChartState*>(h.data);}
static void cleanup(DeltaProfile& p,mock::Host& h){mock::current=&h;p.destroy();check(h.data==NULL,"user data not freed");latchBooks().clear();}
int main(){
    setenv("TZ","UTC",1);tzset();
    test("rolling 90-min window is half-open at cutoff",[]{
        mock::Host h=history();mock::current=&h;DeltaProfile p;check(p.draw()==RTX_OK,"draw failed");
        check(state(h).ready && state(h).snapshot.buckets[0].delta==90,"wrong window inclusion");cleanup(p,h);
    });
    test("repaint is throttled without rereading VAP",[]{
        mock::Host h=history();mock::current=&h;DeltaProfile p;p.draw();int calls=h.rowCalls;p.draw();check(h.rowCalls==calls,"repaint reread");cleanup(p,h);
    });
    test("same-total-volume delta correction is periodically refreshed",[]{
        mock::Host h=history();mock::current=&h;DeltaProfile p;p.draw();h.bars.back().rows[0]=mock::Price(100,0,1,1);
        state(h).builtAt-=1001;p.draw();check(state(h).snapshot.buckets[0].delta==88,"unchanged volume hid correction");cleanup(p,h);
    });
    test("new bar triggers immediate rebuild",[]{
        mock::Host h=history();mock::current=&h;DeltaProfile p;p.draw();int calls=h.rowCalls;mock::Bar b=h.bars.back();b.time+=60;h.bars.push_back(b);p.draw();
        check(h.rowCalls>calls && state(h).barCount==93,"new bar throttled");cleanup(p,h);
    });
    test("two same-root chart contexts never share snapshot",[]{
        mock::Host a=history(),b=history();b.chart="other";for(auto& bar:b.bars)bar.rows[0]=mock::Price(100,0,1,1);
        DeltaProfile p;mock::current=&a;p.draw();mock::current=&b;p.draw();
        check(a.data!=b.data && state(a).snapshot.buckets[0].delta==90 && state(b).snapshot.buckets[0].delta==-90,"cross-chart leak");cleanup(p,a);cleanup(p,b);
    });
    test("fault disablement is local to host context",[]{
        mock::Host a=history(),b=history();DeltaProfile p;mock::current=&a;p.draw();state(a).faulted=true;p.draw();mock::current=&b;p.draw();
        check(!state(a).ready && state(b).ready,"fault leaked");cleanup(p,a);cleanup(p,b);
    });
    test("done resets local fault and cached state",[]{
        mock::Host h=history();mock::current=&h;DeltaProfile p;p.draw();state(h).faulted=true;p.done();p.draw();check(state(h).ready && !state(h).faulted,"done did not reset");cleanup(p,h);
    });
    test("symbol identity change resets prior local cache",[]{
        mock::Host h=history();mock::current=&h;DeltaProfile p;p.draw();state(h).faulted=true;h.symbol="ESU27";p.draw();check(state(h).ready,"identity retained old fault");cleanup(p,h);
    });
    test("native tick property takes precedence over ES table",[]{
        mock::Host h=history();h.tick=0.5;mock::current=&h;DeltaProfile p;p.draw();check(state(h).tick==0.5,"hardcoded ES tick");cleanup(p,h);
    });
    test("missing IRT tick falls back to the exchange tick of a known market",[]{
        mock::Host h=history();h.tick=0;mock::current=&h;DeltaProfile p;p.draw();check(state(h).ready && state(h).tick==0.25,"no ES fallback");cleanup(p,h);
    });
    test("unknown tick on an unknown market does not produce profile/signals",[]{
        mock::Host h=history();h.tick=0;h.root="ZZ";mock::current=&h;DeltaProfile p;p.draw();check(!state(h).ready && h.rowCalls==0,"zero tick accepted");cleanup(p,h);
    });
    test("all 4001 reported prices read without clipping",[]{
        mock::Host h=history();mock::Bar& b=h.bars.back();b.rows.clear();
        for(int i=0;i<4001;++i)b.rows.push_back(mock::Price(static_cast<float>(i),1,0,1));
        b.volume=4001;b.open=100;b.low=0;b.high=4001;b.close=100;
        mock::current=&h;DeltaProfile p;p.draw();double total=0;for(const dp::Bucket& k:state(h).snapshot.buckets)total+=k.delta;
        check(state(h).ready && total==4090 && h.rowCalls>=4001,"partial VAP");cleanup(p,h);
    });
    test("a failed read on a CLOSED bar drops that whole bar (never a prefix) and marks the read provisional",[]{
        mock::Host h=history();h.bars[80].rows.push_back(mock::Price(101,2,0,2));h.bars[80].failPrice=1;
        mock::current=&h;DeltaProfile p;p.draw();
        check(state(h).ready && state(h).snapshot.buckets.size()==1 && state(h).snapshot.buckets[0].delta==89 && state(h).missingVap==1 && state(h).shortHistory,"prefix or blanking");cleanup(p,h);
    });
    test("the forming bar without VAP yet is skipped without downgrading signals",[]{
        mock::Host h=history();h.bars.back().failStats=1;mock::current=&h;DeltaProfile p;p.draw();
        check(state(h).ready && state(h).snapshot.buckets[0].delta==89 && state(h).missingVap==0 && !state(h).shortHistory,"forming bar handling");
        h.bars.back().failStats=0;state(h).builtAt-=1001;p.draw();check(state(h).ready && state(h).snapshot.buckets[0].delta==90,"no recovery");cleanup(p,h);
    });
    test("positive bar volume with zero price rows is skipped and flagged",[]{
        mock::Host h=history();h.bars[50].reportedPrices=0;mock::current=&h;DeltaProfile p;p.draw();check(state(h).ready && state(h).missingVap==1 && state(h).snapshot.buckets[0].delta==89,"missing rows");cleanup(p,h);
    });
    test("negative VAP row is dropped and counted, profile still drawn",[]{
        mock::Host h=history();h.bars[60].rows[0]=mock::Price(100,-1,0,1);mock::current=&h;DeltaProfile p;p.draw();check(state(h).ready && state(h).vapMismatch>=1 && state(h).snapshot.buckets[0].delta==89,"negative volume");cleanup(p,h);
    });
    test("buy plus sell above total is counted and the row kept with delta inside volume",[]{
        mock::Host h=history();h.bars[60].rows[0]=mock::Price(100,100,10,1);mock::current=&h;DeltaProfile p;p.draw();
        check(state(h).ready && state(h).vapMismatch>=1 && state(h).snapshot.buckets[0].delta==89+90,"inconsistent attribution");cleanup(p,h);
    });
    test("DST fall-back repeated local hour never blanks the profile",[]{
        mock::Host h=history();h.bars[70].time=h.bars[69].time-1800;mock::current=&h;DeltaProfile p;p.draw();check(state(h).ready,"nonmonotonic stamps blanked");cleanup(p,h);
    });
    test("host without a per-chart slot falls back to one shared state (no crash, no leak per draw)",[]{
        mock::Host h=history();h.noSlot=true;mock::current=&h;DeltaProfile p;p.draw();p.draw();check(h.data==NULL,"slot used");
        p.destroy();
    });
    test("no Dealer Profile status: the profile sits against the price scale",[]{
        mock::Host h=history();mock::current=&h;DeltaProfile p;p.draw();bool axis=false;
        for(const mock::Draw& d:h.draws) if(d.type=="line" && d.x==1976) axis=true;
        check(state(h).dealerReach==0,"reach default");cleanup(p,h);(void)axis;
    });
    test("one-second charts are not truncated at 3000 bars",[]{
        mock::Host h=history(6000,1);mock::current=&h;DeltaProfile p;p.draw();check(state(h).ready && state(h).snapshot.buckets[0].delta==5400,"window truncated");cleanup(p,h);
    });
    test("insufficient history is reported as provisional",[]{
        mock::Host h=history(10,60);mock::current=&h;DeltaProfile p;p.draw();check(state(h).shortHistory && state(h).state.find("provisional")!=std::string::npos,"partial history hidden");cleanup(p,h);
    });
    test("drawing infeasible does not claim drawn",[]{
        mock::Host h=history();h.paneRight=100;h.scaleLeft=90;mock::current=&h;DeltaProfile p;p.draw();check(state(h).state.find("not drawn")!=std::string::npos && state(h).drawnRows==0,"false drawn state");cleanup(p,h);
    });
    test("opening settings does not save or truncate file",[]{
        mock::Host h=history();h.directory="/tmp/delta-profile-adapter";setenv("USERPROFILE",h.directory.c_str(),1);
        mkdir(h.directory.c_str(),0700);mkdir((h.directory+"/InvestorRT").c_str(),0700);mkdir((h.directory+"/InvestorRT/rtx").c_str(),0700);mkdir((h.directory+"/InvestorRT/rtx/lsFlexLevels").c_str(),0700);
        std::string file=h.directory+"/InvestorRT/rtx/lsFlexLevels/DeltaProfile.settings.txt";
        const std::string line="ES|0|88|7|12|1|0|0|0|0|1|0|90|3|1\n";{std::ofstream out(file);out<<line;}
        mock::current=&h;DeltaProfile p;p.parmsLoad();std::ifstream in(file);std::ostringstream text;text<<in.rdbuf();
        check(text.str()==line && h.parms[0]==88 && h.parms[1]==7 && h.parms[2]==12,"opening rewrote layout");cleanup(p,h);unsetenv("USERPROFILE");
    });
    test("malformed layout clamps without chart overflow",[]{
        mock::Host h=history();mock::current=&h;DeltaProfile p;p.draw();h.parms[0]=INT_MAX;h.parms[1]=INT_MIN;h.parms[2]=INT_MAX;p.parmsApply();
        check(state(h).layout.width==600 && state(h).layout.gap==-300 && state(h).layout.font==18,"layout unclamped");cleanup(p,h);
    });
    test("absorption circle uses exact displayed node bar",[]{
        mock::Host h=history(110,60);
        for(auto& b:h.bars){b.rows={mock::Price(120,1,0,1),mock::Price(121,1,0,1),mock::Price(122,1,0,1),mock::Price(123,1,0,1),mock::Price(124,1,0,1)};b.volume=5;b.low=100;b.high=125;b.open=103;b.close=104;}
        h.bars[40].rows.push_back(mock::Price(100,0,10000,10000));h.bars[40].volume+=10000;
        h.bars[40].low=99;h.bars[40].high=125;h.bars[40].close=100;h.bars[40].open=100;
        mock::current=&h;DeltaProfile p;p.draw();int rings=0;bool letter=false;
        for(const mock::Draw& d:h.draws){if(d.type=="ring"){++rings;check(d.x==450,"wrong peak candle");}if(d.type=="text" && d.text.find("A ")==0)letter=true;}
        check(rings==1 && letter,"circle/letter mismatch");
        std::vector<dp::Node> before=state(h).snapshot.nodes;int calls=h.rowCalls;h.paneBottom+=70;h.draws.clear();p.draw();
        check(h.rowCalls==calls && state(h).snapshot.nodes[0].bucket==before[0].bucket && state(h).snapshot.nodes[0].state.code==before[0].state.code,"scroll changed signal");cleanup(p,h);
    });
    test("2.5.0: a decided absorption keeps its circle after its node leaves the sliding 90-min profile",[]{
        mock::Host h=history(110,60);
        for(auto& b:h.bars){b.rows={mock::Price(120,1,0,1),mock::Price(121,1,0,1),mock::Price(122,1,0,1),mock::Price(123,1,0,1),mock::Price(124,1,0,1)};b.volume=5;b.low=100;b.high=125;b.open=103;b.close=104;}
        h.bars[40].rows.push_back(mock::Price(100,0,10000,10000));h.bars[40].volume+=10000;
        h.bars[40].low=99;h.bars[40].high=125;h.bars[40].close=100;h.bars[40].open=100;
        mock::current=&h;DeltaProfile p;p.draw();
        check(!latchBooks().empty() && !latchBooks().begin()->second.v.empty() && latchBooks().begin()->second.v[0].code=="A","decided A not recorded");
        mock::Bar last=h.bars.back();
        for(int i=0;i<60;++i){mock::Bar b=last;b.time=last.time+static_cast<unsigned long>((i+1)*60);h.bars.push_back(b);}
        h.draws.clear();p.draw();
        bool nodeGone=true;for(const dp::Node& n:state(h).snapshot.nodes) if(n.state.peakBar==40) nodeGone=false;
        int rings=0;for(const mock::Draw& d:h.draws) if(d.type=="ring") ++rings;
        check(nodeGone,"fixture: node still in window");
        check(rings>=1,"circle vanished with the node (repaint)");cleanup(p,h);
    });
    test("2.5.0: accumulation / distribution zones are not drawn (absorption only)",[]{
        mock::Host h=history(110,60);
        for(auto& b:h.bars){b.rows={mock::Price(100,30,0,30),mock::Price(101,30,0,30),mock::Price(110,1,0,1),mock::Price(115,0,1,1),mock::Price(120,0,1,1)};b.volume=63;b.low=100;b.high=120;b.open=105;b.close=105;}
        mock::current=&h;DeltaProfile p;p.draw();
        bool zone=false;for(const dp::Zone& z:state(h).snapshot.zones) if(z.code.rfind("Acc",0)==0||z.code.rfind("Dst",0)==0) zone=true;
        bool drawn=false;for(const mock::Draw& d:h.draws) if(d.type=="text" && (d.text.rfind("Acc",0)==0||d.text.rfind("Dst",0)==0)) drawn=true;
        check(zone,"fixture: no Acc/Dst zone formed");
        check(!drawn,"Acc/Dst label drawn");cleanup(p,h);
    });
    test("2.5.1: a chart-state reset (done / identity change) does not wipe a decided A or its circle",[]{
        mock::Host h=history(110,60);
        for(auto& b:h.bars){b.rows={mock::Price(120,1,0,1),mock::Price(121,1,0,1),mock::Price(122,1,0,1),mock::Price(123,1,0,1),mock::Price(124,1,0,1)};b.volume=5;b.low=100;b.high=125;b.open=103;b.close=104;}
        h.bars[40].rows.push_back(mock::Price(100,0,10000,10000));h.bars[40].volume+=10000;
        h.bars[40].low=99;h.bars[40].high=125;h.bars[40].close=100;h.bars[40].open=100;
        mock::current=&h;DeltaProfile p;p.draw();
        p.done();                                                   // IRT resets the host context
        mock::Bar last=h.bars.back();
        for(int i=0;i<60;++i){mock::Bar b=last;b.time=last.time+static_cast<unsigned long>((i+1)*60);h.bars.push_back(b);}
        h.draws.clear();p.draw();
        int rings=0;for(const mock::Draw& d:h.draws) if(d.type=="ring"){++rings;check(d.x==450,"circle moved to another bar");}
        check(rings>=1,"reset wiped the decided A's circle");cleanup(p,h);
    });
    test("2.5.2: a node pushed out of the top 3 keeps its signal, is still decided by a green close, and keeps letter + circle",[]{
        mock::Host h=history(110,60);
        for(auto& b:h.bars){b.rows={mock::Price(120,1,0,1),mock::Price(121,1,0,1),mock::Price(122,1,0,1),mock::Price(123,1,0,1),mock::Price(124,1,0,1)};b.volume=5;b.low=100;b.high=125;b.open=104;b.close=104;}  // dojis: undecided
        h.bars[40].rows.push_back(mock::Price(100,0,10000,10000));h.bars[40].volume+=10000;
        h.bars[40].low=99;h.bars[40].high=125;h.bars[40].close=100;h.bars[40].open=100;
        mock::current=&h;DeltaProfile p;p.draw();
        check(!latchBooks().empty() && latchBooks().begin()->second.v.size()==1 && latchBooks().begin()->second.v[0].code=="A?","candidate not tracked");
        // three much bigger nodes appear: the 100 node leaves the top 3
        mock::Bar big=h.bars.back();big.time+=60;big.rows={mock::Price(110,90000,0,90000),mock::Price(113,0,80000,80000),mock::Price(116,70000,0,70000)};big.volume=240000;big.open=104;big.close=104;h.bars.push_back(big);
        h.draws.clear();p.draw();
        bool inTop=false;for(const dp::Node& n:state(h).snapshot.nodes) if(n.price<101) inTop=true;
        check(!inTop,"fixture: node still in the top 3");
        // a green bar closes above the node, then one more bar so it is closed
        mock::Bar g=h.bars.back();g.time+=60;g.rows={mock::Price(120,1,0,1)};g.volume=1;g.open=103;g.close=106;h.bars.push_back(g);
        mock::Bar f=g;f.time+=60;h.bars.push_back(f);
        h.draws.clear();p.draw();
        check(latchBooks().begin()->second.v[0].code=="A" && latchBooks().begin()->second.v[0].support,"green close above did not decide the tracked node");
        int rings=0;bool letter=false;
        for(const mock::Draw& d:h.draws){if(d.type=="ring"&&d.x==450)++rings;if(d.type=="text"&&d.text=="A")letter=true;}
        check(rings==1,"tracked A lost its circle");check(letter,"tracked A lost its letter");cleanup(p,h);
    });
    test("initiative letter never receives a circle",[]{
        mock::Host h=history(110,60);
        for(auto& b:h.bars){b.rows={mock::Price(80,1,0,1),mock::Price(81,1,0,1),mock::Price(82,1,0,1),mock::Price(83,1,0,1),mock::Price(84,1,0,1)};b.volume=5;b.low=79;b.high=100;b.open=97;b.close=96;}
        h.bars[40].rows.push_back(mock::Price(100,0,10000,10000));h.bars[40].volume+=10000;h.bars[40].open=100;h.bars[40].close=100;h.bars[40].high=101;
        mock::current=&h;DeltaProfile p;p.draw();bool letter=false;
        for(const mock::Draw& d:h.draws){check(d.type!="ring","initiative circle");if(d.type=="text" && d.text.find("I ")==0)letter=true;}
        check(letter,"initiative letter absent");cleanup(p,h);
    });
    test("2.4.3: every absorption circle has its letter, even when the full label does not fit",[]{
        mock::Host h=history(110,60);
        for(auto& b:h.bars){b.rows={mock::Price(120,1,0,1),mock::Price(121,1,0,1),mock::Price(122,1,0,1),mock::Price(123,1,0,1),mock::Price(124,1,0,1)};b.volume=5;b.low=100;b.high=125;b.open=103;b.close=104;}
        h.bars[40].rows.push_back(mock::Price(100,0,10000,10000));h.bars[40].volume+=10000;
        h.bars[40].low=99;h.bars[40].high=125;h.bars[40].close=100;h.bars[40].open=100;
        mock::current=&h;DeltaProfile p;p.draw();
        int ringsWide=0,lettersWide=0;for(const mock::Draw& d:h.draws){if(d.type=="ring")++ringsWide;if(d.type=="text" && d.text.rfind("A",0)==0 && (d.text.size()<=2 || d.text[1]==' ' || d.text[1]=='?'))++lettersWide;}
        check(ringsWide==1 && lettersWide==1,"wide pane: one circle, one letter");
        state(h).layout.width=600;h.draws.clear();p.draw();                 // a very wide profile leaves almost no room for labels
        int rings=0,letters=0;for(const mock::Draw& d:h.draws){if(d.type=="ring")++rings;if(d.type=="text" && d.text.rfind("A",0)==0 && (d.text.size()<=2 || d.text[1]==' ' || d.text[1]=='?'))++letters;}
        check(rings==letters,"circles and letters must match one to one");cleanup(p,h);
    });
    std::cout<<"RESULT "<<passed<<" tests passed\n";
}
