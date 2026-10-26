// DeltaProfileSettings.h  (lsDeltaProfile 2.23.0: + Draw late A (not tested); 2.22.0: wick rows hidden (wick A retired); 2.21.0: + Trapped traders; 2.20.0: + Wick absorption on / Wick attack x normal; was 2.18.2, 2026-10-18 settings audit)  -  the ONE table of the Delta Profile settings window.
// File: lsFlexLevels\DeltaProfile.settings-<MKT>.txt (keyed, per MARKET - the old file was keyed per contract root, so every roll
// (EPZ26 -> EPH27) silently fell back to defaults). Migrates once from DeltaProfile.settings.txt ("<root>|0|width|gap|font|...").
#pragma once
#include "SettingsSpec.h"

namespace dpset {

static const char* const PLUGIN_VERSION = "2.23.2";
enum K { K_WIDTH, K_FONT, K_DELTA_TEXT, K_WICK_ON, K_WICK_X, K_TRAP_ON, K_DRAW_LATE, K_N };

inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        // the column is drawn at least 110 px wide (2.11.2): the window used to offer 40-109, which did nothing
        ss::intRow("width", "Width px", 110, 110, 250, 50),
        ss::same(ss::intRow("font", "Font pt", 10, 6, 18, 40)),   // (2.18.3, his decision) default 10 (was 9)
        // (2.14.0) the row values are no longer drawn; this box only reserved room for them -> hidden, value kept (audit: removed)
        ss::hide(ss::boolRow("delta_text", "Show delta text", true)),
        // (2.20.0, MBO Roadmap Top Defender #1) the wick absorption A: footprint candidate (attack in the wick vs the half-hour normal)
        // confirmed by the order book (Aggression / lsBidAskStats on the same chart)
        // (2.22.0) the wick A is retired (replaced by the shared A, SharedAbsorb.h): both rows hidden, values kept in the file
        ss::hide(ss::boolRow("wick_on", "Wick absorption (order book)", true)),
        ss::hide(ss::floatRow("wick_x", "Wick attack x normal", 3.0, 1.0, 20.0, 40)),
        // (2.21.0, Rassul approved 2026-10-21 11:43) trapped traders on the profile: Tr? / A|Tr? beside the outlined rows
        ss::boolRow("trap_on", "Trapped traders (Tr)", true),
        // (2.23.0, 2026-10-25) the A drawn is the researched A (decided at its candle's or the next candle's close); an A the old path finds
        // later is logged (code AL, its delay) and drawn only when this is ticked - it has no test behind it, so off by default
        ss::boolRow("draw_late", "Draw late A (not tested)", false),
    };
    static const ss::Spec S = { "DeltaProfile", PLUGIN_VERSION, 17, R, K_N, 0 };   // (2.23.0) parameter version 17: + Draw late A; (2.22.0) parameter version 16: wick rows hidden; (2.21.0) parameter version 15: + trap row; (2.20.0) 14: + wick rows
    return S;
}

// contract root -> market, the same rule as DeltaProfile.cpp marketName() (EPZ26 ES, NQ NQ, CLEX26 CL, QGC GC, CPEZ26 HG, NGEX26 NG, EU6Z26 EU)
inline int rootMarket(const std::string& rootIn) {
    std::string r = rootIn.substr(0, rootIn.find('~'));
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return ss::M_NQ;
    if (has("GC")) return ss::M_GC;
    if (has("CL") || r == "QM") return ss::M_CL;
    if (has("HG") || has("CP")) return ss::M_HG;
    if (has("NG") || r == "QG") return ss::M_NG;
    if (has("EU") || has("6E") || has("E6")) return ss::M_EU;
    if (has("ES") || has("EP")) return ss::M_ES;
    return -1;
}

// DeltaProfile.settings.txt: one line per contract root "<root>|0|width|gap|font|...[|text|c28]"; the first line of this market wins
// (a "~3 Minutes*~" per-chart line is used only when no plain root line exists). Pre-2.8 width 50 was the old default -> default.
inline bool legacy(const ss::Spec& S, int mkt, const std::string& text, ss::Values& V) {
    std::vector<std::string> lines = ss::split(text, '\n');
    const std::string* pick = nullptr; const std::string* chartPick = nullptr;
    for (const std::string& l : lines) {
        if (l.size() > 400 || l.find('|') == std::string::npos) continue;
        const std::string root = l.substr(0, l.find('|'));
        if (rootMarket(root) != mkt) continue;
        if (root.find('~') != std::string::npos) continue;   // per-chart keys were never matched by 2.8 - 2.18.0 (they read root == chart root)
        if (!V.hint.empty() && root == V.hint) { pick = &l; break; }   // the chart's own root: exactly what 2.18.0 read
        if (!pick) pick = &l;
    }
    (void)chartPick;
    if (!pick) return false;
    std::string ln = *pick; while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
    const std::vector<std::string> t = ss::split(ln, '|');
    if (t.size() < 5) return false;
    const bool v28 = t.size() >= 17 && t[16] == "c28";
    char* e = nullptr;
    const long w = std::strtol(t[2].c_str(), &e, 10);
    if (e && !*e && !t[2].empty()) {
        if (!v28 && w == 50) V.v[K_WIDTH] = S.rows[K_WIDTH].def;
        else V.v[K_WIDTH] = w < 110 ? 110 : (w > 250 ? 250 : w);    // drawn the same as before (never under 110)
    }
    std::vector<const char*> order; order.push_back(""); order.push_back(""); order.push_back(""); order.push_back(""); order.push_back("font");
    ss::fromPositional(S, order, t, V);
    if (v28) V.v[K_DELTA_TEXT] = t[15] != "0" ? 1 : 0;
    if (V.v[K_FONT] == 9) V.v[K_FONT] = 10;                         // (2.18.3) 9 was the old default (never his choice): the new default 10
    return true;
}
// (2.18.3) a keyed file written by 2.18.1 / 2.18.2 holds the old default font 9 (written at the migration): read as the new default 10
inline void upgrade(const std::string& fileVersion, ss::Values& V) {
    if ((fileVersion == "2.18.1" || fileVersion == "2.18.2") && V.v[K_FONT] == 9) V.v[K_FONT] = 10;
}

} // namespace dpset
