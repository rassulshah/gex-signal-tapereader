#!/usr/bin/env python3
"""
How often does the crown ATLAS DRAWS actually move?

Operator, 2026-08-29: "are you sure the spxw king moves that many times. i am looking at atlas and
it doesn't seem to move that much."

He was right and I had answered with the wrong series TWICE.

⚠⚠ THERE ARE TWO KINGS IN THIS DATA AND THEY ARE NOT THE SAME NUMBER.
  · `tri.SPXW.king` (and `snap.king`) is the RAW TAPE top strike, recomputed every snapshot.
  · What the panel DRAWS is the LATCHED crown — kingLatchTick(), KING_LATCH_MS = 120s of
    CONTINUOUS hold before the label moves. Built in v14.19 for exactly this reason: "the tape's top
    strike traded places three times in an hour on 2026-08-27 and the KING label flapped with it."
Measuring the raw series and calling it "the King" counts the very flapping the latch exists to hide.

This replays kingLatchTick over the recorded tape series, so the count is the crown he sees.
"""
import json, io, glob, datetime

LATCH_MS = 120000          # must match KING_LATCH_MS in the panel

def ct(ms): return datetime.datetime.utcfromtimestamp(ms/1000 - 5*3600).strftime('%H:%M')

def latch(series):
    """series = [(t, tapeKing)] -> [(t, crown)] transitions only, replaying the panel's own rule."""
    if not series: return []
    k = series[0][1]; cand = None; ct0 = 0
    out = [(series[0][0], k)]
    for t, tk in series[1:]:
        if tk == k:
            cand = None; continue
        if cand != tk:
            cand = tk; ct0 = t; continue
        if t - ct0 >= LATCH_MS:
            k = tk; cand = None; out.append((t, k))
    return out

def series_for(snaps, field):
    out = []
    for x in snaps:
        if field == 'raw_spx':
            v = ((x.get('tri') or {}).get('SPXW') or {}).get('king')
        else:
            v = x.get('king')
        if isinstance(v, (int, float)): out.append((x['t'], v))
    return out

# ⚠⚠ SEPARATE DENOMINATORS. Only 6 of the 9 day files carry an SPXW series at all — the field
# arrived mid-corpus. Dividing the SPXW total by 9 reports the average over three sessions that
# contained no SPXW data, which is how "4.2 per session" was told to the operator when the real
# figure is 6.3. Count the days that actually contributed, per series, every time.
print('%-12s %10s %10s %10s %10s' % ('date','SPXW raw','SPXW drawn','SPY raw','SPY drawn'))
tot = [0,0,0,0]; n = 0; nx = 0
sx_list = []; sy_list = []
for f in sorted(glob.glob('data/2026-*.json')):
    d = json.load(io.open(f)); s = (d.get('snaps') or {}).get('SPY') or []
    sx = series_for(s,'raw_spx'); sy = series_for(s,'raw_spy')
    if len(sy) < 20: continue
    n += 1
    a = max(0,len(sx)-1) and sum(1 for i in range(1,len(sx)) if sx[i][1]!=sx[i-1][1])
    b = max(0,len(latch(sx))-1) if sx else 0
    c = sum(1 for i in range(1,len(sy)) if sy[i][1]!=sy[i-1][1])
    e = max(0,len(latch(sy))-1)
    tot[2]+=c; tot[3]+=e; sy_list.append(e)
    if sx: tot[0]+=a; tot[1]+=b; nx += 1; sx_list.append(b)
    print('%-12s %10s %10s %10d %10d' % (d.get('date'), a if sx else '-', b if sx else '-', c, e))
import statistics
print('%-12s %10.1f %10.1f %10.1f %10.1f   (SPXW over %d days, SPY over %d)'
      % ('MEAN', tot[0]/nx, tot[1]/nx, tot[2]/n, tot[3]/n, nx, n))
print('%-12s %10s %10.1f %10s %10.1f' % ('MEDIAN','',statistics.median(sx_list),'',statistics.median(sy_list)))
print('%-12s %10s %7d-%-2d %10s %7d-%-2d'
      % ('RANGE','',min(sx_list),max(sx_list),'',min(sy_list),max(sy_list)))

print('\n--- what the latch is suppressing, 2026-08-20 SPXW ---')
d = json.load(io.open('data/2026-08-20.json')); s = d['snaps']['SPY']
sx = series_for(s,'raw_spx')
raw = [(t,k) for i,(t,k) in enumerate(sx) if i==0 or k!=sx[i-1][1]]
print('  RAW   (%2d moves): %s' % (len(raw)-1, ' '.join(ct(t)+'>'+str(k) for t,k in raw)))
dr = latch(sx)
print('  DRAWN (%2d moves): %s' % (len(dr)-1, ' '.join(ct(t)+'>'+str(k) for t,k in dr)))
