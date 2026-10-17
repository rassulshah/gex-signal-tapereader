// test_repaint216.cpp -- lsDeltaProfile 2.16.0: the pivot against the peak bar's OWN swing (may repaint until confirmed)
// Build (from tests_delta/): g++ -std=c++17 -O1 -I. -I.. -fsanitize=address,undefined test_repaint216.cpp -o t && ./t
#include "../DeltaProfile.cpp"
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) ++passed; else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static std::string home(const std::string& name) {
    const std::string root = "/tmp/dp216-run", h = root + "/" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    return h + "/InvestorRT/rtx/lsFlexLevels/DeltaProfile/";
}
static int dLines(const std::string& text, const char* code) {
    int n = 0; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() == REC_FIELDS && c[12] == "D" && c[2] == code) ++n; }
    return n;
}
static int rings(const mock::Host& h) { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "ring") ++n; return n; }
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
// a falling market into bar PK (a heavy SELLING print at 100, the bar closes GREEN above it = the close rule), then `after` bars
static const int PK = 100;
static mock::Host chart(unsigned long t0) {
    mock::Host h; h.seconds = 60; h.root = "ES"; h.tick = 0.25f;
    for (int i = 0; i <= PK; ++i) { mock::Bar b; b.time = t0 + static_cast<unsigned long>(i * 60);
        b.rows = { mock::Price(120,1,0,1), mock::Price(121,1,0,1), mock::Price(122,1,0,1), mock::Price(123,1,0,1), mock::Price(124,1,0,1) };
        b.volume = 5; b.low = static_cast<float>(130 - 0.25 * i); b.high = b.low + 25; b.open = b.low + 5; b.close = b.low + 5; h.bars.push_back(b); }
    mock::Bar& p = h.bars[PK]; p.rows.push_back(mock::Price(100, 0, 10000, 10000)); p.volume += 10000;
    p.low = 99; p.high = 104; p.open = 100.5f; p.close = 101;          // lower high than the bar before: still the down leg
    return h;
}
static void add(mock::Host& h, double lo, double hi) { mock::Bar b = h.bars.back(); b.time += 60;
    b.rows = { mock::Price(120,1,0,1) }; b.volume = 1; b.low = static_cast<float>(lo); b.high = static_cast<float>(hi); b.open = b.close = static_cast<float>((lo + hi) / 2); h.bars.push_back(b); }
static void redraw(mock::Host& h, DeltaProfile& p) { h.draws.clear(); if (h.data) state(h).builtAt -= 1001; mock::current = &h; p.draw(); }
int main() {
    setenv("TZ", "UTC", 1); tzset();
    const unsigned long t0 = 1791504000UL + 9 * 3600;
    const long long session = 20735;
    // 1) shown before the pivot is confirmed, recorded once it is
    {   const std::string dir = home("confirm"), path = dir + "ES-ESZ26-60-signals-" + std::to_string(session) + ".csv";
        mock::Host h = chart(t0); add(h, 99.5, 103.5);                  // bar PK+1 forming: no turn yet
        DeltaProfile p; redraw(h, p);
        check(rings(h) == 1, "tentative A: the bubble shows while the pivot holds on the bars so far");
        check(dLines(slurp(path), "A") == 0, "tentative A: nothing recorded yet");
        h.bars.back().high = 104.5f; add(h, 100, 105);                  // PK+1 closed with a higher high: the down pivot PK is confirmed
        redraw(h, p);
        check(dLines(slurp(path), "A") == 1, "confirmed: the A is recorded (D line)");
        check(rings(h) == 1, "confirmed: the bubble stays");
        add(h, 60, 70); add(h, 61, 71); redraw(h, p);                   // later bars can no longer change a recorded A
        check(rings(h) == 1 && dLines(slurp(path), "A") == 1, "recorded A never repaints");
        mock::current = &h; p.destroy(); latchBooks().clear(); }
    // 2) shown, then gone when a later bar becomes the swing's down pivot far below; never recorded
    {   const std::string dir = home("fail"), path = dir + "ES-ESZ26-60-signals-" + std::to_string(session) + ".csv";
        mock::Host h = chart(t0); add(h, 99.5, 103.5);
        DeltaProfile p; redraw(h, p);
        check(rings(h) == 1, "fail: shown first");
        h.bars.back().low = 60; h.bars.back().high = 70; add(h, 59, 69);   // the down leg goes on: the pivot is now far below
        redraw(h, p);
        check(rings(h) == 0, "fail: the bubble goes when the pivot rule no longer holds");
        add(h, 60, 72); redraw(h, p);                                   // the turn up confirms the pivot at 59: final - not A
        check(rings(h) == 0 && dLines(slurp(path), "A") == 0, "fail: never recorded as A");
        mock::current = &h; p.destroy(); latchBooks().clear(); }
    // 3) gone, then back: a lower low that is still above the A's own high keeps the rule (low < pivot high)
    {   const std::string dir = home("back"), path = dir + "ES-ESZ26-60-signals-" + std::to_string(session) + ".csv";
        mock::Host h = chart(t0); add(h, 98, 100.5);                    // new low 98, its high 100.5: PK low 99 < 100.5 holds
        DeltaProfile p; redraw(h, p);
        check(rings(h) == 1, "back: shown while the new down pivot's high is above the A bar's low");
        h.bars.back().low = 90; h.bars.back().high = 98.5f; redraw(h, p); // forming bar sinks: 99 < 98.5 fails
        check(rings(h) == 0, "back: gone while the forming bar is the pivot");
        h.bars.back().low = 98; h.bars.back().high = 100.5f; redraw(h, p); // it comes back up before closing
        check(rings(h) == 1, "back: shown again");
        add(h, 99, 102); redraw(h, p);
        check(dLines(slurp(path), "A") == 1 && rings(h) == 1, "back: confirmed and recorded");
        mock::current = &h; p.destroy(); latchBooks().clear(); }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
