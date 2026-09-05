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

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

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
    return None, 'the log for %s is current' % day

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
    print('tick.py selftest ok')

if __name__ == '__main__':
    if '--selftest' in sys.argv:
        selftest()
    else:
        sys.exit(main(sys.argv[1:]))
