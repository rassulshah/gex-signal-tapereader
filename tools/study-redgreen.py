#!/usr/bin/env python3
"""
WILL TODAY CLOSE GREEN OR RED?  (close vs the RTH open) - operator's enhancement, 2026-08-28.

⚠ THE CONTROL THAT DECIDES WHETHER THIS IS REAL: at any moment, "is price above the open right now"
is already a strong guess at the close. A model that scores 70% while simply reading the current sign
has learned nothing. Every number below is quoted against that SIGN-NOW baseline, not against 50%.
"""
import importlib.util, collections, numpy as np, sys
from sklearn.linear_model import LogisticRegression
from sklearn.model_selection import GroupKFold
from sklearn.metrics import roc_auc_score, brier_score_loss

spec = importlib.util.spec_from_file_location('m', 'tools/model-lodhod.py')
mm = importlib.util.module_from_spec(spec); spec.loader.exec_module(mm)
RTH_A = mm.RTH_A
es = mm.load('data/es-1min/ES TestingData.txt')
days = sorted(es)

rows = []
for d in days:
    b = es[d]; n = len(b); op = b[0][1]; close = b[-1][4]
    y = 1 if close > op else 0
    lo = 1e9; loT = None; hi = -1e9; hiT = None
    closes = [x[4] for x in b]
    for t in range(n):
        sec, o, h, l, c = b[t]
        if l < lo: lo, loT = l, t
        if h > hi: hi, hiT = h, t
        if t < 60 or t % 5: continue
        rng = hi - lo
        if rng <= 0: continue
        ma = sum(closes[max(0, t-150+1):t+1]) / min(t+1, 150)
        lowFirst = loT < hiT
        # posr for the standing FIRST extreme, exactly as the ⓪a table uses it
        posr = (c-lo)/rng if lowFirst else (hi-c)/rng
        rows.append(dict(d=d, y=y, mins=(sec-RTH_A)/60.0,
                         signnow=1 if c > op else 0,
                         fromopen=(c-op)/rng,
                         lowfirst=1 if lowFirst else 0,
                         posr=posr,
                         stood=(sec-b[loT if lowFirst else hiT][0])/60.0,
                         above_ma=1 if c > ma else 0,
                         locrange=(c-lo)/rng))
y = np.array([r['y'] for r in rows]); g = np.array([r['d'] for r in rows])
print('corpus: %d sessions, %d observations' % (len(days), len(rows)))
print('green days: %.0f%%\n' % (100*np.mean([1 if es[d][-1][4] > es[d][0][1] else 0 for d in days])))

def cv(feats):
    X = np.array([[r[f] for f in feats] for r in rows])
    oof = np.zeros(len(y))
    for tr, te in GroupKFold(n_splits=5).split(X, y, g):
        m = LogisticRegression(max_iter=2000).fit(X[tr], y[tr]); oof[te] = m.predict_proba(X[te])[:, 1]
    return oof

sets = [('sign now (the baseline)', ['signnow']),
        ('distance from open', ['fromopen']),
        ('sign + time', ['signnow', 'mins']),
        ('+ which extreme came first', ['signnow', 'mins', 'lowfirst']),
        ('+ posr (the LOD/HOD term)', ['signnow', 'mins', 'lowfirst', 'posr']),
        ('+ 50-SMA', ['signnow', 'mins', 'lowfirst', 'posr', 'above_ma']),
        ('everything', ['signnow', 'mins', 'lowfirst', 'posr', 'above_ma', 'fromopen', 'locrange', 'stood'])]
print('%-30s %8s %8s %9s' % ('features', 'AUC', 'Brier', 'accuracy'))
base = None
for nm, f in sets:
    o = cv(f); a = roc_auc_score(y, o); acc = np.mean((o >= .5) == (y == 1))
    if base is None: base = acc
    print('%-30s %8.4f %8.4f %8.0f%%' % (nm, a, brier_score_loss(y, o), 100*acc))

o = cv(['signnow', 'mins', 'lowfirst', 'posr', 'above_ma'])
print('\nBY TIME OF DAY - is the extra machinery beating the bare sign?')
print('  %-8s %8s %10s %10s' % ('time', 'n', 'sign-now', 'model'))
for hh, mmn in [(9, 30), (10, 0), (10, 30), (11, 0), (12, 0), (13, 0), (14, 0)]:
    tgt = ((hh*3600+mmn*60)-RTH_A)/60.0
    idx = [i for i, r in enumerate(rows) if abs(r['mins']-tgt) < 2.5]
    if len(idx) < 50: continue
    yy = y[idx]
    sn = np.mean([(rows[i]['signnow'] == yy[k]) for k, i in enumerate(idx)])
    md = np.mean([((o[i] >= .5) == (yy[k] == 1)) for k, i in enumerate(idx)])
    print('  %-8s %8d %9.0f%% %9.0f%%' % ('%d:%02d' % (hh, mmn), len(idx), 100*sn, 100*md))
