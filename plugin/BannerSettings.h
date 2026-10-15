// BannerSettings.h  (LiquidityProfile 0.24.0, Rassul 2026-10-15 12:56 "use a solid fill background by default. make sure it has
// settings that can be changed and the settings work") - the verdict banner's settings, kept per market in
// lsFlexLevels\LiquidityProfile.banner-<MKT>.txt as "BAN1,on,bg,font,colour" (colour = 0x00BBGGRR as decimal). Garbage never reaches
// the file or the chart: a bad file keeps the last good values; a bad dialog value keeps the file's value.
#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
namespace bns {
static const int FONT_MIN = 7, FONT_MAX = 16, FONT_DEF = 11;
static const long long BG_DEF = 0x00141414;            // near-black, a shade off the chart so the box reads as a box
struct Settings { bool on = true, bg = true; int font = FONT_DEF; long long bgc = BG_DEF; bool ok = false; std::string why; };
inline bool num(const std::string& s, long long lo, long long hi, long long& out) {
    if (s.empty() || s.size() > 12) return false;
    for (size_t i = 0; i < s.size(); ++i) if (s[i] < '0' || s[i] > '9') return false;
    const long long v = std::atoll(s.c_str()); if (v < lo || v > hi) return false; out = v; return true;
}
inline bool parse(const std::string& line, Settings& out) {
    std::string ln = line; while (!ln.empty() && (ln.back() == '\r' || ln.back() == '\n' || ln.back() == ' ')) ln.pop_back();
    std::vector<std::string> f; size_t a = 0;
    for (size_t i = 0; i <= ln.size(); ++i) if (i == ln.size() || ln[i] == ',') { f.push_back(ln.substr(a, i - a)); a = i + 1; }
    long long on = 0, bg = 0, ft = 0, c = 0;
    if (f.size() != 5 || f[0] != "BAN1" || !num(f[1], 0, 1, on) || !num(f[2], 0, 1, bg) || !num(f[3], FONT_MIN, FONT_MAX, ft) || !num(f[4], 0, 0x00FFFFFF, c)) {
        out.ok = false; out.why = "banner file garbled: '" + ln.substr(0, 40) + "' - last good kept"; return false; }
    Settings z; z.on = on != 0; z.bg = bg != 0; z.font = (int)ft; z.bgc = c; z.ok = true; out = z; return true;
}
inline std::string line(const Settings& s) {
    char b[64]; std::snprintf(b, sizeof(b), "BAN1,%d,%d,%d,%lld", s.on ? 1 : 0, s.bg ? 1 : 0, s.font, s.bgc & 0x00FFFFFFLL); return b;
}
// the dialog's values -> clean settings; out of range = the file's (or default) value
inline Settings clean(long long on, long long bg, long long font, long long colour, const Settings& fileVal) {
    Settings s = fileVal; s.ok = true; s.why.clear();
    if (on == 0 || on == 1) s.on = on != 0;
    if (bg == 0 || bg == 1) s.bg = bg != 0;
    if (font >= FONT_MIN && font <= FONT_MAX) s.font = (int)font;
    if (colour >= 0) s.bgc = colour & 0x00FFFFFFLL;
    return s;
}
}  // namespace bns
