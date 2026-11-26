// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2400.cpp -o t2400 && ./t2400   (from plugin\tests_hodlod)
// lsHodLod 2.40.0 (Rassul 2026-11-20 09:43 CT "this looks good. build", mockup hodlod-box-plan-mockup.html): the LOD / HOD SWEEPS sections
// replaced by BUY AT · 3-HOUR SUPPORT / SELL AT · 3-HOUR RESISTANCE = the MQ Reader's TODAY PLAN for the chart's market (TodayPlan.h 1.0.0)
//  [1] = THE READER: 14 cases (golden_2400.txt, written by gen_2400.py with the Reader's own todayPlanHtf() run in node on the bridge's
//      htf dict) - the real ES state + CobSignal status of 11-20 09:43 ("S1 7,105 | PS0+PS | NEAR | T1 7,150.00", sell "-"), NEAR on / off
//      (away, at the 0.25 EM edge, just past it), price_live missing, SPRING, UPTHRUST with key-level tags (ONL · LL -> ONL/LonLO, LH, AH,
//      MQ-only), S2 shown / S1 not standing, no EM, NQ / GC / 6E formats: every row (level, tags, chip, target) and the time identical
//  [2] the files: the real CobSignal status / the trimmed real state.json; a decoy "markets" inside another key; escapes; torn / garbled /
//      oversized / other-market / other-session state; HTFNOW missing / stale
//  [3] the box: plan sections left (BUY, green) / right (SELL, red), SESSION in the middle, every text inside its section, no overlaps,
//      grey tags, the NEAR chip, "-" for an empty side; the box shorter than with the sweep sections and its bottom edge where it was;
//      the gamma / Now lines (above the box) not touched by the box; narrow panes: the header-only box, never past the pane
//  [4] host: the sweep rows are still built and written (SWROW lines, unchanged format) for the bridge's sweep odds + the TODAY_PLAN line;
//      versions 2.40.0 everywhere; gex-build lists TodayPlan.h
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../TodayPlan.h"
#include "lib_widths.h"
#include "scen.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; if (fails < 60) std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static std::string rd(const char* p) { std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
static std::vector<std::string> split(const std::string& s, char d) { std::vector<std::string> o; size_t a = 0; for (;;) { size_t c = s.find(d, a); o.push_back(s.substr(a, c == std::string::npos ? std::string::npos : c - a)); if (c == std::string::npos) break; a = c + 1; } return o; }
static double SC = 1.0;
static int MS(const std::string& s, int fs, bool b) { return (int)(measureLib(s, fs, b) * SC + 0.5); }
static std::vector<hl::Item> texts(const hl::Layout& L) { std::vector<hl::Item> t; for (const hl::Item& it : L.items) if (it.k == hl::K_TEXT && !it.s.empty() && it.role == hl::R_TABLE) t.push_back(it); return t; }
static int overlaps(const std::vector<hl::Item>& tx) { int n = 0; for (size_t i = 0; i < tx.size(); i++) for (size_t j = i + 1; j < tx.size(); j++) { const hl::Item& a = tx[i]; const hl::Item& b = tx[j];
    if (a.t < b.b - 1 && b.t < a.b - 1 && a.l < b.r - 1 && b.l < a.r - 1) ++n; } return n; }
static std::string u8(const std::string& s) { return tpl::toUtf8(s); }
static hl::View planView(int m, const tpl::Plan& P) {
    hl::View v = mkView(m, true); v.planOn = true; v.planTime = P.time12;
    for (const tpl::Row& r : P.buy) v.planBuy.push_back(hl::View::PlanRow{ r.level, r.tags, r.chip, r.target });
    for (const tpl::Row& r : P.sell) v.planSell.push_back(hl::View::PlanRow{ r.level, r.tags, r.chip, r.target });
    return v; }

int main() { (void)&itemsText; (void)&fnv;
    std::printf("[1] = the Reader (golden_2400.txt)\n");
    int cases = 0; tpl::Plan REAL;
    {   std::istringstream g(rd("golden_2400.txt")); std::string ln, name, mkt, htf, state, tm; std::vector<std::string> buy, sell;
        while (std::getline(g, ln)) {
            if (ln.empty() || ln[0] == '#') continue;
            const std::vector<std::string> f = split(ln, '|');
            if (f[0] == "CASE") { name = f[1]; mkt = f[2]; buy.clear(); sell.clear(); tm.clear(); }
            else if (f[0] == "HTF") htf = ln.substr(4);
            else if (f[0] == "STATE") state = ln.substr(6);
            else if (f[0] == "BUY" || f[0] == "SELL") (f[0] == "BUY" ? buy : sell).push_back(f[1] + "|" + f[2] + "|" + f[3] + "|" + f[4]);
            else if (f[0] == "TIME") tm = f[1];
            else if (f[0] == "END") {
                ++cases;
                const tpl::Plan P = tpl::build(tpl::parseHtf(htf + "\n"), false, tpl::parseState(state, mkt), mkt);
                std::vector<std::string> b2, s2; for (const tpl::Row& r : P.buy) b2.push_back(u8(r.level) + "|" + u8(r.tags) + "|" + r.chip + "|" + u8(r.target));
                for (const tpl::Row& r : P.sell) s2.push_back(u8(r.level) + "|" + u8(r.tags) + "|" + r.chip + "|" + u8(r.target));
                std::string want, got; for (auto& x : buy) want += "B " + x + " ; "; for (auto& x : sell) want += "S " + x + " ; "; for (auto& x : b2) got += "B " + x + " ; "; for (auto& x : s2) got += "S " + x + " ; ";
                CHECK(got == want, name + ": rows = the Reader's: want [" + want + "] got [" + got + "]");
                CHECK(P.time12 == tm, name + ": time " + P.time12 + " = " + tm);
                if (name == "real_0943") REAL = P;
            }
        }
    }
    CHECK(cases == 14, "14 Reader cases (got " + std::to_string(cases) + ")");
    CHECK(REAL.ok && REAL.buy.size() == 1 && REAL.buy[0].level == "S1 7,105" && REAL.buy[0].tags == "PS0+PS" && REAL.buy[0].chip == "NEAR" && REAL.buy[0].target == "7,150.00" && REAL.sell.empty(),
          "his Reader 09:33 / 09:43: S1 7,105 PS0+PS NEAR T1 7,150.00, sell -");
    std::printf("[2] the files\n");
    {   const std::string ST = rd("tp_state_es_1120.json"), CS = rd("tp_cob_status_es_1120.txt");
        const tpl::Htf H = tpl::parseHtf(CS);
        CHECK(H.have && H.r1 == 7440.5 && H.r1ok && H.s1 == 7105 && H.s1ok && H.s2 == 7105 && H.dir == 1 && H.ext == 7174.5, "real CobSignal status HTFNOW parsed");
        CHECK(!tpl::parseHtf("VERSION,1.11.0\nHTF,R1=1\n").have && !tpl::parseHtf("HTFNOW,1,1,2\n").have, "no / short HTFNOW = none");
        const tpl::St S = tpl::parseState(ST, "ES");
        CHECK(S.ok && S.marketFound && S.px == 7127 && S.em == 159.87 && S.lad.size() == 8 && S.session == "2026-11-20" && S.nowCt == "09:43", "real state: ES px 7127, em 159.87, 8 ladder rows (decoy plan.markets / live.markets skipped)");
        CHECK(S.lad[0].code == "MQ" && S.lad[0].mqOnly && S.lad[0].zone.size() == 2 && S.lad[0].fmt == "7,550.00", "ladder row fields");
        const tpl::St N = tpl::parseState(ST, "CL"); CHECK(N.ok && !N.marketFound, "market not in the state");
        CHECK(!tpl::build(H, false, N, "CL").ok && tpl::build(H, false, N, "CL").buy.empty(), "no market = both sides '-'");
        CHECK(!tpl::parseState(ST.substr(0, ST.size() / 2), "ES").ok, "torn (half the file) = not ok");
        CHECK(!tpl::parseState("{\"markets\": {\"ES\": {\"em\": 1, \"ladder\": [ {\"price\": 1,, } ] } } }", "ES").ok, "garbled = not ok");
        CHECK(!tpl::parseState("", "ES").ok && !tpl::parseState("[]", "ES").ok, "empty / not an object = not ok");
        {   const tpl::St E = tpl::parseState("{\"session\":\"2026-11-20\",\"now_ct\":\"14:05\",\"x\":{\"markets\":{\"ES\":{\"em\":99}}},\"markets\":{\"NQ\":{\"em\":5},\"ES\":{\"price\":7100.5,\"price_live\":null,\"em\":40,\"ladder\":[{\"code\":\"ONL \\u00b7 LL\",\"price\":7102,\"price_fmt\":\"7,102.00\",\"mq_zone\":[\"Put Support 7,100\"],\"mq_only\":false,\"card\":{\"a\":[1,2,{\"b\":\"]}\\\"\"}]}}]}}}", "ES");
            CHECK(E.ok && E.em == 40 && E.px == 7100.5 && E.lad.size() == 1 && E.lad[0].code == "ONL \xB7 LL" && E.nowCt == "14:05", "escapes, \\u00b7 -> ANSI middle dot, nested decoy markets, price_live null -> price, skipped card");
            const tpl::Plan P = tpl::build(tpl::parseHtf("HTFNOW,7300,1,7300,1,7101,1,7101,1,1,7200\n"), false, E, "ES");
            CHECK(P.buy.size() == 1 && P.buy[0].tags == "ONL/LonLO+PS" && P.buy[0].target.empty() && P.time12 == "2:05 PM", "tag ONL/LonLO+PS, no level 0.25 EM beyond = no target, 2:05 PM"); }
        CHECK(!tpl::build(H, true, S, "ES").ok && tpl::build(H, true, S, "ES").buy.empty(), "stale 3-hour levels (> 14 h) = '-'");
        CHECK(!tpl::build(tpl::Htf(), false, S, "ES").ok, "no HTFNOW = '-'");
        CHECK(tpl::pxTxt(7105, "ES") == "7,105" && tpl::pxTxt(7440.5, "ES") == "7,440.5" && tpl::pxTxt(25900.25, "NQ") == "25,900.25" && tpl::pxTxt(5.5065, "HG") == "5.5065" && tpl::pxTxt(3.2, "NG") == "3.2" && tpl::pxTxt(1.10215, "EU") == "1.10215" && tpl::pxTxt(-12.5, "CL") == "-12.5", "px_txt formats");
        CHECK(tpl::t12("00:05") == "12:05 AM" && tpl::t12("12:00") == "12:00 PM" && tpl::t12("17:30") == "5:30 PM", "t12");
        const std::string line = tpl::statusLine(REAL, "htf ok ; reader ok 09:43");
        CHECK(line == "TODAY_PLAN,1.0.0,ok,time 9:43 AM,buy S1 7,105 PS0+PS NEAR T1 7,150.00,sell -,src htf ok ; reader ok 09:43\n", "status line: " + line);
    }
    std::printf("[3] the box (2.41.0: one wide SESSION section, no BUY / SELL sections)\n");
    {   auto yOf = [](long long) { return 300; }; int checked = 0, wide = 0, narrow = 0, stretched = 0;
        tpl::Plan two = REAL; { tpl::Row r; r.level = "R1 7,180"; r.tags = "AsiaHI"; r.chip = "UPTHRUST"; r.target = "7,100.00"; two.sell.push_back(r); }
        for (int m = 0; m < 6; m++) for (int W : { 600, 760, 930, 1050, 1200, 1530, 1900 }) for (double sc : { 1.0, 1.25, 1.4 }) for (int pos : { 6, 7, 8, 1 }) for (int k = 0; k < 2; k++) {
            SC = sc; const hl::Box pane = { 0, 0, W, 640 };
            const hl::View v = planView(m, k ? two : REAL); hl::Opts O;
            const std::string id = std::string(MKS[m].code) + " W" + std::to_string(W) + " sc" + std::to_string(sc) + " p" + std::to_string(pos) + " k" + std::to_string(k);
            hl::Layout L = hl::layoutAll(v, pane, yOf, MS, (int)(W * 0.75), pos, -1, nullptr, &O);
            const int h1 = L.table.b - L.table.t;
            for (int pass = 0; pass < 2; pass++) {
                if (pass == 1) { if (!hl::stretchToAxis(L, v, pane, yOf, MS, (int)(W * 0.75), pos, -1, nullptr, O, W - 2)) break; ++stretched;
                    CHECK(L.table.r == W - 3 && L.table.b - L.table.t == h1, id + ": stretched to the axis, same height"); }
                ++checked; if (!L.tableOk) continue;
                CHECK(L.table.l >= pane.l && L.table.r <= pane.r && L.table.t >= pane.t && L.table.b <= pane.b, id + ": box inside the pane");
                const std::vector<hl::Item> tx = texts(L);
                int out = 0; for (const hl::Item& it : tx) if (it.l < L.table.l || it.r > L.table.r || it.t < L.table.t || it.b > L.table.b) ++out;
                CHECK(out == 0 && overlaps(tx) == 0, id + ": every text inside the box, none overlap (" + std::to_string(out) + " out, " + std::to_string(overlaps(tx)) + " overlaps)");
                int plan = 0, sweep = 0; const hl::Item* e1 = nullptr; const hl::Item* e2 = nullptr; const hl::Item* by = nullptr; const hl::Item* pd = nullptr; int maxCellR = 0;
                for (const hl::Item& it : tx) { if (it.s == hl::PLAN_T[0] || it.s == hl::PLAN_T[1] || it.s == hl::PLAN_T3[0] || it.s == hl::PLAN_T3[1] || it.s == "Target" || it.s == "S1 7,105") ++plan;
                    if (it.s.find("SWEEPS") != std::string::npos || it.s == "Swept") ++sweep;
                    { if (it.s == "1st extreme") e1 = &it; } { if (it.s == "2nd extreme") e2 = &it; } { if (it.s == "1st extreme by") by = &it; } { if (it.s == "% of days") pd = &it; } }
                CHECK(plan == 0 && sweep == 0, id + ": no BUY AT / SELL AT and no sweep sections");
                if (!L.swOk) { ++narrow; continue; }   // the 2.12.0 header-only box (narrowest panes)
                ++wide;
                CHECK(pass == 1 || L.table.l == pane.l || L.table.l <= pane.l + 2, id + ": the box starts at the pane's left edge (" + std::to_string(L.table.l) + ")");
                CHECK(L.swCols.empty() && L.swTier == 0, id + ": one section");
                CHECK(e1 && e2 && e1->r < e2->l && e1->t == e2->t, id + ": strip names the columns 1st extreme | 2nd extreme");
                CHECK(by && pd && by->t < pd->t && L.tmRow.l > by->r, id + ": 1st-extreme table present");
                if (by && e2) { for (const hl::Item& it : tx) if (it.t > e2->t && it.l < by->l) maxCellR = (std::max)(maxCellR, it.r);
                    CHECK(maxCellR < by->l && e2->r < by->l, id + ": the 1st-extreme table sits RIGHT of the Exp / Act cells and the strip heads");
                    CHECK(by->t == e2->b || (by->t > e2->t && by->t - e2->b < 4), id + ": times on the Exp row (right under the strip)"); }
            }
        }
        std::printf("  layouts %d, wide %d, header-only (narrow) %d, stretched %d\n", checked, wide, narrow, stretched);
        CHECK(wide > 900 && stretched > 300, "wide box in most layouts, stretched to the axis");
        {   // the box is 2 rows shorter than 2.40.0's three-section box on a typical pane (ES, 1530 x 640, bottom centre): 2.40.0 height there = mktH + hdr + strip + 6 rows
            SC = 1.0; const hl::Box pane = { 0, 0, 1530, 640 }; hl::Opts O; const hl::View v = planView(0, REAL);
            const hl::Layout L = hl::layoutAll(v, pane, yOf, MS, 1150, 7, -1, nullptr, &O);
            std::printf("  ES 1530: box %d,%d,%d,%d (h %d)\n", L.table.l, L.table.t, L.table.r, L.table.b, L.table.b - L.table.t);
            CHECK(L.tableOk && L.swOk && L.table.l <= 2 && L.table.b - L.table.t <= 132, "ES 1530: left edge, height " + std::to_string(L.table.b - L.table.t) + " (2.40.0: 166)"); }
    }
    if (const char* dp = std::getenv("HL2410_DUMP")) {   // render dump for the mockup check (render_2400.py)
        SC = 1.0; auto yOf = [](long long) { return 300; }; const hl::Box pane = { 0, 0, 1530, 640 }; hl::Opts O;
        tpl::Plan two = REAL; { tpl::Row r; r.level = "R1 7,180"; r.tags = "AsiaHI"; r.chip = "UPTHRUST"; r.target = "7,100.00"; two.sell.push_back(r); }
        std::FILE* f = std::fopen(dp, "w");
        for (int k = 0; k < 2 && f; k++) { const hl::View v = planView(0, k ? two : REAL); hl::Layout L = hl::layoutAll(v, pane, yOf, MS, 1150, 7, -1, nullptr, &O);
            std::fprintf(f, "LAYOUT %d %d %d %d %d\n", k, L.table.l, L.table.t, L.table.r, L.table.b);
            for (const hl::Item& it : L.items) if (it.role == hl::R_TABLE) std::fprintf(f, "%d|%d|%d|%d|%d|%06X|%06X|%d|%d|%s\n", it.k, it.l, it.t, it.r, it.b, it.c1 & 0xFFFFFF, it.c2 & 0xFFFFFF, it.fs, it.bold ? 1 : 0, u8(it.s).c_str()); }
        if (f) std::fclose(f); }
    std::printf("[4] host + versions\n");
    {   const std::string h = rd("../HodLod.cpp"), st = rd("../HodLodSettings.h"), gb = rd("../../tools/gex-build.bat"), lg = rd("../HodLodLogic.h");
        CHECK(h.find("SWROW|") != std::string::npos && h.find("for (int sd = 0; sd < 2; sd++) for (const hl::View::SwRow& r : sd ? S->view.swLo : S->view.swHi)") != std::string::npos, "status file still carries every SWROW|HOD / LOD line (format unchanged)");
        CHECK(h.find("S->view.swOn = true; S->view.swHi.clear(); S->view.swLo.clear();") != std::string::npos && h.find("hsw::build(S->sw, S->swKeys") != std::string::npos && h.find("loadSweeps(*S);") != std::string::npos, "sweep rows still built every draw");
        CHECK(h.find("S->view.planOn = true;") != std::string::npos && h.find("loadPlan(*S);") != std::string::npos && h.find("tpl::statusLine(S->tpPlan") != std::string::npos, "host fills the plan and writes TODAY_PLAN");
        CHECK(h.find("data\\\\menthorq\\\\reader\\\\") != std::string::npos && h.find("CobSignal.status-\" + S.mkt + \"-180.txt\"") != std::string::npos, "sources: the Reader's state.json + CobSignal status");
        CHECK(h.find("now - S.tpChecked < 20") != std::string::npos && h.find("torn read (kept the last copy)") != std::string::npos, "stamp-gated (20 s), torn reads keep the last copy");
        {   // the SWROW lines in the planOn view are the sweep rows exactly
            hl::View v = planView(0, REAL); std::string sw; for (int sd = 0; sd < 2; sd++) for (const hl::View::SwRow& r : sd ? v.swLo : v.swHi) sw += std::string("SWROW|") + (sd ? "LOD" : "HOD") + "|" + r.name + "|" + r.px + "|" + r.pct + "|" + (r.elig ? "1" : "0") + "\n";
            CHECK(!v.swHi.empty() && !v.swLo.empty() && sw.find("SWROW|HOD|") == 0 && sw.find("SWROW|LOD|") != std::string::npos, "a plan view keeps its sweep rows for SWROW"); }
        CHECK(std::string(hl::HL_VERSION) == "2.46.1" && std::string(hlset::PLUGIN_VERSION) == "2.42.4" && h.find("p->setVersion(\"2.42.3\")") != std::string::npos && st.find("\"2.42.3\"") != std::string::npos && hl::HL_WIDE_SESSION, "versions 2.42.4 everywhere, wide session on");
        CHECK(std::string(tpl::VERSION) == "1.0.0" && lg.find("TODAY PLAN") != std::string::npos, "TodayPlan 1.0.0");
        { size_t a = gb.find("call :one HodLod "); const std::string l = a == std::string::npos ? std::string() : gb.substr(a, gb.find('\n', a) - a);
          CHECK(l.find(" TodayPlan.h") != std::string::npos && l.find(" GammaLine.h") != std::string::npos, "gex-build HodLod line lists TodayPlan.h"); }
    }
    std::printf("t_hl2410 (lsHodLod 2.41.3): %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
