// test_cobsignal_host.cpp - lsCobSignal 1.0.0 host + logic suite on the mock SDK (CobSignal_test_support/irtsdk.h).
//   (in plugin\tests_cobsignal) g++ -std=c++17 -Wall -Wextra -ICobSignal_test_support -I.. test_cobsignal_host.cpp -o th && ./th cobsignal_es.csv
// Covers: the ES fixture (the last 40 sessions of a year of 3-min bars) through the real host = the logic run on the same bars;
// PHH / PHL add CoBs (useNew); labels Cob / Cob+R / Cob+V / Cob+RV; bar-by-bar feeding = one pass (no repaint); the forming bar is never
// read; the record written once (a restart never writes a CoB twice); the live file; labels under bull / over bear bars in the pane,
// green / red; the settings-window copy (no chart) never calls into the chart; the status file; the version literals.
#include "irtsdk.h"
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
#include "CobSignal.cpp"

static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)

struct Fix { std::vector<cobs::Bar> B; };
static Fix load(const char* path) {
    Fix F; std::ifstream f(path); std::string l; std::getline(f, l);
    while (std::getline(f, l)) { std::stringstream ss(l); std::string tok; std::vector<std::string> x; while (std::getline(ss, tok, ',')) x.push_back(tok);
        cobs::Bar b; b.t = std::atoll(x[0].c_str()); b.o = std::atof(x[1].c_str()); b.h = std::atof(x[2].c_str()); b.l = std::atof(x[3].c_str()); b.c = std::atof(x[4].c_str()); b.v = std::atof(x[5].c_str()); F.B.push_back(b); }
    return F;
}
static void chart(mock::Host& H, const Fix& F, int upto, int first) {
    for (int a = 0; a < 4; ++a) H.f[a].clear(); H.t.clear(); H.vol.clear();
    for (int i = 0; i <= upto; ++i) { const cobs::Bar& b = F.B[(size_t)i]; H.f[0].push_back((float)b.o); H.f[1].push_back((float)b.h); H.f[2].push_back((float)b.l); H.f[3].push_back((float)b.c); H.t.push_back((RTDATE)b.t); H.vol.push_back((RTDATE)b.v); }
    float lo = 1e9f, hi = -1e9f; for (int i = first; i <= upto; ++i) { lo = std::min(lo, H.f[2][(size_t)i]); hi = std::max(hi, H.f[1][(size_t)i]); }
    H.lo = lo - 3; H.hi = hi + 3; H.first = first; H.paneR = 10 * (upto - first + 1) + H.rightMargin; H.paneB = 900;
}
static std::string home;
static std::string flex() { return home + "/InvestorRT/rtx/lsFlexLevels"; }
static std::string slurp(const std::string& p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void mdraw(CobSignal& x) { mock::current->drawing = true; x.draw(); mock::current->drawing = false; }
static std::string sig(const cobs::Engine& E) { std::string s; for (const cobs::Mark& m : E.marks) s += std::to_string(m.t) + m.label() + m.levels + ";"; return s; }

int main(int argc, char** argv) {
    const Fix F = load(argc > 1 ? argv[1] : "cobsignal_es.csv");
    CHECK(F.B.size() > 5000, "fixture loaded (" << F.B.size() << " bars)");
    char tmpl[] = "/tmp/cobsig-XXXXXX"; home = mkdtemp(tmpl); setenv("USERPROFILE", home.c_str(), 1);
    mkdir((home + "/InvestorRT").c_str(), 0755); mkdir((home + "/InvestorRT/rtx").c_str(), 0755); mkdir(flex().c_str(), 0755);
    // ---- the logic alone on the same bars (closed bars = all but the last)
    cobs::Engine L; L.setMarket("ES", 0.25, 180); for (size_t i = 0; i + 1 < F.B.size(); ++i) L.addBar(F.B[i]);
    cobs::Engine O; O.setMarket("ES", 0.25, 180); O.useNew = false; for (size_t i = 0; i + 1 < F.B.size(); ++i) O.addBar(F.B[i]);
    int r = 0, v = 0, ph = 0; for (const cobs::Mark& m : L.marks) { r += m.r; v += m.v; ph += m.levels.find("PH") != std::string::npos; }
    std::cerr << "fixture: " << L.marks.size() << " CoBs (" << O.marks.size() << " without PHH / PHL), +R " << r << ", +V " << v << ", using PHH / PHL " << ph << "\n";
    CHECK(L.marks.size() > O.marks.size() && ph > 0, "PHH / PHL add CoBs (" << L.marks.size() << " vs " << O.marks.size() << ")");
    { bool sub = true; for (const cobs::Mark& m : O.marks) { bool f = false; for (const cobs::Mark& n : L.marks) f = f || (n.t == m.t && n.side == m.side); sub = sub && f; } CHECK(sub, "every CoB without PHH / PHL is still a CoB with them"); }
    { bool lab = true; for (const cobs::Mark& m : L.marks) { const std::string s = m.label(); lab = lab && (s == "Cob" || s == "Cob+R" || s == "Cob+V" || s == "Cob+RV") && (s.find('R') != std::string::npos) == m.r && (s.find('V') != std::string::npos) == m.v; } CHECK(lab, "labels Cob / Cob+R / Cob+V / Cob+RV only, matching the tags"); }
    { bool rules = true; for (const cobs::Mark& m : L.marks) { const int mod = (int)(m.t % 86400) / 60; rules = rules && m.dratio >= 1.9 && m.legEnd - m.legStart >= 3 && (m.side < 0 || mod <= 660) && mod >= 510 && mod <= 900; } CHECK(rules, "every CoB: dratio >= 1.9, >= 3 bars, RTH, bulls by 11:00"); }
    // ---- (1.1.0) PHH / PHL-only CoBs: drawn in ES / HG / GC, recorded (LOG) but not drawn in the other markets
    {   int phOnly = 0, esShow = 0; for (const cobs::Mark& m : L.marks) { phOnly += m.phOnly; esShow += m.phOnly && m.show; }
        CHECK(phOnly > 0 && esShow == phOnly, "ES: every PHH / PHL-only CoB is drawn (" << esShow << " of " << phOnly << ")");
        cobs::Engine C; C.setMarket("CL", 0.25, 180); for (size_t i = 0; i + 1 < F.B.size(); ++i) C.addBar(F.B[i]);   // the same bars as "CL"
        int cOnly = 0, cShow = 0, other = 0, otherShow = 0; for (const cobs::Mark& m : C.marks) { cOnly += m.phOnly; cShow += m.phOnly && m.show; other += !m.phOnly; otherShow += !m.phOnly && m.show; }
        CHECK(C.marks.size() == L.marks.size() && cOnly == phOnly && cShow == 0 && otherShow == other, "CL: PHH / PHL-only CoBs recorded but not shown, all others shown (" << cOnly << " log, " << otherShow << " shown)");
        bool logTag = true; for (const cobs::Mark& m : C.marks) { const std::string l = C.recLine(m); logTag = logTag && l.size() > 4 && l.compare(l.size() - 4, 4, m.show ? "SHOW" : ",LOG") == 0; }
        CHECK(logTag, "record lines end with SHOW / LOG");
        cobs::View cv; cv.first = 0; cv.last = (int)C.B.size() - 1; cv.t = -1000000; cv.b = 1000000; cv.X = [](int i) { return (double)i * 10; }; cv.Y = [](double p) { return -p * 4; };
        const auto P = cobs::layout(C, cv); int drawnPh = 0; for (const auto& p : P) for (const cobs::Mark& m : C.marks) if (m.phOnly && std::abs(p.x0 + (p.x1 - p.x0) / 2 - (int)std::lround(cv.X(m.idx))) <= 1) ++drawnPh;
        CHECK(drawnPh == 0 && (int)P.size() > 0, "CL: no PHH / PHL-only label in the drawing (" << P.size() << " labels)");
    }
    // ---- the host: whole chart in one calc
    mock::Host A; mock::current = &A; A.root = "ES"; A.sym = "ESZ26"; chart(A, F, (int)F.B.size() - 1, (int)F.B.size() - 3000);
    CobSignal x; x.calc(0);
    CSState* S = static_cast<CSState*>(A.userData);
    CHECK(S && S->mktOk && S->E.mkt == "ES", "host identified ES");
    CHECK(S && sig(S->E) == sig(L), "host = logic on the same closed bars (" << (S ? S->E.marks.size() : 0) << " vs " << L.marks.size() << ")");
    // the forming bar is never read: change it, nothing changes
    A.f[1].back() += 50; A.f[2].back() -= 50; x.calc(0); CHECK(S && sig(S->E) == sig(L), "the forming bar is never read");
    // the record / live file / status
    const std::string rec = slurp(flex() + "/CobSignal/ES-180.csv"), live = slurp(flex() + "/CobSignal-ES-180-live.csv"), st = slurp(flex() + "/CobSignal.status-ES-180.txt");
    size_t nrec = 0; for (size_t p = rec.find("\nCOB,"); p != std::string::npos; p = rec.find("\nCOB,", p + 1)) ++nrec;
    CHECK(nrec == L.marks.size(), "record: every CoB once (" << nrec << ")");
    CHECK(live.rfind("COBSIGNAL,1.1.0\n", 0) == 0 && live.find(L.recLine(L.marks.back())) != std::string::npos, "live file: version + every CoB");
    CHECK(st.rfind("VERSION,1.1.0\nMARKET,ES", 0) == 0 && st.find("COBS," + std::to_string(L.marks.size())) != std::string::npos, "status file");
    // bar-by-bar feeding = one pass; a restart never writes a CoB twice
    {   mock::Host Bh; mock::current = &Bh; Bh.root = "ES"; Bh.sym = "ESZ26"; CobSignal y;
        const int n = (int)F.B.size(); for (int u = n - 400; u < n; ++u) { chart(Bh, F, u, std::max(0, u - 120)); y.calc(0); }
        CSState* S2 = static_cast<CSState*>(Bh.userData);
        CHECK(S2 && sig(S2->E) == sig(L), "bar-by-bar feeding = one pass (no repaint)");
        const std::string rec2 = slurp(flex() + "/CobSignal/ES-180.csv");
        CHECK(rec2 == rec, "a restart (second chart, same market) appends nothing new");
    }
    // the draw: labels in the pane, bull green under the bar, bear red over it
    mock::current = &A; A.drawn.clear(); mdraw(x);
    int bull = 0, bear = 0, other = 0;
    for (const mock::Prim& p : A.drawn) { if (p.k != 'T') { ++other; continue; } if (p.col == cobs::C_BULL) ++bull; else if (p.col == cobs::C_BEAR) ++bear; }
    int vis = 0, vb = 0; for (const cobs::Mark& m : L.marks) if (m.idx >= A.first) { ++vis; vb += m.side > 0; }
    CHECK(other == 0 && bull + bear > 0 && bull + bear == S->drawn && bull + bear <= vis && bull <= vb, "only label text drawn (" << bull << " green / " << bear << " red of " << vis << " visible)");
    {   bool place = true; for (const mock::Prim& p : A.drawn) { if (p.k != 'T') continue; for (const cobs::Mark& m : L.marks) { if (m.idx < A.first) continue;
            PNT q; q.set(m.idx, 0.0f, kBarCenter); if (std::abs(q.h - (p.x0 + p.x1) / 2) > 6) continue;
            const int yl = (int)x.getVerticalDrawPosition((float)L.B[(size_t)m.idx].l), yh = (int)x.getVerticalDrawPosition((float)L.B[(size_t)m.idx].h);
            place = place && (m.side > 0 ? p.y0 >= yl : p.y0 + 14 <= yh); } }
        CHECK(place, "bull labels under the bar's low, bear labels over its high"); }
    // the settings-window copy: no chart, nothing called on it
    {   mock::Host D; D.dialog = true; D.root = ""; mock::current = &D; CobSignal z; z.calc(0); mdraw(z); CHECK(D.drawn.empty(), "the Add / settings copy with no chart draws nothing and does not fault"); }
    // versions
    { const std::string src = slurp("../CobSignal.cpp"); const size_t p = src.find("p->setVersion(\""); const std::string lit = p == std::string::npos ? "" : src.substr(p + 15, src.find('"', p + 15) - p - 15);
      CHECK(lit == CS_VERSION && lit == cobs::VERSION && lit == "1.1.0", "version literals match (" << lit << ")"); }
    std::cout << "test_cobsignal_host: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
