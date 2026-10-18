/********************************************************************************
 *  HotShim.cpp  --  the stable RTX DLL Investor/RT loads once; forwards every call to the newest hot-reload build   1.0.0 (2026-10-18)
 *
 *  Rassul 2026-10-18 15:17 CT: "is there a way for you to do the build without me having to restart every time, maybe i can just
 *  refresh the chart instead?"
 *
 *  One generic source for every indicator: it takes its name from its own file name (lsHotReloadDemo.dll -> "lsHotReloadDemo") and
 *  reads  <InvestorRT>\hotreload\<name>.manifest  to find the implementation DLL (<name>_impl_<build>.dll, a normal RTX build that
 *  the build script copied under a NEW name, so the file IRT has loaded is never the one being replaced).
 *  It does NOT include irtsdk.h or link irtsdk.lib: the SDK's whole contract with IRT is one export, cpp_main(void *pAPI).
 *  Status: <InvestorRT>\hotreload\<name>.status.txt (rewritten only when something changes).
 *  All decisions are in HotReloadLogic.h (tested on Linux); this file is the Win32 plumbing.
 *
 *  Build (gex-hot-build.bat):  cl /nologo /LD /EHsc /O2 /MD HotShim.cpp /Fe:lsHotReloadDemo.dll     (kernel32 only, no SDK lib)
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "HotReloadLogic.h"

#if defined(_MSC_VER)
#pragma comment(lib, "kernel32.lib")
#endif

namespace {

typedef int (*CppMainFn)(void*);
typedef int (*InfoFn)(hotreload::HrInfo*);
typedef void (*AttachFn)(const hotreload::HrHost*);

HMODULE g_self = NULL;
CRITICAL_SECTION g_cs;
bool g_csReady = false;
std::wstring g_hotDir;          // ...\InvestorRT\hotreload\  (with the trailing backslash)
std::wstring g_nameW;           // lsHotReloadDemo
std::string g_name;
bool g_pathsOk = false;
hotreload::Router* g_router = nullptr;

// ---- crash guards: no C++ objects with destructors in these functions (MSVC rule for __try) ----
int sehCall(CppMainFn fn, void* api, bool* faulted)
{
#if defined(_MSC_VER)
    __try { return fn(api); }
    __except (EXCEPTION_EXECUTE_HANDLER) { *faulted = true; return -1; }
#else
    (void)faulted; return fn(api);
#endif
}
int sehInfo(InfoFn fn, hotreload::HrInfo* out, bool* faulted)
{
#if defined(_MSC_VER)
    __try { return fn(out); }
    __except (EXCEPTION_EXECUTE_HANDLER) { *faulted = true; return 0; }
#else
    (void)faulted; return fn(out);
#endif
}
void sehAttach(AttachFn fn, const hotreload::HrHost* h, bool* faulted)
{
#if defined(_MSC_VER)
    __try { fn(h); }
    __except (EXCEPTION_EXECUTE_HANDLER) { *faulted = true; }
#else
    (void)faulted; fn(h);
#endif
}

struct WinModule : hotreload::Module {
    HMODULE h = NULL; CppMainFn fn = nullptr; InfoFn inf = nullptr; AttachFn att = nullptr;
    int call(void* api, bool& faulted) override { bool f = false; int rc = sehCall(fn, api, &f); faulted = f; return rc; }
    bool info(hotreload::HrInfo& out) override
    {
        if (!inf) return false;
        bool f = false; int ok = sehInfo(inf, &out, &f);
        return ok == 1 && !f;
    }
    void attach(const hotreload::HrHost* host) override { if (att) { bool f = false; sehAttach(att, host, &f); } }
};

bool asciiOnly(const std::wstring& w, std::string& out)
{
    out.clear();
    for (wchar_t c : w) { if (c <= 0 || c > 126) return false; out.push_back((char)c); }
    return true;
}

bool initPaths()
{
    wchar_t buf[MAX_PATH + 1];
    DWORD n = GetModuleFileNameW(g_self, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return false;
    std::wstring full(buf, n);
    size_t slash = full.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return false;
    std::wstring dir = full.substr(0, slash);                    // ...\InvestorRT\dllx64
    std::wstring file = full.substr(slash + 1);                  // lsHotReloadDemo.dll
    size_t dot = file.find_last_of(L'.');
    g_nameW = dot == std::wstring::npos ? file : file.substr(0, dot);
    size_t up = dir.find_last_of(L"\\/");
    if (up == std::wstring::npos) return false;
    g_hotDir = dir.substr(0, up) + L"\\hotreload\\";             // ...\InvestorRT\hotreload\  (never inside dllx64: IRT loads every DLL there)
    return asciiOnly(g_nameW, g_name) && !g_name.empty();
}

struct WinPlatform : hotreload::Platform {
    bool stampOf(const std::wstring& path, unsigned long long& stamp, unsigned long long& size)
    {
        WIN32_FILE_ATTRIBUTE_DATA a;
        if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &a)) return false;
        unsigned long long t = ((unsigned long long)a.ftLastWriteTime.dwHighDateTime << 32) | a.ftLastWriteTime.dwLowDateTime;
        size = ((unsigned long long)a.nFileSizeHigh << 32) | a.nFileSizeLow;
        stamp = t * 1000003ULL + size + 1;
        return true;
    }
    bool readManifest(std::string& text, unsigned long long& stamp) override
    {
        std::wstring path = g_hotDir + g_nameW + L".manifest";
        unsigned long long s1 = 0, z1 = 0, s2 = 0, z2 = 0;
        if (!stampOf(path, s1, z1) || z1 == 0 || z1 > 4096) return false;
        HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL);
        if (f == INVALID_HANDLE_VALUE) return false;
        char buf[4097]; DWORD got = 0;
        BOOL ok = ReadFile(f, buf, 4096, &got, NULL);
        CloseHandle(f);
        if (!ok || got != (DWORD)z1) return false;
        if (!stampOf(path, s2, z2) || s2 != s1) return false;   // changed while reading: try again on the next check
        text.assign(buf, got);
        stamp = s1;
        return true;
    }
    hotreload::Module* load(const std::string& file, std::string& why) override
    {
        if (!hotreload::safeFileName(file)) { why = "bad file name"; return nullptr; }
        std::wstring path = g_hotDir + std::wstring(file.begin(), file.end());
        if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) { why = file + " not found in hotreload folder"; return nullptr; }
        HMODULE h = LoadLibraryExW(path.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!h) { why = file + " would not load (error " + std::to_string((unsigned long)GetLastError()) + ")"; return nullptr; }
        WinModule* m = new WinModule();
        m->h = h;
        m->fn = reinterpret_cast<CppMainFn>(reinterpret_cast<void*>(GetProcAddress(h, "cpp_main")));
        m->inf = reinterpret_cast<InfoFn>(reinterpret_cast<void*>(GetProcAddress(h, "hr_info")));
        m->att = reinterpret_cast<AttachFn>(reinterpret_cast<void*>(GetProcAddress(h, "hr_attach")));
        if (!m->fn) { why = file + " has no cpp_main - not an RTX build"; FreeLibrary(h); delete m; return nullptr; }
        return m;
    }
    void discard(hotreload::Module* mod) override
    {
        WinModule* m = static_cast<WinModule*>(mod);
        if (m && m->h) FreeLibrary(m->h);   // refused before IRT ever called into it: safe to unload
        delete m;
    }
    unsigned long long nowMs() override { return (unsigned long long)GetTickCount64(); }
    void writeStatus(const std::string& text) override
    {
        CreateDirectoryW(g_hotDir.c_str(), NULL);
        std::wstring path = g_hotDir + g_nameW + L".status.txt", tmp = path + L".tmp";
        HANDLE f = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (f == INVALID_HANDLE_VALUE) return;
        std::string crlf;
        for (char c : text) { if (c == '\n') crlf += "\r\n"; else crlf.push_back(c); }
        DWORD put = 0;
        BOOL ok = WriteFile(f, crlf.data(), (DWORD)crlf.size(), &put, NULL);
        FlushFileBuffers(f);
        CloseHandle(f);
        if (ok && put == (DWORD)crlf.size()) MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
        else DeleteFileW(tmp.c_str());
    }
    void lock() override { EnterCriticalSection(&g_cs); }
    void unlock() override { LeaveCriticalSection(&g_cs); }
};

WinPlatform g_platform;

} // namespace

extern "C" __declspec(dllexport) int cpp_main(void* pAPI)
{
    if (!g_csReady) return -1;
    EnterCriticalSection(&g_cs);
    if (!g_router && g_pathsOk) {
        g_router = new hotreload::Router(g_platform, g_name);
        hotreload::Router::instance() = g_router;
    }
    hotreload::Router* r = g_router;
    LeaveCriticalSection(&g_cs);
    if (!r) return -1;
    return r->onCall(pAPI);
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = hinst;
        DisableThreadLibraryCalls(hinst);
        InitializeCriticalSection(&g_cs);
        g_csReady = true;
        g_pathsOk = initPaths();
    }
    // DLL_PROCESS_DETACH: nothing - Investor/RT is exiting; the builds stay loaded until the process ends
    return TRUE;
}
