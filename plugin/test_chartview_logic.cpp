// test_chartview_logic.cpp -- unit tests for ChartViewLogic.h 1.2.0 (no SDK).
//   g++ -std=c++17 -Wall -Wextra -I. test_chartview_logic.cpp -o test_chartview_logic   then   CV_FIXTURES=tests_cv110/fixtures ./test_chartview_logic
#include "ChartViewLogic.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
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
    {   // (1.2.0) labels: legend form, cleaning, lookup names, custom names, SDK fixed fields
        std::vector<std::string> lv = cvl::lookupNames("CI[EntryLevel]");
        check(lv.size() == 2 && lv[0] == "CI[EntryLevel]" && lv[1] == "EntryLevel", "CI[x]: as written, then x (not 'CI')");
        lv = cvl::lookupNames("updn[Distance_Ratio_updn]");
        check(lv.size() == 3 && lv[1] == "Distance_Ratio_updn" && lv[2] == "updn", "updn[y]: as written, y, then updn");
        check(cvl::lookupNames("BV_Since").size() == 1, "plain label: one name");
        eq(cvl::customName("CI[CobLTFEntry]"), "CobLTFEntry", "custom name of CI[x]");
        eq(cvl::customName("updn[Distance_Ratio_updn]"), "Distance_Ratio_updn", "custom name of updn[y] = y");
        eq(cvl::customName("cob_bull"), "cob_bull", "a plain name is also tried as a custom indicator (as DealerProfile does)");
        eq(cvl::customName("a[b"), "", "unbalanced brackets: no custom try");
        eq(cvl::cleanLabel("  updn[Distance_Ratio_updn]=0.37 "), "updn[Distance_Ratio_updn]", "legend '=value' tail dropped");
        eq(cvl::cleanLabel("\"CI[EntryLevel]\""), "CI[EntryLevel]", "quotes dropped");
        eq(cvl::cleanLabel("CI[Entry\x01Level]"), "CI[EntryLevel]", "control bytes dropped");
        eq(cvl::cleanLabel("CI[Entry\xE2\x80\xA6]"), "CI[Entry???]", "non-ASCII shown as ?, never garbage");
        eq(cvl::cleanLabel("a   b"), "a b", "blanks collapsed");
        check(cvl::cleanLabel(std::string(500, 'x')).size() == cvl::LABEL_MAX, "label length capped");
        cvl::LabelParts lp = cvl::splitLabel("CI[A[1]]");
        check(lp.bracket && lp.prefix == "CI" && lp.inner == "A[1]", "inner name may hold brackets");
        check(!cvl::splitLabel("plain").bracket && !cvl::splitLabel("x[]").bracket && !cvl::splitLabel("x[y]z").bracket, "not a legend form");
        char f16[16]; std::memcpy(f16, "Distance_Ratio_u", 16); bool cut = false;
        eq(cvl::fixedField(f16, 16, &cut), "Distance_Ratio_u", "16-byte field read to its end, not past it"); check(cut, "and flagged as cut");
        char f8[8] = { 'a', 'b', 0, 'z', 'z', 'z', 'z', 'z' }; eq(cvl::fixedField(f8, 8, &cut), "ab", "stops at NUL"); check(!cut, "not cut");
        char fx[4] = { 'a', (char)0xC3, 0x07, 0 }; eq(cvl::fixedField(fx, 4), "a??", "non-printable shown as ?");
        std::string u16le = "\xFF\xFE"; for (char c : std::string("CI[A]\r\nB\r\n")) { u16le += c; u16le += '\0'; }
        std::vector<std::string> u = cvl::parseLabelFile(u16le);
        check(u.size() == 2 && u[0] == "CI[A]" && u[1] == "B", "UTF-16 LE labels file (Notepad 'Unicode')");
        std::string u16be = "\xFE\xFF"; for (char c : std::string("Q\n")) { u16be += '\0'; u16be += c; }
        u = cvl::parseLabelFile(u16be); check(u.size() == 1 && u[0] == "Q", "UTF-16 BE labels file");
        std::string nobom; for (char c : std::string("CI[Z]\n")) { nobom += c; nobom += '\0'; }
        u = cvl::parseLabelFile(nobom); check(u.size() == 1 && u[0] == "CI[Z]", "UTF-16 without a BOM");
        u = cvl::parseLabelFile("updn[Distance_Ratio_updn]=0\nupdn[Distance_Ratio_updn]\n");
        check(u.size() == 1, "a legend copy and the clean label are the same label");
    }

    // ---- want / picture requests
    check(cvl::wantMatches("", "ES", 180), "blank want = every chart");
    check(cvl::wantMatches("*", "NQ", 3600), "* = every chart");
    check(cvl::wantMatches("es", "ES", 180), "market token, case-insensitive");
    check(cvl::wantMatches("NQ, ES_180", "ES", 180), "exact chart token");
    check(!cvl::wantMatches("ES_3600", "ES", 180), "other period does not answer");
    check(!cvl::wantMatches("# just a comment\nCL", "ES", 180), "other market does not answer");
    {
        cvl::Request r = cvl::parseRequest("ES_180 1791603738\r\n");
        check(r.epoch == 1791603738 && r.toks.size() == 1 && r.toks[0] == "ES_180", "'ES_180 <epoch>': token + epoch");
        check(cvl::requestMatches(r, "ES", 180) && !cvl::requestMatches(r, "ES", 3600) && !cvl::requestMatches(r, "NQ", 180), "ES_180 only");
        r = cvl::parseRequest("* 1791603738"); check(cvl::requestMatches(r, "GC", 3600), "'* <epoch>' = every chart (the epoch is not a market)");
        check(cvl::requestFresh(r, 1791603738 + 60, 0, true), "a 60-s-old request is fresh");
        check(!cvl::requestFresh(r, 1791603738 + 600, 0, false), "a 10-min-old request is stale");
        check(!cvl::requestFresh(r, 1791603738 - 600, 0, false), "a request 10 min in the future is refused");
        cvl::Request n = cvl::parseRequest("GC");
        check(cvl::requestFresh(n, 1000, 990, true) && !cvl::requestFresh(n, 1000, 900, true) && cvl::requestFresh(n, 1000, 900, false), "no epoch: only a leftover file at load is ignored");
        check(cvl::parseRequest("12345678").epoch == -1 && cvl::parseRequest("99999999999999").epoch == -1, "8 or 14 digits are not an epoch");
    }

    // ---- (1.2.0) which window is this chart
    {
        const std::string es3 = "ES LTF: EPZ26 (3m*), EPZ26 (1m) 3 Minutes*, Full Session 17:00-16:00";
        const std::string es60 = "ES HTF: EPZ26 (60m*) 60 Minutes*, Full Session 17:00-16:00";
        const std::string cl3 = "CL LTF: CLEX26 (3m*) Crude Light (Globex): November 2026 3 Minutes*, Full Session 17:00-16:00";
        const std::string esH = "ES HTF: EPZ26 (1h*) 1 Hour*, Full Session";
        check(cvl::titleScore(es3, "EPZ26", 180, "") == 8, "ES 3-min title: phrase + code = 8");
        check(cvl::titleScore(es3, "EPZ26", 3600, "") == 0, "ES 3-min title is NOT the 60-min chart");
        check(cvl::titleScore(es3, "EPZ26", 60, "") == 0, "the overlay's '(1m)' does not make it a 1-min chart");
        check(cvl::titleScore(es60, "EPZ26", 3600, "") == 8, "ES 60-min title");
        check(cvl::titleScore(es60, "EPZ26", 180, "") == 0, "ES 60-min title is not the 3-min chart");
        check(cvl::titleScore(esH, "EPZ26", 3600, "") == 8, "'1 Hour' / '(1h*)' = 3600");
        check(cvl::titleScore(cl3, "CLEX26", 180, "3 Minutes*") == 10, "periodicity label adds 2 when IRT gives it");
        check(cvl::titleScore(cl3, "EPZ26", 180, "") == -1, "other symbol = -1");
        check(cvl::titleScore("X: EPZ26 (13m*) 13 Minutes*", "EPZ26", 180, "") == 0, "13 Minutes is not 3 Minutes");
        check(cvl::titleScore("X: EPZ26 (30s*) 30 Seconds*", "EPZ26", 30, "") == 8, "seconds charts");
        check(cvl::titleScore("X: EPZ26 (1d*) Daily", "EPZ26", 86400, "") == 8, "daily charts");
        std::vector<cvl::WinCand> c(4);
        c[0].title = "Investor/RT"; c[0].area = 9000000;
        c[1].title = es3; c[1].area = 1100 * 1049; c[1].w = 1100; c[1].h = 1049;
        c[2].title = es60; c[2].area = 1300 * 1100; c[2].w = 1300; c[2].h = 1100;
        c[3].title = "EPZ26 Quote"; c[3].area = 60000;
        cvl::Pick p = cvl::pickChart(c, "EPZ26", 180, "", 0, 0);
        check(p.index == 1 && p.matches == 1, "ES_180 picks the 3-min window though the 60-min one is bigger (the 1.1.0 bug)");
        p = cvl::pickChart(c, "EPZ26", 3600, "", 0, 0); check(p.index == 2, "ES_3600 picks the 60-min window");
        c[1].iconic = true; p = cvl::pickChart(c, "EPZ26", 180, "", 0, 0);
        check(p.index == -1 && p.symbolOnly >= 1 && p.why.find("none shows its bar size") != std::string::npos, "minimised chart + symbol-only windows: none taken, reason given");
        c[1].iconic = false; c.push_back(c[1]); c.back().area = 5; c.back().w = 900; c.back().h = 700;
        p = cvl::pickChart(c, "EPZ26", 180, "", 900, 700); check(p.index == 4, "two equal titles: the one nearest the pane size");
        p = cvl::pickChart(c, "EPZ26", 180, "", 0, 0); check(p.index == 1, "two equal titles, no size: the larger");
        check(cvl::pickChart(c, "", 180, "", 0, 0).index == -1, "no symbol: nothing");
        check(cvl::periodPhrases(0).empty() && cvl::periodCodes(-1).empty(), "unknown bar size: no phrases");
    }

    // ---- (1.2.0) blank pictures: his real ES_180 (black price pane) vs good CL / GC / HG pictures, at half size
    {
        auto ppm = [](const std::string& p, int& w, int& h) {
            std::vector<unsigned char> px; FILE* f = std::fopen(p.c_str(), "rb"); w = h = 0; if (!f) return px;
            int mx = 0; if (std::fscanf(f, "P6 %d %d %d", &w, &h, &mx) == 3 && w > 0 && h > 0 && mx == 255) { std::fgetc(f); px.resize((size_t)w * h * 3); if (std::fread(px.data(), 1, px.size(), f) != px.size()) px.clear(); }
            std::fclose(f); return px;
        };
        const char* dirEnv = std::getenv("CV_FIXTURES"); std::string dir = dirEnv ? dirEnv : "tests_cv110/fixtures";
        int w = 0, h = 0;
        std::vector<unsigned char> es = ppm(dir + "/blank_ES_180_2026-10-09.ppm", w, h);
        check(!es.empty(), "fixture ES_180 loaded");
        if (!es.empty()) { cvl::Blank b = cvl::blankCheck(es.data(), w, h); check(b.blank && b.frac > 0.99, "his blank ES_180 picture is detected (" + std::to_string(b.frac) + ")"); }
        const char* good[3] = { "/good_CL_180_2026-10-09.ppm", "/good_GC_180_2026-10-09.ppm", "/good_HG_3600_2026-10-09.ppm" };
        for (int k = 0; k < 3; k++) {
            std::vector<unsigned char> g = ppm(dir + good[k], w, h);
            check(!g.empty(), std::string("fixture ") + good[k]);
            if (!g.empty()) { cvl::Blank b = cvl::blankCheck(g.data(), w, h); check(!b.blank && b.frac < 0.9, std::string("good picture not blank ") + good[k] + " (" + std::to_string(b.frac) + ")"); }
        }
        std::vector<unsigned char> zero(400 * 300 * 3, 0);
        check(cvl::blankCheck(zero.data(), 400, 300).blank, "an all-black bitmap (PrintWindow painted nothing) is blank");
        check(cvl::blankCheck(nullptr, 400, 300).blank && cvl::blankCheck(zero.data(), 10, 10).blank, "no / tiny bitmap = blank");
        check(cvl::afterCapture(true, false, 0) == cvl::SAVE && cvl::afterCapture(true, true, 0) == cvl::RETRY && cvl::afterCapture(true, true, 1) == cvl::RETRY
              && cvl::afterCapture(true, true, 2) == cvl::GIVE_UP && cvl::afterCapture(false, false, 2) == cvl::GIVE_UP, "save / retry / give up after 3 tries");
        check(cvl::printFlags(0) == 2 && cvl::printFlags(1) == 2 && cvl::printFlags(2) == 0, "PW_RENDERFULLCONTENT, again, then classic");
        cvl::SnapStatus st; st.result = "OK"; st.window = "ES LTF"; st.attempts = 1; st.blankFrac = 0.75;
        std::string t = cvl::snapStatusText(st);
        check(t.find("VERSION|1.2.0\nRESULT|OK\n") == 0 && t.find("BLANK_FRAC|0.7500") != std::string::npos, "status text");
    }

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
    eq(cvl::num(3000000123.0), "3000000123", "volume above 2^31 exact (#2)");
    eq(cvl::num(123456789012.0), "123456789012", "large whole number exact");
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
        cvl::Series x; x.label = "CI[Biggest_Wave_Price_Since_Bars1]"; x.foundAs = "Biggest_Wave_Price_Since_Bars1"; x.sdkName = "Biggest Wave"; x.sdkTextLabel = "BW"; x.via = "chart";
        x.arrayNo = { 0, 1 }; x.values = { { 12, std::numeric_limits<double>::quiet_NaN() }, { 0.5, 0.25 } };
        s.series.push_back(x);
        s.missing = { "lsTapeFlowES" };
        cvl::FileBlob f; f.name = "DealerProfile.status.txt"; f.bytes = 40; f.mtime = "2026-10-09 10:14:59"; f.text = "VERSION,2.5.1\nSTATE,drawn \x97 ok\n";
        s.files.push_back(f); s.filesSkipped = { "LRA-Touch-ES.csv (read failed)" };
        s.watchFile = "C:\\Users\\r\\InvestorRT\\rtx\\lsFlexLevels\\ChartView.labels.txt"; s.watchFromFile = 6;
        std::string b = cvl::body(s);
        std::string d = cvl::document(b, "2026-10-09T10:15:03-05:00", 1791558903, cvl::NEWBAR);
        check(validJson(d), "the snapshot is valid JSON (UTF-8 safe)");
        check(d.find("\"version\":\"1.2.0\"") != std::string::npos, "version in header");
        check(d.find("\"legend_prefix\":\"CI\",\"legend_name\":\"Biggest_Wave_Price_Since_Bars1\"") != std::string::npos, "legend parts in the export");
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

    {   // (1.1.0) the camera's PNG encoder: signature, IHDR size, IEND, and a > 64 KB image (several stored blocks)
        std::vector<unsigned char> px(300 * 250 * 3, 7);
        std::string png = cvl::pngEncode(px.data(), 300, 250);
        check(png.size() > 64 * 1024 && png.compare(0, 8, std::string("\x89PNG\r\n\x1a\n", 8)) == 0, "png signature + size");
        check((unsigned char)png[16] == 0 && (unsigned char)png[18] == 1 && (unsigned char)png[19] == 44 && (unsigned char)png[22] == 0 && (unsigned char)png[23] == 250, "png IHDR 300 x 250");
        check(png.compare(png.size() - 8, 4, "IEND") == 0, "png ends with IEND");
        check(cvl::pngEncode(nullptr, 3, 3).empty() && cvl::pngEncode(px.data(), 0, 3).empty(), "png refuses bad input");
    }
    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
