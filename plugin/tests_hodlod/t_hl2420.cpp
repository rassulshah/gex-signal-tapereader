// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2420.cpp -o t2420 && ./t2420   (from plugin\tests_hodlod)
// lsHodLod 2.42.0 (Rassul 2026-11-20 13:22 / 13:36 "perfect" / 14:13 build, mockup hodlod-signal-rows-mockup.html): the two signal rows
#include <cstdio>
#include <string>
#include <vector>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../TopSignals.h"
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
int main() {
    // TopSignals 1.2.0: numbers and Cob levels from his real files' formats
    const long long E = tsig::E1904; const long long t0 = 1795176000LL;   // 2026-11-20 local, around 9-10 AM
    std::string wy = "WY|1|NQ|180|0.21.4\n" + std::to_string(t0 + 600 + E) + "|Sw|-1|PHH|105805|R99+\n" + std::to_string(t0 + 1200 + E) + "|Sw|1|PHL|104307|R38\n" + std::to_string(t0 + 1800 + E) + "|Sw|1|ONL|1|bad\n";
    const std::vector<tsig::Sig> w = tsig::parseWy(wy, "NQ", t0 - 7200, t0 + 7200);
    CHECK(w.size() == 3 && w[0].num == "99+" && w[1].num == "38" && w[2].num.empty(), "Sw strength -> 99+ / 38 / none for a bad token");
    CHECK(tsig::strengthNum("V99+") == "99+" && tsig::strengthNum("R20") == "20" && tsig::strengthNum("X5").empty() && tsig::strengthNum("V100").empty(), "strengthNum");
    std::string cob = "# head\nCOB," + std::to_string(t0) + ",BEAR,Cob,ONH/LonHI/AsiaHI/PHH,6.68,1,1,1,1,0,0,SHOW\nZZO," + std::to_string(t0) + ",BEAR,51,OK," + std::to_string(t0) + "\nREV," + std::to_string(t0 + 300) + ",BEAR," + std::to_string(t0) + ",0.1,0.2,1\n";
    const std::vector<tsig::Sig> c = tsig::parseCob(cob, "NQ", t0 - 7200, t0 + 7200);
    CHECK(c.size() == 1 && c[0].num == "51" && c[0].lv == "ONH/LonHI", "Cob: ZZO count + the first two swept levels");
    CHECK(tsig::cobLevels("PHH") == "PHH" && tsig::cobLevels("") == "" && tsig::cobLevels("ONH/LonHI/AsiaHI") == "ONH/LonHI", "cobLevels");
    // rowOf: all fresh of a type, newest first, 60 min
    std::vector<tsig::Sig> all = w; all.insert(all.end(), c.begin(), c.end());
    { tsig::Sig o; o.mkt = "ES"; o.type = "Sw"; o.t = t0 - 4000; o.dir = 1; all.push_back(o); }
    const std::vector<tsig::Sig> r = tsig::rowOf(all, "Sw", t0 + 1900);
    CHECK(r.size() == 4 && r[0].t == t0 + 1800 && r[2].t == t0 + 600 && r[3].mkt == "ES", "(1.2.1) rowOf: Sw only, newest first, the 66 min old one KEPT (whole session)");
    CHECK(tsig::rowOf(all, "Cob", t0 + 1900).size() == 1, "rowOf Cob");
    // rowLayout: inside the row, no overlap, oldest drops first at the right
    auto meas = [](const std::string& s, int fs, bool b) { return (int)s.size() * (fs * 6 / 10 + (b ? 1 : 0)); };
    std::vector<htop::Chip> ch; const char* mk[] = { "NQ", "ES", "GC", "NQ", "HG", "CL", "NG", "6E" };
    for (int i = 0; i < 8; i++) { htop::Chip x; x.mkt = mk[i]; x.when = "9:0" + std::to_string(i); x.lv = i == 1 ? "ONH" : "PHH"; x.num = i ? "99+" : "38"; x.dir = i % 3 ? -1 : 1; ch.push_back(x); }
    for (int W : { 300, 450, 608, 800, 1200 }) for (int n = 0; n <= 8; n++) {
        std::vector<htop::Chip> cc(ch.begin(), ch.begin() + n);
        const htop::Row R = htop::rowLayout(cc, "SW", meas, 10, 10 + W, 100, 118);
        char m[96]; std::snprintf(m, sizeof m, "W%d n%d", W, n);
        CHECK(R.ok && R.shown + R.dropped == n, (std::string(m) + ": every chip shown or dropped").c_str());
        CHECK(R.r <= 10 + W, (std::string(m) + ": inside the row").c_str());
        CHECK(R.fs >= htop::FS_MIN && R.fs <= htop::FS_NORMAL, (std::string(m) + ": font 10-11").c_str());
        bool in = true; for (const htop::It& it : R.items) { if (it.l < 10 || it.r > 10 + W + 1) in = false; if (it.k != 4 && (it.t < 100 || it.b > 118)) in = false; if (it.k == 4 && (it.t < 100 || it.b > 118 || it.t > 118 || it.b < 100)) in = false; }
        CHECK(in, (std::string(m) + ": every item inside").c_str());
        // the chip tints do not overlap and keep the order (newest left)
        int lastR = -1, k = 0; bool ord = true; for (const htop::It& it : R.items) if (it.k == 0 && (it.col == htop::CH_TINT_UP || it.col == htop::CH_TINT_DN)) { if (it.l < lastR) ord = false; lastR = it.r; ++k; }
        CHECK(ord && k == R.shown, (std::string(m) + ": chips in order, no overlap").c_str());
        // the shown chips are the NEWEST (the first n - dropped)
        bool newest = true; int seen = 0; for (const htop::It& it : R.items) if (it.k == 1 && it.bold && it.col == htop::CH_WHITE) { if (seen < (int)cc.size() && it.s != cc[(size_t)seen].mkt) newest = false; ++seen; }
        CHECK(newest && seen == R.shown, (std::string(m) + ": the newest kept, oldest dropped").c_str());
        if (n && W >= 1200) CHECK(R.dropped == 0, (std::string(m) + ": wide row drops nothing").c_str());
    }
    { const htop::Row R = htop::rowLayout(std::vector<htop::Chip>(), "COB", meas, 10, 610, 100, 118); CHECK(R.ok && R.shown == 0 && R.items.size() == 1, "an empty row shows only its head"); }
    { htop::Chip u; u.mkt = "ES"; u.when = "9:03"; u.lv = "ONH/LonHI"; u.num = "51"; u.dir = 1; CHECK(htop::chipLv(u.lv) == "ONH \xB7 LonHI", "Cob levels with a middle dot"); }
    CHECK(std::string(hl::HL_VERSION) == "2.42.2" && std::string(hlset::PLUGIN_VERSION) == "2.42.2" && std::string(htop::VERSION) == "1.2.0" && std::string(tsig::VERSION) == "1.2.1", "versions");
    std::printf("t_hl2420 (lsHodLod 2.42.0): %d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
