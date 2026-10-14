// test_dealerread_470.cpp -- lsDealerRead 4.7.0 (2026-10-14 18:25 CT, Rassul approved the external-audit display list, section 2).
// Default build = the approved display; -DLRA_SHOW_CONTROL_WORD=1 -DLRA_SHOW_ANALYST=1 = the restore path (everything back).
#include <cstdio>
#include <string>
#include "DealerReadGates.h"
static int failures = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++failures; std::printf("  FAIL  %s (line %d)\n", m, __LINE__); } } while (0)
int main()
{
    const std::string file =
        "VERSION|2.4\nMARKET|ES\n"
        "TURN|L|DEMAND 3 of 4 \xB7 LONG at PDL 6,012 \xB7 low 9:12 \xB7 T1 6,030|09:09|09:15\n"
        "TR|1|10:03|Demand|||heavy green bar off the low|F|D\n"
        "TR|2|10:06|RevBar|||a reversal bar closed back above PDL|F|D\n"
        "TR|2|10:06|Tr?|||trapped sellers under the low|F|D\n"
        "SIG|2026-10-14 10:03:00|Dem|B|1|heavy green bar\n"
        "SIG|2026-10-14 10:03:00|Van|B|1|vanna\n"
        "SIG|2026-10-14 10:06:00|oSh|B|1|short cover\n"
        "SIG|2026-10-14 10:06:00|Abs|B|1|absorbed\n"
        "SIG|2026-10-14 10:09:00|Tr|B|1|trapped\n";
    dl::Data D = dl::parseText(file);
    CHECK(D.hasTurn && D.turnHead.find("DEMAND 3 of 4") != std::string::npos, "the file parses with the control word (before)");
    D.aMag = "Magnet: 6,040 pins"; D.aDeal = "Dealers long"; D.aNow = "now"; D.aBias = "bias up"; D.scn.resize(3);
    drg::apply(D);
#if LRA_SHOW_CONTROL_WORD && LRA_SHOW_ANALYST
    CHECK(D.turnHead.compare(0, 13, "DEMAND 3 of 4") == 0, "restored: control word back");
    CHECK(!D.aMag.empty() && D.scn.size() == 3, "restored: Analyst text back");
    std::printf("dealerread 4.7.0 RESTORED: %d passed, %d failed\n", passes, failures);
#else
    // before -> after: "DEMAND 3 of 4 . LONG at PDL 6,012 . low 9:12 . T1 6,030" -> "LONG at PDL 6,012 . low 9:12 . T1 6,030"
    CHECK(D.turnHead == "LONG at PDL 6,012 \xB7 low 9:12 \xB7 T1 6,030", "control word + tally HIDDEN, the header facts kept");
    CHECK(drg::stripControl("SUPPLY? 2 of 3 - SHORT at PDH 6,050 - high 10:01") == "SHORT at PDH 6,050 - high 10:01", "SUPPLY? n of m hidden (' - ' separator)");
    CHECK(drg::stripControl("BALANCED (1 vs 1) - ES - last 90 min") == "ES - last 90 min", "BALANCED (n vs m) hidden");
    CHECK(drg::stripControl("DEMAND 3 of 4") == "", "a header that is only the control word -> empty");
    CHECK(drg::stripControl("LONG at PDL 6,012 - low 9:12") == "LONG at PDL 6,012 - low 9:12", "a header with no control word is unchanged");
    CHECK(D.aMag.empty() && D.aDeal.empty() && D.aNow.empty() && D.aBias.empty() && D.scn.empty(), "Magnet / Pinned / Analyst text HIDDEN");
    // FUTURES chips: Demand -> Demand?, RevBar -> RevBar?, Tr? stays Tr? (no double ?)
    CHECK(D.trows.size() == 3 && D.trows[0].tag == "Demand?" && D.trows[1].tag == "RevBar?" && D.trows[2].tag == "Tr?", "FUTURES chips marked ? (no double ?)");
    CHECK(drg::contextText("heavy green bar off the low") == "heavy green bar off the low - context", "a footprint point ends ' - context'");
    CHECK(drg::contextText("x - context") == "x - context" && drg::contextText("") == "", "context tail added once, never to an empty point");
    {   // the 2-line cut keeps the tail: width = characters
        std::vector<std::string> ln = { "sellers hit 2,302 contracts within", "three ticks of the low and then the..." };
        const std::string full = "sellers hit 2,302 contracts within three ticks of the low and then the bar closed green - context";
        drg::keepTail(ln, full, 40, [](const std::string& q) { return (int)q.size(); });
        CHECK(ln.size() == 2 && ln[1].size() <= 40 && ln[1].size() >= 13 && ln[1].compare(ln[1].size() - 13, 13, "... - context") == 0, "the cut point still ends '... - context'");
    }
    // REASONS row: Dem -> Dem?, Tr -> Tr?; Van / oSh (options) and Abs (absorption) gone
    std::string codes; for (size_t i = 0; i < D.sigs.size(); i++) codes += D.sigs[i].code + " ";
    CHECK(codes == "Dem? Tr? ", "no-level REASONS: options codes + Abs hidden, footprint codes marked ?");
    CHECK(cgate::isOptionsCode("oSh") && cgate::isOptionsCode("Van") && cgate::isOptionsCode("CallB?") && !cgate::isOptionsCode("Dem") && !cgate::isOptionsCode("Tr") && !cgate::isOptionsCode("Ex"), "options codes recognised");
    std::printf("dealerread 4.7.0 audit display (default): %d passed, %d failed\n", passes, failures);
#endif
    return failures ? 1 : 0;
}
