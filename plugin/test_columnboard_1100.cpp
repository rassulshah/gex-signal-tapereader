// test_columnboard_1100.cpp - ColumnBoard 1.10.0 (2026-11-04, audit "key level badges pinned against the HOD/LOD table", Rassul "just build it"):
//  - stack(): the 3rd (forward) pass - no badge above 'top', none under 'bottom', none overlapping (fuzzed, 20,000 random scenes);
//  - planKeys(): the band decided first, from his live layouts (status files 2026-11-04: HG table 533-590, ES 479-634, CL 329-502,
//    NG 255-428, each stretched over the Key Levels column): a line behind the table / title / marker row = an edge marker candidate,
//    never a badge; every badge within 2 badge heights of its line; nothing in the table rows; no static pinning while price / scale move.
// g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra -Werror -I. test_columnboard_1100.cpp -o t && ./t
#include "ColumnBoard.h"
#include <cstdio>
#include <random>
static int P = 0, F = 0;
static void ok(bool c, const std::string& m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m.c_str()); } }

struct Geo { const char* mkt; int paneT, paneB, tblT, tblB; };
static const int BH = 16;   // badge text height (bh); a badge is bh + 1 tall, a marker row bh + 2

// the DealerProfile 2.52.3 band rule: under the title row, minus the table side it covers
static void band0(const Geo& g, int& t0, int& b0)
{
    t0 = g.paneT + ((g.paneB - g.paneT) >= 200 ? 22 : 2); b0 = g.paneB - 4;
    if (g.tblT < (g.paneT + g.paneB) / 2) t0 = std::max(t0, g.tblB + 4); else b0 = std::min(b0, g.tblT - 4);
}
// every invariant of one plan
static bool planClean(const Geo& g, const std::vector<cb::Item>& v, const cb::KeyPlan& kp, int cap, std::string& why)
{
    for (size_t i = 0; i < v.size(); ++i) {
        if (kp.st[i] != cb::K_SHOWN) { if (v[i].shown) { why = "a non-shown state drawn"; return false; } continue; }
        if (!v[i].shown) { why = "shown state not drawn"; return false; }
        const int t = v[i].y - v[i].h / 2, b = t + v[i].h;
        if (t < kp.top || b > kp.bot) { why = "badge outside the band " + std::to_string(t) + "-" + std::to_string(b); return false; }
        if (b > g.tblT && t < g.tblB) { why = "badge in the HOD/LOD table rows"; return false; }
        if (std::abs(v[i].y - v[i].want) > cap) { why = "pushed " + std::to_string(std::abs(v[i].y - v[i].want)) + " px > cap"; return false; }
        if (kp.up && t < kp.upY + BH / 2 + 1) { why = "badge over the top marker row"; return false; }
        if (kp.dn && b > kp.dnY - BH / 2 - 1) { why = "badge over the bottom marker row"; return false; }
        for (size_t j = i + 1; j < v.size(); ++j) if (kp.st[j] == cb::K_SHOWN) {
            const int t2 = v[j].y - v[j].h / 2, b2 = t2 + v[j].h; if (t < b2 && t2 < b) { why = "overlap"; return false; } }
        if (kp.outside[i]) { why = "outside but shown"; return false; }
    }
    for (size_t i = 0; i < v.size(); ++i) {
        if (kp.outside[i] && v[i].want >= kp.top && v[i].want <= kp.bot) { why = "inside line marked outside"; return false; }
        if (!kp.outside[i] && (v[i].want < kp.top || v[i].want > kp.bot)) { why = "outside line not marked"; return false; }
        if (kp.st[i] == cb::K_UP && !kp.up) { why = "up candidate without an up marker row"; return false; }
        if (kp.st[i] == cb::K_DN && !kp.dn) { why = "down candidate without a down marker row"; return false; }
    }
    if (kp.up && kp.upY - BH / 2 < g.paneT + 2) { why = "top marker in the title row"; return false; }
    if (kp.dn && kp.dnY + BH / 2 > g.tblT && kp.dnY - BH / 2 < g.tblB) { why = "bottom marker in the table"; return false; }
    return true;
}

int main()
{
    ok(std::string(cb::VERSION) == "1.10.0", "version 1.10.0");
    // ---- stack(): the 3rd pass (fuzz) -----------------------------------------------------------------------------------------------
    {   std::mt19937 rng(11); int bad = 0, total = 0;
        for (int k = 0; k < 20000; ++k) {
            const int top = (int)(rng() % 200), bottom = top + 30 + (int)(rng() % 600), n = 1 + (int)(rng() % 25);
            std::vector<cb::Item> v(n);
            for (cb::Item& it : v) { it.h = 9 + (int)(rng() % 12); it.want = top - 60 + (int)(rng() % (bottom - top + 120)); }
            cb::stack(v, top, bottom, top + (int)(rng() % (bottom - top)));
            for (size_t i = 0; i < v.size(); ++i) { if (!v[i].shown) continue; ++total;
                const int t = v[i].y - v[i].h / 2, b = t + v[i].h;
                if (t < top || b > bottom) ++bad;
                for (size_t j = i + 1; j < v.size(); ++j) if (v[j].shown) { const int t2 = v[j].y - v[j].h / 2; if (t < t2 + v[j].h && t2 < b) ++bad; } }
        }
        ok(bad == 0 && total > 50000, "stack 3rd pass: 20,000 random scenes, " + std::to_string(total) + " badges, 0 outside / overlapping (" + std::to_string(bad) + ")");
        // a full band, everything wanting the bottom edge (the pull-up must not run over top; the 2-pass stack also passed this - the drop step keeps the total inside the band - so the 3rd pass is a guard)
        std::vector<cb::Item> v(6); for (cb::Item& it : v) { it.h = 15; it.want = 99; }
        cb::stack(v, 0, 96, 50); bool in = true; for (const cb::Item& it : v) if (it.shown && (it.y - 7 < 0 || it.y - 7 + 15 > 96)) in = false;
        ok(in, "6 badges of 15 px in a 96 px band, all at the bottom: every badge inside");
    }
    const int cap = cb::KEY_MAX_PUSH * (BH + 1), rowH = BH + 2;
    const Geo geos[] = { { "HG", 0, 596, 533, 590 }, { "ES", 0, 640, 479, 634 }, { "CL", 0, 508, 329, 502 }, { "NG", 0, 434, 255, 428 }, { "TOP", 0, 600, 30, 180 } };
    for (const Geo& g : geos) {
        int t0 = 0, b0 = 0; band0(g, t0, b0); const std::string M = g.mkt;
        // (a) his audit case: a level whose line is BEHIND the table (HG PFL 6.3180 drawn ~25 px off) -> the bottom edge marker, not a badge
        {   const int behind = g.tblT < (g.paneT + g.paneB) / 2 ? (g.tblT + g.tblB) / 2 : (g.tblT + g.tblB) / 2;
            std::vector<cb::Item> v(4); v[0].want = behind; v[1].want = (t0 + b0) / 2; v[2].want = (t0 + b0) / 2 + 60; v[3].want = (t0 + b0) / 2 - 80;
            for (cb::Item& it : v) it.h = BH + 1;
            const cb::KeyPlan kp = cb::planKeys(v, t0, b0, rowH, false, false, 0, (t0 + b0) / 2, cap);
            std::string why; const bool clean = planClean(g, v, kp, cap, why);
            const bool isUp = g.tblT < (g.paneT + g.paneB) / 2;
            ok(clean, M + " (a) plan clean (" + why + ")");
            ok(kp.outside[0] && kp.st[0] == (isUp ? cb::K_UP : cb::K_DN) && (isUp ? kp.up : kp.dn), M + " (a) the level behind the table is the " + (isUp ? "top" : "bottom") + " edge marker, not a badge");
            ok(kp.st[1] == cb::K_SHOWN && v[1].y == v[1].want && v[2].y == v[2].want && v[3].y == v[3].want, M + " (a) the other badges sit exactly at their price");
        }
        // (b) NO STATIC PINNING: 12 levels spread over the pane, the chart scrolls / rescales (offset -300..+300, scale 0.6..1.6)
        {   int fails = 0, moving = 0, frames = 0; std::string firstWhy; std::vector<int> prevY; std::vector<int> prevW;
            for (int sc = 0; sc < 6; ++sc) for (int d = -300; d <= 300; d += 7) {
                const double s = 0.6 + sc * 0.2; std::vector<cb::Item> v(12);
                for (int i = 0; i < 12; ++i) { v[i].h = BH + 1; v[i].want = (int)std::lround((g.paneB / 2) + (i - 6) * 41 * s + d + (i % 3) * 5); }
                const cb::KeyPlan kp = cb::planKeys(v, t0, b0, rowH, false, false, 0, g.paneB / 2 + d, cap);
                std::string why; if (!planClean(g, v, kp, cap, why)) { if (fails++ == 0) firstWhy = why; }
                // a badge whose line moved moves with it (within the cap of its line - never stuck at one y while its line slides past)
                if (!prevY.empty() && sc == 0) for (int i = 0; i < 12; ++i) if (kp.st[i] == cb::K_SHOWN && prevY[i] != -1 && v[i].want != prevW[i] && v[i].y != prevY[i]) ++moving;
                prevY.assign(12, -1); prevW.assign(12, 0); for (int i = 0; i < 12; ++i) { prevW[i] = v[i].want; if (kp.st[i] == cb::K_SHOWN) prevY[i] = v[i].y; }
                ++frames;
            }
            ok(fails == 0, M + " (b) " + std::to_string(frames) + " scroll / scale frames: no overlap, nothing in the table, every badge within 2 badge heights (" + firstWhy + ")");
            ok(moving > 100, M + " (b) badges follow their lines as the chart scrolls (" + std::to_string(moving) + " moves)");
        }
        // (c) a tight cluster at the band's table edge: some are capped -> that edge's marker; the rest within the cap
        {   const bool isUp = g.tblT < (g.paneT + g.paneB) / 2; std::vector<cb::Item> v(7);
            for (int i = 0; i < 7; ++i) { v[i].h = BH + 1; v[i].want = isUp ? t0 + rowH + 4 + i : b0 - rowH - 4 - i; }   // just inside the band once the marker row is reserved
            const cb::KeyPlan kp = cb::planKeys(v, t0, b0, rowH, false, false, 0, (t0 + b0) / 2, cap);
            std::string why; ok(planClean(g, v, kp, cap, why), M + " (c) edge cluster clean (" + why + ")");
            int edge = 0, shown = 0; for (size_t i = 0; i < v.size(); ++i) { if (kp.st[i] == (isUp ? cb::K_UP : cb::K_DN)) ++edge; if (kp.st[i] == cb::K_SHOWN) ++shown; }
            ok(edge > 0 && shown >= 2 && kp.capped + kp.out >= edge, M + " (c) 7 levels in 6 px at the edge: " + std::to_string(shown) + " badges, " + std::to_string(edge) + " to the edge marker");
            // the same cluster right AT the edge: reserving the marker row puts their lines behind it -> all are that marker's candidates (it names the nearest)
            for (int i = 0; i < 7; ++i) v[i].want = isUp ? t0 + 2 + i : b0 - 2 - i;
            const cb::KeyPlan k2 = cb::planKeys(v, t0, b0, rowH, false, false, 0, (t0 + b0) / 2, cap);
            int e2 = 0; for (int s2 : k2.st) if (s2 == (isUp ? cb::K_UP : cb::K_DN)) ++e2;
            ok(planClean(g, v, k2, cap, why) && e2 == 7, M + " (c) cluster on the band edge: all 7 go to the edge marker row (" + why + ")");
        }
        // (d) a cluster in the middle: past the cap = dropped (counted), never an edge marker
        {   std::vector<cb::Item> v(9); const int mid = (t0 + b0) / 2; for (int i = 0; i < 9; ++i) { v[i].h = BH + 1; v[i].want = mid + i; }
            const cb::KeyPlan kp = cb::planKeys(v, t0, b0, rowH, false, false, 0, mid, cap);
            std::string why; ok(planClean(g, v, kp, cap, why), M + " (d) mid cluster clean (" + why + ")");
            int drop = 0, edge = 0; for (int s : kp.st) { if (s == cb::K_DROP) ++drop; if (s == cb::K_UP || s == cb::K_DN) ++edge; }
            ok(drop > 0 && edge == 0 && drop == kp.dropped, M + " (d) 9 levels in 9 px mid-pane: " + std::to_string(drop) + " dropped (counted), none sent to an edge");
        }
        // (e) nothing outside and no off-screen level: no marker rows, the band = band0
        {   std::vector<cb::Item> v(2); v[0].want = t0 + 40; v[1].want = b0 - 40; for (cb::Item& it : v) it.h = BH + 1;
            const cb::KeyPlan kp = cb::planKeys(v, t0, b0, rowH, false, false, 0, (t0 + b0) / 2, cap);
            ok(!kp.up && !kp.dn && kp.top == t0 && kp.bot == b0 && kp.st[0] == cb::K_SHOWN && kp.st[1] == cb::K_SHOWN, M + " (e) no markers: band unchanged");
            const cb::KeyPlan k2 = cb::planKeys(v, t0, b0, rowH, true, true, 1, (t0 + b0) / 2, cap);
            ok(k2.up && k2.dn && k2.wkY > 0 && k2.top == t0 + 2 * rowH && k2.bot == b0 - rowH && k2.wkY > k2.upY, M + " (e) off-screen up / down + weekly up: three rows reserved, weekly under the up marker");
        }
        // (f) deterministic (no flicker)
        {   std::vector<cb::Item> a(10), b; for (int i = 0; i < 10; ++i) { a[i].h = BH + 1; a[i].want = t0 - 30 + i * 37; } b = a;
            const cb::KeyPlan p1 = cb::planKeys(a, t0, b0, rowH, false, true, 0, 200, cap), p2 = cb::planKeys(b, t0, b0, rowH, false, true, 0, 200, cap);
            bool same = p1.st == p2.st; for (int i = 0; i < 10; ++i) if (a[i].y != b[i].y) same = false;
            ok(same, M + " (f) same input, same plan");
        }
    }
    std::printf("ColumnBoard %s: %d passed, %d failed\n", cb::VERSION, P, F);
    return F ? 1 : 0;
}
