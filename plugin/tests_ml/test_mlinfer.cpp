// tests for MLInfer.h - parity with lra.ml.toolbox.tree_predict on a real exported HGB model.
// Build + run: tests_ml/run_tests.sh (g++ -std=c++17). Inputs written by tests_ml/make_fixture.py:
//   fixture_model.csv  (LRA-ML-*.csv format), fixture_X.csv (rows of features, 'nan' = missing), fixture_expect.csv (Python p)
#include "../MLInfer.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf("FAIL: %s\n", msg); ++fails; } } while (0)

static std::vector<std::vector<double>> readRows(const std::string& p) {
    std::vector<std::vector<double>> out; std::ifstream f(p); std::string line;
    while (std::getline(f, line)) {
        std::vector<double> r; std::stringstream ss(line); std::string c;
        while (std::getline(ss, c, ',')) r.push_back(c == "nan" ? std::numeric_limits<double>::quiet_NaN() : std::atof(c.c_str()));
        if (!r.empty()) out.push_back(r);
    }
    return out;
}

int main(int argc, char** argv) {
    std::string dir = argc > 1 ? argv[1] : ".";
    mlinfer::Model m;
    CHECK(m.load(dir + "/fixture_model.csv"), "load fixture");
    CHECK(!m.enabled(), "ships OFF: ENABLED,0 -> enabled() false");
    auto X = readRows(dir + "/fixture_X.csv");
    auto E = readRows(dir + "/fixture_expect.csv");
    CHECK(X.size() == E.size() && !X.empty(), "fixture sizes");
    double worst = 0;
    for (size_t i = 0; i < X.size() && i < E.size(); ++i) {
        double p = m.predictTrees(X[i].data(), X[i].size());
        double d = std::fabs(p - E[i][0]);
        if (d > worst) worst = d;
    }
    std::printf("parity: %zu rows, max |C++ - Python| = %.3g\n", X.size(), worst);
    CHECK(worst < 1e-9, "parity with toolbox.tree_predict");
    // served value: offset + Platt applied, finite and inside (0, 1)
    double s = m.predict(X[0].data(), X[0].size());
    CHECK(s > 0 && s < 1, "served probability in (0,1)");
    // robustness: malformed files never crash and never load
    mlinfer::Model b;
    CHECK(!b.parse("VERSION,1\nFEAT,0,a\nNODE,0,0,5,1.0,1,2,0,0,1\n"), "bad feature index rejected");
    CHECK(!b.parse(""), "empty rejected");
    CHECK(!b.load(dir + "/no_such_file.csv"), "missing file -> not loaded");
    CHECK(std::isnan(b.predict(X[0].data(), X[0].size())), "not loaded -> NaN (indicator keeps its own read)");
    // short feature row: missing trailing features are treated as NaN (missing), no out-of-bounds read
    double short1 = m.predictTrees(X[0].data(), 1);
    CHECK(short1 > 0 && short1 < 1, "short row handled");
    std::printf(fails ? "%d FAILED\n" : "ALL PASSED\n", fails);
    return fails ? 1 : 0;
}
