#!/usr/bin/env python3
"""(2026-09-17) "LOD IN X%" FOR THE SECOND EXTREME — operator: "give me a good model ... make sure the model is excellent and
you test it". The question at a clock: IS THE RUNNING EXTREME ON THIS SIDE THE DAY'S FINAL ONE?  (y = 1: no later bar
trades beyond it.)  Sampled every 5 min from 35 min after the open to 14:55, both sides, 301 sessions, out-of-fold BY DAY
(10 folds).  Features at the clock: d = |close - running extreme| / sigma  (sigma = realized 1-min close-to-close vol so
far x sqrt(minutes left) - the first-passage scale, as F-14/F-15), minsLeft, age = minutes since that extreme printed,
share = |close - extreme| / range so far.
Models:  TIME   P(in) by minutes-left bin (the base rate)
         FSTAB  the shipped far-side touch table (fitted on synthetic levels k*sigma off the close), 1 - touch
         REFIT  the same (d x minsLeft) table re-fit on RUNNING EXTREMES (train folds only)
         LOGIT  logistic regression on [d, sqrt(d), log(minsLeft), age/390, share, d*log(minsLeft)]  (Newton, train folds)
Scores: AUC, Brier, calibration by decile (reliability), and the "LOD IN" share at >= 70%.  Timing for the NOT-in case:
minutes until the new extreme, median by d-bin -> coverage of "by now+median (50%)".
Writes data/es-1min/SECOND-IN-STUDY.json.
"""
import json, math, collections, numpy as np
from importlib.machinery import SourceFileLoader
SP = SourceFileLoader('sp2', 'tools/study-secondpred2.py').load_module()
RTH_A, RTH_B = 8*3600+30*60, 15*3600
DB = [0.0, 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 99]
TB = [0, 45, 90, 135, 180, 240, 300, 390]
def b(edges, x):
    for i in range(len(edges) - 1):
        if edges[i] <= x < edges[i + 1]: return i
    return len(edges) - 2

raw = SP.load24('data/es-1min/ES TestingData.txt')
# the recent days the nightly study also folds in
import glob, csv, os
def load_day_csv(p):
    out = []
    with open(p) as f:
        for r in csv.reader(f):
            try:
                t = r[0]; hh, mm = int(t[11:13]), int(t[14:16]); sec = hh*3600 + mm*60
                out.append((sec, float(r[1]), float(r[2]), float(r[3]), float(r[4]), float(r[5]) if len(r) > 5 else 0))
            except Exception: pass
    return out
for p in sorted(glob.glob('data/futures/ES/*.csv')):
    d = os.path.basename(p)[:10]
    if d not in raw:
        bars = load_day_csv(p)
        if len(bars) > 300: raw[d] = bars

S = []   # samples: dict(day, side, now, d, minsLeft, age, share, y, tto)
days = sorted(raw)
for d in days:
    rth = [x for x in raw[d] if RTH_A <= x[0] <= RTH_B]
    if len(rth) < 380: continue
    closes = [x[4] for x in rth]
    hi = lo = None; hiT = loT = None
    for i, (sec, o, h, l, c, v) in enumerate(rth):
        if hi is None or h > hi: hi, hiT = h, sec
        if lo is None or l < lo: lo, loT = l, sec
        mins = (sec - RTH_A) / 60.0
        if mins < 35 or int(mins) % 5: continue
        minsLeft = (RTH_B - sec) / 60.0
        if minsLeft < 5: continue
        rv = float(np.std(np.diff(np.array(closes[:i+1]))))
        if rv <= 0: continue
        sig = rv * math.sqrt(minsLeft)
        fut = rth[i+1:]
        if not fut: continue
        rng = hi - lo if hi > lo else 0.25
        for side, ext, extT in (('H', hi, hiT), ('L', lo, loT)):
            later = [x for x in fut if (x[2] > ext if side == 'H' else x[3] < ext)]
            y = 0 if later else 1
            tto = ((later[0][0] - sec) / 60.0) if later else None
            S.append(dict(day=d, side=side, now=mins, d=abs(c - ext) / sig, minsLeft=minsLeft, age=(sec - extT) / 60.0, share=abs(c - ext) / rng, y=y, tto=tto))
print('samples %d over %d days (%s -> %s)' % (len(S), len(set(s['day'] for s in S)), days[0], days[-1]))

def auc(y, p):
    y = np.asarray(y); p = np.asarray(p); pos = p[y == 1]; neg = p[y == 0]
    if not len(pos) or not len(neg): return float('nan')
    # rank-based
    order = np.argsort(np.concatenate([pos, neg])); ranks = np.empty(len(order)); ranks[order] = np.arange(1, len(order) + 1)
    return (ranks[:len(pos)].sum() - len(pos) * (len(pos) + 1) / 2) / (len(pos) * len(neg))
def brier(y, p): y = np.asarray(y); p = np.asarray(p); return float(np.mean((p - y) ** 2))
def feats(s): dd = s['d']; ml = s['minsLeft']; return [1.0, dd, math.sqrt(dd), math.log(ml), s['age'] / 390.0, s['share'], dd * math.log(ml)]
def logit_fit(X, y, iters=25):
    w = np.zeros(X.shape[1])
    for _ in range(iters):
        z = X @ w; p = 1 / (1 + np.exp(-z)); g = X.T @ (p - y) + 1e-3 * w; Wd = p * (1 - p)
        H = (X * Wd[:, None]).T @ X + 1e-3 * np.eye(X.shape[1]); w -= np.linalg.solve(H, g)
    return w

uniq = sorted(set(s['day'] for s in S)); K = 10
folds = [set(uniq[i::K]) for i in range(K)]
res = {m: dict(y=[], p=[]) for m in ('TIME', 'FSTAB', 'REFIT', 'LOGIT')}
FS = json.load(open('data/es-1min/FARSIDE.json'))
def fs_touch(dd, ml):
    di = min(b(DB, dd), len(FS['touch']) - 1); ti = b(TB, ml)
    cell = FS['touch'][di][ti] if di < len(FS['touch']) and ti < len(FS['touch'][di]) else None
    return (cell[1] / 100.0) if cell and cell[1] is not None else None
tto_by = collections.defaultdict(list)
cov50 = []
for f in folds:
    tr = [s for s in S if s['day'] not in f]; te = [s for s in S if s['day'] in f]
    # TIME
    tb = collections.defaultdict(lambda: [0, 0])
    for s in tr: k = b(TB, s['minsLeft']); tb[k][0] += 1; tb[k][1] += s['y']
    # REFIT
    rb = collections.defaultdict(lambda: [0, 0])
    for s in tr: k = (b(DB, s['d']), b(TB, s['minsLeft'])); rb[k][0] += 1; rb[k][1] += s['y']
    pooled = sum(s['y'] for s in tr) / len(tr)
    # LOGIT
    X = np.array([feats(s) for s in tr]); Y = np.array([s['y'] for s in tr], dtype=float); w = logit_fit(X, Y)
    # timing (not-in): median minutes-to-new-extreme by d bin
    tm = collections.defaultdict(list)
    for s in tr:
        if s['y'] == 0 and s['tto'] is not None: tm[b(DB, s['d'])].append(s['tto'])
    tmed = {k: float(np.median(v)) for k, v in tm.items() if len(v) >= 30}
    for s in te:
        k = b(TB, s['minsLeft']); pt = tb[k][1] / tb[k][0] if tb[k][0] else pooled
        res['TIME']['y'].append(s['y']); res['TIME']['p'].append(pt)
        t = fs_touch(s['d'], s['minsLeft']); res['FSTAB']['y'].append(s['y']); res['FSTAB']['p'].append((1 - t) if t is not None else pt)
        kk = (b(DB, s['d']), b(TB, s['minsLeft'])); pr = rb[kk][1] / rb[kk][0] if rb[kk][0] >= 30 else pt
        res['REFIT']['y'].append(s['y']); res['REFIT']['p'].append(pr)
        pl = 1 / (1 + math.exp(-float(np.array(feats(s)) @ w))); res['LOGIT']['y'].append(s['y']); res['LOGIT']['p'].append(pl)
        if s['y'] == 0 and s['tto'] is not None and b(DB, s['d']) in tmed: cov50.append(1 if s['tto'] <= tmed[b(DB, s['d'])] else 0)
print('\n%-6s %7s %7s   calibration by decile (mean p -> observed in-rate, n)' % ('model', 'AUC', 'Brier'))
out = {}
for m, r in res.items():
    y = np.array(r['y']); p = np.array(r['p'])
    A = auc(y, p); B = brier(y, p)
    qs = np.quantile(p, np.linspace(0, 1, 11)); cal = []
    for i in range(10):
        sel = (p >= qs[i]) & (p <= qs[i + 1]) if i == 9 else (p >= qs[i]) & (p < qs[i + 1])
        if sel.sum(): cal.append((round(float(p[sel].mean()), 2), round(float(y[sel].mean()), 2), int(sel.sum())))
    hi = (p >= 0.70); share70 = float(hi.mean()); hit70 = float(y[hi].mean()) if hi.sum() else float('nan')
    lo = (p <= 0.30); share30 = float(lo.mean()); hit30 = float(1 - y[lo].mean()) if lo.sum() else float('nan')
    out[m] = dict(auc=round(float(A), 3), brier=round(B, 4), cal=cal, share_ge70=round(share70, 3), hit_ge70=round(hit70, 3), share_le30=round(share30, 3), hit_le30=round(hit30, 3))
    print('%-6s %7.3f %7.4f   >=70%%: %4.1f%% of readings, right %4.1f%% | <=30%%: %4.1f%% of readings, right %4.1f%%' % (m, A, B, 100*share70, 100*hit70, 100*share30, 100*hit30))
    print('       cal: ' + '  '.join('%.2f->%.2f(%d)' % c for c in cal))
print('\nTIMING (not in): "by now + median(d-bin) 50%%" covered %.1f%% out-of-fold (n=%d)' % (100 * np.mean(cov50), len(cov50)))
# the LOGIT fit on ALL for the panel to bake, + the timing medians on all
X = np.array([feats(s) for s in S]); Y = np.array([s['y'] for s in S], dtype=float); wall = logit_fit(X, Y)
tm = collections.defaultdict(list)
for s in S:
    if s['y'] == 0 and s['tto'] is not None: tm[b(DB, s['d'])].append(s['tto'])
timing = {str(k): dict(n=len(v), med=round(float(np.median(v)), 1), q25=round(float(np.quantile(v, .25)), 1), q75=round(float(np.quantile(v, .75)), 1)) for k, v in sorted(tm.items())}
json.dump(dict(n=len(S), days=len(uniq), first=days[0], last=days[-1], models=out, timing_cov50=round(float(np.mean(cov50)), 3),
               logit=dict(features=['1', 'd', 'sqrt(d)', 'log(minsLeft)', 'age/390', 'share', 'd*log(minsLeft)'], w=[round(float(x), 5) for x in wall]),
               timing_by_dbin=timing, dbins=DB, tbins=TB), open('data/es-1min/SECOND-IN-STUDY.json', 'w'), indent=1)
print('\nlogit weights (all days):', [round(float(x), 3) for x in wall])
print('timing medians by d-bin:', {k: v['med'] for k, v in timing.items()})
print('wrote data/es-1min/SECOND-IN-STUDY.json')
