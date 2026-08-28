#!/usr/bin/env python3
"""
IS THE LOW / HIGH OF DAY IN?  — the operator's actual question, 2026-08-28:

    "my intention and goal behind this field is to identify a low or a high of day so i can profit
     when it goes to the other extremity ... you need to help identify whether the lod or hod is
     done ... run many combinations and figure out a high probability way"

⚠⚠ THE THING ANY RULE MUST BEAT IS SIMPLY WAITING. The survival ladder already says a standing
extreme is the day's ~82% of the time once it has stood two hours. A rule that scores 85% at 14:00
is WORSE than useless - it is the base rate wearing a costume. Every number below is therefore
reported against the base rate AT THE SAME TIME OF DAY, and with the median clock time it fires.

⚠ THREE HONESTY CONTROLS, each of which this project has been burned by before:
  1. TIME STRATIFICATION. The bar-level base rate climbs from ~49% at the open to 100% at the close,
     so any condition that can only fire late earns accuracy for free (PROJECT-CONSTANTS pattern 7).
     Lift is computed WITHIN time buckets.
  2. FIRST-FIRE, ONE ROW PER SESSION-SIDE. 400 bars in a session are not 400 observations - the
     label barely changes across them. The decision-relevant question is "when the rule FIRST says
     the low is in, is it right?", and that is one observation per session per side.
  3. TRAIN / HOLDOUT BY DATE. Many rules are tested; with a small n the best of them is partly luck.
     The holdout column is the one to believe.
"""
import csv, io, sys, collections, statistics as st

RTH_A, RTH_B = 8*3600+30*60, 15*3600
MIN_BARS = 386
SMA_MIN = 150     # his chart is 3-minute with a 50-SMA -> 150 minutes
NBAR_MIN = 60     # his "20 bar high/low" on 3-minute bars -> 60 minutes


def load(path):
    ses = collections.defaultdict(list)
    with io.open(path, encoding='utf-8') as f:
        for x in csv.DictReader(f):
            s = (x.get('Date') or '').strip()
            if ' ' not in s:
                continue
            d, t = s.split(' ', 1)
            p = t.split(':')
            try:
                sec = int(p[0])*3600 + int(p[1])*60
                if not (RTH_A <= sec <= RTH_B):
                    continue
                ses[d].append((sec, float(x['Open']), float(x['High']),
                               float(x['Low']), float(x['Close'])))
            except (ValueError, IndexError):
                continue
    return {d: sorted(v) for d, v in ses.items() if len(v) >= MIN_BARS}


def build(ses):
    days = sorted(ses)
    rows = []
    prev_hi = prev_lo = None
    for d in days:
        b = ses[d]
        n = len(b)
        op = b[0][1]
        fin_lo = min(x[3] for x in b)
        fin_hi = max(x[2] for x in b)
        ib30h = max(x[2] for x in b[:30]); ib30l = min(x[3] for x in b[:30])
        ib60h = max(x[2] for x in b[:60]); ib60l = min(x[3] for x in b[:60])
        closes = [x[4] for x in b]
        run_lo, run_lo_t, run_hi, run_hi_t = 1e9, None, -1e9, None
        for t in range(n):
            sec, o, h, l, c = b[t]
            if l < run_lo:
                run_lo, run_lo_t = l, t
            if h > run_hi:
                run_hi, run_hi_t = h, t
            if t < 60:                       # IB60 must exist for a fair comparison
                continue
            ma = sum(closes[max(0, t-SMA_MIN+1):t+1]) / min(t+1, SMA_MIN)
            w = b[max(0, t-NBAR_MIN):t]
            hi_n = max(x[2] for x in w); lo_n = min(x[3] for x in w)
            rng = run_hi - run_lo
            if rng <= 0:
                continue
            for side in (0, 1):              # 0 = is the LOW in, 1 = is the HIGH in
                low = side == 0
                ext_t = run_lo_t if low else run_hi_t
                y = 1 if (run_lo <= fin_lo + 1e-9 if low else run_hi >= fin_hi - 1e-9) else 0
                f = dict(
                    ib30=(c > ib30h) if low else (c < ib30l),
                    ib60=(c > ib60h) if low else (c < ib60l),
                    swp=(prev_lo is not None and run_lo < prev_lo and c > prev_lo) if low
                        else (prev_hi is not None and run_hi > prev_hi and c < prev_hi),
                    sma=(c > ma) if low else (c < ma),
                    bN=(c > hi_n) if low else (c < lo_n),
                    opn=(c > op) if low else (c < op),
                    pos=((c-run_lo)/rng > 0.5) if low else ((run_hi-c)/rng > 0.5),
                    far=((c-run_lo)/rng > 0.75) if low else ((run_hi-c)/rng > 0.75),
                )
                rows.append(dict(d=d, t=t, side=side, y=y,
                                 stood=(sec - b[ext_t][0])//60, f=f))
        prev_hi, prev_lo = fin_hi, fin_lo
    return days, rows


def first_fire(rows, pred, dayset):
    """one observation per (session, side): the FIRST bar the rule fires."""
    seen, out = set(), []
    for r in rows:
        if r['d'] not in dayset:
            continue
        k = (r['d'], r['side'])
        if k in seen:
            continue
        if pred(r):
            seen.add(k)
            out.append(r)
    if not out:
        return dict(n=0, hit=None, med=None)
    ts = sorted(r['t'] for r in out)
    med = ts[len(ts)//2]
    return dict(n=len(out), hit=round(100*sum(r['y'] for r in out)/len(out)),
                med=RTH_A + med*60)


def clock(sec):
    if sec is None:
        return '--'
    h, m = sec//3600, (sec % 3600)//60
    return '%d:%02d' % (h, m)


def main(path):
    ses = load(path)
    days, rows = build(ses)
    cut = int(len(days)*0.6)
    TR, HO = set(days[:cut]), set(days[cut:])
    print('corpus   : %d complete sessions, %s -> %s' % (len(days), days[0], days[-1]))
    print('rows     : %d bar-observations (NOT independent)' % len(rows))
    print('effective: %d session-sides\n' % (len(days)*2))

    F = lambda k: (lambda r: r['f'][k])
    AND = lambda *ks: (lambda r: all(r['f'][k] for k in ks))
    LAD = lambda m: (lambda r: r['stood'] >= m)
    cands = [
        ('WAIT 60m (ladder)', LAD(60)), ('WAIT 90m', LAD(90)),
        ('WAIT 120m', LAD(120)), ('WAIT 180m', LAD(180)),
        ('IB30 broken', F('ib30')), ('IB60 broken', F('ib60')),
        ('SMA50 (150m)', F('sma')), ('60m breakout', F('bN')),
        ('sweep+reclaim', F('swp')), ('open reclaimed', F('opn')),
        ('>50% of range', F('pos')), ('>75% of range', F('far')),
        ('IB60+SMA', AND('ib60', 'sma')), ('IB60+far', AND('ib60', 'far')),
        ('SMA+far', AND('sma', 'far')), ('IB60+SMA+far', AND('ib60', 'sma', 'far')),
        ('IB60+60m brk', AND('ib60', 'bN')),
        ('IB60+SMA & 60m stood', lambda r: r['f']['ib60'] and r['f']['sma'] and r['stood'] >= 60),
    ]
    print('%-22s %-14s %-14s %-14s %s' % ('rule', 'TRAIN n/hit', 'HOLDOUT n/hit', 'ALL n/hit', 'median fire'))
    print('-'*82)
    res = []
    for name, p in cands:
        a, b, z = first_fire(rows, p, TR), first_fire(rows, p, HO), first_fire(rows, p, set(days))
        res.append((name, a, b, z))
        print('%-22s %-14s %-14s %-14s %s CT' % (
            name,
            '%d/%s%%' % (a['n'], a['hit']), '%d/%s%%' % (b['n'], b['hit']),
            '%d/%s%%' % (z['n'], z['hit']), clock(z['med'])))
    return res


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else 'data/es-1min/ES TestingData.txt')
