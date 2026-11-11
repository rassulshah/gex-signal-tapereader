// HodLodSettings.h  (lsHodLod 2.6.0: Show pre-open line + Pre-open minutes (the PRE-OPEN BOOK row), parameter version 8; 2.4.4: Offset X / Y removed, the old files' offsets are ignored; 2.4.2, 2026-10-18 settings audit)  -  the ONE table of the HOD / LOD settings window (SettingsSpec.h).
// File: lsFlexLevels\HodLod.settings-<MKT>.txt (keyed). Migrates once from HodLod.place-<MKT>.txt ("pos,dx,dy,table,proj,mark,5 colours").
#pragma once
#include "SettingsSpec.h"

namespace hlset {

static const char* const PLUGIN_VERSION = "2.23.0";   // (2.23.0) header row / read box simplified - no settings change; was 2.22.2:   // (2.22.2) read box further left - no settings change   // was "2.22.1";   // (2.22.1) read box moved up / ASCII - no settings change   // was "2.22.0":   // (2.22.0) the READ BOX left column - no settings change   // was "2.21.0":   // (2.21.0) the HOD / LOD confirm badge - no settings change   // was "2.20.0":   // (2.20.0) the chart read strip - no settings change   // was "2.19.1":   // (2.19.0) the Range Day pill - no settings change   // was "2.18.0":   // (2.18.0) LOD SWEEPS left of HOD SWEEPS - no settings change   // was "2.17.9":   // (2.17.9) the sweep sections kept in a compact form on narrow panes - no settings change   // was "2.17.8":   // (2.17.8) London ends at the earlier of 07:30 and the RTH open - no settings change   // was (2.17.7) the box runs to the price axis - no settings change   // was "2.17.6":   // (2.17.6) no settings change   // (2.17.5) no settings change   // was "2.17.4":   // (2.17.4) no settings change   // was "2.17.3":   // (2.17.3) no settings change   // was "2.17.2":   // (2.17.2) no settings change   // was "2.17.1":   // (2.17.1) no settings change   // was "2.17.0":   // (2.17.0) no settings change   // was "2.16.0":   // (2.16.0) no settings change   // was "2.15.4":   // (2.15.4) no settings change   // was "2.15.3":   // (2.15.3) no settings change   // was "2.15.2":   // (2.15.2) no settings change   // was "2.15.1":   // (2.14.3) no settings change   // was "2.14.2":   // (2.14.2) no settings change   // was "2.14.1":   // (2.14.1) no settings change (compact)   // was "2.14.0":   // (2.14.0) no settings change (new box design)   // was "2.13.0":   // (2.13.0) no settings change (the HOD / LOD sweep sections)   // was "2.12.0":   // (2.12.0) no settings change (box trimmed: no Q-Scores / 1ST-2ND row / Est / footer; column separator)
// was 2.11.0:
//   2.11.0: (2.11.0) no settings change (docks bottom-left, right of the Key Levels column; no S / R squares; smaller spot)
// was 2.9.3
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
