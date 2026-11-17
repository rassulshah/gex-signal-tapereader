// MktMove.h  --  the market strength line of the HOD / LOD box (lsHodLod 2.29.0). SDK-free.
// (1.0.0, 2026-11-16 21:45-21:52 CT Rassul "i want to add a line on top of the hod lod box , which indicates how much each market is up or down
// for the session ... im trying to identify strong trending moving markets", "all markets that i trade", "lets go with session"; mockup
// claude.ai/artifact/TmYk67SB3emJaALWR4PbpJ)
//   move  = (last close - the session's open, 5:00 PM CT) / the market's average session range (last 10 complete sessions, at least 5)
//   pos   = where the last close sits in the session's range so far (0 = the low, 1 = the high)
//   line  = every market whose chart wrote its move in the last 10 minutes, biggest |move| first: "ES -80%"
//           grey under 0.25x, else green up / red down; BOLD + arrow = 0.7x or more AND still in the outer quarter of the range (trending)
// Each chart writes lsFlexLevels\HodLod.move-<MKT>.txt ("MOVE,1,<MKT>,<move>,<pos>,<epoch>") and reads the others'.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace mkm {

static const char* const VERSION = "1.1.0";   // (1.1.0, Rassul 2026-11-17 05:55-05:58 CT "in the line, on the right show which markets have a cob like the mq reader trade now section", "build"; mockup claude.ai/artifact/F2X8n9DNVXPFFh9ZZbUfUN) + each market's live Cob
static const int SESS_START = 17 * 3600;          // 5:00 PM CT: a bar ENDING after it belongs to the next day's session
static const int ADR_N = 10, ADR_MIN = 5;
static const long long SESS_MIN_SEC = 6 * 3600;   // a session shorter than this (first loaded day, holiday stub) never feeds the average
static const double STRONG = 0.7, FLAT = 0.25, EDGE = 0.25;
static const long long STALE_SEC = 600;

struct Bar { long long absEnd; double o, h, l, c; };
struct Stat { bool ok = false; double open = 0, last = 0, hi = 0, lo = 0, adr = 0, ratio = 0, pos = 0.5; long long sid = 0; int sessions = 0; };

inline long long sidOf(long long absEnd) { long long d = absEnd / 86400; if (absEnd < 0 && d * 86400 != absEnd) --d; const long long sod = absEnd - d * 86400; return d + (sod > SESS_START ? 1 : 0); }

inline Stat compute(const std::vector<Bar>& B)
{
    Stat s; if (B.empty()) return s;
    struct Ses { long long sid, t0, t1; double o, h, l, c; };
    std::vector<Ses> S;
    for (const Bar& b : B) {
        if (!std::isfinite(b.o) || !std::isfinite(b.h) || !std::isfinite(b.l) || !std::isfinite(b.c) || !(b.h >= b.l) || !(b.l > 0)) continue;
        const long long id = sidOf(b.absEnd);
        if (S.empty() || S.back().sid != id) { if (!S.empty() && id < S.back().sid) continue; S.push_back(Ses{ id, b.absEnd, b.absEnd, b.o, b.h, b.l, b.c }); }
        else { Ses& x = S.back(); x.t1 = b.absEnd; x.h = (std::max)(x.h, b.h); x.l = (std::min)(x.l, b.l); x.c = b.c; }
    }
    if (S.empty()) return s;
    const Ses& cur = S.back();
    double sum = 0; int n = 0;
    for (int i = (int)S.size() - 2; i >= 1 && n < ADR_N; --i) {   // S[0] = the first loaded session, usually cut: never used
        if (S[(size_t)i].t1 - S[(size_t)i].t0 < SESS_MIN_SEC) continue;
        const double r = S[(size_t)i].h - S[(size_t)i].l; if (!(r > 0)) continue;
        sum += r; ++n;
    }
    s.sessions = n; s.sid = cur.sid; s.open = cur.o; s.last = cur.c; s.hi = cur.h; s.lo = cur.l;
    if (n < ADR_MIN) return s;
    s.adr = sum / n; if (!(s.adr > 0)) return s;
    s.ratio = (cur.c - cur.o) / s.adr;
    s.pos = cur.h > cur.l ? (cur.c - cur.l) / (cur.h - cur.l) : 0.5;
    s.ok = std::isfinite(s.ratio) && std::isfinite(s.pos);
    return s;
}

// ---- (1.1.0) the market's live Cob: the latest drawn (SHOW) Cob of the current session from lsCobSignal's CobSignal-<MKT>-<secs>-live.csv
// ("COB,<bar end>,<BULL|BEAR>,<label>,<levels>,<dratio>,<origin price>,<adr>,<leg start>,<leg end>,<rN>,<vN>,<SHOW|LOG>"), alive until a later
// bar CLOSES beyond its origin (bull: below the origin low; bear: above the origin high). A failed latest Cob = no Cob.
struct Cob { int dir = 0; long long t = 0; double origin = 0; };
inline std::vector<Cob> parseCobs(const std::string& text)
{
    std::vector<Cob> out; size_t a = 0;
    while (a < text.size() && out.size() < 5000) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        std::string l = text.substr(a, e - a); a = e + 1; if (!l.empty() && l.back() == '\r') l.pop_back();
        if (l.compare(0, 4, "COB,") != 0) continue;
        std::vector<std::string> f; size_t b = 0; for (;;) { const size_t c = l.find(',', b); f.push_back(l.substr(b, c == std::string::npos ? std::string::npos : c - b)); if (c == std::string::npos) break; b = c + 1; }
        if (f.size() != 13 || f[12] != "SHOW") continue;
        char* end = nullptr; const long long t = std::strtoll(f[1].c_str(), &end, 10); if (!end || *end != 0 || t <= 0) continue;
        end = nullptr; const double o = std::strtod(f[6].c_str(), &end); if (!end || *end != 0 || !std::isfinite(o) || !(o > 0)) continue;
        const int d = f[2] == "BULL" ? 1 : f[2] == "BEAR" ? -1 : 0; if (!d) continue;
        Cob c; c.dir = d; c.t = t; c.origin = o; out.push_back(c);
    }
    return out;
}
inline Cob liveCob(const std::vector<Cob>& cobs, const std::vector<Bar>& B)
{
    Cob none; if (cobs.empty() || B.empty()) return none;
    const long long cur = sidOf(B.back().absEnd);
    const Cob* best = nullptr; for (const Cob& c : cobs) if (sidOf(c.t) == cur && c.t <= B.back().absEnd && (!best || c.t > best->t)) best = &c;
    if (!best) return none;
    for (size_t i = B.size(); i-- > 0;) { const Bar& b = B[i]; if (b.absEnd <= best->t) break; if (!std::isfinite(b.c)) continue; if (best->dir > 0 ? b.c < best->origin : b.c > best->origin) return none; }
    return *best;
}
inline std::string clock12(long long t) { long long sod = ((t % 86400) + 86400) % 86400; int h = (int)(sod / 3600), m = (int)((sod % 3600) / 60); h = h % 12; if (h == 0) h = 12; char b[16]; std::snprintf(b, sizeof b, "%d:%02d", h, m); return b; }

struct Item { std::string mkt; double ratio = 0, pos = 0.5; long long epoch = 0; int cobDir = 0; long long cobT = 0; };

inline std::string fileLine(const std::string& mkt, const Stat& s, long long epoch, const Cob& cob = Cob())
{
    char b[200];
    if (cob.dir == 0) std::snprintf(b, sizeof b, "MOVE,1,%s,%.4f,%.3f,%lld\n", mkt.c_str(), s.ratio, s.pos, epoch);
    else std::snprintf(b, sizeof b, "MOVE,2,%s,%.4f,%.3f,%lld,%d,%lld\n", mkt.c_str(), s.ratio, s.pos, epoch, cob.dir, cob.t);   // (1.1.0) + the live Cob
    return b;
}
// strict: "MOVE,1,<MKT>,<ratio>,<pos>,<epoch>" or "MOVE,2,...,<cob dir 1|-1>,<cob bar end>"; the market must be the one asked for
inline bool parseLine(const std::string& text, const std::string& want, Item& out)
{
    const size_t e = text.find('\n'); const std::string l = text.substr(0, e == std::string::npos ? text.size() : e);
    std::vector<std::string> f; size_t a = 0; for (;;) { const size_t c = l.find(',', a); f.push_back(l.substr(a, c == std::string::npos ? std::string::npos : c - a)); if (c == std::string::npos) break; a = c + 1; }
    const bool v2 = f.size() == 8 && f[1] == "2";
    if (!(v2 || (f.size() == 6 && f[1] == "1")) || f[0] != "MOVE" || f[2] != want) return false;
    auto num = [](const std::string& t, double& v) { if (t.empty() || t.size() > 24) return false; char* end = nullptr; v = std::strtod(t.c_str(), &end); return end && *end == 0 && std::isfinite(v); };
    double r = 0, p = 0, ep = 0;
    if (!num(f[3], r) || !num(f[4], p) || !num(f[5], ep)) return false;
    if (std::fabs(r) > 20 || p < 0 || p > 1 || ep < 0) return false;
    out = Item(); out.mkt = want; out.ratio = r; out.pos = p; out.epoch = (long long)ep;
    if (v2) { double cd = 0, ct = 0; if (!num(f[6], cd) || !num(f[7], ct) || !(cd == 1 || cd == -1) || !(ct > 0)) return false; out.cobDir = (int)cd; out.cobT = (long long)ct; }
    return true;
}

struct Seg { std::string s; int dir = 0; bool flat = false, bold = false; };   // dir +1 up / -1 down; flat = grey; bold = strong and at the extreme (arrow)

inline const char* shown(const std::string& mkt) { return mkt == "EU" ? "6E" : mkt.c_str(); }

inline Seg segOf(const Item& it)
{
    Seg g; const int r = (int)std::lround(it.ratio * 100.0);   // (1.0.1, Rassul 2026-11-17 05:31 CT "lets use percentages like 20%") the move as a % of the average session range
    char b[48]; if (r == 0) std::snprintf(b, sizeof b, "%s 0%%", shown(it.mkt)); else std::snprintf(b, sizeof b, "%s %+d%%", shown(it.mkt), r);
    g.s = b; g.dir = it.ratio > 0 ? 1 : it.ratio < 0 ? -1 : 0; g.flat = std::fabs(it.ratio) < FLAT;
    g.bold = std::fabs(it.ratio) >= STRONG && (it.ratio > 0 ? it.pos >= 1.0 - EDGE : it.pos <= EDGE);
    return g;
}
// the line: fresh items only (now - epoch <= STALE_SEC), biggest |move| first, ties by name
inline std::vector<Seg> line(std::vector<Item> items, long long now)
{
    std::vector<Item> ok; for (const Item& it : items) if (now - it.epoch <= STALE_SEC && it.epoch - now <= STALE_SEC) ok.push_back(it);
    std::stable_sort(ok.begin(), ok.end(), [](const Item& a, const Item& b) { const double x = std::fabs(a.ratio), y = std::fabs(b.ratio); return x != y ? x > y : a.mkt < b.mkt; });
    std::vector<Seg> out; for (const Item& it : ok) out.push_back(segOf(it)); return out;
}

// (1.1.0) the Cob section: fresh items with a live Cob, newest Cob first
struct CobSeg { std::string mkt, when; int dir = 0; };
inline std::vector<CobSeg> cobLine(std::vector<Item> items, long long now)
{
    std::vector<Item> ok; for (const Item& it : items) if (it.cobDir != 0 && now - it.epoch <= STALE_SEC && it.epoch - now <= STALE_SEC) ok.push_back(it);
    std::stable_sort(ok.begin(), ok.end(), [](const Item& a, const Item& b) { return a.cobT != b.cobT ? a.cobT > b.cobT : a.mkt < b.mkt; });
    std::vector<CobSeg> out; for (const Item& it : ok) { CobSeg g; g.mkt = shown(it.mkt); g.when = clock12(it.cobT); g.dir = it.cobDir; out.push_back(g); } return out;
}

} // namespace mkm
