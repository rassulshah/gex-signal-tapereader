#ifndef EX2_EXHAUSTION_LOGIC_H
#define EX2_EXHAUSTION_LOGIC_H

// SDK-free causal exhaustion core.  This is intentionally independent from the
// legacy RTX indicator: references and atrPrior arrive already causal from the
// caller and are never imputed here.

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace ex2 {

inline double nan() { return std::numeric_limits<double>::quiet_NaN(); }
inline bool finite(double x) { return std::isfinite(x); }
inline bool positive(double x) { return finite(x) && x > 0.0; }

struct Config {
    std::string mode = "pullback";
    int pullback_n = 3, trend_ema = 20, trend_slope_bars = 3;
    double trend_min_slope_atr = .05;
    int n = 10;
    int smooth = 2;
    int look = 20;
    double fade_threshold = 2.0;
    double min_peak_norm = 1.5;
    double min_current_norm = 0.20;
    double max_current_norm = 1.0;
    double min_total_norm = 0.5;
    double min_coverage = 0.9;
    int min_peak_age = 1;
    double min_leg_atr = 0.75;
    double min_rejection = 0.20;
    double max_extension_atr = 0.35;
    int rearm_bars = 3;
    double rearm_fraction = 0.7;
    int confirmation_bars = 3;
    double confirm_close_fraction = 0.5;
    double confirm_opposing_share = 0.5;
    bool require_confirmation = true;

    bool validate(std::string* why = 0) const {
        const auto fail = [why](const char* text) { if (why) *why = text; return false; };
        if (n < 1 || smooth < 1 || look < 1 || min_peak_age < 1 || rearm_bars < 1 || confirmation_bars < 1)
            return fail("integer fields must be >= 1");
        if (mode != "pullback" && mode != "reversal") return fail("invalid mode");
        if (pullback_n < 1 || pullback_n > look || trend_ema < 2 || trend_slope_bars < 1 || !positive(trend_min_slope_atr)) return fail("invalid context settings");
        if (look > 10000 || smooth > 10000 || trend_ema > 10000 || trend_slope_bars > 10000) return fail("unbounded configuration");
        if (look < n) return fail("look must be >= n");
        if (smooth > look) return fail("smooth must be <= look");
        if (!positive(fade_threshold) || fade_threshold <= 1.0) return fail("fade_threshold must be finite and > 1");
        if (!positive(min_peak_norm) || !positive(min_leg_atr)) return fail("minimum peak and leg values must be finite and > 0");
        if (!finite(min_current_norm) || !finite(max_current_norm) || !finite(min_total_norm) || min_current_norm < 0.0 || max_current_norm < 0.0 || min_total_norm < 0.0 || min_current_norm > max_current_norm)
            return fail("current norm bounds are invalid");
        if (!finite(max_extension_atr) || max_extension_atr < 0.0) return fail("max_extension_atr must be finite and >= 0");
        if (!finite(min_coverage) || !finite(min_rejection) || !finite(confirm_close_fraction) || !finite(confirm_opposing_share)
            || min_coverage < 0.0 || min_coverage > 1.0 || min_rejection < 0.0 || min_rejection > 1.0
            || confirm_close_fraction < 0.0 || confirm_close_fraction > 1.0
            || confirm_opposing_share < 0.0 || confirm_opposing_share > 1.0)
            return fail("fraction fields must be within [0, 1]");
        if (!finite(rearm_fraction) || rearm_fraction <= 0.0 || rearm_fraction > 1.0) return fail("rearm_fraction must be in (0, 1]");
        if (why) why->clear();
        return true;
    }
};

struct Bar {
    long long time = 0;       // close timestamp supplied by caller
    long long run = 0;        // changes at a gap, session, or eligibility-part boundary
    int trend = 0;
    bool eligible = false;
    bool valid = false;       // adapter's complete/known evidence flag; never repaired here
    double o = nan(), h = nan(), l = nan(), c = nan(), v = nan();
    double buy = nan(), sell = nan();
    double buyRef = nan(), sellRef = nan(), totalRef = nan(), atrPrior = nan();
};

struct Event {
    std::string id;
    int side = 0;             // -1 short/top, +1 long/bottom
    std::string kind;         // WATCH, CONFIRM, INVALIDATE, EXPIRE, EXIT_WARN
    std::string setup_type;
    int trend = 0;
    long long watchTime = 0;
    long long time = 0;
    double extreme = nan(), stop = nan(), trigger = nan();
    double fade = nan(), peakNorm = nan(), currentNorm = nan(), coverage = nan();
    std::string reason;
};

struct Result {
    double fadeBuy = nan();
    double fadeSell = nan();
    std::string status;
    std::vector<Event> events;
};

class Engine {
public:
    Engine(const Config& config, double tick) : config_(config), tick_(tick) {
        std::string why;
        config_ok_ = config_.validate(&why) && positive(tick_);
        config_why_ = config_ok_ ? std::string() : (why.empty() ? "tick must be finite and > 0" : why);
    }

    // One closed bar at a time.  No finalization method exists: an open watch
    // remains WATCH-only at end of available input, matching prefix semantics.
    Result update(const Bar& input) {
        Result result;
        result.status = "OK";
        if (!config_ok_) { result.status = "CONFIG_INVALID:" + config_why_; return result; }
        if (have_last_ && input.time <= last_time_) { result.status = "NON_MONOTONIC_TIME"; return result; }

        if (input.trend < -1 || input.trend > 1) { result.status="INVALID_TREND"; return result; }
        const bool transition = have_last_ && input.run != last_run_;
        bool closed_this_bar = false;
        if (transition) {
            if (active_) {
                terminal(result, "INVALIDATE", input.time, "run_transition");
                result.status = "INVALIDATE";
                closed_this_bar = true;
            }
            history_.clear();
            run_bars_ = 0;
            latched_short_ = latched_long_ = false;
            rearm_short_ = rearm_long_ = 0;
            exit_short_ = exit_long_ = false; exit_rearm_short_ = exit_rearm_long_ = 0;
        }

        history_.push_back(input);
        ++run_bars_;
        trim_history();
        const Derived d = derive_current();
        result.fadeBuy = d.fade_buy;
        result.fadeSell = d.fade_sell;

        // Pending state is always resolved before a new watch.  A stop has
        // precedence over confirmation in the same completed bar.
        if (!transition && active_) {
            ++watch_age_;
            const bool stop_hit = (watch_.side == -1 && finite(input.h) && std::round((watch_.stop - input.h) / tick_) <= 0.0)   // LRA 2.0.1: whole ticks
                               || (watch_.side == 1 && finite(input.l) && std::round((input.l - watch_.stop) / tick_) <= 0.0);
            if (stop_hit) {
                terminal(result, "INVALIDATE", input.time, "stop_cross");
                result.status = "INVALIDATE";
                closed_this_bar = true;
            } else if (watch_.setup_type == "PULLBACK_ENTRY" && input.trend == -watch_.trend) {
                terminal(result,"INVALIDATE",input.time,"trend_flip"); result.status="INVALIDATE"; closed_this_bar=true;
            } else {
                const bool trend_ok = watch_.setup_type != "PULLBACK_ENTRY" || input.trend == watch_.trend;
                const bool known = trend_ok && input.eligible && d.observation_valid && finite(d.total_norm)
                    && d.total_norm >= config_.min_total_norm;
                const double classified = input.buy + input.sell;
                const double opposing_share = (finite(classified) && classified > 0.0)
                    ? ((watch_.side == -1 ? input.sell : input.buy) / classified) : nan();
                const bool opposing_body = (watch_.side == -1 && input.c < input.o)
                                        || (watch_.side == 1 && input.c > input.o);
                const bool beyond_trigger = (watch_.side == -1 && input.c < watch_.trigger)
                                         || (watch_.side == 1 && input.c > watch_.trigger);
                const bool progress = (watch_.side == -1 && input.c < watch_.watch_close)
                                   || (watch_.side == 1 && input.c > watch_.watch_close);
                if (known && opposing_body && beyond_trigger && progress && finite(opposing_share)
                    && opposing_share >= config_.confirm_opposing_share) {
                    terminal(result, "CONFIRM", input.time, "opposing_confirmation");
                    result.status = "CONFIRM";
                    closed_this_bar = true;
                } else if (watch_age_ >= config_.confirmation_bars) {
                    terminal(result, "EXPIRE", input.time, "confirmation_timeout");
                    result.status = "EXPIRE";
                    closed_this_bar = true;
                }
            }
        }

        rearm_exit(-1,d.current_buy,d.fade_buy,d.total_norm,d.observation_valid,input.eligible);
        rearm_exit(1,d.current_sell,d.fade_sell,d.total_norm,d.observation_valid,input.eligible);
        if (d.exit_short && !exit_short_) { emit_exit(result,input,-1,d); exit_short_=true; }
        else if (d.exit_long && !exit_long_) { emit_exit(result,input,1,d); exit_long_=true; }

        // Rearm is directional and only accrues while no watch is active; it
        // uses current observed pusher pace, not a retrospective scan.
        if (!active_ && !closed_this_bar) {
            rearm(-1, d.current_buy, d.fade_buy, d.total_norm, d.observation_valid, input.eligible);
            rearm(1, d.current_sell, d.fade_sell, d.total_norm, d.observation_valid, input.eligible);
        }

        if (!active_ && !closed_this_bar) {
            if (d.candidate_short && d.candidate_long) {
                result.status = "CONFLICT";
            } else if (d.candidate_short && !latched_short_) {
                create_watch(result, input, -1, d);
                result.status = "WATCH";
            } else if (d.candidate_long && !latched_long_) {
                create_watch(result, input, 1, d);
                result.status = "WATCH";
            }
        }

        have_last_ = true;
        last_time_ = input.time;
        last_run_ = input.run;
        return result;
    }

private:
    struct Derived {
        double coverage = nan();
        double current_buy = nan(), current_sell = nan(), total_norm = nan();
        double fade_buy = nan(), fade_sell = nan();
        double peak_buy = nan(), peak_sell = nan();
        bool observation_valid = false;
        bool candidate_short = false, candidate_long = false;
        bool exit_short = false, exit_long = false;
    };
    struct Watch {
        std::string id;
        int side = 0, trend = 0;
        std::string setup_type;
        long long watch_time = 0;
        double watch_close = nan();
        double extreme = nan(), stop = nan(), trigger = nan();
        double fade = nan(), peak = nan(), current = nan(), coverage = nan();
    };

    Config config_;
    double tick_;
    bool config_ok_ = false;
    std::string config_why_;
    std::deque<Bar> history_;
    long long run_bars_ = 0;
    bool have_last_ = false;
    long long last_time_ = 0, last_run_ = 0;
    bool active_ = false;
    Watch watch_;
    int watch_age_ = 0;
    bool latched_short_ = false, latched_long_ = false;
    int rearm_short_ = 0, rearm_long_ = 0;
    bool exit_short_=false,exit_long_=false;
    int exit_rearm_short_=0,exit_rearm_long_=0;

    void trim_history() {
        const std::size_t limit = static_cast<std::size_t>(config_.look + config_.smooth + 1);
        while (history_.size() > limit) history_.pop_front();
    }

    static bool price_valid(const Bar& b) {
        return finite(b.o) && finite(b.h) && finite(b.l) && finite(b.c) && b.h >= b.l
            && b.h >= std::max(b.o, b.c) && b.l <= std::min(b.o, b.c);
    }
    static bool raw_flow_valid(const Bar& b) {
        return finite(b.v) && finite(b.buy) && finite(b.sell) && b.v > 0.0 && b.buy >= 0.0 && b.sell >= 0.0;
    }
    static bool refs_valid(const Bar& b) { return positive(b.buyRef) && positive(b.sellRef) && positive(b.totalRef); }

    Derived derive_current() const {
        Derived out;
        const int m = static_cast<int>(history_.size());
        if (m == 0) return out;
        std::vector<double> bp(m, nan()), sp(m, nan()), tp(m, nan());
        std::vector<double> raw_nb(m, nan()), raw_ns(m, nan()), raw_nt(m, nan());
        std::vector<double> nb(m, nan()), ns(m, nan()), nt(m, nan());
        std::vector<bool> obs(m, false), atr_ok(m, false), flow_coverage_ok(m, false);
        for (int i = 0; i < m; ++i) {
            const Bar& b = history_[i];
            if (positive(b.buyRef) && finite(b.buy)) raw_nb[i] = b.buy / b.buyRef;
            if (positive(b.sellRef) && finite(b.sell)) raw_ns[i] = b.sell / b.sellRef;
            if (positive(b.totalRef) && finite(b.buy) && finite(b.sell)) raw_nt[i] = (b.buy + b.sell) / b.totalRef;
            if (i + 1 >= config_.smooth) {
                double sb = 0.0, ss = 0.0, st = 0.0;
                double snb = 0.0, sns = 0.0, snt = 0.0;
                bool full = true, normalized_full = true;
                for (int j = i - config_.smooth + 1; j <= i; ++j) {
                    if (!finite(history_[j].buy) || !finite(history_[j].sell)) { full = false; break; }
                    sb += history_[j].buy; ss += history_[j].sell; st += history_[j].buy + history_[j].sell;
                    if (!finite(raw_nb[j]) || !finite(raw_ns[j]) || !finite(raw_nt[j])) normalized_full = false;
                    else { snb += raw_nb[j]; sns += raw_ns[j]; snt += raw_nt[j]; }
                }
                if (full) { bp[i] = sb / config_.smooth; sp[i] = ss / config_.smooth; tp[i] = st / config_.smooth; }
                if (full && normalized_full) { nb[i] = snb / config_.smooth; ns[i] = sns / config_.smooth; nt[i] = snt / config_.smooth; }
            }
            const double coverage = positive(b.v) && finite(b.buy) && finite(b.sell) ? (b.buy + b.sell) / b.v : nan();
            const bool coverage_ok = raw_flow_valid(b) && finite(coverage) && coverage >= config_.min_coverage && coverage <= 1.0 + 1e-12;
            flow_coverage_ok[i] = coverage_ok;
            bool smooth_flow_ok = i + 1 >= config_.smooth;
            for (int j = i - config_.smooth + 1; smooth_flow_ok && j <= i; ++j) smooth_flow_ok = flow_coverage_ok[j];
            obs[i] = b.valid && price_valid(b) && coverage_ok && smooth_flow_ok && refs_valid(b) && finite(bp[i]) && finite(sp[i]) && finite(tp[i]) && finite(nt[i]);
            atr_ok[i] = positive(b.atrPrior);
            if (i == m - 1) {
                out.coverage = coverage;
                out.current_buy = nb[i]; out.current_sell = ns[i]; out.total_norm = nt[i];
                out.observation_valid = obs[i];
            }
        }

        // Python requires a full LOOK range counted from the run start.  The
        // bounded deque retains LOOK+smooth history, while run_bars_ preserves
        // that qualification after old values have been evicted.
        if (run_bars_ < config_.look + 1 || m < config_.look + 1) return out;
        const int i = m - 1;
        const int prior_n = i - config_.n;
        const int prior_look = i - config_.look;
        bool finite_price_history = true;
        for (int j = prior_n; j < i; ++j) finite_price_history = finite_price_history && finite(history_[j].h) && finite(history_[j].l);
        for (int j = prior_look; j < i; ++j) finite_price_history = finite_price_history && finite(history_[j].h) && finite(history_[j].l);
        if (!finite_price_history || !finite(history_[i].h) || !finite(history_[i].l)) return out;

        double max_h = history_[prior_n].h, min_l = history_[prior_n].l;
        for (int j = prior_n + 1; j < i; ++j) { max_h = std::max(max_h, history_[j].h); min_l = std::min(min_l, history_[j].l); }
        double pb_high=history_[i-config_.pullback_n].h,pb_low=history_[i-config_.pullback_n].l;
        for(int j=i-config_.pullback_n+1;j<i;++j){pb_high=std::max(pb_high,history_[j].h);pb_low=std::min(pb_low,history_[j].l);}
        // LRA 2.0.1: whole ticks, exactly as the research code
        const bool local_high=std::round((pb_high-history_[i].h)/tick_)<=1.0,local_low=std::round((history_[i].l-pb_low)/tick_)<=1.0;
        const bool new_high = history_[i].h > max_h;
        const bool new_low = history_[i].l < min_l;
        int short_start = prior_look, long_start = prior_look;
        double low = history_[prior_look].l, high = history_[prior_look].h;
        for (int j = prior_look + 1; j < i; ++j) {
            if (history_[j].l <= low) { low = history_[j].l; short_start = j; }
            if (history_[j].h >= high) { high = history_[j].h; long_start = j; }
        }
        bool short_quality = true, long_quality = true;
        for (int j = short_start; j <= i; ++j) short_quality = short_quality && obs[j];
        for (int j = long_start; j <= i; ++j) long_quality = long_quality && obs[j];

        if (out.current_buy > 0.0) {
            bool all_finite = true; double peak = -std::numeric_limits<double>::infinity(); int peak_i = short_start;
            for (int j = short_start; j < i; ++j) {
                if (!finite(nb[j])) { all_finite = false; break; }
                if (nb[j] >= peak) { peak = nb[j]; peak_i = j; }
            }
            if (all_finite) { out.peak_buy = peak; out.fade_buy = peak / out.current_buy;
                const double range = history_[i].h - history_[i].l;
                const double rejection = (price_valid(history_[i]) && range > 0.0) ? std::round((history_[i].h - std::max(history_[i].o, history_[i].c)) / tick_) / std::round(range / tick_) : nan();
                const double leg = positive(history_[i].atrPrior) ? (history_[i].h - history_[short_start].l) / history_[i].atrPrior : nan();
                const double extension = positive(history_[i].atrPrior) ? (history_[i].h - max_h) / history_[i].atrPrior : nan();
                const bool quality = history_[i].eligible && short_quality && atr_ok[i]
                    && finite(out.fade_buy) && out.fade_buy >= config_.fade_threshold && out.peak_buy >= config_.min_peak_norm
                    && out.current_buy >= config_.min_current_norm && out.current_buy <= config_.max_current_norm
                    && finite(out.total_norm) && out.total_norm >= config_.min_total_norm
                    && (i - peak_i) >= config_.min_peak_age && finite(leg) && leg >= config_.min_leg_atr
                    && finite(rejection) && rejection >= config_.min_rejection;
                const bool reversal=quality && new_high && finite(extension) && extension<=config_.max_extension_atr;
                const double local_extension=(history_[i].h-pb_high)/history_[i].atrPrior;
                out.candidate_short=config_.mode=="reversal" ? reversal : quality && history_[i].trend==-1 && local_high && finite(local_extension) && local_extension<=config_.max_extension_atr;
                out.exit_short=config_.mode=="pullback" && history_[i].trend==1 && reversal;
            }
        }
        if (out.current_sell > 0.0) {
            bool all_finite = true; double peak = -std::numeric_limits<double>::infinity(); int peak_i = long_start;
            for (int j = long_start; j < i; ++j) {
                if (!finite(ns[j])) { all_finite = false; break; }
                if (ns[j] >= peak) { peak = ns[j]; peak_i = j; }
            }
            if (all_finite) { out.peak_sell = peak; out.fade_sell = peak / out.current_sell;
                const double range = history_[i].h - history_[i].l;
                const double rejection = (price_valid(history_[i]) && range > 0.0) ? std::round((std::min(history_[i].o, history_[i].c) - history_[i].l) / tick_) / std::round(range / tick_) : nan();
                const double leg = positive(history_[i].atrPrior) ? (history_[long_start].h - history_[i].l) / history_[i].atrPrior : nan();
                const double extension = positive(history_[i].atrPrior) ? (min_l - history_[i].l) / history_[i].atrPrior : nan();
                const bool quality = history_[i].eligible && long_quality && atr_ok[i]
                    && finite(out.fade_sell) && out.fade_sell >= config_.fade_threshold && out.peak_sell >= config_.min_peak_norm
                    && out.current_sell >= config_.min_current_norm && out.current_sell <= config_.max_current_norm
                    && finite(out.total_norm) && out.total_norm >= config_.min_total_norm
                    && (i - peak_i) >= config_.min_peak_age && finite(leg) && leg >= config_.min_leg_atr
                    && finite(rejection) && rejection >= config_.min_rejection;
                const bool reversal=quality && new_low && finite(extension) && extension<=config_.max_extension_atr;
                const double local_extension=(pb_low-history_[i].l)/history_[i].atrPrior;
                out.candidate_long=config_.mode=="reversal" ? reversal : quality && history_[i].trend==1 && local_low && finite(local_extension) && local_extension<=config_.max_extension_atr;
                out.exit_long=config_.mode=="pullback" && history_[i].trend==-1 && reversal;
            }
        }
        return out;
    }

    void rearm(int side, double norm, double fade, double total_norm, bool observation_valid, bool eligible) {
        bool& latched = side == -1 ? latched_short_ : latched_long_;
        int& count = side == -1 ? rearm_short_ : rearm_long_;
        if (!latched) return;
        if (eligible && observation_valid && finite(total_norm) && total_norm >= config_.min_total_norm
            && finite(norm) && norm >= config_.min_current_norm && finite(fade)
            && fade < config_.fade_threshold * config_.rearm_fraction) ++count;
        else count = 0;
        if (count >= config_.rearm_bars) { latched = false; count = 0; }
    }

    void rearm_exit(int side,double norm,double fade,double total,bool observed,bool eligible){
        bool& latch=side==-1?exit_short_:exit_long_; int& count=side==-1?exit_rearm_short_:exit_rearm_long_;
        if(!latch)return;
        if(eligible && observed && finite(total) && total>=config_.min_total_norm && finite(norm) && norm>=config_.min_current_norm && finite(fade) && fade<config_.fade_threshold*config_.rearm_fraction)++count;else count=0;
        if(count>=config_.rearm_bars){latch=false;count=0;}
    }
    void emit_exit(Result& r,const Bar& b,int side,const Derived& d){
        Event e; e.id="EXIT"+std::to_string(b.time)+":"+std::to_string(side); e.side=side;e.kind="EXIT_WARN";e.setup_type="TREND_EXIT";e.trend=b.trend;e.watchTime=e.time=b.time;
        e.extreme=side==-1?b.h:b.l;e.stop=e.extreme-side*tick_;e.trigger=side==-1?b.h-config_.confirm_close_fraction*(b.h-b.l):b.l+config_.confirm_close_fraction*(b.h-b.l);
        e.fade=side==-1?d.fade_buy:d.fade_sell;e.peakNorm=side==-1?d.peak_buy:d.peak_sell;e.currentNorm=side==-1?d.current_buy:d.current_sell;e.coverage=d.coverage;
        e.reason=side==-1?"trend_exit_buy_fade":"trend_exit_sell_fade";r.events.push_back(e);
    }
    Event event_from_watch(const std::string& kind, long long time, const std::string& reason) const {
        Event e;
        e.id = watch_.id; e.side = watch_.side; e.kind = kind; e.setup_type=watch_.setup_type;e.trend=watch_.trend; e.watchTime = watch_.watch_time; e.time = time;
        e.extreme = watch_.extreme; e.stop = watch_.stop; e.trigger = watch_.trigger; e.fade = watch_.fade;
        e.peakNorm = watch_.peak; e.currentNorm = watch_.current; e.coverage = watch_.coverage; e.reason = reason;
        return e;
    }
    void terminal(Result& result, const std::string& kind, long long time, const std::string& reason) {
        result.events.push_back(event_from_watch(kind, time, reason));
        active_ = false;
        watch_age_ = 0;
    }
    void create_watch(Result& result, const Bar& b, int side, const Derived& d) {
        Watch w;
        std::ostringstream id;
        id << "W" << b.time << ":" << (side == -1 ? "short" : "long");
        w.id = id.str(); w.side = side;w.trend=b.trend;w.setup_type=config_.mode=="reversal"?"REVERSAL":"PULLBACK_ENTRY"; w.watch_time = b.time; w.watch_close = b.c;
        w.extreme = side == -1 ? b.h : b.l; w.stop = side == -1 ? b.h + tick_ : b.l - tick_;
        const double range = b.h - b.l;
        w.trigger = side == -1 ? b.h - config_.confirm_close_fraction * range
                               : b.l + config_.confirm_close_fraction * range;
        w.fade = side == -1 ? d.fade_buy : d.fade_sell;
        w.peak = side == -1 ? d.peak_buy : d.peak_sell;
        w.current = side == -1 ? d.current_buy : d.current_sell;
        w.coverage = d.coverage;
        watch_ = w; active_ = true; watch_age_ = 0;
        if (side == -1) { latched_short_ = true; rearm_short_ = 0; }
        else { latched_long_ = true; rearm_long_ = 0; }
        result.events.push_back(event_from_watch("WATCH", b.time, "candidate"));
    }
};

}  // namespace ex2
#endif
