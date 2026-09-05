#!/usr/bin/env python3
"""origin-guard.py — (v15.74b) THE INSTALLER MUST NEVER CARRY A STALE COPY OF A FILE HIS MACHINE WRITES.

    python3 tools/origin-guard.py                 # check every file the installer carries; adopt / report
    python3 tools/origin-guard.py --keep-mine=learning/recommendations.json   # a conflict resolved: the cloud's copy ships
    python3 tools/origin-guard.py --selftest

What happened (2026-09-04, 23:41 CT). His "GEX nightly" task had run the 9/4 analysis on his machine at 22:35 and the
sync task had pushed its seven outputs (the log, results, studies, examples, recommendations, SWEEPS, SWEEPS-BOOK).
The cloud built v15.74 from a clone that still held the CLOUD's earlier copies of the same files; the installer
extracts its payload over the repo, so it overwrote his machine's run with the cloud's — the log's `ranOn` went from
"his machine" back to "cloud", the Analysis tab named the cloud at 19:44, and he asked "double check .. look at
analysis". The numbers were identical; the record of WHO ran it was wrong, and the mechanism can overwrite real
differences just as silently. (The task re-ran at 23:45 because the extracted log carries mtime 0 — the tar is
diffable on purpose — and healed the record on its own; the guard is so it never needs to.)

The rule. Before a build, for every file the installer carries: if origin has moved it since the clone's base
(`git merge-base HEAD origin/main`) and the cloud never touched it, ADOPT origin's bytes into the working tree
(the cloud was carrying a stale copy). If both moved it and it is one of the files his machine writes, adopt
origin's when the numbers are the same (his Python writes CRLF and float noise in the 16th digit) — his machine's
run IS the process's run; the runner stamps (`ranOn` · `ranAt` · `writtenBy`) are his to set. Otherwise it is a
CONFLICT: the build refuses until it is merged by hand or the file is named in --keep-mine. Adopting changes the
working tree, so this runs BEFORE the commit (BUILD-CHECKLIST §1a); build-installer.py runs it again and refuses if
anything is left to adopt — HEAD, the tree and the payload stay one thing. The cloud can fetch; only push is blocked.
"""
import io, json, math, os, subprocess, sys

NIGHTLY_WRITES = ('learning/log/', 'learning/results.json', 'learning/studies.json', 'learning/recommendations.json',
                  'learning/deflections/examples.json', 'learning/items.json', 'learning/requests.json',
                  'data/es-1min/SWEEPS.json', 'data/es-1min/SWEEPS-BOOK.json')
STAMPS = ('ranOn', 'ranAt', 'writtenBy')


def _git(args, cwd):
    return subprocess.run(['git'] + args, capture_output=True, cwd=cwd)


def blob(ref, path, cwd='.'):
    r = _git(['show', ref + ':' + path], cwd)
    return r.stdout if r.returncode == 0 else None


def same_numbers(a, b, ignore=STAMPS, rel=1e-9):
    """Two JSON documents with the same content up to float noise and the runner stamps."""
    try:
        x = json.loads(a); y = json.loads(b)
    except Exception:
        return False

    def eq(p, q):
        if isinstance(p, dict) and isinstance(q, dict):
            kp = set(k for k in p if k not in ignore); kq = set(k for k in q if k not in ignore)
            return kp == kq and all(eq(p[k], q[k]) for k in kp)
        if isinstance(p, list) and isinstance(q, list):
            return len(p) == len(q) and all(eq(u, v) for u, v in zip(p, q))
        if isinstance(p, bool) or isinstance(q, bool):
            return isinstance(p, bool) and isinstance(q, bool) and p == q      # True == 1 in Python; not here
        if isinstance(p, (int, float)) and isinstance(q, (int, float)):
            return math.isclose(p, q, rel_tol=rel, abs_tol=1e-12)
        return p == q
    return eq(x, y)


def check(files, cwd='.', origin='origin/main', keep=(), write=True):
    """-> (adopted [(path, why)], conflicts [path]). Adopting writes origin's bytes into the working tree."""
    mb = _git(['merge-base', 'HEAD', origin], cwd)
    base = mb.stdout.decode().strip() if mb.returncode == 0 else None
    adopted, conflicts = [], []
    for p in files:
        o = blob(origin, p, cwd)
        if o is None:
            continue                                   # origin does not have it: the cloud's copy ships
        try:
            with io.open(os.path.join(cwd, p), 'rb') as fh:
                w = fh.read()
        except OSError:
            w = None
        if w == o:
            continue                                   # the same bytes: nothing to decide
        b = blob(base, p, cwd) if base else None
        if o == b:
            continue                                   # origin has not moved since the base: the cloud's edit wins
        if o in cloud_versions(p, base, cwd):
            continue                                   # origin's copy is one the cloud itself made (pushed by his installer)
        if p in keep:
            continue                                   # a conflict he/I resolved: the cloud's copy ships
        why = None
        if w == b or w in origin_versions(p, base, origin, cwd):
            why = 'origin moved it and the cloud never touched it'
        elif p.startswith(NIGHTLY_WRITES) and same_numbers(w, o):
            why = 'his machine rewrote it with the same numbers (stamps / CRLF / float noise)'
        if why:
            if write:
                with io.open(os.path.join(cwd, p), 'wb') as fh:
                    fh.write(o)
            adopted.append((p, why))
        else:
            conflicts.append(p)
    return adopted, conflicts


def versions(path, rng, cwd='.'):
    """The blobs `path` had at the commits in `rng` that touched it."""
    r = _git(['rev-list', rng, '--', path], cwd)
    out = set()
    for c in r.stdout.decode().split():
        v = blob(c, path, cwd)
        if v is not None:
            out.add(v)
    return out


def cloud_versions(path, base, cwd='.'):
    """The blobs the cloud committed for `path` since the base (his installer pushes the cloud's commits under new
    hashes, so origin/main is never an ancestor of HEAD here; a file origin holds in one of these versions was
    moved by the cloud, not by his machine)."""
    return versions(path, (base + '..HEAD') if base else 'HEAD', cwd)


def origin_versions(path, base, origin, cwd='.'):
    """The blobs origin has held for `path` since the base — a working copy equal to one of them was adopted from
    origin earlier, not written by the cloud."""
    return versions(path, (base + '..' + origin) if base else origin, cwd)


def manifest():
    r = subprocess.run([sys.executable, os.path.join('tools', 'build-installer.py'), '--list'], capture_output=True, text=True)
    return [l.strip() for l in r.stdout.split('\n') if l.strip()]


def selftest():
    import tempfile, shutil
    root = tempfile.mkdtemp()
    env = dict(os.environ, GIT_AUTHOR_NAME='t', GIT_AUTHOR_EMAIL='t@t', GIT_COMMITTER_NAME='t', GIT_COMMITTER_EMAIL='t@t')

    def g(*a):
        r = subprocess.run(['git'] + list(a), cwd=root, capture_output=True, text=True, env=env)
        assert r.returncode == 0, (a, r.stderr)
        return r.stdout

    def put(rel, text):
        p = os.path.join(root, rel); os.makedirs(os.path.dirname(p), exist_ok=True)
        with io.open(p, 'w', encoding='utf-8', newline='') as fh:
            fh.write(text)
    g('init', '-q', '-b', 'main')
    LOG = 'learning/log/2026-09-04.json'; DOC = 'design/DOC.md'; REC = 'learning/recommendations.json'
    RES = 'learning/results.json'; SW = 'data/es-1min/SWEEPS.json'; ITEMS = 'learning/items.json'
    put(LOG, '{"date":"2026-09-04","ranOn":"cloud","ranAt":"00:44Z","n":78}\n')
    put(DOC, 'doc\n'); put(REC, '{"rows":[{"id":"R-5","text":"v15.74"}]}\n'); put(RES, '{"x":0.2130072096618035}\n')
    put(SW, '{"a":1}\n'); put(ITEMS, '{"items":[]}\n')
    g('add', '-A'); g('commit', '-q', '-m', 'base')
    # "his machine": the nightly rewrote the log (stamps), results (float noise, CRLF), rec (CRLF only), SWEEPS (a real change), items (a new item)
    g('checkout', '-q', '-b', 'theirs')
    put(LOG, '{"date":"2026-09-04","ranOn":"his machine","ranAt":"04:45Z","n":78}\r\n')
    put(RES, '{"x":0.21300720966180334}\r\n'); put(REC, '{"rows":[{"id":"R-5","text":"v15.74"}]}\r\n')
    put(SW, '{"a":2}\n'); put(ITEMS, '{"items":[{"id":"I1"}]}\n')
    g('add', '-A'); g('commit', '-q', '-m', 'gex: sync')
    # "the cloud": edited the doc and the rec text; touched nothing else
    g('checkout', '-q', 'main')
    put(DOC, 'doc2\n'); put(REC, '{"rows":[{"id":"R-5","text":"v15.75"}]}\n')
    files = [LOG, DOC, REC, RES, SW, ITEMS, 'never/there.json']
    adopted, conflicts = check(files, cwd=root, origin='theirs')
    A = dict(adopted)
    assert set(A) == {LOG, RES, SW, ITEMS}, A
    assert 'never touched' in A[SW] and 'never touched' in A[ITEMS] and 'never touched' in A[LOG] and 'never touched' in A[RES], A
    assert conflicts == [REC], conflicts                          # the cloud changed the text, his machine the bytes: a real conflict
    assert io.open(os.path.join(root, LOG), 'rb').read() == b'{"date":"2026-09-04","ranOn":"his machine","ranAt":"04:45Z","n":78}\r\n'
    assert io.open(os.path.join(root, SW), 'rb').read() == b'{"a":2}\n'
    assert io.open(os.path.join(root, DOC), 'rb').read() == b'doc2\n'                    # the cloud's edit untouched
    assert io.open(os.path.join(root, REC), 'rb').read() == b'{"rows":[{"id":"R-5","text":"v15.75"}]}\n'   # a conflict is never overwritten
    # a second pass finds nothing left to adopt; the conflict stands until named
    adopted2, conflicts2 = check(files, cwd=root, origin='theirs')
    assert adopted2 == [] and conflicts2 == [REC], (adopted2, conflicts2)
    assert check(files, cwd=root, origin='theirs', keep=(REC,)) == ([], [])
    # both moved a machine-written file, same numbers up to stamps and noise → adopt his machine's bytes
    g('add', '-A'); g('commit', '-q', '-m', 'cloud')                                      # the cloud commits what it adopted (base stays the merge-base)
    g('checkout', '-q', 'theirs')                                                         # his machine ran again…
    put(LOG, '{"date":"2026-09-04","ranOn":"his machine","ranAt":"05:00Z","n":78}\r\n'); put(RES, '{"x":0.21300720966180330}\r\n')
    g('add', '-A'); g('commit', '-q', '-m', 'gex: sync 2'); g('checkout', '-q', 'main')
    put(RES, '{"x":0.21300720966180350}\n')                                              # …and so did the cloud
    put(LOG, '{"date":"2026-09-04","ranOn":"cloud","ranAt":"01:00Z","n":78}\n')
    adopted3, conflicts3 = check([LOG, RES], cwd=root, origin='theirs')
    A3 = dict(adopted3)
    assert set(A3) == {LOG, RES} and all('same numbers' in v for v in A3.values()), adopted3
    assert conflicts3 == []
    # a real difference in a machine-written file both sides changed is a conflict, not noise
    put(LOG, '{"date":"2026-09-04","ranOn":"cloud","ranAt":"01:00Z","n":79}\n')
    assert check([LOG], cwd=root, origin='theirs') == ([], [LOG])
    # a doc only the cloud changed, with origin unmoved, is never touched (the cloud's edit wins)
    assert check([DOC], cwd=root, origin='theirs') == ([], [])
    g('add', '-A'); g('commit', '-q', '-m', 'cloud 2')                                    # (the tree must be clean to switch branches below)
    # the installer pushes the cloud's commits under new hashes: origin then holds a version the CLOUD made. That is
    # not his machine moving the file — the cloud's next edit must not read as a conflict (build-installer.py did, once)
    g('checkout', '-q', 'theirs'); put(DOC, 'doc2\n'); g('add', '-A'); g('commit', '-q', '-m', 'v15.xx (his installer)')
    g('checkout', '-q', 'main'); put(DOC, 'doc3\n')
    assert check([DOC], cwd=root, origin='theirs') == ([], []), check([DOC], cwd=root, origin='theirs', write=False)
    assert io.open(os.path.join(root, DOC), 'rb').read() == b'doc3\n'
    # …while a file his machine moved past the cloud's own version is still adopted
    g('checkout', '-q', 'theirs'); put(SW, '{"a":3}\n'); g('add', '-A'); g('commit', '-q', '-m', 'gex: sync'); g('checkout', '-q', 'main')
    assert dict(check([SW], cwd=root, origin='theirs')[0]).get(SW, '').startswith('origin moved it'), 'SW should adopt'
    assert io.open(os.path.join(root, SW), 'rb').read() == b'{"a":3}\n'
    # same_numbers: stamps ignored, bools are not numbers, lists by position
    assert same_numbers(b'{"ranOn":"a","v":[1,2.0000000000001]}', b'{"ranOn":"b","v":[1,2]}')
    assert not same_numbers(b'{"v":true}', b'{"v":1}') and not same_numbers(b'{"v":[1,2]}', b'{"v":[2,1]}') and not same_numbers(b'x', b'x')
    shutil.rmtree(root, ignore_errors=True)
    print('origin-guard selftest ok')


def main(argv):
    if '--selftest' in argv:
        selftest(); return 0
    keep = tuple(a.split('=', 1)[1] for a in argv if a.startswith('--keep-mine='))
    f = _git(['fetch', 'origin', 'main'], '.')
    if f.returncode != 0:
        print('origin-guard: git fetch failed — cannot compare with origin; nothing adopted'); return 3
    adopted, conflicts = check(manifest(), '.', 'origin/main', keep, write=('--dry' not in argv))
    for p, why in adopted:
        print(('WOULD ADOPT' if '--dry' in argv else 'ADOPTED') + ' from origin: %s  (%s)' % (p, why))
    if conflicts:
        print('ORIGIN CONFLICT — origin moved these since the clone and the cloud changed them too. Merge by hand'
              ' (git show origin/main:<path>), or ship the cloud\'s copy on purpose with --keep-mine=<path>:')
        for p in conflicts:
            print('  ' + p)
        return 2
    if not adopted:
        print('origin-guard: the installer carries nothing origin has moved past')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
