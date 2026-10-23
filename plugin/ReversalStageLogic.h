#ifndef LS_REVERSAL_STAGE_LOGIC_H
#define LS_REVERSAL_STAGE_LOGIC_H
// ReversalStageLogic.h 1.2.0 (2026-10-22) -- SDK-free, typed-evidence core for
// local pivot attempts.  A stage reports observations; it is not a trade model,
// probability, entry recommendation, or performance claim.
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

static const char* const VERSION = "1.3.0";   // (1.3.0, 2026-10-23 merge) the outside review's typed MBO evidence engine (EV-ATK / EV-DEF / EV-QUEUE, E letter, Q!, causal checkpoints) + the 1.2.0 fix-build rule: a Q steps only at the attempt's own extreme (max(4 ticks, 0.3 ATR))
enum { LOW = lst::LOW, HIGH = lst::HIGH };
enum Status { NONE = 0, FORMING = 1, FAILED = 2, COMPLETE = 3, EXPIRED = 4 };
enum Readiness {
    READ_UNKNOWN = 0,       // no complete evidence establishes this read
    READ_REVIEW = 1,        // an attempt exists, but is not confirmed
    READ_CONFIRMED = 2,     // typed, complete evidence established a phase
    READ_PARTIAL = 3,       // partial evidence was present: review only
    READ_UNAVAILABLE = 4,   // a source/bar was unavailable: do not infer neutral
    READ_PENDING = 5        // a future-dependent source has not resolved
};
// Flags are independent observations/diagnostics, not a score.  In particular,
// ABSORB_MBO is deliberately distinct from P and queue support.
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
    F_PACE_FADE = 1 << 10, // existing atk::Pace.e100 <= 50; no new alpha
    F_TURN_MBO = 1 << 11,
    F_TURN_HYBRID = 1 << 12,
    F_RETEST_EXECUTED_HELD = 1 << 13,
    F_LEGACY_CONTEXT = 1 << 14,
    F_PARTIAL_CONTEXT = 1 << 15,
    F_UNAVAILABLE_CONTEXT = 1 << 16,
    F_PENDING_CONTEXT = 1 << 17,
    F_LINEAGE_REJECTED = 1 << 18,
    F_STOCKED_ONLY = 1 << 19,
    F_WEAKEN_MBO = 1 << 20,
    F_WEAKEN_HYBRID = 1 << 21,
    F_WEAKEN_IMPORTED = 1 << 22,
    F_TURN_IMPORTED = 1 << 23,
    F_WEAKEN_UNKNOWN = 1 << 24,
    F_TURN_UNKNOWN = 1 << 25
};
static const int EXT_BARS = 5, ATR_BARS = 14, IDLE_BARS = 5, MAX_BARS = 30, FAIL = -1;
static const int E_SHOW100 = 50;       // existing AttackLogic threshold, only named here
static const char* const ODDS_TICKET = "T-20261022-39";

inline const char* stageName(int s) {
    static const char* const N[] = { "", "Attack", "Defense", "Attack weakening", "Turn", "Re-test", "Expansion" };
    return s >= 1 && s <= 6 ? N[s] : "";
}
inline const char* statusName(int s) { return s == FORMING ? "forming" : s == FAILED ? "failed" : s == COMPLETE ? "complete" : s == EXPIRED ? "expired" : ""; }
inline const char* readinessName(int r) {
    return r == READ_REVIEW ? "review" : r == READ_CONFIRMED ? "confirmed" : r == READ_PARTIAL ? "partial" : r == READ_UNAVAILABLE ? "unavailable" : r == READ_PENDING ? "pending" : "unknown";
}
inline const char* basisName(int b) {
    return b == rse::MBO ? "MBO" : b == rse::HYBRID ? "hybrid" : b == rse::IMPORTED ? "imported" : "unknown";
}

// Prices are integer ticks.  quality applies to the closed bar frame; default
// UNVERIFIED is deliberately not treated as complete/neutral evidence.
struct Bar {
    long long key = 0, h = 0, l = 0, c = 0;
    bool ok = false;
    int quality = rse::UNVERIFIED;
};
struct Ev {
    long long key = 0;
    // key is the journaled effective decision bar.  sourceKey keeps the
    // original LE decision bar and received keeps first consumer receipt;
    // neither is substituted for producer publication time.
    long long received = 0, sourceKey = 0;
    int pane = lst::ATK;
    lst::Lt l;
    rse::Meta m;
};

// Persisted post-bar active-attempt state.  `anchor` is the defended cause
// through stage 3 and becomes the established turn cause at stage 4+.  It is
// intentionally the agreed compact contract: no inferred sub-bar ordering.
struct Checkpoint {
    bool on = false;
    int side = -1, stage = 0, flags = 0;
    long long ext = 0, start = 0, anchor = 0;
    int age = 0;
    std::string chain;
    bool operator==(const Checkpoint& o) const {
        return on == o.on && side == o.side && stage == o.stage && flags == o.flags && ext == o.ext && start == o.start && anchor == o.anchor && age == o.age && chain == o.chain;
    }
};

// One immutable decision record.  stage is achieved phase only; quality and
// readiness disclose data/evidence state independently.
struct Step {
    long long key = 0;
    int side = -1, stage = 0, status = NONE;
    long long ext = 0, start = 0;
    int failedStage = 0;
    std::string why, chain, here;
    int quality = rse::UNVERIFIED, flags = 0, readiness = READ_UNKNOWN;
    long long decided = 0;              // host stamps actual native decision clock
    Checkpoint resume;                  // exact true post-bar state for replay
    bool operator==(const Step& o) const {
        return key == o.key && side == o.side && stage == o.stage && status == o.status && ext == o.ext && start == o.start && failedStage == o.failedStage && why == o.why && chain == o.chain && here == o.here && quality == o.quality && flags == o.flags && readiness == o.readiness && decided == o.decided && resume == o.resume;
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
// Strict five-candle local extreme.  The trigger/ATR/expiry constants remain
// the established defaults, not optimized parameters.
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
};
inline Checkpoint checkpointOf(const Attempt& a) {
    Checkpoint c; c.on = a.on; c.side = a.on ? a.side : -1; c.stage = a.on ? a.stage : 0; c.flags = a.on ? a.flags : 0;
    c.ext = a.on ? a.ext : 0; c.start = a.on ? a.start : 0; c.anchor = a.on ? a.anchor : 0; c.age = a.on ? a.age : 0; c.chain = a.on ? a.chain : std::string();
    return c;
}
inline Attempt attemptOf(const Checkpoint& c) {
    Attempt a;
    if (!c.on) return a;
    a.on = true; a.side = c.side; a.stage = c.stage; a.flags = c.flags; a.ext = c.ext; a.start = c.start; a.anchor = c.anchor; a.age = c.age; a.chain = c.chain;
    return a;
}

inline bool typedComplete(const Ev& e) { return e.m.typed && e.m.quality == rse::COMPLETE; }
inline bool knownPrice(const Ev& e) { return e.m.priceTicks != LLONG_MIN; }
// A delayed LE keeps its original sourceKey.  Never reinterpret a late source
// as caused on its journaled effective bar merely because m.cause is absent.
inline long long causeOf(const Ev& e) { return e.m.cause > 0 ? e.m.cause : e.sourceKey > 0 ? e.sourceKey : e.key; }
inline long long orderOf(const Ev& e) { return e.received > 0 ? e.received : e.m.published; }
inline bool causeWithin(const Ev& e, const Attempt& a) {
    const long long c = causeOf(e);
    return c >= a.start && c <= e.key;
}
// The only zone relationship asserted without a newly invented distance rule:
// a low defense is not below the established low; a high defense is not above it.
inline bool zoneMatches(const Ev& e, const Attempt& a) {
    return knownPrice(e) && causeWithin(e, a) && (a.side == LOW ? e.m.priceTicks >= a.ext : e.m.priceTicks <= a.ext);
}
// (1.3.0, keeps the 1.2.0 fix-build rule "NOW HIGH stage 2 failed chain Q") a queue Q steps only AT the attempt's own
// extreme: its line within max(4 ticks, 0.3 ATR) of the extreme.  Legacy strip lines carry the anchor as " tk<N>".
inline long long qAnchorOf(const std::string& nums) {
    const size_t p = nums.rfind(" tk"); if (p == std::string::npos) return 0;
    long long v = 0; size_t k = p + 3; if (k >= nums.size()) return 0;
    for (; k < nums.size(); ++k) { const char c = nums[k]; if (c < '0' || c > '9') return 0; v = v * 10 + (c - '0'); if (v > 4000000000000LL) return 0; }
    return v;
}
inline std::string numsShown(const std::string& n) { const size_t p = n.rfind(" tk"); return p != std::string::npos && qAnchorOf(n) > 0 ? n.substr(0, p) : n; }
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
    int q = b.quality;
    bool partial = false, pending = false, unavailable = false, complete = false;
    for (const Ev* e : v) if (e->m.typed) {
        partial = partial || e->m.quality == rse::PARTIAL;
        pending = pending || e->m.quality == rse::PENDING;
        unavailable = unavailable || e->m.quality == rse::UNAVAILABLE;
        complete = complete || e->m.quality == rse::COMPLETE;
    }
    if (b.quality == rse::UNAVAILABLE || unavailable) return rse::UNAVAILABLE;
    if (b.quality == rse::PENDING || pending) return rse::PENDING;
    if (b.quality == rse::PARTIAL || partial) return rse::PARTIAL;
    if (b.quality == rse::COMPLETE) return rse::COMPLETE;
    (void)complete;
    return q;
}
inline int readinessOf(int flags, int stage, int quality, bool active) {
    if (quality == rse::UNAVAILABLE || (flags & F_UNAVAILABLE_CONTEXT)) return READ_UNAVAILABLE;
    if (quality == rse::PENDING || (flags & F_PENDING_CONTEXT)) return READ_PENDING;
    if (quality == rse::PARTIAL || (flags & F_PARTIAL_CONTEXT)) return READ_PARTIAL;
    if (quality != rse::COMPLETE) return active || stage > 0 ? READ_REVIEW : READ_UNKNOWN;
    if (stage > 0) return READ_CONFIRMED;
    return active ? READ_REVIEW : READ_UNKNOWN;
}

// Legacy/untyped, incomplete, and rejected letters stay visible in `here` and
// the chain with `*`; they cannot promote/fail a confirmed attempt.
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

// Stage compatibility helpers keep callers that only draw letter context from
// breaking; run() is the authoritative typed/provenance-aware classifier.
inline int stageOf(const Ev& e) {
    if (!typedComplete(e)) return 0;
    const std::string& c = e.l.code;
    if (e.pane == lst::ATK) return (c == "S" || c == "K" || c == "L") ? 1 : (c == "G" || c == "Tr") ? 3 : c == "R" ? 4 : 0;
    if (e.pane == lst::QUE) return rse::supportive(e.m.outcome) ? 2 : 0;
    if (c == "A") return 2;
    if (c == "P" && defenderBook(e)) return 2;
    if (c == "B" && defenderBook(e)) return FAIL;
    if (c == "H") return 4;
    if (c == "L") return 5;
    if (c == "X") return FAIL;
    return 0;
}
inline int stepOf(const Ev& e, int cur) {
    const int st = stageOf(e); const std::string& c = e.l.code;
    if (e.pane == lst::DEF && c == "H" && cur < 2) return 0;
    if (e.pane == lst::DEF && (c == "L" || c == "X") && cur < 4) return 0;
    if (e.pane == lst::DEF && c == "B" && st == FAIL && cur < 2) return 0;
    return st;
}

// Replays closed bars oldest first.  Frozen decisions are copied byte-for-byte
// as Steps and their persisted checkpoint, so late letters cannot invisibly
// alter later state.  Non-frozen Steps retain a true post-bar checkpoint.
inline std::vector<Step> run(const std::vector<Bar>& b, const std::multimap<long long, Ev>& evs, const std::map<long long, Step>* frozen = nullptr) {
    std::vector<Step> out; out.reserve(b.size());
    Attempt a;
    for (size_t i = 0; i < b.size(); ++i) {
        if (frozen) {
            const std::map<long long, Step>::const_iterator fixed = frozen->find(b[i].key);
            if (fixed != frozen->end()) {
                out.push_back(fixed->second); a = attemptOf(fixed->second.resume); continue;
            }
        }
        Step s; s.key = b[i].key;
        std::vector<const Ev*> all, here[2];
        for (std::multimap<long long, Ev>::const_iterator it = evs.lower_bound(b[i].key); it != evs.end() && it->first == b[i].key; ++it) {
            all.push_back(&it->second); here[it->second.l.pivot == HIGH ? 1 : 0].push_back(&it->second);
        }
        s.quality = stepQuality(b[i], all);
        for (const Ev* e : all) s.flags |= dataFlag(*e);

        // A bad / unavailable intermediate bar is a data gap, not neutral
        // evidence.  It cannot carry a defended state across the gap.
        if (!b[i].ok || b[i].quality != rse::COMPLETE) {
            if (a.on) { s.side = a.side; s.ext = a.ext; s.start = a.start; s.chain = a.chain; s.failedStage = a.stage; s.status = EXPIRED; s.why = "data unavailable; attempt not carried across an unobserved bar"; }
            a = Attempt(); s.quality = b[i].quality; s.readiness = readinessOf(s.flags, s.stage, s.quality, false); s.resume = checkpointOf(a); out.push_back(s); continue;
        }

        bool isLow = false, isHigh = false; extremeOf(b, i, isLow, isHigh);
        const double atr = atrAt(b, i);
        auto endAttempt = [&](int status, const std::string& why) {
            s.status = status; s.failedStage = a.stage; s.why = why; s.side = a.side; s.ext = a.ext; s.start = a.start; s.chain = a.chain; s.flags |= a.flags; a = Attempt();
        };
        // First advance/fail the existing attempt.  age exactly persists across
        // frozen windows and preserves the legacy 5/30-bar defaults.
        if (a.on) {
            ++a.age;
            const bool takenOut = a.side == LOW ? b[i].l < a.ext : b[i].h > a.ext;
            if (takenOut && a.stage >= 2) endAttempt(FAILED, a.side == LOW ? "the low was taken out" : "the high was taken out");
            else if (takenOut) a.ext = a.side == LOW ? b[i].l : b[i].h;
            else {
                const int other = 1 - a.side; const bool otherExt = other == LOW ? isLow : isHigh;
                if (otherExt && a.stage <= 1) endAttempt(EXPIRED, "a new extreme on the other side");
                else if ((a.stage == 0 && a.age >= IDLE_BARS) || a.age >= MAX_BARS) endAttempt(EXPIRED, "no progress in time");
            }
        }
        const bool ended = s.status != NONE;
        // A failed/expired attempt may be followed by a new strict extreme on
        // this same bar.  This is a bar fact, not a reordering of evidence.
        if (!a.on) {
            int sd = -1;
            if (isLow && isHigh) {
                // A tie is intentionally ambiguous.  Typed complete events may
                // break it only by counts, never a stage-rank cascade.
                int low = 0, high = 0;
                for (const Ev* e : here[LOW]) low += typedComplete(*e);
                for (const Ev* e : here[HIGH]) high += typedComplete(*e);
                sd = low > high ? LOW : high > low ? HIGH : -1;
            } else if (isLow) sd = LOW; else if (isHigh) sd = HIGH;
            if (sd >= 0) { a.on = true; a.side = sd; a.ext = sd == LOW ? b[i].l : b[i].h; a.start = b[i].key; a.age = 0; }
        }
        if (a.on) {
            std::vector<const Ev*> v = here[a.side];
            // Do not manufacture a causal path.  We sort only when *every*
            // same-bar observation has a distinct known receipt (else its
            // producer publication); tied/unknown summaries retain journal
            // order and cannot gain a synthetic rank ordering.
            bool fullyOrdered = !v.empty(); std::map<long long, int> pubs;
            for (const Ev* e : v) {
                const long long ord = orderOf(*e);
                if (!e->m.typed || ord <= 0 || ++pubs[ord] != 1) { fullyOrdered = false; break; }
            }
            if (fullyOrdered) std::stable_sort(v.begin(), v.end(), [](const Ev* x, const Ev* y) { return orderOf(*x) < orderOf(*y); });
            std::string part, hereText;
            bool failed = false;
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
                        a.stage = std::max(a.stage, 1); a.flags |= F_ATTACK; if (e->m.basis == rse::MBO && e->m.lots > 0) a.flags |= F_ATTACK_MBO;
                        // Pace is a separate execution-fading observation.  50
                        // is the existing E_SHOW100 display threshold, not tuned.
                        if (e->m.basis == rse::MBO && e->m.lots > 0 && e->m.pace100 >= 0 && e->m.pace100 <= E_SHOW100) { a.flags |= F_PACE_FADE; a.stage = std::max(a.stage, 3); }
                        applied = true;
                    }
                } else if (e->pane == lst::ATK && c == "E") {
                    if (e->m.basis != rse::MBO || !atZone || e->m.lots <= 0 || e->m.pace100 < 0 || e->m.pace100 > E_SHOW100) why = "needs measured complete MBO execution fade";
                    else { a.stage = std::max(a.stage, 3); a.flags |= F_ATTACK | F_ATTACK_MBO | F_PACE_FADE; applied = true; }
                } else if (e->pane == lst::ATK && (c == "G" || c == "Tr")) {
                    if (a.stage < 1) why = "needs prior attack";
                    else {
                        a.stage = std::max(a.stage, 3); a.flags |= c == "G" ? F_WEAKEN_G : F_WEAKEN_TR;
                        a.flags |= e->m.basis == rse::MBO ? F_WEAKEN_MBO : e->m.basis == rse::HYBRID ? F_WEAKEN_HYBRID : e->m.basis == rse::IMPORTED ? F_WEAKEN_IMPORTED : F_WEAKEN_UNKNOWN;
                        if ((a.flags & F_ATTACK_MBO) && e->m.pace100 >= 0 && e->m.pace100 <= E_SHOW100) a.flags |= F_PACE_FADE;
                        applied = true;
                    }
                } else if (e->pane == lst::ATK && c == "R") {
                    if (a.stage < 1) why = "needs current attack";
                    else if (e->m.basis != rse::HYBRID || e->m.cause <= 0 || !atZone) why = "needs hybrid reclaim with explicit current cause";
                    else { a.stage = std::max(a.stage, 4); a.anchor = e->m.cause; a.flags |= F_TURN_HYBRID; applied = true; }
                } else if (e->pane == lst::QUE && c == "Q") {
                    if (rse::supportive(e->m.outcome)) {
                        if (!atZone) why = "outside current attempt";
                        else if (!qAtExtreme(*e, a, atr)) why = "queue not at the attempt's own extreme";
                        else if (a.anchor && ecause != a.anchor) { a.flags |= F_LINEAGE_REJECTED; why = "different defended cause"; }
                        else { if (!a.anchor) a.anchor = ecause; a.stage = std::max(a.stage, 2); a.flags |= F_DEFENSE_QUEUE; applied = true; }
                    } else if (rse::adverse(e->m.outcome)) {
                        a.flags |= F_QUEUE_ADVERSE;
                        if (a.stage >= 2 && atZone && exactAnchor(*e, a)) { adverse = true; why = "queue broke/drained at the defended cause"; }
                        else { a.flags |= F_LINEAGE_REJECTED; why = "queue adverse warning not tied to an active defended cause"; }
                    } else { a.flags |= F_QUEUE_UNKNOWN; why = "queue outcome unknown"; }
                } else if (e->pane == lst::DEF && (c == "A" || c == "P")) {
                    if (c == "P" && !defenderBook(*e)) why = "attackers' book: review only";
                    else if (!atZone) why = "outside current attempt";
                    else if (a.anchor && ecause != a.anchor) { a.flags |= F_LINEAGE_REJECTED; why = "different defended cause"; }
                    else {
                        if (!a.anchor) a.anchor = ecause;
                        a.stage = std::max(a.stage, 2);
                        if (c == "P") a.flags |= F_DEFENSE_P;
                        else { a.flags |= F_DEFENSE_A; if (e->m.basis == rse::MBO && e->m.lots > 0) a.flags |= F_ABSORB_MBO; }
                        applied = true;
                    }
                } else if (e->pane == lst::DEF && c == "H") {
                    const bool parentMatches = e->m.parentCause > 0 && e->m.parentCause == (a.anchor > 0 ? a.anchor : a.start);
                    const bool linked = exactAnchor(*e, a) || parentMatches;
                    const bool attackPath = (a.flags & F_ATTACK_MBO) && parentMatches && e->m.basis == rse::MBO;
                    if (a.stage < 2 && !attackPath) why = "needs known defense or a linked MBO attack";
                    else if (e->m.lots <= 0 || e->m.auxLots <= 0 || !atZone || !linked) { a.flags |= F_LINEAGE_REJECTED; why = "needs complete executable response matching native pivot/turn lineage"; }
                    else { a.stage = std::max(a.stage, 4); a.anchor = e->m.cause; a.flags |= e->m.basis == rse::MBO ? F_TURN_MBO : e->m.basis == rse::HYBRID ? F_TURN_HYBRID : e->m.basis == rse::IMPORTED ? F_TURN_IMPORTED : F_TURN_UNKNOWN; applied = true; }
                } else if (e->pane == lst::DEF && c == "L") {
                    const bool closesHeld = knownPrice(*e) && (a.side == LOW ? b[i].c >= e->m.priceTicks : b[i].c <= e->m.priceTicks);
                    if (a.stage < 4) why = "needs prior turn";
                    else if (!exactAnchor(*e, a)) { a.flags |= F_LINEAGE_REJECTED; why = "different turn cause"; }
                    else if (e->m.auxLots <= 0 || !knownPrice(*e) || !closesHeld) { a.flags |= F_STOCKED_ONLY; why = "stocked book only; no executed held re-test"; }
                    else { a.stage = std::max(a.stage, 5); a.flags |= F_RETEST_EXECUTED_HELD; applied = true; }
                } else if (e->pane == lst::DEF && c == "X") {
                    if (a.stage < 4) why = "needs prior turn";
                    else if (!exactAnchor(*e, a) || !atZone) { a.flags |= F_LINEAGE_REJECTED; why = "different turn cause"; }
                    else { adverse = true; why = "matched turned re-test was adverse"; }
                } else if (e->pane == lst::DEF && c == "B") {
                    if (!defenderBook(*e)) why = "attackers' book: review only";
                    else if (a.stage < 2) why = "needs prior defense";
                    else if (!atZone || !exactAnchor(*e, a)) { a.flags |= F_LINEAGE_REJECTED; why = "different defended cause"; }
                    else { adverse = true; why = "defenders stepped back at the defended cause"; }
                } else why = "context only"; // W and unsupported combinations

                if (adverse) { appendToken(part, c + "!"); appendText(hereText, contextText(*e, why)); failed = true; break; }
                appendToken(part, c + (applied ? "" : "*")); appendText(hereText, contextText(*e, applied ? "" : why));
            }
            if (!part.empty()) a.chain += (a.chain.empty() ? "" : " . ") + part;
            s.here = hereText;
            if (failed) {
                const std::string reason = hereText.empty() ? std::string("matched adverse evidence") : hereText;
                endAttempt(FAILED, reason);
            } else if (a.on && atr > 0 && a.stage >= 4 && (a.flags & (F_TURN_MBO | F_TURN_HYBRID | F_TURN_IMPORTED | F_TURN_UNKNOWN)) && b[i].quality == rse::COMPLETE && (a.side == LOW ? (long double)b[i].c - a.ext : (long double)a.ext - b[i].c) >= atr) {
                s.here = hereText; a.stage = 6; s.stage = 6; s.status = COMPLETE; s.side = a.side; s.ext = a.ext; s.start = a.start; s.chain = a.chain + " . 1 ATR"; s.why = "the close left the extreme by 1 ATR after a confirmed turn"; s.flags = a.flags; a = Attempt();
            } else if (!ended) {
                s.side = a.side; s.stage = a.stage; s.status = a.stage > 0 ? FORMING : NONE; s.ext = a.ext; s.start = a.start; s.chain = a.chain;
            } else if (a.stage > 0) {
                // The visual Step remains the old terminal status, while its
                // resume checkpoint faithfully records the new post-bar attempt.
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

// ---- immutable decision record -------------------------------------------------
// RS|2|key|side|stage|status|ext|start|failedStage|quality|flags|readiness|decided|
//      resumeOn|resumeSide|resumeStage|resumeFlags|resumeExt|resumeStart|resumeAnchor|
//      resumeAge|resumeChain|why|chain|here
inline std::string cleanF(const std::string& s) { std::string o; for (char c : s) if (c != '|' && c != '\r' && c != '\n') o += c; return o.size() > 200 ? o.substr(0, 200) : o; }
inline std::string line(const Step& s) {
    char b[360];
    std::snprintf(b, sizeof b, "RS|2|%lld|%d|%d|%d|%lld|%lld|%d|%d|%d|%d|%lld|%d|%d|%d|%d|%lld|%lld|%lld|%d|",
        s.key, s.side, s.stage, s.status, s.ext, s.start, s.failedStage, s.quality, s.flags, s.readiness, s.decided,
        s.resume.on ? 1 : 0, s.resume.side, s.resume.stage, s.resume.flags, s.resume.ext, s.resume.start, s.resume.anchor, s.resume.age);
    return std::string(b) + cleanF(s.resume.chain) + "|" + cleanF(s.why) + "|" + cleanF(s.chain) + "|" + cleanF(s.here);
}
inline bool parseNumber(const std::string& f, long long& n) { return lst::fullLL(f, n); }
inline bool validCheckpoint(const Checkpoint& c) {
    if (!c.on) return c.side == -1 && c.stage == 0 && c.flags == 0 && c.ext == 0 && c.start == 0 && c.anchor == 0 && c.age == 0 && c.chain.empty();
    return c.side >= LOW && c.side <= HIGH && c.stage >= 0 && c.stage <= 6 && c.flags >= 0 && c.start > 0 && c.age >= 0 && c.anchor >= 0;
}
inline bool parse(const std::string& s0, Step& s) {
    std::string t = s0; while (!t.empty() && (t.back() == '\r' || t.back() == '\n')) t.pop_back();
    std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t p = t.find('|', a); f.push_back(t.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; if (f.size() > 25) return false; }
    if (f.size() == 12 && f[0] == "RS" && f[1] == "1") {
        long long key = 0, side = 0, stage = 0, st = 0, ext = 0, start = 0, fs = 0;
        if (!parseNumber(f[2], key) || key <= 0 || !parseNumber(f[3], side) || side < -1 || side > 1 || !parseNumber(f[4], stage) || stage < 0 || stage > 6 || !parseNumber(f[5], st) || st < NONE || st > EXPIRED || !parseNumber(f[6], ext) || !parseNumber(f[7], start) || start < 0 || !parseNumber(f[8], fs) || fs < 0 || fs > 6 || (side < 0 && (stage != 0 || st != NONE))) return false;
        Step x; x.key = key; x.side = (int)side; x.stage = (int)stage; x.status = (int)st; x.ext = ext; x.start = start; x.failedStage = (int)fs; x.why = f[9]; x.chain = f[10]; x.here = f[11];
        // RS|1 has no durable post-bar state.  It remains display-compatible but
        // is never trusted as a v2 replay checkpoint.
        s = x; return true;
    }
    if (f.size() != 25 || f[0] != "RS" || f[1] != "2") return false;
    long long key = 0, side = 0, stage = 0, st = 0, ext = 0, start = 0, fs = 0, q = 0, flags = 0, ready = 0, decided = 0;
    long long on = 0, rs = 0, rst = 0, rf = 0, rext = 0, rstart = 0, ranchor = 0, rage = 0;
    if (!parseNumber(f[2], key) || key <= 0 || !parseNumber(f[3], side) || side < -1 || side > 1 || !parseNumber(f[4], stage) || stage < 0 || stage > 6 || !parseNumber(f[5], st) || st < NONE || st > EXPIRED || !parseNumber(f[6], ext) || !parseNumber(f[7], start) || start < 0 || !parseNumber(f[8], fs) || fs < 0 || fs > 6 || !parseNumber(f[9], q) || q < rse::UNVERIFIED || q > rse::PENDING || !parseNumber(f[10], flags) || flags < 0 || flags >= (1LL << 26) || !parseNumber(f[11], ready) || ready < READ_UNKNOWN || ready > READ_PENDING || !parseNumber(f[12], decided) || decided < 0 || !parseNumber(f[13], on) || (on != 0 && on != 1) || !parseNumber(f[14], rs) || !parseNumber(f[15], rst) || !parseNumber(f[16], rf) || rf < 0 || rf >= (1LL << 26) || !parseNumber(f[17], rext) || !parseNumber(f[18], rstart) || !parseNumber(f[19], ranchor) || !parseNumber(f[20], rage) || rage > INT_MAX) return false;
    if (side < 0 && (stage != 0 || st != NONE)) return false;
    Checkpoint c; c.on = on != 0; c.side = (int)rs; c.stage = (int)rst; c.flags = (int)rf; c.ext = rext; c.start = rstart; c.anchor = ranchor; c.age = (int)rage; c.chain = f[21];
    if (rs < -1 || rs > 1 || rst < 0 || rst > 6 || rf < 0 || rage < 0 || !validCheckpoint(c)) return false;
    // line() emits ...age|resumeChain|why|chain|here.  The exact 25-field
    // count above rejects a truncated or extended journal line.
    Step x; x.key = key; x.side = (int)side; x.stage = (int)stage; x.status = (int)st; x.ext = ext; x.start = start; x.failedStage = (int)fs; x.quality = (int)q; x.flags = (int)flags; x.readiness = (int)ready; x.decided = decided; x.resume = c; x.why = f[22]; x.chain = f[23]; x.here = f[24];
    s = x; return true;
}
inline std::string fileName(const std::string& mkt, int spb) { return "RS2-" + (mkt == "6E" ? std::string("EU") : mkt) + "-" + std::to_string(spb) + ".csv"; }
inline std::string fileName(const std::string& mkt, const std::string& contract, int spb) { return "RS2-" + (mkt == "6E" ? std::string("EU") : mkt) + "-" + rse::token(contract) + "-" + std::to_string(spb) + ".csv"; }

// ---- drawing/layout helpers retained for the host --------------------------------
struct Look { int height = 0; int colour = lst::GREEN; bool dim = false; bool stub = false; };
inline Look lookOf(const Step& s) {
    Look k; if (s.side < 0) return k;
    k.colour = s.side == LOW ? lst::GREEN : lst::RED;
    if (s.status == FAILED && s.stage == 0) { k.stub = true; k.dim = true; return k; }
    if (s.status == EXPIRED && s.stage == 0) return k;
    k.height = s.stage; k.dim = s.stage <= 1; return k;
}
inline bool boxAt(int pos, int l, int t, int r, int b, int w, int h, int margin, int& x, int& y) {
    if (pos < 0 || pos > 8) pos = 0;
    if (r - l < w + 2 * margin || b - t < h + 2 * margin) return false;
    const int col = pos % 3, row = pos / 3;
    x = col == 0 ? l + margin : col == 1 ? l + (r - l - w) / 2 : r - margin - w;
    y = row == 0 ? t + margin : row == 1 ? t + (b - t - h) / 2 : b - margin - h;
    return true;
}
static const double AXIS_MAX = 6.5;
inline int yOf(double v, int top, int bottom) { return bottom - (int)std::lround(v / AXIS_MAX * (double)(bottom - top)); }
inline std::string sideWord(int side) { return side == LOW ? "LOW" : side == HIGH ? "HIGH" : "-"; }
inline std::string evidenceSummary(int f) {
    std::vector<std::string> p;
    if (f & F_ABSORB_MBO) p.push_back("MBO absorption");
    if (f & F_DEFENSE_P) p.push_back("P/book support");
    if (f & F_DEFENSE_QUEUE) p.push_back("held/refilled queue");
    if (f & F_WEAKEN_G) p.push_back("G withdrawal");
    if (f & F_WEAKEN_TR) p.push_back("Tr weakening");
    if (f & F_WEAKEN_MBO) p.push_back("MBO weakening");
    if (f & F_WEAKEN_HYBRID) p.push_back("hybrid weakening");
    if (f & F_WEAKEN_IMPORTED) p.push_back("imported weakening");
    if (f & F_WEAKEN_UNKNOWN) p.push_back("unknown-source weakening");
    if (f & F_PACE_FADE) p.push_back("execution pace fade");
    if (f & F_TURN_MBO) p.push_back("MBO turn");
    if (f & F_TURN_HYBRID) p.push_back("hybrid turn");
    if (f & F_TURN_IMPORTED) p.push_back("imported turn");
    if (f & F_TURN_UNKNOWN) p.push_back("unknown-source turn");
    if (f & F_RETEST_EXECUTED_HELD) p.push_back("executed held re-test");
    if (f & F_QUEUE_ADVERSE) p.push_back("adverse queue");
    if (f & F_STOCKED_ONLY) p.push_back("stocked-only L");
    if (p.empty()) return "none";
    std::string o; for (size_t i = 0; i < p.size(); ++i) o += (i ? ", " : "") + p[i]; return o;
}
inline std::vector<std::string> nowRows(const Step& s, const std::string& when) {
    std::vector<std::string> r; r.push_back("Now " + when);
    if (s.side < 0 || (s.stage == 0 && s.status != FAILED)) { r.push_back("No attempt"); r.push_back("Phase -"); r.push_back("Evidence " + std::string(rse::qualityName(s.quality)) + ", " + readinessName(s.readiness)); }
    else {
        r.push_back(sideWord(s.side) + " attempt");
        if (s.status == FAILED && s.stage == 0) r.push_back("Phase " + std::to_string(s.failedStage) + ", failed");
        else if (s.status == COMPLETE) r.push_back("Phase 6, complete");
        else r.push_back("Phase " + std::to_string(s.stage) + " " + stageName(s.stage) + (s.status == FAILED ? " (last failed)" : ""));
        r.push_back("Data " + std::string(rse::qualityName(s.quality)) + ", evidence " + readinessName(s.readiness));
        r.push_back("Evidence " + evidenceSummary(s.flags));
        r.push_back("Chain " + (s.chain.empty() ? std::string("-") : s.chain));
    }
    r.push_back(std::string("Odds ?  (") + ODDS_TICKET + ")"); return r;
}
inline std::vector<std::string> hoverRows(const Step& s, const std::string& when) {
    std::vector<std::string> r;
    const std::string piv = s.side == LOW ? "LOW" : s.side == HIGH ? "HIGH" : "-";
    std::string head = "Reversal stage  " + when + "  -  " + piv + " attempt";
    if (s.status == FAILED && s.stage == 0) head += ", failed at phase " + std::to_string(s.failedStage);
    else head += ", phase " + std::to_string(s.stage) + " " + stageName(s.stage) + (s.status == COMPLETE ? " (complete)" : s.status == FAILED ? " (the attempt before failed)" : "");
    r.push_back(head);
    r.push_back("Question: which observed phase is this local pivot in - attack, defense, weakening, turn, re-test, expansion?");
    r.push_back("   Data: " + std::string(rse::qualityName(s.quality)) + "; evidence: " + readinessName(s.readiness));
    r.push_back("   Evidence: " + evidenceSummary(s.flags));
    r.push_back("   Letters behind it: " + (s.chain.empty() ? std::string("none yet (the 5-candle extreme only)") : s.chain));
    if (!s.here.empty()) r.push_back("   On this candle: " + s.here);
    if (!s.why.empty()) r.push_back("   " + std::string(s.status == FAILED ? "Failed: " : s.status == EXPIRED ? "Ended: " : "") + s.why);
    r.push_back("   Use: phase, evidence readiness, and data quality are separate; neither implies odds or a trade entry.");
    r.push_back("   A stocked L alone is context, not proof that a re-test held.");
    r.push_back(std::string("   Odds: ? - not proven (" ) + ODDS_TICKET + ")");
    return r;
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
} // namespace rsl
#endif
