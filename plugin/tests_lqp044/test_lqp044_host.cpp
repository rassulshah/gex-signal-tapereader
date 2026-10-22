// test_lqp044_host.cpp -- lsLiquidityProfile 0.44.0: the WHOLE plugin on the mock host (tests_lqp044/irtsdk.h = tests_lqp/irtsdk.h +
// the newer SDK calls). From plugin\:
//   g++ -std=c++17 -fsanitize=address,undefined -Itests_lqp044 -I. tests_lqp044/test_lqp044_host.cpp -o th044 && ./th044
// Checks: no "LIQ TESTING" header (nothing drawn while depth is fine), the 2x+ wall drawn, the status DRAW / normal_from / MQ lines,
// MenthorQ rows of another contract skipped and a file older than 18 h not used, Market State LIQ / LOTS posted after a bar closes and
// cleared on removal, the band pool saved.
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>
#include <utime.h>

namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)

static void setDepth(mock::Host& h, double bestBid, int levels, double extraPx = 0, double extra = 0) {
    h.depth.clear();
    for (int i = 0; i < levels; ++i) {
        DEPTH_LEVEL L; L.bid = (float)(bestBid - i * 0.01); L.ask = (float)(bestBid + 0.01 + i * 0.01);
        L.bidsize = 10; L.asksize = 10;
        if (extra > 0 && std::fabs(L.bid - extraPx) < 0.001) L.bidsize = (unsigned short)(10 + extra);
        h.depth.push_back(L);
    }
}
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static long long civil(int y, unsigned m, unsigned d) { y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; return era * 146097 + (long long)doe - 719468; }

int main() {
    char tmpl[] = "/tmp/lqp044XXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    const std::string mqPath = flex + "\\MenthorQLevels.csv";
    { std::ofstream mq(mqPath.c_str()); mq << "SYMBOL,PRICE,LABEL\r\nCLEX26,79.95,PS 0D\r\nCLEX26,80.40,CR 0D\r\nCLEZ26,79.95,PS 1D\r\nEPZ26,7000,CR 0D\r\n"; }
    mock::Host h; mock::current = &h;
    h.tick = 0.01f; h.seconds = 180; h.symbol = "CLZ6"; h.root = "CL"; h.chart = "test";
    const unsigned long day = (unsigned long)(civil(2026, 10, 12) * 86400LL);
    const unsigned long t0 = day + 9 * 3600;
    for (int i = 0; i < 25; ++i) {
        mock::Bar b; b.time = t0 - (24 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.03f;
        b.rows.push_back(mock::Price(80.03f, 50, 40, 90)); h.bars.push_back(b);
    }
    { mock::Bar f; f.time = t0 + 180; f.open = f.high = f.low = f.close = 80.03f; f.rows.push_back(mock::Price(80.03f, 0, 0, 0)); h.bars.push_back(f); }
    setDepth(h, 80.00, 80, 79.95, 200);
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 45; ++s) { h.now = t0 + s; p->calc(0); usleep(10000); }
    LQState* S = static_cast<LQState*>(h.data);
    CHECK(S && S->e.haveBook, "depth accepted");
    if (!S) { std::printf("no state\n"); return 1; }
    // ---- the 2x wall is drawn; nothing under 2x; no header text
    const lqp::Zone* wall = nullptr; for (auto& z : S->e.zones) if (z.side == lqp::BID && z.lo <= 7995 && z.hi >= 7995) wall = &z;
    CHECK(wall && wall->bx >= 2.0 && wall->drawn, "the 200-lot wall (2x+) is drawn");
    for (auto& z : S->e.zones) if (z.drawn) CHECK(z.bx >= 2.0 || z.behindN == 0, "every drawn zone is 2.0x+");
    h.draws.clear(); h.priceBase = 79.0f; p->draw(); h.priceBase = 80;
    bool testing = false, anyLiqHdr = false, xl = false;
    for (auto& d : h.draws) if (d.type == "text") { if (d.text.find("TESTING") != std::string::npos) testing = true; if (d.text.compare(0, 4, "LIQ ") == 0) anyLiqHdr = true;
        if (d.text.size() > 1 && d.text.back() == 'x' && std::isdigit((unsigned char)d.text[0])) xl = true; }
    CHECK(!testing && !anyLiqHdr, "no 'LIQ TESTING' / header text while depth is fine");
    CHECK(xl, "the wall's ratio is drawn in its node");
    // ---- MenthorQ: CLEX26 rows skipped on the CLZ6 chart, the CLEZ26 row used
    bool ps1 = false, ps0 = false; for (auto& l : S->mqLevels) { if (l.code == "PS 1D") ps1 = true; if (l.code == "PS 0D" || l.code == "CR 0D") ps0 = true; }
    CHECK(ps1 && !ps0, "only the chart contract's MenthorQ rows are used");
    CHECK(S->mqNote.find("2 rows of CLEX26 skipped") != std::string::npos, ("MQ note: " + S->mqNote).c_str());
    // ---- status lines
    usleep(1100000); h.now = t0 + 46; p->calc(0);
    std::string st = readAll(flex + "\\LiquidityProfile.status-CL-180.txt");
    CHECK(st.find("\nDRAW,only >= 2.0x") != std::string::npos, "status DRAW line");
    CHECK(st.find(",normal_from ") != std::string::npos && st.find(",pool_sessions ") != std::string::npos, "status says where the normal came from");
    CHECK(st.find(",norms file") == std::string::npos, "the old unconditional 'norms file' wording is gone");
    CHECK(st.find("\nMQ,") != std::string::npos && st.compare(0, 8 + std::string(lqp::VERSION).size() + 1, "VERSION," + std::string(lqp::VERSION) + "\n") == 0, "MQ line, the current version");   // (2026-10-22) the version moves on with each approved build (0.47.1 now): at least this one
    CHECK(std::string(LQ_VERSION) == lqp::VERSION, "one version everywhere");
    // ---- a stale MenthorQ file (20 h old) is not used
    { struct utimbuf u; u.actime = u.modtime = std::time(nullptr) - 20 * 3600; utime(mqPath.c_str(), &u); }
    usleep(1100000); h.now = t0 + 48; p->calc(0);
    CHECK(S->mqLevels.empty() && S->mqNote.find("older than 18 h") != std::string::npos, ("stale MQ not used: " + S->mqNote).c_str());
    { struct utimbuf u; u.actime = u.modtime = std::time(nullptr); utime(mqPath.c_str(), &u); }
    // ---- a bar closes: Market State LIQ / LOTS posted for it
    { mock::Bar n; n.time = t0 + 360; n.open = n.high = n.low = n.close = 80.02f; n.rows.push_back(mock::Price(80.02f, 0, 0, 0)); h.bars.push_back(n); }
    usleep(1100000); h.now = t0 + 181; p->calc(0);
    const long long nowS = steadyMs();
    mst::Facts liq = mst::read("LIQ", "CLZ6", 180, "test", nowS), lots = mst::read("LOTS", "CLZ6", 180, "test", nowS);
    CHECK(liq.ok && lots.ok, "LIQ and LOTS posted");
    CHECK(liq.bar == (long long)(t0 + 180), "posted for the closed bar (its Central stamp)");
    std::printf("  LIQ: zone_id=%s zone_x=%s step=%s side=%s level=%s | LOTS keys %zu s20=%s mkt_buy_max=%s\n", mst::getS(liq, "zone_id").c_str(), mst::getS(liq, "zone_x").c_str(),
                mst::getS(liq, "zone_step").c_str(), mst::getS(liq, "zone_side").c_str(), mst::getS(liq, "zone_level").c_str(), lots.kv.size(), mst::getS(lots, "s20").c_str(), mst::getS(lots, "mkt_buy_max").c_str());
    CHECK(liq.kv.size() == 10 && lots.kv.size() == 16, "every spec key present");
    CHECK(mst::getS(liq, "zone_side") == "support" && mst::getD(liq, "zone_x") >= 2.0 && mst::getS(liq, "zone_level").find("PS 1D") != std::string::npos, "the wall on PS 1D, 2x+, support");
    CHECK(mst::getS(lots, "mkt_buy_max") == "?", "no order feed: ? (never a made-up 0)");
    // the same bar's facts do not change on a re-post
    const std::map<std::string, std::string> before = liq.kv;
    setDepth(h, 80.00, 80, 79.95, 600);
    usleep(1100000); h.now = t0 + 183; p->calc(0);
    liq = mst::read("LIQ", "CLZ6", 180, "test", steadyMs());
    CHECK(liq.ok && liq.kv == before, "the closed bar's LIQ facts are frozen (no repaint)");
    // ---- removed: entries cleared, the band pool saved
    p->destroy();
    CHECK(!mst::read("LIQ", "CLZ6", 180, "test", steadyMs()).ok && !mst::read("LOTS", "CLZ6", 180, "test", steadyMs()).ok, "cleared on destroy");
    const std::string bp = readAll(flex + "\\LiquidityProfile.bandnorms-CL.txt");
    CHECK(bp.compare(0, 4, "BP1\n") == 0, "the band pool file is written");
    // (p is not deleted: the mock cppExtension has no virtual destructor, as in the other host tests)
    std::printf("lqp 0.44.0 host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
