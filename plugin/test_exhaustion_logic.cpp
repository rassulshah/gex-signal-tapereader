// test_exhaustion_logic.cpp -- g++ -std=c++17 -Wall -Wextra -I. test_exhaustion_logic.cpp -o t && ./t [3m.csv]
#include "ExhaustionLogic.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
using namespace ex;
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
int main(int argc, char** argv) {
    // a push up: 10 bars of rising highs, buyers 100 -> 20 per bar
    std::vector<double> h, l, b, s;
    for (int i = 0; i < 25; ++i) { h.push_back(100 + 0.1 * (i % 2)); l.push_back(99); b.push_back(50); s.push_back(50); }   // quiet
    for (int i = 0; i < 10; ++i) { h.push_back(101 + i); l.push_back(100 + i); b.push_back(100 - 8 * i); s.push_back(30); }
    Params P; P.N = 5; P.S = 1; P.T = 2.0;
    Out o = compute(h, l, b, s, P);
    int first = -1; for (int i = 0; i < (int)h.size(); ++i) if (o.eTop[i]) { first = i; break; }
    // buyers 100, 92, ..., 28: 100/x >= 2 first at x <= 50 -> 100-8i <= 50 -> i = 7 -> bar 32
    CHECK(first == 32, "E at the first new high where buyers trade at half their peak pace");
    int count = 0; for (char e : o.eTop) count += e; CHECK(count == 1, "one E per run (the next bars over the line are not new E's)");
    CHECK(std::fabs(o.fb[34] - 100.0 / 28) < 1e-9, "the fade = peak / now");
    CHECK(o.fb[10] == 1.0 && !o.eBot[33], "quiet bars stay 1.0, no bottom E in a push up");
    // the mirror: a push down with fading sellers
    std::vector<double> h2, l2, b2, s2;
    for (int i = 0; i < 10; ++i) { h2.push_back(100); l2.push_back(99.9 + 0.1 * (i % 2)); b2.push_back(40); s2.push_back(40); }
    for (int i = 0; i < 8; ++i) { h2.push_back(99 - i); l2.push_back(98 - i); b2.push_back(10); s2.push_back(i < 2 ? 90 : 30); }
    Out q = compute(h2, l2, b2, s2, P);
    int bot = -1; for (int i = 0; i < (int)h2.size(); ++i) if (q.eBot[i]) { bot = i; break; }
    CHECK(bot == 12, "sellers 90 then 30 at a new low: 3.0x -> E at the bottom");
    CHECK(compute({}, {}, {}, {}, P).fb.empty(), "no bars: nothing");
    // replay a stored 3m file (t_ct,o,h,l,c,v,bought,sold,...) and count E's
    if (argc > 1) {
        std::ifstream in(argv[1]); std::string line; std::getline(in, line); std::vector<double> H, L, B, S2;
        while (std::getline(in, line)) { std::stringstream ss(line); std::string f; std::vector<std::string> c; while (std::getline(ss, f, ',')) c.push_back(f);
            if (c.size() < 8) continue; H.push_back(atof(c[2].c_str())); L.push_back(atof(c[3].c_str())); B.push_back(atof(c[6].c_str())); S2.push_back(atof(c[7].c_str())); }
        Params E = defaults("ES"); Out r = compute(H, L, B, S2, E); int t = 0, bt = 0; for (size_t i = 0; i < H.size(); ++i) { t += r.eTop[i]; bt += r.eBot[i]; }
        std::printf("replay %zu bars: %d top E, %d bottom E\n", H.size(), t, bt);
    }
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
