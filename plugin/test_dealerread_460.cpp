// test_dealerread_460.cpp -- lsDealerRead 4.6.0 claim gates (Rassul 2026-10-14 15:07 CT approval; peer review A / C / E). Built twice:
//   g++ ... test_dealerread_460.cpp                                    the DEFAULT (hidden): no VERDICT, no FSUM / FWIN, no OPTIONS rows,
//                                                                      no footprint "Absorbed" row; the other footprint rows kept
//   g++ -DLRA_SHOW_STRENGTH=1 -DLRA_SHOW_OPTIONS_CAUSES=1 -DLRA_SHOW_ABSORPTION_REASON=1 ...   RESTORED: everything back, unchanged
#include <cstdio>
#include <string>
#include "DealerReadGates.h"
static int failures = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++failures; std::printf("  FAIL  %s (line %d)\n", m, __LINE__); } } while (0)
int main()
{
    const std::string file =
        "VERSION|2.4\nMARKET|ES\n"
        "VERDICT|STRONG|2|2|a MenthorQ gamma floor at 6,700 under your stop absorbs the sweep; above the HVL: dealers long gamma lean against the move\n"
        "FSUM|10:00|10:15|120|30|5|0|9000|6,700.25|6,705.50|14.20|14.05\n"
        "FWIN|10:00|10:05|40|10|2|0|3000\n"
        "TR|1|10:03|Short cover|||dealers bought 300 back|O|D\n"
        "TR|1|10:03|Absorbed|||sellers hit 2,302 contracts within three ticks of the low|F|D\n"
        "TR|2|10:06|Delta|||delta -23 -> +351|F|D\n"
        "TR|3|10:06|Demand|||heavy green bar off the low|F|D\n";
    dl::Data D = dl::parseText(file);
    CHECK(D.vLabel == "STRONG" && D.hasFsum && D.fwin.size() == 1 && D.trows.size() == 4, "the file parses (before the gates)");
    drg::apply(D);
#if LRA_SHOW_STRENGTH && LRA_SHOW_OPTIONS_CAUSES && LRA_SHOW_ABSORPTION_REASON
    CHECK(D.vLabel == "STRONG" && D.vPts == 2 && D.vOf == 2 && !D.vText.empty(), "restored: VERDICT back");
    CHECK(D.hasFsum && D.fwin.size() == 1, "restored: FSUM / FWIN back");
    CHECK(D.trows.size() == 4, "restored: every Turn row back");
    std::printf("dealerread 4.6.0 gates RESTORED: %d passed, %d failed\n", passes, failures);
#else
    CHECK(!cgate::kShowStrength && !cgate::kShowOptionsCauses && !cgate::kShowAbsorptionReason, "default build: the gates are hidden");
    CHECK(D.vLabel.empty() && D.vText.empty() && D.vPts == -1 && D.vOf == 0, "hidden: no VERDICT (Strength label / points / floor text)");
    CHECK(!D.hasFsum && D.fwin.empty(), "hidden: no forced-futures FSUM / FWIN numbers");
    bool opt = false, ab = false; for (size_t i = 0; i < D.trows.size(); i++) { if (D.trows[i].sec != 'F') opt = true; if (D.trows[i].tag == "Absorbed") ab = true; }
    CHECK(!opt, "hidden: no OPTIONS-column cause");
    CHECK(!ab, "hidden: no footprint 'Absorbed' cause");
    CHECK(D.trows.size() == 2 && D.trows[0].tag == "Delta?" && D.trows[1].tag == "Demand?", "the other footprint points stay (4.7.0: marked ?)");
    dl::Data E = dl::parseText("VERSION|2.4\nMARKET|ES\nTR|1|10:03|Short cover|||dealers bought 300 back\n");
    drg::apply(E);
    CHECK(E.trows.empty(), "an older file (rows with no section = options side): hidden too");
    CHECK(cgate::isAbsorptionTag("Absorption (FP)") && cgate::isAbsorptionTag("Abs") && !cgate::isAbsorptionTag("Delta"), "absorption tags recognised");
    std::printf("dealerread 4.6.0 gates (default, hidden): %d passed, %d failed\n", passes, failures);
#endif
    return failures ? 1 : 0;
}
