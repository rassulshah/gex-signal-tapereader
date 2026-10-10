// DP254_test_record.cpp -- lsDeltaProfile 2.5.4: the per-session signal record (DeltaProfile\<MKT>-<spb>-signals-<session>.csv).
// Uses the existing Delta adapter mock (tests_delta/irtsdk.h: getLocaltime returns the stamp's wall-clock fields, so a stamp IS
// the Central clock as epoch seconds - exactly the record's time convention).
// Build (Linux, from tests_delta):  g++ -std=c++17 -g -fsanitize=address,undefined -I. test_record_254.cpp -o t && ./t
#include "../DeltaProfile.cpp"
#include <iostream>
#include <functional>
#include <random>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) ++passed; else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static bool exists(const std::string& p) { struct stat st; return stat(p.c_str(), &st) == 0; }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static std::string home(const std::string& name) {
    const char* r = std::getenv("DP254_RUN"); const std::string root = r && r[0] ? r : "/tmp/dp254-run";   // scratch folder for the record files
    const std::string h = root + "/" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    return h + "/InvestorRT/rtx/lsFlexLevels/DeltaProfile/";            // NOT created here: the plugin must create it
}
static int lines(const std::string& text, const char* event) {
    int n = 0; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() == 22 && c[12] == event) ++n; }
    return n;
}
static int linesWithCode(const std::string& text, const char* event, const char* code) {
    int n = 0; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() == 22 && c[12] == event && c[2] == code) ++n; }
    return n;
}
// A whole session's bars with absorption at several places: a random walk with heavy one-sided prints at swing points.
static std::vector<mock::Bar> sessionBars(unsigned long start, int count, int seconds, unsigned seed) {
    std::mt19937 rng(seed); std::uniform_int_distribution<int> step(-2, 2), vol(1, 6), pick(0, 9);
    std::vector<mock::Bar> out; double px = 105;
    for (int i = 0; i < count; ++i) {
        mock::Bar b; b.time = start + static_cast<unsigned long>(i) * static_cast<unsigned long>(seconds);
        const double open = px; px = std::max(88.0, std::min(122.0, px + step(rng)));
        b.open = static_cast<float>(open); b.close = static_cast<float>(px);
        b.low = static_cast<float>(std::min(open, px) - 1); b.high = static_cast<float>(std::max(open, px) + 1);
        b.volume = 0;
        for (int k = static_cast<int>(b.low); k <= static_cast<int>(b.high); ++k) {
            const long buy = vol(rng), sell = vol(rng);
            b.rows.push_back(mock::Price(static_cast<float>(k), buy, sell, buy + sell)); b.volume += static_cast<unsigned long>(buy + sell);
        }
        if (i % 9 == 4) {                                               // a heavy one-sided print at the bar's edge
            const bool sellAtLow = pick(rng) < 5;
            const float at = sellAtLow ? b.low : b.high;
            const long q = 400 + 60 * pick(rng);
            for (mock::Price& r : b.rows) if (r.price == at) { if (sellAtLow) { r.sell += q; } else { r.buy += q; } r.total += q; }
            b.volume += static_cast<unsigned long>(q);
        }
        out.push_back(b);
    }
    return out;
}
struct Frame { std::vector<std::string> draws; };
static Frame frame(const mock::Host& h) {
    Frame f; for (const mock::Draw& d : h.draws) {
        std::ostringstream o; o << d.type << ' ' << d.text << ' ' << d.x << ' ' << d.y << ' ' << d.color; f.draws.push_back(o.str()); }
    return f;
}
// one straight pass, bar by bar; with restart = true, an "IRT restart" (new DLL object, empty book, new host context) before every bar
static std::vector<Frame> replay(const std::vector<mock::Bar>& all, int seconds, const std::string& root, int firstBars, bool restart) {
    std::vector<Frame> frames; mock::Host h; h.seconds = seconds; h.root = root; h.symbol = root + "Z26";
    h.bars.assign(all.begin(), all.begin() + firstBars);
    std::unique_ptr<DeltaProfile> p(new DeltaProfile());
    for (std::size_t n = static_cast<std::size_t>(firstBars); n <= all.size(); ++n) {
        if (restart) { mock::current = &h; p->destroy(); latchBooks().clear(); p.reset(new DeltaProfile()); }
        h.bars.assign(all.begin(), all.begin() + static_cast<long>(n));
        h.draws.clear(); mock::current = &h; p->draw(); frames.push_back(frame(h));
    }
    mock::current = &h; p->destroy(); latchBooks().clear();
    return frames;
}
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }

int main()
{
    setenv("TZ", "UTC", 1); tzset();
    const unsigned long day = 1791504000UL;                             // 2026-10-09 00:00 (wall-clock fields)
    const long long session = 20735;                                    // TapeFlow's id for the 2026-10-09 session

    // 1) restart after EVERY bar = one straight pass: the same file, byte for byte, and the same drawn frames (checklist #11 / #28)
    {
        const std::vector<mock::Bar> bars = sessionBars(day + 8 * 3600, 260, 60, 7);
        const std::string dirA = home("straight"); const std::vector<Frame> a = replay(bars, 60, "ES", 100, false);
        {   // the same bars as a TapeFlow bars3 export (ticks; tick 1) for the Python reader's end-to-end check
            std::ofstream o((dirA + "ES-bars3-" + std::to_string(session) + ".csv").c_str());
            o << "# TapeFlow test tape ES ESZ26 session " << session << " tick 1\nte|o|h|l|c|volume|buy|sell|trades\n";
            for (const mock::Bar& b : bars) o << b.time << '|' << b.open << '|' << b.high << '|' << b.low << '|' << b.close << '|' << b.volume << "|0|0|0\n";
        }
        const std::string fileA = slurp(dirA + "ES-60-signals-" + std::to_string(session) + ".csv");
        const std::string dirB = home("restart"); const std::vector<Frame> b = replay(bars, 60, "ES", 100, true);
        const std::string fileB = slurp(dirB + "ES-60-signals-" + std::to_string(session) + ".csv");
        const int F = lines(fileA, "F"), D = lines(fileA, "D"), DA = linesWithCode(fileA, "D", "A"), DI = linesWithCode(fileA, "D", "I");
        std::cout << "replay: " << a.size() << " bars, " << F << " first-seen, " << D << " decided (" << DA << " A, " << DI << " I), file " << fileA.size() << " bytes\n";
        check(F >= 8 && DA >= 3 && DI >= 1, "fixture: enough candidates, A's and I's to mean something");
        check(!fileA.empty() && fileA == fileB, "restart after every bar writes the same file, byte for byte");
        int differ = 0, rings = 0; for (std::size_t i = 0; i < a.size() && i < b.size(); ++i) { if (a[i].draws != b[i].draws) ++differ; for (const std::string& d : a[i].draws) if (d.compare(0, 5, "ring ") == 0) ++rings; }
        std::cout << "replay: " << differ << " of " << a.size() << " frames differ; " << rings << " circles drawn across the frames\n";
        check(a.size() == b.size() && differ == 0 && rings > 0, "restart after every bar draws the same signals in every frame");
        // every candidate has exactly one F line and at most one D line; a D line never repeats or changes
        std::map<long long, int> f, d; std::istringstream in(fileA); std::string l;
        while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() != 22 || c[0] == "t_first") continue; long long id = std::atoll(c[14].c_str()); (c[12] == "F" ? f : d)[id]++; }
        bool once = true; for (auto& x : f) if (x.second != 1) once = false; for (auto& x : d) if (x.second != 1 || !f.count(x.first)) once = false;
        check(once, "one F per candidate, at most one D, never a D without its F");
    }

    // 1b) the same across the 17:00 CT session change (Central clock): both sessions' files and every frame identical
    {
        setenv("TZ", "America/Chicago", 1); tzset();
        const std::vector<mock::Bar> bars = sessionBars(day + 14 * 3600 + 30 * 60, 280, 60, 9);   // 14:30 .. 19:09 CT
        const std::string dirA = home("straight17"); const std::vector<Frame> a = replay(bars, 60, "ES", 100, false);
        const std::string dirB = home("restart17"); const std::vector<Frame> b = replay(bars, 60, "ES", 100, true);
        bool same = true; int n = 0;
        for (long long sid = session; sid <= session + 1; ++sid) {
            const std::string fa = slurp(dirA + "ES-60-signals-" + std::to_string(sid) + ".csv"), fb = slurp(dirB + "ES-60-signals-" + std::to_string(sid) + ".csv");
            if (fa != fb || fa.empty()) same = false;
            n += lines(fa, "F");
        }
        int differ = 0; for (std::size_t i = 0; i < a.size() && i < b.size(); ++i) if (a[i].draws != b[i].draws) ++differ;
        std::cout << "replay across 17:00: " << n << " candidates in two files, " << differ << " of " << a.size() << " frames differ\n";
        check(same && differ == 0 && a.size() == b.size(), "restart after every bar across 17:00 CT = one straight pass (both files, every frame)");
        setenv("TZ", "UTC", 1); tzset();
    }

    // 2) the columns: CT clock times, prices, tick, net delta (above 2^31: long long), multiple, decided-by bar, version
    {
        const std::string dir = home("columns");
        mock::Host h; h.seconds = 60; h.root = "ES"; h.tick = 0.25f;
        const unsigned long t0 = day + 9 * 3600 + 30 * 60;
        for (int i = 0; i < 110; ++i) { mock::Bar b; b.time = t0 + static_cast<unsigned long>(i * 60);
            b.rows = { mock::Price(120,1,0,1), mock::Price(121,1,0,1), mock::Price(122,1,0,1), mock::Price(123,1,0,1), mock::Price(124,1,0,1) };
            b.volume = 5; b.low = 100; b.high = 125; b.open = 104; b.close = 104; h.bars.push_back(b); }
        h.bars[40].rows.push_back(mock::Price(100, 0, 3000000000L, 3000000000L)); h.bars[40].volume += 3000000000UL;
        h.bars[40].low = 99; h.bars[40].high = 125; h.bars[40].close = 100; h.bars[40].open = 100;
        mock::current = &h; DeltaProfile p; p.draw();                       // undecided: dojis after the node
        const std::string path = dir + "ES-60-signals-" + std::to_string(session) + ".csv";
        check(exists(dir), "the DeltaProfile folder is created when missing");
        std::string text = slurp(path);
        check(lines(text, "F") == 1 && lines(text, "D") == 0, "a new candidate writes one F line and no D line");
        mock::Bar g = h.bars.back(); g.time += 60; g.open = 103; g.close = 106; g.rows = { mock::Price(120,1,0,1) }; g.volume = 1; h.bars.push_back(g);
        mock::Bar f2 = g; f2.time += 60; h.bars.push_back(f2);              // the green bar is now closed
        mock::current = &h; p.draw();
        text = slurp(path);
        std::istringstream in(text); std::string l; std::vector<std::string> F, D;
        while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() == 22 && c[12] == "F") F = c; if (c.size() == 22 && c[12] == "D") D = c; }
        check(!F.empty() && !D.empty(), "F and D lines present");
        if (!F.empty() && !D.empty()) {
            check(F[0] == std::to_string(t0 + 109 * 60), "t_first = the chart's last bar end on the Central clock (" + F[0] + ")");
            check(F[1] == "0" && F[2] == "A?" && F[3] == "support" && F[4] == "bullish", "F: undecided candidate, support / bullish");
            check(F[5] == "100" && F[6] == "100.25" && F[7] == "0.25" && F[18] == "400" && F[19] == "401", "F: the node's frozen row 100-100.25 (ticks 400-401) at tick 0.25 (" + F[5] + "/" + F[6] + "/" + F[7] + ")");
            check(F[8] == "-3000000000", "F: net delta above 2^31 exact (" + F[8] + ")");
            check(std::atof(F[9].c_str()) >= 2.0, "F: multiple vs the average row >= 2");
            check(F[11] == "2.5.4" && F[13] == "ES" && F[14] == "0" && F[15] == std::to_string(t0 + 40 * 60) && F[20] == "sell" && F[21] == "node", "F: version, root, id, absorption bar, node side");
            check(D[2] == "A" && D[3] == "support" && D[4] == "bullish", "D: green close above a selling node = A, bullish");
            check(D[10] == std::to_string(t0 + 110 * 60), "D: decided by the green bar's end (" + D[10] + ")");
            check(D[1] == std::to_string(t0 + 111 * 60) && D[0] == F[0], "D: t_decided = the chart's last bar end when decided; t_first kept");
        }
        // write only on change: more draws with nothing new append nothing and never re-read the file
        const std::string before = slurp(path); const long long loaded = latchBooks().begin()->second.loaded;
        for (int i = 0; i < 5; ++i) { state(h).builtAt -= 1001; mock::current = &h; p.draw(); }
        check(slurp(path) == before && latchBooks().begin()->second.loaded == loaded, "no change = no append and no re-read (checklist #31)");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }

    // 3) a torn (half-written) last line: ignored on read, ended before the next append; junk lines rejected, never fatal
    {
        const std::string dir = home("partial"); mkdir(dir.c_str(), 0700);
        const std::string path = dir + "ES-60-signals-" + std::to_string(session) + ".csv";
        Latch L; L.low = 400; L.high = 400; L.code = "A?"; L.support = true; L.peakTime = static_cast<RTDATE>(day + 9 * 3600); L.price = 100;
        L.sell = true; L.id = 0; L.tFirst = day + 9 * 3600 + 60; L.peakT = day + 9 * 3600; L.net = -500; L.mult = 3;
        const std::string good = recordLine(L, 'F', 0.25, "ES");
        { std::ofstream o(path.c_str(), std::ios::binary); o << recordHeader() << '\n' << good << '\n'
            << "garbage|line\n" << good.substr(0, good.size() / 2) << "nan|\n"   // a junk line, then a full-width line with a non-finite field
            << good.substr(0, 40); }                                           // torn last line (no newline)
        LatchBook bk; seedFromText(slurp(path), "ES", 0.25, bk);
        check(bk.v.size() == 1 && bk.v[0].low == 400 && bk.v[0].code == "A?" && bk.nextId == 1, "the complete line seeds the book, the torn line is ignored");
        check(bk.rejected == 2, "malformed lines counted and skipped (" + std::to_string(bk.rejected) + ")");
        // the plugin appends after the torn line: it must end that line first
        std::vector<std::string> add(1, recordLine(L, 'D', 0.25, "ES"));
        L.code = "A"; L.tDecided = L.tFirst + 60; L.decidedBy = L.tFirst + 60; add[0] = recordLine(L, 'D', 0.25, "ES");
        check(appendRecord(path, add), "append after a torn line");
        const std::string text = slurp(path);
        check(text.find(good.substr(0, 40) + "\n" + add[0] + "\n") != std::string::npos, "the torn line was ended with a newline, the new record is whole");
        LatchBook bk2; seedFromText(text, "ES", 0.25, bk2);
        check(bk2.v.size() == 1 && bk2.v[0].code == "A" && bk2.v[0].logged && bk2.rejected == 3, "re-read: decided A restored; the ended torn line now rejected as malformed");
        // a duplicated (retried) append and a second, different D for the same id: the first decision wins
        Latch I = L; I.code = "I"; I.support = false;
        std::vector<std::string> again; again.push_back(add[0]); again.push_back(recordLine(I, 'D', 0.25, "ES")); appendRecord(path, again);
        LatchBook bk3; seedFromText(slurp(path), "ES", 0.25, bk3);
        check(bk3.v.size() == 1 && bk3.v[0].code == "A" && bk3.v[0].support, "duplicates ignored; a decided signal never changes");
        // another root in the same file (MES on an ES file) is not mixed in
        LatchBook bk4; seedFromText(slurp(path), "MES", 0.25, bk4);
        check(bk4.v.empty(), "lines of another root are not loaded");
        // strict parsing of single fields
        long long v = 0; double x = 0;
        check(!parseWhole(" 12", 0, 99, v) && !parseWhole("12x", 0, 99, v) && !parseWhole("100", 0, 99, v) && parseWhole("-5", -9, 9, v) && v == -5, "integer tokens: strict");
        check(!parseReal("inf", x) && !parseReal("nan", x) && !parseReal("1.5 ", x) && parseReal("1.5", x) && x == 1.5, "real tokens: strict and finite");
        // a file written on another tick size re-keys from its prices
        LatchBook bk5; seedFromText(slurp(path), "ES", 0.5, bk5);
        check(bk5.v.size() == 1 && bk5.v[0].low == 200 && bk5.v[0].high == 200, "tick 0.25 record on a 0.5 chart: zone re-keyed from price");
        // an oversized record is not read and not appended to
        { std::string big(REC_MAX_BYTES + 10, 'x'); std::string text2; bool tooLarge = false; const std::string bp = dir + "big.csv";
          { std::ofstream o(bp.c_str(), std::ios::binary); o << big; } check(!readWholeFile(bp, text2, tooLarge) && tooLarge, "oversized record refused (checklist #33)"); }
    }

    // 4) a failing write: lines are kept and land once, in order, when the file can be written again (checklist #6)
    {
        const std::string dir = home("failwrite"); mkdir(dir.c_str(), 0700);
        const std::string path = dir + "ES-60-signals-" + std::to_string(session) + ".csv";
        if (symlink("/nonexistent-dp254/x.csv", path.c_str()) != 0) check(false, "fixture: symlink");   // a dangling link: every append fails
        const std::vector<mock::Bar> bars = sessionBars(day + 8 * 3600, 200, 60, 11);
        mock::Host h; h.seconds = 60; h.root = "ES"; h.bars.assign(bars.begin(), bars.begin() + 100);
        DeltaProfile p;
        for (std::size_t n = 100; n <= 150; ++n) { h.bars.assign(bars.begin(), bars.begin() + static_cast<long>(n)); mock::current = &h; p.draw(); }
        const std::size_t pending = latchBooks().begin()->second.journal.size();
        check(pending > 0 && latchBooks().begin()->second.note.find("failed") != std::string::npos, "write failures keep the lines and say so");
        unlink(path.c_str());
        for (std::size_t n = 151; n <= bars.size(); ++n) { h.bars.assign(bars.begin(), bars.begin() + static_cast<long>(n)); mock::current = &h; p.draw(); }
        const std::string got = slurp(path);
        mock::current = &h; p.destroy(); latchBooks().clear();
        const std::string ref = home("failwrite-ref"); const std::vector<Frame> unused = replay(bars, 60, "ES", 100, false); (void)unused;
        check(!got.empty() && got == slurp(ref + "ES-60-signals-" + std::to_string(session) + ".csv"), "after the failure clears, the file equals an undisturbed run (no loss, no duplicate)");
    }

    // 5) two charts on one DLL - ES 3-min and ES 60-min - never mix (checklist #1 / #9), and both survive a restart
    {
        const std::string dir = home("twocharts");
        const std::vector<mock::Bar> b3 = sessionBars(day + 6 * 3600, 160, 180, 21), b60 = sessionBars(day - 6 * 3600 + 3600, 125, 3600, 33);
        mock::Host a; a.seconds = 180; a.root = "ES"; mock::Host b; b.seconds = 3600; b.root = "ES"; b.chart = "hourly";
        DeltaProfile p;
        for (std::size_t n = 100; n <= 160; ++n) {
            a.bars.assign(b3.begin(), b3.begin() + static_cast<long>(n)); mock::current = &a; p.draw();
            b.bars.assign(b60.begin(), b60.begin() + static_cast<long>(std::min<std::size_t>(n, b60.size()))); mock::current = &b; p.draw();
        }
        const std::string f3 = slurp(dir + "ES-180-signals-" + std::to_string(session) + ".csv");
        const long long s60 = latchBooks()["ES|3600"].session;
        const std::string f60 = slurp(dir + "ES-3600-signals-" + std::to_string(s60) + ".csv");
        check(lines(f3, "F") > 0 && lines(f60, "F") > 0, "both charts record (" + std::to_string(lines(f3, "F")) + " / " + std::to_string(lines(f60, "F")) + " candidates)");
        bool clean3 = true, clean60 = true; std::istringstream i3(f3), i60(f60); std::string l;
        while (std::getline(i3, l)) { std::vector<std::string> c = fields(l); if (c.size() == 22 && c[0] != "t_first" && (std::atoll(c[15].c_str()) % 180) != 0) clean3 = false; }
        while (std::getline(i60, l)) { std::vector<std::string> c = fields(l); if (c.size() == 22 && c[0] != "t_first" && (std::atoll(c[15].c_str()) % 3600) != 0) clean60 = false; }
        check(clean3 && clean60, "each file holds only its own chart's bars");
        const std::vector<Latch> keep3 = latchBooks()["ES|180"].v, keep60 = latchBooks()["ES|3600"].v;
        a.draws.clear(); b.draws.clear(); state(a).builtAt -= 1001; state(b).builtAt -= 1001;
        mock::current = &a; p.draw(); mock::current = &b; p.draw();
        const Frame fa = frame(a), fb = frame(b);
        mock::current = &a; p.destroy(); mock::current = &b; p.destroy(); latchBooks().clear();   // IRT restart
        a.draws.clear(); b.draws.clear();
        DeltaProfile q; mock::current = &a; q.draw(); mock::current = &b; q.draw();
        check(latchBooks()["ES|180"].v.size() == keep3.size() && latchBooks()["ES|3600"].v.size() == keep60.size(), "after a restart each chart's book is seeded from its own file");
        check(frame(a).draws == fa.draws && frame(b).draws == fb.draws, "after a restart both charts draw exactly what they drew before");
        mock::current = &a; q.destroy(); mock::current = &b; q.destroy(); latchBooks().clear();
    }

    // 6) the 17:00 CT session change: the book clears, a new file starts, the old file is never touched; a restart after 17:00
    //    seeds from the new session's file only. Central clock (DST) as on his PC.
    {
        setenv("TZ", "America/Chicago", 1); tzset();
        const std::string dir = home("session");
        const std::vector<mock::Bar> bars = sessionBars(day + 14 * 3600, 200, 60, 5);   // 14:00 .. 17:19 CT
        mock::Host h; h.seconds = 60; h.root = "ES"; DeltaProfile p; std::string oldAt1700;
        const std::string oldPath = dir + "ES-60-signals-" + std::to_string(session) + ".csv", newPath = dir + "ES-60-signals-" + std::to_string(session + 1) + ".csv";
        for (std::size_t n = 100; n <= bars.size(); ++n) {
            h.bars.assign(bars.begin(), bars.begin() + static_cast<long>(n)); mock::current = &h; p.draw();
            if (h.bars.back().time == day + 17 * 3600 - 60) oldAt1700 = slurp(oldPath);   // the 16:59 bar is the last of the old session
        }
        check(!oldAt1700.empty() && slurp(oldPath) == oldAt1700, "the old session's file is not touched after 17:00 CT");
        check(latchBooks().begin()->second.session == session + 1, "the book is on the new session after 17:00 CT");
        std::istringstream in(slurp(newPath)); std::string l; bool onlyNew = true;
        while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() == 22 && c[0] != "t_first" && std::atoll(c[0].c_str()) < static_cast<long long>(day + 17 * 3600)) onlyNew = false; }
        check(onlyNew, "the new session's file holds only lines first seen after 17:00 CT");
        const std::size_t kept = latchBooks().begin()->second.v.size();
        mock::current = &h; p.destroy(); latchBooks().clear();
        DeltaProfile q; mock::current = &h; q.draw();
        check(latchBooks().begin()->second.v.size() == kept && latchBooks().begin()->second.session == session + 1, "a restart after 17:00 CT seeds from the new session only");
        mock::current = &h; q.destroy(); latchBooks().clear();
        // just before 17:00 the session is still the old one (a bar ENDING at 17:00 belongs to the new session - the 2.5.x edge)
        mock::Host e; e.seconds = 60; e.root = "ES"; e.bars.assign(bars.begin(), bars.begin() + 180);   // last bar ends 16:59
        DeltaProfile r; mock::current = &e; r.draw();
        check(latchBooks().begin()->second.session == session, "16:59 CT is still the old session");
        mock::current = &e; r.destroy(); latchBooks().clear();
        setenv("TZ", "UTC", 1); tzset();
    }

    // 7) market mapping in the file name, like the other plugins (CPE -> HG, EU6 -> EU, MES -> ES)
    {
        check(marketName("CPE") == "HG" && marketName("EU6") == "EU" && marketName("MES") == "ES", "market aliases");
        check(recordName(marketName("CPE"), 180, 20735) == "HG-180-signals-20735.csv", "record name: <MKT>-<spb>-signals-<session>.csv");
        const std::string dir = home("mapping");
        const std::vector<mock::Bar> bars = sessionBars(day + 8 * 3600, 160, 60, 7);
        mock::Host h; h.seconds = 60; h.root = "CPE"; h.tick = 1; h.bars = bars; DeltaProfile p; mock::current = &h; p.draw();
        check(exists(dir + "HG-60-signals-" + std::to_string(session) + ".csv"), "a CPE chart writes HG-60-signals-<session>.csv");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    // 8) no storage (no USERPROFILE): nothing written, nothing kept in memory, drawing unchanged
    {
        unsetenv("USERPROFILE");
        const std::vector<mock::Bar> bars = sessionBars(day + 8 * 3600, 160, 60, 7);
        mock::Host h; h.seconds = 60; h.root = "ES"; h.bars = bars; DeltaProfile p; mock::current = &h; p.draw();
        check(latchBooks().begin()->second.journal.empty() && latchBooks().begin()->second.path.empty(), "no storage: no record, no growing journal");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
