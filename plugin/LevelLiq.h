#ifndef LS_LEVELLIQ_H
#define LS_LEVELLIQ_H
// LevelLiq.h 1.0.0 (2026-11-20, lsLimitProfile 2.68.0; Rassul approved 06:02 CT "ok. i'll go with your recommendation"; mockup
// level-liquidity-states-mockup.html). SDK-free, C++11, no files, no clocks of its own.
// WHAT THE LIQUIDITY ON A LEVEL LINE DID - for every bright level-line stretch (a key / volume / gamma level with one of the largest
// resting nodes within 1 ATR of it; the blue bid / orange offer line, 2.60.0):
//   node      the node's price is fixed when the stretch starts (the price the host's node row had)
//   reach     price first trades within 1 ATR of the level -> S0 = the node's resting lots then
//   BUILDING  node lots >= 1.10 x S0 at a closed candle (before contact); LEAVING <= 0.90 x S0; else STEADY
//   contact   the first candle that trades AT the level after reach, the candle before it wholly on the defending side
//             (bids: its low above the level; offers: its high below)
//   at the contact candle's CLOSE (never before, never changed after):
//   P  pulled while hit: node lots at the close < 0.5 x lots at the candle's open, cancelled there (DEL / MOD away / size cut) > 0.5 x
//      the open lots and more than was filled
//   X  attackers slowing while the size stays: not P, node lots at the close >= 0.7 x the open, and the attack (lots filled on the
//      defending side within 2 ticks of the level) in the candle's last minute < 0.5 x its busiest earlier minute
//   H  hit and held: not P, the candle closed back out (bids: close above the level) and never traded 3+ ticks through it
//   the line ENDS at the close of a P candle, or of a candle that traded 3+ ticks through the level and closed through it
//   CONF the level row also names a MenthorQ level (decided by the host from the row's names)
// Every state / letter is COMPUTED and RECORDED; Params.draw* says which are drawn (only what passed the per-market scoring; the rest is
// recorded for the nightly - lra.levelliq_score - and published for lsHodLod's Now line with shown = 0).
#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
namespace llq {
static const char* const VERSION = "1.0.0";
enum State { ST_NONE = 0, ST_STEADY = 1, ST_BUILDING = 2, ST_LEAVING = 3 };
struct Params {
    double up = 1.10, dn = 0.90, pullFrac = 0.5, keepFrac = 0.7, slowFrac = 0.5; int thru = 3, near = 2; int outcomeBars = 3;
    // what is DRAWN (scoring 2026-11-20: see NOTES-LP2680.md); everything is computed, recorded and published either way
    // scored 2026-11-20 (56 MBO contacts, 5 markets, 6 days + 312 bar-only contacts): every read LEARNING (< 20 a side in both day halves);
    // H is drawn as a DESCRIPTION (closed back out, never 3+ ticks through - a price fact; the node was still >= half there in 13 of 15)
    bool drawBuild = false, drawLeave = false, drawX = false, drawH = true, drawP = false, drawConf = false;
};
struct Probe { long long resting = 0, cxl = 0, fill = 0; };   // at one price & side: resting lots, cumulative cancelled / filled lots
struct SegIn { long long lvl = 0; bool bid = true; long long node = 0; std::string name; bool conf = false; long long t0 = 0; };   // ticks, CT s
struct Bar { long long endS = 0, o = 0, h = 0, l = 0, c = 0; };   // a CLOSED candle, prices in ticks, bar END (CT s)
struct Mark { long long endS = 0; long long lvl = 0; bool bid = true; char letter = 0; };   // a letter at a contact candle, or a line END (letter 'B' break / 'P' pulled)
struct Change { long long tS = 0; int st = ST_NONE; };
struct Rec { std::string line; };
inline const char* stName(int s) { return s == ST_BUILDING ? "building" : s == ST_LEAVING ? "leaving" : s == ST_STEADY ? "steady" : "none"; }
struct Watch {
    long long lvl = 0, node = 0, t0 = 0, lastSeen = 0; bool bid = true, conf = false; std::string name;
    int phase = 0;                 // 0 waiting for reach, 1 reached, 2 contact candle forming, 3 decided (outcome counting), 4 done
    bool prevOnSide = false; long long S0 = 0; int st = ST_NONE, stLive = ST_NONE;
    long long cStart = -1; Probe cOpen; long long atk0 = 0, atk1 = -1, atk2 = -1;   // the forming candle's open snapshot + attack marks
    long long Sc = 0, Se = 0, cx = 0, fl = 0, m1 = 0, m2 = 0, m3 = 0; std::string letters; long long contactEnd = 0; int after = 0; bool broke = false;
    long long endS = 0;            // the line ends here (0 = still on)
    std::vector<Change> ch;        // style changes, decided at candle closes
};
inline long long key(long long lvl, bool bid) { return lvl * 2 + (bid ? 1 : 0); }
class Tracker {
public:
    explicit Tracker(const Params& p = Params()) : P(p) {}
    Params P;
    void reset() { W.clear(); M.clear(); Ends.clear(); out.clear(); curStart = -1; lastClose = 0; ++resets; }
    // ~once a second: the active bright stretches, last trade, ATR (ticks), probe(side 0 bid / 1 offer, px) -> Probe
    template <class Fn> void step(long long nowS, long long lastPx, double atrTicks, const std::vector<SegIn>& active, Fn probe) {
        for (const SegIn& s : active) {
            std::map<long long, Watch>::iterator it = W.find(key(s.lvl, s.bid));
            if (it != W.end() && it->second.endS > 0 && s.t0 > it->second.endS) { W.erase(it); it = W.end(); }   // a NEW stretch after the line ended
            if (it == W.end()) {
                if (W.size() >= 200) continue;
                Watch w; w.lvl = s.lvl; w.bid = s.bid; w.node = s.node; w.name = s.name; w.conf = s.conf; w.t0 = s.t0; w.lastSeen = nowS;
                it = W.insert(std::make_pair(key(s.lvl, s.bid), w)).first;
                emit("S|" + num(nowS) + "|" + w.name + "|" + num(w.lvl) + "|" + (w.bid ? "1" : "-1") + "|" + num(w.node) + "|" + (w.conf ? "1" : "0"));
            } else { it->second.lastSeen = nowS; if (!s.name.empty()) it->second.name = s.name; it->second.conf = s.conf; }
        }
        for (std::map<long long, Watch>::iterator it = W.begin(); it != W.end(); ++it) {
            Watch& w = it->second; if (w.endS > 0 || w.phase >= 4) continue;
            const int ds = w.bid ? 0 : 1; const Probe pr = probe(ds, w.node);
            if (w.phase == 0 && atrTicks > 0 && lastPx != 0 && std::fabs((double)(lastPx - w.lvl)) <= atrTicks && nowS - w.lastSeen <= 180) { w.phase = 1; w.S0 = pr.resting; w.st = w.stLive = ST_STEADY; }
            if (w.phase == 1) {
                w.stLive = stateOf(w.S0, pr.resting);
                const bool at = w.bid ? lastPx <= w.lvl : lastPx >= w.lvl;
                if (at && w.prevOnSide && w.cStart >= 0) { w.phase = 2; w.Sc = w.cOpen.resting; }
            }
            if (w.phase == 2 && w.cStart >= 0) {
                const long long a = attack(w, probe);
                if (w.atk1 < 0 && nowS >= w.cStart + 60) w.atk1 = a;
                if (w.atk2 < 0 && nowS >= w.cStart + 120) w.atk2 = a;
            }
        }
    }
    // once per newly CLOSED candle (in order), with the probe at its close; the next candle starts at b.endS
    template <class Fn> void close(const Bar& b, Fn probe) {
        if (b.endS <= lastClose) return;   // never twice, never backwards
        lastClose = b.endS;
        for (std::map<long long, Watch>::iterator it = W.begin(); it != W.end(); ++it) {
            Watch& w = it->second; if (w.endS > 0) continue;
            const int ds = w.bid ? 0 : 1; const Probe pr = probe(ds, w.node);
            const bool thru = w.bid ? b.l <= w.lvl - P.thru : b.h >= w.lvl + P.thru;
            const bool closedThru = w.bid ? b.c < w.lvl : b.c > w.lvl;
            const bool back = w.bid ? b.c > w.lvl : b.c < w.lvl;
            if (w.phase == 1) {
                const int s = stateOf(w.S0, pr.resting);
                if (s != w.st || w.ch.empty()) { w.st = s; w.ch.push_back(Change{b.endS, s}); emit("T|" + num(b.endS) + "|" + w.name + "|" + num(w.lvl) + "|" + (w.bid ? "1" : "-1") + "|" + stName(s) + "|" + num(w.S0) + "|" + num(pr.resting)); }
            }
            if (w.phase == 2 && w.cStart >= 0) {
                w.Se = pr.resting; w.cx = pr.cxl - w.cOpen.cxl; w.fl = pr.fill - w.cOpen.fill;
                const long long a3 = attack(w, probe), a1 = w.atk1 < 0 ? a3 : w.atk1, a2 = w.atk2 < 0 ? a3 : w.atk2;
                w.m1 = a1 - w.atk0; w.m2 = a2 - a1; w.m3 = a3 - a2;
                const bool Pp = w.Sc > 0 && (double)w.Se < P.pullFrac * (double)w.Sc && (double)w.cx > P.pullFrac * (double)w.Sc && w.cx > w.fl;
                const long long busy = w.m1 > w.m2 ? w.m1 : w.m2;
                const bool Xx = !Pp && w.Sc > 0 && (double)w.Se >= P.keepFrac * (double)w.Sc && busy > 0 && (double)w.m3 < P.slowFrac * (double)busy;
                const bool Hh = back && !thru && !Pp;   // a pulled level is not 'held' by its defenders
                w.letters.clear(); if (Pp) w.letters += 'P'; if (Xx) w.letters += 'X'; if (Hh) w.letters += 'H';
                for (char L : w.letters) { Mark m; m.endS = b.endS; m.lvl = w.lvl; m.bid = w.bid; m.letter = L; M.push_back(m); }
                w.contactEnd = b.endS; w.phase = 3; w.after = 0; w.broke = thru;
                emit("C|" + num(b.endS) + "|" + w.name + "|" + num(w.lvl) + "|" + (w.bid ? "1" : "-1") + "|" + (w.letters.empty() ? "-" : w.letters) + "|" + stName(w.st) + "|" +
                     num(w.S0) + "|" + num(w.Sc) + "|" + num(w.Se) + "|" + num(w.cx) + "|" + num(w.fl) + "|" + num(w.m1) + "|" + num(w.m2) + "|" + num(w.m3) + "|" + (w.conf ? "1" : "0"));
                if (thru && closedThru) endLine(w, b.endS, "B"); else if (Pp) endLine(w, b.endS, "P");
            } else if (w.phase == 3) {
                ++w.after; if (thru) w.broke = true;
                if (w.after >= P.outcomeBars) { w.phase = 4; emit("O|" + num(b.endS) + "|" + w.name + "|" + num(w.lvl) + "|" + (w.bid ? "1" : "-1") + "|" + (w.letters.empty() ? "-" : w.letters) + "|" + (w.broke ? "broke" : "held")); }
                if (w.endS == 0 && thru && closedThru) endLine(w, b.endS, "B");
            } else if (w.phase == 1 && thru && closedThru) endLine(w, b.endS, "B");
            w.prevOnSide = w.bid ? b.l > w.lvl : b.h < w.lvl;
            // the next candle's open snapshot
            w.cStart = b.endS; w.cOpen = pr; w.atk0 = attack(w, probe); w.atk1 = w.atk2 = -1;
        }
        // forget watches whose stretch is long gone (the drawn history keeps its marks in M)
        for (std::map<long long, Watch>::iterator it = W.begin(); it != W.end();) { if (b.endS - it->second.lastSeen > 6 * 3600) W.erase(it++); else ++it; }
        if (M.size() > 2000) M.erase(M.begin(), M.begin() + (long)(M.size() - 2000));
    }
    std::vector<Rec> take() { std::vector<Rec> r; r.swap(out); return r; }
    const std::vector<Mark>& marks() const { return M; }
    const std::vector<Mark>& ends() const { return Ends; }   // where drawn lines END (a break; a P only when P is drawn)
    // the end time of the line of (lvl, side) for a stretch [t0, t1] (0 = no end inside it)
    long long endIn(long long lvl, bool bid, long long t0, long long t1) const { long long e = 0; for (const Mark& m : Ends) if (m.lvl == lvl && m.bid == bid && m.endS >= t0 && m.endS <= t1 && (e == 0 || m.endS < e)) e = m.endS; return e; }
    const Watch* find(long long lvl, bool bid) const { std::map<long long, Watch>::const_iterator it = W.find(key(lvl, bid)); return it == W.end() ? nullptr : &it->second; }
    const std::map<long long, Watch>& watches() const { return W; }
    bool drawLetter(char L) const { return L == 'P' ? P.drawP : L == 'X' ? P.drawX : L == 'H' ? P.drawH : false; }
    // restore decided marks / line ends from the session record after a restart (C and E lines; the rest is only for the nightly)
    void restore(const std::string& text) {
        size_t a = 0; int n = 0;
        while (a < text.size() && n < 5000) { size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size(); std::string ln = text.substr(a, e - a); a = e + 1; ++n;
            if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
            std::vector<std::string> f; size_t p = 0; for (;;) { size_t q = ln.find('|', p); f.push_back(ln.substr(p, q == std::string::npos ? std::string::npos : q - p)); if (q == std::string::npos) break; p = q + 1; }
            long long t = 0, lv = 0, sd = 0;
            if (f.size() < 6 || !tok(f[1], t) || !tok(f[3], lv) || !tok(f[4], sd) || (sd != 1 && sd != -1) || t <= 0) continue;
            if (f[0] == "C") { for (char L : f[5]) if (L == 'P' || L == 'X' || L == 'H') { bool dup = false; for (const Mark& m : M) if (m.endS == t && m.lvl == lv && m.bid == (sd == 1) && m.letter == L) dup = true;
                    if (!dup) { Mark m; m.endS = t; m.lvl = lv; m.bid = sd == 1; m.letter = L; M.push_back(m); } }
                if (t > lastClose) lastClose = t; }
            else if (f[0] == "E") { Watch& w = W[key(lv, sd == 1)]; w.lvl = lv; w.bid = sd == 1; w.name = f[2]; if (w.endS == 0) w.endS = t; w.phase = 4; w.lastSeen = t; if (t > lastClose) lastClose = t;
                if (f[5] == "B" || (f[5] == "P" && P.drawP)) { bool dup = false; for (const Mark& m : Ends) if (m.endS == t && m.lvl == lv && m.bid == (sd == 1)) dup = true; if (!dup) { Mark m; m.endS = t; m.lvl = lv; m.bid = sd == 1; m.letter = f[5][0]; Ends.push_back(m); } } }
        }
    }
    // the Now-line file body for lsHodLod (no stamp - the host adds it): one row per live stretch whose line is on
    std::string nowBody(long long nowS) const {
        std::string s;
        for (std::map<long long, Watch>::const_iterator it = W.begin(); it != W.end(); ++it) { const Watch& w = it->second;
            if (nowS - w.lastSeen > 180 || w.phase == 0) continue;
            std::string st; bool shown = false; long long since = w.ch.empty() ? w.t0 : w.ch.back().tS;
            if (w.endS > 0) { if (w.letters.find('P') != std::string::npos && nowS - w.endS <= 600) { st = "P"; shown = P.drawP; since = w.endS; } else continue; }
            else if (w.phase >= 3 && !w.letters.empty() && nowS - w.contactEnd <= 600) { st = std::string(1, w.letters[w.letters.size() - 1]); for (char L : w.letters) if (L == 'X') st = "X"; shown = drawLetter(st[0]); since = w.contactEnd; }
            else { const int x = w.phase == 1 ? w.st : ST_STEADY; st = stName(x); shown = x == ST_BUILDING ? P.drawBuild : x == ST_LEAVING ? P.drawLeave : false; }
            char b[64]; std::snprintf(b, sizeof(b), "%lld", w.lvl);
            s += "L|" + clean(w.name) + "|" + b + "|" + (w.bid ? "B" : "O") + "|" + st + "|" + (shown ? "1" : "0") + "|" + num(since) + "|" + (w.conf ? "1" : "0") + "\n";
        }
        return s;
    }
    int resets = 0;
private:
    std::map<long long, Watch> W; std::vector<Mark> M, Ends; std::vector<Rec> out; long long curStart = -1, lastClose = 0;
    int stateOf(long long s0, long long now) const { if (s0 <= 0) return now > 0 ? ST_BUILDING : ST_STEADY; if ((double)now >= P.up * (double)s0) return ST_BUILDING; if ((double)now <= P.dn * (double)s0) return ST_LEAVING; return ST_STEADY; }
    template <class Fn> long long attack(const Watch& w, Fn probe) const { long long a = 0; const int ds = w.bid ? 0 : 1; for (long long p = w.lvl - P.near; p <= w.lvl + P.near; ++p) a += probe(ds, p).fill; return a; }
    void endLine(Watch& w, long long t, const char* why) { w.endS = t; if (why[0] == 'B' || (why[0] == 'P' && P.drawP)) { Mark m; m.endS = t; m.lvl = w.lvl; m.bid = w.bid; m.letter = why[0]; Ends.push_back(m); if (Ends.size() > 2000) Ends.erase(Ends.begin()); } emit("E|" + num(t) + "|" + w.name + "|" + num(w.lvl) + "|" + (w.bid ? "1" : "-1") + "|" + why); }
    void emit(const std::string& s) { if (out.size() < 500) { Rec r; r.line = s; out.push_back(r); } }
    static std::string num(long long v) { char b[32]; std::snprintf(b, sizeof(b), "%lld", v); return b; }
    static std::string clean(const std::string& s) { std::string r; for (char c : s) r += (c == '|' || c == '\n' || c == '\r') ? '/' : c; return r; }
    static bool tok(const std::string& s, long long& v) { if (s.empty() || s.size() > 20) return false; char* e = nullptr; v = std::strtoll(s.c_str(), &e, 10); return e && *e == 0; }
};
} // namespace llq
#endif
