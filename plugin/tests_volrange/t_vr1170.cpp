// VolRangeLogic 1.17.0 - the BI (Break In) from the price chart's marks file on both panes; VOB / VOT (Sw) and AR lines not marked any more
#include "../VolRangeLogic.h"
#include <cstdio>
static int f = 0, p = 0; static void chk(bool c, const char* w) { std::printf("%s %s\n", c ? "PASS" : "FAIL", w); c ? ++p : ++f; }
int main() {
    const std::string txt = "WY|1|ES|180|0.43.0\n100|Bk|1|PDH|29025|V12\n101|Sw|-1|ONH|29000|V99+\n102|AR|1||0|R40\n103|BI|1|ONL|28954|R99+\n104|BI|-1|ONH|29000|V25\n105|Cx|1|PDL|28930|\n106|BI|7|ONL|1|R20\n";
    std::map<long long, vrr::Mk> w; std::map<long long, int> bi; const int n = vrr::parseWy(txt, w, &bi);
    chk(n == 1, "the count = Bk lines only (the Sw line is not read)");
    chk(w.count(100) && w[100].code == vrr::MK_BK, "Bk still read");
    chk(!w.count(101), "Sw (VOB / VOT) not marked");
    chk(!bi.count(102) && !w.count(102), "AR not marked");
    chk(bi.size() == 2 && bi[103] == 1 && bi[104] == -1, "BI lines read with their direction; a bad direction dropped");
    chk(w.count(105) && w[105].code == vrr::MK_P, "Cx still read");
    chk(std::string(vrr::mkText(vrr::MK_AR)) == "BI", "the mark reads BI");
    chk(std::string(vrr::VERSION) >= "1.17.0", "VolRangeLogic 1.17.0+");
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", p, f); return f ? 1 : 0;
}
