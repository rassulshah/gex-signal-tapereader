// WyMarks.h 1.0.0 (2026-11-18) -- the Reversal Read I on the price candles (lsRevReadMarks 0.18.0). SDK-free, tested (tests_wymarks).
// Rassul 2026-11-18 17:06 CT "I also want the I indicator on the candle, put it on top of green candle and below red candles"; 17:11 "go"
// on the mockup (i-on-candles.html: a small tag, green = bullish I, red = bearish I, white letter).
// Source: lsFlexLevels\RevRead\wy-<MKT>-<secs>.txt, written by the lsRevRead pane once per closed candle (WyRead.h letters: header
// WY|1|MKT|secs|version, then stamp|letter|dir) - so the candle and the pane always agree, and a decided I never changes (no repaint).
#pragma once
#include <cstdlib>
#include <map>
#include <string>

namespace wyi {
static const char* const VERSION = "1.0.0";
static const int TAG_H = 13, PAD_X = 3, GAP = 3, CX_STEP = 16;
static const size_t MAX_BYTES = 1024 * 1024;

// the I letters of the file: candle stamp -> +1 bullish / -1 bearish. -1 = not a letters file (torn / foreign: keep the last good set).
inline int parse(const std::string& txt, std::map<long long, int>& out) {
    out.clear(); size_t a = 0; int n = 0; bool head = false;
    while (a < txt.size()) {
        size_t e = txt.find('\n', a); if (e == std::string::npos) e = txt.size();
        std::string l = txt.substr(a, e - a); a = e + 1;
        if (!l.empty() && l[l.size() - 1] == '\r') l.erase(l.size() - 1);
        if (l.empty()) continue;
        if (!head) { if (l.compare(0, 5, "WY|1|") != 0) return -1; head = true; continue; }
        const size_t p1 = l.find('|'); if (p1 == std::string::npos) continue;
        const size_t p2 = l.find('|', p1 + 1); if (p2 == std::string::npos) continue;
        char* end = nullptr; const long long t = std::strtoll(l.c_str(), &end, 10); if (!end || *end != '|' || t <= 0) continue;
        if (l.substr(p1 + 1, p2 - p1 - 1) != "I") continue;
        char* e2 = nullptr; const long d = std::strtol(l.c_str() + p2 + 1, &e2, 10); if (!e2 || *e2 != 0 || (d != 1 && d != -1)) continue;
        out[t] = (int)d; ++n;
    }
    return head ? n : -1;
}

// where the tag goes: OVER a green candle (close >= open), UNDER a red one (his rule). cxSameSide = a Cx already sits on that side of
// this candle (lsRevReadMarks draws Cx 16 px beyond the extreme): the I steps one row further out so the two never overlap.
struct Tag { int l = 0, t = 0, r = 0, b = 0; bool ok = false; bool over = false; };
inline Tag place(int x, int yHigh, int yLow, bool greenCandle, int textW, bool cxSameSide, int paneTop, int paneBottom) {
    Tag g; g.over = greenCandle; const int w = (textW > 0 ? textW : 4) + 2 * PAD_X; const int step = cxSameSide ? CX_STEP : 0;
    g.l = x - w / 2; g.r = g.l + w;
    if (greenCandle) { g.b = yHigh - GAP - step; g.t = g.b - TAG_H; } else { g.t = yLow + GAP + step; g.b = g.t + TAG_H; }
    g.ok = g.t >= paneTop + 1 && g.b <= paneBottom - 1;
    return g;
}
} // namespace wyi
