// par.cpp - the SR journal written by lsSR (-DPAR_SR) and by lsRevRead 0.8.0's background engine must be the same lines for the same order feed.
#ifdef PAR_SR
#include "SR.cpp"
typedef SRPane Ind;
#else
#include "RevRead.cpp"
typedef RevRead Ind;
#endif
#include <unistd.h>
namespace off { long long off_test_now_ms = 1000000; void* off_test_cursor_wnd = nullptr; }
namespace mock { Host* current = nullptr; Feed feed; }
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
static long long& NOW = off::off_test_now_ms;
#pragma pack(push, 1)
struct FR { int t; signed char a, s; int px; unsigned sz; int oid, aggr; };
#pragma pack(pop)
static std::vector<FR> load(const std::string& p) { std::vector<FR> v; FILE* f = std::fopen(p.c_str(), "rb"); if (!f) return v; FR r; while (std::fread(&r, sizeof r, 1, f) == 1) v.push_back(r); std::fclose(f); return v; }
static mock::Bar bar(unsigned long t, float px) { mock::Bar b; b.time = t; b.close = px; b.open = px; b.low = px; b.high = px; b.volume = 100; b.rows.push_back(mock::Price(px, 50, 50, 100)); return b; }
static void mk(const std::string& p) { mkdir(p.c_str(), 0700); }
int main(int argc, char** argv) {
    if (argc < 5) return 2;
    const std::string mkt = argv[1], fx = argv[2], out = argv[4]; const double tick = std::atof(argv[3]);
    const std::vector<FR> v = load(fx); if (v.size() < 1000) { std::printf("no fixture\n"); return 2; }
    char tmpl[] = "/tmp/parXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels"; mk(std::string(tmp) + "\\InvestorRT"); mk(std::string(tmp) + "\\InvestorRT\\rtx"); mk(flex);
#ifdef PAR_SR
    srh::testWall() = (long long)v[0].t; srh::clockCheck() = false;
#endif
    { std::ifstream lf((fx + ".levels").c_str()); std::ofstream f((flex + "\\LRA-KeyLevels-" + mkt + ".csv").c_str(), std::ios::binary); f << "KEYLV,1," << (argc > 5 ? std::atoll(argv[5]) : (long long)v[0].t) << "\n"; std::string ln; while (std::getline(lf, ln)) { const size_t b = ln.find('|'); if (b == std::string::npos) continue; char z[96]; std::snprintf(z, sizeof z, "L|%s|%.6f|x\n", ln.substr(0, b).c_str(), std::atoll(ln.c_str() + b + 1) * tick); f << z; } }
    mock::Host h; mock::current = &h; h.tick = (float)tick; h.seconds = 180; h.root = mkt; h.symbol = mkt + "Z6"; h.paneTop = 0; h.paneBottom = 120; h.paneLeft = 0; h.paneRight = 900; h.scaleLeft = 880; h.barW = 12;
    mock::feed = mock::Feed();
    const unsigned long t0 = (unsigned long)(v[0].t / 180 * 180);
    for (int k = -40; k <= 0; ++k) h.bars.push_back(bar(t0 + 180UL * k, (float)(v[0].px * tick)));
    Ind* p = static_cast<Ind*>(CreateExtension()); p->setup(); NOW += 1000; p->calc(0); p->calc(0);
    size_t i = 0; const size_t to = v.size();
    while (i < to) {
        const unsigned long end = (unsigned long)((v[i].t / 180 + 1) * 180);
        for (; i < to && (unsigned long)v[i].t < end; ++i) { DEPTH_ORDER o; std::memset(&o, 0, sizeof o); o.timestamp = (RTDATE)v[i].t; o.price = (float)(v[i].px * tick); o.size = v[i].sz; o.orderID = v[i].oid; o.action = (unsigned char)v[i].a; o.buySell = v[i].s == 0 ? 1 : 0; o.aggressorOrderID = v[i].aggr; mock::feed.ev.push_back(o); }
        h.bars.push_back(bar(end + 180, (float)(v[i - 1].px * tick)));
        h.visFrom = std::max(0, (int)h.bars.size() - 60); h.visTo = (int)h.bars.size() - 1;
        for (int k = 0; k < 400 && mock::feed.at < mock::feed.ev.size(); ++k) p->calc(0);
        p->calc(0); NOW += 1000;
    }
    p->destroy();
    const std::string cmd = "cat " + std::string(tmp) + "*SR*2*.csv > " + out + "; ls -la " + std::string(tmp) + "* | grep -c . ";
    return std::system(cmd.c_str()) == 0 ? 0 : 1;
}
