// test_exhaustion_logic.cpp -- g++ -std=c++17 -Wall -Wextra -I. test_exhaustion_logic.cpp -o t && ./t [3m.csv]
#include "ExhaustionLogic.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <map>
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

    // (0.3.0) the grind: one burst, then 1-tick new highs on quiet volume - the 0.2 read crosses 3x with nobody pushing; the gate does not
    {
        std::vector<double> H, L, B, S2;
        for (int i = 0; i < 40; ++i) { H.push_back(100 + 0.25 * (i % 2)); L.push_back(99); B.push_back(300); S2.push_back(300); }   // normal pace 300
        H.push_back(102); L.push_back(100); B.push_back(1000); S2.push_back(300);                                                 // the burst (bar 40)
        for (int i = 0; i < 20; ++i) { H.push_back(102 + 0.25 * (i + 1) * (i % 3 == 2)); L.push_back(101.5); B.push_back(150); S2.push_back(150); }
        // make every 3rd bar a 1-tick new high: rebuild highs as a slow staircase
        for (int i = 0; i < 20; ++i) H[41 + i] = 102 + 0.25 * ((i + 1) / 3) - (i % 3 == 2 ? 0.0 : 0.5);   // a 1-tick new high every 3rd bar, dips between
        Params A; A.N = 5; A.S = 2; A.T = 3.0;                 // the 0.2 read (K 0, P 0)
        Out a = compute(H, L, B, S2, A); int ea = 0; for (size_t i = 45; i < a.eTop.size(); ++i) ea += a.eTop[i];
        Params G = A; G.K = 3; G.P = 1.0;                      // recent (3 bars) and real (>= normal)
        Out g = compute(H, L, B, S2, G); int eg = 0; for (size_t i = 45; i < g.eTop.size(); ++i) eg += g.eTop[i];
        CHECK(ea >= 1, "the 0.2 read fires in the grind 5+ bars after the burst (the bug he saw on ES / GC)");
        CHECK(eg == 0, "the push gate stays quiet in the grind once the burst is more than 3 bars back (bars 45+)");
        bool quiet = true; for (int i = 45; i < 61; ++i) if (g.fb[i] != 1.0) quiet = false;
        CHECK(quiet, "gated bars read 1.0, not a fade");
        Params Pz = G; Pz.P = 5.0;                              // a peak must be 5x normal: the burst (1000 / 300 = 3.3x) is not
        Out z = compute(H, L, B, S2, Pz); CHECK(z.fb[40] == 1.0 && z.newH[40], "P gate: a peak under P x normal pace reads 1.0 at a new high");
        Params Pn = G; Pn.P = 1.0;
        Out y = compute(std::vector<double>(H.begin(), H.begin() + 9), std::vector<double>(L.begin(), L.begin() + 9), std::vector<double>(B.begin(), B.begin() + 9), std::vector<double>(S2.begin(), S2.begin() + 9), Pn);
        bool none = true; for (double v : y.fb) if (v != 1.0) none = false;
        CHECK(none, "fewer than 10 bars of normal pace: no gated fade");
        Params sd = A; sd.sides = 2; Out w = compute(H, L, B, S2, sd); int ex_ = 0, xt = 0; for (size_t i = 0; i < w.eTop.size(); ++i) { ex_ += w.eTop[i]; xt += w.xTop[i]; }
        int xa = 0; for (char e : a.xTop) xa += e;
        CHECK(ex_ == 0 && xt == xa, "SIDES 2 (bottoms): tops still marked as first crossings (x) but no top E");
        CHECK(std::fabs(median({1, 5, 3, 9}, 0, 4) - 4.0) < 1e-12 && median({2, 7, 4}, 0, 3) == 4.0, "median like pandas (even: mean of the middle two)");
    }
    // (0.3.0) parity with lra.exh_study.fades2 on his bars: argv[2] = the python dump (i|K|P|fb|fs per line)
    if (argc > 2) {
        std::ifstream in(argv[1]); std::string line; std::getline(in, line); std::vector<double> H, L, B, S2;
        while (std::getline(in, line)) { std::stringstream ss(line); std::string f; std::vector<std::string> c; while (std::getline(ss, f, ',')) c.push_back(f);
            if (c.size() < 8) continue;
            H.push_back(atof(c[2].c_str())); L.push_back(atof(c[3].c_str())); B.push_back(atof(c[6].c_str())); S2.push_back(atof(c[7].c_str())); }
        std::ifstream py(argv[2]); int bad = 0, n = 0; std::map<std::pair<int, double>, Out> memo;
        while (std::getline(py, line)) { std::stringstream ss(line); std::string f; std::vector<std::string> c; while (std::getline(ss, f, '|')) c.push_back(f);
            if (c.size() < 5) continue;
            const int i = atoi(c[0].c_str()), K = atoi(c[1].c_str()); const double Pp = atof(c[2].c_str());
            auto key = std::make_pair(K, Pp);
            if (!memo.count(key)) { Params q = defaults("ES"); q.K = K; q.P = Pp; memo[key] = compute(H, L, B, S2, q); }
            const Out& o = memo[key]; ++n;
            if (std::fabs(o.fb[i] - atof(c[3].c_str())) > 1e-6 || std::fabs(o.fs[i] - atof(c[4].c_str())) > 1e-6) { if (bad++ < 5) std::printf("  mismatch bar %d K%d P%.1f: c++ %.6f %.6f py %s %s\n", i, K, Pp, o.fb[i], o.fs[i], c[3].c_str(), c[4].c_str()); }
        }
        CHECK(n > 1000 && bad == 0, "C++ = python (exh_study.fades2) on every bar of his ES file, every gate");
        std::printf("parity: %d values, %d mismatches\n", n, bad);
    }
    // replay a stored 3m file (t_ct,o,h,l,c,v,bought,sold,...) and count E's
    if (argc == 2) {
        std::ifstream in(argv[1]); std::string line; std::getline(in, line); std::vector<double> H, L, B, S2;
        while (std::getline(in, line)) { std::stringstream ss(line); std::string f; std::vector<std::string> c; while (std::getline(ss, f, ',')) c.push_back(f);
            if (c.size() < 8) continue;
            H.push_back(atof(c[2].c_str())); L.push_back(atof(c[3].c_str())); B.push_back(atof(c[6].c_str())); S2.push_back(atof(c[7].c_str())); }
        Params E = defaults("ES"); Out r = compute(H, L, B, S2, E); int t = 0, bt = 0; for (size_t i = 0; i < H.size(); ++i) { t += r.eTop[i]; bt += r.eBot[i]; }
        std::printf("replay %zu bars: %d top E, %d bottom E\n", H.size(), t, bt);
    }
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
