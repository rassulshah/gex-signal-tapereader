// Compile: g++ -std=c++11 -Wall -Wextra -pedantic native/core_test.cpp -o core_test
// Self-test: ./core_test
// CSV driver: ./core_test input.csv events.csv
// Input columns (header optional):
// time,run,eligible,valid,o,h,l,c,v,buy,sell,buyRef,sellRef,totalRef,atrPrior
// The CSV driver uses Config defaults and tick=.25; the self-test demonstrates
// parameter replacement in code for compact engineering fixtures.

#include "ExhaustionLogic.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

ex2::Config compact_config() {
    ex2::Config c;
    c.mode="reversal"; c.n = 3; c.smooth = 1; c.look = 4;
    c.min_leg_atr = .5; c.max_extension_atr = 1.;
    c.rearm_bars = 2; c.confirmation_bars = 3;
    return c;
}

ex2::Bar bar(long long time, int i) {
    ex2::Bar b;
    const double h[] = {100., 101., 102., 103., 104.};
    const double l[] = {98., 99., 100., 101., 102.};
    const double o[] = {99., 100., 101., 102., 103.5};
    const double c[] = {99.5, 100.5, 101.5, 102.5, 103.4};
    const double buy[] = {10., 15., 20., 30., 8.};
    b.time = time; b.run = 7; b.eligible = b.valid = true;
    b.o = o[i]; b.h = h[i]; b.l = l[i]; b.c = c[i]; b.v = 100.;
    b.buy = buy[i]; b.sell = 100. - buy[i];
    b.buyRef = b.sellRef = 10.; b.totalRef = 100.; b.atrPrior = 2.;
    return b;
}

void self_test() {
    ex2::Config bad; bad.look = 1;
    assert(!bad.validate());
    const ex2::Config cfg = compact_config();
    assert(cfg.validate());
    ex2::Engine engine(cfg, .25);
    ex2::Result r;
    for (int i = 0; i < 5; ++i) r = engine.update(bar(1000 + i, i));
    assert(r.status == "WATCH");
    assert(r.events.size() == 1 && r.events[0].kind == "WATCH");
    assert(r.events[0].side == -1);
    assert(std::fabs(r.events[0].stop - 104.25) < 1e-12);
    assert(std::fabs(r.events[0].trigger - 103.0) < 1e-12);

    ex2::Bar confirm = bar(1005, 4);
    confirm.o = 103.4; confirm.h = 103.8; confirm.l = 102.; confirm.c = 102.6;
    confirm.buy = 10.; confirm.sell = 90.;
    r = engine.update(confirm);
    assert(r.status == "CONFIRM");
    assert(r.events.size() == 1 && r.events[0].kind == "CONFIRM");
    assert(r.events[0].id == "W1004:short");

    // A transition closes a still-pending watch rather than discarding it.
    ex2::Engine gap_engine(cfg, .25);
    for (int i = 0; i < 5; ++i) gap_engine.update(bar(2000 + i, i));
    ex2::Bar after_gap = bar(2005, 4); after_gap.run = 8;
    r = gap_engine.update(after_gap);
    assert(r.events.size() == 1 && r.events[0].kind == "INVALIDATE");
    assert(r.events[0].reason == "run_transition");
}

std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> out; std::string item; std::stringstream stream(line);
    while (std::getline(stream, item, ',')) out.push_back(item);
    return out;
}

bool number(const std::string& text, double* value) {
    char* end = 0;
    *value = std::strtod(text.c_str(), &end);
    return end && *end == '\0' && ex2::finite(*value);
}

bool integer(const std::string& text, long long* value) {
    char* end = 0;
    *value = std::strtoll(text.c_str(), &end, 10);
    return end && *end == '\0';
}

bool bool_value(const std::string& text, bool* value) {
    if (text == "1" || text == "true" || text == "TRUE") { *value = true; return true; }
    if (text == "0" || text == "false" || text == "FALSE") { *value = false; return true; }
    return false;
}

bool parse_bar(const std::string& line, ex2::Bar* b) {
    const std::vector<std::string> f = split(line);
    if (f.size() != 16) return false;
    if (!integer(f[0], &b->time) || !integer(f[1], &b->run) || !bool_value(f[2], &b->eligible) || !bool_value(f[3], &b->valid)) return false;
    double* fields[] = {&b->o, &b->h, &b->l, &b->c, &b->v, &b->buy, &b->sell, &b->buyRef, &b->sellRef, &b->totalRef, &b->atrPrior};
    for (int i = 0; i < 11; ++i) if (!number(f[4 + i], fields[i])) return false;
    long long trend=0;if(!integer(f[15],&trend) || trend < -1 || trend > 1)return false;b->trend=(int)trend;
    return true;
}

void csv_driver(const char* input, const char* output) {
    std::ifstream in(input);
    std::ofstream out(output);
    if (!in || !out) throw std::runtime_error("could not open CSV input/output");
    out.precision(17);
    out << "id,side,kind,setup_type,trend,watchTime,time,extreme,stop,trigger,fade,peakNorm,currentNorm,coverage,reason\n";
    ex2::Engine engine(ex2::Config(), .25);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        ex2::Bar b;
        if (!parse_bar(line, &b)) continue;  // permits a header; malformed lines never become evidence
        const ex2::Result r = engine.update(b);
        for (std::size_t i = 0; i < r.events.size(); ++i) {
            const ex2::Event& e = r.events[i];
            out << e.id << ',' << e.side << ',' << e.kind << ',' << e.setup_type << ',' << e.trend << ',' << e.watchTime << ',' << e.time << ','
                << e.extreme << ',' << e.stop << ',' << e.trigger << ',' << e.fade << ',' << e.peakNorm << ','
                << e.currentNorm << ',' << e.coverage << ',' << e.reason << '\n';
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 1) { self_test(); std::cout << "native core tests passed\n"; return 0; }
        if (argc == 3) { csv_driver(argv[1], argv[2]); return 0; }
        std::cerr << "usage: core_test [input.csv output.csv]\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
