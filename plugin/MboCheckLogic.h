#ifndef LS_MBO_CHECK_LOGIC_H
#define LS_MBO_CHECK_LOGIC_H
// MboCheckLogic.h (1.0.0, 2026-10-10) -- the SDK-free half of lsMboCheck: counting the order-by-order (DBO / MBO) events,
// the verdict (YES / NO / subscribed but silent / waiting), the one chart line and the status file text.
// Tested on Linux by tests_mc100/test_mbocheck_logic.cpp; the plugin (MboCheck.cpp) only feeds it what the SDK hands over.
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>

namespace mbo {

static const char* const VERSION = "1.1.0";     // keep equal to MC_VERSION / setVersion in MboCheck.cpp
static const int SLICE_MAX = 5000;               // at most this many getNext() events per call (never blocks IRT)
static const long long SLICE_BUDGET_MS = 8;      // and at most ~8 ms per call, checked every 512 events
static const long long WAIT_MS = 60000;          // "waiting" for the first 60 s
static const long long STATUS_EVERY_MS = 30000;  // status file: on change, else at most every 30 s
static const int SAMPLE_MAX = 20;                // raw events kept for the status file
static const int DEPTH_READ_MAX = 200;           // levels read from MARKET_DEPTH per sample (the SDK reports up to 192)

enum Action { A_NEW = 0, A_MOD = 1, A_DEL = 2, A_FILL = 3, A_OTHER = 4, A_N = 5 };
enum Side { S_SELL = 0, S_BUY = 1, S_UNK = 2, S_N = 3 };

inline int actionOf(unsigned char a) { return a <= 3 ? (int)a : A_OTHER; }
inline int sideOf(unsigned char b) { return b == 1 ? S_BUY : b == 0 ? S_SELL : S_UNK; }

struct Cells {
    long long c[A_N][S_N];
    Cells() { clear(); }
    void clear() { std::memset(c, 0, sizeof(c)); }
    long long action(int a) const { long long t = 0; for (int s = 0; s < S_N; s++) t += c[a][s]; return t; }
    long long total() const { long long t = 0; for (int a = 0; a < A_N; a++) t += action(a); return t; }
    void add(const Cells& o) { for (int a = 0; a < A_N; a++) for (int s = 0; s < S_N; s++) c[a][s] += o.c[a][s]; }
};

// rolling 60 s by arrival second: 60 one-second buckets, each stamped with its second
struct Rolling {
    Cells b[60];
    long long sec[60];
    Rolling() { for (int i = 0; i < 60; i++) sec[i] = -1; }
    void add(long long s, int a, int sd, long long n = 1)
    {
        if (s < 0) return;
        int i = (int)(s % 60);
        if (sec[i] != s) { b[i].clear(); sec[i] = s; }
        b[i].c[a][sd] += n;
    }
    Cells sum(long long nowSec) const
    {
        Cells t;
        for (int i = 0; i < 60; i++) if (sec[i] >= 0 && nowSec - sec[i] < 60 && nowSec >= sec[i]) t.add(b[i]);
        return t;
    }
};

struct Sample {
    std::string when;                   // 12-hour CT time of the event's own timestamp
    int action = 0, side = 0;
    double price = 0, prev = 0, bid = 0, ask = 0;
    long long size = 0, bidsize = 0, asksize = 0;
    long long orderID = 0, aggressorID = 0;
};

enum Verdict { V_WAITING = 0, V_YES, V_YES_IDLE, V_SILENT, V_NO, V_BLOCKED };

struct Probe {
    long long loadMs = -1;              // when this chart's check started (steady clock)
    long long windowMs = -1;            // start of the current 60-s wait (restarted on the 2nd try)
    int attempt = 0;                    // 0 = not opened, 1 = DEPTH_BY_ORDER(chart symbol), 2 = 2nd try DEPTH_BY_ORDER(NULL)
    std::string via, dboSymbol;
    int flag = -1;                      // bDboSubscribed: -1 not read yet, 0, 1
    Cells total; Rolling roll;
    long long events = 0, lastEventMs = -1;
    std::string lastEventWhen;
    bool depthAvail = false, depthSeen = false;
    int depthMax = 0, depthLevels = 0;
    double bestBid = 0, bestAsk = 0; long long bestBidSize = 0, bestAskSize = 0;
    std::vector<Sample> samples;
    long long calls = 0, capHits = 0, longestBacklog = 0, faults = 0;
    int backlogRun = 0;                 // consecutive calls that hit the slice cap
    bool blocked = false;               // a crash guard file says IRT died while opening DBO last time
};

// one call's worth of events has been read: n events, cap = the slice cap or the time budget ended the call
inline void noteSlice(Probe& p, int n, bool cap)
{
    p.calls++;
    if (cap) { p.capHits++; p.backlogRun++; if (p.backlogRun > p.longestBacklog) p.longestBacklog = p.backlogRun; }
    else p.backlogRun = 0;
    (void)n;
}
inline void noteEvent(Probe& p, long long nowMs, unsigned char action, unsigned char buySell)
{
    const int a = actionOf(action), s = sideOf(buySell);
    p.total.c[a][s]++; p.events++; p.lastEventMs = nowMs;
    p.roll.add(nowMs / 1000, a, s);
}

inline Verdict verdict(const Probe& p, long long nowMs)
{
    if (p.blocked) return V_BLOCKED;
    if (p.events > 0) return p.roll.sum(nowMs / 1000).total() > 0 ? V_YES : V_YES_IDLE;
    if (p.windowMs < 0 || nowMs - p.windowMs < WAIT_MS) return V_WAITING;
    return p.flag == 1 ? V_SILENT : V_NO;
}
inline const char* verdictName(Verdict v)
{
    switch (v) { case V_WAITING: return "WAITING"; case V_YES: return "YES"; case V_YES_IDLE: return "YES_IDLE";
                 case V_SILENT: return "SUBSCRIBED_NO_EVENTS"; case V_NO: return "NO"; default: return "BLOCKED"; }
}
// colour class for the line: 0 grey, 1 green, 2 red
inline int colourOf(Verdict v) { return v == V_WAITING ? 0 : (v == V_YES || v == V_YES_IDLE) ? 1 : 2; }

inline std::string commas(long long n)
{
    char b[32]; std::snprintf(b, sizeof(b), "%lld", n < 0 ? -n : n);
    std::string d(b), o;
    for (size_t i = 0; i < d.size(); i++) { if (i && (d.size() - i) % 3 == 0) o += ','; o += d[i]; }
    return (n < 0 ? "-" : "") + o;
}
// 12-hour clock, e.g. "9:41 AM" / "9:41:05 AM"; empty for an invalid time
inline std::string twelveHour(int h, int m, int s, bool withSec)
{
    if (h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 60) return std::string();
    int hh = h % 12; if (hh == 0) hh = 12;
    char b[24];
    if (withSec) std::snprintf(b, sizeof(b), "%d:%02d:%02d %s", hh, m, s, h < 12 ? "AM" : "PM");
    else std::snprintf(b, sizeof(b), "%d:%02d %s", hh, m, h < 12 ? "AM" : "PM");
    return b;
}
inline const char* actionName(int a) { static const char* N[A_N] = { "NEW", "MOD", "DEL", "FILL", "OTHER" }; return (a >= 0 && a < A_N) ? N[a] : "?"; }
inline const char* sideName(int s) { return s == S_BUY ? "BUY" : s == S_SELL ? "SELL" : "UNKNOWN"; }

inline std::string depthText(const Probe& p)
{
    if (!p.depthSeen) return "depth ?";
    if (!p.depthAvail) return "no depth";
    return "depth " + std::to_string(p.depthLevels) + " lvls" + (p.depthLevels == 0 ? " (empty)" : "");
}

// the ONE line on the chart. nowText = the 12-hour CT time now ("9:41 AM")
inline std::string lineText(const Probe& p, long long nowMs, const std::string& nowText)
{
    const Verdict v = verdict(p, nowMs);
    std::string s;
    switch (v) {
    case V_YES: {
        Cells r = p.roll.sum(nowMs / 1000);
        s = "MBO: YES " + commas(r.total()) + " ev/min (N " + commas(r.action(A_NEW)) + " M " + commas(r.action(A_MOD)) + " D "
            + commas(r.action(A_DEL)) + " F " + commas(r.action(A_FILL)) + ")";
        break;
    }
    case V_YES_IDLE: s = "MBO: YES, idle (" + commas(p.events) + " events since load)"; break;
    case V_WAITING: {
        long long left = p.windowMs < 0 ? WAIT_MS / 1000 : (WAIT_MS - (nowMs - p.windowMs) + 999) / 1000;
        if (left < 0) left = 0;
        s = "MBO: waiting " + std::to_string(left) + "s" + (p.attempt >= 2 ? " (2nd try)" : "");
        break;
    }
    case V_SILENT: s = "MBO: subscribed but no events"; break;
    case V_NO: s = p.flag == 0 ? "MBO: NO (not subscribed)" : "MBO: NO (no events)"; break;
    default: s = "MBO: not checked (IRT closed while opening it last time)"; break;
    }
    s += " | " + depthText(p);
    if ((v == V_YES || v == V_YES_IDLE) && !p.lastEventWhen.empty()) s += " | last " + p.lastEventWhen;
    else if (!nowText.empty()) s += " | " + nowText;
    s += std::string(" | v") + VERSION;
    return s;
}

// what makes the status file "changed" (written at once); everything else waits for the 30-s refresh
inline std::string changeKey(const Probe& p, long long nowMs)
{
    return std::string(verdictName(verdict(p, nowMs))) + "|" + std::to_string(p.flag) + "|" + std::to_string(p.attempt) + "|"
        + (p.depthSeen ? (p.depthAvail ? "A" : "N") : "?") + std::to_string(p.depthLevels) + "|" + std::to_string(p.depthMax)
        + "|" + (p.samples.size() >= (size_t)SAMPLE_MAX ? "S" : "s");
}
inline bool statusDue(const std::string& key, const std::string& lastKey, long long nowMs, long long lastWriteMs)
{
    if (lastWriteMs < 0) return true;
    if (key != lastKey) return nowMs - lastWriteMs >= 1000 || nowMs < lastWriteMs;    // a change: at once (at most 1 per s)
    return nowMs - lastWriteMs >= STATUS_EVERY_MS || nowMs < lastWriteMs;
}

inline std::string cellsLine(const Cells& c)
{
    std::string s;
    for (int a = 0; a < A_N; a++) {
        if (a) s += ",";
        s += std::string(actionName(a)) + " " + commas(c.action(a)) + " (buy " + commas(c.c[a][S_BUY]) + " sell " + commas(c.c[a][S_SELL])
             + " unknown " + commas(c.c[a][S_UNK]) + ")";
    }
    return s;
}
inline std::string px(double v)
{
    if (!(v == v) || v > 1e12 || v < -1e12) return "nan";
    char b[40]; std::snprintf(b, sizeof(b), "%.6f", v);
    std::string s(b);
    while (s.size() > 1 && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
    return s;
}
// keep only printable ASCII (the SDK's symbol field is a fixed 24-char buffer, maybe unterminated)
inline std::string cleanField(const char* p, size_t n)
{
    std::string s;
    for (size_t i = 0; i < n && p[i]; i++) { unsigned char c = (unsigned char)p[i]; if (c >= 32 && c < 127 && c != ',') s += (char)c; }
    return s;
}

struct Ident { std::string market, symbol; int spb = 0; };

// the status file (CSV-ish, one fact per line). No user name, login, account or path is ever written.
inline std::string statusText(const Probe& p, const Ident& id, long long nowMs, const std::string& checkedWhen, const std::string& line)
{
    const Verdict v = verdict(p, nowMs);
    std::string o;
    o += std::string("VERSION,") + VERSION + "\n";
    o += "MARKET," + id.market + "\nSYMBOL," + id.symbol + "\nSECONDS_PER_BAR," + std::to_string(id.spb) + "\n";
    o += "CHECKED," + checkedWhen + " CT\n";
    o += std::string("VERDICT,") + verdictName(v) + "\n";
    o += "LINE," + line + "\n";
    o += "DBO_FLAG," + std::string(p.flag < 0 ? "unknown" : p.flag ? "1 (bDboSubscribed set: IRT accepted the subscription)" : "0 (bDboSubscribed not set: IRT refused the subscription)") + "\n";
    o += "DBO_OPENED_WITH," + (p.via.empty() ? std::string("not opened") : p.via) + "\n";
    o += "DBO_SYMBOL," + p.dboSymbol + "\n";
    o += "SECONDS_SINCE_LOAD," + std::to_string(p.loadMs < 0 ? 0 : (nowMs - p.loadMs) / 1000) + "\n";
    o += "EVENTS_TOTAL," + commas(p.events) + "\n";
    o += "EVENTS_60S," + commas(p.roll.sum(nowMs / 1000).total()) + "\n";
    o += "LAST_EVENT," + (p.lastEventMs < 0 ? std::string("none") : p.lastEventWhen + " CT (" + std::to_string((nowMs - p.lastEventMs) / 1000) + " s ago)") + "\n";
    o += "COUNTS_60S," + cellsLine(p.roll.sum(nowMs / 1000)) + "\n";
    o += "COUNTS_TOTAL," + cellsLine(p.total) + "\n";
    o += "DEPTH," + std::string(!p.depthSeen ? "not read yet" : p.depthAvail ? "available" : "NOT available") + ",max_levels "
         + std::to_string(p.depthMax) + ",populated_levels " + std::to_string(p.depthLevels) + ",best_bid " + px(p.bestBid) + " x "
         + commas(p.bestBidSize) + ",best_ask " + px(p.bestAsk) + " x " + commas(p.bestAskSize) + "\n";
    o += "READS,calls " + commas(p.calls) + ",slice_cap_hits " + commas(p.capHits) + ",longest_backlog_calls " + commas(p.longestBacklog)
         + ",faults " + commas(p.faults) + ",slice_max " + std::to_string(SLICE_MAX) + "\n";
    o += "SAMPLE_COLUMNS,n,time,action,side,price,previous_price,size,order_id,aggressor_order_id,bid,ask,bid_size,ask_size\n";
    for (size_t i = 0; i < p.samples.size(); i++) {
        const Sample& e = p.samples[i];
        o += "SAMPLE," + std::to_string(i + 1) + "," + e.when + "," + actionName(e.action) + "," + sideName(e.side) + "," + px(e.price) + ","
             + px(e.prev) + "," + std::to_string(e.size) + "," + std::to_string(e.orderID) + "," + std::to_string(e.aggressorID) + ","
             + px(e.bid) + "," + px(e.ask) + "," + std::to_string(e.bidsize) + "," + std::to_string(e.asksize) + "\n";
    }
    return o;
}

// "ES" from the chart's root / symbol (his GC chart's root is QGC; ES is EP..., HG is CPE..., 6E is EU6...)
inline std::string marketOf(const std::string& rootIn, const std::string& symIn)
{
    for (int pass = 0; pass < 2; pass++) {
        std::string r; for (char ch : (pass ? symIn : rootIn)) r += (char)((ch >= 'a' && ch <= 'z') ? ch - 32 : ch);
        auto has = [&](const char* k) { return r.find(k) != std::string::npos; };
        if (r.empty()) continue;
        if (has("NQ")) return "NQ";
        if (has("GC")) return "GC";
        if (has("CL") || r == "QM") return "CL";
        if (has("HG") || has("CP")) return "HG";
        if (has("NG") || r == "QG") return "NG";
        if (has("EU") || has("6E") || has("E6")) return "EU";
        if (has("ES") || has("EP")) return "ES";
    }
    std::string r; for (char ch : (rootIn.empty() ? symIn : rootIn)) if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) r += ch;
    if (r.size() > 12) r.resize(12);
    return r.empty() ? std::string("UNKNOWN") : r;
}
inline std::string fileStem(const std::string& mkt, int spb) { return mkt + "-" + std::to_string(spb > 0 ? spb : 0); }

} // namespace mbo
#endif
