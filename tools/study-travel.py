#!/usr/bin/env python3
"""
IS THERE A TRADE IN IT?  - the question the LOD/HOD table does NOT answer.

The table says whether the extreme HOLDS. The operator's requirement is to PROFIT travelling to the
other extremity. Those are different, and the difference could be fatal: the table's confidence comes
from `posr` - how far price has ALREADY moved off the extreme - so it is most confident exactly when
the move is furthest along.

Measured here, per (session, side): at the first bar the table gives >= X%, how much of the day's
range is STILL AHEAD, and how far does price actually get before the close?
"""
import importlib.util, collections, numpy as np
spec = importlib.util.spec_from_file_location('m', 'tools/model-lodhod.py')
mm = importlib.util.module_from_spec(spec); spec.loader.exec_module(mm)

es = mm.load('data/es-1min/ES TestingData.txt')
days, rows = mm.build(es, None)
RTH_A = mm.RTH_A

# the table, fit out-of-fold so a session never scores itself
from sklearn.model_selection import GroupKFold
y = np.array([r['y'] for r in rows]); g = np.array([r['d'] for r in rows])
key = lambda r: (min(int(r['posr']*8), 7), min(int(r['mins']//45), 8))
p = np.zeros(len(rows))
for tr, te in GroupKFold(n_splits=5).split(np.zeros(len(y)), y, g):
    tab = collections.defaultdict(lambda: [0, 0]); prior = y[tr].mean()
    for i in tr:
        k = key(rows[i]); tab[k][0] += 1; tab[k][1] += y[i]
    for i in te:
        n, h = tab[key(rows[i])]; p[i] = (h + prior*10)/(n + 10)
for i, r in enumerate(rows):
    r['p'] = p[i]

# per session: the true extremes and the full-day close
sess = {}
for d in days:
    b = es[d]
    lo = min(x[3] for x in b); hi = max(x[2] for x in b)
    sess[d] = dict(lo=lo, hi=hi, rng=hi-lo, bars=b)

byk = collections.defaultdict(list)
for r in rows:
    byk[(r['d'], r['side'])].append(r)

print('AT THE FIRST BAR THE TABLE REACHES A GIVEN CONFIDENCE:\n')
print('  %-8s %6s %7s %10s %12s %12s'
      % ('conf', 'n', 'correct', 'median CT', 'range LEFT', 'actually got'))
for th in [0.60, 0.70, 0.75, 0.80, 0.85, 0.90]:
    n = 0; hit = 0; left = []; got = []; ts = []
    for (d, side), rs in byk.items():
        rs.sort(key=lambda r: r['mins'])
        S = sess[d]
        for r in rs:
            if r['p'] < th:
                continue
            n += 1; hit += r['y']; ts.append(r['mins'])
            # where is price now, and how much of the day's range is still ahead toward the far side?
            i = min(int(r['mins']), len(S['bars'])-1)
            px = S['bars'][i][4]
            if side == 0:      # low is in -> the trade is UP toward the high
                left.append((S['hi']-px)/S['rng'])
                fwd = max(x[2] for x in S['bars'][i:]) if i < len(S['bars']) else px
                got.append((fwd-px)/S['rng'])
            else:              # high is in -> the trade is DOWN toward the low
                left.append((px-S['lo'])/S['rng'])
                fwd = min(x[3] for x in S['bars'][i:]) if i < len(S['bars']) else px
                got.append((px-fwd)/S['rng'])
            break
    if n:
        ts.sort(); med = ts[len(ts)//2]; sec = RTH_A+med*60
        print('  P>=%.2f  %6d %6.0f%% %10s %11.0f%% %11.0f%%'
              % (th, n, 100*hit/n, '%d:%02d' % (sec//3600, (sec % 3600)//60),
                 100*np.median(left), 100*np.median(got)))
print("""
  range LEFT   = share of the day's range between price and the far extreme, at the moment of the call
  actually got = share of the day's range price DID travel that way before the close (median)""")
