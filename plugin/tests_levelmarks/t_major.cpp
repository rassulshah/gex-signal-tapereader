// t_major.cpp -- LevelMarks 1.14.0 (Rassul 2026-11-23 14:00-14:18 CT "for SW signal it needs to cross a major level like a session or pd pf  weekly or 3h S1 S2 ,
// R1 R2 level etc", "no the 3h is  the S1 S2 R1 R2  levels", "no the bk and tst can also include hourly", "ok so you are just adding to bk right ?", "build"):
// a Sw only on a major level, the 3-hour S / R as mark levels. Real candles: GC 2026-11-12 .. 11-23 with the live marks file of 11-23 2:15 PM (written by 1.13.0).
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_major.cpp -o t && ./t
#include "../LevelMarks.h"
#include "../HtfSr.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static std::vector<std::string> split(const std::string& s) { std::vector<std::string> v; std::stringstream ss(s); std::string x; while (std::getline(ss, x, '|')) v.push_back(x); return v; }
static wyr::Bar bar(int i, long long o, long long h, long long l, long long c, long long vol = 100) { wyr::Bar b; b.t = 3878236800LL + 9 * 3600 + i * 180LL; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = vol / 2; b.sell = vol - vol / 2; return b; }
static std::vector<wyr::Bar> flat(int n) { std::vector<wyr::Bar> B; for (int i = 0; i < n; ++i) B.push_back(i % 2 ? bar(i, 99, 101, 98, 100) : bar(i, 100, 101, 98, 99)); return B; }
static wyr::Lvl key(const char* n, long long px) { wyr::Lvl z; z.name = n; z.px = px; return z; }
static std::string one(const std::map<long long, lvm::Mark>& M, long long t) { const auto it = M.find(t); return it == M.end() ? std::string("-") : std::string(lvm::codeText(it->second.code)) + " " + it->second.lv + " " + std::to_string(it->second.dir); }
int main() {
    C(std::string(lvm::VERSION) == "1.20.0", "LevelMarks 1.14.0");
    // ---- which levels may name a Sw
    for (const char* n : { "ONH", "ONL", "LonHI", "LonLO", "AsiaHI", "AsiaLO", "ONH/LonHI", "PDH", "PDL", "PFH/PDH", "PFH", "PFL", "PWH", "PWL", "CWH", "CWL", "R1", "R2", "S1", "S2", "PFL/PWL" }) C(lvm::swMajor(n, 0), std::string("major: ") + n);
    for (const char* n : { "VAH", "VAL", "PHH", "PHL", "DPoc", "WPoc", "Open", "VWAP", "CR0", "" }) C(!lvm::swMajor(n, 0), std::string("not major: ") + n);
    C(!lvm::swMajor("ONH", 1) && !lvm::swMajor("9 hour high", 2), "an hour level is never major");
    C(lvm::pri("R1", 0) == 45 && lvm::pri("S2", 0) == 45 && lvm::pri("PWH", 0) < 45 && lvm::pri("PHL", 0) > 45 && lvm::shortName("R1", 0, true) == "R1", "the 3h S / R: after weekly, before the hour levels; shown as R1 .. S2");
    // ---- hand-made: 12 flat candles (98 .. 101), then a green candle that dips to 93 and closes back at 99
    { std::vector<wyr::Bar> B = flat(12); B.push_back(bar(12, 98, 100, 93, 99, 400)); B.push_back(bar(13, 99, 101, 98, 100)); const long long t = B[12].t;
      C(one(lvm::run(B, { key("ONL", 95) }), t) == "Sw ONL 1", "a sweep of ONL is a Sw");
      C(one(lvm::run(B, { key("PDL", 95) }), t) == "Sw PDL 1" && one(lvm::run(B, { key("PFL", 95) }), t) == "Sw PFL 1" && one(lvm::run(B, { key("PWL", 95) }), t) == "Sw PWL 1", "PDL, PFL, PWL: a Sw");
      C(lvm::run(B, { key("VAL", 95) }).count(t) == 0, "a sweep of VAL only: no Sw (was a Sw in 1.13.0)");
      C(lvm::run(B, std::vector<wyr::Lvl>()).count(t) == 0, "a sweep of the flat candles' own lows / hour levels only: no Sw");
      C(one(lvm::run(B, { key("VAL", 96), key("ONL", 95) }), t) == "Sw ONL 1", "VAL and ONL swept together: the Sw names ONL");
      std::vector<lvm::H3> H(B.size()); for (lvm::H3& q : H) q.s1 = 95;
      C(one(lvm::run(B, std::vector<wyr::Lvl>(), -1, 0, nullptr, &H), t) == "Sw S1 1", "a sweep of the 3h S1: Sw S1");
      C(one(lvm::run(B, { key("ONL", 95) }, -1, 0, nullptr, &H), t) == "Sw ONL 1", "ONL and S1 on one price: the session level names it");
      std::vector<lvm::H3> H2(B.size()); for (size_t i = 0; i < H2.size(); ++i) if (i >= 10) H2[i].s2 = 95;
      C(one(lvm::run(B, std::vector<wyr::Lvl>(), -1, 0, nullptr, &H2), t) == "Sw S2 1", "S2 standing only since candle 10: still swept at 12");
      std::vector<lvm::H3> H3v(B.size() - 1); C(lvm::run(B, std::vector<wyr::Lvl>(), -1, 0, nullptr, &H3v).count(t) == 0, "a 3h list of the wrong length is ignored");
      // traded through before: not a sweep again
      std::vector<wyr::Bar> B2 = flat(12); B2[6] = bar(6, 100, 101, 94, 99); B2.push_back(bar(12, 98, 100, 93, 99, 400)); B2.push_back(bar(13, 99, 101, 98, 100));
      std::vector<lvm::H3> H4(B2.size()); for (lvm::H3& q : H4) q.s1 = 95;
      C(lvm::run(B2, std::vector<wyr::Lvl>(), -1, 0, nullptr, &H4).count(B2[12].t) == 0, "S1 already traded through at candle 6: no Sw at 12 (the strict rule holds for 3h levels)"); }
    // ---- hand-made: a candle that closes down through S1 = Bk S1; an hour-only break keeps its hour label
    { std::vector<wyr::Bar> B = flat(12); B.push_back(bar(12, 98, 99, 90, 91, 900)); B.push_back(bar(13, 91, 92, 89, 90)); const long long t = B[12].t;
      std::vector<lvm::H3> H(B.size()); for (lvm::H3& q : H) q.s1 = 95;
      C(one(lvm::run(B, std::vector<wyr::Lvl>(), -1, 0, nullptr, &H), t) == "Bk S1 -1", "a close down through the 3h S1: Bk S1");
      C(one(lvm::run(B, { key("VAL", 95) }), t) == "Bk VAL -1", "Bk keeps VAH / VAL");
      C(one(lvm::run(B, { key("ONL", 95) }, -1, 0, nullptr, &H), t) == "Bk ONL -1", "ONL and S1 on one price: Bk ONL"); }
    // ---- a sweep of an hour low that closes up through a major level is the Bk it is (GC 7:57 AM in small)
    { std::vector<wyr::Bar> B = flat(12); B.push_back(bar(12, 99, 108, 96, 107, 900)); B.push_back(bar(13, 107, 109, 106, 108)); const long long t = B[12].t;
      C(one(lvm::run(B, { key("ONH", 104) }), t) == "Bk ONH 1", "dips under the flat lows, closes up through ONH: Bk ONH (1.13.0 called it a Sw of the hour low)"); }
    // ---- GC 2026-11-23, the real candles
    std::vector<wyr::Bar> A; std::vector<wyr::Lvl> keys; std::map<long long, std::string> live; std::string line;
    { std::ifstream in("gc1123.psv"); while (std::getline(in, line)) { const auto v = split(line); if (v.size() < 8 || v[0] != "B") continue;
        wyr::Bar b; b.t = std::stoll(v[1]); b.o = std::stoll(v[2]); b.h = std::stoll(v[3]); b.l = std::stoll(v[4]); b.c = std::stoll(v[5]); b.buy = std::stoll(v[6]); b.sell = std::stoll(v[7]); A.push_back(b); } }
    { std::ifstream in("wy_GC_1123.txt"); while (std::getline(in, line)) { if (!line.empty() && line.back() == '\r') line.pop_back(); const auto v = split(line);
        if (v.size() >= 3 && v[0] == "L") { wyr::Lvl z; z.px = std::stoll(v[1]); z.name = v[2]; if (lvm::keyForI(z.name, z.px)) keys.push_back(z); }
        if (v.size() >= 5 && (v[1] == "Sw" || v[1] == "Bk" || v[1] == "Cx")) live[std::stoll(v[0])] = v[1] + " " + v[3] + " " + v[2]; } }
    C(A.size() > 3000 && keys.size() >= 12, "GC fixture loaded");
    const long long D = 3878236800LL, s0 = D - 7 * 3600 + 180;
    hsr::Walk W; std::vector<hsr::Snap> hs; for (const wyr::Bar& b : A) { hsr::Bar q; q.t = b.t; q.h = (double)b.h; q.l = (double)b.l; q.c = (double)b.c; hs.push_back(W.add(q, 180)); }
    C(hs.back().r1ok && std::llround(hs.back().r1) == 36910 && hs.back().s1ok && std::llround(hs.back().s1) == 35999 && hs.back().s2ok && std::llround(hs.back().s2) == 35029, "the 3h levels at 2:15 PM: R1 3691.0, S1 3599.9, S2 3502.9 (his 180-minute chart)");
    int first = 0; while (first < (int)A.size() && A[(size_t)first].t < s0) ++first;
    auto sess = [&](int n, std::vector<wyr::Bar>& WB, std::vector<lvm::H3>& H) { WB.assign(A.begin() + first, A.begin() + n); H.assign(WB.size(), lvm::H3());
        for (size_t j = 0; j < WB.size(); ++j) { const hsr::Snap& s = hs[(size_t)first + j - 1]; lvm::H3 q; if (s.r1ok) q.r1 = std::llround(s.r1); if (s.r2ok) q.r2 = std::llround(s.r2); if (s.s1ok) q.s1 = std::llround(s.s1); if (s.s2ok) q.s2 = std::llround(s.s2); H[j] = q; } };
    std::vector<wyr::Bar> WB; std::vector<lvm::H3> H; sess((int)A.size(), WB, H);
    const std::map<long long, lvm::Mark> oldRule = lvm::run(WB, keys, lvm::rthOpenMin("GC"), 180, nullptr);            // no 3h levels handed in
    std::map<long long, lvm::Mark> M = lvm::run(WB, keys, lvm::rthOpenMin("GC"), 180, nullptr, &H);
    { std::vector<long long> T; std::vector<double> V, R; std::vector<char> G; for (const wyr::Bar& q : A) { T.push_back(q.t); V.push_back((double)(q.buy + q.sell)); R.push_back((double)(q.h - q.l)); G.push_back(q.c >= q.o ? 1 : 0); }
      lvm::fillStrength(M, T, V, R, G); }   // as the host: a Sw under strength 20 is not kept (two weak ones here: PDH 5:24 PM, AsiaHI 7:30 AM - as before)
    C(M.count(D + 7 * 3600 + 57 * 60) && one(M, D + 7 * 3600 + 57 * 60) == "Bk ONH 1", "GC 7:57 AM: Bk ONH (was Sw PhrL) - " + one(M, D + 7 * 3600 + 57 * 60));
    C(!M.count(D + 8 * 3600 + 6 * 60) && !M.count(D + 13 * 3600 + 3 * 60), "GC 8:06 AM (Sw 1hH) and 1:03 PM (Sw 1hL): no mark any more");
    { int sw = 0, same = 0, other = 0; for (const auto& kv : M) if (kv.second.code == 'S') ++sw;
      for (const auto& kv : live) { if (kv.first < s0 || kv.second.compare(0, 2, "Sw") == 0) continue; const auto it = M.find(kv.first);
          if (it != M.end() && std::string(lvm::codeText(it->second.code)) + " " + it->second.lv + " " + std::to_string(it->second.dir) == kv.second) ++same; else { ++other; std::printf("  differs: %lld live '%s' now '%s'\n", kv.first - D, kv.second.c_str(), one(M, kv.first).c_str()); } }
      C(sw == 0, "GC today: no Sw left (none of the three crossed a major level)"); C(same == 14 && other == 3, "(1.20.0: the 3 wrong-side Bk gone - bearish through VAH / 3hH, bullish through 2hL) every other Bk / Cx of the live file is unchanged: " + std::to_string(same) + " same, " + std::to_string(other) + " other");
      C(M.size() == 15, "15 marks: the live file's 17 Bk / Cx - 3 wrong-side Bk + the 7:57 AM Bk"); }
    for (const auto& kv : oldRule) if (kv.second.code == 'S') C(lvm::swMajor(kv.second.lv == "LHI" ? "LonHI" : kv.second.lv, 0) || kv.second.lv.size() == 3, "without 3h levels a Sw label is still major: " + kv.second.lv);
    // every Sw of every session of the fixture carries a major label; 3h-labelled marks exist somewhere in the 8 sessions
    { int sw = 0, h3m = 0, bad = 0; long long day = A.front().t;
      for (int a = 0; a < (int)A.size(); ) { int b = a; const long long sk = (A[(size_t)a].t + 7 * 3600 - 180) / 86400; while (b < (int)A.size() && (A[(size_t)b].t + 7 * 3600 - 180) / 86400 == sk) ++b;
          std::vector<wyr::Bar> S(A.begin() + a, A.begin() + b); std::vector<lvm::H3> HH(S.size());
          for (size_t j = 0; j < S.size(); ++j) if (a + (int)j >= 1) { const hsr::Snap& s = hs[(size_t)a + j - 1]; lvm::H3 q; if (s.r1ok) q.r1 = std::llround(s.r1); if (s.r2ok) q.r2 = std::llround(s.r2); if (s.s1ok) q.s1 = std::llround(s.s1); if (s.s2ok) q.s2 = std::llround(s.s2); HH[j] = q; }
          for (const auto& kv : lvm::run(S, std::vector<wyr::Lvl>(), lvm::rthOpenMin("GC"), 180, nullptr, &HH)) { const std::string& lv = kv.second.lv; const bool is3 = lv == "R1" || lv == "R2" || lv == "S1" || lv == "S2";
              if (kv.second.code == 'S') { ++sw; if (!is3) ++bad; } if (is3) ++h3m; }
          a = b; (void)day; }
      C(bad == 0, "8 sessions with no level file: every Sw is on a 3h level (" + std::to_string(sw) + " Sw)"); C(h3m >= 5, "marks on the 3h levels exist: " + std::to_string(h3m)); }
    // no repaint: today candle by candle - a mark on a candle 4+ candles back never changes or leaves
    { int bad = 0, steps = 0; std::map<long long, std::string> seen;
      for (int n = first + 60; n <= (int)A.size(); n += 1) { std::vector<wyr::Bar> w; std::vector<lvm::H3> h; sess(n, w, h); const auto m = lvm::run(w, keys, lvm::rthOpenMin("GC"), 180, nullptr, &h); ++steps;
          const long long cut = w[w.size() - 5].t;
          for (const auto& kv : m) if (kv.first <= cut) { const std::string v = one(m, kv.first); const auto it = seen.find(kv.first); if (it == seen.end()) seen[kv.first] = v; else if (it->second != v) { ++bad; it->second = v; } }
          for (const auto& kv : seen) if (kv.first <= cut && !m.count(kv.first)) { ++bad; } }
      C(bad == 0 && steps > 300, "no repaint over " + std::to_string(steps) + " candle closes: " + std::to_string(bad) + " changes"); }
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", ok, f); return f;
}
