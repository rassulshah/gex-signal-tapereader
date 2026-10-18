// test_cobimpulse_gamma_host.cpp -- lsCobImpulse 0.3.0 GAMMA PART on the mock SDK (tests_cobimpulse/irtsdk.h): 12 synthetic sessions
// (the 20-day ADR, the prior 15:00 CT close) + the real NQ 2026-10-14 bars (tests_cobimpulse/nq_20261014.csv; CoBs 08:39 and 08:54).
// g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -Itests_cobimpulse test_cobimpulse_gamma_host.cpp -o tgh && ./tgh tests_cobimpulse/nq_20261014.csv
#include "irtsdk.h"
namespace mock { Host* current = nullptr; }
#include "CobImpulse.cpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

static int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); } } while (0)
static std::string HOME;
static std::string flex() { return HOME + "\\InvestorRT\\rtx\\lsFlexLevels"; }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void spit(const std::string& p, const std::string& t) { std::ofstream f(p.c_str(), std::ios::binary | std::ios::trunc); f << t; }
static void wipe() { std::string cmd = "rm -f '" + HOME + "\\'* 2>/dev/null"; (void)!system(cmd.c_str()); }
static std::string recFiles() {
    std::string out; std::string cmd = "cat '" + HOME + "\\'*CobImpulse\\\\NQ* 2>/dev/null"; FILE* f = popen(cmd.c_str(), "r"); char b[4096]; size_t k;
    while ((k = fread(b, 1, sizeof(b), f)) > 0) out.append(b, k);
    pclose(f); return out;
}
static long long epochOf(const std::string& s) {
    struct tm m; std::memset(&m, 0, sizeof(m));
    std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &m.tm_year, &m.tm_mon, &m.tm_mday, &m.tm_hour, &m.tm_min, &m.tm_sec);
    m.tm_year -= 1900; m.tm_mon -= 1; return (long long)timegm(&m);
}
struct Fix { std::vector<mock::Bar> bars; std::vector<long long> t; };
// 12 synthetic NQ sessions (2026-09-28 .. 10-13, 17:03 the evening before .. 16:00): a daily swing of ~300 points around 30,780,
// the 10-13 15:00 CT close = 30,790; then the real NQ bars of 10-14
static Fix load(const char* path) {
    Fix F;
    const int days[12][3] = { {9,28},{9,29},{9,30},{10,1},{10,2},{10,5},{10,6},{10,7},{10,8},{10,9},{10,12},{10,13} };
    for (int d = 0; d < 12; ++d) {
        char b0[32]; std::snprintf(b0, sizeof(b0), "2026-%02d-%02d 16:00:00", days[d][1] == 0 ? 0 : days[d][0], days[d][1]);
        const long long end = epochOf(b0), start = end - 23 * 3600;     // 17:00 the evening before
        int k = 0;
        for (long long t = start + 180; t <= end; t += 180, ++k) {
            const double px = 30780 + 150 * std::sin(6.283185307179586 * k / 460.0);
            mock::Bar b; b.time = (unsigned long)t; b.open = (float)px; b.close = (float)(px + 1); b.high = (float)(px + 6); b.low = (float)(px - 6); b.volume = 100;
            if (d == 11 && t == end - 3600) b.close = 30790;            // 10-13 15:00 CT
            F.bars.push_back(b); F.t.push_back(t);
        }
    }
    std::ifstream in(path); std::string line; std::getline(in, line);
    while (std::getline(in, line)) {
        std::stringstream ss(line); std::string f[6]; for (int k = 0; k < 6; ++k) std::getline(ss, f[k], ',');
        mock::Bar b; b.time = (unsigned long)epochOf(f[0]); b.open = std::stof(f[1]); b.high = std::stof(f[2]); b.low = std::stof(f[3]); b.close = std::stof(f[4]);
        b.volume = (unsigned long)std::stod(f[5]); F.bars.push_back(b); F.t.push_back((long long)b.time);
    }
    return F;
}
static int idx(const Fix& F, const char* t) { const long long e = epochOf(t); for (size_t i = 0; i < F.t.size(); ++i) if (F.t[i] == e) return (int)i; return -1; }
static void setBars(mock::Host& H, const Fix& F, size_t n, int i839, int i854, int i833, bool absOnPivot) {
    H.bars.assign(F.bars.begin(), F.bars.begin() + (long)n);
    std::vector<float> bull(n, 0.0f), bear(n, 0.0f), ab(n, 0.0f), as(n, 0.0f);
    if ((int)n > i839) bull[(size_t)i839] = 1;
    if ((int)n > i854) bull[(size_t)i854] = 1;
    if (absOnPivot && (int)n > i833 + 1) ab[(size_t)i833] = 1;
    H.custom["cob_bull_3m_1b"] = bull; H.custom["cob_bear_3m_1b"] = bear; H.custom["Abs_Bull"] = ab; H.custom["Abs_Bear"] = as;
    H.now = H.bars.back().time; H.priceBase = 30300; H.tick = 0.25f; H.barOffset = (int)F.bars.size() - 300;
}
static std::vector<mock::Draw> texts(const mock::Host& H, const std::string& t) {
    std::vector<mock::Draw> o; for (const auto& d : H.draws) if ((d.type == "ctext" || d.type == "text") && d.text == t) o.push_back(d); return o;
}
static bool hoverOver(CobImpulse* p, mock::Host& H, const std::string& lab) {   // move the mouse onto the label, redraw
    H.draws.clear(); p->draw(); const auto L = texts(H, lab); if (L.empty()) return false;
    RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)((L[0].l + L[0].r) / 2); e.v.mouse.v = (short)((L[0].t + L[0].b) / 2);
    hb::fakeCursor() = 77; p->mouse(&e); H.draws.clear(); p->draw(); return true;
}
static bool anyText(const mock::Host& H, const std::string& t) { for (const auto& d : H.draws) if (d.text.find(t) != std::string::npos) return true; return false; }
// the NQ book: SESSION 10-14, ASOF 08:30 CT, EM band mid 30,782 (prior close 30,790), PS0 at 30,500 (08:33 / 08:39 lows: origin)
static std::string book(const std::string& extra, const char* session = "2026-10-14", int asof = 30600, double shift = 0) {
    char b[1024];
    std::snprintf(b, sizeof(b), "# test book\nBOOK,MQ\nMARKET,NQ\nKIND,intraday\nSESSION,%s\n%sKING,30000\nCW,31500\nPW,30000\nFLIP,30880\nEMH,%.2f\nEML,%.2f\nASOF,%d,%s\n",
                  session, extra.c_str(), 31135.99 + shift, 30428.01 + shift, asof, session);
    return b;
}
static const char* const S_PS0 = "STRIKE,30000,-100,1,1,KING SLICE,30000,,0\nSTRIKE,30500,-35,2,0,PS0,30500,,0\nSTRIKE,30880,0,15,0,,30880,,0\nSTRIKE,31500,25,3,0,,31500,,0\n";
static const char* const S_GM = "STRIKE,30000,-100,4,1,KING SLICE,30000,,0\nSTRIKE,30650,22,1,0,,30650,,0\nSTRIKE,30880,0,15,0,,30880,,0\nSTRIKE,31500,25,3,0,,31500,,0\n";

static CobImpulse* run(mock::Host& H, const Fix& F, size_t from, size_t to, int i839, int i854, int i833) {
    CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
    for (size_t n = from; n <= to; ++n) { setBars(H, F, n, i839, i854, i833, true); p->calc(0); p->draw(); }
    return p;
}
static const cimp::Rec& rec(CobImpulse* p, const Fix& F, int i) { return p->testState()->recs.at(std::make_pair((long long)F.bars[(size_t)i].time, 1)); }

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/cimpgXXXXXX"; HOME = mkdtemp(tmpl); setenv("USERPROFILE", HOME.c_str(), 1);
    const Fix F = load(argc > 1 ? argv[1] : "nq_20261014.csv");
    const int i839 = idx(F, "2026-10-14 08:39:00"), i854 = idx(F, "2026-10-14 08:54:00"), i833 = idx(F, "2026-10-14 08:33:00");
    CHECK(i839 > 5000 && i854 > 0 && i833 > 0, "fixture: 12 sessions + NQ 10-14");
    const size_t N = F.bars.size(), from = (size_t)i833 - 3;
    const std::string bookPath = flex() + "\\GammaProfile-MQ-NQ.csv";
    (void)!system(("mkdir -p '" + HOME + "' && touch '" + HOME + "'").c_str());

    // 0) the chart's own 20-day ADR and the prior 15:00 CT close
    {
        std::vector<long long> ct(F.t.begin(), F.t.end()); std::vector<cimp::Bar> B;
        for (const auto& b : F.bars) { cimp::Bar x; x.t = (long long)b.time; x.o = b.open; x.h = b.high; x.l = b.low; x.c = b.close; x.v = (double)b.volume; B.push_back(x); }
        const std::vector<cimp::DayAgg> D = cimp::dailyTable(ct, B);
        CHECK(D.size() == 13 && D.back().day == cimp::civil(2026, 10, 14), "13 sessions, the last = 10-14");
        const double adr = cimp::adrAt(D, 12);
        CHECK(adr > 290 && adr < 330 && D[11].has15 && std::fabs(D[11].c15 - 30790) < 1e-9, "ADR ~312 from 11 true ranges, 10-13 15:00 close 30,790");
        CHECK(cimp::adrAt(D, 10) == 0, "fewer than 10 true ranges -> no ADR");
    }
    // 1) book with PS0 30,500: both CoBs start in its zone -> Cob+V+PS0? (Abs on the pivot, MODE abs) and Cob+PS0? (08:54: C hidden)
    std::string files1;
    {
        wipe(); (void)!system(("mkdir -p '" + HOME + "'").c_str());
        mock::Host H; mock::current = &H; spit(bookPath, book(S_PS0));
        CobImpulse* p = run(H, F, from, N, i839, i854, i833);
        const cimp::Rec& a = rec(p, F, i839); const cimp::Rec& b = rec(p, F, i854);
        CHECK(a.gCode == "PS0" && a.gKind == "origin" && a.gSign == -1 && a.hedge == 'W' && a.gDist == 20 && a.gWhy.empty() && a.gAsof == cimp::civil(2026, 10, 14) * 86400 + 30600,
              "08:39: origin PS0 30,500 (5 pts = 20 ticks from the 30,505 low), short gamma = W");
        CHECK(b.gCode == "PS0" && b.gKind == "origin" && b.gDist == 8, "08:54: origin PS0 (the 30,498 low, 8 ticks)");
        H.draws.clear(); p->draw();
        CHECK(texts(H, "Cob+V+PS0?").size() == 1 && texts(H, "Cob+PS0?").size() == 1, "labels Cob+V+PS0? and Cob+PS0?");
        // hover over the 08:39 label: the impulse rows + the gamma rows (dealer line says model; no HVL regime text - claim gate)
        CHECK(hoverOver(p, H, "Cob+V+PS0?"), "hover the 08:39 label");
        CHECK(anyText(H, "Cob+V+PS0? - did the CoB leg start with force, and is a MenthorQ level near its start?") && !anyText(H, "behind it") && !anyText(H, "driving"), "hover headline");
        CHECK(anyText(H, "biggest volume in 100+ bars") && anyText(H, "MenthorQ level near the CoB start: PS0 30,500 (model, data ") && anyText(H, "min old).") && anyText(H, "Zone: the leg started inside +-0.06 ADR (+-18.7"), "(0.3.1) hover: V row + 'MenthorQ level near the CoB start: PS0 30,500 (model, data N min old).' (was 'Gamma: origin in PS0 30,500 zone')");
        CHECK(anyText(H, "Dealer hedge (model): short gamma at 30,500 - dealers sold into the low and buy as price rises = WITH the CoB."), "hover: dealer hedge (model)");
        CHECK(!anyText(H, "Regime (model)") && anyText(H, "? = still testing: CoBs that start at a MenthorQ level have NOT made more R"), "no regime text (hvl_cushion gate), ? line");
        hb::fakeCursor() = 0;
        p->testState()->lastStatusMs = -1; p->calc(0);                     // the status file is written at most once a second
        const std::string st = slurp(flex() + "\\CobImpulse.status-NQ-NQZ6-180.txt");
        CHECK(st.find("VERSION,0.4.0") != std::string::npos && st.find("BOOK,GammaProfile-MQ-NQ.csv,ok,asof ") != std::string::npos && st.find(",gamma 2,") != std::string::npos
              && st.find("gamma 1,gamma_w 6,gamma_em 100") != std::string::npos && st.find("tick 0.25") != std::string::npos, "status: version, book ok, 2 gamma tags, settings");
        p->destroy(); delete p;
        files1 = recFiles();
        CHECK(files1.find("|PS0|origin|30500|-1|20|W|") != std::string::npos && files1.find(cimp::GCTX_HEADER) != std::string::npos, "record + sidecar written");
    }
    // 2) restart after EVERY bar -> byte-identical records and sidecar (no repaint)
    {
        wipe(); mock::Host H; mock::current = &H; spit(bookPath, book(S_PS0));
        for (size_t n = from; n <= N; ++n) { CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup(); setBars(H, F, n, i839, i854, i833, true); p->calc(0); p->draw(); p->destroy(); delete p; }
        CHECK(recFiles() == files1, "restart after every bar = byte-identical records + sidecar");
        // a NEWER book after the CoBs were decided: the restart reloads the latched decision (never re-decided)
        spit(bookPath, book(S_GM, "2026-10-14", 33000));
        CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup(); setBars(H, F, N, i839, i854, i833, true); p->calc(0); p->draw();
        CHECK(rec(p, F, i839).gCode == "PS0" && recFiles() == files1, "a later book never changes a decided CoB");
        // the hover after the restart (record + sidecar) = the hover at the decision
        CHECK(hoverOver(p, H, "Cob+V+PS0?"), "hover after the restart");
        CHECK(anyText(H, "+-0.06 ADR (+-18.7") && anyText(H, "MenthorQ level near the CoB start: PS0 30,500 (model, data ") && anyText(H, "Dealer hedge (model): short gamma at 30,500"), "hover rebuilt from the files after a restart");
        hb::fakeCursor() = 0; p->destroy(); delete p;
    }
    // 3) a GM node ahead (rank 1 at 30,650, unvisited at 08:39, nearer than 2R) -> 08:39 Cob+V+GM? with the caution line;
    //    08:54: RTH already traded through 30,650 -> no gamma part ("Cob", plain)
    {
        wipe(); mock::Host H; mock::current = &H; spit(bookPath, book(S_GM));
        CobImpulse* p = run(H, F, from, N, i839, i854, i833);
        const cimp::Rec& a = rec(p, F, i839); const cimp::Rec& b = rec(p, F, i854);
        CHECK(a.gCode == "GM" && a.gKind == "target" && a.gSign == 1 && a.hedge == 'A' && a.gDist == 303, "08:39: target GM 30,650 (75.75 pts = 303 ticks), long gamma = A");
        CHECK(b.gCode.empty() && b.gWhy.empty(), "08:54: the GM zone was traded into this RTH session -> none");
        H.draws.clear(); p->draw();
        CHECK(texts(H, "Cob+V+GM?").size() == 1 && texts(H, "Cob").size() == 1, "labels Cob+V+GM? and plain Cob");
        CHECK(hoverOver(p, H, "Cob+V+GM?"), "hover the GM label");
        CHECK(anyText(H, "Gamma: GM 30,650 ahead, 0.2 EM away, not traded into this session.") && anyText(H, "Ahead: GM 30,650 is 1.1R away, before 2R"), "hover: target row + caution");
        CHECK(anyText(H, "Dealer hedge (model): long-gamma node ahead - can stall price at it."), "hover: long-gamma node ahead (model)");
        hb::fakeCursor() = 0;
        // GAMMA,0 hides the gamma part (still recorded); GAMMA_W / GAMMA_EM bad -> defaults + reason
        spit(flex() + "\\LRA-CobImpulse-NQ.csv", "VERSION,1\nMARKET,NQ\nMODE,always\nGAMMA,0\nGAMMA_W,6\nGAMMA_EM,100\n");
        p->testState()->paramsCheckMs = -1; p->testState()->paramsStamp = -3; H.draws.clear(); p->draw();
        CHECK(texts(H, "Cob+V?").size() == 1 && texts(H, "Cob+C?").size() == 1 && texts(H, "Cob+V+GM?").empty() && rec(p, F, i839).gCode == "GM", "GAMMA,0: label without the gamma part, still recorded");
        spit(flex() + "\\LRA-CobImpulse-NQ.csv", "VERSION,1\nMARKET,NQ\nMODE,always\nGAMMA,1\nGAMMA_W,11\n");
        p->testState()->paramsCheckMs = -1; p->testState()->paramsStamp = -3; p->calc(0);
        CHECK(p->testState()->paramsWhy == "bad GAMMA_W (defaults)", "GAMMA_W 11 -> defaults + reason");
        std::remove((flex() + "\\LRA-CobImpulse-NQ.csv").c_str());
        p->destroy(); delete p;
    }
    // 4) the reasons there is no gamma part: stale (08:00 book at an RTH CoB), wrong session, late book, misaligned, missing, garbled
    struct W { std::string text; const char* why; const char* name; };
    const W cases[] = {
        { book(S_PS0, "2026-10-14", 8 * 3600), "stale", "a 08:00 book at the 08:39 RTH CoB (39 min) = stale" },
        { book(S_PS0, "2026-10-13", 30600), "wrong session", "a book for 10-13 = wrong session" },
        { book(S_PS0, "2026-10-14", 8 * 3600 + 40 * 60), "late book", "a book stamped 08:40 for an 08:39 CoB = late" },
        { book(S_PS0, "2026-10-14", 30600, 100), "misaligned", "band mid 92 pts from the prior close (> 0.20 x ADR ~62) = misaligned" },
        { std::string(), "no book", "no file = no book" },
        { std::string("VERSION,9\nMARKET,NQ\n"), "no book", "a garbled book = no book" },
    };
    for (const auto& c : cases) {
        wipe(); mock::Host H; mock::current = &H; std::remove(bookPath.c_str());
        if (!c.text.empty()) spit(bookPath, c.text);
        CobImpulse* p = run(H, F, N - 1, N, i839, i854, i833);
        const cimp::Rec& a = rec(p, F, i839);
        CHECK(a.gCode.empty() && a.gWhy == c.why, c.name);
        H.draws.clear(); p->draw();
        CHECK(texts(H, "Cob+V?").size() == 1, "label without a gamma part");
        p->destroy(); delete p;
    }
    // 5) two charts on one DLL: NQ reads its own book, ES (no book) has none
    {
        wipe(); mock::Host A, B; B.symbol = "ESZ6"; B.root = "ES"; spit(bookPath, book(S_PS0));
        CobImpulse* p = static_cast<CobImpulse*>(CreateExtension());
        mock::current = &A; p->setup(); setBars(A, F, N, i839, i854, i833, true); p->calc(0);
        mock::current = &B; setBars(B, F, N, i839, i854, i833, true); p->calc(0);
        CHECK(static_cast<CIState*>(A.data)->recs.begin()->second.gCode == "PS0" && static_cast<CIState*>(B.data)->recs.begin()->second.gWhy == "no book", "no book crosses charts");
        mock::current = &A; p->destroy(); mock::current = &B; p->destroy(); delete p;
    }
    // 6) a torn / garbled sidecar line is skipped (the hover just lacks those rows), a garbled record line is skipped
    {
        cimp::GCtx c; CHECK(!cimp::parseCtx("1|bull|6|1|2||0|GM|||1|", c) && !cimp::parseCtx("1|bull|6|1|2||0|XX|1|1|1|1", c) && !cimp::parseCtx("garbage", c), "bad sidecar lines rejected");
        cimp::Rec r; CHECK(!cimp::parseRec("1|bull|1|2|0|0|0.00|0|0|0.00|0|0|0|0.00|0|GM|origin|-5|1|0|W|0|", r), "negative g_px rejected");
    }
    std::printf("%d checks, %d failures\n", checks, fails);
    std::string cmd = "rm -rf '" + HOME + "' '" + HOME + "\\'* 2>/dev/null"; (void)!system(cmd.c_str());
    return fails ? 1 : 0;
}
