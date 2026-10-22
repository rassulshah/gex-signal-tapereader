// test_dealerprofile_2170.cpp -- lsDealerProfile 2.17.0 / DealerColumnsLogic 1.2.0 (2026-10-18, audit C findings 7 / 10 + the Market State).
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. test_dealerprofile_2170.cpp -o t2170 && ./t2170   (from plugin\)
//  [1] the GEX facts: keys exactly as MARKET-STATE-SPEC 1.0 (no DEX key), HVL / CR / PS from the drawn MenthorQ levels (combined labels
//      "CR 0D / GW 0D", "PS 0D / HVL 0D"), regime price vs HVL, a stale grid never posts a regime / hedge lots, the nearest level + ticks + lots
//  [2] the stale rule the Profile now uses = Fresh.h (one rule with the HOD / LOD box): the cases that changed (outside MenthorQ hours)
//  [3] the per-chart status file name; the board round trip; the version literals (DP_VERSION == setVersion == 2.17.0)
#include "DealerColumnsLogic.h"
#include "Fresh.h"
#include "MarketState.h"
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
static int n = 0, f = 0;
#define CHECK(c, m) do { ++n; if (!(c)) { ++f; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, std::string(m).c_str()); } } while (0)
static long long at(int y, int mo, int d, int h, int mi) { return fr::ctAbs(y, mo, d, h * 3600 + mi * 60); }
int main()
{
    // ES 10-16 as MenthorQLevels.csv draws it
    std::vector<dcol::MqLvl> lv = { { "CR 0D / GW 0D", 7795 }, { "PS 0D", 7700 }, { "HVL 0D", 7780 }, { "G1", 7750 }, { "G2", 7725 } };
    std::vector<dcol::HedgeNode> nodes = { { 7700, true, -560 }, { 7705, true, 381 }, { 7750, true, -1200 }, { 7780, false, 0 } };
    {   const std::vector<std::string> t = dcol::mstTokens("CR 0D / GW 0D");
        CHECK(t.size() == 2 && t[0] == "CR0" && t[1] == "GW0", "tokens CR0 / GW0");
        CHECK(dcol::mstTokens("MQ HVL 0D")[0] == "HVL0" && dcol::mstTokens("G1")[0] == "G1" && dcol::mstTokens("PS 0D STALE")[0] == "PS0", "tokens: MQ prefix, G1, STALE suffix");
        std::map<std::string, std::string> kv = dcol::mstGex(lv, nodes, 7703.0, 0.25, 2, 7.4, false);
        const std::set<std::string> want = { "regime", "hvl", "cr", "ps", "near_level", "near_dist_ticks", "near_lots", "data_age_min", "stale" };
        std::set<std::string> got; for (const auto& p : kv) got.insert(p.first);
        CHECK(got == want, "GEX keys = the spec's 9 (no DEX key: DEX is context inside the gamma read)");
        CHECK(kv["regime"] == "neg" && kv["hvl"] == "7780.00" && kv["cr"] == "7795.00" && kv["ps"] == "7700.00", "regime neg below HVL 7780 (the HVL the Profile draws)");
        CHECK(kv["near_level"] == "PS0@7700.00" && kv["near_dist_ticks"] == "12" && kv["near_lots"] == "-560", "nearest: PS0 7700, 12 ticks, dealers sell 560 there: " + kv["near_level"] + " " + kv["near_dist_ticks"] + " " + kv["near_lots"]);
        CHECK(kv["data_age_min"] == "7" && kv["stale"] == "0", "age 7 min, live");
        std::map<std::string, std::string> up = dcol::mstGex(lv, nodes, 7790.0, 0.25, 2, 3, false);
        CHECK(up["regime"] == "pos" && up["near_level"] == "CR0/GW0@7795.00" && up["near_lots"] == "?", "above HVL: pos; CR0/GW0 nearest; no hedge node at 7795 = '?'");
        std::map<std::string, std::string> st = dcol::mstGex(lv, nodes, 7703.0, 0.25, 2, 300, true);
        CHECK(st["regime"] == "?" && st["near_lots"] == "?" && st["stale"] == "1" && st["data_age_min"] == "300" && st["hvl"] == "7780.00", "stale grid: no regime, no hedge lots; levels + age + stale=1");
        std::vector<dcol::MqLvl> eu = { { "CR 0D / GW 0D", 1.1225 }, { "PS 0D / HVL 0D", 1.12 }, { "G1", 1.125 } };
        std::map<std::string, std::string> e = dcol::mstGex(eu, {}, 1.1210, 0.00005, 5, 2, false);
        CHECK(e["hvl"] == "1.12000" && e["ps"] == "1.12000" && e["regime"] == "pos" && e["near_level"] == "PS0/HVL0@1.12000" && e["near_dist_ticks"] == "20", "6E: 'PS 0D / HVL 0D' gives both PS and HVL: " + e["near_level"] + " " + e["near_dist_ticks"]);
        std::map<std::string, std::string> none = dcol::mstGex({}, {}, 7703.0, 0.25, 2, -1, true);
        CHECK(none["regime"] == "?" && none["hvl"] == "?" && none["near_level"] == "?" && none["data_age_min"] == "?" && none["stale"] == "1", "no levels / unknown age: '?' and stale");
        std::map<std::string, std::string> nan = dcol::mstGex(lv, nodes, NAN, 0.25, 2, 5, false);
        CHECK(nan["regime"] == "?" && nan["near_level"] == "?", "no close: no regime, no nearest");
    }
    {   // [2] the stale rule now = Fresh.h (before: dpaudit::bookStale = only in MenthorQ hours past 20 min; the old-file badge 25 min)
        const long long mon10 = at(2026, 10, 19, 10, 0), sat = at(2026, 10, 17, 23, 50);
        CHECK(fr::judge(fr::K_DEALER, mon10 - 21 * 60, mon10).stale() == dpaudit::bookStale(21, true), "in hours 21 min: both STALE");
        CHECK(fr::judge(fr::K_DEALER, mon10 - 15 * 60, mon10).stale() == dpaudit::bookStale(15, true), "in hours 15 min: both live");
        CHECK(!fr::judge(fr::K_DEALER, at(2026, 10, 16, 15, 55), sat).stale() && !dpaudit::bookStale(1915, false), "weekend, Friday's closing grid: live (PAUSED) in both");
        CHECK(fr::judge(fr::K_DEALER, at(2026, 10, 15, 15, 55), sat).stale() && !dpaudit::bookStale(3355, false), "CHANGED: weekend with THURSDAY's grid = STALE now (was never stale outside hours)");
        CHECK(fr::judge(fr::K_DEALER, mon10 - 22 * 60, mon10).stale(), "CHANGED: the old-file badge at 22 min in hours (was 25)");
        const dpaudit::Hover h = dcol::headerHoverC("2.17.0", dpaudit::BookAsOf(), -1, true);
        bool rule = false; for (const auto& l : h.lines) if (l.find("over 20 min old while MenthorQ updates") != std::string::npos) rule = true;
        CHECK(rule, "the header hover states the shared rule");
    }
    {   // [3]
        CHECK(dcol::statusName("ES", 180) == "DealerProfile.status-ES-180.txt" && dcol::statusName("EU", 60) == "DealerProfile.status-EU-60.txt", "per-chart status file names");
        CHECK(dcol::statusName("", 180).empty() && dcol::statusName("ES", 0).empty() && dcol::statusName("E/S", 180).empty(), "no market / bar size / odd name: no per-chart file");
        std::map<std::string, std::string> kv = dcol::mstGex(lv, nodes, 7703.0, 0.25, 2, 7.4, false);
        mst::publish("GEX", "ESZ6", 180, "ES 3 Min", 5000, at(2026, 10, 19, 9, 3), kv);
        mst::Facts F = mst::read("GEX", "ESZ6", 180, "ES 3 Min", 9000);
        CHECK(F.ok && F.kv == kv && mst::getS(F, "regime") == "neg" && mst::getD(F, "near_lots") == -560, "board round trip (near_level '@' and '/' survive)");
        mst::clear("GEX", "ESZ6", 180, "ES 3 Min"); CHECK(!mst::read("GEX", "ESZ6", 180, "ES 3 Min", 9000).ok, "cleared");
        std::ifstream src("DealerProfile.cpp"); std::stringstream ss; ss << src.rdbuf(); const std::string s = ss.str();
        // (2026-10-22) the version moves on with every approved build (2.17.x .. 2.19.0, DealerProfile.cpp DP_VERSION history): the check is
        // that DP_VERSION and setVersion agree and are at least 2.17.0, not the literal 2.17.0 (test_dealerprofile_2190 pins the current one)
        auto lit = [&](const std::string& pre) { size_t p = s.find(pre); if (p == std::string::npos) return std::string(); p += pre.size(); size_t q = s.find('"', p); return q == std::string::npos ? std::string() : s.substr(p, q - p); };
        const std::string dv = lit("static const char* DP_VERSION = \""), sv = lit("p->setVersion(\"");
        int a = 0, b = 0, c = 0; const bool parsed = std::sscanf(dv.c_str(), "%d.%d.%d", &a, &b, &c) == 3;
        CHECK(!dv.empty() && dv == sv && parsed && (a > 2 || (a == 2 && b >= 17)), "DP_VERSION == setVersion, >= 2.17.0");
        CHECK(s.find("static dpaudit::WriteGate gate;") == std::string::npos, "no function-static write gate (lesson 1)");
        CHECK(std::string(dcol::VERSION) == "1.2.0", "DealerColumnsLogic 1.2.0");
    }
    std::printf("dealerprofile 2.17.0: %d checks, %d failures\n", n, f);
    return f ? 1 : 0;
}
