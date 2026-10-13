#pragma once
#include <array>
#include <cmath>
#include <limits>

// Native research integration helper, not an Investor/RT ABI adapter.
// Input probabilities are unconditional {neither, exact LOD-first, exact HOD-first}.
// A marker can fail but is never moved/deleted/replaced within its session.
struct FirstExtremeMarker {
    bool exists=false;
    bool price_failed=false;
    int side=0; // 1 LOD, 2 HOD
    double candidate_price=0;
    int candidate_elapsed_minutes=0;
    int declared_elapsed_minutes=0;
    double probability=0;
};
struct FirstExtremePolicy {
    double threshold=0.70; // default overridden from frozen locked_policy.json
    int persistence=1;
    bool require_previous_band_gate=false;
    int deadline_minutes=90; // override per-market training-derived deadline
    double tick=0.25;
};
struct FirstExtremeState {
    FirstExtremeMarker marker;
    int previous_side=0, consecutive=0, previous_elapsed=-1;
    double previous_candidate=0;
    void reset_session(){*this=FirstExtremeState();}
    bool assess(const FirstExtremePolicy& policy, int elapsed,
                double running_low,double running_high,
                int low_formation_elapsed,int high_formation_elapsed,
                bool low_gate,bool high_gate,const std::array<double,3>& probability,
                bool known_prefix_valid=true) {
        if(marker.exists){
            if(marker.side==1 && running_low<marker.candidate_price-policy.tick/2)
                marker.price_failed=true;
            if(marker.side==2 && running_high>marker.candidate_price+policy.tick/2)
                marker.price_failed=true;
            return false;
        }
        if(!known_prefix_valid || elapsed<9 || elapsed>policy.deadline_minutes || elapsed%3){
            previous_side=0;consecutive=0;previous_elapsed=elapsed;return false;
        }
        double low=(!policy.require_previous_band_gate || low_gate)?probability[1]:-1;
        double high=(!policy.require_previous_band_gate || high_gate)?probability[2]:-1;
        int side=low>=high?1:2;double p=low>=high?low:high;
        double price=side==1?running_low:running_high;
        bool same=(side==previous_side && std::abs(price-previous_candidate)<policy.tick/2
                   && (previous_elapsed<0 || elapsed==previous_elapsed+3));
        if(p>=policy.threshold){
            consecutive=same?consecutive+1:1;previous_side=side;previous_candidate=price;
            previous_elapsed=elapsed;
            if(consecutive>=policy.persistence){
                marker.exists=true;marker.side=side;marker.candidate_price=price;
                marker.candidate_elapsed_minutes=side==1?low_formation_elapsed:high_formation_elapsed;
                marker.declared_elapsed_minutes=elapsed;marker.probability=p;return true;
            }
        } else {previous_side=0;consecutive=0;previous_elapsed=elapsed;}
        return false;
    }
};
