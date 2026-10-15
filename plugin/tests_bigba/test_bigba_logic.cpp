// lsBigBidAsk 1.0.0 logic tests (BigBidAskLogic.h). g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra
#include "../BigBidAskLogic.h"
#include <cstdio>
#include <random>
static int fails = 0, passes = 0;
#define CHECK(c) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)
using namespace bba;

static Row R(long long k, long long bid, long long ask) { Row r; r.key = k; r.bid = bid; r.ask = ask; return r; }
static Facts mk(long long t, bool closed, long long o, long long h, long long l, long long c, std::vector<Row> rows) { return facts(t, closed, o, h, l, c, mergeRows(rows), true); }
// a plain candle whose biggest bid / ask are 'b' / 'a' lots
static Facts plain(long long t, long long b, long long a) { return mk(t, true, 100, 104, 96, 101, { R(97, b, 1), R(103, 1, a) }); }

static void testBiggest() {
    // biggest bid / ask picked per side
    std::vector<Row> rows = { R(10, 5, 50), R(11, 80, 3), R(12, 7, 9), R(13, 2, 60) };
    Pick b = biggest(rows, -1, 12), a = biggest(rows, +1, 12);
    CHECK(b.ok && b.key == 11 && b.vol == 80);
    CHECK(a.ok && a.key == 13 && a.vol == 60);
    // ties -> nearest the close
    rows = { R(10, 40, 0), R(14, 40, 0), R(18, 40, 0) };
    CHECK(biggest(rows, -1, 15).key == 14);
    CHECK(biggest(rows, -1, 17).key == 18);
    CHECK(biggest(rows, -1, 9).key == 10);
    // order of rows does not matter
    std::vector<Row> rev(rows.rbegin(), rows.rend());
    CHECK(biggest(rev, -1, 15).key == 14 && biggest(rev, -1, 17).key == 18);
    // equidistant: bid takes the lower price, ask the higher
    rows = { R(10, 30, 30), R(14, 30, 30) };
    CHECK(biggest(rows, -1, 12).key == 10);
    CHECK(biggest(rows, +1, 12).key == 14);
    rev.assign(rows.rbegin(), rows.rend());
    CHECK(biggest(rev, -1, 12).key == 10 && biggest(rev, +1, 12).key == 14);
    // a side with no lots -> no number
    rows = { R(10, 0, 5), R(11, 0, 7) };
    CHECK(!biggest(rows, -1, 10).ok);
    CHECK(biggest(rows, +1, 10).vol == 7);
    // duplicate keys merged (a float price twice for one tick), empty rows dropped
    std::vector<Row> m = mergeRows({ R(5, 10, 1), R(5, 15, 2), R(6, 0, 0), R(4, 20, 0) });
    CHECK(m.size() == 2 && m[1].key == 5 && m[1].bid == 25 && m[1].ask == 3);
    CHECK(biggest(m, -1, 4).key == 5 && biggest(m, -1, 4).vol == 25);
    // volumes above 2^31
    rows = { R(1, 3000000000LL, 0), R(2, 2999999999LL, 0) };
    CHECK(biggest(rows, -1, 2).key == 1 && biggest(rows, -1, 2).vol == 3000000000LL);
}

static void testRatioPastOnly() {
    std::vector<Facts> v;
    for (int i = 0; i < 25; ++i) v.push_back(plain(1000 + i, 100, 50));
    v.push_back(plain(2000, 250, 49));                     // index 25
    rate(v, 25);
    CHECK(v[25].bidQ == 250);                              // 250 / median 100
    CHECK(v[25].askQ == 98);
    CHECK(bold(v[25].bidQ, 200) && !bold(v[25].askQ, 200));
    // the candle itself and LATER candles never change it
    v.push_back(plain(2001, 99999, 99999));
    rate(v, 25);
    CHECK(v[25].bidQ == 250);
    v[25].bid.vol = 250; std::vector<Facts> w(v.begin(), v.begin() + 26); rate(w, 25); CHECK(w[25].bidQ == 250);
    // only the prior 20: a huge candle 21 back does not count
    std::vector<Facts> u;
    u.push_back(plain(1, 100000, 100000));
    for (int i = 0; i < 20; ++i) u.push_back(plain(10 + i, 100, 100));
    u.push_back(plain(99, 200, 100));
    rate(u, 21);
    CHECK(u[21].bidQ == 200 && u[21].askQ == 100);
    // under 10 prior candles with data = n/a, never bold
    std::vector<Facts> s;
    for (int i = 0; i < 9; ++i) s.push_back(plain(i + 1, 10, 10));
    s.push_back(plain(50, 1000, 1000)); rate(s, 9);
    CHECK(s[9].bidQ == -1 && !bold(s[9].bidQ, 120));
    // invalid (no VAP) priors are skipped
    std::vector<Facts> x;
    for (int i = 0; i < 20; ++i) { Facts f = plain(i + 1, 100, 100); if (i % 2) f.valid = false; x.push_back(f); }
    x.push_back(plain(99, 300, 100)); rate(x, 20);
    CHECK(x[20].bidQ == 300);
    std::vector<Facts> y;
    for (int i = 0; i < 20; ++i) { Facts f = plain(i + 1, 100, 100); if (i % 3) f.valid = false; y.push_back(f); }
    y.push_back(plain(99, 300, 100)); rate(y, 20);
    CHECK(y[20].bidQ == -1);                               // 7 valid priors only
    // even count median
    std::vector<Facts> e;
    for (int i = 0; i < 20; ++i) e.push_back(plain(i + 1, i < 10 ? 100 : 300, 100));
    e.push_back(plain(99, 400, 100)); rate(e, 20);
    CHECK(e[20].bidQ == 200);                               // median (100 + 300) / 2 = 200 -> 2.0x
    // exactly at the threshold is bold (>=), one hundredth under is not
    CHECK(bold(200, 200) && !bold(199, 200));
    CHECK(toQ(2.0) == 200 && toQ(1.995) == 200 && toQ(1.994) == 199 && toQ(NAN) == -1);
}

static void testAbsorption() {
    const int Q = 200;
    // bullish: bold red in the lower half, green close above it -> +1
    Facts F = mk(1, true, 100, 108, 100, 105, { R(101, 400, 10), R(106, 20, 30) }); F.bidQ = 310; F.askQ = 100;
    CHECK(absorption(F, Q) == +1);
    // at exactly the halfway mark is allowed (lower half includes it): high 108 low 100 -> mid 104
    Facts G = mk(1, true, 100, 108, 100, 105, { R(104, 400, 10) }); G.bidQ = 300; CHECK(absorption(G, Q) == +1);
    // upper half -> no circle
    Facts U = mk(1, true, 100, 108, 100, 107, { R(105, 400, 10) }); U.bidQ = 300; CHECK(absorption(U, Q) == 0);
    // closes red (the selling won) -> no circle (mockup 270)
    Facts D = mk(1, true, 105, 108, 100, 101, { R(101, 400, 10) }); D.bidQ = 300; CHECK(absorption(D, Q) == 0);
    // doji (close == open) -> no circle
    Facts J = mk(1, true, 103, 108, 100, 103, { R(101, 400, 10) }); J.bidQ = 300; CHECK(absorption(J, Q) == 0);
    // green but closes AT the bid price (not above it) -> no circle
    Facts A = mk(1, true, 100, 108, 99, 101, { R(101, 400, 10) }); A.bidQ = 300; CHECK(absorption(A, Q) == 0);
    // not bold -> no circle
    Facts N = F; N.bidQ = 199; CHECK(absorption(N, Q) == 0);
    Facts NA = F; NA.bidQ = -1; CHECK(absorption(NA, Q) == 0);
    // forming candle -> never
    Facts Fm = F; Fm.closed = false; CHECK(absorption(Fm, Q) == 0);
    // bearish mirror: bold green in the upper half, red close below it -> -1 (mockup 320)
    Facts B = mk(1, true, 106, 108, 100, 102, { R(107, 10, 320), R(101, 30, 20) }); B.askQ = 280; B.bidQ = 100;
    CHECK(absorption(B, Q) == -1);
    Facts B2 = mk(1, true, 106, 108, 100, 102, { R(103, 10, 320) }); B2.askQ = 280; CHECK(absorption(B2, Q) == 0);   // lower half
    Facts B3 = mk(1, true, 101, 108, 100, 104, { R(107, 10, 320) }); B3.askQ = 280; CHECK(absorption(B3, Q) == 0);   // closes green
    // whole ticks only: odd range halfway (high 107 low 100 -> 103.5): 103 is below, 104 above
    Facts H1 = mk(1, true, 100, 107, 100, 106, { R(103, 400, 0) }); H1.bidQ = 300; CHECK(absorption(H1, Q) == +1);
    Facts H2 = mk(1, true, 100, 107, 100, 106, { R(104, 400, 0) }); H2.bidQ = 300; CHECK(absorption(H2, Q) == 0);
    // the threshold setting applies: 2.5x needs 250
    CHECK(absorption(F, 250) == +1 && absorption(F, 320) == 0);
}

static void testSettings() {
    Settings s; s.font = 14; s.boldQ = 275; s.circle = false; s.nonBold = false;
    std::string why; Settings t = parseSettings(formatSettings(s) + "\n", why);
    CHECK(why.empty() && t.font == 14 && t.boldQ == 275 && !t.circle && !t.nonBold);
    Settings d; t = parseSettings(formatSettings(d), why);
    CHECK(t.font == 11 && t.boldQ == 200 && t.circle && t.nonBold);
    // garbage -> defaults + a reason
    const char* bad[] = { "", "BBA1|11|200|1", "BBA2|11|200|1|1", "BBA1|x|200|1|1", "BBA1|11|2.0|1|1", "BBA1|11|200|2|1", "BBA1|-11|200|1|1",
                          "BBA1|11|200|1|1|9", "BBA1| 11|200|1|1", "BBA1|99999999999999999999|200|1|1" };
    for (const char* b : bad) { t = parseSettings(b, why); CHECK(!why.empty() && t.font == 11 && t.boldQ == 200 && t.circle && t.nonBold); }
    t = parseSettings("BBA1|40|200|0|1", why); CHECK(t.font == 11 && !t.circle && !why.empty());
    t = parseSettings("BBA1|11|5000|1|0", why); CHECK(t.boldQ == 200 && !t.nonBold && !why.empty());
    t = parseSettings("BBA1|8|120|1|1\r\n", why); CHECK(why.empty() && t.font == 8 && t.boldQ == 120);
    CHECK(fontOk(8) && fontOk(20) && !fontOk(7) && !fontOk(21) && boldOk(120) && boldOk(1000) && !boldOk(119) && !boldOk(1001));
}

static void testRecord() {
    Facts F = mk(1792000000, true, 100, 108, 100, 105, { R(101, 400, 10), R(106, 20, 30) }); F.bidQ = 310; F.askQ = 100;
    Facts G; CHECK(parseRec(recLine(F), G));
    CHECK(G.t == F.t && G.valid && G.closed && G.o == 100 && G.h == 108 && G.l == 100 && G.c == 105 && G.bid.key == 101 && G.bid.vol == 400 &&
          G.ask.key == 106 && G.ask.vol == 30 && G.bidQ == 310 && G.askQ == 100);
    CHECK(absorption(G, 200) == absorption(F, 200) && hover(G, 0.25, 200, "9:00 AM", -1) == hover(F, 0.25, 200, "9:00 AM", -1));
    Facts H = mk(5, true, -3, 2, -8, -1, { R(-7, 3000000000LL, 0) }); H.bidQ = -1;
    CHECK(parseRec(recLine(H), G) && G.bid.key == -7 && G.bid.vol == 3000000000LL && !G.ask.ok && G.bidQ == -1);
    const char* bad[] = { "", "1|1|1|1|1|1|1|1|1|1|1", "x|1|100|108|100|105|101|400|310|106|30|100", "0|1|100|108|100|105|101|400|310|106|30|100",
                          "1|2|100|108|100|105|101|400|310|106|30|100", "1|1|100|99|100|105|101|400|310|106|30|100", "1|1|100|108|100|105|101|-4|310|106|30|100",
                          "1|1|100|108|100|105|101|400|-2|106|30|100", "1|1|100|108|100|105|0|0|310|106|30|100" };
    for (const char* b : bad) CHECK(!parseRec(b, G));
}

static void testText() {
    CHECK(price(30965, 0.25) == "7,741.25");
    CHECK(price(40000, 0.1) == "4,000.0");
    CHECK(price(9001, 0.0005) == "4.5005");
    CHECK(price(23450, 0.00005) == "1.17250");
    CHECK(price(3456, 0.001) == "3.456");
    CHECK(price(6512, 0.01) == "65.12");
    CHECK(price(-4, 0.25) == "-1.00");
    CHECK(commas(1240) == "1,240" && commas(999) == "999" && commas(1234567) == "1,234,567");
    CHECK(lots(410) == "410" && lots(12345) == "12345" && lots(123456) == "123k");
    CHECK(ratioText(310) == "3.1x" && ratioText(-1) == "n/a" && ratioText(200) == "2.0x");
    CHECK(clock12(0, 5) == "12:05 AM" && clock12(13, 54) == "1:54 PM" && clock12(12, 0) == "12:00 PM" && clock12(24, 0) == "?");
    // the hover: Rassul's line 1, every row short enough (<= 125 characters, BUILD-LESSONS #83), absorption explained when circled
    std::vector<Row> rows = { R(30965, 410, 10), R(30974, 20, 160) };
    Facts F = facts(1, true, 30970, 30976, 30964, 30972, rows, true); F.bidQ = 310; F.askQ = 120;
    std::vector<std::string> L = hover(F, 0.25, 200, "8:48 AM", -1);
    CHECK(L[0] == "Biggest bid 7,741.25: 410 lots sold at the bid (3.1x normal) \xB7 biggest ask 7,743.50: 160 lots (1.2x)");
    CHECK(L[2].find("ABSORPTION") == 0 && L.size() == 10);
    for (const auto& s : L) CHECK(s.size() <= 125);
    bool hasQ = false, hasUse = false, hasEx = false, hasWhy = false;
    for (const auto& s : L) { hasQ |= s.find("Question:") == 0; hasUse |= s.find("Use:") == 0; hasEx |= s.find("Example:") == 0; hasWhy |= s.find("Why:") == 0; }
    CHECK(hasQ && hasUse && hasEx && hasWhy);
    CHECK(hover(F, 0.25, 200, "8:48 AM", -1, false).size() == 8);   // circle off: no absorption lines
    Facts Fm = F; Fm.closed = false; CHECK(hover(Fm, 0.25, 200, "x", 1).size() == 8 && hover(Fm, 0.25, 200, "x", 1)[1].find("forming") != std::string::npos);
}

static Cand C(int bar, int side, int x, int y, int w, bool bold = false, bool circle = false) { Cand c; c.bar = bar; c.side = side; c.x = x; c.y = y; c.w = w; c.h = 14; c.bold = bold; c.circle = circle; return c; }
static void testPlace() {
    Box area; area.l = 0; area.t = 20; area.r = 1000; area.b = 800;
    // two numbers that would touch: the second is nudged one row (up first)
    std::vector<Cand> c = { C(0, +1, 100, 400, 20), C(1, -1, 140, 400, 20) };
    LayoutStats st; std::vector<Placed> P = place(c, {}, area, &st);
    CHECK(P[0].shown && P[0].dy == 0 && P[1].shown && P[1].dy == -15 && st.nudged == 1);
    CHECK(!touch(P[0].foot, P[1].foot));
    // up blocked too -> down
    c = { C(0, +1, 100, 400, 20), C(0, +1, 100, 385, 20), C(1, -1, 140, 400, 20) };
    P = place(c, {}, area, &st);
    CHECK(P[2].shown && P[2].dy == 15);
    // all three rows blocked -> a non-bold number is not drawn
    c = { C(0, +1, 100, 400, 20), C(0, +1, 100, 385, 20), C(0, +1, 100, 415, 20), C(1, -1, 140, 400, 20) };
    P = place(c, {}, area, &st);
    CHECK(!P[3].shown && st.dropped == 1);
    // never on a candle: a candle box where the number would go -> nudged off it
    Box candle; candle.l = 120; candle.r = 128; candle.t = 380; candle.b = 405;
    c = { C(0, +1, 100, 400, 20) };
    P = place(c, { candle }, area, &st);
    CHECK(P[0].shown && P[0].dy == 15 && !touch(P[0].foot, candle));
    // the candle covers all three rows: a bold number still shows on its own row (it touches no other number), a plain one does not
    candle.t = 300; candle.b = 500;
    c = { C(0, +1, 100, 400, 20, true) }; P = place(c, { candle }, area, &st);
    CHECK(P[0].shown && P[0].crowded && P[0].dy == 0 && st.crowded == 1);
    c = { C(0, +1, 100, 400, 20, false) }; P = place(c, { candle }, area, &st);
    CHECK(!P[0].shown);
    // a bold one never covers another number
    c = { C(0, +1, 100, 400, 20), C(0, +1, 100, 385, 20), C(0, +1, 100, 415, 20), C(1, -1, 140, 400, 20, true) };
    P = place(c, {}, area, &st);
    CHECK(!P[3].shown);
    // the area: nothing right of the price area (the columns), nothing in the title row
    Box narrow = area; narrow.r = 115;
    c = { C(0, +1, 100, 400, 20, true) }; P = place(c, {}, narrow, &st); CHECK(!P[0].shown);
    c = { C(0, +1, 100, 25, 20, true) }; P = place(c, {}, area, &st); CHECK(P[0].shown && P[0].text.t >= 20);
    // older first: appending a newer number never moves or removes an older one of its group (lesson #49); a plain number never
    // takes a row from a closed candle's bold one; red left / green right of the candle
    std::mt19937 rng(7);
    for (int trial = 0; trial < 3000; ++trial) {
        std::vector<Cand> cs; std::vector<Box> obs;
        const int n = 2 + (int)(rng() % 40);
        for (int i = 0; i < n; ++i) {
            const int bar = i / 2, side = i % 2 ? +1 : -1, xl = 40 + bar * 24, xr = xl + 8, y = 100 + (int)(rng() % 300);
            if (side < 0) { Box b; b.l = xl; b.r = xr; b.t = y - (int)(rng() % 40); b.b = y + (int)(rng() % 40); obs.push_back(b); }
            Cand k = C(bar, side, side < 0 ? xl : xr, y, 12 + (int)(rng() % 20), rng() % 4 == 0, false);
            k.circle = k.bold && rng() % 3 == 0; k.first = k.bold; cs.push_back(k);
        }
        std::vector<Placed> all = place(cs, obs, area);
        std::vector<Cand> pre(cs.begin(), cs.end() - 1);
        std::vector<Placed> before = place(pre, obs, area);
        bool same = true;
        for (size_t i = 0; i < pre.size(); ++i) {
            const bool mustKeep = cs.back().first ? pre[i].first : true;     // a new plain number changes nothing; a new bold one no older bold one
            if (mustKeep && (before[i].shown != all[i].shown || before[i].dy != all[i].dy)) same = false;
        }
        CHECK(same);
        bool clean = true;
        for (size_t i = 0; i < all.size(); ++i) {
            if (!all[i].shown) continue;
            if (cs[i].side < 0 && all[i].text.r >= cs[i].x) clean = false;
            if (cs[i].side > 0 && all[i].text.l <= cs[i].x) clean = false;
            for (size_t j = i + 1; j < all.size(); ++j) if (all[j].shown && touch(all[i].foot, all[j].foot)) clean = false;
            if (!all[i].crowded) for (const Box& o : obs) if (touch(all[i].foot, o)) clean = false;
            if (!inside(all[i].foot, area)) clean = false;
        }
        CHECK(clean);
    }
    // a closed candle's bold (circled) number wins the spot over an OLDER plain one
    {   Cand plainOld = C(0, +1, 100, 400, 20), boldNew = C(1, -1, 140, 400, 20, true, true); boldNew.first = true;
        Cand up = C(0, +1, 100, 385, 20), dn = C(0, +1, 100, 415, 20);
        std::vector<Cand> v = { plainOld, up, dn, boldNew };
        P = place(v, {}, area, &st);
        CHECK(P[3].shown && P[3].dy == 0 && !P[3].crowded && !P[0].shown);
        boldNew.first = false; v[3] = boldNew; P = place(v, {}, area, &st);   // the forming candle's bold: no priority
        CHECK(!P[3].shown && P[0].shown);
    }
    // another indicator's box (the Tape Reader banner): never covered, not even by a bold number without room
    Box banner; banner.l = 0; banner.t = 380; banner.r = 1000; banner.b = 430;
    c = { C(0, +1, 100, 400, 20, true) }; P = place(c, {}, area, &st, { banner }); CHECK(!P[0].shown);
    c = { C(0, +1, 100, 436, 20, false) }; P = place(c, {}, area, &st, { banner }); CHECK(P[0].shown && P[0].dy == 15);
    // the circle hugs the number (1 px rounded outline round the text, 3 px aside)
    c = { C(0, -1, 300, 400, 20, true, true) }; P = place(c, {}, area, &st);
    CHECK(P[0].foot.l == P[0].text.l - 3 && P[0].foot.r == P[0].text.r + 3 && P[0].foot.t == P[0].text.t - 1 && P[0].foot.r < 300);
}

int main() {
    testBiggest(); testRatioPastOnly(); testAbsorption(); testSettings(); testRecord(); testText(); testPlace();
    std::printf("bigba logic: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
