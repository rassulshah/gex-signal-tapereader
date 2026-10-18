// test_dealerprofile_2120.cpp -- lsDealerProfile 2.12.0 / DealerColumnsLogic 1.1.0 (2026-10-14, Rassul approved the external-audit display
// list at 18:25 CT, section 5): every reworded hover line, before -> after, and the lines that must stay (his "dealers must BUY N").
// g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. test_dealerprofile_2120.cpp -o t2120 && ./t2120
#include "DealerColumnsLogic.h"
#include <cstdio>
static int n = 0, f = 0;
#define CHECK(c, m) do { ++n; if (!(c)) { ++f; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, std::string(m).c_str()); } } while (0)
static bool any(const dpaudit::Hover& h, const std::string& k) { for (const auto& s : h.lines) if (s.find(k) != std::string::npos) return true; return false; }
int main()
{
    CHECK(std::string(dcol::VERSION).compare(0, 2, "1.") == 0 && std::string(dcol::VERSION) >= "1.1.0", "DealerColumnsLogic 1.1.0 or later (1.2.0 = DP 2.17.0)");
    // the bar (node) hover
    dpaudit::NodeX x; x.k = 7900; x.absG = 27e6; x.hasD = true; x.d = 1200; x.dUsd = 4.7e8;
    const dpaudit::Hover w = dcol::nodeHover("ES", x, 26e6, 40e6, 7857, true, 7.2);
    CHECK(any(w, "Teal = MenthorQ model shows + net gamma here (vendor model, data 7 min old). Not a prediction it holds."), "teal bar: model, data age, not a hold forecast");
    CHECK(!any(w, "lean against") && !any(w, "dealers are net LONG") && !any(w, "price tends to stall or pin"), "before: 'dealers net LONG gamma ... lean against moves' gone");
    CHECK(any(w, "\"1.2K lots\" = futures dealers must BUY 1.2K ES contracts"), "KEEP (his call): the node text (2.16.0: '1.2K lots', colour = buy / sell) = futures dealers must BUY ...");
    CHECK(any(w, "How to use: wait for your trigger - a sweep through the exact level, then RB or CoB, before 11:00 AM CT."), "node hover: his locked trigger");
    CHECK(!any(w, "absorption") && !any(w, "can set up") && !any(w, "can run"), "no absorption / set-up / run forecast in the node hover");
    const dpaudit::Hover o = dcol::nodeHover("ES", x, -26e6, 40e6, 7857, true);
    CHECK(any(o, "Orange = MenthorQ model shows - net gamma here (vendor model, data age unknown). Not a prediction it breaks."), "orange bar: model, age unknown");
    // the badge hover: a MenthorQ level
    const dpaudit::Hover m = dcol::levelHover("ES", "CR0", "7925.00", "33%  /  50%", "900 offers resting (2.7x)", true, true);
    CHECK(any(m, "Why it matters: MenthorQ model level, about 6 to 8 min delayed. Watch price and tape here; it isn't a turn forecast."), "MQ badge: the approved Why line");
    CHECK(!any(m, "so price often stalls or turns"), "before: 'dealers ... price often stalls or turns' gone");
    CHECK(any(m, "33%  /  50%") && any(m, "A chance to REACH only - not hold or reverse."), "KEEP the touch odds + 'chance to REACH only'");
    CHECK(any(m, "How to use: wait for your trigger - a sweep through the exact level, then RB or CoB, before 11:00 AM CT."), "badge 'How to use' = the locked trigger");
    CHECK(!any(m, "absorption (A) or exhaustion (Ex?) at the level is the turn"), "before: 'absorption (A) or exhaustion (Ex?) ... is the turn' gone");
    CHECK(any(m, "HL? on the badge") && any(m, "whether they hold is unknown") && !any(m, "defending"), "HL hover: 'whether they hold is unknown', no 'defending'");
    CHECK(any(m, "today's pick ?") && any(m, "not proven"), "white border pick: MARK ?");
    CHECK(any(m, "call resistance") == false || !any(m, "dealers sell into rallies"), "levelWhat: no 'dealers sell into rallies' claim");
    CHECK(dcol::levelWhat("CR 6020").find("dealers") == std::string::npos && dcol::levelWhat("HVL").find("moves fade") == std::string::npos, "levelWhat: model wording only");
    // a key level keeps its own Why (stops gather at prior highs / lows) - not on the list
    const dpaudit::Hover k = dcol::levelHover("ES", "LonLO", "7866.25", "", "", false, false);
    CHECK(any(k, "stops and resting orders gather at prior highs / lows"), "key level Why unchanged");
    CHECK(dcol::liqBlockText(900, true) == "HL? 900" && dcol::liqBlockText(900, false) == "900", "badge block: 'HL 900' -> 'HL? 900'");
    std::printf("dealerprofile 2.12.0 hovers: %d checks, %d failures\n", n, f);
    return f ? 1 : 0;
}
