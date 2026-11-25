// t_vr1120.cpp -- VolRangeLogic 1.12.0 / VRRead 1.3.0 (Rassul 2026-11-24 19:44 CT "Go" on panes-read-mockup.html): the Volume read says how much,
// the biggest orders / sweeps, icebergs and a neutral note - never Bullish / Bearish / Buyers pushing (the Turn pane calls the winner).
// Real candles: GC 2026-11-20 7:30 - 10:21 AM (the mockup), inside numbers from the recorded order data.
#include "VolRangeLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int ok = 0, bad = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++bad; std::printf("FAIL %s\n", m); } }
int main() {
    std::ifstream f("gc_1120.txt"); std::string ln; std::vector<vrd::Cd> Cd; std::vector<vrd::Inside> I;
    while (std::getline(f, ln)) { std::istringstream s(ln); std::string hm; double o, h, l, c, v, bb, bs, sb, ss, hb, hs, z, atr; s >> hm >> o >> h >> l >> c >> v >> bb >> bs >> sb >> ss >> hb >> hs >> z >> atr;
        vrd::Cd d; d.o = o / 10; d.h = h / 10; d.l = l / 10; d.c = c / 10; d.vol = v; d.when = hm; Cd.push_back(d);
        vrd::Inside q; q.ok = true; q.bigBuy = (long long)bb; q.bigSell = (long long)bs; q.swBuy = (int)sb; q.swSell = (int)ss; q.hidBuy = (long long)hb; q.hidSell = (long long)hs; I.push_back(q); }
    C(Cd.size() >= 50, "fixture read");
    auto at = [&](const char* hm, int cntAll, int cntSame) { int k = -1; for (size_t i = 0; i < Cd.size(); ++i) if (Cd[i].when == hm) k = (int)i; vrd::Ctx x; x.dec = 1;
        x.c.assign(Cd.begin(), Cd.begin() + k + 1); x.in = I[(size_t)k]; x.cntAll = cntAll; x.cntSame = cntSame; return vrd::volPlain(x, vrd::volumeRead(x)); };
    vrd::Out o = at("09:30", 6, 99);
    C(o.ok && o.head == "Heavy - the biggest green bar in 99+ candles (1.9x)" && o.verdict == 1, "9:30: heavy, the biggest green bar in 99+, green");
    C(o.ex.size() >= 2 && o.ex[0] == "Biggest orders: buyers 25, sellers 21; one sell order swept 9 prices, buyers 7." && o.ex[1] == "Icebergs: bids refilled 29 lots, offers 34.", "9:30: orders and icebergs");
    o = at("10:06", 3, 7); C(o.head == "Above normal (1.3x)" && o.verdict == 0, "10:06: above normal, no colour");
    C(o.ex[0] == "Biggest orders: buyers 28, sellers 12; one buy order swept 8 prices, sellers 4.", "10:06: the buy sweep");
    o = at("09:21", 5, 2); C(o.head == "Heavy (2.4x)" && o.verdict == 0, "9:21: heavy, not the biggest - no colour");
    for (const char* hm : { "09:12", "09:15", "09:18", "09:21", "09:24", "09:27", "09:30", "09:33", "09:42", "10:06", "10:09", "10:21" }) { const vrd::Out p = at(hm, 1, 1);
        bool clean = p.head.find("Bullish") == std::string::npos && p.head.find("Bearish") == std::string::npos && p.head.find("pushing") == std::string::npos && p.head.find("Mixed") == std::string::npos && p.head.find("Quiet") == std::string::npos;
        for (const std::string& e : p.ex) if (e.find("Watch") != std::string::npos || e.find("in control") != std::string::npos) clean = false;
        C(clean && p.ex.size() >= 1 && p.ex.size() <= 3, "no verdict / watch words in any Volume read"); }
    // no order detail; climax note; light volume
    { vrd::Ctx x; x.dec = 2; for (int i = 0; i < 25; ++i) { vrd::Cd k; k.o = 100; k.c = 100.2; k.h = 100.3; k.l = 99.9; k.vol = 1000; x.c.push_back(k); } x.c.back().vol = 300; x.cx = 1;
      const vrd::Out p = vrd::volPlain(x, vrd::volumeRead(x)); C(p.head == "Light (0.3x)" && p.ex.size() == 2 && p.ex[0] == "No order detail for this candle." && p.ex[1] == "Climax volume at the low.", "light, no detail, the climax note"); }
    { vrd::Ctx x; vrd::Cd k; k.o = 1; k.c = 1; k.h = 1; k.l = 1; k.vol = 5; x.c.push_back(k); const vrd::Out p = vrd::volPlain(x, vrd::volumeRead(x)); C(p.head == "Not rated yet", "no normal yet"); }
    C(!vrd::volPlain(vrd::Ctx(), vrd::Out()).ok, "nothing in, nothing out");
    // (1.4.0) the Big Lot tier: GC RTH S20 = 32 / S10 43 / S5 56; overnight S20 39
    { vrd::Ctx x; x.dec = 1; x.c.assign(Cd.begin(), Cd.end()); vrd::Inside q; q.ok = true; q.bigBuy = 45; q.bigSell = 31; x.in = q; x.mkt = "GC"; x.rth = true;
      vrd::Out p = vrd::volPlain(x, vrd::volumeRead(x)); C(p.ex[0] == "Biggest orders: buyers 45 (S10), sellers 31.", "GC RTH: 45 = S10, 31 below S20");
      x.rth = false; q.bigSell = 39; x.in = q; p = vrd::volPlain(x, vrd::volumeRead(x)); C(p.ex[0].find("buyers 45 (S20), sellers 39 (S20)") != std::string::npos, "GC overnight: 45 and 39 = S20");
      x.mkt.clear(); p = vrd::volPlain(x, vrd::volumeRead(x)); C(p.ex[0].find("(S") == std::string::npos, "no market: no tier"); }
    C(std::string(vrr::VERSION) == "1.13.0" && std::string(vrd::VERSION) == "1.4.0", "versions");
    std::printf("%s %d pass, %d fail\n", bad ? "FAILED" : "ALL PASS", ok, bad); return bad ? 1 : 0; }
