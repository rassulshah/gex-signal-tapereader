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
        c = x.get('corpus') or '?'
        key = ('tap record' if 'tap record' in c else 'price corpus' if '284d' in c else 'book' if 'book' in c else 'live / API' if ('API' in c or 'live' in c) else 'other')
        b = by.setdefault(key, dict(studies=0, open=0))
        b['studies'] += 1
        if x.get('status') == 'OPEN':
            b['open'] += 1
    return dict(total=len(flat), byStatus=dict((k, sum(1 for x in flat if x.get('status') == k)) for k in set(x.get('status') for x in flat)), byCorpus=by)


def build(root=ROOT):
    days = sorted(glob.glob(os.path.join(root, 'data', '20??-??-??.json')))
    rows = [day_row(root, p) for p in days]
    logs = sorted(f for f in glob.glob(os.path.join(root, 'learning', 'log', '20??-??-??.json')))
    last_log = _jload(logs[-1]) if logs else None
    return dict(schema=1, writtenBy='tools/nightly/coverage.py', asOf=time.strftime('%Y-%m-%d'), generatedAt=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
                ranOn=('his machine' if os.name == 'nt' else 'cloud'), complete=COMPLETE, days=rows, corpora=corpora(root), studies=studies_need(root),
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
        print('coverage.py selftest ok')
    finally:
        shutil.rmtree(root, ignore_errors=True)


if __name__ == '__main__':
    if '--selftest' in sys.argv:
        selftest()
    else:
        j = write()
        print('wrote %s · %d sessions · corpora %s · studies %s' % (OUT, len(j['days']), ', '.join('%s %s' % (k, (v.get('sessions'))) for k, v in j['corpora'].items()), (j['studies'] or {}).get('total')))
