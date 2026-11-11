// test_readbox_110.cpp -- ReadBox 1.1.0 (Rassul 2026-11-11 11:41 CT "updat eht readt at the top to be more simple"; 11:45 "where it says HTF change it
// to Hrly Trend: and give it values like up or down or NA"): Hrly Trend / Day / Lvl / Mkt / Lmt; the RTH Day line without ADR / Used.
// Run (from plugin/): g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. tests_readbox/test_readbox_110.cpp -o t && ./t
#include "../ReadBox.h"
#include <cstdio>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s\n", m); } } while (0)
int main() {
    rbx::In I; I.mkt = "ES"; I.dec = 2; I.per = 180; I.openMin = 510; I.closeMin = 900; I.day.kind = 0; I.day.pBig = 0.3; I.day.pSmall = 0.35; I.day.xr = 1.0; I.day.adr = 60;
    const long long day0 = 20760LL * 86400;   // a CT civil day; bars every 3 min from the prior 17:00 to 10:30 the next morning
    double px = 7500; unsigned s = 7; for (long long t = day0 - 7 * 3600; t <= day0 + 10 * 3600 + 1800; t += 180) { s = s * 1103515245u + 12345u; const double d = ((int)((s >> 16) % 9) - 4) * 0.25;
        rbx::Bar b; b.t = t; b.o = px; px += d; b.c = px; b.h = std::max(b.o, b.c) + 0.5; b.l = std::min(b.o, b.c) - 0.5; I.bars.push_back(b); }
    const rbx::Out o = rbx::build(I);
    CHECK(o.ok && o.n == 5, "5 lines");
    CHECK((o.l[0].text == "Hrly Trend: Up" || o.l[0].text == "Hrly Trend: Down" || o.l[0].text == "Hrly Trend: NA") && o.l[1].text.compare(0, 4, "Day:") == 0 && o.l[2].text.compare(0, 4, "Lvl:") == 0 && o.l[3].text.compare(0, 4, "Mkt:") == 0 && o.l[4].text.compare(0, 4, "Lmt:") == 0, "the lines are Hrly Trend / Day / Lvl / Mkt / Lmt");
    CHECK(o.l[0].col == (o.l[0].text == "Hrly Trend: Up" ? rbx::GREEN : o.l[0].text == "Hrly Trend: Down" ? rbx::RED : rbx::WHITE), "Up green, Down red, NA white");
    { rbx::In J = I; J.bars.resize(3); const rbx::Out q = rbx::build(J); CHECK(q.ok && q.l[0].text == "Hrly Trend: NA", "no completed hours yet: NA"); }
    { rbx::In J = I; for (size_t k = J.bars.size() - 60; k < J.bars.size(); ++k) { const double up = (double)(k - (J.bars.size() - 60)) * 2.0; J.bars[k].o += up; J.bars[k].c += up + 1.0; J.bars[k].h += up + 1.0; J.bars[k].l += up; } const rbx::Out q = rbx::build(J); CHECK(q.l[0].text == "Hrly Trend: Up", "three rising hours: Up"); }
    bool htf = false; for (int k = 0; k < 5; ++k) if (o.l[k].text.find("HTF") != std::string::npos) htf = true;
    CHECK(!htf, "no line says HTF");
    CHECK(o.l[1].text.find("ADR") == std::string::npos && o.l[1].text.find("Used") == std::string::npos && o.l[1].text.find("Nor") != std::string::npos, "the RTH Day line has the day kind and no ADR / Used");
    const rbx::Layout L4 = rbx::layout(0, 0, 800, 600, 200, 10, 4), L5 = rbx::layout(0, 0, 800, 600, 200, 10, o.n);
    CHECK(L4.ok && L5.ok && (L5.box.b - L5.box.t) - (L4.box.b - L4.box.t) == L4.lh, "the box is one line shorter");
    std::printf("  %s | %s\nRESULT %d passed, %d failed\n", o.l[0].text.c_str(), o.l[1].text.c_str(), pass, fail); return fail ? 1 : 0;
}
