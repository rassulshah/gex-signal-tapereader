// test_trapped_logic.cpp -- lsTrapped 1.0.0 logic tests (SDK-free; ASan/UBSan, -Werror). Run: tests_trapped/run_tests.sh
// (1) the approved mockup days on REAL data: ES 2026-10-05 and NQ 2026-10-09 3-min fixtures (tests_ob126, bar END stamps, cells px:ask:bid)
// (2) unit cases: qualify on the next candle, freed, fuel burn cap, record round trip + old / garbled / other-kind lines ignored,
//     no repaint on recompute (a restart after every candle == one straight pass), normal fallback / history.
#include "../TrappedLogic.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>

static int g_fail = 0, g_ok = 0;
#define CHECK(c, m) do { if (c) ++g_ok; else { ++g_fail; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, std::string(m).c_str()); } } while (0)

static long long civil(int y, int mo, int d, int h, int mi) {      // CT civil seconds (treated as UTC by the test clock)
    y -= mo <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + (long long)doe - 719468) * 86400 + h * 3600 + mi * 60;
}
static std::string hm(long long t) { const long long s = ((t % 86400) + 86400) % 86400; int h = (int)(s / 3600), m = (int)(s % 3600 / 60); char b[16]; std::snprintf(b, sizeof b, "%d:%02d", h % 12 == 0 ? 12 : h % 12, m); return b; }
static long long sessOf(long long t) { return (t - 1 - 17 * 3600) / 86400 + 1; }   // the 17:00 CT day (bar END - 1 s)
static bool rthEsNq(long long t) { const long long s = ((t % 86400) + 86400) % 86400; return s - 180 >= 8 * 3600 + 30 * 60 && s <= 15 * 3600; }

static std::vector<trp::Bar> loadFixture(const std::string& path, double tick) {
    std::vector<trp::Bar> v; std::ifstream f(path.c_str()); std::string ln; std::getline(f, ln);
    while (std::getline(f, ln)) {
        std::vector<std::string> p = trp::split(ln, '|'); if (p.size() < 7) continue;
        int y, mo, d, h, mi; if (std::sscanf(p[0].c_str(), "%d-%d-%d %d:%d", &y, &mo, &d, &h, &mi) != 5) continue;
        trp::Bar b; b.t = civil(y, mo, d, h, mi); b.sess = sessOf(b.t); b.rth = rthEsNq(b.t);
        trp::toTicks(std::atof(p[1].c_str()), tick, b.o); trp::toTicks(std::atof(p[2].c_str()), tick, b.h); trp::toTicks(std::atof(p[3].c_str()), tick, b.l); trp::toTicks(std::atof(p[4].c_str()), tick, b.c);
        if (b.rth) {                                                   // the host reads volume at price for RTH candles only
            for (const std::string& c : trp::split(p[6], ';')) { std::vector<std::string> q = trp::split(c, ':'); if (q.size() != 3) continue; trp::Cell e; trp::toTicks(std::atof(q[0].c_str()), tick, e.px); e.ask = std::atoll(q[1].c_str()); e.bid = std::atoll(q[2].c_str()); b.cells.push_back(e); }
            b.cellsOk = !b.cells.empty();
        }
        v.push_back(b);
    }
    return v;
}
static const trp::Trap* find(const std::vector<trp::Trap>& v, const std::string& t, int side) { for (const trp::Trap& T : v) if (hm(T.sweepT) == t && T.side == side) return &T; return nullptr; }
static void dump(const char* name, const trp::Engine& E, double tick) {
    std::printf("== %s: %zu traps (candidates %lld, unqualified %lld)\n", name, E.traps.size(), E.candidates, E.unqualified);
    for (const trp::Trap& T : E.traps)
        std::printf("   %s %-7s sweep %s Tr %s piv %s %s (%d back) lots %lld avg %s share %d%% ratio %.1f normal %lld%s atr %.2f | %s %s fuel %.0f\n",
                    hm(T.sweepT).c_str(), T.side == trp::SELLERS ? "sellers" : "buyers", hm(T.sweepT).c_str(), hm(T.trT).c_str(), hm(T.pivT).c_str(), trp::pxText((double)T.pivPx, tick).c_str(), T.back,
                    T.lots, trp::pxText(T.avgT, tick, true).c_str(), T.sharePct, T.ratio, T.normal, T.building ? " (building)" : "", T.atrT * tick, trp::statusName(T.status), T.endT ? hm(T.endT).c_str() : "", T.fuelNow);
}
static std::string D;   // the plugin folder

// the mockup's normal (445 lots = the ES 10-05 day median) as 5 earlier sessions, so the ratios read as in opt-A.png
static void seedHist(trp::Engine& E, long long day, long long lots) { for (int k = 1; k <= 5; ++k) E.H.setSession(day - k, std::vector<long long>(1, lots)); }

static void testRealDays() {
    const double tick = 0.25;
    {   // ES Mon 2026-10-05
        std::vector<trp::Bar> B = loadFixture(D + "/tests_ob126/es_2026-10-05_3m.txt", tick); CHECK(B.size() > 400, "ES fixture loaded");
        trp::Engine E; E.tickForLines = tick; seedHist(E, B.back().sess, 445);
        for (const trp::Bar& b : B) E.feed(b);
        dump("ES 2026-10-05 (normal 445 = the mockup's day median)", E, tick);
        const trp::Trap* a = find(E.traps, "9:03", trp::SELLERS);
        CHECK(a != nullptr, "ES 9:03 sellers trap");
        if (a) {
            CHECK(hm(a->pivT) == "8:48" && a->back == 2, "ES 9:03 swept the 8:48 low (2 swings back, deepest of the last 3) - got " + hm(a->pivT));
            CHECK(a->lots == 3140, "ES 9:03 lots 3,140 (the mockup's 3,625 counted below the 8:54 low, 1 back) - got " + std::to_string(a->lots));
            CHECK(std::fabs(a->avgT * tick - 7782.55) < 0.02, "ES 9:03 avg ~7782.55 - got " + trp::pxText(a->avgT, tick, true));
            CHECK(a->trT == a->sweepT, "ES 9:03 qualified on its own candle");
            CHECK(std::fabs(a->ratio - 7.1) < 1e-9, "ES 9:03 ratio 7.1x vs normal 445");
            CHECK(trp::message(*a, hm).find("9:03 Sellers trapped 3,140 lots (7.1x) below the 8:48 low") == 0, "ES message: " + trp::message(*a, hm));
        }
        const trp::Trap* b = find(E.traps, "12:27", trp::BUYERS);
        CHECK(b != nullptr, "ES 12:27 buyers trap");
        if (b) { CHECK(b->lots == 919, "ES 12:27 lots 919"); CHECK(hm(b->pivT) == "12:18", "ES 12:27 swept the 12:18 high"); CHECK(b->status == trp::FREED && hm(b->endT) == "12:45", "ES 12:27 freed 12:45 (as the mockup) - got " + std::string(trp::statusName(b->status)) + " " + hm(b->endT)); }
        // without history the first trap of the day is its own normal (building)
        trp::Engine F; F.tickForLines = tick; for (const trp::Bar& x : B) F.feed(x);
        const trp::Trap* c = find(F.traps, "9:03", trp::SELLERS);
        CHECK(c && c->building && c->normal == 3140 && std::fabs(c->ratio - 1.0) < 1e-9, "ES 9:03 with no history: normal building, 1.0x");
    }
    {   // NQ Fri 2026-10-09
        std::vector<trp::Bar> B = loadFixture(D + "/tests_ob126/nq_2026-10-09_3m.txt", tick); CHECK(B.size() > 400, "NQ fixture loaded");
        trp::Engine E; E.tickForLines = tick; seedHist(E, B.back().sess, 342);
        for (const trp::Bar& b : B) E.feed(b);
        dump("NQ 2026-10-09 (normal 342 as the fuel mockup)", E, tick);
        const trp::Trap* a = find(E.traps, "9:12", trp::BUYERS);
        CHECK(a != nullptr, "NQ 9:12 buyers trap");
        if (a) {
            CHECK(hm(a->pivT) == "9:03" && a->lots == 636, "NQ 9:12 swept the 9:03 high, 636 lots");
            CHECK(std::fabs(a->avgT * tick - 31057.57) < 0.02, "NQ 9:12 avg 31,057.57");
            CHECK(a->status == trp::FREED && hm(a->endT) == "9:30", "NQ 9:12 freed 9:30 - got " + std::string(trp::statusName(a->status)) + " " + hm(a->endT));
        }
        const trp::Trap* b = find(E.traps, "9:45", trp::SELLERS);
        CHECK(b != nullptr, "NQ 9:45 sellers trap");
        if (b) { CHECK(hm(b->pivT) == "9:24" && b->lots == 606, "NQ 9:45 swept the 9:24 low, 606 lots"); }
    }
}

// ---- synthetic candles -----------------------------------------------------------------------------------------------------------
struct Syn {
    std::vector<trp::Bar> v; long long t0 = civil(2026, 10, 20, 8, 30), sess = sessOf(civil(2026, 10, 20, 9, 0));
    void add(long long o, long long h, long long l, long long c, std::vector<trp::Cell> cells = {}, bool rth = true) {
        trp::Bar b; b.t = t0 + 180 * (long long)(v.size() + 1); b.sess = sess; b.rth = rth; b.o = o; b.h = h; b.l = l; b.c = c;
        if (cells.empty()) { trp::Cell e; e.px = c; e.ask = 50; e.bid = 50; cells.push_back(e); }
        b.cells = cells; b.cellsOk = true; v.push_back(b);
    }
};
static trp::Cell cell(long long px, long long ask, long long bid) { trp::Cell c; c.px = px; c.ask = ask; c.bid = bid; return c; }
// 16 flat warm-up candles (ATR = 8 ticks), then a pivot high at 120 (candle 17), a lower candle, then the sweep
static Syn base() {
    Syn s; for (int i = 0; i < 16; ++i) s.add(100, 104, 96, 100);
    s.add(100, 120, 98, 110);          // 17: pivot high 120
    s.add(110, 112, 104, 106);         // 18: confirms it
    return s;
}

static void testUnits() {
    const double tick = 0.25;
    {   // qualify on the NEXT candle: the sweep candle closes red back below 120 but only 0.1 ATR under the trapped average
        Syn s = base();
        s.add(124, 126, 105, 119, { cell(121, 300, 10), cell(124, 100, 10), cell(110, 50, 400) });   // avg = (121*300+124*100)/400 = 121.75; close 119 -> 2.75 ticks (ATR ~8.9 x .25 = 2.2)
        trp::Engine A; A.tickForLines = tick; for (auto& b : s.v) A.feed(b);
        CHECK(A.traps.size() == 1 && A.traps[0].trT == A.traps[0].sweepT, "qualified on the sweep candle when the close is far enough");
        Syn s2 = base();
        s2.add(124, 126, 105, 119, { cell(120, 300, 10), cell(123, 100, 10), cell(110, 50, 400) });  // avg 120.75, close 119: 1.75 ticks under (< 2.25) - not yet
        trp::Engine B; B.tickForLines = tick; for (auto& b : s2.v) B.feed(b);
        CHECK(B.traps.empty() && B.pend.size() == 1, "pending after the sweep candle");
        s2.add(121, 122, 112, 114, { cell(115, 20, 300) });                                          // next candle closes 7.75 ticks under
        B.feed(s2.v.back());
        CHECK(B.traps.size() == 1 && B.traps[0].trT == s2.v.back().t && B.traps[0].sweepT == s2.v[s2.v.size() - 2].t, "Tr decided on the next candle, the sweep candle keeps the letter");
        CHECK(B.traps.size() == 1 && B.traps[0].fuel.size() == 2, "the next candle already burns fuel");
        Syn s3 = base();
        s3.add(124, 126, 105, 119, { cell(120, 300, 10), cell(123, 100, 10), cell(110, 50, 400) });
        s3.add(119, 121, 118, 119, { cell(119, 50, 50) });                                           // again only 1.75 ticks under: dropped
        trp::Engine C; C.tickForLines = tick; for (auto& b : s3.v) C.feed(b);
        CHECK(C.traps.empty() && C.unqualified == 1, "not qualified in 2 candles: dropped");
        s3.add(120, 125, 100, 101, { cell(121, 80, 5), cell(101, 5, 300) });                         // a 3rd candle through the pivot again: the pivot traps once
        C.feed(s3.v.back());
        CHECK(C.traps.empty(), "a pivot traps once (no second candidate)");
    }
    {   // freed: a later close back above the buyers' average; the fuel line stops before that candle
        Syn s = base();
        s.add(124, 126, 105, 115, { cell(121, 300, 10), cell(124, 100, 10), cell(110, 50, 400) });
        s.add(115, 118, 110, 112, { cell(112, 50, 200) });
        s.add(112, 125, 111, 123, { cell(123, 400, 20) });                                           // close 123 > avg 121.75 -> freed
        trp::Engine E; E.tickForLines = tick; for (auto& b : s.v) E.feed(b);
        CHECK(E.traps.size() == 1 && E.traps[0].status == trp::FREED && E.traps[0].endT == s.v.back().t, "freed on the close back above the average");
        CHECK(E.traps.size() == 1 && E.traps[0].fuel.back().t == s.v[s.v.size() - 2].t, "the line ends on the candle before");
        bool trs = false; for (auto& l : E.newLines) if (l.find("TRS|") == 0 && l.find("|freed|") != std::string::npos) trs = true;
        CHECK(trs, "a TRS freed line is written");
    }
    {   // fuel burn and the offside cap: fuel = min(still offside, fuel - share x net exit flow)
        Syn s = base();
        s.add(124, 126, 105, 115, { cell(121, 300, 10), cell(124, 100, 10), cell(110, 50, 400) });   // candle volume 870, lots 400 -> share 0.4598
        s.add(115, 118, 110, 112, { cell(112, 100, 300) });                                          // net selling 200 -> cover 91.95
        trp::Engine E; E.tickForLines = tick; E.H.setSession(1, {}); for (auto& b : s.v) E.feed(b);
        CHECK(E.traps.size() == 1, "fuel case trap");
        if (!E.traps.empty()) { const trp::Trap& T = E.traps[0]; CHECK(T.fuel.size() == 2 && std::fabs(T.fuel[0].fuel - 400) < 1e-9 && std::fabs(T.fuel[1].fuel - (400 - 400.0 / 870 * 200)) < 1e-6, "burn = share x net exit flow"); }
        Syn c = base();
        c.add(124, 126, 105, 115, { cell(121, 300, 10), cell(124, 100, 10), cell(110, 50, 400) });
        c.add(115, 124, 114, 121, { cell(121, 300, 100) });                                          // close 121 (< avg 121.75): the 121 fills are not offside any more
        trp::Engine F; F.tickForLines = tick; F.P.qualAtr = 0.25; for (auto& b : c.v) F.feed(b);
        CHECK(F.traps.size() == 1 && F.traps[0].fuel.size() == 2 && F.traps[0].fuel[1].still == 100 && std::fabs(F.traps[0].fuel[1].fuel - 100) < 1e-9, "fuel capped by the lots still offside (100)");
        CHECK(F.traps.size() == 1 && F.traps[0].status == trp::SPENT && F.traps[0].endT == c.v.back().t, "fuel under min(1.0x normal, half the trap): spent, drawn to that candle");
    }
    {   // record round trip; old / garbled / other-kind lines ignored; the first line of a key wins
        Syn s = base();
        s.add(124, 126, 105, 115, { cell(121, 300, 10), cell(124, 100, 10), cell(110, 50, 400) });
        s.add(115, 125, 111, 123, { cell(123, 400, 20) });
        trp::Engine E; E.tickForLines = tick; for (auto& b : s.v) E.feed(b);
        std::vector<std::string> lines; lines.push_back(trp::REC_HEAD); for (auto& l : E.newLines) lines.push_back(l);
        lines.push_back("TRP|0.9.0|sweep|1|2|3|S|4|100|1|50|100|10|500|100|1.0|50|hist|2");          // old format
        lines.push_back("TRP|1.0.0|sweep|garbage");                                                  // garbled
        lines.push_back("TRP|1.0.0|pullback|20381|1|1|S|1|100|1|50|100|10|500|100|1.0|50|hist|2");   // a later kind: skipped, counted
        lines.push_back("XYZ|1.0.0");
        lines.push_back("TRP|1.0.0|sweep|20381|1|1|S|1|nan|1|50|100|10|500|100|1.0|50|hist|2");      // non-finite
        trp::Record R; trp::parseLines(lines, tick, R);
        CHECK(R.traps.size() == 1 && R.old == 1 && R.otherKind == 1 && R.bad == 3, "parse: 1 trap, 1 old, 1 other kind, 3 bad - got " + std::to_string(R.traps.size()) + "/" + std::to_string(R.old) + "/" + std::to_string(R.otherKind) + "/" + std::to_string(R.bad));
        if (!R.traps.empty() && !E.traps.empty()) {
            const trp::Trap& a = E.traps[0]; const trp::Trap& b = R.traps[0];
            CHECK(trp::trpLine(a, tick) == trp::trpLine(b, tick), "TRP round trip identical");
            CHECK(b.recEnd && b.recStatus == trp::FREED && b.recEndT == a.endT, "TRS end read back");
            std::vector<std::string> ra = trp::hoverRows(a, tick, 12.5, hm, 2.0), rb = trp::hoverRows(b, tick, 12.5, hm, 2.0);
            CHECK(ra[0] == rb[0] && ra[1] == rb[1] && ra[4] == rb[4], "hover header rows rebuilt from the record are the same");
            CHECK(ra.size() <= 12, "hover <= 12 rows"); for (auto& r : ra) CHECK(r.size() <= 125, "hover row <= 125 chars: " + r);
        }
        if (E.traps.empty()) return;
        std::vector<std::string> dup = lines; dup.push_back(trp::trpLine(E.traps[0], tick).replace(trp::trpLine(E.traps[0], tick).find("|400|"), 5, "|999|"));
        trp::Record R2; trp::parseLines(dup, tick, R2); CHECK(R2.traps.size() == 1 && R2.traps[0].lots == 400, "the first line of a key wins");
    }
    {   // no repaint: restart (new engine + the record so far) after every candle == one straight pass, on both real days
        const double t = 0.25;
        const char* files[2] = { "/tests_ob126/es_2026-10-05_3m.txt", "/tests_ob126/nq_2026-10-09_3m.txt" };
        for (int f = 0; f < 2; ++f) {
            std::vector<trp::Bar> B = loadFixture(D + files[f], t);
            trp::Engine S; S.tickForLines = t; for (auto& b : B) S.feed(b);
            std::vector<std::string> rec;                 // the record file as appended so far
            trp::Engine L; bool same = true;
            for (size_t n = 1; n <= B.size(); ++n) {
                trp::Engine R; R.tickForLines = t; trp::Record r; trp::parseLines(rec, t, r); R.loadRecord(r);
                for (size_t k = 0; k < n; ++k) R.feed(B[k]);
                for (auto& l : R.newLines) rec.push_back(l);
                if (n == B.size()) L = R;
                (void)same;
            }
            CHECK(L.traps.size() == S.traps.size(), "restart after every candle: the same number of traps");
            bool eq = L.traps.size() == S.traps.size();
            for (size_t i = 0; eq && i < S.traps.size(); ++i) eq = trp::trpLine(S.traps[i], t) == trp::trpLine(L.traps[i], t) && S.traps[i].status == L.traps[i].status && S.traps[i].endT == L.traps[i].endT
                                                          && S.traps[i].fuel.size() == L.traps[i].fuel.size();
            CHECK(eq, std::string("restart after every candle == straight pass (numbers, status, end, fuel line) ") + files[f]);
            int trp_ = 0, trs = 0, trd = 0; for (auto& l : rec) { if (l.find("TRP|") == 0) ++trp_; if (l.find("TRS|") == 0) ++trs; if (l.find("TRD|") == 0) ++trd; }
            CHECK(trp_ == (int)S.traps.size(), "each trap recorded once across restarts");
            int ends = 0; for (auto& T : S.traps) if (T.status != trp::ACTIVE) ++ends;
            CHECK(trs == ends && trd == 1, "each end recorded once, one TRD (session done)");
        }
    }
    {   // a record value wins over a recompute (e.g. the chart's volume at price changed after the decision)
        std::vector<trp::Bar> B = loadFixture(D + "/tests_ob126/es_2026-10-05_3m.txt", tick);
        trp::Engine S; S.tickForLines = tick; for (auto& b : B) S.feed(b);
        std::vector<std::string> rec; for (auto& l : S.newLines) if (l.find("TRP|") == 0) { rec.push_back(l); break; }
        std::string& l0 = rec[0]; const size_t p = l0.find("|1.0|"); if (p != std::string::npos) l0.replace(p, 5, "|9.9|");
        trp::Record r; trp::parseLines(rec, tick, r);
        trp::Engine R; R.tickForLines = tick; R.loadRecord(r); for (auto& b : B) R.feed(b);
        CHECK(!R.traps.empty() && std::fabs(R.traps[0].ratio - 9.9) < 1e-9 && R.traps[0].fromRecord, "the record's ratio is drawn (record wins)");
        bool reTRP = false; for (auto& l : R.newLines) if (l.find("TRP|") == 0 && l.find("|" + std::to_string(R.traps[0].sweepT) + "|") != std::string::npos) reTRP = true;
        CHECK(!reTRP, "a recorded trap is not appended again");
    }
    {   // normal: history needs 5 sessions with a record; the last 20 only; until then today's median so far
        trp::Hist H; trp::Params P;
        for (int k = 1; k <= 4; ++k) H.setSession(100 - k, { 100, 300 });
        std::pair<long long, bool> n = H.normalFor(100, { 50, 70, 1000 }, P); CHECK(n.second && n.first == 70, "4 sessions: building, today's median 70");
        H.setSession(95, {}); n = H.normalFor(100, { 50 }, P); CHECK(!n.second && n.first == 200, "5 sessions (one with no traps): median of 100 / 300 = 200");
        for (int k = 1; k <= 30; ++k) H.setSession(60 + k, { 10 });   // sessions 61..90, older than 95..99 -> outside the last 20? (61..90 are older)
        n = H.normalFor(100, { 1 }, P);
        // last 20 sessions before 100: 99..95 (5 sessions: 4x {100,300} + {}) + 90..76 (15 x {10}) -> pool 8 x {100,300} + 15 x 10 -> median 10
        CHECK(!n.second && n.first == 10, "only the last 20 sessions count - got " + std::to_string(n.first));
        CHECK(H.normalFor(96, { 5 }, P).first == 10 && H.sessionsBefore(96, P) == 20, "sessions before the trap's own only");
        CHECK(trp::medianOf({ 400, 490 }) == 445, "even count: the two middles' mean (445)");
    }
    {   // text
        CHECK(trp::commas(3625) == "3,625" && trp::commas(-1234567) == "-1,234,567" && trp::commas(0) == "0", "commas");
        CHECK(trp::pxText(31130.0, 0.25) == "7,782.50" && trp::pxText(31130.21, 0.25, true) == "7,782.5525", "pxText");
        CHECK(trp::pxText(1234.0, 0.0005) == "0.6170" && trp::pxText(25, 0.1) == "2.5", "pxText HG / GC ticks");
        trp::Trap T; T.lots = 3625; T.ratio = 8.1; CHECK(trp::spikeLabel(T) == "3,625 \xB7 8.1x", "spike label '3,625 · 8.1x'");
        CHECK(trp::tickValueOf("ES") == 12.5 && trp::tickValueOf("EU") == 6.25 && trp::tickValueOf("XX") == 0, "tick values");
    }
    {   // non-RTH candles: no traps, the RTH close ends active traps (ENDED) and writes TRD once
        Syn s = base();
        s.add(124, 126, 105, 115, { cell(121, 300, 10), cell(124, 100, 10), cell(110, 50, 400) });
        s.add(115, 117, 113, 114, { cell(114, 10, 10) }, false);
        trp::Engine E; E.tickForLines = tick; E.P.spentX = 0; E.P.smallSpentFrac = 0; for (auto& b : s.v) E.feed(b);
        CHECK(E.traps.size() == 1 && E.traps[0].status == trp::ENDED, "RTH close: active trap ended");
        int trd = 0; for (auto& l : E.newLines) if (l.find("TRD|") == 0) ++trd; CHECK(trd == 1, "one TRD at the RTH close");
        CHECK(E.H.sess.count(s.sess) == 1 && E.H.sess[s.sess].size() == 1, "the finished session joins the normal history");
    }
}

int main(int argc, char** argv) {
    D = argc > 1 ? argv[1] : "..";
    testRealDays();
    testUnits();
    std::printf("test_trapped_logic: %d passed, %d failed\n", g_ok, g_fail);
    return g_fail ? 1 : 0;
}
