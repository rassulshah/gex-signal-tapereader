// test_t10.cpp - Top10Stable 1.0.0 unit checks + the ES 10-21 RTH book replay (flicker before vs after)
#include "../Top10Stable.h"
#include <cstdio>
#include <map>
#include <fstream>
#include <sstream>
#include <string>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", m); } } while (0)
static t10::In I(long long k, long long lots, bool on = true) { return t10::In{ k, (double)k, lots, true, on }; }
int main(int argc, char** argv) {
    { t10::Board b; std::vector<t10::In> bk = { I(1, 100) };
      CHECK(b.update(0, bk).empty(), "not shown before 30 s");
      CHECK(b.update(29999, bk).empty(), "still not at 29.999 s");
      auto o = b.update(30000, bk); CHECK(o.size() == 1 && o[0].rank == 1, "enters at 30 s"); }
    { t10::Board b; std::vector<t10::In> bk; for (int k = 1; k <= 15; ++k) bk.push_back(I(k, 100 + k));
      b.update(0, bk); auto o = b.update(30000, bk); CHECK(o.size() == 10, "never more than 10");
      CHECK(o.size() == 10 && o[0].key == 15 && o[9].key == 6, "first fill = the 10 biggest in order"); }
    { t10::Board b; std::vector<t10::In> bk; for (int k = 1; k <= 10; ++k) bk.push_back(I(k, 100));
      b.update(0, bk); b.update(30000, bk); bk.push_back(I(99, 115)); b.update(30000, bk);
      auto o = b.update(60000, bk); bool in99 = false; for (auto& x : o) in99 |= x.key == 99; CHECK(!in99, "115 vs 100 (1.15x) does not enter (needs 1.20x)");
      bk.back().lots = 125; o = b.update(61000, bk); in99 = false; for (auto& x : o) in99 |= x.key == 99; CHECK(in99 && o.size() == 10, "125 (1.25x) replaces the weakest, still 10"); }
    { t10::Board b; std::vector<t10::In> bk = { I(1, 100), I(2, 90) };
      b.update(0, bk); auto o = b.update(30000, bk); CHECK(o.size() == 2 && o[0].key == 1, "rank 1 = 100");
      bk[1].lots = 115; o = b.update(31000, bk); o = b.update(200000, bk); CHECK(!o.empty() && o[0].key == 1, "115 vs 100 (1.15x) no swap");
      bk[1].lots = 130; o = b.update(201000, bk); CHECK(!o.empty() && o[0].key == 1, "a 1 s jump to 130 does not swap yet (smoothed)");
      o = b.update(300000, bk); CHECK(!o.empty() && o[0].key == 2, "130 held (1.30x) swaps"); }
    { t10::Board b; std::vector<t10::In> bk = { I(1, 100), I(2, 90) }; b.update(0, bk); b.update(30000, bk);
      std::vector<t10::In> gone = { I(2, 90) };
      auto o = b.update(35000, gone); CHECK(o.size() == 2, "pulled order kept in its 10 s grace");
      o = b.update(40001, gone); CHECK(o.size() == 1 && o[0].key == 2, "gone after the grace, #2 becomes #1"); }
    { t10::Board b; std::vector<t10::In> bk = { I(1, 100), I(2, 90, false) };
      b.update(0, bk); auto o = b.update(30000, bk); CHECK(o.size() == 1, "off-screen orders are not members"); }
    { t10::Board b; std::vector<t10::In> bk; for (int s = 0; s < 3000; ++s) { bk.clear(); for (int k = 0; k < 20; ++k) bk.push_back(I(s * 7 + k, 50 + k)); b.update((long long)s * 1000, bk); }
      CHECK(b.tracked() <= 400, "bounded tracking"); }
    if (argc > 1) {   // replay: ms,key,px,lots,bid
        std::ifstream f(argv[1]); std::string line; std::getline(f, line);
        std::map<long long, std::vector<t10::In>> snaps;
        while (std::getline(f, line)) { std::stringstream ss(line); std::string a, b, c, d, e;
            std::getline(ss, a, ','); std::getline(ss, b, ','); std::getline(ss, c, ','); std::getline(ss, d, ','); std::getline(ss, e, ',');
            snaps[std::stoll(a)].push_back(t10::In{ std::stoll(b), std::stod(c), std::stoll(d), e == "1", true }); }
        t10::Board st; std::map<long long, int> prevOld, prevNew; long long invOld = 0, invNew = 0, chOld = 0, chNew = 0, setOld = 0, setNew = 0; long long first = -1, lastMs = 0; size_t maxNew = 0;
        for (auto& kv : snaps) {
            if (first < 0) first = kv.first; lastMs = kv.first;
            std::vector<t10::In> v = kv.second; std::sort(v.begin(), v.end(), [](const t10::In& a, const t10::In& b) { return a.lots > b.lots; });
            std::map<long long, int> old; for (size_t k = 0; k < v.size() && k < 10; ++k) old[v[k].key] = (int)k + 1;
            std::map<long long, int> nw; auto o = st.update(kv.first, kv.second); maxNew = std::max(maxNew, o.size()); for (auto& x : o) nw[x.key] = x.rank;
            for (auto& p : old) { auto it = prevOld.find(p.first); if (it == prevOld.end()) ++setOld; else if (it->second != p.second) ++chOld; }
            for (auto& p : prevOld) if (!old.count(p.first)) ++setOld;
            for (auto& p : nw) { auto it = prevNew.find(p.first); if (it == prevNew.end()) ++setNew; else if (it->second != p.second) ++chNew; }
            for (auto& p : prevNew) if (!nw.count(p.first)) ++setNew;
            auto inv = [](const std::map<long long,int>& a, const std::map<long long,int>& b) { long long n = 0;
                for (auto& x : a) for (auto& y : a) if (x.first < y.first && b.count(x.first) && b.count(y.first) && ((x.second < y.second) != (b.at(x.first) < b.at(y.first)))) ++n; return n; };
            invOld += inv(prevOld, old); invNew += inv(prevNew, nw);
            prevOld = old; prevNew = nw; }
        const double mins = (lastMs - first) / 60000.0;
        std::printf("REPLAY %zu snapshots, %.0f min: BEFORE rank changes %.1f/min, rows in/out %.1f/min, re-orders %.1f/min | AFTER rank changes %.1f/min, rows in/out %.1f/min, re-orders %.1f/min, max rows %zu\n",
            snaps.size(), mins, chOld / mins, setOld / mins, invOld / mins, chNew / mins, setNew / mins, invNew / mins, maxNew);
        CHECK(maxNew <= 10, "replay: never more than 10"); CHECK(chNew + setNew < chOld + setOld, "replay: fewer changes after");
    }
    std::printf("test_t10: %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
