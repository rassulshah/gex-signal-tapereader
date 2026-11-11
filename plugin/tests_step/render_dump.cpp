// render_dump.cpp -- lsStepTrend: the plugin's own draw list on the mock host for one fixture day, as text for render.py (the render check).
// Build: g++ -std=c++17 -Wno-delete-non-virtual-dtor -I.. -Imock render_dump.cpp -o rd && ./rd fx_mockup.txt 0 > draws.txt
#include "StepTrend.cpp"
namespace mock { Host* current = nullptr; Feed feed; }
int main(int argc, char** argv) {
    if (argc < 3) return 1; std::ifstream f(argv[1]); const int want = std::atoi(argv[2]); std::string tag;
    while (f >> tag) { if (tag != "D") break; int i, n; std::string m; double tick; f >> i >> m >> tick >> n;
        mock::Host h; mock::current = &h; h.tick = (float)tick; h.seconds = 180; h.root = m; h.symbol = m + "Z6"; h.barW = 7; double lo = 1e18, hi = -1e18;
        for (int k = 0; k < n; ++k) { long long t; double o, hh, l, c; f >> t >> o >> hh >> l >> c; mock::Bar b; b.time = (unsigned long)(1794268800LL + t + 180); b.open = (float)o; b.high = (float)hh; b.low = (float)l; b.close = (float)c; h.bars.push_back(b); lo = std::min(lo, l); hi = std::max(hi, hh); }
        if (i != want) continue;
        h.priceBase = (float)lo; h.pxPerPrice = (float)(560.0 / (hi - lo)); h.paneBottom = 600; h.paneRight = 50 + n * 7 + 30; h.visFrom = 0; h.visTo = n - 1;
        StepTrend* p = static_cast<StepTrend*>(CreateExtension()); p->setup(); p->calc(0); p->draw();
        std::printf("W %d %d %d\n", h.paneRight, h.paneBottom, h.barW);
        for (int k = 0; k < n; ++k) { const mock::Bar& b = h.bars[(size_t)k]; auto y = [&](float v) { return h.paneBottom - (v - h.priceBase) * h.pxPerPrice; }; std::printf("C %d %.1f %.1f %.1f %.1f\n", 50 + k * 7, y(b.open), y(b.high), y(b.low), y(b.close)); }
        for (const mock::Draw& d : h.draws) std::printf("%s %d %d %d %d %06X %d %d\n", d.type == "line" ? "L" : "O", d.r, d.b, d.x, d.y, (unsigned)d.pen, d.penW, d.pat);
        p->destroy(); delete p; return 0; }
    return 1;
}
