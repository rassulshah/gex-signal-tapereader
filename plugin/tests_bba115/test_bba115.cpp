// test_bba115.cpp -- lsBigBidAsk 1.15.0 (2026-10-18, audit B finding 2): size tiers decided once per session, never re-judged.
//   [logic] bba::tiersFor / bigLotOf / catSizeOf: only complete sessions BEFORE the judged one; latest 5; under 3 = learning; a later or
//           partial session never changes an earlier session's tiers (replay = straight pass)
//   [host]  the chart's tiers for today stay identical while today's session grows (old code: moving window incl. today); the tiers
//           and the SignalBoard B events do not depend on the view (narrow view / scrolled back / full); today's partial session is
//           never pooled; B events = closed candles from the record (+ bit 7 pivot confirmed); a restart gives the same tiers
// Build (from tests_bba115/): g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. -I.. test_bba115.cpp -o /tmp/t_bba115 && /tmp/t_bba115
#include "irtsdk.h"
#include "../BigBidAsk.cpp"
#include <iostream>
#include <random>
#include <sys/stat.h>
namespace mock { Host* current = nullptr; }
static int passed = 0, failed = 0;
#define CHECK(c, why) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL line " << __LINE__ << ": " << why << "\n"; } } while (0)

static void logicTests() {
    std::map<long, bba::SessPrints> m;
    auto sess = [&](long k, int day, long base) { bba::SessPrints& P = m[k]; P.dayCandles = day; P.pv[0].clear(); P.pv[1].clear();
        for (int i = 0; i < 100; ++i) { P.pv[1].push_back(base + i); P.pv[0].push_back(base / 2 + i); } };
    for (long k = 1; k <= 7; ++k) sess(k, 120, 100 * k);
    sess(8, 40, 100000);                                            // today: partial (40 day candles), huge prints
    const int need = 100;
    bba::TierSet T = bba::tiersFor(8, m, need);
    CHECK(T.ok && T.nSess == 5 && T.used.front() == 7 && T.used.back() == 3, "[logic] the latest 5 complete sessions before today");
    CHECK(T.pv[1].size() == 500 && T.pv[1].front() == 799 && T.pv[1].back() == 300, "[logic] pooled prints, biggest first, today excluded");
    CHECK(bba::bigLotOf(T, 1, 5) == T.pv[1][24] && bba::bigLotOf(T, 1, 20) == T.pv[1][99], "[logic] Big Lot N = rank N x sessions");
    bba::Cat c; c.n = 5; c.lots = 0; bba::Cat c4; c4.n = 0; c4.lots = 1350; bba::Cat cm; cm.n = 5; cm.lots = 2000;
    CHECK(bba::catSizeOf(T, 1, c) == T.pv[1][24] && bba::catSizeOf(T, 1, c4) == 1350 && bba::catSizeOf(T, 1, cm) == 2000, "[logic] category 1-3 / 4 / minimum");
    sess(8, 400, 999999); sess(9, 120, 5);                          // today completes, a later session appears
    bba::TierSet T2 = bba::tiersFor(8, m, need);
    CHECK(T2.pv[1] == T.pv[1] && T2.pv[0] == T.pv[0] && T2.used == T.used, "[logic] today's own prints and later sessions never change today's tiers");
    m[5].dayCandles = 10;                                            // a partly read session is not complete: skipped, the next older one used
    bba::TierSet T3 = bba::tiersFor(8, m, need);
    CHECK(T3.nSess == 5 && std::find(T3.used.begin(), T3.used.end(), 5L) == T3.used.end() && T3.used.back() == 2, "[logic] incomplete session skipped");
    std::map<long, bba::SessPrints> few; few[1] = m[1]; few[2] = m[2];
    bba::TierSet L = bba::tiersFor(3, few, need);
    CHECK(!L.ok && bba::bigLotOf(L, 1, 5) == 0 && bba::catSizeOf(L, 1, c) == 0 && bba::catSizeOf(L, 1, c4) == 1350, "[logic] under 3 complete sessions = learning (category 4 still works)");
    CHECK(L.why.find("learning") == 0, "[logic] learning reason");
    bba::TierSet S2; S2.ok = true; S2.nSess = 3; S2.pv[1].assign(29, 50);
    CHECK(bba::bigLotOf(S2, 1, 5) == 0, "[logic] fewer than 30 prints = no tier");
    // replay = straight: grow a session candle by candle; the tiers of every session judged on the way equal the final pass
    std::mt19937 rng(7); std::map<long, bba::SessPrints> live; std::map<long, bba::TierSet> decided; bool same = true;
    for (long k = 1; k <= 9; ++k) for (int i = 0; i < 150; ++i) {
        bba::SessPrints& P = live[k]; ++P.dayCandles; P.pv[1].push_back((long long)(rng() % 900 + 10));
        if (!decided.count(k)) decided[k] = bba::tiersFor(k, live, need);   // decided at the session's first candle
    }
    for (long k = 1; k <= 9; ++k) { const bba::TierSet F = bba::tiersFor(k, live, need); if (F.pv[1] != decided[k].pv[1] || F.ok != decided[k].ok) same = false; }
    CHECK(same, "[logic] tiers decided at a session's first candle = the same from the full history");
}

// ---- host: a 3-min ES chart, 10 complete sessions (Mon Oct 5 - Fri Oct 16) + today's partial session (Mon Oct 19)
static const unsigned long DAY = 86400, START = 1791133200UL;   // Sun 2026-10-04 17:00 (Central wall time as UTC in the mock)
static std::vector<mock::Bar> makeBars(int todayBars, unsigned seed) {
    std::mt19937 rng(seed); std::vector<mock::Bar> v; float p = 7700;
    auto addSession = [&](unsigned long open, int n) {
        for (int i = 1; i <= n; ++i) {
            mock::Bar b; b.time = open + (unsigned long)i * 180;
            const float step = (float)((int)(rng() % 9) - 4) * 0.25f; p += step;
            b.open = p; b.low = p - 1.0f; b.high = p + 1.0f; b.close = p + ((rng() % 2) ? 0.5f : -0.5f);
            for (int r = 0; r <= 8; ++r) { mock::VRow x; x.price = b.low + 0.25f * (float)r; x.ask = (long)(rng() % 40 + 1); x.bid = (long)(rng() % 40 + 1); b.rows.push_back(x); }
            const int big = (int)(rng() % 9); const long lots = (long)(std::exp(3.5 + (double)(rng() % 1000) / 1000.0 * 3.0));
            if (rng() % 2) b.rows[(size_t)big].bid += lots; else b.rows[(size_t)big].ask += lots;
            for (const auto& x : b.rows) b.volume += (unsigned long)(x.ask + x.bid);
            v.push_back(b);
        }
    };
    unsigned long open = START;
    for (int d = 0; d < 12; ++d, open += DAY) {
        const int wd = (d + 0) % 7;                                   // d = 0 -> the Monday Oct 5 session (opens Sun 17:00)
        if (wd == 5 || wd == 6) continue;                             // Sat / Sun sessions do not exist
        addSession(open, 460);                                        // 17:00 -> 16:00 (23 h of 3-min bars)
    }
    open = START + 14 * DAY;                                          // Mon Oct 19 session (opens Sun Oct 18 17:00)
    addSession(open, todayBars);
    return v;
}
struct Chart {
    mock::Host H; BigBidAsk* x = nullptr;
    explicit Chart(const std::vector<mock::Bar>& b) { H.bars = b; H.now = b.back().time; mock::current = &H; x = static_cast<BigBidAsk*>(CreateExtension()); x->setup(); }
    ~Chart() { mock::current = &H; x->destroy(); delete x; mock::current = nullptr; }
    BBState* S() { mock::current = &H; return x->testState(); }
    void pump(int n, bool draw) {
        for (int k = 0; k < n; ++k) { mock::current = &H; BBState* s = x->testState();
            if (s) { s->builtMs = -1; s->boardMs = -1; s->sdCheckMs = -1; s->statsMs = -1; s->lastStatusMs = -1; s->logMs = -1; }
            if (draw) x->draw(); else x->calc(0); }
    }
    void addBar(const mock::Bar& b) { H.bars.push_back(b); H.now = b.time; }
};
static std::vector<sb::Ev> board(const Chart& c) { std::vector<sb::Ev> v; sb::readAll('B', c.H.symbol, c.H.seconds, c.H.chart, -1, v); return v; }
static std::string evKey(const std::vector<sb::Ev>& v) { std::string o; for (const auto& e : v) { char b[96]; std::snprintf(b, sizeof b, "%lld,%.2f,%d,%g,%d;", e.bar, e.px, e.side, e.val, e.flag); o += b; } return o; }
static void home(const std::string& name) {
    const std::string root = "/tmp/bba115-run", h = root + "/" + name; std::string c = "rm -rf '" + h + "'"; if (std::system(c.c_str()) != 0) {}
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
}
static long todaySess() { return svl::classify(svl::daysFromCivil(2026, 10, 19), 9 * 3600, 180, 510, 900).sess; }

int main() {
    setenv("TZ", "UTC", 1); tzset();
    logicTests();
    const std::vector<mock::Bar> bars = makeBars(200, 42);
    const long today = todaySess();
    std::vector<long long> pv1, pv0; std::vector<long> used; std::string evFull;
    {   home("full");
        Chart c(bars); c.pump(12, false);
        BBState* S = c.S();
        CHECK(S && S->tiers.count(today) && S->tiers[today].ok, "[host] today's tiers decided once the history is read");
        if (!S || !S->tiers.count(today)) { std::cout << "RESULT " << passed << " passed, " << failed << " failed\n"; return 1; }
        const bba::TierSet& T = S->tiers[today];
        pv1 = T.pv[1]; pv0 = T.pv[0]; used = T.used;
        CHECK(T.nSess == 5 && std::find(T.used.begin(), T.used.end(), today) == T.used.end() && T.used.front() < today, "[host] 5 complete sessions, today's partial never pooled");
        CHECK(S->covSess < today - 7, "[host] every closed bar final for the 7+ days before today (read whatever the view)");
        evFull = evKey(board(c));
        const std::vector<sb::Ev> ev = board(c);
        bool closedOnly = true, conf = false; for (const auto& e : ev) { if (e.bar >= c.S()->forming.t) closedOnly = false; if (e.flag & 128) conf = true; }
        CHECK(!ev.empty() && closedOnly, "[host] B events posted from closed candles");
        CHECK(conf, "[host] B events carry bit 7 (pivot confirmed)");
        CHECK(c.S()->sdWhy.find("decided once") != std::string::npos && c.S()->sdWhy.find("Big Mu") == std::string::npos, "[host] SIZES status = the S-tiers (no log-normal fit)");
        // today's session grows with huge prints: today's tiers (and so every earlier print's tier) must not move
        std::mt19937 rng(5);
        for (int k = 0; k < 120; ++k) { mock::Bar b = c.H.bars.back(); b.time += 180; b.rows[3].bid += 5000 + (long)(rng() % 3000); b.volume += 5000; c.addBar(b); c.pump(1, false); }
        const bba::TierSet& T2 = c.S()->tiers[today];
        CHECK(T2.pv[1] == pv1 && T2.pv[0] == pv0 && T2.used == used, "[host] today's tiers unchanged while today's session grows (no size repaint)");
        // the old B events of the candles still in the board window keep their flags / categories
        const std::vector<sb::Ev> ev2 = board(c); std::map<std::string, int> before; for (const auto& e : ev) before[std::to_string(e.bar) + (e.side < 0 ? "b" : "a")] = e.flag;
        bool stable = true; for (const auto& e : ev2) { auto it = before.find(std::to_string(e.bar) + (e.side < 0 ? "b" : "a")); if (it != before.end() && ((it->second & ~128) != (e.flag & ~128))) stable = false; }
        CHECK(stable, "[host] an earlier candle's B event keeps its category / circle / level bits");
    }
    {   home("narrow");                                                        // drawn with a narrow view at the end
        Chart c(bars); c.H.visStart = (int)bars.size() - 40; c.H.visEnd = (int)bars.size() - 1; c.pump(12, true);
        BBState* S = c.S();
        CHECK(S && S->tiers.count(today) && S->tiers[today].pv[1] == pv1 && S->tiers[today].used == used, "[host] narrow view: the same tiers");
        CHECK(evKey(board(c)) == evFull, "[host] narrow view: the same B events");
    }
    {   home("scrolled");                                                      // drawn while scrolled far back
        Chart c(bars); c.H.visStart = 100; c.H.visEnd = 160; c.pump(12, true);
        BBState* S = c.S();
        CHECK(S && S->tiers.count(today) && S->tiers[today].pv[1] == pv1, "[host] scrolled back: the same tiers for today");
        CHECK(evKey(board(c)) == evFull, "[host] scrolled back: the same B events (not from the view)");
    }
    {   home("restart");
        { Chart c(bars); c.pump(12, false); }                                   // writes the record, then removed
        Chart c2(bars); c2.pump(12, false);
        BBState* S = c2.S();
        CHECK(S && S->tiers.count(today) && S->tiers[today].pv[1] == pv1 && S->tiers[today].used == used, "[host] restart: the same tiers");
    }
    {   home("short");                                                        // only 2 complete sessions on the chart: learning, nothing by size
        std::vector<mock::Bar> b2(bars.end() - (460 * 2 + 200), bars.end());
        Chart c(b2); c.pump(8, false);
        BBState* S = c.S();
        CHECK(S && S->tiers.count(today) && !S->tiers[today].ok, "[host] 2 complete sessions: learning");
        bool onlyCat4 = true; for (const auto& e : board(c)) if (((e.flag >> 1) & 7) != 4) onlyCat4 = false;
        CHECK(onlyCat4, "[host] learning: only category 4 (fixed size) events");
    }
    {   const std::string src = [] { std::ifstream f("../BigBidAsk.cpp", std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }();
        CHECK(std::string(BBA_VERSION) == "1.16.0" && std::string(bba::VERSION) == "1.16.0" && src.find("p->setVersion(\"1.16.0\")") != std::string::npos, "[ver] 1.16.0 everywhere");
    }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
