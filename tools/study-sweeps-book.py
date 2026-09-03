#!/usr/bin/env python3
"""
SWEEPS AND THE BOOK — the part the price-only corpus cannot see. Reads data/<day>.json (the panel's own
day export), where every 3-minute bar carries the book beside the price: the King, the top nodes
(tri.SPY.top), the InsiderFinance walls (lev.cr0/ps0/cr/ps), and the bar's own h / l / close.

    "good point, about CW0, PW0 etc.. can you add them"  - operator, 2026-09-03

TWO QUESTIONS, ONE UNIT (a 3-minute SPY bar, SPY scale — no ratio, no conversion):
  A · SWEEPS OF THE BOOK'S LEVELS. CW0 / PW0 / CW / PW / KING treated as levels, valued AS OF the sweep bar
      and frozen for the reclaim. Side by position against the session open (a level below the open is a
      low-side level whatever its name).
  B · SWEEP × NODE (H6). Every price-level sweep-reclaim (PDL/PDH/PDC/IB) is tagged with whether its
      extremum sat inside Skylit's tap zone (±0.50 SPY, doctrine C2) of a top-5 node or the King at that
      bar, or of a wall. Printed-the-extreme rate AT a node vs NOT at a node — the control is the same
      kind of sweep that landed at nothing.

DEFINITIONS (written before the first number, 2026-09-03; the corpus study's, rescaled to 3-minute SPY bars):
  session          the snaps of one day file, >= 100 bars with h/l. Bars are 3 minutes; 10 bars = 30 min.
  PDH/PDL/PDC      the previous session's max h / min l / last close (the previous day FILE).
  IBH/IBL          the first 20 bars (60 min); IB sweeps are searched after bar 20.
  sweep            the first bar whose l prints strictly below the level (high side mirrored).
  reclaim          the first later bar within 10 whose CLOSE is back inside; none = ACCEPTED (counted, not scored).
  extremum         the lowest l from the sweep bar to the reclaim bar.
  printed the LOD  the extremum equals the session's final low.
  at a node        |extremum − strike| <= 0.50 for any top-5 strike or the King AT THE SWEEP BAR.
  fresh-low control  standing extreme printed within the last 10 bars at the same bar index, all sessions.
  bins             depth shallow <= 0.30 / deep > 0.80 (≈ 3 / 8 ES points); reclaim poke <= 2 bars, flush 3–10.
ONH/ONL are NOT here: SPY has no overnight session. They stay in the ES study.

usage: python3 tools/study-sweeps-book.py [--json data/es-1min/SWEEPS-BOOK.json] [--selftest]
"""
import io, json, glob, math, os, sys

ZONE = 0.50
RECLAIM_MAX = 10
IB_BARS = 20
MIN_BARS = 100
DEPTH_SHALLOW, DEPTH_DEEP = 0.30, 0.80
POKE_MAX = 2


def wilson(k, n, z=1.96):
    if n <= 0:
        return (0.0, 0.0)
    p = k / n; den = 1 + z*z/n
    c = (p + z*z/(2*n)) / den; h = z*math.sqrt(p*(1-p)/n + z*z/(4*n*n)) / den
    return (max(0.0, c-h), min(1.0, c+h))


def load_sessions(paths):
    """-> [(date, bars)], bars = [dict(h,l,c,king,top5,cr0,ps0,cr,ps)] with the book carried forward bar to bar."""
    out = []
    for p in paths:
        try:
            D = json.load(io.open(p, encoding='utf-8'))
        except Exception:
            continue
        sn = ((D.get('snaps') or {}).get('SPY')) or []
        bars = []; last = dict(king=None, top5=[], cr0=None, ps0=None, cr=None, ps=None, hvl=None, mag=None)
        for s in sn:
            if s.get('h') is None or s.get('l') is None or s.get('px') is None:
                continue
            b = dict(h=float(s['h']), l=float(s['l']), c=float(s['px']))
            if isinstance(s.get('king'), (int, float)): last['king'] = float(s['king'])
            tri = ((s.get('tri') or {}).get('SPY') or {}).get('top')
            if isinstance(tri, list) and tri:
                top = sorted([t for t in tri if isinstance(t, list) and len(t) >= 2 and isinstance(t[0], (int, float))], key=lambda t: -abs(t[1]))
                last['top5'] = [float(t[0]) for t in top[:5]]
            lev = s.get('lev') or {}
            for k in ('cr0', 'ps0', 'cr', 'ps', 'hvl', 'mag'):
                if isinstance(lev.get(k), (int, float)): last[k] = float(lev[k])
            b.update(last); b['top5'] = list(last['top5'])
            bars.append(b)
        if len(bars) >= MIN_BARS:
            out.append((D.get('date') or os.path.basename(p)[:10], bars))
    out.sort(key=lambda x: x[0])
    return out


def fresh_control(sessions):
    base = {0: {}, 1: {}}
    for d, bars in sessions:
        fl = min(b['l'] for b in bars); fh = max(b['h'] for b in bars)
        rl, rh = 1e18, -1e18; tl = th = 0
        for i, b in enumerate(bars):
            if b['l'] < rl: rl, tl = b['l'], i
            if b['h'] > rh: rh, th = b['h'], i
            if i-tl <= RECLAIM_MAX: base[0].setdefault(i, []).append(1 if rl <= fl+1e-9 else 0)
            if i-th <= RECLAIM_MAX: base[1].setdefault(i, []).append(1 if rh >= fh-1e-9 else 0)
    return {s: {i: sum(v)/len(v) for i, v in m.items()} for s, m in base.items()}


def scan(bars, level, low, start=0):
    for i in range(start, len(bars)):
        b = bars[i]
        if (b['l'] < level) if low else (b['h'] > level):
            ext = b['l'] if low else b['h']
            for j in range(i+1, min(len(bars), i+1+RECLAIM_MAX)):
                bj = bars[j]
                ext = min(ext, bj['l']) if low else max(ext, bj['h'])
                if (bj['c'] > level) if low else (bj['c'] < level):
                    return dict(sweep=i, reclaim=j, ext=ext)
            return dict(sweep=i, reclaim=None, ext=ext)
    return None


def near(px, strikes):
    return [s for s in strikes if s is not None and abs(px - s) <= ZONE + 1e-9]


def run(paths=None):
    paths = paths or sorted(glob.glob(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data', '2026-*.json')))
    S = load_sessions(paths)
    if not S:
        return dict(corpus=dict(sessions=0), cells=[], lookup=dict(level={}, node={}, sessions=0), events=[], ledger=dict(cells_read=0))
    fresh = fresh_control(S)
    events = []; prev = None; ledger = 0
    for d, bars in S:
        fl = min(b['l'] for b in bars); fh = max(b['h'] for b in bars); op = bars[0]['c']
        ibh = max(b['h'] for b in bars[:IB_BARS]); ibl = min(b['l'] for b in bars[:IB_BARS])
        levels = [('IBL', ibl, True, IB_BARS, 'price'), ('IBH', ibh, False, IB_BARS, 'price')]
        if prev is not None:
            levels += [('PDL', prev[0], True, 0, 'price'), ('PDH', prev[1], False, 0, 'price'), ('PDC-', prev[2], True, 0, 'price'), ('PDC+', prev[2], False, 0, 'price')]
        # book levels: the value AS OF each bar; the first bar that prints through it defines the event and freezes the level
        for name, key in (('CW0', 'cr0'), ('PW0', 'ps0'), ('CW', 'cr'), ('PW', 'ps'), ('KING', 'king'), ('HVL', 'hvl'), ('MAG', 'mag')):   # (v15.57) zero-gamma and the magnet
            ev = None
            for i, b in enumerate(bars):
                lv = b.get(key)
                if lv is None: continue
                low = lv < op
                if (b['l'] < lv) if low else (b['h'] > lv):
                    r = scan(bars, lv, low, i)
                    if r: ev = (name + ('-' if low else '+'), lv, low, r)
                    break
            if ev:
                nm, lv, low, r = ev
                printed = None if r['reclaim'] is None else (1 if ((r['ext'] <= fl+1e-9) if low else (r['ext'] >= fh-1e-9)) else 0)
                events.append(dict(d=d, level=nm, kind='book', side='LOD' if low else 'HOD', px=lv, sweep=r['sweep'], reclaim=r['reclaim'], ext=r['ext'],
                                   depth=abs(lv-r['ext']), speed=(r['reclaim']-r['sweep']) if r['reclaim'] is not None else None,
                                   printed=printed, fresh=(fresh[0 if low else 1].get(r['reclaim']) if r['reclaim'] is not None else None),
                                   atNode=None, atKing=None, atWall=None))
        for name, lv, low, start, kind in levels:
            r = scan(bars, lv, low, start)
            if not r: continue
            b0 = bars[r['sweep']]
            printed = None if r['reclaim'] is None else (1 if ((r['ext'] <= fl+1e-9) if low else (r['ext'] >= fh-1e-9)) else 0)
            at_top = near(r['ext'], b0['top5']); at_king = near(r['ext'], [b0.get('king')]); at_wall = near(r['ext'], [b0.get('cr0'), b0.get('ps0'), b0.get('cr'), b0.get('ps'), b0.get('hvl'), b0.get('mag')])
            events.append(dict(d=d, level=name, kind='price', side='LOD' if low else 'HOD', px=lv, sweep=r['sweep'], reclaim=r['reclaim'], ext=r['ext'],
                               depth=abs(lv-r['ext']), speed=(r['reclaim']-r['sweep']) if r['reclaim'] is not None else None,
                               printed=printed, fresh=(fresh[0 if low else 1].get(r['reclaim']) if r['reclaim'] is not None else None),
                               atNode=bool(at_top or at_king), atTop5=at_top[:1], atKing=bool(at_king), atWall=bool(at_wall)))
        prev = (fl, fh, bars[-1]['c'])

    def cell(rows, label):
        nonlocal ledger
        ledger += 1
        sc = [r for r in rows if r['printed'] is not None]
        n = len(sc); k = sum(r['printed'] for r in sc); lo, hi = wilson(k, n)
        fr = [r['fresh'] for r in sc if r['fresh'] is not None]
        return dict(label=label, events=len(rows), reclaimed=n, accepted=len(rows)-n, printed=k, rate=(k/n if n else None), ci=[lo, hi],
                    fresh=(sum(fr)/len(fr) if fr else None), lift_fresh=((k/n - sum(fr)/len(fr)) if (n and fr) else None), n=n)
    cells = []
    LV = ['PDL', 'PDH', 'PDC-', 'PDC+', 'IBL', 'IBH', 'PW0-', 'PW0+', 'CW0-', 'CW0+', 'PW-', 'PW+', 'CW-', 'CW+', 'KING-', 'KING+', 'HVL-', 'HVL+', 'MAG-', 'MAG+']
    for lv in LV:
        cells.append(cell([e for e in events if e['level'] == lv], lv))
    price = [e for e in events if e['kind'] == 'price']
    cells.append(cell([e for e in price if e['atNode']], 'price sweep AT a top-5 node or the King (±%.2f)' % ZONE))
    cells.append(cell([e for e in price if e['atNode'] is False], 'price sweep NOT at a node'))
    cells.append(cell([e for e in price if e['atKing']], 'price sweep AT the King'))
    cells.append(cell([e for e in price if e['atWall']], 'price sweep AT a wall (CW0/PW0/CW/PW)'))
    byl = {c['label']: c for c in cells}
    slim = lambda c: dict(rate=c['rate'], n=c['n'], fresh=c['fresh'], ci=c['ci'], lift=c['lift_fresh'], accepted=c['accepted'], events=c['events'])
    out = dict(corpus=dict(sessions=len(S), first=S[0][0], last=S[-1][0], unit='SPY 3-minute bars from data/<day>.json snaps', zone=ZONE, reclaimMaxBars=RECLAIM_MAX),
               cells=cells,
               lookup=dict(level={lv: slim(byl[lv]) for lv in LV},
                           node=dict(atNode=slim(byl['price sweep AT a top-5 node or the King (±%.2f)' % ZONE]), notAtNode=slim(byl['price sweep NOT at a node']),
                                     atKing=slim(byl['price sweep AT the King']), atWall=slim(byl['price sweep AT a wall (CW0/PW0/CW/PW)'])),
                           bins=dict(speedPokeMaxBars=POKE_MAX, depthShallowMaxPts=DEPTH_SHALLOW, depthDeepMinPts=DEPTH_DEEP, reclaimMaxBars=RECLAIM_MAX, zone=ZONE),
                           sessions=len(S), first=S[0][0], last=S[-1][0]),
               ledger=dict(cells_read=ledger, note='book corpus: every session the panel exported with the book per bar; thin by construction until the exports accumulate'),
               events=events)
    return out


def fmt(out):
    L = ['SWEEPS × THE BOOK · %d sessions · %s → %s · %s' % (out['corpus']['sessions'], out['corpus'].get('first'), out['corpus'].get('last'), out['corpus'].get('unit', ''))]
    L.append('%-46s %6s %6s %8s %14s %7s %7s' % ('cell', 'events', 'accept', 'printed', 'rate [95%]', 'fresh', 'lift'))
    for c in out['cells']:
        if c['rate'] is None:
            L.append('%-46s %6d %6d %8s' % (c['label'], c['events'], c['accepted'], 'thin (n=%d)' % c['n'])); continue
        L.append('%-46s %6d %6d %8d %5.0f%% [%2.0f-%2.0f] %6s %7s' % (c['label'], c['events'], c['accepted'], c['printed'], 100*c['rate'], 100*c['ci'][0], 100*c['ci'][1],
                 ('%.0f%%' % (100*c['fresh'])) if c['fresh'] is not None else '—', ('%+.0fpp' % (100*c['lift_fresh'])) if c['lift_fresh'] is not None else '—'))
    L.append('ledger: %d cells read · %s' % (out['ledger']['cells_read'], out['ledger']['note']))
    return '\n'.join(L)


def selftest():
    """A planted day: the PDL is swept at bar 30 and the extremum sits on a top-5 strike; the reclaim is the LOD."""
    import tempfile
    def snap(i, h, l, c, king, top, lev):
        return dict(t=1788183000000 + i*180000, bar=i, px=c, h=h, l=l, king=king, tri=dict(SPY=dict(top=top)), lev=lev)
    def day(date, bars):
        return dict(schema='gex-day-export/v1', date=date, snaps=dict(SPY=bars))
    top = [[760, 100], [765, -80], [770, 60], [772, 40], [775, 30]]
    lev = dict(cr0=770, ps0=760, cr=772, ps=758, hvl=763.4, mag=765)
    prev = [snap(i, 767 + 0.01*i, 766 + 0.01*i, 766.5 + 0.01*i, 765, top, lev) for i in range(130)]   # PDL = 766.00
    cur = []
    for i in range(130):
        if i < 30: cur.append(snap(i, 768, 767, 767.5, 765, top, lev))
        elif i == 30: cur.append(snap(i, 767, 764.8, 765.5, 765, top, lev))     # sweeps PDL 766 down to 764.8 -> through the 765 King, within its zone
        elif i < 35: cur.append(snap(i, 765.8, 765.3, 765.6, 765, top, lev))
        elif i == 35: cur.append(snap(i, 766.6, 765.9, 766.4, 765, top, lev))   # close back above 766 -> reclaim
        else: cur.append(snap(i, 770 + 0.02*i, 767 + 0.02*i, 769 + 0.02*i, 765, top, lev))
    tmp = tempfile.mkdtemp()
    p1 = os.path.join(tmp, '2026-09-01.json'); p2 = os.path.join(tmp, '2026-09-02.json')
    io.open(p1, 'w').write(json.dumps(day('2026-09-01', prev))); io.open(p2, 'w').write(json.dumps(day('2026-09-02', cur)))
    out = run([p1, p2])
    ev = [e for e in out['events'] if e['level'] == 'PDL']
    ok1 = len(ev) == 1 and ev[0]['sweep'] == 30 and ev[0]['reclaim'] == 35 and ev[0]['printed'] == 1 and ev[0]['atNode'] is True and ev[0]['atKing'] is True
    kk = [e for e in out['events'] if e['level'].startswith('KING')]
    ok2 = len(kk) == 1 and kk[0]['level'] == 'KING-' and kk[0]['printed'] == 1
    print('SELFTEST  PDL sweep at a node found and scored: %s   KING- sweep found: %s' % (ok1, ok2))
    return ok1 and ok2


if __name__ == '__main__':
    if '--selftest' in sys.argv:
        sys.exit(0 if selftest() else 1)
    out = run()
    print(fmt(out))
    if '--json' in sys.argv:
        jp = sys.argv[sys.argv.index('--json')+1]
        slim = dict(out); slim.pop('events')
        io.open(jp, 'w', encoding='utf-8').write(json.dumps(slim, indent=1))
        print('wrote', jp)
