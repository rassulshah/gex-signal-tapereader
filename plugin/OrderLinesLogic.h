#ifndef LS_ORDERLINESLOGIC_H
#define LS_ORDERLINESLOGIC_H
// OrderLinesLogic.h 1.0.0 (2026-10-26) -- the LIMIT-ORDER LINES of lsLiquidityProfile 0.55.0. SDK-free; tested by
// tests_oln/test_oln.cpp (logic) and tests_oln/test_oln_host.cpp (mock host). Spec: BUILD-SPEC-2026-10-25.md (Rassul's walk-through
// 2026-10-25 20:00-22:29 CT, mockup all-approved-mockup.html, reference renderer stp/final.py).
//
//  - LINE: one big resting order (shown >= the market FLOOR lots, or an iceberg whose shown + traded reaches it), drawn once it (or its
//    rolled chain) has rested REST_MIN (9) minutes, drawn back to its placement time. Orders placed at / after the market's late-day
//    cut-off never start a line (ES / NQ 14:30, CL / NG 13:00, GC / HG 12:00, 6E 13:30 CT).
//  - MOVED (same order id re-priced) and ROLLED (pulled, then a NEW order on the same side with EXACTLY the same size, placed within
//    ROLL_SEC (15) s before or after the pull, within 1 ATR, and the ONLY such candidate) = ONE line that steps to the new price. No tag.
//    A FILL is never joined to a new order. The roll is decided ROLL_SEC after the pull (the pull is not shown until then).
//  - ICEBERG: the same id refilled after fills (size up at the same price) -> label "I R<refills> <total lots filled>".
//  - LADDER: >= 3 smaller orders (LADDER_PIECE_MIN .. FLOOR-1 lots) on one side placed within 1 s, spanning <= LADDER_SPAN ticks, together
//    >= FLOOR lots -> ONE line at the size-weighted average price, label "<n>o <total>", stepping when its pieces move.
//  - END: FILLED (>= half the size it showed traded) = fill dot + ONE word; PULLED = the line turns very dark (no x); LIVE = full colour.
//  - WORD (Rassul 22:01 / 22:13 / 22:16): in the fill bar + the next 3 bars: THRU as soon as a bar trades wholly beyond the order (bid:
//    the bar's HIGH below it; offer: its LOW above it); HELD at the first FAVOURABLE close (bid: a GREEN bar closing above it; offer: a
//    RED bar closing below it); none in 4 bars = THRU. Decided on closed bars only, drawn beside the fill dot, never changed.
//  - ONE WORD PER ZONE (22:09): a word decided within 3 bars of an earlier SAME word on the same side within 1 ATR is not drawn (hover
//    only); among words decided at the SAME bar close the one nearest the pivot extreme wins (earliest on a tie).
//  - RECORD (append-only, read back after a restart, never repainted): N line, S step, E end, W word lines (see header()).
// No look-ahead: everything is decided from events as they arrive and from closed bars.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace oln {
static const char* const VERSION = "1.0.0";
typedef long long Tick;
enum Side { BID = 0, ASK = 1 };
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum Kind { K_ONE = 0, K_LADDER = 1 };
enum End { LIVE = 0, FILLED = 1, PULLED = 2 };
enum Word { W_NONE = 0, W_HELD = 1, W_THRU = 2 };
enum Ink { INK_WHITE = 0, INK_GREEN = 1, INK_RED = 2, INK_GREY = 3 };

// ---- per-market rule (defaults; LRA-LimitLines-<MKT>.csv overrides, written by the nightly reverse-engineering study) -----------
struct Rule {
    long long floorLots = 50; int restMin = 9; int rollSec = 15; double rollAtr = 1.0;
    long long ladPieceMin = 10; int ladMinN = 3; Tick ladSpan = 8; int ladWinMs = 1000;
    int cutMin = 14 * 60 + 30;   // CT minutes of day: no new line placed at / after
    int wordBars = 4;            // the fill bar + 3
    double zoneAtr = 1.0; int zoneBars = 3;
    std::string src = "defaults";
};
inline Rule ruleFor(const std::string& m) {
    Rule r;
    // starting floors = the ES 50-lot line approved on the mockup, others scaled to a similar number of lines a day; the nightly study
    // (LRA-LimitLines-<MKT>.csv) replaces them with what the data proved per market
    if (m == "ES") { r.floorLots = 50; r.ladPieceMin = 10; r.ladSpan = 8; r.cutMin = 14 * 60 + 30; }
    else if (m == "NQ") { r.floorLots = 15; r.ladPieceMin = 3; r.ladSpan = 20; r.cutMin = 14 * 60 + 30; }
    else if (m == "CL") { r.floorLots = 20; r.ladPieceMin = 4; r.ladSpan = 10; r.cutMin = 13 * 60; }
    else if (m == "NG") { r.floorLots = 10; r.ladPieceMin = 2; r.ladSpan = 10; r.cutMin = 13 * 60; }
    else if (m == "GC") { r.floorLots = 10; r.ladPieceMin = 2; r.ladSpan = 10; r.cutMin = 12 * 60; }
    else if (m == "HG") { r.floorLots = 5; r.ladPieceMin = 1; r.ladSpan = 10; r.cutMin = 12 * 60; }
    else if (m == "6E" || m == "EU") { r.floorLots = 20; r.ladPieceMin = 4; r.ladSpan = 10; r.cutMin = 13 * 60 + 30; }
    return r;
}
inline std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> o; std::string c; for (char ch : s) { if (ch == d) { o.push_back(c); c.clear(); } else if (ch != '\r') c += ch; } o.push_back(c); return o;
}
inline std::string trim(const std::string& s) { size_t a = s.find_first_not_of(" \t\""), b = s.find_last_not_of(" \t\"\r"); return a == std::string::npos ? "" : s.substr(a, b - a + 1); }
inline std::string upper(std::string s) { for (auto& c : s) c = (char)std::toupper((unsigned char)c); return s; }
// accepted lines: "KEY,VALUE" or "PARAM,KEY,VALUE" (header / comment / unknown lines skipped). Values out of range keep the default.
inline Rule parseParams(const std::vector<std::string>& lines, const std::string& market) {
    Rule r = ruleFor(market); int got = 0, bad = 0;
    for (const auto& ln : lines) {
        if (ln.empty() || ln[0] == '#') continue;
        std::vector<std::string> f = split(ln, ',');
        for (auto& x : f) x = trim(x);
        if (!f.empty() && upper(f[0]) == "PARAM") f.erase(f.begin());
        if (f.size() < 2) continue;
        const std::string k = upper(f[0]); char* e = nullptr; const double v = std::strtod(f[1].c_str(), &e);
        if (e == f[1].c_str() || !(v == v)) { if (k != "VERSION" && k != "MARKET" && k != "KEY") ++bad; continue; }
        bool ok = true;
        if (k == "FLOOR" && v >= 1 && v <= 100000) r.floorLots = (long long)std::llround(v);
        else if (k == "REST_MIN" && v >= 0 && v <= 240) r.restMin = (int)std::llround(v);
        else if (k == "ROLL_SEC" && v >= 0 && v <= 600) r.rollSec = (int)std::llround(v);
        else if (k == "ROLL_ATR" && v > 0 && v <= 10) r.rollAtr = v;
        else if (k == "LADDER_PIECE_MIN" && v >= 1 && v <= 100000) r.ladPieceMin = (long long)std::llround(v);
        else if (k == "LADDER_MIN_ORDERS" && v >= 2 && v <= 50) r.ladMinN = (int)std::llround(v);
        else if (k == "LADDER_SPAN_TICKS" && v >= 0 && v <= 1000) r.ladSpan = (Tick)std::llround(v);
        else if (k == "WORD_BARS" && v >= 1 && v <= 10) r.wordBars = (int)std::llround(v);
        else if (k == "VERSION" || k == "MARKET" || k == "KEY") continue;
        else ok = false;
        if (ok) ++got; else ++bad;
    }
    char b[96]; std::snprintf(b, sizeof(b), "file: %d values%s", got, bad ? " (some ignored)" : ""); r.src = got ? b : "file empty - defaults";
    return r;
}

struct Seg { long long t = 0; Tick px = 0; };
struct Line {
    int id = 0, side = BID, kind = K_ONE;
    long long t0 = 0, t1 = 0;                 // placement; end (0 = live)
    std::vector<long long> oids;              // the chain (rolls) or the pieces (ladder)
    std::vector<Seg> segs;                    // price over time; segs.back() = now
    long long size = 0, maxShown = 0, traded = 0; int refills = 0; int rolls = 0, moves = 0;
    int end = LIVE; long long endMs = 0; Tick endPx = 0;
    bool drawn = false; long long drawnMs = 0; bool restored = false;
    long long pullDue = 0;                    // a pull waiting for the roll window (0 = none)
    int word = W_NONE; long long wordMs = 0; bool wordShown = false; long long fillMs = 0; Tick fillPx = 0; long long fillBarEnd = 0; int barsSeen = 0;
    long long rollGapMs = -1;                 // the last roll: re-placed this many ms after the pull (negative = before)
    std::string label() const {
        char b[64];
        if (kind == K_LADDER) std::snprintf(b, sizeof(b), "%do %lld", (int)oids.size(), size);
        else if (refills > 0) std::snprintf(b, sizeof(b), "I R%d %lld", refills, std::max(traded, size));
        else std::snprintf(b, sizeof(b), "%lld", std::max(maxShown, size));
        return b;
    }
    Tick px() const { return segs.empty() ? 0 : segs.back().px; }
};
struct Ord {
    int side = BID; Tick px = 0; long long size = 0, maxShown = 0, traded = 0; long long tAdd = 0; int line = -1;
    bool filledSince = false; int refills = 0; int ladder = -1; bool known = true;
    std::vector<Seg> hist;   // its price steps (first + every re-price, at most 24) - a line that starts later keeps them
};
struct Ladder { int side = BID; long long t0 = 0; std::vector<long long> oids; bool closed = false; int line = -1; };
struct PendRoll { int line = -1; int side = BID; Tick px = 0; long long size = 0, t = 0, due = 0; };
struct Bar { long long endMs = 0; Tick o = 0, h = 0, l = 0, c = 0; };
struct Tally { long long lines = 0, drawn = 0, filled = 0, pulled = 0, rolls = 0, rollAmbig = 0, moves = 0, ladders = 0, icebergs = 0,
               held = 0, thru = 0, wordsHidden = 0, lateSkipped = 0, events = 0, unknownFill = 0; };

class Engine {
public:
    Rule R; std::string market;
    std::unordered_map<long long, Ord> book;
    std::vector<Line> lines;
    std::vector<Ladder> ladders;
    std::vector<PendRoll> rolls;
    std::vector<std::pair<long long, long long> > recentNew;   // (t, oid) of NEW orders >= the floor in the last ROLL_SEC (roll candidates)
    std::vector<std::string> rows;            // record lines not yet written
    std::vector<Bar> bars; double atr = 0; long long barMs = 180000;
    Tally tally; long long lastT = 0; int nextId = 1;
    size_t bookCap = 400000;

    static std::string header() { return "# OrderLines 1.0.0 record: N|id|side|kind|t0|px|label  S|id|t|px  E|id|end|t|px|traded  W|id|word|decide_ms|shown"; }
    static int minuteCT(long long ms) { long long m = (ms / 60000LL) % 1440LL; if (m < 0) m += 1440; return (int)m; }
    double atrTicks() const { return atr > 0 ? atr : 8.0; }

    // ---- MBO events ----------------------------------------------------------------------------------------------------------
    void onEvent(int act, int side, Tick px, long long size, long long oid, long long t) {
        if (t <= 0 || oid == 0) return;
        ++tally.events; if (t > lastT) lastT = t;
        dueWork(t);
        auto it = book.find(oid);
        if (act == NEW || (act == MOD && it == book.end())) {
            if (px <= 0 || size <= 0 || (side != BID && side != ASK)) return;
            if (it != book.end()) { moveOrd(oid, it->second, px, size, t); return; }
            if (book.size() >= bookCap) prune(t);
            Ord o; o.side = side; o.px = px; o.size = size; o.maxShown = size; o.tAdd = t; o.known = act == NEW;
            { Seg g; g.t = t; g.px = px; o.hist.push_back(g); }
            book[oid] = o;
            {   auto ro = restoredOid.find(oid);   // a restored live line: the same order again -> follow it, never a second line
                if (ro != restoredOid.end() && lines[(size_t)ro->second].end == LIVE) { Ord& b2 = book[oid]; b2.line = ro->second; Line& L = lines[(size_t)ro->second]; L.restored = false; if (px != L.px()) addSeg(ro->second, t, px); return; } }
            if (size >= R.floorLots) { recentNew.push_back(std::make_pair(t, oid)); if (!tryRollInto(oid, t)) startLine(oid, t, K_ONE); }
            else if (size >= R.ladPieceMin) ladderPiece(oid, t);
            return;
        }
        if (it == book.end()) { if (act == FILL) ++tally.unknownFill; return; }
        Ord& o = it->second;
        if (act == MOD) { moveOrd(oid, o, px, size, t); return; }
        if (act == FILL) {
            if (size <= 0) return;
            o.traded += size; o.size = std::max(0LL, o.size - size); o.filledSince = true;
            if (o.line >= 0) { Line& L = lines[(size_t)o.line]; L.traded += size; L.fillMs = t; L.fillPx = px > 0 ? px : o.px; }
            return;
        }
        if (act == DEL) {
            const bool filled = o.size <= 0 && o.traded > 0;
            if (o.line >= 0) {
                Line& L = lines[(size_t)o.line];
                if (L.kind == K_LADDER) { ladderPieceGone(o.line, oid, t); }
                else if (L.end == LIVE && !L.oids.empty() && L.oids.back() == oid) {
                    if (filled || o.traded * 2 >= std::max<long long>(1, o.maxShown)) endLine(o.line, FILLED, t);
                    else { L.pullDue = t + (long long)R.rollSec * 1000LL; PendRoll p; p.line = o.line; p.side = o.side; p.px = o.px; p.size = o.size; p.t = t; p.due = L.pullDue; rolls.push_back(p); }
                }
            }
            book.erase(it);
        }
    }
    void moveOrd(long long oid, Ord& o, Tick px, long long size, long long t) {
        if (px <= 0) return;
        if (px == o.px) {
            if (o.filledSince && size > o.size) { o.refills++; o.filledSince = false; if (o.line >= 0) { Line& L = lines[(size_t)o.line]; if (L.kind == K_ONE) { L.refills = std::max(L.refills, o.refills); if (L.refills == 1) ++tally.icebergs; } } }
            o.size = size; if (size > o.maxShown) o.maxShown = size;
            if (o.line >= 0) { Line& L = lines[(size_t)o.line]; if (L.kind == K_ONE) { L.size = size; L.maxShown = std::max(L.maxShown, size); } }
            else if (o.ladder < 0 && size >= R.floorLots && o.refills == 0) startLine(oid, t, K_ONE);   // grew to the floor (e.g. 10 -> 50 lots)
            else if (o.ladder < 0 && o.refills > 0 && o.traded + o.size >= R.floorLots && !late(o.tAdd)) startLine(oid, o.tAdd, K_ONE, t);   // an iceberg reaching the floor
            return;
        }
        // re-priced: the same order moved (its line steps)
        o.px = px; o.size = size; o.filledSince = false; if (size > o.maxShown) o.maxShown = size;
        if (o.hist.size() < 24) { Seg g; g.t = t; g.px = px; o.hist.push_back(g); }
        if (o.line >= 0) {
            Line& L = lines[(size_t)o.line];
            if (L.kind == K_LADDER) ladderReprice(o.line, t);
            else if (L.end == LIVE) { addSeg(o.line, t, px); L.moves++; ++tally.moves; }
        } else if (size >= R.floorLots && o.ladder < 0) { if (!tryRollInto(oid, t)) startLine(oid, t, K_ONE); }
    }
    bool late(long long t) const { return minuteCT(t) >= R.cutMin; }
    void startLine(long long oid, long long, int kind, long long tNow = 0) {
        auto it = book.find(oid); if (it == book.end()) return;
        Ord& o = it->second;
        if (late(o.tAdd)) { ++tally.lateSkipped; return; }
        Line L; L.id = nextId++; L.side = o.side; L.kind = kind; L.t0 = o.tAdd; L.oids.push_back(oid);
        if (o.hist.empty()) { Seg s; s.t = o.tAdd; s.px = o.px; L.segs.push_back(s); } else L.segs = o.hist;
        L.size = o.size; L.maxShown = o.maxShown; L.traded = o.traded; L.refills = o.refills;
        o.line = (int)lines.size(); waiting.push_back(o.line); lines.push_back(L); ++tally.lines; (void)tNow;
    }
    void addSeg(int li, long long t, Tick px) {
        Line& L = lines[(size_t)li]; if (L.segs.empty() || L.segs.back().px == px) return;
        Seg s; s.t = t; s.px = px; L.segs.push_back(s);
        if (L.drawn) { char b[128]; std::snprintf(b, sizeof(b), "S|%d|%lld|%lld", L.id, t, (long long)px); rows.push_back(b); }
    }
    // a NEW order (>= floor, exact size) may be the re-placement of an order pulled within ROLL_SEC. Linked only when it is the ONLY
    // candidate in both directions (decided when the pull's window is over - see settleRolls). Here: a pull already waiting.
    bool tryRollInto(long long, long long) { return false; }   // the link is made in settleRolls (uniqueness needs the whole window)
    void endLine(int li, int end, long long t) {
        Line& L = lines[(size_t)li]; if (L.end != LIVE) return;
        L.end = end; L.endMs = t; L.t1 = t; L.endPx = L.px(); L.pullDue = 0;
        if (end == FILLED) { ++tally.filled; if (!L.fillMs) L.fillMs = t; if (!L.fillPx) L.fillPx = L.px(); }
        else ++tally.pulled;
        if (!L.drawn && (t - L.t0) >= (long long)R.restMin * 60000LL && !late(L.t0)) qualify(li, t);   // rested long enough before it ended
        if (L.drawn) { char b[160]; std::snprintf(b, sizeof(b), "E|%d|%s|%lld|%lld|%lld", L.id, end == FILLED ? "FILLED" : "PULLED", t, (long long)L.endPx, L.traded); rows.push_back(b); }
    }
    void qualify(int li, long long t) {
        Line& L = lines[(size_t)li]; if (L.drawn) return;
        L.drawn = true; L.drawnMs = t; ++tally.drawn; if (L.kind == K_LADDER) ++tally.ladders;
        char b[220]; std::snprintf(b, sizeof(b), "N|%d|%s|%s|%lld|%lld|%s|%lld", L.id, L.side == BID ? "B" : "A", L.kind == K_LADDER ? "LD" : "ONE", L.t0, (long long)L.segs.front().px, L.label().c_str(), L.oids.empty() ? 0LL : L.oids.back()); rows.push_back(b);
        for (size_t k = 1; k < L.segs.size(); ++k) { std::snprintf(b, sizeof(b), "S|%d|%lld|%lld", L.id, L.segs[k].t, (long long)L.segs[k].px); rows.push_back(b); }
    }
    // ---- rolls -------------------------------------------------------------------------------------------------------------
    void settleRolls(long long t) {
        for (size_t k = 0; k < rolls.size();) {
            PendRoll& p = rolls[k];
            if (p.due > t) { ++k; continue; }
            Line& L = lines[(size_t)p.line];
            const double reach = R.rollAtr * atrTicks();
            std::vector<long long> cand;
            for (auto& rn : recentNew) {
                if (rn.first < p.t - (long long)R.rollSec * 1000LL || rn.first > p.due) continue;
                auto it = book.find(rn.second); if (it == book.end()) { continue; }
                const Ord& o = it->second;
                if (o.side != p.side || o.maxShown != p.size || o.px == p.px || std::fabs((double)(o.px - p.px)) > reach) continue;
                if (std::find(L.oids.begin(), L.oids.end(), rn.second) != L.oids.end()) continue;
                if (o.line >= 0 && lines[(size_t)o.line].drawn) continue;   // already its own drawn line
                cand.push_back(rn.second);
            }
            std::sort(cand.begin(), cand.end()); cand.erase(std::unique(cand.begin(), cand.end()), cand.end());
            if (cand.size() == 1 && L.end == LIVE) {
                const long long noid = cand[0]; Ord& o = book[noid];
                if (o.line >= 0 && o.line != p.line) { Line& M = lines[(size_t)o.line]; M.end = PULLED; M.t1 = M.endMs = t; M.oids.clear(); --tally.lines; }   // its own (undrawn) line folds into the chain
                const long long tj = std::max(o.tAdd, p.t);
                L.oids.push_back(noid); L.rolls++; ++tally.rolls; L.rollGapMs = o.tAdd - p.t; L.pullDue = 0;
                if (L.drawn) { char b[96]; std::snprintf(b, sizeof(b), "O|%d|%lld|%lld", L.id, noid, o.tAdd - p.t); rows.push_back(b); }   // the chain's new order id (restart re-attach)
                L.size = o.size; L.maxShown = std::max(L.maxShown, o.maxShown);
                o.line = p.line; addSeg(p.line, tj, o.px);
            } else {
                if (cand.size() > 1) ++tally.rollAmbig;
                if (L.end == LIVE) endLine(p.line, PULLED, p.t);
            }
            rolls.erase(rolls.begin() + (long)k);
        }
        size_t cut = 0; while (cut < recentNew.size() && recentNew[cut].first < t - 2LL * (long long)R.rollSec * 1000LL - 1000LL) ++cut;
        if (cut) recentNew.erase(recentNew.begin(), recentNew.begin() + (long)cut);
    }
    // ---- ladders -----------------------------------------------------------------------------------------------------------
    void ladderPiece(long long oid, long long t) {
        Ord& o = book[oid];
        for (size_t k = ladders.size(); k-- > ladOpen;) {
            Ladder& g = ladders[k]; if (g.closed) continue;
            if (g.side == o.side && t - g.t0 <= R.ladWinMs) { g.oids.push_back(oid); o.ladder = (int)k; return; }
        }
        Ladder g; g.side = o.side; g.t0 = t; g.oids.push_back(oid); o.ladder = (int)ladders.size(); ladders.push_back(g);
    }
    void closeLadders(long long t) {
        for (size_t k = ladOpen; k < ladders.size(); ++k) {
            Ladder& g = ladders[k]; if (g.closed) { if (k == ladOpen) ++ladOpen; continue; }
            if (t - g.t0 <= R.ladWinMs) continue;
            g.closed = true; if (k == ladOpen) ++ladOpen;
            long long tot = 0, w = 0; Tick lo = 0, hi = 0; int n = 0;
            for (long long id : g.oids) { auto it = book.find(id); if (it == book.end()) continue; const Ord& o = it->second; tot += o.size; w += o.size * o.px; lo = n ? std::min(lo, o.px) : o.px; hi = n ? std::max(hi, o.px) : o.px; ++n; }
            if (n < R.ladMinN || tot < R.floorLots || hi - lo > R.ladSpan || late(g.t0)) { for (long long id : g.oids) { auto it = book.find(id); if (it != book.end()) it->second.ladder = -1; } continue; }
            Line L; L.id = nextId++; L.side = g.side; L.kind = K_LADDER; L.t0 = g.t0;
            for (long long id : g.oids) if (book.count(id)) { L.oids.push_back(id); book[id].line = (int)lines.size(); }
            Seg s; s.t = g.t0; s.px = (Tick)std::llround((double)w / (double)std::max(1LL, tot)); L.segs.push_back(s);
            L.size = tot; L.maxShown = tot; g.line = (int)lines.size(); waiting.push_back(g.line); lines.push_back(L); ++tally.lines;
        }
        if (ladders.size() > 4000) { /* closed ladders keep indexes for orders; prune only fully closed without lines is unsafe - cap */ }
    }
    void ladderReprice(int li, long long t) {
        Line& L = lines[(size_t)li]; if (L.end != LIVE) return;
        long long tot = 0, w = 0; for (long long id : L.oids) { auto it = book.find(id); if (it == book.end()) continue; tot += it->second.size; w += it->second.size * it->second.px; }
        if (tot > 0) addSeg(li, t, (Tick)std::llround((double)w / (double)tot));
    }
    void ladderPieceGone(int li, long long oid, long long t) {
        Line& L = lines[(size_t)li]; if (L.end != LIVE) return;
        auto it = book.find(oid); if (it != book.end() && it->second.traded > 0) { L.fillMs = t; L.fillPx = it->second.px; }
        bool any = false; for (long long id : L.oids) if (id != oid && book.count(id)) any = true;
        if (!any) endLine(li, L.traded * 2 >= L.size ? FILLED : PULLED, t);
    }
    // ---- timers ------------------------------------------------------------------------------------------------------------
    long long lastDue = 0; size_t ladOpen = 0;   // first ladder that may still be open
    std::vector<int> waiting;                     // live lines not yet drawn
    std::unordered_map<long long, int> restoredOid;   // order id -> a restored LIVE line (after an IRT restart the feed sends it again)
    void dueWork(long long t) {
        if (!rolls.empty()) { bool due = false; for (auto& p : rolls) if (p.due <= t) { due = true; break; } if (due) settleRolls(t); }
        if (ladOpen < ladders.size()) closeLadders(t);
        if (t >= lastDue && t - lastDue < 1000) return;   // the 9-min rest is checked once a second (a clock step back re-checks)
        lastDue = t;
        for (size_t k = 0; k < waiting.size();) {
            const int i = waiting[k]; Line& L = lines[(size_t)i];
            if (L.drawn || L.end != LIVE || L.oids.empty()) { waiting[k] = waiting.back(); waiting.pop_back(); continue; }
            if (L.pullDue == 0 && t - L.t0 >= (long long)R.restMin * 60000LL) { qualify(i, t); waiting[k] = waiting.back(); waiting.pop_back(); continue; }
            ++k;
        }
    }
    // ---- bar close: the HELD / THRU words ----------------------------------------------------------------------------------
    void closeBar(const Bar& b) {
        if (!bars.empty() && b.endMs <= bars.back().endMs) return;
        if (!bars.empty()) { const long long d = b.endMs - bars.back().endMs; if (d > 0 && d < 3600000LL) barMs = d; }
        bars.push_back(b); if (bars.size() > 2000) bars.erase(bars.begin(), bars.begin() + 500);
        // ATR = the mean bar range (ticks) of the last 40 closed bars (2 h on 3-min) - the research's 1-ATR roll / zone reach
        { const size_t n = bars.size(); const size_t k0 = n > 40 ? n - 40 : 0; double s = 0; int m = 0;
          for (size_t k = k0; k < n; ++k) { s += (double)(bars[k].h - bars[k].l); ++m; }
          if (m > 0) atr = s / m; }
        dueWork(b.endMs);
        std::vector<int> decided;
        for (size_t i = 0; i < lines.size(); ++i) {
            Line& L = lines[i];
            if (!L.drawn || L.end != FILLED || L.word != W_NONE || L.restored) continue;
            if (!L.fillBarEnd) { const long long f = L.fillMs ? L.fillMs : L.endMs; L.fillBarEnd = f; }
            if (b.endMs <= L.fillBarEnd) continue;   // this bar closed before (or exactly at) the fill: not the fill bar yet
            const Tick q = L.fillPx ? L.fillPx : L.px();
            L.barsSeen++;
            if (L.word == W_NONE) {
                if (L.side == BID ? b.h < q : b.l > q) L.word = W_THRU;
                else if (L.side == BID ? (b.c > b.o && b.c > q) : (b.c < b.o && b.c < q)) L.word = W_HELD;
                else if (L.barsSeen >= R.wordBars) L.word = W_THRU;
            }
            if (L.word != W_NONE) { L.wordMs = b.endMs; decided.push_back((int)i); }
        }
        if (!decided.empty()) zoneWords(decided, b);
    }
    // one word per zone: a word within zoneBars of an earlier SAME shown word, same side, within zoneAtr ATR -> hover only; among the words
    // decided at this same close, the one nearest the pivot extreme (bid: lowest low since the first fill; offer: highest high) is shown
    void zoneWords(const std::vector<int>& dec, const Bar& b) {
        const double reach = R.zoneAtr * atrTicks();
        std::vector<int> order = dec;
        // extreme since the earliest fill among these words
        for (int side = 0; side < 2; ++side) {
            std::vector<int> v; for (int i : order) if (lines[(size_t)i].side == side) v.push_back(i);
            if (v.empty()) continue;
            long long f0 = lines[(size_t)v[0]].fillMs; for (int i : v) f0 = std::min(f0, lines[(size_t)i].fillMs);
            Tick ext = side == BID ? b.l : b.h;
            for (const Bar& x : bars) if (x.endMs > f0) ext = side == BID ? std::min(ext, x.l) : std::max(ext, x.h);
            std::sort(v.begin(), v.end(), [&](int a, int c) {
                const Line& A = lines[(size_t)a]; const Line& Cc = lines[(size_t)c];
                const Tick da = std::llabs((A.fillPx ? A.fillPx : A.px()) - ext), dc = std::llabs((Cc.fillPx ? Cc.fillPx : Cc.px()) - ext);
                if (da != dc) return da < dc;
                return A.fillMs < Cc.fillMs; });
            for (int i : v) {
                Line& L = lines[(size_t)i]; const Tick q = L.fillPx ? L.fillPx : L.px();
                bool hide = false;
                for (const Line& M : lines) {
                    if (&M == &L || !M.wordShown || M.side != L.side || M.word != L.word) continue;
                    const Tick mq = M.fillPx ? M.fillPx : M.px();
                    if (std::fabs((double)(mq - q)) <= reach && L.wordMs - M.wordMs <= (long long)R.zoneBars * barMs) hide = true;
                }
                L.wordShown = !hide;
                if (L.word == W_HELD) ++tally.held; else ++tally.thru;
                if (hide) ++tally.wordsHidden;
                char bb[160]; std::snprintf(bb, sizeof(bb), "W|%d|%s|%lld|%d", L.id, L.word == W_HELD ? "HELD" : "THRU", L.wordMs, L.wordShown ? 1 : 0); rows.push_back(bb);
            }
        }
    }
    // ---- restart: read the record back (drawn lines, steps, ends, words) ---------------------------------------------------
    void restore(const std::vector<std::string>& recs) {
        std::map<int, size_t> byId;
        for (const auto& r : recs) {
            if (r.empty() || r[0] == '#') continue;
            std::vector<std::string> f = split(r, '|');
            if (f[0] == "N" && f.size() >= 7) {
                Line L; L.id = std::atoi(f[1].c_str()); L.side = f[2] == "A" ? ASK : BID; L.kind = f[3] == "LD" ? K_LADDER : K_ONE; L.t0 = std::atoll(f[4].c_str());
                Seg s; s.t = L.t0; s.px = std::atoll(f[5].c_str()); L.segs.push_back(s); L.drawn = true; L.restored = true;
                // label back into size / refills / pieces
                const std::string& lb = f[6];
                if (L.kind == K_LADDER) { size_t o = lb.find("o "); L.size = o != std::string::npos ? std::atoll(lb.c_str() + o + 2) : 0; int n = std::atoi(lb.c_str()); for (int k = 0; k < n; ++k) L.oids.push_back(-1); }
                else if (lb.size() > 3 && lb[0] == 'I' && lb[2] == 'R') { L.refills = std::atoi(lb.c_str() + 3); size_t sp = lb.find(' ', 3); L.traded = sp != std::string::npos ? std::atoll(lb.c_str() + sp + 1) : 0; L.size = L.traded; }
                else { L.size = std::atoll(lb.c_str()); L.maxShown = L.size; }
                if (L.kind == K_ONE) { const long long oid = f.size() >= 8 ? std::atoll(f[7].c_str()) : 0; L.oids.push_back(oid ? oid : -1); if (oid) restoredOid[oid] = (int)lines.size(); }
                if (L.oids.empty()) L.oids.push_back(-1);
                byId[L.id] = lines.size(); lines.push_back(L); if (L.id >= nextId) nextId = L.id + 1;
            } else if (f[0] == "S" && f.size() >= 4) {
                auto it = byId.find(std::atoi(f[1].c_str())); if (it == byId.end()) continue;
                Seg s; s.t = std::atoll(f[2].c_str()); s.px = std::atoll(f[3].c_str()); lines[it->second].segs.push_back(s);
            } else if (f[0] == "O" && f.size() >= 4) {
                auto it = byId.find(std::atoi(f[1].c_str())); if (it == byId.end()) continue;
                const long long oid = std::atoll(f[2].c_str()); Line& L = lines[it->second]; L.oids.push_back(oid); L.rolls++; L.rollGapMs = std::atoll(f[3].c_str()); restoredOid[oid] = (int)it->second;
            } else if (f[0] == "E" && f.size() >= 6) {
                auto it = byId.find(std::atoi(f[1].c_str())); if (it == byId.end()) continue;
                Line& L = lines[it->second]; L.end = f[2] == "FILLED" ? FILLED : PULLED; L.endMs = L.t1 = std::atoll(f[3].c_str()); L.endPx = std::atoll(f[4].c_str()); L.traded = std::atoll(f[5].c_str());
                if (L.end == FILLED) { L.fillMs = L.endMs; L.fillPx = L.endPx; }
            } else if (f[0] == "W" && f.size() >= 5) {
                auto it = byId.find(std::atoi(f[1].c_str())); if (it == byId.end()) continue;
                Line& L = lines[it->second]; L.word = f[2] == "HELD" ? W_HELD : W_THRU; L.wordMs = std::atoll(f[3].c_str()); L.wordShown = f[4] == "1";
            }
        }
        // restored LIVE lines are ended at the restart as unknown? No: they stay as drawn; with no book they cannot be followed, so they
        // are closed as PULLED-unknown only for drawing purposes when the feed proves the order is gone. Kept LIVE (full colour).
    }
    void prune(long long t) {
        std::vector<long long> kill;
        for (auto& kv : book) if (kv.second.line < 0 && kv.second.ladder < 0 && kv.second.size < R.ladPieceMin && t - kv.second.tAdd > 3600000LL) kill.push_back(kv.first);
        for (long long k : kill) book.erase(k);
    }
    // lines to draw (drawn, not folded)
    bool visible(const Line& L) const { return L.drawn && !L.oids.empty(); }
};

// ---- text / hover --------------------------------------------------------------------------------------------------------------
inline int decimals(double tick) { int d = 0; double t = tick; while (d < 6 && std::fabs(t - std::round(t)) > 1e-9) { t *= 10; ++d; } return d; }
inline std::string price(Tick t, double tick) { char b[48]; std::snprintf(b, sizeof(b), "%.*f", decimals(tick), (double)t * tick); return b; }
inline std::string clock12(long long ms) { long long m = (ms / 60000LL) % 1440LL; if (m < 0) m += 1440; int h = (int)(m / 60), mi = (int)(m % 60), s = (int)((ms / 1000LL) % 60LL); char b[24]; std::snprintf(b, sizeof(b), "%d:%02d:%02d %s", h % 12 == 0 ? 12 : h % 12, mi, s < 0 ? 0 : s, h < 12 ? "AM" : "PM"); return b; }
inline std::string dur(long long ms) { if (ms < 0) ms = 0; char b[32]; long long m = ms / 60000LL; if (m >= 60) std::snprintf(b, sizeof(b), "%lld h %lld min", m / 60, m % 60); else std::snprintf(b, sizeof(b), "%lld min", m); return b; }
inline const char* wordText(int w) { return w == W_HELD ? "HELD" : w == W_THRU ? "THRU" : ""; }
struct Row { std::string text; int ink = INK_WHITE; };
inline std::vector<Row> hoverRows(const Line& L, double tick, long long nowMs) {
    std::vector<Row> r; char b[300];
    const bool bid = L.side == BID; const int ink = bid ? INK_GREEN : INK_RED;
    if (L.kind == K_LADDER) std::snprintf(b, sizeof(b), "%s LADDER %s  %do = %d orders sent together totaling %lld lots", bid ? "BID" : "OFFER", price(L.px(), tick).c_str(), (int)L.oids.size(), (int)L.oids.size(), L.size);
    else if (L.refills > 0) std::snprintf(b, sizeof(b), "%s ICEBERG %s  I R%d = refilled %d times, %lld lots filled", bid ? "BID" : "OFFER", price(L.px(), tick).c_str(), L.refills, L.refills, L.traded);
    else std::snprintf(b, sizeof(b), "%s %lld lots at %s", bid ? "BID" : "OFFER", std::max(L.maxShown, L.size), price(L.px(), tick).c_str());
    r.push_back(Row{ b, ink });
    r.push_back(Row{ std::string("Question: is real size ") + (bid ? "waiting to buy" : "waiting to sell") + " here, and did it hold?", INK_WHITE });
    std::snprintf(b, sizeof(b), "Placed %s, rested %s%s", clock12(L.t0).c_str(), dur((L.end == LIVE ? nowMs : L.endMs) - L.t0).c_str(), L.restored ? " (from the record)" : ""); r.push_back(Row{ b, INK_WHITE });
    if (L.segs.size() > 1) {
        std::string s = L.rolls > 0 ? "Rolled: " : "Moved: ";
        for (size_t k = 0; k < L.segs.size() && k < 6; ++k) { if (k) s += " -> "; s += price(L.segs[k].px, tick); }
        if (L.rolls > 0 && L.rollGapMs >= -60000) { std::snprintf(b, sizeof(b), " (re-placed %+.1f s after the pull)", (double)L.rollGapMs / 1000.0); s += b; }
        r.push_back(Row{ s, INK_WHITE });
    }
    if (L.end == FILLED) {
        std::snprintf(b, sizeof(b), "Filled %s at %s, %lld lots traded", clock12(L.fillMs ? L.fillMs : L.endMs).c_str(), price(L.fillPx ? L.fillPx : L.endPx, tick).c_str(), L.traded); r.push_back(Row{ b, INK_WHITE });
        if (L.word != W_NONE) { std::snprintf(b, sizeof(b), "%s%s: %s", wordText(L.word), L.wordShown ? "" : " (not drawn: same zone as an earlier word)", L.word == W_HELD ? (bid ? "a green bar closed back above it and no bar traded wholly below it" : "a red bar closed back below it and no bar traded wholly above it") : (bid ? "a bar traded wholly below it or no green close above it within 4 bars" : "a bar traded wholly above it or no red close below it within 4 bars")); r.push_back(Row{ b, L.word == W_HELD ? ink : INK_GREY }); }
        else r.push_back(Row{ "Waiting for the favourable close (up to 4 bars)", INK_GREY });
    } else if (L.end == PULLED) { std::snprintf(b, sizeof(b), "Pulled %s (dark line: size that left)", clock12(L.endMs).c_str()); r.push_back(Row{ b, INK_GREY }); }
    else r.push_back(Row{ "Still resting", INK_WHITE });
    r.push_back(Row{ std::string("How to use: a level where ") + (bid ? "buyers" : "sellers") + " committed size; HELD = they defended it (a read, not an entry).", INK_WHITE });
    r.push_back(Row{ "Example: ES Oct 22, 7,680 bids filled 1:04:20 PM, green close above = HELD at the 1:04 PM low (+24 pts).", INK_WHITE });
    return r;
}
}   // namespace oln
#endif
