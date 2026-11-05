// test_bo100_host.cpp -- lsBreakout 1.5.0 (2026-11-04: + "BI" - red above / green below, font 7, the BO row or rows further out, only on kept BI candles, every BI in view drawn once, no overlaps, in SignalBoard 'L'; the BO letters skip a BO / BD a BI cancelled) / 1.4.0 (2026-11-04: version only - the carry is tested in test_bo120_week [6]) / 1.3.0 (2026-11-03; 1.3.0 version only - the records have their own suite test_bo130_rec; 1.2.0 3-bar ZigZag: version + status RULES line; 1.0.0 + Pb; 1.1.1 Pb two rows out: 35 px from the wick) host suite: Breakout.cpp on the mock SDK (Breakout_test_support/irtsdk.h).
//   (in plugin\tests_bo100) g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -IBreakout_test_support -I.. test_bo100_host.cpp -o th && ./th
// Covers, on the real ES Oct 28 - 30 3-min candles (es_2026-10-28_30_3m.txt, from fp_bars):
//   the ONLY drawn things are the texts "BO" / "BD" / "Rly" (no fills, boxes, lines, ovals, numbers); BO green just above the candle's
//   high (Y(high) - 3), BD red just below its low (Y(low) + 3), small font (7), Rly font 9 one row further out; no two letters overlap;
//   each letter sits on a candle with a fired decision (nothing on other candles, nothing on the forming candle); every fired
//   decision in view is drawn; bar-by-bar feeding then a restart draws the same letters (no repaint); two charts on one DLL keep their
//   own state; a 1-min chart gets BO / BD but no Rly; the settings-window copy with no chart is safe (SS_SAFE_DIALOG); the status file;
//   SignalBoard 'L' posted for the BO letters; the version literals agree (BKO_VERSION = bko::VERSION = setVersion).
//   (1.1.0) + "Pb": green just below a pivot LOW / red just above a pivot HIGH (3 px, font 7, one row out when another letter is there),
//   only on bars with a kept Pb decision, every Pb in view drawn, no overlaps, carried in SignalBoard 'L'.
#include "irtsdk.h"
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
#include "Breakout.cpp"

static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
struct Row { RTDATE t; float o, h, l, c; unsigned long v; };
static std::vector<Row> load(const char* f) {
    std::ifstream in(f); std::string l; std::vector<Row> v;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        Row r; r.t = (RTDATE)(svl::daysFromCivil(y, mo, d) * 86400LL + h * 3600 + mi * 60 + s); r.o = (float)o; r.h = (float)hi; r.l = (float)lo; r.c = (float)c; r.v = (unsigned long)vo; v.push_back(r); }
    return v;
}
static void chart(mock::Host& H, const std::vector<Row>& R, int upto, int view = 260) {
    for (int a = 0; a < 4; ++a) H.f[a].clear();
    H.t.clear(); H.vol.clear();
    for (int i = 0; i <= upto; ++i) { H.f[barOpen].push_back(R[i].o); H.f[barHigh].push_back(R[i].h); H.f[barLow].push_back(R[i].l); H.f[barClose].push_back(R[i].c); H.t.push_back(R[i].t); H.vol.push_back(R[i].v); }
    const int first = std::max(0, upto - view + 1);
    float lo = 1e9f, hi = -1e9f; for (int i = first; i <= upto; ++i) { lo = std::min(lo, R[i].l); hi = std::max(hi, R[i].h); }
    H.lo = lo - 6; H.hi = hi + 6; /* IRT keeps a margin above / below the visible range */ H.first = first; H.paneR = 10 * (upto - first + 1) + H.rightMargin; H.paneB = 900; H.root = "ES"; H.sym = "ESZ26";
}
static std::string home;
static std::string slurp(const std::string& p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void pass(Breakout& x) { x.calc(0); mock::current->drawn.clear(); mock::current->drawing = true; x.draw(); mock::current->drawing = false; }
static std::vector<mock::Prim> coloured(const mock::Host& H) { std::vector<mock::Prim> v; for (const auto& p : H.drawn) if (p.k == 'T' && p.col != 0) v.push_back(p); return v; }
static std::string sig(const mock::Host& H) { std::string s; for (const auto& p : H.drawn) { char b[128]; std::snprintf(b, sizeof b, "%c%s|%d|%d|%d|%d|%lx|%d;", p.k, p.text.c_str(), p.x0, p.y0, p.x1, p.y1, p.col, p.font); s += b; } return s; }
static int barAt(const mock::Host& H, int cx) { return (int)std::lround((cx - H.paneL) / 10.0 - 0.5) + H.first; }

int main() {
    char tmpl[] = "/tmp/bo100XXXXXX"; home = mkdtemp(tmpl); setenv("USERPROFILE", home.c_str(), 1);
    mkdir((home + "/InvestorRT").c_str(), 0755); mkdir((home + "/InvestorRT/rtx").c_str(), 0755);
    const std::vector<Row> R = load("es_2026-10-28_30_3m.txt");
    CHECK(R.size() > 1000, "fixture loaded");
    const int N = (int)R.size() - 1;
    // ---- 1. one chart, the whole history at once ----
    mock::Host H; mock::current = &H; chart(H, R, N);
    Breakout* x = static_cast<Breakout*>(CreateExtension()); x->init(); x->setup();
    pass(*x);
    const BState* S = x->testState(); CHECK(S && S->E.bars.size() == R.size(), "state built from every bar");
    // only texts, only the three words
    int other = 0; std::set<std::string> words; for (const auto& p : H.drawn) { if (p.k != 'T') ++other; else words.insert(p.text); }
    CHECK(other == 0, "nothing but text is drawn (no boxes / fills / lines / ovals)");
    bool onlyW = true; for (const auto& w : words) if (w != "BO" && w != "BD" && w != "Rly" && w != "Pb" && w != "BI") onlyW = false;
    CHECK(onlyW && words.count("BD") && words.count("Rly") && words.count("Pb") && !words.count("BI"), "(1.6.0) the only texts are BO / BD / Rly / Pb - no BI (no numbers, prints or lots)");
    const std::vector<mock::Prim> C = coloured(H);
    int bad = 0, nBo = 0, nBd = 0, nR = 0, nPb = 0, pbExact = 0, nBi = 0; std::set<int> boBars, pbBars; std::set<std::pair<int, int>> biSides; std::set<std::pair<int, int>> sides;
    const int lift7 = std::max(1, (1 + (7 / 4 + 1) + 1) / 2), lift9 = std::max(1, (1 + (9 / 4 + 1) + 1) / 2);
    for (const auto& p : C) {
        const int bar = barAt(H, (p.x0 + p.x1) / 2);
        if (bar < 0 || bar >= N) { ++bad; continue; }                                   // never on the forming candle
        const int yh = mockY(H.f[barHigh][(size_t)bar]), yl = mockY(H.f[barLow][(size_t)bar]);
        if (p.text == "Pb") { ++nPb; pbBars.insert(bar); const bool up = p.col == 0x0022C55E; if (p.font != 7 || (p.col != 0x0022C55E && p.col != 0x00EF4444)) ++bad;   // (1.1.0)
            const int ye = up ? p.y0 + lift7 : p.y1 + lift7; if (up ? ye < yl + 35 : ye > yh - 35) ++bad; if (up ? ye == yl + 35 : ye == yh - 35) ++pbExact; sides.insert({ bar, up ? -1 : 1 });   // (1.1.1) two rows out
            if (up ? ye < yl + 3 + 14 + 16 + 2 : ye > yh - 3 - 14 - 16 - 2) ++bad; }   // (1.1.1) never in the wave-time label row (+3 .. +17) or the CandleAbsorb A row (+19 .. +33)
        if (p.text == "BO") { sides.insert({ bar, 1 }); } if (p.text == "BD") { sides.insert({ bar, -1 }); }
        if (p.text == "BI") { ++nBi; const bool up = p.col == 0x0022C55E; if (p.font != 7 || (p.col != 0x0022C55E && p.col != 0x00EF4444)) ++bad;   // (1.5.0) BI UP green below / BI DOWN red above
            const int off = up ? p.y0 + lift7 - yl : yh - (p.y1 + lift7); if (off < 3 || (off - 3) % 16 != 0 || (off - 3) / 16 == 3 || (off - 3) / 16 > 5) ++bad;   // a row: +3, +19, +35, (not +51 = LVP), +67, +83
            if (!biSides.insert({ bar, up ? -1 : 1 }).second) { ++bad; }
            sides.insert({ bar, up ? -1 : 1 }); }
        if (p.text == "BO") { ++nBo; boBars.insert(bar); if (p.col != 0x0022C55E || p.font != 7 || p.y1 + lift7 != yh - 3) ++bad; }
        else if (p.text == "BD") { ++nBd; boBars.insert(bar); if (p.col != 0x00EF4444 || p.font != 7 || p.y0 + lift7 != yl + 3) ++bad; }
        else if (p.text == "Rly") { ++nR; const bool up = p.col == 0x0022C55E; if (p.font != 9) ++bad;
            if (up ? !(p.y1 + lift9 <= yh - 3 - 16) : !(p.y0 + lift9 >= yl + 3 + 16)) ++bad; }
    }
    CHECK(bad == 0, "BO green at Y(high) - 3, BD red at Y(low) + 3, font 7; Rly font 9 at least 16 px further out; none on the forming candle (" + std::to_string(bad) + " bad)");
    // each coloured letter <-> a fired decision in view, and every fired decision in view drawn
    int fired = 0, drawnOk = 0; std::set<int> decBars;
    for (const auto& kv : S->E.bo) if (S->E.shownBo(kv.second)) { for (int i = H.first; i < N; ++i) if ((long long)H.t[(size_t)i] == kv.first) { ++fired; decBars.insert(i); if (boBars.count(i)) ++drawnOk; } }
    CHECK(fired > 0 && drawnOk == fired && boBars == decBars, "every fired BO / BD in view is drawn on its breakout candle and no other candle has one (" + std::to_string(drawnOk) + " / " + std::to_string(fired) + ")");
    int rf = 0; for (const auto& kv : S->E.rly) if (kv.second.fired) for (int i = H.first; i < N; ++i) if ((long long)H.t[(size_t)i] == kv.second.te) ++rf;
    CHECK(rf == nR && nR > 0, "every fired Rly in view drawn once (" + std::to_string(nR) + " / " + std::to_string(rf) + ")");
    {   int pf = 0; std::set<int> pdec; for (const auto& kv : S->E.pb) for (int i = H.first; i < N; ++i) if ((long long)H.t[(size_t)i] == kv.first) pdec.insert(i);
        pf = (int)pdec.size(); CHECK(pf > 0 && pdec == pbBars && nPb == pf && pbExact >= nPb - 2, "(1.1.1) every kept Pb in view drawn once on its pivot bar, no other bar has one, 35 px from the wick (two rows out) unless a letter is there (" + std::to_string(nPb) + " / " + std::to_string(pf) + ", exact " + std::to_string(pbExact) + ")"); }
    CHECK(S->E.bi.empty() && nBi == 0 && biSides.empty(), "(1.6.0) BI removed: none decided, none drawn");
    int ov = 0; for (size_t a = 0; a < C.size(); ++a) for (size_t b = a + 1; b < C.size(); ++b) { const auto& p = C[a]; const auto& q = C[b]; if (p.x0 < q.x1 && q.x0 < p.x1 && p.y0 < q.y1 && q.y0 < p.y1) ++ov; }
    CHECK(ov == 0, "no two letters overlap (" + std::to_string(ov) + ")");
    { int halo = 0; for (const auto& p : H.drawn) if (p.k == 'T' && p.col == 0) ++halo; CHECK(halo == 4 * (int)C.size(), "each letter has its 1 px black edge (4 offsets), as lsBigBidAsk drew them"); }
    std::printf("drawn in view: BO %d, BD %d, Rly %d, Pb %d, BI %d\n", nBo, nBd, nR, nPb, nBi);
    // SignalBoard 'L'
    { std::vector<sb::Ev> ev; const bool ok = sb::readAll('L', "ESZ26", 180, "", bkoMs(), ev); int good = 0; for (const auto& e : ev) if (e.val > 3 && e.val < 400) ++good;
      CHECK(ok && (int)ev.size() == std::min<int>((int)sb::CAP, (int)sides.size()) && good == (int)ev.size(), "SignalBoard 'L' posted for the BO / BD and Pb letters, one per bar and side (px from the extreme to the outer edge) (" + std::to_string(ev.size()) + " / " + std::to_string(sides.size()) + ")"); }
    // status file + versions
    const std::string st = slurp(home + "/InvestorRT/rtx/lsFlexLevels/Breakout.status-ES-180.txt");
    CHECK(st.rfind("VERSION,1.8.0\n", 0) == 0 && st.find("BI,removed") != std::string::npos && st.find(",bi 0,") != std::string::npos && st.find("overlaps the BO / BD candle, at most 2 per BO / BD") != std::string::npos && st.find("3-bar High/Low ZigZag") != std::string::npos && st.find(",Pb ") != std::string::npos && st.find("MARKET,ES") != std::string::npos && st.find("DRAWN,") != std::string::npos, "status file written, version first");
    { const std::string src = slurp("../Breakout.cpp"); const size_t a = src.find("p->setVersion(\""); const std::string lit = a == std::string::npos ? "" : src.substr(a + 15, src.find('"', a + 15) - a - 15);
      CHECK(lit == BKO_VERSION && lit == bko::VERSION, "setVersion literal = BKO_VERSION = bko::VERSION (" + lit + ")"); }
    const std::string full = sig(H);
    x->destroy(); delete x; CHECK(H.userData == nullptr, "destroy releases the per-chart state");
    // ---- 2. bar by bar from the start of Oct 30's session, then compare with one pass; then a restart ----
    {   mock::Host G; mock::current = &G; Breakout* y = static_cast<Breakout*>(CreateExtension()); y->setup();
        int start = 0; for (int i = 0; i < (int)R.size(); ++i) if (R[i].t % 86400 == 17 * 3600 + 180 && i > 600) { start = i; break; }
        for (int u = start; u <= N; ++u) { chart(G, R, u); y->calc(0); }
        chart(G, R, N); pass(*y);
        CHECK(sig(G) == full, "bar-by-bar feeding draws exactly what one pass draws");
        y->destroy(); delete y; y = static_cast<Breakout*>(CreateExtension()); y->setup(); pass(*y);
        CHECK(sig(G) == full, "a restart draws the same letters (no repaint)");
        y->destroy(); delete y; }
    // ---- 3. two charts on one DLL: ES 3-min and a 1-min chart of other data; then back to ES ----
    {   mock::Host A, B2; mock::current = &A; chart(A, R, N); Breakout* z = static_cast<Breakout*>(CreateExtension()); z->setup(); pass(*z);
        const std::string a1 = sig(A);
        mock::current = &B2; chart(B2, R, 400); B2.spb = 60; B2.root = "NQ"; B2.sym = "NQZ26"; pass(*z);
        bool noRly = true; for (const auto& p : B2.drawn) if (p.text == "Rly") noRly = false;
        CHECK(noRly, "a 1-min chart gets no Rly (3-min rule)");
        mock::current = &A; pass(*z); CHECK(sig(A) == a1, "chart A unchanged after chart B used the same DLL object");
        mock::current = &B2; z->destroy(); mock::current = &A; z->destroy(); delete z; }
    // ---- 4. the Add / settings window copy: no chart (no bars, no symbol) ----
    {   mock::Host D; D.dialog = true; D.root = ""; D.sym = ""; mock::current = &D; Breakout* w = static_cast<Breakout*>(CreateExtension()); w->setup();
        w->parmsLoad(); w->parmsApply(); w->parmsUpdt(0); pass(*w);
        CHECK(D.drawn.empty(), "the chart-less settings copy draws nothing and does not fault");
        w->destroy(); delete w; }
    // ---- 4b. no room: a letter that would leave the price area (here: the pane squeezed to the bars) is not drawn, never clipped ----
    {   mock::Host E3; mock::current = &E3; chart(E3, R, N); float lo = 1e9f, hi = -1e9f; for (int i = E3.first; i <= N; ++i) { lo = std::min(lo, R[i].l); hi = std::max(hi, R[i].h); }
        E3.lo = lo - 0.5; E3.hi = hi + 0.5; Breakout* w = static_cast<Breakout*>(CreateExtension()); w->setup(); pass(*w);
        bool in = true; for (const auto& p : E3.drawn) if (p.y0 < E3.paneT + 20 - 3 || p.y1 > E3.paneB + 3) in = false;
        CHECK(in && w->testState()->last.noRoom > 0, "letters with no room inside the pane are dropped (counted), never drawn outside (" + std::to_string(w->testState()->last.noRoom) + ")");
        w->destroy(); delete w; }
    // ---- 5. too few bars / NaN bars ----
    {   mock::Host E2; mock::current = &E2; chart(E2, R, 1); Breakout* w = static_cast<Breakout*>(CreateExtension()); w->setup(); pass(*w);
        CHECK(E2.drawn.empty(), "2 bars: nothing drawn");
        chart(E2, R, 300); E2.f[barHigh][250] = NAN; E2.f[barLow][251] = INFINITY; pass(*w);
        bool okW = true; for (const auto& p : E2.drawn) if (p.text != "BO" && p.text != "BD" && p.text != "Rly" && p.text != "Pb" && p.text != "BI") okW = false;
        CHECK(okW, "NaN / Inf bars: still only letters, no fault");
        w->destroy(); delete w; }
    std::printf("RESULT %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
