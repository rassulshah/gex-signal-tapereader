// test_dpb.cpp -- DpBanner.h 1.0.0: the top read = the Delta Profile read (fresh / stale / broken, tiers, Y / T detail, level name).
// From tests_dpb/: g++ -std=c++17 -O1 -I.. test_dpb.cpp -o /tmp/t_dpb && /tmp/t_dpb
#include "../DpBanner.h"
#include <iostream>
static int ok = 0, bad = 0; static void C(bool c, const std::string& w) { if (c) ++ok; else { ++bad; std::cerr << "FAIL " << w << "\n"; } }
int main() {
    const long spb = 180; const double tk = 0.25; const long long t0 = 80460;   // 22:21 ct seconds of day (any day)
    auto lv = [](double p) { return p == 7765.0 ? std::string("CR0") : std::string(); };
    sb::Fact r; r.bar = t0 - 180; r.lo = r.hi = 7765.0; r.side = -1; r.val = 4.55; r.code = 3; r.proven = 2;   // A? at 22:18
    sb::Fact t; t.bar = t0 - 360; t.side = -1; t.val = 1.72;
    dpb::Banner b = dpb::build({r}, {}, {t}, t0, 7763.75, spb, tk, lv);
    C(b.fresh && b.favour == -1, "fresh bearish");
    C(b.name == "A? SELLERS ABSORBED AT 7765.00 (CR0)", "name: " + b.name);
    C(b.detail.find("buyers hit 4.5x") == 0 && b.detail.find("NEXT candle") != std::string::npos && b.detail.find("buyers trapped 1.7x ?") != std::string::npos, "detail: " + b.detail);
    b = dpb::build({r}, {}, {}, t0 + 6 * 180, 7763.0, spb, tk, lv); C(!b.fresh && b.any && b.name == "NO FRESH DELTA READ" && b.detail.find("7 candles ago") != std::string::npos, "stale: " + b.detail);
    b = dpb::build({r}, {}, {}, t0, 7766.0, spb, tk, lv); C(!b.fresh && b.detail.find("since broken") != std::string::npos, "broken: " + b.detail);
    sb::Fact p = r; p.side = 1; p.code = 4; p.proven = 2; p.lo = p.hi = 7831.5; p.val = 3.5;
    sb::Fact y; y.bar = p.bar; y.side = 1; y.code = 31; y.val = 0.08; y.proven = 2;
    b = dpb::build({p}, {y}, {}, t0, 7832.0, spb, tk, lv);
    C(b.name == "A+? BUYERS ABSORBED AT 7831.50" && b.favour == 1, "A+ name: " + b.name);
    C(b.detail.find("sellers hit 3.5x") == 0 && b.detail.find("closed green above") != std::string::npos && b.detail.find("bids holding +0.1x ?") != std::string::npos, "A+ detail: " + b.detail);
    p.proven = 1; b = dpb::build({p}, {y}, {}, t0, 7832.0, spb, tk, lv); C(b.name.compare(0, 3, "A+ ") == 0, "proven A+");
    sb::Fact ex = r; ex.code = 2; b = dpb::build({ex}, {}, {}, t0, 7763.0, spb, tk, lv); C(!b.any, "Ex code ignored");
    sb::Fact fut = r; fut.bar = t0 + 180; b = dpb::build({fut}, {}, {}, t0, 7763.0, spb, tk, lv); C(!b.any, "a forming-candle fact ignored");
    sb::Fact a1 = r; a1.code = 1; a1.bar = t0 - 540; b = dpb::build({a1, r}, {}, {}, t0, 7763.0, spb, tk, lv); C(b.name.compare(0, 3, "A? ") == 0, "newest wins");
    b = dpb::build({}, {}, {}, t0, 7763.0, spb, tk, lv); C(!b.any && !b.fresh, "none");
    std::cout << "DpBanner " << dpb::VERSION << ": " << ok << " passed, " << bad << " failed\n"; return bad ? 1 : 0;
}
