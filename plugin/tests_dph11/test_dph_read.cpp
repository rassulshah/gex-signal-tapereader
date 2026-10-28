// parity: dph_read::sameCandleA on the research hourly bars == the python research events (hdp/p1.py, W24, 2.0x, same-candle A)
#include "../DeltaProfileHourlyRead.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <set>
#include <string>
int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s\n", msg); } } while (0)
int main(int argc, char** argv) {
    const char* mk[] = {"ES", "NQ", "CL", "GC", "HG", "NG", "EU"};
    std::string dir = argc > 1 ? argv[1] : ".";
    for (const char* m : mk) {
        std::vector<dph_read::HBar> bars;
        std::ifstream in(dir + "/hb_" + m + ".csv"); std::string line;
        while (std::getline(in, line)) {
            std::stringstream s(line); std::string t; std::getline(s, t, ',');
            if (t == "B") { dph_read::HBar b; long long i; char c; s >> i >> c >> b.o >> c >> b.h >> c >> b.l >> c >> b.c; b.end = i; bars.push_back(b); }
            else { dph_read::Row r; char c; s >> r.key >> c >> r.delta >> c >> r.volume; bars.back().rows.push_back(r); }
        }
        std::set<std::pair<int,int>> exp; std::ifstream ex(dir + "/exp_" + m + ".csv");
        while (std::getline(ex, line)) { int k, b; std::sscanf(line.c_str(), "%d,%d", &k, &b); exp.insert({k, b}); }
        std::set<std::pair<int,int>> got; int n = static_cast<int>(bars.size());
        for (int k = 24; k < n - 2; ++k) {
            std::vector<dph_read::HBar> w(bars.begin() + (k - 23), bars.begin() + k + 1);
            dph_read::Decision d = dph_read::sameCandleA(w);
            if (d.ok) got.insert({k, d.bull ? 1 : 0});
        }
        int miss = 0, extra = 0;
        for (auto& e : exp) if (!got.count(e)) ++miss;
        for (auto& g : got) if (!exp.count(g)) ++extra;
        std::printf("%s python %zu c++ %zu missing %d extra %d\n", m, exp.size(), got.size(), miss, extra);
        CHECK(miss == 0 && extra == 0, m);
    }
    // flow / pullback
    std::vector<dph_read::HBar> f(5); for (int i = 0; i < 5; ++i) f[i].rows.push_back({100, i < 4 ? 50.0 : -30.0, 100});
    dph_read::Flow fl = dph_read::flow(f); CHECK(fl.ok && fl.net4 == 120 && fl.net1 == -30, "flow sums");
    CHECK(dph_read::pullback(fl) == 1, "pullback in buying flow");
    f[4].rows[0].delta = 10; CHECK(dph_read::pullback(dph_read::flow(f)) == 0, "with both = no pullback");
    std::printf("checks %d failed %d\n", checks, fails); return fails ? 1 : 0;
}
