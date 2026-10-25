// lsDealerProfile 2.7.0 native key levels (KeyLevelsLogic.h) - checked against the lra.key_levels definitions
#include "KeyLevelsLogic.h"
#include "NearLevels.h"
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) passes++; else { fails++; printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
using namespace klv;
static std::vector<Bar> day(long sessDays, double base, double rthHi, double rthLo, double onHi, double onLo, int upToEndMin = 16 * 60)
{
    // 3-min bars for one ES session: 17:00 (previous day) .. upToEndMin of sessDays; ON range onLo..onHi, RTH range rthLo..rthHi
    std::vector<Bar> v;
    for (int e = 17 * 60 + 3; e <= 24 * 60; e += 3) v.push_back(Bar{sessDays - 1, e == 24 * 60 ? 0 : e * 60, (float)base, (float)base, (float)base});
    if (v.back().endSec == 0) v.back().days = sessDays;
    for (int e = 3; e <= upToEndMin; e += 3) {
        double h = base, l = base;
        if (e <= 510) { if (e == 300) h = onHi; if (e == 360) l = onLo; }
        else if (e <= 900) { if (e == 600) h = rthHi; if (e == 702) l = rthLo; }
        v.push_back(Bar{sessDays, e * 60, (float)h, (float)l, (float)base});
    }
    return v;
}
static double get(const std::vector<Level>& L, const char* c) { for (auto& x : L) if (x.code == c) return x.price; return -1.0; }
static bool has(const std::vector<Level>& L, const char* c) { return get(L, c) != -1.0; }
// a session with a chosen high / low bar at given END minutes (CT minutes of day; >= 17:00 = the evening before)
struct Mark { int endMin; double h, l; };
static std::vector<Bar> sess(long sessDays, double base, std::vector<Mark> mk, int upToEndMin = 16 * 60)
{
    std::vector<Bar> v;
    auto at = [&](int e, double& h, double& l) { h = base; l = base; for (auto& m : mk) if (m.endMin == e) { h = m.h; l = m.l; } };
    for (int e = 17 * 60 + 3; e <= 24 * 60; e += 3) { double h, l; at(e, h, l); v.push_back(Bar{e == 24 * 60 ? sessDays : sessDays - 1, e == 24 * 60 ? 0 : e * 60, (float)h, (float)l, (float)base}); }
    for (int e = 3; e <= upToEndMin; e += 3) { double h, l; at(e, h, l); v.push_back(Bar{sessDays, e * 60, (float)h, (float)l, (float)base}); }
    return v;
}
static void extra()
{
    // ---- Asia window: bar END in (18:00, 02:00]
    long thu = svl::daysFromCivil(2026, 10, 8), wed = thu - 1;
    std::vector<Bar> B = sess(wed, 100, {});
    // 17:30 high 130 (NOT Asia: the reopen hour), 18:03 high 106 (Asia), 01:59-> bar ending 02:00 low 93 (Asia), 02:03 low 90 (London)
    auto T = sess(thu, 100, {{17 * 60 + 30, 130, 100}, {18 * 60 + 3, 106, 100}, {1 * 60, 100, 93}, {2 * 60 + 3, 100, 90}, {24 * 60, 104, 100}}, 10 * 60);
    B.insert(B.end(), T.begin(), T.end());
    auto L = compute(B, 180, 510, 900, 0.25);
    CHECK(get(L, "AsiaHI") == 106, "AsiaHI: 18:03 bar in, 17:30 reopen bar out");
    CHECK(get(L, "AsiaLO") == 93, "AsiaLO: the bar ending 01:00 is in (his session 18:00-01:00), 02:03 (London) is out");
    CHECK(get(L, "ONH") == 130 && get(L, "LonLO") == 90, "ON / London unchanged");
    // shown only after 01:00
    {
        auto cut = [&](int endMin) { std::vector<Bar> v; for (auto& b : B) { if (b.days < thu || (b.days == thu && b.endSec <= endMin * 60) || (b.days == thu - 1)) v.push_back(b); } return v; };
        CHECK(!has(compute(cut(57), 180, 510, 900, 0.25), "AsiaHI"), "00:57: Asia still forming - not drawn");
        CHECK(!has(compute(cut(60), 180, 510, 900, 0.25), "AsiaHI"), "01:00: last Asia bar just closed - drawn from the next bar");
        CHECK(get(compute(cut(63), 180, 510, 900, 0.25), "AsiaHI") == 106, "01:03: drawn");
        // no repaint: the same Asia prices at every later bar of the session
        bool same = true;
        for (int e = 63; e <= 600; e += 3) { auto x = compute(cut(e), 180, 510, 900, 0.25); if (get(x, "AsiaHI") != 106 || get(x, "AsiaLO") != 93) same = false; }
        CHECK(same, "no repaint: AsiaHI / AsiaLO fixed from 01:03 to 10:00");
    }
    // partly loaded Asia window: history starts 19:00 -> no Asia level
    {
        std::vector<Bar> P; for (auto& b : T) if (!(b.days == thu - 1 && b.endSec <= 19 * 3600)) P.push_back(b);
        CHECK(!has(compute(P, 180, 510, 900, 0.25), "AsiaHI"), "history from 19:00: Asia partial - not drawn");
        std::vector<Bar> Q; for (auto& b : T) if (!(b.days == thu - 1 && b.endSec <= 17 * 3600 + 45 * 60)) Q.push_back(b);
        CHECK(has(compute(Q, 180, 510, 900, 0.25), "AsiaHI"), "history from 17:45: the Asia window is whole - drawn");
    }
    int s = 0, e = 0; asiaWindow("GC", s, e); CHECK(s == 18 * 60 && e == 1 * 60, "asia window table: his IRT session 18:00-01:00");
    // ---- merge: new levels within N ticks join, else their own line; existing ones only on the tick
    {
        std::vector<Level> X = {{"ONH", 100.00, false}, {"PDH", 100.50, false}, {"AsiaHI", 100.75, false}, {"CWH", 102.00, false}, {"AsiaLO", 99.00, false}, {"ONL", 98.75, false}, {"PFH", 100.75, false}};
        auto M4 = merge(X, 0.25, 4);
        auto nm = [&](double p) { for (auto& g : M4) if (std::fabs(g.price - p) < 1e-9) return g.name; return std::string("-"); };
        CHECK(nm(100.75) == "PFH/AsiaHI", "AsiaHI on PFH's tick joins it");
        CHECK(nm(100.00) == "ONH" && nm(100.50) == "PDH", "existing levels 2 ticks apart stay separate (no change)");
        CHECK(nm(98.75) == "ONL/AsiaLO", "AsiaLO 1 tick from ONL joins it, at ONL's price");
        CHECK(nm(102.00) == "CWH", "CWH 5 ticks from PFH (N=4): own line");
        CHECK(merge({{"PFH", 100.00, false}, {"CWH", 101.25, false}}, 0.25, 4).size() == 2, "N+1 ticks: separate");
        CHECK(merge({{"PFH", 100.00, false}, {"CWH", 101.00, false}}, 0.25, 4)[0].name == "PFH/CWH", "exactly N ticks: merged");
        CHECK(merge({{"PDH", 100.00, false}, {"ONH", 100.75, false}, {"AsiaHI", 100.50, false}}, 0.25, 4)[1].name == "ONH/AsiaHI", "nearest line wins");
        for (const char* m : {"ES", "NQ", "CL", "GC", "HG", "NG", "EU"}) CHECK(mergeTicksFor(m) == 0, "(2026-10-24) new levels merge only on the exact tick");
        CHECK(displayName("ONH/AsiaHI") == std::string("ONH \xB7 AsiaHI") && displayName("PDH") == "PDH", "badge name: middle dot");
    }
    // ---- current week: Mon 2026-10-05 .. Fri 2026-10-09, each weekday x case
    long mon = svl::daysFromCivil(2026, 10, 5);
    // build Mon..(day) sessions; highs[i] / lows[i] per session (RTH 10:00 bar), the last session cut at 10:00
    auto week = [&](std::vector<double> hi, std::vector<double> lo) {
        std::vector<Bar> W;
        auto pre = sess(mon - 3, 100, {});                       // the Friday before (prior week)
        W.insert(W.end(), pre.begin(), pre.end());
        for (size_t i = 0; i < hi.size(); i++) { auto v = sess(mon + (long)i, 100, {{600, hi[i], lo[i]}}, i + 1 == hi.size() ? 10 * 60 : 16 * 60); W.insert(W.end(), v.begin(), v.end()); }
        return compute(W, 180, 510, 900, 0.25);
    };
    struct Case { const char* why; std::vector<double> hi, lo; double cwh, cwl; };
    std::vector<Case> C = {
        {"Mon: never",                               {120},                     {80},                     -1, -1},
        {"Tue: never (Monday = PDH/PFH)",            {120, 101},                {80, 99},                 -1, -1},
        {"Wed: Mon high above Tue's -> CWH",         {120, 110, 101},           {80, 90, 99},             120, 80},
        {"Wed: Tue holds the week high -> none",     {110, 120, 101},           {90, 80, 99},             -1, -1},
        {"Wed: tie with PFH -> none",                {120, 120, 101},           {80, 80, 99},             -1, -1},
        {"Thu: Mon high, Tue/Wed lower -> CWH",      {120, 110, 112, 101},      {95, 80, 85, 99},         120, 80},
        {"Thu: Wed (yesterday) holds both -> none",  {110, 112, 120, 101},      {90, 95, 80, 99},         -1, -1},
        {"Thu: today makes a new high - CWH stays",  {120, 110, 112, 130},      {95, 80, 85, 99},         120, 80},
        {"Fri: Tue high, Mon low",                   {110, 125, 112, 111, 101}, {80, 90, 85, 95, 99},     125, 80},
        {"Fri: Thu holds both -> none",              {110, 112, 111, 125, 101}, {90, 95, 85, 80, 99},     -1, -1},
    };
    for (auto& c : C) {
        auto x = week(c.hi, c.lo);
        CHECK(get(x, "CWH") == c.cwh && get(x, "CWL") == c.cwl, c.why);
    }
    // CW never changes inside the session (built only from completed sessions)
    {
        bool same = true;
        for (int cut = 3; cut <= 16 * 60; cut += 3) {
            std::vector<Bar> W;
            auto pre = sess(mon - 3, 100, {}); W.insert(W.end(), pre.begin(), pre.end());
            std::vector<double> hi = {120, 110, 112, 140}, lo = {95, 80, 85, 60};
            for (size_t i = 0; i < 4; i++) { auto v = sess(mon + (long)i, 100, {{600, hi[i], lo[i]}}, i == 3 ? cut : 16 * 60); W.insert(W.end(), v.begin(), v.end()); }
            auto x = compute(W, 180, 510, 900, 0.25);
            if (get(x, "CWH") != 120 || get(x, "CWL") != 80) same = false;
        }
        CHECK(same, "CWH / CWL fixed all Thursday session, even when today trades through them");
    }
    // a week that is only partly loaded (history starts Tuesday): no CW
    {
        std::vector<Bar> W;
        std::vector<double> hi = {120, 110, 112}, lo = {95, 90, 85};
        for (size_t i = 1; i < 3; i++) { auto v = sess(mon + (long)i, 100, {{600, hi[i], lo[i]}}, i == 2 ? 600 : 16 * 60); W.insert(W.end(), v.begin(), v.end()); }
        auto x = compute(W, 180, 510, 900, 0.25);
        CHECK(!has(x, "CWH") && !has(x, "CWL"), "Monday not loaded: no CW (it could be wrong)");
    }
    // a short week: Monday holiday (no session) - Tue, Wed, Thu
    {
        std::vector<Bar> W;
        auto pre = sess(mon - 3, 100, {}); W.insert(W.end(), pre.begin(), pre.end());
        std::vector<double> hi = {120, 110, 101}, lo = {80, 90, 99};
        for (size_t i = 0; i < 3; i++) { auto v = sess(mon + 1 + (long)i, 100, {{600, hi[i], lo[i]}}, i == 2 ? 600 : 16 * 60); W.insert(W.end(), v.begin(), v.end()); }
        auto x = compute(W, 180, 510, 900, 0.25);
        CHECK(get(x, "CWH") == 120 && get(x, "CWL") == 80, "Monday holiday: Thu shows Tuesday's high / low");
    }
}
int main()
{
    long thu = svl::daysFromCivil(2026, 10, 8), wed = thu - 1;
    std::vector<Bar> B = day(wed, 100, 110, 90, 105, 95);
    std::vector<Bar> T = day(thu, 100, 120, 80, 104, 97, 10 * 60);
    B.insert(B.end(), T.begin(), T.end());
    auto L = compute(B, 180, 510, 900, 0.25);
    auto find = [&](const char* c) { for (auto& x : L) if (x.code == c) return x.price; return -1.0; };
    CHECK(find("PFH") == 110 && find("PFL") == 90, "prior full session high / low");
    CHECK(find("PDH") == 110 && find("PDL") == 90, "prior RTH high / low");
    CHECK(find("ONH") == 104 && find("ONL") == 97, "overnight high / low once RTH started");
    CHECK(find("LonHI") == 104 && find("LonLO") == 97, "London window 02:00-07:30 (the 06:00 low 97 is in it: LonLO = 97)" ) ;
    std::vector<Bar> N(B.begin(), B.end() - (10 * 60 - 7 * 60) / 3);   // cut at 07:00: before London ends, before RTH
    auto L2 = compute(N, 180, 510, 900, 0.25);
    bool on = false, lon = false; for (auto& x : L2) { if (x.code == "ONH") on = true; if (x.code == "LonHI") lon = true; }
    CHECK(!on && !lon, "no ONH before the RTH open, no LonHI before 07:30");
    CHECK(compute({}, 180, 510, 900, 0.25).empty(), "no bars: no levels");
    auto M = merge({{"PDH", 110, false}, {"PFH", 110, false}, {"PDL", 90, false}}, 0.25);
    CHECK(M.size() == 2 && M[1].name == "PDH/PFH" && !M[1].pivotOnly, "same tick merged");
    for (auto& x : L) CHECK(x.code.compare(0, 2, "Hr") != 0, "(2.10.0) no HrHI / HrLO any more");
    // (2.7.4) a chart whose history starts part-way through a session: no level from the partial session / week
    {
        std::vector<Bar> P(B.begin() + 200, B.end());            // starts Wed ~03:00: Wed's session is only partly loaded
        auto L3 = compute(P, 180, 510, 900, 0.25);
        bool pf = false, pd = false; for (auto& x : L3) { if (x.code == "PFH" || x.code == "PFL") pf = true; if (x.code == "PDH") pd = true; }
        CHECK(!pf && !pd, "(2.7.4) prior session only partly loaded: no PFH / PFL / PDH (they would be wrong)");
        CHECK(find("PFH") == 110, "(2.7.4) loaded from the 17:00 open: PFH still drawn");
        // a full previous week (Mon 5 .. Fri 9 Oct sessions) and then Mon 12 Oct: PWH / PWL; cut its Monday: none
        std::vector<Bar> W;
        long mon = svl::daysFromCivil(2026, 10, 5);
        for (int d = 0; d < 5; d++) { auto v = day(mon + d, 100, 110 + d, 90 - d, 105, 95); W.insert(W.end(), v.begin(), v.end()); }
        auto v = day(mon + 7, 100, 101, 99, 105, 95, 10 * 60); W.insert(W.end(), v.begin(), v.end());
        auto LW = compute(W, 180, 510, 900, 0.25);
        double pwh = -1; for (auto& x : LW) if (x.code == "PWH") pwh = x.price;
        CHECK(pwh == 114, "(2.7.4) whole previous week loaded: PWH = the week's high");
        std::vector<Bar> W2(W.begin() + 100, W.end());           // the week's Monday session only partly loaded
        bool pw = false; for (auto& x : compute(W2, 180, 510, 900, 0.25)) if (x.code == "PWH") pw = true;
        CHECK(!pw, "(2.7.4) previous week only partly loaded: no PWH");
        std::vector<Bar> O(T.begin() + 150, T.end());            // today's session from ~00:30: ON partial, London (02:00-07:30) whole
        bool onh = false, lon = false; for (auto& x : compute(O, 180, 510, 900, 0.25)) { if (x.code == "ONH") onh = true; if (x.code == "LonHI") lon = true; }
        CHECK(!onh && lon, "(2.7.4) loaded from 00:30: no ONH (partial overnight), LonHI kept (London fully loaded)");
    }
    extra();
    printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
