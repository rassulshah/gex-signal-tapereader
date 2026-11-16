// test_accum_200.cpp - AccumLogic.h 2.0.0 (the simple rule).  g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_accum_200.cpp -o t200 && ./t200 es_2026-11-16.csv
// Fixture: ES 3-min candles of 2026-11-16 (t = candle end, local seconds; lo, hi, close in ticks; buy, sell lots) - the day Rassul circled the 12:54 PM and 2:00 PM highs.
#include "AccumLogic.h"
#include <fstream>
#include <iostream>
#include <sstream>
static int passed = 0, failed = 0;
#define CHECK(c, m) do { if (c) ++passed; else { ++failed; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
static std::vector<acc::Bar> load(const char* p) { std::vector<acc::Bar> v; std::ifstream f(p); std::string l;
    while (std::getline(f, l)) { std::stringstream s(l); std::string c; long long x[6] = { 0 }; int k = 0; while (std::getline(s, c, ',') && k < 6) x[k++] = atoll(c.c_str());
        if (k == 6) { acc::Bar b; b.t = x[0]; b.lo = x[1]; b.hi = x[2]; b.close = x[3]; b.buy = x[4]; b.sell = x[5]; v.push_back(b); } }
    return v; }
static int hhmm(long long t) { return (int)((t % 86400) / 3600 * 100 + (t % 3600) / 60); }
static const acc::Zone* at(const acc::Engine& E, int side, int t0) { for (const acc::Zone& z : E.Z) if (z.side == side && hhmm(z.t0) == t0) return &z; return nullptr; }
int main(int argc, char** argv) {
    if (argc < 2) { std::cout << "fixture path needed\n"; return 1; }
    const std::vector<acc::Bar> B = load(argv[1]); CHECK(B.size() > 250, "the fixture loads (" << B.size() << " candles)");
    acc::Engine E; for (const acc::Bar& b : B) E.onCandle(b);
    const acc::Zone* a = at(E, -1, 1254); const acc::Zone* c = at(E, -1, 1400); const acc::Zone* h = at(E, -1, 821);
    CHECK(a && a->state == acc::CONFIRMED && hhmm(a->tDec) == 1303 && a->absPx == 29844 && a->absX >= 2.0 && a->absX < 2.3, "his first circle: distribution at 7461.00, climax 2.1x, confirmed 1:03 PM");
    CHECK(c && c->state == acc::CONFIRMED && hhmm(c->tDec) == 1412 && c->absPx == 29795 && c->absX >= acc::CLIMAX_X, "his second circle: distribution at 7448.75, confirmed 2:12 PM");
    CHECK(h && h->state == acc::CONFIRMED && hhmm(h->tDec) == 842, "the morning high: distribution confirmed 8:42 AM");
    int drawn = 0, rth = 0; for (const acc::Zone& z : E.Z) { if (acc::drawn(z)) ++drawn; if (hhmm(z.t0) >= 830 && hhmm(z.t0) < 1500) ++rth; CHECK(z.absX >= acc::CLIMAX_X && z.lo <= z.absPx && z.hi >= z.absPx && z.t0 <= z.t1 && (z.state == acc::OPEN || z.tDec >= z.t1), "zone " << z.id << " is consistent"); }
    CHECK(drawn == 3 && rth == 2 && E.Z.size() <= 8, "few zones: " << E.Z.size() << " in the session, " << drawn << " drawn");
    if (a) { const std::vector<acc::Line> L = acc::readLines(*a, 0.25, 180, "1:03 PM");
        CHECK(L.size() == 5 && L[0].text == "Distrib 7461.00" && L[0].col == -1 && L[1].text == "1. Climax 2.1x (1.4x)" && L[2].text.compare(0, 9, "2. Stall ") == 0 && L[3].text == "3. Delta -1,466" && L[3].col == -1 && L[4].text == "Confirmed 1:03 PM", "the read: " << L[0].text << " / " << L[1].text << " / " << L[3].text);
        const std::vector<acc::Line> C = acc::readCompact(*a, 0.25, 180, "1:03 PM"); CHECK(C.size() == 3 && C[1].text.find("Climax 2.1x") == 0 && C[1].text.find("Delta -1,466") != std::string::npos, "the 3-line read: " << C[1].text); }
    {   // never repaints: a restart at every 25th candle (save, reload, prime the history, go on) = the straight pass
        bool same = true; for (size_t cut = 70; cut < B.size() && same; cut += 25) { acc::Engine P; for (size_t i = 0; i < cut; ++i) P.onCandle(B[i]);
            const std::string f = acc::fileText(P, "ES", 0.25); acc::Engine Q; Q.load(acc::parseFile(f), acc::parseLast(f));
            for (const acc::Bar& b : B) { if (b.t <= Q.resume) Q.prime(b); else Q.onCandle(b); }
            same = Q.Z.size() == E.Z.size(); for (size_t i = 0; same && i < E.Z.size(); ++i) same = acc::zLine(Q.Z[i]) == acc::zLine(E.Z[i]); }
        CHECK(same, "a restart anywhere gives the same zones as the straight pass");
        acc::Engine T; std::vector<std::string> dec; bool frozen = true;
        for (const acc::Bar& b : B) { T.onCandle(b); for (size_t i = 0; i < T.Z.size(); ++i) { if (i >= dec.size()) dec.push_back(""); const acc::Zone& z = T.Z[i]; if (z.state == acc::OPEN) continue; const std::string l = acc::zLine(z);
            if (z.state == acc::CONFIRMED && dec[i].empty()) { dec[i] = "c"; continue; } if (dec[i].empty() || dec[i] == "c") dec[i] = l; else frozen = frozen && dec[i] == l; } }
        CHECK(frozen, "a decided zone never changes afterwards"); }
    {   // the mirror: the same day upside down gives ACCUMULATION at the mirrored lows
        acc::Engine M; for (const acc::Bar& b : B) { acc::Bar q = b; q.lo = 60000 - b.hi; q.hi = 60000 - b.lo; q.close = 60000 - b.close; q.buy = b.sell; q.sell = b.buy; M.onCandle(q); }
        const acc::Zone* m = at(M, 1, 1254); CHECK(m && m->state == acc::CONFIRMED && m->absPx == 60000 - 29844 && hhmm(m->tDec) == 1303, "the mirror: accumulation at the mirrored low");
        if (m) { const std::vector<acc::Line> L = acc::readLines(*m, 0.25, 180, "1:03 PM"); CHECK(L[0].text.compare(0, 6, "Accum ") == 0 && L[0].col == 1 && L[3].text == "3. Delta +1,466" && L[3].col == 1, "its read: " << L[3].text); }
        CHECK(M.Z.size() == E.Z.size(), "the mirror finds as many zones (" << M.Z.size() << " / " << E.Z.size() << ")"); }
    {   // no climax, no zone; against the trend, no zone
        acc::Engine N; for (const acc::Bar& b : B) { acc::Bar q = b; q.buy = 500 + (b.buy > b.sell ? 30 : 0); q.sell = 500 + (b.sell > b.buy ? 30 : 0); N.onCandle(q); }
        CHECK(N.Z.empty(), "flat volume (no climax at any pivot): no zone (" << N.Z.size() << ")");
        acc::Engine S; for (size_t i = 0; i < B.size(); ++i) { acc::Bar q = B[i]; const long long up = (long long)i * 12; q.lo += up; q.hi += up; q.close += up; S.onCandle(q); }   // the same candles on a rising day
        CHECK(at(S, -1, 1254) == nullptr && at(S, -1, 1400) == nullptr, "the same stalls AGAINST the trend open nothing"); }
    {   acc::Engine X; for (int i = 0; i < 40; ++i) { acc::Bar b; b.t = 1000 + i * 180; b.lo = 100; b.hi = 110; b.close = 105; b.buy = b.sell = 10; X.onCandle(b); } CHECK(X.Z.empty(), "a short history opens nothing");
        acc::Bar bad; bad.t = 99999999; bad.lo = 0; bad.hi = 5; X.onCandle(bad); bad.lo = 9; bad.hi = 5; X.onCandle(bad); CHECK(X.B.size() == 40, "bad candles are dropped");
        CHECK(acc::commas(-1466) == "-1,466" && acc::commas(66) == "+66" && acc::commas(1234567) == "+1,234,567" && acc::commas(0) == "+0", "the delta text"); }
    std::cout << "test_accum_200: " << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
