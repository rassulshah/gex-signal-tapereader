// test_trademanager_130.cpp -- lsTradeManager 1.3.0 (2026-10-14, Rassul approved the external-audit display list at 18:25 CT, section
// 11): READY only from his checklist (key level in play, before 11:00 AM CT, no news, limits OK); unknown / stale input = WAIT; the
// Read's DEMAND / SUPPLY no longer counts (it is not an input at all).
// g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. test_trademanager_130.cpp -o t130 && ./t130
#include "TradeGuardLayout.h"
#include "ClaimGates.h"
#include <cstdio>
#include <string>
static int n = 0, f = 0;
#define CHECK(c, m) do { ++n; if (!(c)) { ++f; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, m); } } while (0)
static tgl::ReadyIn all() { tgl::ReadyIn r; r.levelInPlay = true; r.dealerFresh = true; r.nowMin = 9 * 60 + 12; r.calendarKnown = true; r.newsNear = false; r.guardKnown = true; r.limitsOk = true; return r; }
int main()
{
    CHECK(!cgate::kShowControlWord && !cgate::kShowReadExit, "gates hidden by default (READY from the checklist, no HOLD / EXIT?)");
    CHECK(tgl::readyFromChecklist(all()), "every checklist item OK at 9:12 AM = READY");
    { tgl::ReadyIn r = all(); r.levelInPlay = false; CHECK(!tgl::readyFromChecklist(r), "no key level in play = WAIT"); }
    { tgl::ReadyIn r = all(); r.dealerFresh = false; CHECK(!tgl::readyFromChecklist(r), "stale level data = WAIT"); }
    { tgl::ReadyIn r = all(); r.nowMin = 10 * 60 + 59; CHECK(tgl::readyFromChecklist(r), "10:59 AM CT = still before the cutoff"); }
    { tgl::ReadyIn r = all(); r.nowMin = 11 * 60; CHECK(!tgl::readyFromChecklist(r), "11:00 AM CT = no new entry (his locked cutoff) = WAIT"); }
    { tgl::ReadyIn r = all(); r.nowMin = -1; CHECK(!tgl::readyFromChecklist(r), "unknown time = WAIT"); }
    { tgl::ReadyIn r = all(); r.newsNear = true; CHECK(!tgl::readyFromChecklist(r), "high-impact news near = WAIT"); }
    { tgl::ReadyIn r = all(); r.calendarKnown = false; CHECK(!tgl::readyFromChecklist(r), "calendar unknown / stale = WAIT"); }
    { tgl::ReadyIn r = all(); r.guardKnown = false; CHECK(!tgl::readyFromChecklist(r), "Topstep Guard not connected = WAIT"); }
    { tgl::ReadyIn r = all(); r.limitsOk = false; CHECK(!tgl::readyFromChecklist(r), "limits used up / warn = WAIT"); }
    CHECK(tgl::ageMinutes(1000, 1000 + 16 * 60) == 16 && tgl::ageMinutes(-1, 5000) == -1 && tgl::ageMinutes(1000, 0) == -1, "file age in minutes; missing = unknown");
    CHECK(tgl::beforeCutoff(8 * 60 + 30) && !tgl::beforeCutoff(11 * 60 + 1) && tgl::CUTOFF_MIN == 660, "cutoff 11:00 AM CT");
    std::printf("trademanager 1.3.0: %d checks, %d failures\n", n, f);
    return f ? 1 : 0;
}
