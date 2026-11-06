// render_dump.cpp -- lsBreakout 1.2.0 (BO / BD / Rly / Pb): dumps the mock chart (ES Oct 30 RTH) and every drawing call for render_check.py (a PNG of what
// the plugin draws). g++ -std=c++17 -IBreakout_test_support -I.. render_dump.cpp -o rd && ./rd > dump.txt && python3 render_check.py dump.txt
#include "irtsdk.h"
#include <fstream>
namespace mock { Host* current = nullptr; }
#include "Breakout.cpp"
int main() {
    std::ifstream in("es_2026-10-28_30_3m.txt"); std::string l; mock::Host H; mock::current = &H; H.root = "ES"; H.sym = "ESZ26";
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        H.t.push_back((RTDATE)(svl::daysFromCivil(y, mo, d) * 86400LL + h * 3600 + mi * 60 + s)); H.f[0].push_back((float)o); H.f[1].push_back((float)hi); H.f[2].push_back((float)lo); H.f[3].push_back((float)c); H.vol.push_back((unsigned long)vo); }
    const int n = (int)H.t.size(); int first = 0; for (int i = 0; i < n; ++i) if (H.t[(size_t)i] % 86400 == 8 * 3600 + 33 * 60 && i > 900) { first = i; break; }
    float lo = 1e9f, hi = -1e9f; for (int i = first; i < n; ++i) { lo = std::min(lo, H.f[2][(size_t)i]); hi = std::max(hi, H.f[1][(size_t)i]); }
    H.lo = lo - 6; H.hi = hi + 6; H.first = first; H.paneR = 10 * (n - first) + 40; H.paneB = 900;
    Breakout* x = static_cast<Breakout*>(CreateExtension()); x->setup(); x->calc(0); x->draw();
    std::printf("PANE %d %d %d %d\n", H.paneL, H.paneT, H.paneR, H.paneB);
    for (int i = first; i < n; ++i) std::printf("BAR %d %d %d %d %d %d\n", mockX(i), mockY(H.f[0][(size_t)i]), mockY(H.f[1][(size_t)i]), mockY(H.f[2][(size_t)i]), mockY(H.f[3][(size_t)i]), (int)(H.t[(size_t)i] % 86400 / 60));
    for (const auto& p : H.drawn) { if (p.k == 'L') std::printf("LINE %d %d %d %d %lx\n", p.x0, p.y0, p.x1, p.y1, p.col);   // (1.9.0) the Rev lines
        else std::printf("TXT %d %d %d %d %lx %d %s\n", p.x0, p.y0, p.x1, p.y1, p.col, p.font, p.text.c_str()); }
    x->destroy(); delete x; return 0;
}
