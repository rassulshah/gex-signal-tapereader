// test_exhaustion_051.cpp -- lsExhaustion 0.5.1 (Rassul 13:02 CT: one info box at the pane's left = explanation / hovered candle's values,
// no fading ratio labels) on top of the 0.5.0 suite. Was: test_exhaustion_050.cpp -- lsExhaustion 0.5.0 THE BOOK (Rassul approved 2026-10-29 12:38 CT, final_exh.html / fx/final.py) through the
// REAL Exhaustion.cpp on the MBO-capable mock (tests_lqp044/irtsdk.h): the book momentum + S / P letters vs the approved generator on its
// own fixture (ES Oct 28 05:21-09:09, tests_exhaustion/exh050_fixture.txt), the eligibility rule, the 1 px light grey line, the info box /
// hover lines <= 60 chars (separate draw calls), the book counts from the order feed (adds / pulls per candle), no repaint after a restart
// (book file read back), and the feed-missing path ("order book: n/a", no line, no letters).
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Itests_lqp044 -I. test_exhaustion_050.cpp -o t && ASAN_OPTIONS=detect_leaks=0 ./t
#include "Exhaustion.cpp"
#include <cstdio>
#include <unistd.h>
#include <sstream>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
struct FX { std::string t; double net, ab, aa, pb, pa, mom; std::string let; double sv, sx, pv, px; };
static std::vector<FX> loadFx() {
    std::vector<FX> v; std::ifstream in("tests_exhaustion/exh050_fixture.txt"); std::string ln;
    while (std::getline(in, ln)) { if (ln.empty() || ln[0] == '#') continue; std::vector<std::string> f; std::string c; std::istringstream ss(ln); while (std::getline(ss, c, '|')) f.push_back(c); if (ln.back() == '|') f.push_back("");
        if (f.size() < 12) continue; FX x; x.t = f[0]; x.net = std::stod(f[1]); x.ab = std::stod(f[2]); x.aa = std::stod(f[3]); x.pb = std::stod(f[4]); x.pa = std::stod(f[5]); x.mom = std::stod(f[6]); x.let = f[7];
        x.sv = std::stod(f[8]); x.sx = std::stod(f[9]); x.pv = std::stod(f[10]); x.px = std::stod(f[11]); v.push_back(x); }
    return v; }
static std::string readAll2(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void bar(mock::Host& h, unsigned long t, long buy, long sell) { mock::Bar b; b.time = t; b.open = 6999.5f; b.high = 7001.0f; b.low = 6999.0f; b.close = 7000.0f; b.rows.push_back(mock::Price(7000.0f, buy, sell, buy + sell)); h.bars.push_back(b); }
int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    const std::vector<FX> F = loadFx();
    CHECK(F.size() == 77, "fixture: 77 bars from the generator");
    {   // the logic vs the generator
        std::vector<double> net; std::vector<ex::BookBar> bb; for (const FX& x : F) { net.push_back(x.net); ex::BookBar b; b.have = true; b.addB = x.ab; b.addA = x.aa; b.pullB = x.pb; b.pullA = x.pa; bb.push_back(b); }
        const ex::BookOut o = ex::computeBook(net, bb);
        int momBad = 0, letBad = 0, xBad = 0, nLet = 0;
        for (size_t k = 0; k < F.size(); ++k) { if (k >= 3 && (!o.momOk[k] || std::fabs(o.mom[k] - F[k].mom) > 1e-4)) ++momBad; if (o.let[k] != F[k].let) ++letBad; if (!o.let[k].empty()) ++nLet;
            if (std::fabs(o.sX[k] - F[k].sx) > 1e-4 || std::fabs(o.pX[k] - F[k].px) > 1e-4 || o.sVal[k] != F[k].sv || o.pVal[k] != F[k].pv) ++xBad; }
        std::printf("  vs generator: momentum mismatches %d, letter mismatches %d, ratio mismatches %d, letters %d\n", momBad, letBad, xBad, nLet);
        CHECK(momBad == 0 && letBad == 0 && xBad == 0 && nLet == 13, "book momentum, S / P ratios and letters = the approved generator (13 letters)");
        CHECK(!o.momOk[0] && !o.momOk[2] && o.momOk[3], "momentum from the 4th bar of a run (EMA5[k] - EMA5[k-3])");
        // eligibility: flip, or a bigger bar of the same colour; nothing on a smaller same-colour bar
        std::vector<double> n2 = { 100, -50, -80, -60, 40 }; std::vector<ex::BookBar> b2(5); for (auto& b : b2) { b.have = true; b.addB = b.addA = b.pullB = b.pullA = 100; }
        const ex::BookOut e2 = ex::computeBook(n2, b2);
        CHECK(!e2.elig[0] && e2.elig[1] && e2.elig[2] && !e2.elig[3] && e2.elig[4], "eligible: colour flip (1, 4) or |net| above the previous same-colour bar (2), not a smaller one (3)");
        // a gap (no book) restarts the EMA; no letters without book
        std::vector<ex::BookBar> b3 = bb; b3[40].have = false; const ex::BookOut o3 = ex::computeBook(net, b3);
        CHECK(!o3.momOk[40] && !o3.momOk[43] && o3.momOk[44] && o3.let[40].empty(), "no book on a bar = no momentum there (the EMA restarts) and no letter");
        size_t mx = 0; for (int q = 0; q < ex::INFO_N; ++q) mx = std::max(mx, std::string(ex::INFO_LINES[q]).size());
        CHECK(mx <= 60 && ex::INFO_N == 26 && std::string(ex::INFO_LINES[0]) == "What it answers: is the side pushing price running" && std::string(ex::INFO_LINES[2]).empty(), "info box = the mockup's 26 lines (<= 60 chars, blank spacers)");
    }
    // ---- host: the order feed -> per-candle adds / pulls -> line / letters / hover (the fixture's counts sent as events)
    for (int pass = 0; pass < 2; ++pass) {
        char tmpl[] = "/tmp/ex50XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
        mock::feed = mock::Feed(); mock::feed.subscribe = pass == 0;      // pass 1: no order feed
        mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ6"; h.root = "ES"; h.chart = "ES 3m #1";
        h.priceBase = 6990.0f; h.pxPer = 20.0f; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
        const unsigned long day = (unsigned long)(civilDays(2026, 10, 28) * 86400LL);
        unsigned long t = day + 5 * 3600 + 18 * 60;   // bar 0 (partial, the feed's first candle) ends 5:18; the fixture bars follow
        bar(h, t, 500, 500);
        for (size_t k = 0; k < F.size(); ++k) bar(h, t + 180 * (k + 1), (long)(500 + std::max(0.0, F[k].net)), (long)(500 + std::max(0.0, -F[k].net)));
        bar(h, t + 180 * (F.size() + 1), 10, 10);                         // forming
        int id = 1; std::vector<std::pair<int, int> > rest(F.size());       // per bar: a resting bid / offer placed in bar 0, pulled in its bar
        for (size_t k = 0; k < F.size(); ++k) { if (F[k].pb > 0) { mock::feed.push(0, 1, 7000.0f, (unsigned long)F[k].pb, (RTDATE)(t - 100), id); rest[k].first = id++; }
            if (F[k].pa > 0) { mock::feed.push(0, 0, 7000.25f, (unsigned long)F[k].pa, (RTDATE)(t - 100), id); rest[k].second = id++; } }
        for (size_t k = 0; k < F.size(); ++k) { const RTDATE s = (RTDATE)(t + 180 * (k + 1) - 90);
            if (F[k].ab > 0) mock::feed.push(0, 1, 7000.0f, (unsigned long)F[k].ab, s, id++);
            if (F[k].aa > 0) mock::feed.push(0, 0, 7000.25f, (unsigned long)F[k].aa, s, id++);
            if (rest[k].first) mock::feed.push(2, 1, 7000.0f, 0, s, rest[k].first);
            if (rest[k].second) mock::feed.push(2, 0, 7000.25f, 0, s, rest[k].second); }
        h.now = t + 180 * (F.size() + 1) - 60;
        Exhaustion* p = static_cast<Exhaustion*>(CreateExtension());
        for (int s = 0; s < 4; ++s) { p->calc(0); usleep(20000); }
        usleep(5200000); p->calc(0); p->calc(0);
        EXState* S = p->testState();
        h.draws.clear(); p->draw();
        int grey = 0, greyW = 0; for (auto& x : h.draws) if (x.type == "line" && x.pen == (COLOR)0xD1D5DB) { ++grey; greyW = x.penW; }
        int lets = 0; for (auto& x : h.draws) if (x.type == "text" && (x.text == "S" || x.text == "P" || x.text == "SP")) ++lets;
        if (pass == 0) {
            int cntBad = 0; for (size_t k = 0; k < F.size(); ++k) { auto f = S->bookBars.find((long long)h.bars[k + 1].time); if (f == S->bookBars.end() || !f->second.have || f->second.addB != F[k].ab || f->second.addA != F[k].aa || f->second.pullB != F[k].pb || f->second.pullA != F[k].pa) ++cntBad; }
            auto b0 = S->bookBars.find((long long)h.bars[0].time);
            std::printf("  feed: %s; book bars %zu, count mismatches %d, grey segments %d (w %d), letters drawn %d\n", S->feedState.c_str(), S->bookBars.size(), cntBad, grey, greyW, lets);
            CHECK(cntBad == 0, "book counts per candle from the order feed = the fixture (adds / pulls near price, both sides)");
            CHECK(b0 == S->bookBars.end() || !b0->second.have, "the feed's first (partial) candle has no book");
            CHECK(grey >= 20 && greyW == 1, "the book momentum line: 1 px light grey (0xD1D5DB)");
            int letBad = 0; for (size_t k = 0; k < F.size(); ++k) if (S->book.let[(size_t)((int)k + 1 - S->outFirst)] != F[k].let) ++letBad;
            CHECK(letBad == 0 && lets >= 10, "letters on the chart = the generator's");
            // hover on the 08:48 bar
            size_t k848 = 0; for (size_t k = 0; k < F.size(); ++k) if (F[k].t == "08:48") k848 = k;
            RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = 300; e.v.mouse.v = 500; e.v.mouse.bar = (long)(k848 + 1); p->mouse(&e); h.draws.clear(); p->draw();
            bool hs = false, hp = false, hm = false; size_t longest = 0; for (auto& x : h.draws) if (x.type == "text") { longest = std::max(longest, x.text.size());
                if (x.text.rfind("S  ", 0) == 0 && x.text.find("stacked") != std::string::npos) hs = true; if (x.text.rfind("P  ", 0) == 0 && x.text.find("pulled") != std::string::npos) hp = true; if (x.text.rfind("Book momentum", 0) == 0) hm = true; }
            std::printf("  08:48 hover: S %d P %d mom %d, longest %zu (generator S %.0f %.1fx, P %.0f %.1fx, mom %+.0f)\n", hs, hp, hm, longest, F[k848].sv, F[k848].sx, F[k848].pv, F[k848].px, F[k848].mom);
            char want[96]; std::snprintf(want, sizeof(want), "offers %s lots (%.1fx avg)", ex::lots(F[k848].sv).c_str(), F[k848].sx); bool exact = false; for (auto& x : h.draws) if (x.type == "text" && x.text == want) exact = true;
            CHECK(hs && hp && hm && exact && longest <= 60, "bar hover: S / P / Book momentum rows (label + value), every line <= 60 characters");
            {   // (0.5.1) the SAME box at the pane's left, under the header, left of the middle; a rounded dark panel; title = the candle
                int rb = 0; for (auto& x : h.draws) if (x.type == "rbox") ++rb;
                bool title = false, bought = false; for (auto& x : h.draws) if (x.type == "text") { if (x.text.rfind("Candle ending ", 0) == 0) title = true; if (x.text == "Bought at ask") bought = true; }
                CHECK(rb == 1 && S->boxL == h.paneLeft + 4 && S->boxT == h.paneTop + 17 && S->boxR <= h.paneLeft + (h.paneRight - h.paneLeft) / 2 && S->boxB <= h.paneBottom - 2, "(0.5.1) one rounded box at the pane's top-left, left of the middle, inside the pane");
                CHECK(title && bought, "(0.5.1) the box shows the hovered candle: title 'Candle ending ...', label 'Bought at ask'");
                bool green = false, red = false; for (auto& x : h.draws) if (x.type == "text") { if (x.text.find(" lots") != std::string::npos && x.color == (COLOR)0x22C55E) green = true; if (x.text.find(" lots") != std::string::npos && x.color == (COLOR)0xEF4444) red = true; }
                CHECK(green && red, "(0.5.1) buyers' values green, sellers' red");
                int ratio = 0; for (auto& x : h.draws) if (x.type == "text" && x.text.size() <= 6 && x.text.size() >= 4 && x.text.back() == 'x' && std::isdigit((unsigned char)x.text[0])) ++ratio;
                CHECK(ratio == 2, "(0.5.1) no fading ratio labels in the pane: only the header's two live values"); }
            e.v.mouse.h = 300; e.v.mouse.v = 500; e.v.mouse.bar = -1; p->mouse(&e); h.draws.clear(); p->draw();   // (0.5.1) mouse off the candles: the explanation
            int info = 0; for (auto& x : h.draws) if (x.type == "text") for (int q = 0; q < ex::INFO_N; ++q) if (*ex::INFO_LINES[q] && x.text == ex::INFO_LINES[q]) ++info;
            std::printf("  info lines drawn %d, tipOn %d icon %d..%d\n", info, S->tipOn ? 1 : 0, S->iconL, S->iconR);
            CHECK(info == 20, "info box (default): the 20 non-blank mockup lines, each its own draw call");
            { bool t2 = false; for (auto& x : h.draws) if (x.type == "text" && x.text == "How to read Exhaustion") t2 = true; CHECK(t2 && S->iconR < 0, "(0.5.1) the explanation's title; no (i) icon"); }
            {   // (0.5.1) a short pane: the box shrinks to what fits (never past the pane)
                const int pb = h.paneBottom; h.paneBottom = h.paneTop + 120; h.draws.clear(); p->draw();
                int info2 = 0; for (auto& x : h.draws) if (x.type == "text") for (int q = 0; q < ex::INFO_N; ++q) if (*ex::INFO_LINES[q] && x.text == ex::INFO_LINES[q]) ++info2;
                CHECK(info2 > 0 && info2 < 20 && S->boxB <= h.paneBottom - 2, "(0.5.1) short pane: fewer explanation lines, box inside");
                e.v.mouse.bar = (long)(k848 + 1); p->mouse(&e); h.draws.clear(); p->draw();
                bool b2 = false; for (auto& x : h.draws) if (x.type == "text" && x.text == "Bought at ask") b2 = true;
                CHECK(b2 && S->boxB <= h.paneBottom - 2, "(0.5.1) short pane hover: the bar values first, box inside");
                e.v.mouse.bar = -1; p->mouse(&e); h.paneBottom = pb; }
            // restart: the book is read back from the record -> the same line / letters (no repaint)
            std::vector<std::string> before = S->book.let; std::vector<double> mb = S->book.mom;
            p->destroy(); h.data = nullptr; mock::feed = mock::Feed(); mock::feed.subscribe = false;
            Exhaustion* q = static_cast<Exhaustion*>(CreateExtension()); q->calc(0); q->calc(0); EXState* R = q->testState();
            bool same = R->book.let.size() == before.size(); for (size_t k = 0; same && k < before.size(); ++k) same = R->book.let[k] == before[k] && std::fabs(R->book.mom[k] - mb[k]) < 1e-9;
            for (size_t k = 0; k < before.size() && k < R->book.let.size(); ++k) if (R->book.let[k] != before[k] || std::fabs(R->book.mom[k] - mb[k]) > 1e-9) { std::printf("  first diff at %zu: let '%s' vs '%s', mom %.3f vs %.3f, outFirst %d/%d\n", k, R->book.let[k].c_str(), before[k].c_str(), R->book.mom[k], mb[k], R->outFirst, 0); break; }
            std::printf("  restart: loaded %lld, session %lld, lets %zu/%zu\n", R->bookLoaded, R->session, R->book.let.size(), before.size());
            CHECK(same && R->bookLoaded >= 77, "restart without the feed: the book read back, the same letters and line (no repaint)");
            std::string rec; { std::ifstream in((std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\Exhaustion\\ES-ESZ6-180-net-" + std::to_string(R->session) + ".csv").c_str()); std::stringstream ss; ss << in.rdbuf(); rec = ss.str(); }
            CHECK(rec.find("|bar|net ") != std::string::npos && rec.find("x|P ") != std::string::npos, "every bar's book / S / P with 10- and 20-bar ratios logged in the net record");
            CHECK(std::string(EX_VERSION) == "0.5.1" && std::string(ex::VERSION) == "0.5.1" && readAll2("Exhaustion.cpp").find("p->setVersion(\"0.5.1\")") != std::string::npos, "versions 0.5.1");
            q->destroy();
        } else {
            CHECK(grey == 0 && lets == 0, "no order feed: no momentum line, no letters");
            RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = 300; e.v.mouse.v = 500; e.v.mouse.bar = 40; p->mouse(&e); h.draws.clear(); p->draw();
            bool na = false; for (auto& x : h.draws) if (x.type == "text" && x.text == "n/a (not recorded for this candle)") na = true;
            CHECK(na, "no order feed: the box says Order book n/a (not recorded for this candle)");
            p->destroy();
        }
        h.data = nullptr; mock::current = nullptr;
    }
    std::printf("lsExhaustion %s BOOK: %d passed, %d failed\n", EX_VERSION, passes, fails);
    return fails ? 1 : 0;
}
