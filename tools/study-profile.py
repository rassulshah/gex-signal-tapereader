#!/usr/bin/env python3
"""
CAN WE COMPUTE VAH / VAL / POC, AND DOES PRICE ACTUALLY TAG THEM?

Operator, 2026-08-29: "there maybe some explicit levels that the market tagged. like today it went to
VAL on the way down ... can you calculate key levels like VAH VAL POC or TPO as well as price based
levels."

YES on the arithmetic: the corpus carries Volume per 1-minute bar, and so does the live tap
(futBarsRaw rows are [epoch, o, h, l, c, VOLUME]). So this is a real VOLUME profile, not a TPO
approximation, and the same code runs live and historical.

⚠⚠ THIS IS NOT THE PANEL'S EXISTING "VALUE 70%" TILE. That one is evaBandFromPct() — the band around
the KING holding 70% of GAMMA mass. Same word, different book, different axis: exposure, not traded
volume. Shipping a price VA next to it without saying so would be failure pattern #1 (a value under a
label implying a different claim). If both ship, they need different names.

The test that matters is not "can we draw it" but "does price care": how often does the NEXT session
trade to the prior session's POC / VAH / VAL, against what a random level of the same distance does.
"""
import csv, io, collections, statistics

TICK = 0.25
def profile(bars):
    """bars = [(m, h, l, c, vol)] -> (poc, vah, val) by volume, 70% value area, ticks as bins."""
    vol = collections.defaultdict(float)
    for m, h, l, c, v in bars:
        if h < l or v <= 0: continue
        n = int(round((h - l) / TICK)) + 1
        if n <= 0 or n > 4000: continue
        share = v / n
        p = l
        for _ in range(n):
            vol[round(p / TICK)] += share
            p += TICK
    if not vol: return None
    total = sum(vol.values())
    poc = max(vol, key=lambda k: vol[k])
    lo = hi = poc; acc = vol[poc]
    while acc < 0.70 * total:
        up = vol.get(hi + 1, 0.0); dn = vol.get(lo - 1, 0.0)
        if up == 0 and dn == 0: break
        if up >= dn: hi += 1; acc += up
        else:        lo -= 1; acc += dn
    return (poc * TICK, hi * TICK, lo * TICK)

rows = collections.defaultdict(list)
r = csv.DictReader(io.open('data/es-1min/ES TestingData.txt', encoding='utf-8'))
for row in r:
    v = row['Date'].strip().strip('"')
    if ' ' not in v: continue
    d, t = v.split(' ', 1)
    hh, mm = t.split(':')[0:2]
    m = int(hh) * 60 + int(mm)
    if m < 9*60+30 or m > 15*60+59: continue
    try: rows[d].append((m, float(row['High']), float(row['Low']), float(row['Close']), float(row['Volume'])))
    except Exception: continue

days = sorted(d for d, b in rows.items() if len(b) >= 300)
print('sessions with a full RTH profile: %d  (%s .. %s)' % (len(days), days[0], days[-1]))

profs = {}
for d in days:
    p = profile(sorted(rows[d]))
    if p: profs[d] = p

# does the NEXT session trade to the prior session's levels?
TOL = 1.0    # ES points; a "tag" is price coming within this of the level
hits = collections.Counter(); tot = 0
dist = collections.defaultdict(list)
for i in range(1, len(days)):
    pd_, d = days[i-1], days[i]
    if pd_ not in profs: continue
    poc, vah, val = profs[pd_]
    bars = sorted(rows[d])
    hi = max(b[1] for b in bars); lo = min(b[2] for b in bars); op = bars[0][3]
    tot += 1
    for nm, lv in (('POC', poc), ('VAH', vah), ('VAL', val)):
        if lo - TOL <= lv <= hi + TOL: hits[nm] += 1
        dist[nm].append(abs(lv - op))
print('\n--- does the next session TAG the prior session\'s level? (tol %.2f pts) ---' % TOL)
for nm in ('POC', 'VAH', 'VAL'):
    med = statistics.median(dist[nm]) if dist[nm] else 0
    print('   prior %-4s tagged %5.1f%%  (n=%d)   median distance from the open %.1f pts'
          % (nm, 100.0*hits[nm]/tot, tot, med))

# ⚠ THE CONTROL. A level 15 points from the open gets tagged a lot because the day is 40 points
# wide, not because it is a POC. Compare each level against a RANDOM level at the SAME distance.
import random
random.seed(7)
ctl = collections.Counter(); cn = collections.Counter()
for i in range(1, len(days)):
    pd_, d = days[i-1], days[i]
    if pd_ not in profs: continue
    poc, vah, val = profs[pd_]
    bars = sorted(rows[d]); hi = max(b[1] for b in bars); lo = min(b[2] for b in bars); op = bars[0][3]
    for nm, lv in (('POC', poc), ('VAH', vah), ('VAL', val)):
        dd = abs(lv - op)
        sham = op + (dd if random.random() < 0.5 else -dd)   # same distance, arbitrary side
        cn[nm] += 1
        if lo - TOL <= sham <= hi + TOL: ctl[nm] += 1
print('\n--- CONTROL: a sham level the SAME distance from the open, arbitrary side ---')
for nm in ('POC', 'VAH', 'VAL'):
    print('   sham @ %-4s distance tagged %5.1f%%   -> the level itself is worth %+.1f pp'
          % (nm, 100.0*ctl[nm]/cn[nm], 100.0*hits[nm]/tot - 100.0*ctl[nm]/cn[nm]))
