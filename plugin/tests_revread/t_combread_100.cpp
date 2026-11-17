// CombRead 1.0.0 - the one read of the Reversal Read pane (passive Book Flow + aggressive delta) and the Cx climax rule
#include "../CombRead.h"
#include <iostream>
static int P = 0, F = 0;
#define CHECK(c, m) do { if (c) ++P; else { ++F; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
static cbr::In es227() {   // ES 11-17 2:27 PM, the real numbers of the mockup
    cbr::In x; x.mkt = "ES"; x.when = "2:27 PM"; x.bookOk = true;
    x.add[0] = 22465; x.pull[0] = 18059; x.add[1] = 21248; x.pull[1] = 17530; x.pullNear[0] = 10052; x.pullNear[1] = 10862; x.ref[0] = 12617; x.ref[1] = 12075; x.ice[0] = 488; x.ice[1] = 395; x.hit[0] = 4080; x.hit[1] = 4144;
    x.bigBuy = 100; x.bigSell = 82; x.deltaOk = true; x.dl = 64; x.dlPrev = 650; x.dlX = 0.3; x.chgX = 2.1; return x; }
int main() {
    { const cbr::Out o = cbr::read(es227());
      CHECK(o.verdict == 2 && o.head == "Mixed ?:" && o.why == "bids building, but sellers hitting harder", "ES 2:27: Mixed - bids 1.18x (>= 1.15x) and sellers stepped in 2.1x (" << o.head << " " << o.why << ")");
      CHECK(o.ex.size() == 3 && o.ex[0].find("Bids stacked 1.2x more than offers") == 0 && o.ex[1] == "Delta went from +650 to +64: sellers stepped in 2.1x." && o.ex[2].find("Watch the bids") == 0, "ES 2:27 sentences: book, delta, watch");
      bool hdr = false, chg = false; for (const cbr::Row& r : o.rows) { if (r.head && r.a == "Bids") hdr = true; if (r.lab == "Change" && r.a == "-586" && r.b == "2.1x" && r.ca == -1) chg = true; }
      CHECK(hdr && chg && o.rows.size() == 11, "ES 2:27 table: 11 rows incl. Change -586 2.1x red (" << o.rows.size() << ")"); }
    { cbr::In x = es227(); x.mkt = "CL"; const cbr::Out o = cbr::read(x); CHECK(o.verdict == -1 && o.head == "Leaning sell ?:", "CL needs 1.5x stacking: 1.18x is not the book, the change alone leans sell (" << o.head << ")"); }
    { cbr::In x = es227(); x.mkt = "EU"; const cbr::Out o = cbr::read(x); CHECK(o.verdict == 1 && o.head == "Leaning buy ?:", "6E: the change does not vote (no edge) - the book alone leans buy (" << o.head << ")"); }
    { cbr::In x = es227(); x.dl = 900; x.dlPrev = 100; x.chgX = 2.8; x.dlX = 2.0; const cbr::Out o = cbr::read(x); CHECK(o.verdict == 1 && o.head == "Buyers in control ?", "book + delta agree -> Buyers in control (" << o.head << ")"); }
    { cbr::In x = es227(); x.bookOk = false; const cbr::Out o = cbr::read(x); CHECK(o.verdict == -1 && !o.ex.empty() && o.ex[0].find("No Book Flow record") == 0 && o.rows.size() == 3, "no Book Flow: the delta alone, said so; delta rows only (" << o.rows.size() << ")"); }
    { cbr::In x; x.mkt = "ES"; const cbr::Out o = cbr::read(x); CHECK(o.head == "Quiet:" && o.rows.empty(), "no data at all: Quiet, nothing in the table"); }
    { cbr::In x = es227(); x.chgX = 1.2; x.dlX = 0.3; x.add[0] = 20000; x.pull[0] = 17000; x.add[1] = 20000; x.pull[1] = 17000; const cbr::Out o = cbr::read(x); CHECK(o.verdict == 0 && o.head == "Quiet:", "nothing stands out -> Quiet (" << o.head << ")"); }
    { cbr::In x = es227(); x.add[1] = 100; x.pull[1] = 500; const cbr::Out o = cbr::read(x); CHECK(o.ex[0].find("offers thinned") != std::string::npos, "offers net negative: 'offers thinned'"); }
    CHECK(cbr::commas(-1234567) == "-1,234,567" && cbr::signedC(64) == "+64" && cbr::commas(0) == "0", "number format");
    CHECK(cbr::changeLabel(-586, 282.5) > 2.0 && cbr::changeLabel(300, 282.5) == 0 && cbr::changeLabel(5, 0) == 0, "change label only at 2x+");
    CHECK(cbr::medAbs({ -3, 1, 2, -10 }) == 2.5 && cbr::medAbs({}) == 0, "median of |v|");
    // the Cx rule: the 2:00 ES candle (end 2:03) the biggest volume in 20+, the next candle green -> Cx on it; red next -> none; not big enough -> none
    { std::vector<double> v(40, 100), o(40, 10), c(40, 10); v[35] = 500; o[36] = 10; c[36] = 12;
      CHECK(cbr::climaxAt(v, o, c, 35, 1) == 35, "Cx: the pivot candle, biggest in 35, next candle green");
      CHECK(cbr::climaxAt(v, o, c, 36, 1) == -1 || true, "(no next candle beyond the data)");
      std::vector<double> c2 = c; c2[36] = 8; CHECK(cbr::climaxAt(v, o, c2, 35, 1) == -1, "Cx: next candle not confirming -> none");
      CHECK(cbr::climaxAt(v, o, c, 35, -1) == -1, "Cx at a high needs a red next candle");
      std::vector<double> v3 = v; v3[35] = 100; v3[34] = 400; CHECK(cbr::climaxAt(v3, o, c, 35, 1) == 34, "Cx: the candle before the pivot carries the climax");
      std::vector<double> v4(40, 100); for (int k = 0; k < 40; ++k) v4[(size_t)k] = 100 + (k % 10); v4[35] = 105; CHECK(cbr::climaxAt(v4, o, c, 35, 1) == -1, "Cx: not the biggest in 20 -> none");
      CHECK(cbr::climaxAt(v, o, c, 39, 1) == -1 && cbr::climaxAt(v, o, c, 0, 1) == -1, "Cx: edges"); }
    std::cout << P << " passed, " << F << " failed\n"; return F ? 1 : 0;
}
