// test_bf136.cpp -- lsBookFlow 1.36.0 (2026-11-04 20:39 CT, Rassul "this is overcomplicated.. how do i make it basic" -> "option A" on bf_basic):
// the one-line header "Defending?  B 0.4x  · S 0.5x" (ratios only, no lots, no Live / time word; "Feed silent" kept), turn marks and
// level squares not drawn (BookFlow.cpp flags; the host suites tests_bf134 / tests_bf135 check the draw list), versions.
// Build (tests_bf136/): g++ -std=c++17 -O1 -Wall -Wextra -Werror -I.. test_bf136.cpp -o t && ./t
#include <cstdio>
#include <fstream>
#include <sstream>
#include "../OrderFlowPaneSettings.h"
#include "../BookFlowLogic.h"
static int P = 0, F = 0; static void check(bool c, const std::string& m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m.c_str()); } }
int main() {
    const off::TextW M = [](const std::string& s, int pt, bool bold) { return (int)(s.size() * pt * (bold ? 0.62 : 0.56) + 0.999); };
    {   off::Info I; off::InfoRow r; r.label = "Refill Effort"; r.v[0] = "+76 (0.4x)"; r.v[1] = "+77 (0.5x)"; I.rows.push_back(r);
        std::string b, s; bf::refillCells(I, b, s);
        const bf::OneLine L = bf::oneLine(b, s, "", 10, 5, 1000, M);
        check(L.ok && L.segs.size() == 3 && L.segs[0].text == "Defending?" && L.segs[1].text == "B 0.4x" && L.segs[2].text == "\xB7  S 0.5x", "the line: Defending? | B 0.4x | \xB7 S 0.5x - no lots, no Live (" + (L.segs.size() > 2 ? L.segs[1].text + " / " + L.segs[2].text : std::string("?")) + ")");
        check(L.segs[0].color == bf::LC_Q && L.segs[1].color == 0x0022C55E && L.segs[2].color == 0x00EF4444, "colours: yellow / green / red");
        const bf::OneLine S2 = bf::oneLine(b, s, "Feed silent", 10, 5, 1000, M); check(S2.ok && S2.segs.size() == 4 && S2.segs[3].text == "Feed silent", "a silent feed still says so");
        const bf::OneLine Z = bf::oneLine(b, s, "", 10, 5, 60, M); check(!Z.ok, "no room for the core: nothing drawn");
        const bf::OneLine W = bf::oneLine("+76 (0.4x)", "+77 (0.5x)", "", 10, 5, 1000, M);
        const bf::OneLine Old = bf::oneLine("+923 (0.6x)", "+534 (0.3x)", "", 10, 5, 1000, M);
        check(W.r < 10 + 1000 && Old.segs[1].text == "B 0.6x", "shorter than before: the lots are gone"); }
    {   off::Info I; std::string b, s; bf::refillCells(I, b, s); const bf::OneLine L = bf::oneLine(b, s, "", 0, 0, 1000, M);
        check(L.ok && L.segs[1].text == "B n/a" && L.segs[2].text == "\xB7  S n/a", "no data: n/a");
        check(bf::ratioCell("+1,200 (12.5x)") == "12.5x" && bf::ratioCell("-40") == "-40" && bf::ratioCell("") == "n/a" && bf::ratioCell("0.3x") == "0.3x", "ratioCell keeps the ratio only"); }
    {   std::ifstream f("../BookFlow.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string src = ss.str();
        check(src.find("static const bool BF_DRAW_TURN_MARKS = false;") != std::string::npos && src.find("if (BF_DRAW_TURN_MARKS && e->dlSet.b(dlset::K_SHOW_MARKS)") != std::string::npos, "turn marks not drawn (still decided and recorded)");
        check(src.find("static const bool BF_DRAW_SQUARES = false;") != std::string::npos && src.find("if (BF_DRAW_SQUARES && vyNow.ok") != std::string::npos && src.find("if (BF_DRAW_SQUARES && e && mn < 0") != std::string::npos, "squares not drawn and no axis room for them (still decided and recorded)");
        check(src.find("p->setVersion(\"1.36.0\")") != std::string::npos && src.find("#define BF_VERSION \"1.36.0\"") != std::string::npos, "setVersion / BF_VERSION 1.36.0"); }
    check(std::string(bf::VERSION) == "1.36.0" && std::string(ofset::BF_VERSION) == "1.36.0", "versions: bf::VERSION / ofset::BF_VERSION 1.36.0");
    std::printf("bf136: %d passed, %d failed\n", P, F); return F ? 1 : 0;
}
