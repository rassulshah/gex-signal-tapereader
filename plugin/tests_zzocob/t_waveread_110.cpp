// WaveRead 1.1.0: the plain-English sentences of the Cob Reader wave read
#include "WaveRead.h"
#include <iostream>
static int P = 0, Fl = 0;
#define CHECK(c, m) do { if (c) ++P; else { ++Fl; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
using namespace wvr;
static W mk(int dir, long long vol, double volX, int cnt, bool cap, long long size, double sizeX, bool ab = false, bool cmx = false, bool live = false) { W w; w.dir = dir; w.vol = vol; w.volX = volX; w.cnt = cnt; w.cntCap = cap; w.size = size; w.sizeX = sizeX; w.ab = ab; w.cmx = cmx; w.live = live; return w; }
static std::string all(const std::vector<std::string>& E) { std::string s; for (auto& e : E) s += e + " | "; return s; }
int main() {
    std::vector<W> ws = { mk(1, 596, 0.2, 0, false, 8, 0.3), mk(-1, 19244, 3.1, 9, false, 20, 1.0, false, true), mk(1, 15798, 6.2, 50, true, 60, 3.0), mk(-1, 2191, 0.4, 0, false, 10, 0.5),
                          mk(1, 6161, 2.4, 0, false, 40, 2.0), mk(-1, 6105, 1.0, 1, false, 30, 1.5), mk(1, 28433, 8.5, 50, true, 120, 6.0), mk(-1, 7085, 1.1, 2, false, 25, 1.2, false, false, true) };
    { Out o = read(ws, -1, ""); const std::string s = all(o.ex);
      CHECK(s.find("pullback was light (0.25x") != std::string::npos && s.find("Buyers are putting more volume into the up waves - 3.3x") != std::string::npos && s.find("This up wave was strong (8.5x") != std::string::npos, "the 5:51 AM wave (" << s << ")"); }
    { Out o = read(ws, 1, ""); const std::string s = all(o.ex);
      CHECK(s.find("A climax at the low") != std::string::npos && s.find("little progress - someone is absorbing the selling") != std::string::npos, "the climax wave (" << s << ")"); }
    { Out o = read(ws, 7, ""); const std::string s = all(o.ex);   // the forming down wave, normal size, while up volume builds
      CHECK(s.find("This down wave was normal size (1.1x), so sellers are not pressing.") != std::string::npos, "a normal down wave against up volume building (" << s << ")"); }
    { std::vector<W> q = { mk(1, 500, 0.9, 0, false, 10, 0.9), mk(-1, 300, 0.8, 0, false, 9, 0.9), mk(1, 520, 1.0, 0, false, 11, 1.0, false, false, true) };
      Out t = read(q, -1, ""); CHECK(t.ex.size() == 2 && t.ex[1] == "Nothing unusual in this wave.", "quiet: size + nothing unusual (" << all(t.ex) << ")"); }
    for (size_t k = 0; k < ws.size(); ++k) { Out o = read(ws, (int)k, ""); bool ok = !o.ex.empty(); for (auto& e : o.ex) if (e.empty() || e.back() != '.') ok = false; CHECK(ok, "wave " << k << ": sentences end with a full stop (" << all(o.ex) << ")"); }
    std::cout << "t_waveread_110: " << P << " passed, " << Fl << " failed\n"; return Fl ? 1 : 0;
}
