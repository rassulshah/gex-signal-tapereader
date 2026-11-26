// EventCalLogic.h 1.1.0 (2026-11-26; 1.1.0: the contract line + the 0DTE line on top, a thin rule, then the events; docked top left. 1.0.1: status VERSION line + 10-min heartbeat for the nightly code-health check, today-only rows) - the SDK-free logic of lsEventCal, the Event Calendar box (Linux-tested).
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
static const char* const VERSION = "1.1.0";
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
    static const int H[][3] = { {2026,11,26},{2026,12,25},{2027,1,1},{2027,1,18},{2027,2,15},{2027,3,26},{2027,5,31},{2027,6,18},{2027,7,5},{2027,9,6},{2027,11,25},{2027,12,24} };
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
// ---- (1.1.0) the exchange calendar, the contract to trade and today's option expiry --------------------------------------------
// Rassul 2026-11-26 08:05 CT "i also need the expiration displayed ... so i know what contract i should be trading"; 08:06 "i also need
// the 0 dte option expiry. later we might need to use this to determine charm flows impacting price"; 08:31 "move the calendar to the
// left of the price chart ... no black space on top of it and to the left of it. ok build". Everything below is computed here from
// CME's rules (no file needed): the front month and its roll day, and the options that expire today.
inline bool exHoliday(long long day) {   // CME: no settlement / no expiry on these days (2026-2028)
    static const int H[][3] = {
        {2026,1,1},{2026,1,19},{2026,2,16},{2026,4,3},{2026,5,25},{2026,6,19},{2026,7,3},{2026,9,7},{2026,11,26},{2026,12,25},
        {2027,1,1},{2027,1,18},{2027,2,15},{2027,3,26},{2027,5,31},{2027,6,18},{2027,7,5},{2027,9,6},{2027,11,25},{2027,12,24},
        {2028,1,17},{2028,2,21},{2028,4,14},{2028,5,29},{2028,6,19},{2028,7,4},{2028,9,4},{2028,11,23},{2028,12,25} };
    for (const auto& h : H) if (daysFromCivil(h[0], (unsigned)h[1], (unsigned)h[2]) == day) return true;
    return false;
}
inline bool earlyClose(long long day) {  // the day after Thanksgiving and Christmas Eve: options expire at the early close
    static const int H[][3] = { {2026,11,27},{2026,12,24},{2027,11,26},{2028,11,24} };
    for (const auto& h : H) if (daysFromCivil(h[0], (unsigned)h[1], (unsigned)h[2]) == day) return true;
    return false;
}
inline int dowDay(long long day) { return dowOf(day * 1440); }
inline bool isBiz(long long day) { const int w = dowDay(day); return w >= 1 && w <= 5 && !exHoliday(day); }
inline long long bizBack(long long day, int n) { while (n > 0) { --day; if (isBiz(day)) --n; } return day; }   // n business days before `day`
inline long long bizOnOrBefore(long long day) { while (!isBiz(day)) --day; return day; }
inline void civil(long long day, int& y, int& m, int& d) {   // days since 1970 -> y m d
    day += 719468; const long long era = (day >= 0 ? day : day - 146096) / 146097; const unsigned doe = (unsigned)(day - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153; d = (int)(doy - (153 * mp + 2) / 5 + 1); m = (int)(mp < 10 ? mp + 3 : mp - 9); y = (int)(yoe + era * 400 + (m <= 2));
}
inline long long nthDow(int y, int m, int dow, int n) {   // the n-th <dow> (0 = Sunday) of a month
    const long long first = daysFromCivil(y, (unsigned)m, 1u); const int w = dowDay(first);
    return first + (dow - w + 7) % 7 + (long long)(n - 1) * 7;
}
inline long long lastBiz(int y, int m) { int ny = y, nm = m + 1; if (nm > 12) { nm = 1; ++ny; } return bizOnOrBefore(daysFromCivil(ny, (unsigned)nm, 1u) - 1); }
inline void addMonths(int& y, int& m, int k) { const int t = y * 12 + (m - 1) + k; y = t / 12; m = t % 12 + 1; }
static const char MONTH_CODE[] = "FGHJKMNQUVXZ";
inline std::string mdText(long long day) { int y, m, d; civil(day, y, m, d); static const char* const D[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" }; char b[24]; std::snprintf(b, sizeof b, "%s %d/%d", D[dowDay(day)], m, d); return b; }
// the root he sees on the chart for each market
inline std::string rootOf(const std::string& mkt) { return mkt == "EU" ? "6E" : mkt; }
inline bool knownMkt(const std::string& mkt) { return mkt == "ES" || mkt == "NQ" || mkt == "CL" || mkt == "GC" || mkt == "HG" || mkt == "NG" || mkt == "EU"; }
inline bool listed(const std::string& mkt, int m) {   // the months he trades (the active months of each market)
    if (mkt == "ES" || mkt == "NQ" || mkt == "EU") return m % 3 == 0;
    if (mkt == "GC") return m == 2 || m == 4 || m == 6 || m == 8 || m == 10 || m == 12;
    if (mkt == "HG") return m == 3 || m == 5 || m == 7 || m == 9 || m == 12;
    return true;                                       // CL, NG: every month
}
struct Contract { int y = 0, m = 0; long long ltd = 0, roll = 0; std::string code(const std::string& mkt) const { return rootOf(mkt) + MONTH_CODE[m - 1] + std::to_string(y % 10); } };
// last trading day (or first notice day for GC / HG) and the day the volume moves to the next contract
inline Contract contractOf(const std::string& mkt, int y, int m) {
    Contract c; c.y = y; c.m = m;
    if (mkt == "ES" || mkt == "NQ") { c.ltd = bizOnOrBefore(nthDow(y, m, 5, 3)); c.roll = bizOnOrBefore(c.ltd - 8); }           // 3rd Friday; roll the Thursday 8 days before
    else if (mkt == "EU") { c.ltd = bizBack(nthDow(y, m, 3, 3), 2); c.roll = bizOnOrBefore(c.ltd - 7); }                      // 2 business days before the 3rd Wednesday
    else if (mkt == "CL") { int py = y, pm = m; addMonths(py, pm, -1); const long long d25 = daysFromCivil(py, (unsigned)pm, 25u);
                            c.ltd = bizBack(isBiz(d25) ? d25 : bizOnOrBefore(d25), 3); c.roll = bizBack(c.ltd, 6); }           // 3 business days before the 25th of the prior month
    else if (mkt == "NG") { c.ltd = bizBack(daysFromCivil(y, (unsigned)m, 1u), 3); c.roll = bizBack(c.ltd, 4); }               // 3 business days before the 1st
    else { int py = y, pm = m; addMonths(py, pm, -1); c.ltd = lastBiz(py, pm); c.roll = bizBack(c.ltd, 2); }                   // GC / HG: first notice = last business day of the prior month
    return c;
}
struct Front { bool ok = false; Contract cur, next; };
// the contract to trade on `day`: the first listed one whose roll day is still ahead
inline Front frontOf(const std::string& mkt, long long day) {
    Front F; if (!knownMkt(mkt)) return F;
    int y, m, d; civil(day, y, m, d); addMonths(y, m, -1);
    bool have = false;
    for (int k = 0; k < 30; ++k, addMonths(y, m, 1)) {
        if (!listed(mkt, m)) continue;
        const Contract c = contractOf(mkt, y, m);
        if (!have) { if (day < c.roll) { F.cur = c; have = true; } continue; }
        F.next = c; F.ok = true; return F;
    }
    return F;
}
// the chart's contract from its symbol ("ESZ6", "6EZ6", "GCG27", "MNQZ6"); false = no month / year in the symbol
inline bool parseContract(const std::string& sym, int todayY, int& y, int& m) {
    size_t e = sym.size(); while (e > 0 && (sym[e - 1] == ' ')) --e;
    size_t ds = e; while (ds > 0 && sym[ds - 1] >= '0' && sym[ds - 1] <= '9') --ds;
    const size_t nd = e - ds; if (nd < 1 || nd > 2 || ds < 2) return false;
    const char* p = std::strchr(MONTH_CODE, sym[ds - 1]); if (!p || !*p) return false;
    m = (int)(p - MONTH_CODE) + 1; const int v = std::atoi(sym.substr(ds, nd).c_str());
    if (nd == 2) y = 2000 + v;
    else { y = todayY - todayY % 10 + v; if (y < todayY - 2) y += 10; else if (y > todayY + 7) y -= 10; }
    return true;
}
enum CState { CS_NONE = 0, CS_OK = 1, CS_SOON = 2, CS_OLD = 3, CS_AHEAD = 4 };
struct ContractLine { int state = CS_NONE; std::string label, text, count; };
inline ContractLine contractLine(const std::string& mkt, const std::string& chartSym, long long now) {
    ContractLine L; const long long day = now / 1440; const Front F = frontOf(mkt, day); if (!F.ok) return L;
    int ty, tm, td; civil(day, ty, tm, td);
    int cy = 0, cm = 0; const bool parsed = parseContract(chartSym, ty, cy, cm);
    const std::string front = F.cur.code(mkt), nxt = F.next.code(mkt);
    int bizLeft = 0; for (long long d = day + 1; d <= F.cur.roll; ++d) if (isBiz(d)) ++bizLeft;
    char cnt[16]; std::snprintf(cnt, sizeof cnt, "%lldd", F.cur.roll - day);
    if (parsed && cy * 12 + cm < F.cur.y * 12 + F.cur.m) { L.state = CS_OLD; L.label = rootOf(mkt) + MONTH_CODE[cm - 1] + std::to_string(cy % 10) + " OLD"; L.text = "trade " + front + " now"; return L; }
    if (parsed && cy * 12 + cm > F.cur.y * 12 + F.cur.m) { L.state = CS_AHEAD; L.label = rootOf(mkt) + MONTH_CODE[cm - 1] + std::to_string(cy % 10); L.text = "front is " + front + " until " + mdText(F.cur.roll); L.count = cnt; return L; }
    L.state = bizLeft <= 3 ? CS_SOON : CS_OK;
    L.label = front; L.text = std::string(L.state == CS_SOON ? "ROLL to " : "roll to ") + nxt + " " + mdText(F.cur.roll); L.count = cnt;
    return L;
}
// today's option expiries of a market: minute of day (CT) and the kind (0 daily, 1 weekly, 2 month-end, 3 monthly, 4 quarterly)
struct Expiry { long long t = 0; int kind = 0; };
inline const char* kindName(int k) { static const char* const N[] = { "daily", "weekly", "MONTH-END", "MONTHLY", "QUARTERLY" }; return N[k < 0 ? 0 : k > 4 ? 4 : k]; }
inline std::vector<Expiry> expiriesOn(const std::string& mkt, long long day) {
    std::vector<Expiry> v; if (!knownMkt(mkt) || !isBiz(day)) return v;
    int y, m, d; civil(day, y, m, d); const int w = dowDay(day); const bool early = earlyClose(day);
    auto add = [&](int minOfDay, int kind) { if (early && minOfDay > 12 * 60) minOfDay = 12 * 60; Expiry e; e.t = day * 1440 + minOfDay; e.kind = kind; v.push_back(e); };
    const int daily = w == 5 ? 1 : 0;
    if (mkt == "ES" || mkt == "NQ") {
        if (day == bizOnOrBefore(nthDow(y, m, 5, 3))) add(8 * 60 + 30, m % 3 == 0 ? 4 : 3);                     // 3rd Friday: the AM standard options (open print)
        else add(15 * 60, day == lastBiz(y, m) ? 2 : daily);                                                    // the PM weeklies / dailies; month-end
        return v;
    }
    int minute = 0, monthly = 0;
    if (mkt == "CL") { minute = 13 * 60 + 30; int ny = y, nm = m; addMonths(ny, nm, 1); monthly = day == bizBack(contractOf("CL", ny, nm).ltd, 3) ? 3 : 0; }
    else if (mkt == "NG") { minute = 13 * 60 + 30; int ny = y, nm = m; addMonths(ny, nm, 1); monthly = day == bizBack(contractOf("NG", ny, nm).ltd, 1) ? 3 : 0; }
    else if (mkt == "GC" || mkt == "HG") { minute = mkt == "GC" ? 12 * 60 + 30 : 12 * 60; monthly = day == bizBack(lastBiz(y, m), 3) ? 3 : 0; }   // the 4th last business day of the month
    else {                                                                                                                                 // EU: the 2nd Friday before the 3rd Wednesday
        minute = 9 * 60;
        long long f2 = nthDow(y, m, 3, 3); int seen = 0; while (seen < 2) { --f2; if (dowDay(f2) == 5) ++seen; }
        if (day == bizOnOrBefore(f2)) monthly = m % 3 == 0 ? 4 : 3;
    }
    add(minute, monthly ? monthly : daily);
    return v;
}
enum XState { XS_NONE = 0, XS_LATER = 1, XS_SOON = 2, XS_OUT = 3, XS_NEXT = 4 };
struct ExpiryLine { int state = XS_NONE; std::string text, count; bool big = false; long long t = 0; int kind = 0; long long minsLeft = -1; };
// the session day (after 5 PM CT the next day's session) - the 0DTE line follows the trading session like the events
inline long long sessionDay(long long now) { const long long day = now / 1440, mod = now % 1440; return mod >= SESSION_START ? day + 1 : day; }
inline ExpiryLine expiryLine(const std::string& mkt, long long now) {
    ExpiryLine L; if (!knownMkt(mkt)) return L;
    const long long sd = sessionDay(now);
    const std::vector<Expiry> today = expiriesOn(mkt, sd);
    for (const Expiry& e : today) {
        if (e.t + KEEP_MIN <= now) continue;
        L.t = e.t; L.kind = e.kind; L.big = e.kind >= 2; L.minsLeft = e.t - now;
        L.text = whenText(e.t) + " " + kindName(e.kind) + (earlyClose(sd) ? " (early close)" : "");
        if (e.t <= now) { L.state = XS_OUT; char b[24]; std::snprintf(b, sizeof b, "expired %lldm", now - e.t); L.count = b; }
        else { L.state = e.t - now <= SOON_MIN ? XS_SOON : XS_LATER; L.count = countText(e.t, now); }
        return L;
    }
    for (long long d = sd + 1; d <= sd + 14; ++d) {
        const std::vector<Expiry> v = expiriesOn(mkt, d); if (v.empty()) continue;
        L.state = XS_NEXT; L.t = v[0].t; L.kind = v[0].kind; L.big = false; L.minsLeft = v[0].t - now;
        L.text = std::string(today.empty() ? "none today" : "none left today") + " - next " + whenText(v[0].t) + " " + kindName(v[0].kind) + (earlyClose(d) ? " (early close)" : "");
        return L;
    }
    return L;
}
// ---- layout ---------------------------------------------------------------------------------------------------------------------
static const unsigned long C_BG = 0x000B0F14, C_EDGE = 0x00444444, C_TEXT = 0x00E8E8E8, C_DIM = 0x009A9A9A, C_AMBER = 0x00FFB020, C_RED = 0x00E5484D, C_RULE = 0x002A2F36;   // 0x00RRGGBB
static const int PAD = 4, GAP = 8, ROW_H = 13;
struct Cell { int x = 0, y = 0, w = 0, h = 0; std::string text; unsigned long col = 0; bool bold = false; bool right = false; };
struct Rule { int x1 = 0, x2 = 0, y = 0; unsigned long col = C_RULE; };
struct Layout { int w = 0, h = 0; std::vector<Cell> cells; std::vector<Rule> rules; };
// textW(text, bold) = pixel width. Cells are relative to the box's top-left corner.
// (1.1.0) rows: the contract line, the 0DTE line, a thin grey rule, then today's events (one shared 3-column grid)
inline Layout layout(const std::vector<Row>& R, const std::function<int(const std::string&, bool)>& textW,
                     const ContractLine& C = ContractLine(), const ExpiryLine& X = ExpiryLine()) {
    Layout L;
    struct G { std::string a, b, c; unsigned long ca = 0, cb = 0, cc = 0; bool ba = false, bb = false, bc = false; };
    std::vector<G> top;
    if (C.state != CS_NONE) {
        G g; g.a = C.label; g.b = C.text; g.c = C.count; g.ba = true;
        if (C.state == CS_OLD) { g.ca = g.cb = g.cc = C_RED; g.bb = true; }
        else if (C.state == CS_SOON) { g.ca = g.cb = g.cc = C_AMBER; g.bb = g.bc = true; }
        else { g.ca = C_TEXT; g.cb = g.cc = C_DIM; }
        top.push_back(g);
    }
    if (X.state != XS_NONE) {
        G g; g.a = "0DTE"; g.b = X.text; g.c = X.count; g.ba = true;
        if (X.state == XS_NEXT) { g.ca = g.cb = g.cc = C_DIM; }
        else if (X.state == XS_OUT) { g.ca = g.cb = g.cc = C_RED; }
        else if (X.state == XS_SOON) { g.ca = g.cb = g.cc = C_AMBER; g.bb = g.bc = true; }
        else { g.ca = C_TEXT; g.cb = X.big ? C_TEXT : C_DIM; g.cc = C_DIM; g.bb = X.big; }
        top.push_back(g);
    }
    if (top.empty() && R.empty()) return L;
    int w0 = 0, w1 = 0, w2 = 0;
    for (const G& g : top) { w0 = std::max(w0, textW(g.a, g.ba)); w1 = std::max(w1, textW(g.b, g.bb)); w2 = std::max(w2, g.c.empty() ? 0 : textW(g.c, g.bc)); }
    for (const Row& r : R) { const bool b = r.tier == 'H'; w0 = std::max(w0, textW(r.when, b)); w1 = std::max(w1, textW(r.name, b)); w2 = std::max(w2, textW(r.count, b)); }
    const int RULE_H = (!top.empty() && !R.empty()) ? 5 : 0;
    L.w = PAD + w0 + GAP + w1 + (w2 > 0 ? GAP + w2 : 0) + PAD; L.h = PAD / 2 + (int)(top.size() + R.size()) * ROW_H + RULE_H + PAD / 2;
    int y = PAD / 2;
    for (const G& g : top) {
        Cell a; a.x = PAD; a.y = y; a.w = w0; a.h = ROW_H; a.text = g.a; a.col = g.ca; a.bold = g.ba; L.cells.push_back(a);
        Cell n; n.x = PAD + w0 + GAP; n.y = y; n.w = w1 + (g.c.empty() ? GAP + w2 : 0); n.h = ROW_H; n.text = g.b; n.col = g.cb; n.bold = g.bb; L.cells.push_back(n);
        if (!g.c.empty()) { Cell k; k.x = PAD + w0 + GAP + w1 + GAP; k.y = y; k.w = w2; k.h = ROW_H; k.text = g.c; k.col = g.cc; k.bold = g.bc; k.right = true; L.cells.push_back(k); }
        y += ROW_H;
    }
    if (RULE_H) { Rule r; r.x1 = PAD; r.x2 = L.w - PAD; r.y = y + RULE_H / 2; L.rules.push_back(r); y += RULE_H; }
    for (size_t i = 0; i < R.size(); ++i) {
        const Row& r = R[i]; const bool b = r.tier == 'H';
        const unsigned long c = r.state == ST_OUT ? C_RED : r.state == ST_SOON ? C_AMBER : C_TEXT;
        Cell a; a.x = PAD; a.y = y; a.w = w0; a.h = ROW_H; a.text = r.when; a.col = r.state == ST_LATER ? C_DIM : c; a.bold = b; L.cells.push_back(a);
        Cell n; n.x = PAD + w0 + GAP; n.y = y; n.w = w1; n.h = ROW_H; n.text = r.name; n.col = c; n.bold = b; L.cells.push_back(n);
        Cell k; k.x = PAD + w0 + GAP + w1 + GAP; k.y = y; k.w = w2; k.h = ROW_H; k.text = r.count; k.col = c; k.bold = b; k.right = true; L.cells.push_back(k);
        y += ROW_H;
    }
    return L;
}
inline std::string statusText(const std::string& mkt, const std::string& srcWhy, const std::vector<Row>& R, const std::string& placeWhy,
                              const ContractLine& C = ContractLine(), const ExpiryLine& X = ExpiryLine(), const std::string& chartSym = std::string()) {
    std::string s = std::string("lsEventCal ") + VERSION + "\nVERSION," + VERSION + "\nMARKET," + mkt + "\nSOURCE," + srcWhy + "\nPLACE," + placeWhy + "\n";
    static const char* const CS[] = { "none", "ok", "roll soon", "old month", "ahead" };
    static const char* const XS[] = { "none", "later", "last hour", "expired", "next day" };
    s += "CONTRACT," + std::string(CS[C.state]) + ",chart " + (chartSym.empty() ? "-" : chartSym) + "," + C.label + "," + C.text + "," + C.count + "\n";
    // (1.1.0) for the charm research: today's expiry, its kind and the minutes left (negative = expired)
    s += "EXPIRY0DTE," + std::string(XS[X.state]) + "," + (X.state == XS_NONE ? std::string("-") : whenText(X.t)) + "," + kindName(X.kind) + ",minutes left " + std::to_string(X.minsLeft) + "\n";
    for (const Row& r : R) s += "ROW," + r.when + "," + r.name + "," + r.tier + "," + (r.state == ST_OUT ? "out" : r.state == ST_SOON ? "soon" : "later") + "\n";
    if (R.empty()) s += "ROW,none left today\n";
    return s;
}
} // namespace ecal
