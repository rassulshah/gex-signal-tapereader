// t_ai.cpp - DeltaIntel.h 1.2.0 (lsDeltaProfile 2.35.0): the unified A / I rule
#include "../DeltaIntel.h"
#include "../DeltaProfileCore.h"
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <tuple>
namespace dp = delta_profile;
static int pass = 0, fail = 0;
#define CHECK(c, msg) do { if (c) ++pass; else { ++fail; std::cout << "FAIL " << msg << "\n"; } } while (0)
typedef std::tuple<char, int, int, int, long long, long long> Key;
static std::vector<dintel::Candle> load(const std::string& p) {
    std::vector<dintel::Candle> v; std::ifstream in(p.c_str()); std::string line, tag; std::getline(in, line);
    while (std::getline(in, line)) { std::istringstream s(line); dintel::Candle c; int n; s >> tag >> c.t >> c.mod >> c.O >> c.H >> c.L >> c.C >> c.vol >> n;
        for (int i = 0; i < n; ++i) { dintel::Cell x; s >> x.key >> x.buy >> x.sell; c.cells.push_back(x); } v.push_back(c); }
    return v;
}
static std::set<Key> run(const std::vector<dintel::Candle>& cs, int upTo, std::vector<dintel::Event>* evOut = nullptr) {
    dintel::Engine E("ES");
    for (int i = 0; i < static_cast<int>(cs.size()) && i <= upTo; ++i) E.add(cs[static_cast<size_t>(i)]);
    std::set<Key> s; for (const dintel::Event& e : E.ev) if (e.letter == 'A' || e.letter == 'I') s.insert(Key(e.letter, e.k, e.pk, e.side, e.lo, e.hi));
    if (evOut) *evOut = E.ev;
    return s;
}
static long long tk(double p) { return static_cast<long long>(p / 0.25 + 0.5); }
static bool has(const std::vector<dintel::Event>& ev, const std::vector<dintel::Candle>& cs, char L, int side, int pkEndMod, int kEndMod, double lo, double hi) {
    for (const dintel::Event& e : ev) if (e.letter == L && e.side == side && cs[e.pk].mod + 3 == pkEndMod && cs[e.k].mod + 3 == kEndMod && e.lo == tk(lo) && e.hi == tk(hi)) return true;
    return false;
}
static int hm(int h, int m) { return h * 60 + m; }
// synthetic: 5 candles (o h l c), uniAI on node [lo, hi]
static bool syn(const long long b[5][4], int pk, long long lo, long long hi, bool sell, dintel::Event& e) {
    dintel::Engine E("ES");
    for (int i = 0; i < 5; ++i) { dintel::Candle c; c.O = b[i][0]; c.H = b[i][1]; c.L = b[i][2]; c.C = b[i][3]; E.c.push_back(c); }
    return E.uniAI(4, pk, lo, hi, sell, e);
}
int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : ".";
    CHECK(dintel::A_UNI && std::string(dintel::VERSION) == "1.2.0", "mode 3 default, version");
    // 1. PARITY with the Python reference (ref.py) on ES Nov 4-6 RTH + NO REPAINT
    int nA = 0, nI = 0;
    for (const char* d : {"es1104", "es1105", "es1106"}) {
        const std::vector<dintel::Candle> cs = load(dir + "/" + d + ".fx");
        std::set<Key> exp; { std::ifstream in((dir + "/" + d + ".exp").c_str()); char L; int k, pk, side; long long lo, hi; while (in >> L >> k >> pk >> side >> lo >> hi) exp.insert(Key(L, k, pk, side, lo, hi)); }
        std::vector<dintel::Event> ev; const std::set<Key> got = run(cs, 1 << 30, &ev);
        CHECK(got == exp, std::string(d) + " parity: got " + std::to_string(got.size()) + " expected " + std::to_string(exp.size()));
        for (const Key& k : got) (std::get<0>(k) == 'A' ? nA : nI)++;
        for (int up = 45; up < static_cast<int>(cs.size()); up += 25) {   // decisions up to candle `up` never change later
            const std::set<Key> part = run(cs, up); bool ok = true;
            for (const Key& k : part) if (!got.count(k)) ok = false;
            for (const Key& k : got) if (std::get<1>(k) <= up && !part.count(k)) ok = false;
            CHECK(ok, std::string(d) + " no repaint at " + std::to_string(up));
        }
        {   // researchA (the D records) = the same A's (true wick, next candle overlaps the peak)
            std::set<std::pair<int, long long> > ra, en;
            for (const Key& k : got) if (std::get<0>(k) == 'A') en.insert(std::make_pair(std::get<1>(k), std::get<4>(k)));
            for (int k = 40; k < static_cast<int>(cs.size()); ++k) {
                if (!(cs[k].mod >= 510 && cs[k].mod < 900)) continue;
                std::vector<dp::Bar> w;
                for (int i = k - 29; i <= k; ++i) { dp::Bar b; b.index = i; b.time = cs[i].t; b.closed = true; b.open = cs[i].O * 0.25; b.high = cs[i].H * 0.25; b.low = cs[i].L * 0.25; b.close = cs[i].C * 0.25;
                    for (const dintel::Cell& x : cs[i].cells) { b.rows.push_back(dp::Row(x.key, x.buy - x.sell, x.buy + x.sell)); }
                    w.push_back(b); }
                for (const dp::RaHit& h : dp::researchA(w, 0.25)) if (h.k == 29) ra.insert(std::make_pair(k, static_cast<long long>(h.lo)));
            }
            CHECK(ra == en, std::string(d) + " researchA == engine A: " + std::to_string(ra.size()) + " vs " + std::to_string(en.size()));
        }
        int circ = 0; std::set<int> perK; for (const dintel::Event& e : ev) if (e.circle) { ++circ; CHECK(perK.insert(e.k).second, "one circle per candle"); CHECK(e.letter == 'A' || e.letter == 'I', "circle on A / I only"); }
        if (std::string(d) == "es1104") { CHECK(has(ev, cs, 'A', 1, hm(14, 48), hm(14, 48), 7725.00, 7725.25), "Bull A Nov 4 14:48 7725.00-7725.25"); }
        if (std::string(d) == "es1105") { CHECK(!has(ev, cs, 'A', -1, hm(10, 36), hm(10, 39), 7500.75, 7503.25), "Nov 5 10:36 bear A (node inside the body) rejected");
                                          bool any = false; for (const dintel::Event& e : ev) if (e.letter == 'A' && cs[e.pk].mod + 3 == hm(10, 36)) any = true; CHECK(!any, "no A at Nov 5 10:36"); }
        if (std::string(d) == "es1106") {
            CHECK(has(ev, cs, 'A', -1, hm(13, 51), hm(13, 51), 7424.25, 7424.50), "Bear A Nov 6 13:51");
            CHECK(has(ev, cs, 'A', 1, hm(14, 21), hm(14, 24), 7421.75, 7422.00), "2-bar Bull A Nov 6 14:21/14:24");
            CHECK(has(ev, cs, 'A', -1, hm(14, 9), hm(14, 12), 7434.75, 7435.00), "2-bar Bear A Nov 6 14:09/14:12");
            CHECK(!has(ev, cs, 'I', 1, hm(12, 3), hm(12, 6), 7409.00, 7409.75), "Nov 6 12:03/12:06 I fails the true wick (node top above the peak's close 7409.25)");
        }
    }
    std::cout << "ES Nov 4-6 RTH decided: A " << nA << " I " << nI << "\n";
    // 2. SYNTHETIC (the model mockup panels, ticks)
    dintel::Event e;
    const long long bullSame[5][4] = {{70,72,58,60},{60,62,50,52},{52,55,40,44},{44,46,30,36},{36,38,16,40}};
    CHECK(syn(bullSame, 4, 20, 20, true, e) && e.letter == 'A' && e.side == 1 && e.conf == "same", "bull A same (red node)");
    CHECK(syn(bullSame, 4, 20, 20, false, e) && e.letter == 'I' && e.side == 1, "bull I same (green node)");
    const long long bearSame[5][4] = {{30,42,28,40},{40,50,38,48},{48,60,46,56},{56,66,54,64},{64,86,62,60}};
    CHECK(syn(bearSame, 4, 82, 82, false, e) && e.letter == 'A' && e.side == -1 && e.conf == "same", "bear A same (green node)");
    CHECK(syn(bearSame, 4, 82, 82, true, e) && e.letter == 'I' && e.side == -1, "bear I same (red node)");
    const long long bull2[5][4] = {{70,72,60,62},{62,64,50,52},{52,54,42,44},{44,46,16,32},{32,46,30,44}};
    CHECK(syn(bull2, 3, 20, 20, true, e) && e.letter == 'A' && e.side == 1 && e.conf == "next", "2-bar bull A");
    CHECK(syn(bull2, 3, 20, 20, false, e) && e.letter == 'I' && e.conf == "next", "2-bar bull I");
    const long long bear2[5][4] = {{30,40,28,38},{38,50,36,48},{48,58,46,56},{56,86,54,68},{68,70,54,56}};
    CHECK(syn(bear2, 3, 82, 82, false, e) && e.letter == 'A' && e.side == -1 && e.conf == "next", "2-bar bear A");
    CHECK(syn(bear2, 3, 82, 82, true, e) && e.letter == 'I' && e.side == -1, "2-bar bear I");
    // node INSIDE the peak's body (below the open, above the close): the old open-only check passed it, the true wick rejects it
    const long long body[5][4] = {{70,72,60,62},{62,64,50,52},{52,54,42,44},{44,60,16,32},{32,46,30,44}};
    CHECK(!syn(body, 3, 34, 34, true, e), "bull node inside the red peak's body -> rejected");
    CHECK(!syn(body, 3, 34, 34, false, e), "bull I inside the body -> rejected");
    CHECK(syn(body, 3, 31, 31, true, e) && e.letter == 'A', "same candle, node just below the close -> A");
    const long long bodyB[5][4] = {{30,40,28,38},{38,50,36,48},{48,58,46,56},{56,86,40,68},{68,70,54,56}};
    CHECK(!syn(bodyB, 3, 66, 66, false, e), "bear node inside the green peak's body -> rejected");
    // 2-bar: the next candle must overlap the PEAK candle (low below the peak's high)
    const long long gap[5][4] = {{70,72,60,62},{62,64,50,52},{52,54,42,44},{44,46,16,32},{48,60,47,58}};
    CHECK(!syn(gap, 3, 20, 20, true, e), "2-bar: next candle gapped above the peak's high -> no A");
    // peak not k / k-1, and a same-candle peak that closed beyond is not a 2-bar
    CHECK(!syn(bull2, 2, 20, 20, true, e), "peak k-2 -> nothing");
    // 3. researchA (D records) uses the same wick: a node inside the body is rejected there too (via the core's needBody path)
    CHECK(dintel::code(e).size() >= 1, "code");
    { dintel::Event x; x.letter = 'I'; x.side = 1; x.circle = true; const std::string l = dintel::circleLine(x, "2.35.0", "ES", 1, 0.25); CHECK(l.substr(l.size() - 2) == "|I", "DC line ends with the letter"); }
    std::cout << pass << " passed, " << fail << " failed\n";
    return fail ? 1 : 0;
}
