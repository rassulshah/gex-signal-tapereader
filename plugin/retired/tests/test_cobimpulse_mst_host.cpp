// test_cobimpulse_mst_host.cpp -- lsCobImpulse 0.4.1: the Market State post SRC "COB" (MARKET-STATE-SPEC 1.0) on the mock SDK.
// g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -Itests_cobimpulse test_cobimpulse_mst_host.cpp -o tmh && ./tmh tests_cobimpulse/nq_20261014.csv
// The NQ 10-14 fixture (CoBs at 08:39 and 08:54, Abs on the 08:33 pivot), bar by bar:
//  [1] on the CoB's bar (the last closed bar = the CoB bar): cob bull, cob_odds = the decided LIVE p, base, n, cob_tag = the recorded
//      R / V / C letters, cob_gamma = the recorded gamma name; the keys exactly the spec's 6; on the next bar cob = none
//  [2] odds not LIVE (LEARNING) or no odds file: cob_odds '?' (what the label shows), n '?' when unknown
//  [3] decided once: a restart re-posts the same facts for the same bar; re-post only every 10 s; another chart = another entry;
//      destroy() clears; versions 0.4.1 (CI_VERSION == setVersion), the logic stays cimp 0.4.0
#include "irtsdk.h"
namespace mock { Host* current = nullptr; }
#include "CobImpulse.cpp"
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

static int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, std::string(msg).c_str()); } } while (0)
static std::string HOME;
static std::string flex() { return HOME + "\\InvestorRT\\rtx\\lsFlexLevels"; }
static void spit(const std::string& p, const std::string& t) { std::ofstream f(p.c_str(), std::ios::binary | std::ios::trunc); f << t; }
static void wipe() { std::string cmd = "rm -f '" + HOME + "\\'* 2>/dev/null"; (void)!system(cmd.c_str()); }
struct Fix { std::vector<mock::Bar> bars; std::vector<std::string> t; };
static long long epochOf(const std::string& s) {
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
static void setBars(mock::Host& H, const Fix& F, size_t n, int i839, int i854, int i833) {
    H.bars.assign(F.bars.begin(), F.bars.begin() + (long)n);
    std::vector<float> bull(n, 0.0f), bear(n, 0.0f), ab(n, 0.0f), as(n, 0.0f);
    if ((int)n > i839) bull[(size_t)i839] = 1;
    if ((int)n > i854) bull[(size_t)i854] = 1;
    if ((int)n > i833 + 1) ab[(size_t)i833] = 1;
    H.custom["cob_bull_3m_1b"] = bull; H.custom["cob_bear_3m_1b"] = bear; H.custom["Abs_Bull"] = ab; H.custom["Abs_Bear"] = as;
    H.now = H.bars.back().time; H.priceBase = 30300;
}
static long long ct(const std::string& s) { const long long e = epochOf(s); return e; }   // mock getLocaltime = gmtime: CT seconds = the epoch value
static const char* ODDS_A =
    "VERSION|1\nMARKET|NQ\nBUILT|2026-10-01 01:00|test\n"
    "Q|COB_1R|CoB reaches 1R before the stop|base|0.3190|n|69|keys|side,risk,legb,hv|status|LIVE|calib|0.070\n"
    "P|COB_1R|*|*|*|*|p|0.3190|n|464|lo|0.28|hi|0.36\n"
    "P|COB_1R|long|*|*|*|p|0.3300|n|265|lo|0.28|hi|0.38\n"
    "P|COB_1R|short|*|*|*|p|0.3000|n|199|lo|0.25|hi|0.35\n";
int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/cimpmXXXXXX"; HOME = mkdtemp(tmpl); setenv("USERPROFILE", HOME.c_str(), 1);
    const Fix F = load(argc > 1 ? argv[1] : "nq_20261014.csv");
    const int i839 = idx(F, "2026-10-14 08:39:00"), i854 = idx(F, "2026-10-14 08:54:00"), i833 = idx(F, "2026-10-14 08:33:00");
    CHECK(i839 > 0 && i854 > 0 && i833 > 0, "fixture loaded");
    std::string at839;
    {   // [1]
        wipe(); spit(flex() + "\\LRA-Odds-NQ.csv", ODDS_A);
        mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        bool sawNone = false;
        for (size_t n = 200; n <= (size_t)i839 + 2; ++n) {   // bars[n-1] is the forming bar: n = i839 + 2 closes 08:39
            setBars(H, F, n, i839, i854, i833); p->calc(0); p->draw();
            const mst::Facts M = mst::read("COB", "NQZ6", 180, "NQ 3 Min", -1);
            if (n < (size_t)i839 + 2 && M.ok && mst::getS(M, "cob") == "none") sawNone = true;
        }
        CHECK(sawNone, "bars without a CoB: cob = none");
        const mst::Facts M = mst::read("COB", "NQZ6", 180, "NQ 3 Min", -1);   // the last closed bar = 08:39, the CoB bar
        const std::set<std::string> want = { "cob", "cob_odds", "cob_odds_base", "cob_odds_n", "cob_tag", "cob_gamma" };
        std::set<std::string> got; for (const auto& kv : M.kv) got.insert(kv.first);
        CHECK(M.ok && got == want, "COB keys = the spec's 6");
        CHECK(M.bar == ct("2026-10-14 08:39:00"), "posted for the CoB's bar (the last closed bar)");
        CIState* S = p->testState();
        const auto it = S->recs.find(std::make_pair((long long)F.bars[(size_t)i839].time, 1));
        CHECK(it != S->recs.end(), "the 08:39 CoB is recorded");
        if (it != S->recs.end()) {
            std::string tag; if (it->second.r()) tag += "R"; if (it->second.v()) tag += "V"; if (it->second.c()) tag += "C";
            CHECK(mst::getS(M, "cob") == "bull" && mst::getS(M, "cob_tag") == tag && mst::getS(M, "cob_gamma") == it->second.gCode, "cob bull, the recorded tag '" + tag + "' and gamma '" + it->second.gCode + "'");
            CHECK(tag.find('V') != std::string::npos, "08:39 = V (lesson 50: biggest volume in 100)");
        }
        CHECK(mst::getS(M, "cob_odds") == "0.33" && mst::getS(M, "cob_odds_base") == "0.319" && mst::getS(M, "cob_odds_n") == "265", "odds: the decided LIVE p 0.33, base 0.319, n 265: " + mst::getS(M, "cob_odds") + " " + mst::getS(M, "cob_odds_base") + " " + mst::getS(M, "cob_odds_n"));
        at839 = mst::encode(0, M.bar, M.kv);
        setBars(H, F, (size_t)i839 + 3, i839, i854, i833); p->calc(0); p->draw();
        const mst::Facts N = mst::read("COB", "NQZ6", 180, "NQ 3 Min", -1);
        CHECK(N.ok && N.bar == ct("2026-10-14 08:42:00") && mst::getS(N, "cob") == "none" && mst::getS(N, "cob_tag").empty() && mst::getS(N, "cob_odds") == "?", "the next bar: cob none");
        // re-post only every 10 s (same stamp), a new bar at once
        std::string r1; bb::bbGet(mst::keyOf("COB", "NQZ6", 180, "NQ 3 Min"), r1); p->draw(); std::string r2; bb::bbGet(mst::keyOf("COB", "NQZ6", 180, "NQ 3 Min"), r2);
        CHECK(r1 == r2, "no re-post within 10 s");
        // another chart of the same symbol: its own entry; destroy clears only this chart's
        mock::Host H2; H2.chart = "NQ 3 Min #2"; mock::current = &H2; CobImpulse* q = static_cast<CobImpulse*>(CreateExtension()); q->setup();
        setBars(H2, F, (size_t)i839 + 2, i839, i854, i833); q->calc(0); q->draw();
        CHECK(mst::read("COB", "NQZ6", 180, "NQ 3 Min #2", -1).ok && mst::read("COB", "NQZ6", 180, "NQ 3 Min", -1).ok, "two charts, two entries");
        q->destroy(); delete q;
        CHECK(!mst::read("COB", "NQZ6", 180, "NQ 3 Min #2", -1).ok && mst::read("COB", "NQZ6", 180, "NQ 3 Min", -1).ok, "destroy() clears its own entry only");
        mock::current = &H; p->destroy(); delete p;
        CHECK(!mst::read("COB", "NQZ6", 180, "NQ 3 Min", -1).ok, "destroy() cleared");
    }
    {   // [3] restart: the same facts for the same bar (decided once, from the records)
        mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        setBars(H, F, (size_t)i839 + 2, i839, i854, i833); p->calc(0); p->draw();
        const mst::Facts M = mst::read("COB", "NQZ6", 180, "NQ 3 Min", -1);
        CHECK(M.ok && mst::encode(0, M.bar, M.kv) == at839, "after a restart: byte-identical facts for the CoB bar");
        p->destroy(); delete p;
    }
    {   // [2] LEARNING odds / no odds file
        wipe(); std::string learn(ODDS_A); learn.replace(learn.find("status|LIVE"), 11, "status|LEARNING"); spit(flex() + "\\LRA-Odds-NQ.csv", learn);
        mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        setBars(H, F, (size_t)i839 + 2, i839, i854, i833); p->calc(0); p->draw();
        const mst::Facts M = mst::read("COB", "NQZ6", 180, "NQ 3 Min", -1);
        CHECK(M.ok && mst::getS(M, "cob") == "bull" && mst::getS(M, "cob_odds") == "?" && mst::getS(M, "cob_odds_n") == "265", "LEARNING: cob_odds '?' (the label shows none), n kept");
        p->destroy(); delete p;
        wipe();
        mock::Host H2; mock::current = &H2; p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        setBars(H2, F, (size_t)i839 + 2, i839, i854, i833); p->calc(0); p->draw();
        const mst::Facts N = mst::read("COB", "NQZ6", 180, "NQ 3 Min", -1);
        CHECK(N.ok && mst::getS(N, "cob_odds") == "?" && mst::getS(N, "cob_odds_n") == "?" && mst::getS(N, "cob_odds_base") == "?", "no odds file: '?'");
        p->destroy(); delete p;
    }
    {   std::ifstream f("CobImpulse.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string s = ss.str();
        CHECK(std::string(CI_VERSION) == "0.4.1" && s.find("p->setVersion(\"0.4.1\");") != std::string::npos && std::string(cimp::VERSION) == "0.4.0", "versions: plugin 0.4.1, logic 0.4.0");
    }
    std::string cmd = "rm -rf '" + HOME + "'*"; (void)!system(cmd.c_str());
    std::printf("cobimpulse 0.4.1 market state: %d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
