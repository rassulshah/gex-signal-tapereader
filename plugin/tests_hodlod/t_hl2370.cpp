// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2370.cpp -o t2370 && ./t2370   (from plugin\tests_hodlod)
// lsHodLod 2.37.0 (2026-11-19 19:29 CT, Rassul "ok.. do it", mockup hodlod-new-box-mockup.html; 19:33 "switch positions of sweeps and Cobs")
//  [1] the signal line (HodLodTop.h): Cob LEFT, Sw CENTRE, BkTst RIGHT, newest first in each; green / red, black bold text; crowding
//      11 pt -> 10 pt (never below) -> tighter spacing -> only then the OLDEST of the most crowded group drops; never overlapping, inside
//  [2] the box (hl::layoutAll): line 1 = the signal line, line 2 = market / Close / LTD / moves ... Norm?  v2.37.0; no Hrly / 3m / Day,
//      no gamma pill and no expiry inside the box; one box height for every content (the signal row is always there)
//  [3] the Now row's left part: the gamma pill at the box's left edge, then "0DTE Exp 3:00 PM"; never touching the Now line: the
//      expiry drops first, then the pill
//  [4] spoken sweep alerts (SweepVoice.h): parse (named levels, strength >= 10), spoken text / clip names, once-only, no replay on chart
//      start / reload, two charts of one market speak once, RTH gate, late marks quiet, colour rule, the queue, a missing clip = fallback
//  [5] his real ES 11-19 wy file: the one RTH Sw (8:42 AM PHL V99+) speaks "ES, sweep, PHL, up"; overnight / pre-RTH ones do not; the
//      9:42 AM PDL live SWEEP of the Now line is not a confirmed Sw mark in his file (no Sw line) - a V5 one would stay quiet, a V12 one speaks
//  [6] versions 2.37.0 everywhere + the build list
#include <cstdio>
#include <climits>
#include <fstream>
#include <sstream>
#include <set>
#include <map>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../SweepVoice.h"
#include "../NowLevels.h"
#include "lib_widths.h"
#include "scen.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; if (fails < 60) std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static double SC = 1.0;
static int MS(const std::string& s, int fs, bool b) { return (int)(measureLib(s, fs, b) * SC + 0.5); }
static std::string rd(const char* p) { std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
static long long L(int y, int mo, int d, int h, int mi) {   // local secs, days from 1970 (civil)
    y -= mo <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + (long long)doe - 719468) * 86400 + h * 3600 + mi * 60; }
static std::string st(long long t) { return std::to_string(t + svx::E1904); }
static bool ovl(const htop::It& a, const htop::It& b) { return a.l < b.r && b.l < a.r; }

int main() { (void)&fnv; (void)&itemsText;
    std::printf("[1] the signal line\n");
    auto ms = [](const std::string& s, int fs, bool b) { return MS(s, fs, b); };
    const char* TX[9] = { "ES Cob 3:18", "NQ BkTst PHL 2:21", "NQ Sw PHL 1:09", "ES BkTst PDL 11:39", "ES Sw PDL 9:42", "GC Sw PDL 9:30", "CL Cob 9:12", "HG Sw LonLO 8:51", "GC BkTst VAH 8:40" };
    const int DR[9] = { -1, -1, 1, -1, 1, 1, 1, 1, -1 };
    auto tagsOf = [&](int n) { std::vector<htop::Tag> v; for (int i = 0; i < n; i++) { htop::Tag t; t.text = TX[i]; t.dir = DR[i]; t.group = htop::groupOf(htop::typeOf(TX[i])); v.push_back(t); } return v; };
    CHECK(htop::typeOf("NQ BkTst PHL 2:21") == "BkTst" && htop::typeOf("ES Cob 3:18") == "Cob" && htop::groupOf("Cob") == 0 && htop::groupOf("Sw") == 1 && htop::groupOf("BkTst") == 2 && htop::groupOf("Bk") == -1, "types -> groups: Cob left, Sw centre, BkTst right (19:33)");
    int nNormal = 0, nSmall = 0, nTight = 0, nDrop = 0, scen1 = 0;
    for (int W : { 1400, 1100, 900, 760, 640, 520, 420, 300 }) for (int n : { 1, 3, 5, 6, 9 }) {
        const std::vector<htop::Tag> tg = tagsOf(n); const int x0 = 10, x1 = 10 + W;
        const htop::Sig S = htop::sigLayout(tg, ms, x0, x1, 100, 121);
        scen1++;
        CHECK(S.ok, "laid out W" + std::to_string(W) + " n" + std::to_string(n));
        if (!S.ok) continue;
        CHECK(S.fs >= 10 && S.fs <= 11, "font 11 or 10 pt, never below his 10 pt floor");
        if (S.fs == 11) nNormal++; else if (!S.tight) nSmall++; else nTight++;
        if (S.dropped) nDrop++;
        CHECK(S.dropped == 0 || (S.fs == 10 && S.tight), "a tag drops only at 10 pt with tight spacing");
        std::vector<htop::It> fills, texts; for (const htop::It& it : S.items) (it.k == 0 ? fills : texts).push_back(it);
        CHECK(fills.size() == texts.size() && (int)fills.size() == n - S.dropped, "one badge per tag kept");
        for (size_t i = 0; i < fills.size(); i++) { CHECK(fills[i].l >= x0 && fills[i].r <= x1, "inside the line");
            for (size_t j = i + 1; j < fills.size(); j++) CHECK(!ovl(fills[i], fills[j]), "badges never overlap");
            CHECK(texts[i].col == 0x00000000u && texts[i].bold && texts[i].fs == S.fs, "black bold text");
            CHECK(texts[i].l >= fills[i].l && texts[i].r <= fills[i].r, "text inside its badge"); }
        // groups: the shown ones in left -> right order Cob | Sw | BkTst, newest first inside each; the dropped are the oldest of their group
        std::map<int, std::vector<int> > byG; for (const htop::It& t : texts) { int k = -1; for (int i = 0; i < n; i++) if (t.s == TX[i]) k = i; CHECK(k >= 0, "known tag");
            const htop::It* f = nullptr; for (const htop::It& ff : fills) if (ff.l <= t.l && ff.r >= t.r) f = &ff;
            CHECK(f && f->col == (DR[k] > 0 ? 0x0022C55Eu : 0x00EF4444u), "fill = side colour"); byG[htop::groupOf(htop::typeOf(TX[k]))].push_back(k); }
        int lastR = INT_MIN; for (int g = 0; g < 3; g++) { if (!S.shown[g]) continue; CHECK(S.gl[g] >= lastR, "groups left to right: Cob, Sw, BkTst"); lastR = S.gr[g];
            const std::vector<int>& v = byG[g]; for (size_t i = 1; i < v.size(); i++) CHECK(v[i] > v[i - 1], "newest first inside a group");
            int all = 0; for (int i = 0; i < n; i++) if (htop::groupOf(htop::typeOf(TX[i])) == g) { if (all < S.shown[g]) CHECK(std::find(v.begin(), v.end(), i) != v.end(), "the kept ones are the newest of the group"); all++; } }
        if (S.shown[0]) CHECK(S.gl[0] == x0, "Cob left-aligned");
        if (S.shown[2]) CHECK(S.gr[2] == x1, "BkTst right-aligned");
        if (S.shown[1] && W >= 900 && n <= 5) CHECK(std::abs((S.gl[1] + S.gr[1]) / 2 - (x0 + x1) / 2) <= 2, "Sw centred when there is room");
    }
    CHECK(nNormal > 0 && nSmall > 0 && nTight > 0 && nDrop > 0, "all four crowding steps seen (11 pt " + std::to_string(nNormal) + ", 10 pt " + std::to_string(nSmall) + ", tight " + std::to_string(nTight) + ", drops " + std::to_string(nDrop) + ")");
    {   // borrowing: 4 Sw and nothing else - the centre group grows past its third into the empty neighbours' room, no drop
        std::vector<htop::Tag> v; for (const char* s : { "NQ Sw PHL 1:09", "ES Sw PDL 9:42", "GC Sw PDL 9:30", "HG Sw LonLO 8:51" }) { htop::Tag t; t.text = s; t.dir = 1; t.group = 1; v.push_back(t); }
        const htop::Sig S = htop::sigLayout(v, ms, 0, 560, 0, 21);
        CHECK(S.ok && S.dropped == 0 && S.shown[1] == 4 && S.gr[1] - S.gl[1] > 560 / 3, "a crowded group borrows its empty neighbours' width"); }
    {   // most crowded group loses its oldest first
        std::vector<htop::Tag> v; htop::Tag c; c.text = "ES Cob 3:18"; c.dir = -1; c.group = 0; v.push_back(c);
        for (const char* s : { "NQ Sw PHL 1:09", "ES Sw PDL 9:42", "GC Sw PDL 9:30", "HG Sw LonLO 8:51", "CL Sw PFL 8:41", "NG Sw ONL 8:12" }) { htop::Tag t; t.text = s; t.dir = 1; t.group = 1; v.push_back(t); }
        htop::Tag b; b.text = "NQ BkTst PHL 2:21"; b.dir = -1; b.group = 2; v.push_back(b);
        const htop::Sig S = htop::sigLayout(v, ms, 0, 640, 0, 21);
        CHECK(S.ok && S.dropped > 0 && S.shown[0] == 1 && S.shown[2] == 1 && S.shown[1] == 6 - S.dropped, "the crowded Sw group drops its oldest; Cob / BkTst keep theirs");
        bool hasNewest = false, hasOldest = false; for (const htop::It& it : S.items) { if (it.s == "NQ Sw PHL 1:09") hasNewest = true; if (it.s == "NG Sw ONL 8:12") hasOldest = true; }
        CHECK(hasNewest && !hasOldest, "newest kept, oldest dropped"); }
    CHECK(!htop::sigLayout(std::vector<htop::Tag>(), ms, 0, 500, 0, 21).items.size(), "no tags: an empty line");

    std::printf("[2] the box\n");
    auto yOf = [](long long) { return 300; };
    int scen = 0, heightsSame = 0; std::set<int> heights;
    for (int m = 0; m < 6; m++) for (int W : { 930, 1200, 1530, 1900 }) for (double sc : { 1.0, 1.3 }) for (int ntag : { 0, 3, 9 }) {
        SC = sc; const hl::Box pane = { 0, 0, W, 640 }; hl::View v = mkView(m, true); hl::Opts O;
        v.hdrLeft.clear(); v.hdrLeft.push_back(hl::Seg{ "Close 4:00 PM", hl::C_SLATE }); v.hdrLeft.push_back(hl::Seg{ "LTD Dec 18 (29d)", hl::C_SLATE });
        v.mkts.push_back(hl::View::MkSeg{ "NQ -9%", hl::C_RED, false, 0 }); v.mkts.push_back(hl::View::MkSeg{ "ES -3%", hl::C_RED, false, 0 });
        v.rangePill = hl::Seg{ "Norm?", hl::C_GREY };
        for (int i = 0; i < ntag; i++) v.cobs.push_back(hl::View::CobSeg{ "", "", DR[i], TX[i] });
        hl::Layout Lo = hl::layoutAll(v, pane, yOf, MS, (int)(W * 0.8), 2, -1, nullptr, &O);
        if (!Lo.tableOk) continue;
        scen++;
        std::string all; for (const hl::Item& it : Lo.items) if (it.role == hl::R_TABLE && it.k == hl::K_TEXT) all += it.s + "\n";
        CHECK(all.find("Hrly") == std::string::npos && all.find("Dly") == std::string::npos && all.find("3m:") == std::string::npos, "no Hrly / 3m / Day in the box");
        CHECK(all.find("GAMMA") == std::string::npos && all.find("Exp 3:00") == std::string::npos, "no gamma pill / expiry in the box (moved to the Now row)");
        CHECK(all.find("v2.37.2") != std::string::npos && all.find("Norm?") != std::string::npos, "line 2 right: Norm? + v2.37.1 (2.37.1: version strings updated)");
        CHECK(all.find("Close 4:00 PM") != std::string::npos && all.find("LTD Dec 18 (29d)") != std::string::npos, "line 2 left: Close + LTD");
        CHECK(Lo.sigRow.t == Lo.table.t + 1 && Lo.sigRow.b > Lo.sigRow.t, "line 1 = the signal row, at the box top");
        const hl::Item* ver = nullptr; const hl::Item* mk = nullptr; const hl::Item* cls = nullptr;
        for (const hl::Item& it : Lo.items) if (it.role == hl::R_TABLE && it.k == hl::K_TEXT) { if (it.s == "v2.37.2") ver = &it; if (it.s == "ES -3%" || it.s == "NQ -9%") mk = &it; if (it.s == "Close 4:00 PM") cls = &it; }
        CHECK(ver && cls && ver->t == cls->t && ver->t >= Lo.sigRow.b - 1, "line 2 is one line under the signal row");
        if (mk) CHECK(mk->t == cls->t && mk->l > cls->r, "market moves on line 2 after Close / LTD");
        for (const hl::Item& it : Lo.items) if (it.role == hl::R_TABLE && it.k == hl::K_TEXT && it.t >= Lo.sigRow.t && it.b <= Lo.sigRow.b + 1) {
            bool tag = false; for (int i = 0; i < 9; i++) if (it.s == TX[i]) tag = true;
            CHECK(tag, "only tags on the signal row (" + it.s + ")");
            CHECK(it.l >= Lo.table.l && it.r <= Lo.table.r && it.fs >= 10, "tag inside the box, >= 10 pt"); }
        // line 2 items never overlap each other
        std::vector<const hl::Item*> l2; for (const hl::Item& it : Lo.items) if (it.role == hl::R_TABLE && it.k == hl::K_TEXT && ver && it.t == ver->t) l2.push_back(&it);
        for (size_t i = 0; i < l2.size(); i++) for (size_t j = i + 1; j < l2.size(); j++) CHECK(!(l2[i]->l < l2[j]->r && l2[j]->l < l2[i]->r), "line 2 texts do not overlap (" + l2[i]->s + " / " + l2[j]->s + ")");
        if (ntag == 0) heights.insert(Lo.table.b - Lo.table.t); else heightsSame += 0;
        hl::Layout L0 = hl::layoutAll([&] { hl::View w = v; w.cobs.clear(); return w; }(), pane, yOf, MS, (int)(W * 0.8), 2, -1, nullptr, &O);
        if (L0.tableOk && L0.tableFs == Lo.tableFs && L0.swTier == Lo.swTier) { CHECK(L0.table.b - L0.table.t == Lo.table.b - Lo.table.t, "the box height does not change with the tags"); heightsSame++; }
    }
    CHECK(scen >= 100, "scenarios laid out: " + std::to_string(scen));
    CHECK(heightsSame > 50, "height checks: " + std::to_string(heightsSame));
    {   // same height as 2.36.0: the 2.36.0 box had the market strength row (rowH + 3) + the header band; dump compared by the harness
        SC = 1.0; const hl::Box pane = { 0, 0, 1530, 640 }; hl::View v = mkView(0, true); hl::Opts O;
        v.hdrLeft.clear(); v.hdrLeft.push_back(hl::Seg{ "Close 4:00 PM", hl::C_SLATE }); v.hdrLeft.push_back(hl::Seg{ "LTD Dec 18 (29d)", hl::C_SLATE });
        hl::Layout Lo = hl::layoutAll(v, pane, yOf, MS, 1200, 2, -1, nullptr, &O);
        std::FILE* f = std::fopen("h2370.txt", "w"); if (f) { std::fprintf(f, "%d %d %d\n", Lo.table.b - Lo.table.t, Lo.tableFs, Lo.swTier); std::fclose(f); } }

    std::printf("[3] the Now row's left part\n");
    auto m10 = [](const std::string& s, int fs, bool b) { return MS(s, fs, b); };
    CHECK(htop::zeroDte("Exp 3:00 PM") == "0DTE Exp 3:00 PM" && htop::zeroDte("Exp 3:00 PM done") == "0DTE Exp 3:00 PM done" && htop::zeroDte("").empty(), "0DTE prefix");
    {   const htop::Left a = htop::nowLeft("-GAMMA", hl::C_RED, hl::C_PILL_R, "0DTE Exp 3:00 PM", hl::C_SLATE, m10, 100, 50, 68, 600, 2000);
        CHECK(a.keep == 2 && a.items.size() == 3 && a.items[0].k == 3 && a.items[0].l == 100 && a.items[1].s == "-GAMMA" && a.items[2].s == "0DTE Exp 3:00 PM", "room: pill at the box's left edge, then the expiry");
        CHECK(a.items[2].l > a.items[0].r && a.r + 10 <= 600, "expiry right of the pill, clear of the Now line");
        const int need2 = a.r - 100;
        const htop::Left b = htop::nowLeft("-GAMMA", hl::C_RED, hl::C_PILL_R, "0DTE Exp 3:00 PM", hl::C_SLATE, m10, 100, 50, 68, 100 + need2 + 5, 2000);
        CHECK(b.keep == 1 && b.items.size() == 2 && b.r + 10 <= 100 + need2 + 5, "tight: the expiry drops first, the pill stays");
        const htop::Left c = htop::nowLeft("-GAMMA", hl::C_RED, hl::C_PILL_R, "0DTE Exp 3:00 PM", hl::C_SLATE, m10, 100, 50, 68, 120, 2000);
        CHECK(c.keep == 0 && c.items.empty(), "tighter: the pill goes too - the Now line is never covered");
        const htop::Left d = htop::nowLeft("", hl::C_GREY, 0, "0DTE Exp 3:00 PM", hl::C_SLATE, m10, 100, 50, 68, 1000000, 2000);
        CHECK(d.keep == 2 && d.items.size() == 1, "no regime known: the expiry alone");
        const htop::Left e = htop::nowLeft("+GAMMA", hl::C_GREEN, hl::C_PILL_G, "", hl::C_SLATE, m10, 100, 50, 68, 1000000, 2000);
        CHECK(e.keep == 1 && e.items.size() == 2 && e.items[1].col == hl::C_GREEN, "no expiry: the pill alone, green for +GAMMA"); }
    {   // with the real Now line (2.36.0 11:33 AM): centred over a 1,130 px box at x 0..1130, the pill + expiry fit left of it
        nowl::Out o; std::vector<nowl::Seg> sg;
        nowl::Seg b1; b1.s = "TESTING: PDL BkTst";   /* 2.37.1 wording */ b1.arrow = -1; b1.badge = true; b1.bold = true; b1.fill = nowl::C_RED; b1.col = nowl::C_BLACK; sg.push_back(b1);
        nowl::Seg sp; sp.sep = true; sg.push_back(sp);
        nowl::Seg n1; n1.s = "Next"; n1.col = nowl::C_GREY; sg.push_back(n1); nowl::Seg n2; n2.arrow = -1; n2.s2 = "2hL"; n2.col = nowl::C_GOLD; sg.push_back(n2);
        auto mn = [](const std::string& s, bool b) { return MS(s, 10, b); };
        const nowl::Draw D = nowl::layout(sg, mn, 565, 200, 0, 1400, 10);
        const htop::Left a = htop::nowLeft("-GAMMA", hl::C_RED, hl::C_PILL_R, "0DTE Exp 3:00 PM", hl::C_SLATE, m10, 0, D.t, D.b, D.l, 1400);
        CHECK(D.ok && a.keep == 2 && a.r < D.l && std::abs((D.l + D.r) / 2 - 565) <= 1, "mockup case: pill + expiry left, the Now line centred and untouched"); }

    std::printf("[4] spoken sweep alerts\n");
    CHECK(svx::spoken("NQ", "PDL", 1) == "N Q, sweep, P D L, up." && svx::text("NQ", "PDL", 1) == "NQ, sweep, PDL, up", "NQ, sweep, PDL, up");
    CHECK(svx::spoken("EU", "LHI", -1) == "Euro, sweep, London high, down." && svx::text("EU", "LHI", -1) == "6E, sweep, LonHI, down", "EU = Euro; LonHI = London high");
    CHECK(svx::levelSpoken("2hL") == "two hour low" && svx::levelSpoken("3hH") == "three hour high" && svx::levelSpoken("ALO") == "Asia low", "hour / Asia levels in words");
    CHECK(svx::levelSpoken("VAH") == "V eh H" && svx::levelSpoken("ONL") == "O N L" && svx::marketSpoken("GC") == "G C", "letters spelled (A as 'eh': Kokoro says a bare A as a schwa)");
    {   std::set<std::string> files; int n = 0; bool roman = false;
        for (const std::string& m : svx::markets()) for (const std::string& c : svx::levels()) for (int d : { 1, -1 }) { files.insert(svx::fileName(m, c, d)); n++;
            const std::string sp2 = svx::spoken(m, c, d); if (sp2.find("IV") != std::string::npos || sp2.find("II") != std::string::npos) roman = true;
            for (char ch : sp2) if (ch >= '0' && ch <= '9') roman = true; }
        CHECK(n == 308 && files.size() == 308, "308 clips: 7 markets x 22 levels x 2 sides, unique names");
        CHECK(!roman, "no digits / Roman-numeral-looking text in what is spoken"); }
    CHECK(svx::canon("LonLO") == "LLO" && svx::canon("PDL") == "PDL" && svx::canon("DP").empty() && svx::canon("Op").empty() && svx::canon("4hL").empty() && svx::canon("WP").empty(), "named levels only (no POC / Open)");
    CHECK(svx::strengthOf("V99+") == 99 && svx::strengthOf("R10") == 10 && svx::strengthOf("V9") == 9 && svx::strengthOf("") == 0 && svx::strengthOf("X50") == 0 && svx::strengthOf("V1e3") == 0, "strength parse");
    const long long d0 = L(2026, 11, 19, 0, 0);
    auto at = [&](int h, int mi) { return d0 + h * 3600 + mi * 60; };
    {   std::string wy = "WY|1|NQ|180|0.20.0\n";
        wy += st(at(9, 42)) + "|Sw|1|PDL|29221|V12\n" + st(at(9, 45)) + "|Sw|1|PHL|29221|V9\n" + st(at(9, 48)) + "|Sw|-1|PHH|1|\n" + st(at(9, 51)) + "|Sw|1|DP|1|V40\n"
            + st(at(9, 54)) + "|Bk|1|PDH|1|V40\n" + st(at(9, 57)) + "|Sw|2|PDL|1|V40\n" + st(at(10, 0)) + "|Sw|-1|LonHI|1|R99+\n" + "garbage|Sw|1|PDL|1|V40\n" + st(at(10, 3)) + "|Sw|1|PDL|1\n";
        const std::vector<svx::Sw> v = svx::parseSw(wy, at(0, 0), at(23, 0));
        CHECK(v.size() == 2 && v[0].lv == "PDL" && v[0].dir == 1 && v[0].st == 12 && v[1].lv == "LHI" && v[1].dir == -1, "only drawn Sw: named level, strength >= 10, 6 fields");
        CHECK(svx::parseSw("garbage\n" + wy, 0, at(23, 0)).empty(), "not a wy file: nothing");
        CHECK(svx::parseSw(wy, at(9, 43), at(23, 0)).size() == 1, "from bound"); }
    auto ok = [](long long, int) { return -1; };
    auto mk = [](long long t, int dir, const char* lv) { svx::Sw s; s.t = t; s.dir = dir; s.lv = lv; s.st = 50; return s; };
    {   svx::Hub H; const long long start = at(9, 0), now = at(9, 42) + 5;
        const std::vector<svx::Sw> one(1, mk(at(9, 42), 1, "PDL"));
        CHECK(H.offer("NQ", start, now, one, 510, 900, ok) == 1, "a new Sw after the chart started is queued");
        CHECK(H.offer("NQ", start, now + 1, one, 510, 900, ok) == 0, "once only: the same Sw again (next second)");
        CHECK(H.offer("NQ", at(9, 30), now + 2, std::vector<svx::Sw>(1, mk(at(9, 39), 1, "PDL")), 510, 900, ok) == 0, "two charts of NQ (another timeframe, another stamp, same level + side): spoken once");
        CHECK(H.offer("NQ", start, now, std::vector<svx::Sw>(1, mk(at(9, 45), -1, "PDL")), 510, 900, ok) == 1, "the other side of the same level is its own alert");
        CHECK(H.offer("ES", start, now, one, 510, 900, ok) == 1, "another market, same level: its own alert");
        int played = 0, fb = 0; std::vector<std::string> order;
        auto play = [&](const std::string& f) -> long long { order.push_back(f); played++; return f == "sw-ES-PDL-up.wav" ? -1 : 1800; };
        auto fall = [&]() -> long long { fb++; return 1200; };
        const long long t0 = 1000000;
        CHECK(H.pump(t0, play, fall) && order.back() == "sw-NQ-PDL-up.wav" && H.last == "NQ, sweep, PDL, up", "first clip plays");
        CHECK(!H.pump(t0 + 1000, play, fall), "queue: the next clip waits while one is talking");
        CHECK(H.pump(t0 + 1800 + svx::GAP_MS, play, fall) && order.back() == "sw-NQ-PDL-down.wav", "then the next, in order");
        CHECK(H.pump(t0 + 5000, play, fall) && fb == 1 && H.fallbacks == 1 && H.lastMissing == "sw-ES-PDL-up.wav", "a missing clip plays the system sound instead (counted)");
        CHECK(!H.pump(t0 + 5100, play, fall) && !H.pump(t0 + 20000, play, fall) && H.spoken == 3 && played == 3, "queue empty: nothing more"); }
    {   svx::Hub H; const long long now = at(9, 42) + 5;
        CHECK(H.offer("NQ", at(9, 42) + 2, now, std::vector<svx::Sw>(1, mk(at(9, 42), 1, "PDL")), 510, 900, ok) == 0, "a mark already there when the chart (re)loaded is never spoken");
        CHECK(H.offer("NQ", at(9, 42), now, std::vector<svx::Sw>(1, mk(at(9, 42), 1, "PDL")), 510, 900, ok) == 0, "a mark ending exactly at the start: not spoken");
        CHECK(H.offer("CL", at(5, 0), at(5, 34), std::vector<svx::Sw>(1, mk(at(5, 33), -1, "PHH")), 480, 810, ok) == 0 && H.skippedRth == 1, "RTH only: 5:33 AM quiet");
        CHECK(H.offer("ES", at(14, 0), at(15, 0) + 3, std::vector<svx::Sw>(1, mk(at(15, 0), 1, "PHL")), 510, 900, ok) == 1, "the last RTH candle (ends 3:00 PM) speaks");
        CHECK(H.offer("ES", at(14, 0), at(15, 3) + 3, std::vector<svx::Sw>(1, mk(at(15, 3), -1, "PHH")), 510, 900, ok) == 0, "3:03 PM: after RTH, quiet");
        CHECK(H.offer("ES", at(8, 0), at(9, 40), std::vector<svx::Sw>(1, mk(at(9, 30), 1, "VAL")), 510, 900, ok) == 0 && H.skippedOld == 1, "a mark first seen 10 min late stays quiet");
        auto wrong = [](long long, int) { return 0; }; auto right = [](long long, int) { return 1; };
        CHECK(H.offer("GC", at(8, 0), at(9, 42) + 5, std::vector<svx::Sw>(1, mk(at(9, 42), 1, "ONL")), 440, 750, wrong) == 0 && H.skippedColour == 1, "Sw colour rule: a red candle is not a bullish Sw");
        CHECK(H.offer("GC", at(8, 0), at(9, 45) + 5, std::vector<svx::Sw>(1, mk(at(9, 45), 1, "ONL")), 440, 750, right) == 1, "a later right-coloured Sw of that level still speaks");
        CHECK(H.offer("GC", at(8, 0), at(9, 45) + 6, std::vector<svx::Sw>(1, mk(at(9, 42), 1, "ONL")), 440, 750, wrong) == 0 && H.skippedColour == 1, "a wrong candle is counted once"); }
    {   svx::Hub H; const long long s5 = L(2026, 11, 19, 17, 0);
        CHECK(svx::sessionOf(s5 + 60) == s5 && svx::sessionOf(s5 - 60) == s5 - 86400, "session = 5 PM to 5 PM");
        CHECK(H.offer("NQ", at(9, 0), at(9, 42) + 5, std::vector<svx::Sw>(1, mk(at(9, 42), 1, "PDL")), 510, 900, ok) == 1, "today");
        CHECK(H.offer("NQ", at(9, 0) + 86400, at(9, 42) + 5 + 86400, std::vector<svx::Sw>(1, mk(at(9, 42) + 86400, 1, "PDL")), 510, 900, ok) == 1, "the next session says it again"); }
    {   svx::Hub H; std::vector<svx::Sw> many; for (int i = 0; i < 30; i++) many.push_back(mk(at(9, 0) + 180 * (i + 1), 1, svx::levels()[(size_t)i % 22].c_str()));
        for (int i = 0; i < 30; i++) many[(size_t)i].dir = i < 22 ? 1 : -1;
        H.offer("ES", at(9, 0), at(9, 0) + 180 * 31, many, 510, 900, ok);
        CHECK(H.q.size() <= svx::QUEUE_MAX, "the queue is bounded"); }
    {   const std::string w = rd("sw-ES-PHL-up.wav");
        const long long msW = svx::wavMs(w.substr(0, 256), (long long)w.size());
        CHECK(!w.empty() && msW > 800 && msW < 4000, "the rendered clip's play time from its header: " + std::to_string(msW) + " ms");
        CHECK(svx::wavMs("RIFF....WAVEjunk", 16) == -1 && svx::wavMs("", 0) == -1, "not a WAV: -1"); }

    std::printf("[5] his real ES 11-19\n");
    {   const std::string wy = rd("sv_wy_es_1119.txt"); CHECK(!wy.empty(), "fixture read");
        // the 3-minute bars (candle END stamps) for the colour rule
        std::map<long long, int> green; { std::istringstream in(rd("nl_es_1119_3m.csv")); std::string ln; while (std::getline(in, ln)) { double t, o, h, l, c, v; if (std::sscanf(ln.c_str(), "%lf,%lf,%lf,%lf,%lf,%lf", &t, &o, &h, &l, &c, &v) == 6) green[(long long)t] = c > o ? 1 : c < o ? -1 : 0; } }
        auto col = [&](long long t, int dir) -> int { auto it = green.find(t); if (it == green.end()) return -1; return it->second == dir ? 1 : 0; };
        const std::vector<svx::Sw> all = svx::parseSw(wy, L(2026, 11, 18, 17, 0), L(2026, 11, 19, 16, 0));
        CHECK(all.size() == 5, "his file: 5 drawn Sw this session (strength >= 10): " + std::to_string(all.size()));
        // replay the session minute by minute as a chart started at 5 PM would see it
        svx::Hub H; std::vector<std::string> said;
        for (long long now = L(2026, 11, 18, 17, 1); now <= L(2026, 11, 19, 16, 0); now += 60) {
            std::vector<svx::Sw> seen; for (const svx::Sw& s : all) if (s.t + 5 <= now) seen.push_back(s);
            H.offer("ES", L(2026, 11, 18, 17, 0), now, seen, 510, 900, col);
            while (H.pump(now * 1000, [](const std::string&) -> long long { return 1800; }, []() -> long long { return 1200; })) said.push_back(H.last); }
        CHECK(said.size() == 1 && said[0] == "ES, sweep, PHL, up", "RTH: the 8:42 AM PHL sweep (V99+) is the one spoken (" + std::to_string(said.size()) + ")");
        CHECK(svx::spoken("ES", "PHL", 1) == "E S, sweep, P H L, up.", "said as: E S, sweep, P H L, up.");
        CHECK(H.skippedRth == 4, "the overnight / pre-RTH ones (7:36 PM, 9:18 PM, 12:06 AM, 5:33 AM) stay quiet");
        // a chart opened at 9:00 AM: the 8:42 one was already there - quiet
        svx::Hub H2; int n2 = 0; for (long long now = L(2026, 11, 19, 9, 0); now <= L(2026, 11, 19, 16, 0); now += 60) { std::vector<svx::Sw> seen; for (const svx::Sw& s : all) if (s.t + 5 <= now) seen.push_back(s); n2 += H2.offer("ES", L(2026, 11, 19, 9, 0), now, seen, 510, 900, col); }
        CHECK(n2 == 0, "chart opened at 9:00 AM: nothing replayed");
        // the 9:42 AM PDL sweep on the Now line was live only: his file has no Sw line for it; as a mark it would need strength >= 10
        CHECK(wy.find(st(L(2026, 11, 19, 9, 42)) + "|Sw") == std::string::npos, "no 9:42 Sw line in his file");
        svx::Hub H3; const std::string w5 = wy + st(L(2026, 11, 19, 9, 42)) + "|Sw|1|PDL|29221|V5\n", w12 = wy + st(L(2026, 11, 19, 9, 42)) + "|Sw|1|PDL|29221|V12\n";
        CHECK(H3.offer("ES", L(2026, 11, 19, 9, 0), L(2026, 11, 19, 9, 42) + 5, svx::parseSw(w5, L(2026, 11, 19, 9, 0), L(2026, 11, 19, 16, 0)), 510, 900, col) == 0, "a V5 9:42 PDL sweep stays quiet (strength < 10)");
        CHECK(H3.offer("ES", L(2026, 11, 19, 9, 0), L(2026, 11, 19, 9, 42) + 5, svx::parseSw(w12, L(2026, 11, 19, 9, 0), L(2026, 11, 19, 16, 0)), 510, 900, col) == 1 && H3.q.back().text == "ES, sweep, PDL, up", "a V12 9:42 PDL sweep (green candle) speaks: ES, sweep, PDL, up"); }

    std::printf("[6] versions\n");
    CHECK(std::string(hl::HL_VERSION) == "2.33.0" && std::string(hlset::PLUGIN_VERSION) == "2.37.2", "2.37.1 (version strings updated in 2.37.1)");
    {   const std::string s = rd("../HodLod.cpp");
        CHECK(s.find("p->setVersion(\"2.37.2\")") != std::string::npos && s.find(" *  2.37.0 (2026-11-19") != std::string::npos, "setVersion 2.37.1 + header comment 2.37.0 still there");
        CHECK(s.find("#pragma comment(lib, \"winmm.lib\")") != std::string::npos && s.find("#include <mmsystem.h>") != std::string::npos, "winmm pragma + mmsystem.h");
        CHECK(s.find("SND_FILENAME | SND_ASYNC | SND_NODEFAULT") != std::string::npos, "PlaySound async, no default");
        CHECK(s.find("SWEEP_VOICE,%s,%d,%s") != std::string::npos, "status line SWEEP_VOICE,1.0.0,<count>,<last text>"); }
    {   const std::string b = rd("../../tools/gex-build.bat"); const size_t p = b.find("call :one HodLod ");
        const std::string ln = p == std::string::npos ? std::string() : b.substr(p, b.find('\n', p) - p);
        for (const char* h : { "HodLodTop.h", "SweepVoice.h", "NowLevels.h", "TopSignals.h", "HodLodLogic.h", "HodLodSettings.h" }) CHECK(ln.find(h) != std::string::npos, std::string("gex-build HodLod line lists ") + h); }
    std::printf("t_hl2370 (lsHodLod 2.37.0): %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
