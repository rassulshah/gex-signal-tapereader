// EventCalLogic.h 1.0.0 (2026-11-17) - the SDK-free logic of lsEventCal, the Event Calendar box (Linux-tested).
// Rassul 2026-11-17 18:09 CT: "the calendar at the top center of important events that are coming up for the market that im trading
// ... for crude it could include inventories. other events are like fomc etc ... after 30 min of the even it can be removed";
// 18:22 "i only want market moving events medium and high impact that can move the market. also since i have limited space, be
// carefull with the font size and spacing"; 18:25 "build".
//   Input: lsFlexLevels\LRA-Calendar-ALL.csv, written each night by lra.event_calendar (EV,<yyyy-mm-dd>,<HH:MM CT>,<name>,<H|M>,<MKT>;
//   VERSION first, MARKET,ALL). Missing / unreadable / older than 8 days = the BUILT-IN list below (the dated releases known on
//   2026-11-17 + the weekly rules), so the box never goes blank because the nightly missed a night.
//   Rows: at most 3, the next events of the chart's market in TODAY's session (18:39 "i only want todays events": 5:00 PM CT to 5:00 PM CT, so
//   after 5 PM it is the next day's session), one line each: "Wed 1:00 PM   FOMC Minutes   3h 48m".
//   High impact bold, medium plain. Colours: later = white (time grey), within 60 min = amber, released = red "out 11m" for 30 min,
//   then gone. No title row, no settings, no hovers. Times = the PC's clock (Central).
#pragma once
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace ecal {
static const char* const VERSION = "1.0.0";
static const int MAX_ROWS = 3;          // his limited space
static const int SESSION_START = 17 * 60;   // (18:39 "i only want todays events") today = the trading session, 5:00 PM CT to 5:00 PM CT
static const int KEEP_MIN = 30;         // shown 30 min after the release, then removed
static const int SOON_MIN = 60;         // amber inside the last hour
static const int MAX_EVENTS = 4000;     // parse cap
static const int STALE_DAYS = 8;        // a file not rebuilt for 8 days = the built-in list

struct Ev { long long t = 0; std::string name; char tier = 'M'; std::string mkt; };   // t = CT minutes since 1970-01-01
// ---- civil time ------------------------------------------------------------------------------------------------------------------
inline long long daysFromCivil(long long y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
inline long long minAbs(int y, int mo, int d, int hh, int mi) { return daysFromCivil(y, (unsigned)mo, (unsigned)d) * 1440 + hh * 60 + mi; }
inline int dowOf(long long t) { long long days = (t >= 0 ? t / 1440 : (t - 1439) / 1440); return (int)(((days % 7) + 11) % 7); }   // 0 = Sunday (1970-01-01 was a Thursday)
inline std::string hm12(int minOfDay) { int h = minOfDay / 60, m = minOfDay % 60; const char* ap = h >= 12 ? "PM" : "AM"; h %= 12; if (h == 0) h = 12; char b[16]; std::snprintf(b, sizeof b, "%d:%02d %s", h, m, ap); return b; }
inline std::string whenText(long long t) {
    static const char* const D[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    const int mod = (int)(((t % 1440) + 1440) % 1440);
    return std::string(D[dowOf(t)]) + " " + hm12(mod);
}
// "18m", "3h 48m", "1d 13h"; released: "out 11m"
inline std::string countText(long long t, long long now) {
    char b[32]; long long d = t - now;
    if (d <= 0) { std::snprintf(b, sizeof b, "out %lldm", -d); return b; }
    if (d < 60) std::snprintf(b, sizeof b, "%lldm", d);
    else if (d < 1440) std::snprintf(b, sizeof b, "%lldh %lldm", d / 60, d % 60);
    else std::snprintf(b, sizeof b, "%lldd %lldh", d / 1440, (d % 1440) / 60);
    return b;
}
// ---- parse ----------------------------------------------------------------------------------------------------------------------
inline std::string trim(const std::string& s) { size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n"); return a == std::string::npos ? std::string() : s.substr(a, b - a + 1); }
inline std::vector<std::string> split(const std::string& l) { std::vector<std::string> v; size_t a = 0; for (;;) { size_t b = l.find(',', a); v.push_back(trim(l.substr(a, b == std::string::npos ? std::string::npos : b - a))); if (b == std::string::npos) break; a = b + 1; } return v; }
inline bool num(const std::string& s, int lo, int hi, int& out) { if (s.empty() || s.size() > 4) return false; for (char c : s) if (c < '0' || c > '9') return false; out = std::atoi(s.c_str()); return out >= lo && out <= hi; }
inline bool dateOf(const std::string& s, int& y, int& m, int& d) { return s.size() == 10 && s[4] == '-' && s[7] == '-' && num(s.substr(0, 4), 2000, 2100, y) && num(s.substr(5, 2), 1, 12, m) && num(s.substr(8, 2), 1, 31, d); }
inline bool timeOf(const std::string& s, int& h, int& mi) { const size_t c = s.find(':'); return c != std::string::npos && num(s.substr(0, c), 0, 23, h) && num(s.substr(c + 1), 0, 59, mi); }
struct File { bool ok = false; std::string why; long long built = 0; std::vector<Ev> ev; };
inline File parse(const std::string& text) {
    File F; size_t a = 0; int line = 0; bool ver = false, mk = false;
    while (a < text.size()) {
        size_t b = text.find('\n', a); const std::string l = trim(text.substr(a, b == std::string::npos ? std::string::npos : b - a)); a = b == std::string::npos ? text.size() : b + 1;
        if (l.empty() || l[0] == '#') continue;
        const std::vector<std::string> r = split(l); ++line;
        if (line == 1) { if (r[0] != "VERSION" || r.size() < 2) { F.why = "VERSION must come first"; return F; } ver = true; continue; }
        if (r[0] == "MARKET") { if (r.size() < 2 || r[1] != "ALL") { F.why = "MARKET must be ALL"; return F; } mk = true; continue; }
        if (r[0] == "BUILT" && r.size() >= 2) { int y, m, d, hh = 0, mi = 0; const std::string& s = r[1]; if (s.size() >= 10 && dateOf(s.substr(0, 10), y, m, d)) { if (s.size() >= 16) timeOf(s.substr(11, 5), hh, mi); F.built = minAbs(y, m, d, hh, mi); } continue; }
        if (r[0] != "EV" || r.size() < 6) continue;
        int y, m, d, hh, mi; if (!dateOf(r[1], y, m, d) || !timeOf(r[2], hh, mi)) continue;
        if (r[3].empty() || r[3].size() > 24 || (r[4] != "H" && r[4] != "M") || r[5].empty() || r[5].size() > 4) continue;
        Ev e; e.t = minAbs(y, m, d, hh, mi); e.name = r[3]; e.tier = r[4][0]; e.mkt = r[5]; F.ev.push_back(e);
        if ((int)F.ev.size() >= MAX_EVENTS) break;
    }
    if (!ver) { F.why = "empty"; return F; }
    if (!mk) { F.why = "no MARKET,ALL line"; return F; }
    F.ok = true; F.why = std::to_string(F.ev.size()) + " events"; return F;
}
// ---- the built-in list (used when the nightly file is missing / bad / stale) ---------------------------------------------------
struct Fixed { int y, m, d, hh, mi; const char* name; char tier; const char* mkts; };
inline const std::vector<Fixed>& fixedList() {
    static const std::vector<Fixed> L = {   // analytics/lra/data/calendar_2026_2027.csv on 2026-11-17 (official schedules; see its sources)
        {2026,11,18,13,0,"FOMC Minutes",'H',"ALL"}, {2026,11,25,7,30,"GDP + PCE",'H',"ALL"}, {2026,11,25,7,30,"Jobless Claims",'M',"ALL"},
        {2026,11,25,11,0,"EIA Gas",'H',"NG"}, {2026,11,29,6,0,"OPEC+",'H',"CL"}, {2026,11,29,19,30,"China PMI",'M',"HG"},
        {2026,12,1,9,0,"ISM Mfg",'M',"ALL"}, {2026,12,4,7,30,"NFP",'H',"ALL"}, {2026,12,9,13,0,"FOMC",'H',"ALL"}, {2026,12,10,7,30,"CPI",'H',"ALL"},
        {2026,12,15,7,30,"PPI",'M',"ALL"}, {2026,12,16,7,30,"Retail Sales",'H',"ALL"}, {2026,12,17,7,15,"ECB",'H',"EU"}, {2026,12,17,7,45,"ECB Presser",'M',"EU"},
        {2026,12,23,7,30,"GDP + PCE",'H',"ALL"}, {2026,12,30,13,0,"FOMC Minutes",'H',"ALL"}, {2026,12,30,19,30,"China PMI",'M',"HG"},
        {2027,1,4,9,0,"ISM Mfg",'M',"ALL"}, {2027,1,15,7,30,"Retail Sales",'H',"ALL"}, {2027,1,21,11,0,"EIA Crude",'H',"CL"},
        {2027,1,27,13,0,"FOMC",'H',"ALL"}, {2027,2,4,7,15,"ECB",'H',"EU"}, {2027,2,4,7,45,"ECB Presser",'M',"EU"},
        {2027,2,17,7,30,"Retail Sales",'H',"ALL"}, {2027,3,16,7,30,"Retail Sales",'H',"ALL"}, {2027,3,17,13,0,"FOMC",'H',"ALL"},
        {2027,3,18,7,15,"ECB",'H',"EU"}, {2027,3,18,7,45,"ECB Presser",'M',"EU"},
    };
    return L;
}
inline bool isHoliday(long long day) {   // days since 1970: no weekly rule rows on these
    static const int H[][3] = { {2026,11,26},{2026,12,25},{2027,1,1},{2027,1,18},{2027,2,15},{2027,4,2},{2027,5,31},{2027,6,18},{2027,7,5},{2027,9,6},{2027,11,25},{2027,12,24} };
    for (const auto& h : H) if (daysFromCivil(h[0], (unsigned)h[1], (unsigned)h[2]) == day) return true;
    return false;
}
inline bool forMkt(const std::string& list, const std::string& mkt) { return list == "ALL" || list == mkt; }
// the built-in events of one market from day d0 to d1 (days since 1970); a dated row of the same name in the same week replaces the rule
inline std::vector<Ev> builtIn(const std::string& mkt, long long d0, long long d1) {
    std::vector<Ev> v;
    for (const Fixed& f : fixedList()) { const long long t = minAbs(f.y, f.m, f.d, f.hh, f.mi); if (t / 1440 < d0 || t / 1440 > d1 || !forMkt(f.mkts, mkt)) continue; Ev e; e.t = t; e.name = f.name; e.tier = f.tier; e.mkt = mkt; v.push_back(e); }
    struct W { int dow; const char* name; int min; char tier; const char* mkts; };
    static const W R[] = { {4,"Jobless Claims",450,'M',"ALL"}, {3,"EIA Crude",570,'H',"CL"}, {4,"EIA Gas",570,'H',"NG"}, {2,"API Crude",930,'M',"CL"} };
    auto week = [](long long day) { return (day + 3) / 7; };   // Monday-start weeks (day 0 = Thursday)
    for (long long d = d0; d <= d1; ++d) {
        if (isHoliday(d)) continue;
        for (const W& r : R) {
            if (dowOf(d * 1440) != r.dow || !forMkt(r.mkts, mkt)) continue;
            bool dated = false; for (const Ev& e : v) if (e.name == r.name && week(e.t / 1440) == week(d)) dated = true;
            if (dated) continue;
            Ev e; e.t = d * 1440 + r.min; e.name = r.name; e.tier = r.tier; e.mkt = mkt; v.push_back(e);
        }
    }
    std::sort(v.begin(), v.end(), [](const Ev& a, const Ev& b) { return a.t < b.t || (a.t == b.t && a.name < b.name); });
    return v;
}
// ---- the rows -------------------------------------------------------------------------------------------------------------------
enum State { ST_LATER = 0, ST_SOON = 1, ST_OUT = 2 };
struct Row { long long t = 0; std::string when, name, count; char tier = 'M'; int state = ST_LATER; };
// the end of the session that holds `now` (the next 5:00 PM CT)
inline long long sessionEnd(long long now) { const long long day = now / 1440, mod = now % 1440; return (mod >= SESSION_START ? day + 1 : day) * 1440 + SESSION_START; }
inline std::vector<Row> select(const std::vector<Ev>& ev, const std::string& mkt, long long now, int maxRows = MAX_ROWS) {
    const long long end = sessionEnd(now);
    std::vector<Ev> v; for (const Ev& e : ev) if (e.mkt == mkt && e.t >= now - KEEP_MIN && e.t < end) v.push_back(e);
    std::sort(v.begin(), v.end(), [](const Ev& a, const Ev& b) { return a.t < b.t || (a.t == b.t && (a.tier == 'H') > (b.tier == 'H')) || (a.t == b.t && a.tier == b.tier && a.name < b.name); });
    std::vector<Row> R;
    for (const Ev& e : v) {
        bool dup = false; for (const Row& r : R) if (r.t == e.t && r.name == e.name) dup = true;
        if (dup) continue;
        Row r; r.t = e.t; r.when = whenText(e.t); r.name = e.name; r.count = countText(e.t, now); r.tier = e.tier;
        r.state = e.t <= now ? ST_OUT : (e.t - now <= SOON_MIN ? ST_SOON : ST_LATER);
        R.push_back(r); if ((int)R.size() >= maxRows) break;
    }
    return R;
}
// which list to use: the nightly file when good and fresh, else the built-in one (the reason goes to the status file)
inline std::vector<Ev> source(const File& F, const std::string& mkt, long long now, std::string* why) {
    if (F.ok && F.built > 0 && now - F.built <= (long long)STALE_DAYS * 1440) { if (why) *why = "nightly file, " + F.why; return F.ev; }
    if (why) *why = std::string("built-in list (") + (F.ok ? (F.built > 0 ? "file older than 8 days" : "file has no BUILT line") : (F.why.empty() ? "no file" : F.why)) + ")";
    return builtIn(mkt, now / 1440 - 1, now / 1440 + 3);
}
// ---- layout ---------------------------------------------------------------------------------------------------------------------
static const unsigned long C_BG = 0x000B0F14, C_EDGE = 0x00444444, C_TEXT = 0x00E8E8E8, C_DIM = 0x009A9A9A, C_AMBER = 0x00FFB020, C_RED = 0x00E5484D;   // 0x00RRGGBB
static const int PAD = 4, GAP = 8, ROW_H = 13;
struct Cell { int x = 0, y = 0, w = 0, h = 0; std::string text; unsigned long col = 0; bool bold = false; bool right = false; };
struct Layout { int w = 0, h = 0; std::vector<Cell> cells; };
// textW(text, bold) = pixel width. Cells are relative to the box's top-left corner.
inline Layout layout(const std::vector<Row>& R, const std::function<int(const std::string&, bool)>& textW) {
    Layout L; if (R.empty()) return L;
    int w0 = 0, w1 = 0, w2 = 0;
    for (const Row& r : R) { const bool b = r.tier == 'H'; w0 = std::max(w0, textW(r.when, b)); w1 = std::max(w1, textW(r.name, b)); w2 = std::max(w2, textW(r.count, b)); }
    L.w = PAD + w0 + GAP + w1 + GAP + w2 + PAD; L.h = PAD / 2 + (int)R.size() * ROW_H + PAD / 2;
    for (size_t i = 0; i < R.size(); ++i) {
        const Row& r = R[i]; const bool b = r.tier == 'H'; const int y = PAD / 2 + (int)i * ROW_H;
        const unsigned long c = r.state == ST_OUT ? C_RED : r.state == ST_SOON ? C_AMBER : C_TEXT;
        Cell a; a.x = PAD; a.y = y; a.w = w0; a.h = ROW_H; a.text = r.when; a.col = r.state == ST_LATER ? C_DIM : c; a.bold = b; L.cells.push_back(a);
        Cell n; n.x = PAD + w0 + GAP; n.y = y; n.w = w1; n.h = ROW_H; n.text = r.name; n.col = c; n.bold = b; L.cells.push_back(n);
        Cell k; k.x = PAD + w0 + GAP + w1 + GAP; k.y = y; k.w = w2; k.h = ROW_H; k.text = r.count; k.col = c; k.bold = b; k.right = true; L.cells.push_back(k);
    }
    return L;
}
inline std::string statusText(const std::string& mkt, const std::string& srcWhy, const std::vector<Row>& R, const std::string& placeWhy) {
    std::string s = std::string("lsEventCal ") + VERSION + "\nMARKET," + mkt + "\nSOURCE," + srcWhy + "\nPLACE," + placeWhy + "\n";
    for (const Row& r : R) s += "ROW," + r.when + "," + r.name + "," + r.tier + "," + (r.state == ST_OUT ? "out" : r.state == ST_SOON ? "soon" : "later") + "\n";
    if (R.empty()) s += "ROW,none left today\n";
    return s;
}
} // namespace ecal
