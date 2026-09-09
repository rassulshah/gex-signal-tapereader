#!/usr/bin/env python3
"""bake-hodlod.py — (v15.87) re-bake the panel's HODLOD_BASE literal from data/es-1min/BASERATES.json.

    python3 tools/bake-hodlod.py            # rewrite the literal in current/gex-signal-tapereader.user.js
    python3 tools/bake-hodlod.py --check    # exit 1 if the literal and the file disagree (test_hodlod b5/b6/s2)

WHY. The literal is the panel's BOOT fallback for the ⓪a base rates; the companion couriers the live file over it.
test_hodlod pins the literal EQUAL to the file so "panel and evidence cannot drift" — which was fine while the corpus
changed once a build. From v15.87 the corpus appends itself every night (tools/nightly/run.py → append-futures →
study-hodlod), so the file moves without a build and the literal must be re-baked at build time, by this tool, not by
hand. The teaching comments inside the literal are kept verbatim below; only the numbers come from the file.

The normalisation is hlBaseNormalise's, field for field (n · first · last · ladder[{w,rate,n,held}] · the trimmed means
· the medians for contrast · the wick family with its own n · lodFirstPct · byDow with `recent`).
"""
import io, json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPT = os.path.join(ROOT, 'current', 'gex-signal-tapereader.user.js')
BASE = os.path.join(ROOT, 'data', 'es-1min', 'BASERATES.json')


def num(x):
    if x is None:
        return 'null'
    if isinstance(x, bool):
        return 'true' if x else 'false'
    if isinstance(x, float) and x == int(x) and abs(x) < 1e9:
        return ('%d' % int(x)) if False else repr(x)
    return repr(x) if isinstance(x, (int, float)) else json.dumps(x, ensure_ascii=False)


def wick_block(W, X):
    W = W or {}; X = X or {}
    return ('{ bop:%s, wick:%s, mud:%s, wickPct:%s,\n          bopMed:%s, wickMed:%s, mudMed:%s, wickPctMed:%s,\n          bop_n:%s, wick_n:%s, mud_n:%s, wickPct_n:%s,\n          zeroWick:%s, neverReclaimed:%s }'
            % (num(W.get('bop')), num(W.get('wick')), num(W.get('mud')), num(W.get('wick_pct')),
               num(W.get('bop_median')), num(W.get('wick_median')), num(W.get('mud_median')), num(W.get('wick_pct_median')),
               num(W.get('bop_n')), num(W.get('wick_n')), num(W.get('mud_n')), num(W.get('wick_pct_n')),
               num(X.get('zero_wick')), num(X.get('never_reclaimed'))))


def dow_block(b):
    E = b.get('expected') or {}; M = E.get('median_for_contrast') or {}; W = (b.get('wickFamily') or {}).get('median') or {}
    R = b.get('recent') or {}; S = b.get('sequence') or {}
    d = dict(n=b.get('sessions'), first=b.get('first'), last=b.get('last'),
             tookMin=E.get('took_min'), gapMin=E.get('gap_min'), rngPts=E.get('rng_pts'), rngUsd=E.get('rng_usd'),
             rngP25=E.get('rng_p25'), rngP75=E.get('rng_p75'), firstClock=E.get('first_clock'), secondClock=E.get('second_clock'),
             medTook=M.get('took_min'), medGap=M.get('gap_min'), medRng=M.get('rng_pts'), lodFirstPct=S.get('pct_LOD_first'),
             wick=dict(bop=W.get('bop'), wick=W.get('wick'), mud=W.get('mud'), wickPct=W.get('wick_pct'),
                       bopMed=W.get('bop_median'), wickMed=W.get('wick_median'), mudMed=W.get('mud_median'), wickPctMed=W.get('wick_pct_median'),
                       bop_n=W.get('bop_n'), wick_n=W.get('wick_n'), mud_n=W.get('mud_n'), wickPct_n=W.get('wick_pct_n')),
             recent=dict(n=R.get('n'), green=R.get('green'), red=R.get('red'), last=(R.get('days') or [None])[-1], rngPts=R.get('rng_pts')))
    return json.dumps(d, ensure_ascii=False, separators=(',', ':'))


def render(J):
    C = J['corpus']; E = J['expected']; M = E.get('median_for_contrast') or {}; S = J.get('sequence') or {}
    L = J['ladder']['both']; W = (J.get('wickFamily') or {}).get('median'); X = (J.get('wickFamily') or {}).get('excluded')
    rungs = ', '.join('{w:%d,rate:%s,n:%s,held:%s}' % (int(w), num(L[w].get('rate')), num(L[w].get('n')), num(L[w].get('held')))
                      for w in sorted(L, key=int))
    dows = ',\n'.join('    %s: %s' % (d, dow_block(J['byWeekday'][d])) for d in ('Mon', 'Tue', 'Wed', 'Thu', 'Fri') if d in (J.get('byWeekday') or {}))
    defn = C.get('definition') or ''
    return '''var HODLOD_BASE = {
  // (v15.87) RE-BAKED BY tools/bake-hodlod.py FROM data/es-1min/BASERATES.json — never edited by hand. The corpus
  // appends itself every night (the nightly runs append-futures + study-hodlod), so the file moves without a build and
  // this literal is the boot fallback as of the build; the companion couriers the live file over it (v14.59).
  // Definition: %s
  n: %s, first: %s, last: %s,
  // ⚠ `held` WAS MISSING AND THE HOVER PRINTED "undefined of 1169" FROM v14.57 THROUGH v14.58.
  // 42 assertions passed over it because not one of them executed the hover text. That is failure
  // pattern #8 exactly: a test that greps the source instead of running it. test_hodlod now
  // renders the tip and greps the OUTPUT for 'undefined'.
  ladder: [%s],
  // ⚠⚠ EVERY E FIELD IS A TRIMMED MEAN, not a median. Operator 2026-08-28, after I had switched
  // only the wick columns: "i thought they were all averages." He was right and the split was mine
  // - one row must be one statistic. Tukey 1.5xIQR outliers are excluded before averaging, per
  // "crazy outliers should not be averaged"; the fence removed 8 range and 15 first-clock outliers.
  // ⚠ WHAT MOVED (v14.62), so nobody re-derives the old numbers and thinks the study broke:
  //     Took 21m -> 34m · HL Gap 3h58 -> 3h50 · Rng 56.5 -> 61.4pts · 1st 8:51 -> 9:03
  // The medians are kept below purely for the hover's mean-vs-median disclosure.
  tookMin: %s, gapMin: %s,
  rngPts: %s, rngUsd: %s,
  rngP25: %s, rngP75: %s,
  firstClock: %s, secondClock: %s,
  medTook: %s, medGap: %s, medRng: %s, medFirstClock: %s, medSecondClock: %s,
  lodFirstPct: %s,
  // (v14.61) THE WICK FAMILY, MEASURED over the same sessions that reproduce the ladder. The vendor
  // corpus reached GitHub 2026-08-28 as `data/es-1min/ES TestingData.txt` (406,155 rows, EPM26).
  // ⚠ THESE ARE TRIMMED MEANS, NOT MEDIANS. Operator: "the e row is the expected result based on
  // AVERAGES", plus "no wick days should not be averaged. also crazy outliers should not be
  // averaged." So: drop the zero-wick days and the Tukey 1.5xIQR outliers, THEN average.
  // ⚠ EACH FIELD CARRIES ITS OWN n because the exclusions bite differently. A shared n would
  // overstate all of them.
  // ⚠ The median is very different on this right-skewed data (BOP mean ~14m vs median ~7m) and the
  // hover says so - the choice of statistic is his, and it is disclosed rather than assumed.
  wick: %s,
  // (v15.77) THE SAME ROW PER WEEKDAY — his seasonality. Generated from BASERATES.json byWeekday by the
  // build (tools/study-hodlod.py); the courier replaces it the same way it replaces the rest. Each
  // weekday is ~55-60 sessions: a fifth of the corpus, and the face says so. `recent` is his tool's
  // "last 6 per weekday" — the colour of the last six sessions of that weekday, a COUNT with its
  // last day, never a rate.
  byDow: {
%s
  }
};
''' % (defn, num(C.get('sessions')), json.dumps(C.get('first')), json.dumps(C.get('last')), rungs,
       num(E.get('took_min')), num(E.get('gap_min')), num(E.get('rng_pts')), num(E.get('rng_usd')),
       num(E.get('rng_p25')), num(E.get('rng_p75')), num(E.get('first_clock')), num(E.get('second_clock')),
       num(M.get('took_min')), num(M.get('gap_min')), num(M.get('rng_pts')), num(M.get('first_clock')), num(M.get('second_clock')),
       num(S.get('pct_LOD_first')), wick_block(W, X), dows)


def main(argv):
    J = json.load(io.open(BASE, encoding='utf-8'))
    s = io.open(SCRIPT, encoding='utf-8').read()
    a = s.index('var HODLOD_BASE = {'); b = s.index('\n};\n', a) + 4
    new = render(J)
    cur = s[a:b]
    if '--check' in argv:
        same = (cur.strip() == new.strip())
        print('bake-hodlod: the literal %s the file (%s sessions through %s)' % ('EQUALS' if same else 'DIFFERS FROM', J['corpus']['sessions'], J['corpus']['last']))
        return 0 if same else 1
    if cur.strip() == new.strip():
        print('bake-hodlod: already current (%s sessions through %s)' % (J['corpus']['sessions'], J['corpus']['last']))
        return 0
    io.open(SCRIPT, 'w', encoding='utf-8').write(s[:a] + new + s[b:])
    print('bake-hodlod: re-baked HODLOD_BASE — %s sessions, %s → %s' % (J['corpus']['sessions'], J['corpus']['first'], J['corpus']['last']))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
