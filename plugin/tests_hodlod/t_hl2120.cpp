// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2120.cpp -o t2120 && ./t2120   (from plugin\tests_hodlod)
// lsHodLod 2.12.0 (2026-10-28, Rassul approved 18:22 CT "build") - the trimmed box, SDK-free:
//  [1] top line "ES [+GAMMA] Uptrend": no Q-Scores (M2 O0 V2); the version small (fs - 2, grey) at the right of the top line, clear of the trend
//  [2] no "1ST" / "2ND" header row, no "Est" row, no "Options exp" footer; Exp / Act content and colours unchanged
//  [3] a thin grey vertical separator between the 1st- and 2nd-extreme columns spanning the Exp + Act rows, never through a text
//  [4] height = header band + 2 rows (+ Pre / notes); no text overlaps; every text inside its box; 9 positions x 3 pane widths
//  [5] docked bottom-left right of the Key Levels column (ColumnBoard dockKeys): the bottom edge is the same as 2.11.0's, the top moves down
//  [6] versions: HL_VERSION == hlset::PLUGIN_VERSION == the setVersion literal == 2.12.0
#include <cstdio>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "HodLodLogic.h"
#include "HodLodGex.h"
#include "HodLodQ.h"
#include "ColumnBoard.h"
#include "SettingsSpec.h"
#include "HodLodSettings.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static int measure(const std::string& s, int fs, bool bold) { return (int)(s.size() * fs * (bold ? 0.62 : 0.56) + 0.999); }
static bool inside(const hl::Box& a, const hl::Box& o) { return a.l >= o.l && a.r <= o.r && a.t >= o.t && a.b <= o.b; }
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static hl::View shotView(bool pre, bool note) {   // his ES box 2026-10-28 18:20 CT
    hl::View v; v.has = true; v.prior = false; v.headMain = "";
    v.headLeftSegs.push_back(hl::Seg{ "ES RTH 8:30-3:00", hl::C_SLATE });
    v.headLeftSegs.push_back(hl::Seg{ "Range used 74%", hl::C_GREY });            // the old Est content: must NOT be drawn
    v.sessSegs.push_back(hl::Seg{ "Regime: Positive", hl::C_GREEN }); v.sessSegs.push_back(hl::Seg{ "Expiry 3:00 PM (in 20h 40m)", hl::C_GREY });
    v.trendReady = true; v.trendLvl = 1;
    v.E[hl::K_1ST] = hl::mkCell("8:33 - 9:18", hl::C_BLUE); v.headTail = "late-day HOD";
    v.A[hl::K_LABEL] = hl::mkCell("Actual", hl::C_SLATE);
    v.A[hl::K_1ST] = hl::mkCell("8:51", hl::C_SLATE); v.A[hl::K_1ST].s2 = "LOD"; v.A[hl::K_1ST].col2 = hl::C_RED; v.A[hl::K_1ST].s3 = "in";
    v.A[hl::K_2ND] = hl::mkCell("1:39", hl::C_SLATE); v.A[hl::K_2ND].s2 = "HOD"; v.A[hl::K_2ND].col2 = hl::C_GREEN;
    hlq::toView(v, hlq::parse("VERSION|1\nMARKET|ES\nDATE|2026-10-28\nQ|2|0|2|-\nPREV|2026-10-27|2|1|3|-\nRECV|1793230000\n", "ES"), "Uptrend", 1);
    auto gi = [](const char* st, const char* nm, bool above, bool mid) { hl::View::GexItem g; g.strike = st; g.name = nm; g.above = above; g.mid = mid; return g; };
    v.gexItems = { gi("6900", "CR", true, false), gi("6950", "G1", true, false), gi("6871.25", "", false, true), gi("6850", "HVL", false, false), gi("6800", "PS", false, false) };
    if (pre) { v.preOn = true; v.preText = "bids 61% stayed . 1st LOW early ?"; v.preCol = hl::C_GREEN; }
    if (note) { hl::View::Note n; n.label = "News"; n.text = "CPI 7:30 AM . -5 / +15 min"; n.col = hl::C_AMBER; v.notes.push_back(n); }
    return v;
}
static std::vector<hl::Item> texts(const hl::Layout& L) { std::vector<hl::Item> t; for (const hl::Item& it : L.items) if (it.k == hl::K_TEXT && !it.s.empty()) t.push_back(it); return t; }
static int overlaps(const std::vector<hl::Item>& tx) { int n = 0; for (size_t i = 0; i < tx.size(); i++) for (size_t j = i + 1; j < tx.size(); j++) { const hl::Item& a = tx[i]; const hl::Item& b = tx[j];
    if (a.t < b.b - 1 && b.t < a.b - 1 && a.l < b.r - 1 && b.l < a.r - 1) { if (++n < 4) std::printf("  overlap '%s' / '%s'\n", a.s.c_str(), b.s.c_str()); } } return n; }
static const hl::Item* find(const std::vector<hl::Item>& tx, const std::string& s) { for (const hl::Item& it : tx) if (it.s == s) return &it; return nullptr; }
static bool hasSub(const std::vector<hl::Item>& tx, const std::string& s) { for (const hl::Item& it : tx) if (it.s.find(s) != std::string::npos) return true; return false; }

int main() {
    auto yOf = [](long long) { return 300; };
    const std::string ver = std::string("v") + hl::HL_VERSION;
    std::printf("[1-3] the trimmed box\n");
    {   const hl::View v = shotView(false, false);
        hl::Layout L = hl::layoutAll(v, hl::Box{ 0, 0, 1530, 520 }, yOf, measure, 960, 6, -1, nullptr, nullptr);
        CHECK(L.tableOk, "table drawn");
        const std::vector<hl::Item> tx = texts(L);
        CHECK(find(tx, "ES") != nullptr, "top line: ES");
        CHECK(!hasSub(tx, "M2") && !hasSub(tx, "O0") && !hasSub(tx, "V2") && !hasSub(tx, "Q STALE"), "[1] no Q-Scores on the top line");
        const hl::Item* pill = find(tx, "+GAMMA"); const hl::Item* tr = nullptr; for (const hl::Item& it : tx) if (it.s.find("Uptrend") != std::string::npos) tr = &it;
        CHECK(pill && pill->c1 == hl::C_GREEN && tr, "[1] +GAMMA pill and the trend kept");
        const hl::Item* vi = find(tx, ver);
        CHECK(vi && vi->fs < (pill ? pill->fs : 99) && vi->c1 == hl::C_GREY, "[1] version " + ver + " small (fs " + std::to_string(vi ? vi->fs : -1) + ") and grey");
        CHECK(vi && pill && vi->t < L.table.t + 30 && std::abs((vi->t + vi->b) / 2 - (pill->t + pill->b) / 2) <= 2, "[1] the version on the top line");
        CHECK(vi && tr && vi->l >= tr->r + 6 && vi->r <= L.table.r - 8, "[1] the version right of the trend, inside the box");
        CHECK(!find(tx, "1ST") && !find(tx, "2ND"), "[2] no 1ST / 2ND header row");
        CHECK(!find(tx, "Est") && !hasSub(tx, "Range used"), "[2] no Est row");
        CHECK(!hasSub(tx, "Options exp") && !hasSub(tx, "3:00 PM"), "[2] no Options exp footer");
        const hl::Item* ex = find(tx, "Exp"); const hl::Item* ac = find(tx, "Act");
        CHECK(ex && ac && ac->t > ex->t, "[2] Exp then Act rows kept");
        CHECK(find(tx, "8:33 - 9:18") && find(tx, "8:33 - 9:18")->c1 == hl::C_BLUE && find(tx, "late-day HOD") && find(tx, "late-day HOD")->c1 == hl::C_BLUE, "[2] Exp content blue as before");
        CHECK(find(tx, " LOD") && find(tx, " LOD")->c1 == hl::C_RED && find(tx, " HOD") && find(tx, " HOD")->c1 == hl::C_GREEN, "[2] Act colours as before");
        // [3] the separator
        int nsep = 0; const hl::Item* sp = nullptr;
        for (const hl::Item& it : L.items) if (it.k == hl::K_LINE && it.l == it.r && it.c1 == hl::C_COLSEP) { ++nsep; sp = &it; }
        CHECK(nsep == 1 && sp && sp->w == 1, "[3] one thin (1 px) grey vertical separator");
        CHECK(sp && ex && ac && sp->t <= ex->t + 3 && sp->b >= ac->b - 3 && sp->t > L.table.t && sp->b < L.table.b, "[3] it spans the Exp + Act rows inside the box");
        int crossed = 0, leftN = 0, rightN = 0;
        for (const hl::Item& it : tx) { if (it.t >= (sp ? sp->b : 0) || it.b <= (sp ? sp->t : 0) || it.s == "Exp" || it.s == "Act") continue;
            if (sp && it.l < sp->l && it.r > sp->l) ++crossed; else if (sp && it.r <= sp->l) ++leftN; else ++rightN; }
        CHECK(sp && crossed == 0 && leftN >= 2 && rightN >= 2, "[3] 1st column left of it, 2nd right of it, no text crossed (" + std::to_string(crossed) + ")");
        int out = 0; for (const hl::Item& it : tx) { const hl::Box b = { it.l, it.t, it.r, it.b }; if (!inside(b, L.table) && !(L.gexOk && inside(b, L.gexRow))) ++out; }
        CHECK(out == 0, "[4] every text inside the table or the strip");
        CHECK(overlaps(tx) == 0, "[4] no two texts overlap");
        const int fs = ex ? ex->fs : 10, rowH = hl::lineH(fs) + 1, H = L.table.b - L.table.t;
        CHECK(H == 1 + (rowH + 4) + 1 + 2 * rowH + 3, "[4] height = header band + Exp + Act (" + std::to_string(H) + " px)");
        std::printf("  table %d x %d px (fs %d)\n", L.table.r - L.table.l, H, fs);
    }
    std::printf("[4] fuzz: positions x widths x Pre / News rows\n");
    {   int bad = 0, runs = 0;
        for (int pre = 0; pre < 2; pre++) for (int note = 0; note < 2; note++) for (int W : { 1530, 1100, 800 }) for (int pos = 0; pos < 9; pos++) {
            const hl::View v = shotView(pre, note); hl::Box pane = { 0, 0, W, 520 };
            hl::Layout L = hl::layoutAll(v, pane, yOf, measure, W - 300, pos, -1, nullptr, nullptr);
            if (!L.tableOk) { ++bad; continue; } ++runs;
            const std::vector<hl::Item> tx = texts(L);
            if (overlaps(tx)) { ++bad; continue; }
            int out = 0; for (const hl::Item& it : tx) { const hl::Box b = { it.l, it.t, it.r, it.b }; if (!inside(b, L.table) && !(L.gexOk && inside(b, L.gexRow))) ++out; }
            if (out || find(tx, "Est") || find(tx, "1ST") || hasSub(tx, "Options exp") || !find(tx, ver)) { ++bad; continue; }
            const int rowH = hl::lineH(find(tx, "Exp")->fs) + 1;
            if (L.table.b - L.table.t != 1 + rowH + 4 + 1 + (2 + pre + note) * rowH + 3) { ++bad; continue; }
            if (L.colSepX < 0 || (pre && L.colSepB > L.preRow.t)) ++bad;   // the separator stops above the Pre row
        }
        CHECK(bad == 0 && runs == 108, "[4] " + std::to_string(runs) + " layouts clean (" + std::to_string(bad) + " bad)");
    }
    std::printf("[5] docking\n");
    {   const hl::View v = shotView(false, false);
        hl::Layout L = hl::layoutAll(v, hl::Box{ 0, 0, 1530, 700 }, yOf, measure, 1200, 0, -1, nullptr, nullptr);
        int ul = L.table.l, ut = L.table.t, ur = L.table.r, ub = L.table.b;
        if (L.gexOk) { ul = (std::min)(ul, L.gexRow.l); ut = (std::min)(ut, L.gexRow.t); ur = (std::max)(ur, L.gexRow.r); }
        const cb::Dock d = cb::dockKeys(ul, ut, ur, ub, 0, 0, 700, 120);
        CHECK(d.ok && ub + d.dy == 700 - cb::DOCK_GAP - 2 && ul + d.dx == 1 + 120 + cb::DOCK_GAP, "[5] docked bottom-left right of the column, bottom at " + std::to_string(ub + d.dy));
        const int hOld = (ub - ut) + 2 * (hl::lineH(10) + 1) + (3 + 1 + hl::lineH(10) + 2) - 3;   // 2.11.0: + 1ST/2ND + Est rows + footer
        const cb::Dock dOld = cb::dockKeys(ul, ub - hOld, ur, ub, 0, 0, 700, 120);
        CHECK(dOld.ok && ub + dOld.dy == ub + d.dy && (ub - hOld) + dOld.dy < ut + d.dy, "[5] the bottom edge stays put, the top comes down (" + std::to_string(hOld - (ub - ut)) + " px shorter)");
    }
    std::printf("[6] versions\n");
    {   CHECK(std::string(hl::HL_VERSION) == "2.12.0" && std::string(hlset::PLUGIN_VERSION) == hl::HL_VERSION, "HL_VERSION == PLUGIN_VERSION == 2.12.0");
        const std::string src = readAll("../HodLod.cpp"); CHECK(src.find("p->setVersion(\"2.12.0\")") != std::string::npos, "setVersion literal 2.12.0");
    }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
