// test_dp217.cpp -- lsDeltaProfile 2.17.0 / SignalBoard 1.1.0 / DeltaProfileCore 2.17.0 (2026-10-18, audit B finding 1 + Market State)
//   [core] tentStep / boardFlag / dpFacts   [sb] flag 1 final / 2 tentative, read() = final only, readAll, nearestFinal
//   [host] the tentative A posts flag 2, the final one flag 1; TA|S once when first shown, TA|X once when it fails, none duplicated after
//          a restart; per-chart status file + the old shared one; Market State "DP" once per closed bar, decided once, cleared on destroy
// Build (from tests_dp217/): g++ -std=c++17 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation -fsanitize=address,undefined
//        -I. -I../tests_delta -I.. test_dp217.cpp -o /tmp/t_dp217 && /tmp/t_dp217
#include "../DeltaProfile.cpp"
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) ++passed; else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static std::string flex;
static std::string home(const std::string& name) {
    const std::string root = "/tmp/dp217-run", h = root + "/" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    flex = h + "/InvestorRT/rtx/lsFlexLevels/";
    return flex + "DeltaProfile/";
}
static int countPrefix(const std::string& text, const std::string& pre) {
    int n = 0; std::istringstream in(text); std::string l; while (std::getline(in, l)) if (l.compare(0, pre.size(), pre) == 0) ++n; return n;
}
static int dLines(const std::string& text, const char* code) {
    int n = 0; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() == REC_FIELDS && c[12] == "D" && c[2] == code) ++n; }
    return n;
}
static int rings(const mock::Host& h) { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "ring") ++n; return n; }
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
static const int PK = 100;
static mock::Host chart(unsigned long t0) {   // the test_repaint216 market: a falling market into PK (heavy selling at 100, PK closes green above it)
    mock::Host h; h.seconds = 60; h.root = "ES"; h.tick = 0.25f;
    for (int i = 0; i <= PK; ++i) { mock::Bar b; b.time = t0 + static_cast<unsigned long>(i * 60);
        b.rows = { mock::Price(120,1,0,1), mock::Price(121,1,0,1), mock::Price(122,1,0,1), mock::Price(123,1,0,1), mock::Price(124,1,0,1) };
        b.volume = 5; b.low = static_cast<float>(130 - 0.25 * i); b.high = b.low + 25; b.open = b.low + 5; b.close = b.low + 5; h.bars.push_back(b); }
    mock::Bar& p = h.bars[PK]; p.rows.push_back(mock::Price(100, 0, 10000, 10000)); p.volume += 10000;
    p.low = 99; p.high = 104; p.open = 100.5f; p.close = 101;
    return h;
}
static void add(mock::Host& h, double lo, double hi) { mock::Bar b = h.bars.back(); b.time += 60;
    b.rows = { mock::Price(120,1,0,1) }; b.volume = 1; b.low = static_cast<float>(lo); b.high = static_cast<float>(hi); b.open = b.close = static_cast<float>((lo + hi) / 2); h.bars.push_back(b); }
static void redraw(mock::Host& h, DeltaProfile& p) { h.draws.clear(); if (h.data) { state(h).builtAt -= 1001; state(h).statusAt = -1; } mock::current = &h; p.draw(); }
static std::vector<sb::Ev> boardAll() { std::vector<sb::Ev> v; sb::readAll('A', "ESZ26", 60, "test", -1, v); return v; }
static std::vector<sb::Ev> boardFinal() { std::vector<sb::Ev> v; sb::read('A', "ESZ26", 60, "test", -1, v); return v; }
static mst::Facts dpState() { return mst::read("DP", "ESZ26", 60, "test", -1); }

static void coreTests() {
    namespace D = delta_profile;
    check(D::tentStep(false, true, true) == D::TS_RULED_OUT && D::tentStep(false, false, false) == D::TS_RULED_OUT, "[core] no close rule = never A");
    check(D::tentStep(true, true, true) == D::TS_FINAL, "[core] close + pivot + confirmed = final");
    check(D::tentStep(true, true, false) == D::TS_SHOWN, "[core] close + pivot so far, unconfirmed = tentative shown");
    check(D::tentStep(true, false, false) == D::TS_HIDDEN, "[core] close, pivot not holding, unconfirmed = hidden (may come back)");
    check(D::tentStep(true, false, true) == D::TS_FAILED, "[core] confirmed pivot against it = failed");
    check(D::boardFlag(false) == 1 && D::boardFlag(true) == 2, "[core] board flag 1 final / 2 tentative");
    check(D::tickDecimals(0.25) == 2 && D::tickDecimals(0.01) == 2 && D::tickDecimals(0.1) == 1 && D::tickDecimals(0.001) == 3 &&
          D::tickDecimals(0.0005) == 4 && D::tickDecimals(0.00005) == 5 && D::tickDecimals(1) == 0, "[core] decimals for ES/NQ CL GC NG HG 6E");
    std::vector<D::FactNode> top(3); top[0].px = 7741.25; top[0].x = 3.14; top[0].buy = false; top[1].px = 7738.5; top[1].x = 2.5; top[2].px = 7744; top[2].x = 2.0;
    std::vector<D::FactA> onBar(2); onBar[0].px = 7741.25; onBar[0].x = 3.1; onBar[0].bull = true; onBar[0].fin = false;
    onBar[1].px = 7740; onBar[1].x = 2.2; onBar[1].bull = false; onBar[1].fin = true;
    std::map<std::string, std::string> kv = D::dpFacts(top, onBar, {}, {}, 0.25);
    check(kv["node_px"] == "7741.25" && kv["node_x"] == "3.1" && kv["node_side"] == "sell" && kv["top3"] == "7741.25/7738.50/7744.00", "[core] node facts");
    check(kv["a"] == "final" && kv["a_side"] == "bear" && kv["a_px"] == "7740.00" && kv["a_x"] == "2.2", "[core] a final wins over a stronger tentative");
    check(kv["a_conf_n"] == "0" && kv["a_fail_n"] == "0" && kv["a_conf_px"] == "?", "[core] nothing confirmed / failed");
    onBar.resize(1); kv = D::dpFacts({}, onBar, onBar, onBar, 0.00005);
    check(kv["a"] == "tentative" && kv["a_px"] == "7741.25000" && kv["node_px"] == "?" && kv["a_conf_n"] == "1" && kv["a_fail_n"] == "1", "[core] tentative, 6E decimals, counts");
    kv = D::dpFacts({}, {}, {}, {}, 0.25);
    check(kv["a"] == "none" && kv["a_side"] == "?", "[core] no A on the bar");
    bool clean = true; for (const auto& p : D::dpFacts(top, onBar, onBar, onBar, 0.25)) for (char c : p.first + p.second) if (c == ';' || c == '=' || c == '|') clean = false;
    check(clean, "[core] no ; = | in the facts");
    check(D::dpFacts(top, onBar, onBar, onBar, 0.25).size() <= 40, "[core] under 40 keys");
}
static void boardTests() {
    std::vector<sb::Ev> v(3);
    v[0].bar = 100; v[0].px = 10; v[0].side = 1; v[0].val = 5; v[0].flag = 2;    // tentative, strongest
    v[1].bar = 100; v[1].px = 10; v[1].side = 1; v[1].val = 3; v[1].flag = 1;    // final
    v[2].bar = 100; v[2].px = 10; v[2].side = 1; v[2].val = 2; v[2].flag = 0;    // pre-1.1.0 publisher
    check(!sb::finalA(v[0]) && sb::finalA(v[1]) && sb::finalA(v[2]), "[sb] finalA: 1 and legacy 0 final, 2 tentative");
    const sb::Ev* a = sb::nearestEv(v, 10, 1, 0, 200, 1); const sb::Ev* f = sb::nearestFinal(v, 10, 1, 0, 200, 1);
    check(a && a->val == 5 && f && f->val == 3, "[sb] nearestEv any / nearestFinal skips the tentative");
    sb::publish('A', "SBTEST", 180, "c", 1000, v);
    std::vector<sb::Ev> all, fin; sb::readAll('A', "SBTEST", 180, "c", 1000, all); sb::read('A', "SBTEST", 180, "c", 1000, fin);
    check(all.size() == 3 && fin.size() == 2 && fin[0].flag == 1 && fin[1].flag == 0, "[sb] read('A') = final only, readAll = all");
    sb::publish('B', "SBTEST", 180, "c", 1000, v); std::vector<sb::Ev> b; sb::read('B', "SBTEST", 180, "c", 1000, b);
    check(b.size() == 3, "[sb] other kinds unfiltered (B flag bits are not the A flag)");
    check(std::string(sb::VERSION) == "1.1.0", "[sb] version 1.1.0");
}
int main() {
    setenv("TZ", "UTC", 1); tzset();
    coreTests(); boardTests();
    const unsigned long t0 = 1791504000UL + 9 * 3600;
    const long long session = 20735;
    // 1) tentative -> final: flag 2 then 1, one TA|S, no TA|X; Market State a = tentative on PK, then a_conf on the next bar
    {   const std::string dir = home("confirm"), path = dir + "ES-ESZ26-60-signals-" + std::to_string(session) + ".csv";
        mock::Host h = chart(t0); add(h, 99.5, 103.5);
        DeltaProfile p; redraw(h, p);
        check(rings(h) == 1, "[host] tentative A shown (2.16 behaviour kept)");
        std::vector<sb::Ev> all = boardAll();
        check(all.size() == 1 && all[0].flag == 2, "[host] tentative A posted with flag 2");
        check(boardFinal().empty(), "[host] sb::read('A') (the Liquidity Profile's call) does not see the tentative A");
        check(countPrefix(slurp(path), "TA|S|") == 1 && countPrefix(slurp(path), "TA|X|") == 0, "[host] TA|S written when first shown");
        redraw(h, p); redraw(h, p);
        check(countPrefix(slurp(path), "TA|S|") == 1, "[host] TA|S only once across redraws");
        mst::Facts F = dpState();
        check(F.ok && mst::getS(F, "a") == "tentative" && mst::getS(F, "a_side") == "bull" && mst::getS(F, "version") == "2.17.0", "[host] DP fact: a tentative on its bar");
        check(F.ok && F.bar == state(h).mstBar && F.bar > 0 && mst::getS(F, "node_px") != "?" && mst::getS(F, "a_conf_n") == "0", "[host] DP fact: node + bar stamp, no history reported");
        const long long bar1 = F.bar;
        h.bars.back().high = 104.5f; add(h, 100, 105); redraw(h, p);       // the pivot is confirmed
        all = boardAll();
        check(all.size() == 1 && all[0].flag == 1 && boardFinal().size() == 1, "[host] confirmed A posted with flag 1 and readable by sb::read");
        check(dLines(slurp(path), "A") == 1 && countPrefix(slurp(path), "TA|X|") == 0, "[host] D line, no failure line");
        F = dpState();
        check(F.ok && F.bar > bar1 && mst::getS(F, "a") == "none" && mst::getS(F, "a_conf_n") == "1" && mst::getS(F, "a_conf_side") == "bull", "[host] DP fact: next bar reports the confirmation");
        const std::map<std::string, std::string> kv = F.kv;
        redraw(h, p); F = dpState();
        check(F.ok && F.kv == kv, "[host] DP facts of a bar never change on later redraws");
        const std::string st = slurp(flex + "DeltaProfile.status-ES-60.txt"), old = slurp(flex + "DeltaProfile.status.txt");
        check(st.find("VERSION,2.17.0\n") == 0 && st.find("TENTATIVE_A,") != std::string::npos && st.find("MARKET_STATE,DP bar ") != std::string::npos, "[host] per-chart status file");
        check(old == st, "[host] the shared status file still written (compatibility)");
        mock::current = &h; p.destroy();
        check(!dpState().ok, "[host] destroy clears the Market State entry");
        latchBooks().clear(); }
    // 2) tentative -> failed: TA|S then exactly one TA|X; nothing on the board; Market State reports the failure; restart = no duplicates
    {   const std::string dir = home("fail"), path = dir + "ES-ESZ26-60-signals-" + std::to_string(session) + ".csv";
        mock::Host h = chart(t0); add(h, 99.5, 103.5);
        DeltaProfile p; redraw(h, p);
        check(rings(h) == 1 && boardAll().size() == 1 && boardAll()[0].flag == 2, "[host] fail: shown, flag 2");
        h.bars.back().low = 60; h.bars.back().high = 70; h.bars.back().open = h.bars.back().close = 65; add(h, 59, 69); redraw(h, p);   // valid OHLC (test_repaint216 left open / close at 101: a calc error)
        check(state(h).state.find("error") == std::string::npos, "[host] fail: the bars are valid (no calc error)");
        check(rings(h) == 0 && boardAll().empty(), "[host] fail: hidden while the pivot rule does not hold, off the board");
        check(countPrefix(slurp(path), "TA|X|") == 0, "[host] fail: hidden is not yet failed (may come back)");
        add(h, 60, 72); redraw(h, p);                                    // the turn confirms the pivot at 59: the A failed
        const std::string rec = slurp(path);
        check(countPrefix(rec, "TA|S|") == 1 && countPrefix(rec, "TA|X|") == 1 && dLines(rec, "A") == 0, "[host] fail: one TA|S, one TA|X, no D line");
        { std::istringstream in(rec); std::string l; bool ok = false;
          while (std::getline(in, l)) if (l.compare(0, 5, "TA|X|") == 0) { std::vector<std::string> c = fields(l); ok = c.size() == 13 && c[2] == "2.17.0" && c[3] == "ESZ26" && c[8] == "bullish"; }
          check(ok, "[host] TA|X line: 13 fields, version, root, bias"); }
        mst::Facts F = dpState();
        check(F.ok && mst::getS(F, "a_fail_n") == "1" && mst::getS(F, "a_fail_side") == "bull", "[host] DP fact reports the failed tentative A");
        redraw(h, p); add(h, 61, 73); redraw(h, p);
        check(countPrefix(slurp(path), "TA|X|") == 1, "[host] TA|X only once");
        F = dpState(); check(F.ok && mst::getS(F, "a_fail_n") == "0", "[host] the failure is reported on one bar only");
        mock::current = &h; p.destroy(); latchBooks().clear();
        DeltaProfile q; mock::Host h2 = h; h2.data = NULL; redraw(h2, q); redraw(h2, q);   // restart from the record
        const std::string rec2 = slurp(path);
        check(countPrefix(rec2, "TA|S|") == 1 && countPrefix(rec2, "TA|X|") == 1, "[host] restart: TA lines seeded, never written twice");
        check(state(h2).mstDecT >= 0 && dpState().ok && mst::getS(dpState(), "a_fail_n") == "0", "[host] restart: history not re-reported");
        mock::current = &h2; q.destroy(); latchBooks().clear(); }
    // 3) restart while tentative: TA|S seeded, not repeated
    {   const std::string dir = home("restart"), path = dir + "ES-ESZ26-60-signals-" + std::to_string(session) + ".csv";
        mock::Host h = chart(t0); add(h, 99.5, 103.5);
        DeltaProfile p; redraw(h, p); mock::current = &h; p.destroy(); latchBooks().clear();
        DeltaProfile q; mock::Host h2 = h; h2.data = NULL; redraw(h2, q);
        check(rings(h2) == 1 && countPrefix(slurp(path), "TA|S|") == 1, "[host] restart while tentative: shown, TA|S not repeated");
        mock::current = &h2; q.destroy(); latchBooks().clear(); }
    // 4) versions
    {   const std::string src = slurp("../DeltaProfile.cpp");
        check(std::string(DLT_VERSION) == "2.17.0" && src.find("p->setVersion(\"2.17.0\")") != std::string::npos, "[ver] DLT_VERSION = setVersion literal = 2.17.0");
        check(slurp("../DeltaProfileCore.h").find("DeltaProfileCore.h 2.17.0") != std::string::npos, "[ver] core header 2.17.0"); }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
