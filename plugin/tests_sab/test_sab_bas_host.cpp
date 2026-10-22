// test_sab_bas_host.cpp -- lsBidAskStats 1.27.0 SHARED A end to end on the mock host (2026-10-22, Rassul 06:43 "then do it", 06:49 "then
// require it"): the chart's own candles (VAP) -> the shared A decided once at the close (5-candle low, sellers >= E x their 20-candle average,
// GREEN close) -> one SAB line in lsFlexLevels\SharedAbsorb\SAB-ES-60.csv -> the attacker badge "A" in green with the shared hover; an
// attacker-colour (red) candle with the same effort never gets an A; LRA-Badges-ES.csv overrides E; a restart reads the record back and
// writes nothing new; a line another plugin wrote first is adopted (never re-decided).
//   g++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation -DBAS_TEST_CLOCK -DPC_TEST_CURSOR -I.. -I../tests_aggbook/mock test_sab_bas_host.cpp
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
static const unsigned long T0 = 1791900000UL;
static unsigned long keyOf(int k) { return T0 + 60UL * (unsigned long)k; }
static const int KA = 25, KR = 31, KH = 37;                 // KA: bullish A (green at a low); KR: same effort, RED at a low (no A); KH: bearish A at a high
static mock::Bar barFor(int k) {
    mock::Bar b; b.time = keyOf(k);
    float o = 7100.0f - (float)k * 2.0f, c = o - 1.0f, hi = o + 0.25f, lo = c - 0.25f; long buy = 100, sell = 100;
    if (k == KA) { lo = 7100.0f - 24 * 2.0f - 1.0f - 3.0f; o = lo + 0.5f; c = lo + 2.0f; hi = c + 0.25f; sell = 400; }      // 4.0x sellers, green close at a new low
    if (k > KA && k < KR) { o = 7046.0f + (float)(k - KA); c = o + 0.5f; hi = c + 0.25f; lo = o - 0.25f; }                  // a small bounce
    if (k == KR) { hi = 7050.0f; o = 7049.0f; c = 7040.0f; lo = 7039.0f; sell = 900; }                                        // a new low, sellers heavy, RED close
    if (k > KR && k < KH) { o = 7040.0f + (float)(k - KR) * 2.0f; c = o + 1.5f; hi = c + 0.25f; lo = o - 0.25f; }            // up into a high
    if (k == KH) { lo = 7051.0f; o = 7058.0f; hi = 7060.0f; c = 7052.0f; buy = 800; }                                          // buyers 8x at a new high, RED close
    if (k > KH) { o = 7052.0f - (float)(k - KH); c = o - 0.5f; hi = o + 0.25f; lo = c - 0.25f; }
    b.open = o; b.high = hi; b.low = lo; b.close = c;
    b.rows.push_back(mock::Price(c, buy, sell, buy + sell)); b.volume = (unsigned long)(buy + sell); return b;
}
int main() {
    char tmpl[] = "/tmp/sabbasXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 60; h.symbol = "ESZ6"; h.root = "ES"; h.paneBottom = 420; h.paneRight = 1400; h.scaleLeft = 1380; h.ppb = 20; h.barW = 20; h.listIndex = 0;
    for (int k = 0; k <= 45; ++k) h.bars.push_back(barFor(k));
    Ind* p = make(); for (int r = 0; r < 4; ++r) { NOW += 1500; p->calc(0); p->draw(); }
    BASState* S = p->testState();
    CHECK(S && std::string(BAS_VERSION) == "1.27.0" && std::string(sab::VERSION) == "1.0.0", "lsBidAskStats 1.27.0 with SharedAbsorb 1.0.0");
    const std::string recPath = flex + "\\SharedAbsorb\\SAB-ES-60.csv";
    std::string rec = readAll(recPath);
    std::vector<std::string> lines; { std::stringstream ss(rec); std::string ln; while (std::getline(ss, ln)) if (ln.compare(0, 4, "SAB|") == 0) lines.push_back(ln); }
    const long long kA = (long long)keyOf(KA), kR = (long long)keyOf(KR), kH = (long long)keyOf(KH);
    const sab::Rec* ra = S ? S->sab.st.atRaw(sab::LOW, kA) : nullptr;
    CHECK(ra && ra->close == 'G' && std::fabs(ra->effort - 4.0) < 1e-6 && ra->E == 2.0 && ra->by == "BAS" && ra->book == -1, "KA: the shared A at the 5-candle low (sellers 4.0x >= 2.0x, green close, no order feed = book n/a)");
    CHECK(S && !S->sab.st.atRaw(sab::LOW, kR) && !S->sab.st.atRaw(sab::HIGH, kR), "KR: sellers 9x but a RED close at the low - no A (the close is required)");
    const sab::Rec* rh = S ? S->sab.st.atRaw(sab::HIGH, kH) : nullptr;
    CHECK(rh && rh->close == 'R' && rh->effort >= 2.0, "KH: the bearish shared A at the 5-candle high (buyers >= 2x, red close)");
    CHECK(lines.size() == 2, ("the record holds exactly the two A's (" + std::to_string(lines.size()) + " SAB lines)").c_str());
    bool anyGreenHigh = false; for (auto& l : lines) { sab::Rec q; if (sab::parse(l, q) && ((q.side == sab::LOW) != (q.close == 'G'))) anyGreenHigh = true; }
    CHECK(!anyGreenHigh, "no record line is on an attacker-colour candle");
    h.draws.clear(); p->draw();
    auto badgeText = [&](long long key, std::vector<std::pair<std::string, COLOR> >* parts = nullptr) {
        std::string b; const int xc = 50 + (int)((key - (long long)T0) / 60) * h.barW;
        for (size_t q = 0; q < S->badgeHit.size(); ++q) { const off::IconRect& r = S->badgeHit[q].first; if (xc < r.l - 12 || xc > r.r + 12) continue;
            for (auto& d : h.draws) if (d.type == "text" && d.x >= r.l - 1 && d.x <= r.r && d.y >= r.t - 3 && d.y <= r.b) { b += d.text; if (parts) parts->push_back(std::make_pair(d.text, d.color)); } break; }
        return b; };
    {   std::vector<std::pair<std::string, COLOR> > pa; const std::string ba = badgeText(kA, &pa);
        CHECK(ba.compare(0, 1, "A") == 0 && !pa.empty() && pa[0].second == C_ASK, ("KA: the attacker badge starts with A in green (" + ba + ")").c_str()); }
    CHECK(badgeText(kR).find('A') == std::string::npos, "KR (attacker-colour candle): no A on the badge");
    {   std::vector<std::pair<std::string, COLOR> > ph; const std::string bh = badgeText(kH, &ph);
        CHECK(bh.compare(0, 1, "A") == 0 && !ph.empty() && ph[0].second == C_BID, ("KH: the badge starts with A in red (" + bh + ")").c_str()); }
    // the hover on KA: the shared block in the agreed order
    {   const off::IconRect* r = nullptr; const int xc = 50 + KA * h.barW; for (auto& b : S->badgeHit) if (xc >= b.first.l - 12 && xc <= b.first.r + 12) r = &b.first;
        CHECK(r != nullptr, "the KA badge's rectangle kept for the mouse");
        if (r) {
            std::string tip = [&]() { for (auto& b : S->badgeHit) if (&b.first == r) return b.second; return std::string(); }();
            for (size_t w = tip.find("\n     "); w != std::string::npos; w = tip.find("\n     ")) tip.replace(w, 6, " ");   // unwrap continuation rows
            const size_t q = tip.find("A  Absorbed - will this low hold?"), e = tip.find("Effort: 4.0x (2.0x)"), c = tip.find("Close: green"),
                         bk = tip.find("Book: n/a"), pl = tip.find("Pulling:"), at = tip.find("Attack:"), lv = tip.find("Level:"), hd = tip.find("this low held 66% vs 51%"),
                         us = tip.find("arms your reversal bar / CoB entry - not a direction call"), ex = tip.find("Example:");
            CHECK(q != std::string::npos && e != std::string::npos && q < e && e < c && c < bk && bk < pl && pl < at && at < lv && lv < hd && hd < us && us < ex,
                  "hover: question, effort vs threshold, close, book, pulling, attack, level + gamma, held rate with n, not an entry, example - in that order");
            bas_test_cursor_wnd = (void*)0x1; pc::test_ok = true; const off::IconRect rr = *r;
            pc::test_cx = (rr.l + rr.r) / 2 + 1000; pc::test_cy = (rr.t + rr.b) / 2 + 500;
            RTX_EVENT ev = moveAt((rr.l + rr.r) / 2, (rr.t + rr.b) / 2, KA); p->mouse(&ev); h.draws.clear(); p->draw();
            CHECK(S->badgeTip >= 0 && S->lastTip.r > S->lastTip.l && S->lastTip.b <= h.paneBottom - 2, "mouse on the A badge: the hover drawn inside the pane");
            pc::test_cy = 2000; NOW += 300; p->calc(0); h.draws.clear(); p->draw();
        }
    }
    {   const std::string st = readAll(flex + "\\BidAskStats.status-ES-60.txt").empty() ? std::string() : std::string("x");
        (void)st; CHECK(S->sab.statusLine().find("SHARED_A,sab 1.0.0,E 2.0") == 0, "status line SHARED_A with E and its source"); }
    // no repaint: later candles never change the record
    for (int k = 46; k <= 50; ++k) { h.bars.push_back(barFor(k)); NOW += 1500; p->calc(0); }
    CHECK(readAll(recPath) == rec, "later candles: the record unchanged (decided once)");
    dispose(p);
    // restart: the record read back, nothing new written, the same badges
    Ind* p2 = make(); for (int r = 0; r < 4; ++r) { NOW += 1500; p2->calc(0); p2->draw(); }
    S = p2->testState(); h.draws.clear(); p2->draw();
    CHECK(readAll(recPath) == rec, "restart: nothing decided again, nothing written");
    CHECK(S->sab.st.atRaw(sab::LOW, kA) && S->sab.st.atRaw(sab::HIGH, kH) && S->sab.fired == 0 && badgeText(kA).compare(0, 1, "A") == 0, "restart: the same A's from the record");
    dispose(p2);
    // a line another plugin wrote first wins (adopted): a fresh chart, KA's line pre-written by "DP" with effort 3.3
    {   std::remove(recPath.c_str());
        sab::Rec r; r.mkt = "ES"; r.spb = 60; r.key = 0; r.raw = kA; r.side = sab::LOW; r.effort = 3.3; r.E = 2.0; r.att = 330; r.avg = 100; r.close = 'G'; r.by = "DP"; r.loc = "ext5";
        Ind* p3 = make(); r.key = p3->testKey(kA);
        { std::ofstream f(recPath.c_str(), std::ios::binary); f << sab::line(r) << "\n"; }
        for (int q = 0; q < 4; ++q) { NOW += 1500; p3->calc(0); p3->draw(); }
        BASState* S3 = p3->testState();
        const sab::Rec* g = S3 ? S3->sab.st.atRaw(sab::LOW, kA) : nullptr;
        CHECK(g && g->by == "DP" && std::fabs(g->effort - 3.3) < 1e-9 && S3->sab.adopted >= 1, "a line another plugin wrote first is adopted, never re-decided");
        dispose(p3); }
    // LRA-Badges-ES.csv raises E to 5.0: KA (4.0x) no longer fires on a fresh record; garbled = study default
    {   std::remove(recPath.c_str());
        { std::ofstream f((flex + "\\LRA-Badges-ES.csv").c_str(), std::ios::binary); f << "# x\nVERSION,1\nMARKET,ES\nASOF,2026-10-22\nPARAM,ABS_MIN_EFFORT_X,5.0\n"; }
        Ind* p4 = make(); for (int q = 0; q < 4; ++q) { NOW += 1500; p4->calc(0); }
        BASState* S4 = p4->testState();
        CHECK(S4 && S4->sab.P.E == 5.0 && S4->sab.P.fromFile && !S4->sab.st.atRaw(sab::LOW, kA) && S4->sab.st.atRaw(sab::HIGH, kH), "LRA-Badges ABS_MIN_EFFORT_X 5.0: KA (4.0x) not an A, KH (8x) still is");
        dispose(p4); std::remove(recPath.c_str());
        { std::ofstream f((flex + "\\LRA-Badges-ES.csv").c_str(), std::ios::binary); f << "garbage\n"; }
        Ind* p5 = make(); for (int q = 0; q < 4; ++q) { NOW += 1500; p5->calc(0); }
        BASState* S5 = p5->testState();
        CHECK(S5 && S5->sab.P.E == 2.0 && !S5->sab.P.fromFile && S5->sab.P.why.find("garbled") != std::string::npos && S5->sab.st.atRaw(sab::LOW, kA), "garbled params: the default 2.0, the reason in the status");
        dispose(p5); }
    std::printf("SharedAbsorb BAS host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
