// t_hl2440.cpp -- lsHodLod 2.44.0 / NowLevels 1.4.0 (Rassul 2026-11-23 14:09 CT GC screenshot: the line read "WATCH: PHL .. 2hL" with PFH 3638.1 and ONH 3636.4
// right under price - "why isnt the level line catching the most obvious levels"; 14:15 "and for levels, you are also using the same levels as SW right"; "build"):
// the Level line names MAJOR levels only (session, PD, PF, weekly, the 3-hour S / R). Real candles: GC 2026-11-12 .. 11-23 2:15 PM, the live marks file (lsRevRead
// 0.23.0) and the marks lsRevRead 0.24.0 writes for the same candles.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2440.cpp -o t && ./t
#include "../HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../HtfSr.h"
#include "../NowLevels.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static std::vector<std::string> sp(const std::string& l) { std::vector<std::string> v; std::stringstream ss(l); std::string x; while (std::getline(ss, x, '|')) v.push_back(x); return v; }
static std::string slurp(const char* p) { std::ifstream in(p, std::ios::binary); std::stringstream s; s << in.rdbuf(); return s.str(); }
static bool has(const std::string& s, const char* w) { return s.find(w) != std::string::npos; }
int main() {
    C(std::string(nowl::VERSION) == "1.6.1" && std::string(hl::HL_VERSION) == "2.46.1" && std::string(hlset::PLUGIN_VERSION) == "2.46.1", "NowLevels 1.6.1, lsHodLod 2.46.1 = the settings version");
    for (const char* n : { "ONH", "ONL", "LHI", "LLO", "LonHI", "AHI", "AsiaLO", "PDH", "PDL", "PFH", "PFL", "PWH", "PWL", "CWH", "CWL", "R1", "R2", "S1", "S2" }) C(nowl::major(n), std::string("major: ") + n);
    for (const char* n : { "PHH", "PHL", "2hH", "3hL", "1hL", "VAH", "VAL", "PhrH", "PhrL", "DPoc", "Open", "" }) C(!nowl::major(n), std::string("not major: ") + n);
    const double tick = 0.1; const long long D = 3878236800LL - nowl::E1904, s0 = D - 7 * 3600 + 180;
    std::vector<nowl::Bar> A; std::string line;
    { std::ifstream in("gc1123.psv"); while (std::getline(in, line)) { const auto v = sp(line); if (v.size() < 8 || v[0] != "B") continue;
        nowl::Bar b; b.t = std::stoll(v[1]) - nowl::E1904; b.o = std::stoll(v[2]) * tick; b.h = std::stoll(v[3]) * tick; b.l = std::stoll(v[4]) * tick; b.c = std::stoll(v[5]) * tick; b.v = std::stod(v[6]); A.push_back(b); } }
    const std::string wyOld = slurp("wy_GC_1123.txt"), wyNew = slurp("wy_GC_1123_new.txt"); std::vector<nowl::Key> file;
    { std::stringstream in(wyOld); while (std::getline(in, line)) { if (!line.empty() && line.back() == '\r') line.pop_back(); const auto v = sp(line); if (v.size() < 3 || v[0] != "L") continue;
        std::stringstream ns(v[2]); std::string one; while (std::getline(ns, one, '/')) if (!one.empty()) file.push_back(nowl::Key{ one, std::stoll(v[1]) * tick }); } }
    C(A.size() > 3000 && file.size() >= 14 && wyNew.size() > 200, "GC fixture loaded");
    // the host's walk: every chart bar, the forming one last
    auto keysAt = [&](size_t n) { std::vector<nowl::Key> k = file; hsr::Walk W; hsr::Snap s; for (size_t i = 0; i < n; ++i) { hsr::Bar q; q.t = A[i].t; q.h = A[i].h; q.l = A[i].l; q.c = A[i].c; s = W.add(q, 180); }
        for (const hsr::Lv& z : hsr::standing(s, tick)) { k.push_back(nowl::Key{ z.name, z.px }); }
        return k; };
    auto lineAt = [&](size_t n, const std::string& wy, const std::vector<nowl::Key>& keys) { std::vector<nowl::Bar> B; for (size_t i = 0; i < n; ++i) if (A[i].t >= s0) B.push_back(A[i]);
        const std::vector<nowl::Mark> marks = nowl::parseWy(wy, tick, B.front().t - 180, B.back().t); return nowl::plain(nowl::segments(nowl::compute(B, marks, keys, B.back().c, tick))); };
    { const std::vector<nowl::Key> k = keysAt(A.size()); bool r1 = false, s1 = false, s2 = false;
      for (const nowl::Key& q : k) { if (q.name == "R1" && std::fabs(q.px - 3691.0) < 1e-6) r1 = true; if (q.name == "S1" && std::fabs(q.px - 3599.9) < 1e-6) s1 = true; if (q.name == "S2" && std::fabs(q.px - 3502.9) < 1e-6) s2 = true; }
      C(r1 && s1 && s2, "the 3h keys at 2:15 PM: R1 3691.0, S1 3599.9, S2 3502.9 (his 180-minute chart)");
      const std::string a = lineAt(A.size(), wyNew, k), b = lineAt(A.size(), wyOld, k);
      C(a == "[VOB(T) WATCH: PFH \xE2\x86\x93 \xC2\xB7 R1 \xE2\x86\x91 \xC2\xB7 Bk / VOB(T)] \xE2\x94\x82 Last Bk \xE2\x86\x91 ONH", "GC 2:15 PM with 0.24.0's marks: " + a);
      C(b == "[VOB(T) WATCH: PFH \xE2\x86\x93 \xC2\xB7 R1 \xE2\x86\x91 \xC2\xB7 Bk / VOB(T)] \xE2\x94\x82 Last Bk \xE2\x86\x91 LHI", "the same with the live (0.23.0) marks: the last MAJOR break is LHI 7:27 AM (the 1hL breaks after it are not named): " + b); }
    // every candle close of today: no hour level, VAH / VAL or pre-open hour on the line; a line is always there
    { int bad = 0, empty = 0, steps = 0; hsr::Walk W; hsr::Snap s; size_t fed = 0;
      for (size_t n = 0; n < A.size(); ++n) { if (A[n].t < s0 + 60 * 180) continue; ++steps;
          for (; fed <= n; ++fed) { hsr::Bar q; q.t = A[fed].t; q.h = A[fed].h; q.l = A[fed].l; q.c = A[fed].c; s = W.add(q, 180); }
          std::vector<nowl::Key> k = file; for (const hsr::Lv& z : hsr::standing(s, tick)) k.push_back(nowl::Key{ z.name, z.px });
          const std::string l = lineAt(n + 1, wyOld, k); if (l.empty()) ++empty;
          for (const char* w : { "PHL", "PHH", "2h", "3h", "1h", "VAH", "VAL", "Phr" }) if (has(l, w)) { ++bad; if (bad < 4) std::printf("  %s\n", l.c_str()); } }
      C(bad == 0 && steps > 300, "no hour level / VAH / VAL / PhrH on the line over " + std::to_string(steps) + " candle closes (" + std::to_string(bad) + " bad)"); C(empty == 0, "the line is never empty"); }
    // the 7:57 AM candle just closed (8:00 forming): its mark decides the badge
    { size_t n = 0; while (n < A.size() && A[n].t <= D + 7 * 3600 + 57 * 60) ++n; ++n;   // + the forming 8:00 candle
      const std::vector<nowl::Key> k = keysAt(n); const std::string st = std::to_string(D + 7 * 3600 + 57 * 60 + nowl::E1904);
      const std::string bk = lineAt(n, "WY|1|GC|180|0.24.0\n" + st + "|Bk|1|ONH|36364|V99+\n", k), swH = lineAt(n, "WY|1|GC|180|0.23.0\n" + st + "|Sw|1|PhrL|36125|V99+\n", k);
      const std::string swM = lineAt(n, "WY|1|GC|180|0.24.0\n" + st + "|Sw|1|ONL|36125|V99+\n", k), bkH = lineAt(n, "WY|1|GC|180|0.24.0\n" + st + "|Bk|1|1hH|36270|V99+\n", k), bk3 = lineAt(n, "WY|1|GC|180|0.24.0\n" + st + "|Bk|1|R1|36364|V99+\n", k);
      C(has(bk, "BREAK: ONH"), "a Bk ONH mark on the closed candle: BREAK: ONH - " + bk);
      C(!has(swH, "SWEEP:") && has(swH, "WATCH"), "a Sw on PhrL (not major) gives no SWEEP badge - " + swH);
      C(has(swM, "VOB: ONL"), "a Sw on a session level: SWEEP - " + swM);
      C(!has(bkH, "BREAK") && !has(bkH, "Last Bk") && !has(bkH, "1hH"), "a Bk on an hour level: no BREAK badge, no Last Bk part - " + bkH);
      C(has(bk3, "BREAK: R1"), "a Bk on the 3h R1: BREAK: R1 - " + bk3); }
    std::printf("t_hl2440 (lsHodLod 2.46.1): %d passed, %d failed\n", ok, f); return f ? 1 : 0;
}
