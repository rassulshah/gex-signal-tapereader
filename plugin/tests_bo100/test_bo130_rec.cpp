// (lsBreakout 1.6.0) BI removed: "| bi 0", no # BBI header, no BBI| / bi_cancel lines; version 1.8.0-zz3
// (lsBreakout 1.5.0) version 1.8.0-zz3; the breakouts file's first line adds "| bi 1", a third line "# BBI|..." and BBI| lines (one per BI, key t_bi + dir + pivot); a BO / BD a BI cancelled is written fired 0 / bi_cancel
// test_bo130_rec.cpp -- lsBreakout 1.4.0 (2026-11-04: header + status say zz_carry 1, version 1.4.0-zz3) / 1.3.0 (2026-11-03 evening) RECORDS suite: Breakout.cpp on the mock SDK writes the BO / BD, Rly and Pb
// decision records into lsFlexLevels\Breakout (lsBigBidAsk was removed from his charts; the nightly reads these now).
//   (in plugin\tests_bo100) g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -IBreakout_test_support -I.. test_bo130_rec.cpp
// Covers (real ES Oct 28 - 30 3-min candles, es_2026-10-28_30_3m.txt):
//   [1] file names = lsBigBidAsk's (<MKT>-<SYM>-<spb>-breakouts-<daykey>.csv, Rly\<MKT>-<SYM>-<spb>-<daykey>.csv) + the Pb file, in the
//       Breakout folder; first line "# lsBreakout 1.3.0 | ... | zz_rev_bars 3 | ft_tie 1", then the column header;
//   [2] every line parses with lsBigBidAsk's own parsers (bba::parseBoLine 27 fields, rly::parseLine 17 fields) / the Pb layout (14),
//       version field "1.3.0-zz3"; ONE line per decision of the last 3 sessions and every such decision written (vs the engine's maps);
//   [3] no duplicate on a restart (a new DLL object: files byte-identical), on a recompute (history changed -> full pass), on a
//       bar-by-bar feed then a restart, with two charts of the same market on one DLL object and two DLL objects on the same chart;
//   [4] a new decision is appended exactly once (bar-by-bar: each line appears when decided, never again);
//   [5] a failed write (the file path blocked) is counted, nothing marked written, retried and written once when it clears;
//   [6] lsBigBidAsk's Oct 30 bug pattern (a file already holding a line) is not repeated: a key already in the file is never appended.
#include "irtsdk.h"
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
#include "Breakout.cpp"

static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) { ++passed; std::cout << "PASS " << t << "\n"; } else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
struct Row { RTDATE t; float o, h, l, c; unsigned long v; };
static std::vector<Row> load(const char* f) {
    std::ifstream in(f); std::string l; std::vector<Row> v;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        Row r; r.t = (RTDATE)(svl::daysFromCivil(y, mo, d) * 86400LL + h * 3600 + mi * 60 + s); r.o = (float)o; r.h = (float)hi; r.l = (float)lo; r.c = (float)c; r.v = (unsigned long)vo; v.push_back(r); }
    return v;
}
static void chart(mock::Host& H, const std::vector<Row>& R, int upto, int from = 0) {
    for (int a = 0; a < 4; ++a) H.f[a].clear();
    H.t.clear(); H.vol.clear();
    for (int i = from; i <= upto; ++i) { H.f[barOpen].push_back(R[i].o); H.f[barHigh].push_back(R[i].h); H.f[barLow].push_back(R[i].l); H.f[barClose].push_back(R[i].c); H.t.push_back(R[i].t); H.vol.push_back(R[i].v); }
    const int first = std::max(0, (int)H.t.size() - 260);
    float lo = 1e9f, hi = -1e9f; for (int i = first; i < (int)H.t.size(); ++i) { lo = std::min(lo, H.f[barLow][(size_t)i]); hi = std::max(hi, H.f[barHigh][(size_t)i]); }
    H.lo = lo - 6; H.hi = hi + 6; H.first = first; H.paneR = 10 * ((int)H.t.size() - first) + H.rightMargin; H.paneB = 900; H.root = "ES"; H.sym = "ESZ26";
}
static std::string home, flex;
static std::string slurp(const std::string& p) { std::ifstream f(p, std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void pass(Breakout& x) { x.calc(0); mock::current->drawing = true; x.draw(); mock::current->drawing = false; mock::current->drawn.clear(); }
static void rmrf(const std::string& d) { DIR* D = opendir(d.c_str()); if (!D) { std::remove(d.c_str()); return; } struct dirent* e; while ((e = readdir(D))) { const std::string n = e->d_name; if (n == "." || n == "..") continue; rmrf(d + "/" + n); } closedir(D); rmdir(d.c_str()); }
static std::vector<std::string> list(const std::string& d) { std::vector<std::string> v; DIR* D = opendir(d.c_str()); if (!D) return v; struct dirent* e; while ((e = readdir(D))) { const std::string n = e->d_name; if (n.size() > 4 && n.substr(n.size() - 4) == ".csv") v.push_back(n); } closedir(D); std::sort(v.begin(), v.end()); return v; }
static std::vector<std::string> linesOf(const std::string& text) { std::vector<std::string> v; std::istringstream in(text); std::string l; while (std::getline(in, l)) v.push_back(l); return v; }
// every record file's text (rel path -> text)
static std::map<std::string, std::string> snapshot() {
    std::map<std::string, std::string> m;
    for (const auto& f : list(flex + "/Breakout")) m["Breakout/" + f] = slurp(flex + "/Breakout/" + f);
    for (const auto& f : list(flex + "/Breakout/Rly")) m["Breakout/Rly/" + f] = slurp(flex + "/Breakout/Rly/" + f);
    return m;
}
// total record lines and duplicate keys over all files
static void count(const std::map<std::string, std::string>& S, int& lines, int& dups, int& bad) {
    lines = dups = bad = 0;
    for (const auto& kv : S) { std::set<std::string> keys; for (const auto& l : linesOf(kv.second)) { if (l.empty() || l[0] == '#' || l.find("|t") == 3) continue;
        const std::string k = bko::recKey(l); if (k.empty()) { ++bad; continue; } ++lines; if (!keys.insert(k).second) ++dups; } }
}
static Breakout* fresh() { Breakout* x = static_cast<Breakout*>(CreateExtension()); x->init(); x->setup(); return x; }

int main() {
    char tmpl[] = "/tmp/bo130XXXXXX"; home = mkdtemp(tmpl); setenv("USERPROFILE", home.c_str(), 1);
    mkdir((home + "/InvestorRT").c_str(), 0755); mkdir((home + "/InvestorRT/rtx").c_str(), 0755);
    flex = home + "/InvestorRT/rtx/lsFlexLevels";
    const std::vector<Row> R = load("es_2026-10-28_30_3m.txt"); const int N = (int)R.size() - 1;
    CHECK(R.size() > 1000, "fixture loaded (" + std::to_string(R.size()) + " candles)");
    // ---- [1] / [2] one chart, one pass ----
    mock::Host H; mock::current = &H; chart(H, R, N);
    Breakout* x = fresh(); pass(*x);
    const BState* S = x->testState();
    const std::map<std::string, std::string> s1 = snapshot();
    const long lastDay = (long)(R[(size_t)N - 1].t / 86400);   // the last closed bar's civil day (all ES Oct 30 RTH)
    const long long sLast = bko::sessOf(lastDay, (int)(R[(size_t)N - 1].t % 86400)), minS = sLast - 2;
    std::set<std::string> names; for (const auto& kv : s1) names.insert(kv.first);
    bool nm = true; for (long long d = minS; d <= sLast; ++d) { const std::string k = std::to_string(d);
        if (!names.count("Breakout/ES-ESZ26-180-breakouts-" + k + ".csv") || !names.count("Breakout/ES-ESZ26-180-pb-" + k + ".csv") || !names.count("Breakout/Rly/ES-ESZ26-180-" + k + ".csv")) nm = false; }
    CHECK(nm && names.size() == 9, "[1] lsBigBidAsk's names in the Breakout folder: breakouts / pb / Rly x the last 3 daykeys (" + std::to_string(minS) + " .. " + std::to_string(sLast) + "), " + std::to_string(names.size()) + " files");
    bool heads = true;
    for (const auto& kv : s1) { const auto L = linesOf(kv.second); const bool rl = kv.first.find("/Rly/") != std::string::npos, pb = kv.first.find("-pb-") != std::string::npos;
        if (L.size() < 2 || L[0].rfind("# lsBreakout 1.8.0 | ES ESZ26 180 | zz_rev_bars 3 | ft_tie 1 | zz_carry 1 | ", 0) != 0) heads = false;
        if (!rl && !pb && (L.size() < 3 || L[0].find(" | bi 0 | ") == std::string::npos || L[2].rfind("# ", 0) == 0)) heads = false;   // (1.6.0) bi 0, no # BBI header
        if (L.size() >= 2 && L[1] != (rl ? rly::REC_HEADER : pb ? bko::PB_HEADER : bba::BO_HEADER)) heads = false; }
    CHECK(heads, "[1] first line names lsBreakout 1.8.0, the market, zz_rev_bars 3, ft_tie 1, zz_carry 1 (breakouts: bi 0); the second is lsBigBidAsk's column header; (1.6.0) no # BBI header");
    // [2] parse every line with lsBigBidAsk's own parsers; one line per decision; every decision of the last 3 sessions written
    int nb = 0, nr = 0, np = 0, ni = 0, nc = 0, badP = 0; std::set<long long> bt0, rkey, ptp, ikey;
    for (const auto& kv : s1) for (const auto& l : linesOf(kv.second)) {
        if (l.rfind("BBO|", 0) == 0 && l.find("|0|bi_cancel|") != std::string::npos) { bba::BoDec D;   // (1.5.0) a cancelled BO / BD: the fired line with fired 0 / bi_cancel
            std::string r = l; r.replace(r.find("|0|bi_cancel|"), 13, "|1|fired|");
            if (!bba::parseBoLine(r, D) || l.find("|1.8.0-zz3|") == std::string::npos || !S->E.cancelled(D.t0)) ++badP; else { ++nb; ++nc; bt0.insert(D.t0);
                const auto it = S->E.bo.find(D.t0); if (it == S->E.bo.end() || !it->second.fired || it->second.t != D.t) ++badP; } }
        else if (l.rfind("BBI|", 0) == 0) { std::vector<std::string> f; size_t a = 0; for (;;) { const size_t q = l.find('|', a); f.push_back(l.substr(a, q == std::string::npos ? q : q - a)); if (q == std::string::npos) break; a = q + 1; }
            if (f.size() != 22 || f[21] != "1.8.0-zz3" || bko::recKey(l).empty()) { ++badP; continue; } ++ni;
            const long long t = std::atoll(f[1].c_str()); const int d = std::atoi(f[2].c_str()); bool found = false;
            for (const auto& kv : S->E.bi) if (kv.second.t == t && kv.second.dir == d && std::atoll(f[3].c_str()) == kv.second.t0 && bko::biLine(kv.second, "1.8.0-zz3") == l) { found = true; ikey.insert(kv.first); }
            if (!found) ++badP; }
        else if (l.rfind("BBO|", 0) == 0 && l.rfind("BBO|t|", 0) != 0) { bba::BoDec D;
            if (!bba::parseBoLine(l, D) || D.rule != 5 || l.find("|1.8.0-zz3|") == std::string::npos) ++badP; else { ++nb; bt0.insert(D.t0);
                const auto it = S->E.bo.find(D.t0); if (it == S->E.bo.end() || it->second.dir != D.dir || it->second.fired != D.fired || it->second.t != D.t || it->second.reason != D.reason) ++badP; } }
        else if (l.rfind("RLY|", 0) == 0 && l.rfind("RLY|t_", 0) != 0) { rly::Rec Q; if (!rly::parseLine(l, Q) || l.substr(l.rfind('|') + 1) != "1.8.0-zz3") ++badP; else { ++nr; rkey.insert(Q.te * 2 + (Q.dir > 0 ? 1 : 0));
                const auto it = S->E.rly.find(Q.te * 2 + (Q.dir > 0 ? 1 : 0)); if (it == S->E.rly.end() || it->second.fired != Q.fired) ++badP; } }
        else if (l.rfind("BPB|", 0) == 0 && l.rfind("BPB|t_", 0) != 0) { std::vector<std::string> f; size_t a = 0; for (;;) { const size_t q = l.find('|', a); f.push_back(l.substr(a, q == std::string::npos ? q : q - a)); if (q == std::string::npos) break; a = q + 1; }
            if (f.size() != 14 || f[13] != "1.8.0-zz3") { ++badP; continue; } ++np; const long long tp = std::atoll(f[1].c_str()); ptp.insert(tp);
            const auto it = S->E.pb.find(tp); const auto bo = it == S->E.pb.end() ? S->E.bo.end() : S->E.bo.find(it->second.t0);
            if (it == S->E.pb.end() || std::atoi(f[2].c_str()) != it->second.dir || std::atoll(f[3].c_str()) != it->second.t0 || bo == S->E.bo.end() || std::atoll(f[4].c_str()) != bo->second.t
                || std::atoll(f[5].c_str()) != it->second.tc || std::atoll(f[6].c_str()) != it->second.td || std::atof(f[12].c_str()) != bo->second.piv) ++badP; } }
    int wantB = 0, wantR = 0, wantP = 0;
    for (const auto& kv : S->E.boL) if (kv.second.sess >= minS) { ++wantB; if (!bt0.count(kv.first)) ++badP; }
    for (const auto& kv : S->E.rlyL) if (kv.second.sess >= minS) { ++wantR; if (!rkey.count(kv.first)) ++badP; }
    for (const auto& kv : S->E.pbL) if (kv.second.sess >= minS) { ++wantP; if (!ptp.count(kv.first)) ++badP; }
    int wantI = 0; for (const auto& kv : S->E.biL) if (kv.second.sess >= minS) { ++wantI; if (!ikey.count(kv.first)) ++badP; }
    CHECK(ni == 0 && wantI == 0 && nc == 0, "[2] (1.6.0) BI removed: no BBI| line, no bi_cancel BO line (" + std::to_string(ni) + " / " + std::to_string(wantI) + ", cancelled BO lines " + std::to_string(nc) + ")");
    CHECK(badP == 0 && nb == wantB && nr == wantR && np == wantP && nb > 50 && nr > 3 && np > 10,
          "[2] every line parses (bba::parseBoLine / rly::parseLine / 14-field Pb / 22-field BBI, version 1.8.0-zz3) and equals the engine's decision; one per decision: BO / BD "
          + std::to_string(nb) + " / " + std::to_string(wantB) + ", Rly " + std::to_string(nr) + " / " + std::to_string(wantR) + ", Pb " + std::to_string(np) + " / " + std::to_string(wantP) + " (" + std::to_string(badP) + " bad)");
    { int L = 0, D = 0, B = 0; count(s1, L, D, B); CHECK(D == 0 && B == 0, "[2] no duplicate key in any file (" + std::to_string(L) + " lines)"); }
    { const std::string st = slurp(flex + "/Breakout.status-ES-180.txt"); CHECK(st.rfind("VERSION,1.8.0\n", 0) == 0 && st.find("RECORDS,lsFlexLevels\\Breakout (ES-ESZ26-180),zz_rev_bars 3,ft_tie 1,zz_carry 1,bi 0,lines written " + std::to_string(nb + nr + np + ni) + ",pending 0,write fails 0") != std::string::npos, "status RECORDS line: folder, zz 3, tie 1, carry 1, bi 1, lines written, 0 pending / fails"); }
    // more passes with nothing new: nothing written
    pass(*x); pass(*x); CHECK(snapshot() == s1, "[3] repeated passes with no new bar write nothing");
    x->destroy(); delete x;
    // ---- [3] restart: a new DLL object on the same chart ----
    { mock::Host G; mock::current = &G; chart(G, R, N); Breakout* y = fresh(); pass(*y); CHECK(snapshot() == s1, "[3] restart (new DLL object, one full pass): every file byte-identical, no line repeated");
      // recompute: the history changes (first 50 bars gone) -> a full pass again on the same object
      chart(G, R, N, 50); pass(*y); { int L = 0, D = 0, B = 0; count(snapshot(), L, D, B); CHECK(D == 0 && B == 0 && snapshot() == s1, "[3] recompute (history changed -> full pass): no duplicate, files unchanged (" + std::to_string(L) + " lines)"); }
      y->destroy(); delete y; }
    // ---- [3] two charts of the same market on ONE DLL object, and two DLL objects on the same chart ----
    { mock::Host A, B; mock::current = &A; chart(A, R, N); Breakout* z = fresh(); pass(*z); mock::current = &B; chart(B, R, N); pass(*z);
      Breakout* w = fresh(); pass(*w); mock::current = &A; pass(*w); pass(*z);
      CHECK(snapshot() == s1, "[3] two charts on one DLL object + a second DLL object on both charts: files byte-identical");
      mock::current = &B; z->destroy(); w->destroy(); mock::current = &A; z->destroy(); w->destroy(); delete z; delete w; }
    // ---- [4] bar by bar from empty: each decision appended once when decided; then a restart and a second chart ----
    rmrf(flex + "/Breakout");
    {   mock::Host G; mock::current = &G; Breakout* y = fresh();
        int start = 0; for (int i = 0; i < (int)R.size(); ++i) if (R[i].t % 86400 == 17 * 3600 + 180 && i > 600) { start = i; break; }
        int grewBad = 0; int prevL = 0;
        for (int u = start; u <= N; ++u) { chart(G, R, u); y->calc(0);
            int L = 0, D = 0, B = 0; count(snapshot(), L, D, B); if (D || B || L < prevL) ++grewBad; prevL = L; }
        const std::map<std::string, std::string> live = snapshot(); int L = 0, D = 0, B = 0; count(live, L, D, B);
        CHECK(grewBad == 0 && D == 0 && L > 50, "[4] bar by bar from the Oct 29 17:03 bar: lines only ever appended, never a duplicate (" + std::to_string(L) + " lines)");
        y->destroy(); delete y;
        y = fresh(); pass(*y); int L2 = 0, D2 = 0, B2 = 0; count(snapshot(), L2, D2, B2);
        CHECK(D2 == 0 && B2 == 0, "[4] a restart after the live feed adds no duplicate (" + std::to_string(L) + " -> " + std::to_string(L2) + " lines)");
        bool kept = true; const auto now = snapshot(); for (const auto& kv : live) { const auto it = now.find(kv.first); if (it == now.end() || it->second.compare(0, kv.second.size(), kv.second) != 0) kept = false; }
        CHECK(kept, "[4] the restart only appends: every live line stays as written (first line per decision wins)");
        y->destroy(); delete y; }
    // ---- [5] a failed write: the breakouts file path blocked by a directory -> counted, retried after 1 s, written once ----
    rmrf(flex + "/Breakout");
    {   mkdir(flex.c_str(), 0755); mkdir((flex + "/Breakout").c_str(), 0755);
        const std::string blk = flex + "/Breakout/ES-ESZ26-180-breakouts-" + std::to_string(sLast) + ".csv"; mkdir(blk.c_str(), 0755); mkdir((blk + "/x").c_str(), 0755);
        mock::Host G; mock::current = &G; chart(G, R, N); Breakout* y = fresh(); pass(*y);
        const BState* T = y->testState(); CHECK(T->recFails >= 1 && T->recPend > 0, "[5] a blocked file: write fail counted, its lines kept pending (" + std::to_string(T->recPend) + ")");
        rmrf(blk); usleep(1100 * 1000); pass(*y);
        int L = 0, D = 0, B = 0; const auto now = snapshot(); count(now, L, D, B);
        CHECK(T->recPend == 0 && D == 0 && now == s1, "[5] once clear, retried and written once: files identical to the clean run");
        y->destroy(); delete y; }
    // ---- [6] a file that already holds some of the lines (lsBigBidAsk's Oct 30 pattern): those keys are never appended again ----
    {   const std::string p = "Breakout/ES-ESZ26-180-breakouts-" + std::to_string(sLast) + ".csv"; const auto L = linesOf(s1.at(p));
        std::string part; for (size_t i = 0; i < L.size() / 2; ++i) part += L[i] + "\n";
        rmrf(flex + "/Breakout"); mkdir((flex + "/Breakout").c_str(), 0755); { std::ofstream f(flex + "/" + p, std::ios::binary); f << part; }
        mock::Host G; mock::current = &G; chart(G, R, N); Breakout* y = fresh(); pass(*y);
        CHECK(slurp(flex + "/" + p) == s1.at(p), "[6] a half-written file is completed: the lines already there are not repeated, the rest appended (= the clean file)");
        std::set<std::string> in; int added = 0; const std::string t2 = bko::recMerge(s1.at(p), "H\n", linesOf(s1.at(p)), in, added);
        CHECK(added == 0 && t2 == s1.at(p), "[6] recMerge of a file with every line again: 0 appended, text unchanged");
        const std::string t3 = bko::recMerge("", "H\n", { L[3], L[3], L[4] }, in, added);   // (1.6.0) no BBI header: L[2] onward are decision lines
        CHECK(added == 2 && t3 == "H\n" + L[3] + "\n" + L[4] + "\n", "[6] recMerge: a new file starts with its header, a repeated line in one batch is written once");
        CHECK(bko::recKey("BBO|t|t0|dir") == "" && bko::recKey("# x") == "" && bko::recKey("garbage") == "" && bko::recKey(L[2]) != "" && bko::recKey(L[3]) != "" && bko::recKey(std::string("# ") + bko::BI_HEADER) == "", "[6] recKey: header / comment / garbage lines have no key");
        y->destroy(); delete y; }
    // market names = lsBigBidAsk's
    CHECK(bko::recMarket("EP") == "ES" && bko::recMarket("6E") == "EU" && bko::recMarket("QM") == "CL" && bko::recMarket("MNQ") == "NQ" && bko::recFileKey("ESZ26") == "ESZ26" && bko::recFileKey("es z6") == "ES_Z6", "recMarket / recFileKey = lsBigBidAsk's marketName / fileKey");
    std::printf("records left in %s\n", flex.c_str());
    std::printf("RESULT %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
