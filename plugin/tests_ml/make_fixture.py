"""writes the MLInfer.h parity fixture: a real HistGradientBoosting model -> toolbox.tree_dump -> serve.irt_csv format,
plus feature rows (with missing values) and the Python tree_predict output. Run by run_tests.sh.

    python make_fixture.py <analytics dir> <out dir>
"""
import os
import sys

import numpy as np
import pandas as pd


def main(analytics, out):
    sys.path.insert(0, os.path.join(analytics, "lra", "ml"))
    import toolbox as T
    from sklearn.ensemble import HistGradientBoostingClassifier
    rng = np.random.default_rng(3)
    n = 1500
    X = rng.normal(size=(n, 5))
    X[rng.random((n, 5)) < .08] = np.nan                       # missing values exercise miss_left
    y = ((np.nan_to_num(X[:, 0]) + .6 * np.nan_to_num(X[:, 1]) - .4 * np.nan_to_num(X[:, 3]) + rng.normal(0, .7, n)) > 0).astype(int)
    g = HistGradientBoostingClassifier(max_iter=40, max_depth=4, max_leaf_nodes=12, learning_rate=.08, random_state=0).fit(X, y)
    Tb = T.tree_dump(g)
    base = float(np.ravel(g._baseline_prediction)[0])
    os.makedirs(out, exist_ok=True)
    ed = os.path.join(out, "export")
    os.makedirs(ed, exist_ok=True)
    Tb.to_csv(os.path.join(ed, "trees.csv"), index=False)
    pd.DataFrame(dict(idx=range(5), feature=[f"f{i}" for i in range(5)])).to_csv(os.path.join(ed, "features.csv"), index=False)
    import json
    json.dump(dict(base_raw=base, platt=[1.1, -0.05], market_offsets={"ES": 0.2}), open(os.path.join(ed, "meta.json"), "w"))
    import serve as SV
    txt = SV.irt_csv(dict(id="FIXTURE", kind="bin"), ed, "ES")
    open(os.path.join(out, "fixture_model.csv"), "w", newline="").write(txt)
    Xt = X[:300]
    p = T.tree_predict(Tb, base, Xt)
    sk = g.predict_proba(Xt)[:, 1]
    assert np.max(np.abs(p - sk)) < 1e-9, "tree_dump/tree_predict vs sklearn"
    with open(os.path.join(out, "fixture_X.csv"), "w") as f:
        for r in Xt:
            f.write(",".join("nan" if v != v else repr(float(v)) for v in r) + "\n")
    with open(os.path.join(out, "fixture_expect.csv"), "w") as f:
        for v in p:
            f.write(repr(float(v)) + "\n")
    print(f"fixture: {len(Tb)} nodes, {Tb.tree.nunique()} trees; python vs sklearn max diff {np.max(np.abs(p - sk)):.2g}")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
