// test_hotreload_logic.cpp  (2026-10-18)  -- the hot-reload shim's decisions, with fake builds and a fake host struct.
// Build: g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -I. test_hotreload_logic.cpp -o t && ./t
#include "HotReloadLogic.h"
#include "HotReloadDemoLogic.h"
#include <cstdio>
#include <map>
#include <memory>

using namespace hotreload;

static int g_fail = 0, g_pass = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

// ---------------- fake host struct (IRTAPI) ----------------
struct Api {
    alignas(8) char b[0x100];
    Api() { std::memset(b, 0, sizeof b); }
    void* p() { return b; }
    void setReason(int r) { writeReason(b, r); }
    void* ud() { return readUser(b); }
};

// ---------------- fake build ----------------
struct FakeModule : Module {
    std::string tag;
    HrInfo inf{};
    bool hasInfo = true;
    bool setsCheck = true;           // REASON_INIT writes checkversion like the real SDK
    int faultOnReason = -1;          // simulate a crash
    struct Call { int reason; void* ud; };
    std::vector<Call> calls;
    std::vector<std::unique_ptr<int>> states;     // per-chart state objects it made
    hotreload::Router* reenter = nullptr;         // make a nested call from inside calc
    int reenterReason = R_DRAW;
    const HrHost* host = nullptr;
    std::string statusSeen;
    bool askStatus = false;
    int discarded = 0;

    int call(void* api, bool& faulted) override
    {
        int reason = readReason(api);
        void* ud = readUser(api);
        calls.push_back({reason, ud});
        if (reason == faultOnReason) { faulted = true; return -1; }
        if (reason == R_INIT && setsCheck) { int v = SDK_CHECKVERSION; std::memcpy(static_cast<char*>(api) + OFF_CHECKVERSION, &v, 4); }
        if ((reason == R_CALC || reason == R_UPDT) && !ud) {                  // like HostSlot: make state on first calc
            states.emplace_back(new int(0));
            writeUser(api, states.back().get());
        }
        if (reason == R_CALC || reason == R_UPDT) { int* s = static_cast<int*>(readUser(api)); if (s) ++*s; }
        if (reason == R_DESTR) writeUser(api, nullptr);
        if (askStatus && host) { char buf[200]; host->status(buf, sizeof buf); statusSeen = buf; }
        if (reenter && reason == R_CALC) {
            hotreload::Router* r = reenter; reenter = nullptr;
            int saved = readReason(api);
            writeReason(api, reenterReason);
            r->onCall(api);
            writeReason(api, saved);
        }
        return 0;
    }
    bool info(HrInfo& out) override { if (!hasInfo) return false; out = inf; return true; }
    void attach(const HrHost* h) override { host = h; }
    int count(int reason) const { int n = 0; for (auto& c : calls) if (c.reason == reason) ++n; return n; }
    int lastReason() const { return calls.empty() ? -99 : calls.back().reason; }
    void* lastUd() const { return calls.empty() ? nullptr : calls.back().ud; }
};

static HrInfo makeInfo(const char* name, long long build, const char* ver = "1.0.0", const char* params = "v1|output:X", int arrays = 1,
                       uint32_t flags = 7, uint32_t fext = 0)
{
    HrInfo i; std::memset(&i, 0, sizeof i);
    i.size = sizeof(HrInfo); i.abi = ABI; i.build = build;
    std::strncpy(i.name, name, sizeof i.name - 1); std::strncpy(i.version, ver, sizeof i.version - 1);
    i.paramSig = fnv1a(params); i.arrayCount = arrays; i.flags = flags; i.flagsExt = fext;
    return i;
}

// ---------------- fake platform ----------------
struct FakePlatform : Platform {
    std::string manifest; bool haveManifest = false; unsigned long long stamp = 1;
    std::map<std::string, FakeModule*> files;       // what "exists" in the hotreload folder
    std::map<std::string, int> failLoads;           // file -> times to fail first
    unsigned long long now = 1000;
    std::vector<std::string> statuses;
    int lockDepth = 0, maxLockDepth = 0;
    int discards = 0;
    std::vector<std::unique_ptr<FakeModule>> owned;

    void publish(const std::string& name, long long build, const std::string& swap = "")
    {
        manifest = "HOTRELOAD 1\r\nNAME " + name + "\r\nBUILD " + std::to_string(build) + "\r\nFILE " + name + "_impl_" + std::to_string(build) + ".dll\r\n";
        if (!swap.empty()) manifest += "SWAP " + swap + "\r\n";
        haveManifest = true; ++stamp;
    }
    FakeModule* add(const std::string& name, long long build, HrInfo inf)
    {
        owned.emplace_back(new FakeModule());
        FakeModule* m = owned.back().get();
        m->tag = std::to_string(build); m->inf = inf;
        files[name + "_impl_" + std::to_string(build) + ".dll"] = m;
        return m;
    }
    bool readManifest(std::string& text, unsigned long long& st) override { if (!haveManifest) return false; text = manifest; st = stamp; return true; }
    Module* load(const std::string& file, std::string& why) override
    {
        auto f = failLoads.find(file);
        if (f != failLoads.end() && f->second > 0) { --f->second; why = "locked"; return nullptr; }
        auto it = files.find(file);
        if (it == files.end()) { why = file + " not found"; return nullptr; }
        return it->second;
    }
    void discard(Module* m) override { ++discards; static_cast<FakeModule*>(m)->discarded++; }
    unsigned long long nowMs() override { return now; }
    void writeStatus(const std::string& t) override { statuses.push_back(t); }
    void lock() override { ++lockDepth; if (lockDepth > maxLockDepth) maxLockDepth = lockDepth; }
    void unlock() override { --lockDepth; }
};

static const std::string NAME = "lsHotReloadDemo";
static int R_PREDRAW_FAKE() { return 7; }

static void testManifest()
{
    Manifest m; std::string why;
    CHECK(parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 3\nFILE lsHotReloadDemo_impl_3.dll\n", NAME, m, why));
    CHECK(m.build == 3 && m.file == "lsHotReloadDemo_impl_3.dll" && !m.swapOnTick);
    CHECK(parseManifest("# comment\r\nHOTRELOAD 1\r\n  NAME lsHotReloadDemo\r\nBUILD 12\r\nFILE lsHotReloadDemo_impl_12.dll\r\nSWAP tick\r\n", NAME, m, why));
    CHECK(m.build == 12 && m.swapOnTick);
    CHECK(parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 4\nFILE lsHotReloadDemo_impl_4.dll", NAME, m, why));   // no final newline
    CHECK(!parseManifest("", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 2\nNAME lsHotReloadDemo\nBUILD 3\nFILE lsHotReloadDemo_impl_3.dll\n", NAME, m, why));
    CHECK(!parseManifest("NAME lsHotReloadDemo\nBUILD 3\nFILE lsHotReloadDemo_impl_3.dll\n", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 3x\nFILE lsHotReloadDemo_impl_3.dll\n", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD -3\nFILE lsHotReloadDemo_impl_3.dll\n", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 0\nFILE lsHotReloadDemo_impl_0.dll\n", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 99999999999999999999\nFILE lsHotReloadDemo_impl_3.dll\n", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 3\nBUILD 4\nFILE lsHotReloadDemo_impl_3.dll\n", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsOther\nBUILD 3\nFILE lsOther_impl_3.dll\n", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 3\nFILE ..\\lsHotReloadDemo_impl_3.dll\n", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 3\nFILE C:\\x\\lsHotReloadDemo_impl_3.dll\n", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 3\nFILE lsDealerProfile.dll\n", NAME, m, why));       // not this name's impl
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 3\nFILE lsHotReloadDemo_impl_3.exe\n", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 3\n", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 3\nFILE lsHotReloadDemo_impl_3.dll\nCOLOR red\n", NAME, m, why));
    CHECK(!parseManifest("HOTRELOAD 1\nNAME lsHotReloadDemo\nBUILD 3\nFILE lsHotReloadDemo_impl_3.dll\nSWAP always\n", NAME, m, why));
    CHECK(!parseManifest(std::string(5000, 'x'), NAME, m, why));
    CHECK(safeFileName("a_b-1.dll") && !safeFileName("a b.dll") && !safeFileName("a/b.dll") && !safeFileName(".dll"));
}

static void testVerdicts()
{
    HrInfo base = makeInfo("lsHotReloadDemo", 1);
    HrInfo c = makeInfo("lsHotReloadDemo", 2);
    CHECK(checkInfo(c, &base, NAME, 2) == Verdict::Accept);
    CHECK(checkInfo(base, nullptr, NAME, 1) == Verdict::Accept);
    HrInfo x = c; x.abi = 2;            CHECK(checkInfo(x, &base, NAME, 2) == Verdict::BadAbi);
    x = c; x.size = 8;                  CHECK(checkInfo(x, &base, NAME, 2) == Verdict::BadAbi);
    x = c; std::strcpy(x.name, "lsX"); CHECK(checkInfo(x, &base, NAME, 2) == Verdict::WrongName);
    x = c; std::memset(x.name, 'A', sizeof x.name);  CHECK(checkInfo(x, &base, NAME, 2) == Verdict::WrongName);   // unterminated
    CHECK(checkInfo(c, &base, NAME, 3) == Verdict::WrongBuild);
    x = makeInfo("lsHotReloadDemo", 2, "1.0.1", "v2|output:X|bool:Show"); CHECK(checkInfo(x, &base, NAME, 2) == Verdict::ParamsChanged);
    x = makeInfo("lsHotReloadDemo", 2, "1.0.1", "v1|output:X", 2);        CHECK(checkInfo(x, &base, NAME, 2) == Verdict::ArraysChanged);
    x = makeInfo("lsHotReloadDemo", 2, "1.0.1", "v1|output:X", 1, 9);     CHECK(checkInfo(x, &base, NAME, 2) == Verdict::FlagsChanged);
    x = makeInfo("lsHotReloadDemo", 2, "1.0.1", "v1|output:X", 1, 7, 1);  CHECK(checkInfo(x, &base, NAME, 2) == Verdict::FlagsChanged);
    CHECK(std::string(verdictText(Verdict::ParamsChanged)).find("restart") != std::string::npos);
}

struct Rig {
    FakePlatform P;
    std::unique_ptr<Router> R;
    Rig() { R.reset(new Router(P, NAME)); Router::instance() = R.get(); }
    ~Rig() { Router::instance() = nullptr; }
    int send(Api& a, int reason) { a.setReason(reason); return R->onCall(a.p()); }
};

static void startUp(Rig& g, FakeModule*& m1)
{
    m1 = g.P.add(NAME, 1, makeInfo(NAME.c_str(), 1));
    g.P.publish(NAME, 1);
    Api boot;
    CHECK(g.send(boot, R_INIT) == 0);
    CHECK(g.send(boot, R_SETUP) == 0);
    CHECK(m1->count(R_INIT) == 1 && m1->count(R_SETUP) == 1);
}

static void testStartup()
{
    {   // no manifest: INIT fails cleanly, says why
        Rig g; Api a;
        CHECK(g.send(a, R_INIT) == RTX_FAIL_RC);
        CHECK(g.R->buildCount() == 0);
        CHECK(!g.P.statuses.empty() && g.P.statuses.back().find("no manifest") != std::string::npos);
        CHECK(g.send(a, R_CALC) == RTX_FAIL_RC);
        CHECK(g.send(a, R_SETUP) == RTX_FAIL_RC);
    }
    {   // first build without hr_info / with a wrong build number: refused at start
        Rig g; FakeModule* m = g.P.add(NAME, 1, makeInfo(NAME.c_str(), 1)); m->hasInfo = false; g.P.publish(NAME, 1);
        Api a; CHECK(g.send(a, R_INIT) == RTX_FAIL_RC); CHECK(g.P.discards == 1);
        Rig h; h.P.add(NAME, 1, makeInfo(NAME.c_str(), 5)); h.P.publish(NAME, 1);
        Api b; CHECK(h.send(b, R_INIT) == RTX_FAIL_RC);
    }
    {   // normal start, SDK layout verified
        Rig g; FakeModule* m1; startUp(g, m1);
        CHECK(!g.R->passThrough());
        CHECK(g.R->activeBuild() == 1);
        CHECK(m1->host != nullptr);
    }
    {   // INIT did not write the 15.1.6 checkversion: plain forwarding, user data untouched
        Rig g; FakeModule* m1 = g.P.add(NAME, 1, makeInfo(NAME.c_str(), 1)); m1->setsCheck = false; g.P.publish(NAME, 1);
        Api boot; CHECK(g.send(boot, R_INIT) == 0);
        CHECK(g.R->passThrough());
        Api a; g.send(a, R_CALC);
        CHECK(a.ud() == m1->states.back().get());            // the build's own pointer, no record
        CHECK(g.R->liveRecords() == 0);
        g.P.add(NAME, 2, makeInfo(NAME.c_str(), 2)); g.P.publish(NAME, 2); g.P.now += 5000;
        g.send(a, R_CALC);
        CHECK(g.R->buildCount() == 1);                       // never swaps in pass-through
    }
}

static void testChartsAndSwap()
{
    Rig g; FakeModule* m1; startUp(g, m1);
    Api A, B;
    CHECK(g.send(A, R_CALC) == 0);
    CHECK(g.send(B, R_CALC) == 0);
    CHECK(g.R->liveRecords() == 2);
    CHECK(A.ud() != nullptr && A.ud() != B.ud());
    CHECK(m1->states.size() == 2);
    CHECK(A.ud() != m1->states[0].get());                    // the host slot holds the shim's record, not the build's state
    g.send(A, R_DRAW);
    CHECK(m1->lastUd() == m1->states[0].get());              // ...and the build sees its own state
    g.send(B, R_DRAW);
    CHECK(m1->lastUd() == m1->states[1].get());
    CHECK(*m1->states[0] == 1);
    g.send(A, R_UPDT);
    CHECK(*m1->states[0] == 2);
    CHECK(g.P.maxLockDepth == 1);                            // never re-locked; no lock held while a build runs

    // publish build 2; not looked at until a second has passed
    FakeModule* m2 = g.P.add(NAME, 2, makeInfo(NAME.c_str(), 2, "1.0.1"));
    g.P.publish(NAME, 2);
    g.P.now += 200;
    g.send(A, R_CALC);
    CHECK(g.R->buildCount() == 1);
    g.P.now += 1000;
    int destrBefore = m1->count(R_DESTR), doneBefore = m1->count(R_DONE);
    g.send(A, R_CALC);
    CHECK(g.R->buildCount() == 2 && g.R->activeBuild() == 2);
    CHECK(m1->count(R_DONE) == doneBefore + 1 && m1->count(R_DESTR) == destrBefore + 1);   // A closed on build 1, as if removed
    CHECK(m1->calls[m1->calls.size() - 1].reason == R_DESTR && m1->calls[m1->calls.size() - 1].ud == m1->states[0].get());
    CHECK(m1->calls[m1->calls.size() - 2].reason == R_DONE && m1->calls[m1->calls.size() - 2].ud == m1->states[0].get());
    CHECK(m2->count(R_CALC) == 1 && m2->calls[0].ud == nullptr);   // A starts fresh on build 2
    CHECK(*m2->states[0] == 1);
    CHECK(readReason(A.p()) == R_CALC);                      // host struct's reason restored
    CHECK(g.R->chartsOn(1) == 1 && g.R->chartsOn(2) == 1);
    CHECK(g.R->shortStatus().find("refresh the chart") != std::string::npos);
    // B keeps running on build 1 until its own refresh; UPDT does not move it (SWAP refresh)
    g.send(B, R_UPDT);
    CHECK(m1->lastReason() == R_UPDT && m1->lastUd() == m1->states[1].get());
    g.send(B, R_DRAW);
    CHECK(m1->lastReason() == R_DRAW);
    g.send(B, R_CALC);
    CHECK(m2->lastReason() == R_CALC && m2->states.size() == 2);
    CHECK(g.R->chartsOn(1) == 0 && g.R->chartsOn(2) == 2);
    CHECK(g.R->shortStatus().empty());
    // an OLDER build in the manifest is ignored
    g.P.publish(NAME, 1); g.P.now += 2000; g.send(A, R_CALC);
    CHECK(g.R->activeBuild() == 2 && g.R->buildCount() == 2);
    // chart removed
    g.send(A, R_DONE); g.send(A, R_DESTR);
    CHECK(A.ud() == nullptr);
    CHECK(g.R->liveRecords() == 1 && g.R->chartsOn(2) == 1);
    CHECK(m2->lastReason() == R_DESTR);
    // status file: rewritten on changes only
    size_t n = g.P.statuses.size();
    g.send(B, R_DRAW); g.send(B, R_DRAW);
    CHECK(g.P.statuses.size() == n);
    CHECK(g.P.statuses.back().find("active: build 2 (1.0.1)") != std::string::npos);
}

static void testTickMode()
{
    Rig g; FakeModule* m1; startUp(g, m1);
    Api A; g.send(A, R_CALC);
    FakeModule* m2 = g.P.add(NAME, 2, makeInfo(NAME.c_str(), 2));
    g.P.publish(NAME, 2, "tick");
    g.P.now += 1500;
    g.send(A, R_UPDT);                                       // the manifest is read on this update; SWAP tick moves the chart now
    CHECK(g.R->activeBuild() == 2);
    CHECK(m2->count(R_CALC) == 1 && m2->count(R_UPDT) == 0); // promoted: the first call on the new build is a full calc
    CHECK(readReason(A.p()) == R_UPDT);                      // and the host struct says UPDT again afterwards
    g.send(A, R_UPDT);
    CHECK(m2->lastReason() == R_UPDT);
}

static void testReadyWithoutRefresh()
{
    Rig g; FakeModule* m1; startUp(g, m1);
    Api A; g.send(A, R_CALC);
    FakeModule* m2 = g.P.add(NAME, 2, makeInfo(NAME.c_str(), 2));
    g.P.publish(NAME, 2); g.P.now += 1500;
    g.send(A, R_UPDT);                                       // a live tick: build 2 is loaded and announced, the chart is not moved
    CHECK(g.R->buildCount() == 2 && m2->calls.empty());
    CHECK(m1->lastReason() == R_UPDT);
    CHECK(g.R->shortStatus() == "build 2 ready - refresh the chart");
    g.send(A, R_DRAW);
    CHECK(m1->lastReason() == R_DRAW);                       // draws stay on build 1 until the refresh
    g.send(A, R_CALC);
    CHECK(m2->count(R_CALC) == 1 && g.R->shortStatus().empty());
}

static void testRefuse()
{
    Rig g; FakeModule* m1; startUp(g, m1);
    Api A; g.send(A, R_CALC);
    FakeModule* bad = g.P.add(NAME, 2, makeInfo(NAME.c_str(), 2, "1.1.0", "v2|output:X|bool:Show line"));
    g.P.publish(NAME, 2); g.P.now += 1500;
    g.send(A, R_CALC);
    CHECK(g.R->activeBuild() == 1 && g.R->buildCount() == 1);
    CHECK(bad->discarded == 1 && bad->calls.empty());        // never called into
    CHECK(g.R->refusedBuild() == 2);
    CHECK(g.R->shortStatus().find("settings list changed") != std::string::npos);
    CHECK(m1->lastReason() == R_CALC);                       // the chart keeps running the old build
    g.P.now += 1500; ++g.P.stamp; g.send(A, R_CALC);       // same build again: not retried
    CHECK(bad->discarded == 1);
    FakeModule* m3 = g.P.add(NAME, 3, makeInfo(NAME.c_str(), 3, "1.0.2"));
    g.P.publish(NAME, 3); g.P.now += 1500; g.send(A, R_CALC);
    CHECK(g.R->activeBuild() == 3 && m3->count(R_CALC) == 1);
    CHECK(g.R->refusedBuild() == 0 && g.R->shortStatus().empty());
    // arrays / flags changes refused too
    g.P.add(NAME, 4, makeInfo(NAME.c_str(), 4, "1.0.3", "v1|output:X", 2)); g.P.publish(NAME, 4); g.P.now += 1500; g.send(A, R_CALC);
    CHECK(g.R->refusedBuild() == 4 && g.R->activeBuild() == 3);
}

static void testTransientLoad()
{
    Rig g; FakeModule* m1; startUp(g, m1);
    Api A; g.send(A, R_CALC);
    FakeModule* m2 = g.P.add(NAME, 2, makeInfo(NAME.c_str(), 2));
    g.P.failLoads[NAME + "_impl_2.dll"] = 1;                 // e.g. the virus scanner still has it open
    g.P.publish(NAME, 2); g.P.now += 1500; g.send(A, R_CALC);
    CHECK(g.R->activeBuild() == 1 && g.R->refusedBuild() == 2);
    g.P.now += 1500; g.send(A, R_CALC);                      // retried without a new manifest
    CHECK(g.R->activeBuild() == 2 && m2->count(R_CALC) == 1);
}

static void testGarbledManifest()
{
    Rig g; FakeModule* m1; startUp(g, m1);
    Api A; g.send(A, R_CALC);
    g.P.manifest = "HOTRELOAD 1\nNAME lsHotRel"; ++g.P.stamp; g.P.now += 1500;
    g.send(A, R_CALC);
    CHECK(g.R->activeBuild() == 1 && m1->lastReason() == R_CALC);
    CHECK(g.R->statusText().find("manifest:") != std::string::npos);
    g.P.add(NAME, 2, makeInfo(NAME.c_str(), 2)); g.P.publish(NAME, 2); g.P.now += 1500; g.send(A, R_CALC);
    CHECK(g.R->activeBuild() == 2);
    CHECK(g.R->statusText().find("manifest:") == std::string::npos);
}

static void testFault()
{
    Rig g; FakeModule* m1; startUp(g, m1);
    Api A, B; g.send(A, R_CALC); g.send(B, R_CALC);
    FakeModule* m2 = g.P.add(NAME, 2, makeInfo(NAME.c_str(), 2));
    m2->faultOnReason = R_CALC;
    g.P.publish(NAME, 2); g.P.now += 1500;
    CHECK(g.send(A, R_CALC) == RTX_FAIL_RC);                 // build 2 crashed on its first calc
    CHECK(g.R->activeBuild() == 1);                          // fell back to build 1
    CHECK(g.R->shortStatus().find("crashed") != std::string::npos);
    CHECK(g.send(A, R_DRAW) == RTX_FAIL_RC);                 // A is blank until its next full calc...
    int m2calls = (int)m2->calls.size();
    CHECK(g.send(A, R_CALC) == 0);                           // ...which starts it fresh on build 1
    CHECK(m1->lastReason() == R_CALC && m1->lastUd() == nullptr);
    CHECK((int)m2->calls.size() == m2calls);                 // the crashed build is never called again (no DESTR into it)
    g.send(B, R_CALC);                                       // B never left build 1
    CHECK(m1->lastUd() == m1->states[1].get());
    FakeModule* m3 = g.P.add(NAME, 3, makeInfo(NAME.c_str(), 3));
    g.P.publish(NAME, 3); g.P.now += 1500; g.send(A, R_CALC);
    CHECK(g.R->activeBuild() == 3 && m3->count(R_CALC) == 1);
    // removing a chart that sits dead on a crashed build frees its record
    Rig h; FakeModule* n1; startUp(h, n1);
    Api C; h.send(C, R_CALC);
    FakeModule* n2 = h.P.add(NAME, 2, makeInfo(NAME.c_str(), 2)); n2->faultOnReason = R_CALC;
    h.P.publish(NAME, 2); h.P.now += 1500; h.send(C, R_CALC);
    h.send(C, R_DONE); h.send(C, R_DESTR);
    CHECK(h.R->liveRecords() == 0 && C.ud() == nullptr);
}

static void testFirstBuildFault()
{
    Rig g; FakeModule* m1; startUp(g, m1);
    m1->faultOnReason = R_DRAW;
    Api A; g.send(A, R_CALC);
    CHECK(g.send(A, R_DRAW) == RTX_FAIL_RC);
    CHECK(g.send(A, R_CALC) == RTX_FAIL_RC);                 // nothing good to move to: stays blank
    CHECK(g.R->shortStatus().find("waiting for a fixed build") != std::string::npos);
    FakeModule* m2 = g.P.add(NAME, 2, makeInfo(NAME.c_str(), 2));
    g.P.publish(NAME, 2); g.P.now += 1500;
    CHECK(g.send(A, R_CALC) == 0);                           // a fixed build heals it without a restart
    CHECK(m2->count(R_CALC) == 1);
}

static void testNested()
{
    Rig g; FakeModule* m1; startUp(g, m1);
    Api A; g.send(A, R_CALC);
    m1->reenter = g.R.get();                                 // during calc the build triggers a nested call for the same chart
    g.send(A, R_CALC);
    CHECK(m1->calls.back().reason == R_DRAW);
    CHECK(m1->calls.back().ud == m1->states[0].get());       // routed to the same build with its own pointer
    CHECK(g.R->liveRecords() == 1);                          // no stray record
    CHECK(A.ud() != m1->states[0].get() && A.ud() != nullptr);
    // nested on a brand-new chart whose build has not set state yet (user data still null inside)
    Api N; m1->reenter = g.R.get(); m1->reenterReason = R_PREDRAW_FAKE();
    g.send(N, R_CALC);
    CHECK(g.R->liveRecords() == 2);
}

static void testStatusFromBuild()
{
    Rig g; FakeModule* m1; startUp(g, m1);
    Api A; g.send(A, R_CALC);
    g.P.add(NAME, 2, makeInfo(NAME.c_str(), 2, "1.1.0", "v2|output:X|bool:Show"));
    g.P.publish(NAME, 2); g.P.now += 1500; g.send(A, R_CALC);
    m1->askStatus = true;
    g.send(A, R_DRAW);                                       // the build asks the shim from inside draw (no deadlock)
    CHECK(m1->statusSeen.find("build 2") != std::string::npos);
    char tiny[8]; int n = Router::hostStatus(tiny, sizeof tiny);
    CHECK(n == 7 && tiny[7] == '\0');
    CHECK(Router::hostStatus(nullptr, 5) == 0);
}

static void testCap()
{
    Rig g; FakeModule* m1; startUp(g, m1);
    Api A; g.send(A, R_CALC);
    for (int b = 2; b <= Router::MAX_BUILDS + 2; ++b) {
        g.P.add(NAME, b, makeInfo(NAME.c_str(), b)); g.P.publish(NAME, b); g.P.now += 1500; g.send(A, R_CALC);
    }
    CHECK(g.R->buildCount() == Router::MAX_BUILDS);
    CHECK(g.R->shortStatus().find("too many builds") != std::string::npos);
}

static void testForeignPointer()
{
    Rig g; FakeModule* m1; startUp(g, m1);
    Api A; int junk = 5; writeUser(A.p(), &junk);
    g.send(A, R_DRAW);                                       // a pointer the shim never made: passed through untouched
    CHECK(m1->lastUd() == &junk && A.ud() == &junk);
    CHECK(g.R->liveRecords() == 0);
}

static void testDemoText()
{
    CHECK(hrdemo::title("1.0.1", 3, 12) == "Hot Reload Demo v1.0.1  build 3  calcs 12");
    CHECK(hrdemo::statusLine("") == "hot reload OK");
    std::string longs(200, 'x');
    CHECK((int)hrdemo::statusLine(longs).size() == hrdemo::MAX_STATUS);
    CHECK(hrdemo::statusLine("build 4 ready - refresh the chart") == "build 4 ready - refresh the chart");
}

int main()
{
    testManifest();
    testVerdicts();
    testStartup();
    testChartsAndSwap();
    testTickMode();
    testReadyWithoutRefresh();
    testRefuse();
    testTransientLoad();
    testGarbledManifest();
    testFault();
    testFirstBuildFault();
    testNested();
    testStatusFromBuild();
    testCap();
    testForeignPointer();
    testDemoText();
    std::printf("hot reload logic: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
