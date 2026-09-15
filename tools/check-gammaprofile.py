#!/usr/bin/env python3
"""check-gammaprofile.py — the unattended half of the IRT-vs-Skylit test plan (design/IRT-VS-SKYLIT-TESTING-PLAN.md).

Validates GammaProfile.csv — the ONE contract between the panel and the four RTX plugins — for freshness,
completeness, internal consistency, correct scale/sign, and the v16.17+ rows (READ / EXPMODEL / bodied DAYEXP).
It cannot read Skylit or the IRT render (that needs a live browser + computer-use, i.e. an attended run), so it
does NOT do the full Skylit↔CSV↔IRT comparison. What it DOES catch, hands-off, every checkpoint:
  - the panel stopped writing / the file went stale (ASOF old)   - a required row is missing / malformed
  - the King is internally inconsistent (KING vs KINGNOW vs the flagged STRIKE) or the SPX→ES scale drifted
  - node ranks not ordered by |pct|                              - the expected candle lost its body (old model)
  - EXPMODEL basis wrong for the clock (exante/open30/open60)

Usage:
  python3 tools/check-gammaprofile.py <path-to-GammaProfile.csv> [--now-so <ct_sec_of_day>]
Exit 0 = PASS, 1 = WARN only, 2 = FAIL. Prints a one-block report suitable to append to the daily log.
"""
import sys, re

RTH_A, RTH_B = 8*3600+30*60, 15*3600

def parse(path):
    rows = {}   # tag -> list of field-lists
    with open(path) as f:
        for ln in f:
            ln = ln.rstrip('\r\n')
            if not ln or ln.startswith('#'):
                continue
            parts = ln.split(',')
            rows.setdefault(parts[0], []).append(parts[1:])
    return rows

def num(x):
    try: return float(x)
    except: return None

def main(argv):
    if not argv:
        print('usage: check-gammaprofile.py <csv> [--now-so N]'); return 2
    path = argv[0]
    now_so = None
    if '--now-so' in argv:
        try: now_so = int(argv[argv.index('--now-so')+1])
        except: pass
    try:
        R = parse(path)
    except Exception as e:
        print('FAIL: cannot read %s: %s' % (path, e)); return 2

    fails, warns, oks = [], [], []
    def ok(m):   oks.append(m)
    def warn(m): warns.append(m)
    def fail(m): fails.append(m)

    # ---- freshness ----
    asof = None
    if 'ASOF' in R and R['ASOF'] and num(R['ASOF'][0][0]) is not None:
        asof = int(num(R['ASOF'][0][0]))
        if now_so is not None:
            age = now_so - asof
            if age < 0: age += 86400
            if age > 600: fail('STALE: ASOF %d is %dm old (panel not writing?)' % (asof, age//60))
            else: ok('fresh: ASOF %d, %ds old' % (asof, age))
        else:
            ok('ASOF %d (no now-so given; freshness unchecked)' % asof)
    else:
        fail('no ASOF row')
    in_rth = (asof is not None and RTH_A <= asof < RTH_B)

    # ---- required rows ----
    for tag, mn in [('STRIKE',20),('KING',1),('SPOT',1),('WEEKDAY',1),('BOOK',1),
                    ('DAYACT',1),('DAYSA',1),('DAYEXP',1),('DAYSE',1),('KINGNOW',1)]:
        n = len(R.get(tag, []))
        if n < mn: fail('missing/short row: %s (%d < %d)' % (tag, n, mn))
    if not fails:
        ok('all required rows present (%d STRIKE lines)' % len(R.get('STRIKE',[])))

    # ---- King internal consistency (KING == the kingFlag STRIKE == KINGNOW SPX -> ES) ----
    king_es = num(R['KING'][0][0]) if R.get('KING') else None
    flagged = [s for s in R.get('STRIKE',[]) if len(s) >= 4 and s[3] == '1']
    if len(flagged) != 1:
        fail('expected exactly 1 kingFlag STRIKE, found %d' % len(flagged))
    else:
        fe = num(flagged[0][0]); fp = num(flagged[0][1])
        if king_es is not None and fe is not None and abs(king_es - fe) > 0.5:
            fail('KING %.2f != flagged STRIKE %.2f' % (king_es, fe))
        if fp is None or abs(abs(fp) - 100) > 0.01:
            warn('King STRIKE pct is %s (expected ±100)' % (flagged[0][1] if flagged[0] else '?'))
        else:
            ok('King node %.2f @ %s%% (flag ok)' % (fe, flagged[0][1]))
    # KINGNOW SPX: scale + agreement with KING
    kn_spx = [r for r in R.get('KINGNOW',[]) if len(r) >= 3 and r[1] == 'SPX']
    if kn_spx:
        espx = num(kn_spx[0][2]); strike = num(kn_spx[0][3]) if len(kn_spx[0])>3 else None
        if espx and strike:
            ratio = espx/strike
            if not (1.0003 <= ratio <= 1.0010):
                fail('SPX→ES scale off: %.2f/%.2f = %.5f (expect ~1.0006)' % (espx, strike, ratio))
            else:
                ok('SPX→ES scale ok: %.5f (strike %g → ES %.2f)' % (ratio, strike, espx))
        if king_es is not None and espx is not None and abs(king_es - espx) > 2:
            warn('KING %.2f vs KINGNOW SPX ES %.2f differ by >2 (roll between writes?)' % (king_es, espx))

    # ---- node ranks ordered by |pct| ----
    ranked = sorted([(int(s[2]), abs(num(s[1]) or 0), s[0]) for s in R.get('STRIKE',[]) if len(s)>=3 and s[2].isdigit()],
                    key=lambda t: t[0])[:5]
    if len(ranked) >= 3:
        mags = [m for _,m,_ in ranked]
        if mags != sorted(mags, reverse=True):
            warn('top-5 node ranks not ordered by |pct|: %s' % mags)
        else:
            ok('top-5 nodes ordered by |pct|: %s' % mags)

    # ---- the v16.17+ rows (only expected during RTH from a live v16.22 write) ----
    has_read = 'READ' in R
    has_expmodel = 'EXPMODEL' in R
    if in_rth:
        if not has_read: warn('no READ row (v16.17+); panel may be pre-16.17 or D.ok false')
        else: ok('READ row present: %s' % ','.join(R['READ'][0]))
        if not has_expmodel: warn('no EXPMODEL row (v16.18+)')
        else:
            basis = R['EXPMODEL'][0][0] if R['EXPMODEL'][0] else '?'
            # basis sanity vs the clock
            mins = (asof - RTH_A)//60 if asof else 0
            want = 'exante' if mins < 30 else ('open30' if mins < 60 else 'open60')
            if basis != want and not (want=='open60' and basis=='open30'):
                warn('EXPMODEL basis "%s" but %dm into RTH suggests "%s"' % (basis, mins, want))
            else:
                ok('EXPMODEL basis "%s" ok for %dm in' % (basis, mins))
    else:
        ok('outside RTH (ASOF %s) — READ/EXPMODEL/body not required' % (asof))

    # ---- DAYEXP body (v16.18: close != open) ----
    if R.get('DAYEXP') and len(R['DAYEXP'][0]) >= 4:
        o, c = num(R['DAYEXP'][0][0]), num(R['DAYEXP'][0][3])
        if o is not None and c is not None:
            if in_rth and abs(o - c) < 0.01:
                warn('DAYEXP close==open (no body) during RTH — old symmetric model, expected adaptive')
            elif abs(o - c) >= 0.01:
                ok('DAYEXP has a body (close-open %.2f)' % (c-o))

    # ---- DAYSA / DAYSE shape ----
    for tag in ('DAYSA','DAYSE'):
        if R.get(tag) and len(R[tag][0]) < 15:
            fail('%s has %d fields (expected ≥15)' % (tag, len(R[tag][0])))

    # ---- report ----
    verdict = 'FAIL' if fails else ('WARN' if warns else 'PASS')
    print('=== check-gammaprofile: %s  (ASOF %s) ===' % (verdict, asof))
    for m in fails: print('  FAIL  ' + m)
    for m in warns: print('  warn  ' + m)
    for m in oks:   print('  ok    ' + m)
    return 2 if fails else (1 if warns else 0)

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
