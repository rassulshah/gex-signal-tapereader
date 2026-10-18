#ifndef LS_ZONESTORY_H
#define LS_ZONESTORY_H
// ZoneStory.h 1.0.0 (2026-10-18) -- the story of the zone price is working, built from the Market State facts (MarketState.h) once per
// CLOSED bar. SDK-free, unit-tested on Linux. Rassul 2026-10-17: "the liquidity read ... can better judge the situation"; the process
// he described 2026-10-11: "absorb, exhaust, counter or absorb, pull, break" (absorption-exhaustion skill section 3).
//   none -> arrived (a close inside the zone)
//        -> absorbing        (attackers spike INTO the zone - Aggression >= 2x on the attacking side - and price does not close through
//                             it; or a FINAL Delta Profile A at the zone; or the Liquidity Profile says absorbing)
//        -> attack_fading    (after absorbing: the attackers' next spike at the zone is < 0.5x the first, or 3 closed bars without one)
//        -> attackers_pulling(after fading: the ATTACKERS' own resting side leaves - Book Flow, path S2)
//        -> counter / held   (the defenders spike >= 2x and the bar closes back away from the zone)            paths S0 / S1 / S2
//   or   -> defenders_pulling(the DEFENDERS' resting side leaves while attackers hit - Book Flow)
//        -> break            (a close beyond the zone by more than its half-width)                             path F
// Support zone: attackers = sellers, defenders = buyers; resistance zone: the mirror. Decided per closed bar, never re-written for a bar.
// The read uses the step + path so far ("absorbing, attack fading - S1 so far; needs a counter bar") and the odds engine looks them up.
#include <algorithm>
#include <string>
#include "MarketState.h"

namespace zst {

static const char* const VERSION = "1.0.0";
enum Step { NONE = 0, ARRIVED, ABSORBING, FADING, ATT_PULLING, DEF_PULLING, COUNTER, BREAK };
inline const char* stepName(int s) {
    static const char* const N[] = { "none", "arrived", "absorbing", "attack_fading", "attackers_pulling", "defenders_pulling", "counter", "break" };
    return (s >= 0 && s <= BREAK) ? N[s] : "none";
}

struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0; };
struct Zone { std::string id; double lo = 0, hi = 0; int side = 0; };   // side +1 support (bull turn), -1 resistance; 0 = none

struct Story {
    Zone z; int step = NONE; std::string path = "?"; long long lastBar = 0, since = 0;
    double firstAttackX = 0; int barsSinceAttack = 0; bool sawFade = false, sawAttPull = false, sawDefPull = false; bool done = false;
};

// what the facts say about this bar (pure helpers so the rules are testable)
struct Obs {
    bool inZone = false, closedThrough = false, closedAway = false;
    double attackX = 0, defendX = 0;          // the attackers' / defenders' aggression ratio this bar (0 = no spike)
    bool dpAFinalHere = false;                // a FINAL Delta Profile A at the zone, on the defenders' side
    bool attPulling = false, defPulling = false;
    std::string lpStep;                       // the Liquidity Profile's own step for this zone (if it posts one)
};

inline Obs observe(const Zone& z, const Bar& b, const std::map<std::string, mst::Facts>& F) {
    Obs o; if (z.side == 0 || z.hi < z.lo) return o;
    const double half = (z.hi - z.lo) / 2;
    o.inZone = b.l <= z.hi && b.h >= z.lo;
    o.closedThrough = z.side > 0 ? b.c < z.lo - half : b.c > z.hi + half;
    o.closedAway = z.side > 0 ? b.c > z.hi : b.c < z.lo;
    const std::string att = z.side > 0 ? "sell" : "buy", def = z.side > 0 ? "buy" : "sell";
    auto it = F.find("AGG");
    if (it != F.end()) {
        if (mst::getS(it->second, "spike") == att) o.attackX = mst::getD(it->second, att + "_x");
        if (mst::getS(it->second, "spike") == def) o.defendX = mst::getD(it->second, def + "_x");
    }
    it = F.find("DP");
    if (it != F.end() && mst::getS(it->second, "a") == "final") {
        const double ax = mst::getD(it->second, "a_px", -1);
        if (mst::getS(it->second, "a_side") == (z.side > 0 ? "bull" : "bear") && ax >= z.lo - half && ax <= z.hi + half) o.dpAFinalHere = true;
    }
    it = F.find("BOOK");
    if (it != F.end()) {
        // defenders of a support zone rest on the bid; attackers of it rest on the ask
        const std::string defSt = mst::getS(it->second, z.side > 0 ? "bid_state" : "ask_state"), attSt = mst::getS(it->second, z.side > 0 ? "ask_state" : "bid_state");
        o.defPulling = defSt == "leaving"; o.attPulling = attSt == "leaving";
    }
    it = F.find("LIQ");
    if (it != F.end() && mst::getS(it->second, "zone_id") == z.id) o.lpStep = mst::getS(it->second, "zone_step");
    return o;
}

// one closed bar -> the story's next step (each bar once: a bar at or before lastBar is ignored)
inline void step(Story& s, const Bar& b, const Obs& o) {
    if (b.t <= s.lastBar || s.done) return;
    s.lastBar = b.t;
    if (s.step == NONE) { if (o.inZone) { s.step = ARRIVED; s.since = b.t; } else return; }
    if (o.closedThrough) { s.step = BREAK; s.path = s.sawDefPull ? "F" : "break"; s.done = true; return; }
    const bool absorbNow = (o.attackX >= 2.0 && !o.closedThrough) || o.dpAFinalHere || o.lpStep == "absorbing";
    switch (s.step) {
    case ARRIVED:
        if (absorbNow) { s.step = ABSORBING; s.firstAttackX = o.attackX; s.barsSinceAttack = 0; }
        else if (o.defPulling && o.attackX >= 2.0) { s.step = DEF_PULLING; s.sawDefPull = true; }
        break;
    case ABSORBING:
        if (o.defendX >= 2.0 && o.closedAway) { s.step = COUNTER; s.path = "S0"; s.done = true; break; }
        if (o.defPulling) { s.step = DEF_PULLING; s.sawDefPull = true; break; }
        if (o.attackX >= 2.0) { if (s.firstAttackX > 0 && o.attackX < 0.5 * s.firstAttackX) { s.step = FADING; s.sawFade = true; } else { s.firstAttackX = std::max(s.firstAttackX, o.attackX); s.barsSinceAttack = 0; } }
        else if (++s.barsSinceAttack >= 3) { s.step = FADING; s.sawFade = true; }
        break;
    case FADING:
    case ATT_PULLING:
        if (o.defendX >= 2.0 && o.closedAway) { s.step = COUNTER; s.path = s.sawAttPull ? "S2" : "S1"; s.done = true; break; }
        if (o.defPulling) { s.step = DEF_PULLING; s.sawDefPull = true; break; }
        if (o.attPulling) { s.step = ATT_PULLING; s.sawAttPull = true; }
        if (o.attackX >= 2.0 && s.firstAttackX > 0 && o.attackX >= 1.5 * s.firstAttackX) { s.step = ABSORBING; s.firstAttackX = o.attackX; s.barsSinceAttack = 0; }   // the attack came back stronger
        break;
    case DEF_PULLING:
        if (o.defendX >= 2.0 && o.closedAway) { s.step = COUNTER; s.path = "S0"; s.done = true; }   // they came back
        break;
    default: break;
    }
}
// the path so far for the read: S1 so far once faded, S2 once attackers pulled, F once defenders pulled
inline std::string pathSoFar(const Story& s) {
    if (s.done) return s.path;
    if (s.sawDefPull) return "F";
    if (s.sawAttPull) return "S2";
    if (s.sawFade) return "S1";
    if (s.step == ABSORBING) return "S0";
    return "?";
}
// the read's one line (plain words) and what is still needed
inline std::string words(const Story& s) {
    switch (s.step) {
    case ARRIVED: return "price in the zone - no absorption yet";
    case ABSORBING: return std::string(s.z.side > 0 ? "sellers" : "buyers") + " absorbed - needs the attack to fade or a counter bar";
    case FADING: return std::string("absorbed, attack fading (") + pathSoFar(s) + " so far) - needs a counter-attack bar";
    case ATT_PULLING: return std::string(s.z.side > 0 ? "sellers" : "buyers") + " pulling their orders (S2 so far) - needs a counter-attack bar";
    case DEF_PULLING: return std::string(s.z.side > 0 ? "bids" : "offers") + " leaving under attack - break risk";
    case COUNTER: return std::string("held: counter-attack (") + s.path + ")";
    case BREAK: return "broke through the zone";
    default: return "";
    }
}

}   // namespace zst
#endif
