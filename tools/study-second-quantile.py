#!/usr/bin/env python3
"""(2026-09-17) IS "LOD AFTER <T> 80%" BETTER THAN THE BASE RATE?  Operator: "did you check to see if you could make a model
for predicting it also that is better than the base rate?"  The 2ND clock is not predictable as a POINT (study-daystats-cond:
~93 min MAE for every model).  But the READ line makes a QUANTILE claim - "on 80% of days the 2ND printed after T" - and a
quantile can be sharpened by conditioning even when the point cannot.  Out-of-fold (10 folds, 301 sessions), for each
conditioning: the 20th-percentile clock is fit on train and, on test, we measure
  coverage  = share of test days whose 2ND printed AFTER the clock (should be ~80%; below it the line over-promises),
  sharpness = the mean clock itself (LATER is more informative: "after 11:30" says more than "after 10:00"),
  pinball   = the q=0.2 pinball loss (the proper score for a quantile; lower is better).
Conditionings: pooled (the base rate) | pos30 / pos60 terciles (the stage tables) | which extreme was first (known once the
READ calls it IN) | the 1ST's clock terciles (known then too) | first x 1ST-clock | 1ST-clock x pos60.
Also the SURVIVAL view: given the 2ND has not printed by 'now' (the clock at which the panel would print the line), the
conditional 20th percentile - i.e. does re-fitting on days with t2 > now beat the static clock?
"""
import sys, os, json, statistics as st, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import importlib
sc = importlib.import_module('study-daystats-cond')

Q = 0.2
def qtile(xs, q):
    if not xs: return None
    xs = sorted(xs); k = max(0, min(len(xs) - 1, int(round(q * (len(xs) - 1))))); return xs[k]
def pinball(y, f, q): return (q * (y - f)) if y >= f else ((1 - q) * (f - y))
def T3(v): return 0 if v < 1 / 3 else (1 if v < 2 / 3 else 2)

def main():
    rows = sc.sessions(); n = len(rows)
    t1e = [qtile([r['t1'] for r in rows], 1/3), qtile([r['t1'] for r in rows], 2/3)]
    def t1b(r): return 0 if r['t1'] < t1e[0] else (1 if r['t1'] < t1e[1] else 2)
    feats = [
        ('pooled (the base rate)',            lambda r: 0),
        ('pos30 tercile',                     lambda r: T3(r['pos30'])),
        ('pos60 tercile',                     lambda r: T3(r['pos60'])),
        ('first extreme side (HOD|LOD)',      lambda r: r['first']),
        ('1ST clock tercile',                 lambda r: t1b(r)),
        ('first side x 1ST clock tercile',    lambda r: (r['first'], t1b(r))),
        ('1ST clock tercile x pos60',         lambda r: (t1b(r), T3(r['pos60']))),
        ('weekday',                           lambda r: r['wd']),
    ]
    print('n=%d sessions %s -> %s   out-of-fold, q=0.20' % (n, rows[0]['day'], rows[-1]['day']))
    print('%-38s %9s %10s %9s %9s' % ('conditioning', 'coverage', 'mean clock', 'pinball', 'p20 spread'))
    res = {}
    for name, fb in feats:
        cov = []; clk = []; pb = []; spread = set()
        for train, test in sc.folds(rows):
            by = collections.defaultdict(list)
            for x in train: by[fb(x)].append(x['t2'])
            pooled = qtile([x['t2'] for x in train], Q)
            for x in test:
                grp = by.get(fb(x), [])
                f = qtile(grp, Q) if len(grp) >= 15 else pooled
                cov.append(1 if x['t2'] > f else 0); clk.append(f); pb.append(pinball(x['t2'], f, Q)); spread.add(round(f))
        res[name] = dict(coverage=round(st.mean(cov), 3), clock=round(st.mean(clk), 1), pinball=round(st.mean(pb), 2))
        h, m = divmod(int(st.mean(clk)), 60)
        print('%-38s %8.1f%% %6.0f (%02d:%02d) %9.2f %9s' % (name, 100 * st.mean(cov), st.mean(clk), 8 + h + (m + 30) // 60 if False else 8 + (30 + int(st.mean(clk))) // 60, (30 + int(st.mean(clk))) % 60, st.mean(pb), '%d values' % len(spread)))
    # the survival view: at 'now' = 60 / 120 / 180 min after the open, among days whose 2ND is still ahead
    print('\nSURVIVAL: p20 of the 2ND clock among days with t2 > now (out-of-fold), vs the static pooled p20')
    print('%-10s %10s %14s %10s %14s' % ('now (min)', 'days left', 'p20 | t2>now', 'coverage', 'static p20 cov'))
    for now in (60, 120, 180, 240):
        cov = []; clk = []; scov = []
        for train, test in sc.folds(rows):
            alive = [x['t2'] for x in train if x['t2'] > now]
            f = qtile(alive, Q) if len(alive) >= 15 else None
            s = qtile([x['t2'] for x in train], Q)
            for x in test:
                if x['t2'] <= now: continue
                if f is not None: cov.append(1 if x['t2'] > f else 0); clk.append(f)
                scov.append(1 if x['t2'] > s else 0)
        left = sum(1 for x in rows if x['t2'] > now)
        print('%-10d %10d %7.0f (%02d:%02d) %9.1f%% %13.1f%%' % (now, left, st.mean(clk), (30 + int(st.mean(clk))) // 60 + 8, (30 + int(st.mean(clk))) % 60, 100 * st.mean(cov), 100 * st.mean(scov)))
    out = dict(n=n, first=rows[0]['day'], last=rows[-1]['day'], q=Q, models=res, note='coverage ~80% = honest; a later mean clock at the same coverage = more informative; lower pinball = better quantile')
    json.dump(out, open('data/es-1min/SECOND-QUANTILE-STUDY.json', 'w'), indent=1)
    print('\nwrote data/es-1min/SECOND-QUANTILE-STUDY.json')

if __name__ == '__main__':
    main()
