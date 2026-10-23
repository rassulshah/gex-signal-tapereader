// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl240.cpp -o t240 && ./t240   (from plugin\tests_hodlod)
// HL 2.4.0 (2026-10-18 night, audit C findings 7 / 8 / 13 + the Market State) tests, SDK-free (HodLodLogic.h + HodLodGex.h + HodLodQ.h + Fresh.h + MarketState.h):
//  [1] the regime's ONE source: GammaProfile-MQ-<MKT>.csv HVL0 (not FLIP), strict parse, the ES / CL / EU files of 10-16
//  [2] the regime verdict (Fresh.h): LIVE / PAUSED shown, STALE -> "Reg: STALE"; the header text through regimeParts
//  [3] Q-Scores age: STALE -> "Q: STALE" + the old scores in the hover only; fresh unchanged
//  [4] the Gex S/R row: outside MenthorQ hours an old grid reads STALE (was blank); in hours > 20 min STALE; the closing set PAUSED (blank)
//  [5] the Market State facts HODLOD / TREND: keys exactly as MARKET-STATE-SPEC 1.0, '?' when unknown, stale regime / Q never posted as live,
//      round trip through mst::encode / decode / publish / read / clear
//  [6] versions: HL_VERSION == the setVersion literal in HodLod.cpp == 2.4.0
#include <cstdio>
#include <cmath>
#include <fstream>
#include <sstream>
#include <set>
#include "HodLodLogic.h"
#include "HodLodGex.h"
#include "HodLodQ.h"
#include "Fresh.h"
#include "MarketState.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static long long at(int y, int mo, int d, int h, int mi) { return fr::ctAbs(y, mo, d, h * 3600 + mi * 60); }
static const char* ES_BOOK =
    "# MenthorQ intraday book ES1! (ES) as of 2026-10-16T16:45:00 ET (15:45 CT), for session 2026-10-16\n"
    "BOOK,MQ\nMARKET,ES\nKIND,intraday\nSESSION,2026-10-16\n"
    "STRIKE,7700,-100,1,1,KING SLICE,7700,,0\nSTRIKE,7780,-5,13,0,HVL0,7780,,600\nSTRIKE,7795,1,15,0,CR0+GW0,7795,,54000\nSTRIKE,7820,-2,14,0,,7820,,23700\n"
    "KING,7700\nCW,8000\nPW,7700\nFLIP,7820\nEMH,7848.65\nEML,7723.35\nASOF,56700,2026-10-16\n";
static const char* CL_BOOK = "BOOK,MQ\r\nMARKET,CL\r\nSESSION,2026-10-16\r\nSTRIKE,92.5,0,14,0,PS0,92.5,,54900\r\nSTRIKE,94.75,0,16,0,HVL0,94.75,,12300\r\nFLIP,87.25\r\nASOF,56700,2026-10-16\r\n";
static const char* EU_BOOK = "MARKET,EU\nSESSION,2026-10-16\nSTRIKE,1.12,3,4,0,PS0+HVL0,1.12,,0\nFLIP,1.1150\nASOF,56700\n";
static std::string qFile(const std::string& date) { return "VERSION|1\nMARKET|ES\nDATE|" + date + "\nQ|1|1|4|-\nPREV|2026-10-15|2|1|3|-\nRECV|1792199400\n"; }
static std::string txt(const hl::Line& L) { std::string s; for (const hl::Seg& g : L) s += g.s; return s; }
static std::string segs(const std::vector<hl::Seg>& v) { std::string s; for (const hl::Seg& g : v) s += g.s; return s; }
// the plugin's hlSessSegs, the same decisions (HodLod.cpp), on SDK-free inputs
static void sessSegs(hl::View& v, const hlg::HvlBook& B, double close, long long nowCt, int& sign, fr::Verdict& V)
{
    v.sessSegs.clear(); sign = 0; V = fr::Verdict();
    if (!B.ok) return;
    V = fr::judge(fr::K_MQ_LEVELS, B.asofCt, nowCt);
    const int sg = hlg::regimeSign(B, close);
    if (V.stale()) v.sessSegs.push_back(hl::Seg{ "Regime: STALE", hl::C_GREY });
    else if (sg != 0) { sign = sg; v.sessSegs.push_back(hl::Seg{ sg < 0 ? "Regime: Negative" : "Regime: Positive", sg < 0 ? hl::C_RED : hl::C_GREEN }); }
}
static std::string gexFile(const std::string& asofUnix, const std::string& state) {
    return "VERSION|2\nMARKET|ES\nSESSION|2026-10-16\nASOF|" + asofUnix + "|4:02 PM|15:55\nSTATE|" + state + "|\nVWAP|7720|down\nBASIS|VERIFIED|0|0|5|-|same\n"
           "BAND|7703.00|RTH|7800.00|7650.00|7773.00|7664.00|1.38|8:30|EOD\nLOC|1|A|7765|7765|F|0|1\nSTR|1|7765|7765|9|30|F|-|-|-|1\nLOC|2|B|7700|7700|F|0|1\nSTR|2|7700|7700|10|33|F|-|-|PS|1\n";
}

int main() {
    std::printf("[1] the HVL0 book\n");
    {   hlg::HvlBook B = hlg::parseHvl(ES_BOOK, "ES");
        CHECK(B.ok && B.hvl == 7780 && B.hvlTxt == "7780" && B.flip == 7820 && B.asofCt == at(2026, 10, 16, 15, 45), "ES: HVL0 7780 (not FLIP 7820), asof Fri 15:45 CT");
        hlg::HvlBook C = hlg::parseHvl(CL_BOOK, "CL"); CHECK(C.ok && C.hvl == 94.75 && C.flip == 87.25, "CL (CRLF): HVL0 94.75, FLIP 87.25 not used");
        hlg::HvlBook E = hlg::parseHvl(EU_BOOK, "EU"); CHECK(E.ok && std::fabs(E.hvl - 1.12) < 1e-12 && E.asofCt == at(2026, 10, 16, 15, 45), "EU: 'PS0+HVL0' strike; no ASOF date -> the session date");
        CHECK(!hlg::parseHvl(ES_BOOK, "NQ").ok && hlg::parseHvl(ES_BOOK, "NQ").why == "book is for another market", "another market: refused");
        std::string noHvl = ES_BOOK; noHvl.replace(noHvl.find("HVL0"), 4, "    "); CHECK(!hlg::parseHvl(noHvl, "ES").ok && hlg::parseHvl(noHvl, "ES").why == "no HVL0 strike in the book", "no HVL0: no regime (FLIP is never a fallback)");
        std::string noAsof = ES_BOOK; noAsof.erase(noAsof.find("ASOF")); CHECK(!hlg::parseHvl(noAsof, "ES").ok, "no ASOF: refused (unknown age is never live)");
        CHECK(!hlg::parseHvl(std::string(70000, 'x'), "ES").ok, "over 64 KiB: refused");
        CHECK(!hlg::parseHvl(std::string(ES_BOOK).substr(0, 60), "ES").ok, "torn: refused");
        std::string bad = ES_BOOK; bad.replace(bad.find("STRIKE,7780"), 11, "STRIKE,77x0"); CHECK(!hlg::parseHvl(bad, "ES").ok, "garbled HVL0 price: not used");
        std::string eve = "MARKET,ES\nSESSION,2026-10-20\nSTRIKE,7780,-5,13,0,HVL0,7780,,0\nASOF,79200\n";
        CHECK(hlg::parseHvl(eve, "ES").asofCt == at(2026, 10, 19, 22, 0), "an evening snapshot (22:00) without a date belongs to the day before its session");
        CHECK(hlg::regimeSign(B, 7703) == -1 && hlg::regimeSign(B, 7780) == 1 && hlg::regimeSign(B, 7790) == 1 && hlg::regimeSign(B, NAN) == 0 && hlg::regimeSign(hlg::HvlBook(), 7703) == 0, "regime sign: below Neg, at / above Pos, unknown 0");
        CHECK(hlg::regimeSign(B, 7800) == 1, "the audit case: between HVL0 7780 and FLIP 7820 the box now says Pos, as the HVL badge does");
    }
    std::printf("[2] the regime verdict and the header text\n");
    {   hlg::HvlBook B = hlg::parseHvl(ES_BOOK, "ES"); hl::View v; int sg = 0; fr::Verdict V;
        sessSegs(v, B, 7703, at(2026, 10, 17, 23, 50), sg, V);   // the audit time: Saturday night, MenthorQ paused
        CHECK(V.st == fr::PAUSED && sg == -1 && hl::regimeText(v) == "Reg: Neg", "Saturday: Friday's closing set = PAUSED, 'Reg: Neg' (the closing regime): " + hl::regimeText(v));
        sessSegs(v, B, 7703, at(2026, 10, 19, 9, 0), sg, V);    // Monday 9:00, the bridge never sent Monday's grid
        CHECK(V.stale() && sg == 0 && hl::regimeText(v) == "Reg: STALE" && v.sessSegs[0].col == hl::C_GREY, "Monday 9:00 with Friday's book = 'Reg: STALE' grey, never 'Reg: Neg': " + hl::regimeText(v));
        v.trendReady = true; v.trendLvl = -1; CHECK(hl::regimeText(v) == "Reg: STALE / Dntrend", "STALE keeps the trend part: " + hl::regimeText(v));
        hlg::HvlBook L = B; L.asofCt = at(2026, 10, 19, 9, 50);
        sessSegs(v, L, 7790, at(2026, 10, 19, 10, 5), sg, V); CHECK(V.st == fr::LIVE && sg == 1 && hl::regimeText(v) == "Reg: Pos / Dntrend", "a 15-min-old book: LIVE, Pos");
        sessSegs(v, L, 7790, at(2026, 10, 19, 10, 11), sg, V); CHECK(V.stale() && hl::regimeText(v).compare(0, 10, "Reg: STALE") == 0, "21 min old in hours: STALE");
        sessSegs(v, hlg::HvlBook(), 7790, at(2026, 10, 19, 10, 5), sg, V); CHECK(v.sessSegs.empty() && hl::regimeText(v) == "Reg: -- / Dntrend", "no book: 'Reg: --' (as before)");
    }
    std::printf("[3] Q-Scores age\n");
    {   hlq::File F = hlq::parse(qFile("2026-10-16"), "ES");
        fr::Verdict fv = hlq::fresh(F, at(2026, 10, 18, 0, 10)); hl::View v; hlq::toView(v, F, "Dntrend", -1, &fv);
        CHECK(fv.st == fr::LIVE && segs(v.qSegs) == "Q: M1  O1  V4" && v.qHover.size() == 6, "Friday's scores on Sunday: live, unchanged");
        fv = hlq::fresh(F, at(2026, 10, 20, 9, 0)); hlq::toView(v, F, "Dntrend", -1, &fv);
        CHECK(fv.stale() && segs(v.qSegs) == "Q: STALE" && v.qSegs[1].col == hl::C_GREY, "Friday's scores on Tuesday: 'Q: STALE' (grey), no M / O / V in the box");
        CHECK(v.qHover.size() == 2 && txt(v.qHover[0]).find("STALE") != std::string::npos && txt(v.qHover[0]).find("Oct 16") != std::string::npos && txt(v.qHover[1]).find("M1  O1  V4") != std::string::npos, "the hover: STALE + the date + the old scores");
        CHECK(hlq::statusLine(F, "", &fv).find(",STALE") != std::string::npos && hlq::statusLine(F, "").find("STALE") == std::string::npos, "status line says STALE");
        hl::View w; hlq::toView(w, F, "", 0); CHECK(segs(w.qSegs) == "Q: M1  O1  V4", "no verdict (old callers / tests): unchanged");
        CHECK(hlq::fresh(hlq::File(), at(2026, 10, 18, 0, 10)).stale(), "no file: stale (and nothing drawn)");
    }
    std::printf("[4] the Gex S/R row age\n");
    {   // ASOF 1792184535 = Fri 2026-10-16 16:02:15 CDT; now = unix, wd / min = the CT clock at that moment
        hlg::File F = hlg::parse(gexFile("1792184535", "LIVE"), "ES");
        hl::View a; hlg::toView(a, F, 7703, 1792184535 + 33 * 3600, 6, (16 * 60 + 2 + 33 * 60) % 1440);   // Saturday 01:02
        CHECK(a.gexStale.empty() && !a.gexItems.empty(), "the weekend (Sun 01:02) with Friday's closing grid: PAUSED, no STALE (as before)");
        hl::View b; hlg::toView(b, F, 7703, 1792184535 + 65 * 3600 + 58 * 60, 1, 10 * 60);   // Monday 10:00
        CHECK(b.gexStale.compare(0, 6, "STALE ") == 0, "Monday 10:00 with Friday's grid: STALE: " + b.gexStale);
        hlg::File T = hlg::parse(gexFile("1792140000", "LIVE"), "ES");                       // Fri 03:40 CDT
        hl::View c; hlg::toView(c, T, 7703, 1792184535 + 33 * 3600, 6, (16 * 60 + 2 + 33 * 60) % 1440);
        CHECK(c.gexStale.compare(0, 6, "STALE ") == 0, "the weekend with a grid from 03:40 Friday (the feed died before the close): STALE (was blank): " + c.gexStale);
        hl::View d; hlg::toView(d, F, 7703, 1792184535 - 2 * 3600 + 15 * 60, 5, 14 * 60 + 17);   // Fri 14:17, ASOF in the future by 1:45 h
        CHECK(d.gexItems.empty(), "a grid from the future is never shown (unchanged)");
        hl::View e; hlg::toView(e, F, 7703, 1792184535 + 10 * 60, 5, 16 * 60 + 12); CHECK(e.gexStale.empty(), "Fri 16:12, the 16:02 grid: PAUSED");
    }
    std::printf("[5] Market State facts\n");
    {   hl::View v; v.has = true; v.side = 2; v.hod = 31000; v.lod = 30800; v.hodMod = 8 * 60 + 33; v.lodMod = 9 * 60 + 21; v.usedPct = 72;
        v.projOk = true; v.projUp = false; v.projTo = 30700; v.trendReady = true; v.trendLvl = -2;
        hl::MstIn in; in.tick = 0.25; in.dec = 2; in.regime = -1; in.dataAgeMin = 7; in.qOk = true; in.qm = 1; in.qo = 0; in.qv = 4;
        in.bandOk = true; in.bandHi = 7800; in.bandLo = 7650;
        std::map<std::string, std::string> kv = hl::mstHodLod(v, in);
        const std::set<std::string> want = { "hod", "lod", "hod_time", "lod_time", "first_extreme", "proj_hi", "proj_lo", "range_used", "q_m", "q_o", "q_v", "trend", "regime", "gex_band_hi", "gex_band_lo", "data_age_min" };
        std::set<std::string> got; for (const auto& p : kv) got.insert(p.first);
        CHECK(got == want, "HODLOD keys = the spec's 16");
        CHECK(kv["hod"] == "7750.00" && kv["lod"] == "7700.00" && kv["hod_time"] == "08:33" && kv["lod_time"] == "09:21" && kv["first_extreme"] == "hod", "extremes");
        CHECK(kv["proj_lo"] == "7675.00" && kv["proj_hi"] == "?" && kv["range_used"] == "0.72", "projection + range used");
        CHECK(kv["q_m"] == "1" && kv["q_o"] == "0" && kv["q_v"] == "4" && kv["trend"] == "-2" && kv["regime"] == "neg" && kv["gex_band_hi"] == "7800.00" && kv["gex_band_lo"] == "7650.00" && kv["data_age_min"] == "7", "slow state");
        hl::MstIn st = in; st.regime = 0; st.qOk = false; st.bandOk = false; st.dataAgeMin = -1;
        std::map<std::string, std::string> k2 = hl::mstHodLod(v, st);
        CHECK(k2["regime"] == "?" && k2["q_m"] == "?" && k2["gex_band_hi"] == "?" && k2["data_age_min"] == "?", "stale regime / Q / band are posted as '?', never as live");
        hl::View none; std::map<std::string, std::string> k3 = hl::mstHodLod(none, hl::MstIn());
        bool allQ = true; for (const auto& p : k3) if (p.second != "?") allQ = false; CHECK(allQ && k3.size() == 16, "no view: every fact '?'");
        std::map<std::string, std::string> t = hl::mstTrend(v); CHECK(t.size() == 2 && t["trend"] == "-2" && t["trend_word"] == "S. Dntrend", "TREND: -2 S. Dntrend");
        hl::View nr; std::map<std::string, std::string> t2 = hl::mstTrend(nr); CHECK(t2["trend"] == "?" && t2["trend_word"] == "?", "TREND not ready: '?'");
        // the board: publish / read / clear (Linux fake = setenv)
        mst::publish("HODLOD", "ESZ6", 180, "ES 3 Min", 1000, at(2026, 10, 19, 9, 3), kv);
        mst::Facts F = mst::read("HODLOD", "ESZ6", 180, "ES 3 Min", 2000);
        CHECK(F.ok && F.bar == at(2026, 10, 19, 9, 3) && F.kv == kv, "round trip through the board");
        CHECK(!mst::read("HODLOD", "ESZ6", 180, "ES 3 Min", 1000 + mst::FRESH_MS + 1).ok, "older than 60 s: not read (why the plugin re-posts every 10 s)");
        CHECK(!mst::read("HODLOD", "ESZ6", 180, "NQ 3 Min", 2000).ok && !mst::read("HODLOD", "ESZ6", 60, "ES 3 Min", 2000).ok, "another chart / bar size: separate entries");
        mst::clear("HODLOD", "ESZ6", 180, "ES 3 Min"); CHECK(!mst::read("HODLOD", "ESZ6", 180, "ES 3 Min", 2000).ok, "cleared on removal");
        CHECK(mst::encode(1, 2, kv).size() < 600, "well under the entry cap");
    }
    std::printf("[6] versions\n");
    {   CHECK(std::string(hl::HL_VERSION) == "2.7.1", "HL_VERSION 2.6.0 (2.6.0 = the pre-open book row; 2.4.4 = the compact table + strip; 2.4.1 = the table rectangle published, BoxDock.h; 2.4.2 = the settings layer)");
        std::ifstream f("../HodLod.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string src = ss.str();
        CHECK(src.find(std::string("p->setVersion(\"") + hl::HL_VERSION + "\")") != std::string::npos, "setVersion literal == HL_VERSION");
        CHECK(std::string(hlg::VERSION) == "2.1.0" && std::string(hlq::VERSION) == "1.1.0" && std::string(fr::VERSION) == "1.0.0", "header versions");
    }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
