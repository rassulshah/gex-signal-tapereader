#ifndef LS_DPBANNER_H
#define LS_DPBANNER_H
// DpBanner.h 1.0.0 (2026-10-28 overnight, Rassul 22:31 CT "lets update the top read to be a delta profile read", OK'd 22:37 CT; mockup
// https://claude.ai/artifact/V3Jzmce8BZC2kDzaPBTHz6 section 4) -- SDK-free.
// THE TOP READ = the Delta Profile's latest decided read (SignalBoard 'R' from lsDeltaProfile: code 1 A, 3 A?, 4 A+; val = the ratio),
// shown only while FRESH: decided within FRESH_BARS closed candles and not invalidated (no close beyond the node against it). The detail
// line carries only facts tested useful, "?" where unproven: the attack ratio + the close, the defenders at the node ('Y' from
// lsDeltaProfile's node read), the trapped side ('T' from lsTrappedTraders, unproven -> "?"). Nothing fresh = a quiet grey line.
// One owner per fact: this only READS the board (ChainSpec: lsDeltaProfile.READ -> lsLiquidityProfile.Banner, DISPLAY).
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <functional>
#include "SignalBoard.h"
namespace dpb {
static const char* const VERSION = "1.1.0";   // (1.1.0, 2026-10-28) + the hourly flow (kind H from lsDeltaProfile 2.30.0): "with 4h flow (buyers), pullback ?"
static const int FRESH_BARS = 4;
struct Banner { bool fresh = false, any = false; std::string name, detail; int favour = 0; double nodePx = 0; };
inline std::string fx1(double v) { char b[32]; std::snprintf(b, sizeof(b), "%.1fx", std::fabs(v)); return b; }
inline std::string px(double v, double tick) {
    int d = 0; double t = tick; while (d < 6 && t > 0 && std::fabs(t - std::round(t)) > 1e-9) { t *= 10; ++d; }
    char b[48]; std::snprintf(b, sizeof(b), "%.*f", d, v); return b;
}
inline std::string clock12(long long ct) { const long long sd = ((ct % 86400) + 86400) % 86400; const int h = (int)(sd / 3600), m = (int)(sd % 3600 / 60);
    char b[24]; std::snprintf(b, sizeof(b), "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, h < 12 ? "AM" : "PM"); return b; }
// R / Y / T facts (decided), the last CLOSED candle's Central end second + close, seconds per bar, tick, the level name at a price ("" none)
inline Banner build(const std::vector<sb::Fact>& R, const std::vector<sb::Fact>& Y, const std::vector<sb::Fact>& T, const std::vector<sb::Fact>& H,
                    long long lastCt, double lastClose, long spb, double tick, const std::function<std::string(double)>& levelAt) {
    Banner b; if (spb <= 0 || !(tick > 0)) return b;
    const sb::Fact* best = nullptr;
    for (const sb::Fact& f : R) { if (f.code != 1 && f.code != 3 && f.code != 4) continue; if (f.bar > lastCt) continue; if (!best || f.bar > best->bar) best = &f; }
    if (!best) return b;
    b.any = true; const bool bull = best->side > 0; const double node = (best->lo + best->hi) / 2; b.nodePx = node;
    const long long ago = (lastCt - best->bar) / spb;
    const bool broken = bull ? lastClose < best->lo - tick * 0.5 : lastClose > best->hi + tick * 0.5;
    const std::string code = best->code == 3 ? "A?" : best->code == 4 ? (best->proven == 1 ? "A+" : "A+?") : "A";
    if (ago > FRESH_BARS || broken) {
        b.name = "NO FRESH DELTA READ"; b.favour = 0;
        b.detail = "last " + code + ": " + px(node, tick) + " at " + clock12(best->bar) + ", " + std::to_string(ago) + " candles ago" + (broken ? ", since broken" : "");
        return b;
    }
    b.fresh = true; b.favour = bull ? +1 : -1;
    const std::string lv = levelAt ? levelAt(node) : std::string();
    b.name = code + (bull ? " BUYERS" : " SELLERS") + " ABSORBED AT " + px(node, tick) + (lv.empty() ? std::string() : " (" + lv + ")");
    // the attackers' side hit the node; the defenders (who absorbed) are the read's side
    b.detail = std::string(bull ? "sellers" : "buyers") + " hit " + fx1(best->val) + " \xB7 closed " + (bull ? "green above" : "red below") + (best->code == 3 ? " on the NEXT candle" : "");
    for (const sb::Fact& y : Y) if (y.bar == best->bar && y.side == best->side && y.code % 10 != 0) {
        const bool hold = y.code % 10 == 1; char v[32]; std::snprintf(v, sizeof(v), "%+.1fx", y.val);
        b.detail += std::string(" \xB7 ") + (bull ? "bids " : "offers ") + (hold ? "holding " : "pulling ") + v + (y.proven == 1 ? "" : " ?"); break; }
    for (const sb::Fact& t : T) if (t.side == best->side && t.bar >= best->bar - spb && t.bar <= best->bar + 2 * spb) {
        b.detail += std::string(" \xB7 ") + (bull ? "sellers" : "buyers") + " trapped " + fx1(t.val) + " ?"; break; }
    // (1.1.0) the hourly flow at the read's candle (H code 1 = a pullback in its favour: the A's side won the 4 closed hours, the last hour went
    // against it - the research: turn 82% vs 71%, swing 57% vs 46%; "?" until the nightly proves it per market)
    for (const sb::Fact& h : H) if (h.bar == best->bar && h.side == best->side && h.code == 1) {
        b.detail += std::string(" \xB7 with 4h flow (") + (bull ? "buyers" : "sellers") + "), pullback" + (h.proven == 1 ? "" : " ?"); break; }
    return b;
}
// (1.1.0) the 1.0.0 call (no hourly flow)
inline Banner build(const std::vector<sb::Fact>& R, const std::vector<sb::Fact>& Y, const std::vector<sb::Fact>& T,
                    long long lastCt, double lastClose, long spb, double tick, const std::function<std::string(double)>& levelAt) {
    return build(R, Y, T, std::vector<sb::Fact>(), lastCt, lastClose, spb, tick, levelAt);
}
}  // namespace dpb
#endif
