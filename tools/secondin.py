#!/usr/bin/env python3
"""THE SECOND-EXTREME "IS IT IN" MODEL — fitted nightly from the corpus + every daily session (study-hodlod imports this and
writes the block into BASERATES.json as `secondIn`; the panel's hlSecondRead reads it through the same courier as the base
rates, with a baked fallback and floors).  Operator, 2026-09-17: "give me a good model ... make sure it is self calibrating
and using the data that we get daily so it keeps adapting and is current."

THE QUESTION at a 3-minute bar end: is the running extreme on this side (the day's high so far / low so far) the day's
FINAL one?  y = 1 when no later bar trades beyond it.  Features, all from the tool bars the panel already holds:
  d      = |close - running extreme| / sigma,  sigma = std of the 3-min close-to-close changes so far x sqrt(bars left)
           (the first-passage scale: how many "typical remaining moves" away the extreme is)
  ml     = minutes left in RTH
  age    = minutes since the running extreme printed
  share  = |close - running extreme| / the range so far
Model: logistic regression on [1, d, sqrt(d), ln(ml), age/390, share, d*ln(ml)] (Newton's method, ridge 1e-3).
Timing when NOT in: minutes from the clock to the new extreme, median by d-bin (first-passage; F-15) -> "~HH:MM (50%)".
Validation (every nightly, recorded in the block): out-of-fold BY DAY (10 folds) AUC / Brier / reliability deciles /
hit rates at >=70% and <=30%; and ROLLING: fit on all but the newest 20 sessions, score those 20 - the "is it still
calibrated on recent data" check the panel shows.  The panel refuses a block that fails its floors (see FLOORS).
Measured 2026-09-17 on 284 vendor sessions (1-min sampling): LOGIT AUC 0.890, Brier 0.129, calibrated at every decile,
half of all readings >= 70% and right 90%; the time-only base rate AUC 0.677.
"""
import math, collections
import numpy as np

RTH_A, RTH_B = 8*3600+30*60, 15*3600
DB = [0.0, 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 99.0]
FEATURES = ['1', 'd', 'sqrt(d)', 'ln(minsLeft)', 'age/390', 'share', 'd*ln(minsLeft)']
FLOORS = dict(minDays=150, minSamples=15000, minAuc=0.84, maxCalErr=0.08)

def dbin(x):
    for i in range(len(DB) - 1):
        if DB[i] <= x < DB[i + 1]: return i
    return len(DB) - 2

def feats(d, ml, age, share):
    return [1.0, d, math.sqrt(d), math.log(max(ml, 1.0)), age / 390.0, share, d * math.log(max(ml, 1.0))]

def samples_for_day(day, bars):
    """bars: 3-minute tool bars (end_sec, high, low, close, open) sorted, RTH. -> list of sample dicts (both sides)."""
    rth = [b for b in bars if RTH_A < b[0] <= RTH_B]
    if len(rth) < 120: return []
    out = []; hi = lo = None; hiT = loT = None; closes = []
    for i, (end, h, l, c, o) in enumerate(rth):
        closes.append(c)
        if hi is None or h > hi: hi, hiT = h, end
        if lo is None or l < lo: lo, loT = l, end
        mins = (end - RTH_A) / 60.0
        ml = (RTH_B - end) / 60.0
        if mins < 36 or ml < 6 or i < 6: continue
        rv = float(np.std(np.diff(np.array(closes))))
        if rv <= 0: continue
        sig = rv * math.sqrt(ml / 3.0)
        fut = rth[i + 1:]
        if not fut: continue
        rng = (hi - lo) if hi > lo else 0.25
        for side, ext, extT in (('H', hi, hiT), ('L', lo, loT)):
            later = [x for x in fut if (x[1] > ext if side == 'H' else x[2] < ext)]
            out.append(dict(day=day, side=side, mins=mins, d=abs(c - ext) / sig, ml=ml, age=(end - extT) / 60.0,
                            share=abs(c - ext) / rng, y=0 if later else 1, tto=((later[0][0] - end) / 60.0) if later else None))
    return out

def fit_logit(S, iters=25):
    X = np.array([feats(s['d'], s['ml'], s['age'], s['share']) for s in S]); y = np.array([s['y'] for s in S], dtype=float)
    w = np.zeros(X.shape[1])
    for _ in range(iters):
        p = 1 / (1 + np.exp(-(X @ w))); g = X.T @ (p - y) + 1e-3 * w
        H = (X * (p * (1 - p))[:, None]).T @ X + 1e-3 * np.eye(X.shape[1]); w -= np.linalg.solve(H, g)
    return w

def predict(w, d, ml, age, share):
    z = float(np.dot(w, feats(d, ml, age, share))); return 1 / (1 + math.exp(-z))

def timing_table(S):
    tm = collections.defaultdict(list)
    for s in S:
        if s['y'] == 0 and s['tto'] is not None: tm[dbin(s['d'])].append(s['tto'])
    return {str(k): dict(n=len(v), med=round(float(np.median(v)), 1), q25=round(float(np.quantile(v, .25)), 1), q75=round(float(np.quantile(v, .75)), 1))
            for k, v in sorted(tm.items()) if len(v) >= 30}

def _auc(y, p):
    y = np.asarray(y); p = np.asarray(p); pos = p[y == 1]; neg = p[y == 0]
    if not len(pos) or not len(neg): return float('nan')
    order = np.argsort(np.concatenate([pos, neg])); r = np.empty(len(order)); r[order] = np.arange(1, len(order) + 1)
    return float((r[:len(pos)].sum() - len(pos) * (len(pos) + 1) / 2) / (len(pos) * len(neg)))

def score(y, p):
    y = np.asarray(y, dtype=float); p = np.asarray(p, dtype=float)
    qs = np.quantile(p, np.linspace(0, 1, 11)); cal = []
    for i in range(10):
        sel = (p >= qs[i]) & ((p <= qs[i + 1]) if i == 9 else (p < qs[i + 1]))
        if sel.sum(): cal.append([round(float(p[sel].mean()), 3), round(float(y[sel].mean()), 3), int(sel.sum())])
    hi = p >= 0.70; lo = p <= 0.30
    return dict(n=int(len(y)), auc=round(_auc(y, p), 3), brier=round(float(np.mean((p - y) ** 2)), 4), cal=cal,
                maxCalErr=round(max(abs(a - b) for a, b, _ in cal), 3) if cal else None,
                shareGe70=round(float(hi.mean()), 3), hitGe70=round(float(y[hi].mean()), 3) if hi.sum() else None,
                shareLe30=round(float(lo.mean()), 3), hitLe30=round(float(1 - y[lo].mean()), 3) if lo.sum() else None)

def block(ses, tool_bars):
    """ses: {day: minute bars}; tool_bars: the 3-minute aggregator. -> the BASERATES `secondIn` block (or None)."""
    days = sorted(ses); S = []
    for d in days: S.extend(samples_for_day(d, tool_bars(ses[d])))
    if len(set(s['day'] for s in S)) < 40: return None
    uniq = sorted(set(s['day'] for s in S)); K = 10; folds = [set(uniq[i::K]) for i in range(K)]
    y_all = []; p_all = []; p_time = []; cov = []
    for f in folds:
        tr = [s for s in S if s['day'] not in f]; te = [s for s in S if s['day'] in f]
        w = fit_logit(tr); tt = timing_table(tr)
        tb = collections.defaultdict(lambda: [0, 0])
        for s in tr: k = int(s['ml'] // 45); tb[k][0] += 1; tb[k][1] += s['y']
        for s in te:
            y_all.append(s['y']); p_all.append(predict(w, s['d'], s['ml'], s['age'], s['share']))
            k = int(s['ml'] // 45); p_time.append(tb[k][1] / tb[k][0] if tb[k][0] else 0.5)
            if s['y'] == 0 and s['tto'] is not None and str(dbin(s['d'])) in tt: cov.append(1 if s['tto'] <= tt[str(dbin(s['d']))]['med'] else 0)
    oof = score(y_all, p_all); base = score(y_all, p_time)
    # rolling: the newest 20 sessions scored by a fit on everything before them
    recent = set(uniq[-20:]); tr = [s for s in S if s['day'] not in recent]; te = [s for s in S if s['day'] in recent]
    wr = fit_logit(tr); roll = score([s['y'] for s in te], [predict(wr, s['d'], s['ml'], s['age'], s['share']) for s in te]) if te else None
    w = fit_logit(S)
    return dict(n=len(S), days=len(uniq), first=uniq[0], last=uniq[-1], features=FEATURES, w=[round(float(x), 5) for x in w],
                dbins=DB, timing=timing_table(S), timingCov50=round(float(np.mean(cov)), 3) if cov else None,
                oof=oof, timeOnly=dict(auc=base['auc'], brier=base['brier']), rolling20=dict(days=sorted(recent)[0] + '..' + sorted(recent)[-1], **roll) if roll else None,
                floors=FLOORS, ok=bool(oof['auc'] >= FLOORS['minAuc'] and oof['maxCalErr'] <= FLOORS['maxCalErr'] and len(uniq) >= FLOORS['minDays'] and len(S) >= FLOORS['minSamples']))

if __name__ == '__main__':
    import sys, json
    from importlib.machinery import SourceFileLoader
    hl = SourceFileLoader('hl', 'tools/study-hodlod.py').load_module()
    raw, prov = hl.load_sources(hl.market_sources("ES")); ses = hl.complete(raw)
    B = block(ses, hl.tool_bars)
    print(json.dumps({k: v for k, v in B.items() if k not in ('timing',)}, indent=1)[:3000])
    print('timing', B['timing'])
