// TapeReaderSettings.h  (lsTapeReader 1.3.1, 2026-10-18 settings audit)  -  the ONE table of the Tape Reader settings window.
// File: lsFlexLevels\TapeReader.settings-<MKT>.txt (keyed). Migrates once from TapeReader.place-<MKT>.txt ("pos,dx,dy,bg").
#pragma once
#include "SettingsSpec.h"

namespace trset {

static const char* const PLUGIN_VERSION = "1.3.4";
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";
enum K { K_POSITION, K_BACKGROUND, K_N };

inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::listRow("position", "Position", 0, POS_CHOICES, 110),
        ss::boolRow("background", "Banner background", true),
    };
    static const ss::Spec S = { "TapeReader", PLUGIN_VERSION, 4, R, K_N, 0 };   // 4 (1.3.2): Offset X / Y removed;
    return S;
}

// 1.1.3 - 1.3.0 place file "pos[,dx,dy[,bg]]" (1.1.0 - 1.1.2: "pos" only)
inline bool legacy(const ss::Spec& S, int, const std::string& text, ss::Values& V) {
    std::string ln = text.substr(0, text.find('\n'));
    while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
    if (ln.empty() || ln.size() > 100) return false;
    const std::vector<std::string> f = ss::split(ln, ',');
    if (f.size() != 1 && f.size() != 3 && f.size() != 4) return false;
    std::vector<const char*> order; order.push_back("position"); order.push_back("offset_x"); order.push_back("offset_y"); order.push_back("background");
    ss::fromPositional(S, order, f, V);
    return true;
}

} // namespace trset
