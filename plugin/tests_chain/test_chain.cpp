// test_chain.cpp -- ChainSpec.h 1.0.0 + SignalBoard.h 1.4.0: the static dependency graph has no cycle, no duplicate producer, no read from a
// higher layer, every read has a producer; the checker CATCHES each of those faults when one is injected; the Fact form round-trips;
// stale / missing producers give an empty read (never a hang); decidedOnly drops live facts; the board never grows past its cap.
// From plugin\:  g++ -std=c++17 -O1 -I. tests_chain/test_chain.cpp -o tch && ./tch
#include "ChainSpec.h"
#include "SignalBoard.h"
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
int main() {
    chain::Report r = chain::checkDefault();
    for (auto& p : r.problems) std::printf("  problem: %s\n", p.c_str());
    CHECK(r.ok, "[graph] the shipped graph is clean");
    { auto N = chain::nodes(); auto P = chain::producers(); auto E = chain::edges();
      E.push_back({"lsDeltaProfile.Combiner", "lsDeltaProfile.READ", 'K', chain::DECIDE});
      CHECK(!chain::check(N, P, E).ok, "[graph] a read from a higher layer is caught"); }
    { auto N = chain::nodes(); auto P = chain::producers(); auto E = chain::edges();
      P.push_back({'P', "lsDealerProfile", 15, "dup"});
      CHECK(!chain::check(N, P, E).ok, "[graph] a duplicate producer is caught"); }
    { auto N = chain::nodes(); auto P = chain::producers(); auto E = chain::edges();
      E.push_back({"lsDeltaProfile.READ", "lsLiquidityProfile.Ex", 'R', chain::DECIDE});   // Ex <-> READ both ways = a cycle
      chain::Report q = chain::check(N, P, E); bool cyc = false; for (auto& p : q.problems) if (p.find("cycle") != std::string::npos) cyc = true;
      CHECK(!q.ok && cyc, "[graph] a same-layer cycle is caught"); }
    { auto N = chain::nodes(); auto P = chain::producers(); auto E = chain::edges();
      E.push_back({"lsBookFlow", "lsDeltaProfile.Combiner", 'Z', chain::DECIDE});
      CHECK(!chain::check(N, P, E).ok, "[graph] a read with no producer is caught"); }
    // every consumer's inputs have a producer (input coverage)
    { std::set<char> owned; for (auto& p : chain::producers()) owned.insert(p.kind); bool all = true;
      for (auto& e : chain::edges()) if (!owned.count(e.kind)) all = false; CHECK(all, "[coverage] every input has a producer"); }
    // Fact round trip, freshness, decidedOnly, cap
    std::vector<sb::Fact> v;
    for (int i = 0; i < 250; ++i) { sb::Fact f; f.bar = 1000 + i; f.lo = 7700.25; f.hi = 7701.0; f.side = i % 2 ? 1 : -1; f.val = 2.5; f.code = 1; f.state = i % 3 ? 1 : 0; f.proven = 2; v.push_back(f); }
    sb::postFacts('N', "ESZ6", 180, "c1", 5000, v);
    std::vector<sb::Fact> o; CHECK(sb::readFacts('N', "ESZ6", 180, "c1", 5100, o, false) && o.size() == sb::FACT_CAP, "[board] capped at 200");
    CHECK(o.back().bar == 1249 && o.back().lo == 7700.25 && o.back().hi == 7701.0 && o.back().proven == 2, "[board] newest kept, fields round-trip");
    std::vector<sb::Fact> d; sb::readFacts('N', "ESZ6", 180, "c1", 5100, d, true); bool allDec = !d.empty(); for (auto& f : d) if (f.state != 1) allDec = false;
    CHECK(allDec && d.size() < o.size(), "[board] decidedOnly drops live facts");
    CHECK(!sb::readFacts('N', "ESZ6", 180, "c1", 5000 + sb::FRESH_MS + 1, o, false) && o.empty(), "[board] a stale producer reads as missing (never a hang)");
    CHECK(!sb::readFacts('Q', "ESZ6", 180, "c1", 5100, o, false) && o.empty(), "[board] a missing producer reads as missing");
    CHECK(!sb::readFacts('N', "ESZ6", 180, "other", 5100, o, false), "[board] another chart's facts are never read");
    // repeated posting does not grow the board (bounded)
    std::string a1; bb::bbGet(sb::factKey('N', "ESZ6", 180, "c1"), a1);
    for (int k = 0; k < 50; ++k) sb::postFacts('N', "ESZ6", 180, "c1", 6000 + k, v);
    std::string a2; bb::bbGet(sb::factKey('N', "ESZ6", 180, "c1"), a2);
    CHECK(a2.size() <= a1.size() + 4, "[board] size bounded across 50 re-posts");
    std::printf("ChainSpec %s / SignalBoard %s: %d passed, %d failed\n", chain::VERSION, sb::VERSION, passes, fails);
    return fails ? 1 : 0;
}
