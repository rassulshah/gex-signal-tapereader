#!/usr/bin/env python3
"""
THE CORPUS, CLUSTERED BY EPISODE — the statistical discipline, checked in so it re-runs.

⚠⚠ WHY THIS EXISTS. On 2026-09-03 I analysed the 11-day feature archive in a scratchpad, by hand,
and threw the code away. The numbers were right; the PROCESS was unrepeatable, sat beside none of
the ~40 study-*.py already here, and fed nothing. This is that analysis, checked in, so the next
pass is a re-run and not a re-derivation.

WHAT IT ENCODES THAT A ONE-OFF FORGETS:

 1 · EPISODES, NOT ROWS. Every bar re-records the same node with an OVERLAPPING forward window.
     1,151 node rows are 94 (date, sym, strike) episodes — 12.2 rows each. Treating rows as
     independent inflates every interval ~3.5x and manufactures significance from autocorrelation.
     This clusters first and reports the honest n. ⚠ It also reports how many episodes carry
     INTERNALLY CONTRADICTORY labels, which on 2026-09-03 was 62 of 94 — the sign that the label is
     per-bar and Q11 needs a per-EVENT one.

 2 · WILSON INTERVALS, NOT BARE PERCENTAGES. A cell reading 71% on n=21 is not a finding.

 3 · A MULTIPLE-COMPARISON LEDGER. It counts the cells searched and prints how many flags CHANCE
     ALONE predicts at alpha, beside how many were found. On 2026-09-03: ~40 cells, 2 expected,
     2 found. ⚠ THAT COMPARISON IS THE POINT. Without it a table of 40 cells always looks like it
     contains something.

 4 · LEAKAGE REFUSAL. `rr` derives from tgt/inval, which FRAME the outcome. It is excluded by name
     and the exclusion is printed, so a future reader sees the choice rather than wondering.

USAGE
    python3 tools/study-corpus-episodes.py [export.json]
"""
import json, sys, math, collections

DEFAULT = 'data/corpus/feat-2026-08-19_2026-09-02.json'
LEAK = {'rr'}                       # frames the outcome — see §4 above
ALPHA = 0.05
MIN_CELL = 15                       # below this a flag is not even considered

def wilson(k, n, z=1.96):
    if n == 0: return (0.0, 0.0)
    p = k / n; den = 1 + z*z/n
    c = (p + z*z/(2*n)) / den
    h = z * math.sqrt(p*(1-p)/n + z*z/(4*n*n)) / den
    return (max(0.0, c-h), min(1.0, c+h))

def subject_zone(rec):
    for z in rec.get('zones') or []:
        if z.get('k') == rec.get('k'): return z
    return {}

def episodes(recs):
    """Cluster to one row per (date, sym, strike); keep the FIRST bar — the ex-ante state."""
    ep = collections.defaultdict(list)
    for r in recs: ep[(r['date'], r['sym'], r['rec'].get('k'))].append(r)
    for k in ep: ep[k].sort(key=lambda r: r['t'])
    return ep

def main(path):
    d = json.load(open(path))
    N = [r for r in d if r.get('key') == 'node' and r.get('hit') is not None
         and isinstance(r.get('rec'), dict)]
    if not N:
        print('no scored node records in', path); return 1
    ep = episodes(N)
    mixed = sum(1 for v in ep.values() if len(set(x['hit'] for x in v)) > 1)

    print('=' * 74)
    print('CORPUS  %s' % path)
    print('  scored node rows      %d' % len(N))
    print('  EPISODES (honest n)   %d       rows/episode %.1f' % (len(ep), len(N)/len(ep)))
    print('  sessions              %d' % len(set(r['date'] for r in N)))
    print('  ⚠ episodes with CONTRADICTORY labels inside: %d of %d (%.0f%%)'
          % (mixed, len(ep), 100*mixed/len(ep)))
    if mixed > len(ep) * 0.25:
        print('  ⚠⚠ the label is PER-BAR, not per-event. Q11 asks about a discrete TEST.')
        print('     An event-level ledger (day.defl, deduped by strike per fresh tap) is the')
        print('     shape this question needs; this corpus can refute, not confirm.')
    print('=' * 74)

    rows = []
    for v in ep.values():
        f = v[0]; rc = f['rec']; z = subject_zone(rc)
        rows.append(dict(lab=f['hit'], grade=rc.get('grade'), pol=rc.get('pol'),
                         tap=rc.get('tap'), rocNow=rc.get('rocNow'), rocDay=rc.get('rocDay'),
                         path=rc.get('path'), role=z.get('role'), sess=f.get('session'),
                         score=rc.get('score')))
    held = sum(r['lab'] for r in rows); n = len(rows)
    base = held / n; lo, hi = wilson(held, n)
    print('\nBASE RATE   held %d/%d = %.1f%%   [95%% CI %.0f-%.0f%%]\n'
          % (held, n, 100*base, 100*lo, 100*hi))

    feats = ['grade', 'pol', 'tap', 'rocNow', 'rocDay', 'path', 'role', 'sess', 'score']
    cells = flags = 0
    for f in feats:
        if f in LEAK: continue
        c = collections.defaultdict(lambda: [0, 0])
        for r in rows:
            v = r[f] if r[f] is not None else '(none)'
            c[v][0] += r['lab']; c[v][1] += 1
        if len(c) < 2: continue
        print('-- %s' % f)
        for v, (h, nn) in sorted(c.items(), key=lambda x: -x[1][1]):
            l, u = wilson(h, nn)
            cells += 1
            hit = nn >= MIN_CELL and (l > base or u < base)
            if hit: flags += 1
            print('     %-12s %3d/%-3d = %5.1f%%  [%3.0f-%3.0f%%]%s'
                  % (str(v)[:12], h, nn, 100*h/nn, 100*l, 100*u, '  <- excludes base' if hit else ''))
        print()

    exp = cells * ALPHA
    print('=' * 74)
    print('MULTIPLE-COMPARISON LEDGER')
    print('  cells searched            %d' % cells)
    print('  flags CHANCE predicts     %.1f   (alpha=%.2f)' % (exp, ALPHA))
    print('  flags found               %d' % flags)
    print('  excluded as leakage       %s' % (', '.join(sorted(LEAK)) or 'none'))
    if flags <= exp + 1:
        print('\n  ⚠⚠ FOUND IS WHAT CHANCE PREDICTS. NOTHING HERE IS A FINDING.')
        print('  Do not re-cut this sample looking for one — that is how a search becomes a')
        print('  result. Pre-register a hypothesis and test it on sessions this run never saw.')
    else:
        print('\n  more flags than chance predicts — still needs a FORWARD test before use.')
    print('=' * 74)
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else DEFAULT))
