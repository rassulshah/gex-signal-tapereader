#ifndef LS_ENTRYBADGE_H
#define LS_ENTRYBADGE_H
// EntryBadge.h 1.0.0 (2026-11-09, Rassul 16:34 CT "Ok build"; mockup v2 https://claude.ai/artifact/8yUygzBF5C8SEx8toBGaQw; study
// https://claude.ai/artifact/2Fsf96svMpHxTgLT8kk1Zc) -- the ENTRY / EXIT liquidity tags lsDealerProfile / lsLimitProfile put on its OWN
// columns (no new column, nothing on the price area): a tag on a level row of the Key Levels column (PB POC, gamma, key) or on a book row of
// the Liquidity column. Inputs: lsBreakout's drawn decided pullback labels (fact kind 'E', EntryFacts.h), the chart's closed bars, the levels
// the profile knows (key / volume + MenthorQ gamma), the native top book (kind 'G') and lsBookFlow's per-candle defense (kind 'D').
// RULES (per market, from the step-2 study; only the NEWEST label counts, a newer label replaces the older tags):
//   entry0 = the close of the label's decision bar; R = |entry0 - stop|; the label needs R > 0 and its target beyond entry0.
//   ES  ENT at the PB POC of a Pdoc                            "ENT Pdoc"       (+0.26R vs +0.15R at the close, n 73)
//   NG  ENT at the PB POC of a Pdoc                            "ENT Pdoc ?"     (n 29, learning)
//   CL  ENT at the gamma level between the stop and entry0 (nearest the stop side rule of the study: the highest below entry0 for a long,
//       the lowest above it for a short) after Pb / Pboc / Pdoc   "ENT ?"  (+0.22R, n 36, learning)
//       EXIT ? at the first real book level (offers above a long / bids below a short) >= 0.5R beyond the ENT price, after the ENT filled
//   GC  scale-out from the label close: "1/2" on the first gamma / key level >= 0.5R beyond entry0, "rest" on the next one more than
//       0.25 ATR beyond it (else the wave target)             (+0.06R vs -0.17R trailing, n 279)
//   NQ, HG, 6E: nothing.
//   An ENT level must sit beyond the stop by more than a tick and at least 0.1 ATR from entry0 (the study's candidate rule).
//   PULL: once price traded at the ENT level (a bar's low <= ENT + 1 tick for a long), a candle whose decided defense is "defenders leaving"
//         (lsBookFlow D code 2 = bids leaving for a long, 4 = offers leaving for a short) -> " PULL" appended, kept.
// LIFECYCLE (closed bars after the decision bar, deterministic, no repaint of a decision):
//   ENT shows from the decision bar until: the stop trades (gone), the target trades before the fill (gone), 20 bars pass unfilled (gone),
//   or after the fill the stop or the target trades (resolved, gone). EXIT shows only after the ENT fill and goes when it trades or the
//   trade resolves. GC: both "1/2" and "rest" from the decision bar; "1/2" goes when it trades, "rest" when it trades; the stop ends both;
//   40 bars max. SDK-free.
// 1.1.0 (2026-11-09, Rassul 17:28 CT "Ok", B-20261109-LQ1): the per-market ON / ? / OFF of each badge comes from the nightly
//   lsFlexLevels\LRA-EntryBadge-<MKT>.csv (lra.liq_learn: "status,ENT_PDOC,ON", "status,ENT_GAMMA,?", "status,EXIT_BOOK,OFF",
//   "status,SCALE,ON"; header "# ... <YYYY-MM-DDTHH:MM:SS> ..."). Missing / unreadable / no valid status line / header time older than
//   8 days or in the future = the compiled defaults below (the 1.0.0 rules). A garbled value keeps that badge's default. "?" adds " ?"
//   to the text. Any market can turn on (NQ / HG / 6E too). ENT path: Pdoc PB POC first (ENT_PDOC), else gamma (ENT_GAMMA); EXIT_BOOK
//   after any ENT fill; SCALE only when both ENT badges are OFF. The plugin snapshots the params when a NEW label arrives, so a
//   nightly change never repaints a decided tag. pull_x / quiet_* lines are logged by the nightly only (PULL stays lsBookFlow's
//   decided "defenders leaving").
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <string>
#include <vector>
namespace ebd {
static const char* const VERSION = "1.1.0";
enum { ST_UNSET = -1, ST_OFF = 0, ST_Q = 1, ST_ON = 2 };
static const long long PAR_MAX_AGE = 8LL * 86400;
struct Par { int entPdoc = ST_OFF, entGamma = ST_OFF, exitBook = ST_OFF, scale = ST_OFF; bool fromFile = false; long long built = 0; std::string why = "defaults"; };
static const int ENT_WAIT = 20, LIFE_BARS = 40;
struct Bar { long long t = 0; double h = 0, l = 0, c = 0; };          // t = Central wall-clock seconds of the bar (same clock as the facts)
struct Lbl { long long td = 0; int side = 0, code = 0; double pbPoc = 0, stop = 0, target = 0; };
struct Lvl { double px = 0; std::string name; bool gamma = false; };
struct Book { double px = 0; long long lots = 0; bool bid = true; };
struct Tag { double px = 0; std::string name, text; int side = 0; int kind = 0; bool book = false; };   // kind 1 ENT, 2 EXIT, 3 1/2, 4 rest
struct Out { std::vector<Tag> tags; std::string why = "none"; long long td = 0; int lab = 0; bool filled = false, pull = false; };
inline std::string labName(int code) { const int c = code % 10; return c == 3 ? "Pdoc" : c == 2 ? "Pboc" : "Pb"; }
inline std::string root(const std::string& m) { std::string r; for (char ch : m) if (ch != ' ') r += (char)std::toupper((unsigned char)ch); if (r == "6E" || r == "EC") r = "EU"; return r; }
inline long long civilDays(int y, int m, int d) { y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153u * (unsigned)(m + (m > 2 ? -3 : 9)) + 2u) / 5u + (unsigned)d - 1u; const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy; return era * 146097 + (long long)doe - 719468; }
inline Par defaults(const std::string& mkt) {
    Par p; const std::string m = root(mkt);
    if (m == "ES") p.entPdoc = ST_ON; else if (m == "NG") p.entPdoc = ST_Q;
    else if (m == "CL") { p.entGamma = ST_Q; p.exitBook = ST_Q; } else if (m == "GC") p.scale = ST_ON;
    return p;
}
inline int stOf(const std::string& v) { return v == "ON" ? ST_ON : v == "?" ? ST_Q : v == "OFF" ? ST_OFF : ST_UNSET; }
// the nightly file's text -> params; anything wrong = the defaults (with why). nowSec = Central wall-clock seconds (same clock as the header)
inline Par parse(const std::string& mkt, const std::string& text, long long nowSec) {
    Par d = defaults(mkt); if (text.empty()) { d.why = "no file - defaults"; return d; }
    if (text.size() > 65536) { d.why = "file too big - defaults"; return d; }
    Par p = d; int good = 0; long long built = 0; size_t a = 0;
    while (a < text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        std::string ln = text.substr(a, e - a); a = e + 1;
        while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
        if (ln.empty()) continue;
        if (ln[0] == '#') { for (size_t k = 0; built == 0 && k + 19 <= ln.size(); ++k) {
                const char* c = ln.c_str() + k; bool ok = true; const char* pat = "dddd-dd-ddTdd:dd:dd";
                for (int j = 0; j < 19 && ok; ++j) ok = pat[j] == 'd' ? (c[j] >= '0' && c[j] <= '9') : c[j] == pat[j];
                if (!ok) continue;
                auto n = [&](int o, int w) { int v = 0; for (int j = 0; j < w; ++j) v = v * 10 + (c[o + j] - '0'); return v; };
                const int Y = n(0, 4), M = n(5, 2), D = n(8, 2), h = n(11, 2), mi = n(14, 2), se = n(17, 2);
                if (M >= 1 && M <= 12 && D >= 1 && D <= 31 && h < 24 && mi < 60 && se < 61) built = civilDays(Y, M, D) * 86400 + h * 3600 + mi * 60 + se; }
            continue; }
        if (ln.compare(0, 7, "status,") != 0) continue;
        const size_t c2 = ln.find(',', 7); if (c2 == std::string::npos) continue;
        const std::string b = ln.substr(7, c2 - 7); const int v = stOf(ln.substr(c2 + 1)); if (v == ST_UNSET) continue;
        if (b == "ENT_PDOC") p.entPdoc = v; else if (b == "ENT_GAMMA") p.entGamma = v; else if (b == "EXIT_BOOK") p.exitBook = v; else if (b == "SCALE") p.scale = v; else continue;
        ++good;
    }
    if (!good) { d.why = "no valid status line - defaults"; return d; }
    if (built <= 0) { d.why = "no time in the header - defaults"; return d; }
    if (nowSec > 0 && (built > nowSec + 3600 || nowSec - built > PAR_MAX_AGE)) { d.why = "stale (older than 8 days) - defaults"; d.built = built; return d; }
    p.fromFile = true; p.built = built; p.why = "file"; return p;
}
inline std::string qs(int st) { return st == ST_Q ? " ?" : ""; }
inline std::string parText(const Par& p) { auto t = [](int s) { return s == ST_ON ? "ON" : s == ST_Q ? "?" : "OFF"; };
    return std::string("ENT_PDOC ") + t(p.entPdoc) + " ENT_GAMMA " + t(p.entGamma) + " EXIT_BOOK " + t(p.exitBook) + " SCALE " + t(p.scale); }
// pull(barT, side) = true when the decided defense of that candle is "defenders leaving" for this side
template <class Pull>
inline Out compute(const std::string& mkt, const std::vector<Bar>& bars, const std::vector<Lbl>& labels, const std::vector<Lvl>& lv,
                   const std::vector<Book>& book, double tick, double atr, Pull pull, const Par& P)
{
    Out o; const std::string m = root(mkt);
    const bool entOn = P.entPdoc != ST_OFF || P.entGamma != ST_OFF;
    if (!entOn && P.scale == ST_OFF) { o.why = "market has no badge ON (" + P.why + ")"; return o; }
    if (labels.empty() || bars.size() < 2 || !(tick > 0) || !(atr > 0) || !std::isfinite(atr)) { o.why = "no label / bars / atr"; return o; }
    const Lbl* L = &labels.front(); for (const Lbl& x : labels) if (x.td > L->td) L = &x;
    o.td = L->td; o.lab = L->code;
    size_t i = bars.size(); for (size_t k = 0; k < bars.size(); ++k) if (bars[k].t == L->td) { i = k; break; }
    if (i >= bars.size()) { o.why = "decision bar not on the chart"; return o; }
    const int d = L->side > 0 ? 1 : -1; const double e0 = bars[i].c; const double R = (e0 - L->stop) * d;
    if (!(R > 0) || !((L->target - e0) * d > 0)) { o.why = "no risk / target"; return o; }
    auto okEnt = [&](double p) { return std::isfinite(p) && p > 0 && (p - L->stop) * d > tick && (e0 - p) * d >= 0.1 * atr; };
    auto stopHit = [&](const Bar& b) { return d > 0 ? b.l <= L->stop : b.h >= L->stop; };
    auto reach = [&](const Bar& b, double p) { return d > 0 ? b.h >= p : b.l <= p; };          // a price beyond in the trade's direction
    auto touch = [&](const Bar& b, double p) { return d > 0 ? b.l <= p : b.h >= p; };          // back to a price against the trade
    if (!entOn) {   // SCALE (GC by default)
        std::vector<double> t; for (const Lvl& x : lv) if (std::isfinite(x.px) && (x.px - e0) * d >= 0.5 * R) t.push_back(x.px);
        std::sort(t.begin(), t.end(), [&](double a, double b) { return (a - e0) * d < (b - e0) * d; });
        if (t.empty()) { o.why = "no target level"; return o; }
        const double t1 = t[0]; double t2 = L->target; for (double x : t) if ((x - t1) * d > 0.25 * atr) { t2 = x; break; }
        if ((t2 - t1) * d <= 0) t2 = t1 + d * atr;
        bool h1 = false, h2 = false;
        for (size_t k = i + 1; k < bars.size(); ++k) {
            if ((int)(k - i) > LIFE_BARS || stopHit(bars[k])) { o.why = "ended"; return o; }
            if (!h1 && reach(bars[k], t1)) h1 = true;
            if (reach(bars[k], t2)) { h2 = true; break; }
        }
        if (h2) { o.why = "rest target traded"; return o; }
        auto nm = [&](double p) { for (const Lvl& x : lv) if (std::fabs(x.px - p) < 0.5 * tick) return x.name; return std::string(); };
        if (!h1) o.tags.push_back(Tag{ t1, nm(t1), "1/2" + qs(P.scale), d, 3, false });
        o.tags.push_back(Tag{ t2, nm(t2), "rest" + qs(P.scale), d, 4, false });
        o.why = "ok"; return o;
    }
    double ent = 0; std::string nm, txt;
    if (P.entPdoc != ST_OFF && L->code % 10 == 3 && okEnt(L->pbPoc)) {
        ent = L->pbPoc; nm = "PbPOC"; txt = "ENT Pdoc" + qs(P.entPdoc);
    } else if (P.entGamma != ST_OFF) {   // the gamma level between the stop and entry0, nearest entry0
        bool any = false; for (const Lvl& x : lv) if (x.gamma && okEnt(x.px) && (!any || (x.px - ent) * d > 0)) { ent = x.px; nm = x.name; any = true; }
        if (!any) { o.why = "no gamma entry"; return o; }
        txt = "ENT" + qs(P.entGamma);
    } else { o.why = "no Pdoc PB POC entry"; return o; }
    bool filled = false; size_t fk = 0;
    for (size_t k = i + 1; k < bars.size(); ++k) {
        const Bar& b = bars[k];
        if (!filled) {
            if (touch(b, ent)) { filled = true; fk = k; if (stopHit(b)) { o.why = "stopped"; return o; } continue; }
            if (reach(b, L->target)) { o.why = "target before fill"; return o; }
            if ((int)(k - i) >= ENT_WAIT) { o.why = "not filled in 20 bars"; return o; }
        } else if (stopHit(b) || reach(b, L->target)) { o.why = "resolved"; return o; }
    }
    // PULL: a candle at / after the touch whose defense was "defenders leaving" (decided, from lsBookFlow)
    for (size_t k = i + 1; k < bars.size(); ++k) if (touch(bars[k], ent + d * tick) && pull(bars[k].t, d)) { o.pull = true; break; }
    o.filled = filled;
    o.tags.push_back(Tag{ ent, nm, txt + (o.pull ? " PULL" : ""), d, 1, false });
    if (P.exitBook != ST_OFF && filled) {
        const double r2 = (ent - L->stop) * d; const Book* best = nullptr;
        for (const Book& q : book) { if (!(q.lots > 0) || q.bid == (d > 0) || !((q.px - ent) * d >= 0.5 * r2)) continue;
            if (!best || (q.px - ent) * d < (best->px - ent) * d) best = &q; }
        bool gone = false; if (best) for (size_t k = fk; k < bars.size(); ++k) if (reach(bars[k], best->px)) { gone = true; break; }
        if (best && !gone) o.tags.push_back(Tag{ best->px, std::string(), "EXIT" + qs(P.exitBook), d, 2, true });
    }
    o.why = "ok"; return o;
}
// 1.0.0 call form (the compiled defaults) - kept for the existing tests / callers
template <class Pull>
inline Out compute(const std::string& mkt, const std::vector<Bar>& bars, const std::vector<Lbl>& labels, const std::vector<Lvl>& lv,
                   const std::vector<Book>& book, double tick, double atr, Pull pull)
{ return compute(mkt, bars, labels, lv, book, tick, atr, pull, defaults(mkt)); }
}  // namespace ebd
#endif
