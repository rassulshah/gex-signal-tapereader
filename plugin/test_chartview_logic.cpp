// test_chartview_logic.cpp -- unit tests for ChartViewLogic.h (no SDK).
//   g++ -std=c++17 -Wall -Wextra -I. test_chartview_logic.cpp -o test_chartview_logic && ./test_chartview_logic
#include "ChartViewLogic.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

static int passes = 0, fails = 0;
static void check(bool ok, const std::string& what) { if (ok) ++passes; else { ++fails; std::printf("FAIL %s\n", what.c_str()); } }
static void eq(const std::string& got, const std::string& want, const std::string& what)
{
    if (got == want) ++passes; else { ++fails; std::printf("FAIL %s\n  got:  %s\n  want: %s\n", what.c_str(), got.c_str(), want.c_str()); }
}

// a strict-enough JSON validator: structure, strings (escapes), numbers, literals; and every raw byte must be valid UTF-8
struct J {
    const std::string& s; size_t i = 0; bool ok = true;
    explicit J(const std::string& x) : s(x) {}
    void ws() { while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) i++; }
    bool lit(const char* w) { size_t n = std::string(w).size(); if (s.compare(i, n, w) == 0) { i += n; return true; } return false; }
    bool str() {
        if (i >= s.size() || s[i] != '"') { return false; }
        i++;
        while (i < s.size()) {
            unsigned char c = (unsigned char)s[i];
            if (c == '"') { i++; return true; }
            if (c < 0x20) return false;
            if (c == '\\') { i++; if (i >= s.size()) return false; char e = s[i];
                if (e == 'u') { for (int k = 1; k <= 4; k++) if (i + k >= s.size() || !std::isxdigit((unsigned char)s[i + k])) return false; i += 5; continue; }
                if (std::string("\"\\/bfnrt").find(e) == std::string::npos) { return false; }
                i++; continue; }
            if (c >= 0x80) { int len = (c >= 0xC2 && c <= 0xDF) ? 2 : (c >= 0xE0 && c <= 0xEF) ? 3 : (c >= 0xF0 && c <= 0xF4) ? 4 : 0;
                if (!len || i + len > s.size()) { return false; }
                for (int k = 1; k < len; k++) { if (((unsigned char)s[i + k] & 0xC0) != 0x80) return false; }
                i += len; continue; }
            i++;
        }
        return false;
    }
    bool number() { size_t a = i; if (i < s.size() && s[i] == '-') i++; while (i < s.size() && (std::isdigit((unsigned char)s[i]) || s[i] == '.' || s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-')) i++; return i > a && std::isdigit((unsigned char)s[i - 1]); }
    bool value() {
        ws(); if (i >= s.size()) return false;
        char c = s[i];
        if (c == '{') { i++; ws(); if (i < s.size() && s[i] == '}') { i++; return true; }
            for (;;) { ws(); if (!str()) return false; ws(); if (i >= s.size() || s[i] != ':') return false; i++; if (!value()) return false; ws();
                if (i < s.size() && s[i] == ',') { i++; continue; } if (i < s.size() && s[i] == '}') { i++; return true; } return false; } }
        if (c == '[') { i++; ws(); if (i < s.size() && s[i] == ']') { i++; return true; }
            for (;;) { if (!value()) return false; ws(); if (i < s.size() && s[i] == ',') { i++; continue; } if (i < s.size() && s[i] == ']') { i++; return true; } return false; } }
        if (c == '"') return str();
        if (lit("true") || lit("false") || lit("null")) return true;
        return number();
    }
    bool all() { bool v = value(); ws(); return v && i == s.size(); }
};
static bool validJson(const std::string& s) { J j(s); return j.all(); }

int main()
{
    // ---- market mapping (reuses dl::marketForRoot)
    eq(cvl::marketOf("ES", "EPZ26"), "ES", "ES");
    eq(cvl::marketOf("EP", "EPZ26"), "ES", "EP root = ES");
    eq(cvl::marketOf("NQ", "NQ"), "NQ", "NQ");
    eq(cvl::marketOf("CL", "CLEX26"), "CL", "CL");
    eq(cvl::marketOf("QGC", "QGC"), "GC", "QGC = GC (not NG)");
    eq(cvl::marketOf("HG", "CPEZ26"), "HG", "HG");
    eq(cvl::marketOf("CP", "CPEZ26"), "HG", "CP root = HG");
    eq(cvl::marketOf("NG", "NGEX26"), "NG", "NG");
    eq(cvl::marketOf("6E", "6EZ26"), "EU", "6E = EU");
    eq(cvl::marketOf("EU", "EUZ26"), "EU", "EU");
    eq(cvl::marketOf("MES", "MESZ26"), "ES", "micro MES = ES");
    eq(cvl::marketOf("MNQ", "MNQZ26"), "NQ", "micro MNQ = NQ");
    eq(cvl::marketOf("MCL", "MCLX26"), "CL", "micro MCL = CL");
    eq(cvl::marketOf("MGC", "MGCZ26"), "GC", "micro MGC = GC");
    eq(cvl::marketOf("", "@ES#"), "ES", "empty root falls back to the symbol");
    eq(cvl::marketOf("ZB", "ZBZ26"), "ZB", "unknown root keeps its name");
    eq(cvl::marketOf("", ""), "UNKNOWN", "nothing = UNKNOWN");
    eq(cvl::marketOf("a/b c", ""), "ABC", "unknown root sanitised (file-name safe)");

    // ---- period
    check(cvl::periodOf(180, {}) == 180, "SDK seconds used");
    check(cvl::periodOf(-1, { 0, 180, 360, 540 }) == 180, "period from stamps when the SDK says -1");
    check(cvl::periodOf(0, { 0, 3600, 3600, 10800 }) == 3600, "period: smallest positive gap");
    check(cvl::periodOf(-1, { 5 }) == 0, "period unknown = 0");
    eq(cvl::fileStem("ES", 180), "ES_180", "file stem 3-min");
    eq(cvl::fileStem("EU", 3600), "EU_3600", "file stem hourly");

    // ---- label file
    {
        std::string f = "\xEF\xBB\xBF# starter list\r\nCI[Biggest_Wave_Price_Since_Bars1]\r\n\r\n  BV_Since  \n# off: lsTapeFlowES\nupdn[Distance_Ratio_updn]\nBV_Since\n";
        std::vector<std::string> v = cvl::parseLabelFile(f);
        check(v.size() == 3, "label file: 3 labels (comments, blanks, BOM, CR, duplicate dropped)");
        if (v.size() == 3) { eq(v[0], "CI[Biggest_Wave_Price_Since_Bars1]", "label 1"); eq(v[1], "BV_Since", "label 2 trimmed"); eq(v[2], "updn[Distance_Ratio_updn]", "label 3"); }
        std::string many; for (int i = 0; i < 100; i++) many += "L" + std::to_string(i) + "\n";
        check((int)cvl::parseLabelFile(many).size() == cvl::MAX_LABELS, "label file capped");
        check(cvl::parseLabelFile("").empty(), "empty file = no labels");
        check(cvl::parseLabelFile("no newline at end").size() == 1, "last line without newline");
    }
    {
        std::vector<std::string> e = cvl::parseExtra(" lsSessionVWAP , lsTapeFlowES;; none ,");
        check(e.size() == 2 && e[0] == "lsSessionVWAP" && e[1] == "lsTapeFlowES", "extra labels: comma / semicolon, 'none' and blanks dropped");
        check(cvl::parseExtra("none").empty(), "'none' = no extra");
        std::vector<std::string> m = cvl::mergeLabels({ "A", "B" }, { "B", "C" });
        check(m.size() == 3 && m[2] == "C", "merge keeps order, no duplicates");
        std::vector<std::string> lv = cvl::labelVariants("CI[EntryLevel]");
        check(lv.size() == 2 && lv[1] == "EntryLevel", "CI[x] also tried as x");
        check(cvl::labelVariants("BV_Since").size() == 1, "plain label: one variant");
        eq(cvl::customName("CI[CobLTFEntry]"), "CobLTFEntry", "custom-indicator name");
        eq(cvl::customName("updn[Distance_Ratio_updn]"), "", "no custom try for non-CI labels");
    }

    // ---- want file
    check(cvl::wantMatches("", "ES", 180), "blank want = every chart");
    check(cvl::wantMatches("*", "NQ", 3600), "* = every chart");
    check(cvl::wantMatches("es", "ES", 180), "market token, case-insensitive");
    check(cvl::wantMatches("NQ, ES_180", "ES", 180), "exact chart token");
    check(!cvl::wantMatches("ES_3600", "ES", 180), "other period does not answer");
    check(!cvl::wantMatches("# just a comment\nCL", "ES", 180), "other market does not answer");

    // ---- scheduling
    {
        cvl::Sched s;
        check(cvl::due(s, 1, 0, 0, false) == cvl::NONE, "fewer than 2 bars: nothing");
        check(cvl::due(s, 500, 1000, 0, false) == cvl::FIRST, "first build");
        cvl::built(s, 500, 1000, 0);
        check(cvl::due(s, 500, 1000, 5000, false) == cvl::NONE, "forming bar within 10 s: nothing");
        check(cvl::due(s, 500, 1000, 9999, false) == cvl::NONE, "9.999 s: nothing");
        check(cvl::due(s, 500, 1000, 10000, false) == cvl::TIMER, "10 s: forming-bar rebuild");
        check(cvl::due(s, 501, 1180, 2000, false) == cvl::NEWBAR, "a new bar: at once");
        check(cvl::due(s, 500, 1180, 2000, false) == cvl::NEWBAR, "same count, new stamp (rolling window): new bar");
        check(cvl::due(s, 500, 1000, 2000, true) == cvl::WANT, "request: at once");
        check(cvl::due(s, 500, 1000, -50, false) == cvl::TIMER, "clock stepped back: rebuild");
        check(!cvl::shouldWrite(cvl::TIMER, 7, 7, true), "unchanged forming bar is not written");
        check(cvl::shouldWrite(cvl::TIMER, 8, 7, true), "changed forming bar is written");
        check(cvl::shouldWrite(cvl::WANT, 7, 7, true), "a request is always written");
        check(cvl::shouldWrite(cvl::NEWBAR, 9, 8, true), "new bar written");
        check(cvl::shouldWrite(cvl::TIMER, 7, 7, false), "never written yet: written");
        check(!cvl::shouldWrite(cvl::NONE, 1, 2, true), "NONE never writes");
    }

    // ---- JSON text
    eq(cvl::jsonEscape("a\"b\\c\nd\te\r"), "a\\\"b\\\\c\\nd\\te\\r", "escape quotes, backslash, controls");
    eq(cvl::jsonEscape(std::string("x\x01y", 3)), "x\\u0001y", "escape other control chars");
    eq(cvl::jsonEscape("caf\xC3\xA9"), "caf\xC3\xA9", "valid UTF-8 passes through");
    eq(cvl::jsonEscape("dash \x97 here"), "dash \\u0097 here", "Windows-1252 byte escaped");
    eq(cvl::jsonEscape("\xE2\x82\xAC"), "\xE2\x82\xAC", "3-byte UTF-8 (euro) passes");
    eq(cvl::jsonEscape("\xE2\x82"), "\\u00e2\\u0082", "truncated UTF-8 escaped");
    eq(cvl::jsonEscape("\xC0\xAF"), "\\u00c0\\u00af", "overlong UTF-8 escaped");
    eq(cvl::jsonEscape("\xED\xA0\x80"), "\\u00ed\\u00a0\\u0080", "UTF-16 surrogate in UTF-8 escaped");
    eq(cvl::num(6712.25), "6712.25", "ES price");
    eq(cvl::num((double)1.17345f), "1.17345", "EU price from a float");
    eq(cvl::num((double)25000.25f), "25000.25", "NQ price from a float");
    eq(cvl::num(0.0), "0", "zero");
    eq(cvl::num(-0.0), "0", "negative zero");
    eq(cvl::num(std::numeric_limits<double>::quiet_NaN()), "null", "NaN = null");
    eq(cvl::num(std::numeric_limits<double>::infinity()), "null", "inf = null");
    eq(cvl::num(12345678.0), "12345678", "8 digits whole");
    eq(cvl::num(-3.5), "-3.5", "negative");
    check(cvl::fnv1a("abc") != cvl::fnv1a("abd"), "hash differs on change");
    check(cvl::fnv1a("abc") == cvl::fnv1a(std::string("abc")), "hash stable");

    // ---- time
    {
        cvl::Civil l; l.y = 2026; l.mo = 10; l.d = 9; l.h = 10; l.mi = 15; l.s = 3;
        cvl::Civil u = l; u.h = 15;
        check(cvl::utcOffsetMin(l, u) == -300, "CDT offset -300");
        eq(cvl::iso(l, -300), "2026-10-09T10:15:03-05:00", "ISO with offset");
        cvl::Civil l2 = l; l2.d = 31; l2.mo = 12; l2.h = 20; cvl::Civil u2 = l2; u2.y = 2027; u2.mo = 1; u2.d = 1; u2.h = 2;
        check(cvl::utcOffsetMin(l2, u2) == -360, "CST offset across the year end");
        eq(cvl::iso(l2, -360), "2026-12-31T20:15:03-06:00", "ISO CST");
        eq(cvl::iso(l, 330), "2026-10-09T10:15:03+05:30", "positive half-hour offset");
        eq(cvl::stamp(l), "2026-10-09 10:15:03", "bar stamp");
        long long d = cvl::daysFromCivil(2026, 10, 9);
        eq(cvl::dateOfDays(d), "2026-10-09", "civil round trip");
        eq(cvl::dateOfDays(d + 23), "2026-11-01", "month roll");
        eq(cvl::dateOfDays(cvl::daysFromCivil(2028, 2, 28) + 1), "2028-02-29", "leap day");
        check(cvl::daysFromCivil(1970, 1, 1) == 0, "epoch day 0");
    }

    // ---- sources and cuts
    {
        long long today = cvl::daysFromCivil(2026, 10, 9);
        std::vector<cvl::Src> v = cvl::sourcesFor("ES", "EPZ26", today);
        bool tf = false, odds = false, dealer = false, gp = false, ev = false;
        for (auto& s : v) {
            if (s.rel == "TapeFlow.status-ES.txt") tf = true;
            if (s.rel == "SessionVWAP-ES.odds.txt") odds = true;
            if (s.rel == "LRA-Dealer-ES.csv" && s.cut == cvl::HEADTAIL) dealer = true;
            if (s.rel == "GammaProfile.status-SPX-Right.txt") gp = true;
            if (s.rel == "TapeFlow\\ES-events-v110-EPZ26-2026-10-10.csv" && s.cut == cvl::TAIL) ev = true;
        }
        check(tf && odds && dealer && gp && ev, "ES sources include TapeFlow / VWAP odds / Dealer data / Gamma status / TapeFlow events");
        eq(cvl::contractTag("@ES#"), "_ES_", "contract tag like TapeFlow::evPath");
        cvl::Plan p = cvl::planRead(1000, cvl::HEAD, 4096);
        check(!p.truncated && p.headLen == 1000, "small file whole");
        p = cvl::planRead(100000, cvl::HEAD, 1000); check(p.truncated && p.headLen == 1000 && p.tailFrom == 100000, "head cut");
        p = cvl::planRead(100000, cvl::TAIL, 1000); check(p.truncated && p.headLen == 0 && p.tailFrom == 99000, "tail cut");
        p = cvl::planRead(100000, cvl::HEADTAIL, 1000); check(p.truncated && p.headLen == 250 && p.tailFrom == 100000 - 750, "head+tail cut");
        check(p.headLen + (100000 - p.tailFrom) <= 1000, "cut within cap");
        std::string j = cvl::joinCut("h1\nh2\npart", "ial\nt1\nt2\n", p, 100000);
        check(j.find("h1\nh2\n...[ChartView: 100000 bytes") == 0, "cut head ends at a full line");
        check(j.size() >= 6 && j.compare(j.size() - 6, 6, "t1\nt2\n") == 0 && j.find("ial") == std::string::npos, "cut tail starts at a full line");
        cvl::Plan w = cvl::planRead(5, cvl::HEAD); eq(cvl::joinCut("abcde", "", w, 5), "abcde", "uncut text unchanged");
    }

    // ---- the document
    {
        cvl::Snap s; s.market = "ES"; s.symbol = "EPZ26"; s.root = "ES"; s.chart = "ES 3 Min \"LTF\""; s.period = 180; s.periodicity = "3 Minute";
        s.chartBars = 1200; s.barsWanted = 2;
        s.t = { "2026-10-09 10:12:00", "2026-10-09 10:15:00" }; s.o = { 6700, 6701.25 }; s.h = { 6702, 6703 }; s.l = { 6699.5, 6700 };
        s.c = { 6701.25, 6702.75 }; s.v = { 12000, 8000 };
        cvl::Series x; x.label = "CI[Biggest_Wave_Price_Since_Bars1]"; x.name = "Biggest Wave"; x.textLabel = "BW"; x.via = "chart";
        x.arrayNo = { 0, 1 }; x.values = { { 12, std::numeric_limits<double>::quiet_NaN() }, { 0.5, 0.25 } };
        s.series.push_back(x);
        s.missing = { "lsTapeFlowES" };
        cvl::FileBlob f; f.name = "DealerProfile.status.txt"; f.bytes = 40; f.mtime = "2026-10-09 10:14:59"; f.text = "VERSION,2.5.1\nSTATE,drawn \x97 ok\n";
        s.files.push_back(f); s.filesSkipped = { "LRA-Touch-ES.csv (read failed)" };
        s.watchFile = "C:\\Users\\r\\InvestorRT\\rtx\\lsFlexLevels\\ChartView.labels.txt"; s.watchFromFile = 6; s.watchExtra = 1;
        std::string b = cvl::body(s);
        std::string d = cvl::document(b, "2026-10-09T10:15:03-05:00", 1791558903, cvl::NEWBAR);
        check(validJson(d), "the snapshot is valid JSON (UTF-8 safe)");
        check(d.find("\"version\":\"1.1.0\"") != std::string::npos, "version in header");
        check(d.find("\"why\":\"new bar\"") != std::string::npos, "why in header");
        check(d.find("\"arrays\":{\"0\":[12,null],\"1\":[0.5,0.25]}") != std::string::npos, "series arrays keyed by array number, NaN = null");
        check(d.find("\"missing\":[\"lsTapeFlowES\"]") != std::string::npos, "missing listed");
        check(d.find("\"bars\":{\"n\":2,") != std::string::npos, "bar count");
        check(d.find("\\u0097") != std::string::npos, "status text cp1252 byte escaped");
        std::string d2 = cvl::document(b, "2026-10-09T10:15:13-05:00", 1791558913, cvl::TIMER);
        check(cvl::fnv1a(b) == cvl::fnv1a(cvl::body(s)), "same content = same hash (written time not in the body)");
        check(d2 != d, "documents differ only in the header");
        cvl::Snap s2 = s; s2.c[1] = 6703.0;
        check(cvl::fnv1a(cvl::body(s2)) != cvl::fnv1a(b), "forming bar change = new hash");
        cvl::Snap e; check(validJson(cvl::document(cvl::body(e), "x", 0, cvl::FIRST)), "an empty snapshot is valid JSON");
    }

    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
