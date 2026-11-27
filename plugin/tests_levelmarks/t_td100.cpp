// ThrustDot 1.0.0 on ES 2026-11-27 (IRT store bars, START stamps): the cyan thrust dots, no repaint
#include "../ThrustDot.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <ctime>
static long long toT(const std::string& s) { std::tm t{}; std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec); t.tm_year -= 1900; t.tm_mon -= 1; return (long long)timegm(&t); }
static int fails = 0, passes = 0; static void chk(bool c, const std::string& w) { std::printf("%s: %s\n", c ? "PASS" : "FAIL", w.c_str()); c ? ++passes : ++fails; }
int main() {
    std::ifstream f("es1127c.csv"); std::string ln; std::vector<wyr::Bar> B; std::vector<std::string> W; tdot::Chart X;
    while (std::getline(f, ln)) { std::stringstream ss(ln); std::string c[6]; for (int k = 0; k < 6; ++k) std::getline(ss, c[k], ',');
        wyr::Bar b; b.t = toT(c[0]) + 180; b.o = std::llround(std::stod(c[1]) * 4); b.h = std::llround(std::stod(c[2]) * 4); b.l = std::llround(std::stod(c[3]) * 4); b.c = std::llround(std::stod(c[4]) * 4);
        B.push_back(b); W.push_back(c[0].substr(11, 5)); X.T.push_back(b.t); X.O.push_back(std::stod(c[1])); X.H.push_back(std::stod(c[2])); X.L.push_back(std::stod(c[3])); X.C.push_back(std::stod(c[4])); }
    auto at = [&](long long t) { size_t i = 0; while (i < B.size() && B[i].t != t) ++i; return i; };
    const auto D = tdot::dots(B, X);
    for (const auto& kv : D) std::printf("  dot %s start %s\n", W[at(kv.first)].c_str(), kv.second > 0 ? "bull" : "bear");
    bool d906 = false, d1118 = false; for (const auto& kv : D) { if (W[at(kv.first)] == "09:06" && kv.second == 1) d906 = true; if (W[at(kv.first)] == "11:18" && kv.second == 1) d1118 = true; }
    chk(d906, "9:09 AM (store 09:06) green, Thrust 1.34 / PIC 2.62, low under the 9:06 pivot candle's high: bullish dot");
    chk(d1118, "11:21 AM (store 11:18) green, Thrust 1.58 / PIC 2.01: bullish dot");
    bool colour = true, strong = true; for (const auto& kv : D) { const size_t i = at(kv.first); if (kv.second > 0 ? !(B[i].c > B[i].o) : !(B[i].c < B[i].o)) colour = false; double th, pc; if (!tdot::thPic(X, (int)i, th, pc) || th < 1.2 || pc < 1.2) strong = false; }
    chk(colour, "every dot has its own colour (green bullish, red bearish)"); chk(strong, "every dot: Thrust >= 1.20 and PIC >= 1.20");
    // no repaint: the dots of every prefix are exactly the full run's dots up to that candle
    bool same = true; for (size_t n = 30; n <= B.size(); n += 13) { const std::vector<wyr::Bar> b2(B.begin(), B.begin() + n); const auto D2 = tdot::dots(b2, X);
        for (const auto& kv : D) if (kv.first <= b2.back().t && (!D2.count(kv.first) || D2.at(kv.first) != kv.second)) same = false;
        for (const auto& kv : D2) if (!D.count(kv.first)) same = false; }
    chk(same, "no repaint: every prefix shows exactly the same dots");
    // the done cache gives the same answer
    std::map<long long, int> done; std::vector<long long> dec; const auto D3 = tdot::dots(std::vector<wyr::Bar>(B.begin(), B.begin() + 200), X, nullptr, &dec);
    for (long long t : dec) { done[t] = D3.count(t) ? D3.at(t) : 0; }
    const auto D4 = tdot::dots(B, X, &done); chk(D4 == D, "the decided-candle cache gives the same dots");
    { double th, pc; chk(!tdot::thPic(X, 5, th, pc), "fewer than 14 candles before: no dot"); }
    std::printf("%s (%d pass, %d failed)\n", fails ? "FAILED" : "ALL PASS", passes, fails); return fails ? 1 : 0;
}
