// test_bba116.cpp -- (1.17.0: [fixed] no user settings, no category 4, numbers never through the HOD/LOD box / Big Lot box) lsBigBidAsk 1.16.0 (2026-10-18, Rassul approved 15:03 CT) + lsHodLod 2.4.1 publish (BoxDock.h).
//   [box]    typeNeed / typeAvg / boxRows / boxHover: the averages of the CURRENT session type, thousands separators, "--" learning,
//            only 3 rows in the box, everything else in the hover
//   [cat]    category order 1 > 4 > 2 / 3 (Rassul 15:04-15:07 "cat 1 and 4 should be cyan"): 1,000 lots with BL 1,391 / 1,190 / 1,007
//            and category 4 = 1,000 -> category 4; 1,500 -> category 1; 900 -> none
//   [dock]   bdk::place: right / left / above / below of the HOD/LOD table, top vs bottom alignment, the last-bar edge, the pane edges,
//            the Tape Reader banner, no table -> the 9-spot Position, every spot blocked -> not drawn; never overlaps; board round trip
//   [set]    all 49 settings fields round trip (file and window), defaults per market, colour byte order, the divider shown as used,
//            group off = none of its levels, near-black swatch kept
//   [host]   the chart: docked box beside the published table, own position without it, unchecked = nothing drawn, hover text, the
//            settings window: swatches filled with setParameterColor, Divider 10 on ES, group click checks / unchecks / greys its levels
// Build (from tests_bba115/): g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. -I.. test_bba116.cpp -o /tmp/t_bba116 && /tmp/t_bba116
#include "irtsdk.h"
#include "../BigBidAsk.cpp"
#include <iostream>
#include <random>
#include <sys/stat.h>
namespace mock { Host* current = nullptr; }
static int passed = 0, failed = 0;
#define CHECK(c, why) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL line " << __LINE__ << ": " << why << "\n"; } } while (0)

static void boxLogic() {
    CHECK(bba::typeNeed(false, 510, 900, 180) == 112 && bba::typeNeed(true, 510, 900, 180) == 232, "[box] complete day 3/4 of 150, night 3/4 of 310 (ES 3-min)");
    CHECK(bba::typeNeed(false, 510, 900, 0) == 112 && bba::typeNeed(true, 510, 900, 60) == 697, "[box] bad period -> 180 s; 1-min night");
    // sessions 1..6 complete, 7 = today (partial): session k's prints 1..200 scaled by k
    std::map<long, std::vector<long long>> P; std::map<long, int> C;
    for (long k = 1; k <= 7; ++k) { for (int i = 1; i <= 200; ++i) P[k].push_back(i * 10 * (k == 7 ? 100 : 1) + k); C[k] = k == 7 ? 20 : 150; }
    const long long thr[3] = { 1500, 1000, 50 };
    const bba::TypeAvg A = bba::typeAvg(P, C, 7, 112, thr, false);
    CHECK(A.done.size() == 6 && A.nSess == 5, "[box] 6 complete earlier sessions, the latest 5 averaged, today never");
    CHECK(A.prior[0] == 1966 && A.prior[1] == 1916 && A.prior[2] == 1816, "[box] prior session = session 6's 5th / 10th / 20th biggest");
    CHECK(A.avg[0].avg == 1964 && A.avg[0].used == 5 && A.avg[0].dropped < 0, "[box] 5-session average of the 5th biggest (sessions 2-6)");
    CHECK(A.today[0] == 199 && A.today[2] == 200, "[box] today's prints at or above today's tier");
    const auto rows = bba::boxRows(A);
    CHECK(rows.size() == 3 && rows[0].first == "BL5" && rows[1].first == "BL10" && rows[2].first == "BL20", "[box] exactly 3 rows BL5 / BL10 / BL20");
    CHECK(rows[0].second == "1,964", "[box] thousands separator");
    std::map<long, std::vector<long long>> P2 = P; for (auto& x : P2[6]) x *= 3;   // one outlier session (3x)
    const bba::TypeAvg B = bba::typeAvg(P2, C, 7, 112, thr, false);
    CHECK(B.droppedSess == 6 && B.avg[0].used == 4 && B.avg[0].avg == 1964, "[box] the outlier session dropped -> 4-session average");
    const bba::TypeAvg L = bba::typeAvg(P, C, 1, 112, thr, true);
    CHECK(L.done.empty() && bba::boxRows(L)[0].second == "--" && bba::boxValue(L, 2) == "--", "[box] learning shows --");
    CHECK(bba::commas(995) == "995" && bba::commas(1434) == "1,434" && bba::commas(1234567) == "1,234,567" && bba::commas(0) == "0", "[box] commas");
    const std::vector<std::string> H = bba::boxHover(B, L, "ES", "Sizes the candles use ...", "Oct 14", "Scoring: learning");
    auto has = [&](const std::string& s) { for (const auto& h : H) if (h.find(s) != std::string::npos) return true; return false; };
    CHECK(has("Question:") && has("How:") && has("Use:") && has("Example:") && has("What:") && has("Why:"), "[box] hover: question / how / use / example / what / why");
    CHECK(has("prior session 5,898") && has("4-session avg 1,964") && has("today 199 of ~5"), "[box] hover: prior session, average, today's count");
    CHECK(has("overnight averages: BL5 / 10 / 20 = -- / -- / --") && has("Outlier dropped: Oct 14 (3.0x") && has("Scoring: learning"), "[box] hover: the other type, the outlier, scoring");
    CHECK(H[0].find("RTH") != std::string::npos && H[0].find("4-session average") != std::string::npos, "[box] hover title names the session type");
}
static void catLogic() {
    const long long zz[4] = { 1391, 1190, 1007, 1000 }; const bool all[4] = { true, true, true, true };
    CHECK(bba::firstCat(zz, 1000, all) == 3, "[cat] 1,000 lots (BL 1,391 / 1,190 / 1,007, his 1,000) -> category 4 (cyan)");
    CHECK(bba::firstCat(zz, 1200, all) == 3, "[cat] 1,200 reaches BL10 too - his category 4 wins");
    CHECK(bba::firstCat(zz, 1500, all) == 0, "[cat] 1,500 -> category 1 (Big Lot 5)");
    CHECK(bba::firstCat(zz, 900, all) == -1, "[cat] 900 -> none");
    const long long z2[4] = { 1391, 1190, 1007, 0 };   // category 4 off
    CHECK(bba::firstCat(z2, 1200, all) == 1 && bba::firstCat(z2, 1010, all) == 2, "[cat] category 4 off: Big Lot 10 / 20 as before");
    const bool no1[4] = { false, true, true, true };
    CHECK(bba::firstCat(zz, 1500, no1) == 3, "[cat] category 1's filter fails -> his category 4");
    const long long z3[4] = { 1391, 1007, 1190, 2000 };   // categories 2 / 3 swapped sizes: the bigger first
    int ord[4]; bba::catOrder(z3, ord);
    CHECK(ord[0] == 0 && ord[1] == 3 && ord[2] == 2 && ord[3] == 1, "[cat] order 1, 4, then 2 / 3 biggest first");
    CHECK(bba::firstCat(z3, 1500, all) == 0 && bba::firstCat(z3, 1200, all) == 2, "[cat] category 4 at 2,000: 1,500 -> 1; 1,200 -> the bigger of 2 / 3");
}
static void dockLogic() {
    const bdk::Box area = bdk::mk(2, 20, 1500, 800);
    const bdk::Box low = bdk::mk(300, 650, 900, 790), high = bdk::mk(300, 30, 900, 140);
    std::vector<bdk::Box> none;
    bdk::Result r = bdk::place(true, low, 120, 60, area, 9, none);
    CHECK(r.mode == bdk::DOCKED && r.side == bdk::S_RIGHT && r.b.l == 904 && r.b.b == 790 && r.b.r - r.b.l == 120, "[dock] right of a bottom table, bottom-aligned, 4 px gap");
    r = bdk::place(true, high, 120, 60, area, 9, none);
    CHECK(r.mode == bdk::DOCKED && r.side == bdk::S_RIGHT && r.b.t == 30, "[dock] right of a top table, top-aligned");
    r = bdk::place(true, bdk::mk(300, 650, 1450, 790), 120, 60, area, 9, none);
    CHECK(r.mode == bdk::DOCKED && r.side == bdk::S_LEFT && r.b.r == 296 && r.b.r <= 1450, "[dock] the last bar is near: left of the table");
    r = bdk::place(true, bdk::mk(10, 650, 1450, 790), 120, 60, area, 9, none);
    CHECK(r.mode == bdk::DOCKED && r.side == bdk::S_ABOVE && r.b.b == 646 && r.b.r == 1450, "[dock] no room either side: above, right edges aligned");
    r = bdk::place(true, bdk::mk(10, 25, 1450, 140), 120, 60, area, 9, none);
    CHECK(r.mode == bdk::DOCKED && r.side == bdk::S_BELOW && r.b.t == 144, "[dock] a full-width top table: below");
    std::vector<bdk::Box> tr = { bdk::mk(890, 700, 1100, 790) };
    r = bdk::place(true, low, 120, 60, area, 9, tr);
    CHECK(r.mode == bdk::DOCKED && r.side == bdk::S_LEFT, "[dock] the Tape Reader banner on the right: left instead");
    r = bdk::place(false, low, 120, 60, area, 9, none);
    CHECK(r.mode == bdk::OWN && r.b.r == 1496 && r.b.b == 796, "[dock] no table: the 9-spot Position (bottom right, up to the last bar)");
    r = bdk::place(false, low, 120, 60, area, 1, none);
    CHECK(r.mode == bdk::OWN && r.b.l == 6 && r.b.t == 24, "[dock] Position top left");
    r = bdk::place(true, bdk::mk(2, 20, 1500, 800), 120, 60, area, 9, none);
    CHECK(r.mode == bdk::NONE, "[dock] the table fills the area: not drawn (never on it)");
    r = bdk::place(true, low, 2000, 60, area, 9, none);
    CHECK(r.mode == bdk::NONE && r.why == "area too small", "[dock] a box wider than the area: not drawn");
    std::vector<bdk::Box> blk = { bdk::mk(1300, 700, 1500, 800) };
    r = bdk::place(false, low, 120, 60, area, 9, blk);
    CHECK(r.mode == bdk::OWN_MOVED && !bdk::overlap(r.b, blk[0]), "[dock] own spot blocked: the first free spot");
    // fuzz: never outside the area, never on the table or an avoid box
    std::mt19937 rng(3); bool good = true;
    for (int k = 0; k < 4000; ++k) {
        const int l = (int)(rng() % 1600) - 50, t = (int)(rng() % 820) - 10, w = (int)(rng() % 900) + 1, h = (int)(rng() % 300) + 1;
        const bdk::Box host = bdk::mk(l, t, l + w, t + h); const bdk::Box a = bdk::mk(2, 20, 300 + (int)(rng() % 1300), 800);
        std::vector<bdk::Box> av; if (rng() % 2) { const int x = (int)(rng() % 1400); av.push_back(bdk::mk(x, 20, x + 300, 70)); }
        const bool hon = rng() % 4 != 0; const int ww = 110 + (int)(rng() % 30), pos = 1 + (int)(rng() % 9);
        const bdk::Result q = bdk::place(hon, host, ww, 55, a, pos, av);
        if (q.mode == bdk::NONE) continue;
        if (!bdk::inside(a, q.b) || (hon && bdk::overlap(q.b, host)) || (!av.empty() && bdk::overlap(q.b, av[0]))) good = false;
    }
    CHECK(good, "[dock] fuzz 4,000: inside the area, never over the table or the banner");
    // the board
    bdk::Box got; long long ms = -1;
    bdk::publishHodLod("ESZ6", 180, "ES 3m", 1234, low);
    CHECK(bdk::peekHodLod("ESZ6", 180, "ES 3m", got, ms) && ms == 1234 && got.l == 300 && got.b == 790, "[dock] publish / peek round trip");
    CHECK(!bdk::peekHodLod("ESZ6", 180, "NQ 3m", got, ms) && !bdk::peekHodLod("ESZ6", 60, "ES 3m", got, ms), "[dock] another chart / bar size: separate");
    bdk::clearHodLod("ESZ6", 180, "ES 3m");
    CHECK(!bdk::peekHodLod("ESZ6", 180, "ES 3m", got, ms), "[dock] cleared on removal");
}
static void keepOutLogic() {   // (1.17.0, Rassul 19:58 CT) numbers never through the HOD/LOD table + strip or the Big Lot box
    std::mt19937 rng(23); int bad = 0, slid = 0, blocked = 0, clear = 0;
    for (int it = 0; it < 20000; ++it) {
        bba::Box area; area.l = 60; area.t = 20; area.r = 900 + (int)(rng() % 400); area.b = 400 + (int)(rng() % 400);
        bba::Box hl; hl.l = 100 + (int)(rng() % 300); hl.t = 100 + (int)(rng() % 250); hl.r = hl.l + 280 + (int)(rng() % 100); hl.b = hl.t + 140;
        bba::Box blb; blb.l = hl.r + 4; blb.t = hl.t + 25; blb.r = blb.l + 90; blb.b = hl.b;
        const std::vector<bba::Box> ko = { hl, blb };
        bba::Box f; f.l = area.l + (int)(rng() % (area.r - area.l - 40)); f.t = area.t + (int)(rng() % (area.b - area.t - 20)); f.r = f.l + 18 + (int)(rng() % 14); f.b = f.t + 13;
        const bba::Box f0 = f; const int r = bba::keepOut(f, ko, area);
        if (r < 0) { ++blocked; continue; }
        if (r == 0) ++clear; else ++slid;
        if (bba::hits(f, hl) || bba::hits(f, blb) || f.t < area.t || f.b > area.b || f.l != f0.l || std::abs(f.t - f0.t) > 40) ++bad;
    }
    std::cerr << "  keep-out fuzz: clear " << clear << ", slid " << slid << ", not drawn " << blocked << "\n";
    CHECK(bad == 0 && slid > 100 && blocked > 100, "[keepout] 20,000 numbers: none drawn over the HOD/LOD box or the Big Lot box; slid at most 40 px up / down, never sideways, inside the area");
}
static void setLogic() {
    std::mt19937 rng(11); bool fileRt = true;
    for (int it = 0; it < 500; ++it) {
        bba::Settings s; s.div = (int)(rng() % 50); s.box = rng() % 2; s.dim = rng() % 2; s.circle = rng() % 2;
        s.circleCol = 0x202020 + rng() % 0xDFDFDF; s.boxBid = 0x202020 + rng() % 0xDFDFDF; s.boxAsk = 0x202020 + rng() % 0xDFDFDF;
        for (int k = 0; k < 4; ++k) { bba::Cat& c = s.cat[k]; c.on = rng() % 2; c.n = k < 3 ? 1 + (int)(rng() % 100) : 0; c.lots = k < 3 ? (int)(rng() % 5000) : 1 + (int)(rng() % 5000);
            c.font = 6 + (int)(rng() % 15); c.show = k < 3 ? (int)(rng() % 7) : bba::SH_ALL; c.bid = 0x202020 + rng() % 0xDFDFDF; c.ask = 0x202020 + rng() % 0xDFDFDF; }
        s.skipFirst = (int)(rng() % 241); s.skipLast = (int)(rng() % 241); s.atrUpQ = (int)(rng() % 2001); s.atrDnQ = (int)(rng() % 2001);
        s.keyOn = rng() % 2; s.gamOn = rng() % 2; s.volOn = rng() % 2;
        s.keyMask = s.keyOn ? rng() % (1u << bba::NKEY) : 0; s.gamMask = s.gamOn ? rng() % (1u << bba::NGAM) : 0; s.volMask = s.volOn ? rng() % (1u << bba::NVOL) : 0;
        s.statsChart = rng() % 2; s.statsPos = 1 + (int)(rng() % 9); s.rthOnly = rng() % 2;
        std::string why; const std::string f = bba::formatSettings(s);
        const bba::Settings b = bba::parseSettings(f + "\n", why, "ES");
        if (bba::formatSettings(b) != f || !why.empty()) fileRt = false;
        if (std::count(f.begin(), f.end(), '|') + 1 != (long)bba::NFIELDS) fileRt = false;
    }
    CHECK(fileRt, "[set] 500 random settings: every one of the 49 fields survives the file");
    CHECK(bba::NFIELDS == 49, "[set] 49 file fields (the old file format, still parsed by the tests only)");
    // his ES file (15:10)
    std::string why; const std::string his = "BBA7|0|1|0|0|16777215|15680580|2278750|1|5|0|8|0|65535|65535|1|10|0|8|6|16777215|16777215|1|20|0|8|6|16777215|16777215|1|0|1000|8|0|65535|65535|15|15|200|100|0|0|0|0|0|0|1|9|0";
    const bba::Settings h = bba::parseSettings(his, why, "ES");
    CHECK(why.empty() && h.div == 0 && h.boxBid == 0xEF4444 && h.boxAsk == 0x22C55E && h.cat[3].lots == 1000 && h.cat[3].bid == 0x00FFFF, "[set] his ES file: box colours EF4444 / 22C55E (0x00RRGGBB), category 4 1,000 cyan");
    CHECK(bba::shownSettings(h, "ES").div == 10 && bba::shownSettings(h, "NQ").div == 1 && bba::shownSettings(h, "CL").div == 10, "[set] Divider shows the market default (ES / CL 10, others 1)");
    bba::Settings h7 = h; h7.div = 7; CHECK(bba::shownSettings(h7, "ES").div == 7, "[set] a typed divider shows as typed");
    CHECK(bba::formatSettings(bba::Settings()).find("|15680580|2278750|") != std::string::npos, "[set] default box colours red at the bid / green at the ask");
    CHECK(bba::defaultsFor("ES").cat[3].lots == 1000 && bba::defaultsFor("NQ").cat[3].lots == 450 && bba::defaultsFor("CL").cat[3].lots == 95 && bba::defaultsFor("GC").cat[3].lots == 105
          && bba::defaultsFor("HG").cat[3].lots == 90 && bba::defaultsFor("NG").cat[3].lots == 295 && bba::defaultsFor("6E").cat[3].lots == 490, "[set] category 4 default per market");
    bba::Settings g = h; g.keyOn = false; g.keyMask = 0xFFF; g.gamOn = true; g.gamMask = 3;
    const bba::Settings e = bba::effective(g);
    CHECK(e.keyMask == 0 && e.gamMask == 3, "[set] an off group uses none of its levels");
    CHECK(bba::nearBlack(0) && bba::nearBlack(0x101010) && !bba::nearBlack(0xEF4444) && !bba::nearBlack(0x0000FF), "[set] near-black swatch detection");
    // (1.17.0) THE fixed behaviour = what his chart showed (his 17:15 file) without category 4
    for (const char* m : { "ES", "NQ", "CL", "GC", "HG", "NG", "6E" }) {
        const bba::Settings f = bba::fixedSettings(m);
        bool ok = f.div == 0 && f.box && !f.dim && !f.circle && f.boxBid == 0xEF4444 && f.boxAsk == 0x22C55E && f.skipFirst == 15 && f.skipLast == 15 && !f.rthOnly
               && !f.keyOn && !f.gamOn && !f.volOn && f.keyMask == 0 && f.gamMask == 0 && f.volMask == 0 && f.statsChart && f.statsPos == 9 && !f.cat[3].on;
        const int n[3] = { 5, 10, 20 }, sh[3] = { bba::SH_ALL, bba::SH_PBBO, bba::SH_PBBO }; const unsigned col[3] = { 0xFFFFFF, 0xFFFFFF, 0xFFFFFF };   // (1.18.0) all white, font 7
        for (int k = 0; k < 3; ++k) ok = ok && f.cat[k].on && f.cat[k].n == n[k] && f.cat[k].lots == 0 && f.cat[k].font == 7 && f.cat[k].show == sh[k] && f.cat[k].bid == col[k] && f.cat[k].ask == col[k];
        CHECK(ok, std::string("[fixed] ") + m + ": (1.18.0) white numbers, font 7, boxes red / green, no circle / dim / levels, skip 15 / 15, box on, no category 4");
    }
    {   bba::Settings f = bba::fixedSettings("ES"); f.cat[3] = h.cat[3];
        for (int k = 0; k < 3; ++k) { f.cat[k].font = 8; f.cat[k].bid = f.cat[k].ask = k == 0 ? 0x00FFFF : 0xFFFFFF; }   // (1.18.0) only font / colour changed
        CHECK(bba::formatSettings(f) == bba::formatSettings(h), "[fixed] = his 17:15 ES file field for field, except category 4 (removed) and 1.18.0 font 7 / white"); }
    {   const bba::Settings f = bba::fixedSettings("ES"); long long zz[4]; bool pass[4] = { true, true, true, true };
        for (int k = 0; k < 4; ++k) zz[k] = f.cat[k].on ? (k == 3 ? (long long)f.cat[3].lots : 100LL * (k + 1)) : 0;
        bool never4 = zz[3] == 0; for (long long vol : { 50LL, 150LL, 999LL, 1000LL, 5000LL, 100000LL }) if (bba::firstCat(zz, vol, pass) == 3) never4 = false;
        CHECK(never4, "[fixed] category 4 never chosen (off, size 0), whatever the print size"); }
}

// ---- host ---------------------------------------------------------------------------------------------------------------------------
static const unsigned long DAY = 86400, START = 1791133200UL;   // Sun 2026-10-04 17:00 (Central wall time as UTC in the mock)
static std::vector<mock::Bar> makeBars(int todayBars, unsigned seed) {
    std::mt19937 rng(seed); std::vector<mock::Bar> v; float p = 7700;
    auto addSession = [&](unsigned long open, int n) {
        for (int i = 1; i <= n; ++i) {
            mock::Bar b; b.time = open + (unsigned long)i * 180;
            const float step = (float)((int)(rng() % 9) - 4) * 0.25f; p += step; if (p > 7720 || p < 7680) p = 7700;
            b.open = p; b.low = p - 1.0f; b.high = p + 1.0f; b.close = p + ((rng() % 2) ? 0.5f : -0.5f);
            for (int r = 0; r <= 8; ++r) { mock::VRow x; x.price = b.low + 0.25f * (float)r; x.ask = (long)(rng() % 40 + 1); x.bid = (long)(rng() % 40 + 1); b.rows.push_back(x); }
            const int big = (int)(rng() % 9); const long lots = (long)(std::exp(3.5 + (double)(rng() % 1000) / 1000.0 * 3.0));
            if (rng() % 2) b.rows[(size_t)big].bid += lots; else b.rows[(size_t)big].ask += lots;
            for (const auto& x : b.rows) b.volume += (unsigned long)(x.ask + x.bid);
            v.push_back(b);
        }
    };
    unsigned long open = START;
    for (int d = 0; d < 12; ++d, open += DAY) { const int wd = d % 7; if (wd == 5 || wd == 6) continue; addSession(open, 460); }
    addSession(START + 14 * DAY, todayBars);
    return v;
}
struct Chart {
    mock::Host H; BigBidAsk* x = nullptr;
    explicit Chart(const std::vector<mock::Bar>& b) { H.bars = b; H.now = b.back().time; H.barW = 6; H.barOffset = (int)b.size() - 200; H.visStart = (int)b.size() - 200; H.visEnd = (int)b.size() - 1;
        mock::current = &H; x = static_cast<BigBidAsk*>(CreateExtension()); x->setup(); }
    ~Chart() { mock::current = &H; x->destroy(); delete x; mock::current = nullptr; }
    BBState* S() { mock::current = &H; return x->testState(); }
    void pump(int n, bool draw) {
        for (int k = 0; k < n; ++k) { mock::current = &H; BBState* s = x->testState();
            if (s) { s->builtMs = -1; s->boardMs = -1; s->sdCheckMs = -1; s->statsMs = -1; s->lastStatusMs = -1; s->logMs = -1; s->setCheckMs = -1; }
            if (draw) { H.draws.clear(); x->draw(); } else x->calc(0); }
    }
    const mock::Draw* boxRect() const { for (const auto& d : H.draws) if (d.type == "rect" && d.color == C_BLEDGE) return &d; return nullptr; }
    std::vector<std::string> texts() const { std::vector<std::string> v; for (const auto& d : H.draws) if (d.type == "text") v.push_back(d.text); return v; }
};
static void home(const std::string& name) {
    const std::string root = "/tmp/bba116-run", h = root + "/" + name; std::string c = "rm -rf '" + h + "'"; if (std::system(c.c_str()) != 0) {}
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
}
static void writeSettings(const bba::Settings& s) {
    std::ofstream f(std::string(getenv("USERPROFILE")) + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk.settings-ES.txt"); f << bba::formatSettings(s) << "\n";
}
static std::string readSettingsLine() {
    std::ifstream f(std::string(getenv("USERPROFILE")) + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk.settings-ES.txt"); std::string l; std::getline(f, l); return l;
}
static void hostTests() {
    const std::vector<mock::Bar> bars = makeBars(200, 42);   // today = 200 bars from 17:00 -> 03:00, overnight
    {   home("dock");
        Chart c(bars); c.pump(10, false);
        bdk::clearHodLod("ESZ6", 180, "ES 3m");
        c.pump(2, true);
        BBState* S = c.S();
        CHECK(S && S->blNightNow, "[host] 03:00: the overnight session type");
        const mock::Draw* r = c.boxRect();
        CHECK(r != nullptr, "[host] the box is drawn (no table: own Position)");
        int lastX = 20 + ((int)bars.size() - 1 - c.H.barOffset) * c.H.barW + c.H.barW / 2;
        if (r) CHECK(r->r == lastX - 4 && r->b == c.H.paneBottom - 1 - 4, "[host] Position bottom right = up to the LAST BAR, not the empty space");
        const std::vector<std::string> T = c.texts();
        auto at = [&](const std::string& s) { return std::find(T.begin(), T.end(), s) != T.end(); };
        CHECK(S && at("BL5") && at("BL10") && at("BL20") && at(bba::boxValue(S->blNight, 0)) && at(bba::boxValue(S->blNight, 2)), "[host] 3 rows with the overnight averages");
        CHECK(S && bba::boxValue(S->blNight, 0) != "--" && bba::boxValue(S->blDay, 0) != "--", "[host] both types have averages (10 complete sessions)");
        size_t nBox = 0; for (const auto& d : c.H.draws) if (d.type == "text" && r && d.l >= r->l && d.r <= r->r + 2 && d.t >= r->t && d.b <= r->b) ++nBox;
        CHECK(nBox == 6, "[host] only 6 strings in the box (3 labels + 3 values), nothing else");
        // the HOD/LOD table published (lower half): docked to its right, bottom-aligned
        const bdk::Box tb = bdk::mk(400, 640, 900, 790);
        bdk::publishHodLod("ESZ6", 180, "ES 3m", steadyMs(), tb);
        c.pump(1, true); r = c.boxRect();
        CHECK(r && r->l == 904 && r->b == 790 && S->blOn, "[host] docked right of the HOD/LOD table, same bottom, 4 px gap");
        if (r) CHECK(!bdk::overlap(bdk::mk(r->l, r->t, r->r, r->b), tb), "[host] never over the table");
        // the table moves: the box follows
        bdk::publishHodLod("ESZ6", 180, "ES 3m", steadyMs() + 1, bdk::mk(400, 30, 900, 150));
        c.pump(1, true); r = c.boxRect();
        CHECK(r && r->l == 904 && r->t == 30, "[host] the table moved to the top: the box follows (top-aligned)");
        // a table that ends at the last bar: the box goes left
        bdk::publishHodLod("ESZ6", 180, "ES 3m", steadyMs() + 2, bdk::mk(600, 640, lastX - 10, 790));
        c.pump(1, true); r = c.boxRect();
        CHECK(r && r->r == 596 && r->b == 790, "[host] no room right of the table (last bar): left of it");
        // hover over the box
        hb::fakeCursor() = 77;
        if (r) { S->msHas = true; S->msH = (short)((r->l + r->r) / 2); S->msV = (short)((r->t + r->b) / 2); S->msWnd = hb::hbCursor();
                 RTX_EVENT ev; ev.type = E_MOUSE_MOVE; ev.v.mouse.h = S->msH; ev.v.mouse.v = S->msV; c.x->mouse(&ev); }
        CHECK(BBA_HOVERS ? S->hovKey == "BLBOX" : S->hovKey.empty(), (BBA_HOVERS ? "[host] the mouse over the box" : "[host] (1.17.1) hovers off: the mouse over the box is ignored"));
        c.pump(1, true);
        bool q = false, pr = false; for (const auto& t : c.texts()) { if (t.find("Question: how big is a really big print") == 0) q = true; if (t.find("prior session") != std::string::npos) pr = true; }
        CHECK(BBA_HOVERS ? (q && pr) : (!q && !pr), (BBA_HOVERS ? "[host] the hover: question, prior session, ..." : "[host] (1.17.1) hovers off: no box hover drawn"));
        if (BBA_HOVERS && !(q && pr)) for (const auto& t : c.texts()) std::cerr << "  text: " << t << "\n";
        S->msHas = false;
        // (1.17.0) a settings file with everything off is IGNORED (no settings): the box still drawn, the file untouched
        bba::Settings off = bba::defaultsFor("ES"); off.statsChart = false; for (int k = 0; k < 4; ++k) off.cat[k].on = false; off.box = false; writeSettings(off);
        const std::string offLine = readSettingsLine();
        c.pump(2, true);
        CHECK(c.boxRect() != nullptr && c.S()->blOn && c.S()->set.box && c.S()->set.cat[0].on && !c.S()->set.cat[3].on, "[fixed host] BigBidAsk.settings-ES.txt is not read: the fixed behaviour stays");
        CHECK(readSettingsLine() == offLine && c.S()->settingsWhy.find("fixed") == std::string::npos, "[fixed host] the old BBA7 file is never written (1.19.0 reads BigBidAsk.settings2-<MKT>.txt)");
        CHECK(c.S()->catN4[3] == 0, "[fixed host] no category 4 number drawn");
        bdk::clearHodLod("ESZ6", 180, "ES 3m");
    }
    {   home("dock162");   // (1.16.2) lsHodLod 2.4.4: table + strip (LRA_HLB_) and the table alone (LRA_HLT_), same stamp
        Chart c(bars); c.pump(10, false);
        const long long ms = steadyMs();
        const bdk::Box full = bdk::mk(400, 576, 680, 790), tab = bdk::mk(400, 601, 680, 790);   // a 21 px strip + 4 px gap above the table
        bdk::publishHodLod("ESZ6", 180, "ES 3m", ms, full); bdk::publishHodLodTable("ESZ6", 180, "ES 3m", ms, tab);
        c.pump(2, true);
        bdk::publishHodLod("ESZ6", 180, "ES 3m", ms + 1, full); bdk::publishHodLodTable("ESZ6", 180, "ES 3m", ms + 1, tab);
        c.pump(1, true);
        const mock::Draw* r = c.boxRect();
        CHECK(r && r->l == 684 && r->t == 601 && r->b == 790, "[host 1.16.2] docked right of the TABLE, its top and bottom (not the strip's)");
        if (r) std::cerr << "  box " << r->l << "," << r->t << "," << r->r << "," << r->b << "\n";
        CHECK(r && !(r->l < full.r && full.l < r->r && r->t < full.b && full.t < r->b), "[host 1.16.2] never over the table or the strip");
        std::vector<int> ys; for (const auto& d : c.H.draws) if (d.type == "text" && r && d.l >= r->l && d.r <= r->r + 2 && d.t >= r->t && d.b <= r->b && (d.text == "BL5" || d.text == "BL10" || d.text == "BL20")) ys.push_back(d.t);
        CHECK(ys.size() == 3 && ys[1] - ys[0] == ys[2] - ys[1] && ys[1] - ys[0] >= 40 && std::abs((ys[0] - r->t) - (r->b - (ys[2] + (ys[1] - ys[0])))) <= 1, "[host 1.16.2] the 3 rows spread evenly over the table's height");
        bdk::clearHodLod("ESZ6", 180, "ES 3m"); bdk::clearHodLodTable("ESZ6", 180, "ES 3m");
    }
    {   home("dialog");   // (1.17.0) the settings window has no controls: opening / OK / Apply / a click never write anything
        Chart c(bars); c.H.inDialog = true; mock::current = &c.H;
        c.x->parmsLoad(); c.x->parmsUpdt(3); c.x->parmsApply();
        struct stat st;
        const std::string fx = std::string(getenv("USERPROFILE")) + "/InvestorRT/rtx/lsFlexLevels/";
        CHECK(stat((fx + "BigBidAsk.settings-ES.txt").c_str(), &st) != 0 && stat((fx + "BigBidAsk.dialog-ES.txt").c_str(), &st) != 0, "[fixed host] the window writes no settings / dialog file");
        mock::current = &c.H;
    }
    {   home("keepout");   // (1.17.0, Rassul 19:58 CT) no number drawn through the HOD/LOD table + strip or the Big Lot box
        Chart c(bars); c.pump(10, false);
        int over = 0, frames = 0;
        for (int it = 0; it < 12; ++it) {
            const int t = 60 + it * 50; const bdk::Box hb = bdk::mk(120 + it * 20, t, 520 + it * 20, t + 160);
            bdk::publishHodLod("ESZ6", 180, "ES 3m", steadyMs() + it, hb);
            c.pump(2, true); ++frames;
            const mock::Draw* r = c.boxRect();
            for (const auto& d : c.H.draws) {
                if (d.type != "text") continue;
                const bool inBl = r && d.l >= r->l && d.r <= r->r + 2 && d.t >= r->t && d.b <= r->b;
                if (inBl) continue;                                     // the Big Lot box's own labels
                const bdk::Box tx = bdk::mk(d.l, d.t, d.r, d.b);
                if (bdk::overlap(tx, hb) || (r && bdk::overlap(tx, bdk::mk(r->l, r->t, r->r, r->b)))) ++over;
            }
        }
        int moved = 0;
        { BBState* S = c.S(); moved = S ? S->clHl + S->slidHl : 0; std::cerr << "  keep-out host (last frame): not drawn " << (S ? S->clHl : -1) << ", slid " << (S ? S->slidHl : -1) << "\n"; }
        CHECK(moved > 0, "[keepout host] the rule acted (numbers under the box were slid or dropped)");
        CHECK(frames == 12 && over == 0, "[keepout host] 12 table positions: no number over the HOD/LOD box or the Big Lot box (" + std::to_string(over) + ")");
        bdk::clearHodLod("ESZ6", 180, "ES 3m");
    }

}

int main() {
    setenv("TZ", "UTC", 1); tzset();
    boxLogic(); catLogic(); dockLogic(); keepOutLogic(); setLogic(); hostTests();
    {   const std::string src = [] { std::ifstream f("../BigBidAsk.cpp", std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }();
        CHECK(std::string(BBA_VERSION) == "1.23.0" && std::string(bba::VERSION) == "1.23.0" && src.find("p->setVersion(\"1.23.0\")") != std::string::npos, "[ver] 1.21.0 everywhere");
        CHECK(src.find("ss::registerAll(") != std::string::npos && src.find("\"BigBidAsk.settings-\" +") == std::string::npos && bbaset::spec().paramVersion == 10, "[ver] (1.21.0) rows from the table, parameter version 10, the old BBA7 file never opened");
    }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
