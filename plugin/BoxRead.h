// BoxRead.h  1.0.0  (2026-10-18, Rassul approved 23:48 CT aggr-read-v2-mockup.png / bookflow-read-mockup.png) - the "Read" row of the
// Aggression (lsBidAskStats 1.15.0) and Book Flow (lsBookFlow 1.4.0) info boxes: one word (BULLISH green / BEARISH red / NEUTRAL light
// grey) + the read in up to 2 short lines, wrapped at word boundaries and NEVER cut: when the full read needs more lines than the box has,
// its optional clauses are dropped from the last (least important) one first; a single word wider than the box keeps its own line.
// SDK-free (tests_aggbook/test_aggbook_115.cpp); the plugins measure with their own font (a width functor).
#pragma once
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace br {

static const char* const VERSION = "1.0.0";
enum { BEAR = -1, NEUTRAL = 0, BULL = 1 };
static const unsigned long C_NEUTRAL = 0x00D1D5DB, C_BULL = 0x0022C55E, C_BEAR = 0x00EF4444;   // light grey / the panes' buyer green / seller red
static const int MAX_LINES = 2, TEXT_PT = 9;

struct Read {
    int impl = NEUTRAL;
    std::string head;                  // the read itself (always shown)
    std::string tag;                   // (Defense 1.14.0) a word in place of BULLISH / BEARISH (WATCH / CAUTION); empty = the usual word
    std::vector<std::string> opt;      // optional clauses in text order, each with its own separator; dropped from the last when too long
    std::string word() const { if (!tag.empty()) return tag; return impl == BULL ? "BULLISH" : impl == BEAR ? "BEARISH" : "NEUTRAL"; }
    unsigned long color() const { return impl == BULL ? C_BULL : impl == BEAR ? C_BEAR : C_NEUTRAL; }
    std::string text(size_t nOpt) const { std::string t = head; for (size_t k = 0; k < nOpt && k < opt.size(); ++k) t += opt[k]; return t; }
    std::string full() const { return text(opt.size()); }
};

typedef std::function<int(const std::string&)> Width;

// greedy word wrap at spaces; a word wider than the width stays whole on its own line (never cut)
inline std::vector<std::string> wrap(const std::string& t, int width, const Width& W) {
    std::vector<std::string> out; std::string line; size_t i = 0;
    while (i < t.size()) {
        while (i < t.size() && t[i] == ' ') ++i;
        if (i >= t.size()) break;
        size_t j = t.find(' ', i); if (j == std::string::npos) j = t.size();
        const std::string w = t.substr(i, j - i); i = j;
        const std::string cand = line.empty() ? w : line + " " + w;
        if (line.empty() || W(cand) <= width) line = cand;
        else { out.push_back(line); line = w; }
    }
    if (!line.empty()) out.push_back(line);
    return out;
}
// the lines shown: the fullest read that fits maxLines; if even the head alone needs more, the head's lines (the caller gives it room)
inline std::vector<std::string> fit(const Read& r, int width, int maxLines, const Width& W) {
    if (maxLines < 1) maxLines = 1;
    for (size_t n = r.opt.size() + 1; n-- > 0;) { std::vector<std::string> L = wrap(r.text(n), width, W); if ((int)L.size() <= maxLines) return L; }
    return wrap(r.head, width, W);
}

inline std::string x1(double v) { if (!(v >= 0) || v > 9999) v = v > 9999 ? 9999 : 0; char b[24]; std::snprintf(b, sizeof(b), "%.1f", v); return b; }
// "~3" for 3.0, "~1.6" for 1.6 (the both-sides averages)
inline std::string approx(double v) { std::string s = x1(v); if (s.size() > 2 && s.compare(s.size() - 2, 2, ".0") == 0) s.erase(s.size() - 2); return s; }

} // namespace br
