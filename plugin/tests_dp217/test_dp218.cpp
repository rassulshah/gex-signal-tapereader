// test_dp218.cpp -- lsDeltaProfile 2.18.0 / DeltaProfileCore 2.18.0 (Rassul 2026-10-18, mockup approved 11:49 CT):
// "the close can be from the same candle or the close of the next candle as long as the low of both bars is below the high of the pivot
//  bar which may or may not be either bar for bullish absorption and vice versa for the bearish absorption"
//   [core] aStep (same wins / pending one candle / next / ruled out), nextPivotOk, swingPivot's pivot bars
//   [host] same-candle A unchanged (conf=same); next-candle A (conf=next, bubble on the node's candle, never before the next close);
//          rejected: next low not below the pivot high / neither closes beyond / a 3rd candle; bearish mirrors; no repaint after the
//          pivot is confirmed; restart from the record; status A_CONF
//   [rec]  D / TA lines round-trip old (24 / 13 fields = same) and new (25 / 14) formats; 24-column header upgraded once
//   [lab]  (12:49 CT) ratio labels only on the 3 biggest profile bars by abs delta: 4th at 2.0x+ unlabelled, a top-3 bar under 2.0x labelled, A prefix kept
//   [ver]  DLT_VERSION = setVersion literal = core header
// Build (from tests_dp217/): see run_tests.sh
#include "../DeltaProfile.cpp"
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) ++passed; else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static std::string flex;
static std::string home(const std::string& name) {
    const std::string root = "/tmp/dp218-run", h = root + "/" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    flex = h + "/InvestorRT/rtx/lsFlexLevels/";
    return flex + "DeltaProfile/";
}
static int countPrefix(const std::string& text, const std::string& pre) {
    int n = 0; std::istringstream in(text); std::string l; while (std::getline(in, l)) if (l.compare(0, pre.size(), pre) == 0) ++n; return n;
}
static std::vector<std::vector<std::string> > dRows(const std::string& text) {
    std::vector<std::vector<std::string> > v; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() >= 22 && c[12] == "D" && c[2] == "A") v.push_back(c); }
    return v;
}
static std::string taConf(const std::string& text, const char* pre) {   // the conf field of the first TA line with this prefix
    std::istringstream in(text); std::string l;
    while (std::getline(in, l)) if (l.compare(0, std::strlen(pre), pre) == 0) { std::vector<std::string> c = fields(l); return c.size() == 14 ? c[13] : "?"; }
    return "";
}
static int rings(const mock::Host& h) { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "ring") ++n; return n; }
// (2.19.0) the gray circle (full or dimmed away from a zone) vs the coloured A circles
static COLOR dimGray() { const COLOR a = 0x009CA3AF; COLOR o = 0; for (int sh = 0; sh < 24; sh += 8) { const float x = (float)((a >> sh) & 0xFF) * 0.45f; o |= ((COLOR)(x + 0.5f) & 0xFF) << sh; } return o; }
static int grayRings(const mock::Host& h) { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "ring" && (d.color == 0x009CA3AF || d.color == dimGray())) ++n; return n; }
static int colourRings(const mock::Host& h) { return rings(h) - grayRings(h); }
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
static const int PK = 100;
// Prices are given in BULLISH coordinates; bear = true mirrors them (x -> 220 - x): a rising market into heavy BUYING at 120.
static bool bear = false;
static double M(double x) { return bear ? 220 - x : x; }
static void setBar(mock::Bar& b, double lo, double hi, double o, double c) {
    b.low = static_cast<float>(std::min(M(lo), M(hi))); b.high = static_cast<float>(std::max(M(lo), M(hi)));
    b.open = static_cast<float>(M(o)); b.close = static_cast<float>(M(c));
}
// the test_dp217 market: falling into PK, heavy selling at 100 on PK (bullish); PK OHLC given (lo 99, hi 104)
static mock::Host chart(unsigned long t0, double pkOpen, double pkClose) {
    mock::Host h; h.seconds = 60; h.root = "ES"; h.tick = 0.25f;
    for (int i = 0; i <= PK; ++i) { mock::Bar b; b.time = t0 + static_cast<unsigned long>(i * 60);
        for (int r = 120; r <= 124; ++r) b.rows.push_back(mock::Price(static_cast<float>(M(r)), 1, 0, 1));
        b.volume = 5; const double lo = 130 - 0.25 * i; setBar(b, lo, lo + 25, lo + 5, lo + 5); h.bars.push_back(b); }
    mock::Bar& p = h.bars[PK];
    p.rows.push_back(bear ? mock::Price(static_cast<float>(M(100)), 10000, 0, 10000) : mock::Price(100, 0, 10000, 10000)); p.volume += 10000;
    setBar(p, 99, 104, pkOpen, pkClose);
    return h;
}
static void add(mock::Host& h, double lo, double hi, double o, double c) { mock::Bar b = h.bars.back(); b.time += 60;
    b.rows = { mock::Price(static_cast<float>(M(120)), 1, 0, 1) }; b.volume = 1; setBar(b, lo, hi, o, c); h.bars.push_back(b); }
static void add(mock::Host& h, double lo, double hi) { add(h, lo, hi, (lo + hi) / 2, (lo + hi) / 2); }
static void redraw(mock::Host& h, DeltaProfile& p) { h.draws.clear(); if (h.data) { state(h).builtAt -= 1001; state(h).statusAt = -1; } mock::current = &h; p.draw(); }
static std::vector<sb::Ev> boardAll() { std::vector<sb::Ev> v; sb::readAll('A', "ESZ26", 60, "test", -1, v); return v; }
static void finish(mock::Host& h, DeltaProfile& p) { mock::current = &h; p.destroy(); latchBooks().clear(); }
static const long long SESSION = 20735;
static std::string recPath(const std::string& dir) { return dir + "ES-ESZ26-60-signals-" + std::to_string(SESSION) + ".csv"; }

static void coreTests() {
    using namespace dp;
    // same candle wins and behaves exactly as 2.17.0's tentStep
    for (int pv = 0; pv < 2; ++pv) for (int pf = 0; pf < 2; ++pf) {
        const AStep a = aStep(true, pv != 0, false, false, false, pf != 0);
        check(a.by == CONF_SAME && a.step == tentStep(true, pv != 0, pf != 0), "[core] same-candle close = 2.17.0 tentStep, conf same");
    }
    check(aStep(false, true, false, true, true, true).step == TS_PENDING && aStep(false, true, false, true, true, true).by == CONF_NONE, "[core] own close failed, next not closed = pending");
    check(aStep(false, true, true, false, true, true).step == TS_RULED_OUT, "[core] neither candle closes beyond = never A");
    check(aStep(false, true, true, true, true, true).step == TS_FINAL && aStep(false, true, true, true, true, true).by == CONF_NEXT, "[core] next close + both beyond pivot + confirmed = final, conf next");
    check(aStep(false, true, true, true, true, false).step == TS_SHOWN, "[core] next-candle A before the pivot is confirmed = tentative shown");
    check(aStep(false, true, true, true, false, false).step == TS_HIDDEN, "[core] next-candle A, pivot rule not holding yet = hidden");
    check(aStep(false, true, true, true, false, true).step == TS_FAILED, "[core] next-candle A, confirmed pivot against it = failed");
    // nextPivotOk
    std::vector<double> hi = { 110, 104, 103.5, 106 }, lo = { 100, 99, 99.5, 104.25 };
    check(nextPivotOk(hi, lo, 4, 2, 1, true), "[core] bullish: next low 99.5 < pivot high 104");
    check(!nextPivotOk(hi, lo, 4, 3, 1, true), "[core] bullish: next low 104.25 not below pivot high 104");
    check(nextPivotOk(hi, lo, 4, 1, 1, true), "[core] the confirming bar may BE the pivot bar");
    check(nextPivotOk(hi, lo, 4, 3, 1, false) && !nextPivotOk(hi, lo, 4, 1, 3, false), "[core] bearish mirror: next high 106 > pivot low 99; 104 not above 104.25");
    check(!nextPivotOk(hi, lo, 4, 4, 1, true) && !nextPivotOk(hi, lo, 4, 2, -1, true), "[core] out of range / no pivot = false");
    std::vector<double> hn = hi; hn[2] = std::nan("");
    check(!nextPivotOk(hn, lo, 4, 2, 1, true), "[core] non-finite bar = false");
    // swingPivot pivot bars: falling into bar 3, then up
    std::vector<double> H = { 120, 115, 110, 104, 103.5, 106, 108 }, L = { 115, 110, 105, 99, 99.5, 101, 103 };
    std::vector<char> b, r, c; std::vector<int> d, u;
    swingPivot(H, L, 7, b, r, c, &d, &u);
    check(d[3] == 3 && d[2] == 3 && b[3] == 1, "[core] swingPivot: the down-leg bars' down pivot = the low bar 3");
    std::vector<char> b2, r2, c2; swingPivot(H, L, 7, b2, r2, c2);
    check(b2 == b && r2 == r && c2 == c, "[core] swingPivot without the pivot outputs = unchanged (2.17.0 callers)");
}

// one bullish / bearish run of the host scenarios
static void hostTests(bool bearish, unsigned long t0) {
    bear = bearish; const std::string S = bearish ? "[bear] " : "[bull] ";
    long long samePeakBar = -1;
    // 1) same-candle A unchanged: PK closes beyond the node in its colour
    {   const std::string dir = home(std::string("same") + (bearish ? "b" : "u")), path = recPath(dir);
        mock::Host h = chart(t0, 100.5, 101); add(h, 99.5, 103.5);
        DeltaProfile p; redraw(h, p);
        check(rings(h) == 1 && taConf(slurp(path), "TA|S|") == "conf=same", S + "same candle: tentative A at PK's close (2.17.0 behaviour), TA|S conf=same");
        if (!bearish) h.bars.back().high = 104.5f; else h.bars.back().low = static_cast<float>(M(104.5));   // then a bar that confirms the turn
        add(h, 100, 105); redraw(h, p);
        const std::vector<std::vector<std::string> > d = dRows(slurp(path));
        check(d.size() == 1 && d[0].size() == 25 && d[0][24] == "conf=same", S + "same candle: one D line, 25 fields, conf=same at the end");
        check(rings(h) == 1 && boardAll().size() == 1 && boardAll()[0].flag == 1, S + "same candle: final bubble, board flag 1");
        samePeakBar = boardAll().empty() ? -2 : boardAll()[0].bar;
        const std::string st = slurp(flex + "DeltaProfile.status-ES-60.txt");
        check(st.find("A_CONF,same 1,next 0,") != std::string::npos, S + "status A_CONF same 1 next 0");
        finish(h, p); }
    // 2) next-candle A: PK fails its own close (wrong colour), PK+1 closes beyond with its low below the pivot (PK) high
    {   const std::string dir = home(std::string("next") + (bearish ? "b" : "u")), path = recPath(dir);
        mock::Host h = chart(t0, 101.5, 100.75);                      // red (bullish coordinates): own close fails
        add(h, 99.5, 103.5, 100, 103);                                 // PK+1 forming: green, closes above the node
        DeltaProfile p; redraw(h, p);
        check(colourRings(h) == 0 && grayRings(h) == 1 && boardAll().empty() && dRows(slurp(path)).empty() && countPrefix(slurp(path), "TA|") == 0, S + "next: nothing before the next candle closes (2.19.0: GRAY circle, no colour)");
        add(h, 99.75, 103.25); redraw(h, p);                           // PK+1 closed; PK+2 forming (no turn yet)
        check(rings(h) == 1 && boardAll().size() == 1 && boardAll()[0].flag == 2, S + "next: tentative A once the next candle closed");
        check(taConf(slurp(path), "TA|S|") == "conf=next", S + "next: TA|S ends in conf=next");
        check(!boardAll().empty() && boardAll()[0].bar == samePeakBar, S + "next: the bubble stays on the node's candle (same bar as the same-candle A)");
        redraw(h, p); redraw(h, p);
        check(rings(h) == 1 && countPrefix(slurp(path), "TA|S|") == 1, S + "next: stable across redraws, TA|S once");
        if (!bearish) h.bars.back().high = 104.5f; else h.bars.back().low = static_cast<float>(M(104.5));
        add(h, 100, 105); redraw(h, p);                                // the turn confirms PK as the pivot: final
        std::vector<std::vector<std::string> > d = dRows(slurp(path));
        check(d.size() == 1 && d[0].size() == 25 && d[0][24] == "conf=next", S + "next: one D line ending in conf=next");
        check(d.size() == 1 && d[0][16] == std::to_string(static_cast<unsigned long long>(h.bars[PK].time)), S + "next: the D line's peak is the node's candle");
        check(d.size() == 1 && std::stoll(d[0][10]) == std::stoll(d[0][15]) + 60, S + "next: decided_by_t = the next candle's bar end");
        check(rings(h) == 1 && boardAll().size() == 1 && boardAll()[0].flag == 1 && countPrefix(slurp(path), "TA|X|") == 0, S + "next: final bubble, flag 1, no failure");
        check(slurp(flex + "DeltaProfile.status-ES-60.txt").find("A_CONF,same 0,next 1,") != std::string::npos, S + "next: status A_CONF same 0 next 1");
        // no repaint after the pivot is confirmed: a crash far beyond the node, more bars
        add(h, 60, 70); add(h, 50, 60); add(h, 55, 65); redraw(h, p);
        check(rings(h) == 1 && dRows(slurp(path)).size() == 1 && countPrefix(slurp(path), "TA|X|") == 0 && boardAll().size() == 1 && boardAll()[0].flag == 1,
              S + "next: never repainted after the pivot is confirmed");
        // restart from the record
        finish(h, p);
        DeltaProfile q; mock::Host h2 = h; h2.data = NULL; redraw(h2, q); redraw(h2, q);
        check(rings(h2) == 1 && dRows(slurp(path)).size() == 1 && countPrefix(slurp(path), "TA|S|") == 1, S + "next: restart shows it, nothing written twice");
        check(slurp(flex + "DeltaProfile.status-ES-60.txt").find("A_CONF,same 0,next 1,") != std::string::npos, S + "next: restart keeps conf=next from the record");
        finish(h2, q); }
    // 3) mockup case 3: PK+1 closes beyond but its low is NOT below the pivot bar's (PK) high -> no A, ever
    {   const std::string dir = home(std::string("c3") + (bearish ? "b" : "u")), path = recPath(dir);
        mock::Host h = chart(t0, 101.5, 100.75);
        add(h, 104.25, 106, 104.5, 105.75);                            // green, above the node, low 104.25 >= PK high 104
        add(h, 105, 106.5);
        DeltaProfile p; redraw(h, p);
        check(colourRings(h) == 0 && grayRings(h) == 1 && boardAll().empty() && countPrefix(slurp(path), "TA|") == 0, S + "case 3: no A when the next low is not below the pivot high (2.19.0: GRAY circle, no colour)");
        add(h, 99.5, 103, 100, 102.5); add(h, 101, 103); redraw(h, p);   // a later green candle closing above with a low below: still never
        check(colourRings(h) == 0 && grayRings(h) == 1 && dRows(slurp(path)).empty() && countPrefix(slurp(path), "TA|") == 0, S + "case 3: a later candle never confirms it (2.19.0: GRAY circle, no colour)");
        finish(h, p); }
    // 4) neither candle closes beyond (next closes back below the node in the wrong colour) -> no A, then a 3rd candle cannot
    {   const std::string dir = home(std::string("none") + (bearish ? "b" : "u")), path = recPath(dir);
        mock::Host h = chart(t0, 101.5, 100.75);
        add(h, 99.5, 103.5, 103, 100.5);                               // red: the next candle fails too
        add(h, 99.75, 103.25, 100, 103);                               // 3rd candle green above, lows below the pivot high
        add(h, 99.8, 103.2);
        DeltaProfile p; redraw(h, p);
        check(colourRings(h) == 0 && grayRings(h) == 1 && boardAll().empty() && countPrefix(slurp(path), "TA|") == 0, S + "neither closes beyond: no A; the 3rd candle never confirms (2.19.0: GRAY circle, no colour)");
        if (!bearish) h.bars.back().high = 104.5f; else h.bars.back().low = static_cast<float>(M(104.5));
        add(h, 100, 105); redraw(h, p);
        check(colourRings(h) == 0 && grayRings(h) == 1 && dRows(slurp(path)).empty(), S + "neither: no D line after the pivot is confirmed (2.19.0: GRAY circle, no colour)");
        finish(h, p); }
    // 5) next candle a doji above the node (not green / not red) -> no A
    {   const std::string dir = home(std::string("doji") + (bearish ? "b" : "u")), path = recPath(dir);
        mock::Host h = chart(t0, 101.5, 100.75);
        add(h, 99.5, 103.5, 102, 102); add(h, 99.75, 103.25);
        DeltaProfile p; redraw(h, p);
        check(colourRings(h) == 0 && grayRings(h) == 1 && countPrefix(slurp(path), "TA|") == 0, S + "next candle doji: no A (its own colour is required) (2.19.0: GRAY circle, no colour)");
        finish(h, p); }
    bear = false;
}

static void recordTests() {
    // a 24-field (2.17.0) D line = same; a 25-field one round-trips; a bad conf token / a 25-field F line is rejected
    Latch L; L.low = 400; L.high = 400; L.code = "A"; L.support = true; L.sell = true; L.peakTime = 1791540000; L.price = 100; L.id = 3;
    L.tFirst = 1791540060; L.tDecided = 1791540180; L.decidedBy = 1791540120; L.peakT = 1791540000; L.net = -10000; L.mult = 5.5;
    L.formedT = 1791540000; L.formedTime = 1791540000; L.conf = 'N';
    const std::string dn = recordLine(L, 'D', 0.25, "ESZ26"), fn = recordLine(L, 'F', 0.25, "ESZ26");
    check(fields(dn).size() == 25 && fields(dn)[24] == "conf=next" && fields(fn).size() == 24, "[rec] D line 25 fields (conf at the end), F line 24");
    RecordEvent r;
    check(parseRecordLine(dn, 0.25, r) && r.event == 'D' && r.latch.conf == 'N', "[rec] new D line parses back as next");
    L.conf = 'S'; const std::string ds = recordLine(L, 'D', 0.25, "ESZ26");
    check(parseRecordLine(ds, 0.25, r) && r.latch.conf == 'S' && fields(ds)[24] == "conf=same", "[rec] conf=same round-trips");
    const std::string old = ds.substr(0, ds.rfind('|'));
    check(fields(old).size() == 24 && parseRecordLine(old, 0.25, r) && r.latch.conf == 'S', "[rec] old 24-field D line = same");
    check(!parseRecordLine(old + "|conf=later", 0.25, r) && !parseRecordLine(fn + "|conf=next", 0.25, r), "[rec] bad conf token / 25-field F line rejected");
    // seedFromText: F + old D + TA 13 fields, and F + new D (next) + TA 14 fields
    {   Latch a = L; a.id = 0; a.conf = 'S'; Latch b = L; b.id = 1; b.peakTime = 1791540060; b.peakT = 1791540060; b.conf = 'N';
        const std::string aD = recordLine(a, 'D', 0.25, "ESZ26"), aOld = aD.substr(0, aD.rfind('|'));
        const std::string taOld = taLine(a, 'S', 1791540120, "ESZ26"), taOld13 = taOld.substr(0, taOld.rfind('|'));
        const std::string text = std::string("# c\n") + recordHeaderOld() + "\n" + recordLine(a, 'F', 0.25, "ESZ26") + "\n" + aOld + "\n" + taOld13 + "\n"
            + recordLine(b, 'F', 0.25, "ESZ26") + "\n" + recordLine(b, 'D', 0.25, "ESZ26") + "\n" + taLine(b, 'S', 1791540180, "ESZ26") + "\n";
        LatchBook bk; seedFromText(text, "ESZ26", "", 0.25, bk);
        check(bk.v.size() == 2 && bk.rejected == 0, "[rec] old and new lines seed together, none rejected");
        check(bk.v.size() == 2 && bk.v[0].conf == 'S' && bk.v[1].conf == 'N' && bk.v[0].logged && bk.v[1].logged, "[rec] conf seeded: old = same, new = next");
        check(bk.v.size() == 2 && bk.v[0].shownTent && bk.v[1].shownTent, "[rec] TA lines with 13 and 14 fields both accepted");
        check(fields(taLine(b, 'S', 1, "ESZ26")).size() == 14 && fields(taLine(b, 'S', 1, "ESZ26"))[13] == "conf=next", "[rec] TA line 14 fields, conf at the end"); }
    // header upgrade (lesson 66): a 2.17.0 file gets the 25-column header once, its lines untouched
    {   const std::string dir = home("hdr"); mkdir(dir.c_str(), 0700); const std::string p = dir + "x.csv";
        const std::string body = std::string("# lsDeltaProfile signals\n") + recordHeaderOld() + "\n" + recordLine(L, 'F', 0.25, "ESZ26") + "\n";
        { std::ofstream o(p.c_str(), std::ios::binary); o << body; }
        check(appendRecord(p, std::vector<std::string>(1, ds)), "[rec] append to a 2.17.0 file");
        const std::string t = slurp(p);
        check(t.find(std::string("\n") + recordHeader() + "\n") != std::string::npos && t.find(std::string("\n") + recordHeaderOld() + "\n") == std::string::npos,
              "[rec] the header now has 25 columns (old one gone)");
        check(t.find(recordLine(L, 'F', 0.25, "ESZ26") + "\n" + ds + "\n") != std::string::npos && fields(recordHeader()).size() == 25, "[rec] old lines kept, new line appended");
        check(appendRecord(p, std::vector<std::string>(1, ds)) && countPrefix(slurp(p), "t_first|") == 1, "[rec] a second append does not touch the header again"); }
}

// (2.18.0, Rassul 12:49 CT) the profile labels
static std::vector<std::string> ratioTexts(const mock::Host& h) {
    std::vector<std::string> v; for (const mock::Draw& d : h.draws) if (d.type == "text" && !d.text.empty() && d.text.back() == 'x') v.push_back(d.text); return v;
}
static void labelTests(unsigned long t0) {
    {   std::vector<dp::Bucket> b(5); const double d[5] = { 5, -900, 300, std::nan(""), -900 };
        for (int i = 0; i < 5; ++i) { b[i].key = 10 + i; b[i].delta = d[i]; }
        const std::vector<dp::Tick> k = dp::labelKeys(b);
        check(k.size() == 3 && k[0] == 11 && k[1] == 14 && k[2] == 12, "[lab] core: 3 biggest by abs delta, ties to the lower key, NaN skipped");
        check(dp::labelKeys(std::vector<dp::Bucket>(b.begin(), b.begin() + 2)).size() == 2, "[lab] core: fewer than 3 rows = all of them"); }
    bear = false;
    for (int fx = 0; fx < 2; ++fx) {   // fx 0: fillers lower the average so the 4th bar is 2.0x+; fx 1: small 3rd / 4th bars (3rd under 2.0x)
        home(fx ? "lab_small" : "lab_big");
        mock::Host h = chart(t0, 100.5, 101);
        auto put = [&](int bar, double px, long buy, long sell) { h.bars[bar].rows.push_back(mock::Price(static_cast<float>(px), buy, sell, buy + sell)); h.bars[bar].volume += static_cast<unsigned long>(buy + sell); };
        if (!fx) for (int i = 20; i <= 78; i += 2) put(i, 154 - 0.25 * i, 5, 0);
        put(90, 112, 3000, 0); put(91, 114, 0, fx ? 1200 : 2900); put(92, 116, fx ? 900 : 2800, 0);
        add(h, 99.5, 103.5);
        DeltaProfile p; redraw(h, p);
        const std::vector<dp::Bucket>& B = state(h).snapshot.buckets;
        std::vector<const dp::Bucket*> v; for (const dp::Bucket& b : B) v.push_back(&b);
        std::sort(v.begin(), v.end(), [](const dp::Bucket* a, const dp::Bucket* b) { return std::fabs(a->delta) > std::fabs(b->delta); });
        const std::vector<std::string> T = ratioTexts(h);
        auto has = [&](const std::string& t) { return std::find(T.begin(), T.end(), t) != T.end(); };
        const bool fixture = v.size() >= 4 && (fx ? v[2]->ratio < 2.0 : v[3]->ratio >= 2.0);
        check(fixture, fx ? "[lab] fixture: the 3rd biggest bar is under 2.0x" : "[lab] fixture: the 4th biggest bar is 2.0x or more");
        if (!fixture) continue;
        check(T.size() == 3, std::string("[lab] exactly 3 ratio labels (") + std::to_string(T.size()) + ")" + (fx ? " small" : " big"));
        check(has("A " + ratio(v[0]->ratio)) && has(ratio(v[1]->ratio)) && has(ratio(v[2]->ratio)), "[lab] the 3 biggest bars are labelled, the absorbed node keeps \"A \"");
        if (!fx) check(!has(ratio(v[3]->ratio)), "[lab] the 4th bar at 2.0x+ has no label");
        else check(has(ratio(v[2]->ratio)) && v[2]->ratio < 2.0, "[lab] a top-3 bar under 2.0x is labelled");
        check(colourRings(h) == 1, "[lab] the coloured candle circle unchanged (gray ones may sit on other candles)");
        finish(h, p);
    }
}

int main() {
    setenv("TZ", "UTC", 1); tzset();
    coreTests();
    const unsigned long t0 = 1791504000UL + 9 * 3600;
    hostTests(false, t0);
    hostTests(true, t0);
    recordTests();
    labelTests(t0);
    {   const std::string src = slurp("../DeltaProfile.cpp");
        check(std::string(DLT_VERSION) == "2.21.2", "[ver] this suite is for 2.18.x rules (2.19.0 = + gray circles, 2.20.0 = + wick A, 2.21.x = + trapped traders)");
        check(src.find(std::string("p->setVersion(\"") + DLT_VERSION + "\")") != std::string::npos, "[ver] DLT_VERSION = setVersion literal");
        check(slurp("../DeltaProfileCore.h").find(std::string("DeltaProfileCore.h ") + DLT_VERSION) != std::string::npos, "[ver] core header = DLT_VERSION"); }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
