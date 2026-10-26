// TradeManagerSettings.h  (lsTradeManager 1.3.1, 2026-10-18 settings audit)  -  the ONE table of the Trade Manager settings window.
// Same 3 rows in the same order (parameter version 2 kept). File: lsFlexLevels\TradeManager.settings-<MKT>.txt (keyed; market / font /
// position); the Position is also still written to TradeManager.place-<MKT>.txt, which the drag / placement code reads.
// Fixed: Font was read only while the settings window was open (the chart drew 8 pt whatever he chose) - the chart now reads the file.
#pragma once
#include "SettingsSpec.h"

namespace tmset {
static const char* const PLUGIN_VERSION = "1.3.3";
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";
enum K { K_MARKET, K_FONT, K_POSITION, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::listRow("market", "Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU", 70),
        ss::same(ss::intRow("font", "Font size (pt)", 10, 6, 20, 40)),   // (1.3.2, his decision) default 10 (was 8)
        ss::listRow("position", "Position", 2, POS_CHOICES, 110),
    };
    static const ss::Spec S = { "TradeManager", PLUGIN_VERSION, 2, R, K_N, 0 };
    return S;
}
// the 1.2 / 1.3.0 place file: "N" (the Position 0-8)
inline bool legacy(const ss::Spec& S, int, const std::string& text, ss::Values& V) {
    std::string t = text.substr(0, text.find('\n')); while (!t.empty() && (t.back() == '\r' || t.back() == ' ')) t.pop_back();
    if (t.empty() || t.size() > 2) return false;
    std::vector<std::string> f(1, t); std::vector<const char*> order(1, "position");
    ss::fromPositional(S, order, f, V); return true;
}
// (1.3.2) a keyed file written by 1.3.1 holds the old default font 8 (written at the migration): read as the new default 10
inline void upgrade(const std::string& fileVersion, ss::Values& V) {
    if (fileVersion == "1.3.1" && V.v[K_FONT] == 8) V.v[K_FONT] = 10;
}
} // namespace tmset
