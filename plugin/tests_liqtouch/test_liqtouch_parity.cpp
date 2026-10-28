// test_liqtouch_parity.cpp -- LiqTouch.h 1.0.0 against the 10-28 liquidity study (touch.py / eng2.py) on REAL ES MBO (22 Oct 2026):
// six touches (STAYS+, GONE, STAYS weak, STAYS, GONE+, an offer STAYS) replayed event by event in IRT semantics from the fixtures
// written by gen_fixture.py; every number the study used must match: size at the touch, max size, added / eaten / pulled / left at
// +15 s, seconds at the level, aggression both sides, the tag, and the outcome (study 'hold' 1 = HELD).
// Build (from plugin/): g++ -std=c++17 -O1 -I. tests_liqtouch/test_liqtouch_parity.cpp -o t && ./t tests_liqtouch
#include "LiqTouch.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
static int FAILS = 0, PASSES = 0;
static void check(bool c, const std::string& m) { if (c) ++PASSES; else { ++FAILS; std::printf("FAIL %s\n", m.c_str()); } }
static std::vector<std::string> csv(const std::string& l) { std::vector<std::string> o; std::string c; bool q = false; for (char ch : l) { if (ch == '"') q = !q; else if (ch == ',' && !q) { o.push_back(c); c.clear(); } else c += ch; } o.push_back(c); return o; }
static const char* expTag(const std::string& state, int dwell, double churn, double refill, double pullX) {
    if (state == "STAYS") return (dwell <= 4 && churn < 15) ? "STAYS+" : dwell >= 14 ? "STAYS_WEAK" : "STAYS";
    if (state == "GONE") return (refill < 0.25 || pullX > 1) ? "GONE+" : "GONE";
    return "MID";
}
int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "tests_liqtouch";
    std::ifstream in((dir + "/cases.csv").c_str()); std::string line; std::getline(in, line);
    const std::vector<std::string> H = csv(line); auto col = [&](const std::string& n) { for (size_t i = 0; i < H.size(); ++i) if (H[i] == n) return (int)i; return -1; };
    int n = 0;
    while (std::getline(in, line)) {
        const std::vector<std::string> f = csv(line); const std::string name = f[col("name")];
        ltc::Engine E(ltc::defaults("ES"));
        { std::stringstream ss(f[col("atr")]); std::string cp; while (std::getline(ss, cp, ';')) { const size_t k = cp.find(':'); E.atrOverride.push_back(std::make_pair(std::atoll(cp.substr(0, k).c_str()), std::atof(cp.substr(k + 1).c_str()))); } }
        const long long W0 = std::atoll(f[col("W0")].c_str()), px = std::atoll(f[col("px")].c_str()), tMs = std::atoll(f[col("tMs")].c_str());
        const int side = std::atoi(f[col("side")].c_str());
        E.begin(W0, std::atoll(f[col("last")].c_str()));
        std::ifstream b((dir + "/" + name + ".bin").c_str(), std::ios::binary); char rec[34]; long long lastMs = 0;
        std::vector<ltc::Engine::Rec> R;
        while (b.read(rec, 34)) { long long ms, p, s, id; signed char a, sd; std::memcpy(&ms, rec, 8); a = rec[8]; sd = rec[9]; std::memcpy(&p, rec + 10, 8); std::memcpy(&s, rec + 18, 8); std::memcpy(&id, rec + 26, 8);
            E.onEvent(a, sd, p, s, id, ms); lastMs = ms; std::vector<ltc::Engine::Rec> r = E.drain(); R.insert(R.end(), r.begin(), r.end()); }
        E.advance(lastMs + 2000); { std::vector<ltc::Engine::Rec> r = E.drain(); R.insert(R.end(), r.begin(), r.end()); }
        const ltc::Engine::Rec* T15 = nullptr; const ltc::Engine::Rec* OUT = nullptr;
        for (const auto& r : R) if (r.t.px == px && r.t.side == side && r.t.secEnd == tMs) { if (r.kind == 'T') T15 = &r; if (r.kind == 'O') OUT = &r; }
        check(T15 != nullptr, name + ": touch decided at the study's second");
        if (!T15) { for (const auto& r : R) if (r.t.px == px && r.t.side == side) std::printf("  %s rec %c secEnd %lld (want %lld) tag %s\n", name.c_str(), r.kind, r.t.secEnd, tMs, ltc::tagName(r.t.tag)); continue; }
        const ltc::Touch& t = T15->t; ++n;
        auto eqI = [&](const char* what, long long got, long long want) { check(got == want, name + ": " + what + " " + std::to_string(got) + " vs study " + std::to_string(want)); };
        eqI("szT", t.szT, std::atoll(f[col("szT")].c_str())); eqI("szmax", t.szMax, std::atoll(f[col("szmax")].c_str()));
        // added / eaten: the study's Databento engine NETS an iceberg refill against the fill just before it (F 8 then M 8 -> 50 = +42 added,
        // 0 eaten); IRT applies the fill first (8 eaten, +50 added). The net (added - eaten) must be identical; each side may differ only by that refill.
        eqI("a15 - e15 (net)", t.a15 - t.e15, std::atoll(f[col("a")].c_str()) - std::atoll(f[col("e")].c_str()));
        check(t.a15 >= std::atoll(f[col("a")].c_str()) && t.a15 - std::atoll(f[col("a")].c_str()) <= 40, name + ": a15 within the iceberg-refill netting");
        eqI("p15", t.p15, std::atoll(f[col("p")].c_str())); eqI("s15", t.s15, std::atoll(f[col("s")].c_str()));
        eqI("dwell", t.dwell15, std::atoll(f[col("dwell")].c_str()));
        // aggression: the study summed Databento TRADE rows by aggressor (side 'N' / odd rows skipped); IRT sums FILLs by the resting side - within 1.5% or 6 lots
        auto near = [&](const char* what, long long got, double want) { check(std::fabs((double)got - want) <= std::max(6.0, 0.015 * want), name + ": " + what + " " + std::to_string(got) + " vs study " + std::to_string((long long)want)); };
        near("g", t.g15, std::atof(f[col("g")].c_str())); near("opp", t.opp15, std::atof(f[col("opp")].c_str()));
        check(t.t0 == tMs - 1000LL * std::atoll(f[col("age")].c_str()), name + ": the level was followed from the study's second (episode start)");
        check(std::fabs(t.A - std::atof(f[col("A")].c_str())) < 1e-9, name + ": ATR");
        const double pullX = std::atof(f[col("pull60")].c_str());
        check(std::fabs((double)t.pull60 / std::max(1LL, t.szMax) - pullX) < 1e-6, name + ": pull60 ratio " + std::to_string((double)t.pull60 / std::max(1LL, t.szMax)) + " vs " + std::to_string(pullX));
        const std::string want = expTag(f[col("state")], std::atoi(f[col("dwell")].c_str()), (double)(t.a15 + t.p15) / std::max(1LL, t.szT), (double)t.a15 / std::max(1LL, t.e15), pullX);
        check(want == ltc::tagName(t.tag), name + ": tag " + ltc::tagName(t.tag) + " vs study " + want);
        const int hold = std::atoi(f[col("hold")].c_str());
        check(OUT != nullptr && ((hold == 1) == (OUT->t.outcome == ltc::O_HELD)), name + ": outcome " + (OUT ? ltc::outName(OUT->t.outcome) : "none") + " vs study hold " + std::to_string(hold));
        std::printf("   t0 %lld age %lld pull60 %lld szmax %lld sz60 %lld\n", t.t0, (t.secEnd - t.t0) / 1000, t.pull60, t.szMax, t.sz60);
        std::printf("   outMs %lld\n", OUT ? OUT->t.outMs : 0LL);
        std::printf("%-12s tag %-10s vote %+d  szT %lld a %lld e %lld p %lld s %lld dwell %d nd %.2f  outcome %s\n", name.c_str(), ltc::tagName(t.tag), t.vote, t.szT, t.a15, t.e15, t.p15, t.s15, t.dwell15, t.nd, OUT ? ltc::outName(OUT->t.outcome) : "-");
    }
    check(n == 6, "six study cases replayed");
    std::printf("RESULT %d passed, %d failed\n", PASSES, FAILS);
    return FAILS ? 1 : 0;
}
