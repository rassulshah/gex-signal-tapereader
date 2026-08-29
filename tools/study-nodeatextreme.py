#!/usr/bin/env python3
"""
WAS THE EXTREME AT A NODE? — using the TRINITY tops, which have been recorded all along.

Operator, 2026-08-30: "i have the trinity dom open which has all the nodes for spy qqq spxw and vix
and you have been recording that."

He is right and I had missed it. Every recorded snapshot carries `tri.<SYM>.top` — the ranked node
list [[strike, %King], ...] for SPY, QQQ, SPXW and VIX. I had been reading only `tri.<SYM>.king`,
which is the single crown, and concluding "no node was there" from a one-line view of a full book.

⚠⚠ EACH BOOK IS MEASURED IN ITS OWN STRIKE SPACE. QQQ nodes are QQQ strikes, SPY nodes SPY strikes.
Distance is reported in that book's own points and as a % of its own spot, never converted, because
"2 points" means something different on a 724 instrument than on a 775 one.

⚠ THE EXTREMES HERE ARE SNAPSHOT SPOTS, not intrabar highs. Snapshots are ~3 minutes apart, so a
true high made between them is missed. The extreme reported is therefore a floor on the real one.
"""
import json, io, glob, datetime

def ct(ms): return datetime.datetime.utcfromtimestamp(ms/1000-5*3600).strftime('%H:%M')

def spot(x, sym):
    xm = x.get('xm') or {}
    return (xm.get(sym) or {}).get('px')

def analyse(day, sym):
    d = json.load(io.open(day))
    s = [x for x in d['snaps']['SPY']
         if ((x.get('tri') or {}).get(sym, {}) or {}).get('top') and spot(x, sym)]
    if len(s) < 20: return None
    # RTH only — the 26th/25th files run into the evening
    s = [x for x in s if '08:30' <= ct(x['t']) <= '15:00']
    if len(s) < 20: return None
    hi = max(s, key=lambda x: spot(x, sym)); lo = min(s, key=lambda x: spot(x, sym))
    out = {'date': d['date'], 'sym': sym, 'n': len(s)}
    for lab, x in (('HOD', hi), ('LOD', lo)):
        px = spot(x, sym)
        top = (x['tri'][sym] or {}).get('top') or []
        if not top: continue
        # nearest node, and where it ranks by |%King|
        ranked = sorted(top, key=lambda p: -abs(p[1]))
        near = min(((abs(px - p[0]), p[0], p[1], i + 1) for i, p in enumerate(ranked)))
        out[lab] = {'ct': ct(x['t']), 'px': round(px, 2),
                    'node': near[1], 'dist': round(px - near[1], 2),
                    'pctKing': near[2], 'rank': '%d/%d' % (near[3], len(ranked)),
                    'king': (x['tri'][sym] or {}).get('king'),
                    'distPct': round(100 * abs(px - near[1]) / px, 3)}
    return out

print('%-11s %-5s %-4s %-7s %-8s %-7s %-6s %-6s %-7s' %
      ('date','sym','what','ct','spot','node','dist','%King','rank'))
rows=[]
for f in sorted(glob.glob('data/2026-*.json')):
    for sym in ('SPY','QQQ','SPXW'):
        r = analyse(f, sym)
        if not r: continue
        for what in ('HOD','LOD'):
            if what not in r: continue
            e = r[what]; rows.append((r['date'], sym, what, e))
            print('%-11s %-5s %-4s %-7s %-8.2f %-7s %-6.2f %-6s %-7s' %
                  (r['date'], sym, what, e['ct'], e['px'], e['node'], e['dist'], e['pctKing'], e['rank']))

if rows:
    print('\n--- how often the extreme sat ON a node ---')
    for tol in (0.10, 0.25, 0.50):
        n = sum(1 for _,_,_,e in rows if e['distPct'] <= tol)
        print('   within %.2f%% of spot:  %3d of %d  (%.0f%%)' % (tol, n, len(rows), 100.0*n/len(rows)))
    n1 = sum(1 for _,_,_,e in rows if e['rank'].startswith('1/'))
    print('   and the node it sat on was the BIGGEST on the board: %d of %d (%.0f%%)'
          % (n1, len(rows), 100.0*n1/len(rows)))
