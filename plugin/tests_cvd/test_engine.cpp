#include "CvdEngine.h"
#include <functional>
#include <iostream>
#include <sstream>

using namespace cvd;
void require(bool ok,const std::string& what) { if(!ok) throw std::runtime_error(what); }
Config base() {
    Config c; c.lookback=2; c.pivot_left=1; c.pivot_right=1;
    c.enable_div=c.enable_abs=c.enable_exh=c.enable_br=false;
    c.min_relative_volume=0; c.min_total_volume=1; c.level_tolerance_ticks=4;
    c.expiry_bars=4; c.confirmation_cvd_units=.1; c.confirmation_delta_pct=5;
    return c;
}
Bar bar(int seq,double close,double delta=0,std::string anchor="S1") {
    Bar b; b.seq=seq; b.timestamp="T"+std::to_string(seq); b.anchor=anchor;
    b.open=b.close=close; b.high=close+.25; b.low=close-.25;
    b.total=200; b.buy=(b.total+delta)/2; b.sell=(b.total-delta)/2;
    return b;
}
void level(Bar& b,double price,bool support=true) { b.levels.push_back({"L",price,support,0}); }
std::vector<Bar> abs_fixture() {
    auto a=bar(0,100,0), b=bar(1,100,0), c=bar(2,100,-160),d=bar(3,100,-160);
    level(c,99.75); level(d,99.75); return {a,b,c,d};
}
Config abs_config() { auto c=base(); c.enable_abs=true; c.stall_window=2; return c; }
std::vector<Event> run(Engine& e,const std::vector<Bar>& bs) {
    std::vector<Event> events;
    for(const auto& b:bs) { auto o=e.push(b); events.insert(events.end(),o.events.begin(),o.events.end()); }
    return events;
}
bool has(const std::vector<Event>& es,Type t,State s,Direction d=Direction::Bull) {
    for(const auto& e:es) if(e.type==t&&e.state==s&&e.direction==d) return true;
    return false;
}
std::string signature(const Event& e) {
    std::ostringstream o; o<<e.id<<'|'<<e.seq<<'|'<<e.warning_seq<<'|'<<e.source_seq
      <<'|'<<name(e.type)<<'|'<<name(e.state)<<'|'<<e.label<<'|'<<e.cvd;
    return o.str();
}
int main() {
    int count=0;
    auto test=[&](const std::string& name,const std::function<void()>& f) {
        try { f(); ++count; std::cout<<"PASS "<<name<<'\n'; }
        catch(const std::exception& e) { std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n'; throw; }
    };
    try {
        test("CVD and delta percentage arithmetic",[]{
            Engine e(base()); auto a=e.push(bar(0,100,40)), b=e.push(bar(1,100,-60));
            require(a.cvd==40&&a.delta_pct==20&&b.cvd==-20&&b.delta_pct==-30,"arithmetic mismatch");
        });
        test("partial bars never commit",[]{
            Engine e(base()); auto b=bar(0,100,40); b.complete=false;
            require(!e.push(b).accepted,"partial accepted"); b.complete=true;
            require(e.push(b).cvd==40,"partial mutated CVD");
        });
        test("duplicate/out-of-order committed bars rejected",[]{
            Engine e(base()); e.push(bar(0,100)); bool thrown=false;
            try { e.push(bar(0,100)); } catch(const std::invalid_argument&) { thrown=true; }
            require(thrown,"duplicate accepted");
        });
        test("invalid input fails without advancing state",[]{
            Engine e(base()); auto b=bar(0,100); b.buy=201; bool thrown=false;
            try { e.push(b); } catch(const std::invalid_argument&) { thrown=true; }
            require(thrown,"bad volume accepted"); require(e.push(bar(0,100,20)).cvd==20,"state advanced");
        });
        test("zero volume produces unavailable delta percentage",[]{
            Engine e(base()); auto b=bar(0,100); b.buy=b.sell=b.total=0;
            auto o=e.push(b); require(std::isnan(o.delta_pct)&&!o.ready&&o.cvd==0,"zero treated as signal");
        });
        test("absorption warning and later confirmation",[]{
            Engine e(abs_config()); auto es=run(e,abs_fixture());
            require(has(es,Type::Abs,State::Pending),"no Abs?");
            auto o=e.push(bar(4,100.75,100)); require(has(o.events,Type::Abs,State::Confirmed),"no Abs confirmation");
            require(es.back().seq==3&&o.events[0].seq==4&&o.events[0].warning_seq==3,"timestamps wrong");
        });
        test("bearish absorption uses bearish setup direction",[]{
            Engine e(abs_config()); auto bs=abs_fixture();
            for(auto& b:bs) { std::swap(b.buy,b.sell); for(auto& l:b.levels) { l.price=100.25; l.support=false; } }
            auto es=run(e,bs); require(has(es,Type::Abs,State::Pending,Direction::Bear),"no bearish Abs?");
            auto o=e.push(bar(4,99.25,-100)); require(has(o.events,Type::Abs,State::Confirmed,Direction::Bear),"no bearish Abs");
        });
        test("invalidation wins over confirmation on ambiguous OHLC",[]{
            Engine e(abs_config()); run(e,abs_fixture()); auto b=bar(4,100.75,100); b.low=99;
            auto o=e.push(b); require(has(o.events,Type::Abs,State::Invalidated),"did not invalidate first");
            require(!has(o.events,Type::Abs,State::Confirmed),"confirmed invalid bar");
        });
        test("minimum volume also gates confirmation",[]{
            auto c=abs_config(); c.min_total_volume=50; Engine e(c); run(e,abs_fixture());
            auto b=bar(4,100.75); b.total=10; b.buy=10; b.sell=0;
            require(!has(e.push(b).events,Type::Abs,State::Confirmed),"low volume confirmed");
        });
        test("expiry deadline is inclusive and terminal event immutable",[]{
            auto c=abs_config(); c.expiry_bars=2; Engine e(c); auto es=run(e,abs_fixture());
            auto saved=signature(es.back());
            require(e.push(bar(4,100)).events.empty(),"expired too early");
            require(e.push(bar(5,100)).events.empty(),"deadline not inclusive");
            auto o=e.push(bar(6,100)); require(has(o.events,Type::Abs,State::Expired),"no expiry");
            require(signature(es.back())==saved,"earlier warning mutated");
        });
        test("anchor reset ends pending without cross-anchor plot",[]{
            Engine e(abs_config()); run(e,abs_fixture()); auto o=e.push(bar(4,100,20,"S2"));
            require(o.cvd==20&&e.pending_count()==0,"reset failed");
            require(o.events.size()==1&&o.events[0].reason=="anchor_reset"&&!o.events[0].plot,"reset marker wrong");
        });
        test("data gap taints anchor until a fresh anchor",[]{
            Engine e(abs_config()); run(e,abs_fixture()); auto b=bar(4,100); b.quality=false;
            auto o=e.push(b); require(o.tainted&&std::isnan(o.cvd),"gap not tainted");
            require(o.events.size()==1&&!o.events[0].plot,"gap cancellation absent");
            require(e.push(bar(5,100,50)).tainted,"resumed without recovery");
            require(e.push(bar(6,100,50,"S2")).cvd==50,"new anchor not recovered");
        });
        test("future level is never used",[]{
            Engine e(abs_config()); auto bs=abs_fixture();
            for(auto& b:bs) for(auto& l:b.levels) l.available_seq=100;
            require(run(e,bs).empty(),"used future level");
        });
        test("no level means no signal",[]{
            Engine e(abs_config()); auto bs=abs_fixture(); for(auto& b:bs) b.levels.clear();
            require(run(e,bs).empty(),"location filter bypassed");
        });
        test("one active warning per level and direction",[]{
            Engine e(abs_config()); auto es=run(e,abs_fixture()); auto b=bar(4,100,-160); level(b,99.75);
            auto o=e.push(b); require(o.events.empty()&&e.pending_count()==1,"duplicate warning");
        });
        test("cooldown and fresh-extreme requirement suppress repeated setup",[]{
            Engine e(abs_config()); run(e,abs_fixture()); e.push(bar(4,100.75,100));
            int emitted=0;
            for(int i=5;i<15;++i) { auto b=bar(i,100,-160); level(b,99.75); emitted+=e.push(b).events.size(); }
            require(emitted==0,"repeated unchanged extreme rearmed");
        });
        test("divergence recognized after pivot-right bars, never backdated",[]{
            auto c=base(); c.enable_div=true; c.divergence_delta_units=.2; Engine e(c);
            std::vector<Bar> bs={bar(0,102,0),bar(1,100,-100),bar(2,102,150),bar(3,99.5,-20),bar(4,100,0)};
            level(bs[3],99.5); auto es=run(e,bs);
            require(has(es,Type::Div,State::Pending),"no divergence");
            require(es.back().seq==4&&es.back().source_seq==3,"pivot backdated");
            require(has(e.push(bar(5,101,100)).events,Type::Div,State::Confirmed),"no Div confirmation");
        });
        test("equal lows are not strict pivots",[]{
            auto c=base(); c.enable_div=true; Engine e(c);
            std::vector<Bar> bs={bar(0,101),bar(1,100),bar(2,100),bar(3,101)};
            for(auto& b:bs) level(b,100);
            require(run(e,bs).empty(),"plateau became pivot");
        });
        test("exhaustion requires falling directional volume rate",[]{
            auto c=base(); c.enable_exh=true; c.exhaust_window=2; Engine e(c);
            std::vector<Bar> bs={bar(0,103),bar(1,103),bar(2,102,-120),bar(3,101.5,-120),bar(4,101,-20),bar(5,100.75,-20)};
            for(int i=4;i<=5;++i) { bs[i].sell=40; bs[i].buy=20; bs[i].total=60; level(bs[i],100.5); }
            auto es=run(e,bs); require(has(es,Type::Exh,State::Pending),"no Exh?");
            require(has(e.push(bar(6,101.75,100)).events,Type::Exh,State::Confirmed),"no Exh confirmation");
        });
        test("balanced heavy volume is not exhaustion",[]{
            auto c=base(); c.enable_exh=true; c.exhaust_window=2; Engine e(c);
            std::vector<Bar> bs={bar(0,103),bar(1,103),bar(2,102,-120),bar(3,101.5,-120),bar(4,101,0),bar(5,100.75,0)};
            for(int i=4;i<=5;++i) { bs[i].buy=bs[i].sell=300; bs[i].total=600; level(bs[i],100.5); }
            require(run(e,bs).empty(),"net delta zero misread as exhaustion");
        });
        test("breakout needs retest then a later bounce bar",[]{
            auto c=base(); c.enable_br=true; Engine e(c);
            std::vector<Bar> bs={bar(0,99.5),bar(1,100),bar(2,100.75,100)};
            for(auto& b:bs) level(b,100,false);
            auto es=run(e,bs); require(has(es,Type::BR,State::Pending),"no BR?");
            auto retest=bar(3,100.25,20); retest.low=100;
            require(!has(e.push(retest).events,Type::BR,State::Confirmed),"same-bar retest confirmed");
            require(has(e.push(bar(4,101,100)).events,Type::BR,State::Confirmed),"no later BR confirmation");
        });
        test("prefix causality and fresh-engine replay parity",[]{
            auto c=abs_config(); auto bs=abs_fixture(); bs.push_back(bar(4,100.75,100));
            Engine full(c); auto all=run(full,bs);
            for(std::size_t n=1;n<=bs.size();++n) {
                Engine e(c); auto part=run(e,std::vector<Bar>(bs.begin(),bs.begin()+n));
                std::vector<Event> expected; for(const auto& z:all) if(z.seq<static_cast<int>(n)) expected.push_back(z);
                require(part.size()==expected.size(),"prefix event count differs");
                for(std::size_t j=0;j<part.size();++j) require(signature(part[j])==signature(expected[j]),"prefix changed history");
            }
            Engine replay(c); auto twice=run(replay,bs); require(twice.size()==all.size(),"replay count");
            for(std::size_t j=0;j<all.size();++j) require(signature(all[j])==signature(twice[j]),"replay mismatch");
        });
        test("bad config rejected",[]{
            auto c=base(); c.tick_size=0; bool thrown=false;
            try { Engine e(c); } catch(const std::invalid_argument&) { thrown=true; }
            require(thrown,"invalid config accepted");
        });
        test("zero-volume divergence source prohibited even with numeric gates disabled",[]{
            auto c=base(); c.enable_div=true; c.divergence_delta_units=.2;
            c.min_total_volume=0; c.min_relative_volume=0; Engine e(c);
            std::vector<Bar> bs={bar(0,102,0),bar(1,100,-100),bar(2,102,150),bar(3,99.5,0),bar(4,100,0)};
            bs[3].buy=bs[3].sell=bs[3].total=0; level(bs[3],99.5);
            require(run(e,bs).empty(),"zero source pivot armed Div");
        });
        test("exhaustion requires a preceding baseline bar",[]{
            auto c=base(); c.enable_exh=true; c.exhaust_window=2; c.min_relative_volume=.5; Engine e(c);
            std::vector<Bar> bs={bar(0,102,-120),bar(1,101.5,-120),bar(2,101,-20),bar(3,100.75,-20)};
            for(int i=2;i<=3;++i) { bs[i].sell=40; bs[i].buy=20; bs[i].total=60; level(bs[i],100.5); }
            require(run(e,bs).empty(),"exhaustion armed without baseline history");
        });
        test("absorption directional baseline independent of CVD scale floor",[]{
            auto c=abs_config(); c.scale_floor=1000; Engine e(c);
            require(has(run(e,abs_fixture()),Type::Abs,State::Pending),"CVD scale floor changed absorption baseline");
        });
        std::cout<<"TOTAL "<<count<<" tests passed\n";
    } catch(...) { return 1; }
}
