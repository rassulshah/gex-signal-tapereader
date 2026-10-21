// dialog_guard_test.cpp - SettingsGuard 1.1.0 (2026-10-21, Rassul 09:55 "you have had constant problems with allowing the user to change
// settings . fix this issue"; msglog 09:51 lsSessionVWAP parmsLoad on chart "???"). The settings window round trip every indicator must
// pass, on the IRT-behaviour mock: open (filled), change, OK, re-open (shows the change), the chart reads it, a second chart of another
// market keeps its own; and the cases that broke it: a window copy with NO chart (opens filled on the last market, OK on the real chart
// saves only the change, Apply with no chart saves nothing), a faulting symbol call, a blank copy. Both ways a plugin reads its market
// (getRootSymbol in the class = SessionVWAP / HodLod style; ss::safeMarket = KeyLevels / BigBidAsk style).
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. -I.. dialog_guard_test.cpp -o /tmp/dialog_guard_test
#include "mock_irtsdk.h"
#include "../SettingsGuard.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::cout << "FAIL line " << __LINE__ << ": " << m << "\n"; } } while (0)

namespace pr {
enum K { K_SHOW, K_COLOR, K_WIDTH, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = { ss::boolRow("show", "Show", true), ss::colorRow("color", "Colour", 0xFF80FF), ss::listRow("width", "Width", 1, "1 px;2 px;3 px", 60) };
    static const ss::Spec S = { "Probe", "1.0.0", 1, R, K_N, 0 };
    return S;
}
}
static std::string flex() { return ss::flexDirGuard(); }
static std::string fileOf(const std::string& m) { return flex() + "Probe.settings-" + m + ".txt"; }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }

// a plugin as most of his are written: the market from getRootSymbol inside the class (shadowed by SS_SAFE_DIALOG)
template <bool SAFE_MARKET> class Probe : public cppExtension {
public:
    SS_SAFE_DIALOG(Probe, SAFE_MARKET ? "ProbeB" : "ProbeA")
    ss::Ids ids; ss::Snap snap;
    void setup() { ss::registerAll(this, pr::spec(), ids); }
    std::string market() {
        if (SAFE_MARKET) return ss::safeMarket(this);
        char b[16] = {0}; const char* r = getRootSymbol(b); const int m = r ? ss::marketOfRoot(r) : -1; return m < 0 ? std::string() : ss::MARKETS[m];
    }
    ss::Store store(const std::string& m) { ss::Store s; s.S = &pr::spec(); s.path = m.empty() ? std::string() : fileOf(m); return s; }
    int parmsLoadBody(void) {
        if (!ss::inDialog(this)) return RTX_OK;
        const std::string m = market(); if (m.empty()) return RTX_OK;      // (old behaviour: an unknown market = an unfilled window)
        ss::Store st = store(m); ss::fillDialog(this, pr::spec(), ids, st.load(ss::marketIndex(m), 0, true), snap); return RTX_OK;
    }
    int parmsApplyBody(void) {
        if (!ss::inDialog(this)) return RTX_OK;
        const std::string m = market(); if (m.empty()) return RTX_OK;
        ss::Store st = store(m); const ss::Values V = ss::readDialog(this, pr::spec(), ids, st.load(ss::marketIndex(m), 0, true), snap);
        if (st.save(V)) snap.take(ss::dialogNow(this, pr::spec(), ids, V));
        return RTX_OK;
    }
    int parmsUpdtBody(unsigned int i) { ss::onUpdate(this, pr::spec(), ids, (int)i, snap); return RTX_OK; }
    ss::Values chartReads(const std::string& m) { ss::Store st = store(m); return st.load(ss::marketIndex(m), 0, true); }
};

template <bool B> static void run(const char* tag) {
    const std::string T = std::string("[") + tag + "] ";
    std::remove(fileOf("ES").c_str()); std::remove(fileOf("NQ").c_str());
    ss::lastDialogMarket().clear(); std::remove(ss::lastMarketFile(B ? "ProbeB" : "ProbeA").c_str());
    Probe<B> w; w.setup(); w.m.dialog = true; w.m.root = "ES";
    // 1 the ordinary round trip
    w.parmsLoad();
    CHECK(w.isBoxChecked(w.ids.id[pr::K_SHOW]) && w.getParameterColor(w.ids.id[pr::K_COLOR]) == 0xFF80FF, T + "opens filled with the defaults");
    w.checkBox(w.ids.id[pr::K_SHOW], false); w.parmsUpdt((unsigned)w.ids.id[pr::K_SHOW]); w.parmsApply();
    CHECK(!w.chartReads("ES").b(pr::K_SHOW), T + "OK saves 'Show' off for ES; the chart reads it");
    Probe<B> r; r.setup(); r.m.dialog = true; r.m.root = "ES"; r.parmsLoad();
    CHECK(!r.isBoxChecked(r.ids.id[pr::K_SHOW]) && r.getParameterColor(r.ids.id[pr::K_COLOR]) == 0xFF80FF, T + "re-opened: shows the saved choice");
    Probe<B> n; n.setup(); n.m.dialog = true; n.m.root = "NQ"; n.parmsLoad();
    CHECK(n.isBoxChecked(n.ids.id[pr::K_SHOW]), T + "an NQ chart's window keeps NQ's own (defaults)");
    // 2 the window copy with NO chart (IRT "???"): opens FILLED on the last market (ES), not blank
    r.parmsLoad();                                                        // his last window was the ES chart's
    Probe<B> c; c.setup(); c.m.dialog = true; c.m.root = ""; c.parmsLoad();
    CHECK(!c.isBoxChecked(c.ids.id[pr::K_SHOW]) && c.getParameterColor(c.ids.id[pr::K_COLOR]) == 0xFF80FF && c.getListIndex(c.ids.id[pr::K_WIDTH]) == 1,
          T + "a chart-less window opens filled from the last market (ES), never blank / black");
    c.setParameterColor(c.ids.id[pr::K_COLOR], 0x00FF00);
    const std::string es0 = slurp(fileOf("ES"));
    c.parmsApply();
    CHECK(slurp(fileOf("ES")) == es0 && slurp(fileOf("NQ")).empty(), T + "Apply with still no chart saves nothing (never into a guessed market)");
    c.m.root = "ES"; c.parmsApply();
    const ss::Values es = w.chartReads("ES");
    CHECK((es.c(pr::K_COLOR) & 0xFFFFFF) == 0x00FF00 && !es.b(pr::K_SHOW), T + "OK on the real chart saves the change (green) and keeps 'Show' off");
    // 3 a symbol call that faults: no crash, same as no chart
    Probe<B> f; f.setup(); f.m.dialog = true; f.m.symFault = true; f.parmsLoad();
    CHECK((f.getParameterColor(f.ids.id[pr::K_COLOR]) & 0xFFFFFF) == 0x00FF00, T + "a faulting symbol call: the window still opens filled (last market)");
    // 4 the chart copy never saves (IRT hands it blank controls)
    Probe<B> k; k.setup(); k.m.dialog = false; k.m.root = "ES"; const std::string es1 = slurp(fileOf("ES")); k.parmsApply(); k.parmsLoad();
    CHECK(slurp(fileOf("ES")) == es1, T + "the chart copy (blank controls) never saves");
    // 5 the guess survives an IRT restart (file), and a chart-less copy of a plugin never used falls back to ES
    ss::lastDialogMarket().clear(); n.parmsApply();                         // NQ window used last
    ss::lastDialogMarket().clear();
    CHECK(ss::guessDialogMarket(B ? "ProbeB" : "ProbeA") == "NQ", T + "the last market is remembered across a restart (file)");
    CHECK(ss::guessDialogMarket("NeverUsed") == "ES", T + "nothing remembered: ES");
    // 6 every window call is logged
    const std::string log = slurp(flex() + "Settings-dialog.log");
    CHECK(log.find(std::string(B ? "ProbeB" : "ProbeA") + ",load,no chart, opened on ES") != std::string::npos && log.find(",apply,no chart (not saved)") != std::string::npos
          && log.find("change,chart ES") != std::string::npos, T + "the dialog log names chart / no chart / guessed market per call");
}

int main() {
    char tmpl[] = "/tmp/dgtXXXXXX"; const char* home = mkdtemp(tmpl); setenv("USERPROFILE", home, 1);
    const std::string d = std::string(home) + "/InvestorRT"; mkdir(d.c_str(), 0755); mkdir((d + "/rtx").c_str(), 0755); mkdir((d + "/rtx/lsFlexLevels").c_str(), 0755);
    run<false>("getRootSymbol in the class");
    run<true>("ss::safeMarket");
    std::cout << "dialog guard: " << pass << " passed, " << fail << " failed\n";
    return fail ? 1 : 0;
}
