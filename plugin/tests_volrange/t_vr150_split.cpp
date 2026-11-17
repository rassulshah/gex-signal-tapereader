// lsVolRead / lsRangeRead 1.5.0: the split read - verdict + sentences left, numbers right; nothing overlaps, everything inside the pane
#include "VolRangeLogic.h"
#include <iostream>
static int P = 0, Fl = 0;
#define CHECK(c, m) do { if (c) ++P; else { ++Fl; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
static bool isHead(const std::string& t) { return t.rfind("Bullish", 0) == 0 || t.rfind("Bearish", 0) == 0 || t.rfind("Mixed", 0) == 0 || t.rfind("Quiet", 0) == 0; }
int main() {
    // the wrap helper
    auto tw = [](const std::string& s) { return (int)s.size() * 6; };
    { std::vector<std::string> L = spt::wrap({ "one two three four five six seven eight nine ten", "short one." }, 60, 5, tw);
      bool ok = true; for (auto& l : L) if (tw(l) > 60) ok = false; CHECK(ok && L.size() >= 3, "wrap: every line within the width"); }
    { std::vector<std::string> L = spt::wrap({ "aaaa bbbb cccc dddd eeee ffff gggg hhhh", "second sentence." }, 30, 2, tw);
      CHECK(L.size() == 2 && L.back().size() >= 3 && L.back().substr(L.back().size() - 3) == "..." && tw(L.back()) <= 30, "wrap: the first sentence is cut with ... when it does not fit"); }
    { std::vector<std::string> L = spt::wrap({ "first fits.", "this second sentence needs two lines here" }, 72, 2, tw);
      CHECK(L.size() == 1 && L[0] == "first fits.", "wrap: a later sentence that does not fit whole is left out"); }
    { spt::Cols c = spt::cols(700, 200, 150); CHECK(c.dataOn && c.leftW == spt::LEFT_MAX, "cols: room for both - left at its max"); }
    { spt::Cols c = spt::cols(300, 200, 150); CHECK(!c.dataOn && c.leftW == 300, "cols: no room for both - the read alone"); }
    { spt::Cols c = spt::cols(100, 200, 150); CHECK(c.leftW == 0, "cols: the verdict does not fit - nothing"); }
    { spt::Cols c = spt::cols(600, 200, 360); CHECK(c.dataOn && c.leftW == 360, "cols: a long verdict line widens the left column"); }
    { spt::Cols c = spt::cols(500, 200, 360); CHECK(!c.dataOn && c.leftW >= 360, "cols: a long verdict line is never wrapped - the numbers give way"); }
    for (int mode = 0; mode < 2; ++mode) {
        vrr::Engine E; E.mode = mode; E.tick = 0.25; E.spb = 180; E.hlOn = true;
        for (int i = 0; i < 60; ++i) { vrr::Bar b; b.t = 3877700000LL + i * 180; b.o = 7400 + i * 0.5; b.c = b.o + ((i % 3) ? 0.75 : -0.5); b.h = std::max(b.o, b.c) + 0.5; b.l = std::min(b.o, b.c) - 0.25; b.v = 500 + (i % 7) * 60 + (i == 59 ? 4000 : 0); E.addBar(b, 20774LL * 86400 + 5 * 3600 + i * 180); }
        vrd::Inside q; q.ok = true; q.bigBuy = 100; q.bigSell = 20; q.swBuy = 6; q.swSell = 1; q.hidSell = 300; E.inside[E.T.back()] = q;
        vrr::View v; v.l = 0; v.t = 0; v.r = 1600; v.b = 110; v.bw = 8; v.first = 0; v.last = 60; v.X = [](int j) { return 20.0 + j * 10.0; }; v.textW = tw;
        vrr::Bar form; form.t = 1; form.o = 7430; form.h = 7431; form.l = 7429.5; form.c = 7430.5; form.v = 90;
        const char* nm = mode ? "Range" : "Volume";
        for (int pick : { -1, 10, 59, 60, 0, 500 }) {
            v.readX = 642; const std::vector<vrr::Prim> Pm = vrr::layout(E, v, &form, pick);
            bool head = false, inside = true, cand = false, oldBox = false; int sentences = 0, dataX = 1 << 30, leftR = 0;
            for (const vrr::Prim& p : Pm) { if (p.fill) continue; if (p.x1 > 1300 || p.x0 < 642) continue;   // the bars / marks are left of 642
                if (isHead(p.text) && p.x0 == 642) head = true;
                if (p.text == "Candle:") cand = true;
                if (p.text == "BVMax" || p.text == "BRMax") oldBox = true;
                if (p.x0 == 642 && !isHead(p.text)) { ++sentences; leftR = std::max(leftR, p.x1); }
                if (p.text == (mode ? "Range" : "Volume") && p.x0 > 642) dataX = std::min(dataX, p.x0);
                if (p.x0 < v.l || p.x1 > v.r || p.y0 < v.t || p.y1 > v.b) inside = false; }
            CHECK(head && !cand && !oldBox && inside, nm << " pick " << pick << ": the verdict line is drawn, no old Candle: line or box, inside the pane");
            CHECK(sentences >= 1, nm << " pick " << pick << ": at least one sentence under the verdict (" << sentences << ")");
            CHECK(leftR <= 642 + spt::LEFT_MAX, nm << " pick " << pick << ": the sentences stay inside the left column (" << leftR << ")");
            CHECK(dataX < (1 << 30) && dataX >= 642 + spt::LEFT_MAX + spt::GAP - 1, nm << " pick " << pick << ": the numbers sit right of the left column (" << dataX << ")");
        }
        { const vrd::Out o = mode ? vrd::Out() : vrd::Out(); (void)o; }
        // the 4,000-lot candle with a 100-lot buy sweep and iceberg offers: the sentences say so in words
        { vrd::Ctx x; for (int i = 0; i < 25; ++i) { vrd::Cd k; k.o = 100 + i * 0.1; k.c = k.o + (i % 2 ? 0.2 : -0.1); k.h = std::max(k.o, k.c) + 0.1; k.l = std::min(k.o, k.c) - 0.1; k.vol = 500; x.c.push_back(k); }
          vrd::Cd big; big.o = 102.6; big.h = 104; big.l = 102.4; big.c = 102.5; big.vol = 3000; x.c.push_back(big); x.in = q; x.cntAll = 40;
          const vrd::Out o = mode ? vrd::rangeRead(x) : vrd::volumeRead(x); std::string all; for (auto& s : o.ex) all += s + " ";
          if (!mode) CHECK(all.find("100-lot buy order swept 6 offer levels") != std::string::npos && all.find("Hidden sellers") != std::string::npos && all.find("Heavy volume") != std::string::npos, "Volume sentences: the sweep, the hidden sellers, the size (" << all << ")");
          else CHECK(all.find("new high") != std::string::npos && all.find("closed near its low") != std::string::npos, "Range sentences: the rejected high and the shape (" << all << ")");
          bool dot = true; for (auto& s : o.ex) if (s.empty() || (s.back() != '.' && s.back() != '?')) dot = false; CHECK(dot, nm << ": every sentence ends with a full stop"); }
        v.readX = 1300; { const std::vector<vrr::Prim> Pm = vrr::layout(E, v, &form, -1); bool head = false, num = false; for (const vrr::Prim& p : Pm) { if (!p.fill && isHead(p.text) && p.x0 == 1300) head = true; if (!p.fill && p.x0 > 1300 && (p.text == "Volume" || p.text == "Range")) num = true; }
          CHECK(head && !num, nm << ": 300 px of room - the read alone, no numbers"); }
        v.readX = 1560; { const std::vector<vrr::Prim> Pm = vrr::layout(E, v, &form, -1); bool head = false; for (const vrr::Prim& p : Pm) if (!p.fill && isHead(p.text)) head = true; CHECK(!head, nm << ": no room right of the last candle - no read"); }
        v.readX = 642; v.b = 20; { const std::vector<vrr::Prim> Pm = vrr::layout(E, v, &form, -1); int lines = 0; bool inside = true; for (const vrr::Prim& p : Pm) { if (p.fill || p.x0 < 642) continue; if (p.x0 == 642) ++lines; if (p.y1 > v.b) inside = false; } CHECK(lines <= 1 && inside, nm << ": a very low pane keeps only the verdict (" << lines << ")"); }
    }
    std::cout << "t_vr150_split: " << P << " passed, " << Fl << " failed\n"; return Fl ? 1 : 0;
}
