// test_hotreload_e2e.cpp (2026-10-18) -- end to end on Linux: real files, real dlopen of versioned copies, the manifest published
// with tmp + rename exactly like gex-hot-build.bat, the router from HotReloadLogic.h. (HotShim.cpp is the same flow with Win32 calls.)
// Build: see run_hotreload_tests.sh
#include "HotReloadLogic.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <mutex>
#include <chrono>
using namespace hotreload;
static int g_fail = 0, g_pass = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

typedef int (*MainFn)(void*); typedef int (*InfoFn)(HrInfo*); typedef void (*AttFn)(const HrHost*); typedef int (*LiveFn)();
struct SoModule : Module {
    void* h = nullptr; MainFn fn = nullptr; InfoFn inf = nullptr; AttFn att = nullptr;
    int call(void* api, bool& f) override { f = false; return fn(api); }
    bool info(HrInfo& o) override { return inf && inf(&o) == 1; }
    void attach(const HrHost* x) override { if (att) att(x); }
};
struct PosixPlatform : Platform {
    std::string dir, name; std::recursive_mutex mu; unsigned long long fakeNow = 10000; std::string status;
    std::vector<SoModule*> mods;
    bool readManifest(std::string& text, unsigned long long& stamp) override
    {
        std::string p = dir + "/" + name + ".manifest"; struct stat st;
        if (stat(p.c_str(), &st) != 0 || st.st_size <= 0 || st.st_size > 4096) return false;
        std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); text = ss.str();
        stamp = (unsigned long long)st.st_mtim.tv_sec * 1000000000ULL + (unsigned long long)st.st_mtim.tv_nsec + (unsigned long long)st.st_ino * 7 + (unsigned long long)st.st_size;
        return (long long)text.size() == (long long)st.st_size;
    }
    Module* load(const std::string& file, std::string& why) override
    {
        if (!safeFileName(file)) { why = "bad name"; return nullptr; }
        std::string so = dir + "/" + file.substr(0, file.size() - 4) + ".so";      // the test builds .so files under the .dll names
        void* h = dlopen(so.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!h) { why = dlerror(); return nullptr; }
        SoModule* m = new SoModule(); m->h = h;
        m->fn = (MainFn)dlsym(h, "cpp_main"); m->inf = (InfoFn)dlsym(h, "hr_info"); m->att = (AttFn)dlsym(h, "hr_attach");
        if (!m->fn) { dlclose(h); delete m; why = "no cpp_main"; return nullptr; }
        mods.push_back(m); return m;
    }
    void discard(Module* m) override { SoModule* s = static_cast<SoModule*>(m); dlclose(s->h); for (auto& x : mods) if (x == s) x = nullptr; delete s; }
    unsigned long long nowMs() override { return fakeNow; }
    void writeStatus(const std::string& t) override { status = t; }
    void lock() override { mu.lock(); }
    void unlock() override { mu.unlock(); }
};
static void publish(const std::string& dir, long long build, const std::string& swap = "refresh")
{
    std::string m = "HOTRELOAD 1\r\nNAME lsHotReloadDemo\r\nBUILD " + std::to_string(build) + "\r\nFILE lsHotReloadDemo_impl_" + std::to_string(build) + ".dll\r\nSWAP " + swap + "\r\n";
    std::string tmp = dir + "/lsHotReloadDemo.manifest.tmp";
    { std::ofstream f(tmp, std::ios::binary); f << m; }
    std::rename(tmp.c_str(), (dir + "/lsHotReloadDemo.manifest").c_str());
}
struct Api { alignas(8) char b[0x100]; Api() { std::memset(b, 0, sizeof b); } long long ran() { long long v; std::memcpy(&v, b + 0xC0, 8); return v; } };
static int send(Router& r, Api& a, int reason) { writeReason(a.b, reason); return r.onCall(a.b); }

int main(int argc, char** argv)
{
    if (argc < 2) { std::printf("usage: e2e <dir with lsHotReloadDemo_impl_1.so, _2.so, _3.so (settings changed)>\n"); return 2; }
    PosixPlatform P; P.dir = argv[1]; P.name = "lsHotReloadDemo";
    Router R(P, P.name); Router::instance() = &R;
    unlink((P.dir + "/lsHotReloadDemo.manifest").c_str());
    Api boot; CHECK(send(R, boot, R_INIT) == RTX_FAIL_RC);                // nothing published yet
    publish(P.dir, 1);
    CHECK(send(R, boot, R_INIT) == 0 && send(R, boot, R_SETUP) == 0);
    CHECK(!R.passThrough());
    Api A, B;
    send(R, A, R_CALC); send(R, B, R_CALC);
    CHECK(A.ran() == 1 && B.ran() == 1);
    LiveFn live1 = (LiveFn)dlsym(P.mods[0]->h, "e2e_live");
    CHECK(live1 && live1() == 2);
    usleep(20000);
    publish(P.dir, 2); P.fakeNow += 1500;
    send(R, A, R_UPDT);                                                     // a tick: build 2 loads, A stays on build 1
    CHECK(A.ran() == 1 && R.activeBuild() == 2);
    CHECK(R.shortStatus() == "build 2 ready - refresh the chart");
    send(R, A, R_CALC);                                                     // refresh
    CHECK(A.ran() == 2);
    CHECK(live1() == 1);                                                    // A's state was destroyed on build 1
    LiveFn live2 = (LiveFn)dlsym(P.mods[1]->h, "e2e_live");
    CHECK(live2 && live2() == 1);
    send(R, B, R_CALC);
    CHECK(B.ran() == 2 && live1() == 0 && live2() == 2);
    CHECK(R.shortStatus().empty());
    usleep(20000);
    publish(P.dir, 3); P.fakeNow += 1500;                                   // build 3 changed the settings list
    send(R, A, R_CALC);
    CHECK(A.ran() == 2 && R.activeBuild() == 2 && R.refusedBuild() == 3);
    CHECK(P.status.find("settings list changed") != std::string::npos);
    send(R, A, R_DONE); send(R, A, R_DESTR); send(R, B, R_DONE); send(R, B, R_DESTR);
    CHECK(live2() == 0 && R.liveRecords() == 0);
    std::printf("%s", P.status.c_str());
    std::printf("hot reload e2e: %d passed, %d failed\n", g_pass, g_fail);
    Router::instance() = nullptr;
    return g_fail ? 1 : 0;
}
