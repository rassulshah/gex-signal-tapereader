// lsEventCal 1.0.0 logic tests (g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra)
#include "../EventCalLogic.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
using namespace ecal;
static int pass = 0;
#define OK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); return 1; } ++pass; } while (0)
static int W(const std::string& s, bool b) { return (int)s.size() * (b ? 7 : 6); }

int main(int argc, char** argv) {
    // civil time
    OK(dowOf(minAbs(2026, 11, 17, 18, 25)) == 2);                 // Tuesday
    OK(whenText(minAbs(2026, 11, 18, 13, 0)) == "Wed 1:00 PM");
    OK(whenText(minAbs(2026, 11, 19, 0, 5)) == "Thu 12:05 AM");
    const long long now = minAbs(2026, 11, 17, 18, 25);
    OK(countText(now + 18, now) == "18m"); OK(countText(now + 228, now) == "3h 48m"); OK(countText(now + 1440 + 13 * 60 + 5, now) == "1d 13h");
    OK(countText(now - 11, now) == "out 11m"); OK(countText(now, now) == "out 0m");

    // parse: the nightly's real output (written by lra.event_calendar on the staged store)
    std::string txt = "VERSION,1.0.0\nMARKET,ALL\nBUILT,2026-11-17 18:32\nEV,2026-11-17,15:30,API Crude,M,CL\nEV,2026-11-18,09:30,EIA Crude,H,CL\n"
                      "EV,2026-11-18,13:00,FOMC Minutes,H,CL\nEV,2026-11-18,13:00,FOMC Minutes,H,ES\nEV,2026-11-19,07:30,Jobless Claims,M,CL\n"
                      "EV,2026-11-19,07:30,Jobless Claims,M,ES\nEV,2026-11-19,09:30,EIA Gas,H,NG\nEV,2026-11-20,07:30,Jobless Claims,M,ES\n";
    if (argc > 1) { std::ifstream f(argv[1]); std::stringstream ss; ss << f.rdbuf(); if (!ss.str().empty()) txt = ss.str(); }
    File F = parse(txt);
    OK(F.ok); OK(argc > 1 ? F.built > 0 : F.built == minAbs(2026, 11, 17, 18, 32)); OK(F.ev.size() >= 8);
    // garbled files
    OK(!parse("").ok); OK(!parse("EV,2026-11-18,09:30,EIA Crude,H,CL\n").ok); OK(!parse("VERSION,1\nMARKET,CL\n").ok);
    { File G = parse("VERSION,1\nMARKET,ALL\nBUILT,2026-11-17 18:00\nEV,2026-13-18,09:30,X,H,CL\nEV,2026-11-18,25:30,X,H,CL\nEV,2026-11-18,09:30,X,Z,CL\nEV,2026-11-18,09:30,,H,CL\nEV,2026-11-18,9:30,Ok,H,CL\r\n");
      OK(G.ok); OK(G.ev.size() == 1); OK(G.ev[0].name == "Ok"); }

    // ES tonight: FOMC Minutes (bold, later), Jobless Claims; no CL-only rows
    std::string why; std::vector<Ev> src = source(F, "ES", now, &why);
    OK(why.find("nightly") == 0);
    std::vector<Row> R = select(src, "ES", now);
    OK(!R.empty()); OK(R[0].name == "FOMC Minutes"); OK(R[0].tier == 'H'); OK(R[0].state == ST_LATER); OK(R[0].count == "18h 35m");
    OK(R.size() <= 3); for (const Row& r : R) OK(r.name != "EIA Crude" && r.name != "API Crude");
    OK(R.size() == 2);                                            // Fri 7:30 claims is > 2 days ahead (2d 13h)
    // CL Wed 9:12 AM: EIA Crude amber; 9:41 red "out 11m"; 10:01 gone
    const long long t912 = minAbs(2026, 11, 18, 9, 12), t941 = minAbs(2026, 11, 18, 9, 41), t1001 = minAbs(2026, 11, 18, 10, 1);
    R = select(source(F, "CL", t912, nullptr), "CL", t912); OK(R[0].name == "EIA Crude"); OK(R[0].state == ST_SOON); OK(R[0].count == "18m");
    R = select(source(F, "CL", t941, nullptr), "CL", t941); OK(R[0].name == "EIA Crude"); OK(R[0].state == ST_OUT); OK(R[0].count == "out 11m");
    R = select(source(F, "CL", t1001, nullptr), "CL", t1001); OK(R[0].name == "FOMC Minutes");
    R = select(source(F, "CL", minAbs(2026, 11, 18, 10, 0), nullptr), "CL", minAbs(2026, 11, 18, 10, 0)); OK(R[0].name == "EIA Crude");   // 30 min still shown
    // stale / missing file -> built-in list
    const long long later = minAbs(2026, 11, 30, 7, 0);
    source(F, "ES", later, &why); OK(why.find("built-in") == 0); OK(why.find("older than 8 days") != std::string::npos);
    source(File(), "ES", now, &why); OK(why.find("no file") != std::string::npos);
    // built-in: Thanksgiving week - claims Wed 7:30 (dated), no Thu rule; EIA gas Wed 11:00; CL crude Wed 9:30 rule
    {   std::vector<Ev> b = builtIn("ES", minAbs(2026, 11, 23, 0, 0) / 1440, minAbs(2026, 11, 27, 0, 0) / 1440);
        int claims = 0; for (const Ev& e : b) if (e.name == "Jobless Claims") { ++claims; OK(e.t == minAbs(2026, 11, 25, 7, 30)); }
        OK(claims == 1);
        std::vector<Ev> g = builtIn("NG", minAbs(2026, 11, 23, 0, 0) / 1440, minAbs(2026, 11, 27, 0, 0) / 1440);
        int gas = 0; for (const Ev& e : g) if (e.name == "EIA Gas") { ++gas; OK(e.t == minAbs(2026, 11, 25, 11, 0)); } OK(gas == 1);
        std::vector<Ev> c = builtIn("CL", minAbs(2026, 11, 23, 0, 0) / 1440, minAbs(2026, 11, 27, 0, 0) / 1440);
        int crude = 0, api = 0; for (const Ev& e : c) { if (e.name == "EIA Crude") ++crude; if (e.name == "API Crude") ++api; } OK(crude == 1); OK(api == 1);
        std::vector<Ev> m = builtIn("CL", minAbs(2027, 1, 18, 0, 0) / 1440, minAbs(2027, 1, 22, 0, 0) / 1440);   // MLK: crude Thu 11:00 only
        crude = 0; for (const Ev& e : m) if (e.name == "EIA Crude") { ++crude; OK(e.t == minAbs(2027, 1, 21, 11, 0)); } OK(crude == 1);
        std::vector<Ev> h = builtIn("HG", minAbs(2026, 11, 29, 0, 0) / 1440, minAbs(2026, 11, 30, 0, 0) / 1440);
        bool pmi = false; for (const Ev& e : h) if (e.name == "China PMI") pmi = true; OK(pmi);
        std::vector<Ev> eu = builtIn("EU", minAbs(2026, 12, 17, 0, 0) / 1440, minAbs(2026, 12, 17, 0, 0) / 1440);
        bool ecb = false; for (const Ev& e : eu) if (e.name == "ECB") ecb = true; OK(ecb); }
    // same time: high first, at most 3 rows, no duplicates
    {   std::vector<Ev> v; Ev a; a.t = now + 100; a.name = "Jobless Claims"; a.tier = 'M'; a.mkt = "ES"; v.push_back(a);
        Ev b = a; b.name = "GDP + PCE"; b.tier = 'H'; v.push_back(b); v.push_back(b);
        Ev c = a; c.t = now + 200; v.push_back(c); Ev d = a; d.t = now + 300; v.push_back(d); Ev e = a; e.t = now - 31; v.push_back(e);
        R = select(v, "ES", now); OK(R.size() == 3); OK(R[0].name == "GDP + PCE"); OK(R[1].name == "Jobless Claims"); OK(R[2].t == now + 200); }
    // layout: 3 cells per row, widths from the widest text, box fits the cells, bold measured for H
    R = select(source(F, "ES", now, nullptr), "ES", now);
    Layout L = layout(R, W);
    OK(L.cells.size() == R.size() * 3); OK(L.h == PAD + (int)R.size() * ROW_H);
    for (const Cell& c : L.cells) { OK(c.x >= 0 && c.x + c.w <= L.w); OK(c.y >= 0 && c.y + c.h <= L.h); }
    OK(L.cells[0].col == C_DIM); OK(L.cells[1].col == C_TEXT); OK(L.cells[1].bold); OK(!L.cells[4].bold); OK(L.cells[2].right);
    OK(layout(std::vector<Row>(), W).w == 0);
    OK(L.w < 260);                                                 // compact: well under a quarter of his 3440 px chart
    // status
    const std::string s = statusText("ES", why, R, "docked right");
    OK(s.find("lsEventCal 1.0.0") == 0); OK(s.find("ROW,Wed 1:00 PM,FOMC Minutes,H,later") != std::string::npos);
    std::printf("lsEventCal logic: %d pass\n", pass);
    return 0;
}
