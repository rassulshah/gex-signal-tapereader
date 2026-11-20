// t_levelmarks.cpp -- LevelMarks 1.6.0 (2026-11-20: Sw needs BVMax / BRMax >= 20) / 1.3.0 (lsRevRead 0.20.0: FT / Tst / Rev, Bk candle rule, Sw colour, no POC / Open) / 1.2.0 (lsRevRead 0.19.2; 1.0.0 / 0.19.0 first): Bk / Sw / Cx with one level, on ES 11-19 (mockup v8) + rule cases
#include "../LevelMarks.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <set>
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
    const std::string want = "7:09 Sw PHH; 7:33 Bk AHI; 8:27 Cx AHI; 8:30 Bk PHH; 8:33 Sw PHH; 8:39 Sw PHL; 8:45 Bk PHL; 9:12 Bk PDH; 9:18 Bk ALO; 9:30 Bk ONL; 9:39 Sw PDL; ";   // (1.3.0) no Open / POC levels; the BO candle rule drops the small 7:06 / 8:54 breaks
    std::printf("ES 11-19 7:00-9:48: %s\n", got.c_str());
    C(got == want, "ES 11-19 = mockup v8 + the 1.3.0 rules (1.8.0: + 7:09 / 8:33 Sw PHH, no volume gate in run(); the own-candle 20 test is fillStrength)");
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
    // (1.1.0) levelNear: the Rly at the 9:39 AM low (7293.00) - PDL 7305.25 is inside the candle -> "PDL"
    { int i = -1; for (size_t q = 0; q < B.size(); ++q) if (startOf[B[q].t] == "9:39" && B[q].t > t700) i = (int)q;
      std::string sn; long long px = 0; C(i >= 0 && lvm::levelNear(B, i, K, B[(size_t)i].l, 8, sn, px) && sn == "PDL" && px == 29221, "levelNear: the 9:39 AM low = PDL (" + sn + ")");
      int j = -1; for (size_t q = 0; q < B.size(); ++q) if (startOf[B[q].t] == "8:00" && B[q].t > t700) j = (int)q;
      std::string s2; long long p2 = 0; const bool any = lvm::levelNear(B, j, std::vector<wyr::Lvl>(), B[(size_t)j].h, 0, s2, p2); C(!any || !s2.empty(), "levelNear: hour levels only still gives a short name or nothing"); }
    // rule cases (stamps on the clock: 2 AM, 3-min)
    const long long T = 3600LL * 24 * 100 + 2 * 3600;
    auto flat = [&](int n, long long p) { std::vector<wyr::Bar> v; for (int i = 0; i < n; ++i) v.push_back(bar(T + (i + 1) * 180, p, p + 2, p - 2, p, 100)); return v; };
    { auto v = flat(12, 120); std::vector<wyr::Lvl> k(1); k[0].px = 110; k[0].name = "PDL";
      v.push_back(bar(T + 13 * 180, 120, 120, 104, 106, 300));   // closes through PDL
      for (int i = 0; i < 4; ++i) v.push_back(bar(T + (14 + i) * 180, 106, 108, 103, 105, 100));
      auto m = lvm::run(v, k); C(m.count(v[12].t) && m[v[12].t].code == 'B' && m[v[12].t].lv == "PDL" && m[v[12].t].dir == -1, "Bk: a close through PDL"); }
    { auto v = flat(12, 120); std::vector<wyr::Lvl> k(1); k[0].px = 110; k[0].name = "PDL";
      v.push_back(bar(T + 13 * 180, 106, 120, 104, 114, 300));   // trades through, closes back = Sw
      for (int i = 0; i < 4; ++i) v.push_back(bar(T + (14 + i) * 180, 114, 118, 112, 116, 100));
      auto m = lvm::run(v, k); C(m.count(v[12].t) && m[v[12].t].code == 'S' && m[v[12].t].dir == 1, "Sw: a failed break of PDL"); }
    { auto v = flat(12, 120); std::vector<wyr::Lvl> k(1); k[0].px = 110; k[0].name = "PDL";
      v.push_back(bar(T + 13 * 180, 106, 120, 104, 114, 50));    // the sweep without volume
      for (int i = 0; i < 4; ++i) v.push_back(bar(T + (14 + i) * 180, 114, 118, 112, 116, 100));
      auto m = lvm::run(v, k); C(m.count(v[12].t) && m[v[12].t].code == 'S', "(1.8.0) no volume gate: a sweep on low volume is still a Sw in run() (fillStrength's own-candle 20 judges it)"); }
    { auto v = flat(12, 120); std::vector<wyr::Lvl> k(1); k[0].px = 110; k[0].name = "PDL";
      v[3] = bar(v[3].t, 120, 120, 108, 118, 100);                // PDL already traded through earlier
      v.push_back(bar(T + 13 * 180, 106, 120, 104, 114, 300)); for (int i = 0; i < 4; ++i) v.push_back(bar(T + (14 + i) * 180, 114, 118, 112, 116, 100));
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
    // (1.2.0) the strength line = his BVMax / BRMax over the chart's candles (the 1,500 before the session + the session), as the panes count it
    {   std::vector<long long> T; std::vector<double> V, R; std::vector<char> G;
        std::ifstream in(argc > 2 ? argv[2] : "es1119hist.psv"); std::string line;
        while (std::getline(in, line)) { auto v = split(line); if (v.size() != 7 || v[0] != "H") continue; T.push_back(std::stoll(v[1]));
            const long long o = std::stoll(v[2]), h = std::stoll(v[3]), l = std::stoll(v[4]), c = std::stoll(v[5]); V.push_back(std::stod(v[6])); R.push_back(lvm::rangeTicks((double)h, (double)l, 1.0)); G.push_back(c >= o ? 1 : 0); }
        C(T.size() > 1500, "history fixture loaded");
        auto M2 = lvm::run(B, K); lvm::fillStrength(M2, T, V, R, G);
        auto stOf = [&](const std::string& when, char code) { for (const auto& kv : M2) if (startOf[kv.first] == when && kv.second.code == code) return kv.second.lv + "/" + kv.second.st; return std::string("none"); };
        C(stOf("7:33", 'B') == "AHI/R99+", "7:33 Bk AHI R99+ (got " + stOf("7:33", 'B') + ")");
        C(stOf("8:30", 'B') == "PHH/V99+", "8:30 Bk PHH V99+ (got " + stOf("8:30", 'B') + ")");
        C(stOf("8:36", 'B') == "none", "(1.3.0) no Bk at the open - Open is not a level (got " + stOf("8:36", 'B') + ")");
        C(stOf("7:06", 'B') == "none" && stOf("8:54", 'B') == "none", "(1.3.0) the BO candle rule drops the 7:06 / 8:54 small breaks");
        C(stOf("8:39", 'S') == "PHL/V99+", "8:39 Sw PHL V99+ (got " + stOf("8:39", 'S') + ")");
        C(stOf("9:30", 'B') == "ONL/V99+", "9:30 Bk ONL V99+ (got " + stOf("9:30", 'B') + ")");
        C(stOf("9:39", 'S') == "PDL/V99+", "9:39 Sw PDL V99+ (got " + stOf("9:39", 'S') + ")");
        for (const char* w : { "8:45", "9:12", "9:18" }) C(stOf(w, 'B').size() > 0 && stOf(w, 'B').back() == '/', std::string("no strength line on the ") + w + " Bk (got " + stOf(w, 'B') + ")");
        { long long t754 = 0; for (const auto& kv : startOf) if (kv.second == "7:54") t754 = kv.first; const std::string si = lvm::strengthAt(T, V, R, G, t754);
          std::printf("I 7:54 strength: '%s'\n", si.c_str()); C(t754 > 0, "the 7:54 I candle is in the fixture"); }
        int cx = 0; for (const auto& kv : M2) if (kv.second.code == 'C' && !kv.second.st.empty()) ++cx; C(cx == 0, "Cx carries no strength line"); }
    {   // stCount rules: consecutive count stops at the first candle as big or bigger; same-colour count skips the other colour; caps at 99; under 20 = no line
        std::vector<double> V(30, 10.0); std::vector<char> G(30, 1); V[29] = 50; C(lvm::stCount(V, G, 29) == 29, "consecutive: biggest in 29");
        V[20] = 60; C(lvm::stCount(V, G, 29) == 8, "stops at the first candle as big or bigger");
        std::vector<char> G2(30, 0); for (int i = 0; i < 30; i += 3) G2[(size_t)i] = 1; std::vector<double> V2(30, 10.0); V2[29] = 50; V2[28] = 60; G2[29] = 1; G2[28] = 0;
        C(lvm::stCount(V2, G2, 29) == 10, "same colour: the bigger red candle before does not stop the green count (10 green candles before) (got " + std::to_string(lvm::stCount(V2, G2, 29)) + ")");
        std::vector<double> V3(400, 5.0); std::vector<char> G3(400, 1); V3[399] = 9; C(lvm::stCount(V3, G3, 399) == 99, "capped at 99");
        std::vector<double> R3(400, 5.0); R3[399] = 9; C(lvm::strength(V3, R3, G3, 399, 399) == "V99+", "a tie: volume wins");
        std::vector<double> V4(30, 10.0); V4[29] = 11; V4[10] = 20; C(lvm::strength(V4, std::vector<double>(30, 0.0), std::vector<char>(30, 1), 29, 29) == "V18", "(1.4.0, Rassul 18:19 \"switch to 10\") 18 = shown (from 10 up)");
        { std::vector<double> V5(30, 10.0); V5[29] = 11; V5[20] = 20; C(lvm::strength(V5, std::vector<double>(30, 0.0), std::vector<char>(30, 1), 29, 29).empty(), "(1.4.0) under 10 = no line"); }
        std::vector<double> V5(60, 10.0); V5[58] = 30; V5[59] = 5; std::vector<char> G5(60, 1); C(lvm::strength(V5, std::vector<double>(60, 0.0), G5, 57, 59) == "V58", "a Sw: the max over its attempt's candles (got " + lvm::strength(V5, std::vector<double>(60, 0.0), G5, 57, 59) + ")"); }

    {   // (1.5.0, Rassul 18:29 CT "the BRmax or BVmax should be at least 10") a Sw needs its attempt's BVMax or BRMax >= 10, else it is removed; a Bk is never removed
        std::vector<long long> T; std::vector<double> V, R; std::vector<char> G; for (int i = 0; i < 40; ++i) { T.push_back(1000 + i * 180); V.push_back(10); R.push_back(4); G.push_back(1); }
        V[39] = 11; V[30] = 20;            // candle 39: biggest in 8 only
        R[25] = 9; R[15] = 20;             // candle 25: range biggest in 9 only; volume 10 = not bigger
        V[35] = 30;                        // candle 35: biggest volume in 35 (> 10)
        std::map<long long, lvm::Mark> M; lvm::Mark sw; sw.code = 'S'; sw.dir = 1;
        M[T[39]] = sw; M[T[25]] = sw; M[T[35]] = sw; lvm::Mark bk; bk.code = 'B'; bk.dir = 1; M[T[24]] = bk;
        lvm::Mark at; at = sw; at.t0 = T[34]; M[T[36]] = at;   // an attempt 34..36: candle 35 inside it counts
        lvm::fillStrength(M, T, V, R, G);
        C(!M.count(T[39]), "(1.5.0) Sw with BVMax 8 is removed");
        C(!M.count(T[25]), "(1.5.0) Sw with BRMax 9 is removed");
        C(M.count(T[35]) && M.at(T[35]).st == "V35", "(1.5.0) Sw with BVMax 35 stays, V35");
        C(!M.count(T[36]), "(1.8.0) the attempt's max no longer counts: the Sw candle 36 itself is weak (34..36 holds the V35 candle) -> removed");
        C(M.count(T[24]), "(1.5.0) a weak Bk is never removed by the Sw rule");
    }
    {   // (1.6.0, Rassul 2026-11-20 07:50 CT "update the sweep threshold to 20") a Sw needs BVMax or BRMax >= 20; the strength line still shows from 10
        // one fixture per case: 60 flat candles (V 10, R 4), candle 59 = the Sw / Bk; a bigger candle k back sets its count to k - 1
        auto one = [](char code, int volBack, int rngBack, std::string& st) {
            std::vector<long long> T; std::vector<double> V(60, 10.0), R(60, 4.0); std::vector<char> G(60, 1); for (int i = 0; i < 60; ++i) T.push_back(9000 + i * 180);
            V[59] = 11; V[(size_t)(59 - volBack)] = 50; if (rngBack > 0) { R[59] = 5; R[(size_t)(59 - rngBack)] = 50; }
            std::map<long long, lvm::Mark> M; lvm::Mark m; m.code = code; m.dir = 1; M[T[59]] = m;
            lvm::fillStrength(M, T, V, R, G); st = M.count(T[59]) ? M.at(T[59]).st : std::string("gone"); return (int)M.size(); };
        std::string st;
        C(lvm::SW_MIN == 20 && lvm::ST_MIN == 10, "(1.6.0) SW_MIN 20, ST_MIN still 10");
        C(one('S', 16, 0, st) == 0, "(1.6.0) a Sw with BVMax 15 is no longer a Sw (got " + st + ")");
        C(one('S', 20, 0, st) == 0, "(1.6.0) a Sw with BVMax 19 is no longer a Sw (got " + st + ")");
        C(one('S', 10, 19, st) == 0, "(1.6.0) a Sw with BRMax 18 is no longer a Sw (got " + st + ")");
        C(one('S', 21, 0, st) == 1 && st == "V20", "(1.6.0) a Sw with BVMax 20 stays, V20 (got " + st + ")");
        C(one('S', 5, 26, st) == 1 && st == "R25", "(1.6.0) a Sw with BRMax 25 stays, R25 (got " + st + ")");
        C(one('B', 16, 0, st) == 1 && st == "V15", "(1.6.0) a Bk with max 15 keeps its V15 line (got " + st + ")");
        C(one('B', 3, 0, st) == 1 && st.empty(), "(1.6.0) a weak Bk is never removed");
    }
    {   // (1.8.0, Rassul 2026-11-20 10:35 CT "the red sweep bar doesnt meet the threshold.. fix") HG 11-20 shape: a big GREEN candle (99+) then the
        // weak RED Sw candle in the same attempt - the Sw is judged on its own candle and is removed; a strong red Sw candle stays with ITS own count
        std::vector<long long> T; std::vector<double> V(120, 10.0), R(120, 4.0); std::vector<char> G(120, 1); for (int i = 0; i < 120; ++i) T.push_back(50000 + i * 180);
        V[118] = 500; R[118] = 40; G[118] = 1;     // the green 99+ candle
        V[119] = 6;   R[119] = 3;  G[119] = 0;     // the red Sw candle: small
        std::map<long long, lvm::Mark> M; lvm::Mark sw; sw.code = 'S'; sw.dir = -1; sw.t0 = T[118]; M[T[119]] = sw;
        lvm::fillStrength(M, T, V, R, G);
        C(!M.count(T[119]), "(1.8.0) HG shape: weak red Sw after a 99+ green candle is removed (was V99+ from the attempt)");
        V[119] = 40; R[119] = 3; for (int i = 90; i < 118; ++i) G[(size_t)i] = 0; std::map<long long, lvm::Mark> M2; M2[T[119]] = sw; lvm::fillStrength(M2, T, V, R, G);
        // red candle volume 40: bigger than the 1 candle before? no (500) -> all = 0; same colour (red) none before -> counts back to the cap/look
        C(M2.count(T[119]) && M2.at(T[119]).st.size() > 1 && M2.at(T[119]).st[0] == 'V', "(1.8.0) a strong red Sw candle stays with its own line (got " + (M2.count(T[119]) ? M2.at(T[119]).st : std::string("gone")) + ")");
        // iKey / I_MIN
        C(lvm::I_MIN == 20, "(1.8.0) I_MIN 20");
        C(lvm::iKey(lvm::pri("ONL", 0)) && lvm::iKey(lvm::pri("PDL", 0)) && lvm::iKey(lvm::pri("PWH", 0)) && lvm::iKey(lvm::pri("VAL", 0)), "(1.8.0) ON / PD / weekly / VA name an I");
        C(!lvm::iKey(lvm::pri("PHL", 0)) && !lvm::iKey(lvm::pri("x hour low", 1)) && !lvm::iKey(lvm::pri("x hour low", 2)) && !lvm::iKey(lvm::pri("x", 3)) && !lvm::iKey(lvm::pri("HVL", 0)), "(1.8.0) PHL / 2hL / 3hL / gamma never name an I");
        std::vector<wyr::Lvl> c(2); c[0].px = 100; c[0].name = "x hour low"; c[0].hb = 2; c[1].px = 99; c[1].name = "x hour low"; c[1].hb = 1; wyr::Lvl z;
        C(!lvm::iPick(c, 1, z), "(1.8.0) iPick: only hour levels -> no level (no I)");
        c.push_back(wyr::Lvl()); c[2].px = 98; c[2].name = "ONL"; C(lvm::iPick(c, 1, z) && z.name == "ONL", "(1.8.0) iPick: ONL with hour levels -> ONL");
    }
    // ---- (1.3.0) Bk -> FT -> Tst -> Rev: parity with the agreed mockup (price-pane-cleanup-mockup.html, mk10.py) on 11-19 ES / NQ 8:33 AM - 12:33 PM
    for (const char* mk : { "es", "nq" }) {
        std::vector<wyr::Bar> FB; std::vector<wyr::Lvl> FK; startOf.clear(); load((std::string(mk) + "1119full.psv").c_str(), FB, FK);
        const auto FM = lvm::run(FB, FK); const auto FS = lvm::seq(FB, FM);
        std::set<std::string> want, got; long long w0 = 0, w1 = 0;
        { std::ifstream in((std::string("mock_") + mk + ".psv").c_str()); std::string line; while (std::getline(in, line)) { auto v = split(line);
            if (v.size() >= 3 && v[0] == "W") { w0 = std::stoll(v[1]); w1 = std::stoll(v[2]); }
            if (v.size() >= 4 && v[0] == "E") want.insert(v[1] + "|" + v[2] + "|" + v[3] + (v[2] == "Tst" && v.size() >= 6 ? "|" + v[4] + "|" + v[5] : "")); } }
        for (const auto& kv : FS) { if (kv.first < w0 || kv.first > w1) continue; for (const auto& m : kv.second) got.insert(std::to_string(kv.first) + "|" + lvm::seqText(m.code) + "|" + std::to_string(m.dir) + (m.code == 'T' ? "|" + m.lv + "|" + std::to_string(m.pct) : "")); }
        std::string miss, extra; for (const auto& w : want) if (!got.count(w)) miss += w + " "; for (const auto& g : got) if (!want.count(g)) extra += g + " ";
        std::printf("%s seq: %zu marks, mockup-only [%s] plugin-only [%s]\n", mk, got.size(), miss.c_str(), extra.c_str());
        C(!want.empty() && miss.empty(), std::string(mk) + ": every mockup FT / Tst / Rev is drawn");
        // NQ: the 10:54 AM candle is no longer a Sw (red candle) - it is a Bk VAL (it closed below VAL), whose FT is the 11:03 AM candle
        C(std::string(mk) == "es" ? extra.empty() : extra == "1795086180|FT|-1 ", std::string(mk) + ": nothing beyond the mockup but the NQ 10:54 Bk VAL's FT");
        // replay: a decided FT / Tst / Rev never changes
        int bad = 0; for (size_t n = 60; n <= FB.size(); n += 9) { std::vector<wyr::Bar> P(FB.begin(), FB.begin() + n); const auto Q = lvm::seq(P, lvm::run(P, FK));
            for (const auto& kv : Q) { if (kv.first > P[P.size() - 16].t) continue; const auto it = FS.find(kv.first); if (it == FS.end() || it->second.size() != kv.second.size()) { ++bad; continue; }
                for (size_t q = 0; q < kv.second.size(); ++q) if (it->second[q].code != kv.second[q].code || it->second[q].pct != kv.second[q].pct) ++bad; } }
        C(bad == 0, std::string(mk) + " replay: a decided FT / Tst / Rev never changes (" + std::to_string(bad) + ")");
    }
    {   // rule cases: a bearish Bk of PDL 110, its FT, the move to 90, a pullback to 106 (overlaps the Bk candle) = Tst 80%, then a Rev
        const long long T2 = 3600LL * 24 * 200 + 2 * 3600; int q = 0; std::vector<wyr::Bar> v; auto B2 = [&](long long o, long long h, long long l, long long c, long long vol) { v.push_back(bar(T2 + (++q) * 180, o, h, l, c, vol)); };
        for (int i = 0; i < 10; ++i) B2(110 + i, 112 + i, 108 + i, 111 + i, 100);   // a rise to the 121 pivot high (the move's origin)
        B2(120, 121, 112, 113, 120);   // pivot high area
        B2(113, 114, 104, 105, 300);   // Bk PDL 110 (closes below)
        B2(105, 106, 98, 99, 200);     // FT (red, closes below the Bk low 104)
        B2(99, 100, 92, 93, 200); B2(93, 94, 90, 91, 200);   // the move to 90
        B2(91, 99, 91, 98, 150); B2(98, 104, 97, 103, 150); B2(103, 106, 102, 105, 150);   // pullback up to 106 (overlaps the Bk candle 104-114)
        B2(105, 105, 99, 100, 150); B2(100, 101, 97, 98, 150); B2(98, 99, 95, 96, 150);   // pivot high confirmed; candle 2 back
        for (int i = 0; i < 4; ++i) B2(96, 97, 94, 95, 100);
        std::vector<wyr::Lvl> k(1); k[0].px = 110; k[0].name = "PDL";
        const auto M = lvm::run(v, k); const auto S = lvm::seq(v, M);
        auto has = [&](int i, char c) { const auto it = S.find(v[(size_t)i].t); if (it == S.end()) return (const lvm::SMark*)nullptr; for (const auto& m : it->second) if (m.code == c) return &m; return (const lvm::SMark*)nullptr; };
        C(M.count(v[11].t) && M.at(v[11].t).code == 'B', "rule: Bk PDL (range >= the prior candle's, volume up)");
        C(has(12, 'F') != nullptr, "rule: FT = the next red candle closing below the Bk candle's low");
        const lvm::SMark* t = has(17, 'T'); C(t && t->lv == "PDL" && t->pct >= 38 && t->pct <= 100, "rule: Tst / PDL on the pullback pivot high that overlaps the Bk candle");
        C(has(18, 'R') != nullptr, "rule: Rev = the red thrust candle after the Tst (range > 0.8 ATR, close below the prior low)");
        // the same with the pullback taking out the Bk candle's high (115 > 114): no Tst
        std::vector<wyr::Bar> w = v; w[17].h = 115; const auto S2 = lvm::seq(w, lvm::run(w, k)); bool anyT = false; for (const auto& kv : S2) for (const auto& m : kv.second) if (m.code == 'T') anyT = true;
        C(!anyT, "rule: a pullback that takes out the Bk candle's high is not a Tst");
        // a pullback that does not reach the Bk candle (stops at 103 < 104): no Tst (it must overlap)
        std::vector<wyr::Bar> u = v; u[16].h = 102; u[17].h = 103; u[18] = bar(u[18].t, 102, 102, 99, 100, 150); const auto S3 = lvm::seq(u, lvm::run(u, k)); anyT = false; for (const auto& kv : S3) for (const auto& m : kv.second) if (m.code == 'T') anyT = true;
        C(!anyT, "rule: no near-miss - the Tst pivot must overlap the Bk candle");
        // no FT -> no Tst
        std::vector<wyr::Bar> x = v; x[12] = bar(x[12].t, 105, 106, 104, 105, 200); x[13] = bar(x[13].t, 105, 105, 104, 104, 200); x[14] = bar(x[14].t, 104, 104, 90, 104, 200);
        const auto S4 = lvm::seq(x, lvm::run(x, k)); anyT = false; bool anyF = false; for (const auto& kv : S4) for (const auto& m : kv.second) { if (m.code == 'T') anyT = true; if (m.code == 'F') anyF = true; }
        C(!anyF && !anyT, "rule: no FT within 3 candles = no Tst");
        // the BO candle rule: a Bk candle smaller than the prior candle is not a Bk
        std::vector<wyr::Bar> y = v; y[10] = bar(y[10].t, 120, 125, 105, 113, 120); const auto M5 = lvm::run(y, k);
        C(!M5.count(y[11].t) || M5.at(y[11].t).code != 'B', "rule: Bk needs range >= the prior candle's range");
        // the Sw colour rule: a bullish Sw on a red candle is not a Sw
        auto f2 = [&](int n, long long p) { std::vector<wyr::Bar> z; for (int i = 0; i < n; ++i) z.push_back(bar(T2 + 90000 + (i + 1) * 180, p, p + 2, p - 2, p, 100)); return z; };
        auto z = f2(12, 120); z.push_back(bar(T2 + 90000 + 13 * 180, 120, 120, 104, 114, 300)); for (int i = 0; i < 4; ++i) z.push_back(bar(T2 + 90000 + (14 + i) * 180, 114, 118, 112, 116, 100));
        const auto M6 = lvm::run(z, k); C(!M6.count(z[12].t) || M6.at(z[12].t).code != 'S', "rule: a bullish Sw must be a green candle");
        C(lvm::pri("DPoc", 0) < 0 && lvm::pri("WPoc", 0) < 0 && lvm::pri("Open", 0) < 0 && lvm::pri("VAL", 0) >= 0, "rule: no POC / Open levels, VAH / VAL kept");
    }
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", ok, f); return f;
}
