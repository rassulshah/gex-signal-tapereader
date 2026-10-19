// test_aggbook_114_host.cpp -- lsBidAskStats 1.14.0 BIG LOT MARK end to end on the mock host (2026-10-18), 3-min ES bars Mon - Thu:
//   the tier of a session comes from complete EARLIER sessions only (Mon / Tue / Wed learning, Thu = 3 sessions), RTH and overnight
//   have their own size (a 90-lot bar is marked overnight, not in RTH), the cyan cap only on marked closed bars (Off draws nothing,
//   BL10 marks more), the box row "BL5" / "100", no repaint (a Ratio-bars recompute, later bigger prints, a restart), the records.
#include "BidAskStats.cpp"
typedef BidAskStats Ind;
long long bas_test_now_ms = 1000000;
void* bas_test_cursor_wnd = nullptr;
static long long& NOW = bas_test_now_ms;
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
namespace mock { Host* current = nullptr; Feed feed; }
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void dispose(Ind* p) { p->destroy(); p->~Ind(); ::operator delete(p); }
static int cyan(mock::Host& h) { int n = 0; for (auto& d : h.draws) if (d.type == "box" && d.color == C_BIG_LOT) ++n; return n; }
static bool hasText(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text == t) return true; return false; }
static mock::Bar bar(unsigned long t, long bid, long ask) {
    mock::Bar b; b.time = t; b.close = 7000.0f; b.open = 7000.0f; b.low = 6999.5f; b.high = 7000.5f;
    b.rows.push_back(mock::Price(7000.0f, ask, bid, bid + ask)); b.rows.push_back(mock::Price(7000.25f, 2, 2, 4)); b.volume = (unsigned long)(bid + ask + 4); return b;
}
static unsigned long SUN;   // Sunday 2026-10-04 17:00 (the mock's clock = UTC)
// the bar j (0 - 459) of session d (0 = Monday): it starts 17:00 the evening before + 3 min x j; stamp = its close
static unsigned long stampOf(int d, int j) { return SUN + 86400UL * (unsigned long)d + 180UL * (unsigned long)(j + 1); }
static void session(mock::Host& h, int d, int upto, bool big) {
    for (int j = 0; j < upto; ++j) {
        long bid = 5 + j % 5, ask = 5 + (j + 2) % 5;
        if (big) {
            if (j >= 20 && j < 26) ask = 80;       // overnight: 6 x 80 (BL5 80), 4 x 40 (BL10 40)
            if (j >= 30 && j < 34) bid = 40;
            if (j >= 400 && j < 406) bid = 100;    // RTH (13:00+): 6 x 100 (BL5 100), 4 x 50 (BL10 50)
            if (j >= 410 && j < 414) ask = 50;
        }
        h.bars.push_back(bar(stampOf(d, j), bid, ask));
    }
}
static void run(Ind* p, int n = 12) { for (int k = 0; k < n; ++k) { NOW += 700; p->calc(0); } }

int main() {
    char tmpl[] = "/tmp/agg114XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    { struct tm x; std::memset(&x, 0, sizeof(x)); x.tm_year = 126; x.tm_mon = 9; x.tm_mday = 4; x.tm_hour = 17; SUN = (unsigned long)timegm(&x); }
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ6"; h.root = "ES"; h.paneBottom = 300; h.paneRight = 2600; h.scaleLeft = 2590; h.ppb = 2; h.barW = 1; h.listIndex = 0;
    for (int d = 0; d < 3; ++d) session(h, d, 460, true);
    h.bars[(size_t)(2 * 460 + 405)].rows[0] = mock::Price(7000.0f, 5, 200, 205);   // Wed RTH: a 200-lot bar while Wed is still learning
    session(h, 3, 352, false);                                                      // Thu up to the 10:33 bar + the forming one
    const unsigned long tN = stampOf(3, 100), tD90 = stampOf(3, 320), tD120 = stampOf(3, 330), tWed = stampOf(2, 405);
    h.bars[(size_t)(3 * 460 + 100)] = bar(tN, 5, 90);      // Thu overnight 90 bought  (>= overnight BL5 80)
    h.bars[(size_t)(3 * 460 + 320)] = bar(tD90, 90, 5);    // Thu RTH 90 sold          (< RTH BL5 100, >= BL10 50)
    h.bars[(size_t)(3 * 460 + 330)] = bar(tD120, 5, 120);  // Thu RTH 120 bought       (>= RTH BL5 100)
    Ind* p = static_cast<Ind*>(CreateExtension()); p->setup(); run(p);
    BASState* S = p->testState();
    CHECK(!S->backlog && S->ser.bars.size() == 3 * 460 + 351, "every closed bar read");
    const long thu = svl::daysFromCivil(2026, 10, 8), wed = thu - 1;
    {   const blt::Tier& D = S->blBook.tiers[blt::DAY][thu]; const blt::Tier& N = S->blBook.tiers[blt::NIGHT][thu];
        CHECK(D.ok && D.nSess == 3 && D.bl[0] == 100 && D.bl[1] == 50 && N.ok && N.bl[0] == 80 && N.bl[1] == 40, "Thu tiers from Mon / Tue / Wed: RTH BL5 100 BL10 50, overnight BL5 80 BL10 40");
        CHECK(!S->blBook.tiers[blt::DAY][wed].ok && S->blBook.tiers[blt::DAY][wed].nSess == 2, "Wed: learning (2 earlier complete sessions)"); }
    CHECK(S->blMarks.count(tN) && blt::shows(S->blMarks[tN], 1), "Thu overnight 90 >= overnight BL5 80: marked");
    CHECK(S->blMarks.count(tD90) && !blt::shows(S->blMarks[tD90], 1) && blt::shows(S->blMarks[tD90], 2), "Thu RTH 90 < RTH BL5 100: not marked (BL10: marked) - RTH vs overnight");
    CHECK(S->blMarks.count(tD120) && blt::shows(S->blMarks[tD120], 1) && S->blMarks[tD120].side == 1 && S->blMarks[tD120].lots == 120, "Thu RTH 120: marked, buyers");
    CHECK(S->blMarks.count(tWed) && !S->blMarks[tWed].tier.ok && !blt::shows(S->blMarks[tWed], 1), "Wed 200-lot bar while learning: no mark");
    int shown = 0; for (const auto& kv : S->blMarks) shown += blt::shows(kv.second, 1) ? 1 : 0;
    CHECK(shown == 2, "exactly 2 candles marked at BL5 (Thu 90 overnight, Thu 120 RTH)");
    CHECK(!S->blMarks.count(stampOf(3, 351)), "the forming candle is never judged");
    h.draws.clear(); p->draw();
    CHECK(cyan(h) == 2, "2 cyan caps drawn");
    {   int ok = 0; for (auto& d : h.draws) if (d.type == "box" && d.color == C_BIG_LOT) ok += (d.b - d.y) == BIG_LOT_CAP_PX - 1 ? 1 : 0;
        CHECK(ok == 2, "each cap is 3 px tall (top of the bar)"); }
    {   int keep = 0; for (auto& c : h.draws) if (c.type == "box" && c.color == C_BIG_LOT && c.x <= 50 + 3 * 460 + 330 && c.r >= 50 + 3 * 460 + 330)
            for (auto& d : h.draws) if (d.type == "box" && d.color == C_ASK_SPIKE && d.x == c.x && d.r == c.r && d.y == c.y && d.b > c.b) ++keep;   // 120 = 2.0x+: bright green, its cap on its top edge
        CHECK(keep == 1, "the marked bar keeps its green bar under the cap"); }
    CHECK(!hasText(h, "BL5") && S->blBook.tiers[blt::DAY][thu].bl[0] == 100, "(1.15.0) no BL5 box row (removed); the tier is still decided and drawn as caps");
    // the records: history (S lines Mon - Wed both types + the T lines), this session's candles >= BL20
    {   const std::string hist = readAll(flex + "\\BidAskStats\\BigLot-ES-180.txt");
        CHECK(hist.find("S|" + std::to_string(wed) + "|1|150|100|50|9") != std::string::npos, "history: Wed RTH S line (150 candles, 100 / 50 / 9)");
        CHECK(hist.find("T|" + std::to_string(thu) + "|1|3|100|50|9") != std::string::npos && hist.find("T|" + std::to_string(thu) + "|0|3|80|40|9") != std::string::npos, "history: Thu tiers T lines (RTH, overnight)");
        const std::string rec = flex + "\\BidAskStats\\ES-" + off::fileKey("ESZ6") + "-180-" + S->curSession + "-biglot.txt"; const std::string r = readAll(rec);
        CHECK(!rec.empty() && r.find("bar_time|ct_time|session|type|side|lots|ratio|spike|reached|bl5|bl10|bl20|n_sess") != std::string::npos, "this session's record with its header");
        CHECK(r.find(std::to_string(tD120) + "|") != std::string::npos && r.find("|1|1|120|") != std::string::npos && r.find("|BL5|100|50|9|3") != std::string::npos, "the 120 candle recorded: RTH, buyers, BL5 reached, its tier");
        CHECK(r.find("|0|1|90|") != std::string::npos && r.find("|1|0|90|") != std::string::npos, "the overnight 90 and the RTH 90 recorded (scored by the nightly)");
        NOW += 61000; p->calc(0);
        const std::string st = readAll(flex + "\\BidAskStats.status-ES-" + off::fileKey("ESZ6") + "-180.txt");
        CHECK(st.find("BIG_LOT_MARK,BL5") != std::string::npos && st.find("BIG_LOT_TIER_RTH,session " + std::to_string(thu) + ",BL5 100,BL10 50") != std::string::npos && st.find("BIG_LOT_HITS_TODAY,BL5 2,BL10 3") != std::string::npos, "status file BIG_LOT lines"); }
    // the setting: BL10 marks more, Off draws nothing (no caps, no box row); frozen numbers - nothing is recomputed
    mkdir(flex.c_str(), 0700);
    { std::ofstream f((flex + "\\BidAskStats.place-ES.txt").c_str()); f << "20,0,20,15,10,0,0,0,2\n"; }
    NOW += 1500; p->calc(0); h.draws.clear(); p->draw();
    CHECK(S->bigLot == 2 && cyan(h) == 3 && !hasText(h, "BL10"), "BL10: 3 caps (no box row since 1.15.0)");
    { std::ofstream f((flex + "\\BidAskStats.place-ES.txt").c_str()); f << "20,0,20,15,10,0,0,0,0\n"; }
    NOW += 1500; p->calc(0); h.draws.clear(); p->draw();
    CHECK(S->bigLot == 0 && cyan(h) == 0 && !hasText(h, "BL5") && !hasText(h, "BL10"), "Off: no cap, no box row");
    { std::ofstream f((flex + "\\BidAskStats.place-ES.txt").c_str()); f << "20,0,20,15,10,0,0,0\n"; }
    NOW += 1500; p->calc(0); CHECK(S->bigLot == 1, "an 8-field file (1.13) = BL5");
    { std::ofstream f((flex + "\\BidAskStats.place-ES.txt").c_str()); f << "20,0,20,15,10,0,0,0,7\n"; }
    NOW += 1500; p->calc(0); CHECK(S->bigLot == 1 && S->ratioBars == 20, "an invalid Big Lot field: the whole line rejected -> defaults (never garbage)");
    // no repaint: a Ratio-bars recompute and later, bigger prints in the same session never move a decided mark or the session's tier
    const std::map<long long, blt::Mark> before = S->blMarks;
    { std::ofstream f((flex + "\\BidAskStats.place-ES.txt").c_str()); f << "40,0,20,15,10,0,0,0,1\n"; }
    NOW += 1500; run(p, 3);
    {   bool same = true; for (const auto& kv : before) { auto it = S->blMarks.find(kv.first); same = same && it != S->blMarks.end() && it->second.lots == kv.second.lots && it->second.tier.bl[0] == kv.second.tier.bl[0] && it->second.tier.ok == kv.second.tier.ok; }
        CHECK(S->ratioBars == 40 && same, "Ratio bars 20 -> 40 re-decides spikes only: every Big Lot mark unchanged"); }
    h.bars.pop_back();
    for (int j = 351; j < 420; ++j) h.bars.push_back(bar(stampOf(3, j), j == 380 ? 5000 : 5 + j % 5, 5 + (j + 2) % 5));   // Thu 12:00: a 5,000-lot print
    run(p);
    CHECK(S->blBook.tiers[blt::DAY][thu].bl[0] == 100 && blt::shows(S->blMarks[tD120], 1) && !blt::shows(S->blMarks[tD90], 1), "after a 5,000-lot print: Thu tier still 100, the 120 still marked, the 90 still not");
    CHECK(blt::shows(S->blMarks[stampOf(3, 380)], 1), "the 5,000-lot candle marked at its close");
    h.draws.clear(); p->draw(); CHECK(cyan(h) == 3, "3 caps drawn");
    // a restart: the same marks (tiers and this session's candles read back from the records)
    dispose(p);
    Ind* q = static_cast<Ind*>(CreateExtension()); q->setup(); run(q);
    BASState* S2 = q->testState();
    CHECK(S2->blMarks.count(tD120) && S2->blMarks[tD120].tier.why == "recorded" && blt::shows(S2->blMarks[tD120], 1), "after a restart: the 120 candle's mark read back exactly as decided");
    {   int n = 0; for (const auto& kv : S2->blMarks) n += blt::shows(kv.second, 1) ? 1 : 0; CHECK(n == 3, "after a restart: the same 3 marks"); }
    h.draws.clear(); q->draw(); CHECK(cyan(h) == 3, "after a restart: 3 caps drawn");
    dispose(q);
    std::printf("BidAskStats Big Lot host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
