// test_volrange_logic.cpp - lsVolRead / lsRangeRead 1.0.0 logic suite (SDK-free).
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_volrange_logic.cpp -o tv && ./tv ../tests_cobsignal/cobsignal_es.csv [dump.txt]
#include "VolRangeLogic.h"
#include <cctype>
// (1.1.1) BreakoutLogic.h bko::recFileKey / bko::recMarket, copied verbatim on 2026-11-15 (lines 1161-1178): the names lsBreakout really writes
namespace bko {
inline std::string recFileKey(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) { const unsigned char c = (unsigned char)s[i];
        out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_'); }
    return out.empty() ? "UNKNOWN" : out;
}
inline std::string recMarket(std::string r) {
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return recFileKey(r.substr(0, 8));
}
}
#include <fstream>
#include <iostream>
static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
static vrr::Bar mk(double o, double c, double v, double h = 0, double l = 0) { vrr::Bar b; b.o = o; b.c = c; b.v = v; b.h = h ? h : std::max(o, c); b.l = l ? l : std::min(o, c); return b; }
int main(int argc, char** argv) {
    {   vrr::Engine E;   // volumes: G5 R3 G4 R9 G2 G6 R1 G8 ; the last bar G10
        const double V[] = { 5, 3, 4, 9, 2, 6, 1, 8 }; const bool G[] = { true, false, true, false, true, true, false, true };
        for (int i = 0; i < 8; ++i) E.addBar(mk(10, G[i] ? 11 : 9, V[i]));
        CHECK(E.S[7].all == 3 && E.S[7].same == 4 && E.S[7].green, "G8: 3 bars in a row smaller (1, 6, 2 - stops at 9); all 4 green bars before it are smaller (" << E.S[7].all << ", " << E.S[7].same << ")");
        E.addBar(mk(10, 11, 10)); CHECK(E.S[8].all == 8 && E.S[8].same == 5 && E.S[8].mx() == 8, "G10 beats all 8 bars and all 5 green bars (" << E.S[8].all << ", " << E.S[8].same << ")");
        E.addBar(mk(10, 9, 9)); CHECK(E.S[9].all == 0 && E.S[9].same == 1, "R9 after G10: 0 in a row; red bars: 1 is smaller, the earlier R9 is equal and stops the count (" << E.S[9].all << ", " << E.S[9].same << ")");
        E.addBar(mk(10, 9, 10)); CHECK(E.S[10].all == 1 && E.S[10].same == 4, "an equal bar stops the count (G10 = 10): all 1, red 4 (" << E.S[10].all << ", " << E.S[10].same << ")");
        E.addBar(mk(10, 10, 0)); CHECK(E.S[11].mx() == 0 && E.S[11].green, "a zero bar counts nothing; close = open is green");
        const vrr::Stat f = E.forming(mk(10, 11, 99)); CHECK(f.all == 12 && E.S.size() == 12, "the forming bar is measured, never stored");
    }
    {   vrr::Engine E; for (int i = 0; i < 300; ++i) E.addBar(mk(10, 11, 1 + (i % 7))); E.addBar(mk(10, 11, 50)); CHECK(E.S.back().all == vrr::CAP && E.S.back().same == vrr::CAP && vrr::fm(E.S.back().mx()) == "99+", "counts stop at 99 -> 99+"); CHECK(vrr::fm(98) == "98", "98 prints as 98"); }
    {   vrr::Engine E; E.mode = vrr::RANGE; E.tick = 0.25;
        E.addBar(mk(10, 11, 1, 11.0, 10.0)); E.addBar(mk(10, 9, 1, 10.5, 9.0)); E.addBar(mk(10, 11, 1, 11.5, 10.0 - 1e-9));
        CHECK(E.V[0] == 4 && E.V[1] == 6 && E.V[2] == 6, "range in whole ticks (" << E.V[0] << ", " << E.V[1] << ", " << E.V[2] << ")"); CHECK(E.S[2].all == 0 && E.S[2].same == 1, "equal ranges do not count (" << E.S[2].all << ", " << E.S[2].same << ")"); }
    {   // (1.1.0) the Range pane highlight
        std::map<long long, vrr::Hl> m;
        vrr::parseBo("# head\nBBO|t|t0|dir|fired|reason\nBBO|900|800|1|1|fired|x\r\nBBO|950|850|-1|0|not_10bar_extreme|x\nBBO|990|870|-1|1|fired\nBBX|1|2|1|1\nBBO|bad|x|1|1\n", m);
        CHECK(m.size() == 2 && m[800].dir == 1 && m[800].kind == vrr::HL_BO && m[870].dir == -1 && !m.count(850), "BO file: fired breakouts only, keyed by the BO candle (" << m.size() << ")");
        vrr::parsePbRev("# BRV|t_rev|side\nBRV|1200|1|1100|x\nBRV|1300|-1|1250\nBPB|1|2\nBRV|x|1\n", m); CHECK(m.size() == 4 && m[1200].dir == 1 && m[1300].dir == -1 && m[1300].kind == vrr::HL_PBREV, "pb file: every Rev bar");
        std::map<long long, vrr::Hl> c; vrr::parseCobRev("COB,5,BULL,Cob\nREV,7000,BULL,6000,0.21,0.30,1\nREV,8000,BEAR,7500,0.4,0.1,2\nREV,x,BULL\nREV,9000,UP\n", c); CHECK(c.size() == 2 && c[7000].dir == 1 && c[8000].dir == -1 && c[8000].kind == vrr::HL_COBREV, "CoB record: REV lines by local seconds");
        vrr::Engine E; E.mode = vrr::RANGE; E.tick = 0.25; E.hlOn = true;
        auto bar = [&](long long t, double o, double h, double l, double cl) { vrr::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = cl; b.v = 1; E.addBar(b, t + 5); };
        for (int i = 0; i < 20; ++i) bar(100 + i, 10, 11, 10, 10.5);             // 20 bars of range 1 -> ATR(13) = 1
        CHECK(std::fabs(E.ATR[12] - 1.0) < 1e-9 && std::fabs(E.ATR[19] - 1.0) < 1e-9 && E.ATR[11] == 0, "Wilder ATR(13): ready from the 13th bar");
        bar(120, 10, 11.19, 10, 11); bar(121, 10, 11.3, 10, 11); bar(122, 10, 11, 10, 10.5); bar(123, 10, 10.5, 10, 10.2); bar(124, 10, 10.4, 10, 10.1); bar(125, 10, 13, 10, 10.2);
        E.hlRaw[120] = vrr::Hl{ 1, vrr::HL_BO }; E.hlRaw[121] = vrr::Hl{ -1, vrr::HL_BO }; E.hlRaw[123] = vrr::Hl{ -1, vrr::HL_PBREV }; E.hlLocal[124 + 5] = vrr::Hl{ 1, vrr::HL_COBREV };
        E.hlRaw[125] = vrr::Hl{ 1, vrr::HL_BO }; E.hlLocal[125 + 5] = vrr::Hl{ -1, vrr::HL_COBREV };
        CHECK(E.highlight(20) == 0, "a BO of 1.19x ATR stays grey"); CHECK(E.highlight(21) == -1, "a BD of 1.2x ATR or more is red (the signal's side, not the candle's)");
        CHECK(E.highlight(22) == 0, "an ordinary bar is grey"); CHECK(E.highlight(23) == -1, "a PB Rev bar is always coloured, whatever its range"); CHECK(E.highlight(24) == 1, "a CoB pullback Rev bar is coloured (matched by local time)");
        CHECK(E.highlight(25) == -1, "a Rev wins over a BO on the same bar");
        vrr::View v; v.l = 0; v.t = 0; v.r = 600; v.b = 80; v.bw = 6; v.first = 0; v.last = 26; v.X = [](int j) { return 110.0 + j * 8.0; };
        int grey = 0, up = 0, dn = 0; for (const vrr::Prim& p : vrr::layout(E, v, nullptr)) if (p.fill) { if (p.col == vrr::C_GREY) ++grey; else if (p.col == vrr::C_UP) ++up; else if (p.col == vrr::C_DN) ++dn; }
        CHECK(grey == 22 && up == 4 && dn == 0, "Range pane: grey bars, only the signal bars coloured, each in its CANDLE's colour (1.3.3: all four are up candles) (" << grey << " grey, " << up << " green, " << dn << " red)");
        E.hlOn = false; int g2 = 0; for (const vrr::Prim& p : vrr::layout(E, v, nullptr)) if (p.fill && p.col == vrr::C_GREY) ++g2; CHECK(g2 == 0, "highlight off (the Volume pane): candle colours as before");
        E.hlOn = true; CHECK(vrr::statusText(E, "ES", "ESZ6", 180).find("\nHIGHLIGHT,1.1.0") != std::string::npos, "status: highlight line"); }
    if (argc < 2) { std::cerr << "fixture path needed\n"; return 2; }
    std::vector<vrr::Bar> B; { std::ifstream f(argv[1]); std::string l; std::getline(f, l); while (std::getline(f, l)) { vrr::Bar b; if (std::sscanf(l.c_str(), "%lld,%lf,%lf,%lf,%lf,%lf", &b.t, &b.o, &b.h, &b.l, &b.c, &b.v) == 6) B.push_back(b); } }
    CHECK(B.size() > 5000, "fixture loaded (" << B.size() << ")");
    for (int mode = 0; mode < 2; ++mode) {
        vrr::Engine E; E.mode = mode; E.tick = 0.25; for (const vrr::Bar& b : B) E.addBar(b);
        bool same = true; int big = 0;   // brute force
        for (int i = 0; i < (int)B.size(); i += 7) { const double x = E.V[(size_t)i]; const bool g = E.G[(size_t)i] != 0; int a = 0, s = 0;
            for (int j = i - 1; j >= 0 && E.V[(size_t)j] < x && a < vrr::CAP; --j) ++a;
            for (int j = i - 1; j >= 0 && s < vrr::CAP; --j) { if ((E.G[(size_t)j] != 0) != g) continue; if (E.V[(size_t)j] >= x) break; ++s; }
            if (!(x > 0)) a = s = 0;
            same = same && a == E.S[(size_t)i].all && s == E.S[(size_t)i].same; }
        for (const vrr::Stat& s : E.S) if (s.mx() >= vrr::SHOW_MIN) ++big;
        CHECK(same, (mode ? "Range" : "Volume") << ": counts = brute force on the ES fixture"); CHECK(big > 20 && big < (int)B.size() / 4, (mode ? "Range" : "Volume") << ": a sensible share of bars is numbered (" << big << " of " << B.size() << ")");
        {   vrr::Engine P; P.mode = mode; P.tick = 0.25; const size_t cut = B.size() / 2; for (size_t i = 0; i < cut; ++i) P.addBar(B[i]); bool pre = true; for (size_t i = 0; i < cut; ++i) pre = pre && P.S[i].all == E.S[i].all && P.S[i].same == E.S[i].same; CHECK(pre, "no repaint: a prefix run = the full run's prefix"); }
        vrr::View v; v.l = 0; v.t = 0; v.r = 1400; v.b = 90; v.bw = 5; v.last = (int)E.V.size(); v.first = v.last - 220; const int f0 = v.first; v.X = [f0](int j) { return 110.0 + (j - f0) * 5.8; };
        const vrr::Bar form = B.back(); const std::vector<vrr::Prim> P = vrr::layout(E, v, &form);
        int bars = 0, nums = 0, boxTxt = 0; bool in = true, order = true; std::vector<std::string> keys; std::string title;
        for (const vrr::Prim& p : P) { in = in && p.x0 >= v.l && p.x1 <= v.r && p.y0 >= v.t && p.y1 <= v.b; if (p.fill) { if (p.col == vrr::C_UP || p.col == vrr::C_DN) ++bars; } else if (p.x1 <= 2 + vrr::BOX_W) { ++boxTxt; if (p.text.rfind("B", 0) == 0) keys.push_back(p.text); else if (p.text.find(" v") != std::string::npos) title = p.text; } else { ++nums; order = order && (p.text == "99+" || std::atoi(p.text.c_str()) >= vrr::SHOW_MIN) && p.x0 > 2 + vrr::BOX_W; } }
        const std::string pre = mode ? "BR" : "BV";
        { bool clear = true; for (size_t i = 0; i < P.size(); ++i) for (size_t k = i + 1; k < P.size(); ++k) if (!P[i].fill && !P[k].fill && P[i].x0 > 2 + vrr::BOX_W && P[k].x0 > 2 + vrr::BOX_W && P[i].x0 < P[k].x1 && P[k].x0 < P[i].x1 && P[i].y0 < P[k].y1 && P[k].y0 < P[i].y1) clear = false; CHECK(clear, "no number overlaps another"); }
        CHECK(in, "everything inside the pane"); CHECK(bars > 150, "bars drawn (" << bars << ")"); CHECK(nums > 0 && order, "numbers only for Max 20+, right of the box (" << nums << ")");
        CHECK(keys.size() == 4 && keys[0] == pre + "Max" && keys[1] == pre && keys[2] == pre + "G" && keys[3] == pre + "R" && boxTxt == 8, "box rows in his order: Max, " << pre << ", " << pre << "G, " << pre << "R");
        CHECK(title.empty(), "no title row in the box (" << title << ")");
        { int dash = 0; for (const vrr::Prim& p : P) if (!p.fill && p.text == "-") ++dash; CHECK(dash == 1, "the other colour's row shows -"); }
        vrr::View s2 = v; s2.r = 120; CHECK([&] { for (const vrr::Prim& p : vrr::layout(E, s2, &form)) if (!p.fill && p.text.rfind("B", 0) == 0) return false; return true; }(), "a narrow pane draws no box");
        vrr::View s3 = v; s3.b = 8; CHECK(vrr::layout(E, s3, &form).empty(), "a pane under 12 px draws nothing");
        vrr::Engine Z; Z.mode = mode; CHECK([&] { for (const vrr::Prim& p : vrr::layout(Z, v, nullptr)) if (p.fill && (p.col == vrr::C_UP || p.col == vrr::C_DN)) return false; return true; }(), "no bars: only the empty box");
        const std::string st = vrr::statusText(E, "ES", "ESZ6", 180); CHECK(st.rfind(std::string("VERSION,") + vrr::VERSION + "\nPANE," + (mode ? "Range" : "Volume"), 0) == 0 && st.find("\nLAST,") != std::string::npos, "status text");
        if (argc > 2) { std::ofstream d(std::string(argv[2]) + (mode ? ".range" : ".vol")); for (const vrr::Prim& p : P) d << (p.fill ? "F" : p.bold ? "B" : "T") << "," << p.x0 << "," << p.y0 << "," << p.x1 << "," << p.y1 << "," << p.col << "," << p.text << "\n"; }
    }
    {   // (1.1.1, independent review) lsBreakout's file-name parts are lsBreakout's own; an unreadable (zeroed) bar leaves the ATR alone
        const char* syms[] = { "ESZ6", "@ES#", "esz6", "GC Z6.cme", "", "A-very-long-symbol-name-beyond-24-characters", "NQ_Z6" };
        const char* roots[] = { "ES", "EP", "ENQ", "NQ", "CLE", "QM", "GCE", "CPE", "HG", "NGE", "QG", "EU6", "6E", "@ES", "XYZ", "" };
        bool same = true; for (const char* s : syms) same = same && vrr::boFileKey(s) == bko::recFileKey(s); for (const char* r : roots) same = same && vrr::boMarket(r) == bko::recMarket(r);
        CHECK(same, "boFileKey / boMarket = bko::recFileKey / bko::recMarket"); CHECK(vrr::boFileKey("@ES#") == "_ES_" && vrr::boMarket("EP") == "ES", "file key examples");
        vrr::Engine A, B2; A.mode = B2.mode = vrr::RANGE; A.tick = B2.tick = 0.25; double px = 100;
        for (int i = 0; i < 60; ++i) { vrr::Bar b; b.t = 1000 + i; b.o = px; b.h = px + 2; b.l = px - 2; b.c = px + (i % 2 ? 1 : -1); b.v = 10; A.addBar(b, 0); if (i == 30) { vrr::Bar z; z.t = b.t; z.o = z.h = z.l = z.c = 0; z.v = 0; B2.addBar(z, 0); } else B2.addBar(b, 0); px = b.c; }
        double worst = 0; for (size_t i = 32; i < A.ATR.size(); ++i) worst = std::max(worst, std::fabs(A.ATR[i] - B2.ATR[i]) / A.ATR[i]);
        CHECK(worst < 0.15, "an unreadable bar moves the ATR by under 15% (was: tens of times), worst " << worst); }
    {   // (1.2.0) the read box follows the picked bar: a closed bar's own counts, the forming bar's live counts, the last closed bar when nothing is picked
        vrr::Engine E; E.mode = vrr::VOL; E.tick = 0.25; const double vv[8] = { 10, 20, 5, 40, 3, 2, 30, 1 };
        for (int i = 0; i < 8; ++i) { vrr::Bar b; b.t = 100 + i; b.o = 100; b.c = i % 2 ? 99 : 101; b.h = 102; b.l = 98; b.v = vv[i]; E.addBar(b, 0); }
        vrr::Bar f; f.t = 200; f.o = 100; f.c = 101; f.h = 102; f.l = 98; f.v = 35;
        vrr::View v; v.l = 0; v.t = 0; v.r = 600; v.b = 120; v.first = 0; v.last = 8; v.bw = 8; v.X = [](int j) { return 150.0 + j * 10.0; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; };
        auto box = [&](int pick) { std::vector<std::string> r; bool in = false; for (const vrr::Prim& p : vrr::layout(E, v, &f, pick)) { if (p.fill) continue; if (p.text == "BVMax") in = true; if (in && r.size() < 8) r.push_back(p.text); } return r; };
        const std::vector<std::string> last = box(-1), b3 = box(3), b6 = box(6), fm = box(8), out = box(99);
        // rows: BVMax val, BV val, BVG val, BVR val. bar 3 (40, red): bigger than all 3 before -> BV 3, red bars before (20) -> BVR 1; bar 6 (30, green): BV 2 (2, 3), BVG 2 (5, 10 smaller; 10 < 30, 5 < 30 -> 3? green bars before: 3, 5, 10 all smaller) 
        CHECK(last.size() == 8 && b3.size() == 8, "the box has its four rows");
        CHECK(last[3] == "0" && last[7] == "0" && last[5] == "-", "nothing picked = the last closed bar (1 lot, red): BV 0, BVR 0, BVG -  (" << last[3] << " " << last[5] << " " << last[7] << ")");
        CHECK(b3[3] == "3" && b3[7] == "1" && b3[5] == "-", "bar 3 (40 lots, red): BV 3, BVR 1 (" << b3[3] << " " << b3[7] << ")");
        CHECK(b6[3] == "2" && b6[5] == "3" && b6[7] == "-", "bar 6 (30 lots, green): BV 2, BVG 3 (" << b6[3] << " " << b6[5] << ")");
        CHECK(fm[3] == "4" && fm[5] == "4" && fm[7] == "-", "the forming bar (35 lots, green): BV 4 (1, 30, 2, 3), BVG 4 (30, 3, 5, 10) (" << fm[3] << " " << fm[5] << ")");
        CHECK(out == last, "a pick outside the bars = the last closed bar"); }
    std::cout << "test_volrange_logic: " << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
