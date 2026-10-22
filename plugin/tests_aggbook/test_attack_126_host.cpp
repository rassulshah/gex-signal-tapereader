// test_attack_126_host.cpp -- lsBidAskStats 1.26.0 end to end on the mock host (2026-10-21, Rassul 20:11-20:23 CT, attacker-mockup-v2.png):
//   the order-feed candles decided at the close -> AS / K / S lines; K at a 5-candle high (red) and at a low (green), the initiative veto, L;
//   G / Tr read from lsBookFlow's turns file and the Delta Profile's TR record (moved from the Defense pane); R from the stop run; one badge per
//   candle (at most 4 letters, the 5th in the hover); the badge hover (every letter, the combination with the Defense A, inside the pane, off
//   the info box); "Who is aggressive?"; no "?"; restart = the same letters from the record; later candles never change them (no repaint).
//   g++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation -DBAS_TEST_CLOCK -DPC_TEST_CURSOR -I.. -Imock test_attack_126_host.cpp
#include "BidAskStats.cpp"
typedef BidAskStats Ind;
long long bas_test_now_ms = 1000000;
void* bas_test_cursor_wnd = nullptr;
static long long& NOW = bas_test_now_ms;
#include <cstdio>
#include <unistd.h>
namespace mock { Host* current = nullptr; Feed feed; }
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void dispose(Ind* p) { p->destroy(); p->~Ind(); ::operator delete(p); }
static Ind* make() { Ind* p = static_cast<Ind*>(CreateExtension()); static bool once = false; if (!once) { once = true; p->setup(); } return p; }
static RTX_EVENT moveAt(int h, int v, long bar) { RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)h; e.v.mouse.v = (short)v; e.v.mouse.bar = bar; return e; }
static const unsigned long T0 = 1791900000UL;                               // 14:00 UTC (the mock's local clock): RTH
static mock::Bar bar(unsigned long t, float o, float hi, float lo, float c) {
    mock::Bar b; b.time = t; b.open = o; b.high = hi; b.low = lo; b.close = c;
    b.rows.push_back(mock::Price(c, 10, 10, 20)); b.volume = 20; return b;
}
static unsigned long keyOf(int k) { return T0 + 60UL * (unsigned long)k; }
// one aggressor order on RESTING side `rs` (ASK = buyers lifting offers, BID = sellers hitting bids)
static off::Aggr order(int rs, long long size, int prices, off::Tick lo, long long t0) {
    off::Aggr g; g.side = rs; g.size = size; g.t0 = t0; g.lo = lo; g.hi = lo + prices - 1;
    for (int k = 0; k < prices && k < off::AGGR_PX_MAX; ++k) g.px[k] = lo + k;
    g.nPx = std::min(prices, off::AGGR_PX_MAX); g.more = prices > off::AGGR_PX_MAX; return g;
}
static std::vector<std::string> linesWith(const std::string& text, const std::string& tag) {
    std::vector<std::string> v; std::stringstream ss(text); std::string ln; while (std::getline(ss, ln)) if (ln.compare(0, tag.size(), tag) == 0) v.push_back(ln); return v;
}
int main() {
    char tmpl[] = "/tmp/atk126XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 60; h.symbol = "ESZ6"; h.root = "ES"; h.paneBottom = 420; h.paneRight = 1400; h.scaleLeft = 1380; h.ppb = 20; h.barW = 20; h.listIndex = 0;
    for (int k = 0; k <= 15; ++k) h.bars.push_back(bar(keyOf(k), 7000.0f, 7000.5f, 6999.5f, 7000.0f));
    Ind* p = make(); p->calc(0); p->draw(); p->calc(0);
    BASState* S = p->testState();
    CHECK(S && std::string(bas::QUESTION) == "Who is aggressive?" && std::string(bas::VERSION) == "1.26.0" && std::string(BAS_VERSION) == "1.26.0", "lsBidAskStats 1.26.0, the question 'Who is aggressive?'");
    // the Big Lot pool (S20 / S10 / S5): 3 past RTH sessions and 3 overnight ones of a different size
    {   llp::LogDist d; for (int k = 0; k < 200; ++k) d.addN(20, 1); for (int k = 0; k < 20; ++k) d.addN(100, 1); for (int k = 0; k < 10; ++k) d.addN(150, 1); for (int k = 0; k < 5; ++k) d.addN(220, 1);
        llp::LogDist n; for (int k = 0; k < 200; ++k) n.addN(15, 1); for (int k = 0; k < 20; ++k) n.addN(40, 1); for (int k = 0; k < 10; ++k) n.addN(60, 1); for (int k = 0; k < 5; ++k) n.addN(90, 1);
        for (long long s = 1; s <= 3; ++s) { S->e.lp.past[llp::CH_RTH][s] = d; S->e.lp.past[llp::CH_ON][s] = n; llp::LogDist a = d; a.add(n); S->e.lp.past[llp::CH_ALL][s] = a; } }
    const llp::Tiers TR = S->e.lp.tiersFor(llp::CH_RTH), TN = S->e.lp.tiersFor(llp::CH_ON);
    CHECK(TR.ok && TN.ok && TR.channel == llp::CH_RTH && TR.t[1] > TN.t[1] && TR.t[0] < TR.t[1], ("tiers RTH S20 / S10 / S5 " + std::to_string(TR.t[0]) + " / " + std::to_string(TR.t[1]) + " / " + std::to_string(TR.t[2]) + ", overnight S10 " + std::to_string(TN.t[1])).c_str());
    const long long S10 = TR.t[1], S20 = TR.t[0];
    S->firstBarKey = (long long)keyOf(15);
    // candles 15..: each closes with its order-feed accumulator. X = 30 (a 5-candle high: S + K), Y = 32 (an initiative high: no K),
    // Z = 34 (a 5-candle low: K green), W = 36 (an L order, no extreme)
    const int X = 30, Y = 32, Z = 34, W = 36;
    auto accFor = [&](int k) {
        off::Acc a; const long long kt = (long long)keyOf(k), st = kt - 60;
        if (k == X) {
            a.aggr[7] = order(off::ASK, S10 + 51, 39, 28004, st + 18);      // one buy order through 39 prices (> 24: the span)
            a.aggr[8] = order(off::BID, 30, 1, 28010, st + 50);
            a.tFirst = st + 1; a.tHi = st + 18; a.tLast = st + 59; a.hitC[off::ASK] = S10 + 51 + 10; a.cumHi[off::ASK] = S10 + 51; a.hitC[off::BID] = 30;
            a.hitSec[off::ASK][st + 5] = 1; a.hitSec[off::ASK][st + 18] = S10 + 50;
        } else if (k == Y) {
            a.aggr[9] = order(off::ASK, S10 + 5, 2, 28016, st + 30); a.aggr[10] = order(off::ASK, 400, 1, 28016, st + 40);   // + 400 more lots bought: initiative
        } else if (k == Z) {
            a.aggr[11] = order(off::BID, S10 + 3, 2, 27980, st + 20); a.aggr[12] = order(off::ASK, 60, 1, 27990, st + 40);
        } else if (k == W) {
            a.aggr[13] = order(off::BID, S20, 1, 27996, st + 33);
        } else {
            a.aggr[1] = order(off::ASK, k == 22 ? 60 : 20, 3, 28000, st + 10);   // the leg's buy sweeps (biggest 60 lots on candle 22)
            a.aggr[2] = order(off::BID, 10, 1, 27999, st + 20);
        }
        return a;
    };
    auto barFor = [&](int k) {
        if (k == X) return bar(keyOf(k), 7001.0f, 7003.0f, 7000.0f, 7001.5f);   // the high; closed back down (not initiative)
        if (k == X + 1) return bar(keyOf(k), 7001.5f, 7002.0f, 7000.5f, 7001.0f);
        if (k == Y) return bar(keyOf(k), 7001.0f, 7004.0f, 7000.75f, 7004.0f);  // a new high, closed at the top: initiative
        if (k == Y + 1) return bar(keyOf(k), 7003.0f, 7003.5f, 6999.0f, 6999.5f);
        if (k == Z) return bar(keyOf(k), 6999.5f, 7000.0f, 6995.0f, 6997.5f);   // the low; closed in the middle
        return bar(keyOf(k), 7000.0f, 7000.5f, 6999.5f, 7000.0f);
    };
    for (int k = 15; k <= 40; ++k) {
        S->accs[keyOf(k)] = accFor(k);
        h.bars.back() = barFor(k);
        h.bars.push_back(barFor(k + 1)); NOW += 100; p->calc(0); NOW += 600; p->calc(0);
    }
    const long long kX = (long long)keyOf(X), kY = (long long)keyOf(Y), kZ = (long long)keyOf(Z), kW = (long long)keyOf(W);
    CHECK(S->atkAS.size() >= 20 && !S->atkAS.count((long long)keyOf(15)), "every complete order-feed candle has its AS line (the first, partial one has none)");
    {   auto k = S->atkK.find(std::make_pair((int)atk::HIGH, kX)); auto w = S->atkS.find(std::make_pair((int)atk::HIGH, kX));
        CHECK(k != S->atkK.end() && k->second.K && k->second.lots == S10 + 51 && k->second.tier[1] == S10 && k->second.ch == llp::CH_RTH && !k->second.init, "X: K at the 5-candle high (one buy order >= RTH S10, frozen tier, RTH channel)");
        CHECK(w != S->atkS.end() && w->second.S && w->second.nPx == 39 && w->second.prior == 60 && w->second.lots == S10 + 51, "X: S - the 39-price buy sweep beats the leg's biggest (60 lots)");
        CHECK(k != S->atkK.end() && k->second.p.e100 >= 0 && k->second.p.e100 <= 50, "X: the attack faded after the high (E, hover only)"); }
    {   auto k = S->atkK.find(std::make_pair((int)atk::HIGH, kY));
        CHECK(k != S->atkK.end() && !k->second.K && k->second.init && k->second.lots >= S10, "Y: an initiative candle at a high - no K (veto), its line kept for the nightly"); }
    {   auto k = S->atkK.find(std::make_pair((int)atk::LOW, kZ));
        CHECK(k != S->atkK.end() && k->second.K, "Z: K at the 5-candle low (one sell order >= S10)");
        CHECK(!S->atkK.count(std::make_pair((int)atk::LOW, kW)) && !S->atkK.count(std::make_pair((int)atk::HIGH, kW)) && atk::lOf(S->atkAS[kW], off::BID), "W: no extreme, one S20 sell order: L"); }
    // ---- the other panes' records: G + the Defense A on X (lsBookFlow's turns file), Tr on X (the Delta Profile's TR record); R on X
    {   dt::GiveUp g; g.t = kX; g.side = dt::HIGH; g.G = true; g.pull = 19226; g.add = 18673; g.mov = 0; g.secs = 41; g.x = 3.6; g.pace = 0.2; g.normal = 17.82;
        dt::Turn t; t.t = kX; t.side = dt::HIGH; t.A = true; t.pivotPx = 28012; t.linePx = 28010; t.pivotT = kX;
        std::ofstream f((flex + "\\BookFlow\\ES-60-turns.csv").c_str(), std::ios::binary); f << dt::lineOf('T', t, 0.25) << "\n" << dt::gLineOf(g) << "\n"; }
    {   struct tm x; time_t v = (time_t)S->formingKey; gmtime_r(&v, &x);
        const long long sess = off::civilDays(x.tm_year + 1900, (unsigned)x.tm_mon + 1, (unsigned)x.tm_mday) + (x.tm_hour >= 17 ? 1 : 0);
        trapl::Rec tr; tr.id = 0; tr.top = true; tr.barT = 5; tr.barRaw = (unsigned long long)kX; tr.lo = 28008; tr.hi = 28012; tr.tick = 0.25; tr.lots = 640; tr.med = 220; tr.x = 2.9; tr.version = "2.21.1"; tr.root = "ESZ6";
        std::ofstream f((flex + "\\DeltaProfile\\ES-ESZ6-60-signals-" + std::to_string(sess) + ".csv").c_str(), std::ios::binary); f << "D|x\n" << trapl::line(tr) << "\n"; }
    { sr::Event e; e.breakT = kX; e.reclaimT = (long long)keyOf(X + 1); e.dir = 1; e.level = "PDH"; e.ticks = 6; e.bars = 1; e.spike = true; e.ratioOk = true; e.ratio = 3.1; e.show = sr::SHOW_Q; S->srEv[kX].push_back(e); }
    NOW += 3500; p->calc(0);
    CHECK(S->atkO.G.count(std::make_pair((int)dt::HIGH, kX)) && S->atkO.Tr.count(std::make_pair((int)dt::HIGH, kX)) && S->atkO.D.A.count(std::make_pair((int)dt::HIGH, kX)), "G + the Defense A read from lsBookFlow's turns file, Tr from the Delta Profile's record");
    h.draws.clear(); p->draw();
    auto badgeText = [&](long long key, std::vector<std::pair<std::string, COLOR> >* parts = nullptr) {
        std::string b; const int xc = 50 + (int)((key - (long long)T0) / 60) * h.barW;
        for (size_t q = 0; q < S->badgeHit.size(); ++q) { const off::IconRect& r = S->badgeHit[q].first; if (xc < r.l - 12 || xc > r.r + 12) continue;
            for (auto& d : h.draws) if (d.type == "text" && d.x >= r.l - 1 && d.x <= r.r && d.y >= r.t - 3 && d.y <= r.b) { b += d.text; if (parts) parts->push_back(std::make_pair(d.text, d.color)); } break; }
        return b; };
    std::vector<std::pair<std::string, COLOR> > px;
    const std::string bx = badgeText(kX, &px);
    CHECK(bx == "S|K|G|Tr", ("X badge: the 4 most important letters (S, K, G, Tr; R to the hover): " + bx).c_str());
    {   bool red = !px.empty(); for (auto& q : px) red = red && q.second == C_BID; CHECK(red, "X: buyers attacking / giving up / trapped at a high - every letter red (bearish)"); }
    {   std::vector<std::pair<std::string, COLOR> > pz; const std::string bz = badgeText(kZ, &pz);
        CHECK(bz == "K" && pz.size() == 1 && pz[0].second == C_ASK, ("Z: K green (sellers attacking a low = bullish): " + bz).c_str()); }
    {   std::vector<std::pair<std::string, COLOR> > pw; const std::string bw = badgeText(kW, &pw);
        CHECK(bw == "L" && pw.size() == 1 && pw[0].second == C_BID, ("W: L red (a seller, not at a low): " + bw).c_str()); }
    CHECK(badgeText(kY).empty(), "Y: the initiative candle has no K badge");
    {   bool q = false; for (auto& d : h.draws) if (d.type == "text" && d.text.find('?') != std::string::npos && d.text != bas::QUESTION) q = true;
        CHECK(!q, "no '?' anywhere on the pane (letters, message / read line) except the question itself"); }
    {   const off::IconRect* r = nullptr; for (auto& b : S->badgeHit) if (50 + X * h.barW >= b.first.l - 12 && 50 + X * h.barW <= b.first.r + 12) r = &b.first;
        CHECK(r && r->t >= off::QUESTION_H && r->b <= r->t + 16, ("the badge row sits under the question line (" + std::to_string(r ? r->t : -1) + ")").c_str());
        int tallest = 1 << 30; for (auto& d : h.draws) if (d.type == "box" && (d.color == C_ASK_BAR || d.color == C_BID_BAR || d.color == C_ASK_SPIKE || d.color == C_BID_SPIKE)) tallest = std::min(tallest, d.y);
        CHECK(r && tallest > r->b, ("the bars stay below the badge row (top bar y " + std::to_string(tallest) + ")").c_str()); }
    // ---- the hover on X: every letter (R cut from the badge), the numbers, the combination with the Defense A, inside the pane, off the box
    {   const off::IconRect* r = nullptr; for (auto& b : S->badgeHit) if (50 + X * h.barW >= b.first.l - 12 && 50 + X * h.barW <= b.first.r + 12) r = &b.first;
        CHECK(r != nullptr, "the X badge's rectangle kept for the mouse");
        if (r) {
            bas_test_cursor_wnd = (void*)0x1; pc::test_ok = true;
            const off::IconRect rr = *r; pc::test_cx = (rr.l + rr.r) / 2 + 1000; pc::test_cy = (rr.t + rr.b) / 2 + 500;   // the screen cursor = the event + the pane's screen offset
            RTX_EVENT ev = moveAt((rr.l + rr.r) / 2, (rr.t + rr.b) / 2, X); p->mouse(&ev);
            h.draws.clear(); p->draw();
            if (const char* dp = std::getenv("ATK_DUMP")) { std::ofstream o(dp); for (auto& d : h.draws) o << d.type << '\t' << d.x << '\t' << d.y << '\t' << d.r << '\t' << d.b << '\t' << d.color << '\t' << d.border << '\t' << d.fsize << '\t' << d.fstyle << '\t' << d.text << '\n'; }   // render check (not a test)
            CHECK(S->badgeTip >= 0 && S->lastTip.r > S->lastTip.l, ("mouse on the badge: the hover drawn (level " + std::to_string(S->lastTipLevel) + ")").c_str());
            std::string joined; for (auto& d : h.draws) if (d.type == "text" && d.x >= S->lastTip.l && d.r <= S->lastTip.r + 1 && d.y >= S->lastTip.t && d.y <= S->lastTip.b) {
                if (d.text.compare(0, 5, "     ") == 0) joined += " " + d.text.substr(5); else joined += "\n" + d.text; }
            auto has = [&](const std::string& t) { return joined.find(t) != std::string::npos; };
            CHECK(has("Attack S|K|G|Tr") && has("(+ R: no room on the badge)"), "hover header: the badge and the letter cut from it");
            CHECK(has("K  Big aggressor - did one big buyer hit the high?") && has("One buy order of " + dt::lotsText(S10 + 51) + " lots at 2:"), "hover K: the question, the lots and the second it hit");
            CHECK(has("S  Sweep of the leg") && has("took 39 prices") && has("the biggest sweep of the 10 candles before (60 lots)"), "hover S: prices and the leg");
            CHECK(has("G  Giving up") && has("19,226 pulled vs 18,673 added"), "hover G: lsBookFlow's G line numbers");
            CHECK(has("Tr  Trapped") && has("640 lots"), "hover Tr: the Delta Profile record");
            CHECK(has("R  Stop run - did buyers get trapped above PDH?"), "hover R (cut from the badge): its own block");
            CHECK(has("with Defense A: K + A held 61% vs 47% (n 49, learning)"), "the combination line with the Defense A on the same candle");
            CHECK(S->lastTipLevel >= 1 || has("Status: still learning, n = "), "status still learning, n");
            const bp::Box& b = S->lastBox; const dt::TipRect& t = S->lastTip;
            CHECK(t.b <= h.paneBottom - 2 && t.r <= h.scaleLeft && t.l >= 0, "the hover fits inside the pane, left of the price axis");
            CHECK(b.r > b.l && !(t.l < b.r + 4 && t.r + 4 > b.l && t.t < b.b + 4 && t.b + 4 > b.t), "the hover never covers the info box");
            {   const int pb0 = h.paneBottom; h.paneBottom = 200; h.draws.clear(); p->draw();
                CHECK(S->lastTip.r > S->lastTip.l ? (S->lastTip.b <= 198 && !(S->lastTip.l < S->lastBox.r + 4 && S->lastTip.r + 4 > S->lastBox.l && S->lastTip.t < S->lastBox.b + 4 && S->lastTip.b + 4 > S->lastBox.t)) : S->badgeTipNoRoom > 0,
                      ("200 px pane: the hover fits (level " + std::to_string(S->lastTipLevel) + ") or is counted as no room").c_str());
                h.paneBottom = pb0; h.draws.clear(); p->draw(); }
            pc::test_cy = 2000; NOW += 300; p->calc(0); h.draws.clear(); p->draw();
            CHECK(S->badgeTip < 0 && S->lastTip.r <= S->lastTip.l, "the cursor leaves the pane: the hover goes");
            pc::test_ok = false; bas_test_cursor_wnd = nullptr;
        }
    }
    // ---- the record: K / S / AS lines in the session file (for the nightly)
    const std::string rec = readAll(S->file.path);
    {   const std::vector<std::string> ks = linesWith(rec, "K|"), ss = linesWith(rec, "S|"), as = linesWith(rec, "AS|");
        CHECK(!ks.empty() && !ss.empty() && as.size() == S->atkAS.size(), ("the session record keeps " + std::to_string(ks.size()) + " K, " + std::to_string(ss.size()) + " S, " + std::to_string(as.size()) + " AS lines").c_str());
        bool y = false; for (auto& l : ks) y = y || l.compare(0, std::to_string(kY).size() + 4, "K|" + std::to_string(kY) + "|1") == 0;
        CHECK(y, "the initiative candle's K line is recorded (K 0, initiative 1)"); }
    // ---- no repaint: later candles never change X; a restart reads the same letters back
    for (int k = 41; k <= 44; ++k) { S->accs[keyOf(k)] = accFor(k); h.bars.push_back(barFor(k + 1)); NOW += 100; p->calc(0); NOW += 600; p->calc(0); }
    h.draws.clear(); p->draw();
    CHECK(badgeText(kX) == "S|K|G|Tr" && badgeText(kZ) == "K", "later candles: X and Z unchanged (decided once)");
    const std::string rec1 = readAll(S->file.path);
    const size_t nK = linesWith(rec1, "K|").size(), nS = linesWith(rec1, "S|").size(), nA = linesWith(rec1, "AS|").size();
    dispose(p);
    Ind* p2 = make(); p2->calc(0); p2->draw(); NOW += 3500; p2->calc(0);
    BASState* S2 = p2->testState();
    h.draws.clear(); p2->draw();
    S = S2;
    CHECK(S2->atkLoaded == (long long)(nK + nS + nA) && S2->atkRejected == 0, ("restart: every K / S / AS line read back (" + std::to_string(S2->atkLoaded) + ")").c_str());
    CHECK(badgeText(kX) == "S|K|G|Tr" && badgeText(kZ) == "K" && badgeText(kW) == "L", "restart: the same badges (from the record, G / Tr from the other panes' records)");
    NOW += 1000; p2->calc(0);
    const std::string rec2 = readAll(S2->file.path);
    CHECK(linesWith(rec2, "K|") == linesWith(rec1, "K|") && linesWith(rec2, "S|") == linesWith(rec1, "S|") && linesWith(rec2, "AS|") == linesWith(rec1, "AS|"), "restart: nothing decided again, nothing rewritten");
    {   std::ofstream f(S2->file.path.c_str(), std::ios::binary | std::ios::app); f << "K|garbage\n"; }
    const size_t kN2 = S2->atkK.size();
    dispose(p2);
    Ind* p3 = make(); p3->calc(0); p3->draw();
    CHECK(p3->testState()->atkRejected == 1 && p3->testState()->atkK.size() == kN2, "a garbled K line is counted and skipped; the rest load");
    dispose(p3);
    std::printf("Attack host 1.26.0: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
