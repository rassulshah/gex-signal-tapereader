// profile_check.cpp -- parses a Python-written LRA-MarketProfile-<MKT>.csv with the C++ reader (MarketProfile.h) and prints what it got:
// ./profile_check <file> <MKT>  ->  "<loaded 0|1> <slots> <kn> <news> <aggr_p99 or ?> <note>"
#include <cstdio>
#include <fstream>
#include <sstream>
#include "MarketProfile.h"
#include "Memory.h"
int main(int argc, char** argv) {
    if (argc < 3) return 2;
    std::ifstream f(argv[1], std::ios::binary); std::stringstream s; s << f.rdbuf();
    if (argc > 3 && std::string(argv[3]) == "mem") {             // ./profile_check <memory file> <MKT> mem -> "<loaded> <levels> <story days> <half>"
        const apmem::Mem M = apmem::parse(s.str(), argv[2]);
        std::printf("%d %d %d %.3g %s\n", M.loaded ? 1 : 0, (int)M.levels.size(), (int)M.story.size(), M.half, M.note.c_str());
        return 0;
    }
    const apm::Profile P = apm::parse(s.str(), argv[2]);
    std::printf("%d %d %d %d %s %s\n", P.loaded ? 1 : 0, (int)P.slots.size(), (int)P.kn.size(), (int)P.news.size(),
                P.has("aggr_p99") ? std::to_string(P.get("aggr_p99", 0)).c_str() : "?", P.note.c_str());
    return 0;
}
