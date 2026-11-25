// t_bk17.cpp -- LevelMarks 1.17.0 (Rassul 2026-11-24 08:33-09:01 CT "go for hourly pivots and 4h high and lows", "absorption levels should match
// bk levels", "go for bk with hourly pivots and 4 hour highs and lows"): the hourly pivots (HpH / HpL) and the 4-hour high / low (4hH / 4hL)
// are Bk levels (biggest volume in 5+), never Sw / Cx levels.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_bk17.cpp -o t && ./t
#include "../LevelMarks.h"
#include "../HtfSr.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static std::vector<std::string> split(const std::string& s) { std::vector<std::string> v; std::stringstream ss(s); std::string x; while (std::getline(ss, x, '|')) v.push_back(x); return v; }
static wyr::Bar bar(int i, long long o, long long h, long long l, long long c, long long vol = 100) { wyr::Bar b; b.t = 3878236800LL + 9 * 3600 + i * 180LL; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = vol / 2; b.sell = vol - vol / 2; return b; }
static std::vector<wyr::Bar> flat(int n) { std::vector<wyr::Bar> B; for (int i = 0; i < n; ++i) B.push_back(i % 2 ? bar(i, 99, 101, 98, 100) : bar(i, 100, 101, 98, 99)); return B; }
static std::string one(const std::map<long long, lvm::Mark>& M, long long t) { const auto it = M.find(t); return it == M.end() ? std::string("-") : std::string(lvm::codeText(it->second.code)) + " " + it->second.lv + " " + std::to_string(it->second.dir); }
int main() {
    C(std::string(lvm::VERSION) == "1.24.0", "LevelMarks 1.17.0");
    C(lvm::pri("4hH", 0) == 65 && lvm::pri("4hL", 0) == 65 && lvm::pri("HpH", 0) == 70 && lvm::pri("HpL", 0) == 70, "4h after the pre-open hour, pivots last");
    C(lvm::pri("4hH", 0) > lvm::pri("PHH", 0) && lvm::pri("HpL", 0) > lvm::pri("x", 2), "ranked after every older level");
    C(!lvm::swMajor("HpH", 0) && !lvm::swMajor("4hL", 0), "never a Sw level");
    // ---- PivWalk on 3-hour blocks = hsr::Walk (the tested 3h walk) on GC's recorded candles, every candle
    { std::ifstream in("gc1123.psv"); std::string l; std::vector<wyr::Bar> A;
      while (std::getline(in, l)) { const auto v = split(l); if (v.size() < 8 || v[0] != "B") continue; wyr::Bar b; b.t = std::stoll(v[1]); b.o = std::stoll(v[2]); b.h = std::stoll(v[3]); b.l = std::stoll(v[4]); b.c = std::stoll(v[5]); A.push_back(b); }
      C(A.size() > 2000, "gc1123.psv read (" + std::to_string(A.size()) + " candles)");
      hsr::Walk W; lvm::PivWalk P; P.secs = 10800; P.off = 7200; int same = 0, n = 0;
      for (const wyr::Bar& b : A) { hsr::Bar q; q.t = b.t; q.h = (double)b.h; q.l = (double)b.l; q.c = (double)b.c; const hsr::Snap s = W.add(q, 180); const lvm::PivSnap p = P.add(b.t, 180, b.h, b.l, b.c);
          auto tk = [](bool okv, double x) -> long long { return okv && std::isfinite(x) ? (long long)std::llround(x) : 0; };
          const long long r2 = tk(s.r2ok, s.r2) == tk(s.r1ok, s.r1) ? 0 : tk(s.r2ok, s.r2), s2 = tk(s.s2ok, s.s2) == tk(s.s1ok, s.s1) ? 0 : tk(s.s2ok, s.s2);
          ++n; if (p.r1 == tk(s.r1ok, s.r1) && p.s1 == tk(s.s1ok, s.s1) && p.r2 == r2 && p.s2 == s2) ++same; }
      C(same == n, "PivWalk(3h) = hsr::Walk on every candle (" + std::to_string(same) + " / " + std::to_string(n) + ")");
      // the hourly walk gives standing pivots most of the time and they sit on 1-hour block highs / lows
      lvm::PivWalk H; int have = 0; for (const wyr::Bar& b : A) { const lvm::PivSnap p = H.add(b.t, 180, b.h, b.l, b.c); if (p.r1 > 0 || p.s1 > 0) ++have; }
      C(have > (int)A.size() * 8 / 10, "hourly pivots exist on most candles (" + std::to_string(have) + ")"); }
    // ---- Blk4: the last COMPLETED 4-hour block (from 17:00), before the candle
    { lvm::Blk4 K; const long long d0 = 3878236800LL;   // a local midnight
      // candles ending 17:03 .. 21:00 = block 17-21; the candle ending 21:03 starts 21-01
      std::pair<long long, long long> r; long long hi = 0, lo = 1 << 30;
      for (long long t = d0 + 17 * 3600 + 180; t <= d0 + 21 * 3600; t += 180) { const long long h = 100 + (t / 180) % 7, l = 90 - (t / 180) % 5; hi = std::max(hi, h); lo = std::min(lo, l); r = K.add(t, 180, h, l); C(r.first == 0, "no completed block yet"); }
      r = K.add(d0 + 21 * 3600 + 180, 180, 120, 80); C(r.first == hi && r.second == lo, "the 21:03 candle sees the 17-21 block's high / low");
      r = K.add(d0 + 21 * 3600 + 360, 180, 130, 70); C(r.first == hi && r.second == lo, "still the 17-21 block (the forming block is not used)"); }
    // ---- a candle that closes down through an hourly pivot low: Bk HpL only with the biggest volume in 5+
    { std::vector<wyr::Bar> B = flat(12); B.push_back(bar(12, 99, 99, 90, 91, 400)); B.push_back(bar(13, 91, 92, 89, 90)); const long long t = B[12].t;
      std::vector<lvm::H3> H(B.size()); for (lvm::H3& q : H) q.hs1 = 95;
      C(one(lvm::run(B, std::vector<wyr::Lvl>(), -1, 0, nullptr, &H), t) == "Bk HpL -1", "close down through the hourly pivot low on big volume: Bk HpL");
      std::vector<wyr::Bar> B2 = B; B2[12] = bar(12, 99, 99, 90, 91, 60);   // (1.21.0) range up, volume not the biggest: now a Bk (volume OR range up)
      C(one(lvm::run(B2, std::vector<wyr::Lvl>(), -1, 0, nullptr, &H), t) == "Bk HpL -1", "(1.21.0) the same break on small volume but a bigger range: Bk HpL");
      std::vector<wyr::Bar> B3 = B; B3[12] = bar(12, 99, 99, 90, 91, 1); B3[11] = bar(11, 100, 100, 90, 99, 50);   // neither range nor volume up
      C(lvm::run(B3, std::vector<wyr::Lvl>(), -1, 0, nullptr, &H).count(t) == 0, "(1.21.0) neither volume nor range up: no Bk");
      std::vector<lvm::H3> H4(B.size()); for (lvm::H3& q : H4) { q.hs1 = 95; q.h4l = 95; }
      C(one(lvm::run(B, std::vector<wyr::Lvl>(), -1, 0, nullptr, &H4), t) == "Bk 4hL -1", "pivot and 4h low on one price: the 4h names it (65 < 70)");
      std::vector<lvm::H3> H5(B.size()); for (lvm::H3& q : H5) { q.hs1 = 95; q.s1 = 95; }
      C(one(lvm::run(B, std::vector<wyr::Lvl>(), -1, 0, nullptr, &H5), t) == "Bk S1 -1", "a 3h S1 on the same price wins"); }
    // ---- a sweep of only an hourly pivot / 4h level: no Sw (Sw unchanged)
    { std::vector<wyr::Bar> B = flat(12); B.push_back(bar(12, 98, 100, 93, 99, 400)); B.push_back(bar(13, 99, 101, 98, 100)); const long long t = B[12].t;
      std::vector<lvm::H3> H(B.size()); for (lvm::H3& q : H) { q.hs1 = 95; q.h4l = 95; }
      C(lvm::run(B, std::vector<wyr::Lvl>(), -1, 0, nullptr, &H).count(t) == 0 || one(lvm::run(B, std::vector<wyr::Lvl>(), -1, 0, nullptr, &H), t).rfind("Sw", 0) != 0, "a sweep of the pivot / 4h low only: no Sw"); }
    // ---- on GC's real candles: every Sw / Cx is identical with and without the new levels; new marks are Bk only
    { std::ifstream in("gc1123.psv"); std::string l; std::vector<wyr::Bar> A;
      while (std::getline(in, l)) { const auto v = split(l); if (v.size() < 8 || v[0] != "B") continue; wyr::Bar b; b.t = std::stoll(v[1]); b.o = std::stoll(v[2]); b.h = std::stoll(v[3]); b.l = std::stoll(v[4]); b.c = std::stoll(v[5]); b.buy = std::stoll(v[6]); b.sell = std::stoll(v[7]); A.push_back(b); }
      const int s0 = (int)A.size() - 460; std::vector<wyr::Bar> W(A.begin() + s0, A.end());
      hsr::Walk w3; lvm::PivWalk ph; lvm::Blk4 b4; std::vector<lvm::H3> Hold(A.size() + 1), Hnew(A.size() + 1);
      for (size_t i = 0; i < A.size(); ++i) { const wyr::Bar& b = A[i]; hsr::Bar q; q.t = b.t; q.h = (double)b.h; q.l = (double)b.l; q.c = (double)b.c; const hsr::Snap s = w3.add(q, 180);
          const auto f4 = b4.add(b.t, 180, b.h, b.l); Hnew[i].h4h = f4.first; Hnew[i].h4l = f4.second;
          const lvm::PivSnap p = ph.add(b.t, 180, b.h, b.l, b.c); lvm::H3 z; auto tk = [](bool okv, double x) -> long long { return okv && std::isfinite(x) ? (long long)std::llround(x) : 0; };
          z.r1 = tk(s.r1ok, s.r1); z.r2 = tk(s.r2ok, s.r2); z.s1 = tk(s.s1ok, s.s1); z.s2 = tk(s.s2ok, s.s2); Hold[i + 1] = z; z.hr1 = p.r1; z.hr2 = p.r2; z.hs1 = p.s1; z.hs2 = p.s2; z.h4h = Hnew[i + 1].h4h; z.h4l = Hnew[i + 1].h4l; Hnew[i + 1] = z; }
      std::vector<lvm::H3> WO(Hold.begin() + s0, Hold.begin() + s0 + (long)W.size()), WN(Hnew.begin() + s0, Hnew.begin() + s0 + (long)W.size());
      const auto Mo = lvm::run(W, std::vector<wyr::Lvl>(), lvm::rthOpenMin("GC"), 180, nullptr, &WO), Mn = lvm::run(W, std::vector<wyr::Lvl>(), lvm::rthOpenMin("GC"), 180, nullptr, &WN);
      int swcx = 0, swcxSame = 0, bkOld = 0, bkKept = 0, bkNew = 0, newOk = 0;
      for (const auto& kv : Mo) { if (kv.second.code != 'B') { ++swcx; const auto it = Mn.find(kv.first); if (it != Mn.end() && it->second.code == kv.second.code && it->second.lv == kv.second.lv) ++swcxSame; } else { ++bkOld; if (Mn.count(kv.first)) ++bkKept; } }
      for (const auto& kv : Mn) if (!Mo.count(kv.first)) { ++bkNew; if (kv.second.code == 'B' && (kv.second.lv == "HpH" || kv.second.lv == "HpL" || kv.second.lv == "4hH" || kv.second.lv == "4hL")) ++newOk; else std::printf("  new mark %lld %s %s\n", kv.first, lvm::codeText(kv.second.code), kv.second.lv.c_str()); }
      std::printf("  GC last session: Sw/Cx %d (same %d), Bk before %d (kept %d), new marks %d (Bk on the new levels %d)\n", swcx, swcxSame, bkOld, bkKept, bkNew, newOk);
      C(swcx == swcxSame, "every Sw / Cx unchanged"); C(bkKept == bkOld, "every earlier Bk kept"); C(newOk == bkNew, "every new mark is a Bk on an hourly pivot / 4h level"); }
    std::printf("%d ok, %d failed\n", ok, f); return f ? 1 : 0; }
