// test_bf131.cpp -- lsBookFlow 1.31.0 (2026-10-28 overnight): the decided candle's defense as SignalBoard 1.4.0 kind 'D' (bf::defenseFact) -
// side / code from the spike, nets, rebuilt %, unknown ("?") inputs = no fact, post + read back through the board (decided, "?"), cap 200.
// Build (from tests_bf131/): g++ -std=c++17 -O1 -w -I.. test_bf131.cpp -o /tmp/t_bf131 && /tmp/t_bf131
#include <map>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include "../SignalBoard.h"
#include "../OrderFlowPaneSettings.h"
#include "../BookFlowLogic.h"
static int P = 0, F = 0; static void check(bool c, const char* m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m); } }
static std::map<std::string, std::string> kv(const char* bn, const char* an, const char* sp, const char* rb) { return { { "bid_net", bn }, { "ask_net", an }, { "spike", sp }, { "rebuilt_pct", rb } }; }
int main() {
    bf::DFact d = bf::defenseFact(1000, kv("420", "-35", "bid_arriving", "62"));
    check(d.ok && d.side == 1 && d.code == 1 && d.lo == 420 && d.hi == -35 && d.val == 62, "bids arriving = bullish, code 1, nets + rebuilt %");
    d = bf::defenseFact(1000, kv("-300", "10", "bid_leaving", "?")); check(d.ok && d.side == -1 && d.code == 2 && d.val == -1, "bids leaving = bearish, rebuilt unknown = -1");
    d = bf::defenseFact(1000, kv("5", "500", "ask_arriving", "0")); check(d.ok && d.side == -1 && d.code == 3, "offers arriving = bearish");
    d = bf::defenseFact(1000, kv("5", "-500", "ask_leaving", "0")); check(d.ok && d.side == 1 && d.code == 4, "offers leaving = bullish");
    d = bf::defenseFact(1000, kv("5", "5", "none", "0")); check(d.ok && d.side == 0 && d.code == 0, "no spike = side 0");
    check(!bf::defenseFact(1000, kv("?", "?", "?", "?")).ok, "an unread candle = no fact (\"?\" upstream)");
    check(!bf::defenseFact(0, kv("1", "1", "none", "1")).ok, "no candle time = no fact");
    check(!bf::defenseFact(1000, kv("1x", "1", "none", "1")).ok, "garbled net = no fact");
    check(std::string(bf::VERSION) == "1.31.0" && std::string(ofset::BF_VERSION) == "1.31.0", "version 1.31.0 (logic + settings)");
    std::vector<sb::Fact> v; for (int i = 0; i < 250; ++i) { sb::Fact f; f.bar = 1000 + i; f.side = 1; f.code = 1; f.state = 1; f.proven = 2; v.push_back(f); }
    sb::postFacts('D', "ESZ6", 180, "ES 3m", 5000, v); std::vector<sb::Fact> r;
    check(sb::readFacts('D', "ESZ6", 180, "ES 3m", 6000, r, true) && r.size() == sb::FACT_CAP && r.back().bar == 1249 && r[0].proven == 2, "board: decided D facts read back, capped at 200 (newest kept)");
    std::printf("bf131: %d passed, %d failed\n", P, F); return F ? 1 : 0;
}
