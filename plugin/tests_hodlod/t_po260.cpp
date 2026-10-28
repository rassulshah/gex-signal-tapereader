// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_po260.cpp -o t260 && ./t260   (from plugin\tests_hodlod)
// lsHodLod 2.6.0 PRE-OPEN BOOK (PreOpenLogic.h 1.0.0, Rassul 15:57 CT 2026-10-22 "yes hod lod box", T-20261022-29) tests, SDK-free:
//  [1] engine: near band, resting lots per side, STAYED (rested >= 60 s, still there) vs PULLED (rested >= 60 s, then cancelled / cut /
//      moved inside the window); actual filled lots are never a pull, but an explicit DEL / MOD of unfilled residuals is; flashed orders
//      (< 60 s) count in neither; unknown orders ignored
//  [2] the call: bids 1.5x + 50% stay -> 1st LOW; 2.0x + 60% -> early; offers -> 1st HIGH; even -> no lean; heavy but pulled -> no lean
//  [3] timing: frozen at the first event at / after the open (the book as it was), imbalance after open + 3 min (offer fills = buys);
//      a feed that started after the window = "started late"; the window setting; another day's events ignored
//  [4] the line: hidden before -30 min / after +60 min, "Pre-open read from 8:15 AM", live, frozen, final; colours; 12-hour times
//  [5] the record: SNAP / IMB lines round trip, a restored engine draws the identical line (no repaint), strict parse (garbled, other
//      session / market / bar size, torn line, stayed > lots)
//  [6] the hover: question / what / why / how / example first, then the numbers
//  [7] the box: the Pre row inside the table under Est, no text overlaps, the table only grows by one row, the hover inside the pane and
//      clear of the table; preOn = false = the 2.5.0 table exactly
//  [8] versions: HL_VERSION == hlset::PLUGIN_VERSION == setVersion literal == 2.6.0; parameter version 8; the 2 new settings rows
#include <cstdio>
#include <cmath>
#include <fstream>
#include <sstream>
#include "PreOpenLogic.h"
#include "HodLodLogic.h"
#include "SettingsSpec.h"
#include "HodLodSettings.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static int measure(const std::string& s, int fs, bool bold) { return (int)(s.size() * fs * (bold ? 0.62 : 0.56) + 0.999); }
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static const long long DAY = 20748LL * 86400000LL;            // 2026-10-22 CT midnight (ms)
static long long at(int h, int m, int sec = 0) { return DAY + ((long long)h * 3600 + m * 60 + sec) * 1000LL; }
static po::Engine mk(const char* mkt = "ES") { po::Engine E; E.mkt = mkt; E.openMin = 510; E.s20 = po::s20Of(mkt); E.nearTicks = 8; E.setDay(DAY); return E; }
static unsigned long long gid = 1000;
// a trade at px (sets the reference) - a 1-lot fill of a throw-away bid
static void trade(po::Engine& E, long long px, long long ms) { E.onEvent(po::NEW, po::BID, px, 1, ++gid, ms); E.onEvent(po::FILL, po::BID, px, 1, gid, ms); E.onEvent(po::DEL, po::BID, px, 0, gid, ms); }

int main() {
    std::printf("PreOpenLogic %s / HodLod %s\n", po::VERSION, hl::HL_VERSION);
    {   // [1] stayed vs pulled
        po::Engine E = mk();
        trade(E, 30000, at(7, 46));                                     // the feed starts 7:46 (window 8:15, lead 30 min: 7:45)
        E.onEvent(po::NEW, po::BID, 29998, 50, 1, at(8, 0));           // rests from 8:00, stays -> stayed 50 (S20 25: big)
        E.onEvent(po::NEW, po::BID, 29996, 20, 2, at(8, 16));          // rests 8:16, pulled at 8:20 (4 min) -> pulled 20
        E.onEvent(po::DEL, po::BID, 29996, 20, 2, at(8, 20));
        E.onEvent(po::NEW, po::BID, 29997, 30, 3, at(8, 20));          // flashed 10 s -> neither
        E.onEvent(po::DEL, po::BID, 29997, 30, 3, at(8, 20, 10));
        E.onEvent(po::NEW, po::ASK, 30002, 10, 4, at(8, 10));          // offer 10, moved away at 8:25 (rested 15 min) -> pulled 10 (old price near)
        E.onEvent(po::MOD, po::ASK, 30050, 10, 4, at(8, 25));          // the new price is far (not near): not counted at the open
        E.onEvent(po::NEW, po::ASK, 30003, 12, 5, at(8, 0));           // offer 12, cut to 4 at 8:22 -> pulled 8, stays 4
        E.onEvent(po::MOD, po::ASK, 30003, 4, 5, at(8, 22));
        E.onEvent(po::NEW, po::BID, 29999, 6, 6, at(8, 1));            // bid 6 filled 2 then unchanged residual 4 (not a pull)
        E.onEvent(po::FILL, po::BID, 29999, 2, 6, at(8, 21)); E.onEvent(po::MOD, po::BID, 29999, 4, 6, at(8, 21));
        E.onEvent(po::NEW, po::BID, 29999, 9, 7, at(8, 1));            // bid 9 fully filled then DEL: no residual pull
        E.onEvent(po::FILL, po::BID, 29999, 9, 7, at(8, 23)); E.onEvent(po::DEL, po::BID, 29999, 0, 7, at(8, 23));
        E.onEvent(po::DEL, po::BID, 29999, 99, 77777, at(8, 24));      // never seen: ignored
        E.onEvent(po::NEW, po::BID, 29950, 500, 8, at(8, 0));          // far bid (50 ticks): never counted
        E.onEvent(po::DEL, po::BID, 29950, 500, 8, at(8, 26));
        E.onEvent(po::NEW, po::BID, 29998, 7, 9, at(8, 29, 30));       // 30 s before the open: counted in lots, not committed
        CHECK(!E.snapDone, "no snap before the open");
        trade(E, 29999, at(8, 30, 1));                                 // the first event at / after the open freezes the read
        CHECK(E.snapDone, "snap at the first event after the open");
        const po::Snap& s = E.snap;
        CHECK(s.lots[po::BID] == 50 + 4 + 7, "bid lots near = 61 (" + std::to_string(s.lots[po::BID]) + ")");
        CHECK(s.lots[po::ASK] == 4, "offer lots near = 4 (" + std::to_string(s.lots[po::ASK]) + ")");
        CHECK(s.stayed[po::BID] == 54 && s.pulled[po::BID] == 20, "bids stayed 54 / pulled 20 (" + std::to_string(s.stayed[po::BID]) + "/" + std::to_string(s.pulled[po::BID]) + ")");
        CHECK(s.stayed[po::ASK] == 4 && s.pulled[po::ASK] == 18, "offers stayed 4 / pulled 18 (" + std::to_string(s.stayed[po::ASK]) + "/" + std::to_string(s.pulled[po::ASK]) + ")");
        CHECK(s.big[po::BID] == 1 && s.big[po::ASK] == 0, "S20 big orders: bids 1");
        CHECK(po::stayPct(s, po::BID) == 73, "bid stay 54 / 74 = 73%");
        CHECK(s.call == 1 && s.early, "bids 15x + 73% -> 1st LOW early");
        CHECK(po::snapText(s, nullptr) == "Bids 15.2x \xB7 73% stay \xB7 1st LOW early ?", "line text: " + po::snapText(s, nullptr));
        // imbalance: offers filled = buys
        E.onEvent(po::NEW, po::ASK, 30000, 40, 50, at(8, 30, 5)); E.onEvent(po::FILL, po::ASK, 30000, 30, 50, at(8, 31));
        E.onEvent(po::NEW, po::BID, 29999, 10, 51, at(8, 30, 5)); E.onEvent(po::FILL, po::BID, 29999, 10, 51, at(8, 32));
        E.onEvent(po::FILL, po::ASK, 30000, 10, 50, at(8, 33, 1));     // after open + 3 min: not counted, closes the window
        CHECK(E.imbDone && E.imb.ok && E.imb.buy == 30 && E.imb.sell == 1 + 10, "imbalance buy 30 / sell 11 (the 8:30:01 trade was a bid fill) (" + std::to_string(E.imb.buy) + "/" + std::to_string(E.imb.sell) + ")");
        CHECK(po::imbText(E.imb) == "open buy 2.7x", "imb text: " + po::imbText(E.imb));
        const po::Line L = po::lineAt(E, at(8, 40), 1);
        CHECK(L.show && L.state == 4 && L.col == po::COL_GREEN && L.s == "Bids 15.2x \xB7 73% stay \xB7 open buy 2.7x \xB7 1st LOW early ?", "final line: " + L.s);
        // the frozen read never changes after the open (later book changes do not repaint it)
        E.onEvent(po::NEW, po::ASK, 30001, 900, 60, at(8, 34));
        CHECK(po::lineAt(E, at(8, 50), 1).s == L.s, "frozen: later orders do not change the line");
        // [5] record round trip -> a restored engine draws the same line
        const std::string r1 = po::recLine(E, 180, false), r2 = po::recLine(E, 180, true);
        po::Engine R = mk(); po::Snap sn; po::Imb im;
        CHECK(po::parseRec(r1, "2026-10-22", "ES", 180, DAY, sn, im) == 1, "SNAP parses");
        R.restoreSnap(sn);
        CHECK(po::parseRec(r2, "2026-10-22", "ES", 180, DAY, sn, im) == 2, "IMB parses");
        R.restoreImb(im);
        const po::Line L2 = po::lineAt(R, at(8, 40), 1);
        CHECK(L2.s == L.s && L2.col == L.col && L2.state == L.state, "restored line identical: " + L2.s);
        CHECK(po::parseRec(r1, "2026-10-21", "ES", 180, DAY, sn, im) == 0, "other session ignored");
        CHECK(po::parseRec(r1, "2026-10-22", "NQ", 180, DAY, sn, im) == 0, "other market ignored");
        CHECK(po::parseRec(r1, "2026-10-22", "ES", 60, DAY, sn, im) == 0, "other bar size ignored");
        CHECK(po::parseRec(r1.substr(0, r1.size() / 2), "2026-10-22", "ES", 180, DAY, sn, im) == 0, "torn line ignored");
        std::string bad = r1; { size_t k = 0; for (int c = 0; c < 11; ++c) k = bad.find(',', k) + 1; bad.replace(k, bad.find(',', k) - k, "x7"); }
        CHECK(po::parseRec(bad, "2026-10-22", "ES", 180, DAY, sn, im) == 0, "garbled number ignored");
        CHECK(r1.find('\n') == r1.size() - 1 && std::string(po::REC_HEADER).find("stay_bid") != std::string::npos, "one line per record");
        // [6] hover order
        const std::vector<po::HLine> H = po::hover(E, at(8, 40));
        CHECK(H.size() >= 8 && H[0].s.find("FIRST extreme?") != std::string::npos && H[1].s.compare(0, 11, "What it is:") == 0 && H[2].s.compare(0, 15, "Why it matters:") == 0
              && H[3].s.compare(0, 14, "How to use it:") == 0 && H[4].s.compare(0, 8, "Example:") == 0, "hover: question / what / why / how / example");
        CHECK(H[5].s.find("At the open 8:30 AM: bids 61 lots") != std::string::npos, "hover numbers: " + H[5].s);
        CHECK(H[7].s.find("buyers 30 lots vs sellers 11 lots") != std::string::npos, "hover open: " + H[7].s);
    }
    {   // [1b] a FILL is actual execution, but an explicit action later removes its unfilled residual from the book
        po::Engine E = mk(); trade(E, 30000, at(7, 46));
        E.onEvent(po::NEW, po::BID, 29999, 10, 201, at(8, 16));
        E.onEvent(po::FILL, po::BID, 29999, 4, 201, at(8, 18));
        E.onEvent(po::DEL, po::BID, 29999, 6, 201, at(8, 20));          // 6 unfilled lots cancelled
        E.onEvent(po::NEW, po::ASK, 30001, 10, 202, at(8, 16));
        E.onEvent(po::FILL, po::ASK, 30001, 3, 202, at(8, 18));
        E.onEvent(po::MOD, po::ASK, 30001, 4, 202, at(8, 20));          // 3 unfilled lots cut at the price
        E.onEvent(po::NEW, po::ASK, 30002, 10, 203, at(8, 16));
        E.onEvent(po::FILL, po::ASK, 30002, 2, 203, at(8, 18));
        E.onEvent(po::MOD, po::ASK, 30050, 8, 203, at(8, 20));          // 8 unfilled lots repriced away
        const po::Snap s = E.readAt(at(8, 29));
        CHECK(s.pulled[po::BID] == 6 && s.pulled[po::ASK] == 11,
              "partial-fill residuals: DEL bid 6 + same-price cut ask 3 + reprice ask 8 (" + std::to_string(s.pulled[po::BID]) + "/" + std::to_string(s.pulled[po::ASK]) + ")");
        CHECK(E.book.find(201) == E.book.end() && E.book.find(202) != E.book.end() && E.book.find(203) != E.book.end(),
              "DEL removes cancelled order; MOD keeps the re-sized/repriced orders");
    }
    {   // [2] the call variants
        po::Params P; po::Snap s;
        s.lots[po::BID] = 150; s.lots[po::ASK] = 100; s.stayed[po::BID] = 50; s.pulled[po::BID] = 50; po::decideCall(s, P);
        CHECK(s.call == 1 && !s.early, "1.5x + 50% -> LOW, not early");
        s.lots[po::BID] = 149; po::decideCall(s, P); CHECK(s.call == 0, "1.49x -> no lean");
        s.lots[po::BID] = 300; s.stayed[po::BID] = 40; s.pulled[po::BID] = 60; po::decideCall(s, P); CHECK(s.call == 0, "heavy but 40% stayed -> no lean");
        s = po::Snap(); s.lots[po::ASK] = 200; s.lots[po::BID] = 100; s.stayed[po::ASK] = 60; s.pulled[po::ASK] = 40; po::decideCall(s, P);
        CHECK(s.call == -1 && s.early, "offers 2.0x + 60% -> 1st HIGH early");
        CHECK(po::snapText(s, nullptr) == "Offers 2.0x \xB7 60% stay \xB7 1st HIGH early ?", "HIGH text: " + po::snapText(s, nullptr));
        s = po::Snap(); s.lots[po::ASK] = 110; s.lots[po::BID] = 100; po::decideCall(s, P);
        CHECK(s.call == 0 && po::snapText(s, nullptr) == "Book even \xB7 no lean", "even: " + po::snapText(s, nullptr));
        s = po::Snap(); CHECK(po::snapText(s, nullptr) == "Book empty near price \xB7 no lean", "empty");
        s.lots[po::BID] = 10; po::decideCall(s, P); CHECK(s.call == 0, "no offers + nothing committed -> no lean");
    }
    {   // [3] late feed, window setting, other day
        po::Engine E = mk(); trade(E, 30000, at(8, 17));                // window 8:15: feed from 8:17 = late
        trade(E, 30000, at(8, 30, 2));
        CHECK(E.snapDone && E.snap.late && !E.snap.ok && E.snap.call == 0, "late feed: no read");
        CHECK(po::lineAt(E, at(8, 31), 1).s == "Pre-open: started late - no read", "late line");
        po::Engine W = mk(); W.P.windowMin = 30; trade(W, 30000, at(7, 31));   // window 8:00, lead 30 -> feed from 7:30 is in time
        trade(W, 30000, at(8, 30)); CHECK(W.snapDone && !W.snap.late, "30-min window: feed from 7:31 in time");
        po::Engine W2 = mk(); W2.P.windowMin = 20; CHECK(po::lineAt(W2, at(8, 5), 1).s == "Pre-open read from 8:10 AM", "window text 8:10: " + po::lineAt(W2, at(8, 5), 1).s);
        po::Engine D = mk(); D.onEvent(po::NEW, po::BID, 30000, 5, 1, at(8, 0) - 86400000LL); CHECK(D.ignored == 1 && D.book.empty(), "yesterday's event ignored");
        po::Engine T = mk(); trade(T, 30000, at(8, 10)); T.tick(at(8, 30, 4)); CHECK(!T.snapDone, "tick: 5 s grace for the feed");
        T.tick(at(8, 30, 6)); CHECK(T.snapDone, "tick: decided without an event after the open");
        CHECK(T.wantFeed(at(7, 45)) && !T.wantFeed(at(7, 44, 59)) && T.wantFeed(at(8, 33, 59)) && !T.wantFeed(at(8, 34)), "feed wanted 7:45 - 8:34");
        po::Engine G = mk("GC"); G.openMin = 440; CHECK(po::lineAt(G, at(6, 50), 0).s == "Pre-open read from 7:05 AM (MBO not ready)", "GC 7:20 open: " + po::lineAt(G, at(6, 50), 0).s);
    }
    {   // [4] line states and visibility
        po::Engine E = mk();
        CHECK(!po::lineAt(E, at(7, 59, 59), 1).show, "hidden before 8:00");
        CHECK(po::lineAt(E, at(8, 0), 1).show && po::lineAt(E, at(8, 0), 1).state == 1, "waiting from 8:00");
        CHECK(po::lineAt(E, at(8, 20), 0).s == "Pre-open: no order feed (MBO)", "no feed in the window");
        trade(E, 30000, at(7, 50)); E.onEvent(po::NEW, po::ASK, 30001, 40, 1, at(8, 0)); E.onEvent(po::NEW, po::BID, 29999, 10, 2, at(8, 0));
        const po::Line Lv = po::lineAt(E, at(8, 20), 1);
        CHECK(Lv.state == 2 && Lv.col == po::COL_RED && Lv.s == "Offers 4.0x \xB7 100% stay \xB7 1st HIGH early ?", "live line: " + Lv.s);
        trade(E, 30000, at(8, 30, 1));
        CHECK(po::lineAt(E, at(8, 31), 1).state == 3, "frozen before the imbalance");
        CHECK(!po::lineAt(E, at(9, 30), 1).show && po::lineAt(E, at(9, 29, 59), 1).show, "hidden from 9:30 (open + 60)");
        CHECK(po::hm12(0) == "12:00 AM" && po::hm12(780) == "1:00 PM" && po::hm12(495) == "8:15 AM", "12-hour times");
        CHECK(po::lotsTxt(1234567) == "1,234,567", "lots with commas");
    }
    {   // [7] the box: the Pre row
        auto view = [](bool pre) {
            hl::View v; v.has = true; v.prior = true; v.headMain = "prior RTH 10/21";
            v.headLeftSegs.push_back(hl::Seg{ "ES RTH 8:30-3:00", hl::C_SLATE });
            v.sessSegs.push_back(hl::Seg{ "Regime: Negative", hl::C_RED }); v.sessSegs.push_back(hl::Seg{ "Expiry 3:00 PM (in 7h 10m)", hl::C_GREY });
            v.trendReady = true; v.trendLvl = -2;
            v.E[hl::K_1ST] = hl::mkCell("8:33 - 9:21", hl::C_BLUE); v.headTail = "late-day LOD";
            v.A[hl::K_LABEL] = hl::mkCell("Actual", hl::C_SLATE); v.A[hl::K_1ST] = hl::mkCell("8:33 HOD in", hl::C_SLATE); v.A[hl::K_2ND] = hl::mkCell("2:09 LOD PFL", hl::C_SLATE);
            if (pre) { v.preOn = true; v.preText = "Bids 2.1x \xB7 70% stay \xB7 open sell 1.8x \xB7 1st LOW early ?"; v.preCol = hl::C_GREEN;
                       v.preHover.push_back(hl::Line{ hl::Seg{ "Pre-open book - what does the book before the 8:30 AM open say about the day's FIRST extreme?", hl::C_SLATE } });
                       for (int i = 0; i < 7; ++i) v.preHover.push_back(hl::Line{ hl::Seg{ "a long explanatory hover line that wraps across the box when it is too wide for the room", hl::C_GREY } }); }
            return v;
        };
        const hl::Box pane = { 0, 0, 1600, 800 };
        auto yOf = [](long long t) { return (int)(400 - t / 4); };
        for (int pos = 0; pos < 9; ++pos) {
            const hl::View v0 = view(false), v1 = view(true);
            const hl::Layout A = hl::layoutAll(v0, pane, yOf, measure, 1500, pos);
            hl::Layout B = hl::layoutAll(v1, pane, yOf, measure, 1500, pos);
            CHECK(A.tableOk && B.tableOk && !A.preOk && B.preOk, "pos " + std::to_string(pos) + ": tables laid out, Pre row only when on");
            const int rowH = hl::lineH(10) + 1;
            CHECK((B.table.b - B.table.t) - (A.table.b - A.table.t) == rowH, "pos " + std::to_string(pos) + ": the table grows by one row");
            CHECK(B.preRow.t >= B.table.t && B.preRow.b <= B.table.b && B.preRow.l == B.table.l && B.preRow.r == B.table.r, "Pre row inside the table");
            bool estAbove = false, inside = true, overl = false; std::vector<hl::Item> T;
            for (const hl::Item& it : B.items) if (it.k == hl::K_TEXT && it.role == hl::R_TABLE) {
                if (it.s == "Act" && it.b <= B.preRow.t + 1) estAbove = true;   // (2.12.0) no Est row: Act is the last row above Pre
                if (it.l < B.table.l || it.r > B.table.r) inside = false;
                T.push_back(it);
            }
            for (size_t i = 0; i < T.size(); ++i) for (size_t j = i + 1; j < T.size(); ++j)
                if (T[i].l < T[j].r && T[j].l < T[i].r && T[i].t < T[j].b && T[j].t < T[i].b) { overl = true; std::printf("  overlap '%s' / '%s'\n", T[i].s.c_str(), T[j].s.c_str()); }
            CHECK(estAbove && inside && !overl, "pos " + std::to_string(pos) + ": Est above Pre, every text inside, no overlaps");
            bool preTxt = false; for (const hl::Item& it : B.items) if (it.k == hl::K_TEXT && it.s == v1.preText && it.c1 == hl::C_GREEN) preTxt = true;
            CHECK(preTxt, "the Pre text drawn in green");
            hl::layoutPreHover(B, v1, pane, 1500, measure);
            CHECK(B.preHoverOk && B.preHover.l >= pane.l && B.preHover.r <= 1500 && B.preHover.t >= pane.t + hl::TITLE_ROW_PX && B.preHover.b <= pane.b, "hover inside the pane, left of the last bar");
            CHECK(!hl::overlap(B.preHover, B.table), "hover clear of the table");
            // the 2.5.0 table (preOn false) is byte-identical to a layout without any Pre field
            hl::View v2 = view(false); v2.preText = "ignored"; const hl::Layout C = hl::layoutAll(v2, pane, yOf, measure, 1500, pos);
            bool same = C.items.size() == A.items.size(); for (size_t i = 0; same && i < A.items.size(); ++i) same = A.items[i].s == C.items[i].s && A.items[i].l == C.items[i].l && A.items[i].t == C.items[i].t;
            CHECK(same, "preOn false = unchanged table");
        }
    }
    {   // [8] versions and settings
        const std::string src = readAll("../HodLod.cpp");
        CHECK(std::string(hl::HL_VERSION) == "2.7.0" && std::string(hlset::PLUGIN_VERSION) == "2.7.0", "HL_VERSION / PLUGIN_VERSION 2.6.0");
        CHECK(src.find("p->setVersion(\"2.7.0\")") != std::string::npos, "setVersion literal 2.6.0");
        CHECK(src.find("static const bool HL_HOVERS = false;") != std::string::npos && src.find("static const bool HL_PRE_HOVER = true;") != std::string::npos, "hovers: only the Pre row");
        CHECK(src.find("((HL_HOVERS || HL_PRE_HOVER) ? TRACK_MOUSE : 0)") != std::string::npos, "TRACK_MOUSE only through the switches");
        const ss::Spec& S = hlset::spec();
        CHECK(S.paramVersion == 8 && S.n == hlset::K_N && hlset::K_N == 11, "parameter version 8, 11 rows");
        CHECK(std::string(S.rows[hlset::K_SHOW_PRE].key) == "show_pre" && S.rows[hlset::K_SHOW_PRE].def == 1, "Show pre-open line default on");
        CHECK(std::string(S.rows[hlset::K_PRE_MIN].key) == "pre_min" && S.rows[hlset::K_PRE_MIN].def == 15 && S.rows[hlset::K_PRE_MIN].lo == 5 && S.rows[hlset::K_PRE_MIN].hi == 60, "Pre-open minutes 15 (5..60)");
        CHECK(po::s20Of("ES") == 25 && po::s20Of("NQ") == 15 && po::s20Of("HG") == 5 && po::s20Of("EU") == 41, "S20 overnight tiers");
    }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
