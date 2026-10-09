#include "../DeltaProfileCore.h"
#include <iostream>
#include <random>
#include <functional>
#include <cstdlib>
using namespace delta_profile;
static int passed = 0;
static void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
static void test(const char* name, const std::function<void()>& fn) {
    try { fn(); ++passed; std::cout << "PASS " << name << '\n'; }
    catch (const std::exception& e) { std::cerr << "FAIL " << name << ": " << e.what() << '\n'; std::exit(1); }
}
static Bar candle(int i, std::int64_t time, double lo, double hi, double cl, bool closed = true) {
    Bar b; b.index = i; b.time = time; b.closed = closed;
    b.low = lo; b.high = hi; b.open = cl; b.close = cl; return b;
}
static std::vector<Bar> held(bool sell, bool up, std::int64_t elapsed = 900, bool lastClosed = true) {
    std::vector<Bar> b;
    Bar seed = candle(0, 0, 99, 101, 100);
    seed.rows.push_back(Row(100, sell ? -100 : 100, 100)); b.push_back(seed);
    b.push_back(candle(1, 60, up ? 100 : 95, up ? 105 : 100, up ? 104 : 96));
    b.push_back(candle(2, 60 + elapsed, up ? 100 : 95, up ? 105 : 100, up ? 104 : 96, lastClosed));
    b[1].open = b[2].open = 100;   // (2.5.0) green bars going up, red bars going down
    return b;
}
int main() {
    test("sell absorption confirmed after 900 seconds", [] { State s = classify(held(true, true),100,100,-100,1); check(s.code == "A" && s.support && s.peakBar == 0,"wrong A"); });
    test("buy absorption is mirrored downward", [] { State s = classify(held(false,false),100,100,100,1); check(s.code == "A" && !s.support,"wrong mirrored A"); });
    test("sell initiative confirmed downward", [] { State s = classify(held(true,false),100,100,-100,1); check(s.code == "I" && !s.support,"wrong I"); });
    test("buy initiative confirmed upward", [] { State s = classify(held(false,true),100,100,100,1); check(s.code == "I" && s.support,"wrong mirrored I"); });
    // (2.5.0) the FIRST closed bar beyond the node decides - no 15-minute hold - and the decision is final
    test("(2.5.0) one closed bar beyond the node confirms at once", [] {
        std::vector<Bar> b = held(true,true,60); b.resize(2);
        State s = classify(b,100,100,-100,1); check(s.code == "A" && s.support && s.decidedBar == 1,"close above did not confirm sell absorption");
    });
    test("(2.5.0) bearish: a close below a heavy-BUYING node = A (buyers absorbed)", [] {
        std::vector<Bar> b; Bar seed = candle(0,0,99,101,100); seed.rows.push_back(Row(100,100,100)); b.push_back(seed);
        b.push_back(candle(1,180,98,101,99)); b[1].open = 100.5;   // a red bar closing below
        State s = classify(b,100,100,100,1); check(s.code == "A" && !s.support,"close below buying node not bearish A");
    });
    test("(2.5.0) a GREEN bar closing below a buying node does not confirm (needs a red bar)", [] {
        std::vector<Bar> b; Bar seed = candle(0,0,99,101,100); seed.rows.push_back(Row(100,100,100)); b.push_back(seed);
        b.push_back(candle(1,180,97,99.5,99)); b[1].open = 97.5;   // closes below the node but up on the bar
        check(classify(b,100,100,100,1).code == "A?","green bar confirmed a bearish absorption");
        b.push_back(candle(2,360,97,99,98)); b[2].open = 99;         // the next RED bar closing below does
        check(classify(b,100,100,100,1).code == "A","red bar below did not confirm");
    });
    test("(2.5.0) a doji (open == close) beyond the node does not decide", [] {
        std::vector<Bar> b = held(true,true,60); b.resize(2); b[1].open = b[1].close;
        check(classify(b,100,100,-100,1).code == "A?","doji decided");
    });
    test("(2.5.0) a forming bar beyond the node does not decide", [] {
        std::vector<Bar> b = held(true,true,60); b.resize(2); b[1].closed = false;
        check(classify(b,100,100,-100,1).code == "A?","forming bar decided");
    });
    test("(2.5.0) a close back inside / at the node edge does not decide", [] {
        std::vector<Bar> b = held(true,true,60); b.resize(2); b[1].close = 100;
        check(classify(b,100,100,-100,1).code == "A?","close at node decided");
    });
    test("(2.5.0) the decision is final: a later close on the other side does not change it", [] {
        std::vector<Bar> b = held(true,true,60); b.resize(2);
        b.push_back(candle(2,360,95,101,96)); b.push_back(candle(3,540,94,99,95)); b[2].open = 100; b[3].open = 98;
        check(classify(b,100,100,-100,1).code == "A","later opposite close changed a decided A");
    });
    test("(2.5.0) the first decisive close wins over later ones (I stays I)", [] {
        std::vector<Bar> b = held(true,false,60); b.resize(2);   // sell node, closes below = I
        b.push_back(candle(2,360,100,106,105)); b[2].open = 101;
        check(classify(b,100,100,-100,1).code == "I","I flipped");
    });
    test("formation restarts at last material contribution", [] {
        std::vector<Bar> b = held(true,true); b[2].rows.push_back(Row(100,-20,20));
        check(classify(b,100,100,-120,1).code == "A?","formation not restarted");
    });
    test("peak ring is hardest contributor not newest minute", [] {
        std::vector<Bar> b = held(true,true); b[0].index=42;
        check(classify(b,100,100,-100,1).peakBar == 42,"bar identity lost");
    });
    test("no contributor above 10 percent yields no node", [] {
        std::vector<Bar> b(20,candle(0,0,99,101,100));
        for (std::size_t i=0;i<b.size();++i) { b[i].rows.push_back(Row(100,-5,5)); b[i].time=static_cast<std::int64_t>(i)*60; }
        check(!classify(b,100,100,-100,1).valid(),"invalid formation");
    });
    test("unknown tick fails closed", [] { check(!classify(held(true,true),100,100,-100,0).valid(),"zero tick accepted"); });
    test("zero delta yields no signal", [] { check(!classify(held(true,true),100,100,0,1).valid(),"zero delta signal"); });
    test("negative prices use mathematical floor buckets", [] { check(floorDiv(-1,3)==-1 && floorDiv(-3,3)==-1 && floorDiv(-4,3)==-2,"wrong negative grouping"); });
    test("decimal tick boundaries convert to exact keys", [] { check(priceKey(4200.1,0.1)==42001 && priceKey(1.12345,0.00005)==22469 && priceKey(-0.01,0.01)==-1,"wrong key"); });
    test("nonfinite tick rejected", [] { bool bad=false; try { priceKey(1,std::numeric_limits<double>::quiet_NaN()); } catch (...) { bad=true; } check(bad,"NaN accepted"); });
    test("unrepresentable tick quotient rejected", [] { bool bad=false; try { priceKey(1e30,1e-8); } catch (...) { bad=true; } check(bad,"overflow accepted"); });
    test("duplicates normalize before classification", [] {
        Bar b=candle(0,0,99,102,100); b.rows.push_back(Row(101,2,2)); b.rows.push_back(Row(100,-3,3)); b.rows.push_back(Row(100,1,1));
        normalize(b); check(b.rows.size()==2 && b.rows[0].key==100 && b.rows[0].delta==-2 && b.rows[0].volume==4,"duplicate merge");
    });
    test("double totals preserve a one-contract addition", [] {
        Bar b=candle(0,0,99,101,100); b.rows.push_back(Row(100,16777216,16777216));
        Bar c=b; c.index=1; c.time=60; c.rows[0]=Row(100,1,1);
        Snapshot s=build(std::vector<Bar>{b,c},1);
        check(s.buckets[0].delta==16777217 && s.buckets[0].volume==16777217,"unit precision lost");
    });
    test("net delta cancels and volume remains", [] {
        Bar b=candle(0,0,99,101,100); b.rows.push_back(Row(100,100,120));
        Bar c=b; c.index=1; c.time=60; c.rows[0]=Row(100,-100,130);
        Snapshot s=build(std::vector<Bar>{b,c},1);
        check(s.buckets[0].delta==0 && s.buckets[0].volume==250 && s.nodes.empty(),"cancellation error");
    });
    test("ratios use all nonzero grouped rows", [] {
        Bar b=candle(0,0,90,110,100);
        b.rows={Row(100,80,80),Row(101,10,10),Row(102,10,10),Row(103,0,50)};
        Snapshot s=build(std::vector<Bar>{b},1);
        check(std::fabs(s.meanAbsDelta-100.0/3)<1e-12 && std::fabs(s.buckets[0].ratio-2.4)<1e-12,"wrong mean");
    });
    test("node and amount ranking have deterministic ties", [] {
        Bar b=candle(0,0,0,50,10); b.rows={Row(10,100,100),Row(11,-100,100),Row(12,1,1),Row(13,1,1),Row(14,1,1)};
        Snapshot s=build(std::vector<Bar>{b},1);
        check(s.nodes.size()==2 && s.nodes[0].bucket==10 && s.nodes[1].bucket==11,"unstable tie");
    });
    test("provisional history cannot expose confirmed nodes", [] {
        std::vector<Bar> b=held(true,true);
        for(int i=0;i<5;++i) b[0].rows.push_back(Row(110+i,1,1));
        normalize(b[0]);
        Snapshot s=build(b,1,false); check(!s.nodes.empty() && s.nodes[0].state.code=="A?","partial history confirmed");
    });
    test("zone classification excludes older 90-minute contributor", [] {
        Bar old=candle(0,0,90,130,100); old.rows={Row(100,200,200),Row(101,200,200)};
        Bar recent=candle(1,2000,90,130,100); recent.rows={Row(100,50,50),Row(101,50,50),Row(110,1,1),Row(120,-1,1),Row(130,-1,1)};
        Bar last=candle(2,4000,100,110,100);   // (2.5.0) closes inside the zone: undecided
        Snapshot s=build(std::vector<Bar>{old,recent,last},1);
        check(!s.zones.empty() && s.zones[0].code=="Acc?","older bar polluted zone state");
    });
    test("(2.4.2) zone rows may skip ONE missing tick (lra.delta_profile.find_zones: gap <= 2 ticks)", [] {
        Bar b=candle(0,0,0,20,10); b.rows={Row(0,100,100),Row(2,100,100),Row(10,1,1),Row(15,-1,1),Row(20,-1,1)};
        Snapshot s=build(std::vector<Bar>{b},1); check(s.zones.size()==1 && s.zones[0].low==0 && s.zones[0].high==2,"one-tick gap split the zone");
    });
    test("(2.4.2) a gap of two missing ticks splits the zone", [] {
        Bar b=candle(0,0,0,20,10); b.rows={Row(0,100,100),Row(3,100,100),Row(10,1,1),Row(15,-1,1),Row(20,-1,1)};
        Snapshot s=build(std::vector<Bar>{b},1); check(s.zones.empty(),"three-tick gap joined");
    });
    test("invalid row rejected rather than partly drawn", [] {
        Bar b=candle(0,0,99,101,100); b.rows.push_back(Row(100,100,1));
        bool bad=false; try { build(std::vector<Bar>{b},1); } catch (...) { bad=true; } check(bad,"invalid delta/volume accepted");
    });
    test("nonchronological bars rejected", [] {
        std::vector<Bar> b=held(true,true); b[2].time=-1;
        bool bad=false; try { build(b,1); } catch (...) { bad=true; } check(bad,"bad time order accepted");
    });
    test("OHLC inconsistency rejected", [] {
        Bar b=candle(0,0,99,101,200);
        bool bad=false; try { build(std::vector<Bar>{b},1); } catch (...) { bad=true; } check(bad,"bad OHLC accepted");
    });
    test("4001 prices are all included", [] {
        Bar b=candle(0,0,0,5000,100);
        for(int i=0;i<4001;++i) b.rows.push_back(Row(i,i==4000?100000:1,i==4000?100000:1));
        Snapshot s=build(std::vector<Bar>{b},1); double total=0; for(const Bucket& k:s.buckets) total+=k.delta;
        check(total==104000,"4001st price omitted");
    });
    test("10000 bars are all included in core", [] {
        std::vector<Bar> b;
        for(int i=0;i<10000;++i) { Bar k=candle(i,i,99,101,100); k.rows.push_back(Row(100,1,1)); b.push_back(k); }
        Snapshot s=build(b,1); check(s.buckets[0].delta==10000,"3000-bar truncation");
    });
    test("randomized grouped delta/volume oracle (200 histories)", [] {
        std::mt19937 rng(20261008);
        for(int trial=0;trial<200;++trial) {
            std::vector<Bar> b; std::map<Tick,std::pair<double,double> > expected;
            for(int i=0;i<30;++i) {
                Bar k=candle(i,i*60,-200,200,0);
                for(int j=0;j<20;++j) {
                    Tick key=static_cast<int>(rng()%300)-150; double buy=rng()%1000, sell=rng()%1000;
                    k.rows.push_back(Row(key,buy-sell,buy+sell)); expected[key].first+=buy-sell; expected[key].second+=buy+sell;
                }
                normalize(k); b.push_back(k);
            }
            Snapshot s=build(b,1); std::map<Tick,std::pair<double,double> > grouped;
            for(const auto& x:expected) { auto& y=grouped[floorDiv(x.first,s.ticksPerRow)]; y.first+=x.second.first; y.second+=x.second.second; }
            check(grouped.size()==s.buckets.size(),"bucket count mismatch");
            for(const Bucket& k:s.buckets) check(k.delta==grouped[k.key].first && k.volume==grouped[k.key].second,"oracle totals mismatch");
            for(const Node& n:s.nodes) check(n.ratio>=2 && n.state.peakBar>=0 && n.state.peakBar<30,"invalid node/ring association");
        }
    });
    std::cout << "RESULT " << passed << " tests passed\n";
}
