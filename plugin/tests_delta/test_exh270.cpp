// test_exh270.cpp -- lsDeltaProfile 2.7.0: the thin-extreme exhaustion mark "Ex?" and the absorption + attacker exhaustion label "AE?"
// (Rassul 2026-10-13 "the thin extreme seems promising for exhaustion signals especially after absorbtion or trapped detla" /
//  "this looks good, lets implement this" / 10:33 "too many signals .. focus on the absorbtion + exhaustion")
#include "../DeltaProfile.cpp"
#include <iostream>
#include <functional>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) ++passed; else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static std::string flex;   // the lsFlexLevels folder of the current test
static std::string home(const std::string& name) {
    const char* r = std::getenv("DP254_RUN"); const std::string root = r && r[0] ? r : "/tmp/dp270-run";
    const std::string h = root + "/" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    flex = h + "/InvestorRT/rtx/lsFlexLevels/";
    exConfigs().clear(); exLevelCaches().clear(); latchBooks().clear();
    return flex + "DeltaProfile/";
}
static void put(const std::string& path, const std::string& text) { std::ofstream o(path.c_str(), std::ios::binary | std::ios::trunc); o << text; }
static void keyLevels(double price) {
    put(flex + "LRA-KeyLevels-ES.csv", "KEYLV,1," + std::to_string(static_cast<long long>(std::time(NULL))) + "\nL|PDH|" + std::to_string(price) + "|0\n");
}
static void params(const std::string& show, const std::string& showAe) {
    put(flex + "LRA-DeltaEx-ES.csv", "# test\nVERSION,1\nMARKET,ES\nPARAM,N,10\nPARAM,K,auto\nSHOW," + show + ",?\nSHOW_AE," + showAe + "\n");
}
static std::vector<std::vector<std::string> > tagged(const std::string& text, const char* tag) {
    std::vector<std::vector<std::string> > out; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (!c.empty() && c[0] == tag) out.push_back(c); }
    return out;
}
static std::vector<mock::Draw> texts(const mock::Host& h, const std::string& prefix) {
    std::vector<mock::Draw> out;
    for (const mock::Draw& d : h.draws) if (d.type == "text" && d.text.compare(0, prefix.size(), prefix) == 0) out.push_back(d);
    return out;
}
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }

// ---- the scenario (ES 1-min, tick 0.25): 100 neutral chop bars (102-106), then a 16-bar rally 103 -> 112 into PDH 112.25.
// Bar 106: a 3,000-lot buying print at 108 that closes RED below it (a bearish A, sellers absorbed buyers) - or GREEN above it
// (no A; a trapped-buyer node when the next 3 bars stall at it). Bars 112-113 buy hard, 114-115 barely: aggression fading. Bar 115
// makes the high 112.00 with almost no delta in its top two rows (thin). Bar 116 closes red (the print). Then quiet bars at 110.
enum Variant { A_CTX, T_CTX, THICK, AE_NOFADE };
static void addRows(mock::Bar& b, double lo, double hi, long buy, long sell) {
    for (double p = lo; p <= hi + 1e-9; p += 0.25) { b.rows.push_back(mock::Price(static_cast<float>(p), buy, sell, buy + sell)); b.volume += static_cast<unsigned long>(buy + sell); }
}
static mock::Bar mk(unsigned long t, double o, double h, double l, double c) { mock::Bar b; b.time = t; b.open = (float)o; b.high = (float)h; b.low = (float)l; b.close = (float)c; b.volume = 0; return b; }
static std::vector<mock::Bar> scenario(Variant v, int total) {
    const unsigned long day = 1791504000UL, t0 = day + 8 * 3600 + 30 * 60;
    std::vector<mock::Bar> out;
    for (int k = 0; k < total; ++k) {
        const unsigned long t = t0 + static_cast<unsigned long>(k) * 60;
        mock::Bar b;
        if (k < 100) { b = mk(t, 104, 106, 102, 104); addRows(b, 102, 106, k % 2 ? 5 : 10, k % 2 ? 10 : 5); }
        else if (k <= 114) {
            const double lo = 103 + (k - 100) * 0.5, hi = lo + 1.0;
            b = mk(t, lo + 0.25, hi, lo, hi - 0.25);
            const long buy = (k == 112 || k == 113) ? 50 : k == 114 ? 6 : 20, sell = (k == 112 || k == 113) ? 10 : k == 114 ? 2 : 10;
            addRows(b, lo, hi, buy, sell);
            if (k == 106) {
                b = v == T_CTX ? mk(t, 107.0, 108.5, 106.75, 108.25) : mk(t, 108.25, 108.5, 106.75, 107.0);
                addRows(b, 106.75, 108.5, 20, 10);
                for (mock::Price& p : b.rows) if (p.price == 108.0f) { p.buy += 3000; p.total += 3000; b.volume += 3000; }
            }
            if (v == AE_NOFADE && (k == 108 || k == 109 || k == 110))
                for (mock::Price& p : b.rows) if (p.price == 108.0f) { p.buy += 1600; p.total += 1600; b.volume += 1600; }
        } else if (k == 115) {
            b = mk(t, 111.25, 112, 111, 111.75);
            addRows(b, 111, 111.5, 6, 2);
            if (v == THICK) { addRows(b, 111.75, 112, 400, 0); }
            else { b.rows.push_back(mock::Price(111.75f, 2, 1, 3)); b.rows.push_back(mock::Price(112.0f, 1, 1, 2)); b.volume += 5; }
        } else if (k == 116) { b = mk(t, 111.5, 111.75, 110.75, 111.0); addRows(b, 110.75, 111.75, 3, 6); }
        else { b = mk(t, 110, 110.5, 109.5, 110); addRows(b, 109.5, 110.5, 4, 4); }
        out.push_back(b);
    }
    return out;
}
static std::vector<mock::Bar> mirror(const std::vector<mock::Bar>& in) {   // price -> 216 - price, buyers <-> sellers
    std::vector<mock::Bar> out = in;
    for (mock::Bar& b : out) {
        const float h = b.high, l = b.low; b.high = 216 - l; b.low = 216 - h; b.open = 216 - b.open; b.close = 216 - b.close;
        for (mock::Price& p : b.rows) { p.price = 216 - p.price; std::swap(p.buy, p.sell); }
    }
    return out;
}
static mock::Host host() { mock::Host h; h.seconds = 60; h.root = "ES"; h.symbol = "ESZ26"; h.tick = 0.25f; return h; }
static void draw(DeltaProfile& p, mock::Host& h, int n, const std::vector<mock::Bar>& all) {
    h.bars.assign(all.begin(), all.begin() + n); h.draws.clear(); mock::current = &h;
    if (h.data) { state(h).builtAt = -1; state(h).statusAt = -1; }
    p.draw();
}
static const std::string session = "20735";
static std::string record(const std::string& dir) { return dir + "ES-ESZ26-60-signals-" + session + ".csv"; }
struct Box { int l, t, r, b; };
static bool hit(const Box& a, const Box& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
static Box textBox(const mock::Draw& d) { const int w = static_cast<int>(d.text.size()) * 6, y = d.y + 4; return Box{ d.x - w, y - 6, d.x, y + 6 }; }
static Box ringBox(const mock::Draw& d) { return Box{ d.x - 6, d.y - 6, d.x + 7, d.y + 7 }; }

int main()
{
    setenv("TZ", "UTC", 1); tzset();
    check(std::string(DLT_VERSION) == "2.8.5", "version string is 2.8.5");
    // 1) a thin top after an A prints Ex? at the FIRST red close, and never moves or disappears after later bars
    mock::Draw firstMark("none", 0, 0);
    {
        const std::string dir = home("thin_after_A"); keyLevels(112.25); params("ctx", "?");
        const std::vector<mock::Bar> all = scenario(A_CTX, 130);
        mock::Host h = host(); DeltaProfile p;
        draw(p, h, 117, all);                                           // bar 115 (the extreme) just closed, 116 forming
        std::string text = slurp(record(dir));
        auto E = tagged(text, "E");
        check(E.size() == 1, "one candidate logged at the extreme's close (" + std::to_string(E.size()) + ")");
        if (E.size() == 1) {
            const auto& c = E[0];
            check(c.size() == 26, "E line has 26 fields");
            check(c[1] == "C" && c[5] == "top" && c[10] == "112" && c[12] == "PDH" && c[14] == "1", "C line: top at 112.00, PDH 1 tick away");
            check(c[17] == "1" && c[18] == "1" && std::atof(c[15].c_str()) < 0.35 && std::atof(c[16].c_str()) < 0.5, "thin and fading (thin " + c[15] + ", fade " + c[16] + ")");
            check(c[19] == "A" && c[20] == "108" && c[22] == "Ex?" && c[23] == "live", "context = the A at 108, mark Ex?, live");
            check(c[2] == DLT_VERSION && c[25].find("show=ctx") != std::string::npos && c[25].find("src=file") != std::string::npos, "version + params recorded");
        }
        check(texts(h, "Ex").empty(), "nothing drawn before the close back");
        draw(p, h, 118, all);                                           // bar 116 closed red: the print
        text = slurp(record(dir)); E = tagged(text, "E");
        check(E.size() == 2 && E[1][1] == "P" && E[1][24] == "red close" && E[1][9] == std::to_string(all[116].time), "P line at bar 116's close");
        auto m = texts(h, "Ex?");
        check(m.empty(), "2.8.1: the printed Ex? is recorded (P line) but NOT drawn");   // (2.8.4 test update: was "one red Ex? drawn")
        if (m.size() == 1) {
            firstMark = m[0];
            check(m[0].x == 50 + 115 * 10 + 9 && m[0].y + 4 < 1000 - (112 - 80) * 20, "Ex? centred on the extreme bar, above its high");
        }
        for (int n = 119; n <= 130; ++n) draw(p, h, n, all);           // later bars (one makes a lower high): nothing moves
        m = texts(h, "Ex?");
        check(m.empty(), "2.8.1: still nothing drawn after later bars");
        check(tagged(slurp(record(dir)), "E").size() == 2, "no new E lines after the decision");
        const std::string st = slurp(flex + "DeltaProfile.status.txt");
        check(st.find("EXH,0,P,top,112,Ex?,") != std::string::npos && st.find("Ex? thin top 0.02x delta; aggression 0.08x") != std::string::npos && st.find("after A at 108") != std::string::npos &&
              st.find("at PDH 112.25 (1 ticks)") != std::string::npos, "status carries the plain-words detail");
        // 7) restart in the same session: the mark reloads from the record at the same place; nothing re-logged
        mock::current = &h; p.destroy(); latchBooks().clear(); exConfigs().clear(); exLevelCaches().clear();
        DeltaProfile p2; mock::Host h2 = host(); draw(p2, h2, 130, all);
        m = texts(h2, "Ex?");
        check(m.empty(), "2.8.1 restart: the mark reloads into the book, still not drawn");
        check(tagged(slurp(record(dir)), "E").size() == 2 && latchBooks().begin()->second.exLoaded == 1, "restart: loaded, nothing re-logged");
        // 8) an A circle on the extreme bar right where the mark would sit: the mark steps up, never overlapping it
        LatchBook& bk = latchBooks().begin()->second;
        Latch L; L.low = L.high = dp::priceKey(112.5, 0.25); L.code = "A"; L.support = false; L.sell = false; L.peakTime = all[115].time;
        L.formedTime = all[115].time; L.price = 112.5; L.id = bk.nextId++; L.decidedBy = 1; L.tFirst = 1; L.tDecided = 1; L.logged = true; L.mult = 3; L.net = 100;
        bk.v.push_back(L);
        draw(p2, h2, 130, all);
        m = texts(h2, "Ex?"); bool clash = false, ringThere = false;
        for (const mock::Draw& d : h2.draws) if (d.type == "ring") { if (d.x == 50 + 115 * 10) ringThere = true; for (const mock::Draw& e : m) if (hit(textBox(e), ringBox(d))) clash = true; }
        check(ringThere && m.empty() && !clash, "2.8.1: the A circle is drawn, no Ex? beside it");
        for (const mock::Draw& d : h2.draws) if (d.type == "text" && d.text != "Ex?" && !m.empty() && hit(textBox(d), textBox(m[0]))) clash = true;
        check(!clash, "Ex? overlaps no other label");
        mock::current = &h2; p2.destroy(); latchBooks().clear();
    }
    // 2) a thick top (delta at the extreme >= thinX) prints nothing - logged, not drawn
    {
        const std::string dir = home("thick"); keyLevels(112.25); params("all", "?");
        const std::vector<mock::Bar> all = scenario(THICK, 125);
        mock::Host h = host(); DeltaProfile p; draw(p, h, 117, all); draw(p, h, 118, all);
        const auto E = tagged(slurp(record(dir)), "E");
        check(E.size() == 2 && E[0][17] == "0" && E[0][22] == "none" && E[1][1] == "P" && E[1][22] == "none", "thick top: logged with thin_ok 0, mark none");
        check(texts(h, "Ex").empty(), "thick top: nothing drawn");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    // 3) no level in reach -> no candidate at all
    {
        const std::string dir = home("no_level"); params("all", "?");
        const std::vector<mock::Bar> all = scenario(A_CTX, 125);
        mock::Host h = host(); DeltaProfile p; draw(p, h, 117, all); draw(p, h, 118, all);
        check(tagged(slurp(record(dir)), "E").empty() && texts(h, "Ex").empty(), "no levels file: nothing logged, nothing drawn");
        mock::current = &h; p.destroy(); latchBooks().clear();
        const std::string dir2 = home("far_level"); params("all", "?"); keyLevels(113.5);   // 6 ticks away (L = 4)
        mock::Host h2 = host(); DeltaProfile p2; draw(p2, h2, 117, all); draw(p2, h2, 118, all);
        check(tagged(slurp(record(dir2)), "E").empty() && texts(h2, "Ex").empty(), "a level 6 ticks away: nothing");
        mock::current = &h2; p2.destroy(); latchBooks().clear();
    }
    // 4) SHOW none (no params file) -> computed and logged, never drawn
    {
        const std::string dir = home("show_none"); keyLevels(112.25);
        const std::vector<mock::Bar> all = scenario(A_CTX, 125);
        mock::Host h = host(); DeltaProfile p; draw(p, h, 117, all); draw(p, h, 118, all);
        const auto E = tagged(slurp(record(dir)), "E");
        check(E.size() == 2 && E[0][17] == "1" && E[0][18] == "1" && E[0][22] == "none" && E[1][1] == "P" && E[0][25].find("src=default") != std::string::npos,
              "no params file: the qualifying top is logged (mark none, src=default)");
        check(texts(h, "Ex").empty(), "no params file: nothing drawn");
        check(slurp(flex + "DeltaProfile.status.txt").find("EXH_PARAMS,missing") != std::string::npos, "status says the params file is missing");
        mock::current = &h; p.destroy(); latchBooks().clear();
        const std::string dir2 = home("show_none_explicit"); keyLevels(112.25); params("none", "none");
        mock::Host h2 = host(); DeltaProfile p2; draw(p2, h2, 117, all); draw(p2, h2, 118, all);
        check(tagged(slurp(record(dir2)), "E").size() == 2 && texts(h2, "Ex").empty(), "SHOW,none: logged, not drawn");
        mock::current = &h2; p2.destroy(); latchBooks().clear();
    }
    // 9) a garbled params file -> the defaults, log only
    {
        const std::string dir = home("garbled"); keyLevels(112.25);
        put(flex + "LRA-DeltaEx-ES.csv", "VERSION,1\nMARKET,ES\nPARAM,thinX,abc\nSHOW,all,?\n");
        const std::vector<mock::Bar> all = scenario(A_CTX, 125);
        mock::Host h = host(); DeltaProfile p; draw(p, h, 117, all); draw(p, h, 118, all);
        const auto E = tagged(slurp(record(dir)), "E");
        check(E.size() == 2 && E[0][22] == "none" && E[0][25].find("src=garbled") != std::string::npos && E[0][25].find("thinX=0.35") != std::string::npos,
              "garbled file: defaults (thinX 0.35), log only");
        check(texts(h, "Ex").empty() && slurp(flex + "DeltaProfile.status.txt").find("EXH_PARAMS,garbled: PARAM thinX invalid") != std::string::npos, "garbled: nothing drawn, status says why");
        mock::current = &h; p.destroy(); latchBooks().clear();
        const std::string dir2 = home("binary"); keyLevels(112.25); put(flex + "LRA-DeltaEx-ES.csv", std::string("\x01\x02garbage\0zz", 12));
        mock::Host h2 = host(); DeltaProfile p2; draw(p2, h2, 117, all); draw(p2, h2, 118, all);
        check(tagged(slurp(record(dir2)), "E").size() == 2 && texts(h2, "Ex").empty(), "binary junk params: log only");
        mock::current = &h2; p2.destroy(); latchBooks().clear();
        const std::string dir3 = home("wrong_market"); keyLevels(112.25); put(flex + "LRA-DeltaEx-ES.csv", "VERSION,1\nMARKET,NQ\nSHOW,all,?\n");
        mock::Host h3 = host(); DeltaProfile p3; draw(p3, h3, 117, all); draw(p3, h3, 118, all);
        check(texts(h3, "Ex").empty(), "another market's params file: log only");
        mock::current = &h3; p3.destroy(); latchBooks().clear();
    }
    // 5) context T: a big buying node at 108 that price could not get 2 ticks past for 3 bars (no A)
    {
        const std::string dir = home("trapped"); keyLevels(112.25); params("ctx", "?");
        const std::vector<mock::Bar> all = scenario(T_CTX, 125);
        mock::Host h = host(); DeltaProfile p; draw(p, h, 117, all); draw(p, h, 118, all);
        const auto E = tagged(slurp(record(dir)), "E");
        check(E.size() == 2 && E[0][19] == "T" && E[0][20] == "108" && E[0][21] == std::to_string(all[106].time), "context T: trapped buyers at 108 (bar 106)");
        check(texts(h, "Ex").empty(), "2.8.1: context T + SHOW ctx: logged, Ex? not drawn");
        check(slurp(flex + "DeltaProfile.status.txt").find("after trapped buyers at 108") != std::string::npos, "status: trapped buyers in words");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    // 6) the bullish mirror: a thin bottom after a bullish A prints a green Ex? BELOW the low on the first green close
    {
        const std::string dir = home("mirror"); keyLevels(216 - 112.25); params("ctx", "?");
        const std::vector<mock::Bar> all = mirror(scenario(A_CTX, 125));
        mock::Host h = host(); DeltaProfile p; draw(p, h, 117, all); draw(p, h, 118, all);
        const auto E = tagged(slurp(record(dir)), "E");
        check(E.size() == 2 && E[0][5] == "bottom" && E[0][10] == "104" && E[0][19] == "A" && E[0][20] == "108" && E[1][1] == "P" && E[1][24] == "green close",
              "bottom: thin low 104 after the bullish A at 108, printed on the green close");
        const auto m = texts(h, "Ex?");
        check(m.empty(), "2.8.1: the bottom Ex? is logged, not drawn");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    // 7b) restart after EVERY bar writes the same record, byte for byte, as one straight pass (Ex + AE)
    {
        const std::vector<mock::Bar> all = scenario(A_CTX, 140);
        std::string files[2];
        for (int restart = 0; restart < 2; ++restart) {
            const std::string dir = home(restart ? "replay_restart" : "replay_straight"); keyLevels(112.25); params("ctx", "?");
            mock::Host h = host(); std::unique_ptr<DeltaProfile> p(new DeltaProfile());
            for (int n = 100; n <= 140; ++n) {
                if (restart) { mock::current = &h; p->destroy(); latchBooks().clear(); p.reset(new DeltaProfile()); }
                draw(*p, h, n, all);
            }
            files[restart] = slurp(record(dir));
            mock::current = &h; p->destroy(); latchBooks().clear();
        }
        check(!files[0].empty() && files[0] == files[1], "restart after every bar = one straight pass, byte for byte");
        check(tagged(files[0], "E").size() == 2 && tagged(files[0], "AE").size() == 1, "replay: one Ex candidate (C+P) and one AE decision");
    }
    // ---- AE (scope update 10:33): attacker exhaustion inside a confirmed A
    // 10) the A at 108 (bar 106, 3,010 buyers) is re-tested by bars 108 and 109 with +10 each: at bar 109's close the later half
    //     (10) is 0.003x the earlier (3,010) -> AE, decided at that close; the label becomes "AE?" and never flips back
    {
        const std::string dir = home("ae_found"); params("none", "?");
        std::vector<mock::Bar> all = scenario(A_CTX, 130);
        mock::Host h = host(); DeltaProfile p;
        draw(p, h, 108, all);                                           // A decided (bar 106 closed red below)
        check(!texts(h, "A ").empty() && texts(h, "AE").empty() && tagged(slurp(record(dir)), "AE").empty(), "before any re-test: label A, no AE line");
        draw(p, h, 110, all);                                           // bar 108 closed: only 2 bars in the node - not decidable
        check(tagged(slurp(record(dir)), "AE").empty() && texts(h, "AE").empty(), "2 bars in the node: not yet");
        draw(p, h, 111, all);                                           // bar 109 closed: the fade
        auto AE = tagged(slurp(record(dir)), "AE");
        check(AE.size() == 1 && AE[0].size() == 18 && AE[0][1] == "Y" && AE[0][6] == std::to_string(all[109].time) && AE[0][7] == std::to_string(all[106].time) &&
              AE[0][10] == "buyers" && AE[0][11] == "3" && AE[0][16] == "AE?", "AE decided at bar 109's close: Y, 3 bars, label AE?");
        auto L = texts(h, "AE? ");
        check(L.empty() && texts(h, "A ").size() == 1, "2.8.1 (\"no AE\"): AE? recorded, the profile label stays A");
        // never flips back: a strong new attack into the node later does not undo it
        all[118] = mk(all[118].time, 107.75, 108.25, 107.75, 108.0); addRows(all[118], 107.75, 108.25, 2000, 1);
        for (int n = 112; n <= 125; ++n) draw(p, h, n, all);
        AE = tagged(slurp(record(dir)), "AE");
        check(AE.size() == 1 && AE[0][16] == "AE?" && texts(h, "AE").empty() && texts(h, "A ").size() == 1, "AE decided once (record AE?), never re-logged; label A");
        // the label fits and overlaps nothing
        L = texts(h, "A "); bool clash = false;
        for (const mock::Draw& d : h.draws) {
            if (L.empty()) break;
            if (d.type == "ring" && hit(textBox(L[0]), ringBox(d))) clash = true;
            if (d.type == "text" && !(d.x == L[0].x && d.y == L[0].y) && hit(textBox(L[0]), textBox(d))) clash = true;
        }
        check(!L.empty() && !clash && L[0].x - static_cast<int>(L[0].text.size()) * 6 > 0, "2.8.1: the A label fits and overlaps no circle or label");
        check(slurp(flex + "DeltaProfile.status.txt").find("they ran out inside the absorption") != std::string::npos, "status: AE in plain words");
        // a restart reloads AE? from the record
        mock::current = &h; p.destroy(); latchBooks().clear(); exConfigs().clear();
        DeltaProfile p2; mock::Host h2 = host(); draw(p2, h2, 125, all);
        check(texts(h2, "AE").empty() && texts(h2, "A ").size() == 1 && tagged(slurp(record(dir)), "AE").size() == 1, "restart: AE reloads (label A), nothing re-logged");
        mock::current = &h2; p2.destroy(); latchBooks().clear();
    }
    // 11) an A whose attackers do NOT fade (re-tests of 1,600) stays A; decided N once aeMaxBars (20) bars have passed
    {
        const std::string dir = home("ae_nofade"); params("none", "?");
        const std::vector<mock::Bar> all = scenario(AE_NOFADE, 132);
        mock::Host h = host(); DeltaProfile p;
        for (int n = 108; n <= 127; ++n) draw(p, h, n, all);
        check(tagged(slurp(record(dir)), "AE").empty() && texts(h, "AE").empty() && !texts(h, "A ").empty(), "no fade: stays A, undecided while watched");
        for (int n = 128; n <= 132; ++n) draw(p, h, n, all);
        const auto AE = tagged(slurp(record(dir)), "AE");
        check(AE.size() == 1 && AE[0][1] == "N" && AE[0][16] == "A" && AE[0][6] == std::to_string(all[127].time), "no fade: N at bar 106 + 21, label A");
        check(texts(h, "AE").empty() && !texts(h, "A ").empty(), "no fade: label A");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    // 12) SHOW_AE none (and no params file at all) keeps the label "A" but logs the AE decision
    {
        const std::string dir = home("ae_show_none"); params("none", "none");
        const std::vector<mock::Bar> all = scenario(A_CTX, 125);
        mock::Host h = host(); DeltaProfile p; for (int n = 108; n <= 115; ++n) draw(p, h, n, all);
        auto AE = tagged(slurp(record(dir)), "AE");
        check(AE.size() == 1 && AE[0][1] == "Y" && AE[0][16] == "A" && AE[0][17].find("show_ae=none") != std::string::npos, "SHOW_AE none: Y logged, label A");
        check(texts(h, "AE").empty() && !texts(h, "A ").empty(), "SHOW_AE none: label stays A");
        mock::current = &h; p.destroy(); latchBooks().clear();
        const std::string dir2 = home("ae_no_file");
        mock::Host h2 = host(); DeltaProfile p2; for (int n = 108; n <= 115; ++n) draw(p2, h2, n, all);
        AE = tagged(slurp(record(dir2)), "AE");
        check(AE.size() == 1 && AE[0][1] == "Y" && AE[0][16] == "A" && texts(h2, "AE").empty(), "no params file: AE logged, label A");
        mock::current = &h2; p2.destroy(); latchBooks().clear();
        const std::string dir3 = home("ae_proven"); put(flex + "LRA-DeltaEx-ES.csv", "VERSION,1\nMARKET,ES\nSHOW_AE,proven\n");
        mock::Host h3 = host(); DeltaProfile p3; for (int n = 108; n <= 115; ++n) draw(p3, h3, n, all);
        check(texts(h3, "AE").empty() && texts(h3, "A ").size() == 1, "2.8.1: SHOW_AE proven still draws only A");
        mock::current = &h3; p3.destroy(); latchBooks().clear();
    }
    // 13) parser units: params grammar and E / AE line round trips; a torn E line is ignored
    {
        dp::exh::Params q; std::string why;
        check(parseExParams({ "VERSION,1", "MARKET,ES", "PARAM,N,12", "PARAM,thinX,0.3", "PARAM,K,6", "SHOW,all,proven", "SHOW_AE,?" }, "ES", q, why) &&
              q.N == 12 && q.thinX == 0.3 && q.K == 6 && q.show == 2 && q.proven && q.showAe == 1, "params parse");
        check(!parseExParams({ "VERSION,1", "MARKET,ES", "PARAM,N,1" }, "ES", q, why), "N out of range rejected");
        check(!parseExParams({ "MARKET,ES", "VERSION,1" }, "ES", q, why), "VERSION must come first");
        check(!parseExParams({ "VERSION,1", "MARKET,ES", "SHOW,some,?" }, "ES", q, why), "bad SHOW rejected");
        check(parseExParams({ "VERSION,1", "MARKET,ES", "FUTURE,x,y" }, "ES", q, why) && q.show == 0, "unknown keys ignored; no SHOW = none");
        ExMark m; m.id = 3; m.top = false; m.extRaw = 1791540000; m.extT = 1791540000; m.decRaw = 1791540060; m.decT = 1791540060; m.extreme = 104;
        m.level = "PDL"; m.levelPx = 103.75; m.levelTicks = 1; m.thin = 0.1; m.fade = 0.2; m.thinOk = m.fadeOk = true; m.ctx = 'T'; m.ctxPx = 108; m.ctxT = 1791539000;
        m.mark = "Ex?"; m.live = true; m.params = "N=10;W=4"; m.reason = "green close";
        LatchBook bk;
        seedFromText(exLine(m, 'C', 0.25, "ESZ26") + "\n" + exLine(m, 'P', 0.25, "ESZ26") + "\n" + exLine(m, 'C', 0.25, "ESZ26").substr(0, 30), "ESZ26", 0.25, bk);
        check(bk.ex.size() == 1 && bk.ex[0].status == 'P' && bk.ex[0].ctx == 'T' && bk.ex[0].W == 4 && !bk.ex[0].top && bk.rejected == 0,
              "E lines round trip; the torn last line ignored; not counted as A/I rejects");
        AeRec a; a.result = 'Y'; a.decT = 5; a.decRaw = 6; a.peakRaw = 4; a.lo = a.hi = 108; a.bars = 3; a.early = 3010; a.late = 10; a.fade = 0.0033; a.label = "AE?"; a.params = "x";
        LatchBook b2; seedFromText(aeLine(7, a, "ESZ26") + "\n", "ESZ26", 0.25, b2);
        check(b2.ae.size() == 1 && b2.ae[7].label == "AE?" && b2.ae[7].decRaw == 6, "AE line round trip");
        LatchBook b3; a.result = 'N'; seedFromText(aeLine(7, a, "ESZ26") + "\n", "ESZ26", 0.25, b3);
        check(b3.ae.empty() && b3.exRejected == 1, "an AE label without a Y result is rejected");
    }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
