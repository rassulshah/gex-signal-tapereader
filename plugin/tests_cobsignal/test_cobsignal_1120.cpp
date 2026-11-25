// test_cobsignal_1120.cpp -- lsCobSignal 1.12.0 (Rassul 2026-11-25 15:43-15:44 CT "get rid of tracking that type of cob", "cob must have at least 10"):
// a CoB is written to the record only once its ZZO count reaches 10; one under 10 writes nothing (COB / ZZO / LVP / REV); one still counting waits.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_cobsignal_1120.cpp -o t && ./t
#include "../CobSignalLogic.h"
#include <cstdio>
static int ok = 0, bad = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++bad; std::printf("FAIL %s\n", m); } }
int main() {
    C(std::string(cobs::VERSION) == "1.12.0", "version 1.12.0");
    std::vector<cobs::Mark> M(3);
    M[0].t = 1000; M[0].zDone = true; M[0].zOk = true;    // ZZO 10+
    M[1].t = 2000; M[1].zDone = true; M[1].zOk = false;   // ZZO under 10 (GC 11-25 8:18 AM)
    M[2].t = 3000; M[2].zDone = false;                    // still counting
    const std::vector<std::string> in = {
        "COB,1000,BULL,Cob,ONL,5.85,6982.5,186.9,900,980,0,0,SHOW", "ZZO,1000,BULL,11,OK,1000", "LVP,1100,BULL,1000,1050,1150,10,100,10.0,1,SHOW", "REV,1200,BULL,1000,1.40,0.50,1",
        "COB,2000,BULL,Cob,ONL/LonLO,1.93,3480,90,1900,1980,0,0,SHOW", "ZZO,2000,BULL,0,NO,0", "LVP,2100,BULL,2000,2050,2150,10,100,10.0,1,SHOW", "REV,2200,BULL,2000,1.40,0.50,1",
        "COB,3000,BEAR,Cob,LonHI,2.29,1.09,0.01,2900,2980,0,0,SHOW", "LVP,3100,BEAR,3000,3050,3150,10,100,10.0,1,SHOW",
        "COB,4000,BEAR,Cob,PDH,2.0,1,1,3900,3980,0,0,SHOW" };   // a CoB no longer on the loaded bars
    std::vector<std::string> pass, keep; cobs::gate(in, M, pass, keep);
    C(pass.size() == 4, "the ZZO 10+ CoB: its COB, ZZO, LVP and REV lines are written");
    bool any2 = false; for (const std::string& l : pass) if (cobs::lineCob(l) == 2000) any2 = true;
    C(!any2, "the under-10 CoB writes nothing (no COB, ZZO, LVP or REV line)");
    C(keep.size() == 2 && cobs::lineCob(keep[0]) == 3000 && cobs::lineCob(keep[1]) == 3000, "the CoB still counting waits (2 lines kept)");
    bool any4 = false; for (const std::string& l : pass) if (cobs::lineCob(l) == 4000) any4 = true; for (const std::string& l : keep) if (cobs::lineCob(l) == 4000) any4 = true;
    C(!any4, "a line whose CoB is not on the loaded bars is dropped");
    // the waiting CoB reaches 10 later: its lines pass
    M[2].zDone = true; M[2].zOk = true; std::vector<std::string> p2, k2; cobs::gate(keep, M, p2, k2);
    C(p2.size() == 2 && k2.empty(), "the waiting CoB passes once its count reaches 10");
    C(cobs::lineCob("LVP,1100,BULL,1000,1,2,3,4,5.0,1,SHOW") == 1000 && cobs::lineCob("ZZO,77,BEAR,3,NO,0") == 77 && cobs::lineCob("COB,x") == 0, "lineCob fields");
    std::printf("test_cobsignal_1120: %d passed, %d failed\n", ok, bad); return bad ? 1 : 0;
}
