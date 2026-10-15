// replay_es285.cpp -- lsDeltaProfile 2.8.5 replay of Rassul's E-S chart, 2026-10-14 evening (session 20741), through the plugin on the
// mock SDK: his session record (lsFlexLevels\DeltaProfile\ES-ESZ6-180-signals-20741.csv) + his chart's 3-min bars as IRT exported them
// (lsFlexLevels\LRA-IRT-Bars-ES.csv, bar END times, Central clock). Prints, per A record: the 2.8.4 re-check rule by rule and the 2.8.5
// display decision; then the bubbles 2.8.5 draws. Also re-runs the pivot rule from 6 later chart starts (his chart holds more history
// than the export's first bar) to show the answer does not depend on where the chart starts.
// IRT raw stamps in the record are seconds since 1904 on the Central clock; the mock's are seconds since 1970 on the same clock, so the
// raw columns are shifted by 2082844800 (the 1904 -> 1970 offset) and nothing else is changed.
// Build (from tests_delta): g++ -std=c++17 -g -fsanitize=address,undefined -I. replay_es285.cpp -o r && ./r <signals.csv> <bars.csv>
#include "../DeltaProfile.cpp"
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
namespace mock { Host* current = NULL; }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static const long long MAC = 2082844800LL;
static std::string hhmm(long long t) { time_t v = static_cast<time_t>(t); struct tm m; gmtime_r(&v, &m); char b[16]; std::snprintf(b, sizeof b, "%02d:%02d", m.tm_hour, m.tm_min); return b; }
int main(int argc, char** argv)
{
    if (argc < 3) { std::cerr << "usage: replay_es285 <signals.csv> <bars.csv>\n"; return 2; }
    setenv("TZ", "UTC", 1); tzset();
    // the chart
    mock::Host h; h.seconds = 180; h.root = "ES"; h.symbol = "ESZ6"; h.tick = 0.25f; h.period = "3 min";
    h.paneRight = 4000; h.scaleLeft = 3980; h.priceOrigin = 7700; h.priceScale = 10;
    {
        std::istringstream in(slurp(argv[2])); std::string l;
        while (std::getline(in, l)) {
            if (l.empty() || l.compare(0, 7, "IRTBARS") == 0) continue;
            const std::vector<std::string> c = fields(l); if (c.size() < 6) continue;
            struct tm m = {}; if (!strptime(c[0].c_str(), "%Y-%m-%d %H:%M:%S", &m)) continue;
            mock::Bar b; b.time = static_cast<unsigned long>(timegm(&m));
            b.open = std::stof(c[1]); b.high = std::stof(c[2]); b.low = std::stof(c[3]); b.close = std::stof(c[4]);
            b.rows = { mock::Price(b.close, 1, 1, 2) }; b.volume = 2;   // delta 0: no live nodes, only the record is replayed
            h.bars.push_back(b);
        }
    }
    std::cout << "chart: " << h.bars.size() << " bars, " << hhmm(static_cast<long long>(h.bars.front().time)) << " .. " << hhmm(static_cast<long long>(h.bars.back().time)) << " (bar END times)\n";
    // the record, raw stamps shifted to the mock's clock
    std::string out; int aRecords = 0;
    {
        std::istringstream in(slurp(argv[1])); std::string l;
        while (std::getline(in, l)) {
            std::vector<std::string> c = fields(l);
            if (c.size() == REC_FIELDS && (c[12] == "F" || c[12] == "D")) {
                c[16] = std::to_string(std::stoll(c[16]) - MAC); if (c[23] != "0") c[23] = std::to_string(std::stoll(c[23]) - MAC);
                if (c[12] == "D" && c[2] == "A") ++aRecords;
                std::string j; for (std::size_t k = 0; k < c.size(); ++k) j += (k ? "|" : "") + c[k]; out += j + "\n";
            } else out += l + "\n";
        }
    }
    const char* r = std::getenv("DP254_RUN"); const std::string root = std::string(r && r[0] ? r : "/tmp/dp254-run") + "/es285";
    if (std::system(("rm -rf '" + root + "'").c_str()) != 0) {}
    const std::string parts[] = { root, root + "/InvestorRT", root + "/InvestorRT/rtx", root + "/InvestorRT/rtx/lsFlexLevels", root + "/InvestorRT/rtx/lsFlexLevels/DeltaProfile" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", root.c_str(), 1);
    { std::ofstream o((root + "/InvestorRT/rtx/lsFlexLevels/DeltaProfile/ES-ESZ6-180-signals-20741.csv").c_str(), std::ios::binary); o << out; }
    DeltaProfile p; mock::current = &h; p.draw();
    ChartState& s = *static_cast<ChartState*>(h.data);
    LatchBook& bk = latchBooks().begin()->second;
    std::cout << "record: " << aRecords << " A decision lines; book " << bk.v.size() << " events, loaded " << bk.loaded << ", rejected " << bk.rejected << ", session " << bk.session << "\n\n";
    // the rule-by-rule table
    std::vector<const Latch*> A; for (const Latch& L : bk.v) if (confirmedAbsorption(L)) A.push_back(&L);
    std::sort(A.begin(), A.end(), [](const Latch* a, const Latch* b) { return a->peakT != b->peakT ? a->peakT < b->peakT : a->id < b->id; });
    std::cout << "id|peak bar|side|price|decided|ver|close beyond|halfway|pivot (pivot bar)|2.8.4 re-check|one-per-candle|2.8.5 shows\n";
    std::vector<double> O, H, Lo, C; for (const mock::Bar& b : h.bars) { O.push_back(b.open); H.push_back(b.high); Lo.push_back(b.low); C.push_back(b.close); }
    int shown = 0, hidden = 0, merged = 0, nobars = 0, sens = 0, sensSame = 0;
    for (const Latch* Lp : A) {
        const Latch& L = *Lp; const std::map<long long, dp::Recheck>::const_iterator rc = bk.recheck.find(L.id);
        const std::string d = s.bubbleState.count(L.id) ? s.bubbleState[L.id] : std::string("?");
        std::string piv = "-";
        if (rc != bk.recheck.end() && rc->second.pivotBar >= 0) piv = hhmm(static_cast<long long>(h.bars[static_cast<std::size_t>(rc->second.pivotBar)].time));
        std::cout << L.id << '|' << hhmm(L.peakT) << '|' << (L.support ? "bull" : "bear") << '|' << L.price << '|' << hhmm(L.tDecided) << '|' << L.version << '|';
        if (rc == bk.recheck.end()) std::cout << "-|-|-|bars not on chart|";
        else std::cout << (rc->second.close ? "ok" : "FAIL") << '|' << (rc->second.halfway ? "ok" : "FAIL") << '|' << (rc->second.pivot ? "ok" : "FAIL") << " (" << piv << ")|" << (rc->second.pass() ? "pass" : "fail") << '|';
        std::string one = d.compare(0, 7, "merged:") == 0 ? "joins id " + d.substr(7) : d.compare(0, 6, "hidden") == 0 || d.compare(0, 7, "undrawn") == 0 ? "-" : "holds the bubble";
        std::cout << one << '|' << d << '\n';
        if (d == "drawn") ++shown; else if (d.compare(0, 6, "hidden") == 0) ++hidden; else if (d.compare(0, 7, "merged:") == 0) ++merged; else ++nobars;
        // pivot sensitivity: the same verdict when the chart starts 6 / 12 / 24 / 36 / 48 / 60 bars later?
        int pk = -1; for (std::size_t i = 0; i < h.bars.size(); ++i) if (h.bars[i].time == L.peakTime) pk = static_cast<int>(i);
        if (pk > 0 && rc != bk.recheck.end()) for (int off : { 6, 12, 24, 36, 48, 60 }) {
            if (off >= pk - 1) continue;
            std::vector<double> o2(O.begin() + off, O.end()), h2(H.begin() + off, H.end()), l2(Lo.begin() + off, Lo.end()), c2(C.begin() + off, C.end());
            const dp::Recheck q = dp::recheckA(o2, h2, l2, c2, static_cast<int>(o2.size()), pk - off, L.low, L.high, 0.25, L.sell);
            ++sens; if (q.pivot == rc->second.pivot) ++sensSame;
        }
    }
    std::cout << "\nA records " << A.size() << ": shown " << shown << ", hidden by the pre-2.8.4 re-check " << hidden << ", merged (one per candle) " << merged << ", undrawn (no bars) " << nobars << '\n';
    std::cout << "pivot verdict with the chart starting later: " << sensSame << " / " << sens << " the same\n";
    std::cout << "bubbles drawn: " << s.bubblesDrawn << " (offscreen " << s.bubblesOffscreen << ", suppressed " << s.bubblesSuppressed << ")\n";
    for (const mock::Draw& d : h.draws) if (d.type == "ring") std::cout << "  ring x " << d.x << " y " << d.y << '\n';
    const std::string st = slurp(root + "/InvestorRT/rtx/lsFlexLevels/DeltaProfile.status.txt");
    std::istringstream si(st); std::string line;
    while (std::getline(si, line)) if (line.compare(0, 8, "BUBBLES_") == 0 || line.compare(0, 2, "A_") == 0 || line.compare(0, 9, "RECHECK_P") == 0 || line.compare(0, 7, "VERSION") == 0) std::cout << "status> " << line << '\n';
    mock::current = &h; p.destroy(); latchBooks().clear();
    return 0;
}
