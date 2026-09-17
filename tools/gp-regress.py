#!/usr/bin/env python3
"""
gp-regress.py — the lsGammaProfile LIVE regression runner (Gate A + Gate B expectations + the Atlas cross-check).

  python3 tools/gp-regress.py --csv GammaProfile.csv --audit GammaProfile.audit.json \
      [--atlas-read atlas.json] [--irt-read irt.json] [--shots shot1.png shot2.png] [--note "..."] [--out testing/gamma-profile/runs]
  python3 tools/gp-regress.py --indicator daymodel|daystats|kingtracker --csv ... --audit ... [--irt-read irt.json] [--shots ...]
      (v16.37) the same same-moment run for the other three indicators: the day rows are re-derived from the audit's AU.day
      block by tools/day-derive.js (the panel's own functions), the King rows against AU.kingNow / AU.rolls; each prints the
      EXPECTED chart / strip for its eyes-on checklist and writes testing/<indicator folder>/runs/<date>/<HHMM>.md + RESULTS.md.

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
import argparse, json, os, sys, datetime, math, shutil, subprocess

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
    ap.add_argument('--indicator', default='gamma', choices=['gamma', 'daymodel', 'daystats', 'kingtracker'])
    ap.add_argument('--dry', action='store_true', help='check only: no run file, no RESULTS.md line (tools/regress.py runs the fixtures this way)')
    a = ap.parse_args()

    R = read_csv(a.csv)
    AU = json.load(open(a.audit, encoding='utf-8'))
    if a.indicator != 'gamma':
        if a.out == 'testing/gamma-profile/runs': a.out = OTHER_FOLDERS[a.indicator] + '/runs'
        return main_other(a, R, AU)
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
    # (v16.30) THE IF BOOK: GammaProfile-IF.csv carries BOOK,IF0DTE; its "tape" is the audit's ifProf (the normalised
    # InsiderFinance 0DTE ladder the panel derived from the companion's gexProf) and its King is ifProf.king.
    book_row = R.get('BOOK', [['']])[0][0]
    if book_row == 'IF0DTE':
        ip = AU.get('ifProf') or {}
        tape = {('%.2f' % k): v for k, v in (ip.get('prof') or [])}
        king = ip.get('king'); kingNeg = (tape.get('%.2f' % king, 0) < 0) if king is not None else None
        add(ip.get('ok') is True, 'A', 'IF book: audit.ifProf ok (king %s, %s strikes, coverage %s%%, spot from %s, payload %s)' % (king, ip.get('n'), ip.get('coverage'), ip.get('spotSrc'), ip.get('payloadT')))
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
        if book_row == 'IF0DTE':
            add(sign == (AU.get('regime') or {}).get('sign', sign), 'A', 'IF book: REGIME sign == the Skylit file\'s sign (same flip, same spot); type %s is read on IF\'s own structure' % typ)
        else:
            add(au_rg.get('type') == typ and au_rg.get('sign') == sign, 'A', 'REGIME row == the audit\'s regime read')
    else:
        add(False, 'A', 'REGIME row present')
    add((AU.get('spot') or {}).get('src') in ('trinity', 'ladder', 'ifchain'), 'A', 'spot source named: %s (spx %s)' % ((AU.get('spot') or {}).get('src'), spot))

    # ---------------- ATLAS cross-check ----------------
    if a.atlas_read and book_row == 'IF0DTE':
        warn('ATLAS', 'IF book: the Atlas screen is not this book\'s source; --atlas-read ignored')
    elif a.atlas_read:
        AR = json.load(open(a.atlas_read)); diffs = []; missing = []
        for k, v in AR.items():
            if k in ('king', 'es1', 'notes'): continue
            key = '%.2f' % float(k); tp = tape.get(key)
            if tp is None: missing.append(k); continue
            if abs(float(v) - tp) > 1.5: diffs.append((k, v, tp))
        add(not missing, 'ATLAS', 'every strike read off the Atlas screen exists in the tape the panel read' + ('' if not missing else ' — missing: %s' % missing))
        nvals = len([k for k in AR if k not in ('king', 'es1', 'notes')])
        add(not diffs, 'ATLAS', '%d Atlas-screen values within +/-1.5 of the panel\'s tape' % nvals + ('' if not diffs else ' — off: %s' % diffs))
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
        exp['named'] = {('%g' % n['spx']): {'PIKA': 'P', 'BARNEY': 'B', 'RUG': 'R', 'RRUG': 'RR'}[n['type']] for n in nodes if n['type'] in ('PIKA', 'BARNEY', 'RUG', 'RRUG') and not n['king']}   # the King's name wins on its own bar
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
        if 'top5' in IR:
            seen = set(IR['top5']); expected = set(exp.get('top5') or [])
            partial = len(IR['top5']) < 5
            add(seen <= expected if partial else seen == expected, 'B', 'top-5 badges on %s (expected %s)%s' % (IR['top5'], exp.get('top5'), ' — partial read, subset check' if partial else ''))
        if 'regime' in IR: add(IR['regime'].replace(' ', '') == (exp.get('regime') or '').replace(' ', ''), 'B', 'regime line reads "%s" (expected "%s")' % (IR['regime'], exp.get('regime')))
        for k, v in (IR.get('named') or {}).items(): add(exp.get('named', {}).get(k) == v, 'B', 'pattern letter %s on %s (expected %s)' % (v, k, exp.get('named', {}).get(k)))
    else:
        warn('B', 'no --irt-read: the IRT screen was not transcribed; the expected chart below is the checklist')

    # ---------------- write the run ----------------
    fails = sum(1 for s, _, _ in results if s == 'FAIL')
    if a.dry:
        for s, sec, m in results: print('  %-4s [%s] %s' % (s, sec, m))
        print('=== gp-regress (dry): %s — %d checks, %d failed ===' % ('PASS' if fails == 0 else 'FAIL', len([r for r in results if r[0] != 'warn']), fails))
        return fails
    now = datetime.datetime.now()
    day = now.strftime('%Y-%m-%d'); od = os.path.join(a.out, day); os.makedirs(od, exist_ok=True)
    stamp = hhmm(asof).replace(':', '')[:4] if asof is not None else now.strftime('%H%M')
    name = stamp + ('-IF' if book_row == 'IF0DTE' else '') + (('-' + a.label) if a.label else '') + '.md'
    shots = []
    for s in a.shots:
        if os.path.exists(s):
            dst = os.path.join(od, stamp + '-' + os.path.basename(s)); shutil.copy(s, dst); shots.append(os.path.basename(dst))
    verdict = 'PASS' if fails == 0 else 'FAIL'
    lines = ['# lsGammaProfile regression — %s %s CT — %s%s' % (day, hhmm(asof), verdict, ' — IF BOOK' if book_row == 'IF0DTE' else ''), '',
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

# =====================================================================================================================
# (v16.37) THE OTHER THREE INDICATORS — same rules: same moment, every row re-derived from what the panel recorded,
# the expected picture printed for the eyes-on check, one run file + one RESULTS line.
# =====================================================================================================================
OTHER_FOLDERS = { 'daymodel': 'testing/day-model', 'daystats': 'testing/day-stats', 'kingtracker': 'testing/king-tracker' }
OTHER_TITLES = { 'daymodel': 'lsDayModel', 'daystats': 'lsDayStats', 'kingtracker': 'lsKingTracker' }
HERE = os.path.dirname(os.path.abspath(__file__))

def hhmm(so):
    if so is None: return '?'
    so = int(so); return '%02d:%02d:%02d' % (so // 3600, (so % 3600) // 60, so % 60)

# ---- lsDayStats' formatting, ported from plugin/DayStatsLogic.h (the C++ test pins the C++; this prints the expected strip)
def ds_clk(so):
    if so is None or so < 0: return '--'
    so = int(so); h = so // 3600; m = (so % 3600) // 60
    ap = 'am' if h < 12 else 'pm'; h12 = h % 12
    if h12 == 0: h12 = 12
    return '%d:%02d%s' % (h12, m, ap)
def ds_dur(mn):
    if mn is None or mn < 0: return '--'
    t = int(round(mn)); h = t // 60; m = t % 60
    return ('%dh %02dm' % (h, m)) if h else ('%dm' % m)
def ds_px1(raw, off):
    v = num(raw)
    return '--' if v is None else '%d' % round(v + off)
def ds_pct(v, tilde):
    return '--' if (v is None or v < 0) else ('~%d%%' % round(v) if tilde else '%d%%' % round(v))
def ds_cells(row, off, expected, cond=None):
    """row = the 17 DAYSA/DAYSE fields; -> the 12 cells lsDayStats prints"""
    f = lambda i: row[i] if i < len(row) else ''
    first, second = f(0), f(9)
    t = lambda i: num(f(i))
    tl = '~' if expected else ''
    eq = (cond and cond.get('basis', '').startswith('read-in')) and expected
    c = [''] * 12
    c[0] = 'E' if expected else 'A'
    c[1] = ('%s %s%s' % (first, '=' if eq else tl, ds_clk(t(2)))) if expected else ('%s %s %s' % (first, ds_clk(t(2)), ds_px1(f(1), off)))
    c[2] = tl + ds_dur(t(3)); c[3] = tl + ds_dur(t(4)); c[4] = tl + ds_dur(t(5)); c[5] = tl + ds_clk(t(6)); c[6] = ds_pct(t(7), expected)
    c[7] = tl + ds_dur(t(8))
    mudt = (t(12) or 0) - (t(4) or 0) if (t(12) is not None and t(4) is not None) else None
    c[8] = tl + ds_dur(mudt)
    if expected:
        c[9] = '%s %s%s' % (second, tl, ds_clk(t(11)))
        if cond and cond.get('lastHr') is not None and cond.get('lastHr') >= 0: c[9] += '  %d%% last hr' % cond['lastHr']
    else:
        c[9] = '%s %s %s' % (second, ds_clk(t(11)), ds_px1(f(10), off))
    c[10] = tl + ds_dur(t(12))
    c[11] = ('~$%s  %sp' % (f(14), f(13))) if expected else ('$%s  %sp' % (f(14), f(13)))
    return c

def derive_day(audit_path):
    try:
        out = subprocess.run(['node', os.path.join(HERE, 'day-derive.js'), audit_path], capture_output=True, text=True, timeout=60)
        if out.returncode != 0: return { 'err': 'day-derive.js rc %s: %s' % (out.returncode, (out.stderr or '')[-300:]) }
        return json.loads(out.stdout)
    except Exception as e:
        return { 'err': str(e) }

def row_eq(a, b, tol=None):
    """CSV row (list of str) vs derived row; numeric fields compared with a tolerance when given"""
    if a is None or b is None or len(a) != len(b): return False
    for x, y in zip(a, b):
        if x == y: continue
        nx, ny = num(x), num(y)
        if tol is not None and nx is not None and ny is not None and abs(nx - ny) <= tol: continue
        return False
    return True

def main_other(a, R, AU):
    results = []
    def add(ok, sec, msg): results.append(('ok' if ok else 'FAIL', sec, msg))
    def warn(sec, msg): results.append(('warn', sec, msg))
    asof = num(R.get('ASOF', [['']])[0][0]); au_ct = AU.get('ct')
    add(asof is not None and au_ct is not None and abs(asof - au_ct) <= 5, 'time', 'CSV ASOF %s and audit ct %s are the same export (<=5s)' % (asof, au_ct))
    SR = num(R.get('SCALEREF', [['']])[0][0]); SRrow = R.get('SCALEREF', [['']])[0]
    SPOT = num(R.get('SPOT', [['']])[0][0])
    IR = json.load(open(a.irt_read)) if a.irt_read else None
    off = num(IR.get('offset')) if (IR and IR.get('offset') is not None) else ((num(IR['chart_close']) - SR) if (IR and IR.get('chart_close') is not None and SR) else 0.0)
    exp = {}
    ind = a.indicator
    if ind in ('daymodel', 'daystats'):
        Y = AU.get('day')
        add(bool(Y) and not Y.get('err'), 'A', 'the audit carries AU.day (panel >= 16.37): %s' % ('yes' if Y and not Y.get('err') else (Y or {}).get('err', 'absent — the day rows cannot be re-derived')))
        DER = derive_day(a.audit) if (Y and not Y.get('err')) else { 'err': 'no AU.day' }
        if DER.get('err'):
            add(False, 'A', 'day-derive.js: %s' % DER['err'])
        else:
            el = Y.get('elapsed')
            stage = 'pre-open' if (el is None or el < 30) else ('open30' if el < 60 else 'open60')
            if ind == 'daymodel':
                for k, tol in (('DAYACT', 0.011), ('DAYEXP', 0.011), ('EXPMODEL', 0.051)):
                    csv = R.get(k, [None])[0]; der = DER.get(k)
                    add(row_eq(csv, der, tol), 'A', '%s row re-derived from the audit == the CSV (%s vs %s)' % (k, ','.join(csv or ['absent']), ','.join(der or ['absent'])))
                EM = R.get('EXPMODEL', [None])[0]
                if EM:
                    basis = EM[0]
                    okb = { 'pre-open': ('exante', 'em-exante', 'weekday'), 'open30': ('open30', 'em-open30', 'weekday'), 'open60': ('open60', 'em-open60', 'weekday') }[stage]
                    add(basis in okb, 'A', 'basis %s fits the stage (%s min after the open -> %s)' % (basis, el, '/'.join(okb)))
                    f = num(EM[3]); add(f is not None and 0.05 <= f <= 0.95, 'A', 'placement f %s inside [0.05, 0.95]' % EM[3])
                    if stage == 'pre-open': add(f == 0.5, 'A', 'pre-open: the candle is symmetric (f 0.50)')
                    add((EM[5] in ('pin', 'est') and EM[4] != '') or (EM[5] == '' and EM[4] == '' and not basis.startswith('em-')), 'A', 'EM %s (%s): present with a pin state, or absent with a non-EM basis' % (EM[4] or '-', EM[5] or '-'))
                    if Y.get('em') and Y['em'].get('atMin') is not None: add(Y['em']['atMin'] <= 60, 'A', 'the EM was pinned %s min after the open (<= 60)' % Y['em']['atMin'])
                DE = R.get('DAYEXP', [None])[0]
                if DE:
                    o, h, l, c = [num(x) for x in DE[:4]]
                    add(h is not None and l is not None and h >= o >= l and h >= c >= l, 'A', 'DAYEXP O %s inside [L %s, H %s], close %s inside too' % (o, l, h, c))
                    exp['expected_candle_es'] = dict(o=o, h=h, l=l, c=c)
                    exp['expected_candle_chart'] = dict(o=o + off, h=h + off, l=l + off, c=c + off)
                DA = R.get('DAYACT', [None])[0]
                if DA:
                    exp['actual_candle_es'] = dict(zip('ohlc', [num(x) for x in DA[:4]]))
                    exp['actual_candle_chart'] = dict(zip('ohlc', [num(x) + off for x in DA[:4]]))
                exp['model'] = DER.get('model'); exp['stage'] = stage; exp['elapsed_min'] = el
                for k in ('DAYEHOD', 'DAYELOD', 'DAYHOD', 'DAYLOD', 'DAYEMUD', 'DAYMUD'):
                    if k in R: exp[k] = R[k][0]
                exp['basis_note'] = 'chart price = ES price + (chart close at the SCALEREF minute %s - SCALEREF %s); offset used here: %+.2f' % (SRrow[1] if len(SRrow) > 1 else '?', SR, off)
                if IR:
                    for k in ('exp', 'act'):
                        if k in IR:
                            want = exp['expected_candle_chart' if k == 'exp' else 'actual_candle_chart']
                            for f_ in ('h', 'l', 'c', 'o'):
                                if f_ in IR[k]: add(abs(num(IR[k][f_]) - want[f_]) <= 1.0, 'B', 'IRT %s candle %s = %s (expected %.2f on the chart)' % (k, f_, IR[k][f_], want[f_]))
                    if 'basis' in IR: add(IR['basis'] == (EM[0] if EM else None), 'B', 'IRT basis chip reads %s (expected %s)' % (IR['basis'], EM[0] if EM else None))
                else:
                    warn('B', 'no --irt-read: the IRT candle was not transcribed; the expected chart below is the checklist')
            else:   # daystats
                for k, tol in (('DAYSA', 0.011), ('DAYSE', 0.011), ('CONDE', 0.051)):
                    csv = R.get(k, [None])[0]; der = DER.get(k)
                    if k == 'DAYSA' and csv is None and der is None:
                        warn('A', 'no DAYSA row yet (hodLod not ready) — consistent with the audit (no actual)'); continue
                    add(row_eq(csv, der, tol), 'A', '%s row re-derived from the audit == the CSV (%s vs %s)' % (k, ','.join(csv or ['absent']), ','.join(der or ['absent'])))
                CE = R.get('CONDE', [None])[0]
                if CE:
                    b = CE[0]
                    okb = { 'pre-open': ('pre-open',), 'open30': ('pos30-bottom', 'pos30-middle', 'pos30-top', 'pre-open'), 'open60': ('pos60-bottom+orclock', 'pos60-middle', 'pos60-top+orclock', 'pos30-bottom', 'pos30-middle', 'pos30-top', 'pre-open') }[stage]
                    add(b in okb, 'A', 'CONDE basis %s fits the stage (%s min -> %s)' % (b, el, '/'.join(okb)))
                    add(num(CE[2]) is not None and num(CE[1]) is not None and num(CE[2]) > num(CE[1]), 'A', 'CONDE t2 %s > t1 %s' % (CE[2], CE[1]))
                    add(len(CE) >= 7 and CE[5] != '' and CE[6] != '', 'A', 'CONDE carries the 2ND ladder (%s%% last hr / %s%% last 30)' % (CE[5] if len(CE) > 5 else '?', CE[6] if len(CE) > 6 else '?'))
                SA = R.get('DAYSA', [None])[0]; SE = R.get('DAYSE', [None])[0]
                if SA and SE:
                    add(not (SA[2] == SE[2] and SA[3] == SE[3] and (Y.get('call') or {}).get('in')), 'A', '(16.36) the E row does not copy the actual 1ST clock/took after the READ (A %s/%s vs E %s/%s)' % (SA[2], SA[3], SE[2], SE[3]))
                cond = { 'basis': CE[0], 'lastHr': num(CE[5]) if len(CE) > 5 and CE[5] != '' else -1 } if CE else None
                if SA: exp['A_cells'] = ds_cells(SA, off, False)
                if SE: exp['E_cells'] = ds_cells(SE, off, True, cond)
                exp['stage'] = stage; exp['elapsed_min'] = el; exp['cond'] = DER.get('cond')
                exp['basis_note'] = 'A prices land on the chart (+%.2f); E clocks = open + t1 / t2; the 2ND cell carries the ladder' % off
                if IR:
                    for k in ('A', 'E'):
                        if k in IR:
                            want = exp.get(k + '_cells') or []
                            for i, cell in enumerate(IR[k]):
                                if cell is None or cell == '': continue
                                w = want[i] if i < len(want) else None
                                add(str(cell).replace(' ', '') == (w or '').replace(' ', ''), 'B', 'IRT %s cell %d reads "%s" (expected "%s")' % (k, i, cell, w))
                else:
                    warn('B', 'no --irt-read: the IRT strip was not transcribed; the expected cells below are the checklist')
    else:   # kingtracker
        rolls = AU.get('rolls') or {}; KN = AU.get('kingNow') or {}
        fam = 'ES'
        for book in ('SPX', 'SPY'):
            steps = [t for t in R.get('KINGTRACK', []) if len(t) >= 5 and t[0] == fam and t[1] == book]
            now = [t for t in R.get('KINGNOW', []) if len(t) >= 4 and t[0] == fam and t[1] == book]
            n = len(steps)
            if book in rolls and rolls[book] is not None and rolls[book] >= 0:
                add(n == rolls[book] + 1, 'A', '%s: %d KINGTRACK steps == audit rolls %s + 1' % (book, n, rolls[book]))
            if n:
                so = [num(t[2]) for t in steps]
                add(all(so[i] < so[i + 1] for i in range(n - 1)), 'A', '%s: the steps are in time order' % book)
                add(all(num(steps[i][4]) != num(steps[i + 1][4]) for i in range(n - 1)), 'A', '%s: no two consecutive steps on the same strike' % book)
                ratios = [num(t[3]) / num(t[4]) for t in steps if num(t[4])]
                add(all(0.95 < r < 1.05 for r in ratios) if book == 'SPX' else all(9.5 < r < 10.6 for r in ratios), 'A', '%s: every step price / strike is the book\'s ratio (%s)' % (book, ', '.join('%.4f' % r for r in ratios[:6])))
            kn = KN.get(book)
            if kn and kn.get('es') is not None:
                add(bool(now) and abs(num(now[0][2]) - kn['es']) < 0.011 and num(now[0][3]) == kn.get('strike'), 'A', '%s: KINGNOW %s == audit kingNow (%s @ %s)' % (book, ','.join(now[0][2:]) if now else 'absent', kn.get('es'), kn.get('strike')))
                if n:
                    last = num(steps[-1][4])
                    if last == kn.get('strike'): add(True, 'A', '%s: the last step is the live King (%s)' % (book, last))
                    else: warn('A', '%s: KINGNOW %s differs from the last step %s — a challenger dwelling (not yet confirmed KTRK_CONFIRM_N times), or the strip has moved since' % (book, kn.get('strike'), last))
            else:
                add(not now, 'A', '%s: no KINGNOW row when the audit has no live King' % book)
            # the expected chart (KingTrackerLogic.h: every step re-derived as strike x nowPx/nowStrike, then + the SCALEREF offset)
            if n and now and num(now[0][3]):
                r = num(now[0][2]) / num(now[0][3])
                exp[book] = { 'steps': [[hhmm(num(t[2])), num(t[4]), round(num(t[4]) * r + off, 2)] for t in steps], 'now': [num(now[0][3]), round(num(now[0][2]) + off, 2), now[0][4] if len(now[0]) > 4 else ''], 'ratio_now': round(r, 5) }
        exp['basis_note'] = 'chart price = re-derived price + (chart close at the SCALEREF minute - SCALEREF %s); offset used here: %+.2f; the plugin refuses an offset > 300' % (SR, off)
        if IR:
            for book in ('SPX', 'SPY'):
                if book in IR and book in exp:
                    if 'now' in IR[book]: add(abs(num(IR[book]['now']) - exp[book]['now'][1]) <= 1.0, 'B', '%s: the live King line on IRT at %s (expected %s on the chart)' % (book, IR[book]['now'], exp[book]['now'][1]))
                    if 'steps' in IR[book]: add(len(IR[book]['steps']) == len(exp[book]['steps']), 'B', '%s: %d steps drawn (expected %d)' % (book, len(IR[book]['steps']), len(exp[book]['steps'])))
                    if 'strike_label' in IR[book]: add(num(IR[book]['strike_label']) == exp[book]['now'][0], 'B', '%s: the label reads strike %s (expected %s)' % (book, IR[book]['strike_label'], exp[book]['now'][0]))
        else:
            warn('B', 'no --irt-read: the IRT lines were not transcribed; the expected chart below is the checklist')
    # ---- write the run
    fails = sum(1 for s, _, _ in results if s == 'FAIL')
    if a.dry:
        for s_, sec, m in results: print('  %-4s [%s] %s' % (s_, sec, m))
        print('=== gp-regress %s (dry): %s — %d checks, %d failed ===' % (ind, 'PASS' if fails == 0 else 'FAIL', len([r for r in results if r[0] != 'warn']), fails))
        return fails
    now_ = datetime.datetime.now(); day = now_.strftime('%Y-%m-%d'); od = os.path.join(a.out, day); os.makedirs(od, exist_ok=True)
    stamp = hhmm(asof).replace(':', '')[:4] if asof is not None else now_.strftime('%H%M')
    name = stamp + (('-' + a.label) if a.label else '') + '.md'
    shots = []
    for s_ in a.shots:
        if os.path.exists(s_):
            dst = os.path.join(od, stamp + '-' + os.path.basename(s_)); shutil.copy(s_, dst); shots.append(os.path.basename(dst))
    verdict = 'PASS' if fails == 0 else 'FAIL'
    lines = ['# %s regression — %s %s CT — %s' % (OTHER_TITLES[ind], day, hhmm(asof), verdict), '',
             '_CSV ASOF %s · panel %s · SPOT %s · SCALEREF %s · elapsed %s min · rolls %s_' % (hhmm(asof), AU.get('v'), SPOT, SR, (AU.get('day') or {}).get('elapsed'), AU.get('rolls')), '']
    if a.note: lines += ['**Operator / runner note:** ' + a.note, '']
    if shots: lines += ['Screenshots: ' + ', '.join('`%s`' % s_ for s_ in shots), '']
    for sec, title in (('time', 'Timing'), ('A', 'Gate A — the panel\'s read -> CSV (re-derived from the audit)'), ('B', 'Gate B — CSV -> IRT')):
        rows = [r for r in results if r[1] == sec]
        if not rows: continue
        lines.append('## ' + title); lines.append('')
        for s_, _, m in rows: lines.append('- %s  %s' % ({'ok': '✅', 'FAIL': '❌', 'warn': '⚠️'}[s_], m))
        lines.append('')
    if exp: lines += ['## Expected picture (the Gate B checklist for the IRT screenshot)', '', '```json', json.dumps(exp, indent=1, default=str), '```', '']
    with open(os.path.join(od, name), 'w', encoding='utf-8') as f: f.write('\n'.join(lines))
    res_path = os.path.join(os.path.dirname(a.out.rstrip('/')), 'RESULTS.md')
    new = not os.path.exists(res_path)
    with open(res_path, 'a', encoding='utf-8') as f:
        if new: f.write('# %s — live runs (same moment: CSV + audit + IRT screenshot)\n\n| date | CSV time | panel | verdict | checks | note | run |\n|---|---|---|---|---|---|---|\n' % OTHER_TITLES[ind])
        f.write('| %s | %s | %s | %s | %d/%d | %s | [%s](runs/%s/%s) |\n' % (day, hhmm(asof), AU.get('v'), verdict, sum(1 for s_, _, _ in results if s_ == 'ok'), len([r for r in results if r[0] != 'warn']),
                (a.note or '').replace('|', '/')[:80], name.replace('.md', ''), day, name))
    for s_, sec, m in results: print('  %-4s [%s] %s' % (s_, sec, m))
    if exp: print('--- expected picture ---'); print(json.dumps(exp, indent=1, default=str))
    print('=== gp-regress %s: %s — %d checks, %d failed -> %s ===' % (ind, verdict, len([r for r in results if r[0] != 'warn']), fails, os.path.join(od, name)))
    return fails


if __name__ == '__main__':
    sys.exit(main())
