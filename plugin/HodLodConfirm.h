// HodLodConfirm.h - the HOD / LOD CONFIRM badge of lsHodLod (SDK-free, C++14, ASCII). 1.0.0 (2026-11-07, lsHodLod 2.21.0)
// Rassul 2026-10-21: "badges marking where a HOD / LOD occurred, drawn only AFTER the reversal is confirmed - not while the extreme
// is forming; confirmation = a certain amount of movement in the opposite direction (a CoB is not required)". 2026-11-07 19:36 CT
// "hod lod is a go"; option B approved on the mockup (HOD LOD Confirm Badge): taken-out early badges stay exactly as drawn.
// THE RULE (option B, the same k for every market; ADR sets the amount per market):
//   a running RTH HOD is CONFIRMED on the first CLOSED bar whose CLOSE is >= K x ADR below it AND whose END is >= HOLD_SEC after the
//   END of the extreme's bar (no higher high since - a higher high starts a new running HOD). LOD = the mirror. Ties keep the FIRST
//   bar that printed the price. ADR = the mean of the last 14 RTH ranges before the session (weekday sessions with RTH bars; a
//   session whose RTH open is before the first loaded bar is not used - a cut session would change the ADR). Roll day (|first RTH
//   open - prior RTH close| > 4 ADR) = no badges. Fewer than 14 usable prior sessions = no badges (status says so).
// Measured (lra hodlod confirm study 2026-11-07, 3-min RTH, May 2025 - Nov 6 2026, ~370 sessions a market, build / check halves
// agree within ~3 pts): the day's real HOD / LOD gets a badge 78-84%, ~1.0 badge a day (both sides) is later taken out, 60-64% of
// badges mark the real extreme, lag median 30 min (q75 36-48).
// Decided on closed bars only, from the chart's bars alone: the same bars give the same badges (no repaint; a restart redraws them).
#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace hcf {
static const char* const VERSION = "1.0.0";
static const double K = 0.25;          // x ADR, close back from the extreme
static const long long HOLD_SEC = 1800; // the extreme held 30 minutes (bar END to bar END)
static const int ADR_N = 14;
static const double ROLL_ADR = 4.0;

struct Bar { long long absEnd; long long h, l, c, o; };   // ticks (price / tick, rounded); absEnd = CT wall-clock seconds of the bar END
struct Badge { bool hod; int ext, conf; long long px; };     // indices into the session's bars; px = the extreme (ticks)
struct Session { long long day = 0; int first = -1, last = -1; double adrT = NAN; bool roll = false, usable = false; std::string why; std::vector<Badge> badges; };

inline long long dayOf(long long absEnd) { return absEnd >= 0 ? absEnd / 86400 : -((-absEnd + 86399) / 86400); }
inline int modOf(long long absEnd) { return (int)(((absEnd % 86400) + 86400) % 86400 / 60); }
inline int weekday(long long day) { return (int)(((day % 7) + 7 + 4) % 7); }   // 1970-01-01 = Thursday (4); 0 = Sunday
inline long long d2open(long long day, int open) { return day * 86400 + (long long)open * 60; }
// an RTH bar (= lsHodLod's Tracker rule): bar START >= the open and bar END <= the close, on the same CT day (per = bar seconds)
inline bool rthBar(long long absEnd, long long per, int open, int close)
{ const long long d = dayOf(absEnd), st = absEnd - per; return st >= d * 86400 + (long long)open * 60 && absEnd <= d * 86400 + (long long)close * 60; }

// the badges of one session's RTH bars (B[first..last]) with the given ADR (ticks)
inline std::vector<Badge> badgesOf(const std::vector<Bar>& B, int first, int last, double adrT)
{
    std::vector<Badge> out;
    if (first < 0 || last < first || !(adrT > 0) || !std::isfinite(adrT)) return out;
    const double thr = K * adrT - 1e-9;
    for (int side = 0; side < 2; side++) {
        const bool hod = side == 0;
        long long ext = 0; int ei = -1; bool conf = false;
        for (int i = first; i <= last; i++) {
            const long long x = hod ? B[(size_t)i].h : -B[(size_t)i].l;
            const long long c = hod ? B[(size_t)i].c : -B[(size_t)i].c;
            if (ei < 0 || x > ext) { ext = x; ei = i; conf = false; }
            if (!conf && (double)(ext - c) >= thr && B[(size_t)i].absEnd - B[(size_t)ei].absEnd >= HOLD_SEC) {
                out.push_back(Badge{ hod, ei, i, hod ? ext : -ext }); conf = true;
            }
        }
    }
    std::sort(out.begin(), out.end(), [](const Badge& a, const Badge& b) { return a.conf != b.conf ? a.conf < b.conf : a.hod > b.hod; });
    return out;
}

// every weekday RTH session in the bars, each with its ADR, roll check and badges. Bars must be in time order (closed bars only).
inline std::vector<Session> sessions(const std::vector<Bar>& B, long long per, int open, int close)
{
    std::vector<Session> S;
    const long long firstLoaded = B.empty() ? 0 : B.front().absEnd - per;   // the first loaded bar's START
    for (int i = 0; i < (int)B.size(); i++) {
        if (!rthBar(B[(size_t)i].absEnd, per, open, close)) continue;
        const long long d = dayOf(B[(size_t)i].absEnd);
        const int wd = weekday(d); if (wd == 0 || wd == 6) continue;
        if (S.empty() || S.back().day != d) { Session s; s.day = d; s.first = i; s.last = i; S.push_back(s); }
        else S.back().last = i;
    }
    std::vector<double> rng; std::vector<long long> lastC; // usable prior sessions only
    for (size_t k = 0; k < S.size(); k++) {
        Session& s = S[k];
        const bool cut = d2open(s.day, open) < firstLoaded;   // the session's RTH open is before the first loaded bar's start
        if (rng.size() >= (size_t)ADR_N) {
            double a = 0; for (size_t j = rng.size() - ADR_N; j < rng.size(); j++) a += rng[j];
            s.adrT = a / ADR_N;
            const double gap = std::fabs((double)(B[(size_t)s.first].o - lastC.back())) / s.adrT;
            s.roll = gap > ROLL_ADR;
            s.usable = !s.roll;
            if (s.roll) s.why = "roll day (gap > 4 ADR): no badges";
            else s.badges = badgesOf(B, s.first, s.last, s.adrT);
        } else s.why = "needs 14 prior RTH sessions on the chart (has " + std::to_string(rng.size()) + ")";
        if (!cut) {
            long long hi = B[(size_t)s.first].h, lo = B[(size_t)s.first].l;
            for (int i = s.first; i <= s.last; i++) { hi = (std::max)(hi, B[(size_t)i].h); lo = (std::min)(lo, B[(size_t)i].l); }
            rng.push_back((double)(hi - lo)); lastC.push_back(B[(size_t)s.last].c);
        }
    }
    return S;
}

// the badge pill: "HOD" above the extreme (HOD) / "LOD" below it, centred on the extreme candle; the first spot clear of every
// obstacle and inside the pane, stepping away from price, then sideways toward the confirm bar; none clear = no pill (line kept)
struct Px { int l, t, r, b; };
inline bool hit(const Px& a, const Px& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
inline bool pill(int xExt, int xConf, int yExt, bool hod, int w, int h, const Px& pane, const std::vector<Px>& obst, Px& out)
{
    const int dys[3] = { 4, 4 + h + 2, 4 + 2 * (h + 2) };
    const int dir = xConf >= xExt ? 1 : -1;
    const int dxs[3] = { 0, dir * (w / 2 + 2), dir * (w + 4) };
    for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) {
        const int cx = xExt + dxs[a];
        const int t = hod ? yExt - dys[b] - h : yExt + dys[b];
        const Px p{ cx - w / 2, t, cx - w / 2 + w, t + h };
        if (p.l < pane.l || p.r > pane.r || p.t < pane.t || p.b > pane.b) continue;
        bool clear = true; for (const Px& o : obst) if (hit(p, o)) { clear = false; break; }
        if (clear) { out = p; return true; }
    }
    return false;
}

// one record line per badge: B,<CT day yyyymmdd>,HOD|LOD,<extreme bar end HH:MM>,<confirm bar end HH:MM>,<price>,<adr>,<version>
inline std::string ymd(long long day)
{
    long long z = day + 719468, era = (z >= 0 ? z : z - 146096) / 146097; unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; long long y = (long long)yoe + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); unsigned mp = (5 * doy + 2) / 153; unsigned dd = doy - (153 * mp + 2) / 5 + 1;
    unsigned mm = mp < 10 ? mp + 3 : mp - 9; if (mm <= 2) y++;
    char b[48]; std::snprintf(b, sizeof b, "%04lld%02u%02u", y, mm, dd); return b;
}
inline std::string hm(long long absEnd) { char b[16]; const int m = modOf(absEnd); std::snprintf(b, sizeof b, "%02d:%02d", m / 60, m % 60); return b; }
inline std::string key(const Session& s, const std::vector<Bar>& B, const Badge& g) { return ymd(s.day) + "," + (g.hod ? "HOD" : "LOD") + "," + hm(B[(size_t)g.ext].absEnd); }
inline std::string recordLine(const Session& s, const std::vector<Bar>& B, const Badge& g, double tick, int dec, const char* plugin)
{
    char px[48], adr[48];
    std::snprintf(px, sizeof px, "%.*f", dec, (double)g.px * tick); std::snprintf(adr, sizeof adr, "%.*f", dec, s.adrT * tick);
    return "B," + key(s, B, g) + "," + hm(B[(size_t)g.conf].absEnd) + "," + px + "," + adr + "," + plugin + "\n";
}
}  // namespace hcf
