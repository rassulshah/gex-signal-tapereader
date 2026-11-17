// SplitText.h  --  the split read (lsRevRead 0.13.0, lsZzoCombo 3.2.0, lsVolRead / lsRangeRead 1.5.0). SDK-free.
// (1.0.0, 2026-11-17 Rassul 08:35-08:44 CT "the idea is to have a split with the left having a verdict and human readable explanation and the right
// side to have the data points. cna you do this for all these sections", "i like this approach . overtime we will refine this further to gain better
// more descriptive and predictive insight", "build"; mockup split_reads.html)
// LEFT  = the verdict line, then plain-English sentences (most important first), word-wrapped to the left column.
// RIGHT = the numbers, as before.
// A sentence is shown whole or not at all (only the first one may be cut, ending in "..."), so a low pane drops the least important sentences.
#pragma once
#include <algorithm>
#include <string>
#include <vector>

namespace spt {

static const char* const VERSION = "1.0.0";
static const int LEFT_MAX = 340;   // the left column is never wider than this (px)
static const int LEFT_MIN = 170;   // narrower than this: the numbers are left out and the left column takes the room
static const int GAP = 26;         // between the two columns (a thin divider sits in the middle)

// split a sentence into words (single spaces)
inline std::vector<std::string> words(const std::string& s) {
    std::vector<std::string> w; std::string cur;
    for (char ch : s) { if (ch == ' ') { if (!cur.empty()) { w.push_back(cur); cur.clear(); } } else cur += ch; }
    if (!cur.empty()) w.push_back(cur);
    return w;
}
// word-wrap ONE sentence into lines no wider than width (a single word wider than width gets its own line)
template <class TW> std::vector<std::string> wrapOne(const std::string& s, int width, TW tw) {
    std::vector<std::string> L; std::string line;
    for (const std::string& w : words(s)) {
        const std::string t = line.empty() ? w : line + " " + w;
        if (!line.empty() && tw(t) > width) { L.push_back(line); line = w; } else line = t;
    }
    if (!line.empty()) L.push_back(line);
    return L;
}
// the explanation lines: whole sentences while they fit in maxLines; the first sentence alone may be cut short with "..."
template <class TW> std::vector<std::string> wrap(const std::vector<std::string>& S, int width, int maxLines, TW tw) {
    std::vector<std::string> out; if (maxLines <= 0 || width <= 0) return out;
    for (size_t i = 0; i < S.size(); ++i) {
        if (S[i].empty()) continue;
        std::vector<std::string> L = wrapOne(S[i], width, tw);
        if ((int)(out.size() + L.size()) <= maxLines) { out.insert(out.end(), L.begin(), L.end()); continue; }
        if (out.empty()) {   // the most important sentence does not fit: show what fits, the last line ending in "..."
            for (int k = 0; k < maxLines && k < (int)L.size(); ++k) out.push_back(L[(size_t)k]);
            std::string& last = out.back();
            while (!last.empty() && tw(last + "...") > width) { const size_t sp = last.find_last_of(' '); if (sp == std::string::npos) { last.clear(); break; } last.erase(sp); }
            last += "...";
        }
        break;   // a later sentence that does not fit is left out, and so are the ones after it (most important first)
    }
    return out;
}
// the two column widths for an area `avail` px wide with a numbers block `dataW` wide and a verdict line `headW` wide.
// leftW = 0 means nothing fits; dataOn = false means the numbers are left out.
struct Cols { int leftW = 0; bool dataOn = false; };
inline Cols cols(int avail, int dataW, int headW) {
    Cols c; if (avail <= 0) return c;
    const int withData = avail - dataW - GAP, need = std::max(LEFT_MIN, headW);   // the verdict line is never wrapped
    if (dataW > 0 && withData >= need) { c.leftW = std::max(std::min(LEFT_MAX, withData), headW); c.dataOn = true; return c; }
    if (avail >= headW && avail >= 60) c.leftW = std::min(avail, std::max(LEFT_MAX, headW));   // no room for both: the read alone
    return c;
}

} // namespace spt
