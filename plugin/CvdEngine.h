#pragma once
// CvdEngine.h -- lsCVD signal core 1.0.0 (from the outside package CVD Signals v0.1, reviewed 2026-10-12).
// Portable C++17, no SDK calls, no orders. LRA review changes (each has a test in tests/test_cvd_review.cpp):
//   R1 every price comparison is made in whole ticks on prices snapped to the tick grid (float IRT prices; 0.1 / 0.0005 / 0.00005 ticks)
//   R2 event IDs are anchor:seq:type:dir:level - the same event gets the same ID however much history the chart loaded (no global counter)
// Signal meaning is unchanged from v0.1 (SIGNAL_SPEC.md).
#define CVD_ENGINE_VERSION "1.0.0"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace cvd {
constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
enum class Direction { Bull = 1, Bear = -1 };
enum class Type { Div, Abs, Exh, BR };
enum class State { Pending, Confirmed, Expired, Invalidated };
inline std::string name(Type t) {
    switch(t) { case Type::Div:return "Div"; case Type::Abs:return "Abs";
        case Type::Exh:return "Exh"; case Type::BR:return "BR"; }
    return "";
}
inline std::string name(State s) {
    switch(s) { case State::Pending:return "pending"; case State::Confirmed:return "confirmed";
        case State::Expired:return "expired"; case State::Invalidated:return "invalidated"; }
    return "";
}
struct Level {
    std::string id;
    double price = 0;
    bool support = true; // support for bullish reversals, resistance for bearish reversals
    std::int64_t available_seq = 0; // adapter must not supply levels before they are knowable
};
struct Bar {
    std::int64_t seq = 0;
    std::string timestamp, anchor;
    double open=0, high=0, low=0, close=0;
    double buy=0, sell=0, total=0, seconds=60;
    bool complete=true, quality=true;
    std::vector<Level> levels;
};
struct Config {
    // Research starting points, NOT optimized values or instrument-specific recommendations.
    double tick_size=.25;
    int lookback=20, pivot_left=2, pivot_right=2, max_swing_gap=60;
    int stall_window=3, exhaust_window=3, expiry_bars=8, cooldown_bars=5;
    double level_tolerance_ticks=3, price_extension_ticks=1;
    double invalidate_ticks=2, confirm_ticks=1, rearm_ticks=1;
    double min_relative_volume=.5, min_total_volume=1;
    double divergence_delta_units=1, scale_floor=1;
    double absorption_side_multiplier=1.5, absorption_delta_pct=15;
    double stall_progress_ticks=3, stall_range_ticks=8;
    double exhaust_rate_ratio=.65, exhaust_prior_side_share=.55;
    double breakout_delta_pct=12, confirmation_delta_pct=5, confirmation_cvd_units=.5;
    bool enable_div=true, enable_abs=true, enable_exh=true, enable_br=true;
};
struct Event {
    std::string id, timestamp, anchor, level_id, label, reason;
    std::int64_t seq=0, warning_seq=0, source_seq=0;
    Type type=Type::Div;
    Direction direction=Direction::Bull;
    State state=State::Pending;
    double cvd=NaN, level=0, trigger=0, invalidation=0;
    bool plot=true;
};
struct Output {
    std::int64_t seq=0;
    double cvd=NaN, delta=NaN, delta_pct=NaN;
    bool accepted=false, ready=false, tainted=false;
    std::vector<Event> events;
};
class Engine {
    struct Sample { Bar b; double cvd; };
    struct Pivot { std::size_t index; bool low; };
    struct Pending {
        Event warning;
        double extreme=0, scale=1;
        bool retested=false;
        std::int64_t retest_seq=-1;
        double retest_cvd=0;
    };
    struct Latch { std::string key; std::int64_t terminal; double extreme; };
    Config cfg_;
    std::vector<Sample> h_;
    std::vector<Pivot> lows_, highs_;
    std::vector<Pending> pending_;
    std::vector<Latch> latches_;
    std::string anchor_;
    std::int64_t last_seq_=-1;
    double cumulative_=0;
    bool tainted_=false;

    static double median(std::vector<double> v) {
        if(v.empty()) return 0;
        std::sort(v.begin(),v.end());
        std::size_t n=v.size();
        return n%2 ? v[n/2] : (v[n/2-1]+v[n/2])/2;
    }
    double tick(double n) const { return n*cfg_.tick_size; }
    // R1: (a - b) in whole ticks. Inputs are on the tick grid (push() snaps them), so the rounding is exact.
    long long dt(double a,double b) const { return std::llround((a-b)/cfg_.tick_size); }
    double snap(double p) const { return std::round(p/cfg_.tick_size)*cfg_.tick_size; }
    int sign(Direction d) const { return static_cast<int>(d); }
    std::string key(const std::string& id,Direction d) const {
        return id+(d==Direction::Bull?"|bull":"|bear");
    }
    double med(std::size_t end,int field) const {
        std::vector<double> v;
        std::size_t start=end>static_cast<std::size_t>(cfg_.lookback)?end-cfg_.lookback:0;
        for(std::size_t i=start;i<end;++i) {
            const auto& b=h_[i].b;
            v.push_back(field==0?b.total:field==1?std::abs(b.buy-b.sell):field==2?b.buy:b.sell);
        }
        return median(v);
    }
    double scale(std::size_t end) const { return std::max(cfg_.scale_floor,med(end,1)); }
    bool volume_ok(const Bar& b,std::size_t end) const {
        return b.total>0 && b.total>=cfg_.min_total_volume && b.total>=cfg_.min_relative_volume*med(end,0);
    }
    bool touch(const Bar& b,const Level& l) const {
        return l.available_seq<=b.seq && dt(b.low,l.price)<=cfg_.level_tolerance_ticks
            && dt(b.high,l.price)>=-cfg_.level_tolerance_ticks;
    }
    const Level* level_for(const Bar& b,Direction d,double extreme) const {
        const Level* best=nullptr;
        for(const auto& l:b.levels) {
            if(l.support!=(d==Direction::Bull) || !touch(b,l)) continue;
            if(!best || std::llabs(dt(l.price,extreme))<std::llabs(dt(best->price,extreme))
                || (std::llabs(dt(l.price,extreme))==std::llabs(dt(best->price,extreme)) && l.id<best->id)) best=&l;
        }
        return best;
    }
    Event transition(const Pending& p,const Bar& b,State state,const std::string& why,double y,bool plot=true) {
        Event e=p.warning;
        e.seq=b.seq; e.timestamp=b.timestamp; e.state=state; e.reason=why; e.cvd=y; e.plot=plot;
        e.label=name(e.type)+(state==State::Pending?"?":state==State::Confirmed?"":"×");
        return e;
    }
    void latch(const Pending& p,std::int64_t seq) {
        const auto k=key(p.warning.level_id,p.warning.direction);
        for(auto& l:latches_) if(l.key==k) { l.terminal=seq; l.extreme=p.extreme; return; }
        latches_.push_back({k,seq,p.extreme});
    }
    bool can_arm(const Level& l,Direction d,double extreme,Type t,const Bar& b) const {
        const auto k=key(l.id,d);
        for(const auto& p:pending_) if(key(p.warning.level_id,p.warning.direction)==k) return false;
        for(const auto& x:latches_) if(x.key==k) {
            if(b.seq-x.terminal<=cfg_.cooldown_bars) return false;
            if(t!=Type::BR && sign(d)*dt(extreme,x.extreme)>-cfg_.rearm_ticks) return false;
        }
        return true;
    }
    bool arm(Type t,Direction d,const Level& l,double extreme,double trigger,double invalidation,
             std::int64_t source_seq,double sc,const Bar& b,Output& out) {
        if(!can_arm(l,d,extreme,t,b)) return false;
        // Reject a warning already invalidated at its recognition bar.
        if((d==Direction::Bull && dt(b.low,invalidation)<=0)||(d==Direction::Bear && dt(b.high,invalidation)>=0)) return false;
        for(const auto& e:out.events) if(e.state==State::Pending && e.direction==d) return false;
        Event e;
        e.id=anchor_+":"+std::to_string(b.seq)+":"+name(t)+":"+(d==Direction::Bull?"bull":"bear")+":"+l.id;   // R2
        e.seq=e.warning_seq=b.seq; e.source_seq=source_seq; e.timestamp=b.timestamp;
        e.anchor=anchor_; e.type=t; e.direction=d; e.state=State::Pending;
        e.level_id=l.id; e.level=l.price; e.trigger=trigger; e.invalidation=invalidation;
        e.cvd=cumulative_; e.label=name(t)+"?"; e.reason="candidate";
        pending_.push_back({e,extreme,sc,false,-1,0}); out.events.push_back(e); return true;
    }
    void lifecycle(const Bar& b,Output& out) {
        std::vector<Pending> keep;
        for(auto p:pending_) {
            const auto d=p.warning.direction;
            State state=State::Pending; std::string why;
            // Conservative OHLC ordering: invalidation wins over confirmation on ambiguous bars.
            if((d==Direction::Bull && dt(b.low,p.warning.invalidation)<=0)
                ||(d==Direction::Bear && dt(b.high,p.warning.invalidation)>=0)) {
                state=State::Invalidated; why="price_invalidation";
            } else if(b.seq-p.warning.warning_seq>cfg_.expiry_bars) {
                state=State::Expired; why="timeout";
            } else {
                if(p.warning.type==Type::BR && !p.retested) {
                    const double tol=cfg_.level_tolerance_ticks;
                    const bool tested=dt(b.low,p.warning.level)<=tol && dt(b.high,p.warning.level)>=-tol;
                    const bool held=sign(d)*dt(b.close,p.warning.level)>=0;
                    if(tested && held) {
                        p.retested=true; p.retest_seq=b.seq; p.retest_cvd=cumulative_;
                        p.warning.trigger=(d==Direction::Bull?b.high:b.low)+sign(d)*tick(cfg_.confirm_ticks);
                    }
                }
                const bool eligible=p.warning.type!=Type::BR || (p.retested && b.seq>p.retest_seq);
                const double base=p.warning.type==Type::BR?p.retest_cvd:p.warning.cvd;
                const bool price=sign(d)*dt(b.close,p.warning.trigger)>0;
                const bool flow=sign(d)*out.delta_pct>=cfg_.confirmation_delta_pct
                    && sign(d)*(cumulative_-base)>=cfg_.confirmation_cvd_units*p.scale;
                if(eligible && price && flow && volume_ok(b,h_.size()-1)) {
                    state=State::Confirmed; why="price_and_flow";
                }
            }
            if(state==State::Pending) keep.push_back(p);
            else { out.events.push_back(transition(p,b,state,why,cumulative_)); latch(p,b.seq); }
        }
        pending_=std::move(keep);
    }
    void pivots(const Bar& b,Output& out,bool ready) {
        const auto n=h_.size();
        const auto L=static_cast<std::size_t>(cfg_.pivot_left),R=static_cast<std::size_t>(cfg_.pivot_right);
        if(n<L+R+1) return;
        const std::size_t j=n-R-1;
        bool low=true,high=true;
        for(std::size_t k=j-L;k<=j+R;++k) if(k!=j) {
            low=low && dt(h_[j].b.low,h_[k].b.low)<0;
            high=high && dt(h_[j].b.high,h_[k].b.high)>0;
        }
        for(bool islow:{true,false}) {
            if(!(islow?low:high)) continue;
            auto& ps=islow?lows_:highs_;
            const Direction d=islow?Direction::Bull:Direction::Bear;
            const double extreme=islow?h_[j].b.low:h_[j].b.high;
            if(ready && cfg_.enable_div && !ps.empty() && volume_ok(h_[j].b,j)) {
                const auto q=ps.back().index;
                const double prev=islow?h_[q].b.low:h_[q].b.high;
                const bool extension=sign(d)*dt(extreme,prev)<=-cfg_.price_extension_ticks;
                const bool divergence=sign(d)*(h_[j].cvd-h_[q].cvd)>=cfg_.divergence_delta_units*scale(n-1);
                const bool gap=h_[j].b.seq-h_[q].b.seq<=cfg_.max_swing_gap;
                const auto* l=level_for(h_[j].b,d,extreme);
                if(extension && divergence && gap && l) {
                    arm(Type::Div,d,*l,extreme,prev+sign(d)*tick(cfg_.confirm_ticks),
                        extreme-sign(d)*tick(cfg_.invalidate_ticks),h_[j].b.seq,scale(n-1),b,out);
                }
            }
            ps.push_back({j,islow});
        }
    }
    void absorption(const Bar& b,Output& out) {
        const auto n=h_.size(), w=static_cast<std::size_t>(cfg_.stall_window);
        if(!cfg_.enable_abs || n<w+1) return;
        const auto start=n-w;
        double hi=-std::numeric_limits<double>::infinity(),lo=std::numeric_limits<double>::infinity();
        double buys=0,sells=0,total=0;
        for(std::size_t i=start;i<n;++i) {
            hi=std::max(hi,h_[i].b.high); lo=std::min(lo,h_[i].b.low);
            buys+=h_[i].b.buy; sells+=h_[i].b.sell; total+=h_[i].b.total;
        }
        if(total<=0 || dt(hi,lo)>cfg_.stall_range_ticks
            || std::llabs(dt(b.close,h_[start-1].b.close))>cfg_.stall_progress_ticks) return;
        for(Direction d:{Direction::Bull,Direction::Bear}) {
            const double side=d==Direction::Bull?sells:buys;
            const double baseline=med(start,d==Direction::Bull?3:2);
            const double pct=100*(buys-sells)/total;
            const double extreme=d==Direction::Bull?lo:hi;
            const auto* l=level_for(b,d,extreme);
            if(l && side>=w*baseline*cfg_.absorption_side_multiplier
                && -sign(d)*pct>=cfg_.absorption_delta_pct && volume_ok(b,n-1)) {
                arm(Type::Abs,d,*l,extreme,(d==Direction::Bull?hi:lo)+sign(d)*tick(cfg_.confirm_ticks),
                    extreme-sign(d)*tick(cfg_.invalidate_ticks),b.seq,scale(n-1),b,out);
            }
        }
    }
    void exhaustion(const Bar& b,Output& out) {
        const auto n=h_.size(), w=static_cast<std::size_t>(cfg_.exhaust_window);
        if(!cfg_.enable_exh || n<2*w+1) return;
        const auto start=n-2*w,mid=n-w;
        for(Direction d:{Direction::Bull,Direction::Bear}) {
            double oldside=0,newside=0,oldtime=0,newtime=0,oldtotal=0,newtotal=0;
            double oldext=d==Direction::Bull?std::numeric_limits<double>::infinity():-std::numeric_limits<double>::infinity();
            double newext=oldext,hi=-std::numeric_limits<double>::infinity(),lo=std::numeric_limits<double>::infinity();
            for(std::size_t i=start;i<n;++i) {
                const auto& z=h_[i].b; const double side=d==Direction::Bull?z.sell:z.buy;
                const double ext=d==Direction::Bull?z.low:z.high;
                if(i<mid) { oldside+=side; oldtime+=z.seconds; oldtotal+=z.total;
                    oldext=d==Direction::Bull?std::min(oldext,ext):std::max(oldext,ext); }
                else { newside+=side; newtime+=z.seconds; newtotal+=z.total;
                    newext=d==Direction::Bull?std::min(newext,ext):std::max(newext,ext);
                    hi=std::max(hi,z.high); lo=std::min(lo,z.low); }
            }
            const auto* l=level_for(b,d,newext);
            if(l && oldside>0 && oldtotal>0 && newside>0
                && oldside/oldtotal>=cfg_.exhaust_prior_side_share
                && newside/newtime<=cfg_.exhaust_rate_ratio*(oldside/oldtime)
                && newtotal>=cfg_.min_total_volume*w
                && newtotal>=cfg_.min_relative_volume*w*med(start,0)
                && sign(d)*dt(newext,oldext)<=-cfg_.price_extension_ticks) {
                arm(Type::Exh,d,*l,newext,(d==Direction::Bull?hi:lo)+sign(d)*tick(cfg_.confirm_ticks),
                    newext-sign(d)*tick(cfg_.invalidate_ticks),b.seq,scale(n-1),b,out);
            }
        }
    }
    void breakout(const Bar& b,Output& out) {
        const auto n=h_.size(); if(!cfg_.enable_br || n<2 || !volume_ok(b,n-1)) return;
        const auto& prev=h_[n-2].b;
        for(const auto& l:b.levels) {
            if(l.available_seq>prev.seq) continue; // level must predate the crossing
            const Direction d=l.support?Direction::Bear:Direction::Bull;
            const double threshold=l.price+sign(d)*tick(cfg_.confirm_ticks);
            if(sign(d)*dt(prev.close,threshold)<=0 && sign(d)*dt(b.close,threshold)>0
                && sign(d)*out.delta_pct>=cfg_.breakout_delta_pct) {
                const double extreme=d==Direction::Bull?b.low:b.high;
                arm(Type::BR,d,l,extreme,threshold,l.price-sign(d)*tick(cfg_.invalidate_ticks),
                    b.seq,scale(n-1),b,out);
            }
        }
    }
    static void validate(const Bar& b) {
        const auto finite=[](double x){return std::isfinite(x);};
        if(b.seq<0 || b.anchor.empty() || b.timestamp.empty()) throw std::invalid_argument("seq/anchor/timestamp required");
        if(!finite(b.open)||!finite(b.high)||!finite(b.low)||!finite(b.close)
            || b.high<b.low || b.high<std::max(b.open,b.close) || b.low>std::min(b.open,b.close))
            throw std::invalid_argument("invalid OHLC");
        if(!finite(b.buy)||!finite(b.sell)||!finite(b.total)||!finite(b.seconds)
            ||b.buy<0||b.sell<0||b.total<0||b.seconds<=0
            ||b.buy+b.sell>b.total+1e-8*std::max(1.0,b.total))
            throw std::invalid_argument("invalid volume/duration");
        std::vector<std::string> ids;
        for(const auto& l:b.levels) {
            if(l.id.empty()||!finite(l.price)||l.available_seq<0) throw std::invalid_argument("invalid level");
            if(std::find(ids.begin(),ids.end(),l.id)!=ids.end()) throw std::invalid_argument("duplicate level id");
            ids.push_back(l.id);
        }
    }
public:
    explicit Engine(Config c=Config{}):cfg_(c) {
        const double params[]={c.tick_size,c.level_tolerance_ticks,c.price_extension_ticks,c.invalidate_ticks,
            c.confirm_ticks,c.rearm_ticks,c.min_relative_volume,c.min_total_volume,c.divergence_delta_units,
            c.scale_floor,c.absorption_side_multiplier,c.absorption_delta_pct,c.stall_progress_ticks,
            c.stall_range_ticks,c.exhaust_rate_ratio,c.exhaust_prior_side_share,c.breakout_delta_pct,
            c.confirmation_delta_pct,c.confirmation_cvd_units};
        for(double p:params) if(!std::isfinite(p)||p<0) throw std::invalid_argument("nonnegative finite config required");
        if(c.tick_size<=0||c.scale_floor<=0||c.lookback<1||c.pivot_left<1||c.pivot_right<1
            ||c.max_swing_gap<1||c.stall_window<1||c.exhaust_window<1||c.expiry_bars<1||c.cooldown_bars<0
            ||c.exhaust_rate_ratio>1||c.exhaust_prior_side_share>1
            ||c.absorption_delta_pct>100||c.breakout_delta_pct>100||c.confirmation_delta_pct>100)
            throw std::invalid_argument("invalid config bounds");
    }
    const Config& config() const { return cfg_; }
    std::size_t pending_count() const { return pending_.size(); }
    Output push(const Bar& in) {
        Output out; out.seq=in.seq;
        if(!in.complete) return out; // preview bars never mutate the committed engine
        validate(in);
        Bar b=in;   // R1: snap prices and levels onto the tick grid once; every later comparison is in whole ticks
        b.open=snap(b.open); b.high=snap(b.high); b.low=snap(b.low); b.close=snap(b.close);
        for(auto& l:b.levels) l.price=snap(l.price);
        if(last_seq_>=0 && b.seq!=last_seq_+1) throw std::invalid_argument("completed bars must have contiguous increasing seq; replay required");
        if(anchor_!=b.anchor) {
            for(const auto& p:pending_) out.events.push_back(transition(p,b,State::Expired,"anchor_reset",NaN,false));
            pending_.clear(); latches_.clear(); h_.clear(); lows_.clear(); highs_.clear();
            anchor_=b.anchor; cumulative_=0; tainted_=false;
        }
        last_seq_=b.seq; out.accepted=true;
        if(!b.quality || tainted_) {
            if(!tainted_) {
                for(const auto& p:pending_) out.events.push_back(transition(p,b,State::Invalidated,"data_gap",NaN,false));
                pending_.clear();
            }
            tainted_=true; out.tainted=true; return out;
        }
        out.delta=b.buy-b.sell; cumulative_+=out.delta;
        out.cvd=cumulative_; out.delta_pct=b.total>0?100*out.delta/b.total:NaN;
        h_.push_back({b,cumulative_});
        lifecycle(b,out);
        out.ready=h_.size()>static_cast<std::size_t>(cfg_.lookback) && b.total>0;
        pivots(b,out,out.ready);
        if(out.ready) { absorption(b,out); exhaustion(b,out); breakout(b,out); }
        return out;
    }
};
} // namespace cvd
