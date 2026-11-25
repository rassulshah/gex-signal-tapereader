// BsRead 2.0.0: the buy / sell read concludes - the real GC 11-20 candles of reads-conclude-mockup.html v30
#include "../BsRead.h"
#include <cstdio>
#include <map>
static int checks = 0, fails = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static void show(const char* t, const bsr::Out& o) { std::printf("%s  %s", t, o.head.c_str()); for (size_t k = 0; k < o.ex.size(); ++k) std::printf(" | %s", o.ex[k].c_str()); std::printf("   (buy %.2f sell %.2f swing %.2f delta %.2f atr %lld ext %d)\n", o.buyX, o.sellX, o.swingX, o.deltaX, o.atr, o.newExt); }
int main() {
    using namespace bsr; std::map<std::string, In> G;
#include "gc_bs.inc"
    Out a = read(G["09:30"]); show("9:30", a);
    CHECK(a.verdict == 1 && a.head == "Buyers took over at the low");
    CHECK(a.ex.size() >= 2 && a.ex[0] == "Delta and price agreed." && a.ex[1] == "Strong buyers flipped delta 3.2x bullish.");
    Out b = read(G["10:06"]); show("10:06", b);
    CHECK(b.verdict == 1 && b.head == "Buyers took over - the delta flipped 3.7x bullish" && b.ex.size() >= 1 && b.ex[0] == "Delta and price agreed.");
    Out c = read(G["09:21"]); show("9:21", c);
    CHECK(c.verdict == 0 && c.head == "Sellers hit harder but could not keep the low");
    Out d = read(G["09:12"]); show("9:12", d);
    CHECK(d.verdict == 0 && d.head == "Two-sided - no one in control" && d.ex.size() >= 1 && d.ex[0] == "Buyers hit hard but price fell - buyers trapped.");
    for (const Out* o : { &a, &b, &c, &d }) { CHECK(o->head.find('?') == std::string::npos && o->head.find(" vs ") == std::string::npos); for (const std::string& s : o->ex) CHECK(s.find('?') == std::string::npos && s.find("normal") == std::string::npos); CHECK(o->exQ.size() == o->ex.size()); }
    // drying up: sellers 900, 700, 500 then 300 at a new low
    In x = G["09:30"]; x.prev[2].sell = 900; x.prev[1].sell = 700; x.prev[0].sell = 500; x.sell = 300; Out e = read(x); show("dry", e);
    bool dry = false; for (const std::string& s : e.ex) if (s == "Selling dried up over 3 candles.") dry = true; CHECK(dry);
    // mirror: sellers trapped
    In m = G["09:12"]; std::swap(m.buy, m.sell); m.open = G["09:12"].close; m.close = G["09:12"].open; Out f = read(m); show("mirror", f);
    bool tr = false; for (const std::string& s : f.ex) if (s == "Sellers hit hard but price rose - sellers trapped.") tr = true; CHECK(tr);
    In bad; CHECK(!read(bad).ok); In z; z.ok = true; CHECK(read(z).head == "No trades");
    In few; few.ok = true; few.buy = 100; few.sell = 50; Out g = read(few); CHECK(g.ok && g.ex.empty() && g.head == "Two-sided - no one in control");   // nothing known: no claims
    { std::map<long long, std::pair<int, int> > m; CHECK(parseBsv("BSV|1|GC|180|0.31.0\n100|1|0\n200|0|1\n300|5|0\n400|-1|", "GC", 180, m) && m.size() == 2 && m[100].first == 1 && m[200].second == 1);
      CHECK(!parseBsv("BSV|1|ES|180|x\n", "GC", 180, m)); }
    CHECK(std::string(VERSION) == "2.0.0");
    std::printf("t_bs2: %d checks, %d fails\n", checks, fails); return fails ? 1 : 0;
}
