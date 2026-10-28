// test_ex_host.cpp -- lsLiquidityProfile 0.61.1 THE Ex MARK on the mock host (order feed -> ExhaustLogic engine -> record -> the letter on
// the decision candle -> restart reads the record back -> the params file gate). ES 3-min, a key price level (PDH) from the chart's own
// bars: buyers lift into PDH from below and fade (exhausted at a top) -> red "Ex?" ABOVE the decision candle; later sellers hit PDH from
// above and fade with a params file of 31 key-level cases -> green "Ex" BELOW. A lsBigBidAsk BO on the first decision candle -> nudged.
// From plugin\:  g++ -std=c++17 -O1 -Itests_lqp044 -I. tests_ex/test_ex_host.cpp -o texh && ./texh [drawdump.txt]
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sstream>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void bar(mock::Host& h, unsigned long t, float o, float hi, float lo, float c) { mock::Bar b; b.time = t; b.open = o; b.high = hi; b.low = lo; b.close = c; b.rows.push_back(mock::Price(c, 50, 40, 90)); h.bars.push_back(b); }
static const COLOR GREEN = 0x22C55E, RED = 0xEF4444;
// the coloured (not the outline) "Ex" / "Ex?" texts
static std::vector<const mock::Draw*> exTexts(mock::Host& h) { std::vector<const mock::Draw*> o; for (auto& d : h.draws) if (d.type == "text" && (d.text == "Ex" || d.text == "Ex?") && d.color != 0) o.push_back(&d); return o; }
static int barOfX(mock::Host& h, int x) { return (x - h.barX0 + h.barW / 2) / h.barW; }
static void dump(mock::Host& h, const char* path, int from, int boBar) {
    if (!path) return; std::ofstream o(path);
    o << "BO," << boBar << "\n";
    o << "PANE," << h.paneBottom << "," << h.priceBase << "," << h.pxPer << "," << h.barX0 << "," << h.barW << "\n";
    for (size_t i = (size_t)from; i < h.bars.size(); ++i) o << "BAR," << i << "," << h.bars[i].open << "," << h.bars[i].high << "," << h.bars[i].low << "," << h.bars[i].close << "\n";
    for (auto& d : h.draws) if (d.type == "text" && (d.text == "Ex" || d.text == "Ex?")) o << "TEXT," << d.text << "," << d.l << "," << d.t << "," << d.r << "," << d.b << "," << std::hex << d.color << std::dec << "\n";
}
int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/exhXXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 22) * 86400LL), prev = day - 86400;
    mock::Host h; mock::current = &h;
    h.tick = 0.25f; h.seconds = 180; h.symbol = "EPZ26"; h.root = "EPZ26";
    h.priceBase = 7680.0f; h.pxPer = 20.0f; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 3000; h.scaleLeft = 2980;
    // yesterday 08:33-15:00 (RTH, PDH 7,712.00), the evening / overnight 15:03-08:30 quiet, today from 08:33 below PDH
    // continuous 3-min candles from 17:03 two days back: full sessions, so the key levels are complete (PDH = yesterday's RTH high 7,712)
    for (unsigned long t = prev - 86400 - 7 * 3600 + 180; t <= day + 10 * 3600; t += 180) {
        const bool top = t == prev + 11 * 3600; bar(h, t, 7700, top ? 7712.0f : 7703.0f, 7698, 7701); }
    h.barX0 = 2600 - (int)h.bars.size() * h.barW;   // the last candles inside the pane (x of bar i = barX0 + i * barW)
    for (int i = 0; i < 80; ++i) { DEPTH_LEVEL L; L.bid = (float)(7700.75 - i * 0.25); L.ask = (float)(7701.0 + i * 0.25); L.bidsize = 10; L.asksize = 10; h.depth.push_back(L); }
    const unsigned long t0 = day + 10 * 3600;   // the last closed candle ends 10:00 (fully below PDH: arms it as a resistance)
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    bar(h, t0 + 180, 7701, 7702, 7700, 7701);   // the forming candle 10:00-10:03
    for (int s = 0; s < 4; ++s) { h.now = t0 + 5 + s; p->calc(0); usleep(5000); }
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    std::string keys; for (auto& k : S->exKeys) keys += k.name + " ";
    std::printf("  market %s keys [%s] zones %zu armed %zu atr %.2f session %lld/%lld\n", S->market.c_str(), keys.c_str(), S->ex.zones.size(), S->ex.armed.size(), S->atr, S->exSession, S->session);
    const exh::Zone* pdh = nullptr; for (auto& z : S->ex.zones) if (z.name.find("PDH") != std::string::npos) pdh = &z;
    CHECK(pdh && (double)pdh->lo * 0.25 <= 7712.0 && (double)pdh->hi * 0.25 >= 7712.0, "PDH 7,712 is an Ex zone (key price level from the chart's own bars)");
    CHECK(pdh && S->ex.armed.count(pdh->key) && S->ex.armed[pdh->key] == exh::RES, "armed as a RESISTANCE (the 10:00 candle closed fully below it)");
    bool onlyKeys = true; for (auto& z : S->ex.zones) { std::string n = z.name; size_t a = 0; for (;;) { size_t b = n.find('+', a); if (!exh::isKeyCode(n.substr(a, b == std::string::npos ? b : b - a))) onlyKeys = false; if (b == std::string::npos) break; a = b + 1; } }
    CHECK(onlyKeys, "every Ex zone is a key price level (no POC / gamma / order zones)");
    // a 1,000-lot offer stack at 7,711.75-7,712.25 (ids 100+), bids under (ids 200+)
    for (int k = 0; k < 3; ++k) mock::feed.push(0, 0, 7711.75f + 0.25f * k, 1000, (RTDATE)(t0 + 10), 100 + k);
    // 10:00:30 buyers lift into PDH: 10 lots / s for 90 s, then 1 lot every 3 s (fade) - price stays at the zone -> exhausted buyers
    const unsigned long ta = t0 + 30;
    int aid = 5000;
    for (int s = 0; s < 90; ++s) mock::feed.push(3, 1, 7712.0f, 10, (RTDATE)(ta + s), 101, aid++);
    for (int s = 90; s <= 180; s += 3) mock::feed.push(3, 1, 7712.0f, 1, (RTDATE)(ta + s), 101, aid++);
    for (int s = 0; s < 3; ++s) { h.now = ta + 60 + s; p->calc(0); usleep(5000); }
    CHECK(S->ex.active.size() == 1 && S->ex.active[0].side == exh::RES, "a visit at PDH (resistance) started at the first trade inside the zone");
    // the candles: 10:03 (contains the arrival), 10:06 (contains the decision 10:03:30), 10:09 forming
    h.bars.back().high = 7712.25f; h.bars.back().close = 7711.5f;
    bar(h, t0 + 360, 7711.5f, 7712.25f, 7710.5f, 7711.0f);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 215 + s; p->calc(0); usleep(5000); }   // 10:03:35 (> decision + 3 s)
    CHECK(S->ex.decided.size() == 1 && S->ex.decided[0].isEx && S->ex.decided[0].label == "Ex?", "decided once at arrival + 180 s: Ex, labelled Ex? (no params file)");
    if (!S->ex.decided.empty()) std::printf("  exh %.3f share %.3f s1 %d s2 %d att %.0f / %.0f\n", S->ex.decided[0].exh, S->ex.decided[0].share, S->ex.decided[0].s1, S->ex.decided[0].s2, S->ex.decided[0].att1, S->ex.decided[0].att2);
    const int decBar = (int)h.bars.size() - 1;   // 10:03-10:06, still forming
    h.draws.clear(); p->draw(); CHECK(exTexts(h).empty(), "nothing drawn while the decision candle is still forming");
    bar(h, t0 + 540, 7711.0f, 7711.5f, 7708.0f, 7708.5f);   // 10:06 closes; 10:06-10:09 forming
    // lsBigBidAsk fired a BO on that decision candle (BO above it): the Ex must not sit on it
    {   const std::string bd = flex + "\\BigBidAsk"; mkdir(bd.c_str(), 0777);
        std::string ln = "BBO|" + std::to_string((long long)h.bars[(size_t)decBar].time) + "|" + std::to_string((long long)h.bars[(size_t)decBar].time) + "|1|1|fired|7712";
        for (int k = 7; k <= 22; ++k) ln += "|1"; ln += "|1.31.0|5|1|0";
        std::ofstream f((bd + "\\ES-EPZ26-180-breakouts-" + std::to_string(S->session) + ".csv").c_str()); f << ln << "\n"; }
    S->exBoMs = -1;
    for (int s = 0; s < 3; ++s) { h.now = t0 + 365 + s; p->calc(0); usleep(5000); }
    CHECK(S->exBo.size() == 1, "the BO letter of that candle is known (BigBidAsk breakout record)");
    h.draws.clear(); p->draw();
    std::vector<const mock::Draw*> X = exTexts(h);
    cppExtension::PNT hiP; hiP.set(decBar, 7712.25f); const int cx = h.barX0 + decBar * h.barW;
    CHECK(X.size() == 1 && X[0]->text == "Ex?" && X[0]->color == RED, "one red Ex? (buyers exhausted at a top)");
    CHECK(X.size() == 1 && X[0]->b <= hiP.v, "ABOVE the decision candle's high");
    CHECK(X.size() == 1 && X[0]->b < hiP.v - 2 - rowPx(10), "moved clear of the BO letter + its print room (no board entry: 2 rows)");
    CHECK(X.size() == 1 && std::abs((X[0]->l + X[0]->r) / 2 - cx) <= 1, "(0.61.1) centred on ITS candle, never sideways");
    {   // (0.61.1) lsBigBidAsk 1.31.1 posts the BO letter as drawn (kind L: 15 px from the high to its top) -> Ex stacked straight on it, 1 px
        std::vector<sb::Ev> L1(1); L1[0].bar = S->ex.decided.empty() ? 0 : (long long)h.bars[(size_t)decBar].time; L1[0].side = 1; L1[0].val = 15;
        sb::publish('L', S->symbol, 180, std::string(p->getChartLabel() ? p->getChartLabel() : ""), steadyMs(), L1);
        h.draws.clear(); p->draw(); std::vector<const mock::Draw*> Y = exTexts(h);
        CHECK(Y.size() == 1 && Y[0]->b == (int)hiP.v - 15 - 1 - 2 &&   /* the glyph lift (2 px, as the BO letter) */ std::abs((Y[0]->l + Y[0]->r) / 2 - cx) <= 1, "(0.61.1) stacked straight against the drawn BO letter: no gap, same candle");
        sb::publish('L', S->symbol, 180, std::string(p->getChartLabel() ? p->getChartLabel() : ""), steadyMs(), std::vector<sb::Ev>());
        h.draws.clear(); p->draw(); X = exTexts(h); }
    int outlines = 0; for (auto& d : h.draws) if (d.type == "text" && d.text == "Ex?" && d.color == 0) ++outlines;
    CHECK(outlines == 4, "the BO letter's dark outline (4 offsets)");
    usleep(1100000); h.now = t0 + 370; p->calc(0);
    const std::string recPath = flex + "\\LiquidityProfile\\ES-EPZ26-180-exhaust-" + std::to_string(S->session) + ".csv";
    const std::string rec = readAll(recPath);
    CHECK(rec.rfind("EXH|ver|", 0) == 0 && std::count(rec.begin(), rec.end(), '\n') == 2 && rec.find("PDH") != std::string::npos && rec.find("|T|R|") != std::string::npos && rec.find("|1|Ex?|-1\n") != std::string::npos, "the record: header + one decision line (PDH, resistance, is_ex 1, Ex?)");
    const std::string st = readAll(flex + "\\LiquidityProfile.status-ES-180.txt");
    CHECK(st.find(std::string("EXHAUST,") + exh::VERSION + ",on,settings no file: on (default),gate Ex? (no params file)") != std::string::npos && st.find("decided 1 (Ex 1,") != std::string::npos && st.find("drawn 1 nudged 1") != std::string::npos, "status: EXHAUST line");
    CHECK(std::string(LQ_VERSION) == "0.62.0" && std::string(lpset::PLUGIN_VERSION) == LQ_VERSION, "versions = 0.62.0");
    { const ss::Spec& sp = lpset::profileSpec(); bool ex = false; for (int k = 0; k < sp.n; ++k) if (std::string(sp.rows[k].label) == "Exhaustion (Ex)" && sp.rows[k].def == 1) ex = k == sp.n - 1;
      CHECK(ex && sp.paramVersion == 14, "settings: \"Exhaustion (Ex)\" appended, default on, parameter version 14"); }
    const int boxL = X.empty() ? 0 : X[0]->l, boxT = X.empty() ? 0 : X[0]->t;
    // RESTART: a new instance reads the record back - the same letter at the same place, nothing decided again
    p->destroy(); p->mdPark_.clear(); delete p; h.data = nullptr;
    p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 4; ++s) { h.now = t0 + 380 + s; p->calc(0); usleep(5000); }
    S = static_cast<LQState*>(h.data);
    CHECK(S->exRestored == 1 && S->ex.decided.size() == 1 && S->ex.decided[0].restored, "after a restart the decision comes back from the record");
    h.draws.clear(); p->draw(); X = exTexts(h);
    CHECK(X.size() == 1 && X[0]->text == "Ex?" && X[0]->color == RED && X[0]->l == boxL && X[0]->t == boxT, "the same red Ex? at the same place (never repainted)");
    usleep(1100000); h.now = t0 + 390; p->calc(0);
    { const std::string r2 = readAll(recPath); CHECK(std::count(r2.begin(), r2.end(), '\n') == 2, "nothing written twice"); }
    // THE GATE: the nightly writes 31 key-level cases for ES -> a NEW decision is "Ex"; the old one keeps its Ex? (decided then)
    { std::ofstream f((flex + "\\LRA-Exhaust-ES.csv").c_str()); f << "market,n,held,control,min_n,min_lift,updated\nES,31,0.72,0.62,30,0,2026-11-20\n"; }
    S->exGateMs = -1;
    // price runs ABOVE PDH (10:09 and 10:12 close fully above the zone) -> armed as SUPPORT; 10:15:20 sellers hit the bids at 7,712, fading
    h.bars.back().open = 7709; h.bars.back().high = 7716; h.bars.back().low = 7709; h.bars.back().close = 7715;   // 10:06-10:09
    bar(h, t0 + 720, 7715, 7718, 7714, 7717);   // 10:12
    bar(h, t0 + 900, 7717, 7718, 7715, 7716);   // 10:15 (closes fully above)
    bar(h, t0 + 1080, 7716, 7716, 7716, 7716);  // 10:15-10:18 forming
    for (int s = 0; s < 3; ++s) { h.now = t0 + 905 + s; p->calc(0); usleep(5000); }
    CHECK(S->ex.gate.ok && S->ex.gate.n == 31, "the gate file: 31 key-level cases -> Ex");
    for (int k = 0; k < 3; ++k) mock::feed.push(0, 1, 7711.75f + 0.25f * k, 1000, (RTDATE)(t0 + 905), 300 + k);
    const unsigned long tb = t0 + 920;
    for (int s = 0; s < 90; ++s) mock::feed.push(3, 0, 7712.0f, 8, (RTDATE)(tb + s), 301, aid++);
    for (int s = 90; s <= 180; s += 6) mock::feed.push(3, 0, 7712.0f, 1, (RTDATE)(tb + s), 301, aid++);
    for (int s = 0; s < 3; ++s) { h.now = tb + 30 + s; p->calc(0); usleep(5000); }
    CHECK(S->ex.active.size() == 1 && S->ex.active[0].side == exh::SUP, "a support visit at PDH (price came from above)");
    h.bars.back().low = 7711.75f; h.bars.back().close = 7712.5f;   // 10:15-10:18
    bar(h, t0 + 1260, 7712.5f, 7713, 7711.75f, 7712.75f);           // 10:18-10:21 (holds the decision 10:18:20)
    for (int s = 0; s < 3; ++s) { h.now = t0 + 1105 + s; p->calc(0); usleep(5000); }
    bar(h, t0 + 1440, 7712.75f, 7714, 7712.5f, 7713.5f);            // 10:21 closes it
    for (int s = 0; s < 3; ++s) { h.now = t0 + 1265 + s; p->calc(0); usleep(5000); }
    CHECK(S->ex.decided.size() == 2 && S->ex.decided[1].isEx && S->ex.decided[1].label == "Ex" && S->ex.decided[1].gateN == 31, "the new decision: Ex (gate open)");
    h.draws.clear(); p->draw(); X = exTexts(h);
    const int dec2 = (int)h.bars.size() - 2;
    cppExtension::PNT loP; loP.set(dec2, 7711.75f);
    const mock::Draw* g = nullptr; const mock::Draw* r = nullptr; for (auto* d : X) { if (d->text == "Ex" && d->color == GREEN) g = d; if (d->text == "Ex?" && d->color == RED) r = d; }
    CHECK(g && g->t >= loP.v && std::abs(barOfX(h, (g->l + g->r) / 2) - dec2) <= 0, "a green Ex BELOW its decision candle (sellers exhausted at a bottom)");
    CHECK(r && r->b <= hiP.v && std::abs((r->l + r->r) / 2 - cx) <= 1, "the old red Ex? still there, still Ex?, still centred on its candle");
    dump(h, argc > 1 ? argv[1] : nullptr, (int)h.bars.size() - 26, decBar);
    // the setting off: nothing drawn, nothing measured
    { std::ofstream f((flex + "\\LiquidityProfile.exhaust-ES.txt").c_str()); f << "0\n"; }
    usleep(1100000); h.now = t0 + 1280; p->calc(0);
    h.draws.clear(); p->draw(); CHECK(!S->exOn && exTexts(h).empty(), "\"Exhaustion (Ex)\" off: no letters");
    p->destroy(); p->mdPark_.clear(); delete p;
    std::printf("lsLiquidityProfile %s Ex host: %d passed, %d failed\n", LQ_VERSION, passes, fails);
    return fails ? 1 : 0;
}
