// SRBack.h (1.0.0, 2026-11-16) - lsSR's stop-run engine running in the BACKGROUND of another order-feed pane (lsRevRead 0.8.0).
// Rassul 2026-11-16 06:00 CT: "i dont have space for too many indicators so can you take the information you get from ... SR and put it in the
// reversal read indicator in the background. so the reversal read would carry that informaiton and continue to only show what it shows now".
// The host hands every order event here; this feeds sr::Engine (SRLogic.h - the ONE copy of the rules, the same parameters as lsSR) and
// appends its decisions to lsSR's own journal, lsFlexLevels\SR\<MKT>-<spb>-<session>.csv (the nightly sr_score / cob_reader read it there).
// Nothing is drawn. ONE WRITER per journal: when lsSR is on a chart of the same market too, whichever opened the journal first writes
// (a lock file held open); the other only reads. The feed path below is lsSR 1.1.3's onRaw / openSession / flush, line for line.
// Include AFTER irtsdk.h and OrderFlowFeed.h (uses DEPTH_ORDER, off::evOf, nl::levels).
#pragma once
#include "SRLogic.h"
#include "SRParams.h"
#include "SRGradeParams.h"
#include "MarketKey.h"
#include "NearLevels.h"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <sys/stat.h>

namespace srb {
static const char* const VERSION = "1.0.0";
static const long long IDLE_MS = 5000;
inline std::string flexDir() { const char* u = std::getenv("USERPROFILE"); return u && *u ? std::string(u) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string(); }
inline void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0700);
#endif
}
inline bool stat2(const std::string& p, long long& mt, long long& sz) { struct stat s; if (stat(p.c_str(), &s) != 0) return false; mt = (long long)s.st_mtime; sz = (long long)s.st_size; return true; }
inline std::string journalDir(const std::string& flex) { return flex.empty() ? std::string() : flex + "\\SR"; }
inline std::string journalPath(const std::string& flex, const std::string& mkt, int spb, const std::string& session) {
    return flex.empty() || mkt.empty() || spb <= 0 || session.empty() ? std::string() : journalDir(flex) + "\\" + mkt + "-" + std::to_string(spb) + "-" + session + ".csv";
}
inline bool known(const std::string& m) { static const char* const K[] = { "ES", "NQ", "CL", "NG", "GC", "HG", "EU" }; for (const char* k : K) if (m == k) return true; return false; }
inline std::string upper(std::string r) { for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]); return r; }
inline std::string marketOf(const std::string& root, const std::string& symbol = std::string()) {
    std::string m = mkey::norm(upper(root)); if (known(m)) return m;
    m = mkey::norm(upper(symbol)); if (known(m)) return m;
    const std::string s = upper(symbol); if (s.size() > 3) { m = mkey::norm(s.substr(0, s.size() - 2)); if (known(m)) return m; }
    return std::string();
}
struct Owners { std::map<std::string, const void*> by; static Owners& get() { static Owners o; return o; } };
inline bool claim(const std::string& path, const void* who, void*& handle) {
    if (path.empty()) return false;
    std::map<std::string, const void*>::iterator it = Owners::get().by.find(path);
    if (it != Owners::get().by.end()) return it->second == who;
#if defined(_WIN32)
    HANDLE h = CreateFileA((path + ".lock").c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;                  // lsSR (or another program) writes this journal
    handle = (void*)h;
#else
    handle = nullptr;
#endif
    Owners::get().by[path] = who; return true;
}
inline void unclaim(const std::string& path, const void* who, void*& handle) {
    std::map<std::string, const void*>::iterator it = Owners::get().by.find(path);
    if (it != Owners::get().by.end() && it->second == who) Owners::get().by.erase(it);
#if defined(_WIN32)
    if (handle) CloseHandle((HANDLE)handle);
#endif
    handle = nullptr;
}
inline std::string sessionOf(const struct tm& t) {               // 17:00 CT starts the next trade date (= lsSR's srh::sessionOf)
    struct tm x = t; x.tm_isdst = -1; if (x.tm_hour >= 17) x.tm_mday += 1; x.tm_hour = 12; x.tm_min = 0; x.tm_sec = 0;
    std::time_t tt = std::mktime(&x); struct tm y; std::memset(&y, 0, sizeof y);
#if defined(_WIN32)
    localtime_s(&y, &tt);
#else
    localtime_r(&tt, &y);
#endif
    char b[40]; std::snprintf(b, sizeof b, "%04d-%02d-%02d", y.tm_year + 1900, y.tm_mon + 1, y.tm_mday); return b;
}
inline long long daysOfTm(const struct tm& t) { long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, d = (unsigned)t.tm_mday; y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; return era * 146097 + (long long)doe - 719468; }
struct JFile {                                                    // the journal read incrementally (append-only); the model only grows
    std::string path; long long off = 0, mt = -1, sz = -1; sr::Journal J; std::string partial;
    bool poll() {
        long long m = 0, s = 0; if (path.empty() || !stat2(path, m, s)) return false;
        if (s < off) { off = 0; J = sr::Journal(); partial.clear(); }
        if (s == sz && m == mt) return false;
        std::ifstream in(path.c_str(), std::ios::binary); if (!in.is_open()) return false;
        in.seekg(off); std::string chunk((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        off += (long long)chunk.size(); mt = m; sz = s;
        std::string all = partial + chunk; size_t a = 0, nl;
        while ((nl = all.find('\n', a)) != std::string::npos) { J.add(all.substr(a, nl - a)); a = nl + 1; }
        partial = all.substr(a);
        if (partial.size() > 4096) partial.clear();
        return true;
    }
};

struct Back {
    std::unique_ptr<sr::Engine> eng; std::string mkt, session, path, note, lvlNote; int spb = 0; double tick = 0;
    size_t written = 0; long long writeFails = 0, events = 0, skipped = 0, sessions = 0, fills = 0, faults = 0;
    long long stamp = -1, localSec = 0, secArr0 = 0; bool haveOff = false;
    JFile jf; nl::Cache lvc; long long lvlSig = -1; size_t nLevels = 0;
    bool writer = false; void* lock = nullptr; long long tornFixed = 0, lastTick = 0;
    Back() {}
    Back(const Back&) = delete; Back& operator=(const Back&) = delete;
    ~Back() { try { stop(); } catch (...) {} }

    void refreshLevels(const std::string& symbol) {
        const std::string flex = flexDir(); if (flex.empty() || !(tick > 0) || !eng) return;
        const std::vector<nl::Level> v = nl::levels(lvc, flex + "\\", mkt, symbol, off::steadyMs(), (long long)std::time(nullptr));
        std::vector<sr::Level> out; unsigned long long sig = 1469598103ULL;
        for (size_t i = 0; i < v.size(); ++i) { if (!(v[i].px > 0) || v[i].name.empty()) continue;
            sr::Level l; l.name = v[i].name; for (size_t c = 0; c < l.name.size(); ++c) if (l.name[c] == '|' || l.name[c] == ',' || l.name[c] == '\n' || l.name[c] == '\r') l.name[c] = ' ';
            l.px = (long long)std::llround(v[i].px / tick); l.side = sr::Engine::sideOfName(l.name); out.push_back(l);
            for (size_t c = 0; c < l.name.size(); ++c) sig = sig * 131 + (unsigned char)l.name[c];
            sig = sig * 1000003ULL + (unsigned long long)l.px; }
        lvlNote = lvc.note;
        const long long sg = (long long)(sig & 0x3FFFFFFFFFFFFFFFULL); if (sg == lvlSig && out.size() == nLevels) return;
        lvlSig = sg; nLevels = out.size(); eng->setLevels(out);
    }
    void openSession(const std::string& sess, const std::string& m, int spbNow, double tickNow) {
        if (eng) { if (haveOff && writer) eng->advance(localSec * 1000); flush(); }
        if (writer) unclaim(path, this, lock);
        writer = false;
        mkt = m; spb = spbNow; tick = tickNow; session = sess; ++sessions;
        eng.reset(new (std::nothrow) sr::Engine()); if (!eng) return;
        const srp::Mkt& M = srp::forMarket(mkt); eng->spb = spb; eng->rthOpenSec = M.rthOpen * 60LL; eng->rthCloseSec = M.rthClose * 60LL; eng->P.recBars = M.recBars; eng->P.drawRev = M.drawRev; eng->P.burstX = M.burstX;
        { const srg::Mkt& G = srg::forMarket(mkt); eng->P.bigA = G.bigA; eng->P.bigP = G.bigP; eng->P.gShare = G.share; eng->P.gBids = G.bids; eng->P.gDeep = G.deep; eng->P.gHeavy = G.heavy; eng->P.gStrong = G.strong; }
        written = 0; lvlSig = -1; nLevels = 0; lastTick = 0;
        const std::string flex = flexDir(); makeDir(flex); makeDir(journalDir(flex));
        path = journalPath(flex, mkt, spb, sess);
        writer = claim(path, this, lock);
        jf = JFile(); jf.path = path; jf.poll();
        eng->setNextId(jf.J.maxId + 1000);
        if (!writer || path.empty()) return;
        long long mt = 0, sz = 0;
        if (!stat2(path, mt, sz)) { std::ofstream f(path.c_str(), std::ios::binary | std::ios::app); char b[64]; std::snprintf(b, sizeof b, "H|%.10g|%s\n", tick, sr::VERSION); f << b; return; }
        { std::ifstream in(path.c_str(), std::ios::binary); if (sz > 0) { in.seekg(sz - 1); char c = 0; in.get(c); if (c != '\n') { std::ofstream f(path.c_str(), std::ios::binary | std::ios::app); f << "\n"; ++tornFixed; } } }
    }
    // one order event. newSec = the local time of this event's second when the second changed (else null): the host converts the clock once a second
    void onRaw(const DEPTH_ORDER& o, const struct tm* newSec, const std::string& root, const std::string& symbol, int spbNow, double tickNow) {
        const std::string m = marketOf(root, symbol);
        if (m.empty()) { note = "market not supported (" + root + ")"; ++skipped; return; }
        if (spbNow <= 0 || spbNow > 3600 || 3600 % spbNow != 0) { note = "needs 1-60 min bars that divide the hour"; ++skipped; return; }
        if (!(tickNow > 0) || !o.timestamp) { ++skipped; return; }
        note.clear();
        const long long arr = off::steadyMs();
        if (newSec) {
            stamp = (long long)o.timestamp; localSec = daysOfTm(*newSec) * 86400LL + newSec->tm_hour * 3600LL + newSec->tm_min * 60LL + newSec->tm_sec; haveOff = true; secArr0 = arr;
            const std::string sess = sessionOf(*newSec);
            if (!eng || sess != session || m != mkt || spbNow != spb) openSession(sess, m, spbNow, tickNow);
            if (eng) refreshLevels(symbol);
        }
        if (!eng || !writer || !haveOff || (long long)o.timestamp != stamp) return;
        off::Ev x = off::evOf(o, tickNow, arr);
        if (x.action > 3 || x.px <= 0) { ++skipped; return; }
        long long ms = arr - secArr0; if (ms < 0) ms = 0;
        if (ms > 999) ms = 999;
        long long tMs = localSec * 1000 + ms; if (tMs < lastTick) tMs = lastTick;
        lastTick = tMs; ++events;
        if (x.action == off::FILL) { if (x.size <= 0) return;
            if (x.implied || x.side < 0) { eng->onVolume(tMs, x.size); return; }
            ++fills;
            eng->onFill(tMs, (long long)x.px, x.size, x.side == off::BID ? -1 : 1, (unsigned long long)(x.aggr ? x.aggr : x.id), (unsigned long long)(x.aggr ? x.id : 0)); }
        else if (x.action == off::NEW) eng->onBook(tMs, x.side, true, (long long)x.px, x.size);
        else if (x.action == off::DEL) eng->onBook(tMs, x.side, false, (long long)x.px, x.size);
        else eng->advance(tMs);
    }
    void flush() {
        if (!eng || !writer || path.empty() || written >= eng->journal.size()) return;
        std::ofstream f(path.c_str(), std::ios::binary | std::ios::app);
        if (!f.is_open()) { ++writeFails; return; }
        for (; written < eng->journal.size(); ++written) { const std::string& l = eng->journal[written];
            if (!jf.J.bars.empty() && l.size() > 2 && l[0] == 'B') { const long long end = std::atoll(l.c_str() + 2); if (jf.J.bars.count(end)) continue; }   // a candle decided before the restart stays as it was
            f << l << "\n"; }
        f.flush(); if (!f.good()) ++writeFails;
        if (eng->journal.size() > 20000) { eng->journal.clear(); written = 0; }
    }
    void afterPump() {                                            // a quiet feed: time still passes (candles close, pending runs are decided)
        if (eng && haveOff && writer) { const long long idle = off::steadyMs() - secArr0; if (idle > IDLE_MS + 1000) eng->advance(localSec * 1000 + std::min<long long>(idle - IDLE_MS, 3600000LL)); }
        flush(); jf.poll();
    }
    void stop() { if (eng && writer) flush(); if (writer) unclaim(path, this, lock); writer = false; }
    std::string status() const {
        std::string o = std::string("SR_BACKGROUND,SRBack ") + VERSION + ",rules lsSR " + sr::VERSION + ",market " + (mkt.empty() ? std::string("?") : mkt)
            + (!eng ? ",waiting for the order feed" : writer ? ",writer" : ",read-only (lsSR or another chart writes this journal)") + ",session " + session
            + ",events " + std::to_string(events) + ",fills " + std::to_string(fills) + ",skipped " + std::to_string(skipped) + ",levels " + std::to_string(nLevels) + " (" + lvlNote + ")"
            + (note.empty() ? std::string() : ",note " + note) + ",journal " + path + ",lines " + std::to_string(jf.J.lines) + ",gold runs " + std::to_string(jf.J.gold.size()) + ",reversals " + std::to_string(jf.J.rev.size())
            + ",write fails " + std::to_string(writeFails) + ",faults " + std::to_string(faults);
        return o;
    }
};
} // namespace srb
