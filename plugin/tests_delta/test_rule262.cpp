// test_rule262.cpp -- lsDeltaProfile 2.6.2: absorption is decided by the node's own peak bar (Rassul 2026-10-13 05:16)
#include "../DeltaProfile.cpp"
#include <iostream>
#include <functional>
#include <random>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) ++passed; else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static bool exists(const std::string& p) { struct stat st; return stat(p.c_str(), &st) == 0; }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static std::string home(const std::string& name) {
    const char* r = std::getenv("DP254_RUN"); const std::string root = r && r[0] ? r : "/tmp/dp254-run";   // scratch folder for the record files
    const std::string h = root + "/" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    return h + "/InvestorRT/rtx/lsFlexLevels/DeltaProfile/";            // NOT created here: the plugin must create it
}
static int lines(const std::string& text, const char* event) {
    int n = 0; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() == REC_FIELDS && c[12] == event) ++n; }
    return n;
}
static int linesWithCode(const std::string& text, const char* event, const char* code) {
    int n = 0; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() == REC_FIELDS && c[12] == event && c[2] == code) ++n; }
    return n;
}

static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
static std::vector<std::vector<std::string> > rows(const std::string& text, const char* event) {
    std::vector<std::vector<std::string> > out; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() == REC_FIELDS && c[12] == event) out.push_back(c); }
    return out;
}
static int rings(const mock::Host& h) { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "ring") ++n; return n; }
// 110 one-minute dojis with light buying above 120; bar `peak` holds a heavy SELLING print at 100 with the given open/close
static mock::Host chart(unsigned long t0, int count, int peak, double o, double c) {
    mock::Host h; h.seconds = 60; h.root = "ES"; h.tick = 0.25f;
    for (int i = 0; i < count; ++i) { mock::Bar b; b.time = t0 + static_cast<unsigned long>(i * 60);
        b.rows = { mock::Price(120,1,0,1), mock::Price(121,1,0,1), mock::Price(122,1,0,1), mock::Price(123,1,0,1), mock::Price(124,1,0,1) };
        b.volume = 5; b.low = 100; b.high = 125; b.open = 104; b.close = 104; h.bars.push_back(b); }
    if (peak >= 0 && peak < count) {
        h.bars[peak].rows.push_back(mock::Price(100, 0, 10000, 10000)); h.bars[peak].volume += 10000;
        h.bars[peak].low = 99; h.bars[peak].high = 125; h.bars[peak].open = static_cast<float>(o); h.bars[peak].close = static_cast<float>(c);
    }
    return h;
}
int main()
{
    setenv("TZ", "UTC", 1); tzset();
    const unsigned long day = 1791504000UL;                             // 2026-10-09 00:00 (wall-clock fields)
    const long long session = 20735;
    const unsigned long t0 = day + 9 * 3600 + 30 * 60;
    // 1) the node forms AFTER its peak bar closed: decided at formation from that closed bar; t_decided = formation time, never back-dated
    {
        const std::string dir = home("formed_after_peak"); const std::string path = dir + "ES-ESZ26-60-signals-" + std::to_string(session) + ".csv";
        mock::Host full = chart(t0, 110, 40, 99, 101);                  // peak bar 40 closes GREEN ABOVE the selling node at 100
        mock::Host h = full; h.bars.resize(40);                         // before bar 40 exists: no node
        DeltaProfile p; mock::current = &h; p.draw();
        check(latchBooks().begin()->second.v.empty(), "no node before its bars exist");
        h.bars = full.bars; mock::current = &h; p.draw();               // the node appears (formed) with bar 40 long closed
        const std::string text = slurp(path);
        const auto F = rows(text, "F"), D = rows(text, "D");
        check(F.size() == 1 && D.size() == 1, "formed after peak: one F and one D at formation");
        if (F.size() == 1 && D.size() == 1) {
            const std::string formation = std::to_string(t0 + 109 * 60);   // the mock stamp is the bar END
            check(D[0][2] == "A" && D[0][3] == "support", "formed after peak: green-above peak bar = A support");
            check(F[0][0] == formation && D[0][0] == formation && D[0][1] == formation, "t_first = t_decided = formation time (" + D[0][1] + ")");
            check(D[0][10] == std::to_string(t0 + 40 * 60), "decided_by_t = the peak bar's end (" + D[0][10] + ")");
            check(D[0][11] == DLT_VERSION, "D line carries the package version");
        }
        check(rings(h) == 1, "the confirmed A draws one circle");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    // 2) the forming peak bar is never used; it decides only once closed; a RED peak bar then a later GREEN close above is NOT A
    {
        const std::string dir = home("forming_peak"); const std::string path = dir + "ES-ESZ26-60-signals-" + std::to_string(session) + ".csv";
        mock::Host h = chart(t0, 110, 109, 99, 101);                    // the peak bar is the LAST (forming) bar, green above for now
        DeltaProfile p; mock::current = &h; p.draw();
        check(latchBooks().begin()->second.v.size() == 1 && latchBooks().begin()->second.v[0].code == "A?", "forming peak bar: pending A?");
        check(rows(slurp(path), "D").empty() && rings(h) == 0, "forming peak bar: no D line, no circle");
        h.bars[109].open = 101; h.bars[109].close = 100;                // still forming: turns RED, closes on the node
        h.draws.clear(); state(h).builtAt -= 1001; mock::current = &h; p.draw();
        check(latchBooks().begin()->second.v[0].code == "A?", "forming peak bar still not decided");
        mock::Bar n = h.bars.back(); n.time += 60; n.rows = { mock::Price(120,1,0,1) }; n.volume = 1; n.open = 104; n.close = 104; n.low = 100; n.high = 125;
        h.bars.push_back(n);                                            // the peak bar is now CLOSED red on the node: not A
        h.draws.clear(); mock::current = &h; p.draw();
        check(latchBooks().begin()->second.v[0].code == "I?", "red peak bar closed: absorption ruled out (I?) - got " + latchBooks().begin()->second.v[0].code);
        mock::Bar g = h.bars.back(); g.time += 60; g.open = 103; g.close = 106; h.bars.push_back(g);
        mock::Bar f = g; f.time += 60; h.bars.push_back(f);              // a later GREEN close above, now closed
        h.draws.clear(); mock::current = &h; p.draw();
        check(latchBooks().begin()->second.v[0].code != "A", "a later green close above never makes A");
        check(rings(h) == 0 && linesWithCode(slurp(path), "D", "A") == 0, "no circle and no A line for a red peak bar");
        mock::Bar r = h.bars.back(); r.time += 60; r.open = 99.5f; r.low = 97; r.close = 98; h.bars.push_back(r);
        mock::Bar r2 = r; r2.time += 60; h.bars.push_back(r2);          // a red close below the selling node: sellers won = I
        h.draws.clear(); mock::current = &h; p.draw();
        check(latchBooks().begin()->second.v[0].code == "I" && !latchBooks().begin()->second.v[0].support, "red close below after a failed peak = I");
        check(rings(h) == 0, "I never draws a circle");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    // 3) (2.6.3: in this contract's own file) records written by 2.6.1 under the OLD rule keep loading and keep their A circles (never repaint), even when the peak bar
    //    would fail the 2.6.2 rule
    {
        const std::string dir = home("old_record"); const std::string path = dir + "ES-ESZ26-60-signals-" + std::to_string(session) + ".csv";
        mock::Host h = chart(t0, 110, 40, 99, 101);
        { DeltaProfile p; mock::current = &h; p.draw(); mock::current = &h; p.destroy(); latchBooks().clear(); }
        std::string text = slurp(path);
        check(linesWithCode(text, "D", "A") == 1, "fixture: an A line was written");
        { const std::string cur = std::string("|") + DLT_VERSION + "|"; int swapped = 0;   // (2.6.3) re-tag the current version as 2.6.1
          for (std::size_t at = text.find(cur); at != std::string::npos; at = text.find(cur, at)) { text.replace(at, cur.size(), "|2.6.1|"); ++swapped; }
          check(swapped > 0, "fixture: lines re-tagged as written by 2.6.1"); }
        { std::ofstream o(path.c_str(), std::ios::binary | std::ios::trunc); o << text; }
        mock::Host h2 = chart(t0, 110, 40, 100.5, 100);                 // the same node, but its peak bar is RED on the node (fails 2.6.2)
        DeltaProfile p; mock::current = &h2; p.draw();                  // an IRT restart: the book is seeded from the 2.6.1 record
        check(latchBooks().begin()->second.loaded == 1 && latchBooks().begin()->second.v.size() == 1 && latchBooks().begin()->second.v[0].code == "A",
              "old 2.6.1 A record loads as A");
        // (2.8.5 intended rule change, Rassul 2026-10-14 23:05 CT) a record decided before 2.8.4 is re-checked ONCE against the 2.8.4
        // rules: this peak bar is RED on the node (fails the same-bar close rule) -> not drawn, the record unchanged, the status says why
        // (2.6.2-2.8.4 kept its circle: "old 2.6.1 A record keeps its circle")
        check(rings(h2) == 0, "2.8.5: old 2.6.1 A record that fails the 2.8.4 close rule is NOT drawn");
        { const std::string st = slurp(dir + "../DeltaProfile.status.txt");
          check(st.find("hidden: pre-2.8.4 rule (close") != std::string::npos && st.find("A_HIDDEN_PRE284,1") != std::string::npos, "2.8.5: status says hidden: pre-2.8.4 rule (close ...)"); }
        check(linesWithCode(slurp(path), "D", "A") == 1 && rows(slurp(path), "D").size() == 1, "no new decision line appended for the old A");
        mock::current = &h2; p.destroy(); latchBooks().clear();
    }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
