// t_vr1150.cpp -- VolRangeLogic 1.15.0 / VRRead 1.6.0 (Rassul 2026-11-24 22:39 CT "everything else is a go" on reads-conclude-mockup.html v30): the Volume
// read concludes - the overall call, effort vs result + price agreement, large orders, who led, sliced / pulled. Real GC 11-20 candles (the mockup), the
// other panes' words as the mockup has them (Turn kinds, buy / sell verdicts, the large-order counts measured from the order store).
#include "../VolRangeLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int ok = 0, bad = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++bad; std::printf("FAIL %s\n", m); } }
static void show(const char* t, const vrd::Out& o) { std::printf("%s  [%s] %s", t, o.headQ.c_str(), o.head.c_str()); for (size_t k = 0; k < o.ex.size(); ++k) std::printf(" | [%s] %s", k < o.exQ.size() ? o.exQ[k].c_str() : "-", o.ex[k].c_str()); std::printf("\n"); }
int main() {
    std::ifstream f("gc_1120.txt"); std::string ln; std::vector<vrd::Cd> Cd; std::vector<vrd::Inside> I; std::vector<double> A;
    while (std::getline(f, ln)) { std::istringstream s(ln); std::string hm; double o, h, l, c, v, bb, bs, sb, ss, hb, hs, z, atr; s >> hm >> o >> h >> l >> c >> v >> bb >> bs >> sb >> ss >> hb >> hs >> z >> atr;
        vrd::Cd d; d.o = o / 10; d.h = h / 10; d.l = l / 10; d.c = c / 10; d.vol = v; d.when = hm; Cd.push_back(d); A.push_back(atr / 10);
        vrd::Inside q; q.ok = true; q.bigBuy = (long long)bb; q.bigSell = (long long)bs; q.swBuy = (int)sb; q.swSell = (int)ss; q.hidBuy = (long long)hb; q.hidSell = (long long)hs; I.push_back(q); }
    C(Cd.size() >= 50, "fixture read");
    auto row = [](int kind, int end, long long lmS, long long lmB, long long lB, long long lO, long long bB, long long bO) { tvr::Row r; r.kind = kind; r.end = end; r.lm[0] = lmS; r.lm[1] = lmB; r.ll[0] = lB; r.ll[1] = lO; r.llB[0] = bB; r.llB[1] = bO; return r; };
    auto at = [&](const char* hm, const tvr::Row* tv, int bsV, int bsTrap, bool again, const std::vector<const tvr::Row*>& lead) { int k = -1; for (size_t i = 0; i < Cd.size(); ++i) if (Cd[i].when == hm) k = (int)i;
        vrd::Ctx x; x.dec = 1; x.mkt = "GC"; x.rth = true; x.tick = 0.1; x.c.assign(Cd.begin(), Cd.begin() + k + 1); x.in = I[(size_t)k]; x.atr = A[(size_t)k - 1];
        for (int j = k + 1; j < (int)Cd.size() && (int)x.nx.size() < 10; ++j) x.nx.push_back(Cd[(size_t)j]);
        if (tv) { x.tvOk = true; x.tv = *tv; } if (bsV != 9) { x.bsOk = true; x.bsV = bsV; x.bsTrap = bsTrap; } x.again = again; if (!lead.empty()) vrd::leadOf(lead, x);
        return vrd::volPlain(x, vrd::volumeRead(x)); };
    // the mockup's four candles
    const tvr::Row r921 = row(2, 1, 4, 5, 17, 9, 17, 0), r924 = row(0, 0, 1, 3, 0, 0, 0, 0), r927 = row(0, 0, 6, 6, 0, 0, 0, 0), r930 = row(1, 1, 1, 6, 1, 6, 0, 6);
    vrd::Out o = at("09:30", &r930, 1, 0, false, { &r930, &r927, &r924, &r921 }); show("9:30", o);
    C(o.head == "Strong bullish - buyers took the low on heavy volume" && o.verdict == 1 && o.headQ == "Verdict", "9:30 overall call");
    C(o.ex.size() >= 3 && o.ex[0] == "Effort matched result - price followed through." && o.exQ[0] == vrd::Q1, "9:30 effort + agreement");
    C(o.ex[1] == "Large buyers outnumbered sellers 6 to 1 - but large offers wait over the high." && o.exQ[1] == vrd::Q4, "9:30 large orders");
    C(o.ex[2] == "Large buyers have led for 4 candles (20 to 12).", "9:30 who led");
    tvr::Row r1006 = row(1, 1, 2, 5, 5, 8, 0, 8); r1006.sl[1] = 7; r1006.slN[1] = 5;
    o = at("10:06", &r1006, 1, 0, true, { &r1006 }); show("10:06", o);
    C(o.head == "Strong bullish - buyers took the low again" && o.ex.size() >= 3 && o.ex[0] == "Effort matched result - price followed through.", "10:06 overall + effort");
    C(o.ex[1] == "Large buyers outnumbered sellers 5 to 2 - but large offers wait over the high." && o.ex[2] == "A buyer sliced 35 lots into 7-lot pieces." && o.exQ[2] == vrd::Q5, "10:06 large + sliced");
    o = at("09:21", &r921, 0, 0, false, { &r921 }); show("9:21", o);
    C(o.head == "Bullish - sellers absorbed at a new low" && o.verdict == 1, "9:21 two of three");
    C(o.ex.size() >= 2 && o.ex[0] == "Effort without result - but price broke the low." && o.ex[1] == "Large orders on both sides - large bids stacked under the low.", "9:21 effort + large");
    const tvr::Row r912 = row(0, 0, 1, 2, 7, 12, 0, 0);
    o = at("09:12", &r912, 0, 1, false, { &r912 }); show("9:12", o);
    C(o.head == "Mixed - buyers trapped, no turn yet" && o.verdict == 0, "9:12 trapped, no turn");
    // without the other panes' files: this pane alone
    o = at("09:30", nullptr, 9, 0, false, {}); show("9:30 alone", o); C(o.head == "Bullish - heavy volume closed at the high" && o.ex.size() == 1, "alone: own lean, no large-order lines");
    o = at("09:45", nullptr, 9, 0, false, {}); show("9:45 alone", o); C(o.head.compare(0, 8, "Neutral ") == 0 && o.verdict == 0, "quiet candle: Neutral");
    // disagreement; pulled
    o = at("09:30", &r930, -1, 0, false, {}); C(o.head == "Bullish - buyers took the low", "one of two agrees");
    { tvr::Row r = r930; r.kind = 3; r.end = 1; o = at("09:30", &r, 1, 0, false, {}); show("9:30 aggressor", o); C(o.head == "Mixed - sellers broke the low, the volume and buy / sell disagree" && o.verdict == 0, "nobody agrees: Mixed"); }
    { tvr::Row r = r930; r.pull[0] = 100; r.pull[1] = 400; r.pullN[0] = 100; r.pullN[1] = 200; o = at("09:30", &r, 1, 0, false, {}); bool p = false; for (const std::string& e : o.ex) if (e == "Offers pulled before the push.") p = true; C(p, "offers pulled"); }
    // no "?", no size words the bar shows, nothing about the biggest bar
    for (const char* hm : { "09:12", "09:21", "09:30", "10:06", "09:45" }) { const vrd::Out p = at(hm, &r930, 1, 0, false, {}); bool clean = p.head.find('?') == std::string::npos && p.head.find("biggest") == std::string::npos && p.head.find("(1.") == std::string::npos;
        for (const std::string& e : p.ex) { if (e.find('?') != std::string::npos || e.find("Biggest orders") != std::string::npos || e.find("Icebergs") != std::string::npos) clean = false; } C(clean && p.exQ.size() == p.ex.size(), "no repeats, every line has its question"); }
    // agreement rule
    { std::vector<vrd::Cd> nx(2); nx[0].h = 101; nx[0].l = 99.95; nx[1].h = 102; nx[1].l = 100; C(vrd::agreeOf(1, 100, nx, 1.5, 0.1) == 1, "agreed on the 2nd"); nx[0].l = 99.85; C(vrd::agreeOf(1, 100, nx, 1.5, 0.1) == -2, "broke the low first"); C(vrd::agreeOf(1, 100, std::vector<vrd::Cd>(), 1.5, 0.1) == 0, "waiting"); }
    C(std::string(vrd::VERSION) == "1.6.0" && std::string(vrr::VERSION) == "1.15.0", "versions");
    std::printf("t_vr1150: %d checks, %d fails\n", ok + bad, bad); return bad ? 1 : 0;
}
