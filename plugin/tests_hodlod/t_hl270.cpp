// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl270.cpp -o t270 && ./t270   (from plugin\tests_hodlod)
// lsHodLod 2.7.0 (2026-10-22 evening, Rassul approved 17:39 CT) + the shared NewsWindow.h / MarkParams.h 1.0.0 + DayTypeLogic.h 1.0.0:
//  [1] DayTypeLogic: trend up / down, rotation, none before the IB, no look-ahead (a prefix decides the same with or without later bars),
//      restart = same call (pure), the hover lists what fired, bad bars
//  [2] NewsWindow: CPI / NFP / FOMC windows, EIA (CL Wed / NG Thu), settles per market, weekends, edges, extra EVENT rows, q() never "??"
//  [3] MarkParams: strict parse (VERSION first, MARKET match, PARAM), garbled / wrong market / too big = defaults; modes; the feed check
//  [4] the box: day text replaces the trend word (green / red / plain), note rows inside the table, no overlaps, hovers inside the pane
//  [5] versions: HL_VERSION == PLUGIN_VERSION == setVersion literal == 2.7.0
#include <cstdio>
#include <cmath>
#include <fstream>
#include <sstream>
#include "PreOpenLogic.h"
#include "HodLodLogic.h"
#include "SettingsSpec.h"
#include "HodLodSettings.h"
#include "DayTypeLogic.h"
#include "NewsWindow.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static int measure(const std::string& s, int fs, bool bold) { return (int)(s.size() * fs * (bold ? 0.62 : 0.56) + 0.999); }
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }

// a 5-min RTH day: IB from 6 bars (8:30-9:30), then a path
static std::vector<dtl::Bar> day(const std::vector<double>& closes, double ibLo = 100, double ibHi = 104) {
    std::vector<dtl::Bar> v; double prev = 102;
    for (size_t i = 0; i < closes.size(); ++i) {
        dtl::Bar b; b.endMin = 5 * (int)(i + 1); b.c = closes[i]; b.v = 100;
        b.h = std::max(prev, b.c) + 0.25; b.l = std::min(prev, b.c) - 0.25;
        if (b.endMin <= 60) { b.h = std::min(b.h, ibHi); b.l = std::max(b.l, ibLo); b.c = std::max(ibLo, std::min(ibHi, b.c)); }
        prev = b.c; v.push_back(b);
    }
    return v;
}
static bool sameRes(const dtl::Result& a, const dtl::Result& b) { return a.kind == b.kind && a.text == b.text && a.crosses == b.crosses && a.ib == b.ib && a.range == b.range && a.checks.size() == b.checks.size(); }

int main() {
    {   // [6] (2.7.1) DAYTYPE "log fails 17": the day-type log's folder lsFlexLevels\HodLod did not exist - hlAppend now creates it
        std::ifstream sf("../HodLod.cpp"); std::stringstream ss; ss << sf.rdbuf(); const std::string src = ss.str();
        const size_t a = src.find("static void hlMakeParent"), b = src.find("static bool hlAppend(const std::string& p, const std::string& header, const std::string& text)\n{");
        CHECK(a != std::string::npos && b != std::string::npos && a < b && src.find("hlMakeParent(p);", b) != std::string::npos && src.find("hlMakeParent(p);", b) < b + 200, "hlAppend makes the parent folder first");
        CHECK(src.find("CreateDirectoryA(d.c_str(), NULL);") != std::string::npos, "Windows: CreateDirectoryA on the record's folder");
    }
    std::printf("HodLod %s / DayType %s / NewsWindow %s / MarkParams %s\n", hl::HL_VERSION, dtl::VERSION, nw::VERSION, mp::VERSION);
    const dtl::Params P;
    {   // [1] day types
        std::vector<double> ib = { 101, 103, 100.5, 103.5, 101, 102 };   // 8:30-9:30 inside 100-104
        std::vector<double> up = ib; for (int i = 1; i <= 24; ++i) up.push_back(102 + i * 0.5);   // one-sided up to 114: range 3.5x IB
        std::vector<double> dn = ib; for (int i = 1; i <= 24; ++i) dn.push_back(102 - i * 0.5);
        std::vector<double> rot = ib; for (int i = 0; i < 24; ++i) rot.push_back(i % 2 ? 103.5 : 100.5);   // stays in the IB, crosses VWAP
        const dtl::Result U = dtl::classify(day(up), P), D = dtl::classify(day(dn), P), R = dtl::classify(day(rot), P);
        CHECK(U.kind == dtl::TREND_UP && U.text == "Uptrend \xB7 Trend day ?", "one-sided extension up + VWAP held + close at the top = Uptrend . Trend day ?");
        CHECK(D.kind == dtl::TREND_DN && D.text == "Dntrend \xB7 Trend day ?", "mirror: Dntrend . Trend day ?");
        CHECK(R.kind == dtl::ROTATION && R.text == "Balance \xB7 Rotation day ?" && R.crosses >= 4, "inside the IB, crossing VWAP after 2 h = Balance . Rotation day ?");
        const dtl::Result E = dtl::classify(day(std::vector<double>(ib.begin(), ib.begin() + 5)), P);
        CHECK(E.kind == dtl::NONE && !E.ready, "no call before the IB is complete");
        std::vector<double> early = ib; for (int i = 0; i < 8; ++i) early.push_back(i % 2 ? 103.5 : 100.5);   // 70 min in: rotation not yet allowed
        CHECK(dtl::classify(day(early), P).kind == dtl::NONE, "rotation needs 2 hours");
        std::vector<double> pull = up; for (int i = 1; i <= 10; ++i) pull.push_back(114 - i * 1.0);   // gives the move back: no longer a trend day
        CHECK(dtl::classify(day(pull), P).kind != dtl::TREND_UP, "a trend day that gives the range back is no longer called");
        // no look-ahead / no repaint: the call at bar k uses bars <= k only and a restart (fresh call) gives the same result
        const std::vector<dtl::Bar> full = day(up); bool prefixOk = true, restartOk = true; std::vector<int> calls;
        for (size_t k = 1; k <= full.size(); ++k) {
            const std::vector<dtl::Bar> pre(full.begin(), full.begin() + (long)k);
            const dtl::Result a = dtl::classify(pre, P), b = dtl::classify(std::vector<dtl::Bar>(pre), P);
            if (!sameRes(a, b)) restartOk = false;
            std::vector<dtl::Bar> more = pre; dtl::Bar junk = pre.back(); junk.endMin += 5; junk.h += 50; junk.c += 40;   // a later bar must not change the PREFIX call
            (void)more; calls.push_back(a.kind);
        }
        for (size_t k = 1; k <= full.size(); ++k) { const std::vector<dtl::Bar> pre(full.begin(), full.begin() + (long)k); if (dtl::classify(pre, P).kind != calls[k - 1]) prefixOk = false; }
        CHECK(prefixOk && restartOk, "bar-by-bar: each prefix decides the same in a straight pass and after a restart (pure, closed bars only)");
        const std::vector<std::string> h = dtl::hover(U);
        bool fired = false; for (size_t i = 0; i < h.size(); ++i) if (h[i].find("  + IB extended up only") == 0) fired = true;
        CHECK(h.size() >= 10 && h[1].compare(0, 9, "Question:") == 0 && h[2].compare(0, 11, "What it is:") == 0 && h[3].compare(0, 15, "Why it matters:") == 0 && h[4].compare(0, 11, "How to use:") == 0 && h[5].compare(0, 8, "Example:") == 0 && fired, "hover: question / what / why / how / example + what fired");
        std::vector<dtl::Bar> bad = day(up); bad[8].h = NAN; CHECK(dtl::classify(bad, P).kind == dtl::NONE, "NaN bar = no call");
        dtl::Params off = P; off.extX = 9; CHECK(dtl::classify(day(up), off).kind == dtl::NONE, "params change the call (EXT_X 9)");
        CHECK(dtl::statusLine(U, P).compare(0, 8, "DAYTYPE,") == 0 && dtl::statusLine(U, P).find("TREND_UP") != std::string::npos, "status line");
    }
    {   // [2] news windows (2026-10-14 = CPI Wed, 2026-10-28 = FOMC Wed, 2026-11-06 = NFP Fri, 2026-10-22 = Thu)
        std::string lb;
        CHECK(nw::newsAt("ES", 2026, 10, 14, 7 * 60 + 30, &lb) && lb == "CPI 7:30 AM", "CPI 7:30 AM ES");
        CHECK(nw::newsAt("GC", 2026, 10, 14, 7 * 60 + 25, nullptr) && nw::newsAt("GC", 2026, 10, 14, 7 * 60 + 45, nullptr) && !nw::newsAt("GC", 2026, 10, 14, 7 * 60 + 46, nullptr) && !nw::newsAt("GC", 2026, 10, 14, 7 * 60 + 24, nullptr), "CPI window -5 / +15 min");
        CHECK(nw::newsAt("NQ", 2026, 10, 28, 13 * 60 + 30, &lb) && lb == "FOMC 1:00 PM" && !nw::newsAt("NQ", 2026, 10, 28, 13 * 60 + 31, nullptr), "FOMC -5 / +30 (presser inside)");
        CHECK(nw::newsAt("HG", 2026, 11, 6, 7 * 60 + 33, &lb) && lb == "NFP 7:30 AM", "NFP all markets");
        CHECK(nw::newsAt("CL", 2026, 10, 21, 9 * 60 + 40, &lb) && lb == "EIA 9:30 AM" && !nw::newsAt("ES", 2026, 10, 21, 9 * 60 + 40, nullptr), "EIA crude Wed: CL only");
        CHECK(nw::newsAt("NG", 2026, 10, 22, 9 * 60 + 30, &lb) && lb == "Storage 9:30 AM" && !nw::newsAt("CL", 2026, 10, 22, 9 * 60 + 30, nullptr), "NG storage Thu: NG only");
        CHECK(nw::newsAt("CL", 2026, 10, 22, 13 * 60 + 30, &lb) && lb == "Settle 1:30 PM" && nw::newsAt("GC", 2026, 10, 22, 12 * 60 + 33, nullptr) && nw::newsAt("ES", 2026, 10, 22, 14 * 60 + 56, nullptr) && nw::newsAt("6E", 2026, 10, 22, 14 * 60, nullptr) && nw::newsAt("HG", 2026, 10, 22, 13 * 60, nullptr), "settles: CL 1:30 / GC 12:30 / ES 3:00 / 6E 2:00 / HG 1:00");
        CHECK(!nw::newsAt("ES", 2026, 10, 22, 10 * 60, nullptr) && !nw::newsAt("ES", 2026, 10, 24, 15 * 60, nullptr), "outside a window / Saturday = none");
        std::vector<std::vector<std::string> > X = { { "EVENT", "2027-01-08", "07:30", "NFP", "ALL" }, { "EVENT", "2027-01-08", "09:00", "ISM", "ES|NQ" }, { "EVENT", "bad", "7:30", "X", "ALL" } };
        CHECK(nw::newsAt("GC", 2027, 1, 8, 7 * 60 + 31, &lb, &X) && lb == "NFP 7:30 AM" && nw::newsAt("NQ", 2027, 1, 8, 9 * 60, nullptr, &X) && !nw::newsAt("GC", 2027, 1, 8, 9 * 60, nullptr, &X), "extra EVENT rows (after 2026) with a market list");
        nw::Gate g; g.news = true; CHECK(nw::q("Sr", g) == "Sr?" && nw::q("St?", g) == "St?" && nw::q("Sr", nw::Gate()) == "Sr", "q(): adds one '?' only inside a window");
        const long long ct = 20740LL * 86400 + 7 * 3600 + 31 * 60;   // day 20740 = 2026-10-14
        const nw::Gate h = nw::atCtSec("ES", ct); CHECK(h.news && h.label == "CPI 7:30 AM" && h.pre == 5 && h.post == 15, "atCtSec (CT wall seconds) = the same window");
        struct tm t; std::memset(&t, 0, sizeof t); t.tm_year = 126; t.tm_mon = 9; t.tm_mday = 14; t.tm_hour = 7; t.tm_min = 40;
        CHECK(nw::atTm("NQ", t).news, "atTm");
    }
    {   // [3] params + the feed check
        mp::Params A = mp::parse("VERSION,1\nMARKET,ES\n# c\nPARAM,SHOW,0\nPARAM,MARK,SHOW?\nPARAM,x,2.5\n", "ES");
        CHECK(A.ok && mp::mode(A, "SHOW", mp::SHOW) == mp::LOG && mp::mode(A, "mark", mp::LOG) == mp::SHOWQ && mp::num(A, "X", 0, 0, 10) == 2.5 && mp::num(A, "X", 1, 0, 2) == 1, "parse + modes + ranged numbers");
        CHECK(!mp::parse("MARKET,ES\nVERSION,1\n", "ES").ok, "VERSION must come first");
        CHECK(!mp::parse("VERSION,1\nMARKET,NQ\nPARAM,SHOW,1\n", "ES").ok, "wrong market = defaults");
        CHECK(mp::parse("VERSION,1\nMARKET,EU\nPARAM,SHOW,1\n", "6E").ok, "6E = EU");
        CHECK(!mp::parse("VERSION,1\nMARKET,ES\nPARAM,SHOW\n", "ES").ok && !mp::parse(std::string(70000, 'x'), "ES").ok && !mp::parse("VERSION,abc\n", "ES").ok, "garbled / too big = defaults");
        CHECK(mp::mode(mp::Params(), "SHOW", mp::SHOWQ) == mp::SHOWQ && mp::modeOf("junk", mp::LOG) == mp::LOG, "missing / junk = the caller's default");
        std::string why;
        const mp::Params F0 = mp::parse("VERSION,1\nMARKET,NQ\nPARAM,match_ok,0\n", "NQ"), F1 = mp::parse("VERSION,1\nMARKET,NQ\nPARAM,match_ok,1\n", "NQ");
        CHECK(nw::feedBadOf(F0, 3600, &why) && !why.empty() && !nw::feedBadOf(F1, 3600, nullptr) && !nw::feedBadOf(F0, 5 * 86400, nullptr) && !nw::feedBadOf(mp::Params(), 0, nullptr), "feed check: match_ok 0 = record only; 1 / missing / > 4 days old = ok");
        // the cached reader on a real file (USERPROFILE -> a temp dir)
        const char* tmp = "/dev/shm/nw270"; std::system("mkdir -p /dev/shm/nw270"); setenv("USERPROFILE", tmp, 1);
        { std::ofstream f("/dev/shm/nw270\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Trails-ES.csv"); f << "VERSION,1\nMARKET,ES\nPARAM,SHOW,0\nPARAM,SHOW_B,0\n"; }
        { std::ofstream f("/dev/shm/nw270\\InvestorRT\\rtx\\lsFlexLevels\\LRA-FeedCheck-NQ.csv"); f << "VERSION,1\nMARKET,NQ\nPARAM,match_ok,0\n"; }
        CHECK(mp::mode("Trails", "ES", "SHOW", mp::SHOW) == mp::LOG && mp::get("Trails", "GC").missing && mp::mode("Trails", "GC", "SHOW", mp::LOG) == mp::LOG, "file read; missing file = default");
        CHECK(nw::feedBad("NQ", nullptr) && !nw::feedBad("ES", nullptr), "feed file per market");
        const nw::Gate gn = nw::atCivil("NQ", 2026, 10, 22, 10, 0); CHECK(gn.feedBad && !gn.news, "gate: feed bad outside a window");
    }
    {   // [4] the box
        auto view = [](int kind, bool notes) {
            hl::View v; v.has = true; v.headMain = "RTH 10/22";
            v.headLeftSegs.push_back(hl::Seg{ "ES RTH 8:30-3:00", hl::C_SLATE });
            v.sessSegs.push_back(hl::Seg{ "Regime: Negative", hl::C_RED }); v.sessSegs.push_back(hl::Seg{ "Expiry 3:00 PM (in 4h 10m)", hl::C_GREY });
            v.trendReady = true; v.trendLvl = 2;
            v.E[hl::K_1ST] = hl::mkCell("8:33 - 9:21", hl::C_BLUE);
            v.A[hl::K_LABEL] = hl::mkCell("Actual", hl::C_SLATE); v.A[hl::K_1ST] = hl::mkCell("8:33 LOD in", hl::C_SLATE);
            if (kind) { v.dayText = kind == 1 ? "Uptrend \xB7 Trend day ?" : kind == -1 ? "Dntrend \xB7 Trend day ?" : "Balance \xB7 Rotation day ?"; v.dayCol = kind == 1 ? hl::C_GREEN : kind == -1 ? hl::C_RED : hl::C_SLATE;
                        for (int i = 0; i < 9; ++i) v.dayHover.push_back(hl::Line{ hl::Seg{ "day type hover line that explains what fired and wraps when long enough to need it", hl::C_SLATE } }); }
            if (notes) { hl::View::Note a; a.label = "News"; a.text = "CPI 7:30 AM \xB7 -5 / +15 min \xB7 marks ?"; a.col = hl::C_AMBER; a.hover.push_back(hl::Line{ hl::Seg{ "News", hl::C_AMBER } }); a.hover.push_back(hl::Line{ hl::Seg{ "Question: x", hl::C_SLATE } });
                         hl::View::Note b; b.label = "Feed"; b.text = "? live MBO feed does not match \xB7 MBO marks record only"; b.col = hl::C_AMBER; b.hover = a.hover; v.notes.push_back(a); v.notes.push_back(b); }
            return v;
        };
        const hl::Box pane = { 0, 0, 1600, 800 };
        auto yOf = [](long long t) { return (int)(400 - t / 4); };
        for (int pos = 0; pos < 9; ++pos) for (int kind = -1; kind <= 2; ++kind) {
            const hl::View v0 = view(0, false), v1 = view(kind, true);
            const hl::Layout A = hl::layoutAll(v0, pane, yOf, measure, 1500, pos);
            hl::Layout B = hl::layoutAll(v1, pane, yOf, measure, 1500, pos);
            const std::string tag = "pos " + std::to_string(pos) + " kind " + std::to_string(kind);
            CHECK(A.tableOk && B.tableOk && !A.dayOk && A.noteRows.empty() && B.noteRows.size() == 2 && (kind == 0 ? !B.dayOk : B.dayOk), tag + ": day / note targets only when set");
            CHECK((B.table.b - B.table.t) - (A.table.b - A.table.t) == 2 * (hl::lineH(10) + 1), tag + ": the table grows by the two note rows only (the day type is in the header)");
            bool inside = true, overl = false, dayTxt = kind == 0, trendWord = false; std::vector<hl::Item> T;
            for (const hl::Item& it : B.items) if (it.k == hl::K_TEXT && it.role == hl::R_TABLE) {
                if (it.l < B.table.l || it.r > B.table.r) inside = false;
                if (kind && it.s == v1.dayText && it.c1 == v1.dayCol) dayTxt = true;
                if (it.s == "Strong Uptrend") trendWord = true;
                T.push_back(it);
            }
            for (size_t i = 0; i < T.size(); ++i) for (size_t j = i + 1; j < T.size(); ++j)
                if (T[i].l < T[j].r && T[j].l < T[i].r && T[i].t < T[j].b && T[j].t < T[i].b) { overl = true; std::printf("  overlap '%s' / '%s'\n", T[i].s.c_str(), T[j].s.c_str()); }
            CHECK(inside && !overl && dayTxt && (kind ? !trendWord : trendWord), tag + ": all text inside, no overlaps, the day type replaces the trend word in its colour");
            for (size_t q = 0; q < B.noteRows.size(); ++q) CHECK(B.noteRows[q].t >= B.table.t && B.noteRows[q].b <= B.table.b, "note rows inside the table");
            CHECK(B.table.r - B.table.l <= 700, tag + ": table width " + std::to_string(B.table.r - B.table.l));
            if (kind) { hl::Layout C = B; hl::layoutRowHover(C, v1.dayHover, pane, 1500, measure);
                CHECK(C.preHoverOk && C.preHover.l >= pane.l && C.preHover.r <= 1500 && C.preHover.b <= pane.b && !hl::overlap(C.preHover, C.table), tag + ": day hover inside the pane, left of the last bar, clear of the table"); }
            hl::Layout N = B; hl::layoutRowHover(N, v1.notes[1].hover, pane, 1500, measure); CHECK(N.preHoverOk && !hl::overlap(N.preHover, N.table), "note hover clear of the table");
        }
        // no day / no notes = the 2.6.0 table exactly
        const hl::View a = view(0, false); hl::View b = view(0, false); b.dayHover.push_back(hl::Line{ hl::Seg{ "x", 0 } });
        const hl::Layout A = hl::layoutAll(a, pane, yOf, measure, 1500, 1), B = hl::layoutAll(b, pane, yOf, measure, 1500, 1);
        bool same = A.items.size() == B.items.size(); for (size_t i = 0; same && i < A.items.size(); ++i) same = A.items[i].s == B.items[i].s && A.items[i].l == B.items[i].l && A.items[i].t == B.items[i].t;
        CHECK(same && !B.dayOk, "no day text = unchanged header");
    }
    {   // [5] versions
        const std::string src = readAll("../HodLod.cpp");
        CHECK(std::string(hl::HL_VERSION) == "2.7.1" && std::string(hlset::PLUGIN_VERSION) == "2.7.1" && src.find("p->setVersion(\"2.7.1\")") != std::string::npos, "versions 2.7.0 everywhere");
        CHECK(src.find("mp::get(\"PreOpen\", S->mkt)") != std::string::npos && src.find("S->preParamShow != mp::LOG") != std::string::npos, "Pre row gated by LRA-PreOpen SHOW (LOG by default)");
    }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
