// test_cobimpulse_odds_host.cpp -- lsCobImpulse 0.4.0 live odds on the mock SDK (tests_cobimpulse/irtsdk.h)
// g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -Itests_cobimpulse test_cobimpulse_odds_host.cpp -o toh && ./toh tests_cobimpulse/nq_20261014.csv
#include "irtsdk.h"
namespace mock { Host* current = nullptr; }
#include "CobImpulse.cpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

static int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); } } while (0)

static std::string HOME;
static std::string flex() { return HOME + "\\InvestorRT\\rtx\\lsFlexLevels"; }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void spit(const std::string& p, const std::string& t) { std::ofstream f(p.c_str(), std::ios::binary | std::ios::trunc); f << t; }
static void wipe() {   // every file the plugin writes in HOME's flat namespace
    const std::string pre = HOME + "\\";
    std::string cmd = "rm -f '" + pre + "'* 2>/dev/null"; (void)!system(cmd.c_str());
}

struct Fix { std::vector<mock::Bar> bars; std::vector<std::string> t; };
static long long epochOf(const std::string& s) {   // "2026-10-14 08:39:00" as Central wall time
    struct tm m; std::memset(&m, 0, sizeof(m));
    std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &m.tm_year, &m.tm_mon, &m.tm_mday, &m.tm_hour, &m.tm_min, &m.tm_sec);
    m.tm_year -= 1900; m.tm_mon -= 1; return (long long)timegm(&m);
}
static Fix load(const char* path) {
    Fix F; std::ifstream in(path); std::string line; std::getline(in, line);
    while (std::getline(in, line)) {
        std::stringstream ss(line); std::string f[6]; for (int k = 0; k < 6; ++k) std::getline(ss, f[k], ',');
        mock::Bar b; b.time = (unsigned long)epochOf(f[0]); b.open = std::stof(f[1]); b.high = std::stof(f[2]); b.low = std::stof(f[3]); b.close = std::stof(f[4]);
        b.volume = (unsigned long)std::stod(f[5]); F.bars.push_back(b); F.t.push_back(f[0]);
    }
    return F;
}
static int idx(const Fix& F, const char* t) { for (size_t i = 0; i < F.t.size(); ++i) if (F.t[i] == t) return (int)i; return -1; }

// a host for one chart with the first n bars (+1 forming bar) of the fixture, his RTL flags as IRT would compute them
static void setBars(mock::Host& H, const Fix& F, size_t n, int i839, int i854, int i833, bool absOnPivot) {
    H.bars.assign(F.bars.begin(), F.bars.begin() + (long)n);
    std::vector<float> bull(n, 0.0f), bear(n, 0.0f), ab(n, 0.0f), as(n, 0.0f);
    if (i839 >= 0 && (int)n > i839) bull[(size_t)i839] = 1;
    if (i854 >= 0 && (int)n > i854) bull[(size_t)i854] = 1;
    if (absOnPivot && (int)n > i833 + 1) ab[(size_t)i833] = 1;       // IRT knows the Abs pivot once the next bar exists
    H.custom["cob_bull_3m_1b"] = bull; H.custom["cob_bear_3m_1b"] = bear; H.custom["Abs_Bull"] = ab; H.custom["Abs_Bear"] = as;
    H.now = H.bars.back().time; H.priceBase = 30300;
}
static std::vector<mock::Draw> texts(const mock::Host& H, const std::string& t) {
    std::vector<mock::Draw> o; for (const auto& d : H.draws) if ((d.type == "ctext" || d.type == "text") && d.text.find(t) != std::string::npos) o.push_back(d); return o;
}


static std::string sidecars() {
    std::string out; std::string cmd = "cat '" + HOME + "\\'*CobImpulse\\\\NQ*.odds.csv 2>/dev/null"; FILE* f = popen(cmd.c_str(), "r"); char b[4096]; size_t k;
    while ((k = fread(b, 1, sizeof(b), f)) > 0) out.append(b, k);
    pclose(f);
    return out;
}
static const char* ODDS_A =
    "VERSION|1\nMARKET|NQ\nBUILT|2026-10-01 01:00|test\n"
    "Q|COB_1R|CoB reaches 1R before the stop|base|0.3190|n|69|keys|side,risk,legb,hv|status|LIVE|calib|0.070\n"
    "P|COB_1R|*|*|*|*|p|0.3190|n|464|lo|0.28|hi|0.36\n"
    "P|COB_1R|long|*|*|*|p|0.3300|n|265|lo|0.28|hi|0.38\n"
    "P|COB_1R|short|*|*|*|p|0.3000|n|199|lo|0.25|hi|0.35\n";
int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/cimpoXXXXXX"; HOME = mkdtemp(tmpl); setenv("USERPROFILE", HOME.c_str(), 1);
    const Fix F = load(argc > 1 ? argv[1] : "nq_20261014.csv");
    const int i839 = idx(F, "2026-10-14 08:39:00"), i854 = idx(F, "2026-10-14 08:54:00"), i833 = idx(F, "2026-10-14 08:33:00");
    CHECK(i839 > 0 && i854 > 0 && i833 > 0, "fixture loaded");
    // 1) odds file present: every CoB gets its odds at its close, "Cob 33%" on the chart, the hover line, the sidecar
    std::string side1;
    {
        wipe(); (void)!system(("mkdir -p '" + HOME + "' 2>/dev/null").c_str());
        spit(flex() + "\\LRA-Odds-NQ.csv", ODDS_A);
        mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        for (size_t n = 200; n <= F.bars.size(); ++n) { setBars(H, F, n, i839, i854, i833, true); p->calc(0); p->draw(); }
        CIState* S = p->testState();
        CHECK(S && S->recs.size() == 2 && S->oddsRec.size() == 2, "two CoBs, both with odds decided");
        for (const auto& kv : S->oddsRec) {
            CHECK(kv.second.decided && kv.second.p > 0.2 && kv.second.p < 0.5 && kv.second.n >= 20 && kv.second.live, "odds known and live");
            CHECK(kv.second.facts.find("side=long") != std::string::npos && kv.second.facts.find("legb=") != std::string::npos && kv.second.facts.find("hv=") != std::string::npos, "facts recorded");
        }
        H.draws.clear(); p->draw();
        CHECK(texts(H, " 33%").size() == 2, "both labels end with the odds (long row 33%)");
        auto L = texts(H, "Cob");
        RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)((L[0].l + L[0].r) / 2); e.v.mouse.v = (short)((L[0].t + L[0].b) / 2);
        hb::fakeCursor() = 77; p->draw(); p->mouse(&e); H.draws.clear(); p->draw();
        bool hl = false; for (const auto& d : H.draws) if (d.text.find("Odds: 33% reach 1R before the stop (usual here 32%") != std::string::npos) hl = true;
        /* (0.5.1, Rassul 23:37 CT 2026-10-18 hovers off: with CI_HOVERS false the mouse must change nothing) */ CHECK(CI_HOVERS ? hl : !hl, CI_HOVERS ? "hover: the odds line with the usual rate and cases" : "hovers off: no odds hover");
        hb::fakeCursor() = 0;
        S->lastStatusMs = -1; p->calc(0);
        std::string st = slurp(flex() + "\\CobImpulse.status-NQ-NQZ6-180.txt");
        CHECK(st.find(std::string("VERSION,") + CI_VERSION) != std::string::npos && st.find("ODDS,LRA-Odds-NQ.csv,ok, built 2026-10-01 01:00,show 1,decided 2") != std::string::npos, "status: odds line");
        // 2) ODDS,0 hides the number, keeps the record
        spit(flex() + "\\LRA-CobImpulse-NQ.csv", "VERSION,1\nMARKET,NQ\nODDS,0\n");
        S->paramsCheckMs = -1; S->paramsStamp = -3; H.draws.clear(); p->draw();
        CHECK(texts(H, "%").empty() && S->oddsRec.size() == 2, "ODDS,0: no number on the chart, odds still recorded");
        std::remove((flex() + "\\LRA-CobImpulse-NQ.csv").c_str());
        p->destroy(); delete p;
        side1 = sidecars();
        CHECK(side1.find("trig_time|side|p|n|base|live|key|facts") != std::string::npos && side1.find("|bull|0.3300|265|0.3190|1|long|*|*|*|") != std::string::npos, "sidecar lines");
    }
    // 3) NEVER REPAINTS: a new odds file tonight changes nothing already decided (restart reads the sidecar)
    {
        spit(flex() + "\\LRA-Odds-NQ.csv", std::string(ODDS_A).replace(std::string(ODDS_A).find("long|*|*|*|p|0.3300"), 19, "long|*|*|*|p|0.9000"));
        mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        setBars(H, F, F.bars.size(), i839, i854, i833, true); p->calc(0); H.draws.clear(); p->draw();
        CHECK(texts(H, " 33%").size() == 2 && texts(H, " 90%").empty(), "decided odds kept after the table changed (no repaint)");
        p->destroy(); delete p;
        CHECK(sidecars() == side1, "sidecar byte-identical");
    }
    // 4) restart after every bar = byte-identical sidecar
    {
        wipe(); spit(flex() + "\\LRA-Odds-NQ.csv", ODDS_A);
        mock::Host H; mock::current = &H;
        for (size_t n = 200; n <= F.bars.size(); ++n) {
            CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
            setBars(H, F, n, i839, i854, i833, true); p->calc(0); p->draw(); p->destroy(); delete p;
        }
        CHECK(sidecars() == side1, "restart after every bar = byte-identical odds sidecar");
    }
    // 5) no odds file: no number, the hover says '?', a LEARNING question is not shown
    {
        wipe();
        mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        setBars(H, F, F.bars.size(), i839, i854, i833, true); p->calc(0); H.draws.clear(); p->draw();
        CHECK(texts(H, "%").empty() && p->testState()->oddsRec.size() == 2, "missing file: no number, still decided (unknown)");
        for (const auto& kv : p->testState()->oddsRec) CHECK(kv.second.p < 0 && codds::hoverLine(kv.second).find("Odds: ?") == 0, "unknown -> '?' in the hover");
        p->destroy(); delete p;
        wipe(); std::string learn(ODDS_A); learn.replace(learn.find("status|LIVE"), 11, "status|LEARNING");
        spit(flex() + "\\LRA-Odds-NQ.csv", learn);
        mock::Host H2; mock::current = &H2; p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        setBars(H2, F, F.bars.size(), i839, i854, i833, true); p->calc(0); H2.draws.clear(); p->draw();
        CHECK(texts(H2, "%").empty(), "LEARNING question: not shown on the chart");
        for (const auto& kv : p->testState()->oddsRec) CHECK(codds::hoverLine(kv.second).find("Still learning") != std::string::npos, "hover says still learning");
        p->destroy(); delete p;
    }
    // 6) (1.0.1) a table built AFTER the CoBs: no in-sample odds - not decided, nothing drawn, the hover says why, the sidecar keeps -2
    {
        wipe(); std::string late(ODDS_A); late.replace(late.find("2026-10-01 01:00"), 16, "2026-10-18 01:00");
        spit(flex() + "\\LRA-Odds-NQ.csv", late);
        mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        setBars(H, F, F.bars.size(), i839, i854, i833, true); p->calc(0); H.draws.clear(); p->draw();
        CHECK(texts(H, "%").empty() && p->testState()->oddsRec.size() == 2, "table newer than the CoBs: no number");
        for (const auto& kv : p->testState()->oddsRec) CHECK(!kv.second.decided && codds::hoverLine(kv.second).find("before tonight") != std::string::npos, "not decided, hover says why");
        p->destroy(); delete p;
        mock::Host H2; mock::current = &H2; p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        setBars(H2, F, F.bars.size(), i839, i854, i833, true); p->calc(0);
        for (const auto& kv : p->testState()->oddsRec) CHECK(!kv.second.decided, "restart keeps 'not decided' (sidecar -2)");
        p->destroy(); delete p;
    }
    std::printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
