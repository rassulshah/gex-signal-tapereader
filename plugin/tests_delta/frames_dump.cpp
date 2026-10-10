// DP254_frames_dump.cpp -- prints every drawn frame of a bar-by-bar replay (no restart) so 2.5.3 and 2.5.4 can be diffed:
// "nothing is drawn differently". Build twice: -DDELTA_SRC='"<2.5.3 file>"' and the 2.5.4 file.
#ifndef DELTA_SRC
#define DELTA_SRC "../DeltaProfile.cpp"
#endif
#include DELTA_SRC
#include <iostream>
#include <random>
#include <sys/stat.h>
namespace mock { Host* current = NULL; }
int main(int argc, char** argv) {
    setenv("TZ", argc > 2 ? argv[2] : "UTC", 1); tzset();
    const unsigned seeds[] = { 7, 9, 11, 21, 33, 5 };
    for (unsigned seed : seeds) for (int seconds : { 60, 180 }) {
        std::mt19937 rng(seed); std::uniform_int_distribution<int> step(-2, 2), vol(1, 6), pick(0, 9);
        std::vector<mock::Bar> all; double px = 105; const unsigned long start = 1791504000UL + 14 * 3600;
        for (int i = 0; i < 260; ++i) {
            mock::Bar b; b.time = start + static_cast<unsigned long>(i * seconds);
            const double open = px; px = std::max(88.0, std::min(122.0, px + step(rng)));
            b.open = (float)open; b.close = (float)px; b.low = (float)(std::min(open, px) - 1); b.high = (float)(std::max(open, px) + 1); b.volume = 0;
            for (int k = (int)b.low; k <= (int)b.high; ++k) { long by = vol(rng), sl = vol(rng); b.rows.push_back(mock::Price((float)k, by, sl, by + sl)); b.volume += (unsigned long)(by + sl); }
            if (i % 9 == 4) { bool s = pick(rng) < 5; float at = s ? b.low : b.high; long q = 400 + 60 * pick(rng);
                for (mock::Price& r : b.rows) if (r.price == at) { if (s) r.sell += q; else r.buy += q; r.total += q; } b.volume += (unsigned long)q; }
            all.push_back(b);
        }
        if (argc > 1 && argv[1][0]) {                                   // a fresh profile folder per run (2.5.4 writes its record there)
            std::string d = std::string(argv[1]) + "/" + std::to_string(seed) + "_" + std::to_string(seconds); mkdir(d.c_str(), 0700);
            setenv("USERPROFILE", d.c_str(), 1); d += "/InvestorRT"; mkdir(d.c_str(), 0700); d += "/rtx"; mkdir(d.c_str(), 0700); d += "/lsFlexLevels"; mkdir(d.c_str(), 0700);
        }
        mock::Host h; h.seconds = seconds; h.root = "ES"; DeltaProfile p;
        for (std::size_t n = 100; n <= all.size(); ++n) {
            h.bars.assign(all.begin(), all.begin() + (long)n); h.draws.clear(); mock::current = &h; p.draw();
            std::cout << "frame " << seed << ' ' << seconds << ' ' << n << '\n';
            for (const mock::Draw& d : h.draws) std::cout << d.type << ' ' << d.text << ' ' << d.x << ' ' << d.y << ' ' << d.color << '\n';
        }
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
}
