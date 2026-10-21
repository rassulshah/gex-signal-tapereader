// test_keylevels.cpp -- lsKeyLevels 1.0.0 (Rassul 2026-10-20 22:20-22:41 CT): the hourly-chart Key Levels indicator.
//   [set]   defaults: every price / volume group on, MenthorQ OFF, font 9, width 1; file round trip; garbage -> defaults; spec valid
//   [logic] Open / hourly swings / pivot rows never drawn; a merged level keeps only ticked names; colours = his SessionPrices
//           colours; MenthorQ names (CR0, PS · HVL, STALE kept) and colours; right-justified badge placement never overlaps
//   [host]  a 60-min ES chart reading the real-format LRA-KeyLevels-ES.csv: lines run across the whole pane, badges "NAME  price"
//           right-justified at the pane's right edge, no Open; MenthorQ only when ticked (ES contract only, NQ's not mixed in);
//           a file older than 80 h / missing draws nothing and says why; an NQ chart (indicator copied over) reads NQ's file;
//           group toggles + font / width from the settings file; the settings window round trip (open, change, OK)
// Build (from tests_keylevels/): g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. -I.. test_keylevels.cpp -o /tmp/t_kl && /tmp/t_kl
#include "irtsdk.h"
#include "../KeyLevels.cpp"
#include <iostream>
#include <map>
#include <set>
#include <sys/stat.h>
namespace mock { Host* current = nullptr; }
static int passed = 0, failed = 0;
#define CHECK(c, why) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL line " << __LINE__ << ": " << why << "\n"; } } while (0)
static const std::string MID = "\xB7";

static void settingsTests() {
    const ss::Spec& S = klhset::spec();
    std::vector<std::string> errs = ss::check(S);
    CHECK(errs.empty(), "[set] the spec validates" + (errs.empty() ? std::string() : ": " + errs[0]));
    const ss::Values D = ss::defaults(S, ss::M_ES);
    bool allOn = true; for (int g = klh::G_PF; g <= klh::G_WPOC; ++g) allOn = allOn && D.b(klh::showKey(g));
    CHECK(allOn, "[set] every price and volume group is on by default");
    CHECK(!D.b(klhset::K_SHOW_MQ), "[set] MenthorQ levels OFF by default");
    CHECK(D.i(klhset::K_FONT) == 9 && D.i(klhset::K_WIDTH) == 1, "[set] font 9, line width 1");
    CHECK(D.c(klhset::K_COL_PF) == 0x008A8Aul && D.c(klhset::K_COL_PD) == 0xFFA500ul && D.c(klhset::K_COL_PW) == 0x0000FFul && D.c(klhset::K_COL_ON) == 0x808080ul, "[set] SessionPrices colours: PF teal, PD orange, PW blue, ON grey");
    CHECK(D.c(klhset::K_COL_DPOC) == 0x22D3EEul && D.c(klhset::K_COL_VA) == 0xEF4444ul && D.c(klhset::K_COL_WPOC) == 0xE8D5B0ul, "[set] DPoc cyan, VAH / VAL red, WPoc beige (the Dealer Profile's)");
    ss::Values V = D; V.v[klhset::K_SHOW_PD] = 0; V.v[klhset::K_SHOW_MQ] = 1; V.v[klhset::K_FONT] = 14; V.v[klhset::K_COL_PF] = 0x123456;
    ss::Values W; CHECK(ss::parse(S, ss::M_ES, ss::format(S, V), W) && !W.b(klhset::K_SHOW_PD) && W.b(klhset::K_SHOW_MQ) && W.i(klhset::K_FONT) == 14 && W.c(klhset::K_COL_PF) == 0x123456ul, "[set] file round trip");
    ss::Values B; ss::parse(S, ss::M_ES, "SPEC1|KeyLevels|1.0.0|ES\nfont=0\nwidth=99\n", B);
    CHECK(B.i(klhset::K_FONT) == 9 && B.i(klhset::K_WIDTH) == 1, "[set] out-of-range font / width -> defaults");
    CHECK(std::string(klhset::PLUGIN_VERSION) == KL_VERSION, "[ver] settings version = plugin version");
}

static void logicTests() {
    const ss::Values D = ss::defaults(klhset::spec(), ss::M_ES);
    std::vector<klh::KeyIn> k; long long at = 0;
    const std::string file = "KEYLV,1,1000000\nL|PWL/CWL|7676.000000|0\nL|Open|7700.000000|0\nL|Open/PDL|7710.250000|0\nL|HrHI|7720.000000|0\nL|VAL|7726.500000|0\nL|DPoc|7754.500000|0\nL|X|7760|1\nL|VAH|7768.500000|0\nL|PFH/PDH|7791.500000|0\nL|WPoc|7855.750000|0\nL|PWH|7887.000000|0\nL|bad|abc|0\n";
    CHECK(klh::parseKey(file, 1000000 + 3600, 80 * 3600, k, at) && at == 1000000 && k.size() == 11, "[logic] parses the real file format (11 good rows)");
    CHECK(!klh::parseKey(file, 1000000 + 81 * 3600, 80 * 3600, k, at), "[logic] older than 80 h -> refused");
    CHECK(!klh::parseKey("garbage\nL|PDH|1|0\n", 1000000, 80 * 3600, k, at), "[logic] no header -> refused");
    klh::parseKey(file, 1000000, 80 * 3600, k, at);
    std::vector<klh::Line> L = klh::build(k, {}, D);
    std::map<std::string, double> got; for (auto& q : L) got[q.name] = q.price;
    CHECK(L.size() == 8, "[logic] 8 lines: Open, HrHI and the pivot row dropped");
    CHECK(!got.count("Open") && got.count("PDL") && got["PDL"] == 7710.25, "[logic] 'Open/PDL' -> 'PDL' (no Open)");
    CHECK(got.count("PWL " + MID + " CWL") && got.count("PFH " + MID + " PDH"), "[logic] merged levels joined with a dot");
    bool sorted = true; for (size_t i = 1; i < L.size(); ++i) sorted = sorted && L[i - 1].price <= L[i].price;
    CHECK(sorted, "[logic] sorted by price");
    for (auto& q : L) if (q.name == "PFH " + MID + " PDH") CHECK(q.col == 0x008A8Aul, "[logic] merged PFH/PDH takes PF's teal");
    ss::Values V = D; V.v[klhset::K_SHOW_PF] = 0; V.v[klhset::K_SHOW_VA] = 0;
    L = klh::build(k, {}, V); got.clear(); for (auto& q : L) got[q.name] = q.price;
    CHECK(got.count("PDH") && !got.count("VAH") && !got.count("VAL") && !got.count("PFL") && got.size() == 6, "[logic] PF and VA off: 'PFH/PDH' -> 'PDH', VAH / VAL gone");
    for (auto& q : L) if (q.name == "PDH") CHECK(q.col == 0xFFA500ul, "[logic] PDH left alone takes orange");
    // MenthorQ
    std::vector<klh::MqIn> mq; klh::parseMq("SYMBOL,PRICE,LABEL,PENCOLOR\nEPZ26,7755.000000,CR 0D / GW 0D STALE 24m,48340\nEPZ26,7750.0,PS 0D / HVL 0D,1\nEPZ26,7800,G1,1\nEPZ26,7675,G2,1\nNQZ26,25000,CR 0D,1\nEPZ26,bad,X,1\n", mq);
    CHECK(mq.size() == 5, "[logic] MenthorQ rows parsed (bad price skipped)");
    CHECK(klh::mqName("CR 0D / GW 0D STALE 24m") == "CR0 STALE" && klh::mqName("PS 0D / HVL 0D") == "PS 0D " + MID + " HVL 0D" && klh::mqName("MQ G1") == "G1", "[logic] MenthorQ names: CR0, dot join, STALE kept, no 'MQ'");
    CHECK(klh::mqColour("CR 0D") == 0xEF4444ul && klh::mqColour("PS 0D") == 0x22C55Eul && klh::mqColour("HVL") == 0xFACC15ul && klh::mqColour("G3") == 0x22D3EEul, "[logic] MenthorQ colours CR red, PS green, HVL yellow, G cyan");
    L = klh::build(k, mq, D); CHECK(L.size() == 8, "[logic] MenthorQ off: none added");
    V = D; V.v[klhset::K_SHOW_MQ] = 1; L = klh::build(k, mq, V); CHECK(L.size() == 13, "[logic] MenthorQ on: every row handed in added (G2 too)");
    // badge placement
    std::vector<int> ys = { 100, 105, 108, 200 }, ws = { 80, 60, 70, 90 };
    std::vector<int> xs = klh::placeRight(ys, ws, 1000, 15, 3);
    CHECK(xs[0] + ws[0] == 1000 && xs[3] + ws[3] == 1000, "[logic] badges right-justified at the right edge");
    bool overlap = false;
    for (size_t a = 0; a < ys.size(); ++a) for (size_t b = a + 1; b < ys.size(); ++b)
        if (std::abs(ys[a] - ys[b]) < 15 && xs[a] < xs[b] + ws[b] && xs[b] < xs[a] + ws[a]) overlap = true;
    CHECK(!overlap, "[logic] badges on close rows step left, never overlap");
    for (size_t i = 0; i < xs.size(); ++i) CHECK(xs[i] + ws[i] <= 1000, "[logic] no badge past the right edge");
}

// ---- host -----------------------------------------------------------------------------------------------------------------------
static std::string g_home;
static std::string flexDir() { return g_home + "/InvestorRT/rtx/lsFlexLevels"; }
static void home(const std::string& name) {
    const std::string root = "/tmp/klh-run"; g_home = root + "/" + name;
    if (std::system(("rm -rf '" + g_home + "'").c_str()) != 0) {}
    for (const std::string& p : { root, g_home, g_home + "/InvestorRT", g_home + "/InvestorRT/rtx", flexDir() }) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", g_home.c_str(), 1);
}
static void put(const std::string& file, const std::string& text) { std::ofstream f((flexDir() + "/" + file).c_str(), std::ios::binary); f << text; }
static std::string slurp(const std::string& file) { std::ifstream f((flexDir() + "/" + file).c_str(), std::ios::binary); return std::string((std::istreambuf_iterator<char>(f)), {}); }
static std::string keyFile(long long at) {
    return "KEYLV,1," + std::to_string(at) + "\nL|PWL/CWL|7676.000000|0\nL|PFL|7704.500000|0\nL|Open/PDL|7710.250000|0\nL|VAL|7726.500000|0\nL|DPoc|7754.500000|0\nL|VAH|7768.500000|0\nL|PFH/PDH|7791.500000|0\nL|WPoc|7855.750000|0\nL|PWH|7887.000000|0\nL|Open|7745.000000|0\n";
}
static const std::string MQFILE = "SYMBOL,PRICE,LABEL,PENCOLOR\nEPZ26,7755.000000,CR 0D / GW 0D,48340\nEPZ26,7750.000000,PS 0D / HVL 0D,1\nEPZ26,7800.000000,G1,1\nNQZ26,25000.0,CR 0D,1\nEPH27,7790.0,CR 0D,1\n";

struct Chart {
    mock::Host H; KeyLevels* x = nullptr;
    Chart(const std::string& sym, const std::string& root, float base) {
        H.symbol = sym; H.root = root; H.chart = root + " 60m"; H.seconds = 3600; H.priceBase = base; H.pxPerTick = 0.5f; H.tick = 0.25f;
        H.paneTop = 0; H.paneBottom = 800; H.paneLeft = 0; H.paneRight = 1800; H.bg = 0x000000;
        for (int i = 0; i < 60; ++i) { mock::Bar b; b.time = 1792000000UL + (unsigned long)i * 3600; b.open = base + 40; b.high = base + 60; b.low = base + 20; b.close = base + 45; H.bars.push_back(b); }
        mock::current = &H; x = static_cast<KeyLevels*>(CreateExtension()); x->setup();
    }
    ~Chart() { mock::current = &H; x->destroy(); delete x; mock::current = nullptr; }
    void drawNow() { mock::current = &H; KlState* s = x->stateFor(); if (s) s->lastCheckMs = -100000; H.draws.clear(); x->draw(); }
};
struct Seen { std::map<std::string, mock::Draw> badge; std::vector<mock::Draw> lines; std::vector<mock::Draw> rects; };
static Seen look(Chart& c) {
    Seen r;
    for (const auto& d : c.H.draws) {
        if (d.type == "line") r.lines.push_back(d);
        if (d.type == "rect") r.rects.push_back(d);
        if (d.type == "ctext") r.badge[d.text] = d;
    }
    return r;
}

static void hostTests() {
    const long long now = (long long)time(nullptr);
    {   home("es-default"); put("LRA-KeyLevels-ES.csv", keyFile(now - 120)); put("MenthorQLevels.csv", MQFILE);
        Chart c("ESZ6", "ES", 7700); c.drawNow(); Seen s = look(c);
        CHECK(s.badge.size() == 9 && s.lines.size() == 9, "[host] ES 60-min default: 9 levels, 9 lines (no Open, no MenthorQ)");
        CHECK(s.badge.count("PDL  7710.25") && s.badge.count("PWL " + MID + " CWL  7676.00") && s.badge.count("PFH " + MID + " PDH  7791.50") && s.badge.count("WPoc  7855.75"), "[host] badges read 'NAME  price'");
        bool noOpen = true, noMq = true; for (auto& b : s.badge) { if (b.first.find("Open") != std::string::npos) noOpen = false; if (b.first.find("CR") != std::string::npos || b.first.find("HVL") != std::string::npos) noMq = false; }
        CHECK(noOpen && noMq, "[host] no Open, no MenthorQ by default");
        bool across = true; for (auto& l : s.lines) across = across && l.l == 0 && l.r == 1800 && l.t == l.b && l.width == 1;
        CHECK(across, "[host] every line runs across the whole chart, width 1");
        int atEdge = 0; bool noOv = true; for (size_t a = 0; a < s.rects.size(); ++a) { atEdge += s.rects[a].r == 1797; for (size_t b = a + 1; b < s.rects.size(); ++b) { const auto& A = s.rects[a]; const auto& B = s.rects[b]; if (A.l < B.r && B.l < A.r && A.t < B.b && B.t < A.b) noOv = false; } }
        CHECK(atEdge == 8 && s.rects.size() == 9 && noOv, "[host] badges right-justified at the right edge; PFL (12 px under PDL) steps left of it; none overlap");
        const mock::Draw& pdl = s.badge["PDL  7710.25"];
        CHECK(pdl.color == (COLOR)0xFFA500 && pdl.font == 9 && pdl.bold, "[host] PDL text orange, font 9 bold");
        int pdlY = -1; for (auto& l : s.lines) if (l.color == (COLOR)0xFFA500) pdlY = l.t;
        CHECK(pdlY == (int)(800 - 100 - (7710.25f - 7700) / 0.25f * 0.5f), "[host] the PDL line sits at its price");
        const mock::Draw& pwh = s.badge["PWH  7887.00"];
        CHECK(pwh.color != (COLOR)0x0000FF, "[host] pure-blue PWH text lifted so it reads on black");
        const std::string st = slurp("KeyLevels.status-ES.txt");
        CHECK(st.find("VERSION,1.0.0") != std::string::npos && st.find("KEY_LEVELS,9 levels") == std::string::npos && st.find("10 levels") != std::string::npos && st.find("MENTHORQ,off") != std::string::npos && st.find("9 to draw, 9 on screen") != std::string::npos, "[host] status file: version, 10 levels read, MenthorQ off, 9 drawn");
    }
    {   home("es-mq"); put("LRA-KeyLevels-ES.csv", keyFile(now - 60)); put("MenthorQLevels.csv", MQFILE);
        ss::Values V = ss::defaults(klhset::spec(), ss::M_ES); V.market = ss::M_ES; V.v[klhset::K_SHOW_MQ] = 1;
        put("KeyLevels.settings-ES.txt", ss::format(klhset::spec(), V));
        Chart c("ESZ6", "ES", 7700); c.drawNow(); Seen s = look(c);
        CHECK(s.badge.count("CR0  7755.00") && s.badge.count("PS 0D " + MID + " HVL 0D  7750.00") && s.badge.count("G1  7800.00"), "[host] MenthorQ ticked: CR0, PS · HVL, G1 shown");
        CHECK(!s.badge.count("CR 0D  25000.00") && !s.badge.count("CR 0D  7790.00") && s.badge.size() == 12, "[host] only the chart's contract (EPZ26 = ESZ6): NQ and the March contract not mixed in");
        CHECK(s.badge["CR0  7755.00"].color == (COLOR)dpaudit::liftForText(0xEF4444) && s.badge["G1  7800.00"].color == (COLOR)dpaudit::liftForText(0x22D3EE), "[host] CR red, G1 cyan");
        bool ok = true; for (size_t a = 0; a < s.rects.size(); ++a) for (size_t b = a + 1; b < s.rects.size(); ++b) {
            const auto& A = s.rects[a]; const auto& B = s.rects[b];
            if (A.l < B.r && B.l < A.r && A.t < B.b && B.t < A.b) ok = false; }
        CHECK(ok, "[host] DPoc 7754.5 / CR0 7755 / HVL 7750 badges do not overlap (stepped left)");
    }
    {   home("es-toggles"); put("LRA-KeyLevels-ES.csv", keyFile(now - 60));
        ss::Values V = ss::defaults(klhset::spec(), ss::M_ES); V.market = ss::M_ES; V.v[klhset::K_SHOW_PF] = 0; V.v[klhset::K_SHOW_WPOC] = 0; V.v[klhset::K_FONT] = 13; V.v[klhset::K_WIDTH] = 2; V.v[klhset::K_COL_PD] = 0x00FF00;
        put("KeyLevels.settings-ES.txt", ss::format(klhset::spec(), V));
        Chart c("ESZ6", "ES", 7700); c.drawNow(); Seen s = look(c);
        CHECK(!s.badge.count("PFL  7704.50") && !s.badge.count("WPoc  7855.75") && s.badge.count("PDH  7791.50"), "[host] PF / WPoc off: PFL and WPoc gone, 'PFH/PDH' -> PDH");
        CHECK(s.badge["PDH  7791.50"].font == 13 && s.badge["PDH  7791.50"].color == (COLOR)0x00FF00, "[host] font 13 and the PD colour from the settings file");
        bool w2 = true; for (auto& l : s.lines) w2 = w2 && l.width == 2; CHECK(w2, "[host] line width 2");
    }
    {   home("es-old"); put("LRA-KeyLevels-ES.csv", keyFile(now - 90 * 3600));
        Chart c("ESZ6", "ES", 7700); c.drawNow(); Seen s = look(c);
        CHECK(s.badge.empty() && s.lines.empty(), "[host] a file older than 80 h draws nothing");
        CHECK(slurp("KeyLevels.status-ES.txt").find("older than 80 h") != std::string::npos, "[host] ... and the status file says why");
    }
    {   home("es-missing");
        Chart c("ESZ6", "ES", 7700); c.drawNow(); Seen s = look(c);
        CHECK(s.badge.empty() && slurp("KeyLevels.status-ES.txt").find("missing - open lsDealerProfile on a 3-min ES chart") != std::string::npos, "[host] no file: nothing drawn, status says what to open");
    }
    {   home("weekend"); put("LRA-KeyLevels-ES.csv", keyFile(now - 60 * 3600));
        Chart c("ESZ6", "ES", 7700); c.drawNow(); CHECK(look(c).badge.size() == 9, "[host] a Friday file still drawn on Sunday (60 h old)");
    }
    {   home("nq-copy"); put("LRA-KeyLevels-ES.csv", keyFile(now - 60));
        put("LRA-KeyLevels-NQ.csv", "KEYLV,1," + std::to_string(now - 60) + "\nL|PDH|25010.250000|0\nL|ONL|24890.000000|0\nL|LonHI|24990.500000|0\n");
        Chart c("NQZ6", "NQ", 24800); c.drawNow(); Seen s = look(c);
        CHECK(s.badge.size() == 3 && s.badge.count("PDH  25010.25") && s.badge.count("ONL  24890.00") && s.badge.count("LonHI  24990.50"), "[host] copied to an NQ chart: NQ's own levels (no ES levels)");
        CHECK(s.badge["ONL  24890.00"].color == (COLOR)0x808080 || s.badge["ONL  24890.00"].color == (COLOR)dpaudit::liftForText(0x808080), "[host] ONL grey");
    }
    {   home("partly-off-screen"); put("LRA-KeyLevels-ES.csv", keyFile(now - 60));
        Chart c("ESZ6", "ES", 7700); c.H.paneBottom = 600; c.drawNow(); Seen s = look(c);   // y = 500 - ... : PWL (7676 -> 548) in; lower ones off
        bool inside = true; for (auto& l : s.lines) inside = inside && l.t > 0 && l.t < 600;
        CHECK(inside && !s.lines.empty() && s.lines.size() < 9 + 1, "[host] only levels inside the pane are drawn");
    }
    {   // the settings window: open (filled from the file / defaults), untick PD, set font 12, OK -> saved; the chart then follows
        home("dialog"); put("LRA-KeyLevels-ES.csv", keyFile(now - 60));
        mock::Host D; D.symbol = "ESZ6"; D.root = "ES"; D.chart = "ES 60m"; D.seconds = 3600; D.inDialog = true; mock::current = &D;
        KeyLevels* w = static_cast<KeyLevels*>(CreateExtension()); w->setup();
        const ss::Ids& I = klIds();
        w->parmsLoad();
        CHECK(D.boxes[I.id[klhset::K_SHOW_PD]] && !D.boxes[I.id[klhset::K_SHOW_MQ]] && D.ints[I.id[klhset::K_FONT]] == 9, "[dialog] opens with the defaults: PD ticked, MenthorQ unticked, font 9");
        D.boxes[I.id[klhset::K_SHOW_PD]] = false; w->parmsUpdt((unsigned)I.id[klhset::K_SHOW_PD]);
        D.ints[I.id[klhset::K_FONT]] = 12; w->parmsUpdt((unsigned)I.id[klhset::K_FONT]);
        D.boxes[I.id[klhset::K_SHOW_MQ]] = true; w->parmsUpdt((unsigned)I.id[klhset::K_SHOW_MQ]);
        w->parmsApply();
        const std::string f = slurp("KeyLevels.settings-ES.txt");
        CHECK(f.find("show_pd=0") != std::string::npos && f.find("font=12") != std::string::npos && f.find("show_mq=1") != std::string::npos, "[dialog] OK saves PD off, font 12, MenthorQ on");
        mock::current = &D; w->destroy(); delete w;
        put("MenthorQLevels.csv", MQFILE);
        Chart c("ESZ6", "ES", 7700); c.drawNow(); Seen s = look(c);
        CHECK(!s.badge.count("PDL  7710.25") && s.badge.count("PFH  7791.50") && s.badge.count("CR0  7755.00") && s.badge["PFH  7791.50"].font == 12, "[dialog] the chart follows: no PDL, 'PFH/PDH' -> PFH, CR0 shown, font 12");
        // re-open: the window shows what was saved
        mock::Host D2; D2.symbol = "ESZ6"; D2.root = "ES"; D2.chart = "ES 60m"; D2.seconds = 3600; D2.inDialog = true; mock::current = &D2;
        KeyLevels* w2 = static_cast<KeyLevels*>(CreateExtension()); w2->setup(); w2->parmsLoad();
        CHECK(!D2.boxes[I.id[klhset::K_SHOW_PD]] && D2.boxes[I.id[klhset::K_SHOW_MQ]] && D2.ints[I.id[klhset::K_FONT]] == 12, "[dialog] re-opened: PD unticked, MenthorQ ticked, font 12");
        mock::current = &D2; w2->destroy(); delete w2;
    }
}

int main() {
    setenv("TZ", "UTC", 1); tzset();
    settingsTests(); logicTests(); hostTests();
    std::cout << "lsKeyLevels 1.0.0: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
