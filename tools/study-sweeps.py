#!/usr/bin/env python3
"""
SWEEPS THAT PRINT THE DAY'S EXTREME - event-level, price-only, ES 1-minute, full Globex session.

    "you should have an extensive section covering types of sweeps under hod lod. For example, ONH or
     ONL sweeps leading to a LOD or HOD."  - operator, 2026-09-03

WHAT THIS IS NOT. `model-lodhod.py` already carries a bar-level `swp` feature (prior-day low swept,
close back above) inside the "is the standing extreme the day's" model, and it was REFUSED: it did
not beat the clock (0.559). That verdict stands. This file asks the trader's question instead, which
is a different unit: not "does knowing a sweep happened improve a per-bar probability" but "when a
sweep-and-reclaim EVENT happens, how often is the sweep's extremum the day's extreme, and what does
the day pay after it". One row per event, judged against the clock at the minute of the reclaim.

DEFINITIONS, written before the first number was read (2026-09-03):
  trading day d   bars stamped >= 17:00 belong to the NEXT calendar day; RTH is 08:30-15:00 CT.
  ON session      the bars of d outside RTH (17:00 prev -> 08:29). ONH/ONL = their high/low.
  PDH/PDL         the previous trading day's RTH high/low (previous session in the corpus).
  IBH/IBL         the high/low of the first 60 RTH minutes; IB sweeps are only looked for after 09:30.
  PDC             the previous session's RTH close (both sides: a sweep below it that reclaims -> LOD?,
                  above it -> HOD?). The "gap fill" level.
  POC/VAH/VAL     the previous session's RTH volume profile: 1-minute volume spread evenly across each
                  bar's range on a 1-point grid; POC = the busiest price; the value area = the 70% band
                  grown from the POC one tick at a time, heavier side first (the panel's priorProfile()).
                  VAL is a low-side level, VAH high-side, POC both.
  PMH/PML         the pre-market: the ON bars from 07:00 CT to the open. High-side / low-side.
  PWH/PWL         the previous ISO week's RTH high/low.
  OR5 / OR15      the opening range: the high/low of the first 5 / 15 RTH minutes; sweeps looked for
                  after the range completes.
  LDNH/LDNL       (v15.57) the London range: the ON bars from 02:00 CT to the open.
  VWAP, VW1/VW2   (v15.57) the session VWAP (typical price x volume, cumulative) and its +-1 / +-2 sigma
                  bands (volume-weighted sd of price about the VWAP). DYNAMIC: the level is valued at the
                  sweep bar and frozen for the reclaim; the side is the side price came FROM (a dip below the
                  VWAP from above is a low-side event, VWAP-; a spike above it from below is VWAP+). Looked
                  for from bar 5 (VWAP) / bar 30 (bands).
  DPOC/DVAH/DVAL  (v15.57) TODAY'S developing profile (1-point grid, 70% value area), valued at the sweep
                  bar; looked for from bar 30.
  sweep           the first RTH bar whose low prints strictly below the level (high side mirrored).
  reclaim         the first bar after the sweep whose CLOSE is back above the level, within 30 bars.
                  No reclaim within 30 bars = ACCEPTANCE (the level broke) - counted, not scored.
  extremum        the lowest low from the sweep bar to the reclaim bar (the sweep's wick).
  printed the LOD the extremum equals the session's final low.
  baseline        the fraction of ALL sessions whose standing low at the reclaim MINUTE is the final
                  low - the clock alone, matched minute for minute. Reported beside every rate.
  payoff          points from the extremum to (a) the session close, (b) the session's far extreme.
Buckets (sweep-bar time): 08:30-09:00 / 09:00-10:00 / 10:00-11:30 / 11:30-15:00.
Every cell read is counted in the ledger. Nothing here is pre-registered: it is a FIRST READ, and
a rate that looks good here becomes a register entry to be read again on sessions this file has
not seen.

usage: python3 tools/study-sweeps.py "data/es-1min/ES TestingData.txt" [--json out.json]
"""
import csv, io, sys, json, math, collections

RTH_A, RTH_B = 8*3600+30*60, 15*3600
NIGHT = 17*3600
MIN_RTH, MIN_ON = 386, 200
RECLAIM_MAX = 30
IB_BARS = 60
PREMKT = 7*3600
LONDON = 2*3600
DYN_START_VWAP, DYN_START_DEV = 5, 30
PROF_VA = 0.70
BUCKETS = [('08:30-09:00', 0, 30), ('09:00-10:00', 30, 90), ('10:00-11:30', 90, 180), ('11:30-15:00', 180, 391)]


def wilson(k, n, z=1.96):
    if n <= 0:
        return (0.0, 0.0)
    p = k / n
    den = 1 + z*z/n
    c = (p + z*z/(2*n)) / den
    h = z*math.sqrt(p*(1-p)/n + z*z/(4*n*n)) / den
    return (max(0.0, c-h), min(1.0, c+h))


def load_all(path):
    """Every bar, assigned to its TRADING day (>=17:00 -> next calendar day)."""
    import datetime as dt
    days = collections.defaultdict(list)
    with io.open(path, encoding='utf-8', errors='replace') as f:
        first = f.readline(); f.seek(0)
        sep = '\t' if first.count('\t') > first.count(',') else ','
        has_hdr = 'Open' in first or 'Date' in first
        cols = None
        if has_hdr:
            cols = [c.strip() for c in next(csv.reader([first], delimiter=sep))]
            f.readline()
        for parts in csv.reader(f, delimiter=sep):
            if len(parts) < 6:
                continue
            try:
                v_ = None
                if cols:
                    rec = dict(zip(cols, parts))
                    stamp = (rec.get('Date') or rec.get('DateTime') or '').strip()
                    o_, h_, l_, c_ = float(rec['Open']), float(rec['High']), float(rec['Low']), float(rec['Close'])
                    try: v_ = float(rec.get('Volume') or rec.get('VOL') or 0)
                    except (ValueError, TypeError): v_ = None
                else:
                    stamp = parts[1].strip()
                    o_, h_, l_, c_ = float(parts[2]), float(parts[3]), float(parts[4]), float(parts[5])
                    try: v_ = float(parts[6])
                    except (ValueError, IndexError): v_ = None
                stamp = stamp.strip('"')
                d, t = (stamp.split('T', 1) if 'T' in stamp else stamp.split(' ', 1))
                pt = t.split(':'); sec = int(pt[0])*3600 + int(pt[1])*60
                if sec >= NIGHT:
                    y, m, dd = [int(x) for x in d.split('-')]
                    d = (dt.date(y, m, dd) + dt.timedelta(days=1)).isoformat()
                days[d].append((sec, o_, h_, l_, c_, v_))
            except (ValueError, IndexError, KeyError):
                continue
    out = {}
    for d, bars in days.items():
        bars.sort()
        rth = [b for b in bars if RTH_A <= b[0] <= RTH_B]
        on = [b for b in bars if not (RTH_A <= b[0] <= RTH_B)]
        if len(rth) >= MIN_RTH and len(on) >= MIN_ON:
            out[d] = (rth, on)
    return out


def standing_baseline(sessions):
    """base[side][minute]  = share of sessions whose standing extreme at that minute is the final (the clock).
       fresh[side][minute] = the same, restricted to sessions whose standing extreme was printed within the
                             last RECLAIM_MAX bars - price is AT a fresh low right now, at any level or none.
                             This is the control a sweep must beat: a bounce off a fresh low that happens to
                             be the ONL/PDL/IBL, against a bounce off a fresh low that is nothing in particular."""
    base = {0: collections.defaultdict(list), 1: collections.defaultdict(list)}
    fresh = {0: collections.defaultdict(list), 1: collections.defaultdict(list)}
    for d, (rth, on) in sessions.items():
        fl = min(b[3] for b in rth); fh = max(b[2] for b in rth)
        rl, rh = 1e9, -1e9; tl = th = 0
        for i, b in enumerate(rth):
            if b[3] < rl: rl, tl = b[3], i
            if b[2] > rh: rh, th = b[2], i
            yl = 1 if rl <= fl+1e-9 else 0; yh = 1 if rh >= fh-1e-9 else 0
            base[0][i].append(yl); base[1][i].append(yh)
            if i-tl <= RECLAIM_MAX: fresh[0][i].append(yl)
            if i-th <= RECLAIM_MAX: fresh[1][i].append(yh)
    f = lambda M: {s: {i: (sum(v)/len(v) if v else None) for i, v in m.items()} for s, m in M.items()}
    return f(base), f(fresh)


def prior_profile(rth, vol_col=None):
    """POC / VAH / VAL of one RTH session from 1-minute bars (sec,o,h,l,c[,v]). Same construction as the
    panel's priorProfile(): volume spread evenly over each bar's 1-point grid, VA grown from the POC."""
    vol = {}
    tot = 0.0
    for b in rth:
        lo, hi = b[3], b[2]
        v = b[5] if (len(b) > 5 and b[5] is not None) else 1.0
        n = max(1, int(round(hi-lo))+1)
        share = v/n
        for k in range(n):
            px = int(round(lo))+k
            vol[px] = vol.get(px, 0.0)+share
            tot += share
    if not vol:
        return None
    ks = sorted(vol)
    poc = max(ks, key=lambda k: vol[k])
    pi = ks.index(poc); lo2 = hi2 = pi; acc = vol[poc]
    while acc < PROF_VA*tot and (lo2 > 0 or hi2 < len(ks)-1):
        dn = vol[ks[lo2-1]] if lo2 > 0 else -1
        up = vol[ks[hi2+1]] if hi2 < len(ks)-1 else -1
        if up >= dn and hi2 < len(ks)-1:
            hi2 += 1; acc += vol[ks[hi2]]
        elif lo2 > 0:
            lo2 -= 1; acc += vol[ks[lo2]]
        else:
            break
    return dict(poc=float(poc), val=float(ks[lo2]), vah=float(ks[hi2]))


def vwap_series(rth):
    """per-bar (vwap, sd): typical price x volume, cumulative; sd = volume-weighted sd of price about the vwap."""
    out = []; pv = v = pv2 = 0.0
    for b in rth:
        tp = (b[2]+b[3]+b[4])/3.0; vol = b[5] if (len(b) > 5 and b[5]) else 1.0
        pv += tp*vol; v += vol; pv2 += tp*tp*vol
        m = pv/v; var = max(0.0, pv2/v - m*m)
        out.append((m, math.sqrt(var)))
    return out


def dev_profile_series(rth):
    """per-bar (poc, vah, val) of TODAY'S profile so far, 1-point grid, 70% VA grown from the POC."""
    vol = {}; tot = 0.0; out = []
    for b in rth:
        lo, hi = b[3], b[2]; v = b[5] if (len(b) > 5 and b[5]) else 1.0
        n = max(1, int(round(hi-lo))+1); share = v/n
        for k in range(n):
            px = int(round(lo))+k; vol[px] = vol.get(px, 0.0)+share; tot += share
        ks = sorted(vol); poc = max(ks, key=lambda k: vol[k]); pi = ks.index(poc); lo2 = hi2 = pi; acc = vol[poc]
        while acc < PROF_VA*tot and (lo2 > 0 or hi2 < len(ks)-1):
            dn = vol[ks[lo2-1]] if lo2 > 0 else -1; up = vol[ks[hi2+1]] if hi2 < len(ks)-1 else -1
            if up >= dn and hi2 < len(ks)-1: hi2 += 1; acc += vol[ks[hi2]]
            elif lo2 > 0: lo2 -= 1; acc += vol[ks[lo2]]
            else: break
        out.append((float(poc), float(ks[hi2]), float(ks[lo2])))
    return out


def sweep_event_dynamic(rth, series, start, side=None):
    """A level that moves bar to bar. The first bar from `start` whose low/high prints through the level AS OF
    THAT BAR starts the event; the level is frozen there for the reclaim. side=None: the side is the side price
    came from (prior close above the level -> low-side); side=True/False forces low/high."""
    for i in range(max(1, start), len(rth)):
        lv = series[i]
        if lv is None: continue
        low = side if side is not None else (rth[i-1][4] > lv)
        if (rth[i][3] < lv) if low else (rth[i][2] > lv):
            ev = sweep_event(rth, lv, low, i)
            if ev: ev['low'] = low; ev['level_px'] = lv; return ev
            return None
    return None


def iso_week(d):
    import datetime as dt
    y, m, dd = [int(x) for x in d.split('-')]
    return dt.date(y, m, dd).isocalendar()[:2]


def sweep_event(rth, level, low_side, start=0):
    """Returns None or dict(sweep=i, reclaim=j|None, ext=float)."""
    for i in range(start, len(rth)):
        b = rth[i]
        if (b[3] < level) if low_side else (b[2] > level):
            ext = b[3] if low_side else b[2]
            for j in range(i+1, min(len(rth), i+1+RECLAIM_MAX)):
                bj = rth[j]
                ext = min(ext, bj[3]) if low_side else max(ext, bj[2])
                if (bj[4] > level) if low_side else (bj[4] < level):
                    return dict(sweep=i, reclaim=j, ext=ext)
            return dict(sweep=i, reclaim=None, ext=ext)
    return None


def bucket_of(i):
    for name, a, b in BUCKETS:
        if a <= i < b:
            return name
    return BUCKETS[-1][0]


def run(path):
    S = load_all(path)
    days = sorted(S)
    base, fresh = standing_baseline(S)
    ledger = 0
    events = []
    prev = None
    weeks = {}
    for d in days:
        rth, on = S[d]
        fl = min(b[3] for b in rth); fh = max(b[2] for b in rth)
        op = rth[0][1]; cl = rth[-1][4]
        onh = max(b[2] for b in on); onl = min(b[3] for b in on)
        ibh = max(b[2] for b in rth[:IB_BARS]); ibl = min(b[3] for b in rth[:IB_BARS])
        openloc = 'above ON' if op > onh else ('below ON' if op < onl else 'inside ON')
        pm = [b for b in on if PREMKT <= b[0] < RTH_A]
        or5h = max(b[2] for b in rth[:5]); or5l = min(b[3] for b in rth[:5])
        or15h = max(b[2] for b in rth[:15]); or15l = min(b[3] for b in rth[:15])
        levels = [('ONL', onl, True, 0), ('ONH', onh, False, 0), ('IBL', ibl, True, IB_BARS), ('IBH', ibh, False, IB_BARS),
                  ('OR5L', or5l, True, 5), ('OR5H', or5h, False, 5), ('OR15L', or15l, True, 15), ('OR15H', or15h, False, 15)]
        if pm:
            levels += [('PML', min(b[3] for b in pm), True, 0), ('PMH', max(b[2] for b in pm), False, 0)]
        if prev is not None:
            levels += [('PDL', prev[0], True, 0), ('PDH', prev[1], False, 0), ('PDC-', prev[2], True, 0), ('PDC+', prev[2], False, 0)]
            if prev[3]:
                levels += [('VAL', prev[3]['val'], True, 0), ('VAH', prev[3]['vah'], False, 0), ('POC-', prev[3]['poc'], True, 0), ('POC+', prev[3]['poc'], False, 0)]
        wk = iso_week(d)
        pw = weeks.get((wk[0], wk[1]-1)) or (weeks.get(max([k for k in weeks if k < wk], default=None)) if any(k < wk for k in weeks) else None)
        if pw:
            levels += [('PWL', pw[0], True, 0), ('PWH', pw[1], False, 0)]
        ldn = [b for b in on if LONDON <= b[0] < RTH_A]
        if len(ldn) >= 120:
            levels += [('LDNL', min(b[3] for b in ldn), True, 0), ('LDNH', max(b[2] for b in ldn), False, 0)]
        # (v15.57) DYNAMIC levels: VWAP and its bands, today's developing profile
        VS = vwap_series(rth); DP = dev_profile_series(rth)
        dyn = [('VWAP', [x[0] for x in VS], DYN_START_VWAP, None),
               ('VW1L', [x[0]-x[1] for x in VS], DYN_START_DEV, True), ('VW1H', [x[0]+x[1] for x in VS], DYN_START_DEV, False),
               ('VW2L', [x[0]-2*x[1] for x in VS], DYN_START_DEV, True), ('VW2H', [x[0]+2*x[1] for x in VS], DYN_START_DEV, False),
               ('DPOC', [x[0] for x in DP], DYN_START_DEV, None), ('DVAH', [x[1] for x in DP], DYN_START_DEV, False), ('DVAL', [x[2] for x in DP], DYN_START_DEV, True)]
        for name, ser, start, side in dyn:
            ev = sweep_event_dynamic(rth, ser, start, side)
            if not ev: continue
            low = ev['low']; nm = name + (('-' if low else '+') if side is None else '')
            printed = None
            if ev['reclaim'] is not None:
                printed = 1 if ((ev['ext'] <= fl+1e-9) if low else (ev['ext'] >= fh-1e-9)) else 0
            events.append(dict(d=d, level=nm, side='LOD' if low else 'HOD', openloc=openloc, sweep=ev['sweep'], reclaim=ev['reclaim'], ext=ev['ext'],
                               depth=abs(ev['level_px']-ev['ext']), speed=(ev['reclaim']-ev['sweep']) if ev['reclaim'] is not None else None,
                               bucket=bucket_of(ev['sweep']), printed=printed,
                               base=(base[0 if low else 1][ev['reclaim']] if ev['reclaim'] is not None else None),
                               fresh=(fresh[0 if low else 1].get(ev['reclaim']) if ev['reclaim'] is not None else None),
                               to_close=((cl-ev['ext']) if low else (ev['ext']-cl)), to_far=((fh-ev['ext']) if low else (ev['ext']-fl))))
        for name, lv, low, start in levels:
            ev = sweep_event(rth, lv, low, start)
            if not ev:
                continue
            printed = None
            if ev['reclaim'] is not None:
                printed = 1 if ((ev['ext'] <= fl+1e-9) if low else (ev['ext'] >= fh-1e-9)) else 0
            events.append(dict(
                d=d, level=name, side='LOD' if low else 'HOD', openloc=openloc,
                sweep=ev['sweep'], reclaim=ev['reclaim'], ext=ev['ext'],
                depth=(lv-ev['ext']) if low else (ev['ext']-lv),
                speed=(ev['reclaim']-ev['sweep']) if ev['reclaim'] is not None else None,
                bucket=bucket_of(ev['sweep']),
                printed=printed,
                base=(base[0 if low else 1][ev['reclaim']] if ev['reclaim'] is not None else None),
                fresh=(fresh[0 if low else 1].get(ev['reclaim']) if ev['reclaim'] is not None else None),
                to_close=((cl-ev['ext']) if low else (ev['ext']-cl)),
                to_far=((fh-ev['ext']) if low else (ev['ext']-fl)),
            ))
        prev = (fl, fh, cl, prior_profile(rth))
        weeks.setdefault(iso_week(d), [1e9, -1e9]); weeks[iso_week(d)][0] = min(weeks[iso_week(d)][0], fl); weeks[iso_week(d)][1] = max(weeks[iso_week(d)][1], fh)

    def cell(rows, label):
        nonlocal ledger
        sc = [r for r in rows if r['printed'] is not None]
        n = len(sc); k = sum(r['printed'] for r in sc)
        acc = len(rows) - n
        ledger += 1
        lo, hi = wilson(k, n)
        b = (sum(r['base'] for r in sc)/n) if n else None
        fr = [r['fresh'] for r in sc if r['fresh'] is not None]
        fb = (sum(fr)/len(fr)) if fr else None
        med = lambda xs: (sorted(xs)[len(xs)//2] if xs else None)
        win = [r for r in sc if r['printed']]
        return dict(label=label, events=len(rows), reclaimed=n, accepted=acc, printed=k,
                    rate=(k/n if n else None), ci=[lo, hi], base=b, fresh=fb,
                    lift=((k/n - b) if (n and b is not None) else None),
                    lift_fresh=((k/n - fb) if (n and fb is not None) else None),
                    pay_close_med=med([r['to_close'] for r in win]),
                    pay_far_med=med([r['to_far'] for r in win]))

    out = dict(corpus=dict(sessions=len(days), first=days[0], last=days[-1], file=path),
               definitions=__doc__.split('DEFINITIONS')[1].split('usage:')[0].strip(),
               cells=[])
    by_level = collections.defaultdict(list)
    for e in events:
        by_level[e['level']].append(e)
    LEVELS = ['ONL', 'ONH', 'PDL', 'PDH', 'IBL', 'IBH', 'PDC-', 'PDC+', 'VAL', 'VAH', 'POC-', 'POC+', 'PML', 'PMH', 'PWL', 'PWH', 'OR5L', 'OR5H', 'OR15L', 'OR15H',
              'LDNL', 'LDNH', 'VWAP-', 'VWAP+', 'VW1L', 'VW1H', 'VW2L', 'VW2H', 'DPOC-', 'DPOC+', 'DVAL', 'DVAH']
    def side_of(lv):
        return 'LOD' if (lv.endswith('L') or lv.endswith('-')) else 'HOD'
    for lv in LEVELS:
        rows = by_level[lv]
        out['cells'].append(cell(rows, lv+' sweep-reclaim -> printed the '+side_of(lv)))
        for bname, _, _ in BUCKETS:
            sub = [r for r in rows if r['bucket'] == bname]
            if len([r for r in sub if r['printed'] is not None]) >= 15:
                out['cells'].append(cell(sub, '  '+lv+' · '+bname))
    # pooled by side
    out['cells'].append(cell([e for e in events if e['side'] == 'LOD' and e['level'] in ('ONL', 'PDL')], 'ONL+PDL pooled'))
    out['cells'].append(cell([e for e in events if e['side'] == 'HOD' and e['level'] in ('ONH', 'PDH')], 'ONH+PDH pooled'))
    # open location × ON sweep
    for loc in ('above ON', 'inside ON', 'below ON'):
        sub = [e for e in events if e['level'] in ('ONL', 'ONH') and e['openloc'] == loc]
        if len([r for r in sub if r['printed'] is not None]) >= 15:
            out['cells'].append(cell(sub, 'ON sweep · open '+loc))
    # speed and depth (ON+PD only)
    core = [e for e in events if e['level'] in ('ONL', 'ONH', 'PDL', 'PDH') and e['printed'] is not None]
    out['cells'].append(cell([e for e in core if e['speed'] <= 5], 'reclaim within 5 bars'))
    out['cells'].append(cell([e for e in core if e['speed'] > 5], 'reclaim in 6-30 bars'))
    out['cells'].append(cell([e for e in core if e['depth'] <= 3], 'depth <= 3 pts'))
    out['cells'].append(cell([e for e in core if 3 < e['depth'] <= 8], 'depth 3-8 pts'))
    out['cells'].append(cell([e for e in core if e['depth'] > 8], 'depth > 8 pts'))
    # how many sessions had ANY ON/PD sweep-reclaim that printed the extreme
    win_days = set(e['d'] for e in events if e['printed'] == 1 and e['level'] in ('ONL', 'ONH', 'PDL', 'PDH'))
    out['sessions_with_a_winning_sweep'] = len(win_days)
    out['sessions_with_any_on_pd_sweep'] = len(set(e['d'] for e in events if e['level'] in ('ONL', 'ONH', 'PDL', 'PDH')))
    out['ledger'] = dict(cells_read=ledger, note='first read, not pre-registered; expect ~1 in 20 cells to sit outside its interval by chance')
    # THE LOOKUP the panel reads (statsRead): one entry per key, {rate, n, fresh, ci}
    def slim(c):
        return dict(rate=c['rate'], n=c['reclaimed'], fresh=c['fresh'], ci=c['ci'], lift=c['lift_fresh'], pay=c['pay_far_med']) if c['rate'] is not None else dict(rate=None, n=c['reclaimed'])
    cells = {c['label'].strip(): c for c in out['cells']}
    out['lookup'] = dict(
        level={lv: slim(cells[lv+' sweep-reclaim -> printed the '+side_of(lv)]) for lv in LEVELS},
        clock={b[0]: slim(cell([e for e in events if e['level'] in ('ONL', 'ONH', 'PDL', 'PDH') and e['bucket'] == b[0]], 'lookup clock '+b[0])) for b in BUCKETS},
        speed=dict(poke=slim(cells['reclaim within 5 bars']), flush=slim(cells['reclaim in 6-30 bars'])),
        depth=dict(shallow=slim(cells['depth <= 3 pts']), mid=slim(cells['depth 3-8 pts']), deep=slim(cells['depth > 8 pts'])),
        bins=dict(speedPokeMaxBars=5, depthShallowMaxPts=3, depthDeepMinPts=8, reclaimMaxBars=RECLAIM_MAX),
        sessions=len(days), first=days[0], last=days[-1])
    out['ledger']['cells_read'] = ledger
    out['events'] = events
    return out


def fmt(out):
    L = []
    c = out['corpus']
    L.append('SWEEPS · %d sessions · %s -> %s' % (c['sessions'], c['first'], c['last']))
    L.append('%-38s %6s %6s %6s %8s %14s %7s %8s %7s %8s %8s' % ('cell', 'events', 'recl', 'accept', 'printed', 'rate [95%]', 'clock', 'lift', 'fresh', 'lift', 'pay far'))
    for x in out['cells']:
        if x['rate'] is None:
            L.append('%-38s %6d %6d %6d %8s' % (x['label'], x['events'], x['reclaimed'], x['accepted'], '—'))
            continue
        L.append('%-38s %6d %6d %6d %8d %5.0f%% [%2.0f-%2.0f] %6.0f%% %+7.0fpp %6.0f%% %+7.0fpp %7s' % (
            x['label'], x['events'], x['reclaimed'], x['accepted'], x['printed'],
            100*x['rate'], 100*x['ci'][0], 100*x['ci'][1], 100*x['base'], 100*x['lift'],
            100*(x['fresh'] if x['fresh'] is not None else 0), 100*(x['lift_fresh'] if x['lift_fresh'] is not None else 0),
            ('%.1f' % x['pay_far_med']) if x['pay_far_med'] is not None else '—'))
    L.append('sessions with an ON/PD sweep-reclaim: %d · of which one printed the extreme: %d' % (out['sessions_with_any_on_pd_sweep'], out['sessions_with_a_winning_sweep']))
    L.append('ledger: %d cells read · %s' % (out['ledger']['cells_read'], out['ledger']['note']))
    return '\n'.join(L)


if __name__ == '__main__':
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    path = args[0] if args else 'data/es-1min/ES TestingData.txt'
    out = run(path)
    print(fmt(out))
    if '--json' in sys.argv:
        jp = sys.argv[sys.argv.index('--json')+1]
        slim = dict(out); slim.pop('events')
        io.open(jp, 'w', encoding='utf-8').write(json.dumps(slim, indent=1))
        print('wrote', jp)
