// test_tapereader_levels_112.cpp -- lsTapeReader 1.1.2: hourly swings are not targets, Asia / CW level buckets
//   g++ -std=c++17 -Wall -fsanitize=address,undefined -I. test_tapereader_levels_112.cpp -o t && ./t
#include "TapeReaderLogic.h"
#include <cstdio>
static int f = 0, p = 0;
#define CHECK(c, m) do { if (c) ++p; else { ++f; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
int main() {
    using tr::Level;
    std::vector<Level> key, mq;
    Level a; a.name = "ONH/AsiaHI"; a.kind = "key"; a.px = 100.0; key.push_back(a);
    Level h; h.name = "HrHI"; h.kind = "key"; h.px = 101.0; key.push_back(h);              // an old KeyLevels file still carrying it
    Level c; c.name = "CWH"; c.kind = "key"; c.px = 102.0; key.push_back(c);
    Level m1; m1.name = "HrLO"; m1.kind = "pivot_1h"; m1.px = 98.0; mq.push_back(m1);       // level_touch keeps logging pivots
    Level m2; m2.name = "CR"; m2.kind = "gamma"; m2.px = 103.0; mq.push_back(m2);
    auto L = tr::mergeLevels(key, mq, {}, 0.25);
    bool hr = false, asia = false, cw = false, cr = false;
    for (auto& v : L) { if (v.name.compare(0, 2, "Hr") == 0) hr = true; if (v.name == "ONH") asia = true; if (v.name == "CWH") cw = true; if (v.name == "CR") cr = true; }
    CHECK(!hr, "no HrHI / HrLO among the targets");
    CHECK(asia && cw && cr && L.size() == 3, "ONH/AsiaHI (short name ONH), CWH, CR kept");
    CHECK(tr::levelType("AsiaHI", "key") == "asia" && tr::levelType("AsiaLO", "key") == "asia", "asia bucket");
    CHECK(tr::levelType("CWH", "key") == "current_week" && tr::levelType("CWL", "key") == "current_week", "current_week bucket");
    CHECK(tr::levelType("CR", "gamma") == "gamma" && tr::levelType("LonHI", "key") == "london" && tr::levelType("PWH", "key") == "prior_week", "old buckets unchanged");
    CHECK(tr::turnLevel("AsiaHI", "key").empty() && tr::turnLevel("CWH", "key").empty() && tr::turnLevel("LonHI", "key") == "LonHI", "ledger rows: Asia / CW base row only (not in the ledger yet)");
    CHECK(std::string(tr::VERSION) >= "1.1.2", "version 1.1.2 or later");   // (2026-10-22) TapeReaderLogic moved on (1.3.3, 2026-10-18); the 1.1.2 buckets above still hold
    std::printf("%d passed, %d failed\n", p, f); return f != 0;
}
