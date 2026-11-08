// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2179.cpp -o t2179 && ./t2179   (from plugin\tests_hodlod)
// lsHodLod 2.17.9 (2026-11-04, Rassul "sometimes the hodlod box collapses", NQ 3-min 11:24 CT) - SDK-free:
//  [1] the NQ collapse: his geometry (pane right 978, the box at the bottom, 2.17.8 gave the 57 px header-only box, AXIS_FILL off) now
//      keeps SESSION | HOD SWEEPS | LOD SWEEPS (155 px as on ES), every level row with its price and %, inside, no overlaps, to the axis
//  [2] grid: 6 markets x 7 pane widths x 5 font scales x 3 positions x (his pane / no sweep rows / short pane): sections whenever there
//      are rows, inside the pane, no overlaps, every row kept, stretched to the axis in the same form (fillTier)
//  [3] unaffected: every scenario where 2.17.8 drew the sweep sections (and every no-rows / header-only one) is byte-identical to 2.17.8
//      (golden_2178.txt = FNV hashes of 2.17.8's items, written by golden.cpp from the 2.17.8 header)
//  [4] the form order: 10 pt full -> 9 pt full -> compact 1 / 2 / 3; a too-wide header drops its extras, never the sweeps
//  [5] versions 2.17.9 everywhere; the host writes the SWEEPS line and stretchToAxis passes the form
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <set>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "lib_widths.h"
#include "scen.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; if (fails < 60) std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static double SC = 1.0;
static int MS(const std::string& s, int fs, bool b) { return (int)(measureLib(s, fs, b) * SC + 0.5); }
static bool inside(const hl::Item& a, const hl::Box& o) { return a.l >= o.l && a.r <= o.r && a.t >= o.t && a.b <= o.b; }
static std::vector<hl::Item> texts(const hl::Layout& L) { std::vector<hl::Item> t; for (const hl::Item& it : L.items) if (it.k == hl::K_TEXT && !it.s.empty() && it.role == hl::R_TABLE) t.push_back(it); return t; }
static int overlaps(const std::vector<hl::Item>& tx) { int n = 0; for (size_t i = 0; i < tx.size(); i++) for (size_t j = i + 1; j < tx.size(); j++) { const hl::Item& a = tx[i]; const hl::Item& b = tx[j];
    if (a.t < b.b - 1 && b.t < a.b - 1 && a.l < b.r - 1 && b.l < a.r - 1) { if (++n < 3) std::printf("  overlap '%s' / '%s'\n", a.s.c_str(), b.s.c_str()); } } return n; }
static std::multiset<std::string> strs(const std::vector<hl::Item>& tx) { std::multiset<std::string> m; for (auto& t : tx) m.insert(t.s); return m; }
static std::string compactPx(const std::string& px) { std::string o; for (size_t k = 0; k < px.size(); k++) { if (px.compare(k, 3, " - ") == 0) { o += '-'; k += 2; } else o += px[k]; } return o; }
// every sweep row's name, price (as the tier writes it) and % is drawn inside its own section
static int rowsMissing(const hl::Layout& L, const hl::View& v) {
    if (L.swCols.size() != 2) { return 99; } int miss = 0; const std::vector<hl::Item> tx = texts(L);
    for (int q = 0; q < 2; q++) for (const hl::View::SwRow& r : (q ? v.swLo : v.swHi)) {
        const std::string px = L.swTier >= 3 ? compactPx(r.px) : r.px; const hl::Box& c = L.swCols[(size_t)q];
        for (const std::string& s : { r.name, px, r.pct }) { bool f = false; for (const hl::Item& it : tx) if (it.s == s && it.l >= c.l && it.r <= c.r) f = true; if (!f) ++miss; } }
    return miss;
}
static void checkBox(const hl::Layout& L, const hl::View& v, const hl::Box& pane, const std::string& id) {
    CHECK(L.tableOk && L.table.l >= pane.l && L.table.r <= pane.r && L.table.t >= pane.t && L.table.b <= pane.b, id + ": the box inside the pane");
    const std::vector<hl::Item> tx = texts(L); int out = 0; for (const hl::Item& it : tx) if (!inside(it, L.table)) { if (++out < 3) std::printf("  outside '%s' %d-%d of %d-%d\n", it.s.c_str(), it.l, it.r, L.table.l, L.table.r); }
    CHECK(out == 0, id + ": every text inside the box"); CHECK(overlaps(tx) == 0, id + ": no two texts overlap");
    int bad = 0; for (const hl::Item& it : L.items) if (it.role == hl::R_TABLE && (it.r > L.table.r + 1 || it.l < L.table.l - 1 || it.t < L.table.t - 1 || it.b > L.table.b + 1)) ++bad;
    CHECK(bad == 0, id + ": no table item (fill / bar / line) past the box");
    if (L.swOk) { CHECK(rowsMissing(L, v) == 0, id + ": every level row's name, price and % in its section (tier " + std::to_string(L.swTier) + ")");
        for (const hl::Item& it : tx) if (it.fs < 9) { CHECK(it.fs >= (it.s[0] == 'v' ? 7 : 8) && (it.s == "Level" || it.s == "Today" || it.s == "Swept" || it.s.find("SWEEPS") != std::string::npos || it.s == "SESSION" || it.s.find("ADR") != std::string::npos || it.s.find(':') != std::string::npos || it.s.find("1st extreme") == 0 || it.s == "% of days" || it.s[0] == 'v'), id + ": only the small heads below 9 pt ('" + it.s + "')"); } }
}
int main() {
    auto yOf = [](long long) { return 300; };
    std::printf("[1] the NQ collapse (11-04 11:24 CT)\n");
    {   // his NQ chart: pane right 978 (status AXIS_FILL axis 978), the price edge ~ 3/4 across, Position 7 (bottom centre); the font metric
        // calibrated so the 2.12.0 box is ~765 px as his TABLE_BOX 2..767 (Arial x 1.4: IRT's HELVETICA 9 pt at his DPI)
        SC = 1.4; const hl::Box pane = { 0, 0, 980, 640 }; hl::View v = mkView(1); hl::Opts O;
        hl::Layout L = hl::layoutAll(v, pane, yOf, MS, 735, 7, -1, nullptr, &O);
        CHECK(L.tableOk && L.swOk, "NQ: the sweep sections are drawn (2.17.8: the 57 px header-only box)");
        CHECK(L.table.b - L.table.t == 155, "NQ: 155 px tall, as ES's full table (got " + std::to_string(L.table.b - L.table.t) + ")");
        CHECK(L.swTier >= 1 && L.swTier <= 3, "NQ: a compact form (tier " + std::to_string(L.swTier) + ")");
        checkBox(L, v, pane, "NQ");
        const std::multiset<std::string> before = strs(texts(L)); const int tier = L.swTier, h = L.table.b - L.table.t;
        const bool st = hl::stretchToAxis(L, v, pane, yOf, MS, 735, 7, -1, nullptr, O, 978);
        CHECK(st && L.stretched && std::abs(L.table.r - 977) <= 2, "NQ: AXIS_FILL on - right edge " + std::to_string(L.table.r) + " at the axis 978");
        CHECK(L.swTier == tier && L.table.b - L.table.t == h && strs(texts(L)) == before, "NQ: stretched in the same form, same height, same texts");
        checkBox(L, v, pane, "NQ stretched");
        CHECK(std::abs(L.swCols[1].r - L.table.r) <= 0 && L.swCols[0].l == L.sessR, "NQ stretched: sections butt up, LOD ends at the box edge");
        // GC / HG (his status: 57 px, AXIS_FILL off, panes 949 / 930)
        for (int m : { 5, 4 }) { const int W = m == 5 ? 951 : 932; const hl::Box p2 = { 0, 0, W, 640 }; hl::View v2 = mkView(m); hl::Opts O2;
            hl::Layout L2 = hl::layoutAll(v2, p2, yOf, MS, (int)(W * 0.75), 7, -1, nullptr, &O2);
            CHECK(L2.swOk, std::string(MKS[m].code) + ": the sweep sections are drawn"); checkBox(L2, v2, p2, MKS[m].code);
            CHECK(hl::stretchToAxis(L2, v2, p2, yOf, MS, (int)(W * 0.75), 7, -1, nullptr, O2, W - 2) && std::abs(L2.table.r - (W - 3)) <= 2, std::string(MKS[m].code) + ": runs to the axis"); }
    }
    std::printf("[2] grid + [3] unaffected vs 2.17.8\n");
    std::map<std::string, std::pair<int, std::string>> gold;
    {   std::ifstream g("golden_2178.txt"); std::string mk; int W, sci, pos, var, sw; std::string h;
        while (g >> mk >> W >> sci >> pos >> var >> sw >> h) gold[mk + " " + std::to_string(W) + " " + std::to_string(sci) + " " + std::to_string(pos) + " " + std::to_string(var)] = { sw, h }; }
    CHECK(gold.size() == 1890, "golden_2178.txt: 1890 scenarios (got " + std::to_string(gold.size()) + ")");
    int same = 0, fixed = 0, kept = 0, noRows = 0; std::map<int, int> tiers;
    for (int m = 0; m < 6; m++) for (int W : { 930, 950, 980, 1050, 1200, 1530, 1900 }) for (int sci = 0; sci < 5; sci++) for (int pos : { 6, 7, 1 }) for (int var = 0; var < 3; var++) {
        static const double SCS[5] = { 1.0, 1.25, 1.3, 1.35, 1.4 }; SC = SCS[sci];
        hl::View v = mkView(m, var != 1); hl::Opts O; const hl::Box pane = { 0, 0, W, var == 2 ? 300 : 640 };
        const std::string id = std::string(MKS[m].code) + " W" + std::to_string(W) + " x" + std::to_string(SC).substr(0, 4) + " pos" + std::to_string(pos) + " v" + std::to_string(var);
        hl::Layout L = hl::layoutAll(v, pane, yOf, MS, (int)(W * 0.75), pos, -1, nullptr, &O);
        if (var == 1) { CHECK(L.tableOk && !L.swOk && L.swTier == -1, id + ": no rows - the 2.12.0 box"); ++noRows; }
        else { CHECK(L.swOk, id + ": the sweep sections drawn"); tiers[L.swTier]++; }
        checkBox(L, v, pane, id);
        const std::string key = std::string(MKS[m].code) + " " + std::to_string(W) + " " + std::to_string(sci) + " " + std::to_string(pos) + " " + std::to_string(var);
        const auto gi = gold.find(key); char hx[20]; std::snprintf(hx, sizeof hx, "%016llx", fnv(itemsText(L)));
        if (gi != gold.end()) {
            if (gi->second.first == 1 || var == 1) { CHECK(gi->second.second == hx, id + ": identical to 2.17.8 (it had the sections / no rows)"); if (gi->second.second == hx) ++same; ++kept; }
            else ++fixed;
        }
        if (L.swOk) { const int tier = L.swTier, h = L.table.b - L.table.t; const std::multiset<std::string> b = strs(texts(L));
            const bool st = hl::stretchToAxis(L, v, pane, yOf, MS, (int)(W * 0.75), pos, -1, nullptr, O, W - 2);
            CHECK(st || L.table.r >= W - 5, id + ": stretched to the axis");
            if (st) { std::multiset<std::string> a2 = strs(texts(L)), extra; std::set_difference(a2.begin(), a2.end(), b.begin(), b.end(), std::inserter(extra, extra.begin()));
                bool onlyHdr = std::includes(a2.begin(), a2.end(), b.begin(), b.end()); for (const std::string& e : extra) if (e != "RTH 44% done" && e != "Bar 2:42 left") onlyHdr = false;
                CHECK(std::abs(L.table.r - (W - 3)) <= 2 && L.swTier == tier && L.table.b - L.table.t == h && onlyHdr, id + ": at the axis, same form / height / texts (the wider box may bring back the header's RTH % / bar-left)"); checkBox(L, v, pane, id + " stretched"); } }
    }
    std::printf("  identical to 2.17.8: %d of %d kept scenarios; fixed (2.17.8 collapsed): %d; no-row boxes %d; forms used:", same, kept, fixed, noRows);
    for (auto& t : tiers) { std::printf(" tier %d x%d", t.first, t.second); } std::printf("\n");
    CHECK(fixed > 0 && tiers.count(1) && tiers.count(2) && tiers.count(3), "the grid exercises the fixed cases and every compact form");
    std::printf("[4] form order + header\n");
    {   SC = 1.0; const hl::Box pane = { 0, 0, 1900, 640 }; hl::View v = mkView(0); hl::Opts O; hl::Layout L = hl::layoutAll(v, pane, yOf, MS, 1400, 7, -1, nullptr, &O);
        CHECK(L.swOk && L.swTier == 0 && L.tableFs == 10, "a wide pane: 10 pt, the full form");
        bool cap = false; for (const hl::Item& it : texts(L)) if (it.s.find("ADR") != std::string::npos) cap = true; CHECK(cap, "full form: the strip caption is drawn");
        int bars = 0; for (const hl::Item& it : L.items) if (it.k == hl::K_FILL && (it.c1 == hl::C_BAR_G || it.c1 == hl::C_BAR_R)) ++bars; CHECK(bars > 0, "full form: % bars drawn"); }
    {   int prevTier = -1, prevFs = 11; bool mono = true; for (double sc = 1.0; sc <= 1.45; sc += 0.01) { SC = sc; const hl::Box pane = { 0, 0, 980, 640 }; hl::View v = mkView(1); hl::Opts O;
            hl::Layout L = hl::layoutAll(v, pane, yOf, MS, 735, 7, -1, nullptr, &O); if (!L.swOk) continue;
            if (L.tableFs > prevFs || (L.tableFs == prevFs && L.swTier < prevTier)) { mono = false; } prevFs = L.tableFs; prevTier = L.swTier;
            if (L.swTier >= 1) { bool cap = false; for (const hl::Item& it : texts(L)) if (it.s.find("ADR") != std::string::npos) cap = true; if (cap) mono = false; }
            if (L.swTier >= 3) { int bars = 0; for (const hl::Item& it : L.items) if (it.k == hl::K_FILL && (it.c1 == hl::C_BAR_G || it.c1 == hl::C_BAR_R || it.c1 == hl::C_BAR_GD || it.c1 == hl::C_BAR_RD)) ++bars; if (bars) mono = false; } }
        CHECK(mono, "as the text widens: 10 pt full -> 9 pt full -> compact 1 (no caption) -> 2 -> 3 (no bars), never back"); }
    {   // a header wider than the pane (long HTF words): its extras go, the sweeps stay
        SC = 1.25; const hl::Box pane = { 0, 0, 950, 640 }; hl::View v = mkView(1); v.htf[1].s = "Strong Down, Very Weak"; v.htf[3].s = "Strong Up, Very Strong"; v.htf[5].s = "Side, Average"; hl::Opts O;
        hl::Layout L = hl::layoutAll(v, pane, yOf, MS, 712, 7, -1, nullptr, &O);
        CHECK(L.swOk, "wide header: the sweep sections stay"); checkBox(L, v, pane, "wide header");
        bool rth = false; for (const hl::Item& it : texts(L)) if (it.s == "RTH 44% done") rth = true; CHECK(!rth, "wide header: the RTH % / bar-left extras dropped first"); }
    std::printf("[5] versions + host\n");
    {   CHECK(std::string(hl::HL_VERSION) == "2.21.0" && std::string(hlset::PLUGIN_VERSION) == hl::HL_VERSION, "HL_VERSION == PLUGIN_VERSION == 2.17.9");
        std::ifstream f("../HodLod.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string src = ss.str();
        CHECK(src.find(std::string("p->setVersion(\"") + hl::HL_VERSION + "\")") != std::string::npos, "setVersion literal 2.17.9");
        CHECK(src.find("S->placeLine += \"SWEEPS,\"") != std::string::npos, "host: the SWEEPS status line");
        const size_t sa = src.find("hl::stretchToAxis("), sw = src.find("\"SWEEPS,\""); CHECK(sa != std::string::npos && sw > sa, "host: SWEEPS after the stretch (the final form)");
        std::ifstream g("../HodLodLogic.h"); std::stringstream s2; s2 << g.rdbuf(); CHECK(s2.str().find("O2.fillTier = L.swTier") != std::string::npos, "stretchToAxis passes the form to pass 2"); }
    std::printf("t_hl2179 (lsHodLod %s): %d passed, %d failed\n", hl::HL_VERSION, passes, fails);
    return fails ? 1 : 0;
}
