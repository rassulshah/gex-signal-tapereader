#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
study-key-levels.py — DO THE KEY LEVELS IMPROVE THE EXPECTED HIGH / LOW AND THE RANGE?

Operator, 2026-09-16: "I am particularly interested in whether key levels (which you have) like overnight high and low
and day high and low etc., can improve these numbers even more."

Levels per session, all from the corpus (the sweep study's definitions, tools/study-sweeps.py):
  ONH/ONL  the Globex session 17:00 → 08:29 CT   PDH/PDL  the prior RTH high/low   PDC  the prior RTH close
  AHI/ALO  Asia 17:00 → 02:00                     LHI/LLO  London 02:00 → 08:29     PMH/PML  pre-market 07:00 → 08:29
  PWH/PWL  the prior ISO week's RTH high/low
Four questions, each scored out-of-fold on the same sessions as the EM study (tools/study-em-range.py):
  A  RANGE   — do level distances (the room to the nearest level above / below, the level envelope) add to the EM model?
  B  PLACE   — does the open's position among the levels (room up vs room down) predict the day's upside share,
               pre-open (where nothing has worked so far) or after the opening range?
  C  MAGNET  — does the day's HIGH / LOW land ON a level more often than chance? (within 2 / 3 / 5 pts, against the
               rate a random point in the same span would score)
  D  SNAP    — the practical rule: move the model's E-HOD / E-LOD to the nearest level when one sits within X pts of
               it. Does the error fall? And a CAP: when the model's E-HOD overshoots the nearest level above by d, how
               often does the day actually get through — is a cap warranted?
"""
import collections, csv, datetime as dt, importlib.util, io, json, math, os, statistics as st

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('em', os.path.join(HERE, 'study-em-range.py'))
em = importlib.util.module_from_spec(spec); spec.loader.exec_module(em)
sh = em.sh


def minutes(paths):
    """every minute of the corpus as (trading-day, sec-of-day-from-17:00-prev, h, l, c) — trading day = RTH date"""
    out = collections.defaultdict(list)
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
            sec = int(t[0:2]) * 3600 + int(t[3:5]) * 60
            if sec >= 17 * 3600:
                day = dt.date.fromisoformat(d) + dt.timedelta(days=1)
                while day.weekday() >= 5:
                    day += dt.timedelta(days=1)
                out[day.isoformat()].append((sec - 17 * 3600, h, l, c))          # 0 = 17:00 prev
            else:
                out[d].append((sec + 7 * 3600, h, l, c))                          # 08:30 → 15.5h = 55800
    return out


def levels(rows, mins):
    H = lambda m: max(x[1] for x in m) if m else None
    L = lambda m: min(x[2] for x in m) if m else None
    by_day = {r['day']: r for r in rows}
    days = [r['day'] for r in rows]
    for i, r in enumerate(rows):
        m = sorted(mins.get(r['day'], []))
        on = [x for x in m if x[0] < 15.5 * 3600]                       # 17:00 → 08:29
        asia = [x for x in on if x[0] < 9 * 3600]                        # 17:00 → 02:00
        ldn = [x for x in on if x[0] >= 9 * 3600]                        # 02:00 → 08:29
        pm = [x for x in on if x[0] >= 14 * 3600]                        # 07:00 → 08:29
        r['lv'] = {}
        if len(on) >= 600:
            r['lv'].update(ONH=H(on), ONL=L(on), AHI=H(asia), ALO=L(asia), LHI=H(ldn), LLO=L(ldn), PMH=H(pm), PML=L(pm))
        if i > 0:
            p = rows[i - 1]
            r['lv'].update(PDH=p['hi'], PDL=p['lo'], PDC=p['close'])
        # prior ISO week RTH high / low
        wk = dt.date.fromisoformat(r['day']).isocalendar()[1]; yr = dt.date.fromisoformat(r['day']).isocalendar()[0]
        prev = [q for q in rows[:i] if (dt.date.fromisoformat(q['day']).isocalendar()[0], dt.date.fromisoformat(q['day']).isocalendar()[1]) != (yr, wk)]
        if prev:
            lw = (dt.date.fromisoformat(prev[-1]['day']).isocalendar()[0], dt.date.fromisoformat(prev[-1]['day']).isocalendar()[1])
            wkrows = [q for q in prev if (dt.date.fromisoformat(q['day']).isocalendar()[0], dt.date.fromisoformat(q['day']).isocalendar()[1]) == lw]
            if wkrows:
                r['lv'].update(PWH=max(q['hi'] for q in wkrows), PWL=min(q['lo'] for q in wkrows))
        r['lv'] = {k: v for k, v in r['lv'].items() if v is not None}


UP = ('ONH', 'PDH', 'PWH', 'AHI', 'LHI', 'PMH', 'PDC')
DN = ('ONL', 'PDL', 'PWL', 'ALO', 'LLO', 'PML', 'PDC')


def main():
    rows = em.sessions('ES')
    vix = em.load_vix1d(os.path.join(HERE, '..', 'data', 'vix1d-daily.txt'))
    K = 1 / math.sqrt(252)
    for r in rows:
        r['em_o'] = r['open'] * vix[r['day']][0] / 100 * K if r['day'] in vix else None
    mins = minutes(sh.market_sources('ES'))
    levels(rows, mins)
    base = [r for r in rows if r['em_o'] is not None and all(k in r['lv'] for k in ('ONH', 'ONL', 'PDH', 'PDL', 'PWH', 'PWL'))]
    print('sessions with every level: %d (%s → %s)' % (len(base), base[0]['day'], base[-1]['day']))
    for r in base:
        O = r['open']; lv = r['lv']
        ups = sorted(v for k, v in lv.items() if k in UP and v > O); dns = sorted((v for k, v in lv.items() if k in DN and v < O), reverse=True)
        r['roomU'] = (ups[0] - O) if ups else 80.0; r['roomD'] = (O - dns[0]) if dns else 80.0
        r['env'] = (max(lv['ONH'], lv['PDH']) - min(lv['ONL'], lv['PDL']))
        r['posEnv'] = (O - min(lv['ONL'], lv['PDL'])) / r['env'] if r['env'] > 0 else 0.5
        r['roomShare'] = r['roomU'] / (r['roomU'] + r['roomD']) if (r['roomU'] + r['roomD']) > 0 else 0.5
        r['nUp'] = len(ups); r['nDn'] = len(dns)

    # ---- A. RANGE
    print('\n== A. RANGE — level features added to the EM model (out-of-fold MAE, n=%d) ==' % len(base))
    for stage, bf in (('pre-open', ['em_o']), ('60 min', ['or60', 'em_o'])):
        ref = em.oof(base, bf)
        print('%-44s %6.2f %7.2f %7.2f' % (stage + ': EM model', ref['mae'], ref['mae_hi'], ref['mae_lo']))
        for name, extra in (('+ room to nearest level up & down', ['roomU', 'roomD']), ('+ level envelope (ON ∪ PD span)', ['env']),
                            ('+ envelope + rooms', ['env', 'roomU', 'roomD'])):
            r_ = em.oof(base, bf + extra)
            print('%-44s %6.2f %7.2f %7.2f  (%+.2f)' % ('   ' + name, r_['mae'], r_['mae_hi'], r_['mae_lo'], r_['mae'] - ref['mae']))

    # ---- B. PLACEMENT
    def oof_place(rng_feats, pos_feats):
        idx = base; n = len(idx); eh = []; el = []
        for f in range(10):
            test = idx[f * n // 10:(f + 1) * n // 10]; ts = set(id(x) for x in test)
            train = [x for x in idx if id(x) not in ts]
            br = em.ols([[x[ft] for ft in rng_feats] for x in train], [x['rng'] for x in train])
            bf = em.ols([[x[ft] for ft in pos_feats] for x in train], [(x['hi'] - x['open']) / x['rng'] for x in train]) if pos_feats else None
            for r in test:
                p = em.predict(br, [r[ft] for ft in rng_feats])
                fr = min(0.95, max(0.05, em.predict(bf, [r[ft] for ft in pos_feats]))) if bf else 0.5
                eh.append(abs(r['open'] + fr * p - r['hi'])); el.append(abs(r['open'] - (1 - fr) * p - r['lo']))
        bf_all = em.ols([[x[ft] for ft in pos_feats] for x in idx], [(x['hi'] - x['open']) / x['rng'] for x in idx]) if pos_feats else None
        return st.mean(eh), st.mean(el), [round(b, 3) for b in bf_all] if bf_all else '-'
    print('\n== B. PLACEMENT — upside share from the open\'s position among the levels (MAE E-HOD / E-LOD) ==')
    for stage, rf, cands in (('pre-open (EM range)', ['em_o'], [('symmetric', []), ('room share up/(up+down)', ['roomShare']), ('position in ON∪PD envelope', ['posEnv']),
                                                              ('both', ['roomShare', 'posEnv'])]),
                             ('60 min (IB + EM)', ['or60', 'em_o'], [('pos60 (the built recommendation)', ['pos60']), ('pos60 + room share', ['pos60', 'roomShare']),
                                                                    ('pos60 + envelope position', ['pos60', 'posEnv'])])):
        for name, pf in cands:
            mh, ml, bf = oof_place(rf, pf)
            print('%-52s %7.2f %7.2f  %s' % (stage + ': ' + name, mh, ml, bf))

    # ---- C. MAGNET — does the HOD / LOD land on a level more often than chance?
    print('\n== C. MAGNET — share of days whose HOD (LOD) sits within k pts of a level above (below) the open, vs chance ==')
    for k in (2, 3, 5):
        hitH = hitL = chH = chL = 0
        for r in base:
            O = r['open']; lv = r['lv']
            ups = [v for kk, v in lv.items() if kk in UP and v > O]; dns = [v for kk, v in lv.items() if kk in DN and v < O]
            hitH += 1 if any(abs(r['hi'] - v) <= k for v in ups) else 0
            hitL += 1 if any(abs(r['lo'] - v) <= k for v in dns) else 0
            # chance: a point uniformly placed over the day's own upside span (O → hi) — how much of that span is within k of a level
            spanU = max(1e-9, r['hi'] - O); spanD = max(1e-9, O - r['lo'])
            covU = 0.0; covD = 0.0
            for v in ups:
                a, b = max(O, v - k), min(r['hi'], v + k)
                covU += max(0.0, b - a)
            for v in dns:
                a, b = max(r['lo'], v - k), min(O, v + k)
                covD += max(0.0, b - a)
            chH += min(1.0, covU / spanU); chL += min(1.0, covD / spanD)
        n = len(base)
        print('  k=%d pts: HOD on a level %4.0f%% (chance %4.0f%%) · LOD on a level %4.0f%% (chance %4.0f%%)' % (k, 100 * hitH / n, 100 * chH / n, 100 * hitL / n, 100 * chL / n))
    # which levels catch the extreme, when one does (k=3)
    cnt = collections.Counter()
    for r in base:
        O = r['open']; lv = r['lv']
        for kk, v in lv.items():
            if kk in UP and v > O and abs(r['hi'] - v) <= 3: cnt['HOD@' + kk] += 1
            if kk in DN and v < O and abs(r['lo'] - v) <= 3: cnt['LOD@' + kk] += 1
    print('  which level (k=3): ' + ', '.join('%s %d' % (a, b) for a, b in cnt.most_common(12)))

    # ---- D. SNAP and CAP on the 60-min placed model
    print('\n== D. SNAP — move E-HOD / E-LOD to the nearest level within X pts of it (60-min placed model, out-of-fold) ==')
    idx = base; n = len(idx)
    preds = {}
    for f in range(10):
        test = idx[f * n // 10:(f + 1) * n // 10]; ts = set(id(x) for x in test)
        train = [x for x in idx if id(x) not in ts]
        br = em.ols([[x['or60'], x['em_o']] for x in train], [x['rng'] for x in train])
        bf = em.ols([[x['pos60']] for x in train], [(x['hi'] - x['open']) / x['rng'] for x in train])
        for r in test:
            p = em.predict(br, [r['or60'], r['em_o']]); fr = min(0.95, max(0.05, em.predict(bf, [r['pos60']])))
            preds[r['day']] = (r['open'] + fr * p, r['open'] - (1 - fr) * p)
    def score(X):
        eh = []; el = []
        for r in idx:
            eH, eL = preds[r['day']]; O = r['open']; lv = r['lv']
            if X > 0:
                ups = [v for kk, v in lv.items() if kk in UP and v > O]; dns = [v for kk, v in lv.items() if kk in DN and v < O]
                cu = [v for v in ups if abs(v - eH) <= X]; cd = [v for v in dns if abs(v - eL) <= X]
                if cu: eH = min(cu, key=lambda v: abs(v - eH))
                if cd: eL = min(cd, key=lambda v: abs(v - eL))
            eh.append(abs(eH - r['hi'])); el.append(abs(eL - r['lo']))
        return st.mean(eh), st.mean(el)
    for X in (0, 2, 3, 5, 8, 12):
        mh, ml = score(X)
        print('  snap within %2d pts: MAE E-HOD %6.2f  E-LOD %6.2f' % (X, mh, ml))
    print('\n== D. CAP — when E-HOD sits d pts ABOVE the nearest level above the open, how often does the day get through that level? ==')
    buckets = collections.defaultdict(lambda: [0, 0])
    for r in idx:
        eH, eL = preds[r['day']]; O = r['open']; lv = r['lv']
        ups = sorted(v for kk, v in lv.items() if kk in UP and v > O)
        if not ups: continue
        L1 = ups[0]; d = eH - L1
        b = 'below level' if d < 0 else ('0-5 over' if d < 5 else ('5-15 over' if d < 15 else '15+ over'))
        buckets[b][0] += 1; buckets[b][1] += 1 if r['hi'] > L1 + 1 else 0
    for b in ('below level', '0-5 over', '5-15 over', '15+ over'):
        n_, k_ = buckets[b]
        if n_: print('  E-HOD %-12s n=%3d  day traded through the level %3.0f%%' % (b, n_, 100 * k_ / n_))
    buckets = collections.defaultdict(lambda: [0, 0])
    for r in idx:
        eH, eL = preds[r['day']]; O = r['open']; lv = r['lv']
        dns = sorted((v for kk, v in lv.items() if kk in DN and v < O), reverse=True)
        if not dns: continue
        L1 = dns[0]; d = L1 - eL
        b = 'above level' if d < 0 else ('0-5 under' if d < 5 else ('5-15 under' if d < 15 else '15+ under'))
        buckets[b][0] += 1; buckets[b][1] += 1 if r['lo'] < L1 - 1 else 0
    for b in ('above level', '0-5 under', '5-15 under', '15+ under'):
        n_, k_ = buckets[b]
        if n_: print('  E-LOD %-12s n=%3d  day traded through the level %3.0f%%' % (b, n_, 100 * k_ / n_))


if __name__ == '__main__':
    main()


def extra():
    """C′ and D′ — the honest controls. C′: is the HOD nearer to today's levels than a HOD of the same upside distance drawn
    from another day (permutation, 20 shifts)? D′: is the day's chance of getting through a price d pts under E-HOD any
    different when that price IS a level vs when it is just a price?"""
    import random
    rows = em.sessions('ES')
    vix = em.load_vix1d(os.path.join(HERE, '..', 'data', 'vix1d-daily.txt')); K = 1 / math.sqrt(252)
    for r in rows: r['em_o'] = r['open'] * vix[r['day']][0] / 100 * K if r['day'] in vix else None
    levels(rows, minutes(sh.market_sources('ES')))
    base = [r for r in rows if r['em_o'] is not None and all(k in r['lv'] for k in ('ONH', 'ONL', 'PDH', 'PDL', 'PWH', 'PWL'))]
    n = len(base)
    print('\n== C′. MAGNET, permutation control — HOD/LOD within k pts of a level, real vs a same-distance extreme from another day ==')
    random.seed(7)
    for k in (2, 3, 5):
        real_h = sum(1 for r in base if any(abs(r['hi'] - v) <= k for kk, v in r['lv'].items() if kk in UP and v > r['open'])) / n
        real_l = sum(1 for r in base if any(abs(r['lo'] - v) <= k for kk, v in r['lv'].items() if kk in DN and v < r['open'])) / n
        ph = []; pl = []
        for s in range(20):
            sh_ = random.sample(base, n)
            ph.append(sum(1 for r, q in zip(base, sh_) if any(abs((r['open'] + (q['hi'] - q['open'])) - v) <= k for kk, v in r['lv'].items() if kk in UP and v > r['open'])) / n)
            pl.append(sum(1 for r, q in zip(base, sh_) if any(abs((r['open'] - (q['open'] - q['lo'])) - v) <= k for kk, v in r['lv'].items() if kk in DN and v < r['open'])) / n)
        print('  k=%d: HOD on a level %4.0f%% vs permuted %4.0f%% · LOD on a level %4.0f%% vs permuted %4.0f%%' % (k, 100 * real_h, 100 * st.mean(ph), 100 * real_l, 100 * st.mean(pl)))
    # D′ — through-rate at a LEVEL vs at a plain price, matched on distance from the 60-min E-HOD
    idx = base; preds = {}
    for f in range(10):
        test = idx[f * n // 10:(f + 1) * n // 10]; ts = set(id(x) for x in test)
        train = [x for x in idx if id(x) not in ts]
        br = em.ols([[x['or60'], x['em_o']] for x in train], [x['rng'] for x in train])
        bf = em.ols([[x['pos60']] for x in train], [(x['hi'] - x['open']) / x['rng'] for x in train])
        for r in test:
            p = em.predict(br, [r['or60'], r['em_o']]); fr = min(0.95, max(0.05, em.predict(bf, [r['pos60']])))
            preds[r['day']] = (r['open'] + fr * p, r['open'] - (1 - fr) * p)
    print('\n== D′. THROUGH-RATE at a level vs at a plain price the same distance under E-HOD (above E-LOD) ==')
    for side in ('H', 'L'):
        lvl = collections.defaultdict(lambda: [0, 0]); plain = collections.defaultdict(lambda: [0, 0])
        for r in idx:
            eH, eL = preds[r['day']]; O = r['open']; lv = r['lv']
            if side == 'H':
                ups = sorted(v for kk, v in lv.items() if kk in UP and v > O)
                if not ups: continue
                d = eH - ups[0]
                for dd in (-3, 2, 5, 8, 12):          # plain prices at fixed offsets under E-HOD (not levels)
                    P = eH - dd
                    if all(abs(P - v) > 1.5 for v in ups):
                        plain[dd][0] += 1; plain[dd][1] += 1 if r['hi'] > P + 1 else 0
                b = int(round(d)) if -3 <= d <= 12 else None
                if b is not None:
                    key = min((-3, 2, 5, 8, 12), key=lambda z: abs(z - d))
                    lvl[key][0] += 1; lvl[key][1] += 1 if r['hi'] > ups[0] + 1 else 0
            else:
                dns = sorted((v for kk, v in lv.items() if kk in DN and v < O), reverse=True)
                if not dns: continue
                d = dns[0] - eL
                for dd in (-3, 2, 5, 8, 12):
                    P = eL + dd
                    if all(abs(P - v) > 1.5 for v in dns):
                        plain[dd][0] += 1; plain[dd][1] += 1 if r['lo'] < P - 1 else 0
                if -3 <= d <= 12:
                    key = min((-3, 2, 5, 8, 12), key=lambda z: abs(z - d))
                    lvl[key][0] += 1; lvl[key][1] += 1 if r['lo'] < dns[0] - 1 else 0
        for dd in (-3, 2, 5, 8, 12):
            a, b = lvl[dd]; c, e = plain[dd]
            print('  %s  E-%s %+3d pts beyond the price:  level n=%3d through %3s%%   plain price n=%3d through %3s%%' % (
                side, 'HOD' if side == 'H' else 'LOD', dd, a, ('%.0f' % (100 * b / a)) if a else '-', c, ('%.0f' % (100 * e / c)) if c else '-'))


if __name__ == '__main__' and os.environ.get('EXTRA'):
    extra()
