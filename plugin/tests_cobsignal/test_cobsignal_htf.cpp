// test_cobsignal_htf.cpp - lsCobSignal 1.7.0: the 3-hour S / R sweep (cobs::Htf, checkWaveHtf, htfLayout). Logic only (SDK-free).
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_cobsignal_htf.cpp -o tf && ./tf cobsignal_es.csv
#include "CobSignalLogic.h"
#include <fstream>
#include <iostream>
#include <sstream>
static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
static bool eq(double a, double b) { return std::fabs(a - b) < 1e-9; }
// one 3-min bar per 3-hour bucket is enough to drive the 3-hour walk: bucket n starts at 17:00 + 3 h * n (local secs, day 20000 = a weekday is not needed here)
static cobs::Bar hb(int n, double h, double l, double c) { cobs::Bar b; b.t = 20000LL * 86400 + 17 * 3600 + (long long)n * 10800 + 180; b.o = c; b.h = h; b.l = l; b.c = c; b.v = 1; return b; }
int main(int argc, char** argv) {
    {   // pivots: up to 110 (bar 1), down to 95 (bar 3), up to 108 (bar 5), down to 97 (bar 7), then up
        cobs::Htf H; cobs::HSnap s;
        const double X[][3] = { {100, 98, 99}, {110, 100, 109}, {108, 99, 100}, {101, 95, 96}, {104, 96, 103}, {108, 100, 107}, {106, 99, 100}, {102, 97, 98}, {105, 98, 104} };
        for (int i = 0; i < 9; ++i) s = H.add(hb(i, X[i][0], X[i][1], X[i][2]), 180);
        CHECK(eq(s.r1, 108) && eq(s.r2, 110), "R1 = last pivot high 108, R2 = higher of the last two 110 (" << s.r1 << ", " << s.r2 << ")");
        CHECK(eq(s.s1, 97) && eq(s.s2, 95), "S1 = last pivot low 97, S2 = lower of the last two 95 (" << s.s1 << ", " << s.s2 << ")");
        CHECK(s.dir == 1 && eq(s.ext, 105), "the forming swing is up, its extreme = the forming bar's high");
        CHECK(s.r1ok && s.r2ok && s.s1ok && s.s2ok, "all four standing");
        // the forming bar counts: a later 3-min bar of the SAME bucket dips under the previous bar's low without a new high -> pivot high confirmed live
        cobs::Bar b = hb(9, 104, 103, 103.5); s = H.add(b, 180); CHECK(s.dir == 1 && eq(s.r1, 108), "bar 9 opens inside: nothing changes");
        b.t += 180; b.h = 103.5; b.l = 96.5; b.c = 97.5; s = H.add(b, 180);
        CHECK(s.dir == -1 && eq(s.r1, 105) && eq(s.r2, 108) && eq(s.ext, 96.5), "the forming bar turned the swing down: R1 = 105 live, extreme 96.5 (" << s.r1 << ")");
        CHECK(eq(s.s1, 97) && s.s1ok, "S1 97 undercut inside the forming bar is still standing (no 3-hour close below)");
        // a completed 3-hour close below S1 = broken
        s = H.add(hb(10, 98, 94, 94.5), 180); s = H.add(hb(11, 95, 93, 94), 180);
        CHECK(eq(s.s1, 97) && !s.s1ok, "a completed 3-hour close under S1 breaks it (" << s.s1 << ")");
        // outside bar keeps the extreme (IRT): new low AND higher high than the bar before in a down swing -> stays down
        s = H.add(hb(12, 99, 92, 98), 180); CHECK(s.dir == -1 && eq(s.ext, 92), "an outside bar with a new low keeps the down swing (no split inside the bar)");
        cobs::Htf N; CHECK(!std::isfinite(N.add(hb(0, 1, 0, 1), 10800).r1) && N.HB.empty(), "no 3-hour levels on a 3-hour (or longer) chart");
        cobs::Htf M; CHECK(M.add(hb(0, 1, 0, 1), 420).dir == 0 && M.HB.empty(), "nor on a bar size that does not divide 3 hours");
    }
    if (argc < 2) { std::cerr << "fixture path needed\n"; return 2; }
    std::vector<cobs::Bar> B; { std::ifstream f(argv[1]); std::string l; std::getline(f, l);
        while (std::getline(f, l)) { cobs::Bar b; if (std::sscanf(l.c_str(), "%lld,%lf,%lf,%lf,%lf,%lf", &b.t, &b.o, &b.h, &b.l, &b.c, &b.v) == 6) B.push_back(b); } }
    CHECK(B.size() > 5000, "fixture loaded (" << B.size() << ")");
    cobs::Engine E; E.setMarket("ES", 0.25, 180); E.keySweep = false;   /* (1.9.0) this suite checks the 3-hour sweep itself; key levels OR 3-hour: cmp_190.cpp */
    for (const cobs::Bar& b : B) E.addBar(b);
    CHECK(E.hs.size() == E.B.size(), "one 3-hour snapshot per bar");
    CHECK(!E.marks.empty(), "the fixture has 3-hour sweep CoBs (" << E.marks.size() << ")");
    {   bool lv = true, win = true, geo = true, rules = true, night = false;
        for (const cobs::Mark& m : E.marks) {
            lv = lv && (m.side > 0 ? (m.levels == "S1" || m.levels == "S2" || m.levels == "S1/S2") : (m.levels == "R1" || m.levels == "R2" || m.levels == "R1/R2"));
            const int m0 = (int)((E.B[(size_t)m.legStart].t - 180) % 86400) / 60, mj = (int)(m.t % 86400) / 60; win = win && (m0 >= 1020 || m0 + 3 <= 660) && (mj - 3 >= 1020 || mj <= 660); night = night || m0 >= 1020 || m0 < 420;
            const cobs::HSnap& h = E.hs[(size_t)m.legStart]; const double o = m.ext, e = m.side > 0 ? E.B[(size_t)m.legEnd].h : E.B[(size_t)m.legEnd].l;
            const bool a = m.side > 0 ? (h.s1ok && o < h.s1 && h.s1 < e) : (h.r1ok && e < h.r1 && h.r1 < o), b = m.side > 0 ? (h.s2ok && o < h.s2 && h.s2 < e) : (h.r2ok && e < h.r2 && h.r2 < o);
            geo = geo && (a || b) && h.dir == -m.side && std::fabs(h.ext - o) < 1e-9;
            rules = rules && m.dratio >= 1.9 && m.legEnd - m.legStart >= 3 && m.show;
        }
        CHECK(lv, "every CoB names S1 / S2 (bull) or R1 / R2 (bear) only"); CHECK(win, "(1.7.1) origin and confirming bar between 5:00 PM and 11:00 AM CT"); CHECK(night, "(1.7.1) overnight CoBs are found"); CHECK(cobs::htfInWindow(1020, 1023) && cobs::htfInWindow(0, 3) && cobs::htfInWindow(657, 660) && !cobs::htfInWindow(660, 663) && !cobs::htfInWindow(1017, 1020), "window edges: 5:00 PM in, 11:00 AM in, 11:00-11:03 out, 4:57 PM out");
        CHECK(geo, "origin = the 3-hour swing's extreme beyond a standing level, the wave back through it"); CHECK(rules, "dratio >= 1.9, >= 3 bars, all shown"); }
    {   cobs::Engine O; O.setMarket("ES", 0.25, 180); O.htfSweep = false; for (const cobs::Bar& b : B) O.addBar(b);
        bool named = !O.marks.empty(); for (const cobs::Mark& m : O.marks) named = named && m.levels.find("S1") == std::string::npos && m.levels.find("R1") == std::string::npos;
        CHECK(named, "htfSweep off = the 1.6.x named key levels (" << O.marks.size() << ")"); }
    {   // no repaint: feeding bar by bar gives the same snapshots and marks as they stood (a prefix run = the full run's prefix)
        const size_t cut = B.size() * 2 / 3; cobs::Engine P; P.setMarket("ES", 0.25, 180); P.keySweep = false; for (size_t i = 0; i < cut; ++i) P.addBar(B[i]);
        bool same = true; for (size_t i = 0; i < cut; ++i) { const cobs::HSnap& a = P.hs[i]; const cobs::HSnap& b = E.hs[i]; same = same && ((eq(a.r1, b.r1) || (std::isnan(a.r1) && std::isnan(b.r1))) && (eq(a.s2, b.s2) || (std::isnan(a.s2) && std::isnan(b.s2))) && a.dir == b.dir && a.s1ok == b.s1ok && a.r2ok == b.r2ok); }
        CHECK(same, "3-hour levels never repaint (prefix run = the full run)");
        bool mk = true; size_t n = 0; for (const cobs::Mark& m : E.marks) if (m.idx < (int)cut) { ++n; bool f = false; for (const cobs::Mark& q : P.marks) f = f || (q.t == m.t && q.side == m.side && q.levels == m.levels); mk = mk && f; }
        CHECK(mk && n == P.marks.size(), "CoBs never repaint (" << n << " in the prefix)"); }
    {   cobs::View v; v.l = 0; v.t = 0; v.r = 1400; v.b = 800; v.last = (int)E.B.size() - 1; v.first = v.last - 300;
        double lo = 1e300, hi = -1e300; for (int i = v.first; i <= v.last; ++i) { lo = std::min(lo, E.B[(size_t)i].l); hi = std::max(hi, E.B[(size_t)i].h); }
        const int f0 = v.first; v.X = [f0](int j) { return 10.0 + (j - f0) * 4.0; }; v.Y = [lo, hi](double p) { return 780.0 - (p - lo) / (hi - lo) * 760.0; };
        const std::vector<cobs::Prim> P = cobs::htfLayout(E, v); int lines = 0, names = 0; bool col = true, in = true; std::string nm;
        // (1.11.1, Rassul 2026-11-22 19:41 CT "R should be red and S should be green", 20:22 "A") R1 / R2 red, S1 / S2 green: every piece is one of the
        // two, a name has its line's colour (the name follows its line) and the colour its letter says; no resistance line under a support line
        unsigned lastLine = 0; int red = 0, green = 0, yRedMax = -1000000, yGreenMin = 1000000; bool byName = true;
        for (const cobs::Prim& p : P) { col = col && (p.col == cobs::C_HTF_R || p.col == cobs::C_HTF_S);
            if (p.line) { ++lines; lastLine = p.col; if (p.col == cobs::C_HTF_R) { ++red; yRedMax = std::max(yRedMax, p.y0); } else { ++green; yGreenMin = std::min(yGreenMin, p.y0); } in = in && p.x1 > p.x0 && p.y0 >= v.t && p.y0 <= v.b; }
            else { ++names; nm += p.text + " "; byName = byName && !p.text.empty() && p.col == lastLine && p.col == (p.text[0] == 'R' ? cobs::C_HTF_R : cobs::C_HTF_S); in = in && p.x0 >= v.l && p.x1 <= v.r && p.tiny; } }
        CHECK(lines > 0 && col && in, "3-hour lines drawn in red / green inside the pane (" << lines << " pieces)");
        CHECK(byName && names > 0, "each name in its line's colour: R red, S green (" << nm << ")");
        CHECK(red + green == lines && (red == 0 || green == 0 || yRedMax < yGreenMin), "resistance lines (red " << red << ") sit above support lines (green " << green << ")");
        CHECK(cobs::C_HTF_R == 0x00EF4444u && cobs::C_HTF_S == 0x0022C55Eu && cobs::C_HTF_R == cobs::C_BEAR && cobs::C_HTF_S == cobs::C_BULL, "his standard red / green");
        CHECK(names <= 4 && nm.find("Cob") == std::string::npos, "at most one name per line, at its newest piece (" << nm << ")");
        cobs::View w = v; w.first = 5; w.last = 2; CHECK(cobs::htfLayout(E, w).empty(), "an empty view draws nothing"); }
    {   int nh = 0; bool okh = true; for (const std::string& l : E.out) if (l.rfind("HTF,", 0) == 0) { ++nh; bool f = false; for (const cobs::Mark& m : E.marks) f = f || (l.find("HTF," + std::to_string(m.t) + (m.side > 0 ? ",BULL," : ",BEAR,") + m.levels.substr(0, 2) + ",") == 0); okh = okh && f; }
        CHECK(nh == (int)E.marks.size() && okh, "one HTF record line per CoB with its level's name and price (" << nh << ")");
        const std::string st = cobs::statusText(E, "ESZ6", 180, "x"); CHECK(st.find("\nHTFNOW,") != std::string::npos && st.find("\nHTFX,") != std::string::npos && st.find(",HOD,") != std::string::npos && st.find(",LOD,") != std::string::npos, "status: HTFNOW + the session's HOD / LOD lines for the MQ Reader"); }
    {   // (1.8.0) the CoB pullback Rev: up to 3 per CoB on separate pullbacks
        bool cap = true, rule = true, own = true, sep = true; int n = 0, multi = 0;
        for (const cobs::RevMark& r : E.revs) { ++n; const cobs::Bar& b = E.B[(size_t)r.idx]; const cobs::Bar& p = E.B[(size_t)r.idx - 1]; const cobs::Mark* cm = nullptr; int cnt = 0;
            for (const cobs::Mark& m : E.marks) { if (m.t == r.cobT && m.side == r.side) cm = &m; }
            for (const cobs::RevMark& q : E.revs) { if (q.cobT == r.cobT && q.side == r.side) { ++cnt; if (&q != &r && q.nth == r.nth) sep = false; if (q.nth == r.nth + 1) sep = sep && q.topIdx > r.idx; } }
            cap = cap && cnt <= cobs::REV_MAX && r.nth >= 1 && r.nth <= cobs::REV_MAX; if (r.nth == 2) ++multi;
            own = own && cm && cm->show && cm->zOk && r.idx >= cm->idx && r.idx > cm->legEnd && r.topIdx >= cm->legEnd && r.topIdx < r.idx;
            rule = rule && r.ratio <= cobs::REV_VOL_X + 1e-12 && (r.side > 0 ? (b.c > p.h && b.c > b.o) : (b.c < p.l && b.c < b.o));
            if (cm) { double wv = 0, pv = 0; for (int i = cm->legStart + 1; i <= cm->legEnd; ++i) wv += E.B[(size_t)i].v;
                const double top = r.side > 0 ? E.B[(size_t)r.topIdx].h : E.B[(size_t)r.topIdx].l;
                for (int i = r.topIdx + 1; i < r.idx; ++i) { pv += E.B[(size_t)i].v; const cobs::Bar& x = E.B[(size_t)i]; rule = rule && (r.side > 0 ? x.h <= top : x.l >= top); }
                for (int i = cm->legEnd + 1; i <= r.idx; ++i) { const cobs::Bar& x = E.B[(size_t)i]; rule = rule && (r.side > 0 ? x.c >= cm->ext : x.c <= cm->ext); }
                rule = rule && (r.side > 0 ? b.h <= top : b.l >= top) && std::fabs(pv / wv - r.ratio) < 1e-9; } }
        const bool one = cap && sep;
        int drawnCobs = 0; for (const cobs::Mark& m : E.marks) if (m.show && m.zOk) ++drawnCobs;
        CHECK(n > 0 && n <= 3 * drawnCobs, "CoB pullback Revs found (" << n << " after " << drawnCobs << " drawn CoBs)"); CHECK(one, "up to 3 Revs per CoB, each on its own pullback (a new extreme between them)"); CHECK(multi >= 0, "second Revs: " << multi); CHECK(own, "each Rev belongs to a drawn CoB, at / after its bar and after the wave's end pivot");
        CHECK(rule, "Rev rule: closes beyond the prior bar and its own open, pullback volume <= 0.5x the wave's, no close through the origin, inside the running extreme");
        int lines = 0; for (const std::string& l : E.out) if (l.rfind("REV,", 0) == 0) ++lines; CHECK(lines == n, "one REV record line per Rev");
        {   const size_t cut = B.size() * 2 / 3; cobs::Engine P; P.setMarket("ES", 0.25, 180); P.keySweep = false; for (size_t i = 0; i < cut; ++i) P.addBar(B[i]); bool pre = true; size_t k = 0; for (const cobs::RevMark& r : E.revs) if (r.idx < (int)cut) { ++k; bool f = false; for (const cobs::RevMark& q : P.revs) f = f || (q.t == r.t && q.side == r.side && q.cobT == r.cobT); pre = pre && f; } CHECK(pre && k == P.revs.size(), "Revs never repaint (prefix run)"); }
        cobs::View v; v.l = 0; v.t = 0; v.r = 100000; v.b = 100000; v.first = 0; v.last = (int)E.B.size() - 1; v.X = [](int j) { return 10.0 + j * 4.0; }; v.Y = [](double p) { return 50000.0 - p * 4.0; };
        int lab = 0; bool side = true, badge = true; for (const cobs::Prim& p : cobs::layout(E, v)) if (p.text == "Rev") { ++lab; side = side && (p.col == cobs::C_BULL || p.col == cobs::C_BEAR); badge = badge && p.badge && p.flag == 7; } CHECK(lab == n && side, "a Rev label per Rev, green / red (" << lab << ")");
        CHECK(badge, "(1.10.0) every Rev is a badge posted with flag 7 (one Rev per candle with lsRevReadMarks)");
        { int cb = 0; bool ok = true; for (const cobs::Prim& p : cobs::layout(E, v)) if (p.text.rfind("Cob", 0) == 0) { ++cb; ok = ok && p.badge && !p.text2.empty() && std::isdigit((unsigned char)p.text2[0]); } CHECK(cb > 0 && ok, "(1.10.0) every Cob is a badge with its ZZO count on line 2 (" << cb << ")"); }
        const std::string st = cobs::statusText(E, "ESZ6", 180, "x"); CHECK(st.find("\nREV_RULES,1.8.0") != std::string::npos && st.find("\nREVS," + std::to_string(n) + "\n") != std::string::npos, "status: Rev rule + count"); }
    {   const std::string st = cobs::statusText(E, "ESZ6", 180, "x"); CHECK(st.rfind(std::string("VERSION,") + cobs::VERSION + "\n", 0) == 0 /* (1.11.1) version-agnostic: was pinned to "VERSION,1.11.0" and failed on the bump */ && st.find("\nHTF_RULES,1.7.1") != std::string::npos && st.find("\nHTF,R1=") != std::string::npos, "status: version + 3-hour rules + levels"); }
    {   // (1.8.1, independent review) every drawn CoB starts its pullback look exactly once (revOn); finished looks are pruned without a second
        // walk: no (bar, CoB) has two Rev marks; the 3-hour lines never draw a broken level
        std::map<std::pair<long long, long long>, int> seen; int dup = 0; for (const cobs::RevMark& r : E.revs) if (++seen[std::make_pair(r.t, r.cobT)] > 1) ++dup;
        int started = 0, drawnM = 0; for (const cobs::Mark& m : E.marks) { if (m.show && m.zOk) ++drawnM; if (m.revOn) ++started; }
        CHECK(dup == 0, "no duplicate Rev (" << E.revs.size() << " Revs)"); CHECK(started == drawnM, "every drawn CoB's look started once: " << started << " of " << drawnM); CHECK(E.revTr.size() <= 65, "open tracks bounded: " << E.revTr.size());
        cobs::View v; v.l = 0; v.t = 0; v.r = 1000000; v.b = 1000000; v.first = 0; v.last = (int)E.B.size() - 1; v.X = [](int j) { return (double)j * 3; }; v.Y = [](double p) { return 1000000.0 - p * 100; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; };
        // (1.8.2) ONE line per standing level at the last bar on screen, from the pane's left edge; scrolled back = the levels as they stood at that bar
        int broken = 0, lines = 0, views = 0, notLeft = 0;
        for (int last = 400; last < (int)E.B.size(); last += 97) { v.last = last; ++views; const cobs::HSnap& h = E.hs[(size_t)last];
            for (const cobs::Prim& p : cobs::htfLayout(E, v)) { if (!p.line) continue; ++lines; if (p.x0 != v.l) ++notLeft; const double px = (1000000.0 - p.y0) / 100.0;
                const bool ok = (h.r1ok && std::fabs(h.r1 - px) < 0.011) || (h.r2ok && std::fabs(h.r2 - px) < 0.011) || (h.s1ok && std::fabs(h.s1 - px) < 0.011) || (h.s2ok && std::fabs(h.s2 - px) < 0.011); if (!ok) ++broken; } }
        CHECK(lines > 50 && lines <= 4 * views && broken == 0 && notLeft == 0, "3-hour lines " << lines << " over " << views << " views (at most 4 each), on a broken level " << broken << ", not from the left edge " << notLeft); }
    std::cout << "test_cobsignal_htf: " << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
