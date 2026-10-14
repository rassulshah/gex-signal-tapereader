/********************************************************************************
 *  CobImpulse.cpp  --  Investor/RT RTX extension  lsCobImpulse  (0.1.0, 2026-10-14)
 *
 *  COB IMPULSE: did the CoB leg start with force? Rassul 2026-10-14 12:07 CT (approved; mockup cob/mockup-cob-impulse.png):
 *  "tag CoB signals that have an IMPULSE at their origin". Put ONE on the price pane of each 3-min LTF chart (ES NQ CL GC HG NG 6E).
 *    - It reads WHERE his CoB fired from his own RTL (cob_bull_3m_1b / cob_bear_3m_1b, read by name like the Dealer Profile reads
 *      cob_bull) and his Abs_Bull / Abs_Bear, on each new bar, for CLOSED bars only.
 *    - It finds the CoB leg's pivot (his 1-bar ZigZag, native) and looks at the pivot bar .. 2 bars after it: a bar inside the pivot
 *      bar (bull: its low below the pivot bar's high; bear: its high above the pivot bar's low) with the biggest RANGE in 35+ bars
 *      = R, the biggest VOLUME in 35+ bars = V (the chart's own bars and volume; nothing external at run time).
 *    - Label "Cob" / "Cob+R?" / "Cob+V?" / "Cob+RV?" above the leg's high (bull) / below its low (bear), a thin 1 px white box around
 *      the impulse bar, no numbers on the chart; the details (bar time 12-hour CT, "biggest range in N bars", 2.4x the average) in
 *      the hover over the label or the box.
 *    - Impulse tag display per market (coordinator 2026-10-14: the tag alone flipped sign BUILD -> VALIDATE; Abs on the pivot +
 *      impulse is the only consistent lead): lsFlexLevels\LRA-CobImpulse-<MKT>.csv MODE,off | abs | always (default abs = shown only
 *      with Abs on the pivot); PROVEN,1 drops the "?"; PLAIN,0 stops drawing plain "Cob". The tag is always recorded.
 *  NEVER REPAINTS: each CoB is decided once at its close and recorded (lsFlexLevels\CobImpulse\<MKT>-<CONTRACT>-<sec>-<session>.csv);
 *  labels and boxes are drawn from the records only. Status: lsFlexLevels\CobImpulse.status-<MKT>-<CONTRACT>-<sec>.txt.
 *  No settings window (controls none; the market file is the setting). Logic without the SDK: CobImpulseLogic.h (tested).
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
#include "CobImpulseLogic.h"
#include "HoverBoard.h"                  // hb::hbCursor (GetCursorPos + WindowFromPoint; carries the user32 pragma)
#include "HostSlot.h"
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")      // HoverBoard.h's GetCursorPos / WindowFromPoint
#endif
#include <algorithm>
#include <cerrno>
#include <cctype>
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
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#define CI_VERSION "0.1.0"          // keep equal to cimp::VERSION and the setVersion literal below

static const COLOR C_LABEL = 0x0080FFFF;   // his IRT Cob label colour (light cyan), unchanged
static const COLOR C_BOX = 0x00FFFFFF, C_HOVBG = 0x00111827, C_HOVEDGE = 0x009CA3AF, C_INK = 0x00F3F4F6;
static const int BARS_MAX = 1500;
static const char* const NAME_BULL = "cob_bull_3m_1b";
static const char* const NAME_BEAR = "cob_bear_3m_1b";
static const char* const NAME_ABULL = "Abs_Bull";
static const char* const NAME_ABEAR = "Abs_Bear";

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
static bool statFile(const std::string& p, long long& stamp) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return false;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
#endif
    if ((st.st_mode & S_IFMT) != S_IFREG) return false;
    stamp = (long long)st.st_mtime * 1000003LL + (long long)st.st_size; return true;
}
static bool moveOver(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
static bool publishFile(const std::string& path, const std::string& contents) {
    const std::string tmp = path + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false;
      f.write(contents.data(), (std::streamsize)contents.size()); f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    if (!moveOver(tmp, path)) { std::remove(tmp.c_str()); return false; }
    return true;
}
// bounded whole-file read (64 KB / 512 KB caps); false = missing, unreadable or too big
static bool readSmall(const std::string& path, size_t cap, std::string& out) {
    out.clear();
    std::ifstream in(path.c_str(), std::ios::binary); if (!in) return false;
    in.seekg(0, std::ios::end); const std::streamoff size = in.tellg();
    if (size < 0 || size > (std::streamoff)cap) return false;
    in.seekg(0, std::ios::beg); out.resize((size_t)size);
    if (size > 0) in.read(&out[0], size);
    return (bool)in || in.eof();
}
static std::string fileKey(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) {
        const unsigned char c = (unsigned char)s[i];
        out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_');
    }
    return out.empty() ? "UNKNOWN" : out;
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

static const char* const REC_FILE_HEADER = "trig_time|side|pivot_time|leg_end_time|r_time|r_n|r_x|v_time|v_n|v_x|abs";
static const size_t REC_FILE_MAX = 512 * 1024, REC_LINES_MAX = 5000;

struct HoverT { std::string key; int l = 0, t = 0, r = 0, b = 0; cimp::Rec rec; };
struct CIState {
    bool busy = false;
    std::string identity, market, symbol, state = "starting", lastStatusText, flags = "not read yet", paramsWhy = "missing";
    int spb = 0; long long session = -1;
    cimp::Params P; long long paramsStamp = -2, paramsCheckMs = -1, lastStatusMs = -1, lastRecordMs = -1;
    long lastCount = 0; long long decidedThrough = 0;        // bar time of the last decided closed bar
    std::map<std::pair<long long, int>, cimp::Rec> recs;     // recorded CoBs (drawn)
    std::vector<std::pair<long long, cimp::Rec>> pending;    // (session, record) waiting to be published
    long long recFails = 0, decided = 0;
    // hover
    std::vector<HoverT> targets; bool msHas = false; int msH = 0, msV = 0; unsigned long long msWnd = 0; std::string hovKey;
};

class CobImpulse : public cppExtension {
public:
    CobImpulse() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int parmsLoad(void)  { return RTX_OK; }
    virtual int parmsApply(void) { return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { return RTX_OK; }
    void pump(bool mayCalc);
    void release() { slot_.release(this); }
    CIState* testState() { return slot_.get(this, false); }
private:
    HostSlot<CIState> slot_;
    long long ctSec(RTDATE d);
    long long sessionOf(long long barTime) { const long long cs = ctSec((RTDATE)barTime); return cs > 0 ? (cs - 17LL * 3600) / 86400LL + 1 : -1; }
    void identify(CIState& S);
    void params(CIState& S, long long now);
    bool readFlags(const char* name, long count, std::vector<float>& out);
    void decide(CIState& S, long count);
    bool flush(CIState& S);
    void load(CIState& S);
    void status(CIState& S, long long now);
    void render(CIState& S);
    std::string clockOf(long long t);
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + "\\CobImpulse"; }
    std::string stem(CIState& S) { return S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb); }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<CobImpulse*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<CobImpulse*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void)   { setParameterVersion(1); return RTX_OK; }
int cppExtension::calc(int)     { static_cast<CobImpulse*>(this)->pump(true); return RTX_OK; }
int CobImpulse::draw(void) {
    pump(false);
    CIState* S = slot_.get(this, false);
    if (S) { try { render(*S); } catch (...) { S->state = "draw error"; } }
    return RTX_OK;
}
long long CobImpulse::ctSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    return civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
}
std::string CobImpulse::clockOf(long long t) {
    struct tm m; std::memset(&m, 0, sizeof(m));
    if (t <= 0 || !getLocaltime((RTDATE)t, &m)) return "?";
    return cimp::clock12(m.tm_hour, m.tm_min);
}
void CobImpulse::identify(CIState& S) {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    const std::string id = root + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity) return;
    S.identity = id; S.symbol = sym; S.spb = spb; S.market = marketName(root.empty() ? sym : root);
    S.recs.clear(); S.pending.clear(); S.lastCount = 0; S.decidedThrough = 0; S.session = -1; S.paramsStamp = -2; S.paramsCheckMs = -1;
    S.targets.clear(); S.hovKey.clear();
}
void CobImpulse::params(CIState& S, long long now) {
    if (S.paramsCheckMs >= 0 && now >= S.paramsCheckMs && now - S.paramsCheckMs < 10000) return;
    S.paramsCheckMs = now;
    const std::string f = flexDir(); if (f.empty()) return;
    const std::string p = f + "\\LRA-CobImpulse-" + S.market + ".csv";
    long long st = -1;
    if (!statFile(p, st)) { if (S.paramsStamp != -1) { S.P = cimp::Params(); S.paramsWhy = "missing (defaults)"; S.paramsStamp = -1; } return; }
    if (st == S.paramsStamp) return;
    std::string text, why;
    if (!readSmall(p, 64 * 1024, text)) { S.paramsWhy = "unreadable or over 64 KB (kept the last good)"; return; }   // retried on the next check
    long long st2 = -1; if (!statFile(p, st2) || st2 != st) return;                                                  // torn: next check
    S.P = cimp::parseFile(text, S.market, why);
    S.paramsWhy = why.empty() ? "file" : why + " (defaults)"; S.paramsStamp = st;
}
// his RTL by name, aligned so element i is chart bar i (the array ends on the chart's last bar); false = not readable
bool CobImpulse::readFlags(const char* name, long count, std::vector<float>& out) {
    out.assign((size_t)count, 0.0f);
    char nb[96]; std::memset(nb, 0, sizeof(nb)); std::strncpy(nb, name, sizeof(nb) - 1);
    RTARRAY a(fEmptyArray);
    if (a.getCustomIndicator(nb, 1, true) != RTX_OK || a.count <= 0 || a.count > count) return false;
    const long shift = count - (long)a.count;
    for (long i = shift; i < count; ++i) { const float v = a[(int)(i - shift)]; out[(size_t)i] = (v == v) ? v : 0.0f; }
    return true;
}
void CobImpulse::decide(CIState& S, long count) {
    if (count < 4) return;
    RTARRAYI times(barDateTime), vol(barVolume); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if (times.count < count || vol.count < count || hi.count < count || lo.count < count || op.count < count || cl.count < count) { S.state = "bars not ready"; return; }
    const long lastClosed = count - 2;
    if (S.decidedThrough > 0 && (long long)times[(int)lastClosed] <= S.decidedThrough) return;
    std::vector<float> fb, fs, ab, as;
    const bool okB = readFlags(NAME_BULL, count, fb), okS = readFlags(NAME_BEAR, count, fs);
    const bool okAB = readFlags(NAME_ABULL, count, ab), okAS = readFlags(NAME_ABEAR, count, as);
    S.flags = std::string(NAME_BULL) + (okB ? " ok" : " NOT FOUND") + ", " + NAME_BEAR + (okS ? " ok" : " NOT FOUND") + ", "
            + NAME_ABULL + (okAB ? " ok" : " NOT FOUND") + ", " + NAME_ABEAR + (okAS ? " ok" : " NOT FOUND");
    if (!okB && !okS) { S.state = "CoB RTL not readable - nothing decided (retried next bar)"; return; }   // never advance blind
    const long first = std::max(0L, count - 1 - BARS_MAX);
    std::vector<cimp::Bar> B; B.reserve((size_t)(count - first));
    for (long i = first; i <= lastClosed; ++i) {
        cimp::Bar b; b.t = (long long)times[(int)i]; b.o = op[(int)i]; b.h = hi[(int)i]; b.l = lo[(int)i]; b.c = cl[(int)i];
        b.v = (double)vol[(int)i]; B.push_back(b);
    }
    for (long i = first + 2; i <= lastClosed; ++i) {
        const long long bt = (long long)times[(int)i];
        if (bt <= S.decidedThrough) continue;
        for (int s = 0; s < 2; ++s) {
            const int side = s == 0 ? 1 : -1;
            const bool okFlag = side > 0 ? okB : okS;
            if (!okFlag || (side > 0 ? fb : fs)[(size_t)i] < 0.5f) continue;
            if (S.recs.count(std::make_pair(bt, side))) continue;
            bool queued = false; for (size_t q = 0; q < S.pending.size(); ++q) if (S.pending[q].second.trig == bt && S.pending[q].second.side == side) queued = true;
            if (queued) continue;
            const int k = (int)(i - first);
            const int p = cimp::legPivot(B, k, side);
            bool abs = false;
            if (p >= 0) { const std::vector<float>& A = side > 0 ? ab : as; const bool okA = side > 0 ? okAB : okAS; abs = okA && A[(size_t)(p + first)] >= 0.5f; }
            const cimp::Tag T = cimp::evaluate(B, k, side, S.P, abs);
            const long long sess = sessionOf(bt);
            if (sess < 0 || S.pending.size() >= REC_LINES_MAX) { ++S.recFails; continue; }
            S.pending.push_back(std::make_pair(sess, cimp::toRec(T, B))); ++S.decided;
        }
    }
    S.decidedThrough = (long long)times[(int)lastClosed];
    S.state = "ready";
    flush(S);
}
bool CobImpulse::flush(CIState& S) {
    if (S.pending.empty()) return true;
    const long long now = steadyMs();
    if (S.lastRecordMs >= 0 && now >= S.lastRecordMs && now - S.lastRecordMs < 1000) return false;   // a failed publish is retried after 1 s
    const std::string d = dir(); if (d.empty()) { ++S.recFails; S.lastRecordMs = now; return false; }
    makeDir(flexDir()); makeDir(d);
    std::map<long long, std::vector<size_t>> groups;
    for (size_t i = 0; i < S.pending.size(); ++i) groups[S.pending[i].first].push_back(i);
    std::vector<char> done(S.pending.size(), 0);
    for (const auto& g : groups) {
        const std::string path = d + "\\" + stem(S) + "-" + std::to_string(g.first) + ".csv";
        std::string old; long long st = -1; std::vector<std::string> keep; std::set<std::pair<long long, int>> seen;
        if (statFile(path, st)) {
            if (!readSmall(path, REC_FILE_MAX, old)) { ++S.recFails; continue; }   // never rewrite a file we could not read
            std::istringstream in(old); std::string line; bool head = true;
            while (std::getline(in, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (head) { head = false; if (line != REC_FILE_HEADER) { keep.clear(); break; } continue; }
                cimp::Rec r; if (cimp::parseRec(line, r) && seen.insert(std::make_pair(r.trig, r.side)).second) keep.push_back(line);
            }
            if ((head && !old.empty()) || (keep.empty() && !old.empty() && old.compare(0, std::strlen(REC_FILE_HEADER), REC_FILE_HEADER) != 0)) { ++S.recFails; continue; }
        }
        std::string text = std::string(REC_FILE_HEADER) + "\n";
        for (const auto& k : keep) text += k + "\n";
        for (size_t j : g.second) { const cimp::Rec& r = S.pending[j].second; if (seen.insert(std::make_pair(r.trig, r.side)).second) text += cimp::recLine(r) + "\n"; }
        if (text.size() > REC_FILE_MAX || !publishFile(path, text)) { ++S.recFails; continue; }
        for (size_t j : g.second) { const cimp::Rec& r = S.pending[j].second; if (!S.recs.count(std::make_pair(r.trig, r.side))) S.recs[std::make_pair(r.trig, r.side)] = r; done[j] = 1; }
    }
    std::vector<std::pair<long long, cimp::Rec>> retry;
    for (size_t i = 0; i < S.pending.size(); ++i) if (!done[i]) retry.push_back(S.pending[i]);
    S.pending.swap(retry);
    S.lastRecordMs = S.pending.empty() ? -1 : now;
    return S.pending.empty();
}
void CobImpulse::load(CIState& S) {
    const std::string d = dir(); if (d.empty()) return;
    for (long long k = S.session - 2; k <= S.session; ++k) {
        const std::string path = d + "\\" + stem(S) + "-" + std::to_string(k) + ".csv";
        long long st = -1; if (!statFile(path, st)) continue;
        std::string text; if (!readSmall(path, REC_FILE_MAX, text)) { ++S.recFails; continue; }
        std::istringstream in(text); std::string line; bool head = true; size_t n = 0;
        while (std::getline(in, line) && ++n <= REC_LINES_MAX) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (head) { head = false; if (line != REC_FILE_HEADER) break; continue; }
            cimp::Rec r; if (cimp::parseRec(line, r)) S.recs.insert(std::make_pair(std::make_pair(r.trig, r.side), r));
        }
    }
}
void CobImpulse::status(CIState& S, long long now) {
    if (S.lastStatusMs >= 0 && now >= S.lastStatusMs && now - S.lastStatusMs < 1000) return;
    S.lastStatusMs = now;
    int tagged = 0, shownN = 0;
    for (const auto& kv : S.recs) { if (kv.second.r() || kv.second.v()) { ++tagged; if (cimp::shown(true, kv.second.abs, S.P.mode)) ++shownN; } }
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << CI_VERSION << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nSTATE," << S.state
      << "\nFLAGS," << S.flags << "\nPARAMS,mode " << cimp::modeName(S.P.mode) << ",proven " << (S.P.proven ? 1 : 0) << ",plain " << (S.P.plain ? 1 : 0)
      << ",min_bars " << S.P.minBars << ",window " << S.P.window << "," << S.paramsWhy
      << "\nRECORDS," << S.recs.size() << ",tagged " << tagged << ",tag shown " << shownN << ",pending " << S.pending.size() << ",fails " << S.recFails
      << "\nDECIDED_THROUGH," << S.decidedThrough << "\n";
    const std::string text = o.str(); if (text == S.lastStatusText) return;
    const std::string f = flexDir(); if (f.empty()) return;
    if (publishFile(f + "\\CobImpulse.status-" + stem(S) + ".txt", text)) S.lastStatusText = text;
}
void CobImpulse::pump(bool mayCalc) {
    CIState* P = slot_.get(this, true);
    if (!P || P->busy) return;
    CIState& S = *P; S.busy = true;
    struct Unbusy { CIState* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
    try {
        identify(S);
        const long long now = steadyMs();
        const long long ct = ctSec(currentDate());
        if (ct > 0) { const long long key = (ct - 17LL * 3600) / 86400LL + 1; if (key != S.session) { S.session = key; load(S); } }
        params(S, now);
        const long count = getBarCount();
        // his RTL is read (and recalculated by IRT) only from calc on a new bar, like the Dealer Profile / Chart View
        if (mayCalc && count != S.lastCount) { decide(S, count); S.lastCount = count; }
        else flush(S);
        status(S, now);
    } catch (...) { S.state = "calculation error"; }
}
// the bar index whose time is t (closed bars only), -1 = none
template <class Times> static int indexOf(Times& times, long count, long long t) {
    long lo = 0, hi = count - 2;
    while (lo <= hi) {
        const long mid = lo + (hi - lo) / 2; const long long v = (long long)times[(int)mid];
        if (v == t) return (int)mid;
        if (v < t) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}
void CobImpulse::render(CIState& S) {
    S.targets.clear();
    RCT pane; pane.getPaneRect(false);
    const long count = getBarCount();
    if (count < 3 || S.recs.empty() || pane.bottom - pane.top < 40) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow);
    if (times.count < count || hi.count < count || lo.count < count) return;
    FONT f; f.id = HELVETICA; f.size = 10; f.style = BOLD; setFont(f);
    for (const auto& kv : S.recs) {
        const cimp::Rec& r = kv.second;
        const bool tag = cimp::shown(r.r() || r.v(), r.abs, S.P.mode);
        if (!tag && !S.P.plain) continue;
        const int ti = indexOf(times, count, r.trig); if (ti < 0) continue;
        int ai = r.legEnd > 0 ? indexOf(times, count, r.legEnd) : -1; if (ai < 0) ai = ti > 0 ? ti - 1 : ti;
        PNT x0; x0.set(ai, 0.0f, kBarCenter); if (x0.h < pane.left - 40 || x0.h > pane.right + 40) continue;
        const std::string text = cimp::label(r.r(), r.v(), r.abs, S.P);
        const float ext = r.side > 0 ? hi[ai] : lo[ai];
        PNT pe; pe.set(ai, ext, kBarCenter);
        const int w = getTextWidth(text.c_str(), -1) + 4, h = 14;
        const cimp::Box lb = cimp::labelBox(pe.h, pe.v, r.side, w, h, 30, pane.top, pane.bottom);
        setTextColor(C_LABEL);
        RCT tr; tr.set(coord(lb.l), coord(lb.t), coord(lb.r), coord(lb.b)); tr.drawText(text.c_str(), true, false);
        HoverT H; H.key = std::to_string(r.trig) + (r.side > 0 ? "b" : "s"); H.l = lb.l; H.t = lb.t; H.r = lb.r; H.b = lb.b; H.rec = r;
        S.targets.push_back(H);
        if (!tag) continue;
        // the thin white box around the impulse bar(s)
        const long long bars[2] = { r.rT, r.vT };
        for (int k = 0; k < 2; ++k) {
            if (bars[k] <= 0 || (k == 1 && bars[1] == bars[0])) continue;
            const int bi = indexOf(times, count, bars[k]); if (bi < 0) continue;
            PNT a; a.set(bi, hi[bi], kBarLeft); PNT b; b.set(bi, lo[bi], kBarRight);
            const int l = std::min(a.h, b.h) - 2, rr = std::max(a.h, b.h) + 2, t = std::min(a.v, b.v) - 2, bt = std::max(a.v, b.v) + 2;
            RCT box; box.set(coord(l), coord(t), coord(rr), coord(bt)); box.draw(1, C_BOX, C_BOX, DRAW_OPAQUE, PAT_HOLLOW);
            HoverT HB = H; HB.l = l; HB.t = t; HB.r = rr; HB.b = bt; S.targets.push_back(HB);
        }
    }
    // the hover: only while the cursor is over this chart's window and over a label / box
    if (!S.msHas || S.msWnd == 0 || hb::hbCursor() != S.msWnd) { S.hovKey.clear(); return; }
    const HoverT* T = nullptr;
    for (const auto& t : S.targets) if (S.msH >= t.l && S.msH <= t.r && S.msV >= t.t && S.msV <= t.b) { T = &t; break; }
    S.hovKey = T ? T->key : std::string();
    if (!T) return;
    cimp::HoverIn in; in.side = T->rec.side; in.r = T->rec.r(); in.v = T->rec.v(); in.abs = T->rec.abs;
    in.rn = T->rec.rn; in.vn = T->rec.vn; in.rx = T->rec.rx; in.vx = T->rec.vx; in.rTime = clockOf(T->rec.rT); in.vTime = clockOf(T->rec.vT);
    std::vector<std::string> L = cimp::hover(in, S.P);
    L.insert(L.begin() + 1, std::string("CoB ") + (T->rec.side > 0 ? "long" : "short") + " at " + clockOf(T->rec.trig) + " (leg from the " + clockOf(T->rec.pivot) + " bar).");
    FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
    int w = 0; for (const auto& s : L) w = std::max(w, getTextWidth(s.c_str(), -1));
    w += 16; const int lh = 15, h = (int)L.size() * lh + 10;
    int l = S.msH + 14, t = S.msV + 14;
    if (l + w > pane.right - 2) l = S.msH - 14 - w;
    if (l < pane.left + 2) l = pane.left + 2;
    if (t + h > pane.bottom - 2) t = pane.bottom - 2 - h;
    if (t < pane.top + 2) t = pane.top + 2;
    RCT bg; bg.set(coord(l), coord(t), coord(l + w), coord(t + h)); bg.draw(1, C_HOVEDGE, C_HOVBG, DRAW_OPAQUE, PAT_SOLID);
    for (size_t i = 0; i < L.size(); ++i) {
        setTextColor(i == 0 ? C_LABEL : C_INK);
        RCT tr; tr.set(coord(l + 8), coord(t + 5 + (int)i * lh), coord(l + w - 4), coord(t + 5 + (int)(i + 1) * lh)); tr.drawText(L[i].c_str(), false, false);
    }
}
// the mouse: remember where it is; redraw only when the hovered label / box changes. Never swallows the event.
int CobImpulse::mouse(RTX_EVENT* e) {
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    CIState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    S->msHas = true; S->msH = e->v.mouse.h; S->msV = e->v.mouse.v; S->msWnd = hb::hbCursor();
    std::string k;
    for (const auto& t : S->targets) if (S->msH >= t.l && S->msH <= t.r && S->msV >= t.t && S->msV <= t.b) { k = t.key; break; }
    if (k != S->hovKey) { S->hovKey = k; invalidateChart(false); }
    return RTX_FAIL;
}

extern "C" cppExtension* CreateExtension(void) {
    CobImpulse* p = new CobImpulse();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE | CALC_LAST);   // CALC_LAST: it reads his RTL results
    p->setDescription("LRA Cob Impulse v0.1.0: did the CoB leg start with force? Labels each CoB (his cob_bull_3m_1b / cob_bear_3m_1b) Cob, Cob+R?, Cob+V? or Cob+RV?: R = a bar from the leg's pivot to 2 bars after it, inside the pivot bar, with the biggest range in 35+ bars; V = the biggest volume in 35+ bars. A thin white box marks the impulse bar; hover the label or box for the time, bars and ratio. Per-market file LRA-CobImpulse-<MKT>.csv: MODE off / abs (only with Abs on the pivot, default) / always; PROVEN 1 drops the ?. Decided at the CoB's close, recorded, never repainted.");
    p->setVersion("0.1.0");   // CI_VERSION
    return p;
}
