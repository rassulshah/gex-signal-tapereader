// test_accum.cpp - AccumLogic.h 1.2.0.  g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_accum.cpp -o ta && ./ta [RevRead record] [bars json dump]
#include "AccumLogic.h"
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
static int passed = 0, failed = 0;
#define CHECK(c, m) do { if (c) ++passed; else { ++failed; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
static acc::Bar bar(long long t, long long lo, long long hi, long long c, long long buy, long long sell) { acc::Bar b; b.t = t; b.lo = lo; b.hi = hi; b.close = c; b.buy = buy; b.sell = sell; return b; }
int main(int argc, char** argv) {
    const long long T0 = 1000000080;
    {   // a low that holds under net selling + an absorbed read = an open accumulation zone; a strong up close confirms it
        acc::Engine E; long long t = T0;
        for (int i = 0; i < 20; ++i) { E.onCandle(bar(t, 1100 - i * 4, 1112 - i * 4, 1104 - i * 4, 100, 100)); t += 180; }     // a steady fall
        CHECK(E.Z.empty(), "a plain fall is not a zone");
        for (int i = 0; i < 6; ++i) { if (i == 3) E.onAbsorb(t - 60, true, 1020, 40, 60, 1.5); E.onCandle(bar(t, 1020 + (i % 2), 1030, 1024, 100, 130)); t += 180; }
        CHECK(E.Z.size() == 1 && E.Z[0].side == 1 && E.Z[0].state == acc::OPEN && E.Z[0].absPx == 1020, "net selling + holding low + absorbed = an open accumulation zone (" << E.Z.size() << ")");
        const acc::Zone z0 = E.Z.empty() ? acc::Zone() : E.Z[0];
        CHECK(z0.net > 0 && z0.drift >= 0 && z0.lo == 1020 && z0.hi >= 1030, "the zone's numbers: net " << z0.net << " drift " << z0.drift);
        const std::vector<acc::Line> L = acc::readLines(z0, 0.01, 180, "");
        CHECK(L.size() == 6 && L[0].text == "Accum? 10.20" && L[0].col == 3 && L[3].text == "3. Absorbed 1.5x (60 / 40 shown)" && L[4].text == "4. Book: n/a" && L[5].text.find("Above ") != std::string::npos, "the read while open: " << (L.empty() ? "" : L[0].text));
        E.onCandle(bar(t, 1022, 1029, 1027, 90, 120)); t += 180;                    // inside, no decision
        CHECK(E.Z[0].state == acc::OPEN, "still open");
        E.onCandle(bar(t, 1026, 1040, 1038, 200, 90)); t += 180;                     // closes over the last 3 highs with positive delta
        CHECK(E.Z[0].state == acc::CONFIRMED && E.Z[0].tDec == t - 180, "a buying close above the last 3 highs confirms");
        const acc::Zone dec = E.Z[0];
        for (int i = 0; i < 10; ++i) { E.onCandle(bar(t, 1000, 1010, 1004, 10, 500)); t += 180; }
        CHECK(E.Z[0].state == acc::CONFIRMED && E.Z[0].lo == dec.lo && E.Z[0].hi == dec.hi && E.Z[0].tDec == dec.tDec, "a decided zone never changes");
        const std::vector<acc::Line> L2 = acc::readLines(dec, 0.01, 180, "10:51 AM");
        CHECK(L2[0].text == "Accum 10.20" && L2[0].col == 1 && L2[5].text == "Confirmed 10:51 AM", "the read once confirmed");
    }
    {   // broken: a close through the absorbed price; expired: 30 candles without a decision; no absorbed read = no zone; up-close on negative delta = no confirm
        acc::Engine E; long long t = T0;
        for (int i = 0; i < 20; ++i) { E.onCandle(bar(t, 1100 - i * 4, 1112 - i * 4, 1104 - i * 4, 100, 100)); t += 180; }
        for (int i = 0; i < 6; ++i) { E.onCandle(bar(t, 1020, 1030, 1024, 100, 130)); t += 180; }
        CHECK(E.Z.empty(), "no absorbed read = no zone");
        E.onAbsorb(t - 30, true, 1020, 40, 44, 1.1); E.onCandle(bar(t, 1020, 1030, 1024, 100, 130)); t += 180; CHECK(E.Z.empty(), "an absorbed ratio under 1.2x does not count");
        E.onAbsorb(t - 30, true, 1021, 40, 80, 2.0); E.onCandle(bar(t, 1020, 1030, 1024, 100, 130)); t += 180; CHECK(E.Z.size() == 1 && E.Z[0].state == acc::OPEN, "opens with a real absorbed read");
        E.onCandle(bar(t, 1023, 1036, 1035, 80, 120)); t += 180; CHECK(E.Z[0].state == acc::OPEN, "an up close on NEGATIVE delta does not confirm");
        E.onCandle(bar(t, 1012, 1030, 1015, 60, 300)); t += 180; CHECK(E.Z[0].state == acc::BROKEN && !acc::drawn(E.Z[0]), "a close under the absorbed price breaks it; a broken zone is not drawn");
        acc::Engine X; t = T0;
        for (int i = 0; i < 20; ++i) { X.onCandle(bar(t, 1100 - i * 4, 1112 - i * 4, 1104 - i * 4, 100, 100)); t += 180; }
        for (int i = 0; i < 6; ++i) { if (i == 2) X.onAbsorb(t - 60, true, 1020, 40, 60, 1.5); X.onCandle(bar(t, 1020, 1030, 1024, 100, 130)); t += 180; }
        for (int i = 0; i < 40 && X.Z[0].state == acc::OPEN; ++i) { X.onCandle(bar(t, 1021, 1029, 1024, 100, 100)); t += 180; }
        CHECK(X.Z[0].state == acc::EXPIRED && X.Z[0].bars == acc::MAX_OPEN, "30 candles without a decision = expired (" << X.Z[0].bars << ")");
    }
    {   acc::Abs a; CHECK(acc::parseAbsLine("S|0.8.0|3|bottom|0|A|3877669970|burst 70|opp 19|big 14|p95 36|anchor 86.49999807|shown 39|abs 60|absx 1.538461538|range 8", 0.01, a) && a.side == 1 && a.px == 8650 && a.shown == 39 && a.lots == 60 && std::fabs(a.x - 1.5385) < 1e-3 && a.t == 3877669970, "an Absorbed line of the record");
        CHECK(!acc::parseAbsLine("S|0.8.0|3|bottom|0|C|3877669930|anchor 86.5|shown 39|abs 45|absx 0|x|y", 0.01, a) && !acc::parseAbsLine("V|0.8.0|1|2|3", 0.01, a), "other lines are not Absorbed reads"); }
    {   // the mirror: distribution at a high
        acc::Engine E; long long t = T0;
        for (int i = 0; i < 20; ++i) { E.onCandle(bar(t, 1000 + i * 4, 1012 + i * 4, 1008 + i * 4, 100, 100)); t += 180; }
        for (int i = 0; i < 6; ++i) { if (i == 3) E.onAbsorb(t - 60, false, 1092, 40, 70, 1.75); E.onCandle(bar(t, 1082, 1092, 1088, 130, 100)); t += 180; }
        CHECK(E.Z.size() == 1 && E.Z[0].side == -1 && E.Z[0].state == acc::OPEN, "distribution opens at a high");
        const std::vector<acc::Line> L = acc::readLines(E.Z[0], 0.25, 180, "");
        CHECK(L[0].text == "Distrib? 273.00" && L[1].text.find("Buyers net") != std::string::npos && L[5].text.find("Below ") != std::string::npos, "the mirrored read: " << L[0].text);
        E.onCandle(bar(t, 1070, 1086, 1072, 80, 260)); t += 180;
        CHECK(E.Z[0].state == acc::CONFIRMED, "a selling close under the last 3 lows confirms distribution");
        // record round trip, torn / bad lines
        const std::string txt = acc::fileText(E, "ES", 0.25); std::vector<acc::Zone> R = acc::parseFile(txt + "Z|1.0.0|9|1|0|5|4|0|1|2|3\nZ|1.0.0|3|1|0|100|200|0|10|20|15|21|5|50|1|4|6|1.5|2|12");
        CHECK(R.size() == 1 && R[0].id == E.Z[0].id && R[0].absPx == E.Z[0].absPx && R[0].state == acc::CONFIRMED && std::fabs(R[0].absX - 1.75) < 1e-9, "record round trip; a bad line and a torn last line are dropped (" << R.size() << ")");
        // boxes: decided = its side's colour, clipped to the visible times, outline box with the absorbed price inside
        auto X = [&](long long tt) { return (int)((tt - T0) / 180) * 10; }; auto Y = [](long long tk) { return 5000 - (int)tk; };
        const std::vector<acc::Box> Bx = acc::boxes(E.Z, T0, t, 180, X, Y, 4);
        CHECK(Bx.size() == 1 && Bx[0].col == acc::C_DN && !Bx[0].open && Bx[0].x1 > Bx[0].x0 && Bx[0].y0 <= Bx[0].yAbs && Bx[0].yAbs <= Bx[0].y1, "one red box, the absorbed price inside it");
        CHECK(acc::boxes(E.Z, t + 1800, t + 3600, 180, X, Y, 4).empty(), "a zone off screen draws nothing");
        CHECK(acc::px(8650, 0.01) == "86.50" && acc::px(30100, 0.25) == "7525.00" && acc::px(41234, 0.1) == "4123.4" && acc::px(11590, 0.0001) == "1.1590", "prices keep the market's decimals");
    }
    if (argc > 2) {   // today's real CL session: V lines (buy / sell per candle) + S lines (the Absorbed reads) from lsRevRead's record, the candles' high / low / close from the chart export
        std::map<long long, acc::Bar> bars; { std::ifstream f(argv[2]); std::string l; while (std::getline(f, l)) { acc::Bar b; if (std::sscanf(l.c_str(), "%lld,%lld,%lld,%lld", &b.t, &b.lo, &b.hi, &b.close) == 4) bars[b.t] = b; } }
        struct Ev { long long t; int kind; acc::Bar b; acc::Abs a; }; std::vector<Ev> ev; std::ifstream f(argv[1]); std::string l; int nV = 0, nA = 0;
        while (std::getline(f, l)) { std::vector<std::string> c; std::stringstream ss(l); std::string x; while (std::getline(ss, x, '|')) c.push_back(x);
            if (c.size() > 4 && c[0] == "V") { const long long t = std::atoll(c[2].c_str()); auto it = bars.find(t); if (it == bars.end()) continue; Ev e; e.t = t; e.kind = 0; e.b = it->second; e.b.buy = std::atoll(c[3].c_str()); e.b.sell = std::atoll(c[4].c_str()); ev.push_back(e); ++nV; }
            { acc::Abs q; if (acc::parseAbsLine(l, 0.01, q)) { Ev e; e.kind = 1; e.t = q.t; e.a = q; ev.push_back(e); ++nA; } } }
        std::stable_sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) { return a.t < b.t; });
        acc::Engine E, S2; std::vector<int> firstState;
        for (const Ev& e : ev) { if (e.kind) E.onAbsorb(e.t, e.a.side > 0, e.a.px, e.a.shown, e.a.lots, e.a.x); else E.onCandle(e.b); }
        CHECK(nV > 100, "the CL session loaded (" << nV << " candles, " << nA << " absorbed reads)");
        std::cout << "CL today: zones " << E.Z.size() << "\n";
        for (const acc::Zone& z : E.Z) { const long long s0 = (z.t0 - 2082844800LL) % 86400, s1 = ((z.state == acc::OPEN ? z.t1 : z.tDec) - 2082844800LL) % 86400;
            std::cout << "  " << (z.side > 0 ? "ACCUM " : "DISTR ") << (z.state == acc::OPEN ? "open" : z.state == acc::CONFIRMED ? "CONFIRMED" : z.state == acc::BROKEN ? "broken" : "expired") << " " << s0 / 3600 << ":" << (s0 % 3600) / 60 << " -> " << s1 / 3600 << ":" << (s1 % 3600) / 60
                      << " abs " << acc::px(z.absPx, 0.01) << " x" << z.absX << " net " << z.net << " drift " << z.drift << " box " << acc::px(z.lo, 0.01) << "-" << acc::px(z.hi, 0.01) << "\n"; }
        // bar by bar with a save / reload after every candle = one straight pass (no repaint through restarts)
        bool same = true; { acc::Engine R; for (const Ev& e : ev) { if (e.kind) R.onAbsorb(e.t, e.a.side > 0, e.a.px, e.a.shown, e.a.lots, e.a.x); else R.onCandle(e.b); } same = acc::fileText(R, "CL", 0.01) == acc::fileText(E, "CL", 0.01); }
        CHECK(same, "two passes give the same zones");
        {   // a restart in the middle: save, reload, prime the history, go on = the same zones as the straight pass
            acc::Engine P1; size_t cut = ev.size() * 3 / 4; for (size_t i = 0; i < cut; ++i) { const Ev& e = ev[i]; if (e.kind) P1.onAbsorb(e.t, e.a.side > 0, e.a.px, e.a.shown, e.a.lots, e.a.x); else P1.onCandle(e.b); }
            const std::string saved = acc::fileText(P1, "CL", 0.01); acc::Engine P2; P2.load(acc::parseFile(saved), acc::parseLast(saved));
            CHECK(acc::parseLast(saved) == P1.lastT && P2.Z.size() == P1.Z.size(), "the file carries the zones and where they were decided up to");
            for (const Ev& e : ev) { if (e.kind) { if (e.t > P2.resume) P2.onAbsorb(e.t, e.a.side > 0, e.a.px, e.a.shown, e.a.lots, e.a.x); } else if (e.b.t <= P2.resume) P2.prime(e.b); else P2.onCandle(e.b); }
            bool eq = P2.Z.size() == E.Z.size(); for (size_t i = 0; eq && i < E.Z.size(); ++i) eq = acc::zLine(P2.Z[i]) == acc::zLine(E.Z[i]);
            CHECK(eq, "restart half way = the straight pass (" << P2.Z.size() << " / " << E.Z.size() << " zones)"); }
        int open = 0; for (const acc::Zone& z : E.Z) if (z.state == acc::OPEN) ++open; CHECK(open <= 2, "at most one open zone a side");
    }
    std::cout << "test_accum: " << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
