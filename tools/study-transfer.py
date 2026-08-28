#!/usr/bin/env python3
"""
DOES THE LOD/HOD TABLE TRANSFER BETWEEN INSTRUMENTS?

The strongest caveat on FINDINGS F-4 was "one instrument". If the ES table works on NQ as well as
NQ's own table does, the table is measuring session STRUCTURE rather than something about ES - and
one table serves every market instead of one per market.
"""
import importlib.util, collections, numpy as np, sys
from sklearn.metrics import roc_auc_score, brier_score_loss
from sklearn.model_selection import GroupKFold
spec = importlib.util.spec_from_file_location('m', 'tools/model-lodhod.py')
mm = importlib.util.module_from_spec(spec); spec.loader.exec_module(mm)

CELL = lambda r: (min(int(r['posr']*8), 7), min(int(r['mins']//45), 8))

def rows_for(path):
    return mm.build(mm.load(path), None)

def fit(rows):
    t = collections.defaultdict(lambda: [0, 0])
    for r in rows:
        k = CELL(r); t[k][0] += 1; t[k][1] += r['y']
    return t

def score(tab, rows, prior):
    p, y = [], []
    for r in rows:
        n, h = tab.get(CELL(r), [0, 0])
        p.append((h + prior*10)/(n + 10)); y.append(r['y'])
    return np.array(p), np.array(y)

def main(a, b):
    aD, aR = rows_for(a); bD, bR = rows_for(b)
    print('A: %d sessions / %d rows   B: %d sessions / %d rows' % (len(aD), len(aR), len(bD), len(bR)))
    aT, bT = fit(aR), fit(bR)
    aP = np.mean([r['y'] for r in aR]); bP = np.mean([r['y'] for r in bR])
    p, y = score(aT, bR, aP)
    print('  A table -> B data    AUC %.4f  Brier %.4f' % (roc_auc_score(y, p), brier_score_loss(y, p)))
    g = np.array([r['d'] for r in bR]); yy = np.array([r['y'] for r in bR]); oof = np.zeros(len(yy))
    for tr, te in GroupKFold(n_splits=5).split(np.zeros((len(yy), 1)), yy, g):
        t = fit([bR[i] for i in tr]); pp, _ = score(t, [bR[i] for i in te], yy[tr].mean()); oof[te] = pp
    print("  B's OWN table -> B   AUC %.4f  Brier %.4f  (out-of-fold)"
          % (roc_auc_score(yy, oof), brier_score_loss(yy, oof)))
    d = []
    for k in set(aT) | set(bT):
        an, ah = aT.get(k, [0, 0]); bn, bh = bT.get(k, [0, 0])
        if an >= 25 and bn >= 25:
            d.append(100*ah/an - 100*bh/bn)
    d = np.array(d)
    print('  %d comparable cells | mean |gap| %.1f pts | >10pts: %d'
          % (len(d), np.mean(np.abs(d)), int((np.abs(d) > 10).sum())))

if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else 'data/es-1min/ES TestingData.txt',
         sys.argv[2] if len(sys.argv) > 2 else 'data/es-1min/NQ TestingData.txt')
