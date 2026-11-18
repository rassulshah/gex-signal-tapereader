// Build (in plugin\tests_revread): g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined t_wyread_100.cpp -o t && ./t es_wy1118.psv
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
        wyr::Bar b; b.t = 1000 + std::stoll(v[1]) * 180;
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
    const std::vector<wyr::Bar> B = load(argc > 1 ? argv[1] : "es_wy1118.psv");
    CHECK(B.size() == 129, "129 candles");
    const wyr::Out o = wyr::run(B, 0.25, 2);
    for (const wyr::Bar& b : B) { const auto it = o.marks.find(b.t); if (it == o.marks.end()) continue; std::string s; for (const wyr::Mark& m : it->second) s += wyr::label(m) + (m.dir > 0 ? "+ " : "- ");
        std::printf("  %-9s %s\n", b.when.c_str(), s.c_str()); }
    // his three springs (the 08:06 / 08:10 CT screenshots)
    CHECK(has(o, B, "12:03 AM", 'S', 1), "Sp 12:03 AM (flush)");
    CHECK(has(o, B, "1:39 AM", 'S', 1), "Sp 1:39 AM (absorbed)");
    CHECK(has(o, B, "3:03 AM", 'S', 1), "Sp 3:03 AM (absorbed)");
    CHECK(has(o, B, "1:42 AM", 'I', 1), "I 1:42 AM (the strong rejection)");
    CHECK(has(o, B, "3:06 AM", 'I', 1), "I 3:06 AM (the strong rejection)");
    // the turns of the mockup: 3:45 AM top and 5:18 AM low
    CHECK(has(o, B, "3:30 AM", 'E', -1) || has(o, B, "3:39 AM", 'E', -1) || has(o, B, "3:45 AM", 'E', -1), "E in the leg into the 3:45 AM top");
    CHECK(has(o, B, "3:45 AM", 'A', -1), "A 3:45 AM");
    CHECK(has(o, B, "5:15 AM", 'E', 1) || has(o, B, "5:18 AM", 'E', 1), "E at the 5:18 AM low");
    CHECK(has(o, B, "5:18 AM", 'A', 1), "A 5:18 AM");
    CHECK(has(o, B, "2:00 AM", 'A', -1), "A 2:00 AM");
    // reads
    for (const wyr::Bar& b : B) if (b.when == "12:03 AM") { const auto r = o.reads.find(b.t); CHECK(r != o.reads.end() && r->second.head == "Spring (flush)", "flush read");
        bool sr = false; if (r != o.reads.end()) for (const auto& l : r->second.lines) { std::printf("    %s\n", l.c_str()); if (l.rfind("Stop run: one 72-lot sell swept 5", 0) == 0) sr = true; } CHECK(sr, "stop-run line 72 lots / 5 prices"); }
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
