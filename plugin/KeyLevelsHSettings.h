// KeyLevelsHSettings.h (lsKeyLevels 1.0.0, 2026-10-20, Rassul 22:20-22:41 CT): the ONE settings table of the Key Levels indicator
// for hourly charts. "create a seperate levels indicator without the open .. for my hourly charts", "levels that run across the
// charts", "have an option to show them [MenthorQ] but only show the price and volume levels ... by default", "the levels don't need
// liquidity amounts they just need a name and a price level", "place the badges right justified".
// Each group: a Show box + its colour (his SessionPrices colours as defaults). File: lsFlexLevels\KeyLevels.settings-<MKT>.txt.
#pragma once
#include "SettingsSpec.h"
#ifndef KLH_SEP
#if defined(_WIN32)
#define KLH_SEP "\\"
#else
#define KLH_SEP "/"
#endif
#endif

namespace klhset {
static const char* const PLUGIN_VERSION = "1.2.1";
enum K {
    K_SHOW_PF, K_COL_PF, K_SHOW_PD, K_COL_PD, K_SHOW_PW, K_COL_PW, K_SHOW_CW, K_COL_CW, K_SHOW_ON, K_COL_ON,
    K_SHOW_LON, K_COL_LON, K_SHOW_ASIA, K_COL_ASIA, K_SHOW_DPOC, K_COL_DPOC, K_SHOW_VA, K_COL_VA, K_SHOW_WPOC, K_COL_WPOC,
    K_SHOW_MQ, K_FONT, K_WIDTH, K_N
};
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::masterOf(ss::boolRow("show_pf", "Prior full high / low (PFH / PFL)", true)),
        ss::same(ss::in(ss::colorRow("col_pf", "Colour", 0x008A8A, 0), "show_pf")),
        ss::masterOf(ss::boolRow("show_pd", "Prior day high / low (PDH / PDL)", true)),
        ss::same(ss::in(ss::colorRow("col_pd", "Colour", 0xFFA500, 0), "show_pd")),
        ss::masterOf(ss::boolRow("show_pw", "Prior week high / low (PWH / PWL)", true)),
        ss::same(ss::in(ss::colorRow("col_pw", "Colour", 0x0000FF, 0), "show_pw")),
        ss::masterOf(ss::boolRow("show_cw", "This week high / low (CWH / CWL)", true)),
        ss::same(ss::in(ss::colorRow("col_cw", "Colour", 0x5470D6, 0), "show_cw")),
        ss::masterOf(ss::boolRow("show_on", "Overnight high / low (ONH / ONL)", true)),
        ss::same(ss::in(ss::colorRow("col_on", "Colour", 0x808080, 0), "show_on")),
        ss::masterOf(ss::boolRow("show_lon", "London high / low (LonHI / LonLO)", true)),
        ss::same(ss::in(ss::colorRow("col_lon", "Colour", 0xF59E0B, 0), "show_lon")),
        ss::masterOf(ss::boolRow("show_asia", "Asia high / low (AsiaHI / AsiaLO)", true)),
        ss::same(ss::in(ss::colorRow("col_asia", "Colour", 0x8E6CC0, 0), "show_asia")),
        ss::masterOf(ss::boolRow("show_dpoc", "Prior day POC (DPoc)", true)),
        ss::same(ss::in(ss::colorRow("col_dpoc", "Colour", 0x22D3EE, 0), "show_dpoc")),
        ss::masterOf(ss::boolRow("show_va", "Value area high / low (VAH / VAL)", true)),
        ss::same(ss::in(ss::colorRow("col_va", "Colour", 0xEF4444, 0), "show_va")),
        ss::masterOf(ss::boolRow("show_wpoc", "Prior week POC (WPoc)", true)),
        ss::same(ss::in(ss::colorRow("col_wpoc", "Colour", 0xE8D5B0, 0), "show_wpoc")),
        ss::boolRow("show_mq", "MenthorQ levels (CR / PS / HVL / G)", false),
        ss::intRow("font", "Font size", 9, 6, 20, 50),
        ss::same(ss::intRow("width", "Line width", 1, 1, 4, 50)),
    };
    static const ss::Spec S = { "KeyLevels", PLUGIN_VERSION, 1, R, K_N, 0 };
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) { return flex.empty() || mkt.empty() ? std::string() : flex + KLH_SEP "KeyLevels.settings-" + mkt + ".txt"; }
} // namespace klhset
