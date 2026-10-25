#ifndef LS_LIQUIDITY_PROFILE_LOGIC_H
#define LS_LIQUIDITY_PROFILE_LOGIC_H
// (0.15.0, 2026-10-13, Rassul approved the v2 mockup "liquidity-before-after-v2" at 6:44 PM CT) the HOVER of each NEAR zone is built
//   around its purpose - support / resistance - one line per question (nothing new on the chart face):
//   1 "Support|Resistance [at <LEVEL>] lo - hi \xB7 N lots (X.Xx)"   green support / red resistance (the level = the one the zone holds)
//   2 "Orders N \xB7 avg X lots \xB7 Top order NN%"                  white - resting orders in the zone from the order-by-order book
//   3 "Mostly one order - may not reload ?"                           white, when the top order is 70%+ of the zone (replaces 'may be
//                                                                      bait': his data - such orders were filled, not pulled; '?' until
//                                                                      proven per market)
//   4 "As price came: up NN% - building" | "down NN% - leaving" | "unchanged"   the zone's lots now vs its lots when price first came
//                                                                      within reach (the NEAR rule); +/-10% band; baseline recorded ONCE
//                                                                      per approach (reset when the zone stops being near). Colour by
//                                                                      implication: support building = green, support leaving = red,
//                                                                      resistance building = red, resistance leaving = green, else white
//   5 "Hit N lots \xB7 rebuilt NN% - absorbing"                       only while price is IN the zone (open attempt, lots hit > 0);
//                                                                      green at support, red at resistance; measurement only
//   The makeup (line 2 / 3) shows only when the orders the feed knows cover 80-125% of the depth's lots (orders resting before load
//   are unknown). Decision time only: an attempt's hit / rebuilt stop counting when it is decided and its last makeup + the 'as price
//   came' change at arrival are recorded then (<MKT>-<sym>-<spb>-makeup-<session>.csv), never changed afterwards (no repaint).
// LiquidityProfileLogic.h  (lsLiquidityProfile 0.1.0, 2026-10-11) -- the SDK-free engine of the Liquidity Profile. Tested by
// test_liquidityprofile_logic.cpp (g++ / MSVC, no Investor/RT needed).
// (0.11.4, 2026-10-12 review) finTotal (windows past the 50th touch), no zone flicker at the book's edge, LQ hysteresis 2.5x / 2.1x,
// drawOrder (overlapping zones) and Lanes (labels never overlap), price decimals from IRT's float tick.
//
// WHAT IT DOES (Rassul 2026-10-10: "get the liquidity profile done and the signals that support absorption ... so i can trade monday"):
//   1. BOOK    the resting size per price on each side, from the chart's own depth (MARKET_DEPTH snapshots; native, no outside
//              dependency). Order-by-order (DBO) events are only RECORDED for now: their Rithmic / CQG semantics (does a FILL also
//              delete? is a DEL a cancel?) are not verified yet, so they never move the book here (fail closed).
//   2. TRADES  aggressive volume per price from the chart's own volume-at-price (VAP) of the bar in progress: the increase since the
//              last read, buy = lifted the offer, sell = hit the bid.
//   3. FLOW    between two book reads, per price: executed (trades against that side), PULLED (size that left without trading),
//              ADDED, REFILL (traded more than was shown = reloaded). Only prices inside BOTH reads' depth window are counted, so a
//              level that scrolls out of a 10-level book is never mistaken for a pull.
//   4. ZONES   the size summed over a zone-wide band (width 2*hw+1 ticks, hw = clamp(round(k*ATR/tick), hwMin, hwMax) - the study's
//              MEDIUM zone per market, lra.mbo_fetch.ZONES) at every price; the top N local peaks per side within reach; tracked
//              by centre (drift <= hw). x normal = size / the normal band size for that 30-min slot (the nightly study's walk-
//              forward median, LRA-LiqNorms-<MKT>.csv), else the session's own median band size so far.
//   5. TOUCH   price trades into a durable zone (rested >= persist) from outside on the attacking side -> an ATTEMPT:
//              shown = the zone's size at the last book before arrival; then eaten = attackers' volume in the zone / shown,
//              stayed = 1 - pulled / shown, E = exhaustion (attackers' 2nd-half volume < 0.5 x their 1st half since arrival),
//              resilience = how much of what the zone lost was rebuilt.
//   6. MARKS   (0.5.0) two pane signals, decided on CLOSED bars only and never changed afterwards (no repaint):
//              Exh  (aggression pane) the attackers' 2nd-half / 1st-half volume since arrival under 0.5, drawn with that number
//              Pull (defense pane) the defenders pulled this market's share of the shown size, not replaced, while attacked
//              Each is drawn only in markets where the nightly study confirmed it (SIGNAL lines); unconfirmed = not drawn.
//              Holds / breaks / expiries and the O / R crosses are logged for the study, never drawn.
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <locale>
#include <deque>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "ZoneRule.h"           // (0.17.0) Rassul's zone rule (held / broke), ported from lra.level_confluence

namespace lqp {

static const char* const VERSION = "0.54.0";   // (0.54.0) = LQ_VERSION (LmtZonesLogic 1.3.0: after-fill read RE-BID / ACCEPTED, HELD? not drawn)   // (0.53.0) = LQ_VERSION again (LmtZonesLogic 1.2.0: grey map lines + FILLING / HELD? tags)   // (0.51.0) QueueMeter 1.3.0 + LmtZones 1.1.0;   // (0.50.0) the queue meter 10-s call (QueueMeter 1.1.0)   // (0.49.0) the QUEUE METER engine (QueueMeter.h) feeding the Dealer Profile's Levels column   // (0.48.0) the LMT zones (LmtZonesLogic.h) on the price chart + LMT / LMT F columns   // (0.47.1) the big-order lines stop at the HOD/LOD table (display only)   // (0.47.0, MBO roadmap Middle Ahead #1, Rassul 14:25 CT option A) the AIR POCKET: after a 5-candle turn, the resting size between the close and T1 (1 ATR) fell to <= 0.67x of a minute before by PULLS (not trades) with close-back >= 0.5 -> a green / red band close..T1 with an "AP?" badge in the column while live (ends at T1 / size back / 15 min; decided once, never repainted; AP / APE records per session; setting air_pocket, AirPocketLogic.h); (0.46.1, 2026-10-20 night) the book object (MARKET_DEPTH) parked at done() / destroy() - IRT faulted inside its destructor at every quit; (0.46.0, Rassul 2026-10-19 23:38 "ill keep lmt F and remove the others. build") the right-hand column is LMT F only (64 px, was MKT | LMT F | LMT R 168 px); MKT and LMT R not drawn (still computed and recorded); the S5 / S10 / S20 / mu footer stays under LMT F; // (0.45.3, Rassul 23:37 CT 2026-10-18 "get rid of the hover over on the gex levels and other places. make the colors in the mkt and lmt f lighter, i can barely see them") price-chart hovers OFF (LiquidityProfile.cpp LQ_HOVERS, code kept; no TRACK_MOUSE on the price chart; the Flow pane keeps its candle-follow); MKT / LMT F / LMT R text inks lighter (colInk: 2SD+ 0x86EFAC / 0xFCA5A5, 1SD / older than 30 min 0x4ADE80 / 0xF87171 - every ink >= 6.5:1 on the column background, was 2.3-3.4:1 for the dim tier); (0.45.2, Rassul 20:04 2026-10-18 "MKT column values are flashing ... they come on and then go away") the MKT tiers come from ONE source per calc (mktTiersPick: the 5-session pool, kept between its 5-s recomputes, else the session's own stats) - 0.38-0.45.1 used the pool on 1 calc in 5 s and the session's own 1SD on the others, so an order between the two cuts blinked; a printed MKT order is held (bfl::Mkt::marksHeld); LMT F coloured by the RESTING side (lmtFillGreen); (0.45.1, 2026-10-18 his decision list) label Offset X / Y removed (a saved offset is ignored, saved as 0); "Ended lines kept per side" hidden (table default 10 / the file value); (0.45.0, 2026-10-18 settings audit) the settings windows (Profile + Flow) from LiquidityProfileSettings.h through SettingsHost.h: same rows / order / files; the cut-off big-order line split; "Big resting orders" / "Verdict banner" grey their rows; Font 10-16; the chart copy never saves;  (0.44.2, 2026-10-18) the Flow build links again (band pool calls only in the Profile build); (0.44.1, 2026-10-18 settings audit) Banner background colour filled / read with setParameterColor / getParameterColor (setIntegerValue / getIntegerValue showed a black swatch and saved 0) //   // (0.44.0, Rassul 2026-10-17 night "yes 2x") the column DRAWS a zone only at >= 2.0x (drawX; no 1.5x near entry, no keep to 1.2x / 1.7x, no any-size during absorption): it appears live once durable at 2.0x and is removed only at a BAR CLOSE where it measures under 2.0x (no flicker); the old shown state still drives the attempts / records (logging unchanged); the zone normal says its source (norms file slot / 5-session pool slot / session median) and slots missing from the norms file (overnight) come from a 5-session per-slot pool (BandPool); MenthorQ rows must match the chart contract and be <= 18 h old (sameContract / mqFresh); Market State LIQ / LOTS fact builders (liqKv / lotsKv); lqp::VERSION now equals LQ_VERSION;   // (0.32.3-0.43.x lived only in LQ_VERSION)   // (0.32.2) market-order log from the market's own start size when under 10 lots (NQ);   // (0.32.1) learning rows show the thresholds in use (Big~ / 1-3SD~) + the count; darker, less grey column background;   // (0.32.0) raw tracking of MKT / LMT / FP / DP together: -mktorders-, -iceorders-, -absflow- files (studies find the best combination per market);   // (0.28.0) the MKT half of the column (big market orders) + iceberg "i" boxes in BIG (BigFlow.h);   // (0.27.1) the badge / BIG column 3 px under the pane top (was 20);   // (0.27.0) a pulled opposite big order no longer qualifies an initiative verdict (5 / 22 on ES); Pull % capped at 100%; Attack / pace against the same 30-min slot (log mean of that slot's earlier bars, 10+), else the 20 bars before;   // (0.26.0) BIG column on the big-lot distribution: boxes from 1SD (dim) / 2SD (full) / 3SD (outline), reference Big mu + 1-3SD at the column bottom;   // (0.25.0) BIG column 3 tiers: biggest within reach dimmed, big full colour, huge + white outline;   // (0.24.0) one combined badge: verdict on top (REV UP / DOWN FAILING ?), book state below in its own colour, grey lines verdict side first, levels by name;   // (0.23.0) Big order font 7-16 (default 10) drives the BIG column box and its filled marks;   // (0.22.9) an opposing warning fails the verdict only at the turn and after 15 s; CSV-safe warning names;   // (0.22.8) verdict log to the second with seconds-into-bar, the warning and the band (verdicts2);   // (0.22.7) the verdict is sticky (no REV / NONE flicker);   // (0.22.6) a warning on the other side of the verdict makes it FAILING ? (coordinated);   // (0.22.5) BIG column 56 px wide, from the top of the chart;   // (0.22.4) banner / verdict badge names centred vertically;   // (0.22.3) a big order placed in the current bar gets its stack / pull line at once;   // (0.22.2) verdict only at the 10-bar extreme (no chasing); "big bid filled N (sellers hit it)";   // (0.22.1) every verdict change logged (<MKT>-...-verdicts-<session>.csv);   // (0.22.0) verdict also on INITIATIVE at the session extreme (bold lift / hit) with the book on its side or the opposite big order pulled;   // (0.21.0) SignalBoard: the verdict uses the Delta Profile A, Big Bid/Ask circles and the big fills together;   // (0.20.1) level badges use the drawn zone's own lots / x (in sync with the column);   // (0.20.0) banner: badge + 2 grey lines aligned inside its height, level named first; verdict badge REV UP / REV DOWN / FAILING ? below during a turn;   // (0.19.2) the big size tunes itself from the last 60 min (BigOrderLines 1.2.0);   // (0.19.1) BIG column fills in the bar colours (bid red, ask green);   // (0.19.0, Rassul 2026-10-15) the BIG column + 2 stack / pull lines (no labels on the chart), large = floor (nightly hourly norms) AND 3x the local book;   // (0.18.0, 2026-10-14 night) the biggest resting order lines (BigOrderLines.h) + the warning detail on two lines (warnLines);   // (0.17.0, 2026-10-14 night, Rassul approved 18:25 CT) Held / Broke by the ZONE rule (ZoneRule.h: centre +/- 0.06 ADR, held = a close 0.10 ADR back out, broke = a close 0.10 ADR beyond with no close back inside in 15 min; never a tick-through or a first touch), labels only once resolved; audit wording: "Hit N lots \xB7 NN% re-added ?", LQ? <LEVEL>, no intent words;   // (0.16.1, 2026-10-14 evening) rebuilt % <= 100%: an add is credited only at a hit price, up to the lots hit there and not yet restored (was 200% on every ES attempt: every queue join in the zone counted);   // (0.16.0, CR5200) the solid left column (20-200 px, 60), lots inside in white, the per-level liquidity for the badges   //   // (0.15.1a, CR5000) Tape Reader heartbeat, label Offset X / Y; (0.15.1) Width / Font kept per market
typedef long long Tick;
typedef long long Ms;
enum Side { BID = 0, ASK = 1 };                 // BID = buy orders below price (support), ASK = sell orders above (resistance)

struct Params {
    double tick = 0.01;
    double k = 0.25; int hwMin = 4, hwMax = 8;  // MEDIUM zone half-width rule in ticks (CL default; the norms file sets it per market)
    int topN = 3;                               // zones per side
    double reachAtr = 6.0;                      // zones are looked for within reachAtr x ATR of price
    Ms persistMs = 30000;                       // a zone must have rested 30 s (durable, not flicker) to be bright / touchable
    Ms graceMs = 10000;                         // a tracked zone not seen for 10 s is dropped (unless in an attempt)
    Ms decisionMs = 180000;                     // the study's decision time: arrival + 3 min
    int maxBars = 20;                           // an attempt lives 20 bars after arrival
    double eatenBreak = 1.0;                    // LX: eaten >= 1x the size shown (Q02 / Q17)
    double exhaust = 0.5;                       // E: attackers' 2nd-half volume < 0.5 x the 1st half (Q22)
    double exhShow = 0.5;                       // (0.4.1, Rassul 2026-10-10 23:44 "i want to see weakness, especially under .5") Exh prints under this
    double pullShare = 0.5;                     // (0.3.1) PULL? / P / LX P: share of the shown size pulled. eatenBreak, exhaust and pullShare are
                                                // set PER MARKET from the data (LRA-LiqNorms-<MKT>.csv PARAM lines, lra.liq_params: chosen on the
                                                // build days, kept only if the later validate days agree; else these defaults)
    Ms oldMs = 600000;                          // (0.5.0, Rassul 2026-10-11 "just add the old order mark") OLD = resting 10 min+
    double oldShow = 0.5;                       // a zone is marked old when half+ of its size is old (study old_share >= 0.5, Q-old)
    // (0.8.0, Rassul 2026-10-11 "now we need something that shows sellers stepped in after that" / "use s" / "rename step in to counter")
    // s = COUNTER: after e, a bar where the defenders out-traded the attackers near the zone at counterX x their normal pace per tick
    double counterX = 1.25;                     // the line (PARAM counter_x per market, lra.liq_params; this default until the data sets it)
    int counterBars = 10;                       // s is looked for up to 10 bars after the e bar (lra.liq_params COUNTER_BARS)
    Ms barMs = 180000;                          // the chart's bar length (the STEPPING IN warning stays 2 bars after an s)
    double minShown = 1.0;                      // an attempt needs some size shown at arrival
    double minAttack = 1.0;                     // E needs attackers in the first half
    // (0.10.0, Rassul 2026-10-11 18:21-18:51 rules review + research "Liquidity display for absorption trading"): a zone is DRAWN only
    // when it stands out from the book BEHIND it (further from price): the 8 same-width bands beyond it, at least 6 of them inside the
    // visible book. Shown at >= showX x their median AND the biggest of them (rank 1); kept while >= keepX (hysteresis), removed after
    // graceMs below. A key / gamma level is liquidity-CONFIRMED when a shown zone >= confirmX sits on it. (Study: < 1x held 23%,
    // 1.5-3x 34-37%; 2x shows 12% of zones; level confirmation knee at 2.5x: 54% vs 44%, confluence 71% vs 51%.)
    bool requireShown = false;                  // the plugin sets true: only drawn zones are touchable (tests keep the 0.9 behaviour)
    double showX = 2.0, keepX = 1.7, confirmX = 2.5;
    double confirmKeepX = 2.1;                  // (0.11.4) an LQ already on a zone stays while that zone is shown at 2.1x+ (no flicker at 2.5x)
    int behindBands = 8, behindMin = 6;
    // (0.14.0, Rassul 2026-10-13 "i want to see liquidity when it is close so i can see if there is any absorbtion going on")
    // NEAR rule: a zone within nearAtr x the 3-min range of price (at least 4 half-widths) is shown at nearShowX (1.5x) WITHOUT the
    // rank-1 requirement (still behindOk + durable 30 s), kept while >= nearKeepX (1.2x), same 10 s grace. It becomes near at
    // distance <= nearTicks and stops being near only past nearExit x nearTicks (no flicker at the boundary). Far zones unchanged
    // (2.0x, rank 1, kept to 1.7x). Never shown under minShowX (1.0x). Research basis: zones of 1.5-2.0x held 34% vs 35% at 2-3x,
    // under 1x 23% - the 1.5x near line loses almost nothing in holding odds and shows the liquidity price is about to test.
    double nearAtr = 1.0, nearShowX = 1.5, nearKeepX = 1.2, nearExit = 1.5;
    double minShowX = 1.0;
    // (0.44.0, Rassul 2026-10-17 night "yes 2x" - his 10-16 rule "liquidity only at levels when 2x+ normal") the DRAW gate, separate from
    // 'shown' above (which still decides which zones are touchable / recorded, so the records do not change): a zone is DRAWN once it is
    // durable, behindOk, rank 1 or near, and >= drawX x the book behind it (bx, the ratio printed in the node); a drawn zone is removed
    // only at a closed bar where it measures under drawX. No near 1.5x entry, no keep line under 2.0x, no any-size during absorption.
    double drawX = 2.0;
    int nearPerSide = 2;                        // the strongest near candidates per side always tracked (even outside topN)
    Tick reachTicks = 0;                        // > 0: the reach in ticks set by the plugin (1/2 expected range ...), else reachAtr x ATR
    double oneOrderShare = 0.70;                // (0.15.0) the top order's share of the zone from which the hover says "Mostly one order"
    double cameBand = 0.10;                     // (0.15.0) 'as price came': +/- this share of the baseline = building / leaving
    double coverMin = 0.80, coverMax = 1.25;    // (0.15.0) the feed's known lots / the depth's lots for the makeup to be shown
};

// (0.11.4) the decimals a price needs at this tick, tolerant of IRT's float tick (0.01f = 0.0099999998: 6 decimals / 3 decimals before)
inline int decimals(double tick) {
    if (!(tick > 0)) return 2;
    double k = tick; for (int d = 0; d <= 6; ++d, k *= 10) if (std::fabs(k - (double)std::llround(k)) < 1e-6) return d;
    return 6;
}
inline bool finite(double v) { return v == v && v < std::numeric_limits<double>::infinity() && v > -std::numeric_limits<double>::infinity(); }

// Tick keys are the native comparison grid.  Never cast an unbounded double to
// int64: llround/casting outside its representable domain is undefined.
inline bool tryToTick(double price, double tick, Tick& out) {
    if (!finite(price) || !finite(tick) || tick <= 0) return false;
    const double q = price / tick;
    const double lim = static_cast<double>(std::numeric_limits<Tick>::max());
    if (!finite(q) || q <= -lim || q >= lim) return false;
    const double r = std::round(q);
    if (!finite(r) || r <= -lim || r >= lim) return false;
    out = static_cast<Tick>(r);
    return true;
}
inline Tick toTick(double price, double tick) { Tick out = 0; return tryToTick(price, tick, out) ? out : 0; }

inline int halfWidth(const Params& p, double atrPrice) {
    double a = atrPrice > 0 && finite(atrPrice) ? atrPrice : 15 * p.tick;
    const double raw = p.k * a / p.tick;
    if (!finite(raw) || raw >= p.hwMax) return p.hwMax;
    if (raw <= p.hwMin) return p.hwMin;
    return std::max(p.hwMin, std::min(p.hwMax, (int)std::llround(raw)));
}

// ---- (0.44.0) the 5-session per-slot pool of band sizes, learned live (audit LP-3: the norms file holds RTH slots only, so overnight
// the "normal" was the median of every band this session - NQ 3 lots - while the status said "norms file"). Each session keeps, per
// 30-min Central slot, a log-bucket histogram (5 % buckets) of the band sizes the zones were picked from; when the session ends its
// slot medians join the pool. Only sessions that sampled the slot (n >= MIN_N) count; the 5 newest sessions WITH data are kept
// (empty sessions - weekends, holidays, a chart without depth - are never counted: the LP-1 lesson). Normal for a slot = the median
// of its per-session medians, from 2+ sessions. Saved / loaded as text: "BP1" then "S,<session>,<slot>,<median>,<n>" rows.
struct SlotHist {
    std::vector<unsigned> c; long long n = 0;
    static int bucket(double x) { const double b = std::log(x) / std::log(1.05); return b < 0 ? 0 : b > 400 ? 400 : (int)b; }
    void add(double x) { if (!(x > 0) || !finite(x)) return; const int k = bucket(x); if ((int)c.size() <= k) c.resize((size_t)k + 1, 0); c[(size_t)k]++; ++n; }
    double median() const {
        if (n <= 0) return 0;
        long long acc = 0; const long long half = (n + 1) / 2;
        for (size_t k = 0; k < c.size(); ++k) { acc += c[k]; if (acc >= half) return std::pow(1.05, (double)k + 0.5); }
        return 0;
    }
};
struct BandPool {
    static const int KEEP = 5, MIN_SESS = 2; static const long long MIN_N = 60;
    std::map<long long, std::map<int, std::pair<double, long long> > > past;   // session -> slot -> (median, samples)
    long long curSession = -1; std::map<int, SlotHist> cur;
    void roll() {                                                              // the current session's slot medians join the pool
        std::map<int, std::pair<double, long long> > m;
        for (std::map<int, SlotHist>::const_iterator it = cur.begin(); it != cur.end(); ++it) if (it->second.n >= MIN_N) m[it->first] = std::make_pair(it->second.median(), it->second.n);
        if (!m.empty() && curSession >= 0) past[curSession] = m;
        cur.clear();
        while ((int)past.size() > KEEP) past.erase(past.begin());
    }
    void sample(long long session, int slot, double band) {
        if (session < 0 || slot < 0 || slot > 47 || !(band > 0)) return;
        if (session != curSession) { if (curSession >= 0 && session > curSession) roll(); else cur.clear(); curSession = session; }
        cur[slot].add(band);
    }
    // the pooled normal for a slot from the finished sessions (never the current one); nSess = how many sessions it came from
    bool at(int slot, double& v, int& nSess) const {
        std::vector<double> m;
        for (std::map<long long, std::map<int, std::pair<double, long long> > >::const_reverse_iterator s = past.rbegin(); s != past.rend() && (int)m.size() < KEEP; ++s) {
            if (s->first == curSession) continue;
            std::map<int, std::pair<double, long long> >::const_iterator q = s->second.find(slot);
            if (q != s->second.end() && q->second.second >= MIN_N && q->second.first > 0) m.push_back(q->second.first);
        }
        nSess = (int)m.size(); v = 0;
        if ((int)m.size() < MIN_SESS) return false;
        std::sort(m.begin(), m.end()); const size_t n = m.size();
        v = n % 2 ? m[n / 2] : 0.5 * (m[n / 2 - 1] + m[n / 2]); return v > 0;
    }
    std::string text() const {                                                 // the pool + the current session so far (a restart keeps it)
        std::map<long long, std::map<int, std::pair<double, long long> > > all = past;
        if (curSession >= 0) { std::map<int, std::pair<double, long long> > m;
            for (std::map<int, SlotHist>::const_iterator it = cur.begin(); it != cur.end(); ++it) if (it->second.n >= MIN_N) m[it->first] = std::make_pair(it->second.median(), it->second.n);
            if (!m.empty()) all[curSession] = m; }
        while ((int)all.size() > KEEP + 1) all.erase(all.begin());
        std::ostringstream o; o.imbue(std::locale::classic()); o << "BP1\n";
        for (std::map<long long, std::map<int, std::pair<double, long long> > >::const_iterator s = all.begin(); s != all.end(); ++s)
            for (std::map<int, std::pair<double, long long> >::const_iterator q = s->second.begin(); q != s->second.end(); ++q) {
                char b[96]; std::snprintf(b, sizeof(b), "S,%lld,%d,%.2f,%lld\n", s->first, q->first, q->second.first, q->second.second); o << b;
            }
        return o.str();
    }
    // a saved pool; the CURRENT session's saved row is not restored as finished (it is still being sampled; its slots come back as it runs)
    bool parse(const std::string& body, long long nowSession) {
        std::istringstream in(body); in.imbue(std::locale::classic()); std::string ln; bool head = false; int lines = 0;
        std::map<long long, std::map<int, std::pair<double, long long> > > p;
        while (std::getline(in, ln) && ++lines < 5000) {
            if (!ln.empty() && ln.back() == '\r') ln.pop_back();
            if (!head) { if (ln != "BP1") return false; head = true; continue; }
            if (ln.compare(0, 2, "S,") != 0) continue;
            char* e = nullptr; const char* c = ln.c_str() + 2; errno = 0;
            const long long ses = std::strtoll(c, &e, 10); if (e == c || *e != ',' || errno) continue; c = e + 1;
            const long slot = std::strtol(c, &e, 10); if (e == c || *e != ',' || slot < 0 || slot > 47) continue; c = e + 1;
            const double med = std::strtod(c, &e); if (e == c || *e != ',' || !finite(med) || med <= 0 || med > 1e9) continue; c = e + 1;
            const long long n = std::strtoll(c, &e, 10); if (e == c || (*e != 0) || n < MIN_N || n > 4000000000LL) continue;
            if (ses <= 0 || ses == nowSession) continue;
            p[ses][(int)slot] = std::make_pair(med, n);
        }
        if (!head) return false;
        while ((int)p.size() > KEEP) p.erase(p.begin());
        past.swap(p); return true;
    }
};
enum NormSrc { NS_NONE = 0, NS_FILE = 1, NS_POOL = 2, NS_SESSION = 3 };
inline const char* normSrcName(int s) { return s == NS_FILE ? "norms file slot" : s == NS_POOL ? "5-session pool slot" : s == NS_SESSION ? "session median (fallback)" : "none yet"; }

// ---- normal band size per 30-min Central slot (slot = minute of day / 30): (0.44.0) the nightly file's slot, else the 5-session pool's
// slot (overnight: the file has RTH slots only), else this session's median of every band (labelled as the fallback it is)
struct Norms {
    std::map<int, double> median;               // slot -> median band size (contracts)
    bool loaded = false;
    std::vector<double> session;                // fallback: band sizes sampled this session (capped)
    BandPool pool;                              // (0.44.0)
    double sessionMedian() const {
        if (session.size() < 30) return 0;
        std::vector<double> v(session); std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
        return v[v.size() / 2] > 0 ? v[v.size() / 2] : 0;
    }
    // (0.44.0) the normal for a slot and where it came from
    double pick(int slot, int& src, int* poolSess = nullptr) const {
        std::map<int, double>::const_iterator it = median.find(slot);
        if (it != median.end() && it->second > 0) { src = NS_FILE; return it->second; }
        double v = 0; int ns = 0;
        if (pool.at(slot, v, ns)) { src = NS_POOL; if (poolSess) *poolSess = ns; return v; }
        if (poolSess) *poolSess = ns;
        const double m = sessionMedian();
        src = m > 0 ? NS_SESSION : NS_NONE; return m;
    }
    double at(int slot) const { int src = NS_NONE; return pick(slot, src); }
    void sample(double band) { if (band > 0) { if (session.size() >= 5000) session.erase(session.begin(), session.begin() + 1000); session.push_back(band); } }
};

// ---- (0.44.0) the strict 2.0x draw gate (Rassul 2026-10-17 night "yes 2x"): enter live once durable, behind-book visible, rank 1 or
// near and >= drawX; keep while >= drawX, judged only at a bar close (no flicker). behindN == 0 = no book behind it to measure: unchanged.
inline bool drawEnter(bool behindOk, bool rank1OrNear, double bx, bool durable, double drawX) { return behindOk && rank1OrNear && durable && bx >= drawX; }
inline bool drawKeep(int behindN, double bx, double drawX) { return behindN == 0 || bx >= drawX; }

// ---- (0.44.0, audit LP-5) MenthorQ rows: the row's contract must be the chart's (CLEX26 on a CLZ6 chart = skipped) and the file
// <= 18 h old (as NearLevels.h / ZoneRead.h / the Aggression pane). Month code + last year digit compared (ESZ6 == EPZ26); a
// continuous chart or a symbol without a month code is accepted.
inline bool monthYear(const std::string& s, char& mon, int& year) {
    const size_t n = s.size();
    auto isM = [](char c) { return c != 0 && std::string("FGHJKMNQUVXZ").find(c) != std::string::npos; };
    auto dig = [](char c) { return c >= '0' && c <= '9'; };
    if (n >= 4 && isM(s[n - 3]) && dig(s[n - 2]) && dig(s[n - 1])) { mon = s[n - 3]; year = s[n - 1] - '0'; return true; }
    if (n >= 3 && isM(s[n - 2]) && dig(s[n - 1]) && !dig(s[n - 3])) { mon = s[n - 2]; year = s[n - 1] - '0'; return true; }
    return false;
}
inline bool sameContract(const std::string& chart, const std::string& file) {
    if (chart.empty() || file.empty()) return true;
    std::string a, b;
    for (size_t i = 0; i < chart.size(); ++i) a += (char)std::toupper((unsigned char)chart[i]);
    for (size_t i = 0; i < file.size(); ++i) b += (char)std::toupper((unsigned char)file[i]);
    if (a == b) return true;
    char ma = 0, mb = 0; int ya = 0, yb = 0;
    if (!monthYear(a, ma, ya) || !monthYear(b, mb, yb)) return true;
    return ma == mb && ya == yb;
}
static const long long MQ_FRESH_S = 18 * 3600;
inline bool mqFresh(long long fileMtimeSec, long long wallSec) { return fileMtimeSec > 0 && wallSec > 0 && wallSec - fileMtimeSec <= MQ_FRESH_S && fileMtimeSec - wallSec <= 600; }

// ---- (0.44.0) Market State facts (MARKET-STATE-SPEC.md 1.0: SRC LIQ and SRC LOTS); '?' = not known. Built SDK-free, posted by the plugin.
inline std::string fmtPx(double px, double tick) { if (!finite(px) || px <= 0) return "?"; char b[48]; std::snprintf(b, sizeof(b), "%.*f", decimals(tick), px); return b; }
inline std::string fmtLots(double v) { if (!finite(v) || v < 0) return "?"; char b[32]; std::snprintf(b, sizeof(b), "%lld", (long long)std::llround(v)); return b; }
inline std::string fmtX(double v) { if (!finite(v) || v <= 0) return "?"; char b[32]; std::snprintf(b, sizeof(b), "%.1f", v); return b; }
// the Big Lot tier a size reaches: S5 / S10 / S20 / none; '?' while the tiers are still learning
inline std::string tierOf(double lots, bool ok, double s20, double s10, double s5) {
    if (!ok || !(s20 > 0)) return "?";
    if (!(lots > 0)) return "none";
    if (s5 > 0 && lots >= s5) return "S5";
    if (s10 > 0 && lots >= s10) return "S10";
    return lots >= s20 ? "S20" : "none";
}
// the zone story step on this closed bar: the zone rule's verdict decided on this bar first, then the open attempt's furthest step
inline std::string zoneStepOf(bool heldNow, bool brokeNow, bool open, int barP, int barS, int barE, double attackVol) {
    if (brokeNow) return "break";
    if (heldNow) return "held";
    if (!open) return "none";
    if (barP >= 0) return "defenders_pulling";
    if (barS >= 0) return "counter";
    if (barE >= 0) return "attack_fading";
    return attackVol > 0 ? "absorbing" : "arrived";
}
struct LiqFacts {
    bool have = false; std::string level; double tick = 0.01, lo = 0, hi = 0, lots = -1, x = -1; int side = BID;
    std::string step = "none"; double absorbLots = -1, pulledBeforePct = -1;
};
inline std::map<std::string, std::string> liqKv(const LiqFacts& f) {
    std::map<std::string, std::string> kv;
    if (!f.have) {
        const char* const keys[] = { "zone_id", "zone_lo", "zone_hi", "zone_level", "zone_lots", "zone_x", "zone_side", "absorb_lots", "pulled_before_pct" };
        for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) kv[keys[i]] = "?";
        kv["zone_step"] = "none"; return kv;
    }
    const double mid = 0.5 * (f.lo + f.hi);
    kv["zone_id"] = (f.level.empty() ? std::string("zone") : f.level) + " " + fmtPx(mid, f.tick);
    kv["zone_lo"] = fmtPx(f.lo, f.tick); kv["zone_hi"] = fmtPx(f.hi, f.tick);
    kv["zone_level"] = f.level.empty() ? "?" : f.level;
    kv["zone_lots"] = fmtLots(f.lots);
    kv["zone_x"] = f.x >= 2.0 ? fmtX(f.x) : "?";            // the spec: shown only >= 2.0x
    kv["zone_side"] = f.side == BID ? "support" : "resistance";
    kv["zone_step"] = f.step.empty() ? "none" : f.step;
    kv["absorb_lots"] = fmtLots(f.absorbLots);
    kv["pulled_before_pct"] = f.pulledBeforePct >= 0 ? fmtLots(std::min(100.0, f.pulledBeforePct)) : "?";
    return kv;
}
struct LotsFacts {
    bool tiersOk = false; double s20 = 0, s10 = 0, s5 = 0, mu = 0;          // the LMT Big Lot tiers in force (slow state)
    bool mktTiersOk = false; double ms20 = 0, ms10 = 0, ms5 = 0;            // the MKT tiers (for mkt_tier only)
    double tick = 0.01;
    long long mktBuy = -1, mktSell = -1;                                     // largest market order this bar per side (-1 = no feed)
    long long lmtFilled = -1; int lmtFilledSide = -1;                        // largest big resting order filled this bar (lots, BID / ASK)
    int iceberg = -1;                                                        // -1 none, BID, ASK
    long long restBid = -1, restAsk = -1; double restBidPx = 0, restAskPx = 0; // largest single resting order within 2 ATR (slow)
};
inline std::map<std::string, std::string> lotsKv(const LotsFacts& f) {
    std::map<std::string, std::string> kv;
    kv["s20"] = f.tiersOk ? fmtLots(f.s20) : "?"; kv["s10"] = f.tiersOk ? fmtLots(f.s10) : "?"; kv["s5"] = f.tiersOk ? fmtLots(f.s5) : "?"; kv["mu"] = f.tiersOk ? fmtLots(f.mu) : "?";
    kv["mkt_buy_max"] = f.mktBuy >= 0 ? fmtLots((double)f.mktBuy) : "?";
    kv["mkt_sell_max"] = f.mktSell >= 0 ? fmtLots((double)f.mktSell) : "?";
    kv["mkt_tier"] = (f.mktBuy < 0 && f.mktSell < 0) ? "?" : tierOf((double)std::max(f.mktBuy, f.mktSell), f.mktTiersOk, f.ms20, f.ms10, f.ms5);
    kv["lmt_filled_max"] = f.lmtFilled >= 0 ? fmtLots((double)f.lmtFilled) : "?";
    kv["lmt_filled_side"] = f.lmtFilled > 0 ? (f.lmtFilledSide == BID ? "bid" : f.lmtFilledSide == ASK ? "ask" : "?") : (f.lmtFilled == 0 ? "none" : "?");
    kv["iceberg"] = f.iceberg == BID ? "bid" : f.iceberg == ASK ? "ask" : "none";
    kv["resting_bid_max"] = f.restBid > 0 ? fmtLots((double)f.restBid) : "?";
    kv["resting_bid_px"] = f.restBid > 0 ? fmtPx(f.restBidPx, f.tick) : "?";
    kv["resting_ask_max"] = f.restAsk > 0 ? fmtLots((double)f.restAsk) : "?";
    kv["resting_ask_px"] = f.restAsk > 0 ? fmtPx(f.restAskPx, f.tick) : "?";
    kv["resting_tier_bid"] = f.restBid > 0 ? tierOf((double)f.restBid, f.tiersOk, f.s20, f.s10, f.s5) : "?";
    kv["resting_tier_ask"] = f.restAsk > 0 ? tierOf((double)f.restAsk, f.tiersOk, f.s20, f.s10, f.s5) : "?";
    return kv;
}

struct Zone {
    long long id = 0; int side = BID; Tick center = 0, lo = 0, hi = 0; int hw = 0;
    double size = 0, peak = 0; Ms born = 0, seen = 0;
    bool inAttempt = false;
    // (0.10.0) prominence vs the book behind it, the real edges of its resting size, and the drawn state (hysteresis)
    double bx = -1; int rank = 0, behindN = 0; bool behindOk = false;
    Tick edgeLo = 0, edgeHi = 0;
    bool shown = false; Ms belowSince = -1;
    bool drawn = false;                         // (0.44.0) the strict 2.0x draw state (drawEnter live, drawKeep at a bar close)
    bool near = false;                          // (0.14.0) within the near band of price (hysteresis: in at nearTicks, out past 1.5x)
    double nearBase = -1;                       // (0.15.0) the zone's lots when price first came within reach on THIS approach (-1 = not near)
    bool durable(Ms now, Ms persist) const { return now - born >= persist; }
};

struct FlowEvt { Ms t; int side; Tick px; double executed, pulled, added, refill; };

// (0.15.0) a zone's makeup from the order-by-order book: resting orders in [lo, hi] on one side
struct Makeup {
    bool ok = false;                            // the feed's known lots cover coverMin..coverMax of the depth's lots
    long long orders = 0, lots = 0, top = 0;    // orders, their lots, the biggest single order
    double coverage = 0;                        // known lots / depth lots (0 = no depth size)
    double avg() const { return orders > 0 ? (double)lots / (double)orders : 0; }
    int topPct() const { return lots > 0 ? (int)((top * 100 + lots / 2) / lots) : 0; }
};
// Book: any map of order id -> order with .side .px .size .known (off::Engine::book)
template <class Book> Makeup makeupOf(const Book& book, int side, Tick lo, Tick hi, double depthLots, double coverMin, double coverMax) {
    Makeup m;
    for (typename Book::const_iterator it = book.begin(); it != book.end(); ++it) {
        const auto& o = it->second;
        if (!o.known || o.side != side || o.size <= 0 || o.px < lo || o.px > hi) continue;
        m.orders++; m.lots += (long long)o.size; if ((long long)o.size > m.top) m.top = (long long)o.size;
    }
    m.coverage = depthLots > 0 ? (double)m.lots / depthLots : 0;
    m.ok = m.orders > 0 && depthLots > 0 && m.coverage >= coverMin && m.coverage <= coverMax;
    return m;
}
// the hover's words (the chart's ANSI code page: \xB7 = the middle dot)
inline std::string makeupText(const Makeup& m) {
    char b[128]; const double a = m.avg();
    if (a < 10) std::snprintf(b, sizeof(b), "Orders %lld \xB7 avg %.1f lots \xB7 Top order %d%%", m.orders, a, m.topPct());
    else std::snprintf(b, sizeof(b), "Orders %lld \xB7 avg %.0f lots \xB7 Top order %d%%", m.orders, a, m.topPct());
    return b;
}
// (0.15.0) one order is 70%+ of the zone (only when the makeup is shown)
inline bool oneOrder(const Makeup& m, double share) { return m.ok && m.lots > 0 && (double)m.top >= share * (double)m.lots - 1e-9; }
inline std::string oneOrderText() { return "Mostly one order - may not reload ?"; }
// rebuilt % = lots re-added at the hit prices after the hits / lots hit; -1 = nothing hit yet. (0.16.1, LIQ-DAY-2026-10-14: the makeup
// log's rebuilt_pct was 200 on EVERY attempt - 0.15.0 counted every NEW in the 3-point zone after the first fill, and in ES the queue
// churn alone is 4-5x the hit (status 10-14 15:25: hit 3,854, "rebuilt" 16,911). Now an add is credited only at a price that was
// hit, up to the lots hit there and not yet restored - OrderFlowFeed 1.2.1's rule - so it can never pass 100%.) It feeds the hover
// line "Hit N lots - rebuilt NN%" and the makeup log only; no signal or decision reads it.
inline int rebuiltPct(long long rebuilt, long long hit) {
    if (hit <= 0) return -1;
    if (rebuilt < 0) rebuilt = 0;
    const long long p = (rebuilt * 100 + hit / 2) / hit; return (int)(p > 100 ? 100 : p);
}
// (0.15.0) "Hit 140 lots \xB7 rebuilt 90% - absorbing" (only while price is in the zone and something was hit)
inline std::string hitText(long long hit, long long rebuilt) {
    char b[96]; std::snprintf(b, sizeof(b), "Hit %lld lots \xB7 %d%% re-added ?", hit, rebuiltPct(rebuilt, hit)); return b;   // (0.17.0 audit: was "rebuilt NN% - absorbing")
}
// (0.15.0) 'as price came': the zone's lots now vs its lots when price first came within reach on this approach
enum CameState { CAME_NONE = -1, CAME_UNCHANGED = 0, CAME_BUILDING = 1, CAME_LEAVING = 2 };
struct Came { int state = CAME_NONE; int pct = 0; };    // pct = |now - base| / base, rounded (shown with up / down)
inline Came cameOf(double base, double now, double band) {
    Came c;
    if (!(base > 0) || !(now >= 0) || !finite(base) || !finite(now) || !(band >= 0)) return c;
    const double d = now - base;
    c.pct = (int)std::floor(std::fabs(d) / base * 100.0 + 0.5);
    // the band is inclusive: exactly +10% is building, exactly -10% leaving (1e-9 x base absorbs the floating-point rounding)
    if (d >= band * base - 1e-9 * base) c.state = CAME_BUILDING;
    else if (-d >= band * base - 1e-9 * base) c.state = CAME_LEAVING;
    else c.state = CAME_UNCHANGED;
    return c;
}
inline const char* cameWord(int state) { return state == CAME_BUILDING ? "building" : state == CAME_LEAVING ? "leaving" : state == CAME_UNCHANGED ? "unchanged" : ""; }
inline std::string cameText(const Came& c) {
    if (c.state == CAME_NONE) return "";
    if (c.state == CAME_UNCHANGED) return "As price came: unchanged";
    char b[80]; std::snprintf(b, sizeof(b), "As price came: %s %d%% - %s", c.state == CAME_BUILDING ? "up" : "down", c.pct, cameWord(c.state)); return b;
}
// hover row colours by what they mean for price: INK_WHITE = fact only, GREEN = good for buyers, RED = good for sellers, GREY = no data
enum Ink { INK_WHITE = 0, INK_GREEN = 1, INK_RED = 2, INK_GREY = 3 };
// support building = green, support leaving = red; resistance building = red, resistance leaving = green; unchanged = white
inline int cameInk(int side, int state) {
    if (state == CAME_BUILDING) return side == BID ? INK_GREEN : INK_RED;
    if (state == CAME_LEAVING) return side == BID ? INK_RED : INK_GREEN;
    return INK_WHITE;
}
inline int sideInk(int side) { return side == BID ? INK_GREEN : INK_RED; }   // header and 'absorbing': support green, resistance red
// a price with the market's decimals and thousands commas ("7,864.00", "79.98")
inline std::string pxText(Tick t, double tick) {
    int d = 2; if (tick > 0) { d = 0; double x = tick; while (d < 8 && std::fabs(x - std::floor(x + 0.5)) > 1e-9) { x *= 10; ++d; } }
    char b[48]; std::snprintf(b, sizeof(b), "%.*f", d, (double)t * tick);
    std::string r = b; const size_t dot = r.find('.'); int i = (int)(dot == std::string::npos ? r.size() : dot);
    const int stop = r[0] == '-' ? 1 : 0;
    while ((i -= 3) > stop) r.insert((size_t)i, ",");
    return r;
}
inline std::string lotsText(double v) { char b[32]; std::snprintf(b, sizeof(b), v >= 10000 ? "%.0fk" : "%.0f", v >= 10000 ? v / 1000 : v); return b; }
inline std::string multText(double x) { char b[32]; if (x >= 10) std::snprintf(b, sizeof(b), "10x+"); else std::snprintf(b, sizeof(b), "%.1fx", x); return b; }
// (0.15.0) everything the hover of one near zone shows, in the approved order; the plugin only maps Ink to its colours and draws
struct HoverIn {
    int side = BID; std::string level;          // level = the key / gamma level(s) the zone holds ("PDL", "LonLO/CR"), empty = none
    Tick lo = 0, hi = 0; double tick = 0.25, lots = 0, bx = -1;
    Makeup mk; bool feedReady = false;          // the makeup from the order book; feedReady = the order feed is live
    double nearBase = -1;                       // the zone's lots when price first came within reach on this approach
    bool inZone = false; long long hit = 0, rebuilt = 0;   // price is in the zone now; the open attempt's hit / rebuilt lots
    double oneShare = 0.70, band = 0.10;
};
struct HoverRow { std::string text; int ink = INK_WHITE; };
inline std::vector<HoverRow> hoverRows(const HoverIn& h) {
    std::vector<HoverRow> r;
    {   std::string t = h.side == BID ? "Bids" : "Offers";            // (0.17.0 audit: no "support" / "resistance" - resting size is not a defence)
        if (!h.level.empty()) t += " at " + h.level;
        t += " " + pxText(h.lo, h.tick) + " - " + pxText(h.hi, h.tick) + " \xB7 " + lotsText(h.lots) + " lots (" + (h.bx > 0 ? multText(h.bx) : std::string("-")) + ")";
        HoverRow x; x.text = t; x.ink = sideInk(h.side); r.push_back(x); }
    {   HoverRow x;
        if (h.mk.ok) { x.text = makeupText(h.mk); x.ink = INK_WHITE; }
        else if (!h.feedReady) { x.text = "Orders: waiting for the order feed"; x.ink = INK_GREY; }
        else { char b[96]; std::snprintf(b, sizeof(b), "Orders: building (%d%% of the lots seen)", (int)std::floor(std::min(9.99, std::max(0.0, h.mk.coverage)) * 100 + 0.5)); x.text = b; x.ink = INK_GREY; }
        r.push_back(x); }
    if (oneOrder(h.mk, h.oneShare)) { HoverRow x; x.text = oneOrderText(); x.ink = INK_WHITE; r.push_back(x); }
    {   const Came c = cameOf(h.nearBase, h.lots, h.band);
        if (c.state != CAME_NONE) { HoverRow x; x.text = cameText(c); x.ink = cameInk(h.side, c.state); r.push_back(x); } }
    if (h.inZone && h.hit > 0) { HoverRow x; x.text = hitText(h.hit, h.rebuilt); x.ink = sideInk(h.side); r.push_back(x); }
    return r;
}
// (0.15.0) where the hover box goes: right of the column (colRight + gap), never over it; its right edge never past dpLimit (the left
// end of the Delta Profile's labels: paneRight - 2 - Dealer reach - Delta gap - Delta width - 64 px of its labels). Returns the
// box's left x, or -1 when it cannot fit between the two (then no box: never over the column or the Delta Profile).
inline int hoverX(int colRightAbs, int boxW, int dpLimit, int gap = 12) {
    const int x0 = colRightAbs + gap;
    if (boxW <= 0 || x0 + boxW > dpLimit) return -1;
    return x0;
}
inline int hoverY(int zoneMidY, int boxH, int top, int bottom) {
    int y0 = zoneMidY - boxH / 2;
    if (y0 + boxH > bottom - 2) y0 = bottom - 2 - boxH;
    if (y0 < top) y0 = top;
    return y0;
}

struct TradeEvt { Ms t; Tick px; double buy, sell; };

struct Stats { double attack = -1, eaten = -1, exh = -1, pulled = -1, rebuilt = -1, counter = -1; int side = 0; bool active = false; };

struct Attempt {
    long long zoneId = 0; int side = BID; Tick lo = 0, hi = 0; Ms tA = 0; int barA = 0;
    bool fromDelta = false; Tick dpLo = 0, dpHi = 0;   // (0.11.0) started by a Delta Profile A / A? (not a shown zone) - its price
    double shown = 0, attackVol = 0, counterVol = 0, pulled = 0, added = 0, refill = 0, minSize = 0, startSize = 0, curSize = 0;
    std::vector<std::pair<Ms, double>> attacks;  // (time, attacker contracts) inside the zone since arrival
    bool breachTrade = false;                    // an attacker trade beyond the far edge
    bool done = false; std::string end; Ms tEnd = 0;
    double oldShare = -1;                        // (0.5.0) the zone's old-order share at arrival (-1 = not known), logged for the study
    long long hitLots = 0, rebuiltLots = 0;      // (0.15.0) order feed: lots FILLED in the zone (its side) / lots re-added there after a hit
    std::map<Tick, std::pair<long long, long long>> hitAt;   // (0.16.1) price -> (lots hit there, lots credited as restored there <= hit)
    Makeup mk; bool mkSet = false;               // (0.15.0) the zone's makeup, refreshed while open; the last one stands at the decision
    double cameBase = -1, cameArr = -1;          // (0.15.0) the zone's lots when price first came within reach / at arrival (as price came)
    // (0.2.0) the absorption STORY per bar, for the aggression / defense panes (lsLiquidityFlow) and the nightly study:
    // attack / defend = aggressive contracts in the zone that bar (attackers vs the defending side turning aggressive);
    // rep / pul = replenished (added + refill) and pulled since arrival, cumulative, as of that bar
    // (0.3.0, Rassul 2026-10-10 23:11 "buyers are always green and sellers are always red ... buyers are stronger in aggression and
    // defense") defNet / oppNet = NET resting size since arrival (added + refilled - pulled; fills excluded) of the DEFENDING side in the
    // zone and of the OTHER side in the band of the same width just beyond it (offers above a bid zone, bids below an offer zone).
    // Net is churn-proof: an order placed and cancelled nets to zero. Pane: buyers' bids green, sellers' offers red, up = adding,
    // down = pulling.
    struct BarFlow { double attack = 0, defend = 0, rep = 0, pul = 0, defNet = 0, oppNet = 0; bool book = false; };
    double defNet = 0, oppNet = 0; Tick oppLo = 0, oppHi = 0;
    std::map<int, BarFlow> flowBars;
    std::map<int, Stats> barStats;               // (0.7.0) the pane statistics as of each closed bar (the mouse-over read of a past touch)
    int barEW = -1, barPW = -1, barT = -1;        // (0.7.0) e? / p? watches and t (trapped) - first bar each printed
    int barS = -1; Ms tS = 0; double sX = -1, exE = -1;   // (0.8.0) s (Counter) bar / time / its pace, and Exh when e printed
    double counterNow = -1;                      // (0.8.0) the Counter readout: the defenders' pace per tick on the latest closed bar
    int barE = -1, barO = -1, barP = -1;         // first bar the attackers' pace halved (E), the defenders went on offense (O), the zone was pulled (P)
    int barR = -1, barX = -1;                    // (0.3.0) the defenders' net resting size crossed above the other side's (R); O and R both done = X
    bool defendedFirst = false;                  // (0.3.0) the attackers' volume since arrival led the defenders' before O
    bool pulledFirst = false;                    // (0.3.0) the other side's net resting size was at or above the defenders' before R
};

struct Mark {
    std::string code;      // (0.5.0) Exh / Pull only (Rassul 2026-10-11 "remove whatever doesn't work and keep what works")
    std::string reasons;   // unused since 0.4.0 (kept in the record format)
    int side = BID; long long barTime = 0;       // the closed bar's time (host clock, matched on redraw)
    Tick lo = 0, hi = 0; double eaten = 0, stayed = 0, exhaust = -1, resil = -1, shown = 0; Ms tA = 0, tDecided = 0;
    double value = -1;     // (0.4.1) the number drawn with the code: Exh = 2nd / 1st half pace, Pull = share pulled
};

// (0.6.0, Rassul 2026-10-11) the pane statistics, in the order the process happens (Metric: now (threshold) on the panes):
//   AGGRESSION  attack  the attackers' biggest chart bar inside the zone since arrival, per tick of zone width, vs their average volume per
//                       bar per tick that bar traded over the 20 bars before the arrival bar (1.0x = normal, 2.4x = 2.4 times normal)
//               eaten   the attackers' volume / EVERYTHING that was there to trade against (size at arrival + new size + refills): 0..1
//               exh     the attackers' 2nd-half / 1st-half volume since arrival (the Exh signal's number)
//   DEFENSE     pulled  size pulled (not traded) / size shown at arrival
//               rebuilt what came back / what the zone lost (resilience; -1 = it never lost size)
// Every number is measured exactly as the nightly study measures it (lra.mbo_study att_tick_x, eaten_total, exhaust_ratio, pull_snap,
// resilience), so the per-market lines it chooses apply to what is drawn.

// (0.7.0) the per-market lines the host loaded from the nightly study; -1 = not confirmed for this market (that number is never used)
struct Lines { double attack = -1, eaten = -1, exh = -1, pulled = -1, rebuilt = -1, counter = -1; bool trapped = false; };
// (0.7.0, Rassul 2026-10-11) the warning on the price chart: one at a time, the strongest that applies, in plain words
struct Warn { std::string name, detail; int favour = 0; };   // favour: +1 buyers (green), -1 sellers (red), 0 not decided (grey)
// (0.18.0, Rassul 22:48 CT 2026-10-14, mockup-lp-warning-v5.png) the warning's grey detail drawn on TWO lines, one per side: line 1 next
// to the badge = the first side, line 2 directly under it = the other side; the connector ("; " / ", but " / " while ") is dropped.
// A detail with no second side stays one line. The text itself is unchanged (the Tape Reader hand-off keeps the one-line detail).
inline std::vector<std::string> warnLines(const std::string& d) {
    static const char* const SEP[] = { ", but ", "; but ", "; ", " while ", " but " };
    size_t best = std::string::npos, len = 0;
    for (size_t k = 0; k < sizeof(SEP) / sizeof(SEP[0]); ++k) {
        const size_t at = d.find(SEP[k]);
        if (at != std::string::npos && at > 0 && (best == std::string::npos || at < best)) { best = at; len = std::string(SEP[k]).size(); }
    }
    std::vector<std::string> v;
    if (best == std::string::npos || best + len >= d.size()) { if (!d.empty()) v.push_back(d); return v; }
    v.push_back(d.substr(0, best)); v.push_back(d.substr(best + len));
    return v;
}
// word wrap with NO truncation: a line wider than the room continues on the next row at a space (a single word wider than the room
// stays whole). fits(text) = the text fits the room.
template <class Fits> std::vector<std::string> wrapWords(const std::string& text, Fits fits) {
    std::vector<std::string> out; std::string cur; size_t i = 0;
    while (i < text.size()) {
        size_t j = text.find(' ', i); if (j == std::string::npos) j = text.size();
        const std::string w = text.substr(i, j - i), tryS = cur.empty() ? w : cur + " " + w;
        if (cur.empty() || fits(tryS)) cur = tryS; else { out.push_back(cur); cur = w; }
        i = j + 1;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

struct Readout {         // the live read of the zone price is in (or the last one), for the column label
    bool active = false; int side = BID; Tick lo = 0, hi = 0; double eaten = 0, stayed = 1, exhaust = -1; bool exhausted = false;
    double pulled = 0, refill = -1;      // pulled share of what was shown; refill = new size that came in / what was taken (eaten + pulled)
    bool pulling = false, depleting = false;   // PULL?: half+ of the shown size left without trading and was not replaced; DEPLETING?: eaten 1x+, not replaced
};

inline double exhaustRatio(const std::vector<std::pair<Ms, double>>& a, Ms t0, Ms t1) {
    if (t1 <= t0) return -1;
    const Ms mid = t0 + (t1 - t0) / 2; double v1 = 0, v2 = 0;
    for (size_t i = 0; i < a.size(); ++i) { if (a[i].first < t0 || a[i].first > t1) continue; (a[i].first < mid ? v1 : v2) += a[i].second; }
    return v1 > 0 ? v2 / v1 : -1;
}

// (0.11.4) labels never overlap: a text row remembers the x spans already used; a label goes in the first row where it fits
struct Lanes {
    std::vector<std::vector<std::pair<int, int>>> rows;
    explicit Lanes(int n = 1) : rows((size_t)std::max(1, n)) {}
    bool fits(int row, int x0, int x1, int gap = 4) const {
        if (row < 0 || row >= (int)rows.size()) return false;
        for (size_t i = 0; i < rows[(size_t)row].size(); ++i) if (x0 < rows[(size_t)row][i].second + gap && rows[(size_t)row][i].first < x1 + gap) return false;
        return true;
    }
    int place(int x0, int x1, int gap = 4) {      // the row used, -1 = no room (the label is not drawn)
        for (int r = 0; r < (int)rows.size(); ++r) if (fits(r, x0, x1, gap)) { rows[(size_t)r].push_back(std::make_pair(x0, x1)); return r; }
        return -1;
    }
};

class Engine {
public:
    Params P;
    Norms norms;
    std::map<Tick, double> book[2];              // current resting size per price
    Tick win[2][2] = {{0, 0}, {0, 0}};           // [side][lo, hi] of the last book's depth window
    bool haveBook = false; Ms bookT = 0;
    std::map<Tick, double> pendingExec[2];       // traded against each side since the last book
    std::deque<FlowEvt> flow; std::deque<TradeEvt> tape;
    std::vector<Zone> zones; long long nextZone = 1;
    std::vector<Attempt> attempts; std::vector<Attempt> finished;
    long long finTotal = 0;                      // (0.11.4) attempts finished since load; 'finished' keeps the last 50 (finished[0] is number finTotal - size)
    std::vector<Mark> marks;
    Tick lastPx = 0; bool havePx = false; int hw = 4;
    // (0.5.0) OLD ORDERS. The chart's depth is the total size per price, not single orders, so an order's age cannot be read. The
    // estimate: the size at a price that never dropped below during the last oldMs (10 min) while that price stayed inside the depth
    // window = the size resting 10 min+ (orders join the back of the queue, so a level that never dipped kept its oldest size). A price
    // that left the window, or a gap in the depth feed, restarts its clock (age unknown = not old: conservative).
    struct Age { Ms since = 0; std::deque<std::pair<Ms, double>> q; };   // q: sliding minimum of the size over the last oldMs
    std::map<Tick, Age> age[2];
    void ageUpdate(int s, Tick px, Ms t, double size) {
        std::map<Tick, Age>::iterator it = age[s].find(px);
        if (it == age[s].end()) { it = age[s].insert(std::make_pair(px, Age())).first; it->second.since = t; }
        std::deque<std::pair<Ms, double>>& q = it->second.q;
        while (!q.empty() && q.back().second >= size) q.pop_back();
        q.push_back(std::make_pair(t, size));
        while (q.size() > 1 && q[1].first <= t - P.oldMs) q.pop_front();
        if (q.size() > 5000) q.pop_front();                               // bounded (checklist #33)
    }
    // the old size at a price now (-1 = unknown: not seen continuously for oldMs)
    double oldAt(int s, Tick px, Ms now) const {
        std::map<Tick, Age>::const_iterator it = age[s].find(px);
        if (it == age[s].end() || it->second.q.empty() || now - it->second.since < P.oldMs) return -1;
        return std::min(it->second.q.front().second, get(book[s], px));
    }
    // the share of a zone's size that is old; -1 when the age of less than half of its size is known
    double zoneOld(int side, Tick lo, Tick hi, Ms now) const {
        double total = 0, known = 0, old = 0;
        for (std::map<Tick, double>::const_iterator i = book[side].lower_bound(lo); i != book[side].end() && i->first <= hi; ++i) {
            total += i->second; const double o = oldAt(side, i->first, now);
            if (o >= 0) { known += i->second; old += o; }
        }
        if (total <= 0 || known < 0.5 * total) return -1;
        return old / total;
    }
    // (0.6.0) the chart's closed bars: buyers' / sellers' volume and the ticks each bar traded - the Attack readout's normal
    struct BarNorm { double buy = 0, sell = 0; int ticks = 1; Tick hi = 0, lo = 0; bool range = false; int slot = -1; };   // (0.27.0) slot = the bar END's 30-min slot
    std::map<int, BarNorm> barNorm;
    void setBar(int bar, double buy, double sell, int ticks, Tick hi = 0, Tick lo = 0, int slot = -1) {
        if (bar < 0 || !(buy >= 0) || !(sell >= 0) || !finite(buy) || !finite(sell)) return;
        BarNorm& b = barNorm[bar]; b.buy = buy; b.sell = sell; b.ticks = std::max(1, ticks); b.slot = slot;
        b.range = hi > 0 && lo > 0 && hi >= lo; b.hi = hi; b.lo = lo;
        while (barNorm.size() > 1200) barNorm.erase(barNorm.begin());   // (0.11.0) the always-on Aggressive half: a screen of bars
    }
    // the attackers' (sellers at a support, buyers at a resistance) average volume per bar per tick traded over the 20 bars before 'bar';
    // -1 until 15 of those bars are known
    // (0.27.0, Rassul 2026-10-15 audit #2) the SAME 30-min slot first: the typical (log mean) of that slot's earlier bars on the chart
    // (all sessions loaded, newest 80); 10+ of them, else the 20 bars before as before. kind 0 = per tick traded (Attack), 1 = per bar (pace)
    double slotNormal(int bar, bool buyers, int kind) const {
        std::map<int, BarNorm>::const_iterator me = barNorm.find(bar); if (me == barNorm.end() || me->second.slot < 0) return -1;
        double s1 = 0; int n = 0;
        for (std::map<int, BarNorm>::const_iterator i = barNorm.find(bar); i != barNorm.begin() && n < 80;) {
            --i; if (i->second.slot != me->second.slot) continue;
            double v = buyers ? i->second.buy : i->second.sell; if (kind == 0) v /= (double)i->second.ticks;
            if (v > 0) { s1 += std::log(v); ++n; }
        }
        return n >= 10 ? std::exp(s1 / n) : -1;
    }
    double attackNormal(int side, int bar) const {
        { const double z = slotNormal(bar, side != BID, 0); if (z > 0) return z; }   // (0.27.0) the same slot
        double s = 0; int n = 0;
        for (std::map<int, BarNorm>::const_iterator i = barNorm.lower_bound(bar - 20); i != barNorm.end() && i->first < bar; ++i) {
            s += (side == BID ? i->second.sell : i->second.buy) / (double)i->second.ticks; ++n;
        }
        return n >= 15 && s > 0 ? s / n : -1;
    }
    // (0.11.0) the always-on Aggressive half: a bar's buyers' (side ASK) / sellers' (side BID) volume x their average over the 20 bars
    // before it (-1 until 15 are known)
    double paceX(int bar, bool buyers) const {
        std::map<int, BarNorm>::const_iterator me = barNorm.find(bar); if (me == barNorm.end()) return -1;
        { const double z = slotNormal(bar, buyers, 1); if (z > 0) return (buyers ? me->second.buy : me->second.sell) / z; }   // (0.27.0) the same slot
        double s = 0; int n = 0;
        for (std::map<int, BarNorm>::const_iterator i = barNorm.lower_bound(bar - 20); i != barNorm.end() && i->first < bar; ++i) { s += buyers ? i->second.buy : i->second.sell; ++n; }
        if (n < 15 || s <= 0) return -1;
        return (buyers ? me->second.buy : me->second.sell) / (s / n);
    }
    // (0.11.0) the always-on Passive half: resting bids below / offers above price within the reach, per closed bar (the book at its close)
    std::map<int, std::pair<double, double>> nearBook;
    std::pair<double, double> nearNow() const {
        std::pair<double, double> r(0, 0); if (!havePx || !haveBook) return r;
        const Tick reach = P.reachTicks > 0 ? P.reachTicks : (Tick)std::max(10, 6 * hw);
        for (std::map<Tick, double>::const_iterator i = book[BID].lower_bound(lastPx - reach); i != book[BID].end() && i->first <= lastPx; ++i) r.first += i->second;
        for (std::map<Tick, double>::const_iterator i = book[ASK].lower_bound(lastPx); i != book[ASK].end() && i->first <= lastPx + reach; ++i) r.second += i->second;
        return r;
    }
    // (0.11.0) Delta Profile absorption (A / A?) price bands of the last hour: an attempt also starts at one of them (not only at a shown zone)
    std::vector<std::pair<Tick, Tick>> dpZone[2];
    const std::pair<Tick, Tick>* dpAt(int side, Tick lo, Tick hi) const {
        for (size_t i = 0; i < dpZone[side].size(); ++i) if (dpZone[side][i].first <= hi && lo <= dpZone[side][i].second) return &dpZone[side][i];
        return nullptr;
    }
    // (0.8.0) the DEFENDERS' (buyers at a support, sellers at a resistance) average volume per bar per tick over the 20 bars before 'bar'
    double defNormal(int side, int bar) const { return attackNormal(side == BID ? ASK : BID, bar); }
    Stats stats(const Attempt& a, Ms now) const {
        Stats r; r.side = a.side; r.active = !a.done;
        const double w = (double)(a.hi - a.lo + 1), nrm = attackNormal(a.side, a.barA);
        if (nrm > 0) { double mx = 0; for (std::map<int, Attempt::BarFlow>::const_iterator i = a.flowBars.begin(); i != a.flowBars.end(); ++i) mx = std::max(mx, i->second.attack); r.attack = mx / w / nrm; }
        const double tot = a.shown + a.added + a.refill;
        if (tot > 0) r.eaten = std::min(1.0, a.attackVol / tot);
        const Ms t1 = a.done ? (a.tEnd > a.tA ? a.tEnd : a.tA) : now;
        r.exh = t1 - a.tA >= 60000 ? exhaustRatio(a.attacks, a.tA, t1) : -1;   // (0.7.0) halves of under 30 s say nothing: no Exh read before 1 min
        if (a.shown > 0) r.pulled = std::min(1.0, a.pulled / a.shown);   // (0.27.0) never over 100% (orders added after arrival and then pulled)
        r.rebuilt = resilience(a);
        r.counter = a.counterNow;
        return r;
    }
    // the statistics of the zone price is in now (or the last one)
    bool latest(Ms now, Stats& out) const {
        const Attempt* best = nullptr;
        for (size_t i = 0; i < attempts.size(); ++i) if (!attempts[i].done && (!best || attempts[i].tA > best->tA)) best = &attempts[i];
        if (!best && !finished.empty()) best = &finished.back();
        if (!best) return false;
        out = stats(*best, now); return true;
    }
    long long books = 0, trades = 0, badBooks = 0;
    std::string why;                             // last reason a book or trade was refused

    void setAtr(double atrPrice) { hw = halfWidth(P, atrPrice); }

    // ---- trades: aggressive volume at a price since the last read (buy lifts the offer, sell hits the bid)
    void onTrade(Ms t, double price, double buy, double sell) {
        if (!finite(price) || price <= 0 || !(buy >= 0) || !(sell >= 0) || buy + sell <= 0) return;
        Tick px = 0; if (!tryToTick(price, P.tick, px)) { why = "invalid trade price"; return; }
        if (buy > 0) pendingExec[ASK][px] += buy;
        if (sell > 0) pendingExec[BID][px] += sell;
        TradeEvt e; e.t = t; e.px = px; e.buy = buy; e.sell = sell; tape.push_back(e); trades++;
        for (size_t i = 0; i < attempts.size(); ++i) {
            Attempt& a = attempts[i]; if (a.done) continue;
            const double attack = a.side == BID ? sell : buy;            // support is attacked by sellers, resistance by buyers
            if (attack > 0 && px >= a.lo && px <= a.hi) { a.attackVol += attack; a.attacks.push_back(std::make_pair(t, attack)); a.flowBars[formBar].attack += attack; }
            if (attack > 0 && (a.side == BID ? px < a.lo : px > a.hi)) a.breachTrade = true;
            const double counter = a.side == BID ? buy : sell;           // the defending side turning aggressive (pushback)
            if (counter > 0 && px >= a.lo && px <= a.hi) { a.counterVol += counter; a.flowBars[formBar].defend += counter; }
        }
        startAttempts(t, px, buy, sell);
        lastPx = px; havePx = true;
        trim(t);
    }
    void onLastPrice(double price) { Tick px = 0; if (finite(price) && price > 0 && tryToTick(price, P.tick, px)) { lastPx = px; havePx = true; } }

    // ---- a depth snapshot: (price, size) per level; zero / bad levels are skipped
    bool onBook(Ms t, const std::vector<std::pair<double, double>>& bids, const std::vector<std::pair<double, double>>& asks) {
        std::map<Tick, double> nb[2];
        for (size_t i = 0; i < bids.size(); ++i) {
            Tick px = 0;
            if (finite(bids[i].first) && bids[i].first > 0 && finite(bids[i].second) && bids[i].second > 0 && tryToTick(bids[i].first, P.tick, px)) nb[BID][px] += bids[i].second;
        }
        for (size_t i = 0; i < asks.size(); ++i) {
            Tick px = 0;
            if (finite(asks[i].first) && asks[i].first > 0 && finite(asks[i].second) && asks[i].second > 0 && tryToTick(asks[i].first, P.tick, px)) nb[ASK][px] += asks[i].second;
        }
        if (nb[BID].empty() || nb[ASK].empty()) { why = "one-sided book"; badBooks++; return false; }
        if (nb[BID].rbegin()->first >= nb[ASK].begin()->first) { why = "crossed book"; badBooks++; return false; }
        Tick nw[2][2] = {{nb[BID].begin()->first, nb[BID].rbegin()->first}, {nb[ASK].begin()->first, nb[ASK].rbegin()->first}};
        if (haveBook) {
            for (int s = 0; s < 2; ++s) {
                // only prices inside BOTH windows: bids from max(old lowest, new lowest) up; asks up to min(old highest, new highest)
                const Tick lo = std::max(win[s][0], nw[s][0]), hi = std::min(win[s][1], nw[s][1]);
                std::map<Tick, double> keys;
                for (std::map<Tick, double>::const_iterator i = book[s].begin(); i != book[s].end(); ++i) keys[i->first] = 0;
                for (std::map<Tick, double>::const_iterator i = nb[s].begin(); i != nb[s].end(); ++i) keys[i->first] = 0;
                for (std::map<Tick, double>::const_iterator k = pendingExec[s].begin(); k != pendingExec[s].end(); ++k) keys[k->first] = 0;
                for (std::map<Tick, double>::const_iterator k = keys.begin(); k != keys.end(); ++k) {
                    const Tick px = k->first;
                    const double old = get(book[s], px), now = get(nb[s], px), ex = get(pendingExec[s], px);
                    const bool inWin = px >= lo && px <= hi;
                    FlowEvt e; e.t = t; e.side = s; e.px = px; e.executed = ex; e.pulled = e.added = e.refill = 0;
                    const double rest = old - std::min(ex, old);
                    e.refill = std::max(0.0, ex - old);
                    if (inWin) { e.pulled = std::max(0.0, rest - now); e.added = std::max(0.0, now - rest); }
                    if (e.executed > 0 || e.pulled > 0 || e.added > 0 || e.refill > 0) { flow.push_back(e); applyFlow(e); }
                }
            }
        }
        for (int s = 0; s < 2; ++s) { book[s].swap(nb[s]); pendingExec[s].clear(); win[s][0] = nw[s][0]; win[s][1] = nw[s][1]; }
        // (0.5.0) the ages: a feed gap of a minute+ restarts every clock; prices that left the window restart theirs
        if (haveBook && t - bookT > 60000) { age[0].clear(); age[1].clear(); }
        for (int s = 0; s < 2; ++s) {
            for (std::map<Tick, Age>::iterator i = age[s].begin(); i != age[s].end();) { if (i->first < win[s][0] || i->first > win[s][1]) i = age[s].erase(i); else ++i; }
            if (win[s][1] - win[s][0] <= 400) for (Tick px = win[s][0]; px <= win[s][1]; ++px) ageUpdate(s, px, t, get(book[s], px));
        }
        haveBook = true; bookT = t; books++;
        for (size_t i = 0; i < attempts.size(); ++i) if (!attempts[i].done) {
            Attempt& a = attempts[i]; a.curSize = band(a.side, a.lo, a.hi); a.minSize = std::min(a.minSize, a.curSize);
        }
        trim(t);
        return true;
    }

    // (0.16.0, CR5200 / Rassul 11:39 CT) the liquidity at ANY price, for the Dealer Profile's level badges: the contracts in the zone-wide
    // band around it (2*hw+1 ticks, the band the zones use) on the side price would hit - bids below the last price, offers above - and
    // its prominence vs the 8 same-width bands behind it (prominence(), the rule the zones are drawn by; -1 = nothing behind to compare)
    bool levelLiq(Tick px, double& lots, double& bx, int& side) const {
        lots = 0; bx = -1; side = BID;
        if (!haveBook || !havePx || px == lastPx) return false;
        Zone z; z.side = px < lastPx ? BID : ASK; z.lo = px - hw; z.hi = px + hw;
        lots = band(z.side, z.lo, z.hi);
        if (!(lots > 0)) return false;
        prominence(z); bx = z.behindN > 0 ? z.bx : -1; side = z.side; return true;
    }
    double band(int side, Tick lo, Tick hi) const {
        double s = 0; for (std::map<Tick, double>::const_iterator i = book[side].lower_bound(lo); i != book[side].end() && i->first <= hi; ++i) s += i->second;
        return s;
    }

    // ---- the top N zones per side within reach; tracked by centre. Call after onBook (at most a few times a second).
    void updateZones(Ms t, double atrPrice, int slot) {
        if (!haveBook) return;
        setAtr(atrPrice);
        const Tick reach = P.reachTicks > 0 ? std::max((Tick)(10 * hw), P.reachTicks)
                                            : (Tick)std::llround(std::max(10.0 * hw, P.reachAtr * (atrPrice > 0 ? atrPrice : 15 * P.tick) / P.tick));
        const Tick ref = havePx ? lastPx : (book[BID].rbegin()->first + book[ASK].begin()->first) / 2;
        nearTicks = nearBand(atrPrice);
        const long long sess = zrule::sessionKey(t / 1000);              // (0.44.0) the 5-session per-slot pool
        for (int s = 0; s < 2; ++s) {
            std::vector<std::pair<double, Tick>> cand;
            const std::map<Tick, double>& b = book[s];
            if (b.empty()) continue;
            const Tick from = s == BID ? std::max(b.begin()->first, ref - reach) : b.begin()->first;
            const Tick to = s == BID ? b.rbegin()->first : std::min(b.rbegin()->first, ref + reach);
            std::vector<std::pair<Tick, double>> lv; for (std::map<Tick, double>::const_iterator i = b.lower_bound(from - hw); i != b.end() && i->first <= to + hw; ++i) lv.push_back(*i);
            // sliding band sum at every price that has size
            size_t a = 0, c = 0; double sum = 0;
            for (size_t i = 0; i < lv.size(); ++i) {
                const Tick p = lv[i].first; if (p < from || p > to) continue;
                while (c < lv.size() && lv[c].first <= p + hw) { sum += lv[c].second; ++c; }
                while (a < lv.size() && lv[a].first < p - hw) { sum -= lv[a].second; ++a; }
                cand.push_back(std::make_pair(sum, p));
                norms.sample(sum); norms.pool.sample(sess, slot, sum);
            }
            std::sort(cand.begin(), cand.end(), [](const std::pair<double, Tick>& x, const std::pair<double, Tick>& y) { return x.first != y.first ? x.first > y.first : x.second < y.second; });
            std::vector<std::pair<double, Tick>> pick;
            for (size_t i = 0; i < cand.size() && (int)pick.size() < P.topN; ++i) {
                bool clash = false; for (size_t j = 0; j < pick.size(); ++j) if (std::llabs(pick[j].second - cand[i].second) <= 2 * hw) clash = true;
                if (!clash && cand[i].first > 0) pick.push_back(cand[i]);
            }
            // (0.14.0) the strongest near candidates (up to nearPerSide, not clashing with each other or a pick) are always tracked,
            // even when bigger bands further away fill the topN - else a near zone could never be shown
            int nearN = 0;
            for (size_t i = 0; i < cand.size() && nearN < P.nearPerSide; ++i) {
                if (!(cand[i].first > 0)) break;
                const Tick d = s == BID ? ref - (cand[i].second + hw) : (cand[i].second - hw) - ref;
                if (std::max((Tick)0, d) > nearTicks) continue;
                bool clash = false; for (size_t j = 0; j < pick.size(); ++j) if (std::llabs(pick[j].second - cand[i].second) <= 2 * hw) clash = true;
                if (clash) { bool inTop = false; for (size_t j = 0; j < pick.size(); ++j) if (pick[j].second == cand[i].second) inTop = true; if (inTop) ++nearN; continue; }
                pick.push_back(cand[i]); ++nearN;
            }
            std::vector<Tick> placed;
            for (size_t i = 0; i < pick.size(); ++i) {
                // the zone's centre = the size-weighted middle of its band (a flat run of equal band sums otherwise picks its edge);
                // a zone whose band would overlap a bigger one already placed is dropped (zones never overlap)
                double w = 0, m = 0;
                for (std::map<Tick, double>::const_iterator j = b.lower_bound(pick[i].second - hw); j != b.end() && j->first <= pick[i].second + hw; ++j) { w += j->second; m += j->second * (double)j->first; }
                const Tick c = w > 0 ? (Tick)std::llround(m / w) : pick[i].second;
                bool clash = false; for (size_t j = 0; j < placed.size(); ++j) if (std::llabs(placed[j] - c) <= 2 * hw) clash = true;
                if (clash) continue;
                placed.push_back(c);
                track(t, s, c, band(s, c - hw, c + hw));
            }
        }
        for (size_t i = 0; i < zones.size();) {
            if (!zones[i].inAttempt && t - zones[i].seen > P.graceMs) zones.erase(zones.begin() + i); else ++i;
        }
        for (size_t i = 0; i < zones.size(); ++i) { nearState(zones[i], ref); prominence(zones[i]); showState(zones[i], t); drawState(zones[i], t); }
        normal = norms.pick(slot, normalSrc, &normalPoolN);
    }
    double normal = 0;
    int normalSrc = NS_NONE, normalPoolN = 0;    // (0.44.0) where the normal came from (NormSrc) and the pool sessions behind that slot
    Tick nearTicks = 0;                          // (0.14.0) the near band in ticks, set by updateZones
    // (0.14.0) near band = max(round(nearAtr x the 3-min range / tick), 4 half-widths)
    Tick nearBand(double atrPrice) const {
        const double a = atrPrice > 0 && finite(atrPrice) ? atrPrice : 15 * P.tick;
        const double raw = P.nearAtr * a / P.tick;
        const Tick n = finite(raw) && raw < 1e9 ? (Tick)std::llround(raw) : (Tick)1000000000;
        return std::max(n, (Tick)(4 * hw));
    }
    // (0.14.0) distance of a zone from price, floored at 0: a support ref - hi, a resistance lo - ref
    static Tick zoneDist(const Zone& z, Tick ref) { const Tick d = z.side == BID ? ref - z.hi : z.lo - ref; return d > 0 ? d : 0; }
    // near with hysteresis: in at <= nearTicks, out only past nearExit x nearTicks
    void nearState(Zone& z, Tick ref) const {
        const Tick d = zoneDist(z, ref);
        if (!z.near) { if (d <= nearTicks) { z.near = true; z.nearBase = z.size; } }   // (0.15.0) the baseline, once per approach
        else if ((double)d > P.nearExit * (double)nearTicks) { z.near = false; z.nearBase = -1; }
        if (z.near && !(z.nearBase > 0) && z.size > 0) z.nearBase = z.size;          // (0.15.0) a zone that came near empty: first size
    }
    double xNormal(double size) const { return normal > 0 ? size / normal : -1; }

    // (0.10.0) the zone vs the same side's book BEHIND it: behindBands bands of the zone's width beyond it (lower bids under a
    // support, higher offers over a resistance); a band counts only if it lies inside the visible book. bx = size / their median,
    // rank = 1 + bands bigger than the zone. Also the real edges: the first / last price inside the band that holds size.
    void prominence(Zone& z) const {
        z.bx = -1; z.rank = 0; z.behindN = 0; z.behindOk = false; z.edgeLo = z.lo; z.edgeHi = z.hi;
        const std::map<Tick, double>& b = book[z.side];
        if (b.empty()) return;
        const Tick w = z.hi - z.lo + 1, minPx = b.begin()->first, maxPx = b.rbegin()->first;
        std::vector<double> sz;
        for (int k = 1; k <= P.behindBands; ++k) {
            const Tick a = z.side == BID ? z.lo - k * w : z.hi + (k - 1) * w + 1, e = a + w - 1;
            if (a < minPx || e > maxPx) continue;                     // outside the visible book: not counted
            sz.push_back(band(z.side, a, e));
        }
        z.behindN = (int)sz.size(); z.behindOk = z.behindN >= P.behindMin;
        double zs = band(z.side, z.lo, z.hi);
        if (!sz.empty()) {
            std::vector<double> s2 = sz; std::sort(s2.begin(), s2.end());
            const size_t n = s2.size(); const double med = n % 2 ? s2[n / 2] : 0.5 * (s2[n / 2 - 1] + s2[n / 2]);
            z.bx = zs / std::max(med, 1.0);
            z.rank = 1; for (size_t i = 0; i < sz.size(); ++i) if (sz[i] > zs) z.rank++;
        }
        bool first = true;
        for (std::map<Tick, double>::const_iterator i = b.lower_bound(z.lo); i != b.end() && i->first <= z.hi; ++i)
            if (i->second > 0) { if (first) { z.edgeLo = i->first; first = false; } z.edgeHi = i->first; }
    }
    // shown: appears once durable (rested persistMs at this band), rank 1 and >= showX; stays while >= keepX; goes after graceMs below
    // (0.11.4) a shown zone near the edge of the visible book (fewer than behindMin bands behind it, because price moved and the
    // depth window moved with it) is judged on the bands that ARE visible; with none visible there is no evidence and it stays as it
    // is - before, it was hidden 10 s later and came back when price returned (flicker at the far end of the reach)
    // (0.14.0) a NEAR zone: shown at nearShowX without rank 1, kept to nearKeepX; never APPEARS under minShowX (1.0x) - a shown zone
    // measured under it is hidden at once (no grace) unless it is in an absorption attempt (kept until the attempt ends)
    void showState(Zone& z, Ms t) const {
        const double showAt = std::max(P.minShowX, z.near ? P.nearShowX : P.showX), keepAt = std::max(P.minShowX, z.near ? P.nearKeepX : P.keepX);
        if (!z.shown) {
            if (z.behindOk && (z.near || z.rank == 1) && z.bx >= showAt && z.durable(t, P.persistMs)) { z.shown = true; z.belowSince = -1; }
            return;
        }
        // (0.14.0, coordinator review) a shown zone being touched (in an absorption attempt) stays shown until the attempt ends, whatever
        // its size - that is when he wants to see it absorbed; the 1.0x floor, the keep lines and the grace apply again once it ends
        if (z.inAttempt) { z.belowSince = -1; return; }
        if (z.behindN > 0 && z.bx < P.minShowX) { z.shown = false; z.belowSince = -1; return; }
        const bool strong = z.behindN > 0 && z.bx >= keepAt;
        if (strong || z.behindN == 0) { z.belowSince = -1; return; }
        if (z.belowSince < 0) z.belowSince = t;
        if (t - z.belowSince > P.graceMs) { z.shown = false; z.belowSince = -1; }
    }
    // (0.44.0) the strict 2.0x DRAW state: enters live (drawEnter), leaves only at a closed bar (drawClose -> drawKeep)
    void drawState(Zone& z, Ms t) const {
        if (!z.drawn && drawEnter(z.behindOk, z.near || z.rank == 1, z.bx, z.durable(t, P.persistMs), P.drawX)) z.drawn = true;
    }
    void drawClose() { for (size_t i = 0; i < zones.size(); ++i) if (zones[i].drawn && !drawKeep(zones[i].behindN, zones[i].bx, P.drawX)) zones[i].drawn = false; }
    // (0.44.0) the Market State story step of a zone on the bar that closed at barEnd (+ its open attempt, if any)
    std::string stepOf(long long zoneId, Ms barEnd, const Attempt** open = nullptr) const {
        bool held = false, broke = false;
        for (size_t i = 0; i < ztracks.size(); ++i) {
            const ZTrack& t = ztracks[i];
            if (zoneId > 0 && t.zoneId == zoneId && t.o.resolved() && t.tRes == barEnd) { held = held || t.o.res == zrule::REV; broke = broke || t.o.res == zrule::CONT; }
        }
        const Attempt* a = nullptr;
        for (size_t i = 0; i < attempts.size() && !a; ++i) if (!attempts[i].done && attempts[i].zoneId == zoneId && zoneId > 0) a = &attempts[i];
        if (open) *open = a;
        return zoneStepOf(held, broke, a != nullptr, a ? a->barP : -1, a ? a->barS : -1, a ? a->barE : -1, a ? a->attackVol : 0);
    }
    // a key / gamma level at levelTick is liquidity-confirmed by a shown zone >= confirmX on the right side whose band holds it
    // (0.11.4) keepId = the zone that confirmed this level last time: it keeps the LQ down to confirmKeepX (hysteresis, as the zones
    // themselves: shown at 2x, kept to 1.7x) so the label does not blink on and off while the zone's size hovers around 2.5x
    const Zone* confirming(Tick levelTick, Tick px, long long keepId = 0) const {
        const Zone* best = nullptr;
        for (size_t i = 0; i < zones.size(); ++i) {
            const Zone& z = zones[i];
            if (!z.shown || !z.drawn || z.bx < (z.id == keepId && keepId > 0 ? P.confirmKeepX : P.confirmX)) continue;   // (0.44.0) only a drawn zone names a level
            if (levelTick < z.lo || levelTick > z.hi) continue;
            if (z.side == BID ? z.lo > px : z.hi < px) continue;       // a support below price, a resistance above
            if (!best || z.bx > best->bx) best = &z;
        }
        return best;
    }
    // (0.11.4) the shown zones to draw, strongest first; a shown zone whose real edges overlap a stronger one already listed is left
    // out (two tracked zones a little more than a half-width apart can both be shown for a few seconds: their bars and labels overlapped)
    std::vector<size_t> drawOrder() const {
        std::vector<size_t> idx, out;
        for (size_t i = 0; i < zones.size(); ++i) if (zones[i].drawn) idx.push_back(i);   // (0.44.0) the strict 2.0x draw state
        std::stable_sort(idx.begin(), idx.end(), [this](size_t a, size_t b) { return zones[a].bx > zones[b].bx; });
        for (size_t k = 0; k < idx.size(); ++k) {
            const Zone& z = zones[idx[k]]; bool clash = false;
            for (size_t j = 0; j < out.size() && !clash; ++j) { const Zone& o = zones[out[j]]; clash = z.edgeLo <= o.edgeHi && o.edgeLo <= z.edgeHi; }
            if (!clash) out.push_back(idx[k]);
        }
        return out;
    }

    // ---- a closed bar: decide every attempt (marks are final once written)
    void onBarClose(int bar, long long barTime, Ms barEnd, double close, const std::vector<std::pair<Tick, Tick>>& deltaAbsorbBid,
                    const std::vector<std::pair<Tick, Tick>>& deltaAbsorbAsk) {
        Tick c = 0; if (!tryToTick(close, P.tick, c)) return;
        drawClose();                                                       // (0.44.0) a drawn zone under 2.0x at this close is removed
        if (haveBook && havePx) { nearBook[bar] = nearNow(); while (nearBook.size() > 1200) nearBook.erase(nearBook.begin()); }   // (0.11.0)
        // (0.8.0) s after a touch already decided as held (the defenders often step in on the bars after the close back out): finished
        // attempts first, so one closed on THIS bar is not read twice
        for (size_t i = 0; i < finished.size(); ++i) {
            Attempt& f = finished[i];
            if (f.end.compare(0, 4, "hold") != 0 || f.barE < 0 || bar <= f.barA || bar - f.barE > P.counterBars) continue;
            if (f.barStats.count(bar)) continue;
            counter(f, bar, barTime, barEnd);
            f.barStats[bar] = stats(f, barEnd);
        }
        for (size_t i = 0; i < attempts.size(); ++i) {
            Attempt& a = attempts[i]; if (a.done) continue;
            if (bar < a.barA) continue;
            a.tEnd = barEnd;
            story(a, bar, barEnd, barTime);
            const bool broke = a.side == BID ? c < a.lo : c > a.hi;
            if (!broke) counter(a, bar, barTime, barEnd);                // never an s on the bar that closed through
            a.barStats[bar] = stats(a, barEnd);
            const bool out = a.side == BID ? c > a.hi : c < a.lo;
            const double eaten = a.shown > 0 ? a.attackVol / a.shown : 0;
            // (0.1.3, Rassul 2026-10-10 "pulling was a major factor to why the absorption failed") the defenders leaving without being
            // replaced: PULLING = half+ of the shown size pulled and less than half of what was taken replaced; DEPLETING = eaten 1x+, not replaced
            const double taken = a.attackVol + a.pulled, replaced = a.added + a.refill;
            const double refillX = taken > 0 ? replaced / taken : -1;
            const bool pulling = a.shown > 0 && a.pulled / a.shown >= P.pullShare && refillX >= 0 && refillX < 0.5;
            const bool depleting = !pulling && eaten >= P.eatenBreak && refillX >= 0 && refillX < 0.5;
            // (0.4.0, Rassul 2026-10-10 23:35 "get rid of X and L and LX") the outcome ends the attempt and is LOGGED for the nightly study;
            // nothing is drawn on the price chart. The pane signals (Exh / Pull) come from story().
            // (0.7.0) t = TRAPPED: the attackers traded beyond the zone's far edge, then a bar closes back beyond the zone on the defended
            // side (a green close above a support after the push below) - they broke it, got nothing, now they are stuck
            if (a.barT < 0 && a.breachTrade && (a.side == BID ? c > a.hi : c < a.lo)) { a.barT = bar; push("t", a, barTime, barEnd, -1); }
            if (broke) {
                // (0.17.0) the attempt's own end stays (logged, never a label); Held / Broke come from the ZONE rule (zrClose)
                a.end = std::string("break") + (pulling ? " pulled" : "") + (eaten >= P.eatenBreak ? " eaten" : "") + (depleting ? " depleting" : "");
                close_(a);
                continue;
            }
            if (out && barEnd - a.tA >= P.decisionMs) {
                bool A = false;
                const std::vector<std::pair<Tick, Tick>>& dp = a.side == BID ? deltaAbsorbBid : deltaAbsorbAsk;
                for (size_t k = 0; k < dp.size(); ++k) if (dp[k].first <= a.hi && dp[k].second >= a.lo) A = true;
                a.end = std::string("hold") + (pulling ? " pulled" : "") + (depleting ? " depleting" : "") + (a.barE >= 0 ? " Exh" : "") + (A ? " A" : "")
                      + (a.barO >= 0 ? " offense" : "") + (a.barR >= 0 ? " replenished" : "");
                close_(a);
                continue;
            }
            if (bar - a.barA >= P.maxBars) { a.end = "expired"; close_(a); }
        }
        attempts.erase(std::remove_if(attempts.begin(), attempts.end(), [](const Attempt& a) { return a.done; }), attempts.end());
        zrClose(bar, barTime, barEnd, close);                          // (0.17.0) the zone rule on this closed bar
        curBar = bar;
    }
    int curBar = 0;
    int formBar = 0;                             // the bar in progress (set by the host before trades / books)

    Readout readout(Ms now) const {
        Readout r;
        const Attempt* best = nullptr;
        for (size_t i = 0; i < attempts.size(); ++i) if (!attempts[i].done && (!best || attempts[i].tA > best->tA)) best = &attempts[i];
        if (!best && !finished.empty()) best = &finished.back();
        if (!best) return r;
        r.active = !best->done; r.side = best->side; r.lo = best->lo; r.hi = best->hi;
        r.eaten = best->shown > 0 ? best->attackVol / best->shown : 0;
        r.stayed = best->shown > 0 ? std::max(0.0, std::min(1.0, 1.0 - best->pulled / best->shown)) : 1;
        const Ms t1 = best->done ? (best->attacks.empty() ? best->tA : best->attacks.back().first) : now;
        r.exhaust = exhaustRatio(best->attacks, best->tA, t1);
        r.exhausted = r.exhaust >= 0 && r.exhaust < P.exhaust;
        r.pulled = best->shown > 0 ? std::min(1.0, best->pulled / best->shown) : 0;   // (0.27.0) capped at 100%
        const double taken = best->attackVol + best->pulled, replaced = best->added + best->refill;
        r.refill = taken > 0 ? replaced / taken : -1;
        r.pulling = r.pulled >= P.pullShare && r.refill >= 0 && r.refill < 0.5;
        r.depleting = !r.pulling && r.eaten >= P.eatenBreak && r.refill >= 0 && r.refill < 0.5;
        return r;
    }

    // the record line of a mark (pipe-separated, append-only; LiquidityProfile\<MKT>-<spb>-signals-<session>.csv)
    static std::string header() { return "bar_time|code|reasons|side|zone_lo|zone_hi|shown|eaten|stayed|exhaust|resilience|t_arrival|t_decided|version"; }
    static std::string line(const Mark& m, double tick) {
        std::ostringstream o; o.imbue(std::locale::classic()); o.precision(10);
        o << m.barTime << '|' << m.code << '|' << m.reasons << '|' << (m.side == BID ? "support" : "resistance") << '|'
          << (double)m.lo * tick << '|' << (double)m.hi * tick << '|' << m.shown << '|' << round2(m.eaten) << '|' << round2(m.stayed) << '|'
          << round2(m.exhaust) << '|' << round2(m.resil) << '|' << m.tA << '|' << m.tDecided << '|' << VERSION << '|' << round2(m.value);
        return o.str();
    }
    static bool num(const std::string& t, double& v) {                 // strict full-token parse (checklist #4)
        if (t.empty() || t.size() > 40) return false;
        errno = 0; char* e = nullptr; v = std::strtod(t.c_str(), &e); return !errno && e == t.c_str() + t.size() && finite(v);
    }
    static bool integer(const std::string& t, long long lo, long long hi, long long& v) {
        if (t.empty() || t.size() > 24) return false;
        errno = 0; char* e = nullptr; const long long x = std::strtoll(t.c_str(), &e, 10);
        if (errno || e != t.c_str() + t.size() || x < lo || x > hi) return false;
        v = x; return true;
    }
    static bool parse(const std::string& s, Mark& m, double tick) {
        std::vector<std::string> f; size_t a = 0;
        for (size_t i = 0; i <= s.size(); ++i) if (i == s.size() || s[i] == '|') { f.push_back(s.substr(a, i - a)); a = i + 1; }
        if (f.size() < 14 || !paneCode(f[1]) || !(tick > 0)) return false;
        long long bt = 0, ta = 0, td = 0; double lo = 0, hi = 0;
        if (!integer(f[0], 1, std::numeric_limits<long long>::max(), bt) || !num(f[4], lo) || !num(f[5], hi) || !num(f[6], m.shown) || !num(f[7], m.eaten) || !num(f[8], m.stayed)
            || !num(f[9], m.exhaust) || !num(f[10], m.resil) || !integer(f[11], 0, std::numeric_limits<long long>::max(), ta) || !integer(f[12], 0, std::numeric_limits<long long>::max(), td)) return false;
        if (f[3] != "support" && f[3] != "resistance") return false;
        Tick loTick = 0, hiTick = 0;
        if (!tryToTick(lo, tick, loTick) || !tryToTick(hi, tick, hiTick)) return false;
        m.barTime = bt; m.code = f[1]; m.reasons = f[2]; m.side = f[3] == "support" ? BID : ASK;
        m.lo = loTick; m.hi = hiTick; m.tA = (Ms)ta; m.tDecided = (Ms)td;
        m.value = -1; double v = 0; if (f.size() >= 15 && num(f[14], v)) m.value = v;
        return m.hi >= m.lo;
    }
    static bool paneCode(const std::string& c) { return c == "Exh" || c == "Pull" || c == "ew" || c == "pw" || c == "t" || c == "s" || c == "Held" || c == "Broke"; }   // (0.5.0) older records with LoF / HiB / RpB / RpO are skipped
    static double round2(double v) { return std::floor(v * 100 + 0.5) / 100; }

    // the attempt log line (every attempt, drawn or not) for the nightly scoring
    static std::string attemptHeader() { return "t_arrival|side|zone_lo|zone_hi|shown|eaten|stayed|added_x|refill_x|exhaust|resilience|breach_trade|end|version|pushback|defender_speed|pull_speed|response_cross|t_end|e_bar|o_bar|p_bar|story|r_bar|x_bar|def_net_x|opp_net_x|old_x|zr_adr|zr_lo|zr_hi|zr_x|zr_rule"; }   // (0.17.0) + the zone-rule geometry (the outcome: -zonerule- file)
    std::string attemptLine(const Attempt& a, Ms tEnd) const {
        std::ostringstream o; o.imbue(std::locale::classic()); o.precision(10);
        const double sh = a.shown > 0 ? a.shown : 1;
        o << a.tA << '|' << (a.side == BID ? "support" : "resistance") << '|' << (double)a.lo * P.tick << '|' << (double)a.hi * P.tick << '|'
          << a.shown << '|' << round2(a.attackVol / sh) << '|' << round2(std::max(0.0, 1 - a.pulled / sh)) << '|' << round2(a.added / sh) << '|'
          << round2(a.refill / sh) << '|' << round2(exhaustRatio(a.attacks, a.tA, tEnd)) << '|' << round2(resilience(a)) << '|'
          << (a.breachTrade ? 1 : 0) << '|' << a.end << '|' << VERSION;
        // (the incoming package's buyer / seller response, LOGGED for the nightly study, never drawn): the defending side's
        // aggressive contracts / s vs the defending side's pulled contracts / s over the defence; cross = defenders outpaced pulls
        const double secs = std::max(1.0, (double)(tEnd - a.tA) / 1000.0), tot = a.attackVol + a.counterVol;
        o << '|' << round2(tot > 0 ? a.counterVol / tot : -1) << '|' << round2(a.counterVol / secs) << '|' << round2(a.pulled / secs) << '|'
          << (a.counterVol > a.pulled ? 1 : 0) << '|' << tEnd;
        // (0.2.0) the story: bars after arrival when E / O / P first showed (-1 = never) and their order, e.g. "E>O" = exhaustion, then offense
        std::vector<std::pair<int, char>> seq;
        if (a.barE >= 0) seq.push_back(std::make_pair(a.barE, 'E'));
        if (a.barO >= 0) seq.push_back(std::make_pair(a.barO, 'O'));
        if (a.barP >= 0) seq.push_back(std::make_pair(a.barP, 'P'));
        if (a.barR >= 0) seq.push_back(std::make_pair(a.barR, 'R'));
        std::sort(seq.begin(), seq.end());
        std::string st; for (size_t i = 0; i < seq.size(); ++i) { if (i) st += '>'; st += seq[i].second; }
        o << '|' << (a.barE >= 0 ? a.barE - a.barA : -1) << '|' << (a.barO >= 0 ? a.barO - a.barA : -1) << '|' << (a.barP >= 0 ? a.barP - a.barA : -1) << '|' << (st.empty() ? "-" : st)
          << '|' << (a.barR >= 0 ? a.barR - a.barA : -1) << '|' << (a.barX >= 0 ? a.barX - a.barA : -1)
          << '|' << round2(a.defNet / sh) << '|' << round2(a.oppNet / sh) << '|' << round2(a.oldShare);
        // (0.17.0) the zone-rule geometry of this attempt (empty when the chart had no ADR yet)
        if (zrAdr > 0) { const double c = 0.5 * (double)(a.lo + a.hi) * P.tick, w = zrule::ZONE_W * zrAdr;
            o << '|' << zrAdr << '|' << (c - w) << '|' << (c + w) << '|' << zrule::RES_X * zrAdr << '|' << zrule::VERSION; }
        else o << "|||||" << zrule::VERSION;   // (0.18.0 fix) 5 fields like the header (zr_adr|zr_lo|zr_hi|zr_x|zr_rule): 0.17.0 wrote 4 - zr_rule landed in zr_x
        return o.str();
    }
    std::vector<std::string> attemptLog;          // lines waiting to be appended by the host

    // ---- (0.15.0) the order-by-order feed, one event BEFORE the order book applies it (action 0 NEW, 1 MOD, 2 DEL, 3 FILL, as
    // OrderFlowFeed.h; side = the order's own side when it is known; had / oldPx / oldSize = its resting state before this event).
    // Only OPEN attempts count (decided ones are frozen - no repaint). FILL in the zone on the zone's side = hit; after the first hit, a
    // NEW in the zone, a size increase in place, or an order moved INTO the zone from outside = rebuilt.
    void mboEvent(int action, int side, Tick px, long long size, bool had, Tick oldPx, long long oldSize) {
        if ((side != BID && side != ASK) || size < 0 || size > 10000000 || px <= 0) return;
        for (size_t i = 0; i < attempts.size(); ++i) {
            Attempt& a = attempts[i];
            if (a.done || a.side != side || px < a.lo || px > a.hi) continue;
            if (action == 3) { a.hitLots += size; a.hitAt[px].first += size; continue; }
            if (a.hitLots <= 0) continue;
            long long add = 0;
            if (action == 0) add = size;
            else if (action == 1 && had) {
                const bool wasIn = oldPx >= a.lo && oldPx <= a.hi;
                if (oldPx == px) { if (size > oldSize) add = size - oldSize; }
                else if (!wasIn) add = size;
            }
            if (add <= 0) continue;
            // (0.16.1) only at a price that was hit, at most the lots hit there and not yet restored
            std::map<Tick, std::pair<long long, long long>>::iterator h = a.hitAt.find(px);
            if (h == a.hitAt.end()) continue;
            const long long room = h->second.first - h->second.second;
            const long long k = add < room ? add : room;
            if (k > 0) { h->second.second += k; a.rebuiltLots += k; }
        }
    }
    // the open attempt on a zone (the one being hit), nullptr = none
    const Attempt* openOn(long long zoneId) const {
        const Attempt* best = nullptr;
        for (size_t i = 0; i < attempts.size(); ++i) if (!attempts[i].done && attempts[i].zoneId == zoneId && (!best || attempts[i].tA > best->tA)) best = &attempts[i];
        return best;
    }
    // (0.15.0) the makeup record at each decision (its own file: the attempts file's columns are unchanged)
    static std::string makeupHeader() { return "t_arrival|side|zone_lo|zone_hi|end|t_end|hit_lots|rebuilt_lots|rebuilt_pct|mk_ok|orders|lots|top_lots|top_pct|coverage|one_order|came_base|came_arrival|came_pct|came|version"; }
    std::string makeupLine(const Attempt& a, Ms tEnd) const {
        std::ostringstream o; o.imbue(std::locale::classic()); o.precision(10);
        o << a.tA << '|' << (a.side == BID ? "support" : "resistance") << '|' << (double)a.lo * P.tick << '|' << (double)a.hi * P.tick << '|' << a.end << '|' << tEnd
          << '|' << a.hitLots << '|' << a.rebuiltLots << '|' << rebuiltPct(a.rebuiltLots, a.hitLots) << '|' << (a.mkSet && a.mk.ok ? 1 : 0)
          << '|' << (a.mkSet ? a.mk.orders : -1) << '|' << (a.mkSet ? a.mk.lots : -1) << '|' << (a.mkSet ? a.mk.top : -1) << '|' << (a.mkSet ? a.mk.topPct() : -1)
          << '|' << round2(a.mkSet ? a.mk.coverage : -1) << '|' << (a.mkSet && oneOrder(a.mk, P.oneOrderShare) ? 1 : 0);
        const Came c = cameOf(a.cameBase, a.cameArr, P.cameBand);   // (0.15.0) signed %: + building, - leaving; -1 base = not known
        o << '|' << round2(a.cameBase) << '|' << round2(a.cameArr) << '|' << (c.state == CAME_NONE ? 0 : (a.cameArr < a.cameBase ? -c.pct : c.pct))
          << '|' << (c.state == CAME_NONE ? "none" : cameWord(c.state)) << '|' << VERSION;
        return o.str();
    }
    std::vector<std::string> makeupLog;           // (0.15.0) lines waiting to be appended by the host

    // (0.2.0) the story marks, each decided once at a bar close: E = the attackers' 2nd-half pace < 0.5 x the 1st half; P = half+ of the
    // shown size pulled and not replaced; O = the defenders out-traded the attackers in a bar after the attackers had led one (defense
    // first, then offense - Rassul 2026-10-10 "buyers playing defense first and then offense")
    // (0.4.0, agreed with Rassul 2026-10-10 23:31-23:35, one signal per idea, each ONLY on its pane, decided once at a bar close):
    //   AGGRESSION  Exh  the attackers' pace dried up (2nd-half / 1st-half volume < this market's 'exhaust')
    //   DEFENSE     Pull the defenders pulled (this market's 'pull_share' of the shown size, not replaced) WHILE the attackers lead
    // (0.5.0, Rassul 2026-10-11 07:4x "remove whatever doesn't work and keep what works", "remove stop") LoF / HiB / RpB / RpO removed:
    // the data did not confirm them (LoF pace 2x+ in 0-4% of touches and heavy defender volume held LESS; RpB only in 2 of 7 markets,
    // backwards in ES / CL). The O / R crosses are still LOGGED in the attempt line for the study, never drawn.
    // Each mark is a record line, re-read after a restart: never repaints. The host draws a code only where the market's nightly study
    // confirmed it (SIGNAL lines); nothing unconfirmed is drawn.
    void story(Attempt& a, int bar, Ms barEnd, long long barTime) {
        // (0.7.0) e = Exh: the attackers' 2nd-half / 1st-half volume since arrival under THIS MARKET's line (P.exhaust, from the data);
        //        e? = the watch: under twice the line (at most 0.9x) - the attack is slowing toward it. e? first, e when the line is crossed.
        // e is decided from the study's decision point on (arrival + 3 min: the window lra.mbo_study measures exhaust_ratio over), e? from
        // 1 min - a few quiet seconds right after arrival are not exhaustion
        if (a.barE < 0 && firstHalf(a, barEnd) >= P.minAttack && barEnd - a.tA >= 60000) {
            const double ex = exhaustRatio(a.attacks, a.tA, barEnd);
            if (ex >= 0 && ex < P.exhaust && barEnd - a.tA >= P.decisionMs) { a.barE = bar; a.exE = ex; push("Exh", a, barTime, barEnd, ex); }
            else if (a.barEW < 0 && ex >= 0 && ex < std::min(0.9, 2 * P.exhaust)) { a.barEW = bar; push("ew", a, barTime, barEnd, ex); }
        }
        // (0.7.0) p = Pull: the share of the shown size pulled reaches THIS MARKET's line while the attackers lead (the indicator's own
        //        snapshot measure, lra.mbo_study pull_snap); p? = the watch: half-way to the line (first look: hold odds already drop there)
        if (a.barP < 0 && a.shown > 0 && a.attackVol > a.counterVol) {
            const double pr = std::min(1.0, a.pulled / a.shown);   // (0.27.0) capped at 100% (same decision: the line is 0.5-1.0)
            if (pr >= P.pullShare) { a.barP = bar; push("Pull", a, barTime, barEnd, pr); }
            else if (a.barPW < 0 && pr >= 0.5 * P.pullShare) { a.barPW = bar; push("pw", a, barTime, barEnd, pr); }
        }
        // the O / R crosses (logged only): the defenders' total since arrival crosses above the attackers' after they led; the defenders'
        // net resting size crosses above the other side's after it led
        if (a.attackVol > a.counterVol) a.defendedFirst = true;
        else if (a.barO < 0 && a.defendedFirst && a.counterVol > a.attackVol) a.barO = bar;
        if (a.oppNet >= a.defNet && (a.oppNet != 0 || a.defNet != 0)) a.pulledFirst = true;
        else if (a.barR < 0 && a.pulledFirst && a.defNet > a.oppNet) a.barR = bar;
    }
    // (0.8.0) s = COUNTER: the defenders turned aggressive after the attackers exhausted - a closed bar, from the e bar on (up to
    // counterBars), where the defenders out-traded the attackers (the whole bar, IRT's own bar statistics), the bar traded within one zone
    // width of the zone, and their volume per tick traded was counterX x their normal (the 20 bars before arrival). The Counter readout
    // is that pace on every bar of the touch. lra.mbo_study ctr_seq measures exactly this; lra.liq_params replays it for the line.
    void counter(Attempt& a, int bar, long long barTime, Ms barEnd) {
        std::map<int, BarNorm>::const_iterator it = barNorm.find(bar);
        const double nrm = defNormal(a.side, a.barA);
        if (it == barNorm.end() || nrm <= 0) return;
        const BarNorm& b = it->second;
        const bool sup = a.side == BID;
        const double dv = sup ? b.buy : b.sell, av = sup ? b.sell : b.buy, x = dv / (double)b.ticks / nrm;
        a.counterNow = x;
        if (a.barE < 0 || a.barS >= 0 || bar < a.barE || bar - a.barE > P.counterBars) return;
        const Tick w = a.hi - a.lo + 1;
        const bool near = b.range && (sup ? b.lo <= a.hi + w : b.hi >= a.lo - w);
        if (!(dv > av) || !near || x + 1e-9 < P.counterX) return;
        a.barS = bar; a.tS = barEnd; a.sX = x;
        push("s", a, barTime, barEnd, x);
        const std::string dfn = sup ? "Buyers" : "Sellers", att = sup ? "sellers" : "buyers";
        marks.back().reasons = dfn + " stepped in: " + fx(x) + " normal, out-traded " + att;
    }
    static bool topPane(const std::string& c) { return c == "Exh" || c == "ew" || c == "Held" || c == "Broke"; }
    // green = buyers win / bullish, red = sellers win / bearish (Rassul: buyers always green, sellers always red)
    static bool bullish(const Mark& m) {
        if (m.code == "Exh" || m.code == "ew" || m.code == "t" || m.code == "s" || m.code == "Held") return m.side == BID;   // good for the defenders
        if (m.code == "Broke") return m.side == ASK;
        return m.side == ASK;                                // Pull: buyers pulling at a support is bearish, sellers pulling at a resistance bullish
    }
    Lines L;                                       // (0.7.0) set by the host from the norms file
    static std::string fx(double v) { char b[16]; std::snprintf(b, sizeof(b), v >= 10 ? "%.0fx" : "%.1fx", v); return b; }
    static std::string pc(double v) { char b[16]; std::snprintf(b, sizeof(b), "%.0f%%", v * 100.0); return b; }
    // (0.7.0) the reason in plain words when an attempt is decided (held = absorbed, broke), built ONLY from numbers whose line is confirmed
    // for this market (Rassul's wording: "sellers hit the bid 2.4x normal, then exhausted (0.3x); buyers held and refilled their bids (1.2x)")
    std::string reason(const Attempt& a, bool held, Ms tEnd) const {
        const Stats st = stats(a, tEnd); const bool sup = a.side == BID;
        const std::string att = sup ? "sellers" : "buyers", dfn = sup ? "buyers" : "sellers", hit = sup ? "hit the bid" : "lifted the offer", ord = sup ? "bids" : "offers";
        std::string first = att + " " + hit;
        if (L.attack > 0 && st.attack >= 0) first += " " + fx(st.attack) + " normal";
        const bool exh = L.exh > 0 && st.exh >= 0 && st.exh < L.exh;
        if (L.exh > 0 && st.exh >= 0) first += exh ? ", then slowed to " + fx(st.exh) : (held ? ", kept going (" + fx(st.exh) + ")" : " and never slowed");   // (0.17.0) no "exhausted"
        std::string second;
        const bool pulled = L.pulled > 0 && st.pulled >= L.pulled;
        if (pulled) second = dfn + " pulled " + pc(std::min(st.pulled, 9.99)) + " of their " + ord;
        else if (L.rebuilt > 0 && st.rebuilt >= L.rebuilt) second = dfn + " re-added " + ord + " (" + fx(st.rebuilt) + ")";
        else if (!held && L.eaten > 0 && st.eaten >= L.eaten) second = att + " ate " + pc(st.eaten) + " of everything there";
        else second = held ? "price closed back out" : dfn + " did not re-add";
        return std::string(held ? "Held: " : "Broke: ") + first + "; " + second;
    }
    void pushReason(const Attempt& a, bool held, long long barTime, Ms barEnd) {
        Mark m; m.code = held ? "Held" : "Broke"; m.reasons = reason(a, held, barEnd); m.value = -1; m.side = a.side; m.barTime = barTime;
        m.lo = a.lo; m.hi = a.hi; m.shown = a.shown; m.tA = a.tA; m.tDecided = barEnd; marks.push_back(m);
    }
    // ---- (0.17.0, Rassul 2026-10-14 18:25 CT approved) HELD / BROKE BY THE ZONE RULE (ZoneRule.h, lra.level_confluence LCF v1.1):
    // each arrival at a liquidity zone starts an encounter on the zone centre +/- 0.06 ADR (ADR = the chart's own prior 20 sessions,
    // set by the host as zrAdr). On every CLOSED bar from the arrival bar on: Held = a close 0.10 ADR back out beyond the near edge;
    // Broke = a close 0.10 ADR beyond the far edge with no close back inside within 15 min (5 x 3-min bars) - the label is written on
    // the bar where that became known, never before; otherwise in zone (no label); after 2 h pinned (logged, no label). A first touch
    // or a tick through is never a break. Once written a label never changes (marks are append-only and redrawn from the record).
    // Debounce (zone_entries): a new encounter on the same side within an open / just-resolved encounter's zone waits for 2 closes
    // at least the half-width beyond it.
    double zrAdr = -1; int zrSpb = 180;
    struct ZTrack { long long zoneId = 0; Ms tA = 0; int side = BID; Tick lo = 0, hi = 0; double shown = 0; int barA = -1; zrule::Outcome o; zrule::Reset reset; bool restored = false; Ms tRes = 0; };
    std::vector<ZTrack> ztracks;
    std::vector<std::string> zoneLog;                    // lines for the host (OPEN at the arrival, the outcome once final)
    long long zrOpened = 0, zrRev = 0, zrCont = 0, zrPin = 0, zrCens = 0, zrSkipped = 0, zrNoAdr = 0;
    static std::string zoneHeader() { return "event|t_arrival|side|zone_lo|zone_hi|shown|adr|zr_lo|zr_hi|x|res|failed_breaks|bars|break_bar|t_decided|bar_time|rule|version"; }
    std::string zoneLine(const char* ev, const ZTrack& z, Ms tDec, long long barTime) const {
        std::ostringstream o; o.imbue(std::locale::classic()); o.precision(10);
        o << ev << '|' << z.tA << '|' << (z.side == BID ? "support" : "resistance") << '|' << (double)z.lo * P.tick << '|' << (double)z.hi * P.tick << '|' << z.shown
          << '|' << (z.o.x / zrule::RES_X) << '|' << z.o.lo << '|' << z.o.hi << '|' << z.o.x << '|' << zrule::resName(z.o.res) << '|' << z.o.failed << '|' << z.o.n + 1
          << '|' << z.o.kb << '|' << tDec << '|' << barTime << '|' << zrule::VERSION << '|' << VERSION;
        return o.str();
    }
    bool zrBlocked(int side, double centre) const {
        for (size_t i = 0; i < ztracks.size(); ++i) {
            const ZTrack& t = ztracks[i]; if (t.side != side) continue;
            if (centre < t.o.lo || centre > t.o.hi) continue;
            if (!t.o.resolved() || !t.reset.done) return true;
        }
        return false;
    }
    void zrStart(const Attempt& a) {
        if (!(zrAdr > 0) || !finite(zrAdr)) { zrNoAdr++; return; }
        const double c = 0.5 * (double)(a.lo + a.hi) * P.tick;
        if (zrBlocked(a.side, c)) { zrSkipped++; return; }
        ZTrack t; t.zoneId = a.zoneId; t.tA = a.tA; t.side = a.side; t.lo = a.lo; t.hi = a.hi; t.shown = a.shown; t.barA = a.barA;
        t.o = zrule::makeOutcome(a.side == BID, c, zrAdr, zrSpb);
        t.reset.lo = t.o.lo; t.reset.hi = t.o.hi; t.reset.dist = zrule::ZONE_W * zrAdr;
        ztracks.push_back(t); zrOpened++;
        zoneLog.push_back(zoneLine("OPEN", t, 0, 0));
        while (ztracks.size() > 200) ztracks.erase(ztracks.begin());
    }
    // a track rebuilt from the record after a restart (OPEN with no outcome yet): the host replays the closed bars since its arrival
    bool zrRestore(const std::string& ln, int barA) {
        std::vector<std::string> f; size_t b = 0;
        for (size_t i = 0; i <= ln.size(); ++i) if (i == ln.size() || ln[i] == '|') { f.push_back(ln.substr(b, i - b)); b = i + 1; }
        if (f.size() < 18 || f[0] != "OPEN") return false;
        long long tA = 0; double lo = 0, hi = 0, sh = 0, adr = 0;
        if (!integer(f[1], 1, std::numeric_limits<long long>::max(), tA) || !num(f[3], lo) || !num(f[4], hi) || !num(f[5], sh) || !num(f[6], adr) || adr <= 0) return false;
        if (f[2] != "support" && f[2] != "resistance") return false;
        for (size_t i = 0; i < ztracks.size(); ++i) if (ztracks[i].tA == tA) return false;     // already here
        ZTrack t; t.tA = tA; t.side = f[2] == "support" ? BID : ASK; t.shown = sh; t.barA = barA; t.restored = true;
        if (!tryToTick(lo, P.tick, t.lo) || !tryToTick(hi, P.tick, t.hi)) return false;
        t.o = zrule::makeOutcome(t.side == BID, 0.5 * (lo + hi), adr, zrSpb);
        t.reset.lo = t.o.lo; t.reset.hi = t.o.hi; t.reset.dist = zrule::ZONE_W * adr;
        ztracks.push_back(t); return true;
    }
    void zrResolved(const std::string& ln) {                  // an outcome already in the record: that encounter is done (no second label)
        std::vector<std::string> f; size_t b = 0;
        for (size_t i = 0; i <= ln.size(); ++i) if (i == ln.size() || ln[i] == '|') { f.push_back(ln.substr(b, i - b)); b = i + 1; }
        long long tA = 0; if (f.size() < 18 || f[0] != "RES" || !integer(f[1], 1, std::numeric_limits<long long>::max(), tA)) return;
        resolvedSeen.insert(tA);
    }
    std::set<long long> resolvedSeen;
    void zrClose(int bar, long long barTime, Ms barEnd, double close) {
        for (size_t i = 0; i < ztracks.size(); ++i) {
            ZTrack& t = ztracks[i];
            if (t.o.resolved()) { t.reset.onClose(close); continue; }
            if (t.barA < 0 || bar < t.barA) continue;
            if (!t.o.onClose(close, barEnd / 1000)) continue;
            t.tRes = barEnd;
            if (resolvedSeen.count(t.tA)) continue;               // decided before a restart: already labelled / logged
            resolvedSeen.insert(t.tA);
            const int r = t.o.res;
            if (r == zrule::REV) zrRev++; else if (r == zrule::CONT) zrCont++; else if (r == zrule::PIN) zrPin++; else zrCens++;
            zoneLog.push_back(zoneLine("RES", t, barEnd, barTime));
            if (r != zrule::REV && r != zrule::CONT) continue;    // pinned / censored: logged for the study, no label
            const bool held = r == zrule::REV;
            const Attempt* a = nullptr;
            for (size_t k = 0; k < attempts.size() && !a; ++k) if (attempts[k].tA == t.tA && attempts[k].zoneId == t.zoneId) a = &attempts[k];
            for (size_t k = finished.size(); k-- > 0 && !a;) if (finished[k].tA == t.tA && finished[k].zoneId == t.zoneId) a = &finished[k];
            if (a && !t.restored) { pushReason(*a, held, barTime, barEnd); continue; }
            Mark m; m.code = held ? "Held" : "Broke"; m.value = -1; m.side = t.side; m.barTime = barTime; m.lo = t.lo; m.hi = t.hi; m.shown = t.shown; m.tA = t.tA; m.tDecided = barEnd;
            m.reasons = held ? "Held: closed 0.10 ADR back out of the zone" : "Broke: closed 0.10 ADR beyond the zone, no close back inside in 15 min";
            marks.push_back(m);
        }
        for (size_t i = 0; i < ztracks.size();) {               // done with: resolved, reset passed (or 6 h old)
            const ZTrack& t = ztracks[i];
            if (t.o.resolved() && (t.reset.done || (t.tRes > 0 && barEnd - t.tRes > 6LL * 3600 * 1000))) ztracks.erase(ztracks.begin() + i); else ++i;
        }
    }
    // the session ended (17:00 CT): an open break candidate counts (the study's i_end), else pinned - logged, labelled as above
    void zrEndSession(long long barTime, Ms barEnd) {
        for (size_t i = 0; i < ztracks.size(); ++i) {
            ZTrack& t = ztracks[i]; if (t.o.resolved() || !t.o.endSession()) continue;
            t.tRes = barEnd;
            if (resolvedSeen.count(t.tA)) continue;
            resolvedSeen.insert(t.tA);
            const int r = t.o.res; if (r == zrule::CONT) zrCont++; else zrPin++;
            zoneLog.push_back(zoneLine("RES", t, barEnd, barTime));
            if (r == zrule::CONT) {
                Mark m; m.code = "Broke"; m.value = -1; m.side = t.side; m.barTime = barTime; m.lo = t.lo; m.hi = t.hi; m.shown = t.shown; m.tA = t.tA; m.tDecided = barEnd;
                const Attempt* a = nullptr;
                for (size_t k = finished.size(); k-- > 0 && !a;) if (finished[k].tA == t.tA && finished[k].zoneId == t.zoneId) a = &finished[k];
                m.reasons = a && !t.restored ? reason(*a, false, barEnd) : "Broke: closed 0.10 ADR beyond the zone at the session end";
                marks.push_back(m);
            }
        }
        ztracks.clear();
    }
    // (0.7.0) the warning for the zone price is in now: the strongest that applies (risk first, the good ones only when nothing worse
    // applies); every condition uses only lines confirmed for this market; nothing when no attempt is live or nothing applies
    bool warning(Ms now, Warn& w, double tick) const {
        const Attempt* a = nullptr;
        for (size_t i = 0; i < attempts.size(); ++i) if (!attempts[i].done && (!a || attempts[i].tA > a->tA)) a = &attempts[i];
        // (0.8.0) STEPPING IN: an s in the last 2 bars (a live touch, or one just decided as held) - the defenders turned aggressive
        const Attempt* sa = nullptr;
        if (L.counter > 0) {
            if (a && a->barS >= 0) sa = a;
            else for (size_t i = finished.size(); i-- > 0;) if (finished[i].barS >= 0 && now - finished[i].tS <= 2 * P.barMs && now >= finished[i].tS) { sa = &finished[i]; break; }
        }
        auto stepping = [&](const Attempt& x) {
            const bool s_ = x.side == BID;
            w = Warn(); w.name = std::string(s_ ? "BUYERS" : "SELLERS") + " STEPPING IN"; w.favour = s_ ? +1 : -1;
            w.detail = std::string(s_ ? "sellers" : "buyers") + " slowed" + (x.exE >= 0 ? " (" + fx(x.exE) + ")" : std::string()) + "; " +
                       (s_ ? "buyers now lifting offers " : "sellers now hitting bids ") + fx(x.sX) + " normal";
            return true;
        };
        if (!a) return sa ? stepping(*sa) : false;
        const Stats st = stats(*a, now); const bool sup = a->side == BID; const int good = sup ? +1 : -1, bad = -good;
        const std::string att = sup ? "sellers" : "buyers", dfn = sup ? "buyers" : "sellers", hitting = sup ? "hitting the bid" : "lifting the offer",
            ord = sup ? "bids" : "offers", Att = sup ? "SELLERS" : "BUYERS", Dfn = sup ? "BUYERS" : "SELLERS", Ord = sup ? "BIDS" : "OFFERS";
        const bool okA = L.attack > 0 && st.attack >= 0, okX = L.exh > 0 && st.exh >= 0, okP = L.pulled > 0 && st.pulled >= 0, okE = L.eaten > 0 && st.eaten >= 0;
        const bool heavy = okA && st.attack >= L.attack, light = okA && st.attack < L.attack;
        const bool exh = okX && st.exh < L.exh, slowing = okX && !exh && st.exh < std::min(0.9, 2 * L.exh);
        const bool lead = a->attackVol > a->counterVol;
        const bool pull = okP && lead && st.pulled >= L.pulled, pullW = okP && lead && !pull && st.pulled >= 0.5 * L.pulled;
        const bool thin = okE && st.eaten >= L.eaten;
        const bool trapped = L.trapped && a->barT >= 0;
        auto A = [&]() { return okA ? " " + fx(st.attack) + " normal" : std::string(); };
        char px[32]; std::snprintf(px, sizeof(px), "%.*f", decimals(tick), (double)(sup ? a->hi : a->lo) * tick);
        w = Warn();
        if (heavy && pull && !exh) { w.name = sup ? "BREAKDOWN RISK" : "BREAKOUT RISK"; w.favour = bad;
            w.detail = att + " " + hitting + A() + ", not slowing; " + dfn + " pulling their " + ord + " (" + pc(st.pulled) + ")"; }
        else if (exh && pull) { w.name = Att + " SLOWED, " + Ord + " PULLED ?"; w.favour = bad;
            w.detail = att + " slowed (" + fx(st.exh) + "), but " + dfn + " are pulling their " + ord + " (" + pc(st.pulled) + ")"; }
        else if (pull) { w.name = Dfn + " PULLING"; w.favour = bad;
            w.detail = dfn + " pulling their " + ord + " (" + pc(st.pulled) + ") while " + att + " keep " + hitting; }
        else if (thin && !exh) { w.name = Ord + " THINNING"; w.favour = bad;
            w.detail = att + " have eaten " + pc(st.eaten) + " of everything at " + px + " (line " + pc(L.eaten) + ")"; }
        else if (pullW) { w.name = "PULLING WATCH"; w.favour = bad;
            w.detail = dfn + " have pulled " + pc(st.pulled) + " of their " + ord + " while " + att + " keep " + hitting + " (Pull line " + pc(L.pulled) + ")"; }
        else if (sa == a && a->barS >= 0) return stepping(*a);
        else if (heavy && slowing) { w.name = "EXHAUSTION WATCH"; w.favour = 0;
            w.detail = att + " " + hitting + A() + " and starting to slow (" + fx(st.exh) + ")"; }
        else if (heavy && !exh) { w.name = Dfn + " HOLDING"; w.favour = 0;
            w.detail = att + " " + hitting + A() + ", not slowing; " + dfn + " holding - wait"; }
        else if (trapped) { w.name = "TRAPPED " + Att; w.favour = good;
            w.detail = att + " pushed past " + std::string(px) + " and failed; price back " + (sup ? "above" : "below") + " the zone"; }
        else if (exh && light) { w.name = Att + " LIGHT, SLOWING ?"; w.favour = good;
            w.detail = att + " tested the " + (sup ? "bid" : "offer") + " lightly (" + fx(st.attack) + " normal) and slowed (" + fx(st.exh) + ")"; }
        else if (exh) { w.name = Att + " SLOWED ?"; w.favour = good;
            w.detail = att + " slowed (" + fx(st.exh) + "); price still at the zone" +
                       (L.rebuilt > 0 && st.rebuilt >= L.rebuilt ? "; " + dfn + " re-adding " + ord + " (" + fx(st.rebuilt) + ")" : std::string()); }
        else return false;
        return true;
    }
    static double resilience(const Attempt& a) {
        if (a.startSize - a.minSize <= 0) return -1;
        return std::max(0.0, (a.curSize - a.minSize) / (a.startSize - a.minSize));
    }

private:
    static double get(const std::map<Tick, double>& m, Tick k) { std::map<Tick, double>::const_iterator i = m.find(k); return i == m.end() ? 0 : i->second; }
    double firstHalf(const Attempt& a, Ms t1) const {
        const Ms mid = a.tA + (t1 - a.tA) / 2; double v = 0;
        for (size_t i = 0; i < a.attacks.size(); ++i) if (a.attacks[i].first < mid) v += a.attacks[i].second;
        return v;
    }
    void applyFlow(const FlowEvt& e) {
        for (size_t i = 0; i < attempts.size(); ++i) {
            Attempt& a = attempts[i]; if (a.done || e.t < a.tA) continue;
            const double net = e.added + e.refill - e.pulled;
            if (a.side == e.side && e.px >= a.lo && e.px <= a.hi) {
                a.pulled += e.pulled; a.added += e.added; a.refill += e.refill; a.defNet += net;
            } else if (a.side != e.side && e.px >= a.oppLo && e.px <= a.oppHi) {
                a.oppNet += net;
            } else continue;
            Attempt::BarFlow& f = a.flowBars[formBar]; f.rep = a.added + a.refill; f.pul = a.pulled; f.defNet = a.defNet; f.oppNet = a.oppNet; f.book = true;
        }
    }
    void track(Ms t, int side, Tick center, double size) {
        Zone* z = nullptr;
        for (size_t i = 0; i < zones.size(); ++i) if (zones[i].side == side && std::llabs(zones[i].center - center) <= hw) { z = &zones[i]; break; }
        if (!z) { Zone n; n.id = nextZone++; n.side = side; n.born = t; zones.push_back(n); z = &zones.back(); }
        if (!z->inAttempt) { z->center = center; z->hw = hw; z->lo = center - hw; z->hi = center + hw; }   // frozen while touched
        z->size = size; z->peak = std::max(z->peak, size); z->seen = t;
    }
    void startAttempts(Ms t, Tick px, double buy, double sell) {
        if (!havePx || !haveBook) return;
        for (int side = 0; side < 2; ++side) {
            // one attempt per side per arrival: the biggest durable zone the trade is in (a stale neighbour never doubles it)
            Zone* best = nullptr;
            for (size_t i = 0; i < zones.size(); ++i) {
                Zone& z = zones[i];
                if (z.side != side || !z.durable(t, P.persistMs) || t - z.seen > P.graceMs) continue;
                if (P.requireShown && !z.shown && !dpAt(side, z.lo, z.hi)) continue;   // (0.10.0) a drawn zone, (0.11.0) or a Delta Profile A on it
                if (px < z.lo || px > z.hi) continue;
                if (z.inAttempt) { best = nullptr; break; }               // already being defended: nothing new
                const bool fromOutside = z.side == BID ? lastPx > z.hi : lastPx < z.lo;
                if (!fromOutside) continue;
                if (!best || z.size > best->size) best = &z;
            }
            if (!best) continue;
            Zone& z = *best;
            const double shown = band(z.side, z.lo, z.hi);
            if (shown < P.minShown) continue;
            Attempt a; a.zoneId = z.id; a.side = z.side; a.lo = z.lo; a.hi = z.hi; a.tA = t; a.barA = formBar > 0 ? formBar : curBar;
            if (const std::pair<Tick, Tick>* d = dpAt(z.side, z.lo, z.hi)) { a.fromDelta = !z.shown; a.dpLo = d->first; a.dpHi = d->second; }
            a.shown = shown; a.startSize = a.minSize = a.curSize = shown;
            a.oldShare = zoneOld(z.side, z.lo, z.hi, t);
            a.cameBase = z.nearBase; a.cameArr = z.size;                  // (0.15.0) 'as price came' at arrival, recorded at the decision
            const Tick w = z.hi - z.lo + 1;                               // (0.3.0) the other side's band: same width, just beyond the zone
            if (a.side == BID) { a.oppLo = z.hi + 1; a.oppHi = z.hi + w; } else { a.oppLo = z.lo - w; a.oppHi = z.lo - 1; }
            const double first = a.side == BID ? sell : buy;          // the trade that arrived counts as the first attack
            if (first > 0) { a.attackVol = first; a.attacks.push_back(std::make_pair(t, first)); }
            a.flowBars[a.barA].attack += first > 0 ? first : 0;
            attempts.push_back(a); z.inAttempt = true;
            zrStart(attempts.back());                                   // (0.17.0) the zone-rule encounter of this arrival
        }
    }
    void push(const char* code, const Attempt& a, long long barTime, Ms barEnd, double value) {
        Mark m; m.code = code; m.reasons = ""; m.value = value; m.side = a.side; m.barTime = barTime; m.lo = a.lo; m.hi = a.hi; m.shown = a.shown;
        m.eaten = a.shown > 0 ? a.attackVol / a.shown : 0; m.stayed = a.shown > 0 ? std::max(0.0, std::min(1.0, 1 - a.pulled / a.shown)) : 1;
        m.exhaust = exhaustRatio(a.attacks, a.tA, barEnd); m.resil = resilience(a); m.tA = a.tA; m.tDecided = barEnd;
        marks.push_back(m);
    }
    void close_(Attempt& a) {
        a.done = true; ++finTotal;
        attemptLog.push_back(attemptLine(a, a.tEnd > a.tA ? a.tEnd : (a.attacks.empty() ? a.tA : a.attacks.back().first)));
        makeupLog.push_back(makeupLine(a, a.tEnd > a.tA ? a.tEnd : (a.attacks.empty() ? a.tA : a.attacks.back().first)));   // (0.15.0)
        if (makeupLog.size() > 500) makeupLog.erase(makeupLog.begin());
        for (size_t i = 0; i < zones.size(); ++i) if (zones[i].id == a.zoneId) zones[i].inAttempt = false;
        finished.push_back(a); if (finished.size() > 50) finished.erase(finished.begin());
    }
    void trim(Ms t) {
        while (!flow.empty() && t - flow.front().t > 3600000) flow.pop_front();
        while (!tape.empty() && t - tape.front().t > 3600000) tape.pop_front();
    }
};

// (0.45.2) the MKT tiers in force: the 5-session pool once it has a pick (kept between its 5-s recomputes - NEVER replaced by the session's own
// stats on the calcs in between), else the session's own live stats; false = still learning (the caller uses 2 / 4 / 8 x the start)
struct MktPool { bool ok = false; double mu = 0, v[3] = { 0, 0, 0 }, q266 = 0; };
inline bool mktTiersPick(const MktPool& pool, bool liveOk, double liveMu, const double liveV[3], double& mu, double v[3], double& q266) {
    if (pool.ok) { mu = pool.mu; for (int k = 0; k < 3; ++k) v[k] = pool.v[k]; q266 = pool.q266; return true; }
    if (liveOk) { mu = liveMu; for (int k = 0; k < 3; ++k) v[k] = liveV[k]; q266 = 0; return true; }
    return false;
}
// (0.45.2, Rassul 20:06 "yes") LMT F colour = the RESTING side of the filled limit order: a resting bid (buyers' limit, hit by sellers) GREEN,
// a resting offer (sellers' limit, lifted by buyers) RED; the same for icebergs (their "I" stays white)
inline bool lmtFillGreen(bool restingBid) { return restingBid; }
// (0.45.3, Rassul 23:37 "make the colors in the mkt and lmt f lighter, i can barely see them") the TEXT inks of the MKT / LMT F / LMT R
// column (0x00RRGGBB as IRT COLOR): green = buyers, red = sellers, no new hues - only lighter shades. The bigger tier stays the brighter
// one: FULL (2SD+, bold; 3SD+ also a bigger number) = green-300 / red-300, DIM (1SD, or a fill older than 30 min, plain) = green-400 /
// red-400. Contrast on the column background 0x1B1410 (WCAG): full 12.96 / 9.59, dim 10.44 / 6.58 (was 7.99 / 4.84 and 3.36 / 2.34).
static const unsigned COL_BG = 0x001B1410u;
static const unsigned COL_INK_G = 0x0086EFACu, COL_INK_R = 0x00FCA5A5u, COL_INK_G_DIM = 0x004ADE80u, COL_INK_R_DIM = 0x00F87171u;
inline unsigned colInk(bool green, bool dim) { return green ? (dim ? COL_INK_G_DIM : COL_INK_G) : (dim ? COL_INK_R_DIM : COL_INK_R); }
inline double relLum(unsigned c) {   // WCAG 2.x relative luminance of 0x00RRGGBB
    auto ch = [](unsigned v) { const double x = (double)v / 255.0; return x <= 0.03928 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4); };
    return 0.2126 * ch((c >> 16) & 255u) + 0.7152 * ch((c >> 8) & 255u) + 0.0722 * ch(c & 255u);
}
inline double contrast(unsigned a, unsigned b) { const double x = relLum(a), y = relLum(b); return x > y ? (x + 0.05) / (y + 0.05) : (y + 0.05) / (x + 0.05); }
}  // namespace lqp
#endif
