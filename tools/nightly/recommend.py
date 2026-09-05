#!/usr/bin/env python3
"""recommend.py — the 💡 Rec tab's file (v15.70): learning/recommendations.json — proposals TO the operator, his decisions back.

Operator, 2026-09-04: "we need one other tab called recommendations. based on the entire process and what you have
learned you need to make recommendations and get my approval to implement." · "my expectation from now on is to just
click on the save button once a day probably eod, and from that point on you take over from data, analysis, testing,
learning all the way to the Rec tab, which is where we will discuss what to implement as needed."

    python3 tools/nightly/recommend.py               # update learning/recommendations.json from the newest log + results + the day files
    python3 tools/nightly/recommend.py --selftest

TWO WRITERS, ONE FILE, BY ID.  The review writes its rows in tools/rec-seed.py (ids R-n) and regenerates through
merge(); the nightly writes the MACHINE rows (ids RN-<slug>, from pre-registered conditions below) and applies HIS
DECISIONS (the day files' `reco`, exported by the panel's ✓ / ✗). Neither writer erases the other's rows or his status.

THE MACHINE'S CONDITIONS — written here BEFORE any row was generated (2026-09-04); a machine row exists only when one holds:
    RULE     a pattern class reads at n ≥ RATE_MIN_N on an outcome (held / turn / resume) with its Wilson lower bound
             ABOVE the base rate of every tap on that outcome → "register it out of sample; put the number on the face"
    TEACH    a Learn rule the record CONTRADICTS (results.json rules[].verdict) → "re-teach L-n against the record"
    RULE     a register hypothesis CLEARED → "put its number on the face" · REFUSED → its RULE row, if any, is WITHDRAWN
    WITHDRAW a proposed machine RULE whose condition no longer holds is withdrawn (status 'withdrawn', with why)
STATUSES  proposed → approved / declined (his ✓ / ✗, with an optional note) → implemented (a build sets it, with the
version) · withdrawn (the record, or the review with a reason). A row never disappears.
KINDS     RULE · TEST · FEATURE · DATA · DESIGN · PROCESS · TEACH.
"""
import glob, io, json, os, re, sys, datetime

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RATE_MIN_N = 15
REC = os.path.join(ROOT, 'learning', 'recommendations.json')
OUTCOMES = ('held', 'turn', 'resume')

def _wilson_low(right, n, z=1.96):
    import math
    if not n or n <= 0:
        return 0.0
    p = right / n; d = 1 + z * z / n; c = p + z * z / (2 * n); m = z * math.sqrt((p * (1 - p) + z * z / (4 * n)) / n)
    return max(0.0, (c - m) / d)

def _cell(r, oc):
    if not r:
        return 0, 0
    if oc == 'held':
        return r.get('n', 0), r.get('held', 0)
    x = r.get(oc) or {}
    return x.get('n', 0), x.get('hit', 0)

def empty():
    return dict(schema=1, asOf=None, writtenBy='tools/nightly/recommend.py', rows=[])

def load(path=REC):
    if os.path.exists(path):
        try:
            j = json.load(io.open(path, encoding='utf-8'))
            if j.get('schema') == 1 and isinstance(j.get('rows'), list):
                return j
        except Exception:
            pass
    return empty()

def machine_rows(log, results, asof):
    """-> {id: row} the rows the conditions above produce from this log + results"""
    out = {}
    rows = {r['key']: r for r in ((log or {}).get('patterns') or {}).get('rows') or []}
    base = rows.get('all')
    for key, r in rows.items():
        if key == 'all' or key.startswith('old:') or key.startswith('dir:'):
            continue
        for oc in OUTCOMES:
            n, hit = _cell(r, oc); bn, bh = _cell(base, oc)
            if n < RATE_MIN_N or bn < RATE_MIN_N:
                continue
            lo = _wilson_low(hit, n); br = bh / bn
            if lo <= br:
                continue
            rid = 'RN-%s-%s' % (key.replace(':', '.'), oc)
            out[rid] = dict(id=rid, kind='RULE', by='nightly', asOf=asof, src='patterns:%s:%s' % (key, oc),
                            text='%s: %s %d of %d = %d%% (Wilson low %d%%) against %d%% of every tap — register it out of sample from the next session, and put the number on the face for this class.' % (
                                r.get('label', key), oc, hit, n, int(round(100.0 * hit / n)), int(round(100 * lo)), int(round(100 * br))),
                            changes='the face shows this class’s %s rate with its n; the register tests it on sessions it has never seen' % oc,
                            evidence='the nightly’s pattern table, %s: n=%d, low %d%% > base %d%%' % (asof, n, int(round(100 * lo)), int(round(100 * br))),
                            n=n, rate=int(round(100.0 * hit / n)), lo=int(round(100 * lo)), base=int(round(100 * br)))
    for rid, rule in ((results or {}).get('rules') or {}).items():
        if rule.get('verdict') == 'contradicts':
            mid = 'RN-teach-%s' % rid
            out[mid] = dict(id=mid, kind='TEACH', by='nightly', asOf=asof, src='results:rules:%s' % rid,
                            text='%s: the record contradicts the taught rule — re-teach it against the record, or retire it.' % rid,
                            changes='the Learn tab’s rule %s is rewritten or marked REFUTED by the review' % rid,
                            evidence=rule.get('evidence') or '', n=rule.get('n'))
    for v in (log or {}).get('hypotheses') or []:
        if v.get('verdict') == 'cleared':
            hid = 'RN-hyp-%s' % v['id']
            out[hid] = dict(id=hid, kind='RULE', by='nightly', asOf=asof, src='register:%s' % v['id'],
                            text='%s cleared at n=%s (%s%%): put its number on the face.' % (v['id'], v.get('n'), v.get('rate')),
                            changes='a tested rate renders where the hypothesis applies', evidence=v.get('bar') or '', n=v.get('n'), rate=v.get('rate'))
    return out

def ingest_decisions(days):
    """his ✓ / ✗ from the day files' `reco` ({id: {d:'approved'|'declined', t, note}}) — the latest decision per id wins"""
    dec = {}
    for d, D in days or []:
        R = (D or {}).get('reco') or {}
        if not isinstance(R, dict):
            continue
        for rid, x in R.items():
            if not isinstance(x, dict) or x.get('d') not in ('approved', 'declined'):
                continue
            t = x.get('t') or 0
            if rid not in dec or t >= (dec[rid].get('t') or 0):
                dec[rid] = dict(d=x['d'], t=t, note=x.get('note'), day=d)
    return dec

def merge(doc, seed_rows=None, machine=None, decisions=None, asof=None, refused=None):
    """apply, in this order: the review's rows (by id, text fields only), the machine's rows (new ones proposed; a
    proposed machine RULE no longer produced is withdrawn), his decisions (status on a proposed row), refused
    hypotheses (withdraw their RULE rows). Statuses implemented / withdrawn / declined are never revived here."""
    by = {r['id']: r for r in doc.get('rows', []) if r and r.get('id')}
    for s in seed_rows or []:
        r = by.get(s['id'])
        if r is None:
            by[s['id']] = dict(s, status=s.get('status') or 'proposed')
        else:
            for k in ('kind', 'text', 'changes', 'evidence', 'by', 'asOf', 'version'):
                if k in s:
                    r[k] = s[k]
            if s.get('status') in ('implemented', 'withdrawn') and r.get('status') != s['status']:
                r['status'] = s['status']; r['why'] = s.get('why')
    for mid, m in (machine or {}).items():
        r = by.get(mid)
        if r is None:
            by[mid] = dict(m, status='proposed')
        else:
            for k in ('text', 'changes', 'evidence', 'asOf', 'n', 'rate', 'lo', 'base'):
                if k in m:
                    r[k] = m[k]
            if r.get('status') == 'withdrawn' and r.get('why', '').startswith('the condition no longer held'):
                r['status'] = 'proposed'; r.pop('why', None)     # the condition holds again: back on the tab
    if machine is not None:
        for rid, r in by.items():
            if r.get('by') == 'nightly' and r.get('status') == 'proposed' and rid not in machine and r.get('kind') in ('RULE', 'TEACH'):
                r['status'] = 'withdrawn'; r['why'] = 'the condition no longer held on %s' % (asof or '?')
    for rid, dcs in (decisions or {}).items():
        r = by.get(rid)
        if r is None:
            continue
        if r.get('status') in ('proposed', 'approved', 'declined'):
            r['status'] = dcs['d']; r['decidedAt'] = dcs.get('t'); r['decidedOn'] = dcs.get('day')
            if dcs.get('note'):
                r['note'] = dcs['note']
    for hid in refused or []:
        r = by.get('RN-hyp-%s' % hid)
        if r and r.get('status') in ('proposed', 'approved'):
            r['status'] = 'withdrawn'; r['why'] = '%s was refused by the register on %s' % (hid, asof or '?')
    order = {'proposed': 0, 'approved': 1, 'implemented': 2, 'declined': 3, 'withdrawn': 4}
    doc['rows'] = sorted(by.values(), key=lambda r: (order.get(r.get('status'), 9), 0 if r.get('by') == 'review' else 1, str(r.get('id'))))
    doc['asOf'] = asof or doc.get('asOf'); doc['writtenBy'] = 'tools/nightly/recommend.py'
    doc['counts'] = {k: sum(1 for r in doc['rows'] if r.get('status') == k) for k in order}
    return doc

def atomic_write(path, text):
    tmp = path + '.tmp'
    io.open(tmp, 'w', encoding='utf-8').write(text); os.replace(tmp, path)

def update(root=ROOT, log=None, results=None, days=None):
    """the nightly's step: machine rows + his decisions + refused hypotheses into learning/recommendations.json"""
    rec = os.path.join(root, 'learning', 'recommendations.json')
    doc = load(rec)
    if log is None:
        files = sorted(glob.glob(os.path.join(root, 'learning', 'log', '20??-??-??.json')))
        log = json.load(io.open(files[-1], encoding='utf-8')) if files else {}
    if results is None:
        rp = os.path.join(root, 'learning', 'results.json')
        results = json.load(io.open(rp, encoding='utf-8')) if os.path.exists(rp) else {}
    if days is None:
        days = []
        for f in sorted(glob.glob(os.path.join(root, 'data', '20??-??-??.json'))):
            try:
                days.append((os.path.basename(f)[:10], json.load(io.open(f, encoding='utf-8'))))
            except Exception:
                pass
    asof = (log or {}).get('date') or datetime.date.today().isoformat()
    machine = machine_rows(log, results, asof)
    refused = [v['id'] for v in (log or {}).get('hypotheses') or [] if v.get('verdict') == 'refused']
    merge(doc, None, machine, ingest_decisions(days), asof, refused)
    atomic_write(rec, json.dumps(doc, ensure_ascii=False, indent=1))
    c = doc['counts']
    print('rec: %d machine rows tonight · %d proposed · %d approved · %d implemented · %d declined · %d withdrawn -> learning/recommendations.json' % (len(machine), c['proposed'], c['approved'], c['implemented'], c['declined'], c['withdrawn']))
    return doc

def selftest():
    rows = [dict(key='all', label='every tap', n=51, held=24, broke=27, pending=2, rate=47, lo=34, turn=dict(n=40, hit=3, rate=8, lo=2), resume=dict(n=40, hit=26, rate=65, lo=50)),
            dict(key='king:floor', label='King as a floor (deflected UP)', n=16, held=12, broke=4, pending=0, rate=75, lo=50, turn=dict(n=15, hit=1, rate=7, lo=0), resume=dict(n=15, hit=10, rate=67, lo=42)),
            dict(key='spx:new', label='SPX node NEW at the tap', n=17, held=10, broke=7, pending=0, rate=59, lo=36, turn=dict(n=16, hit=5, rate=31, lo=14), resume=dict(n=16, hit=11, rate=69, lo=44)),
            dict(key='spx:grow', label='g', n=20, held=12, broke=8, pending=0, rate=60, lo=39, turn=dict(n=18, hit=2, rate=11, lo=2), resume=dict(n=18, hit=12, rate=67, lo=44)),
            dict(key='old:Floor', label='old detector · Floor', n=18, held=15, broke=3, pending=0, rate=83, lo=61),
            dict(key='spx:pika', label='k', n=9, held=8, broke=1, pending=0, rate=89, lo=57)]
    log = dict(date='2026-09-15', patterns=dict(rows=rows), hypotheses=[dict(id='H2', verdict='cleared', n=30, rate=71.0, bar='holds'), dict(id='H3', verdict='refused', n=40)])
    results = dict(rules={'L5': dict(verdict='contradicts', evidence='held: a −γ node 13 / 20 = 65% vs a +γ node 18 / 30 = 60% — contradicts', n=20), 'L1': dict(verdict='agrees')})
    M = machine_rows(log, results, '2026-09-15')
    assert set(M.keys()) == {'RN-king.floor-held', 'RN-spx.new-turn', 'RN-teach-L5', 'RN-hyp-H2'}, sorted(M.keys())   # grow: low 39 < base 47 → no; old:* and dir:* never; pika: n=9 → no; resume nowhere above base
    assert M['RN-king.floor-held']['kind'] == 'RULE' and M['RN-king.floor-held']['lo'] == 51 and M['RN-king.floor-held']['base'] == 47 and 'register it out of sample' in M['RN-king.floor-held']['text']
    assert M['RN-spx.new-turn']['rate'] == 31 and M['RN-spx.new-turn']['base'] == 8 and M['RN-teach-L5']['kind'] == 'TEACH' and M['RN-hyp-H2']['kind'] == 'RULE'
    seed = [dict(id='R-1', kind='FEATURE', by='review', asOf='2026-09-04', text='record the reads', changes='c', evidence='e'), dict(id='R-2', kind='TEST', by='review', asOf='2026-09-04', text='draft', changes='c', evidence='e')]
    doc = merge(empty(), seed, M, {}, '2026-09-15', ['H3'])
    by = {r['id']: r for r in doc['rows']}
    assert doc['counts']['proposed'] == 6 and by['R-1']['status'] == 'proposed' and by['RN-hyp-H2']['status'] == 'proposed' and doc['rows'][0]['id'] == 'R-1', doc['counts']
    # his decisions ride the day files; the latest per id wins; an unknown id is ignored
    days = [('2026-09-15', dict(reco={'R-1': dict(d='approved', t=100), 'RN-teach-L5': dict(d='declined', t=100, note='not yet'), 'R-99': dict(d='approved', t=1)})),
            ('2026-09-16', dict(reco={'R-1': dict(d='declined', t=50)}))]   # older than the 09-15 decision: ignored
    dec = ingest_decisions(days)
    assert dec['R-1']['d'] == 'approved' and dec['RN-teach-L5']['note'] == 'not yet' and 'R-99' in dec
    merge(doc, None, M, dec, '2026-09-16', [])
    by = {r['id']: r for r in doc['rows']}
    assert by['R-1']['status'] == 'approved' and by['R-1']['decidedOn'] == '2026-09-15' and by['RN-teach-L5']['status'] == 'declined' and by['RN-teach-L5']['note'] == 'not yet' and 'R-99' not in by
    # a machine RULE whose condition no longer holds is withdrawn; when it holds again it comes back; a refused hypothesis withdraws its row
    M2 = {k: v for k, v in M.items() if k != 'RN-king.floor-held'}
    merge(doc, None, M2, {}, '2026-09-17', ['H2'])
    by = {r['id']: r for r in doc['rows']}
    assert by['RN-king.floor-held']['status'] == 'withdrawn' and by['RN-king.floor-held']['why'].startswith('the condition no longer held on 2026-09-17')
    assert by['RN-hyp-H2']['status'] == 'withdrawn' and 'refused' in by['RN-hyp-H2']['why']
    merge(doc, None, M, {}, '2026-09-18', [])
    by = {r['id']: r for r in doc['rows']}
    assert by['RN-king.floor-held']['status'] == 'proposed' and by['RN-hyp-H2']['status'] == 'withdrawn'    # a refusal is not undone by a later log
    # the review's seed regenerates: the text follows the seed, his status does not
    merge(doc, [dict(id='R-1', kind='FEATURE', by='review', asOf='2026-09-04', text='record the reads (reworded)', changes='c', evidence='e'), dict(id='R-2', kind='TEST', by='review', asOf='2026-09-04', text='draft', changes='c', evidence='e', status='implemented', version='15.71', why='shipped')], None, {}, '2026-09-19', [])
    by = {r['id']: r for r in doc['rows']}
    assert by['R-1']['status'] == 'approved' and by['R-1']['text'] == 'record the reads (reworded)' and by['R-2']['status'] == 'implemented' and by['R-2']['version'] == '15.71'
    # a seed row that says 'proposed' never reverts his ✓; a decision never revives an implemented row
    merge(doc, [dict(id='R-1', kind='FEATURE', by='review', asOf='2026-09-04', text='t', changes='c', evidence='e', status='proposed')], None, {'R-2': dict(d='declined', t=999, day='2026-09-20')}, '2026-09-20', [])
    by = {r['id']: r for r in doc['rows']}
    assert by['R-1']['status'] == 'approved' and by['R-2']['status'] == 'implemented', (by['R-1']['status'], by['R-2']['status'])
    assert [r['id'] for r in doc['rows']][:2] == ['R-1', 'RN-king.floor-held'] or doc['rows'][0]['status'] == 'proposed'
    print('recommend.py selftest ok · %d machine rows on the fixture' % len(M))

if __name__ == '__main__':
    if '--selftest' in sys.argv:
        selftest()
    elif any(a.startswith('-') for a in sys.argv[1:]):
        print(__doc__); sys.exit(2)
    else:
        update()
