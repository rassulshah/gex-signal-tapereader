// t_grade.cpp -- the stop-run STRENGTH GRADE of lsSR 1.1.1 (SRLogic.h), SDK-free.  g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra t_grade.cpp -o tg && ./tg
#include "../SRLogic.h"
#include "../SRGradeParams.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static const long long D0 = 20000LL * 86400, OPEN = 510 * 60;          // 08:30
struct Rig { sr::Engine e; long long t; unsigned long long id = 1;
    Rig(double bigA = 20, double bigP = 20) { e.spb = 180; e.rthOpenSec = OPEN; e.rthCloseSec = 900 * 60; e.P.bigA = bigA; e.P.bigP = bigP; t = (D0 + OPEN) * 1000;
        sr::Level L; L.name = "LonLO"; L.px = 1000; L.side = -1; std::vector<sr::Level> v(1, L); e.setLevels(v); }
    // one candle: a range of rng ticks ending at close c, vol lots traded as small prints above the level
    void candle(long long c, long long rng, long long vol) { const long long t0 = t; e.onFill(t, c + rng, vol / 2, 1, id++); e.onFill(t + 60000, c, vol - vol / 2, -1, id++); e.onFill(t + 120000, c, 1, 1, id++); t = t0 + 180000; }
    void fill(long long px, long long sz, int ag, unsigned long long aggr, unsigned long long rest, long long dtMs = 500) { e.onFill(t, px, sz, ag, aggr, rest); t += dtMs; }
    void endCandle() { t = (t / 180000 + 1) * 180000; }
    std::vector<std::string> G() const { std::vector<std::string> v; for (const std::string& l : e.journal) if (l[0] == 'G') v.push_back(l); return v; }
};
static std::vector<std::string> F(const std::string& l) { return sr::Journal::split(l); }
int main() {
    {   // ---- all four signs: 20 candles of history (range 10 -> ATR ~10, 600 lots each), then a run 12 ticks through the level
        Rig R; for (int i = 0; i < 20; ++i) R.candle(1020, 10, 600);
        CHECK(R.e.atr14() > 9 && R.e.atr14() < 13 && R.e.medVol() == 601, "ATR(14) and the normal candle volume from the closed candles");
        R.fill(1001, 5, -1, 9001, 0);                                   // still above the level
        R.fill(999, 10, -1, 9002, 501); CHECK(R.e.openGrades() == 1, "the crossing opens one grade");
        for (int k = 0; k < 3; ++k) R.fill(995 - k, 30, -1, 9010 + k, 600 + k);          // 3 big resting bids filled by the sellers (30 lots each)
        R.fill(990, 200, -1, 9020, 700); R.fill(988, 400, -1, 9021, 701);                // heavy: > 600 sold below
        R.fill(989, 60, 1, 8000, 800); R.fill(990, 40, 1, 8000, 801); R.fill(991, 300, 1, 8001, 802);   // buyers below: one order of 100 and one of 300 lots
        R.fill(998, 500, 1, 8002, 803); R.fill(1003, 10, 1, 8003, 804);                  // back above the level; this candle closes at 1003
        R.endCandle(); R.e.advance(R.t + 1000);
        const std::vector<std::string> g = R.G(); CHECK(g.size() == 1, "one G line when the candle closes back");
        if (g.size() == 1) { const std::vector<std::string> f = F(g[0]); CHECK(f.size() == 20, "20 fields");
            CHECK(f[4] == "LonLO" && f[5] == "1000" && f[6] == "-1" && f[19] == "1", "level, side, closed back");
            CHECK(f[7] == "4" && f[8] == "1" && f[9] == "1" && f[10] == "1" && f[11] == "1", "grade 4: big buyers, big bids, deep, heavy");
            CHECK(f[13] == "5" && f[16] == "700" && f[17] == "900", "5 big bids filled, 700 sold and 900 bought beyond the level (the print at 1001 and the prints at / above 1000 not counted)");
            CHECK(std::atof(f[12].c_str()) > 0.99 && std::atof(f[14].c_str()) > 0.9 && std::atof(f[14].c_str()) < 1.4 && std::atof(f[15].c_str()) > 1.1, "share, depth in ATRs, heavy in candles");
            CHECK(f[2] == f[3] && f[18] == "1", "closed back on the run candle itself"); }
        CHECK(R.e.openGrades() == 0, "the grade is closed");
        sr::Journal J; for (const std::string& l : R.e.journal) J.add(l); CHECK(J.bad == 0 && J.grade.size() == 1 && J.grade.begin()->second.score == 4 && J.gradeAt(J.grade.begin()->second.barEnd, -1) && !J.gradeAt(J.grade.begin()->second.barEnd, 1), "the journal reader keeps the grade by run candle and side");
    }
    {   // ---- nothing behind it: small prints only, 2 ticks through -> grade 0; it closes back two candles later
        Rig R; for (int i = 0; i < 20; ++i) R.candle(1020, 10, 600);
        R.fill(999, 3, -1, 9002, 501); R.fill(998, 4, -1, 9003, 502); R.fill(998, 2, 1, 8000, 800); R.endCandle(); R.e.advance(R.t + 1000);
        CHECK(R.G().empty() && R.e.openGrades() == 1, "a close beyond the level: not graded yet");
        R.fill(999, 2, 1, 8001, 801); R.endCandle(); R.e.advance(R.t + 1000); CHECK(R.G().empty(), "still beyond");
        R.fill(1000, 2, 1, 8002, 802); R.endCandle(); R.e.advance(R.t + 1000);
        const std::vector<std::string> g = R.G(); CHECK(g.size() == 1, "graded on the candle that closes AT the level");
        if (g.size() == 1) { const std::vector<std::string> f = F(g[0]); CHECK(f[7] == "0" && f[18] == "3" && f[19] == "1" && f[2] != f[3], "grade 0, three candles, closed back later"); }
    }
    {   // ---- a sign is off without its threshold; no history = deep / heavy cannot be known
        Rig R(0, 0); R.candle(1020, 10, 600);
        R.fill(999, 10, -1, 9002, 501); R.fill(980, 900, -1, 9003, 502); R.fill(981, 900, 1, 8000, 800); R.fill(1001, 5, 1, 8001, 801); R.endCandle(); R.e.advance(R.t + 1000);
        const std::vector<std::string> g = R.G(); CHECK(g.size() == 1, "graded");
        if (g.size() == 1) { const std::vector<std::string> f = F(g[0]); CHECK(f[8] == "0" && f[9] == "0" && f[11] == "0", "no thresholds / under 10 candles: big buyers, big bids and heavy stay 0"); CHECK(f[10] == "1", "deep uses the one candle there is"); }
    }
    {   // ---- never closes back: given up after gMaxBars candles, logged with state 2 and never drawn
        Rig R; R.e.P.gMaxBars = 3; for (int i = 0; i < 20; ++i) R.candle(1020, 10, 600);
        R.fill(999, 10, -1, 9002, 501); for (int k = 0; k < 6; ++k) { R.fill(995, 2, -1, 9100 + k, 0); R.endCandle(); R.e.advance(R.t + 1000); }
        const std::vector<std::string> g = R.G(); CHECK(g.size() == 1 && F(g[0])[19] == "2" && F(g[0])[18] == "4", "gave up after 3 candles");
        sr::Journal J; for (const std::string& l : R.e.journal) J.add(l); CHECK(J.grade.empty() && J.bad == 0, "a gave-up grade is not drawn");
    }
    {   // ---- the old lines are untouched: R / B formats, and a garbled G line is counted bad
        Rig R; for (int i = 0; i < 12; ++i) R.candle(1020, 10, 600); R.fill(999, 3, -1, 9002, 501); R.fill(1002, 3, 1, 8000, 800); R.endCandle(); R.e.advance(R.t + 5000);
        int r = 0, b = 0; for (const std::string& l : R.e.journal) { if (l[0] == 'R') { ++r; CHECK(F(l).size() == 15, "R line: 15 fields as 1.0.0"); } if (l[0] == 'B') { ++b; if (F(l).size() != 12) { CHECK(false, "B line: 12 fields as 1.0.0"); } } }
        CHECK(r == 1 && b >= 12, "one run, the candles");
        sr::Journal J; J.add("G|5|100|100|X|1|0|9|0|0|0|0|0|0|0|0|0|0|1|1"); CHECK(J.bad == 1 && J.grade.empty(), "a grade out of range is refused");
        J.add("G|5|360|180|LonLO|1000|-1|2|1|0|1|0|0.1|0|1.2|0.4|10|20|2|1"); J.add("G|6|540|180|ONL|1001|-1|3|1|1|1|0|0.1|3|1.2|0.4|10|20|3|1"); J.add("G|7|540|180|PDL|1002|-1|1|0|0|1|0|0|0|1.2|0.4|10|20|3|1");
        CHECK(J.grade.size() == 1 && J.gradeAt(180, -1)->score == 3 && J.gradeAt(180, -1)->id == 6, "several levels run on one candle: a weak grade gives way to a strong one");
    }
    {   // ---- (1.1.1) review fixes
        Rig R; for (int i = 0; i < 20; ++i) R.candle(1020, 10, 600);
        R.fill(999, 3, -1, 9002, 501);                                   // a run is pending its 1 s
        { sr::Level a, b; a.name = "AAL"; a.px = 900; a.side = -1; b.name = "ZZH"; b.px = 1100; b.side = 1; std::vector<sr::Level> v; v.push_back(a); v.push_back(b); R.e.setLevels(v); }   // the level list is replaced meanwhile
        R.fill(1003, 3, 1, 8000, 800, 1500); R.fill(1003, 1, 1, 8001, 801); R.endCandle(); R.e.advance(R.t + 1000);
        int r = 0; for (const std::string& l : R.e.journal) if (l[0] == 'R') { ++r; const std::vector<std::string> f = F(l); CHECK(f[4] == "LonLO" && f[5] == "1000" && f[6] == "-1", "a pending run keeps ITS level when the list is rebuilt"); }
        CHECK(r == 1 && R.G().size() == 1 && F(R.G()[0])[4] == "LonLO", "decided and graded once, on the old level");
    }
    {   Rig R; R.fill(1001, 1, -1, 9000, 0); for (int k = 0; k < 6; ++k) R.fill(999, 1, -1, 9100 + k, 0, 10); R.fill(1001, 1, 1, 8000, 0, 2000); R.fill(1001, 1, 1, 8001, 0);
        bool gold = false; for (const std::string& l : R.e.journal) if (l[0] == 'R') gold = F(l)[14] == "1"; CHECK(!gold, "the first candle after a start: a burst has no normal to beat, so no gold run");
    }
    {   Rig R; for (int i = 0; i < 20; ++i) R.candle(1020, 10, 600); R.fill(999, 1, -1, 1, 1);
        for (unsigned long long k = 0; k < sr::Engine::EXC_CAP + 500; ++k) R.e.onFill(R.t, 998, 1, -1, 100000 + k, 200000 + k);
        R.fill(1002, 1, 1, 8000, 0, 100); R.endCandle(); R.e.advance(R.t + 1000);
        const std::vector<std::string> g = R.G(); CHECK(g.size() == 1 && std::atoll(F(g[0])[16].c_str()) == (long long)sr::Engine::EXC_CAP + 501, "the order map is capped; every lot still counts in the volume");
    }
    {   sr::Journal J; J.add("R|1500|1|180|LonLO|1000|-1|1|0.50|0.10|5|9|1|1|0"); J.add("G|2300|360|180|LonLO|1000|-1|1|0|0|1|0|0|0|1.2|0.4|10|20|2|2"); CHECK(J.maxId == 2300 && J.byId.empty(), "the highest run id counts every R / G line, gold or not");
        J.add("G|5|360|180|LonLO|1000|-1|3|1|1|1|0|0.1|3|1.2|0.4|10|20|2|1"); J.add("G|6|540|180|ONL|1001|-1|4|1|1|1|1|0.1|3|1.2|1.4|10|20|3|1"); CHECK(J.gradeAt(180, -1)->score == 3 && J.gradeAt(180, -1)->id == 5, "a strong grade once given never changes");
    }
    {   Rig R; R.candle(1020, 10, 600); const long long a0 = R.e.bars.back().add[0]; R.e.onBook(R.t + 500, 0, true, 1019, 50); CHECK(R.e.bars.back().add[0] == a0, "a book event after the candle closed is not added to it"); }
    {   const srg::Mkt g = srg::forMarket("ES"); CHECK(g.bigA == 36 && g.bigP == 35 && g.bids == 3 && g.strong == 3 && g.draw == 1, "ES grade values"); CHECK(srg::forMarket("XX").draw == 0 && srg::forMarket("XX").bigA == 0, "an unknown market: logged only, big signs off");
        for (const char* m : { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" }) CHECK(srg::forMarket(m).bigA >= 5 && srg::forMarket(m).bigP >= 5 && srg::forMarket(m).draw == 1, "every market has its thresholds"); }
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
