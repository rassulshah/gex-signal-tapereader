// WyRead 1.0.0 - the Wyckoff strip on Rassul's ES 2026-11-17/18 overnight (11:00 PM - 5:24 AM, the mockup wyckoff-turn-chart.html)
#include "../WyRead.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int ok = 0, bad = 0;
#define CHECK(c, m) do { if (c) ++ok; else { ++bad; std::printf("FAIL line %d: %s\n", __LINE__, m); } } while (0)
static std::vector<wyr::Bar> load(const char* f) {
    std::vector<wyr::Bar> B; std::ifstream in(f); std::string line;
    while (std::getline(in, line)) { std::vector<std::string> v; std::stringstream ss(line); std::string x; while (std::getline(ss, x, '|')) v.push_back(x);
        if (v.size() < 18 || v[0] != "B") continue;
        wyr::Bar b; b.t = 86400LL * 10 + 23 * 3600 + std::stoll(v[1]) * 180;   // (1.3.0) real clock stamps: candle 0 ENDS 11:00 PM, so the hour / 4h levels line up
        b.o = std::stoll(v[2]); b.h = std::stoll(v[3]); b.l = std::stoll(v[4]); b.c = std::stoll(v[5]); b.buy = std::stoll(v[6]); b.sell = std::stoll(v[7]); b.book = v[8] == "1";
        b.addNet[0] = std::stoll(v[9]); b.addNet[1] = std::stoll(v[10]); b.aggrMax[0] = std::stoll(v[11]); b.aggrMax[1] = std::stoll(v[12]); b.sweepLv[0] = std::stoll(v[13]); b.sweepLv[1] = std::stoll(v[14]); b.ice[0] = std::stoll(v[15]); b.ice[1] = std::stoll(v[16]); b.when = v[17];
        B.push_back(b); }
    return B;
}
static bool has(const wyr::Out& o, const std::vector<wyr::Bar>& B, const std::string& when, char k, int dir) {
    for (const wyr::Bar& b : B) if (b.when == when) { const auto it = o.marks.find(b.t); if (it == o.marks.end()) return false; for (const wyr::Mark& m : it->second) if (m.k == k && m.dir == dir) return true; return false; }
    return false;
}
int main(int argc, char** argv) {
    const std::vector<wyr::Bar> B = load(argc > 1 ? argv[1] : "es1118.psv");
    CHECK(B.size() == 129, "129 candles");
    // (1.6.0) Sp / Ut need a higher-timeframe / key level: the fixture has no level file, so the two springs he marked are given the
    // levels under them as key levels (the 11:09 PM low 7367.75 and the 12:54 AM low 7373.25, counted from 12:57 AM on)
    std::vector<wyr::Lvl> lv(2); lv[0].px = 29471; lv[0].name = "LonLO"; lv[1].px = 29493; lv[1].name = "ONL"; lv[1].from = 39;
    const wyr::Out o = wyr::run(B, 0.25, 2, wyr::Params(), lv);
    const wyr::Out o0 = wyr::run(B, 0.25, 2);   // no levels: no Sp / Ut at all (every break that night was of a local floor)
    { int nS = 0; for (auto& kv : o0.marks) for (auto& m : kv.second) if (m.k == 'S' || m.k == 'U') ++nS; CHECK(nS == 0, "1.6.0: no level file = no Sp / Ut on local floors"); }
    for (const wyr::Bar& b : B) { const auto it = o.marks.find(b.t); if (it == o.marks.end()) continue; std::string s; for (const wyr::Mark& m : it->second) s += wyr::label(m) + (m.dir > 0 ? "+ " : "- ");
        std::printf("  %-9s %s\n", b.when.c_str(), s.c_str()); }
    // his three springs (the 08:06 / 08:10 CT screenshots)
    CHECK(!has(o, B, "12:03 AM", 'S', 1), "1.6.0: no Sp 12:03 AM - its low 7367.75 only matched the 11:09 PM low (a double bottom), it broke the local 11:39 low 7372.00 only");
    CHECK(has(o, B, "1:39 AM", 'S', 1), "Sp 1:39 AM (absorbed)");
    CHECK(!has(o, B, "3:03 AM", 'S', 1), "1.6.0: no Sp 3:03 AM - it broke a local floor only; the 2 AM hour low 7387.00 held");
    CHECK(!has(o, B, "1:42 AM", 'I', 1), "1.0.2: no I 1:42 AM - its low is 18 ticks above the spring low, ATR 15.1");
    CHECK(!has(o, B, "3:06 AM", 'I', 1), "1.3.0: no I 3:06 AM - the 3:03 spring (7392.25) broke only a local floor; the 2 AM hour low 7387.00 held, so no higher-timeframe breakout failed");
    // the turns of the mockup: 3:45 AM top and 5:18 AM low
    CHECK(has(o, B, "3:30 AM", 'E', -1) || has(o, B, "3:39 AM", 'E', -1) || has(o, B, "3:45 AM", 'E', -1), "E in the leg into the 3:45 AM top");
    CHECK(has(o, B, "3:45 AM", 'A', -1), "A 3:45 AM");
    CHECK(has(o, B, "5:15 AM", 'E', 1) || has(o, B, "5:18 AM", 'E', 1), "E at the 5:18 AM low");
    CHECK(has(o, B, "5:18 AM", 'A', 1), "A 5:18 AM");
    CHECK(has(o, B, "2:00 AM", 'A', -1), "A 2:00 AM");
    // (1.0.1, 09:21 CT) a bullish I trades below the high of a pivot-low candle in the 3 before it; a bearish I above the low of a pivot-high candle
    {   int badI = 0; for (size_t i = 3; i < B.size(); ++i) { const auto it = o.marks.find(B[i].t); if (it == o.marks.end()) continue;
            for (const wyr::Mark& m : it->second) { if (m.k != 'I') continue; bool ok2 = false; for (size_t k = i - 3; k < i; ++k) if (m.dir > 0 ? B[i].l < B[k].h : B[i].h > B[k].l) ok2 = true; if (!ok2) ++badI; } }
        CHECK(badI == 0, "every I starts at its pivot"); CHECK(!has(o, B, "11:18 PM", 'I', 1), "11:18 PM I (mid-move, away from the pivot) is gone"); }
    // reads
    // (1.6.0) the 12:03 AM flush / stop-run read checks went with that spring (no higher-timeframe level was broken)
    for (const wyr::Bar& b : B) if (b.when == "1:39 AM") { const auto r = o.reads.find(b.t); CHECK(r != o.reads.end() && r->second.head == "Spring (absorbed)", "absorbed read"); if (r != o.reads.end()) for (const auto& l : r->second.lines) std::printf("    %s\n", l.c_str()); }
    // no repaint: replay candle by candle - a letter, once shown, never changes or goes (a letter may be ADDED later on the same candle:
    // Sp / Ut are decided after the reclaim and two candles without follow-through)
    std::map<long long, std::vector<std::string> > seen; int changed = 0;
    for (size_t n = 15; n <= B.size(); ++n) { const std::vector<wyr::Bar> sub(B.begin(), B.begin() + (long)n); const wyr::Out q = wyr::run(sub, 0.25, 2);
        for (const auto& kv : seen) { const auto it = q.marks.find(kv.first); for (const std::string& want : kv.second) { bool f = false;
                if (it != q.marks.end()) for (const auto& m : it->second) if (wyr::label(m) + std::to_string(m.dir) == want) f = true;
                if (!f) { ++changed; std::printf("  repaint at n=%zu t=%lld: lost %s\n", n, kv.first, want.c_str()); } } }
        for (const auto& kv : q.marks) { std::vector<std::string>& v = seen[kv.first]; for (const auto& m : kv.second) { const std::string s2 = wyr::label(m) + std::to_string(m.dir); if (std::find(v.begin(), v.end(), s2) == v.end()) v.push_back(s2); } } }
    CHECK(changed == 0, "no letter ever changes or disappears");
    // empty / short / bad input
    CHECK(wyr::run(std::vector<wyr::Bar>(), 0.25, 2).marks.empty(), "empty");
    CHECK(wyr::run(B, 0.0, 2).marks.empty(), "tick 0");
    std::printf("t_wyread_100: %d passed, %d failed\n", ok, bad); return bad ? 1 : 0;
}
