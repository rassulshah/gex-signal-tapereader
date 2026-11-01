// test_tt011.cpp -- lsTrappedTraders 0.1.1 (2026-10-28 overnight, Rassul 20:21 / 20:31 CT): the REAL TrappedTraders.cpp calc() path on a
// mocked host (real irtsdk.h). (a) the pane gets NO volume at price (array short, as his ES chart: "candles 135 (without volume at price 135)")
// -> the status names the branch; (b) lsDeltaProfile's candle trap lots ('C') on the SignalBoard feed the candles instead (g / r exact);
// (c) load order: C missing at start = a bounded wait, then fed without, no hang; (d) marks everywhere by default, a 0.1.0 file with the old
// default upgraded, "Levels only" still a choice; (e) decided triggers posted as 'T' (decided, "?"); (f) restart = same triggers (record).
// Build (from tests_tt011/): g++ -std=c++17 -O1 -w -I/mnt/user-data/uploads/sdk/c++/include -I.. test_tt011.cpp -Wl,--unresolved-symbols=ignore-all -o /tmp/t_tt011 && /tmp/t_tt011
#include "../DealerRead_test_linux_sdk_compat.h"
#include <ctime>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#define private public
#define protected public
#include "../TrappedTraders.cpp"
#undef private
#undef protected

static int FAILS = 0, PASSES = 0;
static void check(bool c, const std::string& m) { if (c) ++PASSES; else { ++FAILS; std::printf("FAIL %s\n", m.c_str()); } }
struct Chart { std::string root = "ES", sym = "ESZ6", label = "ES 3m"; int spb = 180; void* user = nullptr; long vapN = 0;
               std::vector<unsigned long> dt; std::vector<float> o, h, l, c; std::vector<unsigned long> v; };
static Chart* CUR = nullptr;
cppExtension::cppExtension() {}
cppExtension::~cppExtension() {}
long cppExtension::getBarCount(void) { return (long)CUR->dt.size(); }
char* cppExtension::getSymbol(void) { return (char*)CUR->sym.c_str(); }
char* cppExtension::getChartLabel(void) { return (char*)CUR->label.c_str(); }
int cppExtension::getSecondsPerBar(void) { return CUR->spb; }
struct tm* cppExtension::getLocaltime(RTDATE d, struct tm* p) { time_t t = (time_t)d; gmtime_r(&t, p); return p; }
void* cppExtension::getUserData(void) { return CUR->user; }
void cppExtension::setUserData(void* p) { CUR->user = p; }
float cppExtension::getProperty(SYMBOL_PROPERTY_ID, const char*) { return 0.25f; }
char* cppExtension::getRootSymbol(char* b, const char*) { if (b) { strcpy(b, CUR->root.c_str()); return b; } return (char*)CUR->root.c_str(); }
static std::string PERIOD = "3 Minutes";
char* cppExtension::getPeriodicityLabel(void) { return (char*)PERIOD.c_str(); }
RTX_RESULT cppExtension::invalidateChart(RTBOOL) { return RTX_OK; }
cppExtension::RTARRAY::RTARRAY(fARRAY f) : tempFloat(0), bFreeTempArray(0), pArray(&tempArray), fArray(f), count(0)
{   tempArray.data = nullptr; tempArray.size = 0; tempArray.width = 0; tempArray.element = 0;
    std::vector<float>* src = f == barOpen ? &CUR->o : f == barHigh ? &CUR->h : f == barLow ? &CUR->l : f == barClose ? &CUR->c : nullptr;
    if (src) { tempArray.data = src->data(); count = (long)src->size(); tempArray.size = count; } }
cppExtension::RTARRAY::~RTARRAY() {}
float& cppExtension::RTARRAY::operator[](int i) { if (i >= 0 && i < count && pArray->data) return pArray->data[i]; return tempFloat; }
cppExtension::RTARRAYI::RTARRAYI(iARRAY a) : tempULong(0), pArray(&tempArrayI), iArray(a), count(0)
{   tempArrayI.data = nullptr; tempArrayI.size = 0;
    std::vector<unsigned long>* src = a == barDateTime ? &CUR->dt : a == barVolume ? &CUR->v : nullptr;
    if (src) { tempArrayI.data = src->data(); count = (long)src->size(); } }
cppExtension::RTARRAYI::~RTARRAYI() {}
unsigned long& cppExtension::RTARRAYI::operator[](int i) { if (i >= 0 && i < count) return pArray->data[i]; return tempULong; }
// his pane: the volume-at-price array is SHORT (0 = what his chart shows)
cppExtension::RTARRAYP::RTARRAYP(pARRAY p) : tempRTLong(0), bVolumeProfile(1), pArray(&tempArrayP), pArrayNumber(p), count(CUR->vapN) {}
cppExtension::RTARRAYP::~RTARRAYP() {}
RTX_RESULT cppExtension::RTARRAYP::getBarStatistics(int, BARSTATISTICS&) { return RTX_FAIL; }
RTX_RESULT cppExtension::RTARRAYP::getVolumeProfile(int, int, VOLPROFILE&) { return RTX_FAIL; }

static std::string UP;
static std::string fl(const std::string& name) { return UP + "/InvestorRT/rtx/lsFlexLevels/" + name; }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static std::string statusOf() { return slurp(fl("TrappedTraders/TrappedTraders.status-ES-180.txt")); }
static const unsigned long T0 = 1792900800UL;   // 2026-10-26 13:00 UTC (the mock's local time = UTC)
// a V: falls 60 -> 40 over 10 candles, then rises; the low candle (10) closes up in its upper half, sellers trapped below it
static Chart chart(int n) {
    Chart a; for (int i = 0; i < n; ++i) { float mid = i <= 10 ? 7000 - 2.0f * i : 6980 + 2.0f * (i - 10);
        a.dt.push_back(T0 + 180UL * (i + 1)); a.o.push_back(mid + (i <= 10 ? 0.5f : -0.5f)); a.c.push_back(mid + (i <= 10 ? -0.5f : 0.5f));
        a.h.push_back(mid + 1.5f); a.l.push_back(mid - 1.5f); a.v.push_back(1000); }
    a.o[10] = 6979; a.c[10] = 6981.5f; a.l[10] = 6977; a.h[10] = 6982;
    return a;
}
static std::vector<sb::Fact> cfacts(const Chart& a, int upto) {
    std::vector<sb::Fact> v; for (int i = 0; i < upto; ++i) { sb::Fact f; struct tm m; time_t t = (time_t)a.dt[i]; gmtime_r(&t, &m);
        f.bar = (long long)t;   // the mock's local = UTC, civil seconds = epoch
        f.lo = i == 10 ? 900 : (i > 10 ? 300 : 100); f.hi = i == 10 ? 200 : (i > 10 ? 100 : 400); f.side = f.lo > f.hi ? 1 : -1; f.val = f.lo + f.hi; f.state = 1; v.push_back(f); }
    return v;
}
static void calcN(TrappedTraders& X, Chart& a, int n) { CUR = &a; for (int k = 0; k < n; ++k) X.calc(0); }
static TtState& st(TrappedTraders& X, Chart& a) { CUR = &a; return *X.slot_.get(&X, false); }

int main() {
    char buf[512]; UP = std::string(getcwd(buf, sizeof buf)) + "/tthome"; std::system(("rm -rf '" + UP + "'").c_str());
    setenv("USERPROFILE", UP.c_str(), 1);
    const std::string dirs[] = { UP, UP + "/InvestorRT", UP + "/InvestorRT/rtx", UP + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& d : dirs) mkdir(d.c_str(), 0700);
    std::printf("probe: flex=%s\n", ttFlex().c_str());
    // (a) no volume at price in the pane, NO lsDeltaProfile: bounded wait (15 s), then fed without; the status names the branch
    {   Chart a = chart(30); TrappedTraders X; calcN(X, a, 3);
        TtState& S = st(X, a);
        check(S.fed == 0 && S.cWaits > 0, "[load order] C missing at start: waits (no candle fed without it yet)");
        S.startMs -= 16000; calcN(X, a, 2);
        check(S.fed == 29 && S.noVap == 29 && S.vapShort >= 29, "[no hang] after 15 s every closed candle is fed without (29)");
        const std::string t = statusOf();
        check(t.find("VAP,array 0 of 30 candles (SHORT") != std::string::npos, "[status] the VAP branch is named: array short");
        check(t.find("FALLBACK,lsDeltaProfile candle trap lots (C) MISSING") != std::string::npos, "[status] the fallback says lsDeltaProfile is missing");
        check(t.find("VERSION,0.1.2") != std::string::npos && t.find("marks everywhere") != std::string::npos, "[status] 0.1.1, marks everywhere by default");
        X.destroy(); }
    std::system(("rm -rf '" + fl("TrappedTraders") + "'").c_str());
    // (b) lsDeltaProfile posts C: every candle is fed from it, g / r exact, the low's trigger decided + posted as T
    {   Chart a = chart(30); CUR = &a; sb::postFacts('C', a.sym, a.spb, a.label, ttMs(), cfacts(a, 29));
        TrappedTraders X; calcN(X, a, 3); TtState& S = st(X, a);
        check(S.fed == 29 && S.fromC == 29 && S.noVap == 0, "[fallback] all 29 closed candles fed from C");
        bool g10 = false; for (const ttr::Bar& b : S.E.bars) if (b.t == (long long)a.dt[10]) g10 = b.ok && b.g == 900 && b.r == 200;
        check(g10, "[fallback] candle 10: green 900 (sold below the close), red 200 (bought above) = the C fact");
        bool bull = false; for (const ttr::Trigger& T : S.E.trig) if (T.side > 0) bull = true;
        check(bull, "[trigger] the V's low gives a bullish Stage 3 trigger (marks everywhere, no level needed)");
        std::vector<sb::Fact> T; const bool okT = sb::readFacts('T', a.sym, a.spb, a.label, ttMs(), T, true);
        check(okT && T.size() == S.E.trig.size() && !T.empty() && T[0].state == 1 && T[0].proven == 2, "[facts] T: each decided trigger, decided, '?'");
        const std::string t = statusOf();
        check(t.find("candles fed from C 29") != std::string::npos && t.find("on the board") != std::string::npos, "[status] FALLBACK line counts the C candles");
        // (c) new candle: C not yet posted for it -> bounded wait, then the C arrives and feeds it
        a.dt.push_back(a.dt.back() + 180); a.o.push_back(7020); a.c.push_back(7021); a.h.push_back(7022); a.l.push_back(7019); a.v.push_back(1000);
        const long long fed0 = S.fed; calcN(X, a, 2);
        check(S.fed == fed0, "[wait] the new closed candle waits for its C fact");
        sb::postFacts('C', a.sym, a.spb, a.label, ttMs(), cfacts(a, 30)); calcN(X, a, 1);
        check(S.fed == fed0 + 1 && S.fromC == 30, "[wait] then it is fed from C");
        // (f) restart: the record wins, the same triggers
        const size_t nT = S.E.trig.size(); X.destroy();
        TrappedTraders Y; Chart b = a; b.user = nullptr; calcN(Y, b, 3); TtState& S2 = st(Y, b);
        check(S2.E.trig.size() == nT, "[restart] the same triggers after a restart (" + std::to_string(S2.E.trig.size()) + " vs " + std::to_string(nT) + ")");
        Y.destroy(); }
    // (e) (0.1.2, live 10-27 22:06 "candles 4 (without volume at price 4)") the VAP array is FULL but never loads: after 3 candles the pane
    //     stops retrying it (4 passes per candle) and feeds every candle at once, the recent ones from the Delta Profile's C facts
    {   Chart a = chart(30); a.vapN = 30; a.label = "ES 3m e"; TrappedTraders X;
        { std::vector<sb::Fact> v = cfacts(a, 29); sb::postFacts('C', a.sym, a.spb, a.label, ttMs(), v); }
        calcN(X, a, 30); TtState& S = st(X, a);
        check(S.vapDead && S.fed == 29 && S.fromC >= 25, "[0.1.2] VAP never loads: retried for 3 candles only, then all 29 fed (" + std::to_string(S.fed) + " fed, " + std::to_string(S.fromC) + " from C)");
        X.destroy(); }
    // (d) settings: a 0.1.0 file with Levels only (the old default) = Everywhere; a 0.1.1 file with Levels only stays Levels only
    {   ss::Values V = ss::defaults(ttrset::spec(), 0); check(V.i(ttrset::K_MARKS) == ttrset::MARKS_ALL, "[settings] default = Everywhere");
        V.v[ttrset::K_MARKS] = ttrset::MARKS_LEVELS; ttrset::upgrade("0.1.0", V); check(V.i(ttrset::K_MARKS) == ttrset::MARKS_ALL, "[settings] a 0.1.0 file's Levels only (old default) -> Everywhere");
        V.v[ttrset::K_MARKS] = ttrset::MARKS_LEVELS; ttrset::upgrade("0.1.1", V); check(V.i(ttrset::K_MARKS) == ttrset::MARKS_LEVELS, "[settings] Levels only chosen under 0.1.1 stays");
        check(std::string(ttrset::PLUGIN_VERSION) == "0.1.2" && std::string(ttr::VERSION) == "0.1.2" && std::string(TTR_VERSION) == "0.1.2", "[version] 0.1.2 everywhere"); }
    std::printf("tt011: %d passed, %d failed\n", PASSES, FAILS);
    return FAILS ? 1 : 0;
}
