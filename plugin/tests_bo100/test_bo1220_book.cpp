// test_bo1220_book.cpp -- lsBreakout 1.22.0 (Rassul 2026-11-11 "Ok build and I want the signal on rev to utilize this also when giving a signal",
// his choice "Require it"): a Rev is DRAWN only when the order book agrees (lsBookFlow fact F bits handed in by the host: bko::Engine::setBook).
//  [1] no lsBookFlow on the chart: every Rev drawn as before        [2] book on, nothing defending: no Rev drawn, every Rev decided "no"
//  [3] defending on the pivot bar / the Rev bar = drawn; pulling on the Rev bar = not drawn; the wrong side defending = not drawn
//  [4] the Rev bar's fact not there yet = not drawn and NOT decided; once decided the verdict never changes (no repaint); records unchanged
#include "BreakoutLogic.h"
#include <fstream>
#include <cstdio>
#include <cstring>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static const long long OFF = 2082844800LL;
struct Fix { std::vector<bko::In> B; std::map<long long, std::vector<wpc::VRow> > V; };
static long long tOf(const char* s, long& days, int& sec) { int y, mo, d, h, mi, se; if (std::sscanf(s, "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &se) != 6) return 0;
    days = svl::daysFromCivil(y, mo, d); sec = h * 3600 + mi * 60 + se; return (long long)days * 86400 + sec + OFF; }
static Fix load(const std::string& f, double tick) {
    Fix X; std::ifstream in(f); std::string l;
    while (std::getline(in, l)) {
        if (l.size() < 3 || l[1] != '|') continue;
        char ts[32] = { 0 };
        if (l[0] == 'B') { double o, h, lo, c, v; if (std::sscanf(l.c_str() + 2, "%19[^|]|%lf|%lf|%lf|%lf|%lf", ts, &o, &h, &lo, &c, &v) != 6) continue;
            bko::In b; b.t = tOf(ts, b.days, b.endSec); b.ok = true; b.o = o; b.h = h; b.l = lo; b.c = c; b.vol = (long long)v; X.B.push_back(b); }
        else if (l[0] == 'V') { double p, v, bu, se; if (std::sscanf(l.c_str() + 2, "%19[^|]|%lf|%lf|%lf|%lf", ts, &p, &v, &bu, &se) != 5) continue;
            long d; int s; const long long t = tOf(ts, d, s); wpc::VRow r; if (!wpc::toTicks(p, tick, r.tick)) continue; r.vol = (long long)v; r.dlt = (long long)(bu - se); X.V[t].push_back(r); }
    }
    return X;
}
[[maybe_unused]] static long long T(const char* hm, long day) { int h, m; std::sscanf(hm, "%d:%d", &h, &m); return (long long)day * 86400 + h * 3600 + m * 60 + OFF; }
[[maybe_unused]] static std::string hmOf(long long t) { const long long s = (t - OFF) % 86400; char b[8]; std::snprintf(b, sizeof b, "%02lld:%02lld", s / 3600, (s / 60) % 60); return b; }
static bko::Scene draw(const bko::Engine& E) {
    bko::View v; v.first = 0; v.last = (int)E.bars.size() - 1; v.area.l = -1000000; v.area.t = -1000000; v.area.r = 1000000; v.area.b = 1000000;
    v.X = [](int i) { return 10 + i * 40; }; v.Y = [](double p) { return (int)std::lround(400000 - p * 40); }; v.textW = [](const char* s, int) { return (int)std::strlen(s) * 6; };
    return bko::layout(E, v);
}
struct Run { bko::Engine E; std::map<long long, std::string> first; int switches = 0, provSwitch = 0; };
// replay: vapFrom = the first bar index whose VAP is "loaded" (bars before it have none until step vapAtStep)
static void replay(Run& R, const Fix& X, int from, int vapAtStep = -1, long long noVapFrom = 0, long long noVapTo = 0) {
    std::map<long long, std::string> lastFinal;
    for (int k = from; k <= (int)X.B.size(); ++k) {
        std::vector<bko::In> B(X.B.begin(), X.B.begin() + k); if (B.size() < 3) continue;
        const bool hide = vapAtStep >= 0 && k < vapAtStep;
        R.E.bocLive = true;
        R.E.vapAt = [&X, &R, hide, noVapFrom, noVapTo](int j, std::vector<wpc::VRow>& out) { out.clear(); const long long t = R.E.bars[(size_t)j].t;
            if (hide && t >= noVapFrom && t <= noVapTo) return 0;
            const auto it = X.V.find(t); if (it == X.V.end()) return 0; out = it->second; return out.empty() ? 0 : 1; };
        R.E.update(B, (long)B.size(), X.B[0].t, 0.25, 180, 510, 900); R.E.vapAt = nullptr;
        const bko::Scene S = draw(R.E);
        for (const bko::Txt& t : S.txt) if (t.kind == 2) { const long long bt = R.E.bars[(size_t)t.bar].t; const bool fin = R.E.pb.count(bt) || R.E.rpb.count(bt);
            if (!fin) continue;
            auto f = R.first.find(bt); if (f == R.first.end()) R.first[bt] = t.s; else if (f->second != t.s) ++R.switches; }
    }
}


static std::set<long long> revs(const bko::Engine& E, int* hid = nullptr) { const bko::Scene S = draw(E); std::set<long long> r; for (const bko::Txt& t : S.txt) if (t.kind == 4 && (t.s == "Rev" || t.s == "RevA")) r.insert(E.bars[(size_t)t.bar].t); if (hid) *hid = S.revNoBook; return r; }
// (1.23.0) the gate is a switch (bko::REV_BOOK_GATE): OFF = every Rev is drawn and the verdict is only logged - the "drawn" set of the checks below is then the set the gate WOULD draw
static std::set<long long> gated(const bko::Engine& E, const std::set<long long>& base) { if (bko::REV_BOOK_GATE) return revs(E); std::set<long long> r;
    auto one = [&](const bko::RevMark& m) { if (!base.count(m.tr)) return; const auto it = E.revBook.find(std::make_pair(m.tr, m.dir)); if (it != E.revBook.end() ? it->second == 1 : E.bookVerdict(m) == 1) r.insert(m.tr); };
    for (const auto& kv : E.rev) one(kv.second);
    for (const auto& kv : E.rro) one(kv.second);
    return r; }
int main() {
    CHECK(std::string(bko::VERSION) == "1.23.1", "version 1.23.1");
    const Fix X = load("pboc_es1109.txt", 0.25); Run R; replay(R, X, (int)X.B.size()); bko::Engine E = R.E;
    std::vector<bko::RevMark> M; for (const auto& kv : E.rev) M.push_back(kv.second); for (const auto& kv : E.rro) M.push_back(kv.second);
    const std::set<long long> base = revs(E); const size_t recs = E.revL.size() + E.rroL.size();
    CHECK(M.size() >= 4 && base.size() >= 4, "fixture has Revs: " + std::to_string(M.size()) + " drawn " + std::to_string(base.size()));
    CHECK(!E.bookSeen && E.setBook(std::map<long long, int>()) == 0 && revs(E) == base, "[1] no lsBookFlow: Revs as before");
    std::map<long long, int> none; for (const bko::In& b : E.bars) none[b.t] = 0;
    { bko::Engine A = E; const int n = A.setBook(none); const std::set<long long> r = gated(A, base); CHECK(bko::REV_BOOK_GATE || revs(A) == base, "[2] gate OFF: every Rev still drawn"); int yes = 0; for (const auto& kv : A.revBook) yes += kv.second;
      CHECK(A.bookSeen && n == (int)M.size() && r.empty() && yes == 0, "[2] nothing defending: no Rev drawn (" + std::to_string(n) + " decided no)"); }
    { bko::Engine A = E; std::map<long long, int> b = none; std::set<long long> want;
      for (size_t i = 0; i < M.size(); ++i) { const bko::RevMark& m = M[i]; const int def = m.dir > 0 ? 1 : 4, pull = m.dir > 0 ? 2 : 8, other = m.dir > 0 ? 4 : 1;
          if (i % 4 == 0) { b[m.tp] |= def; want.insert(m.tr); }                      // defending on the pivot bar
          else if (i % 4 == 1) { b[m.tr] |= def; want.insert(m.tr); }                 // defending on the Rev bar
          else if (i % 4 == 2) { b[m.tp] |= def; b[m.tr] |= pull; }                   // defended, then pulling on the Rev bar
          else b[m.tr] |= other; }                                                   // the wrong side defending
      // a Rev bar shared by two marks, or one mark's pivot inside another's window, can blur the hand-made cases: judge each mark by the rule itself
      std::set<long long> exp; for (const bko::RevMark& m : M) { const int def = m.dir > 0 ? 1 : 4, pull = m.dir > 0 ? 2 : 8; bool ok = !(b[m.tr] & pull), d = false;
          for (auto it = b.lower_bound(m.tp); it != b.end() && it->first <= m.tr; ++it) { if (it->second & def) d = true; }
          if (ok && d && base.count(m.tr)) exp.insert(m.tr); }
      A.setBook(b); const std::set<long long> r = gated(A, base);
      CHECK(r == exp && !r.empty() && r.size() < base.size(), "[3] drawn = the rule: " + std::to_string(r.size()) + " of " + std::to_string(base.size()));
      int w = 0; for (long long t : want) if (exp.count(t)) ++w; CHECK(w >= 1, "[3] a defended Rev is drawn");
      // [4] sticky: the book changes afterwards, the decisions do not
      const std::map<std::pair<long long, int>, int> dec = A.revBook; std::map<long long, int> all = none; for (auto& kv : all) kv.second = 5;
      CHECK(A.setBook(all) == 0 && A.revBook == dec && gated(A, base) == r, "[4] decided once: a later book never changes a Rev");
      CHECK(A.revL.size() + A.rroL.size() == recs, "[4] records unchanged"); }
    { bko::Engine A = E; const bko::RevMark& m = M.back(); std::map<long long, int> b; for (const bko::In& x : A.bars) if (x.t < m.tr) b[x.t] = 5;
      A.setBook(b); CHECK(!A.revBook.count(std::make_pair(m.tr, m.dir)) && !gated(A, base).count(m.tr), "[4] the Rev bar's fact not there yet: not drawn, not decided");
      b[m.tr] = 5; A.setBook(b); const auto it = A.revBook.find(std::make_pair(m.tr, m.dir)); CHECK(it != A.revBook.end() && it->second == 1, "[4] decided when its fact arrives");
      bko::Engine C = E; std::map<long long, int> late; late[A.bars.back().t] = 0; C.setBook(late); const auto ic = C.revBook.find(std::make_pair(M.front().tr, M.front().dir));
      CHECK(ic != C.revBook.end() && ic->second == 0, "[4] no fact for the Rev bar while a later candle has one = no"); }
    std::printf("test_bo1220_book: %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
