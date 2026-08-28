#!/usr/bin/env python3
"""
BEFORE BUILDING: does the NOT-IN half work, and is "expect the other side around X" defensible?

Three questions, each of which could sink the build:
  A · when the table says a LOW probability, is the extreme actually BROKEN at that rate?
  B · when the call fires, has the OTHER extremity already printed? (if so, "toward HOD" is wrong)
  C · given the call fires and is right, WHEN does the other side actually print?
"""
import importlib.util, collections, numpy as np
from sklearn.model_selection import GroupKFold
spec = importlib.util.spec_from_file_location('m', 'tools/model-lodhod.py')
mm = importlib.util.module_from_spec(spec); spec.loader.exec_module(mm)
RTH_A = mm.RTH_A
CELL = lambda r: (min(int(r['posr']*8), 7), min(int(r['mins']//45), 8))

es = mm.load('data/es-1min/ES TestingData.txt')
days, rows = mm.build(es, None)
y = np.array([r['y'] for r in rows]); g = np.array([r['d'] for r in rows])

# out-of-fold table probability, so nothing scores itself
p = np.zeros(len(rows))
for tr, te in GroupKFold(n_splits=5).split(np.zeros((len(y), 1)), y, g):
    t = collections.defaultdict(lambda: [0, 0]); prior = y[tr].mean()
    for i in tr:
        k = CELL(rows[i]); t[k][0] += 1; t[k][1] += y[i]
    for i in te:
        n, h = t[CELL(rows[i])]; p[i] = (h + prior*10)/(n + 10)
for i, r in enumerate(rows):
    r['p'] = p[i]

print('A · THE "NOT IN" SIDE — when the table says LOW, is the extreme really broken?\n')
print('  %-14s %8s %10s %12s' % ('table says', 'n', 'actually broke', 'median CT'))
for lo, hi in [(0, .10), (.10, .20), (.20, .30), (.30, .40)]:
    sel = [r for r in rows if lo <= r['p'] < hi]
    if len(sel) < 50: continue
    broke = np.mean([1-r['y'] for r in sel])
    ts = sorted(r['mins'] for r in sel); med = RTH_A + ts[len(ts)//2]*60
    print('  %-14s %8d %9.0f%% %12s' % ('%.0f-%.0f%%' % (100*lo, 100*hi), len(sel), 100*broke,
                                        '%d:%02d' % (med//3600, (med % 3600)//60)))
# first-crossing version: the decision he would actually act on
print('\n  first time the table drops to <=X%, one row per session-side:')
print('  %-14s %8s %10s %12s' % ('threshold', 'n', 'actually broke', 'median CT'))
byk = collections.defaultdict(list)
for r in rows: byk[(r['d'], r['side'])].append(r)
for th in [.10, .15, .20, .25, .30]:
    n = 0; broke = 0; ts = []
    for k, rs in byk.items():
        for r in sorted(rs, key=lambda x: x['mins']):
            if r['p'] <= th:
                n += 1; broke += (1-r['y']); ts.append(r['mins']); break
    if n:
        ts.sort(); med = RTH_A + ts[len(ts)//2]*60
        print('  %-14s %8d %9.0f%% %12s' % ('P<=%.0f%%' % (100*th), n, 100*broke/n,
                                            '%d:%02d' % (med//3600, (med % 3600)//60)))

print('\nB · WHEN THE CALL FIRES, HAS THE OTHER SIDE ALREADY PRINTED?')
print('  (if it has, "toward the HOD" is advice about something already over)\n')
sess = {}
for d in days:
    b = es[d]
    loT = min(range(len(b)), key=lambda i: b[i][3]); hiT = max(range(len(b)), key=lambda i: b[i][2])
    sess[d] = (loT, hiT, b)
for th in [.70, .80]:
    n = 0; already = 0; laterT = []
    for (d, side), rs in byk.items():
        loT, hiT, b = sess[d]
        otherT = hiT if side == 0 else loT
        for r in sorted(rs, key=lambda x: x['mins']):
            if r['p'] >= th and r['y'] == 1:      # the call fired AND was right
                n += 1
                if otherT*1.0 <= r['mins']: already += 1
                else: laterT.append(b[otherT][0])
                break
    if n:
        laterT.sort()
        med = laterT[len(laterT)//2] if laterT else 0
        q1 = laterT[len(laterT)//4] if laterT else 0
        q3 = laterT[3*len(laterT)//4] if laterT else 0
        print('  P>=%.0f%%  n=%d   other side ALREADY in: %.0f%%   when still ahead: median %s (IQR %s-%s)'
              % (100*th, n, 100*already/n,
                 '%d:%02d' % (med//3600, (med % 3600)//60),
                 '%d:%02d' % (q1//3600, (q1 % 3600)//60),
                 '%d:%02d' % (q3//3600, (q3 % 3600)//60)))
