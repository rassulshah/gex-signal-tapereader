// ChartViewCamera.h - lsChartView 1.2.0 (2026-10-10): a picture of THIS IRT chart, on request only.
// (Rassul 2026-10-09 12:02 "can you ensure that the camera is for irt only" / 12:04 "keep it within irt" / 12:05 "if i say get a
// pic of gold you should be able to do it")
// Rules:
//   * IRT only: the only windows ever looked at belong to this process (investorRT.exe) - GetCurrentProcessId(). No screen or
//     desktop capture: the chart window is asked to paint itself into a private bitmap (PrintWindow), so a window from another
//     program on top of it is never in the picture (no BitBlt from the screen, ever).
//   * (1.2.0) WHICH window: every visible IRT window is listed and cvl::pickChart chooses the one whose title shows the symbol
//     AND this chart's bar size ("3 Minutes" / "(3m*)"). A window that shows only the symbol is never taken. 1.1.0 matched the
//     symbol alone whenever IRT returned an empty periodicity label (most calls), so ES_180 and ES_3600 got the same picture.
//   * (1.2.0) WHEN: only from ChartView's own timer callback (outside IRT's paint of this chart). 1.1.0 could run inside the
//     chart's draw/calc, and PrintWindow then caught the price pane mid-paint: a black price pane (ES_180 2026-10-09 20:47).
//   * (1.2.0) Blank check + retry: cvl::blankCheck on the price-pane area; a blank picture is retried on a later tick after an
//     InvalidateRect (PW_RENDERFULLCONTENT twice, then classic PrintWindow 0). The last good picture is never replaced by a
//     blank one.
//   * Minimised windows are skipped (Windows cannot paint them).
//   * Every Win32 call is checked; failures return an error text, never throw. The caller wraps this in try/catch.
// No GDI+ (its headers do not build next to the IRT SDK): pixels come from GetDIBits and are encoded by cvl::pngEncode.
// DPI: GetWindowRect and PrintWindow both run in IRT's own DPI context (this DLL is inside investorRT.exe), so the bitmap and
// the window size always agree; the size used is written to the status file.
#pragma once
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>
#include <cstring>
#include <vector>
#include <cstdio>
#include "ChartViewLogic.h"          // cvl::pickChart / blankCheck / pngEncode
#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

namespace cvcam {

struct Enum { DWORD pid; std::vector<cvl::WinCand>* cands; std::vector<HWND>* hwnds; };

static void consider(HWND h, Enum* f)
{
    if (f->cands->size() >= 400) return;                 // bounded
    char t[512]; t[0] = 0;
    if (GetWindowTextA(h, t, (int)sizeof(t)) <= 0) return;
    RECT r; if (!GetWindowRect(h, &r)) return;
    cvl::WinCand c; c.title = t; c.w = (int)(r.right - r.left); c.h = (int)(r.bottom - r.top);
    c.area = (long long)c.w * (long long)c.h; c.visible = IsWindowVisible(h) != 0; c.iconic = IsIconic(h) != 0;
    f->cands->push_back(c); f->hwnds->push_back(h);
}
static int CALLBACK childProc(HWND h, LPARAM lp) { consider(h, (Enum*)lp); return 1; }
static int CALLBACK topProc(HWND h, LPARAM lp)
{
    Enum* f = (Enum*)lp;
    DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
    if (pid != f->pid) return 1;                          // IRT ONLY: any other program's window is skipped
    consider(h, f);
    EnumChildWindows(h, childProc, lp);                   // chart windows may be children of an IRT frame
    return 1;
}

// every titled window of this IRT process (top-level and children)
inline void listWindows(std::vector<cvl::WinCand>& cands, std::vector<HWND>& hwnds)
{
    cands.clear(); hwnds.clear();
    Enum e; e.pid = GetCurrentProcessId(); e.cands = &cands; e.hwnds = &hwnds;
    EnumWindows(topProc, (LPARAM)&e);
}

// ask the window to paint itself into a private bitmap; rgb = w * h * 3 top-down. "" on success or the reason it failed.
inline std::string grab(HWND h, unsigned pwFlags, std::vector<unsigned char>& rgb, int* wOut, int* hOut)
{
    rgb.clear();
    if (!h || !IsWindow(h)) return "no window";
    if (IsIconic(h)) return "window minimised";
    RECT r; if (!GetWindowRect(h, &r)) return "no window size";
    int w = r.right - r.left, ht = r.bottom - r.top;
    if (w <= 0 || ht <= 0 || w > 10000 || ht > 10000) return "bad window size";
    if (wOut) *wOut = w;
    if (hOut) *hOut = ht;
    HDC screen = GetDC(NULL); if (!screen) return "no DC";
    HDC mem = CreateCompatibleDC(screen);
    HBITMAP bmp = mem ? CreateCompatibleBitmap(screen, w, ht) : NULL;
    ReleaseDC(NULL, screen);
    if (!mem || !bmp) { if (bmp) DeleteObject(bmp); if (mem) DeleteDC(mem); return "no bitmap"; }
    HGDIOBJ old = SelectObject(mem, bmp);
    RECT all = { 0, 0, w, ht };
    FillRect(mem, &all, (HBRUSH)GetStockObject(BLACK_BRUSH));   // a window that paints nothing reads as blank, not garbage
    int ok = PrintWindow(h, mem, pwFlags);
    if (!ok && pwFlags != 0) ok = PrintWindow(h, mem, 0);
    SelectObject(mem, old);                               // GetDIBits needs the bitmap out of the DC
    std::string err;
    if (!ok) err = "PrintWindow failed";
    else {
        BITMAPINFO bi; memset(&bi, 0, sizeof(bi));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -ht;   // top-down
        bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
        std::vector<unsigned char> px((size_t)w * (size_t)ht * 4);
        if (GetDIBits(mem, bmp, 0, (UINT)ht, px.data(), &bi, DIB_RGB_COLORS) != ht) err = "pixel read failed";
        else {
            rgb.resize((size_t)w * (size_t)ht * 3);
            for (size_t i = 0, n = (size_t)w * (size_t)ht; i < n; i++) { rgb[i * 3] = px[i * 4 + 2]; rgb[i * 3 + 1] = px[i * 4 + 1]; rgb[i * 3 + 2] = px[i * 4]; }
        }
    }
    DeleteObject(bmp); DeleteDC(mem);
    return err;
}

// a repaint is queued (never forced here): the retry on a later tick sees a freshly painted chart
inline void askRepaint(HWND h) { if (h && IsWindow(h)) InvalidateRect(h, NULL, FALSE); }

// write bytes to <path> safely: <path>.tmp, the old file to <path>.bak, rename into place, put .bak back on failure
inline std::string publish(const std::string& path, const std::string& bytes)
{
    if (bytes.empty()) return "nothing to write";
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return "cannot write file";
    bool wr = fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
    if (fflush(f) != 0) wr = false;
    if (fclose(f) != 0) wr = false;
    if (!wr) { DeleteFileA(tmp.c_str()); return "write failed"; }
    bool hadOld = GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
    if (hadOld && !MoveFileExA(path.c_str(), bak.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) { DeleteFileA(tmp.c_str()); return "cannot move the old file aside"; }
    if (!MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        if (hadOld) MoveFileExA(bak.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);   // the good copy back
        DeleteFileA(tmp.c_str());
        return "rename failed";
    }
    return std::string();
}

} // namespace cvcam
#endif
