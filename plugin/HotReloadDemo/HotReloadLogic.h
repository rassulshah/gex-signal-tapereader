#ifndef LS_HOT_RELOAD_LOGIC_H
#define LS_HOT_RELOAD_LOGIC_H
/********************************************************************************
 *  HotReloadLogic.h  --  hot reload for Investor/RT RTX indicators, the SDK-free and Win32-free half   1.0.0 (2026-10-18)
 *
 *  Rassul 2026-10-18 15:17 CT: "is there a way for you to do the build without me having to restart every time, maybe i can just
 *  refresh the chart instead?"
 *
 *  WHAT IRT ACTUALLY CALLS (verified in irtsdkV143-x64.lib 15.1.6, see docs/build/HOT-RELOAD.md):
 *   - an RTX DLL exports ONE function: cpp_main(void *pAPI)   (irtsdk.obj .drectve "/EXPORT:cpp_main", nothing else)
 *   - pAPI points to the host's per-instance struct; offset 0 = the reason (int, cpp_main switches on it, 28 reasons),
 *     offset 0xA0 = the per-chart user-data pointer (getUserData / setUserData read and write it, nothing else),
 *     offset 0xB0 = checkversion, written by REASON_INIT as 0x24A5A (= SDK 15.1.6) - used here to prove the layout.
 *   - REASON_CALC always runs calc(0) (a full recalculation); REASON_UPDT runs updt() = calc(bars - 2).
 *   - set*Parameter only works inside REASON_SETUP (allocParameter checks the reason) -> parameters are declared ONCE per IRT
 *     session, by the first implementation. A later build cannot change the settings list: the shim refuses it ("restart needed").
 *
 *  SO THE SHIM (HotShim.cpp) IS A PLAIN FORWARDER: it exports cpp_main and passes every call to the cpp_main of an implementation
 *  DLL (a normal RTX build, renamed and copied to InvestorRT\hotreload\). Per chart it keeps a small record in the user-data slot and
 *  swaps the implementation's own pointer in and out around each call, so each build only ever sees its own state.
 *  When a newer build is published (manifest), the next full recalculation of a chart (refresh) closes that chart's state on the old
 *  build (REASON_DONE + REASON_DESTR, exactly what IRT sends when an indicator is removed) and starts it fresh on the new build.
 *  Old builds are NEVER unloaded (IRT may still hold pointers into them, e.g. the version text): they are only no longer used.
 *
 *  This header: the manifest parser, the build handshake, the router / swap state machine, the status text. Tested on Linux with
 *  fake modules (test_hotreload_logic.cpp). HotShim.cpp adds LoadLibrary / GetProcAddress / file stamps / the lock / SEH.
 ********************************************************************************/
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace hotreload {

static const char* const SHIM_VERSION = "1.0.0";
static const uint32_t ABI = 1;                       // shim <-> implementation C interface version (HrInfo / HrHost)

// ---- the host struct (IRTAPI) offsets this shim relies on - verified from the SDK lib's own machine code ----
static const size_t OFF_REASON = 0x00;               // cpp_main: movslq (%rcx) ; cmp $0x1b ; jump table
static const size_t OFF_USERDATA = 0xA0;             // getUserData: mov 0xa0(pAPI) ; setUserData: mov %rdx, 0xa0(pAPI)
static const size_t OFF_CHECKVERSION = 0xB0;         // REASON_INIT: movl $0x24a5a, 0xb0(pAPI)
static const int SDK_CHECKVERSION = 0x24A5A;         // 150106 = SDK 15.1.6
static const size_t API_MIN_BYTES = 0xB4;

enum Reason { R_INIT = 0, R_CALC = 1, R_UPDT = 2, R_DONE = 3, R_SETUP = 4, R_DRAW = 5, R_DESTR = 6 };
static const int RTX_FAIL_RC = -1;

inline int readReason(const void* api) { int r; std::memcpy(&r, static_cast<const char*>(api) + OFF_REASON, sizeof r); return r; }
inline void writeReason(void* api, int r) { std::memcpy(static_cast<char*>(api) + OFF_REASON, &r, sizeof r); }
inline void* readUser(const void* api) { void* p; std::memcpy(&p, static_cast<const char*>(api) + OFF_USERDATA, sizeof p); return p; }
inline void writeUser(void* api, void* p) { std::memcpy(static_cast<char*>(api) + OFF_USERDATA, &p, sizeof p); }
inline int readCheck(const void* api) { int v; std::memcpy(&v, static_cast<const char*>(api) + OFF_CHECKVERSION, sizeof v); return v; }

// ---- the C interface between the shim and an implementation DLL (ABI 1) ----
// The implementation exports:  extern "C" int  hr_info(HrInfo* out)      (returns 1 = filled; requires out->size == sizeof(HrInfo))
//                              extern "C" void hr_attach(const HrHost* h)  (optional: gives it the shim's one-line status for its title)
extern "C" {
struct HrInfo {
    uint32_t size;          // set by the caller = sizeof(HrInfo)
    uint32_t abi;           // = ABI
    int64_t  build;         // the build number the build script compiled in (/DHR_BUILD=N), = the manifest BUILD
    char     name[48];      // "lsHotReloadDemo" - must equal the shim DLL's name
    char     version[24];   // "1.0.1" - shown in status / on the chart
    uint32_t paramSig;      // FNV-1a of the settings list text: must equal the first build's (IRT registered those settings)
    int32_t  arrayCount;    // must equal the first build's (IRT allocated these output arrays at INIT)
    uint32_t flags;         // must equal the first build's (IRT read the flags at INIT)
    uint32_t flagsExt;      // must equal the first build's
};
struct HrHost {
    uint32_t size;
    uint32_t abi;
    int (*status)(char* buf, int cap);   // copies the shim's one-line status ("" = all good); returns its length
};
}

inline uint32_t fnv1a(const char* s)
{
    uint32_t h = 2166136261u;
    for (; s && *s; ++s) { h ^= (unsigned char)*s; h *= 16777619u; }
    return h;
}

// ---- manifest:  <InvestorRT>\hotreload\<name>.manifest, written by tools\gex-hot-build.bat (tmp + move = atomic) ----
//   HOTRELOAD 1
//   NAME lsHotReloadDemo
//   BUILD 3
//   FILE lsHotReloadDemo_impl_3.dll
//   SWAP refresh            (optional: refresh = on a full recalculation only; tick = also on the next update)
struct Manifest {
    std::string name;
    long long build = 0;
    std::string file;
    bool swapOnTick = false;
};

inline bool safeFileName(const std::string& f)
{
    if (f.size() < 5 || f.size() > 120) return false;
    for (char c : f) {
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        if (!ok) return false;
    }
    if (f.find("..") != std::string::npos) return false;
    return f.compare(f.size() - 4, 4, ".dll") == 0;
}

inline bool fullTokenInt64(const std::string& s, long long& out)
{
    if (s.empty() || s.size() > 18) return false;
    for (char c : s) if (c < '0' || c > '9') return false;
    errno = 0; char* end = nullptr;
    long long v = std::strtoll(s.c_str(), &end, 10);
    if (errno != 0 || !end || *end != '\0') return false;
    out = v; return true;
}

inline bool parseManifest(const std::string& text, const std::string& expectName, Manifest& out, std::string& why)
{
    Manifest m;
    if (text.size() > 4096) { why = "manifest too big"; return false; }
    bool header = false, haveName = false, haveBuild = false, haveFile = false, haveSwap = false;
    size_t i = 0;
    while (i <= text.size()) {
        size_t e = text.find('\n', i);
        if (e == std::string::npos) e = text.size();
        std::string line = text.substr(i, e - i);
        i = e + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
        size_t b = 0; while (b < line.size() && (line[b] == ' ' || line[b] == '\t')) ++b;
        line = line.substr(b);
        if (line.empty() || line[0] == '#') { if (e == text.size()) break; continue; }
        size_t sp = line.find(' ');
        std::string key = line.substr(0, sp), val = sp == std::string::npos ? "" : line.substr(sp + 1);
        while (!val.empty() && val[0] == ' ') val.erase(0, 1);
        if (!header) {
            if (key != "HOTRELOAD" || val != "1") { why = "first line must be HOTRELOAD 1"; return false; }
            header = true;
        } else if (key == "NAME") {
            if (haveName) { why = "NAME twice"; return false; }
            haveName = true; m.name = val;
        } else if (key == "BUILD") {
            if (haveBuild) { why = "BUILD twice"; return false; }
            haveBuild = true;
            if (!fullTokenInt64(val, m.build) || m.build <= 0) { why = "BUILD is not a positive whole number"; return false; }
        } else if (key == "FILE") {
            if (haveFile) { why = "FILE twice"; return false; }
            haveFile = true; m.file = val;
        } else if (key == "SWAP") {
            if (haveSwap) { why = "SWAP twice"; return false; }
            haveSwap = true;
            if (val == "tick") m.swapOnTick = true;
            else if (val == "refresh") m.swapOnTick = false;
            else { why = "SWAP must be refresh or tick"; return false; }
        } else { why = "unknown key " + key; return false; }
        if (e == text.size()) break;
    }
    if (!header) { why = "empty manifest"; return false; }
    if (!haveName || !haveBuild || !haveFile) { why = "manifest needs NAME, BUILD and FILE"; return false; }
    if (m.name != expectName) { why = "manifest is for " + m.name + ", not " + expectName; return false; }
    if (!safeFileName(m.file)) { why = "FILE is not a plain .dll file name"; return false; }
    const std::string prefix = expectName + "_impl_";
    if (m.file.compare(0, prefix.size(), prefix) != 0) { why = "FILE must start with " + prefix; return false; }
    out = m; return true;
}

// ---- the build handshake ----
enum class Verdict { Accept, NoInfo, BadAbi, WrongName, WrongBuild, ParamsChanged, ArraysChanged, FlagsChanged };

inline const char* verdictText(Verdict v)
{
    switch (v) {
    case Verdict::Accept:        return "accepted";
    case Verdict::NoInfo:        return "not a hot-reload build (no hr_info) - restart Investor/RT once";
    case Verdict::BadAbi:        return "made for another shim version - restart Investor/RT once";
    case Verdict::WrongName:     return "belongs to another indicator - refused";
    case Verdict::WrongBuild:    return "build number does not match the manifest - refused";
    case Verdict::ParamsChanged: return "the settings list changed - restart Investor/RT once";
    case Verdict::ArraysChanged: return "the output count changed - restart Investor/RT once";
    case Verdict::FlagsChanged:  return "the drawing flags changed - restart Investor/RT once";
    }
    return "?";
}

inline bool fixedEq(const char* a, size_t cap, const std::string& b)
{
    size_t n = 0; while (n < cap && a[n]) ++n;
    if (n == cap) return false;                              // not terminated = garbage
    return b.size() == n && std::memcmp(a, b.data(), n) == 0;
}

// baseline = the build IRT loaded at start (it declared the settings, outputs and flags); null when checking that first build
inline Verdict checkInfo(const HrInfo& c, const HrInfo* baseline, const std::string& name, long long manifestBuild)
{
    if (c.size != sizeof(HrInfo) || c.abi != ABI) return Verdict::BadAbi;
    if (!fixedEq(c.name, sizeof c.name, name)) return Verdict::WrongName;
    if (c.build != manifestBuild) return Verdict::WrongBuild;
    if (baseline) {
        if (c.paramSig != baseline->paramSig) return Verdict::ParamsChanged;
        if (c.arrayCount != baseline->arrayCount) return Verdict::ArraysChanged;
        if (c.flags != baseline->flags || c.flagsExt != baseline->flagsExt) return Verdict::FlagsChanged;
    }
    return Verdict::Accept;
}

// ---- what the router needs from the platform (Win32 in HotShim.cpp, fakes in the tests) ----
struct Module {
    virtual ~Module() {}
    virtual int call(void* api, bool& faulted) = 0;          // the implementation's cpp_main; faulted = it crashed (SEH caught it)
    virtual bool info(HrInfo& out) = 0;                       // hr_info; false = not exported / refused
    virtual void attach(const HrHost* host) = 0;              // hr_attach if exported
};
struct Platform {
    virtual ~Platform() {}
    virtual bool readManifest(std::string& text, unsigned long long& stamp) = 0;   // false = no manifest
    virtual Module* load(const std::string& file, std::string& why) = 0;            // file = plain name inside the hotreload folder
    virtual void discard(Module* m) = 0;                     // a refused build nothing has called into yet (may be unloaded)
    virtual unsigned long long nowMs() = 0;
    virtual void writeStatus(const std::string& text) = 0;
    virtual void lock() = 0;
    virtual void unlock() = 0;
};

class Router {
public:
    static const int MAX_BUILDS = 16;                       // per IRT session; then "restart to load more"
    static const unsigned long long CHECK_EVERY_MS = 1000;

    Router(Platform& p, const std::string& name) : P(p), name_(name) {}
    ~Router()                                                // (the shim's router lives until IRT exits; tests destroy theirs)
    {
        for (Rec* r : live_) delete r;
        for (Gen* g : gens_) delete g;                       // the modules are the platform's: never unloaded here
    }
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    // the one entry point: everything IRT sends to the shim's cpp_main
    int onCall(void* api)
    {
        if (!api) return RTX_FAIL_RC;
        const int reason = readReason(api);
        if (reason == R_INIT || reason == R_SETUP) return onStartCall(api, reason);

        Guard g(P);
        if (gens_.empty()) return RTX_FAIL_RC;
        if (passThrough_) { Gen* g0 = gens_[0]; g.release(); bool f = false; return g0->m->call(api, f); }

        void* ud = readUser(api);
        Rec* rec = nullptr;
        bool nested = false;
        if (ud && recs_.count(ud)) rec = static_cast<Rec*>(ud);
        if (!rec) {                                          // a call made while this chart is already inside its build?
            for (Rec* r : live_) {
                if (r->depth > 0 && (r->api == api || (ud && r->implUD == ud))) { rec = r; nested = true; break; }
            }
        }
        if (!rec && ud) {                                    // a pointer we never made: leave it alone, plain forwarding
            Gen* cur = gens_[current_]; g.release(); bool f = false; return cur->m->call(api, f);
        }
        if (!rec) {
            rec = new Rec();
            rec->gen = current_;
            recs_.insert(rec); live_.push_back(rec);
            gens_[current_]->charts++;
            writeUser(api, rec);
            statusDirty_ = true;
        }
        if (nested) {                                        // the user-data slot already holds the build's own pointer
            Gen* gen = gens_[rec->gen];
            rec->depth++;
            g.release();
            bool f = false;
            int rc = gen->m->call(api, f);
            g.take();
            rec->depth--;
            if (f) noteFault(rec->gen, reason);
            return rc;
        }

        // ---- swap decision: only at depth 0, only on a full recalculation (or the next update with SWAP tick) ----
        // (the manifest is looked at, at most once a second, on any calc or update - so "build N ready" shows without a refresh)
        if (rec->depth == 0 && (reason == R_CALC || reason == R_UPDT)) checkUpdatesLocked();
        bool fullCalc = reason == R_CALC;
        bool tick = reason == R_UPDT && swapOnTick_;
        int from = rec->gen;
        bool move = false, closeOld = false, promote = false;
        if (rec->depth == 0 && (fullCalc || tick)) {
            int target = current_;
            if (!gens_[target]->bad && (rec->dead || rec->gen != target)) {
                move = true; closeOld = !rec->dead && !gens_[from]->bad; promote = reason == R_UPDT;
                rec->gen = target; rec->dead = false;
                gens_[from]->charts--; gens_[target]->charts++;
                swaps_++; statusDirty_ = true;
            }
        }
        if (rec->dead || gens_[rec->gen]->bad) {             // its build crashed: blank until a good build takes it over
            flushStatusLocked();
            if (reason == R_DESTR) dropRec(rec, api);
            return RTX_FAIL_RC;
        }
        rec->depth++;
        rec->api = api;
        void* oldUD = rec->implUD;
        Gen* oldGen = gens_[from];
        Gen* gen = gens_[rec->gen];
        flushStatusLocked();
        g.release();

        if (move && closeOld) {                              // close this chart on the old build, as if the indicator were removed
            bool f1 = false, f2 = false;
            writeUser(api, oldUD);
            writeReason(api, R_DONE);  oldGen->m->call(api, f1);
            if (!f1) { writeReason(api, R_DESTR); oldGen->m->call(api, f2); }
            writeReason(api, reason);
            if (f1 || f2) { g.take(); noteFault(from, R_DESTR); g.release(); }
        }
        if (move) rec->implUD = nullptr;                     // the new build starts this chart fresh (bars + its record files)
        writeUser(api, rec->implUD);
        if (promote) writeReason(api, R_CALC);               // first call on the new build is always a full calc(0)
        bool faulted = false;
        int rc = gen->m->call(api, faulted);
        if (promote) writeReason(api, reason);
        rec->implUD = readUser(api);
        writeUser(api, rec);

        g.take();
        rec->depth--;
        if (rec->depth == 0) rec->api = nullptr;
        if (faulted) {
            noteFault(rec->gen, reason);
            rec->dead = true;                                // its state may be corrupt: abandoned (never touched again)
            rc = RTX_FAIL_RC;
        }
        if (reason == R_DESTR && rec->depth == 0) dropRec(rec, api);
        flushStatusLocked();
        return rc;
    }

    // one line for the implementation to show next to its title ("" = nothing to say)
    std::string shortStatus()
    {
        Guard g(P);
        return shortStatusLocked();
    }
    std::string statusText() { Guard g(P); return statusTextLocked(); }

    // for tests
    int buildCount() { Guard g(P); return (int)gens_.size(); }
    long long activeBuild() { Guard g(P); return gens_.empty() ? 0 : gens_[current_]->build; }
    bool passThrough() { Guard g(P); return passThrough_; }
    int chartsOn(long long build) { Guard g(P); for (Gen* x : gens_) if (x->build == build) return x->charts; return -1; }
    long long refusedBuild() { Guard g(P); return refusedBuild_; }
    std::string refusedWhy() { Guard g(P); return refusedWhy_; }
    int liveRecords() { Guard g(P); return (int)live_.size(); }

private:
    struct Gen { Module* m = nullptr; HrInfo info{}; long long build = 0; bool bad = false; int charts = 0; std::string version; };
    struct Rec { int gen = 0; void* implUD = nullptr; int depth = 0; void* api = nullptr; bool dead = false; };
    struct Guard {
        Platform& p; bool held = true;
        explicit Guard(Platform& pp) : p(pp) { p.lock(); }
        ~Guard() { if (held) p.unlock(); }
        void release() { if (held) { p.unlock(); held = false; } }
        void take() { if (!held) { p.lock(); held = true; } }
    };

    Platform& P;
    std::string name_;
    std::vector<Gen*> gens_;
    int current_ = 0;
    bool passThrough_ = false;
    std::string passWhy_;
    bool swapOnTick_ = false;
    unsigned long long lastCheck_ = 0, lastStamp_ = 0;
    bool checkedOnce_ = false;
    long long refusedBuild_ = 0;
    bool refusedFinal_ = false;
    std::string refusedWhy_, manifestWhy_;
    std::unordered_set<const void*> recs_;
    std::vector<Rec*> live_;
    int swaps_ = 0;
    bool statusDirty_ = true;
    std::string lastStatus_;
    std::string faultWhy_;

    int onStartCall(void* api, int reason)
    {
        Guard g(P);
        if (gens_.empty()) {
            if (reason != R_INIT) return RTX_FAIL_RC;
            std::string why;
            if (!loadFirstLocked(why)) { manifestWhy_ = why; statusDirty_ = true; flushStatusLocked(); return RTX_FAIL_RC; }
        }
        Gen* g0 = gens_[0];                                  // INIT / SETUP always go to the first build: it registers with IRT
        g.release();
        bool f = false;
        int rc = g0->m->call(api, f);
        g.take();
        if (f) { noteFault(0, reason); flushStatusLocked(); return RTX_FAIL_RC; }
        if (reason == R_INIT && !passThrough_ && readCheck(api) != SDK_CHECKVERSION) {
            passThrough_ = true;                             // not the SDK layout this shim was verified against: plain forwarding only
            passWhy_ = "SDK layout not the verified 15.1.6 one - hot reload off (plain forwarding)";
            statusDirty_ = true;
        }
        flushStatusLocked();
        return rc;
    }

    bool loadFirstLocked(std::string& why)
    {
        std::string text; unsigned long long stamp = 0;
        if (!P.readManifest(text, stamp)) { why = "no manifest - the build has not published " + name_ + " yet"; return false; }
        Manifest m;
        if (!parseManifest(text, name_, m, why)) return false;
        Module* mod = P.load(m.file, why);
        if (!mod) return false;
        HrInfo inf; std::memset(&inf, 0, sizeof inf); inf.size = sizeof(HrInfo);
        Verdict v = mod->info(inf) ? checkInfo(inf, nullptr, name_, m.build) : Verdict::NoInfo;
        if (v != Verdict::Accept) { why = std::string("build ") + std::to_string(m.build) + " " + verdictText(v); P.discard(mod); return false; }
        addGenLocked(mod, inf, m.build);
        lastStamp_ = stamp; checkedOnce_ = true; lastCheck_ = P.nowMs();
        swapOnTick_ = m.swapOnTick;
        return true;
    }

    void addGenLocked(Module* mod, const HrInfo& inf, long long build)
    {
        Gen* g = new Gen();
        g->m = mod; g->info = inf; g->build = build;
        size_t n = 0; while (n < sizeof inf.version && inf.version[n]) ++n;
        g->version.assign(inf.version, n < sizeof inf.version ? n : 0);
        gens_.push_back(g);
        current_ = (int)gens_.size() - 1;
        mod->attach(&hostTable());
        statusDirty_ = true;
    }

    void checkUpdatesLocked()
    {
        unsigned long long now = P.nowMs();
        if (checkedOnce_ && now - lastCheck_ < CHECK_EVERY_MS) return;
        checkedOnce_ = true; lastCheck_ = now;
        std::string text; unsigned long long stamp = 0;
        if (!P.readManifest(text, stamp)) return;            // missing for a moment: keep what runs
        if (stamp == lastStamp_) return;
        Manifest m; std::string why;
        if (!parseManifest(text, name_, m, why)) {
            if (manifestWhy_ != why) { manifestWhy_ = why; statusDirty_ = true; }
            return;                                          // unreadable: keep the last good build; re-read when it changes again
        }
        lastStamp_ = stamp;
        if (!manifestWhy_.empty()) { manifestWhy_.clear(); statusDirty_ = true; }
        swapOnTick_ = m.swapOnTick;
        long long newest = gens_.back()->build;
        if (m.build <= newest || (m.build == refusedBuild_ && refusedFinal_)) return;
        if ((int)gens_.size() >= MAX_BUILDS) { refuseLocked(m.build, "too many builds this session - restart Investor/RT once", true); return; }
        Module* mod = P.load(m.file, why);
        if (!mod) { refuseLocked(m.build, why, false); lastStamp_ = 0; return; }   // could not load (yet): retried on the next check
        HrInfo inf; std::memset(&inf, 0, sizeof inf); inf.size = sizeof(HrInfo);
        Verdict v = mod->info(inf) ? checkInfo(inf, &gens_[0]->info, name_, m.build) : Verdict::NoInfo;
        if (v != Verdict::Accept) { P.discard(mod); refuseLocked(m.build, verdictText(v), true); return; }
        refusedBuild_ = 0; refusedWhy_.clear(); refusedFinal_ = false;
        addGenLocked(mod, inf, m.build);
    }

    void refuseLocked(long long build, const std::string& why, bool isFinal)
    {
        if (refusedBuild_ != build || refusedWhy_ != why) statusDirty_ = true;
        refusedBuild_ = build; refusedWhy_ = why; refusedFinal_ = isFinal;
    }

    void noteFault(int genIdx, int reason)
    {
        Gen* g = gens_[genIdx];
        if (!g->bad) {
            g->bad = true;
            faultWhy_ = "build " + std::to_string(g->build) + " crashed (reason " + std::to_string(reason) + ")";
            // fall back to the newest build that has not crashed
            for (int i = (int)gens_.size() - 1; i >= 0; --i) if (!gens_[i]->bad) { current_ = i; break; }
            statusDirty_ = true;
        }
    }

    void dropRec(Rec* rec, void* api)
    {
        gens_[rec->gen]->charts--;
        recs_.erase(rec);
        for (size_t i = 0; i < live_.size(); ++i) if (live_[i] == rec) { live_.erase(live_.begin() + (long)i); break; }
        delete rec;
        writeUser(api, nullptr);
        statusDirty_ = true;
    }

    std::string shortStatusLocked()
    {
        if (gens_.empty()) return manifestWhy_.empty() ? "not loaded" : manifestWhy_;
        if (passThrough_) return "hot reload off";
        if (refusedBuild_) return "build " + std::to_string(refusedBuild_) + ": " + refusedWhy_;
        if (gens_[current_]->bad) return faultWhy_ + " - waiting for a fixed build";
        if (!faultWhy_.empty()) return faultWhy_ + " - back on build " + std::to_string(gens_[current_]->build);
        int older = 0;
        for (size_t i = 0; i < gens_.size(); ++i) if ((int)i != current_) older += gens_[i]->charts;
        if (older > 0) return "build " + std::to_string(gens_[current_]->build) + " ready - refresh the chart";
        return "";
    }

    std::string statusTextLocked()
    {
        std::string s = "HOT RELOAD " + name_ + "  shim " + SHIM_VERSION + "\n";
        if (gens_.empty()) { s += "not loaded: " + manifestWhy_ + "\n"; return s; }
        if (passThrough_) s += "mode: " + passWhy_ + "\n";
        else s += std::string("mode: hot reload on, swap on ") + (swapOnTick_ ? "refresh or next update" : "chart refresh") + "\n";
        s += "active: build " + std::to_string(gens_[current_]->build) + " (" + gens_[current_]->version + ")\n";
        s += "builds loaded this session:";
        for (Gen* x : gens_) {
            s += " " + std::to_string(x->build) + (x->bad ? "(crashed)" : "") + "[" + std::to_string(x->charts) + " chart(s)]";
        }
        s += "\n";
        s += "swaps: " + std::to_string(swaps_) + "\n";
        if (refusedBuild_) s += "refused: build " + std::to_string(refusedBuild_) + " - " + refusedWhy_ + "\n";
        if (!manifestWhy_.empty()) s += "manifest: " + manifestWhy_ + " (keeping the running build)\n";
        if (!faultWhy_.empty()) s += "fault: " + faultWhy_ + "\n";
        std::string line = shortStatusLocked();
        s += "chart line: " + (line.empty() ? std::string("(none)") : line) + "\n";
        return s;
    }

    void flushStatusLocked()
    {
        if (!statusDirty_) return;
        statusDirty_ = false;
        std::string s = statusTextLocked();
        if (s != lastStatus_) { lastStatus_ = s; P.writeStatus(s); }
    }

    // the HrHost table handed to every build; status() goes through the singleton set by the shim
public:
    static Router*& instance() { static Router* r = nullptr; return r; }
    static int hostStatus(char* buf, int cap)
    {
        if (!buf || cap <= 0) return 0;
        Router* r = instance();
        std::string s = r ? r->shortStatus() : std::string();
        int n = (int)s.size(); if (n > cap - 1) n = cap - 1;
        std::memcpy(buf, s.data(), (size_t)n); buf[n] = '\0';
        return n;
    }
    static const HrHost& hostTable() { static HrHost h = { (uint32_t)sizeof(HrHost), ABI, &Router::hostStatus }; return h; }
};

} // namespace hotreload
#endif
