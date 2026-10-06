/********************************************************************************
 *  DealerLogic.h  --  the testable half of lsDealerProfile + lsDealerRead (no IRT SDK)
 *
 *  Both plugins read %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Dealer-<MKT>.csv, written every 5 min by the
 *  LRA Reader (level-reversal-analytics: analytics/lra/dealer_irt.py). Every decision is made there; the plugins only
 *  draw. Fields are separated by '|'. Rows:
 *     VERSION|1.0            ASOF|<CT sec-of-day>|<yyyy-mm-dd>      MARKET|GC      PRICE|<px>|<em>
 *     BOOK|<gamma at price, fut per 0.1 EM>|FUEL|WALL|short|long
 *     NODE|<strike>|<gamma on arrival>|<lo>|<hi>|<snapshot>|<delta flow, + = BUY>|<far 0/1>|<gp>|<dp>|<live>|<net GEX $/pt>|<0DTE net GEX $/pt>
 *          (file 1.5 / Profile 2.0, 2026-10-02: the last two = MenthorQ's own net GEX at the strike, all expiries and today's 0DTE)
 *     LEVEL|<key label>|<px>|<phase 0..4>|<context text>|S|R           (LEVEL|NONE = nothing near)
 *     WALLHDR|<title>|<score>|<G/R/A/Y/N>     WALLROW|<name>|<meter -100..100 or NA>|<label>|ok|bad|watch|wait|<why>
 *     FUELHDR|FUEL|<score>|<col>               FUELROW|... (as WALLROW)
 *     TRADE|<key>|<value>|<col>   FLAG|<text>|<col>   DO|<text>   BOOKLINE|<text>   LEARN|1
 *  (file 1.6 / Read 2.0 / Sig 1.0, 2026-10-02 - lra/forced.py) forced futures by cause, gamma strength, the reasons:
 *     FSUM|<from HH:MM>|<to>|<gamma>|<vanna>|<charm>|<positions>|<futures traded>|<px from>|<px to>|<iv from %>|<iv to %>
 *     FWIN|<from>|<to>|<gamma>|<vanna>|<charm>|<positions>|<futures traded>          (every grid window, last 40)
 *     GSTR|<HH:MM>|<px>|<iv %>|<gamma/pt>|<speed -0.25 EM>|<speed +0.25 EM>|<zomma IV+1>|<zomma IV-1>|<color +30m>|<color +60m>
 *     SIG|<yyyy-mm-dd HH:MM:SS CT>|<code SC Van Chm PutS CallB CallS PutB>|<B = dealers buy / S = sell>|<value>|<text>
 *     (file 1.9) SIG = the Turn's reasons only, timed inside the turn; LSIG = the same fields for the forced ledger's marks
 *  (file 1.7 / Read 2.1 / Sig 1.1, lra/stages.py) the Read in stages - 1 APPROACH, 2 TURN STARTS, 3 TURN CONTINUES:
 *     STAGE|<now 0-3>|<L/S>|<level>|<extreme>|<extreme bar HH:MM>       STG|<n>|<title>|<sub>       SSUM|<n>|<summary>
 *     SR|<n>|<HH:MM known>|<code>|<B/S/N>|<value>|<chip:col,chip:col>|<text>   (a code ending in ? = an educated guess)
 *  (file 1.8 / Read 3.0, 2026-10-02 evening - lra/turn.py) the Turn: one read, no stages, reasons in sentences in time order:
 *     TURN|<L/S>|<header>|<window from HH:MM>|<window to HH:MM>
 *     TR|<n>|<HH:MM>|<Rev Reason tag; ' ?' = a guess>|<'' / guess / lean>|<second source e.g. GLD or ''>|<sentence>
 ********************************************************************************/
#pragma once
#include <string>
#include <sys/stat.h>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cctype>

// (2026-10-03) number boxes in the settings window: an explicit width - the default (0) drew them so narrow they showed "T"
#ifndef NUMW
#define NUMW 50
#endif

namespace dl {

struct Node { float k = 0, g = 0, lo = 0, hi = 0, snap = 0, d = 0, gp = -1, dp = -1, usd = 0, usd0 = 0; bool far = false, live = false, hasUsd = false; };   // live: COVER confirmed dealers trading here   // gp / dp: % of a normal 15 min of volume (-1 = none)
struct Trig { std::string group, name, val, note; bool on = false; };
struct Row  { std::string n, lab, st, why; bool hasV = false; float v = 0; };
struct KV   { std::string k, v; char col = 'W'; };

struct Data {
    std::string ver, market;
    std::string srcBook, srcMode, srcAt; float srcRatio = 1.0f;   // (file 2.0) SRC: the book the Profile bars come from (SPX / QQQ / the market itself)
    double asofSo = -1; int y = 0, mo = 0, d = 0;
    float px = 0, em = 0; bool hasPrice = false;
    float book = 0; bool hasBook = false;
    std::vector<Node> nodes;
    bool hasLevel = false; std::string lvlLabel, ctx; float lvlPx = 0; int phase = 0; char side = 'R';
    std::string wallTitle, wallScore; char wallCol = 'N'; std::vector<Row> wall;
    std::string fuelScore; char fuelCol = 'N'; std::vector<Row> fuel;
    std::vector<KV> trade; std::vector<KV> flags;
    std::vector<Trig> trig;
    std::string doText, bookLine; bool learn = false;
    std::vector<std::string> next;   // (2026-09-30) NEXT|above|below: what the Read watches when there is no level
    std::string fuelTitle, keyName, mqName, sideWord, noWall;   // (file 1.2) the price level then the MenthorQ level; NO WALL text
    std::vector<Row> xrows;                                       // (Read 1.2.6) TAPE / CHARM
    std::string aNow, aTime, aBias; char aBiasCol = 'N';          // (Read 1.2.6) the Analyst: NOW paragraph + bias
    struct Scn { std::string title, act, odds, l1, l2, l3; char col = 'N'; };
    std::vector<Scn> scn;                                         // the Analyst's 3 scenarios
    // (file 1.3 / Read 1.4 / Profile 1.4, mockup v27) Dealers + Magnet paragraphs; the Read's Key Level, checklist, STOP / TGT;
    // the Profile's tags (KEY / TGT / MAG)
    std::string aDeal, aMag, keyLvl, stopTxt, tgtTxt;
    struct Chk { std::string n, name, st, val; };
    std::vector<Chk> chk;
    struct Tag { float k = 0; std::string text; char col = 'W'; };
    std::vector<Tag> tags;
    // (file 1.4 / Read 1.5, mockups v28 + v29) the level VERDICT under the Key Level: will the level hold after the reclaim so
    // the stop beyond the sweep is not hit? The Reader's Strength (lra/strength.py): STRONG / OK / WEAK / BROKEN, points / of
    std::string vLabel, vText; int vPts = -1, vOf = 0;
    // (file 1.5 / Read 1.6.0, Rassul 2026-10-01: one decision system) the verdict tally, its votes, the dealer lines, the
    // plan and the summary - written by lra/verdict.py, the same as the Reader panel. q = ok / bad / neutral / wait
    struct Vote { std::string n, q, v; };
    bool hasV2 = false; std::string v2Word, v2Side; int v2Sup = 0, v2Meas = 0, v2Wait = 0, v2Opp = 0;
    std::vector<Vote> votes, dlines, plan;
    std::string bookState, bookPct, summary;
    // (file 1.6) forced futures: what made dealers buy / sell between grids, gamma strength, and the Dealer Sig reasons
    struct FWin { std::string ta, tb, pa, pb; float gam = 0, van = 0, cha = 0, pos = 0, fut = 0, iva = 0, ivb = 0; };
    bool hasFsum = false; FWin fsum; std::vector<FWin> fwin;
    struct GStr { std::string t, px; float iv = 0, g = 0, sdn = 0, sup = 0, zp = 0, zm = 0, c30 = 0, c60 = 0; };
    bool hasGstr = false; GStr gstr;
    struct Sig { int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0; std::string code, text; char side = 'B'; float v = 0; };
    std::vector<Sig> sigs;
    std::vector<Sig> lsigs;          // (file 1.9) the forced ledger's marks - Dealer Sig 1.4 draws them only when asked
    std::string ferr;
    // (file 1.7) the stages
    struct Chip { std::string name; char col = 'N'; };
    struct SRow { int n = 0; std::string t, code, val, text; char side = 'N'; std::vector<Chip> chips; };
    struct Stg { int n = 0; std::string title, sub, sum; };
    bool hasStage = false; int stageNow = 0; char stageSide = 'L'; std::string stageLvl, stageExt, stageExtT;
    std::vector<Stg> stgs; std::vector<SRow> srows;
    // (file 1.8 / Read 3.0) the Turn
    struct TRow { int n = 0; std::string t, tag, kind, src, text; char sec = 'O'; };   // (Read 4.0) sec O = options, F = footprint
    bool hasTurn = false; char turnSide = 'L'; std::string turnHead, turnFrom, turnTo, terr;
    std::vector<TRow> trows;
};

// (Profile 1.3.7 / Read 1.2.5, 2026-09-30) the file's identity (modified time + size): the indicators re-read and re-parse the
// dealer file ONLY when this changes - not on every repaint (IRT showed "Not Responding" while every draw re-read it)
inline long long fileStamp(const std::string& path)
{
#ifdef _WIN32
    struct _stat64 st; if (_stat64(path.c_str(), &st) != 0) return -1;
#else
    struct stat st; if (stat(path.c_str(), &st) != 0) return -1;
#endif
    return (long long)st.st_mtime * 1000003LL + (long long)st.st_size;
}

inline std::vector<std::string> split(const std::string& line, char sep = '|')
{
    std::vector<std::string> t; std::string cur;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == '\r' || c == '\n') continue;
        if (c == sep) { t.push_back(cur); cur.clear(); } else cur += c;
    }
    t.push_back(cur);
    return t;
}
inline float f(const std::string& s) { return (float)atof(s.c_str()); }
inline char col1(const std::string& s) { return s.empty() ? 'W' : s[0]; }

inline bool parseRow(const std::vector<std::string>& t, Row& r)
{
    if (t.size() < 6) return false;
    r.n = t[1]; r.hasV = !(t[2] == "NA" || t[2].empty()); r.v = r.hasV ? f(t[2]) : 0.0f;
    if (r.v > 100) r.v = 100;
    if (r.v < -100) r.v = -100;
    r.lab = t[3]; r.st = t[4]; r.why = t[5];
    return true;
}

// one line into the data; returns false for a line it does not know (ignored)
inline bool parseLine(Data& D, const std::string& line)
{
    if (line.empty() || line[0] == '#') return false;
    std::vector<std::string> t = split(line);
    const std::string& k = t[0];
    if (k == "VERSION" && t.size() >= 2) { D.ver = t[1]; return true; }
    if (k == "MARKET" && t.size() >= 2) { D.market = t[1]; return true; }
    if (k == "SRC" && t.size() >= 2) { D.srcBook = t[1]; if (t.size() >= 3) D.srcRatio = f(t[2]); if (t.size() >= 4) D.srcMode = t[3]; if (t.size() >= 5) D.srcAt = t[4]; return true; }
    if (k == "ASOF" && t.size() >= 2) {
        D.asofSo = atof(t[1].c_str());
        if (t.size() >= 3) { int a = 0, b = 0, c = 0; if (sscanf(t[2].c_str(), "%d-%d-%d", &a, &b, &c) == 3) { D.y = a; D.mo = b; D.d = c; } }
        return true;
    }
    if (k == "PRICE" && t.size() >= 3) { D.px = f(t[1]); D.em = f(t[2]); D.hasPrice = D.px > 0 && D.em > 0; return true; }
    if (k == "BOOK" && t.size() >= 2) { D.book = f(t[1]); D.hasBook = true; return true; }
    if (k == "NODE" && t.size() >= 8) {
        Node n; n.k = f(t[1]); n.g = f(t[2]); n.lo = f(t[3]); n.hi = f(t[4]); n.snap = f(t[5]); n.d = f(t[6]); n.far = t[7] == "1";
        if (t.size() >= 10) { if (!t[8].empty()) n.gp = f(t[8]); if (!t[9].empty()) n.dp = f(t[9]); }   // (1.1) the strength share
        if (t.size() >= 11) n.live = t[10] == "1";                                                        // (1.2) the live mark
        if (t.size() >= 13 && !t[11].empty()) { n.usd = f(t[11]); n.usd0 = f(t[12]); n.hasUsd = true; }   // (2.0) net GEX per strike
        if (n.k > 0) { D.nodes.push_back(n); return true; }
        return false;
    }
    if (k == "LEVEL") {
        if (t.size() >= 2 && t[1] == "NONE") { D.hasLevel = false; return true; }
        if (t.size() < 6) return false;
        D.hasLevel = true; D.lvlLabel = t[1]; D.lvlPx = f(t[2]); D.phase = atoi(t[3].c_str()); D.ctx = t[4];
        if (D.phase < 0) D.phase = 0;
        if (D.phase > 5) D.phase = 5;
        D.side = (t.size() >= 6 && !t[5].empty()) ? t[5][0] : 'R';
        return true;
    }
    if (k == "WALLHDR" && t.size() >= 4) { D.wallTitle = t[1]; D.wallScore = t[2]; D.wallCol = col1(t[3]); return true; }
    if (k == "FUELHDR" && t.size() >= 4) { D.fuelTitle = t[1]; D.fuelScore = t[2]; D.fuelCol = col1(t[3]); return true; }
    if (k == "NAMES" && t.size() >= 4) { D.keyName = t[1]; D.mqName = t[2]; D.sideWord = t[3]; return true; }
    if (k == "NOWALL" && t.size() >= 2) { D.noWall = t[1]; return true; }
    if (k == "XROW") { Row r; if (!parseRow(t, r)) return false; D.xrows.push_back(r); return true; }
    if (k == "ANOW" && t.size() >= 2) { D.aNow = t[1]; D.aTime = t.size() >= 3 ? t[2] : ""; return true; }
    if (k == "ABIAS" && t.size() >= 3) { D.aBias = t[1]; D.aBiasCol = col1(t[2]); return true; }
    if (k == "ASCN" && t.size() >= 7) { Data::Scn s; s.title = t[1]; s.act = t[2]; s.odds = t[3]; s.col = col1(t[4]); s.l1 = t[5]; s.l2 = t[6]; if (t.size() >= 8) s.l3 = t[7]; D.scn.push_back(s); return true; }
    if (k == "WALLROW") { Row r; if (!parseRow(t, r)) return false; D.wall.push_back(r); return true; }
    if (k == "FUELROW") { Row r; if (!parseRow(t, r)) return false; D.fuel.push_back(r); return true; }
    if (k == "TRADE" && t.size() >= 4) { KV v; v.k = t[1]; v.v = t[2]; v.col = col1(t[3]); D.trade.push_back(v); return true; }
    if (k == "FLAG" && t.size() >= 3) { KV v; v.k = t[1]; v.col = col1(t[2]); D.flags.push_back(v); return true; }
    if (k == "DO" && t.size() >= 2) { D.doText = t[1]; return true; }
    if (k == "TRIG" && t.size() >= 6) { Trig g; g.group = t[1]; g.name = t[2]; g.on = t[3] == "1"; g.val = t[4]; g.note = t[5]; D.trig.push_back(g); return true; }
    if (k == "BOOKLINE" && t.size() >= 2) { D.bookLine = t[1]; return true; }
    if (k == "NEXT") { D.next.assign(t.begin() + 1, t.end()); return true; }
    if (k == "ADEAL" && t.size() >= 2) { D.aDeal = t[1]; return true; }
    if (k == "AMAG" && t.size() >= 2) { D.aMag = t[1]; return true; }
    if (k == "KEYLVL" && t.size() >= 2) { D.keyLvl = t[1]; return true; }
    if (k == "CHK" && t.size() >= 5) { Data::Chk c; c.n = t[1]; c.name = t[2]; c.st = t[3]; c.val = t[4]; D.chk.push_back(c); return true; }
    if (k == "VERDICT" && t.size() >= 5) { D.vLabel = t[1]; D.vPts = atoi(t[2].c_str()); D.vOf = atoi(t[3].c_str()); D.vText = t[4]; return true; }
    if (k == "STOPTGT" && t.size() >= 3) { D.stopTxt = t[1]; D.tgtTxt = t[2]; return true; }
    if (k == "TAG" && t.size() >= 4) { Data::Tag g; g.k = f(t[1]); g.text = t[2]; g.col = col1(t[3]); if (g.k > 0) { D.tags.push_back(g); return true; } return false; }
    if (k == "LEARN" && t.size() >= 2) { D.learn = t[1] == "1"; return true; }
    if (k == "VERDICT2" && t.size() >= 7) { D.hasV2 = true; D.v2Word = t[1]; D.v2Side = t[2]; D.v2Sup = atoi(t[3].c_str()); D.v2Meas = atoi(t[4].c_str()); D.v2Wait = atoi(t[5].c_str()); D.v2Opp = atoi(t[6].c_str()); return true; }
    if ((k == "VOTE" || k == "DLINE" || k == "PLAN") && t.size() >= 4) {
        Data::Vote v; v.n = t[1]; v.q = t[2]; v.v = t[3];
        (k == "VOTE" ? D.votes : k == "DLINE" ? D.dlines : D.plan).push_back(v); return true;
    }
    if (k == "BOOKCHK" && t.size() >= 3) { D.bookState = t[1]; D.bookPct = t[2]; return true; }
    if (k == "SUMMARY" && t.size() >= 2) { D.summary = t[1]; return true; }
    if ((k == "FSUM" && t.size() >= 12) || (k == "FWIN" && t.size() >= 8)) {
        Data::FWin w; w.ta = t[1]; w.tb = t[2]; w.gam = f(t[3]); w.van = f(t[4]); w.cha = f(t[5]); w.pos = f(t[6]); w.fut = f(t[7]);
        if (k == "FSUM") { w.pa = t[8]; w.pb = t[9]; w.iva = f(t[10]); w.ivb = f(t[11]); D.fsum = w; D.hasFsum = true; }
        else D.fwin.push_back(w);
        return true;
    }
    if (k == "GSTR" && t.size() >= 11) {
        Data::GStr g; g.t = t[1]; g.px = t[2]; g.iv = f(t[3]); g.g = f(t[4]); g.sdn = f(t[5]); g.sup = f(t[6]); g.zp = f(t[7]); g.zm = f(t[8]);
        g.c30 = f(t[9]); g.c60 = f(t[10]); D.gstr = g; D.hasGstr = true; return true;
    }
    if ((k == "SIG" || k == "LSIG") && t.size() >= 6) {
        Data::Sig g;
        if (sscanf(t[1].c_str(), "%d-%d-%d %d:%d:%d", &g.y, &g.mo, &g.d, &g.h, &g.mi, &g.s) < 5 || t[2].empty()) return false;
        g.code = t[2]; g.side = t[3] == "S" ? 'S' : 'B'; g.v = f(t[4]); g.text = t[5]; (k == "LSIG" ? D.lsigs : D.sigs).push_back(g); return true;
    }
    if (k == "FERR" && t.size() >= 2) { D.ferr = t[1]; return true; }
    if (k == "TURN" && t.size() >= 3) { D.hasTurn = true; D.turnSide = t[1] == "S" ? 'S' : 'L'; D.turnHead = t[2];
        D.turnFrom = t.size() > 3 ? t[3] : ""; D.turnTo = t.size() > 4 ? t[4] : ""; return true; }
    if (k == "TR" && t.size() >= 7) { Data::TRow r; r.n = atoi(t[1].c_str()); r.t = t[2]; r.tag = t[3]; r.kind = t[4]; r.src = t[5]; r.text = t[6]; if (t.size() >= 8 && !t[7].empty()) r.sec = t[7][0];
        D.trows.push_back(r); return true; }
    if (k == "TERR" && t.size() >= 2) { D.terr = t[1]; return true; }
    if (k == "STAGE" && t.size() >= 6) { D.hasStage = true; D.stageNow = atoi(t[1].c_str()); D.stageSide = t[2] == "S" ? 'S' : 'L'; D.stageLvl = t[3]; D.stageExt = t[4]; D.stageExtT = t[5]; return true; }
    if (k == "STG" && t.size() >= 4) { Data::Stg g; g.n = atoi(t[1].c_str()); g.title = t[2]; g.sub = t[3]; D.stgs.push_back(g); return true; }
    if (k == "SSUM" && t.size() >= 3) { int n = atoi(t[1].c_str()); for (size_t i = 0; i < D.stgs.size(); i++) if (D.stgs[i].n == n) D.stgs[i].sum = t[2]; return true; }
    if (k == "SR" && t.size() >= 8) {
        Data::SRow r; r.n = atoi(t[1].c_str()); r.t = t[2]; r.code = t[3]; r.side = t[4].empty() ? 'N' : t[4][0]; r.val = t[5]; r.text = t[7];
        std::vector<std::string> cs = split(t[6], ',');
        for (size_t i = 0; i < cs.size(); i++) {
            if (cs[i].empty()) continue;
            Data::Chip c; size_t p = cs[i].rfind(':');
            if (p == std::string::npos) c.name = cs[i]; else { c.name = cs[i].substr(0, p); c.col = p + 1 < cs[i].size() ? cs[i][p + 1] : 'N'; }
            r.chips.push_back(c);
        }
        D.srows.push_back(r); return true;
    }
    return false;
}

// (Read 2.0) "+589" / "-10" / "0" - the sign says who: + dealers BUY futures, - dealers SELL
inline std::string signedN(float v) { char b[24]; long n = std::lround(v); if (n > 0) snprintf(b, sizeof(b), "+%ld", n); else snprintf(b, sizeof(b), "%ld", n); return b; }
// share of the futures traded, "8.6%" ("" when no volume)
inline std::string shareTxt(float v, float fut) { if (fut <= 0) return ""; char b[16]; float p = 100.0f * std::fabs(v) / fut; snprintf(b, sizeof(b), p < 10 ? "%.1f%%" : "%.0f%%", p); return b; }
// how much gamma changes, in % of gamma now: speed / zomma / color ("-23%" = dealers trade 23% less per point)
inline std::string gChange(float now, float other) { if (std::fabs(now) < 1e-6f) return ""; char b[16]; snprintf(b, sizeof(b), "%+.0f%%", 100.0f * (std::fabs(other) - std::fabs(now)) / std::fabs(now)); return b; }
// the reasons a chart bar (local y/mo/d, sec of day) shows: every SIG known at or after the bar start and before the next bar
inline bool sigInBar(const Data::Sig& g, int y, int mo, int d, double so0, double so1)
{
    if (g.y != y || g.mo != mo || g.d != d) return false;
    double so = g.h * 3600.0 + g.mi * 60.0 + g.s;
    return so >= so0 && so < so1;
}

// (Sig 1.1) a code ending in '?' is an educated guess (drawn with a dashed frame)
// (Read 3.0) word-wrap a sentence into lines no wider than maxW, measured by width(s) (the plugin passes getTextWidth)
template <class W> inline std::vector<std::string> wrapWords(const std::string& text, float maxW, W width)
{
    std::vector<std::string> out; std::string line, word; std::stringstream ss(text);
    while (ss >> word) {
        std::string cand = line.empty() ? word : line + " " + word;
        if (!line.empty() && width(cand) > maxW) { out.push_back(line); line = word; }
        else line = cand;
    }
    if (!line.empty()) out.push_back(line);
    return out;
}
// (Read 3.0) the tag's colour - the Rev Reason colours used in the mockups (r, g, b)
inline void tagColour(const std::string& tag0, int& r, int& g, int& b)
{
    std::string tag = tag0; if (tag.size() > 2 && tag.substr(tag.size() - 2) == " ?") tag = tag.substr(0, tag.size() - 2);
    r = 229; g = 231; b = 235;
    if (tag == "Exhaustion") { r = 245; g = 158; b = 11; }
    else if (tag == "Tape") { r = 96; g = 165; b = 250; }
    else if (tag == "Short cover") { r = 192; g = 140; b = 255; }
    else if (tag == "New bets" || tag == "Profit taking") { r = 74; g = 222; b = 128; }
    else if (tag == "IV move") { r = 167; g = 139; b = 250; }
    else if (tag == "Pin" || tag == "Pin Exp") { r = 34; g = 211; b = 238; }
    else if (tag == "Cushion") { r = 148; g = 163; b = 184; }
    else if (tag.find("0DTE") == 0 || tag.find("Fear") == 0) { r = 251; g = 113; b = 133; }
}

// (Sig 1.4) stack a label so it never touches one already placed: move it away from the bar (down for a buy, up for a sell)
// a row at a time until its box is free. Boxes are {left, top, right, bottom}.
struct Box { int l, t, r, b; };
inline bool overlaps(const Box& a, const Box& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
inline Box placeFree(std::vector<Box>& used, Box x, int step, bool down, int maxRows = 12)
{
    for (int k = 0; k < maxRows; k++) {
        bool hit = false;
        for (size_t i = 0; i < used.size(); i++) if (overlaps(used[i], x)) { hit = true; break; }
        if (!hit) break;
        int d = down ? step : -step; x.t += d; x.b += d;
    }
    used.push_back(x);
    return x;
}

inline bool isGuess(const std::string& code) { return !code.empty() && code[code.size() - 1] == '?'; }

inline Data parseText(const std::string& text)
{
    Data D; std::stringstream ss(text); std::string line;
    while (std::getline(ss, line)) parseLine(D, line);
    return D;
}

// The chart's market from IRT's root symbol (EPZ26 -> "EP", GCEZ26 -> "GCE" ...). Markets: ES NQ CL GC HG NG EU.
inline std::string marketForRoot(const std::string& rootIn)
{
    std::string r; for (size_t i = 0; i < rootIn.size(); i++) r += (char)toupper((unsigned char)rootIn[i]);
    // (1.0.1) "contains", checked in this order: his GC chart's root is "QGC" - 1.0 matched its "QG" prefix to NG
    auto has = [&](const char* k) { return r.find(k) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return "";
}
static const char* MARKETS[] = { "Auto", "ES", "NQ", "CL", "GC", "HG", "NG", "EU" };
inline std::string marketFor(int setting, const std::string& root) { return (setting >= 1 && setting <= 7) ? MARKETS[setting] : marketForRoot(root); }

// Minutes since the file was written (CT sec-of-day vs the chart's clock); wraps over midnight
inline double staleMin(double asofSo, double localSo)
{
    if (asofSo < 0) return 0;
    double d = localSo - asofSo; if (d < -600) d += 86400; if (d < 0) d = 0;
    return d / 60.0;
}

// Contract offset: the chart may be another contract than MenthorQ's front future. Offset = the chart's close at the
// file's minute - the file's PRICE, refused when more than 3% (a wrong market).
inline bool offsetFor(float chartClose, float filePx, float& off)
{
    off = 0;
    if (!(chartClose > 0) || !(filePx > 0)) return false;
    float d = chartClose - filePx;
    if (std::fabs(d) > 0.03f * filePx) return false;
    off = d; return true;
}

// Profile scale: the largest |gamma| drawn (on arrival, whisker top, snapshot) and the largest |delta|
inline void scales(const Data& D, float& gmax, float& dmax)
{
    gmax = 1e-6f; dmax = 1e-6f;
    for (size_t i = 0; i < D.nodes.size(); i++) {
        const Node& n = D.nodes[i];
        float a = std::fabs(n.g); if (std::fabs(n.hi) > a) a = std::fabs(n.hi); if (std::fabs(n.lo) > a) a = std::fabs(n.lo); if (std::fabs(n.snap) > a) a = std::fabs(n.snap);
        if (a > gmax) gmax = a;
        if (std::fabs(n.d) > dmax) dmax = std::fabs(n.d);
    }
}
inline int barLen(float v, float vmax, int width) { if (vmax <= 0) return 0; float r = std::fabs(v) / vmax; if (r > 1) r = 1; return (int)(r * width + 0.5f); }

// (Profile 2.0, Rassul 2026-10-02: "there is no way it is supposed to look like this") the profile = one bar per strike of
// MenthorQ's net GEX ($ of dealer delta change per 1-pt move): red = dealers short gamma, green = long; the scale is the
// largest |net GEX| among the strikes on screen
inline float usdMax(const std::vector<Node>& v) { float m = 1e-6f; for (size_t i = 0; i < v.size(); i++) if (v[i].hasUsd && std::fabs(v[i].usd) > m) m = std::fabs(v[i].usd); return m; }
inline bool anyUsd(const Data& D) { for (size_t i = 0; i < D.nodes.size(); i++) if (D.nodes[i].hasUsd) return true; return false; }
// "-3.9M", "420K", "8K", "-950", "-30.7B" - MenthorQ's own way of writing it (2.0.2: B for billions - EU / NG per 1.00 move)
inline std::string usdLabel(float v)
{
    char b[32]; float a = std::fabs(v);
    if (a >= 1e9f) snprintf(b, sizeof(b), "%s%.1fB", v < 0 ? "-" : "", a / 1e9f);
    else if (a >= 1e6f) snprintf(b, sizeof(b), "%s%.1fM", v < 0 ? "-" : "", a / 1e6f);
    else if (a >= 1e3f) snprintf(b, sizeof(b), "%s%.0fK", v < 0 ? "-" : "", a / 1e3f);
    else snprintf(b, sizeof(b), "%s%.0f", v < 0 ? "-" : "", a);
    return b;
}
// the 0DTE part drawn inside a bar: only when it has the bar's sign, never longer than the bar
inline float odtePart(const Node& n) { if (!n.hasUsd || n.usd == 0 || (n.usd0 < 0) != (n.usd < 0)) return 0; return std::fabs(n.usd0) < std::fabs(n.usd) ? std::fabs(n.usd0) : std::fabs(n.usd); }

// Text inside a node only when the node is long enough to hold it
inline bool fits(int textW, int nodeLen, int pad = 8) { return nodeLen >= textW + pad; }

// Node labels: "4215 +33 (29-56)" on the gamma node, "SELL 82" on the delta node
inline std::string strikeTxt(float k) { char b[32]; if (std::fabs(k - std::floor(k + 0.5f)) < 1e-4f) snprintf(b, sizeof(b), "%d", (int)std::floor(k + 0.5f)); else snprintf(b, sizeof(b), "%g", k); return b; }
inline std::string gammaLabel(const Node& n)   // (Profile 1.3.5, Rassul 2026-09-30) "4220  10  (9 to 17)" - no + sign, no whisker
{
    char b[96]; snprintf(b, sizeof(b), "%s  %ld  (%ld to %ld)", strikeTxt(n.k).c_str(), std::lround(n.g), std::lround(n.lo), std::lround(n.hi)); return b;   // lround: never "-0"
}
inline std::string gammaShort(const Node& n) { char b[48]; snprintf(b, sizeof(b), "%s  %ld", strikeTxt(n.k).c_str(), std::lround(n.g)); return b; }
// (Profile 1.3.5, Rassul 2026-09-30) BUY / SELL stays: it is what dealers must trade on the way there (a snap-back later
// reverses it, but the word says which way the hedge pushes now)
inline std::string deltaLabel(const Node& n) { char b[32]; snprintf(b, sizeof(b), "%s %.0f", n.d > 0 ? "BUY" : "SELL", std::fabs(n.d)); return b; }

// Price text with thousands separators, decimals by market (HG / EU 4, NG 3, else 2; a whole strike has none)
inline std::string fmtPx(float v, const std::string& m, bool strike = false)
{
    int dec = (m == "HG" || m == "EU") ? 4 : (m == "NG" ? 3 : 2);
    if (strike && dec == 2 && std::fabs(v - std::floor(v + 0.5f)) < 1e-3f) dec = 0;
    char b[48]; snprintf(b, sizeof(b), "%.*f", dec, (double)v);
    std::string s = b, ip = s, fp;
    size_t dot = s.find('.'); if (dot != std::string::npos) { ip = s.substr(0, dot); fp = s.substr(dot); }
    bool neg = !ip.empty() && ip[0] == '-'; if (neg) ip = ip.substr(1);
    std::string o; int c = 0;
    for (int i = (int)ip.size() - 1; i >= 0; i--) { o.insert(o.begin(), ip[(size_t)i]); if (++c % 3 == 0 && i > 0) o.insert(o.begin(), ','); }
    return (neg ? "-" : "") + o + fp;
}

// Word-wrap into lines no wider than maxW, using a width function (the plugin passes getTextWidth)
template <class W> inline std::vector<std::string> wrap(const std::string& text, int maxW, W width)
{
    std::vector<std::string> out; std::string line, word; std::stringstream ss(text);
    while (ss >> word) {
        std::string t = line.empty() ? word : line + " " + word;
        if (!line.empty() && width(t) > maxW) { out.push_back(line); line = word; } else line = t;
    }
    if (!line.empty()) out.push_back(line);
    return out;
}

// (1.1) six phases: TRIGGER = RevBar / RevVol / RevWave printed (Rassul 2026-09-29)
static const int NPHASE = 6;
inline const char* phaseName(int i) { static const char* P[] = { "APPROACH", "SWEEP", "TRIGGER", "RECLAIM", "RETEST", "TRADE" }; return (i >= 0 && i < NPHASE) ? P[i] : ""; }
// the strength share shown right outside a node: "2.8%" under 10, "14%" from 10
// (1.2) strength bars beside the %: 0 quiet (< 0.25%), 1 small (0.25-1%), 2 meaningful (1-3%), 3 big (3%+) - the 9/25 first
// bands (worked 1.5% vs failed 0.9%; the big 9/25 lows 3.2% / 5.2%); the nightly study will set them per market
inline int strengthBars(float p) { if (p < 0.25f) return 0; if (p < 1.0f) return 1; if (p < 3.0f) return 2; return 3; }
inline std::string pctTxt(float p) { if (p < 0) return ""; char b[16]; if (p < 10) snprintf(b, sizeof(b), "%.1f%%", p); else snprintf(b, sizeof(b), "%.0f%%", p); return b; }


// (Dealer Summary 1.0, 2026-10-03) LRA-Summary-<MKT>.txt - the Reader's READ for the market (lra/read_store.py):
//    VERSION|1.0   ASOF|<HH:MM:SS CT>|<yyyy-mm-dd>|<epoch sec>   HEAD|<headline>|<#rrggbb>   BODY|<the read>
//    META|<key>|<level>|<mode>|<state>|<verdict>|<trusted 0/1>|<grid age min>
struct Summary {
    bool ok = false;
    std::string ver, asof, day, head, body, key, level, mode, state, verdict;
    long long epoch = 0;
    unsigned int col = 0x00E2E8F0;          // 0x00RRGGBB, as COLOR
    bool trusted = false;
    int gridAge = -1;
};
inline unsigned int hexColour(const std::string& h, unsigned int dflt)
{
    if (h.size() != 7 || h[0] != '#') return dflt;
    unsigned int v = 0;
    for (size_t i = 1; i < 7; i++) {
        char c = (char)std::tolower((unsigned char)h[i]); v <<= 4;
        if (c >= '0' && c <= '9') v |= (unsigned int)(c - '0'); else if (c >= 'a' && c <= 'f') v |= (unsigned int)(c - 'a' + 10); else return dflt;
    }
    return v;
}
inline Summary parseSummary(const std::string& text)
{
    Summary S; std::stringstream ss(text); std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
        std::vector<std::string> f = split(line, '|');
        if (f.empty()) continue;
        const std::string& k = f[0];
        if (k == "VERSION" && f.size() > 1) S.ver = f[1];
        else if (k == "ASOF" && f.size() > 3) { S.asof = f[1]; S.day = f[2]; S.epoch = std::atoll(f[3].c_str()); }
        else if (k == "HEAD" && f.size() > 1) { S.head = f[1]; if (f.size() > 2) S.col = hexColour(f[2], S.col); }
        else if (k == "BODY" && f.size() > 1) S.body = f[1];
        else if (k == "META" && f.size() > 7) { S.key = f[1]; S.level = f[2]; S.mode = f[3]; S.state = f[4]; S.verdict = f[5];
                                                 S.trusted = f[6] == "1"; S.gridAge = f[7].empty() ? -1 : std::atoi(f[7].c_str()); }
    }
    S.ok = !S.head.empty();
    return S;
}
// minutes since the read was written ("12" / "2h" ...); -1 = unknown
inline int ageMin(long long epoch, long long now) { if (epoch <= 0 || now <= 0 || now < epoch - 120) return -1; return (int)((now - epoch) / 60); }
inline std::string ageTxt(int m) { if (m < 0) return ""; char b[24]; if (m < 60) snprintf(b, sizeof(b), "%d min", m); else if (m < 48 * 60) snprintf(b, sizeof(b), "%dh", m / 60); else snprintf(b, sizeof(b), "%dd", m / 1440); return b; }

// (Read 3.1, 2026-10-03 14:41, Rassul: "the Read has too many rows ... a max of 4 rows and the time isn't repeated") the Turn's
// reasons grouped by time: one row per time with all its tags and its sentences joined; more than maxRows times -> the later ones
// fold into the last row, its time shown as a range ("10:20-10:30")
struct TGroup { std::string t; std::vector<size_t> rows; std::string text; };
inline std::vector<TGroup> groupTurn(const std::vector<Data::TRow>& R, size_t maxRows)
{
    std::vector<TGroup> g;
    for (size_t i = 0; i < R.size(); i++) {
        if (g.empty() || g.back().t != R[i].t) { TGroup x; x.t = R[i].t; g.push_back(x); }
        g.back().rows.push_back(i);
        if (!R[i].text.empty()) g.back().text += (g.back().text.empty() ? "" : " ") + R[i].text;
    }
    if (maxRows >= 1 && g.size() > maxRows) {
        TGroup& last = g[maxRows - 1];
        std::string t1 = last.t;
        for (size_t k = maxRows; k < g.size(); k++) {
            for (size_t j = 0; j < g[k].rows.size(); j++) last.rows.push_back(g[k].rows[j]);
            if (!g[k].text.empty()) last.text += (last.text.empty() ? "" : " ") + g[k].text;
            t1 = g[k].t;
        }
        if (!t1.empty() && t1 != last.t) last.t = last.t + "-" + t1;
        g.resize(maxRows);
    }
    return g;
}

}  // namespace dl
