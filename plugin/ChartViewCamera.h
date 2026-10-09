// ChartViewCamera.h - lsChartView 1.1.0 (2026-10-09): a picture of THIS IRT chart, on request only.
// (Rassul 2026-10-09 12:02 "can you ensure that the camera is for irt only" / 12:04 "keep it within irt" / 12:05 "if i say get a
// pic of gold you should be able to do it")
// Rules:
//   * IRT only: the only windows ever looked at belong to this process (investorRT.exe) - GetCurrentProcessId(). No screen or
//     desktop capture: the chart window is asked to paint itself into a private bitmap (PrintWindow), so a window from another
//     program on top of it is never in the picture.
//   * The window is found by its title (the chart's symbol + periodicity label, e.g. "GCEZ26" + "3 Minutes*").
//   * Minimised windows are skipped (Windows cannot paint them).
//   * Every Win32 / GDI+ call is checked; failures return an error text, never throw. The caller wraps this in try/catch.
// The PNG is written to <path>.tmp then renamed over <path>. No GDI+ (its headers do not build next to the IRT SDK): pixels
// come from GetDIBits and are encoded by cvl::pngEncode.
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
#include "ChartViewLogic.h"          // cvl::pngEncode (no GDI+: its headers clash with the SDK build)
#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

namespace cvcam {

struct Find { DWORD pid; std::string sym, per; HWND best; long long bestArea; int seen; };

static void consider(HWND h, Find* f)
{
    char t[512]; t[0] = 0;
    if (GetWindowTextA(h, t, (int)sizeof(t)) <= 0) return;
    std::string s(t);
    f->seen++;
    if (s.find(f->sym) == std::string::npos) return;
    if (!f->per.empty() && s.find(" " + f->per) == std::string::npos) return;
    if (!IsWindowVisible(h) || IsIconic(h)) return;
    RECT r; if (!GetWindowRect(h, &r)) return;
    long long a = (long long)(r.right - r.left) * (long long)(r.bottom - r.top);
    if (a > f->bestArea) { f->bestArea = a; f->best = h; }
}
static int CALLBACK childProc(HWND h, LPARAM lp) { consider(h, (Find*)lp); return TRUE; }
static int CALLBACK topProc(HWND h, LPARAM lp)
{
    Find* f = (Find*)lp;
    DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
    if (pid != f->pid) return TRUE;                       // IRT ONLY: any other program's window is skipped
    consider(h, f);
    EnumChildWindows(h, childProc, lp);                   // chart windows may be children of an IRT frame
    return TRUE;
}

// the chart window of this IRT process whose title holds the symbol and the periodicity label; NULL when not found
inline HWND findChart(const std::string& sym, const std::string& perLabel, int* windowsSeen)
{
    Find f; f.pid = GetCurrentProcessId(); f.sym = sym; f.per = perLabel; f.best = NULL; f.bestArea = 0; f.seen = 0;
    if (!sym.empty()) EnumWindows(topProc, (LPARAM)&f);
    if (windowsSeen) *windowsSeen = f.seen;
    return f.best;
}

// paint the window into a bitmap and save it as PNG; returns "" on success or the reason it failed
inline std::string capture(HWND h, const std::string& path, int* wOut, int* hOut)
{
    if (!h || !IsWindow(h)) return "no window";
    if (IsIconic(h)) return "window minimised";
    RECT r; if (!GetWindowRect(h, &r)) return "no window size";
    int w = r.right - r.left, ht = r.bottom - r.top;
    if (w <= 0 || ht <= 0 || w > 10000 || ht > 10000) return "bad window size";
    HDC screen = GetDC(NULL); if (!screen) return "no DC";
    HDC mem = CreateCompatibleDC(screen);
    HBITMAP bmp = mem ? CreateCompatibleBitmap(screen, w, ht) : NULL;
    ReleaseDC(NULL, screen);
    if (!mem || !bmp) { if (bmp) DeleteObject(bmp); if (mem) DeleteDC(mem); return "no bitmap"; }
    HGDIOBJ old = SelectObject(mem, bmp);
    int ok = PrintWindow(h, mem, 2 /* PW_RENDERFULLCONTENT */);
    if (!ok) ok = PrintWindow(h, mem, 0);
    SelectObject(mem, old);
    std::string err;
    if (!ok) err = "PrintWindow failed";
    else {
        BITMAPINFO bi; memset(&bi, 0, sizeof(bi));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -ht;   // top-down
        bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
        std::vector<unsigned char> px((size_t)w * (size_t)ht * 4);
        if (GetDIBits(mem, bmp, 0, (UINT)ht, px.data(), &bi, DIB_RGB_COLORS) != ht) err = "pixel read failed";
        else {
            std::vector<unsigned char> rgb((size_t)w * (size_t)ht * 3);
            for (size_t i = 0, n = (size_t)w * (size_t)ht; i < n; i++) { rgb[i * 3] = px[i * 4 + 2]; rgb[i * 3 + 1] = px[i * 4 + 1]; rgb[i * 3 + 2] = px[i * 4]; }
            const std::string png = cvl::pngEncode(rgb.data(), w, ht);
            const std::string tmp = path + ".tmp";
            FILE* f = png.empty() ? NULL : fopen(tmp.c_str(), "wb");
            if (!f) err = png.empty() ? "PNG encode failed" : "cannot write file";
            else {
                bool wr = fwrite(png.data(), 1, png.size(), f) == png.size();
                if (fclose(f) != 0) wr = false;
                if (!wr) { DeleteFileA(tmp.c_str()); err = "write failed"; }
                else if (!MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) { DeleteFileA(tmp.c_str()); err = "rename failed"; }
            }
        }
    }
    DeleteObject(bmp); DeleteDC(mem);
    if (wOut) *wOut = w;
    if (hOut) *hOut = ht;
    return err;
}

} // namespace cvcam
#endif
