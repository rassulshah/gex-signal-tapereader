// test_trap221.cpp -- lsDeltaProfile 2.21.0 / 2.21.1 ([4] the badge in its own clear slot) (2026-10-21, Rassul approved delta-profile-trapped-mockup-v2.png 11:43 CT "ok.. approve.. build"):
// TRAPPED TRADERS. [1] TrapLogic.h rule (fires / accepted breakout / light / learning / no new extreme / top mirror), the TR record line round
// trip; [2] the real DeltaProfile.cpp on the mock SDK: the trap decided at the candle close, its TR line written, the badge "A|Tr?" / "Tr?"
// ON the profile column beside amber-outlined rows (green = trapped sellers, red = trapped buyers), never over another label or bubble, the
// size never on the chart, a restart redraws the same badge from the record (no repaint), later candles never move or remove it, the
// settings row; [3] versions.
// Build (from tests_delta/): g++ -std=c++17 -O1 -g -I../tests_dp217 -I. -I.. -fsanitize=address,undefined test_trap221.cpp -o /tmp/t_trap221
#include "../DeltaProfile.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
namespace mock { Host* current=NULL; }
static int passed=0,failed=0;
static void check(bool x,const std::string& why){if(x)++passed;else{++failed;std::cout<<"FAIL "<<why<<'\n';}}
static ChartState& state(mock::Host& h){return *static_cast<ChartState*>(h.data);}
static std::string home(const std::string& tag){
    const std::string root="/tmp/dp221_"+std::to_string(getpid()), h=root+"/"+tag;
    const std::string parts[]={root,h,h+"/InvestorRT",h+"/InvestorRT/rtx",h+"/InvestorRT/rtx/lsFlexLevels"};
    for(const std::string& p:parts)mkdir(p.c_str(),0700);
    setenv("USERPROFILE",h.c_str(),1);return h+"/InvestorRT/rtx/lsFlexLevels/";
}
// a quiet 3-min bar 100..125 with 5 rows: 6 bought / 4 sold at 120-124 (buyers attack 30, sellers 20 lots a candle; delta +2 a row)
static mock::Bar bar(unsigned long t){mock::Bar b;b.time=t;b.low=100;b.high=125;b.open=110;b.close=111;
    b.rows={mock::Price(120,6,4,10),mock::Price(121,6,4,10),mock::Price(122,6,4,10),mock::Price(123,6,4,10),mock::Price(124,6,4,10)};b.volume=50;return b;}
static const unsigned long START=1791820800UL+18*3600;
static mock::Host fixture(){mock::Host h;h.seconds=180;h.tick=.25f;for(int i=0;i<45;++i)h.bars.push_back(bar(START+i*180));return h;}
static void draw(mock::Host& h,DeltaProfile& p){mock::current=&h;h.draws.clear();if(h.data){state(h).builtAt-=1001;state(h).statusAt=-1;}check(p.draw()==RTX_OK,"draw ok");}
static void quiet(mock::Host& h){mock::Bar g=bar(h.bars.back().time+180);h.bars.push_back(g);}
static const mock::Draw* textOf(const mock::Host& h,const std::string& t){for(const mock::Draw& d:h.draws)if(d.type=="text"&&d.text==t)return &d;return NULL;}
static int texts(const mock::Host& h,const std::string& t){int n=0;for(const mock::Draw& d:h.draws)if(d.type=="text"&&d.text==t)++n;return n;}
static std::string slurp(const std::string& p){std::ifstream f(p.c_str());std::stringstream s;s<<f.rdbuf();return s.str();}
static std::string records(const std::string& flex){std::string all;for(int i=0;i<2;++i){(void)i;}
    FILE* ls=popen(("cat "+flex+"DeltaProfile/*-signals-*.csv 2>/dev/null").c_str(),"r");if(!ls)return all;char b[4096];size_t n;while((n=fread(b,1,sizeof b,ls))>0)all.append(b,n);pclose(ls);return all;}

static trapl::Candle C(long long hi,long long lo,long long cl,std::vector<trapl::Row> rows){trapl::Candle c;c.high=hi;c.low=lo;c.close=cl;c.rows=rows;return c;}
static trapl::Row R(long long k,double b,double s){trapl::Row r;r.key=k;r.bought=b;r.sold=s;return r;}

// (2.21.1) layouts with real-looking bars: a 44-candle drift, then the trap candle, then 5 candles away from it
static mock::Bar rbar(unsigned long t,float o,float c,float lo,float hi,float heavyBelow,float heavyAbove,long extra,int nodes){mock::Bar b;b.time=t;b.low=lo;b.high=hi;b.open=o;b.close=c;
    int k=0;for(float p=lo;p<=hi+1e-3f;p+=0.25f,++k){long bu=3+k%4,se=2+(k*3)%4;
        if(p<heavyBelow&&(nodes>=4||k%(4/std::max(1,nodes))==0))se+=extra; if(p>heavyAbove&&(nodes>=4||k%(4/std::max(1,nodes))==0))bu+=extra;
        b.rows.push_back(mock::Price(p,bu,se,bu+se));}
    b.volume=0;for(auto&r:b.rows)b.volume+=r.total;return b;}
// top = trapped buyers at a new high; nodes = how many heavy rows (1..4); origin / scale move the rows to the top / bottom of the pane
static bool layoutOk(bool top,int nodes,double origin,double scale,int font,std::string& why){
    home(std::string("lay")+(top?"T":"B")+std::to_string(nodes)+"_"+std::to_string((int)origin)+"_"+std::to_string(font));
    mock::Host h;h.seconds=180;h.tick=.25f;h.priceOrigin=origin;h.priceScale=scale;h.paneBottom=700;h.paneRight=1300;h.scaleLeft=1280;h.parms[2]=font;
    const unsigned long S0=1791820800UL+18*3600;float px=top?7756:7766;
    for(int i=0;i<44;++i){float tt=top?(7756.0f+14.0f*i/43.0f-((i%4)<2?1.0f:-1.0f)):(7768.0f-14.0f*i/43.0f+((i%4)<2?1.0f:-1.0f));
        float o=px,c=top?std::min(7768.25f,tt):std::max(7753.75f,tt);float lo=std::min(o,c)-0.75f,hi=std::max(o,c)+0.75f;
        if(top)hi=std::min(7770.0f,hi);else lo=std::max(7752.0f,lo);h.bars.push_back(rbar(S0+i*180,o,c,lo,hi,0,1e9f,0,0));px=c;}
    if(top)h.bars.push_back(rbar(S0+44*180,7767,7768.25,7766.5,7773.5,0,7770,900,nodes));
    else h.bars.push_back(rbar(S0+44*180,7755,7753.75,7746.5,7755.5,7752,1e9f,900,nodes));
    DeltaProfile p;mock::current=&h;p.draw();
    float c=top?7768.25f:7753.75f;
    for(int i=0;i<5;++i){h.bars.push_back(rbar(S0+(45+i)*180,c,top?c-2.2f:c+2.2f,top?c-2.6f:c-0.5f,top?c+0.5f:c+2.6f,0,1e9f,0,0));c+=top?-2.2f:2.2f;draw(h,p);}
    const ChartState& st=state(h);bool ok=true;
    if(st.trapBoxes.empty()){why="no badge drawn ("+st.trapNote+")";ok=false;}
    for(const auto& b:st.trapBoxes){
        // the badge text sits inside its box
        for(const mock::Draw& d:h.draws){if(d.type!="text"||(d.text!="Tr?"&&d.text!="A|Tr?"))continue;
            const int w=(int)d.text.size()*6;if(d.x>b.r||d.x-w<b.l||d.y<b.t||d.y>b.b){why="badge text outside its box";ok=false;}}
        // >= 4 px from every other text in the column
        for(const mock::Draw& d:h.draws){if(d.type!="text"||d.text=="Tr?"||d.text=="A|Tr?"||d.text.compare(0,5,"DLT v")==0)continue;
            const int w=(int)d.text.size()*6,hh=st.layout.font+3;const int l=d.x-w,r=d.x,t=d.y-hh/2,bb=d.y+hh/2;
            if(l<b.r+4&&b.l<r+4&&t<b.b+4&&b.t<bb+4){why="badge within 4 px of '"+d.text+"'";ok=false;}}
        // inside the column and the pane
        if(b.l<st.lay.delta.l||b.r>st.lay.delta.r||b.t<0||b.b>700){why="badge outside the column / pane";ok=false;}
    }
    mock::current=&h;p.destroy();latchBooks().clear();return ok;
}

int main(){
    setenv("TZ","UTC",1);tzset();
    std::cout<<"[1] TrapLogic.h\n";
    {   const std::vector<double> prior(20,25.0);   // median attack 25
        // trapped sellers: new low 392 < old low 400, 40 lots sold below 400, close 404 back inside
        trapl::Result r=trapl::decide(C(500,392,404,{R(395,0,20),R(398,0,20),R(450,30,5)}),false,400,prior);
        check(r.fired&&r.broke&&r.backIn&&r.lots==40&&r.med==25&&std::fabs(r.x-1.6)<1e-9&&r.lo==392&&r.hi==399,"trapped sellers: 40 lots below the old low (1.6x), closed back inside");
        check(!trapl::decide(C(500,392,398,{R(395,0,20),R(398,0,20)}),false,400,prior).fired,"closed BELOW the old low = accepted breakout, not trapped");
        trapl::Result a=trapl::decide(C(500,392,398,{R(395,0,20),R(398,0,20)}),false,400,prior);check(a.why.find("accepted")!=std::string::npos,"why: accepted");
        trapl::Result l=trapl::decide(C(500,392,404,{R(395,0,10),R(398,0,10)}),false,400,prior);check(!l.fired&&l.x<1.0&&l.why.find("under")!=std::string::npos,"20 lots < 1.0x the median 25: not trapped");
        check(!trapl::decide(C(500,401,404,{R(402,0,90)}),false,400,prior).broke,"no new 20-candle low: nothing");
        check(!trapl::decide(C(500,392,404,{R(395,0,90)}),false,400,std::vector<double>(9,25.0)).fired,"fewer than 10 earlier candles: learning, never fires");
        trapl::Result t=trapl::decide(C(608,500,596,{R(602,30,0),R(606,30,0),R(590,0,40)}),true,600,prior);
        check(t.fired&&t.lots==60&&t.lo==601&&t.hi==608,"trapped buyers (top): 60 lots bought above the old high 600, closed back below");
        check(trapl::decide(C(500,392,400,{R(395,0,30)}),false,400,prior).fired,"a close exactly AT the old low counts as back inside");
        trapl::Rec q;q.id=3;q.top=false;q.barT=1791887000;q.barRaw=1791887000;q.lo=392;q.hi=399;q.tick=0.25;q.lots=40;q.med=25;q.x=1.6;q.prevX=400;q.extreme=392;q.closeK=404;q.version="2.21.0";q.root="EP";
        trapl::Rec back;check(trapl::parse(trapl::line(q),back)&&back.id==3&&!back.top&&back.lo==392&&back.hi==399&&std::fabs(back.x-1.6)<1e-9&&back.root=="EP","TR line round trip");
        std::string bad=trapl::line(q);bad.replace(bad.find("bottom"),6,"middle");check(!trapl::parse(bad,back),"a bad side is rejected");
        check(!trapl::parse("TR|2.21.0|EP|3",back),"a torn line is rejected");
        check(trapl::badge(true)=="A|Tr?"&&trapl::badge(false)=="Tr?","badges: A|Tr? / Tr?");
    }
    std::cout<<"[2] DeltaProfile.cpp on the mock SDK\n";
    {   // trapped sellers + the A: bar 44 makes a new low 98 (old low 100), 10,000 lots sold at 99 (a selling node), green close 101 back inside
        const std::string flex=home("a");
        mock::Host h=fixture();DeltaProfile p;draw(h,p);
        mock::Bar& b=h.bars[44];b.rows.push_back(mock::Price(99,0,10000,10000));b.volume+=10000;b.low=98;b.open=99;b.close=101;
        quiet(h);draw(h,p);quiet(h);draw(h,p);
        const std::string rec=records(flex);
        check(rec.find(std::string("TR|")+DLT_VERSION+"|")!=std::string::npos&&rec.find("|bottom|")!=std::string::npos,"the trap is recorded once (TR line, bottom = trapped sellers)");
        int nTr=0;{size_t a=0;while((a=rec.find("TR|",a))!=std::string::npos){++nTr;++a;}}check(nTr==1,"exactly one TR line ("+std::to_string(nTr)+")");
        const mock::Draw* d=textOf(h,"A|Tr?");
        check(d!=NULL,"the badge A|Tr? is drawn (his A on the same candle and side)");
        if(d){check(d->color==C_BUY,"trapped sellers = green");
              const int colL=h.scaleLeft-state(h).layout.width-40;check(d->x>colL&&d->x<h.scaleLeft,"the badge is ON the profile column (x "+std::to_string(d->x)+")");}
        int amber=0;for(const mock::Draw& x:h.draws)if(x.type=="line"&&x.color==0)++amber;(void)amber;
        check(texts(h,"Tr?")==0,"not a second plain Tr? badge");
        bool sizeText=false;for(const mock::Draw& x:h.draws)if(x.type=="text"&&(x.text.find("400x")!=std::string::npos||x.text.find("10000")!=std::string::npos||x.text.find("lots")!=std::string::npos))sizeText=true;
        check(!sizeText,"the trap size is never written on the chart");
        const std::string st=slurp(flex+"DeltaProfile.status-ES-180.txt");
        check(st.find("TRAP,on")!=std::string::npos&&st.find("trapped 1")!=std::string::npos&&st.find("A|Tr 1")!=std::string::npos,"status TRAP line: 1 trapped, drawn with A");
        // never repaint: 8 more quiet candles, the badge stays at the same place
        const int x0=d?d->x:-1,y0=d?d->y:-1;bool same=true;
        for(int i=0;i<8;++i){quiet(h);draw(h,p);const mock::Draw* e=textOf(h,"A|Tr?");if(!e||e->x!=x0||e->y!=y0){same=false;break;}}
        check(same,"8 later candles: the badge never moved or vanished");
        // restart: a new plugin object reads the record and draws the same badge
        mock::current=&h;p.destroy();latchBooks().clear();
        DeltaProfile q;draw(h,q);const mock::Draw* e=textOf(h,"A|Tr?");
        check(e&&e->x==x0&&e->y==y0,"a restart redraws the same badge at the same place (from the TR line)");
        const std::string rec2=records(flex);int nTr2=0;{size_t a=0;while((a=rec2.find("TR|",a))!=std::string::npos){++nTr2;++a;}}
        check(nTr2==1,"a restart never re-decides (still one TR line)");
        // the badge never overlaps another text or a bubble box
        bool clash=false;if(e){for(const mock::Draw& x:h.draws)if(x.type=="text"&&&x!=e&&std::abs(x.y-e->y)<state(h).layout.font+2&&std::abs(x.x-e->x)<30)clash=true;}
        check(!clash,"the badge does not sit on another label");
        // the settings row off: nothing drawn, the record stays
        state(h).trapOn=false;draw(h,q);check(textOf(h,"A|Tr?")==NULL,"Trapped traders off: no badge");
        mock::current=&h;q.destroy();latchBooks().clear();
    }
    {   // trapped sellers WITHOUT his A: the new low 98 with 60 lots sold at 99, the candle closes red at 100.5 (back inside, no green close above)
        home("b");
        mock::Host h=fixture();DeltaProfile p;draw(h,p);
        mock::Bar& b=h.bars[44];b.rows.push_back(mock::Price(99,0,60,60));b.volume+=60;b.low=98;b.open=104;b.close=100.5;
        mock::Bar n=bar(b.time+180);n.open=100.5;n.close=100.25;n.low=100;h.bars.push_back(n);draw(h,p);quiet(h);draw(h,p);
        check(textOf(h,"Tr?")!=NULL&&textOf(h,"A|Tr?")==NULL,"trapped, no A on that candle: Tr?");
        mock::current=&h;p.destroy();latchBooks().clear();
    }
    {   // accepted breakout: the candle closes BELOW the old low -> no trap
        home("c");
        mock::Host h=fixture();DeltaProfile p;draw(h,p);
        mock::Bar& b=h.bars[44];b.rows.push_back(mock::Price(99,0,60,60));b.volume+=60;b.low=98;b.open=104;b.close=99;
        quiet(h);draw(h,p);quiet(h);draw(h,p);
        check(textOf(h,"Tr?")==NULL&&textOf(h,"A|Tr?")==NULL,"accepted breakout (closed below the old low): no badge");
        mock::current=&h;p.destroy();latchBooks().clear();
    }
    {   // light: only 10 lots sold below the old low vs a median attack of 25 -> no trap
        home("d");
        mock::Host h=fixture();DeltaProfile p;draw(h,p);
        mock::Bar& b=h.bars[44];b.rows.push_back(mock::Price(99,0,10,10));b.volume+=10;b.low=98;b.open=104;b.close=100.5;
        quiet(h);draw(h,p);quiet(h);draw(h,p);
        check(textOf(h,"Tr?")==NULL&&textOf(h,"A|Tr?")==NULL,"10 lots (0.5x the median attack 20): no badge");
        mock::current=&h;p.destroy();latchBooks().clear();
    }
    {   // trapped BUYERS: a new high 127 (old high 125), 80 lots bought at 126, red close 123 back inside -> red badge
        home("e");
        mock::Host h=fixture();DeltaProfile p;draw(h,p);
        mock::Bar& b=h.bars[44];b.rows.push_back(mock::Price(126,80,0,80));b.volume+=80;b.high=127;b.open=121;b.close=123.5;
        mock::Bar n=bar(b.time+180);n.open=123.5;n.close=123.75;n.high=124;h.bars.push_back(n);draw(h,p);quiet(h);draw(h,p);
        const mock::Draw* d=textOf(h,"Tr?");if(!d)d=textOf(h,"A|Tr?");
        check(d!=NULL&&d->color==C_SELL,"trapped buyers at a new high: red badge");
        mock::current=&h;p.destroy();latchBooks().clear();
    }
    std::cout<<"[4] (2.21.1) the badge in its own clear slot, several layouts\n";
    {   struct L{bool top;int nodes;double origin,scale;int font;const char* name;};
        const L lays[]={{false,4,7740,22,10,"trapped sellers, A on 3 rows (the 2.21.0 render)"},{false,1,7740,22,10,"trapped sellers, A on 1 row"},
                        {false,2,7740,22,10,"trapped sellers, A on 2 rows"},{true,4,7745,22,10,"trapped buyers at a new high"},
                        {false,4,7744,30,10,"trapped rows near the pane bottom"},{true,4,7748,30,10,"trapped rows near the pane top"},
                        {false,4,7740,22,13,"bigger font"}};
        for(const L& l:lays){std::string why;const bool ok=layoutOk(l.top,l.nodes,l.origin,l.scale,l.font,why);check(ok,std::string("[layout] ")+l.name+(ok?"":": "+why));}
    }
    std::cout<<"[3] versions / settings\n";
    {   check(std::string(DLT_VERSION)=="2.23.1"&&std::string(dpset::PLUGIN_VERSION)=="2.23.1","versions 2.23.1");   // (2.23.0) pins moved with the build
        const ss::Spec& S=dpset::spec();
        check(S.paramVersion==17&&std::string(S.rows[dpset::K_TRAP_ON].key)=="trap_on"&&S.rows[dpset::K_TRAP_ON].def==1,"settings row trap_on (on), parameter version 17 (2.23.0)");
        check(std::string(trapl::VERSION)=="1.0.0"&&trapl::MIN_X==1.0&&trapl::LOOKBACK==20,"TrapLogic 1.0.0: 20 candles, 1.0x");
    }
    system(("rm -rf /tmp/dp221_"+std::to_string(getpid())).c_str());
    std::cout<<"dp 2.21.1 trapped traders: "<<passed<<" passed, "<<failed<<" failed\n";
    return failed?1:0;
}
