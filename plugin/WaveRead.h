// WaveRead.h  --  the wave read of the Cob Reader pane (lsZzoCombo 3.1.0). SDK-free.
// (1.0.0, 2026-11-17 Rassul 06:07-07:02 CT "lets move on to the Cob wave volume", "is there anything else you can read in the wave", "add a second
// and third line line which can describe these behaviors but dont forget absorbion", "dont have cob Reader version there because this space is very
// valuable. the first line is the verdict the second line is the wave and then you can have 2 lines which can be behavioral", "build all , make sure
// that as i move my mouse on different bars i get that bars / waves information"; mockup claude.ai/artifact/3GEqJ61mb24wNyuSvNz2JM)
// The wave read is about ONE wave: the wave under the mouse, else the last finished wave. The wave after it (if any) is its pullback.
//   line 1  the verdict + up to two reasons      line 2  the wave: size, count, did the volume move price
//   lines 3-4  the two most important behaviours: absorption, climax, thrust shortening, the earlier climax, the pullback, volume building
// A "?" follows a verdict that leans on an unproven read (absorption, climax, shortening, building) - UNPROVEN until the nightly scores them.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace wvr {

static const char* const VERSION = "1.1.0";   // (1.1.0, 2026-11-17 Rassul 08:35-08:44 CT "a split with the left having a verdict and human readable explanation and the right side to have the data points": Out::ex = the plain-English sentences)
static const double STRONG_X = 2.0;      // a strong wave: 2x the median of the last 5 same-way waves or more
static const double LIGHT_X = 0.33;      // a light pullback: its volume is a third of the wave's or less (his Lvp rule)
static const double HEAVY_X = 0.8;       // a heavy pullback: 0.8x the wave's volume or more (the Cob check's counter-wave warning)
static const double PROGRESS_MIN = 0.5;  // "little progress": the wave's price size ratio is under half its volume ratio
static const double BUILD_X = 1.5;       // "building": one side's volume over the last 3 waves each way is 1.5x the other's

struct W { int dir = 0; long long vol = 0; double volX = 0; int cnt = 0; bool cntCap = false, live = false; double sizeX = 0;   // sizeX = price size / the median size (0 = unknown)
           bool ab = false, cmx = false; long long size = 0; };
struct Row { std::string lab, a, b, c; int ca = 0, cb = 0, cc = 0; };   // colours: +1 green, -1 red, 0 white, 2 grey
struct Out { bool ok = false; int verdict = 0; bool q = false; std::string head, why, wave, b1, b2; std::vector<Row> rows; std::vector<std::string> ex; };   // ex (1.1.0) = the explanation sentences, most important first   // verdict +1 / -1 / 2 mixed / 0 quiet

inline std::string commas(long long v) { std::string s = std::to_string(v < 0 ? -v : v); for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert((size_t)i, ","); return v < 0 ? "-" + s : s; }
inline std::string x1(double v) { char b[24]; std::snprintf(b, sizeof b, "%.1fx", (std::min)(v, 99.9)); return b; }
inline std::string x2(double v) { char b[24]; std::snprintf(b, sizeof b, "%.2fx", (std::min)(v, 99.99)); return b; }
// the wave the read is about: pick if valid, else the last finished wave (-1 = none)
inline int subject(const std::vector<W>& ws, int pick) {
    if (pick >= 0 && pick < (int)ws.size()) return pick;
    for (int i = (int)ws.size() - 1; i >= 0; --i) if (!ws[(size_t)i].live) return i;
    return ws.empty() ? -1 : 0;
}
inline Out read(const std::vector<W>& ws, int pick, const std::string& cobLine) {
    Out o; const int p = subject(ws, pick); if (p < 0) return o;
    const W& w = ws[(size_t)p]; if (w.dir == 0) return o;
    o.ok = true; const bool up = w.dir > 0; const char* dirW = up ? "up" : "down"; const char* ext = up ? "high" : "low";
    const W* pb = p + 1 < (int)ws.size() ? &ws[(size_t)p + 1] : nullptr;      // the wave against it
    const W* pv = p >= 1 ? &ws[(size_t)p - 1] : nullptr;                      // the wave before it (the other way)
    const bool strong = w.volX >= STRONG_X;
    const bool little = w.volX > 0 && w.sizeX > 0 && w.sizeX / w.volX < PROGRESS_MIN && w.volX >= 1.0;
    bool shorten = false;   // the third wave this way in a row that covered less ground than the one before
    if (p >= 4 && ws[(size_t)p - 2].dir == w.dir && ws[(size_t)p - 4].dir == w.dir && w.size > 0 && w.size < ws[(size_t)p - 2].size && ws[(size_t)p - 2].size < ws[(size_t)p - 4].size) shorten = true;
    const double pbX = pb && w.vol > 0 ? (double)pb->vol / (double)w.vol : -1;
    const bool light = strong && pb && pbX >= 0 && pbX <= LIGHT_X, heavy = strong && pb && pbX >= HEAVY_X;   // a pullback is judged only against a strong wave
    const bool prevTurn = pv && pv->dir == -w.dir && (pv->cmx || pv->ab) && strong;   // the wave before was a climax / absorbed, and this one answered strongly
    long long su = 0, sd = 0; int nu = 0, nd = 0;
    for (int i = (pb ? p + 1 : p); i >= 0 && (nu < 3 || nd < 3); --i) { const W& q = ws[(size_t)i]; if (q.dir > 0 && nu < 3) { su += q.vol; ++nu; } else if (q.dir < 0 && nd < 3) { sd += q.vol; ++nd; } }
    int build = 0; if (nu == 3 && nd == 3 && su > 0 && sd > 0) { if ((double)su >= BUILD_X * (double)sd) build = 1; else if ((double)sd >= BUILD_X * (double)su) build = -1; }
    // line 2: the wave
    o.wave = std::string(up ? "up " : "down ") + (w.volX > 0 ? x1(w.volX) : commas(w.vol));
    if (w.cnt > 0 || w.cntCap) o.wave += std::string(", biggest in ") + (w.cntCap ? std::to_string(w.cnt) + "+" : std::to_string(w.cnt));
    if (w.volX > 0 && w.sizeX > 0) o.wave += little ? "; heavy volume, little progress" : "; volume moved price";
    // lines 3-4: the behaviours, most important first
    std::vector<std::string> B;
    if (w.ab) B.push_back(std::string(up ? "Buyers" : "Sellers") + " absorbed at the " + ext + " ?");
    if (w.cmx) B.push_back(std::string("Climax at the ") + ext + " ?");
    if (shorten) B.push_back(std::string("Thrust shortening: third ") + dirW + " wave, less ground each time ?");
    if (prevTurn) B.push_back(std::string("Earlier ") + (up ? "selling" : "buying") + " climax was absorbed at the " + (up ? "low" : "high") + " ?");
    if (pb) { if (pb->ab) B.push_back("Pullback absorbed ?"); else if (light) B.push_back("Pullback light (" + x2(pbX) + "), not absorbed yet"); else if (heavy) B.push_back("Pullback heavy (" + x2(pbX) + ")"); }
    if (build) B.push_back(std::string(build > 0 ? "Up" : "Down") + " volume building: " + x1(build > 0 ? (double)su / (double)sd : (double)sd / (double)su) + " over the last 3 waves ?");
    if (!B.empty()) o.b1 = B[0];
    if (B.size() > 1) o.b2 = B[1];
    {   // (1.1.0) the sentences: the behaviours in words, then the wave's size
        const char* Bu = up ? "Buyers" : "Sellers"; const char* bo = up ? "buying" : "selling";
        if (w.ab) o.ex.push_back(std::string(Bu) + " were absorbed at the " + ext + " - the other side soaked up the " + bo + ".");
        if (w.cmx) o.ex.push_back(std::string("A climax at the ") + ext + " - often the last push before a turn.");
        if (shorten) o.ex.push_back(std::string("The third ") + dirW + " wave in a row covered less ground - the push is tiring.");
        if (prevTurn) o.ex.push_back(std::string("This strong wave answered the ") + (up ? "selling" : "buying") + " climax at the " + (up ? "low" : "high") + " - " + (up ? "buyers" : "sellers") + " took over.");
        if (pb) { if (pb->ab) o.ex.push_back(std::string("Its pullback was absorbed - ") + (up ? "sellers" : "buyers") + " could not push back.");
                  else if (light) o.ex.push_back("The pullback was light (" + x2(pbX) + " of the wave) - little " + (up ? "selling" : "buying") + " against it.");
                  else if (heavy) o.ex.push_back("The pullback was heavy (" + x2(pbX) + " of the wave) - real " + (up ? "selling" : "buying") + " against it."); }
        if (build) o.ex.push_back(std::string(build > 0 ? "Buyers are putting more volume into the up waves - " : "Sellers are putting more volume into the down waves - ") + x1(build > 0 ? (double)su / (double)sd : (double)sd / (double)su) + (build > 0 ? " the down waves" : " the up waves") + " over the last 3.");
        if (little) o.ex.push_back(std::string("Heavy volume but little progress - someone is absorbing the ") + bo + ".");
        std::string sz = std::string("This ") + dirW + " wave was ";
        if (w.volX <= 0) sz += commas(w.vol) + " lots.";
        else if (strong) sz += "strong (" + x1(w.volX) + " its normal volume) - a real push.";
        else if (w.volX < 0.7) sz += "light (" + x1(w.volX) + ") - little conviction.";
        else sz += std::string("normal size (") + x1(w.volX) + ")" + (build && build != w.dir ? ", so " + std::string(up ? "buyers" : "sellers") + " are not pressing." : ".");
        o.ex.push_back(sz);
        if (o.ex.size() == 1) o.ex.push_back("Nothing unusual in this wave.");
    }
    // line 1: the verdict, for the wave's direction (+) or against it (-)
    struct Why { int s; std::string t; }; std::vector<Why> Y;
    if (strong) Y.push_back(Why{ 1, std::string("strong ") + dirW + " wave" });
    if (prevTurn) Y.push_back(Why{ 1, std::string(up ? "selling" : "buying") + " climax absorbed" });
    if (light) Y.push_back(Why{ 1, "light pullback" });
    if (pb && pb->ab) Y.push_back(Why{ 1, "pullback absorbed" });
    if (w.cmx) Y.push_back(Why{ -2, std::string("climax at the ") + ext });   // the turn warnings weigh double
    if (w.ab) Y.push_back(Why{ -2, std::string(up ? "buyers" : "sellers") + " absorbed" });
    if (little && strong) Y.push_back(Why{ -1, "little progress" });
    if (shorten) Y.push_back(Why{ -2, "thrust shortening" });
    if (heavy) Y.push_back(Why{ -1, "heavy pullback" });
    if (build) Y.push_back(Why{ build * w.dir, std::string(build > 0 ? "up" : "down") + " volume building" });
    int s = 0; for (const Why& y : Y) s += y.s;
    o.q = w.cmx || w.ab || shorten || prevTurn || build != 0 || (pb && pb->ab);
    if (Y.empty()) { o.verdict = 0; o.head = "Quiet"; }
    else {
        const int vd = s > 0 ? w.dir : s < 0 ? -w.dir : 0; o.verdict = vd == 0 ? 2 : vd;
        o.head = std::string(vd > 0 ? "Bullish" : vd < 0 ? "Bearish" : "Mixed") + (o.q ? " ?:" : ":");
        // the reasons: for a clear verdict the ones that point its way first; mixed = one each way
        std::vector<std::string> R; const int want = s > 0 ? 1 : s < 0 ? -1 : 0;
        if (want) { for (const Why& y : Y) if ((y.s > 0) == (want > 0) && R.size() < 2) R.push_back(y.t); for (const Why& y : Y) if ((y.s > 0) != (want > 0) && R.size() < 2) R.push_back("but " + y.t); }
        else { for (const Why& y : Y) if (y.s > 0) { R.push_back(y.t); break; } for (const Why& y : Y) if (y.s < 0) { R.push_back(y.t); break; } }
        for (size_t i = 0; i < R.size(); ++i) o.why += (i ? ", " : "") + R[i];
    }
    // the numbers
    { Row r; r.lab = "Wave"; r.a = std::string(up ? "Up " : "Dn ") + commas(w.vol); r.ca = w.dir; r.b = w.volX > 0 ? x1(w.volX) : "-"; r.cb = w.dir;
      if (w.cnt > 0 || w.cntCap) { r.c = std::string("biggest in ") + (w.cntCap ? std::to_string(w.cnt) + "+" : std::to_string(w.cnt)); }
      o.rows.push_back(r); }
    { Row r; r.lab = "Pullback"; if (pb) { r.a = std::string(pb->dir > 0 ? "Up " : "Dn ") + commas(pb->vol); r.ca = pb->dir; r.b = x2(pbX) + " of wave"; if (light) { r.c = "light"; r.cc = w.dir; } else if (heavy) { r.c = "heavy"; r.cc = -w.dir; } } else { r.a = "-"; r.ca = 2; } o.rows.push_back(r); }
    { Row r; r.lab = "Last 3"; if (nu == 3 && nd == 3) { r.a = "Up " + commas(su); r.ca = 1; r.b = "Dn " + commas(sd); r.cb = -1; if (su > 0 && sd > 0) { const bool u = su >= sd; r.c = std::string(u ? "Up " : "Dn ") + x1(u ? (double)su / (double)sd : (double)sd / (double)su); r.cc = u ? 1 : -1; } } else { r.a = "-"; r.ca = 2; } o.rows.push_back(r); }
    { Row r; r.lab = "Cob"; r.a = cobLine.empty() ? "-" : cobLine; r.ca = cobLine.empty() ? 2 : 0; o.rows.push_back(r); }
    return o;
}

} // namespace wvr
