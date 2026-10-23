// test_aggbook_113.cpp -- lsBidAskStats 1.13.0 (Aggression) + lsBookFlow 1.3.0 decision rules (SDK-free, 2026-10-18).
//   one spike rule (>= 2.0x the plain 20-candle average, decided at the close, no jump), closed bars only (forming = dim, no label),
//   Book Flow brightness / label from the NET that is drawn, colour by side, Market State facts (SRC AGG / BOOK) and their round trip,
//   the version literals of both plugins.
// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I<plugin> test_aggbook_113.cpp
//        run: ./a.out <plugin dir>
#include "OrderFlowPaneSettings.h"
#include "BidAskStatsLogic.h"
#include "BookFlowLogic.h"
#include "LetterStrips.h"
#include "MarketState.h"
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>

static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
#define EQS(a, b) do { ++checks; const std::string _a = (a), _b = (b); if (_a != _b) { ++fails; std::printf("FAIL %s:%d  '%s' != '%s'\n", __FILE__, __LINE__, _a.c_str(), _b.c_str()); } } while (0)

static bas::BarMax bm(long long bid, long long ask) { bas::BarMax m; m.ok = true; m.bid = bid; m.ask = ask; m.totBid = bid * 3; m.totAsk = ask * 3; return m; }

// ---------------------------------------------------------------- Aggression
static void aggSpikeRule() {
    bas::SpikeCfg c;                                   // defaults: avg 2.0, jump 1.5 (record field only), lookback 10
    CHECK(bas::isSpike(200, 100.0, c));
    CHECK(!bas::isSpike(199, 100.0, c));
    CHECK(!bas::isSpike(0, 0.0, c));
    bas::SpikeCfg off = c; off.avgT = 0; CHECK(!off.on()); CHECK(!bas::isSpike(500, 100.0, off));
    bas::SpikeCfg noJump = c; noJump.jumpT = 0; noJump.lookback = 0; CHECK(noJump.on());   // an old file's 0 jump / lookback never switches spikes off
    bas::SpikeCfg hi = c; hi.avgT = 30; CHECK(!bas::isSpike(250, 100.0, hi)); CHECK(bas::isSpike(300, 100.0, hi));

    // the old jump would have refused this one: prior asks 100 with one 150 in the last 10 -> needs 225; 210 = 2.0x+ of the mean -> spike now
    bas::Series s; s.spike = c;
    long long t = 1000;
    for (int i = 0; i < 20; ++i) { s.add(t, bm(100, i == 15 ? 150 : 100), 20); t += 180; }
    const bas::Stat& a = s.add(t, bm(100, 210), 20);
    CHECK(a.askRatioOk); CHECK(a.askRatio > 2.0 && a.askRatio < 2.1);
    CHECK(a.askSpike);                                 // the one rule
    CHECK(a.priorMax == 150);                          // the jump base is still computed (record / status Jump field)
    CHECK(!a.bidSpike);
    CHECK(bas::high(a, 1) && !bas::high(a, 0));        // the message's HIGH = the spike
    t += 180;
    const bas::Stat b = s.add(t, bm(100, 100), 20);
    CHECK(!b.askSpike && !b.bidSpike);
    // boundary: exactly 2.0x the plain average is a spike, just under is not
    bas::Series q; q.spike = c; long long u = 5000;
    for (int i = 0; i < 20; ++i) { q.add(u, bm(50, 50), 20); u += 60; }
    CHECK(q.add(u, bm(100, 99), 20).bidSpike); u += 60;
    bas::Series q2; q2.spike = c; long long u2 = 5000;
    for (int i = 0; i < 20; ++i) { q2.add(u2, bm(50, 50), 20); u2 += 60; }
    const bas::Stat z = q2.add(u2, bm(99, 99), 20); CHECK(!z.bidSpike && !z.askSpike);
    // fewer than 5 prior bars: no ratio, no spike
    bas::Series few; few.spike = c; long long w = 1;
    for (int i = 0; i < 4; ++i) { few.add(w, bm(10, 10), 20); ++w; }
    const bas::Stat f = few.add(w, bm(1000, 1000), 20); CHECK(!f.bidRatioOk && !f.bidSpike);
}
static void aggNoRepaint() {
    // a closed bar's decision never changes when later bars arrive (computed once at add)
    bas::Series s; s.spike = bas::SpikeCfg();
    long long t = 100; std::vector<bas::Stat> got;
    for (int i = 0; i < 60; ++i) { const long long v = 50 + (i * 37) % 120; got.push_back(s.add(t, bm(v, 170 - v), 20)); t += 180; }
    long long k = 100; int same = 0;
    for (size_t i = 0; i < got.size(); ++i, k += 180) {
        const bas::Stat& now = s.bars[k];
        same += now.bidSpike == got[i].bidSpike && now.askSpike == got[i].askSpike && now.bidRatio == got[i].bidRatio && now.askRatio == got[i].askRatio;
    }
    CHECK(same == 60);
    // a settings re-decide with the same settings changes nothing
    s.respike(s.spike); k = 100; same = 0;
    for (size_t i = 0; i < got.size(); ++i, k += 180) same += s.bars[k].bidSpike == got[i].bidSpike && s.bars[k].askSpike == got[i].askSpike;
    CHECK(same == 60);
}
static void aggLook() {
    bas::Stat st; st.m = bm(300, 100); st.bidRatioOk = true; st.bidRatio = 3.0; st.bidSpike = true; st.askRatioOk = true; st.askRatio = 1.9;
    bas::SideLook f = bas::sideLook(&st, 0, false);    // the forming candle: never bright, never labelled
    CHECK(!f.bright && !f.ratioOk);
    bas::SideLook c = bas::sideLook(&st, 0, true);
    CHECK(c.bright && c.ratioOk && c.ratio == 3.0);
    bas::SideLook d = bas::sideLook(&st, 1, true);     // closed, not a spike: dim and no ratio handed to the label code
    CHECK(!d.bright && !d.ratioOk);
    CHECK(!bas::sideLook(nullptr, 0, true).bright);
    // a 1.12 record row: its stricter recorded spike OR the one rule (so a 2.0x bar drawn bright by 1.12 stays bright)
    bas::SpikeCfg cfg;
    CHECK(bas::fileSpike(false, 200, true, 100.0, cfg));
    CHECK(!bas::fileSpike(false, 150, true, 100.0, cfg));
    CHECK(bas::fileSpike(true, 150, true, 100.0, cfg));
    CHECK(!bas::fileSpike(false, 900, false, 0.0, cfg));
}
static void aggFacts() {
    bas::Stat st; st.m = bm(240, 90); st.bidRatioOk = true; st.bidRatio = 3.44; st.askRatioOk = true; st.askRatio = 1.06; st.bidSpike = true;
    off::Rec r; r.sweepLv[0] = 4; r.aggrMax[0] = 120; r.aggrMax[1] = 35;
    bas::AggIn in; in.s = &st; in.r = &r; in.let[0] = "SL"; in.p100[0] = 230; in.p100[1] = 80; in.tiersOk = true; in.tiers[0] = 80; in.tiers[1] = 108; in.tiers[2] = 195;
    std::map<std::string, std::string> kv = bas::aggFacts(in);
    const char* keys[] = { "buy_lots", "sell_lots", "buy_x", "sell_x", "spike", "pace_x", "sweep", "big_aggr", "big_tier" };
    CHECK(kv.size() == 9); for (const char* k : keys) CHECK(kv.count(k) == 1);
    EQS(kv["sell_lots"], "240"); EQS(kv["buy_lots"], "90"); EQS(kv["sell_x"], "3.4"); EQS(kv["buy_x"], "1.1");
    EQS(kv["spike"], "sell"); EQS(kv["pace_x"], "2.3"); EQS(kv["sweep"], "sell"); EQS(kv["big_aggr"], "120"); EQS(kv["big_tier"], "S10");
    in.r = nullptr; kv = bas::aggFacts(in);              // order feed not live
    EQS(kv["sweep"], "?"); EQS(kv["big_aggr"], "?"); EQS(kv["big_tier"], "?"); EQS(kv["spike"], "sell");
    // no spike: pace of the drawn (bigger) side; both spiked: the bigger ratio
    bas::Stat n = st; n.bidSpike = false; in.s = &n; in.r = &r; kv = bas::aggFacts(in); EQS(kv["spike"], "none"); EQS(kv["pace_x"], "2.3");
    bas::Stat b2 = st; b2.askSpike = true; b2.askRatio = 4.0; in.s = &b2; kv = bas::aggFacts(in); EQS(kv["spike"], "buy"); EQS(kv["pace_x"], "0.8");
    in.p100[1] = 0; kv = bas::aggFacts(in); EQS(kv["pace_x"], "?");
    bas::AggIn none; kv = bas::aggFacts(none); EQS(kv["buy_lots"], "?"); EQS(kv["spike"], "?");
    const long long T[3] = { 80, 108, 195 };
    EQS(bas::tierOf(79, true, T), "none"); EQS(bas::tierOf(80, true, T), "S20"); EQS(bas::tierOf(108, true, T), "S10"); EQS(bas::tierOf(195, true, T), "S5");
    EQS(bas::tierOf(500, false, T), "?");
    // values never carry the Market State separators
    for (const auto& p : kv) CHECK(p.second.find_first_of(";=|\n") == std::string::npos);
}

// ---------------------------------------------------------------- Book Flow
static off::PaneBar pbar(long long addB, long long pullB, long long addA, long long pullA, bool partial = false) {
    off::PaneBar b; b.ok = true; b.r.events = 10; b.r.partial = partial;
    b.r.addC[0] = addB; b.r.pullC[0] = pullB; b.r.addC[1] = addA; b.r.pullC[1] = pullA; return b;
}
static void bookNetRule() {
    std::vector<long long> pb(20, 100), pa(20, 100);
    off::PaneBar b = pbar(300, 100, 50, 249);           // bid net +200 (2.0x), ask net -199 (1.99x)
    bf::NetStat n = bf::netStat(pb, pa, b.r, 20, 20);
    CHECK(n.ok && n.net[0] == 200 && n.net[1] == -199);
    CHECK(n.spike[0] && !n.spike[1]);
    CHECK(n.ratioOk[0] && n.ratio[0] == 2.0);
    // the audit case: +900 added, -880 pulled = a tiny +20 net -> never bright, whatever the parts did
    off::PaneBar big = pbar(900, 880, 0, 0);
    bf::NetStat m = bf::netStat(pb, pa, big.r, 20, 20);
    CHECK(m.net[0] == 20 && !m.spike[0]);
    // leaving: -250 vs |net| average 100 = 2.5x leaving spike
    bf::NetStat l = bf::netStat(pb, pa, pbar(10, 260, 0, 0).r, 20, 20); CHECK(l.spike[0] && l.net[0] == -250);
    // off / partial / short history
    CHECK(!bf::netStat(pb, pa, b.r, 20, 0).spike[0]);
    CHECK(!bf::netStat(pb, pa, pbar(300, 100, 0, 0, true).r, 20, 20).spike[0]);
    std::vector<long long> s4(4, 100);
    bf::NetStat sh = bf::netStat(s4, s4, b.r, 20, 20); CHECK(!sh.ratioOk[0] && !sh.spike[0]);
    // the mean is floored at 1 lot (quiet book)
    std::vector<long long> zero(20, 0);
    bf::NetStat q = bf::netStat(zero, zero, pbar(2, 0, 1, 0).r, 20, 20); CHECK(q.spike[0] && !q.spike[1]);
    // only the last Ratio bars count: 30 values, the first 10 huge
    std::vector<long long> mix(30, 100); for (int i = 0; i < 10; ++i) mix[(size_t)i] = 100000;
    CHECK(bf::netStat(mix, mix, b.r, 20, 20).spike[0]);
}
static void bookSeries() {
    std::map<long long, off::PaneBar> bars;
    long long t = 1000;
    for (int i = 0; i < 20; ++i) { bars[t] = pbar(150, 50, 50, 150); t += 180; }   // bid net +100, ask net -100
    bars[t] = pbar(5000, 0, 0, 0, true); t += 180;                                   // a partial candle: skipped as a prior value
    off::PaneBar bad; bad.ok = false; bars[t] = bad; t += 180;                         // no order data: skipped
    bars[t] = pbar(260, 50, 0, 0);                                                     // +210 -> 2.1x
    bf::NetStat n = bf::netStatAt(bars, t, 20, 20);
    CHECK(n.ok && n.spike[0] && n.ratio[0] > 2.09 && n.ratio[0] < 2.11);
    CHECK(!n.spike[1]);                                                                 // ask net 0 never spikes
    CHECK(!bf::netStatAt(bars, t + 1, 20, 20).ok);                                      // no such candle
    // no repaint: a candle's decision is the same after later candles exist
    const bf::NetStat before = bf::netStatAt(bars, t, 20, 20);
    bars[t + 180] = pbar(10000, 0, 0, 0); bars[t + 360] = pbar(0, 9000, 0, 0);
    const bf::NetStat after = bf::netStatAt(bars, t, 20, 20);
    CHECK(before.spike[0] == after.spike[0] && before.ratio[0] == after.ratio[0] && before.mean[0] == after.mean[0]);
}
static void bookLook() {
    std::vector<long long> pb(20, 100), pa(20, 100);
    off::PaneBar up = pbar(400, 100, 0, 300); up.r.pace100[0] = 130; up.r.pace100[1] = 250;   // bid +300 arriving, ask -300 leaving
    up.r.hitC[0] = 100; up.r.refC[0] = 430;                                  // (1.5.0) bids: 100 eaten, 430 re-posted at the hit prices -> R4.3
    bf::NetStat n = bf::netStat(pb, pa, up.r, 20, 20);
    bf::NetLook f0 = bf::netLookOf(&up, &n, 0, false);   // forming: drawn dim, no label
    CHECK(!f0.bright && f0.ratio.empty() && f0.pace.empty() && f0.v == 300);
    bf::NetLook c0 = bf::netLookOf(&up, &n, 0, true, "ES");
    CHECK(c0.bright && c0.col == 0); EQS(c0.ratio, "3.0"); EQS(c0.pace, "R2.0");         // (1.5.0) the second label = Refilled vs the ES normal (4.3 / 2.2), not the pace
    CHECK(bf::netLookOf(&up, &n, 0, true).pace.empty());                       // no market: no Refilled label
    bf::NetLook c1 = bf::netLookOf(&up, &n, 1, true);
    CHECK(c1.bright && c1.col == 1 && c1.v == -300); EQS(c1.ratio, "3.0"); EQS(c1.pace, ""); // nothing eaten on the offer side: no Refilled label
    // colour by side, never by meaning: a bid bar below zero is still green (0), an offer bar above zero still red (1)
    off::PaneBar dn = pbar(0, 300, 300, 0); bf::NetStat n2 = bf::netStat(pb, pa, dn.r, 20, 20);
    CHECK(bf::netLookOf(&dn, &n2, 0, true).col == 0 && bf::netLookOf(&dn, &n2, 0, true).v < 0);
    CHECK(bf::netLookOf(&dn, &n2, 1, true).col == 1 && bf::netLookOf(&dn, &n2, 1, true).v > 0);
    // a closed non-spike: dim, no label
    off::PaneBar small = pbar(150, 100, 0, 0); bf::NetStat n3 = bf::netStat(pb, pa, small.r, 20, 20);
    bf::NetLook s0 = bf::netLookOf(&small, &n3, 0, true); CHECK(!s0.bright && s0.ratio.empty());
    // the info box Net cell carries the ratio
    off::Params P; off::Info I = bf::info(&up, P, &n);
    EQS(I.rows[0].label, "Refill Effort"); EQS(I.rows[0].v[0], "+300 (3.0x)"); EQS(I.rows[0].v[1], "-300 (3.0x)");
    off::Info I2 = bf::info(&up, P); EQS(I2.rows[0].v[0], "+300");
    // decide() leaves the drawn spike to the pane (it needs the series)
    off::PaneBar d = up; d.spike[0] = d.spike[1] = true; bf::Logic::decide(d, P, false); CHECK(!d.spike[0] && !d.spike[1]);
}
static void bookFacts() {
    std::vector<long long> pb(20, 100), pa(20, 100);
    off::PaneBar b = pbar(400, 100, 0, 300); b.r.hitC[0] = 100; b.r.hitC[1] = 100; b.r.rebuildC[0] = 30; b.r.rebuildC[1] = 20;
    bf::NetStat n = bf::netStat(pb, pa, b.r, 20, 20); n.ratio[1] = 2.5;   // both spiked: the bigger ratio names the spike
    std::map<std::string, std::string> kv = bf::bookFacts(b, n);
    const char* keys[] = { "bid_net", "ask_net", "bid_net_x", "ask_net_x", "bid_state", "ask_state", "spike", "rebuilt_pct" };
    CHECK(kv.size() == 8); for (const char* k : keys) CHECK(kv.count(k) == 1);
    EQS(kv["bid_net"], "300"); EQS(kv["ask_net"], "-300"); EQS(kv["bid_net_x"], "3.0"); EQS(kv["ask_net_x"], "2.5");
    EQS(kv["bid_state"], "arriving"); EQS(kv["ask_state"], "leaving"); EQS(kv["spike"], "bid_arriving"); EQS(kv["rebuilt_pct"], "25");
    off::PaneBar f = pbar(100, 100, 0, 50); bf::NetStat nf = bf::netStat(pb, pa, f.r, 20, 20);
    kv = bf::bookFacts(f, nf); EQS(kv["bid_state"], "flat"); EQS(kv["spike"], "none"); EQS(kv["rebuilt_pct"], "?");
    off::PaneBar l = pbar(0, 0, 0, 400); bf::NetStat nl2 = bf::netStat(pb, pa, l.r, 20, 20);
    kv = bf::bookFacts(l, nl2); EQS(kv["spike"], "ask_leaving");
    bf::NetStat none; kv = bf::bookFacts(l, none); EQS(kv["bid_net"], "?"); EQS(kv["spike"], "?"); CHECK(kv.size() == 8);
}

// ---------------------------------------------------------------- Market State round trip (Linux fake environment)
static void marketState() {
    bas::Stat st; st.m = bm(240, 90); st.bidRatioOk = true; st.bidRatio = 3.44; st.bidSpike = true;
    bas::AggIn in; in.s = &st; const std::map<std::string, std::string> agg = bas::aggFacts(in);
    mst::publish("AGG", "ESZ6", 180, "ES 3 min", 100000, 1792300000, agg);
    std::vector<long long> pb(20, 100), pa(20, 100); off::PaneBar b = pbar(400, 100, 0, 300);
    const std::map<std::string, std::string> book = bf::bookFacts(b, bf::netStat(pb, pa, b.r, 20, 20));
    mst::publish("BOOK", "ESZ6", 180, "ES 3 min", 100000, 1792300000, book);
    mst::Facts A = mst::read("AGG", "ESZ6", 180, "ES 3 min", 110000), B = mst::read("BOOK", "ESZ6", 180, "ES 3 min", 110000);
    CHECK(A.ok && A.kv == agg && A.bar == 1792300000);
    CHECK(B.ok && B.kv == book);
    CHECK(mst::getD(A, "sell_x") == 3.4);
    CHECK(!mst::read("AGG", "ESZ6", 180, "ES 3 min", 100000 + mst::FRESH_MS + 1).ok);   // why the plugins re-post every 15 s
    CHECK(!mst::read("AGG", "ESZ6", 180, "NQ 3 min", 110000).ok);                         // another chart: not visible
    mst::clear("AGG", "ESZ6", 180, "ES 3 min");
    CHECK(!mst::read("AGG", "ESZ6", 180, "ES 3 min", 110000).ok);
    CHECK(mst::readAll("ESZ6", 180, "ES 3 min", 110000).count("BOOK") == 1);
    mst::clear("BOOK", "ESZ6", 180, "ES 3 min");
}

// ---------------------------------------------------------------- version literals (BUILD-LESSONS 70)
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream b; b << f.rdbuf(); return b.str(); }
static std::string between(const std::string& s, const std::string& pre) { const size_t a = s.find(pre); if (a == std::string::npos) return "?"; const size_t b = s.find('"', a + pre.size()); return b == std::string::npos ? "?" : s.substr(a + pre.size(), b - a - pre.size()); }
static void versions(const std::string& dir) {
    const std::string a = slurp(dir + "/BidAskStats.cpp"), b = slurp(dir + "/BookFlow.cpp");
    // (2026-10-18 settings layer) the window rows live in OrderFlowPaneSettings.h (one table per pane; registered by SettingsHost.h)
    const std::string t = slurp(dir + "/OrderFlowPaneSettings.h");
    const std::string ta = t.substr(0, t.find("bookFlowSpec")), tb = t.substr(t.find("bookFlowSpec"));
    EQS(between(a, "#define BAS_VERSION \""), bas::VERSION); EQS(between(a, "p->setVersion(\""), bas::VERSION); EQS(bas::VERSION, "1.30.0"); EQS(ofset::BAS_VERSION, bas::VERSION);   // (1.13.2 settings layer; 1.14.0 Big Lot mark)
    EQS(between(b, "#define BF_VERSION \""), bf::VERSION); EQS(between(b, "p->setVersion(\""), bf::VERSION); EQS(bf::VERSION, "1.25.0"); EQS(ofset::BF_VERSION, bf::VERSION);
    // every dialog row the chart reads; the removed rows are gone
    CHECK(ta.find("\"Spike vs prior biggest\"") == std::string::npos && ta.find("\"Box shows\"") == std::string::npos && ta.find("\"Spike lookback\"") == std::string::npos && ta.find("\"Spike vs average\"") != std::string::npos);
    CHECK(tb.find("\"Spike vs prior biggest\"") == std::string::npos && tb.find("\"Box shows\"") == std::string::npos && tb.find("\"Spike lookback\"") == std::string::npos && tb.find("\"Spike vs average\"") != std::string::npos);
    CHECK(ofset::bidAskSpec().paramVersion == 6 && ofset::bookFlowSpec().paramVersion == 4 && a.find("ss::registerAll(this, ofset::bidAskSpec()") != std::string::npos && b.find("ss::registerAll(this, ofset::bookFlowSpec()") != std::string::npos);
    {   const ss::Spec& A = ofset::bidAskSpec(); const int k = ss::find(A, "big_lot");   // (1.14.0) the Big Lot mark row: Off / BL5 / BL10 / BL20, default BL5, on the Position line
        CHECK(k == A.n - 1 && A.rows[k].type == ss::T_LIST && A.rows[k].def == 1 && A.rows[k].hi == 3 && A.rows[k].same && !A.rows[k].hidden && ss::check(A).empty() && ss::visibleLines(A) == 1);
        CHECK(a.find("K_BIG_LOT") != std::string::npos); }
    CHECK(a.find("mst::publish(\"AGG\"") != std::string::npos && a.find("mst::clear(\"AGG\"") != std::string::npos);
    CHECK(b.find("mst::publish(\"BOOK\"") != std::string::npos && b.find("mst::clear(\"BOOK\"") != std::string::npos);
    // the descriptions no longer promise what is not drawn
    for (const char* bad : { "white tick", "p =", "Letters from", "read line" }) { CHECK(a.find(std::string("setDescription(\"LRA Aggression")) != std::string::npos);
        const size_t da = a.find("setDescription("), db = b.find("setDescription(");
        CHECK(a.substr(da, a.find('\n', da) - da).find(bad) == std::string::npos); CHECK(b.substr(db, b.find('\n', db) - db).find(bad) == std::string::npos); }
}

// (1.14.2) BBI / BAI: consecutive PRIOR candles strictly smaller, an equal or bigger one stops it, cap 50+, fewer prior = what is there
static void aggBeats() {
    CHECK(bas::beatsOf({}, 100) == 0);                                   // the first candle: no prior candle
    CHECK(bas::beatsOf({ 10, 20, 30 }, 100) == 3);                       // fewer than 50 prior: all 3 beaten
    CHECK(bas::beatsOf({ 10, 100, 20, 30 }, 100) == 2);                  // an EQUAL value stops the run
    CHECK(bas::beatsOf({ 500, 20, 30 }, 100) == 2);                      // a bigger one stops it
    CHECK(bas::beatsOf({ 10, 20, 300 }, 100) == 0);                      // the previous candle bigger: 0
    std::vector<long long> many(80, 5);
    bas::Stat s = bas::statOf(bm(100, 5), many, many, 20);
    EQS(bas::beatText(s.bBid, s.bBidCap, s.m.ok), "50+");                // 80 beaten -> capped at 50+
    EQS(bas::beatText(s.bAsk, s.bAskCap, s.m.ok), "0");                  // 5 vs prior 5s: equal stops at once
    std::vector<long long> fifty(50, 5), fortynine(49, 5);
    EQS(bas::beatText(bas::statOf(bm(100, 5), fifty, fifty, 20).bBid, false, true), "50+");
    EQS(bas::beatText(bas::statOf(bm(100, 5), fortynine, fortynine, 20).bBid, false, true), "49");
    EQS(bas::beatText(0, false, false), "n/a");
}
// (2026-10-22 evening fix build) the squashed bars, the compact strips, the one-line Implied row
static void fixBuild() {
    const int H[3] = { 80, 100, 150 };
    for (int k = 0; k < 3; ++k) {
        const int h = H[k];
        const int reserve = off::textReserve(h, off::MODE_NONE) + off::QUESTION_H + 2 * lst::ROW_H + 2;   // lsBidAskStats' reserve (question + 2 strips)
        const int px = off::pairPx(h, reserve);
        CHECK(px * 2 >= h && px * 10 <= h * 9);                                     // the tallest bar = 50 - 90% of the pane
        double mn = 0, mx = 0; off::axisFor(800, h, px, mn, mx);
        CHECK(mx >= 800 / 0.9 - 1 && mx <= 800 / 0.5 + 1);                          // axis top ~ the tallest bar / (0.5 - 0.9): never 20,000 vs 800-lot bars
    }
    CHECK(off::pairPx(100, 90) >= 50);                                              // even a huge reserve keeps half the pane
    CHECK(lst::ROW_H == 11 && lst::SUB_ROWS == 1 && std::string(lst::VERSION).compare(0, 5, "1.1.0") == 0);
    imp::Params P; P.fromFile = true; off::InfoRow r;
    CHECK(off::impliedRow(P, 1000, 0, r) && r.wide && r.label == "Implied: 0% of volume" && r.v[0].empty() && r.v[1].empty());
    off::Info I; I.twoCol = true; I.colHead[0] = "Buyers"; I.colHead[1] = "Sellers";
    off::InfoRow a; a.label = "Lots"; a.v[0] = "120 (1.0x)"; a.v[1] = "80 (0.7x)"; I.rows.push_back(a); I.rows.push_back(r);
    const off::TextW M = [](const std::string& s, int, bool) { return (int)s.size() * 7; };
    const off::NumGeom n1 = off::numGeom(I, 400, M);
    off::Info J = I; J.rows.pop_back(); const off::NumGeom n0 = off::numGeom(J, 400, M);
    CHECK(n1.lw == n0.lw && n1.vw == n0.vw);                                        // the wide row never widens the label / value columns
    CHECK(n1.W >= (int)r.label.size() * 7 + 2 * off::BOX_PAD);                      // ... and the box fits it on one line
}
int main(int argc, char** argv) {
    fixBuild();
    aggBeats(); aggSpikeRule(); aggNoRepaint(); aggLook(); aggFacts();
    bookNetRule(); bookSeries(); bookLook(); bookFacts();
    marketState();
    if (argc > 1) versions(argv[1]); else std::printf("(version check skipped: no plugin dir given)\n");
    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
