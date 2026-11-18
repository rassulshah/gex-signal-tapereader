// PlainRead.h  --  the PLAIN reads of the panes (lsRevRead 0.16.0, lsZzoCombo 3.3.0, lsVolRead / lsRangeRead 1.6.0). SDK-free.
// (1.0.0, 2026-11-17 Rassul 21:15 CT "i wnat a more human readable and simplified format for these reads and data points. you can keep the datapoints
// silently in the background, but its not something that is usefulle for me because i need something practical"; 21:17 "i like it. lets do this";
// mockup pane-reads-simple-mockup.html)
// Every pane read becomes THREE short lines and no table of numbers:
//   1  who is winning      "Buyers leaning in ?"           (green / red / white for mixed, grey for quiet; "?" until the nightly proves it)
//   2  why, in plain words "Bids stacking under price, offers thinning."
//   3  what to watch       "Watch: bids hold on the next dip -> long idea."
// The numbers are still computed (the status file, the nightly scorers lra.pane_verdicts / lra.orderflow_reads) - only the drawing drops them.
// Each pane's toPlain() reads its own Out (the verdict, the reasons) - the read rules themselves do not change.
#pragma once
#include <string>
#include <vector>

namespace plr {

static const char* const VERSION = "1.0.0";

// "A small green candle (0.7x its normal range), closed near its high (93%)." -> "A small green candle, closed near its high."
inline std::string noParens(const std::string& s) {
    std::string o; int d = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '(') { ++d; while (!o.empty() && o.back() == ' ') o.pop_back(); continue; }
        if (c == ')') { if (d > 0) --d; continue; }
        if (d == 0) o += c;
    }
    std::string r; for (size_t i = 0; i < o.size(); ++i) { if (o[i] == ' ' && (r.empty() || r.back() == ' ')) continue; if (o[i] == ',' && !r.empty() && r.back() == ' ') r.pop_back(); r += o[i]; }
    while (!r.empty() && r.back() == ' ') r.pop_back();
    return r;
}
// the reasons of a verdict line "big buyers lifting, but hidden sellers" -> {"big buyers lifting", "hidden sellers"} (the "but" is dropped)
inline std::vector<std::string> reasons(const std::string& why) {
    std::vector<std::string> R; size_t a = 0;
    while (a <= why.size()) {
        size_t b = why.find(", ", a); if (b == std::string::npos) b = why.size();
        std::string t = why.substr(a, b - a); if (t.compare(0, 4, "but ") == 0) t = t.substr(4);
        while (!t.empty() && (t[0] == '-' || t[0] == ' ')) t.erase(0, 1);
        if (!t.empty()) R.push_back(t);
        a = b + 2;
    }
    return R;
}
inline std::string cap(std::string s) { if (!s.empty() && s[0] >= 'a' && s[0] <= 'z') s[0] = (char)(s[0] - 'a' + 'A'); return s; }
inline std::string lowFirst(std::string s) { if (s.size() > 1 && s[0] >= 'A' && s[0] <= 'Z' && !(s[1] >= 'A' && s[1] <= 'Z')) s[0] = (char)(s[0] - 'A' + 'a'); return s; }
inline std::string noDot(std::string s) { while (!s.empty() && (s.back() == '.' || s.back() == ' ')) s.pop_back(); return s; }
// two plain sentences into one: "A big buyer swept up." + "Hidden sellers soaked it up." -> "A big buyer swept up, but hidden sellers soaked it up."
inline std::string butJoin(const std::string& a, const std::string& b) { return b.empty() ? a : a.empty() ? b : noDot(a) + ", but " + lowFirst(b); }

// the three lines a pane draws: head (verdict colour), then why and watch (white)
struct Plain { std::string head, why, watch; };
inline std::vector<std::string> lines(const Plain& p) { std::vector<std::string> L; if (!p.why.empty()) L.push_back(p.why); if (!p.watch.empty()) L.push_back(p.watch); return L; }

} // namespace plr
