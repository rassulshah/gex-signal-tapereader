// Actual plugin + mock SDK integration. Synthetic inputs, not trade-performance tests.
#define main archived_legacy_stage_tests
#include "test_stage_host.cpp"
#undef main
static rse::Event typed(int k, int pane, const char* code, int cause, long long lots=100, long long aux=-1, long long parent=0) {
    rse::Event e; e.key=keyOf(k); e.pane=pane; e.l=lst::make(pane,code,lst::LOW,-1,"synthetic source");
    e.m.typed=true; e.m.basis=rse::MBO; e.m.quality=rse::COMPLETE; e.m.contract="ESZ6";
    e.m.cause=keyOf(cause); e.m.parentCause=parent; e.m.published=keyOf(k)+2;
    e.m.priceTicks=code==std::string("H")||code==std::string("L")?27982:27980;
    e.m.lots=lots; e.m.auxLots=aux; e.m.id=rse::eventId(pane,e.key,e.l,e.m.cause); return e;
}
static void ack(const std::string& flex, int last, int qa=rse::COMPLETE, int qd=rse::COMPLETE) {
    for(int pane=0;pane<2;++pane) {
        std::string text; for(int k=0;k<=last;++k) {rse::Frame f; f.key=keyOf(k);f.pane=pane;f.contract="ESZ6";f.quality=pane==0?qa:qd;f.published=keyOf(k)+1;text+=rse::line(f)+"\n";}
        appendAll(flex+"\\Strips\\"+rse::fileName(pane,"ES","ESZ6",60),text);
    }
}
static void emit(const std::string& flex,const rse::Event& e) {
    appendAll(flex+"\\Strips\\"+rse::fileName(e.pane,"ES",e.m.contract,60),rse::line(e)+"\n");
}
// (2.0.0) the gate: a fresh LonLO key level at the candle-10 low (6995.00) so the attempt is at a meaningful location
static std::string fresh() {char t[]="/tmp/rspXXXXXX"; const char* p=mkdtemp(t);setenv("USERPROFILE",p,1);const std::string f=std::string(p)+"\\InvestorRT\\rtx\\lsFlexLevels";
    writeAll(f+"\\LRA-KeyLevels-ES.csv","KEYLV,1,"+std::to_string((long long)std::time(nullptr))+"\nL|LonLO|6995.500000|0\n");return f;}
static void advance(mock::Host& h,int last,int after=25) {const auto d=day();h.bars.assign(d.begin(),d.begin()+last+2);h.now=keyOf(last)+after;rs_test_now_ms+=5000;}
int main() {
    mock::Host h; mock::current=&h;h.tick=.25f;h.seconds=60;h.symbol="ESZ6";h.root="ES";h.period="1 min";
    h.paneTop=0;h.paneBottom=260;h.paneRight=900;h.scaleLeft=880;h.ppb=20;h.barW=20;
    std::string flex=fresh();advance(h,10,10);ack(flex,10);emit(flex,typed(10,lst::ATK,"K",10));emit(flex,typed(10,lst::DEF,"A",10));
    Ind* p=make();p->calc(0);auto* S=p->stateFor();
    CHECK(S&&S->steps.empty()&&S->provisional,"cannot confirm before 20-second close settlement");
    advance(h,10);p->calc(0);S=p->stateFor();const std::string rec=S->recPath,journal=S->journalPath;
    CHECK(S->steps.size()==1&&S->steps.count(keyOf(10)),"cold start records only latest confirmed close, not old history");
    CHECK(S->now.stage==2&&S->now.resume.gate=="LonLO"&&S->now.quality==rse::COMPLETE&&!S->provisional,"typed K plus A at the gate confirms Collision");
    CHECK(S->now.decided==(long long)h.now&&readAll(journal).find("RJ|1|")!=std::string::npos,"actual decision and first receipt journaled");
    CHECK(std::string(RS_VERSION)=="2.0.0"&&std::string(rsl::VERSION)=="2.0.0"&&std::string(rsset::PLUGIN_VERSION)=="2.0.0","versions synchronized");
    advance(h,12);ack(flex,12);auto H=typed(12,lst::DEF,"H",11,100,50,keyOf(10));emit(flex,H);p->calc(0);S=p->stateFor();
    CHECK(S->now.stage==4&&S->now.resume.anchor==(long long)keyOf(11),"native next-candle turn links earlier pivot and establishes true turn cause");
    CHECK(!S->steps.count(keyOf(11))&&S->steps.count(keyOf(12)),"missed decisions are not falsely backfilled as live confirmations");
    const auto fixed12=S->steps.at(keyOf(12));const std::string before=readAll(rec);
    auto L=typed(12,lst::DEF,"L",11,40,20);L.m.published=h.now;emit(flex,L);
    rs_test_now_ms+=5000;p->calc(0);CHECK(readAll(rec)==before&&S->steps.at(keyOf(12))==fixed12,"late retest cannot rewrite already confirmed candle");
    advance(h,13);ack(flex,13);p->calc(0);S=p->stateFor();
    CHECK(S->now.stage==4&&(S->now.flags&rsl::F_RETEST_EXECUTED_HELD)&&S->now.decided==(long long)h.now,"late executed-held retest applies at the next actual decision, not its old key");
    CHECK(readAll(journal).find("RJ|1|"+std::to_string(keyOf(13)))!=std::string::npos,"late evidence effective key persisted");
    const auto finalStep=S->now;const std::string stable=readAll(rec);
    p->draw();bool confirmed=false,defense=false,weakening=false;
    for(const auto& d:h.draws)if(d.type=="text"){confirmed|=d.text.find("Confirmed ")==0;defense|=d.text=="2 Collision";weakening|=d.text=="3 Trapped";}
    if(!(confirmed&&defense&&weakening)) for(const auto& d:h.draws)if(d.type=="text")std::printf("DRAW %s\n",d.text.c_str());
    CHECK(confirmed&&defense&&weakening,"display distinguishes confirmed phase and uses honest stage names");
    dispose(p);h.data=nullptr;p=make();rs_test_now_ms+=5000;p->calc(0);S=p->stateFor();
    CHECK(readAll(rec)==stable&&S->now==finalStep,"restart restores exact immutable step and checkpoint");
    std::remove((flex+"\\Strips\\"+rse::fileName(lst::ATK,"ES","ESZ6",60)).c_str());
    rs_test_now_ms+=5000;p->calc(0);
    const std::string live=readAll(S->recPath+".status.txt");
    CHECK(live.find("QUALITY,unavailable")!=std::string::npos&&live.find("ACHIEVED_QUALITY,complete")!=std::string::npos,"status separates current source loss from achieved phase");
    CHECK(readAll(rec)==stable,"source loss never rewrites committed history");
    dispose(p);h.data=nullptr;
    // Full-quality letters cannot manufacture complete source-frame coverage.
    flex=fresh();advance(h,10);ack(flex,10,rse::UNVERIFIED,rse::COMPLETE);emit(flex,typed(10,lst::ATK,"K",10));emit(flex,typed(10,lst::DEF,"A",10));
    p=make();p->calc(0);S=p->stateFor();CHECK(S->now.stage==0&&S->now.readiness!=rsl::READ_CONFIRMED,"unverified required source cannot confirm a phase");dispose(p);h.data=nullptr;
    // Missing acknowledgements and legacy-only files are pending, not neutral holds.
    flex=fresh();advance(h,10);writeAll(flex+"\\Strips\\ATK-ES-60.csv",lst::line(lst::ATK,keyOf(10),lst::make(lst::ATK,"K",lst::LOW,-1))+"\n");
    p=make();p->calc(0);S=p->stateFor();CHECK(S->steps.empty()&&S->provisional,"legacy-only letters do not become new MBO confirmations");
    ack(flex,10);rs_test_now_ms+=5000;p->calc(0);S=p->stateFor();CHECK(S->now.stage==0,"legacy K is not relabeled typed MBO");dispose(p);h.data=nullptr;
    // Separate contracts cannot adopt one another's phases or records.
    flex=fresh();advance(h,10);ack(flex,10);emit(flex,typed(10,lst::ATK,"K",10));h.symbol="ESH7";
    p=make();p->calc(0);S=p->stateFor();CHECK(S->steps.empty()&&S->evs.empty()&&S->recPath.find("ESH7")!=std::string::npos,"contract-isolated source and record paths");dispose(p);h.data=nullptr;h.symbol="ESZ6";
    std::printf("Typed stage pipeline: %d passed, %d failed\n",passes,fails);return fails?1:0;
}
