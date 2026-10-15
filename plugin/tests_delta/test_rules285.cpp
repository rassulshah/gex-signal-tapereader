// test_rules285.cpp -- lsDeltaProfile 2.8.5 (Rassul approved 2026-10-14 23:05 CT) on the mock SDK, through the plugin (records ->
// book -> bubbles / labels / status):
//   rule 1  ONE bubble per candle per side: bullish at the LOWEST price, bearish at the HIGHEST; the profile A label follows the
//           same choice; records unchanged; load-order independent; the same after a restart; never repainted by later bars; a later
//           duplicate never moves a drawn bubble, a lower (bullish) / higher (bearish) A decided at the same first moment does.
//   rule 2  a one-time re-check of A records written before 2.8.4: fails halfway -> hidden, fails pivot -> hidden, passes -> shown,
//           bars not on the chart -> undrawn and said so; a 2.8.4+ record is never re-checked.
// Build (Linux, from tests_delta):  g++ -std=c++17 -g -fsanitize=address,undefined -I. test_rules285.cpp -o t && ./t
#include "../DeltaProfile.cpp"
#include <iostream>
#include <functional>
#include <cstdlib>
#include <random>
#include <sys/stat.h>
#include <unistd.h>
#include <set>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) { ++passed; std::cout << "PASS " << why << '\n'; } else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static void spit(const std::string& p, const std::string& t) { std::ofstream o(p.c_str(), std::ios::binary | std::ios::trunc); o << t; }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static std::string home(const std::string& name) {
    const char* r = std::getenv("DP254_RUN"); const std::string root = r && r[0] ? r : "/tmp/dp254-run";
    const std::string h = root + "/r285_" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels", h + "/InvestorRT/rtx/lsFlexLevels/DeltaProfile" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    return h + "/InvestorRT/rtx/lsFlexLevels/DeltaProfile/";
}
static const unsigned long DAY = 1791504000UL;                      // 2026-10-09 00:00 (TZ=UTC)
static const unsigned long T0 = DAY + 9 * 3600 + 30 * 60;           // 09:30
static std::string recordPath(const std::string& dir) { return dir + "ES-ESZ26-60-signals-20735.csv"; }
static std::string statusOf(const std::string& dir) { return slurp(dir + "../DeltaProfile.status.txt"); }
struct Ring { int x, y; COLOR c; bool operator<(const Ring& o) const { return x != o.x ? x < o.x : y != o.y ? y < o.y : c < o.c; } bool operator==(const Ring& o) const { return x == o.x && y == o.y && c == o.c; } };
static std::vector<Ring> rings(const mock::Host& h) { std::vector<Ring> o; for (const mock::Draw& d : h.draws) if (d.type == "ring") o.push_back(Ring{ d.x, d.y, d.color }); std::sort(o.begin(), o.end()); return o; }
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
static LatchBook& book() { return latchBooks().begin()->second; }
static void freshDraw(DeltaProfile& p, mock::Host& h) { h.draws.clear(); mock::current = &h; if (h.data) { state(h).builtAt -= 100000; state(h).statusAt = -1; } p.draw(); }
// the 2.8.4 suite's chart: bars 0..19 a down leg (bar 19 = the down pivot, low 101 / high 103), 20.. an up leg; bar 30 = the peak.
// Every bar trades 1 lot each side at its close (delta 0): the live profile has no nodes, so only the records under test exist.
struct Peak { double low, high, open, close; };
static mock::Host chart(int count, const Peak& pk) {
    mock::Host h; h.seconds = 60; h.root = "ES"; h.symbol = "ESZ26"; h.tick = 0.25f;
    for (int i = 0; i < count; ++i) {
        mock::Bar b; b.time = T0 + static_cast<unsigned long>(i * 60);
        const double lo = i < 20 ? 120 - i : i < 30 ? 102 + (i - 20) : pk.high - 3 + 0.25 * (i - 30);
        b.low = static_cast<float>(lo); b.high = static_cast<float>(lo + 2); b.open = static_cast<float>(lo + 0.5); b.close = static_cast<float>(lo + 1.5);
        h.bars.push_back(b);
    }
    mock::Bar& p = h.bars[30];
    p.low = static_cast<float>(pk.low); p.high = static_cast<float>(pk.high); p.open = static_cast<float>(pk.open); p.close = static_cast<float>(pk.close);
    for (mock::Bar& b : h.bars) { b.rows = { mock::Price(b.close, 1, 1, 2) }; b.volume = 2; }
    return h;
}
static const Peak PASS{ 101.5, 116, 102, 115.5 };                   // green, close above, lower half, low below the down-pivot's high
static const Peak PIVOT_FAIL{ 112, 116, 112.5, 115.5 };              // low 112 > down-pivot bar 19's high 103, no new down pivot
// one decided A event as an F + D line pair written by `version`
struct Ev { long long id; int bar; double price; bool support; long long tDecided; std::string version; };
static std::string lines(const mock::Host& h, const Ev& e) {
    Latch L; L.id = e.id; L.code = "A"; L.support = e.support; L.sell = e.support; L.zone = false;
    L.low = L.high = dp::priceKey(e.price, 0.25); L.price = e.price;
    L.peakTime = h.bars[static_cast<std::size_t>(e.bar)].time; L.peakT = static_cast<long long>(L.peakTime);
    L.formedTime = L.peakTime; L.formedT = L.peakT; L.tFirst = e.tDecided; L.tDecided = e.tDecided; L.decidedBy = L.peakT; L.net = e.support ? -500 : 500; L.mult = 3;
    std::string f = recordLine(L, 'F', 0.25, "ESZ26"), d = recordLine(L, 'D', 0.25, "ESZ26");
    const std::string cur = std::string("|") + DLT_VERSION + "|", want = "|" + e.version + "|";
    f.replace(f.find(cur), cur.size(), want); d.replace(d.find(cur), cur.size(), want);
    return f + "\n" + d + "\n";
}
static std::string fdLines(const std::string& t) { std::istringstream in(t); std::string l, o; while (std::getline(in, l)) { const std::vector<std::string> c = fields(l); if (c.size() == REC_FIELDS) o += l + "\n"; } return o; }
static std::string file(const mock::Host& h, const std::vector<Ev>& v) { std::string t = std::string(recordHeader()) + "\n"; for (const Ev& e : v) t += lines(h, e); return t; }
static bool ringAtPrice(const mock::Host& h0, int bar, double price) {
    mock::Host& h = const_cast<mock::Host&>(h0); mock::current = &h; cppExtension::PNT p; p.set(bar, static_cast<float>(price), cppExtension::kBarCenter);
    for (const Ring& r : rings(h)) if (r.y == p.v) return true;
    return false;
}
static std::string disp(mock::Host& h, long long id) { const auto it = state(h).bubbleState.find(id); return it == state(h).bubbleState.end() ? "none" : it->second; }

int main()
{
    setenv("TZ", "UTC", 1); tzset();
    const long long TD = static_cast<long long>(T0 + 31 * 60);       // decided while bar 31 formed (the peak bar's close)
    // ---- 1) several A records on one candle -> ONE bubble, bullish at the LOWEST price; records unchanged ----
    {
        const std::string dir = home("bull3");
        mock::Host h = chart(110, PASS);
        const std::vector<Ev> v = { { 0, 30, 105.00, true, TD, "2.8.5" }, { 1, 30, 104.50, true, TD, "2.8.5" }, { 2, 30, 106.00, true, TD, "2.8.5" } };
        const std::string text = file(h, v); spit(recordPath(dir), text);
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        check(book().v.size() == 3, "fixture: 3 A records loaded, no live events (" + std::to_string(book().v.size()) + ")");
        check(rings(h).size() == 1, "bullish: 3 A on one candle -> ONE bubble (" + std::to_string(rings(h).size()) + ")");
        check(ringAtPrice(h, 30, 104.50), "bullish: the bubble is at the LOWEST price (104.50)");
        check(disp(h, 1) == "drawn" && disp(h, 0) == "merged:1" && disp(h, 2) == "merged:1", "status: 1 drawn, the others merged:1");
        check(fdLines(slurp(recordPath(dir))) == fdLines(text) && slurp(recordPath(dir)).compare(0, text.size(), text) == 0, "display only: the record's F / D lines are unchanged (append-only file, no new decision)");
        const std::string st = statusOf(dir);
        check(st.find("BUBBLES_MERGED,2") != std::string::npos && st.find("EVENT_SYNC,0,A,support,105,3,-500") != std::string::npos && st.find(",merged:1,") != std::string::npos,
              "status: BUBBLES_MERGED,2 and each event's disposition");
        mock::current = &h; p->destroy(); latchBooks().clear();
    }
    // ---- 1b) bearish: ONE bubble at the HIGHEST price ----
    {
        const std::string dir = home("bear3");
        mock::Host h = chart(110, PASS);
        const std::vector<Ev> v = { { 0, 40, 110.00, false, TD, "2.8.4" }, { 1, 40, 111.00, false, TD, "2.8.4" }, { 2, 40, 110.50, false, TD, "2.8.4" } };
        spit(recordPath(dir), file(h, v));
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        check(rings(h).size() == 1 && ringAtPrice(h, 40, 111.00), "bearish: 3 A on one candle -> ONE bubble at the HIGHEST price (111.00)");
        check(disp(h, 1) == "drawn" && disp(h, 0) == "merged:1" && disp(h, 2) == "merged:1", "bearish: 1 drawn, the others merged");
        check(statusOf(dir).find("RECHECK_PRE284") == std::string::npos, "2.8.4 records are never re-checked (no RECHECK line)");
        mock::current = &h; p->destroy(); latchBooks().clear();
    }
    // ---- 2) two candles -> two bubbles; one candle both sides -> one bubble per side ----
    {
        const std::string dir = home("two");
        mock::Host h = chart(110, PASS);
        const std::vector<Ev> v = { { 0, 30, 105.0, true, TD, "2.8.5" }, { 1, 30, 104.0, true, TD, "2.8.5" },
                                    { 2, 45, 113.0, true, TD + 900, "2.8.5" }, { 3, 45, 112.5, true, TD + 900, "2.8.5" },
                                    { 4, 45, 118.0, false, TD + 900, "2.8.5" } };
        spit(recordPath(dir), file(h, v));
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        check(rings(h).size() == 3, "two candles (bullish) + the second candle's bearish side -> 3 bubbles (" + std::to_string(rings(h).size()) + ")");
        check(ringAtPrice(h, 30, 104.0) && disp(h, 1) == "drawn" && disp(h, 3) == "drawn" && disp(h, 4) == "drawn", "each candle's lowest bullish A + the bearish A hold the bubbles");
        mock::current = &h; p->destroy(); latchBooks().clear();
    }
    // ---- 3) load-order independence + 4) restart = the same picture ----
    {
        const std::string dir = home("order");
        mock::Host h = chart(110, PASS);
        std::vector<Ev> v = { { 0, 30, 105.0, true, TD, "2.8.5" }, { 1, 30, 104.25, true, TD, "2.8.5" }, { 2, 30, 103.75, true, TD + 60, "2.8.5" },
                              { 3, 30, 106.0, true, TD, "2.8.5" }, { 4, 45, 113.0, true, TD + 900, "2.8.5" }, { 5, 45, 112.0, true, TD + 900, "2.8.5" },
                              { 6, 50, 120.0, false, TD + 1200, "2.8.5" }, { 7, 50, 121.0, false, TD + 1260, "2.8.5" } };
        spit(recordPath(dir), file(h, v));
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        const std::vector<Ring> ref = rings(h); const std::map<long long, std::string> refState = state(h).bubbleState;
        check(ref.size() == 3 && ringAtPrice(h, 30, 104.25) && ringAtPrice(h, 45, 112.0) && ringAtPrice(h, 50, 120.0),
              "reference: 104.25 (103.75 was decided later), 112.00, and the FIRST bearish 120.00 (121.00 decided later)");
        check(refState.at(2) == "merged:1" && refState.at(7) == "merged:6", "later duplicates are merged, not drawn");
        mock::current = &h; p->destroy(); latchBooks().clear();
        std::mt19937 rng(285); int same = 0;
        for (int k = 0; k < 25; ++k) {
            std::shuffle(v.begin(), v.end(), rng);
            spit(recordPath(dir), file(h, v));                           // the same events, appended in another order
            mock::Host g = h; g.data = NULL; DeltaProfile Q; DeltaProfile* q = &Q; freshDraw(*q, g);
            if (rings(g) == ref && state(g).bubbleState == refState) ++same;
            mock::current = &g; q->destroy(); latchBooks().clear();
        }
        check(same == 25, "25 shuffled record orders give the same bubbles and dispositions (" + std::to_string(same) + "/25)");
        // restart (destroy, clear, reload from the record) x3: identical
        spit(recordPath(dir), file(h, v));
        bool stable = true;
        for (int k = 0; k < 3; ++k) {                                     // each = an IRT restart: a new plugin object, the book re-seeded
            mock::Host g = h; g.data = NULL; DeltaProfile R; freshDraw(R, g);
            stable = stable && rings(g) == ref && state(g).bubbleState == refState;
            mock::current = &g; R.destroy(); latchBooks().clear();
        }
        check(stable, "3 restarts: the same bubbles and dispositions");
    }
    // ---- 5) no repaint after later bars (and the live path: a later duplicate cannot move it; a same-moment lower one can) ----
    {
        const std::string dir = home("live");
        mock::Host h = chart(110, PASS);
        spit(recordPath(dir), file(h, { { 0, 30, 105.0, true, TD, "2.8.5" } }));
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        const std::vector<Ring> r1 = rings(h);
        check(r1.size() == 1 && ringAtPrice(h, 30, 105.0), "live: one bubble at 105.00");
        LatchBook& bk = book();
        Latch late = bk.v[0]; late.id = bk.nextId++; late.price = 103.0; late.low = late.high = dp::priceKey(103.0, 0.25); late.tDecided = TD + 180;
        bk.v.push_back(late); freshDraw(*p, h);                           // a re-track decided 3 bars later, LOWER
        check(rings(h) == r1 && disp(h, late.id) == "merged:0", "a LOWER duplicate decided later never moves the drawn bubble (merged:0)");
        for (int k = 0; k < 40; ++k) {                                    // 40 later bars with lower lows
            mock::Bar b = h.bars.back(); b.time += 60; const float lo = static_cast<float>(100 - 0.5 * k);
            b.low = lo; b.high = lo + 1; b.open = lo + 0.75f; b.close = lo + 0.25f; b.rows = { mock::Price(b.close, 1, 1, 2) }; b.volume = 2; h.bars.push_back(b);
            freshDraw(*p, h);
        }
        check(rings(h) == r1, "after 40 lower-low bars: the bubble has not moved");
        Latch same = bk.v[0]; same.id = bk.nextId++; same.price = 104.0; same.low = same.high = dp::priceKey(104.0, 0.25); same.tDecided = TD;
        bk.v.push_back(same); freshDraw(*p, h);                           // decided at the SAME first moment, lower: it takes the bubble
        check(rings(h).size() == 1 && ringAtPrice(h, 30, 104.0) && disp(h, same.id) == "drawn" && disp(h, 0) == "merged:" + std::to_string(same.id),
              "a LOWER A decided at the same first moment takes the bubble (the one allowed move)");
        mock::current = &h; p->destroy(); latchBooks().clear();
    }
    // ---- 6) the one-time re-check of pre-2.8.4 records ----
    {   // passes -> shown
        const std::string dir = home("pre_pass");
        mock::Host h = chart(110, PASS);
        spit(recordPath(dir), file(h, { { 0, 30, 101.5, true, TD, "2.8.0" } }));
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        check(rings(h).size() == 1 && disp(h, 0) == "drawn", "pre-2.8.4 A that passes close / halfway / pivot -> shown");
        const std::string st = statusOf(dir);
        check(st.find("RECHECK_PRE284,0,close ok,halfway ok,pivot ok") != std::string::npos && st.find(",shown") != std::string::npos, "status: RECHECK_PRE284 line, all ok, shown");
        mock::current = &h; p->destroy(); latchBooks().clear();
    }
    {   // fails halfway -> hidden (node at 115.00: upper half of the 101.5 - 116 bar; close 115.5 above it; pivot ok)
        const std::string dir = home("pre_half");
        mock::Host h = chart(110, PASS);
        const std::string text = file(h, { { 0, 30, 115.0, true, TD, "2.8.0" } }); spit(recordPath(dir), text);
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        check(rings(h).empty() && disp(h, 0) == "hidden: pre-2.8.4 rule (halfway)", "pre-2.8.4 A in the upper half -> hidden: pre-2.8.4 rule (halfway) [" + disp(h, 0) + "]");
        const std::string st = statusOf(dir);
        check(st.find("A_HIDDEN_PRE284,1") != std::string::npos && st.find("halfway fail") != std::string::npos && fdLines(slurp(recordPath(dir))) == fdLines(text),
              "status says hidden, the record is unchanged");
        check(book().v[0].code == "A", "the record stays an A (display only)");
        mock::current = &h; p->destroy(); latchBooks().clear();
    }
    {   // fails pivot -> hidden
        const std::string dir = home("pre_pivot");
        mock::Host h = chart(110, PIVOT_FAIL);
        spit(recordPath(dir), file(h, { { 0, 30, 112.0, true, TD, "2.8.0" } }));
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        check(rings(h).empty() && disp(h, 0) == "hidden: pre-2.8.4 rule (pivot)", "pre-2.8.4 A failing the pivot rule -> hidden: pre-2.8.4 rule (pivot) [" + disp(h, 0) + "]");
        mock::current = &h; p->destroy(); latchBooks().clear();
    }
    {   // a 2.8.4+ record on the SAME failing bar is never re-checked: drawn
        const std::string dir = home("post_pivot");
        mock::Host h = chart(110, PIVOT_FAIL);
        spit(recordPath(dir), file(h, { { 0, 30, 112.0, true, TD, "2.8.4" }, { 1, 30, 115.75, true, TD, "2.8.5" } }));
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        check(rings(h).size() == 1 && ringAtPrice(h, 30, 112.0) && disp(h, 0) == "drawn" && book().recheck.empty(),
              "2.8.4 / 2.8.5 records on a bar that fails the rules: never re-checked, drawn (lowest of the two)");
        mock::current = &h; p->destroy(); latchBooks().clear();
    }
    {   // the re-check happens BEFORE the one-bubble choice: a hidden lower duplicate does not take the bubble from a passing one
        const std::string dir = home("pre_mix");
        mock::Host h = chart(110, PASS);
        spit(recordPath(dir), file(h, { { 0, 30, 101.5, true, TD, "2.8.0" }, { 1, 30, 115.0, true, TD, "2.8.0" }, { 2, 30, 101.25, true, TD + 60, "2.8.0" } }));
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        check(rings(h).size() == 1 && ringAtPrice(h, 30, 101.5) && disp(h, 1) == "hidden: pre-2.8.4 rule (halfway)" && disp(h, 2) == "merged:0",
              "mixed: 101.50 shown, 115.00 hidden (halfway), 101.25 decided later -> merged");
        mock::current = &h; p->destroy(); latchBooks().clear();
    }
    {   // bars not on the chart: the peak bar is before the chart's first bar -> undrawn, said so; once the bars arrive it is decided once
        const std::string dir = home("pre_nobars");
        mock::Host full = chart(110, PASS);
        const std::string text = file(full, { { 0, 30, 101.5, true, TD, "2.8.0" } });
        mock::Host h = full; h.bars.erase(h.bars.begin(), h.bars.begin() + 31);   // the chart starts after the peak bar
        spit(recordPath(dir), text);
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        check(rings(h).empty() && disp(h, 0) == "undrawn: pre-2.8.4 re-check - bars not on chart", "pre-2.8.4 A whose bars are not on the chart -> undrawn [" + disp(h, 0) + "]");
        check(statusOf(dir).find("A_UNDRAWN_PRE284_NO_BARS,1") != std::string::npos, "status: A_UNDRAWN_PRE284_NO_BARS,1");
        mock::current = &h; p->destroy(); latchBooks().clear();
        mock::Host g = full; DeltaProfile Q; DeltaProfile* q = &Q; freshDraw(*q, g);   // the history back-filled: checked, shown
        check(rings(g).size() == 1 && disp(g, 0) == "drawn", "with the bars on the chart the same record is re-checked and shown");
        mock::current = &g; q->destroy(); latchBooks().clear();
    }
    {   // the re-check is decided ONCE: later bars (even ones that would change a fresh pivot walk) never flip it
        const std::string dir = home("pre_once");
        mock::Host h = chart(110, PASS);
        spit(recordPath(dir), file(h, { { 0, 30, 101.5, true, TD, "2.8.0" } }));
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        const std::vector<Ring> r1 = rings(h);
        for (int k = 0; k < 20; ++k) { mock::Bar b = h.bars.back(); b.time += 60; b.low -= 3; b.high -= 3; b.open -= 3; b.close -= 3; b.rows = { mock::Price(b.close, 1, 1, 2) }; h.bars.push_back(b); freshDraw(*p, h); }
        check(r1.size() == 1 && rings(h) == r1 && book().recheck.size() == 1, "20 later bars: the re-checked A keeps its bubble; one cached verdict");
        mock::current = &h; p->destroy(); latchBooks().clear();
    }
    // ---- 7) the profile A label follows the same choice: at most ONE A label per candle per side ----
    {
        home("label");
        // the 2.8.4 suite's live A: a node of 10,000 lots sold at 101.50 on the green peak bar 30 (passes every rule)
        mock::Host h = chart(110, PASS);
        h.bars[30].rows = { mock::Price(101.5f, 0, 10000, 10000), mock::Price(h.bars[30].close, 1, 0, 1) }; h.bars[30].volume = 10001;
        for (int i = 0; i < 110; ++i) if (i != 30) { h.bars[static_cast<std::size_t>(i)].rows = { mock::Price(h.bars[static_cast<std::size_t>(i)].close, 1, 0, 1) }; h.bars[static_cast<std::size_t>(i)].volume = 1; }
        DeltaProfile P; DeltaProfile* p = &P; freshDraw(*p, h);
        auto aLabels = [&]() { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "text" && (d.text.compare(0, 2, "A ") == 0 || d.text == "A")) ++n; return n; };
        check(book().v.size() == 1 && book().v[0].code == "A" && aLabels() == 1 && rings(h).size() == 1, "fixture: the live A with its bubble and one A label");
        LatchBook& bk = book();
        Latch lower = bk.v[0]; lower.id = bk.nextId++; lower.price = 101.25; lower.low = lower.high = dp::priceKey(101.25, 0.25);
        bk.v.push_back(lower); freshDraw(*p, h);                          // same candle, same first moment, LOWER: holds the bubble
        check(rings(h).size() == 1 && ringAtPrice(h, 30, 101.25), "the lower same-moment A holds the candle's one bubble");
        check(aLabels() <= 1 && state(h).profileState[bk.v[0].id] == "suppressed:merged:" + std::to_string(lower.id),
              "the merged event's profile A label is suppressed (one label per candle per side) [" + state(h).profileState[bk.v[0].id] + "]");
        mock::current = &h; p->destroy(); latchBooks().clear();
    }
    // ---- version ----
    {
        const std::string src = slurp("../DeltaProfile.cpp"); const std::size_t at = src.find("setVersion(\"");
        const std::string lit = at == std::string::npos ? "" : src.substr(at + 12, src.find('"', at + 12) - at - 12);
        check(std::string(DLT_VERSION) == "2.8.5" && lit == DLT_VERSION, "DLT_VERSION 2.8.5 = the setVersion literal (" + lit + ")");
        check(slurp("../DeltaProfileCore.h").find("DeltaProfileCore.h 2.8.5") != std::string::npos, "DeltaProfileCore.h header says 2.8.5");
    }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
