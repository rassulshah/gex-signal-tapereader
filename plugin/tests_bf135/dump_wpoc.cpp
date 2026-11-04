// dump_wpoc.cpp -- writes the fixture's WavePOC record (as the engine appends it) + bars + the C++ wpocLines per closed bar, for the
// Python parity check of lra.book_level_combo 1.3.0 wpoc_lines (parity_wpoc.py)
#include "../WavePOCLogic.h"
#include "../BookLevelBox.h"
#include <fstream>
#include <iostream>
int main(int, char** argv) {
    std::ifstream f(argv[1]); std::string line; std::vector<wpc::Bar> B; std::vector<std::vector<wpc::VRow> > V;
    while (std::getline(f, line)) { if (line.empty() || line[0] == '#') continue; auto x = wpc::splitCsv(line);
        if (x[0] == "B") { wpc::Bar b; b.t = atoll(x[2].c_str()); wpc::toTicks(atof(x[3].c_str()), .25, b.o); wpc::toTicks(atof(x[4].c_str()), .25, b.h); wpc::toTicks(atof(x[5].c_str()), .25, b.l); wpc::toTicks(atof(x[6].c_str()), .25, b.c); B.push_back(b); V.resize(B.size()); }
        else { wpc::VRow r; wpc::toTicks(atof(x[2].c_str()), .25, r.tick); r.vol = atoll(x[3].c_str()); r.dlt = atoll(x[4].c_str()); V[(size_t)atoi(x[1].c_str())].push_back(r); } }
    wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = V[(size_t)i]; return 1; };
    wpc::Store S; wpc::Engine E; E.store = &S; blb::WpFeed F; std::vector<blb::WpBar> bars;
    std::ofstream rec(std::string(argv[2]) + "/WavePOC/ES-180-nodes.csv"), bo(std::string(argv[2]) + "/bars.csv"), lo(std::string(argv[2]) + "/cpp_lines.csv");
    rec << wpc::REC_HEAD << "\n";
    for (size_t i = 0; i < B.size(); ++i) {
        E.addBar(B[i], vap); for (auto& ln : E.out) { rec << ln << "\n"; F.line(ln, .25); } E.out.clear();
        blb::WpBar b; b.t = B[i].t; b.h = B[i].h; b.l = B[i].l; b.c = B[i].c; bars.push_back(b);
        bo << b.t << "," << b.h * .25 << "," << b.l * .25 << "," << b.c * .25 << "\n";
        auto L = blb::wpocLines(F.waves(), bars, F.nNewest); std::vector<std::string> s; for (auto& l : L) s.push_back(std::to_string(l.tick) + (l.open ? "o" : "t"));
        std::sort(s.begin(), s.end()); lo << b.t; for (auto& x : s) lo << "," << x; lo << "\n";
    }
}
