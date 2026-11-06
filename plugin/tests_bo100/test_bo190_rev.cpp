// test_bo190_rev.cpp -- lsBreakout 1.9.0 Rev (2026-11-06 Rassul "PB Rev ... a thrust after a pullback", "or it can be the pivot bar also",
// "just a green Rev mark", "the biggest range", "only have 1 Rev per wave", "yes build"). SDK-free.
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. test_bo190_rev.cpp -o t190 && ./t190
//   [1] every Rev = an independent re-check: the bar is in its Pb's wave (pivot bar .. before the next opposite pivot's confirm, no trade
//       beyond the pivot extreme before it), green (BD: red), low below the pivot bar's high (BD: high above its low), and range >= 1.2 x
//       Wilder ATR(14) of the bar before, or bigger than each of the last 5 green (red) bars. The FIRST such bar (none earlier in the wave).
//   [2] every Pb without a Rev: no bar of its (closed) wave qualifies (independent scan).
//   [3] one Rev per Pb / wave; the Rev bar is at or after the pivot bar; the decision bar = later of the Rev bar and the Pb confirm.
//   [4] no repaint: a bar-by-bar replay ends with the same Revs as one full pass, and a Rev once made never changes.
//   [5] layout: every Rev drawn says "Rev", kind 4, green below the bar after a BO / red above after a BD, never on another letter.
//   [6] records: one 20-field BRV| line per Rev with its key; version 1.9.0.
//   [7] lines (10:14 "lets add 1.618 as the 2nd and final target"): Stop = 1 tick beyond the lower of the pivot low and the BO candle low,
//       T1 = the wave extreme from the BO candle to the pivot bar (Rly high), T2 = origin + 1.618 x (T1 - origin); independent ends:
//       a target ends at its first touch, every line at the stop / T2 / the next Rev / the session's last bar; never past the session;
//       the line levels never change in the replay; lines drawn = 3 per visible Rev, in the right colours.
#include "BreakoutLogic.h"
#include <fstream>
#include <set>
#include <map>
#include <cstdio>
#include <cstring>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static const long long OFF = 2082844800LL;
static std::vector<bko::In> load(const std::string& f) {
    std::ifstream in(f); std::string l; std::vector<bko::In> v;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        bko::In b; b.days = svl::daysFromCivil(y, mo, d); b.endSec = h * 3600 + mi * 60 + s; b.t = (long long)b.days * 86400 + b.endSec + OFF;
        b.ok = true; b.o = o; b.h = hi; b.l = lo; b.c = c; b.vol = (long long)vo; v.push_back(b); }
    return v;
}
// independent Wilder ATR(14)
static std::vector<double> atrOf(const std::vector<bko::In>& B) {
    std::vector<double> a(B.size(), NAN); double y = 0; bool have = false;
    for (size_t k = 1; k < B.size(); ++k) { const double tr = std::max(B[k].h - B[k].l, std::max(std::fabs(B[k].h - B[k - 1].c), std::fabs(B[k].l - B[k - 1].c)));
        y = have ? y + (tr - y) / 14.0 : tr; have = true; a[k] = y; }
    return a;
}
// does bar j qualify for a Pb of direction d with pivot bar p? (colour, beyond the pivot bar, range rule)
static int qual(const std::vector<bko::In>& B, const std::vector<double>& A, int j, int p, int d, double tk) {
    const bko::In& b = B[(size_t)j]; const bko::In& pv = B[(size_t)p]; const double e = tk * 0.25;
    if (!(d > 0 ? b.c > b.o : b.c < b.o)) return 0;
    if (!(d > 0 ? b.l < pv.h - e : b.h > pv.l + e)) return 0;
    const double r = b.h - b.l; int why = 0;
    if (j >= 1 && std::isfinite(A[(size_t)j - 1]) && r >= 1.2 * A[(size_t)j - 1] - e * 0.01) why |= 1;
    double big = 0; int got = 0; for (int k = j - 1; k >= 0 && k >= j - 400 && got < 5; --k) { const bko::In& x = B[(size_t)k]; if (d > 0 ? x.c > x.o : x.c < x.o) { big = std::max(big, x.h - x.l); ++got; } }
    if (got == 5 && r > big + e) why |= 2;
    return why;
}
int main() {
    struct Mk { const char* f; const char* m; double tk; } MS[4] = { { "wk_ES_eve_3m.txt", "ES", 0.25 }, { "wk_NQ_3m.txt", "NQ", 0.25 }, { "wk_GC_3m.txt", "GC", 0.1 }, { "eu_1104_1105_3m.txt", "EU", 0.00005 } };
    int totRev = 0;
    for (const Mk& m : MS) {
        const std::vector<bko::In> B = load(m.f); int om = 0, cm = 0; svl::rthOf(m.m, om, cm);
        std::map<long long, int> at; for (size_t k = 0; k < B.size(); ++k) at[B[k].t] = (int)k;
        const std::vector<double> A = atrOf(B);
        bko::Engine E; E.update(B, (long)B.size(), B[0].t, m.tk, 180, om, cm);
        const int last = (int)B.size() - 2; const double e = m.tk * 0.25;
        int bad1 = 0, bad2 = 0, bad3 = 0, withRev = 0, onPivot = 0, why[4] = { 0, 0, 0, 0 };
        for (const auto& kv : E.pb) { const bko::PbMark& P = kv.second; const int p = at[P.tp], c = at[P.tc], d = P.dir > 0 ? 1 : -1;
            long long endT = 0; for (const bko::Pivot& q : E.piv) if (q.t > P.tp && q.type == d) { endT = q.tc; break; }
            int first = -1;
            for (int j = p; j <= last; ++j) { if (endT && B[(size_t)j].t >= endT) break;
                if (j > p && (d > 0 ? B[(size_t)j].l < B[(size_t)p].l - e : B[(size_t)j].h > B[(size_t)p].h + e)) break;
                if (qual(B, A, j, p, d, m.tk)) { first = j; break; } }
            const auto it = E.rev.find(P.tp);
            if (it == E.rev.end()) { if (first >= 0) ++bad2; continue; }
            ++withRev; const bko::RevMark& R = it->second; const int j = at[R.tr];
            if (first != j || R.dir != d || qual(B, A, j, p, d, m.tk) != R.why) ++bad1;
            if (j < p || R.td != B[(size_t)std::max(j, c)].t || R.tp != P.tp) ++bad3;
            onPivot += j == p; why[R.why & 3]++;
        }
        for (const auto& kv : E.rev) if (!E.pb.count(kv.first)) ++bad3;
        totRev += withRev;
        CHECK(bad1 == 0 && withRev > 0, std::string("[1] ") + m.m + ": every Rev is the first qualifying bar of its Pb's wave (" + std::to_string(withRev) + " Rev, " + std::to_string(bad1) + " bad)");
        CHECK(bad2 == 0, std::string("[2] ") + m.m + ": no Pb without a Rev has a qualifying bar in its wave (" + std::to_string(bad2) + " missed)");
        CHECK(bad3 == 0, std::string("[3] ") + m.m + ": one Rev per Pb, at / after the pivot bar, decided at the later of its close and the Pb confirm");
        // [4] no repaint
        bko::Engine R; std::map<long long, long long> firstSeen; int changed = 0;
        for (int n = 60; n <= (int)B.size(); ++n) { std::vector<bko::In> Q(B.begin(), B.begin() + n); R.update(Q, n, Q[0].t, m.tk, 180, om, cm);
            for (const auto& kv : R.rev) { const auto f = firstSeen.find(kv.first); if (f == firstSeen.end()) firstSeen[kv.first] = kv.second.tr; else if (f->second != kv.second.tr) ++changed;
                if (kv.second.td > Q[(size_t)n - 2].t) ++changed; } }   // never decided on the forming bar
        bool same = R.rev.size() == E.rev.size(); for (const auto& kv : E.rev) { const auto f = R.rev.find(kv.first); if (f == R.rev.end() || f->second.tr != kv.second.tr) same = false; }
        CHECK(same && changed == 0, std::string("[4] ") + m.m + ": bar-by-bar replay == one full pass, no Rev changed (" + std::to_string(R.rev.size()) + " vs " + std::to_string(E.rev.size()) + ", " + std::to_string(changed) + " changed)");
        // [5] layout over the whole chart
        bko::View v; v.first = 0; v.last = (int)B.size() - 1; v.area = { 0, 0, 1000000, 1000000 };
        double lo = 1e18, hi = -1e18; for (const bko::In& b : B) { lo = std::min(lo, b.l); hi = std::max(hi, b.h); }
        v.X = [](int j) { return 10 + j * 12; }; v.Y = [lo, hi](double px) { return 20 + (int)((hi - px) / (hi - lo) * 400000.0); };
        v.textW = [](const char* s, int) { return (int)std::strlen(s) * 6; };
        const bko::Scene S = bko::layout(E, v); int nr = 0, badL = 0;
        for (size_t a = 0; a < S.txt.size(); ++a) { const bko::Txt& T = S.txt[a]; if (T.kind != 4) continue; ++nr;
            const bko::In& b = E.bars[(size_t)T.bar];
            if (T.s != "Rev" || T.col != (T.dir > 0 ? bko::C_UP : bko::C_DN) || (T.dir > 0 ? T.f.t <= v.Y(b.l) : T.f.b >= v.Y(b.h))) ++badL;
            for (size_t c2 = 0; c2 < S.txt.size(); ++c2) if (c2 != a && bko::hits(T.f, S.txt[c2].f)) ++badL; }
        CHECK(badL == 0 && nr + S.noRoom >= (int)E.rev.size() && nr == S.revShown && nr > 0, std::string("[5] ") + m.m + ": Rev letters: green below / red above the bar, never on another letter (" + std::to_string(nr) + " drawn, " + std::to_string(badL) + " bad)");
        // [6] records
        int badR = 0; for (const auto& kv : E.revL) { const std::string& l = kv.second.line; int bars_ = 0; for (char ch : l) bars_ += ch == '|';
            if (bars_ != 19 || l.rfind("BRV|", 0) != 0 || bko::recKey(l).empty() || l.find("|1.9.0-zz3") == std::string::npos) ++badR; }
        CHECK(badR == 0 && E.revL.size() == E.rev.size(), std::string("[6] ") + m.m + ": one 20-field BRV| record per Rev (" + std::to_string(E.revL.size()) + ")");
        // [7] lines
        int badV = 0, badE = 0, segs = 0; std::map<std::string, int> whyN;
        for (const auto& kv : E.rev) { const bko::RevMark& M = kv.second; const bko::PbMark& P = E.pb.at(kv.first); const int d = M.dir;
            const int i0 = at[P.t0], p = at[P.tp]; double ext = d > 0 ? -1e300 : 1e300; for (int k = i0; k <= p; ++k) ext = d > 0 ? std::max(ext, B[(size_t)k].h) : std::min(ext, B[(size_t)k].l);
            const double stop = d > 0 ? std::min(B[(size_t)p].l, B[(size_t)i0].l) - m.tk : std::max(B[(size_t)p].h, B[(size_t)i0].h) + m.tk;
            if (std::fabs(M.t1 - ext) > 1e-9 || std::fabs(M.stop - stop) > 1e-9 || std::fabs(M.t2 - (M.org + 1.618 * (ext - M.org))) > 1e-6 || !((M.t2 - M.t1) * d > 0) || !((M.t1 - M.stop) * d > 0)) ++badV;
            // independent ends
            const int dI = at[M.td]; const long long ss = bko::sessOf(B[(size_t)dI].days, B[(size_t)dI].endSec);
            long long nx = 0; for (const auto& o : E.rev) if (o.second.td > M.td && (!nx || o.second.td < nx)) nx = o.second.td;
            int endAll = -1, e1 = -1, e2 = -1; std::string w = "live";
            for (int j = dI + 1; j <= (int)B.size() - 2; ++j) { const bko::In& b = B[(size_t)j];
                if (bko::sessOf(b.days, b.endSec) != ss) { endAll = j - 1; w = "session end"; break; }
                if (nx && b.t >= nx) { endAll = j; w = "next Rev"; break; }
                if (d > 0 ? b.l <= M.stop : b.h >= M.stop) { endAll = j; w = "stop"; break; }
                if (e1 < 0 && (d > 0 ? b.h >= M.t1 : b.l <= M.t1)) e1 = j;
                if (d > 0 ? b.h >= M.t2 : b.l <= M.t2) { e2 = j; if (e1 < 0) e1 = j; endAll = j; w = "T2"; break; } }
            std::string why; const std::vector<bko::RevSeg> G = bko::revLines(E, M, &why); ++whyN[why];
            const int last = endAll < 0 ? (int)B.size() - 1 : std::max(at[M.tr], endAll);
            if (G.size() != 3 || why != w) { ++badE; continue; }
            const int want[3] = { last, e1 >= 0 && e1 <= last ? e1 : last, e2 >= 0 ? e2 : last };
            for (const bko::RevSeg& g : G) { ++segs; if (g.i0 != at[M.tr] || g.i1 != want[g.kind] || g.i1 < g.i0) ++badE;
                if (g.i1 <= (int)B.size() - 2 && bko::sessOf(B[(size_t)g.i1].days, B[(size_t)g.i1].endSec) != ss && g.i1 != g.i0) ++badE; } }
        int lvChg = 0; for (const auto& kv : R.rev) { const auto f = E.rev.find(kv.first); if (f == E.rev.end() || f->second.t1 != kv.second.t1 || f->second.t2 != kv.second.t2 || f->second.stop != kv.second.stop) ++lvChg; }
        int nl = (int)S.lines.size(), badC = 0; for (const bko::Line& L : S.lines) if (!(L.col == bko::C_UP || L.col == bko::C_DN) || L.x1 <= L.x0) ++badC;
        CHECK(badV == 0 && badE == 0 && lvChg == 0 && nl == segs && badC == 0, std::string("[7] ") + m.m + ": Rev lines Stop / Rly / 1.618: levels, ends (stop, T2, next Rev, session end), no level change in the replay, " + std::to_string(nl) + " drawn (" + std::to_string(badV) + " / " + std::to_string(badE) + " / " + std::to_string(lvChg) + " bad)");
        std::printf("    line ends: T2 %d, stop %d, next Rev %d, session end %d, live %d\n", whyN["T2"], whyN["stop"], whyN["next Rev"], whyN["session end"], whyN["live"]);
        std::printf("  %s: %zu Pb, %d Rev (%d on the pivot bar; atr only %d, biggest only %d, both %d), drawn %d\n", m.m, E.pb.size(), withRev, onPivot, why[1], why[2], why[3], nr);
    }
    CHECK(std::string(bko::VERSION) == "1.9.0" && bko::REV_ATR_X == 1.2 && bko::REV_GREEN_N == 5 && totRev > 20, "[6] version 1.9.0, 1.2 ATR, last 5 bars");
    std::printf("RESULT %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
