#!/usr/bin/env python3
"""study-gridtells.py — calibrate the grid ladder's NEW and SETUP (stack) tells against the taught days.

Operator, 2026-09-04: "The new column has every node as new, which is useless. The purpose of new was when
price is going to a level, a new node pops up and deflects price. It grew rapidly for this purpose."
"The spx barney and pika stacks are not implemented correctly at all because they show barney stacks at
multiple levels. I think you need threshold and examples. look at the skylit documentation."

Doctrine (skylit-docs): a Pika Cloud is a DENSE cluster of POSITIVE nodes, "magnitude matters most here:
thin cloud = soft/porous; dense king-level cloud can pin all session" (learn/heatseeker-patterns.md);
"Clusters of Nodes: when multiple LARGE values group together price pins or chops"; "Double Stacked Nodes:
when multiple nodes are stacked together price can have a strong bounce" (best-practices/faqs.md);
Velocity Mode tracks nodes "growing, shrinking, APPEARING, disappearing" = dealer urgency; "rapid
accumulation acts like a magnet" (learn/air-pockets-velocity.md, core-concepts.md).

Reads the SPXW book per 3-minute bar from the day files' vend rows ([k, cur, d5, d15, d60, d1d], k >= 1000)
and evaluates candidate definitions: how many rows each flags per bar, and whether it catches the legs he
circled (learning/deflections/LEARNING.md, E001-E004).

  python3 tools/study-gridtells.py            # the report
"""
import json, datetime, statistics, sys, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DAYS = ['2026-08-27', '2026-08-28', '2026-08-31', '2026-09-03']
THR = 0.20          # the node threshold, CFG.nodeThresh
NEW_BARS = 30       # GRID_NEW_BARS

def ct(ms):
    return datetime.datetime.utcfromtimestamp(ms / 1000 - 5 * 3600).strftime('%H:%M')

def hhmm_to_sec(s):
    h, m = s.split(':'); return int(h) * 3600 + int(m) * 60

def bar_sec(ms):
    return int((ms / 1000 - 5 * 3600) % 86400)

def load(day):
    d = json.load(open(os.path.join(ROOT, 'data', day + '.json')))
    sn = [s for s in d['snaps']['SPY'] if (s.get('vend') or {}).get('rows')]
    sn.sort(key=lambda s: s.get('bar') or s['t'])
    bars = []
    for s in sn:
        t = s.get('bar') or s['t']
        rows = {}
        for r in s['vend']['rows']:
            if r[0] >= 1000 and isinstance(r[1], (int, float)):
                rows[float(r[0])] = {'cur': r[1], 'd5': r[2], 'd15': r[3], 'd60': r[4]}
        if not rows: continue
        kmax = max(abs(v['cur']) for v in rows.values())
        for k, v in rows.items():
            v['pct'] = abs(v['cur']) / kmax if kmax else 0
        bars.append({'t': t, 'sec': bar_sec(t), 'rows': rows, 'kmax': kmax})
    return bars

# ---------------------------------------------------------------- NEW ---------------------------------------
def births(bars):
    """first bar a strike holds >= THR of the King, with a PRIOR bar in which it was below or absent.
    The opening book (bar 0) is never a birth; a strike first seen right after a recording gap (> 10 min) is
    UNKNOWN, not a birth."""
    born, seen_below, seen_any = {}, set(), set()
    prev_t = None
    for i, b in enumerate(bars):
        gap = prev_t is not None and (b['t'] - prev_t) > 10 * 60 * 1000
        for k, v in b['rows'].items():
            if v['pct'] >= THR:
                # a prior bar showed it below the threshold, or absent from the top list (= small), and this
                # is not the first bar of the day and not the first bar after a recording gap
                if i > 0 and not gap and k not in born and (k in seen_below or k not in seen_any):
                    born[k] = {'i': i, 't': b['t'], 'mag': abs(v['cur']), 'pct': v['pct']}
            else:
                seen_below.add(k)
        # everything seen so far but absent from THIS bar's rows was below the cut this bar
        for k in seen_any:
            if k not in b['rows']:
                seen_below.add(k)
        seen_any.update(b['rows'].keys())
        prev_t = b['t']
    return born

def new_defs(bars, born):
    """per bar, the rows each definition flags as NEW"""
    out = {'A_current': [], 'B_no_opening_book': [], 'C_B_and_growth': [], 'D_B_and_mass': [], 'E_B_growth_or_mass': []}
    for i, b in enumerate(bars):
        A = B = C = D = E = 0
        for k, v in b['rows'].items():
            if v['pct'] < THR: continue
            # A: what ships — first sight is a birth, including bar 0
            first_i = next(j for j in range(i + 1) if k in bars[j]['rows'] and bars[j]['rows'][k]['pct'] >= THR)
            if i - first_i <= NEW_BARS: A += 1
            bb = born.get(k)
            if not bb or i - bb['i'] > NEW_BARS or i < bb['i']: continue
            B += 1
            # growth since birth: doubled its birth magnitude, or growing >= +20%/15m now
            mag = abs(v['cur']); g15 = None
            if isinstance(v['d15'], (int, float)):
                dm = v['d15'] if v['cur'] >= 0 else -v['d15']
                base = mag - dm
                if base > 0: g15 = dm / base
            grew = (mag >= 2 * bb['mag']) or (g15 is not None and g15 >= 0.20)
            mass = v['pct'] >= 0.35
            if grew: C += 1
            if mass: D += 1
            if grew or mass: E += 1
        for key, n in zip(out.keys(), (A, B, C, D, E)):
            out[key].append(n)
    return out

def new_at(bars, born, k, hhmm, defn):
    """is strike k NEW at hh:mm under definition defn?"""
    sec = hhmm_to_sec(hhmm)
    i = min(range(len(bars)), key=lambda j: abs(bars[j]['sec'] - sec))
    b = bars[i]; v = b['rows'].get(float(k))
    if not v or v['pct'] < THR: return 'not a node (%s)' % ('absent' if not v else '%d%%' % round(100 * v['pct']))
    bb = born.get(float(k))
    if defn == 'A':
        first_i = next(j for j in range(i + 1) if float(k) in bars[j]['rows'] and bars[j]['rows'][float(k)]['pct'] >= THR)
        return 'NEW %db' % (i - first_i) if i - first_i <= NEW_BARS else 'no'
    if not bb or i < bb['i']: return 'no (opening book / no crossing seen)'
    age = i - bb['i']
    if age > NEW_BARS: return 'no (age %db)' % age
    mag = abs(v['cur']); g15 = None
    if isinstance(v['d15'], (int, float)):
        dm = v['d15'] if v['cur'] >= 0 else -v['d15']; base = mag - dm
        if base > 0: g15 = dm / base
    grew = (mag >= 2 * bb['mag']) or (g15 is not None and g15 >= 0.20)
    mass = v['pct'] >= 0.35
    tag = 'NEW %db · born %s at %d%% · now %d%% · x%.1f since birth · g15 %s' % (
        age, ct(bb['t']), round(100 * bb['pct']), round(100 * v['pct']), mag / bb['mag'] if bb['mag'] else 0,
        ('%+d%%' % round(100 * g15)) if g15 is not None else '—')
    if defn == 'B': return tag
    if defn == 'C': return tag if grew else 'no (did not grow: ' + tag + ')'
    if defn == 'D': return tag if mass else 'no (no mass: ' + tag + ')'
    if defn == 'E': return tag if (grew or mass) else 'no (' + tag + ')'
    return tag

# ---------------------------------------------------------------- STACKS ------------------------------------
def runs(nodes, step=5.0):
    """runs of same-sign nodes on adjacent strikes; nodes = [(k, cur, pct)] sorted desc by k"""
    out, run = [], []
    for n in nodes:
        if run and abs(run[-1][0] - n[0]) <= step + 0.01 and (run[-1][1] >= 0) == (n[1] >= 0):
            run.append(n)
        else:
            if len(run) >= 2: out.append(run)
            run = [n]
    if len(run) >= 2: out.append(run)
    return out

# Each definition is (member cut, run test): runs are built from the nodes at or above the MEMBER CUT (a node under
# it is not a member and BREAKS the run), then the test decides whether the run is a stack.
# ⚠ S2 vs S6 is the difference that matters. S2 builds runs from every node (>= 20%) and asks that ALL of them clear
# 30% — so one thin neighbour kills the King's own cloud (09-03: no stack on 66 of 87 bars). S6 builds the run from
# the >= 30% members only, which is what "a dense cluster of LARGE nodes" means. S6 IS WHAT v15.64 SHIPS
# (GRID_STACK_MIN_PCT=30 as the member cut, GRID_STACK_MAX_PCT=40 on the biggest member).
STACK_DEFS = {
    'S1_current':      (THR,  lambda run: max(p for _, _, p in run) >= 0.40),
    'S2_each>=30%':    (THR,  lambda run: min(p for _, _, p in run) >= 0.30),
    'S3_sum>=100%':    (THR,  lambda run: sum(p for _, _, p in run) >= 1.00),
    'S4_each>=30%+sum>=80%': (THR, lambda run: min(p for _, _, p in run) >= 0.30 and sum(p for _, _, p in run) >= 0.80),
    'S5_each>=25%+max>=50%': (THR, lambda run: min(p for _, _, p in run) >= 0.25 and max(p for _, _, p in run) >= 0.50),
    'S6_members>=30%+max>=40%': (0.30, lambda run: max(p for _, _, p in run) >= 0.40),
}

def _runs_for(b, cut):
    nodes = sorted([(k, v['cur'], v['pct']) for k, v in b['rows'].items() if v['pct'] >= cut], key=lambda x: -x[0])
    return runs(nodes), nodes

def stacks_per_bar(bars):
    out = {k: [] for k in STACK_DEFS}
    for b in bars:
        for name, (cut, f) in STACK_DEFS.items():
            rs, _ = _runs_for(b, cut)
            out[name].append(sum(1 for r in rs if f(r)))
    return out

def stacks_at(bars, hhmm):
    sec = hhmm_to_sec(hhmm)
    i = min(range(len(bars)), key=lambda j: abs(bars[j]['sec'] - sec))
    b = bars[i]
    _, nodes = _runs_for(b, THR)
    res = {}
    for name, (cut, f) in STACK_DEFS.items():
        rs, _ = _runs_for(b, cut)
        res[name] = ['%s%s %s' % ('+' if r[0][1] >= 0 else '−', '/'.join(str(int(k)) for k, _, _ in r),
                                   '(' + ' '.join('%d%%' % round(100 * p) for _, _, p in r) + ')') for r in rs if f(r)]
    return ct(b['t']), res, nodes

def main():
    print('THR %d%% of the King · NEW window %d bars' % (round(100 * THR), NEW_BARS))
    NEW_EX = [  # (day, strike, hh:mm, what he circled)
        ('2026-09-03', 7755, '13:00', 'E001 R3 — the session high printed at it (born 12:48)'),
        ('2026-08-31', 7670, '09:27', 'E002 c1 — the LOD stack (born 09:21)'),
        ('2026-08-31', 7665, '09:27', 'E002 c1 — the LOD stack (born 09:27)'),
        ('2026-08-28', 7775, '10:00', 'E003 c2 — the HOD at a fresh node (born 09:33)'),
        ('2026-08-28', 7720, '13:12', 'E003 c4 — the 13:12 high (born 12:39)'),
        ('2026-08-28', 7705, '14:00', 'E003 c5 — the 14:00 low (appeared 13:42)'),
        ('2026-08-27', 7700, '08:57', 'E004 c1 — the morning support (fresh 08:45–08:57)'),
    ]
    STACK_EX = [
        ('2026-09-03', '12:36', 'E001 Pb — 7740 + 7735 +γ, the pullback floor'),
        ('2026-09-03', '12:48', 'the v3b mockup — SPX PIKA STACK 7740/7735'),
        ('2026-08-31', '09:27', 'E002 c1 — 7665/7670/7675 −γ, the LOD'),
        ('2026-08-27', '12:10', 'E004 c3 — 7745 + 7750 +γ, the session high'),
    ]
    data = {d: load(d) for d in DAYS}
    print('\n== NEW — rows flagged per bar (median over the day · at 10:00 · at 13:00) ==')
    for day, bars in data.items():
        born = births(bars)
        nd = new_defs(bars, born)
        def at(hh):
            sec = hhmm_to_sec(hh); i = min(range(len(bars)), key=lambda j: abs(bars[j]['sec'] - sec)); return i
        i10, i13 = at('10:00'), at('13:00')
        nodes_med = statistics.median([sum(1 for v in b['rows'].values() if v['pct'] >= THR) for b in bars])
        print('%s  nodes/bar median %d  births %d' % (day, nodes_med, len(born)))
        for name, series in nd.items():
            print('   %-22s median %4.1f   10:00 %2d   13:00 %2d' % (name, statistics.median(series), series[i10], series[i13]))
    print('\n== NEW — his circled legs, under each definition ==')
    for day, k, hh, what in NEW_EX:
        bars = data[day]; born = births(bars)
        print('%s %s %s — %s' % (day, hh, k, what))
        for defn in 'ABCDE':
            print('   %s: %s' % (defn, new_at(bars, born, k, hh, defn)))
    print('\n== STACKS — runs flagged per bar (median · max) ==')
    for day, bars in data.items():
        sp = stacks_per_bar(bars)
        print(day)
        for name, series in sp.items():
            print('   %-24s median %3.1f  max %d  share of bars with >=1: %d%%' % (name, statistics.median(series), max(series), round(100 * sum(1 for x in series if x) / len(series))))
    print('\n== STACKS — his examples ==')
    for day, hh, what in STACK_EX:
        t, res, nodes = stacks_at(data[day], hh)
        print('%s %s (bar %s) — %s' % (day, hh, t, what))
        print('   nodes >= thr: ' + ' '.join('%d:%s%d%%' % (k, '+' if c >= 0 else '−', round(100 * p)) for k, c, p in nodes))
        for name, lst in res.items():
            print('   %-24s %s' % (name, ' · '.join(lst) if lst else '—'))

if __name__ == '__main__':
    main()
