#!/usr/bin/env python3
"""
A CALIBRATED PROBABILITY THAT THE LOW / HIGH OF DAY IS IN.

    "i want you to create a predictive probabilistic model so i can use it to identify if a hod or
     lod has occurred."  - operator, 2026-08-28

⚠⚠ THE ONE COMPARISON THAT MATTERS. The chance a standing extreme is the day's rises from ~40% at
09:30 to ~64% by noon FOR FREE. So a model fed the clock will look excellent while knowing nothing.
Every score below is measured against a TIME-ONLY BASELINE trained on the same folds. If the full
model does not beat that, it has learned to read a clock.

⚠ GROUPED CROSS-VALIDATION BY SESSION. Bars inside one session share a label; a random split leaks
it. Folds are split by DATE, so a session is wholly in train or wholly in test.

⚠ THE HONEST n IS SESSION-SIDES, NOT BARS. ~188k bar rows are ~568 independent units.
"""
import csv, io, sys, collections, json
import numpy as np
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.model_selection import GroupKFold
from sklearn.metrics import roc_auc_score, brier_score_loss

RTH_A, RTH_B = 8*3600+30*60, 15*3600
MIN_BARS = 386
SMA_MIN, NBAR_MIN = 150, 60


def load(path):
    """⚠ THE TWO CORPORA ARE NOT THE SAME FORMAT, and assuming they were cost a run:
         ES : comma, WITH a header, `Symbol,Date,VOL,Open,High,Low,Close,Volume`, date "Y-m-d H:M"
         NQ : TAB, NO header, `Symbol,DateTime,Open,High,Low,Close,Volume,?`, date "Y-m-dTH:M"
       So the delimiter, the header and the column ORDER are all sniffed from the file itself rather
       than declared. Take the shape from the real artefact - the same rule the IF scrape taught us.
    """
    ses = collections.defaultdict(list)
    with io.open(path, encoding='utf-8', errors='replace') as f:
        first = f.readline()
        f.seek(0)
        tab = first.count('\t') > first.count(',')
        sep = '\t' if tab else ','
        has_hdr = 'Open' in first or 'Date' in first
        cols = None
        if has_hdr:
            cols = [c.strip() for c in next(csv.reader([first], delimiter=sep))]
            f.readline()
        for parts in csv.reader(f, delimiter=sep):
            if len(parts) < 6:
                continue
            try:
                if cols:
                    rec = dict(zip(cols, parts))
                    stamp = (rec.get('Date') or rec.get('DateTime') or '').strip()
                    o_, h_, l_, c_ = (float(rec['Open']), float(rec['High']),
                                      float(rec['Low']), float(rec['Close']))
                else:
                    # headerless: Symbol, stamp, O, H, L, C, ...
                    stamp = parts[1].strip()
                    o_, h_, l_, c_ = (float(parts[2]), float(parts[3]),
                                      float(parts[4]), float(parts[5]))
                stamp = stamp.strip('"')
                if 'T' in stamp:
                    d, t = stamp.split('T', 1)
                elif ' ' in stamp:
                    d, t = stamp.split(' ', 1)
                else:
                    continue
                pt = t.split(':')
                sec = int(pt[0])*3600 + int(pt[1])*60
                if not (RTH_A <= sec <= RTH_B):
                    continue
                ses[d].append((sec, o_, h_, l_, c_))
            except (ValueError, IndexError, KeyError):
                continue
    return {d: sorted(v) for d, v in ses.items() if len(v) >= MIN_BARS}


def rsi(closes, n=14):
    """Wilder RSI over the given close series."""
    out = [None]*len(closes)
    if len(closes) < n+1:
        return out
    gains = losses = 0.0
    for i in range(1, n+1):
        ch = closes[i]-closes[i-1]
        gains += max(ch, 0.0); losses += max(-ch, 0.0)
    ag, al = gains/n, losses/n
    out[n] = 100.0 if al == 0 else 100 - 100/(1+ag/al)
    for i in range(n+1, len(closes)):
        ch = closes[i]-closes[i-1]
        ag = (ag*(n-1) + max(ch, 0.0))/n
        al = (al*(n-1) + max(-ch, 0.0))/n
        out[i] = 100.0 if al == 0 else 100 - 100/(1+ag/al)
    return out


def build(ses, other=None):
    """other: {day: [(sec,o,h,l,c)]} for the cross-market divergence, or None."""
    days = sorted(ses)
    rows = []
    prev_hi = prev_lo = None
    for d in days:
        b = ses[d]; n = len(b); op = b[0][1]
        fin_lo = min(x[3] for x in b); fin_hi = max(x[2] for x in b)
        ib30h = max(x[2] for x in b[:30]); ib30l = min(x[3] for x in b[:30])
        ib60h = max(x[2] for x in b[:60]); ib60l = min(x[3] for x in b[:60])
        closes = [x[4] for x in b]
        R = rsi(closes, 14*5)          # ~5-minute-equivalent RSI(14) on 1-minute bars
        O = other.get(d) if other else None
        oc = {x[0]: x for x in O} if O else None

        run_lo = 1e9; run_lo_t = None; run_hi = -1e9; run_hi_t = None
        # divergence state, refreshed each time a NEW running extreme prints
        prev_lo_rsi = prev_lo_t = None; div_lo = 0
        prev_hi_rsi = prev_hi_t = None; div_hi = 0
        o_run_lo = 1e9; o_run_hi = -1e9; xdiv_lo = 0; xdiv_hi = 0
        for t in range(n):
            sec, o, h, l, c = b[t]
            new_lo = l < run_lo
            new_hi = h > run_hi
            if new_lo:
                # MOMENTUM DIVERGENCE: price made a lower low, momentum did not.
                if prev_lo_rsi is not None and R[t] is not None and R[t] > prev_lo_rsi:
                    div_lo = 1
                elif prev_lo_rsi is not None:
                    div_lo = 0
                prev_lo_rsi = R[t] if R[t] is not None else prev_lo_rsi
                run_lo, run_lo_t = l, t
            if new_hi:
                if prev_hi_rsi is not None and R[t] is not None and R[t] < prev_hi_rsi:
                    div_hi = 1
                elif prev_hi_rsi is not None:
                    div_hi = 0
                prev_hi_rsi = R[t] if R[t] is not None else prev_hi_rsi
                run_hi, run_hi_t = h, t
            # CROSS-MARKET DIVERGENCE: this book made a new extreme, the sibling did not.
            if oc is not None and sec in oc:
                ol, oh = oc[sec][3], oc[sec][2]
                o_new_lo = ol < o_run_lo; o_new_hi = oh > o_run_hi
                if new_lo:
                    xdiv_lo = 0 if o_new_lo else 1
                if new_hi:
                    xdiv_hi = 0 if o_new_hi else 1
                if o_new_lo: o_run_lo = ol
                if o_new_hi: o_run_hi = oh
            if t < 60:
                continue
            # ⚠ DECIDE EVERY 5 MINUTES, not every minute. The 1-minute bars still drive the
            # extremes, the RSI and the divergences - this only samples the DECISION. Consecutive
            # minutes are near-duplicates that add cost and no information (the label is unchanged),
            # and 188k rows through a GBM does not finish. Effective n is session-sides either way.
            if t % 5:
                continue
            ma = sum(closes[max(0, t-SMA_MIN+1):t+1]) / min(t+1, SMA_MIN)
            w = b[max(0, t-NBAR_MIN):t]
            hiN = max(x[2] for x in w); loN = min(x[3] for x in w)
            rng = run_hi - run_lo
            if rng <= 0:
                continue
            for side in (0, 1):
                low = side == 0
                ext_t = run_lo_t if low else run_hi_t
                y = 1 if ((run_lo <= fin_lo+1e-9) if low else (run_hi >= fin_hi-1e-9)) else 0
                rows.append(dict(
                    d=d, t=t, side=side, y=y,
                    mins=(sec-RTH_A)/60.0,
                    stood=(sec-b[ext_t][0])/60.0,
                    ib30=1 if ((c > ib30h) if low else (c < ib30l)) else 0,
                    ib60=1 if ((c > ib60h) if low else (c < ib60l)) else 0,
                    swp=1 if ((prev_lo is not None and run_lo < prev_lo and c > prev_lo) if low
                              else (prev_hi is not None and run_hi > prev_hi and c < prev_hi)) else 0,
                    sma=1 if ((c > ma) if low else (c < ma)) else 0,
                    bN=1 if ((c > hiN) if low else (c < loN)) else 0,
                    opn=1 if ((c > op) if low else (c < op)) else 0,
                    posr=((c-run_lo)/rng) if low else ((run_hi-c)/rng),
                    mdiv=(div_lo if low else div_hi),
                    xdiv=(xdiv_lo if low else xdiv_hi),
                    rsi=(R[t] if R[t] is not None else 50.0) if low else (100-(R[t] if R[t] is not None else 50.0)),
                    extmin=(b[ext_t][0]-RTH_A)/60.0,
                ))
        prev_hi, prev_lo = fin_hi, fin_lo
    return days, rows


FEATS_TIME = ['mins', 'stood', 'extmin']
FEATS_ALL = FEATS_TIME + ['ib30', 'ib60', 'swp', 'sma', 'bN', 'opn', 'posr', 'mdiv', 'xdiv', 'rsi', 'side']


def matrix(rows, feats):
    X = np.array([[float(r.get(f, 0)) if f != 'side' else float(r['side']) for f in feats] for r in rows])
    y = np.array([r['y'] for r in rows])
    g = np.array([r['d'] for r in rows])
    return X, y, g


def cv(rows, feats, model_fn, folds=5):
    X, y, g = matrix(rows, feats)
    oof = np.zeros(len(y))
    for tr, te in GroupKFold(n_splits=folds).split(X, y, g):
        m = model_fn()
        m.fit(X[tr], y[tr])
        oof[te] = m.predict_proba(X[te])[:, 1]
    return oof, y, g


def report(name, oof, y):
    print('  %-26s AUC %.3f   Brier %.4f' % (name, roc_auc_score(y, oof), brier_score_loss(y, oof)))
    return roc_auc_score(y, oof)


def main(es_path, nq_path=None):
    es = load(es_path)
    nq = load(nq_path) if nq_path else None
    if nq:
        print('cross-market: NQ loaded, %d sessions overlap' % len(set(es) & set(nq)))
    else:
        print('cross-market: NQ ABSENT - xdiv will be constant 0 and must be read as UNTESTED')
    days, rows = build(es, nq)
    print('corpus: %d sessions, %d bar-rows, %d session-sides\n' % (len(days), len(rows), len(days)*2))

    LOGIT = lambda: LogisticRegression(max_iter=2000, C=1.0)
    GBM = lambda: GradientBoostingClassifier(n_estimators=120, max_depth=3, learning_rate=0.08,
                                             subsample=0.9, random_state=7)
    print('OUT-OF-FOLD (grouped by session):')
    o_t, y, g = cv(rows, FEATS_TIME, LOGIT); a_t = report('TIME ONLY (the baseline)', o_t, y)
    o_l, _, _ = cv(rows, FEATS_ALL, LOGIT);  a_l = report('logistic, all features', o_l, y)
    o_g, _, _ = cv(rows, FEATS_ALL, GBM);    a_g = report('gradient boosting', o_g, y)
    print('\n  lift over the clock:  logistic %+.3f   GBM %+.3f AUC' % (a_l-a_t, a_g-a_t))

    # calibration of the best model
    best, bname = (o_g, 'GBM') if a_g >= a_l else (o_l, 'logistic')
    print('\nCALIBRATION (%s, out-of-fold):' % bname)
    print('  %-12s %8s %8s %8s' % ('bucket', 'n', 'predicted', 'actual'))
    for lo in [0, .1, .2, .3, .4, .5, .6, .7, .8, .9]:
        m = (best >= lo) & (best < lo+.1)
        if m.sum() >= 30:
            print('  %.1f-%.1f      %8d %8.0f%% %8.0f%%' % (lo, lo+.1, m.sum(),
                                                            100*best[m].mean(), 100*y[m].mean()))
    # the decision: FIRST bar the probability crosses a threshold, one row per session-side
    print('\nDECISION RULE - first bar P crosses the threshold (one row per session-side):')
    print('  %-8s %8s %8s %10s' % ('thresh', 'n', 'hit', 'median CT'))
    idx = collections.defaultdict(list)
    for i, r in enumerate(rows):
        idx[(r['d'], r['side'])].append(i)
    for th in [.60, .70, .75, .80, .85, .90]:
        n = h = 0; ts = []
        for k, ii in idx.items():
            for i in ii:
                if best[i] >= th:
                    n += 1; h += y[i]; ts.append(rows[i]['mins']); break
        if n:
            ts.sort(); med = ts[len(ts)//2]
            sec = RTH_A + med*60
            print('  P>=%.2f  %8d %7.0f%% %10s' % (th, n, 100*h/n,
                  '%d:%02d' % (int(sec//3600), int((sec % 3600)//60))))
    return dict(auc_time=a_t, auc_logit=a_l, auc_gbm=a_g)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else 'data/es-1min/ES TestingData.txt',
         sys.argv[2] if len(sys.argv) > 2 else None)


def ablate(es_path, nq_path=None):
    """Drop ONE feature at a time and measure what the model loses. This is the only honest way to
    answer 'does X help?' - a coefficient in a correlated feature set does not answer it."""
    es = load(es_path); nq = load(nq_path) if nq_path else None
    days, rows = build(es, nq)
    LOGIT = lambda: LogisticRegression(max_iter=2000, C=1.0)
    o, y, g = cv(rows, FEATS_ALL, LOGIT)
    full = roc_auc_score(y, o)
    o_t, _, _ = cv(rows, FEATS_TIME, LOGIT)
    base = roc_auc_score(y, o_t)
    print('time-only AUC %.4f   full AUC %.4f   headroom %+.4f\n' % (base, full, full-base))
    print('%-8s %10s %10s   %s' % ('drop', 'AUC', 'delta', 'reading'))
    out = []
    for f in FEATS_ALL:
        if f in FEATS_TIME:
            continue
        sub = [x for x in FEATS_ALL if x != f]
        oo, _, _ = cv(rows, sub, LOGIT)
        a = roc_auc_score(y, oo)
        d = a - full
        out.append((f, d))
        tag = 'CARRIES IT' if d < -0.004 else ('some' if d < -0.001 else 'nothing measurable')
        print('%-8s %10.4f %+10.4f   %s' % (f, a, d, tag))
    # and each feature ALONE on top of time
    print('\n%-8s %10s %10s' % ('add', 'AUC', 'vs time'))
    for f in FEATS_ALL:
        if f in FEATS_TIME:
            continue
        oo, _, _ = cv(rows, FEATS_TIME+[f], LOGIT)
        a = roc_auc_score(y, oo)
        print('%-8s %10.4f %+10.4f' % (f, a, a-base))
    return out
