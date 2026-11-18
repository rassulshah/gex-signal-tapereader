// t_sdmo_host.cpp -- lsSDMO 0.2.0 on the mock host: two charts on one DLL, lines green / red inside the pane, the read right of the last
// candle (or top-left when scrolled back), the status file, no repaint (a replay bar by bar = one straight pass), a symbol change resets.
// Build (tests_sdmo/): g++ -std=c++17 -g -fsanitize=address,undefined -Wall -Wextra -Wno-misleading-indentation -DOFF_TEST_CLOCK -I.. -I../tests_aggbook/mock t_sdmo_host.cpp -o th && ./th
#include "SDMO.cpp"
namespace mock { Host* current = nullptr; Feed feed; }
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, std::string(m).c_str()); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static SDMO* make() { SDMO* p = static_cast<SDMO*>(CreateExtension()); p->setup(); return p; }
static void dispose(SDMO* p) { p->destroy(); delete p; }
int main() {
    char tmpl[] = "/tmp/sdmoXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    std::string flex = std::string(tmp) + "/InvestorRT/rtx/lsFlexLevels"; mkdir((std::string(tmp) + "/InvestorRT").c_str(), 0700); mkdir((std::string(tmp) + "/InvestorRT/rtx").c_str(), 0700); mkdir(flex.c_str(), 0700);
    std::vector<mock::Bar> all; { std::ifstream f("es.psv"); std::string ln; unsigned long t0 = 1795000000UL - 1795000000UL % 86400; while (std::getline(f, ln)) { long long s; double h, l, c; std::sscanf(ln.c_str(), "%lld|%lf|%lf|%lf", &s, &h, &l, &c); mock::Bar b; b.time = t0 + (unsigned long)s; b.high = (float)h; b.low = (float)l; b.close = (float)c; b.open = (float)c; b.volume = 100; all.push_back(b); } }
    CHECK(all.size() > 150, "fixture loaded");
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ6"; h.root = "ES"; h.paneTop = 0; h.paneBottom = 200; h.paneLeft = 0; h.paneRight = 2600; h.ppb = 10;
    h.bars = all; h.visFrom = 0; h.visTo = (int)all.size() - 1;
    // (0.2.0) the Reversal Read letters file: two I's (one up, one down) and an E that must not show
    mkdir((flex + "/RevRead").c_str(), 0700);
    { std::ofstream f((flex + "/RevRead/wy-ES-180.txt").c_str()); f << "WY|1|ES|180|0.17.5\n" << all[150].time << "|I|1\n" << all[160].time << "|I|-1\n" << all[170].time << "|E|1\n"; }
    SDMO* p = make(); p->calc(0); h.draws.clear(); p->draw();
    { int iG = 0, iR = 0, eLetters = 0, band = 0; for (const auto& d : h.draws) { if (d.type == "text" && d.text == "I") { if (d.color == sdmo::C_GREEN) ++iG; else if (d.color == sdmo::C_RED) ++iR; } if (d.type == "text" && d.text == "E") ++eLetters; if (d.type == "box" && d.color == sdmo::C_STRIP) ++band; }
      CHECK(iG == 0 && iR == 0 && eLetters == 0, "0.3.0: no letters drawn");
      CHECK(band == 0, "0.3.0: no strip"); }
    { int above = 0; for (const auto& d : h.draws) if (d.type == "line" && (d.pen == sdmo::C_GREEN || d.pen == sdmo::C_RED) && (d.y < sdmo::STRIP_H || d.y0 < sdmo::STRIP_H)) ++above; CHECK(above == 0, "no line inside the strip"); }
    int green = 0, red = 0, zero = 0, outside = 0; std::string head;
    for (const auto& d : h.draws) {
        if (d.type == "line") { if (d.pen == sdmo::C_GREEN) ++green; else if (d.pen == sdmo::C_RED) ++red; else if (d.pen == sdmo::C_ZERO) ++zero;
            if (d.y < 0 || d.y > 200 || d.y0 < 0 || d.y0 > 200) ++outside; }
        if (d.type == "text" && (d.text.find("momentum") != std::string::npos || d.text.find("even") != std::string::npos)) head = d.text;
    }
    CHECK(green > 100 && red > 100 && zero == 1, "SD green, average red, one zero line (" + std::to_string(green) + "/" + std::to_string(red) + ")");
    CHECK(outside == 0, "every line inside the pane");
    CHECK(head == "Buyers' momentum building", "the 11:30 read (got '" + head + "')");
    bool title = false; for (const auto& d : h.draws) if (d.type == "text" && d.text == "SDMO v0.3.0") title = true;
    CHECK(title, "title with the version");
    const std::string st = readAll(flex + "/SDMO.status-ES-180.txt");
    CHECK(st.find("VERSION,0.3.0") != std::string::npos && st.find("SD = MA 3; average = MA 20") != std::string::npos && st.find("READ,") != std::string::npos, "status file");
    // read placed right of the last candle
    int readX = -1; for (const auto& d : h.draws) if (d.type == "text" && d.text.find("momentum") != std::string::npos) readX = d.x;
    CHECK(readX > 10 * (int)(all.size() - 1), "read right of the last candle (x " + std::to_string(readX) + ")");
    // scrolled back: read at the top-left
    h.visTo = 120; h.visFrom = 0; h.draws.clear(); p->draw(); readX = -1; for (const auto& d : h.draws) if (d.type == "text" && d.text.find("momentum") != std::string::npos) readX = d.x;
    CHECK(readX >= 0 && readX < 100, "scrolled back: read top-left");
    // no repaint: bar by bar = one pass (status READ of each step equals the logic on the same closed bars)
    mock::Host h2; h2.tick = 0.25f; h2.seconds = 180; h2.symbol = "ESZ6"; h2.root = "ES"; h2.paneBottom = 200; h2.paneRight = 2600; mock::current = &h2; SDMO* q = make(); bool same = true;
    for (size_t i = 0; i < all.size(); ++i) { h2.bars.push_back(all[i]); q->calc(0); }
    { SDMState* A = p->slot_.get(p, false); mock::current = &h2; SDMState* Bq = q->slot_.get(q, false);
      mock::current = &h; A = p->slot_.get(p, false);
      same = A && Bq && A->E.B.size() == Bq->E.B.size(); if (same) for (size_t i = 0; i < A->E.sd.size(); ++i) { const double x = A->E.sd[i], y = Bq->E.sd[i]; if (!((x != x && y != y) || x == y)) { same = false; break; } } }
    CHECK(same, "bar-by-bar replay = one straight pass");
    // two charts on one DLL do not share state; a symbol change resets
    mock::current = &h2; h2.symbol = "NQZ6"; h2.root = "NQ"; q->calc(0); { SDMState* Bq = q->slot_.get(q, false); CHECK(Bq && Bq->mkt == "NQ", "symbol change re-keys"); }
    mock::current = &h; { SDMState* A = p->slot_.get(p, false); CHECK(A && A->mkt == "ES", "other chart untouched"); }
    mock::current = &h; dispose(p); mock::current = &h2; dispose(q);
    std::printf("%s %d pass, %d fail\n", fails ? "FAILED" : "ALL PASS", passes, fails); return fails ? 1 : 0;
}
