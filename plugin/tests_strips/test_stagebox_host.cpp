// test_stagebox_host.cpp -- lsReversalStage 2.1.0 on the mock host: the letters 1 2 3 C on the pivot bar and the STAGE INFO BOX for the
// latest pivot (StageBox.h), the hover in his format, the params file (trapped threshold), the settings rows, the status line.
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -DRS_TEST_CLOCK -I.. -I../tests_aggbook/mock test_stagebox_host.cpp
#include "ReversalStage.cpp"
#include <cstdio>
#include <unistd.h>
long long rs_test_now_ms = 1000000; void* rs_test_cursor_wnd = (void*)0x1; int rs_test_cx = 0, rs_test_cy = 0; bool rs_test_cursor_ok = true;
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void writeAll(const std::string& p, const std::string& t) { std::ofstream f(p.c_str(), std::ios::binary | std::ios::trunc); f << t; }
typedef ReversalStage Ind;
static const unsigned long T0 = 1791900000UL;
static unsigned long keyOf(int k) { return T0 + 60UL * (unsigned long)k; }
static mock::Bar bar(int k, float o, float h, float l, float c) { mock::Bar b; b.time = keyOf(k); b.open = o; b.high = h; b.low = l; b.close = c; b.volume = 1000; b.rows.push_back(mock::Price(c, 500, 500, 1000)); return b; }
static const mock::Draw* text(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text == t) return &d; return nullptr; }
static const mock::Draw* textStarts(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text.rfind(t, 0) == 0) return &d; return nullptr; }
static void settings(const std::string& flex, int key, double v) { std::ofstream f((flex + "\\ReversalStage.settings-ES.txt").c_str()); ss::Values V = ss::defaults(rsset::spec(), 0); if (key >= 0) V.v[(size_t)key] = v; f << ss::format(rsset::spec(), V); }
int main() {
    char tmpl[] = "/tmp/rsbXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 60; h.symbol = "ESZ6"; h.root = "ES"; h.period = "1 min"; h.paneTop = 0; h.paneBottom = 160; h.paneRight = 2600; h.scaleLeft = 2580; h.ppb = 70; h.barW = 70;
    for (int k = 0; k < 22; ++k) h.bars.push_back(bar(k, 7005, 7010, 7002.5f, 7006));
    {   mock::Bar p = bar(22, 7000, 7001.25f, 6990, 6995); p.rows.clear(); p.volume = 2000;   // the pivot low: sellers below the close 6995
        for (float px = 6990; px <= 7001.25f; px += 0.25f) p.rows.push_back(mock::Price(px, 10, px < 6995 ? 25 : 5, px < 6995 ? 35 : 15));
        h.bars.push_back(p); }
    h.bars.push_back(bar(23, 6995.5f, 7000.5f, 6994, 7000));   // green close = the HELD trigger
    h.bars.push_back(bar(24, 7000, 7004, 6999, 7003));         // higher high = follow-through
    h.bars.push_back(bar(25, 7003, 7004, 7001, 7002));
    h.bars.push_back(bar(26, 7002, 7003, 7001, 7002));         // forming
    h.now = keyOf(26) + 5;
    Ind* p = static_cast<Ind*>(CreateExtension()); p->setup(); p->calc(0);
    RSState* S = p->stateFor();
    CHECK(S && std::string(RS_VERSION) == "2.1.0" && std::string(rsl::VERSION) == "2.1.0" && std::string(rsset::PLUGIN_VERSION) == "2.1.0" && std::string(sbx::VERSION) == "1.0.0", "versions 2.1.0 / StageBox 1.0.0");
    // the attempt as the engine records it + its letters (injected: the engine itself is covered by its own tests)
    for (int k = 22; k <= 23; ++k) { rsl::Step s; s.key = (long long)keyOf(k); s.side = rsl::LOW; s.start = (long long)keyOf(22); s.ext = 27960; s.stage = 2; s.status = rsl::FORMING; s.story = "bids 7,752 > offers 6,316~2,169 lots taken, Q 117%~~"; S->steps[s.key] = s; }
    { rsl::Ev e; e.key = (long long)keyOf(22); e.pane = lst::ATK; e.l.code = "E"; e.m.pace100 = 40; S->evs.insert(std::make_pair(e.key, e)); }
    { rsl::Ev e; e.key = (long long)keyOf(23); e.pane = lst::DEF; e.l.code = "H"; e.m.ratio = 1.5; S->evs.insert(std::make_pair(e.key, e)); }
    rs_test_now_ms += 2000; h.bars.push_back(bar(27, 7002, 7003, 7001, 7002)); h.now = keyOf(27) + 5; p->calc(0);
    CHECK(S->boxes.size() == 1 && S->boxes[0].letters == "1 2 3 C" && S->boxes[0].pivKey == (long long)keyOf(22), "one pivot, letters 1 2 3 C on the pivot bar");
    // trapped = sells below 6995 (20 rows x 25 = 500) / avg bar volume 1000 = 0.50x
    CHECK(std::fabs(S->boxes[0].trapped - 0.50) < 1e-9, "trapped 0.50x from the VAP rows");
    h.draws.clear(); p->draw();
    const mock::Draw* L = text(h, "1 2 3 C");
    CHECK(L && L->color == (COLOR)0x22C55E, "the letters 1 2 3 C drawn in the low-pivot green");
    CHECK(textStarts(h, "YOUR 3 REVERSAL STAGES") && text(h, "Trap") && text(h, "Collision") && text(h, "Turn") && text(h, "Confirm"), "the info box: title + stage names");
    CHECK(text(h, "bids 7,752 > offers 6,316") && textStarts(h, "pace 0.4x (0.5x)") && textStarts(h, "trapped 0.50x (0.30x)") && textStarts(h, "Hb 1.5x (1.25x)"), "the info box values with thresholds");
    { int checks = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == (COLOR)0x22C55E && d.penW == 2) ++checks; CHECK(checks >= 2 * 5, "a drawn check mark per seen row (5 rows x 2 strokes)"); }
    CHECK(text(h, "1 Trap") && text(h, "C Confirm"), "the legend right of the last candle");
    {   const std::string st = readAll(S->recPath + ".status.txt");
        CHECK(st.find("STAGE_BOX,1.0.0,1 pivots,latest 1 2 3 C,params defaults (no file) trapped 0.3x") != std::string::npos, "status STAGE_BOX line"); }
    // the hover on the letters: his format
    {   RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)(L ? L->x + 3 : 0); e.v.mouse.v = (short)(L ? L->y + 3 : 0);
        p->mouse(&e); CHECK(S->hoverKey == (long long)keyOf(22), "the mouse on the letters picks the pivot");
        h.draws.clear(); p->draw();
        if (std::getenv("SB_DBG")) std::printf("hk %lld haveWnd %d hits %zu pane %d-%d\n", S->hoverKey, (int)S->haveWnd, S->hits.size(), S->paneT, S->paneB);
        if (std::getenv("SB_DBG")) for (auto& d : h.draws) if (d.type == "text") std::printf("T %s\n", d.text.c_str());
        CHECK(textStarts(h, "   Question:") && textStarts(h, "   Answer:") && textStarts(h, "   How to use:") && textStarts(h, "   Example:"), "hover: question / answer / how to use / example"); S->hoverKey = 0; }
    // the params file changes the trapped threshold (read every 60 s)
    writeAll(flex + "\\LRA-StageBox-ES.csv", "PARAM,TRAP_THR,0.55\nPARAM,HB_MIN,1.6\n");
    rs_test_now_ms += 61000; p->calc(0); h.draws.clear(); p->draw();
    CHECK(textStarts(h, "trapped 0.50x (0.55x)") && S->boxes[0].letters == "1 2 3" && S->par.why == "file (2 values)", "params file: trapped 0.55x shown (no check change), Hb 1.6x -> C not seen");
    std::remove((flex + "\\LRA-StageBox-ES.csv").c_str()); rs_test_now_ms += 61000; p->calc(0);
    // F? on the pivot bar: an amber row + amber F? under the letters
    { rsl::Ev e; e.key = (long long)keyOf(22); e.pane = lst::DEF; e.l.code = "F?"; e.l.nums = "front 1.6x behind 0.6x"; S->evs.insert(std::make_pair(e.key, e)); }
    rs_test_now_ms += 2000; h.bars.push_back(bar(28, 7002, 7003, 7001, 7002)); h.now = keyOf(28) + 5; p->calc(0); h.draws.clear(); p->draw();
    { const mock::Draw* w = text(h, "Warning"); const mock::Draw* f = text(h, "F?"); CHECK(w && f && f->color == (COLOR)0xF59E0B && text(h, "front 1.6x behind 0.6x"), "F? = the amber warning row + amber F? on the pane"); }
    // settings really switch the drawing
    settings(flex, rsset::K_SHOW_NOW, 0); rs_test_now_ms += 2000; h.bars.push_back(bar(29, 7002, 7003, 7001, 7002)); h.now = keyOf(29) + 5; p->calc(0); h.draws.clear(); p->draw();
    CHECK(!textStarts(h, "YOUR 3 REVERSAL STAGES") && text(h, "1 2 3 C"), "Stage info box off: no box, letters stay");
    settings(flex, rsset::K_SHOW_STEPS, 0); rs_test_now_ms += 2000; h.bars.push_back(bar(30, 7002, 7003, 7001, 7002)); h.now = keyOf(30) + 5; p->calc(0); h.draws.clear(); p->draw();
    CHECK(!text(h, "1 2 3 C") && textStarts(h, "YOUR 3 REVERSAL STAGES"), "Stage letters off: no letters, the box stays");
    settings(flex, rsset::K_COL_LOW, (double)0x00AA00); rs_test_now_ms += 2000; h.bars.push_back(bar(31, 7002, 7003, 7001, 7002)); h.now = keyOf(31) + 5; p->calc(0); h.draws.clear(); p->draw();
    { const mock::Draw* t = text(h, "1"); CHECK(t && t->color == (COLOR)0x00AA00, "Low pivot colour applies to the box row codes"); }
    // no repaint: the decided box is identical with later bars
    CHECK(S->boxes.size() == 1 && S->boxes[0].letters == "1 2 3 C" && S->boxes[0].decided, "the decided box never changes with later bars");
    { const ss::Spec& SP = rsset::spec(); CHECK(SP.paramVersion == 2 && std::string(SP.rows[rsset::K_COL_WARN].key) == "col_warn" && std::string(SP.rows[rsset::K_SHOW_NOW].label) == "Stage info box", "settings rows: Stage info box, F? warning colour, parameter version 2"); }
    p->destroy();
    std::printf("lsReversalStage 2.1.0 stage box host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
