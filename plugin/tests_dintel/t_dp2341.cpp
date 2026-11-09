// tests_dintel/t_dp2341.cpp - lsDeltaProfile 2.34.1 (Rassul HG 10:18 "get rid of the absorption circles"): nothing drawn on the candles by
// the 2.34.0 A circle path (no ring, no mark, no "A? 2.4x <price>" label); the A stays decided and recorded (DC / DM lines), the letters in
// front of the profile ratios stay. Source-level checks (the DLL's draw path needs the IRT host): the switch is off, the guard returns
// before the first paint call, the record path is untouched, the versions match.
// build: g++ -std=c++17 -O1 -Wall -Wextra -Werror t_dp2341.cpp -o t_dp2341 && ./t_dp2341
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
static int ok = 0, bad = 0;
#define CHECK(c, m) do { if (c) ++ok; else { ++bad; std::cout << "FAIL " << m << "\n"; } } while (0)
static std::string slurp(const char* p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
int main() {
    const std::string src = slurp("../DeltaProfile.cpp"), set = slurp("../DeltaProfileSettings.h");
    CHECK(!src.empty(), "DeltaProfile.cpp read");
    CHECK(src.find("static const bool PAINT_A_CIRCLES = false;") != std::string::npos, "PAINT_A_CIRCLES is false");
    const size_t a = src.find("auto drawCircle = [&](const dintel::Event& e, long long candleStart, bool live) {");
    const size_t g = src.find("if (!PAINT_A_CIRCLES) { ++s.dcHidden; return; }", a), o = src.find("ko::oval(ring", a), t = src.find("textRight(pt.h - 9", a), m = src.find("stroke(pt.h, pt.v - 7", a);
    CHECK(a != std::string::npos && g != std::string::npos && g < o && g < t && g < m, "the guard returns before the ring, the mark and the candle label");
    CHECK(src.find("bk.journal.push_back(dintel::circleLine(e, DLT_VERSION, s.book, endT, tick));") != std::string::npos, "the DC record line is still written");
    CHECK(src.find("DLT_VERSION = \"2.34.1\"") != std::string::npos && src.find("p->setVersion(\"2.34.1\")") != std::string::npos && set.find("PLUGIN_VERSION = \"2.34.1\"") != std::string::npos, "versions 2.34.1 (DLT_VERSION, setVersion, settings spec)");
    std::cout << "t_dp2341: " << ok << " passed, " << bad << " failed\n"; return bad ? 1 : 0;
}
