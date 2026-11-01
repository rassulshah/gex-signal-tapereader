// test_candleabsorb_host.cpp - lsCandleAbsorb 1.0.0 host + logic suite on the mock SDK (CandleAbsorb_test_support/irtsdk.h).
//   (in plugin\tests_candleabsorb) g++ -std=c++17 -Wall -Wextra -Werror -ICandleAbsorb_test_support -I.. test_candleabsorb_host.cpp -o th && ./th candleabsorb_es1029_1030.csv
// Covers: the rules on hand-made bars (location, 2.0x size, A / Ae / D / S, the 20% drop-off, one code per bar, the re-test bar);
// the ES 10-29 / 10-30 fixture through the real host (VAP buy / sell -> delta, the marks = the approved mockup's afternoon marks);
// the dash on every candle (on the candle, black edge, colour by the POC's side, the forming candle too, none without VAP); bar-by-bar
// feeding = one pass (no repaint); the record written once (a restart never writes a mark twice); VAP never read in draw(); two charts
// on one DLL keep their own state; the status file; the version literals.
#include "irtsdk.h"
#include <fstream>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
#include "CandleAbsorb.cpp"

static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)

static std::vector<std::string> split(const std::string& s) { std::vector<std::string> o; std::string c; for (char ch : s) { if (ch == ',') { o.push_back(c); c.clear(); } else c += ch; } o.push_back(c); return o; }
struct Fix { std::vector<std::vector<float> > bars; std::vector<RTDATE> t; std::vector<std::vector<std::pair<float, long long> > > vap; std::vector<std::vector<long long> > d; };
static Fix load(const char* path) {
    Fix F; std::ifstream f(path); std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        const std::vector<std::string> x = split(line);
        if (x[0] == "B") { F.t.push_back((RTDATE)std::atoll(x[2].c_str())); F.bars.push_back({ (float)std::atof(x[3].c_str()), (float)std::atof(x[4].c_str()), (float)std::atof(x[5].c_str()), (float)std::atof(x[6].c_str()) }); F.vap.resize(F.t.size()); F.d.resize(F.t.size()); }
        else if (x[0] == "V") { const size_t i = (size_t)std::atoi(x[1].c_str()); F.vap[i].push_back(std::make_pair((float)std::atof(x[2].c_str()), std::atoll(x[3].c_str()))); F.d[i].push_back(std::atoll(x[4].c_str())); }
    }
    return F;
}
static void chart(mock::Host& H, const Fix& F, int first, int upto) {
    for (int a = 0; a < 4; ++a) H.f[a].clear();
    H.t.clear(); H.vap.clear(); H.vapD.clear();
    for (int i = 0; i <= upto; ++i) { for (int a = 0; a < 4; ++a) H.f[a].push_back(F.bars[(size_t)i][(size_t)a]); H.t.push_back(F.t[(size_t)i]); H.vap.push_back(F.vap[(size_t)i]); H.vapD.push_back(F.d[(size_t)i]); }
    float lo = 1e9f, hi = -1e9f; for (int i = first; i <= upto; ++i) { lo = std::min(lo, F.bars[(size_t)i][2]); hi = std::max(hi, F.bars[(size_t)i][1]); }
    H.lo = lo - 3; H.hi = hi + 3; H.first = first; H.paneR = 10 * (upto - first + 1) + H.rightMargin; H.paneB = 900;
}
static std::string home;
static std::string flex() { return home + "/InvestorRT/rtx/lsFlexLevels"; }
static std::string slurp(const std::string& p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void mdraw(CandleAbsorb& x) { mock::current->drawing = true; x.draw(); mock::current->drawing = false; }
static std::string marksOf(const cab::Engine& E) { std::string s; for (const cab::Mark& m : E.marks) s += cab::hm(m.t) + (m.low ? "L" : "H") + cab::codeName(m.code) + " "; return s; }

// a hand-made bar: rows from lo to hi, delta 10 each (median 10) except the given rows
static std::vector<cab::DRow> rowsOf(long long lo, long long hi, const std::map<long long, long long>& d) {
    std::vector<cab::DRow> r; for (long long t = lo; t <= hi; ++t) { cab::DRow x; x.tick = t; auto it = d.find(t); x.dlt = it == d.end() ? 10 : it->second; r.push_back(x); } return r; }
static cab::Bar bar(long long o, long long h, long long l, long long c) { cab::Bar b; b.o = o; b.h = h; b.l = l; b.c = c; return b; }

int main(int argc, char** argv) {
    // ---- 1. the rules on one bar ----
    {   const cab::Bar b = bar(108, 110, 100, 106);   // body 106-108, mid 105
        CHECK(cab::readBar(b, rowsOf(100, 110, {{101, -60}, {100, -30}})).codeLow == cab::A, "sellers' POC at the low, 6x, below the body and in the lower half -> A");
        CHECK(cab::readBar(b, rowsOf(100, 110, {{101, -60}, {100, -12}})).codeLow == cab::AE, "the row past it (100) traded 12 <= 20% of 60 -> Ae");
        CHECK(cab::readBar(b, rowsOf(100, 110, {{101, -60}, {100, -13}})).codeLow == cab::A, "13 > 20% of 60 -> A (no Ae)");
        CHECK(cab::readBar(b, rowsOf(101, 110, {{101, -60}})).codeLow == cab::A, "POC on the bar's last row (nothing traded past it) -> A, not Ae");
        CHECK(cab::readBar(b, rowsOf(100, 110, {{101, 60}})).codeLow == cab::D, "buyers' POC at the low -> D");
        CHECK(cab::readBar(b, rowsOf(100, 110, {{101, 19}})).codeLow == cab::NONE, "1.9x the median -> no code");
        CHECK(cab::readBar(b, rowsOf(100, 110, {{107, -60}})).codeLow == cab::NONE, "POC inside the body -> no code at a low");
        CHECK(cab::readBar(b, rowsOf(100, 110, {{107, -60}})).codeHigh == cab::NONE, "POC inside the body -> no code at a high either");
        const cab::Bar h = bar(102, 110, 100, 104);   // body 102-104, mid 105
        CHECK(cab::readBar(h, rowsOf(100, 110, {{109, 70}, {110, 30}})).codeHigh == cab::A, "buyers' POC at the high -> A");
        CHECK(cab::readBar(h, rowsOf(100, 110, {{109, 70}, {110, 5}})).codeHigh == cab::AE, "row past it 5 <= 14 -> Ae");
        CHECK(cab::readBar(h, rowsOf(100, 110, {{109, -70}})).codeHigh == cab::S, "sellers' POC at the high -> S");
        const cab::BarRead r = cab::readBar(h, rowsOf(100, 110, {{109, -70}}));
        CHECK(r.have && r.poc == 109 && r.pocD == -70 && std::fabs(r.x - 7.0) < 1e-9, "delta POC row, delta and size vs median");
    }
    {   // the zigzag: a low pivot confirmed 3+ bars later; the re-test bar gets its own code; one code per bar
        cab::Engine E;
        const long long L = 100;
        std::vector<cab::Bar> B = { bar(130, 131, 125, 126), bar(126, 127, 120, 121), bar(121, 122, 112, 113), bar(113, 114, 105, 106),
                                    bar(108, 110, L, 106),       // 4: the low (sellers' POC at 101 -> A)
                                    bar(106, 109, L + 2, 107),   // 5: re-tests within 4 ticks (buyers' POC at 102 -> D)
                                    bar(107, 112, 106, 111), bar(111, 116, 110, 115) };   // 7: high > the previous 3 highs -> confirms 4
        for (size_t i = 0; i < B.size(); ++i) {
            std::map<long long, long long> d; if (i == 4) { d[101] = -60; d[100] = -30; } if (i == 5) d[102] = 60;
            E.addBar(B[i], rowsOf(B[i].l, B[i].h, d));
            if (i == 6) CHECK(E.marks.empty(), "nothing before the pivot is confirmed (no early mark)");
        }
        CHECK(E.marks.size() == 2 && E.marks[0].bar == 4 && E.marks[0].code == cab::A && E.marks[1].bar == 5 && E.marks[1].code == cab::D && E.marks[0].conf == 7,
              "low pivot (bar 4) A + the re-test bar (5) D, decided at bar 7 (" << E.marks.size() << ")");
        CHECK(E.out.size() == 2 && E.out[0].find(",LOW,A,") != std::string::npos && E.out[1].find("retest bar") != std::string::npos, "two record lines");
    }
    // ---- 2. the ES fixture through the host ----
    const Fix F = load(argc > 1 ? argv[1] : "candleabsorb_es1029_1030.csv");
    if (F.t.size() != 247) { std::cerr << "no fixture\n"; return 1; }
    char tmpl[] = "/tmp/candleabsorb_host_XXXXXX"; home = mkdtemp(tmpl); setenv("USERPROFILE", home.c_str(), 1);
    mkdir((home + "/InvestorRT").c_str(), 0755); mkdir((home + "/InvestorRT/rtx").c_str(), 0755);   // IRT's own folders
    {   std::string src = slurp("../CandleAbsorb.cpp"); if (src.empty()) src = slurp("CandleAbsorb.cpp"); const size_t p = src.find("p->setVersion(\"");
        const std::string lit = p == std::string::npos ? "" : src.substr(p + 15, src.find('"', p + 15) - p - 15);
        CHECK(lit == CA_VERSION && lit == cab::VERSION && lit == "1.0.0", "version literals match (" << lit << ")"); }
    CandleAbsorb x; mock::Host A; A.root = "EP"; mock::current = &A;
    chart(A, F, 165, 246); x.calc(0);
    CState* S = static_cast<CState*>(A.userData);
    CHECK(S && S->E.B.size() == 246 && S->tick == 0.25, "246 closed bars fed (the last one forming)");
    // the logic straight on the fixture's delta = what the host read through VAP buy / sell volume
    cab::Engine D; for (size_t i = 0; i + 1 < F.t.size(); ++i) { std::vector<cab::DRow> r; cab::Bar b; b.t = (long long)F.t[i];
        cab::toTicks(F.bars[i][0], .25, b.o); cab::toTicks(F.bars[i][1], .25, b.h); cab::toTicks(F.bars[i][2], .25, b.l); cab::toTicks(F.bars[i][3], .25, b.c);
        for (size_t j = 0; j < F.vap[i].size(); ++j) { cab::DRow q; cab::toTicks(F.vap[i][j].first, .25, q.tick); q.dlt = F.d[i][j]; r.push_back(q); } D.addBar(b, r); }
    CHECK(S && marksOf(S->E) == marksOf(D), "host (VAP buy - sell) = logic on the recorded delta (" << (S ? marksOf(S->E) : "") << ")");
    {   std::string pm; if (S) for (const cab::Mark& m : S->E.marks) if (m.t >= 1793367000) pm += cab::hm(m.t) + (m.low ? "L" : "H") + cab::codeName(m.code) + " ";
        CHECK(pm == "13:42HS 13:54LD 14:21LD 14:33LAe ", "Oct 30 PM = the approved mockup: 1:42 S, 1:54 D, 2:21 D, 2:33 Ae (bar END times) (" << pm << ")"); }
    // draw: dashes on every visible candle with VAP (+ the forming one), each a black edge then the colour, on the candle
    {   mock::current = &A; A.drawn.clear(); A.reason = REASON_DRAW; const int reads = A.vapInDraw; mdraw(x);
        CHECK(A.vapInDraw == reads, "draw() never reads VAP");
        int edges = 0, cols = 0, onCandle = 0, txt = 0; bool order = true, colOk = true;
        for (size_t i = 0; i < A.drawn.size(); ++i) { const mock::Prim& p = A.drawn[i];
            if (p.k == 'F' && p.col == 0x000000) { ++edges; if (i + 1 >= A.drawn.size() || A.drawn[i + 1].k != 'F' || (A.drawn[i + 1].col != 0x22C55E && A.drawn[i + 1].col != 0xEF4444)) order = false; }
            if (p.k == 'F' && (p.col == 0x22C55E || p.col == 0xEF4444)) { ++cols;
                const int xc = (p.x0 + p.x1) / 2; int bi = -1; for (int k = 165; k <= 246; ++k) if (std::abs(mockX(k) - xc) <= 1) bi = k;
                if (bi >= 0) { ++onCandle; const float lo = F.bars[(size_t)bi][2], hi = F.bars[(size_t)bi][1]; const int yc = (p.y0 + p.y1) / 2;
                    if (yc < mockY(hi) - 2 || yc > mockY(lo) + 2) colOk = false;
                    if (p.x1 - p.x0 < 6 + 3 || p.x1 - p.x0 > 6 + 6) colOk = false; } }   // body 6 px (10 px bars) + 2 px a side
            if (p.k == 'T') ++txt; }
        int withVap = 0; for (int k = 165; k <= 246; ++k) { long long s = 0; for (long long v : F.d[(size_t)k]) s += v != 0; withVap += s > 0; }
        CHECK(edges == cols && order && cols == withVap && onCandle == cols, "a dash on every visible candle with delta incl. the forming one, black edge first (" << cols << " / " << withVap << ")");
        CHECK(colOk, "each dash ON its candle (inside its high-low), a little wider than the body");
        int inView = 0; for (const cab::Mark& m : S->E.marks) inView += m.bar + S->off >= 165;
        CHECK(txt == inView && txt >= 4, "every mark in the view drawn (" << txt << " / " << inView << ")");
        bool where = true; for (const mock::Prim& p : A.drawn) if (p.k == 'T') { const bool low = p.col == 0x22C55E; const std::string t = p.text;
            if (!(t == "A?" || t == "Ae?" || t == "D?" || t == "S?")) where = false;
            if (low && t == "S?") where = false;
            if (!low && t == "D?") where = false; }
        CHECK(where, "codes A? / Ae? / D? / S?: green under lows, red over highs");
        // the dash colour follows the POC side: bar 246 (forming) and one closed bar
        const cab::BarRead r = S->E.R[(size_t)(230 - S->off)]; int c230 = -1;
        for (const mock::Prim& p : A.drawn) if (p.k == 'F' && p.col != 0 && std::abs((p.x0 + p.x1) / 2 - mockX(230)) <= 1) c230 = (int)(p.col == 0x22C55E);
        CHECK(r.have && c230 == (r.pocD > 0 ? 1 : 0), "dash green when buyers made the POC, red when sellers did");
    }
    // the record + status
    {   const std::string rec = slurp(flex() + "/CandleAbsorb/EP-180.csv"), st = slurp(flex() + "/CandleAbsorb.status-EP-180.txt");
        size_t n = 0; for (size_t p = rec.find("\nMARK,"); p != std::string::npos; p = rec.find("\nMARK,", p + 1)) ++n;
        CHECK(n == S->E.marks.size() && n > 0, "record: every mark once (" << n << ")");
        CHECK(st.rfind("VERSION,1.0.0", 0) == 0 && st.find("MARKS,A ") != std::string::npos && st.find("RULES,ZigZag 3 bars") != std::string::npos, "status file");
        // restart: a fresh state replays the chart and writes nothing twice
        x.clear(); mock::current = &A; x.calc(0); const std::string rec2 = slurp(flex() + "/CandleAbsorb/EP-180.csv");
        CHECK(rec2 == rec, "a restart never writes a mark twice");
        S = static_cast<CState*>(A.userData);   // the new state after the restart
    }
    // bar by bar (each new bar a calc) = one straight pass
    {   mock::Host B; B.root = "EP"; B.sym = "EPZ26b"; mock::current = &B; CandleAbsorb y;
        for (int u = 1; u <= 246; ++u) { chart(B, F, 0, u); y.calc(0); }
        CState* T = static_cast<CState*>(B.userData);
        CHECK(T && S && marksOf(T->E) == marksOf(S->E), "bar-by-bar = one pass (no repaint)");
        y.clear();
    }
    // a bar without VAP: no dash, no mark, counted
    {   mock::Host C; C.root = "EP"; C.sym = "EPZ26c"; mock::current = &C; CandleAbsorb z; chart(C, F, 165, 246); C.vapMissing = { 230 };
        z.calc(0); C.drawn.clear(); mdraw(z); int at = 0; for (const mock::Prim& p : C.drawn) if (p.k == 'F' && std::abs((p.x0 + p.x1) / 2 - mockX(230)) <= 1) ++at;
        CState* T = static_cast<CState*>(C.userData);
        CHECK(at == 0 && T && T->vapMissing == 1, "no VAP for bar 230 -> no dash there, counted in the status");
        z.clear();
    }
    // two charts on one DLL: own state
    {   mock::Host N; N.root = "ENQ"; N.sym = "ENQZ26"; mock::current = &N; chart(N, F, 165, 200); x.calc(0);
        CState* q = static_cast<CState*>(N.userData); mock::current = &A; CState* a = static_cast<CState*>(A.userData);
        CHECK(q && a && q != a && q->mkt == "ENQ" && q->E.B.size() == 200 && a->E.B.size() == 246, "two charts, own state");
        mock::current = &N; x.clear(); mock::current = &A; x.clear();
        CHECK(A.userData == nullptr && N.userData == nullptr, "destroy releases each chart's state");
    }
    std::string cmd = "rm -rf '" + home + "'"; if (std::system(cmd.c_str()) != 0) std::cerr << "rm failed\n";
    std::cout << "test_candleabsorb_host: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
