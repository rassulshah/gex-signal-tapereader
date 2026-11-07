// MarketKey.h 1.0.0 (2026-11-06 19:10 CT, Rassul "these are from rithmic but the backfill is from iqfeed", "fix it") - ONE market lookup for
// every indicator: a chart's root symbol in any of his feeds' styles -> the market key the rules use (ES NQ CL NG GC HG EU).
//   IQFeed  @ES @NQ QCL QNG QGC QHG @EU (his GC / HG / CL charts report QGC / QHG / QCL)   Rithmic  ES NQ CL NG GC HG 6E
//   CQG     EP ENQ CLE NGE GCE CPE EU6                                                     contract-style roots (NGX6, GCZ6 ...) -> the first 2 letters
// Unknown roots come back unchanged.
#pragma once
#include <string>
namespace mkey {
inline std::string norm(const std::string& rootIn) {
    std::string r = rootIn; while (!r.empty() && (r[0] == '@' || r[0] == '_')) r.erase(0, 1);
    static const char* const K[] = { "ES", "NQ", "CL", "NG", "GC", "HG", "EU" };
    static const char* const P[][2] = { {"EP", "ES"}, {"ENQ", "NQ"}, {"CLE", "CL"}, {"NGE", "NG"}, {"GCE", "GC"}, {"CPE", "HG"}, {"EU6", "EU"}, {"6E", "EU"},
                                       {"QCL", "CL"}, {"QNG", "NG"}, {"QGC", "GC"}, {"QHG", "HG"}, {"QES", "ES"}, {"QNQ", "NQ"}, {"QEU", "EU"} };
    for (const auto& p : P) if (r == p[0]) return p[1];
    for (const char* k : K) if (r == k) return k;
    if (r.size() >= 3) {                                    // contract-style roots: NGX6, GCZ6, 6EZ6
        if (r.compare(0, 2, "6E") == 0) return "EU";
        for (const char* k : K) if (r.compare(0, 2, k) == 0 && (r[2] >= 'F' && r[2] <= 'Z')) return k;
    }
    return rootIn;
}
}
