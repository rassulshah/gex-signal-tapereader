// test_cobsignal_host.cpp - lsCobSignal 1.3.0 (1.3.0: + LVP records / restart / draw checks; 1.0.0) host + logic suite on the mock SDK (CobSignal_test_support/irtsdk.h).
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
    for (int a = 0; a < 4; ++a) { H.f[a].clear(); }
    H.t.clear(); H.vol.clear();
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
    // ---- (1.2.0, Rassul 2026-11-02 19:19 CT) = his cob_bull_3m_1b / cob_bear_3m_1b + 11:00 CT: no CoB after 11:00 on either side; PWx / Asia levels count
    {   cobs::Engine A; A.setMarket("ES", 0.25, 180); for (const cobs::Bar& b : F.B) A.addBar(b);
        int late = 0, pwAsia = 0; for (const cobs::Mark& m : A.marks) { const int mod = (int)((m.t % 86400 + 86400) % 86400) / 60; if (mod > 660) ++late;
            if (m.levels.find("PW") != std::string::npos || m.levels.find("Asia") != std::string::npos) ++pwAsia; }
        CHECK(late == 0, "(1.2.0) no CoB after 11:00 CT (" << late << ")");
        CHECK(pwAsia >= 1, "(1.2.0) PWL / PWH / AsiaLO / AsiaHI count as swept levels (" << pwAsia << ")");
        cobs::Engine G; G.setMarket("ES", 0.25, 180); G.legacy = true; for (const cobs::Bar& b : F.B) G.addBar(b);
        int bearLate = 0; for (const cobs::Mark& m : G.marks) if (m.side < 0 && (int)((m.t % 86400 + 86400) % 86400) / 60 > 660) ++bearLate;
        std::cout << "1.2.0: " << A.marks.size() << " CoBs (legacy bear rules: " << G.marks.size() << ", bear after 11:00 " << bearLate << ")\n"; }
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
    // (1.3.0) LVP: every LVP once in the record, LVP lines in the status, the live file stays CoB-only
    size_t nlvp = 0; for (size_t p = rec.find("\nLVP,"); p != std::string::npos; p = rec.find("\nLVP,", p + 1)) ++nlvp;
    std::cerr << "fixture: " << L.lvp.marks.size() << " LVPs after " << L.marks.size() << " CoBs\n";
    CHECK(L.lvp.marks.size() > 0 && nlvp == L.lvp.marks.size(), "record: every LVP once (" << nlvp << " of " << L.lvp.marks.size() << ")");
    { bool all = true; for (const cobs::LvpMark& m : L.lvp.marks) all = all && rec.find(cobs::lvpLine(m) + "\n") != std::string::npos; CHECK(all, "record: the LVP lines = the logic's"); }
    CHECK(st.find("\nLVPS," + std::to_string(L.lvp.marks.size()) + "\n") != std::string::npos && st.find("\nLVP_RULES,") != std::string::npos, "status: LVP rules + count");
    CHECK(live.find("LVP,") == std::string::npos, "live file: CoBs only (unchanged format for its readers)");
    { bool ok = true; for (const cobs::LvpMark& m : L.lvp.marks) { bool f = false; for (const cobs::Mark& c : L.marks) f = f || (c.t == m.cobT && c.side == m.side && c.show == m.show); ok = ok && f && m.vol * 3 <= m.cobVol && m.nth >= 1 && m.nth <= 3 && m.idx > 0; } CHECK(ok, "every LVP belongs to a CoB of its side, <= 1/3 volume, pullback 1-3"); }
    {   // (1.4.0) the live file = the CoBs that passed the ZZO wave filter (the drawn ones)
        bool ok = live.rfind("COBSIGNAL,1.5.1\n", 0) == 0; int drawn = 0;
        for (const cobs::Mark& m : L.marks) { const bool in = live.find(L.recLine(m) + "\n") != std::string::npos; ok = ok && in == (m.show && m.zOk); drawn += m.show && m.zOk; }
        CHECK(ok, "live file: version + every drawn CoB (passed the ZZO wave filter), no other (" << drawn << " of " << L.marks.size() << ")"); }
    CHECK(st.rfind("VERSION,1.5.1\nMARKET,ES", 0) == 0 && st.find("COBS," + std::to_string(L.marks.size())) != std::string::npos && st.find("\nZZO_FILTER,1.4.0") != std::string::npos, "status file (+ ZZO_FILTER)");
    {   // (1.4.0) every decided CoB has one ZZO line in the record; passed = count >= 10; both outcomes occur on a year of ES
        size_t nz = 0; for (size_t p = rec.find("\nZZO,"); p != std::string::npos; p = rec.find("\nZZO,", p + 1)) ++nz;
        int dec = 0, okN = 0, noN = 0; bool cnt = true;
        for (const cobs::Mark& m : L.marks) if (m.zDone) { ++dec; (m.zOk ? okN : noN) += 1; cnt = cnt && (!m.zOk || m.zCnt >= cobs::ZZO_COB_MIN); }
        std::cerr << "ZZO filter: " << okN << " passed, " << noN << " failed of " << L.marks.size() << " CoBs\n";
        CHECK(nz == (size_t)dec && okN > 0 && noN > 0 && cnt, "record: one ZZO line per decided CoB; passed ones have count >= 10 (" << nz << " lines, " << okN << " ok / " << noN << " no)"); }
    // bar-by-bar feeding = one pass; a restart never writes a CoB twice
    {   mock::Host Bh; mock::current = &Bh; Bh.root = "ES"; Bh.sym = "ESZ26"; CobSignal y;
        const int n = (int)F.B.size(); for (int u = n - 400; u < n; ++u) { chart(Bh, F, u, std::max(0, u - 120)); y.calc(0); }
        CSState* S2 = static_cast<CSState*>(Bh.userData);
        CHECK(S2 && sig(S2->E) == sig(L), "bar-by-bar feeding = one pass (no repaint)");
        const std::string rec2 = slurp(flex() + "/CobSignal/ES-180.csv");
        CHECK(rec2 == rec, "a restart (second chart, same market) appends nothing new (CoBs and LVPs)");
        std::string a, b; for (const cobs::LvpMark& m : S2->E.lvp.marks) a += cobs::lvpLine(m); for (const cobs::LvpMark& m : L.lvp.marks) b += cobs::lvpLine(m);
        CHECK(a == b, "(1.3.0) bar-by-bar feeding: the same LVPs (no repaint)");
    }
    // the draw: labels in the pane, bull green under the bar, bear red over it
    mock::current = &A; A.drawn.clear(); mdraw(x);
    int bull = 0, bear = 0, other = 0, lvpN = 0, fibL = 0, fibT = 0;   // (1.5.0) + the Fib lines and their names
    auto isFib = [](const mock::Prim& p) { return p.k == 'L' || (p.k == 'T' && (p.text == "Stop" || p.text == "50%" || p.text == "BE" || p.text == "1.272" || p.text == "1.618" || p.text == "2.0")); };
    for (const mock::Prim& p : A.drawn) { if (p.k == 'L') { ++fibL; if (p.w != 1 || p.y0 != p.y1) ++other; continue; } if (isFib(p)) { ++fibT; continue; } if (p.k != 'T') { ++other; continue; } if (p.text == "LVP") { ++lvpN; continue; } if (p.col == cobs::C_BULL) ++bull; else if (p.col == cobs::C_BEAR) ++bear; }
    {   int lvis = 0; for (const cobs::LvpMark& m : L.lvp.marks) lvis += m.show && m.idx >= A.first;
        CHECK(lvpN <= lvis && (lvis == 0 || lvpN > 0) && bull + bear + lvpN + fibL + fibT == S->drawn, "(1.3.0) LVP labels drawn (" << lvpN << " of " << lvis << " visible)");
        bool col = true, place = true;
        for (const mock::Prim& p : A.drawn) { if (p.text != "LVP") continue; bool f = false;
            for (const cobs::LvpMark& m : L.lvp.marks) { PNT q; q.set(m.idx, 0.0f, kBarCenter); if (m.idx < A.first || std::abs(q.h - (p.x0 + p.x1) / 2) > 3) continue; f = true;
                col = col && p.col == (m.side > 0 ? cobs::C_BULL : cobs::C_BEAR);
                const int yl = (int)x.getVerticalDrawPosition((float)L.B[(size_t)m.idx].l), yh = (int)x.getVerticalDrawPosition((float)L.B[(size_t)m.idx].h);
                place = place && (m.side > 0 ? p.y0 >= yl + 50 : p.y1 <= yh - 50); }
            place = place && f; }
        CHECK(col && place, "(1.3.0) LVP: red above after a bear CoB / green below after a bull CoB, 3 rows clear of the wick");
        bool clear = true; for (size_t i = 0; i < A.drawn.size(); ++i) for (size_t j = i + 1; j < A.drawn.size(); ++j) { const mock::Prim& a = A.drawn[i]; const mock::Prim& b = A.drawn[j]; if (isFib(a) || isFib(b)) continue;
            clear = clear && !(a.x0 < b.x1 - 2 && b.x0 < a.x1 - 2 && a.y0 < b.y1 && b.y0 < a.y1); }
        CHECK(clear, "(1.3.0) no label overlaps another"); }
    int vis = 0, vb = 0; for (const cobs::Mark& m : L.marks) if (m.idx >= A.first) { ++vis; vb += m.side > 0; }
    CHECK(other == 0 && bull + bear > 0 && bull + bear + lvpN + fibL + fibT == S->drawn && bull + bear <= vis && bull <= vb, "only label text drawn (" << bull << " green / " << bear << " red of " << vis << " visible)");
    {   int want = 0; for (const cobs::Mark& m : L.marks) if (cobs::fibDrawn(m)) ++want;   // (1.5.0) every drawn CoB in view brings its lines (1 px, solid, horizontal)
        CHECK(fibL > 0 && fibL <= 6 * want && fibT <= fibL, "(1.5.0) Fib lines drawn through the host (" << fibL << " lines, " << fibT << " names, " << want << " drawn CoBs)"); }
    {   bool place = true; for (const mock::Prim& p : A.drawn) { if (p.k != 'T' || isFib(p)) continue; for (const cobs::Mark& m : L.marks) { if (m.idx < A.first) continue;
            PNT q; q.set(m.idx, 0.0f, kBarCenter); if (std::abs(q.h - (p.x0 + p.x1) / 2) > 6) continue;
            const int yl = (int)x.getVerticalDrawPosition((float)L.B[(size_t)m.idx].l), yh = (int)x.getVerticalDrawPosition((float)L.B[(size_t)m.idx].h);
            place = place && (m.side > 0 ? p.y0 >= yl : p.y0 + 14 <= yh); } }
        CHECK(place, "bull labels under the bar's low, bear labels over its high"); }
    // (1.5.1) his charts are CQG: the root symbol is EP / ENQ / CLE / GCE / CPE / NGE / EU6 -> identified as ES / NQ / CL / GC / HG / NG / EU
    {   const char* R[7][2] = { {"EP", "ES"}, {"ENQ", "NQ"}, {"CLE", "CL"}, {"GCE", "GC"}, {"CPE", "HG"}, {"NGE", "NG"}, {"EU6", "EU"} }; bool ok = true;
        for (const auto& r : R) { mock::Host Q; mock::current = &Q; Q.root = r[0]; Q.sym = std::string(r[0]) + "Z26"; chart(Q, F, (int)F.B.size() - 1, (int)F.B.size() - 600);
            CobSignal q; q.calc(0); CSState* SQ = static_cast<CSState*>(Q.userData);
            ok = ok && SQ && SQ->mktOk && SQ->E.mkt == r[1] && !SQ->E.B.empty() && slurp(flex() + "/CobSignal.status-" + r[1] + "-180.txt").rfind("VERSION,1.5.1", 0) == 0; }
        mock::current = &A;
        CHECK(ok, "(1.5.1) CQG roots EP / ENQ / CLE / GCE / CPE / NGE / EU6 run as ES / NQ / CL / GC / HG / NG / EU (status written)"); }
    // the settings-window copy: no chart, nothing called on it
    {   mock::Host D; D.dialog = true; D.root = ""; mock::current = &D; CobSignal z; z.calc(0); mdraw(z); CHECK(D.drawn.empty(), "the Add / settings copy with no chart draws nothing and does not fault"); }
    // versions
    { const std::string src = slurp("../CobSignal.cpp"); const size_t p = src.find("p->setVersion(\""); const std::string lit = p == std::string::npos ? "" : src.substr(p + 15, src.find('"', p + 15) - p - 15);
      CHECK(lit == CS_VERSION && lit == cobs::VERSION && lit == "1.5.1", "version literals match (" << lit << ")"); }
    std::cout << "test_cobsignal_host: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
