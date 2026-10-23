#ifndef LS_REVERSAL_STAGE_LOGIC_H
#define LS_REVERSAL_STAGE_LOGIC_H
// ReversalStageLogic.h 2.0.0 (2026-10-23) -- SDK-free, typed-evidence core for local pivot attempts.
// THE 4-STEP REVAMP (Rassul approved 05:47-05:50 CT, build ordered option B 05:55 CT):
//   GATE (not a step)  an attempt is tracked only at a meaningful location: one of the last 3 intact swing lows / highs
//                      (SwingRef.h rule), a key / MenthorQ level, or a live queue / LMT zone (a typed held Q at the extreme).
//                      The new 5-candle extreme start is kept; the extreme must trade into the gate zone (max(4 ticks, 0.3 ATR)
//                      before it, at most max(that, 0.5 ATR) through it).
//   1 TRAP       (liquidity)  P stacking (defenders' book), Q queue held BEFORE contact.  Fail: W / B (floor pulled before tested).
//   2 COLLISION  (execution)  an attack S / K / L at the zone AND a result A / Q held after contact, on the same candle or within
//                             1 candle.  Step 1 alone never confirms.  Fail: Q! (the queue at the extreme broke / drained).
//   3 TRAPPED    (fuel)       E pace fading, Tr, G, D (MBO CVD divergence within 5 candles of the collision, at the gate), or the
//                             attackers underwater: close vs their average fill >= 0.25 ATR ("sellers -6 ticks, 1,840 lots").
//                             Fail: a new extreme beyond the floor.
//   4 RELEASE    (the turn)   H defenders hit back or R reclaim; then L re-test held.  Fail: X.
//   RESULT       the close leaves the extreme by 1 ATR after a release (a result line, not a step).
// A stage reports observations; it is not a trade model, probability, entry recommendation, or performance claim.
// Kept: never-repaint frozen checkpoints, typed-evidence rules (only typed COMPLETE evidence steps), Q only at the attempt's own
// extreme (1.2.0 fix #7).  Not wired yet (documented in NOTES-STAGE4-1023.md): Sr refill and Sta stop-run (their records live in
// lsSweepRefill / lsStopRun), volume-profile gate levels (needs VAP in this pane).
#include <algorithm>
#include <cmath>
#include <climits>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "LetterStrips.h"
#include "ReversalEvidence.h"

namespace rsl {

static const char* const VERSION = "2.0.0";   // (2.0.0, 2026-10-23) the 4-step model: gate + Trap / Collision / Trapped / Release + result; D divergence, attackers underwater // (1.3.0) typed MBO evidence engine + Q at the own extreme
enum { LOW = lst::LOW, HIGH = lst::HIGH };
enum Status { NONE = 0, FORMING = 1, FAILED = 2, COMPLETE = 3, EXPIRED = 4 };
enum Step4 { TRAP = 1, COLLISION = 2, TRAPPED = 3, RELEASE = 4, STEPS = 4 };
enum Readiness {
    READ_UNKNOWN = 0, READ_REVIEW = 1, READ_CONFIRMED = 2, READ_PARTIAL = 3, READ_UNAVAILABLE = 4, READ_PENDING = 5
};
enum EvidenceFlag {
    F_ATTACK = 1 << 0,
    F_ATTACK_MBO = 1 << 1,
    F_DEFENSE_A = 1 << 2,
    F_ABSORB_MBO = 1 << 3,
    F_DEFENSE_P = 1 << 4,
    F_DEFENSE_QUEUE = 1 << 5,
    F_QUEUE_ADVERSE = 1 << 6,
    F_QUEUE_UNKNOWN = 1 << 7,
    F_WEAKEN_G = 1 << 8,
    F_WEAKEN_TR = 1 << 9,
    F_PACE_FADE = 1 << 10,
    F_TURN_MBO = 1 << 11,
    F_TURN_HYBRID = 1 << 12,
    F_RETEST_EXECUTED_HELD = 1 << 13,
    F_LEGACY_CONTEXT = 1 << 14,
    F_PARTIAL_CONTEXT = 1 << 15,
    F_UNAVAILABLE_CONTEXT = 1 << 16,
    F_PENDING_CONTEXT = 1 << 17,
    F_LINEAGE_REJECTED = 1 << 18,
    F_STOCKED_ONLY = 1 << 19,
    F_CVD_DIV = 1 << 20,          // (2.0.0) D: MBO delta new local extreme while the candle holds a higher low / lower high
    F_UNDERWATER = 1 << 21,       // (2.0.0) the collision's attackers are >= 0.25 ATR underwater vs their average fill
    F_TRAP_QUEUE = 1 << 22,       // (2.0.0) a held queue BEFORE contact (Trap)
    F_GATED = 1 << 23,            // (2.0.0) the attempt passed the location gate
    F_COLLISION = 1 << 24,        // (2.0.0) attack + execution result within 1 candle
    F_TURN_OTHER = 1 << 25        // imported / unknown-source turn
};
static const int EXT_BARS = 5, ATR_BARS = 14, IDLE_BARS = 5, MAX_BARS = 30, FAIL = -1;
static const int E_SHOW100 = 50;          // existing AttackLogic threshold, only named here
static const int D_BARS = 5;              // (2.0.0) D counts within 5 candles after the collision
static const double UNDERWATER_ATR = 0.25; // (2.0.0) start value: attackers >= 0.25 ATR underwater (0.5 x the tick-normalised threshold)
static const double GATE_THROUGH_ATR = 0.5;
static const char* const ODDS_TICKET = "T-20261022-39";

inline const char* stageName(int s) {
    static const char* const N[] = { "", "Trap", "Collision", "Trapped", "Release" };
    return s >= 1 && s <= STEPS ? N[s] : "";
}
inline const char* statusName(int s) { return s == FORMING ? "forming" : s == FAILED ? "failed" : s == COMPLETE ? "complete" : s == EXPIRED ? "expired" : ""; }
inline const char* readinessName(int r) {
    return r == READ_REVIEW ? "review" : r == READ_CONFIRMED ? "confirmed" : r == READ_PARTIAL ? "partial" : r == READ_UNAVAILABLE ? "unavailable" : r == READ_PENDING ? "pending" : "unknown";
}
inline const char* basisName(int b) {
    return b == rse::MBO ? "MBO" : b == rse::HYBRID ? "hybrid" : b == rse::IMPORTED ? "imported" : "unknown";
}

struct Bar {
    long long key = 0, h = 0, l = 0, c = 0;
    bool ok = false;
    int quality = rse::UNVERIFIED;
};
struct Ev {
    long long key = 0;
    long long received = 0, sourceKey = 0;
    int pane = lst::ATK;
    lst::Lt l;
    rse::Meta m;
};
// (2.0.0) a gate location.  dir -1 a low, +1 a high, 0 either side (MenthorQ).  kind 0 swing (needs pivKey = the swing's candle,
// fromKey = the candle that confirmed it), 1 key level, 2 MenthorQ level.  label = what the Now box shows ("swing low 8:48 AM").
enum GateKind { G_SWING = 0, G_KEY = 1, G_MQ = 2, G_QUEUE = 3 };
struct Gate { long long px = 0; int dir = 0, kind = G_KEY; long long pivKey = 0, fromKey = 0; std::string label; };

// Persisted post-bar active-attempt state (the exact replay checkpoint).
struct Checkpoint {
    bool on = false;
    int side = -1, stage = 0, flags = 0;
    long long ext = 0, start = 0, anchor = 0;
    int age = 0;
    std::string chain;
    // (2.0.0) the 4-step facts
    long long atkKey = 0, resKey = 0, vw100 = 0, atkLots = 0, gatePx = 0;
    int collAge = -1;
    std::string gate, story;   // story = "trap~collision~trapped~release" texts
    bool operator==(const Checkpoint& o) const {
        return on == o.on && side == o.side && stage == o.stage && flags == o.flags && ext == o.ext && start == o.start && anchor == o.anchor && age == o.age && chain == o.chain
            && atkKey == o.atkKey && resKey == o.resKey && vw100 == o.vw100 && atkLots == o.atkLots && gatePx == o.gatePx && collAge == o.collAge && gate == o.gate && story == o.story;
    }
};

struct Step {
    long long key = 0;
    int side = -1, stage = 0, status = NONE;
    long long ext = 0, start = 0;
    int failedStage = 0;
    std::string why, chain, here;
    int quality = rse::UNVERIFIED, flags = 0, readiness = READ_UNKNOWN;
    long long decided = 0;
    Checkpoint resume;
    // (2.0.0) display facts of the attempt this step belongs to (also after it ended)
    std::string gate, story; long long gatePx = 0, under = LLONG_MIN, atkLots = 0;   // under = attackers underwater in ticks (LLONG_MIN = n/a)
    bool operator==(const Step& o) const {
        return key == o.key && side == o.side && stage == o.stage && status == o.status && ext == o.ext && start == o.start && failedStage == o.failedStage && why == o.why && chain == o.chain && here == o.here && quality == o.quality && flags == o.flags && readiness == o.readiness && decided == o.decided && resume == o.resume
            && gate == o.gate && story == o.story && gatePx == o.gatePx && under == o.under && atkLots == o.atkLots;
    }
};

inline double atrAt(const std::vector<Bar>& b, size_t i) {
    double s = 0; int k = 0;
    for (size_t j = i + 1; j-- > 1 && k < ATR_BARS;) {
        if (!b[j].ok || !b[j - 1].ok) continue;
        const long double tr = std::max((long double)b[j].h - b[j].l, std::max(std::fabs((long double)b[j].h - b[j - 1].c), std::fabs((long double)b[j].l - b[j - 1].c)));
        s += (double)tr;
        ++k;
    }
    return k >= 5 ? s / k : 0.0;
}
inline void extremeOf(const std::vector<Bar>& b, size_t i, bool& isLow, bool& isHigh) {
    isLow = isHigh = false;
    if (i < (size_t)EXT_BARS || !b[i].ok) return;
    long long mn = b[i - 1].l, mx = b[i - 1].h;
    for (size_t j = i - EXT_BARS; j < i; ++j) {
        if (!b[j].ok) return;
        mn = std::min(mn, b[j].l); mx = std::max(mx, b[j].h);
    }
    isLow = b[i].l < mn; isHigh = b[i].h > mx;
}

struct Attempt {
    bool on = false;
    int side = LOW, stage = 0, flags = 0, age = 0;
    long long ext = 0, start = 0, anchor = 0;
    std::string chain;
    long long atkKey = 0, resKey = 0, vw100 = 0, atkLots = 0, gatePx = 0; int collAge = -1; std::string gate, story;
};
inline Checkpoint checkpointOf(const Attempt& a) {
    Checkpoint c; if (!a.on) return c;
    c.on = true; c.side = a.side; c.stage = a.stage; c.flags = a.flags; c.ext = a.ext; c.start = a.start; c.anchor = a.anchor; c.age = a.age; c.chain = a.chain;
    c.atkKey = a.atkKey; c.resKey = a.resKey; c.vw100 = a.vw100; c.atkLots = a.atkLots; c.gatePx = a.gatePx; c.collAge = a.collAge; c.gate = a.gate; c.story = a.story;
    return c;
}
inline Attempt attemptOf(const Checkpoint& c) {
    Attempt a; if (!c.on) return a;
    a.on = true; a.side = c.side; a.stage = c.stage; a.flags = c.flags; a.ext = c.ext; a.start = c.start; a.anchor = c.anchor; a.age = c.age; a.chain = c.chain;
    a.atkKey = c.atkKey; a.resKey = c.resKey; a.vw100 = c.vw100; a.atkLots = c.atkLots; a.gatePx = c.gatePx; a.collAge = c.collAge; a.gate = c.gate; a.story = c.story;
    return a;
}

inline bool typedComplete(const Ev& e) { return e.m.typed && e.m.quality == rse::COMPLETE; }
inline bool knownPrice(const Ev& e) { return e.m.priceTicks != LLONG_MIN; }
inline long long causeOf(const Ev& e) { return e.m.cause > 0 ? e.m.cause : e.sourceKey > 0 ? e.sourceKey : e.key; }
inline long long orderOf(const Ev& e) { return e.received > 0 ? e.received : e.m.published; }
inline bool causeWithin(const Ev& e, const Attempt& a) { const long long c = causeOf(e); return c >= a.start && c <= e.key; }
inline bool zoneMatches(const Ev& e, const Attempt& a) {
    return knownPrice(e) && causeWithin(e, a) && (a.side == LOW ? e.m.priceTicks >= a.ext : e.m.priceTicks <= a.ext);
}
// a number token " <tag><digits>" at the end of / inside the strip nums ("tk29115" queue anchor, "vw2911550" attack avg fill x100)
inline long long tagOf(const std::string& nums, const char* tag) {
    const std::string t = std::string(" ") + tag; const size_t p = nums.rfind(t); if (p == std::string::npos) return 0;
    long long v = 0; size_t k = p + t.size(); if (k >= nums.size()) return 0;
    for (; k < nums.size() && nums[k] != ' '; ++k) { const char c = nums[k]; if (c < '0' || c > '9') return 0; v = v * 10 + (c - '0'); if (v > 400000000000000LL) return 0; }
    return v;
}
inline long long qAnchorOf(const std::string& nums) {
    const size_t p = nums.rfind(" tk"); if (p == std::string::npos) return 0;
    long long v = 0; size_t k = p + 3; if (k >= nums.size()) return 0;
    for (; k < nums.size(); ++k) { const char c = nums[k]; if (c < '0' || c > '9') return 0; v = v * 10 + (c - '0'); if (v > 4000000000000LL) return 0; }
    return v;
}
// the nums as shown: the hidden " tk<N>" / " vw<N>" tokens removed
inline std::string numsShown(const std::string& n0) {
    std::string n = n0;
    for (const char* tag : { " tk", " vw" }) {
        const size_t p = n.rfind(tag); if (p == std::string::npos) continue;
        size_t e = p + 3; bool digits = e < n.size(); while (e < n.size() && n[e] != ' ') { if (n[e] < '0' || n[e] > '9') digits = false; ++e; }
        if (digits) n.erase(p, e - p);
    }
    return n;
}
inline long long qTolTicks(double atr) { return std::max(4LL, (long long)std::llround(0.3 * (std::isfinite(atr) ? atr : 0.0))); }
inline bool qAtExtreme(const Ev& e, const Attempt& a, double atr) {
    const long long px = knownPrice(e) ? e.m.priceTicks : qAnchorOf(e.l.nums);
    if (px <= 0 || px == LLONG_MIN) return false;
    const long long d = px > a.ext ? px - a.ext : a.ext - px;
    return d <= qTolTicks(atr);
}
inline bool exactAnchor(const Ev& e, const Attempt& a) { return e.m.cause > 0 && a.anchor > 0 && e.m.cause == a.anchor; }
inline bool defenderBook(const Ev& e) { return e.l.book == (e.l.pivot == LOW ? 0 : 1); }
inline int dataFlag(const Ev& e) {
    if (!e.m.typed) return F_LEGACY_CONTEXT;
    return e.m.quality == rse::PARTIAL ? F_PARTIAL_CONTEXT : e.m.quality == rse::UNAVAILABLE ? F_UNAVAILABLE_CONTEXT : e.m.quality == rse::PENDING ? F_PENDING_CONTEXT : 0;
}
inline int stepQuality(const Bar& b, const std::vector<const Ev*>& v) {
    bool partial = false, pending = false, unavailable = false;
    for (const Ev* e : v) if (e->m.typed) {
        partial = partial || e->m.quality == rse::PARTIAL;
        pending = pending || e->m.quality == rse::PENDING;
        unavailable = unavailable || e->m.quality == rse::UNAVAILABLE;
    }
    if (b.quality == rse::UNAVAILABLE || unavailable) return rse::UNAVAILABLE;
    if (b.quality == rse::PENDING || pending) return rse::PENDING;
    if (b.quality == rse::PARTIAL || partial) return rse::PARTIAL;
    return b.quality;
}
inline int readinessOf(int flags, int stage, int quality, bool active) {
    if (quality == rse::UNAVAILABLE || (flags & F_UNAVAILABLE_CONTEXT)) return READ_UNAVAILABLE;
    if (quality == rse::PENDING || (flags & F_PENDING_CONTEXT)) return READ_PENDING;
    if (quality == rse::PARTIAL || (flags & F_PARTIAL_CONTEXT)) return READ_PARTIAL;
    if (quality != rse::COMPLETE) return active || stage > 0 ? READ_REVIEW : READ_UNKNOWN;
    if (stage > 0) return READ_CONFIRMED;
    return active ? READ_REVIEW : READ_UNKNOWN;
}
inline std::string contextText(const Ev& e, const char* reason) {
    std::string t = e.l.code;
    if (e.m.typed) t += std::string(" [") + basisName(e.m.basis) + ", " + rse::qualityName(e.m.quality) + "]";
    else t += " [legacy]";
    if (!e.l.nums.empty()) t += " " + numsShown(e.l.nums);
    if (reason && *reason) t += std::string(" (") + reason + ")";
    return t;
}
inline void appendToken(std::string& s, const std::string& token) { s += (s.empty() ? "" : " ") + token; }
inline void appendText(std::string& s, const std::string& text) { s += (s.empty() ? "" : "; ") + text; }
inline std::string thousands(long long v) {
    const bool neg = v < 0; unsigned long long u = neg ? (unsigned long long)(-(v + 1)) + 1ULL : (unsigned long long)v;
    std::string d = std::to_string(u), o; int n = 0;
    for (size_t k = d.size(); k-- > 0;) { o.insert(o.begin(), d[k]); if (++n % 3 == 0 && k > 0) o.insert(o.begin(), ','); }
    return neg ? "-" + o : o;
}
// the story part k (0 trap, 1 collision, 2 trapped, 3 release) of "a~b~c~d"
inline std::string storyPart(const std::string& s, int k) {
    size_t a = 0; for (int i = 0; i < k; ++i) { const size_t p = s.find('~', a); if (p == std::string::npos) return std::string(); a = p + 1; }
    const size_t e = s.find('~', a); return s.substr(a, e == std::string::npos ? std::string::npos : e - a);
}
inline void storyAdd(std::string& s, int k, const std::string& t) {
    std::string p[4]; for (int i = 0; i < 4; ++i) p[i] = storyPart(s, i);
    if (t.empty() || p[k].find(t) != std::string::npos) return;
    p[k] += (p[k].empty() ? "" : ", ") + t; if (p[k].size() > 60) p[k] = p[k].substr(0, 60);
    s = p[0] + "~" + p[1] + "~" + p[2] + "~" + p[3];
}
// attackers underwater (ticks, > 0 = underwater) at close c: sellers at a low sold at vw -> underwater when the close is above it
inline long long underwater(const Attempt& a, long long c) {
    if (a.vw100 <= 0 || a.atkLots <= 0) return LLONG_MIN;
    const long long d100 = a.side == LOW ? c * 100 - a.vw100 : a.vw100 - c * 100;
    return d100 >= 0 ? (d100 + 50) / 100 : -((-d100 + 50) / 100);
}

// (2.0.0) THE GATE: the location the 5-candle extreme at bar i traded into, or false.  Swings: the last 3 confirmed (fromKey <
// this candle) swings of that side, 2 back only if beyond 1 back, 3 back only if beyond both (SwingRef.h), intact (no candle after the
// swing's own candle and before this one traded through it).  Levels: key (dir matches) and MenthorQ (either).  Nearest wins; a swing
// wins a tie.
inline bool gateFor(const std::vector<Bar>& b, size_t i, const std::vector<Gate>& gates, int side, double atr, Gate& out) {
    const long long tol = qTolTicks(atr), thr = std::max(tol, (long long)std::llround(GATE_THROUGH_ATR * (std::isfinite(atr) ? atr : 0.0)));
    const long long x = side == LOW ? b[i].l : b[i].h; const int dir = side == LOW ? -1 : 1;
    const auto inZone = [&](long long px) { return side == LOW ? (x <= px + tol && x >= px - thr) : (x >= px - tol && x <= px + thr); };
    bool found = false; long long best = 0;
    const auto take = [&](const Gate& g) { const long long d = std::llabs(x - g.px); if (!found || d < best) { found = true; best = d; out = g; } };
    std::vector<const Gate*> sw;
    for (const Gate& g : gates) if (g.kind == G_SWING && g.dir == dir && g.fromKey > 0 && g.fromKey < b[i].key) sw.push_back(&g);
    std::stable_sort(sw.begin(), sw.end(), [](const Gate* p, const Gate* q) { return p->fromKey < q->fromKey; });
    const int n = (int)sw.size();
    for (int k = 0; k < 3 && k < n; ++k) {
        const Gate& g = *sw[(size_t)(n - 1 - k)]; bool elig = true;
        for (int q = 0; q < k && elig; ++q) { const Gate& r = *sw[(size_t)(n - 1 - q)]; elig = side == LOW ? g.px < r.px : g.px > r.px; }
        if (!elig || !inZone(g.px)) continue;
        bool intact = true;
        for (size_t j = 0; j < i && intact; ++j) if (b[j].ok && b[j].key > g.pivKey) intact = side == LOW ? b[j].l >= g.px : b[j].h <= g.px;
        if (intact) take(g);
    }
    const bool swingFound = found;
    for (const Gate& g : gates) {
        if (g.kind == G_SWING || g.kind == G_QUEUE) continue;
        if (g.dir != 0 && g.dir != dir) continue;
        if (!inZone(g.px)) continue;
        if (swingFound && std::llabs(x - g.px) >= best) continue;
        take(g);
    }
    return found;
}

// Replays closed bars oldest first.  gates == nullptr = the gate is off (tests of the step rules only).
inline std::vector<Step> run(const std::vector<Bar>& b, const std::multimap<long long, Ev>& evs, const std::map<long long, Step>* frozen = nullptr, const std::vector<Gate>* gates = nullptr) {
    std::vector<Step> out; out.reserve(b.size());
    Attempt a;
    const auto finish = [&](Step& s, const Attempt& x) { s.gate = x.gate; s.story = x.story; s.gatePx = x.gatePx; s.atkLots = x.atkLots; };
    for (size_t i = 0; i < b.size(); ++i) {
        if (frozen) {
            const std::map<long long, Step>::const_iterator fixed = frozen->find(b[i].key);
            if (fixed != frozen->end()) { out.push_back(fixed->second); a = attemptOf(fixed->second.resume); continue; }
        }
        Step s; s.key = b[i].key;
        std::vector<const Ev*> all, here[2];
        for (std::multimap<long long, Ev>::const_iterator it = evs.lower_bound(b[i].key); it != evs.end() && it->first == b[i].key; ++it) {
            all.push_back(&it->second); here[it->second.l.pivot == HIGH ? 1 : 0].push_back(&it->second);
        }
        s.quality = stepQuality(b[i], all);
        for (const Ev* e : all) s.flags |= dataFlag(*e);
        if (!b[i].ok || b[i].quality != rse::COMPLETE) {
            if (a.on) { s.side = a.side; s.ext = a.ext; s.start = a.start; s.chain = a.chain; s.failedStage = a.stage; s.status = EXPIRED; s.why = "data unavailable; attempt not carried across an unobserved bar"; finish(s, a); }
            a = Attempt(); s.quality = b[i].quality; s.readiness = readinessOf(s.flags, s.stage, s.quality, false); s.resume = checkpointOf(a); out.push_back(s); continue;
        }
        bool isLow = false, isHigh = false; extremeOf(b, i, isLow, isHigh);
        const double atr = atrAt(b, i);
        const long long prevKey = i > 0 ? b[i - 1].key : 0;
        auto endAttempt = [&](int status, const std::string& why) {
            s.status = status; s.failedStage = a.stage; s.why = why; s.side = a.side; s.ext = a.ext; s.start = a.start; s.chain = a.chain; s.flags |= a.flags; finish(s, a); a = Attempt();
        };
        if (a.on) {
            ++a.age;
            const bool takenOut = a.side == LOW ? b[i].l < a.ext : b[i].h > a.ext;
            if (takenOut && a.stage >= COLLISION) endAttempt(FAILED, a.side == LOW ? "the attack re-accelerated: a new low beyond the floor" : "the attack re-accelerated: a new high beyond the floor");
            else if (takenOut) a.ext = a.side == LOW ? b[i].l : b[i].h;
            else {
                const int other = 1 - a.side; const bool otherExt = other == LOW ? isLow : isHigh;
                if (otherExt && a.stage <= TRAP) endAttempt(EXPIRED, "a new extreme on the other side");
                else if ((a.stage == 0 && a.age >= IDLE_BARS) || a.age >= MAX_BARS) endAttempt(EXPIRED, "no progress in time");
            }
        }
        const bool ended = s.status != NONE;
        if (!a.on) {
            int sd = -1;
            if (isLow && isHigh) {
                int low = 0, high = 0;
                for (const Ev* e : here[LOW]) low += typedComplete(*e);
                for (const Ev* e : here[HIGH]) high += typedComplete(*e);
                sd = low > high ? LOW : high > low ? HIGH : -1;
            } else if (isLow) sd = LOW; else if (isHigh) sd = HIGH;
            if (sd >= 0) {
                Attempt n; n.on = true; n.side = sd; n.ext = sd == LOW ? b[i].l : b[i].h; n.start = b[i].key; n.age = 0;
                bool pass = gates == nullptr; Gate g;
                if (gates && gateFor(b, i, *gates, sd, atr, g)) { pass = true; n.gate = g.label; n.gatePx = g.px; n.flags |= F_GATED; }
                if (gates && !pass) {   // a typed held queue AT this extreme = a live queue / LMT zone
                    for (const Ev* e : here[sd]) if (e->pane == lst::QUE && e->l.code == "Q" && typedComplete(*e) && rse::supportive(e->m.outcome) && qAtExtreme(*e, n, atr)) {
                        pass = true; n.gate = "queue zone"; n.gatePx = knownPrice(*e) ? e->m.priceTicks : qAnchorOf(e->l.nums); n.flags |= F_GATED; break; }
                }
                if (pass) a = n;
                else if (!ended) { s.here = std::string("5-candle ") + (sd == LOW ? "low" : "high") + " not at a swing / key / MenthorQ level or queue zone (no attempt)"; }
            }
        }
        if (a.on) {
            std::vector<const Ev*> v = here[a.side];
            bool fullyOrdered = !v.empty(); std::map<long long, int> pubs;
            for (const Ev* e : v) { const long long ord = orderOf(*e); if (!e->m.typed || ord <= 0 || ++pubs[ord] != 1) { fullyOrdered = false; break; } }
            if (fullyOrdered) std::stable_sort(v.begin(), v.end(), [](const Ev* x, const Ev* y) { return orderOf(*x) < orderOf(*y); });
            std::string part, hereText; bool failed = false;
            std::vector<const Ev*> later;   // trapped / release letters: decided after this candle's collision check
            const char* sold = a.side == LOW ? "sold" : "bought";
            for (const Ev* e : v) {
                const std::string& c = e->l.code;
                const int df = dataFlag(*e);
                if (df) { s.flags |= df; appendToken(part, c + "*"); appendText(hereText, contextText(*e, e->m.typed ? "not complete: review only" : "legacy: review only")); continue; }
                if (!typedComplete(*e)) { appendToken(part, c + "*"); appendText(hereText, contextText(*e, "unverified: review only")); continue; }
                const bool atZone = zoneMatches(*e, a);
                const long long ecause = causeOf(*e);
                bool applied = false, adverse = false; const char* why = "";
                if (e->pane == lst::ATK && (c == "S" || c == "K" || c == "L")) {
                    if (!atZone) why = "outside current attempt";
                    else {
                        a.flags |= F_ATTACK; if (e->m.basis == rse::MBO && e->m.lots > 0) a.flags |= F_ATTACK_MBO;
                        a.atkKey = b[i].key;
                        if (e->m.lots > a.atkLots) { a.atkLots = e->m.lots; const long long vw = tagOf(e->l.nums, "vw"); if (vw > 0) a.vw100 = vw; }
                        if (e->m.basis == rse::MBO && e->m.lots > 0 && e->m.pace100 >= 0 && e->m.pace100 <= E_SHOW100) later.push_back(e);   // its pace fade = E
                        applied = true;
                    }
                } else if (e->pane == lst::ATK && (c == "E" || c == "G" || c == "Tr" || c == "D" || c == "R")) { later.push_back(e); continue; }
                else if (e->pane == lst::QUE && c == "Q") {
                    if (rse::supportive(e->m.outcome)) {
                        if (!atZone) why = "outside current attempt";
                        else if (!qAtExtreme(*e, a, atr)) why = "queue not at the attempt's own extreme";
                        else if (a.anchor && ecause != a.anchor) { a.flags |= F_LINEAGE_REJECTED; why = "different defended cause"; }
                        else {
                            if (!a.anchor) a.anchor = ecause;
                            if (!(a.flags & F_ATTACK) && a.stage < TRAP) { a.stage = TRAP; a.flags |= F_TRAP_QUEUE; storyAdd(a.story, 0, "Q " + numsShown(e->l.nums)); }
                            else { a.resKey = b[i].key; a.flags |= F_DEFENSE_QUEUE; }
                            applied = true;
                        }
                    } else if (rse::adverse(e->m.outcome)) {
                        a.flags |= F_QUEUE_ADVERSE;
                        if (a.stage >= TRAP && a.stage <= COLLISION && atZone && (exactAnchor(*e, a) || (!a.anchor && qAtExtreme(*e, a, atr)))) { adverse = true; why = "Q!: the queue at the extreme broke / drained"; }
                        else { a.flags |= F_LINEAGE_REJECTED; why = "queue adverse warning not tied to the defended extreme"; }
                    } else { a.flags |= F_QUEUE_UNKNOWN; why = "queue outcome unknown"; }
                } else if (e->pane == lst::DEF && c == "P") {
                    if (!defenderBook(*e)) why = "attackers' book: review only";
                    else if (!atZone) why = "outside current attempt";
                    else if (a.stage >= COLLISION) why = "context: already past the trap";
                    else { a.stage = std::max(a.stage, (int)TRAP); a.flags |= F_DEFENSE_P; storyAdd(a.story, 0, std::string(a.side == LOW ? "bids " : "offers ") + numsShown(e->l.nums)); applied = true; }
                } else if (e->pane == lst::DEF && c == "A") {
                    if (!atZone) why = "outside current attempt";
                    else if (a.anchor && ecause != a.anchor && a.stage >= COLLISION) { a.flags |= F_LINEAGE_REJECTED; why = "different defended cause"; }
                    else { if (!a.anchor) a.anchor = ecause; a.resKey = b[i].key; a.flags |= F_DEFENSE_A; if (e->m.basis == rse::MBO && e->m.lots > 0) a.flags |= F_ABSORB_MBO; applied = true; }
                } else if (e->pane == lst::DEF && (c == "W" || c == "B")) {
                    if (c == "B" && !defenderBook(*e)) why = "attackers' book: review only";
                    else if (!atZone) why = "outside current attempt";
                    else if (a.stage == TRAP) { adverse = true; why = "the floor was pulled before it was tested"; }
                    else if (c == "B" && a.stage >= COLLISION && exactAnchor(*e, a)) { adverse = true; why = "defenders stepped back at the defended cause"; }
                    else why = "context only";
                } else if (e->pane == lst::DEF && (c == "H" || c == "L" || c == "X")) { later.push_back(e); continue; }
                else why = "context only";
                if (adverse) { appendToken(part, c + "!"); appendText(hereText, contextText(*e, why)); failed = true; break; }
                appendToken(part, c + (applied ? "" : "*")); appendText(hereText, contextText(*e, applied ? "" : why));
            }
            // COLLISION: the attack and an execution result on this candle, or one of them on the candle before
            if (!failed && a.stage < COLLISION && a.atkKey > 0 && a.resKey > 0 && (a.atkKey == b[i].key || a.resKey == b[i].key)
                && (a.atkKey == b[i].key || a.atkKey == prevKey) && (a.resKey == b[i].key || a.resKey == prevKey)) {
                a.stage = COLLISION; a.collAge = a.age; a.flags |= F_COLLISION;
                std::string res; if (a.flags & F_DEFENSE_A) res += "A"; if (a.flags & F_DEFENSE_QUEUE) res += res.empty() ? "Q" : ", Q";
                storyAdd(a.story, 1, (a.atkLots > 0 ? thousands(a.atkLots) + " " + sold + ", " : std::string()) + "held (" + res + ")");
            }
            if (!failed) for (const Ev* e : later) {
                const std::string& c = e->l.code; const bool atZone = zoneMatches(*e, a);
                bool applied = false, adverse = false; const char* why = "";
                if (e->pane == lst::ATK && (c == "S" || c == "K" || c == "L" || c == "E")) {   // E (or the attack's own pace fade)
                    if (a.stage < COLLISION) why = "trapped needs a collision first";
                    else if (e->m.basis != rse::MBO || !atZone || e->m.lots <= 0 || e->m.pace100 < 0 || e->m.pace100 > E_SHOW100) why = "needs measured complete MBO execution fade";
                    else { a.stage = std::max(a.stage, (int)TRAPPED); a.flags |= F_PACE_FADE; storyAdd(a.story, 2, "E"); applied = true; }
                    if (c != "E") { if (applied) appendToken(part, "E"); continue; }
                } else if (e->pane == lst::ATK && (c == "G" || c == "Tr")) {
                    if (a.stage < COLLISION) why = "trapped needs a collision first";
                    else { a.stage = std::max(a.stage, (int)TRAPPED); a.flags |= c == "G" ? F_WEAKEN_G : F_WEAKEN_TR; storyAdd(a.story, 2, c); applied = true; }
                } else if (e->pane == lst::ATK && c == "D") {
                    const long long near = std::max(qTolTicks(atr), (long long)std::llround(0.5 * atr));
                    if (a.stage < COLLISION) why = "D needs a collision first";
                    else if (a.collAge < 0 || a.age - a.collAge > D_BARS) why = "D more than 5 candles after the collision";
                    else if (e->m.basis != rse::MBO || !knownPrice(*e) || std::llabs(e->m.priceTicks - a.ext) > near) why = "D not at the gate level";
                    else { a.stage = std::max(a.stage, (int)TRAPPED); a.flags |= F_CVD_DIV; storyAdd(a.story, 2, "D"); applied = true; }
                } else if (e->pane == lst::ATK && c == "R") {
                    if (a.stage < COLLISION) why = "release needs a collision first";
                    else if (e->m.basis != rse::HYBRID || e->m.cause <= 0 || !atZone) why = "needs hybrid reclaim with explicit current cause";
                    else { a.stage = std::max(a.stage, (int)RELEASE); a.anchor = e->m.cause; a.flags |= F_TURN_HYBRID; storyAdd(a.story, 3, "R reclaim"); applied = true; }
                } else if (e->pane == lst::DEF && c == "H") {
                    const bool parentMatches = e->m.parentCause > 0 && e->m.parentCause == (a.anchor > 0 ? a.anchor : a.start);
                    const bool linked = exactAnchor(*e, a) || parentMatches;
                    if (a.stage < COLLISION) why = "release needs a collision first";
                    else if (e->m.lots <= 0 || e->m.auxLots <= 0 || !atZone || !linked) { a.flags |= F_LINEAGE_REJECTED; why = "needs complete executable response matching native pivot/turn lineage"; }
                    else { a.stage = std::max(a.stage, (int)RELEASE); a.anchor = e->m.cause; a.flags |= e->m.basis == rse::MBO ? F_TURN_MBO : e->m.basis == rse::HYBRID ? F_TURN_HYBRID : F_TURN_OTHER; storyAdd(a.story, 3, "H " + thousands(e->m.lots)); applied = true; }
                } else if (e->pane == lst::DEF && c == "L") {
                    const bool closesHeld = knownPrice(*e) && (a.side == LOW ? b[i].c >= e->m.priceTicks : b[i].c <= e->m.priceTicks);
                    if (a.stage < RELEASE) why = "needs a release first";
                    else if (!exactAnchor(*e, a)) { a.flags |= F_LINEAGE_REJECTED; why = "different turn cause"; }
                    else if (e->m.auxLots <= 0 || !knownPrice(*e) || !closesHeld) { a.flags |= F_STOCKED_ONLY; why = "stocked book only; no executed held re-test"; }
                    else { a.flags |= F_RETEST_EXECUTED_HELD; storyAdd(a.story, 3, "L re-test held"); applied = true; }
                } else if (e->pane == lst::DEF && c == "X") {
                    if (a.stage < RELEASE) why = "needs a release first";
                    else if (!exactAnchor(*e, a) || !atZone) { a.flags |= F_LINEAGE_REJECTED; why = "different turn cause"; }
                    else { adverse = true; why = "the re-test after the release was empty"; }
                }
                if (adverse) { appendToken(part, c + "!"); appendText(hereText, contextText(*e, why)); failed = true; break; }
                appendToken(part, c + (applied ? "" : "*")); appendText(hereText, contextText(*e, applied ? "" : why));
            }
            // the attackers underwater (the collision's average fill vs this close)
            if (!failed && a.stage >= COLLISION) {
                const long long u = underwater(a, b[i].c);
                if (u != LLONG_MIN && atr > 0 && (double)u >= UNDERWATER_ATR * atr) {
                    if (!(a.flags & F_UNDERWATER)) appendToken(part, "uw");
                    a.flags |= F_UNDERWATER; a.stage = std::max(a.stage, (int)TRAPPED);
                }
            }
            if (!part.empty()) a.chain += (a.chain.empty() ? "" : " . ") + part;
            s.here = hereText;
            const long long uNow = a.on && a.stage >= COLLISION ? underwater(a, b[i].c) : LLONG_MIN;
            if (failed) {
                endAttempt(FAILED, hereText.empty() ? std::string("matched adverse evidence") : hereText);
            } else if (atr > 0 && a.stage >= RELEASE && (a.flags & (F_TURN_MBO | F_TURN_HYBRID | F_TURN_OTHER)) && (a.side == LOW ? (long double)b[i].c - a.ext : (long double)a.ext - b[i].c) >= atr) {
                s.stage = RELEASE; s.status = COMPLETE; s.side = a.side; s.ext = a.ext; s.start = a.start; s.chain = a.chain + " . 1 ATR"; s.why = "Result: the close left the extreme by 1 ATR after the release"; s.flags = a.flags; finish(s, a); s.under = uNow; a = Attempt();
            } else if (!ended) {
                s.side = a.side; s.stage = a.stage; s.status = a.stage > 0 ? FORMING : NONE; s.ext = a.ext; s.start = a.start; s.chain = a.chain; finish(s, a); s.under = uNow;
            } else if (a.stage > 0) {
                s.stage = a.stage; s.here += (s.here.empty() ? "" : "; ") + std::string("new ") + (a.side == LOW ? "LOW" : "HIGH") + " attempt";
                if (s.side != a.side) { s.why += " - now a " + std::string(a.side == LOW ? "LOW" : "HIGH") + " attempt"; s.side = a.side; }
            }
        }
        if (s.side < 0) s.status = NONE;
        s.flags |= a.flags;
        s.quality = stepQuality(b[i], all);
        s.readiness = readinessOf(s.flags, s.stage, s.quality, a.on || s.status == FORMING);
        s.resume = checkpointOf(a);
        out.push_back(s);
    }
    return out;
}

// ---- immutable decision record (RS|3, 2.0.0) -----------------------------------------------------------------------------------
// RS|3|key|side|stage|status|ext|start|failedStage|quality|flags|readiness|decided|
//      resumeOn|resumeSide|resumeStage|resumeFlags|resumeExt|resumeStart|resumeAnchor|resumeAge|
//      atkKey|resKey|vw100|atkLots|gatePx|collAge|resumeGate|resumeStory|resumeChain|
//      gate|story|gatePx|under|atkLots|why|chain|here          (38 fields)
inline std::string cleanF(const std::string& s) { std::string o; for (char c : s) if (c != '|' && c != '\r' && c != '\n') o += c; return o.size() > 200 ? o.substr(0, 200) : o; }
inline std::string line(const Step& s) {
    char b[600];
    std::snprintf(b, sizeof b, "RS|3|%lld|%d|%d|%d|%lld|%lld|%d|%d|%d|%d|%lld|%d|%d|%d|%d|%lld|%lld|%lld|%d|%lld|%lld|%lld|%lld|%lld|%d|",
        s.key, s.side, s.stage, s.status, s.ext, s.start, s.failedStage, s.quality, s.flags, s.readiness, s.decided,
        s.resume.on ? 1 : 0, s.resume.side, s.resume.stage, s.resume.flags, s.resume.ext, s.resume.start, s.resume.anchor, s.resume.age,
        s.resume.atkKey, s.resume.resKey, s.resume.vw100, s.resume.atkLots, s.resume.gatePx, s.resume.collAge);
    char t[96]; std::snprintf(t, sizeof t, "|%lld|%lld|%lld|", s.gatePx, s.under, s.atkLots);
    return std::string(b) + cleanF(s.resume.gate) + "|" + cleanF(s.resume.story) + "|" + cleanF(s.resume.chain) + "|" + cleanF(s.gate) + "|" + cleanF(s.story) + t + cleanF(s.why) + "|" + cleanF(s.chain) + "|" + cleanF(s.here);
}
inline bool parseNumber(const std::string& f, long long& n) { return lst::fullLL(f, n); }
inline bool validCheckpoint(const Checkpoint& c) {
    if (!c.on) return c.side == -1 && c.stage == 0 && c.flags == 0 && c.ext == 0 && c.start == 0 && c.anchor == 0 && c.age == 0 && c.chain.empty() && c.atkKey == 0 && c.resKey == 0 && c.vw100 == 0 && c.atkLots == 0 && c.gatePx == 0 && c.collAge == -1 && c.gate.empty() && c.story.empty();
    return c.side >= LOW && c.side <= HIGH && c.stage >= 0 && c.stage <= STEPS && c.flags >= 0 && c.start > 0 && c.age >= 0 && c.anchor >= 0 && c.atkKey >= 0 && c.resKey >= 0 && c.vw100 >= 0 && c.atkLots >= 0 && c.collAge >= -1;
}
inline bool parse(const std::string& s0, Step& s) {
    std::string t = s0; while (!t.empty() && (t.back() == '\r' || t.back() == '\n')) t.pop_back();
    std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t p = t.find('|', a); f.push_back(t.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; if (f.size() > 38) return false; }
    if (f.size() != 38 || f[0] != "RS" || f[1] != "3") return false;
    long long n[40] = {0};
    for (int k = 2; k <= 26; ++k) if (!parseNumber(f[(size_t)k], n[k])) return false;
    for (int k = 32; k <= 34; ++k) if (!parseNumber(f[(size_t)k], n[k])) return false;
    const long long key = n[2], side = n[3], stage = n[4], st = n[5], fs = n[8], q = n[9], flags = n[10], ready = n[11], decided = n[12];
    if (key <= 0 || side < -1 || side > 1 || stage < 0 || stage > STEPS || st < NONE || st > EXPIRED || n[7] < 0 || fs < 0 || fs > STEPS || q < rse::UNVERIFIED || q > rse::PENDING
        || flags < 0 || flags >= (1LL << 26) || ready < READ_UNKNOWN || ready > READ_PENDING || decided < 0 || (n[13] != 0 && n[13] != 1)
        || n[14] < -1 || n[14] > 1 || n[15] < 0 || n[15] > STEPS || n[16] < 0 || n[16] >= (1LL << 26) || n[20] < 0 || n[20] > INT_MAX || n[26] < -1 || n[26] > INT_MAX || n[34] < 0) return false;
    if (side < 0 && (stage != 0 || st != NONE)) return false;
    Checkpoint c; c.on = n[13] != 0; c.side = (int)n[14]; c.stage = (int)n[15]; c.flags = (int)n[16]; c.ext = n[17]; c.start = n[18]; c.anchor = n[19]; c.age = (int)n[20];
    c.atkKey = n[21]; c.resKey = n[22]; c.vw100 = n[23]; c.atkLots = n[24]; c.gatePx = n[25]; c.collAge = (int)n[26]; c.gate = f[27]; c.story = f[28]; c.chain = f[29];
    if (!validCheckpoint(c)) return false;
    Step x; x.key = key; x.side = (int)side; x.stage = (int)stage; x.status = (int)st; x.ext = n[6]; x.start = n[7]; x.failedStage = (int)fs; x.quality = (int)q; x.flags = (int)flags; x.readiness = (int)ready; x.decided = decided; x.resume = c;
    x.gate = f[30]; x.story = f[31]; x.gatePx = n[32]; x.under = n[33]; x.atkLots = n[34]; x.why = f[35]; x.chain = f[36]; x.here = f[37];
    s = x; return true;
}
inline std::string fileName(const std::string& mkt, int spb) { return "RS3-" + (mkt == "6E" ? std::string("EU") : mkt) + "-" + std::to_string(spb) + ".csv"; }
inline std::string fileName(const std::string& mkt, const std::string& contract, int spb) { return "RS3-" + (mkt == "6E" ? std::string("EU") : mkt) + "-" + rse::token(contract) + "-" + std::to_string(spb) + ".csv"; }

// ---- drawing/layout helpers retained for the host --------------------------------
struct Look { int height = 0; int colour = lst::GREEN; bool dim = false; bool stub = false; };
inline Look lookOf(const Step& s) {
    Look k; if (s.side < 0) return k;
    k.colour = s.side == LOW ? lst::GREEN : lst::RED;
    if (s.status == FAILED && s.stage == 0) { k.stub = true; k.dim = true; return k; }
    if (s.status == EXPIRED && s.stage == 0) return k;
    k.height = s.stage; k.dim = s.stage <= TRAP; return k;
}
inline bool boxAt(int pos, int l, int t, int r, int b, int w, int h, int margin, int& x, int& y) {
    if (pos < 0 || pos > 8) pos = 0;
    if (r - l < w + 2 * margin || b - t < h + 2 * margin) return false;
    const int col = pos % 3, row = pos / 3;
    x = col == 0 ? l + margin : col == 1 ? l + (r - l - w) / 2 : r - margin - w;
    y = row == 0 ? t + margin : row == 1 ? t + (b - t - h) / 2 : b - margin - h;
    return true;
}
static const double AXIS_MAX = 4.5;
inline int yOf(double v, int top, int bottom) { return bottom - (int)std::lround(v / AXIS_MAX * (double)(bottom - top)); }
inline std::string sideWord(int side) { return side == LOW ? "LOW" : side == HIGH ? "HIGH" : "-"; }
inline std::string evidenceSummary(int f) {
    std::vector<std::string> p;
    if (f & F_GATED) p.push_back("at a gate level");
    if (f & F_DEFENSE_P) p.push_back("P stacking");
    if (f & F_TRAP_QUEUE) p.push_back("queue before contact");
    if (f & F_COLLISION) p.push_back("collision");
    if (f & F_ABSORB_MBO) p.push_back("MBO absorption");
    if (f & F_DEFENSE_QUEUE) p.push_back("queue held after contact");
    if (f & F_PACE_FADE) p.push_back("E pace fade");
    if (f & F_WEAKEN_TR) p.push_back("Tr trapped");
    if (f & F_WEAKEN_G) p.push_back("G pulling");
    if (f & F_CVD_DIV) p.push_back("D CVD divergence");
    if (f & F_UNDERWATER) p.push_back("attackers underwater");
    if (f & F_TURN_MBO) p.push_back("MBO turn");
    if (f & F_TURN_HYBRID) p.push_back("hybrid turn");
    if (f & F_TURN_OTHER) p.push_back("imported turn");
    if (f & F_RETEST_EXECUTED_HELD) p.push_back("re-test held");
    if (f & F_QUEUE_ADVERSE) p.push_back("adverse queue");
    if (f & F_STOCKED_ONLY) p.push_back("stocked-only L");
    if (p.empty()) return "none";
    std::string o; for (size_t i = 0; i < p.size(); ++i) o += (i ? ", " : "") + p[i]; return o;
}
// price text from ticks ("7,696" / "7,696.25")
inline std::string priceText(long long ticks, double tick) {
    if (!(tick > 0) || !std::isfinite(tick)) return std::to_string(ticks);
    const double px = (double)ticks * tick; char b[48];
    int dec = 0; for (double t = tick; dec < 6 && std::fabs(t - std::round(t)) > 1e-9; t *= 10) ++dec;
    std::snprintf(b, sizeof b, "%.*f", dec, px);
    std::string s = b; const size_t dot = s.find('.'); std::string ip = s.substr(0, dot), fp = dot == std::string::npos ? std::string() : s.substr(dot);
    long long iv = 0; if (!lst::fullLL(ip, iv)) return s;
    return thousands(iv) + fp;
}
// the 4 step texts + the underwater text ("sellers -6 ticks, 1,840 lots")
inline std::string underText(const Step& s) {
    if (s.under == LLONG_MIN || s.atkLots <= 0) return std::string();
    return std::string(s.side == LOW ? "sellers " : "buyers ") + (s.under > 0 ? "-" : "+") + std::to_string(std::llabs(s.under)) + " ticks, " + thousands(s.atkLots) + " lots";
}
inline std::string stepText(const Step& s, int k) {
    std::string t = storyPart(s.story, k - 1);
    if (k == TRAPPED) { const std::string u = (s.flags & F_UNDERWATER) ? underText(s) : std::string(); if (!u.empty()) t = t.empty() ? u : u + ", " + t; }
    if (!t.empty()) return t;
    if (s.status == FAILED && s.failedStage == k - 1) return "failed";
    if (s.stage >= k) return "yes";
    if (k == RELEASE && s.stage >= COLLISION) return "waiting for H / R";
    if (k == TRAPPED && s.stage >= COLLISION) return "waiting for E / Tr / D";
    if (k == COLLISION) return "waiting for an attack + A / Q";
    return "-";
}
// THE NOW BOX: "LOW at 7,696 (swing low 8:48 AM) · Trap: ... · Collision: ... · Trapped: ... · Release: ..." as rows
inline std::vector<std::string> nowRows(const Step& s, const std::string& when, double tick = 0) {
    std::vector<std::string> r; r.push_back("Now " + when);
    if (s.side < 0 || (s.stage == 0 && s.status != FAILED && s.gate.empty())) { r.push_back("No attempt at a level"); r.push_back("Evidence " + std::string(rse::qualityName(s.quality)) + ", " + readinessName(s.readiness)); }
    else {
        std::string head = sideWord(s.side) + " at " + priceText(s.ext, tick);
        if (!s.gate.empty()) head += " (" + s.gate + ")";
        if (s.status == FAILED) head += " - failed";
        else if (s.status == COMPLETE) head += " - result 1 ATR";
        else if (s.status == EXPIRED) head += " - ended";
        r.push_back(head);
        for (int k = TRAP; k <= RELEASE; ++k) r.push_back(std::string(stageName(k)) + ": " + stepText(s, k));
        r.push_back("Evidence " + std::string(rse::qualityName(s.quality)) + ", " + readinessName(s.readiness));
    }
    r.push_back(std::string("Odds ?  (") + ODDS_TICKET + ")"); return r;
}
inline std::vector<std::string> hoverRows(const Step& s, const std::string& when, double tick = 0) {
    std::vector<std::string> r;
    std::string head = "Reversal stage  " + when + "  -  " + sideWord(s.side) + " attempt";
    if (s.status == FAILED && s.stage == 0) head += ", failed at step " + std::to_string(s.failedStage) + " " + stageName(s.failedStage);
    else head += ", step " + std::to_string(s.stage) + " " + stageName(s.stage) + (s.status == COMPLETE ? " (result: 1 ATR)" : s.status == FAILED ? " (the attempt before failed)" : "");
    r.push_back(head);
    r.push_back("Question: is this low / high a trap that is turning? Gate, then Trap > Collision > Trapped > Release.");
    if (!s.gate.empty()) r.push_back("   Gate: " + s.gate + " at " + priceText(s.gatePx, tick));
    for (int k = TRAP; k <= RELEASE; ++k) r.push_back(std::string("   ") + std::to_string(k) + " " + stageName(k) + ": " + stepText(s, k));
    r.push_back("   Evidence: " + evidenceSummary(s.flags) + "  (data " + rse::qualityName(s.quality) + ", " + readinessName(s.readiness) + ")");
    r.push_back("   Letters: " + (s.chain.empty() ? std::string("none yet (the 5-candle extreme at the gate only)") : s.chain));
    if (!s.here.empty()) r.push_back("   On this candle: " + s.here);
    if (!s.why.empty()) r.push_back("   " + std::string(s.status == FAILED ? "Failed: " : s.status == EXPIRED ? "Ended: " : "") + s.why);
    r.push_back("   Use: a step needs typed MBO evidence; Trap alone never confirms. Collision = attack + held on the same / next candle.");
    r.push_back("   Example: LOW at a swing low, bids stacked (Trap), 1,840 sold and absorbed (Collision), sellers -6 ticks + D (Trapped), H (Release).");
    r.push_back(std::string("   Odds: ? - not proven (") + ODDS_TICKET + ")");
    return r;
}
// (2.0.0) how many Now-box columns a pane of paneH px needs for n rows of lh px (1 - 3; the box is never taller than the pane)
inline int nowCols(int n, int paneH, int lh) {
    const int fit = std::max(1, (paneH - 16) / std::max(1, lh));
    for (int c = 1; c < 3; ++c) if ((n + c - 1) / c <= fit) return c;
    return 3;
}
inline std::vector<std::string> wrap(const std::vector<std::string>& rows, size_t width = 78) {
    std::vector<std::string> o;
    for (const std::string& r0 : rows) {
        std::string r = r0;
        while (r.size() > width) { size_t cut = r.rfind(' ', width); if (cut == std::string::npos || cut < 10) cut = width; o.push_back(r.substr(0, cut)); r = r.substr(cut); while (!r.empty() && r[0] == ' ') r.erase(0, 1); r = "     " + r; }
        o.push_back(r);
    }
    return o;
}
// compatibility helper (letter -> the step it can feed); run() is authoritative
inline int stageOf(const Ev& e) {
    if (!typedComplete(e)) return 0;
    const std::string& c = e.l.code;
    if (e.pane == lst::ATK) return (c == "E" || c == "G" || c == "Tr" || c == "D") ? TRAPPED : c == "R" ? RELEASE : (c == "S" || c == "K" || c == "L") ? COLLISION : 0;
    if (e.pane == lst::QUE) return rse::supportive(e.m.outcome) ? TRAP : rse::adverse(e.m.outcome) ? FAIL : 0;
    if (c == "P" && defenderBook(e)) return TRAP;
    if (c == "A") return COLLISION;
    if (c == "H") return RELEASE;
    if (c == "W" || c == "B" || c == "X") return FAIL;
    return 0;
}
} // namespace rsl
#endif
