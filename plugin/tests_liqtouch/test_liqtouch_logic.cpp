// test_liqtouch_logic.cpp -- LiqTouch.h 1.0.0 logic: the per-market defaults (tier, far-king wait, NG no churn rule), the params file
// (good / wrong market / garbled / missing rows / out of range / too big), the tag + vote rules on hand-made touches (STAYS+, STAYS, weak,
// GONE, GONE+, MID all-in attack, defenders also attacking), colour by meaning, NO REPAINT (a decided touch never changes; a later trade
// at the price is a new touch only after the level re-qualifies), the outcome (HELD / BROKE / EXPIRED), a long feed gap.
// From plugin\:  g++ -std=c++17 -O1 -Wall -Wextra -I. tests_liqtouch/test_liqtouch_logic.cpp -o tltl && ./tltl
#include "LiqTouch.h"
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static ltc::Touch T(long long szT, long long a, long long e, long long p, long long s, int dwell, long long g, long long opp, long long pull60 = 0, long long mx = 100, int side = ltc::BID) {
    ltc::Touch t; t.side = side; t.szT = szT; t.a15 = a; t.e15 = e; t.p15 = p; t.s15 = s; t.dwell15 = dwell; t.g15 = g; t.opp15 = opp; t.pull60 = pull60; t.szMax = mx; return t; }
int main() {
    const char* M[] = { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" }; const long long tier[] = { 72, 12, 16, 10, 10, 30, 128 }; const int fw[] = { 60, 300, 300, 300, 60, 60, 0 };
    for (int i = 0; i < 7; ++i) { const ltc::Params p = ltc::defaults(M[i]); CHECK(p.tier[0] == tier[i] && p.tier[1] == tier[i] && p.farWait[1] == fw[i] && (i == 5 ? p.churnCut == 0 : p.churnCut == 15), M[i]); }
    {   ltc::Params p = ltc::parse("VERSION,1\nMARKET,NQ\nTIER,RTH,14\nTIER,ON,9\nFARWAIT,RTH,240\nFARWAIT,ON,120\nDWELL_PLUS,3\nDWELL_WEAK,13\nCHURN,12\nFIT,2026-10-28,412\n", "NQ");
        CHECK(p.fromFile && p.tier[1] == 14 && p.tier[0] == 9 && p.farWait[1] == 240 && p.dwellPlus == 3 && p.dwellWeak == 13 && p.churnCut == 12, "params file read"); }
    CHECK(!ltc::parse("VERSION,1\nMARKET,ES\nTIER,RTH,14\nTIER,ON,9\n", "NQ").fromFile, "wrong market = defaults");
    CHECK(!ltc::parse("MARKET,NQ\nVERSION,1\nTIER,RTH,14\nTIER,ON,9\n", "NQ").fromFile, "VERSION must come first");
    CHECK(!ltc::parse("VERSION,1\nMARKET,NQ\nTIER,RTH,14\n", "NQ").fromFile, "both TIER rows needed");
    CHECK(!ltc::parse("VERSION,1\nMARKET,NQ\nTIER,RTH,abc\nTIER,ON,9\n", "NQ").fromFile, "garbled number = defaults");
    CHECK(!ltc::parse("VERSION,1\nMARKET,NQ\nTIER,RTH,0\nTIER,ON,9\n", "NQ").fromFile, "out of range = defaults");
    CHECK(!ltc::parse("VERSION,1\nMARKET,NQ\nTIER,RTH,14\nTIER,ON,9\nDWELL_PLUS,9\nDWELL_WEAK,5\n", "NQ").fromFile, "DWELL_PLUS under DWELL_WEAK");
    CHECK(!ltc::parse(std::string(70000, 'x'), "NQ").fromFile && ltc::parse(std::string(70000, 'x'), "NQ").tier[1] == 12, "too big = defaults (NQ 12)");
    CHECK(ltc::parse("VERSION,1\nMARKET,6E\nTIER,RTH,130\nTIER,ON,120\n", "EU").fromFile, "6E file for the EU chart");
    const ltc::Params ES = ltc::defaults("ES"), NG = ltc::defaults("NG");
    { ltc::Touch t = T(100, 60, 50, 10, 80, 2, 50, 50); ltc::decide(ES, t); CHECK(t.tag == ltc::T_STAYS_PLUS && t.vote == 2, "STAYS+ +2 bullish (bid)"); }
    { ltc::Touch t = T(100, 60, 50, 10, 80, 2, 50, 50, 0, 100, ltc::ASK); ltc::decide(ES, t); CHECK(t.tag == ltc::T_STAYS_PLUS && t.vote == -2, "offer STAYS+ = -2 (bearish)"); }
    { ltc::Touch t = T(100, 60, 50, 10, 80, 8, 50, 50); ltc::decide(ES, t); CHECK(t.tag == ltc::T_STAYS && t.vote == 1, "STAYS +1"); }
    { ltc::Touch t = T(100, 60, 50, 10, 80, 15, 50, 50); ltc::decide(ES, t); CHECK(t.tag == ltc::T_STAYS_WEAK && t.vote == 0, "weak 0"); }
    { ltc::Touch t = T(100, 60, 50, 10, 80, 8, 20, 60); ltc::decide(ES, t); CHECK(t.tag == ltc::T_STAYS && t.vote == 2, "STAYS + defenders also attacking = +2"); }
    { ltc::Touch t = T(100, 1600, 50, 10, 80, 2, 50, 50); ltc::decide(ES, t); CHECK(t.tag == ltc::T_STAYS, "churn 16x: not STAYS+ in ES"); ltc::Touch u = t; ltc::decide(NG, u); CHECK(u.tag == ltc::T_STAYS_PLUS, "NG: no churn rule"); }
    { ltc::Touch t = T(100, 30, 80, 40, 10, 6, 50, 50, 10, 100); ltc::decide(ES, t); CHECK(t.tag == ltc::T_GONE && t.vote == -1, "GONE bid = -1"); }
    { ltc::Touch t = T(100, 10, 80, 40, 10, 6, 50, 50, 10, 100); ltc::decide(ES, t); CHECK(t.tag == ltc::T_GONE_PLUS && t.vote == -2, "GONE+ (refill < 25%) bid = -2"); }
    { ltc::Touch t = T(100, 30, 80, 40, 10, 6, 50, 50, 150, 100); ltc::decide(ES, t); CHECK(t.tag == ltc::T_GONE_PLUS, "GONE+ (pulled more than its size in the last minute)"); }
    { ltc::Touch t = T(100, 30, 80, 40, 40, 6, 90, 5); ltc::decide(ES, t); CHECK(t.tag == ltc::T_MID && t.vote == -2, "MID + all-in attack = -2 (no tag drawn)"); CHECK(!ltc::drawnTag(t.tag), "MID not drawn"); }
    CHECK(ltc::tagColour(ltc::T_STAYS, ltc::BID) == 1 && ltc::tagColour(ltc::T_STAYS, ltc::ASK) == -1 && ltc::tagColour(ltc::T_GONE, ltc::ASK) == 1 && ltc::tagColour(ltc::T_GONE_PLUS, ltc::BID) == -1 && ltc::tagColour(ltc::T_STAYS_WEAK, ltc::ASK) == 0, "colour by meaning");
    // engine: qualify, touch, decide ONCE, outcome; a second trade at the price is not a new touch until it re-qualifies
    {   ltc::Engine E(ES); E.hostAtr = 24; const long long T0 = 1000000000000LL;
        E.onEvent(ltc::NEW, ltc::BID, 30752, 150, 10, T0); E.onEvent(ltc::NEW, ltc::ASK, 30760, 400, 12, T0);
        for (int s = 0; s < 40; s += 2) E.onEvent(ltc::FILL, ltc::ASK, 30760, 1, 12, T0 + s * 1000);
        CHECK(E.followed() == 1, "followed after 30 s at the tier");
        E.onEvent(ltc::FILL, ltc::BID, 30752, 30, 10, T0 + 41000); E.onEvent(ltc::FILL, ltc::BID, 30752, 5, 10, T0 + 41500);
        E.onEvent(ltc::NEW, ltc::BID, 30752, 40, 20, T0 + 42000);
        for (int s = 43; s < 60; ++s) E.onEvent(ltc::FILL, ltc::ASK, 30760, 1, 12, T0 + s * 1000);
        std::vector<ltc::Engine::Rec> R = E.drain(); int nP = 0, nT = 0; ltc::Touch d; for (auto& r : R) { if (r.kind == 'P') ++nP; if (r.kind == 'T') { ++nT; d = r.t; } }
        CHECK(nP == 1 && nT == 1, "one touch, decided once (the second trade at the price is the same touch)");
        CHECK(d.tag == ltc::T_STAYS_PLUS && d.szT == 150 && d.e15 == 35 && d.a15 == 40, "STAYS+: 150 at the touch, 35 eaten, 40 added");
        for (int s = 60; s < 80; ++s) E.onEvent(ltc::FILL, ltc::ASK, 30766, 1, 99, T0 + s * 1000);   // 14 ticks up >= 0.5 ATR (12)
        R = E.drain(); int nO = 0; for (auto& r : R) if (r.kind == 'O') { ++nO; CHECK(r.t.outcome == ltc::O_HELD, "HELD"); }
        CHECK(nO == 1, "the outcome once");
        for (int s = 80; s < 800; s += 5) E.onEvent(ltc::FILL, ltc::ASK, 30766, 1, 99, T0 + s * 1000);
        R = E.drain(); int again = 0; for (auto& r : R) if (r.kind == 'T' || r.kind == 'O') ++again;
        CHECK(again == 0, "no repaint: nothing about the decided touch is ever re-emitted"); }
    {   ltc::Engine E(ES); E.hostAtr = 24; const long long T0 = 2000000000000LL;
        E.onEvent(ltc::NEW, ltc::BID, 30752, 150, 10, T0); E.onEvent(ltc::NEW, ltc::ASK, 30760, 400, 12, T0);
        for (int s = 0; s < 40; s += 2) E.onEvent(ltc::FILL, ltc::ASK, 30760, 1, 12, T0 + s * 1000);
        E.onEvent(ltc::FILL, ltc::BID, 30752, 120, 10, T0 + 41000); E.onEvent(ltc::DEL, ltc::BID, 30752, 0, 10, T0 + 41100);
        for (int s = 42; s < 56; ++s) E.onEvent(ltc::FILL, ltc::BID, 30751, 1, 50 + s, T0 + s * 1000);
        for (int s = 56; s < 70; ++s) E.onEvent(ltc::FILL, ltc::BID, 30745, 1, 50 + s, T0 + s * 1000);   // 7 ticks through >= 0.25 ATR (6)
        std::vector<ltc::Engine::Rec> R = E.drain(); int tag = -1, oc = -1; for (auto& r : R) { if (r.kind == 'T') tag = r.t.tag; if (r.kind == 'O') oc = r.t.outcome; }
        CHECK(tag == ltc::T_GONE || tag == ltc::T_GONE_PLUS, "eaten away: GONE"); CHECK(oc == ltc::O_BROKE, "BROKE"); }
    {   ltc::Engine E(ES); E.hostAtr = 24; const long long T0 = 3000000000000LL;
        E.onEvent(ltc::NEW, ltc::BID, 30752, 150, 10, T0); for (int s = 0; s < 40; s += 2) E.onEvent(ltc::FILL, ltc::ASK, 30760, 1, 12, T0 + s * 1000);
        E.advance(T0 + 3600000LL); CHECK(E.nGap == 1 && E.followed() == 0, "a long feed gap: followed levels forgotten, the clock restarts"); }
    std::printf("LiqTouch %s logic: %d passed, %d failed\n", ltc::VERSION, passes, fails);
    return fails ? 1 : 0;
}
