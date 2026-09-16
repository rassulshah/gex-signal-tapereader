#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
study-em-range.py — IS THE OPTIONS MARKET'S EXPECTED MOVE A BETTER RANGE PREDICTOR THAN THE DAY MODEL'S STAGES?

Operator, 2026-09-16: "what if you used the expected move to calculate them [the expected high / low]. is its result
better or worse" → "run the study. I want to know how good EM is and if it will improve the model expected high and low".

The day model (v16.18, tools/study-hodlod.py) predicts the RTH range in three stages: EXANTE from the prior day's range,
OPEN30 from the opening 30-min range, OPEN60 from the initial balance, and draws the candle symmetric round the open.
This study adds the expected move as a candidate feature at every stage and scores everything OUT-OF-FOLD on the same
sessions, the same tool grid and the same MAE, so the numbers are comparable to the ones in the panel's comments.

THE EM SERIES. The straddle EM the panel pins (`feat.emband.em`) exists on ~10 days — nothing to fit. The proxy is
CBOE's VIX1D (the 1-day implied vol index) as a daily series (data/vix1d-daily.txt, fetched through the operator's
browser): EM_pts = ES_open × VIX1D/100 / sqrt(252). Two readings: VIX1D's PRIOR CLOSE (what is known before the open)
and VIX1D's OPEN of the session day (the first print of the day). Only proportionality matters — every model fits
a + b·x — so the sqrt(252) is cosmetic. The proxy is validated against the recorded straddle EMs where both exist.

SCORING. 10 contiguous chronological folds (fit on 9, score the 10th), MAE of the predicted RANGE, and — because the
question is about the candle — MAE of the predicted HIGH (open + range/2) vs the actual HOD and of the LOW vs the LOD.
"""
import importlib.util, io, json, math, os, statistics as st, sys

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('sh', os.path.join(HERE, 'study-hodlod.py'))
sh = importlib.util.module_from_spec(spec); spec.loader.exec_module(sh)


def load_vix1d(path):
    out = {}
    with io.open(path, encoding='utf-8') as f:
        for line in f:
            if line.startswith('#') or not line.strip():
                continue
            for tok in line.strip().split(';'):
                d, o, c = tok.split()
                out['20%s-%s-%s' % (d[0:2], d[2:4], d[4:6])] = (int(o) / 100.0, int(c) / 100.0)
    return out


def sessions(market='ES'):
    srcs = sh.market_sources(market)
    raw, prov = sh.load_sources(srcs)
    ses = sh.complete(raw)
    rows = []
    for d in sorted(ses):
        b = sh.tool_bars(ses[d])
        hi = max(x[1] for x in b); lo = min(x[2] for x in b)
        o30 = [r for r in b if r[0] <= sh.RTH_A + 30 * 60]
        o60 = [r for r in b if r[0] <= sh.RTH_A + 60 * 60]
        h30 = max(x[1] for x in o30); l30 = min(x[2] for x in o30)
        h60 = max(x[1] for x in o60); l60 = min(x[2] for x in o60)
        rows.append(dict(day=d, open=b[0][4], hi=hi, lo=lo, close=b[-1][3], rng=hi - lo,
                         pos30=(b[0][4] - l30) / (h30 - l30) if h30 > l30 else 0.5,   # where the open sits in the opening range (0 = at its low)
                         pos60=(b[0][4] - l60) / (h60 - l60) if h60 > l60 else 0.5,
                         up=(hi - b[0][4]),                                          # the day's upside share, for the placement model
                         or30=max(x[1] for x in o30) - min(x[2] for x in o30),
                         or60=max(x[1] for x in o60) - min(x[2] for x in o60), src=prov[d]))
    return rows


def ols(X, y):
    """Multi-feature OLS with intercept, closed form via normal equations (Gauss-Jordan). X: list of feature lists."""
    n = len(y); k = len(X[0]) + 1
    A = [[1.0] + list(map(float, x)) for x in X]
    # normal equations
    M = [[sum(A[i][r] * A[i][c] for i in range(n)) for c in range(k)] for r in range(k)]
    v = [sum(A[i][r] * y[i] for i in range(n)) for r in range(k)]
    # solve M beta = v
    aug = [M[r] + [v[r]] for r in range(k)]
    for c in range(k):
        p = max(range(c, k), key=lambda r: abs(aug[r][c]))
        aug[c], aug[p] = aug[p], aug[c]
        if abs(aug[c][c]) < 1e-12:
            return None
        for r in range(k):
            if r != c:
                f = aug[r][c] / aug[c][c]
                aug[r] = [a - f * b for a, b in zip(aug[r], aug[c])]
    return [aug[r][k] / aug[r][r] for r in range(k)]


def predict(beta, x):
    return beta[0] + sum(b * xi for b, xi in zip(beta[1:], x))


def oof(rows, feats, folds=10):
    """Out-of-fold MAE for rng ~ a + b·feats, plus MAE of the symmetric candle's high and low."""
    idx = [i for i, r in enumerate(rows) if all(r.get(f) is not None for f in feats)]
    n = len(idx)
    if n < 40:
        return None
    err_r, err_h, err_l, inside = [], [], [], 0
    for f in range(folds):
        test = idx[f * n // folds:(f + 1) * n // folds]
        train = [i for i in idx if i not in set(test)]
        beta = ols([[rows[i][ft] for ft in feats] for i in train], [rows[i]['rng'] for i in train])
        if beta is None:
            return None
        for i in test:
            r = rows[i]; p = predict(beta, [r[ft] for ft in feats])
            err_r.append(abs(p - r['rng']))
            eh, el = r['open'] + p / 2, r['open'] - p / 2
            err_h.append(abs(eh - r['hi'])); err_l.append(abs(el - r['lo']))
            inside += 1 if (r['hi'] <= eh and r['lo'] >= el) else 0
    beta_all = ols([[rows[i][ft] for ft in feats] for i in idx], [rows[i]['rng'] for i in idx])
    return dict(n=n, mae=round(st.mean(err_r), 2), med=round(st.median(err_r), 2),
                mae_hi=round(st.mean(err_h), 2), mae_lo=round(st.mean(err_l), 2),
                inside=round(inside / n, 3), beta=[round(b, 3) for b in beta_all])


def main():
    rows = sessions('ES')
    vix = load_vix1d(os.path.join(HERE, '..', 'data', 'vix1d-daily.txt'))
    vdays = sorted(vix)
    K = 1 / math.sqrt(252)
    for i, r in enumerate(rows):
        r['pdr'] = rows[i - 1]['rng'] if i > 0 else None                      # prior-day range (EXANTE today)
        # VIX1D prior close: the last VIX1D day strictly before this session
        prev = [d for d in vdays if d < r['day']]
        r['em_c'] = r['open'] * vix[prev[-1]][1] / 100 * K if prev else None
        r['em_o'] = r['open'] * vix[r['day']][0] / 100 * K if r['day'] in vix else None
        r['em_c_v'] = vix[prev[-1]][1] if prev else None
    have = [r for r in rows if r['em_c'] is not None and r['pdr'] is not None]
    print('sessions %d (%s → %s); with VIX1D prior close %d; with VIX1D open %d' % (
        len(rows), rows[0]['day'], rows[-1]['day'], len(have), sum(1 for r in rows if r['em_o'] is not None)))
    print('range: mean %.1f  median %.1f  sd %.1f' % (st.mean(r['rng'] for r in rows), st.median(r['rng'] for r in rows),
                                                      st.pstdev(r['rng'] for r in rows)))

    # 1. proxy validation against the recorded straddle EMs (SPX points at the pin; compare as ratios)
    rec = {}
    import glob
    for f in sorted(glob.glob(os.path.join(HERE, '..', 'data', '2026-*.json'))):
        try:
            d = json.load(io.open(f, encoding='utf-8'))
        except Exception:
            continue
        for sym in ('SPY', 'QQQ'):
            for s in (d.get('snaps') or {}).get(sym, []):
                b = (s.get('feat') or {}).get('emband') or {}
                # a CLEAN pin only: pinned in the first tenth of the session (or the recorder's unknown -1), not an
                # estimate, and above the panel's own plausibility floor — 08-26 / 08-31 / 09-02 carry a decayed or
                # expired straddle (2.45 / 3.49 / 9.66) that the panel itself refuses on later days
                el = b.get('elapsed')
                if (b.get('ok') and b.get('em') and b.get('k') and b['k'] > 5000 and sym == 'SPY'
                        and not b.get('est') and b['em'] > 7 and (el is None or el < 0 or el <= 0.2)):
                    rec[d['date']] = (b['em'], b['k'])
                    break
    print('\n== proxy vs the recorded 0DTE straddle EM (SPX points, the panel pin; ES ≈ SPX×ratio) ==')
    print('%-12s %8s %8s %8s %8s' % ('day', 'straddle', 'vix1dC', 'vix1dO', 'ES rng'))
    pairs = []
    for d, (em, k) in sorted(rec.items()):
        r = next((x for x in rows if x['day'] == d), None)
        ec = r['em_c'] if r else None; eo = r['em_o'] if r else None
        print('%-12s %8.2f %8s %8s %8s' % (d, em, '%.1f' % ec if ec else '-', '%.1f' % eo if eo else '-',
                                           '%.1f' % r['rng'] if r else '-'))
        if ec:
            pairs.append((ec, em))
    pairs = []
    for d, (em, k) in sorted(rec.items()):
        r = next((x for x in rows if x['day'] == d), None)
        if r and r['em_o']:
            pairs.append((r['em_o'], em))
    if len(pairs) >= 4:
        xs = [p[0] for p in pairs]; ys = [p[1] for p in pairs]
        print('proxy (VIX1D open) / straddle ratio: mean %.2f  min %.2f  max %.2f' % (
            st.mean(x / y for x, y in pairs), min(x / y for x, y in pairs), max(x / y for x, y in pairs)))
        mx, my = st.mean(xs), st.mean(ys)
        cov = sum((x - mx) * (y - my) for x, y in pairs); sx = math.sqrt(sum((x - mx) ** 2 for x in xs)); sy = math.sqrt(sum((y - my) ** 2 for y in ys))
        print('corr(proxy VIX1D-open, straddle) = %.2f over n=%d clean pins' % (cov / (sx * sy) if sx and sy else float('nan'), len(pairs)))

    # 2. the models, out-of-fold
    print('\n== RANGE MODELS — out-of-fold (10 chronological folds), n=%d ES sessions, MAE in ES points ==' % len(have))
    print('%-34s %4s %6s %6s %7s %7s %7s  %s' % ('model', 'n', 'MAE', 'med', 'MAE-HI', 'MAE-LO', 'inside', 'fit (all)'))
    models = [
        ('constant (mean range)', []),
        ('EXANTE: prior-day range', ['pdr']),
        ('EM (VIX1D prior close)', ['em_c']),
        ('EM (VIX1D open)', ['em_o']),
        ('EM prior close + prior-day range', ['em_c', 'pdr']),
        ('EM open + prior-day range', ['em_o', 'pdr']),
        ('OPEN30: opening 30-min range', ['or30']),
        ('OPEN30 + EM prior close', ['or30', 'em_c']),
        ('OPEN30 + EM open', ['or30', 'em_o']),
        ('OPEN60: initial balance', ['or60']),
        ('OPEN60 + EM prior close', ['or60', 'em_c']),
        ('OPEN60 + EM open', ['or60', 'em_o']),
    ]
    out = {}
    for name, feats in models:
        if not feats:
            # constant: predict the training mean
            idx = have; n = len(idx); e = []; eh = []; el = []; ins = 0
            for f in range(10):
                test = idx[f * n // 10:(f + 1) * n // 10]; ts = set(id(x) for x in test)
                m = st.mean(x['rng'] for x in idx if id(x) not in ts)
                for r in test:
                    e.append(abs(m - r['rng'])); eh.append(abs(r['open'] + m / 2 - r['hi'])); el.append(abs(r['open'] - m / 2 - r['lo']))
                    ins += 1 if (r['hi'] <= r['open'] + m / 2 and r['lo'] >= r['open'] - m / 2) else 0
            res = dict(n=n, mae=round(st.mean(e), 2), med=round(st.median(e), 2), mae_hi=round(st.mean(eh), 2),
                       mae_lo=round(st.mean(el), 2), inside=round(ins / n, 3), beta=[round(st.mean(x['rng'] for x in idx), 1)])
        else:
            res = oof(have, feats)
        out[name] = res
        if res:
            print('%-34s %4d %6.2f %6.2f %7.2f %7.2f %7.3f  %s' % (name, res['n'], res['mae'], res['med'], res['mae_hi'],
                                                                 res['mae_lo'], res['inside'], res['beta']))

    # 2b. the ceiling: a PERFECT range, still placed symmetric round the open — how much error is the placement, not the range
    eh = [abs(r['open'] + r['rng'] / 2 - r['hi']) for r in have]; el = [abs(r['open'] - r['rng'] / 2 - r['lo']) for r in have]
    print('%-34s %4d %6.2f %6s %7.2f %7.2f %7s  %s' % ('ORACLE: perfect range, symmetric', len(have), 0.0, '-', st.mean(eh), st.mean(el), '-',
                                                     'the placement error alone (open in the middle)'))

    # 2c. PLACEMENT — the lever the range cannot reach. Predict the UPSIDE share f = (hi-open)/rng from where the open sits
    # in the opening range (pos30 / pos60), then draw eHi = open + f·p, eLo = open - (1-f)·p with p from the best range model.
    def oof_place(rng_feats, pos_feat):
        idx = have; n = len(idx); eh = []; el = []
        for f in range(10):
            test = idx[f * n // 10:(f + 1) * n // 10]; ts = set(id(x) for x in test)
            train = [x for x in idx if id(x) not in ts]
            br = ols([[x[ft] for ft in rng_feats] for x in train], [x['rng'] for x in train])
            bf = ols([[x[pos_feat]] for x in train], [x['up'] / x['rng'] for x in train])
            for r in test:
                p = predict(br, [r[ft] for ft in rng_feats]); fr = min(0.95, max(0.05, predict(bf, [r[pos_feat]])))
                eh.append(abs(r['open'] + fr * p - r['hi'])); el.append(abs(r['open'] - (1 - fr) * p - r['lo']))
        bf_all = ols([[x[pos_feat]] for x in idx], [x['up'] / x['rng'] for x in idx])
        return st.mean(eh), st.mean(el), [round(b, 3) for b in bf_all]
    print('\n== PLACEMENT (upside share of the range predicted from where the open sits in the opening range) — out-of-fold ==')
    for label, rf, pf in (('OPEN30 + EM open, placed by pos30', ['or30', 'em_o'], 'pos30'),
                          ('OPEN60 + EM open, placed by pos60', ['or60', 'em_o'], 'pos60')):
        mh, ml, bf = oof_place(rf, pf)
        print('%-34s %4d %6s %6s %7.2f %7.2f %7s  upside share = %s' % (label, len(have), '-', '-', mh, ml, '-', bf))

    # 3. where does EM help — by VIX1D tercile (event days vs quiet days), EXANTE vs EM, out-of-fold errors per day
    print('\n== by VIX1D prior-close tercile (quiet / normal / event days): out-of-fold MAE of the range ==')
    def oof_errs(feats):
        idx = have; n = len(idx); errs = {}
        for f in range(10):
            test = idx[f * n // 10:(f + 1) * n // 10]; ts = set(id(x) for x in test)
            train = [x for x in idx if id(x) not in ts]
            beta = ols([[x[ft] for ft in feats] for x in train], [x['rng'] for x in train])
            for r in test:
                errs[r['day']] = predict(beta, [r[ft] for ft in feats]) - r['rng']
        return errs
    e_pdr, e_em, e_both = oof_errs(['pdr']), oof_errs(['em_c']), oof_errs(['em_c', 'pdr'])
    vs = sorted(r['em_c_v'] for r in have); t1, t2 = vs[len(vs) // 3], vs[2 * len(vs) // 3]
    for label, lo, hi in (('quiet  VIX1D < %.1f' % t1, -1, t1), ('normal %.1f-%.1f' % (t1, t2), t1, t2), ('event  VIX1D >= %.1f' % t2, t2, 1e9)):
        ds = [r['day'] for r in have if lo <= r['em_c_v'] < hi]
        print('%-24s n=%3d  prior-day %6.2f   EM %6.2f   EM+prior-day %6.2f   mean range %6.1f  (bias: prior-day %+5.1f, EM %+5.1f)' % (
            label, len(ds), st.mean(abs(e_pdr[d]) for d in ds), st.mean(abs(e_em[d]) for d in ds), st.mean(abs(e_both[d]) for d in ds),
            st.mean(r['rng'] for r in have if r['day'] in set(ds)),
            st.mean(e_pdr[d] for d in ds), st.mean(e_em[d] for d in ds)))

    # 4. the biggest misses of EXANTE that EM catches, and vice versa
    print('\n== the ten days where EM and the prior-day range disagree most (out-of-fold errors, pts; + = over-predicted) ==')
    diffs = sorted(have, key=lambda r: -abs(abs(e_pdr[r['day']]) - abs(e_em[r['day']])))[:10]
    print('%-12s %7s %7s %7s %8s %8s' % ('day', 'range', 'VIX1D', 'EMpts', 'errPrior', 'errEM'))
    for r in diffs:
        print('%-12s %7.1f %7.2f %7.1f %+8.1f %+8.1f' % (r['day'], r['rng'], r['em_c_v'], r['em_c'], e_pdr[r['day']], e_em[r['day']]))

    json.dump(dict(n=len(have), first=have[0]['day'], last=have[-1]['day'], models=out),
              io.open(os.path.join(HERE, '..', 'data', 'es-1min', 'EM-RANGE-STUDY.json'), 'w', encoding='utf-8'), indent=1)


if __name__ == '__main__':
    main()
