// NewsWindow.h 1.0.0 (2026-10-22 evening, Rassul approved the build 17:39 CT): the SHARED, SDK-free NEWS / SETTLEMENT WINDOW and the
// FEED CHECK for every plugin that draws a mark. A mark decided inside a window gets "?" (news-gate skill: a release is context, never a
// reason - sweeps run wider in the window, so a mark there is not proven); when the nightly says the live feed does not match
// (lsFlexLevels\LRA-FeedCheck-<MKT>.csv PARAM,match_ok,0) that market's MBO marks are RECORD ONLY (not drawn).
// Windows (Central time, the PC's clock):
//   - tier-1 US data: CPI / NFP 7:30 AM, -5 / +15 min, all markets; FOMC 1:00 PM, -5 / +30 min (the 1:30 presser inside), all markets
//   - EIA crude inventories Wed 9:30 AM, -5 / +30 min, CL;  EIA natural gas storage Thu 9:30 AM, -5 / +30 min, NG
//   - daily settlement, -5 / +5 min: CL / NG 1:30 PM, GC 12:30 PM, HG 1:00 PM, ES / NQ 3:00 PM, EU (6E) 2:00 PM
//   Dates: the research calendar (analytics\lra\data\events_2025_2026.csv, verified BLS / Fed dates) compiled in below, plus any
//   "EVENT,<yyyy-mm-dd>,<HH:MM>,<CODE>,<ALL|ES|NQ|...>" rows in lsFlexLevels\LRA-News-ALL.csv (VERSION / MARKET,ALL first; the
//   nightly can extend the calendar past 2026 without a rebuild).
// Host use (ONE call per mark): const nw::Gate g = nw::atTm(mkt, tmOfMark); if (g.feedBad) continue; txt = nw::q(txt, g);
#pragma once
#include "MarkParams.h"
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

namespace nw {
static const char* const VERSION = "1.0.0";
struct Gate { bool news = false, feedBad = false; std::string label, feedWhy; int pre = 0, post = 0; };

// tier-1 dates (yyyymmdd, minute of day CT, code)
struct Ev { int ymd; int min; const char* code; };
inline const std::vector<Ev>& tier1() {
    static const std::vector<Ev> E = {
        {20250502,450,"NFP"},{20250507,780,"FOMC"},{20250513,450,"CPI"},{20250606,450,"NFP"},{20250611,450,"CPI"},{20250618,780,"FOMC"},
        {20250703,450,"NFP"},{20250715,450,"CPI"},{20250730,780,"FOMC"},{20250801,450,"NFP"},{20250812,450,"CPI"},{20250905,450,"NFP"},
        {20250911,450,"CPI"},{20250917,780,"FOMC"},{20251024,450,"CPI"},{20251029,780,"FOMC"},{20251120,450,"NFP"},{20251210,780,"FOMC"},
        {20251216,450,"NFP"},{20251218,450,"CPI"},{20260109,450,"NFP"},{20260113,450,"CPI"},{20260128,780,"FOMC"},{20260211,450,"NFP"},
        {20260213,450,"CPI"},{20260306,450,"NFP"},{20260311,450,"CPI"},{20260318,780,"FOMC"},{20260403,450,"NFP"},{20260410,450,"CPI"},
        {20260429,780,"FOMC"},{20260508,450,"NFP"},{20260512,450,"CPI"},{20260605,450,"NFP"},{20260610,450,"CPI"},{20260617,780,"FOMC"},
        {20260702,450,"NFP"},{20260714,450,"CPI"},{20260729,780,"FOMC"},{20260807,450,"NFP"},{20260812,450,"CPI"},{20260904,450,"NFP"},
        {20260911,450,"CPI"},{20260916,780,"FOMC"},{20261002,450,"NFP"},{20261014,450,"CPI"},{20261028,780,"FOMC"},{20261106,450,"NFP"},
        {20261110,450,"CPI"},{20261204,450,"NFP"},{20261209,780,"FOMC"},{20261210,450,"CPI"},
    };
    return E;
}
// minutes before / after a release that count as its window
inline void spanOf(const std::string& code, int& pre, int& post) {
    pre = 5; post = 15;
    if (code == "FOMC" || code == "EIA" || code == "EIA_CRUDE" || code == "EIA_GAS" || code == "NG STORAGE") post = 30;
    if (code == "SETTLE") post = 5;
}
inline std::string hm12(int minOfDay) { int h = minOfDay / 60, m = minOfDay % 60; const char* ap = h >= 12 ? "PM" : "AM"; h %= 12; if (h == 0) h = 12; char b[16]; std::snprintf(b, sizeof b, "%d:%02d %s", h, m, ap); return b; }
inline int settleMin(const std::string& mkt) {
    if (mkt == "CL" || mkt == "NG") return 13 * 60 + 30;
    if (mkt == "GC") return 12 * 60 + 30;
    if (mkt == "HG") return 13 * 60;
    if (mkt == "ES" || mkt == "NQ") return 15 * 60;
    if (mkt == "EU") return 14 * 60;
    return -1;
}
// day of week 0 = Sunday for a civil date (Sakamoto)
inline int dow(int y, int m, int d) { static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 }; if (m < 3) y -= 1; return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7; }
inline bool marketIn(const std::string& list, const std::string& mkt) {
    const std::string u = mp::upper(list); if (u.empty() || u == "ALL") return true;
    size_t a = 0; for (;;) { const size_t b = u.find('|', a); if (mp::normMarket(u.substr(a, b == std::string::npos ? std::string::npos : b - a)) == mkt) return true; if (b == std::string::npos) return false; a = b + 1; }
}
// the window this market is in at y-m-d hh:mm (CT); label "CPI 7:30 AM" (empty = none). extra = rows of LRA-News-ALL.csv
inline bool newsAt(const std::string& mktIn, int y, int mo, int d, int minOfDay, std::string* label, const std::vector<std::vector<std::string> >* extra = nullptr, int* preOut = nullptr, int* postOut = nullptr) {
    const std::string mkt = mp::normMarket(mktIn);
    const int ymd = y * 10000 + mo * 100 + d, wd = dow(y, mo, d);
    auto hit = [&](int at, const std::string& code, const char* text) {
        int pre = 0, post = 0; spanOf(code, pre, post);
        if (minOfDay < at - pre || minOfDay > at + post) return false;
        if (label) *label = std::string(text) + " " + hm12(at);
        if (preOut) { *preOut = pre; }
        if (postOut) { *postOut = post; }
        return true;
    };
    if (wd >= 1 && wd <= 5) {
        const std::vector<Ev>& E = tier1();
        for (size_t i = 0; i < E.size(); ++i) if (E[i].ymd == ymd && hit(E[i].min, E[i].code, E[i].code)) return true;
        if (extra) for (size_t i = 0; i < extra->size(); ++i) {
            const std::vector<std::string>& r = (*extra)[i]; if (r.size() < 4 || r[0] != "EVENT") continue;
            int yy = 0, mm = 0, dd = 0, hh = 0, mi = 0;
            if (std::sscanf(r[1].c_str(), "%d-%d-%d", &yy, &mm, &dd) != 3 || std::sscanf(r[2].c_str(), "%d:%d", &hh, &mi) != 2) continue;
            if (yy * 10000 + mm * 100 + dd != ymd || hh < 0 || hh > 23 || mi < 0 || mi > 59) continue;
            if (!marketIn(r.size() >= 5 ? r[4] : std::string("ALL"), mkt)) continue;
            const std::string code = mp::upper(r[3]); if (code.empty() || code.size() > 16) continue;
            if (hit(hh * 60 + mi, code, code.c_str())) return true;
        }
        if (mkt == "CL" && wd == 3 && hit(9 * 60 + 30, "EIA", "EIA")) return true;
        if (mkt == "NG" && wd == 4 && hit(9 * 60 + 30, "EIA", "Storage")) return true;
        const int st = settleMin(mkt); if (st > 0 && hit(st, "SETTLE", "Settle")) return true;
    }
    return false;
}
// the feed check: the nightly's LRA-FeedCheck-<MKT>.csv says PARAM,match_ok,0 (a file older than 4 days is ignored)
inline bool feedBadOf(const mp::Params& P, long long fileAgeSec, std::string* why) {
    if (!P.ok) return false;
    if (fileAgeSec > 4LL * 86400) { if (why) *why = "feed check older than 4 days - ignored"; return false; }
    std::map<std::string, std::string>::const_iterator it = P.kv.find("MATCH_OK"); if (it == P.kv.end()) return false;
    if (mp::trim(it->second) == "0") { if (why) *why = "the nightly feed check says the live MBO feed does not match"; return true; }
    return false;
}
inline bool feedBad(const std::string& mkt, std::string* why) {
    const mp::Params P = mp::get("FeedCheck", mkt);
    const long long age = P.stampMt > 0 ? (long long)std::time(nullptr) - P.stampMt : 0;
    return feedBadOf(P, age, why);
}
inline Gate atCivil(const std::string& mkt, int y, int mo, int d, int hh, int mi) {
    Gate g; const mp::Params X = mp::get("News", "ALL");
    g.news = newsAt(mkt, y, mo, d, hh * 60 + mi, &g.label, X.ok ? &X.rows : nullptr, &g.pre, &g.post);
    g.feedBad = feedBad(mkt, &g.feedWhy);
    return g;
}
inline Gate atTm(const std::string& mkt, const struct tm& t) { return atCivil(mkt, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min); }
// CT wall-clock seconds (days since 1970-01-01 * 86400 + seconds of the day, the plugins' ctSec / absEnd)
inline Gate atCtSec(const std::string& mkt, long long s) {
    if (s <= 0) { Gate g; g.feedBad = feedBad(mkt, &g.feedWhy); return g; }
    long long z = s / 86400 + 719468; const long long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const long long yy = (long long)yoe + era * 400; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp_ = (5 * doy + 2) / 153; const unsigned d = doy - (153 * mp_ + 2) / 5 + 1; const unsigned m = mp_ < 10 ? mp_ + 3 : mp_ - 9;
    const int sec = (int)(s % 86400);
    return atCivil(mkt, (int)(yy + (m <= 2)), (int)m, (int)d, sec / 3600, (sec / 60) % 60);
}
// "Sr" -> "Sr?" inside a window (never "??")
inline std::string q(const std::string& s, const Gate& g) { if (!g.news || (!s.empty() && s[s.size() - 1] == '?')) return s; return s + "?"; }
} // namespace nw
