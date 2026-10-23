#ifndef LS_REVERSAL_EVIDENCE_H
#define LS_REVERSAL_EVIDENCE_H
// Additive reversal evidence transport. Legacy LT files/drawing remain independent.
// key/cause/published are raw IRT bar/clock keys, NOT fabricated sub-bar MBO times.
#include "LetterStrips.h"
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace rse {
enum Basis { UNKNOWN = 0, MBO = 1, HYBRID = 2, IMPORTED = 3 };
enum Quality { UNVERIFIED = 0, COMPLETE = 1, PARTIAL = 2, UNAVAILABLE = 3, PENDING = 4 };
enum Outcome { NA = 0, HELD = 1, REFILLED = 2, BROKE = 3, DRAINED = 4 };
struct Meta {
    bool typed = false;
    int basis = UNKNOWN, quality = UNVERIFIED, outcome = NA;
    long long cause = 0, parentCause = 0, published = 0, priceTicks = LLONG_MIN;
    long long lots = -1, auxLots = -1, pace100 = -1;
    double ratio = -1;
    std::string id, contract;
};
struct Event { long long key = 0; int pane = lst::ATK; lst::Lt l; Meta m; };
// A frame acknowledges processing of this bar, NOT resolution of future tests.
struct Frame { long long key = 0, published = 0; int pane = lst::ATK, quality = UNVERIFIED; std::string contract; };
inline const char* qualityName(int q) {
    return q == COMPLETE ? "complete" : q == PARTIAL ? "partial" : q == UNAVAILABLE ? "unavailable" : q == PENDING ? "pending" : "unverified";
}
inline bool supportive(int q) { return q == HELD || q == REFILLED; }
inline bool adverse(int q) { return q == BROKE || q == DRAINED; }
inline std::string token(const std::string& s) {
    std::string o;
    for (unsigned char c : s) if (std::isalnum(c) || c == '-' || c == '_' || c == '.') o += (char)c;
    return o.empty() ? std::string("UNKNOWN") : o.substr(0, 96);
}
inline std::string fileName(int pane, const std::string& market, const std::string& contract, int spb) {
    return "EV-" + std::string(pane == lst::ATK ? "ATK" : pane == lst::DEF ? "DEF" : "QUEUE") + "-" + token(market) + "-" + token(contract) + "-" + std::to_string(spb) + ".csv";
}
inline std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> f; size_t a = 0;
    for (;;) {
        const size_t b = s.find('|', a); f.push_back(s.substr(a, b == std::string::npos ? b : b - a));
        if (b == std::string::npos || f.size() > 24) break;
        a = b + 1;
    }
    return f;
}
inline bool integer(const std::string& s, long long& n) {
    if (s.empty() || s.find_first_not_of("-0123456789") != std::string::npos) return false;
    return lst::fullLL(s, n);
}
inline bool number(const std::string& s, double& n) {
    if (s.empty() || s.size() > 64) return false;
    errno = 0; char* e = nullptr; n = std::strtod(s.c_str(), &e);
    return errno == 0 && e && *e == '\0' && std::isfinite(n);
}
inline std::string trimLine(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return s;
}
inline std::string eventId(int pane, long long key, const lst::Lt& l, long long cause = 0) {
    return std::string(1, lst::paneChar(pane)) + "-" + std::to_string(key) + "-" + l.code + "-" + std::to_string(l.pivot) + "-" + std::to_string(l.book) + "-" + std::to_string(cause);
}
// LE|2 adds parentCause after cause; LE|1 is read compatibly with parentCause=0.
inline std::string line(const Event& e) {
    std::ostringstream o; o.precision(17);
    o << "LE|2|" << lst::paneChar(e.pane) << '|' << e.key << '|' << e.l.code << '|' << e.l.pivot << '|' << e.l.book
      << '|' << e.m.basis << '|' << e.m.quality << '|' << e.m.outcome << '|' << e.m.cause << '|' << e.m.parentCause << '|' << e.m.published
      << '|' << e.m.priceTicks << '|' << e.m.lots << '|' << e.m.auxLots << '|' << e.m.pace100 << '|' << e.m.ratio
      << '|' << token(e.m.id) << '|' << token(e.m.contract) << '|' << lst::clean(e.l.nums);
    return o.str();
}
inline bool parse(const std::string& raw, Event& out) {
    const std::vector<std::string> f = split(trimLine(raw));
    if (f.size() < 20 || f[0] != "LE" || f[2].size() != 1) return false;
    const int d = f[1] == "2" ? 1 : 0;
    if ((d == 1 && f.size() != 21) || (d == 0 && (f[1] != "1" || f.size() != 20))) return false;
    const int pane = f[2] == "A" ? lst::ATK : f[2] == "D" ? lst::DEF : f[2] == "Q" ? lst::QUE : -1;
    if (pane < 0 || !lst::validCode(pane, f[4])) return false;
    long long key, pivot, book, basis, quality, outcome, cause, parent = 0, published, price, lots, aux, pace; double ratio;
    if (!integer(f[3], key) || key <= 0 || !integer(f[5], pivot) || pivot < 0 || pivot > 1
        || !integer(f[6], book) || book < -1 || book > 1 || !integer(f[7], basis) || basis < UNKNOWN || basis > IMPORTED
        || !integer(f[8], quality) || quality < UNVERIFIED || quality > PENDING || !integer(f[9], outcome) || outcome < NA || outcome > DRAINED
        || !integer(f[10], cause) || cause < 0 || (d && (!integer(f[11], parent) || parent < 0 || parent > cause))
        || !integer(f[11+d], published) || published <= 0
        || !integer(f[12+d], price) || !integer(f[13+d], lots) || lots < -1 || !integer(f[14+d], aux) || aux < -1
        || !integer(f[15+d], pace) || pace < -1 || !number(f[16+d], ratio) || ratio < -1
        || f[17+d].empty() || f[18+d].empty() || token(f[17+d]) != f[17+d] || token(f[18+d]) != f[18+d]) return false;
    Event e; e.key = key; e.pane = pane; e.l = lst::make(pane, f[4], (int)pivot, (int)book, f[19+d]); e.m.typed = true;
    e.m.basis = (int)basis; e.m.quality = (int)quality; e.m.outcome = (int)outcome; e.m.cause = cause; e.m.parentCause = parent; e.m.published = published;
    e.m.priceTicks = price; e.m.lots = lots; e.m.auxLots = aux; e.m.pace100 = pace; e.m.ratio = ratio; e.m.id = f[17+d]; e.m.contract = f[18+d]; out = e; return true;
}
inline std::string line(const Frame& f) {
    return "RF|1|" + std::string(1, lst::paneChar(f.pane)) + "|" + std::to_string(f.key) + "|" + token(f.contract) + "|" + std::to_string(f.quality) + "|" + std::to_string(f.published);
}
inline bool parse(const std::string& raw, Frame& out) {
    const std::vector<std::string> f = split(trimLine(raw));
    if (f.size() != 7 || f[0] != "RF" || f[1] != "1") return false;
    const int pane = f[2] == "A" ? lst::ATK : f[2] == "D" ? lst::DEF : f[2] == "Q" ? lst::QUE : -1;
    long long key, quality, published;
    if (pane < 0 || !integer(f[3], key) || key <= 0 || f[4].empty() || token(f[4]) != f[4]
        || !integer(f[5], quality) || quality < UNVERIFIED || quality > PENDING || !integer(f[6], published) || published <= 0) return false;
    Frame x; x.key = key; x.pane = pane; x.contract = f[4]; x.quality = (int)quality; x.published = published; out = x; return true;
}
// Publisher state is per host binding; failed writes are retried, not marked seen.
class Publisher {
    std::string path_;
    std::set<std::string> ids_;
    std::map<long long, int> frames_;
public:
    void reset() { path_.clear(); ids_.clear(); frames_.clear(); }
    bool append(const std::string& path, const std::vector<Event>& events, const std::vector<Frame>& frames) {
        if (path.empty()) return false;
        if (path_ != path) {
            reset(); path_ = path;
            std::ifstream in(path.c_str(), std::ios::binary);
            if (in) {
                in.seekg(0, std::ios::end); const long long end = (long long)in.tellg();
                const long long from = std::max<long long>(0, end - 4LL * 1024 * 1024);
                if (end > 0) {
                    in.seekg(from); std::string bytes((size_t)(end - from), '\0');
                    in.read(&bytes[0], (std::streamsize)bytes.size());
                    size_t at = 0; if (from > 0) { const size_t cut = bytes.find('\n'); at = cut == std::string::npos ? bytes.size() : cut + 1; }
                    for (;;) {
                        const size_t to = bytes.find('\n', at); if (to == std::string::npos) break;
                        if (to - at <= 4096) {
                            const std::string s = bytes.substr(at, to - at); Event e; Frame f;
                            if (parse(s, e)) ids_.insert(e.m.id);
                            else if (parse(s, f)) frames_[f.key] = f.quality;
                        }
                        at = to + 1;
                    }
                }
            }
        }
        std::string text; std::set<std::string> added; std::map<long long, int> changed;
        for (const Event& e : events) {
            if (!e.m.typed || e.m.quality == PENDING || e.m.published <= 0 || e.m.contract.empty() || e.m.id.empty() || ids_.count(token(e.m.id)) || added.count(token(e.m.id))) continue;
            Event check; const std::string s = line(e); if (!parse(s, check)) continue;
            text += s + '\n'; added.insert(check.m.id);
        }
        for (const Frame& f : frames) {
            if (f.published <= 0 || f.contract.empty()) continue;
            const auto old = changed.find(f.key); const auto before = frames_.find(f.key);
            if ((old != changed.end() && old->second == f.quality) || (old == changed.end() && before != frames_.end() && before->second == f.quality)) continue;
            Frame check; const std::string s = line(f); if (!parse(s, check)) continue;
            text += s + '\n'; changed[f.key] = f.quality;
        }
        if (text.empty()) return true;
        bool separator = false;
        { std::ifstream in(path.c_str(), std::ios::binary); if (in) { in.seekg(0, std::ios::end); if (in.tellg() > 0) { in.seekg(-1, std::ios::end); char c = '\n'; in.get(c); separator = c != '\n'; } } }
        std::ofstream out(path.c_str(), std::ios::binary | std::ios::app);
        if (!out) return false;
        if (separator) out.put('\n');
        out.write(text.data(), (std::streamsize)text.size()); out.flush(); if (!out) return false;
        ids_.insert(added.begin(), added.end()); for (const auto& x : changed) frames_[x.first] = x.second;
        while (ids_.size() > 40000) ids_.erase(ids_.begin());
        while (frames_.size() > 10000) frames_.erase(frames_.begin());
        return true;
    }
};
} // namespace rse
#endif
