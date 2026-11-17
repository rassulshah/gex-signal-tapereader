// FlowRead 1.1.0: the plain-English sentences of the Book Flow read
#include "../FlowRead.h"
#include <iostream>
static int P = 0, Fl = 0;
#define CHECK(c, m) do { if (c) ++P; else { ++Fl; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
static std::string all(const std::vector<std::string>& E) { std::string s; for (auto& e : E) s += e + " | "; return s; }
int main() {
    { flr::In a; a.add[0] = 1364; a.pull[0] = 1304; a.add[1] = 1053; a.pull[1] = 934; a.hit[0] = 89; a.hit[1] = 143; a.ref[0] = 223; a.ref[1] = 305; a.pullX[0] = 0.8; a.pullX[1] = 0.5;   // the 8:30 AM screenshot
      const auto E = flr::explain(a); const std::string s = all(E);
      CHECK(E.size() >= 2 && s.find("offers refilled 305") != std::string::npos && s.find("Sellers stacked 2.0x more offers") != std::string::npos, "8:30: absorbed buyers + resistance building (" << s << ")");
      CHECK(flr::read(a).verdict == -1, "8:30: bearish"); }
    { flr::In a; a.add[0] = 500; a.pull[0] = 100; a.add[1] = 100; a.pull[1] = 300; a.pullX[1] = 2.5; a.hit[0] = 50; a.hit[1] = 80;
      const auto E = flr::explain(a); const std::string s = all(E);
      CHECK(s.find("support is building below") != std::string::npos && s.find("Offers are being pulled (2.5x normal)") != std::string::npos && s.find("Buyers were the more aggressive") != std::string::npos, "bullish book (" << s << ")"); }
    { flr::In a; const auto E = flr::explain(a); CHECK(E.size() == 1 && E[0].find("evenly") != std::string::npos, "empty candle: one quiet sentence"); }
    { flr::In a; a.hit[0] = 10; a.hit[1] = 10; const auto E = flr::explain(a); CHECK(all(E).find("even") != std::string::npos, "even market orders"); }
    { flr::In a; a.ice[1] = 5; a.add[1] = 10; const auto E = flr::explain(a); CHECK(all(E).find("hidden sellers") != std::string::npos, "icebergs on the offer"); }
    for (int k = 0; k < 2000; ++k) { flr::In a; for (int s2 = 0; s2 < 2; ++s2) { a.add[s2] = (k * 37 + s2 * 11) % 900; a.pull[s2] = (k * 53 + s2 * 7) % 900; a.hit[s2] = (k * 13 + s2) % 300; a.ref[s2] = (k * 29) % 400; a.ice[s2] = (k + s2) % 4; a.pullX[s2] = (k % 30) / 10.0; }
        const auto E = flr::explain(a); bool ok = !E.empty(); for (auto& e : E) if (e.empty() || e.back() != '.') ok = false; if (!ok) { CHECK(false, "sweep " << k << ": " << all(E)); break; } }
    ++P;
    std::cout << "t_flowread_110: " << P << " passed, " << Fl << " failed\n"; return Fl ? 1 : 0;
}
