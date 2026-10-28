// BigBidAskSettings.h (lsBigBidAsk 1.19.0, 2026-10-20, Rassul 21:51 CT): the ONE settings table of Big Bid / Ask.
//   "have a setting biggest in which allows the user to enter a threshold like 10 or anything he wants .. default it to 10";
//   "a time exclusion like first 15 min and last 15 min, default to 15 min but the user can add anything ... 30 min or 60 min";
//   "allow user to enter font size and color and bid ask color".
// The number on the chart = how many candles back the cell is the biggest of its side (shown from "Biggest in", capped at 50).
// File: lsFlexLevels\BigBidAsk.settings2-<MKT>.txt (keyed, per market; the 1.16 BBA7 file BigBidAsk.settings-<MKT>.txt is never read).
#pragma once
#include "SettingsSpec.h"

namespace bbaset {
static const char* const PLUGIN_VERSION = "1.31.2";   // (1.31.2) no settings change   // (1.31.1) no settings change   // (1.31.0) no settings change   // (1.30.2) no settings change   // (1.30.1) no settings change   // (1.30.0, 2026-10-27) BO follow-through candle (BigBidAskLogic.h rule 4); no settings change   // (1.29.0, Rassul 2026-10-26 17:55 CT "get rid of them") the 6 OB rows are gone (OB not drawn);   // (1.28.0) no settings change (BO wave confirmation + biggest-in-N boxes);   // (1.27.1) Rly condition 3: +-1 SD tag OR a 30-bar high / low (RallyLogic 1.1.0);   // (1.26.0) the BO boxes row is gone (no boxes), OB candles after the pivot 1 - 3, BO / BD -> BO up / down; (1.25.0) + OB / BO boxes of strong waves (checkboxes, 3+ ATR waves, ATR, origin candles, colours); (1.24.0) + Rly (rally): show checkbox + up / down colours; (1.23.0) BO / BD green / red + same-wave rule
static const int CAP = 50;                      // the number shown never goes above 50 (Rassul 21:47 "cap the max at 50")
enum K { K_BIGGEST_IN, K_SKIP_FIRST, K_SKIP_LAST, K_FONT, K_COL_NUM, K_COL_BID, K_COL_ASK, K_ABS_CIRCLE, K_BREAKOUT, K_COL_BO, K_COL_BD, K_RLY, K_COL_RLY_UP, K_COL_RLY_DN,
         K_N };   // (1.29.0) K_OB / K_OB_ANY / K_OB_ATR / K_OB_ORIGIN / K_COL_OB_UP / K_COL_OB_DN removed (OB not drawn)
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::intRow("biggest_in", "Biggest in (candles)", 10, 2, CAP, 50),
        ss::intRow("skip_first", "Skip first (min)", 15, 0, 240, 50),
        ss::same(ss::intRow("skip_last", "Skip last (min)", 15, 0, 240, 50)),
        ss::intRow("font", "Font size", 10, 6, 20, 50),   // (1.19.1, Rassul 22:32 "set them as defaults") 10, was 7
        ss::same(ss::colorRow("col_num", "Number", 0xFFFFFF, 40)),
        ss::colorRow("col_bid", "Bid box (sellers)", 0xEF4444, 40),
        ss::same(ss::colorRow("col_ask", "Ask box (buyers)", 0x22C55E, 40)),
        // (1.20.0, MBO Roadmap Top Defender #1) a print in the wick that the order book shows absorbed gets a circle in the winner's colour
        ss::boolRow("abs_circle", "Circle prints at a shared A", true),   // (1.22.0) same key / place; the circle needs the shared A (SharedAbsorb.h)
        // (1.21.0, Rassul approved breakout-mockup-v2 20:10 CT 2026-10-21) BO / BD letter + the breakout print on the breakout candle
        ss::boolRow("breakout", "Breakout (BO)", true),
        ss::colorRow("col_bo", "BO up", 0x22C55E, 40),
        ss::same(ss::colorRow("col_bd", "BO down", 0xEF4444, 40)),
        // (1.24.0, Rassul approved the Rly mockup v0.2 15:10-15:15 CT 2026-10-22) the Rly mark (3-min charts) and its colours
        ss::boolRow("rly", "Rally (Rly)", true),
        ss::colorRow("col_rly_up", "Rly up", 0x22C55E, 40),
        ss::same(ss::colorRow("col_rly_dn", "Rly down", 0xEF4444, 40)),
        // (1.29.0, Rassul 2026-10-26 17:55 CT "right now we have OB and OB prints, get rid of them") the OB rows are gone: the OB print
        // and letter are not drawn; the OB decision + record (OBX / OBT) run on the fixed defaults (3.0 ATR, 2 candles after the pivot)
    };
    static const ss::Spec S = { "BigBidAsk", PLUGIN_VERSION, 14, R, K_N, 0 };   // parameter version 14 (1.29.0: - ob / ob_any / ob_atr / ob_origin / col_ob_up / col_ob_dn); 13 (1.26.0: - bo_box; ob_origin 1 - 3); 12 (1.25.0: + ob / bo_box / ob_any / ob_atr / ob_origin / col_ob_up / col_ob_dn); 11 (1.24.0: + rly / col_rly_up / col_rly_dn); 10 (1.21.0: + breakout / col_bo / col_bd; 9 = 1.20 + abs_circle; 8 = 1.19, 7 had no rows)
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\BigBidAsk.settings2-" + mkt + ".txt"; }
} // namespace bbaset
