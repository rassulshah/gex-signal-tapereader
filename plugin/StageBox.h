#ifndef LS_STAGE_BOX_H
#define LS_STAGE_BOX_H
// StageBox.h 1.0.0 (2026-10-25) -- SDK-free rules of the REVERSAL STAGE INFO BOX and the pane letters 1 / 2 / 3 / C of lsReversalStage 2.1.0
// (Rassul approved 2026-10-25 22:51 CT, stage-box-mockup.html; trapped thresholds from trapped/TRAPPED.md, coordinator 23:2x CT).
//
//   One box for the LATEST pivot attempt; each row: stage # . name . what to look for . check / dash . "value (threshold)".
//     1  Trap        the defenders built the floor going into the level   (the engine's Trap step: P stacking / Q held before contact)
//     2  Collision   absorption: the attack hit the floor and was taken   (the engine's Collision step: attack + A / Q held)
//     2              exhaustion: the attack pace faded to <= 0.5x          (an E letter on the attempt: pace100 <= PACE_MAX)
//     3  Turn        trapped attackers in the wick + the defenders' close  (CHECK = the HELD trigger: from the pivot bar to 3 bars after
//                    it, the first bar that closes GREEN (low; RED for a high) with no new extreme beyond the pivot before it)
//                    "Trapped <x> (thr)" = sells below the close / the 20-bar average bar volume (buys above it for a high): DESCRIPTION
//                    ONLY, never a check (TRAPPED.md: dead as a filter once close location is known); the wick volume ratio only in
//                    the hover (also description only).
//     C  Confirm     the defenders hit back (an H letter, ratio >= 1.25x) AND follow-through (the bar after the trigger makes a higher
//                    high for a low / a lower low for a high). Check only when both.
//     F? (amber)     a warning row when the Defense pane recorded F? (the pivot likely dies at once) on the pivot bar.
//   Pane letters on the pivot bar: the checked codes "1 2 3 C" in the side colour (green low, red high); nothing for unchecked rows.
//   Every row is decided from CLOSED bars and recorded letters only (append-only, first line wins), so a box never changes once its
//   window has closed (no repaint); the forming candle is never used.
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace sbx {

static const char* const VERSION = "1.0.0";
enum { LOW = 0, HIGH = 1 };
enum RowState { NOT_SEEN = 0, SEEN = 1, WARN = 2 };

// the per-market thresholds (lsFlexLevels\LRA-StageBox-<MKT>.csv: "PARAM,KEY,VALUE" or "KEY,VALUE"; built-in defaults when missing)
struct Params {
    double trapThr = 0.30, wickThr = 0.37, hbMin = 1.25, paceMax = 0.50;
    int turnBars = 3;                    // the pivot bar + the next 3 bars
    std::string why = "defaults";
};
inline Params defaultsFor(const std::string& m) {
    Params p;   // TRAPPED.md top-third cuts: trapped lots (x avg bar volume) / wick volume (x avg bar volume)
    if (m == "NQ") { p.trapThr = 0.31; p.wickThr = 0.43; }
    else if (m == "CL") { p.trapThr = 0.33; p.wickThr = 0.50; }
    else if (m == "GC") { p.trapThr = 0.33; p.wickThr = 0.48; }
    else if (m == "HG") { p.trapThr = 0.26; p.wickThr = 0.35; }
    else if (m == "NG") { p.trapThr = 0.28; p.wickThr = 0.33; }
    else if (m == "EU" || m == "6E") { p.trapThr = 0.22; p.wickThr = 0.24; }
    else { p.trapThr = 0.30; p.wickThr = 0.37; }                    // ES (and unknown markets)
    return p;
}
inline bool parseNum(const std::string& s, double& v) { if (s.empty()) return false; char* e = nullptr; const double d = std::strtod(s.c_str(), &e); if (!e || *e != 0 || !std::isfinite(d)) return false; v = d; return true; }
// one line of the params file; returns true when a known key was applied (bad values are ignored, the default stays)
inline bool parseParam(const std::string& line0, Params& p) {
    std::string line = line0; while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) line.pop_back();
    std::vector<std::string> f; size_t a = 0; while (true) { const size_t b = line.find(',', a); f.push_back(line.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
    if (f.size() == 3 && f[0] == "PARAM") f.erase(f.begin());
    if (f.size() != 2) return false;
    double v = 0; if (!parseNum(f[1], v)) return false;
    if (f[0] == "TRAP_THR" && v > 0 && v < 10) { p.trapThr = v; return true; }
    if (f[0] == "WICK_THR" && v > 0 && v < 10) { p.wickThr = v; return true; }
    if (f[0] == "HB_MIN" && v >= 1 && v < 10) { p.hbMin = v; return true; }
    if (f[0] == "PACE_MAX" && v > 0 && v <= 1) { p.paceMax = v; return true; }
    if (f[0] == "TURN_BARS" && v >= 0 && v <= 10) { p.turnBars = (int)v; return true; }
    return false;
}

// one closed candle: prices in ticks; vol = total volume; the VAP sums used by the Turn row
struct Bar {
    long long key = 0; bool ok = false;
    long long o = 0, h = 0, l = 0, c = 0;
    double vol = -1;                      // < 0 = no volume at price for this candle
    double sellsBelowClose = 0, buysAboveClose = 0, wickLow = 0, wickHigh = 0;   // wickLow = volume below min(open, close), wickHigh above max
};
// the VAP sums of one candle from its price rows (price in ticks, buy = traded at the ask, sell = traded at the bid)
struct Row { long long px = 0; double buy = 0, sell = 0, total = 0; };
inline void fillVap(Bar& b, const std::vector<Row>& rows) {
    double v = 0, sb = 0, ba = 0, wl = 0, wh = 0; const long long lo = std::min(b.o, b.c), hi = std::max(b.o, b.c);
    for (const Row& r : rows) {
        const double t = r.total > 0 ? r.total : r.buy + r.sell; v += t;
        if (r.px < b.c) sb += r.sell;
        if (r.px > b.c) ba += r.buy;
        if (r.px < lo) wl += t;
        if (r.px > hi) wh += t;
    }
    b.vol = rows.empty() ? -1 : v; b.sellsBelowClose = sb; b.buysAboveClose = ba; b.wickLow = wl; b.wickHigh = wh;
}

// a recorded letter (from the Strips / EV records): candle key, code, ratio / pace, key numbers
struct Letter { long long key = 0; int pane = 0; std::string code, nums; double ratio = -1; long long pace100 = -1, lots = -1; };
// the attempt as the 4-step engine recorded it (side, its candles, its extreme, the stage it reached, the story texts)
struct Attempt { int side = -1; long long start = 0, firstKey = 0, lastKey = 0, ext = 0; int stage = 0; std::string story, gate; };

struct BoxRow { std::string code, name, look, value; int state = NOT_SEEN; };
struct Box {
    bool on = false; int side = -1; long long pivKey = 0, ext = 0, triggerKey = 0; std::string title;
    std::vector<BoxRow> rows;            // 1, 2 absorption, 2 exhaustion, 3, C, (F?)
    std::string letters;                 // "1 2 3 C" (checked codes only)
    double trapped = -1, wick = -1, hb = -1, pace = -1; bool follow = false, decided = false;
    std::vector<std::string> hover;      // his format
};

inline std::string storyPart(const std::string& story, int k) {   // story = "trap~collision~trapped~release"
    size_t a = 0; for (int i = 0; i < k; ++i) { const size_t b = story.find('~', a); if (b == std::string::npos) return std::string(); a = b + 1; }
    const size_t b = story.find('~', a); return story.substr(a, b == std::string::npos ? std::string::npos : b - a);
}
inline std::string fx(double v, int d = 2) { char b[32]; std::snprintf(b, sizeof b, "%.*fx", d, v); return b; }
inline std::string px(long long t, double tick) {
    if (!(tick > 0)) return std::to_string(t);
    const double p = (double)t * tick; char b[48]; std::snprintf(b, sizeof b, "%.2f", p); std::string s = b;
    const size_t dot = s.find('.'); std::string ip = s.substr(0, dot), out; int n = 0;
    for (size_t i = ip.size(); i-- > 0;) { out.insert(out.begin(), ip[i]); if (++n % 3 == 0 && i > 0 && ip[i - 1] != '-') out.insert(out.begin(), ','); }
    return out + s.substr(dot);
}

// group the recorded steps into attempts: consecutive candles with the same side and start (start > 0)
template <class StepMap> inline std::vector<Attempt> attemptsOf(const StepMap& steps) {
    std::vector<Attempt> v;
    for (typename StepMap::const_iterator it = steps.begin(); it != steps.end(); ++it) {
        const auto& s = it->second; if (s.side < 0 || s.start <= 0) continue;
        if (!v.empty() && v.back().side == s.side && v.back().start == s.start) {
            Attempt& a = v.back(); a.lastKey = it->first; a.stage = std::max(a.stage, s.stage); a.ext = s.ext; if (!s.story.empty()) a.story = s.story; if (!s.gate.empty()) a.gate = s.gate;
        } else { Attempt a; a.side = s.side; a.start = s.start; a.firstKey = it->first; a.lastKey = it->first; a.ext = s.ext; a.stage = s.stage; a.story = s.story; a.gate = s.gate; v.push_back(a); }
    }
    return v;
}

// the box of one attempt. bars = closed candles in key order (the forming candle excluded); letters = all recorded letters.
inline Box build(const Attempt& A, const std::vector<Bar>& bars, const std::vector<Letter>& letters, const Params& P, double tick, const std::string& clock = std::string()) {
    Box X; if (A.side < 0 || bars.empty()) return X;
    const bool low = A.side == LOW;
    // the pivot bar: the first candle of the attempt span that made the extreme
    int iFirst = -1, iLast = -1, piv = -1;
    const long long k0 = A.start > 0 ? std::min(A.start, A.firstKey) : A.firstKey;
    for (int i = 0; i < (int)bars.size(); ++i) { if (iFirst < 0 && bars[(size_t)i].key >= k0) iFirst = i; if (bars[(size_t)i].key <= A.lastKey) iLast = i; }
    if (iFirst < 0 || iLast < iFirst) return X;
    for (int i = iFirst; i <= iLast; ++i) { const Bar& b = bars[(size_t)i]; if (!b.ok) continue; if ((low ? b.l : b.h) == A.ext) { piv = i; break; } }
    if (piv < 0) { long long best = low ? LLONG_MAX : LLONG_MIN; for (int i = iFirst; i <= iLast; ++i) { const Bar& b = bars[(size_t)i]; if (!b.ok) continue; if (low ? b.l < best : b.h > best) { best = low ? b.l : b.h; piv = i; } } }
    if (piv < 0) return X;
    const Bar& pb = bars[(size_t)piv];
    X.on = true; X.side = A.side; X.pivKey = pb.key; X.ext = low ? pb.l : pb.h;
    X.title = std::string("YOUR 3 REVERSAL STAGES" " \xB7 " "") + (low ? "low " : "high ") + px(X.ext, tick) + (clock.empty() ? std::string() : " (pivot bar " + clock + ")") + (low ? "" " \xB7 " "bullish" : "" " \xB7 " "bearish");
    // letters on this attempt's candles (from the attempt start to 5 bars after the pivot)
    const long long kLo = std::min(A.firstKey, pb.key), kHi = bars[(size_t)std::min((int)bars.size() - 1, piv + 5)].key;
    auto inSpan = [&](const Letter& L) { return L.key >= kLo && L.key <= kHi; };
    // 1 Trap
    { BoxRow r; r.code = "1"; r.name = "Trap"; r.look = low ? "Floor built: defenders' bids going in" : "Ceiling built: defenders' offers going in";
      const std::string t = storyPart(A.story, 0);
      if (A.stage >= 1 && !t.empty()) { r.state = SEEN; r.value = t; } else { r.value = "not seen"; }
      X.rows.push_back(r); }
    // 2 Collision: absorption
    { BoxRow r; r.code = "2"; r.name = "Collision"; r.look = low ? "Absorption: selling hits the floor and is taken" : "Absorption: buying hits the ceiling and is taken";
      const std::string t = storyPart(A.story, 1);
      if (A.stage >= 2) { r.state = SEEN; r.value = t.empty() ? std::string("attack met, held") : t; } else r.value = "not seen";
      X.rows.push_back(r); }
    // 2 Collision: exhaustion (E letter, pace <= PACE_MAX)
    { BoxRow r; r.code = "2"; r.look = low ? "Exhaustion: the selling fades" : "Exhaustion: the buying fades";
      double best = -1; for (const Letter& L : letters) if (L.pane == 0 && L.code == "E" && inSpan(L) && L.pace100 >= 0) { const double p = L.pace100 / 100.0; if (best < 0 || p < best) best = p; }
      X.pace = best;
      if (best >= 0 && best <= P.paceMax + 1e-9) { r.state = SEEN; r.value = "pace " + fx(best, 1) + " (" + fx(P.paceMax, 1) + ")"; }
      else r.value = best >= 0 ? "pace " + fx(best, 1) + " (" + fx(P.paceMax, 1) + ")" : "not here" " \xB7 " "look for pace <= " + fx(P.paceMax, 1);
      X.rows.push_back(r); }
    // 3 Turn: the HELD trigger (defenders' close within the pivot bar + TURN_BARS) + trapped (description only)
    double avg = 0; int na = 0; for (int i = piv - 1; i >= 0 && na < 20; --i) if (bars[(size_t)i].ok && bars[(size_t)i].vol >= 0) { avg += bars[(size_t)i].vol; ++na; }
    if (na >= 5) avg /= na; else avg = 0;
    if (avg > 0 && pb.vol >= 0) { X.trapped = (low ? pb.sellsBelowClose : pb.buysAboveClose) / avg; X.wick = (low ? pb.wickLow : pb.wickHigh) / avg; }
    int trig = -1; bool windowClosed = false;
    for (int i = piv; i < (int)bars.size() && i <= piv + P.turnBars; ++i) {
        const Bar& b = bars[(size_t)i]; if (!b.ok) continue;
        if (i > piv && (low ? b.l < X.ext : b.h > X.ext)) break;          // a new extreme first: no turn here (a new attempt)
        if (low ? b.c > b.o : b.c < b.o) { trig = i; break; }
        if (i == piv + P.turnBars) windowClosed = true;
    }
    if (trig < 0 && (int)bars.size() - 1 >= piv + P.turnBars) windowClosed = true;
    { BoxRow r; r.code = "3"; r.name = "Turn"; r.look = low ? "Trapped sellers in the wick + GREEN close" : "Trapped buyers in the wick + RED close";
      std::string tr = X.trapped >= 0 ? "trapped " + fx(X.trapped) + " (" + fx(P.trapThr) + ")" : std::string("trapped ?");
      if (trig >= 0) { r.state = SEEN; X.triggerKey = bars[(size_t)trig].key; r.value = tr + "" " \xB7 " "close " + px(bars[(size_t)trig].c, tick) + "" " \xB7 " "HELD"; }
      else r.value = tr + (windowClosed ? "" " \xB7 " "no defenders' close" : "" " \xB7 " "waiting for the close");
      X.rows.push_back(r); }
    // C Confirm: H ratio >= HB_MIN and follow-through on the bar after the trigger
    { BoxRow r; r.code = "C"; r.name = "Confirm"; r.look = low ? "Buyers hit back, follow-through" : "Sellers hit back, follow-through";
      double hb = -1; for (const Letter& L : letters) if (L.pane == 1 && L.code == "H" && L.key >= pb.key && L.key <= kHi && L.ratio > 0) hb = std::max(hb, L.ratio);
      X.hb = hb;
      if (trig >= 0 && trig + 1 < (int)bars.size()) { const Bar& n = bars[(size_t)trig + 1]; const Bar& t = bars[(size_t)trig]; X.follow = n.ok && (low ? n.h > t.h : n.l < t.l); }
      const bool hbOk = hb >= P.hbMin - 1e-9;
      std::string v = hb > 0 ? "Hb " + fx(hb, 1) + " (" + fx(P.hbMin) + ")" : "Hb not seen (" + fx(P.hbMin) + ")";
      v += trig < 0 ? "" " \xB7 " "no trigger yet" : (trig + 1 < (int)bars.size() ? (X.follow ? (low ? "" " \xB7 " "next bar higher high" : "" " \xB7 " "next bar lower low") : "" " \xB7 " "no follow-through") : "" " \xB7 " "waiting for the next bar");
      if (hbOk && X.follow) r.state = SEEN;
      r.value = v;
      X.rows.push_back(r); }
    // F? warning (recorded by the Defense pane on the pivot bar)
    for (const Letter& L : letters) if (L.pane == 1 && (L.code == "F?" || L.code == "F") && L.key == pb.key) {
        BoxRow r; r.code = "F?"; r.name = "Warning"; r.look = "Pivot likely dies at once: skip / wait"; r.state = WARN; r.value = L.nums.empty() ? std::string("F? on the pivot bar") : L.nums; X.rows.push_back(r); break; }
    // pane letters
    std::string let; for (const BoxRow& r : X.rows) if (r.state == SEEN && (r.code != "2" || let.find('2') == std::string::npos)) { if (!let.empty()) let += ' '; let += r.code; }
    X.letters = let;
    X.decided = windowClosed || (trig >= 0 && trig + 1 < (int)bars.size());
    // the hover (his format)
    X.hover.push_back("Question: how far has this " + std::string(low ? "low" : "high") + " got through your 3 stages?");
    X.hover.push_back("Answer: " + (let.empty() ? std::string("no stage seen yet") : "stages " + let + " seen") + "; trapped " + (X.trapped >= 0 ? fx(X.trapped) : std::string("?")) + " (" + fx(P.trapThr) + "), wick volume " + (X.wick >= 0 ? fx(X.wick) : std::string("?")) + " (" + fx(P.wickThr) + ") - both description only, no check.");
    X.hover.push_back("How to use: 1 + 2 = the floor was built and used; 3 = the defenders closed the bar their way (the HELD trigger); C = they hit back and price followed. An amber F? = skip / wait.");
    X.hover.push_back(std::string("Example: ES 10-22 1:04 PM low - 1 bids beat offers going in, 2 2,169 lots taken (Q 117%), 3 green close 7,684.25 = HELD, C Hb 1.5x + higher high."));
    return X;
}

} // namespace sbx
#endif
