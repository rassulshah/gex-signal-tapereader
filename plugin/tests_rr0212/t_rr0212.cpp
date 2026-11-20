// t_rr0212.cpp -- lsRevRead / lsRevReadMarks 0.21.2 (Rassul 2026-11-20 09:20 CT "add [the I] back ... add the mark to the signals line ... add the
// badge ... include the level and strength line, if both sweep and i are true, only show sweep"; mockup i-badge-mockup.html):
//   WyRead 1.9.0 (ilvC, stripFromLines), LevelMarks 1.7.0 (keyForI, iPick, iUnderSw), WyMarks 2.4.0 (I side, iLine).
// pipeline() mirrors the host (RevRead.cpp 0.21.2 wy block) step by step: named levels -> wyr::run -> iPick -> named filter -> lvm::run ->
// fillStrength (the Sw >= 20 rule) -> I strength -> iUnderSw -> the wy lines -> the strip (stripFromLines + stripPick) and the price badges (parseAll).
// Fixtures: ES / NQ 11-19 and 11-20 (store OHLC, buy / sell = the pane's V lines, the key-level files of that session).
// build: g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. t_rr0212.cpp -o t && ./t
#include "../LevelMarks.h"
#include "../WyMarks.h"
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>

static int nf = 0, nok = 0;
static void C(bool c, const std::string& m) { if (c) ++nok; else { ++nf; std::printf("FAIL %s\n", m.c_str()); } }
static std::vector<std::string> split(const std::string& s) { std::vector<std::string> v; std::stringstream ss(s); std::string x; while (std::getline(ss, x, '|')) v.push_back(x); return v; }
static std::map<long long, std::string> whenOf;
static void load(const char* fn, std::vector<wyr::Bar>& B, std::vector<wyr::Lvl>& K) {
    std::ifstream in(fn); std::string line;
    while (std::getline(in, line)) { auto v = split(line); if (v.empty()) continue;
        if (v[0] == "L" && v.size() >= 3) { wyr::Lvl z; z.px = std::stoll(v[1]); z.name = v[2]; K.push_back(z); }
        if (v[0] == "B" && v.size() >= 9) { wyr::Bar b; b.t = std::stoll(v[1]); b.o = std::stoll(v[2]); b.h = std::stoll(v[3]); b.l = std::stoll(v[4]); b.c = std::stoll(v[5]);
            b.buy = std::stoll(v[6]); b.sell = std::stoll(v[7]); b.when = v[8]; whenOf[b.t] = v[8]; B.push_back(b); } } }

struct Pipe { std::vector<std::string> lines; int shownI = 0, flaggedI = 0, unnamedDropped = 0; std::map<long long, std::vector<wyr::Mark> > other; };
// the host's wy block for ONE live session (seg 0) of closed candles B with the level files K
static Pipe pipeline(const std::vector<wyr::Bar>& B, const std::vector<wyr::Lvl>& Kraw, bool forceSw = false) {   // forceSw: every I candle also gets a same-direction Sw (the Sw + I case)
    Pipe P; std::vector<wyr::Lvl> K; for (const wyr::Lvl& z : Kraw) if (lvm::keyForI(z.name, z.px)) K.push_back(z);   // lvNow
    std::vector<wyr::Lvl> KI; for (const wyr::Lvl& z : K) if (lvm::levelForI(z.name, z.px)) KI.push_back(z);   // (0.21.2) no file PHH / PHL for the I
    const wyr::Out o = wyr::run(B, 0.25, 2, wyr::Params(), KI);
    std::map<long long, std::vector<wyr::Mark> > marks = o.marks; std::map<long long, std::pair<std::string, long long> > iLv;
    for (const auto& kv : o.ilv) { const auto hb = o.ilvHb.find(kv.first); int h = hb != o.ilvHb.end() ? hb->second : 0; std::string nm = kv.second.second; long long px = kv.second.first;
        const auto ic = o.ilvC.find(kv.first); int sd = 0; const auto mk = o.marks.find(kv.first); if (mk != o.marks.end()) for (const wyr::Mark& m : mk->second) if (m.k == 'I') sd = m.dir;
        wyr::Lvl z; if (ic != o.ilvC.end() && sd != 0 && lvm::iPick(ic->second, sd, z)) { nm = z.name; px = z.px; h = z.hb; }
        iLv[kv.first] = std::make_pair(lvm::iLevel(nm, h, nm.find("high") != std::string::npos), px); }
    for (auto it = marks.begin(); it != marks.end(); ) { const auto il = iLv.find(it->first); const bool named = il != iLv.end() && !il->second.first.empty();
        if (!named) { auto& v = it->second; const size_t n0 = v.size(); v.erase(std::remove_if(v.begin(), v.end(), [](const wyr::Mark& m) { return m.k == 'I'; }), v.end()); P.unnamedDropped += (int)(n0 - v.size()); }
        if (it->second.empty()) it = marks.erase(it); else ++it; }
    std::map<long long, lvm::Mark> lv = lvm::run(B, K);
    std::vector<long long> T; std::vector<double> V, R; std::vector<char> G;
    for (const wyr::Bar& b : B) { T.push_back(b.t); V.push_back((double)(b.buy + b.sell)); R.push_back((double)(b.h - b.l)); G.push_back(b.c >= b.o ? 1 : 0); }
    lvm::fillStrength(lv, T, V, R, G);
    if (forceSw) for (const auto& kv : marks) for (const wyr::Mark& m : kv.second) if (m.k == 'I') { lvm::Mark w; w.code = 'S'; w.dir = m.dir; w.lv = iLv[kv.first].first; w.px = iLv[kv.first].second; w.st = "V40"; lv[kv.first] = w; }
    std::map<long long, std::string> iSt; for (const auto& kv : marks) for (const wyr::Mark& m : kv.second) if (m.k == 'I') { const std::string s = lvm::strengthAt(T, V, R, G, kv.first); if (!s.empty()) iSt[kv.first] = s; }
    std::map<long long, int> iSw;
    for (auto it = marks.begin(); it != marks.end(); ) { auto& v = it->second;
        for (auto q = v.begin(); q != v.end(); ) { if (q->k == 'I' && lvm::iUnderSw(lv, it->first, q->dir)) { iSw[it->first] = q->dir; q = v.erase(q); } else ++q; }
        if (v.empty()) it = marks.erase(it); else ++it; }
    for (const auto& kv : marks) for (const wyr::Mark& m : kv.second) {
        if (m.k == 'I') { const auto il = iLv.find(kv.first); const auto is = iSt.find(kv.first); P.lines.push_back(wyi::iLine(kv.first, m.dir, il->second.first, il->second.second, is != iSt.end() ? is->second : "", false)); ++P.shownI; }
        else { P.lines.push_back(std::to_string(kv.first) + "|" + wyr::label(m) + "|" + std::to_string(m.dir)); P.other[kv.first].push_back(m); } }
    for (const auto& kv : iSw) { const auto il = iLv.find(kv.first); const auto is = iSt.find(kv.first); P.lines.push_back(wyi::iLine(kv.first, kv.second, il->second.first, il->second.second, is != iSt.end() ? is->second : "", true)); ++P.flaggedI; }
    for (const auto& kv : lv) P.lines.push_back(std::to_string(kv.first) + "|" + lvm::codeText(kv.second.code) + "|" + std::to_string(kv.second.dir) + "|" + kv.second.lv + "|" + std::to_string(kv.second.px) + "|" + kv.second.st);
    return P;
}
static std::string joinLines(const std::vector<std::string>& v) { std::string s = "WY|1|ES|180|0.21.2\n"; for (const std::string& l : v) s += l + "\n"; return s; }

int main() {
    // ---- 1. helpers
    C(lvm::keyForI("PDL", 100) && lvm::keyForI("LonLO", 100) && lvm::keyForI("PFL/PDL", 100), "keyForI: named key levels");
    C(!lvm::keyForI("VWAP", 100) && !lvm::keyForI("VWAP-1", 100) && !lvm::keyForI("VWAP+2", 100) && !lvm::keyForI("HVL", 100) && !lvm::keyForI("GEX 1", 100) && !lvm::keyForI("Call Resistance", 100)
      && !lvm::keyForI("the HBO test line", 100) && !lvm::keyForI("PDL", 0) && !lvm::keyForI("PDL|x", 100), "keyForI: no VWAP / bands, gamma / MenthorQ, HBO, merged or zero levels");
    C(!lvm::levelForI("PHL", 100) && !lvm::levelForI("PHH", 100) && lvm::levelForI("PDL", 100) && lvm::keyForI("PHL", 100), "levelForI: the file's PHH / PHL (they move hourly) are not I levels; Bk / Sw keep them");
    {   std::vector<wyr::Lvl> c(3); c[0].px = 104; c[0].name = "the 9 AM hour low"; c[0].hb = 1; c[1].px = 102; c[1].name = "PDL"; c[2].px = 106; c[2].name = "HVL"; wyr::Lvl z;
        C(lvm::iPick(c, 1, z) && z.name == "PDL" && z.px == 102, "iPick: PDL (pri 10) beats the further prior-hour low (pri 50); gamma never");
        std::vector<wyr::Lvl> d(2); d[0].px = 102; d[0].name = "VAL"; d[1].px = 105; d[1].name = "VAH"; C(lvm::iPick(d, 1, z) && z.px == 105, "iPick: a tie = the furthest reclaimed (bullish: the higher)");
        C(lvm::iPick(d, -1, z) && z.px == 102, "iPick: a tie, bearish: the lower");
        std::vector<wyr::Lvl> g(2); g[0].px = 102; g[0].name = "HVL"; g[1].px = 103; g[1].name = "VWAP-1"; C(!lvm::iPick(g, 1, z), "iPick: only gamma / VWAP levels -> no level (no I)"); }
    {   std::map<long long, lvm::Mark> lv; lvm::Mark s; s.code = 'S'; s.dir = 1; lv[500] = s; lvm::Mark b; b.code = 'B'; b.dir = 1; lv[600] = b;
        C(lvm::iUnderSw(lv, 500, 1), "iUnderSw: Sw + I same candle, same direction"); C(!lvm::iUnderSw(lv, 500, -1), "iUnderSw: opposite direction -> both");
        C(!lvm::iUnderSw(lv, 600, 1), "iUnderSw: a Bk does not hide the I"); C(!lvm::iUnderSw(lv, 700, 1), "iUnderSw: no mark"); }
    {   const std::string a = wyi::iLine(1000, 1, "PDL", 29221, "V35", false), b = wyi::iLine(1003, -1, "ONH", 29300, "", true);
        C(a == "1000|I|1|PDL|29221|V35", "iLine: 6 fields (" + a + ")"); C(b == "1003|I|-1|ONH|29300||Sw", "iLine: flagged = 7 fields (" + b + ")");
        std::map<long long, std::vector<wyi::M> > all; const int n = wyi::parseAll("WY|1|ES|180|0.21.2\n" + a + "\n" + b + "\n1003|Sw|-1|ONH|29300|V40\n", all);
        C(n == 2 && all.count(1000) && all[1000][0].code == 'I' && all[1000][0].lv == "PDL" && all[1000][0].st == "V35" && all[1000][0].px == 29221, "parseAll: the I badge = I / PDL / V35");
        C(all.count(1003) && all[1003].size() == 1 && all[1003][0].code == 'S', "parseAll: the flagged I is not drawn, the Sw is");
        const auto st = wyr::stripFromLines({ a, b, "1003|Sw|-1|ONH|29300|V40", "1006|Bk|1|PDH|1|", "1009|Cx|-1|PDH|1|", "L|29221|PDL", "1012|FT|1" });
        C(st.size() == 3 && st.at(1000).size() == 1 && st.at(1000)[0].lab == "I" && st.at(1000)[0].dir == 1 && !st.count(1003) && st.at(1006)[0].lab == "B" && st.at(1009)[0].lab == "C", "stripFromLines: I / B / C; the flagged I and the Sw not on the strip");
        wyi::M mi; mi.code = 'I'; mi.dir = 1; C(wyi::sideOf(mi) == -1, "I bullish: UNDER the candle"); mi.dir = -1; C(wyi::sideOf(mi) == 1, "I bearish: OVER the candle");
        wyi::M ms; ms.code = 'S'; ms.dir = 1; C(wyi::sideOf(ms) == 0, "Sw side unchanged (by the candle colour)"); }
    // ---- 2. strip priority: Sp / Ut > I > T > A > X > E > B > C, at most 2
    {   auto pick = [](std::vector<std::string> labs) { std::vector<wyr::SMk> in; for (const std::string& l : labs) { wyr::SMk x; x.lab = l; x.dir = 1; in.push_back(x); } std::string s; for (const wyr::SMk& m : wyr::stripPick(in, 2)) s += m.lab + " "; return s; };
        C(pick({ "C", "B", "E", "A", "T", "I", "Sp" }) == "Sp I ", "strip: Sp, I first"); C(pick({ "C", "T", "I" }) == "I T ", "strip: I before T");
        C(pick({ "E", "A", "X", "I" }) == "I A ", "strip: I before A"); C(pick({ "B", "C", "E" }) == "E B ", "strip: E > B > C"); C(pick({ "Ut", "I", "T" }) == "Ut I ", "strip: Ut, then I");
        C(pick({ "I", "I" }) == "I ", "strip: a letter once"); }
    // ---- 3. the badge layout: I under a green bullish candle, Sw / Bk tags around, nothing overlaps
    {   std::vector<wyi::Candle> Cd; for (int i = 0; i < 6; ++i) { wyi::Candle c; c.x = 100 + i * 9; c.yHigh = 200 - i; c.yLow = 230 - i; c.halfW = 3; c.green = i % 2 == 0; Cd.push_back(c); }
        std::vector<wyi::M> ms(5); ms[0].code = 'I'; ms[0].dir = 1; ms[1].code = 'S'; ms[1].dir = 1; ms[2].code = 'I'; ms[2].dir = -1; ms[3].code = 'B'; ms[3].dir = 1; ms[4].code = 'I'; ms[4].dir = 1;
        const int on[5] = { 0, 1, 1, 2, 3 }; std::vector<wyi::Want> W;
        for (int k = 0; k < 5; ++k) { wyi::Want w; w.candle = on[k]; w.side = wyi::sideOf(ms[(size_t)k]); w.w = 26; w.h = wyi::LINE1_H + 2 * wyi::LINE2_H; W.push_back(w); }
        const auto P = wyi::layout(Cd, W, 0, 600, 1000); bool all = true, clash = false;
        for (size_t k = 0; k < P.size(); ++k) { if (!P[k].ok) all = false; for (size_t j = k + 1; j < P.size(); ++j) if (P[k].ok && P[j].ok && wyi::hit(P[k], P[j])) clash = true;
            for (const wyi::Candle& c : Cd) { wyi::Box b; b.l = c.x - c.halfW; b.r = c.x + c.halfW; b.t = c.yHigh; b.b = c.yLow; if (P[k].ok && wyi::hit(P[k], b)) clash = true; } }
        C(all && !clash, "layout: 5 tags (I, Sw, I, Bk, I) on 4 neighbouring candles - all placed, no overlap with each other or a candle");
        C(P[0].t > Cd[0].yLow, "layout: the bullish I sits under its green candle"); C(P[2].b < Cd[1].yHigh, "layout: the bearish I sits over its candle"); }
    // ---- 4. real sessions: counts, named levels only, Sw + I -> Sw only, strip agrees with the badges, replay (no repaint)
    const char* F[4] = { "es1119.psv", "es1120.psv", "nq1119.psv", "nq1120.psv" };
    for (const char* fn : F) {
        std::vector<wyr::Bar> B; std::vector<wyr::Lvl> K; whenOf.clear(); load(fn, B, K); C(B.size() > 250 && K.size() >= 15, std::string(fn) + " loaded");
        const Pipe P = pipeline(B, K);
        std::map<long long, std::vector<wyi::M> > badges; wyi::parseAll(joinLines(P.lines), badges);
        const auto strip = wyr::stripFromLines(P.lines);
        std::string list; int rth = 0, badgeI = 0, stripI = 0, bad = 0;
        for (const auto& kv : badges) for (const wyi::M& m : kv.second) if (m.code == 'I') { ++badgeI;
            const auto w = whenOf[kv.first]; list += w + (m.dir > 0 ? " +" : " -") + m.lv + " " + (m.st.empty() ? "-" : m.st) + "; ";
            if (m.lv.empty() || lvm::pri(m.lv == "LLO" ? "LonLO" : m.lv == "LHI" ? "LonHI" : m.lv == "ALO" ? "AsiaLO" : m.lv == "AHI" ? "AsiaHI" : m.lv, (m.lv.size() == 3 && m.lv[1] == 'h') || m.lv == "PHL" || m.lv == "PHH" ? 1 : 0) < 0) ++bad;
            for (const wyi::M& x : kv.second) if (x.code == 'S' && x.dir == m.dir) ++bad;   // never an I badge with a same-direction Sw
            const long long mod = ((kv.first % 86400) + 86400) % 86400; if (mod > 8 * 3600 + 30 * 60 && mod <= 15 * 3600) ++rth; }
        for (const auto& kv : strip) for (const wyr::SMk& x : kv.second) if (x.lab == "I") { ++stripI; if (!badges.count(kv.first)) ++bad; }
        std::printf("%s: I shown %d (RTH %d), I under a Sw (flagged, not drawn) %d, unnamed dropped %d | %s\n", fn, P.shownI, rth, P.flaggedI, P.unnamedDropped, list.c_str());
        C(bad == 0, std::string(fn) + ": every I badge names a key level, never on a same-direction Sw candle, strip I = badge I");
        C(badgeI == P.shownI && stripI == P.shownI, std::string(fn) + ": badge count = strip count = shown");
        // Sw + I on the same candle (every I given a same-direction Sw): only the Sw shows - badge and strip; the I is written flagged
        if (P.shownI > 0) { const Pipe Q = pipeline(B, K, true); std::map<long long, std::vector<wyi::M> > bq; wyi::parseAll(joinLines(Q.lines), bq); const auto sq = wyr::stripFromLines(Q.lines);
            int iB = 0, swB = 0, iS = 0; for (const auto& kv : bq) for (const wyi::M& m : kv.second) { if (m.code == 'I') ++iB; if (m.code == 'S') ++swB; } for (const auto& kv : sq) for (const wyr::SMk& x : kv.second) if (x.lab == "I") ++iS;
            int flag7 = 0; for (const std::string& l : Q.lines) { const auto f = split(l); if (f.size() == 7 && f[1] == "I" && f[6] == "Sw") ++flag7; }
            C(Q.shownI == 0 && Q.flaggedI == P.shownI && flag7 == P.shownI && iB == 0 && iS == 0 && swB >= P.shownI, std::string(fn) + ": Sw + I -> only the Sw (badge + strip), the I recorded with the Sw flag (" + std::to_string(flag7) + ")"); }
        // the same session with gamma / VWAP levels added at the I's own level prices: nothing new (they never reach the I)
        {   std::vector<wyr::Lvl> K2 = K; for (const auto& kv : badges) for (const wyi::M& m : kv.second) if (m.code == 'I') { wyr::Lvl g; g.px = m.px + 1; g.name = "HVL"; K2.push_back(g); g.name = "VWAP-1"; g.px = m.px - 1; K2.push_back(g); }
            for (int q = 0; q < 6; ++q) { wyr::Lvl g; g.px = B[(size_t)(B.size() / 2)].l + q * 8 - 20; g.name = q % 2 ? "Call Resistance" : "VWAP+1"; K2.push_back(g); }
            C(pipeline(B, K2).lines == P.lines, std::string(fn) + ": gamma / VWAP levels change nothing"); }
        // replay with a restart after every candle (the host recomputes from scratch): an I line, once written, never changes or vanishes, and
        // an I shown on its candle's close is never hidden later (the Sw is decided on the same candles); a session's first 14 candles carry no
        // letters until the 15th closes (wyr::run needs 15 - unchanged since 0.17.0), so those are not counted as late
        {   std::set<std::string> seen; int changed = 0, hiddenLater = 0, appearedLate = 0;
            std::map<long long, std::string> firstI;
            for (size_t n = 15; n <= B.size(); ++n) { std::vector<wyr::Bar> Pp(B.begin(), B.begin() + (long)n); const Pipe Q = pipeline(Pp, K);
                std::map<long long, std::string> now; for (const std::string& l : Q.lines) { const auto f = split(l); if (f.size() >= 3 && f[1] == "I") now[std::stoll(f[0])] = l; }
                for (const auto& kv : firstI) { const auto it = now.find(kv.first); if (it == now.end()) ++hiddenLater; else if (it->second != kv.second) ++changed; }
                for (const auto& kv : now) if (!firstI.count(kv.first)) { if (kv.first != Pp.back().t && kv.first >= B[14].t) { ++appearedLate; std::printf("    late: %s at candle %s (%s)\n", kv.second.c_str(), whenOf[Pp.back().t].c_str(), whenOf[kv.first].c_str()); } firstI[kv.first] = kv.second; } }
            const Pipe full = pipeline(B, K); int fullI = 0; for (const std::string& l : full.lines) { const auto f = split(l); if (f.size() >= 3 && f[1] == "I") ++fullI; }
            std::printf("  replay %s: I lines changed %d, vanished %d, decided after their candle %d (full pass %d, replay %zu)\n", fn, changed, hiddenLater, appearedLate, fullI, firstI.size());
            C(changed == 0 && hiddenLater == 0, std::string(fn) + " replay: an I line never changes or vanishes (incl. the Sw flag)");
            C(appearedLate == 0, std::string(fn) + " replay: every I is decided at its own candle's close");
            C((int)firstI.size() == fullI, std::string(fn) + " replay: the same I's as one straight pass"); }
    }
    std::printf("%s %d pass, %d fail\n", nf ? "FAILED" : "ALL PASS", nok, nf); return nf;
}
