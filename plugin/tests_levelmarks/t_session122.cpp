// LevelMarks 1.22.0 (Rassul 2026-11-25 11:39 CT "update the rule to current session"): GC 2026-11-25, his 3-min chart's own bars
// (lsLimitProfile's LRA-IRT-Bars-GC export, stamps = bar END) + his key levels (LRA-KeyLevels-GC). AsiaLO 3511.3 was traded through
// before the open (last at the 07:15 candle) and never in the day session until the 11:21 candle went to 3516.2 and the 11:24 candle to
// 3516.7 and closed back under it at 3498.4 -> 1.22.0 marks a bearish Sw on the 11:24 candle; 1.21.0 (formation look-back) does not.
// Also: an overnight Sw is unchanged (the day rule never applies before the open).
#include "../LevelMarks.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <ctime>
static long long toT(const std::string& s) { std::tm t{}; std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec); t.tm_year -= 1900; t.tm_mon -= 1; return (long long)timegm(&t); }
int main() {
    std::ifstream f("gc1125_irt.psv"); std::string ln; std::vector<wyr::Bar> B; std::vector<std::string> W;
    while (std::getline(f, ln)) { if (ln.compare(0, 4, "2026") != 0) continue; std::stringstream ss(ln); std::string c[6]; for (int k = 0; k < 6; ++k) std::getline(ss, c[k], '|');
        wyr::Bar b; b.t = toT(c[0]); b.o = std::llround(std::stod(c[1]) * 10); b.h = std::llround(std::stod(c[2]) * 10); b.l = std::llround(std::stod(c[3]) * 10); b.c = std::llround(std::stod(c[4]) * 10);
        const long long v = std::llround(std::stod(c[5])); b.buy = v / 2; b.sell = v - v / 2; B.push_back(b); W.push_back(c[0].substr(11, 5)); }
    std::vector<wyr::Lvl> K; std::ifstream kf("gc1125_keys.txt");
    while (std::getline(kf, ln)) { if (ln.compare(0, 2, "L|") != 0) continue; std::stringstream ss(ln); std::string c[4]; for (int k = 0; k < 4; ++k) std::getline(ss, c[k], '|');
        const std::string nm = c[1];   // the level file's own names (AsiaLO, ONL/LonLO ...), as RevRead hands them over
        wyr::Lvl z; z.px = std::llround(std::stod(c[2]) * 10); z.name = nm; K.push_back(z); }
    std::vector<lvm::H3> H(B.size()); lvm::Blk4 b4, b3; b3.len = 10800; b3.off = 7200;
    for (size_t i = 0; i < B.size(); ++i) { const auto f4 = b4.add(B[i].t, 180, B[i].h, B[i].l); const auto f3 = b3.add(B[i].t, 180, B[i].h, B[i].l); H[i].h4h = f4.first; H[i].h4l = f4.second; H[i].h3h = f3.first; H[i].h3l = f3.second; }
    std::printf("dayStart(11:24) = %s, dayStart(06:15) = %d\n", [&]{ for (size_t i = 0; i < B.size(); ++i) if (W[i] == "11:24") { int d = lvm::dayStart(B, (int)i, lvm::rthOpenMin("GC")); return d >= 0 ? W[(size_t)d].c_str() : "-1"; } return "?"; }(),
        [&]{ for (size_t i = 0; i < B.size(); ++i) if (W[i] == "06:15") return lvm::dayStart(B, (int)i, lvm::rthOpenMin("GC")); return -9; }());
    const auto M = lvm::run(B, K, lvm::rthOpenMin("GC"), 180, nullptr, &H);
    int sw = 0, open = 0;
    for (const auto& kv : M) { size_t i = 0; while (i < B.size() && B[i].t != kv.first) ++i;
        if (kv.second.code == 'S') std::printf("%s  Sw %s dir %d\n", W[i].c_str(), kv.second.lv.c_str(), kv.second.dir);
        if (W[i] == "11:24" && kv.second.code == 'S' && kv.second.dir == -1) sw = 1; }
    for (size_t i = 0; i < B.size(); ++i) if (W[i] == "07:21") open = lvm::dayStart(B, (int)i, lvm::rthOpenMin("GC")) == (int)i;
    std::printf("%s: bearish Sw on the 11:24 candle (AsiaLO untouched in the day session)\n", sw ? "PASS" : "FAIL");
    std::printf("%s: the day session starts at the RTH opening candle (07:21 stamp)\n", open ? "PASS" : "FAIL");
    return (sw && open) ? 0 : 1; }
