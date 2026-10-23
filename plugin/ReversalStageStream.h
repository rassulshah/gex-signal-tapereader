#ifndef LS_REVERSAL_STAGE_STREAM_H
#define LS_REVERSAL_STAGE_STREAM_H
#include "ReversalEvidence.h"
#include <algorithm>

namespace rsh {
// Reads complete bounded lines only. A capped prefix is discarded, not parsed.
template<class Fn> inline bool tail(const std::string& path, long long& offset, Fn consume, bool* reset = nullptr) {
    std::ifstream in(path.c_str(), std::ios::binary); if (!in) return false;
    in.seekg(0, std::ios::end); const long long size = (long long)in.tellg(); if (size < 0) return false;
    if (size < offset) { offset = 0; if (reset) *reset = true; }
    if (size == offset) return true;
    const bool capped = size - offset > 4LL * 1024 * 1024;
    const long long from = capped ? size - 4LL * 1024 * 1024 : offset;
    in.seekg(from); std::string bytes((size_t)(size - from), '\0');
    in.read(&bytes[0], (std::streamsize)bytes.size()); if (in.gcount() != (std::streamsize)bytes.size()) return false;
    const size_t last = bytes.rfind('\n'); if (last == std::string::npos) return true;
    offset = from + (long long)last + 1;
    size_t a = 0;
    if (capped) { a = bytes.find('\n') + 1; }
    while (a <= last) {
        const size_t end = bytes.find('\n', a);
        if (end - a <= 4096) consume(rse::trimLine(bytes.substr(a, end - a)));
        a = end + 1;
    }
    return true;
}
inline bool append(const std::string& path, const std::string& text) {
    if (path.empty()) return false;
    if (text.empty()) return true;
    bool separator = false;
    { std::ifstream in(path.c_str(), std::ios::binary); if (in) { in.seekg(0, std::ios::end); if (in.tellg() > 0) { in.seekg(-1, std::ios::end); char c = '\n'; in.get(c); separator = c != '\n'; } } }
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::app); if (!out) return false;
    if (separator) out.put('\n');
    out.write(text.data(), (std::streamsize)text.size()); out.flush(); return (bool)out;
}
struct Assignment { long long key = 0, received = 0; rse::Event event; };
inline std::string line(const Assignment& a) {
    return "RJ|1|" + std::to_string(a.key) + "|" + std::to_string(a.received) + "|" + rse::line(a.event);
}
inline bool parse(const std::string& raw, Assignment& out) {
    const std::string s = rse::trimLine(raw); size_t at = 0; std::vector<std::string> f;
    for (int i = 0; i < 4; ++i) { const size_t end = s.find('|', at); if (end == std::string::npos) return false; f.push_back(s.substr(at, end - at)); at = end + 1; }
    Assignment a;
    if (f[0] != "RJ" || f[1] != "1" || !rse::integer(f[2], a.key) || a.key <= 0
        || !rse::integer(f[3], a.received) || a.received <= 0 || !rse::parse(s.substr(at), a.event)
        || a.event.key > a.key || a.event.m.published > a.received) return false;
    out = a; return true;
}
class Inbox {
    struct Pending { rse::Event event; long long received = 0; };
    std::string contract_, journal_;
    long long offsets_[3] = {0, 0, 0}, journalOffset_ = 0;
    bool available_[3] = {false, false, false};
    std::set<std::string> seen_, queued_;
    std::vector<Pending> pending_;
    std::map<std::pair<int, long long>, rse::Frame> frames_;
    std::vector<Assignment> restored_;
public:
    std::string paths[3];
    long long bad = 0, ignoredHistory = 0, wrongContract = 0, writeFails = 0, dropped = 0;
    void bind(const std::string& directory, const std::string& market, const std::string& contract, int spb, const std::string& journal) {
        if (journal_ == journal && contract_ == rse::token(contract)) return;
        contract_ = rse::token(contract); journal_ = journal; journalOffset_ = 0;
        seen_.clear(); queued_.clear(); pending_.clear(); frames_.clear(); restored_.clear();
        for (int pane = 0; pane < 3; ++pane) { available_[pane] = false; offsets_[pane] = 0; paths[pane] = directory + "\\" + rse::fileName(pane, market, contract, spb); }
    }
    void read(long long now, long long latestClosed, long long oldestAccepted, bool haveHistory) {
        tail(journal_, journalOffset_, [&](const std::string& s) {
            Assignment a; if (!parse(s, a)) { ++bad; return; }
            if (a.event.m.contract != contract_) { ++wrongContract; return; }
            if (a.received > now) { ++bad; return; }
            if (seen_.insert(a.event.m.id).second) restored_.push_back(a);
        });
        for (int pane = 0; pane < 3; ++pane) {
            bool reset = false, cleared = false;
            auto clearFrames = [&]() { for (auto it = frames_.begin(); it != frames_.end();) { if (it->first.first == pane) it = frames_.erase(it); else ++it; } cleared = true; };
            available_[pane] = tail(paths[pane], offsets_[pane], [&](const std::string& s) {
                if (reset && !cleared) clearFrames();
                rse::Event e; rse::Frame f;
                if (rse::parse(s, f)) {
                    if (f.pane != pane || f.contract != contract_) { ++wrongContract; return; }
                    if (f.published > now) { ++bad; return; }
                    const auto k = std::make_pair(pane, f.key); const auto old = frames_.find(k);
                    if (old == frames_.end() || f.published >= old->second.published) frames_[k] = f;
                    return;
                }
                if (!rse::parse(s, e)) { ++bad; return; }
                if (e.pane != pane || e.m.contract != contract_) { ++wrongContract; return; }
                if (e.m.published > now || e.m.cause > e.key) { ++bad; return; }
                if (seen_.count(e.m.id) || queued_.count(e.m.id)) return;
                if (e.key < oldestAccepted || (!haveHistory && e.key < latestClosed)) { ++ignoredHistory; return; }
                Pending p; p.event = e; p.received = now; pending_.push_back(p); queued_.insert(e.m.id);
            }, &reset);
            if (reset && !cleared) clearFrames();
        }
        while (frames_.size() > 12000) frames_.erase(frames_.begin());
        while (seen_.size() > 60000) { seen_.erase(seen_.begin()); ++dropped; }
        if (pending_.size() > 10000) {
            const size_t n = pending_.size() - 10000;
            for (size_t i = 0; i < n; ++i) queued_.erase(pending_[i].event.m.id);
            pending_.erase(pending_.begin(), pending_.begin() + (long)n); dropped += (long long)n;
        }
    }
    std::vector<Assignment> takeRestored() { std::vector<Assignment> out; out.swap(restored_); return out; }
    int quality(long long key) const {
        const auto a = frames_.find(std::make_pair(lst::ATK, key)), d = frames_.find(std::make_pair(lst::DEF, key));
        if (a == frames_.end() || d == frames_.end()) return rse::PENDING;
        if (!available_[lst::ATK] || !available_[lst::DEF]) return rse::UNAVAILABLE;
        const int q[2] = {a->second.quality, d->second.quality};
        if (q[0] == rse::PENDING || q[1] == rse::PENDING) return rse::PENDING;
        if (q[0] == rse::UNAVAILABLE || q[1] == rse::UNAVAILABLE) return rse::UNAVAILABLE;
        if (q[0] == rse::PARTIAL || q[1] == rse::PARTIAL || dropped > 0) return rse::PARTIAL;
        if (q[0] == rse::UNVERIFIED || q[1] == rse::UNVERIFIED) return rse::UNVERIFIED;
        return rse::COMPLETE;
    }
    bool terminal(long long key) const { return quality(key) != rse::PENDING; }
    size_t pendingCount() const { return pending_.size(); }
    // Call only for an uncommitted closed bar. Journal is written before admission.
    bool admit(long long key, long long now, std::vector<Assignment>& out) {
        std::vector<Assignment> added; std::string text;
        for (const Pending& p : pending_) {
            if (p.event.key > key || p.received > now) continue;
            if (seen_.count(p.event.m.id)) continue;
            Assignment a; a.key = key; a.received = p.received; a.event = p.event; added.push_back(a); text += line(a) + '\n';
        }
        if (!append(journal_, text)) { ++writeFails; return false; }
        for (const Assignment& a : added) { seen_.insert(a.event.m.id); queued_.erase(a.event.m.id); out.push_back(a); }
        pending_.erase(std::remove_if(pending_.begin(), pending_.end(), [&](const Pending& p) { return seen_.count(p.event.m.id) != 0; }), pending_.end());
        return true;
    }
};
} // namespace rsh
#endif
