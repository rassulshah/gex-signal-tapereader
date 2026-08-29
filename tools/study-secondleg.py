#!/usr/bin/env python3
"""
THE LEG FROM THE SECOND EXTREME TO THE CLOSE — how long, how far, how much.

Operator, 2026-08-29: "lets add a field that measures the time it took from the second extreme to the
close ... The idea here is that there maybe an opportunity to trade from the second extreme to the
close and it would be a good idea to identify how much time it usually takes from the second extreme
to the close and the distance (dollar amount)"

⚠ CORPUS TIMES ARE ET (the file is 24h globex: Sunday 17:01 -> Friday 16:00). RTH is 09:30-15:59 ET,
which is the 08:30-15:00 CT the panel speaks. Converting once, here, and never again downstream.
⚠ ES is $50 a point. Stated once, used once.
"""
import csv, io, collections, statistics

PT = 50.0
rows = collections.defaultdict(list)
r = csv.DictReader(io.open('data/es-1min/ES TestingData.txt', encoding='utf-8'))
for row in r:
    v = row['Date'].strip().strip('"')
    if ' ' not in v: continue
    d, t = v.split(' ', 1)
    hh, mm = t.split(':')[0:2]
    m = int(hh) * 60 + int(mm)
    if m < 9*60+30 or m > 15*60+59: continue          # RTH only, ET
    try:
        rows[d].append((m, float(row['High']), float(row['Low']), float(row['Close'])))
    except Exception:
        continue

legs = []
for d, bars in rows.items():
    if len(bars) < 300: continue                       # a short/holiday session is not a session
    bars.sort()
    hi = max(bars, key=lambda b: b[1]); lo = min(bars, key=lambda b: b[2])
    hiT, loT = hi[0], lo[0]
    if hiT == loT: continue
    secondIsHOD = hiT > loT
    secT   = hiT if secondIsHOD else loT
    secPx  = hi[1] if secondIsHOD else lo[2]
    closeM, closePx = bars[-1][0], bars[-1][3]
    mins = closeM - secT
    if mins <= 0: continue
    dist = abs(closePx - secPx)
    legs.append({'d': d, 'second': 'HOD' if secondIsHOD else 'LOD', 'secT': secT,
                 'mins': mins, 'pts': dist, 'usd': dist * PT,
                 'rng': hi[1] - lo[2], 'frac': dist / (hi[1] - lo[2]) if hi[1] > lo[2] else 0})

def q(vals, p):
    vals = sorted(vals); 
    if not vals: return 0
    i = min(len(vals)-1, max(0, int(round(p*(len(vals)-1)))))
    return vals[i]

def clk(m): 
    h = m//60; ap = 'pm' if h >= 12 else 'am'; h12 = h % 12 or 12
    return '%d:%02d%s' % (h12, m % 60, ap)

print('sessions: %d   (%s .. %s)' % (len(legs), min(l['d'] for l in legs), max(l['d'] for l in legs)))
for lab, sel in (('ALL', legs),
                 ('second = HOD', [l for l in legs if l['second']=='HOD']),
                 ('second = LOD', [l for l in legs if l['second']=='LOD'])):
    if not sel: continue
    M=[l['mins'] for l in sel]; P=[l['pts'] for l in sel]; F=[l['frac'] for l in sel]
    T=[l['secT'] for l in sel]
    print('\n--- %s   n=%d ---' % (lab, len(sel)))
    print('  second extreme prints   median %s   middle half %s .. %s'
          % (clk(q(T,.5)), clk(q(T,.25)), clk(q(T,.75))))
    print('  TIME to close           median %dh%02d   middle half %dh%02d .. %dh%02d'
          % (q(M,.5)//60, q(M,.5)%60, q(M,.25)//60, q(M,.25)%60, q(M,.75)//60, q(M,.75)%60))
    print('  DISTANCE to close       median %.1f pts ($%s)   middle half %.1f .. %.1f pts'
          % (q(P,.5), format(int(q(P,.5)*PT), ','), q(P,.25), q(P,.75)))
    print('  ...as %% of the day range median %d%%   middle half %d%% .. %d%%'
          % (round(100*q(F,.5)), round(100*q(F,.25)), round(100*q(F,.75))))

# is the leg worth trading? how often does it clear a threshold
print('\n--- how often the leg clears a size, ALL sessions ---')
for thr in (5,10,15,20,30,40):
    n = sum(1 for l in legs if l['pts'] >= thr)
    print('   >= %2d pts ($%-6s)  %5.1f%%  (n=%d)' % (thr, format(int(thr*PT),','), 100.0*n/len(legs), n))
