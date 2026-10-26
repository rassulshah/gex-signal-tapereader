// replay_oln.cpp -- OrderLinesLogic on a recorded MBO session (binary dump: t_ct_ms, act, side, pt, sz, oid) + 3-min bars csv.
// g++ -std=c++17 -O2 -I.. replay_oln.cpp -o replay && ./replay es1022.bin es1022_bars.csv ES
#include "../OrderLinesLogic.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#pragma pack(push,1)
struct Ev { long long t; int a, s; long long pt, sz, oid; };
#pragma pack(pop)
int main(int argc, char** argv) {
    std::ifstream f(argv[1], std::ios::binary); f.seekg(0, std::ios::end); size_t n = (size_t)f.tellg() / sizeof(Ev); f.seekg(0);
    std::vector<Ev> ev(n); f.read((char*)ev.data(), (std::streamsize)(n * sizeof(Ev)));
    std::stable_sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) { return a.t < b.t; });
    std::vector<oln::Bar> bars; { std::ifstream b(argv[2]); std::string ln; while (std::getline(b, ln)) { auto v = oln::split(ln, ','); oln::Bar x; x.endMs = std::atoll(v[0].c_str()); x.o = std::atoll(v[1].c_str()); x.h = std::atoll(v[2].c_str()); x.l = std::atoll(v[3].c_str()); x.c = std::atoll(v[4].c_str()); bars.push_back(x); } }
    // bar end times are CT ms since epoch day boundaries computed as k*180000 of CT ms -> same clock as events
    oln::Engine E; E.market = argv[3]; E.R = oln::ruleFor(E.market);
    size_t bi = 0;
    size_t ix = 0;
    for (const Ev& e : ev) {
        if (++ix % 250000 == 0) std::fprintf(stderr, "%zu %s lines %zu book %zu wait %zu rolls %zu ladders %zu recent %zu\n", ix, oln::clock12(e.t).c_str(), E.lines.size(), E.book.size(), E.waiting.size(), E.rolls.size(), E.ladders.size(), E.recentNew.size());
        while (bi < bars.size() && bars[bi].endMs <= e.t) { E.closeBar(bars[bi]); ++bi; }
        E.onEvent(e.a, e.s == 0 ? oln::BID : e.s == 1 ? oln::ASK : -1, e.pt, e.sz, e.oid, e.t);
    }
    while (bi < bars.size()) E.closeBar(bars[bi++]);
    int shown = 0;
    for (const auto& L : E.lines) {
        if (!E.visible(L)) continue; ++shown;
        std::printf("%s %-10s %s -> %s  %s px %.2f->%.2f segs %zu end %s %s word %s%s\n", L.side ? "OFF" : "BID", L.label().c_str(), oln::clock12(L.t0).c_str(),
            L.end ? oln::clock12(L.endMs).c_str() : "live", L.kind ? "LAD" : "ONE", L.segs.front().px * 0.25, L.px() * 0.25, L.segs.size(), L.end == 1 ? "FILLED" : L.end == 2 ? "PULLED" : "LIVE",
            L.fillMs ? oln::clock12(L.fillMs).c_str() : "", (std::string(oln::wordText(L.word)) + "@" + (L.wordMs ? oln::clock12(L.wordMs) : std::string("-"))).c_str(), L.word ? (L.wordShown ? " shown" : " hidden") : "");
    }
    const auto& T = E.tally;
    std::printf("lines %lld drawn %d filled %lld pulled %lld rolls %lld ambig %lld moves %lld ladders %lld ice %lld held %lld thru %lld hidden %lld late %lld unknownFill %lld rows %zu\n",
        T.lines, shown, T.filled, T.pulled, T.rolls, T.rollAmbig, T.moves, T.ladders, T.icebergs, T.held, T.thru, T.wordsHidden, T.lateSkipped, T.unknownFill, E.rows.size());
}
