// t_bo3h_host.cpp -- ls3hBO 1.1.0 (+ SP / UT; was 1.0.0) ("3h BO", Bo3h.cpp) on the mock SDK: the 3-hour BO / BD / Tst on a 180-minute chart, on Rassul's own GC 180-minute
// candles of 2026-10-13 .. 11-23 19:26 CT (gc3h_1123.csv from lsFlexLevels\ChartView\GC_10800_GCZ6.json).
//   (in plugin\tests_bo3h) g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra -Werror -Imock -I.. t_bo3h_host.cpp -o th && ./th
// Covers: only texts are drawn, only the words BO / BD / Tst; green (bullish) / red (bearish); BO above its candle's high, BD below its
// low, bullish Tst below the low, bearish Tst above the high; every mark in view is drawn once, on its own candle; no two letters overlap;
// his picture (BO 11-22 20:00, Tst 11-23 05:00, BO 11-23 08:00, live Tst on the forming candle); bar-by-bar feeding then a restart draws
// the same letters; a final letter never moves or disappears while candles are added; a 3-minute chart still draws nothing; two charts
// on one DLL keep their own marks; the status line H3; the version literal.
#include "irtsdk.h"
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
#include "Bo3h.cpp"
static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
static long long days(int y, int m, int d) { y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400), doy = (153u * (unsigned)(m + (m > 2 ? -3 : 9)) + 2) / 5 + (unsigned)d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; return era * 146097 + (long long)doe - 719468; }
struct Row { RTDATE t; float o, h, l, c; std::string ts; };
static std::vector<Row> load(const char* f) { std::ifstream in(f); std::string l; std::vector<Row> v;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d,%lf,%lf,%lf,%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c) != 10) continue;
        Row r; r.t = (RTDATE)(days(y, mo, d) * 86400LL + h * 3600 + mi * 60 + s); r.o = (float)o; r.h = (float)hi; r.l = (float)lo; r.c = (float)c; r.ts = l.substr(0, 16); v.push_back(r); }
    return v; }
static void chart(mock::Host& H, const std::vector<Row>& R, int upto, int spb = 10800, int view = 120) {
    for (int a = 0; a < 4; ++a) H.f[a].clear();
    H.t.clear(); H.vol.clear();
    for (int i = 0; i <= upto; ++i) { H.f[barOpen].push_back(R[i].o); H.f[barHigh].push_back(R[i].h); H.f[barLow].push_back(R[i].l); H.f[barClose].push_back(R[i].c); H.t.push_back(R[i].t); H.vol.push_back(1000); }
    const int first = std::max(0, upto - view + 1);
    float lo = 1e9f, hi = -1e9f; for (int i = first; i <= upto; ++i) { lo = std::min(lo, R[i].l); hi = std::max(hi, R[i].h); }
    H.lo = lo - 60; H.hi = hi + 60; H.first = first; H.paneR = 10 * (upto - first + 1) + H.rightMargin; H.paneB = 900; H.root = "GC"; H.sym = "GCZ6"; H.spb = spb; H.tick = 0.1;
}
static void pass(Bo3h& x) { x.calc(0); mock::current->drawn.clear(); mock::current->drawing = true; x.draw(); mock::current->drawing = false; }
static std::vector<mock::Prim> coloured(const mock::Host& H) { std::vector<mock::Prim> v; for (const auto& p : H.drawn) if (p.k == 'T' && p.col != 0) v.push_back(p); return v; }
static int barAt(const mock::Host& H, int cx) { return (int)std::lround((cx - H.paneL) / 10.0 - 0.5) + H.first; }
static std::string home;
static std::string slurp(const std::string& p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
int main() {
    char tmpl[] = "/tmp/bo1300XXXXXX"; home = mkdtemp(tmpl); setenv("USERPROFILE", home.c_str(), 1);
    mkdir((home + "/InvestorRT").c_str(), 0755); mkdir((home + "/InvestorRT/rtx").c_str(), 0755); mkdir((home + "/InvestorRT/rtx/lsFlexLevels").c_str(), 0755);
    const std::vector<Row> R = load("gc3h_1123.csv"); CHECK(R.size() == 233, "fixture loaded (" + std::to_string(R.size()) + ")");
    const int N = (int)R.size() - 1;
    std::vector<b3h::Bar> LB; for (const Row& r : R) { b3h::Bar b; b.h = r.h; b.l = r.l; b.c = r.c; LB.push_back(b); }
    const std::vector<b3h::Mark> M = b3h::run(LB);
    // ---- 1. the 180-minute chart, whole history ----
    mock::Host H; mock::current = &H; chart(H, R, N);
    Bo3h* x = static_cast<Bo3h*>(CreateExtension()); x->init(); x->setup(); pass(*x);
    int other = 0; std::set<std::string> words; for (const auto& p : H.drawn) { if (p.k != 'T') ++other; else words.insert(p.text); }
    CHECK(other == 0, "nothing but text is drawn"); for (const std::string& w : words) CHECK(w == "BO" || w == "BD" || w == "Tst" || w == "SP" || w == "UT", "word " + w);
    const std::vector<mock::Prim> C = coloured(H);
    int inView = 0; for (const auto& m : M) if (m.bar > H.first) ++inView;   // a letter that would cross the pane's left edge (the first candle in view) is not drawn
    CHECK((int)C.size() == inView && inView >= 10, "every mark in view drawn once (" + std::to_string(C.size()) + " / " + std::to_string(inView) + ")");
    std::set<std::string> got;
    for (const auto& p : C) { const int bar = barAt(H, (p.x0 + p.x1) / 2); CHECK(bar >= 0 && bar <= N, "on a candle"); if (bar < 0 || bar > N) continue;
        const bool green = p.col == (COLOR)B3_UP, red = p.col == (COLOR)B3_DN; CHECK(green || red, "green or red only");
        const b3h::Mark* mm = nullptr; for (const auto& m : M) if (m.bar == bar && std::string(b3h::text(m)) == p.text) mm = &m;
        CHECK(mm != nullptr, "a decided mark under the letter " + p.text + " at " + R[bar].ts); if (!mm) continue;
        CHECK(green == (mm->dir > 0), "colour = direction");
        mock::current = &H; const int yh = mockY(R[bar].h), yl = mockY(R[bar].l); const bool above = b3h::above(*mm);
        if (above) CHECK(p.y1 <= yh, p.text + " above the high at " + R[bar].ts); else CHECK(p.y0 >= yl - 8, p.text + " below the low at " + R[bar].ts);
        CHECK(p.font == B3_FONT, "small font"); got.insert(R[bar].ts + " " + p.text + (mm->live ? " live" : "")); }
    for (size_t a = 0; a < C.size(); ++a) for (size_t b = a + 1; b < C.size(); ++b) CHECK(C[a].x1 <= C[b].x0 || C[b].x1 <= C[a].x0 || C[a].y1 <= C[b].y0 || C[b].y1 <= C[a].y0, "no overlap " + C[a].text + " / " + C[b].text + " " + std::to_string(C[a].x0) + "," + std::to_string(C[a].y0) + " " + std::to_string(C[b].x0) + "," + std::to_string(C[b].y0));
    CHECK(got.count("2026-11-22 20:00 BO") && got.count("2026-11-23 05:00 Tst") && got.count("2026-11-23 08:00 BO") && got.count("2026-11-23 20:00 Tst live"), "his picture: BO, Tst, BO, live Tst");
    const std::string st = slurp(home + "/InvestorRT/rtx/lsFlexLevels/Bo3h.status-GC-10800.txt");
    CHECK(st.find("H3,b3h 1.1.0,BO 11,BD 17,Tst 10,live 1,SP 13,UT 9,bars 233") != std::string::npos, "status line H3");
    CHECK(std::string(B3_VERSION) == "1.1.0" && std::string(b3h::VERSION) == "1.1.0" && st.find("VERSION,1.1.0\n") == 0, "version");
    // ---- 2. bar by bar (a live chart), then a restart: the same letters; a final letter never moves ----
    mock::Host H2; mock::current = &H2; Bo3h* y = static_cast<Bo3h*>(CreateExtension()); y->init(); y->setup();
    std::map<std::string, int> finalAt; int moved = 0;
    for (int k = 40; k <= N; ++k) { chart(H2, R, k, 10800, 5000); pass(*y);
        std::vector<b3h::Bar> P(LB.begin(), LB.begin() + k + 1); const std::vector<b3h::Mark> V = b3h::run(P);
        std::set<std::string> now; for (const auto& p : coloured(H2)) { const int bar = barAt(H2, (p.x0 + p.x1) / 2); bool live = false; for (const auto& m : V) if (m.bar == bar && std::string(b3h::text(m)) == p.text && m.live) live = true; if (!live) now.insert(std::to_string(bar) + p.text); }
        for (const auto& kv : finalAt) if (!now.count(kv.first)) ++moved;
        for (const std::string& s : now) if (!finalAt.count(s)) finalAt[s] = k; }
    CHECK(moved == 0, "a final letter never moves or disappears (" + std::to_string(moved) + ")");
    chart(H2, R, N); pass(*y); std::set<std::string> a2; for (const auto& p : coloured(H2)) a2.insert(std::to_string(barAt(H2, (p.x0 + p.x1) / 2)) + p.text);
    std::set<std::string> a1; for (const auto& p : C) a1.insert(std::to_string(barAt(H, (p.x0 + p.x1) / 2)) + p.text);
    CHECK(a1 == a2, "bar by bar = all at once");
    y->done(); y->destroy(); Bo3h* z = static_cast<Bo3h*>(CreateExtension()); z->init(); z->setup(); pass(*z);
    std::set<std::string> a3; for (const auto& p : coloured(H2)) a3.insert(std::to_string(barAt(H2, (p.x0 + p.x1) / 2)) + p.text); CHECK(a3 == a1, "restart = the same letters");
    // ---- 3. a 3-minute chart still draws nothing; two charts on one DLL ----
    mock::Host H3; mock::current = &H3; chart(H3, R, N, 180); Bo3h* q = static_cast<Bo3h*>(CreateExtension()); q->init(); q->setup(); pass(*q);
    CHECK(H3.drawn.empty(), "3-minute chart: nothing drawn (" + std::to_string(H3.drawn.size()) + ")");
    CHECK(slurp(home + "/InvestorRT/rtx/lsFlexLevels/Bo3h.status-GC-180.txt").find("H3,off: this is not a 180-minute chart") != std::string::npos, "3-minute chart: the status says off");
    mock::current = &H; pass(*x); CHECK(coloured(H).size() == C.size(), "the 180-minute chart keeps its letters next to another chart");
    mock::current = &H3; pass(*q); CHECK(H3.drawn.empty(), "the 3-minute chart still empty");
    // ---- 4. a narrow view: letters only inside the pane ----
    mock::current = &H; chart(H, R, N, 10800, 12); pass(*x); for (const auto& p : coloured(H)) CHECK(p.x0 >= H.paneL && p.x1 <= H.paneR, "inside the pane");
    mock::current = &H; x->done(); x->destroy(); delete x; mock::current = &H2; z->done(); z->destroy(); delete z; delete y; mock::current = &H3; q->done(); q->destroy(); delete q;
    std::cout << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
