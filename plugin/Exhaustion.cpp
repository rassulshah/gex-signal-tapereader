/********************************************************************************
 *  Exhaustion.cpp  --  Investor/RT RTX extension  lsExhaustion  (0.3.0, 2026-10-11; 0.3.0: closed bars only - the bar in progress read its half-built volume as a fade and then changed (Rassul 22:26); the push gate K / P per session part from the study, the value written once on the first bar over the line, E only on the sides the study chose (22:24 "the exhaustion signal needs to be improved"); 0.2.0: overnight E only at the study's own overnight line T_overnight when both halves agree, else none - Rassul 22:05 "customize the thresholds for time of day .. overnight vs rth")
 *
 *  EXHAUSTION: is the side pushing price running out? Rassul 2026-10-11 21:17-21:28 (option A, "build exhaustion indicator").
 *  Its own pane, two lines coloured by what they mean for price:
 *    red   = BUYERS FADING at a new high (bearish): the buyers' peak pace since the swing low / their pace now
 *    green = SELLERS FADING at a new low (bullish): the mirror
 *  1.0 when price is not at a new N-bar extreme (quiet in chop). The line (T) is drawn; a value at or over it is written over its dot; E
 *  (red at tops, green at bottoms) marks the first closed RTH bar that reaches it - drawn once, recorded, never repainted.
 *  Header: "EXHAUSTION  Buyers fading: 1.0x (3.0x)  Sellers fading: 1.0x (3.0x)" for the LAST CLOSED bar (0.3.0; never the bar in progress).
 *  Per market N / S / T (new-extreme bars, pace bars, line) from lra.exh_study (ExhaustionLogic.h defaults; LRA-ExhNorms-<MKT>.csv
 *  PARAM,N / S / T when the nightly study writes it). Native: the chart's own bars and volume at price (buyers = volume at the ask).
 *  Record: lsFlexLevels\Exhaustion\<MKT>-<sec/bar>-<session>.csv (each E: bar time, side, fade, high / low). No settings.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "ExhaustionLogic.h"
#include "HostSlot.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <set>
#include <new>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#define EX_VERSION "0.3.0"          // keep equal to ex::VERSION and the setVersion literal below

static const COLOR C_BUY = 0x0022C55E, C_SELL = 0x00EF4444, C_GREY = 0x009CA3AF, C_AXIS = 0x00334155;
static const int BARS_MAX = 1500;
static inline bool finiteD(double v) { return v == v && v < 1e300 && v > -1e300; }
namespace pl { inline bool finite(double v) { return finiteD(v); } }
static long long steadyMs() {
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
static std::string flexDir() {
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels";
}
static void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    (void)p;
#endif
}
static bool statFile(const std::string& p) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return false;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
#endif
    return (st.st_mode & S_IFMT) == S_IFREG;
}
static bool moveOver(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
static bool publishFile(const std::string& path, const std::string& contents) {
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false;
      f.write(contents.data(), (std::streamsize)contents.size()); f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    const bool hadOld = statFile(path);
    if (hadOld && !moveOver(path, bak)) { std::remove(tmp.c_str()); return false; }
    if (!moveOver(tmp, path)) { if (hadOld) moveOver(bak, path); std::remove(tmp.c_str()); return false; }
    return true;
}
static bool appendLines(const std::string& path, const std::string& header, const std::vector<std::string>& lines) {
    if (lines.empty()) return true;
    const bool fresh = !statFile(path);
    std::ofstream f(path.c_str(), std::ios::binary | std::ios::app); if (!f) return false;
    if (fresh) f << header << "\n";
    for (size_t i = 0; i < lines.size(); ++i) f << lines[i] << "\n";
    f.flush(); return (bool)f;
}
static std::vector<std::string> split(const std::string& s, char c) {
    std::vector<std::string> f; size_t a = 0;
    for (size_t i = 0; i <= s.size(); ++i) if (i == s.size() || s[i] == c) { f.push_back(s.substr(a, i - a)); a = i + 1; }
    return f;
}
static bool parseD(const std::string& t, double& out) {
    if (t.empty() || t.size() > 40) return false;
    errno = 0; char* e = nullptr; const double v = std::strtod(t.c_str(), &e);
    if (errno || e != t.c_str() + t.size() || !pl::finite(v)) return false;
    out = v; return true;
}
static bool parseLL(const std::string& t, long long& out) {
    if (t.empty() || t.size() > 24) return false;
    errno = 0; char* e = nullptr; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno || e != t.c_str() + t.size()) return false;
    out = v; return true;
}
static std::string marketName(std::string r) {
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return r.substr(0, 8);
}
static long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}


// RTH (Central minutes) per market - E prints in RTH only, as the study measured it
static bool rthOf(const std::string& m, int& o, int& c) {
    if (m == "ES" || m == "NQ") { o = 510; c = 900; return true; }
    if (m == "CL" || m == "NG") { o = 480; c = 810; return true; }
    if (m == "GC") { o = 440; c = 750; return true; }
    if (m == "HG") { o = 430; c = 750; return true; }
    if (m == "EU") { o = 440; c = 840; return true; }
    return false;
}

struct EXState {
    ex::Params P; std::string params = "study 2026-10-11", proven = "testing";
    ex::Params Pon; bool onSet = false; std::string provenOn = "testing";   // (0.3.0) overnight read: its own K / P / T / sides; E only when confirmed
    double tOvernight = 0;                                  // (0.2.0) the overnight line (0 = no E overnight) - only when the study agrees on one   // proven: the nightly STATUS line (confirmed / testing)
    bool busy = false, faulted = false;
    std::string identity, market, symbol, state = "starting", lastStatusText;
    int spb = 0; double tick = 0; int rthO = -1, rthC = -1;
    long count = 0; long long session = -1, lastNormsMs = -1, lastStatusMs = -1;
    std::map<long long, std::pair<double, double>> vol;     // bar time -> (buyers, sellers) of CLOSED bars (read once)
    std::map<long long, int> marks;                         // bar time -> +1 bottom E / -1 top E (recorded, never repainted)
    std::map<long long, double> markVal;
    std::vector<std::string> pending; long long recFails = 0;
    ex::Out out; int outFirst = 0; double liveB = 1, liveS = 1, liveT = 0;
    std::vector<char> rthBar;                               // (0.3.0) per computed bar: RTH (1) or overnight (0)
};

class Exhaustion : public cppExtension {
public:
    Exhaustion() : cppExtension() {}
    virtual int draw(void);
    virtual int parmsLoad(void)  { return RTX_OK; }
    virtual int parmsApply(void) { return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { return RTX_OK; }
    void pump();
    void release() { slot_.release(this); }
    EXState* testState() { return slot_.get(this, false); }
private:
    HostSlot<EXState> slot_;
    long long ctSec(RTDATE d);
    void identify(EXState& S);
    bool barVol(EXState& S, RTARRAYP& vap, int bar, std::pair<double, double>& out);
    void calcBars(EXState& S, long count);
    void load(EXState& S);
    void norms(EXState& S);
    void status(EXState& S, long long now);
    void render(EXState& S);
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + "\\Exhaustion"; }
    std::string stem(EXState& S) { return S.market + "-" + std::to_string(S.spb); }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<Exhaustion*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<Exhaustion*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void)   { setParameterVersion(1); return RTX_OK; }
int cppExtension::calc(int)     { static_cast<Exhaustion*>(this)->pump(); return RTX_OK; }
int Exhaustion::draw(void) {
    pump();
    EXState* S = slot_.get(this, false);
    if (S && !S->faulted) { try { render(*S); } catch (...) { S->state = "draw error"; } }
    return RTX_OK;
}
long long Exhaustion::ctSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    return civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
}
void Exhaustion::identify(EXState& S) {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR); if (!finiteD(tick) || tick <= 0 || tick >= 1000) tick = 0;
    const std::string id = sym + "|" + std::to_string(spb);
    if (id == S.identity && tick == S.tick) return;
    S.identity = id; S.symbol = sym; S.spb = spb; S.tick = tick; S.market = marketName(root.empty() ? sym : root);
    S.P = ex::defaults(S.market); S.Pon = S.P; S.onSet = false; S.provenOn = "testing"; S.tOvernight = 0; S.params = "study 2026-10-11"; S.vol.clear(); S.marks.clear(); S.markVal.clear(); S.pending.clear();
    S.count = 0; S.session = -1; S.lastNormsMs = -1; S.rthO = S.rthC = -1; rthOf(S.market, S.rthO, S.rthC);
}
// the buyers / sellers of one bar from its volume at price (guarded like lsLiquidityProfile: a failed read is "not known")
bool Exhaustion::barVol(EXState& S, RTARRAYP& vap, int bar, std::pair<double, double>& out) {
    (void)S;
    BARSTATISTICS st; std::memset(&st, 0, sizeof(st));
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (vap.getBarStatistics(bar, st) != RTX_OK) return false;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
    if (st.buyVolume < 0 || st.sellVolume < 0) return false;
    out = std::make_pair((double)st.buyVolume, (double)st.sellVolume); return true;
}
void Exhaustion::calcBars(EXState& S, long count) {
    if (count < 3) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow); RTARRAYP vap(barVolumeProfile);
    if (times.count < count || hi.count < count || lo.count < count || vap.count < count) return;
    // (0.3.0) CLOSED bars only: the bar in progress (count - 1) is never read - its half-built volume made a huge fade that changed later
    const long closed = count - 1;
    const int first = (int)std::max(0L, closed - BARS_MAX);
    std::vector<double> h, l, b, s; std::vector<char> rth;
    h.reserve(closed - first); l.reserve(closed - first); b.reserve(closed - first); s.reserve(closed - first); rth.reserve(closed - first);
    for (int i = first; i < closed; ++i) {
        const long long bt = (long long)times[i];
        std::pair<double, double> v(0, 0);
        std::map<long long, std::pair<double, double>>::const_iterator c = S.vol.find(bt);
        if (c != S.vol.end()) v = c->second;
        else if (barVol(S, vap, i, v)) S.vol[bt] = v;
        h.push_back(hi[i]); l.push_back(lo[i]); b.push_back(v.first); s.push_back(v.second);
        struct tm t; std::memset(&t, 0, sizeof(t)); bool r = true;
        if (getLocaltime((RTDATE)times[i], &t)) {
            const int endMin = t.tm_hour * 60 + t.tm_min, startMin = endMin - std::max(1, S.spb / 60);
            r = S.rthO < 0 || (startMin >= S.rthO && endMin <= S.rthC);       // IRT stamps the bar END
        }
        rth.push_back(r ? 1 : 0);
    }
    // the RTH read and the overnight read (its own K / P / T / sides when the study wrote one), merged bar by bar
    const ex::Out R = ex::compute(h, l, b, s, S.P);
    const ex::Out O = S.onSet ? ex::compute(h, l, b, s, S.Pon) : R;
    S.out = R; S.outFirst = first; S.rthBar = rth;
    // (0.3.0) "only what works": E (and its written value) only where the nightly study confirmed this part on unseen days; else the lines alone
    const bool rthOk = S.proven == "confirmed";
    for (size_t k = 0; k < rth.size(); ++k) if (rth[k] && !rthOk) { S.out.eTop[k] = S.out.eBot[k] = 0; S.out.xTop[k] = S.out.xBot[k] = 0; }
    for (size_t k = 0; k < rth.size(); ++k) if (!rth[k]) {
        S.out.fb[k] = O.fb[k]; S.out.fs[k] = O.fs[k]; S.out.newH[k] = O.newH[k]; S.out.newL[k] = O.newL[k];
        const bool on = S.onSet && S.provenOn == "confirmed";                 // overnight E only where the study confirmed it
        S.out.xTop[k] = on && O.xTop[k]; S.out.xBot[k] = on && O.xBot[k];
        S.out.eTop[k] = on && O.eTop[k]; S.out.eBot[k] = on && O.eBot[k];
    }
    if (S.out.fb.empty()) return;
    const size_t f = S.out.fb.size() - 1;
    S.liveB = S.out.fb[f]; S.liveS = S.out.fs[f]; S.liveT = rth[f] || !S.onSet ? S.P.T : S.Pon.T;
    // an E is recorded once (closed bars; new bars only after the first pass)
    const long from = S.count == 0 ? first : std::max((long)first, S.count - 2);
    for (long i = from; i < closed; ++i) {
        const int k = (int)(i - first);
        const long long bt = (long long)times[(int)i];
        if (S.marks.count(bt)) continue;
        int side = 0; double val = 0;
        if (S.out.eTop[k]) { side = -1; val = S.out.fb[k]; } else if (S.out.eBot[k]) { side = 1; val = S.out.fs[k]; }
        if (!side) continue;
        const ex::Params& pp = rth[k] ? S.P : S.Pon;
        S.marks[bt] = side; S.markVal[bt] = val;
        char line[200]; std::snprintf(line, sizeof(line), "%lld|%s|%.2f|%.6g|%.6g|N%d S%d T%.2f K%d P%.1f%s", bt, side < 0 ? "top" : "bottom", val, (double)hi[(int)i], (double)lo[(int)i], pp.N, pp.S, pp.T, pp.K, pp.P, rth[k] ? "" : " overnight");
        S.pending.push_back(line);
    }
    S.count = count;
    if (!S.pending.empty() && S.session >= 0) {
        const std::string d = dir(); makeDir(flexDir()); makeDir(d);
        if (!d.empty() && appendLines(d + "\\" + stem(S) + "-" + std::to_string(S.session) + ".csv", "bar_time|side|fade|high|low|params", S.pending)) S.pending.clear();
        else S.recFails++;
    }
    while (S.vol.size() > 3000) S.vol.erase(S.vol.begin());
}
void Exhaustion::load(EXState& S) {
    const std::string d = dir(); if (d.empty()) return;
    for (long long k = S.session - 9; k <= S.session; ++k) {
        std::ifstream in((d + "\\" + stem(S) + "-" + std::to_string(k) + ".csv").c_str()); std::string line; int n = 0;
        while (std::getline(in, line) && ++n < 5000) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const std::vector<std::string> c = split(line, '|'); long long bt = 0; double v = 0;
            if (c.size() < 3 || !parseLL(c[0], bt) || bt <= 0 || (c[1] != "top" && c[1] != "bottom") || !parseD(c[2], v)) continue;
            S.marks[bt] = c[1] == "top" ? -1 : 1; S.markVal[bt] = v;
        }
    }
}
void Exhaustion::norms(EXState& S) {
    const std::string f = flexDir(); if (f.empty()) return;
    std::ifstream in((f + "\\LRA-ExhNorms-" + S.market + ".csv").c_str()); if (!in) return;
    std::string line, used; int n = 0; ex::Params p = ex::defaults(S.market), q; double on = 0; bool onK = false;
    std::string pr = "testing", prOn = "testing";
    int qK = 0, qSides = 3; double qP = 0;
    while (std::getline(in, line) && ++n < 200) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> c = split(line, ','); double v = 0;
        if (c.size() >= 2 && c[0] == "STATUS") { pr = c[1] == "confirmed" ? "confirmed" : "testing"; continue; }
        if (c.size() >= 2 && c[0] == "STATUS_overnight") { prOn = c[1] == "confirmed" ? "confirmed" : "testing"; continue; }
        if (c.size() < 3 || c[0] != "PARAM" || !parseD(c[2], v)) continue;
        if (c[1] == "N" && v >= 2 && v <= 60) { p.N = (int)v; used += " N " + c[2]; }
        else if (c[1] == "S" && v >= 1 && v <= 5) { p.S = (int)v; used += " S " + c[2]; }
        else if (c[1] == "T" && v >= 1.1 && v <= 10) { p.T = v; used += " T " + c[2]; }
        else if (c[1] == "K" && v >= 0 && v <= 40) { p.K = (int)v; used += " K " + c[2]; }
        else if (c[1] == "P" && v >= 0 && v <= 10) { p.P = v; used += " P " + c[2]; }
        else if (c[1] == "SIDES" && v >= 0 && v <= 3) { p.sides = (int)v; used += " SIDES " + c[2]; }
        else if (c[1] == "T_overnight" && v >= 1.1 && v <= 10) { on = v; used += " T_overnight " + c[2]; }
        else if (c[1] == "K_overnight" && v >= 0 && v <= 40) { qK = (int)v; onK = true; used += " K_overnight " + c[2]; }
        else if (c[1] == "P_overnight" && v >= 0 && v <= 10) { qP = v; used += " P_overnight " + c[2]; }
        else if (c[1] == "SIDES_overnight" && v >= 0 && v <= 3) { qSides = (int)v; used += " SIDES_overnight " + c[2]; }
    }
    q = p; q.T = on > 0 ? on : p.T; q.K = qK; q.P = qP; q.sides = qSides;
    // overnight: the study's own read when it wrote one (K_overnight); a bare T_overnight (0.2 norms) = the RTH read at that line, E when it agreed
    const bool onSet = on > 0;
    if (on > 0 && !onK) { q = p; q.T = on; prOn = "confirmed"; }
    const bool same = p.N == S.P.N && p.S == S.P.S && p.T == S.P.T && p.K == S.P.K && p.P == S.P.P && p.sides == S.P.sides
                      && onSet == S.onSet && q.T == S.Pon.T && q.K == S.Pon.K && q.P == S.Pon.P && q.sides == S.Pon.sides && prOn == S.provenOn && pr == S.proven;
    if (!same) { S.P = p; S.Pon = q; S.onSet = onSet; S.provenOn = prOn; S.proven = pr; S.tOvernight = on; S.count = 0; }
    if (!used.empty()) S.params = "data:" + used;
}
void Exhaustion::status(EXState& S, long long now) {
    if (S.lastStatusMs >= 0 && now - S.lastStatusMs < 1000 && now >= S.lastStatusMs) return;
    S.lastStatusMs = now;
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << EX_VERSION << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nSTATE," << S.state
      << "\nPARAMS,N " << S.P.N << ",S " << S.P.S << ",T " << S.P.T << ",K " << S.P.K << ",P " << S.P.P << ",SIDES " << S.P.sides << "," << S.params
      << "\nOVERNIGHT," << (S.onSet ? "set" : "rth read") << ",T " << S.Pon.T << ",K " << S.Pon.K << ",P " << S.Pon.P << ",SIDES " << S.Pon.sides << "," << S.provenOn << "\nRTH," << S.rthO << "," << S.rthC << "\nOVERNIGHT_LINE," << S.tOvernight
      << "\nPROVEN," << S.proven << "\nNOW (last closed bar),buyers_fading " << S.liveB << ",sellers_fading " << S.liveS << ",line " << S.liveT << "\nMARKS," << S.marks.size() << ",fails " << S.recFails << "\n";
    const std::string text = o.str(); if (text == S.lastStatusText) return;
    const std::string f = flexDir(); if (f.empty()) return;
    if (publishFile(f + "\\Exhaustion.status-" + stem(S) + ".txt", text)) S.lastStatusText = text;
}
void Exhaustion::pump() {
    EXState* P = slot_.get(this, true);
    if (!P || P->busy) return;
    EXState& S = *P; S.busy = true;
    struct Unbusy { EXState* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
    try {
        identify(S);
        const long long now = steadyMs();
        const long long ct = ctSec(currentDate());
        if (ct > 0) { const long long key = (ct - 17LL * 3600) / 86400LL + 1; if (key != S.session) { S.session = key; S.marks.clear(); S.markVal.clear(); load(S); } }
        if (S.lastNormsMs < 0 || now - S.lastNormsMs >= 60000) { S.lastNormsMs = now; norms(S); }
        const long count = getBarCount();
        calcBars(S, count);
        S.state = count < S.P.N + 2 ? "waiting for bars" : "ready";
        status(S, now);
    } catch (...) { S.state = "calculation error"; }
}
void Exhaustion::render(EXState& S) {
    RCT pane; pane.getPaneRect(false);
    const long count = getBarCount();
    if (count < 2 || pane.bottom - pane.top < 40 || pane.right - pane.left < 120 || S.out.fb.empty()) return;
    const int lastBar = std::min((int)count - 2, S.outFirst + (int)S.out.fb.size() - 1);    // (0.3.0) the last CLOSED bar
    if (lastBar < S.outFirst) return;
    FONT f; f.id = HELVETICA; f.size = 9; f.style = BOLD; setFont(f);
    auto txt = [&](int x, int y, const std::string& t, COLOR c) { setTextColor(c); const int w = getTextWidth(t.c_str(), -1); RCT r; r.set(coord(x), coord(y), coord(x + w + 2), coord(y + 12)); r.drawText(t.c_str(), false, false); return x + w; };
    auto xs = [](double v) { char b[16]; std::snprintf(b, sizeof(b), "%.1fx", v); return std::string(b); };
    auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
    const double T = S.liveT > 0 ? S.liveT : S.P.T;     // the line of the session part of the last closed bar
    {
        int x = txt(pane.left + 4, pane.top + 1, "EXHAUSTION   ", C_GREY);
        x = txt(x, pane.top + 1, "Buyers fading: ", C_GREY); x = txt(x, pane.top + 1, xs(S.liveB), C_SELL); x = txt(x, pane.top + 1, " (" + xs(T) + ")   ", C_GREY);
        x = txt(x, pane.top + 1, "Sellers fading: ", C_GREY); x = txt(x, pane.top + 1, xs(S.liveS), C_BUY); txt(x, pane.top + 1, " (" + xs(T) + ")", C_GREY);
        // whether the nightly study has confirmed this market's E on his trade (unseen days) for this session part: "testing" until it has
        const bool onNow = S.onSet && !S.rthBar.empty() && !S.rthBar.back();
        const std::string& pv = onNow ? S.provenOn : S.proven;
        const int w = getTextWidth(pv.c_str(), -1); txt(pane.right - w - 8, pane.top + 1, pv, C_GREY);
    }
    const int top = pane.top + 16, bottom = pane.bottom - 16;
    std::vector<int> vis;
    for (int i = lastBar; i >= S.outFirst; --i) { const int x = xOf(i); if (x < pane.left) break; if (x <= pane.right) vis.push_back(i); }
    std::reverse(vis.begin(), vis.end());
    auto val = [&](int bar, bool buyers) { const int k = bar - S.outFirst; if (k < 0 || k >= (int)S.out.fb.size()) return 1.0; return buyers ? S.out.fb[k] : S.out.fs[k]; };
    double mx = std::max(3.0, T * 1.6);
    for (size_t k = 0; k < vis.size(); ++k) mx = std::max(mx, std::max(val(vis[k], true), val(vis[k], false)));
    mx = std::min(mx * 1.2, 10.0);
    auto yOf = [&](double v) { return bottom - (int)((bottom - top) * std::min(1.0, std::max(0.0, v / mx))); };
    setPen(C_AXIS, 1, P_SOLID);
    { const int y = yOf(T); PNT a; a.set(0, 0.0f); a.h = coord(pane.left); a.v = coord(y); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(pane.right); b.v = coord(y); b.drawLineTo();
      txt(pane.right - 34, y - 13, xs(T), C_GREY); }
    for (int s = 0; s < 2; ++s) {
        const bool buyers = s == 0; const COLOR c = buyers ? C_SELL : C_BUY;     // buyers fading = bearish red, sellers fading = bullish green
        setPen(c, 2, P_SOLID); bool started = false;
        for (size_t k = 0; k < vis.size(); ++k) {
            PNT p; p.set(0, 0.0f); p.h = coord(xOf(vis[k])); p.v = coord(yOf(val(vis[k], buyers)));
            if (!started) { p.setDrawPosition(); started = true; } else p.drawLineTo();
        }
        for (size_t k = 0; k < vis.size(); ++k) {
            const double v = val(vis[k], buyers); if (v <= 1.0 + 1e-9) continue;
            const int x = xOf(vis[k]), y = yOf(v);
            RCT d; d.set(coord(x - 2), coord(y - 2), coord(x + 2), coord(y + 2)); d.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID);
            // (0.3.0) the value is written once: on the first bar over its own line (no stacked labels on the bars after it)
            const int kk = vis[k] - S.outFirst;
            if (kk >= 0 && kk < (int)S.out.xTop.size() && (buyers ? S.out.xTop[kk] : S.out.xBot[kk])) { const std::string t = xs(v); const int w = getTextWidth(t.c_str(), -1); txt(x - w / 2, y - 15, t, c); }
        }
    }
    // E: the recorded marks (never repainted), at the bottom of the pane under their bar
    RTARRAYI times(barDateTime);
    if (times.count >= count) for (size_t k = 0; k < vis.size(); ++k) {
        std::map<long long, int>::const_iterator m = S.marks.find((long long)times[vis[k]]);
        if (m == S.marks.end()) continue;
        txt(xOf(vis[k]) - 3, bottom + 2, "E", m->second < 0 ? C_SELL : C_BUY);
    }
}

extern "C" cppExtension* CreateExtension(void) {
    Exhaustion* p = new Exhaustion();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED);
    p->setDescription("LRA Exhaustion: is the side pushing price running out? Red = buyers fading at a new high (bearish), green = sellers fading at a new low (bullish): their peak pace in the push / their pace now, 1.0 when price is not at a new extreme. E = the first RTH bar at or over the market's line (from the nightly study), recorded and never repainted.");
    p->setVersion("0.3.0");   // EX_VERSION
    return p;
}
