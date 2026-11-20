// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2390.cpp -o t2390 && ./t2390   (from plugin\tests_hodlod)
// lsHodLod 2.39.0 (2026-11-20 07:45 CT, Rassul "build", mockups gamma-line-mockup.html + hodlod-new-box-mockup.html): the GAMMA line
//  [1] the real ES book of 11-20 (MenthorQ 07:17 CT): significant strikes, names (PS from the Gex nodes), WATCH line = the mockup row
//  [2] TESTING (forming candle at / through a watched strike) - gold, no side; Next beyond it
//  [3] BREAK at the close (short gamma, pressure the same way); long gamma closing through = back to WATCH (pressure disagrees)
//  [4] SWEEP at the close (long gamma, pressure the other way); short gamma closing back = back to WATCH
//  [5] G pressure: arrow / dash / ratio; V / C hook file (fresh / stale / other market)
//  [6] no repaint: real ES 11-20 bars replayed bar by bar, a restart after EVERY bar = one straight pass (display + record byte-identical);
//      a book that changes after a decision does not change it
//  [7] stale / missing / garbled / oversized / other-market books = no gamma line
//  [8] narrow pane: 0DTE first, then Next, then G V C, then the pill's setups, then its regime word; the badge stays; never past the pane,
//      the 0DTE text never touches the line; the Now line sits above (no overlap)
//  [9] two markets independent (ES / NQ books, records keyed by market + bar size)
//  [10] host source + versions 2.39.0 everywhere + gex-build lists GammaLine.h
#include <cstdio>
#include <fstream>
#include <sstream>
#include <map>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../GammaLine.h"
#include "../HodLodGex.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static std::string rd(const char* p) { std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
using namespace gml;
static const std::string ES_BOOK = rd("gl_gp_es_1120.csv"), NQ_BOOK = rd("gl_gp_nq_1120.csv"), ES_GEX = rd("gl_gex_es_1120.csv");
static std::vector<NameAt> esNames() {   // what the host passes: every STR row of LRA-GexNodes (chart price, MenthorQ name)
    std::vector<NameAt> v; const hlg::File F = hlg::parse(ES_GEX, "ES");
    for (const hlg::Loc& L : F.locs) for (const hlg::Mem& m : L.mem) { double px; if (fullD(m.chartTxt, px)) v.push_back(NameAt{ px, m.name }); }
    return v; }
static const long long T0 = 1795161600;   // 2026-11-20 05:00 CT-naive
static Bar bar(long long t, double o, double h, double l, double c) { Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.v = 1000; return b; }
// 25 quiet candles around px (the normal), then the given ones
static std::vector<Bar> quiet(double px, int n = 25) { std::vector<Bar> B; for (int i = 0; i < n; i++) { const double c = px + ((i % 2) ? 1.0 : -1.0); B.push_back(bar(T0 + 180LL * i, px, c + 1, c - 1, c)); } return B; }
static std::string line(const Ctx& c, const std::vector<Bar>& B, double px, const Ev* ev, const Flow& f = Flow()) {
    Out o = live(c, B, px, ev); o.f = f; std::vector<Seg> b = badgeSegs(o); addWatchWord(b, o, px);
    return plain(join({ b, gvcSegs(o), nextSegs(o) })); }
static std::string book(const std::string& strikes, const char* mkt = "ES", const char* asof = "26220,2026-11-20") {
    return std::string("BOOK,MQ\nMARKET,") + mkt + "\nSESSION,2026-11-20\n" + strikes + "ASOF," + asof + "\n"; }
static const std::string LONG7100 = "STRIKE,7000,-86,2,0,,7000,,1\nSTRIKE,7100,100,1,1,PS0,7100,,1\nSTRIKE,7150,-60,4,0,,7150,,1\nSTRIKE,7200,-69,3,0,,7200,,1\n";
int main() {
    const Book ES = parseBook(ES_BOOK, "ES"); const std::vector<NameAt> NM = esNames();
    std::printf("[1] real ES book 11-20\n");
    {   CHECK(ES.ok && ES.s.size() == 14 && ES.asofCt == (daysFromCivil(2026, 11, 20) * 86400 + 26220), "book parsed: 14 strikes, ASOF 07:17 CT " + ES.why);
        const Ctx c = makeCtx(ES, NM, 0.25, 2);
        std::string sg; for (int i : c.sig) sg += fmtPx(ES.s[(size_t)i].px, 2) + " "; CHECK(sg == "7,000 7,100 7,150 7,200 ", "significant (|%| >= 50): " + sg);
        CHECK(nameOf(c, 7100) == "PS" && nameOf(c, 7150).empty() && nameOf(c, 7200).empty() && nameOf(c, 7500) == "CR" && nameOf(c, 7490) == "HVL" && nameOf(c, 7550) == "CW", "names: PS 7100 (PS/PS0 beats PW), G-names are not names, CR0+GW0 -> CR, HVL0 -> HVL, CW row");
        CHECK(bestCode("CR0+GW0") == "CR" && bestCode("KING SLICE").empty() && bestCode("G1").empty() && bestCode("PS/PS0") == "PS", "codes");
        std::vector<Bar> B = quiet(7144); B.push_back(bar(T0 + 180 * 25, 7144, 7144.5, 7143.5, 7143.75));
        const std::string L = line(c, B, 7143.75, nullptr);
        CHECK(L.rfind("[GAMMA WATCH: PS 7,100 \xE2\x86\x93 \xC2\xB7 7,150 \xE2\x86\x91 \xC2\xB7 Break] \xE2\x94\x82 G", 0) == 0, "WATCH as the mockup: " + L);
        CHECK(L.find("Next \xE2\x86\x93 7,000 \xC2\xB7 \xE2\x86\x91 7,200") != std::string::npos, "Next \xE2\x86\x93 7,000 \xC2\xB7 \xE2\x86\x91 7,200: " + L);
        CHECK(fmtPx(58.5, 2) == "58.5" && fmtPx(1.165, 5) == "1.165" && fmtPx(25400, 2) == "25,400" && fmtPx(4150, 1) == "4,150", "price format"); }
    std::printf("[2] TESTING\n");
    {   const Ctx c = makeCtx(ES, NM, 0.25, 2);
        std::vector<Bar> B = quiet(7104); B.push_back(bar(T0 + 180 * 25, 7104, 7105, 7099.75, 7101));
        const std::string L = line(c, B, 7101, nullptr);
        CHECK(L.rfind("[GAMMA TESTING: PS 7,100 \xC2\xB7 Break]", 0) == 0 && L.find("Next \xE2\x86\x93 7,000") != std::string::npos && L.find("7,200") == std::string::npos, "TESTING PS 7,100 \xC2\xB7 Break, Next \xE2\x86\x93 7,000: " + L);
        Out o = live(c, B, 7101, nullptr); CHECK(badgeSegs(o)[0].fill == C_GOLD, "TESTING is gold"); }
    std::printf("[3] BREAK\n");
    {   const Ctx c = makeCtx(ES, NM, 0.25, 2);
        std::vector<Bar> B = quiet(7104); B.push_back(bar(T0 + 180 * 25, 7104, 7105, 7094, 7095)); B.push_back(bar(T0 + 180 * 26, 7095, 7096, 7094, 7095.5));
        const Ev e = decide(c, B, 25);
        CHECK(e.state == BREAK && e.dir == -1 && e.pdir == -1 && e.name == "PS" && std::isfinite(e.ratio) && e.ratio > 1.2, std::string("short gamma, close through 7100, dealers sell: BREAK \xE2\x86\x93 ") + stateWord(e.state));
        const std::string L = line(c, B, 7095.5, &e);
        CHECK(L.rfind("[GAMMA BREAK: PS 7,100 \xE2\x86\x93] \xE2\x94\x82 G\xE2\x86\x93 ", 0) == 0 && L.find("Next \xE2\x86\x93 7,000") != std::string::npos, "BREAK line: " + L);
        Out o = live(c, B, 7095.5, &e); CHECK(badgeSegs(o)[0].fill == C_RED, "BREAK down is red");
        B.push_back(bar(T0 + 180 * 27, 7095.5, 7096, 7094, 7095));   // the next candle closes: the line moves on
        CHECK(line(c, B, 7095, &e).find("BREAK") == std::string::npos, "BREAK shown only while its candle is the last closed one");
        const Book LB = parseBook(book(LONG7100), "ES"); const Ctx cl = makeCtx(LB, {}, 0.25, 2);
        std::vector<Bar> B2 = quiet(7104); B2.push_back(bar(T0 + 180 * 25, 7104, 7105, 7094, 7095)); B2.push_back(bar(T0 + 180 * 26, 7095, 7096, 7094, 7095.5));
        const Ev e2 = decide(cl, B2, 25); CHECK(e2.state == TESTING && e2.pdir == 1, "long gamma closing through: dealers buy against it - no BREAK (TESTING recorded, line WATCH)");
        CHECK(line(cl, B2, 7095.5, &e2).rfind("[GAMMA WATCH:", 0) == 0, "back to WATCH: " + line(cl, B2, 7095.5, &e2)); }
    std::printf("[4] SWEEP\n");
    {   const Book LB = parseBook(book(LONG7100), "ES"); const Ctx cl = makeCtx(LB, {}, 0.25, 2);
        std::vector<Bar> B = quiet(7104); B.push_back(bar(T0 + 180 * 25, 7104, 7106, 7095, 7103)); B.push_back(bar(T0 + 180 * 26, 7103, 7104, 7102, 7103.5));
        const Ev e = decide(cl, B, 25);
        CHECK(e.state == SWEEP && e.dir == 1 && e.pdir == 1 && e.expect == 1, std::string("long gamma 7100, through and back, dealers buy: SWEEP \xE2\x86\x91 ") + stateWord(e.state));
        const std::string L = line(cl, B, 7103.5, &e);
        CHECK(L.rfind("[GAMMA SWEEP: PS 7,100 \xE2\x86\x91] \xE2\x94\x82 G\xE2\x86\x91", 0) == 0 && L.find("Next \xE2\x86\x91 7,150") != std::string::npos, "SWEEP line as the mockup: " + L);
        Out o = live(cl, B, 7103.5, &e); CHECK(badgeSegs(o)[0].fill == C_GREEN, "SWEEP up is green");
        const Ctx c = makeCtx(ES, NM, 0.25, 2);
        const Ev e2 = decide(c, B, 25); CHECK(e2.state == TESTING && e2.pdir == -1, "short gamma through and back: dealers sell - no SWEEP");
        std::vector<Bar> U = quiet(7140); U.push_back(bar(T0 + 180 * 25, 7140, 7152, 7139, 7148)); U.push_back(bar(T0 + 180 * 26, 7148, 7149, 7147, 7148));
        const Book LU = parseBook(book("STRIKE,7100,-100,1,1,,7100,,1\nSTRIKE,7150,90,2,0,CR0,7150,,1\n"), "ES"); const Ctx cu = makeCtx(LU, {}, 0.25, 2);
        const Ev e3 = decide(cu, U, 25); CHECK(e3.state == SWEEP && e3.dir == -1 && e3.name == "CR", "long gamma CR 7150 above, through and back: SWEEP \xE2\x86\x93");
        CHECK(badgeSegs(live(cu, U, 7148, &e3))[0].fill == C_RED, "SWEEP down is red"); }
    std::printf("[5] G pressure, V / C hook\n");
    {   Press p = press(-0.8, -5, 10); CHECK(p.dir == -1 && std::fabs(p.ratio - 0.4) < 1e-9 && ratioTxt(p.ratio).empty(), "short gamma, down move: dealers sell, 0.4x no ratio text");
        p = press(-0.8, -20, 10); CHECK(p.dir == -1 && ratioTxt(p.ratio) == "1.6x", "1.6x shown");
        p = press(0.8, -20, 10); CHECK(p.dir == 1, "long gamma, down move: dealers buy");
        p = press(0.05, -20, 10); CHECK(p.dir == 0, "|g| under 0.10 = dash");
        p = press(-0.8, -0.1, 10); CHECK(p.dir == 0, "flow under 10% of normal = dash");
        p = press(-0.8, 0, 10); CHECK(p.dir == 0, "no move = dash");
        Out o; o.ok = true; CHECK(plain(gvcSegs(o)) == "G\xE2\x80\x93 V\xE2\x80\x93 C\xE2\x80\x93", "all dashes: " + plain(gvcSegs(o)));
        const long long NOW = T0 + 5000;
        const std::string ff = "OPTFLOW,1\nMARKET|ES\nSTAMP|" + std::to_string(NOW - 60) + "\nV|-1|1.3\nC|1|-\n";
        Flow f = parseFlow(ff, "ES", NOW); o.f = f; CHECK(f.ok && plain(gvcSegs(o)) == "G\xE2\x80\x93 V\xE2\x86\x93 1.3x C\xE2\x86\x91", "fresh hook: " + plain(gvcSegs(o)));
        CHECK(gvcSegs(o)[1].acol == C_RED && gvcSegs(o)[2].acol == C_GREEN, "V selling red, C buying green");
        f = parseFlow(ff, "ES", NOW + 700); CHECK(!f.ok && f.why == "stale", "11 min old hook = dashes");
        f = parseFlow(ff, "NQ", NOW); CHECK(!f.ok, "ES hook never on NQ");
        f = parseFlow("junk\n", "ES", NOW); CHECK(!f.ok && f.why == "bad header", "garbled hook");
        f = parseFlow("", "ES", NOW); CHECK(!f.ok && f.why == "no file", "missing hook"); }
    std::printf("[6] no repaint (real ES 11-20 bars, restart after every bar)\n");
    {   std::vector<Bar> R; { std::istringstream in(rd("gl_bars_es_1120.csv")); std::string ln; while (std::getline(in, ln)) { if (ln.size() < 20 || ln[4] != '-') continue; const std::vector<std::string> c = split(ln, '|'); if (c.size() < 6) continue;
              long long d; if (!parseYmd(c[0].substr(0, 10), d)) continue; const long long t = d * 86400 + std::stoll(c[0].substr(11, 2)) * 3600 + std::stoll(c[0].substr(14, 2)) * 60 + 180;
              R.push_back(bar(t, std::stod(c[1]), std::stod(c[2]), std::stod(c[3]), std::stod(c[4]))); } }
        CHECK(R.size() > 150, "real bars loaded " + std::to_string(R.size()));
        const Ctx c = makeCtx(ES, NM, 0.25, 2);
        auto run = [&](bool restart, std::string& rec, std::vector<std::string>& shown, std::map<int, int>& st) {
            Ev mem; long long memT = -1;
            for (size_t j = 2; j <= R.size(); j++) {   // R[0 .. j-1]: the last is the forming one (seen at its close price)
                std::vector<Bar> B(R.begin(), R.begin() + (long)j);
                if (restart) memT = -1;   // a restart: nothing in memory, the record file is all there is
                if (memT != B[B.size() - 2].t) { const StepR r = closedStep(c, B, rec, "ES", 180, 2, hl::HL_VERSION); if (r.append) rec += r.line; mem = r.ev; memT = r.ev.t; st[r.ev.state]++; }
                shown.push_back(line(c, B, B.back().c, &mem));
            } };
        std::string r1, r2; std::vector<std::string> s1, s2; std::map<int, int> st1, st2; run(false, r1, s1, st1); run(true, r2, s2, st2);
        CHECK(r1 == r2 && s1 == s2, "restart after every bar = one straight pass (record + every displayed line)");
        int nt = 0; for (const Ev& e : parseRec(r1, "ES", 180, 2)) if (e.state != WATCH) nt++;
        std::printf("    real day: %d decided candles, TESTING %d, BREAK %d, SWEEP %d\n", (int)parseRec(r1, "ES", 180, 2).size(), st1[TESTING], st1[BREAK], st1[SWEEP]);
        CHECK((int)parseRec(r1, "ES", 180, 2).size() == (int)R.size() - 2 && nt > 0, "every closed candle decided once; the real day has strike events");
        // a decided candle never changes, even when the book moves after it
        std::vector<Bar> B = quiet(7104); B.push_back(bar(T0 + 180 * 25, 7104, 7105, 7094, 7095)); B.push_back(bar(T0 + 180 * 26, 7095, 7096, 7094, 7095.5));
        const StepR a = closedStep(c, B, "", "ES", 180, 2, "2.39.0"); CHECK(a.append && a.ev.state == BREAK, "decided BREAK, appended");
        const Book LB = parseBook(book(LONG7100), "ES"); const Ctx cl = makeCtx(LB, {}, 0.25, 2);
        const StepR b2 = closedStep(cl, B, a.line, "ES", 180, 2, "2.39.0"); CHECK(!b2.append && b2.ev.state == BREAK && b2.ev.dir == -1, "a new book later: the record keeps BREAK \xE2\x86\x93 (no re-decision)");
        CHECK(line(cl, B, 7095.5, &b2.ev).rfind("[GAMMA BREAK: PS 7,100 \xE2\x86\x93]", 0) == 0, "redrawn from the record: " + line(cl, B, 7095.5, &b2.ev));
        CHECK(parseRec(a.line.substr(0, a.line.size() - 1), "ES", 180, 2).empty(), "a torn last line (no newline) is ignored");
        CHECK(parseRec(a.line + a.line, "ES", 180, 2).size() == 1, "a candle written twice (two charts) counts once");
        CHECK(a.line.find(",BREAK,-1,7100.00,PS,Break,-1,") != std::string::npos && a.line.find(",2.39.0\n") != std::string::npos, "record row: " + a.line);
        CHECK(recPath("ES", 180, T0 + 180) == "GammaLine-ES-180s-2026-11-20.csv" && recPath("ES", 180, daysFromCivil(2026, 11, 20) * 86400 + 17 * 3600 + 180) == "GammaLine-ES-180s-2026-11-21.csv", "session file: 17:00 CT starts the next session"); }
    std::printf("[7] stale / missing / garbled books\n");
    {   CHECK(!parseBook("", "ES").ok && parseBook("", "ES").why == "no file", "missing");
        CHECK(!parseBook(ES_BOOK, "NQ").ok && parseBook(ES_BOOK, "NQ").why == "book is for another market", "ES book never on NQ");
        CHECK(!parseBook("BOOK,MQ\nMARKET,ES\nSTRIKE,7100,-100,1,1,,7100,,1\n", "ES").ok, "no ASOF = no line");
        CHECK(!parseBook(ES_BOOK + std::string(70000, '#'), "ES").ok, "oversized = no line");
        const Book g = parseBook(book("STRIKE,7100,-100,1,1,,7100,,1\nSTRIKE,abc,-1,1,0,,1,,1\nSTRIKE,7200,-300,1,0,,7200,,1\n"), "ES"); CHECK(g.ok && g.s.size() == 1 && g.bad == 2, "bad STRIKE rows skipped");
        CHECK(!parseBook(book(""), "ES").ok, "no strikes = no line");
        const Book ok = parseBook(book("STRIKE,7100,-40,1,1,,7100,,1\n"), "ES"); const Ctx c = makeCtx(ok, {}, 0.25, 2);
        std::vector<Bar> B = quiet(7104); B.push_back(bar(T0 + 180 * 25, 7104, 7105, 7103, 7104));
        CHECK(!live(c, B, 7104, nullptr).ok && badgeSegs(live(c, B, 7104, nullptr)).empty(), "no significant strike = no badge = no line");
        const std::string h = rd("../HodLod.cpp");
        CHECK(h.find("S->gmV.stale()") != std::string::npos && h.find("gamma line: book ") != std::string::npos, "host: a stale book draws no gamma line (status says why)");
        CHECK(h.find("if (!gmDrawn)") != std::string::npos, "host: no gamma line = the pill + 0DTE stay on the Now row (2.37.x)"); }
    std::printf("[8] narrow pane drop order, no overlaps\n");
    {   const Ctx c = makeCtx(ES, NM, 0.25, 2);
        std::vector<Bar> B = quiet(7144); B.push_back(bar(T0 + 180 * 25, 7144, 7146, 7143, 7143.75));
        Out o = live(c, B, 7143.75, nullptr); std::vector<Seg> bd = badgeSegs(o); addWatchWord(bd, o, 7143.75);
        const std::vector<Seg> gv = gvcSegs(o), nx = nextSegs(o);
        auto meas = [](const std::string& s, bool bold) { return (int)s.size() * (bold ? 7 : 6); };
        const std::vector<std::string> forms = htop::gammaTag("-GAMMA");
        int prev = -1; bool mono = true, inside = true, expClear = true, badge = true; std::vector<int> seen;
        for (int w = 1400; w >= 100; w -= 10) {
            const int xMin = 10, xMax = xMin + w, cx = (xMin + xMax) / 2;
            const Draw D = layout(forms, C_RED, 0x450A0A, bd, gv, nx, "0DTE Exp 3:00 PM", 0xE5E7EB, meas, cx, 500, xMin, xMax, xMin, 10);
            if (!D.ok) { if (w > 300) badge = false; continue; }
            if (D.level < prev) { mono = false; }
            prev = D.level;
            if (seen.empty() || seen.back() != D.level) seen.push_back(D.level);
            if (D.l < xMin || D.r > xMax) inside = false;
            if (D.exp && D.expR + EXP_GAP > D.l) expClear = false;
            if (D.text.find("GAMMA WATCH") == std::string::npos) badge = false;
        }
        std::string sq; for (int s : seen) sq += std::to_string(s); CHECK(sq == "0123456", "drop order 0DTE, Next, G V C, setups, regime word, pill (badge stays): " + sq);
        CHECK(mono && inside && expClear && badge, "monotone, inside the pane, 0DTE clear of the line, badge always there while anything fits");
        const Draw full = layout(forms, C_RED, 0x450A0A, bd, gv, nx, "0DTE Exp 3:00 PM", 0xE5E7EB, meas, 565, 500, 0, 1130, 0, 10);
        CHECK(full.ok && full.level == 0 && full.text.rfind("0DTE Exp 3:00 PM | (-GAMMA \xC2\xB7 Trending \xC2\xB7 Bk / BkTst) \xE2\x94\x82 [GAMMA WATCH: PS 7,100", 0) == 0, "mockup row: " + full.text);
        CHECK(std::abs((full.l + full.r) / 2 - 565) <= 1, "centred over the box");
        CHECK(nowBottom(full, 496) == full.t - 3 && nowBottom(Draw(), 496) == 496, "the Now line sits 3 px above the gamma line (no gamma line = its old spot)");
        CHECK(full.b == 500 && full.t == 482, "the gamma line's bottom = the old Now spot (above the box / Gex strip)");
        bool pillOk = false; for (const Item& it : full.items) if (it.k == 0 && it.col == C_RED && it.fill == 0x450A0A) pillOk = true; CHECK(pillOk, "pill: red border on the dark red fill (as 2.37.1)");
        bool blackOnGold = false; for (const Item& it : full.items) if (it.k == 1 && it.s.rfind("GAMMA WATCH", 0) == 0 && it.col == C_BLACK) blackOnGold = true; CHECK(blackOnGold, "black text in the filled badge");
        bool gold = false; for (const Item& it : full.items) if (it.k == 1 && it.s == "7,000" && it.col == C_GOLD) gold = true; CHECK(gold, "Next prices gold"); }
    std::printf("[9] two markets\n");
    {   const Book NQ = parseBook(NQ_BOOK, "NQ"); CHECK(NQ.ok && NQ.s.size() == 16, "NQ book");
        const Ctx cn = makeCtx(NQ, {}, 0.25, 2); std::string sg; for (int i : cn.sig) sg += fmtPx(NQ.s[(size_t)i].px, 2) + " ";
        CHECK(sg == "25,400 26,000 26,500 27,000 ", "NQ significant: " + sg);
        CHECK(nameOf(cn, 25400) == "PS" && nameOf(cn, 27000) == "HVL" && nameOf(cn, 26000) == "PW", "NQ names from its own book (PS0, HVL0, PW row)");
        const Ctx ce = makeCtx(parseBook(ES_BOOK, "ES"), NM, 0.25, 2);
        std::vector<Bar> B = quiet(7104); B.push_back(bar(T0 + 180 * 25, 7104, 7105, 7094, 7095)); B.push_back(bar(T0 + 180 * 26, 7095, 7096, 7094, 7095.5));
        const StepR a = closedStep(ce, B, "", "ES", 180, 2, "2.39.0");
        CHECK(parseRec(a.line, "NQ", 180, 2).empty() && parseRec(a.line, "ES", 60, 2).empty() && parseRec(a.line, "ES", 180, 2).size() == 1, "records keyed by market + bar size");
        CHECK(closedStep(ce, B, a.line, "ES", 60, 2, "2.39.0").append, "the 1-min chart decides its own candles"); }
    std::printf("[10] host + versions\n");
    {   const std::string h = rd("../HodLod.cpp"), lg = rd("../HodLodLogic.h"), st = rd("../HodLodSettings.h");
        CHECK(lg.find("HL_VERSION = \"2.39.0\"") != std::string::npos && h.find("p->setVersion(\"2.39.0\")") != std::string::npos && st.find("PLUGIN_VERSION = \"2.39.0\"") != std::string::npos, "versions 2.39.0");
        CHECK(h.find(" *  2.39.0 (2026-11-20") != std::string::npos, "header comment 2.39.0");
        CHECK(h.find("#include \"GammaLine.h\"") != std::string::npos, "host includes GammaLine.h");
        CHECK(h.find("gml::closedStep(") != std::string::npos && h.find("hlAppend(") != std::string::npos && h.find("gml::recHeader()") != std::string::npos, "host records each closed candle once (append after decide)");
        CHECK(h.find("\"OptFlow-\" + S->mkt + \".csv\"") != std::string::npos, "host reads the V / C hook file");
        CHECK(h.find("GAMMA_LINE,") != std::string::npos, "status line GAMMA_LINE");
        const std::string gb = rd("../../tools/gex-build.bat");
        if (!gb.empty()) { size_t p = gb.find("call :one HodLod "); const std::string ln = p == std::string::npos ? "" : gb.substr(p, gb.find('\n', p) - p);
            CHECK(ln.find(" GammaLine.h") != std::string::npos, "gex-build HodLod line lists GammaLine.h");
            bool crlf = true; for (size_t i = 0; i < gb.size(); i++) if (gb[i] == '\n' && (i == 0 || gb[i - 1] != '\r')) crlf = false; CHECK(crlf, "gex-build.bat CRLF kept"); }
        else std::printf("    (no tools\\gex-build.bat next to this checkout - checked separately)\n"); }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
