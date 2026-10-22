// test_cobimpulse_031.cpp -- lsCobImpulse 0.3.1 (2026-10-14, Rassul approved the external-audit display list at 18:25 CT, section 1)
// g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra test_cobimpulse_031.cpp -o t031 && ./t031
#include "CobImpulseLogic.h"
#include <cstdio>
static int n = 0, f = 0;
#define CHECK(c, m) do { ++n; if (!(c)) { ++f; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, m); } } while (0)
static bool any(const std::vector<std::string>& L, const std::string& k) { for (const auto& s : L) if (s.find(k) != std::string::npos) return true; return false; }
int main()
{
    CHECK(std::string(cimp::VERSION) >= "0.3.1", "version 0.3.1 or later");   // (2026-10-22) CobImpulseLogic moved on (0.4.0, 2026-10-18 live CoB odds); the 0.3.1 display rules below still hold
    cimp::GTag g; g.computed = true; g.code = "CR"; g.kind = "origin"; g.px = 6020; g.sign = 1; g.gw = 6; g.w = 4.5; g.asof = 1000;
    // before: "Gamma: origin in CR 6,020 zone (the leg started inside it; ...)" -> after: "MenthorQ level near the CoB start: CR 6,020 (model, data 7 min old)."
    const std::vector<std::string> L = cimp::gammaLines(g, true, 0.25, false, 7 * 60 + 5);
    CHECK(!L.empty() && L[0] == "MenthorQ level near the CoB start: CR 6,020 (model, data 7 min old).", "the approved hover line, with the data age");
    CHECK(!any(L, "driving") && !any(L, "Gamma: origin in") && !any(L, "gamma driver"), "no 'driving' / old wording");
    const std::vector<std::string> L2 = cimp::gammaLines(g, true, 0.25, false);
    CHECK(!L2.empty() && L2[0] == "MenthorQ level near the CoB start: CR 6,020 (model).", "age unknown: (model)");
    cimp::HoverIn h; h.side = 1; h.v = true; h.vn = 40; h.vTime = "8:33 AM"; h.g = g; h.tick = 0.25; h.bookAgeS = 420;
    cimp::Params P; const std::vector<std::string> H = cimp::hover(h, P);
    CHECK(!H.empty() && H[0].find("is a MenthorQ level near its start?") != std::string::npos && H[0].find("behind") == std::string::npos, "headline: near its start, not behind it");
    CHECK(any(H, "(model, data 7 min old)"), "hover carries the age");
    CHECK(cimp::label(false, true, true, P, false, "CR").find("CR?") != std::string::npos, "the tag itself is unchanged (Cob+V+CR?)");
    std::printf("cobimpulse 0.3.1: %d checks, %d failures\n", n, f);
    return f ? 1 : 0;
}
