// test_lqp452.cpp -- lsLiquidityProfile 0.45.2 (Rassul 20:04 2026-10-18 "MKT column values are flashing ... they come on and then go away";
// 20:06 "yes": LMT F coloured by the RESTING side). SDK-free. From plugin\:
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. tests_lqp044/test_lqp452.cpp -o t452 && ./t452
// 1. the 0.45.1 flicker reproduced: the pool cut on 1 calc in 5 s, the session's own 1SD on the others -> the MKT set differs calc to calc
// 2. mktTiersPick: the pool once known is used on EVERY calc (never replaced by the live stats in between) -> identical MKT items
// 3. marksHeld: printed once = kept with its tier even when the cut rises; a filling order only goes up; gone after the 60-min window
// 4. LMT F colour: resting bid green, resting offer red (both sides)
#include "LiquidityProfileLogic.h"
#include "BigFlow.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)

static std::string sig(const std::vector<bfl::Mark>& v) {
    std::string s; for (size_t i = 0; i < v.size(); ++i) s += std::to_string(v[i].side) + ":" + std::to_string(v[i].px) + ":" + std::to_string(v[i].lots) + ":" + std::to_string(v[i].tier) + " ";
    return s;
}
// one session: 4000 one-lot orders + 60 big ones 20..79 lots (bids even, offers odd), all completed
static void fill(bfl::Mkt& m) {
    long long id = 1, t = 1000;
    for (int i = 0; i < 4000; ++i) m.onFill(id++, i % 2 ? bfl::ASK : bfl::BID, 4000 + i % 7, 1, t);
    for (int s = 20; s < 80; ++s) m.onFill(id++, s % 2 ? bfl::ASK : bfl::BID, 4000 + s, s, t);
    m.step(t + 5000);
}

static void test_old_flicker_and_fix() {
    bfl::Mkt m; fill(m);
    double lmu = 0, lv[3] = { 0, 0, 0 }; const bool lok = m.stats(lmu, lv);
    CHECK(lok, "fixture: the session's own stats are known");
    lqp::MktPool pool; pool.ok = true; pool.mu = 60; pool.v[0] = 100; pool.v[1] = 130; pool.v[2] = 170; pool.q266 = 160;   // 5 finished sessions: bigger orders
    CHECK(lv[0] < pool.v[0], "fixture: the session's 1SD is under the pool's S20 (the 0.45.1 alternation changes the cut)");
    // 0.45.1: calc 1 (pool due) used the pool, calcs 2-5 the live stats -> the MKT set changed between redraws
    std::vector<std::string> old;
    for (int c = 0; c < 10; ++c) {
        const bool due = c % 5 == 0; const double cut0 = due ? pool.v[0] : lv[0];
        old.push_back(sig(m.marks(10000 + c * 1000, (long long)std::llround(cut0), 5)));
    }
    bool differs = false; for (size_t i = 1; i < old.size(); ++i) if (old[i] != old[0]) differs = true;
    CHECK(differs, "0.45.1 reproduced: the MKT items differ between calcs (values blink on / off)");
    // 0.45.2: one source per calc
    std::vector<std::string> now;
    for (int c = 0; c < 10; ++c) {
        double mu = 0, v[3] = { 0, 0, 0 }, q = 0;
        const bool ok = lqp::mktTiersPick(pool, lok, lmu, lv, mu, v, q);   // the pool is kept between its recomputes: same on every calc
        CHECK(ok && v[0] == 100 && q == 160, "the pool's tiers on every calc");
        const long long cut[3] = { (long long)std::llround(v[0]), (long long)std::llround(v[1]), (long long)std::llround(q) };
        now.push_back(sig(m.marksHeld(10000 + c * 1000, cut, 5)));
    }
    bool same = true; for (size_t i = 1; i < now.size(); ++i) if (now[i] != now[0]) same = false;
    CHECK(same, "0.45.2: alternating calcs give identical MKT items");
    // no pool yet: the live stats on every calc (q266 0 -> the host derives 2.66 SD)
    double mu = 0, v[3] = { 0, 0, 0 }, q = 99; lqp::MktPool none;
    CHECK(lqp::mktTiersPick(none, lok, lmu, lv, mu, v, q) && v[0] == lv[0] && q == 0, "no pool: the session's stats");
    CHECK(!lqp::mktTiersPick(none, false, 0, lv, mu, v, q), "nothing known: learning");
}

static void test_held() {
    bfl::Mkt m; fill(m);
    const long long lo[3] = { 40, 60, 75 }, hi[3] = { 100, 130, 170 };
    const std::vector<bfl::Mark> a = m.marksHeld(10000, lo, 5);
    CHECK(a.size() == 10, "10 marks (5 per side) at the low cut");
    // the cut rises (a new pool): the printed marks stay, at the tier they were printed with
    const std::vector<bfl::Mark> b = m.marksHeld(11000, hi, 5);
    CHECK(sig(a) == sig(b), "the cut rose: the printed marks stay unchanged (no blink off)");
    bool tier3 = false; for (size_t i = 0; i < b.size(); ++i) if (b[i].lots >= 75 && b[i].tier == 3) tier3 = true;
    CHECK(tier3, "the tier printed is kept (3 for 75+ lots although the new 3SD cut is 170)");
    // an order never printed and under the new cut does not appear
    bfl::Mkt m2; fill(m2);
    CHECK(m2.marksHeld(10000, hi, 5).empty(), "never printed + under the cut: not drawn");
    // a still-filling order goes up a tier, never down
    bfl::Mkt m3; fill(m3);
    m3.onFill(999999, bfl::BID, 5000, 45, 20000);
    const long long c3[3] = { 40, 80, 120 };
    std::vector<bfl::Mark> f = m3.marksHeld(20000, c3, 20); int t1 = 0; for (auto& x : f) if (x.px == 5000) t1 = x.tier;
    m3.onFill(999999, bfl::BID, 5000, 50, 20500);
    f = m3.marksHeld(20500, c3, 20); int t2 = 0; for (auto& x : f) if (x.px == 5000) t2 = x.tier;
    CHECK(t1 == 1 && t2 == 2, "the forming order goes up 1 -> 2 as it fills");
    const long long c4[3] = { 200, 300, 400 };
    f = m3.marksHeld(21000, c4, 20); int t3 = 0; for (auto& x : f) if (x.px == 5000) t3 = x.tier;
    CHECK(t3 == 2, "and never drops back when the cut rises");
    // gone only after the 60-min window
    m3.step(20500 + bfl::KEEP_MS + 5000);
    CHECK(m3.marksHeld(20500 + bfl::KEEP_MS + 5000, c3, 20).empty() && m3.held.size() <= 256 + 0, "after 60 min: gone (held ids forgotten)");
    m3.clear(); CHECK(m3.held.empty(), "clear() forgets the held ids");
}

static void test_lmt_f_colour() {
    // ES 20:06: MKT "+ 2x 109 S" (buyers swept) and LMT F "100" at that price = a resting OFFER filled -> RED (sellers absorbed)
    CHECK(!lqp::lmtFillGreen(false), "a filled resting offer (sellers' limit, lifted by buyers) is RED");
    CHECK(lqp::lmtFillGreen(true), "a filled resting bid (buyers' limit, hit by sellers) is GREEN");
}

int main() {
    test_old_flicker_and_fix(); test_held(); test_lmt_f_colour();
    CHECK(std::string(lqp::VERSION) >= "0.45.3" && std::string(bfl::VERSION) >= "1.4.0", "lqp 0.45.3 / bfl 1.4.0 or later");   // (2026-10-22) the version moves on with each approved build (0.47.1 now): at least this one
    std::printf("lqp 0.45.2: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
