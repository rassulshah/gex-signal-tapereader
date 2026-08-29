#!/usr/bin/env python3
"""
Re-run of the succession question against the CROWN THE PANEL DRAWS, not the raw tape.

study-succession.py measured the outcome as "snap.king equals that strike" — the RAW tape top,
which flaps between near-equal strikes several times an hour. The operator caught that the drawn
crown moves far less ("i am looking at atlas and it doesn't seem to move that much"), so every hit
rate in that study counted flicker as success. This replays kingLatchTick (120s continuous hold)
and asks the question again against the label he actually sees.

Predictor is read at snap i, outcome strictly after. Growth is measured in %King, NOT in dollars —
the day files predate the `vend` schema, so the DOLLAR-delta question is still unanswered.
"""
import json, io, glob

HZ = 30           # minutes
LATCH_MS = 120000

def drawn(series):
    """[(t,rawKing)] -> [(t, crownAtThatMoment)] for every snap, replaying the panel's latch."""
    if not series: return []
    k = series[0][1]; cand = None; c0 = 0
    out = [(series[0][0], k)]
    for t, tk in series[1:]:
        if tk == k: cand = None
        elif cand != tk: cand = tk; c0 = t
        elif t - c0 >= LATCH_MS: k = tk; cand = None
        out.append((t, k))
    return out

def load():
    days = []
    for f in sorted(glob.glob('data/2026-*.json')):
        d = json.load(io.open(f)); s = (d.get('snaps') or {}).get('SPY') or []
        s = [x for x in s if isinstance(x.get('king'), (int, float))]
        if len(s) < 20: continue
        crown = dict((t, k) for t, k in drawn([(x['t'], x['king']) for x in s]))
        days.append((d.get('date'), s, crown))
    return days

days = load()
def ranks(x):
    r = (x.get('deriv') or {}).get('ranks') or []
    return [(q.get('k'), q.get('m')) for q in r if q.get('k') is not None and q.get('m') is not None]

def crowned_within(s, crown, i, k2, hz):
    t0 = s[i]['t']
    for y in s[i+1:]:
        if y['t'] - t0 > hz*60000: break
        if crown.get(y['t']) == k2: return True
    return False

def run(minSize, mode, growWin=3):
    hit = n = 0
    for date, s, crown in days:
        for i, x in enumerate(s):
            king = crown.get(x['t'])
            R = ranks(x)
            cand = [(k, m) for k, m in R if k != king]
            if not cand: continue
            k2, m2 = cand[0]
            if m2 < minSize: continue
            if mode != 'any':
                j = i - growWin
                if j < 0: continue
                prev = dict(ranks(s[j])).get(k2)
                if prev is None: continue
                grow = m2 > prev
                if mode == 'grow' and not grow: continue
                if mode == 'flat' and grow: continue
            n += 1
            if crowned_within(s, crown, i, k2, HZ): hit += 1
    return n, (100.0*hit/n if n else 0.0)

# chance: any ranked non-crown strike
bn = bh = 0
for date, s, crown in days:
    for i, x in enumerate(s):
        king = crown.get(x['t'])
        for k, m in ranks(x):
            if k == king: continue
            bn += 1
            if crowned_within(s, crown, i, k, HZ): bh += 1
print('days %d   snaps %d' % (len(days), sum(len(s) for _, s, _ in days)))
print('\nCHANCE  any ranked non-crown strike takes the crown in %dm:  n=%d  %.1f%%'
      % (HZ, bn, 100.0*bh/bn if bn else 0))

print('\n%-14s %10s %10s %10s %10s' % ('runner-up', 'all n', 'all %', 'growing %', 'flat %'))
for ms in (40, 50, 60, 70, 80, 90):
    n0, p0 = run(ms, 'any')
    ng, pg = run(ms, 'grow')
    nf, pf = run(ms, 'flat')
    print('  size >= %-3d%% %8d %9.1f%% %6.1f%% (n=%-4d) %6.1f%% (n=%-4d)  lift %+5.1fpp'
          % (ms, n0, p0, pg, ng, pf, nf, pg - pf))
