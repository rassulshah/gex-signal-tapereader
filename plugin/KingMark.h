#ifndef LS_KINGMARK_H
#define LS_KINGMARK_H
// KingMark.h 1.0.0 (2026-11-20, Rassul approved 05:05 CT "no, just 1 king.. we can have the mark as well"; mockup
// limit-profile-king-mockup.html). SDK-free, C++11, no files, no clocks of its own. Used by LimitFeed.h (lsLimitProfile 2.67.0).
// THE ONE KING of the Liquidity column: the biggest REAL resting level within reach on EITHER side (TopBook's evaluate() list, already
// filtered: rested 2 min+, not re-priced, not a just-placed cluster, past the first 2 ticks, within 3 ATR). The ranked rows 2-6 are gone.
// MARKS (one letter on the king row, from the liquidity-analyst reads; MBO only - without the order-by-order feed there are no marks):
//   L = the king LEFT: more than half of it went by CANCELS (DEL, MOD away, size cut) - not fills - before price reached its price
//   P = PULLED WHILE HIT: the same, but price had reached it (traded at / through it, or fills there) - the strongest break warning
//       (pulled while hit broke 78% vs 54%, 5 markets)
//   a king that is FILLED (traded away: fills >= cancels) just goes - no mark
// A marked king stays on the row MARK_MS (1 minute) frozen, then the next king shows. Every king's end is an Event for the nightly
// record (L / P / F filled / R replaced by a bigger one / X dropped off the board / G gone, mixed), so the loop can score L / P vs held / broke.
// A new king replaces the old one only when it is STRICTLY bigger (a tie keeps the king: no flicker between equal bid / offer).
#include <vector>
#include <string>
#include <cstdio>
namespace kgm {
static const char* const VERSION = "1.0.0";
struct Params { long long markMs = 60000; double goneFrac = 0.5; };
struct Cand { int side = 0; long long px = 0, lots = 0; };                  // side 0 bid / 1 offer, px in ticks (TopBook's)
struct Probe { long long resting = 0, cxl = 0, fill = 0; };                 // at the king's price & side: all resting lots, cumulative cancelled / filled lots
struct View { bool have = false; int side = 0; long long px = 0, lots = 0; char mark = 0; long long sinceMs = 0; };
struct Event { long long tMs = 0; char kind = 0; int side = 0; long long px = 0, lots = 0, peak = 0, cxl = 0, fill = 0; bool reached = false; long long sinceMs = 0; };
class Tracker {
public:
    explicit Tracker(const Params& p = Params()) : P(p) {}
    void reset() { k = King(); markOn = false; ++resets; }
    // once a second. list = TopBook's evaluate() output (any order); probe(side, px) -> Probe at that price; lastPx = last trade (ticks);
    // mbo = the order-by-order feed is the source (false: the king shows, never a mark)
    template <class Fn> View step(long long now, const std::vector<Cand>& list, Fn probe, long long lastPx, bool mbo) {
        if (markOn) {
            if (now - markT < P.markMs && now >= markT) return frozen;
            markOn = false; k = King(); }
        // the biggest listed level that is still there (TopBook's board keeps a vanished level 10 s - never re-crown it)
        const Cand* top = nullptr; for (const Cand& c : list) if (c.lots > 0 && (!top || c.lots > top->lots) && (double)probe(c.side, c.px).resting >= P.goneFrac * (double)c.lots) top = &c;
        if (k.on) {
            const Cand* mine = nullptr; for (const Cand& c : list) if (c.side == k.side && c.px == k.px) mine = &c;
            if (mine && mine->lots > k.peak) k.peak = mine->lots;
            if (mine) k.lots = mine->lots;
            const Probe pr = probe(k.side, k.px);
            if (lastPx > 0 && (k.side == 1 ? lastPx >= k.px : lastPx <= k.px)) k.reached = true;
            if (pr.fill > k.fill0) k.reached = true;
            const long long dc = pr.cxl - k.cxl0, df = pr.fill - k.fill0;
            if ((double)pr.resting < P.goneFrac * (double)k.peak) {          // more than half of it is gone
                if (mbo && dc > df && (double)dc > P.goneFrac * (double)k.peak) {
                    const char m = k.reached ? 'P' : 'L'; push(now, m, pr);
                    if (m == 'P') ++nP; else ++nL;
                    frozen = View(); frozen.have = true; frozen.side = k.side; frozen.px = k.px; frozen.lots = k.peak; frozen.mark = m; frozen.sinceMs = k.since;
                    markOn = true; markT = now; return frozen; }
                push(now, df >= dc ? 'F' : 'G', pr); if (df >= dc) ++nF; else ++nG; k = King();
            } else if (!mine) { push(now, 'X', pr); ++nX; k = King(); }     // off the board (out of reach / a filter) while still resting
            else if (top && top->lots > k.lots && !(top->side == k.side && top->px == k.px)) { push(now, 'R', pr); ++nR; k = King(); }
        }
        if (!k.on && top) { k = King(); k.on = true; k.side = top->side; k.px = top->px; k.lots = k.peak = top->lots; k.since = now;
            const Probe pr = probe(k.side, k.px); k.cxl0 = pr.cxl; k.fill0 = pr.fill;
            k.reached = lastPx > 0 && (k.side == 1 ? lastPx >= k.px : lastPx <= k.px); ++nKings; }
        View v; if (k.on) { v.have = true; v.side = k.side; v.px = k.px; v.lots = k.lots; v.sinceMs = k.since; }
        return v;
    }
    std::vector<Event> take() { std::vector<Event> v; v.swap(ev); return v; }
    bool marking() const { return markOn; }
    std::string statusLine() const {
        char b[320]; std::snprintf(b, sizeof b, "KING,KingMark %s,kings %d,L %d,P %d,filled %d,replaced %d,dropped %d,gone mixed %d,mark now %c,mark lasts %llds,resets %d",
            VERSION, nKings, nL, nP, nF, nR, nX, nG, markOn ? frozen.mark : '-', P.markMs / 1000, resets);
        return b; }
    int nKings = 0, nL = 0, nP = 0, nF = 0, nR = 0, nX = 0, nG = 0, resets = 0;
private:
    struct King { bool on = false; int side = 0; long long px = 0, lots = 0, peak = 0, cxl0 = 0, fill0 = 0, since = 0; bool reached = false; };
    void push(long long now, char kind, const Probe& pr) {
        Event e; e.tMs = now; e.kind = kind; e.side = k.side; e.px = k.px; e.lots = k.lots; e.peak = k.peak; e.cxl = pr.cxl - k.cxl0; e.fill = pr.fill - k.fill0;
        e.reached = k.reached; e.sinceMs = k.since; ev.push_back(e); if (ev.size() > 500) ev.erase(ev.begin()); }
    Params P; King k; bool markOn = false; long long markT = 0; View frozen; std::vector<Event> ev;
};
} // namespace kgm
#endif
