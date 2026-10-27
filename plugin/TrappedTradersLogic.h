#ifndef LS_TRAPPEDTRADERS_LOGIC_H
#define LS_TRAPPEDTRADERS_LOGIC_H
// TrappedTradersLogic.h 0.1.0 (2026-10-26) -- SDK-free rules of lsTrappedTraders ("Who is trapped?"). Unit-tested on Linux.
// Rassul 2026-10-26 20:14-20:15 CT: "this makes the other underwater trader better" / "lets just call underwater traders trapped
// traders. build it. its also simple and complements defense better right ?" (variant B0 of the trapped test, outputs/trapped_test).
//
// PER CLOSED CANDLE (volume at price of the candle; aggressor-classified: sellVolume = sold at the bid, buyVolume = bought at the ask):
//   GREEN = SELL lots that traded BELOW the candle's close   (those sellers are underwater -> trapped sellers, bullish fuel)
//   RED   = BUY  lots that traded ABOVE the candle's close   (those buyers are underwater  -> trapped buyers, bearish fuel)
//   Read: green > red = bullish, red > green = bearish. No exceptions.
// STAGE 3 TRIGGER (decided once on closed candles, recorded, never repainted):
//   extreme candle k = a new low below the prior candle's low that is the lowest low of the current down-wave of his 1-bar ZigZag
//   (a higher high without a lower low flips the wave up, a lower low without a higher high flips it down; both = keep) - mirror for
//   highs. From k through k+3 (WINDOW, his 3-candle window), with no candle beyond k's extreme after k and his pivot rule (j == k or
//   low_j < high_k; mirror), the FIRST candle j where the turn side's lots > the other side's AND it was not ahead on the candle before
//   = trigger: a triangle + ratio (turn / other) over the winning bar. One trigger per extreme, one per candle.
//   at_level = the extreme sits within the level band of one of his trade levels / gamma levels and price came INTO it (lows: a high of
//   the 10 candles before k above level + band; highs: a low below level - band) - his direction rule. Default display: levels only.
// NORMAL = the median of (green + red) of the same 30-minute slot (candle start, Central) over the last 20 earlier sessions on the
//   chart -> "vs normal" in the live box (x = side lots / (normal / 2)).
// Param sources (Quality Check): RASSUL = pivot rule, 3-candle window, trade levels + direction rule, green>red read; DATA = the B0
//   definition and the window (trapped test 2026-10-26); PARAM (ours, "?" until the nightly proves them) = level band, 10-candle
//   approach, 30-minute slot / 20-session normal.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ttr {

static const char* const VERSION = "0.1.0";
static const int WINDOW = 3;            // RASSUL: his rule (2026-10-26 19:34) - the pivot candle + up to 3 after it
static const int APPROACH = 10;         // PLACEHOLDER: candles before the extreme checked for "price came into the level" (the nightly trapped_traders part measures 5 / 10 / 20)
static const int NORMAL_SESSIONS = 20;  // PLACEHOLDER: sessions in the same-time normal (shared convention of the zone-read normals)
static const int SLOT_MIN = 30;         // PLACEHOLDER: minutes per same-time slot (shared convention of the zone-read normals)
static const char* const REC_HEAD = "TTREC,1";

inline bool finite(double v) { return v == v && v < 1e300 && v > -1e300; }
inline bool fullLL(const std::string& s, long long& out) {
    if (s.empty() || s.size() > 20) return false;
    char* e = nullptr; const long long v = std::strtoll(s.c_str(), &e, 10);
    if (!e || *e) return false;
    out = v; return true;
}
inline std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> o; std::string cur;
    for (size_t i = 0; i < s.size(); ++i) { if (s[i] == d) { o.push_back(cur); cur.clear(); } else cur += s[i]; }
    o.push_back(cur); return o;
}
inline bool toTicks(double px, double tick, long long& out) {
    if (!finite(px) || !(tick > 0) || !finite(tick)) return false;
    const double q = px / tick;
    if (!finite(q) || q > 9e15 || q < -9e15) return false;
    out = (long long)std::llround(q); return true;
}
inline std::string commas(long long v) {
    const bool neg = v < 0; unsigned long long u = neg ? (unsigned long long)(-(v + 1)) + 1ull : (unsigned long long)v;
    std::string s = std::to_string(u), o; int c = 0;
    for (int i = (int)s.size() - 1; i >= 0; --i) { o.insert(o.begin(), s[(size_t)i]); if (++c % 3 == 0 && i > 0) o.insert(o.begin(), ','); }
    return neg ? "-" + o : o;
}
// "2.4x", "10x+" (a near-zero other side would print absurd ratios)
inline std::string ratioText(double r) {
    if (!finite(r) || r < 0) return "?";
    if (r >= 10.0) return "10x+";
    char b[16]; std::snprintf(b, sizeof b, "%.1fx", r); return b;
}

struct Cell { long long px = 0, bid = 0, ask = 0; };          // ticks; bid = sold at the bid (aggressive sells), ask = bought
struct Level { std::string name; long long px = 0; };          // ticks

// green / red of one candle from its cells; close in ticks
inline void trapped(const std::vector<Cell>& cells, long long close, long long& g, long long& r) {
    g = 0; r = 0;
    for (size_t i = 0; i < cells.size(); ++i) {
        if (cells[i].px < close && cells[i].bid > 0) g += cells[i].bid;
        if (cells[i].px > close && cells[i].ask > 0) r += cells[i].ask;
    }
}

struct Bar {
    long long t = 0, sess = 0; int sod = 0;                    // t = IRT bar stamp (END); sess = session day number; sod = Central seconds of day of the END
    long long o = 0, h = 0, l = 0, c = 0;                      // ticks
    bool ok = false; long long g = 0, r = 0;                   // ok = volume at price read (or recorded)
    int dir = 0; long long wl = 0, wh = 0;                     // ZigZag wave + the wave's extreme so far
    long long norm = -1;                                       // the same-slot normal (green + red), -1 = building
};
struct Trigger {
    long long t = 0, kT = 0, sess = 0; int side = 0;           // t = the mark candle, kT = the extreme candle; side +1 bullish (low) / -1 bearish (high)
    double ratio = 0; long long ext = 0; bool atLevel = false; std::string level; bool fromRecord = false;
};
struct Cand { int k = -1; int side = 0; long long ext = 0, hiK = 0, loK = 0; };

inline int slotOf(int sod, int spb) {                          // the candle START's 30-minute slot (minutes / 30)
    int s = sod - (spb > 0 ? spb : 0); s = ((s % 86400) + 86400) % 86400; return (s / 60) / SLOT_MIN;
}

class Engine {
public:
    std::vector<Bar> bars;
    std::vector<Trigger> trig;                                 // decided (engine + record), in time order
    std::vector<std::string> newLines;                         // record lines to append
    std::map<long long, Trigger> rec;                          // from the record: mark candle t -> trigger
    std::map<long long, std::pair<long long, long long> > recBar;   // t -> (g, r) recorded (fallback when no volume at price)
    long long recLastT = 0;                                    // the record covers decisions up to here: never re-decided (no repaint)
    std::vector<Level> levels; long long bandTicks = 8;        // set by the host before feed()
    size_t maxBars = 20 * 480;
    long long candidates = 0, decided = 0;

    // the record of earlier runs (lines without the header): TTB|t|sess|g|r|norm  and  TTT|t|kT|sess|side|ratio100|ext|at|level
    int loadRecord(const std::vector<std::string>& lines) {
        int bad = 0;
        for (size_t i = 0; i < lines.size(); ++i) {
            const std::string& ln = lines[i]; if (ln.empty() || ln == REC_HEAD) continue;
            const std::vector<std::string> p = split(ln, '|');
            long long a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0;
            if (p[0] == "TTB" && p.size() >= 6 && fullLL(p[1], a) && fullLL(p[2], b) && fullLL(p[3], c) && fullLL(p[4], d) && c >= 0 && d >= 0) {
                if (!recBar.count(a)) recBar[a] = std::make_pair(c, d);
                recLastT = std::max(recLastT, a); continue;
            }
            if (p[0] == "TTT" && p.size() >= 9 && fullLL(p[1], a) && fullLL(p[2], b) && fullLL(p[3], c) && fullLL(p[4], d) && fullLL(p[5], e) && fullLL(p[6], f) && fullLL(p[7], g)
                && (d == 1 || d == -1) && e >= 0) {
                (void)h;
                if (!rec.count(a)) { Trigger T; T.t = a; T.kT = b; T.sess = c; T.side = (int)d; T.ratio = (double)e / 100.0; T.ext = f; T.atLevel = g != 0; T.level = p[8].substr(0, 40); T.fromRecord = true; rec[a] = T; }
                recLastT = std::max(recLastT, a); continue;
            }
            ++bad;
        }
        return bad;
    }

    // one CLOSED candle, strictly in time order, each once. cellsOk = the volume at price was read (cells may be empty)
    void feed(Bar b, const std::vector<Cell>& cells, bool cellsOk) {
        if (!bars.empty() && b.t <= bars.back().t) return;
        if (cellsOk) { trapped(cells, b.c, b.g, b.r); b.ok = true; }
        else { std::map<long long, std::pair<long long, long long> >::const_iterator it = recBar.find(b.t); if (it != recBar.end()) { b.g = it->second.first; b.r = it->second.second; b.ok = true; } }
        // ZigZag wave (as the Python reference tt_calc.zigzag_dir) and the wave extremes
        const int n = (int)bars.size();
        if (n > 0) {
            const Bar& p = bars[(size_t)n - 1];
            const bool hh = b.h > p.h, ll = b.l < p.l;
            b.dir = (hh && !ll) ? 1 : (ll && !hh) ? -1 : p.dir;
            b.wl = (b.dir == -1 && p.dir == -1) ? std::min(b.l, p.wl) : b.l;
            b.wh = (b.dir == 1 && p.dir == 1) ? std::max(b.h, p.wh) : b.h;
        } else { b.dir = 0; b.wl = b.l; b.wh = b.h; }
        b.norm = normalFor(b);
        bars.push_back(b);
        decide();
        if (b.ok) newLines.push_back("TTB|" + std::to_string(b.t) + "|" + std::to_string(b.sess) + "|" + std::to_string(b.g) + "|" + std::to_string(b.r) + "|" + std::to_string(b.norm));
        if (bars.size() > maxBars) trim();
    }

    // the trigger of a candle (or nullptr)
    const Trigger* triggerAt(long long t) const { for (size_t i = trig.size(); i-- > 0;) if (trig[i].t == t) return &trig[i]; return nullptr; }

private:
    std::vector<Cand> open_;
    std::set<long long> usedT_;
    std::map<int, std::vector<std::pair<long long, long long> > > slotHist_;   // slot -> (sess, green + red) of earlier candles

    long long normalFor(const Bar& b) {
        const int s = slotOf(b.sod, 0);
        std::vector<std::pair<long long, long long> >& H = slotHist_[s];
        std::set<long long> sess; std::vector<long long> v;
        for (size_t i = H.size(); i-- > 0;) {
            if (H[i].first >= b.sess) continue;
            sess.insert(H[i].first); if ((int)sess.size() > NORMAL_SESSIONS) break;
            v.push_back(H[i].second);
        }
        if (b.ok) { H.push_back(std::make_pair(b.sess, b.g + b.r)); if (H.size() > 4000) H.erase(H.begin(), H.begin() + 1000); }
        if (sess.size() < 3 || v.empty()) return -1;
        std::sort(v.begin(), v.end()); const size_t m = v.size() / 2;
        return v.size() % 2 ? v[m] : (v[m - 1] + v[m]) / 2;
    }
    bool nearLevel(int k, int side, long long ext, std::string& name) const {
        for (size_t i = 0; i < levels.size(); ++i) {
            const long long lv = levels[i].px; if (std::llabs(lv - ext) > bandTicks) continue;
            bool came = false;
            for (int q = std::max(0, k - APPROACH); q < k && !came; ++q)
                came = side > 0 ? bars[(size_t)q].h > lv + bandTicks : bars[(size_t)q].l < lv - bandTicks;
            if (came) { name = levels[i].name; return true; }
        }
        return false;
    }
    void emit(const Cand& c, int j, double ratio) {
        const Bar& J = bars[(size_t)j];
        if (usedT_.count(J.t)) return;
        usedT_.insert(J.t);
        if (J.t <= recLastT) {                                   // the record already decided this span: it wins (no repaint)
            std::map<long long, Trigger>::const_iterator it = rec.find(J.t); if (it != rec.end()) trig.push_back(it->second);
            return;
        }
        Trigger T; T.t = J.t; T.kT = bars[(size_t)c.k].t; T.sess = J.sess; T.side = c.side; T.ratio = ratio; T.ext = c.ext;
        T.atLevel = nearLevel(c.k, c.side, c.ext, T.level);
        trig.push_back(T); ++decided;
        char b[40]; std::snprintf(b, sizeof b, "%lld", (long long)std::llround(std::min(ratio, 9999.0) * 100.0));
        newLines.push_back("TTT|" + std::to_string(T.t) + "|" + std::to_string(T.kT) + "|" + std::to_string(T.sess) + "|" + std::to_string(T.side) + "|" + b + "|"
                           + std::to_string(T.ext) + "|" + (T.atLevel ? "1" : "0") + "|" + T.level);
    }
    // evaluate candle j for one open candidate; true = the candidate is finished
    bool step(const Cand& c, int j) {
        const Bar& J = bars[(size_t)j];
        if (j > c.k && ((c.side > 0 && J.l < c.ext) || (c.side < 0 && J.h > c.ext))) return true;      // a new extreme: no turn yet
        if (j > c.k + WINDOW) return true;
        if (j > c.k && !((c.side > 0 && J.l < c.hiK) || (c.side < 0 && J.h > c.loK))) return j >= c.k + WINDOW;   // his pivot rule not met on j
        if (j < 1 || !J.ok || !bars[(size_t)j - 1].ok) return j >= c.k + WINDOW;
        const Bar& P = bars[(size_t)j - 1];
        const long long a = c.side > 0 ? J.g : J.r, o = c.side > 0 ? J.r : J.g;
        const long long pa = c.side > 0 ? P.g : P.r, po = c.side > 0 ? P.r : P.g;
        if (a > 0 && a > o && pa <= po) { emit(c, j, (double)a / (double)std::max(o, 1LL)); return true; }
        return j >= c.k + WINDOW;
    }
    void decide() {
        const int j = (int)bars.size() - 1; if (j < 1) return;
        std::vector<Cand> keep;
        for (size_t i = 0; i < open_.size(); ++i) if (!step(open_[i], j)) keep.push_back(open_[i]);
        open_.swap(keep);
        const Bar& B = bars[(size_t)j], &P = bars[(size_t)j - 1];
        for (int side = 1; side >= -1; side -= 2) {
            const bool isExt = side > 0 ? (B.l < P.l && B.l <= B.wl) : (B.h > P.h && B.h >= B.wh);
            if (!isExt) continue;
            Cand c; c.k = j; c.side = side; c.ext = side > 0 ? B.l : B.h; c.hiK = B.h; c.loK = B.l; ++candidates;
            if (!step(c, j)) open_.push_back(c);
        }
    }
    void trim() {
        const size_t cut = bars.size() - maxBars * 3 / 4;
        for (size_t i = 0; i < open_.size(); ++i) open_[i].k -= (int)cut;
        std::vector<Cand> keep; for (size_t i = 0; i < open_.size(); ++i) if (open_[i].k >= 0) keep.push_back(open_[i]); open_.swap(keep);
        bars.erase(bars.begin(), bars.begin() + (long)cut);
        const long long t0 = bars.front().t; usedT_.erase(usedT_.begin(), usedT_.lower_bound(t0)); std::vector<Trigger> tk; for (size_t i = 0; i < trig.size(); ++i) if (trig[i].t >= t0) tk.push_back(trig[i]); trig.swap(tk);
    }
};

// ---- pane layout (pure, tested): the title and the live box own the top; bars and marks live strictly below them, so nothing overlaps
struct Box { int l = 0, t = 0, r = 0, b = 0; };
inline bool overlaps(const Box& a, const Box& b) { return !(a.r < b.l || a.l > b.r || a.b < b.t || a.t > b.b); }
static const int TITLE_H = 19, BOX_Y = 22, BOX_W = 230, BOX_H = 58, MARK_ROOM = 26, BOX_MIN_PANE = 160;
struct Layout { bool box = false; Box title, live; int y0 = 0, base = 0; };
// paneTop / paneBottom in pixels; titleW = title text width; wantBox = the setting. The box only shows on a pane tall enough to keep bars readable.
inline Layout layout(int paneLeft, int paneTop, int paneBottom, int titleW, bool wantBox) {
    Layout L; L.base = paneBottom - 2;
    L.title.l = paneLeft; L.title.t = paneTop; L.title.r = paneLeft + 10 + titleW; L.title.b = paneTop + TITLE_H;
    L.box = wantBox && paneBottom - paneTop >= BOX_MIN_PANE;
    int head = L.title.b;
    if (L.box) { L.live.l = paneLeft + 4; L.live.t = paneTop + BOX_Y; L.live.r = L.live.l + BOX_W; L.live.b = L.live.t + BOX_H; head = L.live.b; }
    L.y0 = head + MARK_ROOM;                                     // the tallest bar ends here; its triangle (9 px) + ratio (15 px) fit in MARK_ROOM above it
    return L;
}
// marks are placed oldest first; a new mark that would touch anything already placed is skipped (an older one never moves)
static const size_t MAX_PLACED = 2000;                      // a pane never holds more boxes than this (cap the work per draw)
struct Placer {
    std::vector<Box> placed;
    bool tryPlace(const Box& a, const Box* b) {
        if (placed.size() >= MAX_PLACED) return false;
        for (size_t i = 0; i < placed.size(); ++i) if (overlaps(a, placed[i]) || (b && overlaps(*b, placed[i]))) return false;
        if (b && overlaps(a, *b)) return false;
        placed.push_back(a); if (b) placed.push_back(*b); return true;
    }
};

// the hover of one candle: the question it answers, what / how / use, the numbers, an example (his format; no text on the chart)
inline std::vector<std::string> hoverRows(const Bar& b, const Trigger* T, const std::string& when) {
    std::vector<std::string> o;
    o.push_back("Who is trapped?   " + when);
    o.push_back("Sellers trapped " + commas(b.g) + " lots   |   Buyers trapped " + commas(b.r) + " lots");
    if (b.norm > 0) {
        char x[96]; std::snprintf(x, sizeof x, "vs normal (this half hour): sellers %.1fx  |  buyers %.1fx", (double)b.g / std::max(1.0, b.norm / 2.0), (double)b.r / std::max(1.0, b.norm / 2.0));
        o.push_back(x);
    }
    if (T) o.push_back(std::string(T->side > 0 ? "TRIGGER (low): sellers trapped " : "TRIGGER (high): buyers trapped ") + ratioText(T->ratio) + " the other side"
                       + (T->atLevel ? " at " + T->level : "") + " - stage 3 turn");
    o.push_back("What: green = sell lots traded BELOW the close (sellers losing),");
    o.push_back("      red = buy lots traded ABOVE the close (buyers losing).");
    o.push_back("How: losing traders must get out - trapped sellers buy back,");
    o.push_back("      trapped buyers sell. The bigger bar is the fuel.");
    o.push_back("Use: green > red bullish, red > green bearish. At your level, take the");
    o.push_back("      triangle with Defense green > red and a green close (mirror).");
    o.push_back("Example: ES 10-26 9:12 low - sellers 8,333 vs buyers 3,887 = 2.1x,");
    o.push_back("      Defense green too; price rose 1 ATR before the low broke.");
    return o;
}

} // namespace ttr
#endif
