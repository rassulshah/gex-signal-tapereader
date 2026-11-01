// tests_strips/test_stage_host.cpp -- lsReversalStage 1.1.0 end to end on the mock host (2026-10-22): the letters read from the Strips folder
// records, each closed candle decided once when settled and appended to RS-<MKT>-<spb>.csv, drawn only from the record; a restart redraws
// the same; bar-by-bar with a restart after every bar = one straight pass; a late letter never changes a decided candle; the forming
// candle is never decided; settings (show steps / colours / names / Now box / Position) really change the drawing; the hover; two charts.
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -DRS_TEST_CLOCK -I.. -I../tests_aggbook/mock test_stage_host.cpp
#include "ReversalStage.cpp"
#include <cstdio>
#include <unistd.h>
long long rs_test_now_ms = 1000000; void* rs_test_cursor_wnd = nullptr; int rs_test_cx = 0, rs_test_cy = 0; bool rs_test_cursor_ok = false;
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void writeAll(const std::string& p, const std::string& t) { std::ofstream f(p.c_str(), std::ios::binary | std::ios::trunc); f << t; }
static void appendAll(const std::string& p, const std::string& t) { std::ofstream f(p.c_str(), std::ios::binary | std::ios::app); f << t; }
typedef ReversalStage Ind;
static void dispose(Ind* p) { p->destroy(); p->~Ind(); ::operator delete(p); }
static Ind* make() { Ind* p = static_cast<Ind*>(CreateExtension()); static bool once = false; if (!once) { once = true; p->setup(); } return p; }
static const unsigned long T0 = 1791900000UL;
static unsigned long keyOf(int k) { return T0 + 60UL * (unsigned long)k; }
static mock::Bar bar(int k, float hi, float lo, float c) { mock::Bar b; b.time = keyOf(k); b.open = c; b.high = hi; b.low = lo; b.close = c; b.volume = 10; return b; }
// 10 flat candles, a 5-candle low at 10 (K + A), 11 Tr, 12 H, then 13 + 14 drift, 15 = expansion; then a HIGH attempt at 20 that fails on X
static std::vector<mock::Bar> day() {
    std::vector<mock::Bar> v;
    for (int k = 0; k < 10; ++k) v.push_back(bar(k, 7002.0f, 6998.0f, 7000.0f));
    v.push_back(bar(10, 6999.5f, 6995.0f, 6996.0f)); v.push_back(bar(11, 6998.0f, 6995.5f, 6997.0f)); v.push_back(bar(12, 6998.0f, 6995.5f, 6997.5f));
    v.push_back(bar(13, 6998.5f, 6996.0f, 6997.5f)); v.push_back(bar(14, 6998.5f, 6996.0f, 6998.0f)); v.push_back(bar(15, 7001.0f, 6997.5f, 7000.5f));
    for (int k = 16; k < 20; ++k) v.push_back(bar(k, 7002.0f, 6999.0f, 7000.5f));
    v.push_back(bar(20, 7004.0f, 7000.5f, 7003.0f)); v.push_back(bar(21, 7003.5f, 7002.0f, 7002.75f)); v.push_back(bar(22, 7003.5f, 7002.0f, 7003.0f));
    for (int k = 23; k < 30; ++k) v.push_back(bar(k, 7004.0f, 7001.0f, 7002.5f));
    return v;
}
static std::string letters(int upTo) {
    std::string a, d;
    auto A = [&](int k, const char* c, int piv, int book, const char* n) { if (k <= upTo) a += lst::line(lst::ATK, (long long)keyOf(k), lst::make(lst::ATK, c, piv, book, n)) + "\n"; };
    auto D = [&](int k, const char* c, int piv, int book, const char* n) { if (k <= upTo) d += lst::line(lst::DEF, (long long)keyOf(k), lst::make(lst::DEF, c, piv, book, n)) + "\n"; };
    A(10, "K", lst::LOW, -1, "450 lots"); D(10, "A", lst::LOW, -1, "1,940 lots 2.6x"); A(11, "Tr", lst::LOW, -1, "300 lots 2.1x"); D(12, "H", lst::LOW, -1, "800 lots 1.4x");
    A(20, "K", lst::HIGH, -1, "520 lots"); D(20, "A", lst::HIGH, -1, "900 lots 2.2x"); D(21, "H", lst::HIGH, -1, "610 lots 1.2x"); D(22, "X", lst::HIGH, -1, "40 lots 0.2x");
    return a + "\x01" + d;
}
int main() {
    char tmpl[] = "/tmp/rsXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    const std::string pa = flex + "\\Strips\\ATK-ES-60.csv", pd = flex + "\\Strips\\DEF-ES-60.csv", rec = flex + "\\ReversalStage\\RS-ES-60.csv";
    { const std::string L = letters(99); const size_t s = L.find('\x01'); writeAll(pa, L.substr(0, s)); writeAll(pd, L.substr(s + 1)); }
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 60; h.symbol = "ESZ6"; h.root = "ES"; h.period = "1 min"; h.paneTop = 0; h.paneBottom = 130; h.paneRight = 900; h.scaleLeft = 880; h.ppb = 11; h.barW = 11;
    h.bars = day(); h.now = keyOf(29) + 5;                                 // 5 s after candle 28 closed... the forming bar is 29
    Ind* p = make(); p->calc(0); p->draw();
    RSState* S = p->stateFor();
    CHECK(S && std::string(rsl::VERSION) == "1.2.0" && std::string(RS_VERSION) == "1.2.0" && std::string(rsset::PLUGIN_VERSION) == "1.2.0" && h.flags == (unsigned long)(POST_DRAWING | NO_UI | TRACK_MOUSE), "lsReversalStage 1.0.0, its own pane, mouse tracked");
    const std::string r1 = readAll(rec);
    std::vector<std::string> lines; { std::stringstream ss(r1); std::string ln; while (std::getline(ss, ln)) lines.push_back(ln); }
    CHECK(lines.size() == 29, ("29 closed candles decided (the forming one never): " + std::to_string(lines.size())).c_str());
    rsl::Step s10, s12, s15, s22; bool got = true;
    for (const std::string& ln : lines) { rsl::Step x; got = got && rsl::parse(ln, x); if (x.key == (long long)keyOf(10)) s10 = x; if (x.key == (long long)keyOf(12)) s12 = x; if (x.key == (long long)keyOf(15)) s15 = x; if (x.key == (long long)keyOf(22)) s22 = x; }
    CHECK(got, "every record line parses");
    CHECK(s10.side == rsl::LOW && s10.stage == 2 && s10.chain == "K A", ("candle 10: LOW stage 2, chain K A: " + rsl::line(s10)).c_str());
    CHECK(s12.stage == 4 && s12.status == rsl::FORMING, ("candle 12: stage 4 (H): " + rsl::line(s12)).c_str());
    CHECK(s15.stage == 6 && s15.status == rsl::COMPLETE, ("candle 15: expansion 1 ATR: " + rsl::line(s15)).c_str());
    CHECK(s22.status == rsl::FAILED && s22.failedStage == 4 && s22.side == rsl::HIGH, ("candle 22: the HIGH attempt failed on X: " + rsl::line(s22)).c_str());
    // the drawing: a green step at candle 10 two units tall, red at 20, the stub at 22, the names right of the last candle, the Now box
    auto boxesAt = [&](int k) { std::vector<mock::Draw> v; const int xc = 50 + k * 11; for (const mock::Draw& d : h.draws) if (d.type == "box" && d.x <= xc && d.r >= xc && d.b == 128) v.push_back(d); return v; };
    { std::vector<mock::Draw> b = boxesAt(10); CHECK(b.size() == 1 && b[0].color == 0x22C55E && b[0].y == rsl::yOf(2, 2, 128), "candle 10: a green step 2 units tall");
      b = boxesAt(20); CHECK(b.size() == 1 && b[0].color == 0xEF4444 && b[0].y == rsl::yOf(2, 2, 128), "candle 20: a red step 2 units tall (K + A)");
      b = boxesAt(22); CHECK(b.size() == 1 && b[0].y > rsl::yOf(1, 2, 128), "candle 22: the step dropped back to a stub (failed)");
      b = boxesAt(29); CHECK(b.empty(), "the forming candle has no step"); }
    bool names = false, nowBox = false, odds = false, attempt = false;
    for (const mock::Draw& d : h.draws) if (d.type == "text") { names = names || d.text == "6 Expansion"; nowBox = nowBox || d.text.compare(0, 4, "Now ") == 0; odds = odds || d.text == "Odds ?  (T-20261022-39)"; attempt = attempt || d.text == "No attempt"; }
    if (!(names && nowBox && odds && attempt)) for (const mock::Draw& d : h.draws) if (d.type == "text") std::printf("  text %s at %d,%d\n", d.text.c_str(), d.x, d.y);
    CHECK(names && nowBox && odds && attempt, "stage names, the Now box (time, No attempt after the failed HIGH, Odds ?)");
    // no repaint across a restart: a new instance reads the record, writes nothing, draws the same steps
    std::vector<mock::Draw> first = h.draws;
    dispose(p); h.data = nullptr; h.draws.clear();
    p = make(); p->calc(0); p->draw();
    CHECK(readAll(rec) == r1, "restart: the record is unchanged (nothing re-decided)");
    { size_t nb1 = 0, nb2 = 0; for (const mock::Draw& d : first) nb1 += d.type == "box"; for (const mock::Draw& d : h.draws) nb2 += d.type == "box"; CHECK(nb1 == nb2, "restart: the same steps drawn"); }
    // a late letter for a decided candle never changes it
    appendAll(pd, lst::line(lst::DEF, (long long)keyOf(16), lst::make(lst::DEF, "L", lst::LOW, -1, "late")) + "\n");
    rs_test_now_ms += 5000; h.draws.clear(); p->calc(0); p->draw();
    CHECK(readAll(rec) == r1, "a late letter (candle 16) does not change the record");
    dispose(p); h.data = nullptr;
    // bar by bar with a restart after every bar (letters arriving with their candle) = one straight pass
    {
        std::remove(rec.c_str());
        const std::vector<mock::Bar> all = day();
        h.bars.assign(all.begin(), all.begin() + 7);
        for (size_t n = 7; n <= all.size(); ++n) {
            h.bars.assign(all.begin(), all.begin() + (long)n); h.now = all[n - 1].time - 30;   // the forming candle's middle
            const std::string L = letters((int)n - 2); const size_t s = L.find('\x01'); writeAll(pa, L.substr(0, s)); writeAll(pd, L.substr(s + 1));
            h.draws.clear(); p = make(); rs_test_now_ms += 2000; p->calc(0); p->draw(); dispose(p); h.data = nullptr;
        }
        const std::string r2 = readAll(rec);
        std::vector<std::string> a, b; { std::stringstream x(r1), y(r2); std::string ln; while (std::getline(x, ln)) a.push_back(ln); while (std::getline(y, ln)) b.push_back(ln); }
        CHECK(a.size() == b.size(), ("bar by bar: the same number of decided candles " + std::to_string(a.size()) + " / " + std::to_string(b.size())).c_str());
        CHECK(r2 == r1, "bar by bar with a restart after every bar = one straight pass (byte-identical record)");
        { const std::string L = letters(99); const size_t s = L.find('\x01'); writeAll(pa, L.substr(0, s)); writeAll(pd, L.substr(s + 1)); }
    }
    // settled: the last closed candle waits 20 s after its close unless the next candle has closed
    {
        std::remove(rec.c_str()); h.bars = day(); h.bars.resize(25); h.now = keyOf(24) - 50;   // candle 23 closed 10 s ago (its END = keyOf(23))
        h.now = keyOf(23) + 10; p = make(); rs_test_now_ms += 2000; p->calc(0);
        std::string r = readAll(rec); CHECK(r.find("RS|1|" + std::to_string(keyOf(23)) + "|") == std::string::npos && r.find("RS|1|" + std::to_string(keyOf(22)) + "|") != std::string::npos, "the last closed candle waits until it is settled (20 s)");
        h.now = keyOf(23) + 25; rs_test_now_ms += 2000; p->calc(0);
        r = readAll(rec); CHECK(r.find("RS|1|" + std::to_string(keyOf(23)) + "|") != std::string::npos, "decided once settled");
        dispose(p); h.data = nullptr;
    }
    // settings really work: steps off, colours, names off, Now box off / Position 8
    {
        h.bars = day(); h.now = keyOf(29) + 5;
        ss::Values V = ss::defaults(rsset::spec(), ss::marketIndex("ES"));
        V.v[rsset::K_COL_LOW] = 0x00FF00; writeAll(flex + "\\ReversalStage.settings-ES.txt", ss::format(rsset::spec(), V));
        p = make(); rs_test_now_ms += 10000; h.draws.clear(); p->calc(0); p->draw();
        bool green = false; for (const mock::Draw& d : h.draws) green = green || (d.type == "box" && d.color == 0x00FF00); CHECK(green, "Low pivot colour applied");
        V.v[rsset::K_SHOW_STEPS] = 0; V.v[rsset::K_SHOW_NAMES] = 0; V.v[rsset::K_POSITION] = 8; writeAll(flex + "\\ReversalStage.settings-ES.txt", ss::format(rsset::spec(), V));
        rs_test_now_ms += 10000; h.draws.clear(); p->calc(0); p->draw();
        int steps = 0; bool nm = false; int bx = -1, by = -1;
        for (const mock::Draw& d : h.draws) { if (d.type == "box" && d.b == 128 && d.width == 0) ++steps; if (d.type == "text" && d.text == "6 Expansion") nm = true; if (d.type == "box" && d.width == 1) { bx = d.x; by = d.y; } }
        CHECK(steps == 0 && !nm, "Steps and Stage names off: nothing drawn");
        CHECK(bx > 150 && bx + 168 <= 50 + 29 * 11 + 7 && by > 40, "Now box at Position 8 (bottom right of the space up to the last candle)");
        V.v[rsset::K_SHOW_NOW] = 0; writeAll(flex + "\\ReversalStage.settings-ES.txt", ss::format(rsset::spec(), V));
        rs_test_now_ms += 10000; h.draws.clear(); p->calc(0); p->draw();
        bool box = false; for (const mock::Draw& d : h.draws) box = box || (d.type == "box" && d.width == 1); CHECK(!box, "Now box off");
        writeAll(flex + "\\ReversalStage.settings-ES.txt", "garbage");
        rs_test_now_ms += 10000; h.draws.clear(); p->calc(0); p->draw();
        green = false; for (const mock::Draw& d : h.draws) green = green || (d.type == "box" && d.color == 0x22C55E); CHECK(green, "garbled settings file = the defaults");
        // the hover: the mouse over candle 12's step
        rs_test_cursor_wnd = (void*)&h; rs_test_cursor_ok = true; rs_test_cx = 50 + 12 * 11; rs_test_cy = 100;
        RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)(50 + 12 * 11); e.v.mouse.v = 100; e.v.mouse.bar = 12;
        p->mouse(&e); h.draws.clear(); p->draw();
        bool tipOk = false, q = false; for (const mock::Draw& d : h.draws) if (d.type == "text") { tipOk = tipOk || d.text.find("Reversal stage") == 0; q = q || d.text.find("Question: which stage") == 0; }
        CHECK(tipOk && q, "hover on a step: the stage, the question, the letters");
        rs_test_cursor_ok = false; h.draws.clear(); p->draw();
        tipOk = false; for (const mock::Draw& d : h.draws) if (d.type == "text") tipOk = tipOk || d.text.find("Reversal stage") == 0; CHECK(!tipOk, "the hover goes when the cursor leaves");
        dispose(p); h.data = nullptr;
    }
    // (1.1.0) news / settle window "?" over a step decided in it; a failed feed check = recorded only (NewsWindow.h)
    {
        std::remove((flex + "\\ReversalStage.settings-ES.txt").c_str()); h.bars = day(); h.now = keyOf(29) + 5;
        p = make(); rs_test_now_ms += 10000; h.draws.clear(); p->calc(0); p->draw();
        auto stepsOf = [&]() { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "box" && d.width == 0 && d.b == 128) ++n; return n; };
        auto qOf = [&]() { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "text" && d.text == "?") ++n; return n; };
        const int base = stepsOf(); CHECK(base > 0 && qOf() == 0, "steps drawn, no '?' outside a window");
        writeAll(flex + "\\LRA-FeedCheck-ES.csv", "VERSION,1\nMARKET,ES\nPARAM,match_ok,0\n"); usleep(3200000);
        rs_test_now_ms += 10000; h.draws.clear(); p->calc(0); p->draw();
        CHECK(stepsOf() == 0 && readAll(rec).find("RS|1|") != std::string::npos, "feed check failed: no steps drawn, the record kept");
        std::remove((flex + "\\LRA-FeedCheck-ES.csv").c_str());
        { const time_t tt = (time_t)keyOf(12); struct tm tm0; gmtime_r(&tt, &tm0); char ev[96];
          std::snprintf(ev, sizeof ev, "EVENT,%04d-%02d-%02d,%02d:%02d,TEST,ALL\n", tm0.tm_year + 1900, tm0.tm_mon + 1, tm0.tm_mday, tm0.tm_hour, tm0.tm_min);
          writeAll(flex + "\\LRA-News-ALL.csv", std::string("VERSION,1\nMARKET,ALL\n") + ev); }
        usleep(3200000); rs_test_now_ms += 10000; h.draws.clear(); p->calc(0); p->draw();
        CHECK(stepsOf() == base && qOf() >= 1, "a step inside a news window: still drawn, with '?' over it");
        std::remove((flex + "\\LRA-News-ALL.csv").c_str());
        dispose(p); h.data = nullptr;
    }
    // two charts: NQ on another host has its own state and record
    {
        mock::Host h2 = h; h2.data = nullptr; h2.root = "NQ"; h2.symbol = "NQZ6"; h2.draws.clear(); mock::current = &h2;
        Ind* q = make(); rs_test_now_ms += 10000; q->calc(0);
        CHECK(q->stateFor() && q->stateFor()->market == "NQ" && q->stateFor()->evs.empty(), "an NQ chart reads only NQ letters (none)");
        dispose(q); mock::current = &h;
    }
    std::printf("%s: %d passed, %d failed\n", fails ? "FAILED" : "lsReversalStage host", passes, fails);
    return fails ? 1 : 0;
}
