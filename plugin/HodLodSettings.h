// HodLodSettings.h  (lsHodLod 2.6.0: Show pre-open line + Pre-open minutes (the PRE-OPEN BOOK row), parameter version 8; 2.4.4: Offset X / Y removed, the old files' offsets are ignored; 2.4.2, 2026-10-18 settings audit)  -  the ONE table of the HOD / LOD settings window (SettingsSpec.h).
// File: lsFlexLevels\HodLod.settings-<MKT>.txt (keyed). Migrates once from HodLod.place-<MKT>.txt ("pos,dx,dy,table,proj,mark,5 colours").
#pragma once
#include "SettingsSpec.h"

namespace hlset {

static const char* const PLUGIN_VERSION = "2.7.3";
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";
enum K { K_POSITION, K_SHOW_TABLE, K_TABLE_BG, K_SHOW_PROJ, K_PROJ_UP, K_PROJ_DN, K_SHOW_MARK, K_MARK_LOD, K_MARK_HOD, K_SHOW_PRE, K_PRE_MIN, K_N };

inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::listRow("position", "Position", 7, POS_CHOICES, 110),
        ss::masterOf(ss::boolRow("show_table", "Show table", true)),
        ss::same(ss::in(ss::colorRow("table_bg", "Table background", 0x000F1520), "show_table")),
        ss::masterOf(ss::boolRow("show_proj", "Show projection", true)),
        ss::same(ss::in(ss::colorRow("proj_up", "Up (HOD)", 0x0022C55E), "show_proj")),
        ss::same(ss::in(ss::colorRow("proj_dn", "Down (LOD)", 0x00EF4444), "show_proj")),
        ss::masterOf(ss::boolRow("show_mark", "Show 1st-extreme mark", true)),
        ss::same(ss::in(ss::colorRow("mark_lod", "LOD call", 0x0022C55E), "show_mark")),
        ss::same(ss::in(ss::colorRow("mark_hod", "HOD call", 0x00EF4444), "show_mark")),
        ss::masterOf(ss::boolRow("show_pre", "Show pre-open line", true)),                      // (2.6.0) the PRE-OPEN BOOK row
        ss::same(ss::in(ss::intRow("pre_min", "Pre-open minutes", 15, 5, 60, 50), "show_pre")),   // (2.6.0) N: the minutes before the open measured
    };
    static const ss::Spec S = { "HodLod", PLUGIN_VERSION, 8, R, K_N, 0 };   // 8 (2.6.0): + Show pre-open line / Pre-open minutes; 7 (2.4.4): Offset X / Y removed (the 9-spot Position only); his saved offsets were 0
    return S;
}

// The 2.0.5 - 2.4.0 place file: "pos[,dx,dy[,table,proj,mark,tableBg,projUp,projDn,markLod,markHod]]" (colours decimal, -1 = default).
inline bool legacy(const ss::Spec& S, int, const std::string& text, ss::Values& V) {
    std::string ln = text.substr(0, text.find('\n'));
    while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
    if (ln.empty() || ln.size() > 200) return false;
    const std::vector<std::string> f = ss::split(ln, ',');
    if (f.size() != 1 && f.size() != 3 && f.size() != 11) return false;
    static const char* const ORDER[11] = { "position", "offset_x", "offset_y", "show_table", "show_proj", "show_mark", "table_bg", "proj_up", "proj_dn", "mark_lod", "mark_hod" };
    std::vector<const char*> order(ORDER, ORDER + 11);
    ss::fromPositional(S, order, f, V);   // -1 colours are invalid -> the default colour (what -1 meant)
    return true;
}

} // namespace hlset
