// tests_strips/replay_stage.cpp -- lsReversalStage 1.0.0 replay on Rassul's REAL ES 3-min day (2026-10-21 20:27 - 2026-10-22 11:21 CT):
// bars from his ChartView export, the Defense letters from his lsBookFlow turns file (T lines with A, H lines, Q lines L / X - the attacker
// letters S / K / L / R / Tr / G and W / P / B were not recorded on his PC yet: 1.26+ records start with this build). Checks: every candle
// decided once, a run on each prefix of the day equals the full run on that prefix (no repaint as candles arrive), and prints the staircase.
//   g++ -std=c++17 -Wall -Wextra -Werror -I.. replay_stage.cpp && ./a.out replay/ES-180-20261022-bars.csv replay/ES-180-turns-20261022.csv
#include "../ReversalStageLogic.h"
#include "../DefenseTurn.h"
#include <cstdio>
#include <fstream>
#include <sstream>
int main(int argc, char** argv) {
    if (argc < 3) { std::printf("usage: bars.csv turns.csv\n"); return 2; }
    std::vector<rsl::Bar> B; { std::ifstream f(argv[1]); std::string ln; while (std::getline(f, ln)) { if (ln.empty() || ln[0] == '#') continue; rsl::Bar b; char c;
        std::stringstream s(ln); s >> b.key >> c >> b.h >> c >> b.l >> c >> b.c; b.ok = true; B.push_back(b); } }
    std::multimap<long long, rsl::Ev> E; int nA = 0, nH = 0, nL = 0, nX = 0;
    auto add = [&](long long key, const char* code, int piv, const std::string& nums) { rsl::Ev e; e.key = key; e.pane = lst::DEF; e.l = lst::make(lst::DEF, code, piv, -1, nums); E.insert(std::make_pair(key, e)); };
    { std::ifstream f(argv[2]); std::string ln; while (std::getline(f, ln)) {
        dt::Line L; dt::Counter h; dt::Retest q;
        if (dt::parseLine(ln, L) && L.kind == 'T' && L.A) { add(L.t, "A", L.side == dt::HIGH ? lst::HIGH : lst::LOW, "margin " + std::to_string(L.margin) + "%"); ++nA; }
        else if (dt::parseH(ln, h) && h.H) { add(h.at, "H", h.side == dt::HIGH ? lst::HIGH : lst::LOW, std::to_string(h.lots) + " lots"); ++nH; }
        else if (dt::parseQ(ln, q) && (q.res == 'L' || q.res == 'X')) { add(q.at, q.res == 'L' ? "L" : "X", q.side == dt::HIGH ? lst::HIGH : lst::LOW, std::to_string(q.rest) + " lots"); q.res == 'L' ? ++nL : ++nX; } } }
    const std::vector<rsl::Step> full = rsl::run(B, E);
    int bad = 0; for (size_t n = 1; n <= B.size(); ++n) { std::vector<rsl::Bar> p(B.begin(), B.begin() + (long)n); const std::vector<rsl::Step> s = rsl::run(p, E); for (size_t k = 0; k < n; ++k) if (!(s[k] == full[k])) ++bad; }
    int attempts = 0, comp = 0, fail = 0, exp = 0, maxStage[7] = {0, 0, 0, 0, 0, 0, 0}; int inB = 0;
    for (const rsl::Step& s : full) { if (s.key >= B.front().key) ++inB; if (s.status == rsl::COMPLETE) ++comp; if (s.status == rsl::FAILED) ++fail; if (s.status == rsl::EXPIRED) ++exp; }
    { long long cur = -1; int best = 0; for (const rsl::Step& s : full) { if (s.side >= 0 && s.start != cur) { if (cur >= 0) ++maxStage[best]; cur = s.start; best = 0; ++attempts; } if (s.side >= 0) best = std::max(best, s.status == rsl::FAILED && s.stage == 0 ? s.failedStage : s.stage); } if (cur >= 0) ++maxStage[best]; }
    std::printf("ES 3-min %zu candles, letters A %d H %d L %d X %d in the bars' range\n", B.size(), nA, nH, nL, nX);
    std::printf("attempts %d: complete (1 ATR) %d, failed %d, expired %d; highest stage 0:%d 1:%d 2:%d 3:%d 4:%d 5:%d 6:%d\n", attempts, comp, fail, exp, maxStage[0], maxStage[1], maxStage[2], maxStage[3], maxStage[4], maxStage[5], maxStage[6]);
    std::printf("prefix runs that differ from the full run (repaint): %d\n", bad);
    if (argc > 3) for (const rsl::Step& s : full) if (s.side >= 0 && (s.stage > 0 || s.status == rsl::FAILED)) {
        const long long t = s.key % 86400; std::printf("%02lld:%02lld %s stage %d %s %s %s\n", t / 3600, (t / 60) % 60, rsl::sideWord(s.side).c_str(), s.stage, rsl::statusName(s.status), s.chain.c_str(), s.why.c_str()); }
    return bad ? 1 : 0;
}
