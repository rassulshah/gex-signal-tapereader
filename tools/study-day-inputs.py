#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
study-day-inputs.py — EVERY INPUT WITH A HISTORY, tested against the day model's range and placement.

Operator, 2026-09-16, after the EM study: "have you considered all the indicators". The EM study tested the EM and the
tape's opening range only. This pass tests every other candidate that HAS a history in the corpus, on the same 299
sessions, same folds, same MAE, so nothing is left to opinion:

  RANGE candidates (added to the best EM model at each stage):
    on_rng   overnight (Globex 17:00 → 08:30 CT) range, points        — the night's realized vol
    gap      |open − prior RTH close|                                  — studied before for DIRECTION (null); here for RANGE
    pdr      prior-day RTH range (already known: dead beside the EM)
    dow      day of week (as five dummies)                             — the old weekday model
    vix_c    VIX1D prior close (the next-day implied, incl. overnight) — beside the open reading
  PLACEMENT candidates (the upside share f = (HOD − open) / range), PRE-OPEN and at 30 / 60 min:
    pos_pd   where the open sits in the PRIOR day's range (0 = at PDL, 1 = at PDH)
    pos_on   where the open sits in the OVERNIGHT range (0 = at ONL, 1 = at ONH)
    gap_s    signed gap / EM (gap up → ?)
    pos30 / pos60 (already known)

NOT testable here (no history in the corpus): the regime sign, the distance to the 0DTE flip, the walls, the King,
Trinity — IF and Skylit levels are recorded only since 2026-08-24 (~17 days). They need a recorded daily series first.
"""
import collections, csv, importlib.util, io, math, os, statistics as st

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('em', os.path.join(HERE, 'study-em-range.py'))
em = importlib.util.module_from_spec(spec); spec.loader.exec_module(em)
sh = em.sh


def overnight(paths):
    """Globex session per RTH day: minutes from 17:00 CT (prior calendar day) to 08:29 CT → (high, low, last close)."""
    night = collections.defaultdict(list)
    import datetime as dt
    for path in paths:
        with sh._open(path) as f:
            head = f.readline(); f.seek(0)
            rows = []
            if '\t' in head and not head.lower().startswith('symbol'):
                for line in f:
                    p = line.rstrip('\r\n').split('\t')
                    if len(p) < 6 or 'T' not in p[1]:
                        continue
                    d, t = p[1].strip().split('T', 1)
                    rows.append((d, t, float(p[3]), float(p[4]), float(p[5])))
            else:
                for x in csv.DictReader(f):
                    s = (x.get('Date') or '').strip()
                    if ' ' not in s:
                        continue
                    d, t = s.split(' ', 1)
                    rows.append((d, t, float(x['High']), float(x['Low']), float(x['Close'])))
        for d, t, h, l, c in rows:
            hh, mm = int(t[0:2]), int(t[3:5])
            sec = hh * 3600 + mm * 60
            if sec >= 17 * 3600:
                day = (dt.date.fromisoformat(d) + dt.timedelta(days=1))
                while day.weekday() >= 5:
                    day += dt.timedelta(days=1)
                night[day.isoformat()].append((sec, h, l, c))
            elif sec < sh.LOAD_A:
                night[d].append((sec + 86400, h, l, c))
    out = {}
    for d, m in night.items():
        if len(m) < 600:
            continue
        m.sort()
        out[d] = dict(onh=max(x[1] for x in m), onl=min(x[2] for x in m), onc=m[-1][3])
    return out


def main():
    rows = em.sessions('ES')
    vix = em.load_vix1d(os.path.join(HERE, '..', 'data', 'vix1d-daily.txt'))
    vdays = sorted(vix); K = 1 / math.sqrt(252)
    night = overnight(sh.market_sources('ES'))
    import datetime as dt
    for i, r in enumerate(rows):
        prev = [d for d in vdays if d < r['day']]
        r['em_o'] = r['open'] * vix[r['day']][0] / 100 * K if r['day'] in vix else None
        r['vix_c'] = r['open'] * vix[prev[-1]][1] / 100 * K if prev else None
        r['pdr'] = rows[i - 1]['rng'] if i > 0 else None
        p = rows[i - 1] if i > 0 else None
        r['gap'] = abs(r['open'] - p['close']) if p else None
        r['gap_s'] = (r['open'] - p['close']) / r['em_o'] if (p and r['em_o']) else None
        r['pos_pd'] = (r['open'] - p['lo']) / (p['hi'] - p['lo']) if p and p['hi'] > p['lo'] else None
        n = night.get(r['day'])
        r['on_rng'] = (n['onh'] - n['onl']) if n else None
        r['pos_on'] = (r['open'] - n['onl']) / (n['onh'] - n['onl']) if n and n['onh'] > n['onl'] else None
        wd = dt.date.fromisoformat(r['day']).weekday()
        for k in range(1, 5):
            r['dow%d' % k] = 1.0 if wd == k else 0.0
    base = [r for r in rows if all(r.get(k) is not None for k in ('em_o', 'pdr', 'gap', 'pos_pd', 'on_rng', 'pos_on', 'vix_c'))]
    print('sessions with every input: %d of %d (%s → %s); overnight sessions found %d' % (len(base), len(rows), base[0]['day'], base[-1]['day'], len(night)))

    def oof_range(feats):
        return em.oof(base, feats)

    print('\n== RANGE — each candidate ADDED to the EM model, out-of-fold MAE (n=%d) ==' % len(base))
    print('%-46s %6s %7s %7s  %s' % ('model', 'MAE', 'MAE-HI', 'MAE-LO', 'fit'))
    for stage, basef in (('pre-open', ['em_o']), ('30 min', ['or30', 'em_o']), ('60 min', ['or60', 'em_o'])):
        b = oof_range(basef)
        print('%-46s %6.2f %7.2f %7.2f  %s' % (stage + ': EM model (reference)', b['mae'], b['mae_hi'], b['mae_lo'], b['beta']))
        for name, extra in (('+ overnight range', ['on_rng']), ('+ |gap|', ['gap']), ('+ prior-day range', ['pdr']),
                            ('+ VIX1D prior close', ['vix_c']), ('+ day of week', ['dow1', 'dow2', 'dow3', 'dow4']),
                            ('+ overnight range + |gap|', ['on_rng', 'gap'])):
            r = oof_range(basef + extra)
            d = r['mae'] - b['mae']
            print('%-46s %6.2f %7.2f %7.2f  %s  (%+.2f)' % ('   ' + name, r['mae'], r['mae_hi'], r['mae_lo'], r['beta'], d))

    # placement: predict f = (hi-open)/rng; score MAE of eHi/eLo with the stage's EM range model
    def oof_place(rng_feats, pos_feats):
        idx = base; n = len(idx); eh = []; el = []
        for f in range(10):
            test = idx[f * n // 10:(f + 1) * n // 10]; ts = set(id(x) for x in test)
            train = [x for x in idx if id(x) not in ts]
            br = em.ols([[x[ft] for ft in rng_feats] for x in train], [x['rng'] for x in train])
            bf = em.ols([[x[ft] for ft in pos_feats] for x in train], [x['up'] / x['rng'] for x in train]) if pos_feats else None
            for r in test:
                p = em.predict(br, [r[ft] for ft in rng_feats])
                fr = min(0.95, max(0.05, em.predict(bf, [r[ft] for ft in pos_feats]))) if bf else 0.5
                eh.append(abs(r['open'] + fr * p - r['hi'])); el.append(abs(r['open'] - (1 - fr) * p - r['lo']))
        bf_all = em.ols([[x[ft] for ft in pos_feats] for x in idx], [x['up'] / x['rng'] for x in idx]) if pos_feats else None
        return st.mean(eh), st.mean(el), [round(b, 3) for b in bf_all] if bf_all else '-'

    print('\n== PLACEMENT — upside share predicted from each candidate; MAE of E-HOD / E-LOD (n=%d) ==' % len(base))
    print('%-46s %7s %7s  %s' % ('model', 'MAE-HI', 'MAE-LO', 'upside-share fit'))
    for stage, rf, cands in (
            ('pre-open (EM range)', ['em_o'], [('symmetric (today)', []), ('pos_on: open in overnight range', ['pos_on']),
                                               ('pos_pd: open in prior-day range', ['pos_pd']), ('gap_s: signed gap / EM', ['gap_s']),
                                               ('pos_on + pos_pd', ['pos_on', 'pos_pd']), ('pos_on + pos_pd + gap_s', ['pos_on', 'pos_pd', 'gap_s'])]),
            ('30 min (OR30 + EM range)', ['or30', 'em_o'], [('symmetric (today)', []), ('pos30', ['pos30']), ('pos30 + pos_on', ['pos30', 'pos_on']),
                                                          ('pos30 + pos_pd', ['pos30', 'pos_pd']), ('pos30 + pos_on + pos_pd + gap_s', ['pos30', 'pos_on', 'pos_pd', 'gap_s'])]),
            ('60 min (IB + EM range)', ['or60', 'em_o'], [('symmetric (today)', []), ('pos60', ['pos60']), ('pos60 + pos_on', ['pos60', 'pos_on']),
                                                        ('pos60 + pos_pd', ['pos60', 'pos_pd']), ('pos60 + pos_on + pos_pd + gap_s', ['pos60', 'pos_on', 'pos_pd', 'gap_s'])])):
        for name, pf in cands:
            mh, ml, bf = oof_place(rf, pf)
            print('%-46s %7.2f %7.2f  %s' % (stage + ': ' + name, mh, ml, bf))


if __name__ == '__main__':
    main()
