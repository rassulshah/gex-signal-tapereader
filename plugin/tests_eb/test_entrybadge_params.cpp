// test_entrybadge_params.cpp -- EntryBadge.h 1.1.0 (lsDealerProfile / lsLimitProfile 2.57.0, B-20261109-LQ1, Rassul 17:28 CT "Ok"):
// the nightly LRA-EntryBadge-<MKT>.csv (lra.liq_learn) turns each badge ON / ? / OFF per market; missing / garbled / stale / future = defaults.
// From plugin\: g++ -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -I. tests_eb/test_entrybadge_params.cpp -o tebp && ./tebp (run from tests_eb)
#include "EntryBadge.h"
#include <cstdio>
#include <fstream>
#include <set>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
struct Case { std::string m; double tick = 0, atr = 0; ebd::Lbl L; std::vector<ebd::Bar> B; std::set<long long> pull; std::vector<ebd::Lvl> V; std::vector<ebd::Book> K; };
static std::vector<std::string> split(const std::string& s) { std::vector<std::string> v; std::string c; for (char ch : s) { if (ch == '|') { v.push_back(c); c.clear(); } else c += ch; } v.push_back(c); return v; }
static std::vector<Case> load(const char* f) {
    std::vector<Case> out; std::ifstream in(f); std::string l; Case C;
    while (std::getline(in, l)) { const std::vector<std::string> p = split(l);
        if (p[0] == "CASE") { C = Case(); C.m = p[2]; C.tick = std::stod(p[3]); C.atr = std::stod(p[4]); }
        else if (p[0] == "L") { C.L.td = std::stoll(p[1]); C.L.side = std::stoi(p[2]); C.L.code = std::stoi(p[3]); C.L.pbPoc = std::stod(p[4]); C.L.stop = std::stod(p[5]); C.L.target = std::stod(p[6]); }
        else if (p[0] == "B") { ebd::Bar b; b.t = std::stoll(p[1]); b.h = std::stod(p[2]); b.l = std::stod(p[3]); b.c = std::stod(p[4]); C.B.push_back(b); if (p[5] == "1") C.pull.insert(b.t); }
        else if (p[0] == "V") C.V.push_back(ebd::Lvl{ std::stod(p[1]), p[2], p[3] == "1" });
        else if (p[0] == "K") C.K.push_back(ebd::Book{ std::stod(p[1]), std::stoll(p[2]), p[3] == "1" });
        else if (p[0] == "END") out.push_back(C); }
    return out;
}
static size_t idxOf(const Case& C, long long t) { for (size_t k = 0; k < C.B.size(); ++k) if (C.B[k].t == t) return k; return C.B.size(); }
static ebd::Out run(const Case& C, size_t n, const ebd::Par& P, const std::string& mkt = "") {
    std::vector<ebd::Bar> B(C.B.begin(), C.B.begin() + (long)std::min(n, C.B.size()));
    return ebd::compute(mkt.empty() ? C.m : mkt, B, std::vector<ebd::Lbl>{ C.L }, C.V, C.K, C.tick, C.atr, [&](long long t, int) { return C.pull.count(t) != 0; }, P);
}
static const ebd::Tag* tagOf(const ebd::Out& o, int k) { for (const ebd::Tag& t : o.tags) if (t.kind == k) return &t; return nullptr; }
static std::string file(const std::string& body, const char* ts = "2026-11-09T22:30:00") {
    return std::string("# LRA-EntryBadge-XX.csv - liq_learn 1.0.0 ") + ts + " - nightly\n" + body; }
int main() {
    const long long NOW = ebd::civilDays(2026, 11, 10) * 86400 + 8 * 3600;   // 08:00 the next morning
    // ---- parse
    { ebd::Par p = ebd::parse("ES", "", NOW); CHECK(!p.fromFile && p.entPdoc == ebd::ST_ON && p.why.find("no file") != std::string::npos, "missing file -> ES defaults (ENT_PDOC ON)"); }
    { ebd::Par p = ebd::parse("CL", "garbage\x01\x02 not a csv\n,,,\nstatus\n", NOW); CHECK(!p.fromFile && p.entGamma == ebd::ST_Q && p.exitBook == ebd::ST_Q, "garbage -> CL defaults"); }
    { ebd::Par p = ebd::parse("ES", file("status,ENT_PDOC,OFF\n"), NOW); CHECK(p.fromFile && p.entPdoc == ebd::ST_OFF, "ES ENT_PDOC OFF read"); }
    { ebd::Par p = ebd::parse("ES", file("status,ENT_PDOC,?\r\nstatus,ENT_GAMMA,ON\r\n"), NOW); CHECK(p.fromFile && p.entPdoc == ebd::ST_Q && p.entGamma == ebd::ST_ON, "CRLF + ? + ON read"); }
    { ebd::Par p = ebd::parse("ES", file("status,ENT_PDOC,maybe\nstatus,SCALE,ON\n"), NOW); CHECK(p.fromFile && p.entPdoc == ebd::ST_ON && p.scale == ebd::ST_ON, "garbled value keeps that badge's default"); }
    { ebd::Par p = ebd::parse("ES", "status,ENT_PDOC,OFF\n", NOW); CHECK(!p.fromFile && p.entPdoc == ebd::ST_ON, "no header time -> defaults"); }
    { ebd::Par p = ebd::parse("ES", file("status,ENT_PDOC,OFF\n", "2026-10-30T22:30:00"), NOW); CHECK(!p.fromFile && p.entPdoc == ebd::ST_ON && p.why.find("stale") != std::string::npos, "11 days old -> defaults"); }
    { ebd::Par p = ebd::parse("ES", file("status,ENT_PDOC,OFF\n", "2026-11-03T22:30:00"), NOW); CHECK(p.fromFile && p.entPdoc == ebd::ST_OFF, "6 days old (a long weekend) still used"); }
    { ebd::Par p = ebd::parse("ES", file("status,ENT_PDOC,OFF\n", "2026-11-12T22:30:00"), NOW); CHECK(!p.fromFile, "future header -> defaults"); }
    { ebd::Par p = ebd::parse("ES", std::string(70000, 'x'), NOW); CHECK(!p.fromFile, "oversize -> defaults"); }
    { ebd::Par p = ebd::parse("ES", file("pull_x,1.5\nquiet_rth_s,60\n"), NOW); CHECK(!p.fromFile, "no status line -> defaults"); }
    { ebd::Par p = ebd::parse("NQ", "", NOW); CHECK(p.entPdoc == ebd::ST_OFF && p.entGamma == ebd::ST_OFF && p.exitBook == ebd::ST_OFF && p.scale == ebd::ST_OFF, "NQ defaults all OFF"); }
    { ebd::Par p = ebd::parse("6E", "", NOW); ebd::Par q = ebd::parse("EU", "", NOW); CHECK(p.entPdoc == q.entPdoc && p.scale == q.scale, "6E = EU"); }
    // ---- compute on the real study cases
    const std::vector<Case> cs = load("cases.txt"); CHECK(cs.size() == 5, "5 study cases");
    const Case* es = nullptr; const Case* cl = nullptr; const Case* gc = nullptr; const Case* ng = nullptr;
    for (const Case& c : cs) { if (c.m == "ES" && !es) es = &c; else if (c.m == "CL") cl = &c; else if (c.m == "GC") gc = &c; else if (c.m == "NG") ng = &c; }
    CHECK(es && cl && gc && ng, "cases found");
    if (es && cl && gc && ng) {
        const size_t ie = idxOf(*es, es->L.td) + 1;   // right after the decision bar
        ebd::Par d = ebd::defaults("ES");
        { const ebd::Out o = run(*es, ie + 1, d); const ebd::Tag* t = tagOf(o, 1); CHECK(t && t->text == "ENT Pdoc", "ES defaults = 1.0.0 ENT Pdoc"); }
        { ebd::Par p = d; p.entPdoc = ebd::ST_Q; const ebd::Out oo = run(*es, ie + 1, p); const ebd::Tag* t = tagOf(oo, 1); CHECK(t && t->text == "ENT Pdoc ?", "ES ? -> ENT Pdoc ?"); }
        { ebd::Par p = d; p.entPdoc = ebd::ST_OFF; CHECK(run(*es, ie + 1, p).tags.empty(), "ES OFF -> nothing"); }
        { ebd::Par p = ebd::defaults("NQ"); p.entPdoc = ebd::ST_ON; const ebd::Out oo = run(*es, ie + 1, p, "NQ"); const ebd::Tag* t = tagOf(oo, 1); CHECK(t && t->text == "ENT Pdoc", "a market turned ON by the nightly gets the badge (NQ)"); }
        { CHECK(run(*es, ie + 1, ebd::defaults("NQ"), "NQ").tags.empty(), "NQ defaults -> nothing"); }
        // CL: full run (filled) -> ENT ? + EXIT ?; EXIT_BOOK OFF -> no EXIT; ENT_GAMMA ON -> "ENT"
        { const ebd::Out o0 = run(*cl, cl->B.size(), ebd::defaults("CL")); const ebd::Out o = o0;
          size_t best = 0; bool hadExit = false; ebd::Out ex;
          for (size_t n = idxOf(*cl, cl->L.td) + 2; n <= cl->B.size(); ++n) { ebd::Out q = run(*cl, n, ebd::defaults("CL")); if (tagOf(q, 2)) { hadExit = true; best = n; ex = q; break; } }
          CHECK(hadExit && tagOf(ex, 2)->text == "EXIT ?" && tagOf(ex, 1)->text == "ENT ?", "CL defaults = ENT ? + EXIT ?");
          if (hadExit) { ebd::Par p = ebd::defaults("CL"); p.exitBook = ebd::ST_OFF; ebd::Out q = run(*cl, best, p); CHECK(!tagOf(q, 2) && tagOf(q, 1), "CL EXIT_BOOK OFF -> ENT only");
                         p = ebd::defaults("CL"); p.entGamma = ebd::ST_ON; p.exitBook = ebd::ST_ON; q = run(*cl, best, p); CHECK(tagOf(q, 1) && tagOf(q, 1)->text == "ENT" && tagOf(q, 2) && tagOf(q, 2)->text == "EXIT", "CL ON -> ENT / EXIT without ?"); }
          (void)o; }
        // GC: scale ON default; ? -> "1/2 ?"; OFF -> nothing; ENT on -> the entry path replaces the scale-out
        { const size_t ig = idxOf(*gc, gc->L.td) + 1;
          const ebd::Out o = run(*gc, ig + 1, ebd::defaults("GC")); CHECK(tagOf(o, 3) && tagOf(o, 3)->text == "1/2" && tagOf(o, 4) && tagOf(o, 4)->text == "rest", "GC defaults = 1/2 + rest");
          ebd::Par p = ebd::defaults("GC"); p.scale = ebd::ST_Q; const ebd::Out q = run(*gc, ig + 1, p); CHECK(tagOf(q, 3) && tagOf(q, 3)->text == "1/2 ?", "GC ? -> 1/2 ?");
          p.scale = ebd::ST_OFF; CHECK(run(*gc, ig + 1, p).tags.empty(), "GC OFF -> nothing");
          p = ebd::defaults("GC"); p.entPdoc = ebd::ST_Q; const ebd::Out r = run(*gc, ig + 1, p); CHECK(!tagOf(r, 3) && !tagOf(r, 4), "GC: an ENT badge ON replaces the scale-out"); }
        // NG default ? ; ON from the nightly -> no ?
        { const size_t in = idxOf(*ng, ng->L.td) + 1; const ebd::Out o1 = run(*ng, in + 1, ebd::defaults("NG")); const ebd::Tag* t = tagOf(o1, 1); CHECK(t && t->text == "ENT Pdoc ?", "NG defaults = ENT Pdoc ?");
          ebd::Par p = ebd::defaults("NG"); p.entPdoc = ebd::ST_ON; const ebd::Out o2 = run(*ng, in + 1, p); const ebd::Tag* u = tagOf(o2, 1); CHECK(u && u->text == "ENT Pdoc", "NG promoted ON -> ENT Pdoc"); }
        // the defaults path equals the 1.0.0 call form on every case and every bar count (no behaviour change without a file)
        { bool same = true; for (const Case& c : cs) for (size_t n = 2; n <= c.B.size(); ++n) {
              std::vector<ebd::Bar> B(c.B.begin(), c.B.begin() + (long)n);
              const ebd::Out a = ebd::compute(c.m, B, std::vector<ebd::Lbl>{ c.L }, c.V, c.K, c.tick, c.atr, [&](long long t, int) { return c.pull.count(t) != 0; });
              const ebd::Out b = run(c, n, ebd::defaults(c.m)); if (a.tags.size() != b.tags.size()) same = false; else for (size_t k = 0; k < a.tags.size(); ++k) if (a.tags[k].text != b.tags[k].text || a.tags[k].px != b.tags[k].px) same = false; }
          CHECK(same, "defaults path == the old call form on every case and bar count"); }
    }
    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
