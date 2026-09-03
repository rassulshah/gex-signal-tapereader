#!/usr/bin/env python3
"""
NODE LOOKUP — what the book did at a strike, minute by minute, from the panel's own day files.  (v15.62)

    "I provide you screenshots, you lookup the nodes, any additional data and learn … make sure you take all the
     factors into account like if it is a new node or heavy node growth or node roll etc. you want to lookup the
     node that caused the deflection"                                                       - operator, 2026-09-03

The day file (data/<day>.json, his Save) carries, per 3-minute bar, the vendor book verbatim (`snaps[sym][i].vend.rows`
= [strike, cur $, d5, d15, d60, d1d], the 90 largest strikes), the King (`snaps[i].king`, in the ladder's display
scale), the price (`snaps[i].px`, display scale), and the node-event ledger (`nodeEvents`: ACCUM / DISSIP / ROLL /
TURN_UP / TURN_DN with %King, stage, taps, side). The ES minute bars ride in `futBars.ES.rows` = [t, o, h, l, c, v].

This prints a strike's line — the thing every learning example is checked against — so a deflection is never
called from the picture alone.

    python3 tools/node-lookup.py 2026-08-28 7775            # one SPXW strike, the whole recorded day
    python3 tools/node-lookup.py 2026-08-28 7775 09:30-10:40
    python3 tools/node-lookup.py 2026-08-28 --es 7780 09:30-10:40   # strikes within ±6 ES pts of an ES price
    python3 tools/node-lookup.py 2026-08-28 --price 12:00-12:40      # the ES minute path only
    python3 tools/node-lookup.py 2026-08-28 --king                   # the King's path through the day
"""
import json, io, sys, datetime, os

ES_PER_SPX = 1.00108   # ES ≈ SPXW × this (the basis on 2026-09-03; use --ratio to override)


def load(day):
    p = 'data/%s.json' % day
    if not os.path.exists(p):
        sys.exit('no day file ' + p)
    return json.load(io.open(p, encoding='utf-8'))


def hhmm_of_ms(ms):
    d = datetime.datetime.utcfromtimestamp(ms / 1000.0 - 5 * 3600)
    return d.hour * 100 + d.minute


def tmin(h):
    return (h // 100) * 60 + h % 100


def parse_win(s):
    if not s:
        return (0, 2400)
    a, b = s.split('-')
    return (int(a.replace(':', '')), int(b.replace(':', '')))


def es_bars(j, day, win):
    rows = ((j.get('futBars') or {}).get('ES') or {}).get('rows') or []
    if not rows or not any(datetime.datetime.utcfromtimestamp((b[0] / 1000.0 if b[0] > 1e12 else b[0]) - 5 * 3600).strftime('%Y-%m-%d') == day for b in rows):
        # the courier's window rolls: a day's bars usually ride in the NEXT few day files
        import glob
        for p in sorted(glob.glob('data/2026-*.json')):
            try:
                jj = json.load(io.open(p, encoding='utf-8'))
            except Exception:
                continue
            r2 = ((jj.get('futBars') or {}).get('ES') or {}).get('rows') or []
            if any(datetime.datetime.utcfromtimestamp((b[0] / 1000.0 if b[0] > 1e12 else b[0]) - 5 * 3600).strftime('%Y-%m-%d') == day for b in r2):
                rows = r2
                break
    out = []
    for b in rows:
        t = b[0] / 1000.0 if b[0] > 1e12 else b[0]
        d = datetime.datetime.utcfromtimestamp(t - 5 * 3600)
        if d.strftime('%Y-%m-%d') != day:
            continue
        h = d.hour * 100 + d.minute
        if win[0] <= h < win[1]:
            out.append((h, b[1], b[2], b[3], b[4]))
    return out


def strike_line(j, k, win):
    """per 3-min snap: hhmm · cur $M · d15 $M · %King · King · px"""
    out = []
    for s in (j.get('snaps') or {}).get('SPY') or []:
        h = hhmm_of_ms(s.get('bar') or s.get('t'))
        if not (win[0] <= h < win[1]):
            continue
        rows = [r for r in (((s.get('vend') or {}).get('rows')) or []) if r and r[0] >= 1000]   # SPXW only (pre-v14.2 rows carry SPY pollution)
        if not rows:
            continue
        king = None
        kmax = 0
        cur = None
        d15 = None
        for r in rows:
            if abs(r[1]) > kmax:
                kmax = abs(r[1])
                king = r[0]
            if r[0] == k:
                cur, d15 = r[1], r[3]
        pct = (round(100.0 * abs(cur) / kmax) if (cur is not None and kmax) else None)
        out.append((h, cur, d15, pct, king, s.get('px')))
    return out


def events(j, k, win):
    out = []
    for e in j.get('nodeEvents') or []:
        w = e.get('w') or {}
        if w.get('k') != k or e.get('ty') == 'ROLL':
            continue
        if not (win[0] <= w.get('hhmm', 0) < win[1]):
            continue
        out.append((w['hhmm'], e['ty'], w.get('pct'), w.get('cur'), w.get('d15'), w.get('stage'), w.get('taps'), w.get('side'), e.get('px')))
    return out


def king_path(j):
    out, last = [], None
    for s in (j.get('snaps') or {}).get('SPY') or []:
        rows = [r for r in (((s.get('vend') or {}).get('rows')) or []) if r and r[0] >= 1000]
        if not rows:
            continue
        king = max(rows, key=lambda r: abs(r[1]))[0]
        if king != last:
            out.append('%04d:%s' % (hhmm_of_ms(s.get('bar') or s.get('t')), king))
            last = king
    return out


def fmt_m(v):
    return '—' if v is None else ('%+.0fM' % (v / 1e6))


def main():
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    day = a[0]
    j = load(day)
    ratio = ES_PER_SPX
    if '--ratio' in a:
        ratio = float(a[a.index('--ratio') + 1])
    if '--king' in a:
        print('KING path (display scale):', ' '.join(king_path(j)))
        return
    if '--price' in a:
        win = parse_win(a[2] if len(a) > 2 else '')
        bars = es_bars(j, day, win)
        print('ES 1-min %s %s: %d bars' % (day, a[2] if len(a) > 2 else 'all', len(bars)))
        for h, o, hi, lo, c in bars:
            print('  %04d  %g-%g  c %g' % (h, lo, hi, c))
        return
    if '--es' in a:
        es = float(a[a.index('--es') + 1])
        spx = es / ratio
        ks = [k for k in range(int(spx // 5 * 5) - 5, int(spx // 5 * 5) + 11, 5)]
        win = parse_win(a[a.index('--es') + 2] if len(a) > a.index('--es') + 2 else '')
        print('ES %.2f ≈ SPXW %.1f → strikes %s (ES-equivalents %s)' % (es, spx, ks, [round(k * ratio, 1) for k in ks]))
    else:
        ks = [int(float(a[1]))]
        win = parse_win(a[2] if len(a) > 2 else '')
    for k in ks:
        print('\n== %s · SPXW %d (ES ≈ %.1f) · %04d–%04d' % (day, k, k * ratio, win[0], win[1]))
        line = strike_line(j, k, win)
        if not line:
            print('   no snaps in the window (the panel was not recording)')
        for h, cur, d15, pct, king, px in line:
            print('   %04d  cur %8s  d15 %6s  %4s  King %s  px %s' % (h, fmt_m(cur), fmt_m(d15), ('%d%%' % pct if pct is not None else 'n/a'), king, (round(px, 1) if isinstance(px, (int, float)) else px)))
        ev = events(j, k, win)
        if ev:
            print('   events:')
            for h, ty, pct, cur, d15, st, taps, side, px in ev:
                print('   %04d  %-7s %3s%%  cur %8s  d15 %6s  %-9s taps %s  %s  px %s' % (h, ty, pct, fmt_m(cur), fmt_m(d15), st, taps, side, round(px, 1) if isinstance(px, (int, float)) else px))


if __name__ == '__main__':
    main()
