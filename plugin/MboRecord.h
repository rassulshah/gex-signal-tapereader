#ifndef LS_MBORECORD_H
#define LS_MBORECORD_H
// MboRecord.h (1.2.0, 2026-10-22) -- lsPullStats' "Record order feed" (Rassul approved 2026-10-13; settings audit 2026-10-14).
// 1.2.0 (2026-10-22, MBO recorder fix P1, Rassul 07:08 "complete the fixes"): rows are stamped by ARRIVAL time. IRT stamps a few
//   MOD / DEL / FILL rows with the resting order's own time and re-sends the resting book with old times after a re-subscription
//   (ES 10-19: 33 stale rows + 2,962 at the 17:08 re-subscription); such a row now gets the latest time already written, so it goes
//   to the right hour file and replays in feed order (ArrivalStamp.h; a jump back of 30 min or more = the DST hour, kept). The status
//   line adds "restamped N,max_back_s M". The host today is lsBookFlow (lsPullStats retired 2026-10-16).
// 1.1.0 (2026-10-14 evening, LIQ-DAY review "the ES MBO recorder drops events under load"): the hour file's path is built once per
//   clock hour (not per event) and the host keeps one clock conversion per second (tsKey / tsTm), so a 50,000-event burst costs a
//   snprintf + an append per event; the host (PullStats 1.3.0) now feeds every event read by its 250 ms order-feed clock and flushes
//   from that clock once a second (OrderFlowFeed.h 1.4.0). The format, files and one-writer rules are unchanged.
// SDK-free, unit-tested on Linux (tests_of/test_mborecord.cpp) and checked with lra.mbo_live's own parser.
//
// WHAT: every order-by-order event PullStats already drains (IRT DEPTH_BY_ORDER: NEW / MOD / DEL / FILL) is appended to
//   C:\Dev\level-reversal-analytics\data\irt\<session>\<MKT>\mbo_HH.csv
// in the format lra.mbo_live archives every night (the format lsPulling 0.2.0 writes, the IRT Reader's dbo format):
//   time|action|price|prev_price|size|order_id|aggressor_id|side|bid|ask
//   2026-10-14 09:31:07|FILL|6712.25|6712.25|3|123456|123999|B|6712.25|6712.5
// <session> = the 17:00 CT trading day of the event's own time (17:00 and later = the next day; Saturday / Sunday = Monday), as
// yyyy-mm-dd - the IRT Reader's / lsPulling's naming, which mbo_live expects. HH = the event's CT hour. A new file starts with the
// header; an existing one is appended to (across IRT restarts). mbo_live moves finished sessions into data\mbo\live\<MKT>\.
//
// HOW (lessons #6, #18, #31): events are formatted into an in-memory buffer as they are drained and written ONCE A SECOND, from
// the calc pass only (a draw never writes unless the buffer passes 4 MB); a buffer past 32 MB drops (counted). Nothing is ever
// retried in a loop. One writer per market per IRT: a process-wide registry gives the market to one PullStats chart (the others
// say so in their status). If ANOTHER program writes the same hour file (lsPulling 0.2.x still on a chart), the file grows by more
// than we wrote: this recorder stops and drops its lines (so no event is written twice) and resumes only after the file has stood
// still for 10 s. Under 2 GB free on the drive: paused with a status note (checked every 30 s).
#include <cerrno>
#include "ArrivalStamp.h"           // (1.2.0) arrival-time stamps
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <mutex>
#include <ostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#if !defined(_WIN32)
#include <sys/statvfs.h>
#endif

namespace mrec {

static const char* const VERSION = "1.2.0";
static const char* const HEADER = "time|action|price|prev_price|size|order_id|aggressor_id|side|bid|ask";
static const long long MIN_FREE = 2LL * 1024 * 1024 * 1024;           // 2 GB
static const size_t SOFT_CAP = 4u * 1024 * 1024, HARD_CAP = 32u * 1024 * 1024;
static const long long FLUSH_MS = 1000, DISK_MS = 30000, FOREIGN_MS = 10000, OWNER_STALE_MS = 5000;
#if defined(_WIN32)
static const char SEP = '\\';
#else
static const char SEP = '/';
#endif

// process-wide settings (tests only change them): the data root, and a fixed "free bytes" answer
inline std::string& rootOverride() { static std::string r; return r; }
inline long long& testFreeBytes() { static long long v = -1; return v; }
inline std::string root() {
    if (!rootOverride().empty()) return rootOverride();
#if defined(_WIN32)
    return "C:\\Dev\\level-reversal-analytics\\data\\irt";
#else
    return std::string();
#endif
}

// one event, its fields already in the chart's clock (Central)
struct Ev {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
    int action = 0;                       // 0 NEW 1 MOD 2 DEL 3 FILL (else "?")
    double price = 0, prev = 0, bid = 0, ask = 0;
    unsigned long size = 0;
    long long oid = 0, aid = 0;
    int side = -1;                        // 1 bid (B), 0 offer (S)
};

inline long long civil(long long y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
inline std::string ymdDash(long long days) {
    const long long z = days + 719468, era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153, day = doy - (153 * mp + 2) / 5 + 1, mon = mp < 10 ? mp + 3 : mp - 9; const long long yr = (long long)yoe + era * 400 + (mon <= 2);
    char b[40]; std::snprintf(b, sizeof(b), "%04lld-%02u-%02u", yr, mon % 100u, day % 100u); return b;
}
// the trading session of a CT clock time: 17:00 and later = the next day; Saturday / Sunday = Monday (mbo_live._today_session)
inline std::string sessionOf(int y, int mo, int d, int h) {
    if (mo < 1 || mo > 12 || d < 1 || d > 31 || h < 0 || h > 23) return std::string();
    long long dd = civil(y, (unsigned)mo, (unsigned)d) + (h >= 17 ? 1 : 0);
    for (int k = 0; k < 3; ++k) { const long long wd = ((dd + 4) % 7 + 7) % 7; if (wd != 0 && wd != 6) break; ++dd; }   // 0 Sun, 6 Sat
    return ymdDash(dd);
}
inline std::string pathOf(const std::string& rootDir, const std::string& market, const Ev& e) {
    const std::string s = sessionOf(e.y, e.mo, e.d, e.h);
    if (rootDir.empty() || market.empty() || s.empty()) return std::string();
    char hh[16]; std::snprintf(hh, sizeof(hh), "mbo_%02d.csv", e.h);
    return rootDir + SEP + s + SEP + market + SEP + hh;
}
inline std::string lineOf(const Ev& e) {
    static const char* ACT[4] = { "NEW", "MOD", "DEL", "FILL" };
    char b[256];
    std::snprintf(b, sizeof(b), "%04d-%02d-%02d %02d:%02d:%02d|%s|%.10g|%.10g|%lu|%lld|%lld|%c|%.10g|%.10g\n", e.y, e.mo, e.d, e.h, e.mi, e.s,
                  e.action >= 0 && e.action < 4 ? ACT[e.action] : "?", e.price, e.prev, e.size, e.oid, e.aid,
                  e.side == 1 ? 'B' : e.side == 0 ? 'S' : '?', e.bid, e.ask);
    return b;
}

inline long long fileSize(const std::string& p) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return -1; return (long long)st.st_size;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return -1; return (long long)st.st_size;
#endif
}
inline void makeDirOne(const std::string& d) {
#if defined(_WIN32)
    CreateDirectoryA(d.c_str(), NULL);
#else
    ::mkdir(d.c_str(), 0755);
#endif
}
inline void makeDirsFor(const std::string& rootDir, const std::string& file) {   // root, root\<session>, root\<session>\<MKT>
    makeDirOne(rootDir);
    size_t a = rootDir.size() + 1;
    for (;;) { const size_t c = file.find(SEP, a); if (c == std::string::npos) break; makeDirOne(file.substr(0, c)); a = c + 1; }
}
inline long long freeBytes(const std::string& rootDir) {
    if (testFreeBytes() >= 0) return testFreeBytes();
#if defined(_WIN32)
    ULARGE_INTEGER avail; avail.QuadPart = 0;
    const std::string drive = rootDir.size() >= 3 && rootDir[1] == ':' ? rootDir.substr(0, 3) : rootDir;
    if (!GetDiskFreeSpaceExA(drive.c_str(), &avail, NULL, NULL)) return -1;
    return (long long)avail.QuadPart;
#else
    struct statvfs v; std::string d = rootDir; if (statvfs(d.c_str(), &v) != 0) return -1;
    return (long long)v.f_bavail * (long long)v.f_frsize;
#endif
}

// ONE recorder per market per IRT process (one DLL serves every chart): who holds which market, last seen when
struct Registry { std::mutex m; std::map<std::string, std::pair<const void*, long long> > owner; };
inline Registry& registry() { static Registry r; return r; }
inline bool claim(const std::string& market, const void* me, long long now) {
    Registry& r = registry(); std::lock_guard<std::mutex> g(r.m);
    std::map<std::string, std::pair<const void*, long long> >::iterator it = r.owner.find(market);
    if (it == r.owner.end() || it->second.first == me || now - it->second.second > OWNER_STALE_MS || now < it->second.second) {
        r.owner[market] = std::make_pair(me, now); return true;
    }
    return false;
}
inline void drop(const std::string& market, const void* me) {
    Registry& r = registry(); std::lock_guard<std::mutex> g(r.m);
    std::map<std::string, std::pair<const void*, long long> >::iterator it = r.owner.find(market);
    if (it != r.owner.end() && it->second.first == me) r.owner.erase(it);
}

inline long long linesIn(const std::string& s) { long long n = 0; for (size_t i = 0; i < s.size(); ++i) if (s[i] == '\n') ++n; return n; }

struct Recorder {
    std::string market;
    bool on = false;                                  // the "Record order feed" box
    std::map<std::string, std::string> buf; size_t bufBytes = 0;
    std::string lastPath;
    long long lastFlushMs = -1, diskCheckMs = -1, freeNow = -1; bool lowDisk = false;
    std::map<std::string, long long> expect;          // file -> its size after our last append (another writer = a mismatch)
    bool foreign = false; std::string foreignPath; long long foreignSize = -1, foreignCheckMs = -1;
    bool owner = false;
    long long rows = 0, bytes = 0, fails = 0, dropped = 0, flushes = 0, biggestFlush = 0, lastNow = 0;
    std::string note = "waiting for the order feed";
    std::string dirsMade;
    unsigned long long tsKey = 0; struct tm tsTm;     // (1.1.0) the host's last clock conversion (one per event second)
    long long pathKey = -1; std::string pathNow;      // (1.1.0) the hour file of the last event (built once per clock hour)
    arr::Clock arrival;                               // (1.2.0) stale stamps -> the latest time already written

    Recorder() { std::memset(&tsTm, 0, sizeof(tsTm)); }
    Recorder(const Recorder&) = delete; Recorder& operator=(const Recorder&) = delete;
    ~Recorder() { try { if (on && !buf.empty()) flush(lastNow + FLUSH_MS); } catch (...) {} if (!market.empty()) drop(market, this); }

    void setMarket(const std::string& m) {
        if (m == market) return;
        if (on && !buf.empty()) flush(lastNow + FLUSH_MS);
        if (!market.empty()) drop(market, this);
        market = m; expect.clear(); foreign = false; owner = false; pathKey = -1; pathNow.clear(); arrival.reset();
    }
    void setOn(bool v) {
        if (v == on) return;
        on = v;
        if (!on) { dropBuffer(false); if (!market.empty()) drop(market, this); owner = false; note = "off (setting)"; }
        else note = "waiting for the order feed";
    }
    void dropBuffer(bool count) { if (count) for (std::map<std::string, std::string>::const_iterator i = buf.begin(); i != buf.end(); ++i) dropped += linesIn(i->second); buf.clear(); bufBytes = 0; }
    void add(const Ev& ev) {
        if (!on || market.empty()) return;
        Ev e = ev; arrival.apply(e.y, e.mo, e.d, e.h, e.mi, e.s);           // (1.2.0) arrival time
        const long long key = (((long long)e.y * 13 + e.mo) * 32 + e.d) * 24 + e.h;   // (1.1.0) one path per clock hour
        if (key != pathKey || pathNow.empty()) { pathNow = pathOf(root(), market, e); pathKey = pathNow.empty() ? -1 : key; }
        const std::string& p = pathNow;
        if (p.empty()) { ++dropped; return; }
        if (bufBytes >= HARD_CAP) { ++dropped; return; }
        const std::string l = lineOf(e);
        buf[p] += l; bufBytes += l.size(); lastPath = p;
    }
    // once a second from the calc pass; from a draw only when the buffer is past SOFT_CAP
    void tick(long long now, bool fromDraw) {
        lastNow = now;
        if (!on) return;
        const bool due = lastFlushMs < 0 || now - lastFlushMs >= FLUSH_MS || now < lastFlushMs;
        if (bufBytes >= SOFT_CAP || (due && !fromDraw)) flush(now);
    }
    void flush(long long now) {
        lastFlushMs = now;
        owner = claim(market, this, now);                                // also the heartbeat while idle
        if (!owner) { dropBuffer(false); note = "another PullStats chart of " + market + " is recording it"; return; }
        if (buf.empty()) return;
        const std::string rootDir = root();
        if (diskCheckMs < 0 || now - diskCheckMs >= DISK_MS || now < diskCheckMs) {
            diskCheckMs = now; freeNow = freeBytes(rootDir); lowDisk = freeNow >= 0 && freeNow < MIN_FREE;
        }
        if (lowDisk) {
            dropBuffer(true);
            char b[96]; std::snprintf(b, sizeof(b), "paused: %.1f GB free on the drive (< 2 GB)", (double)freeNow / 1073741824.0); note = b;
            return;
        }
        if (foreign) {
            if (now - foreignCheckMs >= FOREIGN_MS || now < foreignCheckMs) {
                const long long sz = fileSize(foreignPath);
                if (sz == foreignSize) { foreign = false; expect[foreignPath] = sz; }    // stood still: the other writer is gone
                else { foreignSize = sz; foreignCheckMs = now; }
            }
            if (foreign) { dropBuffer(true); note = "another recorder (lsPulling?) writes " + foreignPath + " - not writing, so no event is recorded twice"; return; }
        }
        long long wroteNow = 0;
        for (std::map<std::string, std::string>::const_iterator i = buf.begin(); i != buf.end(); ++i) {
            const std::string& p = i->first;
            const std::string dir = p.substr(0, p.rfind(SEP));
            if (dir != dirsMade) { makeDirsFor(rootDir, p); dirsMade = dir; }
            const long long sz = fileSize(p);
            std::map<std::string, long long>::iterator ex = expect.find(p);
            if (ex != expect.end() && sz != ex->second) {                    // the file changed under us: someone else writes it
                foreign = true; foreignPath = p; foreignSize = sz; foreignCheckMs = now;
                for (std::map<std::string, std::string>::const_iterator j = i; j != buf.end(); ++j) dropped += linesIn(j->second);
                note = "another recorder (lsPulling?) writes " + p + " - not writing, so no event is recorded twice";
                buf.clear(); bufBytes = 0; ++flushes; return;
            }
            std::ofstream f(p.c_str(), std::ios::binary | std::ios::app);
            if (!f) { ++fails; dropped += linesIn(i->second); expect.erase(p); continue; }
            std::string out;
            if (sz <= 0) { out = HEADER; out += '\n'; }                        // a new (or empty) hour file starts with the header
            out += i->second;
            f.write(out.data(), (std::streamsize)out.size()); f.flush();
            if (!f) { ++fails; dropped += linesIn(i->second); expect.erase(p); continue; }   // never retried (the feed keeps coming)
            rows += linesIn(i->second); bytes += (long long)out.size(); wroteNow += (long long)out.size();
            expect[p] = (sz > 0 ? sz : 0) + (long long)out.size();
        }
        if (expect.size() > 48) { std::map<std::string, long long> keep; for (std::map<std::string, std::string>::const_iterator i = buf.begin(); i != buf.end(); ++i) { std::map<std::string, long long>::iterator e = expect.find(i->first); if (e != expect.end()) keep.insert(*e); } expect.swap(keep); }
        if (wroteNow > biggestFlush) biggestFlush = wroteNow;
        buf.clear(); bufBytes = 0; ++flushes;
        note = fails ? "recording (some writes failed)" : "recording";
    }
    void status(std::ostream& o) const {
        o << "\nMBO_RECORD," << (on ? note : std::string("off (setting)")) << ",rows " << rows << ",bytes " << bytes << ",fails " << fails
          << ",dropped " << dropped << ",restamped " << arrival.restamped << ",max_back_s " << arrival.maxBackS << ",flushes " << flushes << ",biggest_flush " << biggestFlush
          << ",free_gb " << (freeNow < 0 ? std::string("n/a") : std::to_string(freeNow / 1073741824LL)) << ",file " << (lastPath.empty() ? std::string("none yet") : lastPath)
          << "\nMBO_RECORD_FORMAT," << HEADER << " (data\\irt\\<session>\\<MKT>\\mbo_HH.csv, archived nightly by lra.mbo_live)";
    }
};

} // namespace mrec
#endif
