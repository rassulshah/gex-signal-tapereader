#!/usr/bin/env python3
"""
DAY DIGEST — turn a 4MB day export into something a review can actually read.

    python3 tools/day-digest.py data/2026-08-27.json > /tmp/digest.json

⚠⚠ WHY THIS EXISTS. The nightly review was instructed to READ `data/<date>.json` directly. Measured
2026-08-28:

    data/2026-08-27.json    4.2 MB  ~= 1,041,000 tokens
    the review model                    200,000 tokens
    -> the file is 5.2x the ENTIRE context window; the weekly reads all of them, ~7M tokens

The correlation is exact: the last review that ever landed was **2026-08-18**, the last day the file
was 1.3MB. Every day from 08-19 (4.3MB) onward produced nothing. Ten days of scheduled runs, one
216-byte artefact, and every rule in learning/rules.json still reads n=0 - so NOTHING in the learning
layer has ever been measured.

⚠ AND THE PROJECT ALREADY KNEW. DECISIONS.md D-11, 2026-08-24: "a day export is 5.9 MB ... The
archive needs a digest." It was measured, written down, and the review was pointed at the raw file
anyway. The repo is the first place to look.

WHAT THIS EMITS: aggregates only, never raw bars. Per-feature n / resolved / hit-rate / VOTE-SPLIT /
MFE / MAE, per-regime slices, effective n, data health, and the node-event ledger summarised. Target
is well under 200KB.

⚠ IT NEVER INVENTS. A field that is absent comes out absent, and `dataHealth` says so loudly - a
review over a collapsed day file must report the collapse, not average over it.
"""
import json, sys, io, collections, os


def pct(a, b):
    return round(100.0*a/b) if b else None


def digest(path):
    with io.open(path, encoding='utf-8') as f:
        d = json.load(f)
    out = {
        'schema': 'gex-digest/v1',
        'source': os.path.basename(path),
        'sourceBytes': os.path.getsize(path),
        'date': d.get('date'),
        'panelVersion': d.get('version'),
        'rulesAsOf': d.get('rulesAsOf'),
        'generatedBy': 'tools/day-digest.py',
    }
    # ---- snapshots: count only, never the bars themselves ------------------------------------
    snaps = d.get('snaps') or {}
    out['snaps'] = {s: len(v) for s, v in snaps.items() if isinstance(v, list)}

    # ---- DATA HEALTH. The most important block: a review must know the input is degraded. ----
    feat = d.get('feat') or {}
    health = {}
    for sym, arr in feat.items():
        if not isinstance(arr, list):
            continue
        bars = {r.get('bar') for r in arr if isinstance(r, dict)}
        keys = {r.get('key') for r in arr if isinstance(r, dict)}
        res = sum(1 for r in arr if isinstance(r, dict) and r.get('resolved'))
        nsnap = len(snaps.get(sym) or [])
        health[sym] = {
            'records': len(arr), 'distinctBars': len(bars), 'distinctKeys': len(keys),
            'resolved': res, 'snapshots': nsnap,
            'barsCoveredPct': pct(len(bars), nsnap),
            # ⚠ THE COLLAPSE TEST. A healthy day has feature records on most bars. 08-20 had 122
            # bars against 131 snapshots; 08-27 had ONE against 133.
            'COLLAPSED': (nsnap > 20 and len(bars) < max(5, nsnap*0.25)),
        }
    out['dataHealth'] = health
    out['dataHealthVerdict'] = (
        'COLLAPSED — feature records cover almost no bars. Report this and do NOT compute rates over it.'
        if any(h.get('COLLAPSED') for h in health.values())
        else 'ok')

    # ---- per-feature aggregates, with the vote split the honesty rules demand -----------------
    feats = {}
    for sym, arr in feat.items():
        if not isinstance(arr, list):
            continue
        by = collections.defaultdict(lambda: dict(n=0, resolved=0, hit=0, miss=0,
                                                  mfe=[], mae=[], votes=collections.Counter(),
                                                  byRegime=collections.defaultdict(lambda: [0, 0])))
        for r in arr:
            if not isinstance(r, dict):
                continue
            b = by[r.get('key')]
            b['n'] += 1
            if r.get('resolved'):
                b['resolved'] += 1
                if r.get('hit') == 1 or r.get('hit') is True:
                    b['hit'] += 1
                elif r.get('hit') == 0 or r.get('hit') is False:
                    b['miss'] += 1
            for k2 in ('mfe', 'mae'):
                v = r.get(k2)
                if isinstance(v, (int, float)):
                    b[k2].append(v)
            rec = r.get('rec') or {}
            if isinstance(rec, dict):
                # ⚠ THE VOTE SPLIT. A factor that fires one way on a trending day earns accuracy for
                # free - this is how a 71% "edge" got reported on 2026-08-11. Never omit it.
                for cand in ('verdict', 'dir', 'side', 'call'):
                    if isinstance(rec.get(cand), str):
                        b['votes'][rec[cand]] += 1
                        break
                rg = rec.get('regime')
                if isinstance(rg, dict) and rg.get('tag'):
                    cell = b['byRegime'][rg['tag']]
                    cell[0] += 1
                    if r.get('hit') in (1, True):
                        cell[1] += 1
        for k, b in by.items():
            scored = b['hit'] + b['miss']
            feats.setdefault(sym, {})[k] = {
                'n': b['n'], 'resolved': b['resolved'], 'scored': scored,
                'rate': pct(b['hit'], scored),
                'votes': dict(b['votes']),
                'oneWay': (max(b['votes'].values())/sum(b['votes'].values()) >= 0.9) if b['votes'] else None,
                'mfe': round(sum(b['mfe'])/len(b['mfe']), 3) if b['mfe'] else None,
                'mae': round(sum(b['mae'])/len(b['mae']), 3) if b['mae'] else None,
                'byRegime': {rg: {'n': c[0], 'rate': pct(c[1], c[0])} for rg, c in b['byRegime'].items()},
            }
    out['features'] = feats

    # ---- effective n, stated rather than left to be rediscovered ------------------------------
    fwd = (d.get('effN') or {}).get('fwd') or 10
    out['effectiveN'] = {
        'fwd': fwd,
        'note': 'forward windows overlap: effective observations = bars / fwd, NOT bars',
        'bySym': {s: round(n/float(fwd), 1) for s, n in out['snaps'].items()},
    }
    # ---- node events, summarised -------------------------------------------------------------
    nev = d.get('nodeEvents')
    if isinstance(nev, list):
        ty = collections.Counter(e.get('ty') for e in nev if isinstance(e, dict))
        oc = collections.Counter()
        for e in nev:
            if isinstance(e, dict) and isinstance(e.get('o10'), dict):
                oc[e['o10'].get('k')] += 1
        out['nodeEvents'] = {'n': len(nev), 'byType': dict(ty), 'outcome10': dict(oc)}
    # ---- questions and rules, names only ------------------------------------------------------
    q = d.get('questions')
    if isinstance(q, list):
        out['questions'] = [x.get('id') for x in q if isinstance(x, dict)][:120]
    out['matrixRows'] = len(d.get('matrix') or [])
    return out


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(1)
    r = digest(sys.argv[1])
    s = json.dumps(r, indent=1)
    if len(sys.argv) > 2:
        io.open(sys.argv[2], 'w', encoding='utf-8').write(s)
        print('wrote %s  (%.1f KB from %.1f MB)'
              % (sys.argv[2], len(s)/1024.0, r['sourceBytes']/1e6), file=sys.stderr)
    else:
        print(s)
