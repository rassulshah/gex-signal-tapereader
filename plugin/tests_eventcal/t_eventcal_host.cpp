// lsEventCal 1.0.0 host tests on the mock SDK (two charts on one DLL, placement, HOD / LOD table, file states, status files)
#include "irtsdk.h"
namespace mock { Host* current = nullptr; Feed feed; }
#include "../EventCal.cpp"
#include <cassert>
#include <sstream>
#include <cstdio>
#include <sys/stat.h>
static int pass = 0;
#define OK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); return 1; } ++pass; } while (0)
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void bars(mock::Host& h, int n) { h.bars.resize(n); for (int i = 0; i < n; ++i) h.bars[i].time = 1000 + i; }
static std::vector<mock::Draw> boxes(const mock::Host& h) { std::vector<mock::Draw> v; for (const auto& d : h.draws) if (d.type == "box") v.push_back(d); return v; }
static std::vector<mock::Draw> texts(const mock::Host& h) { std::vector<mock::Draw> v; for (const auto& d : h.draws) if (d.type == "text") v.push_back(d); return v; }
int main() {
    char tmpl[] = "/tmp/ecalXXXXXX"; const std::string up = mkdtemp(tmpl);
    setenv("USERPROFILE", up.c_str(), 1); setenv("TZ", "America/Chicago", 1); tzset();
    const std::string flex = up + "/InvestorRT/rtx/lsFlexLevels"; mkdir((up + "/InvestorRT").c_str(), 0700); mkdir((up + "/InvestorRT/rtx").c_str(), 0700); mkdir(flex.c_str(), 0700);
    // a calendar with an event 2 hours from now for ES and CL, built now
    const std::time_t tt = std::time(nullptr); struct tm lt; localtime_r(&tt, &lt);
    const long long now = ecal::minAbs(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min);
    auto fmt = [](long long t, bool time) { long long z = t / 1440; char b[32];
        z += 719468; const long long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
        const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const long long y0 = (long long)yoe + era * 400; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        const unsigned mp = (5 * doy + 2) / 153; const unsigned d = doy - (153 * mp + 2) / 5 + 1; const unsigned m = mp < 10 ? mp + 3 : mp - 9;
        if (time) std::snprintf(b, sizeof b, "%02d:%02d", (int)(t % 1440) / 60, (int)(t % 1440) % 60); else std::snprintf(b, sizeof b, "%04lld-%02u-%02u", y0 + (m <= 2), m, d); return std::string(b); };
    {   std::ofstream f((flex + "/LRA-Calendar-ALL.csv").c_str());
        f << "VERSION,1.0.0\nMARKET,ALL\nBUILT," << fmt(now, false) << " " << fmt(now, true) << "\n";
        f << "EV," << fmt(now + 120, false) << "," << fmt(now + 120, true) << ",FOMC Minutes,H,ES\n";
        f << "EV," << fmt(now + 40, false) << "," << fmt(now + 40, true) << ",EIA Crude,H,CL\n";
        f << "EV," << fmt(now - 10, false) << "," << fmt(now - 10, true) << ",API Crude,M,CL\n"; }
    mock::Host es, cl; es.root = "ES"; es.symbol = "ESZ26"; es.seconds = 180; es.chart = "ES 3m"; cl.root = "QCL"; cl.symbol = "QCLF27"; cl.seconds = 180; cl.chart = "CL 3m";
    bars(es, 120); bars(cl, 120); es.paneRight = 3400; cl.paneRight = 3400;
    mock::current = &es; EventCal* x = static_cast<EventCal*>(CreateExtension());
    // ES: one row, bold, at the top centre, left of the last bar, below the title row
    mock::current = &es; x->draw();
    auto B = boxes(es); OK(B.size() == 1); auto T = texts(es); OK(T.size() == 3);
    OK(T[1].text == "FOMC Minutes"); OK(T[1].fstyle == 1); OK(T[2].text == "2h 0m" || T[2].text == "1h 59m");
    const int lastX = 50 + 119 * es.barW; OK(B[0].r <= lastX); OK(B[0].x >= es.paneLeft); OK(B[0].y >= es.paneTop + 16);
    OK(std::abs((B[0].x + B[0].r) / 2 - (es.paneLeft + lastX) / 2) <= 2);   // centred in the area up to the last bar
    OK(B[0].border == ecal::C_EDGE && B[0].color == ecal::C_BG);
    // CL on the SAME DLL: its own rows (QCL -> CL): EIA Crude amber (40 min), API Crude red (out 10m)
    mock::current = &cl; x->draw();
    T = texts(cl); OK(T.size() == 6); OK(T[1].text == "API Crude"); OK(T[1].color == ecal::C_RED); OK(T[2].text == "out 10m");
    OK(T[4].text == "EIA Crude"); OK(T[4].color == ecal::C_AMBER); OK(T[4].fstyle == 1); OK(T[1].fstyle == 0);
    // ES again: unchanged, its own state
    es.draws.clear(); mock::current = &es; x->draw(); OK(texts(es).size() == 3);
    // the HOD / LOD table at the top centre: the box docks beside it and never overlaps it
    bdk::Box tb = bdk::mk(500, 20, 760, 120);
    bdk::publishHodLod(es.symbol, es.seconds, es.chart, (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(), tb);
    es.draws.clear(); x->draw(); B = boxes(es); OK(B.size() == 1);
    OK(!bdk::overlap(bdk::mk(B[0].x, B[0].y, B[0].r, B[0].b), tb)); OK(B[0].x >= 760); OK(B[0].y == 20);
    std::string st = slurp(flex + "/EventCal.status-ES-180.txt"); OK(st.find("lsEventCal 1.0.0") == 0); OK(st.find("SOURCE,nightly file") != std::string::npos);
    OK(slurp(flex + "/EventCal.status-CL-180.txt").find("MARKET,CL") != std::string::npos);
    // a chart so narrow the box cannot fit: nothing drawn, no crash
    mock::Host nq; nq.root = "NQ"; nq.symbol = "NQZ26"; nq.seconds = 180; bars(nq, 3); nq.paneRight = 60; mock::current = &nq; x->draw(); OK(boxes(nq).empty());
    // garbled file never replaces the good copy; then removing the file -> built-in list
    { std::ofstream f((flex + "/LRA-Calendar-ALL.csv").c_str()); f << "garbage\n"; }
    mock::current = &cl; x->clear(); x->draw();                      // fresh state: garbled + no good copy -> built-in
    OK(slurp(flex + "/EventCal.status-CL-180.txt").find("built-in list") != std::string::npos);
    std::remove((flex + "/LRA-Calendar-ALL.csv").c_str());
    mock::Host gc; gc.root = "QGC"; gc.symbol = "QGCG27"; gc.seconds = 180; gc.chart = "GC"; bars(gc, 50); gc.paneRight = 3400; mock::current = &gc; x->draw();
    st = slurp(flex + "/EventCal.status-GC-180.txt"); OK(st.find("SOURCE,built-in list (no file)") != std::string::npos);
    // chart removal frees the state
    x->clear(); OK(gc.data == nullptr);
    mock::current = &es; x->clear(); mock::current = &cl; x->clear(); mock::current = &nq; x->clear();
    delete x;
    std::printf("lsEventCal host: %d pass\n", pass);
    return 0;
}
