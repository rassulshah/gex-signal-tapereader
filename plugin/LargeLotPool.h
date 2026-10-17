// LargeLotPool.h - the "Large Lot Methodology" for AGGRESSOR orders (BidAskStats L), 2026-10-17 (Rassul: "yes do it").
// (2026-10-17) Big Lot S20 / S10 / S5: tiers t[0..2] = sizes at or above which ~20 / 10 / 5 orders occur per session (rank over the pooled sessions).
// Same method as the LMT pool (BigOrderLines.h): log-bucket histogram of order sizes (>= 10 lots), pool = orders at or above
// mean + 2 SD, tiers 1SD / 2SD / 3SD = ranks 84.13 / 97.72 / 99.87 % of that pool. 5 sessions (4 past + the current one).
// Channels: 0 = all hours, 1 = RTH (8:30-15:00 CT), 2 = overnight. A channel needs MIN_N pool orders, else all hours, else none
// (the caller then keeps its old percentile rule). File lines are the LMT format: BLD1,<session>,<48 counts ;-separated>.
#pragma once
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
namespace llp {
struct LogDist {
    static const int NB = 48;
    long long c[NB];
    int nSess = 0;                      // sessions pooled (set by the caller): > 0 + automatic start = Big Lot S20 / S10 / S5
    LogDist() { clear(); }
    void clear() { for (int i = 0; i < NB; ++i) c[i] = 0; }
    static int bucket(long long s) { if (s < 10) return -1; const int b = (int)std::floor(std::log((double)s / 10.0) / std::log(1.15)); return b < 0 ? 0 : b >= NB ? NB - 1 : b; }
    static double centre(int b) { return 10.0 * std::pow(1.15, b + 0.5); }
    void addN(long long s, long long n) { const int b = bucket(s); if (b >= 0 && n > 0) c[b] += n; }
    void add(const LogDist& o) { for (int i = 0; i < NB; ++i) c[i] += o.c[i]; }
    long long total() const { long long t = 0; for (int i = 0; i < NB; ++i) t += c[i]; return t; }
    std::string line(long long session) const {
        std::string o = "BLD1," + std::to_string(session) + ",";
        for (int i = 0; i < NB; ++i) { if (i) o += ';'; o += std::to_string(c[i]); }
        return o;
    }
    static bool num(const std::string& t, long long& out) {
        if (t.empty() || t.size() > 15) return false;
        for (size_t k = 0; k < t.size(); ++k) if (t[k] < '0' || t[k] > '9') return false;
        out = std::atoll(t.c_str()); return true;
    }
    static bool parse(const std::string& ln, long long& session, LogDist& d) {
        if (ln.compare(0, 5, "BLD1,") != 0) return false;
        const size_t p2 = ln.find(',', 5); if (p2 == std::string::npos) return false;
        long long sv = 0; if (!num(ln.substr(5, p2 - 5), sv)) return false;
        LogDist t; size_t a = p2 + 1; int i = 0;
        while (i < NB) {
            size_t e = ln.find(';', a); if (e == std::string::npos) e = ln.size();
            long long v = 0; if (!num(ln.substr(a, e - a), v)) return false;
            t.c[i++] = v; if (e >= ln.size()) break; a = e + 1;
        }
        if (i != NB) return false;
        session = sv; d = t; return true;
    }
    // start <= 0 = automatic = mean + 2 SD of all recorded orders (bucket centres); pool = at / above start; tiers by rank, log-interpolated in a bucket
    bool rankStats(long long minN, long long& n, double& mu, double v[3], double* startOut) const {
        if (nSess <= 0) return false;
        double tot = 0; for (int i = 0; i < NB; ++i) tot += (double)c[i];
        const double R[3] = { 20.0, 10.0, 5.0 };
        if (tot < R[0] * nSess || (long long)(R[0] * nSess) < minN) return false;
        for (int k = 0; k < 3; ++k) {
            const double target = R[k] * nSess; double above = 0; v[k] = 0;
            for (int i = NB - 1; i >= 0; --i) {
                if (c[i] <= 0) continue;
                if (above + (double)c[i] >= target) { v[k] = 10.0 * std::pow(1.15, (double)(i + 1) - (target - above) / (double)c[i]); break; }
                above += (double)c[i];
            }
        }
        for (int k = 1; k < 3; ++k) if (v[k] < v[k - 1]) v[k] = v[k - 1];
        double sw = 0, s1 = 0;
        for (int i = 0; i < NB; ++i) { if (c[i] <= 0 || centre(i) < v[0]) continue; sw += (double)c[i]; s1 += c[i] * std::log(centre(i)); }
        mu = sw > 0 ? std::exp(s1 / sw) : v[0]; n = (long long)(R[0] * nSess); if (startOut) *startOut = v[0];
        return true;
    }
    bool stats(long long start, long long minN, long long& n, double& mu, double v[3], double* startOut = nullptr) const {
        if (start <= 0 && nSess > 0) return rankStats(minN, n, mu, v, startOut);   // Big Lot S20 / S10 / S5
        double st = (double)start;
        if (start <= 0) {
            double w = 0, a = 0, b = 0;
            for (int i = 0; i < NB; ++i) { if (c[i] <= 0) continue; const double x = centre(i); w += (double)c[i]; a += c[i] * x; b += c[i] * x * x; }
            if (w < 2) return false;
            const double m = a / w, sd = std::sqrt(std::max(0.0, b / w - m * m)); st = m + 2.0 * sd;
        }
        if (startOut) *startOut = st;
        double sw = 0, s1 = 0;
        for (int i = 0; i < NB; ++i) { if (c[i] <= 0 || centre(i) < st) continue; sw += (double)c[i]; s1 += c[i] * std::log(centre(i)); }
        n = (long long)sw; if (n < minN || n < 2) return false;
        mu = std::exp(s1 / sw);
        static const double P[3] = { 0.8413, 0.9772, 0.9987 };
        for (int k = 0; k < 3; ++k) {
            const double target = P[k] * sw; double cum = 0; v[k] = 0;
            for (int i = 0; i < NB; ++i) {
                if (c[i] <= 0 || centre(i) < st) continue;
                if (cum + (double)c[i] >= target) { const double f = (target - cum) / (double)c[i]; v[k] = 10.0 * std::pow(1.15, (double)i + f); break; }
                cum += (double)c[i];
            }
        }
        return true;
    }
};
static const int CH_ALL = 0, CH_RTH = 1, CH_ON = 2, NCH = 3, KEEP_PAST = 4;
static const long long MIN_N = 30;
// 8:30-15:00 CT = RTH, anything else overnight; 0 = unknown clock
inline int channelOf(int hour, int minute) { const int m = hour * 60 + minute; return (m >= 510 && m < 900) ? CH_RTH : CH_ON; }
struct Tiers { bool ok = false; int channel = CH_ALL; long long n = 0; double mu = 0, start = 0; long long t[3] = { 0, 0, 0 }; };
struct Pool {
    long long session = 0;                              // the current session
    LogDist cur[NCH];                                   // the current session, per channel
    std::map<long long, LogDist> past[NCH];             // finished sessions (latest KEEP_PAST kept)
    bool dirty = false;
    void clear() { session = 0; for (int k = 0; k < NCH; ++k) { cur[k].clear(); past[k].clear(); } dirty = false; }
    void add(int ch, long long size) { if (size < 10) return; cur[CH_ALL].addN(size, 1); if (ch == CH_RTH || ch == CH_ON) cur[ch].addN(size, 1); dirty = true; }
    void trim(int k) { while ((int)past[k].size() > KEEP_PAST) past[k].erase(past[k].begin()); }
    // the session ended: the current counts become a past session
    void roll() { if (session > 0) for (int k = 0; k < NCH; ++k) { if (cur[k].total() > 0) past[k][session] = cur[k]; trim(k); } for (int k = 0; k < NCH; ++k) cur[k].clear(); dirty = true; }
    void setSession(long long s) { session = s; }
    // file lines of one channel: the past sessions and the current one
    std::string text(int k) const {
        std::string o;
        for (std::map<long long, LogDist>::const_iterator i = past[k].begin(); i != past[k].end(); ++i) o += i->second.line(i->first) + "\n";
        if (session > 0 && cur[k].total() > 0) o += cur[k].line(session) + "\n";
        return o;
    }
    // loaded lines: session == the current one -> the current counts, older -> past (only if the session has no counts yet), newer ignored
    void feed(int k, const std::vector<std::string>& lines, bool seed) {
        for (size_t i = 0; i < lines.size(); ++i) {
            long long s = 0; LogDist d; if (!LogDist::parse(lines[i], s, d)) continue;
            if (session > 0 && s == session) { if (!seed && cur[k].total() == 0) cur[k] = d; }
            else if (session <= 0 || s < session) { if (!past[k].count(s)) past[k][s] = d; }
        }
        trim(k);
    }
    LogDist sum(int k) const { LogDist t = cur[k]; for (std::map<long long, LogDist>::const_iterator i = past[k].begin(); i != past[k].end(); ++i) t.add(i->second); return t; }
    int sessionsIn(int k) const { return (int)past[k].size() + (cur[k].total() > 0 ? 1 : 0); }
    Tiers tiersFor(int ch) const {
        Tiers r; double v[3]; long long n = 0; double mu = 0, st = 0;
        const int order[2] = { ch == CH_RTH || ch == CH_ON ? ch : CH_ALL, CH_ALL };
        for (int j = 0; j < 2; ++j) {
            if (j == 1 && order[0] == CH_ALL) break;
            LogDist pool; if (past[order[j]].size() >= 2) { for (std::map<long long, LogDist>::const_iterator i = past[order[j]].begin(); i != past[order[j]].end(); ++i) pool.add(i->second); pool.nSess = (int)past[order[j]].size(); }   // finished sessions only: stable all day
            else { pool = sum(order[j]); pool.nSess = sessionsIn(order[j]); }
            if (pool.stats(0, MIN_N, n, mu, v, &st)) {
                r.ok = true; r.channel = order[j]; r.n = n; r.mu = mu; r.start = st;
                for (int k = 0; k < 3; ++k) r.t[k] = (long long)std::ceil(v[k]);
                for (int k = 1; k < 3; ++k) if (r.t[k] < r.t[k - 1]) r.t[k] = r.t[k - 1];
                return r;
            }
        }
        return r;
    }
};
} // namespace llp
