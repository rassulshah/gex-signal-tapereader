// test_columns280.cpp -- (CR5200) lsDeltaProfile 2.8.0: the middle right-side column (ColumnBoard.h), delta text per row, the margin
// when no Dealer Profile is on the chart, Width 40-250 / Show delta text settings. Mock SDK (tests_delta/irtsdk.h).
// Build (from tests_delta/): g++ -std=c++14 -O1 -g -I. -fsanitize=address,undefined test_columns280.cpp
#include "../DeltaProfile.cpp"
#include <iostream>
#include <sys/stat.h>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& m) { if (ok) ++passed; else { ++failed; std::cout << "FAIL " << m << '\n'; } }
static mock::Host history(const std::string& chart, int count = 92, int seconds = 60) {
    mock::Host h; h.seconds = seconds; h.chart = chart;
    const unsigned long start = 1700000000;
    for (int i = 0; i < count; ++i) {
        mock::Bar b; b.time = start + static_cast<unsigned long>(i * seconds);
        b.rows.push_back(mock::Price(100, 3, 1, 4)); b.rows.push_back(mock::Price(101, 1, 2, 3)); b.rows.push_back(mock::Price(99, 2, 0, 2));
        b.volume = 9; b.low = 99; b.high = 101; b.open = 100; b.close = 100; h.bars.push_back(b);
    }
    return h;
}
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
static void cleanup(DeltaProfile& p, mock::Host& h) { mock::current = &h; p.destroy(); latchBooks().clear(); }
// every text / box this plugin draws right of the price area sits inside its own column (rings and Ex marks are left of the gap)
static int strays(const mock::Host& h, const ChartState& s, std::string* why = nullptr) {
    int n = 0;
    for (const mock::Draw& d : h.draws) {
        int l = d.x, r = d.x;
        if (d.type == "text") { const int w = static_cast<int>(d.text.size()) * 6; if (d.color == C_BUY || d.color == C_SELL || d.color == C_SUPPORT || d.color == C_RESISTANCE || d.color == C_MUTED || d.color == C_INK) { l = d.x - w; r = d.x; } }
        if (d.type == "ring" || d.type == "line") continue;
        if (r <= s.lay.priceRight) continue;
        if (l >= s.lay.delta.l && r <= s.lay.delta.r) continue;
        if (d.type == "text" && d.text.rfind("DLT", 0) == 0 && d.x >= s.lay.delta.l && d.x + static_cast<int>(d.text.size()) * 6 <= s.lay.delta.r) continue;   // the left-aligned title
        ++n; if (why && why->empty()) *why = d.type + " '" + d.text + "' " + std::to_string(l) + "-" + std::to_string(r);
    }
    return n;
}
static std::string dirFor(const std::string& name) {
    const std::string d = "/tmp/dlt280-" + name; std::string cmd = "rm -rf " + d; if (std::system(cmd.c_str()) != 0) {}
    mkdir(d.c_str(), 0700); mkdir((d + "/InvestorRT").c_str(), 0700); mkdir((d + "/InvestorRT/rtx").c_str(), 0700); mkdir((d + "/InvestorRT/rtx/lsFlexLevels").c_str(), 0700);
    setenv("USERPROFILE", d.c_str(), 1); return d + "/InvestorRT/rtx/lsFlexLevels/";
}
int main() {
    setenv("TZ", "UTC", 1); tzset();
    check(std::string(DLT_VERSION) == "2.8.0", "version 2.8.0");
    {   // the setVersion literal gex-build reads = DLT_VERSION
        std::ifstream f("../DeltaProfile.cpp"); std::stringstream ss; ss << f.rdbuf();
        check(ss.str().find("p->setVersion(\"2.8.0\")") != std::string::npos, "setVersion literal 2.8.0");
    }
    // 1) no Dealer Profile on the chart: the column sits at the axis (80 px), every bar and text inside it; delta values green / red
    {
        dirFor("alone"); mock::Host h = history("alone"); mock::current = &h; DeltaProfile p; p.draw();
        const ChartState& s = state(h);
        check(s.lay.ok && s.lay.delta.r == 1978 && s.lay.delta.l == 1898 && s.lay.priceRight == 1878 && s.lay.dealer.w() == 0, "alone: column 1898-1978, price ends at 1878");
        std::string why; check(strays(h, s, &why) == 0, "alone: nothing outside the column " + why);
        int green = 0, red = 0; for (const mock::Draw& d : h.draws) if (d.type == "text" && (d.text[0] == '+' || d.text[0] == '-')) { if (d.color == C_BUY) ++green; if (d.color == C_SELL) ++red; }
        check(green > 0 && red > 0 && s.textsDrawn == green + red, "delta values next to the bars: +green / -red (" + std::to_string(green) + " / " + std::to_string(red) + ")");
        bool title = false; for (const mock::Draw& d : h.draws) if (d.type == "text" && d.text == "DLT v2.8.0") title = true;
        check(title, "title 'DLT v2.8.0'");
        // the margin: owner only after 5 s without a Dealer Profile, then gap + 80 = 100 px = 10 bars at 10 px / bar
        p.calc(0); check(h.marginSets == 0, "not in the first 5 s (the Dealer Profile may still be loading)");
        state(h).firstDrawMs -= 6000; p.calc(0);
        check(h.margin == 10 && h.marginSets == 1 && state(h).marginOwner, "no Dealer Profile: this plugin sets the margin, 10 bars");
        p.calc(0); p.preCalc(); check(h.marginSets == 1, "not set again");
        cleanup(p, h);
        int w = 0; long long ms = 0; check(!cb::peek(cb::DELTA, "ESZ26", 60, "alone", w, ms), "destroy: the column leaves the board");
    }
    // 2) a Dealer Profile on the chart (board: Dealer 80, Levels 78): the middle column, no margin from here
    {
        dirFor("dealer"); mock::Host h = history("with dealer"); mock::current = &h; DeltaProfile p;
        cb::publish(cb::DEALER, "ESZ26", 60, "with dealer", 1, 80); cb::publish(cb::LEVELS, "ESZ26", 60, "with dealer", 1, 78);
        p.draw(); cb::publish(cb::DEALER, "ESZ26", 60, "with dealer", 2, 80); cb::publish(cb::LEVELS, "ESZ26", 60, "with dealer", 2, 78);
        h.draws.clear(); p.draw();
        const ChartState& s = state(h);
        check(s.lay.dealer.l == 1898 && s.lay.delta.l == 1818 && s.lay.delta.r == 1898 && s.lay.levels.l == 1740 && s.lay.priceRight == 1720, "with a Dealer Profile: Levels 1740-1818 | Delta 1818-1898 | Dealer 1898-1978");
        std::string why; check(strays(h, s, &why) == 0, "nothing in the Levels / Dealer columns " + why);
        state(h).firstDrawMs -= 60000; p.calc(0);
        check(h.marginSets == 0 && !state(h).marginOwner, "the Dealer Profile owns the margin: never set here");
        int w = 0; long long ms = 0; check(cb::peek(cb::DELTA, "ESZ26", 60, "with dealer", w, ms) && w == 80, "the column's width on the board for the Dealer Profile");
        cleanup(p, h); cb::clear(cb::DEALER, "ESZ26", 60, "with dealer"); cb::clear(cb::LEVELS, "ESZ26", 60, "with dealer");
    }
    // 3) widths 40 / 250 / mixed from the settings file, a narrow chart, the text box off, an old 2.7 row migrated
    {
        const std::string flex = dirFor("widths");
        int bad = 0; std::string why;
        for (int w : { 40, 250, 130 }) {
            { std::ofstream f(flex + "DeltaProfile.settings.txt"); f << "ES|0|" << w << "|0|9|1|0|0|0|0|1|0|90|3|1|1|c28\n"; }
            mock::Host h = history("w" + std::to_string(w)); mock::current = &h; DeltaProfile p; p.draw();
            const ChartState& s = state(h);
            if (s.layout.width != w || s.lay.delta.w() != w || strays(h, s, &why)) ++bad;
            cleanup(p, h);
        }
        check(bad == 0, "widths 40 / 250 / 130: the column follows, nothing outside it " + why);
        { std::ofstream f(flex + "DeltaProfile.settings.txt"); f << "ES|0|250|0|9|1|0|0|0|0|1|0|90|3|1|1|c28\n"; }
        mock::Host n = history("narrow"); n.paneRight = 420; n.scaleLeft = 400; mock::current = &n; DeltaProfile q; q.draw();
        check(state(n).lay.squeezed && state(n).lay.delta.w() < 250 && strays(n, state(n)) == 0 && state(n).lay.priceRight >= 150, "a 400 px chart: the column shrinks, price keeps its room");
        cleanup(q, n);
        { std::ofstream f(flex + "DeltaProfile.settings.txt"); f << "ES|0|80|0|9|1|0|0|0|0|1|0|90|3|1|0|c28\n"; }
        mock::Host t = history("text off"); mock::current = &t; DeltaProfile r; r.draw();
        int vals = 0; for (const mock::Draw& d : t.draws) if (d.type == "text" && (d.text[0] == '+' || d.text[0] == '-')) ++vals;
        check(!state(t).layout.text && vals == 0, "'Show delta text' off: no values");
        cleanup(r, t);
        { std::ofstream f(flex + "DeltaProfile.settings.txt"); f << "ES|0|50|7|12|1|0|0|0|0|1|0|90|3|1\n"; }
        mock::Host o = history("old row"); mock::current = &o; DeltaProfile u; u.draw();
        check(state(o).layout.width == 80 && state(o).layout.gap == 0 && state(o).layout.font == 12 && state(o).layout.text, "an old 2.7 row (width 50 = the old default): 80, the gap ignored, text on");
        // Apply writes the 2.8.0 row (text flag + marker); the dialog's garbage width keeps the last good one
        o.period = ""; o.parms[0] = 7777; o.parms[1] = 10; o.checks[2] = false; u.parmsApply(); o.period = "1 min";
        std::ifstream in(flex + "DeltaProfile.settings.txt"); std::string ln, row; while (std::getline(in, ln)) if (ln.rfind("ES|", 0) == 0) row = ln;
        check(row == "ES|0|80|0|10|1|0|0|0|0|1|0|90|3|1|0|c28", "Apply: '" + row + "' (garbage width refused, text off saved)");
        cleanup(u, o);
    }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
