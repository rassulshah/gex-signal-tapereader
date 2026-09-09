#!/usr/bin/env python3
"""tick.py — the "GEX nightly" task's decision (v15.68): run the nightly only when there is something new to read.

    python3 tools/nightly/tick.py            # run tools/nightly/run.py if the newest day file is newer than its log; else exit 3
    python3 tools/nightly/tick.py --check    # print the decision, run nothing
    python3 tools/nightly/tick.py --selftest

Operator, 2026-09-04: "i envision clicking on the save, the data getting saved and the analysis occurring and the
analysis tab being updated." The Windows task (setup-gex-nightly.bat → tools/gex-nightly.bat, hidden, every 10
minutes) calls this. It never parses a date out of cmd's %DATE% (locale-shaped; the sync's commit messages read
"03-Thu-09" for that reason): the newest data/<day>.json IS today, and it needs a run when learning/log/<day>.json is
missing or older than it. A second 💾 makes the day file newer again → one more run. Nothing new → exit 3, silently.
Exit codes: 0 ran · 1 the nightly failed · 3 nothing to do.
"""
import glob, io, os, sys, time
try:   # (v15.72b) UTF-8 into tools\gex-nightly.log on Windows (see run.py)
    sys.stdout.reconfigure(encoding='utf-8', errors='replace'); sys.stderr.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
# (v15.87) what the nightly writes besides the log — if any of these is older than the log, the run was pasted over
OUTPUTS = ('learning/results.json', 'learning/studies.json', 'learning/recommendations.json', 'learning/deflections/examples.json',
           'data/es-1min/BASERATES.json')

def newest_day(root=ROOT):
    files = sorted(glob.glob(os.path.join(root, 'data', '20??-??-??.json')))
    return files[-1] if files else None

def needs_run(root=ROOT):
    """-> (day or None, why)"""
    p = newest_day(root)
    if not p:
        return None, 'no day file under data/'
    day = os.path.basename(p)[:10]
    lp = os.path.join(root, 'learning', 'log', day + '.json')
    if not os.path.exists(lp):
        return day, 'no log for %s yet' % day
    if os.path.getmtime(p) > os.path.getmtime(lp):
        return day, 'data/%s.json is newer than its log' % day
    # (v15.87) AN INSTALLER RAN AFTER THE NIGHTLY. On 2026-09-08 the 15:05 run's results / studies / recommendations /
    # examples were overwritten at 15:21, 17:11 and 18:01 by installers built from a clone that predated the run — the
    # origin guard checks at BUILD time and cannot see an install that comes later. The extracted files carry mtime 0
    # (the tar is diffable on purpose), so an output OLDER than the log means the nightly's work was pasted over: run
    # again, and the day files (the source) put it back.
    for rel in OUTPUTS:
        op = os.path.join(root, *rel.split('/'))
        if os.path.exists(op) and os.path.getmtime(op) < os.path.getmtime(lp) - 1:
            return day, '%s is older than the log for %s — an installer wrote over the nightly\'s outputs' % (rel, day)
    # (v15.87) ...and by CONTENT, for the case where the log itself came out of the same installer (every file at mtime
    # 0, nothing older than anything): results.json says which day it was computed for. Dated before the log's day, it
    # is the pasted-over copy.
    aso = _as_of(os.path.join(root, 'learning', 'results.json'))
    if aso and aso < day:
        return day, 'learning/results.json is dated %s, the log %s — an installer wrote over the nightly\'s outputs' % (aso, day)
    return None, 'the log for %s is current' % day

def _as_of(path):
    try:
        import json
        with io.open(path, encoding='utf-8') as f:
            v = json.load(f).get('asOf')
        return v if isinstance(v, str) and len(v) == 10 else None
    except Exception:
        return None

def main(argv):
    day, why = needs_run(ROOT)
    stamp = time.strftime('%Y-%m-%d %H:%M:%S')
    if '--check' in argv:
        print(stamp, ('RUN for %s — %s' % (day, why)) if day else ('nothing to do — %s' % why)); return 0
    if not day:
        return 3
    print(stamp, 'GEX nightly: running for', day, '—', why)
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import run as _run
    try:
        log = _run.run(day)
    except Exception as e:
        print(stamp, 'GEX nightly FAILED:', e); return 1
    print(stamp, 'GEX nightly: done', day)
    return 0 if log else 1

def selftest():
    import tempfile, json
    root = tempfile.mkdtemp()
    os.makedirs(os.path.join(root, 'data')); os.makedirs(os.path.join(root, 'learning', 'log'))
    assert needs_run(root) == (None, 'no day file under data/')
    dp = os.path.join(root, 'data', '2026-09-08.json'); io.open(dp, 'w').write('{}')
    io.open(os.path.join(root, 'data', 'README.json'), 'w').write('{}')          # not a day file
    assert needs_run(root) == ('2026-09-08', 'no log for 2026-09-08 yet')
    lp = os.path.join(root, 'learning', 'log', '2026-09-08.json'); io.open(lp, 'w').write('{}')
    os.utime(dp, (1000, 1000)); os.utime(lp, (2000, 2000))
    assert needs_run(root) == (None, 'the log for 2026-09-08 is current')
    os.utime(dp, (3000, 3000))                                                     # a second 💾
    assert needs_run(root) == ('2026-09-08', 'data/2026-09-08.json is newer than its log')
    io.open(os.path.join(root, 'data', '2026-09-09.json'), 'w').write('{}')       # the next day
    assert needs_run(root) == ('2026-09-09', 'no log for 2026-09-09 yet')
    # (v15.87) the installer-after-nightly clobber, by mtime: an output older than the log
    lp9 = os.path.join(root, 'learning', 'log', '2026-09-09.json'); io.open(lp9, 'w').write('{}'); os.utime(lp9, (5000, 5000))
    os.utime(os.path.join(root, 'data', '2026-09-09.json'), (4000, 4000))
    rp = os.path.join(root, 'learning', 'results.json'); io.open(rp, 'w').write('{"asOf": "2026-09-09"}'); os.utime(rp, (5000, 5000))
    assert needs_run(root) == (None, 'the log for 2026-09-09 is current')
    os.utime(rp, (0, 0))                                                           # pasted by an installer (mtime 0)
    assert needs_run(root) == ('2026-09-09', 'learning/results.json is older than the log for 2026-09-09 — an installer wrote over the nightly\'s outputs')
    # ...and by content: both out of the installer (mtime 0 each), the results dated before the log's day
    os.utime(lp9, (0, 0)); os.utime(os.path.join(root, 'data', '2026-09-09.json'), (0, 0))
    io.open(rp, 'w').write('{"asOf": "2026-09-08"}'); os.utime(rp, (0, 0))
    assert needs_run(root) == ('2026-09-09', 'learning/results.json is dated 2026-09-08, the log 2026-09-09 — an installer wrote over the nightly\'s outputs')
    io.open(rp, 'w').write('{"asOf": "2026-09-09"}'); os.utime(rp, (0, 0))
    assert needs_run(root) == (None, 'the log for 2026-09-09 is current')
    print('tick.py selftest ok')

if __name__ == '__main__':
    if '--selftest' in sys.argv:
        selftest()
    else:
        sys.exit(main(sys.argv[1:]))
