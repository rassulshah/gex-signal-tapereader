// t_levelmarks.cpp -- LevelMarks 1.0.0 (lsRevRead 0.19.0): Bk / Sw / Cx with one level, on ES 11-19 (mockup v8) + rule cases
#include "../LevelMarks.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static std::vector<std::string> split(const std::string& s) { std::vector<std::string> v; std::stringstream ss(s); std::string x; while (std::getline(ss, x, '|')) v.push_back(x); return v; }
static std::map<long long, std::string> startOf;
static void load(const char* fn, std::vector<wyr::Bar>& B, std::vector<wyr::Lvl>& K) {
    std::ifstream in(fn); std::string line;
    while (std::getline(in, line)) { auto v = split(line); if (v.empty()) continue;
        if (v[0] == "L" && v.size() >= 3) { wyr::Lvl z; z.px = std::stoll(v[1]); z.name = v[2]; K.push_back(z); }
        if (v[0] == "B" && v.size() >= 9) { wyr::Bar b; b.t = std::stoll(v[1]); b.o = std::stoll(v[2]); b.h = std::stoll(v[3]); b.l = std::stoll(v[4]); b.c = std::stoll(v[5]); b.buy = std::stoll(v[6]); b.sell = std::stoll(v[7]); b.when = v[8]; startOf[b.t] = v[8]; B.push_back(b); } } }
static std::string str(const std::map<long long, lvm::Mark>& M, long long from) { std::string s; for (const auto& kv : M) { if (kv.first < from) continue; s += startOf[kv.first] + " " + lvm::codeText(kv.second.code) + " " + kv.second.lv + "; "; } return s; }
static wyr::Bar bar(long long t, long long o, long long h, long long l, long long c, long long vol) { wyr::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = vol / 2; b.sell = vol - vol / 2; return b; }
int main(int argc, char** argv) {
    std::vector<wyr::Bar> B; std::vector<wyr::Lvl> K; load(argc > 1 ? argv[1] : "es1119.psv", B, K);
    C(B.size() > 300 && K.size() == 18, "fixture loaded");
    const auto M = lvm::run(B, K);
    long long t700 = 0; for (const auto& b : B) if (b.when == "7:00") t700 = b.t;   // the LAST 7:00 = 7:00 AM (the session starts 5 PM)
    const std::string got = str(M, t700);
    const std::string want = "7:06 Bk PFH; 7:33 Bk AHI; 8:27 Cx Op; 8:30 Bk PHH; 8:36 Bk Op; 8:39 Sw PHL; 8:45 Bk PHL; 8:54 Bk LLO; 9:12 Bk PDH; 9:18 Bk ALO; 9:30 Bk ONL; 9:39 Sw PDL; ";
    std::printf("ES 11-19 7:00-9:48: %s\n", got.c_str());
    C(got == want, "ES 11-19 = mockup v8");
    for (const auto& kv : M) if (startOf[kv.first] == "9:39") C(kv.second.dir == 1 && kv.second.px == 29221, "Sw PDL bullish at 7305.25");
    for (const auto& kv : M) if (startOf[kv.first] == "9:12") C(kv.second.dir == -1 && kv.second.px == 29555, "Bk PDH bearish at 7388.75");
    // no repaint: candle-by-candle replay gives the same marks for every candle already decided 6+ candles earlier
    { int bad = 0; for (size_t n = 40; n <= B.size(); n += 7) { std::vector<wyr::Bar> P(B.begin(), B.begin() + n); const auto Q = lvm::run(P, K);
        for (const auto& kv : Q) { if (kv.first > P[P.size() - 7].t) continue; auto it = M.find(kv.first); if (it == M.end() || it->second.code != kv.second.code || it->second.lv != kv.second.lv) ++bad; } }
      C(bad == 0, "replay: a decided mark never changes (" + std::to_string(bad) + ")"); }
    // the I's level (WyRead 1.7.0 Out.ilv): the 7:54 AM I reclaimed the prior hour's low = I / PHL
    { const wyr::Out o = wyr::run(B, 0.25, 2, wyr::Params(), K); std::string got; for (const auto& kv : o.ilv) if (kv.first >= t700) { const auto hb = o.ilvHb.find(kv.first);
        got += startOf[kv.first] + " " + lvm::iLevel(kv.second.second, hb != o.ilvHb.end() ? hb->second : 0, kv.second.second.find("high") != std::string::npos) + "; "; }
      std::printf("I levels: %s\n", got.c_str()); C(got.find("7:51 PHL") != std::string::npos || got.find("7:54 PHL") != std::string::npos, "the 7:54 AM I = I / PHL"); }
    // rule cases (stamps on the clock: 2 AM, 3-min)
    const long long T = 3600LL * 24 * 100 + 2 * 3600;
    auto flat = [&](int n, long long p) { std::vector<wyr::Bar> v; for (int i = 0; i < n; ++i) v.push_back(bar(T + (i + 1) * 180, p, p + 2, p - 2, p, 100)); return v; };
    { auto v = flat(12, 120); std::vector<wyr::Lvl> k(1); k[0].px = 110; k[0].name = "PDL";
      v.push_back(bar(T + 13 * 180, 120, 120, 104, 106, 300));   // closes through PDL
      for (int i = 0; i < 4; ++i) v.push_back(bar(T + (14 + i) * 180, 106, 108, 103, 105, 100));
      auto m = lvm::run(v, k); C(m.count(v[12].t) && m[v[12].t].code == 'B' && m[v[12].t].lv == "PDL" && m[v[12].t].dir == -1, "Bk: a close through PDL"); }
    { auto v = flat(12, 120); std::vector<wyr::Lvl> k(1); k[0].px = 110; k[0].name = "PDL";
      v.push_back(bar(T + 13 * 180, 120, 120, 104, 114, 300));   // trades through, closes back = Sw
      for (int i = 0; i < 4; ++i) v.push_back(bar(T + (14 + i) * 180, 114, 118, 112, 116, 100));
      auto m = lvm::run(v, k); C(m.count(v[12].t) && m[v[12].t].code == 'S' && m[v[12].t].dir == 1, "Sw: a failed break of PDL"); }
    { auto v = flat(12, 120); std::vector<wyr::Lvl> k(1); k[0].px = 110; k[0].name = "PDL";
      v.push_back(bar(T + 13 * 180, 120, 120, 104, 114, 50));    // the sweep without volume
      for (int i = 0; i < 4; ++i) v.push_back(bar(T + (14 + i) * 180, 114, 118, 112, 116, 100));
      auto m = lvm::run(v, k); C(!m.count(v[12].t), "Sw needs the attempt's volume >= the 5 candles before"); }
    { auto v = flat(12, 120); std::vector<wyr::Lvl> k(1); k[0].px = 110; k[0].name = "PDL";
      v[3] = bar(v[3].t, 120, 120, 108, 118, 100);                // PDL already traded through earlier
      v.push_back(bar(T + 13 * 180, 120, 120, 104, 114, 300)); for (int i = 0; i < 4; ++i) v.push_back(bar(T + (14 + i) * 180, 114, 118, 112, 116, 100));
      auto m = lvm::run(v, k); C(!m.count(v[12].t) || m[v[12].t].code != 'S', "Sw only on the first break"); }
    { auto v = flat(12, 120); std::vector<wyr::Lvl> k(2); k[0].px = 110; k[0].name = "VAL"; k[1].px = 111; k[1].name = "ONL";   // priority: session > volume
      v.push_back(bar(T + 13 * 180, 120, 120, 104, 106, 300)); for (int i = 0; i < 4; ++i) v.push_back(bar(T + (14 + i) * 180, 106, 108, 103, 105, 100));
      auto m = lvm::run(v, k); C(m.count(v[12].t) && m[v[12].t].lv == "ONL", "one level: ONL (session) over VAL (volume)"); }
    { auto v = flat(12, 120); std::vector<wyr::Lvl> k(1); k[0].px = 110; k[0].name = "MQ CR";   // a MenthorQ name does not label
      v.push_back(bar(T + 13 * 180, 120, 120, 104, 106, 300)); for (int i = 0; i < 4; ++i) v.push_back(bar(T + (14 + i) * 180, 106, 108, 103, 105, 100));
      auto m = lvm::run(v, k); C(!m.count(v[12].t) || m[v[12].t].lv.empty(), "MenthorQ / gamma names never label a mark"); }
    { auto v = flat(12, 120); std::vector<wyr::Lvl> k(1); k[0].px = 110; k[0].name = "PDL";   // chop: crossed back and forth = one Bk only
      v.push_back(bar(T + 13 * 180, 120, 120, 104, 106, 300)); v.push_back(bar(T + 14 * 180, 106, 116, 105, 114, 300)); v.push_back(bar(T + 15 * 180, 114, 115, 104, 106, 300));
      for (int i = 0; i < 4; ++i) v.push_back(bar(T + (16 + i) * 180, 106, 108, 103, 105, 100));
      auto m = lvm::run(v, k); C(!m.count(v[13].t) && !m.count(v[14].t), "chop: crossing the same level back and forth within 10 candles = no Bk on the re-crosses"); }
    { auto v = flat(12, 120); std::vector<wyr::Lvl> k(1); k[0].px = 110; k[0].name = "PDL";   // repeat cut: a 2nd Bk down after 11 candles without going 1 ATR away = cut
      v.push_back(bar(T + 13 * 180, 120, 120, 104, 106, 300)); v.push_back(bar(T + 14 * 180, 106, 113, 105, 112, 100));
      for (int i = 0; i < 10; ++i) v.push_back(bar(T + (15 + i) * 180, 112, 113, 111, 112, 100));
      v.push_back(bar(T + 25 * 180, 112, 112, 106, 107, 300)); for (int i = 0; i < 3; ++i) v.push_back(bar(T + (26 + i) * 180, 107, 108, 106, 107, 100));
      auto m = lvm::run(v, k); int nb = 0; for (auto& kv : m) if (kv.second.code == 'B' && kv.second.dir == -1) ++nb; C(nb == 1, "repeat cut: the same Bk PDL again without a 1-ATR move away = not shown (" + std::to_string(nb) + ")"); }
    C(lvm::shortName("Open", 0, false) == "Op" && lvm::shortName("LonLO", 0, false) == "LLO" && lvm::shortName("x", 1, true) == "PHH" && lvm::shortName("x", 3, false) == "3hL", "short names");
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", ok, f); return f;
}
