// TopSignals.h -- the HOD / LOD box top line's signal badges (lsHodLod 2.35.0), SDK-free
//
// (2.35.0, Rassul 2026-11-19 17:27-18:10 CT "lets include sweeps too", "build" on the suggestion: the right end of the market strength
// line shows, for EACH market, its latest Cob (the Rev after a drawn Cob), latest Sw and latest BkTst (the Rev after a Tst of a Bk) -
// newest first, green / red badges with black text, e.g. "NQ Sw PHL 1:09", "ES Cob 10:12", "ES BkTst PDL 11:39"; no plain Bk
// (the nightly 11-19: Bk alone -0.20R, Sw +0.09R, Tst+Rev +0.04R). Current session only (from the 5 PM session start).
// Sources (all written by his own indicators, read bounded by the host):
//   lsFlexLevels\RevRead\wy-<MKT>-180.txt  (lsRevRead 0.20.0): "stamp|Sw|dir|lv|px|st", "stamp|Tst|dir|lv|0|62%", "stamp|Rev|dir||0|V35"
//                                          stamp = the candle END in the 1904 epoch, local time (IRT's RTDATE)
//   lsFlexLevels\CobSignal\<MKT>-180.csv   (lsCobSignal 1.10.0): "COB,<bar end local secs>,BULL|BEAR,...,SHOW|LOG",
//                                          "ZZO,<bar end>,BULL|BEAR,<count>,OK|NO,...", "REV,<rev bar end>,BULL|BEAR,<cob bar end>,..."
//   A Cob counts as drawn when its COB line is SHOW and its ZZO count is 10+ (as lsCobSignal draws it).
#pragma once
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cerrno>

namespace tsig {

static const char* const VERSION = "1.2.2";   // (1.2.2, Rassul 2026-11-20 17:22 CT "the cob is the cob wave and i only see 1 on gc but you have listed 3"): one Cob tag per drawn Cob wave at the Cob candle, never one per Rev   // was "1.2.1";   // (1.2.1, Rassul 2026-11-20 16:51 CT "get rid of that rule. keep all sweeps of the session ... latest on the left and the same with cob"): the rows show the whole session (from 5 PM), no 60-min expiry   // was "1.2.0";   // (1.2.0 = lsHodLod 2.42.0, Rassul 2026-11-20 13:22 CT two signal rows "sweep row should have strength like 99", 13:36 "perfect", 14:13 build): each Sig carries its number (Sw strength, Cob ZZO count) and a Cob its swept levels; rowOf() = one row's tags   // was "1.1.0";   // (1.1.0 = lsHodLod 2.37.2, Rassul 2026-11-19 22:53 CT "remove signals after an hour"): a tag shows for 60 min after its candle, aged on the chart's bar clock   // was "1.0.0"
static const long long SHOW_SECS = 3600;      // (1.1.0) a tag is shown while its age < 60 min (59 min shows, 60 min is gone)
static const long long E1904 = 2082844800LL;   // 1904 -> 1970, local
static const size_t MAX_BYTES = 512 * 1024;     // a session's wy / Cob record is a few KB; anything bigger is refused (last good copy kept)
static const int ZZO_MIN = 10;

struct Sig { std::string mkt, type, lv; long long t = 0; int dir = 0; std::string num; };   // (1.2.0) num = "99+" / "38" (Sw strength) or the Cob's ZZO count   // t = candle end, local secs (unix-naive local)

inline bool toLL(const std::string& s, long long& v) {
    if (s.empty() || s.size() > 20) return false;
    errno = 0; char* e = nullptr; const long long x = std::strtoll(s.c_str(), &e, 10);
    if (errno || !e || *e) return false;
    v = x; return true;
}
inline std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> o; std::string cur;
    for (char c : s) { if (c == d) { o.push_back(cur); cur.clear(); } else if (c != '\r') cur += c; }
    o.push_back(cur); return o;
}
// the session the clock is in: from 5 PM local (the session start) on
inline long long sessionStart(long long nowLocal) {
    const long long day = nowLocal >= 0 ? nowLocal / 86400 : (nowLocal - 86399) / 86400;
    const long long five = day * 86400 + 17 * 3600;
    return nowLocal >= five ? five : five - 86400;
}
inline bool cleanLevel(const std::string& s) {
    if (s.empty() || s.size() > 12) return false;
    for (char c : s) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '/')) return false;
    return true;
}

// (1.2.0) a strength token "V99+" / "R38" -> "99+" / "38"; anything else -> ""
inline std::string strengthNum(const std::string& st) {
    if (st.size() < 2 || st.size() > 5 || (st[0] != 'V' && st[0] != 'R')) return std::string();
    std::string n = st.substr(1); const bool plus = !n.empty() && n.back() == '+'; if (plus) n.pop_back();
    long long v = 0; if (!toLL(n, v) || v < 0 || v > 99) return std::string();
    return std::to_string(v) + (plus ? "+" : "");
}
// (1.2.0) a Cob's swept levels "ONH/LonHI/AsiaHI/PHH" -> the first two clean ones, "ONH/LonHI"
inline std::string cobLevels(const std::string& s) {
    std::string o; int n = 0; for (const std::string& p : split(s, '/')) { if (n >= 2 || !cleanLevel(p) || p.find('/') != std::string::npos) continue; o += (n ? "/" : "") + p; ++n; }
    return o;
}
// the Sw and BkTst signals of one market from its wy file, from the session start to now (later candles are ignored)
inline std::vector<Sig> parseWy(const std::string& text, const std::string& mkt, long long from, long long to) {
    std::vector<Sig> out; if (text.size() > MAX_BYTES) return out;
    std::map<int, std::string> lastTst;   // side -> the level of its latest Tst (the Rev after it is the BkTst)
    std::vector<std::string> lines = split(text, '\n');
    struct Row { long long t; std::string code; int dir; std::string lv; std::string st; };
    std::vector<Row> rows;
    for (const std::string& ln : lines) {
        if (ln.empty()) continue;
        const std::vector<std::string> f = split(ln, '|');
        if (f.size() < 3) continue;
        long long st = 0, d = 0; if (!toLL(f[0], st) || !toLL(f[2], d)) continue;
        if (d != 1 && d != -1) continue;
        const long long t = st - E1904; if (t < from || t > to) continue;
        const std::string& c = f[1];
        if (c != "Sw" && c != "Tst" && c != "Rev") continue;
        rows.push_back(Row{ t, c, (int)d, f.size() > 3 ? f[3] : std::string(), f.size() > 5 ? f[5] : std::string() });
    }
    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.t < b.t; });
    for (const Row& r : rows) {
        if (r.code == "Sw") { if (!cleanLevel(r.lv)) continue; Sig s; s.mkt = mkt; s.type = "Sw"; s.lv = r.lv; s.t = r.t; s.dir = r.dir; s.num = strengthNum(r.st); out.push_back(s); }
        else if (r.code == "Tst") { if (cleanLevel(r.lv)) lastTst[r.dir] = r.lv; }
        else {   // Rev: a BkTst only when a Tst of the same side came before it
            const auto it = lastTst.find(r.dir); if (it == lastTst.end()) continue;
            Sig s; s.mkt = mkt; s.type = "BkTst"; s.lv = it->second; s.t = r.t; s.dir = r.dir; out.push_back(s);
            lastTst.erase(it);   // one Rev per Tst
        }
    }
    return out;
}

// (1.2.2) the drawn Cobs of one market from its Cob Signal record (COB SHOW + ZZO 10+), one per Cob wave at its candle, from the session start to now
inline std::vector<Sig> parseCob(const std::string& text, const std::string& mkt, long long from, long long to) {
    std::vector<Sig> out; if (text.size() > MAX_BYTES) return out;
    std::map<long long, int> cobDir; std::map<long long, bool> shown; std::map<long long, long long> zzo; std::map<long long, std::string> cobLv;
    struct Rv { long long t, cob; int dir; }; std::vector<Rv> revs;
    for (const std::string& ln : split(text, '\n')) {
        if (ln.empty() || ln[0] == '#') continue;
        const std::vector<std::string> f = split(ln, ',');
        if (f.size() < 4) continue;
        long long t = 0; if (!toLL(f[1], t)) continue;
        const int d = f[2] == "BULL" ? 1 : f[2] == "BEAR" ? -1 : 0; if (!d) continue;
        if (f[0] == "COB") { cobDir[t] = d; shown[t] = f.back() == "SHOW"; if (f.size() > 4) cobLv[t] = cobLevels(f[4]); }
        else if (f[0] == "ZZO") { long long n = 0; if (toLL(f[3], n)) zzo[t] = n; }
        else if (f[0] == "REV") { long long c = 0; if (!toLL(f[3], c)) continue; if (t >= from && t <= to) revs.push_back(Rv{ t, c, d }); }
    }
    (void)revs;   // (1.2.2) the Revs after a Cob are not Cob signals - the Cob WAVE is (one tag per drawn Cob, at its candle)
    for (const auto& cd : cobDir) {
        const long long t = cd.first; if (t < from || t > to) continue;
        if (!shown[t]) continue;
        const auto z = zzo.find(t); if (z == zzo.end() || z->second < ZZO_MIN) continue;
        Sig s; s.mkt = mkt; s.type = "Cob"; s.t = t; s.dir = cd.second; s.num = std::to_string(z->second); { const auto l = cobLv.find(t); if (l != cobLv.end()) s.lv = l->second; } out.push_back(s);
    }
    return out;
}

// per market and type the latest one; all markets together, newest first
inline std::vector<Sig> latest(const std::vector<Sig>& all) {
    std::map<std::string, Sig> best;
    for (const Sig& s : all) { const std::string k = s.mkt + "|" + s.type; auto it = best.find(k); if (it == best.end() || s.t >= it->second.t) best[k] = s; }
    std::vector<Sig> o; for (const auto& kv : best) o.push_back(kv.second);
    std::stable_sort(o.begin(), o.end(), [](const Sig& a, const Sig& b) { return a.t != b.t ? a.t > b.t : a.mkt < b.mkt; });
    return o;
}

// (1.1.0) the clock a tag is aged on: the chart's newest bar time (the market's own clock: an idle feed or a reload neither keeps nor
// drops tags wrongly); the PC clock only when no bar time is known
inline long long ageClock(long long barNow, long long sysNow) { return barNow > 0 ? barNow : sysNow; }
inline bool fresh(const Sig& s, long long now) { return now - s.t < SHOW_SECS; }
// the tags still shown (order kept; a group with none left is simply empty on the line)
inline std::vector<Sig> freshOnly(const std::vector<Sig>& v, long long now) {
    std::vector<Sig> o; for (const Sig& s : v) if (fresh(s, now)) o.push_back(s); return o;
}

// (1.2.0) one signal row (lsHodLod 2.42.0): every shown tag of a type (Sw or Cob) from all markets, newest first (a tie: by market)
inline std::vector<Sig> rowOf(const std::vector<Sig>& all, const std::string& type, long long now) {
    std::vector<Sig> o; for (const Sig& s : all) if (s.type == type && s.t <= now + 60) o.push_back(s);   // (1.2.1) the whole session, no 60-min expiry
    std::stable_sort(o.begin(), o.end(), [](const Sig& a, const Sig& b) { return a.t != b.t ? a.t > b.t : a.mkt < b.mkt; });
    return o;
}
inline std::string clock12(long long tLocal) {
    long long sod = tLocal % 86400; if (sod < 0) sod += 86400;
    const int h = (int)(sod / 3600), m = (int)((sod % 3600) / 60);
    char b[16]; std::snprintf(b, sizeof b, "%d:%02d", h % 12 == 0 ? 12 : h % 12, m); return b;
}
inline std::string mktLabel(const std::string& m) { return m == "EU" ? "6E" : m; }
// the badge text: "NQ Sw PHL 1:09", "ES Cob 10:12", "ES BkTst PDL 11:39"
inline std::string label(const Sig& s) {
    std::string t = mktLabel(s.mkt) + " " + s.type;
    if (!s.lv.empty() && s.type != "Cob") t += " " + s.lv;
    return t + " " + clock12(s.t);
}

} // namespace tsig
