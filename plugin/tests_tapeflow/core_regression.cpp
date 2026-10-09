// Fabricated engineering regressions only; no trading-edge or SDK assertions.
#include <vector>
#include <deque>
#include <map>
#include <string>
#include <cmath>
#include <climits>
#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <locale>
#include <set>
#include <unordered_set>
#include <iostream>
#include <type_traits>
// White-box state regressions, without modifying the shipped core API.
#define private public
#include "../TapeFlowLogic.h"
#undef private

static int checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { std::cerr << __LINE__ << ": " #x " failed\n"; std::abort(); } } while (0)
using namespace tfl;
static Tick tick(long long t, int px = 100, int bid = 100, int ask = 102, long long q = 10) {
    Tick k; k.t=t; k.px=px; k.bid=bid; k.ask=ask; k.q=q; return k;
}
static BinBase baseline() {
    BinBase b; b.n=100; b.med20=5; b.med5=5; b.medSpread=2;
    for(int h=0;h<8;++h) b.qb[h]=b.qs[h]=100;
    return b;
}
static void feed(Engine& e, long long first, int n, int px=100, int bid=100, int ask=102, long long q=10) {
    for(int k=0;k<n;++k) CHECK(e.add(tick(first+k,px,bid,ask,q)));
    e.advanceTo(first+n-1);
}
static Slice slice(long long t, int mid=202) {
    Slice s; s.t=t; s.path=s.carried=true; s.spread=2; s.mids.push_back(mid); return s;
}
static Win win(long long endT, float rate=10) {
    Win w; w.endT=endT; w.bin=binOf(endT); w.rate20=rate; w.spread=2;
    for(int h=0;h<8;++h) { w.Mb[h]=rate*5; w.Ms[h]=rate*6; }
    for(int k=0;k<4;++k) w.r5[k]=rate;
    return w;
}
static void statsAndDense() {
    for(int n=0;n<250;++n) {
        std::vector<float> v; for(int k=0;k<n;++k) v.push_back((float)((k*73+n*11)%29));
        std::vector<float> sorted=v; std::sort(sorted.begin(),sorted.end());
        std::vector<float> copy=v;
        float expected=n ? (n%2 ? sorted[n/2] : (sorted[n/2-1]+sorted[n/2])*0.5f) : 0;
        CHECK(median(copy)==expected);
        for(double p : {0.0,0.5,0.95,1.0}) {
            copy=v; int k=n ? std::max(0,std::min(n-1,(int)std::ceil(p*n)-1)) : 0;
            CHECK(nearestRank(copy,p)==(n ? sorted[k] : 0));
        }
    }
    std::vector<float> huge(2,std::numeric_limits<float>::max()); CHECK(std::isfinite(median(huge)));
    std::deque<Slice> r(1); r[0].rows.push_back(Row{100,10,20,5}); r[0].rows.push_back(Row{104,30,0,0});
    Dense d; d.build(r,0,1); CHECK(d.valid);
    long long b,s,u; d.band(99,104,&b,&s,&u); CHECK(b==40&&s==20&&u==5);
    int center; long long volume; double cv; CHECK(d.best(SIDE_BUY,1,0.9,202,&center,&volume,&cv)); CHECK(center==104&&volume==30);
    r[0].rows.push_back(Row{4100,0,1000,0}); d.build(r,0,1); CHECK(d.valid); // exact 4000 span allowed
    r[0].rows.push_back(Row{4101,0,100000,0}); d.build(r,0,1); CHECK(!d.valid); CHECK(!d.best(SIDE_SELL,1,0.9,202,&center,&volume,&cv));
    d.band(INT_MIN,INT_MAX,&b,&s,&u); CHECK(b==0&&s==0&&u==0);
    d.build(r,2,1); CHECK(!d.valid);
    r.clear(); d.build(r,0,0); CHECK(d.valid&&!d.best(SIDE_BUY,1,0.9,0,0,0,0));
    r.resize(1); r[0].rows.push_back(Row{INT_MIN,1,0,0}); r[0].rows.push_back(Row{INT_MAX,1,0,0}); d.build(r,0,1); CHECK(!d.valid);
    r[0].rows.clear(); r[0].rows.push_back(Row{100,LLONG_MAX,0,0}); r[0].rows.push_back(Row{101,1,0,0}); d.build(r,0,1); CHECK(!d.valid);
}
static void configAndInput() {
    Cfg c; std::string why; CHECK(c.validate(&why)&&why.empty());
    for(int bad=0;bad<12;++bad) {
        Cfg x=c;
        switch(bad) { case 0:x.part=6;break;case 1:x.obsW=24;break;case 2:x.hMin=0;break;case 3:x.hMax=9;break;case 4:x.ctxW=0;break;case 5:x.sessionsBack=0;break;case 6:x.keepSec=-1;break;case 7:x.cov=NAN;break;case 8:x.quoteAge=-1;break;case 9:x.hMin=8;x.hMax=1;break;case 10:x.inAct=INFINITY;break;case 11:x.holdS=x.inTTL;break; }
        CHECK(!x.validate(&why)&&!why.empty()); Engine e; e.cfg=x; CHECK(!e.add(tick(1000))); CHECK(e.hist.empty());
    }
    CHECK(sideOf(102,100,102)==SIDE_BUY); CHECK(sideOf(100,100,102)==SIDE_SELL); CHECK(sideOf(101,100,102)==SIDE_UNK);
    CHECK(sideOf(100,100,100)==SIDE_UNK); CHECK(sideOf(100,102,100)==SIDE_UNK); CHECK(sideOf(100,0,102)==SIDE_UNK); CHECK(sideOf(INT_MAX,INT_MAX-1,INT_MAX)==SIDE_UNK);
    Engine e; CHECK(!e.add(tick(100,INT_MAX))); CHECK(!e.add(tick(LLONG_MAX)));
    CHECK(e.add(tick(0))); e.advanceTo(0); CHECK(!e.add(tick(0))); CHECK(e.late==1); // t=0 is committed, not a sentinel
    static_assert(std::is_same<decltype(SecRec().b),long long>::value,"exact b");
    static_assert(std::is_same<decltype(SecRec().s),long long>::value,"exact s");
    static_assert(std::is_same<decltype(SecRec().u),long long>::value,"exact u");
    Engine exact; CHECK(exact.add(tick(1000,102,100,102,16777217LL))); CHECK(exact.add(tick(1000,100,100,102,16777219LL))); CHECK(exact.add(tick(1000,101,100,102,16777221LL))); exact.advanceTo(1000);
    CHECK(exact.hist.back().b==16777217LL&&exact.hist.back().s==16777219LL&&exact.hist.back().u==16777221LL);
    Engine lateEngine;lateEngine.setFixedBase(baseline());feed(lateEngine,1500,5);lateEngine.ep[0].state=2;lateEngine.ep[0].dir=1;lateEngine.ep[0].fireT=1504;
    CHECK(!lateEngine.add(tick(1504)));CHECK(!lateEngine.R.empty()&&lateEngine.late==1);   // (1.1.2) a late trade is counted and left out; history and warm-up are keptCHECK(lateEngine.evs.back().kind=="DINV"&&lateEngine.evs.back().t==1504);
    Engine overflow; CHECK(overflow.add(tick(1000,100,100,102,LLONG_MAX))); CHECK(!overflow.add(tick(1000,100,100,102,1))); CHECK(overflow.R.empty()&&!overflow.open);
}
static void quotesWarmGap() {
    Engine e; e.setFixedBase(baseline()); e.cfg.warm=3;
    feed(e,1000,4); CHECK(e.warmN==3&&e.last.quoteOk); CHECK(e.hist.back().flags&SR_WARM);
    CHECK(e.add(tick(1004,100,0,0))); CHECK(e.add(tick(1004,100,100,102))); e.advanceTo(1004);
    CHECK(!e.last.quoteOk&&e.warmN==0); CHECK(e.R.back().mids.size()>0); CHECK(!e.R.back().path);
    feed(e,1005,3); CHECK(e.warmN==3&&e.last.quoteOk);
    CHECK(e.add(tick(1008,100,100,100))); e.advanceTo(1008); CHECK(!e.last.quoteOk&&e.R.back().U==10);
    e.advanceTo(1009); CHECK(e.R.back().mids.empty()); CHECK(e.warmN==0);
    feed(e,1010,2); CHECK(!(e.hist[e.hist.size()-2].flags & (SR_WARM | SR_QUALITY))); // first recovery second has no carry
    CHECK(e.last.quoteOk);
    Engine wide;wide.cfg.warm=1;wide.setFixedBase(baseline());feed(wide,4000,2);
    CHECK(wide.add(tick(4002,100,100,120)));CHECK(wide.add(tick(4002,100,100,102)));wide.advanceTo(4002);CHECK(wide.R.back().spread==20&&!wide.last.spreadOk);
    { const int w0=e.warmN;   // (integration) a merely quiet / stale stretch PAUSES the warm-up instead of restarting it (thin markets)
    e.advanceTo(1017); CHECK(!e.last.quoteOk&&e.warmN>=w0); CHECK(e.lastClass<1017-4); const int w1=e.warmN;
    CHECK(e.add(tick(1018))); e.advanceTo(1018); CHECK(!e.last.quoteOk&&e.warmN==w1); // stale carry is never replaced
    CHECK(e.add(tick(1019))); e.advanceTo(1019); CHECK(e.last.quoteOk&&e.warmN==std::min(w1+1,86400)); }
    e.ep[0].state=2; e.ep[0].dir=1; e.ep[0].id=1; e.ep[0].t0=1010; e.ep[0].fireT=1010;
    e.latches.push_back(Latch()); e.inLatched[0]=true; e.inQuiet[0]=4; e.inS[0].on=true; e.balance=true;
    e.gap(1020,"test gap"); CHECK(e.evs.back().kind=="DINV"); CHECK(e.evs.back().knownAt==e.evs.back().t+1); CHECK(e.latches.empty()&&!e.inLatched[0]&&!e.inS[0].on&&!e.balance);
    CHECK(e.lastClass==LLONG_MIN&&e.quoteT==LLONG_MIN&&e.spreadNow==-1&&!e.haveMid&&e.warmN==0);
    CHECK(!e.add(tick(1019))); CHECK(e.add(tick(1021,101,100,102))); e.advanceTo(1021); CHECK(!e.last.classRecent&&!e.last.quoteOk);
}
static void rawPathsAndFrozen() {
    Engine e; e.cfg.warm=1; e.setFixedBase(baseline()); feed(e,1000,221);
    CHECK(e.last.rangeOk); Feat f=e.last; Engine::Raw r=e.rawFor(1,f,nullptr); CHECK(r.pass&&r.qzone==200&&r.parts==4);
    e.R[e.R.size()-8].path=false; r=e.rawFor(1,f,nullptr); CHECK(!r.pass&&r.why=="incomplete observation midpoint path"); e.R[e.R.size()-8].path=true;
    f.rangeOk=false; CHECK(!e.rawFor(1,f,nullptr).pass);
    Ep fr; fr.dir=1; fr.lo=99;fr.hi=101;fr.h=1;fr.q95=100;fr.med20=5;fr.med5=5;fr.medSpread=2;fr.baseN=100;fr.entry2=202;fr.ext2=202;
    f.baseOk=false;f.bbv=BinBase();f.spreadCal=false;f.spreadOk=false; CHECK(e.rawFor(1,f,&fr).pass); // frozen candidate not current bin
    fr.medSpread=-1; CHECK(!e.rawFor(1,f,&fr).pass); fr.medSpread=2;
    e.R[10].path=false; Feat ff=e.features(e.lastT); CHECK(!ff.rangeOk); CHECK(!e.rawFor(1,ff,nullptr).pass); // context hole cannot shrink band silently
    Engine spread; spread.cfg.warm=1; BinBase b=baseline();b.medSpread=-1;spread.setFixedBase(b);feed(spread,2000,221); CHECK(!spread.rawFor(1,spread.last,nullptr).pass); CHECK(!(spread.hist.back().flags&SR_QUALITY));
    spread.base[spread.last.bin].medSpread=0.1f; spread.spreadNow=10; spread.R.back().spread=10; CHECK(!spread.features(spread.lastT).spreadOk);
}
static void persistenceAndCohorts() {
    long long start=17*3600+86400*10; Win w=win(start+19); long long sid=sessionOf(w.endT), parsedSid=-1; std::string symbol;
    w.rate20=std::nextafter(1.0f,2.0f); w.Mb[7]=std::numeric_limits<float>::max();
    std::string bigSym(3000,'Z'); std::string line=storeLine(sid,bigSym,w); CHECK(line.size()>3000&&line.compare(0,3,"W2|")==0);
    Win parsed; CHECK(parseStoreLine(line,&parsedSid,&symbol,&parsed)); CHECK(parsedSid==sid&&symbol==bigSym&&parsed.rate20==w.rate20&&parsed.Mb[7]==w.Mb[7]&&parsed.endT==w.endT);
    CHECK(storeLine(sid,"bad|symbol",w).empty()); CHECK(!parseStoreLine(line+"|junk",&parsedSid,&symbol,&parsed));
    long long savedSid=parsedSid; Win saved=parsed; std::string savedSymbol=symbol; CHECK(!parseStoreLine("W2|bad",&parsedSid,&symbol,&parsed)); CHECK(parsedSid==savedSid&&parsed.endT==saved.endT&&symbol==savedSymbol);
    Win old=w;old.endT=LLONG_MIN; line=storeLine(sid,"OLD",old); CHECK(line.compare(0,2,"W|")==0); CHECK(parseStoreLine(line,&parsedSid,&symbol,&parsed)); CHECK(parsed.endT==LLONG_MIN);
    std::vector<Win> dst{w}, src{w,win(start+39),win(start+59)};src[0].rate20=777; mergeWindows(dst,src); CHECK(dst.size()==3&&dst[0].rate20==w.rate20); mergeWindows(dst,src); CHECK(dst.size()==3);
    std::vector<Win> ambiguous{old,old}; mergeWindows(ambiguous,{}); CHECK(ambiguous.empty());
    Store st; for(int k=0;k<10;++k) st[sid]["ACTIVE"].push_back(win(start+19+k*20,10)); st[sid]["THIN"].push_back(win(start+19,50));
    CHECK(frontOf(st[sid])==&st[sid]["ACTIVE"]); // total 2000 vs 1000, not mean 10 vs 50
    st[sid]["ACTIVE"].push_back(st[sid]["ACTIVE"][0]); st[sid]["ACTIVE"].push_back(old);
    BinBase out[NBINS];int used; freezeBase(st,sid+1,10,out,&used);CHECK(used==1&&out[0].n==10&&out[0].med20==10);
    freezeBase(st,sid+1,10,out,&used,"THIN",false);CHECK(used==1&&out[0].n==1&&out[0].med20==50);
    freezeBase(st,sid,10,out,&used);CHECK(used==0&&out[0].n==0); // current/future excluded
    freezeBase(st,sid+1,10,out,&used,"MISSING",false);CHECK(used==0&&out[0].n==0);
    Store legacy;legacy[sid]["ACTIVE"]={old,old};freezeBase(legacy,sid+1,10,out,&used);CHECK(used==0&&out[0].n==0);
    Store merged;mergeStore(merged,st);mergeStore(merged,st);CHECK(merged[sid]["ACTIVE"].size()==10&&merged[sid]["THIN"].size()==1);
    Engine e; e.attach(&merged,"ACTIVE");e.curSid=sid;e.sessWins={win(start+219)};e.flushSession();CHECK(merged[sid]["ACTIVE"].size()==11);e.flushSession();CHECK(merged[sid]["ACTIVE"].size()==11);
    e.sessWins={win(start+239)};e.flushSession();CHECK(merged[sid]["ACTIVE"].size()==12); // equal-sized disjoint partial windows merge
    e.cfg.frontRoll=false;e.curSid=sid+1;e.refreeze();CHECK(e.base[0].n==12);
    e.noFlushSid=e.curSid;e.sessWins={win(start+86400+19)};e.flushSession();CHECK(merged.count(sid+1)==0);
}
static void holdsAndRearm() {
    Engine e;e.setFixedBase(baseline());e.cfg.warm=220;feed(e,1000,221);
    // Candidate creation second is never counted in its three-second hold.
    CHECK(e.episode(0).state==1&&e.episode(0).holdN==0);
    for(int k=0;k<2;++k) {CHECK(e.add(tick(1221+k)));e.advanceTo(1221+k);CHECK(e.episode(0).state==1&&e.episode(0).holdN==k+1);}
    CHECK(e.add(tick(1223)));e.advanceTo(1223);CHECK(e.episode(0).state==2&&e.evs.back().kind=="AW");
    CHECK(e.add(tick(1224,100,0,0)));e.advanceTo(1224);CHECK(e.episode(0).state==0&&e.evs.back().kind=="DINV");
    Engine latch;latch.cfg.inRearm=2;latch.inLatched[0]=true;latch.inQuiet[0]=1;latch.cfg.warm=1;latch.setFixedBase(baseline());
    feed(latch,2000,35,102);CHECK(latch.inLatched[0]); // +100 flow is NOT the +/-15 neutral band
    latch.R.clear(); for(int k=0;k<200;++k) {Slice q=slice(3000+k);q.B=q.S=10;q.rows.push_back(Row{100,10,10,0});latch.R.push_back(q);}
    latch.warmN=200;latch.haveMid=true;latch.quoteT=3199;latch.mid2=202;latch.spreadNow=2;latch.lastClass=3199;latch.evaluate(3199);CHECK(latch.inLatched[0]);latch.R.back().t=3200;latch.quoteT=latch.lastClass=3200;latch.evaluate(3200);CHECK(!latch.inLatched[0]);
    Latch l;l.dir=1;l.lo=99;l.hi=101;l.h=1;l.q95=100;l.movedAway=true;l.quietN=29;latch.latches={l};latch.R.back().path=false;latch.updateLatches(3200);CHECK(latch.latches.size()==1&&!latch.latches[0].movedAway&&latch.latches[0].quietN==0);
}
static void sessionsAndInvariance() {
    CHECK(sessionOf(17*3600-1)==0&&sessionOf(17*3600)==1);CHECK(binOf(17*3600)==0&&binOf(17*3600-1)==287);CHECK(binOf(-1)>=0);
    Engine session;session.cfg.warm=1;session.setFixedBase(baseline());long long boundary=86400+17*3600;feed(session,boundary-30,29);CHECK(session.warmN>0);
    session.ep[0].state=2;session.ep[0].dir=1;session.ep[0].fireT=boundary-1;session.inLatched[0]=true;
    CHECK(session.add(tick(boundary)));session.advanceTo(boundary);CHECK(!session.last.quoteOk&&session.warmN==0&&!session.inLatched[0]);CHECK(session.episode(0).state==0);
    Engine a,b;a.cfg.warm=b.cfg.warm=1;a.setFixedBase(baseline());b.setFixedBase(baseline());
    for(int k=0;k<260;++k) {CHECK(a.add(tick(5000+k)));CHECK(b.add(tick(5000+k)));a.advanceTo(5000+k);}
    b.advanceTo(5259);CHECK(a.hist.size()==b.hist.size()&&a.evs.size()==b.evs.size());
    for(size_t k=0;k<a.hist.size();++k) CHECK(a.hist[k].t==b.hist[k].t&&a.hist[k].flags==b.hist[k].flags&&a.hist[k].b==b.hist[k].b&&a.hist[k].s==b.hist[k].s);
    for(size_t k=0;k<a.evs.size();++k) CHECK(a.evs[k].t==b.evs[k].t&&a.evs[k].kind==b.evs[k].kind&&a.evs[k].ep==b.evs[k].ep);
    a.advanceTo(6000);for(int t=5260;t<=6000;++t)b.advanceTo(t);CHECK(a.hist.size()==b.hist.size()&&a.evs.size()==b.evs.size()&&a.warmN==0&&b.warmN==0);
    CHECK(a.add(tick(6001))&&b.add(tick(6001)));a.advanceTo(6001);b.advanceTo(6001);CHECK(!a.last.quoteOk&&!b.last.quoteOk&&a.lastClass==b.lastClass);
    Engine halted;halted.setFixedBase(baseline());feed(halted,17*3600-10,2);halted.advanceTo(17*3600+100000000LL);CHECK(!halted.open&&halted.stopped&&halted.hist.size()<=10);CHECK(halted.add(tick(17*3600+100000001LL)));halted.advanceTo(17*3600+100000001LL);CHECK(!halted.last.quoteOk&&halted.warmN==0);
    Store windows;Engine w;w.attach(&windows,"ES");long long start=17*3600+86400*20;feed(w,start,61);w.flushSession();CHECK(windows[sessionOf(start)]["ES"].size()==2); // first window lacks carried first-second quote
    for(const Win& v:windows[sessionOf(start)]["ES"])CHECK(identified(v));
}

static void seedHistory(Engine& e, long long t, int sd, int mid) {
    e.setFixedBase(baseline());e.cfg.warm=1;e.warmN=200;e.R.clear();e.retainedQty=0;
    for(int k=0;k<200;++k) {Slice q=slice(t-200+k,mid);q.rows.push_back(Row{mid/2,sd==SIDE_BUY?10LL:0LL,sd==SIDE_SELL?10LL:0LL,0});q.B=sd==SIDE_BUY?10:0;q.S=sd==SIDE_SELL?10:0;e.R.push_back(q);e.retainedQty+=10;}
    e.haveMid=true;e.mid2=mid;e.quoteT=e.lastClass=t-1;e.spreadNow=2;
}
static void evalSynthetic(Engine& e, long long t, int mid, int sd, long long volume, int px) {
    Slice q=slice(t,mid);if(volume)q.rows.push_back(Row{px,sd==SIDE_BUY?volume:0,sd==SIDE_SELL?volume:0,sd==SIDE_UNK?volume:0});
    q.B=sd==SIDE_BUY?volume:0;q.S=sd==SIDE_SELL?volume:0;q.U=sd==SIDE_UNK?volume:0;
    e.R.push_back(q);e.retainedQty+=volume;
    while(e.R.size()>205) {e.retainedQty-=e.R.front().B+e.R.front().S+e.R.front().U;e.R.pop_front();}
    e.haveMid=true;e.mid2=mid;e.quoteT=t;e.spreadNow=2;if(volume&&sd!=SIDE_UNK)e.lastClass=t;e.evaluate(t);
}
static Ep watch(int dir,long long t) {
    Ep e;e.id=11;e.dir=dir;e.state=2;e.lo=99;e.hi=101;e.h=1;e.q95=100;e.med20=e.med5=5;e.medSpread=2;e.baseN=100;e.t0=t-1;e.fireT=t-1;e.f30aw=dir>0?-50:50;e.entry2=e.ext2=200;return e;
}
static int eventsOf(const Engine& e,const char* kind) {int n=0;for(const Ev& v:e.evs)if(v.kind==kind)++n;return n;}
static void responseFailureInitiative() {
    for(int dir : {1,-1}) {
        int index=dir>0?0:1;int responseSide=dir>0?SIDE_BUY:SIDE_SELL;
        Engine ar;seedHistory(ar,1000,responseSide,200);ar.ep[index]=watch(dir,1000);
        // Current-bin calibration cannot revoke a frozen response baseline.
        ar.base[binOf(1000)]=BinBase();
        int mid=dir>0?206:194, px=dir>0?103:97;
        evalSynthetic(ar,1000,mid,responseSide,10,px);CHECK(ar.ep[index].arN==1&&eventsOf(ar,"AR")==0);
        evalSynthetic(ar,1001,mid,responseSide,10,px);CHECK(ar.ep[index].arN==2&&eventsOf(ar,"AR")==0);
        evalSynthetic(ar,1002,mid,responseSide,10,px);CHECK(eventsOf(ar,"AR")==1&&ar.ep[index].state==0);CHECK(ar.evs.back().dir==dir&&ar.evs.back().knownAt==1003);
        Engine quoteOnly;seedHistory(quoteOnly,2000,responseSide,200);quoteOnly.ep[index]=watch(dir,2000);
        evalSynthetic(quoteOnly,2000,mid,responseSide,10,px);evalSynthetic(quoteOnly,2001,mid,responseSide,10,px);evalSynthetic(quoteOnly,2002,mid,responseSide,0,px);
        CHECK(eventsOf(quoteOnly,"AR")==0&&quoteOnly.ep[index].state==2); // final-second execution required
        evalSynthetic(quoteOnly,2003,mid,responseSide,10,px);CHECK(eventsOf(quoteOnly,"AR")==1);
        int attempted=dir>0?SIDE_SELL:SIDE_BUY;int adverseMid=dir>0?194:206,adversePx=dir>0?97:103;
        for(int kind=0;kind<3;++kind) {
            Engine fail;seedHistory(fail,3000,attempted,200);fail.ep[index]=watch(dir,3000);
            if(kind==2)fail.ep[index].state=1;
            for(int k=0;k<3;++k)evalSynthetic(fail,3000+k,adverseMid,attempted,kind==1?0:10,adversePx);
            CHECK(eventsOf(fail,kind==0?"FAIL":kind==1?"INVP":"CBRK")==1&&fail.ep[index].state==0);
        }
        Engine expiry;seedHistory(expiry,4000,attempted,200);expiry.ep[index]=watch(dir,4000);expiry.ep[index].fireT=4000-expiry.cfg.watchTTL;expiry.ep[index].breachN=2;expiry.ep[index].breachQty=100;
        evalSynthetic(expiry,4000,adverseMid,attempted,10,adversePx);CHECK(eventsOf(expiry,"EXP")==1&&eventsOf(expiry,"FAIL")==0);
        Engine initiative;seedHistory(initiative,5000,responseSide,200);
        evalSynthetic(initiative,5000,200,responseSide,10,dir>0?101:99);CHECK(initiative.inS[index].on&&initiative.inS[index].n==0);
        int acceptMid=dir>0?204:196,acceptPx=dir>0?103:97;
        evalSynthetic(initiative,5001,acceptMid,responseSide,10,acceptPx);CHECK(initiative.inS[index].n==1&&eventsOf(initiative,"IN")==0);
        evalSynthetic(initiative,5002,acceptMid,responseSide,10,acceptPx);CHECK(initiative.inS[index].n==2&&eventsOf(initiative,"IN")==0);
        evalSynthetic(initiative,5003,acceptMid,responseSide,0,acceptPx);CHECK(eventsOf(initiative,"IN")==0);
        evalSynthetic(initiative,5004,acceptMid,responseSide,10,acceptPx);CHECK(eventsOf(initiative,"IN")==1&&initiative.evs.back().dir==dir);
    }
    Engine bull,bear;bull.cfg.warm=bear.cfg.warm=220;bull.setFixedBase(baseline());bear.setFixedBase(baseline());
    feed(bull,7000,224,100,100,102);feed(bear,7000,224,100,98,100);
    CHECK(eventsOf(bull,"CAND")==1&&eventsOf(bear,"CAND")==1&&eventsOf(bull,"AW")==1&&eventsOf(bear,"AW")==1);
    CHECK(bull.episode(0).state==2&&bear.episode(1).state==2);CHECK(bull.episode(0).lo+bear.episode(1).hi==200&&bull.episode(0).hi+bear.episode(1).lo==200);
}

int main() {
    statsAndDense();configAndInput();quotesWarmGap();rawPathsAndFrozen();persistenceAndCohorts();holdsAndRearm();sessionsAndInvariance();responseFailureInitiative();
    std::cout << "PASS: " << checks << " fabricated core regression checks\n";
}
