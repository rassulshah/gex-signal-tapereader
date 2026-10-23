// SDK-free ReversalStageLogic 2.0.0 fixtures (the 4-step model: gate, Trap, Collision, Trapped, Release, result).
// Synthetic only: no market replay, performance, probability, or trade-result claim.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie -I.. test_stage_core.cpp -o /tmp/test_stage_core && /tmp/test_stage_core
#include "../ReversalStageLogic.h"
#include <cstdio>
#include <map>
#include <vector>

static int fails = 0, checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { ++fails; std::printf("FAIL %d: %s\n", __LINE__, #x); } } while (0)

static long long keyOf(int i) { return 1000LL + 180LL * i; }
// bars 0-9 range 1000-1004; bar 10 = the 5-candle low 990 (close 992); later bars 991-995 close 993
static std::vector<rsl::Bar> bars(int last = 20) {
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
    e.pane = pane; e.l = lst::make(pane == lst::QUE ? lst::DEF : pane, code, lst::LOW, pane == lst::DEF && (std::string(code) == "P" || std::string(code) == "B") ? 0 : -1);
    e.m.typed = true; e.m.basis = basis; e.m.quality = rse::COMPLETE; e.m.outcome = outcome; e.m.cause = cause; e.m.published = 9000 + i;
    e.m.priceTicks = price; e.m.lots = lots; e.m.auxLots = aux; e.m.pace100 = pace; e.m.id = "synthetic" + std::to_string(i) + code + std::to_string(price); e.m.contract = "ESZ6";
    return e;
}
static void add(std::multimap<long long, rsl::Ev>& e, const rsl::Ev& x) { e.insert(std::make_pair(x.key, x)); }
static std::map<long long, rsl::Step> freezeTo(const std::vector<rsl::Step>& s, int last) {
    std::map<long long, rsl::Step> f; for (int i = 0; i <= last; ++i) f[s[(size_t)i].key] = s[(size_t)i]; return f;
}
static rsl::Gate level(long long px, const char* name) { rsl::Gate g; g.px = px; g.dir = -1; g.kind = rsl::G_KEY; g.label = name; return g; }
static rsl::Gate swingLow(long long px, int piv, int conf) { rsl::Gate g; g.px = px; g.dir = -1; g.kind = rsl::G_SWING; g.pivKey = keyOf(piv); g.fromKey = keyOf(conf); g.label = "swing low 8:48 AM"; return g; }
// the collision used by most blocks: K 300 lots (avg fill 990.00) + A on the extreme candle (or resBar)
static std::multimap<long long, rsl::Ev> collision(int resBar = 10) {
    std::multimap<long long, rsl::Ev> e; rsl::Ev k = event(10, lst::ATK, "K", keyOf(10), 990, rse::MBO, rse::NA, 300); k.l.nums = "300 lots vw99000"; add(e, k);
    add(e, event(resBar, lst::DEF, "A", keyOf(10), 990, rse::MBO, rse::NA, 200)); return e;
}

int main() {
    CHECK(std::string(rsl::VERSION) == "2.0.0");
    CHECK(std::string(rsl::stageName(1)) == "Trap" && std::string(rsl::stageName(2)) == "Collision" && std::string(rsl::stageName(3)) == "Trapped" && std::string(rsl::stageName(4)) == "Release" && std::string(rsl::stageName(5)).empty());

    // ---- THE GATE ----
    {
        std::multimap<long long, rsl::Ev> none; std::vector<rsl::Gate> g;
        const std::vector<rsl::Step> off = rsl::run(bars(), none);                  // gate off (nullptr): the extreme alone starts
        CHECK(off[10].resume.on && off[10].side == rsl::LOW && off[10].stage == 0);
        const std::vector<rsl::Step> s0 = rsl::run(bars(), none, nullptr, &g);       // no location = no attempt
        CHECK(!s0[10].resume.on && s0[10].side == -1 && s0[10].here.find("no attempt") != std::string::npos);
        g.push_back(level(992, "LonLO"));
        const std::vector<rsl::Step> s1 = rsl::run(bars(), none, nullptr, &g);
        CHECK(s1[10].resume.on && s1[10].resume.gate == "LonLO" && (s1[10].flags & rsl::F_GATED) && s1[10].gate == "LonLO" && s1[10].gatePx == 992);
        g[0].px = 1010; CHECK(!rsl::run(bars(), none, nullptr, &g)[10].resume.on);   // far above
        g[0].px = 980; CHECK(!rsl::run(bars(), none, nullptr, &g)[10].resume.on);    // more than max(tol, 0.5 ATR) through it
        g[0].dir = 1; g[0].px = 992; CHECK(!rsl::run(bars(), none, nullptr, &g)[10].resume.on);   // a high level does not gate a low
        g[0].dir = 0; g[0].kind = rsl::G_MQ; g[0].label = "MQ HVL"; CHECK(rsl::run(bars(), none, nullptr, &g)[10].resume.gate == "MQ HVL");   // MenthorQ: either side
        std::vector<rsl::Gate> sw; sw.push_back(swingLow(992, 3, 5));
        CHECK(rsl::run(bars(), none, nullptr, &sw)[10].resume.gate == "swing low 8:48 AM");
        sw[0].fromKey = keyOf(11); CHECK(!rsl::run(bars(), none, nullptr, &sw)[10].resume.on);   // confirmed only after the extreme: no look-ahead
        sw[0].fromKey = keyOf(5); std::vector<rsl::Bar> b = bars(); b[4].l = 991;   // traded through after its candle = not intact
        CHECK(!rsl::run(b, none, nullptr, &sw)[10].resume.on);
        // last 3: a 4th-back swing does not count; 2 back counts only when below 1 back
        std::vector<rsl::Gate> four; four.push_back(swingLow(992, 1, 2)); four.push_back(swingLow(1020, 3, 4)); four.push_back(swingLow(1015, 5, 6)); four.push_back(swingLow(1010, 7, 8));
        CHECK(!rsl::run(bars(), none, nullptr, &four)[10].resume.on);
        std::vector<rsl::Gate> two; two.push_back(swingLow(991, 3, 4)); two.push_back(swingLow(992, 6, 7));
        CHECK(rsl::run(bars(), none, nullptr, &two)[10].resume.gatePx == 991);
        std::vector<rsl::Gate> notBelow; notBelow.push_back(swingLow(993, 3, 4)); notBelow.push_back(swingLow(1001, 6, 7)); notBelow[1].px = 992;
        CHECK(rsl::run(bars(), none, nullptr, &notBelow)[10].resume.gatePx == 992);   // 2 back (993) is not below 1 back (992): only 992 counts
        // a live queue zone: a typed held Q at the extreme opens the gate
        std::vector<rsl::Gate> empty; std::multimap<long long, rsl::Ev> q; add(q, event(10, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::HELD));
        const rsl::Step qs = rsl::run(bars(), q, nullptr, &empty)[10];
        CHECK(qs.resume.on && qs.resume.gate == "queue zone" && qs.stage == rsl::TRAP);
    }

    // ---- STEP 1 TRAP ----
    {
        std::multimap<long long, rsl::Ev> p; add(p, event(10, lst::DEF, "P", keyOf(10), 990, rse::HYBRID)); p.begin()->second.l.nums = "2.3x";
        const rsl::Step ps = rsl::run(bars(), p)[10];
        CHECK(ps.stage == rsl::TRAP && (ps.flags & rsl::F_DEFENSE_P) && ps.status == rsl::FORMING && rsl::storyPart(ps.story, 0) == "bids 2.3x");
        std::multimap<long long, rsl::Ev> q; add(q, event(10, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::HELD)); q.begin()->second.l.nums = "143 (2.4x) tk990";
        const rsl::Step qs = rsl::run(bars(), q)[10];
        CHECK(qs.stage == rsl::TRAP && (qs.flags & rsl::F_TRAP_QUEUE) && rsl::storyPart(qs.story, 0) == "Q 143 (2.4x)");   // tk hidden
        // trap alone never confirms: the attack without an execution result stays at Trap
        std::multimap<long long, rsl::Ev> pk = p; add(pk, event(11, lst::ATK, "K", keyOf(11), 991, rse::MBO, rse::NA, 300));
        CHECK(rsl::run(bars(), pk)[11].stage == rsl::TRAP);
        // fail: W / B (floor pulled before it was tested)
        std::multimap<long long, rsl::Ev> w = p; add(w, event(11, lst::DEF, "W", keyOf(10), 991));
        const rsl::Step wf = rsl::run(bars(), w)[11];
        CHECK(wf.status == rsl::FAILED && wf.failedStage == rsl::TRAP && wf.why.find("pulled before it was tested") != std::string::npos);
        std::multimap<long long, rsl::Ev> bb = p; add(bb, event(11, lst::DEF, "B", keyOf(10), 991));
        CHECK(rsl::run(bars(), bb)[11].status == rsl::FAILED);
        std::multimap<long long, rsl::Ev> lone; add(lone, event(11, lst::DEF, "W", keyOf(10), 991));   // W with no trap: context only
        CHECK(rsl::run(bars(), lone)[11].status != rsl::FAILED);
    }

    // ---- STEP 2 COLLISION ----
    {
        const std::vector<rsl::Step> same = rsl::run(bars(), collision(10));
        CHECK(same[10].stage >= rsl::COLLISION && (same[10].flags & rsl::F_COLLISION) && rsl::storyPart(same[10].story, 1) == "300 sold, held (A)");
        CHECK(rsl::run(bars(), collision(11))[11].stage >= rsl::COLLISION);     // within 1 candle
        const std::vector<rsl::Step> late = rsl::run(bars(), collision(12));
        CHECK(late[12].stage == 0 && !(late[12].flags & rsl::F_COLLISION));   // 2 candles apart: no collision
        std::multimap<long long, rsl::Ev> a; add(a, event(10, lst::DEF, "A", keyOf(10), 990, rse::MBO, rse::NA, 200));   // A alone (no attack)
        CHECK(rsl::run(bars(), a)[10].stage == 0);
        // held Q after contact = the result
        std::multimap<long long, rsl::Ev> kq; add(kq, event(10, lst::ATK, "K", keyOf(10), 990, rse::MBO, rse::NA, 300)); add(kq, event(10, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::HELD, 100, -1, -1, 99999));
        const rsl::Step c = rsl::run(bars(), kq)[10];
        CHECK(c.stage == rsl::COLLISION && (c.flags & rsl::F_DEFENSE_QUEUE) && rsl::storyPart(c.story, 1).find("held (Q)") != std::string::npos);
        // Q only at the attempt's own extreme (fix #7): a queue 10 ticks inside the bar is hover only
        std::multimap<long long, rsl::Ev> far; add(far, event(10, lst::ATK, "K", keyOf(10), 990, rse::MBO, rse::NA, 300)); add(far, event(10, lst::QUE, "Q", keyOf(10), 1000, rse::MBO, rse::HELD, 100, -1, -1, 99999));
        const rsl::Step fs = rsl::run(bars(), far)[10];
        CHECK(fs.stage == 0 && fs.here.find("own extreme") != std::string::npos);
        std::multimap<long long, rsl::Ev> near; add(near, event(10, lst::ATK, "K", keyOf(10), 990, rse::MBO, rse::NA, 300)); add(near, event(10, lst::QUE, "Q", keyOf(10), 994, rse::MBO, rse::HELD, 100, -1, -1, 99999));
        CHECK(rsl::run(bars(), near)[10].stage == rsl::COLLISION);
        CHECK(rsl::qAnchorOf("broke 4% q0 24 tk29115") == 29115 && rsl::qAnchorOf("q0 24") == 0 && rsl::qTolTicks(40) == 12 && rsl::qTolTicks(2) == 4);
        // fail: Q! (the queue at the extreme broke) - at Trap and at Collision
        std::multimap<long long, rsl::Ev> qb; add(qb, event(10, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::HELD)); add(qb, event(11, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::BROKE));
        const rsl::Step qf = rsl::run(bars(), qb)[11];
        CHECK(qf.status == rsl::FAILED && qf.failedStage == rsl::TRAP && (qf.flags & rsl::F_QUEUE_ADVERSE));
        std::multimap<long long, rsl::Ev> cb; add(cb, event(10, lst::ATK, "K", keyOf(10), 990, rse::MBO, rse::NA, 300)); add(cb, event(10, lst::DEF, "A", keyOf(10), 990, rse::MBO, rse::NA, 200)); add(cb, event(11, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::DRAINED));
        const rsl::Step cf = rsl::run(bars(), cb)[11];
        CHECK(cf.status == rsl::FAILED && cf.failedStage == rsl::COLLISION);
        std::multimap<long long, rsl::Ev> lone; add(lone, event(10, lst::QUE, "Q", keyOf(10), 990, rse::MBO, rse::DRAINED));   // no floor yet
        CHECK(rsl::run(bars(), lone)[10].status != rsl::FAILED);
    }

    // ---- STEP 3 TRAPPED ----
    {
        std::multimap<long long, rsl::Ev> nv0; add(nv0, event(10, lst::ATK, "K", keyOf(10), 990, rse::MBO, rse::NA, 300)); add(nv0, event(10, lst::DEF, "A", keyOf(10), 990, rse::MBO, rse::NA, 200));
        std::multimap<long long, rsl::Ev> e = nv0; add(e, event(11, lst::ATK, "E", keyOf(11), 991, rse::MBO, rse::NA, 100, -1, 50));
        const rsl::Step es = rsl::run(bars(), e)[11];
        CHECK(es.stage == rsl::TRAPPED && (es.flags & rsl::F_PACE_FADE));
        std::multimap<long long, rsl::Ev> e0; add(e0, event(11, lst::ATK, "E", keyOf(11), 991, rse::MBO, rse::NA, 100, -1, 50));   // E before a collision
        CHECK(rsl::run(bars(), e0)[11].stage == 0);
        // D: within 5 candles of the collision, at the gate
        std::multimap<long long, rsl::Ev> d = nv0; add(d, event(12, lst::ATK, "D", keyOf(12), 991, rse::MBO, rse::NA, 1240));
        const rsl::Step ds = rsl::run(bars(), d)[12];
        CHECK(ds.stage == rsl::TRAPPED && (ds.flags & rsl::F_CVD_DIV) && rsl::storyPart(ds.story, 2) == "D");
        std::multimap<long long, rsl::Ev> dl = nv0; add(dl, event(16, lst::ATK, "D", keyOf(16), 991, rse::MBO, rse::NA, 1240));
        const rsl::Step dls = rsl::run(bars(), dl)[16];
        CHECK(!(dls.flags & rsl::F_CVD_DIV) && dls.here.find("more than 5 candles") != std::string::npos);
        std::multimap<long long, rsl::Ev> df = nv0; add(df, event(12, lst::ATK, "D", keyOf(12), 1010, rse::MBO, rse::NA, 1240));
        CHECK(!(rsl::run(bars(), df)[12].flags & rsl::F_CVD_DIV));
        std::multimap<long long, rsl::Ev> d0; add(d0, event(12, lst::ATK, "D", keyOf(12), 991, rse::MBO, rse::NA, 1240));
        CHECK(rsl::run(bars(), d0)[12].stage == 0);
        // the attackers underwater: avg fill 990.00, close 993 = 3 ticks >= 0.25 ATR
        const std::vector<rsl::Step> u = rsl::run(bars(), collision(10));
        CHECK(u[11].under == 3 && (u[11].flags & rsl::F_UNDERWATER) && u[11].stage == rsl::TRAPPED && u[11].atkLots == 300);
        CHECK(rsl::stepText(u[11], rsl::TRAPPED) == "sellers -3 ticks, 300 lots");
        CHECK(rsl::numsShown("300 lots vw99000") == "300 lots" && rsl::tagOf("300 lots vw99000", "vw") == 99000 && rsl::tagOf("300 lots", "vw") == 0);
        CHECK(rsl::numsShown("held 120% q0 130 tk991") == "held 120% q0 130");
        const std::vector<rsl::Step> nv = rsl::run(bars(), nv0);   // no average fill: no underwater claim
        CHECK(nv[11].under == LLONG_MIN && !(nv[11].flags & rsl::F_UNDERWATER) && nv[11].stage == rsl::COLLISION);
        // fail: the attack re-accelerates to a new low beyond the floor
        std::vector<rsl::Bar> b = bars(); b[12].l = 989;
        const rsl::Step f = rsl::run(b, collision(10))[12];
        CHECK(f.status == rsl::FAILED && f.failedStage >= rsl::COLLISION && f.why.find("beyond the floor") != std::string::npos);
    }

    // ---- STEP 4 RELEASE + RESULT ----
    {
        std::multimap<long long, rsl::Ev> h = collision(10); add(h, event(11, lst::DEF, "H", keyOf(10), 991, rse::MBO, rse::NA, 100, 50));
        const rsl::Step hs = rsl::run(bars(), h)[11];
        CHECK(hs.stage == rsl::RELEASE && (hs.flags & rsl::F_TURN_MBO) && rsl::storyPart(hs.story, 3) == "H 100");
        std::multimap<long long, rsl::Ev> l = h; add(l, event(12, lst::DEF, "L", keyOf(10), 991, rse::MBO, rse::NA, 20, 25));
        const rsl::Step ls = rsl::run(bars(), l)[12];
        CHECK(ls.stage == rsl::RELEASE && (ls.flags & rsl::F_RETEST_EXECUTED_HELD));
        std::multimap<long long, rsl::Ev> x = h; add(x, event(12, lst::DEF, "X", keyOf(10), 991));
        const rsl::Step xs = rsl::run(bars(), x)[12];
        CHECK(xs.status == rsl::FAILED && xs.failedStage == rsl::RELEASE);
        std::multimap<long long, rsl::Ev> trapOnly; add(trapOnly, event(10, lst::DEF, "P", keyOf(10), 990)); add(trapOnly, event(11, lst::DEF, "H", keyOf(10), 991, rse::MBO, rse::NA, 100, 50));
        CHECK(rsl::run(bars(), trapOnly)[11].stage == rsl::TRAP);          // release needs a collision first
        std::multimap<long long, rsl::Ev> r = collision(10); add(r, event(11, lst::ATK, "R", keyOf(10), 991, rse::HYBRID));
        const rsl::Step rs = rsl::run(bars(), r)[11];
        CHECK(rs.stage == rsl::RELEASE && (rs.flags & rsl::F_TURN_HYBRID));
        std::vector<rsl::Bar> b = bars(); b[12].c = 1005; b[12].h = 1007;
        const rsl::Step res = rsl::run(b, h)[12];
        CHECK(res.status == rsl::COMPLETE && res.stage == rsl::RELEASE && res.why.find("Result") != std::string::npos && !res.resume.on);
        const rsl::Step noRel = rsl::run(b, collision(10))[12];               // price left without a release = not a result
        CHECK(noRel.status != rsl::COMPLETE);
    }

    // ---- typed-evidence rules kept ----
    {
        std::multimap<long long, rsl::Ev> e; rsl::Ev legacy = event(10, lst::ATK, "K", keyOf(10), 990); legacy.m.typed = false; add(e, legacy);
        rsl::Ev partial = event(10, lst::DEF, "A", keyOf(10), 990); partial.m.quality = rse::PARTIAL; add(e, partial);
        const rsl::Step s = rsl::run(bars(), e)[10];
        CHECK(s.stage == 0 && (s.flags & rsl::F_LEGACY_CONTEXT) && (s.flags & rsl::F_PARTIAL_CONTEXT) && s.readiness == rsl::READ_PARTIAL);
        for (int q : { rse::UNVERIFIED, rse::PARTIAL, rse::PENDING, rse::UNAVAILABLE }) {
            std::vector<rsl::Bar> b = bars(); b[10].quality = q;
            const rsl::Step r = rsl::run(b, collision(10))[10]; CHECK(r.stage == 0 && r.readiness != rsl::READ_CONFIRMED && !r.resume.on);
        }
        std::vector<rsl::Bar> b = bars(); for (auto& x : b) { x.h = LLONG_MAX; x.l = LLONG_MIN; x.c = 0; }
        CHECK(std::isfinite(rsl::atrAt(b, 10)));
    }

    // ---- never repaint: RS3 round trip; a restart after every candle = one straight pass ----
    {
        std::multimap<long long, rsl::Ev> e = collision(10); add(e, event(12, lst::ATK, "D", keyOf(12), 991, rse::MBO, rse::NA, 1240)); add(e, event(13, lst::DEF, "H", keyOf(10), 991, rse::MBO, rse::NA, 100, 50));
        std::vector<rsl::Gate> g; g.push_back(swingLow(992, 3, 5));
        std::vector<rsl::Step> full = rsl::run(bars(), e, nullptr, &g);
        for (size_t i = 0; i < full.size(); ++i) { full[i].decided = 5000 + (long long)i; rsl::Step p; const std::string ln = rsl::line(full[i]); CHECK(rsl::parse(ln, p) && p == full[i]); }
        for (int cut = 0; cut < (int)full.size(); ++cut) {
            std::map<long long, rsl::Step> f; for (int i = 0; i <= cut; ++i) { rsl::Step p; rsl::parse(rsl::line(full[(size_t)i]), p); f[p.key] = p; }
            const std::vector<rsl::Step> again = rsl::run(bars(), e, &f, &g); bool same = again.size() == full.size();
            for (size_t i = (size_t)cut + 1; i < full.size() && same; ++i) { rsl::Step x = again[i]; x.decided = full[i].decided; same = x == full[i]; }
            CHECK(same);
        }
        std::map<long long, rsl::Step> f10 = freezeTo(full, 10); std::multimap<long long, rsl::Ev> late = e; add(late, event(10, lst::DEF, "P", keyOf(10), 990));
        CHECK(rsl::run(bars(), late, &f10, &g)[10] == full[10]);           // a late letter at a frozen candle cannot change it
        rsl::Step p; CHECK(!rsl::parse(rsl::line(full[12]) + "|extra", p) && !rsl::parse("RS|2|1|0|0|0|0|0|0|0|0|0|0|0|-1|0|0|0|0|0|0||||", p));
        CHECK(full[10].stage >= rsl::COLLISION && full[12].stage == rsl::TRAPPED && full[13].stage == rsl::RELEASE);
    }

    // ---- the Now box / hover text ----
    {
        std::multimap<long long, rsl::Ev> e = collision(10); rsl::Ev p = event(10, lst::DEF, "P", keyOf(10), 990); p.l.nums = "2.3x"; p.received = 1; add(e, p);
        std::vector<rsl::Gate> g; g.push_back(swingLow(992, 3, 5));
        const rsl::Step s = rsl::run(bars(), e, nullptr, &g)[11];
        const std::vector<std::string> r = rsl::nowRows(s, "8:51 AM", 0.25);
        CHECK(r.size() == 8 && r[1] == "LOW at 247.50 (swing low 8:48 AM)" && r[2] == "Trap: bids 2.3x" && r[3] == "Collision: 300 sold, held (A)");
        CHECK(r[4] == "Trapped: sellers -3 ticks, 300 lots" && r[5] == "Release: waiting for H / R" && r[6].find("Evidence complete") == 0 && r[7].find("Odds ?") == 0);
        CHECK(rsl::priceText(30784, 0.25) == "7,696.00" && rsl::priceText(7696, 1.0) == "7,696" && rsl::thousands(1840) == "1,840" && rsl::thousands(-1240) == "-1,240");
        const std::vector<std::string> h = rsl::hoverRows(s, "8:51 AM", 0.25); bool gate = false, d = false;
        for (const std::string& x : h) { gate = gate || x.find("Gate: swing low 8:48 AM at 248.00") != std::string::npos; d = d || x.find("3 Trapped: sellers -3 ticks") != std::string::npos; }
        CHECK(gate && d);
        CHECK(rsl::nowCols(8, 400, 13) == 1 && rsl::nowCols(8, 100, 13) == 2 && rsl::nowCols(8, 60, 13) == 3);
        rsl::Step none; const std::vector<std::string> n = rsl::nowRows(none, "-");
        CHECK(n[1] == "No attempt at a level");
    }

    // ---- LetterStrips 1.3.0: names, D, the CVD divergence ----
    {
        CHECK(std::string(lst::VERSION) == "1.3.0");
        CHECK(std::string(lst::stripName(lst::ATK, 0)) == "Attack" && std::string(lst::stripName(lst::ATK, 1)) == "Trapped" && std::string(lst::stripName(lst::DEF, 0)) == "Floor" && std::string(lst::stripName(lst::DEF, 1)) == "Release");
        CHECK(lst::stripOf(lst::ATK, "D") == 1 && lst::validCode(lst::ATK, "D") && !lst::validCode(lst::DEF, "D") && lst::rankOf(lst::ATK, "Tr") < lst::rankOf(lst::ATK, "D") && lst::rankOf(lst::ATK, "D") < lst::rankOf(lst::ATK, "E"));
        const long long dl[6] = { -100, -50, -80, 20, -30, -200 }, lo[6] = { 1000, 995, 990, 992, 994, 993 }, hi[6] = { 1010, 1005, 1000, 1002, 1004, 1003 };
        const bool ok[6] = { true, true, true, true, true, true };
        CHECK(lst::cvdDiv(dl, lo, hi, ok, 6) == lst::LOW);                     // CVD new low, candle low 993 > 990
        const long long lo2[6] = { 1000, 995, 990, 992, 994, 989 };
        CHECK(lst::cvdDiv(dl, lo2, hi, ok, 6) == -1);                          // a lower low = no divergence
        const bool ok2[6] = { true, true, false, true, true, true };
        CHECK(lst::cvdDiv(dl, lo, hi, ok2, 6) == -1);                          // a missing candle = no call
        const long long du[6] = { 100, 50, 80, -20, 30, 200 }, hu[6] = { 1010, 1015, 1020, 1018, 1016, 1017 };
        CHECK(lst::cvdDiv(du, lo, hu, ok, 6) == lst::HIGH);
        lst::Rec rr; CHECK(lst::parse("LT|1|A|1234|D|0|-1|delta -200 new low, candle higher low", rr) && rr.l.code == "D");
    }

    std::printf("%s: %d checks, %d failed\n", fails ? "FAILED" : "PASSED", checks, fails);
    return fails ? 1 : 0;
}
