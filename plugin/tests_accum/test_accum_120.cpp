// test_accum_120.cpp - AccumLogic.h 1.2.0: the book cells, the quiet end, the other side building, the always-shown read, the file.
#include "AccumLogic.h"
#include <iostream>
static int passed = 0, failed = 0;
#define CHECK(c, m) do { if (c) ++passed; else { ++failed; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
static acc::Bar bar(long long t, long long lo, long long hi, long long c, long long buy, long long sell) { acc::Bar b; b.t = t; b.lo = lo; b.hi = hi; b.close = c; b.buy = buy; b.sell = sell; return b; }
// a fall, then 6 candles holding 1020 under net selling with an absorbed read; book = what to feed during the base (0 none, 1 quiet + thin, 2 hit late + building)
static acc::Engine base(int book, long long& tEnd, int side = 1) {
    acc::Engine E; long long t = 1000000080; auto P = [&](long long p) { return side > 0 ? p : 3000 - p; };
    auto B = [&](long long lo, long long hi, long long c, long long buy, long long sell) { return side > 0 ? bar(t, lo, hi, c, buy, sell) : bar(t, 3000 - hi, 3000 - lo, 3000 - c, sell, buy); };
    if (book) E.onBook(t - 200, 0, true, 900, 1);                                    // the book is known from before the fall
    for (int i = 0; i < 20; ++i) { E.onCandle(B(1100 - i * 4, 1112 - i * 4, 1104 - i * 4, 100, 100)); t += 180; }
    for (int i = 0; i < 6; ++i) {
        const long long t0 = t - 180;                                                 // this candle's start
        if (book == 1) { if (i < 3) E.onBook(t0 + 40, 2, side > 0, P(1021), 30); E.onBook(t0 + 50, 0, side < 0, P(1026), 20); E.onBook(t0 + 70, 1, side < 0, P(1026), 15); }
        if (book == 2) { E.onBook(t0 + 40, 2, side > 0, P(1021), 30); E.onBook(t0 + 50, 0, side < 0, P(1026), 200); E.onBook(t0 + 70, 1, side < 0, P(1026), 20); }
        if (i == 3) E.onAbsorb(t - 60, side > 0, P(1020), 40, 60, 1.5);
        E.onCandle(B(1020 + (i % 2), 1030, 1024, 100, 130)); t += 180; }
    tEnd = t; return E;
}
int main() {
    long long t = 0;
    {   acc::Engine E = base(1, t); CHECK(E.Z.size() == 1 && E.Z[0].state == acc::OPEN, "the base opens a zone");
        const acc::Zone z = E.Z.empty() ? acc::Zone() : E.Z[0];
        CHECK(z.book == 1 && z.lateFill == 0 && acc::quiet(z), "defender fills only in the first two thirds = a quiet end (late " << z.lateFill << ", book " << z.book << ")");
        CHECK(z.offNet > 0 && acc::thin(z) && acc::grade(z) == 2, "offers net +" << z.offNet << " of " << z.total << " lots = not building, grade 2/2");
        const std::vector<acc::Line> L = acc::readLines(z, 0.01, 180, "");
        CHECK(L.size() == 7 && L[0].text == "Accum? 10.20  2/2" && L[4].text == "4. Quiet end" && L[4].col == 1 && L[5].text.compare(0, 11, "5. Offers: ") == 0 && L[5].col == 1 && L[6].text.find("confirms") != std::string::npos, "the full read: " << (L.size() > 5 ? L[5].text : ""));
        const std::vector<acc::Line> C = acc::readCompact(z, 0.01, 180, "");
        CHECK(C.size() == 3 && C[0].text == L[0].text && C[1].text.find("Quiet") != std::string::npos && C[2].text == L[6].text, "the 3-line read: " << (C.size() > 1 ? C[1].text : ""));
        const std::string f = acc::fileText(E, "ES", 0.25); const std::vector<acc::Zone> R = acc::parseFile(f);
        CHECK(R.size() == 1 && acc::zLine(R[0]) == acc::zLine(z) && R[0].book == 1 && R[0].offNet == z.offNet && R[0].band == z.band, "the file carries the readings");
        E.onCandle(bar(t, 1010, 1022, 1012, 50, 300)); t += 180;                     // closes through the absorbed price
        CHECK(E.Z[0].state == acc::BROKEN && E.current() == nullptr && E.last() == &E.Z[0], "broken: no current zone, last() still gives it");
        const std::vector<acc::Line> X = acc::readLines(E.Z[0], 0.01, 180, "9:03 AM");
        CHECK(X.back().text == "Broken 9:03 AM" && X.back().col == -1 && X[0].col == -1 && X[0].text.compare(0, 11, "Accum 10.20") == 0, "the read of a broken zone: " << X[0].text << " / " << X.back().text);
        const acc::Zone d = E.Z[0]; E.onBook(t + 5, 2, true, 1021, 500); E.onCandle(bar(t, 1000, 1012, 1004, 10, 10));
        CHECK(acc::zLine(E.Z[0]) == acc::zLine(d), "a decided zone's readings never change"); }
    {   acc::Engine E = base(2, t); const acc::Zone z = E.Z.empty() ? acc::Zone() : E.Z[0];
        CHECK(E.Z.size() == 1 && z.book == 1 && z.lateFill > 0 && !acc::quiet(z) && !acc::thin(z) && acc::grade(z) == 0, "the low still hit at the end and offers building = 0/2 (late " << z.lateFill << ", off " << z.offNet << ")");
        const std::vector<acc::Line> L = acc::readLines(z, 0.01, 180, "");
        CHECK(L.size() == 7 && L[0].text == "Accum? 10.20  0/2" && L[4].text.compare(0, 18, "4. Low still hit: ") == 0 && L[4].col == -1 && L[5].col == -1, "its read: " << (L.size() > 4 ? L[4].text : "")); }
    {   acc::Engine E = base(0, t); const acc::Zone z = E.Z.empty() ? acc::Zone() : E.Z[0];
        CHECK(E.Z.size() == 1 && z.book == 0 && acc::grade(z) == 0 && !acc::quiet(z), "no book (a restart): the readings are n/a, never a pass");
        CHECK(acc::readCompact(z, 0.01, 180, "")[1].text == "Abs 1.5x  Book n/a", "the 3-line read without the book"); }
    {   acc::Engine E = base(1, t, -1); const acc::Zone z = E.Z.empty() ? acc::Zone() : E.Z[0];
        CHECK(E.Z.size() == 1 && z.side == -1 && z.book == 1 && acc::quiet(z) && acc::thin(z), "the mirror: distribution, quiet end, bids not building (late " << z.lateFill << ", off " << z.offNet << ")");
        const std::vector<acc::Line> L = acc::readLines(z, 0.01, 180, ""); CHECK(L.size() == 7 && L[0].text.compare(0, 9, "Distrib? ") == 0 && L[5].text.compare(0, 9, "5. Bids: ") == 0 && L[4].col == -1, "its read"); }
    {   acc::Zone z; CHECK(acc::parseZ("Z|1.1.0|6|1|2|3877662780|3877664040|3877664040|30025|30155|30076|30150|5236|198118|21|72|949|2.3592|3|30046", z) && z.book == 0 && z.id == 6, "a 1.1.0 line (20 fields) still reads");
        CHECK(!acc::parseZ("Z|1.2.0|6|1|2|3877662780|3877664040|3877664040|30025|30155|30076|30150|5236|198118|21|72|949|2.3592|3|30046|5|0|x|1", z) && !acc::parseZ("Z|1.2.0|6|1|2|3877662780|3877664040|3877664040|30025|30155|30076|30150|5236|198118|21|72|949|2.3592|3|30046|5|0|7", z), "garbled / short new fields are refused");
        acc::Engine E; E.onBook(0, 0, true, 10, 5); E.onBook(100, 5, true, 10, 5); E.onBook(100, 0, true, 0, 5); E.onBook(100, 0, true, 10, 0); CHECK(E.K.empty(), "bad book events are dropped");
        for (int i = 0; i < 1000; ++i) { E.onBook(1000 + i * 30, 0, true, 10 + i, 1); }
        CHECK((int)E.K.size() == acc::BK_KEEP, "the book cells are capped");
        E.onBook(1000, 0, true, 10, 1); CHECK((int)E.K.size() == acc::BK_KEEP, "an old event is not counted"); CHECK(E.last() == nullptr, "no zone: last() is null"); }
    {   long long t2 = 0; acc::Engine E = base(1, t2); const std::string f = acc::fileText(E, "ES", 0.25); const std::vector<acc::Zone> one = acc::parseFile(f);
        std::vector<acc::Zone> two = one; two.insert(two.end(), one.begin(), one.end());                       // the doubled file of 2026-11-16
        acc::Engine L; L.load(two, 0); CHECK(L.Z.size() == one.size() && one.size() == 1, "a doubled file loads each zone once (" << L.Z.size() << ")");
        acc::Engine R2 = base(1, t2); const size_t n0 = R2.Z.size(); R2.Z[0].state = acc::BROKEN; R2.lastT = 0;   // decided, then the same candles come again
        acc::Engine R3 = base(1, t2); for (const acc::Bar& b : R3.B) R2.onCandle(b);
        CHECK(R2.Z.size() == n0, "replaying decided candles never adds a second copy of a zone (" << R2.Z.size() << ")"); }
    std::cout << "test_accum_120: " << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
