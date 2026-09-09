#!/usr/bin/env python3
"""coverage.py — (v15.89) the record on disk, counted: learning/coverage.json for the 🗄 Data tab.

    python3 tools/nightly/coverage.py            # write learning/coverage.json
    python3 tools/nightly/coverage.py --selftest

Operator, 2026-09-09: "i want you to build a data tab … a snapshot summary of the data we currently have, what we need
for the studies, recommendations and more. If there is missing data that we can obtain from yahoo, it should mention
that." The panel cannot read the repo's files — it fetches them from GitHub one by one — so the nightly, which runs on
the machine that HAS them, counts them here and the panel fetches ONE small file. Nothing here is a claim: a session
row is what the day file, the corpus CSVs, the tape folder and the log say, by count. The panel adds the live half
(today's capture, the couriers' ages, the browser stores) itself.

Per session (every data/<day>.json): the file's size, the snapshots per book, the node events, the deflection rows, the
couriered ES / NQ rows and whether they carry the night, the corpus CSVs' bar counts per market (complete = 386+), the
tape on disk (data/tape/<day>/), the nightly's log. Plus the corpora (ES / NQ BASERATES, the sweep corpus, the book
corpus, the Learn corpus) and the studies by the corpus they wait on.
"""
import glob, io, json, os, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = 'learning/coverage.json'
MARKETS = ('ES', 'NQ', 'GC', 'CL')
COMPLETE = 386      # study-hodlod's MIN_BARS — a session with fewer is dropped from the corpus


def _jload(p):
    try:
        with io.open(p, encoding='utf-8') as f:
            return json.load(f)
    except Exception:
        return None


def _csv_rows(p):
    try:
        with io.open(p, encoding='utf-8') as f:
            return max(0, sum(1 for _ in f) - 1)
    except Exception:
        return None


def day_row(root, path):
    day = os.path.basename(path)[:10]
    d = _jload(path) or {}
    fb = d.get('futBars') or {}
    snaps = d.get('snaps')
    ns = sum(len(v or []) for v in snaps.values()) if isinstance(snaps, dict) else (len(snaps) if isinstance(snaps, list) else 0)
    defl = d.get('defl')
    nd = sum(len(v or []) for v in defl.values()) if isinstance(defl, dict) else (len(defl) if isinstance(defl, list) else 0)
    fut = {}
    for mk in MARKETS:
        m = fb.get(mk) if isinstance(fb, dict) else None
        fut[mk] = dict(n=(m.get('n') if isinstance(m, dict) else None), full=bool(isinstance(m, dict) and m.get('full')), err=(m.get('err') if isinstance(m, dict) else None))
    csv = {}
    for mk in MARKETS:
        n = _csv_rows(os.path.join(root, 'data', 'futures', mk, day + '.csv')) if os.path.exists(os.path.join(root, 'data', 'futures', mk, day + '.csv')) else None
        csv[mk] = dict(n=n, complete=(n is not None and n >= COMPLETE))
    tape_dir = os.path.join(root, 'data', 'tape', day)
    tape = sorted(f[:-5] for f in os.listdir(tape_dir) if f.endswith('.json')) if os.path.isdir(tape_dir) else []
    return dict(day=day, mb=round(os.path.getsize(path) / 1e6, 2), version=d.get('version'), snaps=ns, nodeEvents=len(d.get('nodeEvents') or []),
                defl=nd, feat=bool(d.get('feat')), fut=fut, csv=csv, tape=tape, log=os.path.exists(os.path.join(root, 'learning', 'log', day + '.json')))


def corpora(root):
    out = {}
    for mk, p in (('ES', 'data/es-1min/BASERATES.json'), ('NQ', 'data/futures/NQ/BASERATES.json')):
        j = _jload(os.path.join(root, p))
        if j and j.get('corpus'):
            c = j['corpus']; src = c.get('sources') or {}
            out[mk] = dict(sessions=c.get('sessions'), first=c.get('first'), last=c.get('last'),
                           vendor=sum(v for k, v in src.items() if not k[:4].isdigit()), yahoo=sum(v for k, v in src.items() if k[:4].isdigit()),
                           definition=(c.get('definition') or '')[:80])
    sw = _jload(os.path.join(root, 'data/es-1min/SWEEPS.json'))
    if sw:
        out['sweeps'] = dict(sessions=(sw.get('corpus') or {}).get('sessions'), cells=(sw.get('ledger') or {}).get('cells_read'), levels=len((sw.get('lookup') or {}).get('level') or {}))
    sb = _jload(os.path.join(root, 'data/es-1min/SWEEPS-BOOK.json'))
    if sb:
        out['book'] = dict(sessions=(sb.get('corpus') or {}).get('sessions'), cells=(sb.get('ledger') or {}).get('cells_read'))
    ex = _jload(os.path.join(root, 'learning/deflections/examples.json'))
    if ex:
        out['learn'] = dict(examples=len(ex.get('examples') or []), legs=sum(len(e.get('legs') or []) for e in (ex.get('examples') or [])),
                            rules=len(ex.get('rules') or []), blind=sum(1 for e in (ex.get('examples') or []) if e.get('blind')))
    return out


def studies_need(root):
    st = _jload(os.path.join(root, 'learning/studies.json'))
    if not st:
        return None
    flat = [x for sj in st.get('subjects', []) for ss in sj.get('subsections', []) for x in ss.get('studies', [])]
    by = {}
    for x in flat:
        key = ((x.get('needs') or {}).get('corpus')) or '?'      # (v15.90) the machine-readable need, not a string sniff
        b = by.setdefault(key, dict(studies=0, waiting=0, ready=0))
        b['studies'] += 1
        if x.get('status') == 'WAITING':
            b['waiting'] += 1
        elif x.get('status') == 'READY':
            b['ready'] += 1
    return dict(total=len(flat), byStatus=dict((k, sum(1 for x in flat if x.get('status') == k)) for k in set(x.get('status') for x in flat)), byCorpus=by)


# (v15.90) THE CORPORA THE REGISTRY WAITS ON, COUNTED — one id per corpus a study can name in `needs.corpus`, with what we
# HAVE, the unit, the rate per session (for an ETA) and where it comes from. results.py reads this every night and sets
# WAITING / READY on the rows; the Data tab's ④ reads the same numbers. Nothing here is a claim: every line is a count.
CORPUS_UNITS = dict(price='sessions', nq='sessions', sweeps='sessions', book='sessions', ledger='taps', tap='taps',
                    kingroll='rows', gate='rows', vix='closes', calendar='event days', live='—', register='rows')


def corpus_counts(root=ROOT):
    """-> {corpus id: {have, unit, perSession, sessions, first, note}} — the record, counted, per corpus the studies name."""
    out = {}
    def put(k, have, sessions=None, first=None, note=None):
        ps = (float(have) / sessions) if (sessions and have) else None
        out[k] = dict(have=have, unit=CORPUS_UNITS.get(k, '?'), perSession=(round(ps, 2) if ps is not None else None), sessions=sessions, first=first, note=note)
    C = corpora(root)
    es = C.get('ES') or {}; nq = C.get('NQ') or {}; sw = C.get('sweeps') or {}; bk = C.get('book') or {}
    put('price', es.get('sessions') or 0, es.get('sessions'), es.get('first'), 'ES BASERATES: the vendor + the couriered days')
    put('nq', nq.get('sessions') or 0, nq.get('sessions'), nq.get('first'), 'NQ BASERATES')
    put('sweeps', sw.get('sessions') or 0, sw.get('sessions'), None, 'the sweep corpus (vendor + the couriered nights, v15.90)')
    put('book', bk.get('sessions') or 0, bk.get('sessions'), None, 'the SPY 3-minute book from the day files')
    # the ledger (taps), the King-roll votes and the gatekeeper rows: from the day files themselves
    days = sorted(glob.glob(os.path.join(root, 'data', '20??-??-??.json')))
    n_led = n_kr = n_gate = 0; s_led = s_kr = s_gate = 0; f_led = f_kr = f_gate = None
    for pth in days:
        d = _jload(pth) or {}
        day = os.path.basename(pth)[:10]
        defl = d.get('defl') or {}
        nd = sum(len(v or []) for v in defl.values()) if isinstance(defl, dict) else 0
        if nd:
            n_led += nd; s_led += 1; f_led = f_led or day
        feat = (d.get('feat') or {}).get('SPY') or []
        kr = sum(1 for r in feat if r and r.get('key') == 'dir.kingRoll' and r.get('hit') is not None)
        gt = sum(1 for r in feat if r and r.get('key') == 'gateHour' and r.get('hit') is not None)
        if kr:
            n_kr += kr; s_kr += 1; f_kr = f_kr or day
        if gt:
            n_gate += gt; s_gate += 1; f_gate = f_gate or day
    put('ledger', n_led, s_led, f_led, 'the deflection ledger (±0.50 SPY wobbles) — the tap record replaces it at his scale')
    put('kingroll', n_kr, s_kr, f_kr, 'dir.kingRoll rows with an outcome (per bar, not independent rolls)')
    put('gate', n_gate, s_gate, f_gate, 'gateHour rows with an outcome')
    # the tap record: not built (v15.91) — a file under data/taps/ will be counted here the day it exists
    taps = sorted(glob.glob(os.path.join(root, 'data', 'taps', '20??-??-??.json')))
    n_tap = 0
    for pth in taps:
        t = _jload(pth); n_tap += len(t.get('taps') or []) if isinstance(t, dict) else (len(t) if isinstance(t, list) else 0)
    put('tap', n_tap, len(taps) or None, (os.path.basename(taps[0])[:10] if taps else None), 'THE TAP RECORD — not recorded yet (v15.91)' if not taps else 'data/taps/<day>.json')
    put('vix', 0, None, None, '^VIX daily closes live only in the browser (gpts_vix_daily_v1) — R-35 / a courier file would land them here')
    put('calendar', 0, None, None, 'ForexFactory events live only in the browser (gpts_evcal_v1) — no file in the repo yet')
    put('live', 0, None, None, 'live only — no corpus')
    return out


def build(root=ROOT):
    days = sorted(glob.glob(os.path.join(root, 'data', '20??-??-??.json')))
    rows = [day_row(root, p) for p in days]
    logs = sorted(f for f in glob.glob(os.path.join(root, 'learning', 'log', '20??-??-??.json')))
    last_log = _jload(logs[-1]) if logs else None
    return dict(schema=1, writtenBy='tools/nightly/coverage.py', asOf=time.strftime('%Y-%m-%d'), generatedAt=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
                ranOn=('his machine' if os.name == 'nt' else 'cloud'), complete=COMPLETE, days=rows, corpora=corpora(root), counts=corpus_counts(root), studies=studies_need(root),
                nightly=(dict(date=last_log.get('date'), ranOn=last_log.get('ranOn'), ranAt=last_log.get('ranAt'), sessions=last_log.get('sessions'),
                              episodes=last_log.get('episodes'), deflEvents=last_log.get('deflEvents')) if last_log else None),
                tapeDays=sorted(os.path.basename(p) for p in glob.glob(os.path.join(root, 'data', 'tape', '20??-??-??'))))


def write(root=ROOT):
    j = build(root)
    p = os.path.join(root, *OUT.split('/'))
    with io.open(p, 'w', encoding='utf-8') as f:
        f.write(json.dumps(j, ensure_ascii=False, indent=1))
    return j


def selftest():
    import tempfile, shutil
    root = tempfile.mkdtemp()
    try:
        for d in ('data/futures/ES', 'data/futures/NQ', 'data/tape/2026-09-08', 'learning/log', 'data/es-1min', 'learning/deflections'):
            os.makedirs(os.path.join(root, d))
        io.open(os.path.join(root, 'data', '2026-09-08.json'), 'w').write(json.dumps(dict(version='15.88', snaps=dict(SPY=[1, 2, 3]), nodeEvents=[1] * 5, defl=dict(SPY=[1], QQQ=[]),
                                                                                    feat=dict(SPY=[]), futBars=dict(ES=dict(n=4000, full=True), NQ=dict(n=1400), GC=dict(err='HTTP 500')))))
        io.open(os.path.join(root, 'data', '2026-09-04.json'), 'w').write('{}')
        io.open(os.path.join(root, 'data', 'README.json'), 'w').write('{}')            # not a day file
        io.open(os.path.join(root, 'data', 'futures', 'ES', '2026-09-08.csv'), 'w').write('h\n' + 'r\n' * 393)
        io.open(os.path.join(root, 'data', 'futures', 'NQ', '2026-09-08.csv'), 'w').write('h\n' + 'r\n' * 379)
        io.open(os.path.join(root, 'data', 'tape', '2026-09-08', 'SPXW.json'), 'w').write('{}')
        io.open(os.path.join(root, 'learning', 'log', '2026-09-08.json'), 'w').write(json.dumps(dict(date='2026-09-08', ranOn='his machine', ranAt='2026-09-08T20:05:00Z', sessions=3, episodes=23, deflEvents=141)))
        io.open(os.path.join(root, 'data', 'es-1min', 'BASERATES.json'), 'w').write(json.dumps(dict(corpus=dict(sessions=295, first='2025-06-02', last='2026-09-08', sources={'ES TestingData.txt': 284, '2026-09-08.csv': 1}))))
        j = build(root)
        assert [r['day'] for r in j['days']] == ['2026-09-04', '2026-09-08'], j['days']
        r = j['days'][1]
        assert r['snaps'] == 3 and r['nodeEvents'] == 5 and r['defl'] == 1 and r['feat'] is True
        assert r['fut']['ES'] == dict(n=4000, full=True, err=None) and r['fut']['NQ']['full'] is False and r['fut']['GC']['err'] == 'HTTP 500'
        assert r['csv']['ES'] == dict(n=393, complete=True) and r['csv']['NQ'] == dict(n=379, complete=False) and r['csv']['GC'] == dict(n=None, complete=False)
        assert r['tape'] == ['SPXW'] and r['log'] is True and j['days'][0]['log'] is False and j['days'][0]['tape'] == []
        assert j['corpora']['ES'] == dict(sessions=295, first='2025-06-02', last='2026-09-08', vendor=284, yahoo=1, definition='')
        assert j['nightly']['deflEvents'] == 141 and j['tapeDays'] == ['2026-09-08'] and j['studies'] is None
        c = j['counts']
        assert c['price']['have'] == 295 and c['price']['unit'] == 'sessions' and c['ledger']['have'] == 1 and c['ledger']['sessions'] == 1 and c['tap']['have'] == 0 and 'not recorded' in c['tap']['note'], c
        assert c['kingroll']['have'] == 0 and c['vix']['have'] == 0 and c['live']['unit'] == '—', c
        print('coverage.py selftest ok')
    finally:
        shutil.rmtree(root, ignore_errors=True)


if __name__ == '__main__':
    if '--selftest' in sys.argv:
        selftest()
    else:
        j = write()
        print('wrote %s · %d sessions · corpora %s · studies %s' % (OUT, len(j['days']), ', '.join('%s %s' % (k, (v.get('sessions'))) for k, v in j['corpora'].items()), (j['studies'] or {}).get('total')))
