// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_phr.cpp -o tphr && ./tphr   (from plugin\tests_hodlod)
// 2026-11-04 17:02 CT Rassul "update PHL and PHH to PhrL and PhrH": lsHodLod 2.18.0 sweep rows + lsDealerProfile 2.52.4 badges show PhrH / PhrL;
// the file name in LRA-KeyLevels-<MKT>.csv stays PHH / PHL and is still found (price shown); other names unchanged
#include <cstdio>
#include "../HodLodSweeps.h"
#include "../KeyLevelsLogic.h"
#include "../DealerColumnsLogic.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", m); } } while (0)
int main() {
    const std::string keys = "KEYLV,1,1793833043\nL|PHL|7658.75|0\nL|ONH/PHH|7692.50|0\nL|PDL|7576.50|0\nL|ONL/AsiaLO|7490.00|0\n";
    std::vector<hsw::Lv> lv; CHECK(hsw::parseKeys(keys, 1793833100, 80LL * 3600, lv), "key file parsed");
    hsw::File F; std::vector<hsw::Row> hi, lo; hsw::build(F, lv, 0, 2, hi, lo);
    CHECK(!hi.empty() && hi[0].name == "PhrH" && !lo.empty() && lo[0].name == "PhrL", "sweep rows named PhrH / PhrL");
    CHECK(hi[0].px.find("7,692.50") != std::string::npos && lo[0].px.find("7,658.75") != std::string::npos, "their prices still found from the PHH / PHL file names");
    bool other = false; for (auto& r : lo) if (r.name == "PDL") other = true; CHECK(other, "other names unchanged");
    CHECK(klv::displayName("PHH") == "PhrH" && klv::displayName("PHL") == "PhrL", "badge PhrH / PhrL");
    CHECK(klv::displayName("ONH/PHH") == "ONH \xB7 PhrH" && klv::displayName("PHL/LonLO") == "PhrL \xB7 LonLO", "merged badges");
    CHECK(klv::displayName("ONH/AsiaHI") == "ONH \xB7 AsiaHI" && klv::displayName("PDH") == "PDH" && klv::displayName("PHHX") == "PHHX", "others unchanged");
    CHECK(dcol::isKeyLevel("PhrH") && dcol::isKeyLevel("PhrL") && dcol::isKeyLevel("PHH"), "still key levels");
    std::printf("t_phr: %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
