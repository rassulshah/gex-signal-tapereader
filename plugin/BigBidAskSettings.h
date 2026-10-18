// BigBidAskSettings.h  (lsBigBidAsk 1.16.1, 2026-10-18 settings audit)  -  the ONE table of the Big Bid / Ask settings window.
// The SAME 87 rows in the same order as 1.16.0's setup() (labels included; parameter version 6 kept, so IRT's saved rows stay
// put). The per-market file keeps 1.16's BBA7 format (bba::formatSettings / parseSettings, tested by tests_bba115); the window is
// registered, filled and read through SettingsHost.h; the window <-> settings mapping is 1.16's controls() / toVec / fromVec.
// (1.16.1) the six volume levels on two lines of three (one line was ~422 px: "Wk VAL" cut off on the right).
// Per-market defaults shown in the window: the Divider (divFor) and the category 4 size (cat4For).
#pragma once
#include "SettingsSpec.h"
#include "BigBidAskLogic.h"

namespace bbaset {
static const char* const PLUGIN_VERSION = "1.16.2";
inline const ss::Spec& spec() {
    static const bba::Settings D;
    static const ss::Row R[] = {
        ss::labelRow("lbl_numbers", "NUMBERS", 300),
        ss::perMarket(ss::intRow("div", "Divider", 10, 1, 1000, 45), 10, 1, 10, 1, 1, 1, 1),
        ss::same(ss::boolRow("dim", "Dim away from a level", D.dim)),
        ss::boolRow("box", "Box round the numbers", D.box),
        ss::same(ss::colorRow("box_bid", "Bid", D.boxBid, 35)),
        ss::same(ss::colorRow("box_ask", "Ask", D.boxAsk, 35)),
        ss::boolRow("circle", "Absorption circle", D.circle),
        ss::same(ss::colorRow("circle_col", "Colour", D.circleCol, 35)),
        ss::labelRow("lbl_categories", "BIG LOT CATEGORIES   (number colours: Bid / Ask)", 380),
        ss::boolRow("cat1_on", "Category 1", D.cat[0].on),
        ss::same(ss::intRow("cat1_n", "Big Lot", D.cat[0].n, 1, 100, 40)),
        ss::same(ss::intRow("cat1_lots", "Min lots", D.cat[0].lots, 0, 100000, 50)),
        ss::same(ss::intRow("cat1_font", "Font", D.cat[0].font, 6, 20, 35)),
        ss::listRow("cat1_show", "     Show", D.cat[0].show, bba::SHOW_NAMES, 130),
        ss::same(ss::colorRow("cat1_bid", "Bid", D.cat[0].bid, 35)),
        ss::same(ss::colorRow("cat1_ask", "Ask", D.cat[0].ask, 35)),
        ss::boolRow("cat2_on", "Category 2", D.cat[1].on),
        ss::same(ss::intRow("cat2_n", "Big Lot", D.cat[1].n, 1, 100, 40)),
        ss::same(ss::intRow("cat2_lots", "Min lots", D.cat[1].lots, 0, 100000, 50)),
        ss::same(ss::intRow("cat2_font", "Font", D.cat[1].font, 6, 20, 35)),
        ss::listRow("cat2_show", "     Show", D.cat[1].show, bba::SHOW_NAMES, 130),
        ss::same(ss::colorRow("cat2_bid", "Bid", D.cat[1].bid, 35)),
        ss::same(ss::colorRow("cat2_ask", "Ask", D.cat[1].ask, 35)),
        ss::boolRow("cat3_on", "Category 3", D.cat[2].on),
        ss::same(ss::intRow("cat3_n", "Big Lot", D.cat[2].n, 1, 100, 40)),
        ss::same(ss::intRow("cat3_lots", "Min lots", D.cat[2].lots, 0, 100000, 50)),
        ss::same(ss::intRow("cat3_font", "Font", D.cat[2].font, 6, 20, 35)),
        ss::listRow("cat3_show", "     Show", D.cat[2].show, bba::SHOW_NAMES, 130),
        ss::same(ss::colorRow("cat3_bid", "Bid", D.cat[2].bid, 35)),
        ss::same(ss::colorRow("cat3_ask", "Ask", D.cat[2].ask, 35)),
        ss::boolRow("cat4_on", "Category 4", D.cat[3].on),
        ss::same(ss::perMarket(ss::intRow("cat4_lots", ">= lots", 1000, 1, 100000, 55), 1000, 450, 95, 105, 90, 295, 490)),
        ss::same(ss::intRow("cat4_font", "Font", D.cat[3].font, 6, 20, 35)),
        ss::colorRow("cat4_bid", "     Bid", D.cat[3].bid, 35),
        ss::same(ss::colorRow("cat4_ask", "Ask", D.cat[3].ask, 35)),
        ss::labelRow("lbl_time", "TIME FILTER", 300),
        ss::intRow("skip_first", "Skip first (min)", D.skipFirst, 0, 240, 40),
        ss::same(ss::intRow("skip_last", "Skip last (min)", D.skipLast, 0, 240, 40)),
        ss::listRow("sessions", "Sessions", D.rthOnly ? 1 : 0, bba::SESSION_NAMES, 130),
        ss::labelRow("lbl_near", "NEAR A LEVEL", 300),
        ss::floatRow("atr_up", "ATR above", D.atrUpQ / 100.0, 0.0, 20.0, 45),
        ss::same(ss::floatRow("atr_dn", "ATR below", D.atrDnQ / 100.0, 0.0, 20.0, 45)),
        ss::masterOf(ss::boolRow("key_on", "Key levels", D.keyOn)),
        ss::in(ss::boolRow("key_0", "ONH", D.keyOn && ((D.keyMask >> 0) & 1u)), "key_on"),
        ss::same(ss::in(ss::boolRow("key_1", "ONL", D.keyOn && ((D.keyMask >> 1) & 1u)), "key_on")),
        ss::same(ss::in(ss::boolRow("key_2", "PDH", D.keyOn && ((D.keyMask >> 2) & 1u)), "key_on")),
        ss::same(ss::in(ss::boolRow("key_3", "PDL", D.keyOn && ((D.keyMask >> 3) & 1u)), "key_on")),
        ss::same(ss::in(ss::boolRow("key_4", "PFH", D.keyOn && ((D.keyMask >> 4) & 1u)), "key_on")),
        ss::same(ss::in(ss::boolRow("key_5", "PFL", D.keyOn && ((D.keyMask >> 5) & 1u)), "key_on")),
        ss::in(ss::boolRow("key_6", "LonHI", D.keyOn && ((D.keyMask >> 6) & 1u)), "key_on"),
        ss::same(ss::in(ss::boolRow("key_7", "LonLO", D.keyOn && ((D.keyMask >> 7) & 1u)), "key_on")),
        ss::same(ss::in(ss::boolRow("key_8", "PWH", D.keyOn && ((D.keyMask >> 8) & 1u)), "key_on")),
        ss::same(ss::in(ss::boolRow("key_9", "PWL", D.keyOn && ((D.keyMask >> 9) & 1u)), "key_on")),
        ss::same(ss::in(ss::boolRow("key_10", "HOD", D.keyOn && ((D.keyMask >> 10) & 1u)), "key_on")),
        ss::same(ss::in(ss::boolRow("key_11", "LOD", D.keyOn && ((D.keyMask >> 11) & 1u)), "key_on")),
        ss::in(ss::boolRow("key_12", "AHI", D.keyOn && ((D.keyMask >> 12) & 1u)), "key_on"),
        ss::same(ss::in(ss::boolRow("key_13", "ALO", D.keyOn && ((D.keyMask >> 13) & 1u)), "key_on")),
        ss::same(ss::in(ss::boolRow("key_14", "CWH", D.keyOn && ((D.keyMask >> 14) & 1u)), "key_on")),
        ss::same(ss::in(ss::boolRow("key_15", "CWL", D.keyOn && ((D.keyMask >> 15) & 1u)), "key_on")),
        ss::masterOf(ss::boolRow("gam_on", "Gamma levels", D.gamOn)),
        ss::in(ss::boolRow("gam_0", "CR", D.gamOn && ((D.gamMask >> 0) & 1u)), "gam_on"),
        ss::same(ss::in(ss::boolRow("gam_1", "PS", D.gamOn && ((D.gamMask >> 1) & 1u)), "gam_on")),
        ss::same(ss::in(ss::boolRow("gam_2", "HVL", D.gamOn && ((D.gamMask >> 2) & 1u)), "gam_on")),
        ss::same(ss::in(ss::boolRow("gam_3", "CR 0DTE", D.gamOn && ((D.gamMask >> 3) & 1u)), "gam_on")),
        ss::same(ss::in(ss::boolRow("gam_4", "PS 0DTE", D.gamOn && ((D.gamMask >> 4) & 1u)), "gam_on")),
        ss::same(ss::in(ss::boolRow("gam_5", "G1", D.gamOn && ((D.gamMask >> 5) & 1u)), "gam_on")),
        ss::in(ss::boolRow("gam_6", "G2", D.gamOn && ((D.gamMask >> 6) & 1u)), "gam_on"),
        ss::same(ss::in(ss::boolRow("gam_7", "G3", D.gamOn && ((D.gamMask >> 7) & 1u)), "gam_on")),
        ss::same(ss::in(ss::boolRow("gam_8", "G4", D.gamOn && ((D.gamMask >> 8) & 1u)), "gam_on")),
        ss::same(ss::in(ss::boolRow("gam_9", "G5", D.gamOn && ((D.gamMask >> 9) & 1u)), "gam_on")),
        ss::masterOf(ss::boolRow("vol_on", "Volume levels", D.volOn)),
        ss::in(ss::boolRow("vol_0", "PD POC", D.volOn && ((D.volMask >> 0) & 1u)), "vol_on"),
        ss::same(ss::in(ss::boolRow("vol_1", "PD VAH", D.volOn && ((D.volMask >> 1) & 1u)), "vol_on")),
        ss::same(ss::in(ss::boolRow("vol_2", "PD VAL", D.volOn && ((D.volMask >> 2) & 1u)), "vol_on")),
        ss::in(ss::boolRow("vol_3", "Wk POC", D.volOn && ((D.volMask >> 3) & 1u)), "vol_on"),
        ss::same(ss::in(ss::boolRow("vol_4", "Wk VAH", D.volOn && ((D.volMask >> 4) & 1u)), "vol_on")),
        ss::same(ss::in(ss::boolRow("vol_5", "Wk VAL", D.volOn && ((D.volMask >> 5) & 1u)), "vol_on")),
        ss::labelRow("lbl_box", "BIG LOT BOX", 300),
        ss::boolRow("stats_chart", "Show the Big Lot box (BL5 / BL10 / BL20)", D.statsChart),
        ss::listRow("stats_pos", "Position without a HOD/LOD table", D.statsPos - 1, bba::POS_NAMES, 110),
        ss::labelRow("lbl_stats", "STATISTICS (read only)", 300),
        ss::labelRow("stat_0", "(opens with the chart's numbers)", 380),
        ss::labelRow("stat_1", "", 380),
        ss::labelRow("stat_2", "", 380),
        ss::labelRow("stat_3", "", 380),
        ss::labelRow("stat_4", "", 380),
        ss::labelRow("stat_5", "", 380)
    };
    static const ss::Spec S = { "BigBidAsk", PLUGIN_VERSION, 6, R, (int)(sizeof(R) / sizeof(R[0])), 18 };   // 18 lines tall, as 1.16.0
    return S;
}
} // namespace bbaset
