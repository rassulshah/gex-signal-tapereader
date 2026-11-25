// LevelMarks 1.21.0 on GC 2026-11-25 (his chart, the IRT bar store): the candle that closes below the 3-hour low gets its Bk
#include "../LevelMarks.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <ctime>
static long long toT(const std::string& s) { std::tm t{}; std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec); t.tm_year -= 1900; t.tm_mon -= 1; return (long long)timegm(&t); }
int main() {
    std::ifstream f("gc1125.csv"); std::string ln; std::vector<wyr::Bar> B; std::vector<std::string> W;
    while (std::getline(f, ln)) { std::stringstream ss(ln); std::string c[6]; for (int k = 0; k < 6; ++k) std::getline(ss, c[k], ',');
        wyr::Bar b; b.t = toT(c[0]) + 180; b.o = std::llround(std::stod(c[1]) * 10); b.h = std::llround(std::stod(c[2]) * 10); b.l = std::llround(std::stod(c[3]) * 10); b.c = std::llround(std::stod(c[4]) * 10);
        const long long v = std::llround(std::stod(c[5])); b.buy = v / 2; b.sell = v - v / 2; B.push_back(b); W.push_back(c[0].substr(11, 5)); }
    std::vector<lvm::H3> H(B.size()); lvm::Blk4 b4, b3; b3.len = 10800; b3.off = 7200;
    for (size_t i = 0; i < B.size(); ++i) { const auto f4 = b4.add(B[i].t, 180, B[i].h, B[i].l); const auto f3 = b3.add(B[i].t, 180, B[i].h, B[i].l); H[i].h4h = f4.first; H[i].h4l = f4.second; H[i].h3h = f3.first; H[i].h3l = f3.second; }
    const auto M = lvm::run(B, std::vector<wyr::Lvl>(), lvm::rthOpenMin("GC"), 180, nullptr, &H);
    int ok = 0;
    for (const auto& kv : M) { size_t i = 0; while (i < B.size() && B[i].t != kv.first) ++i; if (W[i] < "05:00") continue;
        std::printf("%s start  %s %s dir %d  (3h low %.1f)\n", W[i].c_str(), lvm::codeText(kv.second.code), kv.second.lv.c_str(), kv.second.dir, H[i].h3l / 10.0);
        if (W[i] == "08:30" && kv.second.code == 'B' && kv.second.dir == -1 && kv.second.lv == "3HrL") ok = 1; }
    std::printf("%s: the candle that closed 3462.7 under the 3-hour low 3465.1 (bar stamped 08:30 in the store; range up, volume down) is Bk 3HrL\n", ok ? "PASS" : "FAIL"); return ok ? 0 : 1;
}
