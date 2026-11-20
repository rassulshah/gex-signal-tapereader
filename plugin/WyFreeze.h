// WyFreeze.h  --  (RevRead 0.20.3, 2026-11-19) the wy-file marks of each finished session, frozen.
// Rassul 2026-11-19 ~18:35 CT: after 5 PM the earlier sessions were recomputed with hour levels only, so their key-level marks
// (ES 11-19 9:42 AM Sw PDL V99+ ...) vanished from wy-<MKT>-<spb>.txt. Now each session's lines are kept in
// RevRead\wymarks-<MKT>-<spb>-<YYYYMMDD>.txt (written atomically) while the session is live, and once it is over the file is
// read back unchanged instead of the recompute.
// SDK-free (tests_wyfreeze). File:  WYF|1|<MKT>|<spb>|<sess>|<t0>|<t1>|<n>  then n body lines (as in the wy file)  then END|<n>.
#pragma once
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace wyf {

static const size_t MAX_BYTES = 512 * 1024, MAX_LINES = 20000;

struct Sess { bool ok = false; long long t0 = 0, t1 = 0; std::vector<std::string> lines; };

// the candle stamp a wy body line starts with; false for the header, an "L|" level line or anything else
inline bool stampOf(const std::string& ln, long long& t) {
    size_t i = 0; while (i < ln.size() && ln[i] >= '0' && ln[i] <= '9') ++i;
    if (i == 0 || i > 18 || i >= ln.size() || ln[i] != '|') return false;
    t = std::strtoll(ln.substr(0, i).c_str(), nullptr, 10); return true;
}

inline std::vector<std::string> splitLines(const std::string& txt) {
    std::vector<std::string> v; size_t a = 0;
    while (a < txt.size()) { size_t b = txt.find('\n', a); if (b == std::string::npos) b = txt.size();
        std::string ln = txt.substr(a, b - a); if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (!ln.empty()) v.push_back(ln); a = b + 1; }
    return v;
}

// the body lines whose stamp is in [t0, t1], in their order
inline std::vector<std::string> inRange(const std::vector<std::string>& body, long long t0, long long t1) {
    std::vector<std::string> v; long long t = 0; for (const std::string& ln : body) if (stampOf(ln, t) && t >= t0 && t <= t1) v.push_back(ln); return v;
}

inline std::string fileText(const std::string& mk, long long spb, const std::string& sess, long long t0, long long t1, const std::vector<std::string>& lines) {
    std::string s = "WYF|1|" + mk + "|" + std::to_string(spb) + "|" + sess + "|" + std::to_string(t0) + "|" + std::to_string(t1) + "|" + std::to_string((long long)lines.size()) + "\n";
    for (const std::string& ln : lines) s += ln + "\n";
    return s + "END|" + std::to_string((long long)lines.size()) + "\n";
}

// a torn / foreign / oversized file is not ok (the caller recomputes)
inline Sess parse(const std::string& txt, const std::string& mk, long long spb, const std::string& sess) {
    Sess s; if (txt.size() > MAX_BYTES) return s;
    const std::vector<std::string> v = splitLines(txt); if (v.size() < 2 || v.size() > MAX_LINES + 2) return s;
    std::vector<std::string> h; { size_t a = 0; const std::string& l = v[0]; while (true) { size_t b = l.find('|', a); h.push_back(l.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; } }
    if (h.size() != 8 || h[0] != "WYF" || h[1] != "1" || h[2] != mk || h[3] != std::to_string(spb) || h[4] != sess) return s;
    char* e = nullptr; const long long n = std::strtoll(h[7].c_str(), &e, 10); if (!e || *e || n < 0 || (size_t)n + 2 != v.size()) return s;
    if (v.back() != "END|" + h[7]) return s;
    s.t0 = std::strtoll(h[5].c_str(), nullptr, 10); s.t1 = std::strtoll(h[6].c_str(), nullptr, 10); if (s.t1 < s.t0) return s;
    long long t = 0; for (size_t i = 1; i + 1 < v.size(); ++i) { if (!stampOf(v[i], t) || t < s.t0 || t > s.t1) return Sess(); s.lines.push_back(v[i]); }
    s.ok = true; return s;
}

// a frozen session plus the computed lines past its last candle (a candle that closed after the last write)
inline std::vector<std::string> extend(const Sess& f, const std::vector<std::string>& computedInRange) {
    std::vector<std::string> v = f.lines; long long t = 0;
    for (const std::string& ln : computedInRange) if (stampOf(ln, t) && t > f.t1) v.push_back(ln);
    return v;
}

// the wy body: every computed line outside the replaced ranges, then each replaced session's lines (oldest first)
struct Rep { long long t0 = 0, t1 = 0; std::vector<std::string> lines; };
inline std::vector<std::string> compose(const std::vector<std::string>& body, std::vector<Rep> reps) {
    std::vector<std::string> v; long long t = 0;
    for (const std::string& ln : body) { bool drop = false; if (stampOf(ln, t)) for (const Rep& r : reps) if (t >= r.t0 && t <= r.t1) { drop = true; break; } if (!drop) v.push_back(ln); }
    for (size_t i = 0; i < reps.size(); ++i) for (size_t j = i + 1; j < reps.size(); ++j) if (reps[j].t0 < reps[i].t0) std::swap(reps[i], reps[j]);
    for (const Rep& r : reps) for (const std::string& ln : r.lines) v.push_back(ln);
    return v;
}

// one wy write: the live session (seg 0, the forming candle in the same session) is written as computed; a session that is over is read
// back unchanged (+ candles that closed after its last write; seg 0 only writes those back) and replaces the recompute.
// Returns the wy body lines. get / put: the file read (bounded, "" = none) and the atomic write.
struct Seg { int seg = 0; std::string nm; long long t0 = 0, t1 = 0; };
inline std::vector<std::string> apply(const std::vector<std::string>& body, const std::vector<Seg>& segs, bool liveSame, const std::string& mk, long long spb,
                                      const std::function<std::string(const std::string&)>& pathOf, const std::function<std::string(const std::string&)>& get,
                                      const std::function<void(const std::string&, const std::string&)>& put) {
    std::vector<Rep> reps; std::vector<Seg> ss;   // a long session is read in 2+ segments (600 candles each): one session = one file
    for (const Seg& g : segs) { if (g.nm.empty()) continue; bool in = false;
        for (Seg& x : ss) if (x.nm == g.nm) { x.seg = std::min(x.seg, g.seg); x.t0 = std::min(x.t0, g.t0); x.t1 = std::max(x.t1, g.t1); in = true; }
        if (!in) ss.push_back(g); }
    for (const Seg& g : ss) {
        const std::string fp = pathOf(g.nm); const std::vector<std::string> comp = inRange(body, g.t0, g.t1); const std::string old = get(fp);
        const Sess fz = parse(old, mk, spb, g.nm);
        if (g.seg == 0) { const bool live = liveSame || !fz.ok; const std::vector<std::string> keep = live ? comp : extend(fz, comp);
            const long long a = live ? g.t0 : std::min(g.t0, fz.t0), b = live ? g.t1 : std::max(g.t1, fz.t1);
            const std::string nt = fileText(mk, spb, g.nm, a, b, keep); if (nt != old) put(fp, nt);
            if (!live) { Rep r; r.t0 = a; r.t1 = b; r.lines = keep; reps.push_back(r); } }
        else if (fz.ok) { Rep r; r.t0 = std::min(g.t0, fz.t0); r.t1 = std::max(g.t1, fz.t1); r.lines = extend(fz, comp); reps.push_back(r); } }
    return compose(body, reps);
}

}  // namespace wyf
