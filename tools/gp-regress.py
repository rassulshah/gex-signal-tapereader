#!/usr/bin/env python3
"""
gp-regress.py — the lsGammaProfile LIVE regression runner (Gate A + Gate B expectations + the Atlas cross-check).

  python3 tools/gp-regress.py --csv GammaProfile.csv --audit GammaProfile.audit.json \
      [--atlas-read atlas.json] [--irt-read irt.json] [--shots shot1.png shot2.png] [--note "..."] [--out testing/gamma-profile/runs]

What it checks, and where each gap would live (design/IRT-VS-SKYLIT-TESTING-PLAN.md):
  GATE A  Atlas -> CSV.  The audit sidecar is what the panel READ from Atlas at the CSV's own timestamp (the tape,
          King, ratio, spot, the companion's 0DTE numbers). Every CSV row is re-derived from it and diffed.
  ATLAS   Atlas screen -> panel.  --atlas-read is a JSON {strike: pct, ...} transcribed from an Atlas screenshot
          taken at the same minute; it is diffed against the tape the panel read (a parse error would show here).
  GATE B  CSV -> chart.  The runner prints the EXPECTED chart (which node carries K/C/F/G, the CW/PW tags, where the
          FLIP tick sits, the bracket runs, the regime line) using the same rules as GammaProfileLogic.h, and diffs
          it against --irt-read, a JSON transcribed from the IRT screenshot: {"tags":{"7610":"G.C","7605":"F"},
          "cw":"7675","pw":"7600","flip_between":[7620,7625],"regime":"-gamma | WHIPSAW | ...","top5":[7685,...]}.
  Every run writes testing/gamma-profile/runs/<date>/<HHMM>.md (inputs, every check, the screenshots' file names,
  the operator's notes) and appends one line to testing/gamma-profile/RESULTS.md. Exit code = failures.
"""
import argparse, json, os, sys, datetime, math, shutil

def num(x):
    try: return float(x)
    except: return None

def read_csv(path):
    R = {}
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            line = line.strip()
            if not line: continue
            t = line.split(',')
            R.setdefault(t[0], []).append(t[1:])
    return R

def es_of(spx, ratio): return round(spx * ratio / 0.25) * 0.25

# ---- the plugin's decisions, ported (must match plugin/GammaProfileLogic.h; the C++ test pins the C++) ----
GK_MIN_PCT, AIR_THIN, AIR_EDGE, AIR_MIN_RUN = 30.0, 8.0, 20.0, 3
def roles(nodes, spot):
    """nodes: list of dict(spx, pct, king). returns dict spx -> tag among K,C,F,G,G.C,G.F"""
    k = next((n for n in nodes if n['king']), None)
    tags = {}
    if k: tags[k['spx']] = 'K'
    below = [n for n in nodes if not n['king'] and n['spx'] < spot]
    above = [n for n in nodes if not n['king'] and n['spx'] > spot]
    f = max(below, key=lambda n: abs(n['pct'])) if below else None
    c = max(above, key=lambda n: abs(n['pct'])) if above else None
    if c: tags[c['spx']] = 'C'
    if f: tags[f['spx']] = 'F'
    g = None
    if k:
        lo, hi = min(spot, k['spx']), max(spot, k['spx'])
        cand = [n for n in nodes if not n['king'] and lo < n['spx'] < hi and abs(n['pct']) >= GK_MIN_PCT]
        if cand:
            g = max(cand, key=lambda n: abs(n['pct']))
            tags[g['spx']] = 'G.C' if (c and g['spx'] == c['spx']) else ('G.F' if (f and g['spx'] == f['spx']) else 'G')
    return tags

def air_pockets(nodes):
    s = sorted(nodes, key=lambda n: n['spx']); out = []; i = 0
    while i < len(s):
        if abs(s[i]['pct']) < AIR_THIN:
            j = i
            while j + 1 < len(s) and abs(s[j+1]['pct']) < AIR_THIN: j += 1
            lowEdge = i > 0 and abs(s[i-1]['pct']) >= AIR_EDGE
            highEdge = j + 1 < len(s) and abs(s[j+1]['pct']) >= AIR_EDGE
            if j - i + 1 >= AIR_MIN_RUN and lowEdge and highEdge:
                out.append((s[i]['spx'], s[j]['spx'], sum(n['pct'] for n in s[i:j+1]) < 0))
            i = j + 1
        else: i += 1
    return out

def regime_line(sign, typ, conflict):
    tact = "FOLLOW, don't fade" if typ.startswith('TREND') else ("fade EXTREMES only / sit out" if typ == 'WHIPSAW' else ("FADE the extremes" if typ == 'RANGE' else "no edge - wait"))
    sg = '-gamma' if sign == 'NEG' else ('+gamma' if sign == 'POS' else ('AT flip' if sign == 'AT' else 'flip n/a'))
    ty = {'TREND_UP': 'TREND UP', 'TREND_DN': 'TREND DOWN'}.get(typ, typ or 'FORMING')
    return "REGIME  %s  |  %s  |  %s%s" % (sg, ty, tact, '  !CONFLICT' if conflict else '')

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--csv', required=True); ap.add_argument('--audit', required=True)
    ap.add_argument('--atlas-read'); ap.add_argument('--irt-read'); ap.add_argument('--shots', nargs='*', default=[])
    ap.add_argument('--note', default=''); ap.add_argument('--out', default='testing/gamma-profile/runs')
    ap.add_argument('--label', default='')
    a = ap.parse_args()

    R = read_csv(a.csv)
    AU = json.load(open(a.audit, encoding='utf-8'))
    results = []   # (status, section, message)
    def add(ok, sec, msg): results.append(('ok' if ok else 'FAIL', sec, msg))
    def warn(sec, msg): results.append(('warn', sec, msg))

    # ---------------- timing ----------------
    asof = num(R.get('ASOF', [['']])[0][0]); au_ct = AU.get('ct')
    add(asof is not None and au_ct is not None and abs(asof - au_ct) <= 5, 'time', 'CSV ASOF %s and audit ct %s are the same export (<=5s)' % (asof, au_ct))
    def hhmm(so):
        if so is None: return '?'
        so = int(so); return '%02d:%02d:%02d' % (so // 3600, (so % 3600) // 60, so % 60)

    # ---------------- Gate A: the ladder ----------------
    tape = AU.get('tape') or {}; ratio = (AU.get('ratio') or {}).get('SPXW'); king = AU.get('king'); kingNeg = AU.get('kingNeg')
    strikes = R.get('STRIKE', [])
    add(ratio is not None and ratio > 0.98, 'A', 'SPX->ES ratio present in the audit: %s (front-month basis %+.1f pts on the King)' % (ratio, (king * ratio - king) if (ratio and king) else float('nan')))
    add(len(strikes) == len(tape), 'A', 'STRIKE rows (%d) == tape strikes the panel read (%d)' % (len(strikes), len(tape)))
    bad_es, bad_pct, bad_spx = [], [], []
    nodes = []
    for t in strikes:
        if len(t) < 6: continue
        es, pct, rank, isk, typ, spx = num(t[0]), num(t[1]), int(t[2]), t[3] == '1', t[4], num(t[5])
        key = '%.2f' % spx
        tp = tape.get(key)
        if tp is None: bad_spx.append(spx); continue
        if ratio and abs(es - es_of(spx, ratio)) > 0.001: bad_es.append((spx, es, es_of(spx, ratio)))
        exp = (-100 if (tp < 0 or kingNeg) else 100) if isk else round(tp)
        if int(pct) != exp: bad_pct.append((spx, pct, exp))
        nodes.append(dict(spx=spx, pct=pct, rank=rank, king=isk, type=typ, es=es))
    add(not bad_spx, 'A', 'every STRIKE row is a strike the panel read from the tape' + ('' if not bad_spx else ' — extras: %s' % bad_spx[:5]))
    add(not bad_es, 'A', 'every ES price = round(SPX x ratio, 0.25)' + ('' if not bad_es else ' — off: %s' % bad_es[:5]))
    add(not bad_pct, 'A', 'every %King equals the tape (King forced to +/-100 by its sign)' + ('' if not bad_pct else ' — off: %s' % bad_pct[:5]))
    ranked = sorted(nodes, key=lambda n: (-abs(n['pct']), n['spx']))
    add(all(ranked[i]['rank'] == i + 1 for i in range(min(10, len(ranked)))), 'A', 'ranks 1..10 follow |%King| (ties to the lower strike)')
    kn = [n for n in nodes if n['king']]
    add(len(kn) == 1 and kn[0]['spx'] == king, 'A', 'exactly one King flag, on the tape King %s' % king)
    KING = num(R.get('KING', [['']])[0][0]); add(KING is not None and kn and abs(KING - kn[0]['es']) < 0.001, 'A', 'KING row (%s) == the flagged strike\'s ES' % KING)
    SR = num(R.get('SCALEREF', [['']])[0][0]); add(SR is not None and AU.get('scaleRef') is not None and abs(SR - AU['scaleRef']) < 0.01, 'A', 'SCALEREF %s == the ES1 payload spot the panel read (%s)' % (SR, AU.get('scaleRef')))
    if SR and kn: add(abs(SR - kn[0]['es']) <= 200, 'A', 'SCALEREF within 200 pts of the King (sanity)')

    # ---------------- Gate A: the IF rows ----------------
    ifc = AU.get('ifc') or {}; d0 = ifc.get('dte0') or {}
    flip_spx = (d0.get('gf') or {}).get('flip'); cr = d0.get('cr'); ps = d0.get('ps')
    fresh = ifc and not ifc.get('err') and not ifc.get('stale')
    for name, val in (('FLIP', flip_spx), ('CW', cr), ('PW', ps)):
        row = R.get(name)
        if fresh and val:
            add(row is not None and abs(num(row[0][0]) - es_of(val, ratio)) < 0.001 and row[0][2] == '0DTE', 'A', '%s row = companion 0DTE %s -> ES %s, window 0DTE' % (name, val, row[0][0] if row else None))
        else:
            add(row is None, 'A', '%s row ABSENT because the chain is %s (never an all-expiry substitute)' % (name, 'stale' if ifc.get('stale') else ('missing' if not ifc else 'without a value')))
    # ---------------- Gate A: REGIME ----------------
    RG = R.get('REGIME'); spot = (AU.get('spot') or {}).get('spxUsed')
    if RG:
        sign, typ, conf, conflict = RG[0][0], RG[0][1], RG[0][2], RG[0][3] == '1'
        if spot and flip_spx and fresh:
            d = spot - flip_spx; exp_sign = 'AT' if abs(d) <= 3 else ('POS' if d > 0 else 'NEG')
            add(sign == exp_sign, 'A', 'REGIME sign %s: spot %.2f vs flip %.2f (%+.2f) -> expected %s' % (sign, spot, flip_spx, d, exp_sign))
        else:
            add(sign == 'NA', 'A', 'REGIME sign NA when spot or flip is missing (spot %s, flip %s, fresh %s)' % (spot, flip_spx, fresh))
        add(typ in ('RANGE', 'TREND_UP', 'TREND_DN', 'WHIPSAW', 'MIXED', 'FORMING'), 'A', 'REGIME type is one of the six (%s, %s)' % (typ, conf))
        au_rg = AU.get('regime') or {}
        add(au_rg.get('type') == typ and au_rg.get('sign') == sign, 'A', 'REGIME row == the audit\'s regime read')
    else:
        add(False, 'A', 'REGIME row present')
    add((AU.get('spot') or {}).get('src') in ('trinity', 'ladder', 'ifchain'), 'A', 'spot source named: %s (spx %s)' % ((AU.get('spot') or {}).get('src'), spot))

    # ---------------- ATLAS cross-check ----------------
    if a.atlas_read:
        AR = json.load(open(a.atlas_read)); diffs = []; missing = []
        for k, v in AR.items():
            key = '%.2f' % float(k); tp = tape.get(key)
            if tp is None: missing.append(k); continue
            if abs(float(v) - tp) > 1.5: diffs.append((k, v, tp))
        add(not missing, 'ATLAS', 'every strike read off the Atlas screen exists in the tape the panel read' + ('' if not missing else ' — missing: %s' % missing))
        add(not diffs, 'ATLAS', '%d Atlas-screen values within +/-1 of the panel\'s tape' % len(AR) + ('' if not diffs else ' — off: %s' % diffs))
        if 'king' in AR: add(float(AR['king']) == king, 'ATLAS', 'Atlas King %s == panel King %s' % (AR['king'], king))
        if 'es1' in AR and SR: add(abs(float(AR['es1']) - SR) <= 1.0, 'ATLAS', 'Atlas ES1 %s vs SCALEREF %s within 1 pt' % (AR['es1'], SR))
    else:
        warn('ATLAS', 'no --atlas-read: the Atlas screen was not transcribed for this run')

    # ---------------- Gate B: the expected chart ----------------
    exp = {}
    if nodes and spot:
        tags = roles(nodes, spot)
        exp['tags'] = {('%g' % k): v for k, v in tags.items()}
        top5 = [n['spx'] for n in ranked[:5]]; exp['top5'] = top5
        exp['cw'] = '%g' % cr if (cr and fresh) else None; exp['pw'] = '%g' % ps if (ps and fresh) else None
        if flip_spx and fresh:
            below = max([n['spx'] for n in nodes if n['spx'] <= flip_spx] or [None]); above = min([n['spx'] for n in nodes if n['spx'] >= flip_spx] or [None])
            exp['flip_between'] = [below, above]
        st = {}
        for n in nodes:
            if n['type'] in ('PIKA', 'PIKAM', 'BARNEY', 'BARNEYM'): st.setdefault('P' if n['type'].startswith('PIKA') else 'B', []).append(n['spx'])
        exp['brackets'] = {k: [min(v), max(v)] for k, v in st.items()}
        exp['named'] = {('%g' % n['spx']): {'PIKA': 'P', 'BARNEY': 'B', 'RUG': 'R', 'RRUG': 'RR'}[n['type']] for n in nodes if n['type'] in ('PIKA', 'BARNEY', 'RUG', 'RRUG')}
        exp['air'] = [(lo, hi, neg) for lo, hi, neg in air_pockets(nodes)]
        if RG: exp['regime'] = regime_line(RG[0][0], RG[0][1], RG[0][3] == '1')
        exp['basis_note'] = 'chart price = ES price + (chart last close - SCALEREF %s)' % SR
    if a.irt_read:
        IR = json.load(open(a.irt_read))
        for k, v in (IR.get('tags') or {}).items():
            add(exp.get('tags', {}).get(k) == v, 'B', 'IRT tag on %s reads %s (expected %s)' % (k, v, exp.get('tags', {}).get(k)))
        for k in exp.get('tags', {}):
            if k not in (IR.get('tags') or {}): add(False, 'B', 'expected tag %s on %s not seen on IRT' % (exp['tags'][k], k))
        if 'cw' in IR: add(str(IR['cw']) == str(exp.get('cw')), 'B', 'CW tag on %s (expected %s)' % (IR['cw'], exp.get('cw')))
        if 'pw' in IR: add(str(IR['pw']) == str(exp.get('pw')), 'B', 'PW tag on %s (expected %s)' % (IR['pw'], exp.get('pw')))
        if 'flip_between' in IR: add(list(IR['flip_between']) == list(exp.get('flip_between') or []), 'B', 'FLIP tick between %s (expected %s)' % (IR['flip_between'], exp.get('flip_between')))
        if 'top5' in IR: add(sorted(IR['top5']) == sorted(exp.get('top5') or []), 'B', 'top-5 badges on %s (expected %s)' % (IR['top5'], exp.get('top5')))
        if 'regime' in IR: add(IR['regime'].replace(' ', '') == (exp.get('regime') or '').replace(' ', ''), 'B', 'regime line reads "%s" (expected "%s")' % (IR['regime'], exp.get('regime')))
        for k, v in (IR.get('named') or {}).items(): add(exp.get('named', {}).get(k) == v, 'B', 'pattern letter %s on %s (expected %s)' % (v, k, exp.get('named', {}).get(k)))
    else:
        warn('B', 'no --irt-read: the IRT screen was not transcribed; the expected chart below is the checklist')

    # ---------------- write the run ----------------
    fails = sum(1 for s, _, _ in results if s == 'FAIL')
    now = datetime.datetime.now()
    day = now.strftime('%Y-%m-%d'); od = os.path.join(a.out, day); os.makedirs(od, exist_ok=True)
    stamp = hhmm(asof).replace(':', '')[:4] if asof is not None else now.strftime('%H%M')
    name = stamp + (('-' + a.label) if a.label else '') + '.md'
    shots = []
    for s in a.shots:
        if os.path.exists(s):
            dst = os.path.join(od, stamp + '-' + os.path.basename(s)); shutil.copy(s, dst); shots.append(os.path.basename(dst))
    verdict = 'PASS' if fails == 0 else 'FAIL'
    lines = ['# lsGammaProfile regression — %s %s CT — %s' % (day, hhmm(asof), verdict), '',
             '_CSV ASOF %s · panel %s · King %s (%s) · ratio %s · spot %s (%s) · SCALEREF %s · flip %s · CW %s · PW %s · rolls %s_' % (
                 hhmm(asof), AU.get('v'), king, '-' if kingNeg else '+', ratio, spot, (AU.get('spot') or {}).get('src'), SR, flip_spx, cr, ps, (AU.get('rolls') or {}).get('SPX')), '']
    if a.note: lines += ['**Operator / runner note:** ' + a.note, '']
    if shots: lines += ['Screenshots: ' + ', '.join('`%s`' % s for s in shots), '']
    for sec, title in (('time', 'Timing'), ('A', 'Gate A — Atlas (as read) -> CSV'), ('ATLAS', 'Atlas screen -> panel'), ('B', 'Gate B — CSV -> IRT chart')):
        rows = [r for r in results if r[1] == sec]
        if not rows: continue
        lines.append('## ' + title); lines.append('')
        for s, _, m in rows: lines.append('- %s  %s' % ({'ok': '✅', 'FAIL': '❌', 'warn': '⚠️'}[s], m))
        lines.append('')
    if exp:
        lines += ['## Expected chart (the Gate B checklist for the IRT screenshot)', '', '```json', json.dumps(exp, indent=1), '```', '']
    with open(os.path.join(od, name), 'w', encoding='utf-8') as f: f.write('\n'.join(lines))
    res_path = os.path.join(os.path.dirname(a.out.rstrip('/')), 'RESULTS.md') if os.path.basename(a.out.rstrip('/')) == 'runs' else os.path.join(a.out, 'RESULTS.md')
    if os.path.exists(res_path):
        with open(res_path, 'a', encoding='utf-8') as f:
            f.write('| %s | %s | %s | %s | %d/%d | %s | [%s](runs/%s/%s) |\n' % (day, hhmm(asof), AU.get('v'), verdict, sum(1 for s, _, _ in results if s == 'ok'), len([r for r in results if r[0] != 'warn']),
                    (a.note or '').replace('|', '/')[:80], name.replace('.md', ''), day, name))
    for s, sec, m in results: print('  %-4s [%s] %s' % (s, sec, m))
    print('=== gp-regress: %s — %d checks, %d failed -> %s ===' % (verdict, len([r for r in results if r[0] != 'warn']), fails, os.path.join(od, name)))
    return fails

if __name__ == '__main__':
    sys.exit(main())
