// test_dealerread_459.cpp -- lsDealerRead 4.5.9 (CR4800): the place file "pos,dx,dy" (old one-digit files still load), garbage = Bottom
// left with no offset, and the offset clamped inside the area (BoxPlace.h, tested in full by test_panel_logic). Same unit as the 4.5.8 test.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include "DealerRead.cpp"
static int failures = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++failures; std::printf("  FAIL  %s (line %d)\n", m, __LINE__); } } while (0)
int main()
{
    char tmpl[] = "/tmp/dr9XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    auto put = [&](const std::string& m, const std::string& t) { std::ofstream f(dl::placePath("DealerRead", m).c_str()); f << t; };
    CHECK(drPlace("ES").pos == 6 && drPlace("ES").dx == 0, "no file: Bottom left, no offset");
    put("ES", "2\n"); CHECK(drPlace("ES").pos == 2 && drPlace("ES").dx == 0, "an old 4.5.8 file (position only)");
    put("ES", "4,-45,12\n"); { const bp::Place p = drPlace("ES"); CHECK(p.pos == 4 && p.dx == -45 && p.dy == 12, "pos,dx,dy"); }
    put("ES", "4,9999,0\n"); { const bp::Place p = drPlace("ES"); CHECK(p.pos == 4 && p.dx == 0, "an out-of-range offset: ignored, position kept"); }
    put("ES", "garbage\n"); CHECK(drPlace("ES").pos == 6, "garbage: Bottom left");
    CHECK(drPlace("").pos == 6, "no market: Bottom left");
    int ax = 0, ay = 0; dl::anchorXY(0, 16, 700, 500, 300, 120, 8, 8, ax, ay);
    const bp::Box o = bp::offset(bp::mk(ax, ay, ax + 300, ay + 120), 600, 600, bp::mk(0, 16, 700, 500));
    CHECK(o.r == 700 && o.b == 500, "Bottom right + (600, 600): stays inside (never past the profiles' edge / the pane bottom)");
    std::printf("dealerread 4.5.9: %d passed, %d failed\n", passes, failures); return failures ? 1 : 0;
}
