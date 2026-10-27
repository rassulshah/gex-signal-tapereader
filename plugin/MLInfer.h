// MLInfer.h 1.0.0 (2026-10-26) - SDK-free inference for LRA ML champions exported by ml_nightly (lra.ml.serve.irt_csv).
//
// Reads lsFlexLevels\LRA-ML-<target>-<MKT>.csv:
//   VERSION,1 | MODEL,<id> | ENABLED,0|1 | KIND,bin|reg | BASE,<raw> | OFFSET,<market logit offset> | PLATT,<a>,<b>
//   FEAT,<idx>,<name> ... | NODE,<tree>,<node>,<feat>,<thr>,<left>,<right>,<leaf>,<value>,<miss_left> ...
// and scores one row of features with the same rules as lra.ml.toolbox.tree_predict (parity-tested in plugin/tests_ml):
//   raw = BASE + sum over trees of the leaf value (x <= thr -> left; NaN -> left when miss_left); bin: p = sigmoid(raw);
//   then the market offset (logit shift) and Platt (a * logit(p) + b) when present.
// Header-only, no IRT SDK, no allocation in predict(). NOT wired into any drawing: a plugin may load it only after
// Rassul approves a mockup, and it must still honour ENABLED,0 (ships OFF; enabled() returns false).
#pragma once

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace mlinfer {

struct Node {
    int feat = 0, left = -1, right = -1;
    double thr = 0.0, value = 0.0;
    bool leaf = true, missLeft = true;
};

struct Model {
    std::string id;
    bool enabledFlag = false, isReg = false, loaded = false;
    double base = 0.0, offset = 0.0, plattA = 1.0, plattB = 0.0;
    std::vector<std::string> feats;            // index -> name
    std::vector<std::vector<Node>> trees;
    std::string error;

    bool enabled() const { return loaded && enabledFlag; }
    int featIndex(const std::string& name) const {
        for (size_t i = 0; i < feats.size(); ++i) if (feats[i] == name) return (int)i;
        return -1;
    }

    static std::vector<std::string> split(const std::string& s) {
        std::vector<std::string> out; std::string cur;
        for (char c : s) { if (c == ',') { out.push_back(cur); cur.clear(); } else if (c != '\r' && c != '\n') cur += c; }
        out.push_back(cur);
        return out;
    }

    // parse the CSV text; returns false (and sets error) on a malformed file - the caller then keeps the indicator's own read
    bool parse(const std::string& text) {
        *this = Model();
        std::istringstream in(text); std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::vector<std::string> f = split(line);
            const std::string& k = f[0];
            if (k == "MODEL" && f.size() >= 2) id = f[1];
            else if (k == "ENABLED" && f.size() >= 2) enabledFlag = (f[1] == "1");
            else if (k == "KIND" && f.size() >= 2) isReg = (f[1] == "reg");
            else if (k == "BASE" && f.size() >= 2) base = std::atof(f[1].c_str());
            else if (k == "OFFSET" && f.size() >= 2) offset = std::atof(f[1].c_str());
            else if (k == "PLATT" && f.size() >= 3) { plattA = std::atof(f[1].c_str()); plattB = std::atof(f[2].c_str()); }
            else if (k == "FEAT" && f.size() >= 3) {
                int i = std::atoi(f[1].c_str());
                if (i < 0 || i > 4096) { error = "bad FEAT index"; return false; }
                if ((int)feats.size() <= i) feats.resize(i + 1);
                feats[i] = f[2];
            } else if (k == "NODE" && f.size() >= 10) {
                int t = std::atoi(f[1].c_str()), n = std::atoi(f[2].c_str());
                if (t < 0 || n < 0 || t > 100000 || n > 100000) { error = "bad NODE index"; return false; }
                if ((int)trees.size() <= t) trees.resize(t + 1);
                if ((int)trees[t].size() <= n) trees[t].resize(n + 1);
                Node& d = trees[t][n];
                d.feat = std::atoi(f[3].c_str()); d.thr = std::atof(f[4].c_str());
                d.left = std::atoi(f[5].c_str()); d.right = std::atoi(f[6].c_str());
                d.leaf = std::atoi(f[7].c_str()) != 0; d.value = std::atof(f[8].c_str()); d.missLeft = std::atoi(f[9].c_str()) != 0;
            }
        }
        // validate: every internal node points inside its tree and at a known feature
        for (const auto& tr : trees)
            for (const auto& d : tr)
                if (!d.leaf && (d.left < 0 || d.right < 0 || d.left >= (int)tr.size() || d.right >= (int)tr.size() ||
                                d.feat < 0 || d.feat >= (int)feats.size())) { error = "bad tree"; return false; }
        if (trees.empty() || feats.empty()) { error = "empty model"; return false; }
        loaded = true;
        return true;
    }

    bool load(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) { error = "missing file"; loaded = false; return false; }
        std::stringstream ss; ss << f.rdbuf();
        return parse(ss.str());
    }

    // raw margin (no link): BASE + sum of leaf values. x has feats.size() values; NaN = missing.
    double raw(const double* x, size_t n) const {
        double out = base;
        for (const auto& tr : trees) {
            int k = 0, guard = 0;
            while (k >= 0 && k < (int)tr.size() && !tr[k].leaf && guard++ < 64) {
                const Node& d = tr[k];
                double v = (d.feat < (int)n) ? x[d.feat] : std::numeric_limits<double>::quiet_NaN();
                bool goLeft = std::isnan(v) ? d.missLeft : (v <= d.thr);
                k = goLeft ? d.left : d.right;
            }
            if (k >= 0 && k < (int)tr.size()) out += tr[k].value;
        }
        return out;
    }

    static double sigmoid(double z) { return 1.0 / (1.0 + std::exp(-z)); }

    // tree output exactly as toolbox.tree_predict (bin: sigmoid(raw); reg: raw)
    double predictTrees(const double* x, size_t n) const {
        double r = raw(x, n);
        return isReg ? r : sigmoid(r);
    }

    // full served value: market offset (logit shift) + Platt, as written by the nightly; NaN when not loaded
    double predict(const double* x, size_t n) const {
        if (!loaded) return std::numeric_limits<double>::quiet_NaN();
        double r = raw(x, n);
        if (isReg) return r;
        double z = r + offset;
        return sigmoid(plattA * z + plattB);
    }
};

}  // namespace mlinfer
