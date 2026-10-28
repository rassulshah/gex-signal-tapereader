#ifndef LS_COMBINERLOGIC_H
#define LS_COMBINERLOGIC_H
// CombinerLogic.h 0.2.0 (2026-10-28 overnight, Rassul 21:52 CT "go"): STATUS per outcome - the TURN and the SWING odds each SHOW or LOG on
// their own (STATUS,turn|swing,SHOW|LOG), a per-market LOG override (STATUS,swing,LOG,ES), the swing odds drawn with "?" from SHOW,swing,<p>
// (round 2: swing beat its best single input and the base on untuned days, turn did not; ES beat its base on neither -> ES log only)
// CombinerLogic.h 0.1.0 (2026-10-28, Rassul 20:14 / 20:18 CT approved "the combiner" - layer 3 of ChainSpec.h) -- ONE read with odds at every
// closed pivot-reversal candle (a 6-candle low that closes green in its upper half; high mirror): the chance of a TURN (+1 ATR before the
// candle's extreme breaks by 0.25 ATR) and of a SWING (+2.5 ATR first), from a logistic model whose weights come from the nightly fit
// (lsFlexLevels\LRA-Combiner.csv). Inputs, all decided at the candle's close (ChainSpec: lower layers / decided facts only):
//   A on the candle (READ 'R'), node in the wick, any big node (Delta 'N' / the record), candle volume vs the 20 before (log), body share,
//   counter- / with-trend (20-candle move >= 1 ATR), momentum into the candle (5), late session (RTH last hour / post-close), London, RTH
//   open hour, volatility (ATR vs the market's median, log), key level within 0.5 ATR of the extreme ('V'), MenthorQ gamma level likewise
//   ('V' code >= 1000), a big LMT F fill on the candle ('P' tier >= 1), the candle's stop distance in ATR.
// Research (scratchpad lmtf/combiner_fit.py, 7 markets, MBO archive 10 Sep - 22 Oct, built on 65% of the days, checked on the last 35%):
//   turn  AUC 0.591, Brier 0.2220 vs base 0.2269 vs the best single input (volume) 0.2233; calibrated (0.73 -> 0.72, 0.84 -> 0.82)
//   swing AUC 0.604, Brier 0.2366 vs base 0.2441 vs best single 0.2406; every market beat its base rate on both
//   his 2R trade with the pivot stop: no edge (AUC 0.51) -> R is LOGGED only
// SDK-free; the host feeds plain numbers.
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <fstream>
#include <sstream>
namespace cmb {
static const char* const VERSION = "0.2.0";
static const char* const FEATURES[] = { "A", "wick", "node", "logvol", "body", "counter", "withtr", "mom", "late", "london", "open_hr", "atr_rel", "key",
                                        "gamma", "bigfill", "risk_atr" };
static const int NF = 16;
struct Model { bool ok = false; double b0 = 0; double w[NF] = {0}; };
struct Params {
    bool ok = false; std::string why = "no params file";
    Model turn, swing; double showTurn = 0.72; bool showOn = true; std::map<std::string, double> atrMed;
    double showSwing = 0.55; bool swingOn = false; std::map<std::string, int> logMkt;   // (0.2.0) swing status + threshold; market -> 1 turn LOG / 2 swing LOG (bits)
};
// the 2026-10-28 fit (used when the nightly file is missing, said so in the status)
inline Params defaults() {
    Params p; p.ok = true; p.why = "built-in 2026-10-28 fit (no LRA-Combiner.csv yet)";
    const double t[NF] = {0.143928,0.299766,-0.158273,0.167725,-0.105257,-0.043220,0.092090,-0.052738,-0.216290,0.062476,0.129617,0.005380,-0.163855,-0.038633,0.084395,0.272454};
    const double s[NF] = {0.131542,0.280436,-0.059885,0.113790,-0.047911,-0.079396,0.038363,-0.032157,-0.484091,0.182017,0.203169,-0.209117,-0.106393,-0.085738,-0.155312,0.217842};
    p.turn.ok = p.swing.ok = true; p.turn.b0 = 0.421265; p.swing.b0 = -0.523747;
    for (int i = 0; i < NF; ++i) { p.turn.w[i] = t[i]; p.swing.w[i] = s[i]; }
    p.atrMed = { {"ES", 2.625}, {"NQ", 17.225}, {"CL", 0.1675}, {"GC", 3.205}, {"HG", 0.00495}, {"NG", 0.0037}, {"EU", 0.0001925} };
    return p;
}
inline std::vector<std::string> split(const std::string& s, char d) { std::vector<std::string> o; std::string c; std::stringstream ss(s); while (std::getline(ss, c, d)) o.push_back(c); return o; }
inline bool num(const std::string& s, double& v) { char* e = nullptr; v = std::strtod(s.c_str(), &e); return e && *e == 0 && !s.empty() && std::isfinite(v); }
// LRA-Combiner.csv: VERSION,1 / FEATURES,<names> / MODEL,turn|swing,<b0>,<w...> / ATR_MED,<MKT>,<v> / SHOW,turn,<p> / STATUS,turn,SHOW|LOG
inline Params parse(const std::string& text) {
    Params p = defaults(); p.ok = false; p.why = "bad params file";
    bool ver = false, feat = false; int models = 0;
    std::stringstream ss(text); std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> f = split(line, ',');
        if (f.empty()) continue;
        if (f[0] == "VERSION") ver = f.size() > 1 && f[1] == "1";
        else if (f[0] == "FEATURES") { feat = (int)f.size() == NF + 1; for (int i = 0; i < NF && feat; ++i) if (f[i + 1] != FEATURES[i]) feat = false; }
        else if (f[0] == "MODEL" && (int)f.size() == NF + 3) {
            Model m; m.ok = num(f[2], m.b0);
            for (int i = 0; i < NF && m.ok; ++i) m.ok = num(f[3 + i], m.w[i]);
            if (!m.ok) continue;
            if (f[1] == "turn") { p.turn = m; ++models; } else if (f[1] == "swing") { p.swing = m; ++models; }
        }
        else if (f[0] == "ATR_MED" && f.size() == 3) { double v; if (num(f[2], v) && v > 0) p.atrMed[f[1]] = v; }
        else if (f[0] == "SHOW" && f.size() == 3 && f[1] == "turn") { double v; if (num(f[2], v) && v > 0.5 && v < 1) p.showTurn = v; }
        else if (f[0] == "STATUS" && f.size() == 3 && f[1] == "turn") p.showOn = f[2] == "SHOW";
        else if (f[0] == "STATUS" && f.size() == 3 && f[1] == "swing") p.swingOn = f[2] == "SHOW";                       // (0.2.0)
        else if (f[0] == "STATUS" && f.size() == 4 && (f[1] == "turn" || f[1] == "swing") && f[2] == "LOG") p.logMkt[f[3]] |= f[1] == "turn" ? 1 : 2;   // (0.2.0) per market
        else if (f[0] == "SHOW" && f.size() == 3 && f[1] == "swing") { double v; if (num(f[2], v) && v > 0.2 && v < 1) p.showSwing = v; }   // (0.2.0)
    }
    if (!ver || !feat || models < 2) return p;
    p.ok = true; p.why = "nightly params"; return p;
}
inline Params load(const std::string& path) {
    std::ifstream in(path.c_str()); if (!in) return defaults();
    std::stringstream b; b << in.rdbuf(); Params p = parse(b.str());
    if (!p.ok) { Params d = defaults(); d.why = "params file rejected (" + p.why + "): built-in fit"; return d; }
    return p;
}
struct Candle { double o = 0, h = 0, l = 0, c = 0, v = 0; int mod = 0; };   // mod = Central minutes since midnight of the candle's start
struct Inputs { bool A = false, wick = false, node = false, key = false, gamma = false, bigfill = false; };
struct Read { bool pivot = false, bull = false; double turn = 0, swing = 0; double f[NF] = {0}; bool show = false, showSwing = false; };   // (0.2.0) showSwing = the swing odds drawn with "?"
// a pivot-reversal candle: the 6-candle extreme that closes back in its own direction in its own upper / lower half
inline bool pivotShape(const std::vector<Candle>& c, int k, bool bull) {
    if (k < 5 || k >= (int)c.size()) return false;
    const double eps = 1e-9 * std::max(1.0, std::fabs(c[k].c));   // prices are tick multiples carried as doubles: equal means equal
    for (int i = k - 5; i < k; ++i) if (bull ? c[i].l < c[k].l - eps : c[i].h > c[k].h + eps) return false;
    const double mid = (c[k].h + c[k].l) / 2;
    return bull ? (c[k].c > c[k].o + eps && c[k].c >= mid - eps) : (c[k].c < c[k].o - eps && c[k].c <= mid + eps);
}
inline double atrAt(const std::vector<Candle>& c, int k, int n = 20) {   // the mean true range of the n candles BEFORE k
    double s = 0; int m = 0;
    for (int i = std::max(1, k - n); i < k; ++i) { const double tr = std::max(c[i].h - c[i].l, std::max(std::fabs(c[i].h - c[i - 1].c), std::fabs(c[i].l - c[i - 1].c))); s += tr; ++m; }
    return m >= 10 ? s / m : 0;
}
inline double sigm(double z) { return 1.0 / (1.0 + std::exp(-z)); }
// rthFirst / rthLast: the market's RTH in Central minutes; tick for the stop distance (1 tick beyond the extreme)
inline Read score(const Params& P, const std::vector<Candle>& c, int k, bool bull, const Inputs& in, const std::string& mkt, int rthFirst, int rthLast, double tick) {
    Read r; r.bull = bull; r.pivot = pivotShape(c, k, bull);
    if (!r.pivot || k < 21) return r;
    const double at = atrAt(c, k); if (!(at > 0)) { r.pivot = false; return r; }
    double vs = 0; for (int i = k - 20; i < k; ++i) vs += c[i].v; vs /= 20;
    const double dir = bull ? 1 : -1;
    const double trend = (c[k].c - c[k - 20].c) / at * dir, mom = (c[k - 1].c - c[k - 6].c) / at * dir;
    const double range = c[k].h - c[k].l;
    const int m = c[k].mod; const bool rth = m >= rthFirst && m < rthLast;
    const std::map<std::string, double>::const_iterator am = P.atrMed.find(mkt);
    const double atrRel = am != P.atrMed.end() && am->second > 0 ? at / am->second : 1.0;
    const double stop = bull ? c[k].l - tick : c[k].h + tick;
    double* f = r.f;
    f[0] = in.A; f[1] = in.wick; f[2] = in.node;
    f[3] = std::log(std::min(10.0, std::max(0.1, vs > 0 ? c[k].v / vs : 1.0)));
    f[4] = range > 0 ? std::fabs(c[k].c - c[k].o) / range : 0;
    f[5] = trend < -1; f[6] = trend > 1; f[7] = std::max(-5.0, std::min(5.0, mom));
    f[8] = (rth && m >= rthLast - 60) || (!rth && m >= rthLast && m < 17 * 60);
    f[9] = !rth && m >= 120 && m < rthFirst; f[10] = rth && m < rthFirst + 60;
    f[11] = std::log(std::min(5.0, std::max(0.2, atrRel))); f[12] = in.key; f[13] = in.gamma; f[14] = in.bigfill;
    f[15] = std::min(5.0, std::max(0.0, std::fabs(c[k].c - stop) / at));
    double zt = P.turn.b0, zs = P.swing.b0;
    for (int i = 0; i < NF; ++i) { zt += P.turn.w[i] * f[i]; zs += P.swing.w[i] * f[i]; }
    r.turn = sigm(zt); r.swing = sigm(zs);
    const std::map<std::string, int>::const_iterator lm = P.logMkt.find(mkt); const int logBits = lm == P.logMkt.end() ? 0 : lm->second;
    r.show = P.showOn && !(logBits & 1) && r.turn >= P.showTurn;
    r.showSwing = P.swingOn && !(logBits & 2) && r.swing >= P.showSwing;   // (0.2.0)
    return r;
}
}  // namespace cmb
#endif
