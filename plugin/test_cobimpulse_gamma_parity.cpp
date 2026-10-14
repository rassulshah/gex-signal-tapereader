// test_cobimpulse_gamma_parity.cpp -- lsCobImpulse 0.3.0 parity driver: reads random cases written by
// analytics/test_cob_tag_gamma.py and prints the plugin's decision (label, the record's 8 gamma fields, the hover sidecar line and the
// hover rows) so the Python side (lra.cob_tag.gamma_part = lra.cob_gamma.drivers) can be compared line for line.
// g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra test_cobimpulse_gamma_parity.cpp -o tp && ./tp cases.txt > out.txt
//   case line: CASE|market|bull|ext|entry|tick|trigCt|inRth|adr|close15 ('' = none)|rthHi ('' = none)|rthLo|gammaW|gammaEM|gamma|mode|proven|r|v|c|abs
//   then the book text lines, then ENDBOOK (a book of exactly "NOBOOK" = no file)
#include "CobImpulseLogic.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
int main(int argc, char** argv)
{
    if (argc < 2) return 2;
    std::ifstream in(argv[1]); std::string line;
    while (std::getline(in, line)) {
        if (line.compare(0, 5, "CASE|") != 0) continue;
        const std::vector<std::string> f = cimp::splitBar(line);
        if (f.size() != 21) { std::printf("BADCASE\n"); continue; }
        std::string book; std::string bl;
        while (std::getline(in, bl) && bl != "ENDBOOK") book += bl + "\n";
        cimp::GIn g; const std::string mkt = f[1]; g.bull = f[2] == "1";
        g.ext = std::strtod(f[3].c_str(), nullptr); g.entry = std::strtod(f[4].c_str(), nullptr); g.tick = std::strtod(f[5].c_str(), nullptr);
        g.trigCt = std::strtoll(f[6].c_str(), nullptr, 10); g.sessDay = cimp::sessDayOf(g.trigCt); g.inRth = f[7] == "1";
        g.adr = std::strtod(f[8].c_str(), nullptr);
        if (!f[9].empty()) { g.hasClose15 = true; g.close15 = std::strtod(f[9].c_str(), nullptr); }
        if (!f[10].empty()) { g.rthHi = std::strtod(f[10].c_str(), nullptr); g.rthLo = std::strtod(f[11].c_str(), nullptr); }
        cimp::Params P; P.gammaW = std::atoi(f[12].c_str()); P.gammaEM = std::atoi(f[13].c_str()); P.gamma = f[14] == "1";
        cimp::parseMode(f[15], P.mode); P.proven = f[16] == "1";
        const bool r = f[17] == "1", v = f[18] == "1", c = f[19] == "1", abs = f[20] == "1";
        cimp::GBook B; const cimp::GBook* bp = nullptr;
        if (book != "NOBOOK\n") { B = cimp::parseBook(book, mkt); bp = &B; }
        const cimp::GTag G = cimp::gammaTag(g, bp, P);
        cimp::Tag T; T.trig = 0; T.side = g.bull ? 1 : -1;
        std::vector<cimp::Bar> bars(1); bars[0].t = 1;
        const cimp::Rec rec = cimp::toRec(T, bars, &G);
        const std::string rl = cimp::recLine(rec);
        size_t pos = 0; for (int k = 0; k < 15; ++k) pos = rl.find('|', pos) + 1;
        std::printf("BOOKWHY %s\n", bp ? B.why.c_str() : "-");
        std::printf("LABEL %s\n", cimp::label(r, v, abs, P, c, G.code).c_str());
        std::printf("REC %s\n", rl.substr(pos).c_str());
        std::printf("CTX %s\n", G.computed && G.why.empty() ? cimp::ctxLine(cimp::toCtx(G, 1, T.side)).c_str() : "-");
        // the hover rows: as decided, and as rebuilt from the record + its sidecar (both must match, and match Python)
        cimp::Rec rr; const bool okr = cimp::parseRec(rl, rr);
        cimp::GCtx cx; const bool okc = G.computed && G.why.empty() && cimp::parseCtx(cimp::ctxLine(cimp::toCtx(G, 1, T.side)), cx);
        const cimp::GTag G2 = cimp::fromRec(rr, okc ? &cx : nullptr);
        for (int reg = 0; reg < 2; ++reg) {
            const std::vector<std::string> L = cimp::gammaLines(G, g.bull, g.tick, reg == 1), L2 = cimp::gammaLines(G2, g.bull, g.tick, reg == 1);
            std::printf("HOVER%d", reg); for (const auto& s : L) std::printf(" ~%s", s.c_str()); std::printf("\n");
            std::printf("REBUILT%d %s\n", reg, okr && L == L2 ? "same" : "DIFF");
        }
    }
    return 0;
}
