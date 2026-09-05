#!/usr/bin/env python3
"""results.py — the nightly writes the Analysis tab (v15.68).

Operator, 2026-09-04: "i envision clicking on the save, the data getting saved and the analysis occurring and the
analysis tab being updated." Until v15.67 the nightly wrote only the log; the study registry (learning/studies.json,
what the Analysis tab renders) changed only when a review edited tools/studies-seed.py in a session. Now every study
whose result is a number the nightly can compute gets that number written by the nightly itself:

    STUDY_SOURCES  study id -> where its number comes from
        ('pattern', [class keys])   the held-rate table (tools/nightly/patterns.py): held / broke / n / Wilson low per class
        ('hyp', 'Hn')               the register's verdict for hypothesis Hn (run.py's judge), via HYP_STUDY

    python3 tools/nightly/results.py                 # from the newest learning/log/<day>.json: write results.json, patch studies.json
    python3 tools/nightly/results.py --json log.json # a fixture log → the map and the computed rows as JSON (the panel test pins them)
    python3 tools/nightly/results.py --selftest

Files:  learning/results.json  — the machine's numbers by study id: {id: {line, status|null, n, rate, lo, src, asOf, by:'nightly'}}
        learning/studies.json  — patched in place on the mapped rows: `nightly` (the machine's line, always) + `asOf`; and
                                 `result` + `status` + `by` ONLY when the machine has a verdict — a rate, or a register
                                 verdict. A thin row keeps the review's sentence and status; the nightly line shows the
                                 progress under it. Everything else untouched.
tools/studies-seed.py merges results.json when it regenerates studies.json, so a review never erases the nightly's numbers.
Status rules (mechanical, and only these): a pattern row → READ at n ≥ RATE_MIN_N, THIN under it, untouched at n = 0;
a hypothesis row → cleared → READ · refused → REFUSED · ready → READ NEXT · thin / blocked → the row keeps its status and
its sentence and gets the "n so far" line. A row the nightly cannot compute is never touched.
⚠ HYP_STUDY is duplicated in the panel (var HYP_STUDY); test_v1568 pins them equal.
"""
import glob, io, json, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RATE_MIN_N = 15
HYP_STUDY = {'H1': 'F5.2', 'H2': 'F2.1', 'H3': 'F6.1', 'H4': 'F1.4', 'H5': 'H1.3', 'H6': 'H2.7', 'H7': 'H2.8', 'H8': 'K2.6', 'H9': 'K2.7'}
STUDY_SOURCES = {
    # the King (subject S0 / K1 / K2)
    'S0.1': ('pattern', ['king:any']),
    'S0.2': ('pattern', ['king:SPX']),
    'S0.3': ('pattern', ['king:SPY']),
    'S0.4': ('pattern', ['king:QQQ']),
    'S0.5': ('pattern', ['king:grow', 'king:fade']),
    'S0.7': ('pattern', ['king:pos', 'king:neg']),
    'K1.1': ('pattern', ['king:SPX']),
    'K1.2': ('pattern', ['king:SPY', 'king:QQQ']),
    'K2.1': ('pattern', ['king:floor']),
    'K2.2': ('pattern', ['king:ceil']),
    'K2.6': ('pattern', ['king:floor:up', 'king:floor:dn']),   # (v15.72) the rolling floor — his read of 2026-09-04 (H8)
    'K2.7': ('pattern', ['king:ceil:dn', 'king:ceil:up']),     # (v15.72) the rolling ceiling (H9)
    # the rugs (S1) and the stacks (S7)
    'S1.1': ('pattern', ['spx:rug', 'spy:rug', 'qqq:rug']),
    'S1.2': ('pattern', ['spx:rrug', 'spy:rrug', 'qqq:rrug']),
    'S7.1': ('pattern', ['spx:pika', 'spy:pika', 'qqq:pika']),
    'S7.2': ('pattern', ['spx:barney', 'spy:barney', 'qqq:barney']),
}
for _h, _s in HYP_STUDY.items():
    STUDY_SOURCES[_s] = ('hyp', _h)

def _rows(log):
    P = (log or {}).get('patterns') or {}
    return {r['key']: r for r in (P.get('rows') or [])}

def _short(key):
    # the class key as the result line names it: 'king:SPX' -> 'SPX King', 'spx:rug' -> 'SPX'
    if key == 'king:any': return 'any King'
    if key == 'king:floor': return 'King as floor'
    if key == 'king:ceil': return 'King as ceiling'
    if key == 'king:grow': return 'growing'
    if key == 'king:fade': return 'fading'
    if key == 'king:pos': return '+γ King'
    if key == 'king:neg': return '−γ King'
    if key.startswith('king:'): return key[5:] + ' King'
    return key.split(':')[0].upper()

def _oc(x):
    """an objective outcome cell {n, hit, rate, lo} as text"""
    if not x or not x.get('n'):
        return '—'
    return ('%d / %d = %d%% (low %d%%)' % (x['hit'], x['n'], x['rate'], x['lo'])) if x['n'] >= RATE_MIN_N else ('%d / %d (thin)' % (x['hit'], x['n']))

def pattern_result(rows, keys, asof):
    """-> (result line, status, n, rate, lo) from the held-rate table, or None when no class has a tap yet.
    (v15.69) the line carries the objective outcomes too when the row has them: held · turn · resume"""
    parts = []; nmax = 0; best = None
    for k in keys:
        r = rows.get(k)
        if not r or not (r['n'] + r['pending']):
            parts.append('%s —' % _short(k)); continue
        if r['n'] >= RATE_MIN_N:
            txt = '%s held %d / %d = %d%% (low %d%%)' % (_short(k), r['held'], r['n'], r['rate'], r['lo'])
        else:
            txt = '%s held %d / %d (thin)' % (_short(k), r['held'], r['n'])
        if 'turn' in r:
            txt += ' · turn %s · resume %s' % (_oc(r.get('turn')), _oc(r.get('resume')))
        parts.append(txt)
        if r['n'] > nmax:
            nmax = r['n']; best = r
    if nmax == 0:
        return None
    status = 'READ' if nmax >= RATE_MIN_N else 'THIN'
    line = ' · '.join(parts) + ' · nightly ' + asof
    return line, status, nmax, (best['rate'] if best else None), (best['lo'] if best else None)

# ---- (v15.69) THE LEARN TAB'S RULES CARRY THE RECORD'S NUMBERS ---------------------------------------------------------
# Operator, 2026-09-04: "the learn tab should also be updated" · "it is from the learning that you can know something".
# Each rule L-n that names a condition the pattern table has as a class gets the record's numbers for it, and a verdict
# the record gives — agrees / contradicts (both sides at n ≥ RATE_MIN_N, in / against the direction the rule claims),
# thin (not there yet), measured (a base row, no direction), not measured (the ledger has no class for it: L3 / L8 need
# the King's path, L4 the side flip, L7 the clock). The rule's own status (CONFIRMED from his taught legs) is the
# prior and stays; the verdict is the record's, beside it. Written into learning/deflections/examples.json (the Learn
# tab) and learning/results.json; tools/learn-seed.py merges them when it regenerates.
RULE_SOURCES = {
    'L1': dict(kind='compare', a='spx:grow', b='spx:fade', outcome='held', expect='a>b', why='growing into the tap holds more than fading'),
    'L2': dict(kind='vs_all', a=['spx:new'], outcome='turn', expect='a>all', why='a fresh node at the extreme IS the turn more often than any tap'),
    'L5': dict(kind='compare', a='spx:neg', b='spx:pos', outcome='held', expect='a<b', why='a −γ node holds less than a +γ node'),
    'L6': dict(kind='vs_all', a=['spx:pika', 'spx:barney'], outcome='turn', expect='a>all', why='a stack marks the extreme more often than any tap'),
    'L9': dict(kind='rows', rows=['king:any', 'king:SPX', 'king:SPY', 'king:QQQ'], outcome='held', why='the base case: every King tap, by book'),
    'L3': dict(kind='none', why='needs the King path (price pulled back to a growing King) — not a tap class'),
    'L8': dict(kind='none', why='needs the King path with the sign — not a tap class'),
    'L4': dict(kind='none', why='needs the side flip (a node re-growing under price) — not a tap class'),
    'L7': dict(kind='none', why='needs the clock as a class — to add'),
}

def _cell(r, outcome):
    """(n, hit) of a row for an outcome: held → (n, held); turn / resume → the objective cell"""
    if not r:
        return 0, 0
    if outcome == 'held':
        return r.get('n', 0), r.get('held', 0)
    x = r.get(outcome) or {}
    return x.get('n', 0), x.get('hit', 0)

def _sum_cells(rows, keys, outcome):
    n = 0; hit = 0
    for k in (keys if isinstance(keys, list) else [keys]):
        a, b = _cell(rows.get(k), outcome); n += a; hit += b
    return n, hit

def _rate_txt(n, hit):
    if not n:
        return '—'
    r = _js_round(100.0 * hit / n); lo = _js_round(100 * wilson_low(hit, n))
    return ('%d / %d = %d%% (low %d%%)' % (hit, n, r, lo)) if n >= RATE_MIN_N else ('%d / %d (thin)' % (hit, n))

def _js_round(x):
    import math
    return int(math.floor(x + 0.5))

def wilson_low(right, n, z=1.96):
    import math
    if not n or n <= 0:
        return 0.0
    p = right / n; d = 1 + z * z / n; c = p + z * z / (2 * n); m = z * math.sqrt((p * (1 - p) + z * z / (4 * n)) / n)
    return max(0.0, (c - m) / d)

_RULE_LABEL = {'spx:grow': 'growing', 'spx:fade': 'fading', 'spx:new': 'a NEW node', 'spx:pos': 'a +γ node', 'spx:neg': 'a −γ node',
               'spx:pika': 'pika stack', 'spx:barney': 'barney stack', 'all': 'every tap'}
def _rl(key):
    return _RULE_LABEL.get(key) or _short(key)

def rule_evidence(rows, asof):
    """-> {rule id: {evidence, verdict, n, asOf, by}} for every rule in RULE_SOURCES"""
    out = {}
    for rid, spec in RULE_SOURCES.items():
        kind = spec['kind']; oc = spec.get('outcome', 'held')
        if kind == 'none':
            out[rid] = dict(evidence='not measured by the ledger: ' + spec['why'], verdict='not measured', n=0, asOf=asof, by='nightly'); continue
        if kind == 'rows':
            parts = []; nmax = 0
            for k in spec['rows']:
                n, hit = _cell(rows.get(k), oc); parts.append('%s %s' % (_rl(k), _rate_txt(n, hit))); nmax = max(nmax, n)
            out[rid] = dict(evidence=oc + ': ' + ' · '.join(parts) + ' · nightly ' + asof, verdict=('measured' if nmax >= RATE_MIN_N else 'thin'), n=nmax, asOf=asof, by='nightly'); continue
        if kind == 'compare':
            na, ha = _cell(rows.get(spec['a']), oc); nb, hb = _cell(rows.get(spec['b']), oc)
            la, lb = _rl(spec['a']), _rl(spec['b'])
        else:   # vs_all
            na, ha = _sum_cells(rows, spec['a'], oc); nb, hb = _cell(rows.get('all'), oc)
            la, lb = ' + '.join(_rl(k) for k in spec['a']), 'every tap'
        ev = '%s: %s %s vs %s %s' % (oc, la, _rate_txt(na, ha), lb, _rate_txt(nb, hb))
        if na >= RATE_MIN_N and nb >= RATE_MIN_N:
            ra, rb = ha / na, hb / nb
            holds = (ra > rb) if spec['expect'] in ('a>b', 'a>all') else (ra < rb)
            verdict = 'agrees' if holds else 'contradicts'
        else:
            verdict = 'thin'
        out[rid] = dict(evidence=ev + ' — ' + verdict + ' (' + spec['why'] + ') · nightly ' + asof, verdict=verdict, n=min(na, nb), asOf=asof, by='nightly')
    return out

def apply_rules(learn, rules):
    """patch the Learn doc's rules in place with evidence · verdict · asOf; -> count"""
    R = rules.get('rules') if isinstance(rules, dict) and 'rules' in rules and isinstance(rules.get('rules'), dict) and not rules.get('schema') is None and 'results' in rules else rules
    n = 0
    for r in (learn or {}).get('rules') or []:
        e = (R or {}).get(r.get('id'))
        if not e:
            continue
        r['evidence'] = e['evidence']; r['verdict'] = e['verdict']; r['asOf'] = e.get('asOf'); n += 1
    return n

def hyp_result(verdicts, hid, asof):
    v = None
    for x in verdicts or []:
        if x.get('id') == hid:
            v = x; break
    if not v:
        return None
    vd = v.get('verdict'); n = v.get('n'); minN = v.get('minN')
    if vd == 'cleared':
        return ('%s CLEARED at n=%s: %s%s · nightly %s' % (hid, n, ('%s%%' % v['rate']) if v.get('rate') is not None else '', (' — ' + v['bar']) if v.get('bar') else '', asof), 'READ', n, v.get('rate'), None)
    if vd == 'refused':
        return ('%s REFUSED at n=%s: %s%s · nightly %s' % (hid, n, ('%s%%' % v['rate']) if v.get('rate') is not None else '', (' — ' + v['bar']) if v.get('bar') else '', asof), 'REFUSED', n, v.get('rate'), None)
    if vd == 'ready':
        return ('%s ready: %s · nightly %s' % (hid, v.get('bar') or ('n=%s of %s' % (n, minN)), asof), 'READ NEXT', n, None, None)
    # thin / blocked: the row keeps its status, the line says how far along it is
    return ('%s %s: n=%s of %s · nightly %s' % (hid, vd or 'thin', n if n is not None else 0, minN, asof), None, n, None, None)

def compute_rules(log):
    return rule_evidence(_rows(log), (log or {}).get('date') or '?')

def compute(log):
    """-> {study id: {line, status|None, n, rate, lo, src, asOf, by}} for every mapped study the log can answer"""
    asof = (log or {}).get('date') or '?'
    rows = _rows(log); verdicts = (log or {}).get('hypotheses') or []
    out = {}
    for sid, (kind, arg) in STUDY_SOURCES.items():
        r = pattern_result(rows, arg, asof) if kind == 'pattern' else hyp_result(verdicts, arg, asof)
        if not r:
            continue
        line, status, n, rate, lo = r
        out[sid] = dict(line=line, status=status, n=n, rate=rate, lo=lo, src=('patterns:' + ','.join(arg)) if kind == 'pattern' else ('register:' + arg), asOf=asof, by='nightly')
    return out

def apply(studies, results):
    """patch the registry in place: result · status (when the nightly has one) · by · asOf on the mapped rows; -> count"""
    n = 0
    R = (results or {}).get('results') if isinstance(results, dict) and 'results' in (results or {}) else results
    for sj in (studies or {}).get('subjects') or []:
        for ss in sj.get('subsections') or []:
            for x in ss.get('studies') or []:
                r = (R or {}).get(x.get('id'))
                if not r:
                    continue
                x['nightly'] = r['line']; x['asOf'] = r.get('asOf'); n += 1
                if r.get('status'):                       # a verdict: the number becomes the row's result
                    x['result'] = r['line']; x['status'] = r['status']; x['by'] = 'nightly'
                # no verdict (thin / blocked): the review's sentence and status stand; the line rides beside them
    # the counts follow the statuses
    tot = 0; by = {}
    for sj in (studies or {}).get('subjects') or []:
        for ss in sj.get('subsections') or []:
            for x in ss.get('studies') or []:
                tot += 1; by[x['status']] = by.get(x['status'], 0) + 1
    if studies is not None:
        studies['counts'] = dict(studies=tot, byStatus=by)
    return n

def atomic_write(path, text):
    """write beside, then replace — the sync task's `git add -A` two minutes from now must never see half a file"""
    tmp = path + '.tmp'
    io.open(tmp, 'w', encoding='utf-8').write(text)
    os.replace(tmp, path)

def newest_log(root=ROOT):
    files = sorted(glob.glob(os.path.join(root, 'learning', 'log', '20??-??-??.json')))
    return files[-1] if files else None

def write(root=ROOT, log=None):
    """results.json + studies.json patched from the (newest) log; -> (results, patched count)"""
    if log is None:
        p = newest_log(root)
        if not p:
            print('results: no learning/log/<day>.json'); return None, 0
        log = json.load(io.open(p, encoding='utf-8'))
    R = compute(log); RL = compute_rules(log)
    doc = dict(schema=1, asOf=log.get('date'), writtenBy='tools/nightly/results.py', results=R, rules=RL)
    atomic_write(os.path.join(root, 'learning', 'results.json'), json.dumps(doc, ensure_ascii=False, indent=1))
    sp = os.path.join(root, 'learning', 'studies.json')
    n = 0
    if os.path.exists(sp):
        S = json.load(io.open(sp, encoding='utf-8'))
        n = apply(S, R)
        atomic_write(sp, json.dumps(S, ensure_ascii=False, indent=1))
    # (v15.69) the Learn tab's rules
    lp = os.path.join(root, 'learning', 'deflections', 'examples.json')
    nl = 0
    if os.path.exists(lp):
        L = json.load(io.open(lp, encoding='utf-8'))
        nl = apply_rules(L, RL)
        atomic_write(lp, json.dumps(L, ensure_ascii=False, indent=1))
    print('results: %d studies answered by the nightly (%s) · %d rows patched in learning/studies.json · %d Learn rules carry the record' % (len(R), log.get('date'), n, nl))
    return doc, n

def selftest():
    rows = [dict(key='all', label='every tap', n=51, held=24, broke=27, pending=2, rate=47, lo=34),
            dict(key='king:any', label='King · any book', n=20, held=9, broke=11, pending=0, rate=45, lo=26),
            dict(key='king:SPX', label='King · SPX', n=20, held=9, broke=11, pending=0, rate=45, lo=26),
            dict(key='king:SPY', label='King · SPY', n=3, held=2, broke=1, pending=1, rate=67, lo=21),
            dict(key='spx:rug', label='SPX rug', n=4, held=2, broke=2, pending=1, rate=50, lo=15)]
    log = dict(date='2026-09-08', patterns=dict(rows=rows), hypotheses=[
        dict(id='H1', verdict='thin', n=3, minN=40), dict(id='H2', verdict='cleared', n=30, minN=30, rate=71.0, bar='holds, CI excludes base'),
        dict(id='H3', verdict='refused', n=40, minN=40, rate=52.0, bar='gap 0.1 pts >= 8'), dict(id='H5', verdict='ready', n=51, minN=50, bar='ledger has 51 events — the join can be run')])
    R = compute(log)
    assert R['S0.1']['status'] == 'READ' and R['S0.1']['n'] == 20 and R['S0.1']['line'] == 'any King held 9 / 20 = 45% (low 26%) · nightly 2026-09-08', R['S0.1']
    assert R['S0.2']['status'] == 'READ' and R['K1.1']['line'] == R['S0.2']['line']
    assert R['S0.3']['status'] == 'THIN' and R['S0.3']['line'] == 'SPY King held 2 / 3 (thin) · nightly 2026-09-08'
    assert 'S0.4' not in R and 'S7.1' not in R and 'K2.1' not in R          # no tap in the class yet: the row is not touched
    assert R['K1.2']['status'] == 'THIN' and R['K1.2']['line'].startswith('SPY King held 2 / 3 (thin) · QQQ King —')
    assert R['S1.1']['status'] == 'THIN' and R['S1.1']['line'] == 'SPX held 2 / 4 (thin) · SPY — · QQQ — · nightly 2026-09-08'
    assert R['F5.2']['status'] is None and R['F5.2']['line'] == 'H1 thin: n=3 of 40 · nightly 2026-09-08'
    assert R['F2.1']['status'] == 'READ' and R['F2.1']['line'].startswith('H2 CLEARED at n=30: 71.0% — holds')
    assert R['F6.1']['status'] == 'REFUSED' and R['H1.3']['status'] == 'READ NEXT' and 'F1.4' not in R
    S = dict(schema=1, subjects=[dict(key='S', subsections=[dict(key='S0', studies=[
        dict(id='S0.1', q='q', status='THIN', result='old text'), dict(id='S0.9', q='q', status='OPEN'), dict(id='F5.2', q='q', status='REGISTERED', result='the review said this')])])])
    n = apply(S, dict(schema=1, results=R))
    st = {x['id']: x for x in S['subjects'][0]['subsections'][0]['studies']}
    assert n == 2 and st['S0.1']['status'] == 'READ' and st['S0.1']['by'] == 'nightly' and st['S0.1']['asOf'] == '2026-09-08' and st['S0.1']['result'] == R['S0.1']['line'] and st['S0.1']['nightly'] == R['S0.1']['line']
    assert st['S0.9']['status'] == 'OPEN' and 'by' not in st['S0.9'] and st['S0.9'].get('result') is None and 'nightly' not in st['S0.9']
    # a thin hypothesis: the review's sentence and status stand; the machine's progress rides beside them
    assert st['F5.2']['status'] == 'REGISTERED' and st['F5.2']['result'] == 'the review said this' and 'by' not in st['F5.2'] and st['F5.2']['nightly'] == 'H1 thin: n=3 of 40 · nightly 2026-09-08'
    assert S['counts'] == dict(studies=3, byStatus={'READ': 1, 'OPEN': 1, 'REGISTERED': 1}), S['counts']
    assert apply(S, {}) == 0
    # (v15.69) the objective outcomes ride the line; the Learn rules get the record's verdict
    rows2 = [dict(key='all', label='every tap', n=51, held=24, broke=27, pending=2, rate=47, lo=34, turn=dict(n=40, hit=3, rate=8, lo=2), resume=dict(n=40, hit=26, rate=65, lo=50)),
             dict(key='spx:grow', label='g', n=20, held=15, broke=5, pending=0, rate=75, lo=53, turn=dict(n=18, hit=2, rate=11, lo=2), resume=dict(n=18, hit=12, rate=67, lo=44)),
             dict(key='spx:fade', label='f', n=16, held=6, broke=10, pending=0, rate=38, lo=18, turn=dict(n=15, hit=1, rate=7, lo=0), resume=dict(n=15, hit=9, rate=60, lo=36)),
             dict(key='spx:new', label='n', n=17, held=10, broke=7, pending=0, rate=59, lo=36, turn=dict(n=16, hit=5, rate=31, lo=14), resume=dict(n=16, hit=11, rate=69, lo=44)),
             dict(key='spx:pos', label='p', n=30, held=18, broke=12, pending=0, rate=60, lo=42, turn=dict(n=25, hit=2, rate=8, lo=1), resume=dict(n=25, hit=17, rate=68, lo=48)),
             dict(key='spx:neg', label='q', n=20, held=13, broke=7, pending=0, rate=65, lo=43, turn=dict(n=15, hit=1, rate=7, lo=0), resume=dict(n=15, hit=9, rate=60, lo=36)),
             dict(key='spx:pika', label='k', n=9, held=5, broke=4, pending=0, rate=56, lo=27, turn=dict(n=8, hit=2, rate=25, lo=7), resume=dict(n=8, hit=5, rate=63, lo=31)),
             dict(key='king:any', label='K', n=20, held=9, broke=11, pending=0, rate=45, lo=26, turn=dict(n=18, hit=2, rate=11, lo=2), resume=dict(n=18, hit=13, rate=72, lo=49))]
    log2 = dict(date='2026-09-09', patterns=dict(rows=rows2), hypotheses=[])
    R2 = compute(log2)
    assert R2['S0.1']['line'] == 'any King held 9 / 20 = 45% (low 26%) · turn 2 / 18 = 11% (low 2%) · resume 13 / 18 = 72% (low 49%) · nightly 2026-09-09', R2['S0.1']['line']
    RL = compute_rules(log2)
    assert RL['L1']['verdict'] == 'agrees' and RL['L1']['evidence'].startswith('held: growing 15 / 20 = 75% (low 53%) vs fading 6 / 16 = 38% (low 18%) — agrees'), RL['L1']
    assert RL['L5']['verdict'] == 'contradicts' and RL['L5']['evidence'].startswith('held: a −γ node 13 / 20 = 65% (low 43%) vs a +γ node 18 / 30 = 60% (low 42%) — contradicts'), RL['L5']   # a −γ node held MORE here
    assert RL['L2']['verdict'] == 'agrees' and RL['L2']['evidence'].startswith('turn: a NEW node 5 / 16 = 31% (low 14%) vs every tap 3 / 40 = 8% (low 3%) — agrees')
    assert RL['L6']['verdict'] == 'thin' and RL['L6']['evidence'].startswith('turn: pika stack + barney stack 2 / 8 (thin) vs every tap 3 / 40')
    assert RL['L9']['verdict'] == 'measured' and RL['L9']['evidence'].startswith('held: any King 9 / 20 = 45% (low 26%) · SPX King —')
    assert RL['L3']['verdict'] == 'not measured' and RL['L7']['verdict'] == 'not measured'
    Ld = dict(rules=[dict(id='L1', rule='r', status='PROPOSED'), dict(id='L9', rule='r', status='CONFIRMED'), dict(id='L99', rule='r', status='PROPOSED')])
    assert apply_rules(Ld, RL) == 2 and Ld['rules'][0]['verdict'] == 'agrees' and Ld['rules'][0]['status'] == 'PROPOSED' and Ld['rules'][0]['asOf'] == '2026-09-09' and 'verdict' not in Ld['rules'][2]   # the record never changes a rule's status — the review does
    print('results.py selftest ok · %d studies answered · %d rules judged' % (len(R), len([r for r in RL.values() if r['verdict'] in ('agrees', 'contradicts')])))

if __name__ == '__main__':
    if '--selftest' in sys.argv:
        selftest()
    elif '--json' in sys.argv:      # a fixture log → compute() as JSON (test_v1568 pins the panel's HYP_STUDY and the rows)
        _log = json.load(io.open(sys.argv[sys.argv.index('--json') + 1], encoding='utf-8'))
        print(json.dumps(dict(hypStudy=HYP_STUDY, sources={k: list(v) for k, v in STUDY_SOURCES.items()}, results=compute(_log), rules=compute_rules(_log)), ensure_ascii=False))
    elif any(a.startswith('-') for a in sys.argv[1:]):
        print(__doc__); sys.exit(2)
    else:
        write()
