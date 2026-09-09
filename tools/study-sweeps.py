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
  WPOC            (v15.88) the previous ISO week's POC — the week's RTH volume profile (the same 1-point grid as POC,
                  over five sessions); both sides, like POC. His 'weekly POC' (2026-09-08); the panel draws it from the
                  companion's 5-minute weekly bars (v1.19), the corpus from its own minutes.
  OR5 / OR15      the opening range: the high/low of the first 5 / 15 RTH minutes; sweeps looked for
                  after the range completes.
  AHI/ALO         (v15.88) the ASIA session: the ON bars from the Globex open 17:00 CT (the evening before) to 02:00.
  LHI/LLO         (v15.88) the LONDON session: the ON bars from 02:00 CT to the open. (v15.57's LDNH/LDNL, renamed to
                  his names — operator 2026-09-08: "they can be ALO, AHI, LLO, LHI"; the hours 2026-09-09: "whatever is standard".)
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
import csv, io, sys, json, math, collections, os

RTH_A, RTH_B = 8*3600+30*60, 15*3600
NIGHT = 17*3600
MIN_RTH, MIN_ON = 386, 200
RECLAIM_MAX = 30
IB_BARS = 60
PREMKT = 7*3600
LONDON = 2*3600          # (v15.88) the Asia / London cut, 02:00 CT; Asia runs from NIGHT (17:00) to it
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


VENDOR_FILES = ['data/es-1min/ES TestingData.txt', 'data/es-1min/ES_TestingData.txt']
def sources(market='ES'):
    """(v15.90) the corpus = the vendor file (one contract, through 2026-08-21) + every couriered CSV under data/futures/<MK>/ —
    the RTH files study-hodlod reads AND the -night files (17:00 -> 08:27) append-futures writes since v15.90. The sweep
    corpus had stopped on the vendor's last day: H7 on the register read n = 0 after thirteen sessions (the audit of
    2026-09-09). A session enters when it has both halves (MIN_RTH / MIN_ON); vendor and Yahoo never mix inside one day."""
    import glob
    out = [p for p in VENDOR_FILES if os.path.exists(p)][:1]
    out += sorted(glob.glob(os.path.join('data', 'futures', market, '*.csv')))
    return out


def source_of(path):
    return 'yahoo' if path.replace(os.sep, '/').startswith('data/futures/') else 'vendor'


def load_all(paths):
    """Every bar, assigned to its TRADING day (>=17:00 -> next calendar day). `paths`: one file or a list (v15.90); a day
    holds bars from ONE source — the vendor's when both have it (the continuous front month differs by the calendar
    spread across a roll; the levels and the bars of a session must share a basis)."""
    import datetime as dt
    days = collections.defaultdict(list)
    day_src = {}
    if isinstance(paths, str):
        paths = [paths]
    for path in paths:
        src = source_of(path)
        _load_file(path, src, days, day_src, dt)
    out = {}
    for d, bars in days.items():
        bars.sort()
        rth = [b for b in bars if RTH_A <= b[0] <= RTH_B]
        on = [b for b in bars if not (RTH_A <= b[0] <= RTH_B)]
        if len(rth) >= MIN_RTH and len(on) >= MIN_ON:
            out[d] = (rth, on)
    out_src = {d: day_src.get(d) for d in out}
    load_all.last_sources = out_src
    return out


def _load_file(path, src, days, day_src, dt):
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
                    while dt.date.fromisoformat(d).weekday() >= 5:      # a Sunday evening is Monday's night
                        d = (dt.date.fromisoformat(d) + dt.timedelta(days=1)).isoformat()
                if d in day_src and day_src[d] != src:
                    if day_src[d] == 'vendor':
                        continue                       # the vendor's day stands; a Yahoo bar never joins it
                    days[d] = [b for b in days[d]]     # (a vendor bar arriving after Yahoo's cannot happen: the vendor file is listed first)
                day_src.setdefault(d, src)
                days[d].append((sec, o_, h_, l_, c_, v_))
            except (ValueError, IndexError, KeyError):
                continue


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


def run(path=None):
    paths = path if path else sources()
    S = load_all(paths)
    srcs = getattr(load_all, 'last_sources', {}) or {}
    days = sorted(S)
    base, fresh = standing_baseline(S)
    ledger = 0
    events = []
    prev = None
    prev_d = None
    weeks = {}
    import datetime as _dt
    for d in days:
        rth, on = S[d]
        # (v15.90) THE PRIOR DAY IS THE PRIOR TRADING DAY OR NOTHING. With the corpus appending across a gap (the vendor ends
        # 08-21, the couriered nights begin 09-01) the previous session in corpus order can be eleven days back; PDH / PDL /
        # PDC / the profile from it would be a different week's levels wearing today's names. Four calendar days covers a
        # Friday -> Monday and a holiday Monday -> Tuesday; beyond that the PD levels are absent for that session.
        gap_ok = (prev_d is not None) and ((_dt.date.fromisoformat(d) - _dt.date.fromisoformat(prev_d)).days <= 4)
        fl = min(b[3] for b in rth); fh = max(b[2] for b in rth)
        op = rth[0][1]; cl = rth[-1][4]
        # (v15.88) THE NIGHT IS BEFORE THE SESSION. `on` also holds the day's own post-close bars (15:01-16:59, before the
        # 17:00 roll to the next key); they came AFTER the RTH they were being measured against, and the panel's overnightHL
        # never had them (17:00 of the prior key -> 08:29). One definition: the evening from 17:00 and the morning to the open.
        night = [b for b in on if b[0] >= NIGHT or b[0] < RTH_A] or on
        onh = max(b[2] for b in night); onl = min(b[3] for b in night)
        ibh = max(b[2] for b in rth[:IB_BARS]); ibl = min(b[3] for b in rth[:IB_BARS])
        openloc = 'above ON' if op > onh else ('below ON' if op < onl else 'inside ON')
        pm = [b for b in on if PREMKT <= b[0] < RTH_A]
        or5h = max(b[2] for b in rth[:5]); or5l = min(b[3] for b in rth[:5])
        or15h = max(b[2] for b in rth[:15]); or15l = min(b[3] for b in rth[:15])
        levels = [('ONL', onl, True, 0), ('ONH', onh, False, 0), ('IBL', ibl, True, IB_BARS), ('IBH', ibh, False, IB_BARS),
                  ('OR5L', or5l, True, 5), ('OR5H', or5h, False, 5), ('OR15L', or15l, True, 15), ('OR15H', or15h, False, 15)]
        if pm:
            levels += [('PML', min(b[3] for b in pm), True, 0), ('PMH', max(b[2] for b in pm), False, 0)]
        if prev is not None and gap_ok:
            levels += [('PDL', prev[0], True, 0), ('PDH', prev[1], False, 0), ('PDC-', prev[2], True, 0), ('PDC+', prev[2], False, 0)]
            if prev[3]:
                levels += [('VAL', prev[3]['val'], True, 0), ('VAH', prev[3]['vah'], False, 0), ('POC-', prev[3]['poc'], True, 0), ('POC+', prev[3]['poc'], False, 0)]
        wk = iso_week(d)
        pw = weeks.get((wk[0], wk[1]-1)) or weeks.get((wk[0], wk[1]-2))   # (v15.90) the prior ISO week, or the one before it (a holiday week) — never an older one across a gap
        if pw:
            levels += [('PWL', pw[0], True, 0), ('PWH', pw[1], False, 0)]
            if len(pw) > 2 and pw[2]:
                wp = max(sorted(pw[2]), key=lambda k: pw[2][k])
                levels += [('WPOC-', wp, True, 0), ('WPOC+', wp, False, 0)]
        asia = [b for b in on if b[0] >= NIGHT or b[0] < LONDON]
        if len(asia) >= 150:      # the panel's floor (sessionHL): a holiday evening that starts late is still a session
            levels += [('ALO', min(b[3] for b in asia), True, 0), ('AHI', max(b[2] for b in asia), False, 0)]
        ldn = [b for b in on if LONDON <= b[0] < RTH_A]
        if len(ldn) >= 120:
            levels += [('LLO', min(b[3] for b in ldn), True, 0), ('LHI', max(b[2] for b in ldn), False, 0)]
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
        prev = (fl, fh, cl, prior_profile(rth)); prev_d = d
        weeks.setdefault(iso_week(d), [1e9, -1e9, {}]); weeks[iso_week(d)][0] = min(weeks[iso_week(d)][0], fl); weeks[iso_week(d)][1] = max(weeks[iso_week(d)][1], fh)
        wv = weeks[iso_week(d)][2]                     # (v15.88) the week's volume by price, for WPOC
        for b in rth:
            lo_, hi_ = b[3], b[2]; v_ = b[5] if (len(b) > 5 and b[5] is not None) else 1.0
            n_ = max(1, int(round(hi_-lo_))+1); sh_ = v_/n_
            for k_ in range(n_):
                px_ = int(round(lo_))+k_; wv[px_] = wv.get(px_, 0.0)+sh_

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

    src_counts = {}
    for d in days:
        k = srcs.get(d) or 'vendor'; src_counts[k] = src_counts.get(k, 0) + 1
    out = dict(corpus=dict(sessions=len(days), first=days[0], last=days[-1], file=(path if isinstance(path, str) else 'the vendor file + data/futures/ES/*.csv (v15.90)'),
                           sources=src_counts, yahooFirst=min([d for d in days if srcs.get(d) == 'yahoo'], default=None)),
               definitions=__doc__.split('DEFINITIONS')[1].split('usage:')[0].strip(),
               cells=[])
    by_level = collections.defaultdict(list)
    for e in events:
        by_level[e['level']].append(e)
    LEVELS = ['ONL', 'ONH', 'PDL', 'PDH', 'IBL', 'IBH', 'PDC-', 'PDC+', 'VAL', 'VAH', 'POC-', 'POC+', 'PML', 'PMH', 'PWL', 'PWH', 'WPOC-', 'WPOC+', 'OR5L', 'OR5H', 'OR15L', 'OR15H',
              'ALO', 'AHI', 'LLO', 'LHI', 'VWAP-', 'VWAP+', 'VW1L', 'VW1H', 'VW2L', 'VW2H', 'DPOC-', 'DPOC+', 'DVAL', 'DVAH']
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


def selftest():
    """(v15.90) load_all over many files: one source per day, the vendor's wins; a night file + an RTH file make a session;
    the gap rule keeps the prior day's levels off a session eleven days after the previous one."""
    import tempfile, shutil, datetime as dt
    root = tempfile.mkdtemp(); cwd = os.getcwd(); os.chdir(root)
    try:
        os.makedirs('data/es-1min'); os.makedirs('data/futures/ES')
        def bars(day, a, b, px, step=60):
            out = []; t = a
            while t < b:
                out.append('EPM26,%s %02d:%02d:00,,%s,%s,%s,%s,10' % (day, t // 3600, (t % 3600) // 60, px, px + 1, px - 1, px)); t += step
            return out
        # the vendor: two full sessions, 2026-08-20 and 08-21 (each: the night from 17:00 the evening before + RTH)
        V = ['Symbol,Date,VOL,Open,High,Low,Close,Volume']
        V += bars('2026-08-19', 17 * 3600, 24 * 3600, 100) + bars('2026-08-20', 0, RTH_A, 100) + bars('2026-08-20', RTH_A, RTH_B + 60, 100)
        V += bars('2026-08-20', 17 * 3600, 24 * 3600, 101) + bars('2026-08-21', 0, RTH_A, 101) + bars('2026-08-21', RTH_A, RTH_B + 60, 101)
        io.open('data/es-1min/ES TestingData.txt', 'w').write('\n'.join(V) + '\n')
        # Yahoo: 09-01 = a night file + an RTH file (a session); 09-02 = RTH only (NOT a session); a Yahoo copy of 08-21 (ignored: the vendor's stands)
        H = 'Symbol,Date,VOL,Open,High,Low,Close,Volume'
        io.open('data/futures/ES/2026-09-01-night.csv', 'w').write('\n'.join([H] + bars('2026-08-31', 17 * 3600, 24 * 3600, 200) + bars('2026-09-01', 0, RTH_A - 180, 200)) + '\n')
        # 09-01's RTH dips under the vendor's 08-21 low (100) for three minutes at 09:00 and reclaims — a PDL sweep IF a PDL existed
        r1 = bars('2026-09-01', RTH_A - 180, 9 * 3600, 200) + bars('2026-09-01', 9 * 3600, 9 * 3600 + 180, 99) + bars('2026-09-01', 9 * 3600 + 180, RTH_B, 200)
        io.open('data/futures/ES/2026-09-01.csv', 'w').write('\n'.join([H] + r1) + '\n')
        io.open('data/futures/ES/2026-09-02.csv', 'w').write('\n'.join([H] + bars('2026-09-02', RTH_A - 180, RTH_B, 201)) + '\n')
        io.open('data/futures/ES/2026-08-21.csv', 'w').write('\n'.join([H] + bars('2026-08-21', RTH_A - 180, RTH_B, 999)) + '\n')
        src = sources('ES')
        assert src[0].endswith('ES TestingData.txt') and any(p.endswith('2026-09-01-night.csv') for p in src) and len(src) == 5, src
        S = load_all(src)
        assert sorted(S) == ['2026-08-20', '2026-08-21', '2026-09-01'], sorted(S)
        assert S['2026-08-21'][0][0][1] == 101 and all(b[1] != 999 for b in S['2026-08-21'][0] + S['2026-08-21'][1]), 'the vendor\'s 08-21 stands; the Yahoo copy (999) never joins it'
        assert load_all.last_sources == {'2026-08-20': 'vendor', '2026-08-21': 'vendor', '2026-09-01': 'yahoo'}, load_all.last_sources
        out = run(src)
        assert out['corpus']['sessions'] == 3 and out['corpus']['sources'] == {'vendor': 2, 'yahoo': 1} and out['corpus']['yahooFirst'] == '2026-09-01', out['corpus']
        # the gap rule: 09-01 follows 08-21 by eleven days -> no PD / PDC / profile levels were even looked for on 09-01
        lv = set(e['level'] for e in out['events'] if e['d'] == '2026-09-01')
        assert not (lv & {'PDL', 'PDH', 'PDC-', 'PDC+', 'VAL', 'VAH', 'POC-', 'POC+'}), lv
        # …and the same dip WITH a prior day one day back is a PDL sweep: rebuild 09-01's night/RTH as 08-24 (the Monday after 08-21)
        os.remove('data/futures/ES/2026-09-01-night.csv'); os.remove('data/futures/ES/2026-09-01.csv'); os.remove('data/futures/ES/2026-08-21.csv')
        io.open('data/futures/ES/2026-08-24-night.csv', 'w').write('\n'.join([H] + bars('2026-08-21', 17 * 3600, 24 * 3600, 200) + bars('2026-08-24', 0, RTH_A - 180, 200)) + '\n')
        r2 = bars('2026-08-24', RTH_A - 180, 9 * 3600, 200) + bars('2026-08-24', 9 * 3600, 9 * 3600 + 180, 99) + bars('2026-08-24', 9 * 3600 + 180, RTH_B, 200)
        io.open('data/futures/ES/2026-08-24.csv', 'w').write('\n'.join([H] + r2) + '\n')
        out2 = run(sources('ES'))
        lv2 = set(e['level'] for e in out2['events'] if e['d'] == '2026-08-24')
        assert 'PDL' in lv2, lv2
        # …while 08-21, one day after 08-20, could have them (flat fixture bars sweep nothing; the rule is what is pinned)
        assert (dt.date(2026, 8, 21) - dt.date(2026, 8, 20)).days <= 4
        print('study-sweeps selftest ok')
    finally:
        os.chdir(cwd); shutil.rmtree(root, ignore_errors=True)


if __name__ == '__main__':
    if '--selftest' in sys.argv:
        selftest(); sys.exit(0)
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    path = args[0] if args else 'data/es-1min/ES TestingData.txt'
    out = run(path)
    print(fmt(out))
    if '--json' in sys.argv:
        jp = sys.argv[sys.argv.index('--json')+1]
        slim = dict(out); slim.pop('events')
        io.open(jp, 'w', encoding='utf-8').write(json.dumps(slim, indent=1))
        print('wrote', jp)
