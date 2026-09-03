#!/usr/bin/env python3
"""
THE NIGHTLY, IN ONE COMMAND — workflow step 12 (design/ARCHITECTURE-E2E-WORKFLOW.md).

    python3 tools/nightly/run.py                 # score everything through the latest exported day
    python3 tools/nightly/run.py 2026-09-05      # ...through a given day
    python3 tools/nightly/run.py --selftest      # plant a known effect in synthetic days; it MUST be found,
                                                 # and a planted nothing MUST come back thin/null

WHAT IT DOES, AND WHAT IT REFUSES
  reads   learning/register.json         THE ONE REGISTER — the same list the panel's Testing ② renders
          data/<day>.json  (day >= from)  the panel's own exports: feat records, the defl ledger, snaps
  scores  every hypothesis BY EPISODE (one row per day x node, first bar — never per bar: 1,151 rows
          were 94 episodes on 2026-09-03), with a Wilson interval, against the base rate over the same
          episodes, and against a SHUFFLE NULL: the best-of-K gap the K registered claims reach when the
          labels are permuted. A claim must beat that, not the null for one test.
  reads   a hypothesis ONCE it reaches minN. Below that it is THIN and its rate is not printed.
  writes  learning/log/<day>.json  schema 2  — verdicts, the HOD/LOD live-vs-table calibration from the
          close-scored lodhod records, and one pre-open line — which pipeNightlyTry() reads back into
          Analysis REVIEW.
  ⚠ THE LLM PROPOSES, THE HARNESS DISPOSES. This script has no opinion; it counts. New hypotheses go
    into the register with a `written` date and are tested on sessions AFTER that date only.
"""
import io, json, os, sys, glob, math, random, collections, datetime

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REG  = os.path.join(ROOT, 'learning', 'register.json')
LOGD = os.path.join(ROOT, 'learning', 'log')
DATA = os.path.join(ROOT, 'data')

def wilson(k, n, z=1.96):
    if n == 0: return (0.0, 0.0)
    p = k / n; den = 1 + z*z/n
    c = (p + z*z/(2*n)) / den
    h = z * math.sqrt(p*(1-p)/n + z*z/(4*n*n)) / den
    return (max(0.0, c-h), min(1.0, c+h))

# ---- episodes ----------------------------------------------------------------------------------
def load_days(upto):
    days = []
    for f in sorted(glob.glob(os.path.join(DATA, '2026-*.json'))):
        d = os.path.basename(f)[:-5]
        if upto and d > upto: continue
        try: days.append((d, json.load(open(f))))
        except Exception as e: print('  skip %s: %s' % (d, e))
    return days

def episodes(days, frm, sym='SPY'):
    """one row per (day, key, strike): the FIRST resolved+scored record — the ex-ante state."""
    first = {}
    for d, D in days:
        if d < frm: continue
        F = (D.get('feat') or {}).get(sym) or []
        for r in F:
            if not r or not r.get('resolved') or r.get('hit') is None or not isinstance(r.get('rec'), dict): continue
            if r.get('key') not in ('node', 'reaction'): continue
            k = (d, r['key'], r['rec'].get('k'))
            if k not in first or (r.get('t') or 0) < (first[k].get('t') or 0): first[k] = dict(r, _day=d)
    return list(first.values())

def defl_events(days, frm, sym='SPY'):
    out = []
    for d, D in days:
        if d < frm: continue
        out += [dict(e, _day=d) for e in ((D.get('defl') or {}).get(sym) or []) if e and e.get('cont') is not None]
    return out

# ---- the picks: what each registered hypothesis counts -----------------------------------------
def pick_rows(pick, eps):
    node = [e for e in eps if e['key'] == 'node']
    if pick == 'gradeA': return [e for e in node if e['rec'].get('grade') == 'A'], node
    if pick == 'tap1':   return [e for e in node if isinstance(e['rec'].get('tap'), (int, float)) and e['rec']['tap'] >= 1], node
    if pick == 'pol':    return ([e for e in node if e['rec'].get('pol') == '+'], [e for e in node if e['rec'].get('pol') == '-'])
    if pick == 'wick':
        r = [e for e in eps if e['key'] == 'reaction']
        return ([e for e in r if e['rec'].get('quality') == 'confirmed'], [e for e in r if e['rec'].get('quality') == 'weak'])
    return [], []

def rate(rows):
    n = len(rows); h = sum(1 for e in rows if e['hit']); return (n, h, (h/n if n else None))

def shuffle_null(H_list, eps, iters=400, seed=11):
    """best-of-K absolute gap (subset rate - base rate, in points) under permuted labels."""
    rnd = random.Random(seed); best = []
    labels = [e['hit'] for e in eps]
    for _ in range(iters):
        z = labels[:]; rnd.shuffle(z)
        perm = [dict(e, hit=z[i]) for i, e in enumerate(eps)]
        b = 0.0
        for H in H_list:
            if H.get('gap') or H.get('blocked'): continue
            sub, base = pick_rows(H['pick'], perm)
            n, h, r = rate(sub); nb, hb, rb = rate(base)
            if n >= H['minN'] and rb is not None: b = max(b, abs(100*r - 100*rb))
        best.append(b)
    best.sort()
    return dict(p50=best[len(best)//2], p95=best[int(.95*len(best))])

def judge_sweep(H, since):
    """(v15.55) H7 — the early sweep, re-read on sessions AFTER the corpus it was found in. Uses the same
    definitions as tools/study-sweeps.py (imported, not re-typed). H6 needs the TAP record: blocked."""
    out = dict(id=H['id'], claim=H['claim'], minN=H['minN'], pick=H.get('pick'), judgedBy='nightly')
    if H.get('pick') == 'sweepNode':
        # (v15.56) the day files carry the book per 3-minute bar — tools/study-sweeps-book.py tags every price-level
        # sweep-reclaim AT a top-5 node / the King or NOT. n counts the AT-node events on sessions from `since`.
        try:
            sys.path.insert(0, os.path.join(ROOT, 'tools'))
            sb = __import__('study-sweeps-book')
            res = sb.run()
            at = [e for e in res['events'] if e['kind'] == 'price' and e['printed'] is not None and e['atNode'] and e['d'] >= since]
            no = [e for e in res['events'] if e['kind'] == 'price' and e['printed'] is not None and e['atNode'] is False and e['d'] >= since]
            n = len(at); k = sum(e['printed'] for e in at); nn = len(no); kn = sum(e['printed'] for e in no)
            out.update(n=n, nControl=nn, sessions=res['corpus'].get('sessions', 0))
            if n < H['minN']:
                out.update(verdict='thin', bar='%d sweep-at-a-node events on sessions from %s (needs %d); not at a node: %d' % (n, since, H['minN'], nn)); return out
            r = k / n; lo, hi = wilson(k, n); ctrl = (kn / nn) if nn else None
            out.update(rate=round(100*r, 1), base=(round(100*ctrl, 1) if ctrl is not None else None), ci=[round(100*lo), round(100*hi)])
            thr = _points(H.get('predict', ''), 40); ref = _points(H.get('refuteIf', ''), 30)
            if 100*r > thr and ctrl is not None and lo > ctrl: out.update(verdict='cleared', bar='holds (> %.0f%%) and the CI excludes the not-at-a-node rate %.0f%%' % (thr, 100*ctrl))
            elif 100*r <= ref: out.update(verdict='refused', bar='%.0f%% <= %.0f%%: refuted' % (100*r, ref))
            else: out.update(verdict='refused', bar='between the prediction and the refutation, or the CI covers the control')
            return out
        except Exception as e:
            out.update(n=0, verdict='thin', bar='book study threw: %s' % e); return out
    if H.get('blocked'):
        out.update(n=0, verdict='blocked', bar='blocked: %s' % H.get('note', '')[:80])
        return out
    try:
        sys.path.insert(0, os.path.join(ROOT, 'tools'))
        sw = __import__('study-sweeps')
        es = os.path.join(ROOT, 'data', 'es-1min', 'ES TestingData.txt')
        if not os.path.exists(es):
            out.update(n=0, verdict='thin', bar='no ES file to read'); return out
        res = sw.run(es)
        ev = [e for e in res['events'] if e['d'] > since and e['level'] in ('ONL', 'ONH', 'PDL', 'PDH') and e['bucket'] == '08:30-09:00' and e['printed'] is not None]
        n = len(ev); k = sum(e['printed'] for e in ev)
        out.update(n=n, sessionsAfter=len(set(e['d'] for e in ev)))
        if n < H['minN']:
            out.update(verdict='thin', bar='%d early ON/PD sweep-reclaims on sessions after %s (needs %d)' % (n, since, H['minN'])); return out
        r = k / n; lo, hi = wilson(k, n); ctrl = sum(e['fresh'] for e in ev if e['fresh'] is not None) / max(1, len([e for e in ev if e['fresh'] is not None]))
        out.update(rate=round(100*r, 1), base=round(100*ctrl, 1), ci=[round(100*lo), round(100*hi)])
        thr = _points(H.get('predict', ''), 24); ref = _points(H.get('refuteIf', ''), 18)
        if 100*r > thr and lo > ctrl: out.update(verdict='cleared', bar='holds (> %.0f%%) and the CI excludes the fresh-low control %.0f%%' % (thr, 100*ctrl))
        elif 100*r <= ref: out.update(verdict='refused', bar='%.0f%% <= %.0f%%: refuted' % (100*r, ref))
        else: out.update(verdict='refused', bar='between the prediction and the refutation, or the CI covers the control %.0f%%' % (100*ctrl))
        return out
    except Exception as e:
        out.update(n=0, verdict='thin', bar='sweep judge threw: %s' % e); return out


def judge(H, eps, defl, null95, since='2026-09-03'):
    out = dict(id=H['id'], claim=H['claim'], minN=H['minN'], pick=H.get('pick'))
    if H.get('judgedBy') == 'nightly' or H.get('pick') in ('sweepNode', 'sweepEarly'):
        return judge_sweep(H, H.get('since') or since)
    if H.get('blocked'):
        n = len(defl); out.update(n=n, verdict=('ready' if n >= H['minN'] else 'blocked'),
            bar=('the event ledger holds %d of %d' % (n, H['minN'])) if n < H['minN'] else 'ledger has %d events — the join can be run' % n)
        return out
    if H.get('gap'):
        a, b = pick_rows(H['pick'], eps)
        na, ha, ra = rate(a); nb, hb, rb = rate(b)
        n = min(na, nb); out.update(n=n, nA=na, nB=nb)
        if n < H['minN']: out['verdict'] = 'thin'; return out
        gap = abs(100*ra - 100*rb); thr = _points(H.get('predict', ''), 8)
        out.update(rate=round(100*ra, 1), rateB=round(100*rb, 1), gap=round(gap, 1))
        out['verdict'] = 'cleared' if gap < thr else 'refused'      # for a NULL, "cleared" = the null stands
        out['bar'] = ('gap %.1f pts < %d: the null stands' % (gap, thr)) if gap < thr else ('gap %.1f pts >= %d: the null is refuted' % (gap, thr))
        return out
    sub, base = pick_rows(H['pick'], eps)
    n, h, r = rate(sub); nb, hb, rb = rate(base)
    out.update(n=n, nBase=nb)
    if n < H['minN'] or rb is None: out['verdict'] = 'thin'; return out
    lo, hi = wilson(h, n)
    out.update(rate=round(100*r, 1), base=round(100*rb, 1), ci=[round(100*lo), round(100*hi)])
    want_less = 'LESS' in H['claim'].upper() or '<' in H.get('predict', '')
    thr = _points(H.get('predict', ''), 50)
    holds = (100*r < thr) if want_less else (100*r > thr)
    excludes = (hi < rb) if want_less else (lo > rb)
    beats_null = abs(100*r - 100*rb) > null95
    if holds and excludes and beats_null: out['verdict'] = 'cleared'; out['bar'] = 'holds, CI excludes base, beats the best-of-K null (%.1f pts)' % null95
    elif not holds: out['verdict'] = 'refused'; out['bar'] = 'the prediction (%s) does not hold' % H.get('predict', '')
    elif not excludes: out['verdict'] = 'refused'; out['bar'] = 'CI [%d-%d] covers the base rate %.0f%%' % (round(100*lo), round(100*hi), 100*rb)
    else: out['verdict'] = 'refused'; out['bar'] = 'inside the best-of-K noise band (%.1f pts)' % null95
    return out

def _points(txt, dflt):
    import re
    m = re.search(r'(\d+(?:\.\d+)?)\s*(%|points?|pts)', txt or '')
    return float(m.group(1)) if m else dflt

# ---- HOD/LOD: live vs the table, close-scored only -----------------------------------------------
def lodhod_calibration(days, frm, sym='SPY'):
    cells = collections.defaultdict(lambda: [0, 0, 0.0]); sessions = set()
    for d, D in days:
        if d < frm: continue
        for r in (D.get('feat') or {}).get(sym) or []:
            if not r or r.get('key') != 'lodhod' or not r.get('atClose') or r.get('hit') is None: continue
            p = (r.get('rec') or {}).get('p')
            if not isinstance(p, (int, float)): continue
            b = int(p // 20) * 20; c = cells['%d-%d' % (b, b + 19)]
            c[0] += 1; c[1] += (1 if r['hit'] else 0); c[2] += p; sessions.add(d)
    out = [dict(cell=k, n=v[0], live=round(100*v[1]/v[0]), table=round(v[2]/v[0])) for k, v in sorted(cells.items())]
    return dict(sessions=len(sessions), cells=out,
                note='close-scored rows only (v15.51+); a flat live column against a rising table column means the scorer cannot fail')

# ---- main ----------------------------------------------------------------------------------------
def run(upto=None, write=True, reg_path=REG, days=None):
    R = json.load(open(reg_path)); frm = R.get('from', '2026-09-03')
    days = days if days is not None else load_days(upto)
    if not days: print('no day files under data/'); return None
    last = days[-1][0]
    eps = episodes(days, frm); defl = defl_events(days, frm)
    H_list = R['hypotheses']
    null = shuffle_null(H_list, eps) if eps else dict(p50=0, p95=0)
    verdicts = [judge(H, eps, defl, null['p95'], since=frm) for H in H_list]
    # (v15.55) TRACK requests: whatever he typed into an Analysis-tab field rides in the day export; copy every new one
    # into learning/requests.json (append-only, by id) so the review can turn it into a study row.
    try:
        newreq = ingest_requests(days) if write else 0
    except Exception as eR:
        newreq = 0; print('requests ingest threw:', eR)
    # (v15.55) refresh the sweep table the panel reads (data/es-1min/SWEEPS.json) when the ES file is present
    try:
        if write: refresh_sweeps()
    except Exception as eS:
        print('sweep refresh threw:', eS)
    cal = lodhod_calibration(days, frm)
    n_sess = len(set(e['_day'] for e in eps))
    thin = sum(1 for v in verdicts if v['verdict'] in ('thin', 'blocked'))
    preopen = ('%d session%s since %s · %d episodes · %d of %d hypotheses still thin · null band %.1f pts'
               % (n_sess, '' if n_sess == 1 else 's', frm, len(eps), thin, len(verdicts), null['p95']))
    log = dict(schema=2, date=last, writtenBy='tools/nightly/run.py', from_=frm, sessions=n_sess, episodes=len(eps),
               deflEvents=len(defl), null=null, hypotheses=verdicts, lodhod=cal, preopen=preopen, newRequests=(newreq if write else 0))
    log['from'] = log.pop('from_')
    if write:
        os.makedirs(LOGD, exist_ok=True)
        p = os.path.join(LOGD, last + '.json'); io.open(p, 'w', encoding='utf-8').write(json.dumps(log, indent=1))
        print('wrote', os.path.relpath(p, ROOT))
    print(preopen)
    for v in verdicts:
        print('  %-3s %-9s n %4s/%-3s %s %s' % (v['id'], v['verdict'].upper(), v.get('n', '-'), v['minN'],
              (('%s%%' % v['rate']) if v.get('rate') is not None else ''), v.get('bar', '')))
    return log

# ---- (v15.55) TRACK requests and the sweep table -----------------------------------------------------
REQ = os.path.join(ROOT, 'learning', 'requests.json')

def ingest_requests(days):
    cur = {'schema': 1, 'note': 'what he asked to be tracked, copied from data/<day>.json `requests` by the nightly; the review turns each into a study row (studies.json carries req:<id>) and marks it here', 'requests': []}
    if os.path.exists(REQ):
        try: cur = json.load(open(REQ))
        except Exception: pass
    seen = set(r.get('id') for r in cur.get('requests', []))
    added = 0
    for d, D in days:
        for r in (D.get('requests') or []):
            if not r or not r.get('id') or r['id'] in seen: continue
            cur['requests'].append(dict(id=r['id'], subj=r.get('subj'), text=r.get('text'), date=r.get('date'), seenIn=d, status='NEW'))
            seen.add(r['id']); added += 1
    if added or not os.path.exists(REQ):
        io.open(REQ, 'w', encoding='utf-8').write(json.dumps(cur, indent=1, ensure_ascii=False))
        print('requests: %d new -> learning/requests.json (%d total)' % (added, len(cur['requests'])))
    return added

def refresh_sweeps():
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    es = os.path.join(ROOT, 'data', 'es-1min', 'ES TestingData.txt')
    if os.path.exists(es):
        sw = __import__('study-sweeps')
        out = sw.run(es); out.pop('events', None)
        io.open(os.path.join(ROOT, 'data', 'es-1min', 'SWEEPS.json'), 'w', encoding='utf-8').write(json.dumps(out, indent=1))
        print('sweeps: refreshed data/es-1min/SWEEPS.json (%d sessions, %d cells)' % (out['corpus']['sessions'], out['ledger']['cells_read']))
    # (v15.56) the book table from the panel's own day files
    sb = __import__('study-sweeps-book')
    ob = sb.run(); ob.pop('events', None)
    io.open(os.path.join(ROOT, 'data', 'es-1min', 'SWEEPS-BOOK.json'), 'w', encoding='utf-8').write(json.dumps(ob, indent=1))
    print('sweeps x book: refreshed data/es-1min/SWEEPS-BOOK.json (%d sessions, %d cells)' % (ob['corpus'].get('sessions', 0), ob['ledger'].get('cells_read', 0)))
    return True

# ---- self-test: the harness must find a planted effect and refuse a planted nothing ---------------
def selftest():
    rnd = random.Random(5)
    def mkdays(effect):
        days = []
        for i in range(12):
            d = '2026-09-%02d' % (3 + i); F = []
            for k in range(16):
                grade = rnd.choice(['A', 'B', 'C']); tap = rnd.choice([0, 0, 1]); pol = rnd.choice(['+', '-'])
                p_hold = 0.5
                if effect and grade == 'A': p_hold = 0.15          # the planted effect: A holds far less
                hit = 1 if rnd.random() < p_hold else 0
                F.append(dict(key='node', t=1000 + k, resolved=True, hit=hit, rec=dict(k=7600 + 5*k, grade=grade, tap=tap, pol=pol)))
                F.append(dict(key='reaction', t=1000 + k, resolved=True, hit=hit, rec=dict(k=7600 + 5*k, quality=rnd.choice(['weak', 'confirmed']))))
            days.append((d, dict(feat=dict(SPY=F), defl=dict(SPY=[]))))
        return days
    a = run(write=False, days=mkdays(True)); b = run(write=False, days=mkdays(False))
    H1a = [v for v in a['hypotheses'] if v['id'] == 'H1'][0]; H1b = [v for v in b['hypotheses'] if v['id'] == 'H1'][0]
    ok1 = H1a['verdict'] == 'cleared'; ok2 = H1b['verdict'] in ('thin', 'refused')
    print('\nSELFTEST  planted effect found: %s (%s)   planted nothing refused/thin: %s (%s)' % (ok1, H1a['verdict'], ok2, H1b['verdict']))
    return ok1 and ok2

if __name__ == '__main__':
    if '--selftest' in sys.argv: sys.exit(0 if selftest() else 1)
    upto = next((a for a in sys.argv[1:] if not a.startswith('--')), None)
    sys.exit(0 if run(upto) else 1)
