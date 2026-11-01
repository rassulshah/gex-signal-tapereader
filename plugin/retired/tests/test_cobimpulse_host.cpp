// test_cobimpulse_host.cpp -- lsCobImpulse 0.1.0-0.3.0 on the mock SDK (tests_cobimpulse/irtsdk.h); the 0.3.0 gamma part: test_cobimpulse_gamma_host.cpp
// g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -Itests_cobimpulse test_cobimpulse_host.cpp -o th && ./th nq_20261014.csv
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
static void wipe() {   // every file the plugin writes in HOME's flat namespace
    const std::string pre = HOME + "\\";
    std::string cmd = "rm -f '" + pre + "'* 2>/dev/null"; (void)!system(cmd.c_str());
}

struct Fix { std::vector<mock::Bar> bars; std::vector<std::string> t; };
static long long epochOf(const std::string& s) {   // "2026-10-14 08:39:00" as Central wall time
    struct tm m; std::memset(&m, 0, sizeof(m));
    std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &m.tm_year, &m.tm_mon, &m.tm_mday, &m.tm_hour, &m.tm_min, &m.tm_sec);
    m.tm_year -= 1900; m.tm_mon -= 1; return (long long)timegm(&m);
}
static Fix load(const char* path) {
    Fix F; std::ifstream in(path); std::string line; std::getline(in, line);
    while (std::getline(in, line)) {
        std::stringstream ss(line); std::string f[6]; for (int k = 0; k < 6; ++k) std::getline(ss, f[k], ',');
        mock::Bar b; b.time = (unsigned long)epochOf(f[0]); b.open = std::stof(f[1]); b.high = std::stof(f[2]); b.low = std::stof(f[3]); b.close = std::stof(f[4]);
        b.volume = (unsigned long)std::stod(f[5]); F.bars.push_back(b); F.t.push_back(f[0]);
    }
    return F;
}
static int idx(const Fix& F, const char* t) { for (size_t i = 0; i < F.t.size(); ++i) if (F.t[i] == t) return (int)i; return -1; }

// a host for one chart with the first n bars (+1 forming bar) of the fixture, his RTL flags as IRT would compute them
static void setBars(mock::Host& H, const Fix& F, size_t n, int i839, int i854, int i833, bool absOnPivot) {
    H.bars.assign(F.bars.begin(), F.bars.begin() + (long)n);
    std::vector<float> bull(n, 0.0f), bear(n, 0.0f), ab(n, 0.0f), as(n, 0.0f);
    if (i839 >= 0 && (int)n > i839) bull[(size_t)i839] = 1;
    if (i854 >= 0 && (int)n > i854) bull[(size_t)i854] = 1;
    if (absOnPivot && (int)n > i833 + 1) ab[(size_t)i833] = 1;       // IRT knows the Abs pivot once the next bar exists
    H.custom["cob_bull_3m_1b"] = bull; H.custom["cob_bear_3m_1b"] = bear; H.custom["Abs_Bull"] = ab; H.custom["Abs_Bear"] = as;
    H.now = H.bars.back().time; H.priceBase = 30300;
}
static std::vector<mock::Draw> texts(const mock::Host& H, const std::string& t) {
    std::vector<mock::Draw> o; for (const auto& d : H.draws) if ((d.type == "ctext" || d.type == "text") && d.text.find(t) != std::string::npos) o.push_back(d); return o;
}
static int hollowBoxes(const mock::Host& H) { int n = 0; for (const auto& d : H.draws) if (d.type == "rect" && d.pattern == cppExtension::PAT_HOLLOW && d.color == 0x00FFFFFF && d.width == 1) ++n; return n; }

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/cimpXXXXXX"; HOME = mkdtemp(tmpl); setenv("USERPROFILE", HOME.c_str(), 1);
    const Fix F = load(argc > 1 ? argv[1] : "nq_20261014.csv");
    const int i839 = idx(F, "2026-10-14 08:39:00"), i854 = idx(F, "2026-10-14 08:54:00"), i833 = idx(F, "2026-10-14 08:33:00");
    CHECK(i839 > 0 && i854 > 0 && i833 > 0, "fixture loaded");

    // 1) bar-by-bar straight pass (Abs on the pivot) -> 08:39 Cob+V? (box on 08:33), 08:54 plain Cob
    {
        wipe(); mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        for (size_t n = 200; n <= F.bars.size(); ++n) { setBars(H, F, n, i839, i854, i833, true); p->calc(0); p->draw(); }
        CIState* S = p->testState();
        CHECK(S && S->recs.size() == 2, "two CoBs recorded");
        const cimp::Rec& a = S->recs.at(std::make_pair((long long)F.bars[(size_t)i839].time, 1));
        CHECK(a.v() && !a.r() && a.abs && a.vT == (long long)F.bars[(size_t)i833].time && a.vn >= 35, "08:39 = V on 08:33 with Abs on the pivot");
        const cimp::Rec& b = S->recs.at(std::make_pair((long long)F.bars[(size_t)i854].time, 1));
        CHECK(!b.v() && !b.r() && b.c() && !b.abs, "08:54 = C (08:33 climax), no Abs on its pivot -> hidden in the default mode");
        H.draws.clear(); p->draw();
        CHECK(texts(H, "Cob+V?").size() == 1 && texts(H, "Cob").size() == 2, "labels: one Cob+V?, one plain Cob");
        CHECK(hollowBoxes(H) == 1, "one 1 px white box (the 08:33 impulse bar)");
        {   // (0.5.2, Rassul 16:05 CT 2026-10-21 "fix this, it should be solid") the lsHodLod table over the Cob+V? label and the impulse box:
            // neither is painted, both records stay; a table on another chart changes nothing
            auto L0 = texts(H, "Cob+V?"); int bl = 1 << 30, bt = 1 << 30, br = -1, bb = -1;
            for (const auto& d : H.draws) if (d.type == "rect" && d.pattern == cppExtension::PAT_HOLLOW) { bl = d.l; bt = d.t; br = d.r; bb = d.b; }
            if (!L0.empty() && br > 0) {
                const bdk::Box T = bdk::mk(std::min(L0[0].l, bl) - 3, std::min(L0[0].t, bt) - 3, std::max(L0[0].r, br) + 3, std::max(L0[0].b, bb) + 3);
                const std::string sym = p->getSymbol() ? p->getSymbol() : "";
                bdk::publishHodLod(sym, p->getSecondsPerBar(), "test", 1, T);
                H.draws.clear(); p->draw();
                bool inside = false; for (const auto& d : H.draws) if (bdk::hitsTable(T, d.l, d.t, d.r, d.b)) inside = true;
                CHECK(!inside && texts(H, "Cob+V?").empty() && hollowBoxes(H) == 0, "[keep-out] the label and box under the table are not painted");
                CHECK(S->recs.size() == 2, "[keep-out] both CoBs still recorded");
                bdk::publishHodLod(sym, p->getSecondsPerBar(), "another chart", 1, T); bdk::clearHodLod(sym, p->getSecondsPerBar(), "test");
                H.draws.clear(); p->draw();
                CHECK(texts(H, "Cob+V?").size() == 1 && hollowBoxes(H) == 1, "[keep-out] a table on another chart: drawn as before");
                bdk::clearHodLod(sym, p->getSecondsPerBar(), "another chart");
            } else CHECK(false, "[keep-out] fixture: label and box drawn");
        }
        // the box surrounds the impulse bar: x = bar 08:33
        for (const auto& d : H.draws) if (d.type == "rect" && d.pattern == cppExtension::PAT_HOLLOW) CHECK(d.l <= 20 + i833 * 10 && d.r >= 20 + i833 * 10 + 8, "box around the 08:33 bar");
        // label above the leg high (bull)
        auto L = texts(H, "Cob+V?"); cppExtension::PNT pe; pe.set(i833 + 1, F.bars[(size_t)i833 + 1].high, cppExtension::kBarCenter);
        CHECK(!L.empty() && L[0].b < pe.v, "label above the leg's high");
        // the hover over the label
        RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)((L[0].l + L[0].r) / 2); e.v.mouse.v = (short)((L[0].t + L[0].b) / 2);
        hb::fakeCursor() = 77; p->draw(); H.invalidates = 0; p->mouse(&e);
        /* (0.5.1, Rassul 23:37 CT 2026-10-18 hovers off: with CI_HOVERS false the mouse must change nothing) */ CHECK(CI_HOVERS ? H.invalidates == 1 : H.invalidates == 0, CI_HOVERS ? "hovering the label redraws once" : "hovers off: the mouse over the label asks for no redraw");
        H.draws.clear(); p->draw();
        bool vol = false, tm = false; for (const auto& d : H.draws) { if (d.text.find("biggest volume in 100+ bars") != std::string::npos) vol = true; if (d.text.find("8:33 AM bar") != std::string::npos) tm = true; }
        CHECK(CI_HOVERS ? (vol && tm) : (!vol && !tm), CI_HOVERS ? "hover: 8:33 AM bar, biggest volume in 100+ bars" : "hovers off: no hover box over the label");
        hb::fakeCursor() = 99; H.draws.clear(); p->draw();
        bool gone = true; for (const auto& d : H.draws) if (d.text.find("biggest volume") != std::string::npos) gone = false;
        CHECK(gone, "hover hidden when the cursor leaves the chart window");
        hb::fakeCursor() = 0;
        p->destroy(); CHECK(H.data == nullptr, "destroy releases the chart state");
        delete p;
    }
    // the record files (straight pass)
    std::string files1;
    { std::string cmd = "cat '" + HOME + "\\'*CobImpulse\\\\NQ* 2>/dev/null"; FILE* f = popen(cmd.c_str(), "r"); char b[4096]; size_t k; while ((k = fread(b, 1, sizeof(b), f)) > 0) files1.append(b, k); pclose(f); }
    CHECK(files1.find("|bull|") != std::string::npos, "records written");

    // 2) restart after EVERY bar (new state, same files) -> byte-identical records (no repaint)
    {
        wipe(); mock::Host H; mock::current = &H;
        for (size_t n = 200; n <= F.bars.size(); ++n) {
            CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
            setBars(H, F, n, i839, i854, i833, true); p->calc(0); p->draw(); p->destroy(); delete p;
        }
        std::string files2; std::string cmd = "cat '" + HOME + "\\'*CobImpulse\\\\NQ* 2>/dev/null"; FILE* f = popen(cmd.c_str(), "r"); char b[4096]; size_t k; while ((k = fread(b, 1, sizeof(b), f)) > 0) files2.append(b, k); pclose(f);
        CHECK(files2 == files1, "restart after every bar = byte-identical records");
    }
    // 3) display modes from the market file; Abs off; PROVEN; PLAIN
    {
        wipe(); mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        setBars(H, F, F.bars.size(), i839, i854, i833, false); p->calc(0); H.draws.clear(); p->draw();
        CHECK(texts(H, "Cob+").empty() && hollowBoxes(H) == 0 && texts(H, "Cob").size() == 2, "default mode abs, no Abs on the pivot: plain labels, no box");
        CHECK(p->testState()->recs.size() == 2 && p->testState()->recs.begin()->second.trig > 0, "the tag is still recorded");
        bool logged = false; for (auto& kv : p->testState()->recs) if (kv.second.v()) logged = true;
        CHECK(logged, "V logged even when hidden");
        spit(flex() + "\\LRA-CobImpulse-NQ.csv", "VERSION,1\nMARKET,NQ\nMODE,always\n");
        p->testState()->paramsCheckMs = -1; H.draws.clear(); p->draw();
        CHECK(texts(H, "Cob+V?").size() == 1 && texts(H, "Cob+C?").size() == 1 && hollowBoxes(H) == 2, "MODE always: Cob+V? and Cob+C? shown (each boxes the 08:33 bar)");
        spit(flex() + "\\LRA-CobImpulse-NQ.csv", "VERSION,1\nMARKET,NQ\nMODE,always\nPROVEN,1\nPLAIN,0\n");
        p->testState()->paramsCheckMs = -1; p->testState()->paramsStamp = -3; H.draws.clear(); p->draw();
        CHECK(texts(H, "Cob+V").size() == 1 && texts(H, "Cob+V?").empty() && texts(H, "Cob+C").size() == 1 && texts(H, "Cob").size() == 2, "PROVEN drops the ?");
        spit(flex() + "\\LRA-CobImpulse-NQ.csv", "VERSION,1\nMARKET,NQ\nMODE,off\n");
        p->testState()->paramsCheckMs = -1; p->testState()->paramsStamp = -3; H.draws.clear(); p->draw();
        CHECK(texts(H, "Cob+").empty() && hollowBoxes(H) == 0, "MODE off: no tag, no box");
        spit(flex() + "\\LRA-CobImpulse-NQ.csv", "VERSION,1\nMARKET,ES\nMODE,always\n");
        p->testState()->paramsCheckMs = -1; p->testState()->paramsStamp = -3; H.draws.clear(); p->draw();
        CHECK(texts(H, "Cob+").empty() && p->testState()->paramsWhy.find("defaults") != std::string::npos, "wrong-market file -> defaults (abs), reason in the status");
        std::string st = slurp(flex() + "\\CobImpulse.status-NQ-NQZ6-180.txt");
        CHECK(st.find(std::string("VERSION,") + CI_VERSION) != std::string::npos && st.find("MARKET,NQ") != std::string::npos, "status file");
        p->destroy(); delete p; std::remove((flex() + "\\LRA-CobImpulse-NQ.csv").c_str());
    }
    // 3b) a 0.1.0 record file (11 fields) is read, kept and rewritten in the 0.2.0 layout
    {
        wipe(); mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        setBars(H, F, (size_t)i839 + 1, i839, i854, i833, true); p->calc(0);      // decides nothing yet (forming) but identifies
        const long long sess = (epochOf("2026-10-14 08:39:00") - 17LL * 3600) / 86400LL + 1;
        const std::string path = flex() + "\\CobImpulse\\NQ-NQZ6-180-" + std::to_string(sess) + ".csv";
        spit(path, std::string(cimp::REC_HEADER_010) + "\n" + std::to_string(F.bars[(size_t)i833].time) + "|bull|1|2|0|0|0.00|0|0|0.00|0\n");
        p->testState()->session = -1;                                              // reload on the next pump
        setBars(H, F, F.bars.size(), i839, i854, i833, true); p->calc(0);
        const std::string t = slurp(path);
        CHECK(t.rfind(cimp::REC_HEADER, 0) == 0 && t.find(std::to_string(F.bars[(size_t)i833].time) + "|bull|1|2|0|0|0.00|0|0|0.00|0|0|0|0.00|0|||0|0|0||0|pre 0.3.0\n") != std::string::npos
              && t.find(std::to_string(F.bars[(size_t)i839].time) + "|bull|") != std::string::npos, "0.1.0 record kept in the 0.3.0 layout (no gamma part: pre 0.3.0), new CoBs added");
        p->destroy(); delete p;
    }
    // 4) his RTL not readable -> nothing decided, retried; forming-bar flag never decided
    {
        wipe(); mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        setBars(H, F, (size_t)i839 + 3, i839, i854, i833, true);
        H.customFail["cob_bull_3m_1b"] = true; H.customFail["cob_bear_3m_1b"] = true;
        p->calc(0);
        CHECK(p->testState()->recs.empty() && p->testState()->decidedThrough == 0 && p->testState()->state.find("not readable") != std::string::npos, "RTL missing: nothing decided");
        H.customFail.clear(); setBars(H, F, (size_t)i839 + 4, i839, i854, i833, true); p->calc(0);
        CHECK(p->testState()->recs.size() == 1, "readable on the next bar: decided then");
        // a flag on the forming bar only
        wipe(); mock::Host G; mock::current = &G; CobImpulse* q = static_cast<CobImpulse*>(CreateExtension()); q->setup();
        setBars(G, F, (size_t)i839 + 1, i839, i854, i833, true); q->calc(0);
        CHECK(q->testState()->recs.empty(), "a flag on the forming bar is not decided");
        setBars(G, F, (size_t)i839 + 2, i839, i854, i833, true); q->calc(0);
        CHECK(q->testState()->recs.size() == 1, "decided once the bar has closed");
        q->destroy(); delete q; mock::current = &H; p->destroy(); delete p;
    }
    // 5) two charts on one DLL object: NQ and ES keep their own state and files
    {
        wipe(); mock::Host A, B; B.symbol = "ESZ6"; B.root = "ES";
        CobImpulse* p = static_cast<CobImpulse*>(CreateExtension());
        mock::current = &A; p->setup(); setBars(A, F, F.bars.size(), i839, i854, i833, true); p->calc(0); p->draw();
        mock::current = &B; setBars(B, F, F.bars.size(), -1, -1, i833, true); B.custom["cob_bull_3m_1b"].assign(F.bars.size(), 0.0f); p->calc(0); p->draw();
        CHECK(static_cast<CIState*>(A.data)->recs.size() == 2 && static_cast<CIState*>(B.data)->recs.empty(), "no state crosses charts");
        CHECK(static_cast<CIState*>(B.data)->market == "ES", "ES chart is ES");
        mock::current = &A; p->destroy(); mock::current = &B; p->destroy(); delete p;
    }
    // 6) volumes over 2^31 and a corrupt record file
    {
        wipe(); mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        Fix G = F; for (auto& b : G.bars) b.volume = b.volume * 1000000UL;   // > 2^31 on a 64-bit unsigned long
        setBars(H, G, G.bars.size(), i839, i854, i833, true); p->calc(0);
        bool v = false; for (auto& kv : p->testState()->recs) if (kv.second.v()) v = true;
        CHECK(v, "huge volumes still tag V");
        p->destroy(); delete p;
    }
    // (0.5.0) the settings window and file: Cob labels / Impulse bar box, each Show + colour
    {
        wipe(); mock::Host H; mock::current = &H; CobImpulse* p = static_cast<CobImpulse*>(CreateExtension()); p->setup();
        CHECK(cppExtension::P().size() == 4 && cppExtension::pver() == 2, "setup registers the 4 rows (parameter version 2)");
        setBars(H, F, F.bars.size(), i839, i854, i833, true); p->calc(0);
        const std::string sf = flex() + "\\CobImpulse.settings-NQ.txt";
        spit(sf, "SPEC1|CobImpulse|0.5.0|NQ\nshow_labels=1\nlabel_color=FF0000\nshow_box=0\nbox_color=FFFFFF\n");
        H.draws.clear(); p->draw();
        const std::vector<mock::Draw> t1 = texts(H, "Cob+V?");
        CHECK(t1.size() == 1 && t1[0].color == 0xFF0000ul && hollowBoxes(H) == 0, "label colour from the file; Impulse bar box off: no box");
        spit(sf, "SPEC1|CobImpulse|0.5.0|NQ\nshow_labels=0\nshow_box=1\nbox_color=00FF00\n");
        H.draws.clear(); p->draw();
        int green = 0; for (const auto& d : H.draws) if (d.type == "rect" && d.pattern == cppExtension::PAT_HOLLOW && d.color == 0x00FF00ul) ++green;
        CHECK(texts(H, "Cob+V?").empty() && texts(H, "Cob").empty() && green == 1, "labels off: none drawn; the box in its colour");
        spit(sf, "garbage"); H.draws.clear(); p->draw();
        CHECK(texts(H, "Cob+V?").size() == 1 && hollowBoxes(H) == 1, "a bad file = the defaults (what 0.4.1 drew)");
        std::remove(sf.c_str());
        cppExtension::dlgPeriod() = ""; p->parmsLoad();
        CHECK(slurp(sf).empty(), "opening the window never writes");
        CHECK(p->getParameterColor(1) == 0x80FFFFul && p->isBoxChecked(2) == 1, "the window opens with the defaults");
        p->checkBox(2, false); p->setParameterColor(1, 0x123456); p->parmsApply();
        const std::string w = slurp(sf);
        CHECK(w.find("\nshow_box=0\n") != std::string::npos && w.find("\nlabel_color=123456\n") != std::string::npos && w.find("\nshow_labels=1\n") != std::string::npos, "Apply saves the changes, the rest at their values");
        cppExtension::dlgPeriod() = "1 Minute"; std::remove(sf.c_str()); p->parmsApply();
        CHECK(slurp(sf).empty(), "the chart copy never saves");
        p->destroy(); delete p;
    }
    std::printf("%d checks, %d failures\n", checks, fails);
    std::string cmd = "rm -rf '" + HOME + "' '" + HOME + "\\'* 2>/dev/null"; (void)!system(cmd.c_str());
    return fails ? 1 : 0;
}
