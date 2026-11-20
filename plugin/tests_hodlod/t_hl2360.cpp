// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2360.cpp -o t2360 && ./t2360   (from plugin\tests_hodlod)
// lsHodLod 2.36.0 (2026-11-19 18:35-18:47 CT, Rassul, mockup now-box-levels-mockup.html): the Now box becomes ONE line of named levels
// (NowLevels.h 1.0.0), most important first:
//   a Tst in progress: [TESTING: <level> · BkTst Rev <arrow>] │ Next ↓ <level> · ↑ <level> │ Last Bk <arrow> <level>     (18:48, red / green)
//   else:              [WATCH: <level> ↓ · <level> ↑ · Bk / Sw] │ Last Bk <arrow> <level>   (gold; 19:06; BkTst: "WATCH: PDL BkTst Rev ↓", 19:03)
//   no numbers at all (18:42-18:43); one badge at a time (18:47); no level named twice - the WATCH badge carries the next levels, a Next or
//   Last Bk level already named is left out (18:50)
//  [1] parse: the wy Bk lines (this session, the marks' level names), other sessions / garbage / bad sides / POC levels ignored
//  [2] the three states live on the forming candle (18:52): WATCH (gold, 18:53) = the move topped and the pullback heads back but is not in
//      the Bk candle yet; TESTING (red / green) = it trades into the Bk candle (<= 100% back, never past its extreme); else the gold Bk / Sw
//      badge: still extending, just topped, took out the Bk candle high, waiting for / no FT, the pullback already turned, a Bk the other way;
//      candle, no FT, the pullback already turned, a Bk the other way since; bullish mirror
//  [3] next levels: the marks' set (no DPoc / WPoc / Open / VWAP / gamma), hour levels live from the bars, the tested level skipped
//  [4] his real ES 11-19: 11:33 AM TESTING: PDL · BkTst Rev down | Next | Last Bk; 10:45 AM no test: only "WATCH: VAL ↓ · ONL ↑ · Bk / Sw"
//  [4b] live BREAK / SWEEP (18:54): 9:42 AM "SWEEP: PDL ↑ │ Next ↑ VAL", 10:54 AM "BREAK: PDL ↓"; red / green by side; TESTING outranks
//  [4c] no Cob states, no REV state (19:02-19:04): after a BkTst Rev the line moves on to the next WATCH
//  [5] layout: one line, badges filled with black bold text (red / green TESTING, gold WATCH), arrows drawn as strokes, cp1252 only,
//      inside its box and the pane; narrow: the Last Bk part drops first, then pieces off the right end, never a dangling separator
//  [6] versions 2.36.0 everywhere + the build list
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include "../NowLevels.h"
#include "../HodLodLogic.h"
#include "../HodLodSettings.h"
#include "lib_widths.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; if (fails < 40) std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static std::string rd(const char* p) { std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
static long long L(int y, int mo, int d, int h, int mi) {   // local secs, days from 1970 (civil)
    y -= mo <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + (long long)doe - 719468) * 86400 + h * 3600 + mi * 60; }
using namespace nowl;
static std::string st(long long t) { return std::to_string(t + E1904); }

// a bearish move: origin high 103 (bar 2), Bk candle bar 4 (95..99), FT bar 5, extreme 92 (bar 6), pullback from bar 7; the forming candle last
static std::vector<Bar> bear(double fo, double fh, double fl, double fc, double origin = 103) {
    const long long t0 = L(2026, 11, 19, 10, 0);
    const double R[][5] = { { 100, 101, 99, 100, 10 }, { 100, 102, 100, 101, 10 }, { 101, origin, 101, 102, 10 }, { 102, 102, 98, 98.5, 10 }, { 98, 99, 95, 95.5, 20 },
                            { 95.5, 96, 93, 93.5, 10 }, { 93.5, 94, 92, 92.5, 10 }, { 92.5, 95, 92.5, 94.8, 10 } };
    std::vector<Bar> B; for (int i = 0; i < 8; i++) { Bar b; b.t = t0 + 180LL * (i + 1); b.o = R[i][0]; b.h = R[i][1]; b.l = R[i][2]; b.c = R[i][3]; b.v = R[i][4]; B.push_back(b); }
    Bar f; f.t = t0 + 180LL * 9; f.o = fo; f.h = fh; f.l = fl; f.c = fc; B.push_back(f); return B;
}
static std::vector<Bar> mirror(std::vector<Bar> B) { for (Bar& b : B) { const double h = b.h; b.o = 200 - b.o; b.c = 200 - b.c; b.h = 200 - b.l; b.l = 200 - h; } return B; }
static Mark bk(long long t, int dir, const char* lv, double px) { Mark m; m.t = t; m.code = "Bk"; m.dir = dir; m.lv = lv; m.px = px; return m; }
static int MS(const std::string& s, bool b) { return measureLib(s, 10, b); }
static bool noNumbers(const std::string& s) { for (size_t i = 0; i < s.size(); i++) { const char c = s[i]; if (c == '.' || c == ',' || c == '%' || (c == ':' && i + 2 < s.size() && s[i + 2] >= '0' && s[i + 2] <= '9')) return false; } return s.find("pts") == std::string::npos; }

int main() {
    std::printf("[1] parse\n");
    {   const long long from = L(2026, 11, 18, 17, 0), to = L(2026, 11, 19, 14, 0);
        std::string wy = "WY|1|ES|180|0.20.0\n";
        wy += st(L(2026, 11, 18, 15, 0)) + "|Bk|-1|PDL|29221|\n";        // last session: ignored
        wy += st(L(2026, 11, 19, 10, 54)) + "|Bk|-1|PDL|29221|V99+\n";
        wy += st(L(2026, 11, 19, 9, 33)) + "|Bk|-1|ONL|29373|V99+\n";     // out of order: sorted
        wy += st(L(2026, 11, 19, 9, 40)) + "|Bk|-1|DP|29300|\n";          // a POC level: not a mark level
        wy += st(L(2026, 11, 19, 9, 41)) + "|Bk|2|PDL|29221|\n";          // bad side
        wy += st(L(2026, 11, 19, 9, 42)) + "|Sw|1|PDL|29221|V99+\n";      // a Sw: not a Bk
        wy += st(L(2026, 11, 19, 9, 43)) + "|Bk|1|PDL|abc|\n" + "garbage\n|||\n";
        wy += st(L(2026, 11, 19, 15, 0)) + "|Bk|1|PDH|29555|\n";          // after now: ignored
        const std::vector<Mark> M = parseWy(wy, 0.25, from, to);
        CHECK(M.size() == 2, "two Bk marks of this session");
        CHECK(M.size() == 2 && M[0].lv == "ONL" && M[1].lv == "PDL" && M[1].dir == -1 && std::fabs(M[1].px - 7305.25) < 1e-9 && M[1].st == "V99+", "sorted, level / side / price (ticks x tick) / strength");
        CHECK(parseWy(std::string(MAX_BYTES + 1, 'x'), 0.25, from, to).empty() && parseWy(wy, 0, from, to).empty(), "too big / no tick: nothing");
        CHECK(pri("PDL") == 10 && pri("2hL") == 60 && pri("PHH") == 50 && pri("DPoc") < 0 && pri("WPoc") < 0 && pri("Open") < 0 && pri("VWAP") < 0 && pri("CallRes") < 0 && pri("HVL") < 0, "the marks' level set");
        CHECK(shortName("LonLO") == "LLO" && shortName("AsiaHI") == "AHI" && shortName("PDL") == "PDL", "short names");
    }
    std::printf("[2] the Tst rules, live\n");
    const long long kT = L(2026, 11, 19, 10, 0) + 180LL * 5;   // the Bk candle (bar 4) end
    const std::vector<Mark> M1 = { bk(kT, -1, "PDL", 97) };
    {   const std::vector<Bar> B = bear(94.8, 96.5, 94.5, 96);   // back 4.5 of 11 = 41%, into the Bk candle (95..99), not past its high
        const Out o = compute(B, M1, {}, 96, 0.25);
        CHECK(o.tst.ok && o.tst.lv == "PDL" && o.tst.dir == -1 && o.tst.pct == 41, "testing PDL 41%: " + o.tst.why + " " + std::to_string(o.tst.pct));
        const std::string p = plain(segments(o));
        CHECK(p.rfind("[TESTING: PDL BkTst \xE2\x86\x93]", 0) == 0, "starts with the TESTING badge: " + p);
        CHECK(p.find("Bk / Sw") == std::string::npos, "one WATCH badge at a time (18:47): " + p);
    }
    {   const Out o = compute(bear(94.8, 96.5, 94.5, 96), { bk(kT, 1, "PDL", 97), bk(kT, -1, "PDL", 97) }, {}, 96, 0.25);   // same candle both ways: the last is the dir
        CHECK(o.haveBk, "a Bk");
    }
    auto why = [&](const std::vector<Bar>& B, const std::vector<Mark>& M, double live) { const Out o = compute(B, M, {}, live, 0.25); return o.tst.ok ? std::string("TESTING") : o.tst.why; };
    CHECK(why(bear(94.8, 94.8, 91, 91.5), M1, 91.5) == "still extending", "a new low on the forming candle: still extending");
    CHECK(why(bear(94.8, 99.5, 94.5, 99), M1, 99) == "took out the Bk candle high", "past the Bk candle's high: back to the Bk / Sw badge");
    {   std::vector<Bar> B = bear(94.8, 94.9, 94.5, 94.6); B[7].h = 94.6;   // heading back, short of the Bk candle (95..99): WATCH, gold
        const Out o = compute(B, M1, {}, 94.6, 0.25);
        CHECK(o.tst.ok && o.tst.state == 1 && o.tst.lv == "PDL", "short of the Bk candle: WATCH");
        CHECK(plain(segments(o)).rfind("[WATCH: PDL BkTst Rev \xE2\x86\x93]", 0) == 0, "WATCH: PDL BkTst Rev down: " + plain(segments(o)));
        int gold = 0; for (const Item& it : layout(segments(o), MS, 500, 300, 0, 1200, 10).items) if (it.k == 0 && it.fill == C_GOLD) gold++;
        CHECK(gold == 1, "a WATCH badge is gold (18:53)");
        B.back().h = 95.25; B.back().c = 95.25; const Out o1 = compute(B, M1, {}, 95.25, 0.25);   // (2.37.1, 22:46 CT) into the Bk candle but ~20% back: still WATCH
        CHECK(o1.tst.ok && o1.tst.state == 1 && o1.tst.pct < 38 && plain(segments(o1)).rfind("[WATCH: PDL BkTst Rev", 0) == 0, "2.37.1: into the Bk candle under 38.2% back: still the gold WATCH: " + plain(segments(o1)));
        B.back().h = 98.5; B.back().c = 98.5; const Out o2 = compute(B, M1, {}, 98.5, 0.25);   // 38.2%+ back and in the Bk candle: TESTING
        CHECK(o2.tst.ok && o2.tst.state == 2 && o2.tst.pct >= 38 && plain(segments(o2)).rfind("[TESTING: PDL BkTst", 0) == 0, "WATCH -> TESTING once 38.2%+ back with the Bk candle overlapped: pct " + std::to_string(o2.tst.pct));
        int red = 0; for (const Item& it : layout(segments(o2), MS, 500, 300, 0, 1200, 10).items) if (it.k == 0 && it.fill == C_RED) red++;
        CHECK(red == 1, "TESTING bearish = red");
    }
    {   std::vector<Bar> B = bear(91.2, 91.8, 91.1, 91.6); B[7].o = 92.5; B[7].l = 91; B[7].c = 91.2; B[7].h = 92.6;   // the extreme on the last closed candle
        CHECK(why(B, M1, 91.6) == "just topped", "a candle after the extreme must close first: " + why(B, M1, 91.6));
    }
    {   const Out o = compute(bear(94.8, 96.5, 94.5, 96, 110), M1, {}, 96, 0.25);   // (2.37.1, 22:46 CT) 25% back into the Bk candle: under 38.2% = WATCH (2.36.0 said TESTING)
        CHECK(o.tst.ok && o.tst.state == 1 && o.tst.pct == 25, "2.37.1: 25% back, into the Bk candle: WATCH, not TESTING");
        for (double h : { 97.5, 98.0, 98.25, 98.75 }) { const Out w = compute(bear(94.8, h, 94.5, h, 110), M1, {}, h, 0.25);
            CHECK(w.tst.ok && w.tst.state == 1 && w.tst.pct <= 38, "under 38.2% (rounded " + std::to_string(w.tst.pct) + "%) in the Bk candle: WATCH"); }
        for (double h : { 98.9, 99.0 }) { const Out w = compute(bear(94.8, h, 94.5, h, 110), M1, {}, h, 0.25);
            CHECK(w.tst.ok && w.tst.state == 2 && w.tst.pct >= 38 && w.tst.pct <= 100, "38.2%+ (" + std::to_string(w.tst.pct) + "%) in the Bk candle, not past its high: TESTING"); }
    }
    {   std::vector<Bar> B = bear(94.8, 96.5, 94.5, 96); B[5].o = 93; B[5].c = 95.8;   // the candle after the Bk is green: no FT within 3
        B[6].o = 95.8; B[6].c = 95.5; B[7].o = 95.5; B[7].c = 95.6;
        CHECK(why(B, M1, 96) == "no FT after the break", "no FT: no test");
    }
    {   std::vector<Bar> B = bear(0, 0, 0, 0); B.pop_back();   // the pullback turned: a lower high confirmed before the forming candle
        const long long t = B.back().t; Bar b8; b8.t = t + 180; b8.o = 94.8; b8.h = 97; b8.l = 94.5; b8.c = 96.8; Bar b9 = b8; b9.t += 180; b9.o = 96.8; b9.h = 97.5; b9.l = 95; b9.c = 97;
        Bar b10 = b9; b10.t += 180; b10.o = 97; b10.h = 97.2; b10.l = 92.3; b10.c = 92.4; Bar f = b10; f.t += 180; f.o = f.h = f.l = f.c = 92.4;
        B.push_back(b8); B.push_back(b9); B.push_back(b10); B.push_back(f);
        CHECK(why(B, M1, 92.4) == "the pullback already turned", "a confirmed pivot after the pullback: the test is over: " + why(B, M1, 92.4));
    }
    {   std::vector<Mark> M = M1; M.push_back(bk(kT + 180LL * 3, 1, "PHL", 93));   // bar 7 (its range is up on the prior: a Bk)   // a Bk the other way since: that move's Bks are out
        const Out o = compute(bear(94.8, 96.5, 94.5, 96), M, {}, 96, 0.25);
        CHECK(!o.tst.ok && o.haveBk && o.bk.dir == 1, "a bullish Bk after: no bearish test; last Bk = bullish");
    }
    {   const Out o = compute(mirror(bear(94.8, 96.5, 94.5, 96)), { bk(kT, 1, "PDH", 103) }, {}, 104, 0.25);
        CHECK(o.tst.ok && o.tst.dir == 1 && o.tst.pct == 41, "bullish mirror: testing 41%");
        CHECK(plain(segments(o)).rfind("[TESTING: PDH BkTst \xE2\x86\x91]", 0) == 0, "bullish badge: " + plain(segments(o)));
        int green = 0, gold = 0; for (const Item& it : layout(segments(o), MS, 600, 300, 0, 1400, 10).items) if (it.k == 0) { green += it.fill == C_GREEN; gold += it.fill == C_GOLD; }
        CHECK(green == 1 && gold == 0, "a bullish TESTING = green (18:55)");
        const Out o2 = compute(mirror(bear(94.8, 96.5, 94.5, 96)), { bk(kT, 1, "PHH", 103) }, std::vector<Key>{ { "PFH", 115 }, { "PWL", 60 } }, 104, 0.25);
        CHECK(plain(segments(o2)) == "[TESTING: PHH BkTst \xE2\x86\x91] \xE2\x94\x82 Next \xE2\x86\x93 PWL \xC2\xB7 \xE2\x86\x91 PFH", "a bullish pullback testing a broken PHH: " + plain(segments(o2)));
    }
    {   std::vector<Bar> B = bear(94.8, 96.5, 94.5, 96); B[4].h = 98.5; B[4].l = 95; B[4].v = 5;   // Bk candle range 3.5 < the prior 4: not a Bk (the marks' BO candle rule)
        const Out o = compute(B, M1, {}, 96, 0.25);
        CHECK(!o.haveBk && !o.tst.ok && o.tst.why == "no break yet", "a Bk failing the candle rule is not used");
    }
    CHECK(!compute(bear(94.8, 96.5, 94.5, 96), {}, {}, NAN, 0.25).ok && !compute({}, M1, {}, 96, 0.25).ok, "no price / no bars: nothing");
    std::printf("[3] next levels\n");
    {   const std::vector<Key> K = { { "PDL", 90 }, { "DPoc", 95.5 }, { "Open", 96.5 }, { "VWAP+1", 96.25 }, { "HVL", 96.75 }, { "VAL", 99 }, { "LonHI", 104 }, { "PHL", 95.75 } };
        const Out o = compute(bear(94.8, 96.5, 94.5, 96), {}, K, 96, 0.25);
        CHECK(o.up.ok && o.up.name == "VAL" && o.dn.ok && o.dn.name == "PDL", "no POC / Open / VWAP / gamma; the key file's PHL is not used (hour levels come from the bars): " + o.up.name + " " + o.dn.name);
        CHECK(plain(segments(o)) == "[WATCH: PDL \xE2\x86\x93 \xC2\xB7 VAL \xE2\x86\x91 \xC2\xB7 Bk / Sw]", "no test: the gold badge carries the next levels, no Next part (18:50): " + plain(segments(o)));
        const Out o2 = compute(bear(94.8, 96.5, 94.5, 96), M1, std::vector<Key>{ { "PDL", 97 }, { "PFL", 97 }, { "VAL", 99 }, { "PWL", 90 } }, 96, 0.25);
        CHECK(o2.tst.ok && o2.up.name == "VAL", "the level being tested is not a next level (nor its twin PFL)");
        CHECK(plain(segments(o2)) == "[TESTING: PDL BkTst \xE2\x86\x93] \xE2\x94\x82 Next \xE2\x86\x93 PWL \xC2\xB7 \xE2\x86\x91 VAL", "the whole line - the Last Bk PDL is already named (18:50): " + plain(segments(o2)));
    }
    {   std::vector<Bar> B; const long long t0 = L(2026, 11, 19, 7, 0);
        for (int i = 0; i < 80; i++) { Bar b; b.t = t0 + 180LL * (i + 1); b.o = b.c = 100 + (i / 20) * 10; b.h = b.o + 1 + (i % 20 == 5 ? 2 : 0); b.l = b.o - 1 - (i % 20 == 7 ? 2 : 0); B.push_back(b); }
        auto find = [&](const std::vector<Key>& H, const char* n) { for (const Key& k : H) if (k.name == n) return k.px; return -1.0; };
        const std::vector<Key> H = hourKeys(B, L(2026, 11, 19, 11, 3)), H2 = hourKeys(B, L(2026, 11, 19, 10, 3));
        CHECK(find(H, "PHH") == 133 && find(H, "PHL") == 127 && find(H, "2hH") == 123 && find(H, "2hL") == 117 && find(H, "3hH") == 113 && find(H, "3hL") == 107, "PHH / PHL / 2h / 3h from the bars");
        CHECK(find(H2, "PHH") == 123 && find(H2, "2hL") == 107 && find(H2, "3hH") < 0, "never the session's first (partial) hour");
    }
    std::printf("[4] his real ES 11-19\n");
    std::vector<Bar> all; { std::stringstream f(rd("nl_es_1119_3m.csv")); std::string ln;
        while (std::getline(f, ln)) { const std::vector<std::string> p = split(ln, ','); if (p.size() < 6) continue; Bar b; b.t = atoll(p[0].c_str()); b.o = atof(p[1].c_str()); b.h = atof(p[2].c_str()); b.l = atof(p[3].c_str()); b.c = atof(p[4].c_str()); b.v = atof(p[5].c_str()); all.push_back(b); } }
    CHECK(all.size() > 400, "the bars fixture");
    std::vector<Key> keys; for (const std::string& ln : split(rd("nl_keys_es_1119.csv"), '\n')) { const std::vector<std::string> p = split(ln, '|'); if (p.size() > 2 && p[0] == "L") for (const std::string& n : split(p[1], '/')) keys.push_back(Key{ n, atof(p[2].c_str()) }); }
    const std::string wy = rd("nl_wy_es_1119.txt");
    const std::string wyPdl = rd("nl_wy_es_1119_pdl.txt");   // the same marks without the 11:03 PHL break
    auto atW = [&](const std::string& w, int hh, int mm, Out& o) {   // the moment the hh:mm candle closed: the next candle forming at its open
        const long long T = L(2026, 11, 19, hh, mm); std::vector<Bar> B;
        for (const Bar& b : all) { if (b.t <= T) B.push_back(b); else { Bar f = b; f.h = f.l = f.c = f.o; B.push_back(f); break; } }
        o = compute(B, parseWy(w, 0.25, L(2026, 11, 18, 17, 0), T), keys, B.back().c, 0.25); return plain(segments(o)); };
    auto at = [&](int hh, int mm, Out& o) { return atW(wy, hh, mm, o); };
    {   Out o; const std::string p = at(11, 33, o);
        CHECK(o.tst.ok && o.tst.lv == "PDL" && o.tst.dir == -1, "11:33 AM: testing the PDL break: " + p);
        CHECK(p == "[TESTING: PDL BkTst \xE2\x86\x93] \xE2\x94\x82 Next \xE2\x86\x93 2hL \xC2\xB7 \xE2\x86\x91 VAL \xE2\x94\x82 Last Bk \xE2\x86\x93 PHL", "11:33 line (the 11:03 PHL break is not named yet): " + p);
        CHECK(p.find("Bk / Sw") == std::string::npos && noNumbers(p), "11:33: one badge, no numbers");
    }
    {   Out o; const std::string p = at(10, 45, o);
        CHECK(!o.tst.ok, "10:45 AM: no test in progress (" + o.tst.why + ")");
        CHECK(p == "[WATCH: VAL \xE2\x86\x93 \xC2\xB7 ONL \xE2\x86\x91 \xC2\xB7 Bk / Sw]", "10:45 line: the gold badge only - the Last Bk ONL is in it (18:50): " + p);
        CHECK(p.find("Testing") == std::string::npos && noNumbers(p), "10:45: nothing for the test (18:41), no numbers");
    }
    {   Out o; at(11, 39, o); CHECK(!o.tst.ok && o.tst.why == "the pullback already turned", "11:39 AM (the BkTst Rev candle closed): the test is over");
        at(11, 6, o); CHECK(!o.tst.ok && o.haveBk && o.bk.lv == "PHL", "11:06 AM: the move just topped, last Bk PHL");
        at(10, 57, o); CHECK(!o.tst.ok && o.tst.why == "waiting for the FT", "10:57 AM: the PDL break's FT window is open: no WATCH yet");
    }
    {   // the WATCH -> TESTING transition after the 10:54 PDL break (18:52)
        Out o; std::string p = atW(wyPdl, 11, 12, o);
        CHECK(o.tst.state == 1 && p.rfind("[WATCH: PDL BkTst Rev \xE2\x86\x93]", 0) == 0, "PDL marks: 11:12 AM WATCH: PDL BkTst Rev down (gold): " + p);
        p = atW(wyPdl, 11, 21, o); CHECK(o.tst.state == 1 && o.tst.lv == "PDL", "11:21 AM still WATCH PDL: " + p);
        p = atW(wyPdl, 11, 24, o); CHECK(o.tst.state == 2 && p.rfind("[TESTING: PDL BkTst \xE2\x86\x93]", 0) == 0, "11:24 AM TESTING: PDL: " + p);
        p = atW(wyPdl, 11, 39, o); CHECK(!o.tst.ok && p.rfind("[WATCH: ", 0) == 0, "11:39 AM the pullback turned: on to the next WATCH: " + p);
        // with the 11:03 PHL break too: the nearest break ahead is watched first, then each is tested in turn
        p = at(11, 12, o); CHECK(o.tst.state == 1 && o.tst.lv == "PHL", "all marks: 11:12 AM WATCH: PHL (the nearest break ahead): " + p);
        p = at(11, 21, o); CHECK(o.tst.state == 1 && o.tst.lv == "PHL" && o.tst.pct < 38, "2.37.1: 11:21 AM into the PHL Bk candle but " + std::to_string(o.tst.pct) + "% back (< 38.2%): WATCH: PHL (2.36.0 said TESTING): " + p);
        p = at(11, 24, o); CHECK(o.tst.state == 2 && o.tst.lv == "PDL", "11:24 AM TESTING: PDL (the furthest-back break reached): " + p);
    }
    {   // (18:54) the live BREAK / SWEEP of a watched level by the forming candle (the hh:mm candle forming, at its close values)
        auto live = [&](int hh, int mm, Out& o) { const long long T = L(2026, 11, 19, hh, mm); std::vector<Bar> B; for (const Bar& b : all) { if (b.t <= T) B.push_back(b); else break; }
            o = compute(B, parseWy(wy, 0.25, L(2026, 11, 18, 17, 0), T - 180), keys, B.back().c, 0.25); return plain(segments(o)); };
        Out o; std::string p = live(9, 42, o);
        CHECK(o.bs.state == 2 && o.bs.dir == 1 && p == "[SWEEP: PDL \xE2\x86\x91] \xE2\x94\x82 Next \xE2\x86\x91 VAL \xE2\x94\x82 Last Bk \xE2\x86\x93 ONL", "9:42 AM forming: SWEEP: PDL up | Next up VAL: " + p);
        int green = 0; for (const Item& it : layout(segments(o), MS, 600, 300, 0, 1400, 10).items) if (it.k == 0 && it.fill == C_GREEN) green++;
        CHECK(green == 1, "a swept low = green");
        p = live(10, 54, o);
        CHECK(o.bs.state == 1 && o.bs.dir == -1 && p == "[BREAK: PDL \xE2\x86\x93] \xE2\x94\x82 Last Bk \xE2\x86\x93 ONL", "10:54 AM forming: BREAK: PDL down (no named level below it): " + p);
        int red = 0; for (const Item& it : layout(segments(o), MS, 600, 300, 0, 1400, 10).items) if (it.k == 0 && it.fill == C_RED) red++;
        CHECK(red == 1, "a break down = red");
        at(10, 57, o); CHECK(o.bs.state == 0, "the next candle: the break is settled, the line moves on");
        p = live(8, 33, o);   // bullish: the 8:33 candle through the nearest level above the 8:30 close (the 2hL of the bars, 7,416.75)
        CHECK(o.bs.state == 1 && o.bs.dir == 1 && p.rfind("[BREAK: 2hL \xE2\x86\x91] \xE2\x94\x82 Next \xE2\x86\x91 2hH", 0) == 0, "8:33 AM forming: BREAK up (green): " + p);
        green = 0; for (const Item& it : layout(segments(o), MS, 600, 300, 0, 1400, 10).items) if (it.k == 0 && it.fill == C_GREEN) green++;
        CHECK(green == 1, "a bullish break = green (18:55)");
    }
    {   // synthetic: the level above swept (red) / broken (green); through it but the wrong colour = nothing; TESTING outranks a break
        std::vector<Bar> B = bear(94.8, 94.9, 94.5, 94.6); B[7].h = 94.6; const std::vector<Key> K = { { "VAL", 95.0 }, { "PWL", 80 } };
        B.back().h = 95.25; B.back().c = 94.5; Out o = compute(B, {}, K, 94.5, 0.25);
        CHECK(o.bs.state == 2 && o.bs.dir == -1 && plain(segments(o)).rfind("[SWEEP: VAL \xE2\x86\x93]", 0) == 0, "a swept high = SWEEP down: " + plain(segments(o)));
        B.back().c = 95.2; o = compute(B, {}, K, 95.2, 0.25);
        CHECK(o.bs.state == 1 && o.bs.dir == 1 && plain(segments(o)).rfind("[BREAK: VAL \xE2\x86\x91]", 0) == 0, "closing above it in a green candle = BREAK up: " + plain(segments(o)));
        B.back().o = 95.24; o = compute(B, {}, K, 95.2, 0.25);
        CHECK(o.bs.state == 0, "above it but a red candle: no break");
        B = bear(94.8, 96.5, 94.5, 96); o = compute(B, M1, std::vector<Key>{ { "VAL", 95.5 } }, 96, 0.25);
        CHECK(o.tst.state == 2 && o.bs.state == 1 && plain(segments(o)).rfind("[TESTING: PDL", 0) == 0, "TESTING outranks a live break: one badge");
    }
    {   // (19:02-19:04) no Cob states, no REV state: after the BkTst Rev the line moves on to the next WATCH
        Out o; std::string p = at(11, 39, o);
        CHECK(p.rfind("[WATCH: ", 0) == 0 && p.find("REV") == std::string::npos, "11:39 AM (the BkTst Rev candle): the next WATCH, no REV: " + p);
        p = at(10, 21, o); CHECK(p.find("Cob") == std::string::npos, "10:21 AM (the Cob Rev candle): no Cob state: " + p);
        CHECK(rd("../HodLod.cpp").find("CobSignal\") + HL_SEP + S->mkt") == std::string::npos, "the Now line reads nothing from CobSignal");
    }
    std::printf("[5] layout\n");
    {   Out o; at(11, 33, o); const std::vector<Seg> S = segments(o);
        const Draw D = layout(S, MS, 700, 500, 100, 1300, 10);
        CHECK(D.ok && D.level == 0 && D.shown == D.pieces && D.b == 500 && D.t == 482 && D.l >= 100 && D.r <= 1300 && std::abs((D.l + D.r) / 2 - 700) <= 1, "fits, centred, its bottom on the anchor");
        int fills = 0, red = 0, gold = 0, arrows = 0, seps = 0; bool inside = true, cp = true, blackBold = true;
        for (const Item& it : D.items) {
            if (it.l < D.l || it.r > D.r + 1 || it.t < D.t || it.b > D.b) inside = false;
            if (it.k == 0 && it.fill != C_BG) { fills++; red += it.fill == C_RED; gold += it.fill == C_GOLD; }
            if (it.k == 2 && it.col == C_SEP) seps++; else if (it.k == 2) arrows++;
            if (it.k == 1) { for (unsigned char c : it.s) if (c >= 0x80 && c != 0xB7) cp = false; }
        }
        { int bt = 0; for (const Item& it : D.items) if (it.k == 1 && it.col == C_BLACK) { bt++; if (!it.bold) blackBold = false; } if (bt != 1) blackBold = false; }   // the badge text: black bold
        { size_t lastFill = 0, firstText = D.items.size(); for (size_t q = 0; q < D.items.size(); q++) { if (D.items[q].k == 0) lastFill = q; if (D.items[q].k == 1 && q < firstText) firstText = q; } CHECK(lastFill < firstText, "the fills are drawn before any text"); }
        CHECK(fills == 1 && red == 1 && gold == 0, "one filled badge, red (bearish BkTst)");
        CHECK(blackBold, "badge text black bold");
        CHECK(arrows == 4 * 3 && seps == 2, "4 arrows as 3 strokes each, 2 separators");
        CHECK(inside && cp, "inside the box; cp1252 only (no UTF-8 in drawn text)");
        int w0 = D.r - D.l;
        const Draw D1 = layout(S, MS, 700, 500, 400, 400 + w0 - 1, 10);
        bool hasLast = false; for (const Item& it : D1.items) if (it.k == 1 && it.s == "Last Bk") hasLast = true;
        CHECK(D1.ok && D1.level == 1 && !hasLast && D1.r - D1.l < w0, "1 px short: the Last Bk part drops first");
        const int x2 = 400 + (D1.r - D1.l) - 30; const Draw D2 = layout(S, MS, 700, 500, 400, x2, 10);
        CHECK(D2.ok && D2.r <= x2 && D2.shown < D1.shown && D2.shown >= 1, "narrower: pieces off the right end (" + std::to_string(D2.shown) + " of " + std::to_string(D1.shown) + ")");
        bool dangling = false; { std::vector<Seg> v = fit(S, 1); v.resize((size_t)D2.shown); dangling = !v.empty() && v.back().sep; }
        CHECK(!dangling, "never a dangling separator");
        CHECK(!layout(S, MS, 700, 500, 400, 430, 10).ok && !layout({}, MS, 700, 500, 0, 900, 10).ok, "no room / nothing: no line");
        Out o2; at(10, 45, o2); const Draw D3 = layout(segments(o2), MS, 700, 500, 0, 1400, 10);
        int g2 = 0; for (const Item& it : D3.items) if (it.k == 0 && it.fill == C_GOLD) g2++;
        CHECK(D3.ok && g2 == 1, "10:45: ONE gold badge across both levels");
    }
    std::printf("[6] versions\n");
    CHECK(std::string(hl::HL_VERSION) == "2.37.2" && std::string(hlset::PLUGIN_VERSION) == "2.37.2", "2.37.1 (version strings updated in 2.37.1)");
    {   const std::string s = rd("../HodLod.cpp"); CHECK(s.find("p->setVersion(\"2.37.2\")") != std::string::npos && s.find("#include \"NowLevels.h\"") != std::string::npos, "setVersion 2.37.1 + NowLevels.h included"); }
    {   const std::string s = rd("../../tools/gex-build.bat"); const size_t a = s.find("call :one HodLod "); CHECK(a != std::string::npos && s.find("NowLevels.h", a) < s.find('\n', a), "NowLevels.h on the HodLod build line"); }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
