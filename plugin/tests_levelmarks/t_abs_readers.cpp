// t_abs_readers.cpp -- (lsRevRead 0.27.0) the new Abs / Ex lines (stamp|Abs|dir|Low 21%|px|lv, stamp|Ex|dir), made from t_ar_readers.cpp: the new AR lines of the marks file (lsRevRead 0.23.0: stamp|AR|dir||0|strength) change nothing for the readers that do not draw it:
// lsHodLod (SweepVoice: the spoken sweeps; NowLevels: the Now line; TopSignals: the signal line), lsTurn (its sweeps and levels) and the Volume pane.
// Each reader gets his real NQ file of 2026-11-23 with and without AR lines and must return the same thing.
// Build: g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -I.. t_ar_readers.cpp -o t && ./t
#include "../SweepVoice.h"
#include "../NowLevels.h"
#include "../TopSignals.h"
#include "../TurnLogic.h"
#include "../VolRangeLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int f = 0, ok = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m); } }
int main() {
    std::ifstream in("wy_NQ_1123.txt"); std::stringstream ss; ss << in.rdbuf(); const std::string a = ss.str();
    C(a.compare(0, 5, "WY|1|") == 0 && a.find("|Sw|") != std::string::npos && a.find("|Abs|") == std::string::npos, "the real file loaded (it has sweeps, no Abs yet)");
    // AR lines on a Sw candle, a Cx candle, a Bk candle and a plain candle, in the middle and at the end of the file
    std::string b; { std::stringstream s2(a); std::string l; int n = 0; while (std::getline(s2, l)) { b += l + "\n";
        if (l.find("|Sw|") != std::string::npos || l.find("|Cx|") != std::string::npos || l.find("|Bk|") != std::string::npos) { const std::string t = l.substr(0, l.find('|'));
            b += t + "|Abs|1|Low 21%|12345|ALO\n" + t + "|Ex|-1\n" + t + "|Abs|-1||7|1hH\n"; ++n; } }
      b += "3878264460|Abs|-1|Top 100%|99999|PhrH\n3878264640|Ex|-1\n"; C(n >= 20, "Abs / Ex lines added"); }
    const long long E = 2082844800LL, from = 3878200000LL - E, to = 3878270000LL - E;
    { const auto x = svx::parseSw(a, from, to), y = svx::parseSw(b, from, to); bool same = x.size() == y.size() && !x.empty(); for (size_t i = 0; same && i < x.size(); ++i) same = x[i].t == y[i].t && x[i].dir == y[i].dir && x[i].lv == y[i].lv && x[i].st == y[i].st;
      C(same, "the spoken sweeps (SweepVoice): the same sweeps, nothing new to say"); }
    { const auto x = nowl::parseWy(a, 0.25, from, to), y = nowl::parseWy(b, 0.25, from, to); bool same = x.size() == y.size() && !x.empty(); for (size_t i = 0; same && i < x.size(); ++i) same = x[i].t == y[i].t && x[i].code == y[i].code && x[i].dir == y[i].dir && x[i].lv == y[i].lv;
      C(same, "the Now line (NowLevels): the same marks"); }
    { const auto x = tsig::parseWy(a, "NQ", from, to), y = tsig::parseWy(b, "NQ", from, to); bool same = x.size() == y.size(); for (size_t i = 0; same && i < x.size(); ++i) same = x[i].t == y[i].t && x[i].type == y[i].type && x[i].dir == y[i].dir && x[i].lv == y[i].lv;
      C(same, "the signal line (TopSignals): the same signals"); }
    { std::vector<turn::Sw> s1, s2; std::vector<turn::Level> l1, l2; std::string w1, w2; const bool r1 = turn::parseWy(a, "NQ", 180, s1, l1, w1), r2 = turn::parseWy(b, "NQ", 180, s2, l2, w2);
      bool same = r1 && r2 && s1.size() == s2.size() && l1.size() == l2.size() && !s1.empty(); for (size_t i = 0; same && i < s1.size(); ++i) same = s1[i].stamp == s2[i].stamp && s1[i].dir == s2[i].dir && s1[i].lv == s2[i].lv && s1[i].lvt == s2[i].lvt;
      C(same, "the Turn pane: the same sweeps and levels"); }
    { std::map<long long, vrr::Mk> w1, w2; const int k1 = vrr::parseWy(a, w1), k2 = vrr::parseWy(b, w2); bool same = k1 == k2 && w1.size() == w2.size(); for (const auto& kv : w1) { const auto it = w2.find(kv.first); if (it == w2.end() || it->second.code != kv.second.code || it->second.dir != kv.second.dir) same = false; }
      C(same, "the Volume / Range panes' Bk / Sw / Cx: the same"); }
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", ok, f); return f;
}
