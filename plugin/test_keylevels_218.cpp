// lsDealerProfile 2.18.0: Open, DPoc / VAH / VAL (prior full session volume profile), WPoc (prior week) - KeyLevelsLogic.h
#include "KeyLevelsLogic.h"
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) passes++; else { fails++; printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
using namespace klv;
static double get(const std::vector<Level>& L, const char* c) { for (auto& x : L) if (x.code == c) return x.price; return -1.0; }
static bool has(const std::vector<Level>& L, const char* c) { return get(L, c) != -1.0; }
// one ES session of 3-min bars (17:03 prev day .. upTo); every bar trades `vol` lots at price p(e) (one row)
struct Gen { std::vector<Bar> B; std::vector<std::vector<Vap>> V; };
static void addSess(Gen& g, long sd, double (*p)(int, long), int upTo = 16 * 60, double vol = 10, bool vapOk = true)
{
    auto put = [&](long d, int e) {
        const int key = (d == sd - 1) ? e : e + 24 * 60;            // minutes since 00:00 of the day before
        const double px = p(key, sd);
        Bar b{ d, (e % (24 * 60)) * 60, (float)px, (float)px, (float)px }; b.o = (float)px; b.v = vol;
        g.B.push_back(b); g.V.push_back(vapOk ? std::vector<Vap>{ Vap{ px, vol } } : std::vector<Vap>{});
    };
    for (int e = 17 * 60 + 3; e < 24 * 60; e += 3) put(sd - 1, e);
    put(sd, 0); g.B.back().days = sd;
    for (int e = 3; e <= upTo; e += 3) put(sd, e);
}
// price path: 100 most of the session, 105 for a stretch, the RTH open bar at 101
static double path(int k, long) { const int m = k - 24 * 60; if (m == 510 + 3) return 101; if (m > 600 && m <= 660) return 105; if (m > 700 && m <= 900) return 98 + (m % 4) * 0.25; return 100; }
int main()
{
    // ---- profileOf
    {
        std::map<long long, double> t{ {400, 10}, {401, 50}, {402, 20}, {403, 5}, {404, 15} };   // ticks of 0.25: 100.00 .. 101.00
        Prof P = profileOf(t, 0.25);
        CHECK(P.ok && P.poc == 100.25, "POC = the biggest row");
        // 70% of 100 = 70: POC 50; up (402,403)=25 vs down (400)=10 -> up: 75 >= 70 -> VA 100.25 .. 100.75
        CHECK(P.vah == 100.75 && P.val == 100.25, "value area: the bigger two-row side first (CBOT)");
        Prof P2 = profileOf(t, 0.25, 2);      // rows 400-401 = 60, 402-403 = 25, 404 = 15
        CHECK(P2.poc == 100.0, "rowTicks 2: rows grouped, row price = its lowest tick");
        std::map<long long, double> tie{ {400, 10}, {402, 10}, {404, 10}, {406, 1} };
        CHECK(profileOf(tie, 0.25).poc == 100.5, "tie: the row nearest the middle");
        CHECK(!profileOf({}, 0.25).ok, "empty: no profile");
        std::map<long long, double> one{ {400, 7} };
        Prof P3 = profileOf(one, 0.25); CHECK(P3.ok && P3.vah == 100 && P3.val == 100, "one row: VA = the row");
        std::map<long long, double> eq{ {398, 5}, {399, 5}, {400, 30}, {401, 5}, {402, 5} };   // 70% of 50 = 35: equal sides -> both
        Prof P4 = profileOf(eq, 0.25); CHECK(P4.vah == 100.5 && P4.val == 99.5, "equal sides: both added");
    }
    // ---- compute with the volume profile
    long tue = svl::daysFromCivil(2026, 10, 13), wed = tue + 1;
    Gen g; addSess(g, tue, path); addSess(g, wed, path, 10 * 60);
    ProfDiag dg;
    auto L = compute(g.B, 180, 510, 900, 0.25, 18 * 60, 2 * 60, &g.V, 1, &dg);
    CHECK(get(L, "DPoc") == 100, "DPoc = the prior full session's biggest volume row");
    CHECK(has(L, "VAH") && has(L, "VAL") && get(L, "VAL") <= 100 && get(L, "VAH") >= 100, "VAH / VAL around the POC");
    CHECK(dg.dayUsed && dg.dayCover > 0.99 && dg.dpoc1 == 100, "diag: coverage and the POC by row size");
    CHECK(get(L, "Open") == 101, "Open = the first RTH bar's open");
    CHECK(get(L, "PFH") == 105, "existing levels unchanged");
    // overnight (before 08:30): no Open yet; DPoc already drawn (prior session complete)
    {
        Gen h; addSess(h, tue, path); addSess(h, wed, path, 8 * 60);
        auto L2 = compute(h.B, 180, 510, 900, 0.25, 18 * 60, 2 * 60, &h.V);
        CHECK(!has(L2, "Open"), "before the RTH open: no Open");
        CHECK(get(L2, "DPoc") == 100, "overnight: DPoc drawn");
    }
    // no repaint: Open and DPoc fixed through the day
    {
        bool same = true;
        for (int e = 516; e <= 900; e += 30) {
            Gen h; addSess(h, tue, path); addSess(h, wed, path, e);
            auto x = compute(h.B, 180, 510, 900, 0.25, 18 * 60, 2 * 60, &h.V);
            if (get(x, "Open") != 101 || get(x, "DPoc") != 100 || get(x, "VAH") != get(L, "VAH")) same = false;
        }
        CHECK(same, "no repaint: Open / DPoc / VAH fixed from 08:36 to 15:00");
    }
    // a prior-session bar without volume-at-price: no DPoc (coverage), the rest kept
    {
        Gen h; addSess(h, tue, path); addSess(h, wed, path, 10 * 60);
        for (int k = 20; k < 120; ++k) h.V[k].clear();                 // 100 of ~460 bars with no volume-at-price (< 98% covered)
        ProfDiag d2; auto x = compute(h.B, 180, 510, 900, 0.25, 18 * 60, 2 * 60, &h.V, 1, &d2);
        CHECK(!has(x, "DPoc") && !has(x, "VAH") && has(x, "PFH") && has(x, "Open"), "volume-at-price missing for 20% of the bars: no DPoc / VA, others kept");
        CHECK(!d2.dayUsed && d2.dayCover < 0.999, "diag says why");
    }
    // one bar missing out of ~460 (> 98% covered): still drawn
    {
        Gen h; addSess(h, tue, path); addSess(h, wed, path, 10 * 60); h.V[50].clear();
        CHECK(get(compute(h.B, 180, 510, 900, 0.25, 18 * 60, 2 * 60, &h.V), "DPoc") == 100, "one bar missing (99.8% covered): DPoc kept");
    }
    // without vap (Liquidity Profile / Big Bid Ask callers): no profile levels
    {
        auto x = compute(g.B, 180, 510, 900, 0.25);
        CHECK(!has(x, "DPoc") && !has(x, "WPoc") && get(x, "Open") == 101, "no vap: no profile levels (Open still from the bars)");
        std::vector<Bar> nb = g.B; for (auto& b : nb) b.o = NAN;
        CHECK(!has(compute(nb, 180, 510, 900, 0.25), "Open"), "no opens supplied: no Open");
    }
    // ---- WPoc: the prior week Mon..Fri, then this week's Tuesday
    {
        Gen w; long mon = svl::daysFromCivil(2026, 10, 5);
        for (int d = 0; d < 5; ++d) addSess(w, mon + d, path);
        // make one price dominate the week: Thursday trades at 110 all session long with heavy volume
        // (rebuild Thursday with vol 100 at 110)
        Gen w2; for (int d = 0; d < 5; ++d) { if (d == 3) addSess(w2, mon + d, [](int, long) { return 110.0; }, 16 * 60, 100); else addSess(w2, mon + d, path); }
        addSess(w2, mon + 7, path); addSess(w2, mon + 8, path, 10 * 60);
        ProfDiag d3; auto x = compute(w2.B, 180, 510, 900, 0.25, 18 * 60, 2 * 60, &w2.V, 1, &d3);
        CHECK(get(x, "WPoc") == 110, "WPoc = the prior week's biggest volume row");
        CHECK(get(x, "DPoc") == 100, "DPoc = Monday's (the prior session) - not the week");
        CHECK(d3.weekUsed && d3.weekCover > 0.99, "diag: week coverage");
        // the week only partly loaded: no WPoc
        Gen w3; w3.B.assign(w2.B.begin() + 100, w2.B.end()); w3.V.assign(w2.V.begin() + 100, w2.V.end());
        CHECK(!has(compute(w3.B, 180, 510, 900, 0.25, 18 * 60, 2 * 60, &w3.V), "WPoc"), "week partly loaded: no WPoc");
        // fixed all week: same at Tuesday 10:00 and Tuesday 14:00
        Gen w4 = w2; Gen w5; for (int d = 0; d < 5; ++d) { if (d == 3) addSess(w5, mon + d, [](int, long) { return 110.0; }, 16 * 60, 100); else addSess(w5, mon + d, path); }
        addSess(w5, mon + 7, path); addSess(w5, mon + 8, path, 14 * 60);
        CHECK(get(compute(w5.B, 180, 510, 900, 0.25, 18 * 60, 2 * 60, &w5.V), "WPoc") == 110, "WPoc fixed through the week");
        (void)w4;
    }
    // merge: VAL on the PFL tick joins the line ("PFL/VAL"), Open keeps its own line
    {
        auto M = merge({ {"PFL", 98, false}, {"VAL", 98, false}, {"Open", 101, false} }, 0.25, 4);
        CHECK(M.size() == 2 && M[0].name == "PFL/VAL" && M[1].name == "Open", "VAL on the PFL tick joins it; Open alone");
    }
    printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
