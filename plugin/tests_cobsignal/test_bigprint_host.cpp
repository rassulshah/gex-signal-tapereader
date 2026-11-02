// test_bigprint_host.cpp - lsBigPrint 1.0.0 host + logic suite on the mock SDK (CobSignal_test_support/irtsdk.h).
//   (in plugin\tests_cobsignal) g++ -std=c++17 -Wall -Wextra -ICobSignal_test_support -I.. test_bigprint_host.cpp -o tb && ./tb ../tests_candleabsorb/candleabsorb_es1029_1030.csv
// Covers: the "since" counts against a brute force (strictly bigger, the bar before = 1, none = i + 1, cap 100; BGB / BRB compare green /
// red candles only; no VAP stops the ask / bid count); the ES 10-29 / 10-30 fixture through the real host = the logic; the forming bar
// never changes a closed bar's counts; VAP never read in draw(); the pane: green and red bars side by side, both UP from one baseline,
// numbers only from 10 (bold from 35), V labels from 35; the settings-window copy draws nothing; the version literals.
#include "irtsdk.h"
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
#include "BigPrint.cpp"

static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
static std::vector<std::string> split(const std::string& s) { std::vector<std::string> o; std::string c; for (char ch : s) { if (ch == ',') { o.push_back(c); c.clear(); } else c += ch; } o.push_back(c); return o; }
struct Fix { std::vector<std::vector<float> > bars; std::vector<RTDATE> t; std::vector<std::vector<std::pair<float, long long> > > vap; std::vector<std::vector<long long> > d; };
static Fix load(const char* path) {
    Fix F; std::ifstream f(path); std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        const std::vector<std::string> x = split(line);
        if (x[0] == "B") { F.t.push_back((RTDATE)std::atoll(x[2].c_str())); F.bars.push_back({ (float)std::atof(x[3].c_str()), (float)std::atof(x[4].c_str()), (float)std::atof(x[5].c_str()), (float)std::atof(x[6].c_str()) }); F.vap.emplace_back(); F.d.emplace_back(); }
        else if (x[0] == "V") { const size_t i = (size_t)std::atoi(x[1].c_str()); F.vap[i].push_back(std::make_pair((float)std::atof(x[2].c_str()), std::atoll(x[3].c_str()))); F.d[i].push_back(std::atoll(x[4].c_str())); }
    }
    return F;
}
static void chart(mock::Host& H, const Fix& F, int upto, int first) {
    for (int a = 0; a < 4; ++a) H.f[a].clear(); H.t.clear(); H.vap.clear(); H.vapD.clear(); H.vol.clear();
    for (int i = 0; i <= upto; ++i) { for (int a = 0; a < 4; ++a) H.f[a].push_back(F.bars[(size_t)i][(size_t)a]); H.t.push_back(F.t[(size_t)i]); H.vap.push_back(F.vap[(size_t)i]); H.vapD.push_back(F.d[(size_t)i]);
        long long v = 0; for (auto& r : F.vap[(size_t)i]) v += r.second; H.vol.push_back((RTDATE)v); }
    H.lo = 0; H.hi = 1; H.first = first; H.paneR = 10 * (upto - first + 1) + H.rightMargin; H.paneB = 300;
}
static void mdraw(BigPrint& x) { mock::current->drawing = true; x.draw(); mock::current->drawing = false; }
static int brute(const std::vector<double>& x, const std::vector<bool>& ok, const std::vector<bool>& skip, int i) {
    if (!ok[(size_t)i] || skip[(size_t)i]) return 0;
    for (int k = i - 1; k >= 0 && i - k < 100; --k) { if (skip[(size_t)k]) continue; if (!ok[(size_t)k] || x[(size_t)k] > x[(size_t)i]) return i - k; }
    return std::min(100, i + 1);
}

int main(int argc, char** argv) {
    const Fix F = load(argc > 1 ? argv[1] : "../tests_candleabsorb/candleabsorb_es1029_1030.csv");
    CHECK(F.bars.size() == 247, "fixture: ES 10-29 + 10-30 (" << F.bars.size() << " bars)");
    char tmpl[] = "/tmp/bigprint-XXXXXX"; const std::string home = mkdtemp(tmpl); setenv("USERPROFILE", home.c_str(), 1);
    mkdir((home + "/InvestorRT").c_str(), 0755); mkdir((home + "/InvestorRT/rtx").c_str(), 0755); mkdir((home + "/InvestorRT/rtx/lsFlexLevels").c_str(), 0755);
    // ---- the since convention on hand-made values
    {   std::vector<bpr::Bar> B; auto mk = [](double v, double o, double c, double h, double l) { bpr::Bar b; b.v = v; b.o = o; b.c = c; b.h = h; b.l = l; b.vap = true; b.bigAsk = v; b.bigBid = v; return b; };
        B.push_back(mk(50, 1, 2, 3, 0)); B.push_back(mk(10, 2, 1, 3, 1)); B.push_back(mk(10, 1, 2, 2, 1));
        bpr::Counts c = bpr::countsAt(B, 3, mk(20, 1, 2, 4, 1));
        CHECK(c.vol == 3 && c.ask == 3, "20 after 50, 10, 10: the bigger bar 3 back -> 3 (" << c.vol << ")");
        c = bpr::countsAt(B, 3, mk(10, 1, 2, 2, 1));
        CHECK(c.vol == 3, "a tie is not bigger: 10 after 50, 10, 10 -> the 50 three back (" << c.vol << ")");
        CHECK(c.green == 3, "BGB: a green range-1 bar vs green bars only (the red bar skipped): the equal one 1 back is not bigger, the range-3 one 3 back is -> 3 (" << c.green << ")");
    }
    // ---- the fixture through the host
    mock::Host A; mock::current = &A; A.root = "ES"; A.sym = "ESZ26"; const int n = (int)F.bars.size(); chart(A, F, n - 1, n - 130);
    BigPrint x; x.calc(0); BPState* S = static_cast<BPState*>(A.userData);
    CHECK(S && (int)S->E.B.size() == n - 1, "every closed bar fed once (" << (S ? S->E.B.size() : 0) << ")");
    {   std::vector<double> ask, bid, vol, rng; std::vector<bool> okV, okAll, notG, notR, none;
        for (int i = 0; i < n - 1; ++i) { double a = 0, b = 0, tv = 0; for (size_t k = 0; k < F.vap[(size_t)i].size(); ++k) { const long long t = F.vap[(size_t)i][k].second, d = F.d[(size_t)i][k]; const long long buy = (t + d) / 2; a = std::max(a, (double)buy); b = std::max(b, (double)(t - buy)); tv += (double)t; }
            ask.push_back(a); bid.push_back(b); vol.push_back(tv); rng.push_back(F.bars[(size_t)i][1] - F.bars[(size_t)i][2]); okV.push_back(tv > 0); okAll.push_back(true);
            notG.push_back(!(F.bars[(size_t)i][3] > F.bars[(size_t)i][0])); notR.push_back(!(F.bars[(size_t)i][3] < F.bars[(size_t)i][0])); none.push_back(false); }
        bool same = true; int strongA = 0, strongB = 0;
        for (int i = 0; i < n - 1; ++i) { const bpr::Counts& c = S->E.C[(size_t)i];
            same = same && c.ask == brute(ask, okV, none, i) && c.bid == brute(bid, okV, none, i) && c.vol == brute(vol, okV, none, i) && c.green == brute(rng, okAll, notG, i) && c.red == brute(rng, okAll, notR, i);
            strongA += c.ask >= 35; strongB += c.bid >= 35; }
        CHECK(same, "host counts = brute force for ask / bid / V / BGB / BRB on every bar");
        std::cerr << "fixture: ask >= 35 on " << strongA << " bars, bid >= 35 on " << strongB << "\n";
    }
    // the forming bar never changes a closed bar
    {   std::vector<bpr::Counts> before = S->E.C; A.vap.back().push_back(std::make_pair(7000.0f, 99999LL)); A.vapD.back().push_back(99999); x.calc(0);
        bool same = before.size() == S->E.C.size(); for (size_t i = 0; same && i < before.size(); ++i) same = before[i].ask == S->E.C[i].ask && before[i].bid == S->E.C[i].bid;
        CHECK(same && S->formC.ask >= 100, "the forming bar's huge print moves only its own count (" << S->formC.ask << ")"); }
    // the draw
    A.vapReads = 0; A.drawn.clear(); mdraw(x);
    CHECK(A.vapInDraw == 0, "VAP never read in draw()");
    int base = -1; bool up = true, numsOk = true, side = true; int nums = 0, vlab = 0, fills = 0;
    for (const mock::Prim& p : A.drawn) if (p.k == 'F' && p.x1 - p.x0 > 500) base = p.y0;   // the baseline (full width)
    std::map<int, int> gx; std::map<int, int> rx;
    for (const mock::Prim& p : A.drawn) {
        if (p.k == 'F' && p.x1 - p.x0 <= 500) { ++fills; up = up && p.y1 <= base + 1 && p.y0 < p.y1; if (p.col == bpr::C_G || p.col == bpr::C_GD) gx[p.x0] = p.x1; else rx[p.x0] = p.x1; }
        if (p.k == 'T') { if (p.text[0] == 'V') { ++vlab; numsOk = numsOk && std::atoi(p.text.c_str() + 1) >= 35; continue; } ++nums; const int val = std::atoi(p.text.c_str()); numsOk = numsOk && val >= 10 && (p.bold == (val >= 35)); }
    }
    for (auto& g : gx) { auto r = rx.upper_bound(g.first); side = side && (r == rx.end() || r->first >= g.second); }
    CHECK(base > 0 && fills > 0 && up, "green and red bars both go UP from one baseline (" << fills << " bars, base " << base << ")");
    CHECK(side, "green left of red in each slot (side by side, no overlap)");
    CHECK(nums > 0 && numsOk, "numbers only from 10, bold from 35 (" << nums << " numbers, " << vlab << " V labels)");
    // the settings-window copy
    {   mock::Host D; D.dialog = true; D.root = ""; mock::current = &D; BigPrint z; z.calc(0); mdraw(z); CHECK(D.drawn.empty(), "the Add / settings copy with no chart draws nothing"); }
    { std::ifstream f("../BigPrint.cpp"); std::stringstream s; s << f.rdbuf(); const std::string src = s.str(); const size_t p = src.find("p->setVersion(\""); const std::string lit = p == std::string::npos ? "" : src.substr(p + 15, src.find('"', p + 15) - p - 15);
      CHECK(lit == BP_VERSION && lit == bpr::VERSION && lit == "1.0.0", "version literals match (" << lit << ")"); }
    std::cout << "test_bigprint_host: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
