// replay_qm.cpp -- QueueMeter.h 1.0.0 replayed on the REAL NQ 2026-10-09 full-depth MBO (every order, Databento raw -> the IRT-style
// NEW / MOD / DEL / FILL rows of tests_qm/conv_qm.py) with the 3-min candles of that night / morning, the G1 31000 / HVL 30980 /
// HVL0 31100 MenthorQ levels of the mockup.
// Checks (Rassul's approved mockup numbers):
//   8:55:39 AM  G1 31000 zone (bids) reads ~76% and HOLDS (green Q at the 8:57 close)
//   9:42:26 AM  the 9:24 pivot-low zone (31024-31027 area) reads ~33%, originals 0%, ~1 s to empty; red Q (it broke 9:42:28)
//   restart: a fresh engine that adopts the record mid-way never writes a Q twice and never changes a written one
// Build (from plugin/): g++ -std=c++17 -O2 -I. tests_qm/replay_qm.cpp -o rqm && ./rqm <events> <bars>
#include "QueueMeter.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <string>
#include <vector>

static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
struct Ev { long long t; int act, side; long long pt, sz, oid; bool reset; };
struct Bar { long long ms; double h, l, c; };
static long long ctMs(int h, int m, int s) { return 1791504000000LL + (h * 3600LL + m * 60 + s) * 1000; }   // 2026-10-09 00:00 CT (naive epoch)

static void setup(qm::Engine& E) {
    E.market = "NQ"; E.tick = 0.25; E.rule = qm::callRuleFor("NQ");   // (1.1.0) NQ: SHOW? red only
    if (const char* z = std::getenv("QM_ZONE")) E.P.zoneAtr = std::atof(z);      // exploration only (the defaults are what ships)
    if (const char* o = std::getenv("QM_ORIG")) E.P.origTicks = std::atoi(o);
    std::vector<qm::Anchor> L;
    const double lv[3] = { 31000.0, 30980.0, 31100.0 }; const char* nm[3] = { "G1", "HVL", "HVL0" };
    for (int i = 0; i < 3; ++i) { qm::Anchor a; a.px = (qm::Tick)(lv[i] / 0.25); a.kind = qm::K_LEVEL; a.name = nm[i]; L.push_back(a); }
    E.setLevels(L);
}
struct Run {
    qm::Engine E; size_t bi = 0; std::vector<std::string> rec;
    std::vector<std::pair<long long, std::vector<qm::Live> > > snaps;
};
static void step(Run& R, const std::vector<Bar>& B, const Ev& e, const std::vector<long long>& at, size_t& ai) {
    while (R.bi < B.size() && B[R.bi].ms <= e.t) {
        R.E.closeBar(B[R.bi].ms, B[R.bi].h, B[R.bi].l, B[R.bi].c);
        for (size_t k = 0; k < R.E.rows.size(); ++k) R.rec.push_back(R.E.rows[k]);
        R.E.rows.clear(); ++R.bi;
    }
    while (ai < at.size() && at[ai] <= e.t) { R.snaps.push_back(std::make_pair(at[ai], R.E.live(at[ai]))); ++ai; }
    if (e.reset) { R.E.book.clear(); R.E.lv[0].clear(); R.E.lv[1].clear(); return; }
    R.E.onEvent(e.act, e.side, e.pt, e.sz, e.oid, e.t);
}
static const qm::Live* near(const std::vector<qm::Live>& L, int side, double px) {
    const qm::Live* b = nullptr;
    for (size_t i = 0; i < L.size(); ++i) if (L[i].side == side && L[i].lo * 0.25 <= px + 1 && L[i].hi * 0.25 >= px - 1) b = &L[i];
    return b;
}
static void show(const char* tag, const qm::Live* v) {
    if (!v) { std::printf("%s: no test\n", tag); return; }
    std::printf("%s: %s %s anchor %.2f zone %.2f-%.2f q0 %lld q %lld = %.0f%%  orig %s  eat %.1f/s (norm %.1f)  pull %.1f/s  refill %.1f/s  tte %s  risk %s (%s)%s\n",
                tag, v->side == qm::BID ? "BID" : "ASK", qm::kindCode(v->kind), v->anchor * 0.25, v->lo * 0.25, v->hi * 0.25, v->q0, v->q, v->pct * 100,
                v->origPct < 0 ? "n/a" : (std::to_string((int)(v->origPct * 100 + 0.5)) + "%").c_str(), v->eat, v->norm, v->pull, v->refill,
                v->tte < 0 ? "holding" : (std::to_string(v->tte).substr(0, 4) + " s").c_str(), v->high ? "HIGH" : "LOW", v->why.c_str(), v->warn ? "  [YELLOW orig 0%]" : "");
}
int main(int argc, char** argv) {
    if (argc < 3) { std::printf("usage: rqm <events> <bars>\n"); return 2; }
    std::vector<Ev> ev; ev.reserve(20000000);
    { const std::string ap = argv[1]; const bool gz = ap.size() > 3 && ap.compare(ap.size() - 3, 3, ".gz") == 0;
      FILE* f = gz ? popen(("gzip -dc '" + ap + "'").c_str(), "r") : std::fopen(argv[1], "r"); if (!f) { std::printf("no events\n"); return 2; }
      char ln[256];
      while (std::fgets(ln, sizeof(ln), f)) {
          Ev e{}; if (ln[0] == 'R') { e.t = std::atoll(ln + 2); e.reset = true; ev.push_back(e); continue; }
          if (std::sscanf(ln, "E,%lld,%d,%d,%lld,%lld,%lld", &e.t, &e.act, &e.side, &e.pt, &e.sz, &e.oid) == 6) ev.push_back(e);
      }
      if (gz) pclose(f); else std::fclose(f); }
    std::vector<Bar> B;
    { std::ifstream f(argv[2]); std::string ln; while (std::getline(f, ln)) { Bar b; if (std::sscanf(ln.c_str(), "%lld,%lf,%lf,%lf", &b.ms, &b.h, &b.l, &b.c) == 4) B.push_back(b); } }
    std::printf("events %zu bars %zu\n", ev.size(), B.size());
    const long long MH = ctMs(8, 55, 39), MB = ctMs(9, 42, 26), MB2 = ctMs(9, 42, 28);
    std::vector<long long> at = { ctMs(8, 55, 15), ctMs(8, 55, 29), MH, ctMs(9, 41, 50), ctMs(9, 42, 8), MB, MB2 };
    // 1) one straight pass
    Run A; setup(A.E); size_t ai = 0;
    for (size_t i = 0; i < ev.size(); ++i) step(A, B, ev[i], at, ai);
    { while (A.bi < B.size()) { A.E.closeBar(B[A.bi].ms, B[A.bi].h, B[A.bi].l, B[A.bi].c); for (auto& r : A.E.rows) A.rec.push_back(r); A.E.rows.clear(); ++A.bi; } }
    std::printf("tests started %lld, unknown fills %lld, Q rows %zu\n", A.E.nStarted, A.E.nUnknownFill, A.rec.size());
    for (auto& r : A.rec) { qm::Engine::QRec q; qm::Engine::parseRec(r, q); const long long s = (q.t0 / 1000) % 86400; if (s >= 8 * 3600 + 30 * 60) std::printf("  %02lld:%02lld:%02lld %s\n", s / 3600, s / 60 % 60, s % 60, r.c_str()); }
    for (auto& s : A.snaps) {
        const long long x = (s.first / 1000) % 86400; char tag[64]; std::snprintf(tag, sizeof(tag), "%02lld:%02lld:%02lld", x / 3600, x / 60 % 60, x % 60);
        const bool hold = s.first < ctMs(9, 0, 0);
        show(tag, near(s.second, qm::BID, hold ? 31000.0 : 31026.0));
    }
    for (auto& r : A.rec) if (r.rfind("C|", 0) == 0) { long long t0 = std::atoll(r.c_str() + 2); const long long x = (t0 / 1000) % 86400; if ((x >= 8 * 3600 + 54 * 60 && x <= 8 * 3600 + 56 * 60) || (x >= 9 * 3600 + 41 * 60 && x <= 9 * 3600 + 43 * 60)) std::printf("  call %s\n", r.c_str()); }
    { long long nr = 0, ng = 0, nn = 0; for (auto& r : A.rec) if (r.rfind("C|", 0) == 0) { if (r.find("|R|") != std::string::npos) ++nr; else if (r.find("|G|") != std::string::npos) ++ng; else ++nn; }
      std::printf("calls: red %lld green %lld none %lld\n", nr, ng, nn); CHECK(ng == 0 && nr > 0, "NQ: red calls only (green off), some red calls"); }
    const qm::Live* h = nullptr; const qm::Live* b = nullptr;
    for (auto& s : A.snaps) { if (s.first == MH) h = near(s.second, qm::BID, 31000.0); if (s.first == MB) b = near(s.second, qm::BID, 31026.0); }
    CHECK(h != nullptr, "8:55:39 a bid test at G1 31000");
    if (h) {
        CHECK(h->kind == qm::K_LEVEL && h->name == "G1", "the hold test is the G1 level badge");
        CHECK(h->pct >= 0.66 && h->pct <= 0.86, "8:55:39 G1 reads ~76% (66-86)");
        // (finding) the mockup's "orig 22%" at the hold came from orders its rebuild kept after they were filled (conv bug fixed in
        // tests_qm/conv_qm.py: order 6883242054741 etc.); on the corrected book the G1 originals were also gone by 8:55:15 - the hold
        // came from NEW bids. What separates the hold from the break is the queue + time to empty, not the originals alone:
        CHECK(!h->high && h->tte < 0, "8:55:39 break risk LOW, holding (refills keep up)");
    }
    CHECK(b != nullptr, "9:42:26 a bid test at the 9:24 low zone");
    if (b) {
        CHECK(b->kind == qm::K_PIVOT, "the 9:42 test is the 9:24 pivot low (no key level: unlabeled badge)");
        CHECK(b->pct >= 0.23 && b->pct <= 0.43, "9:42:26 reads ~33% (23-43)");
        CHECK(b->origPct >= 0 && b->origPct <= 0.005 && b->warn, "9:42:26 originals 0% -> yellow");
        CHECK(b->tte >= 0 && b->tte <= 3.0, "9:42:26 about 1 s to empty (<= 3 s)");
        CHECK(b->high, "9:42:26 break risk HIGH");
    }
    bool gHold = false, rBrk = false;
    for (auto& r : A.rec) { qm::Engine::QRec q; if (!qm::Engine::parseRec(r, q)) continue;
        if (q.side == qm::BID && q.anchor == 124000 && q.t0 >= ctMs(8, 54, 0) && q.t0 <= ctMs(8, 57, 0)) { gHold = q.g == 1; std::printf("hold Q: %s\n", r.c_str()); }
        if (q.side == qm::BID && q.t0 >= ctMs(9, 41, 0) && q.t0 <= ctMs(9, 42, 30) && q.anchor * 0.25 >= 31020 && q.anchor * 0.25 <= 31030) { rBrk = q.g == 0; std::printf("break Q: %s\n", r.c_str()); } }
    CHECK(gHold, "green Q for the G1 hold (decided at a close, recorded)");
    CHECK(rBrk, "red Q for the 9:42 break");
    // 2) restart at 9:42:10 (mid-test) and at 9:50: a fresh engine adopts the record so far; the book comes back only as the feed moves
    for (long long cutT : { ctMs(9, 42, 10), ctMs(9, 50, 0) }) {
        Run C; setup(C.E); size_t ci = 0; std::vector<long long> none;
        size_t i = 0; for (; i < ev.size() && ev[i].t < cutT; ++i) step(C, B, ev[i], none, ci);
        std::vector<std::string> before = C.rec;
        Run D; setup(D.E); D.bi = 0;
        // the restarted plugin reloads its candles (closeBar on history: no tests exist, nothing decided) and the record
        while (D.bi < B.size() && B[D.bi].ms <= cutT) { D.E.closeBar(B[D.bi].ms, B[D.bi].h, B[D.bi].l, B[D.bi].c); ++D.bi; }
        D.E.rows.clear(); D.rec = before; D.E.adopt(before);
        for (; i < ev.size(); ++i) step(D, B, ev[i], none, ci);
        while (D.bi < B.size()) { D.E.closeBar(B[D.bi].ms, B[D.bi].h, B[D.bi].l, B[D.bi].c); for (auto& r : D.E.rows) D.rec.push_back(r); D.E.rows.clear(); ++D.bi; }
        bool same = D.rec.size() >= before.size(); for (size_t k = 0; same && k < before.size(); ++k) same = D.rec[k] == before[k];
        CHECK(same, "restart: every Q written before the restart is unchanged");
        std::set<std::string> K; bool dup = false;
        for (auto& r : D.rec) { qm::Engine::QRec q; if (!qm::Engine::parseRec(r, q)) continue; if (!K.insert(qm::Engine::key(q.side, q.anchor, q.startBar)).second) dup = true; }
        CHECK(!dup, "restart: no Q decided twice");
        std::printf("restart at %lld: before %zu rows, after %zu rows (straight pass %zu)\n", (cutT / 1000) % 86400, before.size(), D.rec.size(), A.rec.size());
    }
    std::printf("replay_qm: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
