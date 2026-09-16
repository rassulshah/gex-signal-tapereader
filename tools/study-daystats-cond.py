#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
study-daystats-cond.py — THE DAY STATS E ROW: is a CONDITIONAL expectation better than the weekday base?

Operator, 2026-09-16 evening: "I am curious about the day stats, how can that model be improved?" → recommendation:
replace the weekday means on the E row (1ST clock, 2ND clock, Took, HL Gap, 1ST = LOD/HOD) with expectations that are
conditional on the first 30 / 60 minutes of the day → "I'll go with your recommendation, do what you need to do but
make sure you test and confirm that the model is better than base."

BASE (what lsDayStats prints today, `hodlodBaseFor(dow)`): the weekday's Tukey-trimmed mean of the first-extreme clock
and the second-extreme clock (Took = first − open, HL Gap = second − first) and the weekday's majority for 1ST = LOD.

CANDIDATES, by stage (the same three stages as the candle):
  pre-open   pooled MEDIANS instead of weekday trimmed means; 1ST = the pooled majority
  30 min     features known at 09:00 CT — where the open sits in the opening range (pos30), the opening drive, the
             clock of the OR's high and low (tH30 / tL30), the READ's own input (has the running extreme been left
             behind: posr30 = distance from the running extreme / OR range)
  60 min     the same on the initial balance
  Each target: 1ST clock (min after open), 2ND clock, 1ST = LOD (accuracy). Out-of-fold, 10 chronological folds,
  the base re-derived inside each fold so it is scored honestly too.
"""
import collections, datetime as dt, importlib.util, io, json, os, statistics as st

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('sh', os.path.join(HERE, 'study-hodlod.py'))
sh = importlib.util.module_from_spec(spec); spec.loader.exec_module(sh)


def sessions():
    raw, prov = sh.load_sources(sh.market_sources('ES'))
    ses = sh.complete(raw)
    rows = []
    for d in sorted(ses):
        b = sh.tool_bars(ses[d])
        hi = max(b, key=lambda r: r[1]); lo = min(b, key=lambda r: r[2])
        O = b[0][4]
        r = dict(day=d, wd=dt.date.fromisoformat(d).weekday(), open=O, hi=hi[1], lo=lo[2],
                 tH=(hi[0] - sh.RTH_A) / 60.0, tL=(lo[0] - sh.RTH_A) / 60.0)
        r['first'] = 'LOD' if lo[0] < hi[0] else 'HOD'
        r['t1'] = min(r['tH'], r['tL']); r['t2'] = max(r['tH'], r['tL'])
        for W in (30, 60):
            o = [x for x in b if x[0] <= sh.RTH_A + W * 60]
            h = max(o, key=lambda x: x[1]); l = min(o, key=lambda x: x[2])
            rng = h[1] - l[2]
            r['pos%d' % W] = (O - l[2]) / rng if rng > 0 else 0.5
            r['tH%d' % W] = (h[0] - sh.RTH_A) / 60.0; r['tL%d' % W] = (l[0] - sh.RTH_A) / 60.0
            r['drive%d' % W] = 1 if o[-1][3] > O else (-1 if o[-1][3] < O else 0)
            c = o[-1][3]
            # how far the close of the window sits from each running extreme, as a share of the window range
            r['offL%d' % W] = (c - l[2]) / rng if rng > 0 else 0.5     # 1 = at the high, far from the low
            r['offH%d' % W] = (h[1] - c) / rng if rng > 0 else 0.5
        rows.append(r)
    return rows


def tmean(xs):
    kept, _ = sh.tukey_keep(list(xs))
    return sum(kept) / len(kept) if kept else st.mean(xs)


def folds(rows, k=10):
    n = len(rows)
    for f in range(k):
        test = rows[f * n // k:(f + 1) * n // k]; ts = set(id(x) for x in test)
        yield [x for x in rows if id(x) not in ts], test


def bucket(v, edges):
    for i, e in enumerate(edges):
        if v < e:
            return i
    return len(edges)


def cond_median(train, key, feat_fn):
    """median of `key` by the bucket feat_fn gives; falls back to the pooled median for an empty bucket"""
    by = collections.defaultdict(list)
    for x in train:
        by[feat_fn(x)].append(x[key])
    pooled = st.median(x[key] for x in train)
    return lambda x: st.median(by[feat_fn(x)]) if len(by[feat_fn(x)]) >= 8 else pooled


def cond_majority(train, feat_fn):
    by = collections.defaultdict(list)
    for x in train:
        by[feat_fn(x)].append(1 if x['first'] == 'LOD' else 0)
    pooled = st.mean(1 if x['first'] == 'LOD' else 0 for x in train)
    def p(x):
        v = by[feat_fn(x)]
        return st.mean(v) if len(v) >= 8 else pooled
    return p


def main():
    rows = sessions()
    n = len(rows)
    print('n=%d sessions %s → %s' % (n, rows[0]['day'], rows[-1]['day']))
    print('first extreme: median %.0f min, mean %.0f; second: median %.0f, mean %.0f; LOD first %.0f%%' % (
        st.median(r['t1'] for r in rows), st.mean(r['t1'] for r in rows), st.median(r['t2'] for r in rows),
        st.mean(r['t2'] for r in rows), 100 * sum(1 for r in rows if r['first'] == 'LOD') / n))

    # ---- the models: each returns (pred_t1, pred_t2, pred_lod_prob) callables fitted on train
    T3 = [1 / 3.0, 2 / 3.0]

    def m_base(train):                       # the panel today: weekday trimmed means + weekday majority
        wd = collections.defaultdict(list)
        for x in train: wd[x['wd']].append(x)
        def t1(x): return tmean([q['t1'] for q in wd[x['wd']]])
        def t2(x): return tmean([q['t2'] for q in wd[x['wd']]])
        def p(x): return st.mean(1 if q['first'] == 'LOD' else 0 for q in wd[x['wd']])
        return t1, t2, p

    def m_pooled_median(train):              # stage 0 candidate
        a = st.median(x['t1'] for x in train); b = st.median(x['t2'] for x in train)
        pl = st.mean(1 if x['first'] == 'LOD' else 0 for x in train)
        return (lambda x: a), (lambda x: b), (lambda x: pl)

    def m_pos(W):                            # conditional on where the open sits in the window (terciles)
        def fit(train):
            fb = lambda x: bucket(x['pos%d' % W], T3)
            return cond_median(train, 't1', fb), cond_median(train, 't2', fb), cond_majority(train, fb)
        return fit

    def m_posdrive(W):                       # pos tercile × drive sign
        def fit(train):
            fb = lambda x: (bucket(x['pos%d' % W], T3), x['drive%d' % W])
            return cond_median(train, 't1', fb), cond_majority(train, fb) and cond_median(train, 't2', fb), cond_majority(train, fb)
        return fit

    def m_orclock(W):                        # 1ST clock = the clock of the OR extreme on the open's side when the open is in an outer
        def fit(train):                      # third (that extreme is probably the day's first); otherwise the conditional median.
            fb = lambda x: bucket(x['pos%d' % W], T3)
            base1 = cond_median(train, 't1', fb); base2 = cond_median(train, 't2', fb); pl = cond_majority(train, fb)
            def t1(x):
                b = fb(x)
                if b == 0: return x['tL%d' % W]      # open at the bottom of the OR → the OR low is the first extreme
                if b == 2: return x['tH%d' % W]
                return base1(x)
            return t1, base2, pl
        return fit

    def m_off(W):                            # the READ's input: has the window's close left its running extreme behind?
        def fit(train):                      # bucket by (which extreme is farther from the close, how far) — terciles of offL/offH
            def fb(x):
                far = 'L' if x['offL%d' % W] >= x['offH%d' % W] else 'H'
                return (far, bucket(max(x['offL%d' % W], x['offH%d' % W]), [0.6, 0.85]))
            base1 = cond_median(train, 't1', fb)
            def t1(x):
                far, bk = fb(x)
                if bk == 2:                     # the close is far from one extreme (≥85% of the window): that extreme is the first
                    return x['tL%d' % W] if far == 'L' else x['tH%d' % W]
                return base1(x)
            return t1, cond_median(train, 't2', fb), cond_majority(train, fb)
        return fit

    models = [
        ('BASE: weekday trimmed mean / majority (the panel today)', m_base, None),
        ('pre-open: pooled median / majority', m_pooled_median, None),
        ('30 min: conditional on pos30 tercile', m_pos(30), 30),
        ('30 min: pos30 tercile x drive', m_posdrive(30), 30),
        ('30 min: OR-extreme clock when open in outer third', m_orclock(30), 30),
        ('30 min: READ input (close far from a running extreme)', m_off(30), 30),
        ('60 min: conditional on pos60 tercile', m_pos(60), 60),
        ('60 min: pos60 tercile x drive', m_posdrive(60), 60),
        ('60 min: OR-extreme clock when open in outer third', m_orclock(60), 60),
        ('60 min: READ input (close far from a running extreme)', m_off(60), 60),
    ]
    print('\n%-58s %8s %8s %8s %8s %8s' % ('model (out-of-fold, 10 folds)', '1ST MAE', '2ND MAE', 'Took MAE', 'Gap MAE', '1ST acc'))
    out = {}
    for name, fit, W in models:
        e1 = []; e2 = []; eg = []; acc = []
        for train, test in folds(rows):
            t1, t2, p = fit(train)
            for x in test:
                a, b = t1(x), t2(x)
                if b < a: a, b = b, a
                e1.append(abs(a - x['t1'])); e2.append(abs(b - x['t2'])); eg.append(abs((b - a) - (x['t2'] - x['t1'])))
                acc.append(1 if ((p(x) >= 0.5) == (x['first'] == 'LOD')) else 0)
        out[name] = dict(t1=round(st.mean(e1), 1), t2=round(st.mean(e2), 1), gap=round(st.mean(eg), 1), acc=round(st.mean(acc), 3))
        print('%-58s %8.1f %8.1f %8.1f %8.1f %8.3f' % (name, st.mean(e1), st.mean(e2), st.mean(e1), st.mean(eg), st.mean(acc)))

    # the conditional tables themselves (fit on all), for the panel / the doc
    print('\n== the tables (fit on all %d) ==' % n)
    for W in (30, 60):
        print('pos%d tercile: n | 1ST median | 2ND median | LOD-first %%' % W)
        for b in range(3):
            sub = [x for x in rows if bucket(x['pos%d' % W], T3) == b]
            print('  %s  n=%3d  %5.0f  %5.0f  %3.0f%%' % (['bottom', 'middle', 'top'][b], len(sub), st.median(x['t1'] for x in sub),
                                                        st.median(x['t2'] for x in sub), 100 * sum(1 for x in sub if x['first'] == 'LOD') / len(sub)))
        print('READ input at %d: far extreme / how far: n | 1ST median | 2ND median | LOD-first %% | share where that extreme IS the first' % W)
        for far in ('L', 'H'):
            for bk in range(3):
                sub = [x for x in rows if (('L' if x['offL%d' % W] >= x['offH%d' % W] else 'H') == far and bucket(max(x['offL%d' % W], x['offH%d' % W]), [0.6, 0.85]) == bk)]
                if not sub: continue
                isfirst = sum(1 for x in sub if (x['first'] == 'LOD') == (far == 'L')) / len(sub)
                print('  %s %-8s n=%3d  %5.0f  %5.0f  %3.0f%%  %3.0f%%' % (far, ['<60%', '60-85%', '>=85%'][bk], len(sub), st.median(x['t1'] for x in sub),
                                                                  st.median(x['t2'] for x in sub), 100 * sum(1 for x in sub if x['first'] == 'LOD') / len(sub), 100 * isfirst))
    json.dump(dict(n=n, first=rows[0]['day'], last=rows[-1]['day'], models=out),
              io.open(os.path.join(HERE, '..', 'data', 'es-1min', 'DAYSTATS-COND-STUDY.json'), 'w', encoding='utf-8'), indent=1)


if __name__ == '__main__':
    main()
