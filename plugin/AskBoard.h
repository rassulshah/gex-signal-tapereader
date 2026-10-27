#ifndef LS_ASKBOARD_H
#define LS_ASKBOARD_H
// AskBoard.h 0.1.0 (2026-10-26) -- the ASK capability of the Market Awareness Layer, SDK-free (unit-tested on Linux, HB_FAKE board).
// Rassul 2026-10-26 13:43 / 13:44 CT: "the indicators ... be able to ask for information that they need to make a better decision" / "build".
// Design: ASK-DESIGN.md. Three parts:
//   rx::   the RAW-DATA INDEX: each owner posts its raw rows by PRICE (lots / orders / delta / volume / bid-ask / events at a price)
//          for the bar it decided. Any teammate LOOKS UP a fact instantly. Only DATA ("D.") and METRIC ("M.") fields are indexed:
//          an interpretation can never be stored here. Two slots per owner (bar parity) so a reader deciding bar t never sees t+1.
//   ask::  the REQUEST / ANSWER channel for questions only the owner can work out (e.g. "was the big offer at 7,712 pulled or filled?").
//          The asker posts Q (id, topic, args, asked bar, deadline bar); the owner answers on its next pass with facts it decided
//          for a bar >= the asked bar; the asker decides with what it has, marks its read PENDING (never drawn) and makes it FINAL when
//          the answer arrives or the deadline passes. A final read is decided at the bar it became final: nothing already drawn changes.
//   askp:: the per-market QUESTION LISTS (LRA-Ask-<MKT>.csv, written by the nightly lra.info_value): which topics each indicator asks in
//          which context. Ships OFF: an indicator asks nothing until "SWITCH,<asker>,1" (Rassul, after the nightly proves it helps).
// PERF (PerfGuard.h, BUILD rule 2026-10-26): every ask / index step in a plugin is SMART work - call it only inside
//   if (lsperf::smartAllowed(LSPERF_PLUGIN, sym)) { ... }   (rx::post / rx::read, ask::post / pending / answer / collect).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include "MarketState.h"
#include "ContextRead.h"

namespace rx {

static const char* const VERSION = "0.1.0";
static const size_t MAX_INDEX = 24000;     // env values hold up to 32,767 chars; rows farthest from the last price are dropped first

struct Row { double px = 0; std::map<std::string, std::string> kv; };          // kv keys "D.<k>" / "M.<k>" only
struct Index { bool ok = false; std::string src; long long ms = 0, bar = 0; double lastPx = 0; std::vector<Row> rows; int dropped = 0; };

inline bool isFactKey(const std::string& k) { return k.size() > 2 && k[1] == '.' && (k[0] == 'D' || k[0] == 'M'); }
inline std::string cleanTok(const std::string& s, size_t cap = 40) {
    std::string o; for (char c : s) { if (o.size() >= cap) break; o += (c == ';' || c == '=' || c == '|' || c == '/' || c == ',' || c == ':' || c == '\n' || c == '\r' || c == '^') ? ' ' : c; } return o;
}
inline std::string keyOf(const std::string& src, const std::string& sym, long spb, const std::string& chart, int slot) {
    return bb::key(("LRA_RX_" + src + "_" + std::to_string(slot) + "_").c_str(), sym, spb, chart);
}
// encode: ms|bar|lastPx|nRowsDropped|px:k=v,k=v/px:k=v ... (non-fact keys are rejected = not stored)
inline std::string encode(long long ms, long long bar, double lastPx, std::vector<Row> rows, int& kept) {
    std::stable_sort(rows.begin(), rows.end(), [&](const Row& a, const Row& b) { return std::fabs(a.px - lastPx) < std::fabs(b.px - lastPx); });
    std::string body; kept = 0;
    for (const Row& r : rows) {
        std::string item = mst::num(r.px, 10) + ":"; bool any = false;
        for (const auto& p : r.kv) { if (!isFactKey(p.first)) continue; item += (any ? "," : "") + cleanTok(p.first, 24) + "=" + cleanTok(p.second); any = true; }
        if (!any) continue;
        if (body.size() + item.size() + 64 > MAX_INDEX) break;
        body += (body.empty() ? "" : "/") + item; ++kept;
    }
    return std::to_string(ms) + "|" + std::to_string(bar) + "|" + mst::num(lastPx, 10) + "|" + std::to_string((int)rows.size() - kept) + "|" + body;
}
inline bool decode(const std::string& s, long long nowMs, Index& I) {
    I = Index(); if (s.empty() || s.size() > MAX_INDEX + 128) return false;
    const std::vector<std::string> f = bb::split(s, '|', 5); if (f.size() != 5) return false;
    long long ms = 0, bar = 0, dr = 0; if (!bb::num(f[0], ms) || !bb::num(f[1], bar) || !bb::num(f[3], dr) || ms < 0) return false;
    if (nowMs >= 0 && (nowMs < ms || nowMs - ms > mst::FRESH_MS)) return false;
    char* e = nullptr; I.lastPx = std::strtod(f[2].c_str(), &e);
    size_t a = 0; const std::string& b = f[4];
    while (a < b.size()) {
        size_t z = b.find('/', a); if (z == std::string::npos) z = b.size();
        const std::string it = b.substr(a, z - a); a = z + 1;
        const size_t c = it.find(':'); if (c == std::string::npos) continue;
        Row r; r.px = std::strtod(it.substr(0, c).c_str(), &e);
        size_t q = c + 1;
        while (q < it.size()) {
            size_t w = it.find(',', q); if (w == std::string::npos) w = it.size();
            const std::string kv = it.substr(q, w - q); q = w + 1;
            const size_t eq = kv.find('='); if (eq == std::string::npos) continue;
            const std::string k = kv.substr(0, eq); if (isFactKey(k)) r.kv[k] = kv.substr(eq + 1);
        }
        if (!r.kv.empty()) I.rows.push_back(r);
    }
    I.ms = ms; I.bar = bar; I.dropped = (int)dr; I.ok = true; return true;
}
// slot = bar NUMBER parity (bar time / seconds per bar), so bar t and bar t+1 never share a slot
inline int slotOf(long long bar, long spb) { const long long n = spb > 0 ? bar / spb : bar; return (int)(((n % 2) + 2) % 2); }
// the owner posts its rows for the bar it just decided (slot = bar parity: the previous bar's index stays readable)
inline int post(const std::string& src, const std::string& sym, long spb, const std::string& chart, long long ms, long long bar, double lastPx, const std::vector<Row>& rows) {
    if (sym.empty()) return 0;
    int kept = 0;
    bb::bbSet(keyOf(src, sym, spb, chart, slotOf(bar, spb)), encode(ms, bar, lastPx, rows, kept));
    return kept;
}
inline void clear(const std::string& src, const std::string& sym, long spb, const std::string& chart) {
    if (sym.empty()) return;
    bb::bbSet(keyOf(src, sym, spb, chart, 0), std::string()); bb::bbSet(keyOf(src, sym, spb, chart, 1), std::string());
}
// the newest index decided for a bar <= t (never one decided after t: no look-ahead)
inline Index read(const std::string& src, const std::string& sym, long spb, const std::string& chart, long long nowMs, long long t) {
    Index best; if (sym.empty()) return best;
    for (int slot = 0; slot < 2; ++slot) {
        std::string v; Index I;
        if (bb::bbGet(keyOf(src, sym, spb, chart, slot), v) && decode(v, nowMs, I) && I.bar <= t && (!best.ok || I.bar > best.bar)) { I.src = src; best = I; }
    }
    return best;
}
// LOOKUPS (facts only). at(): the row nearest px within tol (ok=false when none); sum(): a numeric field summed over rows within tol
inline const Row* at(const Index& I, double px, double tol) {
    const Row* b = nullptr; double bd = 1e300;
    for (const Row& r : I.rows) { const double d = std::fabs(r.px - px); if (d <= tol + 1e-9 && d < bd) { bd = d; b = &r; } }
    return b;
}
inline bool fieldD(const Row* r, const std::string& k, double& x) {   // k with its class, e.g. "D.lots"
    if (!r || !isFactKey(k)) return false;
    auto it = r->kv.find(k); if (it == r->kv.end() || it->second == "?") return false;
    char* e = nullptr; x = std::strtod(it->second.c_str(), &e); return e && *e == 0 && x == x;
}
inline bool sum(const Index& I, double px, double tol, const std::string& k, double& total, int& nRows) {
    total = 0; nRows = 0; for (const Row& r : I.rows) { double x; if (std::fabs(r.px - px) <= tol + 1e-9 && fieldD(&r, k, x)) { total += x; ++nRows; } }
    return nRows > 0;
}

}  // namespace rx

namespace ask {

static const char* const VERSION = "0.1.0";
static const size_t MAX_Q = 16, MAX_A = 32;
static const long long Q_TTL_MS = 120000;  // a question older than 2 min is dropped from the owner's queue

// the topics and who owns them (L = instant lookup in the raw index, Q = a request the owner answers)
struct Topic { const char* id; const char* owner; char kind; const char* what; };
static const Topic TOPICS[] = {
    { "LOTS_AT",    "LIQ",  'L', "resting lots + order count at a price (D.lots, D.orders)" },
    { "ORDER_FATE", "LIQ",  'Q', "the big order at a price: resting / pulled / filled + HELD/THRU (D.state, D.lots, D.filled, D.pulled_ms)" },
    { "DELTA_AT",   "DP",   'L', "delta, volume, bid / ask volume at a price (D.delta, D.vol, D.bidv, D.askv, M.delta_x)" },
    { "TRAPPED_AT", "DP",   'Q', "trapped volume beyond a price, and whether it is still offside (D.trapped_lots, D.offside)" },
    { "AGG_AT",     "AGG",  'L', "aggressor lots per side at a price, sweep levels, pace (D.buy_lots, D.sell_lots, D.sweep_lv, M.pace_x)" },
    { "DEF_RELOAD", "BOOK", 'Q', "are the defenders at a price reloading or pulling since the touch (D.added, D.pulled, M.reload_x)" },
    { "ATTEMPT_N",  "CTX",  'L', "how many times this level / zone was tested today (D.tests, D.swept, D.held)" },
    { "LEVEL_NEAR", "CTX",  'L', "key / volume / gamma / big-order levels within a distance (D.kind, D.px, M.dist_atr)" },
    { "GAMMA_NEAR", "GEX",  'L', "gamma strike / level near a price (D.strike, D.gex, M.dist_atr)" },
    { nullptr, nullptr, 0, nullptr } };
inline const Topic* topic(const std::string& id) { for (const Topic* t = TOPICS; t->id; ++t) if (id == t->id) return t; return nullptr; }

struct Q { std::string id, asker, owner, topic, args; long long askedBar = 0, deadlineBar = 0, askedMs = 0; };
enum class AState { PENDING, ANSWERED, EXPIRED };
struct Answer { AState st = AState::PENDING; long long ansBar = 0, latMs = 0; std::map<std::string, std::string> kv; };

inline std::string qid(const std::string& asker, const std::string& tp, long long bar, const std::string& args) {
    unsigned h = 2166136261u; for (unsigned char c : asker + "|" + tp + "|" + std::to_string(bar) + "|" + args) { h ^= c; h *= 16777619u; }
    char b[16]; std::snprintf(b, sizeof(b), "%08x", h); return b;
}
inline std::string qKey(const std::string& owner, const std::string& sym, long spb, const std::string& chart) { return bb::key(("LRA_AQ_" + owner + "_").c_str(), sym, spb, chart); }
inline std::string aKey(const std::string& asker, const std::string& sym, long spb, const std::string& chart) { return bb::key(("LRA_AA_" + asker + "_").c_str(), sym, spb, chart); }
inline std::string encQ(const Q& q) {
    return rx::cleanTok(q.id, 12) + "|" + rx::cleanTok(q.asker, 12) + "|" + rx::cleanTok(q.topic, 16) + "|" + rx::cleanTok(q.args, 60) + "|" +
           std::to_string(q.askedBar) + "|" + std::to_string(q.deadlineBar) + "|" + std::to_string(q.askedMs);
}
inline bool decQ(const std::string& s, const std::string& owner, Q& q) {
    const std::vector<std::string> f = bb::split(s, '|', 7); if (f.size() != 7) return false;
    q = Q(); q.id = f[0]; q.asker = f[1]; q.topic = f[2]; q.args = f[3]; q.owner = owner;
    return bb::num(f[4], q.askedBar) && bb::num(f[5], q.deadlineBar) && bb::num(f[6], q.askedMs) && !q.id.empty();
}
inline std::vector<Q> readQueue(const std::string& owner, const std::string& sym, long spb, const std::string& chart) {
    std::vector<Q> out; std::string v; if (sym.empty() || !bb::bbGet(qKey(owner, sym, spb, chart), v)) return out;
    for (const std::string& e : bb::split(v, '^', 1000)) { Q q; if (decQ(e, owner, q)) out.push_back(q); }
    return out;
}
inline void writeQueue(const std::string& owner, const std::string& sym, long spb, const std::string& chart, const std::vector<Q>& qs) {
    std::string v; for (const Q& q : qs) v += (v.empty() ? "" : "^") + encQ(q);
    bb::bbSet(qKey(owner, sym, spb, chart), v);
}
// the asker posts a question to the topic's owner; returns the question id ("" when the topic is unknown or a lookup)
inline std::string post(const std::string& asker, const std::string& tp, const std::string& args, long long askedBar, int waitBars, long spb,
                        const std::string& sym, const std::string& chart, long long nowMs) {
    const Topic* T = topic(tp); if (!T || T->kind != 'Q' || sym.empty()) return std::string();
    Q q; q.asker = asker; q.owner = T->owner; q.topic = tp; q.args = args; q.askedBar = askedBar;
    q.deadlineBar = askedBar + (long long)std::max(0, std::min(waitBars, 5)) * spb; q.askedMs = nowMs; q.id = qid(asker, tp, askedBar, args);
    std::vector<Q> qs = readQueue(T->owner, sym, spb, chart), keep;
    for (const Q& o : qs) if (o.id != q.id && nowMs - o.askedMs <= Q_TTL_MS && nowMs >= o.askedMs) keep.push_back(o);   // drop stale + duplicate
    keep.push_back(q);
    while (keep.size() > MAX_Q) keep.erase(keep.begin());                                                                 // oldest first
    writeQueue(T->owner, sym, spb, chart, keep);
    return q.id;
}
// the owner's open questions it can answer with facts decided for ownerBar (asked at or before it, deadline not passed)
inline std::vector<Q> pending(const std::string& owner, const std::string& sym, long spb, const std::string& chart, long long nowMs, long long ownerBar) {
    std::vector<Q> out;
    for (const Q& q : readQueue(owner, sym, spb, chart))
        if (nowMs - q.askedMs <= Q_TTL_MS && nowMs >= q.askedMs && q.askedBar <= ownerBar && ownerBar <= q.deadlineBar) out.push_back(q);
    return out;
}
// the owner answers with FACTS it decided for ansBar (only D./M. keys are delivered; anything else is dropped), then removes the question
inline bool answer(const Q& q, long long ansBar, const std::map<std::string, std::string>& facts, const std::string& sym, long spb,
                   const std::string& chart, long long nowMs) {
    if (sym.empty() || ansBar < q.askedBar || ansBar > q.deadlineBar) return false;
    std::string body; for (const auto& p : facts) if (rx::isFactKey(p.first)) body += (body.empty() ? "" : ",") + rx::cleanTok(p.first, 24) + "=" + rx::cleanTok(p.second);
    const std::string e = rx::cleanTok(q.id, 12) + "|" + std::to_string(ansBar) + "|" + std::to_string(nowMs) + "|" + std::to_string(q.askedMs) + "|" + body;
    std::string v; std::vector<std::string> as;
    if (bb::bbGet(aKey(q.asker, sym, spb, chart), v)) for (const std::string& x : bb::split(v, '^', 1000)) if (!x.empty() && x.rfind(rx::cleanTok(q.id, 12) + "|", 0) != 0) as.push_back(x);
    as.push_back(e); while (as.size() > MAX_A) as.erase(as.begin());
    std::string w; for (const std::string& x : as) w += (w.empty() ? "" : "^") + x;
    bb::bbSet(aKey(q.asker, sym, spb, chart), w);
    std::vector<Q> qs = readQueue(q.owner, sym, spb, chart), keep; for (const Q& o : qs) if (o.id != q.id) keep.push_back(o);
    writeQueue(q.owner, sym, spb, chart, keep);
    return true;
}
// the asker collects: ANSWERED (facts), PENDING (keep waiting) or EXPIRED (deadline passed: decide without it)
inline Answer collect(const std::string& asker, const std::string& id, long long deadlineBar, long long currentBar, const std::string& sym, long spb,
                      const std::string& chart) {
    Answer A; std::string v;
    if (!sym.empty() && bb::bbGet(aKey(asker, sym, spb, chart), v))
        for (const std::string& x : bb::split(v, '^', 1000)) {
            const std::vector<std::string> f = bb::split(x, '|', 5); if (f.size() != 5 || f[0] != id) continue;
            long long ab = 0, ms = 0, qms = 0; if (!bb::num(f[1], ab) || !bb::num(f[2], ms) || !bb::num(f[3], qms)) continue;
            if (ab > currentBar) continue;                            // decided after the asker's current bar: not visible yet (no look-ahead)
            for (const std::string& kv : bb::split(f[4], ',', 100)) { const size_t eq = kv.find('='); if (eq != std::string::npos && rx::isFactKey(kv.substr(0, eq))) A.kv[kv.substr(0, eq)] = kv.substr(eq + 1); }
            A.st = AState::ANSWERED; A.ansBar = ab; A.latMs = ms - qms; return A;
        }
    A.st = currentBar > deadlineBar ? AState::EXPIRED : AState::PENDING; return A;
}

// ---- a PENDING read: decided without the answers (never drawn), made FINAL once all answers are in or the deadline passed
struct Pending {
    std::string asker; long long t = 0, deadline = 0; std::vector<std::string> qids, topics; cr::Read without; bool final_ = false;
};
inline std::string stateWord(AState s) { return s == AState::ANSWERED ? "answered" : s == AState::EXPIRED ? "expired" : "pending"; }
// ASK record (one per question when it resolves): ASK|1|<asker>|<qid>|<topic>|<asked bar>|<state>|<answer bar>|<latency ms>
inline std::string askLine(const std::string& asker, const std::string& id, const std::string& tp, long long askedBar, const Answer& A) {
    return "ASK|1|" + rx::cleanTok(asker, 12) + "|" + rx::cleanTok(id, 12) + "|" + rx::cleanTok(tp, 16) + "|" + std::to_string(askedBar) + "|" + stateWord(A.st) + "|" +
           (A.st == AState::ANSWERED ? std::to_string(A.ansBar) : std::string("-")) + "|" + (A.st == AState::ANSWERED ? std::to_string(A.latMs) : std::string("-"));
}
// ASKR record (one per asking read when it becomes FINAL): the read WITHOUT the answers vs WITH them, for the nightly value test
// ASKR|1|<asker>|<t>|<final bar>|<answered>/<asked>|<topics>|<w/o side>|<w/o conf>|<with side>|<with conf>|<why>
inline std::string askrLine(const Pending& P, long long finalBar, int answered, const cr::Read& with) {
    std::string tp; for (const std::string& x : P.topics) tp += (tp.empty() ? "" : "+") + x;
    return "ASKR|1|" + rx::cleanTok(P.asker, 12) + "|" + std::to_string(P.t) + "|" + std::to_string(finalBar) + "|" + std::to_string(answered) + "/" +
           std::to_string(P.qids.size()) + "|" + (tp.empty() ? std::string("-") : rx::cleanTok(tp, 60)) + "|" + std::to_string(P.without.side) + "|" +
           std::to_string(P.without.conf) + "|" + std::to_string(with.side) + "|" + std::to_string(with.conf) + "|" + mst::clean(with.reasons, 120);
}

// ---- the record file the nightly reads (lra.info_value): <flex>/Context-ASK-<MKT>-<spb>s-<yyyymmdd>.rec, one line per ASK / ASKR record
inline std::string recName(const std::string& market, long spb, long long yyyymmdd) {
    std::string m; for (char c : market) if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') m += c;
    return "Context-ASK-" + (m.empty() ? std::string("X") : m) + "-" + std::to_string(spb > 0 ? spb : 0) + "s-" + std::to_string(yyyymmdd) + ".rec";
}
inline bool appendRecord(const std::string& dir, const std::string& name, const std::string& line) {
    if (dir.empty() || name.empty() || line.empty()) return false;
    const char s = (dir.find('\\') != std::string::npos) ? '\\' : '/';
    FILE* f = std::fopen((dir + s + name).c_str(), "ab"); if (!f) return false;
    const bool ok = std::fputs((line + "\r\n").c_str(), f) >= 0; std::fclose(f); return ok;
}

}  // namespace ask

namespace askp {

static const char* const VERSION = "0.1.0";
// LRA-Ask-<MKT>.csv (written by lra.info_value; SWITCH lines are Rassul's):
//   "VERSION,1" / "MARKET,ES" / "SWITCH,DP,<0|1|2>" / "ASK,DP,<context>,<topic>,<wait bars>,<P|C>"   (context "*" = any)
//   SWITCH 0 = OFF (default: asks nothing), 1 = ON (asks the PROVEN 'P' questions and uses the answers in its drawn read),
//   2 = SHADOW (asks P + candidate 'C' questions and records the read with / without the answers, but DRAWS the read without them).
//   A question line without a status is a candidate. Context = "<rth|eth>_<with|counter>" (session x trend side of the turn).
struct Item { std::string topic; int wait = 1; bool proven = false; };
struct Plan { std::map<std::string, int> mode; std::map<std::string, std::vector<Item>> lists; std::string note = "defaults (asking OFF)"; };
inline std::string listKey(const std::string& asker, const std::string& ctx) { return asker + "|" + ctx; }
inline Plan parse(const std::string& text, const std::string& market) {
    Plan P; if (text.empty()) return P; std::string mkt; size_t a = 0; int n = 0;
    while (a <= text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        std::string line = text.substr(a, e - a); a = e + 1; if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> f = bb::split(line, ',', 7);
        if (f.size() >= 2 && f[0] == "MARKET") mkt = f[1];
        else if (f.size() >= 3 && f[0] == "SWITCH") P.mode[f[1]] = (f[2] == "1") ? 1 : (f[2] == "2") ? 2 : 0;
        else if (f.size() >= 4 && f[0] == "ASK" && ask::topic(f[3])) {
            long long w = 1; if (f.size() >= 5) bb::num(f[4], w);
            Item it; it.topic = f[3]; it.wait = (int)std::max(0LL, std::min(w, 5LL)); it.proven = f.size() >= 6 && f[5] == "P";
            P.lists[listKey(f[1], f[2])].push_back(it); ++n;
        }
        if (e >= text.size()) break;
    }
    if (!mkt.empty() && mkt != market) { Plan D; D.note = "file is for " + mkt + ", not " + market + " - defaults"; return D; }
    P.note = std::to_string(n) + " question lines"; return P;
}
inline int mode(const Plan& P, const std::string& asker) { auto it = P.mode.find(asker); return it == P.mode.end() ? 0 : it->second; }
inline bool on(const Plan& P, const std::string& asker) { return mode(P, asker) != 0; }
inline bool shadow(const Plan& P, const std::string& asker) { return mode(P, asker) == 2; }
// the questions an asker asks in a context: its context list + the "*" list. OFF: none; ON: proven only; SHADOW: proven + candidates
inline std::vector<std::pair<std::string, int>> questions(const Plan& P, const std::string& asker, const std::string& ctx) {
    std::vector<std::pair<std::string, int>> out; const int m = mode(P, asker); if (m == 0) return out;
    for (const std::string& c : { ctx, std::string("*") }) {
        auto it = P.lists.find(listKey(asker, c)); if (it == P.lists.end()) continue;
        for (const Item& q : it->second) {
            if (m == 1 && !q.proven) continue;
            bool dup = false; for (const auto& o : out) dup = dup || o.first == q.topic;
            if (!dup) out.push_back({ q.topic, q.wait });
        }
    }
    return out;
}
// the read the asker DRAWS once final: with the answers when ON, without them in SHADOW (the 'with' read is only recorded)
inline const cr::Read& drawn(const Plan& P, const std::string& asker, const cr::Read& without, const cr::Read& with) { return shadow(P, asker) ? without : with; }

}  // namespace askp
#endif
