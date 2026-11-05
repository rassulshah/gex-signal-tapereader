// test_bo170_onrly.cpp -- lsBreakout 1.7.0 overnight Rly (RallyLogic.h 1.2.0 run(..., overnight = true)). SDK-free.
// Rassul 2026-11-05 13:13 CT, EU 3-min chart, the 05:05-05:40 CT decline circled: "this is a clear rly dn" -> "go".
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. test_bo170_onrly.cpp -o t170 && ./t170
//   [1] overnight = false is the 1.1.0 run exactly (lsBigBidAsk unchanged); with overnight = true the RTH waves are the same, only overnight
//       waves are added (w.on), every one inside 17:00 CT .. the RTH open of its session.
//   [2] EU Nov 4-5 (eu_1104_1105_3m.txt = IRT bars Nov 3 22:48 - Nov 4 20:51 + the ChartView export Nov 4 21:36 - Nov 5 12:27):
//       the circled decline = the overnight down-wave 05:21 -> 05:51 CT fires a red Rly (Strong Dntrend on the 17:00 Globex VWAP).
//   [3] the overnight session's VWAP starts at its first bar (17:00 CT): its trend / bands do not see the RTH bars.
//   [4] no repaint: judging bar by bar (every prefix) gives each confirmed wave the same decision as the full pass.
//   [5] the engine (bko::Engine, 3-min) records the overnight waves with version "...-on" and keeps the RTH lines as 1.6.0 wrote them;
//       rlyOn = false = the 1.6.0 Rly set.
//   [6] the ES / NQ / GC weeks: overnight Rly per night printed (sanity: at most 8 a night from 02:00 CT), RTH Rly unchanged.
#include "BreakoutLogic.h"
#include <fstream>
#include <set>
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static const long long OFF = 2082844800LL;
struct Fx { std::vector<rly::Bar> R; std::vector<bko::In> I; std::vector<std::string> S; };
static Fx load(const std::string& f) {
    Fx X; std::ifstream in(f); std::string l;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        rly::Bar r; r.days = svl::daysFromCivil(y, mo, d); r.endSec = h * 3600 + mi * 60 + s; r.o = o; r.h = hi; r.l = lo; r.c = c; r.v = vo; X.R.push_back(r);
        bko::In b; b.days = r.days; b.endSec = r.endSec; b.t = (long long)b.days * 86400 + b.endSec + OFF; b.ok = true; b.o = o; b.h = hi; b.l = lo; b.c = c; b.vol = (long long)vo; X.I.push_back(b);
        X.S.push_back(l.substr(5, 11)); }
    return X;
}
static std::string key(const rly::Wave& w) { char b[160]; std::snprintf(b, sizeof b, "%d %d %d %d %d %d %d %.6f", w.gp, w.ge, w.gc, w.dir, w.fired ? 1 : 0, w.nb, w.trend, w.ratio); return b; }
static std::set<std::string> keys(const std::vector<rly::Wave>& W, int on) { std::set<std::string> s; for (const rly::Wave& w : W) if (on < 0 || (int)w.on == on) s.insert(key(w)); return s; }

int main() {
    int om = 0, cm = 0; svl::rthOf("EU", om, cm);
    const Fx E = load("eu_1104_1105_3m.txt");
    CHECK(E.R.size() > 600, "EU fixture (" + std::to_string(E.R.size()) + " bars)");
    // [1]
    const std::vector<rly::Wave> A = rly::run(E.R, 180, om, cm, 0, 3), Bw = rly::run(E.R, 180, om, cm, 0, 3, true);
    CHECK(keys(A, -1) == keys(Bw, 0) && keys(A, 1).empty(), "[1] overnight = false == 1.1.0; RTH waves unchanged with overnight on (" + std::to_string(A.size()) + " RTH)");
    int onN = 0, onFired = 0, outside = 0;
    for (const rly::Wave& w : Bw) if (w.on) { ++onN; onFired += w.fired;
        for (int g : { w.gp, w.ge, w.gc }) { const svl::Cls c = svl::classify(E.R[(size_t)g].days, E.R[(size_t)g].endSec, 180, om, cm); if (c.kind != svl::ON || c.sess != w.sess) ++outside; } }
    CHECK(onN > 5 && outside == 0, "[1] " + std::to_string(onN) + " overnight waves, every pivot / extreme / confirm bar inside its overnight session");
    {   int early = 0; for (const rly::Wave& w : Bw) if (w.on && (E.R[(size_t)w.ge].endSec < rly::ON_FROM_SEC || w.ge == w.gp)) ++early;
        CHECK(early == 0, "[1] overnight waves only from 02:00 CT (extreme bar), none of 0 bars"); }
    // [2] the circled decline
    const rly::Wave* hit = nullptr;
    for (const rly::Wave& w : Bw) if (w.on && E.S[(size_t)w.gp] == "11-05 05:21" && E.S[(size_t)w.ge] == "11-05 05:51") hit = &w;
    CHECK(hit && hit->dir == -1 && hit->fired && hit->trend <= -1 && hit->ratio > 2.5 && hit->nb >= 3, "[2] EU 05:21 -> 05:51 overnight decline fires a red Rly" + std::string(hit ? " (" + rly::failed(*hit) + ", " + std::to_string(hit->ratio) + " ATR, trend " + std::to_string(hit->trend) + ")" : ""));
    std::printf("  EU overnight Rly fired: "); for (const rly::Wave& w : Bw) if (w.on && w.fired) std::printf("%s %s  ", E.S[(size_t)w.ge].c_str(), w.dir > 0 ? "up" : "DN"); std::printf("(%d of %d waves)\n", onFired, onN);
    // [3] the overnight VWAP starts at 17:00: the same night alone (no RTH bars before it) gives the same overnight waves
    {   int a = -1, b = -1; for (int i = 0; i < (int)E.R.size(); ++i) { const svl::Cls c = svl::classify(E.R[(size_t)i].days, E.R[(size_t)i].endSec, 180, om, cm);
            if (c.kind == svl::ON && E.S[(size_t)i] >= "11-04 17" && a < 0) { a = i; }
            if (c.kind == svl::ON) { b = i; } }
        std::vector<rly::Bar> N(E.R.begin(), E.R.begin() + b + 1);
        const std::vector<rly::Wave> W1 = rly::run(N, 180, om, cm, 0, 3, true);
        std::set<std::string> s1, s2; for (const rly::Wave& w : W1) if (w.on && w.gp >= a) s1.insert(key(w)); for (const rly::Wave& w : Bw) if (w.on && w.gp >= a && w.gc <= b) s2.insert(key(w));
        CHECK(a > 0 && !s1.empty() && s1 == s2, "[3] the Nov 5 overnight session judged alone == inside the full pass (" + std::to_string(s1.size()) + " waves from " + E.S[(size_t)a] + ")"); }
    // [4] no repaint
    {   std::map<std::string, std::string> first; int bad = 0;
        for (size_t n = 50; n <= E.R.size(); ++n) { std::vector<rly::Bar> P(E.R.begin(), E.R.begin() + (long)n);
            for (const rly::Wave& w : rly::run(P, 180, om, cm, 0, 3, true)) { if (!w.on) continue; const std::string k = std::to_string(w.ge) + "/" + std::to_string(w.dir); const std::string v = key(w);
                auto it = first.find(k); if (it == first.end()) first[k] = v; else if (it->second != v) ++bad; } }
        std::set<std::string> full; for (const rly::Wave& w : Bw) if (w.on) full.insert(std::to_string(w.ge) + "/" + std::to_string(w.dir));
        int miss = 0; for (const auto& kv : first) if (!full.count(kv.first)) ++miss;
        CHECK(bad == 0 && miss == 0, "[4] bar by bar: every overnight wave keeps its first decision (" + std::to_string(first.size()) + " waves, " + std::to_string(bad) + " changed, " + std::to_string(miss) + " not in the full pass)"); }
    // [5] the engine
    {   bko::Engine G; G.update(E.I, (long)E.I.size(), E.I[0].t, 0.00005, 180, om, cm);
        bko::Engine H; H.rlyOn = false; H.update(E.I, (long)E.I.size(), E.I[0].t, 0.00005, 180, om, cm);
        int on = 0, rth = 0, same = 0; bool fiveFiftyOne = false;
        for (const auto& kv : G.rlyL) { const std::string& l = kv.second.line; const bool o = l.find("-zz3-on") != std::string::npos;
            if (o) { ++on; const auto r = G.rly.find(kv.first); if (r != G.rly.end() && r->second.fired && r->second.dir < 0 && l.find("|-1|1|") != std::string::npos) { const long long t = r->second.te - OFF; if ((t % 86400) == 5 * 3600 + 51 * 60) fiveFiftyOne = true; } }
            else { ++rth; const auto h = H.rlyL.find(kv.first); if (h != H.rlyL.end() && h->second.line == l) ++same; } }
        CHECK(on > 5 && fiveFiftyOne, "[5] engine: " + std::to_string(on) + " overnight Rly lines (version -zz3-on), the 05:51 red Rly fired");
        CHECK(rth == (int)H.rlyL.size() && same == rth, "[5] engine: the RTH Rly lines are the rlyOn = false (1.6.0 rule) lines exactly (" + std::to_string(same) + " of " + std::to_string(H.rlyL.size()) + ")");
        CHECK(std::string(bko::VERSION) == "1.8.0" && std::string(rly::VERSION) == "1.2.0", "[5] versions 1.8.0 / RallyLogic 1.2.0"); }
    // [6] the weeks
    struct Mk { const char* f; const char* m; double tk; } MS[3] = { { "wk_ES_eve_3m.txt", "ES", 0.25 }, { "wk_NQ_3m.txt", "NQ", 0.25 }, { "wk_GC_3m.txt", "GC", 0.1 } };
    for (const Mk& m : MS) {
        const Fx X = load(m.f); int o2 = 0, c2 = 0; svl::rthOf(m.m, o2, c2);
        const std::vector<rly::Wave> a = rly::run(X.R, 180, o2, c2, 0, 3), b = rly::run(X.R, 180, o2, c2, 0, 3, true);
        std::map<long, int> perNight; for (const rly::Wave& w : b) if (w.on && w.fired) ++perNight[w.sess];
        int mx = 0, tot = 0; for (const auto& kv : perNight) { mx = std::max(mx, kv.second); tot += kv.second; }
        CHECK(keys(a, -1) == keys(b, 0), std::string("[6] ") + m.m + ": RTH Rly unchanged");
        CHECK(mx <= 8, std::string("[6] ") + m.m + ": at most 8 overnight Rly a night from 02:00 CT (max " + std::to_string(mx) + ")");
        std::printf("  %s: %d overnight Rly over %zu nights (max %d a night)\n", m.m, tot, perNight.size(), mx);
    }
    std::printf("RESULT %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
