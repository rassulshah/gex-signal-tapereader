// RA1009_test_delta_session_noon.cpp -- DeltaProfile's session record (latchBooks, 2.5.x) must clear at 17:00 CENTRAL only.
// 2.5.2 computes the session as (UTC epoch - 17 h) / 1 day, i.e. it changes at 17:00 UTC = 12:00 CDT (11:00 CST): every decided
// A and its circle recorded before noon vanished at noon - a repaint (checklist #10 / #11).
// Uses the existing Delta adapter mock (tests_delta/irtsdk.h: getLocaltime returns the stamp's wall-clock fields).
// Build from plugin/tests_delta (Linux):
//   g++ -std=c++17 -g -fsanitize=address,undefined -I. -DDELTA_SRC='"../RA1009P_DeltaProfile.cpp"' RA1009_test_delta_session_noon.cpp -o t && ./t
//   (against the current ../DeltaProfile.cpp 2.5.2 it fails 3 of 6: cleared at noon CT, NOT cleared at 17:00 CT)
#ifndef DELTA_SRC
#define DELTA_SRC "../DeltaProfile.cpp"
#endif
#include DELTA_SRC
#include <iostream>
#include <cstdlib>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const char* why) { if (ok) ++passed; else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
// 1-min bars from a wall-clock start; bar 40 carries a heavy SELLING node at 100 that a green close above decides as A
static mock::Host fixture(unsigned long wallStart, int count)
{
    mock::Host h; h.seconds = 60;
    for (int i = 0; i < count; ++i) {
        mock::Bar b; b.time = wallStart + static_cast<unsigned long>(i * 60);
        b.rows = { mock::Price(120,1,0,1), mock::Price(121,1,0,1), mock::Price(122,1,0,1), mock::Price(123,1,0,1), mock::Price(124,1,0,1) };
        b.volume = 5; b.low = 100; b.high = 125; b.open = 103; b.close = 104; h.bars.push_back(b);
    }
    h.bars[40].rows.push_back(mock::Price(100,0,10000,10000)); h.bars[40].volume += 10000;
    h.bars[40].low = 99; h.bars[40].high = 125; h.bars[40].close = 100; h.bars[40].open = 100;
    return h;
}
static void extend(mock::Host& h, int n) { mock::Bar last = h.bars.back(); for (int i = 0; i < n; ++i) { mock::Bar b = last; b.time = last.time + static_cast<unsigned long>((i + 1) * 60); h.bars.push_back(b); } }
static int ringsAt450(mock::Host& h) { int r = 0; for (const mock::Draw& d : h.draws) if (d.type == "ring" && d.x == 450) ++r; return r; }
static bool recordedA() { return !latchBooks().empty() && !latchBooks().begin()->second.v.empty() && latchBooks().begin()->second.v[0].code == "A"; }

int main()
{
    setenv("TZ", "America/Chicago", 1); tzset();               // his PC: Central time
    const unsigned long thu = 1791504000UL;                    // 2026-10-09 00:00 as wall-clock fields (CDT that day)
    {   // A decided in the morning (bars from 09:30), then the chart runs past 12:00 CT
        mock::Host h = fixture(thu + 9 * 3600 + 30 * 60, 110);  // 09:30 .. 11:19
        mock::current = &h; DeltaProfile p; p.draw();
        check(recordedA(), "fixture: the morning A is recorded");
        extend(h, 60); h.draws.clear(); p.draw();               // .. 12:19 CT
        check(recordedA(), "the A recorded at 10:10 CT is still on record after 12:00 CT");
        check(ringsAt450(h) >= 1, "its circle is still drawn after 12:00 CT (no noon repaint)");
        p.destroy(); latchBooks().clear();
    }
    {   // the record DOES clear at the real 17:00 CT session change
        mock::Host h = fixture(thu + 14 * 3600 + 30 * 60, 110); // 14:30 .. 16:19
        mock::current = &h; DeltaProfile p; p.draw();
        check(recordedA(), "fixture: the afternoon A is recorded");
        extend(h, 60); h.draws.clear(); p.draw();               // .. 17:19 CT: a new Globex session
        check(!recordedA(), "the record clears at the 17:00 CT session change");
        p.destroy(); latchBooks().clear();
    }
    {   // UTC host (the existing suites' clock): unchanged behaviour - kept through the morning, cleared at 17:00
        setenv("TZ", "UTC", 1); tzset();
        mock::Host h = fixture(thu + 9 * 3600 + 30 * 60, 110);
        mock::current = &h; DeltaProfile p; p.draw(); extend(h, 60); p.draw();
        check(recordedA(), "UTC clock: kept past noon");
        p.destroy(); latchBooks().clear();
    }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
