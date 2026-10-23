// SDK-free ReversalStageLogic 1.2.0 semantic fixtures.  Synthetic only: no
// market replay, performance, probability, or trade-result claim.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie -I.. test_stage_core.cpp -o /tmp/test_stage_core && /tmp/test_stage_core
#include "../ReversalStageLogic.h"
#include <cstdio>
#include <map>
#include <vector>

static int fails = 0, checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { ++fails; std::printf("FAIL %d: %s\n", __LINE__, #x); } } while (0)

static long long keyOf(int i) { return 1000LL + 180LL * i; }
static std::vector<rsl::Bar> bars(int last = 15) {
    std::vector<rsl::Bar> b;
    for (int i = 0; i <= last; ++i) {
        rsl::Bar x; x.key = keyOf(i); x.h = 1004; x.l = 1000; x.c = 1002; x.ok = true; x.quality = rse::COMPLETE;
        if (i == 10) { x.h = 1001; x.l = 990; x.c = 992; }
        else if (i > 10) { x.h = 995; x.l = 991; x.c = 993; }
        b.push_back(x);
    }
    return b;
}
static rsl::Ev event(int i, int pane, const char* code, long long cause, long long price, int basis = rse::MBO, int outcome = rse::NA, long long lots = 100, long long aux = -1, long long pace = -1, long long received = -1) {
    rsl::Ev e; e.key = keyOf(i); e.sourceKey = keyOf(i); e.received = received >= 0 ? received : 10000 + i * 20;
    e.pane = pane; e.l = lst::make(pane == lst::QUE ? lst::DEF : pane, code, lst::LOW, pane == lst::DEF && std::string(code) == "P" ? 0 : -1);
    e.m.typed = true; e.m.basis = basis; e.m.quality = rse::COMPLETE; e.m.outcome = outcome; e.m.cause = cause; e.m.published = 9000 + i;
    e.m.priceTicks = price; e.m.lots = lots; e.m.auxLots = aux; e.m.pace100 = pace; e.m.id = "synthetic" + std::to_string(i) + code; e.m.contract = "ESZ6";
    return e;
}
static void add(std::multimap<long long, rsl::Ev>& e, const rsl::Ev& x) { e.insert(std::make_pair(x.key, x)); }
static const rsl::Step& step(const std::vector<rsl::Step>& s, int i) { return s[(size_t)i]; }
static std::map<long long, rsl::Step> freezeTo(const std::vector<rsl::Step>& s, int last) {
    std::map<long long, rsl::Step> f; for (int i = 0; i <= last; ++i) f[s[(size_t)i].key] = s[(size_t)i]; return f;
}

int main() {
    CHECK(std::string(rsl::VERSION) == "1.3.0");
    CHECK(std::string(rsl::stageName(2)) == "Defense" && std::string(rsl::stageName(3)) == "Attack weakening");

    // Held/refilled queue can support Defense; broken/drained never does and
    // only fails after the event is tied to an already defended anchor.
    {
        std::multimap<long long, rsl::Ev> e; add(e, event(10, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::HELD));
        const std::vector<rsl::Step> s = rsl::run(bars(), e);
        CHECK(step(s, 10).stage == 2 && (step(s, 10).flags & rsl::F_DEFENSE_QUEUE));
        std::multimap<long long, rsl::Ev> broken; add(broken, event(10, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::HELD)); add(broken, event(11, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::BROKE));
        const std::vector<rsl::Step> q = rsl::run(bars(), broken);
        CHECK(step(q, 11).status == rsl::FAILED && step(q, 11).failedStage == 2 && (step(q, 11).flags & rsl::F_QUEUE_ADVERSE));
        std::multimap<long long, rsl::Ev> lone; add(lone, event(10, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::DRAINED));
        const std::vector<rsl::Step> d = rsl::run(bars(), lone);
        CHECK(step(d, 10).stage == 0 && step(d, 10).status != rsl::FAILED && (step(d, 10).flags & rsl::F_QUEUE_ADVERSE));
    }

    // (1.3.0 merge, keeps fix-build 1.2.0 #7 "NOW HIGH stage 2 failed chain Q") a held queue steps only AT the
    // attempt's own extreme: within max(4 ticks, 0.3 ATR); further inside the bar = hover only.
    {
        std::multimap<long long, rsl::Ev> near; add(near, event(10, lst::QUE, "Q", keyOf(10), 994, rse::MBO, rse::HELD));
        CHECK(step(rsl::run(bars(), near), 10).stage == 2);
        std::multimap<long long, rsl::Ev> far; add(far, event(10, lst::QUE, "Q", keyOf(10), 1000, rse::MBO, rse::HELD));
        const std::vector<rsl::Step> s = rsl::run(bars(), far);
        CHECK(step(s, 10).stage == 0 && !(step(s, 10).flags & rsl::F_DEFENSE_QUEUE) && step(s, 10).here.find("own extreme") != std::string::npos);
        CHECK(rsl::qAnchorOf("broke 4% q0 24 tk29115") == 29115 && rsl::qAnchorOf("q0 24") == 0 && rsl::qAnchorOf("x tk12a") == 0 && rsl::qTolTicks(40) == 12 && rsl::qTolTicks(2) == 4);
        CHECK(rsl::numsShown("held 120% q0 130 tk991") == "held 120% q0 130");
        std::multimap<long long, rsl::Ev> lg; rsl::Ev l = event(10, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::HELD); l.m.typed = false; l.l.nums = "held 120% q0 130 tk990"; add(lg, l);
        const std::vector<rsl::Step> g = rsl::run(bars(), lg);
        CHECK(step(g, 10).stage == 0 && step(g, 10).here.find("tk990") == std::string::npos);   // legacy: review only, anchor token hidden
    }

    // Legacy and non-complete typed inputs remain exposed context, never a
    // confirmed phase.  Unknown queue outcome is likewise not supportive.
    {
        std::multimap<long long, rsl::Ev> e; rsl::Ev legacy = event(10, lst::ATK, "K", keyOf(10), 990); legacy.m.typed = false; add(e, legacy);
        rsl::Ev partial = event(10, lst::DEF, "A", keyOf(10), 990); partial.m.quality = rse::PARTIAL; add(e, partial);
        add(e, event(10, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::NA));
        const std::vector<rsl::Step> s = rsl::run(bars(), e);
        CHECK(step(s, 10).stage == 0 && (step(s, 10).flags & rsl::F_LEGACY_CONTEXT) && (step(s, 10).flags & rsl::F_PARTIAL_CONTEXT));
        CHECK((step(s, 10).flags & rsl::F_QUEUE_UNKNOWN) && step(s, 10).readiness == rsl::READ_PARTIAL);
    }

    // P can establish Defense, but it is not called verified MBO absorption;
    // a complete MBO A with executed lots carries the separate absorption flag.
    {
        std::multimap<long long, rsl::Ev> p; add(p, event(10, lst::DEF, "P", keyOf(10), 990, rse::HYBRID));
        const rsl::Step ps = step(rsl::run(bars(), p), 10);
        CHECK(ps.stage == 2 && (ps.flags & rsl::F_DEFENSE_P) && !(ps.flags & rsl::F_ABSORB_MBO));
        std::multimap<long long, rsl::Ev> a; add(a, event(10, lst::DEF, "A", keyOf(10), 990, rse::MBO, rse::NA, 200));
        const rsl::Step as = step(rsl::run(bars(), a), 10);
        CHECK(as.stage == 2 && (as.flags & rsl::F_DEFENSE_A) && (as.flags & rsl::F_ABSORB_MBO));
    }

    // The existing e100<=50 observation is labeled independently from G.
    {
        std::multimap<long long, rsl::Ev> pace; add(pace, event(10, lst::ATK, "K", keyOf(10), 990, rse::MBO, rse::NA, 300, -1, 50));
        const rsl::Step p = step(rsl::run(bars(), pace), 10);
        CHECK(p.stage == 3 && (p.flags & rsl::F_PACE_FADE) && !(p.flags & rsl::F_WEAKEN_G));
        std::multimap<long long, rsl::Ev> g; add(g, event(10, lst::ATK, "K", keyOf(10), 990)); add(g, event(11, lst::ATK, "G", keyOf(10), 991, rse::HYBRID));
        const rsl::Step gs = step(rsl::run(bars(), g), 11);
        CHECK(gs.stage == 3 && (gs.flags & rsl::F_WEAKEN_G) && (gs.flags & rsl::F_WEAKEN_HYBRID) && !(gs.flags & rsl::F_PACE_FADE));
        std::multimap<long long, rsl::Ev> reclaim; add(reclaim, event(10, lst::ATK, "K", keyOf(10), 990)); add(reclaim, event(11, lst::ATK, "R", keyOf(10), 991, rse::HYBRID));
        const rsl::Step rs = step(rsl::run(bars(), reclaim), 11);
        CHECK(rs.stage == 4 && (rs.flags & rsl::F_TURN_HYBRID));
    }

    // H/L/X with an older unrelated cause cannot be attached merely because
    // their LOW side agrees with the current pivot.
    {
        std::multimap<long long, rsl::Ev> e; add(e, event(10, lst::DEF, "A", keyOf(10), 990));
        add(e, event(11, lst::DEF, "H", keyOf(9), 991)); add(e, event(12, lst::DEF, "L", keyOf(9), 991, rse::MBO, rse::NA, 10, 20)); add(e, event(13, lst::DEF, "X", keyOf(9), 991));
        const std::vector<rsl::Step> s = rsl::run(bars(), e);
        CHECK(step(s, 11).stage == 2 && step(s, 12).stage == 2 && step(s, 13).status != rsl::FAILED);
        CHECK((step(s, 13).flags & rsl::F_LINEAGE_REJECTED) != 0);
    }

    // L needs actual attacking retest lots, a known line, held close, and the
    // exact established turn cause; visible stock alone is only hover context.
    {
        std::multimap<long long, rsl::Ev> stocked; add(stocked, event(10, lst::DEF, "A", keyOf(10), 990)); add(stocked, event(11, lst::DEF, "H", keyOf(10), 991, rse::MBO, rse::NA, 100, 50)); add(stocked, event(12, lst::DEF, "L", keyOf(10), 991, rse::MBO, rse::NA, 20, 0));
        const rsl::Step l0 = step(rsl::run(bars(), stocked), 12);
        CHECK(l0.stage == 4 && (l0.flags & rsl::F_STOCKED_ONLY) && !(l0.flags & rsl::F_RETEST_EXECUTED_HELD));
        std::multimap<long long, rsl::Ev> held; add(held, event(10, lst::DEF, "A", keyOf(10), 990)); add(held, event(11, lst::DEF, "H", keyOf(10), 991, rse::MBO, rse::NA, 100, 50)); add(held, event(12, lst::DEF, "L", keyOf(10), 991, rse::MBO, rse::NA, 20, 25));
        const rsl::Step l1 = step(rsl::run(bars(), held), 12);
        CHECK(l1.stage == 5 && (l1.flags & rsl::F_RETEST_EXECUTED_HELD));
    }

    // Price excursion after Attack is not Expansion.  A confirmed Turn plus a
    // complete bar and the unchanged >=1 ATR excursion can complete phase 6.
    {
        std::vector<rsl::Bar> b = bars(); b[11].c = 1005; b[11].h = 1007;
        std::multimap<long long, rsl::Ev> attack; add(attack, event(10, lst::ATK, "S", keyOf(10), 990));
        const rsl::Step only = step(rsl::run(b, attack), 11);
        CHECK(only.stage != 6 && only.status != rsl::COMPLETE);
        std::vector<rsl::Bar> turnBars = bars(); turnBars[12].c = 1005; turnBars[12].h = 1007;
        std::multimap<long long, rsl::Ev> turn; add(turn, event(10, lst::DEF, "A", keyOf(10), 990)); add(turn, event(11, lst::DEF, "H", keyOf(10), 991, rse::MBO, rse::NA, 100, 50));
        const rsl::Step expanded = step(rsl::run(turnBars, turn), 12);
        CHECK(expanded.stage == 6 && expanded.status == rsl::COMPLETE && (expanded.flags & rsl::F_TURN_MBO));
    }

    // Reversed same-publish/receipt summaries must not be stage-ranked into
    // A->H.  H sees no previously established defense and stays review-only.
    {
        std::multimap<long long, rsl::Ev> e; add(e, event(10, lst::DEF, "H", keyOf(10), 991, rse::MBO, rse::NA, 100, -1, -1, 777)); add(e, event(10, lst::DEF, "A", keyOf(10), 990, rse::MBO, rse::NA, 100, -1, -1, 777));
        const rsl::Step s = step(rsl::run(bars(), e), 10);
        CHECK(s.stage == 2 && !(s.flags & rsl::F_TURN_MBO) && s.chain.find("H*") != std::string::npos);
    }

    // A frozen decision restores only its durable resume state.  Late A at a
    // frozen source bar cannot make a later H retroactively establish Turn.
    {
        std::multimap<long long, rsl::Ev> original; add(original, event(10, lst::ATK, "K", keyOf(10), 990));
        const std::vector<rsl::Step> first = rsl::run(bars(), original); const std::map<long long, rsl::Step> frozen = freezeTo(first, 10);
        std::multimap<long long, rsl::Ev> late = original; add(late, event(10, lst::DEF, "A", keyOf(10), 990)); add(late, event(11, lst::DEF, "H", keyOf(10), 991, rse::MBO, rse::NA, 100, 50));
        const std::vector<rsl::Step> resumed = rsl::run(bars(), late, &frozen);
        CHECK(resumed[10] == first[10] && resumed[11].stage == 1 && !(resumed[11].flags & rsl::F_TURN_MBO));
    }

    // RS2 has a strict complete layout and restart parity using its checkpoints;
    // RS1 is display-compatible but carries no v2 resume checkpoint.
    {
        std::multimap<long long, rsl::Ev> e; add(e, event(10, lst::DEF, "A", keyOf(10), 990)); add(e, event(11, lst::DEF, "H", keyOf(10), 991, rse::MBO, rse::NA, 100, 50));
        std::vector<rsl::Step> full = rsl::run(bars(), e); full[11].decided = 123456;
        rsl::Step parsed; const std::string rs2 = rsl::line(full[11]);
        CHECK(rsl::parse(rs2, parsed) && parsed == full[11]);
        CHECK(!rsl::parse(rs2 + "|extra", parsed) && !rsl::parse("RS|2|bad|-1|0|0|0|0|0|0|0|0|0|0|-1|0|0|0|0|0|0||||", parsed));
        CHECK(rsl::parse("RS|1|100|-1|0|0|0|0|0|||", parsed) && !parsed.resume.on && parsed.quality == rse::UNVERIFIED);
        const std::map<long long, rsl::Step> f = freezeTo(full, 11); const std::vector<rsl::Step> restart = rsl::run(bars(), e, &f);
        CHECK(restart.size() == full.size()); for (size_t i = 0; i < full.size(); ++i) CHECK(restart[i] == full[i]);
    }

    // Native next-candle turn: preserve the earlier pivot parent, then match L
    // against the later turn key. No quantity-free H or stocked-only L.
    {
        auto b = bars(); std::multimap<long long, rsl::Ev> e;
        add(e, event(10, lst::DEF, "A", keyOf(10), 990));
        auto h = event(12, lst::DEF, "H", keyOf(11), 991, rse::MBO, rse::NA, 100, 50); h.m.parentCause = keyOf(10); add(e,h);
        add(e,event(13,lst::DEF,"L",keyOf(11),991,rse::MBO,rse::NA,100,20));
        auto r = rsl::run(b,e); CHECK(r[12].stage == 4 && r[12].resume.anchor == keyOf(11)); CHECK(r[13].stage == 5);
        h.m.parentCause = keyOf(9); e.clear(); add(e,event(10,lst::DEF,"A",keyOf(10),990)); add(e,h);
        CHECK(rsl::run(b,e)[12].stage == 2);
        e.clear(); add(e,event(10,lst::ATK,"K",keyOf(10),990)); h.m.parentCause=keyOf(10); add(e,h);
        CHECK(rsl::run(b,e)[12].stage == 4); // linked attack/counter route, not invented absorption
        h.m.auxLots = -1; e.clear(); add(e,event(10,lst::ATK,"K",keyOf(10),990)); add(e,h);
        CHECK(rsl::run(b,e)[12].stage != 4);
    }
    {
        std::multimap<long long,rsl::Ev> e; add(e,event(10,lst::ATK,"E",keyOf(10),990,rse::MBO,rse::NA,100,-1,50));
        CHECK(rsl::run(bars(),e)[10].stage == 3); e.begin()->second.m.pace100=51;
        CHECK(rsl::run(bars(),e)[10].stage == 0);
        e.begin()->second.m.pace100=50; e.begin()->second.m.quality=rse::UNVERIFIED;
        CHECK(rsl::run(bars(),e)[10].stage == 0);
        CHECK(lst::stripOf(lst::ATK,"E")==1 && lst::rankOf(lst::ATK,"Tr")<lst::rankOf(lst::ATK,"E") && lst::rankOf(lst::ATK,"E")<lst::rankOf(lst::ATK,"G"));
    }
    {
        for (int q: {rse::UNVERIFIED,rse::PARTIAL,rse::PENDING,rse::UNAVAILABLE}) {
            auto b=bars(); b[10].quality=q; std::multimap<long long,rsl::Ev> e; add(e,event(10,lst::DEF,"A",keyOf(10),990));
            const auto r=rsl::run(b,e); CHECK(r[10].stage==0 && r[10].readiness!=rsl::READ_CONFIRMED && !r[10].resume.on);
        }
        auto b=bars(); for (auto& x:b) {x.h=LLONG_MAX; x.l=LLONG_MIN; x.c=0;}
        CHECK(std::isfinite(rsl::atrAt(b,10))); // integer subtraction cannot overflow
    }
    std::printf("%s: %d checks, %d failed\n", fails ? "FAILED" : "PASSED", checks, fails);
    return fails ? 1 : 0;
}
