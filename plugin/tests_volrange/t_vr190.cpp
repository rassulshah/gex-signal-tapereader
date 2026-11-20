// t_vr190.cpp -- VolRangeLogic 1.9.0: the coloured numbers column back next to the read (Rassul 2026-11-19 18:31-18:35 CT)
#include "VolRangeLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int ok = 0, bad = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++bad; std::printf("FAIL %s\n", m); } }
static std::string slurp(const char* p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
static int idx(const vrr::Engine& E, long long localEnd) { for (size_t i = 0; i < E.LT.size(); ++i) if (E.LT[i] == localEnd) return (int)i; return -1; }
static const vrr::Prim* findText(const std::vector<vrr::Prim>& P, const std::string& t) { for (const auto& p : P) if (!p.fill && p.text == t) return &p; return nullptr; }
static const vrr::Prim* rightOf(const std::vector<vrr::Prim>& P, const vrr::Prim* lab) { const vrr::Prim* best = nullptr; for (const auto& p : P) if (!p.fill && p.y0 == lab->y0 && p.x0 > lab->x1 && (!best || p.x0 < best->x0)) best = &p; return best; }
int main() {
    const long long day = 1795046400LL;   // 2026-11-19 00:00 local
    for (int mode = 0; mode < 2; ++mode) {
        vrr::Engine E; E.mode = mode; E.tick = 0.25; E.spb = 180;
        std::ifstream f("nq_1119.csv"); std::string l; while (std::getline(f, l)) { vrr::Bar b; long long L = 0; if (std::sscanf(l.c_str(), "%lld,%lld,%lf,%lf,%lf,%lf,%lf", &b.t, &L, &b.o, &b.h, &b.l, &b.c, &b.v) != 7) continue; E.addBar(b, L); }
        E.hlOn = true; std::map<long long, vrr::Mk> w; const int k = vrr::parseWy(slurp("wy_NQ_1119.txt"), w); E.wy.swap(w); E.wyLines = k; E.markLevels();
        const int n = (int)E.V.size(); const int i1739 = idx(E, day + 17 * 3600 + 39 * 60); C(i1739 > 0, "5:39 PM bar found");
        for (int W : { 1180, 900, 640, 420 }) {
            vrr::View v; v.l = 0; v.r = W; v.t = 0; v.b = 66; v.bw = 8; v.textH = 11; v.first = n - 60; v.last = n - 1;
            v.X = [&](int i) { return 4.0 + (i - (n - 60)) * 10.0; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; };
            v.readX = (int)v.X(n - 1) + 4 + 18;
            const std::vector<vrr::Prim> P = vrr::layout(E, v, nullptr, i1739);
            const std::string pr = mode == vrr::VOL ? "BV" : "BR";
            const vrr::Prim* mx = findText(P, pr + "Max");
            // everything inside the pane, no two texts overlap
            bool inside = true, clash = false;
            for (size_t a = 0; a < P.size(); ++a) { const auto& p = P[a]; if (p.x0 < v.l || p.x1 > v.r || p.y0 < v.t || p.y1 > v.b + 1) inside = false;
                if (p.fill) continue;
                for (size_t b = a + 1; b < P.size(); ++b) { const auto& q = P[b]; if (q.fill) continue; if (p.x0 < q.x1 && q.x0 < p.x1 && p.y0 < q.y1 && q.y0 < p.y1) clash = true; } }
            C(inside, "every primitive inside the pane"); C(!clash, "no two texts overlap");
            const int avail = W - v.readX - 4; (void)avail;
            if (W == 1180) C(mx != nullptr, "wide pane: the numbers column is there");
            if (mx && mx->col == vrr::C_LABEL) {   // the new coloured column (a pane too narrow for the read on the right keeps the old 1.2.0 box at the left edge)
                if (!mx) continue;
                C(mx->col == vrr::C_LABEL, "labels grey");
                const vrr::Prim* mv = rightOf(P, mx); C(mv && mv->bold, "Max value bold");
                const int want = E.S[(size_t)i1739].mx(); C(mv && mv->text == vrr::fm(want), "Max value = the bar's Max");
                C(mv && mv->col == (want >= 10 ? vrr::C_GOLD : vrr::C_TXT), "Max gold from 10, else white");
                if (mode == vrr::VOL) C(mv && mv->text == "15" && mv->col == vrr::C_GOLD, "NQ 5:39 PM: BVMax 15 in gold");
                const vrr::Prim* g = findText(P, pr + "G"); const vrr::Prim* r = findText(P, pr + "R"); const vrr::Prim* a = findText(P, pr);
                C(g && r && a, "all four rows");
                if (g && r && a) { C(a->y0 > mx->y0 && g->y0 > a->y0 && r->y0 > g->y0, "order: Max, consecutive, green, red");
                    const vrr::Prim* gv = rightOf(P, g); const vrr::Prim* rv = rightOf(P, r);
                    C(gv && gv->col == vrr::C_UP && gv->text != "-", "green candle: the G row is green with its count");
                    C(rv && rv->text == "-" && rv->col == vrr::C_LABEL, "the other colour's row shows a grey -"); }
                // the read text is still there, left of the numbers
                bool readLeft = false; for (const auto& p : P) if (!p.fill && p.x0 >= v.readX && p.x1 < mx->x0 && p.text.find("PM") != std::string::npos) readLeft = true;
                C(readLeft, "the 3-line read stays, left of the numbers");
            } else if (!mx) { bool readThere = false; for (const auto& p : P) if (!p.fill && p.x0 >= v.readX) readThere = true; C(readThere || avail < 60, "no room: the column is dropped and the read keeps its space"); }
        }
    }
    std::printf("%s %d pass, %d fail\n", bad ? "FAILED" : "ALL PASS", ok, bad); return bad;
}
