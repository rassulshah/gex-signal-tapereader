// Replays real bars through lsExhaustionV2 (ExhaustionV2.cpp) on the mock host, one closed bar at a time.
#include "ExhaustionV2.cpp"
#include <cstdio>
#include <unistd.h>
#include <dirent.h>
namespace mock { Host* current = nullptr; }
int main(int argc, char** argv) {
    const char* market = argv[1]; const char* contract = argv[2]; const char* root = argv[3]; float tick = (float)atof(argv[4]);
    char tmpl[] = "/tmp/exv2XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    { std::ifstream in((std::string("cal_") + market + ".csv").c_str()); std::ofstream out((flex + "\\LRA-ExhaustionV2-" + market + "-" + contract + ".csv").c_str()); out << in.rdbuf(); }
    mock::Host h; mock::current = &h; h.tick = tick; h.seconds = 180; h.symbol = contract; h.root = root;
    std::ifstream bf((std::string("bars_") + market + ".csv").c_str()); std::string line; std::vector<mock::Bar> all;
    while (std::getline(bf, line)) { unsigned long t; double o, hi, lo, c, v, b, s; if (sscanf(line.c_str(), "%lu,%lf,%lf,%lf,%lf,%lf,%lf,%lf", &t, &o, &hi, &lo, &c, &v, &b, &s) != 8) continue;
        mock::Bar x; x.time = t; x.open = (float)o; x.high = (float)hi; x.low = (float)lo; x.close = (float)c; x.volume = (unsigned long)v; x.rows.push_back(mock::Price((float)c, (long)b, (long)s, (long)v)); all.push_back(x); }
    Exhaustion* p = static_cast<Exhaustion*>(CreateExtension());
    for (size_t i = 0; i < all.size(); ++i) {
        h.bars.push_back(all[i]);
        mock::Bar forming = all[i]; forming.time = all[i].time + 180; forming.volume = 1; forming.rows.clear(); forming.rows.push_back(mock::Price(all[i].close, 1, 0, 1));
        h.bars.push_back(forming); h.now = forming.time;
        p->calc(0);
        static std::map<std::string,int> st; st[p->stateText()]++; if (i + 1 == all.size()) for (auto& kv : st) std::fprintf(stderr, "  state %s: %d\n", kv.first.c_str(), kv.second);
        h.bars.pop_back();
    }
    // the journals the indicator wrote
    std::string dir = flex + "\\Exhaustion"; int n = 0;
    DIR* d = opendir("/tmp"); const std::string base = std::string(tmp).substr(5); struct dirent* e; std::vector<std::string> files;
    while (d && (e = readdir(d))) { std::string f = e->d_name; if (f.compare(0, base.size(), base) == 0 && f.find("LRA-ExhaustionV2-events") != std::string::npos) files.push_back("/tmp/" + f); }
    if (d) closedir(d);
    std::sort(files.begin(), files.end());
    for (auto& f : files) { std::ifstream in(f.c_str()); std::getline(in, line);
        while (std::getline(in, line)) { std::vector<std::string> c; std::stringstream ss(line); std::string x; while (std::getline(ss, x, ',')) c.push_back(x);
            if (c.size() > 12) { std::printf("%s,%s,%s\n", c[12].c_str(), c[7].c_str(), c[10].c_str()); ++n; } } }
    std::fprintf(stderr, "bars %zu, journal rows %d, state %s\n", all.size(), n, p->stateText().c_str());
    p->destroy(); return 0;
}
