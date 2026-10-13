// FirstExtremeLogic.h - the First Final Extreme v2 model, native (Rassul 2026-10-12 18:51 "see review and build the indicator/signals").
//
// A line-by-line port of the reviewed research package's extreme_features.py (first_extreme_upgrade/, locked primary model):
// the same sessions (exact 3-min proxy grid, GC/HG/EU from 07:21), the same scale (mean high-low range of the 14 prior complete
// proxy sessions - NOT Wilder ATR - reset after a calendar gap > 7 days), the same VWAP / SD (volume-weighted bar typical price,
// centred on the open), the same 88 features in model/feature_order.json order. Inference = the package's
// first_extreme_inference.cpp (multinomial logistic + temperature / class-bias calibration) and first_extreme_state.hpp (the
// immutable first call). Locked policy: exact-side probability >= 0.70, one bar, no band gate, per-market deadline.
// Pure C++ (no SDK): tested against the Python features (results/causal_decisions.parquet) and the 244 locked test calls.
#pragma once
#include <algorithm>
#include <cctype>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "first_extreme_inference.cpp"
#include "first_extreme_state.hpp"

namespace fx2 {

static const char* const FX_VERSION = "1.0.0";
static const char* const FX_MODEL = "first_final_extreme_v2_research (locked primary: logistic + calibration, t0.70 p1 free)";

struct MarketCfg { const char* code; int open, close; double tick; int deadline; };
// STUDY_PLAN.json sessions (open / close CT minutes, tick) + the locked training-derived deadlines (minutes from the proxy open)
static const MarketCfg MKTS[7] = {
    { "ES", 510, 900, 0.25,    105 },
    { "NQ", 510, 900, 0.25,    87  },
    { "CL", 480, 810, 0.01,    123 },
    { "NG", 480, 810, 0.001,   117 },
    { "GC", 440, 750, 0.1,     105 },
    { "HG", 440, 750, 0.0005,  108 },
    { "EU", 440, 840, 0.00005, 117 },
};
inline int mktIndex(const std::string& m) { for (int i = 0; i < 7; i++) if (m == MKTS[i].code) return i; return -1; }
inline std::string marketForRoot(const std::string& rootIn)
{
    std::string r; for (size_t i = 0; i < rootIn.size(); i++) r += (char)toupper((unsigned char)rootIn[i]);
    auto has = [&](const char* k) { return r.find(k) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return "";
}
inline int proxyOpen(int m) { return (int)std::ceil(MKTS[m].open / 3.0) * 3; }

inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) q--; return q; }
inline long long daysFromCivil(int y, int mo, int d)
{
    y -= mo <= 2; long long era = (y >= 0 ? y : y - 399) / 400; unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1; unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
inline int weekdayMon0(long long days) { long long w = floorDiv(days + 3, 7); return (int)(days + 3 - w * 7); }   // 1970-01-01 = Thursday -> 3

struct Bar { long long tEnd; double o, h, l, c, v; };   // tEnd = local CT seconds since 1970-01-01 (bar END stamp)

static const double NaN_ = std::numeric_limits<double>::quiet_NaN();

struct Session {
    long long day = 0; bool complete = true;
    std::vector<double> o, h, l, c, v;
    double range = NaN_, scale = NaN_, prevclose = NaN_, prev_range = NaN_, prev_ret = NaN_, prev_high = NaN_, prev_low = NaN_;
    double on_high = NaN_, on_low = NaN_, on_last = NaN_; bool on_valid = false;
    double prior5high = NaN_, prior5low = NaN_, range_regime = NaN_;
    std::vector<double> volref; bool haveVolref = false;
    int duration = 0;
};

// the sessions of one market from a chronological bar list: every complete proxy session (the history, with the package's gap
// rule) and, when `today` >= 0, that day's prefix (bars so far) with the context of the complete sessions before it.
// valid prefix = the bars so far are exactly the first k bars of the regular grid, with sane OHLCV.
struct Built {
    std::vector<Session> complete;   // every complete session (each with its own context)
    bool haveToday = false; Session today; std::string todayWhy;
};

inline bool barsOk(const std::vector<const Bar*>& f)
{
    for (const Bar* b : f) {
        if (!(std::isfinite(b->o) && std::isfinite(b->h) && std::isfinite(b->l) && std::isfinite(b->c) && std::isfinite(b->v))) return false;
        if (b->v < 0 || b->h < std::max(b->o, b->c) || b->l > std::min(b->o, b->c) || b->h < b->l) return false;
    }
    return true;
}

inline Built buildSessions(int m, const std::vector<Bar>& bars, long long today = -1)
{
    Built B;
    const int proxy = proxyOpen(m), cl = MKTS[m].close, nExp = (cl - proxy) / 3;
    // RTH proxy bars grouped by calendar day (weekdays only)
    std::vector<std::pair<long long, std::vector<const Bar*>>> days;
    for (const Bar& b : bars) {
        long long d = floorDiv(b.tEnd, 86400); int mod = (int)((b.tEnd - d * 86400) / 60);
        if ((b.tEnd - d * 86400) % 60 != 0) continue;
        if (mod < proxy + 3 || mod > cl || weekdayMon0(d) >= 5) continue;
        if (days.empty() || days.back().first != d) days.push_back(std::make_pair(d, std::vector<const Bar*>()));
        days.back().second.push_back(&b);
    }
    std::vector<Session> history; const Session* previous = nullptr; Session prevCopy;
    auto context = [&](Session& s, long long d) {
        // overnight: every bar with END in (D-1 17:00, D + proxy open]
        long long a = d * 86400 - 7 * 3600, z = d * 86400 + (long long)proxy * 60;
        size_t lo_ = std::lower_bound(bars.begin(), bars.end(), a, [](const Bar& b, long long t) { return b.tEnd <= t; }) - bars.begin();
        int nOn = 0; double oh = -std::numeric_limits<double>::infinity(), ol = std::numeric_limits<double>::infinity(), last = NaN_; bool fin = true;
        for (size_t k = lo_; k < bars.size() && bars[k].tEnd <= z; k++) {
            const Bar& b = bars[k]; nOn++;
            if (!(std::isfinite(b.h) && std::isfinite(b.l) && std::isfinite(b.c))) fin = false;
            oh = std::max(oh, b.h); ol = std::min(ol, b.l); last = b.c;
        }
        s.on_valid = nOn >= 50 && fin;
        if (s.on_valid) { s.on_high = oh; s.on_low = ol; s.on_last = last; }
        s.scale = history.size() >= 14 ? 0.0 : NaN_;
        if (history.size() >= 14) { double t = 0; for (size_t k = history.size() - 14; k < history.size(); k++) t += history[k].range; s.scale = t / 14.0; }
        if (previous) {
            s.prevclose = previous->c.back(); s.prev_range = previous->range; s.prev_ret = previous->c.back() - previous->o.front();
            s.prev_high = *std::max_element(previous->h.begin(), previous->h.end()); s.prev_low = *std::min_element(previous->l.begin(), previous->l.end());
        }
        if (history.size() >= 5) {
            double ph = -std::numeric_limits<double>::infinity(), pl = std::numeric_limits<double>::infinity();
            for (size_t k = history.size() - 5; k < history.size(); k++) {
                ph = std::max(ph, *std::max_element(history[k].h.begin(), history[k].h.end()));
                pl = std::min(pl, *std::min_element(history[k].l.begin(), history[k].l.end()));
            }
            s.prior5high = ph; s.prior5low = pl;
        }
        if (history.size() >= 14) {
            s.volref.assign((size_t)nExp, 0.0);
            for (int i = 0; i < nExp; i++) {
                double t = 0;
                for (size_t k = history.size() - 14; k < history.size(); k++) { double cs = 0; for (int q = 0; q <= i; q++) cs += history[k].v[q]; t += cs; }
                s.volref[i] = t / 14.0;
            }
            s.haveVolref = true;
            if (s.scale > 0) { double t = 0; for (size_t k = history.size() - 5; k < history.size(); k++) t += history[k].range; s.range_regime = (t / 5.0) / s.scale; }
        }
        s.duration = cl - proxy;
    };
    for (auto& dd : days) {
        long long d = dd.first; const auto& f = dd.second;
        bool isToday = today >= 0 && d == today;
        if (today >= 0 && d > today) break;
        // grid check: exactly the expected END minutes (a prefix for today)
        bool grid = isToday ? (int)f.size() <= nExp : (int)f.size() == nExp;
        for (size_t k = 0; grid && k < f.size(); k++) grid = (f[k]->tEnd - d * 86400) / 60 == proxy + 3 * (long long)(k + 1);
        bool ok = grid && barsOk(f);
        if (!ok) {
            if (isToday) { B.todayWhy = "today's bars are not a clean 3-min grid from the open"; }
            continue;
        }
        if (previous && d - previous->day > 7) { history.clear(); previous = nullptr; }
        Session s; s.day = d; s.complete = !isToday;
        for (const Bar* b : f) { s.o.push_back(b->o); s.h.push_back(b->h); s.l.push_back(b->l); s.c.push_back(b->c); s.v.push_back(b->v); }
        s.range = *std::max_element(s.h.begin(), s.h.end()) - *std::min_element(s.l.begin(), s.l.end());
        context(s, d);
        if (isToday) { B.haveToday = true; B.today = s; break; }
        B.complete.push_back(s); history.push_back(s);
        prevCopy = history.back(); previous = &history.back();
    }
    (void)prevCopy;
    if (today >= 0 && !B.haveToday && B.todayWhy.empty()) B.todayWhy = "no RTH bars yet today";
    return B;
}

inline double scalarOr(double x, double def) { return std::isfinite(x) ? x : def; }

// one assessed bar
struct Decision {
    int i = 0, elapsed = 0;
    double lo = 0, hi = 0; int loForm = 0, hiForm = 0; bool gateL = false, gateH = false;
    Features x{}; Probabilities p{}; bool probOk = false;
};

// the package's running_features(): every forecast-grid bar of one session (or its prefix) with its 88 features and probabilities
inline std::vector<Decision> runSession(int m, const Session& s)
{
    std::vector<Decision> out;
    const double tick = MKTS[m].tick, scale = s.scale;
    if (!std::isfinite(scale) || scale <= tick) return out;
    const int n = (int)s.c.size(); if (n == 0) return out;
    const auto& o = s.o; const auto& h = s.h; const auto& l = s.l; const auto& c = s.c; const auto& v = s.v;
    const double opening = o[0];
    std::vector<double> vw(n), sd(n);
    double cv = 0, cvd = 0, cvd2 = 0; std::vector<double> cvs(n);
    for (int i = 0; i < n; i++) {
        double typical = (h[i] + l[i] + c[i]) / 3.0, delta = typical - opening;
        cv += v[i]; cvd += v[i] * delta; cvd2 += v[i] * delta * delta; cvs[i] = cv;
        double w = opening + (cv > 0 ? cvd / cv : 0.0);
        double var = (cv > 0 ? cvd2 / cv : 0.0) - (w - opening) * (w - opening);
        vw[i] = w; sd[i] = std::sqrt(std::max(var, 0.0));
    }
    double hi = -std::numeric_limits<double>::infinity(), lo = std::numeric_limits<double>::infinity();
    int hiidx = 0, loidx = 0, count_h = 0, count_l = 0; bool gate_h = false, gate_l = false; int gate_h_idx = -1, gate_l_idx = -1;
    int crosses = 0, last_sign = 0;
    const double wd = (double)weekdayMon0(s.day);
    for (int i = 0; i < n; i++) {
        int elapsed = (i + 1) * 3;
        if (h[i] > hi + tick / 2) { hi = h[i]; hiidx = i; count_h++; gate_h = false; gate_h_idx = -1; }
        if (l[i] < lo - tick / 2) { lo = l[i]; loidx = i; count_l++; gate_l = false; gate_l_idx = -1; }
        if (i > 0 && sd[i - 1] > 0) {
            if (loidx < i && h[i] >= vw[i - 1] + sd[i - 1] && !gate_l) { gate_l = true; gate_l_idx = i; }
            if (hiidx < i && l[i] <= vw[i - 1] - sd[i - 1] && !gate_h) { gate_h = true; gate_h_idx = i; }
        }
        double dif = c[i] - opening; int sign = dif > 0 ? 1 : (dif < 0 ? -1 : 0);
        if (sign && last_sign && sign != last_sign) crosses++;
        if (sign) last_sign = sign;
        if (elapsed < 9 || (elapsed > 180 && elapsed != 195 && elapsed != 210 && elapsed != 225 && elapsed != 240)) continue;
        const double p = c[i], rg = std::max(hi - lo, tick);
        Decision D; D.i = i; D.elapsed = elapsed; D.lo = lo; D.hi = hi; D.loForm = (loidx + 1) * 3; D.hiForm = (hiidx + 1) * 3; D.gateL = gate_l; D.gateH = gate_h;
        int k = 0; auto put = [&](double x) { D.x[k++] = x; };
        put(elapsed); put((double)elapsed / s.duration); put(rg / scale); put((p - lo) / rg); put((p - opening) / scale);
        put((p - vw[i]) / scale); put((vw[i] - vw[std::max(0, i - 10)]) / scale); put(sd[i] / scale);
        put(scalarOr((opening - s.prevclose) / scale, 0)); put(scalarOr(s.prev_ret / scale, 0)); put(scalarOr(s.prev_range / scale, 0));
        put(scalarOr(s.range_regime, 1)); put(scalarOr((s.on_high - s.on_low) / scale, 0));
        { double den = s.on_high - s.on_low; den = std::isnan(den) ? den : std::max(den, tick); put(scalarOr((opening - s.on_low) / den, 0.5)); }
        put(scalarOr((s.on_last - (s.on_high + s.on_low) / 2) / scale, 0)); put(s.on_valid ? 1.0 : 0.0); put((double)std::min(crosses, 15));
        if (s.haveVolref) { double r = cvs[i] / std::max(s.volref[i], 1.0); put(std::min(std::max(r, -20.0), 20.0)); } else put(1.0);
        put(std::sin(2 * 3.14159265358979323846 * wd / 5)); put(std::cos(2 * 3.14159265358979323846 * wd / 5));
        for (int kk : { 3, 5, 10, 20 }) { double anchor = i >= kk ? c[i - kk] : opening; put((p - anchor) / scale); }
        double sq = 0, ab = 0; { double prev = opening; for (int q = 0; q <= i; q++) { double r = c[q] - prev; sq += r * r; ab += std::fabs(r); prev = c[q]; } }
        put(std::sqrt(sq) / scale); put(std::fabs(p - opening) / std::max(ab, tick));
        for (int mins : { 15, 30, 60 }) {
            int j = mins / 3; bool done = i + 1 >= j;
            double rh = hi, rl = lo;
            if (done) { rh = *std::max_element(h.begin(), h.begin() + j); rl = *std::min_element(l.begin(), l.begin() + j); }
            put(done ? 1.0 : 0.0); put((rh - rl) / scale); put(done ? (hi - rh) / scale : 0.0); put(done ? (rl - lo) / scale : 0.0);
        }
        for (int side = 0; side < 2; side++) {
            const bool low = side == 0; const int idx = low ? loidx : hiidx; const double ex = low ? lo : hi; const double sg = low ? 1.0 : -1.0;
            const bool gate = low ? gate_l : gate_h; const int gidx = low ? gate_l_idx : gate_h_idx;
            double age = (double)(i - idx) * 3, retreat = sg * (p - ex) / scale;
            double cmax = c[idx], cmin = c[idx]; for (int q = idx; q <= i; q++) { cmax = std::max(cmax, c[q]); cmin = std::min(cmin, c[q]); }
            double swingclose = low ? (cmax - ex) / scale : (ex - cmin) / scale;
            double pathAbs = 0; { double prev = ex; for (int q = idx; q <= i; q++) { pathAbs += std::fabs(c[q] - prev); prev = c[q]; } }
            int r0 = std::max(idx, i - 4); double rmax = c[r0], rmin = c[r0]; for (int q = r0; q <= i; q++) { rmax = std::max(rmax, c[q]); rmin = std::min(rmin, c[q]); }
            double vsum = 0; for (int q = 0; q <= idx; q++) vsum += v[q];
            put(age); put((double)(idx + 1) * 3); put(retreat); put(retreat / (rg / scale)); put(sg * (opening - ex) / scale); put(swingclose);
            put(sg * (p - ex) / std::max(pathAbs, tick)); put(gate ? 1.0 : 0.0); put(gate ? (double)(i - gidx) * 3 : 0.0);
            put(sg * (p - opening) > tick / 2 ? 1.0 : 0.0);
            put(sg * (p - (low ? rmax : rmin)) / scale);
            put((double)std::min(low ? count_l : count_h, 20)); put((h[idx] - l[idx]) / scale);
            put(v[idx] / std::max(vsum / (idx + 1), 1.0));
            put(low ? (ex >= s.prev_low - tick / 2 ? 1.0 : 0.0) : (ex <= s.prev_high + tick / 2 ? 1.0 : 0.0));
            const double lvls[3] = { low ? s.on_low : s.on_high, low ? s.prev_low : s.prev_high, low ? s.prior5low : s.prior5high };
            for (int q = 0; q < 3; q++) {
                bool swept = std::isfinite(lvls[q]) && sg * (ex - lvls[q]) < -tick / 2;
                put(swept ? 1.0 : 0.0); put(swept && sg * (p - lvls[q]) > tick / 2 ? 1.0 : 0.0);
            }
        }
        put((double)(loidx - hiidx) * 3);
        static const char* const ORDER[7] = { "ES", "NQ", "CL", "NG", "GC", "HG", "EU" };
        for (int q = 0; q < 7; q++) put(std::string(ORDER[q]) == MKTS[m].code ? 1.0 : 0.0);
        D.probOk = first_extreme_probabilities(D.x, D.p);
        out.push_back(D);
    }
    return out;
}

inline FirstExtremePolicy lockedPolicy(int m)
{
    FirstExtremePolicy P; P.threshold = 0.70; P.persistence = 1; P.require_previous_band_gate = false;
    P.deadline_minutes = MKTS[m].deadline; P.tick = MKTS[m].tick; return P;
}

}  // namespace fx2
