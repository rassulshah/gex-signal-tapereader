// test_wavepoc_host.cpp - lsWavePOC 1.0.0 host suite on the mock SDK (WavePOC_test_support/irtsdk.h).
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
static void drawBoth(WavePOC& x, mock::Host& H) { mock::current = &H; H.drawn.clear(); H.reason = REASON_PREDRAW; x.draw(); H.reason = REASON_DRAW; x.draw(); }
static void wipe() { std::string cmd = "rm -rf '" + home + "/InvestorRT'"; if (std::system(cmd.c_str()) != 0) std::cerr << "rm failed\n"; }

int main(int argc, char** argv) {
    const Fix F = load(argc > 1 ? argv[1] : "wavepoc_es1022.csv");
    if (F.t.size() != 80) { std::cerr << "no fixture\n"; return 1; }
    char tmpl[] = "/tmp/wavepoc_host_XXXXXX"; home = mkdtemp(tmpl); setenv("USERPROFILE", home.c_str(), 1);

    // ---- versions: the setVersion literal = WP_VERSION = the logic = the settings table
    {
        std::string src = slurp("WavePOC.cpp"); if (src.empty()) src = slurp("../WavePOC.cpp"); const size_t p = src.find("p->setVersion(\"");
        const std::string lit = p == std::string::npos ? "" : src.substr(p + 15, src.find('"', p + 15) - p - 15);
        CHECK(lit == WP_VERSION && lit == wpc::VERSION && lit == wpset::PLUGIN_VERSION && lit == "1.1.0" && std::string(wpl::VERSION) == "1.1.0", "version literals match (" << lit << ")");
    }
    // ---- one DLL object, setup once: the settings rows
    WavePOC x; mock::Host A; mock::current = &A; x.setup();
    CHECK(mock::declared.size() == 2 && mock::declared[0].type == 2 && mock::declared[0].i == 3 && mock::declared[0].name == "Reversal bars", "row 1: Reversal bars (int, default 3)");
    CHECK(mock::declared.size() == 2 && mock::declared[1].type == 1 && !mock::declared[1].b && mock::declared[1].name == "Show zigzag", "row 2: Show zigzag (box, default off)");
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
    // ---- chart A: ES 3-min, Oct 22, the store seeded with 20 earlier waves (avg 23.625 / 21.5 pts as the reference day)
    seedStore("ES", 180, 3, (long long)F.t[0]);
    chart(A, F, 79); x.calc(0); drawBoth(x, A);
    WState* S = static_cast<WState*>(A.userData);
    CHECK(S && S->mkt == "ES" && S->N == 3 && !S->zz && S->parms == "native,ok", "chart A: ES, N 3, zigzag off, native parameters");
    CHECK(S && S->storeWhy == "loaded", "store file loaded (" << (S ? S->storeWhy : "") << ")");
    CHECK(S && S->E.waves.size() == 2, "the two Oct 22 waves through the host (" << (S ? S->E.waves.size() : 0) << ")");
    if (S && S->E.waves.size() == 2) {
        CHECK(wpc::pxText(S->E.waves[0].nodes[0].tick, 0.25) == "7719.25" && wpc::pxText(S->E.waves[1].nodes[0].tick, 0.25) == "7700.25", "WPOCs 7719.25 / 7700.25 from IRT's VAP");
        CHECK(S->E.waves[1].nodes.size() == 3 && S->E.waves[0].nodes.size() == 4, "4 + 3 nodes");
    }
    // the draw: behind = fills / boxes / lines in the pre-draw; tags + header in the post-draw; tags right of the last bar
    {
        int pre = 0; mock::current = &A; A.drawn.clear(); A.reason = REASON_PREDRAW; x.draw(); pre = (int)A.drawn.size();
        bool preOnlyBack = true; for (const mock::Prim& p : A.drawn) preOnlyBack = preOnlyBack && p.k != 'T';
        A.drawn.clear(); A.reason = REASON_DRAW; x.draw();
        bool tagsRight = true, header = false; int tags = 0; const int xLast = mockX(79) + 5; bool profInPost = false;
        for (const mock::Prim& p : A.drawn) {
            if (p.k == 'T') { if (p.text == std::string("Wave POC v") + WP_VERSION) header = true; else { ++tags; tagsRight = tagsRight && p.x0 > xLast; } }
            if (p.k == 'F' && p.col == 0x334155) profInPost = true;
        }
        CHECK(pre > 20 && preOnlyBack, "pre-draw: the profile, nodes and lines (no text) - behind the candles");
        CHECK(header && tags > 0 && tagsRight, "post-draw: header + price tags, only right of the last bar (" << tags << " tags)");
        CHECK(!profInPost, "the profile is not drawn again over the candles once IRT pre-draws");
        bool amber3 = false, amberBox = false; int whiteBoxes = 0, amberBoxes = 0, anyBox = 0;
        mock::current = &A; A.drawn.clear(); A.reason = REASON_PREDRAW; x.draw();
        for (const mock::Prim& p : A.drawn) { if (p.k == 'L' && p.col == 0xF5B428 && p.w == 3) amber3 = true; if (p.k == 'F' && p.col == 0xC88C14) { amberBox = true; ++amberBoxes; } if (p.k == 'F' && p.col == 0xB4BECD) ++whiteBoxes; if (p.k == 'B') ++anyBox; }
        CHECK(amber3 && amberBox && anyBox == 0, "WPOC stands out: bright amber row + 3 px amber line; no outline boxes anywhere");
        CHECK(amberBoxes == 3 && whiteBoxes == 5 && S->provOk && S->prov.i0 == 75 && S->prov.i1 == 78, "one WPOC per wave (2 confirmed + the live provisional 14:15-14:24 up wave), the other 5 nodes white " << amberBoxes << "/" << whiteBoxes);
        int dashes = 0, solid = 0; for (const mock::Prim& p : A.drawn) if (p.k == 'L' && p.col == 0xF5B428 && p.w == 3 && p.y0 == p.y1) { if (p.x1 - p.x0 <= 5) ++dashes; else ++solid; }
        CHECK(dashes > 5 && solid >= 1, "WPOC lines: dashed until confirmed (and the provisional wave), solid after (" << dashes << " dashes, " << solid << " solid)");
        bool inPane = true; for (const mock::Prim& p : A.drawn) inPane = inPane && p.x0 >= A.paneL && p.x1 <= A.paneR && std::min(p.y0, p.y1) >= A.paneT && std::max(p.y0, p.y1) <= A.paneB;
        CHECK(inPane, "everything inside the pane");
    }
    // files: record, store, status
    {
        const std::string rec = slurp(flex() + "/WavePOC/ES-180-nodes.csv"), st = slurp(flex() + "/WavePOC.status-ES-180.txt"), sto = slurp(flex() + "/WavePOC/ES-180.csv");
        size_t nw = 0; for (size_t p = rec.find("\nWAVE,"); p != std::string::npos; p = rec.find("\nWAVE,", p + 1)) ++nw;
        CHECK(nw == 2 && rec.find("NODE,") != std::string::npos && rec.find("STOP,") != std::string::npos, "record: 2 waves with nodes and stops");
        CHECK(st.rfind("VERSION,1.1.0", 0) == 0 && st.find("qualified up 0 down 2") != std::string::npos && st.find("PARMS,native,ok,reversal bars 3") != std::string::npos, "status file: version, N, qualified waves");
        CHECK(st.find("touch 5,new wave 2,origin close 0") != std::string::npos, "status: stops by reason");
        wpc::Store chk; std::string why; CHECK(wpc::storeFromText(sto, "ES", 180, 3, 250000000LL, chk, why) && chk.dn.size() == 24, "store saved atomically with the day's 4 down candidates (" << why << ", " << chk.dn.size() << ")");
    }
    // no pre-draw call: everything in the post call
    {
        mock::Host P = A; P.userData = nullptr; mock::current = &P; x.calc(0); P.drawn.clear(); P.reason = REASON_DRAW; x.draw();
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
        bool c5 = s5 != nullptr; if (s5) for (const wpc::Wave& w : s5->E.waves) c5 = c5 && w.conf == w.i1 + 5;
        CHECK(c5 && s5 && s5->E.alt.size() < S->E.alt.size(), "N 5: fewer pivots, confirmation 5 bars after the extreme");
        mock::current = &A; x.calc(0);
        CHECK(static_cast<WState*>(A.userData)->N == 3, "chart A keeps N 3 (per-chart state)");
        mock::current = &B5; x.clear();
    }
    // a second chart (NQ) on the same DLL: own market, own files
    {
        mock::Host Q = A; Q.userData = nullptr; Q.root = "ENQ"; Q.sym = "ENQZ26"; mock::current = &Q; x.calc(0); WState* q = static_cast<WState*>(Q.userData);
        CHECK(q && q != S && q->mkt == "NQ" && q->storePath.find("NQ-180.csv") != std::string::npos && q->E.waves.empty(), "NQ chart: own state, own store (no history yet -> nothing qualifies)");
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
        CHECK(same && si->E.waves.size() == 2, "bar-by-bar feeding = one straight pass (no repaint)");
        CHECK(so && so->E.C.recordUsed == 2, "restart: the recorded waves are redrawn from the record");
        const std::string rec = slurp(flex() + "/WavePOC/ES-180-nodes.csv"); size_t nw = 0; for (size_t p = rec.find("\nWAVE,"); p != std::string::npos; p = rec.find("\nWAVE,", p + 1)) ++nw;
        CHECK(nw == 2, "a restart never writes a wave twice");
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
        CHECK(sv && sv->E.pendingCount() == 1 && sv->E.waves.size() == 2 && sv->E.waves[0].pending, "VAP missing -> the wave waits (pending), retried every 10 min");
        drawBoth(x, V); int amber = 0; for (const mock::Prim& p : V.drawn) amber += p.k == 'F' && p.col == 0xC88C14;
        CHECK(amber == 2, "only the complete wave (+ the provisional one) is drawn " << amber);
        x.clear();
    }
    // symbol change on the same chart: the state follows the new market
    {
        mock::Host C = A; C.userData = nullptr; mock::current = &C; x.calc(0); C.root = "QGC"; C.sym = "QGCZ26"; C.tick = 0.1; x.calc(0);
        WState* sc = static_cast<WState*>(C.userData);
        CHECK(sc && sc->mkt == "GC" && std::fabs(sc->E.tick - 0.1) < 1e-6 && sc->E.waves.empty(), "symbol change -> GC, its own tick and store " << (sc ? sc->mkt : "") << " " << (sc ? sc->E.tick : 0) << " " << (sc ? sc->E.waves.size() : 9));
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
        bool ok = sm && sm->E.waves.size() == 2;
        if (ok) {
            const wpc::Wave& w2 = sm->E.waves[1];
            CHECK(w2.liqOk && !w2.lnodes.empty() && wpc::pxText(w2.lnodes[0].tick, 0.25) == "7685.25" && w2.lnodes[0].side == 1, "LPOC of wave 2 = the 400-lot bid at 7685.25 (" << (w2.lnodes.empty() ? "none" : wpc::pxText(w2.lnodes[0].tick, 0.25)) << ")");
            bool mm = false; for (const wpc::LNode& n : w2.lnodes) mm = mm || n.tick == 30781 || n.tick == 30783;
            CHECK(!mm, "the re-pricing 900-lot market maker is left out");
        } else CHECK(false, "two waves on the live-book chart");
        drawBoth(x, M); int lp = 0, tags = 0; for (const mock::Prim& p : M.drawn) { lp += p.k == 'F' && p.col == 0x22C55E; tags += p.k == 'T'; }
        CHECK(lp >= 1, "LPOC drawn in bright green (bids)");
        const std::string rec = slurp(flex() + "/WavePOC/ES-180-nodes.csv"), st = slurp(flex() + "/WavePOC.status-ES-180.txt");
        CHECK(rec.find("\nLNODE,") != std::string::npos && (rec.find("near_volume_node") != std::string::npos || rec.find(",alone") != std::string::npos), "record: LNODE lines with near / alone");
        CHECK(st.find("MBO,open,events") != std::string::npos && st.find("LIQUIDITY_NODES,waves with liquidity") != std::string::npos, "status: MBO + LIQUIDITY_NODES lines");
        x.clear();
        CHECK(mock::dboAlive == alive0, "chart removed: the feed object is destroyed (unsubscribed)");
    }
    wipe(); rmdir(home.c_str());
    std::cout << "test_wavepoc_host: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
