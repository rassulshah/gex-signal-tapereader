// test_wavepoc_1100.cpp -- Wave POC 1.10.0 THE PULLBACK PROFILE (PULLBACK-PROFILE-SPEC.md, mockup claude.ai/artifact/5faySwa2NBJcWbwTrEGiAn v6)
// on today's REAL ES / NQ 3-min bars + volume at price (fixtures from the recorded footprint / MBO fills, 2026-11-09) and lsBreakout's
// fired BO / Rly records of the same day. Build (in plugin\tests_wavepoc):
//   g++ -std=c++17 -Wall -Wextra -Werror -I.. test_wavepoc_1100.cpp -o t1100 && ./t1100 wavepoc_es1109_pb.csv wavepoc_nq1109_pb.csv
#include "../WavePOCLogic.h"
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
static int passed = 0, failed = 0;
#define CHECK(c, m) do { if (c) ++passed; else { ++failed; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
static long long T(const std::string& s) { struct tm t = {}; std::sscanf(s.c_str(), "%d-%d-%d %d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min); t.tm_year -= 1900; t.tm_mon -= 1; return (long long)timegm(&t); }
static std::string HM(long long t) { char b[8]; const long long s = t % 86400; std::snprintf(b, sizeof b, "%02lld:%02lld", s / 3600, s / 60 % 60); return b; }
struct Fix { std::vector<wpc::Bar> B; std::map<long long, std::vector<wpc::VRow> > V; int at(long long t) const { for (size_t i = 0; i < B.size(); ++i) if (B[i].t == t) return (int)i; return -1; } };
static Fix load(const char* f) {
    Fix X; std::ifstream in(f); std::string ln;
    while (std::getline(in, ln)) {
        if (ln.size() < 3 || ln[0] == '#') { continue; }
        std::vector<std::string> c; size_t a = 0, b; while ((b = ln.find('|', a)) != std::string::npos) { c.push_back(ln.substr(a, b - a)); a = b + 1; } c.push_back(ln.substr(a));
        if (c[0] == "B" && c.size() == 6) { wpc::Bar r; r.t = T(c[1]); r.o = std::atoll(c[2].c_str()); r.h = std::atoll(c[3].c_str()); r.l = std::atoll(c[4].c_str()); r.c = std::atoll(c[5].c_str()); X.B.push_back(r); }
        else if (c[0] == "V" && c.size() == 4) { wpc::VRow v; v.tick = std::atoll(c[2].c_str()); v.vol = std::atoll(c[3].c_str()); X.V[T(c[1])].push_back(v); }
    }
    return X;
}
static wpc::Wave wave(const Fix& X, const char* t0, const char* t1, long long p0, long long p1) {
    wpc::Wave w; w.dir = p1 > p0 ? 1 : -1; w.i0 = X.at(T(t0)); w.i1 = X.at(T(t1)); w.t0 = T(t0); w.t1 = T(t1); w.p0 = p0; w.p1 = p1; w.qWhy = wpc::Q_RLY | wpc::Q_BOW; w.rowTicks = 1; return w; }
static wpc::VapFn vapOf(const Fix& X) { return [&X](int j, std::vector<wpc::VRow>& out) { out.clear(); const auto it = X.V.find(X.B[(size_t)j].t); if (it == X.V.end()) return 0; out = it->second; return 1; }; }
static double px(long long tk) { return tk * 0.25; }
int main(int argc, char** argv) {
    const Fix ES = load(argc > 1 ? argv[1] : "wavepoc_es1109_pb.csv"), NQ = load(argc > 2 ? argv[2] : "wavepoc_nq1109_pb.csv");
    CHECK(ES.B.size() > 60 && NQ.B.size() > 25, "fixtures loaded (" << ES.B.size() << " ES bars, " << NQ.B.size() << " NQ bars)");
    // lsBreakout 11-09 records (fired): BO fire bar t / candle t0, Rly wave end te / fire tc. No Rev fired after 06:00 on either.
    std::vector<wpc::BoCand> esBo, nqBo; std::vector<wpc::RlyCand> esRly, nqRly; const std::vector<wpc::RevCand> none;
    { wpc::BoCand b; b.dir = 1; b.t0 = T("2026-11-09 06:48"); b.tf = T("2026-11-09 06:51"); esBo.push_back(b); b.t0 = T("2026-11-09 08:30"); b.tf = T("2026-11-09 08:33"); esBo.push_back(b);
      wpc::RlyCand r; r.dir = 1; r.te = T("2026-11-09 06:51"); r.tfire = T("2026-11-09 07:09"); esRly.push_back(r);
      b.t0 = T("2026-11-09 08:30"); b.tf = T("2026-11-09 08:33"); nqBo.push_back(b); b.t0 = T("2026-11-09 09:12"); b.tf = T("2026-11-09 09:15"); nqBo.push_back(b); }
    // ---- ES: Rly 7443.00 (06:25, bar 06:27) -> 7476.50 (06:51); its own Rly (te 06:51) never ends its pullback; the BO at 08:33 does ----
    {   const wpc::Wave w = wave(ES, "2026-11-09 06:27", "2026-11-09 06:51", 29772, 29906);
        CHECK(w.i0 >= 0 && w.i1 > w.i0, "ES wave bars found");
        const wpc::PbProf at52 = wpc::pullbackOf(w, ES.B, ES.at(T("2026-11-09 06:51")), esBo, esRly, none, vapOf(ES));
        CHECK(at52.state == wpc::PB_NONE, "ES at the pivot bar: nothing (no pullback yet)");
        const wpc::PbProf g = wpc::pullbackOf(w, ES.B, ES.at(T("2026-11-09 06:54")), esBo, esRly, none, vapOf(ES));
        CHECK(g.state == wpc::PB_LIVE && g.gateBar == ES.at(T("2026-11-09 06:54")), "ES 06:54 bar: past 38.2% (7463.70) -> developing (retrace " << g.retrace << ")");
        const wpc::PbProf F = wpc::pullbackOf(w, ES.B, ES.at(T("2026-11-09 09:00")), esBo, esRly, none, vapOf(ES));
        CHECK(F.state == wpc::PB_FROZEN && F.endBar == ES.at(T("2026-11-09 08:33")) && std::string(F.why) == "new BO / BD", "ES frozen at the 08:33 BO (" << F.why << ", end " << (F.endBar >= 0 ? HM(ES.B[(size_t)F.endBar].t) : "-") << ")");
        const double poc = px(F.e0 + F.poc);
        std::cout << "  ES final pullback POC " << poc << " HVN"; for (int h : F.hvn) std::cout << " " << px(F.e0 + h); std::cout << " retrace " << F.retrace << "\n";
        CHECK(std::fabs(poc - 7462.75) <= 0.5, "ES final POC = the mockup's 7462.75 (+- 2 ticks: 3-min bars from the bar after the pivot vs the mockup's 1-min) -> " << poc);
        CHECK(F.retrace > 0.74 && F.retrace < 0.77, "ES pullback 75% deep (low 7451.25)"); }
    // ---- ES invalidation on a CLOSE: wave 7456.25 (07:44, bar 07:45) -> 7475.00 (07:51); 08:31 traded 7455.75 (one tick under), the bar closed above ----
    {   const wpc::Wave w = wave(ES, "2026-11-09 07:45", "2026-11-09 07:51", 29825, 29900);
        const wpc::PbProf F = wpc::pullbackOf(w, ES.B, ES.at(T("2026-11-09 09:00")), esBo, esRly, none, vapOf(ES));
        CHECK(F.state == wpc::PB_FROZEN && std::string(F.why) == "new BO / BD" && F.endBar == ES.at(T("2026-11-09 08:33")), "ES 08:31 one-tick stop run stays VALID (close rule); frozen at the 08:33 BO (" << F.why << ")");
        wpc::Wave w2 = w; w2.p0 = 29830;   // the same wave with its origin 1.25 points higher: the 08:30 bar (close 7465.25) still above; the 08:33 bar closes above -> valid too
        // a close below the origin: move the origin above the 08:30 bar's close -> invalid at that bar, dimmed
        wpc::Wave w3 = w; w3.p0 = ES.B[(size_t)ES.at(T("2026-11-09 08:09"))].c + 1;
        const wpc::PbProf I = wpc::pullbackOf(w3, ES.B, ES.at(T("2026-11-09 09:00")), esBo, esRly, none, vapOf(ES));
        CHECK(I.state == wpc::PB_INVALID && std::string(I.why) == "close beyond the origin", "a 3-min CLOSE below the origin -> INVALID (" << I.why << ")"); (void)w2; }
    // ---- NQ: Rly 27252.00 (07:25, bar 07:27) -> 27349.75 (07:57); trades above the high in the 08:30 bar -> frozen with the volume to 08:27 ----
    {   const wpc::Wave w = wave(NQ, "2026-11-09 07:27", "2026-11-09 07:57", 109008, 109399);
        CHECK(w.i0 >= 0 && w.i1 > w.i0, "NQ wave bars found");
        const wpc::PbProf F = wpc::pullbackOf(w, NQ.B, NQ.at(T("2026-11-09 09:00")), nqBo, nqRly, none, vapOf(NQ));
        CHECK(F.state == wpc::PB_FROZEN && std::string(F.why) == "traded beyond the wave extreme" && F.endBar == NQ.at(T("2026-11-09 08:30")), "NQ ends when price trades above 27349.75 (08:30 bar) (" << F.why << ")");
        const double poc = px(F.e0 + F.poc);
        std::cout << "  NQ final pullback POC " << poc << " HVN"; for (int h : F.hvn) std::cout << " " << px(F.e0 + h); std::cout << " retrace " << F.retrace << "\n";
        CHECK(std::fabs(poc - 27317.5) <= 2.5, "NQ final POC = the mockup's 27317.50 (+- 10 ticks) -> " << poc);
        CHECK(F.retrace > 0.58 && F.retrace < 0.61, "NQ pullback 59% deep (low 27291.75)"); }
    // ---- a wave that is neither BO nor Rly (CoB only): no pullback profile ----
    {   wpc::Wave w = wave(ES, "2026-11-09 06:27", "2026-11-09 06:51", 29772, 29906); w.qWhy = wpc::Q_COBW;
        CHECK(wpc::pullbackOf(w, ES.B, ES.at(T("2026-11-09 09:00")), esBo, esRly, none, vapOf(ES)).state == wpc::PB_NONE, "a CoB-only wave gets no pullback profile"); }
    // ---- session end 16:00 CT ----
    CHECK(wpc::sessionCross(T("2026-11-09 15:57"), T("2026-11-09 16:00")) == false && wpc::sessionCross(T("2026-11-09 16:00"), T("2026-11-09 16:03")) && wpc::sessionCross(T("2026-11-09 15:57"), T("2026-11-09 17:03")), "16:00 CT session end detected (and not on the 16:00 bar itself)");
    // ---- the drawing: anchored at the pivot bar, POC to the right edge of the 2nd bar after it (min 20 px), white POC / orange HVNs in front, grey rows behind, no text ----
    {   const wpc::Wave w = wave(ES, "2026-11-09 06:27", "2026-11-09 06:51", 29772, 29906);
        const wpc::PbProf F = wpc::pullbackOf(w, ES.B, ES.at(T("2026-11-09 09:00")), esBo, esRly, none, vapOf(ES));
        for (double bw : { 6.5, 20.0 }) {
            wpc::View v; v.l = 0; v.t = 0; v.r = 1600; v.b = 800; v.first = 0; v.last = (int)ES.B.size() - 1; v.lastBar = v.last; v.bw = (int)bw; v.tick = .25;
            v.X = [&](int i) { return 40 + (i + 0.5) * bw; }; v.Y = [&](double tk) { return 800 - (tk - 29700) * 3.0; }; v.textW = [](const std::string& s, bool) { return (int)s.size() * 7; };
            wpc::Scene s; wpc::drawPullback(s, w, F, v, wpc::Colours());
            int poc = 0, hvn = 0, grey = 0, back = 0, text = 0, maxW = 0;
            for (const wpc::Prim& p : s.p) { if (p.k == wpc::Prim::TEXT) ++text; if (p.k != wpc::Prim::FILL) continue; maxW = std::max(maxW, p.x1 - p.x0);
                poc += p.col == wpc::PB_POC && !p.back; hvn += p.col == wpc::PB_HVN && !p.back; grey += p.col == wpc::mix(0x94A3B8UL, 0x0B0F17UL, wpc::GREY_FADE) && p.back; back += p.back; }
            const int want = (int)std::lround(std::max(20.0, (v.X(w.i1 + 2) + bw * 0.5) - (v.X(w.i1) - bw * 0.5)));
            CHECK(poc == 1 && hvn == (int)F.hvn.size() && grey > 10 && text == 1, "bw " << bw << ": 1 white POC row, " << F.hvn.size() << " orange HVN rows in front, " << grey << " grey rows behind, ONE text (the POC lots)");
            {   const wpc::Prim* L = nullptr; const wpc::Prim* R = nullptr; for (const wpc::Prim& p : s.p) { if (p.k == wpc::Prim::TEXT) L = &p; if (p.k == wpc::Prim::FILL && p.col == wpc::PB_POC) R = &p; }
                bool ascii = L != nullptr; if (L) for (unsigned char ch : L->text) ascii = ascii && ch >= 32 && ch < 127;
                CHECK(L && R && L->text == wpc::lotsText(F.rows[(size_t)F.poc]) && L->col == wpc::PB_POC && L->role == 1 && ascii && L->y1 <= R->y0 && R->y0 - L->y1 <= 2 && L->x0 == R->x0,
                      "1.10.1 bw " << bw << ": the pullback POC label = its row's lots '" << (L ? L->text : std::string("-")) << "' (" << F.rows[(size_t)F.poc] << "), white, ASCII, just above the row, left-aligned with it"); }
            CHECK(std::abs(maxW - want) <= 1, "bw " << bw << ": the POC row is " << maxW << " px = " << (bw * 3 >= 20 ? "3 bars" : "the 20 px minimum") << " (" << want << ")");
        }
        wpc::PbProf D = F; D.state = wpc::PB_INVALID; wpc::View v; v.first = 0; v.last = 999; v.lastBar = 999; v.bw = 7; v.X = [](int i) { return 40 + (i + 0.5) * 7.0; }; v.Y = [](double tk) { return 800 - (tk - 29700) * 3.0; };
        wpc::Scene s; wpc::drawPullback(s, w, D, v, wpc::Colours()); bool dim = !s.p.empty(); for (const wpc::Prim& p : s.p) dim = dim && p.col != wpc::PB_POC && p.col != wpc::PB_HVN;
        CHECK(dim, "invalid: every row dimmed"); }
    CHECK(wpc::lotsText(2542) == "2,542" && wpc::lotsText(738) == "738" && wpc::lotsText(1234567) == "1,234,567" && wpc::lotsText(1000) == "1,000", "1.10.1: lots with thousands commas");
    {   // the nudge: a label on top of another text moves up one label height; the wave POC label carries the POC row's lots in the POC colour
        wpc::Wave w; w.dir = 1; w.i0 = 2; w.i1 = 6; w.p0 = 100; w.p1 = 140; w.e0 = 100; w.rowTicks = 1; w.qWhy = wpc::Q_RLY; w.rows.assign(41, 10); w.rows[20] = 1240; w.maxVol = 1240;
        wpc::Node nd; nd.tick = 120; nd.vol = 1240; nd.row = 20; nd.rank = 0; w.nodes.push_back(nd);
        wpc::Engine E; for (int i = 0; i < 12; ++i) { wpc::Bar b; b.t = 1000 + i * 180; b.o = b.c = 110 + i; b.h = b.o + 2; b.l = b.o - 2; E.B.push_back(b); } E.waves.push_back(w);
        wpc::View v; v.l = 0; v.t = 0; v.r = 800; v.b = 600; v.first = 0; v.last = 11; v.lastBar = 11; v.bw = 7; v.tick = .25; v.clipL = 0; v.clipR = 800;
        v.X = [](int i) { return 40 + (i + 0.5) * 7.0; }; v.Y = [](double tk) { return 600 - (tk - 90) * 6.0; }; v.textW = [](const std::string& t, bool) { return (int)t.size() * 6; };
        const wpc::Scene sc = wpc::layout(E, nullptr, v);
        const wpc::Prim* L = nullptr; for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::TEXT && p.role == 1) L = &p;
        CHECK(L && L->text == "1,240" && L->col == 0x22D3EE, "1.10.1: the wave POC label = its row's lots '" << (L ? L->text : std::string("-")) << "' in the POC colour");
        bool overlap = false; for (const wpc::Prim& a : sc.p) for (const wpc::Prim& b : sc.p) if (&a != &b && a.k == wpc::Prim::TEXT && b.k == wpc::Prim::TEXT && a.role == 1 && a.x0 < b.x1 && a.x1 > b.x0 && a.y0 < b.y1 && a.y1 > b.y0) overlap = true;
        CHECK(!overlap, "1.10.1: no lot label on another text"); }
    CHECK(std::string(wpc::VERSION) == "1.10.4", "version 1.10.4");
    std::cout << "test_wavepoc_1100: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
