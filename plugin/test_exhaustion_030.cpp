// test_exhaustion_030.cpp -- lsExhaustion 0.3.0 (pivots + histogram) against the mock host.
// g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Imock -I. test_exhaustion_030.cpp -o t30 && ./t30 [wavepoc_es1028_bars.csv]
#include "Exhaustion.cpp"
#include <cstdio>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static mock::Bar bar(unsigned long t, float h, float l, long buy, long sell) { mock::Bar b; b.time = t; b.high = h; b.low = l; b.open = l; b.close = h; b.rows.push_back(mock::Price(h, buy, sell, buy + sell)); return b; }
// an independent reading of IRT's rule (Linnsoft ZigZag, Min Price Change N bars, Use High/Low) to cross-check exp3::pivots
static std::vector<std::pair<int,int>> refPivots(const std::vector<double>& h, const std::vector<double>& l, int N) {
    std::vector<std::pair<int,int>> o; int dir = 0, ext = 0, hi0 = 0, lo0 = 0;
    for (int k = 0; k < (int)h.size(); ++k) {
        if (k >= N) {
            const double ph = *std::max_element(h.begin() + (k - N), h.begin() + k), pl = *std::min_element(l.begin() + (k - N), l.begin() + k);
            const bool up = h[k] > ph, dn = l[k] < pl;
            if (dir == 0) { if (up) { o.push_back({lo0, -1}); dir = 1; ext = k; } else if (dn) { o.push_back({hi0, 1}); dir = -1; ext = k; } }
            else if (dir > 0) { if (h[k] > h[ext]) ext = k; else if (dn) { o.push_back({ext, 1}); dir = -1; ext = k; } }
            else { if (l[k] < l[ext]) ext = k; else if (up) { o.push_back({ext, -1}); dir = 1; ext = k; } }
        }
        if (dir == 0) { if (h[k] > h[hi0]) hi0 = k; if (l[k] < l[lo0]) lo0 = k; }
    }
    return o;
}
int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    // 1) the pivot rule on real bars: ES 10-28 (his ChartView export) - same pivots as the reference, alternate, confirmed later
    {
        const char* fx = argc > 1 ? argv[1] : "wavepoc_es1028_bars.csv";
        std::ifstream f(fx); std::string line; std::vector<double> h, l; std::vector<long long> t;
        while (std::getline(f, line)) { if (line.size() < 2 || line[0] != 'B') continue; std::vector<std::string> c = split(line, ','); if (c.size() < 7) continue; t.push_back(std::atoll(c[2].c_str())); h.push_back(std::atof(c[4].c_str())); l.push_back(std::atof(c[5].c_str())); }
        CHECK(h.size() > 200, "fixture loaded");
        std::vector<double> b(h.size(), 100), s(h.size(), 100);
        const std::vector<exp3::PivotEx> pv = exp3::pivots(h, l, b, s, 2);
        const std::vector<std::pair<int,int>> rf = refPivots(h, l, 3);
        bool same = pv.size() == rf.size(); for (size_t i = 0; same && i < pv.size(); ++i) same = pv[i].i == rf[i].first && pv[i].type == rf[i].second;
        CHECK(same && pv.size() > 10, "pivots = IRT zigzag rule (N 3, high/low), independent reference");
        bool alt = true, later = true, rev = true;
        for (size_t i = 0; i < pv.size(); ++i) {
            if (i && pv[i].type == pv[i - 1].type) alt = false;
            if (pv[i].conf <= pv[i].i) later = false;
            const int k = pv[i].conf; double ph = h[k - 3], pl = l[k - 3]; for (int j = k - 3; j < k; ++j) { ph = std::max(ph, h[j]); pl = std::min(pl, l[j]); }
            if (pv[i].type > 0 ? !(l[k] < pl) : !(h[k] > ph)) rev = false;
        }
        CHECK(alt && later && rev, "highs / lows alternate; each confirmed on a LATER bar that breaks the previous 3 bars' opposite extreme");
        // the IRT pivots lsWavePOC 1.3.2 was matched to on this day (NOTES-WP130: 10:24 / 12:30 H, 13:18 L)
        auto at = [&](int hh, int mm, int type) { for (size_t i = 0; i < pv.size(); ++i) { const long long s2 = t[pv[i].i] % 86400; if (s2 == hh * 3600 + mm * 60 && pv[i].type == type) return true; } return false; };
        CHECK(at(13, 18, -1), "13:18 pivot low (IRT's)");
        int nH = 0; for (auto& p : pv) nH += p.type > 0; std::printf("  fixture: %zu pivots (%d highs)\n", pv.size(), nH);
        // incremental == batch on the prefix: a pivot confirmed by bar k is the same whatever comes after (no repaint)
        bool stable = true;
        for (size_t n = 50; n < h.size() && stable; n += 25) {
            std::vector<double> h2(h.begin(), h.begin() + n), l2(l.begin(), l.begin() + n), b2(b.begin(), b.begin() + n), s2(s.begin(), s.begin() + n);
            const std::vector<exp3::PivotEx> p2 = exp3::pivots(h2, l2, b2, s2, 2);
            for (size_t i = 0; i < p2.size(); ++i) if (i >= pv.size() || p2[i].i != pv[i].i || p2[i].conf != pv[i].conf || std::fabs(p2[i].ratio - pv[i].ratio) > 1e-12) stable = false;
        }
        CHECK(stable, "every prefix gives the same confirmed pivots and ratios (never repainted by later bars)");
        std::vector<double> bad = h; bad[5] = NAN; CHECK(exp3::pivots(bad, l, b, s, 2).empty(), "a non-finite bar = no pivots (all-or-nothing)");
    }
    // 2) the host: ratios, only >= 2.0x drawn, histogram boxes (no connecting lines), records, restart, no repaint
    char tmpl[] = "/tmp/ex3XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host H; mock::current = &H; H.tick = 0.25f; H.seconds = 180; H.symbol = "ESZ6"; H.root = "ES";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL);
    unsigned long t = day + 8 * 3600 + 30 * 60 + 180;
    auto add = [&](float hh, float ll, long bu, long se) { H.bars.push_back(bar(t, hh, ll, bu, se)); t += 180; };
    for (int i = 0; i < 6; ++i) add(7000.0f - 0.25f * i, 6999.0f - 0.25f * i, 500, 500);           // 0-5 drift down
    const long upB[8] = {2000, 2000, 1800, 1500, 1000, 700, 500, 400};                              // 6-13 up leg, buyers fade (top 13)
    for (int i = 0; i < 8; ++i) add(7001.0f + i, 7000.0f + i, upB[i], 300);
    const long dnS[6] = {900, 1000, 1000, 950, 900, 850};                                           // 14-19 down leg, sellers steady (low 19)
    for (int i = 0; i < 6; ++i) add(7006.0f - 1.5f * i, 7005.0f - 1.5f * i, 300, dnS[i]);
    for (int i = 0; i < 4; ++i) add(7000.0f + 2.0f * i, 6998.0f + 2.0f * i, 900, 300);              // 20-23 up again: confirms the low
    add(7007.0f, 7006.0f, 10, 10);                                                                   // forming bar
    H.now = t;
    Exhaustion* p = static_cast<Exhaustion*>(CreateExtension()); p->calc(0);
    EXState* S = p->testState();
    const long long tTop = (long long)H.bars[13].time, tLow = (long long)H.bars[19].time;
    CHECK(S && S->piv.count(tTop) && S->piv[tTop].first == 1, "pivot high on the top bar (13), confirmed by the down bar 14");
    // buyers pace S 2 in the leg (from the low at 5): peak 2000 / pace at 13 = (500+400)/2 = 450 -> 4.44x
    CHECK(S->piv.count(tTop) && std::fabs(S->piv[tTop].second - 4.44) < 1e-9, "buyers fading at the high = 4.44x");
    CHECK(S->piv.count(tLow) && S->piv[tLow].first == -1 && S->piv[tLow].second < 2.0, "pivot low at 19: sellers did not fade (< 2x) - logged");
    for (auto& kv : S->piv) std::printf("  pivot %lld type %d ratio %.2f\n", kv.first, kv.second.first, kv.second.second);
    H.draws.clear(); p->draw();
    int boxes = 0, lines = 0; bool lab = false, head = false, low = false, e = false;
    for (auto& d : H.draws) {
        if (d.type == "box") { ++boxes; }
        if (d.type == "line") { ++lines; }
        if (d.type == "text" && d.text == "4.4x" && d.color == C_SELL) lab = true;
        if (d.type == "text" && d.text == "EXHAUSTION at pivots   ") head = true;
        if (d.type == "text" && d.text == "E?") e = true;
        char b2[16]; std::snprintf(b2, sizeof(b2), "%.1fx", S->piv.count(tLow) ? S->piv[tLow].second : 0.0); if (d.type == "text" && d.text == b2 && d.color == C_BUY && d.y > 20) low = true;
    }
    long q2 = 0; for (auto& kv : S->piv) q2 += kv.second.second + 1e-9 >= 2.0;
    CHECK(boxes == q2 && boxes >= 1, "one histogram box per pivot at / over 2.0x - nothing for the < 2x low");
    CHECK(lines == 1, "no connecting lines: the only line is the 2.0x level");
    CHECK(lab && head && !e, "4.4x on top of the red bar; header 'EXHAUSTION at pivots'; no E? on the chart");
    (void)low;
    const std::string rec = readAll(flex + "\\Exhaustion\\ES-ESZ6-180-pivots-" + std::to_string(S->session) + ".csv");
    CHECK(rec.find(std::to_string(tTop) + "|top|4.44|") != std::string::npos && rec.find("shown") != std::string::npos && rec.find("|bottom|") != std::string::npos && rec.find("logged") != std::string::npos,
          "every pivot recorded: time, side, ratio, conf / leg times, shown|logged");
    const std::string st = readAll(flex + "\\Exhaustion.status-ES-ESZ6-180.txt");
    CHECK(st.find("VERSION,0.3.0") != std::string::npos && st.find("PIVOTS,zigzag N 3") != std::string::npos, "status: version 0.3.0 + PIVOTS line");
    // no repaint: more bars (a later higher high) do not change the recorded pivot
    const std::map<long long, std::pair<int, double>> before = S->piv;
    H.bars.pop_back(); for (int i = 0; i < 5; ++i) add(7010.0f + i, 7009.0f + i, 50, 50); add(7012.0f, 7011.0f, 10, 10); H.now = t;
    p->calc(0);
    bool kept = true; for (auto& kv : before) { auto it = S->piv.find(kv.first); if (it == S->piv.end() || it->second != kv.second) kept = false; }
    CHECK(kept, "later bars never change a confirmed pivot or its ratio");
    // restart: the same pivots from the records
    const std::map<long long, std::pair<int, double>> all = S->piv;
    p->destroy(); delete p; H.data = nullptr;
    Exhaustion* r = static_cast<Exhaustion*>(CreateExtension()); r->calc(0);
    CHECK(r->testState()->piv == all, "after a restart the same pivots and ratios");
    r->destroy(); delete r; H.data = nullptr;
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
