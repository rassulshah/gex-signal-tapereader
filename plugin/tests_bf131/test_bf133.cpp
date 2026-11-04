// test_bf133.cpp -- lsBookFlow 1.33.0 (2026-11-03, Rassul "go" on bf_box.png): the 4-line info box -> ONE compact line (bf::oneLine), SDK-free.
//   yellow "Defending?", green "B +923 0.6x", red "S +534 0.3x", small white state word; same numbers as the Refill Effort row (netCell);
//   one text row high; never past the pane's right edge (the state word drops first, no room for the core = nothing); versions.
// Build (from tests_bf131/): g++ -std=c++17 -O1 -Wall -Wextra -Werror -I.. test_bf133.cpp -o /tmp/t_bf133 && /tmp/t_bf133
#include <cstdio>
#include <string>
#include "../SignalBoard.h"
#include "../OrderFlowPaneSettings.h"
#include "../BookFlowLogic.h"
static int P = 0, F = 0; static void check(bool c, const std::string& m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m.c_str()); } }
int main() {
    const off::TextW M = [](const std::string& s, int pt, bool bold) { return (int)(s.size() * pt * (bold ? 0.62 : 0.56) + 0.999); };
    {   off::Info I; off::InfoRow r; r.label = "Refill Effort"; r.v[0] = "+923 (0.6x)"; r.v[1] = "+534 (0.3x)"; I.rows.push_back(r);
        std::string b, s; bf::refillCells(I, b, s);
        const bf::OneLine L = bf::oneLine(b, s, "Live", 10, 5, 1000, M);
        check(L.ok && L.segs.size() == 4 && L.segs[0].text == "Defending?" && L.segs[1].text == "B +923 0.6x" && L.segs[2].text == "S +534 0.3x" && L.segs[3].text == "Live", "the line: Defending? | B +923 0.6x | S +534 0.3x | Live");
        check(L.segs[0].color == bf::LC_Q && L.segs[1].color == 0x0022C55E && L.segs[2].color == 0x00EF4444 && L.segs[3].color == 0x00FFFFFF && L.segs[3].pt < L.segs[1].pt && !L.segs[3].bold, "colours: yellow / green / red / small white");
        check(L.l == 10 && L.t == 5 && L.b - L.t == bf::LINE_H && bf::LINE_H == off::QUESTION_H && L.r <= 1000, "one text row high (= the pane's question strip), at the given corner");
        bool order = true; for (size_t k = 1; k < L.segs.size(); ++k) order = order && L.segs[k].x >= L.segs[k - 1].x + L.segs[k - 1].w + bf::LINE_GAP; check(order && L.segs[0].x == 10 + bf::LINE_PAD && L.r == L.segs.back().x + L.segs.back().w + bf::LINE_PAD, "parts left to right, gaps kept, backing just behind the text");
        const int full = L.r;
        const bf::OneLine N = bf::oneLine(b, s, "Live", 10, 5, full - 5, M);
        check(N.ok && N.segs.size() == 3 && N.dropped == 1 && N.r <= full - 5, "a narrow pane drops the state word first");
        const bf::OneLine Z = bf::oneLine(b, s, "Live", 10, 5, 80, M);
        check(!Z.ok && Z.segs.empty(), "no room for the core: nothing drawn");
        const bf::OneLine T = bf::oneLine(b, s, "10:24 AM", 10, 5, 1000, M); check(T.ok && T.segs[3].text == "10:24 AM", "a candle's time as the state word");
        const bf::OneLine S2 = bf::oneLine(b, s, "Feed silent", 10, 5, 1000, M); check(S2.ok && S2.segs[3].text == "Feed silent", "the silent feed as the state word"); }
    {   off::Info I; std::string b, s; bf::refillCells(I, b, s); const bf::OneLine L = bf::oneLine(b, s, "n/a", 0, 0, 1000, M);
        check(L.ok && L.segs[1].text == "B n/a" && L.segs[2].text == "S n/a", "no Refill Effort row / no data: n/a");
        check(bf::compactCell("-40") == "-40" && bf::compactCell("") == "n/a" && bf::compactCell("+1,200 (12.5x)") == "+1,200 12.5x", "compactCell drops only the parentheses"); }
    {   off::PaneBar pb; pb.ok = true; pb.r.addC[0] = 1000; pb.r.pullC[0] = 77; pb.r.addC[1] = 600; pb.r.pullC[1] = 66;
        bf::NetStat ns; ns.ok = true; ns.ratioOk[0] = ns.ratioOk[1] = true; ns.ratio[0] = 0.6; ns.ratio[1] = 0.3;
        const off::Info I = bf::info(&pb, off::Params(), &ns, "ES"); std::string b, s; bf::refillCells(I, b, s);
        check(b == bf::netCell(pb, 0, &ns) && s == bf::netCell(pb, 1, &ns), "the line's cells = the old box's Refill Effort cells (" + b + " | " + s + ")");
        const bf::OneLine L = bf::oneLine(b, s, "Live", 0, 0, 1000, M);
        check(L.segs[1].text == "B " + bf::compactCell(bf::netCell(pb, 0, &ns)) && L.segs[2].text == "S " + bf::compactCell(bf::netCell(pb, 1, &ns)), "numbers identical to before for the same inputs: " + L.segs[1].text + " / " + L.segs[2].text); }
    check(std::string(bf::VERSION).compare(0, 6, "1.33.0") == 0 && std::string(ofset::BF_VERSION) == "1.33.0", "versions: bf::VERSION / ofset::BF_VERSION 1.33.0");
    std::printf("bf133: %d passed, %d failed\n", P, F); return F ? 1 : 0;
}
