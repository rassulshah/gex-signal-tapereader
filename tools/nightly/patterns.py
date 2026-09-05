#!/usr/bin/env python3
"""patterns.py — score the setups and patterns (v15.67): the held rate by pattern class from the deflection ledger
across every recorded day file, the same classes and the same arithmetic as the panel's patternTable().

    python3 tools/nightly/patterns.py                    # the table over every data/<day>.json
    python3 tools/nightly/patterns.py --from 2026-09-03  # days >= from
    python3 tools/nightly/patterns.py --json events.json # a fixture: {"events":[...]} → the table as JSON (the panel test pins this equal)
    python3 tools/nightly/patterns.py --selftest

Every tap in data/<day>.json → defl → <sym> → [...] carries (from v15.67) `pat` — what the PATTERN columns showed at
that strike at the moment of the tap, per book: { spx, spy, qqq } each null (book unreadable) · {node:false} · {node:true,
k, pct, pos, st:'pika'|'barney'|None, mem, rug:'rug'|'rrug'|None, nw:{...}|None (SPX), g:growth% (SPX)} — plus `kings`
(the books whose King the tap touched), `name` (the OLD node-map detector's call), `dir` and `cont` (1 held / 0 broke /
None pending). A tap counts in EVERY class it matches. A rate prints at n >= RATE_MIN_N with its Wilson lower bound.
⚠ The class list and the arithmetic are duplicated in the panel (PAT_CLASSES · tapClasses · patternTable · wilsonLow);
test_v1567.js runs both on one fixture and fails when they differ — change both or neither.

THE OBJECTIVE OUTCOMES (v15.69) — written here BEFORE the first number was read (2026-09-04). PURPOSE.md names two
decisions at a deflection: TURN (the node is the HOD / LOD — turn around) and STAY IN (the pullback ends, the trend
resumes). "Held ten bars" is neither. So every tap is also scored, from the day's own SPY bars (the snaps' h / l per
3-minute bar), at the close:
    TURN    for an UP deflection (dir > 0, a floor held): the lowest low in the tap window (the four bars before the
            event's time and one after) is within TURN_TOL = 0.50 SPY (the doctrine's deflection zone) of the SESSION
            low — the node WAS the LOD. Mirror for DOWN (the session high). 1 / 0.
    RESUME  after the tap, price printed a HIGHER high than any high before it (UP) — the trend resumed to a new
            extreme; LOWER low for DOWN. 1 / 0. (A LOD deflection followed by new highs scores 1 on both.)
    EPISODES  only the FIRST tap per (strike, dir) of a day carries the two outcomes; a retest of the same node gets
            None — the LOD would otherwise be counted once per retest. `held` stays per tap, as before.
    None    when the day's record is thin (< 20 bars) or the event has no time — never a guess.
The nightly aggregates them per class beside `held` (turn / resume: n · hit · rate · Wilson low); the live panel table
has no bars of the whole day and shows `held` only. Known confound, recorded not solved: an early tap has more
session ahead of it to be undercut; the clock is a class of its own to add.
"""
import glob, io, json, math, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RATE_MIN_N = 15
PAT_GROW_PCT = 20
PAT_CLASSES = [
    ('all', 'every tap'),
    ('king:SPX', 'King · SPX'), ('king:SPY', 'King · SPY'), ('king:QQQ', 'King · QQQ'), ('king:any', 'King · any book'), ('king:none', 'no King at the tap'),
    ('king:floor', 'King as a floor (deflected UP)'), ('king:ceil', 'King as a ceiling (deflected DOWN)'), ('king:grow', 'SPX King growing into the tap'), ('king:fade', 'SPX King fading into the tap'), ('king:pos', 'SPX King +γ'), ('king:neg', 'SPX King −γ'),
    ('spx:pika', 'SPX pika stack (named or member)'), ('spx:barney', 'SPX barney stack (named or member)'), ('spy:pika', 'SPY pika stack'), ('spy:barney', 'SPY barney stack'), ('qqq:pika', 'QQQ pika stack'), ('qqq:barney', 'QQQ barney stack'),
    ('spx:rug', 'SPX rug — the yellow over the purple'), ('spx:rrug', 'SPX reverse rug — the yellow under the purple'), ('spy:rug', 'SPY rug'), ('spy:rrug', 'SPY reverse rug'), ('qqq:rug', 'QQQ rug'), ('qqq:rrug', 'QQQ reverse rug'),
    ('spx:new', 'SPX node NEW at the tap'), ('spx:grow', 'SPX node growing into the tap (≥ %d%%)' % PAT_GROW_PCT), ('spx:fade', 'SPX node fading into the tap (≤ −%d%%)' % PAT_GROW_PCT),
    ('spx:pos', 'SPX +γ node (yellow)'), ('spx:neg', 'SPX −γ node (purple)'), ('spx:none', 'no SPX node at the tap (SPY / QQQ level)'),
    ('old:King', 'old detector · King'), ('old:Gate', 'old detector · Gate'), ('old:Rug', 'old detector · Rug'), ('old:Reverse Rug', 'old detector · Reverse Rug'), ('old:Pika', 'old detector · Pika'), ('old:Barney', 'old detector · Barney'), ('old:Floor', 'old detector · Floor'), ('old:Ceiling', 'old detector · Ceiling'),
    ('dir:up', 'deflected UP (a floor held)'), ('dir:dn', 'deflected DOWN (a ceiling held)'),
]

def wilson_low(right, n, z=1.96):
    if not n or n <= 0:
        return 0.0
    p = right / n; d = 1 + z * z / n; c = p + z * z / (2 * n); m = z * math.sqrt((p * (1 - p) + z * z / (4 * n)) / n)
    return max(0.0, (c - m) / d)

def tap_classes(e):
    c = ['all']
    Pt = e.get('pat') or None
    # the King classes: `kings` exists from v15.63 but was [] on every tap until v15.67 fixed the scale (the panel's
    # tapDisp), so "no King at the tap" is only claimed for a STAMPED tap — an older tap says nothing about the King
    K = e.get('kings') or []
    if K:
        for b in K:
            c.append('king:' + str(b))
        c.append('king:any')
        d0 = e.get('dir')
        if isinstance(d0, (int, float)) and d0 > 0:
            c.append('king:floor')
        elif isinstance(d0, (int, float)) and d0 < 0:
            c.append('king:ceil')
        # (v15.68) the SPX King's own condition at the tap — S0.5 (growing into the tap) and S0.7 (polarity) read these
        ks = (Pt or {}).get('spx') if ('SPX' in K and Pt) else None
        if ks and ks.get('node') is True:
            g0 = ks.get('g')
            if isinstance(g0, (int, float)) and not isinstance(g0, bool):
                if g0 >= PAT_GROW_PCT:
                    c.append('king:grow')
                elif g0 <= -PAT_GROW_PCT:
                    c.append('king:fade')
            c.append('king:pos' if ks.get('pos') else 'king:neg')
    elif Pt:
        c.append('king:none')
    if Pt:
        for b in ('spx', 'spy', 'qqq'):
            v = Pt.get(b)
            if not v:
                continue
            if v.get('node') is False:
                if b == 'spx':
                    c.append('spx:none')
                continue
            if v.get('st'):
                c.append(b + ':' + v['st'])
            if v.get('rug'):
                c.append(b + ':' + v['rug'])
            if b == 'spx':
                if v.get('nw'):
                    c.append('spx:new')
                g = v.get('g')
                if isinstance(g, (int, float)) and not isinstance(g, bool):
                    if g >= PAT_GROW_PCT:
                        c.append('spx:grow')
                    elif g <= -PAT_GROW_PCT:
                        c.append('spx:fade')
                c.append('spx:pos' if v.get('pos') else 'spx:neg')
    nm = str(e.get('name') or '')
    nm = re.sub(r'^⭑\s*', '', nm); nm = re.sub(r'\s*\(BO·FT retest\)$', '', nm); nm = re.sub(r' deflection$', '', nm)
    if nm:
        c.append('old:' + nm)
    d = e.get('dir')
    if isinstance(d, (int, float)) and d > 0:
        c.append('dir:up')
    elif isinstance(d, (int, float)) and d < 0:
        c.append('dir:dn')
    return c

def pattern_table(events):
    by = {k: dict(key=k, label=l, n=0, held=0, broke=0, pending=0, rate=None, lo=None) for k, l in PAT_CLASSES}
    stamped = 0; total = 0
    for e in events or []:
        if not e:
            continue
        total += 1
        if e.get('pat'):
            stamped += 1
        for k in tap_classes(e):
            r = by.get(k)
            if r is None:
                continue
            if e.get('cont') == 1:
                r['held'] += 1; r['n'] += 1
            elif e.get('cont') == 0:
                r['broke'] += 1; r['n'] += 1
            else:
                r['pending'] += 1
    rows = []
    for k, _ in PAT_CLASSES:
        r = by[k]
        if not (r['n'] + r['pending']):
            continue
        if r['n']:
            r['rate'] = int(round(100.0 * r['held'] / r['n']))
            r['lo'] = int(round(100 * wilson_low(r['held'], r['n'])))
        rows.append(r)
    return dict(rows=rows, events=total, stamped=stamped)

def _js_round(x):
    # JavaScript's Math.round rounds .5 toward +∞; Python's round() is banker's — the panel is the reference
    return int(math.floor(x + 0.5))

def pattern_table_js(events):
    """the same table with JavaScript's rounding, so the pin against the panel holds on a .5"""
    T = pattern_table(events)
    for r in T['rows']:
        if r['n']:
            r['rate'] = _js_round(100.0 * r['held'] / r['n'])
            r['lo'] = _js_round(100 * wilson_low(r['held'], r['n']))
    return T

TURN_TOL = 0.5          # SPY points — the doctrine's deflection zone (±0.50 SPY)
BAR_MS = 180000         # a 3-minute bar
MIN_BARS = 20           # a day with fewer recorded bars scores nothing
# (v15.70) THE MARKET'S NUMBERS COME FROM learning/markets.json — one place for every market-specific number, so a new
# market is a configuration entry (operator: "if i wanted to start doing this for gold"). The constants above are the
# fallback when the file is absent; test_v1570 pins them equal to the ledger market's entry.
def market_config(root=ROOT):
    try:
        M = json.load(io.open(os.path.join(root, 'learning', 'markets.json'), encoding='utf-8'))
        m = (M.get('markets') or {}).get(M.get('ledgerMarket') or 'SPY') or {}
        return dict(turnTol=float(m.get('turnTolPts') or TURN_TOL), barMs=int(m.get('barMinutes') or 3) * 60000, minBars=int(m.get('minBarsDay') or MIN_BARS), market=M.get('ledgerMarket') or 'SPY')
    except Exception:
        return dict(turnTol=TURN_TOL, barMs=BAR_MS, minBars=MIN_BARS, market='SPY')
try:
    _MC = market_config()
    TURN_TOL, BAR_MS, MIN_BARS = _MC['turnTol'], _MC['barMs'], _MC['minBars']
except Exception:
    pass

def price_series(D, sym='SPY'):
    """[(bar_ms, high, low)] from the day's snaps, sorted, bars with a high and a low only"""
    out = []
    for s in ((D.get('snaps') or {}).get(sym) or []):
        h, l = s.get('h'), s.get('l'); b = s.get('bar') or s.get('t')
        if isinstance(h, (int, float)) and isinstance(l, (int, float)) and isinstance(b, (int, float)):
            out.append((b, float(h), float(l)))
    out.sort()
    return out

def outcomes(D, sym='SPY'):
    """stamp every defl event of the day with `_turn` / `_resume` (1 / 0 / None) from the day's own bars — see the docstring"""
    ev = ((D.get('defl') or {}).get(sym) or [])
    ser = price_series(D, sym)
    seen = set()
    for e in ev:
        e['_turn'] = None; e['_resume'] = None
    if len(ser) < MIN_BARS:
        return ev
    sess_hi = max(h for _, h, _l in ser); sess_lo = min(l for _, _h, l in ser)
    for e in sorted(ev, key=lambda x: (x.get('t') or 0)):
        t = e.get('t'); d = e.get('dir'); k = e.get('strike')
        if not isinstance(t, (int, float)) or d not in (1, -1) or not isinstance(k, (int, float)):
            continue
        key = (round(k, 2), d)
        if key in seen:
            continue                      # a retest of the same node: the outcome was counted on the first tap
        seen.add(key)
        w0, w1 = t - 4 * BAR_MS, t + BAR_MS
        win = [x for x in ser if w0 <= x[0] <= w1]
        if not win:
            continue
        t_tap = win[0][0]
        before = [x for x in ser if x[0] < t_tap]; after = [x for x in ser if x[0] >= t_tap]
        if d > 0:
            tap_lo = min(l for _, _h, l in win)
            e['_turn'] = 1 if (tap_lo - sess_lo) <= TURN_TOL else 0
            pre_hi = max((h for _, h, _l in before), default=None); post_hi = max((h for _, h, _l in after), default=None)
            e['_resume'] = (1 if (pre_hi is not None and post_hi is not None and post_hi > pre_hi) else 0) if after else None
        else:
            tap_hi = max(h for _, h, _l in win)
            e['_turn'] = 1 if (sess_hi - tap_hi) <= TURN_TOL else 0
            pre_lo = min((l for _, _h, l in before), default=None); post_lo = min((l for _, _h, l in after), default=None)
            e['_resume'] = (1 if (pre_lo is not None and post_lo is not None and post_lo < pre_lo) else 0) if after else None
    return ev

def objective_columns(events, by_rows):
    """add turn / resume {n, hit, rate, lo} to every row of a pattern table, from the events' `_turn` / `_resume`"""
    acc = {r['key']: {'turn': [0, 0], 'resume': [0, 0]} for r in by_rows}
    for e in events or []:
        if not e:
            continue
        cls = tap_classes(e)
        for oc in ('turn', 'resume'):
            v = e.get('_' + oc)
            if v is None:
                continue
            for k in cls:
                a = acc.get(k)
                if a is None:
                    continue
                a[oc][0] += 1; a[oc][1] += (1 if v == 1 else 0)
    for r in by_rows:
        for oc in ('turn', 'resume'):
            n, hit = acc[r['key']][oc]
            r[oc] = dict(n=n, hit=hit, rate=(_js_round(100.0 * hit / n) if n else None), lo=(_js_round(100 * wilson_low(hit, n)) if n else None))
    return by_rows

def ledger_events(days=None, frm=None, root=ROOT):
    """every tap in data/<day>.json (day >= frm) across every sym → [(day, sym, event)]"""
    out = []
    files = sorted(glob.glob(os.path.join(root, 'data', '20??-??-??.json')))
    for p in files:
        d = os.path.basename(p)[:10]
        if frm and d < frm:
            continue
        if days is not None and d not in days:
            continue
        try:
            D = json.load(io.open(p, encoding='utf-8'))
        except Exception:
            continue
        for sym, arr in ((D.get('defl') or {}).items()):
            try:
                outcomes(D, sym)          # (v15.69) the objective outcomes from the day's own bars, stamped on the events
            except Exception:
                pass
            for e in arr or []:
                if e:
                    out.append((d, sym, e))
    return out

def table_for(days=None, frm=None, root=ROOT):
    ev = ledger_events(days, frm, root)
    evs = [e for _, _, e in ev]
    T = pattern_table_js(evs)
    objective_columns(evs, T['rows'])     # (v15.69) turn / resume beside held — the nightly only; the live panel has no whole day
    T['schema'] = 1; T['days'] = len(set(d for d, _, _ in ev)); T['writtenBy'] = 'tools/nightly/patterns.py'
    T['outcomes'] = dict(turnTol=TURN_TOL, market=market_config(root).get('market'), episodes='first tap per (strike, dir) per day', scored=sum(1 for e in evs if e.get('_turn') is not None))
    return T

def report(T):
    lines = ['patterns: %d taps · %d stamped · %d day%s' % (T['events'], T['stamped'], T.get('days', 0), '' if T.get('days', 0) == 1 else 's')]
    def oc(x):
        if not x or not x.get('n'):
            return '—'
        return ('%d/%d = %d%% (low %d%%)' % (x['hit'], x['n'], x['rate'], x['lo'])) if x['n'] >= RATE_MIN_N else ('%d/%d thin' % (x['hit'], x['n']))
    for r in T['rows']:
        rate = ('%3d%%  ≥%2d%%  n=%-3d' % (r['rate'], r['lo'], r['n'])) if r['n'] >= RATE_MIN_N else ('thin (n=%d)' % r['n'])
        lines.append('  %-46s held %3d  broke %3d  %s%s%s' % (r['label'], r['held'], r['broke'], rate, ('  · %d pending' % r['pending']) if r['pending'] else '',
                     ('  · turn %s · resume %s' % (oc(r.get('turn')), oc(r.get('resume')))) if ('turn' in r) else ''))
    return lines

def selftest():
    ev = [
        dict(kings=['SPX'], pat=dict(spx=dict(node=True, k=7700, pct=100, pos=True, st='pika', mem=False, rug=None, nw=dict(age=3, x=2.5, g=40), g=40), spy=dict(node=False), qqq=None), name='King deflection', dir=1, cont=1),
        dict(kings=[], pat=dict(spx=dict(node=True, k=7690, pct=45, pos=False, st='barney', mem=True, rug=None, nw=None, g=-25), spy=dict(node=True, k=770, pct=60, pos=False, st='barney', mem=False, rug=None), qqq=dict(node=True, k=567, pct=30, pos=True, st=None, mem=False, rug='rrug')), name='⭑ Ceiling deflection (BO·FT retest)', dir=-1, cont=0),
        dict(kings=['SPY', 'QQQ'], name='Gate deflection', dir=1, cont=None),   # an old tap, no stamp, pending
    ]
    T = pattern_table_js(ev)
    R = {r['key']: r for r in T['rows']}
    assert T['events'] == 3 and T['stamped'] == 2, T
    assert R['all']['n'] == 2 and R['all']['held'] == 1 and R['all']['pending'] == 1 and R['all']['rate'] == 50
    assert R['king:SPX']['held'] == 1 and R['king:any']['pending'] == 1 and R['king:none']['broke'] == 1
    assert R['king:floor']['held'] == 1 and R['king:floor']['pending'] == 1 and 'king:ceil' not in R
    assert R['king:grow']['held'] == 1 and R['king:pos']['held'] == 1 and 'king:fade' not in R and 'king:neg' not in R   # the SPX King tap: +γ, growing 40%
    assert R['spx:pika']['held'] == 1 and R['spx:barney']['broke'] == 1 and R['spy:barney']['broke'] == 1 and R['qqq:rrug']['broke'] == 1
    assert R['spx:new']['held'] == 1 and R['spx:grow']['held'] == 1 and R['spx:fade']['broke'] == 1
    assert R['spx:pos']['held'] == 1 and R['spx:neg']['broke'] == 1 and 'spx:none' not in R
    assert R['old:King']['held'] == 1 and R['old:Ceiling']['broke'] == 1 and R['old:Gate']['pending'] == 1
    assert 'king:none' in R and R['king:none']['broke'] == 1 and R['king:none']['pending'] == 0   # the unstamped third tap claims nothing about the King
    assert R['dir:up']['n'] == 1 and R['dir:dn']['n'] == 1
    assert abs(wilson_low(9, 15) - 0.3575) < 0.001, wilson_low(9, 15)   # 9 held of 15: 60%, Wilson low 36%
    assert R['all']['lo'] == _js_round(100 * wilson_low(1, 2)) == 9
    assert _js_round(2.5) == 3 and _js_round(-2.5) == -2
    # the objective outcomes on a synthetic day: 30 bars, LOD 767.5 at bar 5, HOD 773.9 at bar 22
    t0 = 1788442560000
    bars = []
    for i in range(30):
        lo = 767.5 if i == 5 else (768.2 + 0.1 * i); hi = 773.9 if i == 22 else (lo + 0.6)
        bars.append(dict(bar=t0 + i * BAR_MS, t=t0 + i * BAR_MS, px=lo + 0.3, h=hi, l=lo))
    D = dict(snaps=dict(SPY=bars), defl=dict(SPY=[
        dict(sig='a', strike=768, dir=1, t=t0 + 6 * BAR_MS, cont=1),        # tapped at the LOD (bar 5 in its window): TURN 1; new highs after: RESUME 1
        dict(sig='b', strike=770, dir=1, t=t0 + 15 * BAR_MS, cont=1),       # a floor mid-day, 2.2 above the LOD: TURN 0; new highs after (773.9): RESUME 1
        dict(sig='c', strike=768, dir=1, t=t0 + 20 * BAR_MS, cont=0),       # a RETEST of 768: None on both (episodes)
        dict(sig='d', strike=774, dir=-1, t=t0 + 23 * BAR_MS, cont=1),      # a ceiling at the HOD (bar 22 in its window): TURN 1; lower lows after? no: RESUME 0
        dict(sig='e', strike=772, dir=-1, t=t0 + 28 * BAR_MS, cont=None),   # a ceiling 2.3 under the HOD, the HOD bar outside its window: TURN 0
        dict(sig='f', strike=769, dir=1, cont=1),                            # no time: None
        dict(sig='g', strike=771, dir=1, t=t0 + 27 * BAR_MS, cont=1),       # a floor AFTER the HOD: not the LOD (0); no higher high after it: RESUME 0
    ]))
    ev = outcomes(D)
    got = {e['sig']: (e['_turn'], e['_resume']) for e in ev}
    assert got == {'a': (1, 1), 'b': (0, 1), 'c': (None, None), 'd': (1, 0), 'e': (0, 0), 'f': (None, None), 'g': (0, 0)}, got
    thin = dict(snaps=dict(SPY=bars[:10]), defl=dict(SPY=[dict(sig='a', strike=768, dir=1, t=t0 + 6 * BAR_MS, cont=1)]))
    assert outcomes(thin)[0]['_turn'] is None                                # a thin record scores nothing
    T2 = pattern_table_js(ev); objective_columns(ev, T2['rows'])
    R2 = {r['key']: r for r in T2['rows']}
    assert R2['all']['turn'] == dict(n=5, hit=2, rate=40, lo=12) and R2['all']['resume'] == dict(n=5, hit=2, rate=40, lo=12), R2['all']
    assert R2['dir:up']['turn'] == dict(n=3, hit=1, rate=33, lo=6) and R2['dir:dn']['resume'] == dict(n=2, hit=0, rate=0, lo=0)
    print('patterns.py selftest ok'); print('\n'.join(report(T)))

if __name__ == '__main__':
    a = sys.argv[1:]
    if '--selftest' in a:
        selftest()
    elif '--json' in a:
        j = json.load(io.open(a[a.index('--json') + 1], encoding='utf-8'))
        print(json.dumps(pattern_table_js(j.get('events') or []), ensure_ascii=False))
    else:
        frm = a[a.index('--from') + 1] if '--from' in a else None
        print('\n'.join(report(table_for(frm=frm))))
