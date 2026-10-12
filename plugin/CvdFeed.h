// CvdFeed.h -- lsCVD 1.1.0: closed chart bars -> cvd::Engine (CvdEngine.h). Pure C++17, no SDK; the plugin and the real-data study both use it.
//
// What it decides (tests: tests_cvd/test_cvd_host.cpp, test_cvd_review.cpp):
//   Anchor     the Globex session (17:00 CT). A new engine starts at each session; the first session on the chart is skipped
//              (it may start mid-session, and it has no range history). CVD is never shown for a partial session.
//   Volume     buy / sell = the bar's volume-at-price statistics (ask-traded / bid-traded), total = the chart's bar volume.
//              IRT's volume-at-price can run a few contracts over the bar volume (late prints): up to 2% + 1 the classified total
//              is used; more than that, or a failed read, is a bad bar (quality=false) - the engine then shows no CVD for the
//              rest of that session rather than a silently wrong one.
//   Levels     his trade levels, causal: PDH/PDL, PFH/PFL (prior session), ONH/ONL (from the RTH open), LonHI/LonLO (from 07:30),
//              plus MenthorQ CR / PS / HVL for the newest session. Same price to the tick = one level. Each level is given to the
//              engine as support AND resistance (reversal from either side, breakout through either side).
//   Scale      the package's tick settings were written for an ES-like bar. Markets differ 20x (median 3-min RTH bar: NQ 83 ticks,
//              ES 15, CL 19, GC 34, HG 9, NG 6, 6E 4 - Sep/Oct 2026). Each tick setting is rescaled per session by
//              R = that market's median RTH bar range (the table below, measured on his IRT 3-min bars; other bar sizes scale by
//              sqrt(seconds / 180)):  ticks = max(1, round(default x R / 15)). ES on 3 minutes runs the package defaults unchanged.
//              A fixed table (not the chart's own recent range) so a session's marks depend only on that session and the one
//              before it - a reload or a shorter chart can never change a mark (BUILD-LESSONS #11).
//   Sessions   the first session on the chart is skipped (it may start mid-way); the next shows CVD only (its levels would come
//              from a partial session); from the third on: CVD and marks.
//   Shown      (1.1.0, Rassul 2026-10-12 'simplified and improved') the engine still runs every read and the log keeps them all, but
//              the chart shows only a CONFIRMED Div / Exh / BR that (a) is at a prior-day or prior-session level (PDH/PDL/PFH/PFL)
//              and (b) has its stop within 4.0x the median bar range of the 20 bars before it. Chosen on the first half of
//              Aug-Oct 2026 (7 markets) and held on the second: +0.17R / +0.38R, 88 signals +0.27R, 6 of 7 markets positive; everything
//              else -0.06R. Why: overnight / London levels are made in thin trade (few defend them), and a wide stop means the move had
//              already left the level before it confirmed (a late entry). Abs is off (7 warnings in 3 months, none confirmed).
//   Ranges     the per-market bar range can be refreshed nightly (lra.cvd_nightly -> lsFlexLevels\LRA-CVD-ranges.csv, rows
//              MKT,first session it applies to,ticks). A row only applies from its own session on, so a reload never changes an
//              earlier session's marks.
#pragma once
#include "CvdEngine.h"
#include "KeyLevelsLogic.h"
#include "SessionVWAPLogic.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace cvdf {

static const double REF_RANGE_TICKS = 15.0;    // ES 3-min RTH median bar range (Sep-Oct 2026): the bar the package defaults fit
// median 3-min RTH bar range in ticks, Sep 1 - Oct 9 2026 (lra 3-min store, irt_fp bars; study/README.md)
inline double rangeTable(const std::string& m) {
    return m == "ES" ? 15 : m == "NQ" ? 83 : m == "CL" ? 19 : m == "GC" ? 34 : m == "HG" ? 9 : m == "NG" ? 6 : m == "EU" ? 4 : NAN;
}
static const double SHOW_MAX_RISK = 4.0;       // stop distance / median bar range of the prior 20 bars
static const int RISK_BARS = 20;
inline bool shownLevel(const std::string& id) {    // the level part of a level id: "PDH/ONH@31399|R"
    const std::string n = id.substr(0, id.find('@'));
    return n.find("PDH") != std::string::npos || n.find("PDL") != std::string::npos || n.find("PFH") != std::string::npos || n.find("PFL") != std::string::npos;
}
static const size_t HIST_MAX = 7000;           // ~15 sessions of 3-min bars (PWH/PWL need the prior week)

struct In {                                    // one CLOSED chart bar
    long long raw = 0;                         // the host's bar time (bar END)
    long days = 0; int endSec = 0;             // civil (Central) day and second of the bar END
    double o = 0, h = 0, l = 0, c = 0;
    double buy = NAN, sell = NAN, total = NAN; // NaN = not read
};
struct Out { double cvd = NAN, delta = NAN, pct = NAN; bool tainted = false, ready = false, counted = false; };
struct Mark { long long raw = 0; cvd::Event e; double risk = NAN; bool shown = false; };
struct Lvl { std::string code; double px = 0; };

inline cvd::Config scaled(double rangeTicks, double tick) {
    cvd::Config c; c.tick_size = tick;
    const double k = rangeTicks / REF_RANGE_TICKS;
    auto s = [k](double d) { return std::max(1.0, std::round(d * k)); };
    c.level_tolerance_ticks = s(3); c.price_extension_ticks = s(1); c.invalidate_ticks = s(2); c.confirm_ticks = s(1);
    c.rearm_ticks = s(1); c.stall_progress_ticks = s(3); c.stall_range_ticks = s(8);
    return c;
}

class Feed {
public:
    std::string market; double tick = 0; int spb = 0, openMin = -1, closeMin = -1;
    std::vector<Lvl> mq; long mqSess = -1;     // MenthorQ gamma-1 levels and the session they belong to
    // results, keyed by bar time
    std::map<long long, Out> out;
    std::vector<Mark> marks;
    std::string state = "WAITING FOR A FULL SESSION";
    double rangeTicks = NAN; cvd::Config cfg; long sess = -1; int badBars = 0, overTotal = 0; std::string lastError;
    std::vector<std::pair<long, double> > rangeRows;   // nightly refresh: (first session it applies to, ticks), sorted
    int shownCount = 0;

    bool init(const std::string& mkt, double tk, int perSec) {
        market = mkt; tick = tk; spb = perSec;
        return tk > 0 && perSec > 0 && svl::rthOf(mkt.c_str(), openMin, closeMin);
    }
    // returns the events this bar produced (also appended to marks)
    std::vector<cvd::Event> push(const In& x) {
        std::vector<cvd::Event> ev;
        const svl::Cls cls = svl::classify(x.days, x.endSec, spb, openMin, closeMin);
        klv::Bar kb; kb.days = x.days; kb.endSec = x.endSec; kb.h = (float)x.h; kb.l = (float)x.l; kb.c = (float)x.c;
        hist_.push_back(kb); kinds_.push_back(cls.kind); sessOf_.push_back(cls.sess);
        const size_t cap = std::max(HIST_MAX, (size_t)(2.2 * 82800.0 / spb) + 100);   // always two whole sessions (the prior session's levels)
        if (hist_.size() > cap) { const size_t cut = hist_.size() - cap; hist_.erase(hist_.begin(), hist_.begin() + cut); kinds_.erase(kinds_.begin(), kinds_.begin() + cut); sessOf_.erase(sessOf_.begin(), sessOf_.begin() + cut); }
        Out o;
        if (cls.kind == svl::NONE) { out[x.raw] = o; return ev; }
        if (cls.sess != sess) {                                       // a new anchor
            const bool complete = sess >= 0;                           // we saw the session before it, so this bar is its first
            const bool priorComplete = complete && sessComplete_;      // ... and that one was complete too: its levels are real
            sess = cls.sess; sessComplete_ = complete; engine_.reset(); seq_ = 0; levelKey_.clear(); firstSeen_.clear(); levels_.clear();
            if (complete) {
                double base = rangeTable(market);
                for (size_t k = 0; k < rangeRows.size(); ++k) if (rangeRows[k].first <= sess && rangeRows[k].second > 0) base = rangeRows[k].second;
                rangeTicks = base * std::sqrt(spb / 180.0);
                cfg = scaled(std::isfinite(rangeTicks) ? rangeTicks : REF_RANGE_TICKS, tick);
                cfg.enable_abs = false;                                // 1.1.0: never confirmed in 3 months - off
                if (!priorComplete) cfg.enable_div = cfg.enable_abs = cfg.enable_exh = cfg.enable_br = false;
                try { engine_.reset(new cvd::Engine(cfg)); } catch (...) { engine_.reset(); }
                state = priorComplete ? "READY" : "CVD ONLY - marks start next session (needs one full session before)";
            }
        }
        if (!engine_) { out[x.raw] = o; return ev; }
        refreshLevels(cls);
        cvd::Bar b; b.seq = seq_; b.anchor = label(sess, 0).substr(0, 10); b.timestamp = label(x.days, x.endSec);
        b.open = x.o; b.high = x.h; b.low = x.l; b.close = x.c; b.seconds = spb;
        b.buy = x.buy; b.sell = x.sell; b.total = x.total; b.quality = true;
        const bool read = std::isfinite(x.buy) && std::isfinite(x.sell) && std::isfinite(x.total) && x.buy >= 0 && x.sell >= 0 && x.total >= 0;
        if (!read) { b.buy = b.sell = b.total = 0; b.quality = false; }
        else if (b.buy + b.sell > b.total) {
            if (b.buy + b.sell <= b.total * 1.02 + 1.0) { b.total = b.buy + b.sell; ++overTotal; }
            else { b.buy = b.sell = b.total = 0; b.quality = false; }
        }
        if (!b.quality) ++badBars;
        if (!(std::isfinite(b.open) && std::isfinite(b.high) && std::isfinite(b.low) && std::isfinite(b.close) && b.high >= b.low &&
              b.high >= std::max(b.open, b.close) && b.low <= std::min(b.open, b.close))) {
            b.open = b.high = b.low = b.close = std::isfinite(x.c) ? x.c : 0; b.quality = false; ++badBars;
        }
        for (const cvd::Level& l : levels_) b.levels.push_back(l);
        cvd::Output r;
        try { r = engine_->push(b); }
        catch (const std::exception& ex) { lastError = ex.what(); engine_.reset(); state = "STOPPED - bad bar data, restarts next session"; out[x.raw] = o; return ev; }
        ++seq_;
        o.cvd = r.cvd; o.delta = r.delta; o.pct = r.delta_pct; o.tainted = r.tainted; o.ready = r.ready; o.counted = true;
        out[x.raw] = o;
        if (r.tainted) state = "NO CVD - missing order-flow data this session";
        for (const cvd::Event& e : r.events) {
            Mark mk; mk.raw = x.raw; mk.e = e;
            if (e.state == cvd::State::Confirmed) {
                mk.risk = priorMedianRange() > 0 ? std::fabs(b.close - e.invalidation) / priorMedianRange() : NAN;
                mk.shown = shownLevel(e.level_id) && std::isfinite(mk.risk) && mk.risk <= SHOW_MAX_RISK + 1e-9;
                if (mk.shown) ++shownCount;
            }
            marks.push_back(mk); ev.push_back(e);
        }
        return ev;
    }
    const std::vector<cvd::Level>& levels() const { return levels_; }
    long long ticksOf(double px) const { return std::llround(px / tick); }

private:
    std::unique_ptr<cvd::Engine> engine_;
    std::int64_t seq_ = 0; bool sessComplete_ = false;
    std::vector<klv::Bar> hist_; std::vector<int> kinds_; std::vector<long> sessOf_;
    std::string levelKey_; std::map<std::string, std::int64_t> firstSeen_; std::vector<cvd::Level> levels_;

    // the median high-low of the RISK_BARS chart bars before this one (hist_ ends with this bar)
    double priorMedianRange() const {
        if (hist_.size() < 2) return NAN;
        const size_t end = hist_.size() - 1, from = end > (size_t)RISK_BARS ? end - RISK_BARS : 0;
        std::vector<double> r; for (size_t i = from; i < end; ++i) r.push_back((double)hist_[i].h - (double)hist_[i].l);
        std::sort(r.begin(), r.end()); const size_t n = r.size();
        return n % 2 ? r[n / 2] : (r[n / 2 - 1] + r[n / 2]) / 2;
    }
    static std::string label(long days, int endSec) {
        int y = 0; unsigned m = 0, d = 0; long z = days + 719468; const long era = (z >= 0 ? z : z - 146096) / 146097;
        const unsigned doe = (unsigned)(z - era * 146097), yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        y = (int)yoe + (int)era * 400; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100), mp = (5 * doy + 2) / 153;
        d = doy - (153 * mp + 2) / 5 + 1; m = mp + (mp < 10 ? 3 : -9); y += (m <= 2);
        char b[32]; std::snprintf(b, sizeof(b), "%04d-%02u-%02uT%02d:%02d:%02d", y, m, d, endSec / 3600, (endSec / 60) % 60, endSec % 60);
        return b;
    }
    // the levels change only at a new session, the RTH open and 07:30 - recompute then (hist_ ends with this bar; see the note above)
    void refreshLevels(const svl::Cls& cls) {
        const bool rth = cls.kind == svl::RTH || cls.kind == svl::POST;
        const int endMin = hist_.back().endSec / 60;
        const bool lon = rth || (hist_.back().endSec < 17 * 3600 && endMin > 7 * 60 + 30);
        const std::string key = std::to_string(sess) + (rth ? "R" : "-") + (lon ? "L" : "-") + "|" + std::to_string(mqSess == sess ? (long)mq.size() : -1L);
        if (key == levelKey_) return;
        levelKey_ = key;
        std::vector<klv::Level> L;
        for (const klv::Level& k : klv::compute(hist_, spb, openMin, closeMin, tick)) {
            const std::string& c = k.code;
            if (c == "PDH" || c == "PDL" || c == "PFH" || c == "PFL" || c == "ONH" || c == "ONL" || c == "LonHI" || c == "LonLO") L.push_back(k);
        }
        if (mqSess == sess) for (const Lvl& m : mq) L.push_back(klv::Level{m.code, std::round(m.px / tick) * tick, false});
        levels_.clear();
        for (const klv::Merged& m : klv::merge(L, tick)) {
            const std::string id = m.name + "@" + std::to_string(ticksOf(m.price));
            std::map<std::string, std::int64_t>::iterator f = firstSeen_.find(id);
            if (f == firstSeen_.end()) f = firstSeen_.insert(std::make_pair(id, seq_ > 0 ? seq_ - 1 : 0)).first;   // known at the previous close
            levels_.push_back(cvd::Level{id + "|S", m.price, true, f->second});
            levels_.push_back(cvd::Level{id + "|R", m.price, false, f->second});
        }
    }
};

} // namespace cvdf
