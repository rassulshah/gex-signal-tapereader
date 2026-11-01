// test_exhaustionv2_host.cpp -- lsExhaustionV2 (2.0.1) on the mock host. g++ -std=c++17 -Imock -I. test_exhaustionv2_host.cpp -o th && ./th
// The full real-data check (172 events, 7 markets, Python == this plugin) lives in tests_exhaustion_v2/ (host_replay.cpp + py_side.py).
#include "ExhaustionV2.cpp"
#include <cstdio>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static mock::Bar bar(unsigned long t, double o, double h, double l, double c, long b, long s) {
    mock::Bar x; x.time = t; x.open = (float)o; x.high = (float)h; x.low = (float)l; x.close = (float)c; x.volume = (unsigned long)(b + s);
    x.rows.push_back(mock::Price((float)c, b, s, b + s)); return x; }
int main() {
    char tmpl[] = "/tmp/exhXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ6"; h.root = "ES";
    // 3 sessions of quiet 3-minute bars, 17:00 - 16:00 CT (bar END times), session dates 10/07..10/09
    const long long d0 = civilDays(2026, 10, 6) * 86400LL;
    std::vector<mock::Bar> all;
    for (int s = 0; s < 3; ++s) for (int k = 1; k <= 460; ++k) {
        const unsigned long t = (unsigned long)(d0 + s * 86400LL + 17 * 3600 + k * 180); const double p = 7000 + 0.25 * ((k % 8) < 4 ? k % 4 : 4 - k % 4);
        all.push_back(bar(t, p, p + 0.5, p - 0.5, p + 0.25, 300, 280)); }
    {   // no calibration file: lines are gaps, no marks, state CALIBRATING
        for (auto& b : all) h.bars.push_back(b); h.bars.push_back(bar(all.back().time + 180, 7000, 7020, 6990, 7015, 5, 1)); h.now = h.bars.back().time;
        Exhaustion* p = static_cast<Exhaustion*>(CreateExtension()); p->calc(0);
        CHECK(p->stateText() == "CALIBRATING", "no calibration: CALIBRATING, never a guessed baseline");
        h.draws.clear(); p->draw(); int marks = 0; bool head = false;
        for (auto& d : h.draws) { if (d.type == "text" && (d.text == "E?" || d.text == "R?" || d.text == "EXIT?")) ++marks; if (d.type == "text" && d.text == "EXHAUSTION   ") head = true; }
        CHECK(head && marks == 0, "header drawn, no marks without calibration");
        p->destroy(); h.data = nullptr;
    }
    {   // a calibration (cutoff 10/07): refs for every slot -> the later sessions are READY with finite lines
        std::ofstream c((flex + "\\LRA-ExhaustionV2-ES-ESZ6.csv").c_str());
        c << "SCHEMA,EXH2\nMARKET,ES\nCONTRACT,ESZ6\nBAR_SECONDS,180\nTIMEZONE,America/Chicago\nTICK,0.25\nCUTOFF_SESSION,2026-10-07\nMODEL_ID,test0000000000000000\nSTATUS,research_only\nMIN_BASELINE_SESSIONS,5\nRTH_OPEN,510\nRTH_CLOSE,900\nELIGIBILITY,rth\n";
        const char* params = "n,10;smooth,2;look,20;fade_threshold,2.0;min_peak_norm,1.5;min_current_norm,0.2;max_current_norm,1.0;min_total_norm,0.5;min_coverage,0.9;min_peak_age,1;min_leg_atr,0.75;min_rejection,0.2;max_extension_atr,0.35;rearm_bars,3;rearm_fraction,0.7;confirmation_bars,3;confirm_close_fraction,0.5;confirm_opposing_share,0.5;require_confirmation,1;mode,pullback;pullback_n,3;trend_ema,20;trend_slope_bars,3;trend_min_slope_atr,0.05";
        std::stringstream ps(params); std::string kv; while (std::getline(ps, kv, ';')) c << "PARAM," << kv << "\n";
        for (int slot = 0; slot < 1440; slot += 3) c << "BASE," << slot << ",300,280,580,20\n";
        c << "END,EXH2\n"; c.close();
        Exhaustion* p = static_cast<Exhaustion*>(CreateExtension()); p->calc(0);
        const std::string st = p->stateText();
        CHECK(st.compare(0, 5, "READY") == 0 || st == "BUILDING BAR CONTEXT", "calibrated: READY after the cutoff session");
        h.draws.clear(); p->draw(); bool val = false;
        for (auto& d : h.draws) if (d.type == "text" && d.text.size() > 1 && d.text.back() == 'x' && d.text != "2.0x" && d.text.find('.') != std::string::npos) val = true;
        CHECK(val, "header shows the last closed bar's fades as ratios (x.x x)");
        // the forming bar (a huge new high on 6 contracts) is never read: one more calc changes nothing
        const std::string before = p->stateText(); p->calc(0); CHECK(p->stateText() == before, "the bar in progress is not fed");
        // IRT trims the oldest bars: the frontier is found again by time, no fault
        h.bars.erase(h.bars.begin(), h.bars.begin() + 50); p->calc(0);
        CHECK(p->stateText().find("RELOAD") == std::string::npos && p->stateText().find("CHANGED") == std::string::npos, "oldest bars trimmed: resynced by bar time");
        p->destroy(); h.data = nullptr;
    }
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
