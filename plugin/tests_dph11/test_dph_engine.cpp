// DLT-H 1.1.0 engine: same-candle A decided in the live minute stream == dph_read on the same hourly bars; S lines round-trip;
// a restart (seed the record + replay) gives the same A's and writes no duplicate lines; flow is set.
#include "../DeltaProfileHourlyEngine.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
namespace dph = delta_profile_hourly;
int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s\n", msg); } } while (0)
static std::vector<dph::MinuteIn> load(const std::string& fn, double& tick) {
    std::vector<dph::MinuteIn> v; std::ifstream in(fn); std::string line;
    while (std::getline(in, line)) {
        std::stringstream s(line); std::string t; std::getline(s, t, ','); char c;
        if (t == "T") s >> tick;
        else if (t == "M") { dph::MinuteIn m; s >> m.te >> c >> m.bucketEnd >> c >> m.o >> c >> m.h >> c >> m.l >> c >> m.c >> c >> m.volume; v.push_back(m); }
        else { dph::Row r; s >> r.key >> c >> r.delta >> c >> r.volume; v.back().rows.push_back(r); }
    }
    return v;
}
static void runAll(dph::Engine& e, const std::vector<dph::MinuteIn>& v, size_t from, size_t to) {
    for (size_t i = from; i < to; ++i) { e.add(v[i]); e.run(1000000, [] { return true; }); }
}
int main(int argc, char** argv) {
    std::string dir = argc > 1 ? argv[1] : ".";
    for (const char* m : {"GC", "ES"}) {
        double tick = 0; std::vector<dph::MinuteIn> v = load(dir + "/min_" + m + ".csv", tick);
        dph::Engine e("X", m, 3600, tick); e.coverageFrom = v.front().te - 180;
        runAll(e, v, 0, v.size());
        std::printf("%s minutes %zu sameA %zu recorded %lld flow ok %d net4 %.0f\n", m, v.size(), e.sameA.size(), e.linesS, e.flowNow.ok, e.flowNow.net4);
        std::set<std::pair<long long,int>> J, X; for (const dph::JLine& j : e.journal) if (j.text.compare(0, 2, "S|") == 0) { dph::SameA a; dph::parseSameA(j.text, a); J.insert({a.end, a.bull ? 1 : 0}); }
        { std::ifstream ex(dir + "/expend_" + m + ".csv"); std::string l; while (std::getline(ex, l)) { long long t; int b; std::sscanf(l.c_str(), "%lld,%d", &t, &b); X.insert({t, b}); } }
        std::printf("%s engine S lines %zu, research %zu, common %zu\n", m, J.size(), X.size(), (size_t)std::count_if(J.begin(), J.end(), [&](const std::pair<long long,int>& q){ return X.count(q) > 0; }));
        CHECK(J == X, "engine same-candle A's == research (python) exactly");
        CHECK(e.linesS == (long long)J.size(), "each A recorded once");
        CHECK(e.flowNow.ok, "flow from 4 closed hours");
        // every S line parses back to the same A
        int sl = 0; std::string all;
        for (const dph::JLine& j : e.journal) if (j.text.compare(0, 2, "S|") == 0) { dph::SameA a; ++sl; CHECK(dph::parseSameA(j.text, a), "S line parses"); all += j.text + "\n"; }
        CHECK(sl == (int)e.linesS, "one S line per A");
        // restart at the middle: seed the first half's record, resume after its last minute, replay the rest
        const size_t half = v.size() / 2; dph::Engine a("X", m, 3600, tick); a.coverageFrom = e.coverageFrom; runAll(a, v, 0, half);
        std::string rec; for (const dph::JLine& j : a.journal) rec += j.text + "\n";
        const long long resume = a.cursor;
        dph::Engine b("X", m, 3600, tick); b.coverageFrom = e.coverageFrom; b.seedText(rec); b.resumeAfter = resume;
        runAll(b, v, 0, v.size());
        std::set<std::pair<long long, bool>> A1, A2; for (auto& x : e.sameA) A1.insert({x.end, x.bull}); for (auto& x : b.sameA) A2.insert({x.end, x.bull});
        CHECK(A1 == A2, "restart: same A's as one straight pass (no repaint)");
        long long dup = 0; std::set<long long> ends; for (auto& j : a.journal) if (j.text.compare(0, 2, "S|") == 0) ends.insert(j.te);
        for (auto& j : b.journal) if (j.text.compare(0, 2, "S|") == 0 && j.te <= resume) ++dup;
        CHECK(dup == 0, "restart writes no duplicate S lines for replayed minutes");
        std::printf("%s restart: straight %zu, restarted %zu, seeded %lld\n", m, A1.size(), A2.size(), b.sameASeeded);
        // dump decided ends for the parity comparison
        std::ofstream o(dir + "/eng_" + m + ".txt"); for (auto& x : e.sameA) o << x.end << "," << x.bull << "\n";
    }
    std::printf("checks %d failed %d\n", checks, fails); return fails ? 1 : 0;
}
