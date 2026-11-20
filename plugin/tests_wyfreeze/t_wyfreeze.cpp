// tests_wyfreeze: WyFreeze.h (RevRead 0.20.3) - a replay across 5 PM keeps the earlier session's key-level marks byte-identical
#include "../WyFreeze.h"
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
static int fails = 0, n = 0;
#define CK(c) do { ++n; if (!(c)) { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

// a 1-minute chart: session A = 11-19 (17:00 the day before .. 15:59), session B = 11-20 (from 17:00). stamp = minutes, A starts at 0
static const long long A0 = 0, A1 = 22 * 60 + 59, B0 = 24 * 60;   // A: 17:00 .. 15:59 (+1 day); the halt 16:00-16:59; B from 17:00
static std::string sessOf(long long t) { return t >= B0 ? "20261120" : "20261119"; }
// the recompute: a session that is seg 0 gets the level files (key-level names), older sessions only hour levels (2hL) - and some marks vanish
static std::vector<std::string> recompute(long long lastClosed) {
    std::vector<std::string> v; const bool aLive = lastClosed <= A1;
    for (long long t = A0; t <= std::min(lastClosed, A1); ++t) {
        if (t % 97 == 5) v.push_back(std::to_string(t) + "|A|1");
        if (t % 131 == 7) v.push_back(std::to_string(t) + (aLive ? "|Sw|1|PDL|7305|V99+" : "|Sw|1|2hL|7305|"));
        if (t % 173 == 11 && aLive) v.push_back(std::to_string(t) + "|Bk|-1|ONL|7343|V99+");   // gone without the level files
        if (t == A1 && aLive) v.push_back(std::to_string(t) + "|Rev|-1||0|V40");   // the 15:59 candle: in at 5 PM
        if (t % 211 == 3) v.push_back(std::to_string(t) + (aLive ? "|Tst|1|PDL|0|62%" : "|Tst|1|2hL|0|40%")); }
    v.push_back("L|7305|PDL");
    for (long long t = B0; t <= lastClosed; ++t) if (t % 50 == 1) v.push_back(std::to_string(t) + "|Bk|1|PHH|7425|");
    return v;
}
struct Disk { std::map<std::string, std::string> f; int writes = 0; };
static std::vector<std::string> step(Disk& d, long long forming) {
    const long long last = forming == B0 ? A1 : forming - 1;   // the last closed candle (the halt has no candles)
    std::vector<wyf::Seg> sg; wyf::Seg s0; s0.seg = 0; s0.nm = sessOf(last); s0.t0 = last >= B0 ? B0 : A0; s0.t1 = last; sg.push_back(s0);
    if (last >= B0) { wyf::Seg s1; s1.seg = 1; s1.nm = "20261119"; s1.t0 = A0; s1.t1 = A1; sg.push_back(s1); }
    return wyf::apply(recompute(last), sg, sessOf(forming) == sessOf(last), "ES", 60, [](const std::string& nm) { return "wymarks-ES-60-" + nm + ".txt"; },
        [&](const std::string& p) { const auto it = d.f.find(p); return it == d.f.end() ? std::string() : it->second; },
        [&](const std::string& p, const std::string& t) { d.f[p] = t; ++d.writes; });
}
static std::string inA(const std::vector<std::string>& v) { std::string s; for (const std::string& l : wyf::inRange(v, A0, A1)) s += l + "\n"; return s; }

int main() {
    // 1. the replay: candle by candle through the session, across 5 PM and 300 candles into the next one
    Disk d; std::string lastLiveA; std::vector<std::string> out;
    for (long long f = A0 + 1; f <= A1; ++f) { out = step(d, f); lastLiveA = inA(out); }
    out = step(d, B0);   // the 5 PM recompute: the forming candle is the new session, A is still seg 0
    const std::string fullA = inA(recompute(A1)); CK(inA(out) == fullA); CK(fullA.find("|Bk|-1|ONL|") != std::string::npos);
    CK(lastLiveA != fullA);   // the 15:59 candle's marks came in at 5 PM
    for (long long f = B0 + 1; f <= B0 + 300; ++f) { out = step(d, f); CK(inA(out) == fullA); }
    CK(inA(recompute(B0 + 300)) != fullA);   // without the freeze it repaints (the bug)
    CK(inA(recompute(B0 + 300)).find("|Bk|-1|ONL|") == std::string::npos);
    CK(inA(out).find("|Sw|1|PDL|7305|V99+") != std::string::npos && inA(out).find("|2hL|") == std::string::npos);
    { int nb = 0; for (const auto& l : out) if (l.find("|PHH|") != std::string::npos) ++nb; CK(nb == 6); }   // B's own marks as computed
    { int nl = 0; for (const auto& l : out) if (l.rfind("L|", 0) == 0) ++nl; CK(nl == 1); }
    CK(d.f.count("wymarks-ES-60-20261119.txt") == 1 && d.f.count("wymarks-ES-60-20261120.txt") == 1);
    const std::string frozenA = d.f["wymarks-ES-60-20261119.txt"];
    { const int w = d.writes; step(d, B0 + 301); CK(d.f["wymarks-ES-60-20261119.txt"] == frozenA); CK(d.writes == w + 1); }   // only B's file is rewritten
    // 2. a restart the next day (state gone, files kept): still frozen
    { Disk d2; d2.f = d.f; out = step(d2, B0 + 400); CK(inA(out) == fullA); }
    // 3. a torn / foreign / oversized file: not used (the recompute, as before)
    { Disk d3; d3.f = d.f; std::string& t = d3.f["wymarks-ES-60-20261119.txt"]; t = t.substr(0, t.size() - 4); out = step(d3, B0 + 400); CK(inA(out) == inA(recompute(B0 + 399))); }
    { CK(!wyf::parse(frozenA, "NQ", 60, "20261119").ok); CK(!wyf::parse(frozenA, "ES", 180, "20261119").ok); CK(!wyf::parse(frozenA, "ES", 60, "20261118").ok); CK(wyf::parse(frozenA, "ES", 60, "20261119").ok); }
    { std::string big = frozenA; big.append(wyf::MAX_BYTES, 'x'); CK(!wyf::parse(big, "ES", 60, "20261119").ok); CK(!wyf::parse("", "ES", 60, "20261119").ok); }
    { std::string bad = frozenA; const size_t p = bad.find('\n') + 1; bad.insert(p, "99999999|Sw|1|X|0|\n"); CK(!wyf::parse(bad, "ES", 60, "20261119").ok); }
    // 4. a session that is over but has no file (from before 0.20.3): the recompute, as before; seg 0 at 5 PM with no file writes it
    { Disk d4; out = step(d4, B0 + 10); CK(inA(out) == inA(recompute(B0 + 9))); CK(d4.f.count("wymarks-ES-60-20261119.txt") == 0); }
    { Disk d5; out = step(d5, B0); CK(inA(out) == fullA); CK(d5.f.count("wymarks-ES-60-20261119.txt") == 1); }
    // 5. a file written mid-session (IRT closed at noon): the frozen part + the candles after it
    { Disk d6; for (long long f = A0 + 1; f <= 1140; ++f) step(d6, f); out = step(d6, B0 + 5);
      const wyf::Sess z = wyf::parse(d6.f["wymarks-ES-60-20261119.txt"], "ES", 60, "20261119"); CK(z.ok && z.t1 == 1139);
      for (const auto& l : wyf::inRange(out, A0, 1139)) CK(l.find("|2hL|") == std::string::npos); }
    // 5b. the session read in two segments (seg 0 = its last 600 candles, seg 1 = the rest): one file, nothing dropped
    { Disk d7; auto two = [&](long long forming) { const long long last = forming - 1; std::vector<wyf::Seg> sg; wyf::Seg a; a.seg = 0; a.nm = "20261119"; a.t0 = last - 599; a.t1 = last; sg.push_back(a);
          wyf::Seg b; b.seg = 1; b.nm = "20261119"; b.t0 = A0; b.t1 = last - 600; sg.push_back(b);
          return wyf::apply(recompute(last), sg, true, "ES", 60, [](const std::string& nm) { return "wymarks-ES-60-" + nm + ".txt"; },
              [&](const std::string& p) { const auto it = d7.f.find(p); return it == d7.f.end() ? std::string() : it->second; }, [&](const std::string& p, const std::string& t) { d7.f[p] = t; }); };
      out = two(1000); CK(inA(out) == inA(recompute(999))); CK(d7.f.size() == 1);
      const wyf::Sess z = wyf::parse(d7.f["wymarks-ES-60-20261119.txt"], "ES", 60, "20261119"); CK(z.ok && z.t0 == A0 && z.t1 == 999 && z.lines == wyf::inRange(recompute(999), A0, A1)); }
    // 6. round trip of the real NQ wy body
    { std::ifstream f("../tests_volrange/wy_NQ_1119.txt"); std::stringstream ss; ss << f.rdbuf(); std::vector<std::string> v = wyf::splitLines(ss.str()); v.erase(v.begin());
      long long t0 = 0, t1 = 0, t = 0; bool first = true; std::vector<std::string> st; for (const auto& l : v) if (wyf::stampOf(l, t)) { if (first || t < t0) t0 = t; if (first || t > t1) t1 = t; first = false; st.push_back(l); }
      CK(st.size() > 200); const wyf::Sess z = wyf::parse(wyf::fileText("NQ", 180, "20261119", t0, t1, st), "NQ", 180, "20261119"); CK(z.ok && z.lines == st);
      CK(!wyf::stampOf("L|109410|2hL", t) && !wyf::stampOf("WY|1|NQ", t) && wyf::stampOf("3877712100|Bk|-1|2hL|109410|", t) && t == 3877712100LL); }
    std::printf("%d/%d\n", n - fails, n); return fails ? 1 : 0;
}
