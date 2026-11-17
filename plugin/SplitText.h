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

static const char* const VERSION = "1.1.0";   // (1.1.0, his 11:18 CT screenshot: GC Cob Reader lost its numbers - a long verdict line forced the left column wide) the verdict line wraps too; the numbers keep their room
static const int LEFT_MAX = 340;   // the left column is never wider than this (px)
static const int LEFT_MIN = 150;   // narrower than this: the numbers are left out and the left column takes the room
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
inline Cols cols(int avail, int dataW, int headW = 0) {
    (void)headW;   // (1.1.0) the verdict line wraps like the sentences - it no longer sets the column width
    Cols c; if (avail <= 0) return c;
    const int withData = avail - dataW - GAP;
    if (dataW > 0 && withData >= LEFT_MIN) { c.leftW = std::min(LEFT_MAX, withData); c.dataOn = true; return c; }
    if (avail >= 60) c.leftW = std::min(avail, LEFT_MAX);   // no room for both: the read alone
    return c;
}
// (1.1.0) the verdict "Bearish ?: thrust shortening, but up volume building" word-wrapped to the column; line 0 starts with head (drawn in the
// verdict colour), the rest in white. At most maxLines; the last one ends in "..." when cut.
template <class TW> std::vector<std::string> headLines(const std::string& head, const std::string& why, int width, int maxLines, TW tw) {
    std::vector<std::string> L = wrapOne(head + (why.empty() ? std::string() : " " + why), width, tw);
    if ((int)L.size() > maxLines && maxLines > 0) { L.resize((size_t)maxLines); std::string& last = L.back();
        while (!last.empty() && tw(last + "...") > width) { const size_t sp = last.find_last_of(' '); if (sp == std::string::npos) break; last.erase(sp); }
        last += "..."; }
    return L;
}
} // namespace spt
