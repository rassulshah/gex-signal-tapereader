// replay_dp223.cpp -- lsDeltaProfile 2.23.0 on the mock SDK, fed a real session candle by candle (research 3-min bars from the MBO trade
// store, exported by replay export.py: "B k minute end_utc O H L C n" + n rows "tick delta volume"). Two passes: (1) straight - one
// DeltaProfile, a draw after every new candle; (2) restart - a NEW DeltaProfile (and empty memory) after every candle, reading only its
// record. Prints every researched-A D line of both passes ("S|..." / "R|...") and "SAME" when the two sets are identical.
// usage: replay_dp223 <file> <ES|NQ> <tag>
#include "../DeltaProfile.cpp"
#include <iostream>
#include <set>
#include <cstdlib>
#include <sys/stat.h>
namespace mock { Host* current = NULL; }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
static void redraw(mock::Host& h, DeltaProfile& p) { h.draws.clear(); if (h.data) { state(h).builtAt -= 1001; state(h).statusAt = -1; } mock::current = &h; p.draw(); }
static std::string home(const std::string& name) {
    const std::string root = "/tmp/dp223-replay", h = root + "/" + name; std::string c = "rm -rf '" + h + "'"; if (std::system(c.c_str())) {}
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1); return h + "/InvestorRT/rtx/lsFlexLevels/DeltaProfile/";
}
static std::set<std::string> raLines(const std::string& dir, int& rings) {
    std::set<std::string> out; std::string all;
    FILE* ls = popen(("cat " + dir + "*-signals-*.csv 2>/dev/null").c_str(), "r"); char b[4096]; size_t n;
    if (ls) { while ((n = fread(b, 1, sizeof b, ls)) > 0) all.append(b, n); pclose(ls); }
    std::istringstream in(all); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l);
        if (c.size() == 29 && c[12] == "D" && (c[2] == "A" || c[2] == "AL"))   // peak_t | decided_by | code | bias | lo | hi | conf | delay
            out.insert(c[15] + "|" + c[10] + "|" + c[2] + "|" + c[4] + "|" + c[18] + "|" + c[19] + "|" + c[24] + "|" + c[26]); }
    (void)rings; return out;
}
int main(int argc, char** argv) {
    if (argc < 4) return 2;
    g_sabGraceMs = 1LL << 60;
    std::vector<mock::Bar> all;
    { std::ifstream f(argv[1]); std::string t;
      while (f >> t) { int k, mi, n; long long end; double o, hi, lo, c; f >> k >> mi >> end >> o >> hi >> lo >> c >> n;
          mock::Bar b; b.time = static_cast<unsigned long>(end - 5 * 3600); b.open = (float)(o * .25); b.high = (float)(hi * .25); b.low = (float)(lo * .25); b.close = (float)(c * .25); b.volume = 0;
          for (int i = 0; i < n; ++i) { long long key; long d, v; f >> key >> d >> v; b.rows.push_back(mock::Price((float)(key * .25), (v + d) / 2, (v - d) / 2, v)); b.volume += v; }
          all.push_back(b); } }
    const std::string mkt = argv[2], tag = argv[3];
    auto host = [&]() { mock::Host h; h.seconds = 180; h.root = mkt; h.symbol = mkt + "Z26"; h.tick = .25f; h.period = "3 min"; return h; };
    int r0 = 0;
    // (1) straight
    const std::string d1 = home(tag + "-straight");
    { mock::Host h = host(); DeltaProfile p; for (std::size_t i = 0; i < all.size(); ++i) { h.bars.push_back(all[i]); if (i >= 31) redraw(h, p); }
      redraw(h, p); redraw(h, p); mock::current = &h; p.destroy(); latchBooks().clear(); }
    const std::set<std::string> S = raLines(d1, r0);
    // (2) restart after every candle
    const std::string d2 = home(tag + "-restart");
    { mock::Host h = host(); for (std::size_t i = 0; i < all.size(); ++i) { h.bars.push_back(all[i]); if (i < 31) continue;
          DeltaProfile p; redraw(h, p); redraw(h, p); mock::current = &h; p.destroy(); latchBooks().clear(); h.data = NULL; }
      DeltaProfile p; redraw(h, p); redraw(h, p); mock::current = &h; p.destroy(); latchBooks().clear(); }
    const std::set<std::string> R = raLines(d2, r0);
    for (const std::string& x : S) std::cout << "S|" << x << "\n";
    for (const std::string& x : R) std::cout << "R|" << x << "\n";
    std::cout << (S == R ? "SAME" : "DIFF") << " " << S.size() << " " << R.size() << "\n";
    return 0;
}
