#!/usr/bin/env python3
"""tape.py — read the tape files the panel writes (v15.66): data/tape/<day>/<BOOK>.json, the WHOLE book every bar.

    python3 tools/nightly/tape.py 2026-09-08            # coverage: books · bars · strikes · first/last bar
    python3 tools/nightly/tape.py --selftest            # builds a synthetic day and reads it back

A file is { schema, book, day, src, f, unit, strikes:[...], bars:[{t, bar, px, n, v:[aligned rows], king?, kd?, ts?}] }.
    SPXW  f = [cur, d5, d15, d60, d1d]  dollars, Skylit's own numbers for today's expiry, all strikes (~286)
    SPY · QQQ · VIX  f = [pct, vel]     %King (signed) and the pane's velocity cell; dollars = pct/100 × kd × 1000
`load(day)` returns { book: { 'strikes': [...], 'bars': [ {t, bar, px, n, king, kd, rows:{strike: [..]}} ] } } with the
rows re-expanded per bar (absent strikes omitted), which is the shape the studies read. `dollars(book, bar, k)` gives
the strike's dollar size on any book.
"""
import io, json, os, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BOOKS = ['SPXW', 'SPY', 'QQQ', 'VIX']

def tape_dir(day, root=ROOT):
    return os.path.join(root, 'data', 'tape', day)

def load(day, root=ROOT):
    """-> {book: {'f': [...], 'unit': {...}, 'strikes': [...], 'bars': [...]}} for every book file present; {} if none."""
    d = tape_dir(day, root)
    out = {}
    if not os.path.isdir(d):
        return out
    for book in BOOKS:
        p = os.path.join(d, book + '.json')
        if not os.path.isfile(p):
            continue
        j = json.load(io.open(p, encoding='utf-8'))
        if j.get('schema') != 1 or j.get('book') != book:
            raise ValueError('%s: not a schema-1 %s tape file' % (p, book))
        ks = j['strikes']
        bars = []
        for b in j['bars']:
            rows = {}
            for k, v in zip(ks, b['v']):
                if v is not None:
                    rows[float(k)] = v
            bars.append({'t': b['t'], 'bar': b['bar'], 'px': b.get('px'), 'n': b.get('n'),
                         'king': b.get('king'), 'kd': b.get('kd'), 'rows': rows})
        bars.sort(key=lambda x: x['bar'])
        out[book] = {'f': j['f'], 'unit': j.get('unit'), 'strikes': [float(k) for k in ks], 'bars': bars}
    return out

def dollars(book, bar, k):
    """the strike's dollar size on `bar` of `book`: SPXW carries dollars; the Trinity books carry %King and the King's $K."""
    row = bar['rows'].get(float(k))
    if row is None:
        return None
    if book == 'SPXW':
        return row[0]
    kd = bar.get('kd')
    if kd is None:
        return None
    return row[0] / 100.0 * kd * 1000.0

def coverage(day, root=ROOT):
    T = load(day, root)
    lines = []
    if not T:
        return ['tape %s: no files (data/tape/%s/)' % (day, day)]
    for book in BOOKS:
        if book not in T:
            lines.append('  %-4s  —' % book); continue
        bars = T[book]['bars']
        ns = [b['n'] for b in bars if b.get('n')]
        lines.append('  %-4s  %3d bars  %4d strikes (median %d/bar)  %s → %s' % (
            book, len(bars), len(T[book]['strikes']), sorted(ns)[len(ns) // 2] if ns else 0,
            _ct(bars[0]['bar']) if bars else '—', _ct(bars[-1]['bar']) if bars else '—'))
    return ['tape %s:' % day] + lines

def _ct(ms):
    import datetime
    return datetime.datetime.utcfromtimestamp(ms / 1000 - 5 * 3600).strftime('%H:%M')

def selftest():
    root = tempfile.mkdtemp()
    day = '2026-09-08'
    d = os.path.join(root, 'data', 'tape', day); os.makedirs(d)
    t0 = 1788784200000  # 08:30 CT
    spxw = {'schema': 1, 'book': 'SPXW', 'day': day, 'src': 'skylit', 'f': ['cur', 'd5', 'd15', 'd60', 'd1d'], 'unit': {},
            'strikes': [7700, 7705, 7710],
            'bars': [{'t': t0, 'bar': t0, 'px': 7702.5, 'n': 2, 'ts': t0, 'v': [[5e6, 1e5, 2e5, 3e5, 4e5], None, [-3e6, 0, 0, 0, 0]]},
                     {'t': t0 + 180000, 'bar': t0 + 180000, 'px': 7704, 'n': 3, 'ts': t0, 'v': [[6e6, 1e6, 2e5, 3e5, 4e5], [1e6, 0, 0, 0, 0], [-3.5e6, 0, 0, 0, 0]]}]}
    spy = {'schema': 1, 'book': 'SPY', 'day': day, 'src': 'skylit', 'f': ['pct', 'vel'], 'unit': {},
           'strikes': [769, 770, 771],
           'bars': [{'t': t0, 'bar': t0, 'px': 7702.5, 'n': 3, 'king': 770, 'kd': 390226, 'v': [[-45, None], [100, 2.5], [-38, None]]}]}
    json.dump(spxw, io.open(os.path.join(d, 'SPXW.json'), 'w', encoding='utf-8'))
    json.dump(spy, io.open(os.path.join(d, 'SPY.json'), 'w', encoding='utf-8'))
    T = load(day, root)
    assert set(T.keys()) == {'SPXW', 'SPY'}, T.keys()
    assert len(T['SPXW']['bars']) == 2 and 7705.0 not in T['SPXW']['bars'][0]['rows'] and T['SPXW']['bars'][1]['rows'][7705.0][0] == 1e6
    assert dollars('SPXW', T['SPXW']['bars'][1], 7700) == 6e6
    assert abs(dollars('SPY', T['SPY']['bars'][0], 769) - (-45 / 100.0 * 390226 * 1000)) < 1e-6
    assert dollars('SPY', T['SPY']['bars'][0], 772) is None
    cov = coverage(day, root)
    assert cov[0].startswith('tape 2026-09-08') and any('SPXW' in l and '2 bars' in l for l in cov) and any('QQQ' in l and '—' in l for l in cov), cov
    print('tape.py selftest ok:'); print('\n'.join(cov))

if __name__ == '__main__':
    if '--selftest' in sys.argv:
        selftest()
    elif len(sys.argv) > 1:
        print('\n'.join(coverage(sys.argv[1])))
    else:
        print(__doc__)
