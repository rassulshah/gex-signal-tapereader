// test_contract263.cpp -- lsDeltaProfile 2.6.3: the signal record is named by the sanitized FULL contract
// (<MKT>-<contract>-<spb>-signals-<session>.csv, audit F3 persistence part). A new expiry never loads the prior contract's records,
// the same contract reloads its own records (circles persist across a restart), root-only legacy files are never read or touched,
// and the F6 old-session retry queue still lands its lines in the (new-name) old-session file.
// Build (Linux, from tests_delta):  g++ -std=c++14 -g -fsanitize=address,undefined -I. test_contract263.cpp -o t && ./t
#include "../DeltaProfile.cpp"
#include <iostream>
#include <functional>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) ++passed; else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static bool exists(const std::string& p) { struct stat st; return stat(p.c_str(), &st) == 0; }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static void spit(const std::string& p, const std::string& t) { std::ofstream o(p.c_str(), std::ios::binary | std::ios::trunc); o << t; }
static std::string home(const std::string& name) {
    const char* r = std::getenv("DP254_RUN"); const std::string root = r && r[0] ? r : "/tmp/dp263-run";
    const std::string h = root + "/" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    return h + "/InvestorRT/rtx/lsFlexLevels/DeltaProfile/";
}
static std::vector<std::string> listing(const std::string& dir) {
    std::vector<std::string> out; DIR* d = opendir(dir.c_str()); if (!d) return out;
    while (dirent* e = readdir(d)) { const std::string n = e->d_name; if (n != "." && n != "..") out.push_back(n); }
    closedir(d); std::sort(out.begin(), out.end()); return out;
}
static int linesWithCode(const std::string& text, const char* event, const char* code) {
    int n = 0; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() == static_cast<std::size_t>(REC_FIELDS) && c[12] == event && c[2] == code) ++n; }
    return n;
}
static int recordLines(const std::string& text) {
    int n = 0; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() == static_cast<std::size_t>(REC_FIELDS) && c[0] != "t_first") ++n; }
    return n;
}
static int rings(const mock::Host& h) { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "ring") ++n; return n; }
// one-minute dojis with light buying above 120; bar `peak` holds a heavy SELLING print at 100 with the given open/close
// (o=99, c=101 = the peak bar closes green above the selling node = A under the 2.6.2 rule)
static mock::Host chart(const std::string& symbol, unsigned long t0, int count, int peak, double o, double c) {
    mock::Host h; h.seconds = 60; h.root = "ES"; h.symbol = symbol; h.tick = 0.25f;
    for (int i = 0; i < count; ++i) { mock::Bar b; b.time = t0 + static_cast<unsigned long>(i * 60);
        b.rows = { mock::Price(120,1,0,1), mock::Price(121,1,0,1), mock::Price(122,1,0,1), mock::Price(123,1,0,1), mock::Price(124,1,0,1) };
        b.volume = 5; b.low = 100; b.high = 125; b.open = 104; b.close = 104; h.bars.push_back(b); }
    if (peak >= 0 && peak < count) {
        h.bars[peak].rows.push_back(mock::Price(100, 0, 10000, 10000)); h.bars[peak].volume += 10000;
        h.bars[peak].low = 99; h.bars[peak].high = 125; h.bars[peak].open = static_cast<float>(o); h.bars[peak].close = static_cast<float>(c);
    }
    return h;
}
static LatchBook& book(const std::string& contract, int spb = 60) { return latchBooks()[contract + "|" + std::to_string(spb)]; }
int main()
{
    setenv("TZ", "UTC", 1); tzset();
    const unsigned long day = 1791504000UL;                             // 2026-10-09 00:00 (wall-clock fields)
    const long long session = 20735;
    const unsigned long t0 = day + 9 * 3600 + 30 * 60;
    const std::string S = std::to_string(session);

    check(std::string(DLT_VERSION) == "2.8.0", "version string is 2.8.0");

    // (c) path sanitization: the token keeps ASCII letters, digits, '_', '-', '.'; everything else becomes '_'
    {
        check(contractToken("ESZ26") == "ESZ26", "plain contract kept");
        check(contractToken("ES Z26") == "ES_Z26", "space -> _");
        check(contractToken("ES/Z\\26") == "ES_Z_26", "path separators -> _ (no sub-folder, no escape)");
        check(contractToken("ES:Z*2?6\"<>|") == "ES_Z_2_6____", "Windows-reserved characters -> _");
        check(contractToken("../..") == ".._..", "dot-dot cannot climb: the separator is replaced and the token is mid-name");
        check(contractToken("6E.Z26-x_1") == "6E.Z26-x_1", "'.', '-', '_' kept");
        check(contractToken("ES\xC3\xA9Z") == "ES__Z", "non-ASCII bytes -> _ (locale independent)");
        check(contractToken("ES\tZ\n\r") == "ES_Z__", "control characters -> _");
        check(contractToken("") == "unknown-contract", "empty symbol -> unknown-contract");
        check(contractToken(std::string(200, 'A')).size() == 64, "token capped at 64 characters");
        check(contractRecordName("ES", "ESZ26", 60, session) == "ES-ESZ26-60-signals-" + S + ".csv", "name format <MKT>-<contract>-<spb>-signals-<session>.csv");
        check(contractRecordName("HG", "CPE Z6/x", 180, 20736) == "HG-CPE_Z6_x-180-signals-20736.csv", "odd symbol name");
        // end to end: a chart on an odd symbol writes exactly one flat file in the record folder
        const std::string dir = home("odd");
        mock::Host h = chart("ES Z6/<1>", t0, 110, 40, 99, 101); DeltaProfile p; mock::current = &h; p.draw();
        const std::vector<std::string> files = listing(dir);
        const std::string expect = "ES-ES_Z6__1_-60-signals-" + S + ".csv";
        check(files.size() == 1 && files[0] == expect, "odd symbol: one file named " + expect + (files.empty() ? std::string(" (none)") : " got " + files[0]));
        check(linesWithCode(slurp(dir + expect), "D", "A") == 1, "odd symbol: the A is recorded in that file");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }

    // (a) ESZ26 then ESH27 on the SAME chart (same host, same DLL object): different journal paths, the second has no records from the first
    {
        const std::string dir = home("rollover");
        const std::string z = dir + "ES-ESZ26-60-signals-" + S + ".csv", hh = dir + "ES-ESH27-60-signals-" + S + ".csv";
        mock::Host h = chart("ESZ26", t0, 110, 40, 99, 101);
        DeltaProfile p; mock::current = &h; p.draw();
        check(rings(h) == 1 && linesWithCode(slurp(z), "D", "A") == 1, "ESZ26: A drawn and recorded in ES-ESZ26-60-signals-<session>.csv");
        check(book("ESZ26").path == z, "ESZ26 book journals to its own contract file");
        const std::string zText = slurp(z);
        // the chart is switched to the next expiry; its bars carry no absorption print
        mock::Host next = chart("ESH27", t0, 110, -1, 0, 0); next.data = h.data;   // same chart slot
        mock::current = &next; p.draw();
        check(book("ESH27").path == hh && book("ESH27").path != book("ESZ26").path, "ESH27 journal path differs from ESZ26");
        check(book("ESH27").loaded == 0 && book("ESH27").v.empty(), "ESH27 book has no records from ESZ26");
        check(rings(next) == 0, "ESH27 draws no ESZ26 circle");
        check(slurp(z) == zText, "ESZ26 file untouched by the ESH27 chart");
        check(!exists(hh) || recordLines(slurp(hh)) == 0, "ESH27 file holds no ESZ26 lines");
        mock::current = &next; p.destroy(); latchBooks().clear();
        // and after an IRT restart: an ESH27 chart still loads nothing from the ESZ26 file of the same session
        DeltaProfile q; mock::Host fresh = chart("ESH27", t0, 110, -1, 0, 0); mock::current = &fresh; q.draw();
        check(book("ESH27").loaded == 0 && book("ESH27").v.empty() && rings(fresh) == 0, "restart on ESH27: nothing loaded from ESZ26");
        mock::current = &fresh; q.destroy(); latchBooks().clear();
    }

    // (b) the same contract reloads its records: circles persist across a restart within the session, nothing re-appended
    {
        const std::string dir = home("reload");
        const std::string z = dir + "ES-ESZ26-60-signals-" + S + ".csv";
        mock::Host h = chart("ESZ26", t0, 110, 40, 99, 101);
        { DeltaProfile p; mock::current = &h; p.draw(); check(rings(h) == 1, "fixture: A drawn"); mock::current = &h; p.destroy(); latchBooks().clear(); }
        const std::string before = slurp(z);
        check(linesWithCode(before, "D", "A") == 1, "fixture: A recorded");
        // restart; the same node's peak bar now reads RED on the node (would fail the rule) - the record decides, never repaints
        mock::Host h2 = chart("ESZ26", t0, 110, 40, 100.5, 100);
        DeltaProfile p; mock::current = &h2; p.draw();
        check(book("ESZ26").loaded == 1 && book("ESZ26").v.size() == 1 && book("ESZ26").v[0].code == "A", "same contract: the A record loads back");
        check(rings(h2) == 1, "same contract: the circle persists across the restart");
        check(slurp(z) == before, "same contract: nothing re-appended on reload");
        mock::current = &h2; p.destroy(); latchBooks().clear();
    }

    // legacy root-only files (2.5.x <MKT>-<spb>-signals-* and 2.6.0-2.6.2 <MKT>-<spb>-sync260-signals-*) are not read, not touched
    {
        const std::string src = home("legacy-src");
        { mock::Host h = chart("ESZ26", t0, 110, 40, 99, 101); DeltaProfile p; mock::current = &h; p.draw(); mock::current = &h; p.destroy(); latchBooks().clear(); }
        const std::string valid = slurp(src + "ES-ESZ26-60-signals-" + S + ".csv");
        check(linesWithCode(valid, "D", "A") == 1, "fixture: a valid A record");
        std::string rootOnly = valid;                                    // a 2.5.x-style copy naming only the root
        for (std::size_t at = rootOnly.find("|ESZ26|"); at != std::string::npos; at = rootOnly.find("|ESZ26|", at)) rootOnly.replace(at, 7, "|ES|");
        const std::string dir = home("legacy"); mkdir(dir.c_str(), 0700);
        const std::string l1 = dir + "ES-60-sync260-signals-" + S + ".csv", l2 = dir + "ES-60-signals-" + S + ".csv";
        spit(l1, valid); spit(l2, rootOnly);
        mock::Host h = chart("ESZ26", t0, 110, -1, 0, 0); DeltaProfile p; mock::current = &h; p.draw();
        check(book("ESZ26").loaded == 0 && book("ESZ26").v.empty() && rings(h) == 0, "root-only legacy files are not loaded (no migration)");
        check(slurp(l1) == valid && slurp(l2) == rootOnly, "legacy files left byte-identical");
        const std::string fresh = dir + "ES-ESZ26-60-signals-" + S + ".csv";
        check(!exists(fresh) || recordLines(slurp(fresh)) == 0, "the new contract file starts empty (no imported lines)");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }

    // F6: an old-session write that fails at 17:00 is retried into the OLD session's contract-named file, never the new one
    {
        const std::string dir = home("f6"); mkdir(dir.c_str(), 0700);
        const std::string oldPath = dir + "ES-ESZ26-60-signals-" + S + ".csv", newPath = dir + "ES-ESZ26-60-signals-" + std::to_string(session + 1) + ".csv";
        if (symlink("/nonexistent-dp263/x.csv", oldPath.c_str()) != 0) check(false, "fixture: symlink");   // every append to the old file fails
        mock::Host all = chart("ESZ26", day + 14 * 3600, 200, 130, 99, 101);   // 14:00 .. 17:19; the A forms at 16:10
        mock::Host h = all; DeltaProfile p; std::size_t carried = 0; bool sawNote = false;
        for (std::size_t n = 125; n <= all.bars.size(); ++n) {
            h.bars.assign(all.bars.begin(), all.bars.begin() + static_cast<long>(n)); mock::current = &h; p.draw();
            if (book("ESZ26").session == session + 1 && !book("ESZ26").carry.empty()) {
                carried = book("ESZ26").carry[0].second.size(); sawNote = book("ESZ26").note.find("retained") != std::string::npos;
                check(book("ESZ26").carry[0].first == oldPath, "F6: the owed lines are queued for the old session's contract file");
                break;
            }
        }
        check(carried > 0 && sawNote, "F6: old-session lines kept and reported at 17:00");
        check(book("ESZ26").path == newPath, "F6: the book journals to the new session's contract file");
        unlink(oldPath.c_str());                                            // the old file can be written again
        h.bars = all.bars; mock::current = &h; p.draw();
        check(book("ESZ26").carry.empty(), "F6: the queue drains once the old file is writable");
        const std::string oldText = slurp(oldPath);
        check(linesWithCode(oldText, "F", "A?") == 1 && linesWithCode(oldText, "D", "A") == 1, "F6: the old session's F and D lines land in its contract file");
        {   // the new session's file holds only lines first seen at/after 17:00 (the rolling window may re-detect the node as a NEW event)
            std::istringstream in(slurp(newPath)); std::string l; bool onlyNew = true;
            while (std::getline(in, l)) { std::vector<std::string> c = fields(l);
                if (c.size() == static_cast<std::size_t>(REC_FIELDS) && c[0] != "t_first" && std::atoll(c[0].c_str()) < static_cast<long long>(day + 17 * 3600)) onlyNew = false; }
            check(onlyNew, "F6: nothing of the old session leaks into the new session's file");
        }
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
