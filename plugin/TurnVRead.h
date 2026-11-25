// TurnVRead.h -- what lsTurn tells the Volume read about each closed candle (lsFlexLevels\TurnPane\vread-<MKT>-<spb>.txt). SDK-free, shared by
// Turn.cpp (writes) and VolRead.cpp (reads). 1.0.0, 2026-11-24 (Rassul 22:23-22:39 CT: the Volume read's title = the overall call; How big = how many
// large orders; more than one candle; sliced; pulled - "everything else is a go" on reads-conclude-mockup.html v30).
//   header  VRD|1|<MKT>|<spb>|<lsTurn version>
//   line    V|stamp|kind|end|lmS|lmB|llB|llS|llBbeyond|llSbeyond|slS|slSn|slB|slBn|pullB|pullS|pullBn|pullSn
//     kind  1 Initiative, 2 passive defense won, 3 aggressor won, 0 no push, -9 not known; end +1 the bottom, -1 the top
//     lm    large market orders (sellers, buyers; -1 = not known); ll large new resting orders (bids, offers) and how many sat beyond the candle;
//     sl    sliced: the order size and how many (sellers, buyers; 0 = none); pull lots pulled near price (bids, offers) and the normal (mean of the 20 before; -1 = not known)
#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstdlib>

namespace tvr {
static const char* const VERSION = "1.0.0";
static const size_t KEEP = 900;
struct Row { int kind = -9, end = 0; long long lm[2] = { -1, -1 }, ll[2] = { -1, -1 }, llB[2] = { 0, 0 }, sl[2] = { 0, 0 }, slN[2] = { 0, 0 }, pull[2] = { -1, -1 }, pullN[2] = { -1, -1 }; };
inline std::string fileText(const std::map<long long, Row>& m, const std::string& mkt, int spb, const std::string& ver) {
    std::string t = "VRD|1|" + mkt + "|" + std::to_string(spb) + "|" + ver + "\n"; size_t skip = m.size() > KEEP ? m.size() - KEEP : 0;
    for (const auto& kv : m) { if (skip) { --skip; continue; } const Row& r = kv.second;
        const long long v[] = { r.kind, r.end, r.lm[0], r.lm[1], r.ll[0], r.ll[1], r.llB[0], r.llB[1], r.sl[0], r.slN[0], r.sl[1], r.slN[1], r.pull[0], r.pull[1], r.pullN[0], r.pullN[1] };
        t += "V|" + std::to_string(kv.first); for (long long x : v) t += "|" + std::to_string(x); t += "\n"; }
    return t;
}
inline bool toLLs(const std::string& s, long long& v) { if (s.empty() || s.size() > 20) return false; char* e = nullptr; v = std::strtoll(s.c_str(), &e, 10); return e && *e == 0; }
inline bool parse(const std::string& txt, const std::string& mkt, int spb, std::map<long long, Row>& out) {
    out.clear(); std::vector<std::string> L; { std::string cur; for (char c : txt) { if (c == '\n') { L.push_back(cur); cur.clear(); } else if (c != '\r') cur += c; } }   // an unterminated last line is dropped (a torn write)
    if (L.empty()) return false;
    auto split = [](const std::string& s) { std::vector<std::string> v; std::string cur; for (char c : s) { if (c == '|') { v.push_back(cur); cur.clear(); } else cur += c; } v.push_back(cur); return v; };
    const std::vector<std::string> h = split(L[0]); long long sp = 0; if (h.size() < 4 || h[0] != "VRD" || h[1] != "1" || h[2] != mkt || !toLLs(h[3], sp) || sp != spb) return false;
    for (size_t i = 1; i < L.size(); ++i) { const std::vector<std::string> c = split(L[i]); if (c.size() != 18 || c[0] != "V") continue; long long v[17]; bool ok = true;
        for (int k = 0; k < 17; ++k) if (!toLLs(c[(size_t)k + 1], v[k])) { ok = false; break; }
        if (!ok || v[0] <= 0 || v[1] < -9 || v[1] > 3 || v[2] < -1 || v[2] > 1) continue;
        Row r; r.kind = (int)v[1]; r.end = (int)v[2]; r.lm[0] = v[3]; r.lm[1] = v[4]; r.ll[0] = v[5]; r.ll[1] = v[6]; r.llB[0] = v[7]; r.llB[1] = v[8]; r.sl[0] = v[9]; r.slN[0] = v[10]; r.sl[1] = v[11]; r.slN[1] = v[12];
        r.pull[0] = v[13]; r.pull[1] = v[14]; r.pullN[0] = v[15]; r.pullN[1] = v[16]; out[v[0]] = r; }
    return true;
}
} // namespace tvr
