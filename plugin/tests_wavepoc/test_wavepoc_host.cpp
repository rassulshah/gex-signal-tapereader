// test_wavepoc_host.cpp - lsWavePOC 1.3.4 host suite (1.0.0 origin) on the mock SDK (WavePOC_test_support/irtsdk.h).
//   (in plugin\tests_wavepoc) g++ -std=c++17 -Wall -Wextra -IWavePOC_test_support -I.. test_wavepoc_host.cpp -o th && ./th wavepoc_es1022.csv
// Covers: the settings rows (Reversal bars 3 / Show zigzag off) and the native parameter path (saved set used, refused / blank set =
// defaults); the Oct 22 ES waves through the real host (VAP read per bar, files written); the draw (pre-draw = profile behind the
// candles, post = tags / header; no pre-draw seen = everything in the post call); tags never over candles; WPOC distinct; max 5
// nodes per wave; zigzag only when ticked; N=5 from the chart's own set; two charts on one DLL keep their own state; bar-by-bar
// feeding = one pass; a restart redraws the same waves from the record; VAP missing -> pending; the version literals match.
#include "irtsdk.h"
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
#include "WavePOC.cpp"

static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)

struct Fix { std::vector<std::vector<float> > bars; std::vector<RTDATE> t; std::vector<std::vector<std::pair<float, long long> > > vap; };
static Fix load(const char* path) {
    Fix F; std::ifstream f(path); std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        const std::vector<std::string> x = wpc::splitCsv(line);
        if (x[0] == "B") { F.t.push_back((RTDATE)std::atoll(x[2].c_str())); F.bars.push_back({ (float)std::atof(x[3].c_str()), (float)std::atof(x[4].c_str()), (float)std::atof(x[5].c_str()), (float)std::atof(x[6].c_str()) }); F.vap.resize(F.t.size()); }
        else if (x[0] == "V") F.vap[(size_t)std::atoi(x[1].c_str())].push_back(std::make_pair((float)std::atof(x[2].c_str()), std::atoll(x[3].c_str())));
    }
    return F;
}
// the chart shows bars 0 .. upto (the last one forming)
static void chart(mock::Host& H, const Fix& F, int upto) {
    for (int a = 0; a < 4; ++a) H.f[a].clear();
    H.t.clear(); H.vap.clear();
    for (int i = 0; i <= upto; ++i) { for (int a = 0; a < 4; ++a) H.f[a].push_back(F.bars[(size_t)i][(size_t)a]); H.t.push_back(F.t[(size_t)i]); H.vap.push_back(F.vap[(size_t)i]); }
    H.lo = 7670; H.hi = 7730; H.first = 0; H.paneR = 10 * (upto + 1) + H.rightMargin;
}
static std::string home;
static std::string flex() { return home + "/InvestorRT/rtx/lsFlexLevels"; }
static void seedStore(const std::string& mkt, int spb, int n, long long before) {
    wpc::Store S; S.n = n; S.tickKey = 250000000LL;
    for (int k = 0; k < 20; ++k) { S.add(-1, before - 100000 + k * 1000, k % 2 ? 94 : 95); S.add(1, before - 100000 + k * 1000 + 500, 86); }
    mkdir((home + "/InvestorRT").c_str(), 0755); mkdir((home + "/InvestorRT/rtx").c_str(), 0755); mkdir(flex().c_str(), 0755); mkdir((flex() + "/WavePOC").c_str(), 0755);
    std::ofstream f(flex() + "/WavePOC/" + mkt + "-" + std::to_string(spb) + ".csv"); f << S.toText(mkt, spb);
}
static std::string slurp(const std::string& p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void mdraw(WavePOC& x) { mock::current->drawing = true; x.draw(); mock::current->drawing = false; }   // (1.3.1) marks the draw() context
static void drawBoth(WavePOC& x, mock::Host& H) { mock::current = &H; H.drawn.clear(); H.reason = REASON_PREDRAW; mdraw(x); H.reason = REASON_DRAW; mdraw(x); }
static void wipe() { std::string cmd = "rm -rf '" + home + "/InvestorRT'"; if (std::system(cmd.c_str()) != 0) std::cerr << "rm failed\n"; }

int main(int argc, char** argv) {
    const Fix F = load(argc > 1 ? argv[1] : "wavepoc_es1022.csv");
    if (F.t.size() != 80) { std::cerr << "no fixture\n"; return 1; }
    char tmpl[] = "/tmp/wavepoc_host_XXXXXX"; home = mkdtemp(tmpl); setenv("USERPROFILE", home.c_str(), 1);

    // ---- versions: the setVersion literal = WP_VERSION = the logic = the settings table
    {
        std::string src = slurp("WavePOC.cpp"); if (src.empty()) src = slurp("../WavePOC.cpp"); const size_t p = src.find("p->setVersion(\"");
        const std::string lit = p == std::string::npos ? "" : src.substr(p + 15, src.find('"', p + 15) - p - 15);
        CHECK(lit == WP_VERSION && lit == wpc::VERSION && lit == wpset::PLUGIN_VERSION && lit == "1.3.4" && std::string(wpl::VERSION) == "1.1.3", "version literals match (" << lit << ")");
    }
    // ---- one DLL object, setup once: the settings rows
    WavePOC x; mock::Host A; mock::current = &A; x.setup();
    CHECK(mock::declared.size() == 3 && mock::declared[2].type == 1 && mock::declared[2].b && mock::declared[2].name == "CoB legs" && wpset::spec().paramVersion == 2, "(1.3.0) row 3: CoB legs (box, default ON), parameter version 2");
    CHECK(mock::declared.size() == 3 && mock::declared[0].type == 2 && mock::declared[0].i == 3 && mock::declared[0].name == "Reversal bars", "row 1: Reversal bars (int, default 3)");
    CHECK(mock::declared.size() == 3 && mock::declared[1].type == 1 && !mock::declared[1].b && mock::declared[1].name == "Show zigzag", "row 2: Show zigzag (box, default off)");
    CHECK(wpIds().ok && wpIds().id[0] == 0 && wpIds().id[1] == 1, "row ids registered");
    CHECK(std::strlen(wpp::TEMPLATE) == (size_t)wpset::K_N, "one template token per row");

    // the window: a copy IRT hands no valid set is filled with the defaults; OK / changes never fault; nothing is written by the plugin
    {
        mock::Host W; W.dialog = true; W.parmsMode = 1; mock::current = &W;
        CHECK(x.parmsLoad() == RTX_OK && x.snap_.ok, "window without a valid set: filled from the table (defaults)");
        CHECK(x.parmsUpdt(0) == RTX_OK && x.parmsApply() == RTX_OK && x.lastWindowN_ == 3, "OK / change in the window: read, nothing saved by the plugin");
        mock::Host W2; W2.dialog = true; mock::current = &W2; x.snap_ = ss::Snap();
        CHECK(x.parmsLoad() == RTX_OK && !x.snap_.ok, "window with IRT's own set: IRT shows its values (not overwritten)");
        CHECK(W.userData == nullptr && W2.userData == nullptr, "the window copy keeps no chart state");
    }
    mock::current = &A;   // (1.3.0 test fix) the window hosts above are out of scope: ASan stack-use-after-scope in x.calc otherwise
    // ---- chart A: ES 3-min, Oct 22, the store seeded with 20 earlier waves (avg 23.625 / 21.5 pts as the reference day)
    seedStore("ES", 180, 3, (long long)F.t[0]);
    chart(A, F, 79); x.calc(0); drawBoth(x, A);
    WState* S = static_cast<WState*>(A.userData);
    CHECK(S && S->mkt == "ES" && S->N == 3 && !S->zz && S->parms == "native,ok", "chart A: ES, N 3, zigzag off, native parameters");
    CHECK(S && S->storeWhy == "loaded", "store file loaded (" << (S ? S->storeWhy : "") << ")");
    // (1.3.3) 14-bar extreme + (AVG or 3 ATR): the five Oct 22 waves (1.3.2: three) - + 12:12-12:36 up (ATR) and 13:06-13:24 up (14-bar high)
    CHECK(S && S->E.waves.size() == 5, "(1.3.3) the five Oct 22 waves through the host (" << (S ? S->E.waves.size() : 0) << ")");
    if (S && S->E.waves.size() == 5) {
        CHECK(wpc::pxText(S->E.waves[0].nodes[0].tick, 0.25) == "7719.00" && wpc::pxText(S->E.waves[2].nodes[0].tick, 0.25) == "7700.00", "(1.3.2, 1-tick rows) WPOCs 7719.00 / 7700.00 from IRT's VAP");
        CHECK(S->E.waves[2].nodes.size() == 3 && S->E.waves[0].nodes.size() == 5 && S->E.waves[1].qWhy == wpc::Q_ATR, "5 + 3 nodes; (1.3.3) the 12:12 up wave qualified by ATR");
    }
    // the draw: behind = fills / boxes / lines in the pre-draw; tags + header in the post-draw; tags right of the last bar
    {
        int pre = 0; mock::current = &A; A.drawn.clear(); A.reason = REASON_PREDRAW; mdraw(x); pre = (int)A.drawn.size();
        bool preOnlyBack = true; for (const mock::Prim& p : A.drawn) preOnlyBack = preOnlyBack && p.k != 'T';
        A.drawn.clear(); A.reason = REASON_DRAW; mdraw(x);
        bool tagsRight = true, header = false; int tags = 0; const int xLast = mockX(79) + 5; bool profInPost = false;
        for (const mock::Prim& p : A.drawn) {
            if (p.k == 'T') { if (p.text == std::string("Wave POC v") + WP_VERSION) header = true; else { ++tags; tagsRight = tagsRight && p.x0 > xLast; } }
            if (p.k == 'F' && p.col == 0x334155) profInPost = true;
        }
        CHECK(pre > 20 && preOnlyBack, "pre-draw: the profile, nodes and lines (no text) - behind the candles");
        CHECK(header && tagsRight, "post-draw: header + price tags, only right of the last bar (" << tags << " tags; 1.2.0: this fixture's nodes are all touched / replaced by the last bar = no tags)");
        CHECK(!profInPost, "the profile is not drawn again over the candles once IRT pre-draws");
        bool amber3 = false, amberBox = false; int whiteBoxes = 0, amberBoxes = 0, anyBox = 0;
        mock::current = &A; A.drawn.clear(); A.reason = REASON_PREDRAW; mdraw(x);
        for (const mock::Prim& p : A.drawn) { if (p.k == 'L' && p.col == 0xF5B428 && p.w == 3) amber3 = true; if (p.k == 'F' && p.col == 0xC88C14) { amberBox = true; ++amberBoxes; } if (p.k == 'F' && p.col == 0xB4BECD) ++whiteBoxes; if (p.k == 'B') ++anyBox; }
        CHECK(!amber3 && amberBox && anyBox == 0, "(1.2.0) WPOC = the bright amber row only (no 3 px line); no outline boxes anywhere");
        CHECK(amberBoxes == 6 && whiteBoxes == 13 && S->provOk && S->prov.i0 == 75 && S->prov.i1 == 78, "(1.3.3) one WPOC per wave (5 confirmed + the live provisional leg), the other 13 nodes white " << amberBoxes << "/" << whiteBoxes);
        {   // (1.2.0, Rassul 16:26 CT "remove the drawing of levels across the chart and from the wave pocs"): no line / circle / tick op at all
            int other = 0, spines = 0; for (const mock::Prim& p : A.drawn) { if (p.k == 'L' && p.x0 == p.x1 && (p.col == 0x22C55E || p.col == 0xEF4444)) { ++spines; continue; } if (p.k != 'F') ++other; }   // (1.3.3) + the spines
            std::vector<mock::Prim> pre2 = A.drawn; A.drawn.clear(); A.reason = REASON_DRAW; mdraw(x);
            const int xL2 = mockX(79) + 5; int across = 0, tg = 0; for (const mock::Prim& p : A.drawn) { if (p.k != 'F' && p.k != 'T' && !(p.k == 'L' && p.x0 == p.x1 && (p.col == 0x22C55E || p.col == 0xEF4444)) && std::min(p.x0, p.x1) < xL2) ++across; tg += p.k == 'T'; }
            int open = 0; for (const wpc::Wave& w : S->E.waves) if (!w.pending) for (const wpc::Node& nd : w.nodes) open += nd.stop == wpc::OPEN; if (S->provOk) for (const wpc::Node& nd : S->prov.nodes) open += nd.stop == wpc::OPEN;
            CHECK(other == 0 && across == 0, "(1.2.0) pre-draw = fills only (profile rows, nodes, liquidity bars) + (1.3.3) the spines; no line / circle / tick over the chart (" << other << " / " << across << ")");
            CHECK(spines == 6, "(1.3.3) one spine per drawn profile in the pre-draw (5 waves + the provisional leg: " << spines << ")");
            {   // (1.3.3, Rassul 04:52 CT) each spine (pre-draw = pre2): at its profile's start x, green for an up wave / red for a down wave, over start pivot .. end pivot
                std::vector<const wpc::Wave*> ws; for (const wpc::Wave& w : S->E.waves) if (!w.pending) ws.push_back(&w); if (S->provOk) ws.push_back(&S->prov);
                int good = 0, ups = 0, dns = 0;
                for (const wpc::Wave* w : ws) {
                    PNT p; p.set(w->i0, 0.0f, kBarCenter); const int x0 = p.h - (int)std::lround(10 * 0.45);
                    const int ya = mockY((float)(((double)std::min(w->p0, w->p1) - 0.5) * 0.25)), yb = mockY((float)(((double)std::max(w->p0, w->p1) + 0.5) * 0.25));
                    for (const mock::Prim& q : pre2) if (q.k == 'L' && q.x0 == q.x1 && std::abs(q.x0 - x0) <= 1 && (q.col == 0x22C55E || q.col == 0xEF4444)) {
                        const bool colOk = q.col == (w->dir > 0 ? 0x22C55EUL : 0xEF4444UL);
                        const bool spanOk = std::abs(std::min(q.y0, q.y1) - std::min(ya, yb)) <= 1 && std::abs(std::max(q.y0, q.y1) - std::max(ya, yb)) <= 1;
                        if (colOk && spanOk && q.w == 2) { ++good; (w->dir > 0 ? ups : dns) += 1; } }
                }
                CHECK(good == 6 && ups == 4 && dns == 2, "(1.3.3) spines: green on the 4 up profiles (3 waves + the provisional up leg), red on the 2 down waves, each over its wave's range, 2 px (" << good << ": " << ups << " up / " << dns << " down)");
            }
            CHECK(tg - 1 <= open, "(1.2.0) price tags only for OPEN nodes (" << tg - 1 << " tags, " << open << " open)");
            A.drawn = pre2; }
        bool inPane = true; for (const mock::Prim& p : A.drawn) inPane = inPane && p.x0 >= A.paneL && p.x1 <= A.paneR && std::min(p.y0, p.y1) >= A.paneT && std::max(p.y0, p.y1) <= A.paneB;
        CHECK(inPane, "everything inside the pane");
        // (1.3.2, Rassul 18:24 CT) the Key Levels column (lsDealerProfile posts LRA_COLV_ = its width): nothing drawn under it, profiles clipped
        {   const int kw = 300; cb::publishKeys(A.sym, A.spb, A.chartLabel, wpMs(), kw);
            const int edge = A.paneL + 1 + kw + 2; int under = 0, cut = 0, fills = 0;
            A.drawn.clear(); A.reason = REASON_PREDRAW; mdraw(x); A.reason = REASON_DRAW; mdraw(x);
            int spinesIn = 0; for (const mock::Prim& p : A.drawn) { if (std::min(p.x0, p.x1) < edge) ++under; if (p.k == 'F') { ++fills; cut += p.x0 == edge; } if (p.k == 'L' && p.x0 == p.x1 && (p.col == 0x22C55E || p.col == 0xEF4444)) ++spinesIn; }
            CHECK(spinesIn < 6, "(1.3.3) the spines of profiles starting under the Key Levels column are clipped away with them (" << spinesIn << " of 6 left)");
            CHECK(under == 0 && fills > 0, "(1.3.2) nothing drawn left of the Key Levels column's right edge (" << under << " under, " << cut << " rows clipped at it)");
            cb::clearKeys(A.sym, A.spb, A.chartLabel);
            A.drawn.clear(); A.reason = REASON_PREDRAW; mdraw(x); int back = 0; for (const mock::Prim& p : A.drawn) back += std::min(p.x0, p.x1) < edge;
            CHECK(back > 0, "the column gone: the profiles draw at the pane edge again (" << back << ")");
        }
        // the right-side columns (Delta 80 px posting on this chart): nothing right of the price area
        {   cb::publish(cb::DELTA, A.sym, A.spb, A.chartLabel, wpMs(), 80);
            A.drawn.clear(); A.reason = REASON_PREDRAW; mdraw(x); A.reason = REASON_DRAW; mdraw(x); A.reason = REASON_PREDRAW; mdraw(x); A.reason = REASON_DRAW; mdraw(x);
            const int pr = S->clipR; int over = 0; for (const mock::Prim& p : A.drawn) over += std::max(p.x0, p.x1) > pr;
            CHECK(pr < A.paneR && over == 0, "(1.3.2) nothing drawn over the right-side columns (price area ends at " << pr << ")");
            cb::clear(cb::DELTA, A.sym, A.spb, A.chartLabel);
        }
    }
    // files: record, store, status
    {
        const std::string rec = slurp(flex() + "/WavePOC/ES-180-nodes.csv"), st = slurp(flex() + "/WavePOC.status-ES-180.txt"), sto = slurp(flex() + "/WavePOC/ES-180.csv");
        size_t nw = 0; for (size_t p = rec.find("\nWAVE,"); p != std::string::npos; p = rec.find("\nWAVE,", p + 1)) ++nw;
        CHECK(nw == 5 && rec.find("NODE,") != std::string::npos && rec.find("STOP,") != std::string::npos && rec.find("\nROWS,") != std::string::npos, "(1.3.3) record: 5 waves with nodes, stops and (1.3.2) rows");
        CHECK(st.rfind("VERSION,1.3.4", 0) == 0 && st.find("qualified up 3 down 2") != std::string::npos && st.find("QUALIFIED_BY,AVG 0,ATR 1,BOTH 4") != std::string::npos && st.find("PARMS,native,ok,reversal bars 3") != std::string::npos, "status file: version, N, qualified waves");
        CHECK(st.find("touch 13,new wave 3,origin close 0") != std::string::npos, "status: stops by reason (" << st.substr(st.find("STOPS,"), 50) << ")");
        wpc::Store chk; std::string why; CHECK(wpc::storeFromText(sto, "ES", 180, 3, 250000000LL, chk, why) && chk.dn.size() == 25 && sto.rfind("WAVEPOC_STORE,2,", 0) == 0, "(1.3.3) store (version 2) saved atomically with the day's 5 down candidates (14-bar lows) (" << why << ", " << chk.dn.size() << ")");
    }
    // no pre-draw call: everything in the post call
    {
        mock::Host P = A; P.userData = nullptr; mock::current = &P; x.calc(0); P.drawn.clear(); P.reason = REASON_DRAW; mdraw(x);
        bool prof = false; for (const mock::Prim& p : P.drawn) prof = prof || (p.k == 'F' && p.col == 0x334155);
        CHECK(prof, "IRT never pre-draws: the profile is drawn in the post call (never missing)");
        x.clear();
    }
    // zigzag: off by default, on from the chart's own set
    {
        int zz = 0; mock::current = &A; drawBoth(x, A); for (const mock::Prim& p : A.drawn) zz += p.k == 'L' && (p.col == 0x78849A || p.col == 0xCBD5E1);
        A.zigzag = true; x.calc(0); drawBoth(x, A); int zz2 = 0; for (const mock::Prim& p : A.drawn) zz2 += p.k == 'L' && (p.col == 0x78849A || p.col == 0xCBD5E1);
        CHECK(zz == 0 && zz2 > 5, "zigzag only when 'Show zigzag' is ticked (" << zz << " / " << zz2 << ")");
        A.zigzag = false;
    }
    // refused / blank sets = the defaults
    {
        mock::Host R = A; R.userData = nullptr; R.parmsMode = 1; mock::current = &R; x.calc(0); WState* s = static_cast<WState*>(R.userData);
        CHECK(s && s->N == 3 && !s->zz && s->parms == "defaults,refused", "refused set -> defaults (N 3, zigzag off)");
        x.clear();
        mock::Host Z = A; Z.userData = nullptr; Z.parmsMode = 2; mock::current = &Z; x.calc(0); s = static_cast<WState*>(Z.userData);
        CHECK(s && s->N == 3 && s->parms.rfind("defaults,", 0) == 0, "blank set -> defaults");
        x.clear();
    }
    // N = 5 from this chart's set: other pivots, confirmation 5 bars after the extreme; chart A unchanged (own state)
    {
        mock::Host B5 = A; B5.userData = nullptr; B5.revBars = 5; mock::current = &B5; x.calc(0); WState* s5 = static_cast<WState*>(B5.userData);
        CHECK(s5 && s5->N == 5 && s5->E.N == 5, "N 5 read from the chart's own parameters");
        bool c5 = s5 != nullptr; if (s5) for (const wpc::Wave& w : s5->E.waves) { const int c = w.conf; long long ph = LLONG_MIN, pl = LLONG_MAX;
            for (int j = c - 5; j < c; ++j) { ph = std::max(ph, s5->E.B[(size_t)j].h); pl = std::min(pl, s5->E.B[(size_t)j].l); }
            c5 = c5 && c > w.i1 && (w.dir < 0 ? s5->E.B[(size_t)c].h > ph : s5->E.B[(size_t)c].l < pl); }
        CHECK(c5 && s5, "(1.3.2) N 5: each leg confirmed by the first bar beyond the previous 5 bars (IRT ZIGZAG bars)");
        mock::current = &A; x.calc(0);
        CHECK(static_cast<WState*>(A.userData)->N == 3, "chart A keeps N 3 (per-chart state)");
        mock::current = &B5; x.clear();
    }
    // a second chart (NQ) on the same DLL: own market, own files
    {
        mock::Host Q = A; Q.userData = nullptr; Q.root = "ENQ"; Q.sym = "ENQZ26"; mock::current = &Q; x.calc(0); WState* q = static_cast<WState*>(Q.userData);
        CHECK(q && q != S && q->mkt == "NQ" && q->storePath.find("NQ-180.csv") != std::string::npos && !q->E.waves.empty() && [&] { for (const wpc::Wave& w : q->E.waves) if (!(w.qWhy & wpc::Q_ATR)) return false; return true; }() && q->E.waves[0].nHist < wpc::MIN_HIST,
              "NQ chart: own state, own store; (1.3.3) no history yet -> only the 3 ATR rule can qualify (" << (q ? q->E.waves.size() : 0) << " waves)");
        x.clear();
    }
    // bar by bar (a new closed bar each calc) = one pass; a restart (new host, same files) redraws the same waves from the record
    {
        wipe(); seedStore("ES", 180, 3, (long long)F.t[0]);
        mock::Host I; mock::current = &I;
        for (int k = 1; k <= 79; ++k) { chart(I, F, k); x.calc(0); if (k % 7 == 0) drawBoth(x, I); }
        WState* si = static_cast<WState*>(I.userData);
        mock::Host O; mock::current = &O; chart(O, F, 79); x.calc(0); WState* so = static_cast<WState*>(O.userData);
        bool same = si && so && si->E.waves.size() == so->E.waves.size();
        for (size_t w = 0; same && w < si->E.waves.size(); ++w) for (size_t n = 0; same && n < si->E.waves[w].nodes.size(); ++n)
            same = si->E.waves[w].nodes[n].tick == so->E.waves[w].nodes[n].tick && si->E.waves[w].nodes[n].stop == so->E.waves[w].nodes[n].stop && si->E.waves[w].nodes[n].stopBar == so->E.waves[w].nodes[n].stopBar;
        CHECK(same && si->E.waves.size() == 5, "bar-by-bar feeding = one straight pass (no repaint)");
        CHECK(so && so->E.C.recordUsed == 5, "restart: the recorded waves are redrawn from the record");
        const std::string rec = slurp(flex() + "/WavePOC/ES-180-nodes.csv"); size_t nw = 0; for (size_t p = rec.find("\nWAVE,"); p != std::string::npos; p = rec.find("\nWAVE,", p + 1)) ++nw;
        CHECK(nw == 5, "a restart never writes a wave twice (" << nw << ")");
        mock::current = &I; x.clear(); mock::current = &O; x.clear();
    }
    // a garbled store = rebuilt from the chart (nothing qualifies without 5 earlier waves)
    {
        { std::ofstream f(flex() + "/WavePOC/ES-180.csv"); f << "garbage\n"; }
        mock::Host G; mock::current = &G; chart(G, F, 79); x.calc(0); WState* g = static_cast<WState*>(G.userData);
        CHECK(g && g->storeWhy.find("rebuilt from the chart") != std::string::npos, "garbled store -> rebuilt (" << (g ? g->storeWhy : "") << ")");
        x.clear();
    }
    // VAP missing for a bar of the wave: pending (not drawn, status says so)
    {
        wipe(); seedStore("ES", 180, 3, (long long)F.t[0]);
        mock::Host V; V.vapMissing = { 10 }; mock::current = &V; chart(V, F, 79); x.calc(0); WState* sv = static_cast<WState*>(V.userData);
        CHECK(sv && sv->E.pendingCount() == 1 && sv->E.waves.size() == 5 && sv->E.waves[0].pending, "VAP missing -> the wave waits (pending), retried every 10 min");
        drawBoth(x, V); int amber = 0; for (const mock::Prim& p : V.drawn) amber += p.k == 'F' && p.col == 0xC88C14;
        CHECK(amber == 5, "(1.3.3) only the 4 complete waves (+ the provisional one) are drawn;  (+ the provisional one) is drawn " << amber);
        x.clear();
    }
    // symbol change on the same chart: the state follows the new market
    {
        mock::Host C = A; C.userData = nullptr; mock::current = &C; x.calc(0); C.root = "QGC"; C.sym = "QGCZ26"; C.tick = 0.1; x.calc(0);
        WState* sc = static_cast<WState*>(C.userData);
        CHECK(sc && sc->mkt == "GC" && std::fabs(sc->E.tick - 0.1) < 1e-6 && [&] { for (const wpc::Wave& w : sc->E.waves) if (!(w.qWhy & wpc::Q_ATR)) return false; return true; }(), "symbol change -> GC, its own tick and store ((1.3.3) no GC history: only ATR-qualified waves) " << (sc ? sc->mkt : "") << " " << (sc ? sc->E.tick : 0) << " " << (sc ? sc->E.waves.size() : 9));
        x.clear();
    }
    // (1.1.0) liquidity from the live order-by-order book: big resting bids inside wave 2, a market maker that keeps re-pricing
    {
        wipe(); seedStore("ES", 180, 3, (long long)F.t[0]);
        const int alive0 = mock::dboAlive; mock::Host M; mock::current = &M; int oid = 1;
        auto order = [&](int act, int side, float px, unsigned long sz, int id) { DEPTH_ORDER o; std::memset(&o, 0, sizeof o); o.action = (unsigned char)act; o.buySell = (unsigned char)side; o.price = px; o.size = sz; o.orderID = id; M.mbo.push_back(o); };
        // a thin book every tick 7670 .. 7730 (5 lots a side), + 400 lots bid at 7685.25 and 250 at 7680.25 (real orders)
        for (int k = 0; k <= 240; ++k) { const float px = 7670.0f + 0.25f * (float)k; order(0, px < 7700 ? 1 : 0, px, 5, oid++); }
        order(0, 1, 7685.25f, 400, oid++); order(0, 1, 7680.25f, 250, oid++);
        const int mmId = oid++; order(0, 1, 7695.25f, 900, mmId);   // a market maker: 900 lots, re-priced every bar
        for (int k = 1; k <= 79; ++k) {
            chart(M, F, k);
            order(1, 1, (k % 2) ? 7695.75f : 7695.25f, 900, mmId);
            WState* sm = static_cast<WState*>(M.userData); if (sm) sm->samp.lastMs = -1;   // one sample per bar in the test (IRT: once a second)
            x.calc(0);
        }
        WState* sm = static_cast<WState*>(M.userData);
        CHECK(sm && sm->dbo && mock::dboAlive == alive0 + 1 && sm->dboEvents > 240 && sm->book.book.size() == 244, "the order-by-order feed is open, every event in the book (" << (sm ? sm->book.book.size() : 0) << ")");
        CHECK(sm && sm->samp.bars.size() >= 78, "a sample in every bar");
        bool ok = sm && sm->E.waves.size() == 5;
        if (ok) {
            const wpc::Wave& w2 = sm->E.waves[2];   // (1.3.3) the 12:36 sell wave (index 2: the 12:12 up wave now qualifies)
            CHECK(w2.liqOk && !w2.lnodes.empty() && wpc::pxText(w2.lnodes[0].tick, 0.25) == "7685.25" && w2.lnodes[0].side == 1, "LPOC of wave 2 = the 400-lot bid at 7685.25 (" << (w2.lnodes.empty() ? "none" : wpc::pxText(w2.lnodes[0].tick, 0.25)) << ")");
            bool mm = false; for (const wpc::LNode& n : w2.lnodes) mm = mm || n.tick == 30781 || n.tick == 30783;
            CHECK(!mm, "the re-pricing 900-lot market maker is left out");
        } else CHECK(false, "(1.3.3) five waves on the live-book chart");
        drawBoth(x, M); int lp = 0, tags = 0; for (const mock::Prim& p : M.drawn) { lp += p.k == 'F' && p.col == 0x22C55E; tags += p.k == 'T'; }
        CHECK(lp >= 1, "LPOC drawn in bright green (bids)");
        {   // (1.3.2, Rassul 21:20 "no liquidity nodes in the pictures") live book -> sampler -> wave bar span -> pickRaw -> LNODE bars LEFT of the profile, <= 40 px
            const unsigned long dim = wpc::mix(0x22C55E, 0x0B0F17, 0.6); int ln = 0, leftOk = 0;
            for (const mock::Prim& g : M.drawn) if (g.k == 'F' && (g.col == 0x22C55E || g.col == dim)) { ++ln;
                for (const mock::Prim& q : M.drawn) if (q.k == 'F' && q.col == 0x334155 && q.x0 >= g.x1 + 1 && q.x0 - g.x1 <= 3 && g.x1 - g.x0 <= 40) { ++leftOk; break; } }
            CHECK(ln >= 1 && ln == leftOk && sm && sm->E.C.lnodes >= 1, "(1.3.2) live-book liquidity nodes drawn left of their profile's start, <= 40 px (" << ln << " drawn, " << leftOk << " left)");
        }
        const std::string rec = slurp(flex() + "/WavePOC/ES-180-nodes.csv"), st = slurp(flex() + "/WavePOC.status-ES-180.txt");
        CHECK(rec.find("\nLNODE,") != std::string::npos && (rec.find("near_volume_node") != std::string::npos || rec.find(",alone") != std::string::npos), "record: LNODE lines with near / alone");
        CHECK(st.find("MBO,open,events") != std::string::npos && st.find("LIQUIDITY_NODES,waves with liquidity") != std::string::npos, "status: MBO + LIQUIDITY_NODES lines");
        x.clear();
        CHECK(mock::dboAlive == alive0, "chart removed: the feed object is destroyed (unsubscribed)");
    }

    // ---- (1.3.4, live ES 08:40 CT 2026-10-29: "waves without book 21", no liquidity node anywhere) the LIVE path: IRT (re)starts with the
    // history already on the chart (bars 0 .. L at once), the book opens then and is sampled only from there; the forming bar's stamp
    // moves while it forms (interim stamps) and is final when the next bar appears; calc comes several times per bar.
    for (int mode = 0; mode < 3; ++mode) {
        wipe(); seedStore("ES", 180, 3, (long long)F.t[0]);
        mock::Host M; mock::current = &M; int oid = 1; const int L = 40;
        auto order = [&](int act, int side, float px, unsigned long sz, int id) { DEPTH_ORDER o; std::memset(&o, 0, sizeof o); o.action = (unsigned char)act; o.buySell = (unsigned char)side; o.price = px; o.size = sz; o.orderID = id; M.mbo.push_back(o); };
        for (int k = 0; k <= 240; ++k) { const float px = 7670.0f + 0.25f * (float)k; order(0, px < 7700 ? 1 : 0, px, 5, oid++); }
        order(0, 1, 7685.25f, 400, oid++); order(0, 1, 7680.25f, 250, oid++); order(0, 0, 7712.75f, 300, oid++);
        chart(M, F, L); x.calc(0);                                          // load: 40 bars of history, no book for them
        WState* sm = static_cast<WState*>(M.userData);
        CHECK(sm && sm->dbo && sm->samp.samples == 1 && sm->E.C.liqWaves == 0, "(1.3.4) load: the book opens, one sample (the forming bar), no wave of the history has a book");
        int provSeen = 0, provLiq = 0, provDrawn = 0;
        for (int k = L + 1; k <= 79; ++k) {
            chart(M, F, k);
            const RTDATE fin = F.t[(size_t)k];
            // mode 0: the stamp moves (last trade) then is final; 1: the final stamp from the start; 2: the first interim = the previous bar's stamp
            const RTDATE interim[3] = { mode == 1 ? fin : (RTDATE)(F.t[(size_t)k - 1] + (mode == 2 ? 0 : 7)), mode == 1 ? fin : (RTDATE)(F.t[(size_t)k - 1] + 95), fin };
            for (int c = 0; c < 3; ++c) {
                M.t.back() = interim[c];
                if (sm) sm->samp.lastMs = -1;                                // a sample on every calc (IRT: once a second)
                x.calc(0);
            }
            sm = static_cast<WState*>(M.userData);
            if (sm && sm->provOk) {
                std::vector<long long> ts; for (int j = sm->prov.i0; j <= sm->prov.i1; ++j) ts.push_back(sm->E.B[(size_t)j].t);
                size_t cov = 0; long long n = 0; sm->samp.coverage(ts, 180, cov, n);
                if (wpl::Sampler::enough(ts.size(), cov, n)) { ++provSeen; provLiq += sm->prov.liqOk && !sm->prov.lnodes.empty(); drawBoth(x, M); int g = 0; for (const mock::Prim& q : M.drawn) g += q.k == 'F' && (q.col == 0x22C55E || q.col == 0xEF4444 || q.col == wpc::mix(0x22C55E, 0x0B0F17, 0.6) || q.col == wpc::mix(0xEF4444, 0x0B0F17, 0.6)); provDrawn += g > 0; }
            }
        }
        sm = static_cast<WState*>(M.userData);
        {   // every closed bar 40 .. 78 has its samples under its own final stamp (bar 79 still forming)
            int good = 0, stray = 0; if (sm) for (const auto& kv : sm->samp.bars) { if (kv.first > (long long)F.t[78]) continue; bool bar = false; for (int j = L; j <= 78; ++j) bar = bar || kv.first == (long long)F.t[(size_t)j]; if (bar && kv.second.samples >= (kv.first == (long long)F.t[L] ? 1 : 3)) ++good; else ++stray; }
            CHECK(good == 78 - L + 1 && stray == 0, "(1.3.4) the forming bar's interim stamps are re-keyed: every closed bar's samples under its final stamp (" << good << " good, " << stray << " stray, mode " << mode << ")");
        }
        int after = 0, afterLiq = 0, beforeLiq = 0;
        if (sm) for (const wpc::Wave& w : sm->E.waves) {
            if (w.fromRecord) continue;
            int sampled = 0; for (int j = w.i0; j <= w.i1; ++j) sampled += j >= L;   // bar L (forming at the load) has samples
            if (sampled >= 2) { ++after; afterLiq += w.liqOk && !w.lnodes.empty(); }
            if (w.i1 < L) beforeLiq += !w.lnodes.empty();
        }
        int partial = 0; if (sm) for (const wpc::Wave& w : sm->E.waves) partial += w.i0 < L && w.i1 >= L + 1 && !w.lnodes.empty();
        CHECK(partial >= 1, "(1.3.4 the live bug) a wave that began BEFORE the restart and ended after it has liquidity nodes (" << partial << ", mode " << mode << ")");
        CHECK(after >= 1 && afterLiq == after, "(1.3.4) every wave confirmed while running with >= 2 sampled bars has liquidity nodes (" << afterLiq << " of " << after << ", mode " << mode << ")");
        CHECK(beforeLiq == 0, "(1.3.4) waves wholly before the load stay without liquidity (no book then)");
        CHECK(provSeen >= 1 && provLiq == provSeen && provDrawn == provSeen, "(1.3.4) the live (provisional) wave shows its liquidity nodes once its bars have samples (" << provLiq << " / " << provDrawn << " of " << provSeen << ", mode " << mode << ")");
        // never repainted: the recorded LNODEs of a confirmed wave do not change with later samples / a replay
        std::string rec = slurp(flex() + "/WavePOC/ES-180-nodes.csv");
        CHECK(rec.find("\nLNODE,") != std::string::npos, "(1.3.4) LNODE lines recorded for the live waves");
        std::vector<std::pair<long long, long long> > before; if (sm) for (const wpc::Wave& w : sm->E.waves) for (const wpc::LNode& n : w.lnodes) before.push_back(std::make_pair(w.t0, n.tick));
        { M.t.front() = M.t.front() - 1; if (sm) sm->samp.lastMs = -1; x.calc(0); M.t.front() = F.t[0]; x.calc(0); }   // history changed -> replay from the record
        std::vector<std::pair<long long, long long> > again; sm = static_cast<WState*>(M.userData); if (sm) for (const wpc::Wave& w : sm->E.waves) for (const wpc::LNode& n : w.lnodes) again.push_back(std::make_pair(w.t0, n.tick));
        CHECK(!before.empty() && before == again, "(1.3.4) a replay redraws the same liquidity nodes (from the record, never repainted)");
        if (sm) sm->statusMs = -1;
        x.calc(0);
        const std::string st = slurp(flex() + "/WavePOC.status-ES-180.txt");
        CHECK(st.find("LIQ_SAMPLING,since ") != std::string::npos && st.find("LIQ_WAVE,confirmed,") != std::string::npos && st.find(",bars with samples ") != std::string::npos && st.find(" liquidity nodes\n") != std::string::npos, "(1.3.4) status: LIQ_SAMPLING + per-wave LIQ_WAVE lines");
        if (mode == 0) std::cout << st.substr(st.find("LIQUIDITY_NODES")) ;
        x.clear();
    }

    // ---- (1.3.0, Rassul approved 17:27 CT) CoB legs: the RTL cob_bull_3m_1b / cob_bear_3m_1b (the Cob marks' source) through the host
    {
        // a saved set of parameter version 1 (Reversal bars 5, zigzag on): parmsInit keeps both, CoB legs on; version 2 = untouched
        mock::Host P1; P1.revBars = 5; P1.zigzag = true; P1.cobLegs = false; mock::current = &P1;
        P1.inParmsInit = true; const int pi1 = x.parmsInit(1); P1.inParmsInit = false;
        CHECK(pi1 == RTX_OK && P1.savedTmpl == "IBB" && P1.saved.size() == 3 && P1.saved[0] == 3 && P1.saved[1] == 0 && P1.saved[2] == 1, "(1.3.1) parmsInit(1): the declared defaults saved (N 3, zigzag off, CoB legs on)");
        CHECK(P1.parmsReadInInit == 0, "(1.3.1) parmsInit never calls getParameters (" << P1.parmsReadInInit << ")");
        P1.saved.clear(); CHECK(x.parmsInit(2) == RTX_OK && P1.saved.empty(), "parmsInit(2): his set is left alone");
        mock::Host Bad = P1; Bad.parmsMode = 3; Bad.userData = nullptr; mock::current = &Bad; chart(Bad, F, 79); x.calc(0); WState* sb = static_cast<WState*>(Bad.userData);
        CHECK(sb && sb->parms.find("CoB legs box not 0/1") != std::string::npos && sb->cobLegs, "a garbled CoB box -> the defaults (CoB legs on)");
        x.clear();

        wipe(); seedStore("ES", 180, 3, (long long)F.t[0]);
        auto flags = [&](mock::Host& H, int upto) { H.cust["cob_bull_3m_1b"].assign((size_t)upto + 1, 0.0f); H.cust["cob_bear_3m_1b"].assign((size_t)upto + 1, 0.0f); if (upto >= 50) H.cust["cob_bear_3m_1b"][50] = 1.0f; };
        // bar by bar: (1.3.3) the down leg 13:24-13:42 (bars 47 -> 53, NOT a qualifying wave: no 14-bar low) holds a bear CoB on bar 50
        // (1.3.2 used the up leg 41 -> 47 + a bull CoB on bar 43 - in 1.3.3 that leg is a qualifying wave itself)
        mock::Host K; mock::current = &K; int beforeConf = -1;
        for (int k = 1; k <= 79; ++k) {
            chart(K, F, k); flags(K, k); x.calc(0); if (k % 5 == 0) drawBoth(x, K);
            WState* s = static_cast<WState*>(K.userData);
            if (s && beforeConf < 0) for (const wpc::Wave& w : s->E.waves) if (w.kind == wpc::K_COB) beforeConf = s->E.lastBar();
        }
        WState* sk = static_cast<WState*>(K.userData);
        CHECK(sk && sk->E.waves.size() == 6 && sk->E.waves[4].kind == wpc::K_COB && sk->E.waves[4].i0 == 47 && sk->E.waves[4].i1 == 53, "CoB leg 47 -> 53 (not a wave) gets a profile through the host (" << (sk ? sk->E.waves.size() : 0) << " legs)");
        CHECK(beforeConf == 55, "drawn only once its end pivot is confirmed (bar 55 = IRT's reversal bar after the 13:42 low), not before (" << beforeConf << ")");
        if (sk && sk->E.waves.size() == 6) CHECK(!sk->E.waves[4].nodes.empty() && sk->E.waves[0].kind == wpc::K_WAVE && sk->E.waves[2].kind == wpc::K_WAVE, "its own nodes; the two sell waves stay kind WAVE");
        {   // drawn at the leg START (bar 47), its WPOC amber
            mock::current = &K; K.drawn.clear(); K.reason = REASON_PREDRAW; mdraw(x);
            PNT p; p.set(47, 0.0f, kBarCenter); const int x0 = p.h - (int)std::lround(10 * 0.45); int at = 0, amb = 0;
            for (const mock::Prim& q : K.drawn) if (q.k == 'F' && std::abs(q.x0 - x0) <= 1) { ++at; amb += q.col == 0xC88C14; }
            CHECK(at > 5 && amb == 1, "the CoB leg's profile is drawn at its start (bar 47) with one amber WPOC (" << at << " rows, " << amb << ")");
        }
        sk->statusMs = -1; mock::current = &K; x.calc(0);   // the status is written at most once a second: force the latest one
        const std::string rec = slurp(flex() + "/WavePOC/ES-180-nodes.csv"), st = slurp(flex() + "/WavePOC.status-ES-180.txt");
        CHECK(rec.find(",COB," + std::to_string((long long)F.t[50]) + ",-,") != std::string::npos && rec.find("\nNODE,") != std::string::npos, "record: the WAVE line has kind COB + the CoB bar time");
        CHECK(rec.find(",WAVE,0,BOTH,") != std::string::npos && rec.find(",WAVE,0,ATR,") != std::string::npos && rec.find("NODE,") != std::string::npos && rec.find(",COB\n") != std::string::npos, "record: waves = kind WAVE, the CoB leg's NODE / STOP lines = COB");
        CHECK(st.find("COB_LEGS,on,CoB bars bull 0 bear 1,legs drawn COB only 1") != std::string::npos && st.find("COB_SOURCE,RTL cob_bull_3m_1b ok, cob_bear_3m_1b ok") != std::string::npos && st.find("CoB legs on") != std::string::npos, "status: COB_LEGS + COB_SOURCE + SETTINGS lines");
        // restart (same files, no RTL any more): the recorded CoB leg comes back as recorded, never written twice
        mock::current = &K; x.clear();
        mock::Host R2; mock::current = &R2; chart(R2, F, 79); x.calc(0); WState* sr = static_cast<WState*>(R2.userData);
        CHECK(sr && sr->E.waves.size() == 6 && sr->E.waves[4].kind == wpc::K_COB && sr->E.waves[4].fromRecord && sr->cobSrc.find("NOT FOUND") != std::string::npos, "restart without the RTL: the CoB leg is redrawn from the record; status says NOT FOUND");
        const std::string rec2 = slurp(flex() + "/WavePOC/ES-180-nodes.csv"); size_t nw = 0; for (size_t p = rec2.find("\nWAVE,"); p != std::string::npos; p = rec2.find("\nWAVE,", p + 1)) ++nw;
        CHECK(nw == 6, "(1.3.3) 6 WAVE lines (5 waves + 1 CoB leg), none written twice (" << nw << ")");
        x.clear();
        // the setting off: no CoB leg (recorded COB legs not drawn either); the waves unchanged
        mock::Host Off; Off.cobLegs = false; mock::current = &Off; chart(Off, F, 79); flags(Off, 79); x.calc(0); WState* so = static_cast<WState*>(Off.userData);
        CHECK(so && !so->cobLegs && so->E.waves.size() == 5 && so->cobSrc == "CoB legs off (not read)", "CoB legs unticked: only the 5 waves");
        // the CoB on a down leg's bar with only the bull flag: not a CoB leg (direction must match)
        x.clear(); wipe(); seedStore("ES", 180, 3, (long long)F.t[0]);
        mock::Host Dn; mock::current = &Dn; chart(Dn, F, 79); Dn.cust["cob_bull_3m_1b"].assign(80, 0.0f); Dn.cust["cob_bull_3m_1b"][58] = 1.0f; x.calc(0);
        WState* sd = static_cast<WState*>(Dn.userData);
        CHECK(sd && sd->E.waves.size() == 5, "a bull CoB inside a down leg (13:48-14:03) draws nothing");
        // an RTL shorter than the chart (shifted to the right end, as lsCobImpulse) - the CoB lands on the same bar
        x.clear(); wipe(); seedStore("ES", 180, 3, (long long)F.t[0]);
        mock::Host Sh; mock::current = &Sh; chart(Sh, F, 79); Sh.cust["cob_bear_3m_1b"].assign(60, 0.0f); Sh.cust["cob_bear_3m_1b"][50 - 20] = 1.0f; x.calc(0);
        WState* ss2 = static_cast<WState*>(Sh.userData);
        CHECK(ss2 && ss2->E.waves.size() == 6 && ss2->E.waves[4].cobT == (long long)ss2->E.B[50].t, "an RTL array shorter than the chart is aligned to its last bar");
        x.clear();
    }
    // ---- (1.3.1) THE CRASH: IRT died when 1.3.0 was added to a chart. Adding = the first draw() comes before any calc; 1.3.0 pumped from
    //      draw(), fed every bar and called getCustomIndicator (RTL recalc) inside draw. 1.3.1: draw never reads the RTL; calc reads it
    //      once per batch of new closed bars (as lsCobImpulse); a guard file blocks the read after a read that killed IRT.
    {
        wipe(); seedStore("ES", 180, 3, (long long)F.t[0]);
        auto flags2 = [&](mock::Host& H, int upto) { H.cust["cob_bull_3m_1b"].assign((size_t)upto + 1, 0.0f); H.cust["cob_bear_3m_1b"].assign((size_t)upto + 1, 0.0f); if (upto >= 50) H.cust["cob_bear_3m_1b"][50] = 1.0f; };   // (1.3.3) the bear CoB leg 47 -> 53
        mock::Host N; mock::current = &N; chart(N, F, 79); flags2(N, 79);
        drawBoth(x, N); drawBoth(x, N);                            // added: IRT draws first
        CHECK(N.customInDraw == 0 && N.customCalls == 0, "(1.3.1) add -> first draws: no RTL read inside draw() (" << N.customInDraw << " in draw, 1.3.0 = 2)");
        x.calc(0); WState* sn = static_cast<WState*>(N.userData);
        CHECK(N.customCalls == 2 && N.customInDraw == 0 && sn && sn->E.waves.size() == 6 && sn->E.waves[4].kind == wpc::K_COB, "(1.3.1) the first calc reads both RTL arrays once and draws the CoB leg (" << N.customCalls << " reads)");
        for (int r = 0; r < 5; ++r) { drawBoth(x, N); x.calc(0); }
        CHECK(N.customCalls == 2 && N.customInDraw == 0, "(1.3.1) more ticks / draws with no new bar: no more RTL reads (" << N.customCalls << ")");
        chart(N, F, 79); x.clear();
        // bar by bar: 2 reads per new closed bar, all from calc, never from draw
        wipe(); seedStore("ES", 180, 3, (long long)F.t[0]);
        mock::Host M; mock::current = &M;
        for (int k = 1; k <= 79; ++k) { chart(M, F, k); flags2(M, k); drawBoth(x, M); x.calc(0); drawBoth(x, M); }
        WState* sm = static_cast<WState*>(M.userData);
        CHECK(M.customInDraw == 0 && sm && sm->E.waves.size() == 6 && sm->E.waves[4].kind == wpc::K_COB, "(1.3.1) bar by bar with draws first: CoB leg found, 0 RTL reads in draw (" << M.customInDraw << ")");
        CHECK(!std::ifstream(flex() + "/WavePOC/_cobread-ES-180.txt").good(), "(1.3.1) the crash guard file is removed after every read");
        x.clear();
        // a guard file left behind (the read killed IRT): no read on that chart, waves still drawn, status says why
        wipe(); seedStore("ES", 180, 3, (long long)F.t[0]);
        { std::ofstream g(flex() + "/WavePOC/_cobread-ES-180.txt"); g << "x\n"; }
        mock::Host G2; mock::current = &G2; chart(G2, F, 79); flags2(G2, 79); x.calc(0); WState* sg = static_cast<WState*>(G2.userData);
        CHECK(G2.customCalls == 0 && sg && sg->E.waves.size() == 5 && sg->cobSrc.find("NOT READ (crash guard") != std::string::npos, "(1.3.1) guard file present: the RTL is never read on that chart, the 2 waves still drawn (" << G2.customCalls << ")");
        x.clear();
    }
    mock::current = &A; x.clear();   // (1.3.0 test fix) chart A removed at the end (LeakSanitizer)
    CHECK(A.userData == nullptr, "chart A removed: its state is freed");
    wipe(); rmdir(home.c_str());
    std::cout << "test_wavepoc_host: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
