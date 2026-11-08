// tests_dintel/t_dintel.cpp - DeltaIntel.h 1.0.0 (lsDeltaProfile 2.33.0, the Intelligent Delta Profile)
//  1. PARITY: every *.fx fixture (a real session + its previous session, exported by lra.dp_intel 1.0.0 from the MBO store) is fed to
//     dintel::Engine one closed candle at a time; every letter Python decided (letter, candle, peak, side, lo, hi, the 15 context
//     bits, right / wrong) must be decided identically here, and nothing more.
//  2. NO REPAINT: decisions made with candles up to k never change when later candles arrive (a replay stops at every 25th candle).
//  3. GATE: a letter is shown only when DeltaCtxParams.h says SHOW / "?" for its market and its conditions hold; LOG letters never show.
//  4. RECORD LINE + CODE format.
// build: g++ -std=c++17 -O2 -Wall -Wextra -Werror -I.. -I. t_dintel.cpp -o t_dintel && ./t_dintel <fixture dir>
#include "../DeltaIntel.h"
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <tuple>

static int pass = 0, fail = 0;
#define CHECK(c, msg) do { if (c) ++pass; else { ++fail; std::cout << "FAIL " << msg << "\n"; } } while (0)

struct Fx { std::string m; std::vector<dintel::Candle> prev, cur; std::vector<std::tuple<char, int, int, int, long long, long long, unsigned, int> > exp; };

static bool load(const std::string& path, Fx& f) {
    std::ifstream in(path.c_str()); if (!in) return false;
    std::string tag; int nc = 0, np = 0; in >> tag >> f.m >> nc >> np;
    std::string line; std::getline(in, line);
    int read = 0;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream s(line); s >> tag;
        if (tag == "c") {
            dintel::Candle c; int n = 0; s >> c.t >> c.mod >> c.O >> c.H >> c.L >> c.C >> c.vol >> n;
            for (int i = 0; i < n; ++i) { dintel::Cell x; s >> x.key >> x.buy >> x.sell; c.cells.push_back(x); }
            if (read < np) f.prev.push_back(c); else f.cur.push_back(c);
            ++read;
        } else if (tag == "e") {
            char L; int k, pk, side, right; long long lo, hi; unsigned bits; s >> L >> k >> pk >> side >> lo >> hi >> bits >> right;
            f.exp.push_back(std::make_tuple(L, k, pk, side, lo, hi, bits, right));
        }
    }
    return static_cast<int>(f.cur.size()) == nc;
}

static std::vector<dintel::Event> run(const Fx& f, int upTo = -1) {
    dintel::Engine E(f.m);
    const dintel::Rth r = dintel::rthOf(f.m);
    bool set = false;
    for (size_t i = 0; i < f.cur.size(); ++i) {
        if (upTo >= 0 && static_cast<int>(i) > upTo) break;
        const dintel::Candle& c = f.cur[i];
        if (!set && c.mod >= r.open && c.mod < r.close) {   // levels fixed at the first RTH candle (overnight and London are complete)
            std::vector<dintel::Candle> before(f.cur.begin(), f.cur.begin() + static_cast<long>(i));
            E.setLevels(dintel::keyLevels(before, f.prev.empty() ? nullptr : &f.prev, r)); set = true;
        }
        E.add(c);
    }
    return E.ev;
}

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "tests_dintel/fx";
    DIR* d = opendir(dir.c_str());
    CHECK(d != nullptr, "fixture dir " << dir);
    std::vector<std::string> files;
    if (d) { while (dirent* e = readdir(d)) { std::string n = e->d_name; if (n.size() > 3 && n.substr(n.size() - 3) == ".fx") files.push_back(dir + "/" + n); } closedir(d); }
    std::sort(files.begin(), files.end());
    CHECK(!files.empty(), "fixtures found");
    long long total = 0, matched = 0; std::map<std::string, std::pair<long long, long long> > perM;
    for (const std::string& p : files) {
        Fx f; const bool ok = load(p, f);
        CHECK(ok, "load " << p);
        if (!ok) continue;
        const std::vector<dintel::Event> ev = run(f);
        std::multiset<std::tuple<char, int, int, int, long long, long long, unsigned, int> > got, want(f.exp.begin(), f.exp.end());
        for (const dintel::Event& e : ev) if (e.right >= 0) got.insert(std::make_tuple(e.letter, e.k, e.pk, e.side, e.lo, e.hi, e.bits(), e.right));
        long long same = 0; for (const auto& t : want) if (got.count(t)) ++same;
        total += static_cast<long long>(want.size()); matched += same;
        perM[f.m].first += static_cast<long long>(want.size()); perM[f.m].second += same;
        CHECK(same == static_cast<long long>(want.size()) && got.size() == want.size(), "parity " << p << ": python " << want.size() << ", c++ " << got.size() << ", identical " << same);
        if (got.size() != want.size() || same != static_cast<long long>(want.size())) {
            int shown = 0;
            for (const auto& t : want) if (!got.count(t) && shown++ < 4) std::cout << "  python only: " << std::get<0>(t) << " k" << std::get<1>(t) << " pk" << std::get<2>(t) << " side" << std::get<3>(t) << " " << std::get<4>(t) << "-" << std::get<5>(t) << " bits" << std::get<6>(t) << " right" << std::get<7>(t) << "\n";
            shown = 0;
            for (const auto& t : got) if (!want.count(t) && shown++ < 4) std::cout << "  c++ only:    " << std::get<0>(t) << " k" << std::get<1>(t) << " pk" << std::get<2>(t) << " side" << std::get<3>(t) << " " << std::get<4>(t) << "-" << std::get<5>(t) << " bits" << std::get<6>(t) << " right" << std::get<7>(t) << "\n";
        }
        // 2. no repaint: a replay that stops early decides the same letters (same candle, node, context, status) for every candle it saw
        for (int stop = 60; stop < static_cast<int>(f.cur.size()); stop += 25) {
            const std::vector<dintel::Event> part = run(f, stop);
            bool same2 = true;
            for (const dintel::Event& e : part) {
                bool found = false;
                for (const dintel::Event& g : ev) if (g.letter == e.letter && g.k == e.k && g.lo == e.lo && g.hi == e.hi && g.side == e.side && g.bits() == e.bits() && g.shown == e.shown && g.status == e.status) found = true;
                if (!found) same2 = false;
            }
            CHECK(same2, "no repaint " << p << " stop " << stop);
        }
        // 3. gate
        for (const dintel::Event& e : ev) {
            const dctx_params::Row* g = dintel::gateOf(f.m, e.letter);
            auto holds = [&](int id) { return id == 0 || (id > 0 && id < dintel::N_COND && e.ctx[id]); };
            const bool want2 = g && g->status > 0 && holds(g->c1) && holds(g->c2);
            CHECK(e.shown == want2, "gate " << p << " " << e.letter << " k" << e.k);
            if (g && g->status == 0) CHECK(!e.shown, "LOG letter never shown " << e.letter);
        }
    }
    for (const auto& kv : perM) std::cout << "parity " << kv.first << ": " << kv.second.second << " / " << kv.second.first << " letters identical\n";
    std::cout << "parity all: " << matched << " / " << total << "\n";
    // 4. record line + code
    dintel::Event e; e.letter = 'I'; e.side = 1; e.lo = 30933; e.hi = 30934; e.ratio = 2.4; e.status = 1; e.shown = true; e.ctx[dintel::AT_PIVOT] = true; e.conf = "k";
    const std::string ln = dintel::line(e, "2.33.0", "ES", 3984000000LL, 3983999820LL, 0.25);
    CHECK(ln == "DI|2.33.0|ES|I|bullish|3984000000|3983999820|30933|30934|0.25|2.400|32|1|1|k", "record line " << ln);
    CHECK(dintel::code(e) == "I?", "code ?");
    e.status = 2; CHECK(dintel::code(e) == "I", "code SHOW");
    CHECK(std::string(dintel::COND_NAME[dintel::AT_PIVOT]) == "at_pivot" && std::string(dintel::COND_NAME[dintel::OPEN_HR]) == "open_hr", "cond names = lra.dp_intel CONDS order");
    std::cout << pass << " passed, " << fail << " failed\n";
    return fail ? 1 : 0;
}
